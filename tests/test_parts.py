"""config/parts.txt, which gives each source its thing and its bin, as tools/parts.py reads it: a row's form, the row
a source takes, the directories a thing takes, and that each tracked source takes a row and each row a source."""

import re
import subprocess
import unittest
from pathlib import Path

import parts
from parts import Part
from support import cwd, tree

ROOT = Path(__file__).resolve().parents[1]
# a table: a directory of each of two things, a directory under one, and files under them, one of both. The backend's
# two directories lie out of order
TABLE = ('# path  thing  bin\n'
         'src/  game  play\n'
         'src/draw/model.c  game  both  # a layer that includes a walk\n'
         '\n'
         'engine/dc/sa1/  drawer  picture  # the walk\n'
         'engine/native/  backend  play\n'
         'engine/dc/math.c  game  play\n'
         'engine/dc/  backend  picture\n'
         'engine/tests/  tool  none\n')
AT = 'config/parts.txt'
ROWS = {'src/': Part('game', 'play', f'{AT}:2'), 'src/draw/model.c': Part('game', 'both', f'{AT}:3'),
        'engine/dc/sa1/': Part('drawer', 'picture', f'{AT}:5'), 'engine/native/': Part('backend', 'play', f'{AT}:6'),
        'engine/dc/math.c': Part('game', 'play', f'{AT}:7'), 'engine/dc/': Part('backend', 'picture', f'{AT}:8'),
        'engine/tests/': Part('tool', 'none', f'{AT}:9')}
# files that take each of TABLE's rows
FILES = ['src/main.c', 'src/draw/model.c', 'engine/dc/sa1/model.c', 'engine/dc/owner.c', 'engine/dc/math.c',
         'engine/native/shim.c', 'engine/tests/abi.c']


def rows(table):
    """parts.rows() of a table of table's text."""
    with tree({AT: table}):
        return parts.rows()


class Rows(unittest.TestCase):
    def test_reads_each_row_by_its_path_with_its_thing_its_bin_and_its_line(self):
        self.assertEqual(rows(TABLE), ROWS)

    def test_reads_the_table_at_the_path_it_is_given_and_names_that_path(self):
        # as engine/tests/boundary.py reads it, by the root's path
        with tree({'other/table.txt': 'src/  game  play\nsdk/  sdk\n'}) as root:
            with self.assertRaisesRegex(SystemExit, rf': {re.escape(str(root))}/other/table\.txt:2: not a row: '):
                parts.rows(root / 'other/table.txt')
        with tree({'other/table.txt': '\nsrc/  game  play\n'}) as root:
            self.assertEqual(parts.rows(root / 'other/table.txt'),
                             {'src/': Part('game', 'play', f'{root}/other/table.txt:2')})

    def test_takes_each_thing_and_each_bin_and_no_other(self):
        for thing in parts.THINGS:
            with self.subTest(thing=thing):
                self.assertEqual(rows(f'src/  {thing}  play\n')['src/'].thing, thing)
        for held in parts.BINS:
            with self.subTest(bin=held):
                self.assertEqual(rows(f'src/  game  {held}  # a reason\n')['src/'].bin, held)
        for row, problem in (('src/  engine  play', 'engine: not a thing, one of game, sdk, play-math, drawer, '
                                                    'recorder, contract, backend, tool'),
                             ('src/  Game  play', 'Game: not a thing, one of game, '),
                             ('src/  game  inputs', 'inputs: not a bin, one of play, picture, both, none'),
                             ('src/  play  game', 'play: not a thing, one of game, ')):
            with self.subTest(row=row), self.assertRaisesRegex(SystemExit, rf': {AT}:2: {re.escape(problem)}'):
                rows(f'# a comment\n{row}\n')

    def test_rejects_a_row_of_fewer_or_more_fields_than_three(self):
        for row in ('src/', 'src/  game', 'src/  game  play  picture', 'src/  game  play  both  # a reason'):
            with self.subTest(row=row), self.assertRaisesRegex(SystemExit, rf': {AT}:3: not a row: PATH THING BIN$'):
                rows(f'sdk/  sdk  play\n\n{row}\n')

    def test_a_row_of_both_says_why_after_a_hash_and_any_row_may(self):
        self.assertEqual(rows('src/a.c  game  both  # a layer\nsrc/  game  play  # the game\n'),
                         {'src/a.c': Part('game', 'both', f'{AT}:1'), 'src/': Part('game', 'play', f'{AT}:2')})
        for row in ('src/a.c  game  both', 'src/a.c  game  both  #', 'src/a.c  game  both  #  '):
            with self.subTest(row=row), self.assertRaisesRegex(
                    SystemExit, rf': {AT}:1: src/a\.c is both, and its row says nothing after a #: say there what '
                                'its play half and its picture half are$'):
                rows(f'{row}\n')

    def test_rejects_a_second_row_for_a_path(self):
        # a directory and a file of one name but the / are two paths
        self.assertEqual(set(rows('src/  game  play\nsrc  game  play\n')), {'src/', 'src'})
        with self.assertRaisesRegex(SystemExit, rf': {AT}:3: src/a\.c has a row already, at {AT}:1$'):
            rows('src/a.c  game  play\nsrc/  game  play\nsrc/a.c  game  both  # a reason\n')


class Taken(unittest.TestCase):
    def test_a_source_takes_the_longest_row_that_is_it_or_a_directory_above_it(self):
        # a source of each suffix
        for path, row in (('src/main.c', 'src/'), ('src/draw/model.c', 'src/draw/model.c'),
                          ('src/draw/model.h', 'src/'), ('src/draw/model.c/a.h', 'src/'),
                          ('engine/dc/sa1/model.c', 'engine/dc/sa1/'), ('engine/dc/sa1/deep/a.h', 'engine/dc/sa1/'),
                          ('engine/dc/owner.c', 'engine/dc/'), ('engine/dc/math.c', 'engine/dc/math.c'),
                          ('engine/dc/a.cpp', 'engine/dc/'), ('engine/dc/a.s', 'engine/dc/'),
                          ('src/system/vector.src', 'src/'), ('engine/dc/shaders/area.comp', 'engine/dc/'),
                          ('engine/dc/shaders/draw.frag', 'engine/dc/'), ('engine/dc/shaders/draw.vert', 'engine/dc/')):
            with self.subTest(path=path):
                self.assertEqual(parts.part(ROWS, path), ROWS[row])

    def test_a_source_under_no_row_takes_none(self):
        # a directory's row is no row of a path that only starts as it does, nor of the directory less its /
        for path in ('engine/pc/raster.cpp', 'engine/a.c', 'srcs/a.c', 'a/src/main.c'):
            with self.subTest(path=path):
                self.assertIsNone(parts.part(ROWS, path))
        self.assertIsNone(parts.part({'src/a.c/': Part('game', 'play', f'{AT}:1')}, 'src/a.c'))

    def test_a_file_that_is_no_source_takes_none_whatever_row_lies_above_it(self):
        # a script, a make file, a linker script, a run's keys, a directory, a file of a suffix no build compiles or
        # of none, and a script that has a row of its own
        found = {**ROWS, 'engine/tools/stubs.py': Part('tool', 'none', f'{AT}:10')}
        for path in ('engine/tools/marks.py', 'engine/native/play.lua', 'engine/dc/sa1/walk.mk',
                     'engine/tests/sh4.ld', 'engine/dc/play.keys', 'engine/dc/sa1/', 'src', 'src/.clang-tidy',
                     'src/main.inc', 'src/main.cc', 'src/main', 'engine/tools/stubs.py', ''):
            with self.subTest(path=path):
                self.assertIsNone(parts.part(found, path))


class Directories(unittest.TestCase):
    def test_gives_the_directories_of_a_thing_in_order_and_of_one_bin_among_them(self):
        # in path order, which isn't the rows'
        self.assertEqual(parts.directories(ROWS, 'backend'), ['engine/dc/', 'engine/native/'])
        self.assertEqual(parts.directories(ROWS, 'backend', 'picture'), ['engine/dc/'])
        self.assertEqual(parts.directories(ROWS, 'backend', 'play'), ['engine/native/'])
        self.assertEqual(parts.directories(ROWS, 'drawer'), ['engine/dc/sa1/'])
        # a file's row is no directory
        self.assertEqual(parts.directories(ROWS, 'game'), ['src/'])

    def test_fails_where_the_table_gives_the_thing_no_directory(self):
        for asked, problem in ((('recorder',), 'no row gives a directory to recorder, and a check takes its '
                                               'directories from one: give the directory a row, PATH/ recorder BIN'),
                               (('drawer', 'play'), 'no row gives a directory to drawer as play, and a check takes '
                                                    'its directories from one: give the directory a row, PATH/ '
                                                    'drawer play'),
                               (('game', 'both'), 'no row gives a directory to game as both, ')):
            with self.subTest(asked=asked), self.assertRaisesRegex(SystemExit, rf': {AT}: {re.escape(problem)}'):
                parts.directories(ROWS, *asked)


class Faults(unittest.TestCase):
    def test_finds_none_where_each_source_takes_a_row_and_each_row_a_source(self):
        # a file that is no source, or lies under none of the roots, needs no row: a root is a whole directory's name
        self.assertEqual(parts.faults(ROWS, [*FILES, 'engine/engine.mk', 'engine/pc/sh4.ld',
                                             'engine/numeric/a.py', 'tools/parts.py', 'config/a.c', 'README.md',
                                             'srcs/a.c', 'engine2/a.c', 'sdk.c']),
                         [])

    def test_names_each_source_under_a_root_that_takes_no_row(self):
        # a source of each suffix, one under each root but src, which has a row
        unplaced = ['sdk/a.c', 'modules/mod/a.h', 'events/ev/a.src', 'loader/a.c', 'engine/pc/a.cpp',
                    'engine/numeric/a.s', 'engine/pc/shaders/area.comp', 'engine/pc/shaders/draw.frag',
                    'engine/pc/shaders/draw.vert']
        self.assertEqual(parts.faults(ROWS, [*FILES, *unplaced]),
                         [f'{path}: no row of {AT} gives its thing and its bin: give it one, or a directory above it, '
                          'PATH THING BIN' for path in unplaced])

    def test_names_each_row_that_no_source_takes(self):
        unused = "no source git tracks takes this row: none lies at or under its PATH, or each that does takes a " \
                 "longer row. Remove the row; a directory's PATH ends in /"
        # no file under a directory, a file that isn't there, and a directory whose every file takes a longer row
        for gone, where in (('engine/tests/abi.c', f'{AT}:9: engine/tests/'),
                            ('src/draw/model.c', f'{AT}:3: src/draw/model.c'),
                            ('engine/dc/owner.c', f'{AT}:8: engine/dc/')):
            with self.subTest(gone=gone):
                self.assertEqual(parts.faults(ROWS, [path for path in FILES if path != gone]), [f'{where}: {unused}'])
        # a directory that holds scripts alone, and a script's own row
        scripts = [path for path in FILES if path != 'engine/tests/abi.c'] + ['engine/tests/boundary.py']
        self.assertEqual(parts.faults(ROWS, scripts), [f'{AT}:9: engine/tests/: {unused}'])
        found = {**ROWS, 'engine/tests/boundary.py': Part('tool', 'none', f'{AT}:10')}
        self.assertEqual(parts.faults(found, [*FILES, 'engine/tests/boundary.py']),
                         [f'{AT}:10: engine/tests/boundary.py: {unused}'])
        # a directory's row without its /, which is a file's
        found = rows(TABLE.replace('engine/native/  ', 'engine/native  '))
        self.assertEqual(parts.faults(found, FILES),
                         [f'engine/native/shim.c: no row of {AT} gives its thing and its bin: give it one, or a '
                          'directory above it, PATH THING BIN', f'{AT}:6: engine/native: {unused}'])


@unittest.skipUnless((ROOT / '.git').exists(), 'not a git checkout, so no list of the files it tracks')
class Tree(unittest.TestCase):
    maxDiff = None

    def test_each_tracked_source_takes_a_row_and_each_row_a_tracked_source(self):
        with cwd(ROOT):
            listed = subprocess.run(['git', 'ls-files', '-z'], stdout=subprocess.PIPE, check=True).stdout
            self.assertEqual(parts.faults(parts.rows(), [path for path in listed.decode().split('\0') if path]), [])


if __name__ == '__main__':
    unittest.main()
