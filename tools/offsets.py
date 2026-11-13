"""Read, write and check a body source's table of offsets, which records
where its data sits in the original.

    python3 tools/offsets.py SOURCE...

A body source takes bodies, texts or values from the disc
(TOOLCHAIN.md#stub-and-full-builds). Its table is offsets/ and its path
with .txt for its .c: offsets/src/camera/modes.txt for src/camera/modes.c.
For each datum the source names, each of its automatic arrays that takes a
body and each of its texts, the table gives the section that holds it, its
offset in the source's part of that section and its size. An image's
splits.txt gives the address that part starts at, so the two give the
address of each on the disc, where tools/data.py --bodies reads it.

That is a fact about the original, as splits.txt and symbols.txt hold
others: a map from the names a source gives its data to where each sits.
The matching build measures it: tools/data.py --offsets writes a table from
the listing of the source's stub build. make check compares each table
under offsets/ with the one a fresh listing gives, and make offsets copies
those in.

A table is true of the original whatever its source's text says, so it
records nothing of that text, and no change to the source makes it stale. A
row is wrong only where the source gives its datum another name or another
size, and a datum, an array or a text the source adds has no row until one
is written. tools/data.py's BodySource, which reads a source's offsets,
fails on a body, an automatic array, a text or a datum that holds a value
they don't place, and on a size the source doesn't declare. On a host that
runs no SHC, a datum the source renames takes its new name in its row by
hand, and make check holds the row on x86 Linux as it does any other. This
tool checks only that each source has a table that reads, so it runs where
no stub build does: make dc-check runs it.

Each row is

    KIND  NAME  SECTION  OFFSET  SIZE

SECTION is a section's full name, OFFSET is 0x and eight lowercase hex
digits, and SIZE is in bytes, in decimal. KIND is one of:

- datum: a datum of static storage that the source names, a file's or a
  function's. The rows come in the order of the stub build's listing. Two
  functions' statics can share a name, and each has its row;
- template: a function's automatic array that takes a body, by the array's
  name, at the template the array is copied from;
- text: a text, TEXT_n, with the null that ends it.

A row holds no more than its fields, and a # starts a comment.

The tool checks each SOURCE's table, and fails, naming each that is
missing, and on a row that isn't one.
"""

import argparse
import re
import sys
from collections import Counter, namedtuple
from pathlib import Path

from tables import NAME, SECTION, address, hexa, rows
from tool import fail, publish

# the directory of the tables
ROOT = Path('offsets')
# a row's kinds, in the order a table gives its rows
DATUM, TEMPLATE, TEXT = KINDS = ('datum', 'template', 'text')
# a size: at most nine digits, so that int() reads any it matches
SIZE = re.compile(r'0|[1-9][0-9]{0,8}')
# how a table comes to be written, and where the docs give the tables: the
# stop for a missing table says both, and so does the one for a table its
# source differs from
WRITTEN = 'make offsets writes it on x86 Linux'
SEE = 'see TOOLCHAIN.md#stub-and-full-builds'


class Offsets(namedtuple('Offsets', 'data templates texts')):
    """Where a body source's data sits in the original, as its table records
    it:

    - data: each datum the source names, as (name, section, offset, size),
      in the table's order;
    - templates: each automatic array's template, as {the array's name:
      (section, offset, size)};
    - texts: each text with its null, as {the text's name: (section, offset,
      size)}.

    A section is its full name, an offset is into the source's part of that
    section, and a size is in bytes."""

    def named(self):
        """Each named datum's section, offset and size, by its name, in
        data's order. Of the data two statics of one name give, the name
        keeps the first's place in that order and the last's section, offset
        and size."""
        return {name: (section, offset, size) for name, section, offset, size in self.data}

    def shared(self):
        """The names two statics share."""
        return {name for name, count in Counter(name for name, *_ in self.data).items() if count > 1}


def table(source):
    """The path of the table of the C source at path source."""
    return ROOT / Path(source).with_suffix('.txt')


def spell(offsets):
    """The text of the table that records offsets, an Offsets."""
    found = [(DATUM, *datum) for datum in offsets.data]
    found += [(TEMPLATE, name, *place) for name, place in offsets.templates.items()]
    found += [(TEXT, name, *place) for name, place in offsets.texts.items()]
    return ''.join(f'{kind}  {name}  {section}  {hexa(offset)}  {size}\n'
                   for kind, name, section, offset, size in found)


def parsed(found):
    """The Offsets that a table's rows record. found is those rows, as
    tables.rows() gives them. Fails, naming the row's path and
    line, on a row that isn't KIND NAME SECTION OFFSET SIZE as the module
    docstring gives them, and on a second template or text row of one
    name."""
    data, templates, texts = [], {}, {}
    for where, fields in found:
        if len(fields) != 5 or fields[0] not in KINDS or not NAME.fullmatch(fields[1]) or \
                not SECTION.fullmatch(fields[2]) or not SIZE.fullmatch(fields[4]):
            fail(f'{where}: not a row: KIND NAME SECTION OFFSET SIZE')
        kind, name, section = fields[:3]
        place = section, address(fields[3], where), int(fields[4])
        if kind == DATUM:
            data.append((name, *place))
            continue
        held = templates if kind == TEMPLATE else texts
        if name in held:
            fail(f'{where}: {name} has a {kind} row already')
        held[name] = place
    return Offsets(data, templates, texts)


def write(path, offsets):
    """Write the table at path, which records offsets, an Offsets. Fails,
    and writes nothing, where the table's rows wouldn't read back as
    offsets: where parsed() fails on one, as on a name or a section a row
    can't spell."""
    text = spell(offsets)
    if parsed((f'{path}:{number}', line.split()) for number, line in enumerate(text.splitlines(), 1)) != offsets:
        fail(f'{path}: its rows don\'t read back as the offsets they record')
    publish(path, text)


def missing(path, source):
    """What to say of the table at path, of the C source at path source,
    where it isn't a file, and None where it is one."""
    if path.is_file():
        return None
    return (f'{path}: missing; it records where {source}\'s data sits in the original. {WRITTEN}; a renamed '
            f'source takes a git mv; {SEE}')


def differing(path):
    """What to say of the table at path once a source is found to differ
    from the offsets it records, after the difference."""
    return f'{path} records its offsets, and {WRITTEN}; {SEE}'


def read(path, source):
    """The Offsets that the table at path records for the C source at path
    source. Fails, naming the table, where it is missing, as missing()
    says, and where parsed() fails on a row."""
    path = Path(path)
    stopped = missing(path, source)
    if stopped:
        fail(stopped)
    return parsed(rows(path))


def check(sources):
    """Check that each of sources, C sources' paths, has a table that reads.
    Fails, naming each table that is missing, and then where read() fails
    on one."""
    problems = [stopped for source in sources if (stopped := missing(table(source), source))]
    if problems:
        fail(*problems)
    for source in sources:
        read(table(source), source)


def main(argv):
    parser = argparse.ArgumentParser(prog='offsets', description=__doc__.split('\n\n')[0])
    parser.add_argument('sources', nargs='+', metavar='SOURCE', help="a body source, whose table is checked")
    check(parser.parse_args(argv).sources)


if __name__ == '__main__':
    main(sys.argv[1:])
