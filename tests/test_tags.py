import contextlib
import io
import re
import unittest

import tags
from support import MODULE, tree

# an assembly source, c.src, that holds c_entry and c_next
SRC = '''        .EXPORT     _c_entry
        .EXPORT     _c_next
        .EXPORT     _c_data

        .SECTION    P,CODE,ALIGN=4

; void c_entry(void)
; @asm opcode: cache-line, OCBI @R4 at 0x0c900044 invalidates a line
_c_entry:
        OCBI        @R4
        OCBI        @R4
; the tail c_entry runs on into, which the tables don't name
c_tail:
        RTS
        MOV.L       @R15+,R4
; void c_next(void)
; @asm open: every instruction is one SHC writes for C
_c_next:
        RTS
        NOP
; Uint32 c_data
_c_data:
        .DATA.L     0
        .END
'''

# SRC with c_tail exported, in a list, so that it starts a function the tables don't name, and its tag
TAIL = SRC.replace('        .EXPORT     _c_next\n', '        .EXPORT     _c_next, c_tail\n').replace(
    'c_tail:\n', ('; @asm abi: stack-above-entry, its RTS at 0x0c900048 returns with R15 4\n'
                  '; bytes above its entry\n'
                  'c_tail:\n'))

# a module whose a.c holds host, outer and outline, whose other source, b.c, holds entry, and whose c.src holds
# c_entry and c_next
TABLES = dict(MODULE, **{
    'config/mod/splits.txt': ('P  0x0c900000  0x0c900100\n'
                              'modules/mod/a.c  P  0x0c900000  0x0c900020\n'
                              'modules/mod/b.c  P  0x0c900020  0x0c900040\n'
                              'modules/mod/c.src  P  0x0c900040  0x0c900060\n'),
    'config/mod/symbols.txt': ('0x0c900000  host\n0x0c900008  outer\n0x0c900010  outline\n0x0c900020  entry\n'
                               '0x0c900040  c_entry\n0x0c900050  c_next\n0x0c900058  c_data\n'),
    'modules/mod/b.c': '',
    'modules/mod/c.src': SRC,
})

# an inline body written out of line, one only expanded in host, and host
C = '''/* clang-format off */
#ifndef __clang__
/* return */
/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x0c900014 follows
 * the body's own return at 0x0c900010 */
#pragma inline_asm(outline)
void outline(void)
{
        RTS
        NOP
}

/* @asm opcode: cache-line, OCBI @R4 at 0x0c900004, in host, invalidates a line */
#pragma inline_asm(expanded)
static void expanded(void *line)
{
        OCBI        @R4
}
#endif
/* clang-format on */

void host(void)
{
    expanded(0);
}
'''


# C with a block that reserves 8 bytes after host, and a block of comments only
BLOCKS = C + '''
/* clang-format off */
#ifndef __clang__
/* @asm open: 8 bytes reserved after host, from 0x0c900018 */
#pragma asm
        .RES.B      8
        ;
#pragma endasm
#endif
/* clang-format on */

void later(void)
{
#pragma asm
    ;
#pragma endasm
}
'''

# C with hacks under their @hack tags, one in a #define, two above one if
# (0), the redundant one at the top, and a shipped bug under its @bug tag
HACKED = C + r'''
#define WEIGH(value)                                                           \
    /* @hack redundant: a test of value that never runs; without it, 3 of 9   \
     * words differ */                                                         \
    if (0) {                                                                   \
        (void)(value);                                                         \
    }

void outer(int x)
{
    /* @hack redundant: a never-run call; without it, 2 of 8 words differ */
    if (0) {
        host();
    }
    /* @hack redundant: the tests that never run; without them, 4 of 8 words
     * differ */
    /* @hack spelling: the empty else; without it, 1 of 8 words differs */
    if (0) {
        if (x) {
        }
    } else {
    }
    /* @bug memory: a read through a null pointer, at 0x0c900008 */
    WEIGH(*(int *)0);
}
'''

# the never-run call's tag, and the if (0) under it
CALL = ('    /* @hack redundant: a never-run call; without it, 2 of 8 words differ */\n'
        '    if (0) {\n')


def check(c=C, src=SRC):
    with tree(dict(TABLES, **{'modules/mod/a.c': c, 'modules/mod/c.src': src})):
        tags.main(['modules/mod/a.c', 'modules/mod/c.src'])


def reviewed(c=C, src=SRC, symbols=TABLES['config/mod/symbols.txt']):
    """What --review prints for a.c and c.src, as its lines."""
    files = {'modules/mod/a.c': c, 'modules/mod/c.src': src, 'config/mod/symbols.txt': symbols}
    with tree(dict(TABLES, **files)), contextlib.redirect_stdout(io.StringIO()) as out:
        tags.main(['--review', 'modules/mod/a.c', 'modules/mod/c.src'])
    return out.getvalue().splitlines()


def review(c=C, src=SRC, symbols=TABLES['config/mod/symbols.txt']):
    """What --review lists for a.c and c.src, as its lines, without the count it ends on."""
    *listed, count = reviewed(c, src, symbols)
    assert re.fullmatch(r'\d+ of \d+ inline bodies, .*', count), count
    return listed


def line(text, marker):
    """The number of the line of text where marker starts."""
    return text[:text.index(marker)].count('\n') + 1


# what --review lists for c_next's open tag
NEXT = f'modules/mod/c.src:{line(SRC, "_c_next:")}: _c_next: every instruction is one SHC writes for C'


class Tags(unittest.TestCase):
    def rejects(self, pattern, c=C, src=SRC):
        with self.assertRaisesRegex(SystemExit, pattern):
            check(c, src)

    def test_passes_a_tag_at_every_site(self):
        check()
        # and --review lists the open one
        self.assertEqual(review(), [NEXT])

    def test_wants_a_kind_and_a_text(self):
        form = r'(?m)a\.c:4: not @asm <kind>: <text>, with a kind of opcode, operand, abi, epilogue, name, open$'
        self.rejects(form, C.replace('epilogue: dead-return', 'epilogue:dead-return'))
        for kind in ('producer', 'record', 'trailer', 'no-basis'):
            self.rejects(form, C.replace('epilogue: dead-return, SHC', f'{kind}: SHC'))
        self.rejects(r'a\.c:13: not @asm <kind>',
                     C.replace('opcode: cache-line, OCBI @R4 at 0x0c900004, in host, invalidates a line', 'opcode'))
        # a colon after its name
        self.rejects(form, C.replace('@asm epilogue:', '@asm: epilogue:'))

    def test_wants_a_tag_above_each_inline_body(self):
        self.rejects(r'(?m)a\.c:14: #pragma inline_asm\(expanded\) has no @asm tag$',
                     C.replace('/* @asm opcode', '/* opcode'))

    def test_wants_a_tag_directly_above_an_inline_body(self):
        self.rejects(r'a\.c:13: an @asm tag not directly above a #pragma inline_asm',
                     C.replace('in host, invalidates a line */\n', 'in host, invalidates a line */\n\n'))
        # a second tag is not directly above it
        self.rejects(r'a\.c:13: an @asm tag not directly above a #pragma inline_asm',
                     C.replace('/* @asm opcode', '/* @asm open: one */\n/* @asm opcode'))

    def test_wants_one_tag_above_a_body_or_a_block(self):
        # two tags ending on the line above a body, either one first, or above a block
        for old, new in (('/* @asm opcode', '/* @asm open: one */ /* @asm opcode'),
                         ('invalidates a line */', 'invalidates a line */ /* @asm open: one */')):
            self.rejects(r'(?m)a\.c:13: a second @asm tag for #pragma inline_asm\(expanded\)$', C.replace(old, new))
        self.rejects(rf'(?m)a\.c:{line(BLOCKS, "/* @asm open: 8")}: a second @asm tag for #pragma asm$',
                     BLOCKS.replace('/* @asm open: 8', '/* @asm open: one */ /* @asm open: 8'))

    def test_reads_a_body_given_its_size(self):
        sized = C.replace('#pragma inline_asm(expanded)', '#pragma inline_asm(expanded(size=2))')
        check(sized)
        self.rejects(r'a\.c:14: #pragma inline_asm\(expanded\) has no @asm tag',
                     sized.replace('/* @asm opcode', '/* opcode'))
        self.rejects(r'a\.c:13: 0x0c900014 is outside its function', sized.replace('0x0c900004', '0x0c900014'))

    def test_reads_a_body_or_a_block_only_at_its_directive(self):
        # a comment, a string and a block comment that mention a body or a block open neither
        check(C + '/* Unlike #pragma inline_asm(outline), this is C */\n'
                  'static const char *note = "\\\n#pragma inline_asm(outline)";\n'
                  '/*\n#pragma inline_asm(outline)\n#pragma asm\n        .RES.B      4\n#pragma endasm\n */\n')
        # one indented, or spaced after its #, is a directive
        self.rejects(r'a\.c:14: #pragma inline_asm\(expanded\) has no @asm tag',
                     C.replace('/* @asm opcode', '/* opcode').replace('#pragma inline_asm(expanded)',
                                                                      '  #  pragma inline_asm(expanded)'))

    def test_names_a_pragma_inline_asm_it_can_t_read(self):
        # SHC takes the first two under -code=asmcode; tags.py reads neither, so their bodies' tags would go unchecked.
        # the last two hold a size or a name past the closing parenthesis, which tags.py doesn't read either
        for spelling in ('#pragma inline_asm (expanded)', '#pragma inline_asm(expanded, outline)',
                         '#pragma inline_asm(expanded)(size=2)', '#pragma inline_asm(expanded) outline'):
            self.rejects(r'(?m)a\.c:14: not #pragma inline_asm\(<function>\) or '
                         r'#pragma inline_asm\(<function>\(size=<n>\)\)$',
                         C.replace('#pragma inline_asm(expanded)', spelling))

    def test_wants_a_function_below_each_pragma_inline_asm(self):
        headless = C + '/* @asm open: nothing */\n#pragma inline_asm(none)\n'
        self.rejects(rf'(?m)a\.c:{line(headless, "#pragma inline_asm(none)")}: #pragma inline_asm\(none\), with no '
                     r'function after it$', headless)

    def test_wants_no_tag_but_above_an_inline_body(self):
        self.rejects(r'a\.c:22: an @asm tag not directly above a #pragma inline_asm',
                     C.replace('void host(void)', '/* @asm open: x */\nvoid host(void)'))

    def test_reads_a_tag_only_where_a_comment_opens_one(self):
        check(C.replace('/* return */', '/* return; see @asm below */'))
        check(C + 'static const char *note = "/* @asm bogus */";\n')
        # nor by a longer word
        for word in ('@hacks', '@bugs', '@asmx'):
            check(C.replace('/* return */', f'/* {word} jump: a word, not a tag */'))
        self.rejects(r'(?m)a\.c:3: not @asm <kind>: <text>, with a kind of',
                     C.replace('/* return */', '/* @asm below */'))

    def test_wants_each_address_in_its_function(self):
        self.rejects(r'a\.c:4: 0x0c900020 is outside its function', C.replace('0x0c900014', '0x0c900020'))
        # an expanded body's addresses are its callers', directly or through macros, and its tag needn't name them
        self.rejects(r'a\.c:13: 0x0c900014 is outside its function', C.replace('0x0c900004', '0x0c900014'))
        bare = C.replace(', in host,', ',')
        macros = '#define EXPAND() \\\n    expanded(0)\n#define TWICE() \\\n    EXPAND()\n'
        for call in ('expanded(0)', 'EXPAND()', 'TWICE()'):
            check(bare.replace('    expanded(0);', f'    {call};') + macros)
        # but not a function that calls a host
        self.rejects(r'a\.c:13: 0x0c900008 is outside its function',
                     C.replace('0x0c900004', '0x0c900008') + '\nvoid outer(void)\n{\n    host();\n}\n')
        # nor one that calls a longer name, or names the body only in a comment or a literal
        self.rejects(r'a\.c:13: 0x0c900004 is outside its function',
                     bare.replace('    expanded(0);', '    unexpanded(0);'))
        for mention in ('/* unlike expanded(), this reads no line */', 'static const char *s = "expanded()";'):
            self.rejects(r'a\.c:13: 0x0c900008 is outside its function',
                         bare.replace('0x0c900004', '0x0c900008') + f'\nvoid outer(void)\n{{\n    {mention}\n}}\n')
        # a macro goes on past a comment that runs across its lines
        check(bare.replace('    expanded(0);', '    EXPAND();') +
              '#define EXPAND() \\\n    /* the body, \\\n     * expanded */ \\\n    expanded(0)\n')
        # an address in capitals too
        self.rejects(r'a\.c:4: 0x0c900020 is outside its function', C.replace('0x0c900014', '0X0C900020'))
        # an address may lie in another function of its source that the tag names, but not in another source's
        check(C.replace('0x0c900014 follows', '0x0c900014 follows host\'s 0x0c900004 and'))
        self.rejects(r'a\.c:4: 0x0c900020 is outside its function',
                     C.replace('0x0c900014 follows', '0x0c900014 follows entry\'s 0x0c900020 and'))
        # nor in a datum of its source that the tag names
        splits = TABLES['config/mod/splits.txt'].replace('modules/', 'D  0x0c900100  0x0c900200\nmodules/', 1)
        data = {'config/mod/splits.txt': splits + 'modules/mod/a.c  D  0x0c900100  0x0c900110\n',
                'config/mod/symbols.txt': TABLES['config/mod/symbols.txt'] + '0x0c900100  table\n',
                'modules/mod/a.c': C.replace('0x0c900014 follows', '0x0c900014 follows table\'s 0x0c900104 and')}
        with tree(dict(TABLES, **data)):
            with self.assertRaisesRegex(SystemExit, r'a\.c:4: 0x0c900104 is outside its function'):
                tags.main(['modules/mod/a.c'])

    def test_reads_a_function_only_where_a_brace_opens_one(self):
        # an initializer's braces at the margin, as clang-format off leaves them, before a function or at the end,
        # with a ( in its declaration, or with no word at the margin above them
        steps = '/* clang-format off */\nstatic const int steps[] =\n{\n    1, 2,\n};\n'
        check(C.replace('void host(void)', steps + 'void host(void)'))
        check(C + steps)
        check(C + '/* clang-format off */\nstatic void (*const calls[])(void) =\n{\n    host,\n};\n')
        check(steps.replace('static', '    static') + C)

    def test_wants_an_address_in_every_kind_but_open(self):
        for kind, opening in (('opcode', 'cache-line, '), ('operand', 'small-constant, '), ('abi', 'stack-written, '),
                              ('epilogue', 'fill, '), ('name', 'vmsasm_, ')):
            self.rejects(rf'a\.c:13: @asm {kind} cites no address',
                         C.replace('opcode: cache-line, OCBI @R4 at 0x0c900004,', f'{kind}: {opening}OCBI @R4'))
        self.rejects(r'a\.c:4: @asm epilogue cites no address',
                     C.replace('at 0x0c900014 follows\n * the body\'s own return at 0x0c900010', 'follows its return'))
        check(C.replace('opcode: cache-line, OCBI @R4 at 0x0c900004,', 'open: no rule reads its OCBI @R4'))

    def test_takes_each_epilogue_and_name_evidence(self):
        for evidence in ('dead-return', 'fill'):
            check(C.replace('epilogue: dead-return,', f'epilogue: {evidence},'))
        for member in ('vmsasm_', 'lcdasm_', 'mpasm_', 'kmasm_workaround_for_sh_lnk_', 'kmasm_kamui_lib_'):
            check(C.replace('epilogue: dead-return,', f'name: {member},'))
        # in a .src file too, where it settles its label
        named = SRC.replace('; @asm opcode: cache-line, OCBI @R4 at', '; @asm name: lcdasm_, OCBI @R4 at')
        check(src=named)
        self.assertEqual(review(src=named), [NEXT])

    def test_wants_an_epilogue_or_name_tag_s_evidence(self):
        form = r'(?m)a\.c:4: not @asm epilogue: <evidence>, <text>, with evidence of dead-return, fill$'
        for wrong in ('epilogue: SHC', 'epilogue: trailer, SHC', 'epilogue: dead-return SHC', 'epilogue: Fill, SHC',
                      'epilogue: dead-return,SHC', 'epilogue: fills, SHC', 'epilogue: vmsasm_, SHC'):
            self.rejects(form, C.replace('epilogue: dead-return, SHC', wrong))
        self.rejects(r'(?m)c\.src:8: not @asm name: <evidence>, <text>, with evidence of vmsasm_, lcdasm_, mpasm_, '
                     r'kmasm_workaround_for_sh_lnk_, kmasm_kamui_lib_$',
                     src=SRC.replace('; @asm opcode:', '; @asm name:'))

    def test_takes_each_kind_s_rules(self):
        rules = {'opcode': 'rotcl rotcr swap dmuls dmulu movca fschg cache-line sett',
                 'operand': 'fipr-registers ftrv-registers fsca-registers small-constant zero-displacement '
                            'fschg-pairs',
                 'abi': 'stack-written fpscr-save stack-above-entry return-below-entry register-contract '
                        'exception-entry exception-continuation'}
        for kind, words in rules.items():
            for rule in words.split():
                check(C.replace('opcode: cache-line,', f'{kind}: {rule},'))
                check(src=SRC.replace('; @asm opcode: cache-line,', f'; @asm {kind}: {rule},'))

    def test_wants_a_rule_of_the_tag_s_kind(self):
        form = (r'(?m)a\.c:13: not @asm opcode: <rule>, <text>, with a rule of rotcl, rotcr, swap, dmuls, dmulu, '
                r'movca, fschg, cache-line, sett$')
        for wrong in ('OCBI', 'cache-line OCBI', 'Cache-line, OCBI', 'cache-line,OCBI', 'cache-lines, OCBI',
                      'fschg-pairs, OCBI', 'epilogue, OCBI'):
            self.rejects(form, C.replace('opcode: cache-line, OCBI', f'opcode: {wrong}'))
        # in a .src file too, by its kind
        self.rejects(r'(?m)c\.src:8: not @asm operand: <rule>, <text>, with a rule of fipr-registers, ',
                     src=SRC.replace('; @asm opcode:', '; @asm operand:'))
        self.rejects(r'(?m)c\.src:13: not @asm abi: <rule>, <text>, with a rule of stack-written, ',
                     src=TAIL.replace('abi: stack-above-entry,', 'abi:'))

    def test_names_a_missing_file(self):
        with tree(TABLES), self.assertRaisesRegex(SystemExit, r'modules/mod/none\.c: No such file or directory'):
            tags.main(['modules/mod/none.c'])

    def test_passes_a_tag_above_an_assembly_block(self):
        check(BLOCKS)

    def test_wants_a_tag_above_each_assembly_block(self):
        self.rejects(rf'(?m)a\.c:{line(BLOCKS, "#pragma asm")}: #pragma asm has no @asm tag$',
                     BLOCKS.replace('/* @asm open: 8 bytes', '/* 8 bytes'))

    def test_wants_no_tag_above_a_block_of_comments(self):
        self.rejects(r'an @asm tag not directly above a #pragma inline_asm or a #pragma asm block',
                     BLOCKS.replace('{\n#pragma asm', '{\n/* @asm open: x */\n#pragma asm'))

    def test_wants_a_block_s_addresses_in_its_source_s_rows(self):
        self.rejects(r"a\.c:\d+: 0x0c900020 is outside its source's rows",
                     BLOCKS.replace('from 0x0c900018', 'from 0x0c900020'))

    def test_wants_a_hack_tag_above_each_if_0(self):
        check(HACKED)
        at = line(HACKED, CALL) + 1
        self.rejects(rf'(?m)a\.c:{at}: if \(0\) has no @hack redundant tag above it$',
                     HACKED.replace(CALL, '    /* a never-run call */\n    if (0) {\n'))
        self.rejects(rf'a\.c:{at - 1}: if \(0\) has no @hack redundant tag above it',
                     HACKED.replace(CALL, '    if (0) {\n'))
        # in a #define too, on a line of its own or the directive's
        at = line(HACKED, '    if (0) {        ')
        self.rejects(rf'a\.c:{at}: if \(0\) has no @hack redundant tag above it',
                     HACKED.replace('/* @hack redundant: a test of value', '/* a test of value'))
        never = HACKED + '#define NEVER() if (0) { }\n'
        self.rejects(rf'a\.c:{line(never, "#define NEVER")}: if \(0\) has no @hack redundant tag above it', never)

    def test_wants_the_hack_tag_directly_above(self):
        at = line(HACKED, CALL)
        tagged = CALL.split('\n')[0]
        for above in (tagged + '\n\n',
                      tagged + '\n    /* a never-run call */\n',
                      tagged + '\n    host();\n',
                      '    /* @bug memory: a call through a null pointer, at 0x0c900008 */\n',
                      '    /* @asm open: a call */\n'):
            below = at + above.count('\n')
            self.rejects(rf'a\.c:{below}: if \(0\) has no @hack redundant tag above it',
                         HACKED.replace(CALL, above + '    if (0) {\n'))

    def test_reads_each_spelling_of_an_if_0_and_a_while_0(self):
        at = line(HACKED, CALL)
        for spelling, shown in (('if (0U) {', 'if (0U)'), ('if (FALSE) {', 'if (FALSE)'),
                                ('if (0 && x) {', 'if (0 && ...)'), ('if(0x0) {', 'if (0x0)'),
                                ('if ( 0L ) {', 'if (0L)'), ('if (0 &&\n        x) {', 'if (0 && ...)'),
                                ('if (0 \\\n        && x) {', 'if (0 && ...)'), ('if (0 && x && x) {', 'if (0 && ...)'),
                                ('if (00) {', 'if (00)'),
                                ('if ((0)) {', 'if (0)'), ('if ((0) && x) {', 'if (0 && ...)'),
                                ('if ((0 && x)) {', 'if (0 && ...)'), ('if (0 && (x || x)) {', 'if (0 && ...)'),
                                ('while (0) {', 'while (0)'), ('while (FALSE)\n    {', 'while (FALSE)'),
                                ('while (0) \\\n    {', 'while (0)')):
            self.rejects(rf'a\.c:{at}: {re.escape(shown)} has no @hack redundant tag above it',
                         HACKED.replace(CALL, f'    {spelling}\n'))
            check(HACKED.replace('    if (0) {\n        host();', f'    {spelling}\n        host();'))
        # after an else, which its tag stands above
        text = HACKED.replace(CALL, '    if (x) {\n    } else if (0) {\n')
        self.rejects(rf'a\.c:{at + 1}: if \(0\) has no @hack redundant tag above it', text)
        check(HACKED.replace(CALL, '    if (x) {\n' + CALL.replace('if (0) {', '} else if (0) {')))

    def test_reads_an_if_0_only_in_code(self):
        others = ('/* if (0) { } */\n'
                  'static const char *never = "if (0) {";\n'
                  'static const char flag = \'0\';\n'
                  '#if (0)\n'
                  '#endif\n'
                  'void others(int x, int FALSE_START, int x0)\n'
                  '{\n'
                  '    do {\n'
                  '    } while (0);\n'
                  '    do\n'
                  '        x++;\n'
                  '    while (FALSE);\n'
                  '    if (0x10 || 01 || 0.5F || x0) {\n'
                  '    }\n'
                  '    if (0 || x || 0 == x || FALSE_START) {\n'
                  '    }\n'
                  '    if (0 && x || x0) {\n'
                  '    }\n'
                  '    if ((0) + 1 || (0) || x || (0 && x) || x0) {\n'
                  '    }\n'
                  '    while (0 ? x : x0) {\n'
                  '    }\n'
                  '    if (0 && x ? x : x0) {\n'
                  '    }\n'
                  '    if (0 && x, x0) {\n'
                  '    }\n'
                  '    elif(0);\n'
                  '}\n')
        check(HACKED + others)
        # but sees the one untagged if (0) among them
        with self.assertRaises(SystemExit) as raised:
            check(HACKED.replace(CALL, '    if (0) {\n') + others)
        self.assertEqual(str(raised.exception.code).count('\n'), 0)
        self.assertRegex(str(raised.exception.code),
                         rf'a\.c:{line(HACKED, CALL)}: if \(0\) has no @hack redundant tag')

    def test_wants_a_hack_kind_and_a_text(self):
        kinds = 'implicit, type, redundant, spelling'
        at = line(HACKED, CALL)
        for kind in kinds.split(', '):
            check(HACKED.replace('@hack spelling: the empty else', f'@hack {kind}: the empty else'))
        form = rf'(?m)a\.c:{at}: not @hack <kind>: <text>, with a kind of {kinds}$'
        for wrong in ('@hack workaround: a never-run call', '@hack never run: a never-run call',
                      '@hack size: a never-run call', '@hack: redundant: a never-run call',
                      '@hack:redundant: a never-run call', '@hack redundant:a never-run call',
                      '@hack never-run: a never-run call'):
            self.rejects(form, HACKED.replace('@hack redundant: a never-run call', wrong))
        self.rejects(form, HACKED.replace('a never-run call; without it, 2 of 8 words differ', ''))

    def test_wants_a_redundant_tag_above_each_if_0(self):
        at = line(HACKED, CALL) + 1
        for kind in ('implicit', 'type', 'spelling'):
            self.rejects(rf'(?m)a\.c:{at}: if \(0\) has no @hack redundant tag above it$',
                         HACKED.replace('@hack redundant: a never-run call', f'@hack {kind}: a never-run call'))
        # or among the @hack tags stacked directly above it, as above the if (0) with an else, which a blank line,
        # prose or another tag ends
        stacked = line(HACKED, '    if (0) {\n        if (x) {')
        jump = ' * differ */\n    /* @hack spelling:'
        for between in ('\n\n', '\n    /* an else */\n', '\n    /* @bug memory: a read at 0x0c900008 */\n'):
            below = stacked + between.count('\n') - 1
            self.rejects(rf'a\.c:{below}: if \(0\) has no @hack redundant tag above it',
                         HACKED.replace(jump, jump.replace('\n', between)))
        self.rejects(rf'a\.c:{stacked}: if \(0\) has no @hack redundant tag above it',
                     HACKED.replace('@hack redundant: the tests', '@hack spelling: the tests'))
        # a tag of two lines among them
        check(HACKED.replace('without it, 1 of 8 words differs */', 'without it, 1 of 8\n     * words differs */'))

    def test_wants_a_bug_kind_and_a_text(self):
        kinds = 'memory, uninitialized, logic, arithmetic, mismatched-call'
        at = line(HACKED, '    /* @bug')
        for kind in kinds.split(', '):
            check(HACKED.replace('@bug memory:', f'@bug {kind}:'))
        form = rf'(?m)a\.c:{at}: not @bug <kind>: <text>, with a kind of {kinds}$'
        for wrong in ('@bug typo:', '@bug redundant:', '@bug:memory:', '@bug MEMORY:', '@bug null:'):
            self.rejects(form, HACKED.replace('@bug memory:', wrong))

    def test_wants_an_assembly_tag_s_kind_and_text(self):
        self.rejects(r'(?m)c\.src:8: not @asm <kind>: <text>, with a kind of opcode, operand, abi, epilogue, name, '
                     r'open$', src=SRC.replace('; @asm opcode:', '; @asm producer:'))
        self.rejects(r'c\.src:8: not @asm <kind>', src=SRC.replace('opcode: cache-line', 'opcode:cache-line'))

    def test_reads_each_tag_in_a_src_file(self):
        # an indented @asm tag settles its label
        indented = SRC.replace('; @asm opcode:', '        ; @asm opcode:')
        check(src=indented)
        self.assertEqual(review(src=indented), [NEXT])
        self.rejects(r'(?m)c\.src:8: not @asm <kind>: <text>, with a kind of opcode, operand, abi, epilogue, name, '
                     r'open$', src=indented.replace('@asm opcode:', '@asm producer:'))
        # an @hack or @bug tag's form and kind, at the margin or among the code
        bugs = 'memory, uninitialized, logic, arithmetic, mismatched-call'
        code = '        OCBI        @R4\n        OCBI'
        for indent in ('', '        '):
            between = code.replace('\n', f'\n{indent}; @bug memory: a store through R4, at 0x0c900042\n', 1)
            check(src=SRC.replace(code, between))
            self.rejects(rf'(?m)c\.src:11: not @bug <kind>: <text>, with a kind of {bugs}$',
                         src=SRC.replace(code, between.replace('memory:', 'nonsense:')))
            self.rejects(r'(?m)c\.src:11: not @hack <kind>: <text>, with a kind of implicit, ',
                         src=SRC.replace(code, between.replace('@bug memory:', '@hack nonsense:')))
        # which is no label's @asm tag
        self.rejects(rf'(?m)c\.src:{line(SRC, "_c_entry:")}: _c_entry has no @asm tag$',
                     src=SRC.replace('; @asm opcode: cache-line, OCBI @R4 at 0x0c900044 invalidates a line',
                                     '; @bug memory: a read, at 0x0c900044'))

    def test_wants_an_assembly_tag_at_the_end_of_a_label_s_comment(self):
        where = r'c\.src:8: an @asm tag not at the end of the comment above a label'
        self.rejects(where, src=SRC.replace('invalidates a line\n', 'invalidates a line\n\n'))
        self.rejects(where, src=SRC.replace('invalidates a line\n_c_entry:\n', 'invalidates a line\n'))
        # a second tag ends the comment, not the first
        self.rejects(where, src=SRC.replace('invalidates a line\n',
                                            'invalidates a line\n; @asm opcode: cache-line, at 0x0c900046\n'))

    def test_wants_no_assembly_tag_above_a_label_that_starts_no_function(self):
        # c_tail, which the tables don't name and no .EXPORT exports, starts code but no function
        src = SRC.replace('\nc_tail:', '\n; @asm open: x\nc_tail:')
        self.rejects(rf'(?m)c\.src:{line(src, "; @asm open: x")}: an @asm tag above c_tail, which starts no function$',
                     src=src)

    def test_wants_each_assembly_address_in_its_label_s_function(self):
        self.rejects(r'c\.src:8: 0x0c900050 is outside its function', src=SRC.replace('0x0c900044', '0x0c900050'))
        # one in another function of its source that the tag names
        check(src=SRC.replace('at 0x0c900044 invalidates', 'at 0x0c900044, before c_next at 0x0c900050, '
                                                           'invalidates'))
        # a label the tables don't name lies in the function of the last label above it they name
        check(src=TAIL)
        self.rejects(r'c\.src:13: 0x0c900054 is outside its function', src=TAIL.replace('0x0c900048', '0x0c900054'))
        # but a label in a run with a label they name lies in that label's function: c_next_entry in c_next's, not
        # c_entry's
        check(src=SRC.replace('_c_next:\n', 'c_next_entry:\n_c_next:\n')
              .replace('; @asm open: every instruction', '; @asm open: at 0x0c900050, every instruction'))
        # a function runs past a second name at its address
        self.assertEqual(review(symbols=TABLES['config/mod/symbols.txt'] + '0x0c900040  c_start\n'), [NEXT])
        # and a hex number of fewer than 8 digits is no address
        check(src=SRC.replace('invalidates a line', 'invalidates the line at VBR + 0x100'))

    def test_wants_an_assembly_tag_above_a_label_that_starts_code(self):
        # _c_data labels a .DATA.L, which starts no code
        self.rejects(rf'(?m)c\.src:{line(SRC, "; Uint32 c_data") + 1}: an @asm tag above _c_data, a label that starts '
                     'no code$', src=SRC.replace('; Uint32 c_data\n', '; Uint32 c_data\n; @asm open: a datum\n'))

    def test_wants_an_address_in_an_assembly_tag(self):
        self.rejects(r'c\.src:8: @asm opcode cites no address', src=SRC.replace(' at 0x0c900044', ''))

    def test_wants_a_tag_above_each_function(self):
        untagged = SRC.replace('; @asm opcode: cache-line, OCBI @R4 at 0x0c900044 invalidates a line\n', '')
        self.rejects(rf'(?m)c\.src:{line(untagged, "_c_entry:")}: _c_entry has no @asm tag$', src=untagged)
        # a label before data starts no function, even with a local label between them
        check(src=SRC.replace('_c_data:\n', '_c_data:\n?0001:\n'))

    def test_wants_a_tag_in_a_plain_comment(self):
        self.rejects(r'(?m)a\.c:13: a tag is a plain /\* \*/ comment, not /\*\* \*/$',
                     C.replace('/* @asm opcode:', '/** @asm opcode:'))
        self.rejects(rf'(?m)a\.c:{line(HACKED, CALL)}: a tag is a plain /\* \*/ comment, not /\*\* \*/$',
                     HACKED.replace('/* @hack redundant: a never-run', '/** @hack redundant: a never-run'))

    def test_lists_each_open_tag(self):
        block = (f'modules/mod/a.c:{line(BLOCKS, "#pragma asm")}: #pragma asm: 8 bytes reserved after host, from '
                 '0x0c900018')
        self.assertEqual(review(BLOCKS), [block, NEXT])
        opened = BLOCKS.replace('/* @asm epilogue: dead-return, SHC', '/* @asm open: SHC')
        self.assertEqual(review(opened, SRC.replace('; @asm opcode: cache-line, OCBI', '; @asm open: OCBI')),
                         ["modules/mod/a.c:6: #pragma inline_asm(outline): SHC's unreachable RTS at 0x0c900014 "
                          "follows the body's own return at 0x0c900010", block,
                          f'modules/mod/c.src:{line(SRC, "_c_entry:")}: _c_entry: OCBI @R4 at 0x0c900044 '
                          'invalidates a line', NEXT])
        # and fails on what has no tag
        with self.assertRaisesRegex(SystemExit, r'a\.c:6: #pragma inline_asm\(outline\) has no @asm tag'):
            review(BLOCKS.replace('/* @asm epilogue: dead-return, SHC', '/* SHC'))

    def test_lists_each_function_once(self):
        # c_next with a second name that runs into it, and c_tail, once the tables name it
        src = SRC.replace('; void c_next(void)\n', '; Uint32 c_after\n_c_after:\n; void c_next(void)\n')
        src = src.replace('        .EXPORT     _c_data\n', '        .EXPORT     _c_data\n        .EXPORT     _c_after\n')

        def listed(text):
            return [f'modules/mod/c.src:{line(text, "_c_after:")}: _c_after, _c_next: every instruction is one SHC '
                    'writes for C']
        self.assertEqual(review(src=src), listed(src))
        symbols = TABLES['config/mod/symbols.txt'] + '0x0c900048  c_tail\n'
        with self.assertRaisesRegex(SystemExit, rf'(?m)c\.src:{line(src, "c_tail:")}: c_tail has no @asm tag$'):
            review(src=src, symbols=symbols)
        # a tag above either name is their code's, and may cite it
        tag = '; @asm open: every instruction is one SHC writes for C\n'
        moved = src.replace(tag, '').replace('_c_after:\n', tag + '_c_after:\n')
        self.assertEqual(review(src=moved), listed(moved))
        for text in (src, moved):
            check(src=text.replace('one SHC writes for C', 'one SHC writes for C, to its RTS at 0x0c900050'))
        # review lists a run at its first named label's line
        before = SRC.replace('_c_next:\n', 'c_before:\n_c_next:\n')
        self.assertEqual(review(src=before),
                         [f'modules/mod/c.src:{line(before, "_c_next:")}: _c_next: every instruction is one SHC writes '
                          'for C'])
        # and only one tag
        both = src.replace('_c_after:\n', tag + '_c_after:\n')
        self.rejects(rf'(?m)c\.src:{line(both, "_c_next:") - 1}: a second @asm tag for _c_after, _c_next$', src=both)

    def test_counts_each_kind(self):
        # a.c's two bodies and c.src's c_entry and c_next have tags
        self.assertEqual(reviewed()[-1], '4 of 4 inline bodies, #pragma asm blocks and .src functions have an @asm tag '
                                         '(2 opcode, 0 operand, 0 abi, 1 epilogue, 0 name, 1 open)')
        # BLOCKS adds an open block, and a block of comments, which is no assembly
        self.assertEqual(reviewed(BLOCKS)[-1],
                         '5 of 5 inline bodies, #pragma asm blocks and .src functions have an @asm tag (2 opcode, '
                         '0 operand, 0 abi, 1 epilogue, 0 name, 2 open)')

    def test_lists_and_still_checks_each_tag(self):
        with self.assertRaisesRegex(SystemExit, r'c\.src:8: 0x0c900050 is outside its function'):
            review(src=SRC.replace('0x0c900044', '0x0c900050'))


# c.src with an abi tag above c_next_entry, a label of c_next's run that the tables don't name and no .EXPORT exports
ENTRY = SRC.replace('; @asm open: every instruction is one SHC writes for C\n_c_next:\n',
                    '; @asm abi: stack-written, its RTS at 0x0c900050 returns\nc_next_entry:\n_c_next:\n')


def abi(*contracts, c=C, src=ENTRY):
    """Run tags --abi over modules/mod/a.c and modules/mod/c.src, which hold c and src, with each of contracts as a
    --contract."""
    with tree(dict(TABLES, **{'modules/mod/a.c': c, 'modules/mod/c.src': src})):
        tags.main(['--abi', *(f'--contract={name}' for name in contracts), 'modules/mod/a.c', 'modules/mod/c.src'])


# a.c with outline's body tagged abi
ABI = C.replace("/* @asm epilogue: dead-return, SHC's unreachable RTS at 0x0c900014 follows\n * the body's own return "
                "at 0x0c900010 */", '/* @asm abi: stack-written, its RTS at 0x0c900010 returns */')


class Contracts(unittest.TestCase):
    def test_names_each_abi_function_no_contract_names(self):
        # c_next_entry's tag is c_next's, the function its run of labels starts, for --abi as for the address it cites
        with self.assertRaisesRegex(SystemExit, rf'(?m)modules/mod/c\.src:{line(ENTRY, "c_next_entry:")}: c_next: '
                                                r'its @asm tag is abi, and no --contract names it$'):
            abi()
        abi('c_next')
        # an exported label's is its own
        with self.assertRaisesRegex(SystemExit, rf'(?m)modules/mod/c\.src:{line(TAIL, "c_tail:")}: c_tail: '
                                                r'its @asm tag is abi, and no --contract names it$'):
            abi(src=TAIL)
        abi('c_tail', src=TAIL)
        # and a body's is its function's
        with self.assertRaisesRegex(SystemExit, rf'(?m)modules/mod/a\.c:{line(ABI, "#pragma inline_asm(outline)")}: '
                                                r'outline: its @asm tag is abi, and no --contract names it$'):
            abi('c_next', c=ABI)
        abi('c_next', 'outline', c=ABI)

    def test_names_each_contract_that_names_no_abi_function(self):
        # outline's tag is epilogue and its body is call-safe. gone names nothing in either file
        for name in ('outline', 'gone'):
            with self.subTest(name), self.assertRaisesRegex(
                    SystemExit, rf'(?m): {name}: --contract names no function whose @asm tag is abi or that may not '
                                r'be call-safe$'):
                abi('c_next', name)

    def test_still_checks_each_tag(self):
        with self.assertRaisesRegex(SystemExit, r'c\.src:8: 0x0c900050 is outside its function'):
            abi('c_next', src=ENTRY.replace('0x0c900044', '0x0c900050'))
        # an abi tag above a datum's label is no function's, and fails as any tag there does
        tagged = ENTRY.replace('; Uint32 c_data\n', '; Uint32 c_data\n; @asm abi: stack-written, at 0x0c900058\n')
        with self.assertRaisesRegex(SystemExit, r'c\.src:\d+: an @asm tag above _c_data, a label that starts no code'):
            abi('c_next', src=tagged)

    def test_wants_no_abi_tag_above_a_block_gcc_compiles(self):
        tagged = BLOCKS.replace('/* @asm open: 8 bytes', '/* @asm abi: stack-written, 8 bytes')
        check(tagged)
        with self.assertRaisesRegex(SystemExit, rf'(?m)a\.c:{line(tagged, "#pragma asm")}: a #pragma asm block whose '
                                                r'@asm tag is abi, which GCC compiles and no --contract can name$'):
            abi('c_next', c=tagged)
        # but takes one on the shipped side of a GCC guard
        abi('c_next', c=tagged.replace('/* @asm abi', '#ifndef GCC\n/* @asm abi').replace(
            '        ;\n#pragma endasm\n', '        ;\n#pragma endasm\n#endif\n'))

    def test_reads_only_the_images_that_place_its_sources(self):
        # other's splits.txt holds a line that isn't a row, which reading the image would fail on
        with tree(dict(TABLES, **{'config/other/splits.txt': 'not a row\n', 'modules/mod/a.c': C,
                                  'modules/mod/c.src': ENTRY})):
            tags.main(['--abi', '--contract=c_next', 'modules/mod/a.c', 'modules/mod/c.src'])


# ABI with its bodies in the #else of an #ifdef GCC whose other side defines outline in C
GUARDED = ('#ifdef GCC\nvoid outline(void)\n{\n}\n#else\n' +
           ABI.replace('#endif\n/* clang-format on */\n', '#endif\n/* clang-format on */\n#endif\n'))


def walk(*body, head='static void walk(Uint32 *line)', host='    walk(0);\n'):
    """a.c with one inline body, walk, and a C function, host. walk's head is head and its lines are body, where a line
    that ends in : is a label. host's statements are host."""
    lines = ''.join(f'{line}\n' if line.endswith(':') else f'        {line}\n' for line in body)
    return ('/* clang-format off */\n#ifndef __clang__\n/* @asm open: a walk */\n#pragma inline_asm(walk)\n'
            f'{head}\n{{\n{lines}}}\n#endif\n/* clang-format on */\n\nint host(void)\n{{\n{host}}}\n')


class CallSafety(unittest.TestCase):
    def unsafe(self, c, why):
        """Check that --abi fails on c's body, walk, for why, and passes with a contract for it."""
        with self.assertRaisesRegex(SystemExit, rf'(?m)a\.c:4: walk: {why}, and no --contract names it$'):
            abi('c_next', c=c)
        abi('c_next', 'walk', c=c)

    def test_names_a_body_that_reads_a_register_no_argument_passes(self):
        c = walk('MOV.L @R4,R1', 'OR R12,R1', 'OCBI @R1')
        self.unsafe(c, rf'it reads R12 at line {line(c, "OR R12")}, which no argument passes')
        # the one float argument takes FR4 alone. No argument passes FR5, T, or a register only one path sets
        c = walk('FMOV.S FR4,@R4', 'FMOV.S FR5,@R4', head='static void walk(Uint32 *line, Float scale)')
        self.unsafe(c, rf'it reads FR5 at line {line(c, "FMOV.S FR5")}, which no argument passes')
        c = walk('BT ?done', 'NOP', '?done:')
        self.unsafe(c, rf'it reads T at line {line(c, "BT")}, which no argument passes')
        c = walk('TST R4,R4', 'BT ?read', 'MOV #1,R1', '?read:', 'MOV.L R1,@R4')
        self.unsafe(c, rf'it reads R1 at line {line(c, "MOV.L R1")}, which no argument passes')
        # but a push only saves a register, and SR and the other bank are the machine's. FR0 names the other bank
        # after an FRCHG, and with SZ set, an FMOV of FR1 moves XD0, a pair of it
        abi('c_next', c=walk('MOV.L R12,@-R15', 'MOV.L @R15+,R12', 'STC SR,R0', 'FRCHG', 'FMOV.S FR0,@R4', 'FRCHG',
                             'FSCHG', 'FMOV FR1,@R4', 'FSCHG'))
        # R16 names no register: asmsh reads it as a symbol, here a load's label
        abi('c_next', c=walk('MOV.L R16,R1', 'MOV.L R1,@R4'))
        # with SZ set, an FMOV of FR2 moves DR2, which no argument passes
        c = walk('FSCHG', 'FMOV FR2,@R4', 'FSCHG')
        self.unsafe(c, rf'it reads FR2 at line {line(c, "FMOV FR2")}, which no argument passes')
        # and one in a case a BRAF jumps to, where the host ends on the call
        c = walk('MOVA ?T,R0', 'BRAF R0', 'NOP', '?T:', 'MOV R12,R1', 'MOV.L R1,@R4', host='    return walk(0);\n')
        self.unsafe(c, rf'it reads R12 at line {line(c, "MOV R12")}, which no argument passes')

    def test_names_a_body_that_reads_a_register_a_call_can_change(self):
        c = walk('MOV R4,R6', 'MOV.L #_f,R1', 'STS.L PR,@-R15', 'JSR @R1', 'NOP', 'LDS.L @R15+,PR', 'MOV.L R0,@R6')
        self.unsafe(c, rf'it reads R6 at line {line(c, "MOV.L R0")}, which the call at line {line(c, "JSR")} can '
                       'change')
        # the RTS that tools/inline.py writes after the body reads PR
        lost = walk('MOV.L #_f,R1', 'JSR @R1', 'NOP')
        self.unsafe(lost, rf'its return reads PR, which the call at line {line(lost, "JSR")} can change')
        # but a register the callee keeps still holds the body's value after the call
        abi('c_next', c=c.replace('MOV R4,R6', 'MOV.L R8,@-R15\n        MOV R4,R8').replace(
            'MOV.L R0,@R6', 'MOV.L R0,@R8\n        MOV.L @R15+,R8'))

    def test_names_a_body_that_returns_with_a_kept_register_changed(self):
        self.unsafe(walk('MOV.L @R4,R8'), 'it returns with R8 changed, which its GCC callers keep')
        # or popped from a slot that doesn't hold its whole value at entry. These slots hold R9, a word of R8, R8 once
        # changed, and R8 where one path stores over it through R15
        for body in (('MOV.L R9,@-R15', 'MOV #0,R8', 'MOV.L @R15+,R8'),
                     ('MOV.W R8,@-R15', 'MOV #0,R8', 'MOV.W @R15+,R8'),
                     ('MOV #0,R8', 'MOV.L R8,@-R15', 'MOV.L @R15+,R8'),
                     ('MOV.L R8,@-R15', 'TST R4,R4', 'BT ?pop', 'MOV.L R4,@R15', '?pop:', 'MOV.L @R15+,R8')):
            with self.subTest(body=body):
                self.unsafe(walk(*body), 'it returns with R8 changed, which its GCC callers keep')
        # R15 too, moved by a push, an ADD, or a copy or a pop into it. With SZ set, an FMOV.S pushes a pair. Each
        # spells R15 as R15 or SP, in any case
        for body in (('MOV.L R4,@-R15',), ('ADD #-4,R15',), ('MOV R4,R15',), ('MOV.L R4,@-R15', 'MOV.L @R15+,R15'),
                     ('FSCHG', 'FMOV.S FR0,@-R15', 'FSCHG', 'MOV.L @R15+,R0'), ('ADD #-4,SP',), ('MOV.L R4,@-sp',)):
            with self.subTest(body=body):
                self.unsafe(walk(*body), 'it returns with R15 changed, which its GCC callers keep')
        # a pop past the body's entry
        c = walk('MOV.L @R15+,R1', 'NOP')
        self.unsafe(c, rf"it takes R15 above its entry at line {line(c, 'MOV.L')}, into its caller's frame")
        # and FPSCR's FR and SZ bits, at a return or a call
        for bit, swap in (('FR', 'FRCHG'), ('SZ', 'FSCHG')):
            with self.subTest(bit=bit):
                self.unsafe(walk(swap), rf"it returns with FPSCR's {bit} bit set, which GCC's code takes clear")
                c = walk('MOV.L #_f,R1', swap, 'STS.L PR,@-R15', 'JSR @R1', 'NOP', 'LDS.L @R15+,PR', swap)
                self.unsafe(c, rf"its call at line {line(c, 'JSR')} runs with FPSCR's {bit} bit set, which GCC's code "
                               'takes clear')
        # but not where the body puts each back
        abi('c_next', c=walk('ADD #-8,R15', 'MOV.L R4,@R15', 'ADD #H\'8,R15', 'FRCHG', 'FRCHG'))
        abi('c_next', c=walk('MOV.L R8,@-R15', 'ADD #-4,R15', 'MOV #0,R8', 'ADD #4,R15', 'MOV.L @R15+,R8'))
        abi('c_next', c=walk('FSCHG', 'FMOV.S FR0,@-R15', 'FSCHG', 'ADD #8,R15'))
        abi('c_next', c=walk('ADD #-8,SP', 'MOV.L R4,@(4,SP)', 'ADD #8,R15'))

    def test_names_a_body_that_leaves_where_its_host_goes_on(self):
        on = '    walk(0);\n    return 1;\n'
        c = walk('TST R4,R4', 'BF ?push', 'RTS', 'MOV #0,R0', '?push:', 'MOV #1,R0', host=on)
        self.unsafe(c, rf'its RTS at line {line(c, "RTS")} leaves it, and host goes on past its call')
        out = walk('BRA _elsewhere', 'NOP', host=on)
        self.unsafe(out, rf'its BRA _elsewhere at line {line(out, "BRA")} leaves it, and host goes on past its call')
        # a jump to the body's own P2 alias, a BRAF and an RTE
        for body, statement in (
                (('MOVA ?UNCACHED,R0', 'MOV.L ?P2_BASE,R1', 'OR R0,R1', 'JMP @R1', 'NOP', '.ALIGN 4', '?P2_BASE:',
                  ".DATA.L H'A0000000", '?UNCACHED:'), 'JMP @R1'),
                (('BRAF R4', 'NOP'), 'BRAF R4'), (('RTE', 'NOP'), 'RTE')):
            with self.subTest(statement=statement):
                jump = walk(*body, host=on)
                self.unsafe(jump, rf'its {statement} at line {line(jump, statement)} leaves it, and host goes on past '
                                  'its call')
        # through a macro too
        self.unsafe(c.replace(on, '    WALK();\n') + '#define WALK() walk(0)\n',
                    rf'its RTS at line {line(c, "RTS")} leaves it, and host goes on past its call')
        # and where GCC's side of a guard goes on, though the shipped side returns the call's value
        guarded = c.replace(f'int host(void)\n{{\n{on}}}\n', f'#ifdef GCC\nint host(void)\n{{\n{on}}}\n#else\n'
                            'int host(void)\n{\n    return walk(0);\n}\n#endif\n')
        self.unsafe(guarded, rf'its RTS at line {line(c, "RTS")} leaves it, and host goes on past its call')
        # but not where its host returns the call's value, or ends on the call
        for host in ('    return walk(0);\n', '    if (1) {\n        return walk(0);\n    }\n    walk(0);\n'):
            with self.subTest(host):
                abi('c_next', c=c.replace(on, host))

    def test_fails_on_a_body_it_can_t_read(self):
        # a call to a label of the body, and a write to FPSCR, among them
        for statement, pattern in (('MAC.L @R4+,@R5+', r'MAC\.L @R4\+,@R5\+'), ('BSRF R4', 'BSRF R4'),
                                   ('BSR ?self', r'BSR \?self'), ('LDS R4,FPSCR', 'LDS R4,FPSCR')):
            with self.subTest(statement=statement):
                c = walk('?self:', statement, 'NOP')
                with self.assertRaisesRegex(SystemExit, rf"(?m)a\.c:{line(c, statement)}: walk: {pattern}, which "
                                                        r"--abi doesn't read$"):
                    abi('c_next', 'walk', c=c)
        # a delayed branch whose slot it can't place: a return or a call that ends the body, a slot under a label or a
        # directive, and a slot that branches or returns
        for body, statement in ((('NOP', 'RTS'), 'RTS'), (('MOV.L #_f,R1', 'JSR @R1'), 'JSR @R1'),
                                (('BRA ?next', '?next:', 'NOP'), 'BRA ?next'),
                                (('BRA ?next', '.ALIGN 4', 'NOP', '?next:', 'NOP'), 'BRA ?next'),
                                (('BRA ?next', 'BT ?next', '?next:', 'NOP'), 'BRA ?next'),
                                (('BRA ?next', 'RTS', 'NOP', '?next:', 'NOP'), 'BRA ?next')):
            with self.subTest(body=body):
                c = walk(*body)
                with self.assertRaisesRegex(SystemExit, rf"(?m)a\.c:{line(c, statement)}: walk: "
                                                        rf"{re.escape(statement)}, whose slot --abi can't place$"):
                    abi('c_next', 'walk', c=c)
        # one no path reaches
        c = walk('RTS', 'NOP', 'MOV #1,R1')
        with self.assertRaisesRegex(SystemExit, rf"(?m)a\.c:{line(c, 'MOV #1')}: walk: MOV #1,R1, which no path of "
                                                r"--abi's reaches$"):
            abi('c_next', 'walk', c=c)
        # and where two paths meet with FR, SZ or R15 apart
        for apart in ('FSCHG', 'ADD #-4,R15'):
            with self.subTest(apart=apart):
                c = walk('TST R4,R4', 'BT ?done', apart, '?done:', 'NOP')
                with self.assertRaisesRegex(SystemExit, rf"(?m)a\.c:{line(c, '?done:') + 1}: walk: two paths meet "
                                                        r"here with FPSCR's FR or SZ bits or R15 apart, which --abi "
                                                        r"can't read$"):
                    abi('c_next', 'walk', c=c)
        # one run names each instruction --abi doesn't read, beside the tags' problems. A --contract may name the body
        c = walk('MAC.L @R4+,@R5+', 'BSRF R4', 'NOP') + '/* @bug typo: a shipped bug */\n'
        with self.assertRaises(SystemExit) as raised:
            abi('c_next', 'walk', c=c)
        for pattern in (rf"(?m)a\.c:{line(c, 'MAC.L')}: walk: MAC\.L @R4\+,@R5\+, which --abi doesn't read$",
                        rf"(?m)a\.c:{line(c, 'BSRF')}: walk: BSRF R4, which --abi doesn't read$",
                        rf'(?m)a\.c:{line(c, "/* @bug")}: not @bug <kind>: <text>'):
            self.assertRegex(str(raised.exception.code), pattern)
        self.assertNotIn('--contract names no function', str(raised.exception.code))

    def test_fails_on_a_parameter_it_can_t_place(self):
        # a float typedef it doesn't know, a long long and a variadic tail, whatever --contract names
        for head, parameter in (('Uint32 *line, KMFLOAT scale', 'KMFLOAT scale'),
                                ('Uint32 *line, long long n', 'long long n'), ('Uint32 *line, ...', '...')):
            with self.subTest(head=head), self.assertRaisesRegex(
                    SystemExit, rf"(?m)a\.c:5: walk: its parameter {re.escape(parameter)}, which --abi can't place$"):
                abi('c_next', 'walk', c=walk('OCBI @R4', head=f'static void walk({head})'))
        # but places floats and integers as C and the SDK spell them, in any order, and a pointer past R7 on the stack
        abi('c_next', c=walk('FMOV.S FR4,@R4', 'FMOV.S FR5,@R4', 'MOV.L R5,@R4',
                             head='static void walk(Uint32 *line, const Float a, double b, Sint32 n)'))
        abi('c_next', c=walk('MOV.L @R15,R1', 'MOV.L R7,@R1',
                             head='static void walk(Uint32 *line, Int a, Int b, Int c, Uint32 *out)'))

    def test_reads_bodies_only_under_abi(self):
        # a plain run and --review check tags alone
        c = walk('MAC.L @R4+,@R5+', 'NOP')
        check(c)
        self.assertEqual(review(c), ['modules/mod/a.c:4: #pragma inline_asm(walk): a walk', NEXT])

    def test_reads_no_body_on_the_shipped_side_of_a_gcc_guard(self):
        c = walk('MOV.L @R4,R1', 'OR R12,R1', 'OCBI @R1')
        abi('c_next', c=c.replace('#ifndef __clang__\n', '#ifndef __clang__\n#ifndef GCC\n').replace(
            '#endif\n', '#endif\n#endif\n'))


class Guards(unittest.TestCase):
    def test_takes_no_contract_for_a_body_on_the_shipped_side_of_a_gcc_guard(self):
        abi('c_next', c=GUARDED)
        with self.assertRaisesRegex(SystemExit, r'(?m): outline: --contract names no function whose @asm tag is abi '
                                                r'or that may not be call-safe$'):
            abi('c_next', 'outline', c=GUARDED)
        # SHC still compiles the body, so it still takes its tag
        untagged = GUARDED.replace('/* @asm abi: stack-written, its RTS at 0x0c900010 returns */\n', '')
        with self.assertRaisesRegex(SystemExit, rf'(?m)a\.c:{line(untagged, "#pragma inline_asm(outline)")}: '
                                                r'#pragma inline_asm\(outline\) has no @asm tag$'):
            abi('c_next', c=untagged)

    def test_names_a_source_whose_assembly_is_on_the_shipped_side_of_a_gcc_guard(self):
        # SHC compiles that side, so its route still takes the source through assembly
        with tree({'modules/mod/a.c': GUARDED}), contextlib.redirect_stdout(io.StringIO()) as out:
            tags.main(['--inline', 'modules/mod/a.c'])
        self.assertEqual(out.getvalue(), 'modules/mod/a.c\n')


if __name__ == '__main__':
    unittest.main()
