import struct
import subprocess
import unittest
from pathlib import Path
from unittest import mock

import content
import data
import floats
import test_ld
from cdata import EmitError, Items
from image import Image, Item, window
from layout import LayoutError
from support import MODULE, cwd, tree
from test_data import BASE, KEYED, LOADS, ROOT, bits, land, words


class Keys(unittest.TestCase):
    def test_a_key_names_the_import_its_stage_reads(self):
        # STAGE_THREE's row points where no import lies, and no row keys the word past the table: neither takes a name
        with tree(KEYED):
            first = content.Config('1st_read').image('1ST_READ.BIN')
            self.assertEqual(content.keyed(first), {0x8C010004: 'a_init', 0x8C01000C: 'two__a_init'})

    def test_refuses_a_key_whose_stage_reads_no_image_that_takes_its_word(self):
        files = {**KEYED, '1ST_READ.BIN': words(0x100, 0x0C900000, 0x302, 0x0C900000, 0xFFFF, 0)}
        with tree(files), self.assertRaisesRegex(SystemExit, ': path_lists: its row at 0x8c010008 keys 0x0c900000 to '
                                                             'STAGE_THREE, act 2, which reads three on every pass, and '
                                                             'config/1st_read/imports.txt takes a_init from mod and '
                                                             'two__a_init from two there$'):
            content.keyed(content.Config('1st_read').image('1ST_READ.BIN'))

    def test_refuses_a_table_its_end_key_never_ends(self):
        files = {**KEYED, '1ST_READ.BIN': words(0x100, 0x0C900000)}
        with tree(files), self.assertRaisesRegex(SystemExit, 'path_lists: its rows run past 1ST_READ.BIN with no row '
                                                             'keyed 0xffff'):
            content.keyed(content.Config('1st_read').image('1ST_READ.BIN'))

    def test_refuses_a_key_whose_stage_reads_two_images_that_take_its_word(self):
        read = '        loader_read_prs("mod.PRS", MODULE_AREA);\n'
        loader = LOADS['src/system/loader.c'].replace(read, read + read.replace('mod', 'two'))
        with tree({**KEYED, 'src/system/loader.c': loader}), \
                self.assertRaisesRegex(SystemExit, ': path_lists: its row at 0x8c010000 keys 0x0c900000 to STAGE_ONE, '
                                                   'act 0, which reads mod, two on every pass, and config/1st_read/'
                                                   'imports.txt takes a_init from mod and two__a_init from two there$'):
            content.keyed(content.Config('1st_read').image('1ST_READ.BIN'))

    def test_refuses_a_key_whose_stage_reads_the_image_on_some_passes_alone(self):
        # STAGE_ONE reads mod only where an if holds, so its row's word is no import of an image it always reads, as
        # tools/imports.py holds a use of one
        read = '        loader_read_prs("mod.PRS", MODULE_AREA);\n'
        loader = LOADS['src/system/loader.c'].replace(read, '        if (sonic) {\n    ' + read + '        }\n')
        with tree({**KEYED, 'src/system/loader.c': loader}), \
                self.assertRaisesRegex(SystemExit, ': path_lists: its row at 0x8c010000 keys 0x0c900000 to STAGE_ONE, '
                                                   'act 0, which reads no image on every pass, and config/1st_read/'
                                                   'imports.txt takes a_init from mod and two__a_init from two there$'):
            content.keyed(content.Config('1st_read').image('1ST_READ.BIN'))

    def test_1st_read_bin_takes_names_from_its_own_table_alone(self):
        # data.Config's names give 0x0c900000 its first import, a_init. The content takes an import only by a key
        with tree(KEYED):
            self.assertEqual(data.Config('1st_read').names[0x0C900000], 'a_init')
            self.assertNotIn(0x0C900000, content.Config('1st_read').names)

    def test_reads_path_lists_keys_for_1st_read_bin_alone(self):
        # keyed() would read 1ST_READ.BIN's path_lists from test's image, which doesn't hold it, and fail
        written = write_content({'config/1st_read/symbols.txt': '0x8c010000  path_lists\n'})
        self.assertIn(b'dat_0c000180', written['.c'])

    def test_the_content_takes_no_definition_beside_it_that_no_key_gives(self):
        # STAGE_THREE's row, whose pointer no import holds, and the word past the table, which no row keys. The
        # address each holds could mean mod's definition, two's or three's
        with tree(KEYED):
            config = content.Config('1st_read')
            image = config.image('1ST_READ.BIN')
            for address, value in ((0x8C010014, 0x0C900010), (0x8C010020, 0x0C900000)):
                item = Item(address, address + 4, 'pointers')
                item.vouched = 'the test'
                writer = content.Content(Items(image, [item], [(address, address + 4)], {}), config.names,
                                         [(address, address + 4, False)], content.keyed(image),
                                         content.Providers(config))
                with self.subTest(address=address), \
                        self.assertRaisesRegex(EmitError, f'0x{address:08x}: 0x{value:08x} points into mod, three, two, '
                                                          r"where 1ST_READ\.BIN takes a definition only as the key of "
                                                          r"the word's row gives it, and no key gives this word one$"):
                    writer.source('the test')

    def test_the_content_takes_the_import_a_key_gives(self):
        # STAGE_ONE's row keys mod's a_init, which lies in mod's code, so the content declares a function
        with tree(KEYED):
            config = content.Config('1st_read')
            image = config.image('1ST_READ.BIN')
            item = Item(0x8C010004, 0x8C010008, 'pointers')
            item.vouched = 'the test'
            writer = content.Content(Items(image, [item], [(0x8C010004, 0x8C010008)], {}), config.names,
                                     [(0x8C010004, 0x8C010008, False)], content.keyed(image),
                                     content.Providers(config))
            emitted, placed = writer.source('the test'), writer.placed()
        self.assertIn('extern void a_init();\n\nvoid *dat_8c010004[] = {\n    a_init,\n};\n', emitted.text)
        self.assertEqual(emitted.outside, {'a_init': 0x0C900000})
        self.assertIn(('a_init', 0x0C900000, None, 'taken: mod a_init code'), placed)


class CommandLine(unittest.TestCase):
    def test_writes_the_content_of_image_to_out(self):
        with mock.patch.object(content, 'gcc_content') as function:
            content.main(['IMAGE', 'OUT'])
        function.assert_called_once_with('IMAGE', 'OUT')


# test's image: land() in a gen/ range, a.c's data at a_table, a second
# gen/ range, then b.c's constants. The second range holds four pointers,
# which data.txt vouches for, then a blob of two words and zeros. The
# pointers point at a_table, at TEXT_1, which b.c declares, at an address
# in a.c's data no table names, and at the blob
ROWS = ('0x0c000000  points[3]\n0x0c000024  strips[3]\n0x0c00002c  material[1]\n0x0c000040  meshset[1]\n'
        '0x0c000058  model\n0x0c000080  object\n0x0c0000b4  land_entry[2]\n0x0c0000fc  land\n0x0c000120  words[8]\n'
        '0x0c000180  pointers[4]  vouched\n0x0c000190  words[28]\n')
CONTENT = {
    'config/1st_read/splits.txt': '',
    'config/1st_read/symbols.txt': '',
    'config/test/splits.txt': ('D  0x0c000000  0x0c000200\nC  0x0c000200  0x0c000240\n'
                               'gen/test/d_0c000000.c  D  0x0c000000  0x0c000140\n'
                               'modules/test/a.c  D  0x0c000140  0x0c000180\n'
                               'gen/test/d_0c000180.c  D  0x0c000180  0x0c000200\n'
                               'modules/test/b.c  C  0x0c000200  0x0c000240\n'),
    'config/test/symbols.txt': '0x0c0000fc  test_land\n0x0c000140  a_table\n',
    'config/test/data.txt': ROWS,
    'modules/test/a.c': 'Uint32 a_table[4];\n',
    'modules/test/b.c': '#define TEXT_1_LENGTH 3\n#include "test/b/text.inc"\n',
    'build/gen/modules/test/b/bodies.txt': 'TEXT_1  0x0c000210  3  text\n',
    'TEST.BIN': (land() + bytes(0x40) + words(BASE + 0x140, BASE + 0x210, BASE + 0x150, BASE + 0x190)
                 + words(0x12345678, 0x9ABCDEF0) + bytes(0x68) + bytes(0x10) + b'abc\0' + bytes(0x2C)),
}
CONTENT_C = Path('build/gen/test/content.c')


def write_content(files=None):
    with tree(dict(CONTENT, **(files or {}))):
        content.gcc_content('TEST.BIN', str(CONTENT_C))
        return {suffix: CONTENT_C.with_suffix(suffix).read_bytes() for suffix in ('.c', '.txt', '.ld', '.bin', '.d')}


def pointing(value):
    """CONTENT's TEST.BIN, as a {path: bytes} map, with value in its third pointer."""
    return {'TEST.BIN': CONTENT['TEST.BIN'][:0x188] + words(value) + CONTENT['TEST.BIN'][0x18C:]}


# CONTENT with its third pointer at b_value, which test's tables and two's give 0x0ca00000. A gen/ range of two's
# holds b_value, and two's data.txt types it
TAKEN = {**pointing(0x0CA00000),
         'config/test/symbols.txt': CONTENT['config/test/symbols.txt'] + '0x0ca00000  b_value\n',
         'config/1st_read/splits.txt': MODULE['config/1st_read/splits.txt'], 'src/main.c': '',
         'config/two/splits.txt': 'D  0x0ca00000  0x0ca00100\ngen/two/d_0ca00000.c  D  0x0ca00000  0x0ca00100\n',
         'config/two/symbols.txt': '0x0ca00000  b_value\n', 'config/two/data.txt': '0x0ca00000  Uint32\n'}


def gcc(*args):
    """The CompletedProcess of the toolchain's sh-elf-NAME, NAME being
    args[0], run on args[1:] from the checkout, its output captured as text.
    The toolchain is test_ld.TOOLCHAIN. Where it lacks sh-elf-gcc or
    sh-elf-ld, the test fails if SH_ELF names it, as test_ld's fixture
    does, and skips otherwise. It skips too where tools/sh4zam/include, in
    which make gcc's flags find system headers, is missing."""
    toolchain = test_ld.TOOLCHAIN
    missing = [tool for tool in ('bin/sh-elf-gcc', 'bin/sh-elf-ld') if not (toolchain / tool).is_file()]
    if missing and test_ld.GIVEN:
        raise AssertionError(f'{toolchain}, which SH_ELF names, lacks {", ".join(missing)}')
    if missing:
        raise unittest.SkipTest(f'{toolchain} lacks {", ".join(missing)}, which make gcc builds')
    if not (ROOT / 'tools/sh4zam/include').is_dir():
        raise unittest.SkipTest('tools/sh4zam/include: missing, and make gcc fetches it')
    return subprocess.run([toolchain / 'bin' / f'sh-elf-{args[0]}', *args[1:]], cwd=ROOT, capture_output=True,
                          text=True)


def compiled(root, source, out, respelled=True):
    """The object at out under root that GCC compiles from source, a path
    under root, as make gcc compiles a content. GCC's preprocessor writes
    the C to a .i beside the object. tools/floats.py, run from the checkout,
    respells the .i's floats in place as SHC 5.0's release R32 reads them
    under -fpu=single, and GCC compiles the .i. With respelled false, GCC
    compiles source as written. GCC takes the flags make gives a content's
    compile, and searches the checkout's sdk/include and src for headers,
    where make searches modules too. Fails or skips as gcc() does, and
    raises AssertionError with GCC's messages where GCC fails."""
    obj, c, include = root / out, root / source, ['-Isdk/include', '-Isrc']
    flags = test_ld.flags('GCCFLAGS', 'build/gcc/gen/test/content.o')
    if respelled:
        result = gcc('gcc', *flags, *include, '-E', '-o', obj.with_suffix('.i'), c)
        if result.returncode:
            raise AssertionError(result.stderr)
        c, include = obj.with_suffix('.i'), []
        with cwd(ROOT):
            floats.main(['--shc', 'r32', '-fpu=single', '--system', *test_ld.flags('SH4ZAM_INCLUDE'), str(c)])
    result = gcc('gcc', *flags, *include, '-c', '-o', obj, c)
    if result.returncode:
        raise AssertionError(result.stderr)
    return obj


class Content(unittest.TestCase):
    def test_writes_each_range_as_c_its_pointers_names_and_its_record(self):
        written = write_content()
        text, record = written['.c'].decode(), written['.txt'].decode()
        self.assertIn('#include "collision/world.h"\n', text)
        self.assertIn('struct land_table test_land = { 2, 0, 4, 0, 1000.0F, dat_0c0000b4, 0, 0, 0, 0, 0 };', text)
        # the model's meshset, and its material, which it leaves uncounted
        self.assertIn('NJS_MODEL dat_0c000058 = {\n    dat_0c000000, 0, 3,\n    dat_0c000040, dat_0c00002c, 1, 0,\n',
                      text)
        # a_table by its source's type, and a spelling for 0x0c000150, which
        # no table names
        self.assertIn('extern Uint32 a_table[];\nextern char unresolved_0c000188[] '
                      '__asm__("unresolved.test.0c000188.0c000150");\n', text)
        self.assertIn('static char const dat_0c000210[4] = "abc";', text)
        self.assertIn('void *dat_0c000180[] = {\n    a_table, (void *)dat_0c000210, unresolved_0c000188, dat_0c000190,\n};',
                      text)
        # the blob is a byte array
        self.assertIn('Uint8 dat_0c000190[112] = {\n    0x78, 0x56, 0x34, 0x12, 0xf0, 0xde, 0xbc, 0x9a, 0x00,', text)
        for row in ('test_land  0x0c0000fc  36  struct land_table',
                    'dat_0c000058  0x0c000058  40  NJS_MODEL',
                    'dat_0c000180  0x0c000180  16  void *[]: config/test/data.txt:10',
                    'dat_0c000190  0x0c000190  112  bytes: non-address data, no word an address',
                    'dat_0c000210  0x0c000210  4  text: a copy of TEXT_1, which modules/test/b.c declares',
                    'a_table  0x0c000140  -  taken: test a_table Uint32[]',
                    'unresolved.test.0c000188.0c000150  0x0c000150  -  unresolved: the word at 0x0c000188 points '
                    'where no table names a definition'):
            self.assertIn(row + '\n', record)

    def test_its_check_places_each_definition_at_its_address_and_gives_the_disc_s_bytes(self):
        written = write_content()
        script = written['.ld'].decode()
        # SUBALIGN(1) sets aside each definition's own alignment, so each lies at the offset its address gives
        self.assertIn('  .0c000000 0x0c000000 : SUBALIGN(1) {\n    . = 0x0; *(.data.dat_0c000000 .rodata.dat_0c000000)\n',
                      script)
        self.assertIn('    . = 0xfc; *(.data.test_land .rodata.test_land)\n'
                      '    . = 0x120; *(.data.dat_0c000120 .rodata.dat_0c000120)\n    . = 0x140;\n  } =0x00\n', script)
        self.assertIn('  .0c000210 0x0c000210 : SUBALIGN(1) {\n', script)
        self.assertIn('_a_table = 0x0c000140;\nunresolved.test.0c000188.0c000150 = 0x0c000150;\n', script)
        payload = CONTENT['TEST.BIN']
        self.assertEqual(written['.bin'], payload[:0x140] + bytes(0x40) + payload[0x180:0x200] + bytes(0x10) + b'abc\0')

    def test_its_rule_names_each_file_it_reads(self):
        # and the data.txt of an image whose gen/ range it takes a name from, and the bodies.txt of a source whose
        # text it copies, with every image's splits.txt, which give the source's home
        rule = write_content(TAKEN)['.d'].decode()
        self.assertTrue(rule.startswith('build/gen/test/content.c: '), rule)
        for path in ('TEST.BIN', 'config/test/data.txt', 'config/test/splits.txt', 'modules/test/a.c',
                     'config/two/data.txt', 'modules/test/b.c', 'build/gen/modules/test/b/bodies.txt',
                     'config/two/splits.txt'):
            self.assertIn(f' {path}', rule.split('\n', 1)[0])
            self.assertIn(f'\n{path}:\n', rule)

    def test_a_name_another_images_gen_range_holds_takes_the_type_its_data_txt_gives(self):
        written = write_content(TAKEN)
        self.assertIn('extern Uint32 b_value;\n', written['.c'].decode())
        self.assertIn('    a_table, (void *)dat_0c000210, &b_value, dat_0c000190,\n', written['.c'].decode())
        self.assertIn('b_value  0x0ca00000  -  taken: two b_value Uint32\n', written['.txt'].decode())

    def test_a_malformed_row_of_another_images_data_txt_fails_naming_its_line(self):
        with self.assertRaisesRegex(SystemExit, r'config/two/data\.txt:1: not a row: ADDRESS KIND, or KIND\[COUNT\] '
                                                r'for an array, then vouched where it vouches$'):
            write_content({**TAKEN, 'config/two/data.txt': '0x0ca00000  Uint32[2\n'})

    def test_a_pointer_into_an_array_a_source_defines_names_its_element(self):
        # a_table's second element, and a byte inside its first, which no element starts
        for value, row in ((BASE + 0x144, '&a_table[1]'), (BASE + 0x142, 'unresolved_0c000188')):
            with self.subTest(hex(value)):
                text = write_content(pointing(value))['.c'].decode()
                self.assertIn(f'    a_table, (void *)dat_0c000210, {row}, dat_0c000190,\n', text)

    def test_a_name_a_source_declares_twice_takes_its_last_declaration(self):
        # GCC's side of the guard gives a_table a fifth element, at 0x0c000150. The other side, the last
        # declaration, gives it four
        source = '#ifdef GCC\nUint32 a_table[5];\n#else\nUint32 a_table[4];\n#endif\n'
        text = write_content({**pointing(BASE + 0x150), 'modules/test/a.c': source})['.c'].decode()
        self.assertIn('    a_table, (void *)dat_0c000210, unresolved_0c000188, dat_0c000190,\n', text)

    def test_a_pointer_into_the_contents_const_data_casts_the_const_away(self):
        # a gen/ range of constants in b.c's place, which the content defines const, holds the string at 0x0c000210
        splits = CONTENT['config/test/splits.txt'].replace('modules/test/b.c  C', 'gen/test/d_0c000200.c  C')
        rows = ROWS + '0x0c000200  words[4]\n0x0c000210  string[48]\n'
        text = write_content({'config/test/splits.txt': splits, 'config/test/data.txt': rows})['.c'].decode()
        self.assertIn('char const dat_0c000210[48] = "abc";', text)
        self.assertIn('    a_table, (void *)dat_0c000210, unresolved_0c000188, dat_0c000190,\n', text)

    def test_a_pointer_to_a_text_of_a_source_another_image_links_first_takes_its_address_there(self):
        # one, first by name, links b.c at 0x0ca00000, so b.c's bodies.txt gives TEXT_1 there. It lies as far into
        # b.c in test, at 0x0c000210, and 4 bytes further where bodies.txt gives 0x0ca00014
        one = {'config/one/splits.txt': 'C  0x0ca00000  0x0ca00040\nmodules/test/b.c  C  0x0ca00000  0x0ca00040\n',
               'config/one/symbols.txt': ''}
        for row, value in (('0x0ca00010', '(void *)dat_0c000210'), ('0x0ca00014', 'unresolved_0c000184')):
            with self.subTest(row):
                written = write_content({**one, 'build/gen/modules/test/b/bodies.txt': f'TEXT_1  {row}  3  text\n'})
                self.assertIn(f'    a_table, {value}, unresolved_0c000188, dat_0c000190,\n', written['.c'].decode())
                self.assertIn(' config/one/splits.txt', written['.d'].decode())

    def test_a_pointer_to_a_text_that_takes_no_copy_stays_unresolved_beside_one_that_does(self):
        # the char * and the int * both point at TEXT_1, and only a void or char pointer takes a copy
        files = {**pointing(BASE + 0x210), 'modules/test/a.h': 'struct refs { void *a; char *b; int *c; void *d; };\n',
                 'modules/test/a.c': '#include "test/a.h"\n' + CONTENT['modules/test/a.c'],
                 'config/test/data.txt': ROWS.replace('pointers[4]  vouched', 'struct refs')}
        written = write_content(files)
        text, record = written['.c'].decode(), written['.txt'].decode()
        self.assertIn('extern char unresolved_0c000188[] __asm__("unresolved.test.0c000188.0c000210");\n', text)
        self.assertIn('a_table, (char *)dat_0c000210, (int *)unresolved_0c000188, dat_0c000190', text)
        self.assertIn('unresolved.test.0c000188.0c000210  0x0c000210  -  unresolved: ', record)

    def test_a_pointer_to_a_string_no_source_declares_takes_no_copy(self):
        # xyz just past a_table, in a.c's data, which declares no text, and abc's second byte, inside TEXT_1
        payload = CONTENT['TEST.BIN'][:0x150] + b'xyz\0' + CONTENT['TEST.BIN'][0x154:]
        for value in (BASE + 0x150, BASE + 0x211):
            with self.subTest(hex(value)):
                text = write_content({'TEST.BIN': payload[:0x184] + words(value) + payload[0x188:]})['.c'].decode()
                self.assertIn('    a_table, unresolved_0c000184, unresolved_0c000188, dat_0c000190,\n', text)
                self.assertNotIn(f'dat_{value:08x}', text)

    def test_a_text_takes_a_copy_of_its_bytes_as_its_source_declares_them(self):
        # TEXT_1 of 4 bytes of Shift JIS, あい
        payload = CONTENT['TEST.BIN'][:0x210] + bytes([0x82, 0xA0, 0x82, 0xA2, 0]) + CONTENT['TEST.BIN'][0x215:]
        text = write_content({'TEST.BIN': payload,
                              'build/gen/modules/test/b/bodies.txt': 'TEXT_1  0x0c000210  4  text\n'})['.c'].decode()
        self.assertIn('static char const dat_0c000210[5] = "\\x82\\xa0\\x82\\xa2";', text)

    def test_a_source_with_bodies_and_no_bodies_txt_fails(self):
        with tree(CONTENT) as root:
            (root / 'build/gen/modules/test/b/bodies.txt').unlink()
            with self.assertRaisesRegex(SystemExit, r'build/gen/modules/test/b/bodies\.txt'):
                content.gcc_content('TEST.BIN', str(CONTENT_C))

    def test_a_word_in_ram_that_nothing_types_fails_naming_it(self):
        with self.assertRaisesRegex(EmitError, r'TEST\.BIN: 0x0c000180: 0x0c000140 lies in RAM, in dat_0c000180, an '
                                               r'array of no known structure, and nothing says whether it is a '
                                               r'pointer: a type data\.txt gives, or vouched on its row, says so'):
            write_content({'config/test/data.txt': ROWS.replace('  vouched', '')})

    def test_a_word_of_a_blob_that_points_into_ram_fails(self):
        # 0xac000100, RAM through its uncached view, which the content's views leave out
        payload = CONTENT['TEST.BIN'][:0x194] + words(0xAC000100) + CONTENT['TEST.BIN'][0x198:]
        with self.assertRaisesRegex(EmitError, r'TEST\.BIN: 0x0c000194: 0xac000100 lies in RAM, in dat_0c000190'):
            write_content({'TEST.BIN': payload})

    def test_a_word_in_ram_that_rows_of_blobs_split_fails(self):
        # a_table's address, at 0x0c0001a0, which two rows of halves split
        payload = CONTENT['TEST.BIN']
        payload = payload[:0x1A0] + words(BASE + 0x140) + payload[0x1A4:]
        rows = ROWS.replace('0x0c000190  words[28]\n', '0x0c000190  words[4]\n0x0c0001a0  halves[1]\n'
                                                       '0x0c0001a2  halves[1]\n0x0c0001a4  words[23]\n')
        with self.assertRaisesRegex(EmitError, r'TEST\.BIN: 0x0c0001a0: 0x0c000140 lies in RAM, across dat_0c0001a0 and '
                                               r'dat_0c0001a2, arrays of no known structure'):
            write_content({'TEST.BIN': payload, 'config/test/data.txt': rows})

    def test_a_word_a_row_vouches_for_that_points_into_ram_through_another_view_fails(self):
        # a_table, through RAM's uncached view
        with self.assertRaisesRegex(EmitError, r"TEST\.BIN: 0x0c000188: 0xac000140 points into RAM through a view the "
                                               r"content doesn't name$"):
            write_content({**pointing(0xAC000140),
                           'config/test/data.txt': ROWS.replace('pointers[4]  vouched', 'mixed[4]  vouched')})

    def test_a_row_of_bytes_no_structure_claims_vouches_for_nothing(self):
        with self.assertRaisesRegex(SystemExit, r'config/test/data\.txt:11: words: vouched, and only pointers, mixed or '
                                                r'a C type holds words a row vouches for$'):
            write_content({'config/test/data.txt': ROWS.replace('words[28]', 'words[28]  vouched')})

    def test_a_pointer_into_padding_fails(self):
        # the strip's two bytes of padding, up to the material's alignment
        payload = CONTENT['TEST.BIN'][:0x188] + words(BASE + 0x2A) + CONTENT['TEST.BIN'][0x18C:]
        with self.assertRaisesRegex(EmitError, r'0x0c000188: a pointer to 0x0c00002a, in the padding'):
            write_content({'TEST.BIN': payload})

    def test_a_blob_takes_its_items_alignment_for_the_fill_before_it(self):
        # two bytes of fill after a half word, which align the words the blob holds to 4
        image = Image('TEST.BIN', bytes(4) + words(0x04030201), BASE, (), (window(BASE),))
        items = [Item(BASE, BASE + 2, 'halves'), content.Blob(Item(BASE + 4, BASE + 8, 'words'))]
        self.assertEqual(Items(image, items, [(BASE, BASE + 8)], {}).items, items)

    def test_an_image_with_no_gen_range_fails(self):
        splits = 'D  0x0c000000  0x0c000200\nmodules/test/a.c  D  0x0c000140  0x0c000180\n'
        with tree(dict(CONTENT, **{'config/test/splits.txt': splits})), \
                self.assertRaisesRegex(SystemExit, r'config/test/splits\.txt: no gen/ range, so no content'):
            content.gcc_content('TEST.BIN', str(CONTENT_C))

    def test_takes_each_definition_from_data_txt_alone(self):
        # test_land's row, made nine words, holds dat_0c0000b4 in an array of no known structure
        with self.assertRaisesRegex(EmitError, '0x0c0000b4 lies in RAM, in test_land, an array of no known structure'):
            write_content({'config/test/data.txt': ROWS.replace('  land\n', '  words[9]\n')})

    def test_a_name_no_definition_types_fails(self):
        with self.assertRaisesRegex(SystemExit, r'test: a_table, at 0x0c000140, which its content takes from test: '
                                                r'no definition or pointer types it'):
            write_content({'modules/test/a.c': ''})

    def test_names_storage_no_row_holds_by_the_definition_a_source_gives_it_under_gcc(self):
        # c_store lies in bss that no row holds, and b.c defines it under GCC. A word into it past its start names an
        # element
        store = '#ifdef GCC\nUint32 c_store[4];\n#endif\n'
        storage = {'config/test/splits.txt': CONTENT['config/test/splits.txt'] + 'B  0x0c000240  0x0c000280\n',
                   'config/test/symbols.txt': CONTENT['config/test/symbols.txt'] + '0x0c000240  c_store\n',
                   'modules/test/b.c': CONTENT['modules/test/b.c'] + store,
                   'TEST.BIN': CONTENT['TEST.BIN'].replace(words(BASE + 0x150, BASE + 0x190),
                                                           words(BASE + 0x240, BASE + 0x248))}
        written = write_content(storage)
        text, record = written['.c'].decode(), written['.txt'].decode()
        self.assertIn('extern Uint32 c_store[];\n', text)
        self.assertIn('void *dat_0c000180[] = {\n    a_table, (void *)dat_0c000210, c_store, &c_store[2],\n};', text)
        self.assertIn('c_store  0x0c000240  -  taken: test c_store Uint32[]\n', record)
        # two sources' definitions of it fail, naming both
        twice = dict(storage, **{'modules/test/a.c': CONTENT['modules/test/a.c'] + store})
        with self.assertRaisesRegex(SystemExit, r'test: c_store, at 0x0c000240, where no row of '
                                                r'config/test/splits\.txt lies: modules/test/a\.c and '
                                                r'modules/test/b\.c define it$'):
            write_content(twice)
        # outside bss, a definition no row holds stands for no storage
        storage['config/test/splits.txt'] = CONTENT['config/test/splits.txt'] + 'DSG  0x0c000240  0x0c000280\n'
        with self.assertRaisesRegex(SystemExit, r'test: c_store, at 0x0c000240, which its content takes from test: '
                                                r'no definition or pointer types it'):
            write_content(storage)

    def test_writes_a_motion_file_with_its_actions_objects_by_name_and_links_it_to_the_disc_s_bytes(self):
        # HERO.BIN, a motion file at 0x0cc00000: two angle keys, an mdata entry, a motion and hero_action. The
        # action's object is hero_object, which main.c defines in a row of 1ST_READ.BIN's D at the address its table
        # gives. The check's link gives the file's bytes, with hero_object at its own address
        area = 0x0CC00000
        hero = (words(0, 0, 0, 0, 1, 0, 0, 0) + words(area, 2) + words(area + 0x20, 2) + struct.pack('<HH', 2, 1)
                + words(0x8C1755E4, area + 0x28))
        files = {**MODULE, 'config/1st_read/symbols.txt': MODULE['config/1st_read/symbols.txt'] + '0x8c1755e4  hero_object\n',
                 'config/1st_read/splits.txt': (MODULE['config/1st_read/splits.txt']
                                                + 'src/main.c  D  0x8c1755e4  0x8c175618\n'),
                 'src/main.c': 'NJS_OBJECT hero_object;\n',
                 'config/hero/splits.txt': 'D  0x0cc00000  0x0cc0003c\ngen/hero/d_0cc00000.c  D  0x0cc00000  0x0cc0003c\n',
                 'config/hero/symbols.txt': '0x0cc00034  hero_action\n',
                 'config/hero/data.txt': '0x0cc00000  mkey_a[2]\n0x0cc00020  mdata1[1]\n0x0cc00028  motion\n'
                                         '0x0cc00034  action\n',
                 'HERO.BIN': hero}
        out = Path('build/gen/hero/content.c')
        with tree(files) as root:
            content.gcc_content('HERO.BIN', str(out))
            text, record = out.read_text(), out.with_suffix('.txt').read_text()
            self.assertIn('extern NJS_OBJECT hero_object;\n', text)
            self.assertIn('NJS_ACTION hero_action = { &hero_object, &dat_0cc00028 };', text)
            self.assertIn('hero_object  0x8c1755e4  -  taken: 1st_read hero_object NJS_OBJECT\n', record)
            obj, linked = compiled(root, out, 'content.o'), root / 'content.out'
            result = gcc('ld', '-m', 'shlelf', '--orphan-handling=error', '--fatal-warnings', '--oformat=binary',
                         '-T', root / out.with_suffix('.ld'), '-o', linked, obj)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(linked.read_bytes(), hero)

    def test_gcc_compiles_it_once_respelled_and_its_link_at_the_image_s_addresses_gives_the_disc_s_bytes(self):
        # the land's clip distance, 0x3dcccccc, which the content spells 0.1F as SHC reads it. GCC compiles 0.1F as
        # 0x3dcccccd unless tools/floats.py respells it first. 0.2F, respelled, gives other bytes
        payload = land(clip=0x3DCCCCCC) + CONTENT['TEST.BIN'][len(land()):]
        with tree(dict(CONTENT, **{'TEST.BIN': payload})) as root:
            content.gcc_content('TEST.BIN', str(CONTENT_C))
            source = CONTENT_C.read_text()
            self.assertIn('struct land_table test_land = { 2, 0, 4, 0, 0.1F, ', source)
            for case, edit, respelled in (('respelled', None, True), ('as written', None, False),
                                          ('with another float', ('0.1F', '0.2F'), True)):
                with self.subTest(case):
                    CONTENT_C.write_text(source.replace(*edit) if edit else source)
                    obj, linked = compiled(root, CONTENT_C, 'content.o', respelled), root / 'content.out'
                    result = gcc('ld', '-m', 'shlelf', '--orphan-handling=error', '--fatal-warnings',
                                 '--oformat=binary', '-T', root / CONTENT_C.with_suffix('.ld'), '-o', linked, obj)
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertEqual(linked.read_bytes() == CONTENT_C.with_suffix('.bin').read_bytes(),
                                     case == 'respelled')

    def test_no_definition_takes_an_unresolved_word_and_the_link_names_it(self):
        # other.o defines a_table, and dat_0c000150, the name the generated C
        # would make up for 0x0c000150. Linked with the content, it binds
        # a_table. The link fails on the word's spelling, which names the
        # image, the word's address and its target
        with tree(dict(CONTENT, **{'other.c': 'unsigned long a_table[4];\nchar dat_0c000150[16];\n'})) as root:
            content.gcc_content('TEST.BIN', str(CONTENT_C))
            objects = [compiled(root, CONTENT_C, 'content.o'), compiled(root, 'other.c', 'other.o')]
            result = gcc('ld', '-m', 'shlelf', '-o', root / 'linked', *objects)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('undefined reference to `unresolved.test.0c000188.0c000150\'', result.stderr)
        self.assertNotIn('a_table', result.stderr)


# a code section whose words hold the addresses of a text and of a path's
# points, then the text, the points and the path's record. The text's
# second word, and the first point's angles, read as addresses. The
# record's third word, where its points go, holds step's address
EVIDENCE = {
    'config/1st_read/symbols.txt': '',
    'config/test/splits.txt': ('P  0x0c000000  0x0c000020\nD  0x0c000020  0x0c000080\n'
                               'modules/test/a.c  P  0x0c000000  0x0c000020\n'
                               'gen/test/d_0c000020.c  D  0x0c000020  0x0c000080\n'),
    'config/test/symbols.txt': '0x0c000000  step\n',
    'config/test/data.txt': '0x0c000020  string[16]\n0x0c000030  path_point[2]\n0x0c000058  path\n0x0c000068  words[6]\n',
    'modules/test/a.c': '#include "test/a.h"\n#include "test/b.h"\n\nvoid step(void)\n{\n}\n',
    'modules/test/a.h': '', 'modules/test/b.h': '',
    'TEST.BIN': (words(0x0009000B, BASE + 0x20, BASE + 0x30) + bytes(0x14)
                 # あい[ス検 in Shift JIS, whose second word reads as 0x8c58835b
                 + bytes([0x82, 0xA0, 0x82, 0xA2, 0x5B, 0x83, 0x58, 0x8C, 0x9F]) + bytes(7)
                 + struct.pack('<2h4f', 0x350E, 0x0CEF, 1, 0, 0, 0) + struct.pack('<2h4f', 0, 0, 1, 1, 0, 0)
                 + struct.pack('<2BHfII', 0, 0, 2, 2, BASE, 0) + bytes(0x18)),
}


def evidence(files=None):
    with tree(dict(EVIDENCE, **(files or {}))):
        content.gcc_content('TEST.BIN', str(CONTENT_C))
        return CONTENT_C.read_text(), CONTENT_C.with_suffix('.txt').read_text()


# struct odd, a record of a count and a pointer to points, and struct odd_point, the points' type
ODD = ('struct odd_point { short ang_x; short ang_z; float length; float pos[3]; };\n'
       'struct odd { unsigned long count; struct odd_point *points; };\n')
# EVIDENCE with a struct odd at 0x0c000058, odd_record, that points at the points as struct odd_point
ODD_RECORD = {'config/test/symbols.txt': '0x0c000000  step\n0x0c000058  odd_record\n',
              'config/test/data.txt': ('0x0c000020  string[16]\n0x0c000030  struct odd_point[2]\n'
                                       '0x0c000058  struct odd\n0x0c000060  words[8]\n'),
              'TEST.BIN': EVIDENCE['TEST.BIN'][:0x58] + words(2, BASE + 0x30, 0, 0) + bytes(0x18)}
# test's image: a code word, then zeros, the words 1 to 4 at 0x0c000020,
# and zeros
NUMBERS = words(0x0009000B) + bytes(0x1C) + words(1, 2, 3, 4) + bytes(0x50)


class Definitions(unittest.TestCase):
    def test_text_angles_and_a_pointer_in_one_array_take_their_types(self):
        text, record = evidence()
        # the text is text, though its second word lies in RAM
        self.assertIn('dat_0c000020  0x0c000020  16  text\n', record)
        self.assertIn('char dat_0c000020[16] = "\\x82\\xa0\\x82\\xa2[\\x83X\\x8c\\x9f";', text)
        # the angles, which read as 0x0cef350e, are numbers. The step, where
        # the points go, is a name
        self.assertIn('struct path_point dat_0c000030[] = {\n    { 13582, 3311, 1.0F, { 0.0F, 0.0F, 0.0F } },', text)
        self.assertIn('struct path dat_0c000058 = { 0, 2, 2.0F, (struct path_point *)step, 0 };', text)
        self.assertIn('dat_0c000030  0x0c000030  40  struct path_point[]\n', record)
        self.assertIn('step  0x0c000000  -  taken: test step code\n', record)

    def test_a_c_type_takes_its_type_and_its_header(self):
        # a header of a.c's defines struct odd
        text, record = evidence({**ODD_RECORD, 'modules/test/a.h': ODD})
        self.assertIn('odd_record  0x0c000058  8  struct odd\n', record)
        self.assertIn('dat_0c000030  0x0c000030  40  struct odd_point[]\n', record)
        self.assertIn('#include "test/a.h"\n', text)
        self.assertIn('struct odd odd_record = { 0x00000002, dat_0c000030 };', text)

    def test_a_c_type_a_source_the_image_doesnt_link_defines_has_no_layout(self):
        # other.c, a source the image doesn't link, defines struct odd. Another image's source can give a type of the
        # same name another layout
        with self.assertRaisesRegex(SystemExit, r"config/test/data\.txt:2: struct odd_point: no kind, and no type the "
                                                r"image's declarations lay out"):
            evidence({**ODD_RECORD, 'modules/test/other.c': ODD})

    def test_a_union_is_its_first_member(self):
        # the second cell's word, as the union's first member, is a long
        files = {'config/test/symbols.txt': '0x0c000000  step\n0x0c000020  cells\n',
                 'config/test/data.txt': '0x0c000020  union cell[24]\n',
                 'modules/test/a.h': 'union cell { long word; float number; };\nextern union cell cells[];\n',
                 'TEST.BIN': words(0x0009000B, BASE + 0x20) + bytes(0x18) + words(5, 0x12345678, 7) + bytes(0x54)}
        text, record = evidence(files)
        self.assertIn('cells  0x0c000020  96  union cell[]\n', record)
        self.assertIn('union cell cells[] = {\n    5,\n    305419896,\n    7,\n', text)

    def test_halves_no_structure_claims_are_an_array_of_halves(self):
        # xs, at 2 mod 4, after the half at 0x0c000020. SH's GCC aligns an array of bytes to 4, which would part xs
        # from that half
        files = {'config/test/symbols.txt': '0x0c000000  step\n0x0c000022  xs\n',
                 'config/test/data.txt': '0x0c000020  halves[1]\n0x0c000022  halves[1]\n0x0c000024  words[23]\n',
                 'TEST.BIN': NUMBERS[:0x22] + struct.pack('<H', 0x1234) + NUMBERS[0x24:]}
        text, record = evidence(files)
        self.assertIn('Uint16 dat_0c000020[1] = {\n    0x0001,\n};\n\nUint16 xs[1] = {\n    0x1234,\n};', text)
        self.assertIn('xs  0x0c000022  2  halves: non-address data, a half word\n', record)


class Sources(unittest.TestCase):
    def test_1st_reads_types_come_only_from_the_sources_it_links(self):
        # as any image's do: another image's source can give a type of the same name another layout
        files = {**MODULE, 'src/main.c': 'struct kept { long a; };\n', 'modules/mod/a.c': 'struct other { long b; };\n'}
        with tree(files):
            types = data.Declarations(content.Config('1st_read')).types
            self.assertEqual(types.layout('struct kept').size, 4)
            with self.assertRaises(LayoutError):
                types.layout('struct other')

    def test_a_string_defines_nothing(self):
        # a definition on a line that a splice continues a string onto
        note = 'const char *note = "\\\nUint32 hidden_store[4];";\n'
        with tree({**MODULE, 'src/main.c': note}):
            defined = content.Providers(content.Config('1st_read')).definitions('src/main.c')
        self.assertEqual(defined, {'note': ('const char *', False, '')})


def node(child=0):
    """An object with no model or sibling and a scale of 1, whose child is
    child, or 0 for none."""
    return words(0, 0, 0, 0, 0, 0, 0, 0, bits(1.0), bits(1.0), bits(1.0), child, 0)


# an object, whose child is an object at 0x0c000034
ROOTED = {
    'config/1st_read/symbols.txt': '',
    'config/test/splits.txt': 'D  0x0c000000  0x0c000100\ngen/test/d_0c000000.c  D  0x0c000000  0x0c000100\n',
    'config/test/symbols.txt': '',
    'config/test/data.txt': '0x0c000000  object\n0x0c000034  object\n0x0c000068  words[38]\n',
    'TEST.BIN': node(BASE + 0x34) + node() + bytes(0x98),
}


def rooted(files=None):
    with tree(dict(ROOTED, **(files or {}))):
        content.gcc_content('TEST.BIN', str(CONTENT_C))
        return CONTENT_C.read_text(), CONTENT_C.with_suffix('.txt').read_text()


class Floats(unittest.TestCase):
    def test_a_float_that_lies_in_ram_fails(self):
        # the object's position, whose x holds its child's address
        payload = words(0, 0, BASE + 0x34, 0, 0, 0, 0, 0, bits(1.0), bits(1.0), bits(1.0), BASE + 0x34, 0) + node()
        with self.assertRaisesRegex(EmitError, r'0x0c000008: 0x0c000034 lies in RAM, in dat_0c000000, a NJS_OBJECT, '
                                               r'whose type gives it a float, and nothing says whether it is a '
                                               r'pointer$'):
            rooted({'TEST.BIN': payload + bytes(0x98)})

    def test_a_float_that_lies_in_ram_through_p2_fails(self):
        # the object's position, whose x holds its child's address through P2's view
        payload = words(0, 0, 0xAC000034, 0, 0, 0, 0, 0, bits(1.0), bits(1.0), bits(1.0), BASE + 0x34, 0) + node()
        with self.assertRaisesRegex(EmitError, r'0x0c000008: 0xac000034 lies in RAM, in dat_0c000000, a NJS_OBJECT, '
                                               r'whose type gives it a float'):
            rooted({'TEST.BIN': payload + bytes(0x98)})


class Owners(unittest.TestCase):
    # mod, and two, whose span holds 0x0ca00000, where its table names b_value
    FILES = {**MODULE, 'config/two/splits.txt': 'D  0x0ca00000  0x0ca00010\n',
             'config/two/symbols.txt': '0x0ca00000  b_value\n'}

    def test_a_name_no_definition_types_whose_pointers_take_it_as_one_record_is_no_array(self):
        # two's b_value lies in no row, so only its pointers type it, as a motion. A motion is one record, so
        # b_value is no array
        with tree(self.FILES):
            providers = content.Providers(content.Config('mod'))
            self.assertEqual(providers('b_value', 0x0CA00000, {'void', 'NJS_MOTION'}),
                             ('two', 'b_value', 'NJS_MOTION', False))

    def test_a_name_an_image_beside_gives_where_its_span_holds_it_is_that_images(self):
        # and three's table gives it too, where its span doesn't hold it
        files = {**self.FILES, 'config/three/splits.txt': 'D  0x0cb00000  0x0cb00010\n',
                 'config/three/symbols.txt': '0x0ca00000  b_value\n'}
        with tree(files):
            providers = content.Providers(content.Config('mod'))
            self.assertEqual(providers.provider('b_value', 0x0CA00000), ('two', 'b_value'))

    def test_a_name_two_images_beside_give_where_their_spans_hold_it_is_neithers(self):
        files = {**self.FILES, 'config/three/splits.txt': 'D  0x0ca00000  0x0ca00010\n',
                 'config/three/symbols.txt': '0x0ca00000  b_value\n'}
        with tree(files):
            providers = content.Providers(content.Config('mod'))
            self.assertIsNone(providers.provider('b_value', 0x0CA00000))
            with self.assertRaisesRegex(SystemExit, r'mod: b_value, at 0x0ca00000, which its content takes: three '
                                                    r'and two define it there$'):
                providers('b_value', 0x0CA00000, {'void'})
            with self.assertRaisesRegex(SystemExit, r'mod: b_value, at 0x0cc00000, which its content takes: no image '
                                                    r'defines it there$'):
                providers('b_value', 0x0CC00000, {'void'})


# a function, step, and parts, a table a source declares. Each part is a
# step and a number. The first part's number holds the address of thing,
# eight floats
PARTS = {
    'config/1st_read/symbols.txt': '',
    'config/test/splits.txt': ('P  0x0c000000  0x0c000020\nD  0x0c000020  0x0c000060\n'
                               'modules/test/a.c  P  0x0c000000  0x0c000020\n'
                               'gen/test/d_0c000020.c  D  0x0c000020  0x0c000060\n'),
    'config/test/symbols.txt': '0x0c000000  step\n0x0c000020  parts\n0x0c000040  thing\n',
    'modules/test/a.c': '#include "test/a.h"\n\nvoid step(void)\n{\n}\n',
    'modules/test/a.h': 'struct part { void (*step)(void); long var; };\nextern struct part parts[];\n',
    'config/test/data.txt': '0x0c000020  struct part[4]\n0x0c000040  floats[8]\n',
    'TEST.BIN': (words(0x0009000B, BASE + 0x20) + bytes(0x18) + words(BASE, BASE + 0x40, 0, 0) + bytes(0x10)
                 + struct.pack('<8f', *[1.0] * 8)),
}


def parts(files=None):
    with tree(dict(PARTS, **(files or {}))):
        content.gcc_content('TEST.BIN', str(CONTENT_C))
        return CONTENT_C.read_text(), CONTENT_C.with_suffix('.txt').read_text()


class Numbers(unittest.TestCase):
    def test_a_declared_number_that_lies_in_ram_fails(self):
        with self.assertRaisesRegex(EmitError, r'0x0c000024: 0x0c000040 lies in RAM, in parts, a struct part, whose '
                                               r'type gives it a number'):
            parts()

    def test_a_row_that_vouches_makes_its_number_an_address(self):
        text, record = parts({'config/test/data.txt': PARTS['config/test/data.txt'].replace('[4]', '[4]  vouched')})
        self.assertIn('struct part parts[] = {\n    { step, (Sint32)thing },\n    { 0, 0 },\n', text)
        self.assertIn('parts  0x0c000020  32  struct part[]: config/test/data.txt:1\n', record)

    def test_a_declared_float_that_lies_in_ram_fails_where_its_row_vouches(self):
        # the row makes an integer's word an address, and no float's
        files = {'modules/test/a.h': 'struct part { void *thing; float scale; };\nextern struct part parts;\n',
                 'config/test/data.txt': '0x0c000020  struct part  vouched\n0x0c000028  words[6]\n0x0c000040  floats[8]\n',
                 'TEST.BIN': PARTS['TEST.BIN'][:0x20] + words(BASE + 0x40, BASE + 0x40) + PARTS['TEST.BIN'][0x28:]}
        with self.assertRaisesRegex(EmitError, r'0x0c000024: 0x0c000040 lies in RAM, in parts, a struct part, whose '
                                               r'type gives it a float, and nothing says whether it is a pointer$'):
            parts(files)


if __name__ == '__main__':
    unittest.main()
