"""Lift a straight-line path of SH-4 FPU assembly to C, for make pc.

Usage: sh4lift.py SOURCE FUNCTION LABEL [--reg Rn=NAME]... [--fpul NAME]
                  [--until LABEL]... [--fall-through]

SOURCE is a C source of the tree with an inline body, as
sdk/ninja/njInvertMatrix.c; FUNCTION names the body. The tool interprets
the body from LABEL, or from its first line where LABEL is '-', and
prints C that runs the same float operations on the same operands in the
same order, each a const Float of its own:

- FMUL, FADD, FSUB, FDIV and FNEG, FLDI0 and FLDI1, FMOV between
  registers, FLDS and FSTS through FPUL, FRCHG, which swaps the banks,
  and FSCHG, which makes FMOV move pairs: DRn and XDn are FRn and FRn+1
  of a bank;
- FMOV.S and pair FMOVs to and from memory at @Rn, @Rn+, @-Rn and
  @(R0,Rn), where Rn holds a pointer the run knows: each --reg gives Rn's
  value at entry as NAME, a pointer to Floats, as m, and R15 is the stack.
  MOV, ADD and MOV #imm of the general registers move the pointers;
- a BRA to a label, after its delay slot. The run ends at RTS, at a BRA
  to a label --until names, after the delay slot, or where it reaches
  such a label by running on. With --fall-through, TST and CMP do
  nothing, and a BT or BF without a delay slot never branches, so a run
  can start ahead of the test that chooses its path. Any other branch
  fails the run, as does any instruction the tool doesn't model.

Where a register reads a value the run never wrote, the C reads it from
fr[n] or xf[n], the banks at entry. --fpul names FPUL's value at entry.
Loads and stores through a pointer keep their order: a load reads what an
earlier store of the run wrote there, or NAME[k] as it stood. The C ends
by writing every register the run changed to fr_out[n] and xf_out[n].
"""

import argparse
import re
import sys

LABEL = re.compile(r'^(\?\w+):\s*$')
INSTRUCTION = re.compile(r'^\s+([A-Z][A-Z0-9./]*)\s*([^;]*?)\s*(?:;.*)?$')


class Lift:
    """A run's state: the banks, FPUL, SZ, the general registers' pointers,
    memory, and the C it has written."""

    def __init__(self, registers, fpul):
        self.banks = [dict(), dict()]  # front and back, by register number
        self.front = 0  # which of banks is FR
        self.sz = False
        self.fpul = fpul
        self.gpr = dict(registers)  # Rn -> (base, offset in bytes) or an int
        self.gpr['R15'] = ('stack', 0)
        self.memory = {}
        self.lines = []
        self.count = 0
        self.changed = set()
        self.fall_through = False

    def value(self, bank, n):
        """bank's register n as a C expression."""
        stored = self.banks[bank].get(n)
        if stored is None:
            return f"{'fr' if bank == 0 else 'xf'}[{n}]"
        return stored

    def let(self, expression):
        """a new const Float of expression, returned by name."""
        name = f't{self.count}'
        self.count += 1
        self.lines.append(f'    const Float {name} = {expression};')
        return name

    def fr(self, n):
        return self.value(self.front, n)

    def set_fr(self, n, value):
        self.banks[self.front][n] = value
        self.changed.add((self.front, n))

    def xf(self, n):
        return self.value(1 - self.front, n)

    def set_xf(self, n, value):
        self.banks[1 - self.front][n] = value
        self.changed.add((1 - self.front, n))

    def register(self, name):
        """an FP register's (getter, setter) pairs: one for FRn or XFn, two
        for DRn or XDn"""
        kind, n = re.fullmatch(r'(FR|DR|XD)(\d+)', name).groups()
        n = int(n)
        if kind == 'FR':
            return [(lambda n=n: self.fr(n), lambda v, n=n: self.set_fr(n, v))]
        if kind == 'DR':
            return [(lambda k=k: self.fr(k), lambda v, k=k: self.set_fr(k, v)) for k in (n, n + 1)]
        return [(lambda k=k: self.xf(k), lambda v, k=k: self.set_xf(k, v)) for k in (n, n + 1)]

    def address(self, operand, size):
        """the (base, offset) an operand addresses, and how it moves its
        register: '@Rn', '@Rn+', '@-Rn' or '@(R0,Rn)'"""
        if m := re.fullmatch(r'@\(R0,(R\d+)\)', operand):
            base, offset = self.gpr[m[1]]
            return base, offset + self.gpr['R0']
        if m := re.fullmatch(r'@-(R\d+)', operand):
            base, offset = self.gpr[m[1]]
            self.gpr[m[1]] = (base, offset - size)
            return base, offset - size
        if m := re.fullmatch(r'@(R\d+)(\+?)', operand):
            base, offset = self.gpr[m[1]]
            if m[2]:
                self.gpr[m[1]] = (base, offset + size)
            return base, offset
        raise SystemExit(f'sh4lift.py: no address for {operand}')

    def load(self, base, offset):
        if (base, offset) in self.memory:
            return self.memory[(base, offset)]
        if base == 'stack':
            raise SystemExit(f'sh4lift.py: a load from the stack at {offset} that the run never stored')
        return self.let(f'{base}[{offset // 4}]')

    def store(self, base, offset, value):
        self.memory[(base, offset)] = value
        if base != 'stack':
            self.lines.append(f'    {base}[{offset // 4}] = {value};')

    def fmov(self, src, dst):
        """FMOV and FMOV.S: between registers, or to or from memory"""
        if src.startswith('@'):
            targets = self.register(dst)
            base, offset = self.address(src, 4 * len(targets))
            for k, (_, put) in enumerate(targets):
                put(self.load(base, offset + 4 * k))
        elif dst.startswith('@'):
            sources = self.register(src)
            base, offset = self.address(dst, 4 * len(sources))
            for k, (get, _) in enumerate(sources):
                self.store(base, offset + 4 * k, get())
        else:
            if not self.sz:
                src, dst = src.replace('DR', 'FR'), dst.replace('DR', 'FR')
            for (get, _), (_, put) in zip(self.register(src), self.register(dst)):
                put(get())

    def step(self, op, args):
        """run one instruction; returns a label to go on at, None to go on,
        or 'end'"""
        a = [x.strip() for x in re.split(r',(?![^(]*\))', args)] if args else []
        binary = {'FMUL': '*', 'FADD': '+', 'FSUB': '-', 'FDIV': '/'}
        if op in binary:
            m, n = (int(x[2:]) for x in a)
            self.set_fr(n, self.let(f'{self.fr(n)} {binary[op]} {self.fr(m)}'))
        elif op == 'FNEG':
            n = int(a[0][2:])
            self.set_fr(n, self.let(f'-{self.fr(n)}'))
        elif op in ('FLDI0', 'FLDI1'):
            self.set_fr(int(a[0][2:]), '0.0F' if op == 'FLDI0' else '1.0F')
        elif op in ('FMOV', 'FMOV.S'):
            self.fmov(a[0], a[1])
        elif op == 'FLDS':
            self.fpul = self.fr(int(a[0][2:]))
        elif op == 'FSTS':
            self.set_fr(int(a[1][2:]), self.fpul)
        elif op == 'FRCHG':
            self.front = 1 - self.front
        elif op == 'FSCHG':
            self.sz = not self.sz
        elif op == 'MOV' and a[0].startswith('#'):
            self.gpr[a[1]] = int(a[0][1:], 0)
        elif op == 'MOV.L' and a[0].startswith('#') and a[0][1:].lstrip('-').isdigit():
            self.gpr[a[1]] = int(a[0][1:])
        elif op == 'MOV':
            self.gpr[a[1]] = self.gpr[a[0]]
        elif op == 'ADD' and a[0].startswith('#'):
            value = self.gpr[a[1]]
            k = int(a[0][1:], 0)
            self.gpr[a[1]] = (value[0], value[1] + k) if isinstance(value, tuple) else value + k
        elif op == 'NOP':
            pass
        elif self.fall_through and (op.startswith('TST') or op.startswith('CMP')):
            pass
        elif self.fall_through and op in ('BT', 'BF'):
            pass
        elif op == 'RTS':
            return 'end'
        elif op == 'BRA':
            return a[0]
        else:
            raise SystemExit(f'sh4lift.py: {op} {args} is no instruction the tool models')
        return None


def body(path, function):
    """FUNCTION's inline body in SOURCE, as (labels by line, lines)"""
    text = open(path, encoding='utf-8').read().splitlines()
    start = next(i for i, line in enumerate(text) if re.match(rf'^\w[\w\s*]*\b{function}\s*\(', line))
    end = next(i for i in range(start, len(text)) if text[i] == '}')
    lines = text[start + 2:end]
    return {LABEL.match(line)[1]: i for i, line in enumerate(lines) if LABEL.match(line)}, lines


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source')
    parser.add_argument('function')
    parser.add_argument('label')
    parser.add_argument('--reg', action='append', default=[])
    parser.add_argument('--fpul', default='fpul')
    parser.add_argument('--until', action='append', default=[])
    parser.add_argument('--fall-through', action='store_true')
    args = parser.parse_args()
    labels, lines = body(args.source, args.function)
    registers = {}
    for given in args.reg:
        name, value = given.split('=')
        registers[name] = (value, 0)
    lift = Lift(registers, args.fpul)
    lift.fall_through = args.fall_through
    at = 0 if args.label == '-' else labels[args.label] + 1
    while True:
        line = lines[at]
        at += 1
        if LABEL.match(line) and LABEL.match(line)[1] in args.until:
            break
        match = INSTRUCTION.match(line)
        if not match or match[1].startswith('.'):
            continue
        go = lift.step(match[1], match[2])
        if go is None:
            continue
        # the delay slot runs first
        while True:
            slot = INSTRUCTION.match(lines[at])
            at += 1
            if slot and not slot[1].startswith('.'):
                break
        if lift.step(slot[1], slot[2]) is not None:
            raise SystemExit('sh4lift.py: a branch in a delay slot')
        if go == 'end' or go in args.until:
            break
        at = labels[go] + 1
    out = lift.lines
    for bank, n in sorted(lift.changed):
        out.append(f"    {'fr_out' if bank == 0 else 'xf_out'}[{n}] = {lift.value(bank, n)};")
    print('\n'.join(out))


if __name__ == '__main__':
    sys.exit(main())
