#!/usr/bin/env python3
"""The native slice's passes: asm2c.py, run on small SH-4 sources whose C is
compiled and called, marks.py, on small sources of C and on the tree's own,
and refs.py, on a small program clang optimizes and on lines of IR;
programs.py, on small programs' objects; stubs.py, on the names an object
needs and on the tree's table of them; coff.py, on two objects clang compiles
for Windows; census.py, on small trees of sources under a compiler that says
nothing; the shim's event aliases and its texture records, on textures a
driver loads; gdfs.c, on a directory of files for a disc; card.c, on cards
it writes and cards it is given; settings.c, on the directories an
environment names; threads.c, under the game's own thread.c,
on threads of the test's; symbols.c, on ELF files it writes; where the slice
finds a static of the game's by name; and the runner's end of a run whose
program lacks one. Ninja's motions, its camera's and a light's setters run as
the slice builds them, on a tree, a camera, a light and keys of a driver's.
Needs a host C compiler, and no disc; refs.py's and coff.py's tests need
clang, and asm2c.py's the objects of play math that make
engine-native-tools names in PLAY_MATH. The motions' need both, and a clang
that compiles for x86."""
import contextlib
import errno
import hashlib
import io
import math
import os
import random
import re
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'engine/tools'))
import coff  # noqa: E402
import marks  # noqa: E402
import programs  # noqa: E402
import refs  # noqa: E402
import stubs  # noqa: E402

SOURCE = """
; int first(void)
_first:
        MOVA        ?table,R0
        RTS
        MOV.L       @R0,R0
        .ALIGN      4
?table:
        .DATA.L     H'11

; int second(void)
_second:
        MOVA        ?table,R0
        RTS
        MOV.L       @R0,R0
        .ALIGN      4
?table:
        .DATA.L     H'22

; int slot_runs_first(int n)
_slot_runs_first:
        MOV         R4,R0
        BRA         ?done
        ADD         #1,R0
        ADD         #100,R0
?done:
        RTS
        NOP

; int test_before_slot(int n)
_test_before_slot:
        TST         R4,R4
        BT/S        ?zero
        CLRT
        MOV         #5,R0
        RTS
        NOP
?zero:
        RTS
        MOV         #7,R0

; int signed_byte(void)
_signed_byte:
        RTS
        MOV         #-2,R0

; int calls(int n)
_calls:
        STS.L       PR,@-R15
        BSR         _slot_runs_first
        NOP
        LDS.L       @R15+,PR
        RTS
        ADD         #1,R0

; int pick(int n)
_pick:
        SHLL        R4
        MOVA        ?cases,R0
        MOV.W       @(R0,R4),R1
        BRAF        R1
        NOP
?base:
        RTS
        MOV         #0,R0
?one:
        RTS
        MOV         #10,R0
?two:
        RTS
        MOV         #20,R0
        .ALIGN      4
?cases:
        .DATA.W     ?one-?base,?two-?base

; Float add_mul(Float a, Float b)
_add_mul:
        FMOV.S      FR4,FR0
        FMUL        FR5,FR0
        RTS
        FADD        FR4,FR0

; int back_bank(int *words)
_back_bank:
        FSCHG
        FMOV.S      @R4+,FR1
        FSCHG
        FSCHG
        FMOV        XD0,DR2
        FSCHG
        FMOV.S      FR3,FR0
        FTRC        FR0,FPUL
        RTS
        STS         FPUL,R0

; Float dot(NJS_VECTOR *a)
_dot:
        FMOV.S      @R4+,FR0
        FMOV.S      @R4+,FR1
        FMOV.S      @R4+,FR2
        FLDI0       FR3
        FIPR        FV0,FV0
        RTS
        FMOV.S      FR3,FR0

; int fifth(int a, int b, int c, int d, int e)
_fifth:
        MOV.L       @R15,R0
        RTS
        ADD         R7,R0

; int based(int a, int b)
_based:
        LDC         R4,GBR
        BRA         ?on
        LDC         R5,GBR
?on:
        RTS
        STC         GBR,R0
"""

INLINE = """
#pragma inline_asm(twice)
static Float twice(Float x)
{
        FMOV.S      FR4,FR0
        FADD        FR4,FR0
}
"""

DRIVER = r"""
#include <stdint.h>
#include <stdio.h>
void native_asm_init(void);
uint64_t first(void);
uint64_t second(void);
uint64_t slot_runs_first(uint64_t);
uint64_t test_before_slot(uint64_t);
uint64_t signed_byte(void);
uint64_t calls(uint64_t);
uint64_t pick(uint64_t);
uint64_t back_bank(uint64_t);
float add_mul(float, float);
float dot(uint64_t);
uint64_t fifth(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
uint64_t based(uint64_t, uint64_t);
float twice(float);
static float vector[3] = {1.0F, 2.0F, 2.0F};
static float words[2] = {3.0F, 4.0F};

int main(void)
{
    native_asm_init();
    printf("%x %x %d %d %d %d %d %d %d %g %g %d %d %d %g\n", (unsigned)first(), (unsigned)second(),
           (int)slot_runs_first(3), (int)test_before_slot(0), (int)test_before_slot(1), (int)(int32_t)signed_byte(),
           (int)calls(3), (int)pick(0), (int)pick(1), add_mul(2.0F, 3.0F), dot((uint64_t)(uintptr_t)vector),
           (int)back_bank((uint64_t)(uintptr_t)words), (int)fifth(1, 2, 3, 40, 500), (int)based(8, 9), twice(3.5F));
    return 0;
}
"""

# branches through a register, and low, high and next, which they reach. the
# slot of the first branch in each of jsr_known to jsr_c, and of pick_slot's,
# writes the branch's own register
JUMPS = """
; int low(void)
_low:
        RTS
        MOV         #1,R0

; int high(void)
_high:
        RTS
        MOV         #2,R0

; int next(int n)
_next:
        MOV         R4,R0
        RTS
        ADD         #1,R0

; int jsr_known(void)
_jsr_known:
        STS.L       PR,@-R15
        MOV.L       ?high,R2
        MOV.L       #_low,R1
        JSR         @R1
        MOV         R2,R1
        MOV         R0,R2
        JSR         @R1
        SHLL2       R2
        LDS.L       @R15+,PR
        RTS
        ADD         R2,R0
        .ALIGN      4
?high:
        .DATA.L     _high

; int jsr_register(void)
_jsr_register:
        STS.L       PR,@-R15
        MOV.L       ?low,R1
        MOV.L       ?high,R2
        JSR         @R1
        MOV         R2,R1
        MOV         R0,R2
        JSR         @R1
        SHLL2       R2
        LDS.L       @R15+,PR
        RTS
        ADD         R2,R0
        .ALIGN      4
?low:
        .DATA.L     _low
?high:
        .DATA.L     _high

; int jmp_known(void)
_jmp_known:
        MOV.L       ?high,R2
        MOV.L       #_low,R1
        JMP         @R1
        MOV         R2,R1
        .ALIGN      4
?high:
        .DATA.L     _high

; int jmp_register(void)
_jmp_register:
        MOV.L       ?low,R1
        MOV.L       ?high,R2
        JMP         @R1
        MOV         R2,R1
        .ALIGN      4
?low:
        .DATA.L     _low
?high:
        .DATA.L     _high

; int jmp_pops(void)
_jmp_pops:
        MOV.L       ?high,R1
        MOV.L       R1,@-R15
        MOV.L       ?low,R0
        JMP         @R0
        MOV.L       @R15+,R0
        .ALIGN      4
?low:
        .DATA.L     _low
?high:
        .DATA.L     _high

; Float jsr_c(Float x)
_jsr_c:
        STS.L       PR,@-R15
        MOV.L       ?acosf,R2
        MOV.L       #_asinf,R1
        JSR         @R1
        MOV         R2,R1
        LDS.L       @R15+,PR
        RTS
        NOP
        .ALIGN      4
?acosf:
        .DATA.L     _acosf

; int slots_pass(void)
_slots_pass:
        STS.L       PR,@-R15
        MOV.L       #_next,R1
        JSR         @R1
        MOV         #3,R4
        MOV.L       ?next,R1
        JSR         @R1
        MOV         R0,R4
        LDS.L       @R15+,PR
        MOV.L       #_next,R1
        JMP         @R1
        MOV         R0,R4
        .ALIGN      4
?next:
        .DATA.L     _next

; int jmp_passes(int n)
_jmp_passes:
        MOV.L       ?next,R1
        JMP         @R1
        ADD         #1,R4
        .ALIGN      4
?next:
        .DATA.L     _next

; int pick_slot(int n)
_pick_slot:
        SHLL        R4
        MOVA        ?cases,R0
        MOV.W       @(R0,R4),R1
        BRAF        R1
        MOV         #0,R1
?base:
        RTS
        MOV         #0,R0
?one:
        RTS
        MOV         #30,R0
?two:
        RTS
        MOV         #40,R0
        .ALIGN      4
?cases:
        .DATA.W     ?one-?base,?two-?base
"""

JUMPS_DRIVER = r"""
#include <stdint.h>
#include <stdio.h>
void native_asm_init(void);
uint64_t jsr_known(void);
uint64_t jsr_register(void);
uint64_t jmp_known(void);
uint64_t jmp_register(void);
uint64_t jmp_pops(void);
float jsr_c(float);
uint64_t slots_pass(void);
uint64_t jmp_passes(uint64_t);
uint64_t pick_slot(uint64_t);
/* the game's C that jsr_c names, each with an answer of its own */
float asinf(float x) { return x + 0.5F; }
float acosf(float x) { return x + 0.25F; }

int main(void)
{
    native_asm_init();
    printf("%d %d %d %d %d %g %d %d %d\n", (int)jsr_known(), (int)jsr_register(), (int)jmp_known(),
           (int)jmp_register(), (int)jmp_pops(), jsr_c(1.0F), (int)slots_pass(), (int)jmp_passes(7),
           (int)pick_slot(1));
    return 0;
}
"""

# branches through R1 past a call. names, which a BSR or a JSR reaches, leaves
# R1 high's address, and keeps leaves R1 as it was. each caller keeps PR in R3,
# which neither writes, so the source reads and writes no memory
RETURNS = """
; int low(void)
_low:
        RTS
        MOV         #1,R0

; int high(void)
_high:
        RTS
        MOV         #2,R0

_names:
        RTS
        MOV.L       #_high,R1

_keeps:
        RTS
        NOP

; int bsr_names(void)
_bsr_names:
        STS         PR,R3
        MOV.L       #_low,R1
        BSR         _names
        NOP
        LDS         R3,PR
        JMP         @R1
        NOP

; int jsr_names(void)
_jsr_names:
        STS         PR,R3
        MOV.L       #_low,R1
        MOV.L       #_names,R2
        JSR         @R2
        NOP
        LDS         R3,PR
        JMP         @R1
        NOP

; int slot_names(void)
_slot_names:
        STS         PR,R3
        BSR         _names
        MOV.L       #_low,R1
        LDS         R3,PR
        JMP         @R1
        NOP

; int bsr_keeps(void)
_bsr_keeps:
        STS         PR,R3
        MOV.L       #_low,R1
        BSR         _keeps
        NOP
        LDS         R3,PR
        JMP         @R1
        NOP

; int jsr_after(void)
_jsr_after:
        STS         PR,R3
        MOV.L       #_low,R1
        BSR         _names
        NOP
        JSR         @R1
        NOP
        LDS         R3,PR
        RTS
        ADD         #10,R0
"""

RETURNS_DRIVER = r"""
#include <stdint.h>
#include <stdio.h>
void native_asm_init(void);
uint64_t bsr_names(void);
uint64_t jsr_names(void);
uint64_t slot_names(void);
uint64_t bsr_keeps(void);
uint64_t jsr_after(void);

int main(void)
{
    uint64_t (*const each[])(void) = {bsr_names, jsr_names, slot_names, bsr_keeps, jsr_after};

    native_asm_init();
    for (unsigned at = 0; at < sizeof each / sizeof *each; at++) {
        printf("%d ", (int)each[at]());
    }
    printf("\n");
    return 0;
}
"""


class Asm2c(unittest.TestCase):
    def printed(self, driver, source, inline=''):
        """what driver prints, linked with asm2c.py's C of source and inline"""
        cc = os.environ.get('CC', 'cc')
        # the C runs on play math: the host's objects of its reference, which
        # make builds and names in PLAY_MATH, as engine/mk/play_math.mk names
        # them
        math = os.environ.get('PLAY_MATH', '').split()
        self.assertTrue(math, "engine/tests/native_tools.py: PLAY_MATH names no object of play math's reference, "
                              "which asm2c.py's C links: run make engine-native-tools, which builds them and names "
                              'them')
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)
            (path / 'small.src').write_text(source)
            (path / 'inline.c').write_text(inline)
            (path / 'driver.c').write_text(driver)
            subprocess.run([sys.executable, ROOT / 'engine/tools/asm2c.py', path / 'small.c', path / 'small.src',
                            path / 'inline.c'], check=True, capture_output=True)
            subprocess.run([cc, '-std=gnu17', '-no-pie', '-w', '-I', ROOT / 'engine/native', '-I',
                            ROOT / 'engine/numeric', '-o', path / 'small', path / 'driver.c', path / 'small.c',
                            *(ROOT / name for name in math)], check=True)
            run = subprocess.run([path / 'small'], check=True, capture_output=True, text=True)
        return run.stdout.split()

    def test_a_source_runs_as_it_reads(self):
        # each ? label is its own function's; a slot runs before its branch
        # and after a BT/S's test; a byte's immediate sign-extends; a BRAF
        # goes through a table of differences; under FSCHG an odd FR is the
        # back bank's pair; a fifth integer comes at @R15; a slot loads and
        # stores GBR as any line does; and a static inline body's Float comes
        # back in FR0
        self.assertEqual(self.printed(DRIVER, SOURCE, INLINE),
                         ['11', '22', '4', '7', '5', '-2', '5', '10', '20', '8', '9', '4', '540', '9', '7'])

    def test_a_branch_through_a_register_goes_where_it_pointed_before_the_slot(self):
        # a JSR and a JMP whose slot writes their register reach what it held
        # before: a function of the source or the game's C that a MOV.L named,
        # or an address a table gave. the register holds the slot's value
        # after, and no name, for the next JSR through it. a slot that writes
        # an argument passes it. a BRAF counts by its register as it was
        self.assertEqual(self.printed(JUMPS_DRIVER, JUMPS),
                         ['6', '6', '1', '1', '1', '1.5', '6', '9', '40'])

    def write(self, source, *options):
        """asm2c.py's status, what it says and whether it wrote its C, for a .src of source's text"""
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)
            (path / 'small.src').write_text(source)
            given = [str(path / option) if option.startswith('small') else option for option in options]
            run = subprocess.run([sys.executable, ROOT / 'engine/tools/asm2c.py', path / 'small.c', *given],
                                 capture_output=True, text=True)
            return run.returncode, run.stdout + run.stderr, (path / 'small.c').exists()

    def written(self, source, *options, inline=None):
        """asm2c.py's C for a .src of source's text, or for the C file of inline's text, under options"""
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder)
            (path / 'small.src').write_text(source)
            given = [str(path / option) if option.startswith('small') else option
                     for option in options or ('small.src',)]
            if inline is not None:
                (path / 'inline.c').write_text(inline)
                given = [path / 'inline.c']
            subprocess.run([sys.executable, ROOT / 'engine/tools/asm2c.py', path / 'small.c', *given], check=True,
                           capture_output=True)
            return (path / 'small.c').read_text()

    def body(self, text, name):
        """the lines of name's function in asm2c.py's C, without their indent"""
        lines = re.search(rf'^static void f__{name}\(void\)\n{{\n(.*?)\n}}\n', text, re.M | re.S)[1]
        return '\n'.join(line[4:] for line in lines.split('\n'))

    def test_a_slot_follows_its_branch_across_a_line_that_holds_no_instruction(self):
        # a blank line or a comment between a branch and its slot is no line of the function: the slot runs with
        # its branch as it does beside it
        for name, before, after in (
                ('an RTS', ('MOV         #0,R0', 'RTS'), ('MOV         #1,R0',)),
                ('a BSR', ('STS         PR,R3', 'BSR         _first'), ('MOV         #3,R4', 'LDS         R3,PR',
                                                                        'RTS', 'NOP')),
                ('a BT/S', ('TST         R4,R4', 'BT/S        ?out'), ('MOV         #1,R0', 'MOV         #2,R0',
                                                                       '?out:', 'RTS', 'NOP'))):
            with self.subTest(name=name):
                bodies = [self.body(self.written(SOURCE + '\n; int probe(int x)\n_probe:\n' + ''.join(
                    line + '\n' if line.startswith('?') or not line.strip() or line.startswith(';') else
                    f'        {line}\n' for line in (*before, *between, *after))), 'probe')
                    for between in ((), ('', '; what the slot gives', ''))]
                self.assertEqual(bodies[0], bodies[1])
        self.assertEqual(self.body(self.written(
            SOURCE + '\n; int probe(void)\n_probe:\n        RTS\n\n        MOV         #1,R0\n'), 'probe'),
            'R[0] = (uint32_t)(int32_t)(int8_t)1; return;')

    def test_a_slot_under_a_label_runs_with_its_branch_and_again_under_the_label(self):
        # a label alone between a branch and its slot names the slot. past an RTS or a JMP the slot is written
        # twice: with the branch, and under its label for a branch that lands there. a call and a conditional
        # branch run on from their slot, so there the build ends
        def probe(*lines):
            return SOURCE + '\n; int probe(int x)\n_probe:\n' + ''.join(
                line + '\n' if line.startswith('?') else f'        {line}\n' for line in lines)

        slot = 'R[0] = (uint32_t)(int32_t)(int8_t)1;'
        self.assertEqual(self.body(self.written(probe('RTS', '?again:', 'MOV         #1,R0')), 'probe').split('\n'),
                         [f'{slot} return;', 'L__probe__again:;', slot])
        text = self.written(probe('MOV.L       #_first,R1', 'JMP         @R1', '?again:', 'MOV         #1,R0'))
        self.assertEqual(self.body(text, 'probe').split('\n')[1:], [f'{slot} f__first(); return;', 'L__probe__again:;',
                                                                    slot])
        for lines, problem in (
                (('BSR         _first', '?again:', 'NOP', 'RTS', 'NOP'), 'a label on the delay slot of a BSR'),
                (('JSR         @R1', '?again:', 'NOP', 'RTS', 'NOP'), 'a label on the delay slot of a JSR'),
                (('BT/S        ?again', '?again:', 'NOP', 'RTS', 'NOP'), 'a label on a conditional delay slot'),
                (('BF/S        ?again', '?again: NOP', 'RTS', 'NOP'), 'a label on a conditional delay slot'),
                (('MOV         #1,R0', 'RTS'), 'no delay slot after its RTS')):
            with self.subTest(lines=lines):
                status, said, wrote = self.write(probe(*lines), 'small.src')
                self.assertEqual((status, wrote), (1, False), said)
                self.assertIn(f'asm2c: small: _probe: {problem}\n', said)

    def test_an_add_s_and_a_compare_s_byte_extends_by_its_sign_and_a_logical_one_s_does_not(self):
        # the SH-4 extends the byte of ADD #imm and CMP/EQ #imm by its sign, so H'FF there is -1, and the byte of
        # AND, OR, XOR and TST #imm by zeros
        text = self.written(SOURCE + ''.join(f'        {line}\n' for line in (
            "\n; int probe(int x)\n_probe:", "ADD         #H'FF,R0", "ADD         #H'7F,R0", 'ADD         #-1,R0',
            "ADD         #H'80,R0", "CMP/EQ      #H'FF,R0", "CMP/EQ      #H'7F,R0", "AND         #H'FF,R0",
            "OR          #H'80,R0", "XOR         #H'FF,R0", "TST         #H'80,R0", 'RTS', 'NOP')).replace(
                '        \n; int probe(int x)\n_probe:', '\n; int probe(int x)\n_probe:'))
        self.assertEqual(self.body(text, 'probe').split('\n')[:-1], [
            'R[0] += (uint32_t)4294967295u;', 'R[0] += (uint32_t)127u;', 'R[0] += (uint32_t)4294967295u;',
            'R[0] += (uint32_t)4294967168u;', 'T = R[0] == (uint32_t)4294967295u;', 'T = R[0] == (uint32_t)127u;',
            'R[0] &= (uint32_t)255u;', 'R[0] |= (uint32_t)128u;', 'R[0] ^= (uint32_t)255u;',
            'T = (R[0] & 128u) == 0;'])

    def test_an_align_inside_a_run_of_data_leaves_the_bytes_up_to_its_boundary(self):
        # a datum after an .ALIGN in its run lies on the boundary, counted from the run's start, and so does its
        # label; a boundary wider than the 32 bytes an array starts on ends the build
        def probe(boundary):
            return (SOURCE + '\n; int probe(void)\n_probe:\n        MOV.L       ?wide,R0\n        RTS\n        NOP\n'
                    f"?narrow:\n        .DATA.B     1\n        .ALIGN      {boundary}\n"
                    "?wide:  .DATA.L     H'11223344\n        .ALIGN      4\n")

        text = self.written(probe(4))
        self.assertIn('static unsigned char d_small__probe__narrow[8] ', text)
        self.assertIn('    *(uint8_t *)(d_small__probe__narrow + 0) = 1u;\n'
                      '    *(uint32_t *)(d_small__probe__narrow + 4) = 287454020u;\n', text)
        self.assertIn('U32((uint32_t)(uintptr_t)((char *)d_small__probe__narrow + 4));', self.body(text, 'probe'))
        self.assertIn('    *(uint32_t *)(d_small__probe__narrow + 16) = 287454020u;\n', self.written(probe(16)))
        status, said, wrote = self.write(probe(64), 'small.src')
        self.assertEqual((status, wrote), (1, False), said)
        self.assertIn('asm2c: small: .ALIGN 64 inside a run of data, which starts on 32 bytes', said)

    def test_an_inline_body_s_table_of_differences_gives_its_braf_its_cases(self):
        # both readings of an inline body name its ? labels for the body, so the BRAF finds the table's labels:
        # with a label of its own in the body and with none
        for own in ('', '_stepped:\n'):
            with self.subTest(own=own):
                text = self.written('', inline=(
                    f'#pragma inline_asm(step)\nstatic int step(int x)\n{{\n{own}'
                    '        MOVA        ?cases,R0\n        MOV.B       @(R0,R4),R0\n        BRAF        R0\n'
                    '        NOP\n?cases:\n        .DATA.B     ?one-?cases\n        .DATA.B     ?two-?cases\n'
                    '?one:\n        MOV         #1,R0\n?two:\n        MOV         #2,R0\n}\n'))
                scope = 'stepped' if own else 'step'
                self.assertIn(f'switch (c_) {{ case 0: goto L__{scope}__one; case 1: goto L__{scope}__two; '
                              'default: abort(); }', text)

    def test_a_const_or_a_volatile_says_nothing_of_an_argument_s_or_a_result_s_kind(self):
        # a qualified float is a float, in FR4 on and back in FR0, and a pointer to one is a pointer
        text = self.written('', inline=(
            '#pragma inline_asm(same)\nstatic float same(const float x, const float *p, volatile Float y, '
            'float *const q)\n{\n        FMOV.S      FR4,FR0\n}\n'
            '#pragma inline_asm(held)\nstatic const float held(float const x)\n{\n        FMOV.S      FR4,FR0\n}\n'))
        self.assertIn('float same(float a0, uintptr_t a1, float a2, uintptr_t a3)\n{\n    F.f[4] = a0; '
                      'R[4] = narrow(a1); F.f[5] = a2; R[5] = narrow(a3); enter();', text)
        self.assertIn('float held(float a0)\n{\n    F.f[4] = a0; enter(); f__held(); leave(); return F.f[0];', text)

    def test_a_function_left_out_that_the_c_reaches_is_the_host_s(self):
        # outer calls inner and takes other's address. where a FILE.src's NAMEs leave them out, or --explore
        # does, each is defined as a call of the host's function of its name, which a JSR through a register
        # reaches too. what a function left out reached alone is not defined
        source = (SOURCE + '\n; int outer(void)\n_outer:\n        STS         PR,R3\n        BSR         _inner\n'
                  '        NOP\n        MOV.L       #_other,R1\n        JSR         @R1\n        NOP\n'
                  '        LDS         R3,PR\n        RTS\n        NOP\n\n; int inner(void)\n_inner:\n{trap}'
                  '        RTS\n        MOV         #7,R0\n\n; int other(void)\n_other:\n        RTS\n'
                  '        MOV         #8,R0\n')
        text = self.written(source.format(trap=''), 'small.src:outer')
        for name in ('inner', 'other'):
            with self.subTest(name=name):
                self.assertEqual(self.body(text, name), f'native_jsr((uint32_t)(uintptr_t)&e_{name});')
                self.assertIn(f'extern char e_{name} __asm__("{name}");\n', text)
                self.assertIn(f'    if (target == (uint32_t)(uintptr_t)f__{name}) {{ f__{name}(); return; }}\n', text)
        text = self.written(source.format(trap='        TRAPA       #1\n        BSR         _gone\n        NOP\n'),
                            'small.src', '--explore')
        self.assertEqual(self.body(text, 'inner'), 'native_jsr((uint32_t)(uintptr_t)&e_inner);')
        self.assertEqual(self.body(text, 'other'), 'R[0] = (uint32_t)(int32_t)(int8_t)8; return;')
        self.assertNotIn('gone', text)
        # with every function written, none is the host's
        self.assertNotIn('native_jsr((uint32_t)(uintptr_t)&e_', self.written(source.format(trap='')))

    def test_a_jsr_to_the_game_s_c_is_called_by_x86_64_s_system_v_convention_alone(self):
        # the call passes integers and floats in registers of their own, as those calls do: Windows' and another
        # processor's pass them another way, and there the JSR ends the run by name
        text = self.written(SOURCE)
        self.assertIn('\n#if defined(__x86_64__) && !defined(_WIN32)\n', text)
        self.assertNotIn('__i386__', text)

    def test_a_register_an_instruction_may_have_written_names_no_function(self):
        # a branch through the register a MOV.L named reaches the function itself. once another instruction has had
        # the register for its last operand, whatever its count of operands, the branch goes by the register's value
        for between, named in (('NOP', True), ('ADD         #4,R2', True), ('ADD         #4,R1', False),
                               ('STC         GBR,R1', False), ('SHLL2       R1', False), ('DT          R1', False),
                               ('MOVT        R1', False), ('ROTL        R1', False)):
            with self.subTest(between=between):
                text = self.written(JUMPS + f'\n; int through(void)\n_through:\n        MOV.L       #_low,R1\n'
                                            f'        {between}\n        JMP         @R1\n        NOP\n')
                body = re.search(r'^static void f__through\(void\)\n{\n(.*?)\n}\n', text, re.M | re.S)[1]
                self.assertEqual(('f__low();' in body, 'native_jsr(j_);' in body), (named, not named), body)

    def test_a_branch_through_a_register_goes_by_its_value_past_a_call(self):
        # the function a BSR or a JSR reaches may write any register. so past the call a branch through R1 goes
        # where R1 points then: to high where the function named it, though a MOV.L named low ahead of the call or
        # in its slot, and to low where the function left R1 alone
        self.assertEqual(self.printed(RETURNS_DRIVER, RETURNS), ['2', '2', '2', '1', '12'])

    def test_a_register_named_ahead_of_a_call_names_no_function_past_it(self):
        # a name a MOV.L gives past the call stands, and one it gave ahead of the call or in its slot doesn't
        for lines, named in ((('MOV.L       #_low,R1', 'BSR         _high', 'NOP'), False),
                             (('MOV.L       #_low,R1', 'JSR         @R2', 'NOP'), False),
                             (('BSR         _high', 'MOV.L       #_low,R1'), False),
                             (('BSR         _high', 'NOP', 'MOV.L       #_low,R1'), True)):
            with self.subTest(lines=lines):
                text = self.written(JUMPS + '\n; int through(void)\n_through:\n' +
                                    ''.join(f'        {line}\n' for line in (*lines, 'JMP         @R1', 'NOP')))
                body = re.search(r'^static void f__through\(void\)\n{\n(.*?)\n}\n', text, re.M | re.S)[1]
                self.assertEqual(('f__low();' in body, 'native_jsr(j_);' in body), (named, not named), body)

    def test_a_table_of_differences_holds_each_label_s_number_in_an_entry_s_own_bytes(self):
        # a byte's entry writes one byte, so the next entry's stands, and a long's fills its four
        for directive, kind, step in (('.DATA.B', 'uint8_t', 1), ('.DATA.W', 'uint16_t', 2),
                                      ('.DATA.L', 'uint32_t', 4)):
            with self.subTest(directive=directive):
                text = self.written(SOURCE.replace('.DATA.W     ?one-?base', f'{directive}     ?one-?base'))
                self.assertEqual(re.findall(r'\*\((\w+) \*\)\(d_small__pick__cases \+ (\d+)\) = \d+u;', text),
                                 [(kind, '0'), (kind, str(step))])

    def test_a_function_it_cant_write_ends_the_build_and_exploring_leaves_it_out(self):
        # TRAPA is no instruction asm2c.py knows, and third is no function of the source. LDC.L and STC.L step a
        # register, which leaving them out would lose, and an immediate that names a symbol has no number to write
        def holding(line):
            return SOURCE + f'\n; int trap(void)\n_trap:\n        {line}\n        RTS\n        NOP\n'

        for source, options, problem in (
                (holding('TRAPA       #1'), ('small.src',), 'small: _trap: TRAPA #1'),
                (holding('STC.L       GBR,@-R15'), ('small.src',), 'small: _trap: STC.L GBR,@-R15'),
                (holding('LDC.L       @R15+,GBR'), ('small.src',), 'small: _trap: LDC.L @R15+,GBR'),
                (holding('MOVA        #_first,R0'), ('small.src',), 'small: _trap: MOVA #_first'),
                (holding('TST         #_first,R0'), ('small.src',), 'small: _trap: TST #_first'),
                (holding('CMP/EQ      #_first,R0'), ('small.src',), 'small: _trap: CMP/EQ #_first'),
                (SOURCE, ('small.src:first,third',), 'small: _third: no function of the source')):
            with self.subTest(options=options):
                status, said, wrote = self.write(source, *options)
                self.assertEqual((status, wrote), (1, False), said)
                self.assertIn(f'asm2c: {problem}\n', said)
                status, said, wrote = self.write(source, *options, '--explore')
                self.assertEqual((status, wrote), (0, True), said)
                self.assertIn(f'asm2c: {problem}\n', said)
                self.assertIn('explored: 1 left out', said)
        self.assertEqual(self.write(SOURCE, 'small.src')[::2], (0, True))


@contextlib.contextmanager
def inside(folder):
    """the working directory at folder, as marks.py reads the tree from its root"""
    was = os.getcwd()
    os.chdir(folder)
    try:
        yield
    finally:
        os.chdir(was)


def tree(dest):
    """marks.tree() of the working directory's sources, saying nothing"""
    with contextlib.redirect_stdout(io.StringIO()):
        marks.tree(Path(dest))


def unit(folder, source, text):
    """what marks.py makes of a unit of text, as the compiler names source at its head"""
    path = Path(folder, 'unit.i')
    path.write_text(f'# 1 "{source}"\n{text}')
    Path(folder, 'overlaid.txt').write_text('')
    marks.preprocessed(Path(folder), path)
    return path.read_text()


@contextlib.contextmanager
def tables(**given):
    """marks.py's tables as given, and each of the rest empty, for a tree of a test's own sources"""
    empty = {'REACHED': (), 'SITES': {}, 'CONSOLE': {}, 'MARKED_SITES': {}, 'HOOKS': {},
             'LAYOUTS': {}, 'RECORDS': set(), 'FILES': set()}
    with contextlib.ExitStack() as stack:
        for name, value in {**empty, **given}.items():
            stack.enter_context(mock.patch.object(marks, name, value))
        yield


class Marks(unittest.TestCase):
    def test_the_tree_takes_every_rewrite_as_the_sources_stand(self):
        # each of SITES's texts as many times as its source holds it, each of CONSOLE's guards, and the backend's
        # names
        with tempfile.TemporaryDirectory() as d, inside(ROOT):
            tree(d)
            stage = Path(d, 'src/stage/stage.c').read_text(errors='surrogateescape')
            self.assertEqual(stage.count('        native_hook_stage_step();\n'), 2)
            pace = Path(d, 'src/system/pace.c').read_text(errors='surrogateescape')
            self.assertIn('#include "sa_pc.h"', pace)
            self.assertNotIn('SA_DC_ENGINE', pace)
            # the copy holds each source, and nothing a source doesn't stand for
            self.assertEqual({path.relative_to(d).as_posix() for root in marks.ROOTS
                              for path in Path(d, root).rglob('*.[ch]')},
                             {path.as_posix() for root in marks.ROOTS for path in Path(root).rglob('*.[ch]')})

    def test_a_site_ends_the_pass_where_its_source_holds_its_text_another_count_of_times(self):
        # a text the source spells another way, and one it repeats, where one hook would become two
        for text, count, passes in (('a();\n', (), True), ('b();\n', (), False), ('a();\na();\n', (), False),
                                    ('a();\na();\n', (2,), True), ('a();\n', (2,), False)):
            with self.subTest(text=text, count=count), tempfile.TemporaryDirectory() as d, inside(d), \
                    tables(SITES={'src/a.c': [('a();', 'hook();\na();', *count)]}):
                Path('src').mkdir()
                Path('src/a.c').write_text(text)
                if passes:
                    tree('out')
                    self.assertEqual(Path('out/src/a.c').read_text(), text.replace('a();', 'hook();\na();'))
                else:
                    with self.assertRaises(SystemExit) as failed:
                        tree('out')
                    self.assertIn('src/a.c: holds', str(failed.exception))

    def test_the_tree_ends_where_a_table_names_a_source_or_a_struct_the_tree_lacks(self):
        # a source that has moved leaves its rewrites, its guards and its records standing for nothing, and so does
        # a struct that has taken another name
        site = [('a();', 'hook();')]
        for given, problem in (({'SITES': {'src/moved.c': site}}, 'src/moved.c: SITES or CONSOLE'),
                               ({'CONSOLE': {'src/moved.c': {'a();': 1}}}, 'src/moved.c: SITES or CONSOLE'),
                               # outside the roots the pass copies, where it rewrites nothing
                               ({'SITES': {'sdk/a.c': site}}, 'sdk/a.c: SITES or CONSOLE'),
                               ({'MARKED_SITES': {'sdk/moved.h': site}}, 'sdk/moved.h: MARKED_SITES, HOOKS or LAYOUTS'),
                               ({'HOOKS': {'sdk/moved.c': site}}, 'sdk/moved.c: MARKED_SITES, HOOKS or LAYOUTS'),
                               ({'LAYOUTS': {'src/moved.h': {'struct a': 4}}},
                                'src/moved.h: MARKED_SITES, HOOKS or LAYOUTS'),
                               ({'RECORDS': {'record', 'renamed'}}, 'struct renamed: RECORDS or FILES'),
                               ({'FILES': {'renamed'}}, 'struct renamed: RECORDS or FILES'),
                               ({'SITES': {'src/a.c': site}, 'MARKED_SITES': {'sdk/a.c': site},
                                 'HOOKS': {'sdk/a.c': site}, 'LAYOUTS': {'src/a.c': {'struct record': 4}},
                                 'RECORDS': {'record'}}, None)):
            with self.subTest(given=given), tempfile.TemporaryDirectory() as d, inside(d), tables(**given):
                Path('src').mkdir()
                Path('sdk').mkdir()
                Path('src/a.c').write_text('struct record {\n    int n;\n};\na();\n')
                Path('sdk/a.c').write_text('a();\n')
                if problem is None:
                    tree('out')
                else:
                    with self.assertRaises(SystemExit) as failed:
                        tree('out')
                    self.assertIn(problem, str(failed.exception))
                    self.assertFalse(Path('out').exists())

    def test_the_tree_removes_the_copy_of_a_source_the_tree_holds_no_longer(self):
        with tempfile.TemporaryDirectory() as d, inside(d), tables():
            for path in ('src/a.c', 'src/old/a.h', 'modules/m/old.c'):
                Path(path).parent.mkdir(parents=True, exist_ok=True)
                Path(path).write_text('int a;\n')
            tree('out')
            # what the pass didn't write stays: an object beside the copies, and a file outside the roots
            Path('out/src/old/a.o').write_text('')
            Path('out/other.h').write_text('')
            Path('src/old/a.h').unlink()
            Path('modules/m/old.c').unlink()
            tree('out')
            self.assertEqual(sorted(path.relative_to('out').as_posix() for path in Path('out').rglob('*')
                                    if path.is_file()), ['other.h', 'overlaid.txt', 'src/a.c', 'src/old/a.o'])

    def test_the_tree_ends_where_a_source_names_the_dreamcasts_backend_another_way(self):
        for text, passes in (('#include "sa_dc.h"\nf(SA_DC_ENGINE);\n', True), ('/* sa_dc.h gives it */\n', True),
                             ('#include <sa_dc.h>\n', False), ('#include "../include/sa_dc.h"\n', False)):
            with self.subTest(text=text), tempfile.TemporaryDirectory() as d, inside(d), tables():
                Path('src').mkdir()
                Path('src/a.c').write_text(text)
                if passes:
                    tree('out')
                    self.assertNotIn('"sa_dc.h"', Path('out/src/a.c').read_text())
                else:
                    with self.assertRaises(SystemExit) as failed:
                        tree('out')
                    self.assertIn('src/a.c: names', str(failed.exception))

    def test_a_units_rewrite_stands_once_for_each_of_its_sources_the_unit_reads(self):
        # Ninja's easy drawer takes the adapter's name in the unit that defines it
        easy = 'void _njEasyDrawModel(NJS_MODEL *model)\n{\n}\n'
        with tempfile.TemporaryDirectory() as d:
            self.assertIn('void ninja_easy_draw_model(NJS_MODEL *model)\n{', unit(d, 'sdk/ninja/njModelE.c', easy))
            # as the tree's copy names a source, under the directory make gives it
            self.assertIn('void ninja_easy_draw_model(', unit(d, 'build/native/src/sdk/ninja/njModelE.c', easy))
            # a unit that reads none of a rewrite's sources holds none of its text
            self.assertEqual(unit(d, 'src/main.c', 'int main(void);\n'), '# 1 "src/main.c"\nint main(void);\n')

    def test_a_unit_ends_the_pass_where_it_holds_a_rewrites_text_another_count_of_times(self):
        easy = 'void _njEasyDrawModel(NJS_MODEL *model)\n{\n}\n'
        # the source spells its text another way, so the hook would go; it repeats it; and a unit that reads none
        # of the rewrite's sources holds its text
        for source, text in (('sdk/ninja/njModelE.c', easy.replace('*model', '*object')),
                             ('sdk/ninja/njModelE.c', easy + easy), ('sdk/ninja/other.c', easy)):
            with self.subTest(source=source, text=text), tempfile.TemporaryDirectory() as d:
                with self.assertRaises(SystemExit) as failed:
                    unit(d, source, text)
                self.assertIn('unit.i: holds', str(failed.exception))

    def test_a_wide_units_rewrites_follow_the_sources_it_reads(self):
        with tempfile.TemporaryDirectory() as d:
            # script.h's lines, which a unit that includes it holds once
            header = '# 1 "src/event/script.h" 1\nstruct script_run {\n    char **lines;\n    Sint32 flag;\n};\n'
            self.assertIn(f'    char *{marks.MARK} *lines;\n    Sint32 flag;', unit(d, 'src/a.c', header))
            with self.assertRaises(SystemExit):
                unit(d, 'src/a.c', header.replace('char **lines', 'char * *lines'))

    def test_a_tasks_variables_are_marked_as_task_h_spells_them_and_end_the_pass_spelled_another_way(self):
        # the union's pointers, which no struct's mark reaches: a member moved, renamed or given a comment would
        # leave the union the host's 24 bytes
        union = 'union task_vars {\n    Sint8 b[12];\n    Float f[3];\n    void *p[3];\n};\n'
        with tempfile.TemporaryDirectory() as d, mock.patch.object(marks, 'LAYOUTS', {}):
            self.assertIn(f'    void *{marks.MARK} p[3];\n}};', unit(d, 'src/system/task.h', union))
            for other in (union.replace('    Float f[3];\n    void *p[3];\n', '    void *p[3];\n    Float f[3];\n'),
                          union.replace('*p[3]', '*ptr[3]'), union.replace('*p[3];', '*p[3]; /* any pointer */')):
                with self.subTest(other=other), self.assertRaises(SystemExit) as failed:
                    unit(d, 'src/system/task.h', other)
                self.assertIn('unit.i: holds 0 of a text the pass rewrites 1 of', str(failed.exception))

    def test_a_unit_ends_with_the_bytes_of_each_record_of_the_sources_it_reads(self):
        with tempfile.TemporaryDirectory() as d, mock.patch.object(marks, 'MARKED_SITES', {}), \
                mock.patch.object(marks, 'LAYOUTS', {'src/a.h': {'struct a': 8, 'union b': 12}, 'src/c.h': {'C': 4}}):
            text = unit(d, 'build/native/src/src/a.h', 'int n;\n')
            self.assertEqual(text.splitlines()[2:], [
                '_Static_assert(sizeof(struct a) == 8, "struct a isn\'t the SH-4\'s 8 bytes, which marks.py\'s LAYOUTS '
                'gives it");',
                '_Static_assert(sizeof(union b) == 12, "union b isn\'t the SH-4\'s 12 bytes, which marks.py\'s LAYOUTS '
                'gives it");'])
            # as a unit names the source where the tree holds it too, and none for a unit that reads no such source
            self.assertEqual(unit(d, 'src/a.h', 'int n;\n'), text.replace('build/native/src/', ''))
            self.assertEqual(unit(d, 'src/b.h', 'int n;\n'), '# 1 "src/b.h"\nint n;\n')

    def test_the_adapters_units_end_with_the_bytes_of_ninjas_records_alone(self):
        # marks.py ninja marks Ninja's models and motions in a unit of the adapter's, which reads the game's
        # headers with their own records as the host lays them
        text = ('# 1 "sdk/include/njtypes.h" 1\ntypedef struct {\n    void *p[1];\n    unsigned nb[1];\n} NJS_MDATA1;\n'
                '# 1 "src/system/task.h" 1\nunion task_vars {\n    void *p[3];\n};\n')
        with tempfile.TemporaryDirectory() as d, mock.patch.object(sys, 'argv', ['marks.py', 'ninja', f'{d}/unit.i']):
            Path(d, 'unit.i').write_text(text)
            marks.main()
            made = Path(d, 'unit.i').read_text()
            self.assertIn(f'void *{marks.MARK} p[1];', made)
            self.assertIn('    void *p[3];\n};', made)
            asserted = re.findall(r'(?m)^_Static_assert\(sizeof\((.*?)\) == (\d+),', made)
            self.assertEqual(dict(asserted), {name: str(size)
                                              for name, size in marks.LAYOUTS['sdk/include/njtypes.h'].items()})

    def test_the_compiler_refuses_a_unit_whose_record_the_marks_leave_wide(self):
        if not shutil.which('clang'):
            self.skipTest('no clang')
        # a second pointer the union's rewrite doesn't name: its text stands once all the same, and the union is
        # 24 bytes on a 64-bit host
        union = 'union task_vars {\n    int l[3];\n    void *p[3];\n};\n'
        for text, passes in ((union, True), (union.replace('    int l[3];', '    void *q[3];'), False)):
            with self.subTest(text=text), tempfile.TemporaryDirectory() as d, \
                    mock.patch.object(marks, 'LAYOUTS', {'src/system/task.h': {'union task_vars': 12}}):
                unit(d, 'src/system/task.h', text)
                run = subprocess.run(['clang', '--target=x86_64-unknown-linux-gnu', '-std=gnu89', '-fms-extensions',
                                      '-fsyntax-only', '-x', 'c', f'{d}/unit.i'], capture_output=True, text=True)
                self.assertEqual(run.returncode == 0, passes, run.stderr)
                if not passes:
                    self.assertIn("union task_vars isn't the SH-4's 12 bytes", run.stderr)

    def test_each_record_the_pass_marks_by_name_has_its_bytes_but_a_players(self):
        # LAYOUTS gives the bytes of each of Ninja's models and motions, of each struct a data file holds and of
        # each record but a player's own: its union of each character's variables keeps the host's pointers, so
        # a 64-bit host's is wider than the SH-4's
        named = {record.split()[-1] for records in marks.LAYOUTS.values() for record in records}
        self.assertEqual((marks.RECORDS | marks.FILES | marks.NINJA) - named, {'player'})

    def test_each_rewrite_of_a_unit_names_a_source_the_tree_holds(self):
        for table in (marks.MARKED_SITES, marks.HOOKS, marks.LAYOUTS):
            for source in table:
                with self.subTest(source=source):
                    self.assertTrue((ROOT / source).is_file())

    def test_each_site_names_text_its_source_holds(self):
        # tree() ends the build at a site whose source holds its text another count of times than the site gives
        for path, pairs in marks.SITES.items():
            text = (ROOT / path).read_text(errors='surrogateescape')
            for old, new, *count in pairs:
                with self.subTest(path=path, old=old.splitlines()[0]):
                    self.assertEqual(text.count(old), *count or (1,))
                text = text.replace(old, new)

    def test_each_hook_a_site_calls_has_a_declaration_for_each_side_and_a_body(self):
        # the game's C calls a hook by prelude.h's declaration, and bare.c defines it under native.h's. the adapter's
        # C reads both headers, so its compile holds the two to one type, but only for a hook both declare
        declared = re.compile(r'(?m)^(?:int|void) (native_hook_\w+)\(')
        called = {name for pairs in marks.SITES.values() for _, new, *_ in pairs
                  for name in re.findall(r'\bnative_hook_\w+(?=\()', new)}
        game, host = (set(declared.findall((ROOT / 'engine/native' / header).read_text()))
                      for header in ('prelude.h', 'native.h'))
        bodies = set(re.findall(r'(?m)^(?:int|void) (native_hook_\w+)\([^;{]*\)\n\{',
                                (ROOT / 'engine/native/bare.c').read_text()))
        self.assertTrue(called)
        self.assertEqual(game, called)
        self.assertLess(game, host)
        self.assertEqual(bodies, host)

    def test_the_land_tasks_load_and_releases_reach_the_adapter(self):
        text = (ROOT / 'src/collision/world.c').read_text()
        for old, new, *_ in marks.SITES['src/collision/world.c']:
            text = text.replace(old, new)
        # the task's own load, once the land is read, and each of its two
        # releases, ahead of the release; the stage's own loads reach nothing
        self.assertIn('    world_load_act(world_stage, world_act);\n'
                      '    native_land_loaded(world_land, STAGE_ACT(world_stage, world_act), world_land_material);\n',
                      text)
        self.assertEqual(text.count('native_land_loaded('), 1)
        self.assertEqual(text.count('    native_land_released();\n    world_release_act(world_stage, world_act);\n'), 2)
        self.assertEqual(text.count('native_land_released('), 2)

    def test_a_member_is_marked_and_its_comment_is_not(self):
        text = 'struct over {\n    struct task *owner; /* the *owner, */\n    /* a *comment, */\n    int n;\n};\n'
        self.assertEqual(marks.mark(text, {'over'}),
                         f'struct over {{\n    struct task *{marks.MARK} owner; /* the *owner, */\n'
                         '    /* a *comment, */\n    int n;\n};\n')

    def test_another_struct_is_left(self):
        text = 'struct other {\n    struct task *owner;\n};\n'
        self.assertEqual(marks.mark(text, {'over'}), text)

    def test_a_typedef_before_a_struct_leaves_it_marked(self):
        text = 'typedef struct info {\n    int n;\n} INFO;\nstruct over {\n    struct task *owner;\n};\n'
        self.assertIn(f'struct task *{marks.MARK} owner;', marks.mark(text, {'over'}))

    def test_a_struct_held_whole_is_marked_and_one_with_an_array_of_pointers_is_not(self):
        sources = {'a.h': 'struct pages {\n    char *text;\n};\nstruct shape {\n    void *objects[4];\n};\n'
                          'struct over {\n    struct pages hint;\n    struct shape shape;\n};\n'}
        self.assertEqual(marks.nested({'over'}, sources), {'over', 'pages'})

    def test_ninjas_models_are_marked_by_their_typedefs(self):
        text = ('typedef struct obj {\n    NJS_MODEL *model;\n    struct obj *child;\n} NJS_OBJECT;\n'
                'typedef struct {\n    void *p[3];\n    Uint32 nb[3];\n} NJS_MDATA3;\n'
                'typedef struct {\n    char *name;\n} OTHER;\n')
        out = marks.ninja(text)
        self.assertIn(f'NJS_MODEL *{marks.MARK} model;', out)
        self.assertIn(f'struct obj *{marks.MARK} child;', out)
        self.assertIn(f'void *{marks.MARK} p[3];', out)
        self.assertIn('    char *name;', out)

    def test_a_function_pointer_is_marked(self):
        text = 'struct over {\n    void (*step)(struct task *task);\n};\n'
        self.assertIn(f'(*{marks.MARK} step)', marks.mark(text, {'over'}))

    def test_a_parked_pointer_is_marked(self):
        self.assertEqual(marks.mark('x = *(struct path **)&work->scl.x;', set()),
                         f'x = *(struct path *{marks.MARK} *)&work->scl.x;')

    def test_a_console_guard_takes_its_matching_side(self):
        # a guard is known by its first line of code, so its comment may read anything, or be none
        text = ('#ifdef NONMATCHING\n/* the index,\n * in two lines */\n\nfast();\n#else\nslow();\n#endif\n'
                '#ifdef NONMATCHING\nport();\n#endif\n')
        taken = text.replace('#ifdef NONMATCHING\n/* the index', '#if 0\n/* the index')
        with mock.patch.dict(marks.CONSOLE, {'a.c': {'fast();': 1}}):
            self.assertEqual(marks.console('a.c', text), taken)
            self.assertEqual(marks.console('a.c', text.replace('the index', 'the walk')),
                             taken.replace('the index', 'the walk'))
            self.assertEqual(marks.console('a.c', text.replace('/* the index,\n * in two lines */\n\n', '')),
                             taken.replace('/* the index,\n * in two lines */\n\n', ''))
            for other in (text.replace('fast();', 'faster();'), text + '#ifdef NONMATCHING\nfast();\n#endif\n'):
                with self.assertRaises(SystemExit) as stopped:
                    marks.console('a.c', other)
                self.assertRegex(str(stopped.exception.code), r'\Aa\.c: [02] guards open with this code, where 1 '
                                                              r'should: fast\(\);\Z')
        self.assertEqual(marks.console('b.c', text), text)

    def test_each_console_guard_is_in_the_game_s_sources(self):
        # so a source that changes a guard's first line of code fails here, before the slice builds
        for path in marks.CONSOLE:
            with self.subTest(path):
                text = (ROOT / path).read_text(errors='surrogateescape')
                self.assertEqual(marks.console(path, text).count('#if 0\n'),
                                 text.count('#if 0\n') + sum(marks.CONSOLE[path].values()))

    def test_an_unsuffixed_constant_becomes_a_float(self):
        self.assertEqual(marks.UNSUFFIXED.sub(r'\1F', 'x * 0x1.8p+0 + 0x1p-1F'), 'x * 0x1.8p+0F + 0x1p-1F')

    def test_an_address_in_a_static_word_is_written_as_the_program_starts(self):
        text = ('struct spec { void *step; int var; };\nstatic int object[2];\n'
                'static struct spec specs[2] = {{0, (Sint32)&object[1]}, {0, (Sint32)(object)}};\n'
                'int f(void) { return (Sint32)&object; }\n')
        out = marks.addresses(text)
        # a mark stands in for each address, one more each time
        first, second = (f'{marks.ADDRESS_MARK + n:#x}' for n in range(2))
        self.assertIn('{{0, (Sint32)%s}, {0, (Sint32)%s}}' % (first, second), out)
        self.assertIn(f'native_address32(&specs, sizeof specs, {first}, &(object[1]));', out)
        self.assertIn(f'native_address32(&specs, sizeof specs, {second}, (void *)(object));', out)
        self.assertIn('return (Sint32)&object;', out)

    def test_marked_addresses_run(self):
        program = ('typedef int Sint32;\nstatic int object[2];\nstruct spec { void *step; int var; };\n'
                   'struct spec specs[2] = {{0, (Sint32)&object[1]}, {0, 0}};\n')
        runner = ('#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n'
                  'struct spec { void *step; int var; };\nextern struct spec specs[2];\n'
                  'void native_address32(void *o, unsigned long size, int mark, void *address)\n'
                  '{\n    for (unsigned long at = 0; at < size; at += 4)\n'
                  '        if (*(int *)((char *)o + at) == mark) *(int *)((char *)o + at) = (int)(uintptr_t)address;\n}\n'
                  f'int main(void) {{ return specs[0].var == {marks.ADDRESS_MARK:#x} || '
                  'specs[0].var == 0; }\n')
        with tempfile.TemporaryDirectory() as d:
            Path(d, 'p.c').write_text(marks.addresses(program))
            Path(d, 'm.c').write_text(runner)
            subprocess.run([os.environ.get('CC', 'cc'), '-w', '-no-pie', '-o', f'{d}/run', f'{d}/p.c', f'{d}/m.c'],
                           check=True)
            self.assertEqual(subprocess.run([f'{d}/run']).returncode, 0)

    def test_a_cast_inside_a_longer_expression_stays_as_the_source_writes_it(self):
        # a mark stands for a word's whole value: in a longer expression it would change the value, and the table
        # would hold no mark for the constructor to find
        for expression in ('(Sint32)ZERO ? 1 : 2', 'ZERO ? (Sint32)&object : 0', '1 + (Sint32)ZERO', '-(Sint32)ZERO',
                           '((Sint32)&object) + 4', '(Sint32)ZERO % 3', '(Sint32)ZERO == 0', '0 == (Sint32)ZERO',
                           '(short)(Sint32)ZERO'):
            with self.subTest(expression=expression):
                text = f'enum {{ ZERO }};\nstatic int object;\nstatic Sint32 table[] = {{ 7, {expression}, 9 }};\n'
                self.assertEqual(marks.addresses(text), text)
        # the cast and its operand alone are one, in any parentheses, past a designator, and between the
        # preprocessor's own lines
        for initializer in ('(Sint32)&object', '((Sint32)(&object))', '( ( (Sint32)&object ) )',
                            '[1] = (Sint32)&object', '\n# 12 "a.c"\n(Sint32)&object\n# 14 "a.c" 2\n'):
            with self.subTest(initializer=initializer):
                text = f'static int object;\nstatic Sint32 table[] = {{ 7, {initializer}, 9 }};\n'
                marked = re.sub(r'\(Sint32\)\(?&object\)?', f'(Sint32){marks.ADDRESS_MARK:#x}', text)
                self.assertNotEqual(marked, text)
                out = marks.addresses(text)
                self.assertEqual(out[:len(marked)], marked)
                self.assertIn(f'native_address32(&table, sizeof table, {marks.ADDRESS_MARK:#x}, &(object));', out)

    def test_an_operand_is_the_source_s_own_in_the_constructor_where_it_holds_a_character_or_a_string(self):
        # the pass finds an operand in the text with each character and string as spaces, and writes it as the
        # source spells it
        text = ('typedef int Sint32;\nextern int items[128];\n'
                'static Sint32 table[] = { (Sint32)&items[\'A\'], (Sint32)&items[sizeof "ab"] };\n')
        out = marks.addresses(text)
        first, second = (f'{marks.ADDRESS_MARK + n:#x}' for n in range(2))
        self.assertIn(f"native_address32(&table, sizeof table, {first}, &(items['A']));", out)
        self.assertIn(f'native_address32(&table, sizeof table, {second}, &(items[sizeof "ab"]));', out)
        with tempfile.TemporaryDirectory() as d:
            Path(d, 'p.c').write_text(out)
            subprocess.run([os.environ.get('CC', 'cc'), '-w', '-fsyntax-only', f'{d}/p.c'], check=True)

    def test_a_string_that_opens_a_comment_hides_no_table_from_the_pass(self):
        # a comment's opening in one string and its close in a later one are text, and the code between them code
        text = ('const char *opening = "/*";\nstatic int object;\nstatic Sint32 table[] = { (Sint32)&object };\n'
                'const char *closing = "*/";\n')
        marked = text.replace('&object', f'{marks.ADDRESS_MARK:#x}')
        out = marks.addresses(text)
        self.assertEqual(out[:len(marked)], marked)
        self.assertIn(f'native_address32(&table, sizeof table, {marks.ADDRESS_MARK:#x}, &(object));', out)
        # and a comment that holds a quote is still none of the code
        text = 'static int object; /* the table\'s */\nstatic Sint32 table[] = { (Sint32)&object /* " */ };\n'
        self.assertIn(f'{{ (Sint32){marks.ADDRESS_MARK:#x} /* " */ }};', marks.addresses(text))

    def test_a_string_or_a_name_that_spells_a_long_float_or_a_constant_keeps_its_text(self):
        # SHC's type and the float pass's constants are respelled where they are code: a string's bytes are the
        # program's data, and a name is its own
        text = ('const char *spelled = "long float 0x1p0";\nchar quote = \'"\'; long float x = 0x1p0;\n'
                'int value_0x1p0, long_float = 0x1.8p+1 + 0x1p-1F; /* "long float */ long\nfloat y = -0x1p0;\n')
        made = ('const char *spelled = "long float 0x1p0";\nchar quote = \'"\'; double x = 0x1p0F;\n'
                'int value_0x1p0, long_float = 0x1.8p+1F + 0x1p-1F; /* "long float */ double y = -0x1p0F;\n')
        with tempfile.TemporaryDirectory() as d:
            self.assertEqual(unit(d, 'src/a.c', text), f'# 1 "src/a.c"\n{made}')

    def test_a_files_structs_read_it_where_it_lies(self):
        if not shutil.which('clang'):
            self.skipTest('no clang')
        with tempfile.TemporaryDirectory() as d:
            Path(d, 'f.c').write_text(marks.mark(MESSAGES, marks.FILES))
            subprocess.run(['clang', '-O2', '-fms-extensions', '-w', '-no-pie', '-o', f'{d}/run', f'{d}/f.c'],
                           check=True)
            self.assertEqual(subprocess.run([f'{d}/run']).returncode, 0)

    def test_shcs_long_float_is_a_double(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d, 'x.i')
            path.write_text('long float x = 1.0F;\n')
            Path(d, 'overlaid.txt').write_text('')
            marks.preprocessed(Path(d), path)
            self.assertEqual(path.read_text(), 'double x = 1.0F;\n')


# a message file as the SH-4 lays it out, its pointers into itself where it
# lies, read through script.h's structs
MESSAGES = r"""
#include <string.h>
typedef short Sint16;
typedef int Sint32;
struct script_page {
    char *text;
    Sint32 steps;
};
struct script_row {
    Sint16 *script;
    void *runs;
};
struct field_messages {
    Sint32 people;
    struct script_row *table;
};
static unsigned int file[16];
int main(void)
{
    unsigned int at = (unsigned int)(unsigned long)file;
    struct field_messages *messages = (void *)file;
    struct script_page *page;

    file[0] = 3, file[1] = at + 8;
    file[2] = at + 40, file[3] = at + 16;
    file[4] = at + 32, file[5] = 7, file[6] = 0, file[7] = 0;
    memcpy(&file[8], "hello", 6);
    file[10] = 0xffff;
    page = messages->table[0].runs;
    return !(messages->people == 3 && *messages->table[0].script == -1 && !strcmp(page->text, "hello") &&
             page->steps == 7 && !page[1].text);
}
"""


PROGRAM = """
#define WORD __ptr32 __uptr
struct record { void (*WORD call)(int *); int *WORD at; int spare[8]; };
static int value;
static int spare[8] = {1, 2, 3, 4, 5, 6, 7, 8};
static void bump(int *n) { ++*n; }
struct record table = {bump, &value};
int main(void)
{
    struct record copy;
    __builtin_memcpy(&copy, &table, sizeof copy);
    __builtin_memcpy(copy.at, spare, sizeof *copy.at);
    copy.call(copy.at);
    return value == 2 ? 0 : 1;
}
"""


# the same in a domain above 4 GB: a task's chain, a call through a reference,
# a heap block's address kept in a number, two pointers' difference, and a
# reference passed to a call with no prototype
DOMAIN = 0x100000000
DOMAIN_PROGRAM = """
#define WORD __ptr32 __uptr
void *mmap(void *, unsigned long, int, int, int, long);
struct task { struct task *WORD next; void (*WORD step)(struct task *WORD); int *WORD data; };
static struct task tasks[2];
static struct task *WORD head = &tasks[0];
static int counts[2];
static void step(struct task *WORD t) { counts[t - tasks]++; ++*t->data; }
int peek();
int main(int argc, char **argv)
{
    int *heap = mmap((void *)(BASE + 0x50000000UL), 4096, 3, 0x100022, -1, 0);
    unsigned word = (unsigned)(unsigned long)heap;
    int *WORD cell = (int *)(unsigned long)word;
    tasks[0].next = &tasks[1];
    for (int i = 0; i < 2; i++) { tasks[i].step = step; tasks[i].data = cell; }
    for (struct task *WORD t = head; t; t = t->next) t->step(t);
    if (argc > 1) { tasks[0].data = (int *)argv; }
    return *heap == 2 && counts[0] == 1 && counts[1] == 1 && (unsigned long)heap >> 32 == BASE >> 32 &&
           peek(tasks[1].data) == 2 ? 0 : 1;
}
int peek(int *p) { return *p; }
"""
NARROW = """
#include <stdint.h>
#include <stdlib.h>
uint32_t native_narrow(void *pointer)
{
    uintptr_t address = (uintptr_t)pointer;
    if (address && address >> 32 != BASE >> 32) abort();
    return (uint32_t)address;
}
"""
# an image above 2 GB starts from the crt files a PIE's does
CRT = 'Scrt1.o', 'crti.o', 'crtbeginS.o', 'crtendS.o', 'crtn.o'
# a number of each width as a pointer, a copy that calls no function, and native_narrow as a source of the slice's
# would declare it. NUMBERS_MAIN holds each number's pointer, a value's and a constant's, to its low half in the
# domain, or to null where that half is 0
NUMBERS = """
define ptr @from16(i16 %0) {
  %2 = inttoptr i16 %0 to ptr
  ret ptr %2
}
define ptr @from32(i32 %0) {
  %2 = inttoptr i32 %0 to ptr
  ret ptr %2
}
define ptr @from64(i64 %0) {
  %2 = inttoptr i64 %0 to ptr
  ret ptr %2
}
define void @copy(ptr addrspace(271) %0, ptr %1) {
  call void @llvm.memcpy.inline.p271.p0.i64(ptr addrspace(271) align 4 %0, ptr align 4 %1, i64 4096, i1 false)
  ret void
}
declare void @llvm.memcpy.inline.p271.p0.i64(ptr addrspace(271), ptr, i64, i1 immarg)
declare i32 @native_narrow(ptr noundef)
"""
NUMBERS_MAIN = """
#include <stdint.h>
void *from16(uint16_t), *from32(uint32_t), *from64(uint64_t);
extern void *constants[];
uint32_t native_narrow(void *pointer) { return (uint32_t)(uintptr_t)pointer; }
static void *placed(uint32_t low) { return (void *)(uintptr_t)(low ? BASE | low : 0); }
int main(void)
{
    static const uint64_t numbers[] = {NUMBERS};
    unsigned i;

    for (i = 0; i < sizeof numbers / sizeof *numbers; i++) {
        uint64_t n = numbers[i];
        if (constants[i] != placed(n) || from64(n) != placed(n) || from32(n) != placed(n) ||
            from16(n) != placed((uint16_t)n))
            return 1;
    }
    return 0;
}
"""


class Refs(unittest.TestCase):
    def test_a_call_and_a_static_through_references_run_optimized(self):
        if not shutil.which('clang'):
            self.skipTest('no clang')
        with tempfile.TemporaryDirectory() as folder:
            source, ir, program = (Path(folder) / name for name in ('a.c', 'a.ll', 'a'))
            source.write_text(PROGRAM)
            flags = ['-O2', '-fms-extensions', '-w']
            subprocess.run(['clang', *flags, '-S', '-emit-llvm', '-Xclang', '-disable-llvm-passes', '-o', ir, source],
                           check=True)
            ir.write_text(refs.Pass().run(ir.read_text()))
            subprocess.run(['clang', *flags, '-no-pie', '-o', program, ir], check=True)
            self.assertEqual(subprocess.run([program]).returncode, 0)

    def test_a_static_is_kept_by_name(self):
        self.assertIn('[1 x ptr] [ptr @flag]', refs.keep_statics('@flag = internal global i8 1\n'))

    def test_a_modules_own_list_of_used_names_takes_its_statics_once_each(self):
        # clang writes the list for a static its source marks used, as the game's C marks a few and marks.py an
        # event's head: the module's other statics join it, where a second list would end the compile
        used = '@llvm.compiler.used = appending global [{}], section "llvm.metadata"\n'
        module = '@head = internal global i8 1\n@flag = internal unnamed_addr global i8 1\n@shared = global i8 1\n'
        self.assertEqual(refs.keep_statics(module + used.format('1 x ptr] [ptr @head')),
                         module + used.format('2 x ptr] [ptr @head, ptr @flag'))
        self.assertEqual(refs.keep_statics(module + used.format('1 x ptr] [ptr @shared')),
                         module + used.format('3 x ptr] [ptr @shared, ptr @head, ptr @flag'))
        with self.assertRaises(SystemExit) as failed:
            refs.Pass().run(module + '@llvm.compiler.used = appending global [0 x ptr] zeroinitializer', 'a.ll')
        self.assertRegex(str(failed.exception), r'^a\.ll: @llvm\.compiler\.used in a form ')

    def lowered(self, line, *mode):
        """what the pass makes of one line of a function, as its lines"""
        return refs.Pass(*mode).run(line).split('\n')

    def test_the_pass_lowers_the_forms_it_declares(self):
        # at any base: a call through a reference, a memory intrinsic on one, and a static's address cast to one
        self.assertEqual(self.lowered('  call addrspace(271) void %5(ptr noundef %6) #3')[-1],
                         '  call void %ref.3(ptr noundef %6) #3')
        self.assertEqual(self.lowered('  %7 = call noundef addrspace(271) ptr addrspace(271) %5(i32 1)')[-1],
                         '  %7 = call noundef ptr addrspace(271) %ref.3(i32 1)')
        copied = self.lowered('  call void @llvm.memcpy.p0.p271.i64(ptr align 4 %3, ptr addrspace(271) align 4 %4, '
                              'i64 12, i1 false)')
        self.assertEqual(copied[-2:], ['  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %3, ptr align 4 %ref.3, '
                                       'i64 12, i1 false)',
                                       'declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1 immarg)'])
        self.assertEqual(self.lowered('@p = global ptr addrspace(271) addrspacecast (ptr @x to ptr addrspace(271))'),
                         ['@p = global ptr addrspace(271) inttoptr (i32 trunc (i64 ptrtoint (ptr @x to i64) to i32) '
                          'to ptr addrspace(271))'])
        # past 0: a load and a store through one, and each cast of one
        self.assertEqual(self.lowered('  %3 = load i32, ptr addrspace(271) %2, align 4', DOMAIN)[-2:],
                         ['  %ref.3 = inttoptr i64 %ref.4 to ptr', '  %3 = load i32, ptr %ref.3, align 4'])
        self.assertEqual(self.lowered('  store ptr addrspace(271) %1, ptr addrspace(271) %2, align 4', DOMAIN)[-1],
                         '  store ptr addrspace(271) %1, ptr %ref.3, align 4')
        self.assertEqual(self.lowered('  %3 = addrspacecast ptr %2 to ptr addrspace(271)', DOMAIN)[-1],
                         '  %3 = inttoptr i32 %ref.1 to ptr addrspace(271)')
        self.assertEqual(self.lowered('  %3 = addrspacecast ptr addrspace(271) %2 to ptr', DOMAIN)[-1],
                         '  %3 = inttoptr i64 %ref.5 to ptr')
        # and a reference a call with no prototype passes, which its callee takes as a host pointer
        self.assertEqual(self.lowered('  %3 = call i32 (...) @peek(ptr addrspace(271) noundef %2)', DOMAIN)[-1],
                         '  %3 = call i32 (...) @peek(ptr noundef %ref.6)')
        # under --check a narrowing goes through native_narrow at any base
        self.assertEqual(self.lowered('  %3 = addrspacecast ptr %2 to ptr addrspace(271)', 0, True),
                         ['  %ref.1 = call i32 @native_narrow(ptr %2)',
                          '  %3 = inttoptr i32 %ref.1 to ptr addrspace(271)', 'declare i32 @native_narrow(ptr)'])

    def test_the_backend_takes_a_reference_the_pass_moves_steps_or_compares(self):
        for line in ('  %3 = alloca ptr addrspace(271), align 4',
                     '  %3 = getelementptr inbounds nuw %struct.task, ptr addrspace(271) %2, i32 0, i32 1',
                     '  %3 = icmp ne ptr addrspace(271) %2, null',
                     '  %3 = phi ptr addrspace(271) [ null, %1 ], [ %2, %0 ]',
                     '  %3 = select i1 %1, ptr addrspace(271) %2, ptr addrspace(271) null',
                     '  ret ptr addrspace(271) %2',
                     # a reference read from a host pointer, kept in one, made of a number and read as one
                     '  %3 = load ptr addrspace(271), ptr %2, align 4',
                     '  store ptr addrspace(271) %1, ptr %2, align 4',
                     '  %3 = inttoptr i32 %2 to ptr addrspace(271)',
                     '  %3 = ptrtoint ptr addrspace(271) %2 to i32',
                     '  ret i32 ptrtoint (ptr addrspace(271) inttoptr (i32 8 to ptr addrspace(271)) to i32)',
                     # a call that hands one to a callee with a prototype, and one through a host pointer that
                     # gives one back
                     '  call void @step(ptr addrspace(271) noundef %2)',
                     '  %3 = call ptr addrspace(271) %2(i32 noundef 1)',
                     '%struct.task = type { ptr addrspace(271), i32 }',
                     'define dso_local ptr addrspace(271) @next(ptr addrspace(271) noundef %0) #0 {'):
            for mode in ((), (DOMAIN,), (DOMAIN, True), (DOMAIN, False, True)):
                with self.subTest(line=line, mode=mode):
                    self.assertEqual([kept for kept in self.lowered(line, *mode) if 'native_narrow' not in kept],
                                     [line])
        # at 0 the backend takes each access and each cast too
        for line in ('  %3 = load i32, ptr addrspace(271) %2, align 4',
                     '  store i32 1, ptr addrspace(271) %2, align 4',
                     '  %3 = addrspacecast ptr addrspace(271) %2 to ptr',
                     '  %3 = addrspacecast ptr %2 to ptr addrspace(271)',
                     '  %3 = ptrtoint ptr addrspace(271) %2 to i64',
                     '  ret i64 ptrtoint (ptr addrspace(271) inttoptr (i32 8 to ptr addrspace(271)) to i64)'):
            with self.subTest(line=line):
                self.assertEqual(self.lowered(line), [line])

    def test_a_form_the_pass_has_no_rule_for_ends_it(self):
        for line, modes in (
                # an instruction that reads or writes through a reference in a way no rule lowers
                ('  %3 = atomicrmw add ptr addrspace(271) %2, i32 1 seq_cst, align 4', ((), (DOMAIN,))),
                ('  %3 = va_arg ptr addrspace(271) %2, i32', ((), (DOMAIN,))),
                # an intrinsic on one that is none of the three the pass gives host pointers
                ('  call void @llvm.prefetch.p271(ptr addrspace(271) %2, i32 0, i32 3, i32 1)', ((), (DOMAIN,))),
                ('declare void @llvm.prefetch.p271(ptr addrspace(271) captures(none), i32 immarg, i32 immarg, i32)',
                 ((),)),
                # a memory intrinsic whose operand carries a word the pass doesn't know
                ('  call void @llvm.memset.p271.i64(ptr addrspace(271) sideeffect align 4 %2, i8 0, i64 8, i1 false)',
                 ((),)),
                # a call through a reference that no local value names, and one that is an invoke
                ('  call addrspace(271) void inttoptr (i32 205520896 to ptr addrspace(271))()', ((), (DOMAIN,))),
                ('  invoke addrspace(271) void %5() to label %6 unwind label %7', ((), (DOMAIN,))),
                # a pointer of an address space that is no reference's
                ('  %3 = load i32, ptr addrspace(270) %2, align 4', ((), (DOMAIN,))),
                # past 0, an access or a cast in a form the pass can't lower, where the backend would take the
                # reference without its base
                ('  %3 = load i32, ptr addrspace(271) inttoptr (i32 205520896 to ptr addrspace(271)), align 4',
                 ((DOMAIN,),)),
                ('  store i32 1, ptr addrspace(271) inttoptr (i32 205520896 to ptr addrspace(271)), align 4',
                 ((DOMAIN,),)),
                ('  %3 = addrspacecast ptr addrspace(271) null to ptr', ((DOMAIN,),)),
                ('  %3 = ptrtoint ptr addrspace(271) null to i64', ((DOMAIN,),)),
                ('  %3 = inttoptr i64 205520896 to ptr', ((DOMAIN,),)),
                ('@p = global ptr addrspacecast (ptr addrspace(271) inttoptr (i32 8 to ptr addrspace(271)) to ptr)',
                 ((DOMAIN,),)),
                # a static's reference as a long, which the same cast of a local value gives its base
                ('  ret i64 ptrtoint (ptr addrspace(271) addrspacecast (ptr @g to ptr addrspace(271)) to i64)',
                 ((DOMAIN,),)),
                ('@n = global i64 ptrtoint (ptr addrspace(271) inttoptr (i32 8 to ptr addrspace(271)) to i64)',
                 ((DOMAIN,),)),
                ('  %3 = call i32 (...) @peek(ptr addrspace(271) noundef inttoptr (i32 8 to ptr addrspace(271)))',
                 ((DOMAIN,),)),
                # a narrowing --check can't hand native_narrow
                ('  %3 = addrspacecast ptr @x to ptr addrspace(271)', ((0, True),))):
            for mode in modes:
                with self.subTest(line=line, mode=mode):
                    with self.assertRaises(SystemExit) as failed:
                        refs.Pass(*mode).run(f'define void @f() {{\n{line}\n}}', 'a.ll')
                    self.assertRegex(str(failed.exception), r'^a\.ll:2: .*: ' + re.escape(line.strip()) + '$')

    def test_a_function_that_takes_a_reference_and_then_any_arguments_ends_the_pass_past_0(self):
        # x86-64's clang writes a call with no prototype as its arguments' types and then ..., and the pass hands the
        # callee each reference as a host pointer. a call of a function whose own parameters hold a reference reads
        # the same, and its callee takes the reference, so the function's declaration or definition ends the pass
        self.assertEqual(self.lowered('  %3 = call i32 (ptr addrspace(271), ...) @peek(ptr addrspace(271) noundef %2)',
                                      DOMAIN)[-1], '  %3 = call i32 (ptr, ...) @peek(ptr noundef %ref.6)')
        for line in ('declare void @f(ptr addrspace(271) noundef, ...) #1',
                     'define dso_local void @f(i32 noundef %0, ptr addrspace(271) noundef %1, ...) #0 {'):
            for mode in ((DOMAIN,), (DOMAIN, False, True)):
                with self.subTest(line=line, mode=mode):
                    with self.assertRaises(SystemExit) as failed:
                        refs.Pass(*mode).run(line, 'a.ll')
                    self.assertRegex(str(failed.exception), r'^a\.ll:1: .*: ' + re.escape(line) + '$')
            # at 0 its call stands as the compiler wrote it
            self.assertEqual(self.lowered(line), [line])
        # one with no reference before its ..., one with no prototype, and one that takes a reference and no more
        for line in ('declare i32 @printf(ptr noundef, ...) #1', 'declare i32 @peek(...) #1',
                     'declare void @step(ptr addrspace(271) noundef) #1',
                     'define dso_local ptr addrspace(271) @next(i32 noundef %0, ...) #0 {'):
            with self.subTest(line=line):
                self.assertEqual(self.lowered(line, DOMAIN), [line])

    def test_a_constant_number_of_another_width_as_a_host_pointer_ends_the_pass_past_0(self):
        # clang makes a constant address of a number of the host's width, which the pass places in the domain. one
        # of another width would reach the backend with no base
        for line in ('@p = global ptr inttoptr (i32 4096 to ptr)',
                     '@p = global ptr inttoptr (i32 ptrtoint (ptr @x to i32) to ptr)',
                     '  store ptr inttoptr (i16 8 to ptr), ptr %2, align 8'):
            with self.subTest(line=line):
                with self.assertRaises(SystemExit) as failed:
                    refs.Pass(DOMAIN).run(line, 'a.ll')
                self.assertRegex(str(failed.exception), r'^a\.ll:1: .*: ' + re.escape(line.strip()) + '$')
                # the slice's own numbers are the host's, and at 0 a number is its address
                self.assertEqual(self.lowered(line, DOMAIN, False, True), [line])
                self.assertEqual(self.lowered(line), [line])
        # one of the host's width goes to the domain, and a constant reference stands
        self.assertEqual(self.lowered('@p = global ptr inttoptr (i64 4096 to ptr)', DOMAIN),
                         [f'@p = global ptr inttoptr (i64 {DOMAIN + 4096} to ptr)'])
        line = '@p = global ptr addrspace(271) inttoptr (i32 4096 to ptr addrspace(271))'
        self.assertEqual(self.lowered(line, DOMAIN), [line])

    def test_a_constant_address_is_null_where_its_low_half_is_0_as_a_values_number_is(self):
        # a number holds a reference in its low half, whatever its high half holds: 0 there is null, and anything
        # else lies in the domain
        for number, pointer in ((4096, f'inttoptr (i64 {DOMAIN + 4096} to ptr)'),
                                (2 * DOMAIN + 4096, f'inttoptr (i64 {DOMAIN + 4096} to ptr)'),
                                (-1, f'inttoptr (i64 {DOMAIN + 0xffffffff} to ptr)'),
                                (DOMAIN, 'null'), (-DOMAIN, 'null')):
            with self.subTest(number=number):
                self.assertEqual(self.lowered(f'@p = global [2 x ptr] [ptr inttoptr (i64 {number} to ptr), ptr null]',
                                              DOMAIN), [f'@p = global [2 x ptr] [ptr {pointer}, ptr null]'])
        # as a value's number is: by its low half, and null where that is 0
        self.assertEqual(self.lowered('  %3 = inttoptr i64 %2 to ptr', DOMAIN),
                         ['  %ref.1 = trunc i64 %2 to i32', '  %ref.2 = zext i32 %ref.1 to i64',
                          f'  %ref.3 = or i64 %ref.2, {DOMAIN}', '  %ref.4 = icmp eq i32 %ref.1, 0',
                          '  %ref.5 = select i1 %ref.4, i64 0, i64 %ref.3', '  %3 = inttoptr i64 %ref.5 to ptr'])

    def test_a_memory_intrinsic_that_calls_no_function_keeps_its_name_on_host_pointers(self):
        # llvm.memcpy.inline and llvm.memset.inline promise a copy with no call, which the plain ones don't
        copied = self.lowered('  call void @llvm.memcpy.inline.p0.p271.i64(ptr align 4 %3, ptr addrspace(271) align 4 '
                              '%4, i64 64, i1 false)\ndeclare void @llvm.memcpy.inline.p0.p271.i64(ptr noalias '
                              'writeonly captures(none), ptr addrspace(271) noalias readonly captures(none), i64, '
                              'i1 immarg) #1')
        self.assertEqual(copied[-2:], ['  call void @llvm.memcpy.inline.p0.p0.i64(ptr align 4 %3, ptr align 4 %ref.3, '
                                       'i64 64, i1 false)',
                                       'declare void @llvm.memcpy.inline.p0.p0.i64(ptr, ptr, i64, i1 immarg)'])
        cleared = self.lowered('  call void @llvm.memset.inline.p271.i32(ptr addrspace(271) align 4 %4, i8 0, i32 64, '
                               'i1 false)', DOMAIN)
        self.assertEqual(cleared[-2:], ['  call void @llvm.memset.inline.p0.i32(ptr align 4 %ref.3, i8 0, i32 64, '
                                        'i1 false)',
                                        'declare void @llvm.memset.inline.p0.i32(ptr, i8, i32, i1 immarg)'])
        # one on host pointers already stands, beside the plain one of its operands
        both = ('  call void @llvm.memcpy.inline.p0.p0.i64(ptr %3, ptr %4, i64 64, i1 false)\n'
                '  call void @llvm.memcpy.p0.p0.i64(ptr %3, ptr %4, i64 64, i1 false)')
        self.assertEqual(self.lowered(both), both.split('\n') + [
            'declare void @llvm.memcpy.inline.p0.p0.i64(ptr, ptr, i64, i1 immarg)',
            'declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1 immarg)'])

    def test_a_number_of_any_width_gives_a_host_pointer_by_its_low_32_bits(self):
        # a number narrower than a reference is extended with zeros, as the cast itself extends it
        for bits, word in ((16, 'zext i16 %2 to i32'), (8, 'zext i8 %2 to i32'), (32, 'add i32 %2, 0'),
                           (64, 'trunc i64 %2 to i32'), (128, 'trunc i128 %2 to i32')):
            with self.subTest(bits=bits):
                self.assertEqual(self.lowered(f'  %3 = inttoptr i{bits} %2 to ptr, !dbg !7', DOMAIN),
                                 [f'  %ref.1 = {word}', '  %ref.2 = zext i32 %ref.1 to i64',
                                  f'  %ref.3 = or i64 %ref.2, {DOMAIN}', '  %ref.4 = icmp eq i32 %ref.1, 0',
                                  '  %ref.5 = select i1 %ref.4, i64 0, i64 %ref.3',
                                  '  %3 = inttoptr i64 %ref.5 to ptr, !dbg !7'])

    def test_native_narrow_is_declared_once_and_in_the_type_the_pass_calls_it_by(self):
        # a second declaration of a name ends the compile, so a module that declares or defines native_narrow keeps
        # its own, which the pass's call must fit
        narrowed = '  %3 = addrspacecast ptr %2 to ptr addrspace(271)'
        for own in ('declare i32 @native_narrow(ptr noundef) #1', 'declare zeroext i32 @native_narrow(ptr)',
                    'define dso_local i32 @native_narrow(ptr noundef captures(none) %0) #0 {'):
            with self.subTest(own=own):
                self.assertEqual(self.lowered(f'{narrowed}\n{own}', 0, True),
                                 ['  %ref.1 = call i32 @native_narrow(ptr %2)',
                                  '  %3 = inttoptr i32 %ref.1 to ptr addrspace(271)', own])
        # a module with none takes the pass's, as one that names it in a call alone does
        called = '  %4 = call i32 @native_narrow(ptr noundef %2)'
        self.assertEqual(self.lowered(called, 0, True), [called, 'declare i32 @native_narrow(ptr)'])
        for own in ('declare i64 @native_narrow(ptr noundef)', 'declare i32 @native_narrow(ptr noundef, i32 noundef)',
                    'declare i32 @native_narrow(ptr addrspace(271) noundef)', 'declare i32 @native_narrow(...)',
                    'define dso_local void @native_narrow(ptr noundef %0) #0 {'):
            with self.subTest(own=own):
                with self.assertRaises(SystemExit) as failed:
                    refs.Pass(0, True).run(f'{narrowed}\n{own}', 'a.ll')
                self.assertRegex(str(failed.exception), r'^a\.ll:2: .*: ' + re.escape(own) + '$')
                # without --check the pass calls none, and the module's is its own
                self.assertEqual(self.lowered(own), [own])

    def test_numbers_as_pointers_an_inline_copy_and_a_declared_native_narrow_compile_and_run(self):
        if not shutil.which('clang'):
            self.skipTest('no clang')
        # past 0 and under --check, the compiler takes what the pass makes of NUMBERS, and a number gives the
        # pointer of its low half as a constant as it does as a value, and of its low 16 bits as a value that wide
        numbers =(0, 4096, 0xffff0000, DOMAIN, DOMAIN + 4096, 2 * DOMAIN + 0x10000, -1)
        constants = ', '.join(f'ptr inttoptr (i64 {number} to ptr)' if number else 'ptr null' for number in numbers)
        with tempfile.TemporaryDirectory() as folder:
            source, ir, assembly, program = (Path(folder) / name for name in ('m.c', 'a.ll', 'a.s', 'a'))
            source.write_text(NUMBERS_MAIN)
            ir.write_text(refs.Pass(DOMAIN, True).run(
                f'{NUMBERS}@constants = global [{len(numbers)} x ptr] [{constants}]\n'))
            flags = ['-O2', '-w', f'-DBASE={DOMAIN}ULL',
                     '-DNUMBERS=' + ', '.join(f'{number & 0xffffffffffffffff}ULL' for number in numbers)]
            subprocess.run(['clang', *flags, '-o', program, ir, source], check=True)
            self.assertEqual(subprocess.run([program]).returncode, 0)
            # the copy stays the compiler's own instructions
            subprocess.run(['clang', *flags, '-S', '-o', assembly, ir], check=True)
            self.assertNotIn('memcpy', assembly.read_text())

    def test_a_domain_above_4_gb_runs_and_refuses_what_lies_outside(self):
        if not shutil.which('clang'):
            self.skipTest('no clang')
        with tempfile.TemporaryDirectory() as folder:
            source, narrow, ir, program = (Path(folder) / name for name in ('a.c', 'n.c', 'a.ll', 'a'))
            source.write_text(DOMAIN_PROGRAM)
            narrow.write_text(NARROW)
            flags = ['-O2', '-fms-extensions', '-w', f'-DBASE={DOMAIN}UL']
            crt = [subprocess.run(['clang', f'-print-file-name={name}'], capture_output=True, text=True,
                                  check=True).stdout.strip() for name in CRT]
            for check in (False, True):
                subprocess.run(['clang', *flags, '-S', '-emit-llvm', '-Xclang', '-disable-llvm-passes', '-o', ir,
                                source], check=True)
                ir.write_text(refs.Pass(DOMAIN, check).run(ir.read_text()))
                subprocess.run(['clang', *flags, '-no-pie', '-nostartfiles', f'-Wl,-Ttext-segment={DOMAIN + 0x400000:#x}',
                                '-o', program, *crt[:3], ir, narrow, '-lc', *crt[3:]], check=True)
                self.assertEqual(subprocess.run([program]).returncode, 0)
                # a host pointer parked in a reference: truncated unseen, or refused at the narrowing
                outside = subprocess.run([program, 'x'], capture_output=True, text=True)
                self.assertEqual(outside.returncode != 0, check, outside.stderr)


# what a slice's objects need of stubs.py, as nm and readelf give it: a call of a console service, a read of a
# word, the heap, an image's end, the loader's buffer and its entry, and a call of the engine
NEEDED = {'syHwInit': True, '_errno': False, 'heap_arena': False, 'advertise_module_end': False,
          'loader_buffer': False, 'loader_entry': True, 'sa_present_mode': True}
ROWS = ('# name  kind  host  reason\n'
        'syHwInit  function  *  the hardware\'s start\n'
        '_errno  zeros  *  the runtime\'s error word\n'
        'heap_arena  address  *  the game\'s heap\n'
        'advertise_module_end  address  *  the menus\' end\n'
        'loader_buffer  address  *  the loader\'s place\n'
        'loader_entry  function  *  the loader\'s entry\n'
        'sa_present_mode  engine  *  the backend\'s mode\n')


class Stubs(unittest.TestCase):
    def write(self, table, needed=NEEDED, *options, coff=False, lengths={'loader': 0x100000}):
        """what stubs.py writes for objects that need needed's names, each called where its value is True, by a
        table of table's rows: its C's lines past its head, and what it says. the memory contract gives each image
        lengths' load area, or its own where lengths is None"""
        def run(command, **_):
            if command[0] == 'readelf':
                lines = [f'0000  0004 R_X86_64_PLT32  0000 {name} - 4' for name, called in needed.items() if called]
            elif '-A' in command:
                lines = ['a.o: 0000000000000000 T main', *(f'a.o:                  U {name}' for name in needed)]
            else:
                lines = []
            return mock.Mock(stdout='\n'.join(lines))

        with tempfile.TemporaryDirectory() as d, inside(ROOT), mock.patch.object(stubs.subprocess, 'run', run), \
                mock.patch.object(stubs, 'calls', lambda _: {name for name, called in needed.items() if called}), \
                mock.patch.object(stubs, 'TABLE', Path(d, 'stubs.txt'), create=True), \
                mock.patch.object(stubs, 'contract', (lambda _: mock.Mock(lengths=lengths)) if lengths else
                                  stubs.contract, create=True), \
                contextlib.redirect_stdout(io.StringIO()) as said:
            Path(d, 'stubs.txt').write_text(table)
            Path(d, 'a.o').write_bytes(b'MZ' if coff else b'\x7fELF')
            stubs.main(str(Path(d, 'stubs.c')), str(Path(d, 'a.o')), *options)
            return Path(d, 'stubs.c').read_text().splitlines()[4:], said.getvalue()

    def test_each_needed_name_takes_what_its_row_gives(self):
        # by the tree's own tables and memory contract
        lines, said = self.write(ROWS, lengths=None)
        self.assertEqual(lines, [
            'char _errno[65536] __attribute__((aligned(64)));',
            'void *advertise_module_end = (void *)0x8c9c5b30ULL;',
            '__asm__(".globl heap_arena\\n.type heap_arena, @object\\n.size heap_arena, 0x120040\\n'
            '.set heap_arena, 0x8c7dffc0");',
            # the loader's buffer lies where the console reads the loader's file, with its whole room
            '__asm__(".globl loader_buffer\\n.type loader_buffer, @object\\n.size loader_buffer, 0x100000\\n'
            '.set loader_buffer, 0x8ce00000");',
            # and its entry, which main calls, is a function all the same
            'long loader_entry(void) { native_stub("loader_entry"); ZERO_FLOAT; return 0; }',
            'long syHwInit(void) { native_stub("syHwInit"); ZERO_FLOAT; return 0; }'])
        self.assertIn('2 functions, 1 of zeros, 3 placed, 1 the engine\'s, 0 unsupported', said)

    def test_each_placed_name_lies_in_the_domain_base_names(self):
        # the C names no domain of its own: the heap's address and an image's end both take --base
        lines, _ = self.write(ROWS, NEEDED, '--base', '0x100000000', lengths=None)
        self.assertIn('void *advertise_module_end = (void *)0x18c9c5b30ULL;', lines)
        self.assertIn('.set heap_arena, 0x18c7dffc0");', lines[2])
        self.assertIn('.set loader_buffer, 0x18ce00000");', lines[3])
        self.assertNotIn('NATIVE_DOMAIN', '\n'.join(lines))

    def test_an_unsupported_name_takes_the_stub_of_its_use_and_is_counted_apart(self):
        # a call the game needs an answer to, and a table it reads
        table = (ROWS.replace('loader_entry  function', 'loader_entry  unsupported')
                 .replace('_errno  zeros', '_errno  unsupported'))
        lines, said = self.write(table)
        self.assertIn('long loader_entry(void) { native_stub("loader_entry"); ZERO_FLOAT; return 0; }', lines)
        self.assertIn('char _errno[65536] __attribute__((aligned(64)));', lines)
        self.assertIn('1 functions, 0 of zeros, 3 placed, 1 the engine\'s, 2 unsupported', said)

    def test_a_needed_name_with_no_row_ends_the_build(self):
        with self.assertRaises(SystemExit) as failed:
            self.write(ROWS, {**NEEDED, 'syNewService': True, 'new_word': False})
        # each names the kinds its use allows
        self.assertRegex(str(failed.exception), r'new_word: .*no row.* of kind zeros or unsupported\n'
                                                r'.*syNewService: .*no row.* of kind function or engine or unsupported$')

    def test_a_row_nothing_needs_ends_the_build(self):
        with self.assertRaises(SystemExit) as failed:
            self.write(ROWS + 'syHwFinish  function  *  the hardware\'s end\n')
        self.assertRegex(str(failed.exception), r'stubs\.txt:9: syHwFinish: ')

    def test_a_row_whose_kind_is_not_the_names_use_ends_the_build(self):
        for row, use in (('syHwInit  zeros  *  x', 'calls it'), ('_errno  function  *  x', 'calls it'),
                         ('heap_arena  zeros  *  x', 'place'), ('_errno  address  *  x', 'place'),
                         ('loader_buffer  zeros  *  x', 'place'), ('loader_entry  address  *  x', 'calls it'),
                         # the tables place nothing the PC lacks, and the engine defines a call alone
                         ('heap_arena  unsupported  *  x', 'place.*its kind is address$'),
                         ('heap_arena  engine  *  x', 'place'),
                         ('_errno  engine  *  x', 'calls it.*its kind is zeros or unsupported$')):
            name = row.split()[0]
            table = ''.join(line + '\n' for line in ROWS.splitlines() if not line.startswith(name)) + row + '\n'
            with self.subTest(row=row), self.assertRaises(SystemExit) as failed:
                self.write(table)
            self.assertRegex(str(failed.exception), rf'stubs\.txt:8: {name}: .*{use}')

    def test_exploring_writes_a_stub_for_each_name_with_no_row_and_names_it(self):
        lines, said = self.write(ROWS + 'syHwFinish  function  *  the hardware\'s end\n',
                                 {**NEEDED, 'syNewService': True, 'new_word': False}, '--explore')
        self.assertIn('long syNewService(void) { native_stub("syNewService"); ZERO_FLOAT; return 0; }', lines)
        self.assertIn('char new_word[65536] __attribute__((aligned(64)));', lines)
        self.assertRegex(said, r'syNewService: .*no row')
        self.assertRegex(said, r'new_word: .*no row')
        self.assertRegex(said, r'stubs\.txt:9: syHwFinish: ')
        self.assertIn('explored', said)

    def test_a_row_speaks_for_its_host_alone(self):
        table = ROWS.replace('_errno  zeros  *', '_errno  zeros  linux') + 'sa1_errno  zeros  windows  the same\n'
        self.assertIn('char _errno[65536] __attribute__((aligned(64)));', self.write(table)[0])
        windows = {**{name: called for name, called in NEEDED.items() if name != '_errno'}, 'sa1_errno': False}
        self.assertIn('char sa1_errno[65536] __attribute__((aligned(64)));', self.write(table, windows, coff=True)[0])
        with self.assertRaises(SystemExit) as failed:
            self.write(table, windows)
        self.assertRegex(str(failed.exception), r'sa1_errno: .*no row')

    def test_a_row_out_of_its_form_ends_the_build(self):
        for row, problem in (('syHwInit  function  *', 'not a row: NAME KIND HOST REASON'),
                             ('syHwInit  code  *  x', 'code: not a kind'),
                             ('syHwInit  function  mac  x', 'mac: not a host'),
                             ('sy-HwInit  function  *  x', 'not a row: NAME KIND HOST REASON'),
                             ('_errno  zeros  linux  x', '_errno has a row already')):
            with self.subTest(row=row), self.assertRaises(SystemExit) as failed:
                self.write(ROWS + row + '\n')
            self.assertRegex(str(failed.exception), rf'stubs\.txt:9: .*{re.escape(problem)}')

    def test_the_trees_table_reads_and_gives_each_name_one_row_a_host(self):
        # rows() fails on a row out of its form and on a name's second row for a host
        for host in stubs.HOSTS:
            with self.subTest(host=host):
                self.assertTrue(stubs.rows(ROOT / 'engine/native/stubs.txt', host))


class Programs(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # programs.py reads the names nm gives of ELF objects, as the slice's are, where Mach-O spells each with
        # an underscore. clang writes ELF for a Linux target on any host
        cc = os.environ.get('CC', 'cc')
        with tempfile.TemporaryDirectory() as d:
            Path(d, 'a.c').write_text('int a;\n')
            for cls.cc in ([cc], [cc, '--target=x86_64-unknown-linux-gnu']):
                made = subprocess.run([*cls.cc, '-c', '-o', f'{d}/a.o', f'{d}/a.c'], capture_output=True)
                if made.returncode == 0 and Path(d, 'a.o').read_bytes()[:4] == b'\x7fELF':
                    return
        raise unittest.SkipTest(f'{cc} writes no ELF object')

    def keep(self, rest, *sources, first='', rows='', live=None, explore=False):
        """each program's kept names, the rest's object given rest's C. first and rows are the rows of
        config/1st_read/imports.txt and of config/imports.txt, and live the images the memory contract lets each
        program take names from: none, where it names none, and every program for the rest"""
        with tempfile.TemporaryDirectory() as d:
            names = []
            for i, text in enumerate(sources):
                Path(d, f'p{i}.c').write_text(text)
                subprocess.run([*self.cc, '-c', '-o', f'{d}/program-p{i}.r', f'{d}/p{i}.c'], check=True)
                names.append(f'p{i}')
            Path(d, 'rest.c').write_text(rest)
            subprocess.run([*self.cc, '-c', '-o', f'{d}/rest.o', f'{d}/rest.c'], check=True)
            Path(d, 'config/1st_read').mkdir(parents=True)
            Path(d, 'config/1st_read/imports.txt').write_text(first)
            Path(d, 'config/imports.txt').write_text(rows)
            argv = ['programs.py', *(['--explore'] if explore else []), d, *names, '--', f'{d}/rest.o']
            with mock.patch.object(sys, 'argv', argv), mock.patch.object(programs, 'ROOT', Path(d)), \
                    mock.patch.object(programs, 'colive', create=True, new=lambda: {
                        '1st_read': set(names), **{name: set() for name in names}, **(live or {})}), \
                    contextlib.redirect_stderr(io.StringIO()) as said:
                try:
                    programs.main()
                except SystemExit as failed:
                    # a failure leaves no file a link could take
                    self.assertEqual(sorted(path.name for path in Path(d).glob('keep-*')), [])
                    raise SystemExit(f'{said.getvalue()}{failed}') from None
            self.said = said.getvalue()
            return [set(Path(d, f'keep-{n}.txt').read_text().split()) for n in names]

    def test_a_name_one_program_defines_and_another_reads_is_kept(self):
        self.assertEqual(self.keep('extern int a; int f(void) { return a; }', 'int a = 1, b = 2;', 'int c;'),
                         [{'a'}, {'native_none'}])

    # two programs define both, and a third reads it, and one more name of the first's
    SHARED = ('int both = 1, own = 2;', 'int both = 3;', 'extern int both, own; int f(void) { return both + own; }')

    def test_a_name_two_programs_define_goes_to_the_one_live_with_its_reader(self):
        kept = self.keep('int r;', *self.SHARED, live={'p2': {'p1'}})
        self.assertEqual(kept[:2], [{'own'}, {'both'}])

    def test_a_readers_row_names_its_provider_before_the_contract_does(self):
        rows = '# image name provider symbol view reason\np2 both p1 both 0x0c the model the event animates\n'
        kept = self.keep('int r;', *self.SHARED, rows=rows, live={'p2': {'p0', 'p1'}})
        self.assertEqual(kept[:2], [{'own'}, {'both'}])
        # and a row for every image, where the reader has none of its own
        kept = self.keep('int r;', *self.SHARED, rows=rows.replace('p2 both', '* both'), live={'p2': {'p0', 'p1'}})
        self.assertEqual(kept[:2], [{'own'}, {'both'}])

    def test_a_name_no_rule_gives_one_program_ends_the_build(self):
        # two definers live with the reader and no row, and no definer live with it
        for live in ({'p2': {'p0', 'p1'}}, {}):
            with self.subTest(live=live), self.assertRaises(SystemExit) as failed:
                self.keep('int r;', *self.SHARED, live=live)
            self.assertRegex(str(failed.exception), r'programs: both: p0, p1 define it, and its readers take it p2 '
                                                    r'from none of them\. .*\nprograms: 1 names that no rule binds')

    def test_a_name_two_readers_take_from_two_programs_ends_the_build(self):
        with self.assertRaises(SystemExit) as failed:
            self.keep('int r;', *self.SHARED, 'extern int both; int g(void) { return both; }',
                      live={'p2': {'p0'}, 'p3': {'p1'}})
        self.assertIn('its readers take it p2 from p0, p3 from p1.', str(failed.exception))
        # the rest, which 1ST_READ.BIN's imports give no provider, beside a program that has one
        with self.assertRaises(SystemExit) as failed:
            self.keep('extern int both; int f(void) { return both; }', *self.SHARED, live={'p2': {'p0'}})
        self.assertIn('its readers take it 1st_read from none of them, p2 from p0.', str(failed.exception))

    def test_exploring_gives_such_a_name_to_the_program_its_reader_reads_most_and_says_so(self):
        kept = self.keep('int r;', *self.SHARED, live={'p2': {'p0', 'p1'}}, explore=True)
        self.assertEqual(kept[:2], [{'both', 'own'}, {'native_none'}])
        self.assertIn('programs: both: p0, p1 define it', self.said)

    def test_the_rest_takes_the_program_1st_reads_imports_name(self):
        rest = 'extern int both; int f(void) { return both; }'
        self.assertEqual(self.keep(rest, 'int both = 1;', 'int both = 2;', first='# name image\nboth p1\n'),
                         [{'native_none'}, {'both'}])

    def test_a_name_only_the_rest_reads_and_no_import_names_stays_each_programs(self):
        self.assertEqual(self.keep('extern int both; int f(void) { return both; }', 'int both = 1;', 'int both = 2;'),
                         [{'native_none'}, {'native_none'}])


class Coff(unittest.TestCase):
    def test_a_join_binds_its_reads_and_places_its_commons_and_its_copy_keeps_what_it_names(self):
        if not shutil.which('clang'):
            self.skipTest('no clang')
        with tempfile.TemporaryDirectory() as d:
            for name, text in (('a', 'int shared; int value = 5; int other(void);\n'
                                     'int first(void) { return other() + value + shared; }'),
                               ('b', 'int shared; extern int anchor[]; int other(void) { return anchor[0]; }')):
                Path(d, f'{name}.c').write_text(text)
                subprocess.run(['clang', '--target=x86_64-w64-mingw32', '-fcommon', '-O1', '-c', '-o', f'{d}/{name}.o',
                                f'{d}/{name}.c'], check=True)
            self.assertEqual(coff.calls(f'{d}/a.o'), {'other'})
            coff.link(f'{d}/p.r', [f'{d}/a.o', f'{d}/b.o'], [('anchor', 0x8c010000)])
            _, sections, symbols = coff.read(f'{d}/p.r')
            self.assertFalse({'.drectve', '.llvm_addrsig'} & {s.name for s in sections})
            named = {s.name: s for s in symbols if s.storage == coff.EXTERNAL}
            self.assertEqual((named['anchor'].section, named['anchor'].value), (coff.ABSOLUTE, 0x8c010000))
            # the common in a .bss of its own, aligned to 4 as .drectve said
            self.assertEqual((named['shared'].section.name, named['shared'].section.flags >> 20 & 0xf), ('.bss', 3))
            self.assertEqual({t.section for s in sections for _, t, _ in s.relocations if t.name == 'other'},
                             {named['other'].section})
            coff.copy(f'{d}/p.r', f'{d}/p.o', {'first': 'second'}, {'second'}, {'.data': '.data$p$m'})
            _, sections, symbols = coff.read(f'{d}/p.o')
            self.assertEqual({s.name for s in symbols if s.defined()}, {'second'})
            self.assertIn('.data$p$m', {s.name for s in sections})

    # x86-64's machine number, for an object a test writes itself
    AMD64 = 0x8664

    def test_a_join_ends_at_a_name_given_twice_and_at_a_common_wider_than_a_section_aligns(self):
        # --defsym's name that the object defines, and a common whose directive aligns it to 16 KB, where a
        # section's flags hold 8 KB and the next number up is another flag's
        with tempfile.TemporaryDirectory() as d:
            data = coff.Section('.data', coff.READ | coff.WRITE, bytes(4), 4)
            directive = b' -aligncomm:"wide",14'
            coff.write(f'{d}/a.o', self.AMD64, [data, coff.Section('.drectve', 0, directive, len(directive))],
                       [coff.Symbol('value', 0, data, 0, coff.EXTERNAL),
                        coff.Symbol('wide', 4, coff.UNDEFINED, 0, coff.EXTERNAL)])
            for defsyms, problem in (([('value', 1)], 'value is defined, and --defsym gives it again'),
                                     ([], "wide is aligned to 16384 bytes, past the 8192 a section's flags hold")):
                with self.subTest(defsyms=defsyms):
                    with self.assertRaises(SystemExit) as failed:
                        coff.link(f'{d}/p.r', [f'{d}/a.o'], defsyms)
                    self.assertEqual(str(failed.exception), f'coff.py: {d}/p.r: {problem}')
                    self.assertFalse(Path(d, 'p.r').exists())

    def test_a_relocation_at_a_sections_start_follows_no_call(self):
        # the section's last byte is a call's opcode, which lies ahead of no relocation at its first
        with tempfile.TemporaryDirectory() as d:
            far, near = (coff.Symbol(name, 0, coff.UNDEFINED, 0, coff.EXTERNAL) for name in ('far', 'near'))
            code = coff.Section('.text', coff.CODE, bytes(4) + b'\xe8' + bytes(3) + b'\xe8', 9)
            code.relocations = [(0, far, coff.REL32), (5, near, coff.REL32)]
            coff.write(f'{d}/a.o', self.AMD64, [code], [far, near])
            self.assertEqual(coff.calls(f'{d}/a.o'), {'near'})


class Census(unittest.TestCase):
    def census(self, files):
        """census.py's lines for a tree of files, whose compiler says nothing of any"""
        with tempfile.TemporaryDirectory() as d:
            for path, text in files.items():
                Path(d, path).parent.mkdir(parents=True, exist_ok=True)
                Path(d, path).write_text(text)
            Path(d, 'cc').write_text('#!/bin/sh\nexit 0\n')
            Path(d, 'cc').chmod(0o755)
            subprocess.run([sys.executable, ROOT / 'engine/tools/census.py', './cc', 'census.txt'], cwd=d, check=True)
            return Path(d, 'census.txt').read_text().splitlines()

    def test_a_macro_s_struct_is_its_own_file_s_where_files_share_a_name_and_a_header_s_counts_once(self):
        # a.c's drop_vars holds no pointer and b.c's does, and c.c and d.c lay the header's one struct
        macro = '#define VARS(work) ((struct {} *)&(work)->vars)\n'
        lines = self.census({'src/a.c': 'struct drop_vars {\n    int n;\n};\n' + macro.format('drop_vars'),
                             'src/b.c': 'struct drop_vars {\n    char *name;\n};\n' + macro.format('drop_vars'),
                             'src/seat.h': 'struct seat_vars {\n    void *rider;\n};\n',
                             'src/c.c': '#include "seat.h"\n' + macro.format('seat_vars'),
                             'src/d.c': '#include "seat.h"\n' + macro.format('seat_vars')})
        self.assertIn("       4  a kind's struct laid over a task's vars (4 game)", lines)
        held = ("       2  a struct laid over a task's vars that holds a pointer, so its fields move on the host "
                '(2 game)')
        self.assertIn(held, lines)
        self.assertEqual(lines[lines.index(held) + 1:][:2], ['          src/b.c:4: struct drop_vars',
                                                             '          src/c.c:2: struct seat_vars'])

    def test_a_line_that_opens_with_a_dereference_is_code_and_a_comment_s_line_is_none(self):
        parked = '*(char **)&work->vars = name;'
        lines = self.census({'src/a.c': f'/* {parked}\n * {parked}\n */\nvoid f(struct w *work, char *name)\n{{\n'
                                        f'    {parked}\n}}\n'})
        kept = '       1  a pointer kept in the place of a field of another type (1 game)'
        self.assertIn(kept, lines)
        self.assertEqual(lines[lines.index(kept) + 1], f'          src/a.c:6: {parked}')


class EventAliases(unittest.TestCase):
    def test_the_shim_runs_each_alias_as_the_disc_build_copies_it(self):
        sys.path.insert(0, str(ROOT / 'tools'))
        import images
        shim = (ROOT / 'engine/native/shim.c').read_text()
        found = dict(re.findall(r'\{"(EV\w{4})\.PRS", "(EV\w{4})\.PRS"\}', shim))
        self.assertEqual(found, images.EVENT_ALIASES)


# gdfs.c, compiled into the driver, so that its streams and directories are
# the driver's: each call is the host's, counted, but for the read, seek or
# tell the driver arms, which fails
DISC_DRIVER = r"""
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "native.h"

#ifdef __APPLE__
/* Mach-O's link names the code's bounds by its segment's */
extern char __executable_start[] __asm("segment$start$__TEXT");
extern char etext[] __asm("segment$end$__TEXT");
#endif

/* how many calls from now each one's armed call is, or 0 for none. an armed
 * read fails, an armed short read gives half the bytes asked for and no
 * error, and an armed tell answers told where that is a size, and fails
 * otherwise */
static int read_fails, short_reads, seek_fails, tell_fails;
static long told;
/* the streams open, the directories open, and those opened in all */
static int streams, folders, opened;
/* a host's directory can hold a name twice, in two cases, where a disc's
 * can't. armed, the driver's gives FIRST again as SECOND, after it, and that
 * name opens the file at twice */
#define FIRST "SIZE64.prs"
#define SECOND "size64.PRS"
static const char *twice;
static struct dirent second;

static FILE *host_fopen(const char *path, const char *mode)
{
    const char *name = strrchr(path, '/');
    FILE *file = fopen(twice && name && !strcmp(name + 1, SECOND) ? twice : path, mode);

    streams += file != NULL;
    return file;
}

static int host_fclose(FILE *file)
{
    streams--;
    return fclose(file);
}

static DIR *host_opendir(const char *path)
{
    DIR *folder = opendir(path);

    folders += folder != NULL;
    opened++;
    return folder;
}

static int host_closedir(DIR *folder)
{
    folders--;
    return closedir(folder);
}

static struct dirent *host_readdir(DIR *folder)
{
    static struct dirent given;
    struct dirent *entry;

    if (second.d_name[0]) {
        given = second;
        second.d_name[0] = 0;
        return &given;
    }
    entry = readdir(folder);
    if (twice && entry && !strcmp(entry->d_name, FIRST)) {
        second = *entry;
        strcpy(second.d_name, SECOND);
    }
    return entry;
}

static size_t host_fread(void *to, size_t size, size_t count, FILE *file)
{
    if (read_fails && !--read_fails) {
        /* a write to a stream open to read sets the stream's error */
        fputc(0, file);
        errno = EIO;
        return 0;
    }
    if (short_reads && !--short_reads) {
        return fread(to, size, count / 2, file);
    }
    return fread(to, size, count, file);
}

static int host_fseek(FILE *file, long to, int from)
{
    if (seek_fails && !--seek_fails) {
        errno = ESPIPE;
        return -1;
    }
    return fseek(file, to, from);
}

static long host_ftell(FILE *file)
{
    if (tell_fails && !--tell_fails) {
        if (told) {
            return told;
        }
        errno = EOVERFLOW;
        return -1;
    }
    return ftell(file);
}

#define fopen host_fopen
#define fclose host_fclose
#define opendir host_opendir
#define closedir host_closedir
#define readdir host_readdir
#define fread host_fread
#define fseek host_fseek
#define ftell host_ftell
#include "gdfs.c"
#undef fopen
#undef fclose
#undef opendir
#undef closedir
#undef readdir
#undef fread
#undef fseek
#undef ftell

const char *native_disc;
const struct native_disc_sum native_disc_sums[] = {
SUMS};
const int native_disc_sum_count = sizeof native_disc_sums / sizeof *native_disc_sums;

/* three sectors, a fourth that no read here may reach, and a guard */
static uint8_t buffer[4 * 2048 + 32];

static void fill(void)
{
    memset(buffer, 0xaa, sizeof buffer);
}

/* whether the count bytes at at all hold value */
static int all(size_t at, size_t count, int value)
{
    for (size_t i = 0; i < count; i++) {
        if (buffer[at + i] != value) {
            return 0;
        }
    }
    return 1;
}

/* whether the count bytes at at are the file's from its byte from, as the
 * test wrote them */
static int holds(size_t at, size_t from, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        if (buffer[at + i] != (from + i) % 251 + 1) {
            return 0;
        }
    }
    return 1;
}

/* a read of count sectors, and what it left of the 2053 bytes' file: its
 * result, then whether the buffer holds the file, zeros to its last sector's
 * end, and nothing after */
static void read_tail(struct gdfs *file, int32_t count)
{
    fill();
    printf("read %d", gdFsRead(file, count, buffer));
    printf(" %d %d %d\n", holds(0, 0, 2053), all(2053, 2 * 2048 - 2053, 0),
           all(2 * 2048, sizeof buffer - 2 * 2048, 0xaa));
}

/* count bytes of a file at path, as the test writes one */
static void write_file(const char *path, long count)
{
    FILE *file = fopen(path, "wb");

    for (long i = 0; i < count; i++) {
        fputc((int)(i % 251 + 1), file);
    }
    fclose(file);
}

int main(int argc, char **argv)
{
    const char *test = argv[2];
    char path[1024], words[2048] = "";
    struct gdfs *file;
    /* a call's result. the driver takes it in a statement of its own, and
     * then prints it beside what the call wrote, since C gives the
     * arguments of one printf no order */
    int32_t result;
    int32_t size = -1;

    setvbuf(stdout, NULL, _IONBF, 0);
    native_disc = argv[1];
    if (!strcmp(test, "disc") || !strcmp(test, "twice")) {
        /* the run's third word arms a read, and the second name's file is
         * the root's of 2053 bytes */
        read_fails = argc > 3 ? atoi(argv[3]) : 0;
        snprintf(path, sizeof path, "%s/TAIL.BIN", native_disc);
        twice = !strcmp(test, "twice") ? path : NULL;
        printf("differs %d\n", native_disc_differs(words, sizeof words));
        printf("%s\n", words);
        printf("opened %d folders, left %d and %d streams open\n", opened, folders, streams);
        return 0;
    }
    if (!strcmp(test, "directory")) {
        /* a name the root lacks, a file's, and the game's directory, where
         * its files open and the root's don't, and back */
        printf("lacking %d\n", gdFsChangeDir("nothing"));
        printf("file %d\n", gdFsChangeDir("tail.bin"));
        printf("root %d\n", gdFsOpen("tail.bin", NULL) != NULL);
        printf("directory %d\n", gdFsChangeDir("sonicadv"));
        printf("inside %d %d\n", gdFsOpen("size55.prs", NULL) != NULL, gdFsOpen("tail.bin", NULL) != NULL);
        printf("above %d\n", gdFsChangeDir(".."));
        printf("root %d\n", gdFsOpen("tail.bin", NULL) != NULL);
        printf("left %d folders open\n", folders);
        return 0;
    }
    if (!strcmp(test, "id")) {
        /* a name at the root, and in the game's directory one the disc
         * holds and one it lacks, each spelled as neither the disc nor the
         * host spells it */
        printf("root %08x\n", native_disc_file_id("Tail.bin"));
        gdFsChangeDir("sonicadv");
        printf("inside %08x %08x\n", native_disc_file_id("Size55.prs"), native_disc_file_id("Nothing.pvr"));
        return 0;
    }
    if (!strcmp(test, "size")) {
        seek_fails = 1;
        printf("seek %d\n", gdFsOpen("tail.bin", NULL) != NULL);
        tell_fails = 1;
        printf("tell %d\n", gdFsOpen("tail.bin", NULL) != NULL);
        tell_fails = 1;
        told = (long)INT32_MAX + 1;
        printf("large %d\n", gdFsOpen("tail.bin", NULL) != NULL);
        printf("streams %d\n", streams);
        printf("first %d\n", gdFsOpen("tail.bin", NULL) == &files[0]);
        return 0;
    }
    /* the disc spells each name in capitals */
    file = gdFsOpen(!strcmp(test, "sequence") || !strcmp(test, "stale") ? "long.bin"
                    : !strcmp(test, "short") ? "big.bin" : "tail.bin",
                    NULL);
    printf("open %d\n", file != NULL);
    if (!strcmp(test, "tail")) {
        result = gdFsGetFileSize(file, &size);
        printf("size %d %d\n", result, size);
        read_tail(file, 2);
    } else if (!strcmp(test, "sequence")) {
        fill();
        result = gdFsRead(file, 1, buffer);
        printf("read %d %d\n", result, holds(0, 0, 2048) && all(2048, 3 * 2048, 0xaa));
        result = gdFsReqRd32(file, 1, buffer);
        printf("request %d %d\n", result, holds(0, 2048, 2048) && all(2048, 3 * 2048, 0xaa));
        result = gdFsRead(file, 1, buffer);
        printf("read %d %d\n", result, holds(0, 4096, 5) && all(5, 2043, 0) && all(2048, 3 * 2048, 0xaa));
        printf("request %d\n", gdFsReqRd32(file, 1, buffer));
    } else if (!strcmp(test, "state")) {
        printf("stat %d\n", gdFsGetStat(file));
        read_tail(file, 2);
        printf("stat %d\n", gdFsGetStat(file));
        fill();
        result = gdFsRead(file, 2, buffer);
        printf("at end %d %d\n", result, all(0, sizeof buffer, 0xaa));
        printf("stat %d\n", gdFsGetStat(file));
        gdFsClose(file);
        printf("closed %d %d %d %d\n", gdFsGetStat(file), gdFsGetFileSize(file, &size), gdFsRead(file, 1, buffer),
               gdFsReqRd32(file, 1, buffer));
        printf("null %d %d %d %d\n", gdFsGetStat(NULL), gdFsGetFileSize(NULL, &size), gdFsRead(NULL, 1, buffer),
               gdFsReqRd32(NULL, 1, buffer));
    } else if (!strcmp(test, "over")) {
        read_tail(file, 3);
        gdFsClose(file);
        /* the sectors of a file of 2 GB less a byte */
        file = gdFsOpen("tail.bin", NULL);
        read_tail(file, INT32_MAX / 2048 + 1);
        gdFsClose(file);
        printf("request %d\n", gdFsReqRd32(gdFsOpen("tail.bin", NULL), 3, buffer));
    } else if (!strcmp(test, "count")) {
        /* below zero, and more sectors than any file a handle counts has */
        static const int32_t counts[] = {-1, INT32_MIN, INT32_MAX / 2048 + 2, INT32_MAX};

        fill();
        for (int i = 0; i < 4; i++) {
            result = gdFsRead(file, counts[i], buffer);
            printf("read %d %d\n", result, all(0, sizeof buffer, 0xaa));
        }
        printf("stat %d\n", gdFsGetStat(file));
        read_tail(file, 2);
        printf("stat %d\n", gdFsGetStat(file));
    } else if (!strcmp(test, "short")) {
        snprintf(path, sizeof path, "%s/BIG.BIN", native_disc);
        if (!gdFsGetFileSize(file, &size) || truncate(path, 100) != 0) {
            return 1;
        }
        printf("read %d\n", gdFsRead(file, 2, buffer));
        printf("stat %d\n", gdFsGetStat(file));
        write_file(path, size);
        fill();
        result = gdFsRead(file, 2, buffer);
        printf("read %d %d\n", result, holds(0, 0, 4096) && all(4096, 2 * 2048, 0xaa));
        printf("stat %d\n", gdFsGetStat(file));
    } else if (!strcmp(test, "error")) {
        /* as the loader's queue reads: the request, and then its state */
        read_fails = 1;
        printf("request %d\n", gdFsReqRd32(file, 2, buffer));
        printf("stat %d\n", gdFsGetStat(file));
        read_tail(file, 2);
        printf("stat %d\n", gdFsGetStat(file));
    } else if (!strcmp(test, "seek")) {
        seek_fails = 1;
        printf("read %d\n", gdFsRead(file, 2, buffer));
        printf("stat %d\n", gdFsGetStat(file));
        read_tail(file, 2);
        printf("stat %d\n", gdFsGetStat(file));
    } else if (!strcmp(test, "retry")) {
        /* as the loader's queue serves a request that failed: it closes the
         * file, opens it again and asks again */
        for (int i = 0; i < 2; i++) {
            read_fails = 1;
            printf("request %d\n", gdFsReqRd32(file, 2, buffer));
            printf("stat %d\n", gdFsGetStat(file));
            gdFsClose(file);
            file = gdFsOpen("TAIL.BIN", NULL);
            printf("open %d stat %d\n", file == &files[0], gdFsGetStat(file));
        }
        read_tail(file, 2);
        gdFsClose(file);
        file = gdFsOpen("tail.bin", NULL);
        read_fails = 1;
        printf("read %d\n", gdFsRead(file, 2, buffer));
        fill();
        result = gdFsRead(file, 0, buffer);
        printf("none %d %d\n", result, all(0, sizeof buffer, 0xaa));
        printf("stat %d\n", gdFsGetStat(file));
    } else if (!strcmp(test, "stale")) {
        read_fails = 1;
        printf("read %d\n", gdFsRead(file, 1, buffer));
        fill();
        result = gdFsRead(file, 1, buffer);
        printf("read %d %d\n", result, holds(0, 0, 2048));
        short_reads = 1;
        printf("read %d\n", gdFsRead(file, 1, buffer));
        printf("stat %d\n", gdFsGetStat(file));
        fill();
        result = gdFsRead(file, 1, buffer);
        printf("read %d %d\n", result, holds(0, 2048, 2048));
    } else if (!strcmp(test, "code")) {
        printf("read %d\n", gdFsRead(file, 2, (void *)main));
        printf("stat %d\n", gdFsGetStat(file));
        printf("read %d\n", gdFsRead(file, -1, (void *)main));
        printf("stat %d\n", gdFsGetStat(file));
    }
    return 0;
}
"""
# the SDK's results and states, as shinobi.h numbers them
GDD_ERR_NOTFOUND, GDD_ERR_ILLHNDL, GDD_ERR_NOTDIR, GDD_ERR_MEDIA, GDD_ERR_ILLREQ = -8, -10, -11, -30, -32
GDD_STAT_IDLE, GDD_STAT_COMPLETE, GDD_STAT_ERR = 0, 1, 5


def disc_bytes(count):
    """count bytes of a file on the test's disc, none of them zero"""
    return bytes(i % 251 + 1 for i in range(count))


# the files of the build's disc, as the driver's table sums them: one at the
# root, and in the game's directory one of each size about SHA-256's blocks of
# 64 bytes and the 8 that end the last, and one longer than a read of the sum's
BUILT = {'1ST_READ.BIN': disc_bytes(2053),
         **{f'SONICADV/SIZE{size}.PRS': disc_bytes(size) for size in (0, 55, 56, 63, 64, 65, 119, 120, 70001)}}


# what the driver says last of a check of the test's disc: the root read for
# its files and for the directory's name, and the directory once, however many
# of its files the table names
READ_ONCE = 'opened 3 folders, left 0 and 0 streams open'


class Disc(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        tools = ROOT / 'engine/native'
        cls.built = tempfile.TemporaryDirectory()
        built = Path(cls.built.name)
        sums = ''.join(f'    {{"{path}", "{hashlib.sha256(data).hexdigest()}"}},\n' for path, data in BUILT.items())
        (built / 'driver.c').write_text(DISC_DRIVER.replace('SUMS', sums))
        compiler = os.environ.get('CC') or shutil.which('clang') or 'cc'
        cls.tool = built / 'disc'
        subprocess.run([compiler, '-std=gnu17', '-O1', f'-I{tools}', f'-I{ROOT / "engine/include"}', '-o', cls.tool,
                        built / 'driver.c'], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.built.cleanup()

    def setUp(self):
        # the build's files, the game's directory and its files spelled as
        # no table spells them, and beside them a file that ends 5 bytes into
        # its second sector and one that ends 5 into its third
        self.folder = tempfile.TemporaryDirectory()
        self.disc = Path(self.folder.name)
        (self.disc / 'Sonicadv').mkdir()
        for path, data in BUILT.items():
            self.held(path).write_bytes(data)
        (self.disc / 'TAIL.BIN').write_bytes(disc_bytes(2053))
        (self.disc / 'LONG.BIN').write_bytes(disc_bytes(4101))

    def tearDown(self):
        self.folder.cleanup()

    def held(self, path):
        """where the test's disc holds the build's file or directory at path"""
        return self.disc / path.replace('SONICADV', 'Sonicadv').replace('.PRS', '.prs')

    def run_disc(self, test, *words):
        """the test's exit status, what it said and what gdfs.c reported"""
        run = subprocess.run([self.tool, self.disc, test, *words], capture_output=True, text=True)
        return run.returncode, run.stdout.splitlines(), run.stderr

    def cant_read(self, why, name='tail.bin'):
        return f"sa1: can't read {name} on the disc: {why}\n"

    def test_the_driver_reads_what_a_call_wrote_in_a_statement_after_the_call(self):
        # C gives the arguments of one printf no order. so one that both calls gdfs.c and reads the buffer or the
        # size the call writes may read it first
        mixed = [statement for statement in re.findall(r'printf\((?:[^;"]|"[^"]*")*\);', DISC_DRIVER)
                 if re.search(r'gdFs(?:Read|ReqRd32|GetFileSize)\(', statement) and
                 re.search(r'\b(?:holds|all)\(|(?<!&)\bsize\b', re.sub(r'"[^"]*"', '', statement))]
        self.assertEqual(mixed, [])

    def test_a_change_of_directory_takes_a_directory_alone_and_else_stays_where_it_was(self):
        # the SDK's split: no such name, and a name that is no directory's
        self.assertEqual(self.run_disc('directory'),
                         (0, [f'lacking {GDD_ERR_NOTFOUND}', f'file {GDD_ERR_NOTDIR}', 'root 1', 'directory 0',
                              'inside 1 0', 'above 0', 'root 1', 'left 0 folders open'],
                          'sa1: no file tail.bin on the disc\n'))

    def test_a_file_s_name_id_is_its_path_s_on_the_disc_in_upper_case(self):
        # sa_engine.h's sa_name_id, 32-bit FNV-1a, of the path as the disc spells it: the host here spells the
        # directory Sonicadv and its files in lower case, and the game spells them its own way
        def name_id(path):
            found = 2166136261
            for byte in path.encode():
                found = (found ^ byte) * 16777619 & 0xffffffff
            return found
        self.assertEqual(self.run_disc('id'),
                         (0, [f'root {name_id("TAIL.BIN"):08x}',
                              f'inside {name_id("SONICADV/SIZE55.PRS"):08x} {name_id("SONICADV/NOTHING.PVR"):08x}'],
                          ''))

    def test_a_last_sector_the_file_ends_in_reads_as_zeros_to_its_end(self):
        self.assertEqual(self.run_disc('tail'), (0, ['open 1', 'size 1 2053', 'read 0 1 1 1'], ''))

    def test_each_read_takes_the_sectors_after_the_last_and_a_request_answers_the_sectors_it_read(self):
        self.assertEqual(self.run_disc('sequence'),
                         (0, ['open 1', 'read 0 1', 'request 1 1', 'read 0 1', 'request 0'], ''))

    def test_a_read_over_the_code_writes_nothing_and_a_count_below_zero_is_refused_there_too(self):
        self.assertEqual(self.run_disc('code'),
                         (0, ['open 1', 'read 0', f'stat {GDD_STAT_COMPLETE}', f'read {GDD_ERR_ILLREQ}',
                              f'stat {GDD_STAT_ERR}'], ''))

    def test_a_handle_says_how_its_last_read_ended_and_a_closed_one_is_refused(self):
        refused = f'{GDD_ERR_ILLHNDL} 0 {GDD_ERR_ILLHNDL} {GDD_ERR_ILLHNDL}'
        self.assertEqual(self.run_disc('state'),
                         (0, ['open 1', f'stat {GDD_STAT_IDLE}', 'read 0 1 1 1', f'stat {GDD_STAT_COMPLETE}',
                              'at end 0 1', f'stat {GDD_STAT_COMPLETE}', f'closed {refused}', f'null {refused}'], ''))

    def test_a_read_of_more_sectors_than_the_file_has_ends_at_its_last(self):
        self.assertEqual(self.run_disc('over'), (0, ['open 1', 'read 0 1 1 1', 'read 0 1 1 1', 'request 2'], ''))

    def test_a_count_no_file_has_fails_the_request_and_the_handle_reads_on(self):
        # the queue takes a request for done unless its state is an error
        self.assertEqual(self.run_disc('count'),
                         (0, ['open 1', *[f'read {GDD_ERR_ILLREQ} 1'] * 4, f'stat {GDD_STAT_ERR}', 'read 0 1 1 1',
                              f'stat {GDD_STAT_COMPLETE}'], ''))

    def test_a_file_shorter_than_it_opened_fails_its_read_and_reads_again_once_whole(self):
        # more bytes than a stream buffers, so the read asks the host for
        # sectors the file no longer has
        (self.disc / 'BIG.BIN').write_bytes(disc_bytes((1 << 20) + 5))
        self.assertEqual(self.run_disc('short'),
                         (0, ['open 1', f'read {GDD_ERR_MEDIA}', f'stat {GDD_STAT_ERR}', 'read 0 1',
                              f'stat {GDD_STAT_COMPLETE}'],
                          self.cant_read('it ends before the size it opened with', 'big.bin')))

    def test_a_read_or_a_seek_the_host_fails_fails_the_read_and_the_next_reads_the_same_sectors(self):
        for test, asks, error in ('error', 'request', errno.EIO), ('seek', 'read', errno.ESPIPE):
            with self.subTest(test=test):
                self.assertEqual(self.run_disc(test),
                                 (0, ['open 1', f'{asks} {GDD_ERR_MEDIA}', f'stat {GDD_STAT_ERR}', 'read 0 1 1 1',
                                      f'stat {GDD_STAT_COMPLETE}'], self.cant_read(os.strerror(error))))

    def test_a_file_s_failure_is_one_line_until_a_read_of_it_succeeds_whatever_handle_asks(self):
        # a handle opened again is idle, where its place's last read failed,
        # and a count of no sectors is complete, where the last read failed
        failed = [f'request {GDD_ERR_MEDIA}', f'stat {GDD_STAT_ERR}', f'open 1 stat {GDD_STAT_IDLE}']
        self.assertEqual(self.run_disc('retry'),
                         (0, ['open 1', *failed * 2, 'read 0 1 1 1', f'read {GDD_ERR_MEDIA}', 'none 0 1',
                              f'stat {GDD_STAT_COMPLETE}'], self.cant_read(os.strerror(errno.EIO)) * 2))

    def test_a_read_that_fails_leaves_the_next_failure_its_own_reason(self):
        self.assertEqual(self.run_disc('stale'),
                         (0, ['open 1', f'read {GDD_ERR_MEDIA}', 'read 0 1', f'read {GDD_ERR_MEDIA}',
                              f'stat {GDD_STAT_ERR}', 'read 0 1'],
                          self.cant_read(os.strerror(errno.EIO), 'long.bin') +
                          self.cant_read('it ends before the size it opened with', 'long.bin')))

    def test_a_file_the_host_wont_size_or_sizes_past_2_gb_doesnt_open_and_its_stream_closes(self):
        self.assertEqual(self.run_disc('size'),
                         (0, ['seek 0', 'tell 0', 'large 0', 'streams 0', 'first 1'],
                          self.cant_read(os.strerror(errno.ESPIPE)) + self.cant_read(os.strerror(errno.EOVERFLOW)) +
                          self.cant_read('it is larger than the 2 GB a handle counts')))

    def test_a_disc_that_holds_each_file_as_the_build_read_it_is_the_build_s(self):
        self.assertEqual(self.run_disc('disc'), (0, ['differs 0', '', READ_ONCE], ''))

    def test_a_name_the_directory_holds_twice_is_its_first_entry_s_file_as_an_open_finds_it(self):
        # the second entry's file holds other bytes than the build's
        self.assertEqual(self.run_disc('twice'), (0, ['differs 0', '', READ_ONCE], ''))

    def test_a_disc_that_lacks_a_file_of_the_build_s_differs_by_the_first_it_lacks(self):
        for lacking in (('SONICADV/SIZE120.PRS',), ('SONICADV/SIZE56.PRS', 'SONICADV/SIZE64.PRS'), ('SONICADV',),
                        ('1ST_READ.BIN',)):
            for path in lacking:
                found = self.held(path)
                shutil.rmtree(found) if found.is_dir() else found.unlink()
            first = 'SONICADV/SIZE0.PRS' if lacking[0] == 'SONICADV' else lacking[0]
            with self.subTest(first=first):
                status, said, reported = self.run_disc('disc')
                self.assertEqual((status, said[:2], reported),
                                 (0, ['differs 1', f'the disc at {self.disc} lacks {first}'], ''))

    def test_a_disc_that_holds_other_bytes_of_a_file_of_the_build_s_differs_by_the_first_such(self):
        # a last byte that differs, a byte less and a byte more, in files of
        # one block's sum, of two and of many reads; then no bytes, where a
        # later file is missing and an earlier one is whole
        whole = BUILT['SONICADV/SIZE70001.PRS']
        words = f'the disc at {self.disc} holds another %s than the disc this program was built from'
        for path, data in (('SONICADV/SIZE70001.PRS', whole[:-1] + b'\0'), ('SONICADV/SIZE70001.PRS', whole[:-1]),
                           ('SONICADV/SIZE70001.PRS', whole + b'\1'), ('SONICADV/SIZE120.PRS', disc_bytes(119)),
                           ('SONICADV/SIZE55.PRS', disc_bytes(56)), ('SONICADV/SIZE0.PRS', b'\1'),
                           ('1ST_READ.BIN', b'')):
            with self.subTest(path=path, size=len(data)):
                self.held(path).write_bytes(data)
                self.assertEqual(self.run_disc('disc'), (0, ['differs 1', words % path, READ_ONCE], ''))
                self.held(path).write_bytes(BUILT[path])
        self.held('SONICADV/SIZE119.PRS').unlink()
        self.held('SONICADV/SIZE64.PRS').write_bytes(b'')
        self.assertEqual(self.run_disc('disc')[1][1], words % 'SONICADV/SIZE64.PRS')

    def test_a_disc_whose_file_the_host_fails_a_read_of_differs_and_the_reason_is_the_host_s(self):
        # the root's one file of the table is the first read
        self.assertEqual(self.run_disc('disc', '1'),
                         (0, ['differs 1', f"can't read 1ST_READ.BIN on the disc at {self.disc}: "
                              f'{os.strerror(errno.EIO)}', READ_ONCE], ''))


CARD_DRIVER = r"""
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include "native.h"
struct disk_info {
    uint8_t volume[32];
    uint16_t blocks, user_blocks, free_blocks, free_user_blocks, exe_blocks, free_exe_blocks, block_size, icon;
    uint8_t time[8];
};
int32_t buDeleteFile(int32_t, const char *);
int32_t buFindExecFile(int32_t, char *);
int32_t buFindFirstFile(int32_t, char *);
int32_t buFindNextFile(int32_t, char *);
int32_t buGetDiskFree(int32_t, int32_t);
int32_t buGetDiskInfo(int32_t, struct disk_info *);
int32_t buGetFileSize(int32_t, const char *);
int32_t buIsExistFile(int32_t, const char *);
int32_t buLoadFile(int32_t, const char *, void *, uint32_t);
int32_t buLoadFileEx(int32_t, const char *, void *, uint32_t, uint32_t);
int32_t buRewriteExecFile(int32_t, const char *, const void *, uint32_t, uint32_t);
int32_t buSaveExecFile(int32_t, const char *, const void *, uint32_t, const void *, uint32_t);
int32_t buSaveFile(int32_t, const char *, const void *, uint32_t, const void *, uint32_t);
int32_t buStat(int32_t);
void buExecServer(void);
int32_t buInit(int32_t, int32_t, void *, void (*)(void));
void buSetCompleteCallback(int (*)(int32_t, int32_t, int32_t, uint32_t));
static int mounts;
/* the call the library last ended but a mount, and its status */
static int ended[2];
/* room for a block more than a card's files hold */
static uint8_t blocks[201 * 512];
static int complete(int32_t d, int32_t o, int32_t s, uint32_t p)
{
    if (o == 1) {
        mounts++;
    } else {
        ended[0] = o, ended[1] = s;
    }
    return (void)d, (void)p, 0;
}
/* the nth of blocks is bytes of fill + n */
static void fill_blocks(int fill)
{
    for (int n = 0; n < 201; n++) {
        memset(blocks + n * 512, fill + n, 512);
    }
}
#ifdef STORED
/* an engine's storage, as far as the card reads it, whose next writes fail while failing counts them */
#include "sa_engine.h"
static char engine;
static int failing;
sa_result sa_storage_read(sa_engine *of, const char *name, void *buffer, uint32_t bytes, sa_handle *request)
{
    FILE *in = fopen(name, "rb");
    size_t held;
    (void)of, (void)request;
    if (!in) {
        return SA_ERR_MISSING;
    }
    held = fread(buffer, 1, bytes, in);
    held += fgetc(in) != EOF;
    fclose(in);
    return held > bytes ? SA_ERR_FULL : SA_OK;
}
sa_result sa_storage_write(sa_engine *of, const char *name, const void *data, uint32_t bytes, sa_handle *request)
{
    FILE *out;
    (void)of, (void)request;
    if (failing) {
        failing--;
        return SA_ERR_IO;
    }
    out = fopen(name, "wb");
    fwrite(data, 1, bytes, out);
    fclose(out);
    return SA_OK;
}
#endif
static void list(void)
{
    char name[16];
    for (int found = buFindFirstFile(0, name); found >= 0; found = buFindNextFile(0, name)) {
        printf("file %s %d\n", name, buGetFileSize(0, name));
    }
}
int main(int argc, char **argv)
{
    char data[1024] = {0};
    char name[16];
    struct disk_info info;
    struct rlimit limit;
#ifdef STORED
    native_card_engine = (struct sa_engine *)&engine;
#endif
    native_card_open(argv[1]);
    printf("before %d\n", buStat(0) == 0);
    buSetCompleteCallback(complete);
    buExecServer();
    printf("after %d\n", buStat(0) == 0);
    list();
    buLoadFile(0, "GAME_FILE", data, 2);
    printf("bytes %d %d\n", data[0], data[512]);
    buLoadFileEx(0, "GAME_FILE", data + 512, 1, 1);
    printf("from 1 %d\n", data[512]);
    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "again")) {
            buInit(0, 1, NULL, NULL);
            printf("cleared %d\n", buStat(0) == 0);
            buExecServer();
            printf("mounts %d\n", mounts);
        } else if (!strcmp(argv[i], "start")) {
            buInit(0, 1, NULL, NULL);
        } else if (!strcmp(argv[i], "serve")) {
            buExecServer();
        } else if (!strcmp(argv[i], "info")) {
            /* the call's answer, and its free blocks, 61166 where it left them */
            memset(&info, 238, sizeof info);
            printf("info %d", buGetDiskInfo(0, &info));
            printf(" %d\n", info.free_blocks);
        } else if (!strcmp(argv[i], "list")) {
            list();
            buGetDiskInfo(0, &info);
            printf("free %d\n", info.free_blocks);
        } else if (!strcmp(argv[i], "holds") && i + 2 < argc) {
            /* holds DRIVE NAME: whether the drive's card holds the file */
            printf("holds %d\n", buIsExistFile(atoi(argv[i + 1]), argv[i + 2]));
            i += 2;
        } else if (!strcmp(argv[i], "room") && i + 1 < argc) {
            /* room DRIVE: the blocks the drive's card has for a new file, and for a new game */
            printf("room %d %d\n", buGetDiskFree(atoi(argv[i + 1]), 0), buGetDiskFree(atoi(argv[i + 1]), 1));
            i++;
        } else if (!strcmp(argv[i], "load") && i + 3 < argc) {
            /* load NAME FIRST COUNT: the first byte of each of four blocks, 238 where the read left it */
            memset(blocks, 238, sizeof blocks);
            printf("loaded %d", buLoadFileEx(0, argv[i + 1], blocks, strtoul(argv[i + 2], NULL, 0),
                                             strtoul(argv[i + 3], NULL, 0)));
            printf(" %d %d %d %d\n", blocks[0], blocks[512], blocks[1024], blocks[1536]);
            i += 3;
        } else if ((!strcmp(argv[i], "save") || !strcmp(argv[i], "save-game")) && i + 3 < argc) {
            /* save NAME COUNT FILL: the file's nth block is bytes of FILL + n. save-game: it is the card's game */
            fill_blocks(atoi(argv[i + 3]));
            printf("saved %d\n", (argv[i][4] ? buSaveExecFile : buSaveFile)(0, argv[i + 1], blocks,
                                                                            strtoul(argv[i + 2], NULL, 0), NULL, 0));
            i += 3;
        } else if (!strcmp(argv[i], "rewrite") && i + 4 < argc) {
            /* rewrite NAME FIRST COUNT FILL: COUNT blocks over the game's from FIRST on, the nth bytes of FILL + n */
            fill_blocks(atoi(argv[i + 4]));
            printf("rewrote %d\n", buRewriteExecFile(0, argv[i + 1], blocks, strtoul(argv[i + 2], NULL, 0),
                                                     strtoul(argv[i + 3], NULL, 0)));
            i += 4;
        } else if (!strcmp(argv[i], "game") && i + 1 < argc) {
            /* game DRIVE: the call's answer, and the name of the card's game, none where the call left it */
            strcpy(name, "none");
            printf("game %d", buFindExecFile(atoi(argv[i + 1]), name));
            printf(" %s\n", name);
            i++;
        } else if (!strcmp(argv[i], "delete") && i + 1 < argc) {
            printf("deleted %d\n", buDeleteFile(0, argv[++i]));
        } else if (!strcmp(argv[i], "ended")) {
            printf("ended %d %d\n", ended[0], ended[1]);
        } else if ((!strcmp(argv[i], "limit") || !strcmp(argv[i], "cut")) && i + 1 < argc) {
            /* limit BYTES: a write that would grow a file past BYTES fails, or past any size with none.
               cut BYTES: it ends the run where it stands */
            getrlimit(RLIMIT_FSIZE, &limit);
            limit.rlim_cur = strcmp(argv[i + 1], "none") ? strtoul(argv[i + 1], NULL, 0) : limit.rlim_max;
            setrlimit(RLIMIT_FSIZE, &limit);
            signal(SIGXFSZ, argv[i][0] == 'c' ? SIG_DFL : SIG_IGN);
            i++;
        } else if (!strcmp(argv[i], "shut")) {
            /* a directory takes the card's place, whose name no file can take from it */
            remove(argv[1]);
            mkdir(argv[1], 0700);
#ifdef STORED
        } else if (!strcmp(argv[i], "fail") && i + 1 < argc) {
            failing = atoi(argv[++i]);
#endif
        } else {
            return 3;
        }
    }
    return 0;
}
"""

BLOCK = 512
ROOT_BLOCK, FAT_BLOCK, DIRECTORY_BLOCK = 255, 254, 253
FREE, END, BAD = 0xfffc, 0xfffa, 0xffff


def link(image, block, to):
    """block's word of the FAT"""
    struct.pack_into('<H', image, FAT_BLOCK * BLOCK + block * 2, to)


def file_at(image, slot, name, chain, blocks=None, kind=0x33, directory=None):
    """a file in a slot of the directory, which directory names the block of where it moved: its blocks, chained in
    order, the nth holding n + 1, and the count its entry gives, theirs where blocks gives none"""
    entry = (DIRECTORY_BLOCK - slot // 16 if directory is None else directory) * BLOCK + slot % 16 * 32
    image[entry] = kind
    struct.pack_into('<H', image, entry + 2, chain[0] if chain else END)
    image[entry + 4:entry + 16] = name.encode().ljust(12, b'\0')
    struct.pack_into('<H', image, entry + 0x18, len(chain) if blocks is None else blocks)
    for n, block in enumerate(chain):
        link(image, block, chain[n + 1] if n + 1 < len(chain) else END)
        image[block * BLOCK] = n + 1


def file_of(image, name):
    """what a card whose directory runs down from its first block holds of name's file: its entry, its chain and
    its blocks' bytes, or None"""
    for at in range(DIRECTORY_BLOCK * BLOCK, (DIRECTORY_BLOCK - 13) * BLOCK, -BLOCK):
        for entry in (bytes(image[at + n:at + n + 32]) for n in range(0, BLOCK, 32)):
            if entry[0] and entry[4:16].rstrip(b'\0') == name.encode():
                chain = list(struct.unpack_from('<H', entry, 2))
                for _ in range(struct.unpack_from('<H', entry, 0x18)[0] - 1):
                    chain += struct.unpack_from('<H', image, FAT_BLOCK * BLOCK + chain[-1] * 2)
                return entry, chain, [bytes(image[block * BLOCK:(block + 1) * BLOCK]) for block in chain]
    return None


def card_image():
    """a card as a console formats one, with one file of two blocks, 199 and then 198"""
    image = bytearray(256 * BLOCK)
    root = ROOT_BLOCK * BLOCK
    image[root:root + 16] = b'\x55' * 16
    struct.pack_into('<HHHHHH', image, root + 0x46, FAT_BLOCK, 1, DIRECTORY_BLOCK, 13, 0, 200)
    for block in range(256):
        link(image, block, FREE)
    for block in range(DIRECTORY_BLOCK, DIRECTORY_BLOCK - 13, -1):
        link(image, block, block - 1 if block > DIRECTORY_BLOCK - 12 else END)
    link(image, FAT_BLOCK, END)
    link(image, ROOT_BLOCK, END)
    file_at(image, 0, 'GAME_FILE', [199, 198])
    return image


class Card(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        tools = ROOT / 'engine/native'
        cls.built = tempfile.TemporaryDirectory()
        built = Path(cls.built.name)
        (built / 'driver.c').write_text(CARD_DRIVER)
        compiler = os.environ.get('CC') or shutil.which('clang') or 'cc'
        cls.tool = built / 'card'
        cls.stored = built / 'stored'
        cls.windows = built / 'windows'
        cls.windows_stored = built / 'windows-stored'
        # a run without an engine, which writes the card's file itself, and one whose engine has storage; and each
        # as Windows builds it, where the old card goes before the new one takes its name, on this host's files
        for tool, defines in ((cls.tool, []), (cls.stored, ['-DSTORED']), (cls.windows, ['-D_WIN32']),
                              (cls.windows_stored, ['-D_WIN32', '-DSTORED'])):
            subprocess.run([compiler, '-std=gnu17', '-O1', f'-I{tools}', f'-I{ROOT / "engine/include"}', *defines,
                            '-o', tool, built / 'driver.c', tools / 'card.c'], check=True)
        cls.path = built / 'card.bin'
        cls.fresh = built / 'card.bin.new'

    @classmethod
    def tearDownClass(cls):
        cls.built.cleanup()

    def setUp(self):
        self.fresh.unlink(missing_ok=True)

    def run_card(self, *args, tool=None):
        return subprocess.run([tool or self.tool, self.path, *args], check=True, capture_output=True,
                              text=True).stdout.splitlines()

    def commands(self, *args, tool=None):
        """what a run says of its commands, past the lines every run begins with"""
        said = self.run_card(*args, tool=tool)
        return said[next(n for n, line in enumerate(said) if line.startswith('from 1 ')) + 1:]

    def new_card(self, *files):
        """a new card with each file a save gave it, as name, blocks and fill: its image"""
        self.path.unlink(missing_ok=True)
        self.assertEqual(self.commands(*(word for file in files for word in ('save', *file))),
                         ['saved 0'] * len(files))
        return self.path.read_bytes()

    def refused(self, image, said, tool=None):
        """the run ends at image with said, and leaves the file as it was"""
        self.path.write_bytes(image)
        ran = subprocess.run([tool or self.tool, self.path, 'list'], capture_output=True, text=True)
        self.assertEqual((ran.returncode, ran.stderr), (2, f'sa1: {said}\n'))
        self.assertEqual(self.path.read_bytes(), bytes(image))

    def damaged(self, image, why):
        self.refused(image, f'a damaged card at {self.path}: {why}')

    def test_a_card_mounts_at_the_first_server_call_and_reads_by_its_chain(self):
        self.path.write_bytes(card_image())
        self.assertEqual(self.run_card(), ['before 0', 'after 1', 'file GAME_FILE 2', 'bytes 1 2', 'from 1 2'])

    def test_a_card_mounts_again_as_the_library_starts_again(self):
        self.path.write_bytes(card_image())
        self.assertEqual(self.run_card('again')[-2:], ['cleared 0', 'mounts 2'])

    def test_a_card_gives_its_disk_info_only_while_mounted(self):
        # the library's start unmounts it, and until the server mounts it again the call leaves its record alone
        self.path.write_bytes(card_image())
        self.assertEqual(self.commands('info', 'start', 'info', 'serve', 'info'),
                         ['info 0 198', 'info -255 61166', 'info 0 198'])

    def test_a_card_tells_the_files_it_holds_and_the_room_each_kind_has(self):
        # a new card holds neither the save nor a voice file, and every block is room: its 200 for a file, and
        # the first 128 of them for a game
        save, voice, room = ('holds', '0', 'SONICADV_SYS'), ('holds', '0', 'SONICADV_V01'), ('room', '0')
        self.new_card()
        self.assertEqual(self.commands(*save, *voice, *room), ['holds -251', 'holds -251', 'room 200 128'])
        # the save takes its blocks from the card's last down, off a file's room and none of a game's
        self.assertEqual(self.commands('save', 'SONICADV_SYS', '10', '7', *save, *voice, *room),
                         ['saved 0', 'holds 0', 'holds -251', 'room 190 128'])
        self.assertEqual(self.commands(*save, *voice, *room), ['holds 0', 'holds -251', 'room 190 128'])
        # a save in its place gives back the blocks it has no use for
        self.assertEqual(self.commands('save', 'SONICADV_SYS', '4', '7', *room), ['saved 0', 'room 196 128'])
        # a drive with no card holds no file and has no room, and nor has the card until the server mounts it
        self.assertEqual(self.commands('holds', '1', 'SONICADV_SYS', 'room', '1', 'start', *save, *room,
                                       'serve', *save),
                         ['holds -255', 'room -255 -255', 'holds -255', 'room -255 -255', 'holds 0'])
        # a game's room ends at the first block a file holds, and a full card has none of either kind
        image = card_image()
        file_at(image, 1, 'LOW', [5])
        self.path.write_bytes(image)
        self.assertEqual(self.commands(*room, 'save', 'REST', '197', '9', *room),
                         ['room 197 5', 'saved 0', 'room 0 0'])

    def test_a_card_takes_one_game_in_its_first_blocks_and_rewrites_it_where_it_lies(self):
        # a card without a game names none and rewrites none, and a drive without a card names none either
        before = self.new_card(('SONICADV_SYS', '10', '7'))
        self.assertEqual(self.commands('game', '0', 'game', '1', 'rewrite', 'SONICADV__VM', '24', '1', '99'),
                         ['game -251 none', 'game -255 none', 'rewrote -251'])
        self.assertEqual(self.path.read_bytes(), before)
        # a game's blocks are the card's first in order, under an entry of a game's kind whose header is a block in
        self.assertEqual(self.commands('save-game', 'SONICADV__VM', '128', '20', 'ended', 'game', '0', 'room', '0'),
                         ['saved 0', 'ended 13 0', 'game 0 SONICADV__VM', 'room 62 0'])
        entry, chain, blocks = file_of(self.path.read_bytes(), 'SONICADV__VM')
        self.assertEqual((entry[0], entry[0x1a:0x1c], chain, [block[0] for block in blocks]),
                         (0xcc, b'\1\0', list(range(128)), list(range(20, 148))))
        # a rewrite changes the blocks it names and no other, and the next run reads the game as this one left it
        self.assertEqual(self.commands('rewrite', 'SONICADV__VM', '24', '1', '99', 'ended',
                                       'load', 'SONICADV__VM', '23', '3'),
                         ['rewrote 0', 'ended 20 0', 'loaded 0 43 99 45 238'])
        self.assertEqual(self.commands('game', '0', 'load', 'SONICADV__VM', '23', '3',
                                       'load', 'SONICADV_SYS', '0', '2'),
                         ['game 0 SONICADV__VM', 'loaded 0 43 99 45 238', 'loaded 0 7 8 238 238'])
        # the card refuses a second game, by its room and then by the game it holds, and a game of a name it holds.
        # it refuses a rewrite past the game's end and one of a file that is no game. each leaves the card's file
        held = self.path.read_bytes()
        self.assertEqual(self.commands('save-game', 'OTHER', '1', '9', 'save-game', 'OTHER', '0', '9',
                                       'save-game', 'SONICADV__VM', '0', '9',
                                       'rewrite', 'SONICADV__VM', '127', '2', '9',
                                       'rewrite', 'SONICADV_SYS', '0', '1', '9', 'ended'),
                         ['saved -252', 'saved -247', 'saved -250', 'rewrote -256', 'rewrote -245', 'ended 20 -245'])
        self.assertEqual(self.path.read_bytes(), held)

    def test_a_deleted_file_gives_the_card_its_blocks_and_its_entry_back(self):
        before = self.new_card(('SONICADV_SYS', '10', '7'))
        self.assertEqual(self.commands('save-game', 'SONICADV__VM', '128', '20', 'save', 'OTHER', '3', '9',
                                       'room', '0'), ['saved 0', 'saved 0', 'room 59 0'])
        # the game's blocks are room again, for a file and for a game, and the card names no game
        self.assertEqual(self.commands('delete', 'SONICADV__VM', 'ended', 'room', '0', 'game', '0', 'list'),
                         ['deleted 0', 'ended 7 0', 'room 187 128', 'game -251 none',
                          'file SONICADV_SYS 10', 'file OTHER 3', 'free 187'])
        # the next run deletes another file, and none that isn't there. the save's blocks stay its own
        self.assertEqual(self.commands('delete', 'OTHER', 'delete', 'OTHER', 'ended', 'load', 'SONICADV_SYS', '0', '2'),
                         ['deleted 0', 'deleted -251', 'ended 7 -251', 'loaded 0 7 8 238 238'])
        # with both gone the directory and the FAT are what they were before either
        self.assertEqual(self.path.read_bytes()[241 * BLOCK:], before[241 * BLOCK:])
        # a delete that the engine's storage can't keep leaves the file on the card and in its file
        held = self.path.read_bytes()
        self.assertEqual(self.commands('fail', '1', 'delete', 'SONICADV_SYS', 'holds', '0', 'SONICADV_SYS',
                                       tool=self.stored), ['deleted -192', 'holds 0'])
        self.assertEqual(self.path.read_bytes(), held)

    def test_a_saved_file_stays_on_the_card(self):
        self.path.write_bytes(card_image())
        self.assertEqual(self.run_card('save', 'NEW_FILE', '2', '7')[-1], 'saved 0')
        self.assertIn('file NEW_FILE 2', self.run_card())

    def test_a_file_s_blocks_are_each_the_highest_free_so_its_chain_runs_down(self):
        # as a console's library places them, past a block another file holds or a write spoiled
        image = card_image()
        link(image, 196, BAD)
        self.path.write_bytes(image)
        self.assertEqual(self.commands('save', 'NEW_FILE', '3', '7'), ['saved 0'])
        made = self.path.read_bytes()
        _, chain, blocks = file_of(made, 'NEW_FILE')
        self.assertEqual((chain, [block[0] for block in blocks]), ([197, 195, 194], [7, 8, 9]))
        self.assertEqual(struct.unpack_from('<H', made, FAT_BLOCK * BLOCK + 194 * 2), (END,))
        # a file in the place of one of its name takes that one's blocks among the free
        self.assertEqual(self.commands('save', 'GAME_FILE', '3', '4'), ['saved 0'])
        self.assertEqual(file_of(self.path.read_bytes(), 'GAME_FILE')[1], [199, 198, 193])

    def test_a_card_not_there_is_made_formatted_and_takes_a_save(self):
        self.path.unlink(missing_ok=True)
        said = self.run_card('save', 'NEW_FILE', '2', '7')
        self.assertEqual((said[1], said[-1]), ('after 1', 'saved 0'))
        made = self.path.read_bytes()
        self.assertEqual(len(made), 256 * BLOCK)
        # the root holds its mark, from 0x40 the twelve words of its layout, and nothing else
        layout = struct.pack('<12H', ROOT_BLOCK, 0, ROOT_BLOCK, FAT_BLOCK, 1, DIRECTORY_BLOCK, 13, 0, 200, 31, 0, 128)
        root = (b'\x55' * 16).ljust(0x40, b'\0') + layout
        self.assertEqual(made[ROOT_BLOCK * BLOCK:], root.ljust(BLOCK, b'\0'))
        self.assertIn('file NEW_FILE 2', self.run_card())

    def test_a_file_without_a_formatted_card_is_refused(self):
        for image in (bytes(256 * BLOCK), b'', card_image()[:-1], card_image() + b'\0'):
            with self.subTest(bytes=len(image)):
                self.refused(image, f'no card at {self.path}')

    def test_a_root_whose_layout_no_card_has_is_refused(self):
        fat, long, user, directory = ("its FAT doesn't lie below the root", "its FAT isn't one block long",
                                      'its user blocks reach its FAT', "its directory's chain is broken")
        for word, value, why in ((0x46, 65535, fat), (0x46, ROOT_BLOCK, fat), (0x48, 0, long), (0x48, 2, long),
                                 (0x50, 255, user), (0x50, 65535, user),
                                 (0x50, 242, 'its directory lies among its user blocks'),
                                 (0x4a, 65535, directory), (0x4a, ROOT_BLOCK, directory), (0x4a, FAT_BLOCK, directory),
                                 (0x4c, 0, directory), (0x4c, 12, directory), (0x4c, 14, directory),
                                 (0x4c, 65535, directory)):
            with self.subTest(word=hex(word), value=value):
                image = card_image()
                struct.pack_into('<H', image, ROOT_BLOCK * BLOCK + word, value)
                self.damaged(image, why)
        # no block of directory, and an end mark where its first would be
        image = card_image()
        struct.pack_into('<HH', image, ROOT_BLOCK * BLOCK + 0x4a, END, 0)
        self.damaged(image, directory)

    def test_a_directory_whose_chain_is_broken_is_refused(self):
        broken = "its directory's chain is broken"
        for block, to, why in ((250, END, broken), (250, FREE, broken), (250, 253, broken), (250, FAT_BLOCK, broken),
                               (250, ROOT_BLOCK, broken), (241, 240, broken),
                               (250, 5, 'its directory lies among its user blocks')):
            with self.subTest(block=block, to=to):
                image = card_image()
                link(image, block, to)
                self.damaged(image, why)
        # a chain of the directory's own length, one of whose blocks is the FAT's or the root's
        for through in (FAT_BLOCK, ROOT_BLOCK):
            with self.subTest(through=through):
                image = card_image()
                link(image, 250, through)
                link(image, through, 248)
                self.damaged(image, broken)

    def test_a_file_whose_chain_is_broken_is_refused(self):
        def chain(*links, blocks=2, first=199):
            image = card_image()
            file_at(image, 0, 'GAME_FILE', [199, 198], blocks)
            struct.pack_into('<H', image, DIRECTORY_BLOCK * BLOCK + 2, first)
            for block, to in links:
                link(image, block, to)
            return image
        other = card_image()
        file_at(other, 1, 'OTHER', [198])
        # an entry past those a card reads, which the directory's blocks still hold
        past = card_image()
        file_at(past, 200, 'PAST', [], blocks=1)
        for case, image, why in (('short', chain(blocks=3), "a file's chain ends before the file"),
                                 ('long', chain(blocks=1), "a file's chain doesn't end with the file"),
                                 ('free', chain((198, FREE)), "a file's chain doesn't end with the file"),
                                 ('through free', chain((199, FREE)), "a file's chain leaves the user blocks"),
                                 ('bad', chain((199, BAD)), "a file's chain leaves the user blocks"),
                                 ('directory', chain((199, DIRECTORY_BLOCK)), "a file's chain leaves the user blocks"),
                                 ('past the user blocks', chain((199, 200)), "a file's chain leaves the user blocks"),
                                 ('no first', chain(first=END), "a file's chain ends before the file"),
                                 ('first off the card', chain(first=4660), "a file's chain leaves the user blocks"),
                                 ('circle', chain((198, 199)), "a file's chain doesn't end with the file"),
                                 ('longer circle', chain((198, 199), blocks=3), "a file's chain takes a block twice"),
                                 ('itself', chain((199, 199)), "a file's chain takes a block twice")):
            with self.subTest(case=case):
                self.damaged(image, f'{why}: GAME_FILE')
        self.damaged(other, "a file's chain takes a block twice: OTHER")
        self.damaged(past, "a file's chain ends before the file: PAST")

    def test_a_directory_a_console_moved_is_read_by_its_chain(self):
        # a console that can't write a directory block chains another in its place, from the spare blocks
        image = card_image()
        link(image, 253, 240)
        link(image, 240, 251)
        link(image, 252, BAD)
        file_at(image, 16, 'MOVED', [197], directory=240)
        self.path.write_bytes(image)
        self.assertEqual(self.commands('list', 'load', 'MOVED', '0', '1', 'save', 'NEW_FILE', '2', '7'),
                         ['file GAME_FILE 2', 'file MOVED 1', 'free 197', 'loaded 0 1 238 238 238', 'saved 0'])
        self.assertEqual(self.commands('list'), ['file GAME_FILE 2', 'file NEW_FILE 2', 'file MOVED 1', 'free 195'])

    def test_what_a_console_leaves_on_a_card_mounts(self):
        # a game from the first block up, a block whose write failed, and a file with no block written
        image = card_image()
        file_at(image, 1, 'A_GAME', [0, 1, 2], kind=0xcc)
        link(image, 100, BAD)
        file_at(image, 2, 'NO_BLOCKS', [])
        struct.pack_into('<H', image, DIRECTORY_BLOCK * BLOCK + 2 * 32 + 2, 0xffff)
        self.path.write_bytes(image)
        self.assertEqual(self.commands('load', 'A_GAME', '1', '2', 'list'),
                         ['loaded 0 2 3 238 238', 'file GAME_FILE 2', 'file A_GAME 3', 'file NO_BLOCKS 0', 'free 194'])

    def test_an_entry_s_words_are_read_as_far_as_its_file_runs(self):
        # an entry of no type is no file, and a file of no blocks has no first, whatever block the word names
        image = card_image()
        file_at(image, 1, 'NO_FILE', [], blocks=2, kind=0)
        file_at(image, 2, 'NO_BLOCKS', [])
        for slot in (1, 2):
            struct.pack_into('<H', image, DIRECTORY_BLOCK * BLOCK + slot * 32 + 2, 199)
        self.path.write_bytes(image)
        self.assertEqual(self.commands('list', 'load', 'NO_BLOCKS', '0', '1', 'load', 'NO_FILE', '0', '1'),
                         ['file GAME_FILE 2', 'file NO_BLOCKS 0', 'free 198', 'loaded 0 238 238 238 238',
                          'loaded -251 238 238 238 238'])
        # and a file in its place frees no block of another's
        self.assertEqual(self.commands('save', 'NO_BLOCKS', '1', '7', 'list', 'load', 'GAME_FILE', '0', '2'),
                         ['saved 0', 'file GAME_FILE 2', 'file NO_BLOCKS 1', 'free 197', 'loaded 0 1 2 238 238'])
        self.assertEqual(self.commands('load', 'NO_BLOCKS', '0', '1', 'load', 'GAME_FILE', '0', '2'),
                         ['loaded 0 7 238 238 238', 'loaded 0 1 2 238 238'])

    def test_a_read_ends_with_its_file(self):
        self.path.write_bytes(card_image())
        self.assertEqual(self.commands('load', 'GAME_FILE', '0', '4', 'load', 'GAME_FILE', '1', '4294967295',
                                       'load', 'GAME_FILE', '2', '1', 'load', 'NO_FILE', '0', '1'),
                         ['loaded 0 1 2 238 238', 'loaded 0 2 238 238 238', 'loaded 0 238 238 238 238',
                          'loaded -251 238 238 238 238'])

    def test_a_file_that_outgrows_the_card_leaves_the_one_it_would_replace(self):
        # A's two blocks and B's 198 fill the card
        before = self.new_card(('A', '2', '10'), ('B', '198', '20'))
        self.assertEqual(self.commands('save', 'A', '3', '30', 'list', 'load', 'A', '0', '2'),
                         ['saved -252', 'file A 2', 'file B 198', 'free 0', 'loaded 0 10 11 238 238'])
        self.assertEqual(self.path.read_bytes(), before)
        # the next save that the card takes writes A as it was
        self.assertEqual(self.commands('save', 'A', '3', '30', 'save', 'B', '198', '40', 'list'),
                         ['saved -252', 'saved 0', 'file A 2', 'file B 198', 'free 0'])
        self.assertEqual(file_of(self.path.read_bytes(), 'A'), file_of(before, 'A'))
        self.assertEqual(self.commands('load', 'A', '0', '2', 'load', 'B', '0', '1'),
                         ['loaded 0 10 11 238 238', 'loaded 0 40 238 238 238'])

    def test_a_file_fits_while_the_card_has_a_block_for_each_of_its_own(self):
        self.new_card(('A', '200', '1'))
        self.assertEqual(self.commands('list', 'load', 'A', '199', '1'),
                         ['file A 200', 'free 0', 'loaded 0 200 238 238 238'])
        # a block more than the card has is refused, new or in a file's place, and so is a count past a word
        before = self.new_card(('A', '2', '10'))
        self.assertEqual(self.commands('save', 'B', '199', '20', 'save', 'A', '201', '30',
                                       'save', 'A', '4294967295', '30', 'save', 'B', '65538', '20',
                                       'list', 'load', 'A', '0', '2'),
                         ['saved -252'] * 4 + ['file A 2', 'free 198', 'loaded 0 10 11 238 238'])
        self.assertEqual(self.path.read_bytes(), before)
        self.assertEqual(self.commands('save', 'B', '198', '20', 'list'),
                         ['saved 0', 'file A 2', 'file B 198', 'free 0'])
        self.assertEqual(self.commands('list'), ['file A 2', 'file B 198', 'free 0'])

    def test_a_file_takes_the_place_of_one_of_its_name(self):
        # on a full card the old file's blocks are the new one's room: as many, fewer, more, and all there are
        self.new_card(('A', '3', '10'), ('B', '197', '20'))
        for blocks, fill, free in ((3, 30, 0), (1, 40, 2), (2, 50, 1), (3, 60, 0)):
            with self.subTest(blocks=blocks):
                held = ['file A %d' % blocks, 'file B 197', 'free %d' % free,
                        'loaded 0 ' + ' '.join(str(fill + n if n < blocks else 238) for n in range(4)),
                        'loaded 0 20 238 238 238']
                look = ('list', 'load', 'A', '0', '4', 'load', 'B', '0', '1')
                self.assertEqual(self.commands('save', 'A', str(blocks), str(fill), *look), ['saved 0', *held])
                self.assertEqual(self.commands(*look), held)

    def test_a_directory_of_one_block_holds_its_16_files(self):
        image = card_image()
        struct.pack_into('<H', image, ROOT_BLOCK * BLOCK + 0x4c, 1)
        link(image, DIRECTORY_BLOCK, END)
        for slot in range(1, 15):
            file_at(image, slot, 'F%03d' % slot, [])
        self.path.write_bytes(image)
        said = self.commands('save', 'LAST', '1', '7', 'save', 'NEW_FILE', '1', '8', 'list')
        self.assertEqual((said[:2], len(said[2:-1]), said[-2:]),
                         (['saved 0', 'saved -248'], 16, ['file LAST 1', 'free 197']))
        self.assertEqual(self.commands('list')[-2:], ['file LAST 1', 'free 197'])

    def test_a_directory_with_no_slot_takes_no_new_file(self):
        # a card has a slot for each of its 200 user blocks, as a console reads it: the 8 more that the directory's
        # blocks have room for give no file, and take none
        image = card_image()
        for slot in range(1, 200):
            file_at(image, slot, 'F%03d' % slot, [])
        file_at(image, 200, 'PAST', [])
        self.path.write_bytes(image)
        self.assertEqual(self.commands('save', 'NEW_FILE', '1', '7', 'load', 'NEW_FILE', '0', '1',
                                       'load', 'F199', '0', '1', 'load', 'PAST', '0', '1'),
                         ['saved -248', 'loaded -251 238 238 238 238', 'loaded 0 238 238 238 238',
                          'loaded -251 238 238 238 238'])
        self.assertEqual(self.path.read_bytes(), bytes(image))
        self.assertEqual(self.commands('list')[198:], ['file F198 0', 'file F199 0', 'free 198'])
        # a file of a name it holds has its slot
        self.assertEqual(self.commands('save', 'F100', '1', '8'), ['saved 0'])
        self.assertEqual(self.commands('load', 'F100', '0', '1', 'load', 'GAME_FILE', '0', '2'),
                         ['loaded 0 8 238 238 238', 'loaded 0 1 2 238 238'])
        # where the card has no block either, a file that needs one is told of the blocks, as a console tells it
        self.assertEqual(self.commands('save', 'F001', '197', '9', 'save', 'NEW_FILE', '1', '7',
                                       'save', 'NEW_FILE', '0', '7'), ['saved 0', 'saved -252', 'saved -248'])
        self.assertEqual(self.commands('list')[-1], 'free 0')

    def failed_writes(self, tool, fail, mend):
        """a run whose writes of the card fail between the commands fail and mend"""
        before = self.new_card(('A', '2', '10'))
        # neither a new file nor one in A's place changes the card or its file
        self.assertEqual(self.commands(*fail, 'save', 'B', '3', '20', 'save', 'A', '1', '30',
                                       'list', 'load', 'A', '0', '2', tool=tool),
                         ['saved -192', 'saved -192', 'file A 2', 'free 198', 'loaded 0 10 11 238 238'])
        self.assertEqual((self.path.read_bytes(), self.fresh.exists()), (before, False))
        # the same save goes down once the file takes the write
        self.assertEqual(self.commands(*fail, 'save', 'B', '3', '20', *mend, 'save', 'B', '3', '20', 'list', tool=tool),
                         ['saved -192', 'saved 0', 'file A 2', 'file B 3', 'free 195'])
        self.assertEqual(self.commands('list', 'load', 'B', '0', '3', tool=tool),
                         ['file A 2', 'file B 3', 'free 195', 'loaded 0 20 21 22 238'])
        # and another file's save writes nothing of the saves that failed
        self.path.write_bytes(before)
        self.assertEqual(self.commands(*fail, 'save', 'B', '3', '20', 'save', 'A', '1', '30', *mend,
                                       'save', 'C', '1', '40', tool=tool), ['saved -192', 'saved -192', 'saved 0'])
        after = self.path.read_bytes()
        self.assertEqual((file_of(after, 'A'), file_of(after, 'B')), (file_of(before, 'A'), None))
        self.assertEqual(self.commands('list', 'load', 'A', '0', '2', tool=tool),
                         ['file A 2', 'file C 1', 'free 197', 'loaded 0 10 11 238 238'])

    def test_a_save_the_file_can_t_take_leaves_the_card_and_its_file(self):
        # the card's file stops at 64 KB, half a card
        for tool in (self.tool, self.windows):
            with self.subTest(tool=tool.name):
                self.failed_writes(tool, ('limit', '65536'), ('limit', 'none'))

    def test_a_new_card_that_can_t_take_the_card_s_name_is_removed(self):
        self.new_card(('A', '2', '10'))
        self.addCleanup(shutil.rmtree, self.path, ignore_errors=True)
        self.assertEqual(self.commands('shut', 'save', 'B', '3', '20', 'list', 'load', 'A', '0', '2'),
                         ['saved -192', 'file A 2', 'free 198', 'loaded 0 10 11 238 238'])
        self.assertEqual((self.path.is_dir(), self.fresh.exists()), (True, False))

    def test_a_save_the_engine_s_storage_refuses_leaves_the_card_and_its_file(self):
        self.failed_writes(self.stored, ('fail', '2'), ('fail', '0'))
        damaged = card_image()
        link(damaged, 198, FREE)
        self.refused(damaged, f"a damaged card at {self.path}: a file's chain doesn't end with the file: GAME_FILE",
                     self.stored)

    def test_a_write_cut_short_leaves_the_old_card_at_its_path(self):
        for tool in (self.tool, self.windows):
            with self.subTest(tool=tool.name):
                before = self.new_card(('A', '2', '10'))
                cut = subprocess.run([tool, self.path, 'cut', '65536', 'save', 'B', '3', '20'], capture_output=True)
                self.assertEqual(cut.returncode, -signal.SIGXFSZ)
                # the card's file is whole, with what the run wrote of the new card beside it
                self.assertEqual(self.path.read_bytes(), before)
                self.assertLess(len(self.fresh.read_bytes()), len(before))
                # the next run mounts the card, and its save writes over what lay beside it
                self.assertEqual(self.commands('list', 'save', 'B', '3', '20', tool=tool),
                                 ['file A 2', 'free 198', 'saved 0'])
                self.assertFalse(self.fresh.exists())
                self.assertEqual(self.commands('list', tool=tool), ['file A 2', 'file B 3', 'free 195'])

    def test_a_new_card_left_whole_beside_no_card_is_the_card_on_windows(self):
        # where a write stops there, once the old card has gone and before the new one has its name
        image = card_image()
        for tool in (self.windows, self.windows_stored):
            with self.subTest(tool=tool.name):
                self.path.unlink(missing_ok=True)
                self.fresh.write_bytes(image)
                ran = subprocess.run([tool, self.path, 'list'], check=True, capture_output=True, text=True)
                self.assertEqual((ran.stdout.splitlines()[-2:], ran.stderr), (['file GAME_FILE 2', 'free 198'], ''))
                self.assertEqual((self.path.read_bytes(), self.fresh.exists()), (bytes(image), False))
        # a host whose rename takes a card's place leaves no card that way, and none is there
        for tool in (self.tool, self.stored):
            with self.subTest(tool=tool.name):
                self.path.unlink(missing_ok=True)
                self.fresh.write_bytes(image)
                ran = subprocess.run([tool, self.path, 'list'], check=True, capture_output=True, text=True)
                self.assertEqual((ran.stdout.splitlines()[-1], ran.stderr),
                                 ('free 200', f'sa1: a new card at {self.path}\n'))

    def test_a_part_of_a_card_beside_no_card_is_none(self):
        damaged = card_image()
        link(damaged, 198, FREE)
        for left in (card_image()[:65536], damaged):
            with self.subTest(bytes=len(left)):
                self.path.unlink(missing_ok=True)
                self.fresh.write_bytes(left)
                ran = subprocess.run([self.windows, self.path, 'save', 'NEW_FILE', '2', '7', 'list'], check=True,
                                     capture_output=True, text=True)
                self.assertEqual(ran.stderr, f'sa1: a new card at {self.path}\n')
                self.assertEqual(ran.stdout.splitlines()[-3:], ['saved 0', 'file NEW_FILE 2', 'free 198'])
                self.assertFalse(self.fresh.exists())

    def test_a_card_a_save_wrote_mounts_whatever_card_it_began_as(self):
        # words of the root's layout, of the FAT and of the directory's first entries, changed at a seed's choice:
        # the card is refused, or it mounts, takes what saves it has room for, and mounts as they left it
        chosen = random.Random(1)
        saves = ('save', 'OTHER', '5', '9', 'save', 'NEW_FILE', '190', '3', 'save', 'GAME_FILE', '1', '4', 'list')
        mounted = 0
        for n in range(48):
            image = card_image()
            file_at(image, 1, 'OTHER', [150, 10, 11])
            for _ in range(chosen.randrange(1, 3)):
                at = chosen.choice((ROOT_BLOCK * BLOCK + chosen.choice((0x46, 0x48, 0x4a, 0x4c, 0x50)),
                                    FAT_BLOCK * BLOCK + chosen.choice((199, 198, 150, 10, 11, 253, 241)) * 2,
                                    FAT_BLOCK * BLOCK + chosen.randrange(256) * 2,
                                    DIRECTORY_BLOCK * BLOCK + chosen.randrange(0, 96, 2)))
                struct.pack_into('<H', image, at, chosen.choice((0, 1, 2, 13, 200, 241, 253, 254, 255, END, FREE, BAD,
                                                                 chosen.randrange(256), chosen.randrange(65536))))
            self.path.write_bytes(image)
            with self.subTest(n=n):
                first = subprocess.run([self.tool, self.path, *saves], capture_output=True).returncode
                self.assertIn(first, (0, 2))
                if first == 0:
                    mounted += 1
                    self.assertEqual(subprocess.run([self.tool, self.path, 'list'], capture_output=True).returncode, 0)
        # the seed's cards are of both kinds
        self.assertTrue(8 < mounted < 40, mounted)


PLACE_DRIVER = r"""
#include <stdio.h>
#include <stdlib.h>
#include "native.h"

/* place DATA NAME SIZE: settings.c's place for NAME, the player's data's
 * where DATA is 1, in SIZE bytes */
int main(int argc, char **argv)
{
    char out[256];
    const char *place = argc == 4 ? native_settings_place(out, strtoul(argv[3], NULL, 0), atoi(argv[1]), argv[2])
                                  : NULL;

    puts(place ? place : "none");
    return 0;
}
"""

# a Windows user's %APPDATA%
APPDATA = r'C:\Users\player\AppData\Roaming'


class Settings(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        tools = ROOT / 'engine/native'
        cls.built = tempfile.TemporaryDirectory()
        built = Path(cls.built.name)
        (built / 'driver.c').write_text(PLACE_DRIVER)
        compiler = os.environ.get('CC') or shutil.which('clang') or 'cc'
        cls.tool = built / 'place'
        cls.windows = built / 'windows'
        # the host's own places, and Windows's as its build finds them, which reads the environment alone
        for tool, defines in ((cls.tool, []), (cls.windows, ['-D_WIN32'])):
            subprocess.run([compiler, '-std=gnu17', '-O1', f'-I{tools}', *defines, '-o', tool, built / 'driver.c',
                            tools / 'settings.c'], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.built.cleanup()

    def place(self, data, size=256, tool=None, **environment):
        """settings.c's place for name, the data's where data is 1, in size bytes, under environment alone"""
        return subprocess.run([tool or self.tool, str(data), 'name', str(size)], env=environment,
                              capture_output=True, text=True, check=True).stdout

    def test_a_players_file_lies_under_xdgs_directory_or_else_home_s_and_nowhere_without_either(self):
        both = {'XDG_CONFIG_HOME': '/c', 'XDG_DATA_HOME': '/d', 'HOME': '/h'}
        self.assertEqual((self.place(0, **both), self.place(1, **both)),
                         ('/c/sonic-adventure/name\n', '/d/sonic-adventure/name\n'))
        # a base that isn't absolute is none, and neither is an empty home
        for environment, places in (
                ({'XDG_CONFIG_HOME': 'c', 'XDG_DATA_HOME': '', 'HOME': '/h'},
                 ('/h/.config/sonic-adventure/name\n', '/h/.local/share/sonic-adventure/name\n')),
                ({'XDG_CONFIG_HOME': '/c', 'HOME': ''}, ('/c/sonic-adventure/name\n', 'none\n')),
                ({}, ('none\n', 'none\n'))):
            with self.subTest(environment=environment):
                self.assertEqual((self.place(0, **environment), self.place(1, **environment)), places)

    def test_windows_keeps_both_under_appdata_and_has_no_place_without_it(self):
        # the host's own directories are none of Windows's
        user = {'APPDATA': APPDATA, 'XDG_CONFIG_HOME': '/c', 'XDG_DATA_HOME': '/d', 'HOME': '/h'}
        self.assertEqual((self.place(0, tool=self.windows, **user), self.place(1, tool=self.windows, **user)),
                         (f'{APPDATA}\\sonic-adventure\\name\n',) * 2)
        for environment in ({'APPDATA': '', 'HOME': '/h'}, {'XDG_CONFIG_HOME': '/c', 'XDG_DATA_HOME': '/d'}, {}):
            with self.subTest(environment=environment):
                self.assertEqual((self.place(0, tool=self.windows, **environment),
                                  self.place(1, tool=self.windows, **environment)), ('none\n', 'none\n'))

    def test_a_path_its_buffer_cant_hold_whole_is_no_place(self):
        # a path cut short would name another file, which the runner would read or card.c make
        for tool, environment, whole in (
                (self.tool, {'HOME': '/h'}, '/h/.config/sonic-adventure/name'),
                (self.tool, {'XDG_CONFIG_HOME': '/c'}, '/c/sonic-adventure/name'),
                (self.windows, {'APPDATA': APPDATA}, f'{APPDATA}\\sonic-adventure\\name')):
            with self.subTest(environment=environment):
                self.assertEqual(self.place(0, len(whole) + 1, tool=tool, **environment), f'{whole}\n')
                self.assertEqual(self.place(0, len(whole), tool=tool, **environment), 'none\n')


THREADS_DRIVER = r"""
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include "native.h"

struct thread;
void thread_init(void);
struct thread *thread_create(void *, void *, int);
struct thread *thread_create_child(struct thread *, void *, void *, int);
void thread_start(struct thread *, void *, void *);
void thread_destroy(struct thread *);
void thread_set_destructor(struct thread *, void (*)(void));
void thread_yield(void);

/* the game's heap, first fit, and the slice's word as a block goes back */
static long blocks[80][0x1000 / sizeof(long)];
static char used[80];
void *heap_alloc(int size)
{
    for (int i = 0; i < 80 && size <= 0x1000; i++) {
        if (!used[i]) {
            return used[i] = 1, blocks[i];
        }
    }
    return NULL;
}
void heap_free(void *memory)
{
    used[(long (*)[0x1000 / sizeof(long)])memory - blocks] = 0;
    native_thread_freed(memory);
}
/* the runner's: zeros at at, where the host has placed nothing */
void native_map(void *at, size_t size, const char *what)
{
    if (mmap(at, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0) != at) {
        fprintf(stderr, "sa1: can't map %s\n", what);
        exit(2);
    }
}

static const char *mode;
static int live, turns, stacks, fresh;
/* a local of the last thread to start, and the low end of what it wrote under itself */
static char *top;
static volatile char *dirt;
/* whether the last stack to take a context came as zeros where the thread before it wrote */
static int clean = 1;
static struct thread *destroyer;

static int is(const char *name) { return !strcmp(mode, name); }

static __attribute__((noinline)) void dirty(void)
{
    volatile char bytes[0x8000];

    for (size_t i = 0; i < sizeof bytes; i++) {
        bytes[i] = 0x5a;
    }
    dirt = bytes;
}
static int zeros(void)
{
    int all = 1;

    for (int i = 0; dirt && i < 0x1000; i++) {
        all &= !dirt[i];
    }
    return all;
}
/* arg's letter, for as many turns as its digit */
static void counts(void *arg)
{
    const char *name = arg;

    for (int turn = 1; turn <= name[1] - '0'; turn++) {
        printf("%c %d\n", name[0], turn);
        if (turn < name[1] - '0') {
            thread_yield();
        }
    }
    live--;
}
/* a start takes 1 KB of its stack, and 1000 that kept theirs would run off it */
static void again(void *arg)
{
    volatile char frame[1024];
    long left = (long)arg;

    frame[0] = (char)left;
    if (left) {
        thread_start(NULL, (void *)again, (void *)(left - 1));
    }
    puts("and 1000 times more");
    live--;
}
static void second(void *arg)
{
    char here;
    int found = clean && zeros();

    printf("second %s, from the top %d, on fresh zeros %d\n", (char *)arg, labs(&here - top) < 0x400, found);
    thread_yield();
    puts("second again");
    if (is("restart")) {
        thread_start(NULL, (void *)again, (void *)1000L);
    }
    live--;
}
/* 1 KB of stack a call, so a start from under 32 of them is far from the top */
static void nest(int depth)
{
    volatile char frame[1024];

    frame[0] = (char)depth;
    if (depth) {
        nest(depth - 1);
    } else {
        puts("starting over");
        thread_start(NULL, (void *)second, "two");
        puts("returned");
    }
    frame[1] = frame[0];
}
static void first(void *arg)
{
    char here;

    top = &here;
    dirty();
    printf("first %s\n", (char *)arg);
    thread_yield();
    if (is("restart")) {
        nest(32);
    }
    for (;;) {
        thread_yield();
    }
}
/* a thread that returns, with an arg, or waits for its end */
static void holds(void *arg)
{
    char here;

    fresh += clean && zeros();
    stacks += top != &here;
    top = &here;
    dirty();
    while (!arg) {
        thread_yield();
    }
}
static void turn(void *arg)
{
    (void)arg;
    turns++;
    for (;;) {
        thread_yield();
    }
}
/* 1 KB of stack a call, each byte written, down to until */
static void sink(char *until)
{
    volatile char frame[1024];

    for (size_t i = 0; i < sizeof frame; i++) {
        frame[i] = 0x7e;
    }
    if ((char *)frame > until) {
        sink(until);
    }
    frame[0] = 0;
}
static void sinks(void *arg)
{
    (void)arg;
    sink(top);
    puts("sank into the stack below");
    live--;
}
/* destroys arg: its parent, which takes it along, or a thread whose destructor starts it over */
static void ends(void *arg)
{
    char here;

    top = &here;
    puts("ending");
    thread_destroy(arg);
    puts("returned");
}
static void made(void *arg)
{
    char here;

    printf("made %s, on another stack %d\n", (char *)arg, labs(&here - top) >= 0x40000);
    live--;
}
/* destructors, which run on the stack of the thread that destroys theirs */
static void makes(void)
{
    thread_create((void *)made, "by a destructor", 0);
}
static void starts(void)
{
    thread_start(destroyer, (void *)made, "over its destroyer");
}

void sa1_main(void)
{
    struct thread *thread;

    thread_init();
    if (is("turns")) {
        thread_create((void *)counts, "a2", 0);
        thread_create((void *)counts, "b3", 0);
        live = 2;
    } else if (is("restart")) {
        thread_create((void *)first, "one", 0);
        live = 1;
    } else if (is("other")) {
        thread = thread_create((void *)first, "one", 0);
        thread_yield();
        thread_start(thread, (void *)second, "other");
        live = 1;
    } else if (is("reuse")) {
        for (long i = 0; i < 200; i++) {
            thread = thread_create((void *)holds, (void *)(i & 1), 0);
            thread_yield();
            if (!(i & 1)) {
                thread_destroy(thread);
            }
        }
        printf("200 threads on %d stack, %d on fresh zeros\n", stacks, fresh);
    } else if (is("most")) {
        thread_yield();
        for (int i = 0; i < 63; i++) {
            thread_create((void *)turn, NULL, 0);
        }
        thread_yield();
        printf("%d threads beside the root took a turn\n", turns);
        thread_create((void *)turn, NULL, 0);
        puts("and one more");
    } else if (is("overrun")) {
        /* the second's stack lies over the first's */
        thread_create((void *)first, "below", 0);
        thread_create((void *)sinks, NULL, 0);
        live = 1;
    } else if (is("ended")) {
        /* the child destroys its parent, and so itself first */
        thread = thread_create((void *)turn, NULL, 0);
        thread_set_destructor(thread, makes);
        thread_create_child(thread, (void *)ends, thread, 0);
        live = 1;
    } else if (is("under")) {
        thread = thread_create((void *)turn, NULL, 0);
        thread_set_destructor(thread, starts);
        destroyer = thread_create((void *)ends, thread, 0);
        live = 1;
    } else if (is("where") || is("taken") || is("later") || is("unplaced")) {
        thread_create((void *)first, "one", 0);
        thread_yield();
        printf("%p\n", (void *)top);
    }
    while (live) {
        puts("root");
        thread_yield();
    }
    puts("over");
}

/* the build names the driver's for threads.c's context calls, and these are the host's */
#undef makecontext
#undef getcontext
#undef swapcontext
#undef setcontext
#include <ucontext.h>
/* Apple's makecontext zeroes the stack it is given, so a thread can't tell there what its stack held as its
 * context was made: this looks first */
void looked_makecontext(ucontext_t *context, void (*entry)(void), int argc, ...)
{
    char *stack = context->uc_stack.ss_sp;

    if ((char *)dirt >= stack && (char *)dirt < stack + context->uc_stack.ss_size) {
        clean = zeros();
    }
    makecontext(context, entry, argc);
}

#ifdef REFUSES
/* each fails where the mode is its own */
int refused_getcontext(ucontext_t *context) { return is("make") ? -1 : getcontext(context); }
int refused_swapcontext(ucontext_t *from, const ucontext_t *to) { return is("swap") ? -1 : swapcontext(from, to); }
int refused_setcontext(const ucontext_t *to) { return is("jump") ? -1 : setcontext(to); }
#endif

int main(int argc, char **argv)
{
    static char stack[0x100000];
    /* argv[2] is where a thread's stack lies, and the host asks for 64 KB there */
    char *held = argc > 2 ? (char *)strtoull(argv[2], NULL, 16) : NULL;

    mode = argv[1];
    setvbuf(stdout, NULL, _IONBF, 0);
    if (is("taken")) {
        if (mmap(held, 0x10000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0) != held) {
            return 1;
        }
        held[0] = 0x5a;
    }
    if (!is("unplaced")) {
        native_threads_reserve();
    }
    if (is("later")) {
        puts(mmap(held, 0x10000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0) == held
                 ? "the host took a stack's place" : "the host's map fell elsewhere");
    }
    native_run_main(stack, sizeof stack);
    if (is("taken")) {
        puts(held[0] == 0x5a ? "kept" : "replaced");
    }
    return 0;
}
"""


class Threads(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        tools = ROOT / 'engine/native'
        cls.built = tempfile.TemporaryDirectory()
        built = Path(cls.built.name)
        (built / 'driver.c').write_text(THREADS_DRIVER)
        compiler = os.environ.get('CC') or shutil.which('clang') or 'cc'
        # macOS declares ucontext under _XOPEN_SOURCE alone, calls it
        # deprecated, and maps nothing in a program's first 4 GB, so there the
        # domain lies past them. Linux takes the domain at 0, as the slice does
        host = (['-D_XOPEN_SOURCE=600', '-D_DARWIN_C_SOURCE', '-Wno-deprecated-declarations',
                 '-DNATIVE_DOMAIN=0x10000000000ULL'] if sys.platform == 'darwin' else [])
        # The ring and the turns are the game's own thread.c, which the
        # prelude gives the slice's types. Its resume, run and prepare are
        # assembly on the Dreamcast, and here they link to threads.c's
        subprocess.run([compiler, '-std=gnu89', '-DNONMATCHING', '-w', '-include', tools / 'prelude.h',
                        f'-I{ROOT / "sdk/include"}', f'-I{ROOT / "src"}', '-c', '-o', built / 'thread.o',
                        ROOT / 'src/system/thread.c'], check=True)
        cls.tool, cls.refuses = built / 'threads', built / 'refuses'
        # threads.c's makecontext is the driver's, which looks at the stack
        # before the host's own takes it
        build = [compiler, '-std=gnu17', '-O1', *host, '-Dmakecontext=looked_makecontext', f'-I{tools}',
                 f'-I{ROOT / "engine/include"}']
        sources = [built / 'driver.c', tools / 'threads.c', built / 'thread.o']
        subprocess.run([*build, '-o', cls.tool, *sources], check=True)
        # A second build gives threads.c the driver's getcontext, swapcontext
        # and setcontext, which fail when told to
        renamed = [f'-D{name}=refused_{name}' for name in ('getcontext', 'swapcontext', 'setcontext')]
        subprocess.run([*build, '-DREFUSES', *renamed, '-o', cls.refuses, *sources], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.built.cleanup()

    def run_threads(self, *args, tool=None):
        ran = subprocess.run([tool or self.tool, *args], capture_output=True, text=True)
        return ran.returncode, ran.stdout.splitlines(), ran.stderr

    def test_threads_take_turns_and_one_that_returns_passes_its_turn_on(self):
        self.assertEqual(self.run_threads('turns'),
                         (0, ['root', 'a 1', 'b 1', 'root', 'a 2', 'b 2', 'root', 'b 3', 'over'], ''))

    def test_a_running_thread_starts_over_from_the_top_of_a_stack_of_fresh_zeros(self):
        self.assertEqual(self.run_threads('restart'),
                         (0, ['root', 'first one', 'root', 'starting over',
                              'second two, from the top 1, on fresh zeros 1', 'root', 'second again',
                              'and 1000 times more', 'over'], ''))

    def test_a_waiting_thread_starts_over_on_its_next_turn(self):
        self.assertEqual(self.run_threads('other'),
                         (0, ['first one', 'root', 'second other, from the top 1, on fresh zeros 1', 'root',
                              'second again', 'over'], ''))

    def test_a_thread_takes_the_context_and_the_stack_of_one_that_ended(self):
        self.assertEqual(self.run_threads('reuse'), (0, ['200 threads on 1 stack, 200 on fresh zeros', 'over'], ''))

    def test_one_thread_more_than_the_contexts_ends_the_run(self):
        self.assertEqual(self.run_threads('most'),
                         (2, ['63 threads beside the root took a turn'], 'sa1: more than 64 threads\n'))

    def test_a_thread_that_runs_off_its_stack_faults_short_of_the_stack_below(self):
        status, said, _ = self.run_threads('overrun')
        self.assertEqual(said, ['root', 'first below'])
        self.assertIn(status, (-signal.SIGSEGV, -signal.SIGBUS))

    def test_a_thread_made_while_one_that_ended_still_runs_takes_another_stack(self):
        # A child destroys its parent, whose destructor makes a thread on the
        # child's stack, after the child's own memory went back to the heap
        self.assertEqual(self.run_threads('ended'),
                         (0, ['root', 'ending', 'root', 'made by a destructor, on another stack 1', 'over'], ''))

    def test_a_destructor_that_starts_its_destroyer_over_ends_the_run(self):
        self.assertEqual(self.run_threads('under'),
                         (2, ['root', 'ending'], "sa1: can't map a stack under the thread that runs on it\n"))

    def test_the_stacks_take_no_place_the_host_holds(self):
        # A first run tells where a thread's stack lies, and a second holds
        # 64 KB there before the threads take their place
        held = int(self.run_threads('where')[1][1], 16) & ~0xffff
        self.assertEqual(self.run_threads('taken', f'{held:x}'), (2, [], "sa1: can't map the threads' stacks\n"))

    def test_the_stacks_hold_their_place_before_a_thread_has_one(self):
        # The host asks for a stack's place once the threads have theirs, and
        # before the game's main runs
        held = int(self.run_threads('where')[1][1], 16) & ~0xffff
        status, said, _ = self.run_threads('later', f'{held:x}')
        self.assertEqual((status, said[0], said[-1]), (0, "the host's map fell elsewhere", 'over'))

    def test_a_thread_made_before_the_stacks_have_their_place_ends_the_run(self):
        self.assertEqual(self.run_threads('unplaced'), (2, [], "sa1: can't map a thread's stack\n"))

    def test_a_context_the_host_refuses_ends_the_run(self):
        self.assertEqual(self.run_threads('turns', tool=self.refuses)[1][-1], 'over')
        for mode, said, refused in (('make', [], "make a thread's context"), ('swap', [], 'switch threads'),
                                    ('jump', ['over'], 'switch threads')):
            with self.subTest(mode=mode):
                self.assertEqual(self.run_threads(mode, tool=self.refuses), (2, said, f"sa1: can't {refused}\n"))


# what a driver of shim.c opens with
SHIM_READS = r"""
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "native.h"
#include "play_math.h"
/* what the shim reads of the game, the runner and play math */
int game_mode;
int16_t stage_current, act_current;
unsigned long native_vsyncs;
char native_last_opened[64];
void *_nj_texmemlist;
struct { void *entries[9]; } advertise_entries, summary_entries;
int get_character(void) { return 0; }
void native_lists_over(void) {}
void sa_fsca(uint32_t angle, float out[2]) { (void)angle, out[0] = out[1] = 0; }
float sa_fsrra(float a) { return a; }
float _in_atan2s(float y, float x) { return y + x; }
void native_map(void *at, size_t size, const char *what) { (void)at, (void)size, (void)what; }
int native_hook_clock(uint32_t *count) { (void)count; return 0; }
void native_hook_date(uint32_t count) { (void)count; }
void native_pad_record(void *pad) { (void)pad; }
void native_hook_textures_released(void *texlist) { (void)texlist; }
char *native_symbol(const char *name) { (void)name; return NULL; }
void native_program_read(const char *file) { (void)file; }
void set_character_number(int16_t number) { (void)number; }
void stage_set_stage_act(uint8_t stage, uint8_t act) { (void)stage, (void)act; }
"""

TEXTURES_DRIVER = SHIM_READS + r"""
/* Ninja's names, lists and records, as the shim lays them out */
#define MEMORY 0x40000000
#define INDEXED 0x00800000
struct texname { void *filename; uint32_t attr, texaddr; };
struct texlist { struct texname *textures; uint32_t count; };
struct texinfo { void *pixels; uint32_t type, depth, format, width, height; };
struct record { uint32_t index, words[4]; void *texaddr; uint32_t surface[7], *pointers[3]; uint16_t count, dummy; };
int32_t njLoadTexture(struct texlist *);
int32_t njLoadTexturePvmMemory(uint8_t *, struct texlist *);
int32_t prs_decompress(void *, void *);
int32_t njReleaseTexture(void *);
void njReleaseTextureAll(void);
int32_t njSearchTexList(void *, uint32_t, uint32_t);

/* a texture's size, which its index alone gives */
#define WIDTH(index) ((index) % 251 + 1)
#define HEIGHT(index) ((index) % 127 + 1)
/* record 0, which is no texture's */
#define NONE ((struct record *)_nj_texmemlist)
/* the record at the address a name keeps, its low 32 bits */
static struct record *record(uint32_t texaddr)
{
    return (struct record *)((char *)NONE + (uint32_t)(texaddr - (uint32_t)(uintptr_t)NONE));
}
/* the record a search of index answers */
static struct record *search(uint32_t index)
{
    return NONE + njSearchTexList(NONE, 0, index);
}
/* a texture made in memory */
static void memory_name(struct texname *name, struct texinfo *info, uint32_t index)
{
    *info = (struct texinfo){.width = WIDTH(index), .height = HEIGHT(index)};
    *name = (struct texname){info, MEMORY | INDEXED, index};
}
/* whether a name addresses the record of the texture of index */
static int holds(const struct texname *name, uint32_t index)
{
    const struct record *at = record(name->texaddr);

    return at->index == index && at->surface[3] == WIDTH(index) && at->surface[4] == HEIGHT(index);
}
/* a session's stages, and the pictures of each of a stage's four movies, which come to twice the records and more */
#define STAGES 20
#define PICTURES (RECORDS / 2 + 1)
/* the textures a stage keeps, of indexes from index: made in memory, or files a list names once, however often
 * it loads */
#define KEPT 8
static struct texname kept[KEPT];
static struct texinfo kept_info[KEPT];
static struct texlist keep = {kept, KEPT};
static void name_kept(uint32_t index)
{
    for (uint32_t i = 0; i < KEPT; i++) {
        memory_name(&kept[i], &kept_info[i], index + i);
    }
}
static void file_kept(uint32_t index)
{
    static char files[KEPT][16];

    for (uint32_t i = 0; i < KEPT; i++) {
        snprintf(files[i], sizeof files[i], "T%u", index + i);
        kept[i] = (struct texname){files[i], 0, 0};
    }
}
static uint32_t kept_held(uint32_t index)
{
    uint32_t held = 0;

    for (uint32_t i = 0; i < KEPT; i++) {
        held += holds(&kept[i], index + i) && search(index + i) == record(kept[i].texaddr);
    }
    return held;
}
/* the adapter: the textures it heard loaded, and of those it heard released, the sum of the indexes their records
 * held as it heard, by which it names them */
static unsigned long loaded, released;
/* and the first name the last load gave it, or none */
static const char *first_name = "";
void sa1_textures_loaded(const void *texlist, uint32_t count, const char *const *names)
{
    (void)texlist;
    loaded += count;
    first_name = names != NULL && names[0] != NULL ? names[0] : "";
}
void sa1_textures_released(const void *texlist)
{
    const struct texlist *list = texlist;

    for (uint32_t i = 0; i < list->count; i++) {
        released += record(list->textures[i].texaddr)->index;
    }
}
/* a file's name id, here a sum over the name the shim gives: no two of the test's names share one, and none is 0 */
uint32_t native_disc_file_id(const char *name)
{
    uint32_t id = 0;

    while (*name) {
        id = id * 31 + (uint8_t)*name++;
    }
    return id;
}
/* the disc: T and an index names a file of that global index, and any other name none */
FILE *native_disc_file(const char *name)
{
    static uint8_t head[32] = "GBIX";
    uint32_t index, bytes = 8;
    uint16_t width, height;

    if (sscanf(name, "T%u.PVR", &index) != 1) {
        return NULL;
    }
    width = WIDTH(index), height = HEIGHT(index);
    memcpy(head + 4, &bytes, 4), memcpy(head + 8, &index, 4), memcpy(head + 16, "PVRT", 4);
    memcpy(head + 28, &width, 2), memcpy(head + 30, &height, 2);
    return fmemopen(head, sizeof head, "rb");
}
/* a PVM of count textures of indexes from index: an entry for each, of its number and what status gives of it, its
 * name under NAMED and its index under INDEXES, as Ninja's loader reads them, and then each one's PVRT */
#define NAMED 0x1
#define INDEXES 0x8
static uint8_t *pvm(uint32_t index, uint16_t count, uint8_t status)
{
    static uint8_t bytes[1024];
    uint8_t *at = bytes + 12;
    uint32_t size;

    memset(bytes, 0, sizeof bytes);
    memcpy(bytes, "PVMH", 4);
    bytes[8] = status;
    memcpy(bytes + 10, &count, 2);
    for (uint16_t i = 0; i < count; i++) {
        uint32_t global = index + i;

        memcpy(at, &i, 2), at += 2;
        if (status & NAMED) {
            snprintf((char *)at, 28, "T%u", global), at += 28;
        }
        if (status & INDEXES) {
            memcpy(at, &global, 4), at += 4;
        }
    }
    size = (uint32_t)(at - bytes) - 8;
    memcpy(bytes + 4, &size, 4);
    for (uint16_t i = 0; i < count; i++, at += 16) {
        uint16_t width = WIDTH(index + i), height = HEIGHT(index + i);

        size = 8;
        memcpy(at, "PVRT", 4), memcpy(at + 4, &size, 4), memcpy(at + 12, &width, 2), memcpy(at + 14, &height, 2);
    }
    return bytes;
}
/* a PVM's 1024 bytes as a PRS stream of literals: a flag byte of eight set bits before each eight bytes, then a
 * clear bit, a set one and the word of zeros that ends the stream */
static void pack(uint8_t *to, const uint8_t *from)
{
    for (int i = 0; i < 1024; i += 8, to += 9) {
        to[0] = 0xff;
        memcpy(to + 1, from + i, 8);
    }
    to[0] = 0x02, to[1] = to[2] = 0;
}
/* whether the record a name addresses is of the file the game opened as name */
static int from(const struct texname *texture, const char *name)
{
    return native_texture_file(texture->texaddr) == native_disc_file_id(name);
}

int main(int argc, char **argv)
{
    struct texname other;
    struct texinfo other_info;
    struct texlist one = {&other, 1};
    const char *run = argc > 1 ? argv[1] : "";

    if (!strcmp(run, "kept")) {
        /* a search ahead of any load, and then a stage's textures, while four times the records come and go */
        uint32_t wrong = 0;
        int32_t first = njSearchTexList(NULL, 0, 4321);

        printf("ahead of any load a miss answers record %d, the list set %d\n", first, NONE != NULL);
        name_kept(1234);
        njLoadTexture(&keep);
        for (uint32_t i = 0; i < 4 * (RECORDS + 1); i++) {
            memory_name(&other, &other_info, 100000 + i);
            wrong += njLoadTexture(&one) != 1 || !holds(&other, 100000 + i) || kept_held(1234) != KEPT;
            njReleaseTexture(&one);
        }
        printf("wrong %u, kept %u\n", wrong, kept_held(1234));
        printf("a miss reads %u, %ux%u, at record %d\n", search(4321)->index, search(4321)->surface[3],
               search(4321)->surface[4], (int)(search(4321) - NONE));
    } else if (!strcmp(run, "users")) {
        /* one list loaded twice, as two tasks load the list they share, and released by each */
        uint32_t first;

        file_kept(77);
        njLoadTexture(&keep);
        first = kept[0].texaddr;
        njLoadTexture(&keep);
        printf("shared %d\n", kept[0].texaddr == first);
        njReleaseTexture(&keep);
        printf("after one release: kept %u, the adapter heard %lu\n", kept_held(77), released);
        njReleaseTexture(&keep);
        printf("after the last: kept %u, found %d, the adapter heard %lu\n", kept_held(77), search(77) != NONE,
               released);
        /* and by one too many, once another texture has come */
        memory_name(&other, &other_info, 78);
        njLoadTexture(&one);
        njReleaseTexture(&keep);
        printf("after one too many: the next held %d, apart %d, the adapter heard %lu\n", holds(&other, 78),
               other.texaddr != first, released);
    } else if (!strcmp(run, "all")) {
        /* lists no release names, one of them loaded twice, at a stage's start */
        file_kept(500);
        njLoadTexture(&keep);
        njLoadTexture(&keep);
        memory_name(&other, &other_info, 77);
        njLoadTexture(&one);
        njReleaseTextureAll();
        printf("kept %u, found %d\n", kept_held(500) + holds(&other, 77), search(500) != NONE || search(77) != NONE);
        printf("the adapter heard %lu\n", released);
        /* a list released after it lets go of nothing a later load took */
        memory_name(&other, &other_info, 78);
        njLoadTexture(&one);
        njReleaseTexture(&keep);
        printf("the next held %d\n", holds(&other, 78));
    } else if (!strcmp(run, "full")) {
        /* one texture more than the records, each held */
        static struct texname names[RECORDS + 1];
        static struct texinfo infos[RECORDS + 1];
        struct texlist list = {names, 1};
        uint32_t took = 0, wrong = 0;
        int32_t result;

        for (; took <= RECORDS; took++, list.textures++) {
            memory_name(&names[took], &infos[took], 1 + took);
            if (njLoadTexture(&list) != 1) {
                break;
            }
        }
        if (took > RECORDS) {
            return 1;
        }
        printf("took %u, the next at record %d\n", took, (int)(record(names[took].texaddr) - NONE));
        njReleaseTexture(&list);
        for (uint32_t i = 0; i < took; i++) {
            wrong += !holds(&names[i], 1 + i);
        }
        printf("wrong %u\n", wrong);
        /* an index a record holds needs no free one */
        memory_name(&other, &other_info, 77);
        result = njLoadTexture(&one);
        printf("a held index loaded %d, shared %d\n", result, other.texaddr == names[76].texaddr);
        njReleaseTexture(&one);
        /* a release frees a record for a later load, which takes the one freed longest ago */
        list.textures = &names[5];
        njReleaseTexture(&list);
        list.textures = &names[0];
        njReleaseTexture(&list);
        memory_name(&other, &other_info, 900000);
        result = njLoadTexture(&one);
        printf("then loaded %d, held %d, in the record freed first %d\n", result, holds(&other, 900000),
               other.texaddr == names[5].texaddr);
        /* and a record taken again holds nothing of its last texture */
        other = (struct texname){"NONE", 0, 0};
        njLoadTexture(&one);
        printf("the other freed taken %d, and reads %u, %ux%u\n", other.texaddr == names[0].texaddr,
               record(other.texaddr)->index, record(other.texaddr)->surface[3], record(other.texaddr)->surface[4]);
    } else if (!strcmp(run, "pvm")) {
        /* a PVM read into the loader's names, of which a list keeps as many as it has, as texture_load_pvm keeps
         * them, and read again for another list while the first holds it */
        struct texname scratch[4] = {{0}}, all[4], first[3], second[3];
        struct texlist loader = {scratch, 4}, firsts = {first, 3}, seconds = {second, 3};

        printf("read %d\n", njLoadTexturePvmMemory(pvm(300, 4, NAMED | INDEXES), &loader));
        memcpy(first, scratch, sizeof first);
        njLoadTexturePvmMemory(pvm(300, 4, NAMED | INDEXES), &loader);
        memcpy(second, scratch, sizeof second);
        memcpy(all, scratch, sizeof all);
        printf("shared %d\n", first[0].texaddr == second[0].texaddr && first[2].texaddr == second[2].texaddr);
        njReleaseTexture(&firsts);
        printf("after one list: held %d, the adapter heard %lu\n", holds(&all[0], 300) + holds(&all[2], 302),
               released);
        njReleaseTexture(&seconds);
        printf("after both: held %d, the fourth %d\n", holds(&all[0], 300) + holds(&all[2], 302),
               holds(&all[3], 303));
        njReleaseTextureAll();
        printf("after all: the fourth %d, the adapter heard %lu of %lu\n", holds(&all[3], 303), released, loaded);
    } else if (!strcmp(run, "status")) {
        /* a PVM whose entries hold a global index and no name, and one whose entries hold a name and no index */
        struct texname indexed[2] = {{0}}, named[2] = {{0}};
        struct texlist indexes = {indexed, 2}, names = {named, 2};

        njLoadTexturePvmMemory(pvm(400, 2, INDEXES), &indexes);
        printf("indexes: held %d, named \"%s\"\n", holds(&indexed[0], 400) + holds(&indexed[1], 401), first_name);
        njLoadTexturePvmMemory(pvm(500, 2, NAMED), &names);
        printf("names: indexes %u and %u, sized %d, named \"%s\"\n", record(named[0].texaddr)->index,
               record(named[1].texaddr)->index,
               record(named[0].texaddr)->surface[3] == WIDTH(500) && record(named[1].texaddr)->surface[3] == WIDTH(501),
               first_name);
    } else if (!strcmp(run, "files")) {
        /* two lists that name one file, and two names that have none */
        struct texname a = {"T41", 0, 0}, b = {"T41.PVR", 0, 0}, none[2] = {{"NONE", 0, 0}, {"NONE", 0, 0}};
        struct texname gone = {"GONE", INDEXED, 43}, there = {"T43", 0, 0};
        struct texlist first = {&a, 1}, second = {&b, 1}, nones = {none, 2}, gones = {&gone, 1}, theres = {&there, 1};

        njLoadTexture(&first);
        njLoadTexture(&second);
        printf("shared %d, held %d\n", a.texaddr == b.texaddr, holds(&a, 41));
        njLoadTexture(&nones);
        printf("apart %d, the newest found %d\n", none[0].texaddr != none[1].texaddr && record(none[0].texaddr) != NONE,
               search(0) == record(none[1].texaddr));
        njReleaseTexture(&nones);
        printf("after their release: found %d\n", search(0) != NONE);
        /* an index a name gives with no file, and then the file of that index */
        njLoadTexture(&gones);
        njLoadTexture(&theres);
        printf("shared %d, held with the file's size %d\n", gone.texaddr == there.texaddr, holds(&gone, 43));
    } else if (!strcmp(run, "from")) {
        /* a PVM the game opened as first.prs and decoded, loaded from where the decode wrote it; a PVR's file of
         * an index the PVM's record holds; and the PVM again from bytes no decode wrote */
        static uint8_t packed[1024 / 8 * 9 + 3], decoded[1024];
        struct texname scratch[2] = {{0}}, lone = {"T700", 0, 0}, gone = {"GONE", INDEXED, 43}, there = {"T43", 0, 0};
        struct texlist loader = {scratch, 2}, lones = {&lone, 1}, gones = {&gone, 1}, theres = {&there, 1};

        pack(packed, pvm(700, 2, NAMED | INDEXES));
        snprintf(native_last_opened, sizeof native_last_opened, "first.prs");
        prs_decompress(packed, decoded);
        njLoadTexturePvmMemory(decoded, &loader);
        printf("a decoded PVM: held %d, its file's %d\n", holds(&scratch[0], 700) + holds(&scratch[1], 701),
               from(&scratch[0], "first.prs") + from(&scratch[1], "first.prs"));
        njLoadTexture(&lones);
        printf("a file of its index: shared %d, its first file's %d\n", lone.texaddr == scratch[0].texaddr,
               from(&lone, "first.prs"));
        njReleaseTextureAll();
        printf("after their release: %u\n", native_texture_file(scratch[0].texaddr));
        /* in the other order, the PVR's file is the record's first */
        njLoadTexture(&lones);
        njLoadTexturePvmMemory(decoded, &loader);
        printf("the file first: shared %d, its own %d, the PVM's other texture the PVM's %d\n",
               lone.texaddr == scratch[0].texaddr, from(&scratch[0], "T700.PVR"), from(&scratch[1], "first.prs"));
        njReleaseTextureAll();
        njLoadTexturePvmMemory(pvm(700, 2, NAMED | INDEXES), &loader);
        printf("a PVM no decode wrote: held %d, a file's %u\n", holds(&scratch[0], 700),
               native_texture_file(scratch[0].texaddr));
        /* a texture made in memory, a name that addresses no record, and an index a name gives with no file, and
         * then the file of that index */
        memory_name(&other, &other_info, 800);
        njLoadTexture(&one);
        njLoadTexture(&gones);
        printf("memory's %u, no record's %u, no file's %u\n", native_texture_file(other.texaddr),
               native_texture_file(0), native_texture_file(gone.texaddr));
        njLoadTexture(&theres);
        printf("then the file's %d\n", gone.texaddr == there.texaddr && from(&gone, "T43.PVR"));
    } else if (!strcmp(run, "session")) {
        /* stages, each from its start's release of all: a list two tasks load and one of them releases, a texture
         * nothing releases, and four events, each a PVM of which a list keeps and releases three textures, and
         * then a movie, each of its pictures a load of one index after the last one's release. what a stage
         * keeps, it keeps while more than twice the records come and go */
        struct texname scratch[4] = {{0}}, event[3], leak;
        struct texinfo leak_info;
        struct texlist loader = {scratch, 4}, events = {event, 3}, leaks = {&leak, 1};
        uint32_t wrong = 0;

        for (uint32_t stage = 0; stage < STAGES; stage++) {
            uint32_t index = 1000 + 16 * (stage % 5);

            njReleaseTextureAll();
            file_kept(index);
            wrong += njLoadTexture(&keep) != 1 || njLoadTexture(&keep) != 1;
            memory_name(&leak, &leak_info, 5000 + stage % 7);
            njLoadTexture(&leaks);
            for (uint32_t i = 0; i < 4; i++) {
                njLoadTexturePvmMemory(pvm(300 + 4 * i, 4, NAMED | INDEXES), &loader);
                memcpy(event, scratch, sizeof event);
                wrong += !holds(&event[0], 300 + 4 * i) || !holds(&event[2], 302 + 4 * i);
                njReleaseTexture(&events);
                for (uint32_t picture = 0; picture < PICTURES; picture++) {
                    memory_name(&other, &other_info, 0x7fff0000);
                    wrong += njLoadTexture(&one) != 1 || !holds(&other, 0x7fff0000) || kept_held(index) != KEPT;
                    njReleaseTexture(&one);
                }
            }
            njReleaseTexture(&keep);
            wrong += kept_held(index) != KEPT || !holds(&leak, 5000 + stage % 7);
        }
        printf("wrong %u\n", wrong);
    } else if (!strcmp(run, "misses")) {
        /* searches, once ten times the records have come and gone */
        uint32_t wrong = 0;

        name_kept(1234);
        njLoadTexture(&keep);
        for (uint32_t i = 0; i < 10 * (RECORDS + 1); i++) {
            memory_name(&other, &other_info, 100000 + i);
            njLoadTexture(&one);
            njReleaseTexture(&one);
        }
        for (uint32_t i = 0; i < 2000000; i++) {
            wrong += search(100000 + i % (10 * (RECORDS + 1))) != NONE;
            wrong += search(1234 + i % KEPT) != record(kept[i % KEPT].texaddr);
        }
        printf("wrong %u\n", wrong);
    }
    native_report_state(stdout);
    return 0;
}
"""


class Textures(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # the records a load can hold: all but record 0
        shim = ROOT / 'engine/native/shim.c'
        cls.records = int(re.search(r'(?m)^#define TEXTURE_RECORDS (\d+)$', shim.read_text())[1]) - 1
        cls.built = tempfile.TemporaryDirectory()
        built = Path(cls.built.name)
        (built / 'driver.c').write_text(TEXTURES_DRIVER)
        compiler = os.environ.get('CC') or shutil.which('clang') or 'cc'
        cls.tool = built / 'textures'
        subprocess.run([compiler, '-std=gnu17', '-O1', '-fms-extensions', f'-DRECORDS={cls.records}',
                        f'-I{ROOT / "engine/native"}', f'-I{ROOT / "engine/numeric"}', '-o', cls.tool,
                        built / 'driver.c', shim], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.built.cleanup()

    def run_textures(self, run):
        """what the driver's run printed, less the game's state, and what it said. the last line it printed is the
        shim's own count of its records. a run takes under a second where a search, a load and a release each
        walk one chain"""
        ran = subprocess.run([self.tool, run], check=True, capture_output=True, text=True, timeout=30)
        lines = ran.stdout.splitlines()
        self.assertEqual(lines.pop(-2), '0 frames, game_mode 0, stage 0 act 0, character 0')
        return lines, ran.stderr

    def held(self, most, now, loads, late=0):
        return (f'sa1: texture memory: at most {most} records held of {self.records}, {now} now, {loads} loads in '
                f'all, {late} releases of a free record')

    def test_kept_textures_keep_their_records_while_more_than_the_records_come_and_go(self):
        # each texture that comes holds its own, each kept one stays found, and a search of an index no record
        # holds answers record 0, zeros
        self.assertEqual(self.run_textures('kept'), ([
            'ahead of any load a miss answers record 0, the list set 1', 'wrong 0, kept 8',
            'a miss reads 0, 0x0, at record 0',
            self.held(9, 8, 8 + 4 * (self.records + 1))], ''))

    def test_a_record_two_loads_share_goes_at_the_last_release(self):
        # the adapter hears of each record at its last release alone, with its index in place
        heard = sum(range(77, 85))
        self.assertEqual(self.run_textures('users'), ([
            'shared 1', 'after one release: kept 8, the adapter heard 0',
            f'after the last: kept 0, found 0, the adapter heard {heard}',
            f'after one too many: the next held 1, apart 1, the adapter heard {heard}',
            self.held(8, 1, 17, late=8)], ''))

    def test_every_record_goes_when_all_are_released(self):
        # the adapter hears of each record once, whatever its users
        self.assertEqual(self.run_textures('all'), ([
            'kept 0, found 0', f'the adapter heard {sum(range(500, 508)) + 77}', 'the next held 1',
            self.held(9, 1, 18, late=8)], ''))

    def test_a_load_with_no_record_free_fails_and_takes_no_held_record(self):
        self.assertEqual(self.run_textures('full'), ([
            f'took {self.records}, the next at record 0', 'wrong 0', 'a held index loaded 1, shared 1',
            'then loaded 1, held 1, in the record freed first 1', 'the other freed taken 1, and reads 0, 0x0',
            self.held(self.records, self.records, self.records + 3)],
            f"sa1: none of texture memory's {self.records} records is free\n"))

    def test_a_pvms_records_go_by_the_lists_that_kept_their_addresses(self):
        # the fourth texture, which no list kept, stays until all are released
        self.assertEqual(self.run_textures('pvm'), ([
            'read 1', 'shared 1', 'after one list: held 2, the adapter heard 0', 'after both: held 0, the fourth 1',
            f'after all: the fourth 0, the adapter heard {sum(range(300, 304))} of 8',
            self.held(4, 0, 8)], ''))

    def test_a_pvms_entries_are_read_by_what_its_status_gives_of_them(self):
        # an entry of an index alone gives its record that index and the adapter no name, and one of a name alone
        # gives the adapter its name and its record no index
        self.assertEqual(self.run_textures('status'), ([
            'indexes: held 2, named ""', 'names: indexes 0 and 0, sized 1, named "T500"',
            self.held(4, 4, 4)], ''))

    def test_a_files_index_shares_its_record_and_a_name_with_no_file_shares_none(self):
        # a record a name took with no file to read takes its size from the first load that reads one
        self.assertEqual(self.run_textures('files'), ([
            'shared 1, held 1', 'apart 1, the newest found 1', 'after their release: found 0',
            "shared 1, held with the file's size 1", self.held(3, 2, 6)], ''))

    def test_a_record_is_of_the_file_its_first_load_read(self):
        # the adapter asks a record's file by the address a name keeps: a PVM's is the file the game opened and
        # decoded to where the load reads it, a PVR's its own, and a record another file's load shares stays its
        # first file's until it is freed. a PVM in bytes no decode wrote, as a download's, a texture made in
        # memory and a name with no file are no file's
        self.assertEqual(self.run_textures('from'), ([
            "a decoded PVM: held 2, its file's 2", "a file of its index: shared 1, its first file's 1",
            'after their release: 0', "the file first: shared 1, its own 1, the PVM's other texture the PVM's 1",
            "a PVM no decode wrote: held 1, a file's 0", "memory's 0, no record's 0, no file's 0",
            "then the file's 1", self.held(4, 4, 11)], ''))

    def test_stages_of_events_and_movies_past_the_records_keep_what_they_hold(self):
        # a stage holds sixteen records at most: its list's eight, its leak, three events' fourth textures and the
        # last event's four. it ends with thirteen, each event's fourth among them. its four movies' pictures
        # come to more than twice the records
        pictures = self.records // 2 + 1
        self.assertEqual(self.run_textures('session'), ([
            'wrong 0', self.held(16, 13, 20 * (17 + 4 * (4 + pictures)))], ''))

    def test_searches_answer_once_ten_times_the_records_have_come_and_gone(self):
        # four million of them, which run_textures' wait ends where each walks every record there has been
        self.assertEqual(self.run_textures('misses'), ([
            'wrong 0', self.held(9, 8, 8 + 10 * (self.records + 1))], ''))


# Shinobi's heap and timer, as the shim gives them. the heap's map is
# SHIM_READS', which maps nothing, so a block is an address alone
SHINOBI_DRIVER = SHIM_READS + r"""
void sa1_textures_loaded(const void *texlist, uint32_t count, const char *const *names)
{
    (void)texlist, (void)count, (void)names;
}
void sa1_textures_released(const void *texlist) { (void)texlist; }
uint32_t native_disc_file_id(const char *name) { (void)name; return 0; }
FILE *native_disc_file(const char *name) { (void)name; return NULL; }
void *syMalloc(uint32_t);
void syFree(void *);
uint32_t syTmrGetCount(void);

/* each word a size to take, printed as the block's bytes past the heap's first block, or none where the heap
 * refuses it; free, to give the last block back; or a count of vsyncs, after an at, printed as the timer's count */
int main(int argc, char **argv)
{
    char *first = syMalloc(0), *last = first;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "free")) {
            syFree(last);
        } else if (!strcmp(argv[i], "at") && i + 1 < argc) {
            native_vsyncs = strtoul(argv[++i], NULL, 0);
            printf("%u\n", syTmrGetCount());
        } else if ((last = syMalloc((uint32_t)strtoul(argv[i], NULL, 0)))) {
            printf("%lu\n", (unsigned long)(last - first));
        } else {
            puts("none");
        }
    }
    return 0;
}
"""


class Shinobi(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        shim = ROOT / 'engine/native/shim.c'
        # the heap's bytes, and a vsync in the timer's counts as the game's C reads it
        cls.size = int(re.search(r'(?m)^#define HEAP_SIZE (\w+)UL$', shim.read_text())[1], 16)
        cls.vsync = int(re.search(r'(?m)^#define VSYNC_COUNTS (\d+)$',
                                  (ROOT / 'engine/dc/dreamcast.h').read_text())[1])
        cls.built = tempfile.TemporaryDirectory()
        built = Path(cls.built.name)
        (built / 'driver.c').write_text(SHINOBI_DRIVER)
        compiler = os.environ.get('CC') or shutil.which('clang') or 'cc'
        cls.tool = built / 'shinobi'
        subprocess.run([compiler, '-std=gnu17', '-O1', '-fms-extensions', f'-I{ROOT / "engine/native"}',
                        f'-I{ROOT / "engine/numeric"}', '-o', cls.tool, built / 'driver.c', shim], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.built.cleanup()

    def said(self, *words):
        return subprocess.run([self.tool, *map(str, words)], check=True, capture_output=True,
                              text=True).stdout.splitlines()

    def test_a_block_takes_the_next_bytes_to_32_and_a_freed_one_gives_none_back(self):
        self.assertEqual(self.said(1, 32, 33, 0, 'free', 5), ['0', '32', '64', '128', '128'])

    def test_a_size_past_the_bytes_left_is_refused_and_the_heap_gives_the_rest(self):
        # the heap whole, then nothing; and a block more than it has left, which takes none of them
        self.assertEqual(self.said(self.size, 1, 0), ['0', 'none', str(self.size)])
        self.assertEqual(self.said(self.size - 64, 65, 64, 1), ['0', 'none', str(self.size - 64), 'none'])
        # a size that rounds past 32 bits is no block of none
        self.assertEqual(self.said(0xffffffff, 0xffffffe1, 32), ['none', 'none', '0'])

    def test_the_timer_gains_a_vsync_s_counts_at_each_vsync_and_wraps_at_32_bits(self):
        most = 0xffffffff // self.vsync
        self.assertEqual(self.said('at', 0, 'at', 1, 'at', 600, 'at', most, 'at', most + 1),
                         ['0', str(self.vsync), str(600 * self.vsync), str(most * self.vsync),
                          str((most + 1) * self.vsync - (1 << 32))])


# Kamui's two sources of the slice's, with what they name that the slice gives them elsewhere. the probe reads
# kamui.h under the prelude as they do, and no header of the host's, so a build can give all three a long of 32 bits
KAMUI_PROBE = r"""
#include <kamui.h>

/* the device's state and the lists' words, zeros as stubs.py gives them, and calls that do nothing */
volatile struct kmi_device_extension kmiDeviceExtension;
KMCURRENTLISTSTATE kmiCurrentListState;
KMDWORD kmiCurrentListType;
PKMDWORD *kmiCurrentVertexBuff, *kmiDstVertexBuffer;
KMSTATUS (*kmiBackGroundPlaneFuncs[18])(void *pVertex[3]);
PKMDWORD kmiMemCopy8(PKMDWORD to, PKMDWORD from, KMDWORD size) { return to; }
void kmiDiscardRenderPipeline(void) {}
void kmiResetBoard(void) {}
KMDWORD kmiReadRegister(KMDWORD reg) { return 0; }
KMBOOLEAN kmiQueryFinishDMA(KMDWORD handle) { return 1; }

/* each register's write, in the 32 bits the shim's kmiWriteRegister takes of it */
unsigned int probe_writes[4096][2], probe_written;
KMINT32 kmiWriteRegister(KMDWORD reg, KMDWORD value)
{
    probe_writes[probe_written % 4096][0] = (unsigned int)reg;
    probe_writes[probe_written % 4096][1] = (unsigned int)value;
    probe_written++;
    return 0;
}

static unsigned int seed = 1;
static unsigned int word(void)
{
    seed = seed * 1103515245U + 12345U;
    return seed;
}
static unsigned int below(unsigned int most) { return (word() >> 8) % most; }
static float unit(void) { return (float)below(256) / 255.0F; }

/* a context a seed fills, each field with a value Kamui has a meaning for, over header words of any bits, through
 * kmProcessVertexRenderState and kmSetVertexRenderState. words takes its four header words, then the sixteen of
 * the global parameter, each in the 32 bits Ninja's C and the adapter's read */
void probe_context(unsigned int *words)
{
    KMVERTEXCONTEXT c = {0};
    const KMDWORD *global = &kmiGlobalWork.control;
    int i;

    c.RenderState = word() & 0x1fffffff;
    c.ParamType = below(3), c.ListType = below(5), c.ColorType = below(4), c.UVFormat = below(2);
    c.DepthMode = below(8), c.CullingMode = below(4), c.ShadingMode = below(4), c.SelectModifier = below(2);
    c.bZWriteDisable = below(2), c.SRCBlendingMode = below(12), c.DSTBlendingMode = below(12);
    c.bSRCSel = below(2), c.bDSTSel = below(2), c.FogMode = below(4), c.bUseSpecular = below(2);
    c.bUseAlpha = below(2), c.bIgnoreTextureAlpha = below(2), c.ClampUV = below(4), c.FlipUV = below(4);
    c.FilterMode = below(4), c.bSuperSample = below(2), c.MipMapAdjust = below(16);
    c.TextureShadingMode = below(4), c.bColorClamp = below(2), c.PaletteBank = below(64);
    c.fFaceColorAlpha = unit(), c.fFaceColorRed = unit(), c.fFaceColorGreen = unit(), c.fFaceColorBlue = unit();
    c.fOffsetColorAlpha = unit(), c.fOffsetColorRed = unit(), c.fOffsetColorGreen = unit();
    c.fOffsetColorBlue = unit();
    c.GLOBALPARAMBUFFER = word(), c.ISPPARAMBUFFER = word(), c.TSPPARAMBUFFER = word();
    c.TexturePARAMBUFFER = word(), c.ModifierInstruction = below(3) << 29;
    c.bDCalcExact = below(2), c.StripLength = below(4), c.UserClipMode = below(4);
    kmProcessVertexRenderState(&c);
    kmSetVertexRenderState(&c);
    words[0] = (unsigned int)c.GLOBALPARAMBUFFER, words[1] = (unsigned int)c.ISPPARAMBUFFER;
    words[2] = (unsigned int)c.TSPPARAMBUFFER, words[3] = (unsigned int)c.TexturePARAMBUFFER;
    for (i = 0; i < 16; i++) {
        words[4 + i] = (unsigned int)global[i];
    }
}

/* the setters that write a register, each with what a seed gives it */
void probe_globals(void)
{
    static KMPALETTEDATA palette;
    KMPACKEDARGB color, other;
    KMFLOAT fog[128];
    int i;

    color.dwPacked = word(), other.dwPacked = word();
    kmSetFogTableColor(color), kmSetFogVertexColor(other), kmSetColorClampValue(color, other);
    kmSetBorderColor(other), kmSetFogDensity(word() & 0xffff);
    for (i = 0; i < 128; i++) {
        fog[i] = unit();
    }
    kmSetFogTable(fog), kmSetCullingRegister(unit() + 0.01F), kmSetAlphaThreshold((int)below(512) - 128);
    kmSetPaletteMode(below(4));
    for (i = 0; i < KM_PALETTE_ENTRY; i++) {
        palette.dwPaletteData[i] = word();
    }
    kmSetPaletteData(&palette), kmSetPaletteBankData(below(1024), below(64), &palette);
    kmSetCheapShadowMode((int)below(512) - 256);
}
"""

KAMUI_DRIVER = r"""
#include <stdio.h>
void probe_context(unsigned int *words);
void probe_globals(void);
extern unsigned int probe_writes[4096][2], probe_written;

int main(void)
{
    unsigned int words[20];

    for (int n = 0; n < 512; n++) {
        probe_context(words);
        for (int i = 0; i < 20; i++) {
            printf("%08x%c", words[i], i == 19 ? '\n' : ' ');
        }
    }
    probe_globals();
    probe_globals();
    for (unsigned int n = 0; n < probe_written; n++) {
        printf("%03x %08x\n", probe_writes[n][0], probe_writes[n][1]);
    }
    return 0;
}
"""


class Kamui(unittest.TestCase):
    def test_a_long_of_either_width_gives_the_slice_the_same_header_words_and_registers(self):
        # prelude.h leaves kamui.h's KMDWORD a long, 64 bits on an LP64 host, where its records lie wider than the
        # SH-4's. what leaves them is 32 bits wide, and the same bits as a build whose long holds 32
        compiler = os.environ.get('CC') or shutil.which('clang') or 'cc'
        kamui = ['-std=gnu89', '-O1', '-DNONMATCHING', '-w', '-fwrapv', '-fno-strict-aliasing', '-include',
                 ROOT / 'engine/native/prelude.h', f'-I{ROOT / "sdk/include"}', f'-I{ROOT / "engine/native"}']
        sources = [ROOT / 'sdk/ninja/kmvertex_kamui_lib.c', ROOT / 'sdk/ninja/kmglobal_kamui_lib.c']
        said = []
        with tempfile.TemporaryDirectory() as d:
            (Path(d) / 'probe.c').write_text(KAMUI_PROBE)
            (Path(d) / 'driver.c').write_text(KAMUI_DRIVER)
            for name, width in (('long', []), ('int', ['-Dlong=int'])):
                objects = [Path(d) / f'{name}{n}.o' for n in range(4)]
                for source, made in zip([Path(d) / 'probe.c', *sources], objects):
                    subprocess.run([compiler, *kamui, *width, '-c', '-o', made, source], check=True)
                subprocess.run([compiler, '-std=gnu17', '-O1', '-c', '-o', objects[3], Path(d) / 'driver.c'],
                               check=True)
                subprocess.run([compiler, '-o', Path(d) / name, *objects], check=True)
                said.append(subprocess.run([Path(d) / name], check=True, capture_output=True,
                                           text=True).stdout.splitlines())
        self.assertEqual(said[0], said[1])
        # the seed's contexts differ, and its setters write the fog's table and the palette twice among theirs
        self.assertEqual(len(set(said[0][:512])), 512)
        self.assertGreater(len(said[0]) - 512, 2 * (128 + 1024))


# Ninja's sources that the motions' tests run, each one of the slice's by engine/mk/native.mk's lists: those whose
# inline bodies asm2c.py writes as C, and those the slice compiles through its passes
MOTION_BODIES = ('njInitMatrix', 'njPushMatrix', 'njPopMatrix', 'njUnitMatrix', 'njMultiMatrix', 'njTranslate',
                 'njRotate', 'njRotateX', 'njRotateY', 'njRotateZ', 'njRotateXYZ', 'njRotateZXY', 'njScale',
                 'njCalcVector', 'njUnitVector', 'njTan', 'njMotSrc', 'njCameraMotion', 'njSetCamera',
                 'njPointCamereInterest', 'njRollCameraInterest', 'njSetCameraAngle')
MOTION_C = ('njGlobal', 'njMotSrc', 'njGetMotion', 'njLinear', 'njCameraMotion')
# a tree's motion, of the factors the motions the game takes a pose from hold, a place and angles, each a line
# between its keys: its root's places and angles at three and two frames, and its child's one key of angles. the
# child has no keys of places, so its own place stands
ROOT_PLACES = ((0, (1.0, 2.0, 3.0)), (4, (5.0, 10.0, -3.0)), (10, (11.0, -2.0, 9.0)))
ROOT_ANGLES = ((0, (0, 0x1000, 0)), (10, (0x4000, 0x2000, -0x800)))
CHILD_ANGLES = ((0, (100, 200, 300)),)
# a camera's motion, of the factors the game's camera actions hold, each a line between its keys: its place, the
# point it looks at, level with it at every frame, its roll and its angle of view
VIEW_PLACES = ((0, (10.0, 20.0, 30.0)), (4, (50.0, -20.0, 70.0)), (10, (-10.0, 5.0, 0.0)))
VIEW_POINTS = ((0, (40.0, 20.0, 70.0)), (4, (50.0, -20.0, 120.0)), (10, (-40.0, 5.0, 40.0)))
VIEW_ROLLS = ((0, (0,)), (10, (0x4000,)))
VIEW_ANGLES = ((0, (0x4000,)), (10, (0x2000,)))
# the first frame, one between two keys, the last, and one the motions don't reach
FRAMES = (0.0, 2.5, 10.0)
PAST = 11.0
# what the shim and stubs.py give Ninja's sources in the slice: FSRRA by play math, the unit matrix, and the camera's
# and the view's matrices as zeros. the matrix stack is the driver's own
NINJA_GIVEN = r"""
#include <ninja.h>

int printf(const char *format, ...);
void native_asm_init(void);
float sa_fsrra(float n);

float _builtin_fsrra(float n)
{
    return sa_fsrra(n);
}
NJS_MATRIX _unit_matrix_ = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
NJS_MATRIX _nj_camera_matrix_, _nj_view_matrix_;
static NJS_MATRIX matrices[16];
"""
# the tree and the camera, with their motions, which main joins as it starts. each line it prints is a call's
# name, its frame and what the call wrote: the two nodes' places, angles and scales, over a -1 in each, njLinear's
# point, or the view's matrix, the screen's distance and the two clip planes
MOTION_DRIVER = NINJA_GIVEN + r"""
static NJS_OBJECT nodes[2];
static NJS_MKEY_F root_places[] = {@ROOT_PLACES@};
static NJS_MKEY_A root_angles[] = {@ROOT_ANGLES@};
static NJS_MKEY_A child_angles[] = {@CHILD_ANGLES@};
static NJS_MDATA2 records[2];
static NJS_MOTION motion;
static NJS_ACTION action;
static Float places[6], scales[6];
static Angle angles[6];
static NJS_MOTION_DATA pose = {places, angles, scales};
static Float ends[6] = {1.0F, 2.0F, 3.0F, 5.0F, 10.0F, -3.0F};

struct angle_key {
    Uint32 keyframe;
    Angle key;
};
static NJS_MKEY_F view_places[] = {@VIEW_PLACES@};
static NJS_MKEY_F view_points[] = {@VIEW_POINTS@};
static struct angle_key view_rolls[] = {@VIEW_ROLLS@};
static struct angle_key view_angles[] = {@VIEW_ANGLES@};
static NJS_MDATA4 view_record;
static NJS_MOTION view_motion;
static NJS_CAMERA camera;
static NJS_CACTION view_action;

static void posed(const char *name, Float frame, Bool by_action)
{
    int i;

    for (i = 0; i < 6; i++) {
        places[i] = scales[i] = -1.0F;
        angles[i] = -1;
    }
    if (by_action) {
        njGetAction(&action, &pose, frame);
    } else {
        njGetDrawMotion(nodes, &motion, &pose, frame);
    }
    printf("%s %g", name, frame);
    for (i = 0; i < 6; i++) {
        printf(" %.9g", places[i]);
    }
    for (i = 0; i < 6; i++) {
        printf(" %d", (int)angles[i]);
    }
    for (i = 0; i < 6; i++) {
        printf(" %.9g", scales[i]);
    }
    printf("\n");
}

int main(void)
{
    static const Float frames[] = {@FRAMES@, @PAST@};
    Float at[3];
    int i, j;

    native_asm_init();
    njInitMatrix(matrices, 16, 0);
    nodes[0].scl[0] = 1.0F, nodes[0].scl[1] = 2.0F, nodes[0].scl[2] = 3.0F;
    nodes[0].child = &nodes[1];
    nodes[1].pos[0] = 7.0F, nodes[1].pos[1] = 8.0F, nodes[1].pos[2] = 9.0F;
    nodes[1].scl[0] = 4.0F, nodes[1].scl[1] = 5.0F, nodes[1].scl[2] = 6.0F;
    records[0].p[0] = root_places, records[0].nb[0] = sizeof root_places / sizeof *root_places;
    records[0].p[1] = root_angles, records[0].nb[1] = sizeof root_angles / sizeof *root_angles;
    records[1].p[1] = child_angles, records[1].nb[1] = sizeof child_angles / sizeof *child_angles;
    motion.mdata = records, motion.nbFrame = (Uint32)@PAST@;
    motion.type = NJD_MTYPE_POS_0 | NJD_MTYPE_ANG_1, motion.inp_fn = 2;
    action.object = nodes, action.motion = &motion;
    for (i = 0; i < 4; i++) {
        posed("action", frames[i], TRUE);
        posed("motion", frames[i], FALSE);
    }
    for (i = 0; i < 3; i++) {
        njLinear(ends, at, NULL, frames[i] / 10.0F);
        printf("linear %g %.9g %.9g %.9g\n", frames[i] / 10.0F, at[0], at[1], at[2]);
    }
    _nj_screen_.w = 640.0F, _nj_scr_aspect_x_ = 1.0F;
    camera.n_clip = -1.0F, camera.f_clip = -10000.0F;
    view_record.p[0] = view_places, view_record.nb[0] = sizeof view_places / sizeof *view_places;
    view_record.p[1] = view_points, view_record.nb[1] = sizeof view_points / sizeof *view_points;
    view_record.p[2] = view_rolls, view_record.nb[2] = sizeof view_rolls / sizeof *view_rolls;
    view_record.p[3] = view_angles, view_record.nb[3] = sizeof view_angles / sizeof *view_angles;
    view_motion.mdata = &view_record, view_motion.nbFrame = (Uint32)@PAST@;
    view_motion.type = NJD_MTYPE_POS_0 | NJD_MTYPE_TARGET_3 | NJD_MTYPE_ROLL_6 | NJD_MTYPE_ANGLE_7;
    view_motion.inp_fn = 4;
    view_action.camera = &camera, view_action.motion = &view_motion;
    for (i = 0; i < 3; i++) {
        njCameraAction(&view_action, frames[i]);
        printf("view %g", frames[i]);
        for (j = 0; j < 16; j++) {
            printf(" %.9g", _nj_view_matrix_[j]);
        }
        printf(" %.9g %.9g %.9g\n", _nj_screen_.dist, _nj_near_clip_, _nj_far_clip_);
    }
    return 0;
}
"""


# Ninja's sources that a motion link and a light's setters run, each one of the slice's by the same two lists
LINK_BODIES = ('njInitMatrix', 'njPushMatrix', 'njPopMatrix', 'njUnitMatrix', 'njGetMatrix', 'njTranslate',
               'njRotateXYZ', 'njRotateZXY', 'njScale', 'njMotLinkSrc')
LINK_C = ('njGlobal', 'njMotLinkS', 'njMotLinkSrc', 'njSetLightAlpha', 'njSetLightLocation')
# a link's two motions of one node, each its places and its angles, about z alone, and the frame the link holds
# each at
FROM_PLACES = ((0, (1.0, 2.0, 3.0)), (4, (5.0, 10.0, -3.0)))
FROM_ANGLES = ((0, (0, 0, 0x1000)), (4, (0, 0, 0x3000)))
TO_PLACES = ((0, (-8.0, 0.0, 4.0)), (10, (12.0, 40.0, -6.0)))
TO_ANGLES = ((0, (0, 0, -0x2000)),)
FROM_FRAME, TO_FRAME = 2.5, 7.0
# the link's rate at its first motion, between the two and at its second, and one past it, which draws nothing
RATES = (0.0, 0.25, 1.0)
OVER = 1.5
# a light's color, as alpha, red, green and blue, its intensity and its specular, diffuse and ambient shares, then
# the alpha and the place its setters give it
LIGHT = {'color': (1.0, 0.75, 1.0, 0.5), 'intensity': 0.5, 'specular': 0.25, 'diffuse': 0.5, 'ambient': 2.0,
         'alpha': 0.5, 'place': (1.0, 2.0, 3.0)}
# the node, which has no model, with the link's motions, and the light. njGetkmPointer, njSetkmPointer and the
# simple drawer are the driver's, and do nothing. each of the link's lines is its rate, the nodes it has posed so
# far, and the matrix the last of them stood in, as the callback njSetMotionCallback sets reads it. the light's
# line is its color, its four scaled colors, its place and the copy Ninja keeps of that
LINK_DRIVER = NINJA_GIVEN + r"""
void njGetkmPointer(void)
{
}
void njSetkmPointer(void)
{
}
void njSimpleDrawModel(NJS_MODEL *model)
{
}

static NJS_OBJECT node;
static NJS_MKEY_F from_places[] = {@FROM_PLACES@};
static NJS_MKEY_A from_angles[] = {@FROM_ANGLES@};
static NJS_MKEY_F to_places[] = {@TO_PLACES@};
static NJS_MKEY_A to_angles[] = {@TO_ANGLES@};
static NJS_MDATA2 from_record, to_record;
static NJS_MOTION from, to;
static NJS_MATRIX stood;
static int posed;
static NJS_LIGHT light;

static void pose(NJS_OBJECT *object)
{
    posed++;
    njGetMatrix(&stood);
}

static void colors(const NJS_ARGB *color)
{
    printf(" %.9g %.9g %.9g %.9g", color->a, color->r, color->g, color->b);
}

int main(void)
{
    static const Float rates[] = {@RATES@, @OVER@};
    NJS_MOTION_LINK link;
    NJS_ACTION_LINK action;
    int i, j;

    native_asm_init();
    njInitMatrix(matrices, 16, 0);
    njUnitMatrix(NULL);
    _nj_motion_callback_ = pose;
    node.scl[0] = node.scl[1] = node.scl[2] = 1.0F;
    from_record.p[0] = from_places, from_record.nb[0] = sizeof from_places / sizeof *from_places;
    from_record.p[1] = from_angles, from_record.nb[1] = sizeof from_angles / sizeof *from_angles;
    to_record.p[0] = to_places, to_record.nb[0] = sizeof to_places / sizeof *to_places;
    to_record.p[1] = to_angles, to_record.nb[1] = sizeof to_angles / sizeof *to_angles;
    from.mdata = &from_record, from.nbFrame = 5, from.type = NJD_MTYPE_POS_0 | NJD_MTYPE_ANG_1, from.inp_fn = 2;
    to.mdata = &to_record, to.nbFrame = 11, to.type = NJD_MTYPE_POS_0 | NJD_MTYPE_ANG_1, to.inp_fn = 2;
    link.motion[0] = &from, link.motion[1] = &to, link.frame[0] = @FROM_FRAME@, link.frame[1] = @TO_FRAME@;
    action.object = &node, action.motionlink = &link;
    for (i = 0; i < 4; i++) {
        njSimpleActionLink(&action, rates[i]);
        printf("link %g %d", rates[i], posed);
        for (j = 0; j < 16; j++) {
            printf(" %.9g", stood[j]);
        }
        printf("\n");
    }
    light.attr.argb.a = @LIGHT_A@, light.attr.argb.r = @LIGHT_R@;
    light.attr.argb.g = @LIGHT_G@, light.attr.argb.b = @LIGHT_B@;
    light.ltcal.intns = @LIGHT_INTENSITY@, light.attr.ispc = @LIGHT_SPECULAR@;
    light.attr.idif = @LIGHT_DIFFUSE@, light.attr.iamb = @LIGHT_AMBIENT@;
    njSetLightAlpha(&light, @LIGHT_ALPHA@);
    njSetLightLocation(&light, @LIGHT_PLACE@);
    printf("light");
    colors(&light.attr.argb), colors(&light.ltcal.atten), colors(&light.ltcal.spc);
    colors(&light.ltcal.dif), colors(&light.ltcal.amb);
    printf(" %.9g %.9g %.9g %.9g %.9g %.9g\n", light.pnt.x, light.pnt.y, light.pnt.z, light.ltcal.lpnt.x,
           light.ltcal.lpnt.y, light.ltcal.lpnt.z);
    return 0;
}
"""


def keys_c(keys):
    """keys, each a frame and its values, as the C of an array's initializer"""
    return ', '.join('{%d, {%s}}' % (frame, ', '.join(f'{value}F' if isinstance(value, float) else str(value)
                                                      for value in values)) for frame, values in keys)


def keyed(keys, frame):
    """keys' values at frame, by Ninja's linear rule: the last key at or before the frame's whole part, blended
    toward the next by the frame's share of the frames between the two. the key after the last is the first, and
    the share is then the frames past the last"""
    at = max((index for index, (start, _) in enumerate(keys) if start <= int(frame)), default=0)
    (start, here), (end, there) = keys[at], keys[(at + 1) % len(keys)]
    share = (frame - start) / (end - start if end > start else 1)
    return [a * (1 - share) + b * share for a, b in zip(here, there)]


def slice_ninja():
    """Ninja's sources of the slice's, by name without .c, as engine/mk/native.mk lists them: those asm2c.py
    reads, NATIVE_NINJA, and all the slice compiles, NATIVE_SDK_C, which holds NATIVE_NINJA's too"""
    rules = (ROOT / 'engine/mk/native.mk').read_text()
    read, compiled = (re.search(rf'(?m)^{name} +:= ((?:.*\\\n)*.*)$', rules)[1]
                      for name in ('NATIVE_NINJA', 'NATIVE_SDK_C'))
    read = read.replace('\\\n', ' ').split()
    return set(read), {*read, *re.findall(r'(\w+)\.c\b', compiled)}


def ninja_prints(case, driver, bodies, compiled):
    """what driver prints, a list of words a line, run on Ninja's sources as the slice builds them: asm2c.py's C of
    bodies' inline bodies, on the host's play math, and compiled's objects and the driver's own, each through the
    float pass under R10, marks.py's marks and refs.py, by the flags engine/mk/native.mk compiles the SDK's
    under. case's test ends where a source is none of the slice's, and is skipped where clang compiles for no
    x86"""
    read, built = slice_ninja()
    case.assertEqual((set(bodies) - read, set(compiled) - built), (set(), set()))
    if not shutil.which('clang'):
        case.skipTest('no clang')
    target = subprocess.run(['clang', '-dumpmachine'], check=True, capture_output=True, text=True).stdout
    if not target.startswith('x86_64'):
        case.skipTest(f"clang compiles for {target.strip()}, and the slice's C and asm2c.py's for x86-64")
    math_objects = os.environ.get('PLAY_MATH', '').split()
    case.assertTrue(math_objects, "engine/tests/native_tools.py: PLAY_MATH names no object of play math's reference, "
                                  "which asm2c.py's C links: run make engine-native-tools, which builds them and "
                                  'names them')
    resource = subprocess.run(['clang', '-print-resource-dir'], check=True, capture_output=True, text=True).stdout
    flags = ['-std=gnu89', '-ffreestanding', '-nostdinc', '-isystem', f'{resource.strip()}/include', '-DNONMATCHING',
             '-include', ROOT / 'engine/native/prelude.h', '-O2', '-fcommon', '-fno-strict-aliasing',
             '-fno-delete-null-pointer-checks', '-fwrapv', '-fno-builtin', '-fms-extensions', '-mfma',
             '-frounding-math', '-ffp-contract=off', '-w']
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder)
        (path / 'driver.c').write_text(driver)
        (path / 'overlaid.txt').write_text('')
        subprocess.run([sys.executable, ROOT / 'engine/tools/asm2c.py', path / 'bodies.c',
                        *(ROOT / f'sdk/ninja/{name}.c' for name in bodies)], check=True, capture_output=True)
        objects = []
        for source in (*(ROOT / f'sdk/ninja/{name}.c' for name in compiled), path / 'driver.c'):
            text, ir, made = (path / f'{source.stem}{suffix}' for suffix in ('.i', '.ll', '.o'))
            subprocess.run(['clang', *flags, '-I', ROOT / 'sdk/include', '-I', ROOT / 'sdk', '-E', '-o', text, source],
                           check=True)
            subprocess.run([sys.executable, ROOT / 'tools/floats.py', '--shc', 'r10', text], check=True)
            marks.preprocessed(path, text)
            subprocess.run(['clang', *flags, '-S', '-emit-llvm', '-Xclang', '-disable-llvm-passes', '-o', ir, text],
                           check=True)
            ir.write_text(refs.Pass().run(ir.read_text(), ir))
            subprocess.run(['clang', *flags, '-c', '-o', made, ir], check=True)
            objects.append(made)
        subprocess.run(['clang', '-std=gnu17', '-no-pie', '-w', '-I', ROOT / 'engine/native', '-I',
                        ROOT / 'engine/numeric', '-o', path / 'driver', path / 'bodies.c', *objects,
                        *(ROOT / name for name in math_objects), '-lm'], check=True)
        ran = subprocess.run([path / 'driver'], check=True, capture_output=True, text=True)
    return [line.split() for line in ran.stdout.splitlines()]


class Motions(unittest.TestCase):
    def near(self, found, wanted, delta):
        """each of found's numbers lies within delta of wanted's"""
        self.assertEqual(len(found), len(wanted))
        for index, (got, want) in enumerate(zip(found, wanted)):
            self.assertAlmostEqual(got, want, delta=delta, msg=f'{index}: {found} for {wanted}')

    def test_a_pose_and_a_cameras_view_at_a_frame_are_their_keys(self):
        # the lifts, the monorails and the camera's actions take these from Ninja's own sources, so no row of
        # stubs.txt answers for them, and the matrix the camera's bodies build in has the row its zeros need
        for host in stubs.HOSTS:
            rows = stubs.rows(ROOT / 'engine/native/stubs.txt', host)
            self.assertEqual({'njGetAction', 'njGetDrawMotion', 'njLinear', 'njCameraAction'} & set(rows), set())
            self.assertEqual(rows['_nj_camera_matrix_'].kind, stubs.ZEROS)
        driver = MOTION_DRIVER.replace('@FRAMES@', ', '.join(f'{frame}F' for frame in FRAMES))
        for name, keys in (('ROOT_PLACES', ROOT_PLACES), ('ROOT_ANGLES', ROOT_ANGLES), ('CHILD_ANGLES', CHILD_ANGLES),
                           ('VIEW_PLACES', VIEW_PLACES), ('VIEW_POINTS', VIEW_POINTS), ('VIEW_ROLLS', VIEW_ROLLS),
                           ('VIEW_ANGLES', VIEW_ANGLES)):
            driver = driver.replace(f'@{name}@', keys_c(keys))
        lines = ninja_prints(self, driver.replace('@PAST@', f'{PAST}F'), MOTION_BODIES, MOTION_C)
        said = {(name, float(frame)): [float(word) for word in words] for name, frame, *words in lines}
        self.assertEqual(len(said), len(lines))
        for frame in FRAMES:
            with self.subTest(frame=frame):
                # an action's pose and a motion's are one: the root's keys, the child's own place where it has no
                # keys, and each node's own scale, which the motion doesn't move. an angle is cut to a whole one
                pose = [*keyed(ROOT_PLACES, frame), 7, 8, 9, *map(int, keyed(ROOT_ANGLES, frame)),
                        *keyed(CHILD_ANGLES, frame), 1, 2, 3, 4, 5, 6]
                self.near(said['action', frame], pose, 1e-4)
                self.near(said['motion', frame], pose, 1e-4)
                # njLinear's point lies the frame's share of the way from its first three Floats to its last
                share = frame / 10
                self.near(said['linear', share], [a * (1 - share) + b * share for a, b in ((1, 5), (2, 10), (3, -3))],
                          1e-4)
                *view, distance, front, back = said['view', frame]

                def seen(point):
                    # Ninja's matrix turns a row, and holds the translation in its last four
                    return [sum(point[row] * view[4 * row + column] for row in range(3)) + view[12 + column]
                            for column in range(3)]

                place, point = keyed(VIEW_PLACES, frame), keyed(VIEW_POINTS, frame)
                roll, angle = (math.radians(int(*keyed(keys, frame)) * 360 / 0x10000)
                               for keys in (VIEW_ROLLS, VIEW_ANGLES))
                # the camera stands at the view's origin with its point straight ahead, and a point 10 above
                # that one lies the roll round the line of sight
                ahead = math.dist(place, point)
                self.near(seen(place), [0, 0, 0], 0.05)
                self.near(seen(point), [0, 0, ahead], 0.05)
                self.near([*map(abs, seen([point[0], point[1] + 10, point[2]])[:2])],
                          [10 * abs(math.sin(roll)), 10 * abs(math.cos(roll))], 0.05)
                # the screen lies where its 640 fill the angle of view, and the planes where the camera clips
                self.near([distance, front, back], [640 / (2 * math.tan(angle / 2)), 1, 10000], 0.5)
        # a frame the motion doesn't reach writes none of a pose
        self.assertEqual(said['action', PAST], [-1] * 18)
        self.assertEqual(said['motion', PAST], [-1] * 18)

    def test_a_link_poses_a_node_between_two_motions_and_a_lights_setters_write_its_record(self):
        # a Chao between two motions and Speed Highway's spotlight take these from Ninja's own sources too
        for host in stubs.HOSTS:
            rows = stubs.rows(ROOT / 'engine/native/stubs.txt', host)
            self.assertEqual({'njSimpleActionLink', 'njSetLightAlpha', 'njSetLightLocation'} & set(rows), set())
        floats = {'RATES': RATES, 'OVER': (OVER,), 'FROM_FRAME': (FROM_FRAME,), 'TO_FRAME': (TO_FRAME,),
                  'LIGHT_PLACE': LIGHT['place'], **{f'LIGHT_{name.upper()}': (LIGHT[name],) for name in (
                      'intensity', 'specular', 'diffuse', 'ambient', 'alpha')},
                  **{f'LIGHT_{name}': (value,) for name, value in zip('ARGB', LIGHT['color'])}}
        driver = LINK_DRIVER
        for name, values in floats.items():
            driver = driver.replace(f'@{name}@', ', '.join(f'{value}F' for value in values))
        for name, keys in (('FROM_PLACES', FROM_PLACES), ('FROM_ANGLES', FROM_ANGLES), ('TO_PLACES', TO_PLACES),
                           ('TO_ANGLES', TO_ANGLES)):
            driver = driver.replace(f'@{name}@', keys_c(keys))
        *links, (_, *light) = ninja_prints(self, driver, LINK_BODIES, LINK_C)
        said = {float(rate): [float(word) for word in words] for _, rate, *words in links}
        # each motion's pose at its own frame: a place, and the angle about z, cut to a whole one
        ends = [(keyed(places, frame), int(keyed(angles, frame)[2]))
                for places, angles, frame in ((FROM_PLACES, FROM_ANGLES, FROM_FRAME), (TO_PLACES, TO_ANGLES, TO_FRAME))]
        for count, rate in enumerate(RATES, 1):
            with self.subTest(rate=rate):
                posed, *stood = said[rate]
                # the node stands the rate's share of the way from the first motion's pose to the second's: its
                # matrix holds the place in its last four, and the turn about z in its first two
                place, angle = ([a * (1 - rate) + b * rate for a, b in zip(*(end[0] for end in ends))],
                                math.radians(int(ends[0][1] * (1 - rate) + ends[1][1] * rate) * 360 / 0x10000))
                self.assertEqual(posed, count)
                self.near(stood[12:15], place, 1e-3)
                self.near([stood[0], abs(stood[1])], [math.cos(angle), abs(math.sin(angle))], 1e-3)
        # a rate past 1 poses nothing, so the last node's matrix stands
        self.assertEqual(said[OVER], said[RATES[-1]])
        # the alpha joins the light's color, which the intensity scales, and each share scales that. the place
        # goes to the light and to Ninja's copy
        color = [LIGHT['alpha'], *LIGHT['color'][1:]]
        scaled = [value * LIGHT['intensity'] for value in color]
        self.near([float(word) for word in light],
                  [*color, *scaled, *(value * LIGHT[share] for share in ('specular', 'diffuse', 'ambient')
                                      for value in scaled), *LIGHT['place'], *LIGHT['place']], 1e-6)


# symbols.c with a main after it, which reaches its reader of a file: each
# word a path to read, own for the program's own read, twice, an address,
# for the name whose bytes hold it, or an object's name. driver_object is an
# object the program's own table names
SYMBOLS_DRIVER = r"""
#include <stdio.h>
#include <stdlib.h>
#include "symbols.c"
int driver_object = 1;
int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (strchr(argv[i], '/')) {
            printf("read %d\n", read_file(argv[i]));
        } else if (!strcmp(argv[i], "own")) {
            int first = native_read_symbols();
            printf("own %d %d\n", first, native_read_symbols());
        } else if (!strncmp(argv[i], "0x", 2)) {
            const struct symbol *at =
                names_symbol_at((uint32_t)strtoul(argv[i], NULL, 16));
            printf("%s %s\n", argv[i], at ? at->name : "none");
        } else {
            printf("%s %#lx\n", argv[i], (unsigned long)names_symbol(argv[i]));
        }
    }
    return 0;
}
"""

# what symbols.c reads of elf.h and link.h, for a host whose C library has
# neither, as macOS's has: a 64-bit file's types, which elf_file writes
ELF_H = """
#include <stdint.h>
typedef struct {
    unsigned char e_ident[16];
    uint16_t e_type, e_machine;
    uint32_t e_version;
    uint64_t e_entry, e_phoff, e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
} Elf64_Ehdr;
typedef struct {
    uint32_t sh_name, sh_type;
    uint64_t sh_flags, sh_addr, sh_offset, sh_size;
    uint32_t sh_link, sh_info;
    uint64_t sh_addralign, sh_entsize;
} Elf64_Shdr;
typedef struct {
    uint32_t st_name;
    unsigned char st_info, st_other;
    uint16_t st_shndx;
    uint64_t st_value, st_size;
} Elf64_Sym;
#define SHT_SYMTAB 2
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_FILE 4
"""
LINK_H = '#define ElfW(type) Elf64_##type\n'
OBJECT, FUNC, FILE = 1, 2, 4
SECTION, SYMBOL = '<IIQQQQIIQQ', '<IBBHQQ'
# where elf_file's bytes hold the header's e_shoff and e_shnum, its three
# section headers, the size in its table's header, and the offset and the
# size in its names' header
E_SHOFF, E_SHNUM, HEADERS, TABLE_SIZE, NAMES_OFFSET, NAMES_SIZE = 40, 60, slice(64, 256), 160, 216, 224


def elf_file(symbols, table=True):
    """a 64-bit ELF file: its header, its section headers, its symbol table
    of symbols, each a name, a type, an address and a size, and their names
    last. a name of None lies past the names. with no table, it holds the
    null section alone, as a stripped file's headers name no table"""
    names, rows = b'\0', [struct.pack(SYMBOL, 0, 0, 0, 0, 0, 0)]
    for name, kind, address, size in symbols:
        rows.append(struct.pack(SYMBOL, 1 << 20 if name is None else len(names), kind, 0, 1, address, size))
        names += b'' if name is None else name.encode() + b'\0'
    sections = [struct.pack(SECTION, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)]
    if table:
        at = 64 + 3 * 64
        sections.append(struct.pack(SECTION, 0, 2, 0, 0, at, 24 * len(rows), 2, len(rows), 8, 24))
        sections.append(struct.pack(SECTION, 0, 3, 0, 0, at + 24 * len(rows), len(names), 0, 0, 1, 0))
    head = struct.pack('<16sHHIQQQIHHHHHH', b'\x7fELF\2\1\1', 2, 62, 1, 0, 0, 64, 0, 64, 0, 0, 64, len(sections), 0)
    return head + b''.join(sections) + (b''.join(rows) + names if table else b'')


def patched(whole, fields, more=b''):
    """whole and more after it, with each of fields, a value by its offset and its struct format, written over it"""
    out = bytearray(whole + more)
    for (at, form), value in fields.items():
        struct.pack_into(form, out, at, value)
    return bytes(out)


class Symbols(unittest.TestCase):
    # a function ahead of the object of its name, and one with no object of its name
    NAMED = [('other.c', FILE, 0, 0), ('task_levels', OBJECT, 0x601100, 4), ('calc', FUNC, 0x401100, 8),
             ('task.c', FILE, 0, 0), ('task_levels', OBJECT, 0x601200, 4), ('calc', OBJECT, 0x601000, 64),
             (None, OBJECT, 0x601300, 4), ('step', FUNC, 0x401000, 16)]
    # read_file's answers: the names read, a file that can't be read, and one that holds no symbol table
    READ, UNREAD, BARE = 'read 0', 'read -1', 'read 1'

    @classmethod
    def setUpClass(cls):
        if struct.calcsize('P') != 8 or sys.byteorder != 'little':
            raise unittest.SkipTest('elf_file writes a 64-bit little-endian host\'s file')
        tools = ROOT / 'engine/native'
        cls.built = tempfile.TemporaryDirectory()
        built = Path(cls.built.name)
        (built / 'driver.c').write_text(SYMBOLS_DRIVER)
        compiler = os.environ.get('CC') or shutil.which('clang') or 'cc'
        includes = [f'-I{tools}', f'-I{ROOT / "engine/sa1"}', f'-I{ROOT / "engine/include"}']
        if subprocess.run([compiler, '-fsyntax-only', '-x', 'c', '-'], input='#include <elf.h>\n#include <link.h>\n',
                          capture_output=True, text=True).returncode:
            (built / 'host').mkdir()
            (built / 'host/elf.h').write_text(ELF_H)
            (built / 'host/link.h').write_text(LINK_H)
            includes.append(f'-I{built / "host"}')
        cls.tool = built / 'symbols'
        subprocess.run([compiler, '-std=gnu17', '-O1', *includes, '-o', cls.tool, built / 'driver.c',
                        tools / 'names.c'], check=True)
        cls.kept, cls.stripped = built / 'kept', built / 'stripped'
        cls.kept.write_bytes(elf_file(cls.NAMED))
        cls.stripped.write_bytes(elf_file(cls.NAMED, table=False))

    @classmethod
    def tearDownClass(cls):
        cls.built.cleanup()

    def run_symbols(self, *args):
        return subprocess.run([self.tool, *args], check=True, capture_output=True, text=True).stdout.splitlines()

    def test_a_table_gives_each_object_by_name_task_cs_statics_first_and_each_function_by_address(self):
        self.assertEqual(self.run_symbols(self.kept, 'task_levels', 'other.c', 'absent', '0x601010', '0x401008'),
                         [self.READ, 'task_levels 0x601200', 'other.c 0', 'absent 0', '0x601010 calc', '0x401008 step'])

    def test_a_name_gives_its_object_and_no_function_of_that_name(self):
        # the adapter reads what a name gives as the game's data, so a function that shares an object's name, or
        # has a name no object has, isn't what the name finds
        self.assertEqual(self.run_symbols(self.kept, 'calc', '0x401100', 'step'),
                         [self.READ, 'calc 0x601000', '0x401100 calc', 'step 0'])

    def test_a_file_without_a_symbol_table_is_refused_each_time_and_a_table_after_it_is_read(self):
        self.assertEqual(self.run_symbols(self.stripped, 'calc', self.stripped, 'calc', self.kept, 'calc'),
                         [self.BARE, 'calc 0', self.BARE, 'calc 0', self.READ, 'calc 0x601000'])

    def test_a_file_that_cant_be_read_is_told_from_one_that_holds_no_table(self):
        # the run's words for the two differ: only the second is a program strip has been at
        missing = Path(self.built.name) / 'missing'
        self.assertEqual(self.run_symbols(missing, missing, 'calc', self.stripped),
                         [self.UNREAD, self.UNREAD, 'calc 0', self.BARE])

    def test_a_refused_file_leaves_the_names_as_they_were(self):
        self.assertEqual(self.run_symbols(self.kept, self.stripped, 'calc', '0x401008'),
                         [self.READ, self.BARE, 'calc 0x601000', '0x401008 step'])

    def test_a_programs_own_read_answers_alike_each_time_and_is_refused_once_stripped(self):
        said = self.run_symbols('own', 'driver_object')
        if self.tool.read_bytes()[:4] != b'\x7fELF' or not os.path.exists('/proc/self/exe'):
            # no file of the program's own to read: the second read fails as the first did
            self.assertEqual(said, ['own -1 -1', 'driver_object 0'])
            return
        self.assertEqual(said[0], 'own 0 0')
        self.assertNotEqual(said[1], 'driver_object 0')
        if shutil.which('strip'):
            stripped = Path(self.built.name) / 'own-stripped'
            subprocess.run(['strip', '-o', stripped, self.tool], check=True)
            self.assertEqual(subprocess.run([stripped, 'own', 'driver_object'], check=True, capture_output=True,
                                            text=True).stdout.splitlines(), ['own 1 1', 'driver_object 0'])

    def test_a_file_that_ends_short_of_its_headers_its_table_or_its_names_is_refused(self):
        whole = self.kept.read_bytes()
        names = sum(len(name) + 1 for name, *_ in self.NAMED if name is not None) + 1
        cut = Path(self.built.name) / 'cut'
        for size in 0, 10, 64 + 3 * 64 - 1, len(whole) - names - 1, len(whole) - 1:
            with self.subTest(size=size):
                cut.write_bytes(whole[:size])
                self.assertEqual(self.run_symbols(cut, 'calc'), [self.BARE, 'calc 0'])
        cut.write_bytes(whole)
        self.assertEqual(self.run_symbols(cut, 'calc'), [self.READ, 'calc 0x601000'])
        # the headers last, as a linker leaves them, at a multiple of 8, where the table and its names lie whole
        # ahead of them: read with every header, and refused with the last one cut by the eight bytes of zeros it
        # ends in, which a read past the file's end would take from the zeros its last page ends in
        headers = whole[HEADERS]
        self.assertEqual(headers[-8:], bytes(8))
        moved = patched(whole, {(E_SHOFF, '<Q'): len(whole) + -len(whole) % 8}, bytes(-len(whole) % 8) + headers)
        cut.write_bytes(moved)
        self.assertEqual(self.run_symbols(cut, 'calc'), [self.READ, 'calc 0x601000'])
        cut.write_bytes(moved[:-8])
        self.assertEqual(self.run_symbols(cut, 'calc'), [self.BARE, 'calc 0'])

    def test_a_table_that_leaves_the_file_or_whose_names_are_no_section_of_it_or_end_in_no_nul_is_refused(self):
        whole = self.kept.read_bytes()
        offset, size = struct.unpack_from('<QQ', whole, NAMES_OFFSET)
        page = os.sysconf('SC_PAGE_SIZE')
        fill = b'x' * (-len(whole) % page)
        damaged = {
            # the names lie in the file, and the table ahead of them runs a byte past its end
            'a table that runs past the file': patched(whole, {(TABLE_SIZE, '<Q'): len(whole) - 256 + 1}),
            # the table names section 2 for its names, where the file counts two sections: the header that
            # follows them is no section's
            'names past the count of sections': patched(whole, {(E_SHNUM, '<H'): 2}),
            # the last name runs to the end of the file, which is the end of its last page, with no NUL: a read
            # of the name to its NUL leaves the file
            'names that end the file with no NUL': patched(whole[:-1], {(NAMES_SIZE, '<Q'): size + len(fill)},
                                                           b'x' + fill),
            'names one byte short of their NUL': patched(whole, {(NAMES_SIZE, '<Q'): size - 1}),
            'names of no bytes': patched(whole, {(NAMES_OFFSET, '<Q'): 0, (NAMES_SIZE, '<Q'): 0}),
        }
        self.assertEqual((offset + size, len(damaged['names that end the file with no NUL']) % page), (len(whole), 0))
        file = Path(self.built.name) / 'damaged'
        for what, content in damaged.items():
            with self.subTest(what):
                file.write_bytes(content)
                self.assertEqual(self.run_symbols(file, 'calc', '0x401008'), [self.BARE, 'calc 0', '0x401008 none'])


# what stands ahead of the runner's end_run, syBtExit and native_vsync, and a
# main after them, which runs three steps of a game, a vsync after each: the
# first word is the game's loader_error_flag, the second the run's FRAMES,
# and the third who leaves, step for the game's step, as the logo's does at
# its quit chord, vsync for its vsync function, as loader_poll_drive does, or
# nobody. the run's report, the hooks' end and play.c's close each print a
# line
EXIT_DRIVER = r"""
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
int loader_error_flag;
static unsigned long frames_wanted;
unsigned long native_vsyncs;
static void (*vsync_function)(void);
void (*native_pace)(void);
void native_report(const char *why)
{
    printf("%s after %lu vsyncs\n", why, native_vsyncs);
}
void native_hook_run_end(void)
{
    puts("the hooks' end");
}
void native_play_close(void)
{
    puts("closed");
}
"""
EXIT_MAIN = r"""
/* the game's exit, and what its caller does next */
static void leaves(void)
{
    puts("leaving");
    syBtExit();
    puts("ran on");
}
int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    loader_error_flag = atoi(argv[1]);
    frames_wanted = strtoul(argv[2], NULL, 0);
    if (!strcmp(argv[3], "vsync")) {
        vsync_function = leaves;
    }
    for (int step = 0; step < 3; step++) {
        if (!strcmp(argv[3], "step")) {
            leaves();
        }
        puts("stepped");
        native_vsync();
    }
    puts("main returned");
    return 1;
}
"""


class Exit(unittest.TestCase):
    def test_the_games_exit_ends_the_run_inside_its_call_and_closes_play_once(self):
        # runner.c's end_run, syBtExit and native_vsync, as they stand, under a game whose step or whose vsync
        # function calls syBtExit. The call is the run's end: the report, the hooks' end and play.c's close come
        # once each, in that order, and neither the caller's next line nor a later step runs. An exit the player
        # asks for ends with 0, and one under the game's flag of a disc error ends with 2. An exit in the vsync
        # that would end the run's FRAMES closes once too, and a run nobody leaves ends at its FRAMES with 0
        runner = (ROOT / 'engine/native/runner.c').read_text()
        bodies = [re.search(rf'(?ms)^(?:static )?void {name}\([^;{{]*\)\n{{.*?^}}\n', runner)
                  for name in ('end_run', 'syBtExit', 'native_vsync')]
        self.assertNotIn(None, bodies, 'runner.c defines end_run, syBtExit and native_vsync')
        end = "the hooks' end\nclosed\n"
        left, failed = 'the game left', 'the game left at a disc error'
        compiler = os.environ.get('CC') or shutil.which('clang') or 'cc'
        with tempfile.TemporaryDirectory() as built:
            source, tool = Path(built, 'driver.c'), Path(built, 'exit')
            source.write_text(EXIT_DRIVER + ''.join(body[0] for body in bodies) + EXIT_MAIN)
            subprocess.run([compiler, '-std=gnu17', '-O1', '-w', '-o', tool, source], check=True)
            for words, status, said in (
                    (('0', '0', 'step'), 0, f'leaving\n{left} after 0 vsyncs\n{end}'),
                    (('1', '0', 'step'), 2, f'leaving\n{failed} after 0 vsyncs\n{end}'),
                    (('0', '0', 'vsync'), 0, f'stepped\nleaving\n{left} after 1 vsyncs\n{end}'),
                    (('1', '1', 'vsync'), 2, f'stepped\nleaving\n{failed} after 1 vsyncs\n{end}'),
                    (('0', '2', 'nobody'), 0, f'stepped\nstepped\ndone after 2 vsyncs\n{end}')):
                with self.subTest(words=words):
                    ran = subprocess.run([tool, *words], capture_output=True, text=True)
                    self.assertEqual((ran.returncode, ran.stdout, ran.stderr), (status, said, ''))
        # the game's C and the adapter link to that definition: the table of stubs allows no stub for the name
        self.assertNotIn('syBtExit', {row.split()[0] for row in
                                      (ROOT / 'engine/native/stubs.txt').read_text().splitlines() if row.strip()})


# names.c and loads.c over an allocator that grants a count of blocks and
# then none, and a main after them: the first word is names, which adds a
# name more than the list's first room holds, or programs, which keeps two
# programs' data and reads the first's back. the second is the count of
# blocks granted, or -1 for every block
MEMORY_DRIVER = r"""
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "native.h"

static int left;
static int granted(void)
{
    return left < 0 || (left > 0 && left--);
}
static void *refused_realloc(void *block, size_t size)
{
    return granted() ? realloc(block, size) : NULL;
}
static void *refused_malloc(size_t size)
{
    return granted() ? malloc(size) : NULL;
}
static void *refused_calloc(size_t count, size_t size)
{
    return granted() ? calloc(count, size) : NULL;
}
#define realloc refused_realloc
#define malloc refused_malloc
#define calloc refused_calloc
#include "names.c"
#include "loads.c"

static char first[8] = "first", second[8] = "second", bss[4];
const struct native_program native_programs[] = {
    {"FIRST.BIN", first, first + sizeof first, bss, bss + sizeof bss},
    {"SECOND.BIN", second, second + sizeof second, bss, bss},
};
const int native_program_count = 2;

int main(int argc, char **argv)
{
    left = atoi(argv[2]);
    if (!strcmp(argv[1], "names")) {
        for (uint32_t i = 0; i <= 1 << 16; i++) {
            names_symbol_add(0x1000 + 4 * i, 4, 0, "name");
        }
        names_symbols_ready();
        printf("%u names, the last at %#x\n", symbol_count, names_symbol_at(0x1000 + (4 << 16))->address);
    } else {
        native_programs_keep();
        first[0] = 'F';
        native_program_read("first.bin");
        puts(first);
    }
    return 0;
}
"""


class Memory(unittest.TestCase):
    def test_a_start_the_host_gives_no_memory_says_what_for_and_ends(self):
        # names.c and loads.c, as they stand. The list of symbols takes its room at the first name, and twice the
        # room at the name past 65536 of them. The programs' data takes a table, and then a block for each program
        # that has data. A start the host refuses any of those says what the memory is for, in native.h's one
        # line, and ends with 2, where a write through the null it got would fault
        tools = ROOT / 'engine/native'
        symbols, programs = "the program's symbols", 'the data the programs started with'
        compiler = os.environ.get('CC') or shutil.which('clang') or 'cc'
        with tempfile.TemporaryDirectory() as built:
            source, tool = Path(built, 'driver.c'), Path(built, 'memory')
            source.write_text(MEMORY_DRIVER)
            subprocess.run([compiler, '-std=gnu17', '-O1', '-w', f'-I{tools}', f'-I{ROOT / "engine/sa1"}',
                            f'-I{ROOT / "engine/include"}', '-o', tool, source], check=True)
            for words, said, refused in ((('names', '-1'), '65537 names, the last at 0x41000\n', None),
                                         (('names', '0'), '', symbols), (('names', '1'), '', symbols),
                                         (('programs', '-1'), 'first\n', None),
                                         (('programs', '0'), '', programs), (('programs', '1'), '', programs)):
                with self.subTest(words=words):
                    ran = subprocess.run([tool, *words], capture_output=True, text=True)
                    self.assertEqual((ran.returncode, ran.stdout, ran.stderr),
                                     (0, said, '') if refused is None else
                                     (2, '', f'sa1: no memory for {refused}\n'))


# what stands ahead of the runner's refuse and find_statics, and a main after
# them: the first word is whether the run opens the engine, the read's answer
# the second, the name the adapter lacks the third, where there is one, and
# the window's box prints what it shows
REFUSAL_DRIVER = r"""
#include <stdio.h>
#include <stdlib.h>
static int engine_opens;
static int unread;
static const char *lacking;
int native_read_symbols(void)
{
    return unread;
}
const char *sa1_statics_find(void)
{
    return lacking;
}
void native_play_alert(const char *words)
{
    printf("box: %s\n", words);
}
"""
REFUSAL_MAIN = r"""
int main(int argc, char **argv)
{
    engine_opens = atoi(argv[1]);
    unread = atoi(argv[2]);
    lacking = argc > 3 ? argv[3] : NULL;
    find_statics();
    puts("found");
    return 0;
}
"""


class Statics(unittest.TestCase):
    def test_the_runner_ends_a_run_whose_program_cant_be_read_or_lacks_its_table_or_a_name_and_says_which(self):
        # runner.c's refuse and find_statics, as they stand, between a read and an adapter that answer as the test
        # says: a run goes on only where the symbols are read and the adapter lacks no name. Any other ends with 2
        # once it has said which of the three it is, to stderr, and in the window's box where the run opens the
        # engine: a run that opens none has no window, and a box would wait for a click nobody is there to give.
        # make native holds a copy of play without its table, and one without a name, to words of the two sentences
        # for them
        runner = (ROOT / 'engine/native/runner.c').read_text()
        bodies = [re.search(rf'(?ms)^static void {name}\(.*?^}}\n', runner)[0] for name in ('refuse', 'find_statics')]
        keep = 'the program must keep the symbol table it was linked with, which strip removes'
        unreadable = ("can't read this program's own file, /proc/self/exe, where it finds the game's statics by name "
                      'in its symbol table')
        bare = f"this program's own file holds no symbol table, where it finds the game's statics by name: {keep}"
        nameless = ("this program's symbol table holds no object named iamb, a static of the game's it finds by "
                    f'name: {keep}')
        compiler = os.environ.get('CC') or shutil.which('clang') or 'cc'
        with tempfile.TemporaryDirectory() as built:
            source, tool = Path(built, 'driver.c'), Path(built, 'refusal')
            source.write_text(REFUSAL_DRIVER + ''.join(bodies) + REFUSAL_MAIN)
            subprocess.run([compiler, '-std=gnu17', '-O1', '-w', '-o', tool, source], check=True)
            for words, refusal in ((('0',), None), (('-1',), unreadable), (('1',), bare), (('0', 'iamb'), nameless),
                                   (('-1', 'iamb'), unreadable), (('1', 'iamb'), bare)):
                for opens, box in (('1', f'box: {refusal}\n'), ('0', '')):
                    with self.subTest(words=words, opens=opens):
                        ran = subprocess.run([tool, opens, *words], capture_output=True, text=True)
                        self.assertEqual((ran.returncode, ran.stdout, ran.stderr),
                                         (0, 'found\n', '') if refusal is None else
                                         (2, box, f'sa1: {refusal}\n'))
        rules = (ROOT / 'engine/mk/native.mk').read_text()
        for copy, sentence in (('stripped', bare), ('lacking', nameless)):
            self.assertIn(re.search(rf'(?m)^NATIVE_SAYS_{copy} +:= (.+)$', rules)[1], sentence)

    def test_a_static_is_found_by_name_as_a_run_starts_or_where_the_shim_checks_what_it_found(self):
        # symbols.c defines the lookup. the adapter's one use is sa1_statics_find, which finds its statics all at
        # once, and the shim tests the event head it asks for, so no other read meets a name the program lacks
        reads = {path.name: len(re.findall(r'\bnative_symbol\(', path.read_text()))
                 for directory in ('engine/native', 'engine/sa1') for path in sorted((ROOT / directory).glob('*.c'))}
        self.assertEqual({name: count for name, count in reads.items() if count},
                         {'records.c': 1, 'shim.c': 1, 'symbols.c': 1})
        records = (ROOT / 'engine/sa1/records.c').read_text()
        find = records[records.index('const char *sa1_statics_find(void)'):]
        self.assertIn('native_symbol(', find[:find.index('\n}\n')])
        # the runner finds them, or ends the run, before the game's main starts
        runner = (ROOT / 'engine/native/runner.c').read_text()
        main = runner[runner.index('int main(int argc, char **argv)'):]
        self.assertLess(main.index('find_statics();'), main.index('native_run_main('))


if __name__ == '__main__':
    unittest.main()
