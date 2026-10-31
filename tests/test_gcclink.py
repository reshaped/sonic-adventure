import re
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import elf
import gcclink

# STB_GNU_UNIQUE, a binding of GNU's own, neither global nor weak
GNU_UNIQUE = 10
# a whole map of ld's, with an input section in .head, then in .text an
# empty one, one whose name runs onto the next line, and an archive member's
MAP = '''Archive member included to satisfy reference by file (symbol)

libgcc.a(_movmem_i4.o)        a.o (__movmem_i4_even)

Linker script and memory map

                0x8c8845c4                        __stack_size_ = 0x8c8845c4

.head           0x0cb80000        0x8
                0x0cb80000                        __startof_DHEAD = .
 a.o(.data)
 .data          0x0cb80000        0x8 a.o

.text           0x0cb80020       0x80
 *(.text .text.*)
 .text          0x0cb80020        0x0 a.o
 .text.event_chaos4_init
                0x0cb80020       0x1c a.o
                0x0cb80020                event_chaos4_init
 *fill*         0x0cb8003c        0x4
 .text          0x0cb80040       0x60 libgcc.a(_movmem_i4.o)
LOAD a.o
LOAD libgcc.a
OUTPUT(final.elf elf32-shl)
'''


class Read(unittest.TestCase):
    def test_refuses_a_row_naming_its_line(self):
        # each spec opens with two images, and its last row is at fault
        images = 'image  main  0x8c010000  0x00010000  start\nimage  ova  0x0c900000  0x00010000\n'
        cases = {
            'an image named in capitals': ('image  Main  0x8c020000  0x00010000', 'not a row: '),
            'an input row of two paths': ('input  main  a.o  b.o', 'not a row: '),
            'an input row whose PATH is a pattern': ('input  main  a*.o', r'a\*.o: not a PATH: '),
            'an input row whose PATH is an option': ('input  main  -a.o', '-a.o: not a PATH: '),
            "a tail row whose PATH ends the script's string": ('tail  main  DEND  a".o  .data', 'not a row: '),
            'an import whose ALIAS is no C name': ('import  main  1x  ova  step  0x8c', 'not a row: '),
            'an alias whose ALIAS is no C name': ('alias  main  1x  start', 'not a row: '),
            'a constant whose SYMBOL is no C name': ('constant  main  1x  0x8c0f0000', 'not a row: '),
            "a head row whose SECTION isn't an SHC section's name": ('head  main  dhead  a.o  .data', 'not a row: '),
            'a decimal OFFSET with a leading zero': ('anchor  main  start  08', 'not a row: '),
            'a second image row': ('image  main  0x8c020000  0x00010000', 'main has an image row already'),
            'a name bound twice': ('import  main  step  ova  step  0x8c\nimport  main  step  ova  step  0x0c',
                                   'main binds step already'),
            "a constant bound to the image's end": ('constant  main  image_end  0x8c0f0000',
                                                    'image_end is a boundary of the link'),
            "an alias of a run's start": ('alias  main  _startof_P  start', '_startof_P is a boundary of the link'),
            'an import from its own image': ('import  ova  start  ova  step  0x8c',
                                             'ova: not another image of the spec'),
            'an import that names no provider': ('import  main  step  -  step  0x8c',
                                                 '-: not another image of the spec'),
            'an import with an addend': ('import  main  step  ova  step  0x8c  4', 'not a row: '),
            'an import in neither view': ('import  main  step  ova  step  0x0d', 'not a row: '),
            'an image whose ENTRY is no C name': ('image  solo  0x8c020000  0x00010000  1x', 'not a row: '),
        }
        with tempfile.TemporaryDirectory() as root:
            path = Path(root, 'link.txt')
            for case, (rows, problem) in cases.items():
                with self.subTest(case):
                    path.write_text(f'{images}{rows}\n')
                    line = f'{images}{rows}'.count('\n') + 1
                    with self.assertRaisesRegex(SystemExit, f'{re.escape(str(path))}:{line}: {problem}'):
                        gcclink.read(path)


class Exports(unittest.TestCase):
    def test_exports_a_global_or_weak_name_alone(self):
        # a datum of each binding in .data, GNU_UNIQUE among them
        data = elf.Section(1, '.data', 1, elf.ALLOC, 0x8C010000, 16, 4, bytes(16))
        symbols = [elf.Symbol(f'_{name}', 0x8C010000, 4, 1, 1, bind) for name, bind in
                   (('local', elf.LOCAL), ('global', elf.GLOBAL), ('weak', elf.WEAK), ('unique', GNU_UNIQUE))]
        self.assertEqual(list(gcclink.exports(elf.Elf('a.o', [data], symbols, {}))), ['global', 'weak'])


class Libgcc(unittest.TestCase):
    def test_takes_the_one_libgcc_a_the_toolchain_installs(self):
        with tempfile.TemporaryDirectory() as root:
            installed = Path(root, 'lib/gcc/sh-elf')

            def install(release):
                (installed / release).mkdir(parents=True)
                (installed / release / 'libgcc.a').write_bytes(b'')

            with mock.patch.object(gcclink, 'SH_ELF', Path(root)):
                with self.assertRaisesRegex(SystemExit, r'lib/gcc/sh-elf/: no libgcc\.a, which the toolchain '
                                                        r'installs\Z'):
                    gcclink.libgcc()
                install('16.2.0')
                self.assertEqual(gcclink.libgcc(), installed / '16.2.0/libgcc.a')
                install('15.2.0')
                with self.assertRaisesRegex(SystemExit, r'lib/gcc/sh-elf/: holds 2 libgcc\.a, \S*/15\.2\.0/libgcc\.a, '
                                                        r"\S*/16\.2\.0/libgcc\.a, and make gcc's toolchain installs "
                                                        r'one\Z'):
                    gcclink.libgcc()


class Placements(unittest.TestCase):
    def test_reads_each_input_section_the_map_places(self):
        self.assertEqual(gcclink.placements(MAP, 'final.map'),
                         [gcclink.Placed('.head', 'a.o', '.data', 0x0CB80000, 0x8),
                          gcclink.Placed('.text', 'a.o', '.text', 0x0CB80020, 0),
                          gcclink.Placed('.text', 'a.o', '.text.event_chaos4_init', 0x0CB80020, 0x1C),
                          gcclink.Placed('.text', 'libgcc.a(_movmem_i4.o)', '.text', 0x0CB80040, 0x60)])

    def test_refuses_a_map_cut_short(self):
        # the map without its last line, which names its output, and the map cut after the line that opens its memory
        # map
        opening = '\nLinker script and memory map\n'
        for case, cut in (('its last line', MAP[:MAP.rindex('OUTPUT(')]),
                          ('its memory map', MAP[:MAP.index(opening) + len(opening)])):
            with self.subTest(case), self.assertRaisesRegex(SystemExit, r"final\.map: not a whole map of GNU ld's$"):
                gcclink.placements(cut, 'final.map')


if __name__ == '__main__':
    unittest.main()
