"""The PC readout runs with a controlled clock, flip count and drawing target."""
import os
import shlex
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


class Readout(unittest.TestCase):
    def test_pages_sampling_and_submission(self):
        root = Path(__file__).resolve().parents[1]
        cc = shlex.split(os.environ.get('CC', 'cc'))
        with tempfile.TemporaryDirectory() as directory:
            binary = Path(directory) / 'readout'
            subprocess.run([*cc, '-std=gnu17', '-DNONMATCHING', '-Dclock_gettime=readout_clock',
                            '-ffunction-sections', '-fdata-sections', '-Isrc', '-Iengine/native', '-Iengine/sa1',
                            '-Iengine/include', '-I.', '-idirafter', 'sdk/include',
                            'engine/tests/readout.c', 'engine/sa1/overlay.c', 'src/system/port.c',
                            '-Wl,-dead_strip' if sys.platform == 'darwin' else '-Wl,--gc-sections',
                            '-o', binary], cwd=root, check=True)
            subprocess.run([binary], check=True)
