import unittest

import float_scope
import float_tokens
from float_expr import Reader
from layout import LayoutError, Types
from test_float_fold import SYSTEM


def read(c):
    """The Scopes of c, a.c's C as GCC's preprocessor writes it, with SYSTEM's headers as system headers, and its
    tokens."""
    toks = float_tokens.marked(f'# 1 "a.c"\n{c}', 'a.i', SYSTEM)
    types = Types([])
    return float_scope.Scopes(toks, types, types.take([token.text for token in toks])), toks


def uses(toks, name):
    """The index of each token of toks that is name, by its line."""
    return [(toks.line(at), at) for at, token in enumerate(toks) if token.text == name]


class Lookup(unittest.TestCase):
    def test_binds_a_name_past_its_declarator_and_an_enumerator_past_its_value_to_where_its_block_closes(self):
        # B's value names the outer A, as the inner A's own enumerator and value do, and C's the inner A, whose scope
        # closes with f's body. Each binding opens on the line its enumerator ends on
        c = ('enum { A = 1 };\nvoid f(void)\n{\n    enum {\n        B = A,\n        A = A + 1,\n        C = A\n'
             '    };\n}\nint after = A;\n')
        scopes, toks = read(c)
        found = [(line, scopes.lookup('A', at)) for line, at in uses(toks, 'A')[1:]]
        self.assertEqual([(line, toks.line(binding.start), binding.kind) for line, binding in found], [
            (5, 1, 'enumerator'), (6, 1, 'enumerator'), (6, 1, 'enumerator'), (7, 6, 'enumerator'),
            (10, 1, 'enumerator')])

    def test_reads_an_object_where_it_hides_an_enumerator_or_a_typedef(self):
        # a parameter K and a local F hide the enumerator and the typedef, F from past its declarator, where the
        # typedef still names it; a member K binds no name; and past f, F is the typedef again
        c = ('typedef float F;\nenum { K = 2 };\nstruct s { float K; };\nvoid f(float K)\n{\n    int F = K;\n'
             '    F = K;\n}\nF g;\n')
        scopes, toks = read(c)
        found = [(name, line, scopes.lookup(name, at).kind, scopes.typedef_at(name, at))
                 for name in ('K', 'F') for line, at in uses(toks, name) if line in (3, 6, 7, 9)]
        self.assertEqual(found, [('K', 3, 'enumerator', False), ('K', 6, 'object', False), ('K', 7, 'object', False),
                                 ('F', 6, 'typedef', True), ('F', 7, 'object', False), ('F', 9, 'typedef', True)])

    def test_fails_where_no_declaration_names_a_name_or_one_the_walk_can_t_read_can_hide_it(self):
        # a declaration of a type Types doesn't read can hide any outer x from it on
        scopes, toks = read('float x;\nvoid f(void)\n{\n    y;\n    const __typeof__(x) z;\n    x;\n}\n')
        (_, y), = uses(toks, 'y')
        with self.assertRaisesRegex(LayoutError, r'^y, which no declaration in scope names$'):
            scopes.lookup('y', y)
        _, _, (_, x) = uses(toks, 'x')
        with self.assertRaisesRegex(LayoutError, r'^x, in the scope of a declaration the tool can\'t read: '):
            scopes.lookup('x', x)

    def test_binds_no_name_of_a_declaration_it_reads_no_end_of(self):
        # Types reads the attribute after struct as a declarator, aligned; what follows shows the walk misread it,
        # so aligned binds nothing, and the struct's braces are no function's body
        scopes, toks = read('typedef struct __attribute__((aligned(8))) pair { float x; } pair_t;\nint aligned;\n')
        self.assertEqual(scopes.bodies, [])
        _, (_, after) = uses(toks, 'aligned')
        self.assertEqual(scopes.binding('aligned', after), None)
        self.assertEqual(scopes.lookup('aligned', len(toks) - 1).kind, 'object')


class Constant(unittest.TestCase):
    def test_reads_an_operation_as_no_constant_where_an_operand_shows_it_whatever_the_others_name(self):
        # the object y settles z * y and y + z, though no declaration names z; z * 2 it can't tell; and a builtin's
        # name names a function
        scopes, toks = read('void f(float y)\n{\n    z * y;\n    y + z;\n    z * 2;\n    __builtin_fabsf(y);\n}\n')
        nodes = [Reader(toks, scopes.types, scopes.typedef_at, start, end).full() for start, end in scopes.expressions]
        self.assertEqual([scopes.constant(node) for node in (nodes[0], nodes[1], nodes[3].operands[0])],
                         [False, False, False])
        with self.assertRaisesRegex(LayoutError, r'^z, which no declaration in scope names$'):
            scopes.constant(nodes[2])


class Type(unittest.TestCase):
    def test_reads_parentheses_around_an_assignment_as_its_left_operand_but_not_with_a_comma_after_it(self):
        # (y = 1) has y's type, and (i = 0, y) has y's too, not i's, which the tool doesn't read
        scopes, toks = read('void f(float y, int i)\n{\n    (y = 1) * 2;\n    (i = 0, y) * 2;\n}\n')
        nodes = [Reader(toks, scopes.types, scopes.typedef_at, start, end).full() for start, end in scopes.expressions]
        self.assertTrue(scopes.floating(nodes[0]))
        with self.assertRaisesRegex(LayoutError, r'^\(i = 0, y\), C whose type the tool doesn\'t read$'):
            scopes.floating(nodes[1])


class Expressions(unittest.TestCase):
    def test_gives_each_full_expression_of_a_function_s_statements(self):
        # an automatic object's initializer, each in its braces, an if's head and the statements after it and after
        # its else, each of a for's heads, a while's and a switch's, an asm statement's operands, a return's and a
        # statement expression's own; but a static object's initializer, a case label's and a goto's label
        c = ('int f(int a, int b)\n{\n    static int s = 1;\n    int c = a + 1, d[2] = { a, b };\n'
             '    if (a) b = 2; else b = 3;\n    for (c = 0; c < 2; c++) d[c] = c;\n'
             '    while (a--) switch (b) { case 1: goto out; default: break; }\n'
             '    __asm__ volatile ("" : "=r"(a) : "r"(b + 1));\nout:\n    return ({ int t = a; t + 1; });\n}\n')
        scopes, toks = read(c)
        self.assertEqual([float_tokens.quoted(toks[start:end]) for start, end in scopes.expressions], [
            'a + 1', 'a', 'b', 'a', 'b = 2', 'b = 3', 'c = 0', 'c < 2', 'c++', 'd[c] = c', 'a--', 'b', 'a', 'b + 1',
            '({ int t = a; t + 1; })', 'a', 't + 1'])


class Statics(unittest.TestCase):
    def test_gives_each_initializer_of_an_object_of_static_storage_on_the_tree_s_lines(self):
        # at file scope and in a block where the declaration says static, but an automatic object's and a system
        # header's; and where Types can't read the declaration, each initializer of the rest, which takes no type
        c = ('float a = 1.0F, b = 2.0F;\nvoid f(void)\n{\n    static float c = 3.0F;\n    float d = 4.0F;\n'
             '    static __typeof__(a) e = 5.0F;\n}\n# 1 "tools/sh4zam/include/sh4zam/shz_trig.h" 1 3\n'
             'static float g = 6.0F;\n')
        scopes, toks = read(c)
        found = [(toks[start].text, end - start, term is not None, why is not None)
                 for start, end, term, why in scopes.statics]
        self.assertEqual(found, [('1.0F', 1, True, False), ('2.0F', 1, True, False), ('3.0F', 1, True, False),
                                 ('5.0F', 1, False, True)])


if __name__ == '__main__':
    unittest.main()
