import contextlib
import io
import struct
import sys
import unittest
from unittest import mock

import census
from image import Item
from support import LOADER_SECTIONS, MODULE, tree
from test_content import compiled

# 1ST_READ.BIN's main.c. It reads hero.PRS to AREA, which area.h defines, and names a land in hero.PRS twice: by its
# address, and as AREA + 0x40. The numbers in its comment and its string are no candidates
MAIN = '''#include "area.h"

/* a comment's 0x0cc00040 is no number */
static const char *name = "0x0cc00040";
struct land_table *hero_lands[] = {(struct land_table *)0x0cc00040, (struct land_table *)(AREA + 0x40)};

void hero_load(void)
{
    loader_read_prs("hero.PRS", AREA);
}
'''
# 1ST_READ.BIN's hero.src. It holds a number past the image, and two in a comment and a string, which are no
# candidates
ASSEMBLY = '''        .DATA.L     H'0CC00080          ; H'0CC00090 is a comment's
        .SDATA      "H'0CC000A0"
'''
CENSUS = '''# the census
src/area.h                   0x0cc00000  1  anchor   AREA      0x0cc00000  0x0cc10000  where hero.PRS loads
src/main.c                   0x0cc00040  2  anchor   AREA      0x0cc00000  0x0cc10000  hero's land, in AREA
config/1st_read/symbols.txt  0x0c900040  1  anchor   shadowed  0x0c900000  0x0c900100  a name past the image
src/hero.src                 0x0cc00080  1  anchor   AREA      0x0cc00000  0x0cc10000  the hero's work in AREA
'''
# support.py's MODULE, a module's tables and sources beside 1ST_READ.BIN's, with hero.PRS and 1ST_READ.BIN's main.c,
# hero.src and census. hero.PRS is an image that loads at AREA, and main.c's stub build has written its .d file
FILES = {**MODULE, 'src/main.c': MAIN, 'src/area.h': '#define AREA ((Uint8 *)0x0cc00000)\n', 'src/hero.src': ASSEMBLY,
         'config/1st_read/splits.txt': (MODULE['config/1st_read/splits.txt']
                                        + 'src/hero.src  P  0x8c0101b0  0x8c0101c0\n'),
         'config/hero/splits.txt': 'P  0x0cc00000  0x0cc00040\nD  0x0cc00040  0x0cc00100\n',
         'config/hero/symbols.txt': '',
         'build/stub/src/main.d': 'build/stub/src/main.obj: src/main.c src/area.h\nsrc/area.h:\n',
         'config/1st_read/census.txt': CENSUS}


def words(*values):
    return struct.pack(f'<{len(values)}I', *values)


def motion(area):
    """A motion file that loads at area, as make decodes it. Two angle keys open it. An NJS_MDATA1 at 0x20 points at
    them, an NJS_MOTION at 0x28 points at the NJS_MDATA1, and an NJS_ACTION at 0x34 points at the motion. The action's
    object is hero_object, one of 1ST_READ.BIN's models, which main.c defines where 1ST_READ.BIN's table puts it."""
    return (words(0, 0, 0, 0, 1, 0, 0, 0) + words(area, 2) + words(area + 0x20, 2) + struct.pack('<HH', 2, 1)
            + words(0x8C1755E4, area + 0x28))


def defined(area, *more):
    """The data.txt of motion(area), and of more's rows past it."""
    rows = ('mkey_a[2]', 'mdata1[1]', 'motion', 'action', *more)
    return ''.join(f'0x{area + offset:08x}  {row}\n' for offset, row in zip((0, 0x20, 0x28, 0x34, 0x3c), rows))


# h_mot.PRS, a motion file that loads at MOTION, as make decodes it
MOTION = 0x0CC80000
H_MOT = motion(MOTION)
# FILES with h_mot.PRS, an image whose one section, D, holds no code. The address of its action's object takes no row,
# since its content places a pointer there
MOTIONS = {**FILES, 'config/h_mot/splits.txt': ('D  0x0cc80000  0x0cc8003c\n'
                                                 'gen/h_mot/d_0cc80000.c  D  0x0cc80000  0x0cc8003c\n'),
           'config/h_mot/symbols.txt': '', 'config/h_mot/data.txt': defined(MOTION),
           'config/1st_read/symbols.txt': MODULE['config/1st_read/symbols.txt'] + '0x8c1755e4  hero_object\n',
           'config/1st_read/splits.txt': (FILES['config/1st_read/splits.txt']
                                          + 'src/main.c  D  0x8c1755e4  0x8c175618\n'),
           'src/main.c': MAIN + '\nNJS_OBJECT hero_object;\n', 'build/gen/img/H_MOT.BIN': H_MOT}


def check(files, *args):
    """What census.py ends with, run with args in a tree of files: 0 where it returns, or the text it exits with."""
    with tree(files), contextlib.redirect_stdout(io.StringIO()), \
            mock.patch.object(sys, 'argv', ['tools/census.py']):
        try:
            census.main(list(args))
        except SystemExit as failed:
            return str(failed)
    return 0


class Check(unittest.TestCase):
    def test_explains_each_number_of_the_sources_their_headers_and_the_table(self):
        self.assertEqual(check(FILES), 0)

    def test_rejects_a_number_no_row_explains(self):
        files = {**FILES, 'src/main.c': MAIN + 'int far = 0x0cc00080;\n'}
        self.assertEqual(check(files), 'census: src/main.c:11: 0x0cc00080, which no row of config/1st_read/census.txt '
                                       'explains')

    def test_rejects_a_row_whose_numbers_are_gone_or_counted_otherwise(self):
        second = ', (struct land_table *)(AREA + 0x40)'
        for old, new, problem in (
                (second, '', 'the row counts 2 of the numbers it spans, and src/main.c holds 1'),
                ('(struct land_table *)0x0cc00040' + second, '0', 'src/main.c holds no number the row spans')):
            with self.subTest(problem=problem):
                files = {**FILES, 'src/main.c': MAIN.replace(old, new)}
                self.assertEqual(check(files), f'census: config/1st_read/census.txt:3: {problem}')

    def test_reads_the_numbers_of_assembly_outside_its_comments_and_strings(self):
        files = {**FILES, 'src/hero.src': ASSEMBLY.replace(".DATA.L     H'0CC00080", ".DATA.L     H'00000080")}
        self.assertEqual(check(files), 'census: config/1st_read/census.txt:5: src/hero.src holds no number the row '
                                       'spans')

    def test_reads_a_macro_whole_where_a_comment_spans_its_lines(self):
        # the splices inside the comment continue the definition, so main.c's AREA + 0x40 is a number past the image
        header = "#define AREA \\\n    /* hero.PRS's \\\n       area */ \\\n    ((Uint8 *)0x0cc00000)\n"
        self.assertEqual(check({**FILES, 'src/area.h': header}), 0)

    def test_reads_the_macros_of_each_compile_apart(self):
        # other.c defines an AREA of its own. main.c's compile reads area.h's alone, so neither sum has two values
        other = '#define AREA ((Uint8 *)0x0cd00000)\nUint8 *other_land = AREA + 0x40;\n'
        files = {**FILES, 'src/other.c': other, 'build/stub/src/other.d': 'build/stub/src/other.obj: src/other.c\n',
                 'config/1st_read/splits.txt': FILES['config/1st_read/splits.txt'] + 'src/other.c  P  0x8c0101c0  '
                                                                                    '0x8c0101d0\n',
                 'config/1st_read/census.txt': CENSUS + 'src/other.c  0x0cd00000  0x0cd00100  2  data  the reason\n'}
        self.assertEqual(check(files), 0)

    def test_takes_no_value_for_a_macro_a_compile_defines_two_ways(self):
        # area.h undefines AREA and defines it again, so main.c's AREA + 0x40 has no one value
        header = '#define AREA ((Uint8 *)0x0cc00000)\n#undef AREA\n#define AREA hero_area\n'
        self.assertEqual(check({**FILES, 'src/area.h': header}),
                         'census: src/main.c:5: AREA + 0x40: a compile that reads src/main.c defines AREA otherwise '
                         'too, so the census takes no value for it')

    def test_reads_the_macros_shc_defines(self):
        # area.h's GCC side defines AREA as a name. The census takes AREA from SHC's side alone, as SHC's compile
        # reads it, so main.c's AREA + 0x40 has one value
        header = ('#ifdef GCC\n#define AREA ((Uint8 *)hero_area)\n#else\n#define AREA ((Uint8 *)0x0cc00000)\n'
                  '#endif\n')
        self.assertEqual(check({**FILES, 'src/area.h': header}), 0)

    def test_takes_a_macro_plus_a_constant_only_as_a_whole_operand_that_steps_by_bytes(self):
        # AREA + 0x40 lies 0x40 bytes past AREA where a cast before AREA makes it an integer or a pointer to a byte
        land = '(struct land_table *)(AREA + 0x40)'
        for use in ('((int)AREA + 0x40)', '((char *)AREA + 0x40)'):
            with self.subTest(use=use):
                self.assertEqual(check({**FILES, 'src/main.c': MAIN.replace(land, use)}), 0)
        # the census takes no value for a sum that moves by ints, or whose macro holds more than a cast of its
        # number. Nor does it take one for a sum that is no whole operand. A cast before AREA joins the sum to the
        # token before the cast, as an operator or another cast's ) is
        never = 'AREA there is neither an integer nor a pointer to a byte, as the census reads it, so it takes no value'
        whole = 'beside it makes it no whole operand, so the census takes no value'
        for header, use, problem in (('#define AREA ((int *)0x0cc00000)\n', land, f'AREA + 0x40: {never}'),
                                     ('#define AREA (hero_scale * 0x0cc00000)\n', land, f'AREA + 0x40: {never}'),
                                     (FILES['src/area.h'], '(AREA + 0x20 * 2)', f'AREA + 0x20: the * {whole}'),
                                     (FILES['src/area.h'], '(*(Uint8 *)AREA + 0x40)', f'AREA + 0x40: the * {whole}'),
                                     (FILES['src/area.h'], '(~(Uint32)AREA + 0x40)', f'AREA + 0x40: the ~ {whole}'),
                                     (FILES['src/area.h'], '((int *)(Uint8 *)AREA + 0x10)',
                                      f'AREA + 0x10: the ) {whole}')):
            with self.subTest(header=header, use=use):
                self.assertEqual(check({**FILES, 'src/area.h': header, 'src/main.c': MAIN.replace(land, use)}),
                                 f'census: src/main.c:5: {problem} for it')

    def test_reads_a_number_a_splice_splits_as_the_number(self):
        # a splice inside far's number joins it, and the number lies on the line it starts on. A splice inside AREA's
        # number joins it too, so main.c's AREA + 0x40 takes its value
        files = {**FILES, 'src/main.c': MAIN + 'int far = 0x0cc0\\\n0080;\n'}
        self.assertEqual(check(files), 'census: src/main.c:11: 0x0cc00080, which no row of '
                                       'config/1st_read/census.txt explains')
        self.assertEqual(check({**FILES, 'src/area.h': '#define AREA ((Uint8 *)0x0cc0\\\n0000)\n'}), 0)

    def test_rejects_a_row_out_of_form(self):
        rows = {'src/a.c  0x0cc00000  1  pinned  the reason': 'not a row',
                'src/a.c  0x0cc00000  01  data  the reason': 'not a row',
                'src/a.c  0x0cc00000  1  anchor  AREA  the reason': 'not a row',
                'src/a.c  0x00010000  1  data  the reason': 'not a span of RAM, in one view',
                'src/a.c  0x0cc00000  0x8cc00010  1  data  the reason': 'not a span of RAM, in one view',
                'src/a.c  0x8c8fd150  0x8c8fd170  1  data  the reason': "on both sides of 1ST_READ.BIN's image's end",
                'src/a.c  0x0cc00000  1  anchor  AREA  0x0cc00004  0x0cc00010  the reason': 'outside its extent',
                'src/a.c  0x0cc00000  1  placed  src/main.c  the reason': "a placed row is config/1st_read/symbols",
                'config/1st_read/symbols.txt  0x0c900040  1  placed  src/none.c  the reason': 'src/none.c: no source',
                'config/1st_read/symbols.txt  0x8c0101a0  1  data  the reason': "in 1ST_READ.BIN's image or below it",
                'build/gen/src/main/hero.inc  0x8c010000  1  data  the reason': "in 1ST_READ.BIN's image or below it",
                'src/area.h  0x0cc00000  0x0cc00008  1  data  the reason': 'meet those of the row at'}
        for row, problem in rows.items():
            with self.subTest(row=row):
                self.assertIn(problem, check({**FILES, 'config/1st_read/census.txt': CENSUS + row + '\n'}))
        # a row that ends where the image does lies in it alone
        row = 'src/a.c  0x8c8fd150  0x8c8fd160  1  data  the reason\n'
        self.assertEqual(check({**FILES, 'config/1st_read/census.txt': CENSUS + row}), 0)

    def test_reads_the_generated_data_only_where_asked(self):
        # main.c's full build reads a body, hero.inc, which holds a number past the image
        files = {**FILES, 'build/src/main.d': 'build/src/main.obj: src/main.c src/area.h build/gen/src/main/hero.inc\n',
                 'build/gen/src/main/hero.inc': '0x0cb50000, 0x00000003,\n',
                 'config/1st_read/census.txt': CENSUS + 'build/gen/src/main/hero.inc  0x0cb50000  1  anchor  '
                                                        'CAMERA  0x0cb50000  0x0cb54000  a camera file\n'}
        self.assertEqual((check(files), check(files, '--generated')), (0, 0))
        files['build/gen/src/main/hero.inc'] = '0x00000003,\n'
        self.assertEqual((check(files), check(files, '--generated')),
                         (0, 'census: config/1st_read/census.txt:6: build/gen/src/main/hero.inc holds no number the '
                             'row spans'))

    def test_needs_each_c_sources_dependency_file(self):
        files = {**FILES}
        del files['build/stub/src/main.d']
        self.assertEqual(check(files), 'census: src/main.c: no .d file under build/ or build/stub/, so build it first')
        # a .d file names its source at least, so an empty one is no .d file
        self.assertEqual(check({**files, 'build/stub/src/main.d': ''}),
                         'census: build/stub/src/main.d: not a .d file: TARGET: PREREQUISITE ...')

    def test_holds_each_row_of_symbols_txt_to_its_form(self):
        files = {**FILES, 'config/1st_read/symbols.txt': MODULE['config/1st_read/symbols.txt'] + '0x0c900044  a  b\n'}
        self.assertEqual(check(files), 'census: config/1st_read/symbols.txt:4: not a row: ADDRESS NAME')


class Disc(unittest.TestCase):
    def test_holds_each_address_a_motion_file_holds_to_a_pointer_its_content_places(self):
        self.assertEqual(check(MOTIONS, '--disc'), 0)
        # the word past the action holds an address in 1ST_READ.BIN's image, and no structure of its content's holds
        # it. Without --disc, the census reads no motion file
        stray = {**MOTIONS, 'build/gen/img/H_MOT.BIN': H_MOT + words(0x8C1755E8),
                 'config/h_mot/splits.txt': MOTIONS['config/h_mot/splits.txt'].replace('0x0cc8003c', '0x0cc80040'),
                 'config/h_mot/data.txt': defined(MOTION, 'pointers[1]')}
        self.assertEqual(check(stray, '--disc'),
                         "census: orig/SONICADV/H_MOT.PRS: the word at offset 0x3c holds 0x8c1755e8, an address in "
                         "1ST_READ.BIN's image, and its content places it as no pointer")
        self.assertEqual(check(stray), 0)
        # the census names each such word of each motion file, in each view, and none at the image's end. A second
        # motion file, i_mot.PRS, loads at 0x0cb00000 and holds one such word too
        strays = {**stray, 'build/gen/img/H_MOT.BIN': H_MOT + words(0x8C1755E8, 0x0C1755E8, 0xAC1755E8, 0x8C8FD160),
                  'config/h_mot/splits.txt': MOTIONS['config/h_mot/splits.txt'].replace('0x0cc8003c', '0x0cc8004c'),
                  'config/h_mot/data.txt': defined(MOTION, 'mixed[4]'),
                  'build/gen/img/I_MOT.BIN': motion(0x0CB00000) + words(0x8C1755EC), 'config/i_mot/symbols.txt': '',
                  'config/i_mot/data.txt': defined(0x0CB00000, 'pointers[1]'),
                  'config/i_mot/splits.txt': ('D  0x0cb00000  0x0cb00040\n'
                                              'gen/i_mot/d_0cb00000.c  D  0x0cb00000  0x0cb00040\n')}
        self.assertEqual(check(strays, '--disc'), '\n'.join(
            f"census: orig/SONICADV/{name}.PRS: the word at offset {offset} holds {word}, an address in 1ST_READ.BIN's "
            "image, and its content places it as no pointer"
            for name, offset, word in (('H_MOT', '0x3c', '0x8c1755e8'), ('H_MOT', '0x40', '0x0c1755e8'),
                                       ('H_MOT', '0x44', '0xac1755e8'), ('I_MOT', '0x3c', '0x8c1755ec'))))

    def test_fails_on_a_motion_file_a_row_vouches_for(self):
        # its content reads the mark, and the pointers the census holds the file to don't
        files = {**MOTIONS, 'build/gen/img/H_MOT.BIN': H_MOT + words(0x8C1755E8),
                 'config/h_mot/splits.txt': MOTIONS['config/h_mot/splits.txt'].replace('0x0cc8003c', '0x0cc80040'),
                 'config/h_mot/data.txt': defined(MOTION, 'pointers[1]  vouched')}
        self.assertEqual(check(files, '--disc'), 'census: config/h_mot/data.txt:5: vouched, in h_mot, whose pointers '
                                                 'the census checks without it')

    def test_fails_where_items_refuses_the_contents_items(self):
        # items that hold a name inside one of their words, which Items refuses
        files = {**MOTIONS, 'config/h_mot/symbols.txt': '0x0cc80002  inner\n'}
        with mock.patch('content.definitions', return_value=[Item(MOTION, MOTION + 0x3c, 'words')]):
            self.assertEqual(check(files, '--disc'),
                             'census: build/gen/img/H_MOT.BIN: 0x0cc80002: inner falls inside Uint32 0x0cc80000')

    def test_reads_only_the_motion_files(self):
        # a file of the disc's other than a motion file, decoded or not, and a track of a .gdi's each hold shared's
        # address. --disc reads none of them
        shared = words(0, 0x8C0101A4, 0)
        files = {**MOTIONS, 'orig/SONICADV/ODD.BIN': shared, 'build/gen/img/ODD.BIN': shared, 'orig/sa103.bin': shared}
        self.assertEqual(check(files, '--disc'), 0)

    def test_needs_each_motion_file_decoded(self):
        files = {**MOTIONS}
        del files['build/gen/img/H_MOT.BIN']
        self.assertEqual(check(files, '--disc'), 'census: build/gen/img/H_MOT.BIN: missing, and make gcc decodes it '
                                                 'from orig/SONICADV/H_MOT.PRS with build/prskit')


# CENSUS with shadowed placed, as main.c defines it under GCC
PLACED = CENSUS.replace('1  anchor   shadowed  0x0c900000  0x0c900100', '1  placed   src/main.c')
# the .d files make gcc writes beside main.c's and a.c's GCC objects
DEPENDS = {'build/gcc/src/main.d': 'build/gcc/src/main.o: src/main.c src/area.h\nsrc/area.h:\n',
           'build/gcc/modules/mod/a.d': 'build/gcc/modules/mod/a.o: modules/mod/a.c\n'}


class Placed(unittest.TestCase):
    def check_gcc(self, source, rows=PLACED, port=None):
        """census.py --gcc's result, as check() gives it, with rows as the census. main.c's GCC object is compiled
        from source, and hero.src's from an empty file, so that it defines no name. With port, an --add gives
        1ST_READ.BIN gcc/src/port.c, whose GCC object is compiled from port."""
        files = {**FILES, **DEPENDS, 'config/1st_read/census.txt': rows, 'src/main.c': source, 'empty.c': ''}
        added = []
        if port is not None:
            files.update({'gcc/src/port.c': port, 'build/gcc/src/port.d': 'build/gcc/src/port.o: gcc/src/port.c\n'})
            added = ['--add', '1st_read', 'gcc/src/port.c']
        with tree(files) as root, contextlib.redirect_stdout(io.StringIO()), \
                mock.patch.object(sys, 'argv', ['tools/census.py']):
            compiled(root, 'src/main.c', 'build/gcc/src/main.o')
            compiled(root, 'empty.c', 'build/gcc/src/hero.o')
            if port is not None:
                compiled(root, 'gcc/src/port.c', 'build/gcc/src/port.o')
            try:
                census.main(['--gcc', *added])
            except SystemExit as failed:
                return str(failed)
        return 0

    def test_explains_a_name_past_the_image_as_a_number_of_its_table(self):
        self.assertEqual(check({**FILES, 'config/1st_read/census.txt': PLACED}), 0)

    def test_holds_each_placed_row_to_its_sources_gcc_object(self):
        self.assertEqual(self.check_gcc('#include <sg_xpt.h>\n\nUint32 shadowed[4];\n'), 0)
        # a static shadowed, or one defined elsewhere, is no global main.o defines
        for declaration in ('static Uint32 shadowed[4];', 'extern Uint32 shadowed[];'):
            source = f'#include <sg_xpt.h>\n\n{declaration}\n\nUint32 *f(void)\n{{\n    return shadowed;\n}}\n'
            with self.subTest(declaration=declaration):
                self.assertEqual(self.check_gcc(source), 'census: config/1st_read/census.txt:4: build/gcc/src/main.o '
                                                      'defines no global shadowed, which config/1st_read/symbols.txt '
                                                      'gives 0x0c900040')

    def test_fails_on_a_placed_row_at_an_address_symbols_txt_names_nothing_at(self):
        rows = PLACED.replace('0x0c900040', '0x0c900044', 1)
        self.assertEqual(self.check_gcc('#include <sg_xpt.h>\n\nUint32 shadowed[4];\n', rows),
                         'census: config/1st_read/census.txt:4: config/1st_read/symbols.txt gives 0x0c900044 no name')

    def test_holds_each_other_row_of_symbols_txt_to_no_gcc_object_that_defines_its_name(self):
        # CENSUS's row of shadowed is an anchor, which keeps its address. Where main.c defines shadowed under GCC, the
        # link places it, so the row fails
        self.assertEqual(self.check_gcc('#include <sg_xpt.h>\n\nUint32 main_other[4];\n', CENSUS), 0)
        self.assertEqual(self.check_gcc('#include <sg_xpt.h>\n\nUint32 shadowed[4];\n', CENSUS),
                         'census: config/1st_read/census.txt:4: build/gcc/src/main.o defines global shadowed, which '
                         "config/1st_read/symbols.txt gives 0x0c900040, so the link places it, and the row's "
                         'disposition is anchor, not placed')
        # a definition in gcc/src/port.c, which an --add gives 1ST_READ.BIN, fails the row too
        other = '#include <sg_xpt.h>\n\nUint32 main_other[4];\n'
        self.assertEqual(self.check_gcc(other, CENSUS, '#include <sg_xpt.h>\n\nUint32 port_other[4];\n'), 0)
        self.assertEqual(self.check_gcc(other, CENSUS, '#include <sg_xpt.h>\n\nUint32 shadowed[4];\n'),
                         'census: config/1st_read/census.txt:4: build/gcc/src/port.o defines global shadowed, which '
                         "config/1st_read/symbols.txt gives 0x0c900040, so the link places it, and the row's "
                         'disposition is anchor, not placed')

    def test_needs_the_gcc_object(self):
        with tree({**FILES, 'config/1st_read/census.txt': PLACED}), contextlib.redirect_stdout(io.StringIO()), \
                mock.patch.object(sys, 'argv', ['tools/census.py']), \
                self.assertRaisesRegex(SystemExit, r'^census: build/gcc/src/main\.o: missing, and make gcc builds it$'):
            census.main(['--gcc'])

    def test_takes_neither_the_generated_data_nor_the_disc(self):
        for args in (['--gcc', '--generated'], ['--gcc', '--disc']):
            with self.subTest(args=args), tree(FILES), contextlib.redirect_stderr(io.StringIO()), \
                    self.assertRaises(SystemExit) as failed:
                census.main(args)
            self.assertEqual(failed.exception.code, 2)


# the tree as make gcc leaves it, but with no name past 1ST_READ.BIN's image in its table, so --gcc reads no object.
# mod's table gives a_init 0x0c900000, and shared 0x0c900104
GCC = {**FILES, **DEPENDS, 'config/1st_read/symbols.txt': '0x8c0101a0  main\n0x8c0101a4  shared\n',
       'config/1st_read/census.txt': ''.join(line for line in CENSUS.splitlines(True)
                                             if not line.startswith('config/'))}
# mod's a.c, which names a_init on GCC's side of its guard, and spells a_init's address on SHC's
GUARDED = '''#ifdef GCC
extern void a_init(void);
#define A_INIT a_init
#else
#define A_INIT ((void (*)(void))0x0c900000)
#endif
void (*a_step)(void) = A_INIT;
'''
UNEXPLAINED = 'census: {}: {}, which no row of config/1st_read/census.txt explains'


class GccSide(unittest.TestCase):
    def test_explains_each_number_of_a_gcc_side_at_a_names_address(self):
        self.assertEqual(check(GCC, '--gcc'), 0)
        # a number at a_init's address or shared's fails, in either view, and on GCC's side of a guard
        for text, line, number in (('void (*a_step)(void) = (void (*)(void))0x0c900000;\n', 1, '0x0c900000'),
                                   ('int *a_shared = (int *)0x8c900104;\n', 1, '0x8c900104'),
                                   (GUARDED.replace('#ifdef', '#ifndef'), 5, '0x0c900000')):
            with self.subTest(text=text):
                self.assertEqual(check({**GCC, 'modules/mod/a.c': text}, '--gcc'),
                                 UNEXPLAINED.format(f'modules/mod/a.c:{line}', number))
        # --gcc reads no guard's shipped side, which SHC alone compiles, and takes no number at no name's address
        for text in (GUARDED, 'int *a_end = (int *)0x0c900100;\n'):
            with self.subTest(text=text):
                self.assertEqual(check({**GCC, 'modules/mod/a.c': text}, '--gcc'), 0)

    def test_holds_a_name_to_its_address_only_in_a_view_of_ram(self):
        # mod's table gives a name 0x9c900200, which lies in no view of RAM, so a_far's 0x0c900200 is no name's
        # address
        files = {**GCC, 'modules/mod/a.c': 'int *a_far = (int *)0x0c900200;\n',
                 'config/mod/symbols.txt': GCC['config/mod/symbols.txt'] + '0x9c900200  a_far_name\n'}
        self.assertEqual(check(files, '--gcc'), 0)

    def test_reads_each_header_the_d_file_names(self):
        files = {**GCC, 'modules/mod/a.h': '#define A_INIT ((void (*)(void))0x0c900000)\n'}
        self.assertEqual(check(files, '--gcc'), 0)
        files['build/gcc/modules/mod/a.d'] = 'build/gcc/modules/mod/a.o: modules/mod/a.c modules/mod/a.h\n'
        self.assertEqual(check(files, '--gcc'), UNEXPLAINED.format('modules/mod/a.h:1', '0x0c900000'))

    def test_reads_the_macros_of_a_c_file_the_d_file_names(self):
        # a.c includes c.c, whose B_BASE makes a.c's B_BASE + 4 shared's address
        files = {**GCC, 'modules/mod/c.c': '#define B_BASE ((Uint8 *)0x0c900100)\n',
                 'modules/mod/a.c': '#include "c.c"\nUint8 *a_shared = B_BASE + 4;\n',
                 'build/gcc/modules/mod/a.d': 'build/gcc/modules/mod/a.o: modules/mod/a.c modules/mod/c.c\n'}
        self.assertEqual(check(files, '--gcc'), UNEXPLAINED.format('modules/mod/a.c:2', '0x0c900104'))

    def test_reads_each_spelling_of_a_header_as_one_file(self):
        # a.c's .d file names a.h as ./modules/mod/a.h, and main.c's as src/../modules/mod/a.h. Both are a.h, whose
        # row explains its number once
        files = {**GCC, 'modules/mod/a.h': '#define A_INIT ((void (*)(void))0x0c900000)\n',
                 'build/gcc/modules/mod/a.d': 'build/gcc/modules/mod/a.o: modules/mod/a.c ./modules/mod/a.h\n',
                 'build/gcc/src/main.d': 'build/gcc/src/main.o: src/main.c src/area.h src/../modules/mod/a.h\n'}
        self.assertEqual(check(files, '--gcc'), UNEXPLAINED.format('modules/mod/a.h:1', '0x0c900000'))
        files['config/1st_read/census.txt'] += 'modules/mod/a.h  0x0c900000  1  data  the reason\n'
        self.assertEqual(check(files, '--gcc'), 0)

    def test_reads_a_src_file_as_assembly_or_the_c_that_replaces_it(self):
        files = {**GCC, 'modules/mod/b.src': "        .DATA.L     H'0C900000\n"}
        self.assertEqual(check(files, '--gcc'), UNEXPLAINED.format('modules/mod/b.src:1', '0x0c900000'))
        files.update({'gcc/modules/mod/b.c': 'int *b_shared = (int *)0x0c900104;\n',
                      'build/gcc/modules/mod/b.d': 'build/gcc/modules/mod/b.o: gcc/modules/mod/b.c\n'})
        self.assertEqual(check(files, '--gcc'), UNEXPLAINED.format('gcc/modules/mod/b.c:1', '0x0c900104'))

    def test_reads_a_source_in_gcc_that_an_add_gives_an_image(self):
        # port.c's header, port.h, is read too, as port.c's .d file names it
        files = {**GCC, 'gcc/src/port.c': '#include "port.h"\nint *port_shared = PORT_SHARED;\n',
                 'gcc/src/port.h': '#define PORT_SHARED ((int *)0x0c900104)\n',
                 'build/gcc/src/port.d': 'build/gcc/src/port.o: gcc/src/port.c gcc/src/port.h\n'}
        self.assertEqual(check(files, '--gcc'), 0)
        self.assertEqual(check(files, '--gcc', '--add', '1st_read', 'gcc/src/port.c'),
                         UNEXPLAINED.format('gcc/src/port.h:1', '0x0c900104'))
        del files['build/gcc/src/port.d']
        self.assertEqual(check(files, '--gcc', '--add', '1st_read', 'gcc/src/port.c'),
                         'census: build/gcc/src/port.d: missing, and make gcc writes it')
        self.assertEqual(check(files, '--gcc', '--add', 'nine', 'gcc/src/port.c'),
                         'census: nine: no such image, a directory in config/')

    def test_explains_a_number_by_the_rows_of_the_file_that_spells_it(self):
        # hero's table gives hero_step AREA's address, 0x0cc00000. area.h's row explains area.h's number there, not
        # a.c's. --gcc alone counts a.c's row, as the census without it doesn't read a.c
        files = {**GCC, 'config/hero/symbols.txt': '0x0cc00000  hero_step\n',
                 'modules/mod/a.c': 'Uint8 *a_area = (Uint8 *)0x0cc00000;\n'}
        self.assertEqual(check(files, '--gcc'), UNEXPLAINED.format('modules/mod/a.c:1', '0x0cc00000'))
        files['config/1st_read/census.txt'] += ('modules/mod/a.c  0x0cc00000  1  anchor  AREA  0x0cc00000  0x0cc10000  '
                                                'where hero.PRS loads\n')
        self.assertEqual((check(files), check(files, '--gcc')), (0, 0))
        for text, problem in (('Uint8 *a_area = (Uint8 *)0x0cc00000, *a_top = (Uint8 *)0x0cc00000;\n',
                               'the row counts 1 of the numbers it spans, and modules/mod/a.c holds 2'),
                              ('Uint8 *a_area;\n', 'modules/mod/a.c holds no number the row spans')):
            with self.subTest(problem=problem):
                self.assertEqual(check({**files, 'modules/mod/a.c': text}, '--gcc'),
                                 f'census: config/1st_read/census.txt:5: {problem}')

    def test_reads_the_numbers_in_1st_read_bins_image_too(self):
        # the link lays 1ST_READ.BIN out anew too, so a number at main's address or at shared's fails. mod's a.c
        # holds main's, 0x8c0101a0, and GCC's side of main.c's guard holds shared's, 0x8c0101a4
        gcc_only = MAIN + '#ifdef GCC\nint *main_shared = (int *)0x8c0101a4;\n#endif\n'
        for path, text, line, number in (
                ('modules/mod/a.c', 'void (*a_step)(void) = (void (*)(void))0x8c0101a0;\n', 1, '0x8c0101a0'),
                ('src/main.c', gcc_only, 12, '0x8c0101a4')):
            with self.subTest(path=path):
                files = {**GCC, path: text}
                self.assertEqual(check(files, '--gcc'), UNEXPLAINED.format(f'{path}:{line}', number))
                # a row of the number's file explains it, and --gcc alone counts the row
                files['config/1st_read/census.txt'] += f'{path}  {number}  1  data  the reason\n'
                self.assertEqual((check(files), check(files, '--gcc')), (0, 0))
        files['src/main.c'] = MAIN
        self.assertEqual(check(files, '--gcc'), 'census: config/1st_read/census.txt:5: src/main.c holds no number '
                                                'the row spans')

    def test_counts_the_row_of_a_header_only_other_images_read(self):
        files = {**GCC, 'modules/mod/a.h': '#define A_INIT ((void (*)(void))0x0c900000)\n',
                 'build/gcc/modules/mod/a.d': 'build/gcc/modules/mod/a.o: modules/mod/a.c modules/mod/a.h\n'}
        files['config/1st_read/census.txt'] += 'modules/mod/a.h  0x0c900000  1  data  the reason\n'
        self.assertEqual((check(files), check(files, '--gcc')), (0, 0))
        # the row fails once a.h holds no number, or once no compile reads a.h
        self.assertEqual(check({**files, 'modules/mod/a.h': ''}, '--gcc'),
                         'census: config/1st_read/census.txt:5: modules/mod/a.h holds no number the row spans')
        unread = {**files, 'build/gcc/modules/mod/a.d': 'build/gcc/modules/mod/a.o: modules/mod/a.c\n'}
        self.assertEqual(check(unread, '--gcc'), "census: config/1st_read/census.txt:5: the row is --gcc's, and no "
                                                 "image's GCC side reads modules/mod/a.h")

    def test_counts_the_rows_of_the_files_only_1st_read_bins_gcc_side_reads(self):
        # gcc/src/hero.c replaces hero.src and reads gcc/src/hero.h. The census without --gcc reads neither, so --gcc
        # counts their rows, though they lie past the image
        files = {**GCC, 'gcc/src/hero.c': '#include "hero.h"\nint *hero_shared = (int *)0x0c900104;\n',
                 'gcc/src/hero.h': '#define HERO_INIT ((void (*)(void))0x0c900000)\n',
                 'build/gcc/src/hero.d': 'build/gcc/src/hero.o: gcc/src/hero.c gcc/src/hero.h\n'}
        rows = 'gcc/src/hero.c  0x0c900104  1  data  the reason\ngcc/src/hero.h  0x0c900000  1  data  the reason\n'
        files['config/1st_read/census.txt'] += rows
        self.assertEqual((check(files), check(files, '--gcc')), (0, 0))
        twice = '#include "hero.h"\nint *hero_shared = (int *)0x0c900104, *hero_top = (int *)0x0c900104;\n'
        for path, text, problem in (
                ('gcc/src/hero.c', twice, '5: the row counts 1 of the numbers it spans, and gcc/src/hero.c holds 2'),
                ('gcc/src/hero.h', '', '6: gcc/src/hero.h holds no number the row spans')):
            with self.subTest(path=path):
                self.assertEqual(check({**files, path: text}, '--gcc'), f'census: config/1st_read/census.txt:{problem}')

    def test_needs_each_images_symbols_txt(self):
        # --gcc reads every image's symbols.txt for its names, LOADER.BIN's too, though LOADER.BIN's other tables
        # need none
        files = {**GCC, 'config/loader/splits.txt': ''.join(f'{row}\n' for row in LOADER_SECTIONS)}
        self.assertEqual(check(files, '--gcc'), 'census: config/loader/symbols.txt: No such file or directory')
        self.assertEqual(check({**files, 'config/loader/symbols.txt': ''}, '--gcc'), 0)

    def test_needs_each_d_file(self):
        files = {**GCC}
        del files['build/gcc/modules/mod/a.d']
        self.assertEqual(check(files, '--gcc'), 'census: build/gcc/modules/mod/a.d: missing, and make gcc writes it')
        # --gcc needs the SHC .d files of 1ST_READ.BIN's C sources too, which name the files whose rows the census
        # without --gcc counts
        del files['build/stub/src/main.d']
        self.assertEqual(check(files, '--gcc'), 'census: src/main.c: no .d file under build/ or build/stub/, so build '
                                                'it first')


if __name__ == '__main__':
    unittest.main()
