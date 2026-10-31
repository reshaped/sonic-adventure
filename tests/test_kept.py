import unittest
from pathlib import Path

from config import Config
from kept import reasons
from support import MODULE, MOTION, cwd, tree

ROOT = Path(__file__).resolve().parents[1]


class Reasons(unittest.TestCase):
    def reasons(self, *rows):
        with tree({**MODULE, **MOTION, 'config/kept.txt': ''.join(f'{row}\n' for row in rows)}):
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
                             ('none  0x0c900010  text', 'none: no binary make shift tests'),
                             ('x_mot  0x0cc00000  text', 'x_mot: no binary make shift tests'),
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


if __name__ == '__main__':
    unittest.main()
