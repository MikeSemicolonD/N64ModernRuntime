#include "recomp.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <ultramodern/ultra64.h>
#include <ultramodern/ultramodern.hpp>

#define VI_NTSC_CLOCK 48681812

extern "C" void osAiSetFrequency_recomp(uint8_t* rdram, recomp_context* ctx) {
    uint32_t freq = ctx->r4;
    // This makes actual audio frequency more accurate to console, but may not be desirable
    //uint32_t dacRate = (uint32_t)(((float)VI_NTSC_CLOCK / freq) + 0.5f);
    //freq = VI_NTSC_CLOCK / dacRate;
    ctx->r2 = freq;
    ultramodern::set_audio_frequency(freq);
}

extern "C" void osAiSetNextBuffer_recomp(uint8_t* rdram, recomp_context* ctx) {
    // ROGUESQ_LOG_AUDIO_OUT=1: log which DRAM buffer the game hands the AI to play, so we can
    // compare it against the MusyX synth's output buffer (routing-mismatch check).
    static int s_log = -1;
    if (s_log < 0) { const char* e = std::getenv("ROGUESQ_LOG_AUDIO_OUT"); s_log = (e && e[0] && e[0] != '0') ? 1 : 0; }
    static int s_n = 0;
    if (s_log && (++s_n <= 8 || (s_n % 256) == 0)) {
        uint32_t a = (uint32_t)ctx->r4 & 0x00FFFFFFu; uint32_t len = (uint32_t)ctx->r5; uint8_t mx = 0;
        if (a && a + len <= 0x800000u) for (uint32_t i = 0; i < len; ++i) { uint8_t b = rdram[a + i]; if (b > mx) mx = b; }
        fprintf(stderr, "[ai-buffer #%d] play=0x%08X len=0x%X peakByte=%d %s\n", s_n, (uint32_t)ctx->r4, len, (int)mx, mx ? "<-HAS DATA" : "(silent)");
        fflush(stderr);
    }
    ultramodern::queue_audio_buffer(rdram, ctx->r4, ctx->r5);
    ctx->r2 = 0;
}

extern "C" void osAiGetLength_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = ultramodern::get_remaining_audio_bytes();
}

extern "C" void osAiGetStatus_recomp(uint8_t* rdram, recomp_context* ctx) {
    ctx->r2 = 0x00000000; // Pretend the audio DMAs finish instantly
}
