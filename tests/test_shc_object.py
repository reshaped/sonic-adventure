import unittest
from pathlib import Path

import shc_object
from support import tree
from test_link import HEADER, entries, record, section
from test_relink import definition, obj

# the records of an object: three references, the second split across two
# records, then a definition and data. The sections record is filler, not a
# table section_table() takes
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
        return shc_object.references(Path('x.obj'))


class Records(unittest.TestCase):
    def test_gives_each_record_at_its_offset_to_the_end(self):
        with tree({'x.obj': b''.join(OBJECT)}):
            found = list(shc_object.records(Path('x.obj')))
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


class Sections(unittest.TestCase):
    def table(self, *parts):
        with tree({'x.obj': b''.join([record(0x80, bytes(30)), *parts, record(0xFF)])}):
            return shc_object.section_table(Path('x.obj'))

    def test_reads_each_section_in_the_tables_order(self):
        # a definition's section number counts in this order, which isn't the names' sorted order
        self.assertEqual(list(self.table(record(0x88, section(b'P', 0x520, 32)),
                                         record(0x88, section(b'D', 0xC0, 8, data=True))).items()),
                         [('P', (0x520, 32, True)), ('D', (0xC0, 8, False))])
        self.assertEqual(list(self.table(record(0x88, section(b'PSG', 0xB6C, 4)
                                                + section(b'CSG', 0x92, 8, data=True))).items()),
                         [('PSG', (0xB6C, 4, True)), ('CSG', (0x92, 8, False))])

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


def definitions(*parts):
    with tree({'x.obj': obj('x', {}, defines=parts)}):
        return shc_object.definitions(Path('x.obj'))


class Definitions(unittest.TestCase):
    def test_reads_each_name_defined_in_a_section_or_at_an_address(self):
        self.assertEqual(definitions(definition('_a_init', 0x40), definition('_main', 0x8C0101A0, absolute=True),
                                     definition('_b', 8, section=2)),
                         {'_a_init': (0, 0x40), '_main': (None, 0x8C0101A0), '_b': (2, 8)})

    def test_reads_an_address_in_as_many_bytes_as_its_entry_gives(self):
        self.assertEqual(definitions(definition('_end', 0x8C01, absolute=True, size=2), definition('_b', 8)),
                         {'_end': (None, 0x8C01), '_b': (0, 8)})

    def test_rejects_definitions_that_dont_parse(self):
        for cut in (definition('_a_init', 0)[:-1], definition('_main', 0x8C0101A0, absolute=True)[:5]):
            with self.subTest(cut=cut), self.assertRaisesRegex(SystemExit, r"x\.obj: the definitions at 0x[0-9a-f]+ "
                                                                           r"don't parse"):
                definitions(definition('_b', 8), cut)


if __name__ == '__main__':
    unittest.main()
