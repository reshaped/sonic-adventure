#!/usr/bin/env python3
"""What the contracts keep apart: a renderer backend and the runtime read no
Ninja state and no game memory, the runtime includes no game's header, and a
record holds no pointer, so nothing past a draw's call can follow one into the
game. A pad and its shaking reach the backend in the core's terms, sa_pad's
bits and sa_input_rumble's strengths and times, so it names none of Shinobi's
pad or vibration calls or types, and a save in sa_storage_*'s bytes and names,
so none of the backup library's. The front door finds a player's settings and
card and the disc's label, and the backend takes them as text and bytes, so it
reads no environment and names no file of a disc. A picture's aspect reaches
the backend by sa_present_mode and sa_present_window, and a 2D draw's squeeze
in its record, so it names none of 16:9's game side: the aspects, the pivot,
the cover or the modes. What a step reads of the port's features is the
game's, so it names none of that either: the toggles, their chord, the
curtain's count or whether the frames want the split. Nor does a file of the
slice's directory, engine/native/, which names none of the window's either:
it gives the first pad's samples to the game's adapter, which alone of the
PC's side calls the chord and sets a toggle.
The Dreamcast's model walk, sprite sends and screen
sends name none of Ninja's state or calls and none of the game's, but 16:9's
picture and squeeze, the Dreamcast engine's own, and include the SDK's
headers, SH4ZAM's, the contracts, the engine's and their own alone: they read
what the layers lend them. The Dreamcast engine's batch
kernels, which the walk runs, know no game: they name no type or call of the
SDK's or SA1's, and include zam.h, stdint.h and SH4ZAM's headers alone. A
header gives the Dreamcast's sa_present_live for one layer's formats, and no
object includes two. The PC's backend draws a resource or a payload alone
and gives no sa_present_live: the game's adapter gives it out of line. The
game's port.h and the engine's dreamcast.h include the SDK's types alone,
since ninja.h includes port.h. The game's frame end asks its mode by the
core's calls alone and takes the window it asked and the rate applied, so it
names none of the picture's 16:9 modes, which a backend makes of what it is
asked, and the Dreamcast's pacing names none of the rules that say where the
game's frames may split. A kept draw is the core's
sa_draw_kept, which holds no pointer, so it takes 16 bytes on every
backend. The Dreamcast's keep keeps its draws as it, and so does the PC's
render-owned scene, which holds no draw, sprite or triangles record whole
beside it. The frame's owner and owner.h, its calls past Kamui's, include
Kamui's and the chip's headers alone, and name none of Ninja's state or
calls and none of the game's. The split frame, which a host links too,
names nothing of a host's adapter or slice: a host's draws reach it as
port.h's port_draws_sent, and its pictures leave by Kamui's calls, or on
the Dreamcast a first picture by owner.h's, and the backend names nothing
of the split frame. The PC's overlay draws into the game's picture as the
Dreamcast's does, by Ninja's 2D polygon and the adapter's text record, so
it names no call or name of the engine's and includes neither the engine's
header nor a backend's. Play math, engine/numeric/'s library, knows nothing of
the engine or of a game: a source of it includes the library's own files, the
tables its fragment writes and C17's headers alone. A source outside it
includes one file of it, its header play_math.h, and no other file or table
of its, so the library's bodies stay its own to change. A contract, and a
backend's header beside one, includes no file of it at all, and names none of
the functions its header declares: play calls them and no backend does. Needs
nothing built.

It reads every file of the backend and the runtime but the shaders, whatever
its suffix, and a quoted include names one of theirs or the contracts'. The
backend's may name sa_sa1.h too: it gives SA1's palettes. A name of the
game's own that carries no Ninja prefix is the link's to find:
build/engine/tests/pictures links the backend with no game.

SA1's client, engine/sa1/'s C++, reads SA1's capture, the meshes it holds
and the textures it names from the disc. It reaches the backend through the
contracts alone: it reads no Ninja state, slice or environment either, and
a quoted include names its own, the contracts', SA1's own headers or the
PC's readers of a disc's files, disc.h, pvr.h and png.h, and nothing else
of the backend's.

Anywhere in engine/, a quoted include names a file of the tree, a header the
build writes, or a third party's that make fetches at the commit
tools/SHA256SUMS sums, as the movies' decoder comes: none comes unpinned.

The slice's C reaches the host's system, Linux's or Windows', in the runner's
files alone: runner.c, threads.c, gdfs.c and symbols.c. The rest of
engine/native/ and engine/sa1/ includes the C library's headers and the SDK's,
so it builds for either host.

config/parts.txt gives the directories of the PC's backend and the runtime,
a backend whose bin is picture; of the contracts; of SA1's adapter, a
recorder; of the slice's runner and shim, a backend whose bin is play; of
the Dreamcast's drawers; and of play math. Each is found by its thing, and a
backend's by its bin too, and a source of play math's is one under its
directory whose row gives it that thing. The sources that carry none of play
math are those whose row gives them the thing contract, wherever they lie,
and those under a directory of the contracts', whatever their thing, as the
PC backend's sa_pc.h is. engine/mk/play_math.mk names the library's header
and the tables it writes. The check names every other path itself."""
import functools
import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))

import parts  # noqa: E402
from images import SOURCE_ROOTS  # noqa: E402

# config/parts.txt's directories: the backend's and the runtime's, the
# contracts', a game's adapter's, the slice's runner and shim's, and the
# Dreamcast's drawers'. parts.directories() fails where a list would be empty
PARTS = parts.rows(ROOT / parts.PARTS)
OWN = parts.directories(PARTS, 'backend', parts.PICTURE)
CONTRACTS = parts.directories(PARTS, 'contract')
ADAPTER = parts.directories(PARTS, 'recorder')
HOST = parts.directories(PARTS, 'backend', parts.PLAY)
DRAWERS = parts.directories(PARTS, 'drawer')
SOURCES = [p for d in OWN for p in (ROOT / d).rglob('*') if p.is_file() and 'shaders' not in p.parts]
# the headers the build writes of the backend's shaders, one a shader
SHADERS = {f'{p.name}.h' for d in OWN for p in (ROOT / d / 'shaders').glob('*')}
# what a quoted include may name: the backend's and the runtime's own files,
# the contracts, and the shaders' generated words, and in the backend SA1's
# headers
INCLUDABLE = {p.name for p in SOURCES} | {p.name for d in CONTRACTS for p in (ROOT / d).glob('*.h')} | SHADERS
SA1 = {'sa_sa1.h'}
HEADERS = ('engine/include/sa_engine.h', 'engine/sa1/sa_sa1.h', 'engine/sa1/sa_capture.h')
# what a step reads of the port's features, the game's names: 16:9's window,
# the toggles, their chord, the curtain and whether the frames want the split
FEATURES = r'\bport_(view_aspect|view_x|split_wanted|toggles|chord|curtain\w*)\b|\bTOGGLE_\w+'
# Ninja's state, by any spelling a paste can build, its calls, the slice's
# names, Shinobi's pad, vibration and backup calls and types, 16:9's game
# side, the port's toggles, chord and curtain and the split frame's, the
# game's and SDK's headers, the environment and a disc's files by name
REFUSED = re.compile(r'_nj_\w*|\bnj[A-Z]\w*\s*\(|\bnative_\w+|\bpd[A-Z]\w*|\bPD[DS]_\w+|'
                     r'\bbu[A-Z]\w*\s*\(|\bBU[DSM]_\w+|'
                     r'\bport_(draw_aspect|wide_\w+|band|\w+_sent)\b|\b(WIDE|wide)_\w+|' + FEATURES + '|'
                     r'\bpresent_\w+|\bframe_(split|redrawing|later_draws|step)\b|\bFRAME_DRAWS\b|'
                     r'#\s*include\s*<(ninja|kamui|shinobi|sg_|nj|machine)|'
                     r'\b(secure_)?getenv\b|"\w[^"\n]*\.(PVR|PVM|PRS|BIN|ADX|SFD|MLT|KAT)"')
INCLUDE = re.compile(r'#\s*include\s*"([^"]*)"')
# SA1's client, and what it may include beside the contracts: SA1's headers
# but the slice's, and the PC's readers
CLIENT = sorted(p for d in ADAPTER for p in (ROOT / d).glob('*.cpp'))
CLIENT_INCLUDABLE = {p.name for d in ADAPTER for p in (ROOT / d).glob('*.h') if p.name != 'sa1.h'} | \
    {p.name for d in CONTRACTS for p in (ROOT / d).glob('*.h')} | {'disc.h', 'pvr.h', 'png.h'}
# the Dreamcast's model walk, sprite sends and screen sends: what they may include, and the
# game's names they may not reach past their state, beside Ninja's, and the
# Dreamcast engine's 16:9 names they may
WALK = sorted(p for d in DRAWERS for p in (ROOT / d).glob('*'))
WALK_INCLUDABLE = {'sa_dc.h', 'sa_engine.h', 'sa_sa1.h', 'rules.h', 'live.h', 'sa1/live.h', 'sa1/header.h',
                   'sa1/screen.h',
                   'sa1/sprite.h', 'sa1/strip.h', 'engine/dc/dreamcast.h', 'engine/dc/batch.h', 'batch.h',
                   'sdk/ninja/njkamui.h'}
WIDE = {'port_draw_aspect', 'port_wide_x', 'port_wide_squeezes'}
# the Dreamcast's sa_present_live, defined inline once a format, each in a
# header of its own, of which an object includes one at most. a quoted
# include is found beside its includer, then under the build's -I roots
DEFINES_LIVE = re.compile(r'\bsa_result\s+sa_present_live\s*\([^;{}]*\)\s*\{')
FILES = {p.relative_to(ROOT).as_posix() for d in SOURCE_ROOTS for p in (ROOT / d).rglob('*')}
ROOTS = ('', 'sdk/include', 'src', 'modules', 'events', 'engine/include', 'engine/sa1', 'engine/dc')
# the headers ninja.h includes, which include none of the SDK's but its types
TYPES_ONLY = ('src/system/port.h', 'engine/dc/dreamcast.h')
TYPES = {'sg_xpt.h', 'njtypes.h'}
WALK_SYSTEM = re.compile(r'(kamui|machine|ninja|sg_xpt)\.h|sh4zam/shz_\w+\.h')
WALK_REFUSED = re.compile(r'_nj_\w*|\bnj[A-Z]\w*\s*\(|\b(port|present|frame|event)_\w+|\bFRAME_DRAWS\b|'
                          r'\bdraw_(colors|palettes|unlit_offsets|debug_counts)\b|\bpolygon_sweep\b')
# the Dreamcast engine's batch kernels: SH4ZAM's, zam.h's and C's names
# alone, none of the SDK's types or the walk's refused names, and none of
# SA1's
KERNELS = 'engine/dc/batch.h'
KERNELS_SYSTEM = re.compile(r'zam\.h|stdint\.h|sh4zam/shz_\w+\.h')
KERNELS_REFUSED = re.compile(r'\b(NJS?|KM|SA)_\w+|\b(Sint|Uint)\d+\b|\b(Float|Bool|Int|Angle)\b|\bsa1_\w+|' +
                             WALK_REFUSED.pattern)
# the game's frame end, which asks the mode by sa_present_window and
# sa_present_mode, and names none of the picture's 16:9 modes, which a
# backend makes of the aspect and the window it is asked; the split frame's
# pacing, which names none of the game's toggles, stages or steps a frame,
# nor a host's adapter or slice
FRAME_END = 'src/system/pace.c'
FRAME_END_REFUSED = re.compile(r'\b(WIDE_(OFF|ON|PILLARBOX)|wide_mode|wide_applied)\b')
SPLIT_FRAME = 'src/system/present.c'
SPLIT_FRAME_REFUSED = re.compile(r'\b(port_toggles|TOGGLE_\w+|stage_current|stage_in_play|STAGE_\w+|'
                                 r'frame_step_count|demo_playing|recording_on|(native|sa1)_\w+)\b')
SYSTEM = re.compile(r'#\s*include\s*<([^>]*)>')
# the PC's overlay, which draws into the game's picture as the Dreamcast's does
OVERLAY = 'engine/sa1/overlay.c'
OVERLAY_REFUSED = re.compile(r'\b(sa|SA)_\w+|#\s*include\s*"(sa_engine|sa_pc|sa_dc)\.h"')
# the frame's owner, which any game's Dreamcast build links
OWNER = 'engine/dc/owner.c'
OWNER_HEADER = 'engine/dc/owner.h'
OWNER_SYSTEM = {'kamui.h', 'kmistat.h', 'machine.h'}
# C's and C++'s sources and their headers, by suffix
C_SOURCES = ('.c', '.cpp', '.h')
# what a quoted include in engine/ may name: the tree's sources, the headers
# the engine's make files write, the shaders' words and the fetched headers
ENGINE = [p for p in (ROOT / 'engine').rglob('*') if p.suffix in C_SOURCES and p.is_file()]
TREE = {p.name for d in SOURCE_ROOTS for p in (ROOT / d).rglob('*')}
WRITTEN = {Path(m).name for f in (ROOT / 'engine/mk').glob('*.mk')
           for m in re.findall(r'build/[\w/.-]+\.h\b', f.read_text())} | SHADERS
FETCHED = {Path(line.split()[1]).name for line in (ROOT / 'tools/SHA256SUMS').read_text().splitlines()
           if line.split()[1:2] and line.split()[1].endswith('.h')}
# the slice's C, the runner's files among it, and a system's own headers
SLICE = sorted(p for d in (*HOST, *ADAPTER) for p in (ROOT / d).glob('*.c'))
# each C source and header under the slice's directory, whatever its row
# gives it, which names none of FEATURES
HOST_FILES = sorted(p for d in HOST for p in (ROOT / d).rglob('*') if p.suffix in ('.c', '.h'))
RUNNER = {'runner.c', 'threads.c', 'gdfs.c', 'symbols.c'}
HOST_SYSTEM = re.compile(r'windows\.h|sys/.*|ucontext\.h|signal\.h|unistd\.h|dirent\.h|elf\.h|link\.h|fcntl\.h|'
                         r'dlfcn\.h|pthread\.h')
COMMENT = re.compile(r'/\*.*?\*/|//[^\n]*', re.S)
RECORD = re.compile(r'typedef struct \w+ \{(.*?)\} (\w+);', re.S)
POINTER = re.compile(r'^\s*(const\s+)?\w+\s*\*+\s*\w+\s*;', re.M)
# the one kept draw, and the Dreamcast's keep, which keeps its draws as it;
# the PC's render-owned scene, which does too, and names no record of a draw
# whole as a member, its own or a vector's
KEPT = 'sa_draw_kept'
SCENE = 'engine/pc/world.h'
WHOLE = re.compile(r'^\s*(?:std::vector<)?(sa_(?:draw|sprite|triangles))\b>?\s+\w+', re.M)
# play math: its fragment, which names the header a source outside the
# library includes and the tables it writes for the fast path; its sources,
# those under its directories that config/parts.txt gives its thing; the
# files a source of it may include by name, its own and the tables, of which
# a source outside it includes the header alone; C17's headers, the C
# library's; the contracts' sources and the files under their directories,
# which include none of them; and a name the header declares, a function's,
# which none of those names
MATH_RULES = 'engine/mk/play_math.mk'
MATH_HEADER = re.search(r'^PLAY_MATH_HEADER\s*:=\s*(\S+)$', (ROOT / MATH_RULES).read_text(), re.M)
MATH_TABLES = {Path(m).name for m in re.findall(r'build/[\w/.-]+\.h\b', (ROOT / MATH_RULES).read_text())}
MATH = sorted(f for d in parts.directories(PARTS, 'play-math') for f in FILES
              if f.startswith(d) and (row := parts.part(PARTS, f)) and row.thing == 'play-math')
MATH_OWN = {Path(f).name for f in MATH} | MATH_TABLES
C_LIBRARY = {f'{name}.h' for name in (
    'assert complex ctype errno fenv float inttypes iso646 limits locale math setjmp signal stdalign stdarg '
    'stdatomic stdbool stddef stdint stdio stdlib stdnoreturn string tgmath threads time uchar wchar wctype').split()}
CONTRACT_SOURCES = {f for f in FILES
                    if f.startswith(tuple(CONTRACTS)) or ((row := parts.part(PARTS, f)) and row.thing == 'contract')}
MATH_DECLARED = re.compile(r'\b(\w+)\s*\(')


def code(path):
    return COMMENT.sub('', path.read_text(errors='replace'))


@functools.cache
def found(where, name):
    return next((f for d in (where, *ROOTS) if (f := os.path.normpath(os.path.join(d, name))) in FILES), None)


@functools.cache
def includes(path):
    return {h for name in INCLUDE.findall(code(ROOT / path)) if (h := found(os.path.dirname(path), name))}


def reached(path):
    seen, todo = set(), [path]
    while todo:
        for header in includes(todo.pop()):
            if header not in seen:
                seen.add(header)
                todo.append(header)
    return seen


def main():
    faults = []
    for path in SOURCES:
        text = code(path)
        where = path.relative_to(ROOT)
        faults += [f'{where}: {m[0]}' for m in REFUSED.finditer(text)]
        faults += [f'{where}: defines sa_present_live' for m in DEFINES_LIVE.finditer(text)]
        allowed = INCLUDABLE | (SA1 if where.parts[1] == 'pc' else set())
        faults += [f'{where}: includes "{m[1]}"' for m in INCLUDE.finditer(text) if m[1] not in allowed]
    for path in CLIENT:
        text = code(path)
        where = path.relative_to(ROOT)
        faults += [f'{where}: {m[0]}' for m in REFUSED.finditer(text)]
        faults += [f'{where}: includes "{m[1]}"' for m in INCLUDE.finditer(text) if m[1] not in CLIENT_INCLUDABLE]
    for path in WALK:
        text = code(path)
        where = path.relative_to(ROOT)
        faults += [f'{where}: {m[0]}' for m in WALK_REFUSED.finditer(text) if m[0] not in WIDE]
        faults += [f'{where}: includes "{m[1]}"' for m in INCLUDE.finditer(text) if m[1] not in WALK_INCLUDABLE]
        faults += [f'{where}: includes <{m[1]}>' for m in SYSTEM.finditer(text) if not WALK_SYSTEM.fullmatch(m[1])]
    text = code(ROOT / KERNELS)
    faults += [f'{KERNELS}: {m[0]}' for m in KERNELS_REFUSED.finditer(text)]
    faults += [f'{KERNELS}: includes "{m[1]}"' for m in INCLUDE.finditer(text)]
    faults += [f'{KERNELS}: includes <{m[1]}>' for m in SYSTEM.finditer(text) if not KERNELS_SYSTEM.fullmatch(m[1])]
    live = {f for f in FILES if f.startswith('engine/dc/') and f.endswith('.h')
            and DEFINES_LIVE.search(code(ROOT / f))}
    if not live:
        faults.append('engine/dc: no header defines sa_present_live')
    for path in sorted(f for f in FILES if f.endswith('.c')):
        both = sorted(reached(path) & live)
        if len(both) > 1:
            faults.append(f'{path}: includes {" and ".join(both)}, each defining sa_present_live')
    for header in TYPES_ONLY:
        faults += [f'{header}: includes <{m[1]}>' for m in SYSTEM.finditer(code(ROOT / header)) if m[1] not in TYPES]
    for path in ENGINE:
        where = path.relative_to(ROOT)
        faults += [f'{where}: includes "{m[1]}", which nothing pins' for m in INCLUDE.finditer(code(path))
                   if Path(m[1]).name not in TREE | WRITTEN | FETCHED]
    for path in SLICE:
        if path.name not in RUNNER:
            faults += [f'{path.relative_to(ROOT)}: includes <{m[1]}>' for m in SYSTEM.finditer(code(path))
                       if HOST_SYSTEM.fullmatch(m[1])]
    for path in HOST_FILES:
        where = path.relative_to(ROOT)
        faults += [f"{where}: names {m[0]}, which is the game's: what a step reads of the port's features has one "
                   "copy, in src/system/port.c and pace.c, and the slice's directory names none of it. Give the "
                   "game's adapter what it needs, as pads.c gives sa1_chord the pad, and have it call the game"
                   for m in re.finditer(FEATURES, code(path))]
    for owner in (OWNER, OWNER_HEADER):
        text = code(ROOT / owner)
        faults += [f'{owner}: {m[0]}' for m in WALK_REFUSED.finditer(text)]
        faults += [f'{owner}: includes "{m[1]}"' for m in INCLUDE.finditer(text) if m[1] != 'owner.h']
        faults += [f'{owner}: includes <{m[1]}>' for m in SYSTEM.finditer(text) if m[1] not in OWNER_SYSTEM]
    faults += [f'{OVERLAY}: {m[0]}' for m in OVERLAY_REFUSED.finditer(code(ROOT / OVERLAY))]
    faults += [f'{FRAME_END}: {m[0]}' for m in FRAME_END_REFUSED.finditer(code(ROOT / FRAME_END))]
    faults += [f'{SPLIT_FRAME}: {m[0]}' for m in SPLIT_FRAME_REFUSED.finditer(code(ROOT / SPLIT_FRAME))]
    for header in HEADERS:
        for body, name in RECORD.findall(code(ROOT / header)):
            if 'sa_record_head head' in body and POINTER.search(body):
                faults.append(f'{header}: {name} is a record that holds a pointer')
    kept = [body for body, name in RECORD.findall(code(ROOT / HEADERS[0])) if name == KEPT]
    if not kept or POINTER.search(kept[0]):
        faults.append(f'{HEADERS[0]}: no {KEPT} that holds no pointer')
    for path in (SPLIT_FRAME, SCENE):
        if not re.search(rf'\b{KEPT}\b', code(ROOT / path)):
            faults.append(f'{path}: names no {KEPT}, the record it is to keep each draw as')
    faults += [f'{SCENE}: keeps an {m[1]} whole' for m in WHOLE.finditer(code(ROOT / SCENE))]
    for path in MATH:
        text = code(ROOT / path)
        faults += [f"{path}: includes \"{m[1]}\", which is none of play math's files or tables: the library knows "
                   "nothing of the engine or of a game, so it includes its own files and C17's headers alone"
                   for m in INCLUDE.finditer(text) if m[1] not in MATH_OWN]
        faults += [f"{path}: includes <{m[1]}>, which is none of C17's headers: the library includes its own files "
                   "and the C library's headers alone, so that it builds for any machine"
                   for m in SYSTEM.finditer(text) if m[1] not in C_LIBRARY]
    if not MATH_HEADER or MATH_HEADER[1] not in MATH:
        faults.append(f"{MATH_RULES}: PLAY_MATH_HEADER names no source of play math's: name the library's one "
                      'header for a source outside it, a file config/parts.txt gives the thing play-math')
    else:
        public = Path(MATH_HEADER[1]).name
        declared = set(MATH_DECLARED.findall(code(ROOT / MATH_HEADER[1])))
        for path in sorted(f for f in FILES if f.endswith(C_SOURCES) and f not in MATH):
            text = code(ROOT / path)
            named = [f'"{m[1]}"' for m in INCLUDE.finditer(text)] + [f'<{m[1]}>' for m in SYSTEM.finditer(text)]
            faults += [f"{path}: includes {name}, which is play math's own: a source outside the library includes "
                       f'{public} alone of it, and calls what that declares'
                       for name in named if Path(name[1:-1]).name in MATH_OWN - {public}]
            if path in CONTRACT_SOURCES:
                faults += [f'{path}: includes {name}: a contract and a header beside one carry none of play math, '
                           'which play calls and no backend does. Remove the include; a source that calls a number '
                           f'includes {public} itself'
                           for name in named if Path(name[1:-1]).name == public]
                faults += [f'{path}: names {name}, which {public} declares: a contract and a header beside one '
                           'carry none of play math, which play calls and no backend does. Remove it; '
                           f'{public} alone declares the numbers'
                           for name in sorted(set(re.findall(r'\w+', text)) & declared)]
    for fault in faults:
        print(fault)
    return 1 if faults else 0


if __name__ == '__main__':
    sys.exit(main())
