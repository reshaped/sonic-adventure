import re
import struct
import unittest
from fractions import Fraction

import float_value


def word(literal):
    """The bits of a hexadecimal floating constant's value, as a float."""
    return struct.unpack('<I', struct.pack('<f', float.fromhex(literal.rstrip('fFlL'))))[0]


def wide(literal):
    """The bits of a hexadecimal floating constant's value, as a binary64, in hexadecimal."""
    return struct.pack('>d', float.fromhex(literal.rstrip('fFlL'))).hex()


# SHC's routes, by name: R32 under -fpu=single, as the game takes it; R32 and R10 with no flag that makes a double a
# float, as SDK sources take them; R10 under -double=float; and R26 with no such flag, which the tool takes though no
# source does
ROUTES = {'game': float_value.Route('r32', True), 'r32': float_value.Route('r32'), 'r10': float_value.Route('r10'),
          'r10 -double=float': float_value.Route('r10', True), 'r26': float_value.Route('r26')}
GAME, R32, R10 = ROUTES['game'], ROUTES['r32'], ROUTES['r10']


class Respell(unittest.TestCase):
    def test_names_shc_s_value_whatever_the_type(self):
        # GCC holds a float and a double in binary32, where 0.1F and 0.1 name SHC's 0x3dcccccc, and a long double in
        # binary64, where 0.1L names R32's truncated 0x3fb9999999999999; and each comes with SHC's value
        self.assertEqual(word(float_value.respelled('0.1F', R32)[0]), 0x3DCCCCCC)
        self.assertEqual(word(float_value.respelled('0.1', R32)[0]), 0x3DCCCCCC)
        self.assertEqual(wide(float_value.respelled('0.1L', R32)[0]), '3fb9999999999999')
        self.assertEqual(float_value.respelled('0.0F', R32), ('0x0p+0F', Fraction(0)))
        self.assertEqual(float_value.respelled('0.5F', R32), ('0x1p-1F', Fraction(1, 2)))
        # R32 with no flag holds 0.1 in binary64: the spelling narrows it to a float, and the value keeps it
        self.assertEqual(float_value.respelled('0.1', R32),
                         ('0x1.999998p-4', Fraction(float.fromhex('0x1.9999999999999p-4'))))

    def test_reads_a_constant_in_the_format_its_route_gives_its_type(self):
        # as probes of each release show: under -fpu=single, R32 holds a long double in binary32, and R10 rounds a
        # binary64 value to the nearest, a long double's under -double=float, where R32 and R26 truncate it. R10 takes
        # a tie to the even neighbour: 2**52 + 0.5 to 2**52, 2**53 + 1 to 2**53 and 2**53 + 3 to 2**53 + 4. 1 - 1e-17
        # lies within half a binary64 step below 1.0, so R10's 64-bit double holds 1.0, which a float takes as it is.
        # 1.7976931348623159e308 lies below 2**1024 and past half a step beyond the largest binary64 value, which R32
        # truncates it to
        for route, number, bits in (('game', '0.1L', '3fb9999980000000'), ('r32', '0.1L', '3fb9999999999999'),
                                    ('r26', '0.1L', '3fb9999999999999'), ('r10', '0.1L', '3fb999999999999a'),
                                    ('r10 -double=float', '0.1L', '3fb999999999999a'),
                                    ('r26', '1.7976931348623157e308L', '7feffffffffffffe'),
                                    ('r32', '1.7976931348623159e308L', '7fefffffffffffff'),
                                    ('r10', '4503599627370496.5L', '4330000000000000'),
                                    ('r10', '9007199254740993.0L', '4340000000000000'),
                                    ('r10', '9007199254740995.0L', '4340000000000002'),
                                    ('r32', '0.99999999999999999', '3fefffffe0000000'),
                                    ('r10', '0.99999999999999999', '3ff0000000000000'),
                                    ('r10 -double=float', '0.99999999999999999', '3fefffffe0000000')):
            with self.subTest(route=route, number=number):
                self.assertEqual(wide(float_value.respelled(number, ROUTES[route])[0]), bits)
        # R10 rounds 1.7976931348623159e308 to an infinity, and warns 1012 that its conversion overflows
        with self.assertRaises(ValueError):
            float_value.respelled('1.7976931348623159e308L', R10)
        # R10 reads 1 - 1e-17 as 1.0, and R32 holds 0.7 as the binary64 0x3fe6666666666666, which a float holds as
        # 0x3f333333
        self.assertEqual(float_value.respelled('0.99999999999999999', R10), ('0x1p+0', Fraction(1)))
        self.assertEqual(word(float_value.respelled('0.7', R32)[0]), 0x3F333333)

    def test_truncates_a_constant_less_than_a_step_past_the_largest_float_to_it(self):
        # R32 and R10 read 3.4028236e+38F, which lies below 2**128, as 0x7f7fffff, where GCC rounds it to an infinity;
        # and they warn 1012 that 3.40282367e+38F overflows
        self.assertEqual(word(float_value.respelled('3.4028236e+38F', GAME)[0]), 0x7F7FFFFF)
        with self.assertRaisesRegex(ValueError, r'^2\*\*128 or above, where SHC warns that its conversion overflows$'):
            float_value.respelled('3.40282367e+38F', GAME)

    def test_keeps_a_hexadecimal_constant_whose_value_gcc_s_format_of_its_type_holds_exactly_and_normally(self):
        # which only a line SHC never reads holds, as SHC refuses one: GCC's 0.1F, zero, the largest float and
        # 1 + 2**-28 as a long double, which GCC holds as binary64, each with its value; but 1 + 2**-28 as a float and
        # as a double, which GCC holds as binary32, 2**-127, below the normal floats, and 2**128, past them, fail
        for number in ('0x1.99999ap-4F', '0x0p+0F', '0x1.fffffep+127F', '0x1.0000001p+0L'):
            with self.subTest(number):
                self.assertEqual(float_value.respelled(number, R32),
                                 (number, Fraction(float.fromhex(number.rstrip('FL')))))
        for number, type in (('0x1.0000001p+0F', 'float'), ('0x1.0000001p+0', 'double'), ('0x1p-127F', 'float'),
                             ('0x1p+128F', 'float')):
            with self.subTest(number), self.assertRaisesRegex(
                    ValueError, f"^a value GCC's {type} doesn't hold exactly and normally$"):
                float_value.respelled(number, R32)

    def test_reads_a_constant_by_its_digits_after_its_leading_zeros_and_builds_no_power_past_every_format(self):
        # 5,000 leading zeros, which SHC's count of 17 digits doesn't take, and an exponent that puts the constant
        # back at 0.1F; and exponents that put a decimal and a hexadecimal constant far past every format, which the
        # tool reads with no power of ten or of two computed
        self.assertEqual(float_value.respelled(f'0.{"0" * 5000}1e5000F', GAME)[0], '0x1.999998p-4F')
        for number, why in (('1e999999999999F', '2**128 or above, where SHC warns that its conversion overflows'),
                            ('1e-999999999999F', 'below 2**-126 but not zero, where SHC warns that its conversion '
                                                 'underflows'),
                            ('0x1p+999999999999F', "a value GCC's float doesn't hold exactly and normally"),
                            ('0x1p-999999999999F', "a value GCC's float doesn't hold exactly and normally")):
            with self.subTest(number), self.assertRaisesRegex(ValueError, f'^{re.escape(why)}$'):
                float_value.respelled(number, GAME)


if __name__ == '__main__':
    unittest.main()
