import os
import unittest
from pathlib import Path

import deps
from support import MODULE, tree


class Images(unittest.TestCase):
    def test_names_each_image_config_describes_for_its_file(self):
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
    def test_writes_each_image_and_declaration_scan_rule(self):
        with tree(IMAGE):
            deps.main(['build/deps.mk', 'build/sources.txt'])
            self.assertEqual(Path('build/deps.mk').read_text(), (
                'build/1ST_READ.BIN: config/1st_read/splits.txt build/1st_read/src/main.bin\n'
                'build/SONICADV/MOD.BIN: config/mod/splits.txt build/mod/modules/mod/a.bin '
                'build/stub/mod/modules/mod/a.bin build/mod/modules/mod/b.bin build/mod/gen/mod/d_0c900140.bin\n'
                'build/gen/mod/declared.stamp: modules/mod/a.c config/mod/splits.txt config/mod/symbols.txt '
                'config/1st_read/symbols.txt\n'))

    def test_lists_sources_and_tables_and_rewrites_only_a_change(self):
        with tree(IMAGE):
            deps.main(['build/deps.mk', 'build/sources.txt'])
            self.assertEqual(Path('build/sources.txt').read_text().split(), [
                'modules/mod/a.c', 'src/main.c', 'config/1st_read/splits.txt', 'config/1st_read/symbols.txt',
                'config/mod/splits.txt', 'config/mod/symbols.txt'])
            os.utime('build/sources.txt', (0, 0))
            deps.main(['build/deps.mk', 'build/sources.txt'])
            self.assertEqual(os.stat('build/sources.txt').st_mtime, 0)
            Path('src/extra.c').write_text('')
            deps.main(['build/deps.mk', 'build/sources.txt'])
            self.assertIn('src/extra.c', Path('build/sources.txt').read_text().split())
            # an output that isn't UTF-8 is compared as bytes and rewritten
            written = Path('build/deps.mk').read_bytes()
            Path('build/deps.mk').write_bytes(b'\xff')
            deps.main(['build/deps.mk', 'build/sources.txt'])
            self.assertEqual(Path('build/deps.mk').read_bytes(), written)


if __name__ == '__main__':
    unittest.main()
