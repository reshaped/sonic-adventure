import re
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import elf
import gcclink
import test_ld
from support import cwd

# STB_GNU_UNIQUE, a binding of GNU's own, neither global nor weak
GNU_UNIQUE = 10
# the index of a section of an object, and STT_NOTYPE, the type of a symbol that GNU as gives none
IN_SECTION, NO_TYPE = 1, 0
# the inputs of the tests of selected(), by name: each as GNU as's source, and as the symbols linkable() reads of its
# object, a name, a section index, a type and a binding each. common.o holds x as a common symbol. strong.o defines x
# by a global symbol and weakly.o by a weak one, and wanting.o refers to x weakly. Each of the rest is the one member
# of an archive: it defines x some way and takes helper, which nothing defines, so the link that takes it leaves helper
# undefined
HELPER = ('_helper', elf.UNDEFINED, NO_TYPE, elf.GLOBAL)
INPUTS = {
    'common': ('        .comm _x,4,4\n', [('_x', gcclink.COMMON, elf.OBJECT_SYMBOL, elf.GLOBAL)]),
    'strong': ('        .data\n        .global _x\n_x:     .long 1\n', [('_x', IN_SECTION, NO_TYPE, elf.GLOBAL)]),
    'weakly': ('        .data\n        .weak _x\n_x:     .long 1\n', [('_x', IN_SECTION, NO_TYPE, elf.WEAK)]),
    'wanting': ('        .data\n        .weak _x\n        .long _x\n', [('_x', elf.UNDEFINED, NO_TYPE, elf.WEAK)]),
    'datum': ('        .data\n        .global _x\n        .type _x, @object\n_x:     .long _helper\n',
              [('_x', IN_SECTION, elf.OBJECT_SYMBOL, elf.GLOBAL), HELPER]),
    'untyped': ('        .data\n        .global _x\n_x:     .long _helper\n',
                [('_x', IN_SECTION, NO_TYPE, elf.GLOBAL), HELPER]),
    'fixed': ('        .global _x\n        .set _x, 0x8c100000\n        .data\n        .long _helper\n',
              [('_x', elf.ABSOLUTE, NO_TYPE, elf.GLOBAL), HELPER]),
    'function': ('        .text\n        .global _x\n        .type _x, @function\n_x:     rts\n        nop\n'
                 '        .long _helper\n', [('_x', IN_SECTION, gcclink.FUNCTION, elf.GLOBAL), HELPER]),
    'weak': ('        .data\n        .weak _x\n_x:     .long _helper\n',
             [('_x', IN_SECTION, NO_TYPE, elf.WEAK), HELPER]),
    'shared': ('        .comm _x,4,4\n        .data\n        .long _helper\n',
               [('_x', gcclink.COMMON, elf.OBJECT_SYMBOL, elf.GLOBAL), HELPER]),
}
# whether GNU ld takes an archive's member, by the objects it links ahead of the archive and the member. Of a symbol
# an object holds as a common symbol, it takes the member whose global symbol that is no function's defines it, in a
# section or absolute. An object's global definition replaces the common symbol, before it or after, and its weak one
# doesn't. A weak reference takes no member
TAKES = {(('common',), 'datum'): True, (('common',), 'untyped'): True, (('common',), 'fixed'): True,
         (('common',), 'function'): False, (('common',), 'weak'): False, (('common',), 'shared'): False,
         (('strong', 'common'), 'datum'): False, (('common', 'strong'), 'datum'): False,
         (('weakly', 'common'), 'datum'): True, (('common', 'weakly'), 'datum'): True,
         (('wanting',), 'datum'): False}
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


def reckoned(name):
    """The Linkable that linkable() reads of INPUTS' symbols of the input named name."""
    return gcclink.linkable(elf.Elf(name, [], [elf.Symbol(symbol, 0, 4, section, kind, bind)
                                               for symbol, section, kind, bind in INPUTS[name][1]], {}))


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
            'a kernel row that names no kernel': ('kernel  main  a.o  .text.step', 'not a row: '),
            'a kernel named in capitals': ('kernel  main  DRAW  a.o  .text.step', 'not a row: '),
            'a kernel row of six words': ('kernel  main  draw  a.o  .text.step  pool', 'not a row: '),
            'a window row whose LINES is no number': ('window  main  draw  lines', 'not a row: '),
            'a window row whose OFFSET has a leading zero': ('window  main  draw  44  06', 'not a row: '),
            'a data row that names a kernel': ('data  main  draw  a.o  .bss', 'not a row: '),
            'an over row that names one kernel': ('over  main  sweep', 'not a row: '),
            'an over row that names two images': ('over  main  sweep  draw  ova  ova', 'not a row: '),
            'an over row whose OTHER is its own image': ('over  main  sweep  draw  main',
                                                        'main: not another image of the spec'),
            'an over row whose OTHER names no image': ('over  main  sweep  draw  ovc',
                                                      'ovc: not another image of the spec'),
            'a last row that names two objects': ('last  main  a.o  b.o', 'not a row: '),
            'a cold row that names two objects': ('cold  main  a.o  b.o', 'not a row: '),
            'a heat row whose HASH is no SHA-256': ('heat  main  heat.txt  0123abcd', 'not a row: '),
            'a second heat row': (f'heat  main  heat.txt  {"0" * 64}\nheat  main  heat.txt  {"0" * 64}',
                                  'main has a heat row already'),
            'a searched row without its SIZE': ('searched  main  a.o  .text.step  0x40', 'not a row: '),
            'a searched row whose PLACE is no number': ('searched  main  a.o  .text.step  first  8', 'not a row: '),
            'a second image row': ('image  main  0x8c020000  0x00010000', 'main has an image row already'),
            'a row whose image no image row above names': ('input  ovc  a.o', 'ovc: no image row above names it'),
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

    def test_reads_a_window_s_offset_as_0_where_its_row_gives_none_and_a_data_row_as_a_piece(self):
        with tempfile.TemporaryDirectory() as root:
            path = Path(root, 'link.txt')
            path.write_text('image  main  0x8c010000  0x00010000\nkernel  main  draw  a.o  .text.step\n'
                            'window  main  draw  44\nwindow  main  clip  0x67  6\ndata  main  a.o  .bss\n')
            image = gcclink.read(path)['main']
            self.assertEqual(image.kernel, [gcclink.Piece('draw', 'a.o', '.text.step')])
            self.assertEqual(image.windows, [gcclink.Window('draw', 44, 0), gcclink.Window('clip', 103, 6)])
            self.assertEqual(image.data, [gcclink.Piece('data', 'a.o', '.bss')])

    def test_reads_last_rows_paths_in_row_order(self):
        with tempfile.TemporaryDirectory() as root:
            path = Path(root, 'link.txt')
            path.write_text('image  main  0x8c010000  0x00010000\nlast  main  b.o\nlast  main  a.o\n')
            self.assertEqual(gcclink.read(path)['main'].last, ['b.o', 'a.o'])

    def test_reads_cold_rows_paths_in_row_order(self):
        with tempfile.TemporaryDirectory() as root:
            path = Path(root, 'link.txt')
            path.write_text('image  main  0x8c010000  0x00010000\ncold  main  b.o\ncold  main  a.o\n')
            self.assertEqual(gcclink.read(path)['main'].cold, ['b.o', 'a.o'])

    def test_reads_a_heat_row_s_path_and_hash_and_searched_rows_in_row_order(self):
        with tempfile.TemporaryDirectory() as root:
            path = Path(root, 'link.txt')
            path.write_text('image  main  0x8c010000  0x00010000\nimage  ova  0x0c900000  0x00010000\n'
                            f'heat  main  config/main/heat.txt  {"5a" * 32}\n'
                            'searched  main  b.o  .text.step  0x40  24\nsearched  main  a.o  P  0  0x10\n')
            images = gcclink.read(path)
            self.assertEqual(images['main'].heat, ['config/main/heat.txt', '5a' * 32])
            self.assertEqual(images['main'].searched, [gcclink.Searched('b.o', '.text.step', 64, 24),
                                                       gcclink.Searched('a.o', 'P', 0, 16)])
            self.assertEqual((images['ova'].heat, images['ova'].searched), ([], []))

    def test_reads_a_text_row_s_offset_and_refuses_an_image_s_second(self):
        with tempfile.TemporaryDirectory() as root:
            path = Path(root, 'link.txt')
            images = 'image  main  0x8c010000  0x00010000\nimage  ova  0x0c900000  0x00010000\n'
            path.write_text(images + 'text  main  0xa0\n')
            self.assertEqual([image.text for image in gcclink.read(path).values()], [160, None])
            for rows, problem in (('text  main  0xa0\ntext  main  0xa0\n', ':4: main has a text row already'),
                                  ('text  main  0xA0\n', ':3: not a row: '), ('text  main\n', ':3: not a row: ')):
                path.write_text(images + rows)
                with self.subTest(rows), self.assertRaisesRegex(SystemExit, f'link.txt{problem}'):
                    gcclink.read(path)

    def test_refuses_a_second_searched_row_of_one_section_of_an_image(self):
        # a record gives a section one place. Another image's row of the same PATH and INPUT is its own
        with tempfile.TemporaryDirectory() as root:
            path = Path(root, 'link.txt')
            images = 'image  main  0x8c010000  0x00010000\nimage  ova  0x0c900000  0x00010000\n'
            rows = 'searched  main  a.o  .text.step  0x40  24\nsearched  main  a.o  .text.walk  0x80  8\n'
            path.write_text(images + rows + 'searched  ova  a.o  .text.step  0x60  24\n')
            self.assertEqual([len(image.searched) for image in gcclink.read(path).values()], [2, 1])
            path.write_text(images + rows + 'searched  main  a.o  .text.step  0xa0  24\n')
            with self.assertRaisesRegex(SystemExit, r"link.txt:5: main has a searched row of a.o's \.text\.step "
                                                    'already$'):
                gcclink.read(path)

    def test_reads_an_over_row_s_other_after_its_kernels(self):
        with tempfile.TemporaryDirectory() as root:
            path = Path(root, 'link.txt')
            path.write_text('image  main  0x8c010000  0x00010000\nimage  ova  0x0c900000  0x00010000\n'
                            'over  main  draw  sweep\nover  main  draw  pieces  ova\n')
            self.assertEqual(gcclink.read(path)['main'].over, [('draw', 'sweep'), ('draw', 'pieces', 'ova')])


class Starts(unittest.TestCase):
    def test_reads_the_output_sections_a_starts_row_names_and_none_for_an_image_without_the_row(self):
        with tempfile.TemporaryDirectory() as root:
            path = Path(root, 'link.txt')
            path.write_text('image  main  0x8c010000  0x00010000\nimage  ova  0x0c900000  0x00010000\n'
                            'starts  main  .bss  .rodata\n')
            images = gcclink.read(path)
            self.assertEqual((images['main'].starts, images['ova'].starts), (['.bss', '.rodata'], []))

    def test_refuses_a_starts_row_that_names_no_output_section_of_data_one_twice_or_too_many_and_a_second_row(self):
        image = 'image  main  0x8c010000  0x00010000\n'
        cases = {'no output section': ('starts  main', 'not a row: '),
                 'the code': ('starts  main  .text', 'not a row: '),
                 "an input section's name": ('starts  main  .data.models', 'not a row: '),
                 'four output sections': ('starts  main  .rodata  .data  .bss  .data', 'not a row: '),
                 'one twice': ('starts  main  .data  .bss  .data', 'names .data twice'),
                 'a second row': ('starts  main  .data\nstarts  main  .bss', 'main has a starts row already')}
        with tempfile.TemporaryDirectory() as root:
            path = Path(root, 'link.txt')
            for case, (rows, problem) in cases.items():
                with self.subTest(case):
                    path.write_text(f'{image}{rows}\n')
                    line = f'{image}{rows}'.count('\n') + 1
                    with self.assertRaisesRegex(SystemExit, f'{re.escape(str(path))}:{line}: {problem}'):
                        gcclink.read(path)


class Exports(unittest.TestCase):
    def test_exports_a_global_or_weak_name_alone(self):
        # a datum of each binding in .data, GNU_UNIQUE among them
        data = elf.Section(1, '.data', 1, elf.ALLOC, 0x8C010000, 16, 4, bytes(16))
        symbols = [elf.Symbol(f'_{name}', 0x8C010000, 4, 1, 1, bind) for name, bind in
                   (('local', elf.LOCAL), ('global', elf.GLOBAL), ('weak', elf.WEAK), ('unique', GNU_UNIQUE))]
        self.assertEqual(list(gcclink.exports(elf.Elf('a.o', [data], symbols, {}))), ['global', 'weak'])


class Selected(unittest.TestCase):
    def test_takes_the_member_whose_strong_datum_defines_a_symbol_an_input_holds_as_common_and_none_defines(self):
        # each of TAKES' links, read as INPUTS' symbols give its inputs, so it needs no toolchain. The link that takes
        # the member leaves helper undefined, and names the member for it
        parts = {**{f'{name}.o': [(None, reckoned(name))] for name in INPUTS},
                 **{f'lib{name}.a': [(f'{name}.o', reckoned(name))] for name in INPUTS}}
        for (objects, member), takes in TAKES.items():
            with self.subTest(objects=objects, member=member), mock.patch.object(gcclink, 'linkables', parts.get):
                paths = [*(f'{name}.o' for name in objects), f'lib{member}.a']
                taken, undefined = gcclink.selected(paths)
                self.assertEqual(taken, [(path, None) for path in paths[:-1]]
                                 + ([(f'lib{member}.a', f'{member}.o')] if takes else []))
                self.assertEqual(undefined.get('_helper'), [f'lib{member}.a({member}.o)'] if takes else None)

    def test_takes_of_each_link_the_inputs_gnu_ld_s_map_places_and_reads_each_object_as_inputs_gives_it(self):
        # GNU as assembles INPUTS' sources, and linkable() reads of each object what it reads of INPUTS' symbols. GNU
        # ld then links each of TAKES' links, with helper left undefined, and its map places a section of each input
        # it takes: those selected() takes
        test_ld.toolchain(('bin/sh-elf-as', 'bin/sh-elf-ar', 'bin/sh-elf-ld'))
        assembling = test_ld.flags('GASFLAGS')
        with tempfile.TemporaryDirectory() as root, cwd(root):
            for name, (source, _) in INPUTS.items():
                Path(f'{name}.s').write_text(source)
                test_ld.run('bin/sh-elf-as', *assembling, '-o', f'{name}.o', f'{name}.s', root=root)
                test_ld.run('bin/sh-elf-ar', 'rc', f'lib{name}.a', f'{name}.o', root=root)
                self.assertEqual(gcclink.linkables(f'{name}.o'), [(None, reckoned(name))])
            for (objects, member), takes in TAKES.items():
                with self.subTest(objects=objects, member=member):
                    paths = [*(f'{name}.o' for name in objects), f'lib{member}.a']
                    test_ld.run('bin/sh-elf-ld', '-EL', '-e', '0', '--unresolved-symbols=ignore-all', '-Map',
                                'linked.map', '-o', 'linked.elf', *paths, root=root)
                    placed = {each.file for each in gcclink.placements(Path('linked.map').read_text(), 'linked.map')}
                    self.assertEqual({gcclink.map_name(*each) for each in gcclink.selected(paths)[0]}, placed)
                    self.assertEqual(f'lib{member}.a({member}.o)' in placed, takes)


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
                                                        r"\S*/16\.2\.0/libgcc\.a, and make dc's toolchain installs "
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
