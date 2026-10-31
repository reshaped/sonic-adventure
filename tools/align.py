"""Raise the alignment an assembly text declares for one of its sections.

OUT gets IN with ALIGNMENT in the ALIGN= operand of the one .SECTION line
that declares SECTION; every other byte is IN's. asmsh records the
alignment a section declares in its object, and the linker starts the
section on a multiple of it.

The tool fails, and leaves OUT as it was, where no .SECTION line declares
SECTION, where two declare it with different operands, where the declaration
gives no ALIGN= operand or more than one, where ALIGNMENT is no larger than
its ALIGN=, and where any other line would change. asmsh warns where a later
declaration of a section gives another alignment than the first, so a second
declaration, even one alike, would have to change too.

The tool reads only the text, and takes the section and the alignment as
given: config/compilers.mk gives them for an object as its ALIGN, with the
evidence beside them.
"""

import argparse
import re
import sys

from tool import content, fail, save

# a section's declaration: the directive, which asmsh takes in either case, and
# its operands, which run to a blank or a comment
DECLARATION = re.compile(r'(\s+\.SECTION\s+)([^\s;]+)(.*)', re.I | re.S)
# the operand that gives a section's alignment, in decimal
ALIGN = re.compile(r'ALIGN=([0-9]{1,9})', re.I)


def align(text, section, alignment, where):
    """text with section's declaration on alignment, as OUT gets it. where
    names text in a failure."""
    if not alignment or alignment & (alignment - 1):
        fail(f'{alignment}: not a power of two')
    # a line runs to its LF, a CR before it included, so OUT keeps every byte
    # IN has but the alignment
    lines = text.split('\n')
    declared = [(number, match) for number, line in enumerate(lines)
                if (match := DECLARATION.fullmatch(line)) and match[2].split(',')[0] == section]
    if not declared:
        fail(f'{where}: no .SECTION line declares {section}')
    first, match = declared[0]
    for number, other in declared[1:]:
        if other[2].upper() != match[2].upper():
            fail(f'{where}:{number + 1}: declares {section} as {other[2]}, and line {first + 1} as {match[2]}')
    operands = match[2].split(',')
    given = [index for index, operand in enumerate(operands) if ALIGN.fullmatch(operand)]
    if len(given) != 1:
        fail(f'{where}:{first + 1}: {match[2]} gives {len(given) or "no"} ALIGN= operands, not one')
    old = int(ALIGN.fullmatch(operands[given[0]])[1])
    if alignment <= old:
        fail(f'{where}:{first + 1}: {section} is on {old} already, and {alignment} is no larger')
    operands[given[0]] = f'{operands[given[0]].split("=")[0]}={alignment}'
    rewritten = {number: f'{other[1]}{",".join(operands)}{other[3]}' for number, other in declared}
    changed = [number for number, line in enumerate(lines) if rewritten.get(number, line) != line]
    if changed != [first]:
        fail(f'{where}:{changed[1] + 1}: declares {section} again, and would change with line {first + 1}; '
             'the tool changes one line')
    lines[first] = rewritten[first]
    return '\n'.join(lines)


def main(argv):
    parser = argparse.ArgumentParser(prog='align', description=__doc__.split('\n\n')[0])
    parser.add_argument('section', metavar='SECTION', help='the section whose alignment to raise, such as P')
    parser.add_argument('alignment', metavar='ALIGNMENT', help='its new alignment in bytes, a power of two')
    parser.add_argument('source', metavar='IN', help='an assembly text, as asmsh reads it')
    parser.add_argument('out', metavar='OUT', help='where to write IN with the new alignment')
    args = parser.parse_args(argv)
    if not re.fullmatch(r'[0-9]{1,9}', args.alignment):
        fail(f'{args.alignment}: no alignment, a number of bytes')
    # latin-1 maps each byte to one character and back, so OUT keeps IN's bytes, Shift JIS comments included
    text = align(content(args.source).decode('latin-1'), args.section, int(args.alignment), args.source)
    save(args.out, text.encode('latin-1'))


if __name__ == '__main__':
    main(sys.argv[1:])
