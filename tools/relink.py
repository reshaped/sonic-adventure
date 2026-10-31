"""Link all of an image's objects in one pass, with R32's lnk, and compare
the result with the splicer's image.

link.py links each source alone, with every name it takes from outside
bound at the address the tables give it, and the splicer joins the links.
relink links, in one pass, the full objects under build/ of every source
the image's splits.txt places, so that lnk resolves each name between two
of them and lays out each section itself:

- lnk joins a section's objects in input order, so each section gets its
  objects in the order its rows place them; where no one order of the
  objects does that for every section, relink fails, naming the sections
  that disagree. Objects no section orders follow table order.
- Each section starts at its row, or in 1ST_READ.BIN, which has no
  section rows, where Config.layout() starts it. A section with neither
  starts at its extent, if it has one, as in link.py.
- A symbols object defines each name some object takes from outside and
  no object defines, at the address the tables give it: the names the
  image takes from other images, and the names its tables give inside it,
  which relink binds by address, as each source's link does. A name the
  tables don't give fails.
- Bss the tables leave without an object, before an object they place
  there, is reserved, so the objects after it link where the rows put
  them; the tree holds no object for it. Where a section starts at its
  extent and the extent has an end, the rest of the extent after the
  section's objects is reserved, as link.py reserves it, so that STARTOF
  and SIZEOF give the section-boundary tables the image's bounds.
- lnk keeps one module of a name, and SHC and asmsh name an object's
  module after the object's file, which the build names after its
  source's file. Where a source's file name is an earlier source's, or one
  relink gives its own objects, symbols, hole1, hole2 and so on, relink
  links a copy of the source's object whose module is named apart.
- ALIGN_SECTION lets lnk join the objects of a section that align it
  differently, as R10's and R32's objects do.

The link writes its files to build/relink/IMAGE/, among them lnk's map,
link.map. relink fails where the map holds no externally defined symbols
list. Where the list lacks a name of the image's symbols.txt that an
object of the link exports, or gives one another address than the name's
row does, relink fails, naming each such row. The map lists only the names
objects export, so a row whose name no object exports, such as a static's,
goes unchecked. Once every check passes, relink writes the image to
build/relink/IMAGE.bin, then, with --shift, build/relink/IMAGE/fixed.txt,
which make takes as the shift test's result, so a failed check leaves both
as they were. The image runs from its base to the end of its last section
that isn't bss: bss past the file isn't stored. Given the splicer's image,
relink compares the two byte for byte, and a difference fails, naming the
image, the first address that differs, or where the shorter of the two
ends, and what the splicer says writes the splice's byte there, where it
holds one. --verbose lists each stand-in for what the tree lacks: each
name bound by address, each run of space reserved and each module named
apart. WIBO and R32 name wibo and R32's directory, as for link.py.

--shift adds the shift test. relink links the image twice more, each time
one of SHIFTS past its base, into build/relink/IMAGE/shift1/ and shift2/:
each section and each name the tables give inside the image move by the
shift, and the names it takes from other images stay where they are. An
object that aligns a section to more than the shifts keep fails first,
since the shifted links would lay the image out anew. Each word must then
move by each shift, or keep its value under both. Where words do neither,
relink fails, naming how many and the first: its address, its values and
what the splicer says writes it.

The image's span runs from its first section's start to its last's end,
at either of RAM's two addresses (MIRROR). A word that keeps a value in
it is a candidate fixed address: a number where a relocatable build needs
a name, an address the game fixes whatever the image's layout, such as
where a file loads over the image, or data that only looks like one.
config/kept.txt gives the reason each candidate takes no name, and relink
fails on a candidate it gives no reason and on a row of the image's that
holds no candidate, naming each. relink writes a line for each candidate to
build/relink/IMAGE/fixed.txt: the image, the word's address, its value,
what writes it and the reason; and prints how many words move.
"""

import argparse
import re
import struct
import sys
from pathlib import Path

from code_ranges import writers
from config import Config, order
from images import ZEROED, images
from kept import KEPT, reasons
from link import define, dos, grouped, place, section, symbols
from shc_object import definitions, records, references, section_table
from tables import aligned, hexa, named, rows
from tool import content, fail, save, stoppable

# the kinds of SHC object record relink reads beside tools/shc_object.py's:
# the two headers that name the object's module
HEADERS = (0x04, 0x06)
# the shift test's two shifts past an image's base. Each is a multiple of
# 32, the most an object of the tree aligns a section to, so the image keeps
# its layout, and relink fails on an object that aligns one to more. Each is
# nonzero in both halves, so a word that holds half of an address moves by
# other than the shift
SHIFTS = (0x01234560, 0x02468AC0)
# the bit that tells a byte of RAM's two addresses apart, 0x0c... and
# 0x8c...: the game uses both (TOOLCHAIN.md#images)
MIRROR = 0x80000000
# the alignment of the space relink reserves: asmsh warns on a section
# aligned to less
LEAST = 4
# a row of the externally defined symbols list of lnk's map: a C name the
# link exports, with its leading underscore, its address and how it is
# defined. A name past 32 characters takes a line of its own, and its
# address the next
EXPORTED = re.compile(r"^_(\w+)\s+H'([0-9A-F]{8})\s+\w+\r?$", re.M)


def renamed(obj, old, new):
    """The object's bytes with its module named new, not old, in each of the
    two headers that name it, which must name old once. new is as long as
    old, so each header keeps its size, and takes its checksum anew."""
    old, new = (bytes([len(name)]) + name.encode() for name in (old, new))
    parts, found = [], 0
    for at, record in records(obj):
        if record[0] & 0x7F in HEADERS:
            head = record[:-1]
            if head[2:].count(old) != 1:
                fail(f'{obj}: its header at {at:#x} doesn\'t name its module once')
            head = head[:2] + head[2:].replace(old, new)
            record = head + bytes([(0xFF - sum(head)) & 0xFF])
            found += 1
        parts.append(record)
    if found != len(HEADERS):
        fail(f'{obj}: {len(HEADERS)} headers name its module, and it has {found}')
    return b''.join(parts)


def apart(stem, taken):
    """A module name as long as stem that taken doesn't hold: stem with its
    end replaced by the lowest number that gives one, up to the largest that
    fits stem's length. Fails where taken holds every such name."""
    for number in range(1, 10 ** len(stem)):
        name = stem[:len(stem) - len(str(number))] + str(number)
        if name not in taken:
            return name
    fail(f'{stem}: each module name of its length that ends in a number is taken')


def holes(config, declared, starts):
    """Each run of bss the tables hold no object in, ahead of an object they
    place, as (section, start, end, source), source the object after it:
    where an object's first row in a bss section starts past the end of the
    object before it, or the section's start in starts, rounded up to its
    alignment. declared is each source's section table, by source."""
    found = []
    for name in ZEROED:
        firsts = {}
        for source, label, start, _ in sorted(config.placements, key=lambda row: row[2]):
            if label == name:
                firsts.setdefault(source, start)
        at = starts.get(name)
        for source, start in firsts.items():
            if name not in declared[source]:
                fail(f'{source}: its {name} row names no section of its object')
            size, alignment, _ = declared[source][name]
            if start > aligned(at, alignment):
                found.append((name, at, start, source))
            at = start + size
    return found


def tails(config, pieces, starts):
    """The rest of each extent after its section's pieces, as (section,
    start, end, code), where the section starts at the extent's start and
    the extent has an end. pieces gives each piece of each section in input
    order, as (size, alignment, code); lnk aligns no empty piece. A section
    that runs past its extent fails."""
    found = []
    for name, start, end in config.extents():
        if name not in pieces or starts.get(name) != start or end is None:
            continue
        at = start
        for size, alignment, _ in pieces[name]:
            if size:
                at = aligned(at, alignment) + size
        if at > end:
            fail(f'{config.splits}: the whole link\'s {name} runs to {hexa(at)}, past {hexa(end)}, '
                 f'where tools/images.py ends it')
        if at < end:
            found.append((name, at, end, pieces[name][0][2]))
    return found


def reserve(config, name, start, end, code=False):
    """The space from start to end in section name as relink's objects
    reserve it, (name, size, alignment, code): on LEAST, from start rounded
    up to it. Space too short for that fails."""
    after = aligned(start, LEAST)
    if after >= end:
        fail(f'{config.splits}: {name} {hexa(start)} to {hexa(end)} is too short to reserve on {LEAST}')
    return name, end - after, LEAST, code


def bindings(config, objects, names):
    """Each name some of objects, by source, takes from outside and none
    defines, as (address, name) in the order of names, the tables' pairs;
    then the addresses of those the tables give inside the image, by name.
    relink's symbols object defines the first, and so binds the second by
    address, as each source's link does. A name the tables don't give
    fails."""
    defined = set().union(*map(definitions, objects.values()))
    wanted = {}
    for source, obj in objects.items():
        for name in references(obj) - defined:
            wanted.setdefault(name[1:], source)
    given = {name for _, name in names}
    for name, source in wanted.items():
        if name not in given:
            fail(f'{source}: takes {name}, which no object of {config.name} defines and its tables don\'t name')
    low, high = config.span()
    bound = [(address, name) for address, name in names if name in wanted]
    return bound, {name: address for address, name in bound if low <= address < high}


def image(config, written):
    """The image the link writes, from written, the bytes its S-records
    write by address: from the image's base to the end of its last section
    that isn't bss. A byte written outside that fails."""
    layout = config.layout()
    base, end = config.base, max(high for name, _, high in layout if name not in ZEROED)
    stray = [address for address in written if not base <= address < end]
    if stray:
        fail(f'{config.name}: the whole link writes {len(stray):,} bytes outside its file, from {hexa(min(stray))}')
    # a byte the S-records leave unwritten holds 0 in bss and the image's
    # fill byte elsewhere, as in the splicer's file, where the linker's gaps
    # and the space an object reserves hold it
    image = bytearray([config.fill]) * (end - base)
    for name, low, high in layout:
        if name in ZEROED and low < end:
            image[low - base:min(high, end) - base] = bytes(min(high, end) - low)
    for address, byte in written.items():
        image[address - base] = byte
    return bytes(image)


def link(config, inputs, starts, bound, inside, reserved, at, shift=0):
    """The image the whole link writes, as image() gives it, with its files
    in the directory at. lnk links inputs, the objects in input order, then
    a symbols object that defines bound, (address, name) pairs, and reserves
    reserved, as define() takes them, and starts each section at its address
    in starts. A shift moves each section, and each name of inside, the
    names the tables give inside the image, that many bytes on; the names
    the image takes from others stay where they are, and the image still
    holds each byte at its unshifted address."""
    names = [(address + shift if name in inside else address, name) for address, name in bound]
    objects = [*inputs, define(at / 'symbols.src', names, reserved)]
    placing = grouped({name: start + shift for name, start in starts.items()}, config.extents())
    written = place(objects, placing, at / 'link', 'align_section', f'print {dos(at / "link.map")}')
    return image(config, {address - shift: byte for address, byte in written.items()})


def words(data):
    """data's 4-byte little-endian words, the last filled out with zeros."""
    data += bytes(-len(data) % 4)
    return struct.unpack(f'<{len(data) // 4}I', data)


def writer(found, address):
    """What writes the byte at address, by found, the (start, end, writer)
    ranges code_ranges.writers() gives: a source, the linker's fill or a
    gen/ range's padding; or nothing."""
    return next((by for start, end, by in found if start <= address < end), 'nothing')


def shifted(config, image, links):
    """Check each word of image, the whole link at the image's base, against
    the same word of each of links, its links at SHIFTS's shifts as link()
    gives them: it moves by each shift, or keeps its value under both.
    Return how many words move, and each that keeps a value in the image's
    span, or in the span's mirror, as (address, value, what writes it).
    Where words do neither, fail, naming how many and the first: its
    address, its values and what the splicer says writes it."""
    low, high = config.span()
    moved, kept, problems = 0, [], []
    for at, (own, *others) in enumerate(zip(*map(words, (image, *links)))):
        moves = tuple((other - own) % (1 << 32) for other in others)
        if moves == SHIFTS:
            moved += 1
        elif any(moves):
            problems.append((config.base + 4 * at, own, others))
        elif low <= own < high or low <= own ^ MIRROR < high:
            kept.append((config.base + 4 * at, own))
    found = writers(config, section) if problems or kept else []
    if problems:
        address, own, others = problems[0]
        first = (f'{config.name}: its word at {hexa(address)}, which {writer(found, address)} writes, holds '
                 f'{hexa(own)}, and {" and ".join(map(hexa, others))} at the shifts')
        if len(problems) > 1:
            fail(f'{config.name}: {len(problems):,} of its words move, but not by each shift', first)
        fail(first)
    return moved, [(address, value, writer(found, address)) for address, value in kept]


def reviewed(config, kept, rows):
    """kept, the words shifted() finds keeping a value in the image's span,
    each with the reason of the row of rows, config/kept.txt's for the
    image, that holds it. Where a word has no row, or a row holds no word,
    fail, naming each."""
    found, used, problems = [], set(), []
    for address, value, by in kept:
        row = next((row for row in rows if row[0] <= address < row[1]), None)
        if row is None:
            problems.append(f'{config.name}: its word at {hexa(address)}, which {by} writes, keeps {hexa(value)}, '
                            f'in its span, and {KEPT} gives no reason')
            continue
        used.add(row)
        found.append((address, value, by, row[2]))
    for start, end, _, where in (row for row in rows if row not in used):
        problems.append(f'{where}: no word of {config.name} from {hexa(start)} to {hexa(end)} keeps a value in its '
                        'span')
    if problems:
        fail(*problems)
    return found


def compare_names(config, path, exported):
    """Check the image's symbols.txt against lnk's map of the whole link at
    path. The map must hold its externally defined symbols list, and the
    list a row for each name of the table in exported, the names the input
    and symbols objects export. A name not in exported, such as a static's,
    goes unchecked. Fails without the list, and where the list lacks a
    required name or gives one another address than its row does, naming
    each such row."""
    _, heading, listed = content(path).decode('latin-1').partition('EXTERNALLY DEFINED SYMBOLS LIST')
    if not heading:
        fail(f'{path}: no externally defined symbols list, so it may be cut short')
    found = {name: int(address, 16) for name, address in EXPORTED.findall(listed)}
    table = config.path / 'symbols.txt'
    problems = []
    for (where, _), (address, name) in zip(rows(table), named(table)):
        if name in exported and name not in found:
            problems.append(f'{where}: names {name}, which the whole link of {config.name} exports, and {path} gives '
                            'it no address')
        elif name in found and found[name] != address:
            problems.append(f'{where}: puts {name} at {hexa(address)}, and the whole link of {config.name} defines '
                            f'it at {hexa(found[name])}')
    if problems:
        fail(*problems)


def compare(config, image, path):
    """Fail where image, the whole link's, differs from the splicer's at
    path, naming the first address that differs, or where the shorter of the
    two ends, and what the splicer says writes the splice's byte there, where
    it holds one."""
    spliced = content(path)
    if image == spliced:
        return
    at = next((at for at, (own, other) in enumerate(zip(image, spliced)) if own != other),
              min(len(image), len(spliced)))
    address = config.base + at
    if at < len(spliced):
        by = writer(writers(config, section), address)
        theirs = f'its byte at {hexa(address)}, which {by} writes, is {spliced[at]:#04x}'
        ours = f'writes {image[at]:#04x}' if at < len(image) else 'ends before it'
    else:
        theirs, ours = f'ends at {hexa(address)}', f'writes {image[at]:#04x} there'
    fail(f'{path}: {theirs}, and the whole link of {config.name} {ours}')


def main(argv):
    parser = argparse.ArgumentParser(prog='relink', description=__doc__.split('\n\n')[0])
    parser.add_argument('image', metavar='IMAGE', help='the image to link, config/IMAGE/')
    parser.add_argument('spliced', metavar='SPLICED', type=Path, nargs='?',
                        help="the splicer's image to compare the link with, such as build/SONICADV/STG01.BIN")
    parser.add_argument('--shift', action='store_true',
                        help='link the image at two shifted bases too, and write the words that keep a value in '
                             'its span to build/relink/IMAGE/fixed.txt')
    parser.add_argument('--verbose', action='store_true',
                        help='list the names the link binds by address, the space it reserves and the modules it '
                             'names apart')
    args = parser.parse_args(argv)
    if args.image not in images():
        fail(f'config/{args.image}/: no such image')
    config = Config(args.image)
    out = Path('build/relink', f'{config.name}.bin')
    side = out.with_suffix('')
    # the directory of each shifted link's files, and its shift
    shifts = {side / f'shift{number}': shift for number, shift in enumerate(SHIFTS, 1)} if args.shift else {}
    try:
        for directory in (side / 'modules', *shifts):
            directory.mkdir(parents=True, exist_ok=True)
    except OSError as error:
        # the directory, or one above it, whichever mkdir couldn't make
        fail(f'{error.filename}: {error.strerror}')
    sources = order(config)
    objects = {source: Path('build', source).with_suffix('.obj') for source in sources}
    for source, obj in objects.items():
        if not obj.is_file():
            fail(f'{source}: no object at {obj}, so build the image first')
    declared = {source: section_table(obj) for source, obj in objects.items()}
    for source, table in declared.items():
        for name, (_, alignment, _) in table.items():
            if any(shift % alignment for shift in shifts.values()):
                fail(f'{source}: aligns {name} to {alignment}, more than the shift test\'s shifts keep')
    starts = {}
    for name, start, _ in config.layout():
        starts.setdefault(name, start)
    for name, start, _ in config.extents():
        starts.setdefault(name, start)
    report = []

    # the space before an object that no object holds, reserved by an object
    # of its own just ahead of it in input order. own holds the modules of
    # relink's objects, which asmsh names after their files
    ahead, own = {}, {'symbols'}
    for number, (name, start, end, source) in enumerate(holes(config, declared, starts), 1):
        piece = reserve(config, name, start, end)
        ahead.setdefault(source, []).append((define(side / f'hole{number}.src', [], [piece]), piece))
        own.add(f'hole{number}')
        report.append(f'reserves {name} {hexa(start)} to {hexa(end)}, ahead of {source}, where no object lies')

    inputs, pieces, taken = [], {}, own | {Path(source).stem for source in sources}
    seen = set(own)
    for source in sources:
        for obj, (name, size, alignment, code) in ahead.get(source, []):
            inputs.append(obj)
            pieces.setdefault(name, []).append((size, alignment, code))
        obj, stem = objects[source], Path(source).stem
        if stem in seen:
            module = apart(stem, taken)
            taken.add(module)
            obj = side / 'modules' / f'{module}.obj'
            save(obj, renamed(objects[source], stem, module))
            report.append(f'links {source} as module {module}, since another module is named {stem} too')
        seen.add(stem)
        inputs.append(obj)
        for name, piece in declared[source].items():
            pieces.setdefault(name, []).append(piece)

    reserved = []
    for name, start, end, code in tails(config, pieces, starts):
        reserved.append(reserve(config, name, start, end, code))
        report.append(f'reserves {name} {hexa(start)} to {hexa(end)}, the rest of its extent')
    names = symbols(config, *(obj.with_suffix('.syms') for source, obj in objects.items()
                              if source.startswith('gen/')))
    bound, inside = bindings(config, objects, names)
    # the names the whole link exports: the C names its objects define, and those its symbols object defines
    exported = {name[1:] for obj in objects.values() for name in definitions(obj) if name.startswith('_')}
    exported |= {name for _, name in bound}
    for name, address in inside.items():
        holder = next((f"in {source}'s {label}" for source, label, start, end in config.placements
                       if start <= address < end), 'where no object lies')
        report.append(f'binds {name} at {hexa(address)} by address, {holder}')

    sections = {name: start for name, start in starts.items() if name in pieces}
    relinked = link(config, inputs, sections, bound, inside, reserved, side)
    if args.verbose:
        for line in report:
            print(f'{config.name}: {line}')
    compare_names(config, side / 'link.map', exported)
    if args.spliced is not None:
        compare(config, relinked, args.spliced)
    report = None
    if shifts:
        links = [link(config, inputs, sections, bound, inside, reserved, at, shift) for at, shift in shifts.items()]
        moved, kept = shifted(config, relinked, links)
        kept = reviewed(config, kept, reasons().get(config.name, []))
        report = ''.join(f'{config.name}  {hexa(address)}  {hexa(value)}  {by}  {reason}\n'
                         for address, value, by, reason in kept).encode()
    save(out, relinked)
    if report is not None:
        # after the image, which make's rule for it takes as a prerequisite
        save(side / 'fixed.txt', report)
        print(f'  shift  {config.name}: each shift moves {moved:,} of its words, and keeps {len(kept):,} at a value '
              'in its span')


if __name__ == '__main__':
    stoppable()
    main(sys.argv[1:])
