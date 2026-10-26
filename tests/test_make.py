import contextlib
import fnmatch
import hashlib
import os
import re
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path

import vendor
from config import EVENT_ALIASES, ROOTS
from support import MODULE, tree

ROOT = Path(__file__).resolve().parents[1]
# a compilers.mk that names no source, with the flags the Makefile's rules read
COMPILERS = 'GAME := -cpu=sh4\nSDK := -cpu=sh4\n'
HASH = '0' * 64


@contextlib.contextmanager
def checkout(files=None):
    """A tree make runs in: the repository's Makefile and Python tools, the
    vendored tools' files, empty, a compilers.mk that names no source,
    MODULE's tables and sources, an empty orig/, and files."""
    tools = {f'tools/{path.name}': path.read_text() for path in (ROOT / 'tools').glob('*.py')}
    vendored = {path: '' for tool in vendor.TOOLS for path in vendor.copied(tool)}
    with tree({'Makefile': (ROOT / 'Makefile').read_text(), 'config/compilers.mk': COMPILERS, 'orig/.gitkeep': '',
               'SHA256SUMS': f'{HASH}  orig/1ST_READ.BIN\n', 'tools/SHA256SUMS': '', **tools, **vendored, **MODULE,
               **(files or {})}) as root:
        yield root


def make(root, *args, path=None):
    """make's status and output, stdout and stderr together, run in root
    with args, one job at a time, apart from any make running this, in the
    C locale, so that a host tool's messages read as the tests expect, and
    with path for PATH when given."""
    env = {name: value for name, value in os.environ.items() if name not in ('MAKEFLAGS', 'MFLAGS', 'MAKELEVEL')}
    env['LC_ALL'] = 'C'
    if path is not None:
        env['PATH'] = path
    run = subprocess.run(['make', '-j1', *args], cwd=root, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         text=True)
    return run.returncode, run.stdout


# the commands but the C compiler that the Makefile asks of the host
COMMANDS = ('sha256sum', 'timeout', 'curl', 'tar', 'gzip')


@contextlib.contextmanager
def host(*lacking):
    """A PATH of one directory, which holds each command PATH finds and each
    of COMMANDS, but none of lacking: a link to the command, or for one of
    COMMANDS that PATH lacks, a script that does nothing."""
    with tempfile.TemporaryDirectory() as bin:
        # a directory early in PATH links over a later one's command
        for directory in reversed(os.environ['PATH'].split(os.pathsep)):
            for found in Path(directory).glob('*') if Path(directory).is_dir() else ():
                if os.access(found, os.X_OK):
                    Path(bin, found.name).unlink(missing_ok=True)
                    Path(bin, found.name).symlink_to(found)
        for command in COMMANDS:
            if not Path(bin, command).exists():
                Path(bin, command).write_text('#!/bin/sh\n')
                Path(bin, command).chmod(0o755)
        for command in lacking:
            Path(bin, command).unlink(missing_ok=True)
        yield bin


def settle(root):
    """Date every file under root an hour back, and those under build/ half
    an hour back, so that make takes each file under build/ for newer than
    what it was made from."""
    now = time.time()
    for path in root.rglob('*'):
        age = 1800 if path.relative_to(root).parts[0] == 'build' else 3600
        os.utime(path, (now - age, now - age))


def placing(source):
    # mod's splits.txt with a row for source after b.src's
    return {'config/mod/splits.txt': MODULE['config/mod/splits.txt'] + f'{source}  P  0x0c900040  0x0c900050\n'}


class Images(unittest.TestCase):
    def test_expects_a_line_for_each_image_config_describes(self):
        # SHA256SUMS lists 1ST_READ.BIN's original alone; make check still expects MOD.PRS's lines, and the events'
        # that repeat another
        with checkout() as root:
            status, output = make(root, '-n', 'check')
            self.assertEqual(status, 0, output)
            expected = set((root / 'build/expected.txt').read_text().split())
        for path in ('orig/1ST_READ.BIN', 'orig/SONICADV/MOD.PRS', 'orig/SONICADV/EV0054.PRS', 'build/SONICADV/MOD.BIN',
                     'build/mod/modules/mod/a.bin'):
            self.assertIn(path, expected)

    def test_expects_no_line_for_a_file_rebuilt_from_an_original(self):
        # make checks build/1ST_READ.BIN and build/SONICADV/MOD.PRS against their originals' lines
        with checkout() as root:
            status, output = make(root, '-n', 'check')
            self.assertEqual(status, 0, output)
            expected = set((root / 'build/expected.txt').read_text().split())
        self.assertFalse(expected & {'build/1ST_READ.BIN', 'build/SONICADV/MOD.PRS'})

    def test_splices_each_image_from_its_original_into_a_file_of_its_own(self):
        # the splicer reads 1ST_READ.BIN's original in orig/, and MOD.PRS's as build/gen/img/ decodes it, and no other
        # step writes either image's file: neither starts as a copy of its original
        with checkout({'orig/1ST_READ.BIN': '', 'orig/SONICADV/MOD.PRS': ''}) as root:
            status, output = make(root, '-n', 'build/1ST_READ.BIN', 'build/SONICADV/MOD.BIN')
        self.assertEqual(status, 0, output)
        self.assertIn('build/prskit decompress orig/SONICADV/MOD.PRS build/gen/img/MOD.BIN', output)
        for original, image in (('orig/1ST_READ.BIN', 'build/1ST_READ.BIN'), ('build/gen/img/MOD.BIN', 'build/SONICADV/MOD.BIN')):
            with self.subTest(image=image):
                self.assertEqual([line.split()[:4] for line in output.splitlines() if image in line.split()],
                                 [['python3', 'tools/splice.py', original, image]])


class Stubs(unittest.TestCase):
    def test_writes_each_stub_include_by_its_kind(self):
        # a body, texts and values: the body's stub is 0, and tools/data.py writes the others from the source
        source = ('#define TEXT_1_LENGTH 2\n#include "mod/a/text.inc"\n#include "mod/a/values.inc"\n'
                  'static int a[1] = {\n#include "mod/a/a.inc"\n};\nstatic int b[1] = {VALUE_1};\n')
        with checkout({'modules/mod/a.c': source}) as root:
            status, output = make(root, '-n', 'check')
        self.assertEqual(status, 0, output)
        self.assertIn('echo 0 > build/stub/modules/mod/a/a.inc', output)
        self.assertIn('python3 tools/data.py --texts modules/mod/a.c build/stub/modules/mod/a/text.inc', output)
        self.assertIn('python3 tools/data.py --values modules/mod/a.c build/stub/modules/mod/a/values.inc', output)
        self.assertNotIn('> build/stub/modules/mod/a/values.inc', output)

    def test_keeps_each_stub_object_to_its_own_includes(self):
        # two sources with a body each: each stub object waits for the zero bodies and each lint target for every
        # stub include, through one shared target each, and neither lists another source's include, so make keeps
        # one copy of each list
        files = {**placing('modules/mod/c.c'), 'modules/mod/a.c': 'static int a[1] = {\n#include "mod/a/a.inc"\n};\n',
                 'modules/mod/c.c': 'static int c[1] = {\n#include "mod/c/c.inc"\n};\n'}
        with checkout(files) as root:
            status, output = make(root, '-n', '-p', 'check')
        self.assertEqual(status, 0, output[-3000:])
        rules = dict(line.split(':', 1) for line in output.splitlines()
                     if line.startswith(('build/stub/modules/mod/a.obj:', 'build/lint/modules/mod/a.c.tidy:')))
        self.assertIn('| stub-bodies', rules['build/stub/modules/mod/a.obj'])
        self.assertIn('| stub-includes', rules['build/lint/modules/mod/a.c.tidy'])
        for rule in rules.values():
            self.assertNotIn('build/stub/modules/mod/c/c.inc', rule)

    def test_builds_each_stub_with_its_full_objects_flags(self):
        # a flag config/compilers.mk gives build/X.obj alone reaches build/stub/X.obj only where make builds the
        # stub as the full object's prerequisite, so make check, which builds the stub alone, compiles it otherwise
        # and its link fails; make's database gives every object's flags, the patterns' with them
        status, output = make(ROOT, '-pqn', 'help')
        self.assertIn(status, (0, 1), output[-3000:])
        body = re.search(r'(?m)^BODY_C := (.*)$', output)[1].split()
        flags = {}
        for line in re.finditer(r'(?m)^(build/\S+\.obj): (\w+ (?:\+=|:=|\?=|=) .*)$', output):
            flags.setdefault(line[1], []).append(line[2])
        patterns = output[output.index('# Pattern-specific Variable Values'):]
        patterns = re.findall(r'(?m)^(\S*%\S*) :\n# makefile[^\n]*\n# (\w+ (?:\+=|:=|\?=|=) .*)$', patterns)
        self.assertTrue(body and flags and patterns, 'no body sources, flags or patterns in make\'s database')

        def given(obj):
            own = flags.get(obj, [])
            matched = [flag for pattern, flag in patterns if fnmatch.fnmatchcase(obj, pattern.replace('%', '*'))]
            return sorted(own + matched)

        differ = [source for source in body
                  if given(f'build/{source[:-2]}.obj') != given(f'build/stub/{source[:-2]}.obj')]
        self.assertEqual(differ, [])

    def test_stops_on_a_body_source_whose_route_builds_no_stub(self):
        # x.c includes a body, and none of these routes has a stub rule, so x.c's stub would build as game code
        routes = [(group, 'sdk/ninja/x.c') for group in ('R26_C', 'R32_C', 'RUNTIME_C', 'GRAFTED_C', 'ASMCODE')]
        # the events' shared C runtime is on the runtime's route by its directory, not a group
        routes += [(None, 'events/shared/sdk/x.c')]
        # and on these x.c's stub would search other headers than its route does
        routes += [('LOADER_C', 'loader/x.c'), ('PATCHED_C', 'src/x.c')]
        for group, source in routes:
            files = {'config/compilers.mk': COMPILERS + (f'{group} := {source}\n' if group else ''),
                     'config/mod/splits.txt': f"{MODULE['config/mod/splits.txt']}{source}  P  0x0c900040  0x0c900050\n",
                     source: f'static int a[1] = {{\n#include "{source.split("/", 1)[1][:-2]}/a.inc"\n}};\n'}
            with self.subTest(source=source, group=group), checkout(files) as root:
                status, output = make(root, '-n', 'check')
                self.assertNotEqual(status, 0, output)
                self.assertIn(f"{source}: no rule builds this body source's stub on its route", output)

    def test_touches_each_object_after_its_listing(self):
        # an object depends on its listing, which SHC or asmsh can write after the object, so each kind touches its
        # object once the tool that writes both is done: the stubs of a game source, an SDK source and a patched
        # source, and the objects asmsh assembles of a patched source, a source with inline assembly and a .src file
        files = {'config/compilers.mk': COMPILERS + ('PATCHED_C := sdk/ninja/p.c sdk/ninja/q.c\nPATCHES := align-pool\n'
                                                     'ASMCODE := sdk/ninja/i.c\n'),
                 'config/mod/splits.txt': (MODULE['config/mod/splits.txt'] + 'sdk/ninja/s.c  P  0x0c900040  0x0c900050\n'
                                           'sdk/ninja/p.c  P  0x0c900060  0x0c900070\n'
                                           'sdk/ninja/q.c  P  0x0c900080  0x0c900090\n'
                                           'sdk/ninja/i.c  P  0x0c9000a0  0x0c9000b0\n'),
                 'sdk/ninja/q.c': '', 'sdk/ninja/i.c': '',
                 **{f'{stub}.c': f'static int a[1] = {{\n#include "{stub.split("/", 1)[1]}/a.inc"\n}};\n'
                    for stub in ('modules/mod/a', 'sdk/ninja/s', 'sdk/ninja/p')}}
        with checkout(files) as root:
            status, output = make(root, '-n', 'check')
        self.assertEqual(status, 0, output)
        lines = output.splitlines()
        for stem in ('build/stub/modules/mod/a', 'build/stub/sdk/ninja/s', 'build/stub/sdk/ninja/p', 'build/sdk/ninja/q',
                     'build/sdk/ninja/i', 'build/modules/mod/b'):
            tool = max(index for index, line in enumerate(lines)
                       if ('shc.exe' in line or 'asmsh.exe' in line) and f'{stem}.' in line.replace('\\', '/'))
            self.assertEqual(lines[tool + 1], f'touch {stem}.obj', stem)


class Patched(unittest.TestCase):
    def test_declares_the_alignment_an_align_gives_between_shckit_and_asmsh(self):
        # two patched sources: p.c with an ALIGN, and q.c without
        files = {'config/compilers.mk': COMPILERS + ('PATCHED_C := sdk/ninja/p.c sdk/ninja/q.c\n'
                                                     'PATCHES := align-pool\n'
                                                     'build/sdk/ninja/p.obj: private ALIGN := P 32\n'),
                 'config/mod/splits.txt': (MODULE['config/mod/splits.txt'] + 'sdk/ninja/p.c  P  0x0c900040  0x0c900050\n'
                                           'sdk/ninja/q.c  P  0x0c900060  0x0c900070\n'),
                 'sdk/ninja/p.c': '', 'sdk/ninja/q.c': ''}
        with checkout(files) as root:
            status, output = make(root, '-n', 'check')
        self.assertEqual(status, 0, output)
        lines = [line.replace('\\', '/') for line in output.splitlines()]
        for stem, aligned in (('build/sdk/ninja/p', True), ('build/sdk/ninja/q', False)):
            with self.subTest(stem):
                apply = lines.index(f'build/shckit apply -p align-pool -o {stem}_patched.src {stem}.src')
                steps = [f'python3 tools/align.py P 32 {stem}_patched.src {stem}_aligned.src'] if aligned else []
                self.assertEqual(lines[apply + 1:apply + 1 + len(steps)], steps)
                assembled = lines[apply + 1 + len(steps)]
                self.assertIn('asmsh.exe', assembled)
                self.assertIn(f"'{stem}_{'aligned' if aligned else 'patched'}.src'", assembled)
        self.assertNotIn('tools/align.py P 32 build/sdk/ninja/q', output)

    def test_refuses_an_align_on_any_other_object(self):
        # src/main.c takes the game's route, modules/mod/b.src the assembler's, sdk/ninja/s.c R10's, sdk/ninja/g.c the
        # grafted one and the gen/ range the generated one, and the stubs of modules/mod/a.c and of sdk/ninja/p.c, a
        # patched source, take their stub rules. None runs tools/align.py, so each would build its object without the
        # ALIGN. With no ALIGN the tree builds.
        splits = MODULE['config/mod/splits.txt'] + ('sdk/ninja/s.c  P  0x0c900040  0x0c900050\n'
                                                    'sdk/ninja/g.c  P  0x0c900060  0x0c900070\n'
                                                    'sdk/ninja/p.c  P  0x0c900080  0x0c900090\n'
                                                    'gen/mod/d_0c900180.c  D  0x0c900180  0x0c900200\n')
        routes = COMPILERS + ('GRAFTED_C := sdk/ninja/g.c\nDONOR_C := sdk/ninja/d.c\nPATCHED_C := sdk/ninja/p.c\n'
                              'PATCHES := align-pool\n')
        bodies = {f'{stem}.c': f'static int a[1] = {{\n#include "{stem.split("/", 1)[1]}/a.inc"\n}};\n'
                  for stem in ('modules/mod/a', 'sdk/ninja/p')}
        gen = 'build/gen/mod/d_0c900180.obj'
        for obj in (None, 'build/src/main.obj', 'build/modules/mod/b.obj', 'build/sdk/ninja/s.obj',
                    'build/sdk/ninja/g.obj', gen, 'build/stub/modules/mod/a.obj', 'build/stub/sdk/ninja/p.obj'):
            setting = f'{obj}: private ALIGN := P 32\n' if obj else ''
            files = {'config/compilers.mk': routes + setting, 'config/mod/splits.txt': splits, 'sdk/ninja/s.c': '',
                     'sdk/ninja/g.c': '', 'sdk/ninja/d.c': '', **bodies, 'orig/SONICADV/MOD.PRS': ''}
            with self.subTest(obj), checkout(files) as root:
                # make check builds no gen/ range's object
                status, output = make(root, '-n', 'check', gen)
                if obj is None:
                    self.assertEqual(status, 0, output)
                else:
                    self.assertNotEqual(status, 0, output)
                    self.assertIn(f'{obj}: its recipe runs no tools/align.py, so it takes no ALIGN', output)


# COMPILERS, and config/compilers.mk's own lines that find the C with inline assembly and route it
FOUND = COMPILERS + ''.join(line for line in (ROOT / 'config/compilers.mk').read_text().splitlines(True)
                            if line.startswith(('inline_asm =', 'INLINE_C :=', 'ASMCODE :=')))
# C that mentions the pragmas without giving them: in a comment, in a string continued onto a line of its own, and on
# lines of a block comment
MENTIONS = {'comment': '/* Unlike #pragma inline_asm, this is ordinary C */\n/* nor does it follow #pragma asm\n */\n',
            'string': 'static const char note[] = "\\\n#pragma inline_asm(f)";\n',
            'block comment': '/*\n#pragma inline_asm(f)\n#pragma asm\n */\n'}
# C that gives each pragma: an inline body, one whose pragma has a space before its (, which SHC takes too, and a
# block of assembly
GIVEN = {'inline_asm': '#pragma inline_asm(f)\nstatic void f(void)\n{\n        NOP\n}\n',
         'spaced inline_asm': '#pragma inline_asm (f)\nstatic void f(void)\n{\n        NOP\n}\n',
         'asm': '#pragma asm\n        NOP\n#pragma endasm\n'}


class Inline(unittest.TestCase):
    def test_takes_a_source_to_assembly_by_its_directives_not_by_what_mentions_them(self):
        # src/main.c compiles with R32 straight to an object, or with R10 to assembly where it gives either pragma
        for kind, text in (*MENTIONS.items(), *GIVEN.items()):
            with self.subTest(kind), checkout({**SEARCHED, 'config/compilers.mk': FOUND, 'src/main.c': text}) as root:
                status, output = make(root, '-n', 'check')
                self.assertEqual(status, 0, output)
                self.assertIn('shc.exe src/main.c -objectfile=build/src/main.src -code=asmcode' if kind in GIVEN else
                              'shc.exe src/main.c -objectfile=build/src/main.obj -code=machinecode', output)

    def test_stops_on_a_module_s_directives_not_on_what_mentions_them(self):
        # no rule gives inline assembly in a module its release and flags
        for kind, text in (*MENTIONS.items(), *GIVEN.items()):
            with self.subTest(kind), checkout({**SEARCHED, 'config/compilers.mk': FOUND,
                                               'modules/mod/a.c': text}) as root:
                status, output = make(root, '-n', 'check')
                stopped = 'modules/mod/a.c: no rule gives this inline assembly its release and flags' in output
                self.assertEqual((status != 0, stopped), (kind in GIVEN, kind in GIVEN), output)

    def test_stops_when_it_can_t_list_the_sources_with_inline_assembly(self):
        # with no sdk/ for tags.py to read, make stops rather than route src/main.c by a partial list
        searched = {path: text for path, text in SEARCHED.items() if not path.startswith('sdk/')}
        with checkout({**searched, 'config/compilers.mk': FOUND, 'src/main.c': GIVEN['inline_asm']}) as root:
            status, output = make(root, '-n', 'check')
        self.assertNotEqual(status, 0, output)
        self.assertIn('tags: sdk: No such file or directory', output)
        self.assertIn("tools/tags.py --inline failed; make can't tell which sources hold inline assembly", output)


# a wibo that ignores TERM, and makes the file "survived" if it runs two seconds
OUTLIVES_TERM = "#!/bin/sh\ntrap '' TERM\nsleep 2\ntouch survived\n"


class Hung(unittest.TestCase):
    def test_kills_a_tool_still_running_its_grace_after_term(self):
        # with the limits cut to half a second each, timeout sends the stand-in wibo TERM, then KILL, a second before
        # it would write survived
        with checkout({**SEARCHED, 'tools/wibo-i686': OUTLIVES_TERM, 'tools/shc-v5.0r32/shc.exe': ''}) as root:
            (root / 'tools/wibo-i686').chmod(0o755)
            status, output = make(root, 'build/modules/mod/b.obj', 'HUNG=0.5', 'GRACE=0.5')
            time.sleep(2)
            survived = (root / 'survived').exists()
        self.assertNotEqual(status, 0, output)
        for sent in ('TERM', 'KILL'):
            self.assertIn(f"timeout: sending signal {sent} to command 'tools/wibo-i686'", output)
        self.assertFalse(survived)


def depends(target, source, *headers):
    """A .d file's text for target, as the host compiler's -MM -MP writes it."""
    return f'{target}: {" ".join((source, *headers))}\n' + ''.join(f'{header}:\n' for header in headers)


# a.c, a source that includes a header, a body, its texts and its values; a gen/ range in MOD.PRS's D; g.c, a
# grafted source, whose donor includes a header; p.c, a patched source; s.c, whose object R10 alone builds;
# MOD.PRS's original; and every file make writes in building them, their stubs and the other sources make check
# builds, and in splicing MOD.BIN from them
A_INCS = ('text.inc', 'values.inc', 'a.inc')
BUILT = {
    'config/compilers.mk': COMPILERS + ('GRAFTED_C := sdk/ninja/g.c\nDONOR_C := sdk/ninja/d.c\n'
                                        'PATCHED_C := sdk/ninja/p.c\nPATCHES := align-pool\n'),
    'sdk/ninja/g.c': '',
    'sdk/ninja/d.c': '#include "ninja/d.h"\n',
    'sdk/ninja/d.h': '',
    'sdk/ninja/p.c': '',
    'sdk/ninja/s.c': '',
    'modules/mod/a.c': ('#include "mod/a.h"\n#define TEXT_1_LENGTH 2\n#include "mod/a/text.inc"\n'
                        '#include "mod/a/values.inc"\nstatic int a[1] = {\n#include "mod/a/a.inc"\n};\n'
                        'static int b[1] = {VALUE_1};\n'),
    'modules/mod/a.h': '',
    'config/mod/splits.txt': (MODULE['config/mod/splits.txt'] + 'sdk/ninja/g.c  P  0x0c900040  0x0c900050\n'
                              'sdk/ninja/p.c  P  0x0c900060  0x0c900070\n'
                              'sdk/ninja/s.c  P  0x0c900080  0x0c900090\n'
                              'gen/mod/d_0c900180.c  D  0x0c900180  0x0c900200\n'),
    'orig/SONICADV/MOD.PRS': '',
    **{path: '' for path in (
        'build/tools/wibo.id', 'build/tools/r10.id', 'build/tools/r26.id', 'build/tools/r32.id',
        'build/tools/vendored.ok', 'build/prskit',
        'build/src/main.obj', 'build/1st_read/src/main.bin', 'build/modules/mod/b.obj', 'build/mod/modules/mod/b.bin',
        *(f'build/stub/modules/mod/a/{inc}' for inc in A_INCS), 'build/stub/modules/mod/a.obj',
        'build/stub/modules/mod/a.lis', 'build/stub/mod/modules/mod/a.bin', 'build/verified/SONICADV/MOD.PRS.ok',
        'build/gen/img/MOD.BIN', *(f'build/gen/modules/mod/a/{inc}' for inc in A_INCS),
        'build/gen/modules/mod/a/bodies.txt', 'build/modules/mod/a.obj', 'build/mod/modules/mod/a.bin',
        'build/gen/mod/declared.stamp', 'build/gen/mod/declared.txt', 'build/gen/mod/d_0c900180.syms',
        'build/gen/mod/d_0c900180.c', 'build/gen/mod/d_0c900180.obj', 'build/mod/gen/mod/d_0c900180.bin',
        'build/shckit', 'build/sdk/ninja/g.obj', 'build/sdk/ninja/g.lis', 'build/mod/sdk/ninja/g.bin',
        'build/sdk/ninja/p.obj', 'build/sdk/ninja/p.lis', 'build/mod/sdk/ninja/p.bin', 'build/sdk/ninja/s.obj',
        'build/mod/sdk/ninja/s.bin', 'build/modules/mod/b.lis', 'build/SONICADV/MOD.BIN')},
    'build/src/main.d': depends('build/src/main.obj', 'src/main.c'),
    'build/stub/modules/mod/a.d': depends('build/stub/modules/mod/a.obj', 'modules/mod/a.c', 'modules/mod/a.h',
                                          *(f'build/stub/modules/mod/a/{inc}' for inc in A_INCS)),
    'build/modules/mod/a.d': depends('build/modules/mod/a.obj', 'modules/mod/a.c', 'modules/mod/a.h',
                                     *(f'build/gen/modules/mod/a/{inc}' for inc in A_INCS)),
    'build/gen/mod/d_0c900180.d': depends('build/gen/mod/d_0c900180.obj', 'build/gen/mod/d_0c900180.c'),
    'build/sdk/ninja/g.d': depends('build/sdk/ninja/g.obj', 'sdk/ninja/g.c'),
    'build/sdk/ninja/g_donor.d': depends('build/sdk/ninja/g.obj', 'sdk/ninja/d.c', 'sdk/ninja/d.h'),
    'build/sdk/ninja/p.d': depends('build/sdk/ninja/p.obj', 'sdk/ninja/p.c'),
    'build/sdk/ninja/s.d': depends('build/sdk/ninja/s.obj', 'sdk/ninja/s.c'),
}
# the goals: make check, the links of a.c, the gen/ range and g.c, and MOD.BIN
CHECK, FULL, GEN = 'check', 'build/mod/modules/mod/a.bin', 'build/mod/gen/mod/d_0c900180.bin'
GRAFT, IMAGE = 'build/mod/sdk/ninja/g.bin', 'build/SONICADV/MOD.BIN'
# each step, by the text make -n prints for it
STUB_SHC = 'shc.exe modules/mod/a.c -sub=build/stub/modules/mod/a.obj.sub'
ZERO = 'echo 0 > build/stub/modules/mod/a/a.inc'
TEXTS = 'tools/data.py --texts modules/mod/a.c build/stub/modules/mod/a/text.inc'
VALUES = 'tools/data.py --values modules/mod/a.c build/stub/modules/mod/a/values.inc'
VERIFY = 'touch build/verified/SONICADV/MOD.PRS.ok'
DECODE = 'build/prskit decompress orig/SONICADV/MOD.PRS build/gen/img/MOD.BIN'
BODIES = 'tools/data.py --bodies build/gen/img/MOD.BIN modules/mod/a.c'
SHC = 'shc.exe modules/mod/a.c -objectfile=build/modules/mod/a.obj'
SCAN = 'touch build/gen/mod/declared.stamp'
DECLARED = '[ -f build/gen/mod/declared.txt ] || python3 tools/data.py --declared mod build/gen/mod/declared.txt'
GENERATE = 'tools/data.py build/gen/img/MOD.BIN gen/mod/d_0c900180.c build/gen/mod/d_0c900180.c'
GEN_SHC = 'shc.exe build/gen/mod/d_0c900180.c'
GRAFT_SHC = 'shc.exe sdk/ninja/g.c -objectfile=build/sdk/ninja/g.src'
PATCHED_SHC = 'shc.exe sdk/ninja/p.c -objectfile=build/sdk/ninja/p.src'
ASM = r"asmsh.exe 'modules\mod\b.src'"
SDK_LINK = 'tools/link.py build/sdk/ninja/s.obj'
SPLICE = 'tools/splice.py build/gen/img/MOD.BIN build/SONICADV/MOD.BIN'
STEPS = (STUB_SHC, ZERO, TEXTS, VALUES, VERIFY, DECODE, BODIES, SHC, SCAN, DECLARED, GENERATE, GEN_SHC, GRAFT_SHC,
         PATCHED_SHC, ASM, 'tools/link.py', SPLICE)


class Freshness(unittest.TestCase):
    # each side file, a goal that needs it, and the steps its loss reruns: the step that writes it, and those that
    # read it
    DELETED = [
        ('build/modules/mod/a.d', FULL, [SHC]),
        ('build/stub/modules/mod/a.d', CHECK, [STUB_SHC]),
        ('build/stub/modules/mod/a.lis', CHECK, [STUB_SHC]),
        ('build/stub/modules/mod/a/a.inc', CHECK, [ZERO, STUB_SHC]),
        ('build/stub/modules/mod/a/text.inc', CHECK, [TEXTS, STUB_SHC]),
        ('build/stub/modules/mod/a/values.inc', CHECK, [VALUES, STUB_SHC]),
        ('build/gen/modules/mod/a/a.inc', FULL, [BODIES, SHC]),
        ('build/gen/modules/mod/a/text.inc', FULL, [BODIES, SHC]),
        ('build/gen/modules/mod/a/values.inc', FULL, [BODIES, SHC]),
        ('build/gen/modules/mod/a/bodies.txt', FULL, [BODIES, SHC]),
        ('build/gen/mod/declared.txt', GEN, [DECLARED, GENERATE, GEN_SHC]),
        ('build/gen/mod/d_0c900180.syms', GEN, [GENERATE, GEN_SHC]),
        ('build/gen/mod/d_0c900180.d', GEN, [GEN_SHC]),
        ('build/sdk/ninja/g.d', GRAFT, [GRAFT_SHC]),
        ('build/sdk/ninja/g_donor.d', GRAFT, [GRAFT_SHC]),
        ('build/sdk/ninja/g.lis', GRAFT, [GRAFT_SHC]),
        ('build/modules/mod/b.lis', CHECK, [ASM]),
    ]
    # each file a step reads, a goal that needs it, and the steps a change to it reruns
    CHANGED = [
        ('modules/mod/a.h', FULL, [STUB_SHC, BODIES, SHC]),
        ('modules/mod/a.c', CHECK, [TEXTS, VALUES, STUB_SHC]),
        ('orig/SONICADV/MOD.PRS', FULL, [VERIFY, DECODE, BODIES, SHC]),
        ('config/mod/splits.txt', GEN, [SCAN, GENERATE, GEN_SHC]),
        ('config/mod/splits.txt', FULL, [BODIES, SHC]),
        ('build/sources.txt', GEN, [SCAN]),
        ('sdk/ninja/d.h', GRAFT, [GRAFT_SHC]),
        ('tools/align.py', CHECK, [PATCHED_SHC]),
        ('build/tools/r32.id', CHECK, [SDK_LINK]),
        ('tools/code_ranges.py', IMAGE, [SPLICE]),
    ]

    def test_reruns_the_steps_a_deleted_file_feeds(self):
        with self.built() as root:
            for path, goal, steps in self.DELETED:
                with self.subTest(path):
                    (root / path).unlink()
                    try:
                        self.assertReruns(root, goal, steps)
                    finally:
                        (root / path).write_text(BUILT[path])
                        settle(root)

    def test_reruns_the_steps_a_changed_file_feeds(self):
        with self.built() as root:
            for path, goal, steps in self.CHANGED:
                with self.subTest(path):
                    os.utime(root / path)
                    try:
                        self.assertReruns(root, goal, steps)
                    finally:
                        settle(root)

    def test_generates_a_range_again_when_its_tables_change_and_its_declarations_dont(self):
        # the scan rewrites declared.txt only when what it declares changes, so the tables reach the gen/ C by an edge
        # of their own; -o keeps make -n from taking the scan's rerun for a new declared.txt
        with self.built() as root:
            os.utime(root / 'config/mod/splits.txt')
            status, output = make(root, '-n', '-o', 'build/gen/mod/declared.stamp', GEN)
        self.assertEqual(status, 0, output)
        self.assertNotIn(DECLARED, output)
        for step in (GENERATE, GEN_SHC):
            self.assertIn(step, output)

    # each file a step reads, a goal that reads it, a step a change to it reruns, and those it leaves alone: the
    # host tools read no SHC flag, so a compilers.mk edit rebuilds no tool and decodes no image again; and a tool
    # waits for the vendored check without depending on it, so the check's rerun rebuilds none
    APART = [
        ('config/compilers.mk', FULL, [SHC], ['tools/prskit/prskit.c', DECODE]),
        ('tools/config.py', 'build/prskit', ['tools/vendor.py --check'], ['tools/prskit/prskit.c']),
    ]

    def test_reruns_no_step_a_changed_file_doesnt_feed(self):
        with self.built() as root:
            for path, goal, runs, skips in self.APART:
                with self.subTest(path):
                    os.utime(root / path)
                    try:
                        self.assertReruns(root, goal, runs)
                        status, output = make(root, '-n', goal)
                        for step in skips:
                            self.assertNotIn(step, output)
                    finally:
                        settle(root)

    @contextlib.contextmanager
    def built(self):
        """BUILT's checkout, with the pinned tools in place, in which make -n prints none of STEPS for CHECK, FULL,
        GEN, GRAFT or IMAGE."""
        with checkout({**BUILT, **PINNED}) as root:
            # the first make writes build/deps.mk and build/sources.txt, which settle() then dates with the rest
            status, output = make(root, '-n', CHECK)
            self.assertEqual(status, 0, output)
            settle(root)
            for goal in (CHECK, FULL, GEN, GRAFT, IMAGE):
                status, output = make(root, '-n', goal)
                self.assertEqual(status, 0, output)
                self.assertEqual([step for step in STEPS if step in output], [], output)
            yield root

    def assertReruns(self, root, goal, steps):
        """make -n goal, in root, prints each of steps."""
        status, output = make(root, '-n', goal)
        self.assertEqual(status, 0, output)
        for step in steps:
            self.assertIn(step, output)


# the tools lint and format install from WHEELS
CLANG_TOOLS = {'tools/llvm/bin/clang-format': '', 'tools/llvm/bin/clang-tidy': ''}
# the repository's tools/SHA256SUMS's lines for the wheels of clang-format and clang-tidy, in that order, and the wheels
WHEEL_SUMS = ''.join(line for line in (ROOT / 'tools/SHA256SUMS').read_text().splitlines(True)
                     if line.endswith('.whl\n'))
WHEELS = [line.split()[1] for line in WHEEL_SUMS.splitlines()]
# an install make takes for the one WHEELS pin: both tools, and WHEEL_SUMS in tools/SHA256SUMS and in the file an
# install writes them to last
INSTALLED = {**CLANG_TOOLS, 'tools/SHA256SUMS': WHEEL_SUMS, 'tools/llvm/wheels.txt': WHEEL_SUMS}
# lint's settings, among them modules/.clang-tidy, which clang-tidy reads for a.c and which inherits the root's; its
# tools, which make takes for installed; a.c's lint target, whose .d file names the header a.c includes; and the tags
# check's
LINT = {'.clang-format': '', '.clang-tidy': '', 'modules/.clang-tidy': 'InheritParentConfig: true\n',
        'compile_flags.txt': '-Imodules\n', **INSTALLED,
        'modules/mod/a.c': '#include "mod/a.h"\n', 'modules/mod/a.h': '', 'build/lint/modules/mod/a.c.tidy': '',
        'build/lint/modules/mod/a.c.d': ('build/lint/modules/mod/a.c.tidy: modules/mod/a.c modules/mod/a.h\n'
                                         'modules/mod/a.h:\n'),
        'build/lint/tags.ok': ''}
TIDY = 'tools/llvm/bin/clang-tidy --quiet modules/mod/a.c'
TAGS = 'python3 tools/tags.py src/main.c'


class Lint(unittest.TestCase):
    def test_reruns_a_check_whose_dependency_file_is_gone(self):
        # only the .d file names a.c's header to its lint target, so a lint target whose .d file is gone reruns, as an
        # object does, and a header changed since can't pass unchecked
        with checkout(LINT) as root:
            settle(root)
            status, output = make(root, '-n', 'lint')
            self.assertEqual(status, 0, output)
            self.assertNotIn(TIDY, output)
            (root / 'build/lint/modules/mod/a.c.d').unlink()
            os.utime(root / 'modules/mod/a.h')
            status, output = make(root, '-n', 'lint')
        self.assertEqual(status, 0, output)
        self.assertIn(TIDY, output)

    def test_reads_the_dependency_file_of_a_lint_target_named_alone(self):
        # make build/lint/modules/mod/a.c.tidy, without lint, still reruns a.c's check when its header changes
        with checkout(LINT) as root:
            settle(root)
            status, output = make(root, '-n', 'build/lint/modules/mod/a.c.tidy')
            self.assertEqual(status, 0, output)
            self.assertNotIn(TIDY, output)
            os.utime(root / 'modules/mod/a.h')
            status, output = make(root, '-n', 'build/lint/modules/mod/a.c.tidy')
        self.assertEqual(status, 0, output)
        self.assertIn(TIDY, output)

    # each file a check reads that no .d file names, and the check a change to it reruns
    CHANGED = [
        ('compile_flags.txt', TIDY),
        ('.clang-tidy', TIDY),
        ('modules/.clang-tidy', TIDY),
        ('config/mod/splits.txt', TAGS),
    ]

    def test_reruns_the_check_a_changed_file_feeds(self):
        with checkout(LINT) as root:
            settle(root)
            status, output = make(root, '-n', 'lint')
            self.assertEqual(status, 0, output)
            self.assertEqual([check for check in (TIDY, TAGS) if check in output], [], output)
            for path, check in self.CHANGED:
                with self.subTest(path):
                    os.utime(root / path)
                    try:
                        status, output = make(root, '-n', 'lint')
                        self.assertEqual(status, 0, output)
                        self.assertIn(check, output)
                    finally:
                        settle(root)


# the tools make downloads on first use, in place
PINNED = {'tools/wibo-i686': '', **{f'tools/shc-v5.0{release}/shc.exe': '' for release in ('r10', 'r26', 'r32')}}


def python(setup):
    """A python3 that runs as this one does, but runs the statement setup
    before the code -c gives it."""
    return (f'#!{sys.executable}\n'
            'import os, sys\n'
            "if sys.argv[1:2] != ['-c']:\n"
            '    os.execv(sys.executable, [sys.executable, *sys.argv[1:]])\n'
            'code = sys.argv[2]\n'
            "sys.argv = ['-c', *sys.argv[3:]]\n"
            f'{setup}\n'
            "exec(compile(code, '<string>', 'exec'), {'__name__': '__main__'})\n")


class Host(unittest.TestCase):
    def test_names_each_command_a_build_lacks_before_any_step(self):
        # with no pinned tool in place, each goal that builds runs curl to download them, and tar and gzip to
        # unpack the SHC releases
        with checkout() as root, host(*COMMANDS) as path:
            for goals in ((), ('check',), ('progress',), ('relink',), ('shift',)):
                with self.subTest(goals=goals):
                    status, output = make(root, *goals, 'CC=no-such-cc', path=path)
                    self.assertFalse((root / 'build/deps.mk').exists())
                    self.assertNotEqual(status, 0, output)
                    self.assertIn('*** no-such-cc sha256sum timeout curl tar gzip: not found; see README.md#build',
                                  output)

    def test_needs_no_download_command_once_the_tools_are_in_place(self):
        with checkout(PINNED) as root, host('curl', 'tar', 'gzip') as path:
            status, output = make(root, 'check', 'CC=no-such-cc', path=path)
        self.assertNotEqual(status, 0, output)
        self.assertIn('*** no-such-cc: not found; see README.md#build', output)

    def test_needs_tar_and_gzip_for_an_shc_release_alone(self):
        releases = {path: text for path, text in PINNED.items() if path.endswith('/shc.exe')}
        with checkout(releases) as root, host('curl', 'tar', 'gzip') as path:
            status, output = make(root, 'check', 'CC=no-such-cc', path=path)
        self.assertNotEqual(status, 0, output)
        self.assertIn('*** no-such-cc curl: not found; see README.md#build', output)

    def test_lint_needs_the_c_compiler_alone_once_its_tools_are_in_place(self):
        with checkout(INSTALLED) as root, host(*COMMANDS) as path:
            status, output = make(root, 'lint', 'CC=no-such-cc', path=path)
        self.assertNotEqual(status, 0, output)
        self.assertIn('*** no-such-cc: not found; see README.md#build', output)

    def test_lint_and_format_need_sha256sum_and_curl_to_download_their_wheels(self):
        # with no install, and with an install of the wheels as tools/SHA256SUMS gave them other hashes
        other = {**INSTALLED, 'tools/llvm/wheels.txt': ''.join(f'{HASH}  {wheel}\n' for wheel in WHEELS)}
        for installed in ({}, other):
            with checkout(installed) as root, host(*COMMANDS) as path:
                for goal, lacks in (('lint', 'no-such-cc sha256sum curl'), ('format', 'sha256sum curl')):
                    with self.subTest(installed=bool(installed), goal=goal):
                        status, output = make(root, goal, 'CC=no-such-cc', path=path)
                        self.assertNotEqual(status, 0, output)
                        self.assertIn(f'*** {lacks}: not found; see README.md#build', output)

    def test_lint_and_format_need_ensurepip_to_install_their_tools(self):
        # the install runs python3 -m venv, which needs ensurepip, and this python3 lacks it, as Debian's and
        # Ubuntu's do without python3-venv; an install in place runs no venv. With curl gone, no make downloads
        no_ensurepip = {'bin/python3': python("sys.modules['ensurepip'] = None")}
        for installed, goal, lacks in (({}, 'lint', 'no-such-cc curl python3-venv'),
                                       ({}, 'format', 'curl python3-venv'), (INSTALLED, 'lint', 'no-such-cc')):
            with self.subTest(installed=bool(installed), goal=goal), checkout({**installed, **no_ensurepip}) as root, \
                    host('curl') as path:
                (root / 'bin/python3').chmod(0o755)
                status, output = make(root, goal, 'CC=no-such-cc', path=f'{root / "bin"}{os.pathsep}{path}')
                self.assertNotEqual(status, 0, output)
                self.assertIn(f'*** {lacks}: not found; see README.md#build', output)

    def test_a_dry_run_needs_none_of_the_commands(self):
        with checkout() as root, host(*COMMANDS) as path:
            status, output = make(root, '-n', 'check', 'CC=no-such-cc', path=path)
        self.assertEqual(status, 0, output)

    def test_names_a_make_without_grouped_targets(self):
        # GNU make 4.3 added grouped targets, and grouped-target in .FEATURES with them
        with checkout() as root:
            status, output = make(root, '-n', 'check', '.FEATURES=target-specific order-only')
        self.assertNotEqual(status, 0, output)
        self.assertIn('the build needs GNU make 4.3 or later, and this is ', output)

    def test_names_a_python_before_3_8_and_takes_3_8(self):
        for version, needed in (((3, 7, 99, 'final', 0), True), ((3, 8, 0, 'final', 0), False)):
            versioned = {'bin/python3': python(f'sys.version_info = {version!r}')}
            with self.subTest(version=version), checkout(versioned) as root:
                (root / 'bin/python3').chmod(0o755)
                status, output = make(root, '-n', 'test', path=f'{root / "bin"}{os.pathsep}{os.environ["PATH"]}')
                self.assertEqual(status != 0, needed, output)
                self.assertEqual('the build needs Python 3.8 or later, as python3' in output, needed, output)


# a file in each directory the Makefile searches for sources, so that none of its scans prints an error
SEARCHED = {f'{root}/.gitkeep': '' for root in ROOTS}
# the goals a person types and the variables a person sets
GOALS = {'all', 'relink', 'shift', 'check', 'test', 'lint', 'review', 'format', 'progress', 'clean', 'help',
         'stub-includes'}
VARIABLES = {'V', 'CC', 'WIBO', 'R10', 'R26', 'R32'}


def written(root):
    """Each path make wrote under root's build/ but Python's bytecode. With the pinned tools in place, a goal
    that compiles writes build/deps.mk and stamps each tool."""
    return {path.relative_to(root) for path in (root / 'build').rglob('*') if 'pycache' not in path.parts}


class Help(unittest.TestCase):
    def test_names_each_goal_then_each_variable_beside_its_line(self):
        with checkout({**PINNED, **SEARCHED}) as root:
            status, output = make(root, 'help')
            self.assertEqual(written(root), set())
        self.assertEqual(status, 0, output)
        rows = [line.split(None, 1) for line in output.splitlines()]
        self.assertEqual(len({len(line) - len(row[1]) for line, row in zip(output.splitlines(), rows)}), 1, output)
        # a goal's line is the ## line directly above its rule, and a variable's is its ## line after NAME=form:
        text = (ROOT / 'Makefile').read_text()
        goals = [[goal, does] for does, goal in re.findall(r'^## (.+)\n([a-z-]+):', text, re.M)]
        variables = [list(row) for row in re.findall(r'^## ([A-Z0-9]+=\S+): (.+)$', text, re.M)]
        self.assertEqual(rows, goals + variables, output)
        self.assertEqual({goal for goal, _ in goals}, GOALS)
        self.assertLessEqual(max(map(len, output.splitlines())), 80, output)
        self.assertEqual({name.split('=')[0] for name, _ in variables}, VARIABLES)


# an assembly source for b.src, whose one function's @asm tag is open
OPEN = ('        .EXPORT     _b\n\n        .SECTION    P,CODE,ALIGN=4\n\n'
        '; @asm open: every instruction is one SHC writes for C\n_b:\n        RTS\n        NOP\n        .END\n')


class Review(unittest.TestCase):
    def test_lists_the_open_assembly_and_writes_nothing_under_build(self):
        with checkout({**PINNED, **SEARCHED, 'modules/mod/b.src': OPEN}) as root:
            status, output = make(root, 'review')
            self.assertEqual(written(root), set())
        self.assertEqual(status, 0, output)
        self.assertEqual(output.splitlines()[0], 'modules/mod/b.src:6: _b: every instruction is one SHC writes for C')


class Clean(unittest.TestCase):
    def test_refuses_another_goal_and_removes_build_alone(self):
        # make -j runs the goals at once, so rm -rf build would race the other goal's writes
        with checkout({'build/kept': ''}) as root:
            for goals in (('clean', 'check'), ('check', 'clean')):
                with self.subTest(goals=goals):
                    status, output = make(root, '-n', *goals)
                    self.assertNotEqual(status, 0, output)
                    self.assertRegex(output, r'Makefile:\d+: \*\*\* make clean takes no other goal\.  Stop\.\n\Z')
            status, output = make(root, 'clean')
            self.assertEqual(status, 0, output)
            self.assertFalse((root / 'build').exists())


class Roots(unittest.TestCase):
    def test_names_the_roots_config_names(self):
        # the Makefile's scans read ROOTS, and tools/deps.py lists the sources under config.ROOTS
        with checkout(SEARCHED) as root:
            status, output = make(root, '-n', '-p', 'help')
        self.assertEqual(status, 0, output[-3000:])
        roots = [line.split(' := ', 1)[1].split() for line in output.splitlines() if line.startswith('ROOTS := ')]
        self.assertEqual(roots, [list(ROOTS)])


class Tables(unittest.TestCase):
    def test_reads_a_gen_row_as_config_does(self):
        # tools/config.py splits a row at runs of whitespace, tabs as well as blanks, and skips any before its first
        # field. make check builds no gen/ range's link
        row = '\tgen/mod/d_0c900110.c\tD\t0x0c900110\t0x0c900120\n'
        with checkout({'config/mod/splits.txt': MODULE['config/mod/splits.txt'] + row}) as root:
            status, output = make(root, '-n', 'check')
            self.assertEqual(status, 0, output)
            self.assertNotIn('build/mod/gen/mod/d_0c900110.bin', (root / 'build/checked.txt').read_text().split())


class Objects(unittest.TestCase):
    def test_stops_on_a_c_source_and_an_assembly_source_of_one_stem(self):
        # every image links build/<stem>.obj, so x.c and x.src would build one object, whether two images place them
        # or mod places x.c beside an unplaced x.src
        first = MODULE['config/1st_read/splits.txt'] + 'sdk/ninja/x.c  P  0x8c0101b0  0x8c0101c0\n'
        for case, files in (('two images', {**placing('sdk/ninja/x.src'), 'config/1st_read/splits.txt': first}),
                            ('unplaced', placing('sdk/ninja/x.c'))):
            with self.subTest(case), checkout({**files, 'sdk/ninja/x.c': '', 'sdk/ninja/x.src': ''}) as root:
                status, output = make(root, '-n', 'check')
                self.assertNotEqual(status, 0, output)
                self.assertIn('sdk/ninja/x has a .c and a .src, which would build one object', output)

    def test_lets_a_donor_share_an_assembly_sources_stem(self):
        # a donor builds no object of its own, so build/sdk/ninja/x.obj is x.src's alone
        with checkout({**placing('sdk/ninja/x.src'), 'config/compilers.mk': f'{COMPILERS}DONOR_C := sdk/ninja/x.c\n',
                       'sdk/ninja/x.c': '', 'sdk/ninja/x.src': ''}) as root:
            status, output = make(root, '-n', 'check')
        self.assertEqual(status, 0, output)


# the originals orig/ takes, each holding its own name
ORIGINALS = {f'orig/{name}': name.encode()
             for name in ('1ST_READ.BIN', 'SONICADV/MOD.PRS', *(f'SONICADV/{alias}.PRS' for alias in EVENT_ALIASES))}


class Originals(unittest.TestCase):
    # orig/ takes six originals: 1ST_READ.BIN, MOD.PRS and the four events that repeat another
    TREE = SEARCHED
    HOW = ("  Put them in orig/ as the disc names them, or put the disc's .gdi and its tracks there.\n"
           '  Without the disc, make check checks the linked sources it can build.\n')

    def test_stops_without_the_disc_before_any_step(self):
        with checkout(self.TREE) as root:
            status, output = make(root, '-n')
        self.assertNotEqual(status, 0, output)
        none = 'orig/ holds none of the 6 files the build needs: 1ST_READ.BIN and 5 under SONICADV/.\n'
        self.assertRegex(output, rf'\A{re.escape(none + self.HOW)}Makefile:\d+: \*\*\* no disc in orig/\.  Stop\.\n\Z')

    def test_names_the_first_three_files_orig_lacks_and_counts_the_rest(self):
        held = ('1ST_READ.BIN', 'SONICADV/MOD.PRS', 'SONICADV/EV0111.PRS', 'SONICADV/EV0177.PRS', 'SONICADV/EV017B.PRS')
        for present, lacks in (
                (held[:1], '5 of the 6 files the build needs: SONICADV/MOD.PRS, SONICADV/EV0054.PRS, '
                           'SONICADV/EV0111.PRS and 2 more'),
                (held[:3], '3 of the 6 files the build needs: SONICADV/EV0054.PRS, SONICADV/EV0177.PRS and '
                           'SONICADV/EV017B.PRS'),
                (held, '1 of the 6 files the build needs: SONICADV/EV0054.PRS')):
            # make relink and make shift build the images too
            for goals in ((), ('relink',), ('shift',)):
                with self.subTest(lacks=lacks, goals=goals), \
                        checkout({**self.TREE, **{f'orig/{f}': '' for f in present}}) as root:
                    status, output = make(root, '-n', *goals)
                    self.assertNotEqual(status, 0, output)
                    # before any compile, as with none of them
                    self.assertIn(f'\norig/ lacks {lacks}.\n{self.HOW}', f'\n{output}')
                    self.assertNotIn('shc.exe', output.split('orig/ lacks')[0])
                    self.assertRegex(output, r'\nMakefile:\d+: \*\*\* incomplete disc in orig/\.  Stop\.\n\Z')

    def test_extracts_from_a_gdi_before_it_compiles(self):
        # extraction checks each file it extracts, so a wrong .gdi fails before any compile
        with checkout({'orig/disc.gdi': ''}) as root:
            status, output = make(root, '-n')
        lines = output.splitlines()
        extract = next(index for index, line in enumerate(lines) if line.startswith('build/gdi extract'))
        compiling = next(index for index, line in enumerate(lines) if 'shc.exe' in line)
        self.assertLess(extract, compiling, output)

    def test_extracts_from_a_gdi_whose_name_holds_quotes_or_a_dollar(self):
        # a .gdi takes whatever name its owner gives it; build/gdi's stand-in finds the image by that name, and writes
        # each file it is asked for as ORIGINALS holds it
        extract = ('#!/bin/sh\n[ "$1" = extract ] && [ -f "$2" ] || exit 1\ndir=$3\nshift 3\n'
                   'for f; do mkdir -p "$dir/$(dirname "$f")" && printf %s "$f" > "$dir/$f" || exit 1; done\n')
        sums = ''.join(f'{hashlib.sha256(data).hexdigest()}  {path}\n' for path, data in sorted(ORIGINALS.items()))
        for gdi in ("orig/Sonic Adventure (Tails' copy).gdi", 'orig/disc $HOME "two" `x`.gdi'):
            with self.subTest(gdi=gdi), checkout({**SEARCHED, gdi: '', 'build/gdi': extract,
                                                  'build/tools/vendored.ok': '', 'SHA256SUMS': sums}) as root:
                (root / 'build/gdi').chmod(0o755)
                settle(root)
                status, output = make(root, 'unpack')
                self.assertEqual(status, 0, output)
                self.assertEqual(output, f'  gdi    {gdi}\n')
                self.assertEqual({path: (root / path).read_bytes() for path in ORIGINALS}, ORIGINALS)


class Relink(unittest.TestCase):
    def test_links_each_image_whole_after_its_splice(self):
        with checkout({**SEARCHED, **ORIGINALS}) as root:
            status, output = make(root, '-n', 'relink')
        self.assertEqual(status, 0, output)
        lines = [line.split() for line in output.splitlines()]
        for name, image in (('1st_read', 'build/1ST_READ.BIN'), ('mod', 'build/SONICADV/MOD.BIN')):
            with self.subTest(name=name):
                spliced = next(at for at, line in enumerate(lines) if line[:2] == ['python3', 'tools/splice.py']
                               and image in line)
                self.assertLess(spliced, lines.index(['python3', 'tools/relink.py', name, image]), output)

    def test_make_links_no_image_whole(self):
        # relink checks beside the splice, never in its place
        with checkout({**SEARCHED, **ORIGINALS}) as root:
            status, output = make(root, '-n')
        self.assertEqual(status, 0, output)
        self.assertIn('tools/splice.py', output)
        self.assertNotIn('tools/relink.py', output)

    def test_lists_the_stand_ins_only_under_v_1(self):
        with checkout({**SEARCHED, **ORIGINALS}) as root:
            plain = make(root, '-n', 'relink')
            off = make(root, '-n', 'relink', 'V=0')
            verbose = make(root, '-n', 'relink', 'V=1')
        for status, output in (plain, off, verbose):
            self.assertEqual(status, 0, output)
        command = ['python3', 'tools/relink.py', 'mod', 'build/SONICADV/MOD.BIN']
        for _, output in (plain, off):
            self.assertIn(command, [line.split() for line in output.splitlines()])
        self.assertIn([*command, '--verbose'], [line.split() for line in verbose[1].splitlines()])

    def test_links_each_image_whole_again_once_its_tools_change(self):
        with checkout({**SEARCHED, **ORIGINALS}) as root:
            status, output = make(root, '-n', '-p', 'relink')
        self.assertEqual(status, 0, output[-3000:])
        rules = dict(line.split(':', 1) for line in output.splitlines()
                     if line.startswith(('build/relink/1st_read.bin:', 'build/relink/mod.bin:')))
        self.assertEqual(len(rules), 2, output[-3000:])
        for rule in rules.values():
            for tool in ('tools/relink.py', 'tools/link.py', 'tools/config.py', 'tools/code_ranges.py'):
                self.assertIn(tool, rule.split())

    def test_shifts_each_image_once_its_whole_link_matches_its_splice(self):
        with checkout({**SEARCHED, **ORIGINALS}) as root:
            status, output = make(root, '-n', 'shift')
            # make relink shifts none
            relinked = make(root, '-n', 'relink')
        self.assertEqual(status, 0, output)
        self.assertEqual(relinked[0], 0, relinked[1])
        self.assertNotIn('--shift', relinked[1])
        lines = [line.split() for line in output.splitlines()]
        for name, image in (('1st_read', 'build/1ST_READ.BIN'), ('mod', 'build/SONICADV/MOD.BIN')):
            with self.subTest(name=name):
                # the shift runs after the image's relink, and checks the link it rewrites against the splice too
                self.assertLess(lines.index(['python3', 'tools/relink.py', name, image]),
                                lines.index(['python3', 'tools/relink.py', name, image, '--shift']), output)
        self.assertEqual(lines[-2], ['cat', 'build/relink/1st_read/fixed.txt', 'build/relink/mod/fixed.txt', '>',
                                     'build/relink/fixed.txt'])

    def test_shifts_each_image_again_once_its_reasons_change(self):
        with checkout({**SEARCHED, **ORIGINALS}) as root:
            status, output = make(root, '-n', '-p', 'shift')
        self.assertEqual(status, 0, output[-3000:])
        rules = dict(line.split(':', 1) for line in output.splitlines()
                     if line.startswith(('build/relink/1st_read/fixed.txt:', 'build/relink/mod/fixed.txt:')))
        self.assertEqual(len(rules), 2, output[-3000:])
        for rule in rules.values():
            self.assertIn('config/kept.txt', rule.split())


# a curl that writes "wheel" to the file -o names
CURL = '#!/bin/sh\nwhile [ $# -gt 1 ]; do [ "$1" = -o ] && out=$2; shift; done\nprintf wheel > "$out"\n'


def listing(wheels):
    """tools/SHA256SUMS's lines for wheels, each with the hash of CURL's file."""
    return ''.join(f'{hashlib.sha256(b"wheel").hexdigest()}  {wheel}\n' for wheel in wheels)


class Downloads(unittest.TestCase):
    def test_names_each_download_as_it_starts(self):
        # MODULE's sources need R32 and wibo alone; tools/SHA256SUMS lists neither, so make refuses each before curl
        # runs, and -k goes on to the next. CC=true passes the host check, as nothing here compiles
        with checkout(SEARCHED) as root, host() as path:
            status, output = make(root, '-k', 'check', 'CC=true', path=path)
        self.assertNotEqual(status, 0, output)
        for download in ('tools/shc-v5.0r32.tar.gz', 'tools/wibo-i686'):
            self.assertIn(f'  fetch  {download}\n{download}: not in tools/SHA256SUMS\n', output)

    def test_moves_a_wheel_into_place_only_once_tools_sha256sums_lists_its_hash(self):
        # tools/SHA256SUMS gives clang-format's wheel the hash of CURL's file, and clang-tidy's no line, a line only
        # for a longer path, or another hash, so make moves the first into place, refuses the second and installs
        # neither
        for second, refusal in (('', f'{WHEELS[1]}: not in tools/SHA256SUMS\n'),
                                (listing([f'{WHEELS[1]}.old']), f'{WHEELS[1]}: not in tools/SHA256SUMS\n'),
                                (f'{HASH}  {WHEELS[1]}\n', f'{WHEELS[1]}.part: FAILED\n')):
            with self.subTest(refusal=refusal), \
                    checkout({**SEARCHED, 'bin/curl': CURL, 'tools/SHA256SUMS': listing(WHEELS[:1]) + second}) as root:
                (root / 'bin/curl').chmod(0o755)
                status, output = make(root, 'format', path=f'{root / "bin"}{os.pathsep}{os.environ["PATH"]}')
                self.assertNotEqual(status, 0, output)
                self.assertIn(refusal, output)
                self.assertEqual((root / WHEELS[0]).read_bytes(), b'wheel')
                self.assertFalse((root / WHEELS[1]).exists())
                self.assertFalse((root / 'tools/llvm/bin').exists())

    def test_installs_clang_format_and_clang_tidy_from_their_wheels_alone_then_removes_the_wheels(self):
        # pip reads no index, and installs no package a wheel depends on; nothing reads a wheel once it is installed
        with checkout(SEARCHED) as root:
            status, output = make(root, '-n', 'format')
        self.assertEqual(status, 0, output)
        wheels = ' '.join(WHEELS)
        self.assertIn(f'pip install -q --no-index --no-deps --force-reinstall {wheels}\nrm {wheels}\n', output)


# a pip that installs each wheel's tool, named and versioned by the wheel's file name, and leaves a wheel installed at
# its version alone unless given --force-reinstall, as pip does. It adds its command line to its environment's pip.log
PIP = f'#!{sys.executable}\n' + '''import sys
from pathlib import Path
env = Path(sys.argv[0]).parents[1]
with open(env / 'pip.log', 'a') as log:
    print(*sys.argv[1:], file=log)
for wheel in [Path(arg).name for arg in sys.argv[1:] if arg.endswith('.whl')]:
    name, version = wheel.split('-')[:2]
    installed = env / 'lib' / name
    if '--force-reinstall' in sys.argv or not installed.exists() or installed.read_text() != version:
        installed.parent.mkdir(exist_ok=True)
        installed.write_text(version)
        tool = env / 'bin' / name.replace('_', '-')
        tool.write_text(f'#!/bin/sh\\n# {wheel}\\n')
        tool.chmod(0o755)
'''
# a python3 that runs as this one does, but for -m venv DIR, which makes DIR an environment whose pip is PIP, keeps one
# already there, as venv keeps its packages, and refuses a DIR that is a link, as venv does
VENV = f'#!{sys.executable}\n' + f'''import os, sys
if sys.argv[1:3] != ['-m', 'venv']:
    os.execv(sys.executable, [sys.executable, *sys.argv[1:]])
if os.path.islink(sys.argv[3]):
    sys.exit('Error: Unable to create directory ' + repr(sys.argv[3]))
pip = os.path.join(sys.argv[3], 'bin/pip')
if not os.path.exists(pip):
    os.makedirs(os.path.dirname(pip), exist_ok=True)
    with open(pip, 'w') as file:
        file.write({PIP!r})
    os.chmod(pip, 0o755)
'''

# WHEELS as a later release names them
LATER = [wheel.replace(f'-{wheel.split("-")[1]}-', '-99.0.0-') for wheel in WHEELS]


def pin(root, wheels):
    """Pin root's Makefile to wheels in WHEELS' place, and list them in its tools/SHA256SUMS."""
    text = (root / 'Makefile').read_text()
    for wheel, other in zip(WHEELS, wheels):
        text = text.replace(wheel, other)
    (root / 'Makefile').write_text(text)
    (root / 'tools/SHA256SUMS').write_text(listing(wheels))


def pips(root):
    """How many times make has run the pip of root's tools/llvm."""
    log = root / 'tools/llvm/pip.log'
    return len(log.read_text().splitlines()) if log.exists() else 0


class Install(unittest.TestCase):
    def test_installs_the_wheels_again_when_a_tool_is_gone(self):
        # pip finds each wheel installed, so only a forced install brings the tool back
        for gone in CLANG_TOOLS:
            with self.subTest(gone=gone), self.stood_in() as (root, path):
                self.assertInstalls(root, path, WHEELS)
                (root / gone).unlink()
                self.assertInstalls(root, path, WHEELS)

    def test_installs_the_wheels_a_changed_pin_names(self):
        # the Makefile names a later release's wheels, which tools/SHA256SUMS lists
        with self.stood_in() as (root, path):
            self.assertInstalls(root, path, WHEELS)
            pin(root, LATER)
            self.assertInstalls(root, path, LATER)

    def test_fetches_the_wheels_again_when_a_hash_changes_and_a_refused_one_leaves_the_install(self):
        # tools/SHA256SUMS gives clang-tidy's wheel another hash, so make fetches it again and refuses CURL's file.
        # That stops the install before pip, so the one there keeps its record: with the hash back, make runs no pip
        with self.stood_in() as (root, path):
            self.assertInstalls(root, path, WHEELS)
            (root / 'tools/SHA256SUMS').write_text(listing(WHEELS[:1]) + f'{HASH}  {WHEELS[1]}\n')
            status, output = make(root, 'format', path=path)
            self.assertNotEqual(status, 0, output)
            self.assertIn(f'  fetch  {WHEELS[1]}\n{WHEELS[1]}.part: FAILED\n', output)
            (root / 'tools/SHA256SUMS').write_text(listing(WHEELS))
            status, output = make(root, 'format', path=path)
            self.assertEqual(status, 0, output)
            self.assertEqual(pips(root), 1, output)

    def test_leaves_a_linked_install_alone_when_venv_refuses_the_link(self):
        # tools/llvm links an install of WHEELS, and the Makefile names LATER, so make fetches them and venv refuses the
        # link. That stops the install before pip, so the one linked keeps its tools and its record of WHEELS
        with self.stood_in() as (root, path):
            self.assertInstalls(root, path, WHEELS)
            (root / 'tools/llvm').rename(root / 'llvm')
            (root / 'tools/llvm').symlink_to(root / 'llvm')
            pin(root, LATER)
            status, output = make(root, 'format', path=path)
            self.assertNotEqual(status, 0, output)
            self.assertIn("Error: Unable to create directory 'tools/llvm'\n", output)
            self.assertEqual([(root / tool).exists() for tool in CLANG_TOOLS], [True, True])
            record = root / 'llvm/wheels.txt'
            self.assertEqual(record.read_text() if record.exists() else None, listing(WHEELS))
            self.assertEqual(pips(root), 1, output)

    def test_installs_the_wheels_again_after_pip_fails(self):
        # make runs a pip that fails: before it writes a tool, for LATER, or after it writes both, with clang-tidy
        # gone. The record went before pip ran and comes back only after an install, so either way the next make
        # installs the wheels
        for wheels, fails in ((LATER, 'exit 1'), (WHEELS, '"$0.real" "$@"; exit 1')):
            with self.subTest(fails=fails), self.stood_in() as (root, path):
                self.assertInstalls(root, path, WHEELS)
                pin(root, wheels)
                if wheels == WHEELS:
                    (root / 'tools/llvm/bin/clang-tidy').unlink()
                pip = root / 'tools/llvm/bin/pip'
                pip.rename(pip.with_name('pip.real'))
                pip.write_text(f'#!/bin/sh\n{fails}\n')
                pip.chmod(0o755)
                status, output = make(root, 'format', path=path)
                self.assertNotEqual(status, 0, output)
                pip.with_name('pip.real').replace(pip)
                self.assertInstalls(root, path, wheels)

    def test_names_a_tool_a_first_install_lacks_and_tries_again(self):
        # the Makefile pins a wheel of clang-tidy-extra in place of clang-tidy's, so a first install ends with no
        # clang-tidy and no record, and the next make tries it again
        lacking = [WHEELS[0], WHEELS[1].replace('/clang_tidy-', '/clang_tidy_extra-')]
        with self.stood_in() as (root, path):
            pin(root, lacking)
            for runs in (1, 2):
                status, output = make(root, 'format', path=path)
                self.assertNotEqual(status, 0, output)
                self.assertIn('tools/llvm/bin: no clang-tidy\n', output)
                self.assertEqual(pips(root), runs, output)

    @contextlib.contextmanager
    def stood_in(self):
        """A checkout with CURL and VENV for curl and python3, and the PATH that finds them."""
        with checkout({**SEARCHED, 'bin/curl': CURL, 'bin/python3': VENV, 'tools/SHA256SUMS': listing(WHEELS)}) as root:
            for stand_in in ('bin/curl', 'bin/python3'):
                (root / stand_in).chmod(0o755)
            yield root, f'{root / "bin"}{os.pathsep}{os.environ["PATH"]}'

    def assertInstalls(self, root, path, wheels):
        """make format, in root, installs each tool from its wheel of wheels, and make format again runs no pip."""
        for runs in (1, 0):
            ran = pips(root)
            status, output = make(root, 'format', path=path)
            self.assertEqual(status, 0, output)
            self.assertEqual([(root / tool).read_text() if (root / tool).exists() else None for tool in CLANG_TOOLS],
                             [f'#!/bin/sh\n# {Path(wheel).name}\n' for wheel in wheels])
            self.assertEqual(pips(root) - ran, runs, output)


# a tree in which make and make check have only their hashes left to check: the originals; MODULE's objects and
# links, and MOD.PRS's decoded and rebuilt files, built; SHA256SUMS's line for each original, link and decoded file,
# in path order; and the pinned tools in place. make check's splice.py --check needs every image's code written,
# so a stand-in takes its place that passes, and gives the tools that import the splicer its names; and its
# progress.py --check needs every image and README.md's numbers, so one that passes takes its place too
LISTED = {**ORIGINALS, 'build/1st_read/src/main.bin': bytes(16), 'build/mod/modules/mod/a.bin': bytes(32),
          'build/mod/modules/mod/b.bin': bytes(16), 'build/SONICADV/MOD.BIN': b'MOD'}
HASHED = {
    **PINNED, **SEARCHED, **LISTED, 'build/gen/img/MOD.BIN': b'MOD',
    'tools/splice.py': 'from spliced import *\n', 'tools/spliced.py': (ROOT / 'tools/splice.py').read_text(),
    'tools/progress.py': '',
    'SHA256SUMS': ''.join(f'{hashlib.sha256(data).hexdigest()}  {path}\n' for path, data in sorted(LISTED.items())),
    'build/1ST_READ.BIN': ORIGINALS['orig/1ST_READ.BIN'], 'build/SONICADV/MOD.PRS': ORIGINALS['orig/SONICADV/MOD.PRS'],
    **{path: '' for path in ('build/verified/1ST_READ.BIN.ok', 'build/verified/SONICADV/MOD.PRS.ok',
                             'build/tools/vendored.ok', 'build/prskit',
                             'build/src/main.obj', 'build/modules/mod/a.obj', 'build/modules/mod/b.obj',
                             'build/modules/mod/b.lis')},
    'build/src/main.d': depends('build/src/main.obj', 'src/main.c'),
    'build/modules/mod/a.d': depends('build/modules/mod/a.obj', 'modules/mod/a.c'),
}


class Sums(unittest.TestCase):
    def test_prints_one_line_for_the_files_that_match(self):
        # make checks SHA256SUMS's ten lines and the two files it rebuilds from an original, and make check the three
        # links; neither prints a command, or a line for each file. CC=true passes the host check, as nothing compiles
        for goals, count in (((), 12), (('check',), 3)):
            with self.subTest(goals=goals), self.built() as root:
                status, output = make(root, *goals, 'CC=true')
                self.assertEqual(status, 0, output)
                self.assertEqual(output, f'  sums   {count} files match SHA256SUMS\n')

    def test_names_a_file_that_differs_and_fails(self):
        for goals in ((), ('check',)):
            with self.subTest(goals=goals), self.built() as root:
                (root / 'build/mod/modules/mod/b.bin').write_bytes(b'\x01' + bytes(15))
                # dated with the rest, so that make doesn't splice MOD.PRS again
                settle(root)
                status, output = make(root, *goals, 'CC=true')
                self.assertNotEqual(status, 0, output)
                self.assertIn('build/mod/modules/mod/b.bin: FAILED\n'
                              'sha256sum: WARNING: 1 computed checksum did NOT match\n', output)
                self.assertNotIn('files match', output)

    @contextlib.contextmanager
    def built(self, *args, files=HASHED):
        """The checkout of files, HASHED unless given, settled once make check with args has stamped the tools and
        written build/deps.mk."""
        with checkout(files) as root:
            status, output = make(root, '-n', 'check', *args)
            self.assertEqual(status, 0, output)
            settle(root)
            yield root


class Stamps(unittest.TestCase):
    def test_stops_on_a_tool_it_can_t_hash(self):
        # R32's shc.exe changed, beside a directory sha256sum can't read
        with self.built() as root:
            (root / 'tools/shc-v5.0r32/shc.exe').write_text('another')
            (root / 'tools/shc-v5.0r32/notes').mkdir()
            status, output = make(root, 'check', 'CC=true')
        self.assertNotEqual(status, 0, output)
        self.assertRegex(output, r'\Asha256sum: tools/shc-v5\.0r32/notes: Is a directory\n'
                                 r"Makefile:\d+: \*\*\* build/tools/r32\.id: can't refresh it from tools/shc-v5\.0r32/\*\."
                                 r'  Stop\.\n\Z')

    def test_stops_on_a_tool_outside_tools_that_isn_t_there_with_nothing_to_rebuild(self):
        with self.built() as root:
            self.assertEqual(make(root, 'check', 'CC=true'), (0, '  sums   3 files match SHA256SUMS\n'))
            status, output = make(root, 'check', 'R32=custom', 'CC=true')
        self.assertNotEqual(status, 0, output)
        self.assertRegex(output, r'\AMakefile:\d+: \*\*\* custom/shc\.exe: no such file\.  Stop\.\n\Z')

    def test_fetches_the_default_r32_after_an_override_and_compiles_with_it_in_the_same_run(self):
        # built with R32=custom, then checked with the default R32, which tools/ lacks: one make fetches it, stamps it
        # and compiles again with it
        files = {**{path: text for path, text in HASHED.items() if path != 'tools/shc-v5.0r32/shc.exe'},
                 'custom/shc.exe': ''}
        with self.built('R32=custom', files=files) as root:
            status, output = make(root, '-n', 'check', 'R32=custom')
            self.assertEqual(status, 0, output)
            self.assertNotIn('shc.exe', output)
            status, output = make(root, '-n', 'check')
        self.assertEqual(status, 0, output)
        steps = ('tools/shc-v5.0r32.tar.gz', 'id="$(sha256sum tools/shc-v5.0r32/*)"',
                 'tools/shc-v5.0r32/shc.exe src/main.c')
        for step in steps:
            self.assertIn(step, output)
        self.assertEqual(sorted(steps, key=output.index), list(steps))

    def test_compiles_nothing_again_with_a_tool_fetched_again_byte_for_byte(self):
        # wibo, CURL's file, gone from tools/ and fetched again
        files = {**HASHED, 'tools/wibo-i686': 'wheel', 'bin/curl': CURL,
                 'tools/SHA256SUMS': listing(['tools/wibo-i686'])}
        with self.built(files=files) as root:
            (root / 'bin/curl').chmod(0o755)
            (root / 'tools/wibo-i686').unlink()
            status, output = make(root, 'check', 'CC=true', path=f'{root / "bin"}{os.pathsep}{os.environ["PATH"]}')
        self.assertEqual((status, output), (0, '  fetch  tools/wibo-i686\n  sums   3 files match SHA256SUMS\n'))

    built = Sums.built


class Progress(unittest.TestCase):
    def test_reports_then_writes_the_numbers_and_check_checks_them(self):
        with checkout() as root:
            runs = {}
            for goal in ('progress', 'check'):
                status, output = make(root, '-n', goal)
                self.assertEqual(status, 0, output)
                runs[goal] = [line.split()[2] for line in output.splitlines()
                              if line.startswith('python3 tools/progress.py ')]
        self.assertEqual(runs, {'progress': ['--patched', '--write'], 'check': ['--check']})


class Vendored(unittest.TestCase):
    def test_checks_the_vendored_files_before_it_builds_a_tool(self):
        # tools/SHA256SUMS records none of the tools' files
        with checkout() as root:
            for tool in vendor.TOOLS:
                status, output = make(root, f'build/{tool}')
                self.assertNotEqual(status, 0, output)
                self.assertIn(f'vendor: tools/SHA256SUMS: no # REPOSITORY COMMIT line for {tool}', output)
                self.assertNotIn(f'  cc     build/{tool}', output)
                self.assertFalse((root / 'build' / tool).exists())


def shells(text):
    """Each $(shell ...) in a makefile's text, through its closing parenthesis."""
    for start in (index for index in range(len(text)) if text.startswith('$(shell', index)):
        depth = 0
        for end in range(start + 1, len(text)):
            depth += {'(': 1, ')': -1}.get(text[end], 0)
            if not depth:
                yield text[start:end + 1]
                break


class Bytecode(unittest.TestCase):
    def test_puts_the_bytecode_of_python_a_shell_runs_under_build(self):
        # make before 4.4 passes no exported variable to $(shell), so each that runs Python names the prefix itself
        calls = [call for path in ('Makefile', 'config/compilers.mk') for call in shells((ROOT / path).read_text())
                 if 'python3' in call]
        self.assertTrue(calls)
        for call in calls:
            self.assertRegex(call, r'PYTHONPYCACHEPREFIX=\$\(PYTHONPYCACHEPREFIX\) python3 ')


class Quiet(unittest.TestCase):
    def test_hides_each_command_unless_v_is_set(self):
        # Q is @ unless V is set, so each command a recipe runs opens with it; a status line prints under either, and
        # a make function that runs no command needs neither
        text = (ROOT / 'Makefile').read_text().replace('\\\n', ' ')
        for line in re.findall(r'^\t.*', text, re.M):
            self.assertRegex(line, r'^\t\+?\$\$?\(Q\)|^\t@\$\$?\(call status,|^\t\$\((file|if|info|error) ')


if __name__ == '__main__':
    unittest.main()
