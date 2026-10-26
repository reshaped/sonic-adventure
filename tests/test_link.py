import os
import re
import shutil
import subprocess
import sys
import time
import unittest
from pathlib import Path, PureWindowsPath
from unittest import mock

import link
from config import Config
from support import MODULE, tree

# S-records as cnvs writes them: a header, two data records, and the end
HEADER = 'S00E00007461736B202020206D6F746E'
DATA = 'S3098C1D80980000000035'
MORE = 'S3158C09DEA00B00090006D30B00326000090009000932'
END = 'S7058C09DEA0E7'


def srecords(*lines):
    with tree({'x.mot': ''.join(f'{line}\n' for line in lines)}):
        return link.srecords(Path('x.mot'))


def srecord(address, data):
    # an S3 record as cnvs writes one: its count, address and data, and a byte that brings its sum to 0xff
    raw = bytes([len(data) + 5]) + address.to_bytes(4, 'big') + data
    return 'S3' + (raw + bytes([(0xFF - sum(raw)) & 0xFF])).hex().upper()


def record(kind, body=b''):
    # a record as SHC writes one: its kind, size and body, and a byte that brings its sum to 0xff
    head = bytes([kind, len(body) + 3]) + body
    return head + bytes([(0xFF - sum(head)) & 0xFF])


def entries(*names):
    return b''.join(b'\xc0' + bytes([len(name)]) + name for name in names)


# an object as R32 writes one: three references, the second split across two
# records, then a definition and data
ENTRIES = entries(b'_task_create', b'_njSin', b'_ab')
OBJECT = [record(0x80, bytes(30)), record(0x88, b'@' + bytes(18)),
          record(0x0C, ENTRIES[:15]), record(0x8C, ENTRIES[15:]),
          record(0x94, b'\0\3\x20' + bytes(4) + b'\x07_a_init'),
          record(0x9C, b'\x80' + bytes(4) + b'\x05_abcd\0'),
          record(0xFF)]
# where the run of references starts
RUN = len(OBJECT[0] + OBJECT[1])


def references(*parts):
    with tree({'x.obj': b''.join(parts)}):
        return link.references(Path('x.obj'))


class SRecords(unittest.TestCase):
    def rejects(self, pattern, *lines):
        with self.assertRaisesRegex(SystemExit, pattern):
            srecords(*lines)

    def test_reads_each_byte_at_its_address(self):
        image = srecords(HEADER, DATA, MORE, END)
        self.assertEqual(len(image), 20)
        self.assertEqual([image[0x8C1D8098 + i] for i in range(4)], [0, 0, 0, 0])
        self.assertEqual([image[0x8C09DEA0 + i] for i in range(4)], [0x0B, 0x00, 0x09, 0x00])

    def test_rejects_a_wrong_checksum(self):
        self.rejects(r'x\.mot:2: its count or checksum is wrong', HEADER, DATA[:-2] + '36', END)

    def test_rejects_a_truncated_record(self):
        self.rejects(r'x\.mot:2: its count or checksum is wrong', HEADER, DATA[:-4] + '35', END)

    def test_rejects_a_second_write_to_an_address(self):
        self.rejects(r'x\.mot:3: writes 0x8c1d8098 again', HEADER, DATA, DATA, END)

    def test_rejects_what_cnvs_doesnt_write(self):
        self.rejects(r'x\.mot:1: not an S-record', 'hello')
        self.rejects(r'x\.mot:2: not an S-record', HEADER, DATA.lower(), END)
        self.rejects(r'x\.mot: no end record', HEADER, DATA)
        self.rejects(r'x\.mot:3: a record after the end', HEADER, END, DATA)
        self.rejects(r"x\.mot:2: S5, a record cnvs doesn't write", HEADER, 'S5030001FB', END)

    def test_rejects_a_record_too_short_for_its_address(self):
        # a count and a checksum alone, then an address a byte short
        self.rejects(r'x\.mot:1: S1 too short for its 2-byte address', 'S101FE', 'S901FE')
        self.rejects(r'x\.mot:2: S9 too short for its 2-byte address', 'S1030000FC', 'S901FE')
        self.rejects(r'x\.mot:2: S3 too short for its 4-byte address', HEADER, 'S304000000FB', END)
        self.rejects(r'x\.mot:2: S7 too short for its 4-byte address', HEADER, 'S704000000FB')

    def test_reads_a_record_of_its_address_alone(self):
        self.assertEqual(srecords('S0030000FC', 'S1030000FC', 'S9030000FC'), {})

    def test_rejects_an_end_record_with_more_than_its_address(self):
        self.rejects(r'x\.mot:2: S9 with bytes after its 2-byte address', HEADER, 'S904000000FB')


class Records(unittest.TestCase):
    def test_gives_each_record_at_its_offset_to_the_end(self):
        with tree({'x.obj': b''.join(OBJECT)}):
            found = list(link.records(Path('x.obj')))
        self.assertEqual(found, [(len(b''.join(OBJECT[:index])), record) for index, record in enumerate(OBJECT)])


class References(unittest.TestCase):
    def rejects(self, pattern, *parts):
        with self.assertRaisesRegex(SystemExit, pattern):
            references(*parts)

    def test_reads_the_symbols_an_object_references(self):
        self.assertEqual(references(*OBJECT), {'_task_create', '_njSin', '_ab'})

    def test_reads_a_symbol_that_spans_records(self):
        run = [record(0x0C, ENTRIES[:4]), record(0x0C, ENTRIES[4:20]), record(0x8C, ENTRIES[20:])]
        self.assertEqual(references(*run, record(0xFF)), {'_task_create', '_njSin', '_ab'})

    def test_ignores_a_symbol_spelled_outside_its_references(self):
        # the definition and the data spell _a_init and _abcd after their lengths
        self.assertEqual(references(*OBJECT[:2], *OBJECT[4:]), set())
        self.assertEqual(references(*OBJECT[:2], record(0x8C, entries(b'_abcd')), *OBJECT[4:]), {'_abcd'})

    def test_rejects_a_truncated_record(self):
        end = len(b''.join(OBJECT[:-1]))
        self.rejects(rf'x\.obj: the record at {end:#x} is cut short', *OBJECT[:-1], OBJECT[-1][:-1])
        self.rejects(rf'x\.obj: the record at {end:#x} is cut short', *OBJECT[:-1], b'\xff')

    def test_rejects_a_bad_size(self):
        self.rejects(rf'x\.obj: the record at {RUN:#x} has size 2', *OBJECT[:2], b'\x0c\x02', *OBJECT[3:])
        longer = bytes([0x0C, OBJECT[2][1] + 1]) + OBJECT[2][2:]
        self.rejects(rf'x\.obj: the record at {RUN:#x} fails its checksum', *OBJECT[:2], longer, *OBJECT[3:])
        self.rejects(rf'x\.obj: the record at {RUN:#x} is cut short', *OBJECT[:2], b'\x0c\xff', *OBJECT[3:5])

    def test_rejects_a_wrong_checksum(self):
        wrong = OBJECT[2][:-1] + bytes([OBJECT[2][-1] ^ 1])
        self.rejects(rf'x\.obj: the record at {RUN:#x} fails its checksum', *OBJECT[:2], wrong, *OBJECT[3:])

    def test_rejects_a_run_another_kind_breaks(self):
        broken = len(OBJECT[0] + OBJECT[1] + OBJECT[2])
        self.rejects(rf'x\.obj: the record at {broken:#x} breaks a run of kind 0x0c', *OBJECT[:3], *OBJECT[4:])

    def test_rejects_references_that_dont_parse(self):
        for run in (b'\x80\x02_a', entries(b'_ab')[:-1], b'\xc0'):
            self.rejects(rf"x\.obj: the references at {RUN:#x} don't parse", *OBJECT[:2], record(0x8C, run), OBJECT[-1])

    def test_rejects_what_isnt_an_object(self):
        self.rejects(r'x\.obj: no end record', *OBJECT[:-1])
        self.rejects(r'x\.obj: no end record')
        end = len(b''.join(OBJECT))
        self.rejects(rf'x\.obj: bytes follow its end, at {end:#x}', *OBJECT, b'\0')
        self.rejects(r'x\.obj: the record at 0x0 is cut short', HEADER.encode())
        self.rejects(r'x\.obj: the record at 0x0 has size 0', bytes(4))


class Names(unittest.TestCase):
    def test_adds_the_new_names_of_a_syms_file(self):
        with tree(dict(MODULE, **{'x.syms': '0x0c900050  dat_0c900050\n0x0c900000  a_init\n'})):
            names = link.symbols(Config('mod'), Path('x.syms'))
        self.assertIn((0x0C900050, 'dat_0c900050'), names)
        self.assertEqual(sum(name == 'a_init' for _, name in names), 1)

    def test_adds_the_new_names_of_each_syms_file_in_turn(self):
        # y.syms repeats x.syms's binding, and z.syms moves it
        files = dict(MODULE, **{'x.syms': '0x0c900050  dat_0c900050\n', 'y.syms': '0x0c900050  dat_0c900050\n',
                                'z.syms': '0x0c900054  dat_0c900050\n'})
        with tree(files):
            names = link.symbols(Config('mod'), Path('x.syms'), Path('y.syms'))
            self.assertEqual(names.count((0x0C900050, 'dat_0c900050')), 1)
            with self.assertRaisesRegex(SystemExit, r'z\.syms: dat_0c900050 is at 0x0c900054, and the tables put it at '
                                                    r'0x0c900050'):
                link.symbols(Config('mod'), Path('x.syms'), Path('z.syms'))

    def test_rejects_a_syms_file_that_moves_a_name(self):
        with tree(dict(MODULE, **{'x.syms': '0x0c900004  a_init\n'})), \
                self.assertRaisesRegex(SystemExit, r'x\.syms: a_init is at 0x0c900004, and the tables put it at 0x0c900000'):
            link.symbols(Config('mod'), Path('x.syms'))

    def test_binds_inside_its_sections_only_an_admitted_second_name(self):
        sections = [('P', 0x0C900000, 0x0C900100)]
        names = [(0x0C900000, 'a_init'), (0x0C900010, 'casino_ornament_draw_still'),
                 (0x0C900010, 'casino_ornament_step_still'), (0x0C900020, 'a_draw'), (0x0C900020, 'a_step'),
                 (0x8C010000, 'b')]
        used = {'_a_init', '_casino_ornament_step_still', '_a_step', '_b'}
        self.assertEqual(link.exported(names, used, sections),
                         [(0x0C900010, 'casino_ornament_step_still'), (0x8C010000, 'b')])
        # and only at an address named more than once
        self.assertEqual(link.exported([(0x0C900010, 'casino_ornament_step_still')], used, sections), [])
        # and one at P's end lies outside P
        self.assertEqual(link.exported([(0x0C900100, 'b')], used, sections), [(0x0C900100, 'b')])


def section(name, size, alignment, data=False):
    # a section entry as R32 writes one: attributes, size, alignment, 0x10 for data, then the name
    return (b'@' + bytes(4) + size.to_bytes(4, 'big') + alignment.to_bytes(4, 'big') + bytes([0x10 if data else 0])
            + b'\xff\xc0' + bytes([len(name)]) + name)


def obj(size, alignment):
    # an object as R32 writes one, whose one section is a P of size bytes on alignment
    return b''.join([record(0x80, bytes(30)), record(0x88, section(b'P', size, alignment)), record(0xFF)])


class Sections(unittest.TestCase):
    def table(self, *parts):
        with tree({'x.obj': b''.join([record(0x80, bytes(30)), *parts, record(0xFF)])}):
            return link.section_table(Path('x.obj'))

    def test_reads_each_sections_size_alignment_and_kind(self):
        self.assertEqual(self.table(record(0x88, section(b'P', 0x520, 32)),
                                    record(0x88, section(b'D', 0xC0, 8, data=True))),
                         {'P': (0x520, 32, True), 'D': (0xC0, 8, False)})
        self.assertEqual(self.table(record(0x88, section(b'PSG', 0xB6C, 4) + section(b'CSG', 0x92, 8, data=True))),
                         {'PSG': (0xB6C, 4, True), 'CSG': (0x92, 8, False)})

    def test_rejects_a_cut_entry(self):
        with self.assertRaisesRegex(SystemExit, r"x\.obj: the sections at 0x21 don't parse"):
            self.table(record(0x88, section(b'P', 0x520, 32)[:-1]))

    def test_rejects_a_section_named_twice(self):
        with self.assertRaisesRegex(SystemExit, r'x\.obj: the sections at 0x21 name P again'):
            self.table(record(0x88, section(b'P', 0x520, 32) + section(b'P', 0x40, 4)))

    def test_rejects_an_alignment_not_a_power_of_two(self):
        for alignment in (0, 3):
            with self.subTest(alignment=alignment), self.assertRaisesRegex(
                    SystemExit, rf'x\.obj: the sections at 0x21 align P to {alignment}, not a power of two'):
                self.table(record(0x88, section(b'P', 0x520, alignment)))

    def test_reads_the_object_or_else_its_stub(self):
        with tree({'build/modules/mod/built.obj': obj(0x40, 32), 'build/stub/modules/mod/built.obj': obj(0x40, 32),
                   'build/stub/modules/mod/stubbed.obj': obj(0x60, 32)}):
            self.assertEqual(link.section('modules/mod/built.c', 'P'), (0x40, 32, True))
            self.assertEqual(link.section('modules/mod/stubbed.c', 'P'), (0x60, 32, True))
            with self.assertRaisesRegex(SystemExit, r'build/stub/modules/mod/stubbed\.obj: no D section'):
                link.section('modules/mod/stubbed.c', 'D')
            with self.assertRaisesRegex(SystemExit, r'modules/mod/none\.c: no object under build/ or build/stub/'):
                link.section('modules/mod/none.c', 'P')

    def test_reads_the_later_of_an_object_and_its_stub_where_they_differ(self):
        # a stale object beside the stub make check just built must not lend its alignment
        with tree({'build/modules/mod/stale.obj': obj(0x40, 32), 'build/stub/modules/mod/stale.obj': obj(0x40, 8),
                   'build/modules/mod/fresh.obj': obj(0x40, 32), 'build/stub/modules/mod/fresh.obj': obj(0x40, 8),
                   'build/modules/mod/tied.obj': obj(0x40, 32), 'build/stub/modules/mod/tied.obj': obj(0x40, 8)}):
            for name, built, stub in (('stale', 1, 2), ('fresh', 2, 1), ('tied', 1, 1)):
                os.utime(f'build/modules/mod/{name}.obj', ns=(built, built))
                os.utime(f'build/stub/modules/mod/{name}.obj', ns=(stub, stub))
            self.assertEqual(link.section('modules/mod/stale.c', 'P'), (0x40, 8, True))
            self.assertEqual(link.section('modules/mod/fresh.c', 'P'), (0x40, 32, True))
            with self.assertRaisesRegex(SystemExit, r'name P differently and neither is newer'):
                link.section('modules/mod/tied.c', 'P')


# extents as tools/config.py gives them: DSGLH and B with their ends, D with its
# start alone, D32 and R, empty, at one address, and RSG and RSG32, empty, where
# B starts
EXTENTS = (('DSGLH', 0x8C010000, 0x8C010080), ('D', 0x8C1755E0, None), ('D32', 0x8C6143E0, 0x8C6143E0),
           ('R', 0x8C6143E0, 0x8C6143E0), ('RSG', 0x8C688100, 0x8C688100), ('RSG32', 0x8C688100, 0x8C688100),
           ('B', 0x8C688100, 0x8C7DD484))


class Layout(unittest.TestCase):
    def test_starts_each_section_at_its_first_row(self):
        ranges = [('P', 0x8C0101A0, 0x8C0101B0), ('C', 0x8C128428, 0x8C128430), ('P', 0x8C0101C0, 0x8C0101D0)]
        self.assertEqual(link.layout('a.c', ranges, EXTENTS, {'P': (0x30, 32, True), 'C': (8, 8, False)}),
                         ([(0x8C0101A0, ['P']), (0x8C128428, ['C'])], []))

    def test_starts_a_section_with_no_row_at_its_extent(self):
        # the empty D32 and R start together, in the extents' order, and lnk places the empty PSG, which no extent gives
        declared = {'P': (12, 4, True), 'R': (0, 4, False), 'D32': (0, 32, False), 'D': (0, 4, False), 'PSG': (0, 4, True)}
        starts, reserved = link.layout('a.src', [('P', 0x8C0100A0, 0x8C0100AC)], EXTENTS, declared)
        self.assertEqual(starts, [(0x8C0100A0, ['P']), (0x8C1755E0, ['D']), (0x8C6143E0, ['D32', 'R'])])
        self.assertEqual(reserved, [])

    def test_reserves_the_rest_of_an_extent_after_the_objects_bytes(self):
        # DSGLH's first 0x40 bytes are the object's; its B, empty, takes all of B's extent
        declared = {'DSGLH': (0x40, 4, False), 'B': (0, 32, False)}
        starts, reserved = link.layout('a.src', [('DSGLH', 0x8C010000, 0x8C010040)], EXTENTS, declared)
        self.assertEqual(starts, [(0x8C010000, ['DSGLH']), (0x8C688100, ['B'])])
        self.assertEqual(reserved, [('DSGLH', 0x40, 4, False), ('B', 0x155384, 32, False)])
        # a B of 0x24 bytes on 8 leaves the rest from 0x28, so B still ends at the extent's end
        _, reserved = link.layout('a.c', [('B', 0x8C688100, 0x8C688124)], EXTENTS, {'B': (0x24, 8, False)})
        self.assertEqual(reserved, [('B', 0x155384 - 0x28, 8, False)])
        # nothing is left to reserve where the object fills the extent
        self.assertEqual(link.layout('a.c', [('DSGLH', 0x8C010000, 0x8C010080)], EXTENTS, {'DSGLH': (0x80, 4, False)})[1], [])

    def test_reserves_code_where_the_object_names_code(self):
        # an empty PSGSFD00, code, takes all of its extent as code; PSGSFD01 fills its own
        extents = (('PSGSFD00', 0x8CEB0BE0, 0x8CEB25D2), ('PSGSFD01', 0x8CEB25D4, 0x8CEB2BE0))
        declared = {'PSG': (0xB8, 4, True), 'PSGSFD00': (0, 4, True), 'PSGSFD01': (0x60C, 4, True)}
        ranges = [('PSGSFD01', 0x8CEB25D4, 0x8CEB2BE0), ('PSG', 0x8CEDC800, 0x8CEDC8B8)]
        self.assertEqual(link.layout('a.src', ranges, extents, declared),
                         ([(0x8CEB0BE0, ['PSGSFD00']), (0x8CEB25D4, ['PSGSFD01']), (0x8CEDC800, ['PSG'])],
                          [('PSGSFD00', 0x19F2, 4, True)]))

    def test_starts_the_sections_at_one_address_in_the_extents_order(self):
        # RSG and RSG32 start ahead of B: after it, they would start past the space B reserves
        declared = {'B': (0, 4, False), 'RSG32': (0, 32, False), 'RSG': (0, 4, False)}
        starts, reserved = link.layout('a.src', [], EXTENTS, declared)
        self.assertEqual(starts, [(0x8C688100, ['RSG', 'RSG32', 'B'])])
        self.assertEqual(reserved, [('B', 0x155384, 4, False)])

    def test_reserves_nothing_for_a_section_whose_row_starts_past_its_extents_start(self):
        self.assertEqual(link.layout('a.c', [('DSGLH', 0x8C010040, 0x8C010060)], EXTENTS, {'DSGLH': (0x20, 8, False)}),
                         ([(0x8C010040, ['DSGLH'])], []))

    def test_ends_each_row_where_its_objects_section_ends(self):
        # a P of 0x20 bytes whose code ends 12 bytes short of its end, and e102_hit.c's 0x495 bytes of constants,
        # which end 3 bytes before the next object's, on 8: each row holds its section, no less and no more
        declared = {'P': (0x20, 32, True), 'C': (0x495, 8, False)}

        def rows(p, c):
            return [('P', 0x8C0101A0, p), ('C', 0x8C1702F0, c)]

        self.assertEqual(link.layout('a.c', rows(0x8C0101C0, 0x8C170785), EXTENTS, declared)[1], [])
        for p, c, problem in ((0x8C0101B4, 0x8C170785, r"its P row ends at 0x8c0101b4, and its object's P at 0x8c0101c0"),
                              (0x8C0101C0, 0x8C170788, r"its C row ends at 0x8c170788, and its object's C at 0x8c170785")):
            with self.subTest(p=p, c=c), self.assertRaisesRegex(SystemExit, r'a\.c: ' + problem):
                link.layout('a.c', rows(p, c), EXTENTS, declared)

    def test_ends_a_bss_row_where_its_objects_bss_ends(self):
        # car_sprite and car_matrix, 0x60 bytes of bss, need all of a row from 0x0c960070
        declared = {'B': (0x60, 8, False)}
        self.assertEqual(link.layout('a.c', [('B', 0x0C960070, 0x0C9600D0)], (), declared), ([(0x0C960070, ['B'])], []))
        with self.assertRaisesRegex(SystemExit, r"a\.c: its B row ends at 0x0c9600c0, and its object's B at 0x0c9600d0"):
            link.layout('a.c', [('B', 0x0C960070, 0x0C9600C0)], (), declared)

    def test_ends_a_sections_last_row_where_the_section_ends(self):
        # a P of 0x30 bytes in two rows: the second ends where the section does
        ranges = [('P', 0x8C0101A0, 0x8C0101B0), ('P', 0x8C0101C0, 0x8C0101D0)]
        self.assertEqual(link.layout('a.c', ranges, EXTENTS, {'P': (0x30, 32, True)})[0], [(0x8C0101A0, ['P'])])
        with self.assertRaisesRegex(SystemExit, r"a\.c: its P row ends at 0x8c0101d0, and its object's P at 0x8c0101e0"):
            link.layout('a.c', ranges, EXTENTS, {'P': (0x40, 32, True)})

    def test_rejects_a_row_into_the_space_it_reserves(self):
        # a PSGSFD01 of 0x60c bytes fills its row, and an empty one would reserve all of its row
        extents = (('PSGSFD01', 0x8CEB25D4, 0x8CEB2BE0),)
        row = [('PSGSFD01', 0x8CEB25D4, 0x8CEB2BE0)]
        self.assertEqual(link.layout('a.src', row, extents, {'PSGSFD01': (0x60C, 4, True)})[1], [])
        with self.assertRaisesRegex(SystemExit, r"a\.src: its PSGSFD01 row ends at 0x8ceb2be0, and its object's PSGSFD01 "
                                                r'at 0x8ceb25d4'):
            link.layout('a.src', row, extents, {'PSGSFD01': (0, 4, True)})

    def test_rejects_a_row_of_a_section_its_object_lacks(self):
        with self.assertRaisesRegex(SystemExit, r'a\.c: its D row names no section of its object'):
            link.layout('a.c', [('P', 0x8C0101A0, 0x8C0101B0), ('D', 0x8C1755E0, 0x8C1755E8)], EXTENTS, {'P': (0x10, 32, True)})

    def test_rejects_a_section_with_bytes_and_no_row(self):
        # a bss of 0x18 bytes needs a row as the code does, and an empty C none
        declared = {'P': (0x10, 4, True), 'C': (0, 4, False), 'B': (0x18, 4, False)}
        self.assertEqual(link.layout('a.c', [('P', 0x0CB80008, 0x0CB80018), ('B', 0x0CBA1DB8, 0x0CBA1DD0)], (), declared)[1], [])
        with self.assertRaisesRegex(SystemExit, r"a\.c: its object's B holds 0x18 bytes, and no row places it"):
            link.layout('a.c', [('P', 0x0CB80008, 0x0CB80018)], (), declared)

    def test_rejects_an_object_longer_than_its_extent(self):
        with self.assertRaisesRegex(SystemExit, r'a\.c: its B runs past 0x8c7dd484, where tools/config\.py ends B'):
            link.layout('a.c', [('B', 0x8C688100, 0x8C7DD488)], EXTENTS, {'B': (0x155388, 4, False)})


class Joined(unittest.TestCase):
    # MODULE places a.c's code at 0x0c900000 and its data at 0x0c900100, 16 bytes each, and fills with 0xff
    def test_holds_the_bytes_written_in_the_sources_ranges(self):
        with tree(MODULE):
            linked = link.joined(Config('mod'), 'modules/mod/a.c', {0x0C900000: 1, 0x0C900001: 2, 0x0C90010F: 3})
        self.assertEqual(linked, b'\1\2' + b'\xff' * 29 + b'\3')

    def test_leaves_bss_0(self):
        files = dict(MODULE, **{'config/mod/splits.txt': MODULE['config/mod/splits.txt'] + 'modules/mod/a.c  B  0x0c900200  0x0c900204\n'})
        with tree(files):
            linked = link.joined(Config('mod'), 'modules/mod/a.c', {0x0C900200: 7})
        self.assertEqual(linked, b'\xff' * 32 + b'\7\0\0\0')

    def test_rejects_a_byte_in_bss_the_file_doesnt_store(self):
        # main.c's B lies in 1ST_READ.BIN's bss, past its file: unwritten, it adds no byte, and a 0 or a 1 written
        # there fails
        files = dict(MODULE, **{'config/1st_read/splits.txt': MODULE['config/1st_read/splits.txt']
                                + 'src/main.c  B  0x8c688100  0x8c688104\n'})
        with tree(files):
            self.assertEqual(link.joined(Config('1st_read'), 'src/main.c', {0x8C0101A0: 0x0B}), b'\x0b' + bytes(15))
            with self.assertRaisesRegex(SystemExit, r"src/main\.c: 2 bytes at 0x8c688100 fall in 1ST_READ\.BIN's bss, "
                                                    r"which its file doesn't store"):
                link.joined(Config('1st_read'), 'src/main.c', {0x8C0101A0: 0x0B, 0x8C688100: 0, 0x8C688103: 1})

    def test_rejects_a_byte_outside_every_range(self):
        # one past a.c's code, and one in b.src's
        with tree(MODULE), self.assertRaisesRegex(SystemExit, r'modules/mod/a\.c: 2 bytes at 0x0c900010 fall outside its ranges '
                                                              r'in config/mod/splits\.txt'):
            link.joined(Config('mod'), 'modules/mod/a.c', {0x0C900000: 1, 0x0C900010: 2, 0x0C900020: 3})


# a.c's object, whose P and D of 16 bytes each MODULE places at 0x0c900000 and 0x0c900100
A_OBJ = b''.join([record(0x80, bytes(30)), record(0x88, section(b'P', 0x10, 4) + section(b'D', 0x10, 4, data=True)),
                 record(0xFF)])
# its linked source, of the bytes Main's stand-in for cnvs writes and MODULE's fill
LINKED = b'\1\2' + b'\xff' * 14 + b'\3' + b'\xff' * 15


class Main(unittest.TestCase):
    def link(self, spelled, files=None):
        """What link.py runs for the object spelled(root) names, root the tree's root, in MODULE with a.c's object
        and files: each tool's arguments and input, and the linked sources under build/ after, by path. cnvs's
        stand-in writes 1 and 2 at 0x0c900000 and 3 at 0x0c900100."""
        ran = []

        def run(*argv, stdin=None):
            ran.append((*map(str, argv), stdin))
            if Path(argv[0]).name == 'cnvs.exe':
                Path(PureWindowsPath(argv[2]).as_posix()).write_text(
                    f'{HEADER}\n{srecord(0x0C900000, bytes([1, 2]))}\n{srecord(0x0C900100, bytes([3]))}\n{END}\n')

        with tree({**MODULE, 'build/modules/mod/a.obj': A_OBJ, **(files or {})}) as root, \
                mock.patch.object(link, 'run', run):
            link.main([spelled(root), 'mod'])
            return ran, {path.relative_to(root).as_posix(): path.read_bytes() for path in root.glob('build/**/*.bin')}

    def test_links_an_object_by_any_path_that_resolves_under_build(self):
        ran, linked = self.link(lambda root: 'build/modules/mod/a.obj')
        self.assertEqual(linked, {'build/mod/modules/mod/a.bin': LINKED})
        # lnk, the second tool, takes commands that name the object by its path under build/, then the symbols
        self.assertEqual(ran[1][-1].splitlines(), [r'input build\modules\mod\a.obj', r'input build\mod\modules\mod\a_syms.obj',
                                                  r'output build\mod\modules\mod\a.abs', 'form a',
                                                  'start P(0c900000),D(0c900100)', 'exit'])
        # and an absolute path, or one through .., runs the same
        for spelled in (lambda root: str(root / 'build/modules/mod/a.obj'),
                        lambda root: 'build/../build/modules/mod/a.obj'):
            self.assertEqual(self.link(spelled), (ran, linked))

    def test_links_a_stub_under_build_stub_alone(self):
        # a stub's linked source goes under build/stub/, and the full build's stays as it was
        full = {'build/mod/modules/mod/a.bin': b'full'}
        _, linked = self.link(lambda root: 'build/stub/modules/mod/a.obj',
                              {**full, 'build/stub/modules/mod/a.obj': A_OBJ})
        self.assertEqual(linked, {**full, 'build/stub/mod/modules/mod/a.bin': LINKED})

    def test_finds_a_source_by_its_whole_path(self):
        files = dict(MODULE, **{'modules/other/a.c': ''})
        files['config/mod/splits.txt'] += 'modules/other/a.c  P  0x0c900040  0x0c900050\n'
        with tree(files):
            config = Config('mod')
        self.assertEqual([link.find(config, stem) for stem in ('modules/other/a', 'modules/mod/a', 'a')],
                         ['modules/other/a.c', 'modules/mod/a.c', None])

    def test_wants_the_image_where_two_place_the_source(self):
        files = dict(MODULE, **{'build/modules/mod/a.obj': A_OBJ, 'config/two/symbols.txt': '',
                                'config/two/splits.txt': ('P  0x0c500000  0x0c500100\n'
                                                          'modules/mod/a.c  P  0x0c500000  0x0c500010\n')})
        with tree(files), \
                self.assertRaisesRegex(SystemExit, r'modules/mod/a: 2 splits\.txt list it, so name the image$'):
            link.main(['build/modules/mod/a.obj'])

    def test_refuses_an_object_outside_build(self):
        files = dict(MODULE, **{'a.obj': A_OBJ, 'build/modules/mod/a.obj': A_OBJ})
        for spelled in ('a.obj', 'build/../a.obj'):
            with self.subTest(spelled=spelled), tree(files), \
                    self.assertRaisesRegex(SystemExit, rf'{re.escape(spelled)}: not under build/$'):
                link.main([spelled, 'mod'])


# link.py, as make runs it
LINK = Path(link.__file__).resolve()
# stand-ins for wibo: each starts a writer, which ignores TERM and makes the file "written" a second on, then makes
# "started" and hangs. One makes "termed" on TERM and ends; the other ignores TERM
HANGS = {
    'ends on TERM': ('#!/bin/sh\ntrap "touch termed; exit" TERM\n(trap "" TERM; sleep 1; touch written) &\n'
                     'touch started\nsleep 3 &\nwait $!\n'),
    'ignores TERM': '#!/bin/sh\ntrap "" TERM\n(sleep 1; touch written) &\ntouch started\nexec sleep 3\n',
}


class Run(unittest.TestCase):
    def test_names_a_tool_that_fails_silently(self):
        with mock.patch.object(link, 'WIBO', shutil.which('false')), \
                self.assertRaisesRegex(SystemExit, r'asmsh\.exe exited with status 1'):
            link.run(Path('tools/shc/asmsh.exe'))

    def test_names_a_message_from_a_tool_that_exits_0(self):
        # lnk and asmsh exit 0 after a warning, so a line of lnk's form, opening '** ', or of asmsh's, marked (W),
        # fails the run
        for message in ('** a message', '(W) a message'):
            with self.subTest(message=message), tree({'wibo': f"#!/bin/sh\necho '{message}'\n"}) as root:
                (root / 'wibo').chmod(0o755)
                with mock.patch.object(link, 'WIBO', str(root / 'wibo')), \
                        self.assertRaisesRegex(SystemExit, rf'\A[^\n]*: {re.escape(message)}\Z'):
                    link.run('lnk.exe')

    def test_stops_a_tool_that_hangs(self):
        # sleep stands in for wibo, so the tool, 5, is the seconds it sleeps
        with mock.patch.multiple(link, WIBO=shutil.which('sleep'), HUNG=0.2), \
                self.assertRaisesRegex(SystemExit, r'5 ran 0\.2 seconds and was stopped'):
            link.run('5')

    def test_stops_what_a_hung_tool_started(self):
        # stop() sends the tool's process group TERM, then KILL once the tool ends or its grace is up, short of the
        # writer's second: the tool that ends on TERM makes "termed" first, and neither writer makes "written"
        for kind, wibo in HANGS.items():
            with self.subTest(kind), tree({'wibo': wibo}) as root:
                (root / 'wibo').chmod(0o755)
                with mock.patch.multiple(link, WIBO=str(root / 'wibo'), HUNG=0.2, GRACE=0.4), \
                        self.assertRaisesRegex(SystemExit, r'asmsh\.exe ran 0\.2 seconds and was stopped'):
                    link.run('asmsh.exe')
                time.sleep(1.5)
                self.assertEqual((root / 'termed').exists(), kind == 'ends on TERM')
                self.assertFalse((root / 'written').exists())

    def test_stops_its_tool_when_term_ends_it(self):
        # a TERM to make's process group reaches link.py and not its tool's session: link.py stops the tool's group,
        # then exits 143, as a shell reports a process TERM ends
        files = dict(MODULE, **{'build/modules/mod/a.obj': A_OBJ, 'wibo': HANGS['ends on TERM']})
        with tree(files) as root:
            (root / 'wibo').chmod(0o755)
            with subprocess.Popen([sys.executable, '-B', str(LINK), 'build/modules/mod/a.obj', 'mod'],
                                  env=dict(os.environ, WIBO=str(root / 'wibo'))) as linking:
                # asmsh, link.py's first tool, is running once its stand-in makes "started"
                deadline = time.monotonic() + 10
                while not (root / 'started').exists() and time.monotonic() < deadline:
                    time.sleep(0.01)
                linking.terminate()
                status = linking.wait()
            time.sleep(1.5)
            written = (root / 'written').exists()
        self.assertEqual((status, written), (143, False))


if __name__ == '__main__':
    unittest.main()
