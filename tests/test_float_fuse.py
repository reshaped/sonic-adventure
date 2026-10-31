import re
import unittest

import floats
from support import tree
from test_float_fold import folded
from test_float_value import GAME, R10, R32, ROUTES
from test_floats import returned, spelled


def wrapped(product):
    """product as the tool wraps it."""
    return f'({{ __typeof__({product}) __floats_p = {product}; __floats_p; }})'


def captured(left, right):
    """left += right as the tool writes it as an addition where left is no name."""
    return f'({{ __typeof__({left}) *__floats_l = &({left}); *__floats_l = *__floats_l + ({right}); }})'


def squeezed(text):
    """text on one line, each run of whitespace a space, and none before a ) or a ;, where the whitespace between an
    edit's tokens can leave one."""
    return re.sub(r' (?=[);])', '', ' '.join(text.split()))


class Wraps(unittest.TestCase):
    # each floating product that R26 and R32 don't fuse into its run of + and -, which GCC fuses by FMAC under the
    # -ffp-contract=on of an object they build, as probes of each case under R32 with the game's flags show

    def test_wraps_each_floating_product_shc_subtracts_or_subtracts_from(self):
        # a binary -'s operand, on either side and both of a cross term, and a unary -'s, through parentheses, a unary +
        # and a cast to a floating type; the whole of a run of *, each product inside another, and one beside an int,
        # which C converts, or a negated one; one after negative operands alone, one in a sum a minus negates, one a
        # negated factor makes negative, one ahead of a negative name or of a run of integers C subtracts whole, and
        # one ahead of a minus a constant follows; a constant one, which SHC folds; and each of a constant run, alone or
        # as an operand, which SHC folds whole
        for expression, written in (('y - x * y', f'y - {wrapped("x * y")}'),
                                    ('x * y - y', f'{wrapped("x * y")} - y'),
                                    ('x * y - p[0] * p[1]', f'{wrapped("x * y")} - {wrapped("p[0] * p[1]")}'),
                                    ('-(x * y) + y', f'-({wrapped("x * y")}) + y'),
                                    ('y - +(float)(x * y)', f'y - +(float)({wrapped("x * y")})'),
                                    ('y - x * y * y', f'y - {wrapped("x * y * y")}'),
                                    ('y - x * (y - p[0] * p[1])',
                                     f'y - {wrapped("x * (y - " + wrapped("p[0] * p[1]") + ")")}'),
                                    ('i - x * i', f'i - {wrapped("x * i")}'),
                                    ('y - (-i) * y', f'y - {wrapped("(-i) * y")}'),
                                    ('-p[0] - y + x * y', f'-p[0] - y + {wrapped("x * y")}'),
                                    ('y - (x * y + p[0])', f'y - ({wrapped("x * y")} + p[0])'),
                                    ('y + (-x) * y', f'y + {wrapped("(-x) * y")}'),
                                    ('x * y + -y', f'{wrapped("x * y")} + -y'),
                                    ('x * y - (1 - i)', f'{wrapped("x * y")} - (1 - i)'),
                                    ('x * y - y - 0.5F', f'{wrapped("x * y")} - y - 0x1p-1F'),
                                    ('y + 0.1F * 3.0F', f'y + {wrapped("0x1.999998p-4F * 0x1.8p+1F")}'),
                                    ('0.1F * 0.3F - 0.03F',
                                     f'{wrapped("0x1.999998p-4F * 0x1.333332p-2F")} - 0x1.eb851ep-6F'),
                                    ('y - (0.1F * 0.3F - 0.03F)',
                                     f'y - ({wrapped("0x1.999998p-4F * 0x1.333332p-2F")} - 0x1.eb851ep-6F)')):
            with self.subTest(expression):
                text, problems = returned(expression)
                self.assertEqual((squeezed(text), problems), (written, []))
        # the right operand of a -=, in a call's argument too, and of a +=, where -s negate its parts or it is
        # constant, and a product in an automatic object's initializer, under R26 as under R32
        c = ('void g(float);\nvoid f(float *x, float y)\n{\n    float z = y - y * y;\n    *x -= y * y;\n'
             '    g(*x -= y * 2.0F);\n    *x += -y * y;\n    *x += 0.1F * 3.0F;\n    *x += y * y;\n}\n')
        for route in ('game', 'r26'):
            with self.subTest(route):
                text, problems = folded(c, ROUTES[route])
                self.assertEqual([squeezed(line) for line in text.splitlines()[4:10]],
                                 [f'float z = y - {wrapped("y * y")};', f'*x -= {wrapped("y * y")};',
                                  f'g(*x -= {wrapped("y * 0x1p+1F")});', f'*x += {wrapped("-y * y")};',
                                  f'*x += {wrapped("0x1.999998p-4F * 0x1.8p+1F")};', '*x += y * y;'])
                self.assertEqual(problems, [])

    def test_leaves_a_product_shc_adds_an_integer_one_and_one_on_a_route_that_fuses_none(self):
        # a + takes the product after an operand it adds or ahead of one, a second minus, through a cast to a floating
        # type too, a unary - or a negated factor turns it back, as a negation of integers doesn't, or the run's
        # constants alone stand beside it, which SHC adds; C converts it to an integer or a conditional chooses it; it
        # multiplies integers; or R10 builds it
        for route, expression in ((GAME, 'y + x * y'), (GAME, 'x * y + y'), (GAME, 'y - p[0] + x * y'),
                                  (GAME, 'x * y + p[0] - y'), (GAME, 'y - (p[0] - x * y)'),
                                  (GAME, 'y - (float)(p[0] - x * y)'), (GAME, 'y - -(x * y)'),
                                  (GAME, 'y - (-x) * y'), (GAME, 'y - x * -3.0F'), (GAME, 'x * y + -(1 - i)'),
                                  (GAME, 'x * y - 0.5F'), (GAME, 'x * y - -0.5F'), (GAME, '-(0.5F - x * y)'),
                                  (GAME, 'y - (int)(x * y)'), (GAME, 'y - (i ? x * y : y)'), (GAME, 'i - i * i'),
                                  (GAME, 'u - (u * 2)'), (GAME, '-(i * 2) + u'), (R10, 'y - x * y'),
                                  (ROUTES['r10 -double=float'], '-(x * y)')):
            with self.subTest(expression):
                self.assertEqual(returned(expression, route), (spelled(expression, route), []))
        # R10's += and -=
        c = 'void f(float *x, float y)\n{\n    *x += -y * y;\n    *x -= y * y;\n}\n'
        self.assertEqual(folded(c, R10), (f'# 1 "a.c"\n{c}', []))
        # a static initializer, which the fold settles, a type at file scope, an array's length in a block, and a
        # system header's code
        c = ('float y;\n__typeof__(y - y * y) v;\nfloat f(float x)\n{\n    static float s = 1.0F - 0.5F * 0.5F;\n'
             '    float u[(int)(8 - x * x)];\n    return s;\n}\n'
             '# 1 "tools/sh4zam/include/sh4zam/shz_scalar.h" 1 3\nstatic float g(float x) { return 1.0f - x * x; }\n')
        self.assertEqual([squeezed(line) for line in folded(c)[0].splitlines()[1:]],
                         ['float y;', '__typeof__(y - y * y) v;', 'float f(float x)', '{',
                          'static float s = 0x1.8p-1F;', 'float u[(int)(8 - x * x)];', 'return s;', '}',
                          '# 1 "tools/sh4zam/include/sh4zam/shz_scalar.h" 1 3',
                          'static float g(float x) { return 1.0f - x * x; }'])

    def test_reads_each_operand_s_type_from_the_declarations_in_scope(self):
        # a bit-field and an enumerator are integers, a member of an unnamed struct and a call's value are floating,
        # a local count hides the global one, an inner block's hides that, a typedef's cast is floating, a parameter
        # is what its declaration says, and an old-style definition declares its parameters before its body
        c = ('typedef float Float;\nenum { COUNT = 3 };\n'
             'struct v { Float x; int n : 4; struct { Float w; }; } *list[2];\n'
             'Float scale(int);\nint count;\nvoid f(struct v *s, int n)\n{\n    Float count = 2.0F;\n'
             '    s->x = s->x - s->n * COUNT;\n    s->x = s->x - list[0]->w * n;\n    s->x = s->x - scale(n) * n;\n'
             '    s->x = s->x - count * n;\n    {\n        int count = 1;\n        s->x = s->x - count * n;\n    }\n'
             '    s->x = s->x - (Float)n * n;\n    s->x = s->x - (*s).n * n;\n}\n'
             'int g(n, m)\nint n;\nfloat m;\n{\n    return n - m * n;\n}\n')
        text, problems = folded(c)
        lines = [squeezed(line) for line in text.splitlines()]
        self.assertEqual(lines[9:19], ['s->x = s->x - s->n * COUNT;', f's->x = s->x - {wrapped("list[0]->w * n")};',
                                       f's->x = s->x - {wrapped("scale(n) * n")};',
                                       f's->x = s->x - {wrapped("count * n")};', '{', 'int count = 1;',
                                       's->x = s->x - count * n;', '}', f's->x = s->x - {wrapped("(Float)n * n")};',
                                       's->x = s->x - (*s).n * n;'])
        self.assertEqual((lines[24], problems), (f'return n - {wrapped("m * n")};', []))

    def test_reads_each_name_as_the_innermost_declaration_in_scope_declares_it(self):
        # a parameter and a local K hide the enumerator K, so K * K is floating, and a local F hides the typedef F, so
        # (F) * y multiplies by it; past their scopes, K * K is an int and (F) * p casts *p
        c = ('typedef float F;\nenum { K = 2 };\nfloat f(float K, float y)\n{\n    return y - K * K;\n}\n'
             'float g(float y)\n{\n    float K = y, F = y;\n    return y - K * K - (F) * y;\n}\n'
             'float h(float y, float *p)\n{\n    return y - K * K - (F) * p;\n}\n')
        text, problems = folded(c)
        lines = [squeezed(line) for line in text.splitlines()]
        self.assertEqual([lines[5], lines[10], lines[14]], [
            f'return y - {wrapped("K * K")};', f'return y - {wrapped("K * K")} - {wrapped("(F) * y")};',
            'return y - K * K - (F) * p;'])
        self.assertEqual(problems, [])

    def test_fails_where_it_can_t_tell_whether_a_product_is_floating(self):
        # a name no declaration in scope names, beside an integer, and a call of no function, which leave the run's
        # type unread too; a product beside a floating operand is floating whatever the other's type
        for expression, why in (('i - z * i', 'z, which no declaration in scope names'),
                                ('i - i(2) * i', 'i(2), a call of no function')):
            with self.subTest(expression):
                product = expression[4:]
                self.assertEqual(returned(expression)[1], [
                    f'a.c:3: {expression}: C the tool can\'t tell is floating: {why}',
                    f'a.c:3: {product}: a product SHC doesn\'t fuse, whose type the tool can\'t tell: {why}'])
        self.assertEqual(returned('y - z * y'), (f'y - {wrapped("z * y")}', []))
        with tree({'a.i': '# 1 "a.c"\nint f(int i) { return i - z * i; }\n'}) as root:
            with self.assertRaisesRegex(SystemExit, r'\A[^\n]*: a\.c:1: i - z \* i: C the tool can\'t tell is '
                                                    r'floating: z, which no declaration in scope names\n'
                                                    r'[^\n]*: a\.c:1: z \* i: a product SHC doesn\'t fuse'):
                floats.main(['--shc', 'r32', '-fpu=single', 'a.i'])
            self.assertEqual((root / 'a.i').read_text(), '# 1 "a.c"\nint f(int i) { return i - z * i; }\n')

    def test_fails_where_it_can_t_tell_whether_an_operand_of_a_run_is_constant_or_floating(self):
        # in the scope of a declaration the walk can't read, which can hide the enumerator K and the int i: K is no
        # operand of the run beside a * b if it is constant, and -i is no negative one if i is an int
        for expression, what in (('a * b - K', 'constant'), ('a * b + -i', 'floating')):
            with self.subTest(expression):
                c = ('enum { K = 2 };\nfloat f(float x, int i)\n{\n    const __typeof__(x) z = x;\n'
                     f'    float a = x, b = x;\n    return {expression};\n}}\n')
                name = expression[-1]
                self.assertEqual(folded(c), (None, [
                    f"a.c:6: {name}: C the tool can't tell is {what}: {name}, in the scope of a declaration the tool "
                    "can't read: z after a declarator"]))

    def test_fails_on_a_product_to_wrap_beside_a_cast_it_doesn_t_read(self):
        # and wraps one beside a cast to float
        self.assertEqual(returned('y - (__typeof__(x)) y * y'), (None, [
            "a.c:3: (__typeof__(x)) y * y: a product SHC doesn't fuse, beside (__typeof__(x)) y, a cast to a type the "
            "tool doesn't read: __typeof__ ( x ): __typeof__, a word Types doesn't read"]))
        self.assertEqual(returned('y - (float) y * y'), (f'y - {wrapped("(float) y * y")}', []))


class Compounds(unittest.TestCase):
    # each += of a floating product that R26 and R32 fuse, which GCC doesn't where the product holds a side effect,
    # and each into an integer, which they don't, as probes of each case under -ffp-contract=on show
    C = ('typedef float Float;\nstruct v { Float x; Float a[4]; struct v *next; Float *vp; } *s, list[4], *f(void);\n'
         'struct w { short y; } *camera_work;\nstatic inline Float cosine(short n) { return n + n; }\n'
         'extern Float scale(Float);\nvolatile Float blue;\nvolatile struct v *body;\nint n;\n'
         'struct u { volatile Float m; Float *volatile q; } *pu;\nvolatile Float *pv, va[4];\n'
         'typedef volatile struct v Shaky;\nShaky *vv;\nFloat *volatile vp;\ntypedef Float *volatile Wobbly;\n'
         'Wobbly vw;\ndouble dd;\nextern double dcos(double);\n'
         'void g(Float y, struct v *p)\n{\n    {}\n}\n')

    def compounds(self, statements, route=GAME):
        """statements, in the body of g(Float y, struct v *p) beside C's declarations, as the tool writes them under
        route, the game's by default: each on one line, or None where the tool fails; and the problems."""
        text, problems = folded(self.C.replace('    {}\n', ''.join(f'    {line}\n' for line in statements)), route)
        if text is None:
            return None, problems
        lines = text.splitlines()
        first = lines.index('{') + 1
        return [squeezed(line) for line in lines[first:first + len(statements)]], problems

    def test_writes_a_plus_equals_of_a_product_holding_a_side_effect_as_an_addition(self):
        # camera_step_follow's drift, an inline function's call, and an external one's; a left operand of each
        # form the tool takes, as macros spell them too, after a brace and an if's, a for's, a while's and a switch's
        # head; a product holding a statement expression, the tool's wrap, ++, an assignment or a volatile object, a
        # pointer a declarator qualifies volatile too, and in parentheses, under a unary + and under a floating cast;
        # a += a call's argument holds; and a member that shares a volatile pointer's name
        written, problems = self.compounds([
            's->x += cosine(camera_work->y) * 0.2F;', 'y += scale(y) * y;', 'p->x += cosine(n) * y;',
            'list[n].x += cosine(n) * y;', 'p->a[2] += cosine(n) * y;', 'list[n].next->a[n] += cosine(n) * y;',
            '((struct v *)&list[n])->x += cosine(n) * y;', '{} ((*p)).x += cosine(n) * y;',
            'if (n) {} else { (p)->x += cosine(n) * y; }', 'do { *p->a += cosine(n) * y; } while (n);',
            'if (n) p->x += cosine(n) * y;', 'for (;;) p->x += cosine(n) * y;', 'while (n) p->x += cosine(n) * y;',
            'switch (n) { case 1: (p)->x += cosine(n) * y; }', 'y += ({ y; }) * y;', 'y += (y - p->x * y) * y;',
            'y += p->a[n++] * y;', 'y += (n = 2) * y;', 'y += blue * y;', 'y += vp[0] * y;',
            'p->x += (cosine(n) * y);', 'p->x += +(cosine(n) * y);', 'p->x += (Float)(cosine(n) * y);',
            'scale(p->x += cosine(n) * y);', 'p->vp[0] += cosine(n) * y;'])
        drift = 'cosine(n) * y'
        self.assertEqual(written, [
            f'{captured("s->x", "cosine(camera_work->y) * 0x1.999998p-3F")};', 'y = y + (scale(y) * y);',
            f'{captured("p->x", drift)};', f'{captured("list[n].x", drift)};', f'{captured("p->a[2]", drift)};',
            f'{captured("list[n].next->a[n]", drift)};', f'{captured("((struct v *)&list[n])->x", drift)};',
            f'{{}} {captured("((*p)).x", drift)};', f'if (n) {{}} else {{ {captured("(p)->x", drift)}; }}',
            f'do {{ {captured("*p->a", drift)}; }} while (n);', f'if (n) {captured("p->x", drift)};',
            f'for (;;) {captured("p->x", drift)};', f'while (n) {captured("p->x", drift)};',
            f'switch (n) {{ case 1: {captured("(p)->x", drift)}; }}', 'y = y + (({ y; }) * y);',
            f'y = y + ((y - {wrapped("p->x * y")}) * y);', 'y = y + (p->a[n++] * y);', 'y = y + ((n = 2) * y);',
            'y = y + (blue * y);', 'y = y + (vp[0] * y);', f'{captured("p->x", "(cosine(n) * y)")};',
            f'{captured("p->x", "+(cosine(n) * y)")};', f'{captured("p->x", "(Float)(cosine(n) * y)")};',
            f'scale({captured("p->x", drift)});', f'{captured("p->vp[0]", drift)};'])
        self.assertEqual(problems, [])

    def test_evaluates_a_left_operand_but_a_name_once_ahead_of_the_right(self):
        # a call that moves the pointer or the index the left operand reads, which a second reading would take as
        # another object, and a name, which names one object however the call runs
        c = ('extern float move(void);\nfloat slots[2], *aim, bias;\nint at;\n'
             'void f(float y)\n{\n    *aim += move() * y;\n    slots[at] += move() * y;\n    bias += move() * y;\n}\n')
        text, problems = folded(c)
        self.assertEqual([squeezed(line) for line in text.splitlines()[6:9]], [
            f'{captured("*aim", "move() * y")};', f'{captured("slots[at]", "move() * y")};',
            'bias = bias + (move() * y);'])
        self.assertEqual(problems, [])

    def test_fails_on_a_left_operand_it_doesn_t_take_beside_a_product_shc_fuses(self):
        # a call, an increment and a decrement, a subscript by an expression, an assignment, and a volatile object:
        # a name, a member, what a pointer points to, through a typedef, an array's element and a pointer a
        # declarator qualifies volatile, a typedef of one and a member
        statements = ['f()->x += cosine(n) * y;', 'p++->x += cosine(n) * y;', '(++p)->x += cosine(n) * y;',
                      '(--p)->x += cosine(n) * y;', 'p->a[n + 1] += cosine(n) * y;', '(p = s)->x += cosine(n) * y;',
                      'body->x += cosine(n) * y;', 'pu->m += cosine(n) * y;', '*pv += cosine(n) * y;',
                      'vv->x += cosine(n) * y;', 'va[n] += cosine(n) * y;', 'vp[0] += cosine(n) * y;',
                      'vw[0] += cosine(n) * y;', 'pu->q[1] += cosine(n) * y;']
        why = ('a product SHC fuses that holds a call, which GCC fuses only as an addition, beside a left operand the '
               "tool doesn't take")
        self.assertEqual(self.compounds(statements), (None, [
            f'a.c:20: f()->x += cosine(n) * y: {why}: f(), a call',
            f'a.c:21: p++->x += cosine(n) * y: {why}: p++, an increment',
            f'a.c:22: (++p)->x += cosine(n) * y: {why}: ++p, an increment',
            f'a.c:23: (--p)->x += cosine(n) * y: {why}: --p, a decrement',
            f'a.c:24: p->a[n + 1] += cosine(n) * y: {why}: p->a[n + 1], a subscript by an expression',
            f'a.c:25: (p = s)->x += cosine(n) * y: {why}: (p = s), none of the left operands the tool takes',
            f'a.c:26: body->x += cosine(n) * y: {why}: body->x, a volatile object',
            f'a.c:27: pu->m += cosine(n) * y: {why}: pu->m, a volatile object',
            f'a.c:28: *pv += cosine(n) * y: {why}: *pv, a volatile object',
            f'a.c:29: vv->x += cosine(n) * y: {why}: vv->x, a volatile object',
            f'a.c:30: va[n] += cosine(n) * y: {why}: va[n], a volatile object',
            f'a.c:31: vp[0] += cosine(n) * y: {why}: vp, a volatile object',
            f'a.c:32: vw[0] += cosine(n) * y: {why}: vw, a volatile object',
            f'a.c:33: pu->q[1] += cosine(n) * y: {why}: pu->q, a volatile object']))
        # a left operand whose type the tool can't read
        self.assertEqual(self.compounds(['q->x += cosine(n) * y;']), (None, [
            "a.c:20: q->x += cosine(n) * y: a product SHC can fuse, whose types the tool can't read: q, which no "
            'declaration in scope names']))
        # a parameter a declarator qualifies volatile
        c = 'extern float cosine(short);\nvoid h(float *volatile w, short n)\n{\n    w[0] += cosine(n) * 2.0F;\n}\n'
        self.assertEqual(folded(c)[1], [f'a.c:4: w[0] += cosine(n) * 0x1p+1F: {why}: w, a volatile object'])

    def test_fails_on_a_plus_equals_to_write_beside_a_cast_it_doesn_t_read(self):
        # and writes one beside a cast to a typedef's type
        self.assertEqual(self.compounds(['p->x += (__typeof__(y)) cosine(n) * y;']), (None, [
            "a.c:20: p->x += (__typeof__(y)) cosine(n) * y: a product SHC fuses, beside (__typeof__(y)) cosine(n), a "
            "cast to a type the tool doesn't read: __typeof__ ( y ): __typeof__, a word Types doesn't read"]))
        self.assertEqual(self.compounds(['p->x += (Float) cosine(n) * y;']),
                         ([f'{captured("p->x", "(Float) cosine(n) * y")};'], []))

    def test_leaves_a_plus_equals_gcc_fuses_as_shc_does(self):
        # a product with no side effect, one inside a run of + and - the right operand holds, an integer one, and
        # any on a route that fuses none; on R32 with a 64-bit double, a double's, which SHC computes in binary64
        # and FMAC doesn't take, beside a float's it writes; and a system header's
        statements = ['p->x += p->a[n] * y;', 'p->x += y + cosine(n) * y;', 'n += n * n;']
        self.assertEqual(self.compounds(statements), (statements, []))
        self.assertEqual(self.compounds(['p->x += cosine(n) * y;'], R10), (['p->x += cosine(n) * y;'], []))
        self.assertEqual(self.compounds(['dd += dcos(dd) * dd;', 'p->x += cosine(n) * y;'], R32),
                         (['dd += dcos(dd) * dd;', f'{captured("p->x", "cosine(n) * y")};'], []))
        c = ('# 1 "tools/sh4zam/include/sh4zam/shz_trig.h" 1 3\n'
             'static float turn(float a) { a += __builtin_sqrtf(a) * a; return a; }\n')
        self.assertEqual(folded(c), (f'# 1 "a.c"\n{c}', []))

    def test_wraps_the_product_of_a_plus_equals_into_an_integer(self):
        # SHC converts the integer, adds the product by FMUL and FADD, and converts back, with a call in the product
        # and without
        self.assertEqual(self.compounds(['n += y * y;', 'n += cosine(n) * y;']),
                         ([f'n += {wrapped("y * y")};', f'n += {wrapped("cosine(n) * y")};'], []))

    def test_keeps_a_minus_equals_unfused(self):
        # SHC subtracts the product of a -=, by FMUL and FSUB, as camera_step_follow's other arm does by the FMUL at
        # 0x8c023f46 and the FSUB at 0x8c023f4c in 1ST_READ.BIN
        self.assertEqual(self.compounds(['p->x -= cosine(n) * y;']), ([f'p->x -= {wrapped("cosine(n) * y")};'], []))


if __name__ == '__main__':
    unittest.main()
