"""Split a C source's inline assembly from its C, for make gcc.

OUT gets SOURCE line for line, under #line 1 "SOURCE", with each piece of
assembly blank:

- an inline body: its #pragma inline_asm, its lines and its function's
  closing brace. The opening brace becomes a ;, so that the function's head
  declares the function;
- a #pragma asm block, through its #pragma endasm.

TEXT gets each piece GCC compiles in source order, after a comment that
names its line and its function or #pragma asm:

- a body, as R10 writes an inline function's own definition: an .EXPORT
  unless the function is static, its label, its lines, then the .ALIGN 4,
  RTS and NOP R10 writes after one. That RTS returns from a body that runs
  off its end. A body may open with .SECTION lines that select data
  sections and then the code section its code runs in. Its label then
  follows them, where its code starts;
- a block that holds more than assembler comments, as it is.

Each piece starts in P, the code section asmsh gives the lines above any
.SECTION. Between a piece that holds a .SECTION and the next, TEXT selects
P again with .SECTION P,CODE,ALIGN=4.

Where TEXT holds a piece, OUT ends in a file-scope asm that includes TEXT's
translation, TEXT with a .s suffix. The asm names each body's function as
a ":" operand, so that GCC binds even a static one to the asm's label.

A piece GCC doesn't compile, such as one on the shipped side of a GCC
guard, is SHC's alone: TEXT doesn't take it, and the asm names no function
for it. The tool reads which pieces GCC compiles as csource.py's compiled()
does, from #ifdef and #ifndef of GCC, which make gcc's compile defines, and
of __clang__, which it doesn't.

A piece can name a C function or object of the file by its label, and
branch to the function: config/gcc.mk's -ffunction-sections makes the
branch land on it. Each such name a piece in the tree takes is a global
one, and nothing checks that. GCC sees none of a piece's uses, so it would
drop a static that only a piece names, inline or clone a static function,
and fold the reads of a static its C never writes.

The tool writes nothing, and fails, naming the line and its function or
pragma, on:

- a #pragma inline_asm spelled other than (NAME) or (NAME(size=N)), or
  with no function below it, or one that isn't NAME or has no closing
  brace;
- a #pragma asm with no #pragma endasm below it, or a #pragma endasm that
  closes none;
- a piece inside another, such as a second #pragma inline_asm over one
  body;
- a directive inside a piece, which the split can't carry into TEXT;
- a body, or a block that holds more than assembler comments, under a
  conditional compiled() can't read;

and in a piece GCC compiles, on:

- a .SECTION in a body that doesn't open as above: the first that breaks
  that rule, which would leave the function's code in a data section or in
  two sections;
- a branch to other than a label, such as to a name plus or minus an
  offset, a place in SHC's layout that GCC's doesn't keep;
- a block of code inside a function, which TEXT would move out of it.

It fails first, naming the path, where SOURCE, or TEXT with a .s suffix,
holds other than letters, digits, _, ., / and -: OUT's #line names SOURCE,
and its .include TEXT's translation, each unescaped.

GCC.md#inline-assembly says how make gcc builds OUT and TEXT into the
source's object.
"""

import argparse
import re
import sys
from pathlib import Path

from csource import blanked, bodies, compiled, inline_sites, parse, pragma, regions, spans
from tool import content, fail, save

# a .SECTION statement, after any label. Group 1 is the section's kind,
# where the statement gives CODE or DATA. A statement is a piece's line as
# uncommented() gives it
SECTION = re.compile(r'\s*(?:[\w?]+:)?\s*\.SECTION\b(?:\s+[^,]*,\s*(CODE|DATA)\b)?', re.I)
# an empty statement, an .IMPORT or an .EXPORT, none of which places a byte
# or a label
UNPLACED = re.compile(r'\s*(?:\.(?:IMPORT|EXPORT)\b.*)?$', re.I)
# a branch, after any label, and its target, the rest of its statement; and
# the one target the tool takes, a label
BRANCH = re.compile(r'\s*(?:[\w?]+:)?\s*(?:BSR|BRA|BT|BF|BT/S|BF/S)\s+(.*?)\s*$', re.I)
TARGET = re.compile(r'[A-Za-z_?][\w?]*')
# the characters of a path OUT's #line and .include write unescaped
PATH = re.compile(r'[A-Za-z0-9_./-]+')
# the lines R10 writes after the body in an inline function's own definition
TRAILER = ['        .ALIGN      4', '        RTS', '        NOP']
# what TEXT writes between a piece that holds a .SECTION and the next
SELECT_P = '        .SECTION    P,CODE,ALIGN=4'


def uncommented(line):
    """A line of assembly up to its first ;. asmsh opens a comment there unless a string holds the ;, which this
    doesn't tell apart."""
    return line.split(';')[0]


def refused_section(statements):
    """The index of the first .SECTION in a body's statements that breaks the module docstring's rule for a body's
    .SECTION lines, or None."""
    sections = [(i, (match[1] or '').upper()) for i, each in enumerate(statements) if (match := SECTION.match(each))]
    if sections and not all(map(UNPLACED.match, statements[:sections[0][0]])):
        return sections[0][0]
    code = False
    for at, kind in sections:
        if code or not kind:
            return at
        code = kind == 'CODE'
    # the body's code would run in the last data section
    return sections[-1][0] if sections and not code else None


def branch_problems(path, what, first, lines):
    """A problem for each branch in lines to other than one label, such as to a name plus or minus an offset, or to a
    target the tool can't read. lines are a piece's from line first, counting from 1, and what names the piece."""
    return [f"{path}:{i}: {what}: {statement.strip()}, a branch to other than a label, which may be a place in SHC's "
            "layout that GCC's doesn't keep"
            for i, statement in enumerate(map(uncommented, lines), first)
            if (branch := BRANCH.match(statement)) and not TARGET.fullmatch(branch[1])]


def split(path, source, included):
    """OUT's lines and TEXT's lines for source, the C at path, in source order. included is the path of TEXT
    translated for GNU as, which OUT's asm includes. The run fails on the refusals the module docstring lists, naming
    each."""
    code = blanked(source).split('\n')
    lines, directives = source.split('\n'), parse(source).directives
    problems, pieces, empty, openings = [], [], set(), set()
    compiles, sites, functions = compiled(code), inline_sites(code), list(spans(code))
    found, blocks = bodies(path, code, problems), regions(code)
    problems += [f'{path}:{each.line}: not #pragma inline_asm(<function>) or #pragma inline_asm(<function>(size=<n>))'
                 for each in directives if (pragma(each) or [''])[0] == 'inline_asm' and each.line - 1 not in sites]
    problems += [f'{path}:{each.line}: #pragma endasm, closing no #pragma asm' for each in directives
                 if pragma(each) == ['endasm'] and each.line - 1 not in {end for _, end, _ in blocks}]
    # the index of each directive's line
    directed = [each.line - 1 for each in directives]
    # the last line of the pieces above that reaches furthest, and its piece's first
    reach = -1, None
    for at, end, what in sorted([(at, closing, f'#pragma inline_asm({name})') for at, name, *_, closing in found] +
                                [(at, end, '#pragma asm') for at, end, _ in blocks]):
        if at <= reach[0]:
            problems.append(f'{path}:{at + 1}: {what}, inside the piece at line {reach[1] + 1}')
        reach = max(reach, (end, at))
    names = {opening: name for name, opening, _ in functions}
    for at, name, static, opening, closing in found:
        empty.update([at, *range(opening + 1, closing + 1)])
        openings.add(opening)
        if closing == len(code):
            problems.append(f'{path}:{at + 1}: #pragma inline_asm({name}), with no }} closing its function')
        elif names.get(opening) != name:
            problems.append(f'{path}:{at + 1}: #pragma inline_asm({name}), whose function below is '
                            + names.get(opening, "one the tool can't name"))
        problems += [f"{path}:{i + 1}: {name}: a directive inside the piece, which the split can't carry into TEXT"
                     for i in directed if at < i < closing]
        if compiles[at] is None:
            problems.append(f"{path}:{at + 1}: {name}: a body under a conditional the tool can't read")
        if not compiles[at]:
            continue
        body = lines[opening + 1:closing]
        statements = [uncommented(line) for line in body]
        problems += branch_problems(path, name, opening + 2, body)
        refused = refused_section(statements)
        if refused is not None:
            problems.append(f"{path}:{opening + 2 + refused}: {name}: a .SECTION the split can't take: a body's "
                            ".SECTION lines must open it, selecting data sections and then its code section")
            continue
        start = max((i + 1 for i, each in enumerate(statements) if SECTION.match(each)), default=0)
        exported = [] if static else [f'        .EXPORT     _{name}']
        pieces.append((at, [f'; {path}:{at + 1}: {name}', *exported, *body[:start], f'_{name}:', *body[start:],
                            *TRAILER], name))
    for at, end, holds in blocks:
        empty.update(range(at, end + 1))
        if end == len(code):
            problems.append(f'{path}:{at + 1}: #pragma asm, with no #pragma endasm below it')
        problems += [f"{path}:{i + 1}: #pragma asm: a directive inside the piece, which the split can't carry into "
                     'TEXT' for i in directed if at < i < end]
        if holds and compiles[at] is None:
            problems.append(f"{path}:{at + 1}: #pragma asm: a block under a conditional the tool can't read")
        if not holds or not compiles[at]:
            continue
        problems += branch_problems(path, '#pragma asm', at + 2, lines[at + 1:end])
        host = next((name for name, opening, closing in functions if opening < at < closing), None)
        if host:
            problems.append(f'{path}:{at + 1}: {host}: a #pragma asm block of code inside the function, which the '
                            'split would move out of it')
        pieces.append((at, [f'; {path}:{at + 1}: #pragma asm', *lines[at + 1:end]], None))
    if problems:
        fail(*problems)
    pieces.sort(key=lambda piece: piece[0])
    assembly, selects = [], False
    for _, piece, _ in pieces:
        assembly += [SELECT_P, *piece] if selects else piece
        selects = any(SECTION.match(uncommented(line)) for line in piece)
    c = [f'#line 1 "{path}"'] + [';' if i in openings else '' if i in empty else line for i, line in enumerate(lines)]
    if pieces:
        include = f'__asm__(".include \\"{included}\\""'
        operands = ', '.join(f'":"({name})' for *_, name in pieces if name)
        c += [include, f'        : : {operands});'] if operands else [f'{include});']
    return c, assembly


def main(argv):
    parser = argparse.ArgumentParser(prog='inline', description=__doc__.split('\n\n')[0])
    parser.add_argument('source', metavar='SOURCE', help='a C source with inline assembly, such as src/system/thread.c')
    parser.add_argument('out', metavar='OUT', help="where to write SOURCE's C, such as build/gcc/src/system/thread.c")
    parser.add_argument('text', metavar='TEXT',
                        help="where to write SOURCE's assembly, such as build/gcc/src/system/thread_inline.src")
    args = parser.parse_args(argv)
    included = Path(args.text).with_suffix('.s')
    problems = [f"{path}: holds other than letters, digits, _, ., / and -, so OUT can't write it unescaped"
                for path in (args.source, str(included)) if not PATH.fullmatch(path)]
    if problems:
        fail(*problems)
    # latin-1 maps each byte to one character and back, so OUT and TEXT keep SOURCE's bytes, Shift JIS comments
    # included
    c, assembly = split(args.source, content(args.source).decode('latin-1'), included)
    save(args.text, ''.join(f'{line}\n' for line in assembly).encode('latin-1'))
    save(args.out, ''.join(f'{line}\n' for line in c).encode('latin-1'))


if __name__ == '__main__':
    main(sys.argv[1:])
