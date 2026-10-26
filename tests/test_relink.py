import contextlib
import io
import re
import unittest
from pathlib import Path
from unittest import mock

import config
import link
import relink
from config import Config
from support import MODULE, tree
from test_link import END, HEADER, entries, record, section, srecord


def header(kind, module):
    # the unit's or the module's header as R32 writes it, each naming the module once
    name = bytes([len(module)]) + module.encode()
    if kind == 0x84:
        return record(0x84, b'(261023223641\0\1\x000200\x08  ' + bytes(5) + name + b'\x02SH')
    return record(0x86, b'@\0\4\0 \0\6' + name + b'\x04C_SH261023223641')


def definition(name, value, absolute=False):
    # a definition entry: the section's number, an attribute, the value and the name
    head = b'\0\0\x40\x04' if absolute else b'\0\0\x20'
    return head + value.to_bytes(4, 'big') + bytes([len(name)]) + name.encode()


def obj(module, sections, takes=(), defines=()):
    """An object as R32 writes one: module's headers, sections, a map of
    (size, alignment, code) by name, the names it takes from outside and the
    entries it defines."""
    table = b''.join(section(name.encode(), size, alignment, not code)
                     for name, (size, alignment, code) in sections.items())
    parts = [record(0x80, bytes(30)), header(0x84, module), header(0x86, module), record(0x88, table)]
    if takes:
        parts.append(record(0x8C, entries(*(name.encode() for name in takes))))
    if defines:
        parts.append(record(0x94, b''.join(defines)))
    return b''.join([*parts, record(0xFF)])


# an image whose sections disagree with table order: P places a.c, b.c and two/a.c, D c.c then b.c, and B c.c then
# a.c, so the objects link as c.c, a.c, b.c, two/a.c. b.c's code ends 8 bytes short of 32, where the linker's fill
# runs to two/a.c's; its file ends with D, and B lies past it. a.c's bss starts 8 bytes past c.c's on 8: no object
# lies there. c_static lies in c.c's data, which doesn't define it, and b_free in the bss past a.c's
SPLITS = ('P  0x0c900000  0x0c900060\n'
          'D  0x0c900060  0x0c900070\n'
          'B  0x0c900070  0x0c9000a0\n'
          'modules/img/a.c  P  0x0c900000  0x0c900020\n'
          'modules/img/a.c  B  0x0c900080  0x0c900088\n'
          'modules/img/b.c  P  0x0c900020  0x0c900038\n'
          'modules/img/b.c  D  0x0c900068  0x0c900070\n'
          'modules/img/c.c  D  0x0c900060  0x0c900068\n'
          'modules/img/c.c  B  0x0c900070  0x0c900078\n'
          'modules/two/a.c  P  0x0c900040  0x0c900060\n'
          'fill  0xff\n')
P, D, B = (0x20, 32, True), (8, 8, False), (8, 8, False)
OBJECTS = {
    'modules/img/a.c': obj('a', {'P': P, 'B': B}, ('_main', '_c_static', '_b_step'), [definition('_a_init', 0)]),
    'modules/img/b.c': obj('b', {'P': (0x18, 32, True), 'D': D, 'C': (0, 8, False)}, ('_b_free', '_shared'),
                           [definition('_b_step', 0)]),
    'modules/img/c.c': obj('c', {'D': D, 'B': B}),
    'modules/two/a.c': obj('a', {'P': P}, ('_a_init',)),
}
IMAGE = {
    'config/1st_read/splits.txt': MODULE['config/1st_read/splits.txt'],
    'config/1st_read/symbols.txt': MODULE['config/1st_read/symbols.txt'],
    'config/img/splits.txt': SPLITS,
    'config/img/symbols.txt': ('0x0c900000  a_init\n0x0c900020  b_step\n0x0c900064  c_static\n'
                               '0x0c900090  b_free\n'),
    'src/main.c': '',
    **{source: '' for source in OBJECTS},
    **{str(Path('build', source).with_suffix('.obj')): data for source, data in OBJECTS.items()},
}
ORDER = ['modules/img/c.c', 'modules/img/a.c', 'modules/img/b.c', 'modules/two/a.c']
# what the whole link writes of each object: a.c's code from 1, b.c's from 0x41, two/a.c's from 0x81, and c.c's and
# b.c's data from 0xc1; the file is P and D, and B lies past it
WRITTEN = {**{0x0C900000 + at: 1 + at for at in range(0x20)}, **{0x0C900020 + at: 0x41 + at for at in range(0x18)},
           **{0x0C900040 + at: 0x81 + at for at in range(0x20)}, **{0x0C900060 + at: 0xC1 + at for at in range(0x10)}}
SPLICED = (bytes(range(1, 0x21)) + bytes(range(0x41, 0x59)) + b'\xff' * 8 + bytes(range(0x81, 0xA1))
           + bytes(range(0xC1, 0xD1)))


class Order(unittest.TestCase):
    def test_keeps_each_section_in_its_rows_order(self):
        with tree(IMAGE):
            self.assertEqual(relink.order(Config('img')), ORDER)

    def test_keeps_table_order_where_no_section_orders_two_objects(self):
        # a.c places no bss, so no section orders a.c and c.c: a.c, first in the table, links first
        files = dict(IMAGE, **{'config/img/splits.txt': SPLITS.replace('modules/img/a.c  B  0x0c900080  0x0c900088\n',
                                                                        '')})
        with tree(files):
            self.assertEqual(relink.order(Config('img')), ['modules/img/a.c', 'modules/img/c.c', 'modules/img/b.c',
                                                           'modules/two/a.c'])

    def test_rejects_sections_that_order_two_objects_both_ways(self):
        # P places a.c before b.c, and D b.c's data before a.c's
        splits = ('P  0x0c900000  0x0c900040\nD  0x0c900040  0x0c900050\n'
                  'modules/img/a.c  P  0x0c900000  0x0c900020\nmodules/img/a.c  D  0x0c900048  0x0c900050\n'
                  'modules/img/b.c  P  0x0c900020  0x0c900040\nmodules/img/b.c  D  0x0c900040  0x0c900048\n')
        with tree(dict(IMAGE, **{'config/img/splits.txt': splits})), self.assertRaises(SystemExit) as failed:
            relink.order(Config('img'))
        message = str(failed.exception)
        self.assertIn("config/img/splits.txt: no one order of the objects keeps each section in its rows' order: ",
                      message)
        self.assertIn('modules/img/a.c goes before modules/img/b.c in P', message)
        self.assertIn('modules/img/b.c goes before modules/img/a.c in D', message)


def definitions(*parts):
    with tree({'x.obj': obj('x', {}, defines=parts)}):
        return relink.definitions(Path('x.obj'))


class Definitions(unittest.TestCase):
    def test_reads_each_name_defined_in_a_section_or_at_an_address(self):
        self.assertEqual(definitions(definition('_a_init', 0x40), definition('_main', 0x8C0101A0, absolute=True),
                                     definition('_b', 8)), {'_a_init', '_main', '_b'})

    def test_rejects_definitions_that_dont_parse(self):
        for cut in (definition('_a_init', 0)[:-1], definition('_main', 0x8C0101A0, absolute=True)[:5]):
            with self.subTest(cut=cut), self.assertRaisesRegex(SystemExit, r"x\.obj: the definitions at 0x[0-9a-f]+ "
                                                                           r"don't parse"):
                definitions(definition('_b', 8), cut)


class Renamed(unittest.TestCase):
    def test_names_the_module_apart_in_both_headers(self):
        with tree({'a.obj': OBJECTS['modules/two/a.c']}):
            data = relink.renamed(Path('a.obj'), 'a', '1')
            Path('1.obj').write_bytes(data)
            # each record keeps its size and sums to 0xff, and the object keeps its sections and the names it takes
            self.assertEqual(len(data), len(OBJECTS['modules/two/a.c']))
            heads = [body for _, kind, body in link.runs(Path('1.obj')) if kind in relink.HEADERS]
            self.assertEqual(link.section_table(Path('1.obj')), link.section_table(Path('a.obj')))
            self.assertEqual(link.references(Path('1.obj')), {'_a_init'})
        self.assertEqual([head.count(b'\x011') for head in heads], [1, 1])
        self.assertEqual([head.count(b'\x01a') for head in heads], [0, 0])

    def test_rejects_a_header_that_doesnt_name_the_module_once(self):
        # a's unit header also holds a 1-byte name that is "a", or its module header names b alone
        unit = header(0x84, 'a')
        twice = record(0x84, unit[2:-1] + b'\x01a')
        alone = obj('a', {'P': P})
        for data in (alone.replace(unit, twice), alone.replace(header(0x86, 'a'), header(0x86, 'b'))):
            with self.subTest(data=data), tree({'a.obj': data}), self.assertRaisesRegex(
                    SystemExit, r"a\.obj: its header at 0x[0-9a-f]+ doesn't name its module once"):
                relink.renamed(Path('a.obj'), 'a', '1')

    def test_rejects_an_object_without_both_headers(self):
        alone = obj('a', {'P': P})
        with tree({'a.obj': alone.replace(header(0x86, 'a'), b'')}), self.assertRaisesRegex(
                SystemExit, r'a\.obj: 2 headers name its module, and it has 1$'):
            relink.renamed(Path('a.obj'), 'a', '1')

    def test_rejects_a_record_it_cant_step_past(self):
        # a record of size 0 would hold the walk at its offset
        data = OBJECTS['modules/two/a.c']
        with tree({'a.obj': data[:-3] + bytes(2) + data[-3:]}), self.assertRaisesRegex(
                SystemExit, rf'a\.obj: the record at {len(data) - 3:#x} has size 0'):
            relink.renamed(Path('a.obj'), 'a', '1')

    def test_names_a_module_apart_as_long_as_its_name(self):
        self.assertEqual(relink.apart('draw', {'draw', 'dra1'}), 'dra2')
        self.assertEqual(relink.apart('a', {'a', '1'}), '2')
        self.assertEqual(relink.apart('edit', {'edit'}), 'edi1')
        with self.assertRaisesRegex(SystemExit, r'a: each module name of its length that ends in a number is taken$'):
            relink.apart('a', set('a0123456789'))


def declared():
    return {source: link.section_table(Path('build', source).with_suffix('.obj')) for source in OBJECTS}


STARTS = {'P': 0x0C900000, 'D': 0x0C900060, 'B': 0x0C900070}


class Reserved(unittest.TestCase):
    def test_reserves_bss_where_no_object_lies(self):
        # c.c's bss ends at 0x0c900078, which a.c's on 8 would follow
        with tree(IMAGE):
            self.assertEqual(relink.holes(Config('img'), declared(), STARTS),
                             [('B', 0x0C900078, 0x0C900080, 'modules/img/a.c')])

    def test_reserves_no_bss_an_object_follows_on_its_alignment(self):
        # c.c's 6 bytes of bss end at 0x0c900076, and a.c's on 8 follow at 0x0c900078
        splits = SPLITS.replace('c.c  B  0x0c900070  0x0c900078', 'c.c  B  0x0c900070  0x0c900076').replace(
            'a.c  B  0x0c900080  0x0c900088', 'a.c  B  0x0c900078  0x0c900080')
        with tree(dict(IMAGE, **{'config/img/splits.txt': splits})):
            objects = declared()
            objects['modules/img/c.c']['B'] = (6, 8, False)
            self.assertEqual(relink.holes(Config('img'), objects, STARTS), [])

    def test_reserves_bss_before_a_sections_first_object(self):
        splits = SPLITS.replace('c.c  B  0x0c900070  0x0c900078', 'c.c  B  0x0c900078  0x0c900080')
        with tree(dict(IMAGE, **{'config/img/splits.txt': splits})):
            self.assertEqual(relink.holes(Config('img'), declared(), STARTS),
                             [('B', 0x0C900070, 0x0C900078, 'modules/img/c.c')])

    def test_rejects_a_bss_row_that_names_no_section_of_its_object(self):
        with tree(IMAGE):
            objects = declared()
            del objects['modules/img/a.c']['B']
            with self.assertRaisesRegex(SystemExit, r'modules/img/a\.c: its B row names no section of its object'):
                relink.holes(Config('img'), objects, STARTS)

    def test_reserves_the_rest_of_an_extent(self):
        # MOVIE.BIN's extents, with a PSGSFD00 whose extent ends 0x12 bytes past its pieces, and a PSGSFD01 whose
        # pieces take 12 bytes less than its extent, but which starts past the extent's start, so the extent doesn't
        # bound it
        extents = (('PSGSFD00', 0x8CEB0BE0, 0x8CEB25D2), ('PSGSFD01', 0x8CEB25D4, 0x8CEB2BE0))
        pieces = {'PSGSFD00': [(0x19E0, 4, True)], 'PSGSFD01': [(0x600, 4, True)]}
        starts = {'PSGSFD00': 0x8CEB0BE0, 'PSGSFD01': 0x8CEB25D8}
        with mock.patch.object(Config, 'extents', lambda config: extents):
            self.assertEqual(relink.tails(Config.__new__(Config), pieces, starts),
                             [('PSGSFD00', 0x8CEB25C0, 0x8CEB25D2, True)])

    def test_aligns_no_empty_piece(self):
        # an empty piece on 32 after 0x19f2 bytes leaves the section's end at its extent's, not at 0x8ceb25e0
        extents = (('PSGSFD00', 0x8CEB0BE0, 0x8CEB25D2),)
        pieces = {'PSGSFD00': [(0x19F2, 4, True), (0, 32, True)]}
        with mock.patch.object(Config, 'extents', lambda config: extents):
            self.assertEqual(relink.tails(Config.__new__(Config), pieces, {'PSGSFD00': 0x8CEB0BE0}), [])

    def test_rejects_a_section_past_its_extent(self):
        config = Config.__new__(Config)
        config.splits = Path('config/movie/splits.txt')
        with mock.patch.object(Config, 'extents', lambda config: (('PSGSFD00', 0x8CEB0BE0, 0x8CEB25D2),)), \
                self.assertRaisesRegex(SystemExit, r"config/movie/splits\.txt: the whole link's PSGSFD00 runs to "
                                                   r'0x8ceb25d4, past 0x8ceb25d2, where tools/config\.py ends it'):
            relink.tails(config, {'PSGSFD00': [(0x19F4, 4, True)]}, {'PSGSFD00': 0x8CEB0BE0})

    def test_reserves_from_a_start_rounded_up_to_4(self):
        config = Config.__new__(Config)
        config.splits = Path('config/img/splits.txt')
        self.assertEqual(relink.reserve(config, 'B', 0x0C900076, 0x0C900080), ('B', 8, 4, False))
        with self.assertRaisesRegex(SystemExit, r'config/img/splits\.txt: B 0x0c900079 to 0x0c90007c is too short to '
                                                r'reserve on 4'):
            relink.reserve(config, 'B', 0x0C900079, 0x0C90007C)


class Bindings(unittest.TestCase):
    def bindings(self, files):
        with tree(files):
            config = Config('img')
            objects = {source: Path('build', source).with_suffix('.obj') for source in OBJECTS}
            return relink.bindings(config, objects, link.symbols(config))

    def test_binds_each_name_no_object_defines(self):
        # main and shared are 1ST_READ.BIN's; c_static lies in c.c's data and b_free in bss no object holds.
        # a.c defines a_init and b.c b_step
        bound, inside = self.bindings(IMAGE)
        self.assertEqual(bound, [(0x0C900064, 'c_static'), (0x0C900090, 'b_free'), (0x8C0101A0, 'main'),
                                 (0x8C0101A4, 'shared')])
        self.assertEqual(inside, {'c_static': 0x0C900064, 'b_free': 0x0C900090})

    def test_binds_by_address_the_names_from_the_spans_start_up_to_its_end(self):
        # c.c takes img_start, at the span's start, and img_end, at its end
        files = dict(IMAGE, **{'config/img/symbols.txt': (IMAGE['config/img/symbols.txt']
                                                          + '0x0c900000  img_start\n0x0c9000a0  img_end\n'),
                               'build/modules/img/c.obj': obj('c', {'D': D, 'B': B}, ('_img_start', '_img_end'))})
        _, inside = self.bindings(files)
        self.assertEqual(inside, {'c_static': 0x0C900064, 'b_free': 0x0C900090, 'img_start': 0x0C900000})

    def test_rejects_a_name_the_tables_dont_give(self):
        files = dict(IMAGE, **{'config/img/symbols.txt': IMAGE['config/img/symbols.txt'].replace('b_free', 'b_freed')})
        with self.assertRaisesRegex(SystemExit, r"modules/img/b\.c: takes b_free, which no object of img defines and "
                                                r"its tables don't name"):
            self.bindings(files)


class Image(unittest.TestCase):
    def test_fills_what_the_link_leaves_unwritten_and_stores_no_bss_past_the_file(self):
        with tree(IMAGE):
            self.assertEqual(relink.image(Config('img'), WRITTEN), SPLICED)

    def test_holds_0_in_bss_the_file_stores(self):
        # D past B: B's bytes are 0, and the gap after it, between sections, the fill byte
        splits = ('P  0x0c900000  0x0c900020\nB  0x0c900020  0x0c900028\nD  0x0c900030  0x0c900038\n'
                  'modules/img/a.c  P  0x0c900000  0x0c900020\nmodules/img/c.c  B  0x0c900020  0x0c900028\n'
                  'modules/img/c.c  D  0x0c900030  0x0c900038\nfill  0xff\n')
        with tree(dict(IMAGE, **{'config/img/splits.txt': splits})):
            image = relink.image(Config('img'), {0x0C900000: 1, 0x0C900030: 2})
        self.assertEqual(image, b'\1' + b'\xff' * 0x1F + bytes(8) + b'\xff' * 8 + b'\2' + b'\xff' * 7)

    def test_rejects_a_byte_written_outside_the_file(self):
        with tree(IMAGE), self.assertRaisesRegex(SystemExit, r'img: the whole link writes 2 bytes outside its file, '
                                                             r'from 0x0c900070'):
            relink.image(Config('img'), {**WRITTEN, 0x0C900070: 0, 0x0C900090: 0})


def words(*values):
    return b''.join(value.to_bytes(4, 'little') for value in values)


class Shifted(unittest.TestCase):
    # img's file holds 28 words, P's from 0x0c900000 and D's from 0x0c900060; its span runs on to B's end, 0x0c9000a0
    def shifted(self, image, *links, files=IMAGE):
        with tree(files):
            return relink.shifted(Config('img'), words(*image), [words(*each) for each in links])

    def moved(self, image, *changes):
        """image's words as each link at SHIFTS's shifts holds them, where changes give each changed word as
        (index, its value under the first shift, under the second)."""
        links = [list(image), list(image)]
        for index, *values in changes:
            for each, value in zip(links, values):
                each[index] = value
        return links

    def test_counts_the_words_that_move_and_lists_those_that_keep_a_value_in_the_span(self):
        # the first and sixth words point into the image; 0x0c900000, the span's start, lies in the span and
        # 0x8c90009c in its mirror, and 0x0c9000a0, its end, 0x8c9000a0, its end's mirror, and 7 in neither. Linked at
        # 0x8c900000, img holds each address at its mirror
        mirrored = {path: IMAGE[path].replace('0x0c9', '0x8c9') for path in ('config/img/splits.txt',
                                                                             'config/img/symbols.txt')}
        for files, mirror in ((IMAGE, 0), ({**IMAGE, **mirrored}, relink.MIRROR)):
            with self.subTest(base=0x0C900000 ^ mirror):
                def at(value):
                    return value ^ mirror

                image = [at(0x0C900020), at(0x0C900000), at(0x8C90009C), at(0x0C9000A0), 7, at(0x0C900060),
                         at(0x8C9000A0), *[0] * 21]
                links = self.moved(image, (0, *(image[0] + shift for shift in relink.SHIFTS)),
                                   (5, *(image[5] + shift for shift in relink.SHIFTS)))
                self.assertEqual(self.shifted(image, *links, files=files),
                                 (2, [(at(0x0C900004), at(0x0C900000), 'modules/img/a.c'),
                                      (at(0x0C900008), at(0x8C90009C), 'modules/img/a.c')]))

    def test_rejects_a_word_that_moves_by_other_than_the_shift(self):
        # b.c's word at 0x0c900024 moves 4 bytes too far under the first shift, the fill's at 0x0c90003c under the
        # first alone, and two/a.c's at 0x0c900040 under the second alone
        first, second = relink.SHIFTS
        image = [0] * 28
        wrong = (((9, first + 4, second), r'0x0c900024, which modules/img/b\.c writes, holds 0x00000000, and '
                                          r'0x01234564 and 0x02468ac0'),
                 ((15, first, 0), r"0x0c90003c, which the linker's fill in P writes, holds 0x00000000, and 0x01234560 "
                                  r'and 0x00000000'),
                 ((16, 0, second), r'0x0c900040, which modules/two/a\.c writes, holds 0x00000000, and 0x00000000 and '
                                   r'0x02468ac0'))
        for change, named in wrong:
            with self.subTest(change=change), \
                    self.assertRaisesRegex(SystemExit, rf'^.*: img: its word at {named} at the shifts$'):
                self.shifted(image, *self.moved(image, change))
        # of several, relink names the first and counts them all
        with self.assertRaisesRegex(SystemExit, r'^.*: img: 3 of its words move, but not by each shift\n'
                                                rf'.*: img: its word at {wrong[0][1]} at the shifts$'):
            self.shifted(image, *self.moved(image, *(change for change, _ in wrong)))

    def test_rejects_a_word_that_holds_half_an_address(self):
        # b_step's address, 0x0c900020, by its low half or its high half, beside 0x1234: under each shift, the half
        # moves, but by less than the shift
        for half in (0, 16):
            with self.subTest(half=half):
                def held(address):
                    return 0x12340000 | address >> half & 0xFFFF

                image = [held(0x0C900020), *[0] * 27]
                links = self.moved(image, (0, *(held(0x0C900020 + shift) for shift in relink.SHIFTS)))
                with self.assertRaisesRegex(SystemExit, r'img: its word at 0x0c900000, which modules/img/a\.c writes'):
                    self.shifted(image, *links)

    def test_keeps_every_alignment_and_moves_both_halves_of_an_address(self):
        # an object aligns a section to 32 at most
        self.assertEqual(len(set(relink.SHIFTS)), 2)
        for shift in relink.SHIFTS:
            self.assertEqual((shift % 32, bool(shift & 0xFFFF), bool(shift >> 16)), (0, True, True))

    def test_reads_a_last_word_short_of_4_bytes_as_if_zeros_followed(self):
        self.assertEqual(relink.words(b'\1\2\3\4\5'), (0x04030201, 5))


class Reviewed(unittest.TestCase):
    # a.c's word at 0x0c900004 and c.c's at 0x0c900064 keep a value in img's span
    KEPT = [(0x0C900004, 0x0C900000, 'modules/img/a.c'), (0x0C900064, 0x0C900040, 'modules/img/c.c')]
    ROWS = [(0x0C900000, 0x0C900008, 'two instructions', 'config/kept.txt:1'),
            (0x0C900064, 0x0C900068, 'a number', 'config/kept.txt:2')]

    def reviewed(self, rows):
        with tree(IMAGE):
            return relink.reviewed(Config('img'), self.KEPT, rows)

    def test_gives_each_word_the_reason_of_the_row_that_holds_it(self):
        self.assertEqual(self.reviewed(self.ROWS), [(0x0C900004, 0x0C900000, 'modules/img/a.c', 'two instructions'),
                                                    (0x0C900064, 0x0C900040, 'modules/img/c.c', 'a number')])

    def test_rejects_each_word_no_row_holds(self):
        with self.assertRaisesRegex(SystemExit, r'^.*: img: its word at 0x0c900004, which modules/img/a\.c writes, '
                                                r'keeps 0x0c900000, in its span, and config/kept\.txt gives no reason\n'
                                                r'.*: img: its word at 0x0c900064, which modules/img/c\.c writes, '
                                                r'keeps 0x0c900040, in its span, and config/kept\.txt gives no '
                                                r'reason$'):
            self.reviewed([])

    def test_holds_no_word_at_a_rows_end(self):
        with self.assertRaisesRegex(SystemExit, r'^.*: img: its word at 0x0c900004, which modules/img/a\.c writes, '
                                                r'keeps 0x0c900000, in its span, and config/kept\.txt gives no reason\n'
                                                r'.*: config/kept\.txt:1: no word of img from 0x0c900000 to 0x0c900004 '
                                                r'keeps a value in its span$'):
            self.reviewed([(0x0C900000, 0x0C900004, 'an instruction', 'config/kept.txt:1'), self.ROWS[1]])

    def test_rejects_a_row_that_holds_no_word(self):
        with self.assertRaisesRegex(SystemExit, r'^.*: config/kept\.txt:3: no word of img from 0x0c900068 to '
                                                r'0x0c90006c keeps a value in its span$'):
            self.reviewed([*self.ROWS, (0x0C900068, 0x0C90006C, 'a number', 'config/kept.txt:3')])


class Compare(unittest.TestCase):
    def compare(self, image):
        with tree(dict(IMAGE, **{'spliced.bin': SPLICED})):
            relink.compare(Config('img'), image, Path('spliced.bin'))

    def test_takes_an_image_equal_to_the_splice(self):
        self.compare(SPLICED)

    def test_names_the_first_byte_that_differs_and_its_writer(self):
        for at, writer in ((0x24, 'modules/img/b.c'), (0x3A, "the linker's fill in P"), (0x64, 'modules/img/c.c')):
            image = SPLICED[:at] + bytes([SPLICED[at] ^ 1]) + SPLICED[at + 1:-1] + b'\0'
            with self.subTest(writer=writer), self.assertRaisesRegex(
                    SystemExit, rf'spliced\.bin: its byte at 0x{0x0C900000 + at:08x}, which {re.escape(writer)} '
                                rf'writes, is 0x{SPLICED[at]:02x}, and the whole link of img writes '
                                rf'0x{SPLICED[at] ^ 1:02x}$'):
                self.compare(image)

    def test_names_where_the_shorter_ends(self):
        # the splice's last byte is b.c's, at 0x0c90006f: the whole link ends before it, or writes one byte past it
        with self.assertRaisesRegex(SystemExit, r'spliced\.bin: its byte at 0x0c90006f, which modules/img/b\.c writes, '
                                                r'is 0xd0, and the whole link of img ends before it$'):
            self.compare(SPLICED[:-1])
        with self.assertRaisesRegex(SystemExit, r'spliced\.bin: ends at 0x0c900070, and the whole link of img writes '
                                                r'0x01 there$'):
            self.compare(SPLICED + b'\1')


def listing(*names):
    """A page of the externally defined symbols list of lnk's map, for names, each (name, address, type) with the
    name as the object spells it. A name past 32 characters takes a line of its own, and its address the next."""
    lines = ['', '               H SERIES LINKAGE EDITOR Ver. 6.0A       PAGE :     1', '',
             '             ***  LINKAGE EDITOR EXTERNALLY DEFINED SYMBOLS LIST  ***', '',
             'SYMBOL NAME                          ADDR                 TYPE', '']
    for name, address, kind in names:
        row = f"H'{address:08X}               {kind}"
        lines += [name, f'{"":34}{row}'] if len(name) > 32 else [f'{name:34}{row}']
    return '\r\n'.join([*lines, '']).encode()


# what img's map lists: the names a.c and b.c define, and those the symbols object defines where the tables
# put them
LISTED = [('_a_init', 0x0C900000, 'ENT'), ('_b_step', 0x0C900020, 'ENT'), ('_b_free', 0x0C900090, 'EQU'),
          ('_c_static', 0x0C900064, 'EQU'), ('_main', 0x8C0101A0, 'EQU'), ('_shared', 0x8C0101A4, 'EQU')]


class Main(unittest.TestCase):
    def relink(self, *argv, files=IMAGE, written=lambda shift: WRITTEN, read='build/relink/img.bin', listed=LISTED):
        """What relink.py runs for img: each tool's arguments and input, the text of each source it assembles, by its
        path under build/relink/img/, its stdout, and the bytes of the file read, its image unless named. lnk's
        stand-in writes a map that lists listed, as listing() takes them, or listed itself where it is bytes, and
        cnvs's writes written(shift) shift bytes on, where shift is the link's: one of SHIFTS in shift1/ and shift2/,
        and 0 elsewhere."""
        ran, assembled = [], {}

        def run(*argv, stdin=None):
            ran.append((Path(argv[0]).name, *map(str, argv[1:]), stdin))
            name = Path(argv[0]).name
            if name == 'asmsh.exe':
                source = Path(str(argv[1]).replace('\\', '/'))
                assembled[source.relative_to('build/relink/img').as_posix()] = source.read_text()
            elif name == 'lnk.exe':
                printed = next(line.split()[1] for line in stdin.splitlines() if line.startswith('print '))
                Path(printed.replace('\\', '/')).write_bytes(listed if isinstance(listed, bytes) else listing(*listed))
            elif name == 'cnvs.exe':
                out = Path(str(argv[2]).replace('\\', '/'))
                shift = dict(zip(('shift1', 'shift2'), relink.SHIFTS)).get(out.parent.name, 0)
                lines = [srecord(address + shift, bytes([byte])) for address, byte in sorted(written(shift).items())]
                out.write_text('\n'.join([HEADER, *lines, END, '']))

        with tree(files), mock.patch.object(link, 'run', run), contextlib.redirect_stdout(io.StringIO()) as out:
            try:
                relink.main(['img', *argv])
            finally:
                # what the file holds once main returns or fails, for a test that expects it to fail
                self.left = Path(read).read_bytes() if Path(read).exists() else None
            data = Path(read).read_bytes()
        return ran, assembled, out.getvalue(), data

    def test_links_the_objects_in_one_pass(self):
        ran, assembled, _, image = self.relink()
        self.assertEqual([tool for tool, *_ in ran], ['asmsh.exe', 'asmsh.exe', 'lnk.exe', 'cnvs.exe'])
        # the bss no object holds, ahead of a.c's; two/a.c's object as module 1; then the symbols
        commands = ran[2][-1].splitlines()
        self.assertEqual(commands, [r'input build\modules\img\c.obj', r'input build\relink\img\hole1.obj',
                                    r'input build\modules\img\a.obj', r'input build\modules\img\b.obj',
                                    r'input build\relink\img\modules\1.obj', r'input build\relink\img\symbols.obj',
                                    r'output build\relink\img\link.abs', 'form a', 'align_section',
                                    r'print build\relink\img\link.map', 'start P(0c900000),D(0c900060),B(0c900070)',
                                    'exit'])
        self.assertIn('        .SECTION B,DATA,ALIGN=4\n        .RES.B 8\n', assembled['hole1.src'])
        defined = re.findall(r"^_(\w+): \.EQU H'(\w+)$", assembled['symbols.src'], re.M)
        self.assertEqual(defined, [('c_static', '0c900064'), ('b_free', '0c900090'), ('main', '8c0101a0'),
                                   ('shared', '8c0101a4')])
        self.assertEqual(image, SPLICED)

    def test_compares_the_image_with_the_splice(self):
        files = dict(IMAGE, **{'spliced.bin': SPLICED})
        self.relink('spliced.bin', files=files)
        changed = dict(files, **{'spliced.bin': SPLICED[:4] + b'\0' + SPLICED[5:]})
        # the shift test links the image at its base again, and compares that link too
        for argv in ((), ('--shift',)):
            with self.subTest(argv=argv), self.assertRaisesRegex(
                    SystemExit, r'spliced\.bin: its byte at 0x0c900004, which modules/img/a\.c writes, is 0x00, and '
                                r'the whole link of img writes 0x05$'):
                self.relink('spliced.bin', *argv, files=changed)

    def test_rejects_a_row_that_puts_a_name_elsewhere_than_the_whole_link(self):
        # the map gives a_init, which a.c defines, at 0x0c900000, and a name past 32 characters, on a line of its own,
        # at 0x0c900040; their rows put each 16 bytes on
        long = 'two_a_start_under_a_long_enough_name'
        symbols = IMAGE['config/img/symbols.txt'].replace('0x0c900000', '0x0c900010') + f'0x0c900050  {long}\n'
        with self.assertRaisesRegex(SystemExit, r'^.*: config/img/symbols\.txt:1: puts a_init at 0x0c900010, and the '
                                                r'whole link of img defines it at 0x0c900000\n'
                                                rf'.*: config/img/symbols\.txt:5: puts {long} at 0x0c900050, and the '
                                                r'whole link of img defines it at 0x0c900040$'):
            self.relink(files=dict(IMAGE, **{'config/img/symbols.txt': symbols}),
                        listed=[*LISTED, (f'_{long}', 0x0C900040, 'ENT')])
        self.assertIsNone(self.left)

    def test_rejects_a_map_without_its_symbols_list(self):
        # lnk's map, cut short ahead of its list
        cut = listing(*LISTED).split(b'***  LINKAGE EDITOR EXTERNALLY')[0]
        with self.assertRaisesRegex(SystemExit, r'^.*: build/relink/img/link\.map: no externally defined symbols list, '
                                                r'so it may be cut short$'):
            self.relink(listed=cut)
        self.assertIsNone(self.left)

    def test_rejects_a_map_without_a_name_the_link_exports(self):
        # b.c defines b_step, and the symbols object b_free. No object defines or takes b_static, in b.c's data, so the
        # map's lacking it goes unchecked
        files = dict(IMAGE, **{'config/img/symbols.txt': IMAGE['config/img/symbols.txt'] + '0x0c900068  b_static\n'})
        self.relink(files=files)
        with self.assertRaisesRegex(SystemExit, r'^.*: config/img/symbols\.txt:2: names b_step, which the whole link '
                                                r'of img exports, and build/relink/img/link\.map gives it no address\n'
                                                r'.*: config/img/symbols\.txt:4: names b_free, which the whole link '
                                                r'of img exports, and build/relink/img/link\.map gives it no address$'):
            self.relink(files=files, listed=[row for row in LISTED if row[0] not in ('_b_step', '_b_free')])
        self.assertIsNone(self.left)

    def test_writes_the_image_only_once_its_checks_pass(self):
        # SPLICED is compared before the image is written, even where it names the image's own path, so a difference
        # fails and leaves the old image in place
        old = SPLICED[:4] + b'\0' + SPLICED[5:]
        for spliced in ('spliced.bin', 'build/relink/img.bin'):
            files = dict(IMAGE, **{'build/relink/img.bin': old, 'spliced.bin': old})
            with self.subTest(spliced=spliced):
                with self.assertRaisesRegex(SystemExit, r'its byte at 0x0c900004, which modules/img/a\.c writes, is 0x00'):
                    self.relink(spliced, files=files)
                self.assertEqual(self.left, old)

    def test_lists_what_the_link_stands_in_for(self):
        _, _, out, _ = self.relink('--verbose')
        self.assertEqual(out.splitlines(), [
            'img: reserves B 0x0c900078 to 0x0c900080, ahead of modules/img/a.c, where no object lies',
            'img: links modules/two/a.c as module 1, since another module is named a too',
            "img: binds c_static at 0x0c900064 by address, in modules/img/c.c's D",
            'img: binds b_free at 0x0c900090 by address, where no object lies',
        ])

    def test_names_each_later_module_of_a_name_apart(self):
        # three sources are named a: two/a.c's object links as module 1, and three/a.c's as module 2
        files = dict(IMAGE, **{'config/img/splits.txt': SPLITS.replace('fill', 'modules/three/a.c  B  0x0c900088  '
                                                                               '0x0c900090\nfill'),
                               'modules/three/a.c': '', 'build/modules/three/a.obj': obj('a', {'B': B})})
        ran, *_ = self.relink(files=files)
        commands = next(stdin for tool, *_, stdin in ran if tool == 'lnk.exe').splitlines()
        self.assertIn(r'input build\relink\img\modules\1.obj', commands)
        self.assertIn(r'input build\relink\img\modules\2.obj', commands)

    def test_names_a_module_apart_from_relinks_own(self):
        # relink names its own objects symbols and, for the bss it reserves ahead of a.c's, hole1. Of two sources named
        # holes, the second would otherwise be named apart as hole1
        for sources, module in ((('img/symbols',), 'symbol1'), (('img/hole1',), 'hole2'),
                                (('img/holes', 'two/holes'), 'hole2')):
            rows = ''.join(f'modules/{source}.c  B  {span}\n'
                           for source, span in zip(sources, ('0x0c900088  0x0c900090', '0x0c900090  0x0c900098')))
            files = dict(IMAGE, **{'config/img/splits.txt': SPLITS.replace('fill', f'{rows}fill')})
            for source in sources:
                files.update({f'modules/{source}.c': '',
                              f'build/modules/{source}.obj': obj(Path(source).name, {'B': B})})
            with self.subTest(sources=sources):
                ran, *_ = self.relink(files=files)
                commands = next(stdin for tool, *_, stdin in ran if tool == 'lnk.exe').splitlines()
                self.assertIn(rf'input build\relink\img\modules\{module}.obj', commands)

    def test_starts_and_reserves_the_sections_the_extents_give(self):
        # as 1ST_READ.BIN's extents give RSG and RSG32 at B's start: the empty C, which no row places, starts at its
        # extent ahead of B, and B's pieces end at 0x0c900088, short of its extent's end
        extents = (('C', 0x0C900070, 0x0C900070), ('B', 0x0C900070, 0x0C9000A0))
        with mock.patch.dict(config.EXTENTS, {config.CONFIG / 'img': extents}):
            ran, assembled, out, image = self.relink('--verbose')
        commands = next(stdin for tool, *_, stdin in ran if tool == 'lnk.exe').splitlines()
        self.assertIn('start P(0c900000),D(0c900060),C,B(0c900070)', commands)
        self.assertIn('        .SECTION B,DATA,ALIGN=4\n        .RES.B 24\n', assembled['symbols.src'])
        self.assertIn('img: reserves B 0x0c900088 to 0x0c9000a0, the rest of its extent', out.splitlines())
        self.assertEqual(image, SPLICED)

    def test_binds_a_name_a_gen_range_adds(self):
        # a gen/ range holds the data c.c held, and points into b.c's at dat_0c90006c, which only the range's .syms
        # names
        files = dict(IMAGE, **{'config/img/splits.txt': SPLITS.replace('modules/img/c.c  D', 'gen/img/d_0c900060.c  D'),
                               'build/modules/img/c.obj': obj('c', {'B': B}),
                               'build/gen/img/d_0c900060.obj': obj('d_0c900060', {'D': D}, ('_dat_0c90006c',)),
                               'build/gen/img/d_0c900060.syms': '0x0c90006c  dat_0c90006c\n'})
        _, assembled, out, image = self.relink('--verbose', files=files)
        defined = re.findall(r"^_(\w+): \.EQU H'(\w+)$", assembled['symbols.src'], re.M)
        self.assertIn(('dat_0c90006c', '0c90006c'), defined)
        self.assertIn("img: binds dat_0c90006c at 0x0c90006c by address, in modules/img/b.c's D", out.splitlines())
        self.assertEqual(image, SPLICED)

    def test_links_the_image_at_two_shifted_bases_too(self):
        # c.c's data points at b_step and holds 0x0c900040, inside the image, as a number, and b.c's points at a_init;
        # b_step and a_init move with the image
        def written(shift):
            data = words(0x0C900020 + shift, 0x0C900040, 0x0C900000 + shift)
            return {**WRITTEN, **{0x0C900060 + at: byte for at, byte in enumerate(data)}}

        files = dict(IMAGE, **{'config/kept.txt': 'img  0x0c900064  a number, not an address\n'})
        ran, assembled, out, listed = self.relink('--shift', files=files, written=written,
                                                  read='build/relink/img/fixed.txt')
        self.assertEqual([tool for tool, *_ in ran], ['asmsh.exe', *['asmsh.exe', 'lnk.exe', 'cnvs.exe'] * 3])
        # each section, and each name the tables give inside the image, moves by the shift; main and shared, which
        # 1ST_READ.BIN gives, stay
        links = [stdin.splitlines() for tool, *_, stdin in ran if tool == 'lnk.exe']
        for commands, directory, shift in zip(links, ('', 'shift1/', 'shift2/'), (0, *relink.SHIFTS)):
            with self.subTest(shift=shift):
                self.assertIn(f'output {link.dos(Path("build/relink/img", directory, "link.abs"))}', commands)
                self.assertIn(f'start P({0x0C900000 + shift:08x}),D({0x0C900060 + shift:08x}),'
                              f'B({0x0C900070 + shift:08x})', commands)
                defined = re.findall(r"^_(\w+): \.EQU H'(\w+)$", assembled[f'{directory}symbols.src'], re.M)
                self.assertEqual(defined, [('c_static', f'{0x0C900064 + shift:08x}'),
                                           ('b_free', f'{0x0C900090 + shift:08x}'), ('main', '8c0101a0'),
                                           ('shared', '8c0101a4')])
        self.assertEqual(listed, b'img  0x0c900064  0x0c900040  modules/img/c.c  a number, not an address\n')
        self.assertEqual(out, '  shift  img: each shift moves 2 of its words, and keeps 1 at a value in its span\n')
        # a reason config/kept.txt gives another image's word is none for img's
        files['config/kept.txt'] = '1st_read  0x8c0101a0  text\n'
        with self.assertRaisesRegex(SystemExit, r'img: its word at 0x0c900064, which modules/img/c\.c writes, keeps '
                                                r'0x0c900040, in its span, and config/kept\.txt gives no reason$'):
            self.relink('--shift', files=files, written=written)

    def test_writes_the_shift_report_after_the_image(self):
        # make's rule for fixed.txt takes the image as a prerequisite, so the report written first would leave the
        # image newer, and the next make shift would link again
        files = dict(IMAGE, **{'config/kept.txt': 'img  0x0c900064  a number, not an address\n'})

        def written(shift):
            data = words(0x0C900020 + shift, 0x0C900040, 0x0C900000 + shift)
            return {**WRITTEN, **{0x0C900060 + at: byte for at, byte in enumerate(data)}}

        with mock.patch.object(relink, 'save', wraps=relink.save) as saved:
            self.relink('--shift', files=files, written=written)
        paths = [Path(call.args[0]).as_posix() for call in saved.call_args_list]
        self.assertLess(paths.index('build/relink/img.bin'), paths.index('build/relink/img/fixed.txt'))

    def test_refuses_to_shift_an_object_that_aligns_a_section_past_the_shifts(self):
        files = dict(IMAGE, **{'build/modules/img/c.obj': obj('c', {'D': (8, 64, False), 'B': B})})
        with self.assertRaisesRegex(SystemExit, r"modules/img/c\.c: aligns D to 64, more than the shift test's "
                                                r"shifts keep$"):
            self.relink('--shift', files=files)

    def test_needs_each_full_object(self):
        files = dict(IMAGE)
        del files['build/modules/img/b.obj']
        with self.assertRaisesRegex(SystemExit, r'modules/img/b\.c: no object at build/modules/img/b\.obj, so build '
                                                r'the image first'):
            self.relink(files=files)

    def test_refuses_an_image_config_lacks(self):
        with self.assertRaisesRegex(SystemExit, r'config/none/: no such image'), tree(IMAGE):
            relink.main(['none'])


if __name__ == '__main__':
    unittest.main()
