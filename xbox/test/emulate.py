"""Rewrite SH-4 assembly so that qemu-sh4 can run it.

Usage: emulate.py IN OUT

qemu-sh4 traps FIPR and FTRV unless FPSCR.PR is set, the reverse of the
SH-4, and FSCA and FSRRA under any FPSCR. OUT gets IN, GNU as text such as
build/shckit writes, with each of those instructions replaced by a call to
xbox/test/emulate.s's _harness_emulate, which runs xbox/test/emulate.c's
model of it and returns past the call. The call keeps every register and
T, but for what the instruction writes.

Each call reads a halfword code after its JSR's delay slot: the
instruction's kind in the high byte and its two register numbers in the
low.

An instruction in a delayed branch's slot runs before the branch: the
four write only FPU registers, which no branch reads, and the call keeps
T, the general registers and PR, which the branches read. So the call
goes ahead of the branch, which takes its form without a slot: BF/S and
BT/S become BF and BT, and every other delayed branch keeps a NOP in its
slot. The tool fails, naming the line, where the branch's line also holds
a label, which would then fall after the call.

A call changes the code's length, so a literal pool that followed the
code aligned by the code's length alone may no longer be. Each label whose
next line is a .long or a .float takes a .balign 4 ahead of it: a pool
follows a branch or a return, so the padding never runs. A table of
halves keeps its place, since a BRAF's table must follow the BRAF's slot.
"""

import re
import sys

KINDS = {'fipr': 1, 'ftrv': 2, 'fsca': 3, 'fsrra': 4}
INSTRUCTION = re.compile(r'^(\s*)(fipr|ftrv|fsca|fsrra)\s+([^!;]*?)\s*(?:[!;].*)?$', re.I)
DELAYED = re.compile(r'^\s*(?:\S+:\s*)?(rts|rte|bra|bsr|braf|bsrf|jmp|jsr|bt/s|bf/s|bt\.s|bf\.s)\b', re.I)
# a delayed branch's line with no label, and its mnemonic
BRANCH = re.compile(r'^(\s*)(rts|rte|bra|bsr|braf|bsrf|jmp|jsr|bt/s|bf/s|bt\.s|bf\.s)\b(.*)$', re.I)
LABEL = re.compile(r'^\S+:\s*$')
CODE = re.compile(r'^\s*(?:\S+:\s*)?[a-z]', re.I)


def operands(kind, text):
    """the two register numbers kind's operand text names"""
    names = [name.strip().lower() for name in text.split(',')]
    number = lambda name: int(re.sub(r'^(fv|dr|fr)', '', name))
    if kind == 'fipr':
        return number(names[0]), number(names[1])
    if kind == 'ftrv':
        return 0, number(names[1])
    if kind == 'fsca':
        return 0, number(names[1])
    return 0, number(names[0])


def call(index, kind, text):
    """the lines that replace the instruction"""
    m, n = operands(kind, text)
    code = KINDS[kind] << 8 | m << 4 | n
    return [f'        mov.l   r0,@-r15',
            f'        sts.l   pr,@-r15',
            f'        mov.l   .Lemulate_{index},r0',
            f'        jsr     @r0',
            f'        nop',
            f'        .short  {code}              ! {kind} {text}',
            f'        .align  2',
            f'.Lemulate_{index}:',
            f'        .long   _harness_emulate',
            f'        lds.l   @r15+,pr',
            f'        mov.l   @r15+,r0']


def main():
    """Read IN, write OUT."""
    if len(sys.argv) != 3:
        raise SystemExit(__doc__.split('\n\n')[1])
    out, last, count = [], '', 0
    with open(sys.argv[1], encoding='utf-8') as f:
        lines = f.read().splitlines()
    for number, line in enumerate(lines, 1):
        match = INSTRUCTION.match(line)
        if match:
            if DELAYED.match(last):
                branch = BRANCH.match(last)
                if branch is None or out[-1] != last:
                    raise SystemExit(f'{sys.argv[1]}:{number}: {match[2]} in the slot of a labelled branch')
                out.pop()
                out += call(count, match[2].lower(), match[3])
                mnemonic = branch[2].lower().replace('.s', '/s')
                if mnemonic in ('bt/s', 'bf/s'):
                    out.append(f'{branch[1]}{mnemonic[:2].upper()}{branch[3]}')
                else:
                    out += [last, f'{branch[1]}NOP']
                last = ''
                count += 1
                continue
            out += call(count, match[2].lower(), match[3])
            count += 1
        else:
            out.append(line)
        if CODE.match(line) and not line.lstrip().startswith(('.', '!')):
            last = line
    aligned = []
    for i, line in enumerate(out):
        if (LABEL.match(line) and i + 1 < len(out) and out[i + 1].strip().lower().startswith(('.long', '.float'))
                and not line.startswith('.Lemulate_')):
            aligned.append('        .balign 4')
        aligned.append(line)
    with open(sys.argv[2], 'w', encoding='utf-8') as f:
        f.write('\n'.join(aligned) + '\n')


if __name__ == '__main__':
    main()
