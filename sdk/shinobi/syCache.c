/* the SH-4's caches: CCR, the operand cache's lines over a range or whole,
 * and the pages of the half it can keep as RAM */
#include <machine.h>
#include <sg_xpt.h>
#include <shinobi.h>

/* the cache control register, and its bits */
/* @hack type: CCR isn't volatile, so SHC merges its accesses as the image
 * does. syCacheInit_P2_ never stores its argument to CCR alone, only with
 * CCR_ICI set (0x8c61ba96). syCacheOCBI reads CCR once for both mode bits
 * (0x8c61bb5c), as syCacheOCBP and syCacheOCWB do. as volatile, syCache.c's
 * code runs 4 bytes longer and 62 of the 759 words of the seven functions
 * that use CCR differ, in each image */
#ifdef GCC
/* GCC makes each access to CCR that the C writes, in the C's order, only
 * through a volatile lvalue */
#define CCR (*(volatile Uint32 *)0xff00001cUL)
#else
#define CCR (*(Uint32 *)0xff00001cUL)
#endif
#define CCR_ICI 0x800 /* invalidate the instruction cache */
#define CCR_OCI 0x8 /* invalidate the operand cache */
/* the numbers of CCR's mode bits. with CCR_ORA_BIT set, the operand cache
 * keeps half its entries as RAM. with CCR_OIX_BIT set, it selects an entry by
 * address bit 25 in place of bit 13 */
#define CCR_ORA_BIT 5
#define CCR_OIX_BIT 7
/* an operand cache entry's tag, in the cache's address array */
#define OPERAND_TAG(entry) (*(Uint32 *)(0xf4000000UL | ((entry) << 5)))
/* the operand cache's entries. with CCR_ORA_BIT set, the first
 * CACHED_ENTRIES of each half stay cache, and the rest are RAM */
#define OPERAND_ENTRIES 512
#define HALF_ENTRIES 256
#define CACHED_ENTRIES 128

/* a cache line's bytes, and an address's line as the 29-bit physical
 * address a tag holds */
#define LINE_SIZE 32
#define LINE_MASK 0x1fffffe0UL
/* the sizes from which a range walks the operand cache's address array,
 * entry by entry, rather than its own lines. syCacheOCBI takes the first,
 * and syCacheOCBP and syCacheOCWB the second */
#define INVALIDATE_WALK_SIZE 0x30000
#define WALK_SIZE 0x10000
/* the operand cache's RAM, as the CPU addresses it, and its 1 KB pages */
#define CACHE_RAM 0x7c000000UL
#define CACHE_RAM_SIZE 0x2000
#define RAM_PAGES 8
#define PAGE_SHIFT 10

char *sygCacheBuild = "\r\nsyCache Ver 1.05 Build:Nov 05 1998 18:21:53\r\n";
const Uint32 sygCacheVer = 0x00050001UL;

/* the pages in use. syCacheAllocCacheRAM adds a request's pages, and
 * syCacheFreeCacheRAM takes one away for each entry that matches */
static Sint32 cache_ram_pages;
/* each page's byte offset in the RAM, which every request that reaches the
 * page records */
static Uint16 cache_ram_page_offset[RAM_PAGES];

/* the inline bodies below. Clang never sees them, so it finds these static
 * functions undefined */
/* NOLINTBEGIN(clang-diagnostic-undefined-internal) */
static void prefetch_lines(void *address, Uint32 size);
static void enter_p2(void);
static void invalidate_lines(void *address, Uint32 size);
static void invalidate_low_entries(Uint32 first, Uint32 last, Uint32 keeps_ram);
static void invalidate_high_entries_indexed(Uint32 first, Uint32 last,
                                            Uint32 keeps_ram);
static void invalidate_high_entries(Uint32 first, Uint32 last,
                                    Uint32 keeps_ram);
static void purge_lines(void *address, Uint32 size);
static void purge_low_entries(Uint32 first, Uint32 last, Uint32 keeps_ram);
static void purge_high_entries_indexed(Uint32 first, Uint32 last,
                                       Uint32 keeps_ram);
static void purge_high_entries(Uint32 first, Uint32 last, Uint32 keeps_ram);
static void write_back_lines(void *address, Uint32 size);
static void write_back_low_entries(Uint32 first, Uint32 last, Uint32 keeps_ram);
static void write_back_high_entries_indexed(Uint32 first, Uint32 last,
                                            Uint32 keeps_ram);
static void write_back_high_entries(Uint32 first, Uint32 last,
                                    Uint32 keeps_ram);
static void settle_pipeline(void);
/* NOLINTEND(clang-diagnostic-undefined-internal) */

/* clang-format off */
#ifndef __clang__
/* prefetch the 32-byte lines over size bytes from address. address must be
 * line-aligned, and size above 0: DT takes a count of 0 as 2^32 */
/* expanded into syCachePREF at 0x8c61c154, after the BRA at 0x8c61c148 over
 * syCachePREF's pool */
/* @asm open: every instruction is one SHC writes for C */
#pragma inline_asm(prefetch_lines)
static void prefetch_lines(void *address, Uint32 size)
{
        ADD         #31,R5
        SHLR2       R5
        SHLR2       R5
        SHLR        R5
?LINE:
        PREF        @R4
        DT          R5
        BF/S        ?LINE
        ADD         #32,R4
}

#ifndef GCC
/* go on at the next instruction through its uncached P2 alias */
/* @asm open: no inspect rule reads its JMP to its own P2 alias */
#pragma inline_asm(enter_p2)
static void enter_p2(void)
{
        MOVA        ?UNCACHED,R0
        MOV.L       ?P2_BASE,R1
        OR          R0,R1
        JMP         @R1
        NOP
        .ALIGN      4
?P2_BASE:
        .DATA.L     H'A0000000
?UNCACHED:
}
#endif

/* invalidate the operand cache lines over size bytes from address. address
 * must be line-aligned, and size above 0 */
/* @asm opcode: cache-line, OCBI @R4 at 0x8c61bb18, in syCacheOCBI, invalidates
 * a line */
#pragma inline_asm(invalidate_lines)
static void invalidate_lines(void *address, Uint32 size)
{
        ADD         #31,R5
        SHLR2       R5
        SHLR2       R5
        SHLR        R5
?LINE:
        OCBI        @R4
        DT          R5
        BF/S        ?LINE
        ADD         #32,R4
}

#ifndef GCC
/* invalidate each line of entries 0 to 255 whose address lies from first to
 * last. a line's address is its tag with its entry's offset bits. with
 * keeps_ram set, entries 128 to 255 are RAM, and the walk stops after 127 */
/* @asm opcode: cache-line, OCBI @R1 at 0x8c61bbca, in syCacheOCBI, invalidates
 * a line */
#pragma inline_asm(invalidate_low_entries)
static void invalidate_low_entries(Uint32 first, Uint32 last, Uint32 keeps_ram)
{
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       #H'00000000,R0
        MOV.L       #H'F4000000,R8
        MOV.L       #H'1FFFFC00,R9
        MOV.L       #H'00001FFF,R10
        MOV.L       #H'00002000,R2
        MOV.L       #H'00000FE0,R3
        BRA         ?TEST
        NOP
?ENTRY:
        MOV         R0,R1
        OR          R8,R1
        MOV.L       @R1,R1
        AND         R9,R1
        OR          R0,R1
        CMP/HI      R1,R4
        BT          ?NEXT
        CMP/HI      R5,R1
        BT          ?NEXT
        OCBI        @R1
?NEXT:
        TST         R6,R6
        BT          ?STEP
        CMP/EQ      R0,R3
        BT          ?DONE
?STEP:
        ADD         #32,R0
?TEST:
        CMP/HI      R0,R2
        BT          ?ENTRY
?DONE:
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
}

/* invalidate_low_entries' walk over entries 256 to 511, for index mode. the
 * address takes the entry offset's low 13 bits a second time, which changes
 * nothing */
/* @bug uninitialized: the body ORs R12 into each address too (OR R12,R1 at
 * 0x8c61bc3c, and at 0x8ce0c024 in LOADER.BIN), though no argument passes it.
 * syCacheOCBI leaves last there (AND R3,R12 at 0x8c61bb62, and at 0x8ce0bf4a in
 * LOADER.BIN). an address passes the range test only when it sets no bit that
 * last clears, and the body then invalidates last's line, not its own */
/* @asm opcode: cache-line, OCBI @R1 at 0x8c61bc46, in syCacheOCBI, invalidates
 * a line */
#pragma inline_asm(invalidate_high_entries_indexed)
static void invalidate_high_entries_indexed(Uint32 first, Uint32 last, Uint32 keeps_ram)
{
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       #H'00002000,R0
        MOV.L       #H'F4000000,R8
        MOV.L       #H'1FFFFC00,R9
        MOV.L       #H'00001FFF,R10
        MOV.L       #H'00004000,R2
        MOV.L       #H'00002FE0,R3
        BRA         ?TEST
        NOP
?ENTRY:
        MOV         R0,R1
        OR          R8,R1
        MOV.L       @R1,R1
        AND         R9,R1
        OR          R0,R1
        MOV         R0,R11
        AND         R10,R11
        OR          R11,R1
        OR          R12,R1
        CMP/HI      R1,R4
        BT          ?NEXT
        CMP/HI      R5,R1
        BT          ?NEXT
        OCBI        @R1
?NEXT:
        TST         R6,R6
        BT          ?STEP
        CMP/EQ      R0,R3
        BT          ?DONE
?STEP:
        ADD         #32,R0
?TEST:
        CMP/HI      R0,R2
        BT          ?ENTRY
?DONE:
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
}

/* invalidate_low_entries' walk over entries 256 to 511, stopping after 383 with
 * keeps_ram set */
/* @asm opcode: cache-line, OCBI @R1 at 0x8c61bcb6, in syCacheOCBI, invalidates
 * a line */
#pragma inline_asm(invalidate_high_entries)
static void invalidate_high_entries(Uint32 first, Uint32 last, Uint32 keeps_ram)
{
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       #H'00002000,R0
        MOV.L       #H'F4000000,R8
        MOV.L       #H'1FFFFC00,R9
        MOV.L       #H'00001FFF,R10
        MOV.L       #H'00004000,R2
        MOV.L       #H'00002FE0,R3
        BRA         ?TEST
        NOP
?ENTRY:
        MOV         R0,R1
        OR          R8,R1
        MOV.L       @R1,R1
        AND         R9,R1
        OR          R0,R1
        CMP/HI      R1,R4
        BT          ?NEXT
        CMP/HI      R5,R1
        BT          ?NEXT
        OCBI        @R1
?NEXT:
        TST         R6,R6
        BT          ?STEP
        CMP/EQ      R0,R3
        BT          ?DONE
?STEP:
        ADD         #32,R0
?TEST:
        CMP/HI      R0,R2
        BT          ?ENTRY
?DONE:
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
}
#endif

/* write back and invalidate the operand cache lines over size bytes from
 * address. address must be line-aligned, and size above 0 */
/* @asm opcode: cache-line, OCBP @R4 at 0x8c61bd3c, in syCacheOCBP, writes back
 * and invalidates a line */
#pragma inline_asm(purge_lines)
static void purge_lines(void *address, Uint32 size)
{
        ADD         #31,R5
        SHLR2       R5
        SHLR2       R5
        SHLR        R5
?LINE:
        OCBP        @R4
        DT          R5
        BF/S        ?LINE
        ADD         #32,R4
}

#ifndef GCC
/* write back and invalidate each line of entries 0 to 255 whose address lies
 * from first to last, as invalidate_low_entries finds them */
/* @asm opcode: cache-line, OCBP @R1 at 0x8c61bdee, in syCacheOCBP, writes back
 * and invalidates a line */
#pragma inline_asm(purge_low_entries)
static void purge_low_entries(Uint32 first, Uint32 last, Uint32 keeps_ram)
{
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       #H'00000000,R0
        MOV.L       #H'F4000000,R8
        MOV.L       #H'1FFFFC00,R9
        MOV.L       #H'00001FFF,R10
        MOV.L       #H'00002000,R2
        MOV.L       #H'00000FE0,R3
        BRA         ?TEST
        NOP
?ENTRY:
        MOV         R0,R1
        OR          R8,R1
        MOV.L       @R1,R1
        AND         R9,R1
        OR          R0,R1
        CMP/HI      R1,R4
        BT          ?NEXT
        CMP/HI      R5,R1
        BT          ?NEXT
        OCBP        @R1
?NEXT:
        TST         R6,R6
        BT          ?STEP
        CMP/EQ      R0,R3
        BT          ?DONE
?STEP:
        ADD         #32,R0
?TEST:
        CMP/HI      R0,R2
        BT          ?ENTRY
?DONE:
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
}

/* purge_low_entries' walk over entries 256 to 511, for index mode. the address
 * takes the entry offset's low 13 bits a second time, which changes nothing */
/* @bug uninitialized: the body ORs R12 into each address too (OR R12,R1 at
 * 0x8c61be60, and at 0x8ce0c248 in LOADER.BIN), though no argument passes it.
 * syCacheOCBP leaves last there (AND R3,R12 at 0x8c61bd86, and at 0x8ce0c16e in
 * LOADER.BIN). an address passes the range test only when it sets no bit that
 * last clears, and the body then writes back and invalidates last's line, not
 * its own */
/* @asm opcode: cache-line, OCBP @R1 at 0x8c61be6a, in syCacheOCBP, writes back
 * and invalidates a line */
#pragma inline_asm(purge_high_entries_indexed)
static void purge_high_entries_indexed(Uint32 first, Uint32 last, Uint32 keeps_ram)
{
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       #H'00002000,R0
        MOV.L       #H'F4000000,R8
        MOV.L       #H'1FFFFC00,R9
        MOV.L       #H'00001FFF,R10
        MOV.L       #H'00004000,R2
        MOV.L       #H'00002FE0,R3
        BRA         ?TEST
        NOP
?ENTRY:
        MOV         R0,R1
        OR          R8,R1
        MOV.L       @R1,R1
        AND         R9,R1
        OR          R0,R1
        MOV         R0,R11
        AND         R10,R11
        OR          R11,R1
        OR          R12,R1
        CMP/HI      R1,R4
        BT          ?NEXT
        CMP/HI      R5,R1
        BT          ?NEXT
        OCBP        @R1
?NEXT:
        TST         R6,R6
        BT          ?STEP
        CMP/EQ      R0,R3
        BT          ?DONE
?STEP:
        ADD         #32,R0
?TEST:
        CMP/HI      R0,R2
        BT          ?ENTRY
?DONE:
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
}

/* purge_low_entries' walk over entries 256 to 511, stopping after 383 with
 * keeps_ram set */
/* @asm opcode: cache-line, OCBP @R1 at 0x8c61beda, in syCacheOCBP, writes back
 * and invalidates a line */
#pragma inline_asm(purge_high_entries)
static void purge_high_entries(Uint32 first, Uint32 last, Uint32 keeps_ram)
{
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       #H'00002000,R0
        MOV.L       #H'F4000000,R8
        MOV.L       #H'1FFFFC00,R9
        MOV.L       #H'00001FFF,R10
        MOV.L       #H'00004000,R2
        MOV.L       #H'00002FE0,R3
        BRA         ?TEST
        NOP
?ENTRY:
        MOV         R0,R1
        OR          R8,R1
        MOV.L       @R1,R1
        AND         R9,R1
        OR          R0,R1
        CMP/HI      R1,R4
        BT          ?NEXT
        CMP/HI      R5,R1
        BT          ?NEXT
        OCBP        @R1
?NEXT:
        TST         R6,R6
        BT          ?STEP
        CMP/EQ      R0,R3
        BT          ?DONE
?STEP:
        ADD         #32,R0
?TEST:
        CMP/HI      R0,R2
        BT          ?ENTRY
?DONE:
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
}
#endif

/* write back the operand cache lines over size bytes from address. address
 * must be line-aligned, and size above 0 */
/* @asm opcode: cache-line, OCBWB @R4 at 0x8c61bf60, in syCacheOCWB, writes back
 * a line */
#pragma inline_asm(write_back_lines)
static void write_back_lines(void *address, Uint32 size)
{
        ADD         #31,R5
        SHLR2       R5
        SHLR2       R5
        SHLR        R5
?LINE:
        OCBWB       @R4
        DT          R5
        BF/S        ?LINE
        ADD         #32,R4
}

#ifndef GCC
/* write back each line of entries 0 to 255 whose address lies from first to
 * last, as invalidate_low_entries finds them */
/* @asm opcode: cache-line, OCBWB @R1 at 0x8c61c012, in syCacheOCWB, writes back
 * a line */
#pragma inline_asm(write_back_low_entries)
static void write_back_low_entries(Uint32 first, Uint32 last, Uint32 keeps_ram)
{
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       #H'00000000,R0
        MOV.L       #H'F4000000,R8
        MOV.L       #H'1FFFFC00,R9
        MOV.L       #H'00001FFF,R10
        MOV.L       #H'00002000,R2
        MOV.L       #H'00000FE0,R3
        BRA         ?TEST
        NOP
?ENTRY:
        MOV         R0,R1
        OR          R8,R1
        MOV.L       @R1,R1
        AND         R9,R1
        OR          R0,R1
        CMP/HI      R1,R4
        BT          ?NEXT
        CMP/HI      R5,R1
        BT          ?NEXT
        OCBWB       @R1
?NEXT:
        TST         R6,R6
        BT          ?STEP
        CMP/EQ      R0,R3
        BT          ?DONE
?STEP:
        ADD         #32,R0
?TEST:
        CMP/HI      R0,R2
        BT          ?ENTRY
?DONE:
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
}

/* write_back_low_entries' walk over entries 256 to 511, for index mode. the
 * address takes the entry offset's low 13 bits a second time, which changes
 * nothing */
/* @bug uninitialized: the body ORs R12 into each address too (OR R12,R1 at
 * 0x8c61c084, and at 0x8ce0c46c in LOADER.BIN), though no argument passes it.
 * syCacheOCWB leaves last there (AND R3,R12 at 0x8c61bfaa, and at 0x8ce0c392 in
 * LOADER.BIN). an address passes the range test only when it sets no bit that
 * last clears, and the body then writes back last's line, not its own */
/* @asm opcode: cache-line, OCBWB @R1 at 0x8c61c08e, in syCacheOCWB, writes back
 * a line */
#pragma inline_asm(write_back_high_entries_indexed)
static void write_back_high_entries_indexed(Uint32 first, Uint32 last, Uint32 keeps_ram)
{
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       #H'00002000,R0
        MOV.L       #H'F4000000,R8
        MOV.L       #H'1FFFFC00,R9
        MOV.L       #H'00001FFF,R10
        MOV.L       #H'00004000,R2
        MOV.L       #H'00002FE0,R3
        BRA         ?TEST
        NOP
?ENTRY:
        MOV         R0,R1
        OR          R8,R1
        MOV.L       @R1,R1
        AND         R9,R1
        OR          R0,R1
        MOV         R0,R11
        AND         R10,R11
        OR          R11,R1
        OR          R12,R1
        CMP/HI      R1,R4
        BT          ?NEXT
        CMP/HI      R5,R1
        BT          ?NEXT
        OCBWB       @R1
?NEXT:
        TST         R6,R6
        BT          ?STEP
        CMP/EQ      R0,R3
        BT          ?DONE
?STEP:
        ADD         #32,R0
?TEST:
        CMP/HI      R0,R2
        BT          ?ENTRY
?DONE:
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
}

/* write_back_low_entries' walk over entries 256 to 511, stopping after 383 with
 * keeps_ram set */
/* @asm opcode: cache-line, OCBWB @R1 at 0x8c61c0fe, in syCacheOCWB, writes back
 * a line */
#pragma inline_asm(write_back_high_entries)
static void write_back_high_entries(Uint32 first, Uint32 last, Uint32 keeps_ram)
{
        MOV.L       R11,@-R15
        MOV.L       R10,@-R15
        MOV.L       R9,@-R15
        MOV.L       R8,@-R15
        MOV.L       #H'00002000,R0
        MOV.L       #H'F4000000,R8
        MOV.L       #H'1FFFFC00,R9
        MOV.L       #H'00001FFF,R10
        MOV.L       #H'00004000,R2
        MOV.L       #H'00002FE0,R3
        BRA         ?TEST
        NOP
?ENTRY:
        MOV         R0,R1
        OR          R8,R1
        MOV.L       @R1,R1
        AND         R9,R1
        OR          R0,R1
        CMP/HI      R1,R4
        BT          ?NEXT
        CMP/HI      R5,R1
        BT          ?NEXT
        OCBWB       @R1
?NEXT:
        TST         R6,R6
        BT          ?STEP
        CMP/EQ      R0,R3
        BT          ?DONE
?STEP:
        ADD         #32,R0
?TEST:
        CMP/HI      R0,R2
        BT          ?ENTRY
?DONE:
        MOV.L       @R15+,R8
        MOV.L       @R15+,R9
        MOV.L       @R15+,R10
        MOV.L       @R15+,R11
}
#endif
#endif
/* clang-format on */

void syCachePREF_s(void *address, Uint32 size)
{
    /* @hack spelling: size rounded in two statements, which keeps the function
     * frameless: it opens on ADD #31,R5 at 0x8c61ba6c (0x8ce0be54 in
     * LOADER.BIN). as one statement, SHC keeps a copy of size on a frame,
     * the function is 4 bytes longer and 10 of 10 words differ */
    size += 31;
    size >>= 5;
    do {
        prefetch(address);
        address = (char *)address + 32;
    } while (--size);
}

void syCacheInit_P2_(Uint32 form)
{
    syCachePurgeAll();
#ifdef GCC
    /* one store of form with CCR_ICI set, which SHC makes of the #else
     * side's two */
    CCR = form | CCR_ICI;
#else
    CCR = form;
    CCR = (CCR & ~CCR_ICI) | CCR_ICI;
#endif
    CCR = (CCR & ~CCR_OCI) | CCR_OCI;
}

void syCacheICI_P2_(void)
{
    CCR |= CCR_ICI;
}

void syCacheOCI_P2_(void)
{
    CCR |= CCR_OCI;
}

#ifdef GCC
/* code at its P2 address runs uncached */
#define P2(address) ((Uint32)(address) | 0xa0000000UL)
/* the bits of a line's address that its tag holds, 10 to 28 */
#define TAG_ADDRESS 0x1ffffc00UL

/* what walk_entries does to each line it finds: OCBI, OCBP or OCBWB */
enum line_op { INVALIDATE, PURGE, WRITE_BACK };
/* walk_entries' and clear_tags' types. their callers cast P2(walk_entries) or
 * P2(clear_tags) to one and call through it uncached */
typedef void entry_walk(Uint32 first, Uint32 last, enum line_op op);
typedef void tag_clear(void);

/* eight NOPs, for a program in P2 that has read or written the cache's
 * address array. the SH7750 manual asks it to run eight instructions or more
 * before it branches to cached code. the image's syCachePurgeAll ends its
 * uncached stretch on these. always inlined, they run in the P2 function
 * before its return */
static inline __attribute__((always_inline)) void settle_pipeline(void)
{
    __asm__ volatile(".rept 8\n\tnop\n\t.endr" : : : "memory");
}

/* apply op to each line the operand cache keeps whose address lies from
 * first to last, for syCacheOCBI, syCacheOCBP and syCacheOCWB. the walk
 * reads the address array, so its callers call it at its P2 address. it
 * calls nothing. noinline keeps GCC from splitting part of the walk into a
 * function of its own, which the walk would call at a cached address. it
 * returns to cached code, so it ends on settle_pipeline. the image's walks
 * return without settle_pipeline's NOPs, so they are GCC's addition */
static __attribute__((noinline)) void walk_entries(Uint32 first, Uint32 last,
                                                   enum line_op op)
{
    Uint32 keeps_ram;
    Uint32 entry;
    Uint32 line;

    keeps_ram = (CCR >> CCR_ORA_BIT) & 1;
    for (entry = 0; entry < OPERAND_ENTRIES; entry++) {
        if (keeps_ram && entry % HALF_ENTRIES >= CACHED_ENTRIES) {
            continue;
        }
        /* a line's address is its tag's address bits and its entry's
         * offset within the half, with index mode on or off. this repairs
         * the addresses that the image's walks over entries 256 to 511 form
         * in index mode. each walk ORs the entry's whole offset into the
         * address, which sets bit 13. index mode selects an entry by bit 25
         * in place of bit 13, so a line in those entries may clear bit 13.
         * OCBI, OCBP and OCBWB compare the address's bits 10 to 28 with the
         * entry's tag, so the walk misses such a line. each walk also ORs in
         * R12, as its @bug uninitialized tag names. the movie player runs in
         * index mode, from mwPlyInitSofdec to mwPlyFinishSofdec in
         * modules/movie/sdk/mwsfd.c */
        line = (OPERAND_TAG(entry) & TAG_ADDRESS) |
               (entry % HALF_ENTRIES * LINE_SIZE);
        if (line < first || line > last) {
            continue;
        }
        if (op == INVALIDATE) {
            __asm__ volatile("ocbi @%0" : : "r"(line) : "memory");
        } else if (op == PURGE) {
            __asm__ volatile("ocbp @%0" : : "r"(line) : "memory");
        } else {
            __asm__ volatile("ocbwb @%0" : : "r"(line) : "memory");
        }
    }
    settle_pipeline();
}
#endif

void syCacheOCBI(void *address, Sint32 size)
{
    Uint32 first;
    Uint32 last;
    Uint32 keeps_ram;
    Uint32 indexed;
    Uint16 saved_mask;

    if (size == 0) {
        return;
    }
    first = (Uint32)address & LINE_MASK;
    /* a negative size walks the address array: size is compared unsigned
     * (CMP/HS at 0x8c61bada) */
    /* @hack spelling: the walk in an else rather than after a return, which
     * jumps to the epilogue through R2 (JMP @R2 at 0x8c61bb22, and at
     * 0x8ce0bf0a in LOADER.BIN); with a return, R1, and 2 of 211 words
     * differ */
    if ((Uint32)size < INVALIDATE_WALK_SIZE) {
        invalidate_lines((void *)first,
                         ((Uint32)address & (LINE_SIZE - 1)) + size);
    } else {
        saved_mask = get_imask();
        set_imask(IMASK_ALL);
#ifdef GCC
        last = ((Uint32)address + size - 1) & LINE_MASK;
        ((entry_walk *)P2(walk_entries))(first, last, INVALIDATE);
#else
        enter_p2();
        last = ((Uint32)address + size - 1) & LINE_MASK;
        keeps_ram = (CCR >> CCR_ORA_BIT) & 1;
        indexed = (CCR >> CCR_OIX_BIT) & 1;
        invalidate_low_entries(first, last, keeps_ram);
        if (indexed) {
            invalidate_high_entries_indexed(first, last, keeps_ram);
        } else {
            invalidate_high_entries(first, last, keeps_ram);
        }
#endif
        set_imask(saved_mask);
    }
}

void syCacheOCBP(void *address, Sint32 size)
{
    Uint32 first;
    Uint32 last;
    Uint32 keeps_ram;
    Uint32 indexed;
    Uint16 saved_mask;

    if (size == 0) {
        return;
    }
    first = (Uint32)address & LINE_MASK;
    /* a negative size walks the address array: size is compared unsigned
     * (CMP/HS at 0x8c61bd08) */
    /* @hack spelling: the walk in an else rather than after a return, which
     * jumps to the epilogue through R2 (JMP @R2 at 0x8c61bd46, and at
     * 0x8ce0c12e in LOADER.BIN); with a return, R1, and 2 of 211 words
     * differ */
    if ((Uint32)size < WALK_SIZE) {
        purge_lines((void *)first, ((Uint32)address & (LINE_SIZE - 1)) + size);
    } else {
        saved_mask = get_imask();
        set_imask(IMASK_ALL);
#ifdef GCC
        last = ((Uint32)address + size - 1) & LINE_MASK;
        ((entry_walk *)P2(walk_entries))(first, last, PURGE);
#else
        enter_p2();
        last = ((Uint32)address + size - 1) & LINE_MASK;
        keeps_ram = (CCR >> CCR_ORA_BIT) & 1;
        indexed = (CCR >> CCR_OIX_BIT) & 1;
        purge_low_entries(first, last, keeps_ram);
        if (indexed) {
            purge_high_entries_indexed(first, last, keeps_ram);
        } else {
            purge_high_entries(first, last, keeps_ram);
        }
#endif
        set_imask(saved_mask);
    }
}

void syCacheOCWB(void *address, Uint32 size)
{
    Uint32 first;
    Uint32 last;
    Uint32 keeps_ram;
    Uint32 indexed;
    Uint16 saved_mask;

    if (size == 0) {
        return;
    }
    first = (Uint32)address & LINE_MASK;
    /* @hack spelling: the walk in an else rather than after a return, which
     * jumps to the epilogue through R2 (JMP @R2 at 0x8c61bf6a, and at
     * 0x8ce0c352 in LOADER.BIN); with a return, R1, and 2 of 211 words
     * differ */
    if (size < WALK_SIZE) {
        write_back_lines((void *)first,
                         ((Uint32)address & (LINE_SIZE - 1)) + size);
    } else {
        saved_mask = get_imask();
        set_imask(IMASK_ALL);
#ifdef GCC
        last = ((Uint32)address + size - 1) & LINE_MASK;
        ((entry_walk *)P2(walk_entries))(first, last, WRITE_BACK);
#else
        enter_p2();
        last = ((Uint32)address + size - 1) & LINE_MASK;
        keeps_ram = (CCR >> CCR_ORA_BIT) & 1;
        indexed = (CCR >> CCR_OIX_BIT) & 1;
        write_back_low_entries(first, last, keeps_ram);
        if (indexed) {
            write_back_high_entries_indexed(first, last, keeps_ram);
        } else {
            write_back_high_entries(first, last, keeps_ram);
        }
#endif
        set_imask(saved_mask);
    }
}

void syCachePREF(void *address, Uint32 size)
{
    prefetch_lines((void *)((Uint32)address & LINE_MASK),
                   ((Uint32)address & (LINE_SIZE - 1)) + size);
}

/* clang-format off */
#ifndef __clang__
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x8c61c170 follows the
 * body's own return at 0x8c61c16c */
#pragma inline_asm(syCacheMOVCAL)
void syCacheMOVCAL(void *address, Uint32 value)
{
        MOV         R5,R0
        MOVCA.L     R0,@R4
        RTS
        NOP
}

#ifndef GCC
/* eight NOPs, for the pipeline after the purge */
/* expanded into syCachePurgeAll at 0x8c61c238, after the BRA at 0x8c61c220 over
 * syCachePurgeAll's pool */
/* @asm open: every instruction is one SHC writes for C */
#pragma inline_asm(settle_pipeline)
static void settle_pipeline(void)
{
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
        NOP
}
#endif
#endif
/* clang-format on */

#ifdef GCC
/* write back and invalidate each line the operand cache keeps, by a store
 * of 0 to its entry's tag, for syCachePurgeAll. it runs from P2, calls
 * nothing, ends on settle_pipeline and is noinline, for the reasons
 * walk_entries gives */
static __attribute__((noinline)) void clear_tags(void)
{
    Uint32 keeps_ram;
    Uint32 entry;

    keeps_ram = (CCR >> CCR_ORA_BIT) & 1;
    for (entry = 0; entry < OPERAND_ENTRIES; entry++) {
        if (!keeps_ram || entry % HALF_ENTRIES < CACHED_ENTRIES) {
            OPERAND_TAG(entry) = 0;
        }
    }
    settle_pipeline();
}
#endif

void syCachePurgeAll(void)
{
    Uint32 entry;
    Uint16 saved_mask;

    saved_mask = get_imask();
    set_imask(IMASK_ALL);
#ifdef GCC
    ((tag_clear *)P2(clear_tags))();
    set_imask(saved_mask);
#else
    enter_p2();
    if (((CCR >> CCR_ORA_BIT) & 1) == 1) {
        for (entry = 0; entry < CACHED_ENTRIES; entry++) {
            OPERAND_TAG(entry) = 0;
        }
        for (entry = HALF_ENTRIES; entry < HALF_ENTRIES + CACHED_ENTRIES;
             entry++) {
            OPERAND_TAG(entry) = 0;
        }
    } else {
        for (entry = 0; entry < OPERAND_ENTRIES; entry++) {
            OPERAND_TAG(entry) = 0;
        }
    }
    set_imask(saved_mask);
    settle_pipeline();
#endif
}

void syCacheAllocCacheRAM(void **address, Sint32 pages)
{
    Uint32 page;

    for (page = cache_ram_pages;
         page < (Uint32)(cache_ram_pages + pages) && page < RAM_PAGES; page++) {
        cache_ram_page_offset[page] = page << PAGE_SHIFT;
    }
    /* @bug logic: page reaches RAM_PAGES for a request that ends on the
     * last page, which fits, as for one that doesn't (CMP/EQ #8,R0 at
     * 0x8c61c286, and at 0x8ce0c66e in LOADER.BIN). so no request gets the
     * last page, and a refused one still records its pages' offsets */
    /* @hack spelling: the goto past the stores, which builds *address in R2
     * with CACHE_RAM in R3 (OR R3,R2 at 0x8c61c292, and at 0x8ce0c67a in
     * LOADER.BIN); with an if or a return, the two trade and 6 of 42 words
     * differ */
    if (page == RAM_PAGES) {
        goto full;
    }
    *address = (void *)((cache_ram_pages << PAGE_SHIFT) | CACHE_RAM);
    cache_ram_pages += pages;
full:;
}

void syCacheFreeCacheRAM(void *address)
{
    Sint32 page;

    /* @bug logic: the loop compares byte offsets with a page number
     * (CMP/EQ R0,R3 at 0x8c61c2bc, and at 0x8ce0c6a4 in LOADER.BIN). it reads
     * entries cache_ram_pages down to 1, not the 0 to cache_ram_pages - 1
     * handed out. an entry written past 0 holds 1 KB or more, which no page
     * number reaches. so only an entry never written, which holds 0, matches,
     * and only for an address in page 0. each match takes one page from the
     * count */
    for (page = cache_ram_pages; page > 0; page--) {
        if (cache_ram_page_offset[page] ==
            (((Uint32)address & (CACHE_RAM_SIZE - 1)) >> PAGE_SHIFT)) {
            cache_ram_pages--;
        }
    }
}
