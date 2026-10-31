import contextlib
import errno
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
from tables import hexa

ROOT = Path(__file__).resolve().parents[1]
# the toolchain the fixture compiles and links with: the directory SH_ELF
# names, as in make gcc, or else tools/sh-elf. GIVEN says whether SH_ELF
# names one. A tool of TOOLS missing from SH_ELF's directory fails the
# fixture, and one missing from tools/sh-elf skips it
GIVEN = 'SH_ELF' in os.environ
TOOLCHAIN = Path(os.environ['SH_ELF']).resolve() if GIVEN else ROOT / 'tools/sh-elf'
TOOLS = ('bin/sh-elf-gcc', 'bin/sh-elf-as', 'bin/sh-elf-ar', 'bin/sh-elf-ld')
# a makefile, read from make's standard input, that prints a variable as config/gcc.mk gives it to a target. It takes
# the Makefile's line that defines quoted, a function config/gcc.mk calls. The fixture compiles with GCCFLAGS, as make
# gcc does
PROBE = (next(line for line in (ROOT / 'Makefile').read_text().splitlines() if line.startswith('quoted = '))
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
}
# ova.c with a constant ahead of its data, which moves ova_table on: .rodata lies ahead of .data
SOURCES['ova_moved.c'] = 'const int ova_before[4] = {9, 9, 9, 9};\n' + SOURCES['ova.c']
# C whose tables hold layouts as the checkout's headers declare them. zam_layout holds the size, alignment and
# members' offsets of each SH4ZAM type the game's C takes. ninja_layout holds NJS_VIEW's and NJS_LIGHT's size and first
# and last members' offsets, and ninja_alignment their alignments. With GCC undefined, sg_xpt.h and njtypes.h give
# SHC's text, whose size_t is another type than the one SH4ZAM's headers declare. The two can't compile together, so
# zam_layout is GCC's alone
LAYOUT = '''\
#include <sg_xpt.h>
#include <njtypes.h>

#define AT(type, member) __builtin_offsetof(type, member)
#define LAID(type, ...) sizeof(type), _Alignof(type), __VA_ARGS__

#ifdef GCC
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


def flags(variable='GCCFLAGS', target='flags'):
    """The words of variable, GCCFLAGS by default, as config/gcc.mk gives it to target. They are split as the shell
    splits them in make gcc's recipes. A make that runs the tests passes none of its flags on. config/gcc.mk gives the
    default target no value of its own."""
    env = {name: value for name, value in os.environ.items() if name not in ('MAKEFLAGS', 'MFLAGS', 'MAKELEVEL')}
    done = subprocess.run(['make', '-s', '-f', '-', target], cwd=ROOT, env=env,
                          input=PROBE.format(variable=variable, target=target), stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, text=True)
    if done.returncode or done.stderr:
        raise AssertionError(done.stderr)
    return shlex.split(done.stdout)


def run(*argv, root):
    """Run argv in the directory root. Its first word names a tool of the toolchain, as bin/sh-elf-gcc does. It fails
    with what the tool printed where the tool exits nonzero or prints anything, as make gcc fails on a warning."""
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
        missing = [tool for tool in TOOLS if not (TOOLCHAIN / tool).is_file()]
        if missing and not GIVEN:
            raise unittest.SkipTest(f'{TOOLCHAIN} lacks {", ".join(missing)}, which make gcc builds')
        if missing:
            raise AssertionError(f'{TOOLCHAIN}, which SH_ELF names, lacks {", ".join(missing)}')
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

    def setUp(self):
        # each test links under a directory named for it, in root, where the spec's inputs lie
        self.out = Path(self.id().rsplit('.', 1)[1])
        self.addCleanup(os.chdir, os.getcwd())
        os.chdir(self.root)

    def layouts(self, images, roots=None):
        """Each image's layout pass, by image. roots gives each image's roots, as rooted() does by default."""
        roots = roots or gcclink.rooted(images)
        return {name: ld.link(image, roots[name], ld.provisional(images, image), self.out, 'layout')
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
        # keeps the layout SHC's text gives it. Each set of flags defines SHZ_TLS_MODEL. Without it, SH4ZAM's headers
        # include threads.h, which the SDK lacks
        if not (ROOT / 'tools/sh4zam/fetched.txt').is_file():
            self.skipTest('tools/sh4zam lacks the fetch make gcc makes')

        def laid(compiled):
            run('bin/sh-elf-gcc', *compiled, '-Isdk/include', '-c', '-o', str(self.root / 'layout.o'),
                str(self.root / 'layout.c'), root=ROOT)
            found = elf.read((self.root / 'layout.o').read_bytes(), 'layout.o')
            return {symbol.name[1:]: struct.unpack_from(f'<{symbol.size // 4}I', found.sections[symbol.section].data,
                                                        symbol.value)
                    for symbol in found.symbols if symbol.name in ('_zam_layout', '_ninja_layout', '_ninja_alignment')}

        (self.root / 'layout.c').write_text(LAYOUT)
        compiled = flags()
        game, shc, library = laid(compiled), laid([*compiled, '-UGCC']), laid(flags('GCC_SH4ZAM_FLAGS'))
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
        images = {**linking(self.images, 'ova', 'ova.o', 'record.o'),
                  'ova': linking(self.images, 'ova', 'ova.o', 'record.o')['ova']._replace(
                      heads=[gcclink.Piece('DHEAD', 'record.o', '.data')])}
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
        kernel = [gcclink.Piece(None, 'ova.o', '.text.ova_probe'), gcclink.Piece(None, 'head.o', '.text.head')]
        _, layouts, _ = ld.coordinate({**images, 'ova': images['ova']._replace(kernel=kernel)}, self.out)
        placed = {output: [(placed.file, placed.section) for placed in layouts['ova'].placements
                           if placed.output == output and placed.size] for output in ('.text', '.data')}
        self.assertEqual(placed['.text'], [('ova.o', '.text.ova_probe'), ('head.o', '.text.head'),
                                           ('ova.o', '.text.step'), ('ova.o', '.text.ova_unused')])
        self.assertEqual(placed['.data'], [('more.o', '.data'), ('ova.o', '.data')])

    def test_a_kernel_past_the_instruction_cache_s_lines_fails_the_link(self):
        # ova's code starts at its origin, on a line's start, so whole.o's 8 KB take each line once, and spill.o's
        # two bytes one more
        images = linking(self.images, 'ova', 'ova.o', 'whole.o', 'spill.o')
        kernel = [gcclink.Piece(None, 'whole.o', '.text.whole')]
        ld.coordinate({**images, 'ova': images['ova']._replace(kernel=kernel)}, self.out)
        kernel.append(gcclink.Piece(None, 'spill.o', '.text.spill'))
        with self.assertRaisesRegex(SystemExit, "ova: its kernel touches more than the 256 lines of the SH-4's "
                                                'instruction cache\n'):
            self.layouts({**images, 'ova': images['ova']._replace(kernel=kernel)})

    def test_a_kernel_row_that_places_no_code_or_part_of_an_object_s_split_code_fails_the_link(self):
        rows = {'no section': (gcclink.Piece(None, 'ova.o', '.text.absent'),
                               "ova: its kernel row places ova.o's .text.absent, and that object holds no such section"),
                'data': (gcclink.Piece(None, 'more.o', '.data'),
                         "ova: its kernel row places more.o's .data, which is no code"),
                # mixed.o's P branches to its C, which stays where the object lies
                'split': (gcclink.Piece(None, 'mixed.o', 'P'),
                          "ova: its kernel takes some of mixed.o's code, which is SHC's sections and GCC's both, and "
                          'leaves .text.mixed_twice')}
        for case, (piece, problem) in rows.items():
            with self.subTest(case):
                images = linking(self.images, 'ova', 'ova.o', 'more.o', 'mixed.o')
                images = {**images, 'ova': images['ova']._replace(kernel=[piece])}
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
                linking = subprocess.Popen([sys.executable, str(ROOT / 'tools/ld.py'), 'link.txt', str(self.out)],
                                           env=dict(os.environ, SH_ELF=str(linker.parents[1])),
                                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                while not held.exists() and linking.poll() is None:
                    time.sleep(0.01)
                linking.send_signal(number)
                self.assertEqual(linking.wait(10), status)
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


if __name__ == '__main__':
    unittest.main()
