import re
import struct
import subprocess
import unittest
from pathlib import Path

import float_edits
import float_tokens
import float_value
import floats
import test_ld
from cdata import spell
from support import cwd, tree
from test_float_fold import SYSTEM, folded
from test_float_value import GAME, R10, R32, word

ROOT = Path(__file__).resolve().parents[1]

# GCC's preprocessed C for a.c, as its line markers place each line: a.c includes a.h, whose own line holds a
# constant, then SH4ZAM's shz_trig.h, a system header, whose constant stays as GCC reads it. From line 3 come a.c's
# own lines, with a blank line 5 that GCC keeps
PREPROCESSED = '''# 0 "a.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "a.c"
# 1 "a.h" 1
static const float third = 0.33333334F;
# 2 "a.c" 2
# 1 "tools/sh4zam/include/sh4zam/shz_trig.h" 1 3
static const float pi = 3.14159265359f;
# 3 "a.c" 2
float tenth = 0.1F, low = 0.1f, big = 1e10F, half = .5F, hundred = 100.F;
double d = 0.1, milli = 1.e-3, five = 5E+2, quarter = 2.5e-1f;

long double l = 0.1L, m = 0.1l;
const char *s = "0.1F and 1e5", *quoted = "0.1F\\"", *raw = R"x(a"1.5F)x";
char c = '1';
int hex = 0x1e5, upper = 0X1E5, n = 10, x1e5 = 0;
float hexadecimal = 0x1.99999ap-4F;
#pragma section HEAD
#pragma pack 1.5
'''
# PREPROCESSED as the tool writes it: each decimal floating constant on a line of the tree's files as the hexadecimal
# one that names SHC's value, with the same suffix. 0.1F truncates to 0x3dcccccc, a step below GCC's 0x3dcccccd. The
# strings stay, past an escaped quote and in a raw string, as do the character, the integers, 0X1E5 among them, the
# hexadecimal constant, the system header's constant and the pragmas, 1.5 among their tokens
RESPELLED = '''# 0 "a.c"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "a.c"
# 1 "a.h" 1
static const float third = 0x1.555554p-2F;
# 2 "a.c" 2
# 1 "tools/sh4zam/include/sh4zam/shz_trig.h" 1 3
static const float pi = 3.14159265359f;
# 3 "a.c" 2
float tenth = 0x1.999998p-4F, low = 0x1.999998p-4f, big = 0x1.2a05f2p+33F, half = 0x1p-1F, hundred = 0x1.9p+6F;
double d = 0x1.999998p-4, milli = 0x1.0624dcp-10, five = 0x1.f4p+8, quarter = 0x1p-2f;

long double l = 0x1.9999999999999p-4L, m = 0x1.9999999999999p-4l;
const char *s = "0.1F and 1e5", *quoted = "0.1F\\"", *raw = R"x(a"1.5F)x";
char c = '1';
int hex = 0x1e5, upper = 0X1E5, n = 10, x1e5 = 0;
float hexadecimal = 0x1.99999ap-4F;
#pragma section HEAD
#pragma pack 1.5
'''


class Respell(unittest.TestCase):
    def test_respells_each_decimal_floating_constant_on_the_tree_s_lines_as_shc_reads_it(self):
        # under R32 with a 64-bit double, which a runtime member takes
        self.assertEqual(floats.translate(PREPROCESSED, 'a.i', R32, SYSTEM), (RESPELLED, []))

    def test_takes_a_header_under_a_directory_of_system_headers_for_a_system_header_however_its_marker_names_it(self):
        # shz_trig.h, named by its path from the checkout, by an absolute path, as GCC names it under an absolute
        # -isystem, and through a ..: each lies under SYSTEM's directory once resolved, so its constant keeps its
        # spelling. Without SYSTEM, the header's lines are the tree's
        for named in ('tools/sh4zam/include/sh4zam/shz_trig.h', f'{Path.cwd()}/tools/sh4zam/include/sh4zam/shz_trig.h',
                      'tools/sh4zam/source/../include/sh4zam/shz_trig.h'):
            c = f'# 1 "a.c"\n# 1 "{named}" 1 3\nstatic const float pi = 3.14159265359f;\n'
            with self.subTest(named):
                self.assertEqual(floats.translate(c, 'a.i', R32, SYSTEM)[0], c)
                self.assertEqual(floats.translate(c, 'a.i', R32)[0], c.replace('3.14159265359f', '0x1.921fb4p+1f'))

    def test_reads_back_each_word_spell_gives(self):
        for magnitude in (0x00800000, 0x3DCCCCCD, 0x3EAAAAAB, 0x43935645, 0x4479C000, 0x7F7FFFFE):
            for sign in (0, 0x80000000):
                with self.subTest(word=f'0x{magnitude | sign:08x}'):
                    # the sign is no part of the constant
                    text = spell(magnitude | sign)
                    self.assertEqual(word(float_value.respelled(text.lstrip('-'), GAME)[0]), magnitude, text)

    def test_names_the_file_and_line_of_each_constant_shc_can_t_read(self):
        # past the largest float's step on, where SHC warns 1012 and gives an infinity; below the smallest normal
        # float, where SHC warns 1015 and gives zero; and pp-numbers that aren't floating constants C89 spells. The
        # tool writes no C
        bad = '# 1 "b.c"\nfloat a = 1e39F;\n\nfloat b = 1e-40F, c = 0.5F16;\n# 7 "b.h" 1\nfloat d = 1.0e;\n'
        self.assertEqual(floats.translate(bad, 'b.i', GAME), (None, [
            "b.c:1: 1e39F: 2**128 or above, where SHC warns that its conversion overflows",
            "b.c:3: 1e-40F: below 2**-126 but not zero, where SHC warns that its conversion underflows",
            "b.c:3: 0.5F16: no floating constant C89 spells",
            "b.h:7: 1.0e: no floating constant C89 spells",
        ]))
        # and a double R10 holds as binary64 that no float holds
        self.assertEqual(floats.translate('# 1 "c.c"\ndouble e = 1e300;\n', 'c.i', R10), (None, [
            "c.c:1: 1e300: past the floats, which GCC's double is, where SHC holds it as binary64"]))

    def test_fails_on_a_hexadecimal_constant_on_the_tree_s_lines_gcc_s_format_doesn_t_hold_exactly_and_normally(self):
        # alone as a static object's initializer, which the fold leaves; in parentheses, which it folds; and in code,
        # where GCC would round it. A system header's keeps its own
        c = ('# 1 "a.c"\nfloat alone = 0x1.0000001p+0F, low = 0x1p-127F;\nfloat parenthesized = (0x1.0000001p+0F);\n'
             'float f(float x) { return x * 0x1.0000001p+0F; }\n')
        why = "a value GCC's float doesn't hold exactly and normally"
        self.assertEqual(floats.translate(c, 'a.i', GAME, SYSTEM), (None, [
            f'a.c:1: 0x1.0000001p+0F: {why}', f'a.c:1: 0x1p-127F: {why}', f'a.c:2: 0x1.0000001p+0F: {why}',
            f'a.c:3: 0x1.0000001p+0F: {why}']))
        c = ('# 1 "tools/sh4zam/include/sh4zam/shz_trig.h" 1 3\n'
             'static float g(float x) { return x * 0x1.0000001p+0f; }\n')
        self.assertEqual(floats.translate(c, 'a.i', GAME, SYSTEM), (c, []))

    def test_names_the_line_of_the_declaration_c_ends_inside(self):
        # by its marker, as each problem names its line, and not by the .i's own lines
        self.assertEqual(floats.translate('# 23 "b.h"\nfloat value =\n', 'b.i', GAME),
                         (None, ['b.h:23: C that ends inside a declaration']))

    def test_respells_in_place_and_fails_naming_the_tool(self):
        # and leaves the C of a source it fails on as it was
        failing = '# 1 "b.c"\nfloat a = 1e39F, b = 0.5F;\n'
        with tree({'a.i': PREPROCESSED, 'b.i': failing}) as root:
            floats.main(['--shc', 'r32', '--system', SYSTEM[0], 'a.i'])
            self.assertEqual((root / 'a.i').read_text(), RESPELLED)
            with self.assertRaisesRegex(SystemExit, r'\A[^\n]*: b\.c:1: 1e39F: 2\*\*128 or above'):
                floats.main(['--shc', 'r32', '-fpu=single', 'b.i'])
            self.assertEqual((root / 'b.i').read_text(), failing)


class Edits(unittest.TestCase):
    # a.c's line 1 and the start of its line 2, as tokens: y = a * b + c ;
    C = '# 1 "a.c"\ny = a *\n    b + c;\n'

    def plan(self):
        return float_edits.Plan(float_tokens.marked(self.C, 'a.i'))

    def test_writes_an_edit_in_its_first_token_s_place_and_each_edit_inside_its_spans_there(self):
        # f(a * b, c) in a's place, its spans on one line; what lies between the edit's tokens stays, and its other
        # tokens give nothing. b's edit lies inside the span a * b, and the edit inside that span is written there
        plan = self.plan()
        plan.edit(2, 7, ['f(', (2, 5), ', ', (6, 7), ')'])
        self.assertEqual(plan.text(self.C), '# 1 "a.c"\ny = f(a * b, c) \n      ;\n')
        plan.edit(4, 5, ['B'])
        self.assertEqual(plan.text(self.C), '# 1 "a.c"\ny = f(a * B, c) \n      ;\n')

    def test_fails_on_two_edits_that_overlap_but_one_inside_a_span_of_the_other(self):
        # one past another's end, one inside another but outside its spans, and two of one span
        overlap = "two of the tool's edits overlap here"
        for spans, problem in ((((2, 5), (4, 7)), f'a.c:2: b + c: {overlap}'),
                               (((2, 7), (6, 7)), f'a.c:2: c: {overlap}')):
            with self.subTest(spans):
                plan = self.plan()
                plan.edit(*spans[0], ['(', (2, 5), ')'])
                plan.edit(*spans[1], ['q'])
                with self.assertRaisesRegex(ValueError, f'^{re.escape(problem)}$'):
                    plan.text(self.C)
        plan = self.plan()
        plan.edit(2, 5, ['p'])
        with self.assertRaisesRegex(ValueError, f'^a\\.c:1: a \\* b: {overlap}$'):
            plan.edit(2, 5, ['q'])


def returned(expression, route=GAME):
    """expression as the tool writes it under route, the game's by default, on one line, or None where it fails; and
    the problems. A function of the floats x and y, the int i, the unsigned u and p, a pointer to floats, returns
    expression."""
    c = f'float f(float x, float y, int i, unsigned u, const float *p)\n{{\n    return {expression};\n}}\n'
    text, problems = folded(c, route)
    if text is None:
        return None, problems
    return re.fullmatch(r'return (.*?) ?;', ' '.join(text.splitlines()[3].split()))[1], problems


def spelled(expression, route=GAME):
    """expression on one line, each run of whitespace a space, with each constant of digits, a point, any digits and
    F respelled as the tool respells it under route, the game's by default."""
    return ' '.join(re.sub(r'(?<![\w.])[0-9]+\.[0-9]*F', lambda constant: float_value.respelled(constant[0], route)[0],
                           expression).split())


class Runs(unittest.TestCase):
    def test_leaves_each_run_as_c_groups_it(self):
        # SHC folds a run's constants into one and applies that last, so it multiplies x by 0.1F * 0.3F's 0x3cf5c28d,
        # and multiplies a run of *'s other operands in an order of its own, as it multiplies ev0003_curve's
        # ((along * 2.0F) - 3.0F) by along, then by along again, by the FMUL at 0x0cb800e2 and the FMAC at 0x0cb800ea
        # in EV0003.BIN; it orders three operands of a run of + and - that aren't constant by what they are, and
        # applies the constant of one a unary - negates first. The tool writes each as C groups it, and fails on none
        for expression in ('x * 0.1F * 0.3F', 'x + 0.1F + 0.3F', 'x * x * ((x * 2.0F) - 3.0F)',
                           'x + 0.1F + y + (x / y) + 0.3F', '-(x + 2.0F + y)'):
            with self.subTest(expression):
                self.assertEqual(returned(expression), (spelled(expression), []))


# the levels make gcc compiles the tree's C at, as flags after GCCFLAGS, whose -Os the last level given overrides: -Os,
# GCC_HOT's -O2, and the -O0 of a source SHC builds under -optimize=0
LEVELS = ([], ['-O2'], ['-O0'])
# the -ffp-contract=on the gcc macro gives an object R26 or R32 builds after GCCFLAGS' -ffp-contract=off, which an
# object R10 builds keeps
FUSING = ['-ffp-contract=on']
# the SH-4's floating arithmetic: a sum, a difference, a product, a quotient, a multiply-add, a negation, a square root,
# and a conversion from an integer and to one
ARITHMETIC = ('fadd', 'fsub', 'fmul', 'fdiv', 'fmac', 'fneg', 'fsqrt', 'float', 'ftrc')
# the instructions that call, load a register from memory, and load and store a float, as GCC's listing spells each,
# with Rn and Rm
ACCESSES = {'jsr': r'jsr @(?P<n>r\d+)', 'mov.l @Rm,Rn': r'mov\.l @(?P<m>r\d+),(?P<n>r\d+)',
            'fmov.s @Rm,FRn': r'fmov\.s @(?P<m>r\d+),(?P<n>fr\d+)',
            'fmov.s FRm,@Rn': r'fmov\.s (?P<m>fr\d+),@(?P<n>r\d+)'}


def function(listing):
    """The instructions of the function f in listing, GCC's assembly, in order, each its mnemonic and its operands
    one space apart. The listing spells f's literal pool as data, so no constant there reads as an instruction."""
    body = listing.split('\n_f:\n', 1)[1].split('\n\t.size\t_f,', 1)[0]
    return [' '.join(line.split()) for line in body.splitlines()
            if line.startswith('\t') and not line.startswith('\t.')]


def arithmetic(instructions):
    """The mnemonic of each of instructions that ARITHMETIC holds, sorted."""
    return sorted(instruction.split()[0] for instruction in instructions if instruction.split()[0] in ARITHMETIC)


def accesses(instructions):
    """Each of instructions that ACCESSES names, in the order they take effect, which puts a JSR's delay slot before
    the JSR: its name, its Rn and its Rm, None for a JSR's."""
    ordered = list(instructions)
    for at, instruction in enumerate(instructions):
        if instruction.startswith('jsr '):
            ordered[at:at + 2] = [instructions[at + 1], instruction]
    return [(name, match['n'], match.groupdict().get('m')) for instruction in ordered for name, form in ACCESSES.items()
            for match in [re.fullmatch(form, instruction)] if match]


class Gcc(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # GCCFLAGS, and SH4ZAM's include directory, which the gcc macro names to the tool, as make gives them
        cls.gccflags = test_ld.flags()
        cls.system = ['--system', *test_ld.flags('SH4ZAM_INCLUDE')]

    def compiled(self, files, more=(), code=False):
        """The words of a.c's .data, or where code is true, function()'s instructions of its f, which files hold, as
        GCC compiles a.i under GCCFLAGS and the flags more, as GCC's preprocessor writes it and then as the tool
        respells and folds it, by case; and a.i, as written. GCC and the tool run from the checkout, as make gcc runs
        them, where GCCFLAGS' paths resolve. They take the files by absolute path, which a.i's line markers name,
        and a.i comes back with the files' directory taken out of those paths. The toolchain is test_ld.TOOLCHAIN.
        Where it lacks sh-elf-gcc or sh-elf-objcopy, the test fails if SH_ELF names it, as test_ld's fixture does,
        and skips otherwise."""
        toolchain = test_ld.TOOLCHAIN
        missing = [tool for tool in ('bin/sh-elf-gcc', 'bin/sh-elf-objcopy') if not (toolchain / tool).is_file()]
        if missing and test_ld.GIVEN:
            raise AssertionError(f'{toolchain}, which SH_ELF names, lacks {", ".join(missing)}')
        if missing:
            self.skipTest(f'{toolchain} lacks {", ".join(missing)}, which make gcc builds')
        gcc, objcopy = toolchain / 'bin/sh-elf-gcc', toolchain / 'bin/sh-elf-objcopy'
        with tree(files) as root, cwd(ROOT):
            flags = [*self.gccflags, *more]
            c, i, s, o, raw = (str(root / name) for name in ('a.c', 'a.i', 'a.s', 'a.o', 'a.bin'))
            subprocess.run([gcc, *flags, '-E', '-o', i, c], check=True, capture_output=True)
            written, found = (root / 'a.i').read_text(), {}
            for case in ('as written', 'respelled'):
                if case == 'respelled':
                    floats.main(['--shc', 'r32', '-fpu=single', *self.system, i])
                if code:
                    subprocess.run([gcc, *flags, '-S', '-o', s, i], check=True, capture_output=True)
                    found[case] = function((root / 'a.s').read_text())
                    continue
                subprocess.run([gcc, *flags, '-c', '-o', o, i], check=True, capture_output=True)
                subprocess.run([objcopy, '-O', 'binary', '-j', '.data', o, raw], check=True, capture_output=True)
                data = (root / 'a.bin').read_bytes()
                found[case] = list(struct.unpack(f'<{len(data) // 4}I', data))
        return found, written.replace(f'"{root}/', '"')

    def test_gcc_reads_a_header_macro_s_constant_as_shc_does_once_respelled(self):
        # TENTH expands on a.c's line 2, and GCC's object holds SHC's 0x3dcccccc for it, where it would hold
        # 0x3dcccccd as written
        words, written = self.compiled({'a.h': '#define TENTH 0.1F\n', 'a.c': '#include "a.h"\nfloat tenth = TENTH;\n'})
        self.assertIn('\n# 2 "a.c" 2\nfloat tenth = 0.1F;\n', written)
        self.assertEqual(words, {'as written': [0x3DCCCCCD], 'respelled': [0x3DCCCCCC]})

    def test_gcc_compiles_a_static_initializer_s_quotient_to_shc_s_word_once_folded(self):
        # at file scope and in a function, where GCC folds 1.0F / 6.0F to 0x3e2aaaab as written
        c = ('float sixth = 1.0F / 6.0F;\n\nfloat *share(void)\n{\n    static float part = 1.0F / 6.0F;\n'
             '    return &part;\n}\n')
        words, _ = self.compiled({'a.c': c})
        self.assertEqual(words, {'as written': [0x3E2AAAAB] * 2, 'respelled': [0x3E2AAAAA] * 2})

    def test_gcc_compiles_an_integer_initializer_of_a_float_to_shc_s_word_once_converted(self):
        # pair's float member and bare, where GCC rounds 16777219 to 0x4b800002 and SHC truncates it to 0x4b800001,
        # and pair's int. GCC places the two definitions in either order, but pair's members in theirs
        c = 'struct pair { int count; float scale; } pair = { 16777219, 16777219 };\nfloat bare = 16777219;\n'
        words, _ = self.compiled({'a.c': c})
        for case, scale in (('as written', 0x4B800002), ('respelled', 0x4B800001)):
            pair = [0x01000003, scale]
            self.assertIn(words[case], ([*pair, scale], [scale, *pair]), case)

    def test_gcc_fuses_a_product_where_shc_does_once_the_tool_wraps_the_others(self):
        # under the -ffp-contract=on the gcc macro gives an object R32 builds, GCC fuses c - a * b by FNEG and FMAC
        # as written, and once the tool wraps the product, multiplies by FMUL and subtracts by FSUB, as R32 does; and
        # it fuses a * b - 0.5F by FMAC, adding -0.5F, as written and as the tool leaves it, as R32 does
        for c, instructions in (('float f(float a, float b, float c)\n{\n    return c - a * b;\n}\n',
                                 {'as written': ['fmac', 'fneg'], 'respelled': ['fmul', 'fsub']}),
                                ('float f(float a, float b)\n{\n    return a * b - 0.5F;\n}\n',
                                 {'as written': ['fmac'], 'respelled': ['fmac']})):
            for level in LEVELS:
                with self.subTest(c, level=level):
                    found, _ = self.compiled({'a.c': c}, [*level, *FUSING], True)
                    self.assertEqual({case: arithmetic(found[case]) for case in found}, instructions)

    def test_gcc_fuses_a_plus_equals_of_a_product_holding_a_call_once_the_tool_writes_it(self):
        # camera_step_follow's drift, under -ffp-contract=on, with an inline function of inline assembly in njCos's
        # place, and a call that can move the pointer the left operand reads: as written, GCC multiplies by FMUL,
        # then adds by FADD; once the tool writes the += as an addition, it fuses by FMAC, as R32 does at 0x8c023fb8
        # in 1ST_READ.BIN
        same = 'static inline float same(float a) { float r; __asm__("fmov %1,%0" : "=&f"(r) : "f"(a)); return r; }\n'
        for c in (f'struct v {{ float x; }} *s;\n{same}void f(float a)\n{{\n    s->x += same(a) * 0.2F;\n}}\n',
                  'extern float move(void);\nfloat *aim;\nvoid f(float a)\n{\n    *aim += move() * a;\n}\n'):
            for level in LEVELS:
                with self.subTest(c, level=level):
                    found, _ = self.compiled({'a.c': c}, [*level, *FUSING], True)
                    self.assertEqual({case: arithmetic(found[case]) for case in found},
                                     {'as written': ['fadd', 'fmul'], 'respelled': ['fmac']})

    # -O0 loads an address anew for each access, into a register another access reuses, so the two tests that follow
    # the left operand's registers run at -Os and -O2

    def test_gcc_reads_a_plus_equals_left_operand_s_address_before_the_right_s_call_and_its_value_after(self):
        # SHC evaluates *p's address, then h(), then reads *p, so where h writes *p = 10.0F and returns 2.0F,
        # *p += h() * 3.0F gives 16.0F, not the 7.0F of *p read first, 1.0F; and where h moves p, the sum goes to the
        # float p named first. The addition the tool writes leaves the read and the call in either order; under
        # -ffp-contract=on, GCC 16.2 loads p once, before the call, and *p after it, as the addend of the FMAC, and
        # stores through the p it loaded. As written, it loads p after the call
        c = 'extern float h(void);\nfloat *p;\nvoid f(float b)\n{\n    *p += h() * b;\n}\n'
        for level in LEVELS[:2]:
            with self.subTest(level=level):
                found, _ = self.compiled({'a.c': c}, [*level, *FUSING], True)
                respelled = accesses(found['respelled'])
                call = [name for name, _, _ in respelled].index('jsr')
                before, after = respelled[:call], respelled[call + 1:]
                pointers = [n for name, n, _ in respelled if name == 'mov.l @Rm,Rn']
                self.assertEqual(len(pointers), 1)
                self.assertIn(('mov.l @Rm,Rn', pointers[0]), [(name, n) for name, n, _ in before])
                self.assertNotIn('fmov.s @Rm,FRn', [name for name, _, _ in before])
                self.assertIn(pointers[0], [m for name, _, m in after if name == 'fmov.s @Rm,FRn'])
                self.assertEqual(respelled[-1][:2], ('fmov.s FRm,@Rn', pointers[0]))
                names = [name for name, _, _ in accesses(found['as written'])]
                self.assertGreater(names.index('mov.l @Rm,Rn'), names.index('jsr'))

    def test_gcc_reads_a_plus_equals_left_name_after_the_right_s_call(self):
        # SHC reads a name of static storage after the call, as at g + h() * b, so where h writes x, x += h() * b
        # adds to what h wrote. The tool writes x = x + (h() * b), which leaves the read and the call in either order;
        # under -ffp-contract=on, GCC 16.2 reads x once, after the call, as the addend of the FMAC, and stores through
        # the register it read x through
        c = 'extern float h(void);\nfloat x;\nvoid f(float b)\n{\n    x += h() * b;\n}\n'
        for level in LEVELS[:2]:
            with self.subTest(level=level):
                found, _ = self.compiled({'a.c': c}, [*level, *FUSING], True)
                self.assertEqual(arithmetic(found['respelled']), ['fmac'])
                respelled = accesses(found['respelled'])
                call = [name for name, _, _ in respelled].index('jsr')
                name, through, _ = respelled[-1]
                self.assertEqual(name, 'fmov.s FRm,@Rn')
                reads = [at for at, (name, _, m) in enumerate(respelled) if name == 'fmov.s @Rm,FRn' and m == through]
                self.assertEqual(len(reads), 1)
                self.assertGreater(reads[0], call)

    def test_gcc_leaves_each_constant_operation_whose_value_rounds_to_run_time(self):
        # the tree's code, its spellings of SHC's constant groups among it, relies on GCCFLAGS' -frounding-math,
        # which GCC calls experimental: GCC folds no floating operation of constants whose value rounds, so the
        # console's FPU computes it toward zero, as SHC's folder does, and folds one whose value is exact. So
        # 1.0F / 3.0F is an FDIV, as rraf_fire_create's (1.0F / FLAME_FAR) is; 1.0F - 0.175F an FSUB, as hand_fly's
        # (1.0F - HAND_STEER) is, and 1.0F + 0.175F an FADD; a product of two constants an FMUL, alone and inside a
        # product at run time, as snake_create_segments' (2.0F * NJD_PI / 8) * ANGLE_PER_RADIAN is; a constant
        # product an integer conversion takes an FMUL and an FTRC, as jump.c's (int)JUMP_TIME is; a square root an
        # FSQRT, as spotlight.c's njSqrt(1088888.9F) is; and an integer binary32 doesn't hold a FLOAT, which the
        # tree's code holds none of; where 1.0F / 4.0F, 1.0F - 0.25F, 0.5F + 0.25F, 2.0F * 3.0F, (int)(2.0F * 3.0F),
        # the root of 4.0F and 16777216 fold. Each holds at each level, with contraction and without
        for c, instructions in (('float f(void)\n{\n    return 1.0F / 3.0F;\n}\n', ['fdiv']),
                                ('float f(void)\n{\n    return 1.0F / 4.0F;\n}\n', []),
                                ('float f(void)\n{\n    return 1.0F - 0.175F;\n}\n', ['fsub']),
                                ('float f(void)\n{\n    return 1.0F - 0.25F;\n}\n', []),
                                ('float f(void)\n{\n    return 1.0F + 0.175F;\n}\n', ['fadd']),
                                ('float f(void)\n{\n    return 0.5F + 0.25F;\n}\n', []),
                                ('float f(void)\n{\n    return 0.1F * 0.3F;\n}\n', ['fmul']),
                                ('float f(float x)\n{\n    return x * (0.1F * 0.3F);\n}\n', ['fmul', 'fmul']),
                                ('float f(float x)\n{\n    return x * (2.0F * 3.0F);\n}\n', ['fmul']),
                                ('int f(void)\n{\n    return (int)(25.0F * 0.04F);\n}\n', ['fmul', 'ftrc']),
                                ('int f(void)\n{\n    return (int)(2.0F * 3.0F);\n}\n', []),
                                ('float f(void)\n{\n    return __builtin_sqrtf(2.0F);\n}\n', ['fsqrt']),
                                ('float f(void)\n{\n    return __builtin_sqrtf(4.0F);\n}\n', []),
                                ('float f(float x)\n{\n    return x * 16777219;\n}\n', ['float', 'fmul']),
                                ('float f(float x)\n{\n    return x * 16777216;\n}\n', ['fmul'])):
            for more in ([*level, *fusing] for level in LEVELS for fusing in ([], FUSING)):
                with self.subTest(c, more=more):
                    found, _ = self.compiled({'a.c': c}, more, True)
                    self.assertEqual({case: arithmetic(found[case]) for case in found},
                                     {'as written': instructions, 'respelled': instructions})


if __name__ == '__main__':
    unittest.main()
