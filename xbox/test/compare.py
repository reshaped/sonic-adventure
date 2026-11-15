"""Compare the harness's two runs: compare.py SH4 X86

Each line is a call's name, its case and its results, as hex words. A word
that differs counts as:
- sign of zero: 0 against -0, which FIPR's fourth product can decide by a
  register its caller left, so no translation can match it;
- NaN: two NaNs of other bits, the SH-4's default NaN against x86's;
- real: any other difference, which the tool lists, the first 20 of them.
It exits 1 where any difference is real.
"""

import collections
import sys

ZEROS = {'00000000', '80000000'}


def nan(word):
    """whether the float of word's bits is a NaN"""
    bits = int(word, 16)
    return bits & 0x7f800000 == 0x7f800000 and bits & 0x7fffff != 0


def main():
    """Read both runs and print the counts."""
    with open(sys.argv[1], encoding='utf-8') as f:
        sh4 = f.read().splitlines()
    with open(sys.argv[2], encoding='utf-8') as f:
        x86 = f.read().splitlines()
    if len(sh4) != len(x86):
        print(f'the runs wrote {len(sh4)} and {len(x86)} lines')
        return 1
    counts, real = collections.Counter(), []
    for a, b in zip(sh4, x86):
        for i, (p, q) in enumerate(zip(a.split(), b.split())):
            if p == q:
                continue
            if {p, q} == ZEROS:
                counts['sign of zero'] += 1
            elif nan(p) and nan(q):
                counts['NaN'] += 1
            else:
                counts['real'] += 1
                real.append(f'  {a.split()[0]} case {a.split()[1]} word {i}: sh4 {p}, x86 {q}')
    print(f'{len(sh4)} lines; differing words: ' +
          ', '.join(f'{counts[k]} {k}' for k in ('real', 'sign of zero', 'NaN')))
    if real:
        print('\n'.join(real[:20]))
    return 1 if real else 0


if __name__ == '__main__':
    sys.exit(main())
