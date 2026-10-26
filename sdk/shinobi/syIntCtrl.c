/* the CPU's exception and interrupt entry: syIntInit installs it at VBR,
 * and syIntEnableInt sets the interrupt mask */
#include <machine.h>
#include <sg_xpt.h>
#include <shinobi.h>

/* the cache control register, and its bit that invalidates the instruction
 * cache */
/* @hack type: CCR not volatile, which lets SHC load syIntMastISR's address
 * between its read and its write, as the image does (0x8c65f51c in
 * 1ST_READ.BIN, 0x8ce0cbf0 in LOADER.BIN); as volatile, 3 of 191 words differ
 * in each image */
#define CCR (*(Uint32 *)0xff00001cUL)
#define CCR_ICI 0x800

/* the exception and interrupt codes, every 0x20 from 0x40 to 0x800 */
#define FIRST_CODE 0x40
#define CODE_END 0x820
#define CODE_STEP 0x20
/* the vectors syIntInit fills, as offsets from VBR: general exceptions, TLB
 * misses and interrupts */
#define GENERAL_VECTOR 0x100
#define TLB_MISS_VECTOR 0x400
#define INTERRUPT_VECTOR 0x600

/* the stubs' bounds, labels in syIntIsr.src. a stub runs from its top to its
 * bottom, the word after its last */
/* @hack type: the bounds declared as single words, so SHC loads each one's
 * address at each use, as bottom's in each pass of the copy (0x8c65f55c). it
 * pools a stub's length as one word, 0x34 (0x8c65f5ac, and 0x8ce0cc80 in
 * LOADER.BIN); as arrays, syIntInit is 12 bytes shorter and 181 of 191 words
 * differ */
extern Uint32 syIntExpISRTop;
extern Uint32 syIntExpISRBottom;
extern Uint32 syIntISRTop;
extern Uint32 syIntISRBottom;
/* the words the stubs jump through, and the two masters syIntInit puts
 * there. syIntMasterISR calls a code's handler on the interrupted stack.
 * syIntMasterISR_2 moves the outermost exception or interrupt to
 * syIntIntStack */
extern void (*syIntExpMastISR)(void);
extern void (*syIntMastISR)(void);
extern void syIntMasterISR(void);
extern void syIntMasterISR_2(void);
extern Sint32 syIntNestCount;
extern void *syIntIntStack;

const char *const sygIntBuild = "\nsyInt Ver 1.03 Build:Oct 30 1998 11:07:49\n";
const Uint16 sygIntVer[2] = {1, 3};

/* the inline bodies below. Clang never sees them, so it finds these static
 * functions undefined */
/* NOLINTBEGIN(clang-diagnostic-undefined-internal) */
static void purge_lines(void *address, Sint32 bytes);
static void unblock_exceptions(void);
/* NOLINTEND(clang-diagnostic-undefined-internal) */

/* copy a stub to VBR + offset, from top's word through bottom's, and call
 * purge_lines. to C, top and bottom each address one word, so the walk past
 * top and the length's subtraction are undefined. SHC compiles them as plain
 * address arithmetic */
/* @bug logic: purge_lines misses the copy. the address it gets scales
 * offset as longs, to VBR + 4 * offset (0x400, 0x1000 and 0x1800 at
 * 0x8c65f582, 0x8c65f5f4 and 0x8c65f648, and at 0x8ce0cc56, 0x8ce0ccc8 and
 * 0x8ce0cd1c in LOADER.BIN). the length it gets is bottom - top, the stub's
 * 13 longs, which purge_lines takes as 13 bytes, one line */
#define INSTALL_STUB(top, bottom, offset)                                      \
    do {                                                                       \
        Uint32 *to = (Uint32 *)((char *)get_vbr() + (offset));                 \
        Uint32 *from = (top);                                                  \
        while (from <= (bottom)) {                                             \
            *to++ = *from++;                                                   \
        }                                                                      \
        purge_lines((Uint32 *)get_vbr() + (offset), (bottom) - (top));         \
    } while (0)

/* clang-format off */
#ifndef __clang__
/* write back and invalidate bytes / 32 + 1 operand cache lines from
 * address */
/* @asm opcode: cache-line, OCBP @R4 at 0x8c65f5bc, 0x8c65f610 and 0x8c65f664,
 * in syIntInit, writes back and invalidates a line */
#pragma inline_asm(purge_lines)
static void purge_lines(void *address, Sint32 bytes)
{
        SHLR2       R5
        SHLR2       R5
        SHLR        R5
        ADD         #1,R5
?LINE:
        OCBP        @R4
        DT          R5
        BF/S        ?LINE
        ADD         #32,R4
}

/* clear SR.BL */
/* expanded into syIntInit at 0x8c65f6d8. the body works in R0 and R1, and
 * branches over its own word (BRA at 0x8c65f6e0) */
/* @asm open: every instruction is one SHC writes for C */
#pragma inline_asm(unblock_exceptions)
static void unblock_exceptions(void)
{
        STC         SR,R0
        MOV.L       ?BLOCK,R1
        AND         R1,R0
        LDC         R0,SR
        BRA         ?DONE
        NOP
        .ALIGN      4
?BLOCK:
        .DATA.L     H'EFFFFFFF
?DONE:
}

/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c65f4f8 follows the
 * body's own return at 0x8c65f4ee */
#pragma inline_asm(syIntEnableInt)
void syIntEnableInt(Uint32 level)
{
        SHLL        R4
        SHLL        R4
        SHLL        R4
        SHLL        R4
        STC         SR,R0
        MOV.L       ?MASK,R1
        AND         R1,R0
        OR          R4,R0
        LDC         R0,SR
        RTS
        NOP
        .ALIGN      4
?MASK:
        .DATA.L     H'EFFFFF0F
}
#endif
/* clang-format on */

void syIntInit(Uint32 mode, void *stack)
{
    Sint32 level = get_imask();
    Sint32 code;

    set_imask(IMASK_ALL);
    CCR |= CCR_ICI;
    if (mode == SYD_INT_MODE_INTSTACK) {
        syIntExpMastISR = syIntMasterISR_2;
        syIntMastISR = syIntMasterISR_2;
        syIntNestCount = 0;
        syIntIntStack = stack;
    } else {
        syIntExpMastISR = syIntMasterISR;
        syIntMastISR = syIntMasterISR;
        syIntNestCount = 0;
        syIntIntStack = NULL;
    }
    INSTALL_STUB(&syIntExpISRTop, &syIntExpISRBottom, GENERAL_VECTOR);
    INSTALL_STUB(&syIntExpISRTop, &syIntExpISRBottom, TLB_MISS_VECTOR);
    INSTALL_STUB(&syIntISRTop, &syIntISRBottom, INTERRUPT_VECTOR);
    set_imask(level);
    for (code = FIRST_CODE; code < CODE_END; code += CODE_STEP) {
        USER_VECT(code) = (Uint32)syIntBrkRteRte;
    }
    unblock_exceptions();
}
