#!/usr/bin/env python3
"""Which of each program's names the native slice's link keeps global.

programs.py [--explore] DIR PROGRAM... -- REST... reads each program's
partial link, DIR/program-PROGRAM.r, and the objects the slice links beside
them, by the environment's NM, or else nm, and writes DIR/keep-PROGRAM.txt
for each: the names the program defines that another object or program
reads. A program loads where others do, so two may define one name, each at
the same address. The one a reader binds to is the one make dc's link binds
it to (tools/spec.py): the provider a reviewed row gives the reader,
config/1st_read/imports.txt's for the rest and config/imports.txt's for a
program, or else the one definer the memory contract has live with the
reader. Every other definition stays the program's own. The slice links one
definition of a name, so programs.py fails, and writes nothing, on a name
some reader takes from no program by those rules, and on one two readers
take from two programs. Given --explore it says each such name and gives it
to the program whose names its readers read most, so a build reaches its
next missing name: engine/mk/native.mk's NATIVE_EXPLORE. DIR/ambiguous.txt
lists each name two programs define, its definers and the one its readers
take, and DIR/programs.c the table loads.c reads. A name only the rest reads
and no import names stays each program's own, and the rest takes the host's
or a stub that engine/native/stubs.txt allows. A name an image takes under
another, as the imports give it, is renamed in DIR/rename-PROGRAM.txt, and
1ST_READ.BIN's own second names are DIR/aliases.txt's, for the link."""
import collections
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from images import images  # noqa: E402
from memory import contract  # noqa: E402

# the rest of the slice, as a reader: 1ST_READ.BIN's objects and the host's
FIRST = '1st_read'
# the image of a row of config/imports.txt for every image
EVERY = '*'


def symbols(paths):
    """each path's defined globals and its undefined names"""
    defined, undefined = collections.defaultdict(set), collections.defaultdict(set)
    out = subprocess.run([os.environ.get('NM', 'nm'), '-A', '-g', *map(str, paths)], capture_output=True, text=True,
                         check=True).stdout
    weak = []
    for line in out.splitlines():
        where, _, rest = line.rpartition(':')
        fields = rest.split()
        if len(fields) == 2 and fields[0] in 'Uw':
            undefined[where].add(fields[1])
        elif len(fields) == 3 and fields[1] not in 'Uw':
            defined[where].add(fields[2])
            # COFF's weak read of a name falls back to an absolute 0 by
            # another name, so llvm-nm types the read itself a definition
            if fields[1] == 'A' and (read := re.fullmatch(r'\.weak\.(.+)\.default\..+', fields[2])):
                weak.append((where, read[1]))
    for where, name in weak:
        defined[where].discard(name)
        undefined[where].add(name)
    return defined, undefined


def colive():
    """the images each image may take names from, as the memory contract
    gives them, read from the working directory's tree"""
    return contract(images()).colive


def bound(reader, name, by, imports, reviewed, live):
    """the one of by, the programs that define name, that reader takes it
    from: the provider reader's reviewed row gives, or else the one of by
    that is live with reader, as live() gives the images each may take names
    from. None where neither names one"""
    row = imports.get(name) if reader == FIRST else reviewed.get((reader, name), reviewed.get((EVERY, name)))
    if row in by:
        return row
    near = [program for program in by if program in live()[reader]]
    return near[0] if len(near) == 1 else None


def main():
    explore = '--explore' in sys.argv[1:sys.argv.index('--')]
    args = [a for a in sys.argv[1:] if a != '--explore']
    split = args.index('--')
    out, programs, rest = Path(args[0]), args[1:split], args[split + 1:]
    files = {p: str(out / f'program-{p}.r') for p in programs}
    defined, undefined = symbols([*files.values(), *rest])
    rest_defined = set().union(*(defined[r] for r in rest))
    rest_read = set().union(*(undefined[r] for r in rest)) - rest_defined
    definers = collections.defaultdict(list)
    for p, f in files.items():
        for name in defined[f] - rest_defined:
            definers[name].append(p)
    readers = collections.defaultdict(set)
    for p, f in files.items():
        for name in undefined[f]:
            readers[name].add(p)
    imports, renamed = {}, collections.defaultdict(list)
    keep = collections.defaultdict(set)
    for line in (ROOT / 'config/1st_read/imports.txt').read_text().splitlines():
        fields = line.split()
        if fields and not line.startswith('#'):
            imports[fields[0]] = fields[1]
            # 1ST_READ.BIN's name for an image's symbol of another name
            if len(fields) > 2 and fields[1] in files:
                renamed[fields[1]].append((fields[2], fields[0]))
                keep[fields[1]].add(fields[0])
    # a second name an image takes from 1ST_READ.BIN or another image, as
    # config/imports.txt's rows give it, and the provider a row gives an
    # image for a name it takes under the provider's own
    aliases, reviewed = [], {}
    for line in (ROOT / 'config/imports.txt').read_text().splitlines():
        fields = line.split()
        if len(fields) < 4 or line.startswith('#'):
            continue
        image, name, provider, symbol = fields[:4]
        if name == symbol:
            reviewed[image, name] = provider
            continue
        if image in files and (provider in files or symbol in rest_defined):
            renamed[image].append((name, symbol))
            keep[provider].add(symbol)
        elif image == '1st_read' and symbol in rest_defined:
            aliases.append((name, symbol))
    ambiguous, unbound, live = [], [], []

    def lives():
        # the memory contract, read once, and only where a row names no provider
        if not live:
            live.append(colive())
        return live[0]

    for name, by in definers.items():
        who = readers[name] | ({FIRST} if name in rest_read else set())
        if not who:
            continue
        if len(by) == 1:
            keep[by[0]].add(name)
            continue
        takes = {reader: bound(reader, name, by, imports, reviewed, lives) for reader in sorted(who)}
        if who == {FIRST} and takes[FIRST] is None:
            # no import names it: the rest takes the host's, or a stub
            ambiguous.append(f'{name} {",".join(sorted(by))} read by 1st_read: none')
            continue
        chosen = set(takes.values())
        if None in chosen or len(chosen) > 1:
            said = ', '.join(f'{reader} from {program or "none of them"}' for reader, program in takes.items())
            unbound.append(f'{name}: {", ".join(sorted(by))} define it, and its readers take it {said}. The slice '
                           'links one definition of a name, which each reader takes by its reviewed row, of '
                           'config/1st_read/imports.txt for 1st_read and of config/imports.txt for a program, or as '
                           'the one definer the memory contract has live with it: give the reader a row')
            votes = collections.Counter(d for r in who - {FIRST} for n in undefined[files[r]]
                                        for d in definers.get(n, ()) if len(definers[n]) == 1)
            chosen = {max(by, key=lambda d: votes[d])}
        chosen = chosen.pop()
        keep[chosen].add(name)
        ambiguous.append(f'{name} {",".join(sorted(by))} read by {",".join(sorted(who))}: {chosen}')
    for problem in unbound:
        print(f'programs: {problem}', file=sys.stderr)
    if unbound and not explore:
        sys.exit(f'programs: {len(unbound)} names that no rule binds to one program')
    for p in programs:
        # objcopy keeps every global given an empty list, so a name no
        # program defines stands in for none
        (out / f'keep-{p}.txt').write_text(''.join(f'{n}\n' for n in sorted(keep[p] or {'native_none'})))
        (out / f'rename-{p}.txt').write_text(''.join(f'{a} {b}\n' for a, b in renamed[p]))
    (out / 'ambiguous.txt').write_text(''.join(f'{line}\n' for line in sorted(ambiguous)))
    table = ['#include "native.h"\n']
    coff = Path(files[programs[0]]).read_bytes()[:4] != b'\x7fELF'
    for p in programs:
        for s in 'data', 'bss':
            if coff:
                # PE names no section's bounds: a program's sorts between
                # two marks, the data's initialized as its section is
                for mark, at in ('start', 'a'), ('stop', 'z'):
                    table.append(f'__attribute__((section(".{s}$sa1_{p}${at}"))) char __{mark}_sa1_{s}_{p}[1]'
                                 f'{" = {1}" if s == "data" else ""};\n')
                continue
            table.append(f'extern char __start_sa1_{s}_{p}[] __attribute__((weak)), '
                         f'__stop_sa1_{s}_{p}[] __attribute__((weak));\n')
    table.append('const struct native_program native_programs[] = {\n')
    for p in programs:
        table.append(f'    {{"{p.upper()}.PRS", __start_sa1_data_{p}, __stop_sa1_data_{p}, __start_sa1_bss_{p}, '
                     f'__stop_sa1_bss_{p}}},\n')
    table.append('};\nconst int native_program_count = sizeof native_programs / sizeof *native_programs;\n')
    (out / 'programs.c').write_text(''.join(table))
    (out / 'aliases.txt').write_text(''.join(f'{name} {symbol}\n' for name, symbol in aliases))


if __name__ == '__main__':
    main()
