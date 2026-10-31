import re
import unittest
from pathlib import Path

import inline
from support import tree

# a source with two inline bodies and two #pragma asm blocks. entry is exported, and step is static and called by
# run. run holds a block of comments alone, and the block below run reserves 8 bytes
SOURCE = '''/* a source with inline assembly */
static void step(void);

/* clang-format off */
#ifndef __clang__
/* @asm open: every instruction is one SHC writes for C */
#pragma inline_asm(entry)
void entry(int x)
{
        MOV.L       #_count,R1          ; count
        RTS
        MOV.L       R4,@R1
}

/* @asm open: every instruction is one SHC writes for C */
#pragma inline_asm(step(size=4))
static void step(void)
{
?loop:
        BRA         ?loop
        NOP
}
#endif
/* clang-format on */

void run(void)
{
    step();
#pragma asm
    ;
#pragma endasm
}

/* clang-format off */
#ifndef __clang__
/* @asm open: 8 bytes reserved */
#pragma asm
        .RES.B      8
        ;
#pragma endasm
#endif
/* clang-format on */
'''


def split(source=SOURCE):
    """OUT and TEXT as inline.py writes them for source, given as a.c, each as its lines."""
    with tree({'a.c': source}):
        inline.main(['a.c', 'a_gcc.c', 'a_inline.src'])
        return Path('a_gcc.c').read_text().split('\n'), Path('a_inline.src').read_text().split('\n')


def line(text, marker):
    return text[:text.index(marker)].count('\n') + 1


# a source with a piece on the shipped side of each of two GCC guards. entry's body is in the #else of an #ifdef GCC
# whose first group defines entry in C. run's block of code is under #ifndef GCC. Unguarded, the split would refuse
# entry's branch to an offset and run's block inside a function. step's body stands under no GCC guard
GUARDED = '''#ifdef GCC
void entry(int x)
{
    count = x;
}
#else
/* clang-format off */
#ifndef __clang__
/* @asm open: every instruction is one SHC writes for C */
#pragma inline_asm(entry)
void entry(int x)
{
_entry:
        MOV.L       #_count,R1
        BSR         _entry+4
        MOV.L       R4,@R1
}
#endif
/* clang-format on */
#endif

/* clang-format off */
#ifndef __clang__
/* @asm open: every instruction is one SHC writes for C */
#pragma inline_asm(step)
static void step(void)
{
        RTS
        NOP
}
#endif
/* clang-format on */

void run(void)
{
#ifndef GCC
#pragma asm
        NOP
#pragma endasm
#endif
    step();
}
'''

# the first two lines of SOURCE's block that reserves 8 bytes
RESERVATION_HEAD = '#pragma asm\n        .RES.B      8\n'
# the first two lines of a block of code, which tests put in run in place of its block of comments
CODE_BLOCK_HEAD = '#pragma asm\n        NOP\n'


class Split(unittest.TestCase):
    maxDiff = None

    def assertRefusesSection(self, source, marker):
        """Fail unless the split refuses the .SECTION on the line where source's first marker stands, and no other
        line. That .SECTION must lie in entry's body."""
        with self.assertRaisesRegex(SystemExit, rf"^.*: a\.c:{line(source, marker)}: entry: a \.SECTION the split "
                                                r"can't take: a body's \.SECTION lines must open it, selecting data "
                                                r"sections and then its code section$"):
            split(source)

    def test_keeps_the_c_line_for_line_with_each_piece_empty(self):
        # OUT empties SOURCE's pieces: each line that starts with #pragma, eight spaces or ?loop:, the ; in run, and
        # each } below a line that starts with eight spaces. It turns the { below entry's head and step's into a ;
        lines = SOURCE.split('\n')
        empty = {i for i, text in enumerate(lines)
                 if text.startswith(('#pragma', ' ' * 8, '?loop:')) or text == '    ;'
                 or (text == '}' and lines[i - 1].startswith(' ' * 8))}
        openings = {i for i, text in enumerate(lines)
                    if text == '{' and lines[i - 1].startswith(('void entry', 'static'))}
        c, _ = split()
        self.assertEqual(c[:len(lines) + 1], ['#line 1 "a.c"'] + [';' if i in openings else '' if i in empty else text
                                                                 for i, text in enumerate(lines)])

    def test_ends_the_c_with_an_asm_that_includes_the_text_and_defines_each_body(self):
        c, _ = split()
        self.assertEqual(c[-3:], ['__asm__(".include \\"a_inline.s\\""', '        : : ":"(entry), ":"(step));', ''])

    def test_writes_each_body_as_r10_defines_it_and_each_block_of_assembly(self):
        _, text = split()
        self.assertEqual(text, [
            f'; a.c:{line(SOURCE, "#pragma inline_asm(entry)")}: entry',
            '        .EXPORT     _entry',
            '_entry:',
            '        MOV.L       #_count,R1          ; count',
            '        RTS',
            '        MOV.L       R4,@R1',
            '        .ALIGN      4',
            '        RTS',
            '        NOP',
            f'; a.c:{line(SOURCE, "#pragma inline_asm(step")}: step',
            '_step:',
            '?loop:',
            '        BRA         ?loop',
            '        NOP',
            '        .ALIGN      4',
            '        RTS',
            '        NOP',
            f'; a.c:{line(SOURCE, RESERVATION_HEAD)}: #pragma asm',
            '        .RES.B      8',
            '        ;',
            '',
        ])

    def test_writes_no_asm_for_blocks_of_comments_alone(self):
        # SOURCE without its bodies or its reservation: run's block of comments alone
        source = SOURCE[:SOURCE.index('#pragma inline_asm(entry)')] + '#endif\n' + SOURCE[SOURCE.index('void run'):]
        source = source[:source.index('/* clang-format off */\n#ifndef __clang__\n/* @asm open: 8 bytes')]
        c, text = split(source)
        self.assertEqual(text, [''])
        self.assertFalse(any('__asm__' in line for line in c))

    def test_leaves_out_a_block_a_guard_keeps_from_gcc(self):
        # SOURCE's reservation goes blank in the C, as any block does, and its guard's first line stays. make gcc's
        # compile defines GCC and not __clang__, and the text takes the block where that compile does. A conditional
        # closed above the block leaves the guard's side as it was
        block = SOURCE[SOURCE.index('#ifndef __clang__\n/* @asm open: 8 bytes'):SOURCE.rindex('/* clang-format on */')]
        cases = (
            ("an #ifndef GCC's first group", f'#ifndef GCC\n{block}#endif\n', False),
            ("an #ifdef GCC's #else", f'#ifdef GCC\n#else\n{block}#endif\n', False),
            ("an #ifdef GCC's first group", f'#ifdef GCC\n{block}#else\n#endif\n', True),
            ("an #ifdef GCC's #elif", f'#ifdef GCC\n#elif 1\n{block}#endif\n', False),
            ("an #ifdef GCC's #else after an #elif", f'#ifdef GCC\n#elif 1\n#else\n{block}#endif\n', False),
            ("an #ifndef GCC's #else", f'#ifndef GCC\n#else\n{block}#endif\n', True),
            ('below a guard that ends above it', f'#ifndef GCC\n#endif\n{block}', True),
            ("an #ifdef GCC's #else, below a conditional nested there",
             f'#ifdef GCC\n#else\n#if 1\n#endif\n{block}#endif\n', False),
            ("an #ifdef __clang__'s first group", f'#ifdef __clang__\n{block}#endif\n', False),
            ("an #ifndef __clang__'s #else", f'#ifndef __clang__\n#else\n{block}#endif\n', False),
            ("an #ifdef __clang__'s #else", f'#ifdef __clang__\n#else\n{block}#endif\n', True),
        )
        for where, guard, kept in cases:
            with self.subTest(where):
                source = SOURCE.replace(block, guard)
                c, text = split(source)
                self.assertIn(guard.split('\n')[0], c)
                self.assertNotIn('        .RES.B      8', c)
                self.assertEqual('        .RES.B      8' in text, kept)

    def test_writes_the_label_of_a_body_after_the_sections_it_opens_with(self):
        # entry's body imports _count, then reserves 8 bytes in B and selects C, where its code runs
        source = SOURCE.replace('{\n        MOV.L', '{\n        .IMPORT     _count\n'
                                '        .SECTION    B,DATA,ALIGN=4\nbuffer:\n        .RES.B      8\n'
                                '        .SECTION    C,CODE,ALIGN=4\n        MOV.L')
        _, text = split(source)
        self.assertEqual(text[:9], [
            f'; a.c:{line(source, "#pragma inline_asm(entry)")}: entry',
            '        .EXPORT     _entry',
            '        .IMPORT     _count',
            '        .SECTION    B,DATA,ALIGN=4',
            'buffer:',
            '        .RES.B      8',
            '        .SECTION    C,CODE,ALIGN=4',
            '_entry:',
            '        MOV.L       #_count,R1          ; count',
        ])

    def test_refuses_a_section_after_the_code_section_a_body_opens_with(self):
        # entry's body opens with B and then C, as it may, and selects D or E after its code. The split names that
        # .SECTION alone
        opened = SOURCE.replace('{\n        MOV.L', '{\n        .SECTION    B,DATA,ALIGN=4\n'
                                '        .SECTION    C,CODE,ALIGN=4\n        MOV.L')
        for after in ('D,DATA', 'E,CODE'):
            with self.subTest(after):
                source = opened.replace('        MOV.L       R4,@R1\n',
                                        f'        MOV.L       R4,@R1\n        .SECTION    {after},ALIGN=4\n')
                self.assertRefusesSection(source, f'.SECTION    {after}')

    def test_refuses_a_body_whose_code_would_run_in_a_data_section(self):
        source = SOURCE.replace('{\n        MOV.L', '{\n        .SECTION    D,DATA,ALIGN=4\n        MOV.L')
        self.assertRefusesSection(source, '.SECTION')

    def test_refuses_a_body_that_places_a_statement_above_its_sections(self):
        # the split refuses the .SECTION, as TEXT would put the NOP in P, apart from entry's label and code in C
        source = SOURCE.replace('{\n        MOV.L', '{\n        NOP\n        .SECTION    C,CODE,ALIGN=4\n        MOV.L')
        self.assertRefusesSection(source, '.SECTION')

    def test_refuses_a_body_whose_section_gives_no_kind(self):
        # B's .SECTION doesn't say whether B holds data, though C's code section follows it
        source = SOURCE.replace('{\n        MOV.L', '{\n        .SECTION    B\n        .SECTION    C,CODE,ALIGN=4\n'
                                '        MOV.L')
        self.assertRefusesSection(source, '.SECTION    B')

    def test_selects_p_again_between_a_piece_that_selects_a_section_and_the_next(self):
        # a block above entry selects D, and entry's body B and then C, so TEXT selects P after each. step's body
        # selects no section, so TEXT selects none after it. The reservation selects B, but it is the last piece
        table = '#pragma asm\n        .SECTION    D,DATA,ALIGN=4\ntable:\n        .DATA.L     0\n#pragma endasm\n'
        reserved = '#pragma asm\n        .SECTION    B,DATA,ALIGN=4\n        .RES.B      8\n'
        source = SOURCE.replace('static void step(void);\n', f'static void step(void);\n{table}')
        source = source.replace('{\n        MOV.L', '{\n        .SECTION    B,DATA,ALIGN=4\nbuffer:\n'
                                '        .RES.B      8\n        .SECTION    C,CODE,ALIGN=4\n        MOV.L')
        source = source.replace(RESERVATION_HEAD, reserved)
        select = '        .SECTION    P,CODE,ALIGN=4'
        _, text = split(source)
        self.assertEqual([each for each in text if each.startswith('; ') or each == select], [
            f'; a.c:{line(source, table)}: #pragma asm',
            select,
            f'; a.c:{line(source, "#pragma inline_asm(entry)")}: entry',
            select,
            f'; a.c:{line(source, "#pragma inline_asm(step")}: step',
            f'; a.c:{line(source, reserved)}: #pragma asm',
        ])
        self.assertEqual(text[-4:], ['        .SECTION    B,DATA,ALIGN=4', '        .RES.B      8', '        ;', ''])

    def test_refuses_a_branch_to_other_than_a_label_in_a_body_or_a_block(self):
        # a place before entry, one inside the C function run, one the parentheses hide, and a name past a label's.
        # SOURCE's reservation branches too, in its place
        for target in ('_entry-32', '_run+8', '(_run+8)', 'run.part+4', '_run + 8'):
            for piece, where in (('step', '        BRA         ?loop\n'), ('#pragma asm', '        .RES.B      8\n')):
                with self.subTest(target=target, piece=piece):
                    source = SOURCE.replace(where, f'        BSR         {target}\n')
                    with self.assertRaisesRegex(SystemExit, rf"\A[^\n]*: a\.c:{line(source, '        BSR')}: "
                                                            rf"{re.escape(piece)}: BSR         {re.escape(target)}, a "
                                                            r"branch to other than a label, which may be a place in "
                                                            r"SHC's layout that GCC's doesn't keep\Z"):
                        split(source)

    def test_takes_a_branch_to_a_label_whatever_its_comment_holds(self):
        for piece, where in (('step', '        BRA         ?loop\n'), ('#pragma asm', '        .RES.B      8\n')):
            with self.subTest(piece):
                source = SOURCE.replace(where, '?back:  BT/S        ?back          ; not _run+8\n')
                _, text = split(source)
                self.assertIn('?back:  BT/S        ?back          ; not _run+8', text)

    def test_takes_a_branch_to_a_function_of_the_file(self):
        # run is a C function, and entry another body, whose label TEXT defines
        for target in ('_run', '_entry'):
            with self.subTest(target):
                _, text = split(SOURCE.replace('        BRA         ?loop\n', f'        BSR         {target}\n'))
                self.assertIn(f'        BSR         {target}', text)

    def test_refuses_a_block_of_code_inside_a_function(self):
        # TEXT leaves out SOURCE's block in run, which holds a comment alone. The split would move a block of code out
        # of run
        source = SOURCE.replace('#pragma asm\n    ;\n', CODE_BLOCK_HEAD)
        at = line(source, CODE_BLOCK_HEAD)
        with self.assertRaisesRegex(SystemExit, rf'(?m)a\.c:{at}: run: a #pragma asm block of code inside the '
                                                'function, which the split would move out of it$'):
            split(source)

    def test_leaves_each_piece_on_the_shipped_side_of_a_gcc_guard_to_shc(self):
        # OUT keeps GCC's entry and empties each piece's lines. TEXT and the asm hold step alone
        c, text = split(GUARDED)
        self.assertEqual(c[1:6], ['#ifdef GCC', 'void entry(int x)', '{', '    count = x;', '}'])
        self.assertFalse(any(line.startswith(('#pragma', '_entry:', ' ' * 8)) for line in c[:-3]), c)
        self.assertEqual(text, [f'; a.c:{line(GUARDED, "#pragma inline_asm(step)")}: step', '_step:', '        RTS',
                                '        NOP', '        .ALIGN      4', '        RTS', '        NOP', ''])
        self.assertEqual(c[-3:], ['__asm__(".include \\"a_inline.s\\""', '        : : ":"(step));', ''])

    def test_takes_a_body_in_each_group_gcc_compiles(self):
        # TEXT takes entry's body in an #ifdef GCC's first group, in an #ifndef GCC's #else, and in the group of
        # __clang__'s test GCC takes
        guards = {'#ifdef GCC\n': '#endif\n', '#ifndef GCC\n#else\n': '#endif\n',
                  '#ifdef __clang__\n#else\n': '#endif\n', '#ifndef __clang__\n': '#else\n#endif\n'}
        for opening, closing in guards.items():
            with self.subTest(opening):
                source = SOURCE.replace('#pragma inline_asm(entry)\n', f'{opening}#pragma inline_asm(entry)\n')
                source = source.replace('        MOV.L       R4,@R1\n}\n', f'        MOV.L       R4,@R1\n}}\n{closing}')
                _, text = split(source)
                self.assertIn('_entry:', text)

    def test_takes_a_block_of_code_inside_a_function_a_guard_keeps_from_gcc(self):
        # the block is on the shipped side, so TEXT moves nothing out of run
        source = SOURCE.replace('#pragma asm\n    ;\n#pragma endasm\n',
                                f'#ifdef GCC\n#else\n{CODE_BLOCK_HEAD}#pragma endasm\n#endif\n')
        c, text = split(source)
        self.assertNotIn('        NOP', c)
        self.assertEqual([piece for piece in text if piece.startswith('; ') and '#pragma asm' in piece],
                         [f'; a.c:{line(source, RESERVATION_HEAD)}: #pragma asm'])

    def test_refuses_a_piece_under_a_conditional_it_cant_read(self):
        # make gcc's compile may or may not take a group of #if, of an #ifdef or #ifndef of another macro, or of an
        # #elif below a group it may not take, or an #else below one; GCC's side of a guard inside such a group
        # doesn't settle it
        for opening in ('#if 0\n', '#ifdef SHC\n', '#ifndef GCC\n#elif 1\n', '#if 1\n#else\n', '#if 1\n#ifdef GCC\n'):
            closing = '#endif\n' * opening.count('#if')
            with self.subTest(opening):
                source = SOURCE.replace('#pragma asm\n        .RES.B', f'{opening}#pragma asm\n        .RES.B')
                source = source.replace('#pragma endasm\n#endif\n/* clang-format on */\n',
                                        f'#pragma endasm\n{closing}#endif\n/* clang-format on */\n')
                with self.assertRaisesRegex(SystemExit, rf"\A[^\n]*: a\.c:{line(source, RESERVATION_HEAD)}: #pragma asm: a "
                                                        r"block under a conditional the tool can't read\Z"):
                    split(source)
                source = SOURCE.replace('#ifndef __clang__\n/* @asm open: every', f'{opening}/* @asm open: every', 1)
                source = source.replace('#endif\n', closing, 1)
                with self.assertRaisesRegex(SystemExit, rf"a\.c:{line(source, '#pragma inline_asm(entry)')}: entry: a "
                                                        rf"body under a conditional the tool can't read\n[^\n]*: "
                                                        rf"a\.c:{line(source, '#pragma inline_asm(step')}: step: "):
                    split(source)

    def test_takes_comments_alone_or_a_piece_gcc_skips_under_a_conditional_it_cant_read(self):
        # run's block of comments writes nothing under #if 0, and GCC takes neither side of an #if under #ifndef GCC
        comments = SOURCE.replace('#pragma asm\n    ;\n#pragma endasm\n',
                                  '#if 0\n#pragma asm\n    ;\n#pragma endasm\n#endif\n')
        _, text = split(comments)
        self.assertIn('        .RES.B      8', text)
        shipped = SOURCE.replace(RESERVATION_HEAD, f'#ifndef GCC\n#if 0\n#else\n{RESERVATION_HEAD}')
        shipped = shipped.replace('#pragma endasm\n#endif\n/*', '#pragma endasm\n#endif\n#endif\n#endif\n/*')
        _, text = split(shipped)
        self.assertIn('_entry:', text)
        self.assertNotIn('        .RES.B      8', text)

    def test_refuses_a_directive_inside_a_piece(self):
        # TEXT holds assembly alone, outside the preprocessor: a directive in a body, between a body's #pragma and
        # its function, or in a block, and so in a piece GCC doesn't compile, whose lines the C blanks too
        for piece, where, directive in (('entry', '        RTS\n', '#undef X\n'),
                                        ('entry', 'void entry', '#define X\n'),
                                        ('#pragma asm', '        .RES.B', '#line 9\n')):
            for guard in ('__clang__', 'GCC'):
                with self.subTest(where=where, guard=guard):
                    source = SOURCE.replace(where, directive + where, 1)
                    source = source.replace('#ifndef __clang__', f'#ifndef {guard}')
                    with self.assertRaisesRegex(SystemExit, rf"\A[^\n]*: a\.c:{line(source, directive)}: {piece}: a "
                                                            r"directive inside the piece, which the split can't carry "
                                                            r"into TEXT\Z"):
                        split(source)

    def test_refuses_a_pragma_inline_asm_with_no_function_after_it(self):
        # on either side of a guard. The failure names step's branch to an offset too
        late = '/* @asm open: every instruction is one SHC writes for C */\n#pragma inline_asm(late)\n'
        for where, tail in (('unguarded', late), ('under #ifndef GCC', f'#ifndef GCC\n{late}#endif\n')):
            with self.subTest(where):
                source = SOURCE.replace('        BRA         ?loop\n', '        BSR         _run+8\n') + tail
                with self.assertRaises(SystemExit) as refused:
                    split(source)
                self.assertRegex(str(refused.exception), rf'(?m)a\.c:{line(source, "#pragma inline_asm(late)")}: '
                                                         r'#pragma inline_asm\(late\), with no function after it$')
                self.assertRegex(str(refused.exception), rf"(?m)a\.c:{line(source, '        BSR')}: step: BSR\s+"
                                                         r"_run\+8, a branch to other than a label, which may be a "
                                                         r"place in SHC's layout that GCC's doesn't keep$")

    def test_refuses_a_pragma_that_names_no_whole_piece_of_its_own(self):
        # a #pragma above another function, or above an initializer; one spelled otherwise; a second over one body; a
        # #pragma asm with no #pragma endasm, or one inside a body; a #pragma endasm with no #pragma asm; and a body
        # that runs to the end. Each fails on either side of a guard, as the C blanks a piece GCC skips too
        entry, step, endasm = '#pragma inline_asm(entry)\n', '#pragma inline_asm(step(size=4))\n', '#pragma endasm\n'
        late = '#pragma inline_asm(late)\nvoid late(void)\n{\n        RTS\n'
        fake = SOURCE.replace(entry, '#pragma inline_asm(fake)\n')
        entries = SOURCE.replace('void entry(int x)\n', 'int entries[2] =\n')
        spelled = SOURCE.replace(entry, '#pragma inline_asm entry\n')
        twice = SOURCE.replace(step, step * 2)
        unended = SOURCE.replace(f'        ;\n{endasm}#endif\n', '        ;\n#endif\n')
        inside = SOURCE.replace('        RTS\n        MOV', '#pragma asm\n        RTS\n        MOV')
        stray = SOURCE + endasm
        unclosed = SOURCE + late
        for source, problem in (
                (fake, f'{line(fake, "inline_asm(fake)")}: #pragma inline_asm(fake), whose function below is entry'),
                (entries, f"{line(entries, entry)}: #pragma inline_asm(entry), whose function below is one the tool "
                          "can't name"),
                (spelled, f'{line(spelled, "#pragma inline_asm entry")}: not #pragma inline_asm(<function>) or '
                          '#pragma inline_asm(<function>(size=<n>))'),
                (twice, f'{line(twice, step) + 1}: #pragma inline_asm(step), inside the piece at line '
                        f'{line(twice, step)}'),
                (unended, f'{line(unended, RESERVATION_HEAD)}: #pragma asm, with no #pragma endasm below it'),
                (inside, f'{line(inside, "#pragma asm")}: #pragma asm, inside the piece at line {line(inside, entry)}'),
                (stray, f'{stray.count(chr(10))}: #pragma endasm, closing no #pragma asm'),
                (unclosed, f'{line(unclosed, late)}: #pragma inline_asm(late), with no }} closing its function')):
            for guard in ('__clang__', 'GCC'):
                with self.subTest(problem=problem, guard=guard):
                    with self.assertRaises(SystemExit) as refused:
                        split(source.replace('#ifndef __clang__', f'#ifndef {guard}'))
                    self.assertIn(f'a.c:{problem}', str(refused.exception))

    def test_refuses_a_path_the_c_would_have_to_escape_before_writing(self):
        # OUT's #line and .include take the paths as they are: a quote, a backslash or a newline would end or change
        # the C's string, and a % the asm's template
        for source, text in (('a"b.c', 'a_inline.src'), ('a\\b.c', 'a_inline.src'), ('a\nb.c', 'a_inline.src'),
                             ('a.c', 'a%_inline.src'), ('a.c', 'my dir/a_inline.src')):
            with self.subTest(source=source, text=text), tree({source: SOURCE}):
                # the failure names the path as it is, over two lines where it holds a newline
                refused = (source if source != 'a.c' else str(Path(text).with_suffix('.s'))).split('\n')[-1]
                with self.assertRaisesRegex(SystemExit, rf"[^\n]*: {re.escape(refused)}: holds other than letters, "
                                                        r"digits, _, \., / and -, so OUT can't write it unescaped\Z"):
                    inline.main([source, 'a_gcc.c', text])
                self.assertFalse(Path('a_gcc.c').exists() or Path(text).exists())

    def test_writes_nothing_where_it_refuses(self):
        with tree({'a.c': SOURCE.replace('        BRA         ?loop\n', '        BSR         _run+8\n')}):
            with self.assertRaises(SystemExit):
                inline.main(['a.c', 'a_gcc.c', 'a_inline.src'])
            self.assertFalse(Path('a_gcc.c').exists() or Path('a_inline.src').exists())


if __name__ == '__main__':
    unittest.main()
