"""Link make pc's images for x86, each at its area, by make gcc's spec.

Usage: pclink.py SPEC OUT [--drop PATTERN]... [--add IMAGE=OBJECT]...
                  [--skip IMAGE]... [--stubs IMAGE]...

SPEC is make gcc's spec, build/gcc/link/spec.txt, in tools/gcclink.py's
rows. For each image, pclink.py links make pc's objects in its input rows,
build/gcc/ moved to build/pc/, with ld.lld for i386, to OUT/IMAGE/final.elf
and its bytes from the origin on to OUT/IMAGE/final.bin, which the port's
host loads as the console's loader reads the image's file.

- RAM: the console reaches its 16 MiB at both 0x0c000000 and 0x8c000000.
  The port has one view, at 0x0c000000, so an origin or a constant at
  0x8c000000 on links at 0x0c000000 on.
- Sections: the script lays the head rows' input sections first, as each
  loader reads a record at the image's origin, then .rodata, .data and
  .bss, each object's in link order, then the tail rows'. x86's code is up
  to twice the SH-4's, whose 16-bit instructions the areas were cut to, so
  .text links apart, in a code area of its own past CODE_BASE. Images of
  one origin share an area, as the console's do, and never live together,
  so they share a code area, as long as the longest of their code. No two
  origins' code areas overlap. So the data keeps every address the C and
  the memory contract fix, and an image's size is its data's. A head or
  tail row of an SHC section, which no make pc object holds, is skipped.
  Kernel rows, which order code for the SH-4's cache, are skipped.
- Imports: the first pass links every image, its code anywhere, to
  measure its code. The second lays each out, its code in its group's
  code area, with its imports unbound. The third binds each import, alias
  and constant, as the spec's rows give them, by the second's addresses.
  On x86 a value changes no instruction's length, so the third pass keeps
  the second's layout.
- An object the spec names that make pc doesn't build, as a .src file's
  or a source that still fails, stays out, and what it defines stays
  unbound. make gcc's SH-4 libraries stay out: the port runs none of them.
  XMTRX, the console's one register, is the port layer's, in 1st_read,
  which the other images bind to. config/pc.mk's PC_ADDED objects link
  where their sources do.

- The port layer: --drop leaves out each make gcc input whose source
  matches PATTERN, as fnmatch reads it, as sdk/shinobi/*, which the port
  layer replaces; --add links OBJECT into IMAGE after its inputs, as the
  port layer's objects into 1st_read; --skip leaves IMAGE out, as loader.
- Stubs: for each --stubs IMAGE, each name IMAGE's inputs take and nothing
  defines or binds takes a stub: a function, where make gcc's definition is
  code, that has port_unimplemented(name) log it and returns 0, or else
  storage of the definition's size, 32-byte aligned. OUT/stubs/IMAGE.s
  holds them, which clang assembles. 1st_read's stubs take, too, each
  name another image leaves free.
- Base: a name an image leaves unbound once the spec's rows are bound
  binds to 1st_read's definition, if it has one, as the port layer's and
  its stubs.

It writes OUT/report.txt: each image's size against its area, each
anchor the final link moves from its offset, and each image's unbound
names, and prints the report's summary. It fails where ld.lld does.
"""

import collections
import concurrent.futures
import re
import subprocess
import sys
from pathlib import Path

VIEW_BIT = 0x80000000
# where the code areas start, past the console's RAM, and their alignment
CODE_BASE = 0x10000000
CODE_ALIGN = 0x10000
# the image every other runs beside, which holds the SDK and the port layer
BASE = '1st_read'


def x86_address(value):
    """an address of the console's RAM in the port's one view"""
    return value & ~VIEW_BIT if 0x8c000000 <= value < 0x8d000000 else value


def read_spec(path):
    """the spec's rows, by image"""
    images, rows = {}, collections.defaultdict(list)
    for line in Path(path).read_text().splitlines():
        line = line.split('#')[0].strip()
        if not line:
            continue
        fields = line.split()
        if fields[0] == 'image':
            images[fields[1]] = (int(fields[2], 16), int(fields[3], 16), fields[4] if len(fields) > 4 else None)
        else:
            rows[fields[1]].append(fields)
    return images, rows


def added_objects():
    """config/pc.mk's PC_ADDED, as {source object: added object}"""
    match = re.search(r'^PC_ADDED := (.*)$', Path('config/pc.mk').read_text(), re.M)
    found = {}
    for entry in (match[1].split() if match else []):
        source, _, extra = entry.partition(':')
        found[Path('build/pc') / Path(source).with_suffix('.o')] = Path('build/pc') / Path(extra).relative_to(
            'xbox').with_suffix('.o')
    return found


def objects(rows, added):
    """an image's make pc objects, in link order, and the spec's that make
    pc lacks"""
    found, missing = [], []
    for row in rows:
        if row[0] != 'input':
            continue
        path = row[2]
        # make gcc's SH-4 libraries (libgcc.a and its math library) have no
        # make pc objects: the x86 runtime's helpers come with the port's host
        if '/sh4zam/' in path or not path.startswith('build/gcc/'):
            continue
        pc = Path(path.replace('build/gcc/', 'build/pc/', 1))
        if pc.exists():
            found.append(pc)
            if pc in added:
                found.append(added[pc])
        else:
            missing.append(pc)
    return found, missing


def script(origin, code, rows, inputs, bindings):
    """the image's linker script"""
    pc = {str(p) for p in inputs}
    heads = [row for row in rows if row[0] == 'head']
    tails = [row for row in rows if row[0] == 'tail']

    def place(row):
        path = row[3].replace('build/gcc/', 'build/pc/', 1)
        section = row[4]
        if path not in pc or not section.startswith('.'):
            return None
        return f'    KEEP({path}({section}))'

    lines = [f'/* written by tools/pclink.py */', 'ENTRY(image_origin)', 'SECTIONS', '{',
             f'  . = 0x{origin:08x};', '  image_origin = .;', '  .head : {']
    lines += [x for x in map(place, heads) if x]
    lines += ['  }', '  .rodata : { *(.rodata .rodata.*) }',
              '  .data : { *(.data .data.*) }', '  .bss : { *(.bss .bss.* COMMON) }', '  .tail : {']
    lines += [x for x in map(place, tails) if x]
    lines += ['  }', '  image_end = .;', f'  . = 0x{code:08x};', '  image_code = .;',
              '  .text : { *(.text .text.*) }', '  image_code_end = .;',
              '  /DISCARD/ : { *(.comment) *(.note .note.*) }', '}']
    lines += [f'{name} = 0x{value:08x};' for name, value in sorted(bindings.items())]
    return '\n'.join(lines) + '\n'


def link(image, origin, code, rows, inputs, bindings, out):
    """link an image: its ELF, or ld.lld's complaint"""
    directory = out / image
    directory.mkdir(parents=True, exist_ok=True)
    (directory / 'final.ld').write_text(script(origin, code, rows, inputs, bindings))
    elf = directory / 'final.elf'
    result = subprocess.run(['ld.lld', '-m', 'elf_i386', '-static', '--no-relax', '-nostdlib',
                             '--unresolved-symbols=ignore-all', '-T', str(directory / 'final.ld'),
                             '-o', str(elf), *map(str, inputs)], capture_output=True, text=True)
    if result.returncode != 0:
        return image, None, result.stderr
    return image, elf, result.stderr


def symbols(elf):
    """an ELF's defined symbols, by name"""
    out = subprocess.run(['llvm-nm', '--defined-only', str(elf)], capture_output=True, text=True).stdout
    found = {}
    for line in out.splitlines():
        fields = line.split()
        if len(fields) == 3:
            found[fields[2]] = int(fields[0], 16)
    return found


def undefined(inputs):
    """the names the inputs take from outside"""
    if not inputs:
        return set()
    out = subprocess.run(['llvm-nm', '-u', *map(str, inputs)], capture_output=True, text=True).stdout
    return {line.split()[-1] for line in out.splitlines() if line.strip() and not line.endswith(':')}


def gcc_kinds(rows):
    """each name make gcc's inputs define, as (code, size), from sh-elf-nm"""
    paths = sorted({row[2] for image_rows in rows.values() for row in image_rows
                    if row[0] == 'input' and row[2].startswith('build/gcc/') and Path(row[2]).exists()})
    found = {}
    for start in range(0, len(paths), 400):
        out = subprocess.run(['tools/sh-elf/bin/sh-elf-nm', '--defined-only', '-S', *paths[start:start + 400]],
                             capture_output=True, text=True).stdout
        for line in out.splitlines():
            fields = line.split()
            if len(fields) == 4:
                name = fields[3][1:] if fields[3].startswith('_') else fields[3]
                found.setdefault(name, (fields[2] in 'Tt', int(fields[1], 16)))
    return found


def write_stubs(image, names, kinds, out):
    """an image's stubs, assembled: OUT/stubs/IMAGE.o"""
    directory = out / 'stubs'
    directory.mkdir(parents=True, exist_ok=True)
    lines = ['# written by tools/pclink.py: the names nothing defines yet', '    .text']
    data = []
    for k, name in enumerate(sorted(names)):
        code, size = kinds.get(name, (True, 0))
        if code:
            lines += [f'    .globl {name}', f'{name}:', f'    pushl $stub_name_{k}', '    call port_unimplemented',
                      '    addl $4, %esp', '    xorl %eax, %eax', '    ret']
            data += [f'stub_name_{k}: .asciz "{name}"']
        else:
            lines += [f'    .globl {name}', f'    .comm {name}, {max(size, 4)}, 32']
    lines += ['    .section .rodata'] + data
    source = directory / f'{image}.s'
    source.write_text('\n'.join(lines) + '\n')
    obj = directory / f'{image}.o'
    subprocess.run(['clang', '--target=i386-elf', '-c', '-o', str(obj), str(source)], check=True)
    return obj


def main():
    import argparse
    import fnmatch
    parser = argparse.ArgumentParser()
    parser.add_argument('spec')
    parser.add_argument('out')
    parser.add_argument('--drop', action='append', default=[])
    parser.add_argument('--add', action='append', default=[])
    parser.add_argument('--skip', action='append', default=[])
    parser.add_argument('--stubs', action='append', default=[])
    args = parser.parse_args()
    images, rows = read_spec(args.spec)
    for image in args.skip:
        images.pop(image, None)
    for image in images:
        rows[image] = [row for row in rows[image] if not (
            row[0] == 'input' and any(fnmatch.fnmatch(row[2].replace('build/gcc/', '', 1), pattern.replace('.c', '.o'))
                                      for pattern in args.drop))]
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    added = added_objects()
    inputs, missing = {}, {}
    for image in images:
        inputs[image], missing[image] = objects(rows[image], added)
    for entry in args.add:
        image, _, obj = entry.partition('=')
        inputs[image].append(Path(obj))
    def defined_by(image):
        found = set()
        out_nm = subprocess.run(['llvm-nm', '--defined-only', *map(str, inputs[image])], capture_output=True,
                                text=True).stdout
        for line in out_nm.splitlines():
            fields = line.split()
            if len(fields) == 3:
                found.add(fields[2])
        return found

    def free_names(image, defined):
        bound = {row[2] for row in rows[image] if row[0] in ('import', 'alias', 'constant')}
        return undefined(inputs[image]) - defined - bound - {'port_unimplemented'}

    if args.stubs:
        kinds = gcc_kinds(rows)
        with concurrent.futures.ThreadPoolExecutor(16) as pool:
            others = {image: pool.submit(lambda i: free_names(i, defined_by(i)), image) for image in images}
            others = {image: job.result() for image, job in others.items()}
        for image in args.stubs:
            defined = defined_by(image)
            free = free_names(image, defined)
            # and each name another image imports from this one that its
            # inputs no longer define, as a dropped source's
            for other in images:
                free |= {row[4] for row in rows[other]
                         if row[0] == 'import' and row[3] == image and row[4] not in defined}
            # and, for 1st_read, each name an overlay leaves free, which
            # BASE binds below
            if image == BASE:
                for other, names in others.items():
                    if other != BASE:
                        free |= names - defined
            inputs[image].append(write_stubs(image, free, kinds, out))

    def run_all(bindings, codes):
        with concurrent.futures.ThreadPoolExecutor(16) as pool:
            jobs = [pool.submit(link, image, x86_address(images[image][0]), codes[image], rows[image],
                                inputs[image], bindings.get(image, {}), out) for image in images]
            results = {}
            for job in jobs:
                image, elf, errors = job.result()
                if elf is None:
                    raise SystemExit(f'pclink.py: {image}: ld.lld failed:\n{errors}')
                results[image] = elf
            return results

    # the code areas: each origin's as long as its longest code, in origin
    # order, past CODE_BASE
    measured = run_all({}, {image: CODE_BASE for image in images})
    longest = collections.defaultdict(int)
    for image, elf in measured.items():
        names = symbols(elf)
        origin = x86_address(images[image][0])
        longest[origin] = max(longest[origin], names['image_code_end'] - names['image_code'])
    areas, at = {}, CODE_BASE
    for origin in sorted(longest):
        areas[origin] = at
        at = (at + longest[origin] + CODE_ALIGN - 1) // CODE_ALIGN * CODE_ALIGN
    codes = {image: areas[x86_address(images[image][0])] for image in images}
    (out / 'code_areas.txt').write_text(''.join(
        f'origin 0x{origin:08x} code 0x{base:08x} length {longest[origin]}\n' for origin, base in sorted(areas.items()))
        + f'end 0x{at:08x}\n')
    first = run_all({}, codes)
    defined = {image: symbols(elf) for image, elf in first.items()}
    bindings = collections.defaultdict(dict)
    unbound_imports = collections.defaultdict(list)
    for image in images:
        for row in rows[image]:
            if row[0] == 'import':
                alias, provider, symbol = row[2], row[3], row[4]
                if symbol in defined.get(provider, {}):
                    bindings[image][alias] = defined[provider][symbol]
                else:
                    unbound_imports[image].append(alias)
            elif row[0] == 'alias' and row[3] in defined[image]:
                bindings[image][row[2]] = defined[image][row[3]]
            elif row[0] == 'constant':
                bindings[image][row[2]] = x86_address(int(row[3], 16))
    # an overlay's name that nothing binds binds to 1ST_READ.BIN's, as the
    # port layer's and the SDK's, which the console's overlays find there
    # through the spec's imports, or would, had the SDK the port's names
    if BASE in defined:
        with concurrent.futures.ThreadPoolExecutor(16) as pool:
            jobs = {image: pool.submit(undefined, inputs[image]) for image in images if image != BASE}
            for image, job in jobs.items():
                for name in job.result() - set(defined[image]) - set(bindings[image]):
                    if name in defined[BASE]:
                        bindings[image][name] = defined[BASE][name]
    final = run_all(bindings, codes)
    report, over, moved, total_unbound = [], [], [], 0
    for image, elf in final.items():
        names = symbols(elf)
        origin, length, _ = images[image]
        origin = x86_address(origin)
        size = names['image_end'] - origin
        if size > length:
            over.append(image)
        report.append(f'image {image} origin 0x{origin:08x} size {size} area {length}'
                      f'{" OVER" if size > length else ""} objects {len(inputs[image])} missing {len(missing[image])}')
        for row in rows[image]:
            if row[0] == 'anchor':
                symbol, offset = row[2], int(row[3], 0)
                if symbol in names and names[symbol] != origin + offset:
                    moved.append(f'{image}:{symbol}')
                    report.append(f'  anchor {symbol} at +0x{names[symbol] - origin:x}, not +0x{offset:x}')
        free = sorted(undefined(inputs[image]) - set(names) - set(bindings[image]))
        total_unbound += len(free)
        report.append(f'  unbound {len(free)}: {" ".join(free[:40])}{" ..." if len(free) > 40 else ""}')
    summary = (f'{len(final)} images linked; {len(over)} over their areas ({", ".join(over)}); '
               f'{len(moved)} anchors moved; {total_unbound} unbound names; '
               f'{sum(len(v) for v in unbound_imports.values())} imports whose provider lacks the symbol')
    (out / 'report.txt').write_text(summary + '\n' + '\n'.join(report) + '\n')
    print(summary)
    if over:
        # because an image past its area overlaps the next one's memory (1st_read's
        # data over its area hung the game at its first sound bank), it fails the build
        sys.exit(f'pclink.py: {", ".join(over)} past its area: see {out / "report.txt"}')


if __name__ == '__main__':
    main()
