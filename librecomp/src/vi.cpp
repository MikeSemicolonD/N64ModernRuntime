#include <ultramodern/ultramodern.hpp>
#include "recomp.h"
#include "helpers.hpp"
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>

static std::atomic<uint64_t> g_swap_count{0};
static std::atomic<uint64_t> g_sample_count{0};
static std::atomic<uint32_t> g_last_swap_fb{0};
static std::atomic<uint32_t> g_last_sample_fb{0};
static const bool g_log_rate = []{ const char* e = std::getenv("ROGUESQ_LOG_FRAME_RATE"); return e && *e == '1'; }();

static void log_rate_periodic(const char* who) {
    if (!g_log_rate) return;
    static std::atomic<int64_t> last_log_ms{0};
    static std::atomic<uint64_t> prev_swap{0};
    static std::atomic<uint64_t> prev_sample{0};
    using clock = std::chrono::steady_clock;
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count();
    int64_t prev = last_log_ms.load(std::memory_order_relaxed);
    if (now_ms - prev < 1000) return;
    if (!last_log_ms.compare_exchange_strong(prev, now_ms)) return;
    uint64_t s = g_swap_count.load(), v = g_sample_count.load();
    uint64_t ds = s - prev_swap.exchange(s);
    uint64_t dv = v - prev_sample.exchange(v);
    fprintf(stderr,
        "[rate] @%s window~%lldms: swap +%llu (tot=%llu, lastFb=0x%08X)  sample +%llu (tot=%llu, lastFb=0x%08X)\n",
        who, (long long)(now_ms - prev),
        (unsigned long long)ds, (unsigned long long)s, (unsigned)g_last_swap_fb.load(),
        (unsigned long long)dv, (unsigned long long)v, (unsigned)g_last_sample_fb.load());
    fflush(stderr);
}

extern "C" void roguesq_rate_count_swap(uint32_t fb_addr) {
    g_last_swap_fb.store(fb_addr, std::memory_order_relaxed);
    g_swap_count.fetch_add(1, std::memory_order_relaxed);
    log_rate_periodic("swap");
}

extern "C" void roguesq_rate_count_sample(uint32_t fb_addr) {
    g_last_sample_fb.store(fb_addr, std::memory_order_relaxed);
    g_sample_count.fetch_add(1, std::memory_order_relaxed);
    log_rate_periodic("sample");
}

extern "C" void osViSetYScale_recomp(uint8_t* rdram, recomp_context * ctx) {
    osViSetYScale(ctx->f12.fl);
}

extern "C" void osViSetXScale_recomp(uint8_t* rdram, recomp_context * ctx) {
    osViSetXScale(ctx->f12.fl);
}

extern "C" void osCreateViManager_recomp(uint8_t* rdram, recomp_context* ctx) {
    ;
}

extern "C" void osViBlack_recomp(uint8_t* rdram, recomp_context* ctx) {
    osViBlack((uint32_t)ctx->r4);
}

extern "C" void osViRepeatLine_recomp(uint8_t* rdram, recomp_context* ctx) {
    osViRepeatLine(_arg<0, u8>(rdram, ctx));
}

extern "C" void osViSetSpecialFeatures_recomp(uint8_t* rdram, recomp_context* ctx) {
    osViSetSpecialFeatures((uint32_t)ctx->r4);
}

extern "C" void osViGetCurrentFramebuffer_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = (gpr)(int32_t)osViGetCurrentFramebuffer();
}

extern "C" void osViGetNextFramebuffer_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = (gpr)(int32_t)osViGetNextFramebuffer();
}

extern "C" void osViSwapBuffer_recomp(uint8_t* rdram, recomp_context* ctx) {
    roguesq_rate_count_swap((uint32_t)ctx->r4);
    osViSwapBuffer(rdram, (int32_t)ctx->r4);
}

extern "C" void osViSetMode_recomp(uint8_t* rdram, recomp_context* ctx) {
    osViSetMode(rdram, (int32_t)ctx->r4);
}

extern uint64_t total_vis;

extern "C" void osViGetCurrentField_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = 0; // always field 0 (progressive / non-interlaced)
}

extern "C" void wait_one_frame(uint8_t* rdram, recomp_context* ctx) {
    uint64_t cur_vis = total_vis;
    while (cur_vis == total_vis) {
        std::this_thread::yield();
    }
}
