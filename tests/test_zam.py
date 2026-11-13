"""SH4ZAM as the tree takes it: each entry a source calls gives the console's bits whatever its arguments, as
PORTS.md#sh4zam-and-the-runtime says, a function takes SHZ_FAST_MATH only in a file whose bin config/parts.txt gives as
picture, a body that hands SH4ZAM a caller's matrix as its own type, which aligns to 8, traps in a build without
NDEBUG on one that lies off 8, and GCC refuses a structure that would hold a matrix off 8. The lists hold an entry's
name and not how a body puts entries together: njMultiMatrix.c's product in memory takes EXACT's entries in other
roles than its shipped body's, as its own comment says."""

import re
import subprocess
import unittest
from pathlib import Path

import csource
import parts
import test_ld
from support import cwd, tree

ROOT = Path(__file__).resolve().parents[1]
# a call of an SH4ZAM function or function-like macro
CALL = re.compile(r'\b((?:shz|SHZ)_\w+)\s*\(')
# the entries a source may call, each a back-end entry or a public one with no constant branch, so it runs the same
# instructions whatever its arguments, but XMTRX's unaligned load, store and apply, which move a matrix by pairs or by
# single floats as its address is aligned, to the same bits. shz_floorf and shz_ceilf fold a constant by GCC's floor
# and ceil, which round nothing, and mathf.h gives a zero result x's sign either way
EXACT = {
    # four FIPRs, or three, the vector first in each
    'shz_mat4x4_transform_vec4_sh4', 'shz_mat4x4_transform_vec3',
    # FSRRA and FSCA
    'shz_inv_sqrtf_fsrra_sh4', 'shz_sincosu16_sh4',
    # XMTRX's identity, loads, stores and moves, FTRV and its multiplies, and its turns by FSCA and two FTRVs
    'shz_xmtrx_init_identity', 'shz_xmtrx_load_4x4', 'shz_xmtrx_load_unaligned_4x4', 'shz_xmtrx_store_4x4',
    'shz_xmtrx_store_unaligned_4x4',
    'shz_xmtrx_write_row', 'shz_xmtrx_read_col', 'shz_xmtrx_write_col', 'shz_xmtrx_get_translation',
    'shz_xmtrx_apply_4x4', 'shz_xmtrx_apply_unaligned_4x4', 'shz_xmtrx_translate', 'shz_xmtrx_scale',
    'shz_xmtrx_transform_vec4', 'shz_xmtrx_transform_vec3', 'shz_xmtrx_transform_point3', 'shz_xmtrx_transform_point2',
    'shz_xmtrx_rotate_x_u16', 'shz_xmtrx_rotate_y_u16', 'shz_xmtrx_rotate_z_u16',
    # one product, or one sum, in each lane
    'shz_vec3_scale', 'shz_vec4_scale', 'shz_vec3_add', 'shz_vec3_sub',
    # a float's integer part and a comparison, exact
    'shz_floorf', 'shz_ceilf',
    # a vector of floats as given
    'shz_vec2_init', 'shz_vec3_init', 'shz_vec4_init', 'shz_vec2_vec4', 'shz_vec3_vec4', 'shz_vec3_deref',
    'shz_vec4_deref',
    # bytes and cache lines
    'shz_memcpy1', 'shz_memcpy4', 'shz_memset8', 'shz_dcache_alloc_line', 'SHZ_PREFETCH',
}
# an entry whose C sums products, and the sources that may call it. SH4ZAM's headers take the flags of the object
# that includes them, and R26's and R32's objects compile under -ffp-contract=on, which fuses such a sum into an FMAC.
# Each source here R10 builds, under -ffp-contract=off
SUMMED = {'shz_vec3_cross': {'sdk/ninja/njModel.c'}}
# SH4ZAM's mark that gives a function fast-math, which only a file whose bin is picture takes: a game step reads
# nothing such a file computes but what it brings from outside the game (ENGINE.md#numbers)
FAST_MATH = re.compile(r'\bSHZ_FAST_MATH\b')
# the bodies that cast a matrix a caller passes to SH4ZAM's shz_mat4x4_t, which aligns to 8: njCalcVector's, out of
# line in its own source, and ninja_zam.h's helpers of njCalcPoint and njTranslate, which BOUNDARIES compiles into a
# function each. Each asserts that the matrix lies on 8
CALC_VECTOR = 'engine/dc/ninja/njCalcVector.c'
BOUNDARIES = '''\
#include <ninja.h>

void calc_point(NJS_MATRIX *m, NJS_POINT3 *p)
{
    njCalcPoint(m, p, p);
}

void translate(NJS_MATRIX *m, NJS_VECTOR *v)
{
    njTranslateV(m, v);
}
'''
# what a false assertion runs in a build without NDEBUG, as sdk/include/assert.h spells it, and GCC's test of an
# address's low three bits, which decides the trap of an assertion that the address lies on 8
TRAP = 'trapa #0xc3'
LOW_THREE = '\ttst\t#7,r0\n'
# declarations that hold an NJS_MATRIX, each with what GCC says of it under make dc's flags, whose -fpack-struct=4
# aligns a member to 4 at most, or None where the declaration compiles: a matrix 4 past a boundary, one that starts
# a structure of 4's alignment, an array of them, a union's, one #pragma pack(8) wraps, and a pointer to one
HOLDERS = (
    ('struct off {\n    int word;\n    NJS_MATRIX matrix;\n};\n',
     "'matrix' offset 4 in 'struct off' isn't aligned to 8"),
    ('struct under {\n    NJS_MATRIX matrix;\n    int word;\n};\n', "alignment 4 of 'struct under' is less than 8"),
    ('struct rows {\n    int word;\n    NJS_MATRIX matrices[2];\n};\n',
     "'matrices' offset 4 in 'struct rows' isn't aligned to 8"),
    ('union either {\n    NJS_MATRIX matrix;\n    int word;\n};\n', "alignment 4 of 'union either' is less than 8"),
    ('#pragma pack(push, 8)\nstruct wrapped {\n    int word;\n    NJS_MATRIX matrix;\n};\n#pragma pack(pop)\n', None),
    ('struct points {\n    int word;\n    NJS_MATRIX *matrix;\n};\n', None),
)


def calls(path, text):
    """Each call of an SH4ZAM entry in path's text, outside comments and literals, as (path:line, entry)."""
    code = csource.blanked(text)
    return [(f'{path}:{code.count(chr(10), 0, call.start()) + 1}', call[1]) for call in CALL.finditer(code)]


def refused(found):
    """Each of found, as calls() gives them, that EXACT lacks and SUMMED doesn't allow its source, as
    path:line: entry."""
    return [f'{at}: {entry}' for at, entry in found
            if entry not in EXACT and at.rpartition(':')[0] not in SUMMED.get(entry, ())]


def idle(found):
    """Each entry EXACT lists that none of found, as calls() gives them, calls, and each source SUMMED lists for an
    entry that calls it no more, as what to do."""
    called = {(at.rpartition(':')[0], entry) for at, entry in found}
    return ([f'tests/test_zam.py: EXACT lists {entry}, which no source calls: take it out'
             for entry in sorted(EXACT - {entry for _, entry in called})]
            + [f'tests/test_zam.py: SUMMED lists {entry} for {path}, which calls it no more: take it out'
               for entry, paths in sorted(SUMMED.items()) for path in sorted(paths) if (path, entry) not in called])


def marked(text):
    """Whether text, a source's, marks a function SHZ_FAST_MATH, outside comments and literals."""
    return bool(FAST_MATH.search(csource.blanked(text)))


def off_picture(paths, rows):
    """Each of paths, sources that mark a function SHZ_FAST_MATH, whose bin rows, parts.rows()'s, don't give as
    picture, as path: what to do."""
    found = []
    for path in paths:
        row = parts.part(rows, path)
        if row is None or row.bin != parts.PICTURE:
            found.append(f'{path}: marks a function SHZ_FAST_MATH, and {parts.PARTS} gives it '
                         f'{"no row" if row is None else row.bin}: take the mark off, as only a file whose bin is '
                         'picture takes one')
    return found


def unchecked(assembly, functions):
    """What to do for each of functions, C names, whose code in assembly, GCC's for an SH-4 source, runs no TRAP
    with LOW_THREE ahead of it, or that assembly doesn't define. A trap that another test decides, of the address's
    low two bits or of another argument, holds no matrix to 8."""
    found = []
    for name in functions:
        defined = re.search(rf'^_{name}:\n(.*?)^\t\.size\t_{name},', assembly, re.S | re.M)
        if not defined:
            found.append(f'no {name} is defined: name in tests/test_zam.py the function that hands SH4ZAM the matrix '
                         'now')
        elif TRAP not in defined[1] or LOW_THREE not in defined[1].partition(TRAP)[0]:
            found.append(f"{name} hands SH4ZAM a caller's matrix as an shz_mat4x4_t, which aligns to 8, and runs no "
                         "trap that a test of the address's low three bits decides: assert((uintptr_t)m % 8 == 0) "
                         'ahead of the cast')
    return found


def tracked(*patterns):
    """The files git tracks that match patterns."""
    listed = subprocess.run(['git', 'ls-files', '-z', '--', *patterns], stdout=subprocess.PIPE, check=True).stdout
    return [path for path in listed.decode().split('\0') if path]


class Exact(unittest.TestCase):
    maxDiff = None

    def test_each_source_calls_only_entries_the_lists_allow_and_each_list_only_entries_a_source_calls(self):
        found = []
        with cwd(ROOT):
            for path in tracked('*.c', '*.h'):
                text = Path(path).read_text(errors='replace') if Path(path).is_file() else ''
                if 'shz_' in text or 'SHZ_' in text:
                    found += calls(path, text)
        self.assertTrue(found)
        self.assertEqual(refused(found), [])
        self.assertEqual(idle(found), [])

    def test_names_a_call_off_the_list_and_none_in_a_comment(self):
        # shz_sqrtf approximates by FSRRA, shz_fmaf is a * b + c, two roundings under -ffp-contract=off, and
        # shz_vec3_cross sums products outside njModel.c
        text = ('/* shz_sqrtf(x) */\nfloat f(float x, shz_vec3_t a, shz_vec3_t b)\n{\n'
                '    return shz_sqrtf(x) + shz_vec3_cross(a, b).x + shz_fmaf(x, x, x);\n}\n')
        self.assertEqual(refused(calls('src/a.c', text)),
                         ['src/a.c:4: shz_sqrtf', 'src/a.c:4: shz_vec3_cross', 'src/a.c:4: shz_fmaf'])
        self.assertEqual(refused(calls('sdk/ninja/njModel.c', text)),
                         ['sdk/ninja/njModel.c:4: shz_sqrtf', 'sdk/ninja/njModel.c:4: shz_fmaf'])

    def test_names_a_listed_entry_that_no_source_calls(self):
        # a source that calls every entry of EXACT but one, and none of SUMMED's
        found = [('src/b.c:1', entry) for entry in EXACT - {'shz_memcpy1'}]
        self.assertEqual(idle(found), [
            'tests/test_zam.py: EXACT lists shz_memcpy1, which no source calls: take it out',
            'tests/test_zam.py: SUMMED lists shz_vec3_cross for sdk/ninja/njModel.c, which calls it no more: take it '
            'out'])
        # SUMMED's entry called by another source than the one it lists is idle too, and by its own it is not
        self.assertEqual(idle(found + [('src/b.c:2', 'shz_vec3_cross')])[1:], [
            'tests/test_zam.py: SUMMED lists shz_vec3_cross for sdk/ninja/njModel.c, which calls it no more: take it '
            'out'])
        self.assertEqual(idle(found + [('sdk/ninja/njModel.c:9', 'shz_vec3_cross'), ('src/b.c:2', 'shz_memcpy1')]),
                         [])

    def test_marks_fast_math_only_in_a_file_whose_bin_is_picture(self):
        with cwd(ROOT):
            found = [path for path in tracked('*.c', '*.h')
                     if Path(path).is_file() and marked(Path(path).read_text(errors='replace'))]
            rows = parts.rows()
        # no file takes the mark today: the model walk's six functions gave theirs up
        self.assertEqual(off_picture(found, rows), [])

    def test_names_a_mark_in_a_file_of_another_bin_and_none_in_a_comment(self):
        self.assertTrue(marked('static SHZ_FAST_MATH float f(float x)\n{\n    return x;\n}\n'))
        self.assertFalse(marked('/* SHZ_FAST_MATH */\nfloat f(float x);\nconst char *g = "SHZ_FAST_MATH";\n'))
        # the walk's row lets its mark by, and a file of play, of both, of none or of no row is named
        table = ('engine/dc/sa1/  drawer  picture\nsrc/  game  play\nsrc/draw/model.c  game  both  # a layer\n'
                 'engine/tests/  tool  none\n')
        with tree({'config/parts.txt': table}):
            rows = parts.rows()
        only = 'take the mark off, as only a file whose bin is picture takes one'
        self.assertEqual(
            off_picture(['engine/dc/sa1/model.c', 'src/system/math.c', 'src/draw/model.c', 'engine/tests/a.c',
                         'engine/dc/math.c'], rows),
            [f'src/system/math.c: marks a function SHZ_FAST_MATH, and config/parts.txt gives it play: {only}',
             f'src/draw/model.c: marks a function SHZ_FAST_MATH, and config/parts.txt gives it both: {only}',
             f'engine/tests/a.c: marks a function SHZ_FAST_MATH, and config/parts.txt gives it none: {only}',
             f'engine/dc/math.c: marks a function SHZ_FAST_MATH, and config/parts.txt gives it no row: {only}'])

    def test_a_build_without_ndebug_traps_where_a_caller_hands_sh4zam_a_matrix_off_8(self):
        # an NJS_MATRIX aligns to 8 in make dc, as shz_mat4x4_t does, and sdk/include/njtypes.h says what keeps one
        # there. Each body that casts a caller's matrix to SH4ZAM's type asserts it, which a build without NDEBUG
        # checks, as config/gcc.mk says
        test_ld.toolchain(('bin/sh-elf-gcc',))
        if not (ROOT / 'tools/sh4zam/fetched.txt').is_file():
            self.skipTest('tools/sh4zam lacks the fetch make dc makes')
        compiled = test_ld.flags()
        checked = [flag for flag in compiled if flag != '-DNDEBUG']
        self.assertNotEqual(checked, compiled)
        with tree({'boundaries.c': BOUNDARIES}) as root:
            for source, functions in ((ROOT / CALC_VECTOR, ['transform_by']),
                                      (root / 'boundaries.c', ['calc_point', 'translate'])):
                test_ld.run('bin/sh-elf-gcc', *checked, '-Isdk/include', '-Isrc', '-I.', '-S', '-o',
                            str(root / 'checked.s'), str(source), root=ROOT)
                self.assertEqual(unchecked((root / 'checked.s').read_text(), functions), [], source.name)

    def test_gcc_refuses_a_structure_that_holds_a_matrix_off_8(self):
        # -fpack-struct=4 would put a structure's matrix on 4, and a body handed its address would give SH4ZAM a
        # matrix off 8. sdk/include/njtypes.h has GCC warn of the structure, and make dc fails on a warning
        test_ld.toolchain(('bin/sh-elf-gcc',))
        if not (ROOT / 'tools/sh4zam/fetched.txt').is_file():
            self.skipTest('tools/sh4zam lacks the fetch make dc makes')
        compiled = test_ld.flags()
        with tree({}) as root:
            for text, said in HOLDERS:
                (root / 'holder.c').write_text(f'#include <ninja.h>\n\n{text}')
                argv = ('bin/sh-elf-gcc', *compiled, '-Isdk/include', '-Isrc', '-I.', '-fsyntax-only',
                        str(root / 'holder.c'))
                if said is None:
                    test_ld.run(*argv, root=ROOT)
                    continue
                with self.assertRaises(AssertionError, msg=text) as refused:
                    test_ld.run(*argv, root=ROOT)
                self.assertIn(f'error: {said} [-Werror=if-not-aligned]', str(refused.exception))

    def test_names_a_function_that_holds_no_matrix_to_8_and_one_the_assembly_lacks(self):
        trapped = ('_first:\n\tmov\tr4,r0\n\ttst\t#7,r0\n\tbt\t.L2\n.L3:\n\ttrapa #0xc3\n\tbra\t.L3\n\tnop\n'
                   '.L2:\n\trts\t\n\tnop\n\t.size\t_first, .-_first\n')
        bare = '_second:\n\trts\t\n\tnop\n\t.size\t_second, .-_second\n'

        def lacks(name):
            return (f"{name} hands SH4ZAM a caller's matrix as an shz_mat4x4_t, which aligns to 8, and runs no trap "
                    "that a test of the address's low three bits decides: assert((uintptr_t)m % 8 == 0) ahead of the "
                    'cast')
        self.assertEqual(unchecked(trapped + bare, ['first']), [])
        # a trap in the function before it, or after it, is none of its own
        self.assertEqual(unchecked(trapped + bare + trapped.replace('first', 'third'), ['first', 'second', 'third']),
                         [lacks('second')])
        # a trap for an address off 4, one for a null argument, and a test of the low three bits that comes after
        # the trap hold no matrix to 8
        for name, old, new in (('on_4', '#7,r0', '#3,r0'), ('null', '\ttst\t#7,r0', '\ttst\tr5,r5'),
                               ('after', '\ttst\t#7,r0\n\tbt\t.L2\n.L3:\n\ttrapa #0xc3\n',
                                '\ttst\tr5,r5\n\tbt\t.L2\n.L3:\n\ttrapa #0xc3\n\ttst\t#7,r0\n')):
            self.assertEqual(unchecked(trapped.replace('first', name).replace(old, new), [name]), [lacks(name)])
        self.assertEqual(unchecked(trapped, ['second']),
                         ['no second is defined: name in tests/test_zam.py the function that hands SH4ZAM the matrix '
                          'now'])

    def test_compiles_the_tree_and_sh4zam_s_library_with_no_sh4zam_macro(self):
        # SH4ZAM builds in its default mode, as upstream's does
        for flags in (test_ld.flags(), test_ld.flags('GCC_SH4ZAM_FLAGS')):
            self.assertEqual([flag for flag in flags if flag.startswith('-DSHZ_')], [])
