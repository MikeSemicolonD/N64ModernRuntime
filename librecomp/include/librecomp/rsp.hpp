#ifndef __RSP_H__
#define __RSP_H__

#include <cstdio>
#include <cstdlib>

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

static inline void dma_rdram_to_dmem(uint8_t* rdram, uint32_t dmem_addr, uint32_t dram_addr, uint32_t rd_len) {
    rd_len += 1; // Read length is inclusive
    dram_addr &= 0xFFFFF8;
    // Catch IMEM-target DMAs (overlays/self-modifying code) — confirms whether the synth ucode
    // rewrites its own IMEM (the suspected synth-silence root).
    { static int li=-1; if(li<0){const char*e=std::getenv("ROGUESQ_LOG_IMEMDMA");li=(e&&e[0]&&e[0]!='0')?1:0;}
      static int ni=0; if(li && (dmem_addr & 0x1000) && ni<40){ ++ni;
        fprintf(stderr,"[imemdma] IMEM dst=0x%X <- dram=0x%06X len=%u first8=%02X%02X%02X%02X%02X%02X%02X%02X\n",
          dmem_addr, dram_addr, rd_len, rdram[dram_addr^3],rdram[(dram_addr+1)^3],rdram[(dram_addr+2)^3],
          rdram[(dram_addr+3)^3],rdram[(dram_addr+4)^3],rdram[(dram_addr+5)^3],rdram[(dram_addr+6)^3],rdram[(dram_addr+7)^3]); fflush(stderr); } }
    // Bit 12 of the mem address selects IMEM (1) vs DMEM (0) on real RSP.
    // When a graphics ucode boot DMAs its own text into IMEM, we already ARE
    // the recompiled ucode running as C — silently skip so it doesn't corrupt
    // DMEM at the masked offset.
    if (dmem_addr & 0x1000) return;
    dmem_addr &= 0xFFF;
    if (dmem_addr + rd_len > 0x1000) rd_len = 0x1000 - dmem_addr;
    // Read bound raised to 16MB so the MusyX synth can DMA samples from the
    // permanent sample-bank region placed above the game's 8MB (osMemSize=8MB,
    // so the game never touches 8-16MB; the host RDRAM is 512MB committed).
    if (dram_addr >= 0x1000000) return;
    if (dram_addr + rd_len > 0x1000000) rd_len = 0x1000000 - dram_addr;
    { static int lg=-1; if(lg<0){const char*e=std::getenv("ROGUESQ_LOG_SAMPDMA");lg=(e&&e[0]&&e[0]!='0')?1:0;}
      static int n=0; if(lg && dram_addr>=0x1A0000u && dram_addr<0x600000u && n<24){ ++n;  // samp bank loaded ~0x1AE9A0 (below 8MB); catch its sample fetches
        fprintf(stderr,"[sampdma] dram=0x%06X len=%u first8=%02X%02X%02X%02X%02X%02X%02X%02X\n", dram_addr, rd_len,
          rdram[dram_addr^3],rdram[(dram_addr+1)^3],rdram[(dram_addr+2)^3],rdram[(dram_addr+3)^3],
          rdram[(dram_addr+4)^3],rdram[(dram_addr+5)^3],rdram[(dram_addr+6)^3],rdram[(dram_addr+7)^3]); fflush(stderr); } }
    // What fills the synth's per-voice region (DMEM 0x340..0x600, incl. the 0x350 voice list)?
    { static int lg2=-1; if(lg2<0){const char*e=std::getenv("ROGUESQ_LOG_DMEMFILL");lg2=(e&&e[0]&&e[0]!='0')?1:0;}
      static int n2=0; if(lg2 && dmem_addr<0x1000u && n2<60){ ++n2;
        fprintf(stderr,"[dmemfill] dmem=0x%03X <- dram=0x%06X len=%u first8=%02X%02X%02X%02X%02X%02X%02X%02X\n",
          dmem_addr, dram_addr, rd_len, rdram[dram_addr^3],rdram[(dram_addr+1)^3],rdram[(dram_addr+2)^3],
          rdram[(dram_addr+3)^3],rdram[(dram_addr+4)^3],rdram[(dram_addr+5)^3],rdram[(dram_addr+6)^3],rdram[(dram_addr+7)^3]); fflush(stderr); } }
    for (uint32_t i = 0; i < rd_len; i++) {
        RSP_MEM_B(i, dmem_addr) = MEM_B(0, (int64_t)(int32_t)(dram_addr + i + 0x80000000));
    }
}

static inline void dma_dmem_to_rdram(uint8_t* rdram, uint32_t dmem_addr, uint32_t dram_addr, uint32_t wr_len) {
    wr_len += 1; // Write length is inclusive
    dram_addr &= 0xFFFFF8;
    // Same IMEM/DMEM split as the rdram→dmem direction: silently skip IMEM
    // writebacks, clamp lengths to actual memory bounds.
    if (dmem_addr & 0x1000) return;
    dmem_addr &= 0xFFF;
    if (dmem_addr + wr_len > 0x1000) wr_len = 0x1000 - dmem_addr;
    if (dram_addr >= 0x800000) return;
    if (dram_addr + wr_len > 0x800000) wr_len = 0x800000 - dram_addr;
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
