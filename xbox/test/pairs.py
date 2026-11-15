"""Rewrite SH-4 assembly's memory pair moves for qemu-sh4: pairs.py IN OUT

Under FPSCR.SZ, which FSCHG turns, FMOV moves a pair of FPU registers.
To or from memory, the SH-4 moves the pair's first register at the lower
address, as KallistiOS loads matrices, and as make gcc's side of
njPushMatrix, which runs on the console, loads the same matrices by
singles. qemu-sh4 moves each pair the other way about.

OUT gets IN with each run from one FSCHG to the next that moves pairs only
to or from memory rewritten: each FSCHG becomes a NOP, which keeps the code's
length even, and each pair move two single moves in order, under FRCHG
where the pair is XDn, so that FRn names XFn. Under SZ, FRn names DRn for
an even n and XDn-1 for an odd one, as the instruction's encoding does.
A run that moves a pair between registers too, which qemu-sh4 runs as the
SH-4 does, stays as it is. The tool fails where a run moves a pair by a
form it doesn't rewrite, @Rn or @(R0,Rn), naming the line.
"""

import re
import sys

FSCHG = re.compile(r'^(\s*)FSCHG\b', re.I)
FMOV = re.compile(r'^(\s*)FMOV(?:\.S)?\s+([^,\s]+)\s*,\s*([^\s!;]+)', re.I)
REGISTER = re.compile(r'(FR|DR|XD)(\d+)$', re.I)


def pair(name):
    """('XD' or 'DR', first register's number) for a register under SZ"""
    kind, n = REGISTER.fullmatch(name).groups()
    kind, n = kind.upper(), int(n)
    if kind == 'FR':
        return ('XD', n - 1) if n % 2 else ('DR', n)
    return kind, n


def expand(indent, src, dst, where):
    """a memory pair move's lines as singles"""
    if src.startswith('@'):
        if not re.fullmatch(r'@R\d+\+', src, re.I):
            raise SystemExit(f'{where}: no rewrite of a pair load from {src}')
        kind, n = pair(dst)
        moves = [f'{indent}FMOV.S      {src},FR{n}', f'{indent}FMOV.S      {src},FR{n + 1}']
    else:
        if not re.fullmatch(r'@-R\d+', dst, re.I):
            raise SystemExit(f'{where}: no rewrite of a pair store to {dst}')
        kind, n = pair(src)
        moves = [f'{indent}FMOV.S      FR{n + 1},{dst}', f'{indent}FMOV.S      FR{n},{dst}']
    if kind == 'XD':
        moves = [f'{indent}FRCHG'] + moves + [f'{indent}FRCHG']
    return moves


def main():
    """Read IN, write OUT."""
    if len(sys.argv) != 3:
        raise SystemExit(__doc__.split('\n\n')[0])
    with open(sys.argv[1], encoding='utf-8') as f:
        lines = f.read().splitlines()
    out, i = [], 0
    while i < len(lines):
        if not FSCHG.match(lines[i]):
            out.append(lines[i])
            i += 1
            continue
        end = next((j for j in range(i + 1, len(lines)) if FSCHG.match(lines[j])), None)
        if end is None:
            out += lines[i:]
            break
        moves = [(j, FMOV.match(lines[j])) for j in range(i + 1, end) if FMOV.match(lines[j])]
        memory = [m for _, m in moves if m[2].startswith('@') or m[3].startswith('@')]
        if not moves or len(memory) != len(moves):
            out += lines[i:end + 1]
            i = end + 1
            continue
        out.append(FSCHG.sub(r'\1NOP', lines[i]))
        for j in range(i + 1, end):
            match = FMOV.match(lines[j])
            if match:
                out += expand(match[1], match[2], match[3], f'{sys.argv[1]}:{j + 1}')
            else:
                out.append(lines[j])
        out.append(FSCHG.sub(r'\1NOP', lines[end]))
        i = end + 1
    with open(sys.argv[2], 'w', encoding='utf-8') as f:
        f.write('\n'.join(out) + '\n')


if __name__ == '__main__':
    main()
