import contextlib
import io
import re
import unittest

import float_value
import floats
from support import tree
from test_float_value import GAME, R10, ROUTES, word

# SH4ZAM's include directory, which make gcc names to the tool as a directory of system headers
SYSTEM = ['tools/sh4zam/include']
# the game's route under -aggressive=2, as curve.c and snow_door.c take it
AGGRESSIVE = float_value.Route('r32', True, True)


def folded(c, route=GAME):
    """c, a.c's C as GCC's preprocessor writes it, once the tool translates it under route, the game's by default,
    with SYSTEM's headers as system headers: the C, or None where the tool fails, and the problems."""
    return floats.translate(f'# 1 "a.c"\n{c}', 'a.i', route, SYSTEM)


def initialized(c, route=GAME):
    """The bits, as a float, of the hexadecimal constant the tool writes in place of c's last initializer once it folds
    it under route."""
    text, problems = folded(c, route)
    if problems:
        raise AssertionError(problems)
    written = re.search(r'= (-?0x[0-9a-f.]+p[-+][0-9]+[fFlL]?)\s*;\n\Z', text)
    if written is None:
        raise AssertionError(f'no constant written in place of the last initializer: {text}')
    return word(written[1])


class Fold(unittest.TestCase):
    # each operator's value as R32 under -fpu=single and R10 under -double=float fold it, which GCC, once the tool
    # respells the constants, folds a step above
    def test_folds_a_sum_truncating_it_as_shc_does(self):
        # 1.0F and SHC's 1e-7F add up to 0.84 of a step past 1.0F, which GCC rounds up to 0x3f800001
        self.assertEqual(initialized('float sum = 1.0F + 1e-7F;\n'), 0x3F800000)

    def test_folds_a_difference_truncating_it_as_shc_does(self):
        # GCC's 0x3fd9999a
        self.assertEqual(initialized('float difference = 2.0F - 0.3F;\n'), 0x3FD99999)

    def test_folds_a_product_truncating_it_as_shc_does(self):
        # as big_rock_shapes holds it, where GCC's is 0x41066666
        self.assertEqual(initialized('float product = 14.0F * 0.6F;\n'), 0x41066665)

    def test_folds_a_quotient_truncating_it_as_shc_does(self):
        # as e102_actions holds it, where GCC's is 0x3e2aaaab
        self.assertEqual(initialized('float quotient = 1.0F / 6.0F;\n'), 0x3E2AAAAA)

    def test_folds_a_negation_of_a_floating_expression_or_of_an_integer(self):
        # the negated quotient, and as traffic_cars holds the product, where GCC's are 0xbe2aaaab and 0xbf666666
        self.assertEqual(initialized('float negation = -(1.0F / 6.0F);\n'), 0xBE2AAAAA)
        self.assertEqual(initialized('float negated = 0.3F * -3;\n'), 0xBF666665)

    def test_rounds_each_operation_toward_zero_one_at_a_time_in_c_s_order(self):
        # SHC's third, 0x3eaaaaaa, three times is a step below 1.0F, where GCC's is 0x3f800000. 16777217.0F
        # truncates to 16777216.0F, so the sum's first operation leaves the second nothing to add to, where the two
        # 1.0Fs added first take it to 16777218.0F
        for c, bits in (('float third = 1.0F / 3.0F * 3.0F;\n', 0x3F7FFFFF),
                        ('float order = 16777216.0F + 1.0F + 1.0F;\n', 0x4B800000),
                        ('float grouped = 16777216.0F + (1.0F + 1.0F);\n', 0x4B800001)):
            with self.subTest(c):
                self.assertEqual(initialized(c), bits)

    def test_folds_a_static_initializer_in_a_function_and_leaves_an_automatic_one_and_code(self):
        # under -frounding-math, GCC leaves the automatic quotient and the product to the FPU, which rounds toward
        # zero on the console
        c = ('void step(void)\n{\n    static const float sixth = 1.0F / 6.0F;\n    float automatic = 1.0F / 6.0F;\n'
             '    automatic = automatic * 6.0F;\n}\n')
        text, problems = folded(c)
        self.assertEqual(problems, [])
        self.assertEqual(text, '# 1 "a.c"\nvoid step(void)\n{\n    static const float sixth = 0x1.555554p-3F  ;\n'
                               '    float automatic = 0x1p+0F / 0x1.8p+2F;\n'
                               '    automatic = automatic * 0x1.8p+2F;\n}\n')

    def test_writes_a_cast_to_an_integer_type_s_value_and_folds_a_cast_to_a_floating_one(self):
        # NJM_DEG_ANG(71), whose quotient the cast truncates to an integer, as SHC's does; and adx_qtbl.c's STEP(9),
        # which converts an integer's conditional
        c = ('typedef float Float;\ntypedef long Angle;\nAngle turn = ((Angle)(((71) * 65536.0) / 360.0));\n'
             'const Float step = ((Float)((9) < 8 ? (9) : (9) - 16) * 65536.0F);\n')
        text, problems = folded(c)
        self.assertEqual(problems, [])
        # the spaces between the tokens a value takes the place of stay
        lines = [' '.join(line.split()) for line in text.splitlines()]
        self.assertEqual(lines[3:], ['Angle turn = 12925 ;', 'const Float step = -0x1.cp+18F ;'])

    def test_keeps_a_system_header_s_arithmetic_and_each_line_an_expression_spans(self):
        # the header's declarations, one that Types reads and one, of a __typeof__, that it doesn't
        c = ('float sixth = 1.0F /\n    6.0F;\nint after;\n# 1 "tools/sh4zam/include/sh4zam/shz_trig.h" 1 3\n'
             'static const float half_pi = 3.14159265359f / 2.0f;\n'
             'static const __typeof__(half_pi) third = 1.0f / 3.0f;\n')
        text, problems = folded(c)
        self.assertEqual(problems, [])
        self.assertEqual(text, '# 1 "a.c"\nfloat sixth = 0x1.555554p-3F \n    ;\nint after;\n'
                               '# 1 "tools/sh4zam/include/sh4zam/shz_trig.h" 1 3\n'
                               'static const float half_pi = 3.14159265359f / 2.0f;\n'
                               'static const __typeof__(half_pi) third = 1.0f / 3.0f;\n')

    def test_reads_gnu_s_attributes_and_a_parameter_s_static_extent_around_a_declaration(self):
        # attributes ahead of the specifiers, beside __extension__ and after static, and after a declarator; and a
        # static extent in a prototype in a function's body, which opens no static declaration. SHC's sixth is
        # 0x3e2aaaaa
        c = ('__extension__ __attribute__((unused)) float before = 1.0F / 6.0F;\n'
             'static __attribute__((unused)) float kept = 1.0F / 6.0F;\n'
             'float after __attribute__((aligned(4))) = 1.0F / 6.0F;\n'
             'void step(void)\n{\n    extern void take(float part[static 2]);\n'
             '    static float sixth = 1.0F / 6.0F;\n    static __attribute__((unused)) float held = 1.0F / 6.0F;\n}\n')
        text, problems = folded(c)
        self.assertEqual(problems, [])
        self.assertEqual([' '.join(line.split()) for line in text.splitlines()[1:]], [
            '__extension__ __attribute__((unused)) float before = 0x1.555554p-3F ;',
            'static __attribute__((unused)) float kept = 0x1.555554p-3F ;',
            'float after __attribute__((aligned(4))) = 0x1.555554p-3F ;', 'void step(void)', '{',
            'extern void take(float part[static 2]);', 'static float sixth = 0x1.555554p-3F ;',
            'static __attribute__((unused)) float held = 0x1.555554p-3F ;', '}'])

    def test_names_the_file_and_line_of_anything_else_floating_in_a_static_initializer(self):
        # a cast to a pointer type, a call and a name, on a.c's lines 1, 2 and 3, and on line 4 a compound literal
        # that holds a cast to a floating type
        c = ('char *cast = (char *)(1.0F / 3.0F);\nfloat call = sqrtf(2.0F);\nfloat name = LIMIT * 0.5F;\n'
             'int literal = (int){ (float)16777219 > 16777218 };\n')
        self.assertEqual(folded(c)[1], [
            'a.c:1: (char *)(0x1p+0F / 0x1.8p+1F): a cast to char *',
            'a.c:2: sqrtf(0x1p+1F): a call',
            'a.c:3: LIMIT * 0x1p-1F: LIMIT, which no declaration in scope names',
            'a.c:4: (int){ (float)16777219 > 16777218 }: a compound literal',
        ])
        with tree({'a.i': '# 1 "a.c"\nfloat call = sqrtf(2.0F);\n'}) as root:
            with self.assertRaisesRegex(SystemExit, r'\A[^\n]*: a\.c:1: sqrtf\(0x1p\+1F\): a call\Z'):
                floats.main(['--shc', 'r32', '-fpu=single', 'a.i'])
            self.assertEqual((root / 'a.i').read_text(), '# 1 "a.c"\nfloat call = sqrtf(2.0F);\n')

    def test_fails_on_a_floating_constant_of_a_system_header_s_line_in_an_initializer_on_the_tree_s(self):
        # as an #include inside braces gives: the tool reads no such constant as SHC does, and leaves no arithmetic
        # on such a line folded
        c = ('float table[] = {\n# 1 "tools/sh4zam/include/sh4zam/shz_trig.h" 1 3\n0x1p-2f / 3, 1.0f / 3.0f\n'
             '# 2 "a.c" 2\n};\n')
        why = "a floating constant on a system header's line, which the fold doesn't read"
        self.assertEqual(folded(c), (None, [
            f'tools/sh4zam/include/sh4zam/shz_trig.h:1: 0x1p-2f / 3: 0x1p-2f, {why}',
            f'tools/sh4zam/include/sh4zam/shz_trig.h:1: 1.0f / 3.0f: 1.0f, {why}']))

    def test_leaves_a_call_of_gcc_s_infinity_as_written(self):
        # GCC gives __builtin_inff()'s value exactly, under its sign and parentheses, so the initializer keeps its own
        c = 'static float above = __builtin_inff();\nstatic float below = -(__builtin_huge_valf());\n'
        text, problems = folded(c)
        self.assertEqual(problems, [])
        self.assertEqual(text.splitlines()[1:], c.splitlines())

    # an integer subexpression, a comparison, ?: and a cast to an integer type, each as SHC folds it, and each
    # double as its route holds it
    def test_evaluates_an_integer_subexpression_as_c_s_integers_do(self):
        # the quotient of two ints is an int, and each of SHC's integer operators gives an int's value; an
        # enumerator is an int
        for c, bits in (('float half = (1 / 2) * 1.0F;\n', 0x00000000), ('float less = (-7 / 2) * 1.0F;\n', 0xC0400000),
                        ('float rest = (-7 % 4) * 1.0F;\n', 0xC0400000), ('float shifted = (1 << 3) * 0.5F;\n', 0x40800000),
                        ('float unsigned_ = (0x80000000U >> 28) * 1.0F;\n', 0x41000000),
                        ('float bits = (5 & 3 | 8 ^ 1) * 1.0F;\n', 0x41100000),
                        ('float complement = ~0 * 1.0F;\n', 0xBF800000),
                        ('float both = (2 && 0 || 3) * 1.0F;\n', 0x3F800000),
                        ('enum { SCALE = 3 };\nfloat scaled = SCALE * 0.5F;\n', 0x3FC00000)):
            with self.subTest(c):
                self.assertEqual(initialized(c), bits)

    def test_takes_c_s_precedence_and_promotes_a_narrow_integer_to_int(self):
        # SHC's words where operators of two precedence levels meet unparenthesized
        for c, bits in (('float f = 1.0F + 2.0F * 3.0F;\n', 0x40E00000),
                        ('float f = (7 + 5 % 3) * 1.0F;\n', 0x41100000),
                        ('float f = (1 << 2 + 1) * 1.0F;\n', 0x41000000),
                        ('float f = (3 < 2 == 0) * 1.0F;\n', 0x3F800000),
                        ('float f = (1 & 2 == 2) * 1.0F;\n', 0x3F800000),
                        ('float f = (1 || 0 && 0) * 1.0F;\n', 0x3F800000)):
            with self.subTest(c):
                self.assertEqual(initialized(c), bits)
        # a conditional as a conditional's last operand, and an unsigned char and an unsigned short, each of which
        # promotes to int: no value rounds, so each stays, and GCC computes 2.0F, -1.0F and -1.0F as SHC does
        c = ('float nested = 1 ? 2.0F : 0 ? 3.0F : 4.0F;\nfloat promoted = (unsigned char)1 - 2;\n'
             'float narrow = (unsigned short)1 - 2;\n')
        self.assertEqual(folded(c), ('# 1 "a.c"\nfloat nested = 1 ? 0x1p+1F : 0 ? 0x1.8p+1F : 0x1p+2F;\n'
                                     'float promoted = (unsigned char)1 - 2;\nfloat narrow = (unsigned short)1 - 2;\n',
                                     []))

    def test_wraps_unsigned_arithmetic_as_c_s_does(self):
        # -1U is 4294967295, alone, under a cast and as 1 - 2U, and SHC truncates it to the float 0x4f7fffff, where
        # GCC rounds it to 0x4f800000; 0xffffffffU + 2U is 1, and -1 converts to 4294967295 to compare with 1U. a cast
        # of an int to an unsigned type takes it modulo the type's range: -1 to 4294967295 and 255, 70000 to 4464
        for c, bits in (('float alone = -1U;\n', 0x4F7FFFFF), ('float cast = (float)-1U;\n', 0x4F7FFFFF),
                        ('float unsigned_ = (unsigned)-1 * 1.0F;\n', 0x4F7FFFFF),
                        ('float byte = (unsigned char)-1 * 1.0F;\n', 0x437F0000),
                        ('float half = (unsigned short)70000 * 1.0F;\n', 0x458B8000),
                        ('float difference = 1 - 2U + 0.0F;\n', 0x4F7FFFFF),
                        ('float sum = (0xffffffffU + 2U) * 1.0F;\n', 0x3F800000),
                        ('float compared = (-1 < 1U) * 1.0F;\n', 0x00000000)):
            with self.subTest(c):
                self.assertEqual(initialized(c), bits)

    def test_folds_a_character_constant_as_the_int_it_is(self):
        # 'a' is 97, '\n' 10, '\101' 65 and '\x7f' 127. SHC's '\xff' is -1, as its plain char is signed, which the
        # fold doesn't take, and 'ab' packs two characters: each fails, naming the constant
        for c, bits in (("float letter = 'a' * 1.0F;\n", 0x42C20000), ("float newline = '\\n' + 0.5F;\n", 0x41280000),
                        ("float octal = '\\101' * 1.0F;\n", 0x42820000),
                        ("float hexadecimal = '\\x7f' * 1.0F;\n", 0x42FE0000)):
            with self.subTest(c):
                self.assertEqual(initialized(c), bits)
        self.assertEqual(folded("float byte = '\\xff' * 1.0F;\nfloat pair = 'ab' * 1.0F;\n")[1], [
            "a.c:1: '\\xff' * 0x1p+0F: '\\xff', a character constant the fold can't read",
            "a.c:2: 'ab' * 0x1p+0F: 'ab', a character constant the fold can't read"])

    def test_converts_an_integer_where_c_s_usual_arithmetic_conversions_do_truncating_it(self):
        # 16777219 converts to 16777218.0F, where GCC rounds it to 16777220.0F, and so it does under a cast; but the sum
        # of two ints is an int, which converts once, as 16777218, where each converted first would truncate to
        # 16777216.0F
        for c, bits in (('float difference = 16777219 - 16777216.0F;\n', 0x40000000),
                        ('float cast = (float)16777219 - 16777216;\n', 0x40000000),
                        ('float product = 16777219 * 1.0F;\n', 0x4B800001),
                        ('float sum = (16777217 + 1) * 1.0F;\n', 0x4B800001)):
            with self.subTest(c):
                self.assertEqual(initialized(c), bits)

    def test_fails_on_an_overflow_a_shift_out_of_range_a_division_by_zero_or_a_cast_out_of_range(self):
        # an integer cast to a signed type that can't hold it fails too, where SHC wraps it: (signed char)200 and
        # (char)200 to -56, and (short)-40000 to 25536. Plain char holds only what signed and unsigned char both hold
        c = ('typedef unsigned char Uint8;\nfloat over = (2147483647 + 1) * 1.0F;\nfloat shift = (1 << 32) * 1.0F;\n'
             'float zero = (1 / 0) * 1.0F;\nfloat quotient = 1.0F / 0.0F;\nfloat cast = (Uint8)256.0F * 1.0F;\n'
             'Uint8 converted = 256.0F;\nfloat byte = (signed char)200 * 1.0F;\nfloat low = (signed char)-200 * 1.0F;\n'
             'float word = (int)0x80000000U * 1.0F;\nfloat half = (short)-40000 * 1.0F;\n'
             'float plain = (char)200 * 1.0F;\n')
        self.assertEqual(folded(c)[1], [
            'a.c:2: (2147483647 + 1) * 0x1p+0F: 2147483647 + 1: 2147483648 lies outside int',
            'a.c:3: (1 << 32) * 0x1p+0F: 1 << 32: a shift by 32, outside 0 to 31',
            'a.c:4: (1 / 0) * 0x1p+0F: 1 / 0: a division by zero',
            'a.c:5: 0x1p+0F / 0x0p+0F: a division by zero',
            "a.c:6: (Uint8)0x1p+8F * 0x1p+0F: a cast to Uint8, which can't hold 256",
            "a.c:7: 0x1p+8F: a conversion to unsigned char, which can't hold 256",
            "a.c:8: (signed char)200 * 0x1p+0F: a cast to signed char, which can't hold 200",
            "a.c:9: (signed char)-200 * 0x1p+0F: a cast to signed char, which can't hold -200",
            "a.c:10: (int)0x80000000U * 0x1p+0F: a cast to int, which can't hold 2147483648",
            "a.c:11: (short)-40000 * 0x1p+0F: a cast to short, which can't hold -40000",
            "a.c:12: (char)200 * 0x1p+0F: a cast to char, which can't hold 200",
        ])

    def test_casts_to_an_integer_type_truncating_toward_zero(self):
        # -2.7F to -2, and 255.9F to 255, which an unsigned char holds
        for c, bits in (('float down = (int)(-2.7F) * 1.0F;\n', 0xC0000000),
                        ('float byte = (unsigned char)255.9F * 1.0F;\n', 0x437F0000)):
            with self.subTest(c):
                self.assertEqual(initialized(c), bits)
        # SHC's 0.7F * 10 is a step below 7.0F, which the cast truncates to 6, where GCC's product is 7.0F; the int it
        # initializes takes the value. 0x1.666666P-1F, a hexadecimal constant with an uppercase P, which only a line
        # SHC never reads can hold, is SHC's 0.7F
        text, problems = folded('int seven = (int)(0.7F * 10) + 1;\nint six = (int)(0x1.666666P-1F * 10);\n')
        self.assertEqual((' '.join(text.split()), problems), ('# 1 "a.c" int seven = 7 ; int six = 6 ;', []))

    def test_takes_a_comparison_and_a_conditional_as_c_does(self):
        # SHC's third, three times, lies below 1.0F, where GCC's is 1.0F; && and || take a float's truth; ?: takes the
        # arm its condition chooses, in the two arms' type, so 16777219 truncates to the float 16777218.0F, 3 divides
        # as 3.0F, and -1 beside 0U, in either arm, is 4294967295U; and 16777219 truncates to 16777218.0F in a
        # condition too, which chooses the second arm
        for c, bits in (('float below = (1.0F / 3.0F * 3.0F < 1.0F) * 1.0F;\n', 0x3F800000),
                        ('float equal = (0.5F == 0.5F) + (1.0F != 1.0F) * 1.0F;\n', 0x3F800000),
                        ('float not_ = !(1.0F - 1.0F) * 1.0F;\n', 0x3F800000),
                        ('float and_ = (0.0F && 1.0F) * 1.0F;\n', 0x00000000),
                        ('float or_ = (0.0F || 1.0F) * 1.0F;\n', 0x3F800000),
                        ('float chosen = 0 ? 1.0F : 16777219;\n', 0x4B800001),
                        ('float typed = (0 ? 1.0F : 3) / 2;\n', 0x3FC00000),
                        ('float unsigned_ = 1 ? -1 : 0U;\n', 0x4F7FFFFF), ('float first = 0 ? 0U : -1;\n', 0x4F7FFFFF),
                        ('float condition = (16777219 > 16777218.0F) ? 1.0F : 2.0F;\n', 0x40000000)):
            with self.subTest(c):
                self.assertEqual(initialized(c), bits)
        # an int takes the value of !, && and || where an operand converts with rounding, and of a comparison whose
        # operand a cast to a floating typedef converts
        c = ('typedef float Float;\nint not_ = !(16777219 > 16777218.0F);\n'
             'int and_ = (16777219 > 16777218.0F) && 1.0F;\nint or_ = (16777219 > 16777218.0F) || 0.0F;\n'
             'int cast = (Float)16777219 > 16777218;\n')
        text, problems = folded(c)
        self.assertEqual(([' '.join(line.split()) for line in text.splitlines()[2:]], problems),
                         (['int not_ = 1 ;', 'int and_ = 0 ;', 'int or_ = 0 ;', 'int cast = 0 ;'], []))

    def test_converts_a_comparison_s_operands_as_c_s_usual_arithmetic_conversions_do(self):
        # 16777217 truncates to the float 16777216.0F, no greater than it, and 16777219 to 16777218.0F, equal to it,
        # where GCC rounds it to 16777220.0F; an int takes the comparison's value
        for c, bits in (('float greater = (float)(16777217 > 16777216.0F) * 3.0F;\n', 0x00000000),
                        ('float at_most = (16777219 <= 16777218.0F) * 1.0F;\n', 0x3F800000)):
            with self.subTest(c):
                self.assertEqual(initialized(c), bits)
        text, problems = folded('int greater = 16777219 > 16777218.0F;\nint equal = 16777219 == 16777218.0F;\n')
        self.assertEqual((' '.join(text.split()), problems), ('# 1 "a.c" int greater = 0 ; int equal = 1 ;', []))

    def test_gives_a_zero_the_sign_shc_gives_it(self):
        # a product's or a quotient's zero is negative where its operands' signs differ, a sum's where both are
        # negative, a difference's where the first is and the second isn't; a negation flips the sign, and a
        # conversion keeps it
        for c, bits in (('float zero = -(0.0F * 1.0F);\n', 0x80000000), ('float zero = 0.0F * -1.0F;\n', 0x80000000),
                        ('float zero = -0.0F * 1.0F;\n', 0x80000000), ('float zero = -0.0F * -1.0F;\n', 0x00000000),
                        ('float zero = 0.0F * 1.0F;\n', 0x00000000), ('float zero = 0.0F / -1.0F;\n', 0x80000000),
                        ('float zero = -0.0F + -0.0F;\n', 0x80000000), ('float zero = -0.0F + 0.0F;\n', 0x00000000),
                        ('float zero = -0.0F - 0.0F;\n', 0x80000000), ('float zero = 0.0F - 0.0F;\n', 0x00000000),
                        ('float zero = (double)-0.0F * 1.0;\n', 0x80000000)):
            with self.subTest(c):
                self.assertEqual(initialized(c), bits)

    def test_reads_a_name_as_the_innermost_declaration_in_scope_declares_it(self):
        # the enumerator K, past a function whose local K's scope closes, folds to twice SHC's 0.1F; a block's static
        # const K hides it, and the fold reads no object, where GCC gives y 3 times 0.1F
        c = ('enum { K = 2 };\nvoid h(void)\n{\n    float K = 1.0F;\n}\nfloat twice = K * 0.1F;\nfloat g(void)\n{\n'
             '    static const float K = 3.0F;\n    static float y = K * 0.1F;\n    return y;\n}\n')
        self.assertEqual(folded(c), (None, ["a.c:10: K * 0x1.999998p-4F: K, a name the fold can't read"]))
        self.assertEqual(initialized(c[:c.index('float g')]), 0x3E4CCCCC)

    def test_fails_where_it_reads_no_value_for_the_whole_and_one_gcc_gives_otherwise_takes_part(self):
        # each initializer but two's fails: the fold reads no value for it, as it holds an address or sizeof, and a
        # value GCC gives otherwise takes part, as an operand, a condition or an arm. SHC's 0.7F * 10 truncates to 6,
        # where GCC's rounds to 7. SHC's 16777219 truncates to 16777218.0F, no greater, where GCC's rounds to
        # 16777220.0F. So SHC gives sixth, above, chosen, arm and size buf + 6, buf, buf + 1, 0 and 10, where GCC
        # gives buf + 7, buf + 1, buf, 1 and 11. two's condition doesn't choose the arm that holds such a value, so
        # the fold reads the whole, 2, which GCC computes as SHC does
        c = ('char buf[16];\nchar *sixth = buf + (int)(0.7F * 10);\nchar *above = buf + (16777219 > 16777218.0F);\n'
             'char *chosen = (16777219 > 16777218.0F) ? buf : buf + 1;\n'
             'int arm = sizeof(int) ? (16777219 > 16777218.0F) : 0;\nint size = sizeof(int) + (int)(0.7F * 10);\n')
        beside = 'beside a value GCC can give otherwise'
        self.assertEqual(folded(c), (None, [
            f"a.c:2: buf + (int)(0x1.666666p-1F * 10): buf, a name the fold can't read, {beside}",
            f"a.c:3: buf + (16777219 > 0x1.000002p+24F): buf, a name the fold can't read, {beside}",
            f"a.c:4: (16777219 > 0x1.000002p+24F) ? buf : buf + 1: buf, a name the fold can't read, {beside}",
            f'a.c:5: sizeof(int) ? (16777219 > 0x1.000002p+24F) : 0: sizeof, {beside}',
            f'a.c:6: sizeof(int) + (int)(0x1.666666p-1F * 10): sizeof, {beside}']))
        self.assertEqual(folded('int two = 1 ? 2 : (int)(0.7F * 10);\n'),
                         ('# 1 "a.c"\nint two = 1 ? 2 : (int)(0x1.666666p-1F * 10);\n', []))

    def test_fails_where_either_arm_of_a_conditional_doesn_t_fold(self):
        c = 'float divided = 1 ? 2.0F : 1.0F / 0.0F;\nfloat called = 0 ? sqrtf(2.0F) : 1.0F;\n'
        self.assertEqual(folded(c)[1], ['a.c:1: 1 ? 0x1p+1F : 0x1p+0F / 0x0p+0F: a division by zero',
                                        'a.c:2: 0 ? sqrtf(0x1p+1F) : 0x1p+0F: a call'])

    def test_folds_a_double_and_a_long_double_in_the_format_their_route_gives_them(self):
        # SHC's third of 1.0 three times: a float a step below 1.0 where a flag makes double a float, as under R10's
        # -double=float, and a step below it as binary64, which converts to a float a step below 1.0F too, under R32
        # with a 64-bit double; but R10 rounds a binary64 third to the nearest, and three of it to 1.0. R10's long
        # double is binary64 under -double=float, and R32's binary32 under -fpu=single
        for route, c, bits in (('game', 'double d = 1.0 / 3.0 * 3.0;\n', 0x3F7FFFFF),
                               ('r10 -double=float', 'double d = 1.0 / 3.0 * 3.0;\n', 0x3F7FFFFF),
                               ('r32', 'float f = 1.0 / 3.0 * 3.0;\n', 0x3F7FFFFF),
                               ('r10', 'float f = 1.0 / 3.0 * 3.0;\n', 0x3F800000),
                               ('game', 'float l = 1.0L / 3.0L * 3.0L;\n', 0x3F7FFFFF),
                               ('r10 -double=float', 'float l = 1.0L / 3.0L * 3.0L;\n', 0x3F800000)):
            with self.subTest(route=route, c=c):
                self.assertEqual(initialized(c, ROUTES[route]), bits)

    def test_folds_each_double_from_the_value_its_route_reads_it_as(self):
        # with a 64-bit double, R10 reads 0.1 as the binary64 nearest it, ten of which round to 1.0, and R32 reads
        # 1.0000001073 as binary64, 0.9 of a float's step past 1.0, three of which truncate into the float
        # 0x40400001; each double cut to a float first, as its respelling holds it, would fold a step below. A float
        # meets a double, as an operand or as an arm of ?:, in binary64, which holds 16777217.0
        for route, c, bits in (('r10', 'float f = 0.1 * 10.0;\n', 0x3F800000),
                               ('r10', 'double d = 0.1 * 10.0;\n', 0x3F800000),
                               ('r32', 'float f = 1.0000001073 * 3.0;\n', 0x40400001),
                               ('r10', 'float f = (16777216.0F + 1.0) - 16777216.0F;\n', 0x3F800000),
                               ('r10', 'float f = (1 ? 16777216.0F : 0.1) + 1.0F - 16777216.0F;\n', 0x3F800000)):
            with self.subTest(route=route, c=c):
                self.assertEqual(initialized(c, ROUTES[route]), bits)
        # R10's and R32's binary64 double holds 16777219 and 16777217.0: a double takes the first cut to a float,
        # 0x4b800001, where GCC's float double rounds it to 0x4b800002, and an int takes the second, which GCC's float
        # double reads as 16777216. 0.1F and 0.1 differ in binary64
        c = 'double big = 16777219;\nint whole = (int)16777217.0;\nint equal = 0.1F == 0.1;\n'
        for route in ('r10', 'r32'):
            with self.subTest(route):
                text, problems = folded(c, ROUTES[route])
                lines = [' '.join(line.split()) for line in text.splitlines()]
                self.assertEqual((lines[1:], problems),
                                 (['double big = 0x1.000002p+24;', 'int whole = 16777217;', 'int equal = 0 ;'], []))
        # R10's 0.7 * 10.0 rounds to 7.0, a tie, which the cast keeps, and a long double takes R10's 0.1 whole,
        # 0x3fb999999999999a. (int)16777217.5 truncates to 16777217, and its negation is -16777217. R10 rounds
        # 2**52 + 0.5 to 2**52, the even neighbour
        c = ('int seven = (int)(0.7 * 10.0);\nlong double tenth = 0.1;\nint cut = (int)16777217.5;\n'
             'int negated = -(int)16777217.5;\nlong double tie = 4503599627370496.0L + 0.5L;\n')
        text, problems = folded(c, R10)
        lines = [' '.join(line.split()) for line in text.splitlines()]
        self.assertEqual((lines[1:], problems), (['int seven = 7 ;', 'long double tenth = 0x1.999999999999ap-4L;',
                                                  'int cut = 16777217;', 'int negated = -16777217;',
                                                  'long double tie = 0x1p+52L ;'], []))

    def test_folds_under_the_route_the_command_line_gives(self):
        # R10's double is binary64, where its third of 1.0 three times rounds to 1.0, but under -double=float a float,
        # where the product is a step below 1.0
        for flags, written in (([], 'double d = 0x1p+0 ;'), (['-double=float'], 'double d = 0x1.fffffep-1 ;')):
            with self.subTest(flags), tree({'a.i': '# 1 "a.c"\ndouble d = 1.0 / 3.0 * 3.0;\n'}) as root:
                floats.main(['--shc', 'r10', *flags, 'a.i'])
                self.assertEqual(' '.join((root / 'a.i').read_text().split()), f'# 1 "a.c" {written}')

    def test_takes_no_release_the_probes_don_t_read(self):
        # argparse refuses it, with its usage
        with tree({'a.i': '# 1 "a.c"\ndouble third = 1.0 / 3.0;\n'}), contextlib.redirect_stderr(io.StringIO()), \
                self.assertRaises(SystemExit) as caught:
            floats.main(['--shc', 'r99', 'a.i'])
        self.assertEqual(caught.exception.code, 2)

    # each scalar initializer converts to the type of the object it initializes, as SHC converts it
    def test_converts_each_integer_initializer_to_its_object_s_floating_type(self):
        # 16777219 truncates to 16777218.0F, 0x4b800001, where GCC rounds it to 16777220.0F: as a float, and as an
        # array's element, a struct's member and a union's first member, through braces C elides; 7 converts exactly,
        # and an int takes its own
        c = ('typedef struct { float pos[3]; int count; } Point;\nfloat bare = 16777219;\n'
             'float elements[] = { 7, 16777219 };\nstruct pair { int count; float scale; } pair = { 16777219, 16777219 };\n'
             'union either { float as_float; int as_int; } either = { 16777219 };\n'
             'struct pair pairs[2] = { 1, 2, 3, 16777219 };\nPoint point = { { 1, 2, 16777219 }, 16777219 };\n'
             'struct held { union either either; float scale; } held = { 16777219, 16777219 };\n')
        text, problems = folded(c)
        self.assertEqual(problems, [])
        self.assertEqual(text.splitlines()[2:], [
            'float bare = 0x1.000002p+24F;', 'float elements[] = { 7, 0x1.000002p+24F };',
            'struct pair { int count; float scale; } pair = { 16777219, 0x1.000002p+24F };',
            'union either { float as_float; int as_int; } either = { 0x1.000002p+24F };',
            'struct pair pairs[2] = { 1, 2, 3, 0x1.000002p+24F };',
            'Point point = { { 1, 2, 0x1.000002p+24F }, 16777219 };',
            'struct held { union either either; float scale; } held = { 0x1.000002p+24F, 0x1.000002p+24F };'])

    def test_narrows_a_long_double_into_a_float_as_shc_converts_it(self):
        # where SHC holds a long double as binary64, R32 truncates 0.1L, and R10 rounds it to the nearest, a step
        # above, but each truncates it into a float, 0x3dcccccc, where GCC rounds R32's to the nearest, 0x3dcccccd
        for route in ('r32', 'r10'):
            with self.subTest(route):
                self.assertEqual(initialized('float tenth = 0.1L;\n', ROUTES[route]), 0x3DCCCCCC)

    def test_fails_where_it_can_t_settle_the_type_an_integer_or_a_long_double_converts_to(self):
        # a struct of no definition, a bit-field, which Types doesn't lay out, and a designator, where an address
        # and a float pass, as GCC converts neither otherwise; and an integer the fold doesn't read
        c = ('struct far near = { 1 };\nstruct bits { int low : 3; float scale; } bits = { 1, 2.0F };\n'
             'struct pair { int count; float scale; } pair = { .scale = 16777219 };\n'
             'struct far other = { 0.1L, &near, 0.5F };\nstruct far sized = { sizeof(int) + 1 };\n')
        self.assertEqual(folded(c)[1], [
            "a.c:1: 1: an integer, which converts to a type the fold can't settle: struct far: no definition",
            "a.c:2: 1: an integer, which converts to a type the fold can't settle: struct bits: low is a bit-field",
            "a.c:3: 16777219: an integer, which converts to a type the fold can't settle: a designator, which the fold "
            "doesn't follow",
            "a.c:4: 0x1.999998p-4L: a long double, which converts to a type the fold can't settle: struct far: no "
            "definition",
            "a.c:5: sizeof(int) + 1: sizeof, which converts to a type the fold can't settle: struct far: no definition",
        ])

    def test_fails_on_a_decimal_constant_int_can_t_hold_but_one_the_fold_writes_or_gcc_converts_as_shc_does(self):
        # C17 makes 3000000000L and 2147483648 long longs, where SHC makes them unsigned, so -2147483648 is -2**31
        # under GCC and 2**31 under SHC, and half and t 0xc0000000 under GCC. The fold writes far's value, and GCC
        # converts big's and s's 3000000000 to an unsigned as SHC does; a U suffix and a hexadecimal constant type
        # alike under both, int holds 2147483647, and a system header's code keeps its own
        c = ('unsigned big = 3000000000, half = -2147483648 / 2;\nfloat far = -2147483648;\n'
             'unsigned f(int i, unsigned u)\n{\n    static unsigned s = 3000000000, t = -2147483648 / 2;\n'
             '    u = s + t + 3000000000U + 0x80000000 + 2147483647;\n'
             '    if (i < 3000000000L)\n        u = -2147483648;\n'
             '    return u;\n}\n# 1 "tools/sh4zam/include/sh4zam/shz_trig.h" 1 3\n'
             'static unsigned g(unsigned u) { return u * 3000000000; }\n')
        why = "a decimal constant that int can't hold, which C17 makes a long long, where SHC makes it unsigned"
        self.assertEqual(folded(c), (None, [
            f'a.c:1: 2147483648: {why}', f'a.c:5: 2147483648: {why}', f'a.c:7: 3000000000L: {why}',
            f'a.c:8: 2147483648: {why}']))
        c = 'unsigned big = 3000000000;\nfloat far = -2147483648;\n'
        self.assertEqual(folded(c), (f'# 1 "a.c"\n{c.replace("far = -2147483648", "far = 0x1p+31F")}', []))


class Reciprocals(unittest.TestCase):
    # under -aggressive=2, SHC multiplies a floating value that isn't constant by a constant divisor's reciprocal,
    # truncated: path_curve_sample multiplies by 0x3e2aaaaa, SHC's 1.0F / 6.0F, at 0x8c09b0a4 in 1ST_READ.BIN, where
    # curve.c's shipped weight[0] divides by 6.0F. A probe of R32 under the game's flags multiplies so where the
    # divisor is an int, a product of constants or negated, and folds the reciprocal into the run's constants, so
    # x * 2.0F / 3.0F multiplies by 0x3f2aaaaa; but it folds 5.0F / 3.0F, in code and in a static initializer, as the
    # division, 0x3fd55555, where the reciprocal gives 0x3fd55554

    def test_fails_on_a_division_by_a_constant_whose_reciprocal_no_normal_float_holds_exactly(self):
        # as a /=, in a call's argument, a statement expression, an automatic object's braces and a conditional; by
        # sizeof, whose value the fold doesn't read; and in C Reader doesn't read. Without -aggressive=2 each passes
        c = ('void g(float, float);\nfloat f(float x, float *p, int i)\n{\n    x = x / 6.0F;\n    x = x / 6;\n'
             '    x = x / (2.0F * 3.0F);\n    x = x * 2.0F / 3.0F;\n    *p /= -6.0F;\n'
             '    g(x / 6.0F, ({ float t = x / 6.0F; t; }));\n    {\n        float z[2] = { x / 6.0F, 0.0F };\n    }\n'
             '    x = x / sizeof(x);\n    __typeof__(x) w = x / 6.0F;\n    return i ? x / 6.0F : x;\n}\n')
        why = ('a division by a constant whose reciprocal no normal float holds exactly, which -aggressive=2 makes '
               'SHC multiply by')
        sixth = f'x / 0x1.8p+2F: {why}'
        self.assertEqual(folded(c, AGGRESSIVE), (None, [
            f'a.c:4: {sixth}', f'a.c:5: x / 6: {why}', f'a.c:6: x / (0x1p+1F * 0x1.8p+1F): {why}',
            f'a.c:7: x * 0x1p+1F / 0x1.8p+1F: {why}', f'a.c:8: *p /= -0x1.8p+2F: {why}', f'a.c:9: {sixth}',
            f'a.c:9: {sixth}', f'a.c:11: {sixth}',
            "a.c:13: x / sizeof(x): a division by a constant the tool can't read, under -aggressive=2: sizeof",
            "a.c:14: w = x / 0x1.8p+2F: C Reader doesn't read, which holds a division", f'a.c:15: {sixth}']))
        self.assertEqual(folded(c)[1], [])
        with tree({'a.i': '# 1 "a.c"\nfloat f(float x) { return x / 6.0F; }\n'}) as root:
            with self.assertRaisesRegex(SystemExit, r'\A[^\n]*: a\.c:1: x / 0x1\.8p\+2F: a division by a constant'):
                floats.main(['--shc', 'r32', '-fpu=single', '-aggressive=2', 'a.i'])
            floats.main(['--shc', 'r32', '-fpu=single', 'a.i'])
            self.assertEqual((root / 'a.i').read_text(), '# 1 "a.c"\nfloat f(float x) { return x / 0x1.8p+2F; }\n')

    def test_leaves_a_division_by_a_power_of_two_of_a_constant_or_of_integers(self):
        # and a division by a value that isn't constant, which SHC divides by; and folds a static object's quotient
        # as SHC does, as the division
        c = ('float f(float x, int i)\n{\n'
             '    x = x / 2.0F + x * (1.0F / 6.0F) + (5.0F / 3.0F) * x + 1.0F / x + x / 0.25F;\n'
             '    i = i / 6 + (int)(sizeof(x) / sizeof(x));\n    return x;\n}\nfloat quotient = 5.0F / 3.0F;\n')
        self.assertEqual(initialized(c, AGGRESSIVE), 0x3FD55555)


if __name__ == '__main__':
    unittest.main()
