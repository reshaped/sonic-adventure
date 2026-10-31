import contextlib
import re
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import elf
import gcclink
import memory
import spec
import test_ld
from images import images
from support import GAME_TREE, GAME_TREE_C, cwd, tree

ROOT = Path(__file__).resolve().parents[1]

# main.c, taking stage_secret too, and stga's a.c, with a static stage_secret of its own
STATIC = {'src/main.c': GAME_TREE['src/main.c'].replace('extern int stage_flag;', 'extern int stage_flag;\n'
                                                        'extern int stage_secret;').replace('main_count_alias;\n}',
                                                                                            'main_count_alias + '
                                                                                            'stage_secret;\n}'),
          'modules/stga/a.c': GAME_TREE['modules/stga/a.c'] + 'static int stage_secret = 3;\n\nint *stage_peek(void)\n'
                                                              '{\n    return &stage_secret;\n}\n'}
# main.c, taking stga__stage_flag too, its own name for stga's stage_flag, which no table gives
UNTABLED = {'src/main.c': GAME_TREE['src/main.c'].replace('extern int stage_flag;', 'extern int stage_flag;\n'
                                                          'extern int stga__stage_flag;').replace(
    'main_count_alias;\n}', 'main_count_alias + stga__stage_flag;\n}')}
# ALIAS makes main_count_alias a second name for main's main_count, at the address main's table gives both. PROVIDER is
# ALIAS and a row that imports main's stage_flag from stga
ALIAS = '1st_read  main_count_alias  1st_read  main_count  -  main counts under a second name\n'
PROVIDER = ALIAS + '1st_read  stage_flag  stga  stage_flag  0x8c  the beach is the stage main means\n'
# ev0001's script.c, taking stage_flag
TAKING = ('extern int stage_flag;\nint ev0001_count;\n\nvoid ev0001_script(int phase)\n{\n'
          '    ev0001_count += phase + stage_flag;\n}\n')
# TAKING, taking ev0001_tally too, which test_publish.py's rows make a second name for ev0001_count
TALLYING = TAKING.replace('int ev0001_count;', 'extern int ev0001_tally;\nint ev0001_count;').replace(
    'phase + stage_flag;', 'phase + stage_flag + ev0001_tally;')
# the name stga's content gives its word at 0x0c900104, which points at 0x0c900200, in stga's bss, where no table
# names a definition
SPELLED = 'unresolved.stga.0c900104.0c900200'
# stga's gen/ range that holds the word, and the content's record
GENERATED = {'config/stga/splits.txt': GAME_TREE['config/stga/splits.txt'] + 'gen/stga/d_0c900104.c  D  0x0c900104  '
                                                                              '0x0c900108\n',
             'build/gen/stga/content.txt': (f'dat_0c900104  0x0c900104  4  void *[]\n{SPELLED}  0x0c900200  -  '
                                            'unresolved: the word at 0x0c900104 points where no table names a '
                                            'definition\n')}
# the content's C, which defines the word
POINTING = f'extern char unresolved_0c900104[] __asm__("{SPELLED}");\nvoid *dat_0c900104[] = {{unresolved_0c900104}};\n'
# main.c, calling copy_lines, and the C of the archive member that defines it
COPYING = {'src/main.c': GAME_TREE['src/main.c'].replace(
    'extern int main_count_alias;', 'extern int main_count_alias;\nextern int copy_lines(void);').replace(
    'main_count_alias;\n}', 'main_count_alias + copy_lines();\n}')}
LIBRARY = 'int copy_lines(void)\n{\n    return 2;\n}\n'
# copy_lines again: reading stage_bss, which stga defines and main's objects don't take; and calling bar, which
# EARLY, another archive's member, defines
READING = 'extern int stage_bss;\n\nint copy_lines(void)\n{\n    return stage_bss;\n}\n'
LATE = 'extern int bar(void);\n\nint copy_lines(void)\n{\n    return bar();\n}\n'
EARLY = 'int bar(void)\n{\n    return 1;\n}\n'
# main.c, taking copy_lines weakly
WEAKLY = GAME_TREE['src/main.c'].replace('extern int main_count_alias;', 'extern int main_count_alias;\n'
                                         'extern int copy_lines(void) __attribute__((weak));').replace(
    'main_count_alias;\n}', 'main_count_alias + (copy_lines ? copy_lines() : 0);\n}')
# main.c, taking fixed and buf, which FIXED's object defines: fixed absolute, and buf common
FIXING = GAME_TREE['src/main.c'].replace('extern int main_count_alias;', 'extern int main_count_alias;\n'
                                         'extern char fixed[];\nextern char buf[16];').replace(
    'main_count_alias;\n}', 'main_count_alias + fixed[0] + buf[0];\n}')
FIXED = '        .global _fixed\n        .set _fixed, 0x8c100000\n        .comm _buf,16,4\n'
# stga's a.c, calling stgb's stage_b
CALLING = GAME_TREE['modules/stga/a.c'] + ('extern int stage_b(void);\n\nint stage_a_calls(void)\n{\n'
                                          '    return stage_b();\n}\n')
# the Unit spec.py reads of each of GAME_TREE's GCC objects. test_reads_each_object_as_units_gives_it holds them to the
# objects GCC compiles, so the tests of Bound, which read them in place of the objects, need no toolchain
UNITS = {'build/gcc/src/main.o': spec.Unit({'main', 'main_count'}, set(), {'_main_count_alias', '_stage_flag'}),
         'build/gcc/modules/stga/a.o': spec.Unit({'stage_a', 'stage_bss', 'stage_flag'}, set(), set()),
         'build/gcc/modules/stgb/b.o': spec.Unit({'stage_b', 'stage_flag'}, set(), set()),
         'build/gcc/events/ev0001/head.o': spec.Unit(set(), {'ev0001_head'}, {'_ev0001_script'}),
         'build/gcc/events/ev0001/script.o': spec.Unit({'ev0001_count', 'ev0001_script'}, set(), set())}
# the Unit spec.py reads of CALLING's object, which the same test holds to it
CALLING_UNIT = spec.Unit({'stage_a', 'stage_a_calls', 'stage_bss', 'stage_flag'}, set(), {'_stage_b'})


def compiled(text):
    """The bytes of the GCC object of the C text, compiled under GCCFLAGS. A message from GCC fails it."""
    with tempfile.TemporaryDirectory() as root:
        Path(root, 'source.c').write_text(text)
        done = subprocess.run([str(test_ld.TOOLCHAIN / 'bin/sh-elf-gcc'), *test_ld.flags(), '-c', '-o', 'source.o',
                               'source.c'], cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if done.returncode or done.stdout:
            raise AssertionError(done.stdout)
        return Path(root, 'source.o').read_bytes()


def archived(text):
    """A GNU ar archive whose one member is the GCC object of the C text."""
    with tempfile.TemporaryDirectory() as root:
        Path(root, 'member.o').write_bytes(compiled(text))
        subprocess.run([str(test_ld.TOOLCHAIN / 'bin/sh-elf-ar'), 'rcsD', 'library.a', 'member.o'], cwd=root,
                       check=True)
        return Path(root, 'library.a').read_bytes()


def assembled(path):
    """The bytes of the object GNU as assembles from the file at path under GASFLAGS. A message from GNU as fails
    it."""
    with tempfile.TemporaryDirectory() as root:
        done = subprocess.run([str(test_ld.TOOLCHAIN / 'bin/sh-elf-as'), *test_ld.flags('GASFLAGS'), '-o', 'out.o',
                               str(path)], cwd=root, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        if done.returncode or done.stdout:
            raise AssertionError(done.stdout)
        return Path(root, 'out.o').read_bytes()


class Generator(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        missing = [tool for tool in test_ld.TOOLS if not (test_ld.TOOLCHAIN / tool).is_file()]
        if missing and not test_ld.GIVEN:
            raise unittest.SkipTest(f'{test_ld.TOOLCHAIN} lacks {", ".join(missing)}, which make gcc builds')
        cls.objects, cls.static, cls.untabled = {}, {}, {}
        for sources, objects in ((GAME_TREE, cls.objects), ({**GAME_TREE, **STATIC}, cls.static),
                                 ({**GAME_TREE, **UNTABLED}, cls.untabled)):
            for source in GAME_TREE_C:
                objects[str(Path('build/gcc', source).with_suffix('.o'))] = compiled(sources[source])
        cls.taking = compiled(TAKING)
        cls.tallying = compiled(TALLYING)
        cls.pointing = compiled(POINTING)
        cls.calling = compiled(CALLING)
        cls.copying = {**cls.objects, 'build/gcc/src/main.o': compiled(COPYING['src/main.c'])}
        cls.library = archived(LIBRARY)
        cls.reading = archived(READING)
        cls.late, cls.early = archived(LATE), archived(EARLY)
        cls.weakly = {**cls.objects, 'build/gcc/src/main.o': compiled(WEAKLY)}
        cls.fixing = {**cls.objects, 'build/gcc/src/main.o': compiled(FIXING)}
        with tempfile.TemporaryDirectory() as root:
            Path(root, 'fixed.s').write_text(FIXED)
            cls.fixed = assembled(Path(root, 'fixed.s'))

    @contextlib.contextmanager
    def game_tree(self, files=None, objects=None):
        """GAME_TREE with objects, GAME_TREE's GCC objects by default, and files over them, and gcclink.SH_ELF set to
        test_ld.TOOLCHAIN."""
        with tree({**GAME_TREE, **(objects or self.objects), **(files or {})}) as root, \
                mock.patch.object(gcclink, 'SH_ELF', test_ld.TOOLCHAIN):
            yield root

    def written(self, *args):
        """The lines of the spec spec.py writes for args."""
        spec.main([*args, 'spec.txt'])
        return Path('spec.txt').read_text().splitlines()

    def test_reads_each_object_as_units_gives_it(self):
        self.assertEqual({path: spec.unit(elf.read(data, path)) for path, data in self.objects.items()}, UNITS)
        self.assertEqual(spec.unit(elf.read(self.calling, 'calling')), CALLING_UNIT)

    def test_fails_where_two_images_live_with_it_provide_a_name_and_no_row_gives_the_provider(self):
        # main runs beside each stage in some state, and both stages define stage_flag at the address main's table
        # gives it, which both their spans hold
        with self.game_tree({'config/imports.txt': ALIAS}):
            with self.assertRaisesRegex(SystemExit, r': 1st_read: 1 names nothing binds: ambiguous 1 '
                                                    r'\(stage_flag\)\n.*: 1 of the 4 images take names nothing binds$'):
                spec.main(['spec.txt'])
            self.assertFalse(Path('spec.txt').exists())

    def test_fails_where_two_images_define_a_name_though_only_one_s_tables_name_it(self):
        # stgb's table gives its stage_flag no address, so stgb still provides the name, and it stays ambiguous
        with self.game_tree({'config/imports.txt': ALIAS, 'config/stgb/symbols.txt': ''}), \
                self.assertRaisesRegex(SystemExit, r': 1st_read: 1 names nothing binds: ambiguous 1 \(stage_flag\)\n'):
            self.written()

    def test_takes_the_provider_config_imports_txt_gives_in_its_view(self):
        with self.game_tree({'config/imports.txt': PROVIDER}):
            lines = self.written()
        self.assertIn('import    1st_read  stage_flag  stga  stage_flag  0x8c', lines)

    def test_takes_main_s_provider_from_its_own_table_of_imports(self):
        # main's own table of imports names stga for stage_flag, which both stages define. The import takes the view
        # stga's table spells
        files = {'config/imports.txt': ALIAS, 'config/1st_read/imports.txt': 'stage_flag  stga\n',
                 'config/1st_read/symbols.txt':
                     GAME_TREE['config/1st_read/symbols.txt'].replace('0x8c900010  stage_flag\n', '')}
        with self.game_tree(files):
            lines = self.written()
        self.assertIn('import    1st_read  stage_flag  stga  stage_flag  0x0c', lines)
        # a row of config/imports.txt for the name too fails, as a second row for it
        with self.game_tree({**files, 'config/imports.txt': PROVIDER}), \
                self.assertRaisesRegex(SystemExit, 'config/imports.txt:2: 1st_read binds stage_flag already, at '
                                                   'config/1st_read/imports.txt:1'):
            self.written()

    def test_takes_a_row_for_a_name_no_table_gives_in_the_row_s_view(self):
        # main's own name for stga's stage_flag, as main names an overlay's definition where two overlays define it
        row = '1st_read  stga__stage_flag  stga  stage_flag  0x0c  the beach flag, under a name of main\'s\n'
        with self.game_tree({'config/imports.txt': PROVIDER + row, **UNTABLED}, self.untabled):
            lines = self.written()
        self.assertIn('import    1st_read  stga__stage_flag  stga  stage_flag  0x0c', lines)

    def test_a_row_for_every_image_binds_each_image_that_takes_the_name(self):
        files = {'config/imports.txt': ALIAS + '*  stage_flag  stga  stage_flag  0x8c  the beach is the stage meant\n',
                 'config/ev0001/symbols.txt': GAME_TREE['config/ev0001/symbols.txt'] + '0x8c900010  stage_flag\n'}
        with self.game_tree(files, {**self.objects, 'build/gcc/events/ev0001/script.o': self.taking}):
            lines = self.written()
        self.assertIn('import    1st_read  stage_flag  stga  stage_flag  0x8c', lines)
        self.assertIn('import    ev0001  stage_flag  stga  stage_flag  0x8c', lines)

    def test_fails_on_a_row_whose_view_the_image_s_tables_don_t_spell(self):
        row = ALIAS + '1st_read  stage_flag  stga  stage_flag  0x0c  the beach is the stage main means\n'
        with self.game_tree({'config/imports.txt': row}), \
                self.assertRaisesRegex(SystemExit, r'config/imports.txt:2: config/1st_read/ gives stage_flag at '
                                                   r'0x8c900010, in the 0x8c view, and the row takes the 0x0c view'):
            self.written()

    def test_fails_on_a_second_name_no_row_gives(self):
        # main's table gives main_count_alias main_count's address, and no row makes it a second name
        with self.game_tree({'config/imports.txt': PROVIDER.replace(ALIAS, '')}), \
                self.assertRaisesRegex(SystemExit, r': 1st_read: 1 names nothing binds: second 1 '
                                                   r'\(main_count_alias\)\n'):
            self.written()

    def test_writes_each_image_s_rows_from_its_tables_its_objects_and_the_memory_contract(self):
        with self.game_tree({'config/imports.txt': PROVIDER}):
            lines = self.written()
            archive = str(gcclink.libgcc())
        rows = {kind: [line.split()[1:] for line in lines if line.split()[0] == kind] for kind in
                ('image', 'input', 'import', 'alias', 'head', 'anchor')}
        # each image loads at its tables' base. Its load area runs to the next image or area that starts past that
        # base in a state that holds it. main's runs to the stages'. A stage's runs to the events' area: GAME_TREE's
        # loader reads no set or camera file, so their areas, which start sooner, hold nothing. An event's runs to
        # the motion area
        self.assertEqual(rows['image'], [['1st_read', '0x8c010000', '0x008f0000', 'START_SYS'],
                                         ['ev0001', '0x0cb80000', '0x00080000'], ['stga', '0x0c900000', '0x00280000'],
                                         ['stgb', '0x0c900000', '0x00280000']])
        # ev0001's objects in relink's order, then libgcc.a
        self.assertEqual([path for image, path in rows['input'] if image == 'ev0001'],
                         ['build/gcc/events/ev0001/head.o', 'build/gcc/events/ev0001/script.o', archive])
        # main's second name, and the records main's startup and event_load read: DSGLE's anchor is its run's start
        self.assertEqual(rows['alias'], [['1st_read', 'main_count_alias', 'main_count']])
        self.assertIn(['ev0001', 'DHEAD', 'build/gcc/events/ev0001/head.o', '.data'], rows['head'])
        self.assertIn(['ev0001', 'ev0001_head', '0x0', '8'], rows['anchor'])
        self.assertIn(['1st_read', '_startof_DSGLE', '0x80'], rows['anchor'])
        self.assertEqual([row for row in rows['head'] if row[0] == '1st_read'][0],
                         ['1st_read', 'DSGLH', 'build/gcc/sdk/shinobi/strt1.o', 'DSGLH'])

    def test_links_each_archive_after_an_image_s_objects_and_ahead_of_libgcc(self):
        # main calls copy_lines, which only the library's member defines: without the archive nothing binds it
        files = {'config/imports.txt': PROVIDER, **COPYING, 'build/gcc/library.a': self.library}
        with self.game_tree(files, self.copying), \
                self.assertRaisesRegex(SystemExit, r': 1st_read: 1 names nothing binds: untabled 1 \(copy_lines\)\n'):
            self.written()
        with self.game_tree(files, self.copying):
            lines = self.written('--archive', 'build/gcc/library.a')
            archive = str(gcclink.libgcc())
        self.assertEqual([line.split()[2] for line in lines if line.split()[:2] == ['input', '1st_read']][-2:],
                         ['build/gcc/library.a', archive])
        self.assertNotIn('copy_lines', ' '.join(lines))

    def test_binds_what_each_member_the_link_takes_refers_to_and_takes_none_else(self):
        # the member that defines main's copy_lines reads stage_bss, which main's tables put in stga's span
        tables = {'config/1st_read/symbols.txt': GAME_TREE['config/1st_read/symbols.txt'] + '0x8c900014  stage_bss\n'}
        files = {'config/imports.txt': PROVIDER, **COPYING, **tables, 'build/gcc/library.a': self.reading}
        with self.game_tree(files, self.copying):
            lines = self.written('--archive', 'build/gcc/library.a')
        self.assertIn(['1st_read', 'stage_bss', 'stga', 'stage_bss', '0x8c'],
                      [line.split()[1:] for line in lines if line.split()[0] == 'import'])
        # a weak reference takes no member, so nothing defines copy_lines, and stage_bss goes untaken
        with self.game_tree(files, self.weakly), \
                self.assertRaisesRegex(SystemExit, r': 1st_read: 1 names nothing binds: untabled 1 \(copy_lines\)\n'):
            self.written('--archive', 'build/gcc/library.a')
        # late.a's member takes bar, which early.a defines. The link searches early.a first and takes none of its
        # members, so nothing binds bar but in the other order
        files = {'config/imports.txt': PROVIDER, **COPYING, 'build/gcc/early.a': self.early,
                 'build/gcc/late.a': self.late}
        with self.game_tree(files, self.copying):
            with self.assertRaisesRegex(SystemExit, r': 1st_read: 1 names nothing binds: untabled 1 \(bar\)\n'):
                self.written('--archive', 'build/gcc/early.a', '--archive', 'build/gcc/late.a')
            self.written('--archive', 'build/gcc/late.a', '--archive', 'build/gcc/early.a')

    def test_takes_an_absolute_or_common_definition_for_the_image_s_own_references(self):
        # fixed.o defines fixed with .set and buf with .comm. main takes both, and its own link binds them, with no row
        files = {'config/imports.txt': PROVIDER, 'build/gcc/fixed.o': self.fixed}
        with self.game_tree(files, self.fixing):
            lines = self.written('--add', '1st_read', 'build/gcc/fixed.o')
        self.assertFalse([line for line in lines if line.split()[0] != 'input' and ('fixed' in line or 'buf' in line)])

    def test_fails_on_a_row_that_binds_nothing_or_whose_provider_doesn_t_define_its_symbol(self):
        for row, problem in (('stgb  stage_flag  stga  stage_flag  0x0c  stale\n',
                              'stgb takes no stage_flag from outside, so the row binds nothing'),
                             ('1st_read  stage_flag  stga  stage_none  0x8c  wrong\n',
                              "stga's objects don't define stage_none"),
                             ('1st_read  stage_flag  stga  stage_flag  0x8c\n',
                              'not a row: IMAGE NAME PROVIDER SYMBOL VIEW REASON, '),
                             ('1st_read  stage_flag  stga  stage_flag  0x8c  hand-off:\n', 'not a row: '),
                             ('1st_read  main_count_alias  1st_read  main_count  0x8c  a view\n', 'not a row: '),
                             ('1st_read  stage_flag  stga  stage_flag  -  no view\n', 'not a row: ')):
            with self.subTest(row), self.game_tree({'config/imports.txt': row}), \
                    self.assertRaisesRegex(SystemExit, f'config/imports.txt:1: {problem}'):
                self.written()

    def test_classifies_each_name_nothing_binds_by_what_its_holder_s_tables_put_there(self):
        # stage_secret is a static of stga's, at an address in stga's bss, which only stga's span holds
        files = {'config/imports.txt': PROVIDER, **STATIC,
                 'config/1st_read/symbols.txt': GAME_TREE['config/1st_read/symbols.txt'] + '0x8c900250  stage_secret\n',
                 'config/stga/symbols.txt': GAME_TREE['config/stga/symbols.txt'] + '0x0c900250  stage_secret\n'}
        with self.game_tree(files, self.static):
            with self.assertRaisesRegex(SystemExit, r': 1st_read: 1 names nothing binds: static 1 \(stage_secret\)\n'):
                spec.main(['--list', 'unbound.txt', 'spec.txt'])
            self.assertEqual(Path('unbound.txt').read_text(), '1st_read  static  stage_secret\n')

    def test_reports_a_name_ambiguous_where_two_images_hold_its_address_though_one_s_tables_name_it(self):
        # both stages' spans hold 0x0c900020, and only stga's tables name stage_secret there: no image's own table
        # makes it the holder
        files = {'config/imports.txt': PROVIDER, **STATIC,
                 'config/1st_read/symbols.txt': GAME_TREE['config/1st_read/symbols.txt'] + '0x8c900020  stage_secret\n',
                 'config/stga/symbols.txt': GAME_TREE['config/stga/symbols.txt'] + '0x0c900020  stage_secret\n'}
        with self.game_tree(files, self.static), \
                self.assertRaisesRegex(SystemExit, r': 1st_read: 1 names nothing binds: ambiguous 1 '
                                                   r'\(stage_secret\)\n'):
            spec.main(['spec.txt'])

    def test_links_an_image_s_content_in_place_of_its_gen_ranges_and_marks_a_name_only_it_takes(self):
        # the content's word points into stga's bss, where no object lies: storage, which the content alone takes
        with self.game_tree({'config/imports.txt': PROVIDER, **GENERATED},
                       {**self.objects, 'build/gcc/gen/stga/content.o': self.pointing}):
            with self.assertRaisesRegex(SystemExit, r': stga: 1 names nothing binds: storage 1 '
                                                    r'\(unresolved\.stga\.0c900104\.0c900200\); its content alone '
                                                    r'takes 1\n'):
                spec.main(['--list', 'unbound.txt', 'spec.txt'])
            self.assertEqual(Path('unbound.txt').read_text(), f'stga  storage  {SPELLED}  gen/\n')
            self.assertEqual([str(path) for path in spec.inputs(spec.Config('stga'))],
                             ['build/gcc/modules/stga/a.o', 'build/gcc/gen/stga/content.o'])

    def test_writes_a_kernel_row_for_each_function_of_the_image_s_kernel_table(self):
        kernel = {'config/1st_read/kernel.txt': '# the kernel\nsrc/main.c  main\n',
                  'config/stga/kernel.txt': 'modules/stga/a.c  stage_a\n'}
        with self.game_tree({'config/imports.txt': PROVIDER, **kernel}):
            lines = self.written()
        # GCC puts main in .text.startup.main, which spec.py reads from the object
        self.assertIn('kernel    1st_read  build/gcc/src/main.o  .text.startup.main', lines)
        self.assertIn('kernel    stga  build/gcc/modules/stga/a.o  .text.stage_a', lines)

    def test_fails_on_a_kernel_row_whose_object_doesn_t_define_its_function_or_whose_section_a_row_places(self):
        # src/main.src and stga's a.c name objects make gcc builds, but main's tables place neither
        rows = {'src/main.c  main  more\n': 'config/1st_read/kernel.txt:1: not a row: SOURCE FUNCTION',
                'src/main.src  main\n': 'config/1st_read/kernel.txt:1: config/1st_read/ places no src/main.src',
                'modules/stga/a.c  stage_a\n': ('config/1st_read/kernel.txt:1: config/1st_read/ places no '
                                                'modules/stga/a.c'),
                'src/main.c  absent\n': ('config/1st_read/kernel.txt:1: build/gcc/src/main.o defines no absent in '
                                         'an allocated section'),
                'src/main.c  main\nsrc/main.c  main\n': ('config/1st_read/kernel.txt:2: main lies in '
                                                         "build/gcc/src/main.o's .text.startup.main, which an "
                                                         'earlier row places')}
        for table, problem in rows.items():
            with self.subTest(table), self.game_tree({'config/imports.txt': PROVIDER,
                                                      'config/1st_read/kernel.txt': table}):
                with self.assertRaisesRegex(SystemExit, f'{re.escape(problem)}$'):
                    self.written()


class Bound(unittest.TestCase):
    # spec.py's bindings and checks, without a toolchain: spec.py reads each object as UNITS, or a test's units, gives
    # it, and libgcc.a as an archive with no member

    @contextlib.contextmanager
    def game_tree(self, files=None, units=None):
        """GAME_TREE with an empty object at each path of units, UNITS by default, and files over them. spec.py reads
        each object as units gives it, and the link selects by what it defines and takes."""
        units = units or UNITS
        with tree({**GAME_TREE, **{path: b'' for path in units}, 'libgcc.a': elf.ARCHIVE, **(files or {})}) as root, \
                mock.patch.object(gcclink, 'libgcc', lambda: Path('libgcc.a')), \
                mock.patch.object(elf, 'read', lambda data, where: where), \
                mock.patch.object(gcclink, 'linkable', lambda found: gcclink.Linkable(
                    {f'_{name}' for name in units[str(found)].defined}, set(units[str(found)].taken), set())), \
                mock.patch.object(spec, 'unit', lambda found: units[str(found)]):
            yield root

    written = Generator.written

    def test_holds_a_row_of_main_s_own_table_to_the_contract_but_in_a_hand_off(self):
        # main's own table of imports names stga for stage_flag, in a contract where stga never runs beside main. The
        # row binds only as a hand-off, and only where a state holds main whole and stga's file, read and not yet
        # started
        files = {'config/imports.txt': ALIAS, 'config/1st_read/imports.txt': 'stage_flag  stga{}\n',
                 'config/1st_read/symbols.txt':
                     GAME_TREE['config/1st_read/symbols.txt'].replace('0x8c900010  stage_flag\n', '')}
        contract = memory.contract

        def stga_never_live(known, hand_offs=()):
            terms = contract(known)
            return terms._replace(colive={**terms.colive, '1st_read': terms.colive['1st_read'] - {'stga'}},
                                  states=[*terms.states, *hand_offs])

        with self.game_tree({**files,
                             'config/1st_read/imports.txt': files['config/1st_read/imports.txt'].format('')}), \
                mock.patch.object(memory, 'contract', stga_never_live), \
                self.assertRaisesRegex(SystemExit, "config/1st_read/imports.txt:1: 1st_read takes stage_flag from "
                                                   "stga, which the memory contract's states never hold live with it, "
                                                   'and the row gives no reason that opens with hand-off:'):
            self.written()
        handoff_table = files['config/1st_read/imports.txt'].format('  hand-off: main reads the beach, then starts it')
        main_hands_off = memory.State('main hands off', {'1st_read': 'whole', 'stga': 'file'}, [], False)
        with self.game_tree({**files, 'config/1st_read/imports.txt': handoff_table}):
            with mock.patch.object(memory, 'contract', stga_never_live), \
                    self.assertRaisesRegex(SystemExit, "config/1st_read/imports.txt:1: a hand-off, and no state of the "
                                                       "memory contract holds stga's file, read and not yet started, "
                                                       'with 1st_read'):
                self.written()
            with mock.patch.object(memory, 'contract', lambda known: stga_never_live(known, [main_hands_off])):
                lines = self.written()
        self.assertIn('import    1st_read  stage_flag  stga  stage_flag  0x0c', lines)

    def test_fails_on_a_provider_never_live_with_its_image_but_in_a_hand_off(self):
        # the two stages are never live together. A hand-off of stga's needs a state that holds stga whole and stgb's
        # file, read and not yet started
        row = 'stga  stage_b  stgb  stage_b  0x0c  {}the beach calls the other stage\n'
        units = {**UNITS, 'build/gcc/modules/stga/a.o': CALLING_UNIT}
        with self.game_tree({'config/imports.txt': PROVIDER + row.format('')}, units), \
                self.assertRaisesRegex(SystemExit, "config/imports.txt:3: stga takes stage_b from stgb, which the "
                                                   "memory contract's states never hold live with it, and the row "
                                                   'gives no reason that opens with hand-off:'):
            self.written()
        with self.game_tree({'config/imports.txt': PROVIDER + row.format('hand-off: ')}, units):
            with self.assertRaisesRegex(SystemExit, "config/imports.txt:3: a hand-off, and no state of the memory "
                                                    "contract holds stgb's file, read and not yet started, with stga"):
                self.written()
            contract = memory.contract

            def handing_off(known, part='whole'):
                terms = contract(known)
                return terms._replace(states=[*terms.states, memory.State('the beach hands off', {
                    'stga': part, 'stgb': 'file'}, [], False)])

            # a state that holds stga as its file, not whole, holds no hand-off of stga's
            with mock.patch.object(memory, 'contract', lambda known: handing_off(known, 'file')), \
                    self.assertRaisesRegex(SystemExit, "config/imports.txt:3: a hand-off, and no state of the memory "
                                                       "contract holds stgb's file, read and not yet started, with "
                                                       'stga'):
                self.written()
            with mock.patch.object(memory, 'contract', handing_off):
                lines = self.written()
        self.assertIn('import    stga  stage_b  stgb  stage_b  0x0c', lines)

    def test_binds_each_name_a_content_takes_as_its_record_gives_it(self):
        # stga's content takes main's main_count, in the view its word spells, and spec.py imports it there
        files = {'config/imports.txt': PROVIDER, **GENERATED,
                 'build/gen/stga/content.txt': 'dat_0c900104  0x0c900104  4  void *[]\n'
                                               'main_count  0x8c1755e0  -  taken: 1st_read main_count int\n'}
        units = {**UNITS, 'build/gcc/gen/stga/content.o': spec.Unit({'dat_0c900104'}, set(), {'_main_count'})}
        with self.game_tree(files, units):
            self.assertIn('import    stga  main_count  1st_read  main_count  0x8c', self.written())
        # where its record gives another image's, spec.py fails, naming both
        files['build/gen/stga/content.txt'] = files['build/gen/stga/content.txt'].replace('1st_read main_count',
                                                                                          'stgb main_count')
        with self.game_tree(files, units), \
                self.assertRaisesRegex(SystemExit, ": stga: its content takes main_count from stgb's main_count at "
                                                   "0x8c1755e0, as its record gives it, and the link binds it to "
                                                   "1st_read's main_count in the 0x8c view$"):
            self.written()
        # and where it gives main_count in the 0x0c view, though the tables give the 0x8c view
        files['build/gen/stga/content.txt'] = files['build/gen/stga/content.txt'].replace(
            'main_count  0x8c1755e0  -  taken: stgb', 'main_count  0x0c1755e0  -  taken: 1st_read')
        with self.game_tree(files, units), \
                self.assertRaisesRegex(SystemExit, ": stga: its content takes main_count from 1st_read's main_count at "
                                                   "0x0c1755e0, as its record gives it, and the link binds it to "
                                                   "1st_read's main_count in the 0x8c view$"):
            self.written()

    def test_fails_on_each_name_a_content_s_record_lists_and_the_content_doesn_t_take(self):
        # stga's record lists the word's name, and stage_flag as taken from stga itself, and the content takes neither
        record = GENERATED['build/gen/stga/content.txt'] + 'stage_flag  0x0c900010  -  taken: stga stage_flag int\n'
        units = {**UNITS, 'build/gcc/gen/stga/content.o': spec.Unit({'dat_0c900104'}, set(), set())}
        with self.game_tree({**GENERATED, 'build/gen/stga/content.txt': record}, units), \
                self.assertRaisesRegex(SystemExit, rf": stga: its content's record lists {re.escape(SPELLED)}, which "
                                                   r"the content doesn't take\n.*: stga: its content's record lists "
                                                   r"stage_flag, which the content doesn't take$"):
            self.written()

    def test_fails_on_a_record_that_leaves_out_or_repeats_a_row_or_puts_a_name_elsewhere(self):
        units = {**UNITS, 'build/gcc/gen/stga/content.o': spec.Unit({'dat_0c900104'}, set(), {'_main_count'})}
        record = 'dat_0c900104  0x0c900104  4  void *[]\nmain_count  0x8c1755e0  -  taken: 1st_read main_count int\n'
        where = re.escape('build/gen/stga/content.txt')
        for text, problem in (
                # the content takes main_count and defines its word. The first record lists neither, and the second
                # lists dat_0c900108 too, which the content doesn't define
                ('', ": stga: its content takes main_count, which its record doesn't list\n.*: stga: its content "
                     "defines dat_0c900104, which its record doesn't list"),
                (record + 'dat_0c900108  0x0c900108  4  int\n', ": stga: its content's record lists dat_0c900108, "
                                                                 "which the content doesn't define"),
                # then records that give a name twice, or where the tables put it elsewhere. The last two hold a name
                # that isn't a C name, and a provider that is no image
                (record + 'main_count  0x8c1755e0  -  taken: 1st_read main_count int\n',
                 rf': {where}:3: main_count has a row already, at {where}:2'),
                (record.replace('0x8c1755e0', '0x8c1755e4'),
                 rf": {where}:2: gives main_count at 0x8c1755e4, and config/stga/'s tables give it at 0x8c1755e0"),
                ('bad-name  0x0c900104  4  int\n' + record, rf": {where}:1: not a row of a content's record"),
                (record.replace('taken: 1st_read', 'taken: stgc'), rf": {where}:2: not a row of a content's record"),
                # a size that is no number, an address that isn't hex, a definition with no type after its size, a
                # name taken with too few fields, and an unresolved word with no reason
                (record.replace('  4  void', '  2x  void'), rf": {where}:1: not a row of a content's record"),
                (record.replace('0x0c900104', '0x0c90010g'), rf": {where}:1: not a row of a content's record"),
                ('dat_0c900104  0x0c900104  4\n' + record, rf": {where}:1: not a row of a content's record"),
                (record.replace('taken: 1st_read main_count int', 'taken: two'),
                 rf": {where}:2: not a row of a content's record"),
                (f'{SPELLED}  0x0c900200  -  unresolved:\n' + record, rf": {where}:1: not a row of a content's record")):
            with self.subTest(text), self.game_tree({'config/imports.txt': PROVIDER, **GENERATED,
                                                'build/gen/stga/content.txt': text}, units), \
                    self.assertRaisesRegex(SystemExit, problem + '$'):
                self.written()

    def test_holds_a_name_a_content_takes_to_the_symbol_its_record_gives_whichever_binding_takes_it(self):
        # stga's content takes stage_flag, which stga defines, and its record gives stga's stage_a
        units = {**UNITS, 'build/gcc/gen/stga/content.o': spec.Unit({'dat_0c900104'}, set(), {'_stage_flag'})}
        record = 'dat_0c900104  0x0c900104  4  void *[]\nstage_flag  0x0c900010  -  taken: stga stage_a int\n'
        with self.game_tree({'config/imports.txt': PROVIDER, **GENERATED, 'build/gen/stga/content.txt': record},
                            units), \
                self.assertRaisesRegex(SystemExit, ": stga: its content takes stage_flag from stga's stage_a at "
                                                   "0x0c900010, as its record gives it, and the link binds it to "
                                                   "stga's own stage_flag$"):
            self.written()
        # a second name a row gives binds as the record gives it
        units['build/gcc/gen/stga/content.o'] = spec.Unit({'dat_0c900104'}, set(), {'_stage_count'})
        record = 'dat_0c900104  0x0c900104  4  void *[]\nstage_count  0x0c900010  -  taken: stga stage_flag int\n'
        rows = PROVIDER + 'stga  stage_count  stga  stage_flag  -  the content counts by the flag\n'
        with self.game_tree({'config/imports.txt': rows, **GENERATED, 'build/gen/stga/content.txt': record}, units):
            self.assertIn('alias     stga  stage_count  stage_flag', self.written())

    def test_writes_no_spec_over_its_list_or_an_input_and_no_path_the_spec_can_t_give(self):
        with self.game_tree({'config/imports.txt': PROVIDER}):
            Path('spec.txt').write_text('a spec\n')
            Path('list.txt').symlink_to('spec.txt')
            for args in (['--list', 'spec.txt'], ['--list', './spec.txt'], ['--list', 'list.txt']):
                with self.subTest(args), self.assertRaisesRegex(SystemExit, ': SPEC and --list name the same file$'):
                    spec.main([*args, 'spec.txt'])
            self.assertEqual(Path('spec.txt').read_text(), 'a spec\n')
            with self.assertRaisesRegex(SystemExit, ': build/gcc/src/main.o: names build/gcc/src/main.o, an input of '
                                                    'the link$'):
                spec.main(['build/gcc/src/main.o'])
        units = {**UNITS, 'build/gcc/with space.o': spec.Unit(set(), set(), set())}
        with self.game_tree({'config/imports.txt': PROVIDER}, units), \
                self.assertRaisesRegex(SystemExit, ': build/gcc/with space.o: not a path the spec can give: letters, '
                                                   'digits, _, ., / and -, not starting with -$'):
            spec.main(['--add', '1st_read', 'build/gcc/with space.o', 'spec.txt'])

    def test_takes_no_import_from_an_image_whose_own_table_puts_the_name_elsewhere(self):
        # main's table gives stage_secret where stga's gives its static. stgb defines a global stage_secret, but its own
        # table puts it elsewhere, so stgb doesn't provide it, and nothing binds it
        files = {'config/imports.txt': PROVIDER,
                 'config/1st_read/symbols.txt': GAME_TREE['config/1st_read/symbols.txt'] + '0x8c900250  stage_secret\n',
                 'config/stga/symbols.txt': GAME_TREE['config/stga/symbols.txt'] + '0x0c900250  stage_secret\n',
                 'config/stgb/symbols.txt': GAME_TREE['config/stgb/symbols.txt'] + '0x0c900020  stage_secret\n'}
        units = {**UNITS, 'build/gcc/src/main.o': spec.Unit({'main', 'main_count'}, set(),
                                                            {'_main_count_alias', '_stage_flag', '_stage_secret'}),
                 'build/gcc/modules/stga/a.o': spec.Unit({'stage_a', 'stage_bss', 'stage_flag'}, {'stage_secret'},
                                                         set()),
                 'build/gcc/modules/stgb/b.o': spec.Unit({'stage_b', 'stage_flag', 'stage_secret'}, set(), set())}
        with self.game_tree(files, units), \
                self.assertRaisesRegex(SystemExit, r': 1st_read: 1 names nothing binds: static 1 \(stage_secret\)\n'):
            self.written()

    def test_takes_no_lone_provider_whose_span_doesn_t_hold_the_address(self):
        # as above, but stgb's table gives stage_secret no address. stgb's span ends at 0x0c900200, short of main's
        # address for it, so nothing binds it
        files = {'config/imports.txt': PROVIDER,
                 'config/1st_read/symbols.txt': GAME_TREE['config/1st_read/symbols.txt'] + '0x8c900250  stage_secret\n',
                 'config/stga/symbols.txt': GAME_TREE['config/stga/symbols.txt'] + '0x0c900250  stage_secret\n'}
        units = {**UNITS, 'build/gcc/src/main.o': spec.Unit({'main', 'main_count'}, set(),
                                                            {'_main_count_alias', '_stage_flag', '_stage_secret'}),
                 'build/gcc/modules/stga/a.o': spec.Unit({'stage_a', 'stage_bss', 'stage_flag'}, {'stage_secret'},
                                                         set()),
                 'build/gcc/modules/stgb/b.o': spec.Unit({'stage_b', 'stage_flag', 'stage_secret'}, set(), set())}
        with self.game_tree(files, units), \
                self.assertRaisesRegex(SystemExit, r': 1st_read: 1 names nothing binds: static 1 \(stage_secret\)\n'):
            self.written()

    def test_takes_of_two_providers_the_one_whose_span_holds_the_address(self):
        # main's tables give stage_flag in stga's bss, past stgb's span, and both stages define it
        files = {'config/imports.txt': ALIAS,
                 'config/1st_read/symbols.txt': GAME_TREE['config/1st_read/symbols.txt'].replace(
                     '0x8c900010  stage_flag', '0x8c900210  stage_flag'),
                 'config/stga/symbols.txt': '0x0c900210  stage_flag\n', 'config/stgb/symbols.txt': ''}
        with self.game_tree(files):
            self.assertIn('import    1st_read  stage_flag  stga  stage_flag  0x8c', self.written())
        # where no state holds stga live with main, nothing binds stage_flag. Neither main's span nor that of an image
        # live with it holds the address, and main's interval doesn't either, so it is nowhere
        contract = memory.contract

        def stga_never_live(known):
            terms = contract(known)
            return terms._replace(colive={**terms.colive, '1st_read': terms.colive['1st_read'] - {'stga'}})

        with self.game_tree(files), mock.patch.object(memory, 'contract', stga_never_live), \
                self.assertRaisesRegex(SystemExit, r': 1st_read: 1 names nothing binds: nowhere 1 \(stage_flag\)\n'):
            self.written()

    def test_links_an_image_s_content_where_its_gen_ranges_lie_among_its_objects(self):
        # stga's gen/ range follows a.c's row of D, and c.c's row follows the range
        files = {**GENERATED, 'modules/stga/c.c': '',
                 'config/stga/splits.txt': GENERATED['config/stga/splits.txt'] + 'modules/stga/c.c  D  0x0c900108  '
                                                                                 '0x0c90010c\n'}
        with self.game_tree(files):
            inputs = [str(path) for path in spec.inputs(spec.Config('stga'))]
        self.assertEqual(inputs, ['build/gcc/modules/stga/a.o', 'build/gcc/gen/stga/content.o',
                                  'build/gcc/modules/stga/c.o'])

    def test_links_each_object_an_add_gives_an_image_after_its_sources_objects(self):
        # main takes port_count, which no table gives and only an added object defines, so only an --add binds it.
        # An --add for no image config/ describes fails, and so does one of an object make gcc hasn't built
        units = {**UNITS, 'build/gcc/src/main.o': spec.Unit({'main', 'main_count'}, set(),
                                                             {'_main_count_alias', '_stage_flag', '_port_count'}),
                 'build/gcc/src/port.o': spec.Unit({'port_count'}, set(), set())}
        with self.game_tree({'config/imports.txt': PROVIDER}, units), \
                self.assertRaisesRegex(SystemExit, r': 1st_read: 1 names nothing binds: untabled 1 \(port_count\)\n'):
            self.written()
        with self.game_tree({'config/imports.txt': PROVIDER}, units):
            lines = self.written('--add', '1st_read', 'build/gcc/src/port.o')
        self.assertEqual([line.split()[2] for line in lines if line.split()[:2] == ['input', '1st_read']],
                         ['build/gcc/src/main.o', 'build/gcc/src/port.o', 'libgcc.a'])
        with self.game_tree({'config/imports.txt': PROVIDER}, units), \
                self.assertRaisesRegex(SystemExit, ': --add stgc build/gcc/src/port.o: no image config/ describes '
                                                   'stgc$'):
            self.written('--add', 'stgc', 'build/gcc/src/port.o')
        with self.game_tree({'config/imports.txt': PROVIDER}, units), \
                self.assertRaisesRegex(SystemExit, r": 1st_read: waits for 1 of its 2 objects, which make gcc didn't "
                                                   r'build: build/gcc/src/gone\.o$'):
            self.written('--add', '1st_read', 'build/gcc/src/gone.o')

    def test_writes_the_tail_rows_and_the_constants_the_memory_contract_gives(self):
        # stga closes with a.c's data, as ADVERTISE closes with DEND, and ev0001 takes stage_flag where the contract
        # anchors it
        contract = memory.contract

        def anchoring(known):
            terms = contract(known)
            return terms._replace(anchored={**terms.anchored, 'ev0001': {'stage_flag': 0x0C900010}})

        units = {**UNITS, 'build/gcc/events/ev0001/script.o': spec.Unit({'ev0001_count', 'ev0001_script'}, set(),
                                                                         {'_stage_flag'})}
        with self.game_tree({'config/imports.txt': PROVIDER}, units), \
                mock.patch.dict(memory.TAILS, {'stga': (('DEND', 'modules/stga/a.c', '.data'),)}), \
                mock.patch.object(memory, 'contract', anchoring):
            lines = self.written()
        self.assertIn('tail      stga  DEND  build/gcc/modules/stga/a.o  .data', lines)
        self.assertIn('constant  ev0001  stage_flag  0x0c900010', lines)

    def test_closes_the_image_of_the_fish_models_with_their_constants_and_their_data(self):
        # stga stands in for stg08, its fish.o for the fish models' object, which holds no code
        fish = elf.Elf('fish.o', [elf.Section(0, '', 0, 0, 0, 0, 0, b''),
                                  elf.Section(1, '.rodata', 1, elf.ALLOC, 0, 4, 4, b''),
                                  elf.Section(2, '.data', 1, elf.ALLOC | 1, 0, 8, 4, b''),
                                  elf.Section(3, '.comment', 1, 0, 0, 4, 1, b'')], [], {})
        with self.game_tree({'config/imports.txt': PROVIDER, 'build/gcc/modules/stga/fish.o': b''}), \
                mock.patch.multiple(memory, FISH_IMAGE='stga', FISH_MODELS='modules/stga/fish.c',
                                    ICECAP_C=Path('modules/stga/a.c')):
            read = elf.read
            with mock.patch.object(elf, 'read',
                                   lambda data, where: fish if str(where).endswith('fish.o') else read(data, where)):
                lines = self.written()
        self.assertIn('tail      stga  CFISH  build/gcc/modules/stga/fish.o  .rodata', lines)
        self.assertIn('tail      stga  DFISH  build/gcc/modules/stga/fish.o  .data', lines)

    def test_fails_on_a_row_for_every_image_that_binds_nothing_or_that_main_s_own_table_gives(self):
        # a row for every image binds nothing where no image takes its name, or where only main takes it, by a row of
        # its own. It fails too where config/1st_read/imports.txt gives 1ST_READ.BIN the name, as a second row for it
        for row in ('*  stage_none  stga  stage_flag  0x0c  no image takes it\n',
                    '*  stage_flag  stgb  stage_flag  0x8c  main takes it by its own row\n'):
            wanted = row.split()[1]
            with self.subTest(wanted), self.game_tree({'config/imports.txt': PROVIDER + row}), \
                    self.assertRaisesRegex(SystemExit, f'config/imports.txt:3: no image without a row of its own takes '
                                                       f'{wanted} from outside, so the row binds nothing$'):
                self.written()
        files = {'config/imports.txt': ALIAS + '*  stage_flag  stga  stage_flag  0x0c  the beach is the stage meant\n',
                 'config/1st_read/imports.txt': 'stage_flag  stga\n',
                 'config/1st_read/symbols.txt':
                     GAME_TREE['config/1st_read/symbols.txt'].replace('0x8c900010  stage_flag\n', '')}
        with self.game_tree(files), \
                self.assertRaisesRegex(SystemExit, 'config/imports.txt:2: 1st_read binds stage_flag already, at '
                                                   'config/1st_read/imports.txt:1$'):
            self.written()

    def test_takes_main_s_row_of_config_imports_txt_over_one_for_every_image_in_either_order(self):
        # main's row binds main's stage_flag to stga's, and the row for every image binds ev0001's to stgb's, whichever
        # comes first. A row for one image and a row for every image can give one name
        rows = ('1st_read  stage_flag  stga  stage_flag  0x8c  the beach is the stage main means\n',
                '*  stage_flag  stgb  stage_flag  0x8c  the other stage is the one meant\n')
        files = {'config/ev0001/symbols.txt': GAME_TREE['config/ev0001/symbols.txt'] + '0x8c900010  stage_flag\n'}
        units = {**UNITS, 'build/gcc/events/ev0001/script.o': spec.Unit({'ev0001_count', 'ev0001_script'}, set(),
                                                                         {'_stage_flag'})}
        for order in (rows, rows[::-1]):
            with self.subTest(first=order[0].split()[0]):
                with self.game_tree({**files, 'config/imports.txt': ALIAS + ''.join(order)}, units):
                    lines = self.written()
                self.assertIn('import    1st_read  stage_flag  stga  stage_flag  0x8c', lines)
                self.assertIn('import    ev0001  stage_flag  stgb  stage_flag  0x8c', lines)

    def test_fails_on_a_second_row_for_every_image_naming_the_one_before_it(self):
        # with main's row of config/imports.txt for the name ahead of the two, or none
        row = '*  stage_flag  stgb  stage_flag  0x8c  the other stage is the one meant\n'
        for before, at in ((ALIAS, 2), (PROVIDER, 3)):
            with self.subTest(main=before == PROVIDER), self.game_tree({'config/imports.txt': before + row * 2}), \
                    self.assertRaisesRegex(SystemExit, f'config/imports.txt:{at + 1}: \\* binds stage_flag already, at '
                                                       f'config/imports.txt:{at}$'):
                self.written()

    def test_fails_on_a_row_whose_provider_s_tables_put_its_symbol_elsewhere(self):
        # stga's table puts stage_a at 0x0c900000, and main's gives stage_flag 0x8c900010 and main_count_alias
        # 0x8c1755e0. A row that binds either name to a symbol at another address fails, though its provider defines
        # the symbol
        files = {'config/stga/symbols.txt': GAME_TREE['config/stga/symbols.txt'] + '0x0c900000  stage_a\n'}
        for row, problem in ((ALIAS + '1st_read  stage_flag  stga  stage_a  0x8c  the beach is the stage main means\n',
                              'config/imports.txt:2: config/1st_read/ gives stage_flag at 0x8c900010, and '
                              'config/stga/symbols.txt gives stage_a at 0x0c900000'),
                             (PROVIDER.replace(ALIAS, '1st_read  main_count_alias  1st_read  main  -  a second name\n'),
                              'config/imports.txt:1: config/1st_read/ gives main_count_alias at 0x8c1755e0, and '
                              'config/1st_read/symbols.txt gives main at 0x8c0101a0')):
            with self.subTest(row), self.game_tree({**files, 'config/imports.txt': row}), \
                    self.assertRaisesRegex(SystemExit, f': {problem}$'):
                self.written()

    def test_fails_on_a_row_for_an_image_config_doesn_t_describe(self):
        rows = PROVIDER + 'stgz  stage_flag  stga  stage_flag  0x0c  no such stage\n'
        with self.game_tree({'config/imports.txt': rows}), \
                self.assertRaisesRegex(SystemExit, 'config/imports.txt:3: not a row: IMAGE NAME PROVIDER SYMBOL VIEW '
                                                   'REASON, '):
            self.written()

    def test_fails_on_a_second_name_whose_symbol_the_image_doesn_t_define(self):
        with self.game_tree({'config/imports.txt':
                             '1st_read  main_count_alias  1st_read  main_none  -  a second name\n'}), \
                self.assertRaisesRegex(SystemExit, "config/imports.txt:1: 1st_read's objects don't define main_none$"):
            self.written()

    def test_classifies_a_name_in_an_object_s_row_interior_and_one_no_image_holds_nowhere(self):
        # stage_inner lies in a.c's row of stga's bss, where a.o defines no name, and stage_far in the motion area,
        # where no image lies, past main's interval
        files = {'config/imports.txt': PROVIDER,
                 'config/1st_read/symbols.txt': (GAME_TREE['config/1st_read/symbols.txt']
                                                 + '0x8c900204  stage_inner\n0x8cc00000  stage_far\n'),
                 'config/stga/splits.txt': (GAME_TREE['config/stga/splits.txt']
                                            + 'modules/stga/a.c  B  0x0c900200  0x0c900208\n')}
        units = {**UNITS, 'build/gcc/src/main.o': spec.Unit({'main', 'main_count'}, set(), {
            '_main_count_alias', '_stage_flag', '_stage_inner', '_stage_far'})}
        with self.game_tree(files, units), \
                self.assertRaisesRegex(SystemExit, r': 1st_read: 2 names nothing binds: interior 1 \(stage_inner\), '
                                                   r'nowhere 1 \(stage_far\)\n'):
            self.written()


class Reviewed(unittest.TestCase):
    def test_holds_each_import_row_for_one_image_to_the_memory_contract(self):
        # spec.py holds the rows to the memory contract only where make gcc links, so this holds the tree's. A row for
        # every image is left out: only the objects say which images it binds in. An alias imports nothing
        with cwd(ROOT):
            known = images()
            rows = spec.reviewed(known)
            terms = memory.contract(known)
        for (image, name), row in rows.items():
            if image != spec.EVERY and row.provider != image:
                with self.subTest(image=image, name=name):
                    self.assertTrue(spec.contract_holds(terms, image, row))


if __name__ == '__main__':
    unittest.main()
