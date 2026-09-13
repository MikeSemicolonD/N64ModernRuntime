#include "ultramodern/ultra64.h"
#include "ultramodern/ultramodern.hpp"
#include <cassert>
#include <atomic>

static uint32_t sample_rate = 48000;
// RogueSquadron64Recomp: consumption-paced AI interrupts (events.cpp) need the rate and the last buffer size.
std::atomic<uint32_t> g_rs64_ai_last_bytes{0};
uint32_t rs64_audio_sample_rate() { return sample_rate; }
// One AI interrupt per queued buffer, when that buffer finishes playing (hardware DMA-complete model).
// The playhead advances by each buffer's duration; the AI thread (events.cpp) fires at those deadlines.
#include <deque>
#include <mutex>
#include <chrono>
std::mutex g_rs64_ai_mutex;
std::deque<std::chrono::steady_clock::time_point> g_rs64_ai_deadlines;
static std::chrono::steady_clock::time_point g_rs64_ai_playhead{};
static void rs64_ai_schedule(uint32_t byte_count) {
    const int64_t dur_us = (sample_rate > 0) ? (int64_t)byte_count * 1000000 / (4 * (int64_t)sample_rate) : 16667;
    std::lock_guard<std::mutex> lk(g_rs64_ai_mutex);
    const auto now = std::chrono::steady_clock::now();
    // FREE-RUNNING consumption clock (2026-09-13). Hardware's AI DMA consumes at a FIXED rate regardless of
    // when the CPU refills; the game produces each buffer ~1.1ms AFTER its deadline (wake+synth latency).
    // The old `if (playhead < now) playhead = now` baked that per-buffer latency into the timeline every
    // buffer, drifting the consumption clock ~11% slow -> steady underproduction (89%) -> stutter. Instead
    // advance the playhead on a fixed timeline so that latency is a one-time constant offset, not a drift;
    // resync to now only on a REAL gap (fell >3 buffer-durations behind = genuine DMA idle / audio pause).
    static const bool s_freerun = [](){ const char* e = std::getenv("ROGUESQ_AI_FIXED_CLOCK"); return !(e && e[0] == '0'); }();
    if (!s_freerun) { if (g_rs64_ai_playhead < now) g_rs64_ai_playhead = now; }
    else if (g_rs64_ai_playhead < now - std::chrono::microseconds(3 * dur_us)) { g_rs64_ai_playhead = now; }
    g_rs64_ai_playhead += std::chrono::microseconds(dur_us);
    g_rs64_ai_deadlines.push_back(g_rs64_ai_playhead);
}

static ultramodern::audio_callbacks_t audio_callbacks;

void ultramodern::set_audio_callbacks(const ultramodern::audio_callbacks_t& callbacks) {
    audio_callbacks = callbacks;
}

void ultramodern::init_audio() {
    // Pick an initial dummy sample rate; this will be set by the game later to the true sample rate.
    set_audio_frequency(48000);
}

void ultramodern::set_audio_frequency(uint32_t freq) {
    if (audio_callbacks.set_frequency) {
        audio_callbacks.set_frequency(freq);
    }
    sample_rate = freq;
}

void ultramodern::queue_audio_buffer(RDRAM_ARG PTR(int16_t) audio_data_, uint32_t byte_count) {
    // With async audio, the synth fills this buffer on the SP thread after sp_complete
    // already fired. Wait for it to finish so we never queue a half-synthesized buffer.
    ultramodern::wait_for_pending_audio_synth();

    // Ensure that the byte count is an integer multiple of samples.
    assert((byte_count & 1) == 0);

    // Calculate the number of samples from the number of bytes.
    uint32_t sample_count = byte_count / sizeof(int16_t);
    g_rs64_ai_last_bytes.store(byte_count, std::memory_order_relaxed);
    rs64_ai_schedule(byte_count);

    // Queue the swapped audio data.
    if (sample_count > 0 && audio_callbacks.queue_samples) {
        audio_callbacks.queue_samples(TO_PTR(int16_t, audio_data_), sample_count);
    }
}

// For SDL2
//uint32_t buffer_offset_frames = 1;
// For Godot
float buffer_offset_frames = 0.5f;

// If there's ever any audio popping, check here first. Some games are very sensitive to
// the remaining sample count and reporting a number that's too high here can lead to issues.
// Reporting a number that's too low can lead to audio lag in some games.
uint32_t ultramodern::get_remaining_audio_bytes() {
    // Get the number of remaining buffered audio bytes.
    uint32_t buffered_byte_count;
    if (audio_callbacks.get_frames_remaining != nullptr) {
        buffered_byte_count = audio_callbacks.get_frames_remaining() * 2 * sizeof(int16_t);
    }
    else {
        buffered_byte_count = 100;
    }
    // Adjust the reported count to be some number of refreshes in the future, which helps ensure that
    // there are enough samples even if the audio thread experiences a small amount of lag. This prevents
    // audio popping on games that use the buffered audio byte count to determine how many samples
    // to generate.
    uint32_t samples_per_vi = (sample_rate / 60);
    if (buffered_byte_count > static_cast<uint32_t>(buffer_offset_frames * sizeof(int16_t) * samples_per_vi)) {
        buffered_byte_count -= static_cast<uint32_t>(buffer_offset_frames * sizeof(int16_t) * samples_per_vi);
    }
    else {
        buffered_byte_count = 0;
    }
    return buffered_byte_count;
}
