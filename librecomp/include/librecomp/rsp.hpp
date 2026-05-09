#ifndef __RSP_H__
#define __RSP_H__

#include <cstdio>
#include <atomic>

#include "rsp_vu.hpp"
#include "recomp.h"
#include "ultramodern/ultra64.h"

// TODO: Move these to recomp namespace?

enum class RspExitReason {
    Invalid,
    Broke,
    ImemOverrun,
    UnhandledJumpTarget,
    Unsupported,
    SwapOverlay,
    UnhandledResumeTarget
};

struct RspContext {
    uint32_t      r1,  r2,  r3,  r4,  r5,  r6,  r7,
             r8,  r9,  r10, r11, r12, r13, r14, r15,
             r16, r17, r18, r19, r20, r21, r22, r23,
             r24, r25, r26, r27, r28, r29, r30, r31;
    uint32_t dma_mem_address;
    uint32_t dma_dram_address;
    uint32_t jump_target;
    RSP rsp;
    uint32_t resume_address;
    bool resume_delay;
};

using RspUcodeFunc = RspExitReason(uint8_t* rdram, uint32_t ucode_addr);

extern uint8_t dmem[];
extern uint16_t rspReciprocals[512];
extern uint16_t rspInverseSquareRoots[512];

#define RSP_MEM_B(offset, addr) \
    (*reinterpret_cast<int8_t*>(dmem + (0xFFF & (((offset) + (addr)) ^ 3))))

#define RSP_MEM_BU(offset, addr) \
    (*reinterpret_cast<uint8_t*>(dmem + (0xFFF & (((offset) + (addr)) ^ 3))))

static inline uint32_t RSP_MEM_W_LOAD(uint32_t offset, uint32_t addr) {
    uint32_t out;
    for (int i = 0; i < 4; i++) {
        reinterpret_cast<uint8_t*>(&out)[i ^ 3] = RSP_MEM_BU(offset + i, addr);
    }
    return out;
}

static inline void RSP_MEM_W_STORE(uint32_t offset, uint32_t addr, uint32_t val) {
    for (int i = 0; i < 4; i++) {
        RSP_MEM_BU(offset + i, addr) = reinterpret_cast<uint8_t*>(&val)[i ^ 3];
    }
}

static inline uint32_t RSP_MEM_HU_LOAD(uint32_t offset, uint32_t addr) {
    uint16_t out;
    for (int i = 0; i < 2; i++) {
        reinterpret_cast<uint8_t*>(&out)[(i + 2) ^ 3] = RSP_MEM_BU(offset + i, addr);
    }
    return out;
}

static inline uint32_t RSP_MEM_H_LOAD(uint32_t offset, uint32_t addr) {
    int16_t out;
    for (int i = 0; i < 2; i++) {
        reinterpret_cast<uint8_t*>(&out)[(i + 2) ^ 3] = RSP_MEM_BU(offset + i, addr);
    }
    return out;
}

static inline void RSP_MEM_H_STORE(uint32_t offset, uint32_t addr, uint32_t val) {
    for (int i = 0; i < 2; i++) {
        RSP_MEM_BU(offset + i, addr) = reinterpret_cast<uint8_t*>(&val)[(i + 2) ^ 3];
    }
}

#define RSP_ADD32(a, b) \
    ((int32_t)((a) + (b)))

#define RSP_SUB32(a, b) \
    ((int32_t)((a) - (b)))

#define RSP_SIGNED(val) \
    ((int32_t)(val))

#define SET_DMA_MEM(mem_addr) dma_mem_address = (mem_addr)
#define SET_DMA_DRAM(dram_addr) dma_dram_address = (dram_addr)
#define DO_DMA_READ(rd_len) dma_rdram_to_dmem(rdram, dma_mem_address, dma_dram_address, (rd_len))
#define DO_DMA_WRITE(wr_len) dma_dmem_to_rdram(rdram, dma_mem_address, dma_dram_address, (wr_len))

// DPC (RDP command-queue) bridge for graphics RSP ucodes that emit RDP commands
// directly via mtc0 to DPC_START/DPC_END (Factor5 ucode does this). The host
// implements rsp_dpc_submit() to forward [start..end] RDRAM bytes to the
// rendering backend; default is a weak no-op (commands logged but not rendered).
extern uint32_t g_rsp_dpc_start;
extern uint32_t g_rsp_dpc_end;
void rsp_dpc_submit(uint8_t* rdram, uint32_t start, uint32_t end);
#define RSP_DPC_START(val)         (g_rsp_dpc_start = (val))
#define RSP_DPC_END(val)           (rsp_dpc_submit(rdram, g_rsp_dpc_start, (val)), g_rsp_dpc_end = (val))
#define RSP_DPC_STATUS_WRITE(val)  ((void)(val))   /* clear-flag writes are no-ops */
// DPC_CURRENT returns g_rsp_dpc_end if non-zero, else 0xFFFFFFFF — the latter
// keeps any `beq DPC_CURRENT, $head` busy-wait from spinning when both are 0
// (initial state). Real RDP never holds CURRENT == 0 in normal operation.
#define RSP_DPC_CURRENT_READ()     (g_rsp_dpc_end ? g_rsp_dpc_end : 0xFFFFFFFFu)
#define RSP_DPC_END_READ()         (g_rsp_dpc_end)

// Bounded DMA: clamp lengths/addresses so a graphics ucode that runs with
// uninitialized GPRs (e.g. before its bootloader has set them up) skips bogus
// DMAs instead of access-violating. Real RSP behavior would mask, so this
// matches hardware better than crashing anyway.
static inline void dma_rdram_to_dmem(uint8_t* rdram, uint32_t dmem_addr, uint32_t dram_addr, uint32_t rd_len) {
    rd_len += 1; // Read length is inclusive
    dram_addr &= 0xFFFFF8;
    // Bit 12 of the mem address selects IMEM (1) vs DMEM (0) on real RSP.
    // When a graphics ucode boot DMAs its own text into IMEM, we already ARE
    // the recompiled ucode running as C — silently skip the IMEM DMA so it
    // doesn't corrupt DMEM at the masked offset.
    if (dmem_addr & 0x1000) {
        return;
    }
    dmem_addr &= 0xFFF;
    if (dmem_addr + rd_len > 0x1000) rd_len = 0x1000 - dmem_addr;
    if (dram_addr >= 0x800000) {
        static int n = 0;
        if (++n <= 5) { fprintf(stderr, "[dma] skip rdram_to_dmem: dram=0x%08X len=%u\n", dram_addr, rd_len); fflush(stderr); }
        return;
    }
    if (dram_addr + rd_len > 0x800000) rd_len = 0x800000 - dram_addr;
    for (uint32_t i = 0; i < rd_len; i++) {
        RSP_MEM_B(i, dmem_addr) = MEM_B(0, (int64_t)(int32_t)(dram_addr + i + 0x80000000));
    }
}

static inline void dma_dmem_to_rdram(uint8_t* rdram, uint32_t dmem_addr, uint32_t dram_addr, uint32_t wr_len) {
    wr_len += 1; // Write length is inclusive
    dram_addr &= 0xFFFFF8;
    dmem_addr &= 0xFFF;
    if (dmem_addr + wr_len > 0x1000) wr_len = 0x1000 - dmem_addr;
    if (dram_addr >= 0x800000) {
        static int n = 0;
        if (++n <= 5) { fprintf(stderr, "[dma] skip dmem_to_rdram: dram=0x%08X len=%u\n", dram_addr, wr_len); fflush(stderr); }
        return;
    }
    if (dram_addr + wr_len > 0x800000) wr_len = 0x800000 - dram_addr;
    // INSTRUMENT: log DMAs targeting the contested 0x4B7800 region. Filter for
    // DMAs whose source DMEM bytes start with 0x9A or 0xC2 — that's the pattern
    // we know appears in the final state. Also log first 5 of EACH unique src.
    if (dram_addr >= 0x4B7800u && dram_addr < 0x4B7900u) {
        extern uint8_t dmem[];
        static std::atomic<uint64_t> s_dma_seq{0};
        static std::atomic<uint32_t> s_pattern_seen{0};
        uint64_t v = ++s_dma_seq;
        bool is_9a = (dmem[dmem_addr+0] == 0x9A) || (dmem[dmem_addr+0] == 0xC2);
        bool log_this = false;
        if (v <= 5) log_this = true;
        if (is_9a) {
            uint32_t k = ++s_pattern_seen;
            if (k <= 30) log_this = true;
        }
        if (log_this) {
            fprintf(stderr, "[dma-7800 #%llu%s] dmem_addr=0x%03X dram_addr=0x%06X wr_len=%u  src8=%02X%02X%02X%02X %02X%02X%02X%02X\n",
                (unsigned long long)v, is_9a ? "*" : "",
                dmem_addr, dram_addr, wr_len,
                dmem[dmem_addr+0], dmem[dmem_addr+1], dmem[dmem_addr+2], dmem[dmem_addr+3],
                dmem[dmem_addr+4], dmem[dmem_addr+5], dmem[dmem_addr+6], dmem[dmem_addr+7]);
            fflush(stderr);
        }
    }
    for (uint32_t i = 0; i < wr_len; i++) {
        MEM_B(0, (int64_t)(int32_t)(dram_addr + i + 0x80000000)) = RSP_MEM_B(i, dmem_addr);
    }
}

namespace recomp {
    namespace rsp {
        struct callbacks_t {
            using get_rsp_microcode_t = RspUcodeFunc*(const OSTask* task);

            /**
             * Return a function pointer to the corresponding RSP microcode function for the given `task_type`.
             *
             * The full OSTask (`task` parameter) is passed in case the `task_type` number is not enough information to distinguish out the exact microcode function.
             *
             * This function is allowed to return `nullptr` if no microcode matches the specified task. In this case a message will be printed to stderr and the program will exit.
             */
            get_rsp_microcode_t* get_rsp_microcode;
        };

        void set_callbacks(const callbacks_t& callbacks);

        void constants_init();

        bool run_task(uint8_t* rdram, const OSTask* task);
    }
}

#endif
