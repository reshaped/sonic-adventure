import unittest
from pathlib import Path

import bodies
from config import configs
from support import LOADER_SECTIONS, tree

# src/x.c's ranges as home() gives them, which its linked source holds in turn: P's 16 bytes, then D's 4
HOME = [('P', 0x1000, 0x1010), ('D', 0x2000, 0x2004)]


class Bodies(unittest.TestCase):
    def spans(self, text, held=HOME):
        with tree({'bodies.txt': text}):
            return bodies.bodies('src/x.c', Path('bodies.txt'), held)

    def rejects(self, text, pattern, held=HOME):
        with self.assertRaisesRegex(SystemExit, pattern):
            self.spans(text, held)

    def test_places_each_body_in_the_linked_source(self):
        # in offset order, whatever the rows' order, and bodies that meet share no byte
        self.assertEqual(self.spans('name  0x00002000  4  text\nmodel  0x00001004  8  NJS_MODEL\nhead  0x00001000  4  Uint32\n'),
                         [(0, 4, False, 'head'), (4, 8, False, 'model'), (16, 4, True, 'name')])

    def test_rejects_a_body_past_its_range(self):
        self.rejects('big  0x00001000  100  text\n', r'big, 0x00001000 to 0x00001064, lies in no one range of src/x\.c')

    def test_rejects_a_body_across_two_ranges(self):
        # every byte lies in src/x.c, and no one range holds them all
        self.rejects('across  0x00001004  8  char\n', r'across, .* lies in no one range',
                     [('D', 0x1000, 0x1008), ('D', 0x1008, 0x1010)])

    def test_rejects_bodies_that_share_bytes(self):
        self.rejects('a  0x00001000  8  char\nb  0x00001004  8  char\n', r"src/x\.c's a and b share bytes")

    def test_rejects_rows_out_of_form(self):
        self.rejects('a  0x00001000  0  char\n', r'0 is no size')
        self.rejects('a  0x00001000  -4  char\n', r'-4 is no size')
        self.rejects('a  0x1000  4  char\n', r'not an address')
        self.rejects('a  0x00001000  4\n', r'bodies\.txt:1: not a row')


# a source 1ST_READ.BIN links, then another image whose ranges keep its bss
SHARED = {
    'config/1st_read/splits.txt': ('sdk/x.c  PSG  0x8c600000  0x8c600010\n'
                                   'sdk/x.c  DSG  0x8c680000  0x8c680008\n'
                                   'sdk/x.c  BSG  0x8c8f0000  0x8c8f0100\n'),
    'config/loader/splits.txt': ''.join(f'{row}\n' for row in LOADER_SECTIONS),
    'sdk/x.c': '',
}
STORED = [('PSG', 0x8c600000, 0x8c600010), ('DSG', 0x8c680000, 0x8c680008)]
LOADED = [('PSG', 0x8ce06d00, 0x8ce06d10), ('DSG', 0x8ce20340, 0x8ce20348)]
BSS = ('BSG', 0x8ce50040, 0x8ce50140)


class Home(unittest.TestCase):
    def home(self, ranges, files=SHARED):
        with tree(files):
            return bodies.home('sdk/x.c', ranges, Path('config/loader/splits.txt'), configs())

    def rejects(self, ranges, files=SHARED, image='1st_read'):
        with self.assertRaisesRegex(SystemExit, rf'sdk/x\.c: its ranges in config/{image}/splits\.txt differ'):
            self.home(ranges, files)

    def test_lets_bss_follow_the_ranges_1st_read_stores(self):
        self.assertEqual(self.home(LOADED + [BSS]), STORED)
        self.assertEqual(self.home(LOADED), STORED)

    def test_rejects_ranges_that_join_another_way(self):
        self.rejects([LOADED[0], BSS, LOADED[1]])
        self.rejects([LOADED[0], ('DSG', 0x8ce20340, 0x8ce20350), BSS])
        self.rejects(LOADED + [('DSG', 0x8ce20350, 0x8ce20358)])

    def test_lets_bss_follow_only_1st_reads_ranges(self):
        files = dict(SHARED, **{
            'config/img/splits.txt': ('PSG  0x0c900000  0x0c900100\n'
                                      'DSG  0x0c900100  0x0c900200\n'
                                      'sdk/x.c  PSG  0x0c900000  0x0c900010\n'
                                      'sdk/x.c  DSG  0x0c900100  0x0c900108\n'),
            'config/img/symbols.txt': '',
        })
        del files['config/1st_read/splits.txt']
        self.rejects(LOADED + [BSS], files, 'img')
