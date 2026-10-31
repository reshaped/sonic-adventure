"""The check of an assembly text's floating constants, for tools/floats.py
--assembly.

assembled() checks each .FDATA.S constant of an assembly text as asmsh
reads it, which build/shckit translate writes as GNU as's .float. asmsh
and GNU as each read a constant as its nearest float, through a reading of
its decimal that can fall short of its value, and take different floats on
some ties: 16777217.0 is 0x4b800000 to asmsh and 0x4b800001 to GNU as. So
the check fails on a constant on a tie or past one by less than an eighth
of a step, the most it takes a reading to fall short by. It fails on one
past the normal floats: one below the smallest, which asmsh warns of but
for 2**-127, which it writes as 2**-126, or one that rounds to the nearest
past the largest, as 3.4028236S+38 does, which asmsh warns of. And it
fails on an operand that is no F' constant. Each problem names the file
and line.
"""

import re
from fractions import Fraction

from float_value import FAR, FLOAT, Past, leading, rounded

# in an assembly text: the operands of a .FDATA.S line, after any label;
# and a constant asmsh reads there, F' and a signed decimal, its digits
# before the point and after it, and its exponent, after an S or an E
FDATA = re.compile(r'^(?:[A-Za-z_?$.][\w?$.]*:?)?[ \t]+\.FDATA\.S[ \t]+([^;\n]*)', re.I | re.M)
ASSEMBLED = re.compile(r"F'[+-]?([0-9]*)\.?([0-9]*)(?:[SE]([+-]?[0-9]+))?", re.I)
# the format asmsh and GNU as read a constant into, to the nearest
NEAREST = FLOAT._replace(nearest=True)
# the most, in steps, the check takes a reading of a decimal to fall short
# of its value by
SHORTFALL = Fraction(1, 8)


def assembled(text, path):
    """The problems of text, an assembly text at path as asmsh reads it: each
    .FDATA.S operand the check fails on, in the text's order, naming its
    file and line."""
    problems = []
    for match in FDATA.finditer(text):
        line = text.count('\n', 0, match.start()) + 1
        for operand in match[1].split(','):
            parts = ASSEMBLED.fullmatch(operand.strip())
            if not parts or not (parts[1] or parts[2]):
                problems.append(f'{path}:{line}: {operand.strip()}: no F\' constant asmsh reads')
                continue
            whole, fraction, exponent = parts.groups()
            digits = (whole + fraction).lstrip('0')
            if not digits:
                continue
            scale = int(exponent or 0) - len(fraction)
            past = abs(len(digits) + scale) > FAR
            if not past:
                value = int(digits) * Fraction(10) ** scale
                try:
                    rounded(value, NEAREST)
                except Past:
                    past = True
            if past:
                problems.append(f'{path}:{line}: {operand.strip()}: past the normal floats')
                continue
            power = leading(value)
            # how far value lies past the float at or below it, in steps
            beyond = value / Fraction(2) ** (power - FLOAT.bits + 1) % 1
            if Fraction(1, 2) <= beyond < Fraction(1, 2) + SHORTFALL:
                problems.append(f'{path}:{line}: {operand.strip()}: on a tie or past one by less than an eighth of a '
                                'step, where asmsh and GNU as can read two floats')
    return problems
