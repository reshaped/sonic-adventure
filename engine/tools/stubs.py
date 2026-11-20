"""Stubs for the native slice's missing names, each by its row of stubs.txt.

stubs.py OUT OBJECT... [--aliases FILE] [--base DOMAIN] [--libraries LIBRARY...]
[--explore] writes OUT, C that defines each name the objects use and none of
them defines, but for those the host's C library gives, glibc's or the
LIBRARIES', and FILE's, which the link names "NAME SYMBOL" a line. The
objects' and libraries' names come from the environment's NM, or else nm,
and a COFF object's calls from coff.py.

engine/native/stubs.txt allows each such name. A row is

    NAME  KIND  HOST  REASON

HOST is linux or windows, the host whose objects need the name, or * for
both. REASON says what the name is on the console, what of the game needs
it, and what the game makes of the stub. KIND is what stubs.py writes, and
whether the PC needs more:

- function, a function that reports its first call on stderr and returns
  0, in the integer register and in the float one alike, where the PC needs
  none of what the console's does: a host has no such hardware, the shim,
  the adapter or the engine does the work at another place, or nothing
  reaches the call;
- zeros, 64 KB of zeros, where the PC needs no more: the game keeps its own
  state there, or nothing reads what the console's holds;
- address, a place in the domain at DOMAIN that the matching side's tables
  give the name, which no source of the slice's holds. heap_arena lies in
  the view the image's code uses, with twice the room the tables give it,
  ending at MODULE_AREA, so no heap lies in the host's image or in RAM's
  other view. A name no object calls that 1ST_READ.BIN imports from an
  image the slice doesn't link, as config/1st_read/imports.txt gives it,
  lies where that image's tables place it, with the image's whole load area
  for its room: the game reads the loader's file there. The word an image's
  bounds hold, past which its code places work, Zero's and E-101 mkII's
  banks among it, holds the image's end in the domain as its tables give
  it, the value the disc's file holds there, so the work lies in the
  domain's RAM where the disc's image puts it;
- engine, nothing: the name is a call of the engine's that the PC backend
  defines, which play links beside the slice;
- unsupported, the stub function writes where an object calls the name, and
  the one zeros writes where none does, for what the game takes from the
  console's that the PC gives none of: an answer a caller acts on, a
  picture, a wait, a file. The game runs otherwise than on the console
  there, until a definition takes the stub's place and the row goes.

A row's kind is what the objects make of its name: function where one of
them calls it, address where the tables place it, and zeros where neither
holds. engine's name is one an object calls, and unsupported's one the
tables don't place. A .src file's untyped label is a function only where an
object calls it. stubs.py fails, and writes no OUT, on a name the objects
need that has no row for the host, on a row for the host that no object
needs, and on a row of a kind its name's use isn't. Given --explore it says
each of those and writes OUT all the same, each name's stub by its use, so
a build reaches its next missing name: engine/mk/native.mk's
NATIVE_EXPLORE. rows() fails on a row out of its form, naming its
path:line. stubs.py ends by saying how many names it wrote of each kind.
"""

import os
import re
import subprocess
import sys
from collections import namedtuple
from pathlib import Path

from coff import calls

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools'))
from config import Config, imports  # noqa: E402
from images import FIRST, images  # noqa: E402
from memory import LOADER_H, WORK, contract, macros, physical  # noqa: E402
from tables import NAME, named  # noqa: E402
from tables import rows as table_rows  # noqa: E402
from tool import fail  # noqa: E402

TABLE = Path(__file__).parents[1] / 'native/stubs.txt'
# what a row may give its kind and its host. The table's head says what
# each kind is
FUNCTION, ZEROS, ADDRESS, ENGINE, UNSUPPORTED = KINDS = ('function', 'zeros', 'address', 'engine', 'unsupported')
# the uses of a name that each kind stands for
FITS = {FUNCTION: {FUNCTION}, ZEROS: {ZEROS}, ADDRESS: {ADDRESS}, ENGINE: {FUNCTION}, UNSUPPORTED: {FUNCTION, ZEROS}}
HOSTS = ('linux', 'windows')
EVERY = '*'
# a row's kind and host, and its path:line
Row = namedtuple('Row', 'kind host where')
# the host's C library, whose definitions the slice links to by name
LIBC = '/lib/x86_64-linux-gnu/libc.so.6'
LIBM = '/lib/x86_64-linux-gnu/libm.so.6'
DATA_BYTES = 0x10000
HEAP = 'heap_arena'
# the alignment src/system/task.c gives heap_arena
HEAP_ALIGNMENT = 32
# the names the host's linker defines, which the shim reads, and the P2 call
# native.mk's link places
LINKER = {'__executable_start', 'etext', 'edata', 'end', '__data_start', '_end',
          '_GLOBAL_OFFSET_TABLE_', 'syCacheICI_P2_'}


def rows(path, host):
    """The rows of the table at path that speak for host, as {NAME: Row}.
    Fails, naming the row's path:line, on a row out of the form the module
    docstring gives, and on a second row that speaks for a host a name's
    earlier row speaks for."""
    found, seen = {}, {}
    for where, fields in table_rows(path):
        if len(fields) < 4 or not NAME.fullmatch(fields[0]):
            fail(f'{where}: not a row: NAME KIND HOST REASON')
        name, kind, given = fields[:3]
        if kind not in KINDS:
            fail(f'{where}: {kind}: not a kind, one of {", ".join(KINDS)}')
        if given != EVERY and given not in HOSTS:
            fail(f'{where}: {given}: not a host, {EVERY} or one of {", ".join(HOSTS)}')
        for other in HOSTS if given == EVERY else (given,):
            if (name, other) in seen:
                fail(f'{where}: {name} has a row already, at {seen[name, other]}')
            seen[name, other] = where
        if given in (EVERY, host):
            found[name] = Row(kind, given, where)
    return found


def heap_place():
    """heap_arena's address and room: twice the room 1ST_READ.BIN's tables
    give it, to the next name they place, ending at MODULE_AREA in the view
    they give it"""
    places = {name: address for address, name in named(FIRST / 'symbols.txt')}
    at = places[HEAP]
    room = 2 * (min(address for address in places.values() if address > at) - at)
    return at - physical(at) + ((macros(LOADER_H)['MODULE_AREA'] - room) & ~(HEAP_ALIGNMENT - 1)), room


def places(missing):
    """the address and room of each of missing, names no object defines or
    calls, that the tables place: heap_arena, and a name 1ST_READ.BIN
    imports from an image the slice doesn't link, at that image's address
    for it, with the image's load area"""
    found = {}
    if HEAP in missing:
        found[HEAP] = heap_place()
    handed = [(name, image, address) for name, image, _, address, _ in imports() if name in missing]
    if handed:
        lengths = contract(images()).lengths
        found.update({name: (address, lengths[image]) for name, image, address in handed})
    return found


def symbols(args):
    run = subprocess.run([os.environ.get('NM', 'nm'), *args], capture_output=True, text=True, check=True)
    return run.stdout.splitlines()


def main(out, *args):
    defined, used = set(), set()
    explore = '--explore' in args
    args = tuple(a for a in args if a != '--explore')
    if '--aliases' in args:
        at = args.index('--aliases')
        defined |= {line.split()[0] for line in Path(args[at + 1]).read_text().splitlines() if line.strip()}
        args = args[:at] + args[at + 2:]
    libraries = [LIBC, LIBM]
    if '--libraries' in args:
        at = args.index('--libraries')
        end = next((i for i in range(at + 1, len(args)) if args[i].startswith('--')), len(args))
        libraries, args = list(args[at + 1:end]), args[:at] + args[end:]
    base = 0
    if '--base' in args:
        at = args.index('--base')
        base = int(args[at + 1], 0)
        args = args[:at] + args[at + 2:]
    objects = args
    for line in symbols(['-A', *objects]):
        parts = line.split()
        if len(parts) >= 3 and parts[-2] not in 'Uw':
            defined.add(parts[-1])
        elif parts[-2] == 'U':
            used.add(parts[-1])
    host, host_code = set(), set()
    # an archive's weak name is an alias the library gives, as COFF's
    # strdup is the UCRT's _strdup. An import's __imp_ name is the word
    # that holds its address, which llvm-nm types as code
    for library in libraries:
        shared = '.so' in Path(library).name
        for line in symbols(['-D', '--defined-only', library] if shared else [library]):
            fields = line.split()
            if len(fields) >= 2 and fields[-2] != 'U':
                name = fields[-1].split('@')[0]
                host.add(name)
                if fields[-2] in 'TtWw' and not name.startswith('__imp_'):
                    host_code.add(name)
    called = set()
    coff = Path(objects[0]).read_bytes()[:4] != b'\x7fELF'
    if coff:
        for obj in objects:
            called |= calls(obj)
    else:
        for obj in objects:
            run = subprocess.run(['readelf', '-rW', obj], capture_output=True, text=True, check=True)
            called.update(m[1] for m in re.finditer(r'R_(?:X86_64_PLT32|386_PLT32|386_PC32)\s+\S+\s+(\w+)',
                                                    run.stdout))
    ends = {WORK[image][0]: Config(image).span()[1] for image in WORK}
    # a name the slice reads where COFF's library gives code is the game's
    # own, as SHC's _errno is a word and the UCRT's a function
    if coff:
        host -= host_code - called
    missing = sorted(n for n in used - defined - host - LINKER if not n.startswith('__real_'))
    placed = places([name for name in missing if name not in called])
    use = {name: ADDRESS if name in ends or name in placed else FUNCTION if name in called else ZEROS
           for name in missing}
    # the slice's objects are COFF for Windows alone
    target = HOSTS[coff]
    allowed = rows(TABLE, target)
    said = {FUNCTION: 'an object calls it', ZEROS: 'no object calls it and the tables place none of it',
            ADDRESS: 'the tables place it'}
    fits = {name: ' or '.join(kind for kind in KINDS if use[name] in FITS[kind]) for name in missing}
    problems = [f'{name}: the objects need it and none defines it, and {TABLE} has no row for it on {target}: '
                f'define it, or give it a row, {name} KIND HOST REASON, of kind {fits[name]}' for name in missing
                if name not in allowed]
    problems += [f'{row.where}: {name}: its row gives {row.kind}, and {said[use[name]]}: its kind is {fits[name]}'
                 for name, row in allowed.items() if name in use and use[name] not in FITS[row.kind]]
    problems += [f'{row.where}: {name}: no object needs it, or one defines it: remove the row'
                 for name, row in allowed.items() if name not in use]
    if problems and not explore:
        fail(*problems)
    lines = ['/* the native slice\'s stubs, as engine/tools/stubs.py writes them */',
             'void native_stub(const char *name);',
             '#define ZERO_FLOAT __asm__ volatile("xorps %%xmm0, %%xmm0" ::: "xmm0")', '']
    written = dict.fromkeys(KINDS, 0)
    for name in missing:
        # a row whose kind doesn't fit its name's use is one of problems, and the use decides what --explore writes
        kind = allowed[name].kind if name in allowed and use[name] in FITS[allowed[name].kind] else use[name]
        written[kind] += 1
        if kind == ENGINE:
            continue
        if name in ends:
            lines.append(f'void *{name} = (void *){base + ends[name]:#x}ULL;')
        elif name in placed:
            at, room = placed[name]
            # COFF types and sizes no symbol
            typed = '' if coff else f'.type {name}, @object\\n.size {name}, {room:#x}\\n'
            lines.append(f'__asm__(".globl {name}\\n{typed}.set {name}, {base + at:#x}");')
        elif use[name] == FUNCTION:
            lines.append(f'long {name}(void) {{ native_stub("{name}"); ZERO_FLOAT; return 0; }}')
        else:
            lines.append(f'char {name}[{DATA_BYTES}] __attribute__((aligned(64)));')
    Path(out).write_text('\n'.join(lines) + '\n')
    for problem in problems:
        print(f'stubs: {problem}')
    print(f'{out}: {written[FUNCTION]} functions, {written[ZEROS]} of zeros, {written[ADDRESS]} placed, '
          f'{written[ENGINE]} the engine\'s, {written[UNSUPPORTED]} unsupported' +
          f', explored: {len(problems)} that {TABLE} doesn\'t allow' * bool(problems))
    return 0


if __name__ == '__main__':
    sys.exit(main(*sys.argv[1:]))
