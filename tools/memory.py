"""The memory contract of make gcc's images: the RAM each owner holds, the
states in which owners are live together, and how each image loads and
starts.

RAM's 16 MiB appear at both 0x0c000000 and 0x8c000000
(TOOLCHAIN.md#images). The contract places each owner at its 0x0c address,
so two owners that reach the same RAM through the two views overlap. An
owner runs from its start up to its end, which no byte of it holds. An
image's origin and an anchor's address keep the view their tables give.

The owners are:

- Each image, from its origin to its end. Its origin is its tables' base,
  where the matching build links it, in the view its own code uses. Its end
  lies past its bss and past its tail, the sections its link places after
  the bss. ADVERTISE's tail is DEND, whose advertise_module_end holds the
  address of ADVERTISE's end. STG08's is its fish models. An image's body
  is the image less its tail.
- The areas the sources fix, which the contract reads from them by macro
  name: loader.h's work area, and its set and camera areas of 16 KB each;
  strt2.c's stack; the heaps sbinit.c and LOADER.BIN's sbinit.c give
  syMalloc, each the last megabyte of RAM; download.c's two model buffers,
  each as long as the gap between the two; stream.c's stream work, the
  music's and the voices'; modules/movie/movie.c's work, where Sofdec's
  player decodes; recorder.h's DEMO_RECORDING, of 128 KB; and the bytes
  play.c's event_load zeroes at EVENT_MODULE before each read.
- Each data file, a PRS file a stage reads that is no image, such as
  SAMP_LCD.PRS. It holds its room, the RAM it may take. The room runs from
  the area loader_read_prs decodes the file to. It ends at the nearest
  owner that starts past there in a state that holds the file. Where no
  such owner starts, the room runs to RAM's end. A motion file that is no
  image holds a room from MOTION_AREA in the same way.
- The cast's room, the RAM cast_load's textures may take: the largest
  one's whole sectors, just below WORK_AREA.
- Each anchor, a name the link binds to the address its image's tables
  give. The anchors are the field_messages of ADV00, ADV02 and ADV03,
  below the camera areas. These fields are Station Square, the Mystic
  Ruins and the past. Each field's crowd.c reads a message file to its
  anchor, and each such file holds pointers into itself at that address.
  The anchor holds the field's longest message file in whole sectors.
- The work past an image's end, which the image's code places by a word
  its bounds hold, rounded up to 32. ADVERTISE's is the backup library's
  work, BUM_WORK_SIZE of BUD_CAPACITY_1MB for all eight drives. Zero's and
  E-101 mkII's are their banks, each the longer of its fight's two bank
  files in whole sectors. The contract gives this work's size, and the
  image's link gives its place.

Each image's origin must be where its loader reads it. The boot ROM reads
1ST_READ.BIN, which no source the contract reads places. 1ST_READ.BIN's
code reads every other image:

- loader.c's loader_read_stage_files reads a stage's programs and its board
  motions to the areas loader.h and loader.c name. STG08's icecap_act_start
  reads the board motions to the same areas.
- loader_read_character_motions reads a character's motion file to
  MOTION_AREA.
- play.c's event_load reads an event to EVENT_MODULE.
- main.c reads the menus and SUMMARY to MODULE_AREA. It reads LOADER.BIN
  whole, in sectors, to loader_buffer.
- src/progress/movie.c's movie_play reads MOVIE.BIN in whole sectors to
  movie_entry, then runs it.

1ST_READ.BIN's tables give loader_buffer and movie_entry.

A state is a set of owners live together. It holds each of its images in
one of four parts, each from the image's origin:

- whole, to the image's end;
- its body, to its body's end;
- its file, to its sectors' end, the end of the file's last sector;
- its read, to the later of its end and its sectors' end. A loader that
  reads an image in whole sectors and then runs it leaves the image so.

A state holds one event at most, and one motion file at most. The states
follow the loader's transitions:

- A stage's, for each case of loader.c's loader_read_stage_files. It holds
  1ST_READ.BIN whole, with its stack, heap and work area, download.c's
  model buffers and the stream work. It holds whole each image the case
  reads with loader_read_prs, with the image's anchors and the work past
  it. It holds the areas the case's other reads fill, its data files'
  rooms among them. Where the case reads STG09, it holds SET_AREA_4, where
  casino_act_start reads the palette PL_9MB.BIN. It holds any one
  character's motion file, which set_character has
  loader_read_character_motions read as it sets the character. It holds
  any one event too, which event_load reads to EVENT_MODULE, unless the
  case reads a PRS file to EXTRA_AREA, which lies at EVENT_MODULE's
  address.
- Icecap's with the board motions: stg08's state again, with the board
  motions icecap_act_start reads for Sonic or Tails. They load over the fish
  models, stg08's tail, so the state holds stg08's body.
- A recorded demo's: each state of a stage in which demo.c's demos[] plays
  recorded pads, again, with the areas demo_advance reads the recordings
  to. That is DEMO_RECORDING, which the pads play from. The state holds no
  event, as demos[] gives such a demo none. level_item.c loads an
  upgrade's event, 357 to 369, as the character takes the upgrade. The
  state holds none of them: demo_set_flags sets three upgrades' flags,
  which end their stands, and set files the contract doesn't read place
  the rest.
- A recorded demo's as an event loads: each recorded demo's state again,
  with one event stage.c's entry_events or clear_events gives the demo's
  stage, act and character. stage_start_event and stage_clear_event can
  load one that hasn't played, in any game mode but a trial. The state still
  holds the recording, as no source the contract reads ends the pads first.
  Big's demo of Beach act 2 can load event 212, if the act clears.
- The menus', with ADVERTISE and the backup work past it, and the
  summary's, with SUMMARY. main.c reads each to MODULE_AREA. Each holds
  1ST_READ.BIN with its stack, heap and work area. Each holds the stream
  work too, as each plays its music through stream.c.
- A demo's start: the menus' state again, where main.c's MENU_DEMO has
  demo_advance start a demo. It adds the areas demo_advance reads the
  recordings to, and any one motion file, which set_character has
  loader_read_character_motions read.
- The movie's: 1ST_READ.BIN with its stack and heap, MOVIE.BIN's read,
  modules/movie/movie.c's work and the cast's room. That work is the 3 MB
  from MODULE_AREA where Sofdec's player decodes. Besides the stream the
  player reads through gsc_gd.c, the movie reads only cast_load's textures.
  texture_load_prs_file reads each in whole sectors to its room just below
  WORK_AREA. On GCC's side of texture.c, that room is the file's whole
  sectors, so each read lies in the cast's room. It decodes the texture to
  MODULE_AREA, in modules/movie/movie.c's work.
- The hand-off's: 1ST_READ.BIN with its stack and heap, which stay live
  through main's read of LOADER.BIN, and LOADER.BIN's file.
- The loader's: LOADER.BIN alone once it runs, with the stack and its own
  heap.

An image's interval is the RAM it may take. It runs from the image's origin
up to the nearest other owner that starts past there. That owner must lie
in a state that holds the image in any part but its body. Where none starts
past there, the interval runs to RAM's end. The work past an image ends no
interval or room, as the contract doesn't place it.

The images a state holds may take names from each other, except in the
two states whose images are apart. At a demo's start, ADVERTISE has
stopped, and demo_advance reads for the stage that replaces it. In the
hand-off, LOADER.BIN is read and not yet started, and main takes from it
only the two names config/1st_read/imports.txt's hand-off rows give. The
contract pairs images, not names, so a pair doesn't make every name of the
two live: Icecap's state with the board motions holds stg08's body and not
its tail.

Each image starts as its loader leaves it:

- 1ST_READ.BIN and LOADER.BIN start through strt1.src's tables. Their files
  hold no bss. _INITSCT zeroes B through B32 and BSG through BSG32, which
  must hold every byte of the image's bss. It copies nothing, as R through
  R32 and RSG through RSG32 are empty.
- An event starts as event_load leaves it. event_load zeroes the
  EVENT_MODULE_SIZE bytes from EVENT_MODULE before its read, and no more,
  and each state that holds an event holds those bytes. So an event's file
  holds no bss, unless the bss runs past those bytes. Then its file holds
  its bss as zeros.
- Any other image starts as its file and the RAM under it leave it. Its
  file holds its bss as zeros where its original file holds any of it. An
  original file holds bss where its tables place a section past the bss.
  Otherwise the image's file ends with the last byte before its bss.

GCC's files hold as zeros some bss the shipped game leaves as RAM held it:
MOVIE.BIN's BSG, and in 17 events, the bss past the bytes event_load zeroes.

A record is data an image's loader reads at a fixed offset. records() gives
each image's, and the link holds each at its offset:

- in 1ST_READ.BIN and LOADER.BIN, those of the module strt1.src starts:
  START_SYS, the entry, which opens it; the library handles _INITLIBH
  enters; and DSGLE, its end record;
- MOVIE.BIN's movie_entry, which movie_play calls;
- the menus' and SUMMARY's entry tables, which main reads at MODULE_AREA;
- each event's head, its entry and texture list, which event_load reads at
  EVENT_MODULE.

The contract holds no set or camera file's size. loader_read writes each to
its area in whole sectors, so a file of more than 16 KB would write over the
next area: a read to area 0, 1 or 2 runs into area 3, 4 or 5. The contract
holds no size of a texture texture_load_pvm reads either. On GCC's side of
texture.c, texture_load_pvm reads each file into its whole sectors just
below WORK_AREA_END, so the read ends where the heap starts.

contract() gives the contract, from config/'s tables and the sources
SOURCES names, and its docstring lists how it fails. check() holds a link's
extents to the contract, and its docstring lists the problems it reports.
"""

import ast
import re
from collections import namedtuple
from pathlib import Path

import csource
from config import Config
from images import ZEROED, files
from tables import aligned, hexa, named
from tool import fail

# RAM's start and end in the 0x0c view
RAM = 0x0C000000
RAM_END = 0x0D000000
# the address bit the 0x8c view sets
VIEW_BIT = 0x80000000

# the sources the contract reads macros from
LOADER_H = Path('src/system/loader.h')
LOADER_C = Path('src/system/loader.c')
PLAY_C = Path('src/event/play.c')
STRT2_C = Path('sdk/shinobi/strt2.c')
SBINIT_C = Path('src/system/sbinit.c')
LOADER_SBINIT_C = Path('loader/sbinit.c')
SHINOBI_H = Path('sdk/include/shinobi.h')
DOWNLOAD_C = Path('src/object/download.c')
STREAM_C = Path('src/sound/stream.c')
MOVIE_C = Path('modules/movie/movie.c')
RECORDER_H = Path('src/progress/recorder.h')
# loader.h's areas, each a place a read can name
LOADER_AREAS = ('WORK_AREA', 'MODULE_AREA', 'MOTION_AREA', 'SONIC_BOARD_AREA', 'TAILS_BOARD_AREA',
                *(f'{kind}_AREA_{n}' for kind in ('SET', 'CAMERA') for n in range(6)))
# the macros the contract reads, by the source that defines them
MACROS = {LOADER_H: ('SECTOR_SIZE', 'WORK_AREA_END', *LOADER_AREAS),
          LOADER_C: ('FIELD_AREA', 'TIKAL_AREA', 'EXTRA_AREA'), PLAY_C: ('EVENT_MODULE', 'EVENT_MODULE_SIZE'),
          STRT2_C: ('STACK_BOTTOM', 'STACK_TOP'), SBINIT_C: ('HEAP_SIZE',), LOADER_SBINIT_C: ('HEAP', 'HEAP_SIZE'),
          SHINOBI_H: ('BUD_CAPACITY_1MB', 'BUD_WORK_GAP'), DOWNLOAD_C: ('MODEL_A_BUFFER', 'MODEL_B_BUFFER'),
          STREAM_C: ('STREAM_WORK', 'MUSIC_WORK_SIZE', 'VOICE_WORK_SIZE'), MOVIE_C: ('WORK', 'WORK_SIZE'),
          RECORDER_H: ('DEMO_RECORDING',)}
# cast.c, whose cast_load reads the movie's textures
CAST_C = Path('modules/movie/cast.c')
# texture.c, whose GCC side defines FILE_ROOM
TEXTURE_C = Path('src/system/texture.c')
# demo.c, whose demos[] gives the demos that play recorded pads, and whose
# demo_advance reads their recordings
DEMO_C = Path('src/progress/demo.c')
# stage.c, whose entry_events and clear_events give the events a stage's
# acts load
STAGE_C = Path('src/stage/stage.c')
# Icecap's image
FISH_IMAGE = 'stg08'
# the source of its fish models, its tail
FISH_MODELS = 'modules/stg01/fish_models.c'
# the source that reads the board motions over them
ICECAP_C = Path('modules/stg08/icecap.c')
# Casinopolis' image, whose casino.c reads a palette, and casino.h, which
# names the palette's area
CASINO = 'stg09'
CASINO_C = Path('modules/stg09/casino.c')
CASINO_H = Path('modules/stg09/casino.h')
# each source the contract reads, which make gcc's link depends on. The
# contract reads the symbols.txt of each field MESSAGES names too.
SOURCES = (*MACROS, CAST_C, DEMO_C, STAGE_C, TEXTURE_C, ICECAP_C, CASINO_C, CASINO_H)

# the two forms of read cases() takes in loader.c, each matched from the
# function's name to its closing parenthesis. PRS_READ matches a read of a
# PRS file, named by a literal, to a macro. FILE_READ matches a read by
# loader_read, loader_read_set or loader_read_camera of a file, named by a
# literal or a variable, to a macro or a variable.
PRS_READ = re.compile(r'loader_read_prs\("([^"]+)",\s*([A-Z]\w*)\)')
FILE_READ = re.compile(r'loader_read(?:_set|_camera)?\((?:"[^"]*"|[a-z]\w*),\s*(\w+)\)')
# a call of any function whose name starts with loader_read or gdFsRead.
# cases() takes each as a read, and fails on one in neither form above.
ANY_READ = re.compile(r'\b(?:loader_read|gdFsRead)\w*\s*\(')
# a C string or character literal
LITERAL = re.compile(r'"(?:[^"\\\n]|\\.)*"|\'(?:[^\'\\\n]|\\.)*\'', re.S)
# a goto, which cases() follows to its label
GOTO = re.compile(r'\bgoto\s+(\w+)\s*;')
# a write of a lowercase variable: an assignment, compound or not, or an
# increment or decrement. An & before the name counts as one, as it can pass
# the variable's address to a writer.
WRITE = re.compile(r'(?:\+\+|--|&)\s*([a-z]\w*)\b|\b([a-z]\w*)\s*(?:\+\+|--|(?:[-+*/%&|^]|<<|>>)?=(?!=))')

# the bytes each set and camera area holds, and the bytes DEMO_RECORDING
# holds. loader.h's and recorder.h's comments give them, and no macro does.
AREA_SIZE = 0x4000
RECORDING_SIZE = 0x20000
# the alignment an image's code rounds its work pointer up to
WORK_ALIGNMENT = 32
# the drives buInit takes work for under BUD_USE_DRIVE_ALL, and the bytes
# it caches of each memory card block, as shinobi.h's BUM_WORK_SIZE counts
# them
DRIVES = 8
CARD_BLOCK_SIZE = 512

# the sizes in bytes the contract gives files by hand, as SHA256SUMS holds
# no line for them. tools/disc.py holds each size to the disc it maps.
# - MESSAGES gives the longest message file each field reads to
#   field_messages, by image.
# - CAST gives the textures cast_load reads.
# - BANKS gives each fight's two bank files, by its key in WORK. The fight's
#   banks take the longer file's whole sectors.
MESSAGES = {'adv00': ('SS_MES_S.BIN', 29580), 'adv02': ('MR_MES_S.BIN', 9652), 'adv03': ('PAST_MES_K.BIN', 2680)}
CAST = {'KNUCKLES.PRS': 27423, 'EV_TR2_BIG.PRS': 248920, 'AMY.PRS': 67183, 'MILES.PRS': 37906}
BANKS = {'EROBO': (('EROBO.PB', 301560), ('EROBO.NB', 295156)), 'E101R': (('E101R.PB', 308288), ('E101R.NB', 180920))}

# the #define of FILE_ROOM that GCC's side of texture.c's #ifdef GCC must
# hold. It gives a file its whole sectors as its room.
# texture_load_prs_file reads a texture into its room just below WORK_AREA,
# so the largest cast texture's room is the cast's. texture_load_pvm reads
# a file into its room just below WORK_AREA_END, where the heap starts.
ROOM_DEFINITION = '#define FILE_ROOM(size) (SECTORS(size) * SECTOR_SIZE)'
# the work past an image's end, by image. Each gives the word the image's
# bounds hold, whose value rounded up to WORK_ALIGNMENT is the work's
# start. Each gives the work's kind too: 'backup' for the backup library's,
# or else the fight's key in BANKS.
WORK = {'advertise': ('advertise_module_end', 'backup'), 'b_robo': ('eggmanrobo_bss_end', 'EROBO'),
        'b_e101_r': ('e101_r_bss_end', 'E101R')}

# the images that start through strt1.src's tables
STARTUP = ('1st_read', 'loader')
# the record of START_SYS, the entry that opens the module strt1.src starts
# in 1ST_READ.BIN and LOADER.BIN
START_SYS_RECORD = ('DSGLH', 'sdk/shinobi/strt1.src', 'DSGLH', 'START_SYS', 0, None)
# the records each image's loader reads at fixed offsets, by image, in
# order. Each is a tuple of:
# - the record's section in the tables, its source and its GCC section;
# - its symbol, and its offset past the image's origin;
# - its size in bytes, or None where the link checks no size.
RECORDS = {
    '1st_read': (START_SYS_RECORD, ('DSGLH', 'sdk/shinobi/gdfs.c', '.data.gdg_fs_libhdl', 'gdg_fs_libhdl', 0x40, 32),
                 ('DSGLH', 'sdk/shinobi/ov_main.c', '.data', 'ovg_libhdl', 0x60, 32),
                 ('DSGLE', 'sdk/shinobi/strt1.src', 'DSGLE', '_startof_DSGLE', 0x80, None)),
    'loader': (START_SYS_RECORD, ('DSGLH', 'sdk/shinobi/ov_main.c', '.data', 'ovg_libhdl', 0x40, 32),
               ('DSGLH', 'loader/sdk/gdfs.c', '.data.gdg_fs_libhdl', 'gdg_fs_libhdl', 0x60, 32),
               ('DSGLE', 'sdk/shinobi/strt1.src', 'DSGLE', '_startof_DSGLE', 0x80, None)),
    'movie': (('P', 'modules/movie/entry.src', 'P', 'movie_entry', 0, None),),
    'advertise': (('DHEAD', 'modules/advertise/advertise.c', '.data', 'advertise_entries', 0, 12),),
    'summary': (('DHEAD', 'modules/summary/summary.c', '.data.summary_entries', 'summary_entries', 0, 36),),
}
# an event's one record, its head, with the event's name for {image}
EVENT_HEAD = ('DHEAD', 'events/{image}/head.c', '.data', '{image}_head', 0, 8)
# each image's tail, by image: the sections the link places past its bss.
# Each is a tuple of its section in the tables, its source and its GCC
# section. STG08's tail, its fish models, is the sections of FISH_MODELS'
# object instead.
TAILS = {'advertise': (('DEND', 'modules/advertise/bounds.src', 'DEND'),)}
# the symbol each image's ELF names as its entry, where it names one
ENTRIES = {'1st_read': 'START_SYS', 'loader': 'START_SYS', 'movie': 'movie_entry'}

Owner = namedtuple('Owner', 'name start end')
# a state, as the module docstring gives each:
# - name, by which problems name the state;
# - images, the part of each image the state holds, by image: 'whole',
#   'body', 'file' or 'read';
# - areas, the names of the owners it holds that are no image, a list;
# - event, whether it holds any one event too;
# - motions, whether it holds any one motion file too, False by default;
# - apart, whether its images take no names from each other, False by
#   default.
State = namedtuple('State', 'name images areas event motions apart', defaults=(False, False))
# the memory contract:
# - origins, each image's tables' base, by image;
# - lengths, the bytes of each image's interval, by image;
# - areas, the Owner of each area, room and anchor, by name;
# - anchored, each anchor's address in its image's tables, by image and
#   then by name;
# - states, each a State;
# - events, the events, sorted;
# - motions, the motion files a state may hold one of, sorted. Each is an
#   image's name, or for a file that is no image, its room's name, such as
#   MOTION_AREA file;
# - colive, the images each image may take names from, by image: those a
#   state holds with it, except in a state whose images are apart;
# - work, the bytes of the work past each image WORK names, by image;
# - sector, loader.h's SECTOR_SIZE, the bytes of a sector a loader reads.
Contract = namedtuple('Contract', 'origins lengths areas anchored states events motions colive work sector')
# what check() reads of an image's final link, each address in either view:
# - end, the image's end;
# - body, its body's end: where its tail starts, or its end where it has
#   none;
# - stored, its file's end;
# - sectors, its sectors' end: its file's end in whole sectors;
# - packed, the bytes its PRS file takes in whole sectors, or None for an
#   image with no PRS file;
# - work, the work past it as (start, end), or None where the contract holds
#   none.
Extent = namedtuple('Extent', 'end body stored sectors packed work')


def physical(address):
    """address, in either view, as the 0x0c view spells it."""
    return address & ~VIEW_BIT


def inside(start, end, low=RAM):
    """Whether start and end run in that order from low to RAM's end.
    start and end may be in either view, and low is a 0x0c address. An
    address in neither view lies outside RAM."""
    return low <= physical(start) <= physical(end) <= RAM_END


def code_only(text, path):
    """text, the C at path as csource.code() gives it, with each string and
    character literal blank between its quotes but for its line ends. A scan
    of the result meets code alone. A quote that opens no literal fails,
    naming path's line."""
    def blank(match):
        if len(match[0]) == 1:
            fail(f"{path}:{text.count(chr(10), 0, match.start()) + 1}: a quote that opens no literal, which the "
                 "memory contract doesn't read")
        return match[0][0] + re.sub(r'[^\n]', ' ', match[0][1:-1]) + match[0][-1]

    return re.sub(rf'{LITERAL.pattern}|["\']', blank, text, flags=re.S)


def conditions(text, path):
    """The conditionals each line of text lies in, in line order, where text
    is the C at path as csource.code() or code_only() gives it. Each line's
    entry is a tuple of the directives that open its conditionals,
    outermost first. A line past a conditional's #else, or past an #elif of
    any form, takes ' #else' after that conditional's directive. So a line
    past #ifdef GCC's #else lies in ('#ifdef GCC #else',).

    The file's include guard is its first conditional where that is an
    #ifndef whose next line defines the name it tests. A line inside the
    guard doesn't lie in it, but a line past the guard's #else does.

    A conditional left open fails, naming path. An #elif, #else or #endif
    outside every conditional fails, naming path's line."""
    lines, found, held, first = text.split('\n'), [], [], True
    for number, line in enumerate(lines):
        found.append(tuple(opened + side for opened, guard, side in held if side or not guard))
        directive = re.match(r'\s*#\s*(if|ifdef|ifndef|elif|elifdef|elifndef|else|endif)\b\s*(.*?)\s*$', line)
        if directive is None:
            continue
        word, rest = directive.groups()
        if word.startswith('if'):
            after = lines[number + 1] if number + 1 < len(lines) else ''
            guard = first and word == 'ifndef' and re.fullmatch(rf'\s*#\s*define\s+{re.escape(rest)}\s*', after)
            first = False
            held.append((f'#{word} {rest}', bool(guard), ''))
        elif not held:
            fail(f'{path}:{number + 1}: #{word} closes no conditional')
        elif word == 'endif':
            held.pop()
        else:
            held[-1] = (*held[-1][:2], ' #else')
    if held:
        fail(f'{path}: leaves {held[-1][0]} open')
    return found


def calls(path, function, pattern):
    """The groups pattern captures in each call of function in the C at
    path, in order. pattern is a regex of the call's arguments. A call is
    function's whole name and an opening parenthesis, outside comments and
    literals.

    A call whose arguments pattern doesn't match fails, naming path's line.
    So does a call inside any conditional but the file's include guard, as
    the contract doesn't tell a conditional's sides apart. calls() fails
    too where code_only() or conditions() fails on path."""
    text, found = csource.code(path), []
    held = conditions(text, path)
    for call in re.finditer(rf'\b{function}\s*\(', code_only(text, path)):
        match = re.compile(rf'{function}\({pattern}\)').match(text, call.start())
        line = text.count('\n', 0, call.start())
        if match is None or held[line]:
            fail(f'{path}:{line + 1}: {text[call.start():text.find(";", call.start())].strip()}, a call of {function} '
                 + (f"under {held[line][-1]}, whose sides the memory contract doesn't tell apart" if match else
                    "in a form the memory contract doesn't read"))
        found.append(match.groups())
    return found


def definitions(path):
    """The expression of each #define in the C file at path that the
    contract can read, by the macro's name, with comments left out. Such a
    #define:

    - starts its line, outside any conditional but the file's include
      guard;
    - defines a macro without parameters, whose name is capitals, digits
      and underscores;
    - is the file's only #define or #undef of that name.

    definitions() fails where conditions() fails on path."""
    text = csource.code(path)
    held, found, names = conditions(text, path), {}, []
    for number, line in enumerate(text.split('\n')):
        names += re.findall(r'^\s*#\s*(?:define|undef)\s+(\w+)', line)
        match = re.match(r'#define\s+([A-Z_][A-Z0-9_]*)\s+(.+?)\s*$', line)
        if match and not held[number]:
            found[match[1]] = match[2]
    return {name: expression for name, expression in found.items() if names.count(name) == 1}


def macros(path):
    """The value of each macro of the C file at path that has one, by name,
    among the #defines definitions() gives. A macro has a value where its
    expression is a number, a cast of one, or arithmetic on them and the
    file's other macros.

    Each macro an expression names stands for that macro's expression, as C
    expands it. A macro that names itself, at once or through others, has
    no value. The arithmetic is +, -, *, <<, >>, |, & and ~ on integer
    constants, as Python reads and evaluates them, such as 0x0c900000. A
    cast counts in three forms:

    - to Uint32;
    - to a pointer to Uint8, Sint8 or char, whose arithmetic counts bytes;
    - to any other pointer, in an expression with no arithmetic.

    A macro has no value where its expression holds anything else, such as
    a /, a U suffix or another cast. It has none either where its
    arithmetic fails, as a shift by a negative count does. A macro with a
    cast has no value outside 0 to 0xffffffff."""
    texts, found = definitions(path), {}

    def expanded(text, seen):
        return re.sub(r'\b[A-Z_][A-Z0-9_]*\b', lambda word: word[0] if word[0] not in texts or word[0] in seen
                      else expanded(texts[word[0]], seen | {word[0]}), text)

    # each pattern matches a cast: a type in parentheses, before the name,
    # number or ( it converts. Arithmetic on a number cast to Uint32 or to a
    # pointer to bytes counts bytes. Arithmetic on any other pointer scales
    # by a size the contract doesn't read.
    unscaled = re.compile(r'\(\s*(?:(?:const\s+)?(?:Uint8|Sint8|char)\s*\*|Uint32)\s*\)(?=\s*[\w(])')
    scaled = re.compile(r'\(\s*(?:const\s+)?(?:struct\s+)?[A-Za-z_]\w*\s*\*+\s*\)(?=\s*[\w(])')
    allowed = (ast.Expression, ast.BinOp, ast.UnaryOp, ast.Constant, ast.Add, ast.Sub, ast.Mult, ast.LShift,
               ast.RShift, ast.BitOr, ast.BitAnd, ast.Invert, ast.USub)
    for name, text in texts.items():
        text, casts = unscaled.subn('', expanded(text, {name}))
        text, scaled_casts = scaled.subn('', text)
        try:
            tree = ast.parse(text, mode='eval')
        except SyntaxError:
            continue
        if not all(isinstance(node, allowed) and (not isinstance(node, ast.Constant) or type(node.value) is int)
                   for node in ast.walk(tree)) or scaled_casts and not isinstance(tree.body, ast.Constant):
            continue
        try:
            value = eval(compile(tree, str(path), 'eval'))
        except (ArithmeticError, ValueError):
            continue
        if not (casts or scaled_casts) or 0 <= value <= 0xFFFFFFFF:
            found[name] = value
    return found


def numbers():
    """The values macros() gives the macros of each file MACROS names, by
    path: every macro of the file with a value, not only those MACROS
    lists. The first file without a value for a macro MACROS lists for it
    fails, naming the file and each such macro."""
    found = {}
    for path, names in MACROS.items():
        given = macros(path)
        problems = [f'{path}: gives no number for {name}, which the memory contract reads' for name in names
                    if name not in given]
        if problems:
            fail(*problems)
        found[path] = given
    return found


def cases(function):
    """The cases of the switch in loader.c's function, in loader.c's order.
    Each is (labels, reads, areas). function is the definition's first line,
    and the next line must be { alone. The body runs from there to the
    first line that starts with }. Its comments and literals don't count.

    - labels are the case's own labels, with those of the cases before it
      that hold no statement and fall into it.
    - reads are the PRS files the case reads, each as (file, destination),
      with the destination as the call spells it.
    - areas are the other areas the case fills, sorted. Each is the macro
      one of its other reads takes, or a macro it sets a variable such a
      read takes to.

    A case opens at case NAME: or default:. A case with a statement must
    end with a return, a break or a goto. That statement must lie at the
    level of the case's braces, where no if, else or loop holds it. Only the
    switch's last case may end otherwise. A read under an if, or inside any
    other statement, is its case's: the case's state holds each file the
    case can read. A case that reads nothing, such as the default's spin, is
    left out.

    A case must set each variable a read takes to an area by assigning it a
    macro inside the switch's cases, as dest = CAMERA_AREA_2; does.
    An & before a variable's name counts as a write of the variable, as it
    can pass the variable's address to a writer. Past a label, each read
    must be one FILE_READ matches, to a variable, and nothing may write a
    variable. So the reads past a label that a case's gotos reach fill the
    areas that case sets. A goto reaches a label directly, or through the
    gotos past another label it reaches.

    cases() fails where code_only() or conditions() fails on loader.c, and
    where loader.c holds no such function, naming loader.c. It fails too,
    naming loader.c's line, where the function:
    - lies in a conditional, or holds a directive, such as a guard's #ifdef
      GCC, whose sides the contract doesn't tell apart;
    - holds other than one switch, or a switch whose body isn't in braces;
    - reads outside the switch's cases, or in a form neither PRS_READ nor
      FILE_READ matches;
    - labels a case in another form, or jumps to a label no case holds;
    - holds a case with a statement that can fall into the next;
    - holds a case that reads a file and no PRS file;
    - reads a PRS file, or reads to a macro, past a label;
    - writes a variable past a label;
    - reads to a variable the case doesn't set to an area, in the case or
      past a label its gotos reach;
    - writes a variable a read takes in another form, or outside the
      switch's cases."""
    text = csource.code(LOADER_C)
    code = code_only(text, LOADER_C)
    name = function.split('(')[0].split()[-1]
    body = re.search(rf'^{re.escape(function)}\n\{{\n(.*?)^\}}', code, re.M | re.S)
    if body is None:
        fail(f'{LOADER_C}: holds no {name} whose body closes at the start of a line, for the memory contract to read')

    def where(at):
        return f'{LOADER_C}:{text.count(chr(10), 0, at) + 1}: {name}'

    held = conditions(text, LOADER_C)[text.count('\n', 0, body.start())]
    if held:
        fail(f"{where(body.start())} lies under {held[-1]}, whose sides the memory contract doesn't tell apart")
    start, end = body.span(1)
    directive = re.compile(r'^[ \t]*#.*', re.M).search(code, start, end)
    if directive is not None:
        fail(f"{where(directive.start())} holds {directive.group().strip()}, whose sides the memory contract doesn't "
             'tell apart')
    switch = re.compile(r'\bswitch\s*\([^;{]*\)\s*\{').search(code, start, end)
    if switch is None or len(re.compile(r'\bswitch\b').findall(code, start, end)) != 1:
        fail(f'{where(start)} holds other than one switch with its body in braces, which the memory contract reads')
    depth, close = 1, end
    for brace in re.compile(r'[{}]').finditer(code, switch.end(), end):
        depth += 1 if brace.group() == '{' else -1
        if not depth:
            close = brace.start()
            break
    # each read, as its match of PRS_READ or FILE_READ
    matches = []
    for read in ANY_READ.finditer(code, start, end):
        if not switch.end() <= read.start() < close:
            fail(f"{where(read.start())} reads outside its switch's cases, which the memory contract doesn't read")
        matches.append((PRS_READ if code.startswith('loader_read_prs(', read.start()) else FILE_READ).match(
            text, read.start()))
        if matches[-1] is None:
            fail(f"{where(read.start())} reads in a form the memory contract doesn't read: "
                 + text[read.start():text.find(';', read.start())].strip())

    variables = {read[1] for read in matches if read.re is FILE_READ}
    for write in WRITE.finditer(code, start, end):
        if (write[1] or write[2]) in variables and not switch.end() <= write.start() < close:
            fail(f"{where(write.start())} writes {write[1] or write[2]} outside its switch's cases, which the memory "
                 "contract doesn't read")

    def taking(pattern, low, high):
        return [read for read in matches if read.re is pattern and low <= read.start() < high]

    case_labels = list(re.compile(r'^[ \t]*(?:case[ \t]+(\w+)|(default))[ \t]*:', re.M).finditer(
        code, switch.end(), close))
    for word in re.compile(r'\b(?:case|default)\b').finditer(code, switch.end(), close):
        if not any(label.start() <= word.start() < label.end() for label in case_labels):
            fail(f"{where(word.start())} labels a case in a form the memory contract doesn't read")
    # spans gives each case's statements, from the end of its label to the
    # next case's label, as (start, end). targets gives each label inside a
    # case by name. Each is (the label's end, its case's end), which bound
    # what a goto to it runs.
    spans = [(label.end(), later.start()) for label, later in zip(case_labels, case_labels[1:])]
    spans += [(case_labels[-1].end(), close)] if case_labels else []
    targets = {target[1]: (target.end(), high) for low, high in spans
               for target in re.compile(r'^[ \t]*([a-z_]\w*)[ \t]*:(?!:)', re.M).finditer(code, low, high)}
    found, labels = [], []
    for label, (low, high) in zip(case_labels, spans):
        labels.append(label[1] or 'default')
        part = code[low:high]
        first_label = min([at for at, _ in targets.values() if low <= at < high] + [high])
        writes = list(WRITE.finditer(code, low, high))
        for write in writes:
            if write.start() >= first_label:
                fail(f'{where(write.start())} writes {write[1] or write[2]} past a label, which a goto can reach from '
                     "a case whose areas the memory contract doesn't give the write")
        for read in taking(PRS_READ, first_label, high):
            fail(f'{where(read.start())} reads {read[1]} past a label, which a goto can reach from a case whose state '
                 "the memory contract doesn't give the read")
        for read in taking(FILE_READ, first_label, high):
            if read[1][0].isupper():
                fail(f'{where(read.start())} reads to {read[1]} past a label, which a goto can reach from a case whose '
                     "state the memory contract doesn't give the read")
        # the labels the case's gotos reach, directly or through the gotos
        # past another label they reach
        jumps, reached = list(GOTO.finditer(code, low, high)), []
        while jumps:
            jump = jumps.pop()
            if jump[1] not in targets:
                fail(f'{where(jump.start())} jumps to {jump[1]}, a label no case holds')
            if jump[1] not in reached:
                reached.append(jump[1])
                jumps += GOTO.finditer(code, *targets[jump[1]])
        taken = taking(FILE_READ, low, high) + [read for target in reached for read in taking(FILE_READ,
                                                                                            *targets[target])]
        sets = {}
        for variable, area in re.findall(r'\b([a-z]\w*)\s*=\s*([A-Z]\w*)\s*;', part):
            sets.setdefault(variable, set()).add(area)
        for read in taken:
            if read[1][0].islower() and read[1] not in sets:
                fail(f"{where(read.start())} reads to {read[1]}, which case {labels[-1]} doesn't set to an area")
        for write in writes:
            variable = write[1] or write[2]
            if variable in {read[1] for read in taken} and not re.compile(rf'{variable}\s*=\s*[A-Z]\w*\s*;').match(
                    code, write.start()):
                fail(f"{where(write.start())} writes {variable} in a form the memory contract doesn't read")
        reads = [read.groups() for read in taking(PRS_READ, low, high)]
        ended = re.search(r'(?:^|[;}])\s*(?:[a-z_]\w*\s*:\s*)*(?:goto\s+\w+|return|break)\s*;\s*$', part)
        if not (ended and part.count('{') == part.count('}')) and part.strip() and high != close:
            fail(f"{where(label.start())}'s case {labels[-1]} can fall into the next with a statement, which the "
                 "memory contract doesn't read")
        if taken and not reads:
            fail(f"{where(label.start())}'s case {labels[-1]} reads a file and no PRS file, which the memory contract "
                 "doesn't read")
        if reads:
            found.append((labels, reads, sorted({area for read in taken for area in sets.get(read[1], {read[1]})})))
        if part.strip():
            labels = []
    return found


def recorded_demos():
    """The demos in which demo.c's demos[] plays recorded pads, in its
    order. Each is (STAGE_X, act, CHARACTER_Y). A row of demos[] is
    {STAGE_X, ACT, CHARACTER_Y, EVENT}, where ACT is a decimal. EVENT is
    DEMO_NO_EVENT for a demo of recorded pads, or else a decimal, the event
    the demo plays.

    A demo.c without demos[] fails, naming demo.c. One whose demos[] lies
    in a conditional fails, naming its line. So does one with a row, or
    anything between rows, in another form. recorded_demos() fails too
    where code_only() or conditions() fails on demo.c."""
    text = code_only(csource.code(DEMO_C), DEMO_C)
    table = re.search(r'\bdemos\[\w*\]\s*=\s*\{(.*?)\};', text, re.S)
    if table is None:
        fail(f'{DEMO_C}: holds no demos[] for the memory contract to read')
    line = text.count('\n', 0, table.start())
    held = conditions(text, DEMO_C)[line]
    if held:
        fail(f"{DEMO_C}:{line + 1}: demos[] lies under {held[-1]}, whose sides the memory contract doesn't tell apart")
    found = []
    for item in re.compile(r'\{([^{}]*)\}|[^\s,{}]+|[{}]').finditer(text, *table.span(1)):
        row = re.fullmatch(r'\s*(STAGE_\w+),\s*(\d+),\s*(CHARACTER_\w+),\s*(DEMO_NO_EVENT|\d+)\s*', item[1] or '')
        if row is None:
            fail(f'{DEMO_C}:{text.count(chr(10), 0, item.start()) + 1}: {item.group()}, a row of demos[] in a form the '
                 "memory contract doesn't read")
        if row[4] == 'DEMO_NO_EVENT':
            found.append((row[1], int(row[2]), row[3]))
    return found


def stage_events():
    """The events stage.c's entry_events and clear_events give, in their
    order. Each is (STAGE_X, act, CHARACTER_Y, event). stage_start_event
    loads an entry_events event as a character comes to a stage's act, and
    stage_clear_event loads a clear_events event as the character clears
    it. A row is {STAGE_X, ACT, CHARACTER_Y, FLAG, EVENT, DESTRUCTOR}.
    CHARACTER_Y is a CHARACTER_ macro or 0. ACT and EVENT are decimals,
    FLAG is a number, and DESTRUCTOR is NULL or a cast of a function's
    name. STAGE_NONE's row ends each table, and gives no event.

    A stage.c that lacks either table fails, naming stage.c. One with
    either table in a conditional fails, naming its line. So does one with
    a row, or anything between rows, in another form. stage_events() fails
    too where code_only() or conditions() fails on stage.c."""
    text, found = code_only(csource.code(STAGE_C), STAGE_C), []
    held = conditions(text, STAGE_C)
    for name in ('entry_events', 'clear_events'):
        table = re.search(rf'\b{name}\[\]\s*=\s*\{{(.*?)\}};', text, re.S)
        if table is None:
            fail(f'{STAGE_C}: holds no {name}[] for the memory contract to read')
        line = text.count('\n', 0, table.start())
        if held[line]:
            fail(f"{STAGE_C}:{line + 1}: {name}[] lies under {held[line][-1]}, whose sides the memory contract "
                 "doesn't tell apart")
        for item in re.compile(r'\{([^{}]*)\}|[^\s,{}]+|[{}]').finditer(text, *table.span(1)):
            row = re.fullmatch(r'\s*(STAGE_\w+),\s*(\d+),\s*(CHARACTER_\w+|0),\s*(?:0x[0-9a-f]+|\d+),\s*(\d+),\s*'
                               r'(?:NULL|\([\w\s]*\(\*\)\([\w\s*]*\)\)\s*\w+)\s*', item[1] or '')
            if row is None:
                fail(f'{STAGE_C}:{text.count(chr(10), 0, item.start()) + 1}: {" ".join(item.group().split())}, a row '
                     f"of {name}[] in a form the memory contract doesn't read")
            if row[1] != 'STAGE_NONE':
                found.append((row[1], int(row[2]), row[3], int(row[4])))
    return found


def by_file():
    """Each image, by the lowercase name of its file without its directory,
    such as stg01.prs for SONICADV/STG01.PRS. A read's file, as loader.c
    spells it, such as stg01.PRS, finds its image once lowercased. Two
    images whose files share that name fail, naming both, as a read names
    one image by its file."""
    found = {}
    for image, path in files().items():
        name = Path(path).name.lower()
        if name in found:
            fail(f'{found[name]} and {image}: their files are both {name} once lowercased, and a read names one '
                 'image by its file')
        found[name] = image
    return found


def contract(images):
    """The Contract for images, the names of images config/ describes, each
    once. A state holds no image that images leaves out, and no work past
    one. contract() reads config/'s tables, the sources SOURCES names and
    the symbols.txt of each field MESSAGES names. stage_states() and check()
    read no source.

    contract() fails, naming the file, image or state at fault, on:
    - an image config/ doesn't describe, or one images names twice;
    - two images whose files share a name once lowercased, as by_file()
      fails;
    - a source whose quotes code_only() fails on, or whose conditionals
      conditions() fails on;
    - a macro MACROS lists that its file gives no number for;
    - a function of loader.c's that cases() fails on;
    - a case that reads a PRS file to EXTRA_AREA, where EXTRA_AREA isn't
      EVENT_MODULE;
    - a demos[] that recorded_demos() fails on, or a stage in which it
      plays recorded pads that no case reads;
    - a stage.c that stage_events() fails on, or an event it gives a
      recorded demo that no image is;
    - a read of a recording, a cast texture, the casino's palette or the
      board motions in a form the contract doesn't read;
    - set and camera areas that start less than AREA_SIZE apart;
    - a cast_load whose textures aren't those CAST gives sizes for;
    - a texture.c that defines or undefines FILE_ROOM for GCC other than
      once, as ROOM_DEFINITION, on the GCC side of an #ifdef GCC;
    - a field of MESSAGES whose symbols.txt gives no field_messages;
    - a PRS file loader.c or ICECAP_C reads to an area no source the
      contract reads gives;
    - an image but 1ST_READ.BIN whose origin isn't the destination of each
      read of it, or whose loader the contract doesn't read;
    - an area a state holds that no source the contract reads gives;
    - an area, or an image's origin, outside RAM, or an area that ends
      before it starts."""
    images, known = list(images), files()
    wrong = [f"{image}: no image config/ describes it" for image in images if image not in known]
    wrong += [f'{image}: images names it twice, and the memory contract takes each image once'
              for image in dict.fromkeys(images) if images.count(image) > 1]
    if wrong:
        fail(*wrong)
    given = numbers()
    loader, play, strt2 = given[LOADER_H], given[PLAY_C], given[STRT2_C]
    # the areas a read can name, by macro
    places = {**{name: physical(loader[name]) for name in LOADER_AREAS},
              **{name: physical(given[LOADER_C][name]) for name in MACROS[LOADER_C]},
              'EVENT_MODULE': physical(play['EVENT_MODULE'])}
    heap_size, loader_sbinit = given[SBINIT_C]['HEAP_SIZE'], given[LOADER_SBINIT_C]
    areas = {'stack': Owner('stack', physical(strt2['STACK_BOTTOM']), physical(strt2['STACK_TOP'])),
             'heap': Owner('heap', RAM_END - heap_size, RAM_END),
             'loader heap': Owner('loader heap', physical(loader_sbinit['HEAP']),
                                  physical(loader_sbinit['HEAP']) + loader_sbinit['HEAP_SIZE']),
             'work': Owner('work', physical(loader['WORK_AREA']), physical(loader['WORK_AREA_END']))}
    sized = [Owner(name, start, start + AREA_SIZE) for name, start in places.items()
             if re.fullmatch(r'(SET|CAMERA)_AREA_[0-5]', name)]
    if overlapping(sized):
        fail(f'{LOADER_H}: its set and camera areas hold less than {AREA_SIZE:#x} bytes each')
    areas.update((owner.name, owner) for owner in sized)
    # download.c's two model buffers, each as long as the gap between the
    # two, then stream.c's work for the music and the voices
    download, stream, movie = given[DOWNLOAD_C], given[STREAM_C], given[MOVIE_C]
    models = physical(download['MODEL_A_BUFFER']), physical(download['MODEL_B_BUFFER'])
    areas['download models'] = Owner('download models', models[0], 2 * models[1] - models[0])
    areas['stream work'] = Owner('stream work', physical(stream['STREAM_WORK']), physical(stream['STREAM_WORK'])
                                 + stream['MUSIC_WORK_SIZE'] + stream['VOICE_WORK_SIZE'])
    # modules/movie/movie.c's work, where Sofdec's player decodes and where
    # texture_load_prs_file decodes the cast's textures
    areas['movie work'] = Owner('movie work', physical(movie['WORK']), physical(movie['WORK']) + movie['WORK_SIZE'])
    recording = physical(given[RECORDER_H]['DEMO_RECORDING'])
    areas['DEMO_RECORDING'] = Owner('DEMO_RECORDING', recording, recording + RECORDING_SIZE)
    textures = sorted({file for file, in calls(CAST_C, 'texture_load_prs_file', r'"([^"]+)",\s*&\w+')})
    if textures != sorted(CAST):
        fail(f"{CAST_C}: cast_load reads {', '.join(textures) or 'nothing'}, where the memory contract gives the "
             f"sizes of {', '.join(sorted(CAST))}")
    text = csource.code(TEXTURE_C)
    room_lines = [(line.strip(), held) for line, held in zip(text.split('\n'), conditions(text, TEXTURE_C))
                  if re.match(r'\s*#\s*(?:define|undef)\s+FILE_ROOM\b', line)]
    if [line for line, held in room_lines if held == ('#ifdef GCC',)] != [ROOM_DEFINITION] or any(
            held not in (('#ifdef GCC',), ('#ifdef GCC #else',)) for _, held in room_lines):
        fail(f'{TEXTURE_C}: defines FILE_ROOM for GCC other than once, as {ROOM_DEFINITION}, on the GCC side of an '
             "#ifdef GCC: the room the memory contract gives a texture's read")
    # the cast's room: the largest texture's whole sectors, just below
    # WORK_AREA
    work_area = areas['work'].start
    largest = max(aligned(size, loader['SECTOR_SIZE']) for size in CAST.values())
    areas['cast room'] = Owner('cast room', work_area - largest, work_area)
    # each field's messages run from field_messages, where the field's tables
    # put it, through its longest file's whole sectors
    anchored = {}
    for image, (_, size) in MESSAGES.items():
        if image in images:
            table = {name: address for address, name in named(Path('config', image, 'symbols.txt'))}
            if 'field_messages' not in table:
                fail(f'config/{image}/symbols.txt: gives no address for field_messages, an anchor of the memory '
                     'contract')
            anchored[image] = {'field_messages': table['field_messages']}
            start = physical(table['field_messages'])
            areas[f'{image} field_messages'] = Owner(f'{image} field_messages', start,
                                                     start + aligned(size, loader['SECTOR_SIZE']))
    first_read_symbols = {name: address for address, name in Config('1st_read').symbols()}
    origins = {image: Config(image).base for image in images}
    names, staged = by_file(), cases('void loader_read_stage_files(void)')
    motioned = cases('void loader_read_character_motions(int character)')
    board_reads = calls(ICECAP_C, 'loader_read_prs', r'"([^"]+)",\s*(\w+)') if FISH_IMAGE in images else []
    # the areas casino.c reads its palette to. A destination casino.h
    # defines as another macro, as MUDDY_PALETTE is SET_AREA_4, stands for
    # that macro's area.
    palette = []
    if CASINO in images:
        spelled = {name: area for name, area in definitions(CASINO_H).items() if re.fullmatch(r'[A-Z_]\w*', area)}
        palette = sorted({spelled.get(dest, dest) for dest, in calls(CASINO_C, 'loader_read_file',
                                                                     r'"[^"]+",\s*(\w+)')})
    # each PRS file loader.c and ICECAP_C read, as (reader, file,
    # destination)
    prs_reads = [(f"loader.c's loader_read_stage_files, for {' and '.join(labels)},", file, dest)
                 for labels, reads, _ in staged for file, dest in reads]
    prs_reads += [(f"loader.c's loader_read_character_motions, for {' and '.join(labels)},", file, dest)
                  for labels, reads, _ in motioned for file, dest in reads]
    prs_reads += [(str(ICECAP_C), file, dest) for file, dest in board_reads]
    # each read of an image but 1ST_READ.BIN, as (image, reader,
    # destination). The boot ROM reads 1ST_READ.BIN to no place a source
    # gives.
    image_reads = [('advertise', 'main.c', places['MODULE_AREA']), ('summary', 'main.c', places['MODULE_AREA']),
                   ('movie', "movie.c's movie_play", physical(first_read_symbols.get('movie_entry', 0))),
                   ('loader', 'main.c', physical(first_read_symbols.get('loader_buffer', 0)))]
    image_reads += [(names[file.lower()], who, places[dest]) for who, file, dest in prs_reads
                    if file.lower() in names and dest in places]
    # a case that reads a PRS file to EXTRA_AREA holds no event, which
    # needs EXTRA_AREA to be where event_load reads each event
    extra = [file for _, reads, _ in staged for file, dest in reads if dest == 'EXTRA_AREA']
    if extra and places['EXTRA_AREA'] != places['EVENT_MODULE']:
        fail(f"{LOADER_C}: reads {extra[0]} to EXTRA_AREA, {hexa(places['EXTRA_AREA'])}, and {PLAY_C}'s "
             f"EVENT_MODULE, where event_load reads each event, is {hexa(places['EVENT_MODULE'])}, so the case's "
             "state can't leave the event out")
    # the demos that play recorded pads, the events a stage's acts load, and
    # the areas demo_advance reads the recordings to
    recorded, act_events = recorded_demos(), stage_events()
    recordings = sorted({dest for dest, in calls(DEMO_C, 'loader_read_file', r'"[^"]+",\s*(\w+)')})
    unstaged = [stage for stage in dict.fromkeys(stage for stage, _, _ in recorded)
                if not any(stage in labels for labels, _, _ in staged)]
    if unstaged:
        fail(*(f"{DEMO_C}: demos[] plays recorded pads in {stage}, which no case of loader.c's "
               'loader_read_stage_files reads' for stage in unstaged))
    # the motion files: set_character has loader_read_character_motions
    # read the character's to MOTION_AREA, where it stays as the stages
    # play. A file that is no image holds a room from MOTION_AREA.
    motions = sorted({names[file.lower()] for _, reads, _ in motioned for file, _ in reads
                      if file.lower() in names and names[file.lower()] in images})
    motion_rooms = sorted({f'{dest} file' for _, reads, _ in motioned for file, dest in reads
                           if file.lower() not in names})
    events = sorted(image for image in images if image.startswith('ev'))
    image_reads += [(event, "play.c's event_load", places['EVENT_MODULE']) for event in events]
    problems = [f'{file}: {who} reads it to {dest}, an area no source the memory contract reads gives'
                for who, file, dest in prs_reads if dest not in places]
    problems += [f'{image}: its tables link it at {hexa(origins[image])}, and {who} reads it to {hexa(dest)}'
                 for image, who, dest in image_reads if image in images and physical(origins[image]) != dest]
    problems += [f'{image}: no loader the memory contract reads reads it' for image in images
                 if image != '1st_read' and image not in {image for image, _, _ in image_reads}]
    if problems:
        fail(*dict.fromkeys(problems))
    states = stage_states(staged, names, anchored, board_reads, palette, recorded, act_events, recordings)
    main = ['stack', 'heap']
    states += [State('the menus', {'1st_read': 'whole', 'advertise': 'whole'},
                     main + ['work', 'stream work', 'advertise work'], False),
               State("a demo's start", {'1st_read': 'whole', 'advertise': 'whole'},
                     main + ['work', 'stream work', 'advertise work', *recordings], False, True, apart=True),
               State('the summary', {'1st_read': 'whole', 'summary': 'whole'}, main + ['work', 'stream work'], False),
               State('the movie', {'1st_read': 'whole', 'movie': 'read'}, main + ['movie work', 'cast room'], False),
               State('the hand-off', {'1st_read': 'whole', 'loader': 'file'}, main, False, apart=True),
               State('the loader', {'loader': 'whole'}, ['stack', 'loader heap'], False)]
    # each state holds each of its areas once, and no work past an image
    # that images leaves out
    absent = {f'{image} work' for image in WORK if image not in images}
    states = [state._replace(images={image: part for image, part in state.images.items() if image in images},
                             areas=[name for name in dict.fromkeys(state.areas) if name not in absent])
              for state in states]
    areas.update(file_rooms(states, places, origins, areas, events, motions, motion_rooms))
    unknown = [f'in {state.name}: {name}, an area no source the memory contract reads gives' for state in states
               for name in state.areas if name not in areas and name not in {f'{image} work' for image in WORK}]
    if unknown:
        fail(*dict.fromkeys(unknown))
    areas['event zeroing'] = Owner('event zeroing', places['EVENT_MODULE'],
                                   places['EVENT_MODULE'] + play['EVENT_MODULE_SIZE'])
    outside = [f"{name}: starts at {hexa(start)} and ends at {hexa(end)}, which don't run in that order within RAM, "
               f'{hexa(RAM)} to {hexa(RAM_END)}' for name, start, end in areas.values() if not inside(start, end)]
    outside += [f"{image}: its tables link it at {hexa(origin)}, which isn't in RAM, {hexa(RAM)} to {hexa(RAM_END)}"
                for image, origin in origins.items() if not RAM <= physical(origin) < RAM_END]
    if outside:
        fail(*outside)
    shinobi = given[SHINOBI_H]
    work = {image: (shinobi['BUD_CAPACITY_1MB'] * CARD_BLOCK_SIZE + shinobi['BUD_WORK_GAP']) * DRIVES
            if kind == 'backup' else aligned(max(size for _, size in BANKS[kind]), loader['SECTOR_SIZE'])
            for image, (_, kind) in WORK.items() if image in images}
    lengths = interval_lengths(origins, areas, states, events, motions, motion_rooms)
    return Contract(origins, lengths, areas, anchored, states, events, sorted(motions + motion_rooms),
                    colive(states, images, events, motions), work, loader['SECTOR_SIZE'])


def file_rooms(states, places, origins, areas, events, motions, motion_rooms):
    """The room of each data file a state of states holds, and each room of
    motion_rooms, as Owners by name. motion_rooms are the rooms of the
    motion files that are no image. A room's name is its area's macro with
    file after it, as SET_AREA_1 file. It starts at that area's address in
    places. It ends at the nearest start past there of another owner in a
    state that holds the file: an image, an area of areas, another room, the
    event at EVENT_MODULE or a motion file of motions. Where none starts
    past there, the room ends at RAM's end.

    A state holds each room of motion_rooms where it holds any one motion
    file. No other motion file ends such a room, as a state holds one at
    most. The work past an image ends no room, as areas doesn't place it."""
    starts = {name: places[name[:-len(' file')]] for name in motion_rooms + [
        name for state in states for name in state.areas] if name.endswith(' file')}
    found = {}
    for name, start in sorted(starts.items()):
        later = [RAM_END]
        for state in states:
            if name not in state.areas and not (state.motions and name in motion_rooms):
                continue
            owners = [physical(origins[image]) for image in state.images]
            owners += [areas[other].start if other in areas else starts[other] for other in state.areas
                       if other in areas or other in starts]
            if state.event and events:
                owners.append(places['EVENT_MODULE'])
            if state.motions and name not in motion_rooms:
                owners += [physical(origins[motion]) for motion in motions]
                owners += [starts[room] for room in motion_rooms]
            later += [owner for owner in owners if owner > start]
        found[name] = Owner(name, start, min(later))
    return found


def colive(states, images, events, motions):
    """The images each of images may take names from, by image: those a
    state of states holds with it, except a state whose images are apart.
    events and motions are the events and the motion files that are images.
    A state's one event and one motion file, each any one of them, are live
    with its images and with each other. No event is live with another
    event, nor a motion file with another motion file."""
    found = {image: set() for image in images}
    for state in states:
        if state.apart:
            continue
        choices = (set(events) if state.event else set()) | (set(motions) if state.motions else set())
        for image in state.images:
            found[image] |= set(state.images) - {image} | choices
        for image in choices:
            found[image] |= set(state.images) | (choices - set(events if image in events else motions))
    return found


def interval_lengths(origins, areas, states, events, motions, motion_rooms):
    """The bytes of the interval of each image origins gives, by image, as
    the module docstring defines it. areas gives each area's and room's
    Owner. events and motions are the events and the motion files that are
    images, and motion_rooms the rooms of the motion files that are no
    image. A state that may hold any one event counts as holding each event
    whole. One that may hold any one motion file counts as holding each of
    motions whole and each room of motion_rooms."""
    lengths = {}
    for image, origin in origins.items():
        start, later = physical(origin), [RAM_END]
        for state in states:
            held = dict(state.images, **({event: 'whole' for event in events} if state.event else {}),
                        **({motion: 'whole' for motion in motions} if state.motions else {}))
            if held.get(image) not in ('whole', 'file', 'read'):
                continue
            later += [areas[name].start for name in state.areas + (motion_rooms if state.motions else [])
                      if name in areas and areas[name].start > start]
            later += [physical(origins[other]) for other in held if physical(origins[other]) > start]
        lengths[image] = min(later) - start
    return lengths


def stage_states(staged, names, anchored, board_reads, palette, recorded, act_events, recordings):
    """The states of staged, loader_read_stage_files' cases as cases() gives
    them. Each case gives its own state. A case that reads FISH_IMAGE,
    stg08, gives Icecap's state with the board motions as well, right after
    its own. A case whose labels name a stage of recorded gives each of
    those states again, as a recorded demo's. Each recorded demo's state
    comes again for each event act_events gives its demo, as that event
    loads.

    The other arguments:
    - names is by_file()'s, and anchored is contract()'s anchors;
    - board_reads is each read of ICECAP_C, as (file, destination);
    - palette is the areas casino.c reads its palette to, sorted;
    - recorded is recorded_demos()'s, and act_events is stage_events()'s;
    - recordings is the areas demo_advance reads the recordings to.

    A case's state holds 1ST_READ.BIN and each image the case reads, whole.
    Its areas are the stack, the heap, the work area, the model buffers and
    the stream work, then the areas the case fills. Each data file the case
    reads adds its room, named by its area's macro with file after it, as
    SET_AREA_1 file. Each image adds its anchors and the work past it. Where
    the case reads Casinopolis' image, the state adds palette. The state
    holds any one motion file too, and any one event where the case reads
    no PRS file to EXTRA_AREA.

    Icecap's state with the board motions holds stg08's body, not its tail,
    and adds the board motions and the data files ICECAP_C reads. A
    recorded demo's state adds recordings and holds no event. One as an
    event loads holds that event whole. The event's image is evNNNN, for
    event NNNN in hex, whose file event_load reads.

    stage_states() fails, naming stage.c, on an event act_events gives a
    recorded demo that no image is."""
    states = []
    for labels, reads, filled in staged:
        held = {names[file.lower()]: 'whole' for file, _ in reads if file.lower() in names}
        data = [f'{dest} file' for file, dest in reads if file.lower() not in names]
        areas = ['stack', 'heap', 'work', 'download models', 'stream work', *filled, *data,
                 *(f'{image} {name}' for image in held for name in anchored.get(image, ())),
                 *(f'{image} work' for image in held if image in WORK)]
        if CASINO in held:
            areas += palette
        name = ' and '.join(label.lower() for label in labels)
        event = not any(dest == 'EXTRA_AREA' for _, dest in reads)
        first = len(states)
        states.append(State(name, {'1st_read': 'whole', **held}, areas, event, True))
        if FISH_IMAGE in held:
            states.append(State(f'{name} with the board motions',
                                {'1st_read': 'whole', **held, FISH_IMAGE: 'body',
                                 **{names[file.lower()]: 'whole' for file, _ in board_reads if file.lower() in names}},
                                areas + [f'{dest} file' for file, dest in board_reads if file.lower() not in names],
                                event, True))
        if {stage for stage, _, _ in recorded} & set(labels):
            demo = [state._replace(name=f'a recorded demo of {state.name}', areas=state.areas + recordings,
                                   event=False) for state in states[first:]]
            loaded = [f'ev{event:04x}' for stage, act, character, event in act_events
                      if (stage, act, character) in recorded and stage in labels]
            unknown = [event for event in loaded if f'{event}.prs' not in names]
            if unknown:
                fail(f"{STAGE_C}: gives {unknown[0]} a recorded demo of {name} can load, which no image is")
            states += demo + [state._replace(name=f'{state.name} as {event} loads',
                                             images={**state.images, names[f'{event}.prs']: 'whole'})
                              for state in demo for event in loaded]
    return states


def records(image, events):
    """The records the loader of image reads at fixed offsets, each as
    RECORDS gives one. events is the contract's events. An event's one
    record is its head, EVENT_HEAD with the event's name for {image}. Any
    other image's records are those RECORDS gives it, or none."""
    if image in events:
        return (tuple(field.format(image=image) if isinstance(field, str) else field for field in EVENT_HEAD),)
    return RECORDS.get(image, ())


def overlapping(owners):
    """A problem for each pair of owners that overlap, in the order of their
    starts. Each owner is (name, start, end). An owner whose end isn't past
    its start holds no byte and overlaps none."""
    found = []
    ordered = sorted((owner for owner in owners if owner[1] < owner[2]), key=lambda owner: (owner[1], owner[2]))
    for at, (name, start, end) in enumerate(ordered):
        for other, low, high in ordered[at + 1:]:
            if low >= end:
                break
            found.append(f'{name}, {hexa(start)} to {hexa(end)}, overlaps {other}, {hexa(low)} to {hexa(high)}')
    return found


def check(terms, extents):
    """The problems that terms, the memory contract, finds with extents,
    each problem once. check() reports each problem and fails on none.
    extents gives each image's Extent by name, and must give each image the
    contract holds and no other.

    A state holds each of its images from the image's origin: whole, to its
    end; its body, to its body's end; its file, to its sectors' end; and its
    read, to the later of its end and its sectors' end. It holds an event to
    the later of its end and the end of the bytes event_load zeroes. The work
    past an image lies where its Extent gives it. A state that can hold an
    event or a motion file is checked once for each one it can hold. Where
    it can hold both, each event is checked with each motion file. No check
    holds a set, camera or data file to its area or room, as the contract
    sizes none of them.

    Where extents leaves out an image or gives another, check() reports
    those problems alone. Otherwise it reports:
    - an image that runs past its interval;
    - an Extent whose body's end and end don't run in that order from its
      origin to RAM's end;
    - an Extent whose file's end and sectors' end don't run so;
    - work that doesn't run in order within RAM;
    - an image the contract holds work past whose Extent gives no work, or
      work of another size than the contract's;
    - an event whose bss runs past both its file and the bytes event_load
      zeroes;
    - a PRS file whose size in whole sectors passes the work area's;
    - two owners a state holds that overlap."""
    problems = [f'{image}: extents give it no Extent, and the memory contract holds it' for image in terms.origins
                if image not in extents]
    problems += [f"{image}: extents give it an Extent, and the memory contract doesn't hold it" for image in extents
                 if image not in terms.origins]
    if problems:
        return problems
    areas = dict(terms.areas)
    work_area = areas['work'].end - areas['work'].start
    zeroing = areas['event zeroing']
    for image, extent in extents.items():
        origin = terms.origins[image]
        if not inside(extent.body, extent.end, physical(origin)):
            problems.append(f"{image}: its body ends at {hexa(extent.body)} and it ends at {hexa(extent.end)}, which "
                            f"don't run in that order from its origin, {hexa(origin)}, to RAM's end, {hexa(RAM_END)}")
        if not inside(extent.stored, extent.sectors, physical(origin)):
            problems.append(f"{image}: its file ends at {hexa(extent.stored)} and its sectors at "
                            f"{hexa(extent.sectors)}, which don't run in that order from its origin, {hexa(origin)}, "
                            f"to RAM's end, {hexa(RAM_END)}")
        if extent.work and not inside(*extent.work):
            problems.append(f"{image}: its work starts at {hexa(extent.work[0])} and ends at {hexa(extent.work[1])}, "
                            f"which don't run in that order within RAM, {hexa(RAM)} to {hexa(RAM_END)}")
        if physical(extent.end) - physical(origin) > terms.lengths[image]:
            problems.append(f'{image}: it runs to {hexa(extent.end)}, past its interval, which ends at '
                            f'{hexa(origin + terms.lengths[image])}')
        if image in terms.events and physical(extent.end) > max(physical(extent.stored), zeroing.end):
            problems.append(f'{image}: its bss runs to {hexa(extent.end)}, past its file, which ends at '
                            f'{hexa(extent.stored)}, and past the bytes event_load zeroes, which end at '
                            f'{hexa(zeroing.end)}')
        size = terms.work.get(image)
        if extent.work:
            work = areas[f'{image} work'] = Owner(f'{image} work', *map(physical, extent.work))
            if size is not None and work.end - work.start != size:
                problems.append(f'{image}: its work, {hexa(extent.work[0])} to {hexa(extent.work[1])}, takes '
                                f'{work.end - work.start:#x} bytes, and the memory contract holds {size:#x}')
        elif size is not None:
            problems.append(f'{image}: its extent gives no work, and the memory contract holds {size:#x} bytes of '
                            'work past it')
        if extent.packed is not None and extent.packed > work_area:
            problems.append(f'{image}: its PRS file takes {extent.packed:#x} bytes in whole sectors, and the work '
                            f'area loader_read_prs reads it to holds {work_area:#x}')

    def owner(image, part):
        # an event holds the bytes event_load zeroes for it, and its own
        # bytes past them
        extent = extents[image]
        end = {'whole': extent.end, 'body': extent.body, 'file': extent.sectors,
               'read': max(extent.end, extent.sectors, key=physical)}[part]
        end = max(physical(end), zeroing.end) if image in terms.events else physical(end)
        return image + ('' if part == 'whole' else f"'s {part}"), physical(terms.origins[image]), end

    for state in terms.states:
        owners = [owner(image, part) for image, part in state.images.items()]
        # work that extents leave out is reported above
        owners += [areas[name] for name in state.areas if name in areas]
        # the state's one event, any one, and its one motion file, the
        # character's
        events = [owner(event, 'whole') for event in terms.events] if state.event else []
        motions = [owner(motion, 'whole') if motion in extents else areas[motion]
                   for motion in terms.motions] if state.motions else []
        for event in events or [None]:
            for motion in motions or [None]:
                problems += [f'in {state.name}: {problem}'
                             for problem in overlapping(owners + [one for one in (event, motion) if one])]
    return list(dict.fromkeys(problems))


def holds_bss(config):
    """Whether the original file of config's image holds any of its bss:
    whether a section of its tables that isn't bss lies past one that is."""
    layout = config.layout()
    return any(start >= end and later not in ZEROED for name, _, end in layout if name in ZEROED
               for later, start, _ in layout)
