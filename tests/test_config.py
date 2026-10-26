import unittest
from pathlib import Path
from unittest import mock

import code_ranges
from config import EVENT_ALIASES, EVENT_RUNTIME, EXTENTS, FIRST, Config, configs, directories, files, reasons
from support import LOADER_SECTIONS, MODULE, cwd, tree

ROOT = Path(__file__).resolve().parents[1]


def splits(*rows):
    # MODULE with these rows as mod's splits.txt
    return dict(MODULE, **{'config/mod/splits.txt': ''.join(f'{row}\n' for row in rows)})


def first_read(*rows):
    # MODULE with these rows as 1ST_READ.BIN's splits.txt, and an empty file for each source they place
    return dict(MODULE, **{row.split()[0]: '' for row in rows},
                **{'config/1st_read/splits.txt': ''.join(f'{row}\n' for row in rows)})


def loader(*rows):
    # MODULE with these rows as LOADER.BIN's splits.txt, then LOADER_SECTIONS' rows for the sections they give none
    # of, no names, and an empty loader/a.c for them to place
    given = {row.split()[0] for row in rows}
    rows += tuple(row for row in LOADER_SECTIONS if row.split()[0] not in given)
    return dict(MODULE, **{'config/loader/splits.txt': ''.join(f'{row}\n' for row in rows),
                           'config/loader/symbols.txt': '', 'loader/a.c': ''})


def movie(*rows):
    # MODULE with these rows as MOVIE.BIN's splits.txt, no names, and an empty modules/movie/a.src for them to place
    return dict(MODULE, **{'config/movie/splits.txt': ''.join(f'{row}\n' for row in rows),
                           'config/movie/symbols.txt': '', 'modules/movie/a.src': ''})


SECTIONS = ('P  0x0c900000  0x0c900100', 'B  0x0c900100  0x0c900200')


class Tables(unittest.TestCase):
    def rejects(self, files, pattern):
        with tree(files), self.assertRaisesRegex(SystemExit, pattern):
            Config('mod')

    def test_reads_every_row(self):
        with tree(MODULE):
            config = Config('mod')
            self.assertEqual(config.fill, 0xFF)
            self.assertEqual(config.base, 0x0C900000)
            self.assertEqual(config.sources(), ['modules/mod/a.c', 'modules/mod/b.src'])
            self.assertEqual(config.ranges('modules/mod/a.c'), [('P', 0x0C900000, 0x0C900010), ('D', 0x0C900100, 0x0C900110)])

    def test_rejects_a_placement_with_another_field(self):
        self.rejects(splits(*SECTIONS, 'modules/mod/a.c  P  0x0c900000  0x0c900004  extra'),
                     r'config/mod/splits\.txt:3: not a row')

    def test_rejects_a_row_it_doesnt_know(self):
        self.rejects(splits(*SECTIONS, 'unexpected'), r'splits\.txt:3: not a row')
        self.rejects(splits(*SECTIONS, 'modules/mod/a.c  0x0c900000  0x0c900004'), r'splits\.txt:3: not a row')
        self.rejects(splits(*SECTIONS, '/abs/a.c  P  0x0c900000  0x0c900004'), r'splits\.txt:3: not a row')
        self.rejects(splits(*SECTIONS, '../a.c  P  0x0c900000  0x0c900004'), r'splits\.txt:3: not a row')

    def test_rejects_numbers_out_of_their_form(self):
        self.rejects(splits('P  -0x0c900000  0x0c900100'), r':1: -0x0c900000: not an address')
        self.rejects(splits('P  0x0C900000  0x0c900100'), r':1: 0x0C900000: not an address')
        self.rejects(splits('P  0xc900000  0x0c900100'), r':1: 0xc900000: not an address')
        self.rejects(splits('P  1000  0x0c900100'), r':1: 1000: not an address')
        self.rejects(splits(*SECTIONS, 'fill  0x100'), r':3: 0x100: not a byte')

    def test_rejects_an_empty_or_reversed_range(self):
        self.rejects(splits('P  0x0c900100  0x0c900000'), r':1: 0x0c900100 to 0x0c900000 holds no byte')
        self.rejects(splits(*SECTIONS, 'modules/mod/a.c  P  0x0c900010  0x0c900010'), r':3: .* holds no byte')

    def test_names_a_line_that_isnt_utf_8(self):
        # a Latin-1 é in a comment beside a row
        files = splits(*SECTIONS, 'modules/mod/a.c  P  0x0c900000  0x0c900010  # cafe')
        files['config/mod/splits.txt'] = files['config/mod/splits.txt'].encode().replace(b'cafe', b'caf\xe9')
        self.rejects(files, r"config/mod/splits\.txt:3: a byte that isn't UTF-8")

    def test_rejects_a_second_fill(self):
        self.rejects(splits(*SECTIONS, 'fill  0xff', 'fill  0x00'), r':4: a second fill row, after .*:3')

    def test_rejects_a_second_row_for_a_section(self):
        # P split in two around bytes the split would take out of the code
        self.rejects(splits('P  0x0c900000  0x0c900040', 'P  0x0c900080  0x0c900100', SECTIONS[1]),
                     r':2: a second row for section P, after .*:1')

    def test_rejects_overlaps_and_strays(self):
        self.rejects(splits('P  0x0c900000  0x0c900100', 'D  0x0c9000ff  0x0c900200'), r'section D starts at 0x0c9000ff, inside P')
        self.rejects(splits(*SECTIONS, 'modules/mod/a.c  P  0x0c900000  0x0c900010', 'modules/mod/b.src  P  0x0c90000f  0x0c900020'),
                     r':4: modules/mod/b\.src starts inside modules/mod/a\.c, at .*:3')
        self.rejects(splits(*SECTIONS, 'modules/mod/a.c  P  0x0c9000f0  0x0c900101'), r':3: modules/mod/a\.c lies outside the section P')
        self.rejects(splits(*SECTIONS, 'modules/mod/a.c  D  0x0c900000  0x0c900010'), r'lies outside the section D')
        self.rejects(splits('modules/mod/a.c  P  0x0c900000  0x0c900010'), r'no section rows')

    def test_rejects_a_placement_that_repeats_an_earlier_one(self):
        row = 'modules/mod/a.c  P  0x0c900000  0x0c900010'
        self.rejects(splits(*SECTIONS, row, row),
                     r'config/mod/splits\.txt:4: repeats the placement at config/mod/splits\.txt:3$')

    def test_rejects_a_row_below_its_sources_first_in_the_section(self):
        # link.py starts the object's section at the source's first row in it, so a later row below that one holds
        # none of the object's bytes, and would count the linker's fill there as the source's
        rows = ('modules/mod/a.c  P  0x0c900000  0x0c900010', 'modules/mod/a.c  P  0x0c900020  0x0c900030')
        with tree(splits(*SECTIONS, *rows)):
            self.assertEqual(Config('mod').sources(), ['modules/mod/a.c'])
        self.rejects(splits(*SECTIONS, *reversed(rows)),
                     r':4: modules/mod/a\.c starts below its first row in P, at config/mod/splits\.txt:3$')

    def test_rejects_sources_that_cant_build(self):
        self.rejects(splits(*SECTIONS, 'modules/mod/c.c  P  0x0c900000  0x0c900010'), r':3: modules/mod/c\.c: no such file')
        self.rejects(dict(splits(*SECTIONS, 'modules/mod/a.c  P  0x0c900000  0x0c900010', 'modules/mod/a.src  P  0x0c900010  0x0c900020'),
                          **{'modules/mod/a.src': ''}), r':4: modules/mod/a\.src and modules/mod/a\.c would build one object')
        self.rejects(splits(*SECTIONS, 'gen/other/d_0c900000.c  P  0x0c900000  0x0c900010'), r'a gen/ range is gen/mod/d_0c900000\.c')
        self.rejects(splits(*SECTIONS, 'gen/mod/d_0c900004.c  P  0x0c900000  0x0c900010'), r'a gen/ range is gen/mod/d_0c900000\.c')
        with tree(splits('P  0x0c900000  0x0c900100', 'D  0x0c900100  0x0c900200', 'gen/mod/d_0c900100.c  D  0x0c900100  0x0c900110')):
            self.assertEqual(Config('mod').sources(), ['gen/mod/d_0c900100.c'])

    def test_reads_a_gen_ranges_boundary_declarations(self):
        rows = ('P  0x0c900000  0x0c900100', 'D  0x0c900100  0x0c900200')
        for declarations, declared in (('end', {'end': None}), ('start 8', {'start': 8}), ('continues 4', {'continues': 4}),
                                       ('start 8  end', {'start': 8, 'end': None}), ('end  continues 2', {'end': None, 'continues': 2})):
            with self.subTest(declarations=declarations), tree(splits(*rows, f'gen/mod/d_0c900100.c  D  0x0c900100  0x0c900110  {declarations}')):
                config = Config('mod')
                self.assertEqual(config.declared, {'gen/mod/d_0c900100.c': ('config/mod/splits.txt:3', declared)})
                self.assertEqual(config.placements, [('gen/mod/d_0c900100.c', 'D', 0x0C900100, 0x0C900110)])

    def test_rejects_boundary_declarations_out_of_form(self):
        rows = ('P  0x0c900000  0x0c900100', 'D  0x0c900100  0x0c900200')
        # the last runs past nine digits, and past the 4300 that int() reads by default
        for declarations in ('ends', 'start', 'start 6', 'start 0', 'start 08', 'start 0x8', 'end  end', 'start 8  start 8',
                             'start 8  continues 4', 'continues', '8', 'start 1' + '0' * 5000):
            with self.subTest(declarations=declarations):
                self.rejects(splits(*rows, f'gen/mod/d_0c900100.c  D  0x0c900100  0x0c900110  {declarations}'),
                             r'config/mod/splits\.txt:3: not a row')

    def test_rejects_padding_past_an_items_alignment(self):
        # SHC aligns no item of an object's data past 4, so a range can't go on in an object after padding to 8
        self.rejects(splits('P  0x0c900000  0x0c900100', 'D  0x0c900100  0x0c900200',
                            'gen/mod/d_0c900100.c  D  0x0c900100  0x0c900110  continues 8'),
                     r"config/mod/splits\.txt:3: gen/mod/d_0c900100\.c declares continues 8, and SHC aligns no item of "
                     r"an object's constants, data or bss past 4")

    def test_rejects_a_boundary_declaration_on_a_source(self):
        self.rejects(splits(*SECTIONS, 'modules/mod/a.c  P  0x0c900000  0x0c900010  start 8  end'),
                     r'config/mod/splits\.txt:3: modules/mod/a\.c declares start 8 and end, and only a gen/ range declares '
                     r'its boundaries')

    def test_keeps_gen_ranges_out_of_code(self):
        # code is what Config.code gives: a section whose name starts with P, or LOADER.BIN's and 1ST_READ.BIN's
        # fixed ranges
        for section in ('P', 'PSG'):
            self.rejects(splits(f'{section}  0x0c900000  0x0c900100', f'gen/mod/d_0c900000.c  {section}  0x0c900000  0x0c900010'),
                         r':2: gen/mod/d_0c900000\.c lies in code, and a gen/ range is data from the disc')
        with tree(loader('gen/loader/d_8ce00000.c  DSGLH  0x8ce00000  0x8ce00010')), \
                self.assertRaisesRegex(SystemExit, r':1: gen/loader/d_8ce00000\.c lies in code'):
            Config('loader')
        with tree(first_read('gen/1st_read/d_8c11a578.c  D  0x8c11a578  0x8c11a588')), \
                self.assertRaisesRegex(SystemExit, r':1: gen/1st_read/d_8c11a578\.c lies in code'):
            Config('1st_read')
        with tree(first_read('gen/1st_read/d_8c1755e0.c  D  0x8c1755e0  0x8c1755f0')):
            self.assertEqual(Config('1st_read').sources(), ['gen/1st_read/d_8c1755e0.c'])

    def test_keeps_1st_read_bss_out_of_its_file(self):
        with tree(first_read('src/main.c  B  0x8c688000  0x8c688200')), \
                self.assertRaisesRegex(SystemExit, r"lies outside 1ST_READ\.BIN's bss"):
            Config('1st_read')
        with tree(first_read('src/main.c  D  0x8c688000  0x8c688200')), \
                self.assertRaisesRegex(SystemExit, r"lies outside 1ST_READ\.BIN's file"):
            Config('1st_read')
        with tree(first_read('src/main.c  DSG  0x8c688000  0x8c6880c0', 'src/main.c  B  0x8c688100  0x8c688200')):
            config = Config('1st_read')
            self.assertEqual(config.held(), {'src/main.c': [('DSG', 0x8C688000, 0x8C6880C0)]})

    def test_rejects_a_section_row_in_1st_read(self):
        # even rows that agree with the sections tools/config.py fixes, as layout() gives them
        with tree(MODULE):
            rows = ''.join(f'{name}  0x{start:08x}  0x{end:08x}\n' for name, start, end in Config('1st_read').layout())
        with tree(dict(MODULE, **{'config/1st_read/splits.txt': rows + MODULE['config/1st_read/splits.txt']})), \
                self.assertRaisesRegex(SystemExit, r"config/1st_read/splits\.txt:1: a section row, and tools/config\.py "
                                                   r"fixes 1ST_READ\.BIN's sections$"):
            Config('1st_read')

    def test_names_a_missing_table(self):
        with tree(MODULE), self.assertRaisesRegex(SystemExit, r'config/none/splits\.txt: No such file'):
            Config('none')

    def test_counts_only_directories_as_images(self):
        with tree(dict(MODULE, **{'config/README.md': 'index\n'})):
            self.assertEqual([config.name for config in configs()], ['1st_read', 'mod'])

    def test_gives_each_images_original_file(self):
        with tree(dict(MODULE, **{'config/loader/splits.txt': '', 'config/movie/splits.txt': '', 'config/README.md': ''})):
            self.assertEqual(files(), {'1st_read': '1ST_READ.BIN', 'loader': 'LOADER.BIN', 'mod': 'SONICADV/MOD.PRS',
                                       'movie': 'SONICADV/MOVIE.BIN'})


class Extents(unittest.TestCase):
    def test_follow_one_another_in_link_order(self):
        # each starts where the one before it ends, or past it; one with no end is followed by its X32, and an X32
        # after an end starts there, rounded up to 32
        for extents in EXTENTS.values():
            for name, start, end in extents:
                if end is not None:
                    self.assertLessEqual(start, end, name)
            for (name, start, end), (later, after, _) in zip(extents, extents[1:]):
                self.assertLessEqual(start if end is None else end, after, later)
                self.assertTrue(end is not None or later == f'{name}32', name)
                if end is not None and later == f'{name}32':
                    self.assertEqual(after, -(-end // 32) * 32, later)

    def test_agree_with_the_rows(self):
        # in the repository's tables, each section's first placement starts at its extent's start, and its last
        # ends at its extent's end, where a source holds the object there. None holds LOADER.BIN's last in B, or
        # its first in BSG32.
        unheld = {('loader', 'B', 'end'), ('loader', 'BSG32', 'start')}
        for image in sorted(path.name for path in EXTENTS):
            with cwd(ROOT):
                config = Config(image)
            held = {}
            for source, name, start, end in config.placements:
                held.setdefault(name, []).append((start, end))
            for name, start, end in config.extents():
                if name not in held:
                    continue
                if (image, name, 'start') not in unheld:
                    self.assertEqual(min(held[name])[0], start, (image, name))
                if end is not None and (image, name, 'end') not in unheld:
                    self.assertEqual(max(high for _, high in held[name]), end, (image, name))

    def test_hold_each_row_to_its_sections_extent(self):
        # D's and DSG's run to their X32's start
        rows = ('src/main.c  DSGLH  0x8c010000  0x8c010040', 'src/main.c  D  0x8c614000  0x8c6143e0',
                'src/main.c  B  0x8c688100  0x8c7dd484', 'sdk/ninja/njTexLoad.c  BSG32  0x8c8fc960  0x8c8fd160')
        with tree(first_read(*rows)):
            self.assertEqual(Config('1st_read').sources(), ['src/main.c', 'sdk/ninja/njTexLoad.c'])
        for row, problem in (('src/main.c  B  0x8c7dd480  0x8c7dd490', r'src/main\.c lies outside B, 0x8c688100 to 0x8c7dd484'),
                             ('src/main.c  B  0x8c7dd4a0  0x8c7dd4c0', r'src/main\.c lies outside B, 0x8c688100 to 0x8c7dd484'),
                             ('src/main.c  BSG  0x8c8fc960  0x8c8fd160', r'src/main\.c lies outside BSG, 0x8c8bf4e0 to 0x8c8fb2a0'),
                             ('src/main.c  DSG  0x8c67f27f  0x8c67f290', r'src/main\.c lies outside DSG, 0x8c67f280 to 0x8c6880c0'),
                             ('src/main.c  DSG  0x8c6880c0  0x8c6880d0', r'src/main\.c lies outside DSG, 0x8c67f280 to 0x8c6880c0'),
                             ('src/main.c  DSGLE  0x8c010080  0x8c0100a1', r'src/main\.c lies outside DSGLE, 0x8c010080 to 0x8c0100a0'),
                             ('sdk/ninja/njTexLoad.c  BSG  0x8c8fc960  0x8c8fd160',
                              r'sdk/ninja/njTexLoad\.c lies outside BSG, 0x8c8bf4e0 to 0x8c8fb2a0'),
                             ('sdk/ninja/njLight.c  BSG32  0x8c8fb280  0x8c8fb2c0',
                              r'sdk/ninja/njLight\.c lies outside BSG32, 0x8c8fb2a0 to 0x8c8fd160')):
            with tree(first_read(row)), self.assertRaisesRegex(SystemExit, r'config/1st_read/splits\.txt:1: ' + problem):
                Config('1st_read')

    def test_wants_a_section_row_to_span_the_section_and_its_x32(self):
        rows = ('B  0x8ce20a00  0x8ce50040', 'BSG  0x8ce50040  0x8ce7e920',
                'loader/a.c  B  0x8ce20a00  0x8ce20a10', 'loader/a.c  BSG  0x8ce50040  0x8ce50080')
        with tree(loader(*rows)):
            self.assertEqual(Config('loader').sources(), ['loader/a.c'])
        with tree(loader('B  0x8ce20a00  0x8ce5002c', *rows[1:3])), \
                self.assertRaisesRegex(SystemExit, r'config/loader/splits\.txt: section B runs 0x8ce20a00 to 0x8ce5002c, '
                                                   r'and tools/config\.py gives 0x8ce20a00 to 0x8ce50040'):
            Config('loader')

    def test_wants_a_section_row_to_span_the_section_alone_where_its_x32_has_a_row(self):
        # BSG's row ends at BSG's end, or runs on to BSG32's start, and BSG32's spans BSG32
        bsg32 = 'BSG32  0x8ce7dd40  0x8ce7e920'
        placements = ('loader/a.c  BSG  0x8ce50040  0x8ce50080', 'loader/a.c  BSG32  0x8ce7e120  0x8ce7e920')
        for bsg in ('BSG  0x8ce50040  0x8ce7dd30', 'BSG  0x8ce50040  0x8ce7dd40'):
            with tree(loader(bsg, bsg32, *placements)):
                self.assertEqual(Config('loader').sources(), ['loader/a.c'])
        for rows, problem in ((('BSG  0x8ce50040  0x8ce7dd38', bsg32, *placements), 'section BSG runs 0x8ce50040 to 0x8ce7dd38, '
                               r'and tools/config\.py gives 0x8ce50040 to 0x8ce7dd30'),
                              (('BSG  0x8ce50040  0x8ce7dd30', 'BSG32  0x8ce7dd40  0x8ce7e900', placements[0]),
                               r'section BSG32 runs 0x8ce7dd40 to 0x8ce7e900, and tools/config\.py gives 0x8ce7dd40 to 0x8ce7e920')):
            with tree(loader(*rows)), self.assertRaisesRegex(SystemExit, r'config/loader/splits\.txt: ' + problem):
                Config('loader')

    def test_lets_a_section_row_run_on_to_the_next_section(self):
        # PSGSFD00 ends 2 bytes short of PSGSFD01's start, and its row may end at either, but not at 0x8ceb25d4
        # with no section starting there; nor may it start past PSGSFD00's start
        rows = ('PSGSFD00  0x8ceb0be0  0x8ceb25d4', 'PSGSFD01  0x8ceb25d4  0x8ceb2be0', 'PSGSFD02  0x8ceb2be0  0x8ceb8be0',
                'modules/movie/a.src  PSGSFD01  0x8ceb25d4  0x8ceb2be0')
        for listed in (rows, ('PSGSFD00  0x8ceb0be0  0x8ceb25d2',) + rows[1:]):
            with tree(movie(*listed)):
                self.assertEqual(Config('movie').sources(), ['modules/movie/a.src'])
        for listed in (rows[:1] + rows[2:3], ('PSGSFD00  0x8ceb0be4  0x8ceb25d4',) + rows[1:]):
            with tree(movie(*listed)), \
                    self.assertRaisesRegex(SystemExit, r'config/movie/splits\.txt: section PSGSFD00 runs 0x8ceb0be[04] to '
                                                       r'0x8ceb25d4, and tools/config\.py gives 0x8ceb0be0 to 0x8ceb25d2'):
                Config('movie')

    def test_wants_a_row_for_each_section_that_holds_bytes(self):
        # PSGSFD02's row renamed for constants would take its bytes out of the code; PSGSFD03 holds none, and needs
        # no row
        rows = ('PSGSFD00  0x8ceb0be0  0x8ceb25d4', 'PSGSFD01  0x8ceb25d4  0x8ceb2be0', 'PSGSFD02  0x8ceb2be0  0x8ceb8be0',
                'modules/movie/a.src  PSGSFD01  0x8ceb25d4  0x8ceb2be0')
        with tree(movie(*rows)):
            self.assertEqual(Config('movie').code(), ([], [(0x8CEB0BE0, 0x8CEB25D4), (0x8CEB25D4, 0x8CEB2BE0),
                                                           (0x8CEB2BE0, 0x8CEB8BE0)]))
        with tree(movie(*rows[:2], 'CSGSFD02  0x8ceb2be0  0x8ceb8be0', rows[3])), \
                self.assertRaisesRegex(SystemExit, r'config/movie/splits\.txt: section PSGSFD02 has no row, '
                                                   r'and tools/config\.py gives 0x8ceb2be0 to 0x8ceb8be0'):
            Config('movie')

    def test_bound_no_other_image(self):
        with tree(splits('B  0x0c900100  0x0c900200', 'modules/mod/a.c  B  0x0c900100  0x0c900200')):
            config = Config('mod')
        self.assertEqual(config.extents(), ())


class Layout(unittest.TestCase):
    def test_gives_1st_reads_sections_from_its_tables(self):
        # 1ST_READ.BIN has no section rows: the extents, and P, C, PSG and CSG, tile its file, then its bss
        with tree(MODULE):
            layout = Config('1st_read').layout()
        self.assertEqual([name for name, _, _ in layout],
                         ['DSGLH', 'DSGLE', 'P', 'C', 'D', 'PSG', 'CSG', 'DSG', 'DSG32', 'B', 'B32', 'BSG', 'BSG32'])
        self.assertIn(('C', 0x8C1283CC, 0x8C1755E0), layout)
        self.assertIn(('CSG', 0x8C6785EC, 0x8C67F280), layout)
        stored = layout[:layout.index(('B', 0x8C688100, 0x8C7DD484))]
        self.assertEqual((stored[0][1], stored[-1][2]), (0x8C010000, 0x8C688100))
        for (_, _, end), (name, start, _) in zip(stored, stored[1:]):
            self.assertEqual(end, start, name)

    def test_holds_each_of_1st_reads_placements_in_the_section_it_names(self):
        with cwd(ROOT):
            config = Config('1st_read')
        layout = config.layout()
        for source, name, start, end in config.placements:
            self.assertTrue(any(section == name and low <= start and end <= high for section, low, high in layout), source)

    def test_gives_another_images_section_rows_in_address_order(self):
        with tree(splits('D  0x0c900100  0x0c900200', 'P  0x0c900000  0x0c900100')):
            self.assertEqual(Config('mod').layout(), [('P', 0x0C900000, 0x0C900100), ('D', 0x0C900100, 0x0C900200)])

    def test_spans_each_image_from_its_first_section_to_its_last(self):
        # 1ST_READ.BIN, which has no section rows, from its load address to its bss's end
        with tree(splits('D  0x0c900100  0x0c900200', 'P  0x0c900000  0x0c900100')):
            first, mod = Config('1st_read'), Config('mod')
            self.assertEqual((first.span(), first.base), ((0x8C010000, 0x8C8FD160), 0x8C010000))
            self.assertEqual((mod.span(), mod.base), ((0x0C900000, 0x0C900200), 0x0C900000))


class Aliases(unittest.TestCase):
    def test_each_repeats_a_configured_event_by_its_hash(self):
        # 208 events: one image each but the four that repeat another, which SHA256SUMS holds to its hash
        hashes = dict(reversed(line.split('  ')) for line in (ROOT / 'SHA256SUMS').read_text().splitlines())
        events = {path.name.upper() for path in (ROOT / 'config').iterdir() if path.name.startswith('ev')}
        self.assertEqual(len(events) + len(EVENT_ALIASES), 208)
        for alias, event in EVENT_ALIASES.items():
            self.assertNotIn(alias, events)
            self.assertIn(event, events)
            self.assertEqual(hashes[f'orig/SONICADV/{alias}.PRS'], hashes[f'orig/SONICADV/{event}.PRS'], alias)


class Symbols(unittest.TestCase):
    def test_sees_its_own_names_then_1st_reads_outside_its_span(self):
        with tree(MODULE):
            names = Config('mod').symbols()
        self.assertEqual(names, [(0x0C900000, 'a_init'), (0x0C900000, 'a_start'), (0x0C900104, 'shared'), (0x8C0101A0, 'main')])

    def test_loader_sees_none_of_1st_reads(self):
        with tree(dict(loader(), **{'config/loader/symbols.txt': '0x8ce00000  start\n'})):
            self.assertEqual(Config('loader').symbols(), [(0x8CE00000, 'start')])

    def test_counts_an_events_runtime_as_sdk_code(self):
        runtime = ''.join(f'0x0c9000{0xc0 + 4 * i:02x}  {name}\n' for i, name in enumerate(EVENT_RUNTIME))
        files = dict(MODULE, **{'config/mod/symbols.txt': '0x0c900000  a_init\n' + runtime})
        with tree(files):
            self.assertEqual(Config('mod').code(), ([(0x0C900000, 0x0C9000C0)], [(0x0C9000C0, 0x0C900100)]))
        files['config/mod/symbols.txt'] += '0x0c9000c0  sscanf_entry\n'
        with tree(files):
            self.assertEqual(Config('mod').code(), ([(0x0C900000, 0x0C9000C0)], [(0x0C9000C0, 0x0C900100)]))
        for broken in (runtime.replace('strchr', 'strrchr'), runtime.replace('0x0c9000c4', '0x0c9000f4'),
                       runtime.replace('0x0c9000d8', '0x0c900108')):
            files['config/mod/symbols.txt'] = broken
            with tree(files), self.assertRaisesRegex(SystemExit, r'symbols\.txt: no run of sscanf, strchr'):
                Config('mod').code()

    def test_counts_conversion_helpers_as_sdk_code(self):
        runtime = '0x0c9000bc  _ftou\n0x0c9000dc  _utof\n'
        files = dict(MODULE, **{'config/mod/symbols.txt': '0x0c900000  a_init\n' + runtime})
        with tree(files):
            self.assertEqual(Config('mod').code(), ([(0x0C900000, 0x0C9000BC)], [(0x0C9000BC, 0x0C900100)]))
        for broken in (runtime.replace('_utof', '_itof'), runtime.replace('0x0c9000dc', '0x0c9000b8'),
                       runtime.replace('0x0c9000dc', '0x0c900100')):
            files['config/mod/symbols.txt'] = broken
            with tree(files), self.assertRaisesRegex(SystemExit, r'symbols\.txt: no run of _ftou, _utof'):
                Config('mod').code()

    def test_rejects_a_name_with_two_rows(self):
        for second in ('0x0c900004  a_init', '0x0c900000  a_init'):
            files = dict(MODULE, **{'config/mod/symbols.txt': f'0x0c900000  a_init\n{second}\n'})
            with tree(files), self.assertRaisesRegex(SystemExit, r'symbols\.txt:2: a_init has a row already, at .*symbols\.txt:1'):
                Config('mod').symbols()

    def test_rejects_rows_out_of_form(self):
        for row, problem in (('0x0c900000', 'not a row'), ('0x0c900000  a  b', 'not a row'),
                             ('0x0c900000  1st', 'not a row'), ('c900000  a', 'c900000: not an address')):
            files = dict(MODULE, **{'config/mod/symbols.txt': f'{row}\n'})
            with tree(files), self.assertRaisesRegex(SystemExit, r'config/mod/symbols\.txt:1: ' + problem):
                Config('mod').symbols()


class Directories(unittest.TestCase):
    def test_leaves_out_only_the_images_own_directory(self):
        # events/mod is mod's own directory, and modules/mod, where it places a.c, shares only its name
        with tree(dict(MODULE, **{'events/mod/own.h': ''})):
            self.assertEqual(directories(Config('mod')), [Path('events/mod'), Path('modules/mod')])


class Reasons(unittest.TestCase):
    def reasons(self, *rows):
        with tree(dict(MODULE, **{'config/kept.txt': ''.join(f'{row}\n' for row in rows)})):
            return reasons()

    def test_reads_a_word_or_a_range_by_image_with_its_reason(self):
        # a range may start and end off 4, where it holds a word
        found = self.reasons('# a comment', 'mod  0x0c900010  two   instructions',
                             'mod  0x0c900100  0x0c900110  a glyph bitmap', 'mod  0x0c900113  0x0c900115  a text',
                             '1st_read  0x8c0101a0  text')
        self.assertEqual(found, {'mod': [(0x0C900010, 0x0C900014, 'two instructions', 'config/kept.txt:2'),
                                         (0x0C900100, 0x0C900110, 'a glyph bitmap', 'config/kept.txt:3'),
                                         (0x0C900113, 0x0C900115, 'a text', 'config/kept.txt:4')],
                                 '1st_read': [(0x8C0101A0, 0x8C0101A4, 'text', 'config/kept.txt:5')]})

    def test_rejects_rows_out_of_form(self):
        for row, problem in (('mod  0x0c900010', 'not a row: IMAGE START \\[END\\] REASON'),
                             ('mod  0x0c900010  0x0c900014', 'not a row: IMAGE START \\[END\\] REASON'),
                             ('none  0x0c900010  text', 'none: no such image, a directory in config/'),
                             ('mod  c900010  text', 'c900010: not an address'),
                             ('mod  0x0c900010  0x0c900010  text', '0x0c900010 to 0x0c900010 holds no byte'),
                             ('mod  0x0c900011  0x0c900014  text', '0x0c900011 to 0x0c900014 holds no word'),
                             ('mod  0x0c900012  text', "0x0c900012: not on 4, so no word's address")):
            with self.subTest(row=row), self.assertRaisesRegex(SystemExit, r'config/kept\.txt:1: ' + problem):
                self.reasons(row)

    def test_rejects_a_row_that_overlaps_an_earlier_row_of_its_image(self):
        # another image's row may hold the same addresses
        self.reasons('mod  0x0c900014  text', '1st_read  0x0c900014  text')
        with self.assertRaisesRegex(SystemExit, r'config/kept\.txt:2: 0x0c900010 to 0x0c900020 overlaps the row at '
                                                r'config/kept\.txt:1$'):
            self.reasons("mod  0x0c900014  0x0c900018  a model's UVs", 'mod  0x0c900010  0x0c900020  another reason')
        # each pair shares only the byte at 0x0c900014, and so gives the word there twice
        for rows, later in ((('mod  0x0c900014  0x0c900015  text', 'mod  0x0c900014  another reason'), '0x0c900014 to 0x0c900018'),
                            (('mod  0x0c900014  text', 'mod  0x0c900011  0x0c900015  another reason'), '0x0c900011 to 0x0c900015')):
            with self.subTest(rows=rows), \
                    self.assertRaisesRegex(SystemExit, rf'config/kept\.txt:2: {later} overlaps the row at config/kept\.txt:1$'):
                self.reasons(*rows)

    def test_reads_the_repositorys_rows_each_in_its_images_span(self):
        # config/kept.txt as make shift reads it, so that a malformed row fails without the disc
        with cwd(ROOT):
            found = reasons()
            spans = {image: Config(image).span() for image in found}
        for image, rows in found.items():
            low, high = spans[image]
            for start, end, _, where in rows:
                self.assertTrue(low <= start and end <= high, where)


def objects(aligned):
    # link.section's stand-in: each object's section on aligned[source], with no size, which Config.linker_fill
    # doesn't read
    return lambda source, name: (None, aligned[source])


class LinkerFill(unittest.TestCase):
    def test_names_the_gaps_the_linker_leaves(self):
        # a.c's code ends at 0x10 and b.src's starts at 0x20, on the 32 bytes b.src names; on 16 or 4, it would
        # start at 0x10
        with tree(MODULE):
            config = Config('mod')
            for alignment, gaps in ((32, [('P', 0x0C900010, 0x0C900020)]), (16, []), (4, [])):
                aligned = {'modules/mod/a.c': alignment, 'modules/mod/b.src': alignment}
                self.assertEqual(config.linker_fill(objects(aligned)), gaps)

    def test_names_the_gaps_before_the_next_section(self):
        # a.c's code ends at 0x34, 12 bytes before C at 0x40, and its data starts D at 0x80
        def gaps(row, alignment):
            with tree(splits('P  0x0c900000  0x0c900040', 'C  0x0c900040  0x0c900080', 'D  0x0c900080  0x0c900100',
                             'modules/mod/a.c  P  0x0c900000  0x0c900034', 'modules/mod/a.c  D  0x0c900080  0x0c900090', row)):
                config = Config('mod')
                return config.linker_fill(objects({'modules/mod/a.c': alignment}))

        # C starts at 0x40 on the 16 bytes its first object names, and on 8 would start at 0x38
        first = 'modules/mod/a.c  C  0x0c900040  0x0c900048'
        self.assertEqual(gaps(first, 16), [('P', 0x0C900034, 0x0C900040)])
        self.assertEqual(gaps(first, 8), [])
        # where no row places an object at C's start, a.c, the object in C, stands in for its first, on 32; a.c's
        # constants end 0x18 bytes before its data starts D on 32
        self.assertEqual(gaps('modules/mod/a.c  C  0x0c900060  0x0c900068', 32),
                         [('P', 0x0C900034, 0x0C900040), ('C', 0x0C900068, 0x0C900080)])
        # with only data from the disc in C, C starts on 4 or 8, and on 4 would start at 0x34
        self.assertEqual(gaps('gen/mod/d_0c900040.c  C  0x0c900040  0x0c900080', 16), [])

    def test_names_a_gap_every_alignment_a_section_can_start_on_gives(self):
        # with no object in it, CHEAD starts on 4 or 8: at 0x88 on either after a.c's code ends at 0x86, but on 4 at
        # 0x44 and on 8 at 0x48 after its code ends at 0x44
        def gaps(end, chead):
            with tree(splits(f'PHEAD  0x0c900000  {chead}', f'CHEAD  {chead}  0x0c900100',
                             f'modules/mod/a.c  PHEAD  0x0c900000  {end}')):
                return Config('mod').linker_fill(objects({'modules/mod/a.c': 32}))

        self.assertEqual(gaps('0x0c900086', '0x0c900088'), [('PHEAD', 0x0C900086, 0x0C900088)])
        self.assertEqual(gaps('0x0c900044', '0x0c900048'), [])

    def test_names_the_gaps_in_every_section(self):
        # a.c's constants and bss end 5 and 4 bytes short of b.c's, which start on the 8 bytes b.c names. Nothing
        # sizes a range of data from the disc, so no gap runs from one to c.c's constants, which start on the 32
        # bytes c.c names.
        files = dict(splits('C  0x0c900000  0x0c900040', 'B  0x0c900040  0x0c900080',
                            'modules/mod/a.c  C  0x0c900000  0x0c900003', 'modules/mod/b.c  C  0x0c900008  0x0c900010',
                            'gen/mod/d_0c900010.c  C  0x0c900010  0x0c900018', 'modules/mod/c.c  C  0x0c900020  0x0c900028',
                            'modules/mod/a.c  B  0x0c900040  0x0c900044', 'modules/mod/b.c  B  0x0c900048  0x0c900050'),
                     **{'modules/mod/b.c': '', 'modules/mod/c.c': ''})
        with tree(files):
            config = Config('mod')
            aligned = {'modules/mod/a.c': 8, 'modules/mod/b.c': 8, 'modules/mod/c.c': 32}
            self.assertEqual(config.linker_fill(objects(aligned)), [('C', 0x0C900003, 0x0C900008), ('B', 0x0C900044, 0x0C900048)])

    def test_names_no_gap_inside_an_objects_section(self):
        # a.c's object's section runs from its first row in P to its last, so it holds the bytes before b.c's code,
        # and those between b.src's and c.c's: though b.c and c.c start on 4, no fill writes those bytes, and the
        # code check names them
        aligned = objects({'modules/mod/a.c': 4, 'modules/mod/b.c': 4, 'modules/mod/b.src': 4, 'modules/mod/c.c': 4})
        for rows, hole in (
                (('P  0x0c900000  0x0c90000c', 'modules/mod/a.c  P  0x0c900000  0x0c900002',
                  'modules/mod/b.c  P  0x0c900004  0x0c900008', 'modules/mod/a.c  P  0x0c900008  0x0c90000c'),
                 '0x0c900002 to 0x0c900004'),
                (('P  0x0c900000  0x0c900010', 'modules/mod/a.c  P  0x0c900000  0x0c900004',
                  'modules/mod/b.src  P  0x0c900004  0x0c900006', 'modules/mod/c.c  P  0x0c900008  0x0c90000c',
                  'modules/mod/a.c  P  0x0c90000c  0x0c900010'),
                 '0x0c900006 to 0x0c900008')):
            with self.subTest(hole=hole), tree(dict(splits(*rows), **{'modules/mod/b.c': '', 'modules/mod/c.c': ''})):
                config = Config('mod')
                self.assertEqual(config.linker_fill(aligned), [])
                with self.assertRaisesRegex(SystemExit, rf'config/mod/splits\.txt: no source or fill writes {hole}, '
                                                        r'code in P$'):
                    code_ranges.check_code(config, aligned)

    def test_names_only_the_next_source_inside_1st_reads_sections(self):
        # 1ST_READ.BIN's main.c ends at 0x8c0101b0, before move.c's code at 0x8c0101c0, which ends 8 bytes
        # before data.c's constants, on the 16 bytes data.c names: P runs on to C's start, so data.c's constants
        # start no section there, and no gap runs to them
        with tree(first_read('src/main.c  P  0x8c0101a0  0x8c0101b0', 'src/move.c  P  0x8c0101c0  0x8c0101c8',
                             'src/data.c  C  0x8c0101d0  0x8c0101d8')):
            config = Config('1st_read')
            gaps = config.linker_fill(objects({'src/main.c': 32, 'src/move.c': 32, 'src/data.c': 16}))
        self.assertEqual(gaps, [('P', 0x8C0101B0, 0x8C0101C0)])

    def test_names_the_gap_where_1st_reads_p_ends(self):
        # P ends at C's start, 0x8c1283cc, 2 bytes after a.c's code: b.src's constants start there on the 4 bytes
        # it names, and on 2 they would start at a.c's end. Where they start past C's start, they stand in for its
        # first object, and on 8 would start C at 0x8c1283d0.
        def gaps(c, alignment):
            with tree(first_read('sdk/runtime/a.c  P  0x8c1283a8  0x8c1283ca', f'sdk/shinobi/b.src  C  {c}  0x8c1283f4')):
                config = Config('1st_read')
                return config.linker_fill(objects({'sdk/runtime/a.c': 4, 'sdk/shinobi/b.src': alignment}))

        self.assertEqual(gaps('0x8c1283cc', 4), [('P', 0x8C1283CA, 0x8C1283CC)])
        self.assertEqual(gaps('0x8c1283cc', 2), [])
        self.assertEqual(gaps('0x8c1283d0', 4), [('P', 0x8C1283CA, 0x8C1283CC)])
        self.assertEqual(gaps('0x8c1283d0', 8), [])

    def test_names_the_gaps_where_1st_reads_d_csg_and_dsg_end(self):
        # vmsio.c's data ends D 24 bytes short of D32, and sd_sys_flush_host_cmd.c's constants end CSG 5 bytes short
        # of CSG32: no object places a byte in either, and strt1.src's object, which declares both, starts them on 32,
        # at 0x8c6143e0 and 0x8c67f280. sd_midi_get_total_beat_time.c's data ends DSG 8 bytes short of DSG32, which
        # njGlobal.c's object starts on 32 at 0x8c6880c0. On 4, D32 and DSG32 would start where the bytes before them
        # end, and CSG32 a byte after.
        self.assertEqual(tails(*TAILS), [('D', 0x8C6143C8, 0x8C6143E0), ('CSG', 0x8C67F27B, 0x8C67F280),
                                         ('DSG', 0x8C6880B8, 0x8C6880C0)])
        self.assertEqual(tails(*TAILS, strt1=4, global_=4), [])

    def test_names_no_gap_where_1st_reads_d_csg_or_dsg_ends_further_off(self):
        # data that ends D 40 bytes short of D32, constants that end CSG 37 short of CSG32 and data that ends DSG 72
        # short of DSG32: on 32, the next section would start at 0x8c6143c0, 0x8c67f260 and 0x8c688080, so no
        # alignment explains the bytes up to D32, CSG32 and DSG32. Nor does any explain those after data from the disc.
        self.assertEqual(tails(f'{VMSIO}  D  0x8c6143b4  0x8c6143b8', f'{FLUSH}  CSG  0x8c67f244  0x8c67f25b',
                               f'{BEAT}  DSG  0x8c688074  0x8c688078', TAILS[3]), [])
        self.assertEqual(tails('gen/1st_read/d_8c6143c0.c  D  0x8c6143c0  0x8c6143c8'), [])

    def test_fails_where_the_extents_give_a_tails_section_no_start(self):
        # CSG's fill runs to CSG32's start, which only the extents give
        extents = tuple(row for row in EXTENTS[FIRST] if row[0] != 'CSG32')
        with mock.patch.dict(EXTENTS, {FIRST: extents}), self.assertRaisesRegex(
                SystemExit, r"tools/config\.py: FIRST_TAILS runs the fill from CSG's last placement to CSG32's start, "
                            r'and FIRST_EXTENTS gives CSG32 none'):
            tails(*TAILS)


# 1ST_READ.BIN's last placements in D, CSG and DSG, and njGlobal.c's at DSG32's start
VMSIO, FLUSH, BEAT = 'sdk/shinobi/vmsio.c', 'sdk/shinobi/sd_sys_flush_host_cmd.c', 'sdk/shinobi/sd_midi_get_total_beat_time.c'
TAILS = (f'{VMSIO}  D  0x8c6143c4  0x8c6143c8', f'{FLUSH}  CSG  0x8c67f244  0x8c67f27b', f'{BEAT}  DSG  0x8c6880b4  0x8c6880b8',
         'sdk/ninja/njGlobal.c  DSG32  0x8c6880c0  0x8c688100')


def tails(*rows, strt1=32, global_=32):
    # the gaps the linker leaves in 1ST_READ.BIN with these rows, its sources' objects on 4, strt1.src's on strt1 and
    # njGlobal.c's on global_
    aligned = {VMSIO: 4, FLUSH: 4, BEAT: 4, 'sdk/shinobi/strt1.src': strt1, 'sdk/ninja/njGlobal.c': global_}
    with tree(first_read(*rows)):
        return Config('1st_read').linker_fill(objects(aligned))


def bounded(*rows):
    # mod's linker_fill and padding with sections P, C and D and these rows, a.c's and b.c's objects on 8, c.c's on
    # 16 and d.c's on 32
    files = dict(splits('P  0x0c900000  0x0c900040', 'C  0x0c900040  0x0c900100', 'D  0x0c900100  0x0c900200', *rows),
                 **{'modules/mod/b.c': '', 'modules/mod/c.c': '', 'modules/mod/d.c': ''})
    with tree(files):
        config = Config('mod')
        aligned = objects({'modules/mod/a.c': 8, 'modules/mod/b.c': 8, 'modules/mod/c.c': 16, 'modules/mod/d.c': 32})
        return config.linker_fill(aligned), config.padding(aligned)


class Boundaries(unittest.TestCase):
    def test_runs_a_gap_from_a_gen_range_that_declares_its_end(self):
        # the gen/ range's object ends with its row, at 0x114, and b.c's starts on 8 at 0x118
        after = 'modules/mod/b.c  D  0x0c900118  0x0c900120'
        self.assertEqual(bounded('gen/mod/d_0c900100.c  D  0x0c900100  0x0c900114  end', after),
                         ([('D', 0x0C900114, 0x0C900118)], []))
        self.assertEqual(bounded('gen/mod/d_0c900100.c  D  0x0c900100  0x0c900114', after), ([], []))

    def test_runs_a_gap_to_a_gen_range_that_declares_its_start(self):
        # a.c's object ends at 0x114, and the gen/ range's starts on 8 at 0x118, or on 16 at 0x120, since c.c's after
        # it starts on 16
        before = 'modules/mod/a.c  D  0x0c900100  0x0c900114'
        self.assertEqual(bounded(before, 'gen/mod/d_0c900118.c  D  0x0c900118  0x0c900120  start 8'),
                         ([('D', 0x0C900114, 0x0C900118)], []))
        self.assertEqual(bounded(before, 'gen/mod/d_0c900120.c  D  0x0c900120  0x0c900130  start 16',
                                 'modules/mod/c.c  D  0x0c900130  0x0c900140'),
                         ([('D', 0x0C900114, 0x0C900120)], []))
        self.assertEqual(bounded(before, 'gen/mod/d_0c900118.c  D  0x0c900118  0x0c900120'), ([], []))

    def test_runs_a_gap_between_gen_ranges_that_declare_an_end_and_a_start(self):
        self.assertEqual(bounded('gen/mod/d_0c900100.c  D  0x0c900100  0x0c900114  end',
                                 'gen/mod/d_0c900118.c  D  0x0c900118  0x0c900120  start 8'),
                         ([('D', 0x0C900114, 0x0C900118)], []))

    def test_pads_to_a_gen_range_that_continues_the_sources_object(self):
        # a.c's strings end at 0x46, and the gen/ range's go on, on 4, at 0x48: padding inside one object, no fill.
        # Where the range declares its end too, fill runs from it to b.c's constants on 8.
        before = 'modules/mod/a.c  C  0x0c900040  0x0c900046'
        self.assertEqual(bounded(before, 'gen/mod/d_0c900048.c  C  0x0c900048  0x0c90005b  continues 4'),
                         ([], [('C', 0x0C900046, 0x0C900048)]))
        self.assertEqual(bounded(before, 'gen/mod/d_0c900048.c  C  0x0c900048  0x0c90005b  continues 4  end',
                                 'modules/mod/b.c  C  0x0c900060  0x0c900068'),
                         ([('C', 0x0C90005B, 0x0C900060)], [('C', 0x0C900046, 0x0C900048)]))

    def test_rejects_a_declared_gap_its_alignment_doesnt_give(self):
        for rows, problem in (
                (('gen/mod/d_0c900100.c  D  0x0c900100  0x0c900114  end', 'modules/mod/b.c  D  0x0c900120  0x0c900128'),
                 r'splits\.txt:4: gen/mod/d_0c900100\.c ends at 0x0c900114, which rounds up to 0x0c900118 on 8, and '
                 r'modules/mod/b\.c starts at 0x0c900120'),
                (('modules/mod/a.c  D  0x0c900100  0x0c900114', 'gen/mod/d_0c900118.c  D  0x0c900118  0x0c900120  start 16',
                  'modules/mod/c.c  D  0x0c900120  0x0c900130'),
                 r'splits\.txt:5: modules/mod/a\.c ends at 0x0c900114, which rounds up to 0x0c900120 on 16, and '
                 r'gen/mod/d_0c900118\.c starts at 0x0c900118'),
                (('gen/mod/d_0c900100.c  D  0x0c900100  0x0c900114  end', 'gen/mod/d_0c900118.c  D  0x0c900118  0x0c900120  start 32',
                  'modules/mod/d.c  D  0x0c900120  0x0c900130'),
                 r'splits\.txt:4 and .*splits\.txt:5: gen/mod/d_0c900100\.c ends at 0x0c900114, which rounds up to '
                 r'0x0c900120 on 32'),
                (('modules/mod/a.c  C  0x0c900040  0x0c900045', 'gen/mod/d_0c900048.c  C  0x0c900048  0x0c900050  continues 2'),
                 r'splits\.txt:5: modules/mod/a\.c ends at 0x0c900045, which rounds up to 0x0c900046 on 2'),
                (('gen/mod/d_0c900100.c  D  0x0c900100  0x0c900118  end', 'modules/mod/b.c  D  0x0c900118  0x0c900120'),
                 r'splits\.txt:4: gen/mod/d_0c900100\.c ends where modules/mod/b\.c starts, at 0x0c900118, and no gap '
                 r'lies between them')):
            with self.subTest(rows=rows), self.assertRaisesRegex(SystemExit, problem):
                bounded(*rows)

    def test_rejects_a_start_on_an_alignment_no_object_in_its_section_starts_on(self):
        # a.c's object starts D on 8, and c.c's on 16: no object of the image's starts on 4, and none in D on 32
        for rows, problem in (
                (('modules/mod/a.c  D  0x0c900100  0x0c900114', 'gen/mod/d_0c900114.c  D  0x0c900114  0x0c900120  start 4'),
                 r"splits\.txt:5: gen/mod/d_0c900114\.c declares start 4, and the image's objects in D start on 8$"),
                (('modules/mod/a.c  D  0x0c900100  0x0c900114', 'gen/mod/d_0c900120.c  D  0x0c900120  0x0c900130  start 16'),
                 r"splits\.txt:5: gen/mod/d_0c900120\.c declares start 16, and the image's objects in D start on 8$"),
                (('modules/mod/a.c  D  0x0c900100  0x0c900114', 'gen/mod/d_0c900120.c  D  0x0c900120  0x0c900130  start 32',
                  'modules/mod/c.c  D  0x0c900130  0x0c900140'),
                 r"splits\.txt:5: gen/mod/d_0c900120\.c declares start 32, and the image's objects in D start on 8 or 16$"),
                # with only gen/ ranges in D, an object there starts on 4 or 8, as R10 and R32 start one
                (('gen/mod/d_0c900100.c  D  0x0c900100  0x0c900114  end',
                  'gen/mod/d_0c900120.c  D  0x0c900120  0x0c900130  start 16'),
                 r"splits\.txt:5: gen/mod/d_0c900120\.c declares start 16, and with no source outside gen/ placed in D, "
                 r"an object there starts on 4 or 8$")):
            with self.subTest(rows=rows), self.assertRaisesRegex(SystemExit, problem):
                bounded(*rows)

    def test_rejects_a_declaration_no_gap_uses(self):
        for rows, problem in (
                # the range after it gives no start, or none follows it in its section
                (('gen/mod/d_0c900100.c  D  0x0c900100  0x0c900114  end', 'gen/mod/d_0c900118.c  D  0x0c900118  0x0c900120'),
                 r'splits\.txt:4: gen/mod/d_0c900100\.c declares end, and no gap uses it'),
                (('gen/mod/d_0c900048.c  C  0x0c900048  0x0c90005b  end', 'modules/mod/b.c  D  0x0c900100  0x0c900108'),
                 r'splits\.txt:4: gen/mod/d_0c900048\.c declares end, and no gap uses it'),
                # the range before it gives no end, or the source before it ends no object there
                (('gen/mod/d_0c900100.c  D  0x0c900100  0x0c900114', 'gen/mod/d_0c900118.c  D  0x0c900118  0x0c900120  start 8'),
                 r'splits\.txt:5: gen/mod/d_0c900118\.c declares start 8, and no gap uses it'),
                (('modules/mod/a.c  D  0x0c900100  0x0c900114', 'gen/mod/d_0c900118.c  D  0x0c900118  0x0c900120  start 8',
                  'modules/mod/a.c  D  0x0c900140  0x0c900148'),
                 r'splits\.txt:5: gen/mod/d_0c900118\.c declares start 8, and no gap uses it'),
                # no source's object goes on into a range after another range
                (('gen/mod/d_0c900040.c  C  0x0c900040  0x0c900046  end', 'gen/mod/d_0c900048.c  C  0x0c900048  0x0c900050  continues 4'),
                 r'splits\.txt:4: gen/mod/d_0c900040\.c declares end, and no gap uses it'),
                (('gen/mod/d_0c900040.c  C  0x0c900040  0x0c900046', 'gen/mod/d_0c900048.c  C  0x0c900048  0x0c900050  continues 4'),
                 r'splits\.txt:5: gen/mod/d_0c900048\.c declares continues 4, and no gap uses it')):
            with self.subTest(rows=rows), self.assertRaisesRegex(SystemExit, problem):
                bounded(*rows)


class Tiling(unittest.TestCase):
    def test_takes_the_gap_before_a_section_on_its_alignment(self):
        # a.c's constants end C at 0x34, and its data starts D on 8 at 0x38; on 4, D would start at 0x34
        rows = ('P  0x0c900000  0x0c900020', 'C  0x0c900020  0x0c900034', 'D  0x0c900038  0x0c900100',
                'modules/mod/a.c  P  0x0c900000  0x0c900020', 'modules/mod/a.c  C  0x0c900020  0x0c900034',
                'modules/mod/a.c  D  0x0c900038  0x0c900040')
        with tree(splits(*rows)):
            config = Config('mod')
            self.assertEqual(config.linker_fill(objects({'modules/mod/a.c': 8})), [(None, 0x0C900034, 0x0C900038)])
            with self.assertRaisesRegex(SystemExit, r'config/mod/splits\.txt: no row holds 0x0c900034 to 0x0c900038, '
                                                    r'between C and D, and no alignment D can start on puts its start there'):
                config.linker_fill(objects({'modules/mod/a.c': 4}))

    def test_rejects_a_code_row_split_or_cut_short(self):
        # a.c's and b.c's code end at 0x80, and the code from there to C's start at 0x100, which a.c's constants
        # start on 8, is the disc's. P split around 0x80 to 0xc0, or cut short at 0xc0, would drop those bytes from
        # the code.
        placements = ('modules/mod/a.c  P  0x0c900000  0x0c900040', 'modules/mod/b.c  P  0x0c900040  0x0c900080',
                      'modules/mod/a.c  C  0x0c900100  0x0c900108')
        aligned = objects({'modules/mod/a.c': 8, 'modules/mod/b.c': 8})
        for rows, hole in ((('P  0x0c900000  0x0c900080', 'PB  0x0c9000c0  0x0c900100'), '0x0c900080 to 0x0c9000c0, between P and PB'),
                           (('P  0x0c900000  0x0c9000c0',), '0x0c9000c0 to 0x0c900100, between P and C')):
            with tree(dict(splits(*rows, 'C  0x0c900100  0x0c900200', *placements), **{'modules/mod/b.c': ''})):
                config = Config('mod')
                with self.assertRaisesRegex(SystemExit, rf'no row holds {hole}'):
                    config.linker_fill(aligned)
        with tree(dict(splits('P  0x0c900000  0x0c900100', 'C  0x0c900100  0x0c900200', *placements), **{'modules/mod/b.c': ''})):
            self.assertEqual(Config('mod').linker_fill(aligned), [])

    def test_takes_a_gap_the_extents_give(self):
        # strt1's table starts LOADER.BIN's BSG32 on 32, at 0x8ce7dd40, 16 bytes past BSG's end, though the object
        # it places there starts it on 4
        rows = ('BSG  0x8ce50040  0x8ce7dd30', 'BSG32  0x8ce7dd40  0x8ce7e920',
                'loader/a.c  BSG  0x8ce50040  0x8ce50080', 'loader/a.c  BSG32  0x8ce7dd40  0x8ce7e920')
        with tree(loader(*rows)):
            self.assertEqual(Config('loader').linker_fill(objects({'loader/a.c': 4})), [(None, 0x8CE7DD30, 0x8CE7DD40)])

    def test_starts_a_section_on_its_first_objects_alignment(self):
        # a.c starts D on 4; b.c, on 8, and a.c stand in for an object no row places at D's start
        def starts(a, name='D'):
            with tree(dict(splits('P  0x0c900000  0x0c900100', 'D  0x0c900100  0x0c900200',
                                  f'modules/mod/a.c  D  {a}  0x0c900110', 'modules/mod/b.c  D  0x0c900120  0x0c900130'),
                           **{'modules/mod/b.c': ''})):
                return Config('mod').alignments(name, 0x0C900100, objects({'modules/mod/a.c': 4, 'modules/mod/b.c': 8}))

        self.assertEqual(starts('0x0c900100'), {4})
        self.assertEqual(starts('0x0c900104'), {4, 8})
        # with no object in it, a section other than code starts on 4 or 8, as R10 and R32 start one, and code on
        # an alignment no compiler fixes
        self.assertEqual(starts('0x0c900100', 'C'), {4, 8})
        self.assertEqual(starts('0x0c900100', 'P'), set())


if __name__ == '__main__':
    unittest.main()
