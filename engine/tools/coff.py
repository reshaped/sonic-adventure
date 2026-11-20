#!/usr/bin/env python3
"""A program's partial link and its copy, on COFF.

The Windows slice's image is lld's, which links no partial object, and
llvm-objcopy renames no COFF section and makes no name static, so on Windows
a program links alone and is copied here, as GNU ld and objcopy do it on ELF.

coff.py -r [--defsym NAME=VALUE]... -o OUT OBJECT... joins the objects as
ld -r -d does: each section stays as it is, a name one object defines binds
the others' reads of it, the commons take room in a .bss of their own at the
alignments their objects' .drectve gives, and NAME reads as VALUE, absolute.
The .drectve sections go once their commons lie placed, and .llvm_addrsig
with the symbol numbers the join changes.

coff.py [--redefine-syms FILE] [--keep-global-symbols FILE]
[--rename-section OLD=NEW]... IN OUT copies IN as objcopy does: each name
renamed as FILE's pairs give, each external definition FILE doesn't list
made static, and each section renamed."""
import argparse
import re
import struct
import sys
from pathlib import Path

HEADER = struct.Struct('<HHIIIHH')
SECTION = struct.Struct('<8sIIIIIIHHI')
SYMBOL = struct.Struct('<8sIhHBB')
RELOCATION = struct.Struct('<IIH')
# a section's own symbol's record: its length, its counts, its checksum, the
# number of the section it follows as a COMDAT, and how a COMDAT is chosen
SECTION_RECORD = struct.Struct('<IHHIHB3x')
CODE, UNINITIALIZED, COMDAT, MORE_RELOCATIONS = 0x20, 0x80, 0x1000, 0x1000000
READ, WRITE = 0x40000000, 0x80000000
UNDEFINED, ABSOLUTE = 0, -1
EXTERNAL, STATIC, FILE, WEAK = 2, 3, 103, 105
REL32 = 4
JOINED_OUT = '.drectve', '.llvm_addrsig'
# the widest alignment a section's flags hold, as a power of two: 8192 bytes
MOST_ALIGNED = 13


class Section:
    def __init__(self, name, flags, data, size):
        self.name, self.flags, self.data, self.size, self.relocations = name, flags, data, size, []


class Symbol:
    def __init__(self, name, value, section, kind, storage, aux=b''):
        self.name, self.value, self.section, self.kind, self.storage, self.aux = \
            name, value, section, kind, storage, aux
        # a section's own symbol: that section, and the one its record names
        self.own = self.follows = None

    def defined(self):
        return self.storage == EXTERNAL and self.section != UNDEFINED


def read(path):
    """a COFF object's machine, sections and symbols, each relocation's
    target a symbol"""
    raw = Path(path).read_bytes()
    machine, count, _, table, total, optional, _ = HEADER.unpack_from(raw)
    if machine == 0 and count == 0xffff:
        sys.exit(f'coff.py: {path} is a big object, which no source of the slice makes')
    strings = table + SYMBOL.size * total

    def text(at):
        return raw[at:raw.index(b'\0', at)].decode()

    sections = []
    for k in range(count):
        name, _, _, size, data, at, _, number, _, flags = SECTION.unpack_from(raw, HEADER.size + optional
                                                                              + SECTION.size * k)
        name = name.rstrip(b'\0').decode()
        section = Section(text(strings + int(name[1:])) if name.startswith('/') else name,
                          flags & ~MORE_RELOCATIONS, None if flags & UNINITIALIZED else raw[data:data + size], size)
        first = 0
        if flags & MORE_RELOCATIONS and number == 0xffff:
            # the first relocation's address counts them, itself among them
            number, first = RELOCATION.unpack_from(raw, at)[0], 1
        section.relocations = [RELOCATION.unpack_from(raw, at + RELOCATION.size * i) for i in range(first, number)]
        sections.append(section)
    symbols, at = {}, 0
    while at < total:
        name, value, number, kind, storage, aux = SYMBOL.unpack_from(raw, table + SYMBOL.size * at)
        name = text(strings + struct.unpack_from('<I', name, 4)[0]) if name[:4] == b'\0\0\0\0' \
            else name.rstrip(b'\0').decode()
        records = raw[table + SYMBOL.size * (at + 1):table + SYMBOL.size * (at + 1 + aux)]
        symbol = Symbol(name, value, sections[number - 1] if number > 0 else number, kind, storage, records)
        if storage == STATIC and number > 0 and aux and name == symbol.section.name and not value:
            follows = SECTION_RECORD.unpack_from(records)[4]
            symbol.own, symbol.follows = symbol.section, sections[follows - 1] if follows else None
        elif aux and storage not in (FILE, WEAK):
            sys.exit(f'coff.py: {path}: {name} has records of a kind no source of the slice makes')
        symbols[at] = symbol
        at += 1 + aux
    for section in sections:
        section.relocations = [(address, symbols[index], kind) for address, index, kind in section.relocations]
    return machine, sections, list(symbols.values())


def write(path, machine, sections, symbols):
    strings, placed = bytearray(4), {}

    def name(text, long_form):
        data = text.encode()
        if len(data) <= 8:
            return data.ljust(8, b'\0')
        if text not in placed:
            placed[text] = len(strings)
            strings.extend(data + b'\0')
        if long_form:
            return struct.pack('<II', 0, placed[text])
        if placed[text] > 9999999:
            sys.exit(f'coff.py: {path}: {text} lies past where a section can name it')
        return f'/{placed[text]}'.encode().ljust(8, b'\0')

    number = {section: k + 1 for k, section in enumerate(sections)}
    index, total = {}, 0
    for symbol in symbols:
        index[symbol] = total
        total += 1 + len(symbol.aux) // SYMBOL.size
    out = bytearray(HEADER.size + SECTION.size * len(sections))
    headers = []
    for section in sections:
        data = len(out) if section.data else 0
        out += section.data or b''
        flags, count, at = section.flags, len(section.relocations), 0
        if count:
            at = len(out)
            if count >= 0xffff:
                flags |= MORE_RELOCATIONS
                out += RELOCATION.pack(count + 1, 0, 0)
            for address, target, kind in section.relocations:
                out += RELOCATION.pack(address, index[target], kind)
        headers.append(SECTION.pack(name(section.name, False), 0, 0, section.size, data, at, 0, min(count, 0xffff),
                                    0, flags))
    table = len(out)
    for symbol in symbols:
        aux = symbol.aux
        if symbol.follows:
            aux = aux[:12] + struct.pack('<H', number[symbol.follows]) + aux[14:]
        out += SYMBOL.pack(name(symbol.name, True), symbol.value, number.get(symbol.section, symbol.section),
                           symbol.kind, symbol.storage, len(aux) // SYMBOL.size) + aux
    struct.pack_into('<I', strings, 0, len(strings))
    out[:HEADER.size] = HEADER.pack(machine, len(sections), 0, table, total, 0, 0)
    out[HEADER.size:HEADER.size + SECTION.size * len(sections)] = b''.join(headers)
    Path(path).write_bytes(out + strings)


def link(out, objects, defsyms):
    """ld -r -d's join of OBJECTS as OUT, with each (name, value) of DEFSYMS an
    absolute name"""
    files = [(path, *read(path)) for path in objects]
    machines = {machine for _, machine, _, _ in files}
    if len(machines) != 1:
        sys.exit(f'coff.py: {out}: objects of machines {sorted(machines)}')
    defined, alignments, commons = {}, {}, {}
    for path, _, sections, symbols in files:
        for section in sections:
            if section.name == '.drectve':
                for word in section.data.decode().split():
                    if not (aligned := re.fullmatch(r'-aligncomm:"?([^",]+)"?,(\d+)', word)):
                        sys.exit(f'coff.py: {path}: {word}, a directive the join would lose')
                    alignments[aligned[1]] = max(alignments.get(aligned[1], 0), int(aligned[2]))
        for symbol in symbols:
            if symbol.storage == WEAK:
                sys.exit(f'coff.py: {path}: {symbol.name} is weak, which the join doesn\'t bind')
            if symbol.defined() and not (isinstance(symbol.section, Section) and symbol.section.flags & COMDAT):
                if symbol.name in defined:
                    sys.exit(f'coff.py: {out}: {symbol.name} is defined twice')
                defined[symbol.name] = symbol
    # a read of a COMDAT's name binds the first of its copies
    for _, _, _, symbols in files:
        for symbol in symbols:
            if symbol.defined():
                defined.setdefault(symbol.name, symbol)
    added = []
    for name, value in defsyms:
        if name in defined:
            sys.exit(f'coff.py: {out}: {name} is defined, and --defsym gives it again')
        if not 0 <= value < 1 << 32:
            sys.exit(f'coff.py: {out}: {name}={value:#x}, past the 32 bits COFF keeps an absolute in')
        defined[name] = Symbol(name, value, ABSOLUTE, 0, EXTERNAL)
        added.append(defined[name])
    for _, _, _, symbols in files:
        for symbol in symbols:
            if symbol.storage == EXTERNAL and symbol.section == UNDEFINED and symbol.value \
                    and symbol.name not in defined:
                commons[symbol.name] = max(commons.get(symbol.name, 0), symbol.value)
    joined = []
    if commons:
        bss = Section('.bss', 0, None, 0)
        wide = max(commons, key=lambda name: alignments.get(name, 0))
        widest = alignments.get(wide, 0)
        if widest > MOST_ALIGNED:
            sys.exit(f'coff.py: {out}: {wide} is aligned to {1 << widest} bytes, past the {1 << MOST_ALIGNED} a '
                     'section\'s flags hold')
        bss.flags = UNINITIALIZED | READ | WRITE | (widest + 1) << 20
        for name, size in commons.items():
            step = 1 << alignments.get(name, 0)
            defined[name] = Symbol(name, -(-bss.size // step) * step, bss, 0, EXTERNAL)
            bss.size = defined[name].value + size
            added.append(defined[name])
    undefined, feature, symbols_out = {}, None, []
    for _, _, sections, symbols in files:
        bound = {}
        for symbol in symbols:
            if isinstance(symbol.section, Section) and symbol.section.name in JOINED_OUT:
                continue
            if symbol.storage == EXTERNAL and symbol.section == UNDEFINED:
                if symbol.name not in defined and symbol.name not in undefined:
                    undefined[symbol.name] = Symbol(symbol.name, 0, UNDEFINED, symbol.kind, EXTERNAL)
                bound[symbol] = defined.get(symbol.name) or undefined[symbol.name]
                continue
            # the features each object marks, as one
            if symbol.name == '@feat.00' and symbol.section == ABSOLUTE:
                if feature:
                    feature.value |= symbol.value
                    bound[symbol] = feature
                    continue
                feature = symbol
            bound[symbol] = symbol
            symbols_out.append(symbol)
        for section in sections:
            if section.name not in JOINED_OUT:
                section.relocations = [(address, bound[target], kind) for address, target, kind in section.relocations]
                joined.append(section)
    if commons:
        joined.append(bss)
    write(out, machines.pop(), joined, symbols_out + list(undefined.values()) + added)


def copy(source, target, renames, keep, sections_renamed):
    """objcopy's copy of SOURCE: RENAMES, KEEP's external definitions alone
    left global, SECTIONS_RENAMED"""
    machine, sections, symbols = read(source)
    for section in sections:
        section.name = sections_renamed.get(section.name, section.name)
    for symbol in symbols:
        if symbol.own:
            symbol.name = symbol.own.name
            continue
        symbol.name = renames.get(symbol.name, symbol.name)
        if keep is not None and symbol.defined() and symbol.name not in keep:
            symbol.storage = STATIC
    write(target, machine, sections, symbols)


def calls(path):
    """the names an object's code calls or jumps to, as its relocations' targets
    after a call's, a jump's or a conditional jump's opcode"""
    _, sections, _ = read(path)
    called = set()
    for section in sections:
        if not section.flags & CODE:
            continue
        for address, target, kind in section.relocations:
            # no opcode stands ahead of a relocation at the section's start
            if kind != REL32 or address == 0:
                continue
            opcode, before = section.data[address - 1], section.data[address - 2] if address > 1 else None
            if opcode in (0xe8, 0xe9) or before == 0x0f and 0x80 <= opcode <= 0x8f:
                called.add(target.name)
    return called


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('-r', action='store_true')
    parser.add_argument('-o')
    parser.add_argument('--defsym', action='append', default=[])
    parser.add_argument('--redefine-syms')
    parser.add_argument('--keep-global-symbols')
    parser.add_argument('--rename-section', action='append', default=[])
    parser.add_argument('files', nargs='+')
    args = parser.parse_args()
    if args.r:
        link(args.o, args.files, [(name, int(value, 0)) for name, value in (d.split('=') for d in args.defsym)])
        return
    renames = dict(line.split() for line in Path(args.redefine_syms).read_text().splitlines() if line.strip()) \
        if args.redefine_syms else {}
    keep = set(Path(args.keep_global_symbols).read_text().split()) if args.keep_global_symbols else None
    copy(*args.files, renames, keep, dict(r.split('=') for r in args.rename_section))


if __name__ == '__main__':
    main()
