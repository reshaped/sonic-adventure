import struct
import unittest

import dwarf
from dwarf import At, Form, Lnct, Tag, Unread, Ut

# the forms the tests take for each attribute they read
READ = {At.NAME: dwarf.STRINGS, At.TYPE: dwarf.REFERENCES, At.BYTE_SIZE: frozenset((Form.DATA1, Form.DATA4)),
        At.STMT_LIST: frozenset((Form.SEC_OFFSET,))}
# a DWARF 5 compilation unit's header: its length, version, unit type and address size, and its abbreviation table's
# offset. The length counts the bytes past its own 4
HEADER = '<IHBBI'


def uleb(value):
    out = bytearray()
    while True:
        byte, value = value & 0x7F, value >> 7
        out.append(byte | (0x80 if value else 0))
        if not value:
            return bytes(out)


def abbrev(code, tag, children, *specs):
    # one abbreviation's bytes: its code, tag and children flag, then each (attribute, form). Two 0s end it
    return (uleb(code) + uleb(tag) + bytes((children,)) + b''.join(uleb(name) + uleb(form) for name, form in specs)
            + b'\0\0')


# an abbreviation table. 1 is a compilation unit with a name and children. 2 is a base type with a name and a byte
# size. 3 is a variable with a name and a type by DW_FORM_ref4, and 4 a variable with a name by DW_FORM_strp
ABBREV = (abbrev(1, Tag.COMPILE_UNIT, 1, (At.NAME, Form.STRING))
          + abbrev(2, Tag.BASE_TYPE, 0, (At.NAME, Form.STRING), (At.BYTE_SIZE, Form.DATA1))
          + abbrev(3, Tag.VARIABLE, 0, (At.NAME, Form.STRING), (At.TYPE, Form.REF4))
          + abbrev(4, Tag.VARIABLE, 0, (At.NAME, Form.STRP)) + b'\0')


def unit(body, version=5, kind=Ut.COMPILE, address=4):
    # a unit whose entries are body
    return struct.pack(HEADER, struct.calcsize(HEADER) - 4 + len(body), version, kind, address, 0) + body


def variable(reference):
    # the entry of a variable n whose type is the entry at reference, an offset in its unit
    return uleb(3) + b'n\0' + struct.pack('<I', reference)


# the compilation unit a.c's entry, and the base type int's
A_C = uleb(1) + b'a.c\0'
INT = uleb(2) + b'int\0' + bytes((4,))
# a unit's entries: a.c, whose children are int and a variable n of it, and the null that ends them
ENTRIES = A_C + INT + variable(struct.calcsize(HEADER) + len(A_C)) + b'\0'


def units(info, strings=b''):
    return dwarf.units({'.debug_info': info, '.debug_abbrev': ABBREV, '.debug_str': strings}, READ)


# a line table's header fields from minimum_instruction_length to opcode_base, by version. Version 3 lacks
# maximum_operations_per_instruction, and opcode_base 1 leaves no standard_opcode_lengths
FIELDS = {3: bytes((1, 1, 0xFB, 14, 1)), 5: bytes((1, 1, 1, 0xFB, 14, 1))}
# a version 5 table's entry formats: a path inline, and a directory's index by DW_FORM_udata
PATH = (Lnct.PATH, Form.STRING)
INDEX = (Lnct.DIRECTORY_INDEX, Form.UDATA)


def line_table(version, lists, header_length=None):
    # a line table whose header holds FIELDS, then lists. header_length defaults to the header's own. A version 5
    # table gives 4-byte addresses and no segment selector
    header = FIELDS[version] + lists
    fields = struct.pack('<H', version) + (bytes((4, 0)) if version == 5 else b'')
    fields += struct.pack('<I', len(header) if header_length is None else header_length)
    return struct.pack('<I', len(fields) + len(header)) + fields + header


def entry_list(formats, entries):
    # a version 5 table's list of directories or files: the count of formats and each (content, form), then the count
    # of entries and each one's bytes
    return (bytes((len(formats),)) + b''.join(uleb(content) + uleb(form) for content, form in formats)
            + uleb(len(entries)) + b''.join(entries))


class Reader(unittest.TestCase):
    def test_reads_a_unit_s_entries_and_their_references(self):
        (read,) = units(unit(ENTRIES))
        names = {die.attrs.get(At.NAME): (offset, die) for offset, die in read.dies.items()}
        self.assertEqual(read.root, names['a.c'][0])
        self.assertEqual(names['n'][1].attrs[At.TYPE], names['int'][0])
        self.assertEqual(names['int'][1].attrs[At.BYTE_SIZE], 4)
        self.assertEqual(read.dies[read.root].children, [names['int'][0], names['n'][0]])

    def test_reads_each_representation_of_a_value_and_stops_past_it(self):
        # DW_FORM_indirect reads as the form it names, here DW_FORM_data2, and the byte past each value stays unread
        for data, form, value, read in ((b'\xe5\x8e\x26', Form.UDATA, 624485, Form.UDATA),
                                        (b'\xc0\xbb\x78', Form.SDATA, -123456, Form.SDATA),
                                        (b'\x05\x34\x12', Form.INDIRECT, 0x1234, Form.DATA2),
                                        (b'\x02ab', Form.BLOCK1, b'ab', Form.BLOCK1),
                                        (b'\0', Form.EXPRLOC, b'', Form.EXPRLOC),
                                        (b'\x01\x02\x03', Form.STRX3, 0x030201, Form.STRX3)):
            with self.subTest(form=str(form)):
                self.assertEqual(dwarf.decode(data + b'\xff', 0, len(data) + 1, form), (read, value, len(data)))

    def test_fails_on_a_value_cut_short(self):
        for data, form in ((b'\x01', Form.DATA4), (b'\x05ab', Form.BLOCK1), (b'\x80', Form.UDATA),
                           (b'ab', Form.STRING)):
            with self.subTest(form=str(form)), self.assertRaisesRegex(Unread, 'cut short'):
                dwarf.decode(data, 0, len(data), form)

    def test_fails_on_indirect_naming_itself_or_an_implicit_constant_no_abbreviation_holds(self):
        for data, form, failure in ((uleb(Form.INDIRECT), Form.INDIRECT, 'indirectly'),
                                    (b'', Form.IMPLICIT_CONST, 'no abbreviation holds'),
                                    (uleb(Form.IMPLICIT_CONST), Form.INDIRECT, 'no abbreviation holds')):
            with self.subTest(data=data, form=str(form)), self.assertRaisesRegex(Unread, failure):
                dwarf.decode(data, 0, len(data), form)

    def test_fails_on_a_unit_that_ends_inside_its_last_attribute_or_runs_past_its_section(self):
        with self.assertRaisesRegex(Unread, 'cut short'):
            units(unit(ENTRIES[:-3]))
        with self.assertRaisesRegex(Unread, 'runs past its section'):
            units(unit(ENTRIES)[:-1])

    def test_fails_on_a_unit_length_it_doesn_t_read(self):
        # 0xFFFFFFFF opens a unit of 64-bit DWARF, and DWARF reserves the lengths from 0xFFFFFFF0 to 0xFFFFFFFE
        rest = unit(ENTRIES)[4:]
        for length, failure in ((0xFFFFFFFF, '64-bit DWARF'), (0xFFFFFFF0, 'runs past its section'),
                                (struct.calcsize(HEADER) - 5, 'shorter than its header')):
            with self.subTest(failure), self.assertRaisesRegex(Unread, failure):
                units(struct.pack('<I', length) + rest)
        with self.assertRaisesRegex(Unread, 'unit cut short'):
            units(unit(ENTRIES)[:struct.calcsize(HEADER) - 1])

    def test_fails_on_a_list_of_children_that_doesn_t_close_or_a_null_outside_any_list(self):
        with self.assertRaisesRegex(Unread, "doesn't close"):
            units(unit(ENTRIES[:-1]))
        with self.assertRaisesRegex(Unread, 'outside any list'):
            units(unit(ENTRIES + b'\0'))

    def test_fails_on_a_unit_whose_top_isn_t_one_compilation_unit_s_entry(self):
        with self.assertRaisesRegex(Unread, 'second entry at the top'):
            units(unit(ENTRIES + INT))
        with self.assertRaisesRegex(Unread, 'whose entry has tag'):
            units(unit(INT))

    def test_fails_on_an_abbreviation_given_twice_or_with_a_children_flag_past_1(self):
        with self.assertRaisesRegex(Unread, 'abbreviation 1 twice'):
            dwarf.abbreviations(abbrev(1, Tag.BASE_TYPE, 0) + abbrev(1, Tag.VARIABLE, 0) + b'\0', 0)
        with self.assertRaisesRegex(Unread, 'children flag 2'):
            dwarf.abbreviations(abbrev(1, Tag.BASE_TYPE, 2) + b'\0', 0)

    def test_fails_on_another_version_kind_or_address_size(self):
        for kwargs, failure in (({'version': 4}, 'DWARF 4'), ({'kind': 4}, 'kind 4'), ({'address': 8}, '8-byte')):
            with self.subTest(failure), self.assertRaisesRegex(Unread, failure):
                units(unit(ENTRIES, **kwargs))

    def test_records_a_reference_outside_its_unit_on_its_entry(self):
        # n's type refers to the offset where the unit ends
        (read,) = units(unit(A_C + INT + variable(struct.calcsize(HEADER) + len(ENTRIES)) + b'\0'))
        (n,) = [die for die in read.dies.values() if die.tag == Tag.VARIABLE]
        self.assertRegex(str(n.unread), 'reference outside its unit')
        self.assertNotIn(At.TYPE, n.attrs)

    def test_records_a_string_its_section_doesn_t_hold_whole_on_its_entry(self):
        # .debug_str holds ab without a NUL, and the variable's name lies at its start or at its end
        for offset in (0, 2):
            with self.subTest(offset=offset):
                (read,) = units(unit(A_C + uleb(4) + struct.pack('<I', offset) + b'\0'), strings=b'ab')
                (entry,) = [die for die in read.dies.values() if die.tag == Tag.VARIABLE]
                self.assertRegex(str(entry.unread), "string its section doesn't hold whole")
                self.assertNotIn(At.NAME, entry.attrs)

    def test_bounds_a_version_3_line_table_s_files_by_its_header(self):
        # the table lists no directory, and the file a.c in directory 0 with no time or length. A 0 ends each list
        lists = b'\0' + b'a.c\0' + uleb(0) + uleb(0) + uleb(0) + b'\0'
        self.assertEqual(dwarf.file_names(line_table(3, lists), 0, {}), [None, 'a.c'])
        with self.assertRaisesRegex(Unread, "files don't end in its header"):
            dwarf.file_names(line_table(3, lists, len(FIELDS[3]) + len(lists) - 1), 0, {})

    def test_leaves_a_version_5_line_table_s_directory_0_to_the_compilation_s(self):
        # the table lists the directories build, the compilation's, and sub. It puts a.c in directory 0, and b.c in
        # directory 1
        lists = (entry_list([PATH], [b'build\0', b'sub\0'])
                 + entry_list([PATH, INDEX], [b'a.c\0' + uleb(0), b'b.c\0' + uleb(1)]))
        self.assertEqual(dwarf.file_names(line_table(5, lists), 0, {}), ['a.c', 'sub/b.c'])

    def test_fails_on_a_version_5_line_table_without_directory_0_or_a_file_outside_its_directories(self):
        build = entry_list([PATH], [b'build\0'])
        for lists, failure in ((entry_list([PATH], []) + entry_list([PATH, INDEX], [b'a.c\0' + uleb(0)]),
                                'without directory 0'),
                               (build + entry_list([PATH, INDEX], [b'a.c\0' + uleb(1)]), 'directory 1, outside')):
            with self.subTest(failure), self.assertRaisesRegex(Unread, failure):
                dwarf.file_names(line_table(5, lists), 0, {})

    def test_fails_on_a_directory_index_in_a_form_dwarf_5_doesn_t_give_it(self):
        # DW_FORM_sdata, which could give the index -1, and DW_FORM_implicit_const, whose value no abbreviation of a
        # line table's entry holds
        build = entry_list([PATH], [b'build\0'])
        for form, data, failure in ((Form.SDATA, b'\x7f', 'in DW_FORM_sdata'),
                                    (Form.IMPLICIT_CONST, b'', 'no abbreviation holds')):
            lists = build + entry_list([PATH, (Lnct.DIRECTORY_INDEX, form)], [b'a.c\0' + data])
            with self.subTest(form), self.assertRaisesRegex(Unread, failure):
                dwarf.file_names(line_table(5, lists), 0, {})


if __name__ == '__main__':
    unittest.main()
