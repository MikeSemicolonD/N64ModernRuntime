#include "librecomp/rsp.hpp"
#include "librecomp/rsp_vu_impl.hpp"
RspExitReason gfxMain(uint8_t* rdram, [[maybe_unused]] uint32_t ucode_addr) {
    uint32_t           r1 = 0,  r2 = 0,  r3 = 0,  r4 = 0,  r5 = 0,  r6 = 0,  r7 = 0;
    uint32_t  r8 = 0,  r9 = 0, r10 = 0, r11 = 0, r12 = 0, r13 = 0, r14 = 0, r15 = 0;
    uint32_t r16 = 0, r17 = 0, r18 = 0, r19 = 0, r20 = 0, r21 = 0, r22 = 0, r23 = 0;
    uint32_t r24 = 0, r25 = 0, r26 = 0, r27 = 0, r28 = 0, r29 = 0, r30 = 0, r31 = 0;
    uint32_t dma_mem_address = 0, dma_dram_address = 0, jump_target = 0;
    const char * debug_file = NULL; int debug_line = 0;
    RSP rsp{};
    r1 = 0xFC0;
L_1000:
    // .word       0x000D9704                   # sllv        $18, $13, $zero # 00000700 <InstrIdType: RSP_SPECIAL>
    r18 = S32(r13) << (0 & 31);
    // .word       0x05340000                   # INVALID     $v0, $v0, $v20[1] # 00000000 <InstrIdType: RSP_REGIMM>
L_1008:
    // .word       0x000D7904                   # sllv        $15, $13, $zero # 00000100 <InstrIdType: RSP_SPECIAL>
    r15 = S32(r13) << (0 & 31);
    // nop

    // add         $zero, $18, $zero

    // andi        $14, $28, 0x0
    r14 = r28 & 0X0;
    // ori         $14, $4, 0x400
    r14 = r4 | 0X400;
    // .word       0x42308000                   # INVALID     $v0, $v16, $v16[1] # 00000000 <InstrIdType: RSP_COP0>
L_1020:
    // .word       0x40144100                   # mfc0        $20, DPC_START # 00000100 <InstrIdType: RSP_COP0>
    r20 = 0;
    // beq         $24, $zero, L_1884
    if (r24 == 0) {
        // .word       0x03340300                   # sll         $zero, $20, 12 # 03200000 <InstrIdType: RSP_SPECIAL>
        0 = S32(r20) << 12;
        goto L_1884;
    }
L_1028:
    // .word       0x03340300                   # sll         $zero, $20, 12 # 03200000 <InstrIdType: RSP_SPECIAL>
    0 = S32(r20) << 12;
    // .word       0x43101100                   # INVALID     $v4, $v2, $v16[0] # 00000000 <InstrIdType: RSP_COP0>
    // andi        $6, $9, 0x800
    r6 = r9 & 0X800;
    // .word       0x40140400                   # mfc0        $20, SP_MEM_ADDR # 00000400 <InstrIdType: RSP_COP0>
    r20 = 0;
    // beq         $24, $zero, L_1C90
    if (r24 == 0) {
        // .word       0x4230FE01                   # INVALID     $v24, $v31, $v16[1] # 00000001 <InstrIdType: RSP_COP0>
            goto L_1C90;
    }
L_103C:
    // .word       0x4230FE01                   # INVALID     $v24, $v31, $v16[1] # 00000001 <InstrIdType: RSP_COP0>
    // .word       0x4294D600                   # INVALID     $v24, $v26, $v20[0h] # 00000000 <InstrIdType: RSP_COP0>
    // mfc0        $zero, SP_DRAM_ADDR
    0 = 0;
L_1048:
    // .word       0x02002310                   # INVALID     $v12, $v4, $v0 # 02000000 <InstrIdType: RSP_SPECIAL>
    // .word       0x4230FE01                   # INVALID     $v24, $v31, $v16[1] # 00000001 <InstrIdType: RSP_COP0>
    // .word       0x42946400                   # INVALID     $v16, $v12, $v20[0h] # 00000000 <InstrIdType: RSP_COP0>
    // mfc0        $zero, SP_DRAM_ADDR
    0 = 0;
    // .word       0x02347802                   # srl         $15, $20, 0 # 02200000 <InstrIdType: RSP_SPECIAL>
    r15 = S32(U32(r20) >> 0);
    // addi        $22, $16, -0x1301
    r22 = RSP_ADD32(r16, -0X1301);
L_1060:
    // .word       0x1F348810                   # bgtz        $25, L_10A4 # 00140000 <InstrIdType: RSP_NORMAL>
    if (RSP_SIGNED(r25) > 0) {
        // .word       0x00096C04                   # sllv        $13, $9, $zero # 00000400 <InstrIdType: RSP_SPECIAL>
        r13 = S32(r9) << (0 & 31);
        goto L_10A4;
    }
    // .word       0x00096C04                   # sllv        $13, $9, $zero # 00000400 <InstrIdType: RSP_SPECIAL>
    r13 = S32(r9) << (0 & 31);
    // .word       0x02340040                   # sll         $zero, $20, 1 # 02200000 <InstrIdType: RSP_SPECIAL>
    0 = S32(r20) << 1;
    // lb          $zero, 0x20($18)
    0 = RSP_MEM_B(0X20, r18);
    // sll         $1, $zero, 20
    r1 = S32(0) << 20;
    // beq         $24, $zero, L_18D0
    if (r24 == 0) {
        // .word       0x03347200                   # sll         $14, $20, 8 # 03200000 <InstrIdType: RSP_SPECIAL>
        r14 = S32(r20) << 8;
        goto L_18D0;
    }
    // .word       0x03347200                   # sll         $14, $20, 8 # 03200000 <InstrIdType: RSP_SPECIAL>
    r14 = S32(r20) << 8;
    // beq         $29, $14, L_1080
    if (r29 == r14) {
        // .word       0x43100300                   # INVALID     $v12, $v0, $v16[0] # 00000000 <InstrIdType: RSP_COP0>
            goto L_1080;
    }
L_1080:
    // .word       0x43100300                   # INVALID     $v12, $v0, $v16[0] # 00000000 <InstrIdType: RSP_COP0>
    // bne         $5, $14, L_0088
    if (r5 != r14) {
        // .word       0x00094A04                   # sllv        $9, $9, $zero # 00000200 <InstrIdType: RSP_SPECIAL>
        r9 = S32(r9) << (0 & 31);
        goto L_0088;
    }
    // .word       0x00094A04                   # sllv        $9, $9, $zero # 00000200 <InstrIdType: RSP_SPECIAL>
    r9 = S32(r9) << (0 & 31);
    // beq         $1, $6, L_1090
    if (r1 == r6) {
        // addi        $14, $20, 0x0
        r14 = RSP_ADD32(r20, 0X0);
        goto L_1090;
    }
L_1090:
    // addi        $14, $20, 0x0
    r14 = RSP_ADD32(r20, 0X0);
    // addi        $14, $28, 0x400
    r14 = RSP_ADD32(r28, 0X400);
    // andi        $6, $9, 0x800
    r6 = r9 & 0X800;
    // .word       0x02AE0800                   # sll         $1, $14, 0 # 02A00000 <InstrIdType: RSP_SPECIAL>
    r1 = S32(r14) << 0;
    // .word       0x03AE0C00                   # sll         $1, $14, 16 # 03A00000 <InstrIdType: RSP_SPECIAL>
    r1 = S32(r14) << 16;
L_10A4:
    // beq         $1, $6, L_10A8
    if (r1 == r6) {
        // .word       0x1F34D810                   # bgtz        $25, L_10EC # 00140000 <InstrIdType: RSP_NORMAL>
        if (RSP_SIGNED(r25) > 0) {
        // mtc2        $14, $v1[0]
        rsp.MTC2<0>(r14, rsp.vpu.r[1]);
        goto L_10EC;
    }
        goto L_10A8;
    }
L_10A8:
    // .word       0x1F34D810                   # bgtz        $25, L_10EC # 00140000 <InstrIdType: RSP_NORMAL>
    if (RSP_SIGNED(r25) > 0) {
        // mtc2        $14, $v1[0]
        rsp.MTC2<0>(r14, rsp.vpu.r[1]);
        goto L_10EC;
    }
    // mtc2        $14, $v1[0]
    rsp.MTC2<0>(r14, rsp.vpu.r[1]);
    // j           L_03C8
    // .word       0x40111A00                   # mfc0        $17, SP_WR_LEN # 00000200 <InstrIdType: RSP_COP0>
    r17 = 0;
    goto L_03C8;
    // .word       0x40111A00                   # mfc0        $17, SP_WR_LEN # 00000200 <InstrIdType: RSP_COP0>
    r17 = 0;
    // bgtz        $24, L_087C
    if (RSP_SIGNED(r24) > 0) {
        // .word       0x508E0400                   # INVALID     $v16, $v0, $v14[0h] # 00000000 <InstrIdType: RSP_NORMAL>
            goto L_087C;
    }
    // .word       0x508E0400                   # INVALID     $v16, $v0, $v14[0h] # 00000000 <InstrIdType: RSP_NORMAL>
    // j           L_0160
    // j           L_0960
    // .word       0x61050800                   # INVALID     $v0, $v1, $v5[0] # 00000000 <InstrIdType: RSP_NORMAL>
    goto L_0960;
    goto L_0160;
    // j           L_0960
    // .word       0x61050800                   # INVALID     $v0, $v1, $v5[0] # 00000000 <InstrIdType: RSP_NORMAL>
    goto L_0960;
L_10C8:
    // .word       0x61050800                   # INVALID     $v0, $v1, $v5[0] # 00000000 <InstrIdType: RSP_NORMAL>
    // j           L_0160
    // .word       0x6B310004                   # INVALID     $v0, $v0, $v17[1] # 02000004 <InstrIdType: RSP_NORMAL>
    goto L_0160;
    // .word       0x6B310004                   # INVALID     $v0, $v0, $v17[1] # 02000004 <InstrIdType: RSP_NORMAL>
    // .word       0x6015FDFF                   # INVALID     $v23, $v31, $v21 # 0000003F <InstrIdType: RSP_NORMAL>
    // mtc2        $14, $v0[0]
    rsp.MTC2<0>(r14, rsp.vpu.r[0]);
    // beq         $2, $zero, L_1220
    if (r2 == 0) {
        // j           L_1BFC
        // nop

    goto L_1BFC;
        goto L_1220;
    }
    // j           L_1BFC
    // nop

    goto L_1BFC;
    // nop

    // .word       0x88400040                   # INVALID     $v1, $v0, $v0[0q] # 00000000 <InstrIdType: RSP_NORMAL>
L_10EC:
    // beq         $2, $zero, L_1230
    if (r2 == 0) {
        // beq         $zero, $1, L_1A54
        if (0 == r1) {
        // .word       0x61050300                   # INVALID     $v12, $v0, $v5[0] # 00000000 <InstrIdType: RSP_NORMAL>
            goto L_1A54;
    }
        goto L_1230;
    }
    // beq         $zero, $1, L_1A54
    if (0 == r1) {
        // .word       0x61050300                   # INVALID     $v12, $v0, $v5[0] # 00000000 <InstrIdType: RSP_NORMAL>
            goto L_1A54;
    }
    // .word       0x61050300                   # INVALID     $v12, $v0, $v5[0] # 00000000 <InstrIdType: RSP_NORMAL>
    // j           L_0160
    // .word       0x70012258                   # INVALID     $v9, $v4, $v1 # 00000018 <InstrIdType: RSP_NORMAL>
    goto L_0160;
    // .word       0x70012258                   # INVALID     $v9, $v4, $v1 # 00000018 <InstrIdType: RSP_NORMAL>
    // .word       0x6105FAFF                   # INVALID     $v11, $v31, $v5[0] # 0000003F <InstrIdType: RSP_NORMAL>
    // j           L_0600
    // j           L_1460
    // .word       0x4425FFFF                   # INVALID     $v31, $v31, $v5[1] # 0000003F <InstrIdType: RSP_COP1>
    goto L_1460;
    goto L_0600;
L_1108:
    // j           L_1460
    // .word       0x4425FFFF                   # INVALID     $v31, $v31, $v5[1] # 0000003F <InstrIdType: RSP_COP1>
    goto L_1460;
    // .word       0x4425FFFF                   # INVALID     $v31, $v31, $v5[1] # 0000003F <InstrIdType: RSP_COP1>
    // .word       0x000D7F04                   # sllv        $15, $13, $zero # 00000700 <InstrIdType: RSP_SPECIAL>
    r15 = S32(r13) << (0 & 31);
L_1114:
    // break       564, 384
    return RspExitReason::Broke;
    // .word       0x50AE0800                   # INVALID     $v0, $v1, $v14[1h] # 00000000 <InstrIdType: RSP_NORMAL>
    // lbu         $zero, 0x48($2)
    0 = RSP_MEM_BU(0X48, r2);
L_1120:
    // .word       0xC0030800                   # INVALID     $v0, $v1, $v3 # 00000000 <InstrIdType: RSP_NORMAL>
    // beq         $1, $20, L_115C
    if (r1 == r20) {
        // .word       0x00099A04                   # sllv        $19, $9, $zero # 00000200 <InstrIdType: RSP_SPECIAL>
        r19 = S32(r9) << (0 & 31);
        goto L_115C;
    }
    // .word       0x00099A04                   # sllv        $19, $9, $zero # 00000200 <InstrIdType: RSP_SPECIAL>
    r19 = S32(r9) << (0 & 31);
    // .word       0x05340800                   # INVALID     $v0, $v1, $v20[1] # 00000000 <InstrIdType: RSP_REGIMM>
    // .word       0x428E3000                   # INVALID     $v0, $v6, $v14[0h] # 00000000 <InstrIdType: RSP_COP0>
    // bltz        $2, L_1218
    if (RSP_SIGNED(r2) < 0) {
        // lb          $20, -0x101($zero)
        r20 = RSP_MEM_B(-0X101, 0);
        goto L_1218;
    }
L_1138:
    // lb          $20, -0x101($zero)
    r20 = RSP_MEM_B(-0X101, 0);
    // .word       0x03340F01                   # INVALID     $v28, $v1, $v20[1] # 02000000 <InstrIdType: RSP_SPECIAL>
    // bltz        $2, L_11E4
    if (RSP_SIGNED(r2) < 0) {
        // lb          $20, -0x101($zero)
        r20 = RSP_MEM_B(-0X101, 0);
        goto L_11E4;
    }
    // lb          $20, -0x101($zero)
    r20 = RSP_MEM_B(-0X101, 0);
    // beq         $9, $20, L_1150
    if (r9 == r20) {
        // lbu         $zero, 0x0($10)
        0 = RSP_MEM_BU(0X0, r10);
        goto L_1150;
    }
    // lbu         $zero, 0x0($10)
    0 = RSP_MEM_BU(0X0, r10);
L_1150:
    // lb          $zero, 0x8($18)
    0 = RSP_MEM_B(0X8, r18);
    // lb          $zero, 0x10($26)
    0 = RSP_MEM_B(0X10, r26);
    // .word       0x42AE5C00                   # INVALID     $v16, $v11, $v14[1h] # 00000000 <InstrIdType: RSP_COP0>
L_115C:
    // .word       0xE0030800                   # INVALID     $v0, $v1, $v3 # 00000000 <InstrIdType: RSP_NORMAL>
    // andi        $6, $9, 0x800
    r6 = r9 & 0X800;
    // bltz        $2, L_1228
    if (RSP_SIGNED(r2) < 0) {
        // lb          $20, -0x101($zero)
        r20 = RSP_MEM_B(-0X101, 0);
        goto L_1228;
    }
    // lb          $20, -0x101($zero)
    r20 = RSP_MEM_B(-0X101, 0);
L_116C:
    // .word       0x028C7001                   # INVALID     $v0, $v14, $v12[0h] # 02000000 <InstrIdType: RSP_SPECIAL>
    // lb          $zero, 0x38($2)
    0 = RSP_MEM_B(0X38, r2);
    // .word       0xE0030800                   # INVALID     $v0, $v1, $v3 # 00000000 <InstrIdType: RSP_NORMAL>
    // .word       0x42AE3000                   # INVALID     $v0, $v6, $v14[1h] # 00000000 <InstrIdType: RSP_COP0>
    // bltz        $10, L_1260
    if (RSP_SIGNED(r10) < 0) {
        // sb          $20, -0x101($zero)
        RSP_MEM_B(-0X101, 0) = r20;
        goto L_1260;
    }
    // sb          $20, -0x101($zero)
    RSP_MEM_B(-0X101, 0) = r20;
    // bltz        $10, L_1228
    if (RSP_SIGNED(r10) < 0) {
        // sb          $20, -0x101($zero)
        RSP_MEM_B(-0X101, 0) = r20;
        goto L_1228;
    }
    // sb          $20, -0x101($zero)
    RSP_MEM_B(-0X101, 0) = r20;
    // nop

    // lb          $zero, 0x0($18)
    0 = RSP_MEM_B(0X0, r18);
    // lb          $zero, 0x8($26)
    0 = RSP_MEM_B(0X8, r26);
    // lh          $zero, 0x18($2)
    0 = RSP_MEM_H_LOAD(0X18, r2);
    // lb          $20, -0x1($zero)
    r20 = RSP_MEM_B(-0X1, 0);
    // bltz        $2, L_1264
    if (RSP_SIGNED(r2) < 0) {
        // .word       0xE0030800                   # INVALID     $v0, $v1, $v3 # 00000000 <InstrIdType: RSP_NORMAL>
            goto L_1264;
    }
    // .word       0xE0030800                   # INVALID     $v0, $v1, $v3 # 00000000 <InstrIdType: RSP_NORMAL>
    // lb          $zero, 0x38($2)
    0 = RSP_MEM_B(0X38, r2);
    // bltz        $10, L_1290
    if (RSP_SIGNED(r10) < 0) {
        // sb          $20, -0x101($zero)
        RSP_MEM_B(-0X101, 0) = r20;
        goto L_1290;
    }
L_11B0:
    // sb          $20, -0x101($zero)
    RSP_MEM_B(-0X101, 0) = r20;
    // bltz        $10, L_1258
    if (RSP_SIGNED(r10) < 0) {
        // sb          $20, -0x101($zero)
        RSP_MEM_B(-0X101, 0) = r20;
        goto L_1258;
    }
    // sb          $20, -0x101($zero)
    RSP_MEM_B(-0X101, 0) = r20;
    // nop

    // lb          $zero, 0x0($26)
    0 = RSP_MEM_B(0X0, r26);
    // lb          $zero, 0x8($18)
    0 = RSP_MEM_B(0X8, r18);
    // lh          $zero, 0x10($2)
    0 = RSP_MEM_H_LOAD(0X10, r2);
    // lb          $20, -0x1($zero)
    r20 = RSP_MEM_B(-0X1, 0);
    // bltz        $2, L_1294
    if (RSP_SIGNED(r2) < 0) {
        // .word       0xE0030800                   # INVALID     $v0, $v1, $v3 # 00000000 <InstrIdType: RSP_NORMAL>
            goto L_1294;
    }
    // .word       0xE0030800                   # INVALID     $v0, $v1, $v3 # 00000000 <InstrIdType: RSP_NORMAL>
    // lb          $zero, 0x38($2)
    0 = RSP_MEM_B(0X38, r2);
    // .word       0x04947A00                   # INVALID     $v8, $v15, $v20[0h] # 00000000 <InstrIdType: RSP_REGIMM>
    // .word       0x00099E04                   # sllv        $19, $9, $zero # 00000600 <InstrIdType: RSP_SPECIAL>
    r19 = S32(r9) << (0 & 31);
L_11E4:
    // .word       0x03340000                   # sll         $zero, $20, 0 # 03200000 <InstrIdType: RSP_SPECIAL>
    0 = S32(r20) << 0;
    // sh          $20, 0x7A00($4)
    RSP_MEM_H_STORE(0X7A00, r4, r20);
    // .word       0x02905301                   # INVALID     $v12, $v10, $v16[0h] # 02000000 <InstrIdType: RSP_SPECIAL>
    // bltz        $zero, L_1A60
    if (RSP_SIGNED(0) < 0) {
        // sb          $16, 0x700($16)
        RSP_MEM_B(0X700, r16) = r16;
        goto L_1A60;
    }
    // sb          $16, 0x700($16)
    RSP_MEM_B(0X700, r16) = r16;
    // .word       0x1FA46A01                   # bgtz        $29, L_1A00 # 00040000 <InstrIdType: RSP_NORMAL>
    if (RSP_SIGNED(r29) > 0) {
        // bltz        $13, L_1E04
        if (RSP_SIGNED(r13) < 0) {
        // sb          $20, 0x7C00($20)
        RSP_MEM_B(0X7C00, r20) = r20;
        goto L_1E04;
    }
        goto L_1A00;
    }
    // bltz        $13, L_1E04
    if (RSP_SIGNED(r13) < 0) {
        // sb          $20, 0x7C00($20)
        RSP_MEM_B(0X7C00, r20) = r20;
        goto L_1E04;
    }
L_1200:
    // sb          $20, 0x7C00($20)
    RSP_MEM_B(0X7C00, r20) = r20;
    // addiu       $12, $12, 0x1000
    r12 = RSP_ADD32(r12, 0X1000);
    // lh          $16, -0xF1($1)
    r16 = RSP_MEM_H_LOAD(-0XF1, r1);
L_120C:
    // .word       0x45002110                   # INVALID     $v4, $v4, $v0[0] # 00000010 <InstrIdType: RSP_COP1>
    // .word       0x000D8B04                   # sllv        $17, $13, $zero # 00000300 <InstrIdType: RSP_SPECIAL>
    r17 = S32(r13) << (0 & 31);
    // .word       0x63947600                   # INVALID     $v24, $v14, $v20[4] # 02000000 <InstrIdType: RSP_NORMAL>
L_1218:
    // .word       0x60000800                   # INVALID     $v0, $v1, $v0 # 00000000 <InstrIdType: RSP_NORMAL>
    // .word       0x1F946A01                   # bgtz        $28, L_1A24 # 00140000 <InstrIdType: RSP_NORMAL>
    if (RSP_SIGNED(r28) > 0) {
        // .word       0x438E5C00                   # INVALID     $v16, $v11, $v14[4] # 00000000 <InstrIdType: RSP_COP0>
            goto L_1A24;
    }
L_1220:
    // .word       0x438E5C00                   # INVALID     $v16, $v11, $v14[4] # 00000000 <InstrIdType: RSP_COP0>
    // .word       0x42925200                   # INVALID     $v8, $v10, $v18[0h] # 00000000 <InstrIdType: RSP_COP0>
L_1228:
    // .word       0x43AC5006                   # INVALID     $v0, $v10, $v12[5] # 00000006 <InstrIdType: RSP_COP0>
L_122C:
    // .word       0x51AC5406                   # INVALID     $v16, $v10, $v12[5] # 00000006 <InstrIdType: RSP_NORMAL>
L_1230:
    // .word       0x42240800                   # INVALID     $v0, $v1, $v4[1] # 00000000 <InstrIdType: RSP_COP0>
L_1234:
    // .word       0x42A25200                   # INVALID     $v8, $v10, $v2[1h] # 00000000 <InstrIdType: RSP_COP0>
    // bne         $zero, $zero, L_067C
    if (0 != 0) {
        // .word       0x00096D04                   # sllv        $13, $9, $zero # 00000500 <InstrIdType: RSP_SPECIAL>
        r13 = S32(r9) << (0 & 31);
        goto L_067C;
    }
    // .word       0x00096D04                   # sllv        $13, $9, $zero # 00000500 <InstrIdType: RSP_SPECIAL>
    r13 = S32(r9) << (0 & 31);
    // .word       0x1F348810                   # bgtz        $25, L_1284 # 00140000 <InstrIdType: RSP_NORMAL>
    if (RSP_SIGNED(r25) > 0) {
        // mtc2        $18, $v10[4]
        rsp.MTC2<4>(r18, rsp.vpu.r[10]);
        goto L_1284;
    }
    // mtc2        $18, $v10[4]
    rsp.MTC2<4>(r18, rsp.vpu.r[10]);
L_1248:
    // j           L_03FC
    // .word       0x000586FF                   # INVALID     $v27, $v16, $v5 # 00000000 <InstrIdType: RSP_SPECIAL>
    goto L_03FC;
    // .word       0x000586FF                   # INVALID     $v27, $v16, $v5 # 00000000 <InstrIdType: RSP_SPECIAL>
    // .word       0x48A25200                   # INVALID     $v8, $v10, $v2[1h] # 00000000 <InstrIdType: RSP_COP2_VU>
    // .word       0x000D6D04                   # sllv        $13, $13, $zero # 00000500 <InstrIdType: RSP_SPECIAL>
    r13 = S32(r13) << (0 & 31);
L_1258:
    // srlv        $10, $13, $20
    r10 = S32(U32(r13) >> (r20 & 31));
    // .word       0x00092204                   # sllv        $4, $9, $zero # 00000200 <InstrIdType: RSP_SPECIAL>
    r4 = S32(r9) << (0 & 31);
L_1260:
    // beq         $12, $13, L_027C
    if (r12 == r13) {
        // .word       0x0009BC04                   # sllv        $23, $9, $zero # 00000400 <InstrIdType: RSP_SPECIAL>
        r23 = S32(r9) << (0 & 31);
        goto L_027C;
    }
L_1264:
    // .word       0x0009BC04                   # sllv        $23, $9, $zero # 00000400 <InstrIdType: RSP_SPECIAL>
    r23 = S32(r9) << (0 & 31);
    // .word       0x48260C00                   # INVALID     $v16, $v1, $v6[1] # 00000000 <InstrIdType: RSP_COP2_VU>
    // .word       0x48261000                   # INVALID     $v0, $v2, $v6[1] # 00000000 <InstrIdType: RSP_COP2_VU>
    // slti        $14, $20, 0x0
    r14 = RSP_SIGNED(r20) < 0X0 ? 1 : 0;
    // .word       0x0009C904                   # sllv        $25, $9, $zero # 00000100 <InstrIdType: RSP_SPECIAL>
    r25 = S32(r9) << (0 & 31);
    // andi        $6, $9, 0x800
    r6 = r9 & 0X800;
    // .word       0x0009C204                   # sllv        $24, $9, $zero # 00000200 <InstrIdType: RSP_SPECIAL>
    r24 = S32(r9) << (0 & 31);
    // .word       0x48260C00                   # INVALID     $v16, $v1, $v6[1] # 00000000 <InstrIdType: RSP_COP2_VU>
L_1284:
    // .word       0x48261000                   # INVALID     $v0, $v2, $v6[1] # 00000000 <InstrIdType: RSP_COP2_VU>
L_1288:
    // sltiu       $18, $4, -0x401
    r18 = r4 < -0X401 ? 1 : 0;
    // slti        $18, $28, -0x501
    r18 = RSP_SIGNED(r28) < -0X501 ? 1 : 0;
L_1290:
    // j           L_0400
    // .word       0x8A010450                   # INVALID     $v17, $v0, $v1 # 02000010 <InstrIdType: RSP_NORMAL>
    goto L_0400;
L_1294:
    // .word       0x8A010450                   # INVALID     $v17, $v0, $v1 # 02000010 <InstrIdType: RSP_NORMAL>
    // .word       0x4A21FFFF                   # INVALID     $v31, $v31, $v1[1] # 00000000 <InstrIdType: RSP_COP2_VU>
    // .word       0x6A010450                   # INVALID     $v17, $v0, $v1 # 02000010 <InstrIdType: RSP_NORMAL>
    // .word       0x40012750                   # mfc0        $1, SP_STATUS # 00000750 <InstrIdType: RSP_COP0>
    r1 = 0;
    // j           L_0000
    // .word       0x49012450                   # INVALID     $v17, $v4, $v1[0] # 00000010 <InstrIdType: RSP_COP2_VU>
    goto L_0000;
L_12A8:
    // .word       0x49012450                   # INVALID     $v17, $v4, $v1[0] # 00000010 <InstrIdType: RSP_COP2_VU>
    // .word       0x54012548                   # INVALID     $v21, $v4, $v1 # 00000008 <InstrIdType: RSP_NORMAL>
    // j           L_0000
    // .word       0x538E0C00                   # INVALID     $v16, $v1, $v14[4] # 02000000 <InstrIdType: RSP_NORMAL>
    goto L_0000;
    // .word       0x538E0C00                   # INVALID     $v16, $v1, $v14[4] # 02000000 <InstrIdType: RSP_NORMAL>
    // .word       0x548E1000                   # INVALID     $v0, $v2, $v14[0h] # 00000000 <InstrIdType: RSP_NORMAL>
    // beq         $29, $14, L_12C0
    if (r29 == r14) {
        // bne         $5, $14, L_02C4
        if (r5 != r14) {
        // beq         $1, $6, L_12C8
        if (r1 == r6) {
        // .word       0x00094B04                   # sllv        $9, $9, $zero # 00000300 <InstrIdType: RSP_SPECIAL>
        r9 = S32(r9) << (0 & 31);
        goto L_12C8;
    }
        goto L_02C4;
    }
        goto L_12C0;
    }
L_12C0:
    // bne         $5, $14, L_02C4
    if (r5 != r14) {
        // beq         $1, $6, L_12C8
        if (r1 == r6) {
        // .word       0x00094B04                   # sllv        $9, $9, $zero # 00000300 <InstrIdType: RSP_SPECIAL>
        r9 = S32(r9) << (0 & 31);
        goto L_12C8;
    }
        goto L_02C4;
    }
    // beq         $1, $6, L_12C8
    if (r1 == r6) {
        // .word       0x00094B04                   # sllv        $9, $9, $zero # 00000300 <InstrIdType: RSP_SPECIAL>
        r9 = S32(r9) << (0 & 31);
        goto L_12C8;
    }
L_12C8:
    // .word       0x00094B04                   # sllv        $9, $9, $zero # 00000300 <InstrIdType: RSP_SPECIAL>
    r9 = S32(r9) << (0 & 31);
    // .word       0x1F34D810                   # bgtz        $25, L_1310 # 00140000 <InstrIdType: RSP_NORMAL>
    if (RSP_SIGNED(r25) > 0) {
        // .word       0x428E3400                   # INVALID     $v16, $v6, $v14[0h] # 00000000 <InstrIdType: RSP_COP0>
            goto L_1310;
    }
    // .word       0x428E3400                   # INVALID     $v16, $v6, $v14[0h] # 00000000 <InstrIdType: RSP_COP0>
    // .word       0x0009DA04                   # sllv        $27, $9, $zero # 00000200 <InstrIdType: RSP_SPECIAL>
    r27 = S32(r9) << (0 & 31);
    // .word       0x54002510                   # INVALID     $v20, $v4, $v0 # 00000010 <InstrIdType: RSP_NORMAL>
    // .word       0x428E3400                   # INVALID     $v16, $v6, $v14[0h] # 00000000 <InstrIdType: RSP_COP0>
    // lb          $2, 0x2718($zero)
    r2 = RSP_MEM_B(0X2718, 0);
    // .word       0x43002410                   # INVALID     $v16, $v4, $v0[0] # 00000010 <InstrIdType: RSP_COP0>
    // .word       0x00093604                   # sllv        $6, $9, $zero # 00000600 <InstrIdType: RSP_SPECIAL>
    r6 = S32(r9) << (0 & 31);
    // .word       0x42AE3400                   # INVALID     $v16, $v6, $v14[1h] # 00000000 <InstrIdType: RSP_COP0>
    // .word       0x42863600                   # INVALID     $v24, $v6, $v6[0h] # 00000000 <InstrIdType: RSP_COP0>
    // .word       0x53AE3800                   # INVALID     $v0, $v7, $v14[5] # 02000000 <InstrIdType: RSP_NORMAL>
L_12F8:
    // .word       0x4230FDFF                   # INVALID     $v23, $v31, $v16[1] # 0000003F <InstrIdType: RSP_COP0>
    // .word       0x63320100                   # INVALID     $v4, $v0, $v18[1] # 02000000 <InstrIdType: RSP_NORMAL>
    // .word       0x03004018                   # INVALID     $v0, $v8, $v0[0] # 02000000 <InstrIdType: RSP_SPECIAL>
    // .word       0x43002510                   # INVALID     $v20, $v4, $v0[0] # 00000010 <InstrIdType: RSP_COP0>
    // .word       0x00093604                   # sllv        $6, $9, $zero # 00000600 <InstrIdType: RSP_SPECIAL>
    r6 = S32(r9) << (0 & 31);
    // .word       0x42A63600                   # INVALID     $v24, $v6, $v6[1h] # 00000000 <InstrIdType: RSP_COP0>
L_1310:
    // .word       0x6232FF00                   # INVALID     $v28, $v31, $v18[1] # 02000000 <InstrIdType: RSP_NORMAL>
L_1314:
    // addi        $22, $28, -0x601
    r22 = RSP_ADD32(r28, -0X601);
    // .word       0x4294C600                   # INVALID     $v24, $v24, $v20[0h] # 00000000 <InstrIdType: RSP_COP0>
    // .word       0x43002110                   # INVALID     $v4, $v4, $v0[0] # 00000010 <InstrIdType: RSP_COP0>
L_1320:
    // .word       0x00093604                   # sllv        $6, $9, $zero # 00000600 <InstrIdType: RSP_SPECIAL>
    r6 = S32(r9) << (0 & 31);
L_1324:
    // .word       0x54AC0000                   # INVALID     $v0, $v0, $v12[1h] # 00000000 <InstrIdType: RSP_NORMAL>
    // .word       0x42963400                   # INVALID     $v16, $v6, $v22[0h] # 00000000 <InstrIdType: RSP_COP0>
    // bne         $zero, $zero, L_1C9C
    if (0 != 0) {
        // .word       0x6B31F807                   # INVALID     $v0, $v31, $v17[1] # 02000007 <InstrIdType: RSP_NORMAL>
            goto L_1C9C;
    }
    // .word       0x6B31F807                   # INVALID     $v0, $v31, $v17[1] # 02000007 <InstrIdType: RSP_NORMAL>
    // .word       0x6B257006                   # INVALID     $v0, $v14, $v5[1] # 02000006 <InstrIdType: RSP_NORMAL>
    // bne         $zero, $zero, L_1CC0
    if (0 != 0) {
        // lw          $17, -0x7F9($1)
        r17 = RSP_MEM_W_LOAD(-0X7F9, r1);
        goto L_1CC0;
    }
    // lw          $17, -0x7F9($1)
    r17 = RSP_MEM_W_LOAD(-0X7F9, r1);
    // lw          $5, 0x7006($1)
    r5 = RSP_MEM_W_LOAD(0X7006, r1);
    // bne         $zero, $zero, L_14E8
    if (0 != 0) {
        // sw          $17, -0x7F9($9)
        RSP_MEM_W_STORE(-0X7F9, r9, r17);
        goto L_14E8;
    }
L_1348:
    // sw          $17, -0x7F9($9)
    RSP_MEM_W_STORE(-0X7F9, r9, r17);
    // sw          $5, 0x7006($9)
    RSP_MEM_W_STORE(0X7006, r9, r5);
L_1350:
    // ori         $18, $20, 0x100
    r18 = r20 | 0X100;
    // ori         $18, $28, 0x200
    r18 = r28 | 0X200;
    // xori        $18, $4, 0x300
    r18 = r4 ^ 0X300;
    // xori        $18, $12, 0x0
    r18 = r12 ^ 0X0;
    // .word       0x42300100                   # INVALID     $v4, $v0, $v16[1] # 00000000 <InstrIdType: RSP_COP0>
L_1364:
    // .word       0x40100A00                   # mfc0        $16, SP_DRAM_ADDR # 00000200 <InstrIdType: RSP_COP0>
    r16 = 0;
    // .word       0x62320400                   # INVALID     $v16, $v0, $v18[1] # 02000000 <InstrIdType: RSP_NORMAL>
    // .word       0x63911300                   # INVALID     $v12, $v2, $v17[4] # 02000000 <InstrIdType: RSP_NORMAL>
    // lh          $17, 0x1300($4)
    r17 = RSP_MEM_H_LOAD(0X1300, r4);
L_1374:
    // sh          $17, 0x1300($12)
    RSP_MEM_H_STORE(0X1300, r12, r17);
    // slv         $v17[6], 0x0($4)
    rsp.SLV<6>(rsp.vpu.r[17], r4, 0X0);
    // .word       0xC3A2730B                   # INVALID     $v12, $v14, $v2[5] # 0200000B <InstrIdType: RSP_NORMAL>
    // .word       0xE4A2730B                   # INVALID     $v12, $v14, $v2[1h] # 0000000B <InstrIdType: RSP_NORMAL>
    // .word       0x40100200                   # mfc0        $16, SP_MEM_ADDR # 00000200 <InstrIdType: RSP_COP0>
    r16 = 0;
    // .word       0x05A3730B                   # INVALID     $v12, $v14, $v3[5] # 0000000B <InstrIdType: RSP_REGIMM>
    // slti        $3, $5, 0x730B
    r3 = RSP_SIGNED(r5) < 0X730B ? 1 : 0;
    // .word       0xC38E700B                   # INVALID     $v0, $v14, $v14[4] # 0200000B <InstrIdType: RSP_NORMAL>
    // .word       0xE48E700B                   # INVALID     $v0, $v14, $v14[0h] # 0000000B <InstrIdType: RSP_NORMAL>
    // .word       0x058F700B                   # INVALID     $v0, $v14, $v15[4] # 0000000B <InstrIdType: RSP_REGIMM>
    // slti        $15, $4, 0x700B
    r15 = RSP_SIGNED(r4) < 0X700B ? 1 : 0;
    // .word       0x62320200                   # INVALID     $v8, $v0, $v18[1] # 02000000 <InstrIdType: RSP_NORMAL>
    // .word       0x63AD1000                   # INVALID     $v0, $v2, $v13[5] # 02000000 <InstrIdType: RSP_NORMAL>
    // lh          $13, 0x1000($5)
    r13 = RSP_MEM_H_LOAD(0X1000, r5);
    // .word       0x40100D00                   # mfc0        $16, SP_DRAM_ADDR # 00000500 <InstrIdType: RSP_COP0>
    r16 = 0;
L_13B0:
    // andi        $6, $9, 0x800
    r6 = r9 & 0X800;
    // addi        $10, $22, 0x18
    r10 = RSP_ADD32(r22, 0X18);
    // addi        $10, $22, 0x11C
    r10 = RSP_ADD32(r22, 0X11C);
    // .word       0x40CA0918                   # INVALID     $v4, $v1, $v10[2h] # 00000018 <InstrIdType: RSP_COP0>
    // .word       0x40CA091C                   # INVALID     $v4, $v1, $v10[2h] # 0000001C <InstrIdType: RSP_COP0>
    // .word       0x41CA0818                   # INVALID     $v0, $v1, $v10[6] # 00000018 <InstrIdType: RSP_COP0>
    // .word       0x41CA081C                   # INVALID     $v0, $v1, $v10[6] # 0000001C <InstrIdType: RSP_COP0>
    // .word       0x024AC600                   # sll         $24, $10, 24 # 02400000 <InstrIdType: RSP_SPECIAL>
    r24 = S32(r10) << 24;
    // .word       0x024A8F08                   # jr          $18 # 000A8F00 <InstrIdType: RSP_SPECIAL>
    jump_target = r18;
    debug_file = __FILE__; debug_line = __LINE__;
    // andi        $6, $9, 0x1000
    r6 = r9 & 0X1000;
    goto do_indirect_jump;
    // andi        $6, $9, 0x1000
    r6 = r9 & 0X1000;
    // .word       0x62E90510                   # INVALID     $v20, $v0, $v9[3h] # 02000010 <InstrIdType: RSP_NORMAL>
    // lb          $9, 0x512($23)
    r9 = RSP_MEM_B(0X512, r23);
    // sb          $9, 0x514($23)
    RSP_MEM_B(0X514, r23) = r9;
    // .word       0xE0030800                   # INVALID     $v0, $v1, $v3 # 00000000 <InstrIdType: RSP_NORMAL>
    // sh          $13, 0x1000($13)
    RSP_MEM_H_STORE(0X1000, r13, r13);
    // .word       0x000DEA04                   # sllv        $29, $13, $zero # 00000200 <InstrIdType: RSP_SPECIAL>
    r29 = S32(r13) << (0 & 31);
    // jal         0x0000
    r31 = 0x13F8;
    // .word       0x000DFC05                   # INVALID     $v16, $v31, $v13 # 00000000 <InstrIdType: RSP_SPECIAL>
    goto L_0000;
    // .word       0x000DFC05                   # INVALID     $v16, $v31, $v13 # 00000000 <InstrIdType: RSP_SPECIAL>
L_13F8:
    // j           L_15E0
    // .word       0x00094B04                   # sllv        $9, $9, $zero # 00000300 <InstrIdType: RSP_SPECIAL>
    r9 = S32(r9) << (0 & 31);
    goto L_15E0;
    // .word       0x00094B04                   # sllv        $9, $9, $zero # 00000300 <InstrIdType: RSP_SPECIAL>
    r9 = S32(r9) << (0 & 31);
    // .word       0x1F34D810                   # bgtz        $25, L_1444 # 00140000 <InstrIdType: RSP_NORMAL>
    if (RSP_SIGNED(r25) > 0) {
        // bne         $zero, $zero, L_1DFC
        if (0 != 0) {
        // .word       0xEF31F807                   # INVALID     $v0, $v31, $v17[1] # 02000007 <InstrIdType: RSP_NORMAL>
            goto L_1DFC;
    }
        goto L_1444;
    }
    // bne         $zero, $zero, L_1DFC
    if (0 != 0) {
        // .word       0xEF31F807                   # INVALID     $v0, $v31, $v17[1] # 02000007 <InstrIdType: RSP_NORMAL>
            goto L_1DFC;
    }
    // .word       0xEF31F807                   # INVALID     $v0, $v31, $v17[1] # 02000007 <InstrIdType: RSP_NORMAL>
    // .word       0x000DEA04                   # sllv        $29, $13, $zero # 00000200 <InstrIdType: RSP_SPECIAL>
    r29 = S32(r13) << (0 & 31);
    // .word       0xEF257006                   # INVALID     $v0, $v14, $v5[1] # 02000006 <InstrIdType: RSP_NORMAL>
    // slv         $v13[0], 0x0($5)
    rsp.SLV<0>(rsp.vpu.r[13], r5, 0X0);
    // .word       0xE2E90516                   # INVALID     $v20, $v0, $v9[3h] # 02000016 <InstrIdType: RSP_NORMAL>
    // vmulf       $v16, $v10, $v6[5]
    rsp.VMULF<13>(rsp.vpu.r[16], rsp.vpu.r[10], rsp.vpu.r[6]);
L_1420:
    // .word       0x4DA65600                   # INVALID     $v24, $v10, $v6[5] # 00000000 <InstrIdType: RSP_NORMAL>
    // .word       0x4FA65800                   # INVALID     $v0, $v11, $v6[5] # 02000000 <InstrIdType: RSP_NORMAL>
    // .word       0x000DFC05                   # INVALID     $v16, $v31, $v13 # 00000000 <InstrIdType: RSP_SPECIAL>
    // j           L_15E0
    // .word       0x000D4B04                   # sllv        $9, $13, $zero # 00000300 <InstrIdType: RSP_SPECIAL>
    r9 = S32(r13) << (0 & 31);
    goto L_15E0;
    // .word       0x000D4B04                   # sllv        $9, $13, $zero # 00000300 <InstrIdType: RSP_SPECIAL>
    r9 = S32(r13) << (0 & 31);
    // nop

    // vmulf       $v16, $v10, $v22[4]
    rsp.VMULF<12>(rsp.vpu.r[16], rsp.vpu.r[10], rsp.vpu.r[22]);
L_143C:
    // .word       0x4C965600                   # INVALID     $v24, $v10, $v22[0h] # 00000000 <InstrIdType: RSP_NORMAL>
    // .word       0x4D965800                   # INVALID     $v0, $v11, $v22[4] # 00000000 <InstrIdType: RSP_NORMAL>
L_1444:
    // .word       0x000DFC05                   # INVALID     $v16, $v31, $v13 # 00000000 <InstrIdType: RSP_SPECIAL>
    // j           L_15E0
    // .word       0x00094B04                   # sllv        $9, $9, $zero # 00000300 <InstrIdType: RSP_SPECIAL>
    r9 = S32(r9) << (0 & 31);
    goto L_15E0;
    // .word       0x00094B04                   # sllv        $9, $9, $zero # 00000300 <InstrIdType: RSP_SPECIAL>
    r9 = S32(r9) << (0 & 31);
    // .word       0x1F34D810                   # bgtz        $25, L_1494 # 00140000 <InstrIdType: RSP_NORMAL>
    if (RSP_SIGNED(r25) > 0) {
        // addi        $18, $20, -0x601
        r18 = RSP_ADD32(r20, -0X601);
        goto L_1494;
    }
    // addi        $18, $20, -0x601
    r18 = RSP_ADD32(r20, -0X601);
    // addi        $10, $6, 0x18
    r10 = RSP_ADD32(r6, 0X18);
    // addi        $10, $6, 0x11C
    r10 = RSP_ADD32(r6, 0X11C);
L_1460:
    // .word       0x42944600                   # INVALID     $v24, $v8, $v20[0h] # 00000000 <InstrIdType: RSP_COP0>
    // andi        $6, $9, 0x1000
    r6 = r9 & 0X1000;
    // .word       0x00093604                   # sllv        $6, $9, $zero # 00000600 <InstrIdType: RSP_SPECIAL>
    r6 = S32(r9) << (0 & 31);
    // .word       0x40E80020                   # INVALID     $v0, $v0, $v8[3h] # 00000020 <InstrIdType: RSP_COP0>
    // bne         $zero, $zero, L_08B4
    if (0 != 0) {
        // movn        $14, $25, $20
            goto L_08B4;
    }
    // movn        $14, $25, $20
    // .word       0x6432FFFF                   # INVALID     $v31, $v31, $v18[1] # 0000003F <InstrIdType: RSP_NORMAL>
    // .word       0x00098B04                   # sllv        $17, $9, $zero # 00000300 <InstrIdType: RSP_SPECIAL>
    r17 = S32(r9) << (0 & 31);
    // .word       0x1F34D810                   # bgtz        $25, L_14C4 # 00140000 <InstrIdType: RSP_NORMAL>
    if (RSP_SIGNED(r25) > 0) {
        // slti        $18, $4, -0x601
        r18 = RSP_SIGNED(r4) < -0X601 ? 1 : 0;
      