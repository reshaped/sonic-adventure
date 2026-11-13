import re
import sys
import unittest
from pathlib import Path

import offsets
from offsets import Offsets
from support import tree

# a body source, and the table it takes
SOURCE, TABLE = 'src/a.c', Path('offsets/src/a.txt')
# data of each kind: two statics named at, a datum of no bytes, an automatic array's template and two texts
OFFSETS = Offsets([('texlist', 'D', 0, 8), ('at', 'D', 8, 8), ('none', 'D', 16, 0), ('at', 'DHEAD', 0, 4),
                   ('zeroed', 'BSG32', 0x1000, 123456789)],
                  {'pair': ('C', 0x18, 8)}, {'TEXT_1': ('C', 0x10, 5), 'TEXT_2': ('C', 0x20, 2)})
ROWS = ('datum  texlist  D  0x00000000  8\ndatum  at  D  0x00000008  8\ndatum  none  D  0x00000010  0\n'
        'datum  at  DHEAD  0x00000000  4\ndatum  zeroed  BSG32  0x00001000  123456789\n'
        'template  pair  C  0x00000018  8\ntext  TEXT_1  C  0x00000010  5\ntext  TEXT_2  C  0x00000020  2\n')
# the line a table's last row is on
LAST = len(ROWS.splitlines())
MISSING = ("offsets/src/a.txt: missing; it records where src/a.c's data sits in the original. make offsets writes "
           'it on x86 Linux; a renamed source takes a git mv; see TOOLCHAIN.md#stub-and-full-builds')


def read(text):
    # the Offsets of a table of text
    with tree({str(TABLE): text}):
        return offsets.read(TABLE, SOURCE)


class Table(unittest.TestCase):
    def test_a_sources_table_is_its_path_under_offsets(self):
        self.assertEqual(offsets.table('modules/mod/a.c'), Path('offsets/modules/mod/a.txt'))

    def test_spells_a_row_for_each_datum_template_and_text_and_nothing_of_the_source(self):
        self.assertEqual(offsets.spell(OFFSETS), ROWS)

    def test_a_table_reads_back_as_it_was_written_with_no_source_to_read(self):
        with tree({}):
            offsets.write(TABLE, OFFSETS)
            self.assertEqual(TABLE.read_text(), ROWS)
            self.assertEqual(offsets.read(TABLE, SOURCE), OFFSETS)

    def test_a_table_of_no_row_records_no_datum(self):
        for text in ('', '# none\n'):
            with self.subTest(text=text):
                self.assertEqual(read(text), Offsets([], {}, {}))

    def test_a_name_two_statics_share_keeps_its_first_place_in_the_order_and_its_last_offset(self):
        self.assertEqual(list(OFFSETS.named().items()), [
            ('texlist', ('D', 0, 8)), ('at', ('DHEAD', 0, 4)), ('none', ('D', 16, 0)),
            ('zeroed', ('BSG32', 0x1000, 123456789))])
        self.assertEqual(OFFSETS.shared(), {'at'})

    def test_reads_no_comment_and_no_blank_line(self):
        text = "# where src/a.c's data sits\n" + ROWS.replace('datum  at', '\n# two statics\ndatum  at  ', 1)
        self.assertEqual(read(text.replace('  8\ntext', '  8  # copied\ntext')), OFFSETS)

    def test_a_missing_table_fails_saying_what_writes_one(self):
        # no file, and a directory where the file goes
        for files in ({}, {f'{TABLE}/a': ''}):
            with self.subTest(files=files), tree(files), \
                    self.assertRaisesRegex(SystemExit, f': {re.escape(MISSING)}$'):
                offsets.read(TABLE, SOURCE)

    def test_a_row_out_of_its_form_fails_naming_its_line(self):
        # a field too few or too many, a kind no table has, and a name, a section and a size out of their forms
        for row in ('datum  texlist  D  0x00000000', 'datum  texlist  D  0x00000000  8  9',
                    'body  texlist  D  0x00000000  8', 'source  texlist  D  0x00000000  8',
                    'datum  2nd  D  0x00000000  8', 'datum  texlist  d  0x00000000  8',
                    'datum  texlist  D  0x00000000  08', 'datum  texlist  D  0x00000000  0x8',
                    'datum  texlist  D  0x00000000  -8', 'datum  texlist  D  0x00000000  1234567890'):
            with self.subTest(row=row), self.assertRaisesRegex(
                    SystemExit, rf': offsets/src/a\.txt:{LAST + 1}: not a row: KIND NAME SECTION OFFSET SIZE$'):
                read(f'{ROWS}{row}\n')
        for offset in ('0', '00000000', '0x0000000', '0x0000000G', '0x0000000A'):
            with self.subTest(offset=offset), self.assertRaisesRegex(
                    SystemExit, rf': offsets/src/a\.txt:{LAST + 1}: {offset}: not an address'):
                read(f'{ROWS}datum  texlist  D  {offset}  8\n')

    def test_a_second_template_or_text_row_of_one_name_fails(self):
        for row, problem in (('template  pair  C  0x00000020  8', 'pair has a template row already'),
                             ('text  TEXT_2  C  0x00000020  2', 'TEXT_2 has a text row already')):
            with self.subTest(row=row), self.assertRaisesRegex(
                    SystemExit, rf': offsets/src/a\.txt:{LAST + 1}: {problem}$'):
                read(f'{ROWS}{row}\n')

    def test_writes_no_table_whose_rows_wouldnt_read_back(self):
        # a name and a section no row spells, in a datum's row and a text's, and a text's offset in a list, where
        # a table read back gives it in a tuple
        row = 'not a row: KIND NAME SECTION OFFSET SIZE'
        for held, problem in (
                (Offsets([('texlist', 'D', 0, 8), ('a$b', 'D', 8, 4)], {}, {}), f':2: {row}'),
                (Offsets([], {}, {'TEXT_1': ('d', 0, 5)}), f':1: {row}'),
                (Offsets([], {}, {'TEXT_1': ['D', 0, 5]}), ": its rows don't read back as the offsets they record")):
            with self.subTest(held=held), tree({str(TABLE): 'old\n'}):
                with self.assertRaisesRegex(SystemExit, rf': offsets/src/a\.txt{problem}$'):
                    offsets.write(TABLE, held)
                self.assertEqual(TABLE.read_text(), 'old\n')


class Check(unittest.TestCase):
    def test_passes_where_each_source_has_a_table_that_reads_and_reads_no_source(self):
        with tree({str(TABLE): ROWS, 'offsets/modules/mod/b.txt': ''}):
            offsets.main([SOURCE, 'modules/mod/b.c'])

    def test_names_each_table_that_is_missing(self):
        # a.c and c.c have no table, and b.c's reads
        with tree({'offsets/src/b.txt': ROWS}), self.assertRaises(SystemExit) as stopped:
            offsets.main([SOURCE, 'src/b.c', 'src/c.c'])
        tool = Path(sys.argv[0]).stem
        self.assertEqual(str(stopped.exception).split('\n'), [
            f'{tool}: {MISSING}', f'{tool}: {MISSING.replace("src/a.", "src/c.")}'])

    def test_fails_on_a_row_that_isnt_one_naming_its_line(self):
        with tree({str(TABLE): ROWS, 'offsets/src/b.txt': f'{ROWS}datum  texlist  D\n'}), self.assertRaisesRegex(
                SystemExit, rf': offsets/src/b\.txt:{LAST + 1}: not a row: KIND NAME SECTION OFFSET SIZE$'):
            offsets.main([SOURCE, 'src/b.c'])


if __name__ == '__main__':
    unittest.main()
