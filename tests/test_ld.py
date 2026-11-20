import contextlib
import errno
import glob
import hashlib
import io
import os
import re
import shlex
import signal
import struct
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path
from unittest import mock

import elf
import gcclink
import ld
from support import MAKE
from tables import hexa

ROOT = Path(__file__).resolve().parents[1]
# the toolchain the fixture compiles and links with: the directory SH_ELF
# names, as in make dc, or else tools/sh-elf. GIVEN says whether SH_ELF
# names one
GIVEN = 'SH_ELF' in os.environ
TOOLCHAIN = Path(os.environ['SH_ELF']).resolve() if GIVEN else ROOT / 'tools/sh-elf'
TOOLS = ('bin/sh-elf-gcc', 'bin/sh-elf-as', 'bin/sh-elf-ar', 'bin/sh-elf-ld')
# a makefile, read from make's standard input, that prints a variable as config/gcc.mk gives it to a target. It takes
# the Makefile's lines that define quoted, a function config/gcc.mk calls, and SH_ELF, whose path it names GCC's own
# headers by. The fixture compiles with GCCFLAGS, as make dc does
PROBE = ('\n'.join(line for line in (ROOT / 'Makefile').read_text().splitlines()
                   if line.startswith(('quoted = ', 'SH_ELF ?= ')))
         + '\ninclude config/gcc.mk\n.PHONY: {target}\n{target}: ; $(info $({variable}))\n')
# the fixture's sources. SPEC links main and two overlays, ova and ovb, which share a load area. Each image opens with
# head.o's function, so no name another image takes lies at its origin, its provisional value. main calls each
# overlay's step through main_steps, and calls ova_probe and ovb_helper. A member of ovb's archive defines ovb_helper.
# main_slot points into ova_table. Each overlay takes main_call and main_counter from main. Nothing takes ova_unused
SOURCES = {
    'head.c': 'int head(void)\n{\n    return 0;\n}\n',
    'main.c': '''\
extern int ova_step(int x);
extern int ovb_step(int x);
extern int ova_probe(int x);
extern int ovb_helper(int x);
extern int ova_table[8];

int main_counter;
int (*main_steps[2])(int x) = {ova_step, ovb_step};
int *main_slot = &ova_table[3];

int main_call(int x)
{
    return x + main_counter;
}

void start(void)
{
    main_counter = main_steps[0](1) + main_steps[1](2) + ova_probe(3) + ovb_helper(4) + *main_slot;
}
''',
    'ova.c': '''\
extern int main_call(int x);
extern int main_counter;

int ova_table[8] = {1, 2, 3, 4, 5, 6, 7, 8};

int step(int x)
{
    return main_call(x) + ova_table[x & 7];
}

int ova_probe(int x)
{
    return x + main_counter;
}

int ova_unused(int x)
{
    return x * 3;
}
''',
    'ovb.c': '''\
extern int main_call(int x);
extern int main_counter;

/* on 64, so it lies past ova's step, which lies in ova's first 64 bytes */
__attribute__((aligned(64))) int step(int x)
{
    return main_call(x) - main_counter;
}
''',
    'helper.c': '''\
extern int main_counter;

int ovb_helper(int x)
{
    return x ^ main_counter;
}
''',
    # more bss for main
    'late.c': 'int main_late[4];\n',
    # a word that holds ova_step less its own address, under an R_SH_REL32
    'rel.s': '        .section .data.rel,"aw"\n        .global _main_rel\n        .align 2\n_main_rel:\n'
             '        .long _ova_step - .\n',
    # the word 16 under an R_SH_DIR32 against ova_table whose own addend is 4
    'reloc.s': '        .section .data.reloc,"aw"\n        .global _main_reloc\n        .align 2\n_main_reloc:\n'
               '        .reloc ., R_SH_DIR32, _ova_table + 4\n        .long 16\n',
    # a definition of main_call, which the overlays import
    'own.c': 'int main_call(int x)\n{\n    return x;\n}\n',
    # definitions of image_start and of D's start, boundaries of the link
    'start.s': ('        .section .data,"aw"\n        .global _image_start\n        .align 2\n_image_start:\n'
                '        .long 0\n'),
    'startof.s': ('        .section .data,"aw"\n        .global __startof_D\n        .align 2\n__startof_D:\n'
                  '        .long 0\n'),
    # a section the script doesn't place
    'orphan.s': '        .section .foo,"a"\n        .long 1\n',
    # a weak reference that nothing defines
    'weak.c': 'extern int maybe __attribute__((weak));\nint *main_weak = &maybe;\n',
    # a static record, for ova's head, which only its anchor names, so GCC keeps it as used
    'record.c': ('extern int step(int x);\n\nstruct record {\n    int (*entry)(int x);\n    int size;\n};\n\n'
                 'static struct record ova_record __attribute__((used)) = {step, 8};\n'),
    # main_bounds, a table of B's bounds, and 8 bytes of B. main_end, a word of DEND, holds DEND's end, for a tail row
    'bounds.s': ('        .section C,"aw",@progbits\n        .global _main_bounds\n        .align 2\n_main_bounds:\n'
                 '        .long __startof_B\n        .long __endof_B\n        .long __sizeof_B\n'
                 '        .section B,"aw",@nobits\n        .align 2\n        .space 8\n'
                 '        .section DEND,"aw",@progbits\n        .global _main_end\n        .align 2\n_main_end:\n'
                 '        .long __endof_DEND\n'),
    # a table of B32's start and end, as strt1.src's words hold them, and a word of B32
    'bounds32.s': ('        .section C,"aw",@progbits\n        .global _main_bounds32\n        .align 2\n'
                   '_main_bounds32:\n        .long __startof_B32\n        .long __endof_B32\n'
                   '        .section B32,"aw",@nobits\n        .align 2\n        .space 4\n'),
    # two sections that .data.* matches, one of them for a tail row
    'sibling.s': ('        .section .data.keep,"aw"\n        .global _main_keep\n        .align 2\n_main_keep:\n'
                  '        .long 1\n        .section .data.last,"aw"\n        .global _main_last\n        .align 2\n'
                  '_main_last:\n        .long 2\n'),
    # a pointer to alias_second, and alias_target, which a member of libtarget.a defines and alias_second aliases
    'second.c': 'extern int alias_second(int x);\n\nint (*second_slot)(int x) = alias_second;\n',
    'target.c': 'int alias_target(int x)\n{\n    return x + 1;\n}\n',
    # a table of D32's bounds and D's end, and D32, empty and on 32, as strt1.src's is
    'aligned.s': ('        .section C,"aw",@progbits\n        .global _main_aligned\n        .align 2\n'
                  '_main_aligned:\n        .long __startof_D32\n        .long __endof_D32\n'
                  '        .long __sizeof_D32\n        .long __endof_D\n        .section D32,"aw",@progbits\n'
                  '        .p2align 5\n'),
    # a datum of 64 bytes, for an image's .data, head or tail
    'more.c': 'int main_more[16] = {1};\n',
    # pointers to main_count and main_area, which main binds as an alias and a constant
    'names.c': ('extern int main_count;\nextern char main_area[];\n\nint *main_count_slot = &main_count;\n'
                'char *main_area_slot = main_area;\n'),
    # an entry in P, SHC's code section, which other objects' code takes too
    'entry.s': '        .section P,"ax",@progbits\n        .global _ova_entry\n_ova_entry:\n        rts\n        nop\n',
    # a C function, and a P section that branches to it, as the inline assembly's split writes one. translate writes a
    # BRA to C as its relocation, then the instruction with no displacement
    'mixed.c': ('int mixed_twice(int x)\n{\n    return x * 2;\n}\n\n'
                '__asm__("        .section P,\\"ax\\",@progbits\\n        .global _mixed_call\\n_mixed_call:\\n"\n'
                '        "        .reloc .,R_SH_IND12W,_mixed_twice-4\\n        .short 0xa000\\n        nop\\n");\n'),
    'debug.c': 'int debug_value = 3;\n',
    # a DWARF section that no option of GCCFLAGS writes
    'ranges.s': '        .section .debug_ranges,"",@progbits\n        .long 0\n',
    # an int and a pointer initialized to zero, and an int left unset
    'zeros.c': 'int zero_count = 0;\nchar *zero_name = 0;\nint unset_count;\n',
    # code that fills the SH-4's instruction cache, and two bytes more, for a kernel
    'whole.s': '        .section .text.whole,"ax",@progbits\n        .global _whole\n_whole:\n        .space 8192\n',
    'spill.s': '        .section .text.spill,"ax",@progbits\n        .global _spill\n_spill:\n        .space 2\n',
    # a function whose literal pool names its own code, pool_word, at a multiple of 16 KB, the operand cache's size,
    # and pool_far, 8 KB past pool_word; and pad.s, code that fills a line, for kernels
    'pool.s': ('        .section .text.pool,"ax",@progbits\n        .global _pool_read\n_pool_read:\n'
               '        mov.l   1f,r0\n        rts\n        mov.l   @r0,r0\n        .p2align 2\n'
               '1:      .long   _pool_word\n        .long   _pool_read\n        .long   _pool_far\n'
               '        .section .data.pool,"aw"\n        .global _pool_word\n        .p2align 14\n'
               '        .space 16384\n_pool_word:\n        .long 1\n        .space 8188\n        .global _pool_far\n'
               '_pool_far:\n        .long 2\n'),
    'pad.s': '        .section .text.pad,"ax",@progbits\n        .global _pad\n_pad:\n        .space 32\n',
    # a function whose literal pool names pool_zero, which lies in .bss at a multiple of 16 KB
    'pool_bss.s': ('        .section .text.pool_bss,"ax",@progbits\n        .global _pool_bss_read\n_pool_bss_read:\n'
                   '        mov.l   1f,r0\n        rts\n        mov.l   @r0,r0\n        .p2align 2\n'
                   '1:      .long   _pool_zero\n        .section .bss.pool,"aw",@nobits\n        .global _pool_zero\n'
                   '        .p2align 14\n_pool_zero:\n        .space 4\n'),
    # zam-sched's shape: sched's first pool line holds only a constant a MOV.L loads, and its second a word that
    # names sched_datum and one that names sched's own code, which lies on set 0. sched_on.s puts sched_datum on set 0
    # of the operand cache, and sched_off.s on set 2
    'sched.s': ('        .section .text.sched,"ax",@progbits\n        .global _sched\n_sched:\n'
                '        mov.l   1f,r1\n        mov.l   2f,r0\n        rts\n        mov.l   @r0,r0\n'
                '        .p2align 2\n1:      .long   0x3f800000\n        .p2align 5\n2:      .long   _sched_datum\n'
                '        .long   _sched\n'),
    'sched_on.s': ('        .section .data.sched,"aw"\n        .p2align 14\n        .space 16384\n'
                   '        .global _sched_datum\n_sched_datum:\n        .long 1\n'),
    'sched_off.s': ('        .section .data.sched,"aw"\n        .p2align 14\n        .space 16448\n'
                    '        .global _sched_datum\n_sched_datum:\n        .long 1\n'),
    # reach's pool line, on set 0, holds a word that names reach_datum, which starts 8 bytes before the end of a line
    # on set 511. reach_N.s gives reach_datum N bytes
    'reach.s': ('        .section .text.reach,"ax",@progbits\n        .global _reach\n_reach:\n'
                '        mov.l   1f,r0\n        rts\n        mov.l   @r0,r0\n        .p2align 2\n'
                '1:      .long   _reach_datum\n'),
    # pair_a fills a line, and its pool, on set 0, holds a constant alone. pair_b's, on set 1, names pair_datum, which
    # lies on set 0
    'pair.s': ('        .section .text.pair_a,"ax",@progbits\n        .global _pair_a\n_pair_a:\n'
               '        mov.l   1f,r0\n        rts\n        nop\n        .p2align 2\n1:      .long   0x3f800000\n'
               '        .p2align 5\n        .section .text.pair_b,"ax",@progbits\n        .global _pair_b\n_pair_b:\n'
               '        mov.l   1f,r0\n        rts\n        mov.l   @r0,r0\n        .p2align 2\n'
               '1:      .long   _pair_datum\n        .section .data.pair,"aw"\n        .p2align 14\n'
               '        .space 16384\n        .global _pair_datum\n_pair_datum:\n        .long 1\n'),
    # row_read's pool line, on set 0 in a window from ova's origin, names row_datum, which opens a .bss section of
    # small alignment, for a data row
    'row.s': ('        .section .text.row,"ax",@progbits\n        .global _row_read\n_row_read:\n'
              '        mov.l   1f,r0\n        rts\n        mov.l   @r0,r0\n        .p2align 2\n'
              '1:      .long   _row_datum\n        .section .bss.row,"aw",@nobits\n        .p2align 2\n'
              '        .global _row_datum\n_row_datum:\n        .space 4\n'),
    # code of 8 bytes and of 40, for a kernel's window
    'small.s': '        .section .text.small,"ax",@progbits\n        .global _small\n_small:\n        .space 8\n',
    'large.s': '        .section .text.large,"ax",@progbits\n        .global _large\n_large:\n        .space 40\n',
    # kept, 8 bytes of code, and in added.s 4 KB of the last code too, as port.h's LINKED_LAST marks a function a
    # change adds
    'kept.s': '        .section .text.kept,"ax",@progbits\n        .global _kept\n_kept:\n        .space 8\n',
    'added.s': ('        .section .text.kept,"ax",@progbits\n        .global _kept\n_kept:\n        .space 8\n'
                '        .section .text_last.added,"ax",@progbits\n        .global _added\n_added:\n'
                '        .space 4096\n'),
    # 64 bytes of the last code, for a last row
    'first.s': ('        .section .text_last.first,"ax",@progbits\n        .global _first\n_first:\n'
                '        .space 64\n'),
    # an object for a cold row: code, a string, data and .bss, and in cold_grown.s more of each
    'cold.s': ('        .section .text.cold,"ax",@progbits\n        .global _cold\n_cold:\n        .space 8\n'
               '        .section .rodata.str1.4,"aMS",@progbits,1\n        .string "cold"\n'
               '        .section .data.cold,"aw"\n        .global _cold_datum\n_cold_datum:\n        .long 1\n'
               '        .section .bss.cold,"aw",@nobits\n        .global _cold_zero\n_cold_zero:\n        .space 4\n'),
    'cold_grown.s': ('        .section .text.cold,"ax",@progbits\n        .global _cold\n_cold:\n        .space 72\n'
                     '        .section .rodata.str1.4,"aMS",@progbits,1\n        .string "cold, and more"\n'
                     '        .section .data.cold,"aw"\n        .global _cold_datum\n_cold_datum:\n        .long 1\n'
                     '        .long 2\n        .section .bss.cold,"aw",@nobits\n        .global _cold_zero\n'
                     '_cold_zero:\n        .space 36\n'),
    # PEXTRA, code that takes a run of its own after P's, since a word of C bounds it
    'extra.s': ('        .section PEXTRA,"ax",@progbits\n        .space 4\n        .section C,"aw",@progbits\n'
                '        .long __startof_PEXTRA\n'),
    # kept, and 8 bytes of the last data, as port.h's LINKED_LAST_DATA marks a datum a change adds
    'added_data.s': ('        .section .text.kept,"ax",@progbits\n        .global _kept\n_kept:\n        .space 8\n'
                     '        .section .data_last.added,"aw",@progbits\n        .global _added_datum\n'
                     '_added_datum:\n        .space 8\n'),
    # a function whose literal pool names last_datum, the last data, which follows a word on the operand cache's 16 KB
    'last_row.s': ('        .section .text.last_row,"ax",@progbits\n        .global _last_read\n_last_read:\n'
                   '        mov.l   1f,r0\n        rts\n        mov.l   @r0,r0\n        .p2align 2\n'
                   '1:      .long   _last_datum\n        .section .data.last_row,"aw"\n        .p2align 14\n'
                   '        .long 0\n        .section .data_last.row,"aw"\n        .p2align 2\n'
                   '        .global _last_datum\n_last_datum:\n        .long 0\n'),
    # DEXTRA, data that takes a run of its own after D's, since a word of C bounds it
    'dextra.s': ('        .section DEXTRA,"aw",@progbits\n        .space 4\n        .section C,"aw",@progbits\n'
                 '        .long __startof_DEXTRA\n'),
    # absorb_read's pool line, on set 2 in a window from ova's origin, names absorb_datum, which opens a .bss section
    # on 64 bytes. absorb_lead's 4 bytes open .bss ahead of it, on the operand cache's 16 KB, so it lies on set 2 too,
    # past 60 bytes that its alignment skips
    'absorb.s': ('        .section .text.absorb,"ax",@progbits\n        .global _absorb_read\n_absorb_read:\n'
                 '        mov.l   1f,r0\n        rts\n        mov.l   @r0,r0\n        .p2align 6\n'
                 '1:      .long   _absorb_datum\n        .section .bss.absorb_lead,"aw",@nobits\n'
                 '        .p2align 2\n        .space 4\n        .section .bss.absorb,"aw",@nobits\n'
                 '        .p2align 6\n        .global _absorb_datum\n        .size _absorb_datum, 4\n'
                 '_absorb_datum:\n        .space 4\n'),
    # shift_read's pool lines, on sets 0 and 2 in a window from ova's origin, name shift_const, which opens .rodata on
    # the operand cache's 16 KB, and shift_last, the last data. .data follows .rodata's 4 bytes, and shift_lead's 28
    # bytes put shift_last on set 1
    'shift.s': ('        .section .text.shift,"ax",@progbits\n        .global _shift_read\n_shift_read:\n'
                '        mov.l   1f,r0\n        mov.l   2f,r1\n        rts\n        mov.l   @r0,r0\n'
                '        .p2align 2\n1:      .long   _shift_const\n        .p2align 6\n2:      .long   _shift_last\n'
                '        .section .rodata.shift,"a"\n        .p2align 2\n        .global _shift_const\n'
                '        .size _shift_const, 4\n_shift_const:\n        .long 1\n'
                '        .section .data.shift_lead,"aw"\n        .p2align 2\n        .space 28\n'
                '        .section .data_last.shift,"aw"\n        .p2align 2\n'
                '        .global _shift_last\n        .size _shift_last, 4\n_shift_last:\n        .long 2\n'),
}
SOURCES.update((f'reach_{size}.s', '        .section .data.reach,"aw"\n        .p2align 14\n        .space 16376\n'
                f'        .global _reach_datum\n        .size _reach_datum, {size}\n_reach_datum:\n'
                f'        .space {size}\n') for size in (8, 16, 512))
# ova.c with a constant ahead of its data, which moves ova_table on: .rodata lies ahead of .data
SOURCES['ova_moved.c'] = 'const int ova_before[4] = {9, 9, 9, 9};\n' + SOURCES['ova.c']
# C whose tables hold layouts as the checkout's headers declare them. zam_layout holds the size, alignment and
# members' offsets of each SH4ZAM type the game's C takes. ninja_layout holds NJS_VIEW's and NJS_LIGHT's size and first
# and last members' offsets, and ninja_alignment their alignments. Without NONMATCHING, sg_xpt.h and njtypes.h give
# the matching side, whose size_t is another type than the one SH4ZAM's headers declare. The two can't compile
# together, so zam_layout is GCC's alone
LAYOUT = '''\
#include <sg_xpt.h>
#include <njtypes.h>

#define AT(type, member) __builtin_offsetof(type, member)
#define LAID(type, ...) sizeof(type), _Alignof(type), __VA_ARGS__

#ifdef NONMATCHING
#include <sh4zam/shz_matrix.h>
#include <sh4zam/shz_trig.h>

const unsigned int zam_layout[] = {
    LAID(shz_vec2_t, AT(shz_vec2_t, y)),
    LAID(shz_vec3_t, AT(shz_vec3_t, xy), AT(shz_vec3_t, z)),
    LAID(shz_vec4_t, AT(shz_vec4_t, xyz), AT(shz_vec4_t, w), AT(shz_vec4_t, zw)),
    LAID(shz_mat4x4_t, AT(shz_mat4x4_t, col), AT(shz_mat4x4_t, pos)),
    LAID(shz_sincos_t, AT(shz_sincos_t, cos)),
};
#endif
const unsigned int ninja_layout[] = {
    sizeof(NJS_VIEW), AT(NJS_VIEW, m), AT(NJS_VIEW, aroll),
    sizeof(NJS_LIGHT), AT(NJS_LIGHT, mtrx), AT(NJS_LIGHT, attr),
};
const unsigned int ninja_alignment[] = {_Alignof(NJS_VIEW), _Alignof(NJS_LIGHT)};
'''
SPEC = '''\
image   main  0x8c010000  0x00010000  start
image   ova   0x0c900000  0x00010000
image   ovb   0x0c900000  0x00010000
input   main  head.o
input   main  main.o
input   ova   head.o
input   ova   ova.o
input   ovb   head.o
input   ovb   ovb.o
input   ovb   libovb.a
# main keeps ova's step in the 0x8c view, and ova links at 0x0c900000
import  main  ova_step      ova   step          0x8c
import  main  ovb_step      ovb   step          0x0c
import  main  ova_table     ova   ova_table     0x0c
import  main  ova_probe     ova   ova_probe     0x0c
import  main  ovb_helper    ovb   ovb_helper    0x0c
import  ova   main_call     main  main_call     0x8c
import  ova   main_counter  main  main_counter  0x8c
import  ovb   main_call     main  main_call     0x8c
import  ovb   main_counter  main  main_counter  0x8c
'''
# a .gnu.lto_ section, as an object compiled with -flto carries GCC's intermediate code in sections of that name,
# which the linker leaves out of the image. CARRIED is a line of C that gives its object one
CARRIED = '__asm__("\\t.pushsection .gnu.lto_.fixture,\\"e\\",@progbits\\n\\t.long 0\\n\\t.popsection");\n'
# the sources of the link-time compile's fixture. fat.c and gone.c compile to objects that carry the code, as make dc's
# C objects do, and plain.c, whose text is fat.c's without CARRIED, to one that doesn't. ltrans/fat.c is what a
# link-time compile would keep of fat.c where nothing calls fat_peek: fat_peek and its static scratch are gone. It
# compiles with each datum in a section of its own, as GCC_LTO_LINK compiles. The fixture has no ltrans/gone.o: GCC
# keeps nothing of gone.c, whose one function it inlines at each call. fat_step lies ahead of fat_peek, so that a
# kernel row can place it at its object's start. It has no ltrans/lost.o and no ltrans/weak.o either, and each of
# those sources defines a datum by a global name, which a GCC that held the compile's roots would keep: lost.c's
# is its own, and weak.c's a weak definition of main_spare, which spare.o defines
LINK_TIME = {
    'fat.c': ('int fat_table[4] = {1, 2, 3, 4};\nstatic int fat_scratch[8];\n\n'
              'int fat_step(int x)\n{\n    return x + fat_table[1];\n}\n\n'
              'int *fat_peek(void)\n{\n    return fat_scratch;\n}\n' + CARRIED),
    'ltrans/fat.c': 'int fat_table[4] = {1, 2, 3, 4};\n\nint fat_step(int x)\n{\n    return x + fat_table[1];\n}\n',
    'gone.c': 'int gone_helper(int x)\n{\n    return x + 1;\n}\n' + CARRIED,
    'lost.c': 'int lost_count = 5;\n\nint lost_step(void)\n{\n    return ++lost_count;\n}\n' + CARRIED,
    'weak.c': '__attribute__((weak)) int main_spare[4] = {1, 2, 3, 4};\n' + CARRIED,
}
LINK_TIME['plain.c'] = LINK_TIME['fat.c'].replace(CARRIED, '')
# a stand-in for an sh-elf-gcc built with link-time optimization, for whole()'s command. For each input object that
# carries a .gnu.lto_ section and has an object at ltrans/PATH, it copies that object to OUT.ltransN.ltrans.o, as
# -save-temps names what GCC keeps of a partition, and prints the words by which -Q names the partition and its
# object. It writes its arguments to argv.txt beside OUT, a line each, and OUT, empty. A word of LTO_STAND_IN in the
# environment has it do otherwise: fails has it exit with status 1; warns has it print a warning of GCC's and exit
# with status 0, as GCC does without -Werror; assembles has it print one of GNU as's, which spells Warning with a
# capital, and exit with status 0, as GNU as does without --fatal-warnings; splits=PATH has it break PATH's words
# with a newline; misnames=PATH has it name PATH's partition for other.o; and doubles=PATH has it write PATH's
# partition twice
DRIVER = r'''#!/usr/bin/env python3
import os
import shutil
import sys
from pathlib import Path

arguments = sys.argv[1:]
out = arguments[arguments.index('-o') + 1]
Path(out).with_name('argv.txt').write_text('\n'.join(arguments) + '\n')
how = dict(word.partition('=')[::2] for word in os.environ.get('LTO_STAND_IN', '').split())
taken = [path for path in arguments if path.endswith('.o') and b'.gnu.lto_' in Path(path).read_bytes()
         and Path('ltrans', path).is_file()]
print('Reading object files:', *taken, file=sys.stderr)
print('Streaming out', end='', file=sys.stderr)
number = 0
for path in taken:
    for _ in range(2 if how.get('doubles') == path else 1):
        shutil.copy(Path('ltrans', path), f'{out}.ltrans{number}.ltrans.o')
        named = 'other.o' if how.get('misnames') == path else path
        between = '\n<another line> ' if how.get('splits') == path else ' '
        print(f' {out}.ltrans{number}.o ({named}{between}10 insns)', end='', file=sys.stderr)
        number += 1
print(file=sys.stderr)
if 'warns' in how:
    print("fat.c:1:5: warning: type of 'fat_table' does not match original declaration [-Wlto-type-mismatch]",
          file=sys.stderr)
if 'assembles' in how:
    print(f'{out}.ltrans0.ltrans.s: Assembler messages:', file=sys.stderr)
    print(f'{out}.ltrans0.ltrans.s:12: Warning: overflow in literal (.long)', file=sys.stderr)
if 'fails' in how:
    sys.exit('lto1: fatal error: the stand-in fails')
Path(out).write_bytes(b'')
'''


def flags(variable='GCCFLAGS', target='flags'):
    """The words of variable, GCCFLAGS by default, as config/gcc.mk gives it to target. They are split, and a word
    with a glob expanded from the checkout's root, as the shell does in make dc's recipes. A make that runs the tests
    passes none of its flags on. config/gcc.mk gives the default target no value of its own."""
    env = {name: value for name, value in os.environ.items() if name not in ('MAKEFLAGS', 'MFLAGS', 'MAKELEVEL')}
    done = subprocess.run([MAKE, '-s', '-f', '-', target], cwd=ROOT, env=env,
                          input=PROBE.format(variable=variable, target=target), stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, text=True)
    if done.returncode or done.stderr:
        raise AssertionError(done.stderr)
    words = []
    for word in shlex.split(done.stdout):
        found = sorted(glob.glob(str(ROOT / word))) if '*' in word else []
        words += [os.path.relpath(path, ROOT) for path in found] or [word]
    return words


def toolchain(tools=TOOLS):
    """Skip the test where tools/sh-elf lacks one of tools, and fail it where the directory SH_ELF names does."""
    missing = [tool for tool in tools if not (TOOLCHAIN / tool).is_file()]
    if missing and GIVEN:
        raise AssertionError(f'{TOOLCHAIN}, which SH_ELF names, lacks {", ".join(missing)}')
    if missing:
        raise unittest.SkipTest(f'{TOOLCHAIN} lacks {", ".join(missing)}, which make dc builds')


def run(*argv, root):
    """Run argv in the directory root. Its first word names a tool of the toolchain, as bin/sh-elf-gcc does. It fails
    with what the tool printed where the tool exits nonzero or prints anything, as make dc fails on a warning."""
    done = subprocess.run([str(TOOLCHAIN / argv[0]), *argv[1:]], cwd=root, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True)
    if done.returncode or done.stdout:
        raise AssertionError(done.stdout)


def words(linked, name, count):
    """The first count 32-bit words at name, a C name that linked's ELF defines."""
    symbol = gcclink.exports(linked.elf)[name]
    section = linked.elf.sections[symbol.section]
    return list(struct.unpack_from(f'<{count}I', section.data, symbol.value - section.address))


def importing(images, consumer, **imports):
    """images, with consumer importing each of imports, an Import by its ALIAS. An import of an ALIAS that consumer
    imports already replaces the old one."""
    image = images[consumer]
    return {**images, consumer: image._replace(imports={**image.imports, **imports})}


def linking(images, name, *inputs):
    """images, with the image named name linking head.o and then inputs, in place of its own inputs."""
    return {**images, name: images[name]._replace(inputs=['head.o', *inputs])}


class Fixture(unittest.TestCase):
    """The fixture of the tests that link: SOURCES, compiled and archived with the toolchain, and SPEC's images, which
    each test links under a directory of its own."""

    @classmethod
    def setUpClass(cls):
        toolchain()
        patched = mock.patch.object(gcclink, 'SH_ELF', TOOLCHAIN)
        patched.start()
        cls.addClassCleanup(patched.stop)
        compiled = flags()
        temporary = tempfile.TemporaryDirectory()
        cls.addClassCleanup(temporary.cleanup)
        cls.root = Path(temporary.name)
        for name, text in SOURCES.items():
            (cls.root / name).write_text(text)
        for name in SOURCES:
            run('bin/sh-elf-gcc', *compiled, '-c', '-o', str(Path(name).with_suffix('.o')), name, root=cls.root)
        # -fdata-sections gives spare.o's datum a section of its own, .bss.main_spare
        (cls.root / 'spare.c').write_text('int main_spare[4];\n')
        run('bin/sh-elf-gcc', *compiled, '-fdata-sections', '-c', '-o', 'spare.o', 'spare.c', root=cls.root)
        run('bin/sh-elf-ar', 'rc', 'libovb.a', 'helper.o', root=cls.root)
        run('bin/sh-elf-ar', 'rc', 'libtarget.a', 'target.o', root=cls.root)
        (cls.root / 'link.txt').write_text(SPEC)
        cls.images = gcclink.read(cls.root / 'link.txt')
        # a heat, and the heat row of a record that was written by it
        (cls.root / 'heat.txt').write_text('frames  1\n')
        cls.heat = ['heat.txt', hashlib.sha256(b'frames  1\n').hexdigest()]

    def setUp(self):
        # each test links under a directory named for it, in root, where the spec's inputs lie
        self.out = Path(self.id().rsplit('.', 1)[1])
        self.addCleanup(os.chdir, os.getcwd())
        os.chdir(self.root)

    def layouts(self, images, roots=None, pads=None):
        """Each image's layout pass, by image. roots gives each image's roots, as rooted() does by default, and pads
        each data row's pad, as link() takes them."""
        roots = roots or gcclink.rooted(images)
        return {name: ld.link(image, roots[name], ld.provisional(images, image), self.out, 'layout', pads)
                for name, image in images.items()}

    def finals(self, images, values):
        """Each image's final pass, by image, with its imports' values as values gives them."""
        roots = gcclink.rooted(images)
        return {name: ld.link(image, roots[name], values[name], self.out, 'final') for name, image in images.items()}


class Link(Fixture):
    def test_binds_each_import_to_its_provider_s_final_definition_in_its_view(self):
        bindings, layouts, finals = ld.coordinate(self.images, self.out)
        defined = {name: gcclink.exports(linked.elf) for name, linked in finals.items()}
        step, other = defined['ova']['step'].value, defined['ovb']['step'].value
        table = defined['ova']['ova_table'].value
        # ova's and ovb's step lie apart in the 0x0c view, and main's ova_step takes ova's in the 0x8c view
        self.assertEqual([step >> 24, other >> 24, table >> 24], [0x0C, 0x0C, 0x0C])
        self.assertNotEqual(step, other)
        self.assertEqual(bindings['main'], {
            'ova_step': step + 0x80000000, 'ovb_step': other, 'ova_table': table,
            'ova_probe': defined['ova']['ova_probe'].value, 'ovb_helper': defined['ovb']['ovb_helper'].value})
        for overlay in ('ova', 'ovb'):
            self.assertEqual(bindings[overlay], {name: defined['main'][name].value
                                                 for name in ('main_call', 'main_counter')})
        # no import's final value is its provisional one, so a resolve that kept the provisional values fails here
        for name, image in self.images.items():
            provisional = ld.provisional(self.images, image)
            for alias, value in bindings[name].items():
                self.assertNotEqual(provisional[alias], value, (name, alias))
        # main's words hold the bindings: main_steps holds the two steps, and main_slot points to ova_table[3]. main.o
        # keeps that pointer as the word 12, under a relocation whose own addend is 0. The final ELF keeps that
        # relocation over the relocated word, its addend still 0
        self.assertEqual(words(finals['main'], 'main_steps', 2) + words(finals['main'], 'main_slot', 1),
                         [step + 0x80000000, other, table + 12])
        main = elf.read(Path('main.o').read_bytes(), 'main.o')
        held = gcclink.exports(main)['main_slot']
        slot = next(relocation for relocation in main.relocations[held.section] if relocation.offset == held.value)
        self.assertEqual((main.symbols[slot.symbol].name, slot.addend,
                          main.sections[held.section].data[held.value:held.value + 4]),
                         ('_ova_table', 0, (12).to_bytes(4, 'little')))
        final = finals['main'].elf
        kept = [relocation for relocations in final.relocations.values() for relocation in relocations
                if relocation.offset == defined['main']['main_slot'].value]
        self.assertEqual([(final.symbols[relocation.symbol].name, relocation.addend) for relocation in kept],
                         [('_ova_table', 0)])
        # the passes lay each image out alike. ld collects no section, so ova keeps ova_unused, which nothing takes.
        # ovb takes libovb.a's member for its root alone
        signatures = {name: ld.signature(linked) for name, linked in layouts.items()}
        self.assertEqual(signatures, {name: ld.signature(linked) for name, linked in finals.items()})
        self.assertIn('ova_unused', signatures['ova'])
        self.assertTrue(any(item.startswith("libovb.a(helper.o)'s ") for item in signatures['ovb']), signatures['ovb'])
        for name, image in self.images.items():
            self.assertTrue(signatures[name]['image_start'].startswith(f'{hexa(image.origin)}, 0 bytes of '))
        self.assertEqual(ld.moved(layouts, finals), [])

    def test_compiles_an_object_initialized_to_zero_into_data_as_shc_puts_it_in_d(self):
        # GCCFLAGS put an object initialized to zero in .data, and one left unset in .bss. So an image's file holds the
        # zeros its data needs, and can end before a bss that nothing zeroes
        found = elf.read((self.root / 'zeros.o').read_bytes(), 'zeros.o')
        held = {symbol.name[1:]: found.sections[symbol.section].name for symbol in found.symbols
                if symbol.name in ('_zero_count', '_zero_name', '_unset_count')}
        self.assertEqual(held, {'zero_count': '.data', 'zero_name': '.data', 'unset_count': '.bss'})

    def test_lays_out_sh4zam_s_types_as_its_library_does_and_a_view_s_and_a_light_s_matrix_on_8(self):
        # the game's C takes SH4ZAM's types under -fpack-struct=4, and the library compiles without it, under
        # GCC_SH4ZAM_FLAGS. So each SH4ZAM type must lay out alike under both. njSetView and njTranslateLight hand a
        # view's and a light's matrix to SH4ZAM as a shz_mat4x4_t, which lies on 8. So each structure lies on 8, and
        # keeps the layout SHC's text gives it. SH4ZAM's headers include no threads.h, which the SDK lacks
        if not (ROOT / 'tools/sh4zam/fetched.txt').is_file():
            self.skipTest('tools/sh4zam lacks the fetch make dc makes')

        def laid(compiled):
            run('bin/sh-elf-gcc', *compiled, '-Isdk/include', '-c', '-o', str(self.root / 'layout.o'),
                str(self.root / 'layout.c'), root=ROOT)
            found = elf.read((self.root / 'layout.o').read_bytes(), 'layout.o')
            return {symbol.name[1:]: struct.unpack_from(f'<{symbol.size // 4}I', found.sections[symbol.section].data,
                                                        symbol.value)
                    for symbol in found.symbols if symbol.name in ('_zam_layout', '_ninja_layout', '_ninja_alignment')}

        (self.root / 'layout.c').write_text(LAYOUT)
        compiled = flags()
        game, shc, library = laid(compiled), laid([*compiled, '-UNONMATCHING']), laid(flags('GCC_SH4ZAM_FLAGS'))
        self.assertEqual(game['zam_layout'], library['zam_layout'])
        self.assertEqual(game['ninja_layout'], shc['ninja_layout'])
        self.assertEqual(game['ninja_alignment'], (8, 8))

    def test_links_from_its_command_line(self):
        done = subprocess.run([sys.executable, str(ROOT / 'tools/ld.py'), 'link.txt', str(self.out)],
                              env=dict(os.environ, SH_ELF=str(TOOLCHAIN)), stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True)
        self.assertEqual((done.returncode, done.stdout), (0, ''))
        # each image's imports.ld assigns its imports alone, each beside a comment that names the definition it takes
        for name, image in self.images.items():
            assigned = (self.out / name / 'imports.ld').read_text()
            self.assertEqual({line.split()[0] for line in assigned.splitlines() if ' = ' in line},
                             {f'_{alias}' for alias in image.imports})
            self.assertTrue((self.out / name / 'final.elf').is_file())
        self.assertRegex((self.out / 'main/imports.ld').read_text(),
                         r"\n_ova_step = 0x8c9000[0-9a-f]{2}; /\* ova's step \*/\n")

    def test_a_provider_that_moves_takes_its_consumers_with_it(self):
        # ova's object changes between two runs. ova_moved.o's constant lies ahead of its data, so ova_table moves on,
        # and main's binding and word follow it
        before = ld.coordinate(self.images, self.out)[0]['main']
        bindings, _, finals = ld.coordinate(linking(self.images, 'ova', 'ova_moved.o'), self.out)
        table = gcclink.exports(finals['ova'].elf)['ova_table'].value
        self.assertGreater(table, before['ova_table'])
        self.assertEqual(bindings['main']['ova_table'], table)
        self.assertEqual(words(finals['main'], 'main_slot', 1), [table + 12])

    def test_a_provider_definition_moved_after_its_layout_pass_fails_the_layout_check(self):
        # ova's final pass links ova_moved.o, whose constant moves ova_table on
        layouts = self.layouts(self.images)
        values = ld.resolve(self.images, layouts)
        finals = self.finals(linking(self.images, 'ova', 'ova_moved.o'), values)
        problems = ld.moved(layouts, finals)
        self.assertTrue(problems)
        self.assertTrue(all(problem.startswith('ova: ') for problem in problems), problems)
        self.assertTrue(any(problem.startswith('ova: ova_table is 0x0c9000') for problem in problems), problems)

    def test_a_final_layout_moved_fails_the_layout_check(self):
        # main's final pass links late.o too, whose datum runs its bss on
        layouts = self.layouts(self.images)
        values = ld.resolve(self.images, layouts)
        finals = self.finals(linking(self.images, 'main', 'main.o', 'late.o'), values)
        problems = ld.moved(layouts, finals)
        for item in ('.bss is ', 'image_end is ', 'main_late is ', "late.o's "):
            self.assertTrue(any(problem.startswith(f'main: {item}') for problem in problems), (item, problems))
        self.assertTrue(all(problem.startswith('main: ') for problem in problems), problems)

    def test_an_input_linked_twice_whose_first_placement_moves_fails_the_layout_check(self):
        # ova links record.o twice, which its static record allows, so the map places record.o's .data twice. The
        # test moves the final pass's first placement on
        images = linking(self.images, 'ova', 'ova.o', 'record.o', 'record.o')
        layouts = self.layouts(images)
        finals = self.finals(images, ld.resolve(images, layouts))
        ova = finals['ova']
        first = next(placed for placed in ova.placements if (placed.file, placed.section) == ('record.o', '.data'))
        moved = ova._replace(placements=[placed._replace(address=placed.address + 4) if placed is first else placed
                                         for placed in ova.placements])
        problems = ld.moved(layouts, {**finals, 'ova': moved})
        self.assertEqual(len(problems), 1, problems)
        self.assertTrue(problems[0].startswith("ova: record.o's .data is "), problems)

    def test_an_output_section_whose_flags_or_kind_change_fails_the_layout_check(self):
        # main's final pass with .data executable too, or with .bss holding bytes, not zeroed. Each still starts and
        # ends where the layout pass put it
        layouts = self.layouts(self.images)
        finals = self.finals(self.images, ld.resolve(self.images, layouts))
        main = finals['main']
        for name, change in (('.data', {'flags': 0x7}), ('.bss', {'kind': 1})):
            with self.subTest(name):
                changed = main._replace(elf=main.elf._replace(sections=[
                    section._replace(**change) if section.name == name else section for section in main.elf.sections]))
                problems = ld.moved(layouts, {**finals, 'main': changed})
                self.assertEqual(len(problems), 1, problems)
                self.assertTrue(problems[0].startswith(f'main: {name} is '), problems)

    def test_a_root_pulls_in_the_archive_member_that_defines_it(self):
        # without its root, ovb's link takes no member of libovb.a, so resolve finds no ovb_helper. ld collects no
        # section, so ova keeps ova_probe without its root
        roots = gcclink.rooted(self.images)
        roots['ova'].remove('ova_probe')
        roots['ovb'].remove('ovb_helper')
        layouts = self.layouts(self.images, roots)
        self.assertFalse([item for item in ld.signature(layouts['ovb']) if item.startswith('libovb.a(')])
        self.assertIn('ova_probe', gcclink.exports(layouts['ova'].elf))
        with self.assertRaisesRegex(SystemExit, r"main: ovb_helper takes ovb's ovb_helper, which \S*ovb/layout\.elf "
                                                "doesn't define$"):
            ld.resolve(self.images, layouts)

    def test_a_name_its_provider_only_imports_fails_resolve(self):
        # ova takes main_counter from main, so ova's value for it is no export of ova's that ovb can take
        images = importing(self.images, 'ovb', main_counter=gcclink.Import('ova', 'main_counter', '0x8c'))
        layouts = self.layouts(images)
        with self.assertRaisesRegex(SystemExit, r"ovb: main_counter takes ova's main_counter, which \S*ova/layout\.elf "
                                                "doesn't define: it imports it too$"):
            ld.resolve(images, layouts)

    def test_takes_no_boundary_of_a_link_as_an_export(self):
        # ova's script defines image_start, and _startof_P at the start of .text's P run. Each is a boundary of its
        # link, which no import takes
        images = importing(self.images, 'main', ova_start=gcclink.Import('ova', 'image_start', '0x0c'),
                           ova_code=gcclink.Import('ova', '_startof_P', '0x0c'))
        layouts = self.layouts(images)
        boundaries = {'image_start', 'image_end', '_startof_P', '_endof_P', '_sizeof_P'}
        self.assertEqual(boundaries & ld.signature(layouts['ova']).keys(), boundaries)
        self.assertFalse(boundaries & gcclink.exports(layouts['ova'].elf).keys())
        with self.assertRaisesRegex(SystemExit, r"main: ova_start takes ova's image_start, which \S*ova/layout\.elf "
                                                r"doesn't define: it is a boundary of its link, not an export\n"
                                                r".*main: ova_code takes ova's _startof_P, which \S*ova/layout\.elf "
                                                "doesn't define: it is a boundary of its link, not an export$"):
            ld.resolve(images, layouts)

    def test_bounds_each_shc_section_s_run_from_the_final_layout(self):
        # main_bounds holds B's start, end and size. B's run spans .bss: GCC's bss, and bounds.o's 8 bytes of B. Its
        # bounds are boundaries of the link, which the signature holds and no export does
        images = linking(self.images, 'main', 'main.o', 'bounds.o')
        bindings, layouts, finals = ld.coordinate(images, self.out)
        bss = next(section for section in finals['main'].elf.sections if section.name == '.bss')
        self.assertEqual(words(finals['main'], 'main_bounds', 3), [bss.address, bss.address + bss.size, bss.size])
        self.assertEqual(ld.signature(finals['main'])['_startof_B'], f'{hexa(bss.address)}, 0 bytes of .bss')
        self.assertFalse({'_startof_B', '_endof_B', '_sizeof_B'} & gcclink.exports(finals['main'].elf).keys())
        # more data moves .bss on, and more bss ends it later. Each bound follows the final layout, and main_bounds'
        # words follow it
        for name, inputs in (('data', ('main.o', 'more.o', 'bounds.o')), ('bss', ('main.o', 'late.o', 'bounds.o'))):
            with self.subTest(name):
                moved = ld.coordinate(linking(self.images, 'main', *inputs), self.out)[2]['main']
                after = next(section for section in moved.elf.sections if section.name == '.bss')
                self.assertNotEqual((after.address, after.size), (bss.address, bss.size))
                self.assertEqual(words(moved, 'main_bounds', 3), [after.address, after.address + after.size,
                                                                  after.size])

    def test_gives_a_bounded_section_other_than_its_kind_s_letter_a_run_of_its_own_after_it(self):
        # bounds32.o bounds B32, so B32 takes a run of its own. That run starts where B's ends, and holds bounds32.o's
        # 4 bytes. B's run takes GCC's bss and bounds.o's B
        images = linking(self.images, 'main', 'main.o', 'bounds.o', 'bounds32.o')
        _, _, finals = ld.coordinate(images, self.out)
        end = words(finals['main'], 'main_bounds', 3)[1]
        self.assertEqual(words(finals['main'], 'main_bounds32', 2), [end, end + 4])

    def test_starts_a_run_of_its_own_where_its_first_section_lies(self):
        # aligned.o's D32 takes a run of its own after D's, which ends off 32. The run starts on 32, where D32 lies,
        # and holds nothing
        _, _, finals = ld.coordinate(linking(self.images, 'main', 'main.o', 'aligned.o'), self.out)
        start, end, size, after = words(finals['main'], 'main_aligned', 4)
        placed = next(each for each in finals['main'].placements if (each.file, each.section) == ('aligned.o', 'D32'))
        self.assertNotEqual(after % 32, 0)
        self.assertEqual([start, end, size], [placed.address, placed.address, 0])

    def test_starts_a_tail_run_where_its_first_section_lies(self):
        # bounds.o's DEND, a word, opens main's tail, and aligned.o's D32 follows it in a tail run of its own. The run
        # starts on 32, where D32 lies, and holds nothing
        images = linking(self.images, 'main', 'main.o', 'bounds.o', 'aligned.o')
        images = {**images, 'main': images['main']._replace(tails=[gcclink.Piece('DEND', 'bounds.o', 'DEND'),
                                                                   gcclink.Piece('D32', 'aligned.o', 'D32')])}
        _, _, finals = ld.coordinate(images, self.out)
        placed = next(each for each in finals['main'].placements if (each.file, each.section) == ('aligned.o', 'D32'))
        self.assertNotEqual(words(finals['main'], 'main_end', 1)[0] % 32, 0)
        self.assertEqual(words(finals['main'], 'main_aligned', 3), [placed.address, placed.address, 0])

    def test_places_a_datum_of_a_section_of_its_own_in_bss(self):
        # spare.o's .bss.main_spare, a section GCC names after its datum
        _, _, finals = ld.coordinate(linking(self.images, 'main', 'main.o', 'spare.o'), self.out)
        main = finals['main'].elf
        spare = gcclink.exports(main)['main_spare']
        self.assertEqual(main.sections[spare.section].name, '.bss')

    def test_opens_the_image_with_its_head_rows(self):
        # record.o's .data, ova's head record, lies at ova's origin, ahead of head.o's function and the rest of ova's
        # code
        images = linking(self.images, 'ova', 'ova.o', 'record.o')
        images = {**images, 'ova': images['ova']._replace(heads=[gcclink.Piece('DHEAD', 'record.o', '.data')])}
        _, _, finals = ld.coordinate(images, self.out)
        ova = finals['ova'].elf
        self.assertEqual([section.name for section in ova.sections if section.flags & elf.ALLOC][:2],
                         ['.head', '.text'])
        record = next(symbol for symbol in ova.symbols if symbol.name == '_ova_record')
        self.assertEqual((record.value, record.size), (images['ova'].origin, 8))
        self.assertEqual(ld.signature(finals['ova'])['image_start'].split(',')[0], hexa(images['ova'].origin))

    def test_a_head_run_takes_no_bounds_where_another_run_takes_its_name(self):
        # entry.o's P opens ova, as MOVIE.BIN's entry opens it, and mixed.o's P lies in .text's P run, which holds
        # P's bounds
        images = linking(self.images, 'ova', 'ova.o', 'entry.o', 'mixed.o')
        images = {**images, 'ova': images['ova']._replace(heads=[gcclink.Piece('P', 'entry.o', 'P')])}
        _, _, finals = ld.coordinate(images, self.out)
        self.assertEqual(gcclink.exports(finals['ova'].elf)['ova_entry'].value, images['ova'].origin)
        text = next(section for section in finals['ova'].elf.sections if section.name == '.text')
        self.assertEqual(ld.signature(finals['ova'])['_startof_P'], f'{hexa(text.address)}, 0 bytes of .text')
        script = (self.out / 'ova/final.ld').read_text()
        head = script[script.index('  .head :'):script.index('  .text :')]
        self.assertIn('"entry.[o]"(P)', head)
        self.assertNotIn('__startof_P', head)

    def test_keeps_an_object_s_code_together_file_by_file(self):
        # mixed.o's P, which branches to its own C, lies beside that C, in the order GCC writes the two, ahead of
        # ova.o's code. .text's first run takes each SHC code section that no input bounds, file by file
        images = linking(self.images, 'ova', 'mixed.o', 'ova.o')
        _, layouts, _ = ld.coordinate(images, self.out)
        placed = [(placed.file, placed.section) for placed in layouts['ova'].placements
                  if placed.output == '.text' and placed.size]
        self.assertEqual((placed[0], sorted(placed[1:3]), placed[3]),
                         (('head.o', '.text.head'), [('mixed.o', '.text.mixed_twice'), ('mixed.o', 'P')],
                          ('ova.o', '.text.step')))

    def test_places_dwarf_s_sections_where_they_load_nowhere(self):
        # the script puts debug.o's DWARF sections at 0, and its datum in .data. The sections keep their bytes and
        # load nowhere, and the image defines the datum. The signature holds allocated sections alone, so none of
        # the DWARF
        _, _, finals = ld.coordinate(linking(self.images, 'main', 'main.o', 'debug.o'), self.out)
        sections = {section.name: section for section in finals['main'].elf.sections}
        self.assertTrue(sections['.debug_info'].size)
        self.assertFalse(sections['.debug_info'].flags & elf.ALLOC)
        self.assertIn('debug_value', gcclink.exports(finals['main'].elf))
        self.assertFalse([item for item in ld.signature(finals['main']) if '.debug_' in item])
        # ranges.o's .debug_ranges, which DEBUG doesn't list, fails as a section the script doesn't place
        with self.assertRaisesRegex(SystemExit, "main: error: unplaced orphan section `.debug_ranges' from "
                                                "`ranges.o'\n"):
            self.layouts(linking(self.images, 'main', 'main.o', 'ranges.o'))

    def test_a_head_or_tail_row_that_names_no_section_of_an_object_fails_the_link(self):
        for piece, problem in ((gcclink.Piece('DHEAD', 'record.o', '.rodata'),
                                "ova: its DHEAD row places record.o's .rodata, and that object holds no such section"),
                               (gcclink.Piece('DHEAD', 'libovb.a', '.data'),
                                "ova: its DHEAD row places libovb.a's .data, and that is no object of its inputs")):
            with self.subTest(piece):
                images = linking(self.images, 'ova', 'ova.o', 'record.o', 'libovb.a')
                images = {**images, 'ova': images['ova']._replace(heads=[piece])}
                with self.assertRaisesRegex(SystemExit, f'{problem}$'):
                    self.layouts(images)

    def test_a_head_row_that_splits_its_run_or_places_an_input_section_twice_fails_the_link(self):
        rows = {'a run apart': ([gcclink.Piece('DHEAD', 'record.o', '.data'), gcclink.Piece('DMORE', 'more.o', '.data'),
                                 gcclink.Piece('DHEAD', 'debug.o', '.data')],
                                'ova: its rows split DHEAD into runs apart'),
                'twice': ([gcclink.Piece('DHEAD', 'record.o', '.data'), gcclink.Piece('DHEAD', 'record.o', '.data')],
                          "ova: its rows place record.o's .data twice")}
        for case, (heads, problem) in rows.items():
            with self.subTest(case):
                images = linking(self.images, 'ova', 'ova.o', 'record.o', 'more.o', 'debug.o')
                images = {**images, 'ova': images['ova']._replace(heads=heads)}
                with self.assertRaisesRegex(SystemExit, f'{problem}$'):
                    self.layouts(images)

    def test_closes_the_image_with_its_tail_rows(self):
        # more.o's .data and then bounds.o's DEND follow .bss. main_end, DEND's word, holds DEND's end, which is the
        # image's end. .data, ahead of them, takes neither section
        images = linking(self.images, 'main', 'main.o', 'more.o', 'bounds.o')
        images = {**images, 'main': images['main']._replace(tails=[gcclink.Piece('DMORE', 'more.o', '.data'),
                                                                   gcclink.Piece('DEND', 'bounds.o', 'DEND')])}
        _, _, finals = ld.coordinate(images, self.out)
        main = finals['main'].elf
        allocated = [section.name for section in main.sections if section.flags & elf.ALLOC]
        self.assertEqual(allocated[-2:], ['.bss', '.tail'])
        tail = next(section for section in main.sections if section.name == '.tail')
        more = gcclink.exports(main)['main_more']
        self.assertEqual((more.value, more.size), (tail.address, 64))
        end = next(symbol for symbol in main.symbols if symbol.name == '_image_end').value
        self.assertEqual(words(finals['main'], 'main_end', 1), [end])

    def test_a_tail_row_leaves_its_object_s_other_sections_where_they_lie(self):
        # sibling.o's .data.last closes main, and its .data.keep, which .data.* takes too, lies in .data, between
        # main.o's data and more.o's, in link order
        images = linking(self.images, 'main', 'main.o', 'sibling.o', 'more.o')
        images = {**images, 'main': images['main']._replace(tails=[gcclink.Piece('DLAST', 'sibling.o', '.data.last')])}
        main = ld.coordinate(images, self.out)[2]['main']
        own = gcclink.exports(main.elf)
        self.assertEqual([main.elf.sections[own[name].section].name for name in ('main_keep', 'main_last')],
                         ['.data', '.tail'])
        self.assertEqual([(placed.file, placed.section) for placed in main.placements
                          if placed.output == '.data' and placed.size],
                         [('main.o', '.data'), ('sibling.o', '.data.keep'), ('more.o', '.data')])

    def test_opens_the_code_with_its_kernel_rows_in_row_order_and_keeps_the_rest_in_link_order(self):
        # ova_probe, then head.o's function, then the rest of the code file by file. ova.o's data still follows
        # more.o's
        images = linking(self.images, 'ova', 'more.o', 'ova.o')
        kernel = [gcclink.Piece('draw', 'ova.o', '.text.ova_probe'), gcclink.Piece('sweep', 'head.o', '.text.head')]
        _, layouts, _ = ld.coordinate({**images, 'ova': images['ova']._replace(kernel=kernel)}, self.out)
        placed = {output: [(placed.file, placed.section) for placed in layouts['ova'].placements
                           if placed.output == output and placed.size] for output in ('.text', '.data')}
        self.assertEqual(placed['.text'], [('ova.o', '.text.ova_probe'), ('head.o', '.text.head'),
                                           ('ova.o', '.text.step'), ('ova.o', '.text.ova_unused')])
        self.assertEqual(placed['.data'], [('more.o', '.data'), ('ova.o', '.data')])

    def test_a_kernel_past_the_instruction_cache_s_lines_fails_the_link(self):
        # ova's code starts at its origin, on a line's start, so whole.o's 8 KB take each line once, and spill.o's
        # two bytes one more. As a kernel of its own, spill.o's take one line, over whole.o's first
        images = linking(self.images, 'ova', 'ova.o', 'whole.o', 'spill.o')
        kernel = [gcclink.Piece('draw', 'whole.o', '.text.whole'), gcclink.Piece('sweep', 'spill.o', '.text.spill')]
        ld.coordinate({**images, 'ova': images['ova']._replace(kernel=kernel, over=[('sweep', 'draw')])}, self.out)
        kernel[1] = kernel[1]._replace(group='draw')
        with self.assertRaisesRegex(SystemExit, "ova: its draw kernel touches 257 lines, more than the 256 of the "
                                                "SH-4's instruction cache\n"):
            self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel)})
        self.assertFalse((self.out / 'ova/layout.elf').exists())

    def test_two_kernels_whose_lines_take_one_set_fail_the_link_but_where_an_over_row_names_them(self):
        # spill.o's line, past whole.o's 8 KB, takes the set of whole.o's first. Kernels that share a line, as
        # test_opens_the_code_with_its_kernel_rows_in_row_order_and_keeps_the_rest_in_link_order's do, pass
        images = linking(self.images, 'ova', 'ova.o', 'whole.o', 'spill.o')
        kernel = [gcclink.Piece('draw', 'whole.o', '.text.whole'), gcclink.Piece('sweep', 'spill.o', '.text.spill')]
        with self.assertRaisesRegex(SystemExit, "ova: a line of its draw kernel and another of its sweep kernel take "
                                                "set 0 of the SH-4's instruction cache, and no over row names the "
                                                'two\n'):
            self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel)})
        self.assertFalse((self.out / 'ova/layout.elf').exists())
        self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel, over=[('draw', 'sweep')])})

    def test_kernels_of_images_where_one_imports_from_the_other_take_one_set_only_where_an_over_row_names_them(self):
        # each kernel opens its image's code, on set 0. ova and ovb import nothing from each other, so their kernels
        # may take one set
        kernels = {'main': gcclink.Piece('draw', 'main.o', '.text.main_call'),
                   'ova': gcclink.Piece('pieces', 'ova.o', '.text.step'),
                   'ovb': gcclink.Piece('pieces', 'ovb.o', '.text.step')}
        images = {name: image._replace(kernel=[kernels[name]]) for name, image in self.images.items()}
        with self.assertRaisesRegex(SystemExit, "main: a line of its draw kernel and another of ova's pieces kernel "
                                                "take set 0 of the SH-4's instruction cache, and no over row names "
                                                "the two\n.*: main: a line of its draw kernel and another of ovb's "
                                                "pieces kernel take set 0 of the SH-4's instruction cache, and no "
                                                'over row names the two$'):
            ld.coordinate(images, self.out)
        self.assertEqual(list(self.out.glob('*/final.elf')), [])
        # main's row names ova's pieces, and so not ovb's
        images['main'] = images['main']._replace(over=[('draw', 'pieces', 'ova')])
        with self.assertRaisesRegex(SystemExit, "^[^\n]*: main: a line of its draw kernel and another of ovb's pieces "
                                                "kernel take set 0 of the SH-4's instruction cache, and no over row "
                                                'names the two$'):
            ld.coordinate(images, self.out)
        images['ovb'] = images['ovb']._replace(over=[('pieces', 'draw', 'main')])
        layouts = ld.coordinate(images, self.out)[1]
        # either image's imports from the other hold their kernels to each other's
        bare = {name: image._replace(over=[]) for name, image in images.items()}
        for alone in ({'main': bare['main']._replace(imports={})},
                      {name: bare[name]._replace(imports={}) for name in ('ova', 'ovb')}):
            self.assertEqual(len(ld.across({**bare, **alone}, layouts)), 2)
        images['ova'] = images['ova']._replace(over=[('pieces', 'clip', 'main')])
        with self.assertRaisesRegex(SystemExit, "ova: its over row names main's clip kernel, which no kernel row of "
                                                'main names$'):
            ld.coordinate(images, self.out)

    def test_a_line_of_a_kernel_s_pool_on_the_set_of_a_line_its_pool_names_fails_the_link(self):
        # pool_read's pool lies on ova's first line, in set 0 of the operand cache, and so does pool_word, which lies in
        # set 512 of a cache twice the size. pool_far lies in set 256, set 0 of a cache half the size. The pool's word
        # that names its own code, in set 0 too, names no line. The link checks the pool only where a kernel row places
        # it, and pad.o's line ahead of it moves the pool to set 1. Each link here takes no pad
        images = linking(self.images, 'ova', 'ova.o', 'pool.o', 'pad.o')
        data = [gcclink.Piece('data', 'ova.o', '.data'), gcclink.Piece('data', 'pool.o', '.data.pool')]
        self.layouts({**images, 'ova': images['ova']._replace(data=data)}, pads={})
        kernel = [gcclink.Piece('draw', 'pool.o', '.text.pool')]
        with self.assertRaisesRegex(SystemExit, "^.*: ova: pool_read's pool line at 0x0c900000 and pool_word's line at "
                                                "0x0c90c000, which pool_read's pool names, take set 0 of the SH-4's "
                                                "operand cache, and the draw kernel runs with itself\n"
                                                '.*: ova: its layout pass failed$'):
            self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel, data=data)}, pads={})
        self.assertFalse((self.out / 'ova/layout.elf').exists())
        kernel.insert(0, gcclink.Piece('draw', 'pad.o', '.text.pad'))
        self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel, data=data)}, pads={})
        # so does a word that names .bss
        images = linking(self.images, 'ovb', 'ovb.o', 'libovb.a', 'pool_bss.o')
        kernel = [gcclink.Piece('draw', 'pool_bss.o', '.text.pool_bss')]
        data = [gcclink.Piece('data', 'pool_bss.o', '.bss.pool')]
        with self.assertRaisesRegex(SystemExit, "^.*: ovb: pool_bss_read's pool line at 0x0c900000 and pool_zero's "
                                                "line at 0x0c904000, which pool_bss_read's pool names, take set 0 of "
                                                "the SH-4's operand cache, and the draw kernel runs with itself\n"
                                                ".*: ovb: its layout pass failed$"):
            self.layouts({**images, 'ovb': images['ovb']._replace(kernel=kernel, data=data)}, pads={})
        # and a datum no data row places isn't checked
        self.layouts({**images, 'ovb': images['ovb']._replace(kernel=kernel)})

    def test_a_pool_line_on_the_set_of_a_line_its_pool_names_moves_the_data_row_s_section_on_a_line(self):
        # row_read's pool line and row_datum, which opens .bss on the operand cache's 16 KB, take set 0. Both passes
        # move .bss.row on a line, to set 1, and the layout check holds
        images = linking(self.images, 'ova', 'row.o', 'ova.o')
        images = {**images, 'ova': images['ova']._replace(
            kernel=[gcclink.Piece('draw', 'row.o', '.text.row')], windows=[gcclink.Window('draw', 1, 0)],
            data=[gcclink.Piece('data', 'row.o', '.bss.row')])}
        _, layouts, finals = ld.coordinate(images, self.out)
        for linked in (layouts['ova'], finals['ova']):
            datum = gcclink.exports(linked.elf)['row_datum'].value
            self.assertEqual(datum % (ld.OCACHE_LINES * ld.OCACHE_LINE), ld.OCACHE_LINE)
        with self.assertRaisesRegex(SystemExit, "^.*: ova: row_read's pool line at 0x0c900000 and row_datum's line at "
                                                "0x0c90.000, which row_read's pool names, take set 0 of the SH-4's "
                                                "operand cache, and the draw kernel runs with itself\n"
                                                '.*: ova: its layout pass failed$'):
            self.layouts(images, pads={})

    def test_a_pool_line_on_the_set_of_a_line_its_pool_names_in_the_last_data_moves_them_on_a_line(self):
        # last_read's pool line and last_datum, which closes .data a word past a 16 KB line, take set 0. Both passes
        # move the last data on a line, to set 1, and the layout check holds
        images = linking(self.images, 'ova', 'ova.o', 'last_row.o')
        kernel = [gcclink.Piece('draw', 'last_row.o', '.text.last_row')]
        images = {**images, 'ova': images['ova']._replace(kernel=kernel)}
        _, layouts, finals = ld.coordinate(images, self.out)
        for linked in (layouts['ova'], finals['ova']):
            datum = gcclink.exports(linked.elf)['last_datum'].value
            self.assertEqual(datum % (ld.OCACHE_LINES * ld.OCACHE_LINE), ld.OCACHE_LINE + 4)
        with self.assertRaisesRegex(SystemExit, "^.*: ova: last_read's pool line at 0x0c900000 and last_datum's line "
                                                "at 0x0c90.000, which last_read's pool names, take set 0 of the SH-4's "
                                                "operand cache, and the draw kernel runs with itself\n"
                                                '.*: ova: its layout pass failed$'):
            self.layouts(images, pads={})

    def test_a_pad_that_alignment_absorbs_or_that_moves_a_later_output_section_fails_the_link_on_the_pair_left(self):
        # fit() predicts each pad's move from the pass without pads, in whole lines, and within the pad's output section
        # alone. absorb_datum's pad of a line lands in the 60 bytes its alignment skips, so it stays on set 2.
        # shift_const's pad of a line moves .data after it, and shift_last from set 1, which no pool line takes, to set
        # 2. The padded pass's check fails each link on the pair the pads leave
        absorbed = [gcclink.Piece('data', 'absorb.o', name) for name in ('.bss.absorb_lead', '.bss.absorb')]
        cases = {'absorb': ('absorb.o', absorbed, 'absorb_read', 'absorb_datum'),
                 'shift': ('shift.o', [gcclink.Piece('data', 'shift.o', '.rodata.shift')], 'shift_read', 'shift_last')}
        for case, (path, data, function, name) in cases.items():
            with self.subTest(case):
                images = linking(self.images, 'ova', path)
                images = {**images, 'ova': images['ova']._replace(
                    kernel=[gcclink.Piece('draw', path, f'.text.{case}')], windows=[gcclink.Window('draw', 4, 0)],
                    data=data)}
                with self.assertRaisesRegex(SystemExit, f"^.*: ova: {function}'s pool line at 0x0c900040 and "
                                                        f"{name}'s line at 0x0c90.040, which {function}'s pool names, "
                                                        "take set 2 of the SH-4's operand cache, and the draw kernel "
                                                        "runs with itself\n.*: ova: its layout pass failed$"):
                    self.layouts(images)
                self.assertIn('. += 0x20;', (self.out / 'ova/layout.ld').read_text())
                self.assertFalse((self.out / 'ova/layout.elf').exists())

    def test_a_pool_line_that_holds_only_a_constant_counts_and_a_word_that_names_code_names_no_line(self):
        # sched's first pool line, on set 0, holds the constant alone, and sched_on.o puts the datum its second line's
        # word names on set 0. sched_off.o puts it on set 2, and the word naming sched, on set 0, names no line
        kernel = [gcclink.Piece('draw', 'sched.o', '.text.sched')]
        data = [gcclink.Piece('data', 'ova.o', '.data'), gcclink.Piece('data', 'sched_on.o', '.data.sched')]
        images = linking(self.images, 'ova', 'ova.o', 'sched.o', 'sched_on.o')
        with self.assertRaisesRegex(SystemExit, "^.*: ova: sched's pool line at 0x0c900000 and sched_datum's line at "
                                                "0x0c90c000, which sched's pool names, take set 0 of the SH-4's "
                                                "operand cache, and the draw kernel runs with itself\n"
                                                '.*: ova: its layout pass failed$'):
            self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel, data=data)}, pads={})
        images = linking(self.images, 'ova', 'ova.o', 'sched.o', 'sched_off.o')
        data = [gcclink.Piece('data', 'ova.o', '.data'), gcclink.Piece('data', 'sched_off.o', '.data.sched')]
        self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel, data=data)}, pads={})

    def test_a_word_names_the_lines_of_the_next_64_bytes_within_its_object_or_its_own_line_of_an_object_over_256(self):
        # reach_datum starts on set 511, 8 bytes before its line's end. 16 bytes of it run onto set 0, where reach's
        # pool lies. 8 bytes end with the line, and 512 bytes, though they run on, take the word's line alone
        kernel = [gcclink.Piece('draw', 'reach.o', '.text.reach')]
        cases = ((16, "^.*: ova: reach's pool line at 0x0c900000 and reach_datum's line at 0x0c90c000, which reach's "
                      "pool names, take set 0 of the SH-4's operand cache, and the draw kernel runs with itself\n"
                      '.*: ova: its layout pass failed$'), (8, None), (512, None))
        for size, problem in cases:
            with self.subTest(size):
                images = linking(self.images, 'ova', 'ova.o', 'reach.o', f'reach_{size}.o')
                data = [gcclink.Piece('data', 'ova.o', '.data'),
                        gcclink.Piece('data', f'reach_{size}.o', '.data.reach')]
                images = {**images, 'ova': images['ova']._replace(kernel=kernel, data=data)}
                if problem is None:
                    self.layouts(images, pads={})
                    continue
                with self.assertRaisesRegex(SystemExit, problem):
                    self.layouts(images, pads={})

    def test_a_pool_line_on_the_set_of_a_line_another_kernel_s_pool_names_fails_but_where_an_over_row_names_them(self):
        # pair_a's pool line takes set 0, where pair_datum lies, which pair_b's pool names
        images = linking(self.images, 'ova', 'ova.o', 'pair.o')
        kernel = [gcclink.Piece('draw', 'pair.o', '.text.pair_a'), gcclink.Piece('sweep', 'pair.o', '.text.pair_b')]
        data = [gcclink.Piece('data', 'ova.o', '.data'), gcclink.Piece('data', 'pair.o', '.data.pair')]
        with self.assertRaisesRegex(SystemExit, "^.*: ova: pair_a's pool line at 0x0c900000 and pair_datum's line at "
                                                "0x0c90c000, which pair_b's pool names, take set 0 of the SH-4's "
                                                "operand cache, and no over row names draw and sweep\n"
                                                '.*: ova: its layout pass failed$'):
            self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel, data=data)}, pads={})
        self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel, data=data, over=[('sweep', 'draw')])},
                     pads={})

    def test_a_pool_s_targets_are_its_loads_but_of_a_word_it_found_already_or_past_its_section(self):
        # MOV.W at 0 loads 0x20, MOVA at 2 takes 0x40 and MOV.L at 4 loads 0x60, whose word decodes as a MOV.L of 0x68.
        # MOV.L at 6 loads past the section
        code = struct.pack('<6H', 0x910E, 0xC70F, 0xD216, 0xD3FF, 0x000B, 0x0009).ljust(0x60, b'\0')
        code = (code + struct.pack('<H', 0xD001)).ljust(0x80, b'\0')
        text = elf.Section(1, '.text', 1, elf.ALLOC, 0x0C900000, 0x80, 4, code)
        self.assertEqual(ld.pool_targets(text, 0x0C900000, 0x0C900080), [0x0C900020, 0x0C900040, 0x0C900060])

    def test_lays_each_kernel_in_its_window_so_code_that_grows_within_it_moves_nothing_after(self):
        # draw and sweep take two lines each, from ova's origin. small.o's 8 bytes and large.o's 40 leave step where it
        # was. .rodata and .bss start on the operand cache's 16 KB
        windows = [gcclink.Window('draw', 2, 0), gcclink.Window('sweep', 2, 0)]
        placed = []
        for code in ('small', 'large'):
            images = linking(self.images, 'ova', f'{code}.o', 'ova_moved.o', 'late.o')
            kernel = [gcclink.Piece('draw', f'{code}.o', f'.text.{code}'),
                      gcclink.Piece('sweep', 'ova_moved.o', '.text.step')]
            layouts = self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel, windows=windows)})
            found = {each.section: each.address for each in layouts['ova'].placements}
            placed.append(found['.text.step'])
            sections = {each.name: each.address for each in layouts['ova'].elf.sections}
            self.assertEqual((sections['.rodata'] % 16384, sections['.bss'] % 16384), (0, 0))
        self.assertEqual(placed, [0x0C900040, 0x0C900040])
        # an offset of a line moves small.o's code a line on, and step not
        images = linking(self.images, 'ova', 'small.o', 'ova.o')
        kernel = [gcclink.Piece('draw', 'small.o', '.text.small'), gcclink.Piece('sweep', 'ova.o', '.text.step')]
        windows[0] = gcclink.Window('draw', 2, 1)
        layouts = self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel, windows=windows)})
        self.assertEqual([each.address for each in layouts['ova'].placements
                          if each.section in ('.text.small', '.text.step')], [0x0C900020, 0x0C900040])

    def test_puts_each_warm_row_s_section_after_the_windows_in_row_order_where_other_code_moves_it_not(self):
        # draw's window takes two lines from ova's origin. ova_probe and then step follow it, in row order though step
        # links first, and large.o's 40 bytes linked ahead of ova.o move ova_unused, not them
        kernel, windows = [gcclink.Piece('draw', 'small.o', '.text.small')], [gcclink.Window('draw', 2, 0)]
        warm = [gcclink.Piece('warm', 'ova.o', '.text.ova_probe'), gcclink.Piece('warm', 'ova.o', '.text.step')]
        placed = []
        for objects in (('small.o', 'ova.o'), ('small.o', 'large.o', 'ova.o')):
            images = linking(self.images, 'ova', *objects)
            layout = self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel, windows=windows, warm=warm)})
            placed.append({each.section: each.address for each in layout['ova'].placements if each.file == 'ova.o'})
        alone, after = placed
        self.assertEqual((alone['.text.ova_probe'], alone['.text.step'] > alone['.text.ova_probe']), (0x0C900040, True))
        self.assertEqual(({name: after[name] for name in ('.text.ova_probe', '.text.step')},
                          after['.text.ova_unused'] > alone['.text.ova_unused']),
                         ({name: alone[name] for name in ('.text.ova_probe', '.text.step')}, True))

    def test_a_warm_row_that_places_no_code_part_of_an_object_s_split_code_or_a_kernel_s_section_fails_the_link(self):
        rows = {'data': ([], [gcclink.Piece('warm', 'more.o', '.data')],
                         "ova: its warm row places more.o's .data, which is no code"),
                # mixed.o's P branches to its C, which the warm rows and the kernels leave
                'split': ([], [gcclink.Piece('warm', 'mixed.o', 'P')],
                          "ova: its kernel and warm rows take some of mixed.o's code, which is SHC's sections and "
                          "GCC's both, and leave .text.mixed_twice"),
                'kernel': ([gcclink.Piece('draw', 'ova.o', '.text.step')],
                           [gcclink.Piece('warm', 'ova.o', '.text.step')], "ova: its rows place ova.o's .text.step twice")}
        for case, (kernel, warm, problem) in rows.items():
            with self.subTest(case):
                images = linking(self.images, 'ova', 'ova.o', 'more.o', 'mixed.o')
                with self.assertRaisesRegex(SystemExit, f'{problem}$'):
                    self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel, warm=warm)})

    def searched(self):
        """ova with a kernel in a window of two lines and two warm rows, as an image, its layout pass, and the
        Searched rows that pass gives: small at .text's start, then ova_probe and step from the window's end."""
        images = linking(self.images, 'ova', 'small.o', 'ova.o')
        image = images['ova']._replace(kernel=[gcclink.Piece('draw', 'small.o', '.text.small')],
                                       windows=[gcclink.Window('draw', 2, 0)],
                                       warm=[gcclink.Piece('warm', 'ova.o', '.text.ova_probe'),
                                             gcclink.Piece('warm', 'ova.o', '.text.step')], text=0x0c900000,
                                       heat=self.heat)
        passed = self.layouts({**images, 'ova': image})['ova']
        return image, passed, [gcclink.Searched(*row) for row in ld.placed(image, passed)]

    def test_holds_a_layout_to_its_record_s_heat_and_to_the_rows_the_record_gives(self):
        image, passed, rows = self.searched()
        self.assertEqual([row[:3] for row in rows], [('small.o', '.text.small', 0), ('ova.o', '.text.ova_probe', 64),
                                                     ('ova.o', '.text.step', 64 + rows[1].size)])
        heat = self.heat
        self.assertEqual(ld.stale(image._replace(searched=rows), passed), [])
        # an image with neither warm rows nor a record has nothing to hold
        self.assertEqual(ld.stale(self.images['main'], self.layouts(self.images)['main']), [])
        again = 'make dc-order records the rows again'
        cases = {
            'no record': ([], [], 'ova: its warm rows have no record of their places, config/ova/order.txt; make '
                                  'dc-order writes one, and gives every row a place anew, so restore a record that '
                                  'was lost'),
            'no heat row': (rows, [], f'ova: config/ova/order.txt names no heat; {again}'),
            'another heat': (rows, ['heat.txt', '0' * 64],
                             f"ova: heat.txt isn't the heat config/ova/order.txt was written by; {again}"),
            # the first row that differs is named, though step's differs too
            'a kernel row that moved': ([rows[0]._replace(place=32), rows[1], rows[2]._replace(place=200)], heat,
                                        "ova: small.o's .text.small lies 0x0 bytes into .text and takes 8, and "
                                        f'config/ova/order.txt has it 0x20 bytes in, taking 8; {again}'),
            'a row the record has not': (rows[:2], heat, "ova: config/ova/order.txt doesn't hold ova.o's .text.step, "
                                                         f'which its rows place; {again}'),
            'a row the record has': ([*rows, gcclink.Searched('ova.o', '.text.ova_unused', 200, 12)], heat,
                                     "ova: config/ova/order.txt holds ova.o's .text.ova_unused, which no kernel or "
                                     f'warm row places; {again}'),
        }
        for case, (searched, given, problem) in cases.items():
            with self.subTest(case):
                self.assertEqual(ld.stale(image._replace(searched=searched, heat=given), passed), [problem])
        with self.assertRaisesRegex(SystemExit, ': absent.txt: No such file or directory$'):
            ld.stale(image._replace(searched=rows, heat=['absent.txt', '0' * 64]), passed)

    def test_a_link_whose_kernels_are_not_as_its_record_has_them_fails_before_any_final_pass(self):
        image, _, rows = self.searched()
        images = {**self.images, 'ova': image._replace(searched=rows)}
        ld.coordinate(images, self.out)
        self.assertTrue(gcclink.final(self.out, 'ova').exists())
        # the check comes before any final pass, with the kernels' across the images
        moved = {**images, 'ova': image._replace(searched=[rows[0]._replace(size=4), *rows[1:]])}
        with self.assertRaisesRegex(SystemExit, r": ova: small.o's \.text\.small lies 0x0 bytes into \.text and takes "
                                                r'8, and config/ova/order.txt has it 0x0 bytes in, taking 4; make '
                                                'dc-order records the rows again$'):
            ld.coordinate(moved, self.out)
        self.assertFalse(gcclink.final(self.out, 'ova').exists())

    def test_a_head_that_grows_past_its_line_fails_the_link_naming_text_which_its_record_has_elsewhere(self):
        # record.o's 8 bytes open ova, and .text starts on the next line, where the record has it. more.o's 64 bytes
        # in the head after them move .text two lines on, and each kernel and warm line with it, to another set than
        # the fixed code met it on
        image, _, rows = self.searched()
        heads = [gcclink.Piece('DHEAD', 'record.o', '.data'), gcclink.Piece('DHEAD', 'more.o', '.data')]
        image = image._replace(inputs=[*image.inputs, 'record.o', 'more.o'], searched=rows, text=0x0c900020)
        ld.coordinate({**self.images, 'ova': image._replace(heads=heads[:1])}, self.out)
        with self.assertRaisesRegex(SystemExit, r': ova: \.text starts at 0x0c900060, and config/ova/order.txt has it '
                                                'at 0x0c900020; make dc-order records the rows again$'):
            ld.coordinate({**self.images, 'ova': image._replace(heads=heads)}, self.out)
        self.assertFalse(gcclink.final(self.out, 'ova').exists())

    def test_an_origin_that_moves_fails_the_link_naming_text_though_each_row_lies_as_far_into_it_as_before(self):
        # ova loads a line on: each row lies as far into .text as its record has it, and on another set than the
        # fixed code met it on
        image, _, rows = self.searched()
        images = {**self.images, 'ova': image._replace(searched=rows)}
        ld.coordinate(images, self.out)
        with self.assertRaisesRegex(SystemExit, r': ova: \.text starts at 0x0c900020, and config/ova/order.txt has it '
                                                'at 0x0c900000; make dc-order records the rows again$'):
            ld.coordinate({**images, 'ova': images['ova']._replace(origin=0x0c900020)}, self.out)
        self.assertFalse(gcclink.final(self.out, 'ova').exists())

    def test_a_record_that_gives_text_no_address_fails_the_link(self):
        image, passed, rows = self.searched()
        self.assertEqual(ld.stale(image._replace(searched=rows, text=None), passed),
                         ["ova: config/ova/order.txt doesn't say where .text starts; make dc-order records the rows "
                          'again'])

    def test_closes_the_code_with_the_last_code_so_that_code_a_change_adds_there_moves_nothing_before_it(self):
        # added.o's 4 KB of the last code lie after PEXTRA's run, in none, and leave every other name, input section
        # and bound where kept.o, which holds kept alone, leaves it. ova's window starts .rodata on the operand cache's
        # 16 KB, so its data stays too
        kernel, windows = [gcclink.Piece('draw', 'small.o', '.text.small')], [gcclink.Window('draw', 2, 0)]
        signatures = []
        for code in ('kept', 'added'):
            images = linking(self.images, 'ova', 'small.o', f'{code}.o', 'ova.o', 'extra.o', 'more.o')
            layout = self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel, windows=windows)})['ova']
            signatures.append({item.replace(f'{code}.o', 'its.o'): each for item, each in ld.signature(layout).items()})
        kept, added = signatures
        self.assertEqual({item for item in kept.keys() | added.keys() if kept.get(item) != added.get(item)},
                         {'.text', "its.o's .text_last.added", 'added'})
        text = next(each for each in layout.elf.sections if each.name == '.text')
        last = next(each for each in layout.placements if each.section == '.text_last.added')
        end = next(symbol.value for symbol in layout.elf.symbols if elf.as_c_name(symbol.name) == '_endof_PEXTRA')
        self.assertEqual((last.address >= end, last.address + last.size), (True, text.address + text.size))

    def test_puts_each_last_row_s_object_s_last_code_ahead_of_the_rest_in_row_order(self):
        # first.o links after added.o, and its row puts its last code where added.o's lies without the row, and
        # added.o's after it. Every other name, input section and bound stays
        images = linking(self.images, 'ova', 'small.o', 'added.o', 'ova.o', 'first.o', 'extra.o', 'more.o')
        signatures, starts = [], []
        for last in ([], ['first.o']):
            layout = self.layouts({**images, 'ova': images['ova']._replace(last=last)})['ova']
            signatures.append(ld.signature(layout))
            starts.append({each.section: each.address for each in layout.placements
                           if each.section.startswith(gcclink.LAST)})
        self.assertEqual({item for item in signatures[0].keys() | signatures[1].keys()
                          if signatures[0].get(item) != signatures[1].get(item)},
                         {"added.o's .text_last.added", "first.o's .text_last.first", 'added', 'first'})
        self.assertEqual(starts[1], {'.text_last.first': starts[0]['.text_last.added'],
                                     '.text_last.added': starts[0]['.text_last.added'] + 64})

    def test_a_last_row_that_names_no_object_one_without_last_code_or_one_named_already_fails_the_link(self):
        images = linking(self.images, 'ova', 'ova.o', 'added.o', 'first.o')
        last = ['libovb.a', 'ova.o', 'first.o', 'added.o', 'first.o']
        with self.assertRaisesRegex(SystemExit, 'ova: its last row names libovb.a, which is no object of its inputs\n'
                                                '.*: ova: its last row names ova.o, which holds no last code\n'
                                                '.*: ova: its last rows name first.o twice$'):
            self.layouts({**images, 'ova': images['ova']._replace(last=last)})

    def test_puts_a_cold_row_s_object_after_the_last_code_and_data_so_that_its_changes_move_nothing_else(self):
        # cold.o links ahead of ova_moved.o. Its code closes .text after first.o's last code, and its string, data
        # and .bss close .data after added_data.o's last data. cold_grown.o, more of each, moves .text's and .data's
        # ends and its own sections after its code: every other name, input section and bound stays, .rodata, which
        # its string would join, and .bss too. ova's window starts .rodata, ova_moved.o's constant, and .bss on the
        # operand cache's 16 KB
        kernel, windows = [gcclink.Piece('draw', 'small.o', '.text.small')], [gcclink.Window('draw', 2, 0)]
        signatures = []
        for cold in ('cold', 'cold_grown'):
            images = linking(self.images, 'ova', 'small.o', 'added_data.o', 'first.o', f'{cold}.o', 'ova_moved.o',
                             'more.o', 'spare.o')
            image = images['ova']._replace(kernel=kernel, windows=windows, cold=[f'{cold}.o'])
            layout = self.layouts({**images, 'ova': image})['ova']
            signatures.append({item.replace(f'{cold}.o', 'its.o'): each for item, each in ld.signature(layout).items()})
            code, string, datum, zero = [each for each in layout.placements if each.file == f'{cold}.o' and each.size]
            text, data = (next(each for each in layout.elf.sections if each.name == name) for name in ('.text', '.data'))
            first = next(each for each in layout.placements if each.section == '.text_last.first')
            last = next(each for each in layout.placements if each.section == '.data_last.added')
            self.assertEqual(([each.output for each in (code, string, datum, zero)],
                              code.address >= first.address + first.size, code.address + code.size,
                              string.address >= last.address + last.size, string.address + string.size <= datum.address,
                              datum.address + datum.size <= zero.address, zero.address + zero.size),
                             (['.text', '.data', '.data', '.data'], True, text.address + text.size, True, True, True,
                              data.address + data.size))
        kept, grown = signatures
        self.assertEqual({item for item in kept.keys() | grown.keys() if kept.get(item) != grown.get(item)},
                         {'.text', '.data', "its.o's .text.cold", "its.o's .rodata.str1.4", "its.o's .data",
                          "its.o's .data.cold", "its.o's .bss", "its.o's .bss.cold", 'cold_datum', 'cold_zero'})

    def test_a_cold_row_whose_object_holds_what_it_can_t_place_or_is_named_twice_fails_the_link(self):
        kernel = [gcclink.Piece('draw', 'small.o', '.text.small')]
        images = linking(self.images, 'ova', 'small.o', 'first.o', 'entry.o', 'cold.o', 'ova.o')
        image = images['ova']._replace(kernel=kernel, last=['first.o'],
                                       cold=['libovb.a', 'first.o', 'entry.o', 'small.o', 'cold.o', 'cold.o'])
        with self.assertRaisesRegex(SystemExit, 'ova: its cold row names libovb.a, which is no object of its inputs\n'
                                                ".*: ova: its cold row names first.o, whose .text_last.first no output "
                                                "section's GCC patterns take\n"
                                                '.*: ova: its last and cold rows name first.o\n'
                                                ".*: ova: its cold row names entry.o, whose P no output section's GCC "
                                                'patterns take\n'
                                                '.*: ova: its cold row names small.o, a section of which another row '
                                                'places\n'
                                                '.*: ova: its cold rows name cold.o twice$'):
            self.layouts({**images, 'ova': image})

    def test_a_cold_row_whose_object_holds_a_common_symbol_fails_the_link(self):
        # common.o's main_common lies in no section of it, so no statement of its row places it. .bss would take it
        # with every other object's, where one that grows moves the rest
        images = linking(self.images, 'ova', 'common.o', 'ova.o')
        with self.assertRaisesRegex(SystemExit, 'ova: its cold row names common.o, whose common symbol main_common '
                                                r'lies in no section of it, so \.bss takes it$'):
            self.layouts({**images, 'ova': images['ova']._replace(cold=['common.o'])})

    def test_a_string_a_change_adds_to_a_cold_object_moves_the_data_of_the_cold_row_before_it_and_no_other_datum(self):
        # cold.o's row comes first, so ld gathers the cold rows' strings where cold.o's lie, and leaves said.o's own
        # section empty. said_more.o's string, 41 bytes longer, grows cold.o's section, and moves cold.o's data and
        # .bss, and then all of its own object. Every other name, input section and bound stays: ova's window starts
        # .rodata, ova_moved.o's constant, and .bss on the operand cache's 16 KB
        kernel, windows = [gcclink.Piece('draw', 'small.o', '.text.small')], [gcclink.Window('draw', 2, 0)]
        signatures, strings = [], []
        for said in ('said', 'said_more'):
            images = linking(self.images, 'ova', 'small.o', 'cold.o', f'{said}.o', 'ova_moved.o', 'more.o', 'spare.o')
            image = images['ova']._replace(kernel=kernel, windows=windows, cold=['cold.o', f'{said}.o'])
            layout = self.layouts({**images, 'ova': image})['ova']
            signatures.append({item.replace(f'{said}.o', 'its.o'): each for item, each in ld.signature(layout).items()})
            sizes = {each.file: each.size for each in layout.placements if each.section == '.rodata.str1.4'}
            strings.append((sizes['cold.o'], sizes[f'{said}.o']))
        (gathered, own), (later_gathered, later_own) = strings
        self.assertEqual((own, later_own, later_gathered - gathered), (0, 0, 41))
        kept, grown = signatures
        self.assertEqual({item for item in kept.keys() | grown.keys() if kept.get(item) != grown.get(item)},
                         {'.data', "cold.o's .rodata.str1.4", "cold.o's .data", "cold.o's .data.cold", "cold.o's .bss",
                          "cold.o's .bss.cold", 'cold_datum', 'cold_zero', "its.o's .rodata.str1.4",
                          "its.o's .rodata.cst4", "its.o's .data", "its.o's .bss"})

    def test_closes_the_data_with_the_last_data_so_that_data_a_change_adds_there_moves_nothing_before_it(self):
        # added_data.o's 8 bytes of the last data lie after DEXTRA's run, in none, and leave every other name, input
        # section and bound where kept.o, which holds kept alone, leaves it. ova's window starts .bss, spare.o's, on
        # the operand cache's 16 KB, so it stays too
        kernel, windows = [gcclink.Piece('draw', 'small.o', '.text.small')], [gcclink.Window('draw', 2, 0)]
        signatures = []
        for code in ('kept', 'added_data'):
            images = linking(self.images, 'ova', 'small.o', f'{code}.o', 'ova.o', 'dextra.o', 'more.o', 'spare.o')
            layout = self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel, windows=windows)})['ova']
            signatures.append({item.replace(f'{code}.o', 'its.o'): each for item, each in ld.signature(layout).items()})
        kept, added = signatures
        self.assertEqual({item for item in kept.keys() | added.keys() if kept.get(item) != added.get(item)},
                         {'.data', "its.o's .data_last.added", 'added_datum'})
        data = next(each for each in layout.elf.sections if each.name == '.data')
        last = next(each for each in layout.placements if each.section == '.data_last.added')
        end = next(symbol.value for symbol in layout.elf.symbols if elf.as_c_name(symbol.name) == '_endof_DEXTRA')
        self.assertEqual((last.address >= end, last.address + last.size, '.bss' in kept),
                         (True, data.address + data.size, True))

    def test_a_kernel_past_its_window_or_windows_on_one_set_fail_the_link(self):
        images = linking(self.images, 'ova', 'large.o', 'small.o', 'ova.o')
        kernel = [gcclink.Piece('draw', 'large.o', '.text.large'), gcclink.Piece('sweep', 'small.o', '.text.small')]
        cases = {'past': ([gcclink.Window('draw', 1, 0), gcclink.Window('sweep', 1, 0)],
                          "ova: its draw kernel takes 2 lines, more than its window's 1"),
                 'offset': ([gcclink.Window('draw', 2, 1), gcclink.Window('sweep', 1, 0)],
                            "ova: its draw kernel takes 3 lines, more than its window's 2"),
                 # small's one line, on set 200, would take a set of its own
                 'one set': ([gcclink.Window('draw', 200, 0), gcclink.Window('sweep', 100, 0)],
                             "ova: a line of its draw kernel and another of its sweep kernel take set 0 of the SH-4's "
                             'instruction cache, and no over row names the two\n.*: ova: its layout pass failed'),
                 'rows': ([gcclink.Window('draw', 2, 0), gcclink.Window('draw', 2, 0), gcclink.Window('clip', 1, 0)],
                          'ova: its window row names clip, which no kernel row names\n.*: ova: its draw kernel has 2 '
                          'windows\n.*: ova: its sweep kernel has no window')}
        for case, (windows, problem) in cases.items():
            with self.subTest(case):
                with self.assertRaisesRegex(SystemExit, f'{problem}$'):
                    self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel, windows=windows)})

    def test_opens_its_output_section_with_each_data_row_s_section_in_row_order(self):
        # zeros.o's .bss and then more.o's .data, though link order puts late.o's .bss and ova.o's .data first
        images = linking(self.images, 'ova', 'ova.o', 'late.o', 'more.o', 'zeros.o')
        data = [gcclink.Piece('data', 'zeros.o', '.bss'), gcclink.Piece('data', 'more.o', '.data')]
        layouts = self.layouts({**images, 'ova': images['ova']._replace(data=data)})
        placed = {output: [(each.file, each.section) for each in layouts['ova'].placements
                           if each.output == output and each.size] for output in ('.data', '.bss')}
        self.assertEqual(placed['.data'][:2], [('more.o', '.data'), ('ova.o', '.data')])
        self.assertEqual(placed['.bss'][:2], [('zeros.o', '.bss'), ('late.o', '.bss')])

    def test_a_data_row_that_places_code_a_run_of_its_own_or_a_section_another_row_places_fails_the_link(self):
        rows = {'code': ([gcclink.Piece('data', 'ova.o', '.text.step')],
                         "ova: its data row places ova.o's .text.step, which is no data"),
                'own run': ([gcclink.Piece('data', 'bounds32.o', 'B32')],
                            "ova: its data row places bounds32.o's B32, which takes a run of its own"),
                'twice': ([gcclink.Piece('data', 'more.o', '.data')] * 2, "ova: its rows place more.o's .data twice")}
        for case, (data, problem) in rows.items():
            with self.subTest(case):
                images = linking(self.images, 'ova', 'ova.o', 'more.o', 'bounds32.o')
                with self.assertRaisesRegex(SystemExit, f'{problem}$'):
                    self.layouts({**images, 'ova': images['ova']._replace(data=data)})

    def test_an_over_row_that_names_one_kernel_twice_or_no_kernel_row_s_fails_the_link(self):
        # a row that gives OTHER names one kernel of ova's, its first, so probe's row fails and the last names two
        # kernels
        images = linking(self.images, 'ova', 'ova.o')
        kernel = [gcclink.Piece('draw', 'ova.o', '.text.ova_probe'), gcclink.Piece('sweep', 'ova.o', '.text.step')]
        over = [('sweep', 'clip'), ('draw', 'draw'), ('probe', 'draw', 'main'), ('draw', 'draw', 'main')]
        images = {**images, 'ova': images['ova']._replace(kernel=kernel, over=over)}
        with self.assertRaisesRegex(SystemExit, 'ova: its over row names clip, which no kernel row names\n'
                                                '.*: ova: its over row names its draw kernel twice\n'
                                                '.*: ova: its over row names probe, which no kernel row names$'):
            self.layouts(images)

    def test_a_kernel_row_that_places_no_code_or_part_of_an_object_s_split_code_fails_the_link(self):
        rows = {'no section': ([gcclink.Piece('draw', 'ova.o', '.text.absent')],
                               "ova: its draw kernel row places ova.o's .text.absent, and that object holds no such "
                               'section'),
                'data': ([gcclink.Piece('draw', 'more.o', '.data')],
                         "ova: its draw kernel row places more.o's .data, which is no code"),
                # mixed.o's P branches to its C, which stays where the object lies
                'split': ([gcclink.Piece('draw', 'mixed.o', 'P')],
                          "ova: its kernel and warm rows take some of mixed.o's code, which is SHC's sections and "
                          "GCC's both, and leave .text.mixed_twice"),
                'apart': ([gcclink.Piece('draw', 'ova.o', '.text.ova_probe'),
                           gcclink.Piece('sweep', 'ova.o', '.text.step'),
                           gcclink.Piece('draw', 'ova.o', '.text.ova_unused')],
                          'ova: its rows split its draw kernel into runs apart')}
        for case, (kernel, problem) in rows.items():
            with self.subTest(case):
                images = linking(self.images, 'ova', 'ova.o', 'more.o', 'mixed.o')
                images = {**images, 'ova': images['ova']._replace(kernel=kernel)}
                with self.assertRaisesRegex(SystemExit, f'{problem}$'):
                    self.layouts(images)

    def test_binds_an_alias_to_its_image_s_own_definition_and_a_constant_to_its_value(self):
        images = linking(self.images, 'main', 'main.o', 'names.o')
        images = {**images, 'main': images['main']._replace(aliases={'main_count': 'main_counter'},
                                                             constants={'main_area': 0x8C0F0000})}
        _, _, finals = ld.coordinate(images, self.out)
        self.assertEqual(words(finals['main'], 'main_count_slot', 1) + words(finals['main'], 'main_area_slot', 1),
                         [gcclink.exports(finals['main'].elf)['main_counter'].value, 0x8C0F0000])
        self.assertFalse({'main_count', 'main_area'} & gcclink.exports(finals['main'].elf).keys())
        # an alias of a name nothing defines fails the link, and one of a name the image takes from another fails
        # resolve
        for symbol, problem in (('main_none', 'main: main_count is a second name for its main_none, which nothing '
                                              'defines\n'),
                                ('ova_probe', r"main: main_count is a second name for its ova_probe, which "
                                              r"\S+main/layout\.elf doesn't define$")):
            with self.subTest(symbol):
                wrong = {**images, 'main': images['main']._replace(aliases={'main_count': symbol})}
                with self.assertRaisesRegex(SystemExit, problem):
                    ld.coordinate(wrong, self.out)

    def test_a_root_pulls_in_the_archive_member_that_defines_an_alias_s_symbol(self):
        # second.o takes alias_second, a second name for alias_target. Nothing else takes alias_target, and only a
        # member of libtarget.a defines it
        (self.root / 'alias.txt').write_text('image  solo  0x8c010000  0x00010000\ninput  solo  head.o\n'
                                             'input  solo  second.o\ninput  solo  libtarget.a\n'
                                             'alias  solo  alias_second  alias_target\n')
        final = ld.coordinate(gcclink.read(self.root / 'alias.txt'), self.out)[2]['solo']
        self.assertEqual(words(final, 'second_slot', 1), [gcclink.exports(final.elf)['alias_target'].value])

    def test_an_input_that_defines_a_name_its_image_binds_to_a_constant_fails_the_link(self):
        # main.o defines main_counter, which the assignment would silently override
        images = {**self.images, 'main': self.images['main']._replace(constants={'main_counter': 0x8C0F0000})}
        with self.assertRaisesRegex(SystemExit, 'main: its inputs define main_counter, which it binds to '
                                                '0x8c0f0000\n'):
            self.layouts(images)

    def test_an_input_that_defines_a_name_its_image_aliases_fails_the_link(self):
        # main.o defines main_counter, which the assignment would silently override
        images = {**self.images, 'main': self.images['main']._replace(aliases={'main_counter': 'main_call'})}
        with self.assertRaisesRegex(SystemExit, 'main: its inputs define main_counter, which it aliases\n'):
            self.layouts(images)

    def test_an_image_that_starts_past_its_origin_fails_the_link(self):
        # ova's code aligns to 4, so it would start 2 bytes past this origin, where its loader puts its file
        images = {**self.images, 'ova': self.images['ova']._replace(origin=0x0C900002)}
        with self.assertRaisesRegex(SystemExit, 'ova: it starts past its origin, 0x0c900002\n'):
            self.layouts(images)

    def test_an_image_that_runs_past_its_load_area_fails_the_link(self):
        # ova's code and table take more than 16 bytes
        images = {**self.images, 'ova': self.images['ova']._replace(length=0x10)}
        with self.assertRaisesRegex(SystemExit, 'ova: it runs past its load area\n'):
            self.layouts(images)

    def test_a_linker_that_can_t_run_fails_the_link_naming_it(self):
        # the toolchain's directory holds no bin/sh-elf-ld. ld.py names it by its real path
        self.out.mkdir()
        linker = re.escape(str(self.out.resolve() / 'bin/sh-elf-ld'))
        with mock.patch.object(gcclink, 'SH_ELF', self.out), \
                self.assertRaisesRegex(SystemExit, f"{linker}: can't run it: No such file or directory$"):
            self.layouts(self.images)

    def test_an_input_that_defines_a_name_its_image_imports_fails_the_link(self):
        # own.o defines main_call, which ova takes from main, and which the assignment would silently override
        with self.assertRaisesRegex(SystemExit, 'ova: its inputs define main_call, which it imports\n'):
            self.layouts(linking(self.images, 'ova', 'ova.o', 'own.o'))

    def test_an_input_that_defines_a_boundary_of_its_link_fails_the_link(self):
        # start.o defines image_start, and startof.o D's start, which the script would silently override
        for path, name in (('start.o', 'image_start'), ('startof.o', '_startof_D')):
            with self.subTest(name), self.assertRaisesRegex(SystemExit, f'main: its inputs define {name}, a boundary '
                                                                        'of its link\n'):
                self.layouts(linking(self.images, 'main', 'main.o', path))

    def test_a_section_the_script_doesn_t_place_fails_the_link(self):
        # no statement of the script takes orphan.o's .foo, and --orphan-handling=error keeps ld from placing it as
        # it would an orphan
        with self.assertRaisesRegex(SystemExit, "ova: error: unplaced orphan section `.foo' from `orphan.o'\n"):
            self.layouts(linking(self.images, 'ova', 'ova.o', 'orphan.o'))

    def test_a_weak_reference_that_nothing_defines_fails_the_link(self):
        # ld would bind weak.o's maybe to 0
        with self.assertRaisesRegex(SystemExit, 'main: maybe is a weak reference that nothing defines, and ld.py has '
                                                'no policy for one\n.*main: its layout pass failed$'):
            self.layouts(linking(self.images, 'main', 'main.o', 'weak.o'))
        self.assertFalse((self.out / 'main/layout.elf').exists())

    def test_takes_the_linker_s_name_off_its_lines_where_the_toolchain_is_a_symlink(self):
        # ld names itself by its real path, which a relative path through a symlink doesn't spell
        self.out.mkdir()
        (self.out / 'sh-elf').symlink_to(TOOLCHAIN)
        with mock.patch.object(gcclink, 'SH_ELF', self.out / 'sh-elf'), \
                self.assertRaisesRegex(SystemExit, 'ova: its inputs define main_call, which it imports\n'):
            self.layouts(linking(self.images, 'ova', 'ova.o', 'own.o'))

    def test_a_failure_in_a_pass_or_a_check_leaves_no_image_s_final_elf(self):
        # three failures: ova's layout pass; ovb's final pass, after main's and ova's have written theirs; and the
        # layout check, where ova's final pass links ova_moved.o. Each follows a run that leaves every final.elf
        link = ld.link

        def changed(name, stage, inputs):
            """ld.link, with the image named name linking head.o and then inputs in its pass named stage."""
            def linked(image, roots, values, out, at):
                if (image.name, at) == (name, stage):
                    image = image._replace(inputs=['head.o', *inputs])
                return link(image, roots, values, out, at)
            return linked

        for name, stage, inputs, failed in (('ova', 'layout', ['ova.o', 'own.o'], 'its layout pass failed'),
                                            ('ovb', 'final', ['ovb.o', 'libovb.a', 'own.o'], 'its final pass failed'),
                                            ('ova', 'final', ['ova_moved.o'], 'ova_table is ')):
            with self.subTest(failed):
                ld.coordinate(self.images, self.out)
                self.assertEqual(len(list(self.out.glob('*/final.elf'))), 3)
                with mock.patch.object(ld, 'link', changed(name, stage, inputs)), \
                        self.assertRaisesRegex(SystemExit, f'{name}: {failed}'):
                    ld.coordinate(self.images, self.out)
                self.assertEqual(list(self.out.glob('*/final.elf')), [])

    def test_a_hangup_or_term_stops_the_link_and_leaves_no_final_elf(self):
        # ld.py's command line, with a linker that holds ovb's final pass until the signal stops ld.py. main's and
        # ova's final passes have written theirs by then. Stopping ld.py stops the linker too
        held = Path(f'{self.out}.held').resolve()
        linker = Path(f'{self.out}.sh-elf/bin/sh-elf-ld')
        linker.parent.mkdir(parents=True)
        linker.write_text(f'#!/bin/sh\ncase "$*" in *ovb/final.elf*) touch {held}; exec sleep 30;; esac\n'
                          f'exec {TOOLCHAIN}/bin/sh-elf-ld "$@"\n')
        linker.chmod(0o755)
        for number, status in ((signal.SIGHUP, 129), (signal.SIGTERM, 143)):
            with self.subTest(number.name):
                held.unlink(missing_ok=True)
                running = subprocess.Popen([sys.executable, str(ROOT / 'tools/ld.py'), 'link.txt', str(self.out)],
                                           env=dict(os.environ, SH_ELF=str(linker.parents[1])),
                                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                while not held.exists() and running.poll() is None:
                    time.sleep(0.01)
                running.send_signal(number)
                self.assertEqual(running.wait(10), status)
                self.assertEqual(list(self.out.glob('*/final.elf')), [])

    def test_a_symlink_to_a_directory_fails_the_run_and_ld_py_removes_nothing_through_it(self):
        # alias names a directory outside the run's, which holds a final.elf of its own
        outside = Path(f'{self.out}.outside')
        outside.mkdir()
        (outside / 'final.elf').write_bytes(b'')
        self.out.mkdir()
        (self.out / 'alias').symlink_to(outside.resolve())
        with self.assertRaisesRegex(SystemExit, f'{self.out}/alias: a symlink to a directory, through which ld.py '
                                                'removes nothing$'):
            ld.main(['link.txt', str(self.out)])
        self.assertTrue((outside / 'final.elf').exists())

    def test_removes_a_final_elf_that_is_a_symlink_to_a_directory_and_leaves_the_directory(self):
        # ld.py can't remove a final.elf that is a directory, and names it. One that is a symlink to a directory it
        # removes as it does a file: the link goes, and the directory stays
        outside = Path(f'{self.out}.outside')
        outside.mkdir()
        (self.out / 'main').mkdir(parents=True)
        (self.out / 'main/final.elf').symlink_to(outside.resolve())
        ld.clear(self.out)
        self.assertFalse((self.out / 'main/final.elf').is_symlink())
        self.assertTrue(outside.is_dir())

    def test_names_a_final_elf_in_a_directory_it_cant_search_and_removes_the_rest(self):
        # each call on locked's final.elf fails, as it does in a directory that ld.py can't search. ld.py can't tell
        # whether locked holds a final.elf, so it names the one locked would hold
        stuck = self.out / 'locked/final.elf'
        for image in ('locked', 'main', 'ova'):
            (self.out / image).mkdir(parents=True)
        for image in ('main', 'ova'):
            (self.out / image / 'final.elf').write_bytes(b'')

        def denied(call):
            def calling(path, *arguments, **options):
                if path == stuck:
                    raise PermissionError(errno.EACCES, os.strerror(errno.EACCES), str(path))
                return call(path, *arguments, **options)
            return calling

        calls = {name: denied(getattr(Path, name)) for name in ('stat', 'lstat', 'unlink')}
        with mock.patch.multiple(Path, **calls), \
                self.assertRaisesRegex(SystemExit, rf'\A[^\n]*: {re.escape(str(stuck))}: Permission denied\Z'):
            ld.clear(self.out)
        self.assertEqual(list(self.out.glob('*/final.elf')), [])

    def test_a_directory_it_can_t_list_fails_the_run_naming_it_and_one_that_isn_t_there_holds_no_final_elf(self):
        # ld.py may search the run's directory and write there, but not list it, so it can't tell that gone holds a
        # final.elf an earlier run left. A directory that isn't there yet holds none
        ld.clear(self.out)
        (self.out / 'gone').mkdir(parents=True)
        (self.out / 'gone/final.elf').write_bytes(b'')
        self.out.chmod(0o300)
        self.addCleanup(self.out.chmod, 0o700)
        if os.access(self.out, os.R_OK):
            self.skipTest('this user lists a directory of any mode, as root does')
        with self.assertRaisesRegex(SystemExit, rf"\A[^\n]*: {re.escape(str(self.out))}: can't list it: Permission "
                                                r'denied\Z'):
            ld.clear(self.out)
        self.assertTrue((self.out / 'gone/final.elf').exists())

    def test_tries_to_remove_each_final_elf_and_keeps_the_failure_that_stopped_the_run(self):
        # ova's final.elf can't be removed once its final pass writes it. ova's final pass links ova_moved.o, so the
        # layout check fails the run. The run then removes main's and ovb's final.elf, names ova's on stderr, and
        # raises the layout check's failure
        unlink, stuck, link = Path.unlink, self.out / 'ova/final.elf', ld.link

        def unlinking(path, missing_ok=False):
            if path == stuck and path.exists():
                raise PermissionError(errno.EACCES, os.strerror(errno.EACCES), str(path))
            unlink(path, missing_ok)

        def moving(image, roots, values, out, stage):
            if (image.name, stage) == ('ova', 'final'):
                image = image._replace(inputs=['head.o', 'ova_moved.o'])
            return link(image, roots, values, out, stage)

        with mock.patch.object(Path, 'unlink', unlinking), mock.patch.object(ld, 'link', moving), \
                contextlib.redirect_stderr(io.StringIO()) as printed, \
                self.assertRaisesRegex(SystemExit, 'ova: ova_table is '):
            ld.coordinate(self.images, self.out)
        self.assertEqual(list(self.out.glob('*/final.elf')), [stuck])
        self.assertRegex(printed.getvalue(), f'{re.escape(str(stuck))}: Permission denied\n$')

    def test_a_run_clears_each_final_elf_before_its_first_pass(self):
        # a run's first pass finds no final.elf, so a run killed after one final pass leaves no earlier one beside
        # its own. A spec that fails to read leaves none either. So does a spec that no longer names an image, as gone
        link = ld.link
        before = []

        def linked(*arguments):
            before.append(sorted(self.out.glob('*/final.elf')))
            return link(*arguments)

        def stale():
            ld.coordinate(self.images, self.out)
            (self.out / 'gone').mkdir(exist_ok=True)
            (self.out / 'gone/final.elf').write_bytes(b'')

        stale()
        with mock.patch.object(ld, 'link', linked):
            ld.coordinate(self.images, self.out)
        self.assertEqual(before[0], [])
        self.assertFalse((self.out / 'gone/final.elf').exists())
        stale()
        (self.out / 'bad.txt').write_text('image main\n')
        with self.assertRaisesRegex(SystemExit, 'not a row'):
            ld.main([str(self.out / 'bad.txt'), str(self.out)])
        self.assertEqual(list(self.out.glob('*/final.elf')), [])


class LinkTime(Fixture):
    """The link-time compile, whole(), with DRIVER standing in for a GCC built with link-time optimization, and GNU
    ld's passes of what it gives, which the toolchain's own linker links."""

    FLAGS = ['-O2', '-flto-partition=1to1']

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        compiled = flags()
        (cls.root / 'ltrans').mkdir()
        for name, text in LINK_TIME.items():
            (cls.root / name).write_text(text)
            more = ['-fdata-sections'] if name.startswith('ltrans/') else []
            run('bin/sh-elf-gcc', *compiled, *more, '-c', '-o', str(Path(name).with_suffix('.o')), name, root=cls.root)
        # a toolchain whose driver is DRIVER and whose linker is the fixture's
        (cls.root / 'stand-in/bin').mkdir(parents=True)
        (cls.root / 'stand-in/bin/sh-elf-gcc').write_text(DRIVER)
        (cls.root / 'stand-in/bin/sh-elf-gcc').chmod(0o755)
        (cls.root / 'stand-in/bin/sh-elf-ld').symlink_to(TOOLCHAIN / 'bin/sh-elf-ld')

    def setUp(self):
        super().setUp()
        patched = mock.patch.object(gcclink, 'SH_ELF', self.root / 'stand-in')
        patched.start()
        self.addCleanup(patched.stop)
        # main links two objects that carry the code after its own
        self.fat = linking(self.images, 'main', 'main.o', 'fat.o', 'gone.o')
        self.lto = self.out / 'main/lto'

    def standing_in(self, how):
        """The environment with how for LTO_STAND_IN, DRIVER's instructions."""
        return mock.patch.dict(os.environ, {'LTO_STAND_IN': how})

    def test_tells_an_object_that_carries_gcc_s_intermediate_code(self):
        # an object with a .gnu.lto_ section, and not the same C without one, nor an archive, whatever its member
        # carries: whole() takes objects alone
        run('bin/sh-elf-ar', 'rc', 'libfat.a', 'fat.o', root=self.root)
        self.assertEqual([ld.carries(path) for path in ('fat.o', 'gone.o', 'plain.o', 'main.o', 'libfat.a')],
                         [True, True, False, False, False])
        with self.assertRaisesRegex(SystemExit, ': absent.o: No such file or directory$'):
            ld.carries('absent.o')

    def test_compiles_an_image_s_intermediate_code_whole_and_links_each_link_time_object_in_its_object_s_place(self):
        _, _, finals = ld.coordinate(self.fat, self.out, self.FLAGS)
        # the compile takes the flags, links nothing of a C library, has GCC name its partitions and keep them, and
        # links by a script of its own with the image's entry, then roots what GCC's code may call. The inputs come
        # last, in link order, each by its own path
        argv = (self.lto / 'argv.txt').read_text().splitlines()
        script = str(self.lto / 'whole.ld')
        self.assertEqual(argv, [*self.FLAGS, '-nostdlib', '-Q', '-save-temps', '-o', str(self.lto / 'whole.elf'),
                                f'-Wl,-T,{script}', '-Wl,-e,_start', '-Wl,--no-warn-rwx-segments',
                                *(f'-Wl,-u,_{name}' for name in ld.CALLED_BY_GCC), 'head.o', 'main.o', 'fat.o', 'gone.o'])
        # the script roots what the overlays import from main, and each datum the compiled objects define by a global
        # name, and no static one
        roots = re.findall(r'(?m)^EXTERN\((\w+)\)$', Path(script).read_text())
        self.assertEqual(roots, ['_main_call', '_main_counter', '_fat_table'])
        # the record gives each object the compile took its link-time object, or - where GCC kept nothing of it, and
        # the log keeps what GCC printed. GCC's other files are gone
        self.assertEqual((self.lto / 'objects.txt').read_text(), f'fat.o {self.lto}/fat.o\ngone.o -\n')
        self.assertEqual(gcclink.taken(self.out, 'main'), {'fat.o': f'{self.lto}/fat.o', 'gone.o': None})
        self.assertEqual(gcclink.taken(self.out, 'ova'), {})
        self.assertIn(f' {self.lto}/whole.elf.ltrans0.o (fat.o 10 insns)\n', (self.lto / 'whole.log').read_text())
        self.assertEqual(sorted(path.name for path in self.lto.iterdir()),
                         ['argv.txt', 'fat.o', 'objects.txt', 'provisional.ld', 'whole.ld', 'whole.log'])
        self.assertEqual((self.lto / 'fat.o').read_bytes(), (self.root / 'ltrans/fat.o').read_bytes())
        # the passes link the link-time object in fat.o's place and nothing of gone.o: what GCC dropped is no part
        # of the image, and each datum lies in a section of its own
        placed = {(each.file, each.section) for each in finals['main'].placements if each.size}
        linked = str(self.lto / 'fat.o')
        self.assertLessEqual({(linked, '.text.fat_step'), (linked, '.data.fat_table'), ('main.o', '.text.start')},
                             placed)
        self.assertEqual({file for file, _ in placed} & {'fat.o', 'gone.o'}, set())
        defined = gcclink.exports(finals['main'].elf)
        self.assertLessEqual({'fat_step', 'fat_table', 'start'}, set(defined))
        self.assertEqual({'fat_peek', 'gone_helper'} & set(defined), set())
        # and without the compile, each object's own code
        _, _, finals = ld.coordinate(self.fat, self.out)
        self.assertLessEqual({'fat_peek', 'gone_helper', 'fat_step'}, set(gcclink.exports(finals['main'].elf)))

    def test_an_image_none_of_whose_objects_carries_the_code_runs_no_compile(self):
        ld.coordinate(self.images, self.out, self.FLAGS)
        self.assertEqual(sorted(self.out.glob('*/lto')), [])

    def test_links_from_its_command_line_with_the_compile_s_flags_in_one_word(self):
        Path('fat.txt').write_text(SPEC + 'input   main  fat.o\n')
        for given, compiled in ((['--lto=-O2'], ['-O2']), (['--lto=-O2 -g0'], ['-O2', '-g0']),
                                (['--lto', '-O2 -g0'], ['-O2', '-g0']), (['--lto='], [])):
            with self.subTest(given):
                done = subprocess.run([sys.executable, str(ROOT / 'tools/ld.py'), *given, 'fat.txt', str(self.out)],
                                      env=dict(os.environ, SH_ELF='stand-in'), stdout=subprocess.PIPE,
                                      stderr=subprocess.STDOUT, text=True)
                self.assertEqual((done.returncode, done.stdout), (0, ''))
                argv = (self.lto / 'argv.txt').read_text().splitlines()
                self.assertEqual(argv[:argv.index('-nostdlib')], compiled)

    def test_a_link_with_no_compile_leaves_no_record_of_an_earlier_one(self):
        # tools/verify.py and tools/publish.py read the record, so one a link didn't write would have them check and
        # hash a link that was not made
        ld.coordinate(self.fat, self.out, self.FLAGS)
        self.assertEqual(gcclink.link_time(self.fat, self.out)['main'].inputs,
                         ['head.o', 'main.o', str(self.lto / 'fat.o')])
        ld.coordinate(self.fat, self.out)
        self.assertFalse(self.lto.exists())
        self.assertEqual(gcclink.link_time(self.fat, self.out), self.fat)

    def test_a_compile_that_fails_or_warns_fails_the_link(self):
        for how, problem in (('fails', f': main: its link-time compile failed, as {self.lto}/whole.log says$'),
                             ('warns', r"(?m): main: fat.c:1:5: warning: type of 'fat_table' does not match original "
                                       r'declaration \[-Wlto-type-mismatch\]\n.*: main: its link-time compile warns, as '
                                       rf'{self.lto}/whole.log says, and a warning fails it, as it fails each compile$'),
                             ('assembles', rf'(?m): main: {self.lto}/whole\.elf\.ltrans0\.ltrans\.s:12: Warning: overflow '
                                           r'in literal \(\.long\)\n.*: main: its link-time compile warns, as '
                                           rf'{self.lto}/whole.log says, and a warning fails it, as it fails each '
                                           'compile$')):
            with self.subTest(how), self.standing_in(how), self.assertRaisesRegex(SystemExit, problem):
                ld.coordinate(self.fat, self.out, self.FLAGS)
            self.assertFalse((self.lto / 'objects.txt').exists())
            self.assertFalse(gcclink.final(self.out, 'main').exists())

    def test_a_link_time_object_the_log_names_for_no_object_or_for_one_the_compile_didn_t_take_fails_the_link(self):
        # GCC names each partition's object in what -Q prints, and nowhere else. A line the pattern misses would read
        # as an object GCC kept nothing of, and its code would leave the image with status 0
        log = f'{self.lto}/whole.log'
        cases = {
            'splits=fat.o': f': main: its link-time compile wrote {self.lto}/whole.elf.ltrans0.ltrans.o, which {log} '
                            'names for no object, so its code would leave the link$',
            'misnames=fat.o': ': main: its link-time compile wrote code of other.o, which it took from no object of '
                              f'its own, as {log} says$',
            'doubles=fat.o': f': main: its link-time compile wrote two objects of fat.o, as {log} says, and a '
                             "link-time object takes its object's place alone$",
        }
        for how, problem in cases.items():
            with self.subTest(how), self.standing_in(how), self.assertRaisesRegex(SystemExit, problem):
                ld.coordinate(self.fat, self.out, self.FLAGS)
            self.assertFalse((self.lto / 'objects.txt').exists())

    def test_an_object_that_carries_the_code_by_an_absolute_path_or_one_through_its_parent_fails_the_link(self):
        # its link-time object's path is its own under DIR/IMAGE/lto/, which for either would lie elsewhere: for an
        # absolute path, on the object itself. The link fails before the compile runs, and leaves the object as it was
        Path('inner').mkdir(exist_ok=True)
        before = (self.root / 'fat.o').read_bytes()
        for path in (str(self.root / 'fat.o'), '../' + self.root.name + '/fat.o', 'inner/../fat.o'):
            with self.subTest(path), self.assertRaisesRegex(
                    SystemExit, rf": main: {re.escape(path)} carries GCC's intermediate code, and its path is absolute "
                                rf'or holds \.\., so its link-time object has no place under {self.lto}/; name it by a '
                                'path from the working directory$'):
                ld.coordinate(linking(self.images, 'main', 'main.o', path), self.out, self.FLAGS)
            self.assertFalse(self.lto.exists())
            self.assertEqual((self.root / 'fat.o').read_bytes(), before)
        # an object that carries none links by any path
        ld.coordinate(linking(self.images, 'main', 'main.o', str(self.root / 'plain.o')), self.out, self.FLAGS)

    def test_a_row_s_object_that_carries_the_code_fails_the_link_before_the_compile(self):
        # a kernel, warm, data, last or cold row places its object's own code, so make dc compiles the object without
        # the intermediate code. One that carries it was compiled before its row stood, and the compile would inline
        # its code elsewhere and move what the row places
        main = self.fat['main']
        rows = {'kernel': main._replace(kernel=[gcclink.Piece('draw', 'fat.o', '.text.fat_step')]),
                'warm': main._replace(warm=[gcclink.Piece('warm', 'fat.o', '.text.fat_step')]),
                'data': main._replace(data=[gcclink.Piece('data', 'fat.o', '.data')]),
                'last': main._replace(last=['fat.o']), 'cold': main._replace(cold=['fat.o'])}
        for kind, image in rows.items():
            with self.subTest(kind), self.assertRaisesRegex(
                    SystemExit, rf": main: fat.o carries GCC's intermediate code, and a {kind} row names it, whose "
                                'object keeps its own code; remove fat.o, and make dc compiles it again$'):
                ld.coordinate({**self.fat, 'main': image}, self.out, self.FLAGS)
            self.assertFalse(self.lto.exists())
        # the same kernel row of the object without the code links, beside an object the compile takes: the row's
        # section is its object's own
        plain = linking(self.images, 'main', 'main.o', 'plain.o', 'gone.o')
        kernel = [gcclink.Piece('draw', 'plain.o', '.text.fat_step')]
        _, _, finals = ld.coordinate({**plain, 'main': plain['main']._replace(kernel=kernel)}, self.out, self.FLAGS)
        self.assertEqual((self.lto / 'objects.txt').read_text(), 'gone.o -\n')
        text = next(each for each in finals['main'].placements if each.output == '.text' and each.size)
        self.assertEqual((text.file, text.section), ('plain.o', '.text.fat_step'))

    def test_names_each_image_s_object_at_fault_at_once_before_any_image_s_compile(self):
        # ova's object is at fault and main's is not: main comes first, and its compile, the longest of a link, doesn't
        # run for a link that can't pass. With main's at fault too, one failure names both
        images = dict(self.fat)
        images['ova'] = images['ova']._replace(inputs=[*images['ova'].inputs, 'gone.o'], cold=['gone.o'])
        cold = (": ova: gone.o carries GCC's intermediate code, and a cold row names it, whose object keeps its own "
                'code; remove gone.o, and make dc compiles it again')
        with self.assertRaisesRegex(SystemExit, rf'\A[^\n]*{cold}\Z'):
            ld.coordinate(images, self.out, self.FLAGS)
        self.assertEqual(sorted(self.out.glob('*/lto')), [])
        images['main'] = linking(images, 'main', 'main.o', 'fat.o', str(self.root / 'gone.o'))['main']._replace(
            last=['fat.o'])
        with self.assertRaisesRegex(
                SystemExit, r"\A[^\n]*: main: fat.o carries GCC's intermediate code, and a last row names it, [^\n]*\n"
                            rf"[^\n]*: main: {self.root}/gone.o carries GCC's intermediate code, and its path is "
                            rf'absolute or holds \.\., [^\n]*\n[^\n]*{cold}\Z'):
            ld.coordinate(images, self.out, self.FLAGS)
        self.assertEqual(sorted(self.out.glob('*/lto')), [])

    def test_a_directory_whose_path_the_record_can_t_hold_fails_the_link_before_the_compile(self):
        # the record names each link-time object by its path under DIR, a field of a row, and GCC names each
        # partition's object in a line of words, so a DIR with a space would lose every object the compile wrote
        for out in ('a link', 'link+', 'link:1'):
            with self.subTest(out), self.assertRaisesRegex(
                    SystemExit, rf'\A[^\n]*: {re.escape(out)}: the link-time compile records each object it writes by '
                                r"its path under the directory, in each image's lto/objects.txt, and a path there "
                                r'holds letters, digits and _ \. / - alone; link under a directory named so\Z'):
                ld.coordinate(self.fat, Path(out), self.FLAGS)
            self.assertFalse(Path(out).exists())
            # with no compile to record, the passes link under it
            ld.coordinate(self.fat, Path(out))
            self.assertTrue(gcclink.final(Path(out), 'main').is_file())
        ld.coordinate(self.fat, self.root / 'by-its.root/link_1', self.FLAGS)

    def test_a_datum_of_a_global_name_that_the_compile_drops_fails_the_link(self):
        # the compile's script roots each datum its objects define by a global name, since a debugger reads the
        # game's data by name. Here the stand-in keeps nothing of lost.o all the same, so the passes link an image
        # with no lost_count, and nothing else would say so: the record reads as for an object GCC kept nothing of
        images = linking(self.images, 'main', 'main.o', 'fat.o', 'lost.o')
        with self.assertRaisesRegex(
                SystemExit, rf'\A[^\n]*: main: lost\.o defines lost_count, a datum of a global name, which '
                            rf'{self.lto}/whole\.ld roots for its link-time compile, and {self.out}/main/layout\.elf '
                            r"doesn't define it: GCC dropped a datum a debugger reads by that name\Z"):
            ld.coordinate(images, self.out, self.FLAGS)
        roots = re.findall(r'(?m)^EXTERN\((\w+)\)$', (self.lto / 'whole.ld').read_text())
        self.assertEqual(roots, ['_main_call', '_main_counter', '_fat_table', '_lost_count'])
        self.assertEqual((self.lto / 'objects.txt').read_text(), f'fat.o {self.lto}/fat.o\nlost.o -\n')
        self.assertFalse(gcclink.final(self.out, 'main').exists())
        # lost_step is code, which GCC may inline and drop, as it does gone_helper. With no compile, the image holds
        # lost.o's own code and datum
        _, _, finals = ld.coordinate(images, self.out)
        self.assertLessEqual({'lost_count', 'lost_step'}, set(gcclink.exports(finals['main'].elf)))
        # a weak datum that another input defines is defined in the image, whatever GCC kept of its object
        images = linking(self.images, 'main', 'main.o', 'weak.o', 'spare.o')
        _, _, finals = ld.coordinate(images, self.out, self.FLAGS)
        self.assertEqual((self.lto / 'objects.txt').read_text(), 'weak.o -\n')
        self.assertIn('main_spare', gcclink.exports(finals['main'].elf))

    def test_a_head_or_tail_row_of_a_compiled_object_places_its_link_time_object_s_sections(self):
        # fat.o's .data is one section, and its link-time object's a section for each datum, whose names start with
        # .data and a dot, beside an empty .data. A row of a section GCC kept nothing of, as fat_peek's, goes, and one
        # of no section of the object fails
        main = self.fat['main']._replace(tails=[gcclink.Piece('DEND', 'fat.o', '.data'),
                                                gcclink.Piece('DEND', 'fat.o', '.text.fat_peek')],
                                         heads=[gcclink.Piece('DHEAD', 'main.o', '.data')])
        ld.coordinate({**self.fat, 'main': main}, self.out, self.FLAGS)
        linked, made = gcclink.link_time({**self.fat, 'main': main}, self.out)['main'], str(self.lto / 'fat.o')
        self.assertEqual(linked.tails, [gcclink.Piece('DEND', made, '.data'),
                                        gcclink.Piece('DEND', made, '.data.fat_table')])
        self.assertEqual(linked.heads, main.heads)
        final = gcclink.linked(self.out, 'main', 'final')
        self.assertEqual([(each.file, each.section) for each in final.placements if each.output == '.tail' and each.size],
                         [(made, '.data.fat_table')])
        # tools/verify.py reads the record with the spec's rows, and the compile's own script checks each row first
        absent = {**self.fat, 'main': main._replace(tails=[gcclink.Piece('DEND', 'fat.o', '.data.absent')])}
        with self.assertRaisesRegex(SystemExit, ": main: its tail row places fat.o's .data.absent, and that object "
                                                'holds no such section$'):
            gcclink.link_time(absent, self.out)
        with self.assertRaisesRegex(SystemExit, ": main: its DEND row places fat.o's .data.absent, and that object "
                                                'holds no such section$'):
            ld.coordinate(absent, self.out, self.FLAGS)
        self.assertFalse((self.lto / 'argv.txt').exists())

    def test_reads_the_record_of_the_link_time_objects_and_fails_on_a_line_that_is_none_of_its_rows(self):
        record = self.lto / 'objects.txt'
        self.lto.mkdir(parents=True)
        record.write_text('fat.o -\ngone.o -\n')
        self.assertEqual(gcclink.link_time(self.fat, self.out)['main'].inputs, ['head.o', 'main.o'])
        for text, problem in (('fat.o\n', f"{record}:1: not a row: an object's PATH, then its link-time object's "
                                          'PATH, or - where GCC kept nothing of it'),
                              ('fat.o -\n- gone.o\n', f'{record}:2: not a row: '),
                              ('fat.o a*.o\n', f'{record}:1: not a row: '),
                              ('fat.o -\nfat.o lto/fat.o\n', f'{record}:2: fat.o has a row already')):
            with self.subTest(text):
                record.write_text(text)
                with self.assertRaisesRegex(SystemExit, re.escape(problem)):
                    gcclink.link_time(self.fat, self.out)

    def test_names_the_object_a_link_time_object_stands_for(self):
        # by its path under DIR/IMAGE/lto/, whatever DIR, and no path elsewhere
        for out in ('out', 'build/dc/link', '/tmp/a link'):
            with self.subTest(out):
                self.assertEqual(gcclink.compiled_whole(out, f'{out}/main/lto/build/dc/src/a.o'), 'build/dc/src/a.o')
                for other in ('build/dc/src/a.o', f'{out}/main/lto', f'{out}/main/final.elf', f'{out}/lto/a.o',
                              f'{out}/main/pass/lto/a.o', 'elsewhere/main/lto/a.o'):
                    self.assertIsNone(gcclink.compiled_whole(out, other), other)


class Pinned(Fixture):
    """The link of an image whose record gives each warm row a place: ova, with small.o its draw kernel in a window of
    two lines, and two warm rows, a and b, each a section of an object the test assembles at a size it gives."""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.compiled = flags()

    def unit(self, **sizes):
        """The path of an object the test assembles, new each time: a code section .text.NAME of SIZE bytes, on 4, for
        each NAME=SIZE of sizes, in their order."""
        source = self.out.with_suffix('.s')
        source.write_text(''.join(f'        .section .text.{name},"ax",@progbits\n        .p2align 2\n'
                                  f'        .global _{name}\n_{name}:\n        .space {size}\n'
                                  for name, size in sizes.items()))
        run('bin/sh-elf-gcc', *self.compiled, '-c', '-o', str(self.out.with_suffix('.o')), str(source), root=self.root)
        return str(self.out.with_suffix('.o'))

    def image(self, places, **sizes):
        """The fixture's images, with ova as the class has it: its warm rows the sections of sizes, in their order,
        and its record small's place, then each of places, a (NAME, PLACE) of a warm row's, in their order."""
        path = self.unit(**sizes)
        images = linking(self.images, 'ova', 'small.o', path, 'ova.o')
        searched = [gcclink.Searched('small.o', '.text.small', 0, 8),
                    *(gcclink.Searched(path, f'.text.{name}', place, sizes.get(name, 0)) for name, place in places)]
        return {**images, 'ova': images['ova']._replace(
            kernel=[gcclink.Piece('draw', 'small.o', '.text.small')], windows=[gcclink.Window('draw', 2, 0)],
            warm=[gcclink.Piece('warm', path, f'.text.{name}') for name in sizes], searched=searched, text=0x0c900000,
            heat=self.heat)}

    def placed(self, images):
        """Where a link of images lays each section of ova's object, the test's, and of ova.o, by name, as an offset
        into ova's .text."""
        _, layouts, _ = ld.coordinate(images, self.out)
        return {each.section: each.address - images['ova'].origin for each in layouts['ova'].placements
                if each.output == '.text' and each.file in (str(self.out.with_suffix('.o')), 'ova.o')}

    def refused(self, images, problem):
        """Hold that ova's layout pass of images fails before ld runs, with problem, a pattern, its last line."""
        with mock.patch.object(ld, 'run_child', side_effect=AssertionError('ld ran')), \
                self.assertRaisesRegex(SystemExit, f': {problem}$'):
            ld.link(images['ova'], gcclink.rooted(images)['ova'], ld.provisional(images, images['ova']), self.out,
                    'layout')

    def test_lays_each_warm_row_at_its_record_s_place_and_a_row_that_grows_within_its_lines_moves_nothing(self):
        # the record gives a the window's end and b the third line after it, so a's 36 bytes have two lines and a
        # spare one. a at 40 bytes, at 64 and at the 96 that fill its lines leaves b and the code after it where they
        # lay, and the link passes each
        places = [('a', 0x40), ('b', 0xa0)]
        laid = [self.placed(self.image(places, a=size, b=12)) for size in (36, 40, 64, 96)]
        self.assertEqual((laid[0]['.text.a'], laid[0]['.text.b']), (0x40, 0xa0))
        self.assertEqual(laid[1:], [laid[0]] * 3)

    def test_a_record_of_rows_packed_in_row_order_links_as_the_rows_alone_do(self):
        # the places a link without a record gives the rows, each after the last on its alignment, are a record's,
        # and both passes of the link by it are the same bytes. b's 14 bytes end short of c's alignment
        images = self.image([], a=36, b=14, c=8)
        bare = {**images, 'ova': images['ova']._replace(searched=[])}
        layouts = self.layouts(bare)
        packed = [gcclink.Searched(*row) for row in ld.placed(images['ova'], layouts['ova'])]
        self.assertEqual([row.place for row in packed], [0, 0x40, 0x64, 0x74])
        self.finals(bare, ld.resolve(bare, layouts))
        script = (self.out / 'ova/layout.ld').read_text()
        self.assertEqual([script.count(f'    . = {place:#x};\n') for place in (0x40, 0x64, 0x74)], [1, 0, 0])
        loose = [(self.out / f'ova/{stage}.elf').read_bytes() for stage in ld.PASSES]
        ld.coordinate({**images, 'ova': images['ova']._replace(searched=packed)}, self.out)
        self.assertEqual([(self.out / f'ova/{stage}.elf').read_bytes() for stage in ld.PASSES], loose)
        script = (self.out / 'ova/layout.ld').read_text()
        self.assertEqual([script.count(f'    . = {place:#x};\n') for place in (0x40, 0x64, 0x74)], [2, 1, 1])

    def test_a_warm_row_past_the_next_row_s_place_fails_before_ld_runs_naming_it_and_what_places_it_again(self):
        # a's 100 bytes run 4 past b's place, and c's 40 run 8 past nothing: the last row has no row to pass
        images = self.image([('a', 0x40), ('b', 0xa0), ('c', 0xc0)], a=100, b=12, c=40)
        self.refused(images, rf"ova: {self.out}\.o's \.text\.a takes 100 bytes from its place, 0x40 bytes into "
                             r"\.text, 4 past the next warm row's place in config/ova/order.txt; make dc-order gives "
                             'it another place')
        with self.assertRaisesRegex(SystemExit, "4 past the next warm row's place in config/ova/order.txt; make "
                                                'dc-order gives it another place$'):
            ld.coordinate(images, self.out)
        self.assertFalse(gcclink.final(self.out, 'ova').exists())
        self.assertEqual(self.placed(self.image([('a', 0x40), ('b', 0xa0), ('c', 0xc0)], a=96, b=12, c=40))['.text.c'],
                         0xc0)

    def test_an_image_past_its_load_area_is_told_the_bytes_its_record_leaves_free_and_what_lays_the_rows_without(self):
        # the record has b three lines past a, whose 36 bytes leave 60 free under it, as where a row left for
        # another place. ova's load area ends 4 bytes short of its code
        images = self.image([('a', 0x40), ('b', 0xa0)], a=36, b=12)
        fits = self.layouts(images)['ova']
        length = max(section.address + section.size for section in fits.elf.sections) - images['ova'].origin
        short = {**images, 'ova': images['ova']._replace(length=length - 4)}
        with self.assertRaisesRegex(SystemExit, r': ova: it runs past its load area\n(.*\n)*.*: ova: '
                                                r'config/ova/order\.txt leaves 60 bytes free among its warm rows; '
                                                'make dc-order PACK=1 lays the rows with none free, which moves each '
                                                r'row after the first free byte\n.*: ova: its layout pass failed$'):
            self.layouts(short)
        # a record that leaves none free has nothing to pack
        images = self.image([('a', 0x40), ('b', 0x64)], a=36, b=12)
        with self.assertRaisesRegex(SystemExit, ': ova: it runs past its load area\n') as failed:
            self.layouts({**images, 'ova': images['ova']._replace(length=length - 64)})
        self.assertNotIn('free among', str(failed.exception))

    def windowed(self, places, lines, **sizes):
        """image()'s images, with a window of lines lines for ova's warm rows."""
        images = self.image(places, **sizes)
        return {**images, 'ova': images['ova']._replace(windows=[*images['ova'].windows,
                                                                 gcclink.Window('warm', lines, 0)])}

    def after(self, images):
        """Where a link of images starts ova.o's code, which follows ova's warm rows and head.o's 4 bytes of code,
        as an offset into ova's .text."""
        return min(place for name, place in self.placed(images).items() if name not in ('.text.a', '.text.b'))

    def test_starts_the_code_after_the_warm_rows_at_their_window_s_end_whatever_they_take_and_wherever_they_lie(self):
        # the warm rows have six lines from the end of the kernel's window, 0x40 bytes in. head.o's code starts at
        # their end, and ova.o's after it, whatever a and b take, and when a has left its place for one past b
        cases = (([('a', 0x40), ('b', 0xa0)], {'a': 36, 'b': 12}), ([('a', 0x40), ('b', 0xa0)], {'a': 96, 'b': 96}),
                 ([('b', 0xa0), ('a', 0xc0)], {'b': 12, 'a': 40}))
        self.assertEqual([self.after(self.windowed(places, 6, **sizes)) for places, sizes in cases], [0x104] * 3)
        # without the window it follows the last row, and moves as that row grows or another passes it
        self.assertEqual([self.after(self.image(places, **sizes)) for places, sizes in cases], [0xb0, 0x104, 0xec])
        # an image its warm rows' window lengthens past its load area has no bytes to pack
        images = self.windowed([('a', 0x40), ('b', 0xa0)], 6, a=36, b=12)
        with self.assertRaisesRegex(SystemExit, ': ova: it runs past its load area\n') as failed:
            self.layouts({**images, 'ova': images['ova']._replace(length=0x100)})
        self.assertNotIn('free among', str(failed.exception))

    def test_a_last_warm_row_past_its_window_s_end_fails_before_ld_runs_and_so_does_a_second_window(self):
        places = [('a', 0x40), ('b', 0xa0)]
        self.refused(self.windowed(places, 6, a=36, b=100),
                     rf"ova: {self.out}\.o's \.text\.b takes 100 bytes from its place, 0xa0 bytes into \.text, 4 past "
                     r"the end of the warm rows' window, 0x100 bytes in; make dc-order gives it another place")
        images = self.windowed(places, 6, a=36, b=12)
        twice = [*images['ova'].windows, gcclink.Window('warm', 8, 0)]
        self.refused({**images, 'ova': images['ova']._replace(windows=twice)}, 'ova: its warm rows have 2 windows')
        offset = [images['ova'].windows[0], gcclink.Window('warm', 6, 1)]
        self.refused({**images, 'ova': images['ova']._replace(windows=offset)},
                     "ova: its warm rows' window gives an OFFSET, 1, and the warm rows start where the last window "
                     'ends')

    def test_the_link_time_compile_s_script_lays_the_rows_at_their_places_and_a_row_past_its_fails_before_it(self):
        # an object that carries intermediate code, as one compiled with -flto does, has ova's link-time compile
        # run. A stand-in for GCC's driver writes no code. The compile links by the layout pass's script, so the
        # script it hands the driver moves to each warm row's place
        source = self.out.with_suffix('.lto.s')
        source.write_text('        .section .gnu.lto_.opts,"e",@progbits\n        .byte 0\n')
        carried = str(self.out.with_suffix('.lto.o'))
        run('bin/sh-elf-gcc', *self.compiled, '-c', '-o', carried, str(source), root=self.root)

        def carrying(images):
            return {**images, 'ova': images['ova']._replace(inputs=[*images['ova'].inputs, carried])}

        images = carrying(self.image([('a', 0x40), ('b', 0xa0)], a=36, b=12))
        driver = subprocess.CompletedProcess([], 0, '', None)
        with mock.patch.object(ld, 'run_child', return_value=driver) as ran:
            ld.whole(images, gcclink.rooted(images), self.out, [])
        self.assertEqual(ran.call_count, 1)
        script = (self.out / 'ova/lto/whole.ld').read_text()
        self.assertEqual([script.count(f'    . = {place:#x};\n    "{self.out}.[o]"(.text.{name})\n')
                          for name, place in (('a', 0x40), ('b', 0xa0))], [1, 1])
        # a row that outgrew its place fails before the compile runs, as it does before ld
        images = carrying(self.image([('a', 0x40), ('b', 0xa0)], a=100, b=12))
        with mock.patch.object(ld, 'run_child', side_effect=AssertionError('the compile ran')), \
                self.assertRaisesRegex(SystemExit, r"\.text\.a takes 100 bytes from its place, 0x40 bytes into "
                                                   r"\.text, 4 past the next warm row's place in config/ova/order.txt; "
                                                   'make dc-order gives it another place$'):
            ld.whole(images, gcclink.rooted(images), self.out, [])

    def test_a_record_s_place_off_its_alignment_under_the_windows_or_out_of_row_order_or_none_fails_the_link(self):
        end = 'make dc-order gives it another place'
        cases = {
            'off its alignment': ([('a', 0x42), ('b', 0xa0)],
                                  r"ova: config/ova/order.txt places \S+\.o's \.text\.a 0x42 bytes into \.text, off "
                                  f'the 4 bytes it is aligned to; {end}'),
            "under the windows' end": ([('a', 0x3c), ('b', 0xa0)],
                                       r"ova: config/ova/order.txt places \S+\.o's \.text\.a 0x3c bytes into \.text, "
                                       f'under the end of its windows, 0x40 bytes in; {end}'),
            'out of row order': ([('a', 0xa0), ('b', 0x40)],
                                 r"ova: config/ova/order.txt places \S+\.o's \.text\.b 0x40 bytes into \.text, ahead "
                                 r"of \S+\.o's \.text\.a, the warm row before it; make dc-order writes the warm rows "
                                 "in their places' order"),
            'none': ([('a', 0x40)], r"ova: config/ova/order.txt gives \S+\.o's \.text\.b, a warm row's section, no "
                                    'place; make dc-order gives it one'),
        }
        for case, (places, problem) in cases.items():
            with self.subTest(case):
                self.refused(self.image(places, a=36, b=12), problem)

    def test_a_record_s_place_under_the_kernels_end_fails_the_link_of_an_image_without_windows(self):
        images = self.image([('a', 4), ('b', 0xa0)], a=36, b=12)
        self.refused({**images, 'ova': images['ova']._replace(windows=[])},
                     r"ova: config/ova/order.txt places \S+\.o's \.text\.a 0x4 bytes into \.text, under the end of its "
                     'kernels, 0x8 bytes in; make dc-order gives it another place')

    def test_holds_the_kernels_to_the_record_exactly_and_each_warm_row_to_its_place(self):
        images = self.image([('a', 0x40), ('b', 0xa0)], a=36, b=12)
        image, passed = images['ova'], self.layouts(images)['ova']
        self.assertEqual(ld.stale(image, passed), [])
        path, again = f'{self.out}.o', 'make dc-order records the rows again'
        small, a, b = image.searched
        cases = {
            'a kernel row that grew': ([small._replace(size=4), a, b],
                                       "ova: small.o's .text.small lies 0x0 bytes into .text and takes 8, and "
                                       f'config/ova/order.txt has it 0x0 bytes in, taking 4; {again}'),
            'a kernel row the record has not': ([a, b], "ova: config/ova/order.txt doesn't hold small.o's .text.small, "
                                                        f'which its rows place; {again}'),
            # a warm row's size is its own: the record's is the size it had when the record was written
            'a warm row of another size': ([small, a._replace(size=4), b._replace(size=96)], None),
            'a warm row elsewhere': ([small, a, b._replace(place=0xc0)],
                                     f"ova: {path}'s .text.b lies 0xa0 bytes into .text, and config/ova/order.txt "
                                     f'places it 0xc0 bytes in; {again}'),
            'a row the record has and the image has not': (
                [small, a, b, gcclink.Searched('ova.o', '.text.gone', 0xc0, 4)],
                f"ova: config/ova/order.txt holds ova.o's .text.gone, which no kernel or warm row places; {again}"),
        }
        for case, (searched, problem) in cases.items():
            with self.subTest(case):
                self.assertEqual(ld.stale(image._replace(searched=searched), passed), [problem] if problem else [])


# the sources of the tests of a starts row. said.s holds a string and a constant, the sections ld merges, and
# said_more.s a longer string. both_read's pool lines, on sets 0 and 1 in a window from ova's origin, name both_row, for
# a data row of .data, and both_last, the last data. carried.s holds a section of GCC's intermediate code, as an object
# compiled with -flto does
SOURCES.update({
    'carried.s': '        .section .gnu.lto_carried,"e",@progbits\n        .long 0\n',
    'said.s': ('        .section .rodata.str1.4,"aMS",@progbits,1\n        .string "said"\n'
               '        .section .rodata.cst4,"aM",@progbits,4\n        .long 0x3f800000\n'),
    'said_more.s': ('        .section .rodata.str1.4,"aMS",@progbits,1\n'
                    '        .string "said, and more than a line of the cache holds"\n'
                    '        .section .rodata.cst4,"aM",@progbits,4\n        .long 0x3f800000\n'),
    'both.s': ('        .section .text.both,"ax",@progbits\n        .global _both_read\n_both_read:\n'
               '        mov.l   1f,r0\n        mov.l   2f,r1\n        rts\n        mov.l   @r0,r0\n'
               '        .p2align 2\n1:      .long   _both_row\n        .p2align 5\n2:      .long   _both_last\n'
               '        .section .data.both_row,"aw"\n        .p2align 2\n        .global _both_row\n'
               '        .size _both_row, 4\n_both_row:\n        .long 1\n'
               '        .section .data_last.both,"aw"\n        .p2align 2\n        .global _both_last\n'
               '        .size _both_last, 4\n_both_last:\n        .long 2\n'),
    # a common symbol of 16 bytes, which lies in no section of its object
    'common.s': '        .comm _main_common,16,4\n',
})


class Starts(Fixture):
    # where an image's data starts by its starts row, and what then stays as the image changes
    def test_a_starts_row_keeps_each_datum_of_its_output_sections_where_code_of_another_size_lies_ahead(self):
        # ova, whose name is no stage's, links small.o's 8 bytes of code or large.o's 40 ahead of ova_moved.o's
        # constant and table and late.o's .bss. With no row each lies after the code and moves with it. A row starts
        # each output section it names on the operand cache's 16 KB, where its data stays, and so does the data of
        # the output sections after it, which follow it as before
        names = ('ova_before', 'ova_table', 'main_late')
        cases = {(): set(), ('.bss',): {'main_late'}, ('.data',): {'ova_table', 'main_late'},
                 ('.bss', '.rodata'): set(names), ('.rodata', '.data', '.bss'): set(names)}
        for starts, kept in cases.items():
            with self.subTest(starts):
                placed = []
                for code in ('small', 'large'):
                    images = linking(self.images, 'ova', f'{code}.o', 'ova_moved.o', 'late.o')
                    layout = self.layouts({**images, 'ova': images['ova']._replace(starts=list(starts))})['ova']
                    exports = gcclink.exports(layout.elf)
                    placed.append({name: exports[name].value for name in names})
                    sections = {each.name: each.address for each in layout.elf.sections}
                    self.assertEqual([sections[output] % (ld.OCACHE_LINES * ld.OCACHE_LINE) for output in starts],
                                     [0] * len(starts))
                self.assertEqual({name for name in names if placed[0][name] == placed[1][name]}, kept)

    def test_a_pad_ahead_of_a_data_row_moves_no_datum_after_the_rows_of_an_output_section_a_starts_row_names(self):
        # a data row opens .bss with row.o's datum and .data with more.o's, and each pad moves its row's section on.
        # late.o's .bss and ova.o's table follow the rows, and move with the pads. Where the row names their output
        # section, the script starts on 16 KB again after its rows, and each stays a whole 16 KB past its start. The
        # starts take more than ova's load area, so it has a longer one here
        size = ld.OCACHE_LINES * ld.OCACHE_LINE
        data = [gcclink.Piece('data', 'row.o', '.bss.row'), gcclink.Piece('data', 'more.o', '.data')]
        cases = {(): set(), ('.bss',): {'main_late'}, ('.rodata', '.data', '.bss'): {'ova_table', 'main_late'}}
        for starts, kept in cases.items():
            with self.subTest(starts):
                placed = []
                for pads in ({}, {0: 0x20, 1: 0x40}, {0: 0x60}):
                    images = linking(self.images, 'ova', 'ova.o', 'late.o', 'more.o', 'row.o')
                    images = {**images, 'ova': images['ova']._replace(data=data, starts=list(starts), length=0x20000)}
                    layout = self.layouts(images, pads=pads)['ova']
                    exports = gcclink.exports(layout.elf)
                    sections = {each.name: each.address for each in layout.elf.sections}
                    self.assertEqual((exports['row_datum'].value - sections['.bss'],
                                      exports['main_more'].value - sections['.data']), (pads.get(0, 0), pads.get(1, 0)))
                    placed.append({name: exports[name].value - sections[output] for name, output in
                                   (('ova_table', '.data'), ('main_late', '.bss'))})
                self.assertEqual({name for name in placed[0] if len({each[name] for each in placed}) == 1}, kept)
                self.assertEqual({name: placed[0][name] for name in kept}, dict.fromkeys(kept, size))

    def test_a_started_data_s_rows_pads_move_no_last_data_so_the_last_data_takes_a_pad_of_its_own(self):
        # both_read's pool takes sets 0 and 1. both_row opens .data on set 0, and moves on two lines, to set 2. The
        # script starts again on 16 KB after it, so both_last, which closes .data a line past that start, after
        # ova.o's table, lies on set 1 with the row's pad or without it, and takes a line of its own. Reckoned as moved
        # by the row's pad, to set 3, it would take none, and the padded pass would fail on the pair left
        images = linking(self.images, 'ova', 'both.o', 'ova.o')
        images = {**images, 'ova': images['ova']._replace(
            kernel=[gcclink.Piece('draw', 'both.o', '.text.both')], windows=[gcclink.Window('draw', 2, 0)],
            data=[gcclink.Piece('data', 'both.o', '.data.both_row')], starts=['.data'])}
        _, layouts, finals = ld.coordinate(images, self.out)
        for linked in (layouts['ova'], finals['ova']):
            exports = gcclink.exports(linked.elf)
            self.assertEqual([exports[name].value % (ld.OCACHE_LINES * ld.OCACHE_LINE)
                              for name in ('both_row', 'both_last')], [2 * ld.OCACHE_LINE, 2 * ld.OCACHE_LINE])
        self.assertEqual(exports['both_last'].value - exports['both_row'].value, ld.OCACHE_LINES * ld.OCACHE_LINE)

    def test_a_string_a_change_adds_moves_no_other_datum_of_a_read_only_data_a_starts_row_names(self):
        # said.o links ahead of ova_moved.o, whose constant table follows its string and its constant in link order,
        # so said_more.o's longer string moves the table. Where the row names .rodata, the sections ld merges close
        # it, the constant and then the string, and neither the table nor the constant moves. cold.o's string stays
        # with its data, in .data
        placed = {}
        for starts in ((), ('.rodata',)):
            for said in ('said', 'said_more'):
                images = linking(self.images, 'ova', f'{said}.o', 'cold.o', 'ova_moved.o')
                image = images['ova']._replace(starts=list(starts), cold=['cold.o'])
                layout = self.layouts({**images, 'ova': image})['ova']
                found = {(each.file, each.section): each for each in layout.placements}
                table = gcclink.exports(layout.elf)['ova_before'].value
                constant, string = found[(f'{said}.o', '.rodata.cst4')], found[(f'{said}.o', '.rodata.str1.4')]
                self.assertEqual((constant.output, string.output, found[('cold.o', '.rodata.str1.4')].output),
                                 ('.rodata', '.rodata', '.data'))
                placed.setdefault(starts, []).append((table, constant.address, string.address))
        (table, constant, string), (later_table, later_constant, _) = placed[()]
        self.assertEqual((string < constant < table, later_table - table, later_constant - constant), (True, 40, 40))
        self.assertEqual(placed[('.rodata',)][0], placed[('.rodata',)][1])
        table, constant, string = placed[('.rodata',)][0]
        self.assertEqual((table % (ld.OCACHE_LINES * ld.OCACHE_LINE), table < constant < string), (0, True))

    def test_a_starts_row_starts_no_output_section_that_the_link_gives_no_byte(self):
        # ova.o holds a table, and no constant or .bss. libspare.a's one member holds .bss, and the link takes it only
        # where a root names its datum. The row names all three output sections, and starts .data, and .bss with the
        # member's datum alone: an output section without a byte lies where the one before it ends, so the image ends
        # with its last byte, not on the 16 KB after it
        size = ld.OCACHE_LINES * ld.OCACHE_LINE
        run('bin/sh-elf-ar', 'rc', 'libspare.a', 'spare.o', root=self.root)
        images = linking(self.images, 'ova', 'ova.o', 'libspare.a')
        images = {**images, 'ova': images['ova']._replace(starts=['.rodata', '.data', '.bss'])}
        rooted = gcclink.rooted(images)
        for roots, started in ((rooted, ['.data']), ({**rooted, 'ova': [*rooted['ova'], 'main_spare']},
                                                    ['.data', '.bss'])):
            with self.subTest(started):
                layout = self.layouts(images, roots)['ova']
                sections = {each.name: each for each in layout.elf.sections if each.name in ('.rodata', '.data', '.bss')}
                self.assertEqual([name for name, each in sections.items() if each.size], started)
                self.assertEqual([sections[name].address % size for name in started], [0] * len(started))
                symbols = {symbol.name: symbol.value for symbol in layout.elf.symbols}
                self.assertEqual(symbols['_image_end'], sections[started[-1]].address + sections[started[-1]].size)
                self.assertLess(symbols['__startof_C'], sections['.data'].address - size // 2)

    def test_a_starts_row_starts_the_output_section_each_input_s_bytes_go_to_as_the_link_places_them(self):
        # what gives an output section a byte, by the row that places it. A cold row's object closes .data with its
        # string and its .bss, so they start neither .rodata nor .bss. A common symbol lies in no section, and
        # starts .bss. A head row's section and a tail row's lie in .head and .tail, and start no .data. Each pass
        # fails where ld fills an output section otherwise than ld.py reckoned, so each link here holds the reckoning
        # to ld. The image ends with its last byte: an output section without one starts nowhere
        size = ld.OCACHE_LINES * ld.OCACHE_LINE
        cases = {'cold': (['cold.o'], {'cold': ['cold.o']}, ['.data']),
                 'common': (['common.o'], {}, ['.bss']),
                 'head': (['more.o'], {'heads': [gcclink.Piece('DHEAD', 'more.o', '.data')]}, []),
                 'tail': (['more.o'], {'tails': [gcclink.Piece('DMORE', 'more.o', '.data')]}, [])}
        for case, (inputs, rows, started) in cases.items():
            with self.subTest(case):
                images = linking(self.images, 'ova', *inputs)
                if case == 'head':
                    images = {**images, 'ova': images['ova']._replace(inputs=['more.o', 'head.o'])}
                images = {**images, 'ova': images['ova']._replace(starts=['.rodata', '.data', '.bss'], **rows)}
                layout = self.layouts(images)['ova']
                sections = {each.name: each for each in layout.elf.sections if each.name in gcclink.STARTS}
                self.assertEqual([name for name, each in sections.items() if each.size], started)
                self.assertEqual([sections[name].address % size for name in started], [0] * len(started))
                # the image ends with its last byte, a start's or no start's
                symbols = {symbol.name: symbol.value for symbol in layout.elf.symbols}
                if started:
                    self.assertEqual(symbols['_image_end'], sections[started[0]].address + sections[started[0]].size)
                else:
                    self.assertLess(symbols['_image_end'] - symbols['_image_start'], size // 2)

    def test_a_pass_fails_where_ld_fills_an_output_section_otherwise_than_ld_py_reckoned(self):
        # ova.o holds a table, and neither a constant nor .bss. A reckoning that gives .rodata a byte starts it for
        # nothing, and one that gives .data none leaves its table off the operand cache's 16 KB. The pass fails on
        # each, naming the output section and the reckoning
        images = linking(self.images, 'ova', 'ova.o')
        images = {**images, 'ova': images['ova']._replace(starts=['.rodata', '.data', '.bss'])}
        cases = {('.rodata', '.data'): "ova: ld gives its .rodata no byte, and ld.py reckoned that an input does, so "
                                       "its script starts it on the 16 KB of the SH-4's operand cache for nothing; "
                                       r"tools/ld.py's filled\(\) is at fault",
                 (): "ova: ld gives its .data a byte, and ld.py reckoned that no input does, so its script starts it "
                     "on no 16 KB of the SH-4's operand cache, as its starts row has it; "
                     r"tools/ld.py's filled\(\) is at fault"}
        for reckoned, problem in cases.items():
            with self.subTest(reckoned), mock.patch.object(ld, 'filled', return_value=(set(reckoned), set())):
                with self.assertRaisesRegex(SystemExit, f'{problem}\n.*ova: its layout pass failed$'):
                    self.layouts(images)
                self.assertFalse((self.out / 'ova/layout.elf').exists())

    def test_data_rows_that_are_all_of_an_output_section_a_starts_row_names_take_no_start_after_them(self):
        # row.o's datum is all of ova's .bss, and a data row places it. Nothing follows the row for its pad to move,
        # so the script starts on 16 KB once, ahead of it, and the image ends with the datum. late.o's .bss follows
        # the row a whole 16 KB on
        size = ld.OCACHE_LINES * ld.OCACHE_LINE
        for inputs, ends in ((['ova.o', 'row.o'], 4), (['ova.o', 'late.o', 'row.o'], size + 16)):
            with self.subTest(inputs):
                images = linking(self.images, 'ova', *inputs)
                images = {**images, 'ova': images['ova']._replace(
                    data=[gcclink.Piece('data', 'row.o', '.bss.row')], starts=['.bss'], length=0x20000)}
                layout = self.layouts(images)['ova']
                bss = next(each for each in layout.elf.sections if each.name == '.bss')
                symbols = {symbol.name: symbol.value for symbol in layout.elf.symbols}
                self.assertEqual((bss.address % size, symbols['_row_datum'], symbols['_image_end']),
                                 (0, bss.address, bss.address + ends))

    def test_an_image_whose_starts_take_it_past_its_load_area_fails_naming_its_room_and_the_rows_that_fit_it(self):
        # ova's code, constant, table and .bss fit each load area here, and its starts take it past. The failure gives
        # what the starts take, the ends of the two links apart, and what the load area has for them, past the end of
        # the link without the row, which ld.py links to learn it. Then it names each row of fewer starts that fits,
        # as ld.py links each to learn, but one that a longer row that fits holds, and the packed row. In the padded
        # case row_datum, a data row's, takes a pad of a line in the link without the row, as in any link of the
        # image without it, and the room is what lies past the padded link's end
        plain = linking(self.images, 'ova', 'ova_moved.o', 'late.o')
        padded = linking(self.images, 'ova', 'row.o', 'ova.o', 'late.o')
        padded = {**padded, 'ova': padded['ova']._replace(
            kernel=[gcclink.Piece('draw', 'row.o', '.text.row')], windows=[gcclink.Window('draw', 1, 0)],
            data=[gcclink.Piece('data', 'row.o', '.bss.row')])}
        every = ['.rodata', '.data', '.bss']
        cases = {'singles': (plain, 0x8000, every, 'a row that names fewer fits, starts .rodata or starts .data or '
                                                  'starts .bss, and a packed row starts none of them'),
                 'pairs': (plain, 0xC000, every, 'a row that names fewer fits, starts .rodata .data or starts .rodata '
                                                 '.bss or starts .data .bss, and a packed row starts none of them'),
                 'none': (plain, 0x1000, every, 'no starts row that names fewer fits, and a packed row starts none of '
                                                'them'),
                 'one': (plain, 0x1000, ['.data'], 'a packed row starts none of them'),
                 'padded': (padded, 0xA000, ['.data', '.bss'], 'a row that names fewer fits, starts .data or starts '
                                                               '.bss, and a packed row starts none of them')}
        for case, (images, length, starts, rows) in cases.items():
            with self.subTest(case):
                image = images['ova']._replace(length=length)
                packed = self.layouts({**images, 'ova': image})['ova']
                end = next(symbol.value for symbol in packed.elf.symbols if symbol.name == '_image_end')
                left = image.origin + image.length - end
                with self.assertRaises(SystemExit) as failed:
                    self.layouts({**images, 'ova': image._replace(starts=starts)})
                lines = str(failed.exception).split('\n')
                past = int(re.fullmatch(r".*: ova: region `area' overflowed by (\d+) bytes", lines[-3])[1])
                self.assertEqual((left > 0, past > 0), (True, True))
                named = ' and '.join(filter(None, (', '.join(starts[:-1]), starts[-1])))
                self.assertRegex(lines[-2], f": ova: the starts of its {named} on the 16 KB of the SH-4's operand "
                                            f'cache take {left + past} bytes, and its load area has {left} for them; '
                                            f'in config/ova/kernel.txt, {rows}$')
                self.assertRegex(lines[-1], ': ova: its layout pass failed$')
                self.assertEqual(sorted(path.name for path in (self.out / 'ova').glob('[pn]a*')),
                                 ['packed.ld', 'packed.map'])
                self.assertEqual('. += 0x20;' in (self.out / 'ova/packed.ld').read_text(), case == 'padded')
                self.assertNotIn('ALIGN(0x4000)',
                                 (self.out / 'ova/packed.ld').read_text().split('.data :')[1].split('}')[0])
                # each row the failure names links in the load area, and no longer one does
                for row in re.findall(r'starts ((?:\.\w+ ?)+)', rows):
                    self.layouts({**images, 'ova': image._replace(starts=row.split())})

    def test_lifts_a_load_area_to_the_end_of_ram_in_the_view_its_origin_lies_in(self):
        # the area a link that must not fail for room links in: the room failure's trials, and tools/order.py's pass
        image = self.images['ova']
        for origin, length in ((0x0c900000, 0x00700000), (0x8c010000, 0x00ff0000), (0x8c000000, 0x01000000),
                               (0x8cfffff0, 0x00000010)):
            with self.subTest(hex(origin)):
                self.assertEqual(ld.lifted(image._replace(origin=origin, length=16)),
                                 image._replace(origin=origin, length=length))

    def test_an_image_that_runs_past_its_load_area_packed_fails_naming_no_row_and_each_section_on_16_kb(self):
        # pool_bss.o's .bss.pool lies on 16 KB by its own alignment, after late.o's .bss, so ld skips to it under any
        # row. ova's load area ends short of it. With no starts row the failure names the section and the bytes
        # skipped, as the map has them. With one it says first that the image runs past with none of its starts too,
        # by how many bytes, so that no packed row is tried for nothing. ld skips to absorb.o's code and to its datum
        # too, which lie on 64, and the failure leaves them out
        size = ld.OCACHE_LINES * ld.OCACHE_LINE
        images = linking(self.images, 'ova', 'ova.o', 'absorb.o', 'late.o', 'pool_bss.o')
        long = self.layouts({**images, 'ova': images['ova']._replace(length=0x100000)})['ova']
        symbols = {symbol.name: symbol.value for symbol in long.elf.symbols}
        skip = symbols['_pool_zero'] - symbols['_main_late'] - 16
        self.assertEqual((symbols['_pool_zero'] % size, 0 < skip < size), (0, True))
        runs = symbols['_image_end'] - images['ova'].origin - 0x6000
        wide = (f": ova: pool_bss.o's .bss.pool starts on the 16 KB of the SH-4's operand cache by its own alignment, "
                f'which no row packs, and ld skips {skip} bytes to it$')
        none = (f': ova: with none of its starts it runs {runs} bytes past its load area, so no packed row in '
                'config/ova/kernel.txt gives it room$')
        for starts, problems in (([], [wide]), (['.rodata', '.data'], [none, wide])):
            with self.subTest(starts):
                with self.assertRaises(SystemExit) as failed:
                    self.layouts({**images, 'ova': images['ova']._replace(length=0x6000, starts=starts)})
                lines = str(failed.exception).split('\n')
                self.assertRegex(lines[-len(problems) - 2], r": ova: region `area' overflowed by \d+ bytes$")
                for line, problem in zip(lines[-len(problems) - 1:-1], problems):
                    self.assertRegex(line, problem)
                self.assertNotIn('absorb.o', str(failed.exception))
                self.assertEqual(ld.skipped((self.out / 'ova/packed.map').read_text()),
                                 [('absorb.o', '.text.absorb', 56), ('absorb.o', '.bss.absorb', 60),
                                  ('pool_bss.o', '.bss.pool', skip)])
                self.assertRegex(lines[-1], ': ova: its layout pass failed$')
        # an image with neither a row nor such a section links no second time
        for left in (self.out / 'ova').glob('packed.*'):
            left.unlink()
        with self.assertRaises(SystemExit) as failed:
            self.layouts({**images, 'ova': images['ova']._replace(length=0x10, inputs=['head.o', 'ova.o'])})
        self.assertRegex(str(failed.exception).split('\n')[-2], r": ova: region `area' overflowed by \d+ bytes$")
        self.assertEqual(list((self.out / 'ova').glob('packed.*')), [])

    def test_the_link_time_compile_links_by_the_layout_pass_s_script_without_its_starts(self):
        # carried.o carries intermediate code, so ova takes the link-time compile. A driver that compiles nothing
        # stands in for GCC's. whole.ld, the script it links by, places ova's code as the layout pass's does and
        # starts none of its data, so an image without room for its starts fails in that pass, which names the room
        images = linking(self.images, 'ova', 'ova_moved.o', 'late.o', 'carried.o')
        images = {**images, 'ova': images['ova']._replace(starts=['.rodata', '.data', '.bss'])}
        roots = gcclink.rooted(images)
        with tempfile.TemporaryDirectory() as stood:
            driver = Path(stood, 'bin/sh-elf-gcc')
            driver.parent.mkdir()
            driver.write_text('#!/bin/sh\nexit 0\n')
            driver.chmod(0o755)
            with mock.patch.object(gcclink, 'SH_ELF', Path(stood)):
                linked = ld.whole(images, roots, self.out, [])
        self.assertEqual((linked['ova'].inputs, linked['ova'].starts),
                         (['head.o', 'ova_moved.o', 'late.o'], ['.rodata', '.data', '.bss']))
        whole = (self.out / 'ova/lto/whole.ld').read_text()
        layout = ld.script_text(images['ova'], roots['ova'], self.out / 'ova/lto/provisional.ld', 'link-time', {})
        self.assertEqual((whole.count('ALIGN(0x4000)'), layout.count('ALIGN(0x4000)')), (0, 3))
        self.assertEqual(whole.split('  .rodata :')[0], layout.split('  .rodata :')[0])


if __name__ == '__main__':
    unittest.main()
