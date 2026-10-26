import os
import subprocess
import unittest
from pathlib import Path

import sums
from support import tree

HASH = '0' * 64
# the images config/ describes: 1ST_READ.BIN, and MOD.PRS, which decodes to MOD.BIN
IMAGES = {'config/1st_read/splits.txt': '', 'config/mod/splits.txt': ''}
ORIGINALS = ['orig/1ST_READ.BIN', 'orig/SONICADV/MOD.PRS']


def check(listed, expected, out=None):
    # run sums.py with SHA256SUMS holding the listed lines and build/expected.txt the expected paths, on one line as
    # $(file) writes them. Where out names a file, return the lines sums.py writes there for sha256sum -c
    files = dict(IMAGES, **{'SHA256SUMS': ''.join(f'{line}\n' for line in listed),
                            'build/expected.txt': ' '.join(expected) + '\n'})
    with tree(files):
        sums.main(['build/expected.txt'] if out is None else ['build/expected.txt', out])
        return Path(out).read_text().splitlines() if out else None


class Sums(unittest.TestCase):
    def test_accepts_one_line_for_each_expected_file(self):
        check([f'{HASH}  build/a.bin', f'{HASH}  orig/1ST_READ.BIN'], ['orig/1ST_READ.BIN', 'build/a.bin'])

    def test_rejects_an_original_config_doesnt_describe(self):
        with self.assertRaisesRegex(SystemExit, r'orig/SONICADV/OLD\.PRS: in SHA256SUMS, and config/ describes no such file'):
            check([f'{HASH}  orig/1ST_READ.BIN', f'{HASH}  orig/SONICADV/OLD.PRS'], ['orig/1ST_READ.BIN'])

    def test_rejects_a_line_out_of_form(self):
        with self.assertRaisesRegex(SystemExit, r'SHA256SUMS:2: not a hash, two spaces and a path'):
            check([f'{HASH}  build/a.bin', f'{HASH} build/b.bin'], ['build/a.bin'])

    def test_ends_a_line_at_a_newline_alone(self):
        # make check has awk pick the lines sha256sum -c checks by their second field, and awk ends a line at \n
        # alone: there a CR is the path's last character, and a form feed joins two lines into one
        paths = ['build/a.bin', 'build/b.bin']
        lines = [f'{HASH}  {path}' for path in paths]
        for listed in ([f'{lines[0]}\r', f'{lines[1]}\r'], [f'{lines[0]}\f{lines[1]}']):
            with self.subTest(listed=listed), \
                    self.assertRaisesRegex(SystemExit, r'SHA256SUMS:1: not a hash, two spaces and a path'):
                check(listed, paths)
        # awk reads a last line that has no \n, and so must sums.py
        text = '\n'.join(lines)
        awk = subprocess.run(['awk', '{ print $2 }'], input=text.encode(), stdout=subprocess.PIPE, check=True)
        self.assertEqual(awk.stdout, ''.join(f'{path}\n' for path in paths).encode())
        with tree(dict(IMAGES, **{'SHA256SUMS': text, 'build/expected.txt': ' '.join(paths)})):
            sums.main(['build/expected.txt'])

    def test_names_a_line_that_isnt_utf_8(self):
        with tree(dict(IMAGES, **{'SHA256SUMS': f'{HASH}  build/a.bin\n{HASH}  build/caf\xe9.bin\n'.encode('latin-1'),
                                  'build/expected.txt': 'build/a.bin'})), \
                self.assertRaisesRegex(SystemExit, r"\A[^\n]*: SHA256SUMS:2: a byte that isn't UTF-8\Z"):
            sums.main(['build/expected.txt'])

    def test_names_an_expected_line_that_isnt_utf_8(self):
        with tree(dict(IMAGES, **{'SHA256SUMS': '', 'build/expected.txt': b'build/a.bin\n\xff\n'})), \
                self.assertRaisesRegex(SystemExit, r"build/expected\.txt:2: a byte that isn't UTF-8"):
            sums.main(['build/expected.txt'])

    def test_rejects_an_output_listed_twice(self):
        # a path equal to the one above it doesn't sort before it, so the count alone names it
        with self.assertRaisesRegex(SystemExit, r'\A[^\n]*: build/a\.bin: in SHA256SUMS 2 times\Z'):
            check([f'{HASH}  build/a.bin'] * 2, ['build/a.bin'])

    def test_reports_every_missing_and_stale_line_at_once(self):
        with self.assertRaisesRegex(SystemExit, r'build/b\.bin: not in SHA256SUMS\n.*build/old\.bin: in SHA256SUMS, and make expects no line for it'):
            check([f'{HASH}  build/a.bin', f'{HASH}  build/old.bin'], ['build/a.bin', 'build/b.bin'])

    def test_holds_a_rebuilt_original_to_its_originals_line(self):
        # MOD.PRS's line holds orig/ and build/ to one hash, which a line of build/'s own could contradict; MOD.BIN,
        # which no original is, keeps its own line, and make rebuilds no EV0054.PRS, which repeats another event
        lines = [f'{"4" * 64}  build/SONICADV/MOD.BIN', f'{"1" * 64}  orig/1ST_READ.BIN',
                 f'{"3" * 64}  orig/SONICADV/EV0054.PRS', f'{"2" * 64}  orig/SONICADV/MOD.PRS']
        expected = [*ORIGINALS, 'orig/SONICADV/EV0054.PRS', 'build/SONICADV/MOD.BIN']
        self.assertEqual(check(lines, expected, 'build/sums.txt'),
                         [*lines, f'{"1" * 64}  build/1ST_READ.BIN', f'{"2" * 64}  build/SONICADV/MOD.PRS'])
        with self.assertRaisesRegex(SystemExit, r"build/SONICADV/MOD\.PRS: in SHA256SUMS, and make checks it against "
                                                r"orig/SONICADV/MOD\.PRS's line"):
            check([lines[0], f'{"5" * 64}  build/SONICADV/MOD.PRS', *lines[1:]], expected)

    def test_wants_the_line_each_rebuilt_file_takes(self):
        # build/SONICADV/MOD.PRS takes orig/SONICADV/MOD.PRS's hash, which build/expected.txt need not list
        with self.assertRaisesRegex(SystemExit, r'orig/SONICADV/MOD\.PRS: not in SHA256SUMS'):
            check([f'{HASH}  orig/1ST_READ.BIN'], ['orig/1ST_READ.BIN'], 'build/sums.txt')
        self.assertEqual(check([f'{HASH}  orig/1ST_READ.BIN', f'{HASH}  orig/SONICADV/MOD.PRS'], [], 'build/sums.txt'),
                         [f'{HASH}  orig/1ST_READ.BIN', f'{HASH}  orig/SONICADV/MOD.PRS', f'{HASH}  build/1ST_READ.BIN',
                          f'{HASH}  build/SONICADV/MOD.PRS'])

    def test_holds_an_event_that_repeats_another_to_its_hash(self):
        lines = [f'{HASH}  orig/SONICADV/EV0029.PRS', f'{HASH}  orig/SONICADV/EV0054.PRS']
        expected = ['orig/SONICADV/EV0029.PRS', 'orig/SONICADV/EV0054.PRS']
        check(lines, expected)
        with self.assertRaisesRegex(SystemExit, r"orig/SONICADV/EV0054\.PRS: in SHA256SUMS with a hash other than "
                                                r"orig/SONICADV/EV0029\.PRS's, which it repeats"):
            check([lines[0], f'{"1" * 64}  orig/SONICADV/EV0054.PRS'], expected)

    def test_holds_the_lines_to_path_order(self):
        # by byte: B before a, and a.bin before a/b.bin before a_b.bin, which a locale's collation can reorder; with the
        # middle two swapped, line 3 alone sorts before the line above it
        paths = ['build/B.bin', 'build/a.bin', 'build/a/b.bin', 'build/a_b.bin']
        lines = [f'{HASH}  {path}' for path in paths]
        sort = subprocess.run(['sort', '-k', '2'], input=''.join(f'{line}\n' for line in reversed(lines)),
                              env=dict(os.environ, LC_ALL='C'), stdout=subprocess.PIPE, text=True, check=True)
        self.assertEqual(sort.stdout.splitlines(), lines)
        check(lines, paths)
        with self.assertRaisesRegex(SystemExit, r'\A[^\n]*: SHA256SUMS:3: build/a\.bin sorts before the line above it; '
                                                r'LC_ALL=C sort -k 2 orders it\Z'):
            check([lines[0], lines[2], lines[1], lines[3]], paths)
        # build/B.bin sorts before build/a.bin, two lines up, but the line directly above it is out of form
        with self.assertRaisesRegex(SystemExit, r'\A[^\n]*: SHA256SUMS:2: not a hash, two spaces and a path\Z'):
            check([lines[1], f'{HASH} build/x.bin', lines[0]], paths[:2])

    def test_names_a_missing_input(self):
        with tree(IMAGES), self.assertRaisesRegex(SystemExit, r'build/expected\.txt: No such file or directory'):
            sums.main(['build/expected.txt'])
        with tree(dict(IMAGES, **{'build/expected.txt': ''})), self.assertRaisesRegex(SystemExit, r'SHA256SUMS: No such file'):
            sums.main(['build/expected.txt'])


if __name__ == '__main__':
    unittest.main()
