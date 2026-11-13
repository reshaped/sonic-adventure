import os
import re
import signal
import subprocess
import sys
import unittest
from pathlib import Path

from support import ROOT, tree
from tool import publish, run_child


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


class RunChild(unittest.TestCase):
    def test_a_term_while_popen_starts_the_child_kills_it(self):
        # a tool that takes TERM after Popen has started sleep and before Popen returns, a window that load on the host
        # can widen
        late = ('import os, signal, subprocess\n'
                'from pathlib import Path\n'
                'import tool\n'
                'class Late(subprocess.Popen):\n'
                '    def __init__(self, *args, **options):\n'
                '        super().__init__(*args, **options)\n'
                "        Path('child').write_text(str(self.pid))\n"
                '        os.kill(os.getpid(), signal.SIGTERM)\n'
                'subprocess.Popen = Late\n'
                'tool.stoppable()\n'
                "tool.run_child(['sleep', '30'])\n")
        with tree({}):
            stopped = subprocess.run([sys.executable, '-c', late], env=dict(os.environ, PYTHONPATH=str(ROOT / 'tools')))
            self.assertEqual(stopped.returncode, 143)
            with self.assertRaises(ProcessLookupError):
                os.kill(int(Path('child').read_text()), 0)

    def test_a_ctrl_c_while_popen_starts_the_child_kills_it(self):
        # as above, with ^C, which raises KeyboardInterrupt once Popen returns. The tool takes Python's own handler
        # first: a shell starts a background job with ^C ignored, and Python then leaves it ignored, so the suite
        # run in the background would send a ^C that nothing takes
        late = ('import os, signal, subprocess\n'
                'from pathlib import Path\n'
                'import tool\n'
                'signal.signal(signal.SIGINT, signal.default_int_handler)\n'
                'class Late(subprocess.Popen):\n'
                '    def __init__(self, *args, **options):\n'
                '        super().__init__(*args, **options)\n'
                "        Path('child').write_text(str(self.pid))\n"
                '        os.kill(os.getpid(), signal.SIGINT)\n'
                'subprocess.Popen = Late\n'
                'try:\n'
                "    tool.run_child(['sleep', '30'])\n"
                'except KeyboardInterrupt:\n'
                "    Path('interrupted').touch()\n")
        with tree({}):
            subprocess.run([sys.executable, '-c', late], env=dict(os.environ, PYTHONPATH=str(ROOT / 'tools')))
            self.assertTrue(Path('interrupted').exists())
            with self.assertRaises(ProcessLookupError):
                os.kill(int(Path('child').read_text()), 0)

    def test_the_child_starts_with_no_stop_blocked(self):
        # a child that sends itself TERM, which kills it only where TERM isn't blocked
        done = run_child([sys.executable, '-c', 'import os, signal\nos.kill(os.getpid(), signal.SIGTERM)'])
        self.assertEqual(done.returncode, -signal.SIGTERM)


if __name__ == '__main__':
    unittest.main()
