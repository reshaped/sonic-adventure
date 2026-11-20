import unittest
from pathlib import Path

import shc_listing
from support import tree

# the row that gives a D section of 0x20 bytes its size, in SHC's listing
SIZE = 'DATA     SECTION (D):                               00000020 Byte(s)'


def label(offset, name):
    return f'D   {offset:08X}              L{offset + 1}:{" " * 28}; static: {name}'


def directive(offset, operands, op='.DATA.L'):
    return f'    {offset:08X} {"":10} {op:12}{operands}'


def assembled_listing(rows):
    # asmsh's listing of a D section of rows: (offset, text, code), or a
    # continuation's (offset, None, code)
    lines = ['\f*** SuperH RISC engine ASSEMBLER Ver. 4.0 ***    10/20/26 16:26:38', 'PROGRAM NAME =', '',
             f'    1 00000000 {"":16}     1               .SECTION    D,DATA,ALIGN=4']
    for number, (offset, text, *code) in enumerate(rows, 2):
        code = code[0] if code else ''
        if text is None:
            lines.append(f'      {offset:08X} {code}')
        elif text.startswith('L'):
            lines.append(f'{number:5} {offset:08X} {code:16} {number:5}     {text}')
        else:
            lines.append(f'{number:5} {offset:08X} {code:16} {number:5}               {text}')
    lines += ['*** SECTION DATA LIST', '', 'SECTION                          ATTRIBUTE    SIZE             START', '',
              'D                                REL-DATA    000000020        ']
    return '\n'.join(lines) + '\n'


class Listing(unittest.TestCase):
    def test_gives_each_named_datum_a_row_in_the_listings_order(self):
        # two functions' statics named at, with another datum between them
        rows = [label(0, 'at'), directive(0, "H'00000000,H'00000000"), label(8, 'other'), directive(8, "H'00000000"),
                label(12, 'at'), directive(12, "H'00000000"), SIZE]
        with tree({'a.lis': '\n'.join(rows) + '\n'}):
            self.assertEqual(shc_listing.listing(Path('a.lis'))[0],
                             [('at', 'D', 0, 8), ('other', 'D', 8, 4), ('at', 'D', 12, 4)])

    def test_gives_each_unnamed_datum_outside_the_code_and_the_labels_each_function_names(self):
        # f's code names its pool's label and, in its comments, the labels of two unnamed data in D. The pool's
        # label is code's, and no datum
        rows = ['P   00000000              L1:                              ; function: f',
                f'    00000000 {"D201":10} {"MOV.L":12}L2,R2; L40',
                f'    00000002 {"D302":10} {"MOV.L":12}L2+4,R3; L41',
                '    00000004              L2:', f'    00000004 {"<00000000>":10} {".DATA.L":12}L40',
                'D   00000010              L41:', directive(0x10, "H'00000000"),
                'D   00000018              L40:', directive(0x18, "H'00000000,H'00000000"), SIZE]
        with tree({'a.lis': '\n'.join(rows) + '\n'}):
            self.assertEqual(shc_listing.listing(Path('a.lis')),
                             ([], {'L41': ('D', 0x10, 4), 'L40': ('D', 0x18, 8)}, {'f': {'L1', 'L2', 'L40', 'L41'}}))

    def test_an_assembled_listing_keeps_its_sections_full_names(self):
        # asmsh's sections DAT0 and DAT1, which SHC's rows would both show as
        # DAT, and fail on
        text = assembled_listing(
            [(0, 'L1:                              ; static: first'), (0, ".DATA.L     H'00000000", '00000000'),
             (0, '.SECTION    DAT1,DATA,ALIGN=4'), (0, 'L2:                              ; static: second'),
             (0, ".DATA.L     H'00000000,H'00000000", '0000000000000000')])
        text = text.replace('.SECTION    D,', '.SECTION    DAT0,').replace(
            'D                                REL-DATA    000000020',
            'DAT0                             REL-DATA    000000004\n'
            'DAT1                             REL-DATA    000000008')
        shc = '\n'.join(['DAT 00000000              _first:                          ; static: first',
                         directive(0, "H'00000000"), 'D0       SECTION (DAT0):                            00000004 Byte(s)',
                         'D1       SECTION (DAT1):                            00000004 Byte(s)'])
        with tree({'a.lis': text, 'b.lis': shc + '\n'}):
            self.assertEqual(shc_listing.listing(Path('a.lis'))[0], [('first', 'DAT0', 0, 4), ('second', 'DAT1', 0, 8)])
            with self.assertRaisesRegex(SystemExit, r'b\.lis:1: no one section starts DAT$'):
                shc_listing.listing(Path('b.lis'))

    def test_an_assembled_listings_long_line_goes_on_in_its_code(self):
        # a line too long for the listing is cut short, and its code goes on
        # in rows of their own
        text = assembled_listing(
            [(0, 'L1:                              ; static: table'),
             (0, ".DATA.L     H'00000000,H'00000000,H'00000000,H'000", '0000000000000000'),
             (8, None, '0000000000000000')])
        with tree({'a.lis': text}):
            self.assertEqual(shc_listing.listing(Path('a.lis'))[0], [('table', 'D', 0, 16)])

    def test_a_long_global_names_comment_goes_on_in_a_line_of_its_own(self):
        # SHC writes the label of a global whose name has 31 characters or
        # more past its comment's column, and the comment on the next line
        name = 'a' * 31
        text = '\n'.join([f'D   00000000              _{name}:', f'{"":59}; static: {name}',
                          directive(0, "H'00000000,H'00000000"),
                          'DATA     SECTION (D):                               00000008 Byte(s)'])
        with tree({'a.lis': text + '\n'}):
            self.assertEqual(shc_listing.listing(Path('a.lis'))[0], [(name, 'D', 0, 8)])

    def test_a_res_sizes_the_datum_it_starts_and_pads_the_one_whose_values_it_follows(self):
        # two reserved longwords, then a word and the .RES that pads it to the next datum, which adds nothing to
        # the word with a unit or without one, in SHC's listing and in asmsh's
        for padding in ('.RES.W', '.RES'):
            rows = [label(0, 'space'), directive(0, '2', '.RES.L'), label(8, 'word'),
                    directive(8, "H'0001", '.DATA.W'), directive(10, '1', padding), SIZE]
            text = assembled_listing(
                [(0, 'L1:                              ; static: space'), (0, '.RES.L      2', '00000008'),
                 (8, 'L2:                              ; static: word'), (8, ".DATA.W     H'0001", '0001'),
                 (10, f'{padding:12}1', '00000002')])
            with tree({'a.lis': '\n'.join(rows) + '\n', 'b.lis': text}):
                for path in ('a.lis', 'b.lis'):
                    with self.subTest(padding=padding, path=path):
                        self.assertEqual(shc_listing.listing(Path(path))[0],
                                         [('space', 'D', 0, 8), ('word', 'D', 8, 2)])

    def test_a_strings_bytes_give_its_datums_size(self):
        # a ; in a string, and the bytes c2 a3, which UTF-8 would read as one character
        text = '\n'.join([label(0, 'semi'), directive(0, '"a;b"', '.SDATA'), label(4, 'pound'),
                          directive(4, '"\xc2\xa3"', '.SDATA'),
                          'DATA     SECTION (D):                               00000008 Byte(s)'])
        with tree({'a.lis': (text + '\n').encode('latin-1')}):
            self.assertEqual(shc_listing.listing(Path('a.lis'))[0], [('semi', 'D', 0, 3), ('pound', 'D', 4, 2)])

    def test_a_string_holds_any_byte_but_lf(self):
        # 0x85, which str.splitlines() ends a line at, as TEXT_133's placeholder starts with it, in lines that end
        # in CRLF as SHC's do
        rows = ['D   00000000              L1:', directive(0, '"\x85@"', '.SDATA'), directive(2, "H'00", '.DATA.B'),
                'DATA     SECTION (D):                               00000004 Byte(s)']
        with tree({'a.lis': ('\r\n'.join(rows) + '\r\n').encode('latin-1')}):
            self.assertEqual(shc_listing.strings(Path('a.lis')), [('D', 0, b'\x85@\0')])

    def test_a_line_marker_starts_its_section_for_the_strings_too(self):
        # -optimize=0 names P on a line marker ahead of a function, whose
        # pool's label is then code's and starts no string, and a string in D
        rows = ['P                         ; File a.c      , Line 3        ; block',
                '    00000000              L1:                              ; function: f',
                f'    00000000 {"D000":10} {"MOV.L":12}L2,R0', '    00000004              L2:',
                f'    00000004 {"<00000000>":10} {".DATA.L":12}_table', 'D   00000000              L3:',
                directive(0, '"ab"', '.SDATA'), directive(2, "H'00", '.DATA.B'),
                'PROGRAM  SECTION (P):                               00000008 Byte(s)',
                'DATA     SECTION (D):                               00000004 Byte(s)']
        with tree({'a.lis': '\n'.join(rows) + '\n'}):
            self.assertEqual(shc_listing.strings(Path('a.lis')), [('D', 0, b'ab\0')])

    def test_a_label_whose_rows_give_no_characters_starts_no_string(self):
        # an automatic array's template of one longword, which is an unnamed datum and no string; then a string,
        # and an empty one, which is its null alone
        rows = ['D   00000000              L1:', directive(0, "H'00000001"),
                'D   00000004              L2:', directive(4, '"ab"', '.SDATA'), directive(6, "H'00", '.DATA.B'),
                'D   00000008              L3:', directive(8, "H'00", '.DATA.B'), SIZE]
        with tree({'a.lis': '\n'.join(rows) + '\n'}):
            self.assertEqual(shc_listing.listing(Path('a.lis'))[1],
                             {'L1': ('D', 0, 4), 'L2': ('D', 4, 3), 'L3': ('D', 8, 1)})
            self.assertEqual(shc_listing.strings(Path('a.lis')), [('D', 4, b'ab\0'), ('D', 8, b'\0')])

    def test_a_strings_operand_ends_at_its_closing_quote(self):
        # a string that holds a ; and a doubled quote, with a comment past it; text past a string's closing quote,
        # which SHC doesn't write; and asmsh's listing, which cuts a long string short and sizes it by its code
        for operand in ('"a;""b" ; a comment', '"a;""b"; a comment', '"a;""b"  '):
            rows = ['D   00000000              L1:', directive(0, operand, '.SDATA'), SIZE]
            with self.subTest(operand=operand), tree({'a.lis': '\n'.join(rows) + '\n'}):
                self.assertEqual(shc_listing.listing(Path('a.lis'))[1], {'L1': ('D', 0, 4)})
                self.assertEqual(shc_listing.strings(Path('a.lis')), [('D', 0, b'a;"b')])
        for operand in ('"a"junk', '"a"junk ; a comment', '"a" "b"', '"a","b"'):
            rows = ['D   00000000              L1:', directive(0, operand, '.SDATA'), SIZE]
            for read in (shc_listing.listing, shc_listing.strings):
                with self.subTest(operand=operand, read=read.__name__), tree({'a.lis': '\n'.join(rows) + '\n'}), \
                        self.assertRaisesRegex(SystemExit, r'a\.lis:2: \.SDATA "a".*: no whole string$'):
                    read(Path('a.lis'))
        text = assembled_listing([(0, 'L1:                              ; static: name'),
                                  (0, '.SDATA      "abcdefghijklmnopqrstu', '6162636465666768'),
                                  (8, None, '696A6B6C6D6E6F70'), (16, None, '7172737475767778')])
        with tree({'a.lis': text}):
            self.assertEqual(shc_listing.listing(Path('a.lis'))[0], [('name', 'D', 0, 24)])

    def test_a_listing_fails_naming_the_line_of_a_datum_it_cant_size_or_place(self):
        # a datum past its section's 4 bytes, below a label whose comment goes
        # on a line of its own; a count and a string SHC doesn't write; a
        # .DATA, a .DATAB and a .RES that name no unit; and a file that gives
        # no section's size
        name, size = 'a' * 31, 'DATA     SECTION (D):                               00000004 Byte(s)'
        for rows, problem in (
                ([f'D   00000000              _{name}:', f'{"":59}; static: {name}', directive(0, "H'00000000"),
                  label(4, 'x'), directive(4, '1, 2, 3, 4'), size], r'a\.lis:4: x runs past its section, D, of 4 bytes$'),
                ([label(0, 'x'), directive(0, "H'10,0", '.DATAB.B'), size],
                 r"a\.lis:2: H'10,0: its count isn't a decimal number$"),
                ([label(0, 'x'), directive(0, '"abc', '.SDATA'), size], r'a\.lis:2: \.SDATA "abc: no whole string$'),
                ([label(0, 'x'), directive(0, "H'00000000", '.DATA'), size],
                 r'a\.lis:2: \.DATA: no \.B, \.W or \.L unit$'),
                ([label(0, 'x'), directive(0, '4,0', '.DATAB'), size], r'a\.lis:2: \.DATAB: no \.B, \.W or \.L unit$'),
                ([label(0, 'x'), directive(0, '4', '.RES'), size], r'a\.lis:2: \.RES: no \.B, \.W or \.L unit$'),
                ([], r"a\.lis: gives no section's size")):
            with self.subTest(problem=problem), tree({'a.lis': '\n'.join(rows) + '\n'}), \
                    self.assertRaisesRegex(SystemExit, problem):
                shc_listing.listing(Path('a.lis'))

    def test_an_assembled_listing_fails_naming_the_line_of_a_count_with_no_unit(self):
        # a .DATAB and a .RES that name no .B, .W or .L. A .DATA that names none is measured by its code, as
        # one that names a unit is
        named = (0, 'L1:                              ; static: x')
        for op in ('.DATAB      4,0', '.RES        4'):
            with self.subTest(op=op), tree({'a.lis': assembled_listing([named, (0, op)])}), \
                    self.assertRaisesRegex(SystemExit, rf'a\.lis:6: \{op.split()[0]}: no \.B, \.W or \.L unit$'):
                shc_listing.listing(Path('a.lis'))
        with tree({'a.lis': assembled_listing([named, (0, ".DATA       H'00000000", '00000000')])}):
            self.assertEqual(shc_listing.listing(Path('a.lis'))[0], [('x', 'D', 0, 4)])

    def test_a_string_fails_naming_the_line_of_a_row_that_doesnt_give_its_bytes(self):
        # a byte SHC doesn't write, a row that starts past the string's bytes,
        # and a string past its section's 4 bytes
        size = 'DATA     SECTION (D):                               00000004 Byte(s)'
        for rows, problem in (
                ([directive(0, "H'4G", '.DATA.B')], r"a\.lis:2: H'4G: no byte$"),
                ([directive(0, '"ab"', '.SDATA'), directive(3, "H'00", '.DATA.B')],
                 r"a\.lis:3: \.DATA\.B starts at 00000003, and its string's bytes end at 00000002$"),
                ([directive(0, '"abcd"', '.SDATA'), directive(4, "H'00", '.DATA.B')],
                 r'a\.lis:1: its string runs past its section, D, of 4 bytes$')):
            with self.subTest(problem=problem), tree({'a.lis': '\n'.join(['D   00000000              L1:', *rows, size])}), \
                    self.assertRaisesRegex(SystemExit, problem):
                shc_listing.strings(Path('a.lis'))


if __name__ == '__main__':
    unittest.main()
