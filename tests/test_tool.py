import os
import re
import unittest
from pathlib import Path

from support import tree
from tool import publish


class Publish(unittest.TestCase):
    def test_a_link_at_a_temporary_files_name_writes_nothing_through_it(self):
        # two links to keep.txt: out.c.tmp, a name publish() doesn't use, and
        # this process's temporary name, which publish() refuses
        temporary = f'out.c.{os.getpid()}.tmp'
        with tree({'out.c': 'old', 'keep.txt': 'kept'}):
            os.symlink('keep.txt', 'out.c.tmp')
            publish(Path('out.c'), 'new')
            self.assertEqual((Path('out.c').read_text(), Path('keep.txt').read_text()), ('new', 'kept'))
            os.symlink('keep.txt', temporary)
            with self.assertRaisesRegex(SystemExit, rf': {re.escape(temporary)}: File exists$'):
                publish(Path('out.c'), 'newer')
            self.assertEqual((Path('out.c').read_text(), Path('keep.txt').read_text()), ('new', 'kept'))
            self.assertTrue(Path(temporary).is_symlink())
            self.assertTrue(Path('out.c.tmp').is_symlink())

    def test_a_path_it_cant_write_fails_naming_it(self):
        with tree({'out/a': ''}):
            with self.assertRaisesRegex(SystemExit, r': out: Is a directory$'):
                publish(Path('out'), 'text')
            self.assertEqual(sorted(path.name for path in Path().iterdir()), ['out'])


if __name__ == '__main__':
    unittest.main()
