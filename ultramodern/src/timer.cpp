#include <algorithm>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <variant>
#include <set>
#include <vector>
#include "blockingconcurrentqueue.h"

#include "ultramodern/ultra64.h"
#include "ultramodern/ultramodern.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#endif

// Start time for the program
static std::chrono::high_resolution_clock::time_point start_time = std::chrono::high_resolution_clock::now();
// Offset of the duration since program start used to calculate the value for osGetTime. 
static int64_t ostime_offset = 0;
// N64 CPU counter ticks per millisecond of game time
static constexpr uint64_t counter_per_ms = 46'875;

static double clamp_speed(double rate) {
    return std::clamp(rate, 1.0 / 16.0, 16.0);
}

// The game clock (VI and CPU counter) runs at `rate` times real time from (base_real, base_us); ROGUESQ_SPEED seeds the rate and set_speed rebases, so the clock never jumps.
struct GameClock {
    std::chrono::high_resolution_clock::time_point base_real;
    double base_us;
    double rate;
};

static GameClock make_game_clock() {
    const char* e = std::getenv("ROGUESQ_SPEED");
    const double v = (e && e[0]) ? std::atof(e) : 1.0;
    return { start_time, 0.0, v > 0.0 ? clamp_speed(v) : 1.0 };
}

static std::mutex game_clock_mutex;
static GameClock game_clock = make_game_clock();

static double game_us_at(const GameClock& c, std::chrono::high_resolution_clock::time_point t) {
    return c.base_us + std::chrono::duration<double, std::micro>(t - c.base_real).count() * c.rate;
}

// Read under the lock together with now(), so the clock stays monotonic across a rate change.
static double game_us_now() {
    std::lock_guard<std::mutex> lock(game_clock_mutex);
    return game_us_at(game_clock, std::chrono::high_resolution_clock::now());
}

struct OSTimer {
    PTR(OSTimer) unused1;
    PTR(OSTimer) unused2;
    OSTime interval;
    OSTime timestamp;
    PTR(OSMesgQueue) mq;
    OSMesg msg;
};

struct AddTimerAction {
    PTR(OSTimer) timer;
};

struct RemoveTimerAction {
    PTR(OSTimer) timer;
};

// Wakes the timer thread to recompute its wait after a speed change.
struct WakeAction {};

using Action = std::variant<AddTimerAction, RemoveTimerAction, WakeAction>;

struct {
    std::thread thread;
    moodycamel::BlockingConcurrentQueue<Action> action_queue{};
} timer_context;

std::chrono::high_resolution_clock::time_point ticks_to_timepoint(uint64_t ticks) {
    return ultramodern::game_time_to_real(std::chrono::microseconds(ticks * 1000 / counter_per_ms));
}

uint64_t time_now() {
    // Units: (micros * (counts/millis)) / (micros/millis) = counts
    return ((uint64_t)game_us_now() * counter_per_ms) / 1000;
}

void timer_thread(RDRAM_ARG1) {
    ultramodern::set_native_thread_name("Timer Thread");
    ultramodern::set_native_thread_priority(ultramodern::ThreadPriority::VeryHigh);

    // Lambda comparator function to keep the set ordered
    auto timer_sort = [PASS_RDRAM1](PTR(OSTimer) a_, PTR(OSTimer) b_) {
        OSTimer* a = TO_PTR(OSTimer, a_);
        OSTimer* b = TO_PTR(OSTimer, b_);

        // Order by timestamp if the timers have different timestamps
        if (a->timestamp != b->timestamp) {
            return a->timestamp < b->timestamp;
        }

        // If they have the exact same timestamp then order by address instead
        return a < b;
    };

    // Ordered set of timers that are currently active
    std::set<PTR(OSTimer), decltype(timer_sort)> active_timers{timer_sort};
    
    // Lambda to process a timer action to handle adding and removing timers
    auto process_timer_action = [&](const Action& action) {
        // Determine the action type and act on it
        if (const auto* add_action = std::get_if<AddTimerAction>(&action)) {
            active_timers.insert(add_action->timer);
        } else if (const auto* remove_action = std::get_if<RemoveTimerAction>(&action)) {
            active_timers.erase(remove_action->timer);
        }
    };

    while (true) {
        // Empty the action queue
        Action cur_action;
        while (timer_context.action_queue.try_dequeue(cur_action)) {
            process_timer_action(cur_action);
        }

        // If there's no timer to act on, wait for one to come in from the action queue
        while (active_timers.empty()) {
            timer_context.action_queue.wait_dequeue(cur_action);
            process_timer_action(cur_action);
        }

        // Get the timer that's closest to running out
        PTR(OSTimer) cur_timer_ = *active_timers.begin();
        OSTimer* cur_timer = TO_PTR(OSTimer, cur_timer_);

        // Remove the timer from the queue (it may get readded if waiting is interrupted)
        active_timers.erase(cur_timer_);

        // Determine how long to wait to reach the timer's timestamp
        auto wait_duration = ticks_to_timepoint(cur_timer->timestamp) - std::chrono::high_resolution_clock::now();

        // Wait for either the duration to complete or a new action to come through
        if (wait_duration.count() >= 0 && timer_context.action_queue.wait_dequeue_timed(cur_action, wait_duration)) {
            // Timer was interrupted by a new action 
            // Add the current timer back to the queue (done first in case the action is to remove this timer)
            active_timers.insert(cur_timer_);
            // Process the new action
            process_timer_action(cur_action);
        }
        else {
            // Waiting for the timer completed, so send the timer's message to its message queue
            ultramodern::enqueue_external_message_src(cur_timer->mq, cur_timer->msg, false, ultramodern::EventMessageSource::Timer);
            // If the timer has a specified interval then reload it with that value
            if (cur_timer->interval != 0) {
                cur_timer->timestamp = cur_timer->interval + time_now();
                active_timers.insert(cur_timer_);
            }
        }
    }
}

void ultramodern::init_timers(RDRAM_ARG1) {
    timer_context.thread = std::thread{ timer_thread, PASS_RDRAM1 };
    timer_context.thread.detach();
}

double ultramodern::get_speed() {
    std::lock_guard<std::mutex> lock(game_clock_mutex);
    return game_clock.rate;
}

void ultramodern::set_speed(double rate) {
    std::lock_guard<std::mutex> lock(game_clock_mutex);
    const auto now = std::chrono::high_resolution_clock::now();
    game_clock.base_us = game_us_at(game_clock, now);
    game_clock.base_real = now;
    game_clock.rate = clamp_speed(rate);
    timer_context.action_queue.enqueue(WakeAction{});
}

std::chrono::microseconds ultramodern::game_time_since_start() {
    return std::chrono::microseconds((int64_t)game_us_now());
}

std::chrono::high_resolution_clock::time_point ultramodern::game_time_to_real(std::chrono::microseconds game_time) {
    std::lock_guard<std::mutex> lock(game_clock_mutex);
    const std::chrono::duration<double, std::micro> real_us((game_time.count() - game_clock.base_us) / game_clock.rate);
    return game_clock.base_real + std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(real_us);
}

std::chrono::high_resolution_clock::time_point ultramodern::get_start() {
    return start_time;
}

std::chrono::high_resolution_clock::duration ultramodern::time_since_start() {
    return std::chrono::high_resolution_clock::now() - start_time;
}

extern "C" u32 osGetCount() {
    uint64_t total_count = time_now();

    // Allow for overflows, which is how osGetCount behaves
    return (uint32_t)total_count;
}

extern "C" void osSetCount(u32 count) {
    assert(false);
}

extern "C" OSTime osGetTime() {
    uint64_t total_count = time_now() - ostime_offset;

    return total_count;
}

extern "C" void osSetTime(OSTime t) {
    ostime_offset = time_now() - t;
}

extern "C" int osSetTimer(RDRAM_ARG PTR(OSTimer) t_, OSTime countdown, OSTime interval, PTR(OSMesgQueue) mq, OSMesg msg) {
    OSTimer* t = TO_PTR(OSTimer, t_);

    // Determine the time when this timer will trigger off
    if (countdown == 0) {
        // Set the timestamp based on the interval
        t->timestamp = interval + time_now();
    } else {
        t->timestamp = countdown + time_now();
    }
    t->interval = interval;
    t->mq = mq;
    t->msg = msg;

    timer_context.action_queue.enqueue(AddTimerAction{ t_ });

    return 0;
}

extern "C" int osStopTimer(RDRAM_ARG PTR(OSTimer) t_) {
    timer_context.action_queue.enqueue(RemoveTimerAction{ t_ });

    // TODO don't blindly return 0 here; requires some response from the timer thread to know what the returned value was
    return 0;
}

#ifdef _WIN32

// The implementations of std::chrono::sleep_until and sleep_for were affected by changing the system clock backwards in older versions
// of Microsoft's STL. This was fixed as of Visual Studio 2022 17.9, but to be safe ultramodern uses Win32 Sleep directly.
void ultramodern::sleep_milliseconds(uint32_t millis) {
    Sleep(millis);
}

void ultramodern::sleep_until(const std::chrono::high_resolution_clock::time_point& time_point) {
    // Sleep rounds up to whole milliseconds: a 240 Hz VI tick waited 5-6 ms, and the game counts ticks, so 4-tick frames ran near 45 fps instead of 60.
    thread_local HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    auto time_now = std::chrono::high_resolution_clock::now();
    if (time_point <= time_now) {
        return;
    }

    if (timer != nullptr) {
        LARGE_INTEGER due;
        const long long ticks = std::chrono::duration_cast<std::chrono::nanoseconds>(time_point - time_now).count() / 100;
        due.QuadPart = -(ticks > 0 ? ticks : 1);
        if (SetWaitableTimerEx(timer, &due, 0, nullptr, nullptr, nullptr, 0)) {
            WaitForSingleObject(timer, INFINITE);
            return;
        }
    }

    long long delta_ms = std::chrono::ceil<std::chrono::milliseconds>(time_point - time_now).count();
    Sleep(delta_ms);
}

void* rs64_alloc_write_watched(size_t size) {
    return VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT | MEM_WRITE_WATCH, PAGE_READWRITE);
}

// Byte offsets of the 4 KB pages in [base, base + size) written since the last call, resetting the watch; false when the range isn't write-watched.
bool rs64_written_pages(const void* base, size_t size, std::vector<uint32_t>& offsets) {
    thread_local std::vector<PVOID> addrs;
    addrs.resize(size / 0x1000 + 1);
    ULONG_PTR count = addrs.size();
    ULONG granularity = 0;
    offsets.clear();
    if (GetWriteWatch(WRITE_WATCH_FLAG_RESET, const_cast<void*>(base), size, addrs.data(), &count, &granularity) != 0 || granularity != 0x1000) {
        return false;
    }

    for (ULONG_PTR i = 0; i < count; i++) {
        offsets.push_back(uint32_t(static_cast<uint8_t*>(addrs[i]) - static_cast<const uint8_t*>(base)));
    }

    return true;
}

#else

void* rs64_alloc_write_watched(size_t size) {
    return std::calloc(1, size);
}

bool rs64_written_pages(const void*, size_t, std::vector<uint32_t>& offsets) {
    offsets.clear();
    return false;
}

void ultramodern::sleep_milliseconds(uint32_t millis) {
    std::this_thread::sleep_for(std::chrono::milliseconds{millis});
}

void ultramodern::sleep_until(const std::chrono::high_resolution_clock::time_point& time_point) {
    std::this_thread::sleep_until(time_point);
}

#endif
