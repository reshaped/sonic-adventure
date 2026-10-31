import os
import unittest
from pathlib import Path

import csource
from support import tree

# C with a /* */ comment over two lines, a // comment a splice continues, a string that holds /* */, and a guard
TEXT = '''int a; /* one
two */ int b; // three \\
four
char *c = "/* five */";
#ifdef GCC
int gcc;
#else
int shc;
#endif
'''


class Code(unittest.TestCase):
    def test_blanks_each_comment_but_for_its_line_ends_and_splices(self):
        with tree({'a.c': TEXT}):
            self.assertEqual(csource.code('a.c').splitlines()[:4],
                             ['int a;       ', '       int b;          \\', '    ', 'char *c = "/* five */";'])

    def test_finds_a_comment_whose_opener_a_splice_splits(self):
        # C joins spliced lines before it finds comments, so a / and a * that a splice parts open one, but not in a
        # string
        with tree({'a.c': 'int a; /\\\n* gone */ int b; "/\\\n*";\n'}):
            self.assertEqual(csource.code('a.c'), 'int a;  \\\n          int b; "/\\\n*";\n')

    def test_empties_the_side_of_each_guard_the_compiler_skips(self):
        with tree({'a.c': TEXT}):
            self.assertEqual([csource.code('a.c', compiler).splitlines()[4:] for compiler in (None, 'SHC', 'GCC')],
                             [['#ifdef GCC', 'int gcc;', '#else', 'int shc;', '#endif'],
                              ['#ifdef GCC', '', '#else', 'int shc;', '#endif'],
                              ['#ifdef GCC', 'int gcc;', '#else', '', '#endif']])

    def test_takes_an_elifdef_or_an_elifndef_for_a_group_after_a_guard_s_first(self):
        text = '#ifdef GCC\nint gcc;\n#elifdef X\nint x;\n#endif\n#ifndef GCC\nint shc;\n#elifndef Y\nint y;\n#endif\n'
        with tree({'a.c': text}):
            self.assertEqual([[csource.code('a.c', compiler).splitlines()[at] for at in (1, 3, 6, 8)]
                              for compiler in ('SHC', 'GCC')],
                             [['', 'int x;', 'int shc;', ''], ['int gcc;', '', '', 'int y;']])


class Made(unittest.TestCase):
    def test_reads_a_text_code_made_as_it_reads_the_text_alone(self):
        # each made from TEXT's C, with its comments, a guard's side or its literals blank, and each line past an
        # emptied one moved up
        with tree({'a.c': TEXT}):
            made = [csource.code('a.c', compiler) for compiler in (None, 'SHC', 'GCC')]
        made.append(csource.blanked(made[2]))
        for text in made:
            found, alone = csource.parse(text), csource.C(text)
            self.assertIsNot(found, csource.read(text))
            self.assertEqual((found.lexemes, found.tokens, found.directives, found.blocks),
                             (alone.lexemes, alone.tokens, alone.directives, alone.blocks))
            self.assertEqual(csource.tokens(text), [(token.text, token.line) for token in alone.tokens
                                                    if token.kind != 'comment'])


class Uncommented(unittest.TestCase):
    def test_makes_each_comment_one_blank_and_keeps_its_line_ends_where_asked(self):
        text = '#define A 1 /* one\ntwo */ + 2\nchar *b = "/* three */"; // four\n'
        self.assertEqual((csource.uncommented(text), csource.uncommented(text, lines=False)),
                         ('#define A 1  \n + 2\nchar *b = "/* three */";  \n',
                          '#define A 1   + 2\nchar *b = "/* three */";  \n'))


class Compiled(unittest.TestCase):
    def test_reads_the_groups_of_gcc_s_and_clang_s_tests_and_no_other(self):
        # make gcc's compile defines GCC and not __clang__. It takes no #elif, #elifdef or #elifndef below a group it
        # takes, and may take any other, an #if's group, another macro's, or an #else below one of those; a group it
        # skips settles each inside it
        text = ('#ifdef GCC\na\n#elif X\nb\n#else\nc\n#endif\n#ifndef __clang__\nd\n#else\ne\n#endif\n'
                '#ifndef GCC\nf\n#elif X\ng\n#else\nh\n#endif\n#if 1\ni\n#else\nj\n#endif\n'
                '#ifdef X\n#ifndef GCC\nk\n#else\nl\n#endif\n#endif\nm\n'
                '#ifdef __clang__\nn\n#elifdef X\no\n#endif\n#ifndef __clang__\np\n#elifndef X\nq\n#endif')
        lines = text.split('\n')
        found = dict(zip(lines, csource.compiled(lines)))
        self.assertEqual({line: found[line] for line in 'abcdefghijklmnopq'},
                         {'a': True, 'b': False, 'c': False, 'd': True, 'e': False, 'f': False, 'g': None, 'h': None,
                          'i': None, 'j': None, 'k': False, 'l': None, 'm': True, 'n': False, 'o': None, 'p': True,
                          'q': False})


class Tokens(unittest.TestCase):
    def test_gives_each_token_its_line(self):
        self.assertEqual(csource.tokens('int a =\n    0x10;\n'), [('int', 1), ('a', 1), ('=', 1), ('0x10', 2), (';', 2)])

    def test_ends_a_character_constant_at_its_line(self):
        # SHC's assembly spells hexadecimal H'7B, whose ' opens no constant its line closes
        self.assertEqual(csource.tokens("MOV #H'7B,R0\nRTS\n"),
                         [('MOV', 1), ('#', 1), ('H', 1), ("'", 1), ('7B', 1), (',', 1), ('R0', 1), ('RTS', 2)])

    def test_leaves_out_each_directive_where_asked(self):
        # B's # opens no directive, since a splice joins its line to the one above
        text = '#define A \\\n    1\nint a; \\\n# define B\n#if A\nint b;\n#endif\n'
        self.assertEqual(csource.tokens(text, directives=False),
                         [('int', 3), ('a', 3), (';', 3), ('#', 4), ('define', 4), ('B', 4), ('int', 6), ('b', 6),
                          (';', 6)])
        # a comment that holds a line end runs Y on to line 2, and int is on line 3
        self.assertEqual(csource.tokens('#define Y 1 /* one\ntwo */ 2\nint a;\n', directives=False),
                         [('int', 3), ('a', 3), (';', 3)])

    def test_reads_a_token_a_splice_splits_as_one_on_its_first_line(self):
        self.assertEqual(csource.tokens('int imp\\\nort = 0x0cc0\\\n0080;\nint b;\n'),
                         [('int', 1), ('import', 1), ('=', 2), ('0x0cc00080', 2), (';', 3), ('int', 4), ('b', 4),
                          (';', 4)])


class MacroDirectives(unittest.TestCase):
    def test_gives_each_definition_and_undef_in_order_with_its_line(self):
        # F's ellipsis takes the name __VA_ARGS__, as its body spells it. NOT's # is on a line a splice joins to
        # the one above, so it opens no directive
        text = ('#define A 0x10\n#  define F(x, ...) x, __VA_ARGS__\n#define G() 2\nint a; \\\n#define NOT 1\n'
                '#undef A\n#define B \\\n    (A + 1)\n')
        self.assertEqual(csource.macro_directives(text), [(1, 'A', (None, ['0x10'])),
                                                 (2, 'F', (('x', '__VA_ARGS__'), ['x', ',', '__VA_ARGS__'])),
                                                 (3, 'G', ((), ['2'])), (6, 'A', None),
                                                 (7, 'B', (None, ['(', 'A', '+', '1', ')']))])

    def test_runs_a_directive_on_past_a_comment_that_holds_a_line_end(self):
        # a line end inside a comment doesn't end a directive, as GCC reads it
        self.assertEqual(csource.macro_directives('#define Y 1 /* one\ntwo */ 2\nint a;\n'),
                         [(1, 'Y', (None, ['1', '2']))])

    def test_takes_a_named_ellipsis_by_its_name(self):
        # in GNU C, args ... names the variable arguments args, as the body spells them
        self.assertEqual(csource.macro_directives('#define LOG(fmt, args ...) f(fmt, args)\n'),
                         [(1, 'LOG', (('fmt', 'args'), ['f', '(', 'fmt', ',', 'args', ')']))])


class Spans(unittest.TestCase):
    def test_closes_a_function_at_the_brace_that_matches_its_opening(self):
        text = 'int f(void)\n{\n    if (a) {\n    }\n} /* NOLINT */\nint g(void)\n{\n}\n'
        self.assertEqual(list(csource.spans(csource.blanked(text).split('\n'))), [('f', 1, 4), ('g', 6, 7)])

    def test_reads_an_old_form_head_through_its_parameters_declarations(self):
        text = 'void f(a, b)\n    int a;\n    char *b;\n{\n}\nint g[] = {1};\n'
        self.assertEqual(list(csource.spans(text.split('\n'))), [('f', 3, 4)])

    def test_takes_the_braces_of_the_first_group_to_change_them(self):
        # with the GCC side empty, as code() gives it for SHC, only the #else group opens the if's brace
        text = 'int f(void)\n{\n#ifdef GCC\n\n#else\n    if (a) {\n#endif\n        b();\n    }\n}\n'
        self.assertEqual(list(csource.spans(text.split('\n'))), [('f', 1, 9)])

    def test_closes_an_inline_body_at_its_first_closing_brace(self):
        # an inline body is assembly, so its { opens nothing
        text = '#pragma inline_asm(f)\nstatic int f(void)\n{\n        RTS ; {\n        NOP\n}\nint g(void)\n{\n}\n'
        lines = csource.blanked(text).split('\n')
        self.assertEqual((list(csource.spans(lines)), csource.bodies('a.c', lines)),
                         ([('f', 2, 5), ('g', 7, 8)], [(0, 'f', True, 2, 5)]))


class Included(unittest.TestCase):
    def test_an_include_inside_a_comment_includes_nothing(self):
        files = {'a.c': '/*\n#include "b.h"\n*/\n#include "c.h"\n', 'b.h': '', 'c.h': ''}
        with tree(files):
            texts = {}
            csource.included(Path('a.c'), texts)
        self.assertEqual(list(texts), [Path('a.c'), Path('c.h')])

    def test_an_angle_include_searches_the_roots_alone_and_a_quoted_one_its_directory_first(self):
        files = {'src/a.c': '#include <x.h>\n#include "y.h"\n', 'src/x.h': '', 'src/y.h': '', 'sdk/include/x.h': '',
                 'sdk/include/y.h': ''}
        with tree(files):
            texts = {}
            csource.included(Path('src/a.c'), texts)
        self.assertEqual(list(texts), [Path('src/a.c'), Path('sdk/include/x.h'), Path('src/y.h')])

    def test_a_file_is_read_once_however_its_path_is_spelled(self):
        # a header that includes itself through .., and another through a
        # link to their directory
        files = {'include/a.h': '#ifndef A\n#define A\n#include "../include/a.h"\n#include "link/b.h"\n#endif\n',
                 'include/b.h': '#include "link/b.h"\n'}
        with tree(files):
            os.symlink('.', 'include/link')
            texts = {}
            csource.included(Path('include/a.h'), texts)
        self.assertEqual(list(texts), [Path('include/a.h'), Path('include/b.h')])


class Preprocessed(unittest.TestCase):
    def test_reads_a_name_with_a_dollar_sign_and_a_raw_string_to_its_delimiter(self):
        c = csource.parse('int a$b = R"x(")" )x";\nchar *c = uR"(\n)", *d = xR"(e)";\n')
        self.assertEqual([(token.text, token.kind) for token in c.parsed if token.kind != 'punctuator'],
                         [('int', 'name'), ('a$b', 'name'), ('R"x(")" )x"', 'string'), ('char', 'name'),
                          ('c', 'name'), ('uR"(\n)"', 'string'), ('d', 'name'), ('xR', 'name'),
                          ('"(e)"', 'string')])

    def test_gives_each_line_marker_and_leaves_its_tokens_out(self):
        c = csource.parse('# 1 "a.c"\nint a;\n# 7 "b.h" 1 3\n#pragma pack\nint b;\n')
        self.assertEqual(c.markers, [(1, 'a.c', 1), (3, 'b.h', 7)])
        self.assertEqual([token.text for token in c.parsed], ['int', 'a', ';', 'int', 'b', ';'])


class Brackets(unittest.TestCase):
    def test_pairs_brackets_of_any_kind_and_ends_an_initializer_outside_them(self):
        tokens = csource.parse('f(a[1}, {2, 3}, b) , c;').parsed
        # 0 f ( a [ 1 } , { 2 , 3 } , b ) , c ;
        self.assertEqual([tokens.closing(1), tokens.closing(3), tokens.closing(7)], [14, 5, 11])
        self.assertEqual([tokens.ending(0), tokens.ending(7), tokens.ending(16)], [15, 12, 17])
        self.assertEqual(list(tokens.listed(7, 12)), [(8, 9), (10, 11)])

    def test_names_a_bracket_no_bracket_closes(self):
        tokens = csource.parse('int a[2 = {1;').parsed
        with self.assertRaisesRegex(ValueError, r'^line 1: \[ with no bracket closing it$'):
            tokens.closing(2)


class Reader(csource.Expressions):
    def typedef_at(self, name, at):
        return name == 'T'

    def typename(self, text, at):
        return text == 'T'

    def term(self, words):
        return ' '.join(words)


def shape(node):
    """node as nested tuples of its kind, its what and its operands', and a
    leaf as its what alone."""
    return (node.kind, node.what, *(shape(operand) for operand in node.operands)) if node.operands else node.what


class Expressions(unittest.TestCase):
    def read(self, text):
        return Reader(csource.parse(text).parsed).full()

    def test_reads_operators_by_precedence_and_casts_by_the_words_inside_and_after(self):
        self.assertEqual(shape(self.read('a = b + c * -d ? (T) e : (f) - g, h')),
                         ('comma', ',', ('assign', '=', 'a', ('conditional', '?', ('binary', '+', 'b', (
                             'binary', '*', 'c', ('unary', '-', 'd'))), ('cast', 'T', 'e'), ('binary', '-', (
                                 'parentheses', '(', 'f'), 'g'))), 'h'))
        self.assertEqual(self.read('(unsigned char *) p').term, 'unsigned char *')

    def test_takes_a_call_a_subscript_and_a_compound_literal_whole(self):
        self.assertEqual(shape(self.read('f(a, b + 1)[2] + (T) { 3, T }')),
                         ('binary', '+', ('opaque', 'a subscript', ('opaque', 'a call', 'f', 'a', (
                             'binary', '+', 'b', '1')), '2'), ('opaque', 'a compound literal', '3', 'T')))


class Walk(csource.Walk):
    def __init__(self, tokens):
        self.names, self.statics = [], []
        super().__init__(tokens)

    def typedef_at(self, name, at):
        return name == 'T'

    def specifiers(self, at):
        if self.tokens[at].text not in ('int', 'T'):
            raise csource.Unread(f'{self.tokens[at].text}: no type')
        return self.tokens[at].text, at + 1

    def declarator(self, at, base):
        function = self.tokens[at + 1].text == '('
        return self.tokens[at].text, base, function, self.tokens.closing(at + 1) + 1 if function else at + 1

    def declared(self, name, kind, type, span, start, end):
        self.names.append((name, kind, start, end))

    def enumerated(self, name, start, end):
        self.names.append((name, 'enumerator', start, end))

    def static(self, start, end, type, why):
        self.statics.append((start, end, type, why))


class Walks(unittest.TestCase):
    def test_binds_each_name_in_its_scope_and_gives_each_initializer_and_expression(self):
        tokens = csource.parse('int a = 1; int f(int b) { T c = b; if (c) { static x y = 2; } return a; }').parsed
        walk = Walk(tokens)
        self.assertEqual(walk.names, [('a', 'object', 2, 33), ('f', 'object', 11, 33), ('b', 'object', 11, 32),
                                      ('c', 'object', 14, 32)])
        self.assertEqual(walk.statics, [(3, 4, 'int', None), (26, 27, None, 'x: no type')])
        self.assertEqual([' '.join(token.text for token in tokens[start:end]) for start, end in walk.expressions],
                         ['b', 'c', 'a'])
        self.assertEqual(walk.unread, [(22, 28, 'x: no type')])
        self.assertEqual([walk.code(at) for at in (3, 13, 15, 30)], [False, False, True, True])


if __name__ == '__main__':
    unittest.main()
