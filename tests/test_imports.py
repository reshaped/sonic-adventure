import contextlib
import io
import os
import sys
import time
import unittest
from pathlib import Path
from unittest import mock

import csource
import imports
from support import MODULE, cwd, tree
from test_data import LOADS
from test_relink import definition, obj

ROOT = Path(__file__).resolve().parents[1]

# sections as obj() takes them: 0x10 bytes aligned to 4, P of code and D of data
P, D = (0x10, 4, True), (0x10, 4, False)
# two, an image that loads at the addresses of MODULE's mod and defines an a_init of its own. 1ST_READ.BIN imports
# mod's a_init, two's as two__a_init, and two's b_list
TWO = {
    'config/two/splits.txt': ('P  0x0c900000  0x0c900100\n'
                              'D  0x0c900100  0x0c900200\n'
                              'modules/two/a.c  P  0x0c900000  0x0c900010\n'
                              'gen/two/d_0c900100.c  D  0x0c900100  0x0c900120\n'),
    'config/two/symbols.txt': '0x0c900004  a_init\n0x0c900108  b_list\n0x0c900110  b_tail\n',
    'modules/two/a.c': '',
}
IMPORTS = 'a_init  mod\ntwo__a_init  two  a_init\nb_list  two\n'
FILES = {**MODULE, **TWO, 'config/1st_read/imports.txt': IMPORTS}
# a source that includes a body, which make gcc builds as a stub
BODY = '#include "a.inc"\n'
# the objects that define each a_init where its image's tables put it: mod's at the start of a.c's P, and two's 4
# bytes into its own
OBJECTS = {'build/modules/mod/a.obj': obj('a', {'P': P, 'D': D}, defines=[definition('_a_init', 0)]),
           'build/modules/two/a.obj': obj('a', {'P': P}, defines=[definition('_a_init', 4)])}


# a source whose tables name each of FILES' imports only in a row of a stage that reads its image. LOADS' loader reads
# mod for STAGE_ONE and two for STAGE_TWO. steps' rows take their stages by index. two_pvms takes STAGE_TWO from pvms'
# row for it. Each row of story names its stage
STEPS = '''#include "stage/stage.h"

static void (*steps[STAGE_COUNT])(void) = {
    NULL,
    a_init,
    two__a_init,
};

static struct pvm two_pvms[] = {{"B", &b_list}, {NULL, NULL}};
static struct pvm *pvms[STAGE_COUNT] = {NULL, NULL, two_pvms};

static struct section story[] = {
    {&a_init, STAGE_ONE, 0, "ONE"},
    {&two__a_init, STAGE_TWO, 1, "TWO"},
};
'''
STAGED = {**FILES, **OBJECTS, **LOADS, 'src/stage/steps.c': STEPS,
          'config/1st_read/splits.txt': MODULE['config/1st_read/splits.txt'] + 'src/stage/steps.c  P  0x8c0101b0  '
                                                                               '0x8c0101c0\n'}

# a source whose static functions and table take STAGE_TWO from hooks' row for it. The row names two_hook, which
# names two_start and two_lists. two_start names two__a_init, and two_lists names b_list
CODE = '''#include "stage/stage.h"

#define TWO_INIT (&two__a_init)

static void two_hook(void);

static void (*hooks[STAGE_COUNT])(void) = {NULL, NULL, two_hook};

static struct pvm *two_lists[] = {&b_list};

static void two_start(void)
{
    two__a_init();
}

static void two_hook(void)
{
    two_start();
    pvm_load(two_lists);
}
'''
CODED = {**STAGED, 'src/stage/code.c': CODE,
         'config/1st_read/splits.txt': STAGED['config/1st_read/splits.txt'] + 'src/stage/code.c  P  0x8c0101c0  '
                                                                             '0x8c0101d0\n'}


def unheld(line, place, name):
    """imports.py's line for a use of name, one of IMPORTS', in place at code.c's line, with no stage and no row of
    uses.txt."""
    row, image = {'a_init': (1, 'mod'), 'two__a_init': (2, 'two'), 'b_list': (3, 'two')}[name]
    return (f'src/stage/code.c:{line}: {place} names {name}, which config/1st_read/imports.txt:{row} takes from '
            f'{image}, for no stage this source gives it, and config/1st_read/uses.txt gives it no row for {image}')


def check(files, *argv):
    """imports.py's status and output, run with argv in a tree of files: 0 or the message it fails with, and what it
    printed."""
    out = io.StringIO()
    with tree(files), contextlib.redirect_stdout(out), mock.patch.object(sys, 'argv', ['tools/imports.py']):
        try:
            imports.main(list(argv))
        except SystemExit as failed:
            return str(failed), out.getvalue()
    return 0, out.getvalue()


class Definitions(unittest.TestCase):
    def test_holds_each_import_to_its_images_definition(self):
        self.assertEqual(check({**FILES, **OBJECTS}), (0, ''))

    def test_takes_a_gen_range_for_each_name_the_tables_give_in_it(self):
        # b_list and b_tail lie in two's gen/ range, which needs no object
        files = {**FILES, 'config/1st_read/imports.txt': 'b_list  two\nb_tail  two\n'}
        self.assertEqual(check(files), (0, ''))

    def test_rejects_a_name_its_source_defines_no_global_of(self):
        files = {**FILES, **OBJECTS, 'build/modules/two/a.obj': obj('a', {'P': P})}
        self.assertEqual(check(files)[0], 'imports: config/1st_read/imports.txt:2: two__a_init: '
                                          'build/modules/two/a.obj defines no a_init, which two puts at 0x0c900004')

    def test_rejects_a_definition_elsewhere_than_its_images_table_puts_it(self):
        for defined, at in ((definition('_a_init', 8), 'P + 0x8'), (definition('_a_init', 0, section=1), 'D + 0x0')):
            with self.subTest(at=at):
                files = {**FILES, **OBJECTS, 'build/modules/mod/a.obj': obj('a', {'P': P, 'D': D}, defines=[defined])}
                self.assertEqual(check(files)[0], 'imports: config/1st_read/imports.txt:1: a_init: '
                                                  f'build/modules/mod/a.obj defines a_init at {at}, and mod puts it '
                                                  'at 0x0c900000, in P')

    def test_rejects_a_name_defined_at_an_address(self):
        # an .EQU's a_init, which no layout of the object's sections moves, fails even at the address the tables give
        defined = definition('_a_init', 0x0C900000, absolute=True)
        files = {**FILES, **OBJECTS, 'build/modules/mod/a.obj': obj('a', {'P': P, 'D': D}, defines=[defined])}
        self.assertEqual(check(files)[0], 'imports: config/1st_read/imports.txt:1: a_init: build/modules/mod/a.obj '
                                          'defines a_init at the address 0x0c900000, in none of its sections, and mod '
                                          'puts it at 0x0c900000, in P')

    def test_rejects_an_address_no_row_holds(self):
        files = {**FILES, **OBJECTS, 'config/two/symbols.txt': '0x0c900004  a_init\n0x0c900040  b_list\n'}
        self.assertEqual(check(files)[0], 'imports: config/1st_read/imports.txt:3: b_list: '
                                          'no row of config/two/splits.txt holds 0x0c900040')

    def test_reads_the_stub_where_the_source_has_bodies(self):
        # two's stub defines a_init 8 bytes into P, where two's tables put it 4 bytes in. The check reads the stub
        # while a.c includes a body, and a.c's own object once it doesn't, whichever of the two is the later
        stub = obj('a', {'P': P}, defines=[definition('_a_init', 8)])
        files = {**FILES, **OBJECTS, 'modules/two/a.c': BODY, 'build/stub/modules/two/a.obj': stub}
        with tree(files):
            later = time.time() + 10
            os.utime('build/modules/two/a.obj', (later, later))
            with self.assertRaisesRegex(SystemExit, r'build/stub/modules/two/a\.obj defines a_init at P \+ 0x8'):
                imports.main([])
            Path('modules/two/a.c').write_text('')
            os.utime('build/stub/modules/two/a.obj', (later + 10, later + 10))
            imports.main([])

    def test_needs_the_object_built(self):
        # a source with bodies needs its stub, even beside its own object
        for files, missing in (({**FILES, 'build/modules/mod/a.obj': OBJECTS['build/modules/mod/a.obj']},
                                'build/modules/two/a.obj'),
                               ({**FILES, **OBJECTS, 'modules/two/a.c': BODY}, 'build/stub/modules/two/a.obj')):
            with self.subTest(missing=missing):
                self.assertEqual(check(files)[0], f'imports: modules/two/a.c: no {missing}, so build it first')


class Uses(unittest.TestCase):
    def test_holds_each_stages_row_to_the_images_its_stage_reads(self):
        self.assertEqual(check(STAGED), (0, ''))

    def test_rejects_a_row_that_names_an_image_its_stage_doesnt_read(self):
        # in each, a row of STAGE_ONE's, which reads mod alone, names an import from two
        cases = {'a row by its index': (
                     '    a_init,\n    two__a_init,\n', '    two__a_init,\n    two__a_init,\n',
                     "src/stage/steps.c:5: steps[1], STAGE_ONE's row, names two__a_init, which "
                     'config/1st_read/imports.txt:2 takes from two, and loader_read_stage_files reads mod for '
                     'STAGE_ONE'),
                 'a row of a list a stage names': (
                     '{NULL, NULL, two_pvms}', '{NULL, two_pvms, NULL}',
                     "src/stage/steps.c:9: two_pvms[0], STAGE_ONE's row, names b_list, which "
                     'config/1st_read/imports.txt:3 takes from two, and loader_read_stage_files reads mod for '
                     'STAGE_ONE'),
                 'a row that names its stage': (
                     '{&two__a_init, STAGE_TWO', '{&two__a_init, STAGE_ONE',
                     "src/stage/steps.c:14: story[1], STAGE_ONE's row, names two__a_init, which "
                     'config/1st_read/imports.txt:2 takes from two, and loader_read_stage_files reads mod for '
                     'STAGE_ONE')}
        for case, (old, new, problem) in cases.items():
            with self.subTest(case=case):
                files = {**STAGED, 'src/stage/steps.c': STEPS.replace(old, new)}
                self.assertEqual(check(files)[0], f'imports: {problem}')

    def test_gives_no_row_an_index_where_rows_arent_elements_by_place(self):
        # a_init is late's second row but not its element 1. In the first late its designator makes it element 2, and
        # in the second, a table of arrays, it fills part of element 0. So no row of late has an index, and a_init
        # takes no stage. By its place it would be STAGE_ONE's, which reads mod. A row of uses.txt for late[1] covers
        # no use, and one for late covers it
        use = ('src/stage/steps.c:17: late names a_init, which config/1st_read/imports.txt:1 takes from mod, for no '
               'stage this source gives it, and config/1st_read/uses.txt gives it no row for mod')
        stale = ("config/1st_read/uses.txt:1: src/stage/steps.c's late[1] names no import from mod that its stages "
                 "don't hold")
        for table in ('[STAGE_COUNT])(void) = {NULL, [2] = a_init}', '[STAGE_COUNT][2])(void) = {NULL, a_init}'):
            with self.subTest(table=table):
                files = {**STAGED, 'src/stage/steps.c': f'{STEPS}\nstatic void (*late{table};\n'}
                self.assertEqual(check(files)[0], f'imports: {use}')
                for row, status in (('late[1]', f'imports: {use}\nimports: {stale}'), ('late', 0)):
                    listed = {**files, 'config/1st_read/uses.txt': f'src/stage/steps.c  {row}  mod  a reason\n'}
                    self.assertEqual(check(listed)[0], status)

    def test_rejects_a_row_of_a_stage_the_loader_reads_nothing_for(self):
        files = {**STAGED, 'src/stage/steps.c': STEPS.replace('    NULL,\n    a_init,\n', '    a_init,\n    a_init,\n')}
        self.assertEqual(check(files)[0], 'imports: src/stage/steps.c:4: steps[0], stage 0\'s row, names a_init, which '
                                          'config/1st_read/imports.txt:1 takes from mod, and loader_read_stage_files '
                                          'reads no image for stage 0')

    def test_holds_a_use_to_the_images_its_stage_always_reads(self):
        # in each, STAGE_TWO's case doesn't read two whenever it loads the stage: it reads it under a test of the
        # character, braced or not, or past a return under one, or not at all
        read = '        loader_read_prs("two.PRS", MODULE_AREA);\n'
        test = '        if (get_character() == CHARACTER_SONIC)'
        problem = ("imports: src/stage/steps.c:6: steps[2], STAGE_TWO's row, names two__a_init, which "
                   'config/1st_read/imports.txt:2 takes from two, and loader_read_stage_files reads no image for '
                   'STAGE_TWO')
        for new in (f'{test} {{\n    {read}        }}\n', f'{test}\n    {read}', f'{test}\n            return;\n{read}',
                    ''):
            with self.subTest(read=new):
                loader = LOADS['src/system/loader.c'].replace(read, new)
                self.assertIn(problem, check({**STAGED, 'src/system/loader.c': loader})[0].splitlines())

    def test_passes_a_rows_stages_to_the_static_tables_and_functions_it_names(self):
        self.assertEqual(check(CODED), (0, ''))
        # in hooks' row for STAGE_ONE, which reads mod alone, two_hook passes STAGE_ONE to two_start and two_lists
        two = 'which config/1st_read/imports.txt:{} takes from two, and loader_read_stage_files reads mod for STAGE_ONE'
        files = {**CODED, 'src/stage/code.c': CODE.replace('{NULL, NULL, two_hook}', '{NULL, two_hook, NULL}')}
        self.assertEqual(check(files)[0], f"imports: src/stage/code.c:9: two_lists[0], STAGE_ONE's row, names b_list, "
                                          f"{two.format(3)}\n"
                                          f"imports: src/stage/code.c:13: two_start's code for STAGE_ONE names "
                                          f"two__a_init, {two.format(2)}")

    def test_gives_a_table_or_function_no_stage_unless_it_is_static_and_each_place_naming_it_has_some(self):
        # two_lists and two_start take no stage without static, since another source can name them. Nor does
        # two_lists where run_lists, a function with no stage, names it, or two_start where file scope does
        for old, new, problem in (
                ('static struct pvm *two_lists', 'struct pvm *two_lists', unheld(9, 'two_lists[0]', 'b_list')),
                ('static void two_start', 'void two_start', unheld(13, 'two_start', 'two__a_init')),
                (CODE, f'{CODE}void run_lists(void)\n{{\n    pvm_load(two_lists);\n}}\n',
                 unheld(9, 'two_lists[0]', 'b_list')),
                (CODE, f'{CODE}TASKS(two_start);\n', unheld(13, 'two_start', 'two__a_init'))):
            with self.subTest(new=new):
                self.assertEqual(check({**CODED, 'src/stage/code.c': CODE.replace(old, new)})[0], f'imports: {problem}')

    def test_passes_a_rows_stages_to_statics_that_name_each_other(self):
        # two_a and two_b call each other, and cycle's row for STAGE_TWO names two_a, so both take STAGE_TWO. run_two,
        # a place outside them with no stage, leaves them none. Without cycle, nothing outside them names them, and
        # they take none
        pair = ('static void two_b(void);\n\nstatic void two_a(void)\n{\n    two__a_init();\n    two_b();\n}\n\n'
                'static void two_b(void)\n{\n    two_a();\n}\n')
        cycle = 'static void (*cycle[STAGE_COUNT])(void) = {NULL, NULL, two_a};\n'
        line = (CODE + pair).count('\n', 0, (CODE + pair).index('two__a_init();\n    two_b')) + 1
        for code, status in ((f'{CODE}{pair}{cycle}', 0),
                             (f'{CODE}{pair}{cycle}void run_two(void)\n{{\n    two_b();\n}}\n',
                              f"imports: {unheld(line, 'two_a', 'two__a_init')}"),
                             (f'{CODE}{pair}', f"imports: {unheld(line, 'two_a', 'two__a_init')}")):
            with self.subTest(code=code):
                self.assertEqual(check({**CODED, 'src/stage/code.c': code})[0], status)

    def test_takes_no_stage_from_code(self):
        # a test of the current stage gives the code under it no stage. In the last body the test governs the for
        # alone. The call after it runs in every stage but STAGE_TWO, where the loop never ends
        for body in ('    switch (stage_current) {\n    case STAGE_TWO:\n        two__a_init();\n    }\n',
                     '    if (stage_current == STAGE_TWO) {\n        two__a_init();\n    }\n',
                     '    if (stage_current == STAGE_TWO || IN_STAGE(STAGE_TWO)) {\n        two__a_init();\n    }\n',
                     '    if (stage_current != STAGE_ONE) {\n        two__a_init();\n    }\n',
                     '    if (IN_STAGE(STAGE_TWO))\n        for (;;) {\n        }\n    two__a_init();\n'):
            with self.subTest(body=body):
                code = f'{CODE}void steps_run(void)\n{{\n{body}}}\n'
                line = code.count('\n', 0, code.rindex('two__a_init();')) + 1
                self.assertEqual(check({**CODED, 'src/stage/code.c': code})[0],
                                 f"imports: {unheld(line, 'steps_run', 'two__a_init')}")
                rows = {'config/1st_read/uses.txt': 'src/stage/code.c  steps_run  two  a reason\n'}
                self.assertEqual(check({**CODED, 'src/stage/code.c': code, **rows}), (0, ''))

    def test_rejects_a_use_no_stage_holds_and_no_row_covers(self):
        # run_two, a function with no stage, names two_hook. So two_hook takes no stage, nor do two_start and
        # two_lists
        files = {**CODED, 'src/stage/code.c': CODE + 'void run_two(void)\n{\n    two_hook();\n}\n'}
        self.assertEqual(check(files)[0], f"imports: {unheld(9, 'two_lists[0]', 'b_list')}\n"
                                          f"imports: {unheld(13, 'two_start', 'two__a_init')}")
        rows = 'src/stage/code.c  two_start  two  a reason\nsrc/stage/code.c  two_lists  two  a reason\n'
        self.assertEqual(check({**files, 'config/1st_read/uses.txt': rows}), (0, ''))
        # a row that covers no use fails: two_start's for mod, which it takes nothing from, and hooks[1]'s, which names
        # no import
        for row, problem in (('src/stage/code.c  two_start  mod  a reason\n', 'two_start names no import from mod'),
                             ('src/stage/code.c  hooks[1]  two  a reason\n', 'hooks[1] names no import from two')):
            with self.subTest(row=row):
                status = check({**CODED, 'config/1st_read/uses.txt': row})[0]
                self.assertEqual(status, f"imports: config/1st_read/uses.txt:1: src/stage/code.c's {problem} that its "
                                         "stages don't hold")

    def test_takes_a_row_of_uses_txt_for_one_row_of_a_table(self):
        # models' rows name no stage, so each needs a row of uses.txt for its image
        files = {**CODED, 'src/stage/code.c': CODE + 'static void *models[2] = {TWO_INIT, &a_init};\n'}
        rows = 'src/stage/code.c  models[0]  two  a reason\nsrc/stage/code.c  models[1]  mod  a reason\n'
        self.assertEqual(check({**files, 'config/1st_read/uses.txt': rows}), (0, ''))
        # a row for one row of a table covers that row alone: models[1]'s for two covers nothing in models[0]
        rows = 'src/stage/code.c  models[1]  two  a reason\nsrc/stage/code.c  models[0]  mod  a reason\n'
        self.assertIn(unheld(21, 'models[0]', 'two__a_init'), check({**files, 'config/1st_read/uses.txt': rows})[0])
        # a row for one row of a table fails where a row for the whole table gives the same image
        rows = ('src/stage/code.c  models  two  a reason\nsrc/stage/code.c  models[0]  two  a reason\n'
                'src/stage/code.c  models[1]  mod  a reason\n')
        self.assertEqual(check({**files, 'config/1st_read/uses.txt': rows})[0],
                         "imports: config/1st_read/uses.txt:2: src/stage/code.c's models[0] needs no row for two: "
                         'config/1st_read/uses.txt:1 covers all of models')

    def test_takes_no_declaration_for_a_use(self):
        # a declaration that opens with a keyword and holds no = names no use, extern or not. Neither does one in
        # run_two, a function with no stage
        for declaration in ('extern void two__a_init(void);', 'void two__a_init(void);', 'extern struct pvm b_list;'):
            for scope, code in (('file', f'{declaration}\n'),
                                ('function', f'void run_two(void)\n{{\n    {declaration}\n}}\n')):
                with self.subTest(declaration=declaration, scope=scope):
                    self.assertEqual(check({**CODED, 'src/stage/code.c': CODE + code}), (0, ''))

    def test_takes_no_member_or_parameter_for_a_use(self):
        # run_two's parameter and the members spelled as imports name no use, and its call of two__a_init names one
        code = (f'{CODE}int run_two(struct pvm *b_list)\n{{\n    two__a_init();\n'
                '    return b_list->a_init + b_list[1].a_init;\n}\n')
        line = code.count('\n', 0, code.index('two__a_init();\n    return')) + 1
        self.assertEqual(check({**CODED, 'src/stage/code.c': code})[0],
                         f"imports: {unheld(line, 'run_two', 'two__a_init')}")
        # a member spelled as two_start names no place of it, so two_start keeps STAGE_TWO from two_hook
        code = f'{CODE}void run_hooks(struct hooks *hooks)\n{{\n    hooks->two_start();\n}}\n'
        self.assertEqual(check({**CODED, 'src/stage/code.c': code}), (0, ''))
        # a parameter of a pointer to a function, and a struct's tag, name none of run_two's parameters
        for parameters in ('void (*done)(int two__a_init)', 'struct two__a_init *'):
            with self.subTest(parameters=parameters):
                code = f'{CODE}void run_two({parameters})\n{{\n    two__a_init();\n}}\n'
                line = code.count('\n', 0, code.rindex('two__a_init();')) + 1
                self.assertEqual(check({**CODED, 'src/stage/code.c': code})[0],
                                 f"imports: {unheld(line, 'run_two', 'two__a_init')}")

    def test_rejects_a_use_at_file_scope(self):
        # a statement that opens with a name, a macro's or a typedef's, names uses, and no row of uses.txt covers one
        # at file scope
        for code in (f'{CODE}TWO_HOOK(two__a_init);\n', f'{CODE}Uint8 two__a_init(void);\n'):
            with self.subTest(code=code):
                line = code.count('\n', 0, code.rindex('two__a_init')) + 1
                files = {**CODED, 'src/stage/code.c': code}
                self.assertEqual(check(files)[0], f"imports: {unheld(line, 'file scope', 'two__a_init')}")

    def test_fails_on_a_bracket_at_file_scope_that_nothing_closes(self):
        files = {**CODED, 'src/stage/code.c': CODE + 'static void *models[2] = {&a_init;\n'}
        self.assertEqual(check(files)[0], 'imports: src/stage/code.c:21: an unclosed {')

    def test_reads_a_use_of_the_sources_macro_and_rejects_a_headers(self):
        files = {**CODED, 'src/stage/code.c': CODE.replace('{NULL, NULL, two_hook}', '{NULL, TWO_INIT, two_hook}')}
        self.assertIn("src/stage/code.c:7: hooks[1], STAGE_ONE's row, names two__a_init", check(files)[0])
        # a header's macro that names an import fails where a source includes the header, found under src/ or
        # modules/. It passes where no source includes the header
        for header, include in (('src/stage/two.h', 'stage/two.h'), ('modules/two/two.h', 'two/two.h')):
            with self.subTest(header=header):
                code = CODE.replace('#include "stage/stage.h"\n', f'#include "stage/stage.h"\n#include "{include}"\n')
                files = {**CODED, header: '#define TWO_INIT \\\n    (&two__a_init)\n'}
                self.assertEqual(check(files), (0, ''))
                self.assertEqual(check({**files, 'src/stage/code.c': code})[0],
                                 f'imports: {header}: TWO_INIT is a macro that names an import, in a file '
                                 'src/stage/code.c includes, so no use of it is held to a stage')

    def test_reads_a_source_through_a_link_as_a_source(self):
        # steps.c is a link to held/steps.c, the path included() keys it by:
        # still the source, and no file it includes
        files = {name: text for name, text in STAGED.items() if name != 'src/stage/steps.c'}
        with tree({**files, 'src/stage/held/steps.c': STEPS}), contextlib.redirect_stdout(io.StringIO()), \
                mock.patch.object(sys, 'argv', ['tools/imports.py']):
            os.symlink('held/steps.c', 'src/stage/steps.c')
            imports.main([])

    def test_rejects_an_import_named_in_a_file_a_source_includes(self):
        # the check holds no use in a file a source includes to a stage: a function's in a header, or a macro's in a
        # file of another kind. A declaration there names no use
        code = CODE.replace('#include "stage/stage.h"\n', '#include "stage/stage.h"\n#include "stage/two.h"\n'
                                                           '#include "stage/two.inc"\n')
        files = {**CODED, 'src/stage/code.c': code, 'src/stage/two.h': 'extern void two__a_init(void);\n',
                 'src/stage/two.inc': ''}
        self.assertEqual(check(files), (0, ''))
        header = 'extern void two__a_init(void);\nstatic void two_help(void)\n{\n    two__a_init();\n}\n'
        self.assertEqual(check({**files, 'src/stage/two.h': header})[0],
                         'imports: src/stage/two.h:4: two_help names two__a_init, which config/1st_read/imports.txt:2 '
                         'takes from two, in a file src/stage/code.c includes, so no use of it is held to a stage')
        self.assertEqual(check({**files, 'src/stage/two.inc': '#define TWO_HELP two__a_init\n'})[0],
                         'imports: src/stage/two.inc: TWO_HELP is a macro that names an import, in a file '
                         'src/stage/code.c includes, so no use of it is held to a stage')
        # the line names the first source that includes the file, as splits.txt orders them
        steps = STEPS.replace('#include "stage/stage.h"\n', '#include "stage/stage.h"\n#include "stage/two.inc"\n')
        self.assertEqual(check({**files, 'src/stage/two.inc': '#define TWO_HELP two__a_init\n',
                                'src/stage/steps.c': steps})[0],
                         'imports: src/stage/two.inc: TWO_HELP is a macro that names an import, in a file '
                         'src/stage/steps.c includes, so no use of it is held to a stage')

    def test_reads_a_macro_where_its_definition_stands(self):
        # hooks[1] names TWO_INIT above the #undef that ends its definition. later's row names the definition after
        # it, which names no import
        code = CODE.replace('{NULL, NULL, two_hook}', '{NULL, TWO_INIT, two_hook}').replace(
            'static struct pvm *two_lists', '#undef TWO_INIT\n#define TWO_INIT NULL\nvoid *later[] = {TWO_INIT};\n\n'
                                            'static struct pvm *two_lists')
        self.assertEqual(check({**CODED, 'src/stage/code.c': code})[0],
                         "imports: src/stage/code.c:7: hooks[1], STAGE_ONE's row, names two__a_init, which "
                         'config/1st_read/imports.txt:2 takes from two, and loader_read_stage_files reads mod for '
                         'STAGE_ONE')

    def test_takes_no_parameter_of_a_macro_for_an_import(self):
        # PICK(NULL) is NULL, though PICK's parameter is spelled as an import is
        code = CODE.replace('#define TWO_INIT (&two__a_init)', '#define PICK(two__a_init) two__a_init').replace(
            '{NULL, NULL, two_hook}', '{NULL, PICK(NULL), two_hook}')
        self.assertEqual(check({**CODED, 'src/stage/code.c': code}), (0, ''))
        # LATER's body keeps its comma when its parameters go. So two__a_init is late's row 2, STAGE_TWO's, as in C
        code = (f'{CODE}#define LATER(first, unused) first, two__a_init\n'
                'void (*late[STAGE_COUNT])(void) = {NULL, LATER(NULL, 0)};\n')
        self.assertEqual(check({**CODED, 'src/stage/code.c': code}), (0, ''))

    def test_fails_on_a_macro_whose_paste_can_make_an_imports_name(self):
        # INIT(two) is two__a_init, but the tool pastes no tokens, so INIT fails in a source or a header. A paste that
        # makes a member's name passes, and so does one that can make no import's name
        fails = "imports: {}:{}: INIT's ## can make an import's name, and the tool pastes no tokens"
        header = CODE.replace('#include "stage/stage.h"\n', '#include "stage/stage.h"\n#include "stage/two.h"\n')
        for macro, status in (('#define INIT(part) part##__a_init', fails.format('src/stage/code.c', 3)),
                              ('#define INIT(part) hooks.part##_init', 0), ('#define INIT(part) zam_##part', 0)):
            with self.subTest(macro=macro):
                code = CODE.replace('#define TWO_INIT (&two__a_init)', macro)
                self.assertEqual(check({**CODED, 'src/stage/code.c': code})[0], status)
        files = {**CODED, 'src/stage/code.c': header, 'src/stage/two.h': '#define INIT(part) part##__a_init\n'}
        self.assertEqual(check(files)[0], fails.format('src/stage/two.h', 1))

    def test_reads_a_macro_whole_where_a_comment_spans_its_lines(self):
        # the splices inside the comment continue the definition past it
        macro = '#define TWO_INIT \\\n    /* the second \\\n       image */ \\\n    (&two__a_init)'
        code = CODE.replace('#define TWO_INIT (&two__a_init)', macro)
        self.assertEqual(check({**CODED, 'src/stage/code.c': code}), (0, ''))
        self.assertIn("src/stage/code.c:10: hooks[1], STAGE_ONE's row, names two__a_init",
                      check({**CODED, 'src/stage/code.c': code.replace('{NULL, NULL, two_hook}',
                                                                        '{NULL, TWO_INIT, two_hook}')})[0])
        code = CODE.replace('#include "stage/stage.h"\n', '#include "stage/stage.h"\n#include "stage/two.h"\n')
        self.assertEqual(check({**CODED, 'src/stage/two.h': f'{macro}\n', 'src/stage/code.c': code})[0],
                         'imports: src/stage/two.h: TWO_INIT is a macro that names an import, in a file '
                         'src/stage/code.c includes, so no use of it is held to a stage')

    def test_reads_each_side_of_a_guard_as_its_compiler_compiles_it(self):
        # each side of a guard may open a statement's brace, as a guard of a condition alone does
        code = CODE.replace('    two_start();\n', '#ifdef GCC\n    if (gcc_ready()) {\n#else\n    if (TRUE) {\n#endif\n'
                                                   '        two_start();\n    }\n')
        self.assertEqual(check({**CODED, 'src/stage/code.c': code}), (0, ''))
        # a use on either side is held to the stages. two_start takes STAGE_TWO, which reads two alone, so a_init
        # fails there on either side. A use only GCC's side makes is GCC's
        mod = 'which config/1st_read/imports.txt:1 takes from mod, and loader_read_stage_files reads two for STAGE_TWO'
        use = '{\n    two__a_init();\n'
        for guard, line, gcc in (('#ifdef GCC\n    a_init();\n#endif\n', 15, "GCC's "),
                                 ('#ifndef GCC\n    a_init();\n#endif\n', 15, ''),
                                 ('#ifdef GCC\n    two__a_init();\n#else\n    a_init();\n#endif\n', 17, '')):
            with self.subTest(guard=guard):
                status = check({**CODED, 'src/stage/code.c': CODE.replace(use, use + guard)})[0]
                self.assertEqual(status, f"imports: src/stage/code.c:{line}: {gcc}two_start's code for STAGE_TWO "
                                         f'names a_init, {mod}')

    def test_reads_the_c_in_gcc_in_place_of_a_src_file_it_replaces(self):
        # GCC compiles gcc/src/stage/hooks.c in place of hooks.src, so its use of two__a_init, for no stage, is GCC's
        hook = 'void hook(void)\n{\n    two__a_init();\n}\n'
        files = {**CODED, 'src/stage/hooks.src': '', 'gcc/src/stage/hooks.c': hook,
                 'config/1st_read/splits.txt': CODED['config/1st_read/splits.txt'] + 'src/stage/hooks.src  P  '
                                                                                     '0x8c0101d0  0x8c0101e0\n'}
        self.assertEqual(check(files)[0], "imports: gcc/src/stage/hooks.c:3: GCC's hook names two__a_init, which "
                                          'config/1st_read/imports.txt:2 takes from two, for no stage this source '
                                          'gives it, and config/1st_read/uses.txt gives it no row for two')
        listed = {**files, 'config/1st_read/uses.txt': 'gcc/src/stage/hooks.c  hook  two  a reason\n'}
        self.assertEqual(check(listed), (0, ''))

    def test_reads_the_c_an_add_gives_1st_read_as_gcc_compiles_it(self):
        # make gcc links gcc/src/system/added.c into 1ST_READ.BIN, so its use of two__a_init, for no stage, is GCC's.
        # An --add for another image adds nothing to 1ST_READ.BIN
        files = {**CODED, 'gcc/src/system/added.c': 'void added(void)\n{\n    two__a_init();\n}\n'}
        self.assertEqual(check(files, '--add', '1st_read', 'gcc/src/system/added.c')[0],
                         "imports: gcc/src/system/added.c:3: GCC's added names two__a_init, which "
                         'config/1st_read/imports.txt:2 takes from two, for no stage this source gives it, and '
                         'config/1st_read/uses.txt gives it no row for two')
        self.assertEqual(check(files, '--add', 'two', 'gcc/src/system/added.c'), (0, ''))
        listed = {**files, 'config/1st_read/uses.txt': 'gcc/src/system/added.c  added  two  a reason\n'}
        self.assertEqual(check(listed, '--add', '1st_read', 'gcc/src/system/added.c'), (0, ''))

    def test_reads_a_functions_name_as_csource_does(self):
        # an attribute names no function, so g doesn't take f's place. run_two's body follows its parameter's
        # declaration, and holds its use
        for code, place in ((f'{CODE}__attribute__((noinline)) void f(void)\n{{\n    two__a_init();\n}}\n\n'
                             '__attribute__((noinline)) void g(void)\n{\n}\n', 'f'),
                            (f'{CODE}void run_two(done)\n    int done;\n{{\n    two__a_init();\n}}\n\n'
                             'void run_late(void)\n{\n}\n', 'run_two')):
            with self.subTest(place=place):
                line = code.count('\n', 0, code.rindex('two__a_init();')) + 1
                self.assertEqual(check({**CODED, 'src/stage/code.c': code})[0],
                                 f"imports: {unheld(line, place, 'two__a_init')}")

    def test_reads_the_macros_of_each_side_of_a_guard_for_its_compiler(self):
        # GCC's TWO_INIT names two__a_init, and SHC's names none, so hooks[1] names the import for GCC alone
        guarded = '#ifdef GCC\n#define TWO_INIT (&two__a_init)\n#else\n#define TWO_INIT NULL\n#endif'
        code = CODE.replace('#define TWO_INIT (&two__a_init)', guarded).replace('{NULL, NULL, two_hook}',
                                                                              '{NULL, TWO_INIT, two_hook}')
        line = code.count('\n', 0, code.index('{NULL, TWO_INIT')) + 1
        self.assertEqual(check({**CODED, 'src/stage/code.c': code})[0],
                         f"imports: src/stage/code.c:{line}: GCC's hooks[1], STAGE_ONE's row, names two__a_init, which "
                         'config/1st_read/imports.txt:2 takes from two, and loader_read_stage_files reads mod for '
                         'STAGE_ONE')

    def test_reads_a_name_a_splice_splits_as_the_name(self):
        # C splices lines before it reads names, so two__a_init with a splice inside is a use of it, in code and in a
        # macro's body
        code = f'{CODE}void run_two(void)\n{{\n    two__a\\\n_init();\n}}\n'
        line = code.count('\n', 0, code.index('two__a\\\n_init')) + 1
        self.assertEqual(check({**CODED, 'src/stage/code.c': code})[0],
                         f"imports: {unheld(line, 'run_two', 'two__a_init')}")
        code = CODE.replace('(&two__a_init)', '(&two__a\\\n_init)').replace('{NULL, NULL, two_hook}',
                                                                          '{NULL, TWO_INIT, two_hook}')
        self.assertIn("src/stage/code.c:8: hooks[1], STAGE_ONE's row, names two__a_init",
                      check({**CODED, 'src/stage/code.c': code})[0])

    def test_reads_the_line_a_comment_splices_as_part_of_the_comment(self):
        # C splices lines before it removes comments. So a // comment that ends in a backslash takes in the next
        # line, and the macro names no import
        macro = '#define TWO_INIT 0 // the second \\\n    (&two__a_init)'
        code = CODE.replace('#define TWO_INIT (&two__a_init)', macro)
        self.assertEqual(check({**CODED, 'src/stage/code.c': code}), (0, ''))


class Repository(unittest.TestCase):
    # the repository's own sources and tables, which use_problems() reads without the disc or a build
    def use_problems(self, path=None, old='', new=''):
        """use_problems() for the repository, with new in place of old in the source at path, which must hold old."""
        read = csource.content

        def content(at):
            text = read(at)
            if str(at) != path:
                return text
            self.assertIn(old.encode(), text)
            return text.replace(old.encode(), new.encode())

        with cwd(ROOT), mock.patch.object(csource, 'content', content):
            return imports.use_problems()

    def test_holds_each_use_of_an_import_to_its_stages_or_a_row_of_uses_txt(self):
        self.assertEqual(self.use_problems(), [])

    def test_rejects_the_rhythm_brooch_and_the_shovel_claws_swapped(self):
        # ADV03's model of Tails' rhythm brooch and ADV02's of Knuckles' shovel claws swap rows
        swapped = self.use_problems('src/object/level_item.c', """\
    {&level_item_rhythm_brooch_object, &tails_texlist},
    {&level_item_fighting_gloves_object, &knuckles_texlist},
    {&level_item_shovel_claws_object, &knuckles_texlist},""", """    {&level_item_shovel_claws_object, &tails_texlist},
    {&level_item_fighting_gloves_object, &knuckles_texlist},
    {&level_item_rhythm_brooch_object, &knuckles_texlist},""")
        # uses.txt's row for level_item_models[4] gives adv03, the brooch's image, and its row for [6] gives adv02,
        # the claws'. Neither covers the model swapped in, and the row for [4] then covers no use
        none = 'for no stage this source gives it, and config/1st_read/uses.txt gives it no row for'
        self.assertIn('src/object/level_item.c:220: level_item_models[4] names level_item_shovel_claws_object, which '
                      f'config/1st_read/imports.txt:137 takes from adv02, {none} adv02', swapped)
        self.assertIn('src/object/level_item.c:222: level_item_models[6] names level_item_rhythm_brooch_object, which '
                      f'config/1st_read/imports.txt:192 takes from adv03, {none} adv03', swapped)
        self.assertIn("config/1st_read/uses.txt:32: src/object/level_item.c's level_item_models[4] names no import "
                      "from adv03 that its stages don't hold", swapped)


if __name__ == '__main__':
    unittest.main()
