"""Print a C source with each #ifdef GCC block cut to its #else side.

Usage: shc_side.py SOURCE

A source whose make gcc side is GCC's own code holds its original body on
SHC's side of an #ifdef GCC, which tools/inline.py, reading make gcc's guards,
never splits. With the GCC side cut, it does: all.sh splits and translates
the result for the SH-4 run of a test of the port's C of those bodies.
Other conditionals are kept as they are.
"""

import sys


def shc_side(lines):
    """lines, each #ifdef GCC block reduced to its #else part"""
    out, stack = [], []  # each open conditional: [kind, kept]
    for line in lines:
        s = line.strip()
        kept = all(k for _, k in stack)
        if s == '#ifdef GCC':
            stack.append(['gcc', False])
            continue
        if s.startswith('#if'):
            stack.append(['other', True])
            if kept:
                out.append(line)
            continue
        if s == '#else' and stack and stack[-1][0] == 'gcc':
            stack[-1] = ['gcc-else', True]
            continue
        if s.startswith('#el') and stack and stack[-1][0] == 'other':
            if all(k for _, k in stack[:-1]):
                out.append(line)
            continue
        if s.startswith('#endif') and stack:
            kind, _ = stack.pop()
            if kind == 'other' and all(k for _, k in stack):
                out.append(line)
            continue
        if kept:
            out.append(line)
    return out


def main():
    with open(sys.argv[1], encoding='utf-8') as source:
        sys.stdout.write(''.join(shc_side(source)))


if __name__ == '__main__':
    main()
