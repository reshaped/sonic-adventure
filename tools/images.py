"""The images config/ describes, the directories of their sources, and what
no table gives of them.

images() names each image, a directory in config/. motions() gives the
motion files, the images that hold no code, and binaries() every other
image, a binary, which the matching build rebuilds
(TOOLCHAIN.md#motion-files). files() gives each image's original file. The
Makefile lists the disc's files through binaries(), motions(), files() and
EVENT_ALIASES.

The constants fix what no table gives:

- 1ST_READ.BIN's section boundaries;
- the game and SDK code of 1ST_READ.BIN and LOADER.BIN;
- the extents of the sections that section-boundary tables bound: strt1's
  in those two images, mpvasec's in MOVIE.BIN, and each image's own in
  B_ROBO.BIN, B_E101_R.BIN and ADVERTISE.BIN;
- each image's original file;
- the four events that repeat another;
- the C runtime an event links, counted as its SDK code;
- the images that take none of 1ST_READ.BIN's names or addresses;
- the bss sections.

Every range is [start, end) in linked addresses.
"""

from pathlib import Path

from tables import SECTION, rows
from tool import fail

# the directories that hold the game and SDK sources, as the Makefile's ROOTS
# names them
ROOTS = ('src', 'sdk', 'modules', 'events', 'loader')
CONFIG = Path('config')
FIRST = CONFIG / '1st_read'
# 1ST_READ.BIN's base, where it links and loads; the starts of P, C, PSG and
# CSG; its game code and SDK code; and the ends of its file and its bss. The
# program opens with the entry's 48 bytes of code and pool. The bytes after
# them, up to P's start, are data: the ID of its module's start record, the
# library handles and the end record.
FIRST_BASE = 0x8C010000
FIRST_P = 0x8C0100A0
FIRST_C = 0x8C1283CC
FIRST_PSG = 0x8C6143E0
FIRST_CSG = 0x8C6785EC
FIRST_GAME_CODE = [(0x8C0101A0, 0x8C11A580)]
FIRST_SDK_CODE = [(FIRST_BASE, 0x8C010030), (FIRST_P, 0x8C0101A0), (0x8C11A580, FIRST_C), (FIRST_PSG, FIRST_CSG)]
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


def images():
    """The name of every image config/ describes, a directory there, in name
    order. A working directory without config/ fails."""
    if not CONFIG.is_dir():
        fail(f"{CONFIG}/: not found; run tools from the repository's root")
    return sorted(path.name for path in CONFIG.iterdir() if path.is_dir())


def motions():
    """The name of every motion file config/ describes, in name order. A
    motion file is an image whose splits.txt gives it sections and no row in
    code (TOOLCHAIN.md#motion-files). As in Config.code(), a section is code
    where its name starts with P. A section row names its section first, and
    a placement second. An image with a row in code is a binary even where
    the row is out of form, so that its Config reads the row and fails."""
    found = []
    for name in images():
        splits = CONFIG / name / 'splits.txt'
        if not splits.is_file():
            continue
        rowed = [row for _, row in rows(splits)]
        sections = [row[0] for row in rowed if SECTION.fullmatch(row[0])]
        placed = [row[1] for row in rowed if not SECTION.fullmatch(row[0]) and len(row) > 1]
        if sections and not any(section.startswith('P') for section in sections + placed):
            found.append(name)
    return found


def binaries():
    """The name of every binary config/ describes, in name order: each image
    but those motions() gives."""
    return sorted(set(images()) - set(motions()))


def files():
    """Each image's original file, under orig/, by the image's name, in name order."""
    return {name: FILES.get(name, f'SONICADV/{name.upper()}.PRS') for name in images()}
