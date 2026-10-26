"""Read and check each linked image's tables, config/<image>/splits.txt and
symbols.txt, and the shift test's reasons, config/kept.txt.

TOOLCHAIN.md#image-tables gives the rows of splits.txt and symbols.txt and
their rules, and reasons() gives kept.txt's. Every range is [start, end) in
linked addresses. A malformed row fails, naming its file and line.

This module fixes what no table gives:

- 1ST_READ.BIN's section boundaries and game data;
- the game and SDK code of 1ST_READ.BIN and LOADER.BIN;
- the extents of the sections that section-boundary tables bound: strt1's
  in those two images, mpvasec's in MOVIE.BIN, and each image's own in
  B_ROBO.BIN, B_E101_R.BIN and ADVERTISE.BIN;
- each image's original file;
- the four events that repeat another;
- the C runtime an event links, counted as its SDK code;
- the images that take none of 1ST_READ.BIN's names or addresses.
"""

import re
import sys
from pathlib import Path

# the directories that hold the game and SDK sources, as the Makefile's ROOTS
# names them
ROOTS = ('src', 'sdk', 'modules', 'events', 'loader')
CONFIG = Path('config')
# the aligned 32-bit words the shift test may find keeping a value in their
# image's span, each with the reason it takes no name
KEPT = CONFIG / 'kept.txt'
FIRST = CONFIG / '1st_read'
# 1ST_READ.BIN's base, where it links and loads; the starts of P, C, PSG and
# CSG; its game code, SDK code and game data; and the ends of its file and its
# bss. The program opens with the entry's 48 bytes of code and pool. The bytes
# after them, up to P's start, are data: the ID of its module's start record,
# the library handles and the end record. The game's data lies in D between
# strt2's and the SDK's.
FIRST_BASE = 0x8C010000
FIRST_P = 0x8C0100A0
FIRST_C = 0x8C1283CC
FIRST_PSG = 0x8C6143E0
FIRST_CSG = 0x8C6785EC
FIRST_GAME_CODE = [(0x8C0101A0, 0x8C11A580)]
FIRST_SDK_CODE = [(FIRST_BASE, 0x8C010030), (FIRST_P, 0x8C0101A0), (0x8C11A580, FIRST_C), (FIRST_PSG, FIRST_CSG)]
FIRST_GAME_DATA = (0x8C1755E4, 0x8C614208)
FIRST_FILE_END = 0x8C688100
FIRST_END = 0x8C8FD160
# 1ST_READ.BIN has no section rows: P runs to C's start and PSG to CSG's,
# where strt1's tables start. Each is (name, start, end, the section after it).
FIRST_CODE_SECTIONS = (('P', FIRST_P, FIRST_C, 'C'), ('PSG', FIRST_PSG, FIRST_CSG, 'CSG'))
# the other sections of 1ST_READ.BIN whose last placement the linker's fill
# runs from, each as (name, the section after it). The fill runs to the start
# the extents give the section after it. strt1.src declares D32 and CSG32 on
# 32, and njGlobal.c declares DSG32 on 32.
FIRST_TAILS = (('D', 'D32'), ('CSG', 'CSG32'), ('DSG', 'DSG32'))
# the empty sections of 1ST_READ.BIN after D and CSG, by name, and the source
# whose object declares each: no object places a byte in them, so that
# declaration's alignment starts them
FIRST_EMPTY = {'D32': 'sdk/shinobi/strt1.src', 'CSG32': 'sdk/shinobi/strt1.src'}
# the sections strt1's tables bound by STARTOF and SIZEOF, and CSG32, which
# strt1.src declares between CSG and DSG. They are (name, start, end) in link
# order, as the image's link laid them out. Config.check keeps each placement
# in these sections inside its section's bounds().
# - The tables lie at the starts of C and CSG. They hold each section's start,
#   and its end but for D's, D32's, DSG's and DSG32's. strt1's link writes the
#   tables from these values, so SHA256SUMS's hash of the linked strt1 checks
#   every bound they hold.
# - D's and DSG's ends are None, since D32 and DSG32 start on 32 after them.
#   D32 and DSG32 end where R and RSG start.
# - An empty section starts where the next section that holds bytes does, so
#   CSG32 starts where the tables start DSG.
FIRST_EXTENTS = (('DSGLH', FIRST_BASE, 0x8C010080), ('DSGLE', 0x8C010080, FIRST_P),
                 ('D', 0x8C1755E0, None), ('D32', FIRST_PSG, FIRST_PSG),
                 ('R', FIRST_PSG, FIRST_PSG), ('R32', FIRST_PSG, FIRST_PSG),
                 ('CSG32', 0x8C67F280, 0x8C67F280), ('DSG', 0x8C67F280, None), ('DSG32', 0x8C6880C0, FIRST_FILE_END),
                 ('RSG', FIRST_FILE_END, FIRST_FILE_END), ('RSG32', FIRST_FILE_END, FIRST_FILE_END),
                 ('B', FIRST_FILE_END, 0x8C7DD484), ('B32', 0x8C7DD4A0, 0x8C8BF4E0),
                 ('BSG', 0x8C8BF4E0, 0x8C8FB2A0), ('BSG32', 0x8C8FB2A0, FIRST_END))
# LOADER.BIN opens as 1ST_READ.BIN does; its own code follows strt2, and the
# remaining P and PSG are SDK code
LOADER = CONFIG / 'loader'
LOADER_BASE = 0x8CE00000
LOADER_P = 0x8CE000A0
LOADER_PSG = 0x8CE06D00
LOADER_GAME_CODE = [(0x8CE001A0, 0x8CE003E4)]
LOADER_SDK_CODE = [(LOADER_BASE, 0x8CE00030), (LOADER_P, 0x8CE001A0), (0x8CE003E4, 0x8CE064F4), (LOADER_PSG, 0x8CE1F72C)]
# LOADER.BIN's, as FIRST_EXTENTS gives 1ST_READ.BIN's, from strt1's tables at
# 0x8ce064f4 and 0x8ce1f72c
LOADER_EXTENTS = (('DSGLH', LOADER_BASE, 0x8CE00080), ('DSGLE', 0x8CE00080, LOADER_P),
                  ('D', 0x8CE06CC0, None), ('D32', LOADER_PSG, LOADER_PSG),
                  ('R', LOADER_PSG, LOADER_PSG), ('R32', LOADER_PSG, LOADER_PSG),
                  ('DSG', 0x8CE20340, None), ('DSG32', 0x8CE209C0, 0x8CE20A00),
                  ('RSG', 0x8CE20A00, 0x8CE20A00), ('RSG32', 0x8CE20A00, 0x8CE20A00),
                  ('B', 0x8CE20A00, 0x8CE5002C), ('B32', 0x8CE50040, 0x8CE50040),
                  ('BSG', 0x8CE50040, 0x8CE7DD30), ('BSG32', 0x8CE7DD40, 0x8CE7E920))
# MOVIE.BIN's, as FIRST_EXTENTS gives 1ST_READ.BIN's: the sections mpvasec's
# table at 0x8cedc878 bounds; PSGSFD01 starts on 4 after PSGSFD00 ends
MOVIE = CONFIG / 'movie'
MOVIE_EXTENTS = (('PSGSFD00', 0x8CEB0BE0, 0x8CEB25D2), ('PSGSFD01', 0x8CEB25D4, 0x8CEB2BE0),
                 ('PSGSFD02', 0x8CEB2BE0, 0x8CEB8BE0), ('PSGSFD03', 0x8CEB8BE0, 0x8CEB8BE0))
# B_ROBO.BIN's and B_E101_R.BIN's, as FIRST_EXTENTS gives 1ST_READ.BIN's: the
# sections the table at the end of C bounds, D by its start, R and B by their
# starts and ends. D ends where the file does.
B_ROBO = CONFIG / 'b_robo'
B_ROBO_EXTENTS = (('D', 0x0C9095E0, 0x0C930954), ('R', 0x0C930954, 0x0C930954), ('B', 0x0C930958, 0x0C931E58))
B_E101_R = CONFIG / 'b_e101_r'
B_E101_R_EXTENTS = (('D', 0x0C907CC0, 0x0C92E754), ('B', 0x0C92E758, 0x0C92FD94), ('R', 0x0C92FD94, 0x0C92FD94))
# ADVERTISE.BIN's, as FIRST_EXTENTS gives 1ST_READ.BIN's: B and DEND. DEND is
# the table, and holds B's start and its own end.
ADVERTISE = CONFIG / 'advertise'
ADVERTISE_EXTENTS = (('B', 0x8C941F98, 0x8C9C5B28), ('DEND', 0x8C9C5B28, 0x8C9C5B30))
# the images with section-boundary tables, and their extents
EXTENTS = {FIRST: FIRST_EXTENTS, LOADER: LOADER_EXTENTS, MOVIE: MOVIE_EXTENTS, B_ROBO: B_ROBO_EXTENTS,
           B_E101_R: B_E101_R_EXTENTS, ADVERTISE: ADVERTISE_EXTENTS}
# each image's original file, under orig/, where it isn't the default: the
# image's name in capitals with .PRS, in SONICADV/
FILES = {'1st_read': '1ST_READ.BIN', 'loader': 'LOADER.BIN', 'movie': 'SONICADV/MOVIE.BIN'}
# the four of the disc's 208 events that repeat another byte for byte as
# stored, compressed: config/ holds the event each repeats, and SHA256SUMS
# lists each with that event's hash
EVENT_ALIASES = {'EV0054': 'EV0029', 'EV0111': 'EV0101', 'EV0177': 'EV0176', 'EV017B': 'EV017A'}
# the C runtime an event links at the end of its P, counted as SDK code:
# one of these runs of names, as symbols.txt gives them, in this order
EVENT_RUNTIME = ('sscanf', 'strchr', '_fmtin', '_stf', '_calcfpow', '_calcint', '_packf')
EVENT_CONVERSIONS = ('_ftou', '_utof')
# the images that take none of 1ST_READ.BIN's names or addresses
ALONE = (LOADER,)
# the bss sections: zeroed whatever the fill, and not in 1ST_READ.BIN's file
ZEROED = ('B', 'B32', 'BSG', 'BSG32')

ADDRESS = re.compile(r'0x[0-9a-f]{8}')
BYTE = re.compile(r'0x[0-9a-f]{2}')
# a boundary declaration's alignment, in decimal. It takes at most nine
# digits, so that int() reads any it matches.
ALIGNMENT = re.compile(r'[1-9][0-9]{0,8}')
# the most SHC aligns an item of an object's constants, data or bss to: R10
# starts every section on 4, and R32 aligns those items to at most 4
# (TOOLCHAIN.md#alignment)
ITEM_ALIGNMENT = 4
SECTION = re.compile(r'[A-Z][A-Z0-9]*')
NAME = re.compile(r'[A-Za-z_][A-Za-z0-9_]*')
# a placement's source, and a gen/ range's: gen/<image>/d_<address>.c
SOURCE = re.compile(r'(?:[a-z0-9_]+/)+[A-Za-z0-9_]+\.(?:c|src)')
GENERATED = re.compile(r'gen/([a-z0-9_]+)/d_([0-9a-f]{8})\.c')


def fail(*problems):
    """Exit with status 1, writing each line of each problem to stderr after the tool's name."""
    tool = Path(sys.argv[0]).stem
    raise SystemExit('\n'.join(f'{tool}: {line}' for problem in problems for line in str(problem).splitlines()))


def content(path):
    """The bytes of the file at path. One that can't be read fails, naming it."""
    try:
        return Path(path).read_bytes()
    except OSError as error:
        fail(f'{path}: {error.strerror}')


def save(path, data):
    """Write data, bytes, to the file at path. One that can't be written
    fails, naming it. A write that fails partway can leave part of data in
    the file."""
    try:
        Path(path).write_bytes(data)
    except OSError as error:
        fail(f'{path}: {error.strerror}')


def lines(path):
    """Yield each line of the text file at path as its number and its text.
    A line ends at a newline alone, as awk ends one. A line that isn't
    UTF-8 fails, naming its path:line."""
    found = content(path).split(b'\n')
    if found[-1] == b'':
        found.pop()
    for number, line in enumerate(found, 1):
        try:
            yield number, line.decode()
        except UnicodeDecodeError:
            fail(f"{path}:{number}: a byte that isn't UTF-8")


def rows(path):
    """Yield each row of a table as its path:line and its fields, without comments or blank lines."""
    for number, line in lines(path):
        fields = line.split('#')[0].split()
        if fields:
            yield f'{path}:{number}', fields


def table(path):
    """Yield each row of a table as its fields."""
    return (fields for _, fields in rows(path))


def hexa(address):
    """address as the tables write one: 0x and eight lowercase hex digits."""
    return f'0x{address:08x}'


def aligned(address, alignment):
    """address rounded up to a multiple of alignment."""
    return -(-address // alignment) * alignment


def address(field, where):
    """The address a table's field gives. A field that isn't 0x and eight
    lowercase hex digits fails, naming where."""
    if not ADDRESS.fullmatch(field):
        fail(f'{where}: {field}: not an address, 0x and eight lowercase hex digits')
    return int(field, 16)


def interval(start, end, where):
    """A row's [start, end), from its two address fields. One that holds no
    byte fails, naming where."""
    start, end = address(start, where), address(end, where)
    if start >= end:
        fail(f'{where}: {hexa(start)} to {hexa(end)} holds no byte')
    return start, end


def boundaries(fields):
    """The boundary declarations in fields, a placement row's fields after its
    two addresses, as {kind: alignment}.

    A declaration is end, whose alignment is None, or start N or continues N,
    N a power of two of at most nine decimal digits. Each kind appears at most
    once, and start and continues never together. TOOLCHAIN.md#fill gives each
    kind's meaning.

    {} where fields is empty, and None where it breaks these rules."""
    found, rest = {}, list(fields)
    while rest:
        kind = rest.pop(0)
        if kind in found or kind not in ('end', 'start', 'continues'):
            return None
        if kind == 'end':
            found[kind] = None
            continue
        alignment = rest.pop(0) if rest else ''
        if not ALIGNMENT.fullmatch(alignment) or int(alignment) & (int(alignment) - 1) or found.keys() - {'end'}:
            return None
        found[kind] = int(alignment)
    return found


def spell(declaration):
    """A boundary declaration, (kind, alignment), as its row spells it."""
    kind, alignment = declaration
    return kind if alignment is None else f'{kind} {alignment}'


def named(path):
    """A symbols.txt's (address, name) rows. Two names can share an address; a name has one row."""
    found, seen = [], {}
    for where, row in rows(path):
        if len(row) != 2 or not NAME.fullmatch(row[1]):
            fail(f'{where}: not a row: ADDRESS NAME')
        if row[1] in seen:
            fail(f'{where}: {row[1]} has a row already, at {seen[row[1]]}')
        seen[row[1]] = where
        found.append((address(row[0], where), row[1]))
    return found


def reasons():
    """config/kept.txt's rows by image, as lists of (start, end, reason,
    where) in table order. start and end bound the aligned 32-bit words the
    shift test may find keeping a value in the image's span. reason says why
    each takes no name, and where gives the row's path:line.

    A row is IMAGE START [END] REASON. START alone gives the word at START,
    which must lie on 4. With END, the row gives each aligned word whose
    address lies in [START, END).

    A row fails where it:
    - names no image, a directory in config/;
    - holds no word;
    - gives no reason;
    - overlaps an earlier row of its image.
    """
    found, known = {}, set(images())
    for where, row in rows(KEPT):
        if len(row) < 3:
            fail(f'{where}: not a row: IMAGE START [END] REASON')
        image, start, *rest = row
        if image not in known:
            fail(f'{where}: {image}: no such image, a directory in {CONFIG}/')
        if ADDRESS.fullmatch(rest[0]):
            start, end = interval(start, rest.pop(0), where)
            if aligned(start, 4) >= end:
                fail(f'{where}: {hexa(start)} to {hexa(end)} holds no word')
        else:
            start = address(start, where)
            if start % 4:
                fail(f"{where}: {hexa(start)}: not on 4, so no word's address")
            end = start + 4
        if not rest:
            fail(f'{where}: not a row: IMAGE START [END] REASON')
        for low, high, _, earlier in found.get(image, []):
            if start < high and low < end:
                fail(f'{where}: {hexa(start)} to {hexa(end)} overlaps the row at {earlier}')
        found.setdefault(image, []).append((start, end, ' '.join(rest), where))
    return found


class Config:
    """One image's sections, placements, fill byte and names, checked as they're read."""

    def __init__(self, name):
        self.path = CONFIG / name
        self.name = self.path.name
        self.splits = self.path / 'splits.txt'
        self.sections, self.placements, fills, at, rowed = [], [], [], {}, {}
        # the boundary declarations of each gen/ range that has any, by
        # source, as (its row's path:line, boundaries()'s map)
        self.declared = {}
        for where, row in rows(self.splits):
            if len(row) == 2 and row[0] == 'fill':
                if not BYTE.fullmatch(row[1]):
                    fail(f'{where}: {row[1]}: not a byte, 0x and two lowercase hex digits')
                fills.append((where, int(row[1], 16)))
            elif len(row) == 3 and SECTION.fullmatch(row[0]):
                if self.path == FIRST:
                    fail(f"{where}: a section row, and tools/config.py fixes 1ST_READ.BIN's sections")
                if row[0] in rowed:
                    fail(f'{where}: a second row for section {row[0]}, after {rowed[row[0]]}')
                rowed[row[0]] = where
                self.sections.append((row[0], *interval(*row[1:], where)))
            elif (len(row) >= 4 and SOURCE.fullmatch(row[0]) and SECTION.fullmatch(row[1])
                  and (declared := boundaries(row[4:])) is not None):
                placement = (row[0], row[1], *interval(*row[2:4], where))
                if placement in at:
                    fail(f'{where}: repeats the placement at {at[placement]}')
                self.placements.append(placement)
                at[placement] = where
                if declared:
                    if not GENERATED.fullmatch(row[0]):
                        fail(f'{where}: {row[0]} declares {" and ".join(map(spell, declared.items()))}, '
                             'and only a gen/ range declares its boundaries')
                    if declared.get('continues', 0) > ITEM_ALIGNMENT:
                        fail(f'{where}: {row[0]} declares continues {declared["continues"]}, and SHC aligns no item '
                             f"of an object's constants, data or bss past {ITEM_ALIGNMENT}")
                    self.declared[row[0]] = where, declared
            else:
                fail(f'{where}: not a row: SECTION START END, SOURCE SECTION START END [BOUNDARY...], or fill BYTE')
        if len(fills) > 1:
            fail(f'{fills[1][0]}: a second fill row, after {fills[0][0]}')
        self.fill = fills[0][1] if fills else 0
        self.check(at)

    def check(self, at):
        if not self.sections and self.path != FIRST:
            fail(f'{self.splits}: no section rows')
        game, sdk = self.code()
        ordered = sorted(self.sections, key=lambda row: row[1])
        for (name, _, end), (later, start, _) in zip(ordered, ordered[1:]):
            if start < end:
                fail(f'{self.splits}: section {later} starts at {hexa(start)}, inside {name}')
        stems, opened = {}, {}
        for placement in self.placements:
            source, name, start, end = placement
            where = at[placement]
            # link.py starts the object's section at the source's first row in it
            first = opened.setdefault((source, name), placement)
            if start < first[2]:
                fail(f'{where}: {source} starts below its first row in {name}, at {at[first]}')
            # only 1ST_READ.BIN has no section rows: its bss placements lie past
            # its file, and the rest inside it
            if self.sections:
                if not any(section == name and low <= start and end <= high for section, low, high in self.sections):
                    fail(f'{where}: {source} lies outside the section {name}')
            elif not (FIRST_FILE_END <= start if name in ZEROED else (FIRST_BASE <= start and end <= FIRST_FILE_END)):
                fail(f'{where}: {source} lies outside 1ST_READ.BIN\'s {"bss" if name in ZEROED else "file"}')
            generated = GENERATED.fullmatch(source)
            if source.startswith('gen/'):
                if not generated or generated[1] != self.name or int(generated[2], 16) != start:
                    fail(f'{where}: {source}: a gen/ range is gen/{self.name}/d_{start:08x}.c')
                if any(start < high and low < end for low, high in game + sdk):
                    fail(f'{where}: {source} lies in code, and a gen/ range is data from the disc')
            elif not Path(source).is_file():
                fail(f'{where}: {source}: no such file')
            stem = Path(source).with_suffix('')
            if stems.setdefault(stem, source) != source:
                fail(f'{where}: {source} and {stems[stem]} would build one object')
        owned = sorted(self.placements, key=lambda row: row[2])
        for first, second in zip(owned, owned[1:]):
            if second[2] < first[3]:
                fail(f'{at[second]}: {second[0]} starts inside {first[0]}, at {at[first]}')
        # the row of a section the extents give starts at its region()'s start
        # and spans the region. Past the region, it may run on only to the next
        # section's start. Where the image has section rows, each such section
        # that holds bytes has its own row. A counterpart can lie in its
        # section's row instead, as D32 can in D's. A placement in such a
        # section lies inside the section's bounds().
        bounds = self.bounds()
        starts = {low for _, low, _ in self.sections}
        for name, low, high in self.sections:
            if name not in bounds:
                continue
            bottom, top = self.region(name)
            if low != bottom or high < top or (high > top and high not in starts):
                fail(f'{self.splits}: section {name} runs {hexa(low)} to {hexa(high)}, '
                     f'and tools/config.py gives {hexa(bottom)} to {hexa(top)}')
        spanned = {name for row, _, _ in self.sections for name in (row, f'{row}32')}
        for name, (low, high) in bounds.items():
            if self.sections and low < high and name not in spanned:
                fail(f'{self.splits}: section {name} has no row, and tools/config.py gives {hexa(low)} to {hexa(high)}')
        for placement in self.placements:
            source, name, start, end = placement
            if name not in bounds:
                continue
            low, high = bounds[name]
            if not (low <= start and end <= high):
                fail(f'{at[placement]}: {source} lies outside {name}, {hexa(low)} to {hexa(high)}')

    def extents(self):
        """The sections the image's section-boundary tables bound, as (name,
        start, end) in link order: EXTENTS's entry for the image, or () for an
        image EXTENTS doesn't name. D's or DSG's end is None where D32 or DSG32
        follows it."""
        return EXTENTS.get(self.path, ())

    def bounds(self):
        """The (start, end) of each section the extents give, by name. Each is
        the section's extent, but where the extents end D or DSG with None, it
        runs to the start of D32 or DSG32."""
        found = {name: (start, end) for name, start, end in self.extents()}
        return {name: (start, found[f'{name}32'][0] if end is None else end) for name, (start, end) in found.items()}

    def region(self, name):
        """The (start, end) that the row of section name must span, for a
        section the extents give. It runs from the section's start to the end
        of its counterpart, name with 32 after it, such as D32 for D. Where the
        extents give no counterpart, or the image has a row for it, it runs to
        the section's own end."""
        bounds, tabled = self.bounds(), {section for section, _, _ in self.sections}
        counterpart = f'{name}32'
        alone = counterpart not in bounds or counterpart in tabled
        return bounds[name][0], bounds[name if alone else counterpart][1]

    def layout(self):
        """The image's sections as (name, start, end), in address order.

        For an image with section rows, these are its rows. 1ST_READ.BIN has
        none, so its sections are those the extents give that hold bytes, P
        and PSG as FIRST_CODE_SECTIONS gives them, and C and CSG. C and CSG
        start where P and PSG end, and each runs to the start of the next
        section that holds bytes."""
        if self.path != FIRST:
            return sorted(self.sections, key=lambda row: row[1])
        found = [(name, low, high) for name, (low, high) in self.bounds().items() if low < high]
        for name, low, high, later in FIRST_CODE_SECTIONS:
            after = min(start for _, start, _ in found if start >= high)
            found += [(name, low, high), (later, high, after)]
        return sorted(found, key=lambda row: row[1])

    def span(self):
        """The image's (start, end), from its first section's start to its
        last's end, as layout() gives them."""
        layout = self.layout()
        return min(start for _, start, _ in layout), max(end for _, _, end in layout)

    @property
    def base(self):
        """The linked address of the image's first byte."""
        return self.span()[0]

    def code(self):
        """The image's game code and SDK code, as two lists of (start, end).

        1ST_READ.BIN's and LOADER.BIN's are fixed above. In another image, a
        section whose name starts with P is code: SDK code where the name
        starts with PSG, and game code otherwise. An event's runtime() is SDK
        code too, so the game code of the P that holds the runtime ends where
        the runtime starts."""
        if self.path == FIRST:
            return FIRST_GAME_CODE, FIRST_SDK_CODE
        if self.path == LOADER:
            return LOADER_GAME_CODE, LOADER_SDK_CODE
        code = [(name, start, end) for name, start, end in self.sections if name.startswith('P')]
        game = [(start, end) for name, start, end in code if not name.startswith('PSG')]
        sdk = [(start, end) for name, start, end in code if name.startswith('PSG')]
        runtime = self.runtime()
        if runtime:
            game = [(start, runtime[0]) if start <= runtime[0] < end else (start, end) for start, end in game]
            sdk.append(runtime)
        return game, sdk

    def linker_fill(self, section):
        """The gaps the tables show the linker leaves after objects and between
        sections, as (name, start, end) in address order. name is the section
        that holds the gap, or None for a gap between two section rows.

        section is link.section: section(source, name) gives the (size,
        alignment, code) that source's object declares for section name. Only
        the alignment is read.

        Inside a section, a gap runs from the end of a source's row to the next
        row. The gap is the linker's fill where the next row is another
        source's first in the section, and no source has rows in the section
        both before and after the gap. Such a source's object's section would
        run across it. The alignment of the later source's object must also
        round the end up to that row's start.

        A gen/ range's bytes give neither its object's end nor its alignment.
        Its boundary declarations stand in for them: end for the end of its
        object, and start N for the start of an object on N. No other gap runs
        from a gen/ range, or to one inside a section. TOOLCHAIN.md#fill says
        why.

        An end fits a section's start where alignments() gives the section at
        least one alignment, and each rounds the end up to that start. From a
        section's last placement, a gap runs to the next section's start where
        the placement's end fits that start. No such gap runs from a gen/
        range, even one that declares end. The gap's part inside the section's
        row comes back as the section's. Its part past the row comes back as a
        gap between rows, with name None. 1ST_READ.BIN has no section rows.
        FIRST_CODE_SECTIONS gives its code sections and the sections after
        them, and FIRST_TAILS gives its other sections whose last placement a
        gap runs from.

        The section rows and the fill between them tile the image, so a gap
        between two rows must be the linker's fill. Where a row ends short of
        the next, the row's end must fit the next section's start. Otherwise
        the extents must give the next section that start.

        A gap inside a section that these rules don't justify is left out. The
        splicer then fails its stored bytes as unwritten.

        A gap between rows that these rules don't justify fails, naming
        splits.txt. A boundary declaration and a FIRST_TAILS entry fail as
        gaps() gives.
        """
        return self.gaps(section)[0]

    def padding(self, section):
        """The padding before each gen/ range that declares continues N, inside
        the object the range continues, as (name, start, end) in address
        order. name is the section that holds the padding. section is as
        linker_fill() takes it.

        Padding runs from the end of a source's last row in a section to the
        next row, a gen/ range that declares continues N. The range continues
        that source's object, and starts at that end rounded up to N.

        It fails as gaps() gives."""
        return self.gaps(section)[1]

    def gaps(self, section):
        """linker_fill()'s gaps and padding()'s, as two lists. section is as
        linker_fill() takes it.

        No gap or padding inside a section has one source's rows in the section
        both before and after it. That source's object's section, from its
        first row to its last, would run across it.

        Between two adjacent rows of a section where either is a gen/ range,
        only boundary declarations bound a gap. Such a gap runs from a source's
        last row in the section, or from a gen/ range that declares end. It
        runs to the next row, which is one of:
        - a source's first row in the section, on its object's alignment;
        - a gen/ range that declares start N or continues N, on N.
        A range that declares continues N continues the object of the source
        before it, so no gap runs to one from a gen/ range.

        A gap between section rows fails as linker_fill() gives. In
        1ST_READ.BIN, a FIRST_TAILS entry fails, naming tools/config.py, where
        FIRST_EXTENTS doesn't give the section after it. A boundary
        declaration fails, naming its row, where:
        - the rows it bounds meet, with no gap between them;
        - the later row's start isn't the earlier row's end rounded up to the
          alignment above;
        - no gap uses it;
        - it is start N, alignments() gives the row's section at least one
          alignment, and N isn't one of them.
        """
        owned = sorted(self.placements, key=lambda row: row[2])
        # each source's first start and last end in each section
        first, final = {}, {}
        for source, name, start, end in owned:
            first.setdefault((source, name), start)
            final[(source, name)] = end
        # a range that declares start N opens an object on N. The call gives
        # alignments() the range's start in place of the section's. No source's
        # row starts there, since rows don't overlap, so alignments()'s first
        # case never applies.
        for source, label, start, _ in owned:
            where, declared = self.declared.get(source, (None, {}))
            aligns = self.alignments(label, start, section) if 'start' in declared else ()
            if aligns and declared['start'] not in aligns:
                starts = ' or '.join(map(str, sorted(aligns)))
                if any(row[1] == label and not GENERATED.fullmatch(row[0]) for row in owned):
                    fail(f"{where}: {source} declares start {declared['start']}, and the image's objects in {label} "
                         f'start on {starts}')
                fail(f"{where}: {source} declares start {declared['start']}, and with no source outside gen/ placed in "
                     f'{label}, an object there starts on {starts}')
        gaps, padding, used = [], [], set()
        # in each section, the furthest end of an object's section the walk has entered
        reach = {}
        for (source, name, _, end), (later, label, start, _) in zip(owned, owned[1:]):
            reach[name] = max(reach.get(name, 0), final[(source, name)])
            if label != name or (later, label) == (source, name) or first[(later, label)] != start or reach[name] > end:
                continue
            if not GENERATED.fullmatch(source) and not GENERATED.fullmatch(later):
                if end < start == aligned(end, section(later, label)[1]):
                    gaps.append((name, end, start))
                continue
            before = self.declared.get(source, (None, {}))[1]
            after = self.declared.get(later, (None, {}))[1]
            kind = next((kind for kind in ('start', 'continues') if kind in after), None)
            if GENERATED.fullmatch(later) and kind is None:
                continue
            if GENERATED.fullmatch(source) and ('end' not in before or kind == 'continues'):
                continue
            alignment = after[kind] if kind else section(later, label)[1]
            bounding = [(source, 'end')] * bool(GENERATED.fullmatch(source)) + [(later, kind)] * bool(kind)
            where = ' and '.join(self.declared[each][0] for each, _ in bounding)
            if end == start:
                fail(f'{where}: {source} ends where {later} starts, at {hexa(end)}, and no gap lies between them')
            if aligned(end, alignment) != start:
                fail(f'{where}: {source} ends at {hexa(end)}, which rounds up to {hexa(aligned(end, alignment))} '
                     f'on {alignment}, and {later} starts at {hexa(start)}')
            (padding if kind == 'continues' else gaps).append((name, end, start))
            used.update(bounding)
        for source, (where, declared) in self.declared.items():
            for declaration in declared.items():
                if (source, declaration[0]) not in used:
                    fail(f'{where}: {source} declares {spell(declaration)}, and no gap uses it')
        bounds = self.bounds()
        if self.path == FIRST:
            ends = [(name, top, later, top) for name, _, top, later in FIRST_CODE_SECTIONS]
            for name, later in FIRST_TAILS:
                if later not in bounds:
                    fail(f"tools/config.py: FIRST_TAILS runs the fill from {name}'s last placement to {later}'s start, "
                         f'and FIRST_EXTENTS gives {later} none')
                ends.append((name, bounds[later][0], later, bounds[later][0]))
        else:
            tiles = sorted(self.sections, key=lambda row: row[1])
            ends = [(name, top, later, start) for (name, _, top), (later, start, _) in zip(tiles, tiles[1:])]

        def fits(end, later, start):
            aligns = self.alignments(later, start, section)
            return bool(aligns) and all(aligned(end, align) == start for align in aligns)

        for name, top, later, start in ends:
            if top < start:
                if not fits(top, later, start) and bounds.get(later, (None, None))[0] != start:
                    fail(f'{self.splits}: no row holds {hexa(top)} to {hexa(start)}, between {name} and {later}, '
                         f'and no alignment {later} can start on puts its start there')
                gaps.append((None, top, start))
            last = max((row for row in owned if row[1] == name), key=lambda row: row[3], default=None)
            if last and not GENERATED.fullmatch(last[0]) and last[3] < top and fits(last[3], later, start):
                gaps.append((name, last[3], top))
        return sorted(gaps, key=lambda gap: gap[1]), sorted(padding, key=lambda gap: gap[1])

    def alignments(self, name, at, section):
        """The set of alignments section name can start on, where it starts at
        address at. section is as linker_fill() takes it.

        The linker starts a section on its first object's alignment, which no
        gen/ range's bytes show. The set holds the first of these that
        applies, gen/ ranges aside:
        - the alignment of a source's object, where that source's first row in
          the section starts at at;
        - else the alignment of the object of each source placed in the
          section, each standing in for the first object;
        - else, in 1ST_READ.BIN, that of the object FIRST_EMPTY names for the
          empty section;
        - else none, for a code section, one whose name starts with P;
        - else 4 and 8, on which R10 and R32 start constants, data or bss.

        Standing in assumes each placed object's alignment gives the section
        the start the first object's would, which can fail (TOOLCHAIN.md#fill).
        """
        own = {}
        for source, label, start, _ in self.placements:
            if label == name and not GENERATED.fullmatch(source):
                own[source] = min(start, own.get(source, start))
        sources = [source for source, start in own.items() if start == at] or list(own)
        if not sources and self.path == FIRST and name in FIRST_EMPTY:
            sources = [FIRST_EMPTY[name]]
        found = {section(source, name)[1] for source in sources}
        return found or (set() if name.startswith('P') else {4, 8})

    def runtime(self):
        """The runtime an event links after its own code, as (start, end): from
        the run's first name to the end of the P that holds it. None for an
        image whose symbols.txt gives neither EVENT_RUNTIME's first name nor
        EVENT_CONVERSIONS's.

        The run is EVENT_RUNTIME where symbols.txt gives its first name, and
        EVENT_CONVERSIONS otherwise. Every name of the run must be in
        symbols.txt, at strictly increasing addresses, inside that P. Other
        names may share their addresses or lie between them. A run that
        breaks this fails, naming symbols.txt."""
        own = dict((name, address) for address, name in named(self.path / 'symbols.txt'))
        group = next((group for group in (EVENT_RUNTIME, EVENT_CONVERSIONS) if group[0] in own), None)
        if group is None:
            return None
        start = own[group[0]]
        end = next((high for name, low, high in self.sections if name == 'P' and low <= start < high), None)
        members = [own.get(name) for name in group]
        if end is None or None in members or members != sorted(set(members)) or members[-1] >= end:
            fail(f'{self.path / "symbols.txt"}: no run of {", ".join(group)} in that order, '
                 f'from {group[0]} to the end of its P')
        return start, end

    def ranges(self, source):
        """Each (section, start, end) the image places source at, in table order."""
        return [(section, start, end) for name, section, start, end in self.placements if name == source]

    def sources(self):
        """Every source the image places, in table order."""
        return list(dict.fromkeys(source for source, *_ in self.placements))

    def held(self):
        """Each source's ranges that its linked source holds, by source, as
        lists of (section, start, end) in table order.

        That is every range but 1ST_READ.BIN's bss, which the file doesn't
        store. A source with only those maps to an empty list.
        """
        held = {}
        for source, name, start, end in self.placements:
            ranges = held.setdefault(source, [])
            if self.path != FIRST or name not in ZEROED:
                ranges.append((name, start, end))
        return held

    def symbols(self):
        """The (address, name) pairs the image's code or data can use. They
        start with the rows of the image's own symbols.txt. For an image other
        than 1ST_READ.BIN and those in ALONE, 1ST_READ.BIN's rows follow. A row
        of 1ST_READ.BIN's is left out where it lies inside the image's span(),
        or where the image's own rows give its name."""
        own = named(self.path / 'symbols.txt')
        if self.path == FIRST or self.path in ALONE:
            return own
        low, high = self.span()
        given = {name for _, name in own}
        return own + [(address, name) for address, name in named(FIRST / 'symbols.txt')
                      if not low <= address < high and name not in given]


def images():
    """The name of every image config/ describes, a directory there, in name
    order. A working directory without config/ fails."""
    if not CONFIG.is_dir():
        fail(f"{CONFIG}/: not found; run tools from the repository's root")
    return sorted(path.name for path in CONFIG.iterdir() if path.is_dir())


def files():
    """Each image's original file, under orig/, by the image's name, in name order."""
    return {name: FILES.get(name, f'SONICADV/{name.upper()}.PRS') for name in images()}


def configs():
    """Every image's Config, in name order."""
    return [Config(name) for name in images()]


def directories(config):
    """The directories whose declarations name an image's data, as Paths, its
    own first. For 1ST_READ.BIN, they are ROOTS but sdk/. For another image,
    they are its own directory, then each other modules/ or events/ directory
    it links a source from, sorted. Its own directory is loader/ for
    LOADER.BIN. For any other image, it is events/<image> where that
    directory exists, and modules/<image> otherwise."""
    if config.path == FIRST:
        return [Path(root) for root in ROOTS if root != 'sdk']
    name = config.name
    own = Path('loader') if name == 'loader' else Path('events' if Path('events', name).is_dir() else 'modules', name)
    shared = sorted({Path(*Path(source).parts[:2]) for source in config.sources()
                     if Path(source).parts[0] in ('modules', 'events')} - {own})
    return [own, *shared]
