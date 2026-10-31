import unittest
from pathlib import Path

from stages import stages, uses
from support import MODULE, cwd, tree

ROOT = Path(__file__).resolve().parents[1]


class Uses(unittest.TestCase):
    def uses(self, rows):
        with tree({**MODULE, 'config/1st_read/uses.txt': rows}):
            return uses()

    def test_reads_each_rows_place_image_and_reason(self):
        rows = '# by source\nsrc/main.c  main  mod  the reason\nsrc/main.c  steps[12]  mod  a row\'s reason\n'
        self.assertEqual(self.uses(rows), [('src/main.c', 'main', None, 'mod', 'the reason', 'config/1st_read/uses.txt:2'),
                                           ('src/main.c', 'steps', 12, 'mod', "a row's reason",
                                            'config/1st_read/uses.txt:3')])

    def test_has_none_without_the_table(self):
        with tree(MODULE):
            self.assertEqual(uses(), [])

    def test_rejects_a_row_out_of_form(self):
        for row, problem in (('src/main.c  main  mod', 'not a row: SOURCE PLACE IMAGE REASON'),
                             ('src/main.c  steps[01]  mod  a reason', r'steps\[01\]: not a place, a name or NAME\[N\]'),
                             ('src/main.c  main  three  a reason', 'three: no such image, a directory in config/'),
                             ('src/main.c  main  mod  a reason\nsrc/main.c  main  mod  another',
                              'src/main.c main mod has a row already, at config/1st_read/uses.txt:1')):
            with self.subTest(row=row), self.assertRaisesRegex(SystemExit, r'config/1st_read/uses\.txt:\d: ' + problem):
                self.uses(f'{row}\n')


class Stages(unittest.TestCase):
    # a second image, two, and the stages' numbers, one with a comment after it, as stage.h writes some
    FILES = {**MODULE, 'config/two/splits.txt': 'P  0x0c900000  0x0c900100\n',
             'src/stage/stage.h': '#define STAGE_ONE 1\n#define STAGE_TWO 2 /* the second */\n#define STAGE_THREE 3\n'}

    def stages(self, cases):
        loader = f'void loader_read_stage_files(void)\n{{\n    switch (stage) {{\n{cases}    }}\n}}\n'
        with tree({**self.FILES, 'src/system/loader.c': loader}):
            return stages()

    def test_gives_each_stage_the_images_its_case_reads_in_order(self):
        # STAGE_ONE and STAGE_TWO share a case. It reads TWO.PRS and two.PRS, one image in two spellings, and TEXT.PRS,
        # which is no image. Each comment spells a label or a read, and holds neither
        cases = ('    case STAGE_THREE:\n        /* loader_read_prs("MOD.PRS", 0); */\n        return;\n'
                 '    case STAGE_ONE:\n    case STAGE_TWO: /* case STAGE_THREE: */\n'
                 '        loader_read_prs("TWO.PRS", 0);\n        loader_read_prs("TEXT.PRS", 0);\n'
                 '        loader_read_prs("MOD.PRS", 0);\n        loader_read_prs("two.PRS", 0);\n        break;\n')
        self.assertEqual(self.stages(cases), {1: ('STAGE_ONE', ['two', 'mod']), 2: ('STAGE_TWO', ['two', 'mod']),
                                              3: ('STAGE_THREE', [])})

    def test_leaves_out_the_reads_not_on_every_pass(self):
        # each case reads two.PRS, but not on every pass: STAGE_ONE under a braced if, STAGE_TWO under a bare one, and
        # STAGE_THREE after an if's return
        cases = ('    case STAGE_ONE:\n        loader_read_prs("mod.PRS", MODULE_AREA);\n        if (x) {\n'
                 '            loader_read_prs("two.PRS", EXTRA_AREA);\n        }\n        return;\n'
                 '    case STAGE_TWO:\n        if (x)\n            loader_read_prs("two.PRS", EXTRA_AREA);\n'
                 '        loader_read_prs("mod.PRS", MODULE_AREA);\n        return;\n'
                 '    case STAGE_THREE:\n        loader_read_prs("mod.PRS", MODULE_AREA);\n        if (x)\n'
                 '            return;\n        loader_read_prs("two.PRS", EXTRA_AREA);\n        return;\n')
        self.assertEqual(self.stages(cases), {1: ('STAGE_ONE', ['mod']), 2: ('STAGE_TWO', ['mod']),
                                              3: ('STAGE_THREE', ['mod'])})

    def test_gives_none_without_the_loader(self):
        with tree(MODULE):
            self.assertEqual(stages(), {})

    def test_gives_no_stage_the_reads_after_default(self):
        cases = ('    case STAGE_ONE:\n        loader_read_prs("set.PRS", MODULE_AREA);\n        break;\n'
                 '    default:\n        loader_read_prs("mod.PRS", MODULE_AREA);\n        break;\n')
        self.assertEqual(self.stages(cases), {1: ('STAGE_ONE', [])})

    def test_rejects_a_case_that_falls_into_the_next_or_has_no_number(self):
        # a read after if (x) return; falls into the next label too
        read = '        loader_read_prs("mod.PRS", MODULE_AREA);\n'
        for cases, problem in (
                (f'    case STAGE_ONE:\n{read}    case STAGE_TWO:\n        return;\n',
                 '6: case STAGE_TWO: the case before it reads a file and falls into it'),
                (f'    case STAGE_ONE:\n        if (x)\n            return;\n{read}    case STAGE_TWO:\n'
                 '        return;\n', '8: case STAGE_TWO: the case before it reads a file and falls into it'),
                (f'    case STAGE_ONE:\n{read}    default:\n        return;\n',
                 '6: default: the case before it reads a file and falls into it'),
                ('    case STAGE_FOUR:\n        return;\n',
                 '4: case STAGE_FOUR, which src/stage/stage.h gives no number')):
            with self.subTest(cases=cases), self.assertRaisesRegex(SystemExit, f'src/system/loader.c:{problem}$'):
                self.stages(cases)

    def test_rejects_a_directive_in_the_function(self):
        # a guard's sides can read different files, which stages() doesn't tell apart
        cases = ('    case STAGE_ONE:\n#ifdef GCC\n        loader_read_prs("mod.PRS", MODULE_AREA);\n#endif\n'
                 '        return;\n')
        with self.assertRaisesRegex(SystemExit, r"src/system/loader\.c:5: loader_read_stage_files holds #ifdef GCC, "
                                                r"whose sides stages\(\) doesn't tell apart$"):
            self.stages(cases)

    def test_rejects_a_loader_without_the_function(self):
        with tree({**self.FILES, 'src/system/loader.c': 'void loader_read_stage_files(void);\n'}), \
                self.assertRaisesRegex(SystemExit, r'src/system/loader\.c: no definition of loader_read_stage_files\(void\)'):
            stages()

    def test_reads_the_repositorys_stages(self):
        with cwd(ROOT):
            found = stages()
        # STAGE_CHAOS7's case reads b_chaos7.PRS and then TIKAL_PROG.PRS, whose name stages() lowercases
        self.assertEqual(found[19], ('STAGE_CHAOS7', ['b_chaos7', 'tikal_prog']))
        self.assertTrue(all(images for _, images in found.values()))
        # STAGE_SUNABOARD's case reads the board motions for Sonic and Tails alone, so stages() doesn't give them
        self.assertEqual(found[38], ('STAGE_SUNABOARD', ['sboard']))


if __name__ == '__main__':
    unittest.main()
