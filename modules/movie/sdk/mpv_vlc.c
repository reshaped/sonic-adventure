/* the MPEG-1 video decoder's variable-length code tables, and their builders */
#include <sg_xpt.h>
#include <sofdec.h>

#include "movie/sdk/mpv.h"

/* a run and level code's entry: the run in bits 0 to 7, and the level's
 * magnitude in bits 8 to 15. the bit after the code gives the level's sign.
 * RUN_LEVEL_BITS adds the bit count of the code and its sign bit, from bit 16
 * up, as mpvvlc_run_level_8's entries hold it */
#define RUN_LEVEL(run, level) ((level) << 8 | (run))
#define RUN_LEVEL_BITS(run, level, bits) ((bits) << 16 | RUN_LEVEL(run, level))
/* the run and level of the escape's entry. the block decoders know the
 * escape by its run of 64, which no other code's run reaches */
#define RUN_LEVEL_ESCAPE 64

/* an address increment entry: the code's bit count in bits 0 to 3, and the
 * increment in bits 4 to 9. an entry may also take in the macroblock type
 * code that follows the increment code. such an entry holds the type's
 * MPV_MB_* bits from bit 10 up, and its bit count covers both codes */
#define MBAI(increment, type, bits)                                            \
    ((Sint16)((increment) << 4 | ((type) << 10 | (bits))))
/* an entry that takes in its type code. it also sets MPV_MB_TYPED, which
 * tells a P or B picture's decoder to read no type code */
#define MBAI_TYPED(increment, type, bits)                                      \
    MBAI(increment, MPV_MB_TYPED | (type), bits)
/* the MPV_MB_* bits of a macroblock predicted from both frames */
#define MB_BIDIRECT (MPV_MB_FORWARD | MPV_MB_BACKWARD)

/* a macroblock type entry: the code's bit count in the low byte, and the
 * type's MPV_MB_* bits in the high byte */
#define MB_TYPE(type, bits) ((Sint16)((type) << 8 | (bits)))

/* a motion entry: the motion code, -16 to 16, in the low byte, and its
 * code's bit count in the high byte. the count includes the sign bit, which
 * 0's code lacks */
#define MOTION(code, bits) ((Sint16)((Uint8)(code) | (bits) << 8))

/* a coded block pattern entry: the code's bit count in bits 0 to 3, and the
 * pattern in bits 8 to 13. the pattern has a bit a block: Y0 to Y3 in bits
 * 13 to 10, Cb in 9 and Cr in 8. bits 15 and 14 repeat Cb's and Cr's bits */
#define CBP(pattern, bits)                                                     \
    ((Sint16)(((pattern) & 3) << 14 | (pattern) << 8 | (bits)))

/* a DC size entry: the code's bit count in the low nibble, and the size of
 * the DC difference in the high nibble */
#define DC_SIZE(size, bits) ((Sint8)((size) << 4 | (bits)))

/* the run and level codes with 6 to 11 leading zeros. run_level_4 holds the
 * codes with 6 zeros, run_level_2 those with 7 and run_level_1 those with 8.
 * run_level_0a, 0b and 0c hold those with 9, 10 and 11. a 1 ends a code's
 * zeros, and the bits after that 1 index the code's table: 3 bits in
 * run_level_4, and 4 in the rest. the sign bit comes last */
static const Uint16 mpvvlt_run_level_0c[16] = {
    RUN_LEVEL(1, 18), RUN_LEVEL(1, 17), RUN_LEVEL(1, 16), RUN_LEVEL(1, 15),
    RUN_LEVEL(6, 3),  RUN_LEVEL(16, 2), RUN_LEVEL(15, 2), RUN_LEVEL(14, 2),
    RUN_LEVEL(13, 2), RUN_LEVEL(12, 2), RUN_LEVEL(11, 2), RUN_LEVEL(31, 1),
    RUN_LEVEL(30, 1), RUN_LEVEL(29, 1), RUN_LEVEL(28, 1), RUN_LEVEL(27, 1),
};
static const Uint16 mpvvlt_run_level_0b[16] = {
    RUN_LEVEL(0, 40), RUN_LEVEL(0, 39), RUN_LEVEL(0, 38), RUN_LEVEL(0, 37),
    RUN_LEVEL(0, 36), RUN_LEVEL(0, 35), RUN_LEVEL(0, 34), RUN_LEVEL(0, 33),
    RUN_LEVEL(0, 32), RUN_LEVEL(1, 14), RUN_LEVEL(1, 13), RUN_LEVEL(1, 12),
    RUN_LEVEL(1, 11), RUN_LEVEL(1, 10), RUN_LEVEL(1, 9),  RUN_LEVEL(1, 8),
};
static const Uint16 mpvvlt_run_level_0a[16] = {
    RUN_LEVEL(0, 31), RUN_LEVEL(0, 30), RUN_LEVEL(0, 29), RUN_LEVEL(0, 28),
    RUN_LEVEL(0, 27), RUN_LEVEL(0, 26), RUN_LEVEL(0, 25), RUN_LEVEL(0, 24),
    RUN_LEVEL(0, 23), RUN_LEVEL(0, 22), RUN_LEVEL(0, 21), RUN_LEVEL(0, 20),
    RUN_LEVEL(0, 19), RUN_LEVEL(0, 18), RUN_LEVEL(0, 17), RUN_LEVEL(0, 16),
};
static const Uint16 mpvvlt_run_level_1[16] = {
    RUN_LEVEL(10, 2), RUN_LEVEL(9, 2),  RUN_LEVEL(5, 3),  RUN_LEVEL(3, 4),
    RUN_LEVEL(2, 5),  RUN_LEVEL(1, 7),  RUN_LEVEL(1, 6),  RUN_LEVEL(0, 15),
    RUN_LEVEL(0, 14), RUN_LEVEL(0, 13), RUN_LEVEL(0, 12), RUN_LEVEL(26, 1),
    RUN_LEVEL(25, 1), RUN_LEVEL(24, 1), RUN_LEVEL(23, 1), RUN_LEVEL(22, 1),
};
static const Uint16 mpvvlt_run_level_2[16] = {
    RUN_LEVEL(0, 11), RUN_LEVEL(8, 2),  RUN_LEVEL(4, 3),  RUN_LEVEL(0, 10),
    RUN_LEVEL(2, 4),  RUN_LEVEL(7, 2),  RUN_LEVEL(21, 1), RUN_LEVEL(20, 1),
    RUN_LEVEL(0, 9),  RUN_LEVEL(19, 1), RUN_LEVEL(18, 1), RUN_LEVEL(1, 5),
    RUN_LEVEL(3, 3),  RUN_LEVEL(0, 8),  RUN_LEVEL(6, 2),  RUN_LEVEL(17, 1),
};
static const Uint16 mpvvlt_run_level_4[8] = {
    RUN_LEVEL(16, 1), RUN_LEVEL(5, 2),  RUN_LEVEL(0, 7),  RUN_LEVEL(2, 3),
    RUN_LEVEL(1, 4),  RUN_LEVEL(15, 1), RUN_LEVEL(14, 1), RUN_LEVEL(4, 2),
};

/* the tables the decoder reads. each points at this file's array, or at its
 * copy in the ixa's vlc where MPVVLC_Init makes one */
Sint16 *mpvvlc_mbai_i_0;
Sint16 *mpvvlc_mbai_i_1;
Sint16 *mpvvlc_mbai_p_0;
Sint16 *mpvvlc_mbai_p_1;
Sint16 *mpvvlc_mbai_b_0;
Sint16 *mpvvlc_mbai_b_1;
Sint16 *mpvvlc_p_mbtype;
Sint16 *mpvvlc_b_mbtype;
Sint16 *mpvvlc_motion_0;
Sint16 *mpvvlc_motion_1;
Sint16 *mpvvlc_cbp;
Sint8 *mpvvlc_y_dcsiz;
Sint8 *mpvvlc_c_dcsiz;
const Uint16 *mpvvlc_run_level_0c;
const Uint16 *mpvvlc_run_level_0b;
const Uint16 *mpvvlc_run_level_0a;
const Uint16 *mpvvlc_run_level_1;
const Uint16 *mpvvlc_run_level_2;
const Uint16 *mpvvlc_run_level_4;
const Uint32 *mpvvlc_run_level_8;

static Sint16 mpvvlt_mbai_i_0[256];
static Sint16 mpvvlt_mbai_i_1[64];
static Sint16 mpvvlt_mbai_p_0[128];
static Sint16 mpvvlt_mbai_p_1[32];
static Sint16 mpvvlt_mbai_b_0[128];
static Sint16 mpvvlt_mbai_b_1[32];
static Sint16 mpvvlt_p_mbtype[32];
static Sint16 mpvvlt_b_mbtype[64];
static Sint16 mpvvlt_motion_0[128];
static Sint16 mpvvlt_motion_1[32];
static Sint16 mpvvlt_cbp[512];
static Sint8 mpvvlt_y_dcsiz[128];
static Sint8 mpvvlt_c_dcsiz[128];
static Uint32 mpvvlt_run_level_8[128];

static void mpvvlc_InitMbaiIpic(void);
static void mpvvlc_InitMbaiPpic(void);
static void mpvvlc_InitMbaiBpic(void);
static void mpvvlc_InitMbTypePpic(void);
static void mpvvlc_InitMbTypeBpic(void);
static void mpvvlc_InitMotion(void);
static void mpvvlc_InitCbp(void);
static void mpvvlc_InitDcSizY(void);
static void mpvvlc_InitDcSizC(void);
static void mpvvlc_InitIntRunLevel(void);
static void mpvvlc_SetDflPtr(void);
static void mpvvlc_SetupVlc(Uint8 *vlc);
static Uint8 *mpvvlc_SetVlcRunLevel(Uint8 *end);
static Uint8 *mpvvlc_SetVlcDcSiz(Uint8 *end);
static Uint8 *mpvvlc_SetVlcMotion(Uint8 *end);
static Uint8 *mpvvlc_SetVlcMbType(Uint8 *end);

Sint32 MPVVLC_IsVlcSizErr(void)
{
    return 0;
}

void MPVVLC_Init(Uint8 *vlc)
{
    mpvvlc_InitMbaiIpic();
    mpvvlc_InitMbaiPpic();
    mpvvlc_InitMbaiBpic();
    mpvvlc_InitMbTypePpic();
    mpvvlc_InitMbTypeBpic();
    mpvvlc_InitMotion();
    mpvvlc_InitCbp();
    mpvvlc_InitDcSizY();
    mpvvlc_InitDcSizC();
    /* @hack redundant: a goto to the next statement, which loads
     * mpvvlc_InitIntRunLevel's address into R3 and mpvvlc_SetDflPtr's into R2
     * (MOV.L at 0x8cedaff8 and 0x8cedaffe); without it, 6 of 40 words differ */
    goto run_level_tables;
run_level_tables:
    mpvvlc_InitIntRunLevel();
    mpvvlc_SetDflPtr();
    if (vlc != NULL) {
        mpvvlc_SetupVlc(vlc);
    }
}

void mpvvlc_InitMbai(void)
{
    mpvvlc_InitMbaiIpic();
    mpvvlc_InitMbaiPpic();
    mpvvlc_InitMbaiBpic();
}

/* the address increment codes of an I picture. mbai_i_0 holds those that
 * start with four zeros, by their next 8 bits, and mbai_i_1 the rest, by
 * their first 6. each increment's entry takes in the type code after it: 1
 * for an intra macroblock, or 01 for one with MPV_MB_QUANT */
static void mpvvlc_InitMbaiIpic(void)
{
    Sint16 *out = mpvvlt_mbai_i_0;
    int i;
    int j;
    Sint16 value;

    value = MBAI(MBAI_NO_CODE, 0, 0);
    i = 16;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MBAI(MBAI_ESCAPE, 0, 11);
    i = 2;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MBAI(MBAI_NO_CODE, 0, 0);
    i = 12;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MBAI(MBAI_STUFFING, 0, 11);
    i = 2;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MBAI(MBAI_NO_CODE, 0, 0);
    i = 16;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    for (i = 33; i >= 22; i--) {
        value = MBAI(i, MPV_MB_QUANT | MPV_MB_INTRA, 13);
        /* @hack redundant: a loop of one pass, which SHC reduces to its one
         * store. it still steers the function's registers: MOV R5,R6 at
         * 0x8cedb0aa, and OR R12,R13 at 0x8cedb0d8 in the loop of increments
         * 21 to 16; without it, 64 of 261 words differ */
        for (j = 1; j > 0; j--) {
            *out++ = value;
        }
        value = MBAI(i, MPV_MB_INTRA, 12);
        *out++ = value;
    }
    for (i = 21; i >= 16; i--) {
        value = MBAI(i, MPV_MB_QUANT | MPV_MB_INTRA, 12);
        j = 2;
        while (j != 0) {
            *out++ = value;
            j--;
        }
        value = MBAI(i, MPV_MB_INTRA, 11);
        j = 2;
        while (j != 0) {
            *out++ = value;
            j--;
        }
    }
    for (i = 15; i >= 10; i--) {
        value = MBAI(i, MPV_MB_QUANT | MPV_MB_INTRA, 10);
        j = 8;
        while (j != 0) {
            *out++ = value;
            j--;
        }
        value = MBAI(i, MPV_MB_INTRA, 9);
        j = 8;
        while (j != 0) {
            *out++ = value;
            j--;
        }
    }
    for (i = 9; i >= 8; i--) {
        value = MBAI(i, MPV_MB_QUANT | MPV_MB_INTRA, 9);
        j = 16;
        while (j != 0) {
            *out++ = value;
            j--;
        }
        value = MBAI(i, MPV_MB_INTRA, 8);
        j = 16;
        while (j != 0) {
            *out++ = value;
            j--;
        }
    }
    out = mpvvlt_mbai_i_1;
    value = MBAI(MBAI_NO_CODE, 0, 0);
    i = 4;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    for (i = 7; i >= 6; i--) {
        value = MBAI(i, MPV_MB_QUANT | MPV_MB_INTRA, 7);
        *out++ = value;
        value = MBAI(i, MPV_MB_INTRA, 6);
        *out++ = value;
    }
    for (i = 5; i >= 4; i--) {
        value = MBAI(i, MPV_MB_QUANT | MPV_MB_INTRA, 6);
        j = 2;
        while (j != 0) {
            *out++ = value;
            j--;
        }
        value = MBAI(i, MPV_MB_INTRA, 5);
        j = 2;
        while (j != 0) {
            *out++ = value;
            j--;
        }
    }
    for (i = 3; i >= 2; i--) {
        value = MBAI(i, MPV_MB_QUANT | MPV_MB_INTRA, 5);
        j = 4;
        while (j != 0) {
            *out++ = value;
            j--;
        }
        value = MBAI(i, MPV_MB_INTRA, 4);
        j = 4;
        while (j != 0) {
            *out++ = value;
            j--;
        }
    }
    value = MBAI(1, MPV_MB_QUANT | MPV_MB_INTRA, 3);
    i = 16;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MBAI(1, MPV_MB_INTRA, 2);
    i = 16;
    while (i != 0) {
        *out++ = value;
        i--;
    }
}

/* the address increment codes of a P picture. mbai_p_0 holds those that
 * start with four zeros, by their next 7 bits, and mbai_p_1 the rest, by
 * their first 5. where the type code after an increment code lies within the
 * bits that index the entry, the entry takes it in. incr holds the increment
 * alone, as MBAI(i, 0, 0) does. each entry is incr ORed with
 * MBAI(0, type, bits) */
static void mpvvlc_InitMbaiPpic(void)
{
    Sint16 *out = mpvvlt_mbai_p_0;
    Sint32 i;
    Sint32 incr;
    Sint16 value;

    value = MBAI(MBAI_NO_CODE, 0, 0);
    i = 8;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MBAI(MBAI_ESCAPE, 0, 11);
    *out++ = value;
    value = MBAI(MBAI_NO_CODE, 0, 0);
    i = 6;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MBAI(MBAI_STUFFING, 0, 11);
    *out++ = value;
    value = MBAI(MBAI_NO_CODE, 0, 0);
    i = 8;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    i = 33;
    incr = i << 4;
    for (; i >= 22; i--, incr -= 16) {
        *out++ = incr | MBAI(0, 0, 11);
    }
    i = 21;
    incr = i << 4;
    /* @hack spelling: incr = incr - 16, which keeps the loop's bound in R7 and
     * its typed entry in R14 (MOV #16,R7 at 0x8cedb2ea); as incr -= 16, 11 of
     * 241 words differ */
    for (; i >= 16; i--, incr = incr - 16) {
        *out++ = incr | MBAI(0, 0, 10);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 11);
    }
    i = 15;
    incr = i << 4;
    /* @hack spelling: incr = incr - 16, which gives the loop of increments 33
     * to 22 its i in R6 and its incr in R5 (MOV #33,R6 at 0x8cedb2d4); as
     * incr -= 16, 16 of 241 words differ */
    for (; i >= 10; i--, incr = incr - 16) {
        *out++ = incr | MBAI(0, 0, 8);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD, 11);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_PATTERN, 10);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_PATTERN, 10);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 9);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 9);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 9);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 9);
    }
    i = 9;
    incr = i << 4;
    for (; i >= 8; i--, incr -= 16) {
        *out++ = incr | MBAI(0, 0, 7);
        *out++ = incr | MBAI(0, 0, 7);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD, 10);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD, 10);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_PATTERN, 9);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_PATTERN, 9);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_PATTERN, 9);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_PATTERN, 9);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 8);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 8);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 8);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 8);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 8);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 8);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 8);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 8);
    }
    out = mpvvlt_mbai_p_1;
    value = MBAI(MBAI_NO_CODE, 0, 0);
    i = 2;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    i = 7;
    incr = i << 4;
    /* @hack spelling: incr = incr - 16, which has the function save R13, not
     * R12 (MOV.L R13,@-R15 at 0x8cedb276); as incr -= 16, 27 of 241 words
     * differ */
    for (; i >= 6; i--, incr = incr - 16) {
        *out++ = incr | MBAI(0, 0, 5);
    }
    i = 5;
    incr = i << 4;
    for (; i >= 4; i--, incr -= 16) {
        *out++ = incr | MBAI(0, 0, 4);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 5);
    }
    i = 3;
    incr = i << 4;
    /* @hack spelling: incr = incr - 16, which loads
     * MBAI_TYPED(0, MPV_MB_PATTERN, 5) into R13 (MOV.W at 0x8cedb414); as
     * incr -= 16, 11 of 241 words differ */
    for (; i >= 2; i--, incr = incr - 16) {
        *out++ = incr | MBAI(0, 0, 3);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_PATTERN, 5);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 4);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 4);
    }
    value = MBAI(1, 0, 1);
    *out++ = value;
    *out++ = value;
    value = MBAI_TYPED(1, MPV_MB_FORWARD, 4);
    *out++ = value;
    *out++ = value;
    value = MBAI_TYPED(1, MPV_MB_PATTERN, 3);
    *out++ = value;
    *out++ = value;
    *out++ = value;
    *out++ = value;
    value = MBAI_TYPED(1, MPV_MB_FORWARD | MPV_MB_PATTERN, 2);
    *out++ = value;
    *out++ = value;
    *out++ = value;
    *out++ = value;
    *out++ = value;
    *out++ = value;
    *out++ = value;
    *out++ = value;
}

/* the address increment codes of a B picture, in the form of a P picture's */
static void mpvvlc_InitMbaiBpic(void)
{
    Sint16 *out = mpvvlt_mbai_b_0;
    Sint32 i;
    Sint32 incr;
    int j;
    Sint16 value;

    value = MBAI(MBAI_NO_CODE, 0, 0);
    i = 8;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MBAI(MBAI_ESCAPE, 0, 11);
    *out++ = value;
    value = MBAI(MBAI_NO_CODE, 0, 0);
    i = 6;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MBAI(MBAI_STUFFING, 0, 11);
    *out++ = value;
    value = MBAI(MBAI_NO_CODE, 0, 0);
    i = 8;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    for (i = 33; i >= 22; i--) {
        *out++ = MBAI(i, 0, 11);
    }
    for (i = 21; i >= 16; i--) {
        /* @hack redundant: a loop of one pass, which SHC reduces to its one
         * store. it still steers the function's registers: MOV #21,R7 at
         * 0x8cedb508, and OR R13,R3 at 0x8cedb53e in the loop of increments 15
         * to 10; without it, 15 of 264 words differ */
        for (j = 1; j > 0; j--) {
            *out++ = MBAI(i, 0, 10);
        }
        *out++ = MBAI(i, 0, 10);
    }
    i = 15;
    incr = i << 4;
    /* @hack spelling: incr = incr - 16, which has the loop of increments 21
     * to 16 load 21 << 4 into R6 (MOV.W at 0x8cedb504); as incr -= 16, 25 of
     * 264 words differ */
    for (; i >= 10; i--, incr = incr - 16) {
        *out++ = incr | MBAI(0, 0, 8);
        *out++ = incr | MBAI(0, 0, 8);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_BACKWARD, 11);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_BACKWARD | MPV_MB_PATTERN, 11);
        *out++ = incr | MBAI_TYPED(0, MB_BIDIRECT, 10);
        *out++ = incr | MBAI_TYPED(0, MB_BIDIRECT, 10);
        *out++ = incr | MBAI_TYPED(0, MB_BIDIRECT | MPV_MB_PATTERN, 10);
        *out++ = incr | MBAI_TYPED(0, MB_BIDIRECT | MPV_MB_PATTERN, 10);
    }
    i = 9;
    incr = i << 4;
    for (; i >= 8; i--, incr -= 16) {
        *out++ = incr | MBAI(0, 0, 7);
        *out++ = incr | MBAI(0, 0, 7);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD, 11);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_FORWARD | MPV_MB_PATTERN, 11);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_BACKWARD, 10);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_BACKWARD, 10);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_BACKWARD | MPV_MB_PATTERN, 10);
        *out++ = incr | MBAI_TYPED(0, MPV_MB_BACKWARD | MPV_MB_PATTERN, 10);
        *out++ = incr | MBAI_TYPED(0, MB_BIDIRECT, 9);
        *out++ = incr | MBAI_TYPED(0, MB_BIDIRECT, 9);
        *out++ = incr | MBAI_TYPED(0, MB_BIDIRECT, 9);
        *out++ = incr | MBAI_TYPED(0, MB_BIDIRECT, 9);
        *out++ = incr | MBAI_TYPED(0, MB_BIDIRECT | MPV_MB_PATTERN, 9);
        *out++ = incr | MBAI_TYPED(0, MB_BIDIRECT | MPV_MB_PATTERN, 9);
        *out++ = incr | MBAI_TYPED(0, MB_BIDIRECT | MPV_MB_PATTERN, 9);
        *out++ = incr | MBAI_TYPED(0, MB_BIDIRECT | MPV_MB_PATTERN, 9);
    }
    out = mpvvlt_mbai_b_1;
    value = MBAI(MBAI_NO_CODE, 0, 0);
    i = 2;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    i = 7;
    incr = i << 4;
    /* @hack spelling: incr = incr - 16, which gives i R6 and incr R5 (MOV #7,R6
     * at 0x8cedb61a); as incr -= 16, 6 of 264 words differ */
    for (; i >= 6; i--, incr = incr - 16) {
        *out++ = incr | MBAI(0, 0, 5);
    }
    for (i = 5; i >= 4; i--) {
        *out++ = MBAI(i, 0, 4);
        *out++ = MBAI(i, 0, 4);
    }
    for (i = 3; i >= 2; i--) {
        *out++ = MBAI(i, 0, 3);
        *out++ = MBAI(i, 0, 3);
        *out++ = MBAI_TYPED(i, MB_BIDIRECT, 5);
        *out++ = MBAI_TYPED(i, MB_BIDIRECT | MPV_MB_PATTERN, 5);
    }
    value = MBAI(1, 0, 1);
    *out++ = value;
    *out++ = value;
    *out++ = MBAI_TYPED(1, MPV_MB_FORWARD, 5);
    *out++ = MBAI_TYPED(1, MPV_MB_FORWARD | MPV_MB_PATTERN, 5);
    value = MBAI_TYPED(1, MPV_MB_BACKWARD, 4);
    *out++ = value;
    *out++ = value;
    value = MBAI_TYPED(1, MPV_MB_BACKWARD | MPV_MB_PATTERN, 4);
    *out++ = value;
    *out++ = value;
    value = MBAI_TYPED(1, MB_BIDIRECT, 3);
    *out++ = value;
    *out++ = value;
    *out++ = value;
    *out++ = value;
    value = MBAI_TYPED(1, MB_BIDIRECT | MPV_MB_PATTERN, 3);
    *out++ = value;
    *out++ = value;
    *out++ = value;
    *out++ = value;
}

void mpvvlc_InitMbType(void)
{
    mpvvlc_InitMbTypePpic();
    mpvvlc_InitMbTypeBpic();
}

/* the macroblock type codes of a P picture, by their first 5 bits, for a
 * macroblock whose increment's entry didn't take its type code in */
static void mpvvlc_InitMbTypePpic(void)
{
    Sint16 *out = mpvvlt_p_mbtype;
    Sint32 i;
    Sint16 value;

    value = MB_TYPE(MPV_MB_QUANT | MPV_MB_INTRA, 6);
    *out++ = value;
    value = MB_TYPE(MPV_MB_QUANT | MPV_MB_PATTERN, 5);
    *out++ = value;
    value = MB_TYPE(MPV_MB_QUANT | MPV_MB_FORWARD | MPV_MB_PATTERN, 5);
    *out++ = value;
    value = MB_TYPE(MPV_MB_INTRA, 5);
    *out++ = value;
    value = MB_TYPE(MPV_MB_FORWARD, 3);
    i = 4;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MB_TYPE(MPV_MB_PATTERN, 2);
    i = 8;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MB_TYPE(MPV_MB_FORWARD | MPV_MB_PATTERN, 1);
    i = 16;
    while (i != 0) {
        *out++ = value;
        i--;
    }
}

/* the macroblock type codes of a B picture, by their first 6 bits. no code
 * starts with six zeros. the entry for six zeros holds every MPV_MB_* bit
 * but MPV_MB_TYPED, and a bit count of 0 */
static void mpvvlc_InitMbTypeBpic(void)
{
    Sint16 *out = mpvvlt_b_mbtype;
    Sint32 i;
    Sint16 value;

    value =
        MB_TYPE(MPV_MB_QUANT | MB_BIDIRECT | MPV_MB_PATTERN | MPV_MB_INTRA, 0);
    *out++ = value;
    value = MB_TYPE(MPV_MB_QUANT | MPV_MB_INTRA, 6);
    *out++ = value;
    value = MB_TYPE(MPV_MB_QUANT | MPV_MB_BACKWARD | MPV_MB_PATTERN, 6);
    *out++ = value;
    value = MB_TYPE(MPV_MB_QUANT | MPV_MB_FORWARD | MPV_MB_PATTERN, 6);
    *out++ = value;
    value = MB_TYPE(MPV_MB_QUANT | MB_BIDIRECT | MPV_MB_PATTERN, 5);
    i = 2;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MB_TYPE(MPV_MB_INTRA, 5);
    i = 2;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MB_TYPE(MPV_MB_FORWARD, 4);
    i = 4;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MB_TYPE(MPV_MB_FORWARD | MPV_MB_PATTERN, 4);
    i = 4;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MB_TYPE(MPV_MB_BACKWARD, 3);
    i = 8;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MB_TYPE(MPV_MB_BACKWARD | MPV_MB_PATTERN, 3);
    i = 8;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MB_TYPE(MB_BIDIRECT, 2);
    i = 16;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MB_TYPE(MB_BIDIRECT | MPV_MB_PATTERN, 2);
    i = 16;
    while (i != 0) {
        *out++ = value;
        i--;
    }
}

/* the motion codes. motion_0 holds those that start with four zeros, by their
 * next 7 bits, and motion_1 the rest, by their first 5 */
static void mpvvlc_InitMotion(void)
{
    Sint16 *out = mpvvlt_motion_0;
    Sint32 i;
    Sint32 j;
    Sint16 value;

    value = MOTION(MOTION_NO_CODE, 0);
    i = 24;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    for (i = 16; i >= 11; i--) {
        /* @hack spelling: MOTION(i, 11) and MOTION(-i, 11) built in three
         * statements each, which builds them in R1 (EXTU.B R5,R1 at
         * 0x8cedb80a); in one statement each, they take R0, and 25 of 147 words
         * differ */
        value = i;
        value &= 0xff;
        value |= 11 << 8;
        *out++ = value;
        value = -i;
        value &= 0xff;
        value |= 11 << 8;
        *out++ = value;
    }
    i = 10;
    for (; i >= 8; i--) {
        value = MOTION(i, 10);
        j = 2;
        while (j != 0) {
            *out++ = value;
            j--;
        }
        value = MOTION(-i, 10);
        j = 2;
        while (j != 0) {
            *out++ = value;
            j--;
        }
    }
    for (i = 7; i >= 5; i--) {
        value = MOTION(i, 8);
        j = 8;
        while (j != 0) {
            *out++ = value;
            j--;
        }
        value = MOTION(-i, 8);
        j = 8;
        while (j != 0) {
            *out++ = value;
            j--;
        }
    }
    value = MOTION(4, 7);
    i = 16;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MOTION(-4, 7);
    i = 16;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    out = mpvvlt_motion_1;
    value = MOTION(MOTION_NO_CODE, 0);
    i = 2;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MOTION(3, 5);
    *out++ = value;
    value = MOTION(-3, 5);
    *out++ = value;
    value = MOTION(2, 4);
    i = 2;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MOTION(-2, 4);
    i = 2;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MOTION(1, 3);
    i = 4;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MOTION(-1, 3);
    i = 4;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = MOTION(0, 1);
    i = 16;
    while (i != 0) {
        *out++ = value;
        i--;
    }
}

/* the coded block pattern codes, by their first 9 bits. 0 is no code */
static void mpvvlc_InitCbp(void)
{
    Sint16 *out = mpvvlt_cbp;
    Sint32 i;
    Sint16 value;

    value = 0;
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(39, 9);
    /* @hack redundant: a loop of one pass, which SHC keeps as a loop with its
     * test (BF 0x8cedb982 at 0x8cedb998); without it, 1074 of 1092 words
     * differ */
    for (i = 0; i < 1; i++) {
        *out++ = value;
    }
    value = CBP(27, 9);
    /* @hack redundant: a loop of one pass, which SHC keeps as a loop with its
     * test (BF at 0x8cedb9ba); without it, 1058 of 1092 words differ */
    for (i = 0; i < 1; i++) {
        *out++ = value;
    }
    value = CBP(59, 9);
    /* @hack redundant: a loop of one pass, which SHC keeps as a loop with its
     * test (BF at 0x8cedb9dc); without it, 1042 of 1092 words differ */
    for (i = 0; i < 1; i++) {
        *out++ = value;
    }
    value = CBP(55, 9);
    /* @hack redundant: a loop of one pass, which SHC keeps as a loop with its
     * test (BF at 0x8cedb9fe); without it, 1026 of 1092 words differ */
    for (i = 0; i < 1; i++) {
        *out++ = value;
    }
    value = CBP(47, 9);
    /* @hack redundant: a loop of one pass, which SHC keeps as a loop with its
     * test (BF at 0x8cedba20); without it, 1010 of 1092 words differ */
    for (i = 0; i < 1; i++) {
        *out++ = value;
    }
    value = CBP(31, 9);
    /* @hack redundant: a loop of one pass, which SHC keeps as a loop with its
     * test (BF at 0x8cedba56); without it, 1007 of 1092 words differ */
    for (i = 0; i < 1; i++) {
        *out++ = value;
    }
    value = CBP(58, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(54, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(46, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(30, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(57, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(53, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(45, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(29, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(38, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(26, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(37, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(25, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(43, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(23, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(51, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(15, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(42, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(22, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(50, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(14, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(41, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(21, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(49, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(13, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(35, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(19, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(11, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(7, 8);
    for (i = 0; i < 2; i++) {
        *out++ = value;
    }
    value = CBP(34, 7);
    for (i = 0; i < 4; i++) {
        *out++ = value;
    }
    value = CBP(18, 7);
    for (i = 0; i < 4; i++) {
        *out++ = value;
    }
    value = CBP(10, 7);
    for (i = 0; i < 4; i++) {
        *out++ = value;
    }
    value = CBP(6, 7);
    for (i = 0; i < 4; i++) {
        *out++ = value;
    }
    value = CBP(33, 7);
    for (i = 0; i < 4; i++) {
        *out++ = value;
    }
    value = CBP(17, 7);
    for (i = 0; i < 4; i++) {
        *out++ = value;
    }
    value = CBP(9, 7);
    for (i = 0; i < 4; i++) {
        *out++ = value;
    }
    value = CBP(5, 7);
    for (i = 0; i < 4; i++) {
        *out++ = value;
    }
    value = CBP(63, 6);
    for (i = 0; i < 8; i++) {
        *out++ = value;
    }
    value = CBP(3, 6);
    for (i = 0; i < 8; i++) {
        *out++ = value;
    }
    value = CBP(36, 6);
    for (i = 0; i < 8; i++) {
        *out++ = value;
    }
    value = CBP(24, 6);
    for (i = 0; i < 8; i++) {
        *out++ = value;
    }
    value = CBP(62, 5);
    for (i = 0; i < 16; i++) {
        *out++ = value;
    }
    value = CBP(2, 5);
    for (i = 0; i < 16; i++) {
        *out++ = value;
    }
    value = CBP(61, 5);
    for (i = 0; i < 16; i++) {
        *out++ = value;
    }
    value = CBP(1, 5);
    for (i = 0; i < 16; i++) {
        *out++ = value;
    }
    value = CBP(56, 5);
    for (i = 0; i < 16; i++) {
        *out++ = value;
    }
    value = CBP(52, 5);
    for (i = 0; i < 16; i++) {
        *out++ = value;
    }
    value = CBP(44, 5);
    for (i = 0; i < 16; i++) {
        *out++ = value;
    }
    value = CBP(28, 5);
    for (i = 0; i < 16; i++) {
        *out++ = value;
    }
    value = CBP(40, 5);
    for (i = 0; i < 16; i++) {
        *out++ = value;
    }
    value = CBP(20, 5);
    for (i = 0; i < 16; i++) {
        *out++ = value;
    }
    value = CBP(48, 5);
    for (i = 0; i < 16; i++) {
        *out++ = value;
    }
    value = CBP(12, 5);
    for (i = 0; i < 16; i++) {
        *out++ = value;
    }
    value = CBP(32, 4);
    for (i = 0; i < 32; i++) {
        *out++ = value;
    }
    value = CBP(16, 4);
    for (i = 0; i < 32; i++) {
        *out++ = value;
    }
    value = CBP(8, 4);
    for (i = 0; i < 32; i++) {
        *out++ = value;
    }
    value = CBP(4, 4);
    for (i = 0; i < 32; i++) {
        *out++ = value;
    }
    value = CBP(60, 3);
    for (i = 0; i < 64; i++) {
        *out++ = value;
    }
}

void mpvvlc_InitDcSiz(void)
{
    mpvvlc_InitDcSizY();
    mpvvlc_InitDcSizC();
}

/* the size codes of a Y block's DC difference, by their first 7 bits */
static void mpvvlc_InitDcSizY(void)
{
    Sint8 *out = mpvvlt_y_dcsiz;
    Sint32 i;
    Sint8 value;

    value = DC_SIZE(1, 2);
    for (i = 32; i > 0; i--) {
        *out++ = value;
    }
    value = DC_SIZE(2, 2);
    for (i = 32; i > 0; i--) {
        *out++ = value;
    }
    value = DC_SIZE(0, 3);
    for (i = 16; i > 0; i--) {
        *out++ = value;
    }
    value = DC_SIZE(3, 3);
    for (i = 16; i > 0; i--) {
        *out++ = value;
    }
    value = DC_SIZE(4, 3);
    for (i = 16; i > 0; i--) {
        *out++ = value;
    }
    value = DC_SIZE(5, 4);
    for (i = 8; i > 0; i--) {
        *out++ = value;
    }
    value = DC_SIZE(6, 5);
    for (i = 4; i > 0; i--) {
        *out++ = value;
    }
    value = DC_SIZE(7, 6);
    for (i = 2; i > 0; i--) {
        *out++ = value;
    }
    value = DC_SIZE(8, 7);
    for (i = 2; i > 0; i--) {
        *out++ = value;
    }
}

/* the size codes of a Cb or Cr block's DC difference, by their first 7 bits */
static void mpvvlc_InitDcSizC(void)
{
    Sint8 *out = mpvvlt_c_dcsiz;
    Sint32 i;
    Sint8 value;

    value = DC_SIZE(0, 2);
    i = 32;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = DC_SIZE(1, 2);
    i = 32;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = DC_SIZE(2, 2);
    i = 32;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = DC_SIZE(3, 3);
    i = 16;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = DC_SIZE(4, 4);
    i = 8;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = DC_SIZE(5, 5);
    i = 4;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = DC_SIZE(6, 6);
    i = 2;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = DC_SIZE(7, 7);
    *out++ = value;
    value = DC_SIZE(8, 8);
    *out++ = value;
}

/* the run and level codes that start with 1 to 5 zeros, the escape among
 * them, by the 7 bits after their first 0. the entries for six zeros or more
 * are 0, since those codes have tables of their own */
static void mpvvlc_InitIntRunLevel(void)
{
    Uint32 *out = mpvvlt_run_level_8;
    Sint32 i;
    Uint32 value;

    value = 0;
    i = 4;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = RUN_LEVEL_BITS(RUN_LEVEL_ESCAPE, RUN_LEVEL_ESCAPE, 6);
    i = 4;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = RUN_LEVEL_BITS(2, 2, 8);
    i = 2;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = RUN_LEVEL_BITS(9, 1, 8);
    i = 2;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = RUN_LEVEL_BITS(0, 4, 8);
    i = 2;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = RUN_LEVEL_BITS(8, 1, 8);
    i = 2;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = RUN_LEVEL_BITS(7, 1, 7);
    i = 4;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = RUN_LEVEL_BITS(6, 1, 7);
    i = 4;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = RUN_LEVEL_BITS(1, 2, 7);
    i = 4;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = RUN_LEVEL_BITS(5, 1, 7);
    i = 4;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = RUN_LEVEL_BITS(13, 1, 9);
    *out++ = value;
    value = RUN_LEVEL_BITS(0, 6, 9);
    *out++ = value;
    value = RUN_LEVEL_BITS(12, 1, 9);
    *out++ = value;
    value = RUN_LEVEL_BITS(11, 1, 9);
    *out++ = value;
    value = RUN_LEVEL_BITS(3, 2, 9);
    *out++ = value;
    value = RUN_LEVEL_BITS(1, 3, 9);
    *out++ = value;
    value = RUN_LEVEL_BITS(0, 5, 9);
    *out++ = value;
    value = RUN_LEVEL_BITS(10, 1, 9);
    *out++ = value;
    value = RUN_LEVEL_BITS(0, 3, 6);
    i = 8;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = RUN_LEVEL_BITS(4, 1, 6);
    i = 8;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = RUN_LEVEL_BITS(3, 1, 6);
    i = 8;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = RUN_LEVEL_BITS(0, 2, 5);
    i = 16;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = RUN_LEVEL_BITS(2, 1, 5);
    i = 16;
    while (i != 0) {
        *out++ = value;
        i--;
    }
    value = RUN_LEVEL_BITS(1, 1, 4);
    i = 32;
    while (i != 0) {
        *out++ = value;
        i--;
    }
}

void mpvvlc_InitRunLevel(void)
{
    mpvvlc_InitIntRunLevel();
}

static void mpvvlc_SetDflPtr(void)
{
    mpvvlc_mbai_i_0 = mpvvlt_mbai_i_0;
    mpvvlc_mbai_i_1 = mpvvlt_mbai_i_1;
    mpvvlc_mbai_p_0 = mpvvlt_mbai_p_0;
    mpvvlc_mbai_p_1 = mpvvlt_mbai_p_1;
    mpvvlc_mbai_b_0 = mpvvlt_mbai_b_0;
    mpvvlc_mbai_b_1 = mpvvlt_mbai_b_1;
    mpvvlc_p_mbtype = mpvvlt_p_mbtype;
    mpvvlc_b_mbtype = mpvvlt_b_mbtype;
    mpvvlc_motion_0 = mpvvlt_motion_0;
    mpvvlc_motion_1 = mpvvlt_motion_1;
    mpvvlc_cbp = mpvvlt_cbp;
    mpvvlc_y_dcsiz = mpvvlt_y_dcsiz;
    mpvvlc_c_dcsiz = mpvvlt_c_dcsiz;
    mpvvlc_run_level_0c = mpvvlt_run_level_0c;
    mpvvlc_run_level_0b = mpvvlt_run_level_0b;
    mpvvlc_run_level_0a = mpvvlt_run_level_0a;
    mpvvlc_run_level_1 = mpvvlt_run_level_1;
    mpvvlc_run_level_2 = mpvvlt_run_level_2;
    mpvvlc_run_level_4 = mpvvlt_run_level_4;
    mpvvlc_run_level_8 = mpvvlt_run_level_8;
}

/* copy the run and level, DC size, motion and macroblock type tables into
 * vlc, from its end down. their mpvvlc_ pointers then point at the copies.
 * each mpvvlc_SetVlc function copies its tables to just below end, and
 * returns the lowest copy's start */
static void mpvvlc_SetupVlc(Uint8 *vlc)
{
    Uint8 *end = vlc + sizeof(((struct mpv_ixa *)0)->vlc);

    end = mpvvlc_SetVlcRunLevel(end);
    end = mpvvlc_SetVlcDcSiz(end);
    end = mpvvlc_SetVlcMotion(end);
    end = mpvvlc_SetVlcMbType(end);
}

static Uint8 *mpvvlc_SetVlcRunLevel(Uint8 *end)
{
    /* @hack redundant: end's address taken, which keeps end in the frame
     * (MOV.L R2,@R15 at 0x8cedc642); without it, 77 of 80 words differ */
    (void)&end;
    end -= sizeof mpvvlt_run_level_8;
    mpvvlc_run_level_8 = (const Uint32 *)end;
    UTY_MemcpyDword((void *)mpvvlc_run_level_8, mpvvlt_run_level_8,
                    MPV_NWORD(mpvvlt_run_level_8));
    end -= sizeof mpvvlt_run_level_4;
    mpvvlc_run_level_4 = (const Uint16 *)end;
    UTY_MemcpyDword((void *)mpvvlc_run_level_4, mpvvlt_run_level_4,
                    MPV_NWORD(mpvvlt_run_level_4));
    end -= sizeof mpvvlt_run_level_2;
    mpvvlc_run_level_2 = (const Uint16 *)end;
    UTY_MemcpyDword((void *)mpvvlc_run_level_2, mpvvlt_run_level_2,
                    MPV_NWORD(mpvvlt_run_level_2));
    end -= sizeof mpvvlt_run_level_1;
    mpvvlc_run_level_1 = (const Uint16 *)end;
    UTY_MemcpyDword((void *)mpvvlc_run_level_1, mpvvlt_run_level_1,
                    MPV_NWORD(mpvvlt_run_level_1));
    end -= sizeof mpvvlt_run_level_0a;
    mpvvlc_run_level_0a = (const Uint16 *)end;
    UTY_MemcpyDword((void *)mpvvlc_run_level_0a, mpvvlt_run_level_0a,
                    MPV_NWORD(mpvvlt_run_level_0a));
    end -= sizeof mpvvlt_run_level_0b;
    mpvvlc_run_level_0b = (const Uint16 *)end;
    UTY_MemcpyDword((void *)mpvvlc_run_level_0b, mpvvlt_run_level_0b,
                    MPV_NWORD(mpvvlt_run_level_0b));
    end -= sizeof mpvvlt_run_level_0c;
    mpvvlc_run_level_0c = (const Uint16 *)end;
    UTY_MemcpyDword((void *)mpvvlc_run_level_0c, mpvvlt_run_level_0c,
                    MPV_NWORD(mpvvlt_run_level_0c));
    return end;
}

static Uint8 *mpvvlc_SetVlcDcSiz(Uint8 *end)
{
    /* @hack redundant: end's address taken, which keeps end in the frame
     * (MOV.L R2,@R15 at 0x8cedc6e0); without it, 31 of 31 words differ */
    (void)&end;
    end -= sizeof mpvvlt_y_dcsiz;
    mpvvlc_y_dcsiz = (Sint8 *)end;
    UTY_MemcpyDword(mpvvlc_y_dcsiz, mpvvlt_y_dcsiz, MPV_NWORD(mpvvlt_y_dcsiz));
    end -= sizeof mpvvlt_c_dcsiz;
    mpvvlc_c_dcsiz = (Sint8 *)end;
    UTY_MemcpyDword(mpvvlc_c_dcsiz, mpvvlt_c_dcsiz, MPV_NWORD(mpvvlt_c_dcsiz));
    return end;
}

static Uint8 *mpvvlc_SetVlcMotion(Uint8 *end)
{
    /* @hack redundant: end's address taken, which keeps end in the frame
     * (MOV.L R2,@R15 at 0x8cedc71e); without it, 29 of 30 words differ */
    (void)&end;
    end -= sizeof mpvvlt_motion_0;
    mpvvlc_motion_0 = (Sint16 *)end;
    UTY_MemcpyDword(mpvvlc_motion_0, mpvvlt_motion_0,
                    MPV_NWORD(mpvvlt_motion_0));
    end -= sizeof mpvvlt_motion_1;
    mpvvlc_motion_1 = (Sint16 *)end;
    UTY_MemcpyDword(mpvvlc_motion_1, mpvvlt_motion_1,
                    MPV_NWORD(mpvvlt_motion_1));
    return end;
}

static Uint8 *mpvvlc_SetVlcMbType(Uint8 *end)
{
    /* @hack redundant: end's address taken, which keeps end in the frame
     * (MOV.L R2,@R15 at 0x8cedc7ba); without it, 30 of 30 words differ */
    (void)&end;
    end -= sizeof mpvvlt_p_mbtype;
    mpvvlc_p_mbtype = (Sint16 *)end;
    UTY_MemcpyDword(mpvvlc_p_mbtype, mpvvlt_p_mbtype,
                    MPV_NWORD(mpvvlt_p_mbtype));
    end -= sizeof mpvvlt_b_mbtype;
    mpvvlc_b_mbtype = (Sint16 *)end;
    UTY_MemcpyDword(mpvvlc_b_mbtype, mpvvlt_b_mbtype,
                    MPV_NWORD(mpvvlt_b_mbtype));
    return end;
}
