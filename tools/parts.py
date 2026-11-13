"""Give each source its thing and its bin, as config/parts.txt's rows have
them.

config/parts.txt says of each source what it is part of, its thing, and who
reads what it computes, its bin. Its head defines each thing and each bin,
and says which files take a row. A row is

    PATH  THING  BIN

PATH is a file, or a directory, which ends in /, as git names it from the
repository's root. A source takes the row whose PATH is the longest that is
the source itself or a directory above it. A source is a file a build
compiles, one whose suffix is among SOURCES. The table speaks for no other
file: a script, a make file or a linker script takes no row, whatever
directory holds it. THING is one of THINGS and BIN one of BINS. A row whose
BIN is both ends with a # comment, which says what the file's play half and
its picture half are. Any other row may end with one.

rows() reads the table. It fails, naming the row's path:line, on a row that
isn't PATH THING BIN, on a thing or a bin the table has none of, on a both
that says nothing after a #, and on a second row for one PATH. part() gives
the row a source takes, and directories() the directories a thing takes.
faults() gives what the rows leave wrong of the files git tracks: a source
under SOURCE_ROOTS that takes no row, and a row that no source takes.
tests/test_parts.py holds the table to none.
"""

from collections import namedtuple
from pathlib import Path

from images import CONFIG, SOURCE_ROOTS
from tables import lines
from tool import fail

PARTS = CONFIG / 'parts.txt'
# the names a row may give its thing and its bin, which no other file sets.
# The table's head says what each is, and tests/test_names.py holds its two
# lists to THINGS and BINS
THINGS = ('game', 'sdk', 'play-math', 'drawer', 'recorder', 'contract', 'backend', 'tool')
PLAY, PICTURE, BOTH = 'play', 'picture', 'both'
BINS = (PLAY, PICTURE, BOTH, 'none')
# a source's suffixes: C, C++, their headers, assembly as GNU as and as asmsh
# read it, and the PC's shaders
SOURCES = ('.c', '.cpp', '.h', '.s', '.src', '.comp', '.frag', '.vert')

# a row's thing and bin, and its path:line
Part = namedtuple('Part', 'thing bin where')


def rows(path=PARTS):
    """The rows of the table at path, as {PATH: Part}. Fails, naming the
    row's path:line, on a row out of the form the module docstring gives."""
    found = {}
    for number, line in lines(path):
        row, _, reason = line.partition('#')
        fields = row.split()
        if not fields:
            continue
        where = f'{path}:{number}'
        if len(fields) != 3:
            fail(f'{where}: not a row: PATH THING BIN')
        name, thing, bin = fields
        if thing not in THINGS:
            fail(f'{where}: {thing}: not a thing, one of {", ".join(THINGS)}')
        if bin not in BINS:
            fail(f'{where}: {bin}: not a bin, one of {", ".join(BINS)}')
        if bin == BOTH and not reason.strip():
            fail(f'{where}: {name} is both, and its row says nothing after a #: say there what its play half and '
                 'its picture half are')
        if name in found:
            fail(f'{where}: {name} has a row already, at {found[name].where}')
        found[name] = Part(thing, bin, where)
    return found


def source(path):
    """Whether path, a string, is a source: a file whose suffix is one of
    SOURCES."""
    return Path(path).suffix in SOURCES


def part(found, path):
    """The Part of the row that path, a string, takes among found, rows()'s:
    the row whose PATH is the longest that is path or a directory above it.
    None where found has no such row, and where path is no source."""
    if not source(path):
        return None
    above = [at + 1 for at, character in enumerate(path) if character == '/']
    for end in (len(path), *reversed(above)):
        if path[:end] in found:
            return found[path[:end]]
    return None


def directories(found, thing, bin=None):
    """The directories that found, rows()'s, gives thing, as their rows
    spell them, in order: those of bin alone, where bin names one. Fails
    where it gives none: a check that takes its directories from the table
    would then hold nothing."""
    given = sorted(path for path, row in found.items()
                   if path.endswith('/') and row.thing == thing and bin in (None, row.bin))
    if not given:
        fail(f'{PARTS}: no row gives a directory to {thing}{f" as {bin}" if bin else ""}, and a check takes its '
             f'directories from one: give the directory a row, PATH/ {thing} {bin or "BIN"}')
    return given


def faults(found, files):
    """What found, rows()'s, leaves wrong of files, the paths git tracks:
    each source under SOURCE_ROOTS that takes no row, and each row that no
    source of files takes."""
    taken, problems = set(), []
    for path in files:
        row = part(found, path)
        if row is not None:
            taken.add(row.where)
        elif source(path) and path.startswith(tuple(f'{root}/' for root in SOURCE_ROOTS)):
            problems.append(f'{path}: no row of {PARTS} gives its thing and its bin: give it one, or a directory '
                            'above it, PATH THING BIN')
    problems += [f'{row.where}: {path}: no source git tracks takes this row: none lies at or under its PATH, or each '
                 "that does takes a longer row. Remove the row; a directory's PATH ends in /"
                 for path, row in found.items() if row.where not in taken]
    return problems
