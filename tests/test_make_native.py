"""The native slice's build, engine/mk/native.mk's rules and the PC backend's beside them, run for real in a small tree
with a stand-in for each compiler and pass. Each stand-in's output holds its command and what it read, so two trees
hold the same files where they were built from the same sources under the same settings, and only there."""

import contextlib
import hashlib
import os
import re
import shutil
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import support
from support import MAKE, MODULE, PINNED, SEARCHED, make, path_with_bin, settle
from test_make_dc import imported

ROOT = Path(__file__).resolve().parents[1]
NATIVE = 'build/engine/native'
PLAY = f'{NATIVE}/play'
TREE = f'{NATIVE}/src'
# a compiler: clang, the host's cc or its c++, by the name it runs under. It answers the three questions the rules ask
# clang, its version from the file beside it. Given -E, it writes the source with the text of each header an include
# of its names in place of the include, as it finds the header beside the source or in a directory the command
# searches, and leaves an include it finds no header for. Any other output is a script that prints itself: the
# command, then a sum of each file the command names, a C or C++ source's as -E gives it. A sum reads the checkout's
# own path as a dot, so two checkouts sum alike. Given -MD, it writes the rule of what it read, to -MF's file or
# beside the output, with a rule for each header as -MP gives one, and after the output, as clang does. Given -M, it
# writes the rule alone
COMPILER = f'#!{sys.executable} -S\n' + r'''import os, re, sys
name, args = os.path.basename(sys.argv[0]), sys.argv[1:]
if args == ['--version']:
    with open(sys.argv[0] + '.version') as file:
        sys.exit(print(file.read(), end=''))
if args == ['-print-resource-dir']:
    sys.exit(print('resource'))
if args[0].startswith('-print-file-name='):
    sys.exit(print(args[0].partition('=')[2]))
valued = ('-o', '-MF', '-MT', '-isystem', '-idirafter', '-include', '-Xclang', '-x')
given = {flag: args[at + 1] for at, flag in enumerate(args) if flag in valued}
searched = [word[2:] for word in args if word.startswith('-I')] + [
    args[at + 1] for at, flag in enumerate(args) if flag in ('-isystem', '-idirafter')]
files = [word for at, word in enumerate(args) if not word.startswith('-') and args[at - 1] not in valued
         and os.path.isfile(word)]
read = []


def text(path):
    def include(found):
        for directory in (os.path.dirname(path), *searched):
            if os.path.isfile(os.path.join(directory, found[1])):
                read.append(os.path.join(directory, found[1]))
                return text(read[-1])
        return found[0]
    with open(path) as file:
        return re.sub(r'(?m)^#include [<"](.*)[>"]$', include, file.read())


def summed(path):
    import hashlib
    with open(path, 'rb') as file:
        held = text(path).encode() if path.endswith(('.c', '.cpp')) else file.read()
    return hashlib.sha256(held.replace(os.getcwd().encode(), b'.')).hexdigest()


prelude = text(given['-include']) if '-include' in given else ''
if '-include' in given:
    read.insert(0, given['-include'])
line = f'{name} {" ".join(args)}'
if '-E' in args or '-M' in args:
    out = f'/* {line} */\n{prelude}{text(files[0])}'
else:
    out = '#!/bin/sh\ncat "$0"\n' + f'# {line}\n' + ''.join(f'# {path} {summed(path)}\n' for path in files)
if '-o' in given:
    with open(given['-o'], 'w') as file:
        file.write(out)
    os.chmod(given['-o'], 0o755)
if '-MD' in args or '-M' in args:
    with open(given['-MF'] if '-MF' in given else os.path.splitext(given['-o'])[0] + '.d', 'w') as file:
        file.write(f'{given["-MT"] if "-MT" in given else given["-o"]}: {" ".join((*files, *read))}\n' +
                   ''.join(f'{header}:\n' for header in read))
'''
# a pass over one file, as tools/floats.py, marks.py's mark and ninja, and refs.py are: it adds a line to the .i or
# .ll file its arguments name, MARK and its other arguments. marks.py's tree copies src/, modules/ and events/'s C
# and headers under its directory, each under a line of MARK, writes a file only where its text changes, as the
# slice's does, and removes none
PASS = r'''import os
import sys
MARK = 'a pass'
args = sys.argv[1:]


def put(path, text):
    if not os.path.isfile(path) or open(path).read() != text:
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'w') as file:
            file.write(text)


if args[0] == 'tree':
    for root in ('src', 'modules', 'events'):
        for directory, _, names in os.walk(root):
            for name in names:
                if name.endswith(('.c', '.h')):
                    path = os.path.join(directory, name)
                    put(os.path.join(args[1], path), f'/* {MARK} */\n' + open(path).read())
    put(os.path.join(args[1], 'overlaid.txt'), 'none\n')
else:
    read = next(arg for arg in args if arg.endswith(('.i', '.ll')))
    with open(read, 'a') as file:
        file.write(f'/* {MARK} {" ".join(arg for arg in sys.argv[1:] if arg != read)} */\n')
'''
# a writer of one file, as asm2c.py and stubs.py are: its first argument's file holds MARK, its other arguments and a
# sum of each that names a file. programs.py's writes each program's two lists, the link's aliases and its table
WRITER = r'''import hashlib
import os
import sys
MARK = 'a writer'


def summed(path):
    with open(path, 'rb') as file:
        return hashlib.sha256(file.read().replace(os.getcwd().encode(), b'.')).hexdigest()


out, rest = sys.argv[1], sys.argv[2:]
text = f'/* {MARK} {" ".join(rest)} */\n' + ''.join(
    f'/* {path} {summed(path.partition(":")[0])} */\n' for path in rest if os.path.isfile(path.partition(':')[0]))
if '--' in rest:
    for program in rest[:rest.index('--')]:
        with open(f'{out}/keep-{program}.txt', 'w') as file:
            file.write('native_none\n')
        open(f'{out}/rename-{program}.txt', 'w').close()
        text += f'/* {summed(f"{out}/program-{program}.r")} */\n'
    open(f'{out}/aliases.txt', 'w').close()
    out = f'{out}/programs.c'
with open(out, 'w') as file:
    file.write(text)
'''


def marked(script, mark):
    """script, PASS or WRITER, with mark for its MARK."""
    return re.sub(r"(?m)^MARK = '.*'$", f"MARK = '{mark}'", script)


# an objcopy, which copies the file its last two arguments name under its other arguments
OBJCOPY = ('#!/bin/sh\nfor last; do :; done\nout=$last\nwords=\nwhile [ $# -gt 2 ]; do words="$words $1"; shift; done\n'
           '{ echo "# objcopy$words"; cat "$1"; } > "$out"\n')
# a tools/memory.py whose contract anchors one name, in mod, and which reads one source, loader.h
MEMORY = ("SOURCES = ('src/system/loader.h',)\n\n\nclass Contract:\n"
          "    anchored = {'mod': {'mod_anchor': 0x0c900100}}\n\n\ndef contract(images):\n    return Contract\n")
# the fragment as it stands, but for its three lists of the sources the slice takes from the SDK and from the game's
# assembly, each cut to a source or two, so the tree builds in a second or two
LISTS = {'NATIVE_NINJA': 'njSin', 'NATIVE_ASM': 'modules/mod/b.src:b $(NATIVE_NINJA:%=sdk/ninja/%.c)',
         'NATIVE_SDK_C': '$(NATIVE_NINJA:%=sdk/ninja/%.c)'}
NATIVE_MK = (ROOT / 'engine/mk/native.mk').read_text()
for name, sources in LISTS.items():
    NATIVE_MK, cut = re.subn(rf'(?m)^({name} +:=) (?:.*\\\n)*.*$', lambda found: f'{found[1]} {sources}', NATIVE_MK)
    assert cut == 1, name
# the PC backend's sources: the last two are SDL3's window and pads, and the one before them the window of a host
# without it
BACKEND = ('backend', 'storage', 'scene', 'world', 'raster', 'disc', 'pvr', 'ninja', 'png', 'sound', 'dsp',
           'window_none', 'window', 'input')
# the compilers' stand-ins, by the names the tests run them under
COMPILERS = ('clang', 'cc', 'other-cc', 'c++', 'other-c++')
# the tree: the engine's rules; a game of two sources, main.c in 1ST_READ.BIN and a.c in a program, mod, with a header
# each, and shared.h in both their directories, which main.c takes from its own; one source each of Ninja's, the C
# runtime's, the shim's and the adapter's, the last of which reads Ninja's header; play math, the PC backend and the
# front end, each a line; and each stand-in. The host has no SDL3
SLICE = {
    **PINNED, **SEARCHED, **MODULE,
    **{path: (ROOT / path).read_text() for path in ('engine/engine.mk', 'engine/mk/owner.mk', 'engine/mk/drawers.mk',
                                                    'engine/mk/play_math.mk', 'engine/mk/replay.mk')},
    'engine/mk/native.mk': NATIVE_MK,
    'config/1st_read/splits.txt': ('src/main.c  P  0x8c0101a0  0x8c0101b0\n'
                                   'sdk/ninja/njSin.c  P  0x8c0101b0  0x8c0101c0\n'
                                   'sdk/runtime/rand.c  P  0x8c0101c0  0x8c0101d0\n'),
    **{f'config/{image}/splits.txt': '' for image in ('advertise', 'b_robo', 'b_e101_r', 'loader')},
    'config/imports.txt': '', 'config/1st_read/imports.txt': '',
    'src/main.c': '#include "main.h"\n#include "shared.h"\nint main_word;\n',
    'src/main.h': '/* main.h */\n', 'src/shared.h': '/* src/shared.h */\n',
    'modules/shared.h': '/* modules/shared.h */\n', 'src/unread.h': '/* unread.h */\n',
    'src/system/loader.h': '/* loader.h */\n',
    'modules/mod/a.c': '#include "mod/a.h"\nint a_word;\n', 'modules/mod/a.h': '/* a.h */\n',
    'sdk/include/ninja.h': '/* ninja.h */\n', 'sdk/ninja/njSin.c': '#include <ninja.h>\nint sine;\n',
    'sdk/runtime/rand.c': 'int seed;\n',
    'engine/dc/dreamcast.h': '/* dreamcast.h */\n',
    'engine/native/prelude.h': '#include "intrinsics.h"\n', 'engine/native/intrinsics.h': '/* intrinsics.h */\n',
    'engine/native/intrinsics_machine.h': '', 'engine/native/sh4.h': '/* sh4.h */\n',
    'engine/native/native.h': '/* native.h */\n',
    'engine/native/runner.c': '#include "native.h"\n#include "sa_sa1.h"\nint runner;\n',
    'engine/sa1/sa_sa1.h': '/* sa_sa1.h */\n', 'engine/sa1/sa1.h': '#include <ninja.h>\n',
    'engine/sa1/records.c': '#include "sa1.h"\nint records;\n',
    'engine/sa1/play.c': '#include "sa_sa1.h"\nint play;\n',
    **{f'engine/sa1/{name}.h': '' for name in ('sa_capture', 'sa_client', 'client', 'basic', 'rules')},
    'engine/include/sa_engine.h': '', 'engine/include/sa_pc.h': '',
    'engine/runtime/runtime.h': '', 'engine/runtime/engine.cpp': 'int engine;\n',
    **{f'engine/pc/{name}.cpp': f'int {name};\n' for name in BACKEND},
    'engine/sa1/client.cpp': 'int client;\n', 'engine/sa1/basic.cpp': 'int basic;\n',
    'engine/numeric/play_math.h': '', 'engine/numeric/sh4_fpu_approx.h': '',
    **{f'engine/numeric/{name}.c': f'int {name};\n' for name in ('math', 'fast', 'sine')},
    'tools/floats.py': marked(PASS, 'floats'), 'tools/memory.py': MEMORY,
    'engine/tools/marks.py': marked(PASS, 'marks'), 'engine/tools/refs.py': marked(PASS, 'refs'),
    'engine/tools/asm2c.py': marked(WRITER, 'asm2c'), 'engine/tools/programs.py': marked(WRITER, 'programs'),
    'engine/tools/stubs.py': marked(WRITER, 'stubs'), 'engine/native/stubs.txt': '', 'engine/tools/coff.py': '',
    **{f'bin/{name}': COMPILER for name in COMPILERS},
    **{f'bin/{name}.version': f'{name} version 1\n' for name in COMPILERS},
    'bin/readelf': '#!/bin/sh\n', 'bin/nm': '#!/bin/sh\n', 'bin/objcopy': OBJCOPY,
    'bin/pkg-config': '#!/bin/sh\nexit 1\n',
}
# the source each compile of the slice's clang names in its status line. The game's C, Ninja's and the C runtime's pass
# through the float pass, marks.py and refs.py
PASSED = ['modules/mod/a.c', 'sdk/ninja/njSin.c', 'sdk/runtime/rand.c', 'src/main.c']
# the shim's C, the adapter's, the front end's, play math's and asm2c.py's are the host's
HOSTED = [f'{NATIVE}/asm.c', 'engine/native/runner.c', 'engine/numeric/fast.c', 'engine/sa1/play.c',
          'engine/sa1/records.c']
EVERY = sorted(PASSED + HOSTED)
# the settings the slice takes from the environment, each with another value than make's own
SETTINGS = {'NATIVE_OPT': '-O0', 'NATIVE_CHECK': '--check', 'NATIVE_DOMAIN': '0x100000000'}


@contextlib.contextmanager
def sliced(files=None, gone=()):
    """A checkout of SLICE and files, without each path of gone, in which each file whose text opens with #! is
    executable."""
    files = {**SLICE, **(files or {})}
    with support.checkout(files) as root:
        for path, text in files.items():
            if isinstance(text, str) and text.startswith('#!'):
                (root / path).chmod(0o755)
        for path in gone:
            (root / path).unlink()
        yield root


def play(root, *args, goal=PLAY, **settings):
    """make's status and output for goal, the game unless another is given, in root, with eight jobs, its bin/ first
    in PATH and c++ for the host's C++ compiler. The slice builds in NATIVE whatever its settings. The environment
    holds settings and no other variable named NATIVE_ and more,
    which engine/mk/native.mk's rules take from it."""
    env = {name: value for name, value in os.environ.items() if not re.match(r'NATIVE(_|\Z)', name)}
    with mock.patch.dict(os.environ, {**env, 'CXX': 'c++', 'NATIVE': NATIVE, **settings}, clear=True):
        return make(root, '-j8', *args, goal, path=path_with_bin(root))


def held(root):
    """Each file under root's build/engine/ and its text, by its path, with root's own path written as a dot."""
    return {path.relative_to(root).as_posix(): path.read_text().replace(str(root), '.')
            for path in sorted((root / 'build/engine').rglob('*')) if path.is_file()}


def compiled(output):
    """The source each status line of output names for a compile by the slice's clang, in the order of their
    paths."""
    return sorted(re.findall(r'(?m)^  native (\S+)$', output))


def current(*goals):
    """What make says of goals as it builds nothing: that each is up to date."""
    return ''.join(f"{os.path.basename(MAKE)}: '{goal}' is up to date.\n" for goal in goals)


def edit(root, path, old, new):
    """Write path in root with new in old's place, which its text holds once, and give its text."""
    text = (root / path).read_text()
    assert text.count(old) == 1, (path, old)
    (root / path).write_text(text.replace(old, new))
    return text.replace(old, new)


def closure(script):
    """script, one of engine/tools/'s, and each module of engine/tools/ or tools/ it imports, directly or through
    another, as its path."""
    names = re.findall(r'(?m)^(?:from|import) (\w+)', (ROOT / script).read_text())
    beside = {f'engine/tools/{name}.py' for name in names if (ROOT / f'engine/tools/{name}.py').is_file()}
    return {script, *beside, *imported([name for name in names if (ROOT / f'tools/{name}.py').is_file()])}


class Slice(unittest.TestCase):
    maxDiff = None

    @classmethod
    def setUpClass(cls):
        # the game builds once, in one checkout, which each test takes as that build left it: an object holds the
        # checkout's path, so a copy elsewhere would hold another build's
        stack = contextlib.ExitStack()
        cls.addClassCleanup(stack.close)
        cls.root = stack.enter_context(sliced())
        status, output = play(cls.root)
        assert status == 0, output
        # a second make, over each file as the first dated it, builds nothing: no file the first wrote is older
        # than one it was written from
        status, output = play(cls.root)
        assert (status, output) == (0, current(PLAY)), output
        settle(cls.root)
        cls.kept = Path(stack.enter_context(tempfile.TemporaryDirectory()), 'kept')
        shutil.copytree(cls.root, cls.kept)
        cls.base = held(cls.root)

    def built(self):
        """The checkout as the game's first build and settle() left it."""
        for path in self.root.iterdir():
            shutil.rmtree(path) if path.is_dir() and not path.is_symlink() else path.unlink()
        shutil.copytree(self.kept, self.root, dirs_exist_ok=True)
        return self.root

    def fresh(self, files=None, args=(), gone=(), **settings):
        """What a build of the game from nothing holds, in a checkout of SLICE and files without gone, under args and
        settings."""
        with sliced(files, gone) as root:
            status, output = play(root, *args, **settings)
            self.assertEqual(status, 0, output)
            return held(root)

    def again(self, root, *args, **settings):
        """The sources make compiles as it builds the game in root again, under args and settings, where the next
        make, over each file as that one dated it, builds nothing. settle() then dates what root holds, so the next
        change to a file of it is newer than what make built."""
        status, output = play(root, *args, **settings)
        self.assertEqual(status, 0, output)
        self.assertEqual(play(root, *args, **settings), (0, current(PLAY)))
        settle(root)
        return compiled(output)

    def test_builds_the_game_and_then_nothing(self):
        root = self.built()
        self.assertTrue((root / PLAY).is_file())
        self.assertEqual(play(root), (0, current(PLAY)))
        self.assertEqual(held(root), self.base)

    def test_builds_again_under_each_setting_of_the_environment_and_again_as_each_is_set_back(self):
        # each setting changes a command of the slice's, and the one request holds them all, so a change to one
        # compiles every source of the slice's again. A tree built under one request and then under another holds
        # what a build from nothing under the second holds: under all four here, and under none again, where it holds
        # what its first build did
        root, settings = self.built(), {}
        for setting, value in SETTINGS.items():
            with self.subTest(setting=setting):
                settings[setting] = value
                self.assertEqual(self.again(root, **settings), EVERY)
        self.assertEqual(held(root), self.fresh(**settings))
        self.assertEqual(self.again(root, **settings), [])
        for setting in SETTINGS:
            with self.subTest(unset=setting):
                del settings[setting]
                self.assertEqual(self.again(root, **settings), EVERY)
        self.assertEqual(held(root), self.base)

    def test_builds_again_with_another_compiler_or_another_release_of_one_and_again_with_the_first(self):
        # the slice's compiler is clang, by the version it gives. The PC backend's is the environment's CXX, whose
        # objects print no status. CC compiles the host's reference of play math, which writes the fast path's
        # table, so the slice's fast path compiles again with it
        root = self.built()
        (root / 'bin/clang.version').write_text('clang version 2\n')
        self.assertEqual(self.again(root), EVERY)
        self.assertEqual(self.again(root, CXX='other-c++'), [])
        self.assertEqual(self.again(root, 'CC=other-cc', CXX='other-c++'), ['engine/numeric/fast.c'])
        self.assertEqual(held(root), self.fresh({'bin/clang.version': 'clang version 2\n'}, ('CC=other-cc',),
                                                CXX='other-c++'))
        (root / 'bin/clang.version').write_text(SLICE['bin/clang.version'])
        self.assertEqual(self.again(root, 'CC=other-cc', CXX='other-c++'), EVERY)
        self.assertEqual(self.again(root, 'CC=other-cc'), [])
        self.assertEqual(self.again(root), ['engine/numeric/fast.c'])
        self.assertEqual(held(root), self.base)

    def test_compiles_the_front_door_s_two_objects_again_once_the_disc_lies_elsewhere_and_nothing_else(self):
        # the runner holds the directory orig/1ST_READ.BIN lies in, through a link where orig/ is one, and the front
        # end the directory of the build's images. One record holds both paths
        root = self.built()
        (root / 'elsewhere').mkdir()
        (root / 'elsewhere/1ST_READ.BIN').write_text('')
        shutil.rmtree(root / 'orig')
        (root / 'orig').symlink_to('elsewhere')
        self.assertEqual(self.again(root), ['engine/native/runner.c', 'engine/sa1/play.c'])
        self.assertIn('-DNATIVE_BUILD_DISC="./elsewhere"', held(root)[f'{NATIVE}/obj/engine/native/runner.ll'])

    def test_builds_again_what_a_pass_or_a_writer_gives_once_its_script_or_a_module_it_imports_changes(self):
        # the float pass, marks.py and refs.py read each source of the game's C, and marks.py and refs.py the
        # adapter's. refs.py reads the shim's too, and the front end's passes through none. tools/float_value.py is
        # one of the modules the float pass imports. Each writer's file reaches the link. stubs.py imports
        # tools/tables.py through tools/memory.py, as the anchors' command does: both files stand empty here before
        # that module changes, and make writes both again
        root, files = self.built(), {}
        for script, sources in (('tools/floats.py', PASSED), ('tools/float_value.py', PASSED),
                                ('engine/tools/marks.py', sorted(PASSED + ['engine/sa1/records.c'])),
                                ('engine/tools/refs.py',
                                 sorted(PASSED + ['engine/native/runner.c', 'engine/sa1/records.c'])),
                                ('engine/tools/asm2c.py', [f'{NATIVE}/asm.c']), ('engine/tools/programs.py', []),
                                ('tools/tables.py', []), ('engine/tools/stubs.py', [])):
            with self.subTest(script=script):
                written = {path: (root / path).read_text() for path in (f'{NATIVE}/stubs.c', f'{NATIVE}/anchors.txt')}
                if script == 'tools/tables.py':
                    for path in written:
                        (root / path).write_text('')
                    settle(root)
                if 'MARK' in SLICE.get(script, ''):
                    mark = re.search(r"(?m)^MARK = '(\w+)'$", SLICE[script])[1]
                    files[script] = edit(root, script, f"MARK = '{mark}'", f"MARK = '{mark} two'")
                else:
                    with open(root / script, 'a') as file:
                        file.write('\n# another\n')
                self.assertEqual(self.again(root), sources)
                if script == 'tools/tables.py':
                    self.assertEqual({path: (root / path).read_text() for path in written}, written)
        self.assertEqual(held(root), self.fresh(files))

    def test_each_compile_and_each_writer_waits_on_its_scripts_and_every_module_they_import(self):
        # by make's database for the game, once it is built: the float pass as the repository holds it, with each
        # module of tools/ it imports, directly or through another; stubs.py with coff.py and tools/memory.py and
        # theirs; and the anchors' command, which imports tools/images.py and tools/memory.py, and reads the tables
        # and the sources tools/memory.py lists
        root = self.built()
        status, database = play(root, '-pn')
        self.assertEqual(status, 0, database[:3000])

        def needs(target):
            return set(re.search(rf'(?m)^{re.escape(target)}: (?!(?:private )?\w+ [:+]?=)(.*)$', database)[1].split())

        floats = imported(['floats'])
        self.assertLessEqual({'tools/float_value.py', 'tools/layout.py', 'tools/csource.py'}, floats)
        makefiles = {'engine/engine.mk', 'engine/mk/play_math.mk', 'engine/mk/native.mk', f'{NATIVE}/request.txt'}
        passes = {*floats, *makefiles, 'engine/tools/marks.py', 'engine/tools/refs.py', 'config/compilers.mk',
                  f'{NATIVE}/headers.txt', f'{TREE}/overlaid.txt'}
        for source in PASSED:
            with self.subTest(source=source):
                self.assertEqual(passes - needs(f'{NATIVE}/obj/{source[:-2]}.o'), set())
        for obj, scripts in (('obj/engine/native/runner.o', {'engine/tools/refs.py', f'{NATIVE}/paths.txt'}),
                             ('obj/engine/sa1/records.o', {'engine/tools/marks.py', 'engine/tools/refs.py'}),
                             ('obj/engine/sa1/play.o', {f'{NATIVE}/paths.txt'}), ('obj/engine/numeric/fast.o', set()),
                             ('asm.o', set()), ('programs.o', set()), ('stubs.o', set())):
            with self.subTest(obj=obj):
                self.assertEqual((makefiles | scripts) - needs(f'{NATIVE}/{obj}'), set())
        stubs = closure('engine/tools/stubs.py')
        self.assertLessEqual({'engine/tools/coff.py', 'tools/memory.py', 'tools/tables.py'}, stubs)
        self.assertEqual(stubs - needs(f'{NATIVE}/stubs.c'), set())
        self.assertEqual((imported(['images', 'memory']) | {'src/system/loader.h', 'config/mod/splits.txt'}) -
                         needs(f'{NATIVE}/anchors.txt'), set())
        for linked in ('program-mod.r', 'programs.c', 'stubs.c', 'play'):
            with self.subTest(linked=linked):
                self.assertEqual(({f'{NATIVE}/objects.txt'} | makefiles) - needs(f'{NATIVE}/{linked}'), set())

    def test_builds_again_what_a_make_file_s_setting_changes(self):
        # config/compilers.mk gives the float pass SHC's flags for the game's C, engine/engine.mk the directory of the
        # engine's headers, which the host's C searches, and play math's fragment the directory of its header
        root, files = self.built(), {}
        for path, old, new, sources in (
                ('config/compilers.mk', 'GAME := -cpu=sh4\n', 'GAME := -cpu=sh4 -fpu=single\n',
                 ['modules/mod/a.c', 'src/main.c']),
                ('engine/engine.mk', 'ENGINE_INCLUDE  := engine/include\n', 'ENGINE_INCLUDE  := engine/sa1\n', HOSTED),
                ('engine/mk/play_math.mk', 'PLAY_MATH_INCLUDE := -Iengine/numeric\n',
                 'PLAY_MATH_INCLUDE := -Iengine/numeric -Iengine/sa1\n', HOSTED)):
            with self.subTest(path=path):
                files[path] = edit(root, path, old, new)
                self.assertLessEqual(set(sources), set(self.again(root)))
        self.assertEqual(held(root), self.fresh(files))

    def test_the_float_pass_reads_each_source_under_the_release_and_the_flags_shc_builds_it_with(self):
        # the game's C is R32's under GAME, and the SDK's R10's under SDK. a patched source is R10's under SDK though it
        # is the game's, and a member of Kamui R32's under GAME though it is the SDK's. own() gives the slice's object
        # of a source the flag it gives the Dreamcast object, and a line that gives a Dreamcast object -double=float
        # gives it to the slice's. a double is a float under -double=float, as under -fpu=single, and only R26 and
        # R32 fuse a product into a sum
        own = ''.join(line for line in (ROOT / 'config/compilers.mk').read_text().splitlines(True)
                      if line.startswith('own ='))
        files = {'config/compilers.mk': 'GAME := -cpu=sh4 -fpu=single\nSDK := -cpu=sh4\nPATCHED_C := src/main.c\n'
                                        'KAMUI_C := sdk/ninja/njSin.c\n' + own +
                                        '$(call own,modules/mod/a,-aggressive=2)\n'
                                        'build/src/main.obj build/dc/src/main.o: SDK += -double=float -size\n'}
        with sliced(files) as root:
            status, output = play(root)
            self.assertEqual(status, 0, output)
            for source, told, contract in (('modules/mod/a', '--shc r32 -fpu=single -aggressive=2', 'on'),
                                           ('src/main', '--shc r10 -double=float', 'off'),
                                           ('sdk/ninja/njSin', '--shc r32 -fpu=single', 'on'),
                                           ('sdk/runtime/rand', '--shc r32 -fpu=single', 'on')):
                with self.subTest(source=source):
                    self.assertIn(f'/* floats {told} */\n', (root / f'{NATIVE}/obj/{source}.i').read_text())
                    self.assertEqual(re.findall(r'-ffp-contract=\w+|-Ddouble=float',
                                                (root / f'{NATIVE}/obj/{source}.o').read_text())[:2],
                                     [f'-ffp-contract={contract}', '-Ddouble=float'])

    def test_compiles_again_each_source_that_includes_a_header_once_it_changes_and_no_other(self):
        # the game's C reads its headers' copies, Ninja's source and the adapter's read Ninja's header, which holds
        # the layouts the adapter walks, and the shim and the front end read SA1's. Each compile of the game's C and
        # the adapter's reads the prelude's
        root, files = self.built(), {}
        for header, sources in (('src/main.h', ['src/main.c']), ('modules/mod/a.h', ['modules/mod/a.c']),
                                ('sdk/include/ninja.h', ['engine/sa1/records.c', 'sdk/ninja/njSin.c']),
                                ('engine/sa1/sa1.h', ['engine/sa1/records.c']),
                                ('engine/native/native.h', ['engine/native/runner.c']),
                                ('engine/sa1/sa_sa1.h', ['engine/native/runner.c', 'engine/sa1/play.c']),
                                ('engine/native/intrinsics.h', sorted(PASSED + ['engine/sa1/records.c']))):
            with self.subTest(header=header):
                files[header] = f'/* another {header} */\n'
                (root / header).write_text(files[header])
                self.assertEqual(self.again(root), sources)
        self.assertEqual(held(root), self.fresh(files))

    def test_compiles_a_source_again_whose_dependency_file_is_gone(self):
        rules = {'src/main.c': 'obj/src/main.d', 'engine/native/runner.c': 'obj/engine/native/runner.d',
                 'engine/sa1/records.c': 'obj/engine/sa1/records.d', 'engine/sa1/play.c': 'obj/engine/sa1/play.d',
                 f'{NATIVE}/asm.c': 'asm.d'}
        root = self.built()
        for rule in rules.values():
            (root / NATIVE / rule).unlink()
        self.assertEqual(self.again(root), sorted(rules))
        self.assertEqual(held(root), self.base)

    def test_compiles_a_source_again_once_a_header_joins_a_directory_it_searches_first(self):
        # a.c names its header mod/a.h, which it finds under modules/, searched after src/
        added = {'src/mod/a.h': '/* another a.h */\n'}
        root = self.built()
        (root / 'src/mod').mkdir()
        (root / 'src/mod/a.h').write_text(added['src/mod/a.h'])
        self.assertIn('modules/mod/a.c', self.again(root))
        self.assertEqual(held(root), self.fresh(added))

    def test_removes_the_copy_of_a_source_that_left_the_tree_and_compiles_what_read_it_again(self):
        # main.c takes shared.h from its own directory, src/, and from modules/ once src/ has none. A copy that
        # stayed under the slice's tree would still be the first one found. unread.h is a header nothing includes,
        # and main.h becomes first.h
        root = self.built()
        for path in ('src/shared.h', 'src/unread.h'):
            (root / path).unlink()
        (root / 'src/main.h').rename(root / 'src/first.h')
        files = {'src/main.c': edit(root, 'src/main.c', '"main.h"', '"first.h"'), 'src/first.h': SLICE['src/main.h']}
        self.assertIn('src/main.c', self.again(root))
        after = held(root)
        self.assertEqual(sorted(path for path in after if path.startswith(f'{TREE}/')),
                         [f'{TREE}/{path}' for path in ('modules/mod/a.c', 'modules/mod/a.h', 'modules/shared.h',
                                                        'overlaid.txt', 'src/first.h', 'src/main.c',
                                                        'src/system/loader.h')])
        self.assertEqual(after, self.fresh(files, gone=('src/shared.h', 'src/unread.h', 'src/main.h')))

    def test_links_a_program_again_once_a_source_leaves_it(self):
        # a.c and its header leave the tree and mod's table. What the slice built of a.c stays under its obj/, where
        # no link names it
        root, gone = self.built(), ('modules/mod/a.c', 'modules/mod/a.h')
        for path in gone:
            (root / path).unlink()
        rows = ''.join(line for line in MODULE['config/mod/splits.txt'].splitlines(True) if 'a.c' not in line)
        (root / 'config/mod/splits.txt').write_text(rows)
        self.again(root)
        after, fresh = held(root), self.fresh({'config/mod/splits.txt': rows}, gone=gone)
        left = {f'{NATIVE}/obj/modules/mod/a.{suffix}' for suffix in ('d', 'i', 'll', 'o')}
        self.assertEqual(set(after) - set(fresh), left)
        self.assertEqual({path: text for path, text in after.items() if path not in left}, fresh)

    def test_links_the_game_again_once_the_host_gains_sdl3_and_again_once_it_loses_it(self):
        # with SDL3 the backend's window and pads are SDL3's, and without it the window is none. Each object of the
        # other build stays, and the link names none of them. No source of the slice's compiles again
        sdl = {'bin/pkg-config': '#!/bin/sh\ncase $1 in --libs) echo -lSDL3;; --cflags) echo -Isdl3;; esac\n'}
        root = self.built()
        (root / 'bin/pkg-config').write_text(sdl['bin/pkg-config'])
        self.assertEqual(self.again(root), [])
        after, fresh = held(root), self.fresh(sdl)
        self.assertEqual({path: text for path, text in after.items() if path in fresh}, fresh)
        shutil.copy(self.kept / 'bin/pkg-config', root / 'bin/pkg-config')
        self.assertEqual(self.again(root), [])
        after = held(root)
        self.assertEqual({path: text for path, text in after.items() if path in self.base}, self.base)


# the address each of the engine's downloads comes from, as the fragments give it, and the two archives' paths
REPLAY_MK = (ROOT / 'engine/mk/replay.mk').read_text()
VULKAN_AT, GLSLANG_AT, PL_MPEG_AT = (re.search(rf'(?m)^{name}_AT := (\S+)$', text)[1] for name, text in (
    ('VULKAN', REPLAY_MK), ('GLSLANG', REPLAY_MK), ('PL_MPEG', NATIVE_MK)))
GLSLANG_TAR = re.search(r'(?m)^GLSLANG_TAR := (\S+)$', REPLAY_MK)[1]
LLVM_MINGW_RELEASE = re.search(r'(?m)^LLVM_MINGW_RELEASE := (\S+)$', NATIVE_MK)[1]
LLVM_MINGW_AT = f'https://github.com/mstorsjo/llvm-mingw/releases/download/{LLVM_MINGW_RELEASE}'
LLVM_MINGW = f'tools/llvm-mingw/llvm-mingw-{LLVM_MINGW_RELEASE}-ucrt-ubuntu-22.04-x86_64'
# each download, by its path in the checkout: the address it comes from and the text that address serves
CORE, LICENSE, MPEG = 'tools/vulkan/include/vulkan/vulkan_core.h', 'tools/vulkan/LICENSE.md', 'tools/pl_mpeg/pl_mpeg.h'
DOWNLOADS = {CORE: (f'{VULKAN_AT}/include/vulkan/vulkan_core.h', '/* vulkan_core.h */\n'),
             LICENSE: (f'{VULKAN_AT}/LICENSE.md', 'a licence\n'),
             GLSLANG_TAR: (f'{GLSLANG_AT}/{Path(GLSLANG_TAR).name}', 'glslang 1\n'),
             MPEG: (f'{PL_MPEG_AT}/pl_mpeg.h', '/* pl_mpeg.h */\n'),
             f'{LLVM_MINGW}.tar.xz': (f'{LLVM_MINGW_AT}/{Path(LLVM_MINGW).name}.tar.xz', 'llvm-mingw 1\n')}
# a curl that serves the file served/ holds for an address, less its https://, adds the address to served/log.txt,
# and fails as curl -f does where served/ holds none
SERVES = ('#!/bin/sh\nwhile [ $# -gt 1 ]; do [ "$1" = -o ] && out=$2; shift; done\necho "$1" >> served/log.txt\n'
          'cp "served/${1#https://}" "$out" 2> /dev/null || { echo "curl: (22) $1" >&2; exit 22; }\n')
# a tar that answers --version as libarchive's does, and unpacks one program from an archive: the member it is asked
# for, or ld.lld under the archive's own name. The program is a script that holds the archive's text as its second
# line, and writes that line and then its last argument's file to the file -o names, as glslang writes a shader's
# header
UNPACKS = ('#!/bin/sh\n'
           '[ "$1" = --version ] && { echo "bsdtar 3.5.3 - libarchive 3.7.4 zlib/1.2.12 liblzma/5.4.3"; exit; }\n'
           'archive=$2; name=${archive##*/}; out=$4/${5:-${name%.tar.*}/bin/ld.lld}\n'
           'mkdir -p "${out%/*}" && printf \'#!/bin/sh\\n# %s\\nwhile [ $# -gt 1 ]; do [ "$1" = -o ] && out=$2; shift; '
           'done\\n{ sed -n 2p "$0"; cat "$1"; } > "$out"\\n\' "$(cat "$archive")" > "$out" && chmod +x "$out"\n')
# the backend's raster, which reads Vulkan's header and a shader's, and the adapter's movie, which reads pl_mpeg's
RASTER, MOVIE = 'build/engine/replay/raster.o', f'{NATIVE}/obj/engine/sa1/movie.o'
SHADER = 'build/engine/replay/draw.vert.h'


def sums(texts):
    """tools/SHA256SUMS's lines for texts, each a path's text."""
    return ''.join(f'{hashlib.sha256(text.encode()).hexdigest()}  {path}\n' for path, text in texts.items())


def pinned(serves=None):
    """What a checkout takes to fetch the engine's downloads: the curl and the tar above, what each address serves,
    the downloads' texts unless serves gives a path another, the lines tools/SHA256SUMS gives those texts, a shader,
    and the two sources that read the downloads."""
    texts = {**{path: text for path, (_, text) in DOWNLOADS.items()}, **(serves or {})}
    return {'bin/curl': SERVES, 'bin/tar': UNPACKS, 'tools/SHA256SUMS': sums(texts),
            **{f'served/{DOWNLOADS[path][0][len("https://"):]}': text for path, text in texts.items()},
            'engine/pc/shaders/draw.vert': 'a shader\n',
            'engine/pc/raster.cpp': '#include <vulkan/vulkan_core.h>\n#include "draw.vert.h"\nint raster;\n',
            'engine/sa1/movie.c': '#include "pl_mpeg.h"\nint movie;\n'}


def taken(root):
    """What root holds of the downloads and of what make built with them: each file under its tools/vulkan/,
    tools/pl_mpeg/ and tools/glslang/, the raster's object, the shader's header and the movie's object."""
    paths = [path for name in ('vulkan', 'pl_mpeg', 'glslang') for path in sorted((root / 'tools' / name).rglob('*'))]
    return {path.relative_to(root).as_posix(): path.read_text().replace(str(root), '.')
            for path in (*paths, root / RASTER, root / SHADER, root / MOVIE) if path.is_file()}


def asked(root):
    """Each address root's curl was asked for, in the order of the addresses, and forgets them."""
    log = root / 'served/log.txt'
    addresses = sorted(log.read_text().split()) if log.exists() else []
    log.unlink(missing_ok=True)
    return addresses


class Pins(unittest.TestCase):
    maxDiff = None
    # the addresses of Vulkan's two files
    VULKAN = sorted(DOWNLOADS[path][0] for path in (CORE, LICENSE))

    @classmethod
    def setUpClass(cls):
        # the raster's object and the movie's build once, from downloads make fetches, in one checkout, which each
        # test takes as that build left it
        stack = contextlib.ExitStack()
        cls.addClassCleanup(stack.close)
        cls.root = stack.enter_context(sliced(pinned()))
        status, output = play(cls.root, RASTER, goal=MOVIE)
        assert status == 0, output
        cls.fetched = asked(cls.root)
        # a second make, over each file as the first dated it, builds and fetches nothing
        status, output = play(cls.root, RASTER, goal=MOVIE)
        assert (status, output, asked(cls.root)) == (0, current(RASTER, MOVIE), []), output
        settle(cls.root)
        cls.kept = Path(stack.enter_context(tempfile.TemporaryDirectory()), 'kept')
        shutil.copytree(cls.root, cls.kept)
        cls.base = taken(cls.root)

    built = Slice.built

    def again(self, root, passes=True):
        """make's output as it builds the two objects in root again, which passes unless told otherwise. Where it
        passes, the next make, over each file as that one dated it, builds nothing and asks for no address, so
        asked() gives what the first asked for. settle() then dates what root holds."""
        status, output = play(root, RASTER, goal=MOVIE)
        self.assertEqual(status == 0, passes, output)
        if passes:
            self.assertEqual(play(root, RASTER, goal=MOVIE), (0, current(RASTER, MOVIE)))
        settle(root)
        return output

    def fresh(self, files):
        """What a build of the two objects from nothing takes, in a checkout of SLICE, pinned() and files."""
        with sliced({**pinned(), **files}) as root:
            status, output = play(root, RASTER, goal=MOVIE)
            self.assertEqual(status, 0, output)
            return taken(root)

    def repin(self, root, serves, at=None):
        """Give root's tools/SHA256SUMS the lines of the downloads' texts with serves' in their places, and have each
        address serve its text. Where at is given, a pair of addresses, the make file that holds the first holds
        the second in its place, which serves what the first did. Returns the files so changed, by their paths."""
        files = {path: text for path, text in pinned(serves).items() if path.startswith(('served/', 'tools/'))}
        if at:
            path = next(path for path in ('engine/mk/replay.mk', 'engine/mk/native.mk') if at[0] in SLICE[path])
            files[path] = SLICE[path].replace(at[0], at[1])
            files = {name.replace(at[0][len('https://'):], at[1][len('https://'):]): text
                     for name, text in files.items()}
        for name, text in files.items():
            (root / name).parent.mkdir(parents=True, exist_ok=True)
            (root / name).write_text(text)
        return files

    def test_fetches_each_download_once_and_records_its_pin_beside_it(self):
        root = self.built()
        self.assertEqual(self.fetched, sorted(DOWNLOADS[path][0] for path in (CORE, LICENSE, GLSLANG_TAR, MPEG)))
        lines = sums({path: text for path, (_, text) in DOWNLOADS.items()}).splitlines()
        for directory, at in (('vulkan', VULKAN_AT), ('glslang', GLSLANG_AT), ('pl_mpeg', PL_MPEG_AT)):
            with self.subTest(directory=directory):
                self.assertEqual(self.base[f'tools/{directory}/fetched.txt'].split(),
                                 [at, *' '.join(line for line in lines if f' tools/{directory}/' in line).split()])
        self.assertEqual({path: self.base[path] for path in (CORE, LICENSE, MPEG)},
                         {path: DOWNLOADS[path][1] for path in (CORE, LICENSE, MPEG)})
        self.assertNotIn('  fetch ', self.again(root))
        self.assertEqual(asked(root), [])
        self.assertEqual(taken(root), self.base)

    def test_fetches_a_download_again_once_its_sum_changes_and_builds_what_reads_it_again(self):
        # tools/SHA256SUMS gives one file of each download another sum, and its address serves that text. make
        # fetches every file of that download again, and no other's, and what it then holds is what a build from
        # nothing under the new pin takes. Under the first pin again, it fetches them again, and holds what its first
        # build did
        for path, addresses, built in ((CORE, self.VULKAN, RASTER), (MPEG, [DOWNLOADS[MPEG][0]], MOVIE),
                                       (GLSLANG_TAR, [DOWNLOADS[GLSLANG_TAR][0]], SHADER)):
            with self.subTest(path=path):
                root = self.built()
                files = self.repin(root, {path: 'another text\n'})
                output = self.again(root)
                self.assertEqual(asked(root), addresses)
                self.assertIn(f'  fetch  {path}\n', output)
                after = taken(root)
                self.assertNotEqual(after[built], self.base[built])
                self.assertEqual(after, self.fresh(files))
                self.repin(root, {})
                self.again(root)
                self.assertEqual(asked(root), addresses)
                self.assertEqual(taken(root), self.base)

    def test_fails_on_a_download_whose_address_serves_another_text_than_its_sum_and_builds_nothing_with_it(self):
        # tools/SHA256SUMS gives Vulkan's header a sum its address doesn't serve. make fetches it, refuses it, keeps
        # no receipt, and compiles nothing against the header the last pin left. Under the first pin again, with
        # no receipt, it fetches every file of Vulkan's again
        root = self.built()
        (root / 'tools/SHA256SUMS').write_text(pinned({CORE: 'a text nothing serves\n'})['tools/SHA256SUMS'])
        output = self.again(root, passes=False)
        self.assertIn(f'{CORE}.part: FAILED\n', output)
        self.assertFalse((root / 'tools/vulkan/fetched.txt').exists())
        self.assertEqual(taken(root)[RASTER], self.base[RASTER])
        self.assertIn(DOWNLOADS[CORE][0], asked(root))
        output = self.again(root, passes=False)
        self.assertIn(f'{CORE}.part: FAILED\n', output)
        asked(root)
        self.repin(root, {})
        self.again(root)
        self.assertEqual(asked(root), self.VULKAN)
        self.assertEqual(taken(root), self.base)

    def test_refuses_a_download_changed_in_place_since_its_fetch_and_then_fetches_it_again(self):
        # a file of a download holds another text than its line sums, and is newer than the receipt, as a cache put
        # back over the checkout can leave it. make removes the receipt, writes none over the file and builds
        # nothing with it. With no receipt, the next make fetches every file of that download again
        for path, addresses, built in ((CORE, self.VULKAN, RASTER), (MPEG, [DOWNLOADS[MPEG][0]], MOVIE),
                                       (GLSLANG_TAR, [DOWNLOADS[GLSLANG_TAR][0]], SHADER)):
            with self.subTest(path=path):
                root = self.built()
                (root / path).write_text('changed in place\n')
                output = self.again(root, passes=False)
                self.assertIn(f'{path}: FAILED\n', output)
                self.assertFalse((root / 'tools' / Path(path).parts[1] / 'fetched.txt').exists())
                self.assertEqual((asked(root), taken(root)[built]), ([], self.base[built]))
                self.again(root)
                self.assertEqual(asked(root), addresses)
                self.assertEqual(taken(root), self.base)

    def test_fetches_a_download_again_from_another_address_and_fails_where_it_serves_another_text(self):
        # the make file names another commit for Vulkan's headers, and tools/SHA256SUMS the same sums. make fetches
        # both files from the new address: where it serves the same texts, the receipt names it, and where it
        # serves another, make refuses the file
        commit = VULKAN_AT.rsplit('/', 1)[1]
        other = VULKAN_AT.replace(commit, 'f' * len(commit))
        root = self.built()
        self.repin(root, {}, at=(VULKAN_AT, other))
        self.again(root)
        self.assertEqual(asked(root), sorted(address.replace(VULKAN_AT, other) for address in self.VULKAN))
        self.assertEqual((root / 'tools/vulkan/fetched.txt').read_text().split()[0], other)
        root = self.built()
        files = self.repin(root, {}, at=(VULKAN_AT, other))
        served = next(name for name in files if name.endswith('vulkan_core.h'))
        (root / served).write_text('/* another commit */\n')
        output = self.again(root, passes=False)
        self.assertIn(f'{CORE}.part: FAILED\n', output)
        self.assertFalse((root / 'tools/vulkan/fetched.txt').exists())
        self.assertEqual(taken(root)[RASTER], self.base[RASTER])

    def test_removes_a_file_the_pin_no_longer_lists_and_fetches_the_rest_again_after_a_fetch_cut_short(self):
        # tools/SHA256SUMS drops Vulkan's licence: make fetches the header again and removes the licence. With the
        # receipt gone, as a fetch that failed leaves it, make fetches every file again
        root = self.built()
        lines = (root / 'tools/SHA256SUMS').read_text().splitlines(True)
        (root / 'tools/SHA256SUMS').write_text(''.join(line for line in lines if LICENSE not in line))
        self.again(root)
        self.assertEqual(asked(root), [DOWNLOADS[CORE][0]])
        self.assertFalse((root / LICENSE).exists())
        self.assertTrue((root / CORE).exists())
        (root / 'tools/vulkan/fetched.txt').unlink()
        self.again(root)
        self.assertEqual(asked(root), [DOWNLOADS[CORE][0]])

    def test_fetches_llvm_mingw_again_once_its_sum_or_its_archive_changes_and_compiles_for_windows_again(self):
        # the front end's object for Windows waits on llvm-mingw's receipt: make fetches the archive, unpacks it and
        # records its pin before the compile, and does all three again under another sum. A dry run fetches nothing:
        # the copy of the game's C, which make writes as it starts, waits on no toolchain. The archive changed in
        # place fails its line, as a file of another download does: make keeps no receipt, compiles nothing with
        # what the archive unpacked to, and fetches it again on the next run
        tar, play_o = f'{LLVM_MINGW}.tar.xz', f'{NATIVE}/obj/engine/sa1/play.o'
        root = self.built()
        status, output = play(root, '-n', goal=play_o, NATIVE_HOST='windows')
        self.assertEqual((status, asked(root)), (0, []), output)
        self.assertIn(f'curl -fsSL -o {tar}.part {DOWNLOADS[tar][0]}', output)
        for text in (DOWNLOADS[tar][1], 'another llvm-mingw\n'):
            with self.subTest(text=text):
                self.repin(root, {tar: text})
                status, output = play(root, goal=play_o, NATIVE_HOST='windows')
                self.assertEqual(status, 0, output)
                self.assertEqual(asked(root), [DOWNLOADS[tar][0]])
                self.assertEqual(compiled(output), ['engine/sa1/play.c'])
                self.assertEqual((root / LLVM_MINGW / 'bin/ld.lld').read_text().splitlines()[1], f'# {text.strip()}')
                self.assertEqual((root / 'tools/llvm-mingw/fetched.txt').read_text().split()[:2],
                                 [LLVM_MINGW_AT, hashlib.sha256(text.encode()).hexdigest()])
                self.assertEqual(play(root, goal=play_o, NATIVE_HOST='windows'), (0, current(play_o)))
                self.assertEqual(asked(root), [])
                settle(root)
        (root / tar).write_text('changed in place\n')
        status, output = play(root, goal=play_o, NATIVE_HOST='windows')
        self.assertNotEqual(status, 0, output)
        self.assertIn(f'{tar}: FAILED\n', output)
        self.assertEqual((asked(root), compiled(output)), ([], []))
        self.assertFalse((root / 'tools/llvm-mingw/fetched.txt').exists())
        status, output = play(root, goal=play_o, NATIVE_HOST='windows')
        self.assertEqual((status, asked(root), compiled(output)), (0, [DOWNLOADS[tar][0]], ['engine/sa1/play.c']),
                         output)
