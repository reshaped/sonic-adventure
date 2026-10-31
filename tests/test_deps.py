import os
import unittest
from pathlib import Path

import deps
from support import MODULE, tree


class Images(unittest.TestCase):
    def test_names_each_binary_config_describes_for_its_file(self):
        # whatever SHA256SUMS lists
        files = {'config/1st_read/splits.txt': '', 'config/adv03/splits.txt': '', 'config/movie/splits.txt': '',
                 'config/README.md': '', 'SHA256SUMS': f'{"0" * 64}  orig/SONICADV/STG00.PRS\n'}
        with tree(files):
            self.assertEqual(deps.images(), [(Path('build/1ST_READ.BIN'), '1st_read'),
                                             (Path('build/SONICADV/ADV03.BIN'), 'adv03'),
                                             (Path('build/SONICADV/MOVIE.BIN'), 'movie')])


# MODULE, with a source that imports data and a gen/ range
IMAGE = dict(MODULE, **{
    'config/mod/splits.txt': MODULE['config/mod/splits.txt'] + 'gen/mod/d_0c900140.c  D  0x0c900140  0x0c900150\n',
    'modules/mod/a.c': '#include "mod/a/body.inc"\n',
})


class Main(unittest.TestCase):
    def test_writes_each_binary_rule(self):
        with tree(IMAGE):
            deps.main(['build/deps.mk'])
            self.assertEqual(Path('build/deps.mk').read_text(), (
                'build/1ST_READ.BIN: config/1st_read/splits.txt build/1st_read/src/main.bin\n'
                'build/SONICADV/MOD.BIN: config/mod/splits.txt build/mod/modules/mod/a.bin '
                'build/stub/mod/modules/mod/a.bin build/mod/modules/mod/b.bin build/mod/gen/mod/d_0c900140.bin\n'))

    def test_writes_no_rule_for_a_motion_file(self):
        # hero's one section, D, holds no code, so hero is a motion file
        files = dict(IMAGE, **{'config/hero/splits.txt': ('D  0x0cc00000  0x0cc00100\n'
                                                          'gen/hero/d_0cc00000.c  D  0x0cc00000  0x0cc00100\n'),
                               'config/hero/symbols.txt': ''})
        with tree(files):
            deps.main(['build/deps.mk'])
            self.assertNotIn('HERO', Path('build/deps.mk').read_text())

    def test_reads_1st_read_bin_s_imports_only_with_gcc(self):
        # a row that names no image, which only the --gcc run reads
        with tree(dict(IMAGE, **{'config/1st_read/imports.txt': 'a_init  none\n'})):
            deps.main(['build/deps.mk'])
            with self.assertRaisesRegex(SystemExit, r'config/1st_read/imports\.txt:1: none: no such image'):
                deps.main(['--gcc', 'build/gcc/deps.mk'])

    def test_rewrites_only_a_change(self):
        with tree(IMAGE):
            deps.main(['build/deps.mk'])
            os.utime('build/deps.mk', (0, 0))
            deps.main(['build/deps.mk'])
            self.assertEqual(os.stat('build/deps.mk').st_mtime, 0)
            # an output that isn't UTF-8 is compared as bytes and rewritten
            written = Path('build/deps.mk').read_bytes()
            Path('build/deps.mk').write_bytes(b'\xff')
            deps.main(['build/deps.mk'])
            self.assertEqual(Path('build/deps.mk').read_bytes(), written)


class Gcc(unittest.TestCase):
    def test_has_the_import_check_depend_on_each_import_s_object_once(self):
        # a.c has bodies, so the rule names its stub's object, once, for a_init and a_start. It names b.src's own
        # object for b_end, and none for d_value, which the gen/ range holds. Without imports, the rule names no
        # object.
        files = dict(IMAGE, **{
            'config/mod/symbols.txt': MODULE['config/mod/symbols.txt'] + '0x0c900020  b_end\n0x0c900140  d_value\n',
            'config/1st_read/imports.txt': 'a_init  mod\nb_end  mod\na_start  mod\nd_value  mod\n'})
        for case, given, rule in (
                ('imports', files, 'build/gcc/imports.ok: build/stub/modules/mod/a.obj build/modules/mod/b.obj\n'),
                ('none', IMAGE, 'build/gcc/imports.ok: \n')):
            with self.subTest(case), tree(given):
                deps.main(['--gcc', 'build/gcc/deps.mk'])
                self.assertEqual(Path('build/gcc/deps.mk').read_text(), rule)
                self.assertFalse(Path('build/deps.mk').exists())


if __name__ == '__main__':
    unittest.main()
