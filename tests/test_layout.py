import re
import unittest

from layout import Array, LayoutError, Pointer, Record, Scalar, Types, number, spelled


XPT = '''typedef unsigned char Uint8;
typedef signed char Sint8;
typedef unsigned short Uint16;
typedef short Sint16;
typedef unsigned long Uint32;
typedef long Sint32;
typedef float Float;
enum bool { false, true };
typedef enum bool Bool;
'''


def types(*texts):
    return Types([XPT, *texts])


def offsets(layout):
    return [(name, offset) for name, _, offset in layout.fields]


def member(layout, name):
    return next(field for each, field, _ in layout.fields if each == name)


class Layouts(unittest.TestCase):
    def test_scalars_take_shcs_sizes(self):
        found = types()
        for ctype, code in (('char', 'c'), ('signed char', 'b'), ('Uint8', 'B'), ('short', 'h'), ('Uint16', 'H'),
                            ('int', 'i'), ('unsigned', 'I'), ('long', 'i'), ('Uint32', 'I'), ('Float', 'f'),
                            ('Bool', 'i'), ('enum bool', 'i')):
            with self.subTest(ctype=ctype):
                self.assertEqual(found.layout(ctype), Scalar(code))

    def test_a_struct_aligns_each_member_and_pads_its_end(self):
        layout = types('struct a { Uint8 b; Sint16 h; Uint8 c; Sint32 w; Sint8 e; };').layout('struct a')
        self.assertEqual(offsets(layout), [('b', 0), ('h', 2), ('c', 4), ('w', 8), ('e', 12)])
        self.assertEqual((layout.size, layout.alignment, layout.holes), (16, 4, ((1, 1), (5, 3), (13, 3))))

    def test_one_declaration_declares_each_member(self):
        layout = types('struct a { Float x, y; Uint8 *p, m[3]; };').layout('struct a')
        self.assertEqual(offsets(layout), [('x', 0), ('y', 4), ('p', 8), ('m', 12)])
        self.assertEqual(member(layout, 'p'), Pointer('Uint8'))
        self.assertEqual(member(layout, 'm'), Array(Scalar('B'), 3))

    def test_a_union_holds_its_largest_member_and_writes_its_first(self):
        layout = types('union u { Uint8 b; Uint32 w; Sint16 h[3]; };').layout('union u')
        self.assertTrue(layout.union)
        self.assertEqual((layout.size, layout.holes), (8, ((1, 7),)))

    def test_typedefs_of_records_arrays_and_pointers(self):
        found = types('typedef struct { Float x, y; } P2, *P2P;\ntypedef Float TABLE[4][2];\n'
                      'typedef struct node { struct node *next; P2 at; } NODE;')
        self.assertEqual(found.layout('P2').size, 8)
        # an unnamed struct has no name a cast can take
        self.assertEqual(found.layout('P2P'), Pointer(None))
        self.assertEqual(found.layout('TABLE'), Array(Array(Scalar('f'), 2), 4))
        self.assertEqual(offsets(found.layout('NODE')), [('next', 0), ('at', 4)])
        self.assertEqual(member(found.layout('NODE'), 'next'), Pointer('struct node'))

    def test_a_pointer_names_its_pointee_as_a_cast_would(self):
        layout = types('struct t; struct a { const Uint16 (*rows)[4][2]; struct t **list; void (*f)(int); '
                       'const Uint8 *bytes; };').layout('struct a')
        self.assertEqual([field for _, field, _ in layout.fields],
                         [Pointer('const Uint16[4][2]'), Pointer('struct t *'), Pointer(None), Pointer('const Uint8')])

    def test_a_record_inside_a_record(self):
        layout = types('struct a { Uint8 b; struct { Sint16 h; union { Float f; Uint8 c; } u; } in; };').layout(
            'struct a')
        self.assertEqual(offsets(layout), [('b', 0), ('in', 4)])
        self.assertIsInstance(member(layout, 'in'), Record)
        self.assertEqual(layout.size, 12)

    def test_two_types_share_no_definition(self):
        self.assertEqual(types('struct a { Sint32 w; Uint8 b; };').layout('struct a').size, 8)
        self.assertEqual(types('struct a { Uint8 b; };').layout('struct a').size, 1)

    def test_comments_strings_and_directives_hide_no_declaration(self):
        found = types('/* struct a { int x; }; */\nchar *s = "struct a { char x; }";\n#if 0\n#endif\n'
                      '// struct a { short x; }; \\\nstruct a { long x; };\nstruct a { Float f; };')
        self.assertEqual(member(found.layout('struct a'), 'f'), Scalar('f'))

    def test_a_comment_inside_a_macro_joins_its_lines(self):
        # a comment's line break ends no directive, spliced or not
        for macro in ('#define N \\\n    /* a comment of \\\n     * two lines */ \\\n    (1 + 2)\n',
                      '#define N (1 + /* a comment of\n     * two lines */ 2)\n'):
            with self.subTest(macro=macro):
                self.assertEqual(types(macro + 'struct a { char x[N]; };').layout('struct a').size, 3)

    def test_what_has_no_layout_fails_naming_why(self):
        found = types('struct bits { Uint32 flag : 1; };\nstruct d { double x; };\nstruct a { int x[]; };\n'
                      'typedef void FUNCTION(void);\n#define N (1 << 31)\nstruct s { char a[N]; };\n'
                      'struct n { char a[-1]; };\ntypedef LOOP2 LOOP1;\ntypedef LOOP1 LOOP2;\n'
                      'struct self { struct self x; };\ntypedef ROWS TABLE[2];\ntypedef TABLE ROWS;')
        for ctype, reason in (('struct bits', 'flag is a bit-field'), ('struct d', 'double: no layout'),
                              ('struct a', r'int\[\]: no extent'), ('FUNCTION', 'a function'),
                              ('struct s', r'\[N\]: 1 << 31: 2147483648 lies outside int'),
                              ('struct n', r'\[- 1\]: a negative extent'),
                              ('struct none', 'struct none: no definition'), ('NOTHING', 'NOTHING: no type'),
                              ('LOOP1', 'LOOP1: leads back to itself'), ('struct self', 'struct self: holds itself'),
                              ('TABLE', 'ROWS: holds itself'), ('void', 'void: no layout'),
                              ('struct c { int x; }', 'a type name with a body')):
            with self.subTest(ctype=ctype), self.assertRaisesRegex(LayoutError, reason):
                found.layout(ctype)

    def test_a_tag_defined_two_ways_fails(self):
        found = types('struct a { Sint32 w; };', 'struct a { Uint8 b; };')
        with self.assertRaisesRegex(LayoutError, 'struct a: defined two ways'):
            found.layout('struct a')

    def test_a_member_two_branches_declare_fails(self):
        found = types('struct g2 {\n#ifdef GCC\n    volatile Uint32 device;\n#else\n    Uint32 device;\n#endif\n};')
        with self.assertRaisesRegex(LayoutError, '^struct g2: two members named device$'):
            found.layout('struct g2')

    def test_a_typedef_defined_two_ways_fails(self):
        with self.assertRaisesRegex(LayoutError, '^T: defined two ways$'):
            types('typedef short T;', 'typedef long T;').layout('T')


class Constants(unittest.TestCase):
    def test_extents_take_macros_and_enumerators(self):
        found = types('#define ROWS 3\n#define COLS (ROWS * 2 + 1)\nenum { RED, GREEN = 4, BLUE, COUNT };\n'
                      'struct a { Uint8 grid[ROWS][COLS]; Sint16 colors[COUNT]; };')
        self.assertEqual(member(found.layout('struct a'), 'grid'), Array(Array(Scalar('B'), 7), 3))
        self.assertEqual(found.layout('struct a').size, 21 + 1 + 12)
        self.assertEqual(found.evaluate(['BLUE']), 5)

    def test_a_macro_or_an_enumerator_defined_two_ways_fails(self):
        found = types('#define N 1\nenum { E = 1 };', '#define N 2\nenum { E = 2 };')
        for name in ('N', 'E'):
            with self.subTest(name=name), self.assertRaisesRegex(LayoutError, f'^{name}: defined two ways$'):
                found.evaluate([name])

    def test_an_enumerator_given_one_value_in_two_enums_is_defined_once(self):
        self.assertEqual(types('enum first { P, N = 4 };', 'enum other { N = 4 };').evaluate(['N']), 4)

    def test_a_macro_expands_as_text(self):
        # as the preprocessor does, without parentheses of its own
        self.assertEqual(types('#define SUM 1 + 2').evaluate(['SUM', '*', '3']), 7)

    def test_c_arithmetic(self):
        # each as SHC R10 and R32 give it in an extent
        found = types()
        for text, value in (('-7 / 2', -3), ('-7 % 2', -1), ('7 % -2', 1), ('1 << 4 | 1', 17), ('~0 & 0xff', 255),
                            ('(2 + 3) * 4', 20), ('6 - 2 - 1', 3), ('1 < 2 && 2 < 3', 1), ('-16 >> 2', -4),
                            ('-3 >> 1', -2), ('-3 << 2', -12)):
            with self.subTest(text=text):
                self.assertEqual(found.evaluate(spelled(text)), value)

    def test_unsigned_and_long_values_that_fit_their_types(self):
        # each as SHC R10 and R32 give it in an extent
        found = types()
        for text, value in (('12U + 40UL', 52), ('0x80000000U >> 28', 8), ('0xFFFFFFFF >> 28', 15),
                            ('3000000000 / 1000000000', 3), ('1 < 2U', 1), ('-1 && 1U', 1), ('5U - 3', 2),
                            ('-1L < 1U', 1), ('1 < 0xffffffffUL', 1), ('0x80000000L >> 31', 1), ('-1 >> 1U', -1),
                            ('(1 < 2U) - 2', -1), ('!0U - 2', -1)):
            with self.subTest(text=text):
                self.assertEqual(found.evaluate(spelled(text)), value)

    def test_unsigned_values_wrap_as_c_s_do(self):
        # each as SHC R10, R26 and R32 give it in an extent or a static
        # initializer. An unsigned value wraps modulo 2**32, and so does an
        # operand that converts to unsigned
        found = types()
        for text, value in (('~0U >> 31', 1), ('(~0U >> 31) + 1', 2), ('0xffffffffU + 1U', 0),
                            ('0xffffffffU + 2U', 1), ('-1 < 1U', 0), ('(1 - 2U) >> 28', 15),
                            ('-2147483648', 0x80000000), ('-2147483648 >> 30', 2)):
            with self.subTest(text=text):
                self.assertEqual(found.evaluate(spelled(text)), value)

    def test_integer_constants_take_shcs_types(self):
        self.assertEqual([number(text) for text in ('0', '017', '0x1F', '12U', '40UL', '10L', '0x80000000',
                                                    '3000000000', '0x80000000L')],
                         [(0, 'int'), (15, 'int'), (31, 'int'), (12, 'unsigned int'), (40, 'unsigned long'),
                          (10, 'long'), (0x80000000, 'unsigned int'), (3000000000, 'unsigned int'),
                          (0x80000000, 'unsigned long')])
        for text, reason in (('1.5F', '1.5F: no integer constant'), ('10LL', '10LL: no integer constant'),
                             ('0x100000000', '0x100000000: wider than 32 bits')):
            with self.subTest(text=text), self.assertRaisesRegex(LayoutError, reason):
                number(text)

    def test_what_is_no_constant_fails(self):
        found = types('#define HALF(x) ((x) / 2)\n#define LOOP LOOP\n')
        for tokens, reason in ((['HALF', '(', '4', ')'], 'HALF: a function-like macro'), (['LOOP'], 'LOOP: no'),
                               (['1', '/', '0'], '1 / 0: a division by zero'), (['sizeof', 'x'], 'sizeof')):
            with self.subTest(tokens=tokens), self.assertRaisesRegex(LayoutError, reason):
                found.evaluate(tokens)

    def test_a_value_shc_gives_otherwise_fails_naming_its_operation(self):
        # SHC R10 and R32 give each one a value unbounded integers don't, as
        # they wrap, convert or shift
        found = types('enum { TOP = 0x7fffffff, OVER };')
        for text, reason in (('1L < 0xffffffff', '1L < 0xffffffff: 4294967295 lies outside long'),
                             ('0x7fffffff + 1', '0x7fffffff + 1: 2147483648 lies outside int'),
                             ('1 << 31', '1 << 31: 2147483648 lies outside int'),
                             ('OVER', 'OVER: 2147483648 lies outside int'),
                             ('1 << 32', '1 << 32: a shift by 32, outside 0 to 31'),
                             ('1 << -1', '1 << - 1: a shift by -1, outside 0 to 31')):
            with self.subTest(text=text), self.assertRaisesRegex(LayoutError, re.escape(reason)):
                found.evaluate(spelled(text))

    def test_what_shc_gives_no_value_fails(self):
        # SHC R10 and R32 refuse each, and refuse a division by zero even on
        # the side of && or || that C skips
        found = types()
        for text, reason in (('0 && (1 / 0)', '1 / 0: a division by zero'),
                             ('1 || (1 % 0)', '1 % 0: a division by zero'),
                             ('(-2147483647 - 1) / -1', '( - 2147483647 - 1 ) / - 1: 2147483648 lies outside int'),
                             ('(-2147483647 - 1) % -1',
                              '( - 2147483647 - 1 ) % - 1: its quotient 2147483648 lies outside int'),
                             ('10LL', '10LL: no integer constant'),
                             ('0x100000000', '0x100000000: wider than 32 bits')):
            with self.subTest(text=text), self.assertRaisesRegex(LayoutError, re.escape(reason)):
                found.evaluate(spelled(text))

    def test_both_sides_of_and_are_evaluated(self):
        # SHC R10 and R32 give this 0, skipping its shift. Types evaluates the
        # shift, and fails
        with self.assertRaisesRegex(LayoutError, re.escape('1 << 32: a shift by 32, outside 0 to 31')):
            types().evaluate(spelled('0 && (1 << 32)'))

    def test_an_enum_of_thousands_counts_on(self):
        # past Python's recursion limit
        found = types('enum many { ' + ', '.join(f'E{index}' for index in range(5000)) + ' };')
        self.assertEqual(found.evaluate(['E4999']), 4999)
