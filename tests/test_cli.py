import os
import struct
import subprocess
import sys
import unittest
from pathlib import Path

from support import MODULE, GAME_TREE, tree
from test_link import A_OBJ

ROOT = Path(__file__).resolve().parents[1]
OUTSIDE = "config/: not found; run tools from the repository's root"
# each tool with a command line, by name: the shortest command line it takes, which without its last word lacks an
# argument the tool requires, and the problem that command line fails with in an empty directory. The command line is
# empty for a tool that requires no argument
TOOLS = {
    'align': (['P', '32', 'in.src', 'out.src'], 'in.src: No such file or directory'),
    'census': ([], OUTSIDE),
    'content': (['build/gen/img/STG01.BIN', 'out.c'], 'config/stg01/splits.txt: No such file or directory'),
    'data': (['build/gen/img/STG01.BIN', 'gen/stg01/d_0c900000.c', 'out.c'],
             'config/stg01/splits.txt: No such file or directory'),
    'deps': (['build/deps.mk'], OUTSIDE),
    'disc': (['disc.gdi', 'manifest.txt', 'disc'], OUTSIDE),
    'floats': (['--assembly', 'a.src'], 'a.src: No such file or directory'),
    'graft': (['new.src', 'donor.src', 'out.src'], 'new.src: No such file or directory'),
    'imports': ([], OUTSIDE),
    'inline': (['a.c', 'out.c', 'text.src'], 'a.c: No such file or directory'),
    'ld': (['link.txt', 'out'], 'link.txt: No such file or directory'),
    'link': (['build/src/main.obj'], 'build/src/main.obj: no such object'),
    'progress': (['--patched', '--grafted', '--inline'], OUTSIDE),
    'publish': (['spec.txt', 'out', 'manifest.txt'], 'spec.txt: No such file or directory'),
    'relink': (['stg01'], OUTSIDE),
    'spec': (['spec.txt'], OUTSIDE),
    'splice': (['--check'], OUTSIDE),
    'sums': (['build/expected.txt'], 'build/expected.txt: No such file or directory'),
    'tags': (['src/main.c'], OUTSIDE),
    'vendor': (['--check'], 'tools/SHA256SUMS: No such file or directory'),
    'verify': (['--abi=-ml', 'spec.txt', 'link'], 'spec.txt: No such file or directory'),
}
# command lines argparse takes and the tool's own check refuses: each lacks an argument the check requires, or
# holds one it rules out
REFUSED = [('census', ['--gcc', '--disc']), ('census', ['--add', '1st_read', 'gcc/src/port.c']), ('data', []),
           ('data', ['--texts', 'a.c', 'text.inc', 'out.c']),
           ('floats', ['a.i']), ('floats', ['--shc', 'r10', '-fpu=single', 'a.i']),
           ('floats', ['--shc', 'r26', '-aggressive=2', 'a.i']),
           ('floats', ['--assembly', 'a.src', 'a.i']),
           ('progress', []), ('splice', ['--verbose', 'build/1ST_READ.BIN']),
           ('splice', ['--check', 'build/1ST_READ.BIN']),
           ('tags', ['--contract', 'f', 'a.c']),
           ('vendor', ['--check', 'shckit'])]
# tools/ld.py's spec of GAME_TREE's stga alone, as tools/spec.py opens one. tools/publish.py writes the image to
# build/gcc/SONICADV/STGA.BIN, which build/prskit compresses to STGA.PRS
STGA_SPEC = "# make gcc's images, as tools/spec.py writes them\nimage  stga  0x0c900000  0x00280000\n"
# the shortest final link tools/publish.py takes: an ELF whose sections are a symbol table and its names, which
# defines _image_end, absolute at stga's origin
NAMES, SYMBOLS = b'\0_image_end\0', bytes(16) + struct.pack('<IIIBBH', 1, 0x0C900000, 0, 0x10, 0, 0xFFF1)
FINAL = (b'\x7fELF\x01\x01'.ljust(32, b'\0') + struct.pack('<I', 96).ljust(14, b'\0') + struct.pack('<HHH', 40, 3, 1)
         + NAMES + SYMBOLS + bytes(40) + struct.pack('<10I', 0, 3, 0, 0, 52, len(NAMES), 0, 0, 1, 0)
         + struct.pack('<10I', 0, 2, 0, 0, 64, len(SYMBOLS), 1, 0, 4, 16))
# the toolchain the publish cases name by SH_ELF, whichever one the tests run with: its one cc1, which publish finds
# before it removes or writes a file. Each case fails before publish hashes the cc1, so it can be empty
SH_ELF = 'tools/sh-elf'
CC1 = {f'{SH_ELF}/libexec/gcc/sh-elf/16.2.0/cc1': ''}
# command lines that have a tool read, write or remove where it can't, or run what isn't there, by case: the tool, its
# arguments, the files the directory holds, the variables the environment sets, and the problem the tool fails with
UNABLE = {
    'an input that is a directory': (
        'inline', ['a.c', 'out.c', 'text.src'], {'a.c/a': ''}, {}, 'a.c: Is a directory'),
    'an output whose directory is missing': (
        'align', ['P', '32', 'in.src', 'none/out.src'], {'in.src': '        .SECTION P,CODE,ALIGN=4\n'}, {},
        'none/out.src: No such file or directory'),
    'an output that is a directory': (
        'splice', ['in.bin', 'out'], {'in.bin': '', 'out/a': ''}, {}, 'out: Is a directory'),
    "a file where an output's directory goes": (
        'deps', ['build/deps.mk'], dict(MODULE, build=''), {}, 'build: File exists'),
    'a wibo that is missing': (
        'link', ['build/modules/mod/a.obj', 'mod'], dict(MODULE, **{'build/modules/mod/a.obj': A_OBJ}),
        {'WIBO': 'tools/none'}, "tools/none: can't run it: No such file or directory"),
    'an old output that is a directory': (
        'ld', ['link.txt', 'out'], {'out/main/final.elf/a': ''}, {}, 'out/main/final.elf: Is a directory'),
    "a file where an old output's directory goes": (
        'publish', ['spec.txt', 'out', 'manifest.txt'],
        dict(GAME_TREE, **CC1, **{'spec.txt': STGA_SPEC, 'build/gcc/SONICADV': ''}), {'SH_ELF': SH_ELF},
        'build/gcc/SONICADV/STGA.BIN: Not a directory\npublish: build/gcc/SONICADV/STGA.PRS: Not a directory'),
    'a prskit that is missing': (
        'publish', ['spec.txt', 'out', 'manifest.txt'],
        dict(GAME_TREE, **CC1, **{'spec.txt': STGA_SPEC, 'out/stga/final.elf': FINAL}), {'SH_ELF': SH_ELF},
        "build/prskit: can't run it: No such file or directory"),
    # an empty PATH searches the working directory alone, which holds no git
    'no git on PATH': (
        'vendor', ['shckit', '1' * 40], {'tools/SHA256SUMS': f'# https://example.com/shckit {"0" * 40}\n'},
        {'PATH': ''}, "git: can't run it: No such file or directory"),
}


def run(tool, *args, files=None, env=None):
    """The status, stdout and stderr of tools/TOOL.py run with args in an empty directory, or one holding files, a
    {path: text or bytes} map, with env's variables set in its environment."""
    # there, a command line a tool wrongly takes can touch nothing in the checkout
    with tree(files or {}) as root:
        # from Python 3.14, argparse colors its usage where the environment asks, and PYTHON_COLORS=0 overrides that
        result = subprocess.run([sys.executable, '-B', str(ROOT / 'tools' / f'{tool}.py'), *args], cwd=root,
                                env=dict(os.environ, PYTHON_COLORS='0', **(env or {})), stdout=subprocess.PIPE,
                                stderr=subprocess.PIPE, text=True)
    return result.returncode, result.stdout, result.stderr


class Contract(unittest.TestCase):
    def test_covers_every_tool_with_a_command_line(self):
        found = {path.stem for path in (ROOT / 'tools').glob('*.py') if "__name__ == '__main__'" in path.read_text()}
        self.assertEqual(found, set(TOOLS))

    def test_prints_its_help_for_h(self):
        for tool in TOOLS:
            with self.subTest(tool=tool):
                status, out, err = run(tool, '-h')
                self.assertEqual((status, err), (0, ''))
                self.assertTrue(out.startswith(f'usage: {tool} '), out)

    def test_exits_2_on_an_unknown_option(self):
        for tool, (args, _) in TOOLS.items():
            with self.subTest(tool=tool):
                status, out, err = run(tool, *args, '--no-such-option')
                self.assertEqual((status, out), (2, ''))
                self.assertTrue(err.startswith(f'usage: {tool} '), err)
                self.assertEqual(err.splitlines()[-1], f'{tool}: error: unrecognized arguments: --no-such-option')

    def test_exits_2_on_a_missing_or_refused_argument(self):
        for tool, args in [*((tool, args[:-1]) for tool, (args, _) in TOOLS.items() if args), *REFUSED]:
            with self.subTest(tool=tool, args=args):
                status, out, err = run(tool, *args)
                self.assertEqual((status, out), (2, ''))
                self.assertTrue(err.startswith(f'usage: {tool} '), err)
                self.assertTrue(err.splitlines()[-1].startswith(f'{tool}: error: '), err)

    def test_exits_1_on_a_failure_naming_the_tool(self):
        for tool, (args, problem) in TOOLS.items():
            with self.subTest(tool=tool):
                self.assertEqual(run(tool, *args), (1, '', f'{tool}: {problem}\n'))

    def test_exits_1_naming_a_path_it_cant_use(self):
        for case, (tool, args, files, env, problem) in UNABLE.items():
            with self.subTest(case):
                self.assertEqual(run(tool, *args, files=files, env=env), (1, '', f'{tool}: {problem}\n'))


if __name__ == '__main__':
    unittest.main()
