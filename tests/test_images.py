import unittest
from pathlib import Path

from images import EVENT_ALIASES, EXTENTS, binaries, files, motions
from support import MODULE, tree

ROOT = Path(__file__).resolve().parents[1]


class Images(unittest.TestCase):
    def test_tells_a_motion_file_by_its_sections_none_of_them_code(self):
        # hero's one section, D, holds no code, so hero is a motion file. 1ST_READ.BIN's splits.txt gives no section,
        # blank has no splits.txt at all, and sdk's code is PSG, after D, so each is a binary
        tables = {'config/hero/splits.txt': ('D  0x0cc00000  0x0cc00100\n'
                                             'gen/hero/d_0cc00000.c  D  0x0cc00000  0x0cc00100\n'),
                  'config/hero/symbols.txt': '', 'config/blank/symbols.txt': '',
                  'config/sdk/splits.txt': 'D  0x0cd00000  0x0cd00100\nPSG  0x0cd00100  0x0cd00200\n'}
        with tree(dict(MODULE, **tables)):
            self.assertEqual(motions(), ['hero'])
            self.assertEqual(binaries(), ['1st_read', 'blank', 'mod', 'sdk'])

    def test_takes_an_image_with_a_row_in_code_out_of_form_for_a_binary(self):
        # torn's row for P has no end, and loose places a source in P, which no row of its declares: each image's
        # Config refuses that row, so it must be a binary, whose Config the build reads
        tables = {'config/torn/splits.txt': 'D  0x0cd00000  0x0cd00100\nP  0x0cd00100\n',
                  'config/loose/splits.txt': ('D  0x0ce00000  0x0ce00100\n'
                                              'modules/loose/a.c  P  0x0ce00100  0x0ce00110\n')}
        with tree(dict(MODULE, **tables)):
            self.assertEqual(motions(), [])
            self.assertEqual(binaries(), ['1st_read', 'loose', 'mod', 'torn'])

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


if __name__ == '__main__':
    unittest.main()
