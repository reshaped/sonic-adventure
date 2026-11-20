#!/usr/bin/env python3
"""Link the PSP slice: the program, and each program the loader reads.

psplink.py partial OUT PROGRAM OBJECT... links a program's objects into one,
OUT, as engine/mk/psp.mk's rule for it asks. A record the program's loader
reads at a fixed offset, as tools/memory.py's records() gives them, takes a
section of its own, .head, which the program's link places first.

psplink.py link DIR PLACED PROGRAM... -- WORD... links the program, DIR's
main.elf, by the command WORDs give, and each PROGRAM, from DIR's
program-PROGRAM.o, and writes what a run reads beside the program
(engine/psp/names.h): DIR/game/names.bin and DIR/game/programs/PROGRAM.BIN.

The PSP has no memory at the console's addresses. The program holds the
console's RAM past 1ST_READ.BIN as console_ram (engine/psp/psp.h), and a
program links where the memory contract places it there, so the loader's
read lands on it as on the console. The system loads the program where it
chooses, in practice at BASE, and moves each address the program's file
holds. A program's file holds no such record, so its link fixes its
addresses for a program at BASE, and loads.c refuses it at another.

A program's link leaves out each section that nothing reaches from a name
the program gives another, or from a record its loader reads: programs.py
keeps global the names something else reads, and no other.

The program reads names of the programs' and they read the program's, so
each links twice. The first pass binds each name an image lacks to a
placeholder and gives the image its layout, which no name's value changes.
The second binds each name: the program's to a program's by a label in console_ram's own assembly,
which the system moves with the program, and a program's to the program's
or another's by its address. link fails where a pass moves a name, where a
program outgrows the room the contract gives it, and where a name stays
unbound.

PLACED is stubs.py's --placed file: the names the console's tables place,
which lie at the same bytes of console_ram, and the words that hold an
image's end, which hold the PSP's image's.

The environment's PSPDEV names the toolchain, ~/pspdev by default."""
import os
import re
import struct
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from images import images  # noqa: E402
from memory import FISH_IMAGE, FISH_MODELS, WORK, Extent, check, contract, records  # noqa: E402

BIN = Path(os.environ.get('PSPDEV', Path.home() / 'pspdev')) / 'bin'
# where the system loads a program's first byte, as every firmware has for a
# game's own
BASE = 0x08804000
# console_ram's first byte and its length, as engine/psp/psp.h has them
RAM_FIRST = 0x00900000
RAM_SIZE = 0x00600000
NAMES_MAGIC = 0x4d4e4153
PROGRAM_MAGIC = 0x52504153
ALIGNMENT = 32
# the objects the adapter and the shim find by name: records.c's statics,
# and an event's head
STATICS = re.compile(r'static const char \*const static_names\[STATICS\] = \{(.*?)\};', re.S)
HEAD = re.compile(r'\w+_head')
SCRIPT = '''SECTIONS
{{
  . = {origin:#x};
  .head : {{ KEEP(*(.head)) }}
  .data : {{ *(.data .data.* .sdata .sdata.*) }}
  .rodata : {{ *(.rodata .rodata.*) }}
  .text : {{ *(.text .text.*) }}
  __text_end = .;
  .bss (NOLOAD) : {{ *(.sbss .sbss.* .scommon .bss .bss.* COMMON) }}
  . = ALIGN({alignment});
  __body_end = .;
  .tail : {{ *(.tail .tail.*) }}
  __image_end = SIZEOF(.tail) ? . : __text_end;
  . = ALIGN({alignment});
  __program_end = .;
  /DISCARD/ : {{ *(.reginfo) *(.MIPS.abiflags) *(.pdr) *(.mdebug*) *(.gnu.attributes) *(.comment) *(.eh_frame)
                *(.gcc_compiled_long32) *(.note*) }}
}}
'''


def fail(words):
    sys.exit(f'psplink: {words}')


def run(*words, check=True):
    done = subprocess.run([str(w) for w in words], capture_output=True, text=True)
    if check and done.returncode:
        fail(f'{Path(str(words[0])).name} failed:\n{done.stderr.strip()[-4000:]}')
    return done


def tool(name):
    return BIN / f'psp-{name}'


def console_offset(address):
    """the byte of console_ram that a console address names"""
    offset = (address & 0x00ffffff) - RAM_FIRST
    if not 0 <= offset <= RAM_SIZE:
        fail(f'{address:#x}: an address outside the RAM console_ram holds')
    return offset


def partial(out, program, objects):
    contracted = contract(images())
    heads = {(Path(source).with_suffix('.o').as_posix(), section)
             for _, source, section, _, offset, _ in records(program, contracted.events)}
    # the image's tail, the sections its link places past its bss: the game
    # loads other files over them once it has done with them
    tail = Path(FISH_MODELS).with_suffix('.o').as_posix() if program == FISH_IMAGE else None
    inputs = []
    for obj in objects:
        named = [section for path, section in heads if obj.endswith('/' + path)]
        if named:
            head = Path(out).with_name(f'head-{program}.o')
            run(tool('objcopy'), '--rename-section', f'{named[0]}=.head', obj, head)
            obj = head
        if tail and obj.endswith('/' + tail):
            # its code and data alone: MIPS's own sections keep their names
            last = Path(out).with_name(f'tail-{program}.o')
            listed = run(tool('objdump'), '-h', obj).stdout.split()
            names = sorted({word for word in listed
                            if word.startswith(('.data', '.rodata', '.text', '.sdata', '.bss', '.sbss'))})
            run(tool('objcopy'), *[f'--rename-section={name}=.tail{name}' for name in names], obj, last)
            obj = last
        inputs.append(obj)
    run(tool('ld'), '-r', '-d', '-o', out, *inputs)


def table(elf):
    """an ELF's symbols: (name, address, size, type, binding, section) each,
    in the table's order, its files' names among them"""
    rows = []
    for line in run(tool('readelf'), '-sW', elf).stdout.splitlines():
        fields = line.split()
        if len(fields) >= 8 and fields[0].rstrip(':').isdigit():
            size = int(fields[2], 0)
            rows.append((fields[7], int(fields[1], 16), size, fields[3], fields[4], fields[6]))
    return rows


def globals_of(rows):
    return {name: address for name, address, _, kind, binding, section in rows
            if binding in ('GLOBAL', 'WEAK') and section != 'UND' and kind in ('FUNC', 'OBJECT', 'NOTYPE')}


def undefined(rows):
    return {name for name, _, _, _, _, section in rows if section == 'UND' and name}


def names_table(rows, wanted, moved, console):
    """a table of names (engine/psp/names.h): each object of rows that
    wanted names, with task.c's objects first, as symbols.c adds them. the
    PSP has no memory for a name of each function, and its program names one
    by where it lies (engine/psp/symbols.c). an address goes down by moved"""
    kept, in_task = [], False
    for name, address, size, kind, _, section in rows:
        if kind == 'FILE':
            in_task = name == 'task.c'
        elif section != 'UND' and address and kind == 'OBJECT' and wanted(name):
            kept.append((not (in_task and kind == 'OBJECT'), address - moved, size, kind == 'FUNC', name))
    kept.sort(key=lambda row: row[0])
    text, places = bytearray(), {}
    out = bytearray()
    for _, address, size, code, name in kept:
        if name not in places:
            places[name] = len(text)
            text += name.encode() + b'\0'
        out += struct.pack('<4I', address, size, int(code), places[name])
    return struct.pack('<4I', NAMES_MAGIC, len(kept), len(text), console) + bytes(out) + bytes(text)


def console_source(labels, words):
    lines = ['    .section .bss.console, "aw", @nobits', '    .globl console_ram', '    .type console_ram, @object',
             '    .balign 64', 'console_ram:', f'    .space {RAM_SIZE:#x}', f'    .size console_ram, {RAM_SIZE:#x}']
    # the zeros that stand for the console's registers and its store queues
    # (engine/psp/psp.h)
    for name, size, alignment in ('console_registers', 0x10000, 64), ('console_queues', 0x20000, 0x10000):
        lines += [f'    .globl {name}', f'    .type {name}, @object', f'    .balign {alignment}', f'{name}:',
                  f'    .space {size:#x}', f'    .size {name}, {size:#x}']
    for name, offset in sorted(labels.items()):
        lines += [f'    .globl {name}', f'    .set {name}, console_ram + {offset:#x}']
    if words:
        lines.append('    .data')
    for name, offset in sorted(words.items()):
        lines += ['    .balign 4', f'    .globl {name}', f'    .type {name}, @object', f'    .size {name}, 4',
                  f'{name}:', f'    .word console_ram + {offset:#x}']
    return '\n'.join(lines) + '\n'


def link(directory, placed, programs, words):
    work = directory / 'link'
    work.mkdir(parents=True, exist_ok=True)
    contracted = contract(images())
    wanted_statics = set(re.findall(r'=\s*"(\w+)"',
                                    STATICS.search((ROOT / 'engine/sa1/records.c').read_text())[1]))

    def main_link(out, labels, held):
        source = work / f'{out.stem}.console.s'
        source.write_text(console_source(labels, held))
        run(tool('as'), '-G0', '-o', source.with_suffix('.o'), source)
        run(*words, source.with_suffix('.o'), '-o', out)
        return table(out)

    placed_rows = [line.split() for line in Path(placed).read_text().splitlines() if line.strip()]
    # each name a program keeps global, as programs.py lists them
    kept = {program: set((directory / f'keep-{program}.txt').read_text().split()) - {'native_none'}
            for program in programs}
    every = set().union(*kept.values())
    # the first pass: the program's layout. each name a program gives, each
    # name the tables place and each word that holds an image's end stands
    # at console_ram's start, so the pass differs from the second in those
    # names' values alone
    first = main_link(work / 'main.first.elf',
                      {**dict.fromkeys(every, 0), **{f[0]: 0 for f in placed_rows if f[1] != '='}},
                      {fields[0]: 0 for fields in placed_rows if fields[1] == '='})
    main_first = globals_of(first)
    console = main_first['console_ram']

    def origin_of(program):
        return BASE + console + console_offset(contracted.origins[program])

    def program_link(program, bound, out):
        script = work / f'{program}.ld'
        script.write_text(SCRIPT.format(origin=origin_of(program), alignment=ALIGNMENT) +
                          ''.join(f'{name} = {address:#x};\n' for name, address in sorted(bound.items())))
        # what no name the program gives reaches stays out, in both passes
        done = run(tool('ld'), '-T', script, '--gc-sections', '--gc-keep-exported', '-e', '0', '-o', out,
                   directory / f'program-{program}.o', check=False)
        return program, done.stderr.strip() if done.returncode else '', table(out) if not done.returncode else []

    needs = {program: undefined(table(directory / f'program-{program}.o')) for program in programs}

    def first_pass(program):
        # each name the program lacks, at 0, so the pass differs from the
        # second in those names' values alone
        return program_link(program, dict.fromkeys(needs[program], 0), work / f'{program}.first.elf')

    with ThreadPoolExecutor(max_workers=8) as pool:
        laid = list(pool.map(first_pass, programs))
    failed = [f'{program}: {error}' for program, error, _ in laid if error]
    if failed:
        fail('a program\'s first pass failed:\n' + '\n'.join(failed[:5]))
    first_tables = {program: rows for program, _, rows in laid}
    # each name a program gives: programs.py keeps a name global in one
    provided = {}
    for program, rows in first_tables.items():
        for name, address in globals_of(rows).items():
            # the link's own names, and the names it bound for the program
            if name not in kept[program]:
                continue
            if name in provided:
                fail(f'{name}: {provided[name][0]} and {program} both give it, where programs.py keeps a name '
                     'global in one')
            provided[name] = (program, address)
    ends = {program: globals_of(rows)['__program_end'] for program, rows in first_tables.items()}
    for program in programs:
        room = contracted.lengths[program]
        if ends[program] - origin_of(program) > room:
            fail(f'{program}: its {ends[program] - origin_of(program):#x} bytes outgrow the {room:#x} the memory '
                 'contract gives it')

    # each name a program gives, as a label in console_ram, and what the
    # console's tables place
    lost = sorted(every - set(provided))
    if lost:
        fail(f'{lost[0]}: programs.py keeps it global in a program, and no program\'s link gives it')
    labels = {name: provided[name][1] - BASE - console for name in every}
    held = {}
    work_images = {word: image for image, (word, _) in WORK.items()}
    for fields in placed_rows:
        if len(fields) == 3 and fields[1] == '=':
            # the word that holds an image's end, the later of the console's
            # and the PSP's
            image = work_images[fields[0]]
            end = console_offset(int(fields[2], 0))
            if image in ends:
                end = max(end, ends[image] - BASE - console)
            held[fields[0]] = end
        elif len(fields) == 3:
            labels[fields[0]] = console_offset(int(fields[1], 0))
    final = main_link(directory / 'main.elf', labels, held)
    main_final = globals_of(final)
    moved = sorted((address, name) for name, address in main_first.items()
                   if name not in labels and main_final.get(name) != address)
    if moved:
        at, name = moved[0]
        fail(f'{len(moved)} of the program\'s names moved between its two links, the lowest {name}, from {at:#x} '
             f'to {main_final.get(name, 0):#x}')

    # the second pass: each program against the program's names and the
    # other programs'
    absolute = {name: BASE + address for name, address in main_final.items()}
    absolute.update({name: address for name, (_, address) in provided.items()})

    def second(program):
        lacking = sorted(needs[program] - set(absolute) - set(contracted.anchored.get(program, {})))
        if lacking:
            return program, f'nothing gives it {", ".join(lacking[:8])}', []
        bound = {name: absolute[name] for name in needs[program] if name in absolute}
        # an anchor of the contract's, which the program's own files load at
        for name, address in contracted.anchored.get(program, {}).items():
            bound[name] = BASE + console + console_offset(address)
        return program_link(program, bound, work / f'{program}.elf')

    with ThreadPoolExecutor(max_workers=8) as pool:
        linked = list(pool.map(second, programs))
    failed = [f'{program}: {error}' for program, error, _ in linked if error]
    if failed:
        fail(f'{len(failed)} programs lack names:\n' + '\n'.join(failed[:8])[:6000])
    # the memory contract, held to the programs as the PSP's link lays them
    # out: a file the game loads over a program's tail, or work it places
    # past one, must find the room here that it finds on the console. the
    # PSP's loader writes a program from its origin to its end, bss and all
    # (engine/psp/loads.c), so its file ends where it does, and its PRS file
    # is the disc's, which no link of the PSP's sizes
    terms = contract(programs)
    extents = {}
    for program, _, rows in linked:
        own = globals_of(rows)
        origin = contracted.origins[program]
        end = origin + own['__program_end'] - origin_of(program)
        start = held.get(WORK.get(program, (None,))[0])
        extents[program] = Extent(end, origin + own['__body_end'] - origin_of(program), end, end, None,
                                  None if start is None or program not in terms.work
                                  else (0x0c000000 + RAM_FIRST + start,
                                        0x0c000000 + RAM_FIRST + start + terms.work[program]))
    problems = check(terms, extents)
    if problems:
        fail(f'the memory contract finds {len(problems)} problems with the PSP\'s programs:\n' +
             '\n'.join(problems[:12]))
    out = directory / 'game'
    (out / 'programs').mkdir(parents=True, exist_ok=True)
    for program, _, rows in linked:
        own = globals_of(rows)
        if any(own.get(name) != address for name, (giver, address) in provided.items() if giver == program):
            fail(f'{program}: a name of its moved between its two links')
        image = work / f'{program}.img'
        run(tool('objcopy'), '-O', 'binary', '-j', '.head', '-j', '.data', '-j', '.rodata', '-j', '.text',
            '-j', '.tail', work / f'{program}.elf', image)
        origin = origin_of(program)
        held_bytes = image.read_bytes()
        # objcopy writes no byte past the last section's, and the image runs
        # to __image_end
        held_bytes += bytes(own['__image_end'] - origin - len(held_bytes))
        head = struct.pack('<5I', PROGRAM_MAGIC, BASE, origin, len(held_bytes), own['__program_end'] - origin -
                           len(held_bytes))
        names = names_table(rows, HEAD.fullmatch, BASE, 0)
        (out / 'programs' / f'{program.upper()}.BIN').write_bytes(head + held_bytes + names)
    (out / 'names.bin').write_bytes(names_table(final, lambda name: name in wanted_statics, 0, console))
    print(f'{directory}/main.elf: the program, and {len(programs)} programs, the largest '
          f'{max(ends[p] - origin_of(p) for p in programs):#x} bytes')


def main():
    args = sys.argv[1:]
    if args[0] == 'partial':
        partial(args[1], args[2], args[3:])
    elif args[0] == 'link':
        split = args.index('--')
        link(Path(args[1]), args[2], args[3:split], args[split + 1:])
    else:
        fail('partial OUT PROGRAM OBJECT..., or link DIR PLACED PROGRAM... -- WORD...')


if __name__ == '__main__':
    main()
