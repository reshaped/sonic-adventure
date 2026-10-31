"""SHC's and GCC's floating formats, for tools/floats.py: the format each
gives a floating type, exact rounding to a format, and a decimal floating
constant as each reads it, spelled as the hexadecimal constant that names
SHC's value to GCC.

SHC reads a float as binary32, and a double and a long double as binary64
unless a flag makes them floats: under R26 and R32, -fpu=single or
-double=float makes both floats, and R10 takes -double=float alone, which
makes its double a float. Probes of each release show it truncating a
binary32 value toward zero, as R26 and R32 do a binary64 one, where R10
rounds it to the nearest, a tie to even. GCC rounds to the nearest, a tie
to even, in binary32 for a float and, under -m4-single-only, a double, and
in binary64 for a long double.

Each decimal floating constant on the tree's lines becomes the hexadecimal
one, with the same suffix, that names SHC's value for it exactly, which GCC
reads with no rounding: where SHC holds a double as binary64, the value SHC
gives it in a float, truncated. So 0.1F becomes 0x1.999998p-4F, SHC's
0x3dcccccc, where GCC reads 0x3dcccccd. respelled() gives that constant,
and keeps a hexadecimal one, which only a line SHC never reads can hold, as
SHC refuses one. It refuses:

- a pp-number with a point or an exponent that is no floating constant C89
  spells;
- one of more than 17 digits after its leading zeros, which SHC refuses
  with error 2406;
- one SHC's conversion overflows, which it warns of and gives an infinity:
  from 2**128 for binary32, a step past the largest value, where SHC
  truncates, and from half a step past the largest value where R10 rounds
  its binary64 to the nearest;
- one below the smallest normal value but not zero, which SHC truncating
  warns of and gives zero, and R10's binary64, where it lies more than half
  a subnormal's step below, gives a subnormal or, with the warning, zero;
- a double SHC holds as binary64 that no float holds;
- a hexadecimal one but zero whose value GCC's format of its type doesn't
  hold exactly and normally, so that GCC reads each with no rounding.
"""

import re
from collections import namedtuple
from fractions import Fraction

# a decimal floating constant: its digits before the point, the point, its
# digits after it, its exponent and its suffix
DECIMAL = re.compile(r'([0-9]*)(\.?)([0-9]*)(?:[eE]([+-]?[0-9]+))?([fFlL]?)')
# a hexadecimal floating constant: its digits before the point and after it,
# its exponent and its suffix
HEXADECIMAL = re.compile(r'0[xX]([0-9a-fA-F]*)\.?([0-9a-fA-F]*)[pP]([+-]?[0-9]+)([fFlL]?)')
# a binary floating format: its significand's bits, the exponents of its
# normal values, from the lowest to the highest, and how a value rounds to
# it: to the nearest, a tie to even, where nearest is true, and toward zero
# where it is false
Format = namedtuple('Format', 'bits lowest highest nearest')
# SHC's binary32, which each release truncates to, a value it converts as
# well as one it reads or computes
FLOAT = Format(24, -126, 127, False)
# whether each release rounds a binary64 value to the nearest, as probes of
# each show: R10 does, where R26 and R32 truncate it
WIDE = {'r10': True, 'r26': False, 'r32': False}
# C's floating types, by rank, and the suffix of each one's constants
TYPES = {'float': 'F', 'double': '', 'long double': 'L'}
# GCC's format for each type, under -m4-single-only
GCC = {'float': Format(24, -126, 127, True), 'double': Format(24, -126, 127, True),
       'long double': Format(53, -1022, 1023, True)}
# a constant more decades than this from 1 overflows or underflows every
# format, as its digits and exponent show with no power of ten computed
FAR = 400
# the most digits SHC reads in a decimal constant, after its leading zeros
DIGITS = 17


class Route:
    """The SHC that builds a source, and how it holds a floating value there.
    release is r10, r26 or r32. single says the source's flags make its
    double a float: R10's -double=float, or R26's and R32's -fpu=single or
    -double=float. formats gives SHC's format of each floating type, and
    reciprocal whether the flags hold -aggressive=2."""

    def __init__(self, release, single=False, reciprocal=False):
        wide = Format(53, -1022, 1023, WIDE[release])
        double = FLOAT if single else wide
        self.formats = {'float': FLOAT, 'double': double, 'long double': wide if release == 'r10' else double}
        self.release, self.reciprocal = release, reciprocal

    def wider(self, type):
        """Whether SHC holds a floating type in a wider format than GCC does,
        as it does a double in binary64."""
        return self.formats[type].bits > GCC[type].bits


def decimal(number):
    """Whether a pp-number is meant as a decimal floating constant: no
    hexadecimal one, and a point or an exponent in it."""
    return not number[:2] in ('0x', '0X') and any(mark in number for mark in '.eE')


def floating(number):
    """Whether a pp-number is meant as a floating constant, decimal or
    hexadecimal."""
    return decimal(number) or number[:2] in ('0x', '0X') and any(mark in number for mark in 'pP')


def leading(value):
    """The exponent of the power of two at or below value, a positive
    Fraction."""
    top, bottom = value.numerator, value.denominator
    power = top.bit_length() - bottom.bit_length()
    return power - 1 if top << max(-power, 0) < bottom << max(power, 0) else power


class Past(ValueError):
    """A value whose magnitude lies past form's normal values: at or above
    where it overflows form, where over is true, else below its smallest but
    not zero. To the nearest, a value overflows from half a step past the
    largest, and toward zero from a step past it."""

    def __init__(self, form, over):
        self.over = over
        if over:
            half = f' - 2**{form.highest - form.bits}' if form.nearest else ''
            super().__init__(f'2**{form.highest + 1}{half} or above')
        else:
            super().__init__(f'below 2**{form.lowest} but not zero')


def rounded(value, form):
    """value, a Fraction, rounded to form's significand as form rounds. A
    Past says where value's magnitude lies below form's smallest normal
    value but isn't zero, or where it or its rounding overflows form."""
    if not value:
        return Fraction(0)
    magnitude = abs(value)
    power = leading(magnitude)
    if power > form.highest:
        raise Past(form, True)
    if power < form.lowest:
        raise Past(form, False)
    # value truncated to form's bits of significand, and what is left; to the
    # nearest, it rounds up where that is more than half the last bit, or half
    # and the significand odd
    step = power - form.bits + 1
    unit = magnitude.denominator << max(step, 0)
    significand, rest = divmod(magnitude.numerator << max(-step, 0), unit)
    if form.nearest and (2 * rest > unit or 2 * rest == unit and significand & 1):
        significand += 1
        if significand >> form.bits and power == form.highest:
            raise Past(form, True)
    result = significand * Fraction(2) ** step
    return result if value > 0 else -result


def reading(value, form):
    """value, a positive Fraction a decimal constant spells, as a conversion
    to form reads it: as rounded() gives it, but where form rounds to the
    nearest, a value at most half a subnormal's step below the smallest
    normal value reads as that value, as R10 reads its binary64 and GCC its
    own formats. A Past says where rounded() refuses any other value."""
    smallest = Fraction(2) ** form.lowest
    if form.nearest and smallest - smallest / 2 ** form.bits <= value < smallest:
        return smallest
    return rounded(value, form)


def hexadecimal(value, bits, suffix):
    """A positive value of a format of bits of significand, as the hexadecimal
    floating constant with the suffix that names it."""
    power = leading(value)
    significand = int(value / Fraction(2) ** (power - bits + 1))
    pad = -(bits - 1) % 4
    digits = f'{significand - (1 << (bits - 1)) << pad:0{(bits - 1 + pad) // 4}x}'.rstrip('0')
    return f'0x1{"." if digits else ""}{digits}p{power:+d}{suffix}'


def held(value, type, route):
    """SHC's value of a floating type under route as GCC's format of the type
    holds it. Where GCC's is the narrower, for a double SHC holds as
    binary64, it is the float SHC converts the value to. A ValueError says
    where no float holds it."""
    if not route.wider(type):
        return value
    try:
        return rounded(value, FLOAT)
    except ValueError:
        raise ValueError('past the floats, which GCC\'s double is, where SHC holds it as binary64') from None


def typed(number):
    """The type of a floating constant, by its suffix."""
    suffix = number[-1].upper() if number[-1] in 'fFlL' else ''
    return next(type for type, suffixed in TYPES.items() if suffixed == suffix)


def respelled(number, route):
    """The spelling of a floating constant, a pp-number, that GCC reads
    under route, and the constant's value. A decimal constant is spelled as
    the hexadecimal one, with its suffix, that names SHC's value for it as
    GCC's format of its type holds it, and its value is SHC's, unnarrowed
    where SHC holds a double as binary64. A hexadecimal constant keeps its
    spelling, and its value is GCC's, as SHC never reads it. A ValueError
    says why SHC's conversion can't give a decimal constant's value, or GCC's
    format can't hold a hexadecimal one's exactly."""
    type = typed(number)
    if not decimal(number):
        parts = HEXADECIMAL.fullmatch(number)
        if not parts or not (parts[1] or parts[2]):
            raise ValueError('no hexadecimal floating constant C spells')
        whole, fraction, exponent, _ = parts.groups()
        digits, form, exponent = (whole + fraction).lstrip('0'), GCC[type], int(exponent)
        if not digits:
            return number, Fraction(0)
        # the value lies below 2 ** top, and at or above 2 ** (top - 4)
        top = 4 * (len(digits) - len(fraction)) + exponent
        if form.lowest < top and top - 4 <= form.highest:
            value = int(digits, 16) * Fraction(2) ** (exponent - 4 * len(fraction))
            try:
                if rounded(value, form) == value:
                    return number, value
            except Past:
                pass
        raise ValueError(f'a value GCC\'s {type} doesn\'t hold exactly and normally')
    parts = DECIMAL.fullmatch(number)
    if not parts or not (parts[2] or parts[4]):
        raise ValueError('no floating constant C89 spells')
    whole, _, fraction, exponent, suffix = parts.groups()
    digits = (whole + fraction).lstrip('0')
    if not digits:
        return f'0x0p+0{suffix}', Fraction(0)
    if len(digits) > DIGITS:
        raise ValueError(f'more than {DIGITS} digits after its leading zeros, which SHC refuses with error 2406')
    shc, gcc = route.formats[type], GCC[type]
    scale = int(exponent or 0) - len(fraction)
    try:
        if len(digits) + scale > FAR:
            raise Past(shc, True)
        if len(digits) + scale < -FAR:
            raise Past(shc, False)
        value = int(digits) * Fraction(10) ** scale
        read = reading(value, shc)
    except Past as error:
        # each release warns 1012 that its conversion overflows, and 1015 that
        # it underflows, which R10's binary64 does only where it gives zero
        if error.over:
            raise ValueError(f'{error}, where SHC warns that its conversion overflows') from None
        if shc.nearest:
            raise ValueError(f'{error}, where R10 gives a subnormal, or zero with a warning that its conversion '
                             'underflows') from None
        raise ValueError(f'{error}, where SHC warns that its conversion underflows') from None
    return hexadecimal(held(read, type, route), gcc.bits, suffix), read


def spelled(value, route):
    """A floating Value as the hexadecimal constant that names it, as GCC's
    format of its type holds it under route, with its type's suffix."""
    suffix = TYPES[value.type]
    number_ = held(value.number, value.type, route)
    if not number_:
        return f'{"-" if value.negative else ""}0x0p+0{suffix}'
    return f'{"-" if number_ < 0 else ""}{hexadecimal(abs(number_), GCC[value.type].bits, suffix)}'
