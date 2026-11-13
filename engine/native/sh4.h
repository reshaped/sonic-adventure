/* the register file and memory of asm2c.py's C. an address is a 32-bit
 * offset into the domain (native.h). one register file serves every call,
 * since one host thread runs the game. a wrapper's first entry gives R15 a
 * stack of its own. a nested entry, which only the assembly's call out
 * makes, keeps that stack */
#ifndef SH4_H
#define SH4_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "native.h"
#include "play_math.h"

/* SZ is FPSCR.SZ, which FSCHG turns. LDS and STS move FPSCR without it */
static uint32_t R[16], T, FPUL, PR, MACH, MACL, GBR, FPSCR, SZ;
static union {
    float f[16];
    uint32_t u[16];
} F, X;
static unsigned char sh4_stack[1 << 16] __attribute__((aligned(32)));
static int sh4_depth;

#define AT(type, a) ((type *)(NATIVE_DOMAIN | (uint32_t)(a)))
#define U32(a) (*AT(uint32_t, a))
#define S16(a) (*AT(int16_t, a))
#define S8(a) (*AT(int8_t, a))
#define W32(a, v) (*AT(uint32_t, a) = (uint32_t)(v))
#define W16(a, v) (*AT(uint16_t, a) = (uint16_t)(v))
#define W8(a, v) (*AT(uint8_t, a) = (uint8_t)(v))

static inline void enter(void)
{
    if (sh4_depth++ == 0) {
        R[15] = (uint32_t)(uintptr_t)(sh4_stack + sizeof sh4_stack);
    }
}

static inline void leave(void)
{
    sh4_depth--;
}

/* an argument of the game's C in 32 bits. a pointer carries the domain's
 * high half, and a number carries 0 or all ones */
static inline uint32_t narrow(uintptr_t value)
{
    uint64_t wide = value;

    if (wide >> 32 && wide >> 32 != 0xffffffffU &&
        wide >> 32 != (uint64_t)NATIVE_DOMAIN >> 32) {
        fprintf(
            stderr,
            "sa1: a pointer outside the domain reached the assembly: %#lx\n",
            (unsigned long)value);
        abort();
    }
    return (uint32_t)value;
}

/* a register for the game's C: a pointer into the domain, whose low half a
 * number keeps */
static inline uintptr_t widened(uint32_t value)
{
    return value ? NATIVE_DOMAIN | value : 0;
}

/* FRCHG: the banks trade places */
static inline void frchg(void)
{
    float keep[16];

    memcpy(keep, F.f, sizeof keep);
    memcpy(F.f, X.f, sizeof keep);
    memcpy(X.f, keep, sizeof keep);
}

/* a JSR or JMP that asm2c.py couldn't call by name */
void native_jsr(uint32_t target);

#endif
