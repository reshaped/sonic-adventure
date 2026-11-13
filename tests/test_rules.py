"""One copy of a draw's rules. engine/sa1/rules.h holds the numbers of Ninja's and model.src's that both forms of
a draw read, the Dreamcast's drawers and a host's recorder, and the rules that are one function. It includes
nothing and defines macros and static inline functions alone, no other source under engine/, src/ or sdk/ defines
one of its names, but for the definitions KNOWN lists, and a source of each form calls each of its functions. A
name is held, and no number: a macro of another name and another meaning may have a number of the header's."""

import os
import re
import subprocess
import unittest
from pathlib import Path

import csource
import parts
from support import cwd

ROOT = Path(__file__).resolve().parents[1]
RULES = 'engine/sa1/rules.h'
# where a second definition is looked for, and the suffixes of C's and C++'s sources and headers
ROOTS = ('engine', 'src', 'sdk')
SUFFIXES = ('.c', '.cpp', '.h')
# the two forms of a draw, each by the thing config/parts.txt gives its sources: the Dreamcast's drawers, and a
# host's recorder. A file under a form's directory that takes a row of its own is another thing's, as the game's
# hooks and the contracts' headers under the recorder's are
FORMS = ('drawer', 'recorder')
# today's second definitions, as {file: its names}: each file is SEGA's own source, which the original compiler
# builds for the match, with SEGA's numbers in its own text. It includes no rules.h where it defines them:
# njModelE.c's two lie on its matching side, and rules.h reaches its other side alone. A new second definition
# takes no line here: its file includes rules.h
KNOWN = {
    'sdk/ninja/kmvertex_kamui_lib.c': {
        'TSP_SRC', 'TSP_DST', 'TSP_ALPHA', 'TSP_IGNORE_ALPHA', 'TSP_FLIP', 'TSP_CLAMP', 'TSP_FILTER',
        'TSP_SUPERSAMPLE', 'TSP_MIPMAP_D', 'TSP_SHADING', 'BLEND_ZERO', 'BLEND_ONE', 'BLEND_SRC_ALPHA',
        'BLEND_INV_SRC_ALPHA', 'SRC', 'DST', 'ISP_DEPTH', 'TSP_FOG'},
    'sdk/ninja/njDebug.c': {'LIST_TOP'},
    'sdk/ninja/njModel.c': {'MATERIAL_ID', 'STRIP_COUNT', 'MATERIAL_TEXTURE'},
    'sdk/ninja/njModelE.c': {'STRIP_COUNT', 'MATERIAL_TEXTURE'},
    'sdk/ninja/njModelS.c': {'STRIP_COUNT', 'MATERIAL_TEXTURE'},
    'sdk/ninja/njTexture.c': {'TSP_UV_SIZE'},
    'sdk/ninja/njkm.c': {
        'GLOBAL_TRANSLUCENT', 'GLOBAL_OBJECT_CONTROL', 'TSP_SRC', 'TSP_DST', 'TSP_ALPHA', 'TSP_IGNORE_ALPHA',
        'TSP_FLIP', 'TSP_CLAMP', 'TSP_FILTER', 'TSP_SUPERSAMPLE', 'TSP_MIPMAP_D', 'TSP_SHADING', 'TSP_UV_SIZE',
        'BLEND_ZERO', 'BLEND_ONE', 'BLEND_SRC_ALPHA', 'BLEND_INV_SRC_ALPHA', 'SRC', 'DST', 'TSP_BLEND', 'TSP_OPAQUE',
        'MATERIAL_TSP', 'LIST_TOP', 'ISP_Z_WRITE_DISABLE'},
    'src/collision/polygon.c': {'STRIP_COUNT'},
    'src/draw/text.c': {'LIST_TOP'},
}
# a macro's definition; a constant's or an enumerator's, a name and the = that gives it its value; a function's,
# which opens a line with its type and runs to the brace that closes its body at a line's start; and an include
MACRO = re.compile(r'^[ \t]*#[ \t]*define[ \t]+(\w+)', re.M)
CONSTANT = re.compile(r'\b([A-Za-z_]\w*)\s*=(?!=)')
FUNCTION = re.compile(r'^[A-Za-z_][^;{}()=\n]*?[ \t*](\w+)\([^;{}]*?\)\s*\{.*?^\}', re.M | re.S)
INCLUDE = re.compile(r'^[ \t]*#[ \t]*include\b.*', re.M)


def defined(text, bodies=True):
    """Each name text, a source's, defines as a macro, a constant, an enumerator or a function, as (its line's
    number, the name), in the text's order. Where bodies is False, none that a function's body holds, which are
    the function's own."""
    bare = csource.blanked(text)
    inside = [] if bodies else [(m.end(1), m.end()) for m in FUNCTION.finditer(bare)]
    found = [(bare.count('\n', 0, m.start(1)) + 1, m[1]) for pattern in (MACRO, CONSTANT, FUNCTION)
             for m in pattern.finditer(bare) if not any(start <= m.start(1) < end for start, end in inside)]
    return sorted(found)


def names(text):
    """The names text, rules.h's, defines, less the guard that opens it: its macros and its functions, and no
    local or word a function's body assigns."""
    return [name for _, name in defined(text, bodies=False)][1:]


def functions(text):
    """The functions text, rules.h's, defines, by name, in the text's order."""
    return [function[1] for function in FUNCTION.finditer(csource.blanked(text))]


def loose(text):
    """What text, rules.h's, holds beside macros and static inline functions, as lines: each include, and each
    line of code that is no directive's and no such function's. C17 and C++17 compile such a header alike, and it
    gives an object no symbol."""
    bare = FUNCTION.sub(lambda function: function[0] if not function[0].startswith('static inline ')
                        else '\n' * function[0].count('\n'), csource.blanked(text))
    problems = [f'{RULES}:{bare.count(chr(10), 0, m.start()) + 1}: includes a file: a source that includes none of '
                "the SDK's headers reads the header, so it includes nothing. Have the source that needs the file "
                'include it' for m in INCLUDE.finditer(bare)]
    directive = False
    for number, line in enumerate(bare.split('\n'), 1):
        if line.strip() and not directive and not line.lstrip().startswith('#'):
            problems.append(f'{RULES}:{number}: code outside a macro and a static inline function: C17 and C++17 '
                            'compile the header alike, and it gives an object no symbol, because it defines those '
                            'alone. Make it one of the two, or put it in a header of its own')
        directive = (directive or line.lstrip().startswith('#')) and line.rstrip().endswith('\\')
    return problems


def second(sources, rules, known):
    """What sources, {path: text}, leave wrong of the one copy, as lines: each definition of a name of rules, the
    header's names, known's set aside, and each name known, {path: names}, lists that its file defines no more."""
    problems = []
    for path in sorted(sources):
        found = [(number, name) for number, name in defined(sources[path]) if name in rules]
        problems += [f'{path}:{number}: defines {name}, which {RULES} defines: the numbers and functions both forms '
                     'of a draw read have one copy. Remove the definition and include rules.h'
                     for number, name in found if name not in known.get(path, ())]
        problems += [f'tests/test_rules.py: KNOWN lists {name} for {path}, which defines it no more: take it out'
                     for name in sorted(set(known.get(path, ())) - {name for _, name in found})]
    problems += [f'tests/test_rules.py: KNOWN lists {path}, which is no source under {", ".join(ROOTS)}: take its '
                 'line out' for path in sorted(set(known) - set(sources))]
    return problems


def uncalled(sources, shared, rows):
    """What sources, {path: text}, leave wrong of the rules that are one function, as lines: each of shared, the
    header's functions, that no source of a form names before a (. A form's sources are those whose row of rows,
    parts.rows()'s, gives them the form as their thing. A form that calls one no more applies the rule by a copy
    of its own, or not at all."""
    problems = []
    for form in FORMS:
        own = [csource.blanked(text) for path, text in sorted(sources.items())
               if (row := parts.part(rows, path)) and row.thing == form]
        problems += [f'{parts.PARTS}: no source whose thing is {form} calls {name}, which {RULES} defines: both '
                     'forms of a draw apply a rule that is one function there. Call it, or where this form applies '
                     'the rule no more, move the function to the form that alone calls it'
                     for name in shared if not any(re.search(rf'\b{name}\s*\(', text) for text in own)]
    return problems


def present():
    """The C and C++ sources and headers under ROOTS that the working tree holds and git tracks or would add, but
    the header itself, as {path: text}."""
    listed = subprocess.run(['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard', '--', *ROOTS],
                            stdout=subprocess.PIPE, check=True).stdout
    return {path: Path(path).read_text(errors='replace') for path in listed.decode().split('\0')
            if path.endswith(SUFFIXES) and path != RULES and os.path.isfile(path)}


HEADER = ('/* the rules */\n#ifndef SA1_RULES_H\n#define SA1_RULES_H\n\n/* masks = numbers */\n#define MATERIAL_ID 0x3fff\n'
          '#define TSP_SRC 0xe0000000\n#define SRC(factor) ((Uint32)(factor) << 29)\n'
          '#define MATERIAL_TSP                \\\n    (TSP_SRC | TSP_DST)\n#define INV_255 0x1.010106p-8F\n\n#endif\n')
LOOSE = 'code outside a macro and a static inline function: C17 and C++17 compile the header alike, and it gives ' \
        'an object no symbol, because it defines those alone. Make it one of the two, or put it in a header of its own'
RULE = f'which {RULES} defines: the numbers and functions both forms of a draw read have one copy. Remove the ' \
       'definition and include rules.h'
# a header that holds a function too, under ninja.h's guard, and the function as a host's source wrote it
MODE = ('static inline Uint32 drawn_mode(Uint32 drawn)\n{\n    if (drawn & NJD_FLAG_USE_ALPHA) {\n'
        '        return MODE_TRANS_TEXTURED;\n    }\n    return MODE_DECAL;\n}\n')
WITH_MODE = HEADER.replace('\n#endif\n', f'#ifdef NINJA_H\n{MODE}#endif\n\n#endif\n')
# a header that holds a function too whose body assigns a local and a word of Ninja's
WORDS = ('static inline void set_header(Uint32 drawn, Uint32 mode)\n{\n    Uint32 tsp = drawn & MATERIAL_TSP;\n\n'
         '    sa1_live_header.TSPPARAMBUFFER = tsp | mode;\n}\n')
WITH_WORDS = WITH_MODE.replace('\n#endif\n\n#endif\n', f'\n#endif\n#ifdef TEXTURE_RECORD\n{WORDS}#endif\n\n#endif\n')
# a table's rows that give each form a directory, and the game a file under the recorder's, and what uncalled()
# says of a function a form calls no more, after the form
ROWS = {'engine/dc/sa1/': parts.Part('drawer', 'picture', 'config/parts.txt:1'),
        'engine/sa1/': parts.Part('recorder', 'play', 'config/parts.txt:2'),
        'engine/sa1/hooks.c': parts.Part('game', 'picture', 'config/parts.txt:3')}
UNCALLED = f'which {RULES} defines: both forms of a draw apply a rule that is one function there. Call it, or where ' \
           'this form applies the rule no more, move the function to the form that alone calls it'


class Rules(unittest.TestCase):
    maxDiff = None

    def test_reads_the_names_a_header_defines_less_its_guard(self):
        self.assertEqual(names(HEADER), ['MATERIAL_ID', 'TSP_SRC', 'SRC', 'MATERIAL_TSP', 'INV_255'])
        self.assertEqual(names('#ifndef SA1_RULES_H\n#define SA1_RULES_H\n#endif\n'), [])
        self.assertEqual(names(WITH_MODE), ['MATERIAL_ID', 'TSP_SRC', 'SRC', 'MATERIAL_TSP', 'INV_255', 'drawn_mode'])

    def test_reads_a_function_s_name_and_none_its_body_assigns(self):
        self.assertEqual(names(WITH_WORDS), ['MATERIAL_ID', 'TSP_SRC', 'SRC', 'MATERIAL_TSP', 'INV_255', 'drawn_mode',
                                             'set_header'])
        self.assertEqual(defined(WORDS, bodies=False), [(1, 'set_header')])
        # a source's own definitions are read whole
        self.assertEqual(defined(WORDS), [(1, 'set_header'), (3, 'tsp'), (5, 'TSPPARAMBUFFER')])

    def test_finds_a_macro_a_constant_and_an_enumerator_and_no_use_or_comment(self):
        text = ('/* #define MATERIAL_ID 1\n * INV_255 = 2 */\n'
                '#define STRIP_COUNT 0x7fff\n'
                '  #  define SRC(factor) ((factor) << 29)\n'
                'constexpr uint32_t MATERIAL_ID = 0x3fff;\n'
                'enum { TSP_SRC = 1, TSP_DST };\n'
                'int count = head & STRIP_COUNT; // TSP_FLIP = 3\n'
                'if (mode == MODE_DECAL || TSP_SRC_SELECT) {}\n')
        self.assertEqual(defined(text), [(3, 'STRIP_COUNT'), (4, 'SRC'), (5, 'MATERIAL_ID'), (6, 'TSP_SRC'),
                                         (7, 'count')])
        self.assertEqual(defined(''), [])

    def test_finds_no_comment_and_no_definition_inside_a_string(self):
        # a string that holds a comment's opening hides no line after it, and a name before an = inside one is no
        # constant
        text = ('const char *opens = "/* INV_255 = 2";\n'
                '#define STRIP_COUNT 0x7fff /* a count */\n'
                'report("TSP_SRC=%d // ", source); enum { TSP_DST = 2 };\n')
        self.assertEqual(defined(text), [(1, 'opens'), (2, 'STRIP_COUNT'), (3, 'TSP_DST')])

    def test_finds_a_function_s_definition_and_no_declaration_or_call(self):
        text = ('Uint32 drawn_mode(Uint32 drawn);\n'
                'static inline uint32_t drawn_mode(uint32_t drawn)\n{\n    return drawn ? MODE_DECAL : 0;\n}\n\n'
                'ALWAYS_INLINE void set_header(const NJS_MATERIAL *material, Uint32 drawn,\n'
                '                              Uint32 mode)\n{\n    if (drawn) {\n        take(drawn_mode(drawn));\n'
                '    }\n}\n')
        self.assertEqual(defined(text), [(2, 'drawn_mode'), (7, 'set_header')])

    def test_finds_nothing_loose_in_a_header_of_macros_and_static_inline_functions(self):
        self.assertEqual(loose(HEADER), [])
        self.assertEqual(loose(WITH_MODE), [])
        self.assertEqual(loose(WITH_WORDS), [])

    def test_names_an_include_and_code_outside_a_macro(self):
        text = HEADER.replace('\n#endif\n', '#include <ninja.h>\nenum { STRIP_COUNT = 0x7fff };\n\n#endif\n')
        self.assertEqual(loose(text), [
            f"{RULES}:12: includes a file: a source that includes none of the SDK's headers reads the header, so it "
            'includes nothing. Have the source that needs the file include it',
            f'{RULES}:13: {LOOSE}'])

    def test_names_a_function_that_is_not_static_inline_and_a_declaration(self):
        text = WITH_MODE.replace('static inline Uint32', 'Uint32').replace(
            '\n#endif\n\n#endif\n', '\n#endif\nextern Uint32 mode;\n\n#endif\n')
        self.assertEqual(loose(text), [f'{RULES}:{number}: {LOOSE}' for number in (*range(13, 20), 21)])

    def test_finds_nothing_wrong_where_the_header_alone_defines_its_names(self):
        # a name of the file's own may have a number of the header's: a ring's mask MATERIAL_ID's, the store queue
        # area's base TSP_SRC's and a sample's scale INV_255's, each another thing
        sources = {'engine/sa1/live.c': '#include "rules.h"\n#define OWN 1\nint id = type & MATERIAL_ID;\n',
                   'engine/pc/sound.cpp': '#define RING_MASK 0x3fff\nconstexpr float SCALE = 0x1.010106p-8F;\n',
                   'engine/dc/batch.h': '#define SQ_BASE 0xe0000000\n',
                   'sdk/ninja/njkm.c': '#define TSP_SRC 0xe0000000\n#define TSP_SRC_SELECT 0x02000000\n'}
        self.assertEqual(second(sources, names(HEADER), {'sdk/ninja/njkm.c': {'TSP_SRC'}}), [])
        self.assertEqual(second({}, names(HEADER), {}), [])

    def test_names_each_second_definition_of_a_name_of_the_header_s(self):
        # a macro, a constant in C++, a function and a name a known file defined none of before, each at its line
        sources = {'engine/sa1/sprites.c': '#include "rules.h"\n\n#define MATERIAL_ID 0x3fff\n',
                   'engine/sa1/live.c': '#include "live.h"\n\n' + MODE.replace('Uint32', 'uint32_t'),
                   'engine/sa1/basic.cpp': 'namespace {\nconstexpr float INV_255 = 0x1.010106p-8F;\n}\n',
                   'sdk/ninja/njkm.c': '#define TSP_SRC 0xe0000000\n#define MATERIAL_TSP (TSP_SRC)\n',
                   'src/draw/model.c': '#define MODULATE_ALPHA 0xc0\n'}
        self.assertEqual(second(sources, names(WITH_MODE), {'sdk/ninja/njkm.c': {'TSP_SRC'}}), [
            f'engine/sa1/basic.cpp:2: defines INV_255, {RULE}',
            f'engine/sa1/live.c:3: defines drawn_mode, {RULE}',
            f'engine/sa1/sprites.c:3: defines MATERIAL_ID, {RULE}',
            f'sdk/ninja/njkm.c:2: defines MATERIAL_TSP, {RULE}'])

    def test_names_what_known_lists_that_is_defined_no_more(self):
        sources = {'sdk/ninja/njkm.c': '#define TSP_SRC 0xe0000000\n'}
        known = {'sdk/ninja/njkm.c': {'TSP_SRC', 'SRC'}, 'sdk/ninja/njGone.c': {'TSP_SRC'}}
        self.assertEqual(second(sources, names(HEADER), known), [
            'tests/test_rules.py: KNOWN lists SRC for sdk/ninja/njkm.c, which defines it no more: take it out',
            'tests/test_rules.py: KNOWN lists sdk/ninja/njGone.c, which is no source under engine, src, sdk: take '
            'its line out'])

    def test_reads_the_functions_a_header_defines(self):
        self.assertEqual(functions(HEADER), [])
        self.assertEqual(functions(WITH_WORDS), ['drawn_mode', 'set_header'])

    def test_finds_nothing_wrong_where_a_source_of_each_form_calls_each_function(self):
        # whichever source of a form calls one, and none where the header holds no function
        sources = {'engine/dc/sa1/model.c': 'set_header(material, drawn, drawn_mode(drawn));\n',
                   'engine/sa1/live.c': 'Uint32 mode = drawn_mode (drawn);\n',
                   'engine/sa1/records.c': 'set_header(material, drawn, mode);\n'}
        self.assertEqual(uncalled(sources, ['drawn_mode', 'set_header'], ROWS), [])
        self.assertEqual(uncalled({}, [], ROWS), [])

    def test_names_each_function_no_source_of_a_form_calls(self):
        # a comment, a longer name, a use that is no call and a call in a source of neither form are no call of
        # the form's: the PC backend's, under no directory of a form's, and the game's hooks, under the recorder's
        # with a row of their own
        sources = {'engine/dc/sa1/model.c': 'set_header(material, drawn, drawn_mode(drawn));\n',
                   'engine/dc/sa1/easy.h': 'easy_set_header(material, flags, mode);\n',
                   'engine/sa1/live.c': '/* set_header(material, drawn, mode) */\nown_header(drawn_mode(drawn));\n'
                                        'write = set_header;\n',
                   'engine/sa1/hooks.c': 'set_header(material, drawn, mode);\n',
                   'engine/pc/scene.cpp': 'set_header(material, drawn, mode);\n'}
        self.assertEqual(uncalled(sources, ['drawn_mode', 'set_header'], ROWS),
                         [f'config/parts.txt: no source whose thing is recorder calls set_header, {UNCALLED}'])
        # where the table gives the recorder no source, the recorder calls none
        self.assertEqual(uncalled(sources, ['drawn_mode'], {'engine/dc/sa1/': ROWS['engine/dc/sa1/']}),
                         [f'config/parts.txt: no source whose thing is recorder calls drawn_mode, {UNCALLED}'])
        self.assertEqual(uncalled({}, ['drawn_mode'], ROWS),
                         [f'config/parts.txt: no source whose thing is drawer calls drawn_mode, {UNCALLED}',
                          f'config/parts.txt: no source whose thing is recorder calls drawn_mode, {UNCALLED}'])

    def test_the_tree_s_header_holds_one_copy_of_each_rule_and_both_forms_call_its_functions(self):
        with cwd(ROOT):
            text = Path(RULES).read_text()
            self.assertEqual(loose(text), [])
            self.assertIn('MATERIAL_ID', names(text))
            self.assertIn('drawn_mode', names(text))
            self.assertIn('set_header', names(text))
            sources = present()
            self.assertEqual(second(sources, names(text), KNOWN), [])
            shared = functions(text)
            self.assertLessEqual({'drawn_mode', 'set_header'}, set(shared))
            self.assertEqual(uncalled(sources, shared, parts.rows()), [])
