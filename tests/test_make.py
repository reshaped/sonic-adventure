import contextlib
import fnmatch
import functools
import hashlib
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time
import unittest
from pathlib import Path
from unittest import mock

import offsets
import parts
import support
import vendor
from images import ROOTS
from support import (A_BUILT_TABLE, A_HELD, A_INCS, A_READS, A_TABLE, BODIES, BSDTAR, BUILT, CC, COMMANDS, COMPILERS,
                     CURL, FOUND, GNU_TAR, HASH, HASHED, HOLD, LIBLZMA, LINUX, LISTED, MACS, MAKE, MATCHING_GOALS,
                     MEASURE, MOD_DATA, MODULE, MOTION, NO_ROWS, ORIGINALS, PATH, PINNED, RAN, SEARCHED, SETTINGS, STUB_SHC,
                     TIMEOUT, UNLIMITED, WIBO, ZLIB, checkout, depends, host, listing, make, no_shc, path_with_bin,
                     placing, settle, stopped, tabled, tree)
from test_relink import definition, obj
from test_shc_listing import SIZE, directive, label

ROOT = Path(__file__).resolve().parents[1]
# the repository's makefiles, by path: the Makefile, make dc's rules, config/'s fragments and the engine's
MAKEFILES = ['Makefile', 'dc.mk',
             *sorted(path.relative_to(ROOT).as_posix() for pattern in ('config/*.mk', 'engine/**/*.mk')
                     for path in ROOT.glob(pattern))]


def tracked(*patterns):
    """The files git tracks that match patterns, so a cache or a scratch file left in the tree reads as nothing."""
    listed = subprocess.run(['git', 'ls-files', '-z', '--', *patterns], stdout=subprocess.PIPE, check=True,
                            cwd=ROOT).stdout
    return [path for path in listed.decode().split('\0') if path]


@functools.lru_cache(maxsize=None)
def database():
    """make's status and its database of the repository's own rules and variables, as make -pqn help prints them
    for LINUX."""
    return make(ROOT, '-pqn', 'help')


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

    def test_expects_a_line_for_each_motion_file(self):
        with checkout(MOTION) as root:
            status, output = make(root, '-n', 'check')
            self.assertEqual(status, 0, output)
            expected = set((root / 'build/expected.txt').read_text().split())
        self.assertIn('orig/SONICADV/X_MOT.PRS', expected)

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
        with checkout(tabled({'modules/mod/a.c': source})) as root:
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
        with checkout(tabled(files)) as root:
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
        status, output = database()
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
        with checkout(tabled(files)) as root:
            status, output = make(root, '-n', 'check')
        self.assertEqual(status, 0, output)
        lines = output.splitlines()
        for stem in ('build/stub/modules/mod/a', 'build/stub/sdk/ninja/s', 'build/stub/sdk/ninja/p', 'build/sdk/ninja/q',
                     'build/sdk/ninja/i', 'build/modules/mod/b'):
            tool = max(index for index, line in enumerate(lines)
                       if ('shc.exe' in line or 'asmsh.exe' in line) and f'{stem}.' in line.replace('\\', '/'))
            self.assertEqual(lines[tool + 1], f'touch {stem}.obj', stem)


# a tree of two sources with reads: a.c, a body source of mod's, which includes a header, its texts, its values and
# a body, and main.c, which 1ST_READ.BIN places. bin/cc stands in for the host's C compiler
READ = {**SEARCHED, 'bin/cc': CC, 'modules/mod/a.c': BUILT['modules/mod/a.c'], 'modules/mod/a.h': ''}
# main.c's reads, and the step that writes a.c's and main.c's, by the text make -n prints for it
MAIN_READS = 'build/reads/src/main.d'
WRITE_A_READS = f' -MT {A_READS} -MF {A_READS}.tmp modules/mod/a.c && '
WRITE_MAIN_READS = f' -MT {MAIN_READS} -MF {MAIN_READS}.tmp src/main.c && '
# what a.c's reads hold once a compiler that reads a.c writes them: a.c, its header and its three stub includes,
# each on a line of its own that a backslash continues, as a compiler wraps a long rule
A_NAMES = depends(A_READS, 'modules/mod/a.c', 'modules/mod/a.h',
                  *(f'build/stub/modules/mod/a/{inc}' for inc in A_INCS)).replace(' ', ' \\\n  ')


class Reads(unittest.TestCase):
    def test_names_reads_for_each_body_source_and_each_c_source_of_1st_read_bin_s_and_no_other(self):
        # a.c and t.c take a body, and 1ST_READ.BIN places t.c, main.c, an assembly source and a gen/ range. mod
        # places c.c, which takes no body. READS names a.c's, main.c's and t.c's, each once, and no rule writes
        # another source's
        files = {**SEARCHED, **MOD_DATA, **placing('modules/mod/c.c'), 'modules/mod/c.c': '', 'src/v.src': '',
                 'config/1st_read/splits.txt': (MODULE['config/1st_read/splits.txt'] +
                                                'src/t.c  P  0x8c0101b0  0x8c0101c0\n'
                                                'src/v.src  P  0x8c0101c0  0x8c0101d0\n'
                                                'gen/1st_read/d_8c180000.c  D  0x8c180000  0x8c180010\n'),
                 'config/1st_read/data.txt': '0x8c180000  words[4]\n',
                 **{f'{source}.c': f'static int a[1] = {{\n#include "{source.split("/", 1)[1]}/a.inc"\n}};\n'
                    for source in ('modules/mod/a', 'src/t')}}
        with checkout(files) as root:
            status, output = make(root, '-pqn', 'help')
            self.assertIn(status, (0, 1), output[-3000:])
            self.assertEqual(re.search(r'(?m)^READS := (.*)$', output)[1].split(),
                             [A_READS, MAIN_READS, 'build/reads/src/t.d'])
            for unread in ('build/reads/modules/mod/c.d', 'build/reads/src/v.d',
                           'build/reads/gen/1st_read/d_8c180000.d'):
                with self.subTest(unread):
                    status, output = make(root, '-n', unread)
                    self.assertNotEqual(status, 0, output)
                    self.assertIn(f"No rule to make target '{unread}'", output)

    def test_writes_a_source_s_reads_with_the_host_s_compiler_alone_on_any_host(self):
        # the command is the one that writes an SHC object's .d file, with SHCFLAGS' macro: for main.c with its
        # route's include directories, and for a.c with its stub's, once its zero body, its texts and its values are
        # in place. The tree holds no wibo and no SHC, and no step runs or fetches either, on Linux as on a Mac
        files = {**READ, 'config/compilers.mk': COMPILERS + 'SHCFLAGS := -cpu=sh4 -define=DREAMCAST\n'}
        searched = '-Isdk/include -Isrc -Imodules -Ievents -Ibuild/{0}/src -Ibuild/{0}/modules -Ibuild/{0}/events ' \
                   '-Ibuild/{0}/sdk'
        for uname in (LINUX, *MACS):
            with self.subTest(uname), scripted(files) as root:
                status, output = make(root, '-n', A_READS, MAIN_READS, 'CC=bin/cc', uname=uname)
                self.assertEqual(status, 0, output)
                for source, reads, inc in (('modules/mod/a.c', A_READS, 'stub'), ('src/main.c', MAIN_READS, 'gen')):
                    self.assertIn(f'bin/cc -MM -MP -nostdinc -DDREAMCAST {searched.format(inc)} -MT {reads} '
                                  f'-MF {reads}.tmp {source} && mv {reads}.tmp {reads}\n', output)
                for step in (ZERO, TEXTS, VALUES):
                    self.assertLess(output.index(step), output.index(WRITE_A_READS), output)
                for step in ('wibo', 'shc.exe', 'curl'):
                    self.assertNotIn(step, output)
                self.assertEqual(make(root, A_READS, MAIN_READS, 'CC=bin/cc', uname=uname), (0, ''))
                self.assertEqual((root / A_READS).read_text(), depends(A_READS, 'modules/mod/a.c'))
                self.assertEqual((root / MAIN_READS).read_text(), depends(MAIN_READS, 'src/main.c'))
                self.assertEqual(sorted(path.name for path in (root / 'build/stub/modules/mod/a').iterdir()),
                                 sorted(A_INCS))
                for unmade in ('build/stub/modules/mod/a.obj', 'build/tools/wibo.id', 'build/tools/r32.id', WIBO,
                               f'{A_READS}.tmp', f'{MAIN_READS}.tmp'):
                    self.assertFalse((root / unmade).exists(), unmade)
                status, output = make(root, A_READS, MAIN_READS, 'CC=bin/cc', uname=uname)
                self.assertEqual(status, 0, output)
                self.assertEqual(output.count('is up to date.\n'), 2, output)

    def test_writes_a_source_s_reads_again_once_a_file_they_name_or_what_gives_their_command_changes(self):
        # a.c's reads name a.c, its header and its three stub includes, and main.c's name main.c alone. Each is
        # written again once a file it names changes, its texts and values among a.c's, which tools/data.py writes
        # from a.c, or once the Makefile or config/compilers.mk does, which give the command. The other stays
        changed = [('modules/mod/a.h', [WRITE_A_READS]), ('build/stub/modules/mod/a/a.inc', [WRITE_A_READS]),
                   ('modules/mod/a.c', [TEXTS, VALUES, WRITE_A_READS]),
                   ('tools/data.py', [TEXTS, VALUES, WRITE_A_READS]), ('src/main.c', [WRITE_MAIN_READS]),
                   ('Makefile', [WRITE_A_READS, WRITE_MAIN_READS]),
                   ('config/compilers.mk', [WRITE_A_READS, WRITE_MAIN_READS])]
        with self.listed() as root:
            for path, steps in changed:
                with self.subTest(path):
                    os.utime(root / path)
                    try:
                        self.assertEqual(self.written_again(root), steps)
                    finally:
                        settle(root)

    def test_writes_reads_again_with_another_compiler_or_once_a_header_joins_or_leaves_roots_or_they_go(self):
        # CC's stamp holds its command, and build/headers.txt the headers under ROOTS, any of which a search could
        # find first. a.c's reads name the header that leaves, which stops no make
        with self.listed() as root:
            self.assertEqual(self.written_again(root, cc='bin/cc -DANOTHER'), [WRITE_A_READS, WRITE_MAIN_READS])
            settle(root)
            (root / 'modules/mod/n.h').write_text('')
            self.assertEqual(self.written_again(root), [WRITE_A_READS, WRITE_MAIN_READS])
            settle(root)
            (root / A_READS).unlink()
            self.assertEqual(self.written_again(root), [WRITE_A_READS])
            (root / A_READS).write_text(A_NAMES)
            settle(root)
            (root / 'modules/mod/a.h').unlink()
            self.assertEqual(self.written_again(root), [WRITE_A_READS, WRITE_MAIN_READS])

    def test_writes_no_reads_for_a_goal_that_compiles_nothing_or_under_make_n(self):
        # make reads each of the reads as rules, and includes none as a makefile: a makefile that a rule writes is
        # brought up to date as make starts. So with a header changed that a.c's reads name, make help plans and
        # writes no reads, and make -n plans a.c's and leaves them as they were
        with self.listed() as root:
            os.utime(root / 'modules/mod/a.h')
            dated = (root / A_READS).stat().st_mtime_ns
            for goals in (('help',), ('-n', 'help'), ('-n', 'clean')):
                with self.subTest(goals=goals):
                    status, output = make(root, *goals, 'CC=bin/cc')
                    self.assertEqual(status, 0, output)
                    self.assertNotIn('-MM', output)
                    self.assertEqual((root / A_READS).stat().st_mtime_ns, dated)
            self.assertEqual(self.written_again(root), [WRITE_A_READS])
            self.assertEqual(((root / A_READS).read_text(), (root / A_READS).stat().st_mtime_ns), (A_NAMES, dated))

    def test_each_source_s_reads_search_the_directories_its_shc_rule_gives_depend(self):
        # in the repository's own rules, as make's database gives them. A source's reads follow its stub where it has
        # one, and else its object: READS_INC is the include directories that rule's recipe hands depend
        status, output = database()
        self.assertIn(status, (0, 1), output[-3000:])
        reads = re.search(r'(?m)^READS := (.*)$', output)[1].split()
        searched = dict(re.findall(r'(?m)^(build/reads/\S+\.d): READS_INC = (.*)$', output))
        handed, target = {}, None
        for line in output.splitlines():
            if line.startswith('build/') and line.split(':')[0].endswith('.obj'):
                target = line.split(':')[0]
            elif not line:
                target = None
            elif target and line.startswith('\t') and '$(call depend,' in line:
                handed[target] = line.partition('$(call depend,')[2][:-1]
        self.assertGreater(len(reads), 1000)
        self.assertEqual(sorted(searched), sorted(reads))
        differ = []
        for path in reads:
            stem = path[len('build/reads/'):-len('.d')]
            followed = f'build/stub/{stem}.obj' if f'build/stub/{stem}.obj' in handed else f'build/{stem}.obj'
            if searched[path] != handed.get(followed):
                differ.append(path)
        self.assertEqual(differ, [])

    def written_again(self, root, cc='bin/cc'):
        """Of the steps that write a.c's texts, its values and its reads, and main.c's reads, those make -n prints
        for the two reads in root, with cc as the C compiler."""
        status, output = make(root, '-n', A_READS, MAIN_READS, f'CC={cc}')
        self.assertEqual(status, 0, output)
        return [step for step in (TEXTS, VALUES, WRITE_A_READS, WRITE_MAIN_READS) if step in output]

    @contextlib.contextmanager
    def listed(self):
        """READ's checkout once make has written the two reads, with a.c's as A_NAMES gives them, and settle() has
        dated what it wrote: make -n writes neither again."""
        with scripted(READ) as root:
            self.assertEqual(make(root, A_READS, MAIN_READS, 'CC=bin/cc'), (0, ''))
            (root / A_READS).write_text(A_NAMES)
            settle(root)
            self.assertEqual(self.written_again(root), [])
            yield root


# a.c as a source of two bodies, of types tools/data.py lays out by itself, and the rows its table of offsets takes
# from the listing of its stub build, which places the two at 0 and 8 in D
MEASURED_C = ('NJS_TEXLIST texlist = {\n#include "mod/a/texlist.inc"\n};\n'
              'Uint32 table[2] = {\n#include "mod/a/table.inc"\n};\n')
MEASURED_ROWS = 'datum  texlist  D  0x00000000  8\ndatum  table  D  0x00000008  8\n'
A_STUB_INCS = [f'build/stub/modules/mod/a/{name}.inc' for name in ('texlist', 'table')]
# a tree in which a.c's stub is built, with that listing, and a.c's reads are written, so that make has only a.c's
# table to measure and to hold. The pinned tools are in place
MEASURED = {
    **PINNED, **SEARCHED, 'modules/mod/a.c': MEASURED_C, 'build/stub/modules/mod/a.obj': '',
    'build/stub/modules/mod/a.d': depends('build/stub/modules/mod/a.obj', 'modules/mod/a.c', *A_STUB_INCS),
    'build/stub/modules/mod/a.lis': '\n'.join([label(0, 'texlist'), directive(0, "H'00000000,H'00000000"),
                                               label(8, 'table'), directive(8, "H'00000000,H'00000000"), SIZE]) + '\n',
    **{inc: '0\n' for inc in A_STUB_INCS}, A_READS: depends(A_READS, 'modules/mod/a.c', *A_STUB_INCS),
}
# what make offsets prints for a table it copies in, and for one it removes
COPIED, REMOVED = '  offset {}\n'.format, '  remove {}\n'.format


class Offsets(unittest.TestCase):
    def test_names_a_table_and_the_one_its_stub_build_gives_for_each_body_source_and_no_other(self):
        # a.c and t.c take a body, and c.c and main.c take none. No rule writes a table for a source that takes none,
        # and none writes a table under offsets/, which git tracks
        files = {**SEARCHED, **placing('modules/mod/c.c'), 'modules/mod/c.c': '',
                 'config/1st_read/splits.txt': (MODULE['config/1st_read/splits.txt'] +
                                                'src/t.c  P  0x8c0101b0  0x8c0101c0\n'),
                 **{f'{source}.c': f'static int a[1] = {{\n#include "{source.split("/", 1)[1]}/a.inc"\n}};\n'
                    for source in ('modules/mod/a', 'src/t')}}
        with checkout(tabled(files)) as root:
            status, output = make(root, '-pqn', 'help')
            self.assertIn(status, (0, 1), output[-3000:])
            self.assertEqual(re.search(r'(?m)^OFFSETS := (.*)$', output)[1].split(), [A_TABLE, 'offsets/src/t.txt'])
            self.assertEqual(re.search(r'(?m)^BUILT_OFFSETS := (.*)$', output)[1].split(),
                             [A_BUILT_TABLE, 'build/offsets/src/t.txt'])
            for unmade in ('build/offsets/modules/mod/c.txt', 'build/offsets/src/main.ok', A_TABLE,
                           'offsets/src/main.txt'):
                with self.subTest(unmade):
                    (root / unmade).unlink(missing_ok=True)
                    status, output = make(root, '-n', unmade)
                    self.assertNotEqual(status, 0, output)
                    self.assertNotIn('tools/data.py --offsets', output)

    def test_measures_a_table_from_the_stub_s_listing_and_the_source_s_reads(self):
        # the table a stub build gives waits for the stub, whose object stands for its listing, and for the source's
        # reads, which name the headers its types come from. tools/data.py --offsets then writes it, and reads
        # no image: the tree holds no original
        with checkout({**{path: text for path, text in UNBUILT.items() if not path.startswith('orig/')}, **PINNED}) \
                as root:
            status, output = make(root, '-n', A_BUILT_TABLE)
            self.assertEqual(status, 0, output)
            self.assertIn(f'\npython3 {MEASURE}\n', output)
            for step in (STUB_SHC, WRITE_A_READS):
                self.assertLess(output.index(step), output.index(MEASURE), output)
            for step in (HOLD, 'orig/', 'build/gen/'):
                self.assertNotIn(step, output)

    def test_measures_a_table_again_once_tools_data_py_or_a_module_it_imports_changes_and_compiles_no_stub(self):
        # a.c here has no texts and no values, which tools/data.py would write again for its stub. With the table
        # measured, make measures it no more until a tool changes
        with self.measured() as root:
            self.assertEqual(make(root, A_BUILT_TABLE, 'CC=true'), (0, ''))
            settle(root)
            for tool in (None, 'tools/data.py', 'tools/offsets.py', 'tools/shc_listing.py', 'tools/csource.py'):
                with self.subTest(tool):
                    if tool:
                        os.utime(root / tool)
                    status, output = make(root, '-n', A_BUILT_TABLE, 'CC=true')
                    self.assertEqual(status, 0, output)
                    self.assertEqual([step for step in (STUB_SHC, WRITE_A_READS, MEASURE) if step in output],
                                     [MEASURE] if tool else [], output)
                    settle(root)

    def test_holds_a_table_to_the_one_its_stub_build_gives_byte_for_byte(self):
        # make measures a.c's table from its stub's listing for real, and the check passes where the tracked table is
        # that one. It fails, naming the table and what writes it, on a table with another row, with no row, or with a
        # comment, and the next run checks again
        differs = (f'{A_TABLE}: differs from {A_BUILT_TABLE}, which modules/mod/a.c\'s stub build gives. make offsets '
                   'writes it on x86 Linux; see TOOLCHAIN.md#stub-and-full-builds\n')
        with self.measured({A_TABLE: MEASURED_ROWS}) as root:
            self.assertEqual(make(root, A_HELD, 'CC=true'), (0, ''))
            self.assertEqual((root / A_BUILT_TABLE).read_text(), MEASURED_ROWS)
            status, output = make(root, A_HELD, 'CC=true')
            self.assertEqual((status, output.count('is up to date.\n')), (0, 1), output)
            for text in (MEASURED_ROWS.replace('0x00000008  8', '0x00000008  4'), NO_ROWS,
                         MEASURED_ROWS + '# measured\n'):
                with self.subTest(text=text):
                    (root / A_TABLE).write_text(text)
                    for _ in range(2):
                        status, output = make(root, A_HELD, 'CC=true')
                        self.assertEqual(status, 2, output)
                        self.assertTrue(output.startswith(differs), output)
            (root / A_TABLE).write_text(MEASURED_ROWS)
            self.assertEqual(make(root, A_HELD, 'CC=true'), (0, ''))

    def test_check_waits_for_the_check_of_each_table_and_a_link_named_alone_for_none(self):
        with checkout({**UNBUILT, **PINNED}) as root:
            status, output = make(root, '-n', 'check')
            self.assertEqual(status, 0, output)
            self.assertLess(output.index(MEASURE), output.index(HOLD), output)
            status, output = make(root, '-n', 'build/stub/mod/modules/mod/a.bin')
            self.assertEqual(status, 0, output)
            self.assertIn(STUB_SHC, output)
            for step in (MEASURE, HOLD):
                self.assertNotIn(step, output)

    def test_a_body_source_with_no_table_stops_make_where_a_rule_reads_the_table_as_tools_offsets_py_names_it(self):
        # under make -n too. The Makefile's line is the one tools/offsets.py gives a missing table, so one text says
        # what a missing table is and what writes one. make offsets reads no table, and plans a.c's
        with tree({}):
            missing = offsets.missing(Path(A_TABLE), 'modules/mod/a.c')
        self.assertIn('missing; it records where modules/mod/a.c\'s data sits in the original. make offsets writes it '
                      'on x86 Linux; a renamed source takes a git mv; see TOOLCHAIN.md#stub-and-full-builds', missing)
        with self.measured() as root:
            for flags, goal in ((('-n',), 'check'), (('-n',), A_HELD), ((), A_HELD)):
                with self.subTest(flags=flags, goal=goal):
                    status, output = make(root, *flags, goal, 'CC=true')
                    self.assertEqual(status, 2, output)
                    self.assertRegex(output, rf'Makefile:\d+: \*\*\* {re.escape(missing)}\.  Stop\.\n\Z')
                    self.assertFalse((root / A_HELD).exists())
            status, output = make(root, '-n', 'offsets', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertIn(MEASURE, output)

    def test_check_stops_on_a_table_with_no_body_source_and_no_other_goal_does(self):
        # main.c takes no body, and no source is gone.c. make check names the two tables, in their order, and what
        # removes them. make progress, which builds what make check builds, goes on
        orphans = {'offsets/src/main.txt': NO_ROWS, 'offsets/modules/mod/gone.txt': NO_ROWS}
        with checkout({**UNBUILT, **PINNED, **orphans}) as root:
            status, output = make(root, '-n', 'check')
            self.assertEqual(status, 2, output)
            self.assertRegex(output, r'\nMakefile:\d+: \*\*\* offsets/modules/mod/gone\.txt offsets/src/main\.txt: a '
                                     r'table of offsets with no body source\. make offsets removes it on x86 Linux; a '
                                     r'renamed source takes a git mv; see TOOLCHAIN\.md#stub-and-full-builds\.  Stop\.'
                                     r'\n\Z')
            status, output = make(root, '-n', 'progress')
            self.assertEqual(status, 0, output)
            for orphan in orphans:
                (root / orphan).unlink()
            status, output = make(root, '-n', 'check')
            self.assertEqual(status, 0, output)

    def test_make_offsets_copies_in_each_table_that_differs_and_removes_each_with_no_body_source(self):
        # a.c has no table: make offsets measures one and copies it in, and names it. A second run copies nothing,
        # and leaves the table as it was dated. With the table changed, and beside it a table of main.c's, which
        # takes no body, it copies a.c's in again and removes the other, naming each
        with self.measured() as root:
            self.assertEqual(make(root, 'offsets', 'CC=true'), (0, COPIED(A_TABLE)))
            self.assertEqual((root / A_TABLE).read_text(), MEASURED_ROWS)
            dated = (root / A_TABLE).stat().st_mtime_ns
            self.assertEqual(make(root, 'offsets', 'CC=true'), (0, ''))
            self.assertEqual((root / A_TABLE).stat().st_mtime_ns, dated)
            (root / A_TABLE).write_text(NO_ROWS)
            (root / 'offsets/src').mkdir()
            (root / 'offsets/src/main.txt').write_text(NO_ROWS)
            self.assertEqual(make(root, 'offsets', 'CC=true'), (0, COPIED(A_TABLE) + REMOVED('offsets/src/main.txt')))
            self.assertEqual((root / A_TABLE).read_text(), MEASURED_ROWS)
            self.assertFalse((root / 'offsets/src/main.txt').exists())
            self.assertEqual(make(root, A_HELD, 'CC=true'), (0, ''))

    def test_the_bodies_wait_for_their_table_s_check_for_a_goal_of_the_matching_build_alone(self):
        # make, which builds each image, holds a.c's table to its stub build's before it reads a.c's bodies from the
        # disc. a.c's link named alone reads the bodies and holds no table
        with checkout({**UNBUILT, **PINNED}) as root:
            status, output = make(root, '-n')
            self.assertEqual(status, 0, output)
            self.assertLess(output.index(HOLD), output.index(BODIES), output)
            status, output = make(root, '-n', 'build/mod/modules/mod/a.bin')
            self.assertEqual(status, 0, output)
            self.assertIn(BODIES, output)
            for step in (MEASURE, HOLD):
                self.assertNotIn(step, output)

    @contextlib.contextmanager
    def measured(self, files=None):
        """MEASURED's checkout with files, settled once make -n check has stamped the tools: a.c's stub and its reads
        are up to date."""
        with checkout({**MEASURED, **(files or {})}) as root:
            make(root, '-n', 'check', 'CC=true')
            settle(root)
            status, output = make(root, '-n', A_BUILT_TABLE, 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertEqual([step for step in (STUB_SHC, WRITE_A_READS, MEASURE) if step in output], [MEASURE], output)
            yield root


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
            files = {'config/compilers.mk': routes + setting, 'config/mod/splits.txt': splits, **MOD_DATA,
                     'sdk/ninja/s.c': '', 'sdk/ninja/g.c': '', 'sdk/ninja/d.c': '', **bodies, 'orig/SONICADV/MOD.PRS': ''}
            with self.subTest(obj), checkout(tabled(files)) as root:
                # make check builds no gen/ range's object
                status, output = make(root, '-n', 'check', gen)
                if obj is None:
                    self.assertEqual(status, 0, output)
                else:
                    self.assertNotEqual(status, 0, output)
                    self.assertIn(f'{obj}: its recipe runs no tools/align.py, so it takes no ALIGN', output)


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
# a wibo that writes each word it is given to a line of the file "ran"
RECORDS = "#!/bin/sh\nprintf '%s\\n' \"$@\" > ran\n"


class Hung(unittest.TestCase):
    @unittest.skipUnless(TIMEOUT, "PATH finds no timeout, and the tests' stand-in for one sets no limit")
    def test_kills_a_tool_still_running_its_grace_after_term(self):
        # with the limits cut to half a second each, timeout sends the stand-in wibo TERM, then KILL, a second before
        # it would write survived
        with checkout({**SEARCHED, WIBO: OUTLIVES_TERM, 'tools/shc-v5.0r32/shc.exe': ''}) as root:
            (root / WIBO).chmod(0o755)
            status, output = make(root, 'build/modules/mod/b.obj', 'HUNG=0.5', 'GRACE=0.5')
            time.sleep(2)
            survived = (root / 'survived').exists()
        self.assertNotEqual(status, 0, output)
        for sent in ('TERM', 'KILL'):
            self.assertIn(f"timeout: sending signal {sent} to command '{WIBO}'", output)
        self.assertFalse(survived)

    def test_the_stand_in_for_timeout_runs_the_tool_it_is_given(self):
        # where PATH finds no timeout, UNLIMITED takes its place for every test's make. Here it takes it on any
        # host: wibo runs with the words that follow the limits, asmsh first
        with checkout({**SEARCHED, WIBO: RECORDS, 'tools/shc-v5.0r32/shc.exe': '', 'bin/timeout': UNLIMITED}) as root:
            for script in (WIBO, 'bin/timeout'):
                (root / script).chmod(0o755)
            status, output = make(root, 'build/modules/mod/b.obj', path=f'{root / "bin"}{os.pathsep}{PATH}')
            self.assertEqual(status, 0, output)
            self.assertEqual((root / 'ran').read_text().splitlines()[:2],
                             ['tools/shc-v5.0r32/asmsh.exe', r'modules\mod\b.src'])


# BUILT's goals: make check, the links of a.c, the gen/ range and g.c, and MOD.BIN and its content
CHECK, FULL, GEN = 'check', 'build/mod/modules/mod/a.bin', 'build/mod/gen/mod/d_0c900180.bin'
GRAFT, IMAGE, CONTENT = 'build/mod/sdk/ninja/g.bin', 'build/SONICADV/MOD.BIN', 'build/gen/mod/content.c'
# each step but STUB_SHC and BODIES, which support.py gives, by the text make -n prints for it
ZERO = 'echo 0 > build/stub/modules/mod/a/a.inc'
TEXTS = 'tools/data.py --texts modules/mod/a.c build/stub/modules/mod/a/text.inc'
VALUES = 'tools/data.py --values modules/mod/a.c build/stub/modules/mod/a/values.inc'
VERIFY = 'touch build/verified/SONICADV/MOD.PRS.ok'
DECODE = 'build/prskit decompress orig/SONICADV/MOD.PRS build/gen/img/MOD.BIN'
SHC = 'shc.exe modules/mod/a.c -objectfile=build/modules/mod/a.obj'
GENERATE = 'tools/data.py build/gen/img/MOD.BIN gen/mod/d_0c900180.c build/gen/mod/d_0c900180.c'
GEN_SHC = 'shc.exe build/gen/mod/d_0c900180.c'
GRAFT_SHC = 'shc.exe sdk/ninja/g.c -objectfile=build/sdk/ninja/g.src'
PATCHED_SHC = 'shc.exe sdk/ninja/p.c -objectfile=build/sdk/ninja/p.src'
ASM = r"asmsh.exe 'modules\mod\b.src'"
SDK_LINK = 'tools/link.py build/sdk/ninja/s.obj'
SPLICE = 'tools/splice.py build/gen/img/MOD.BIN build/SONICADV/MOD.BIN'
WRITE_CONTENT = 'tools/content.py build/gen/img/MOD.BIN build/gen/mod/content.c'
STEPS = (STUB_SHC, ZERO, TEXTS, VALUES, MEASURE, HOLD, VERIFY, DECODE, BODIES, SHC, GENERATE, GEN_SHC, GRAFT_SHC,
         PATCHED_SHC, ASM, 'tools/link.py', SPLICE, WRITE_CONTENT)


class Freshness(unittest.TestCase):
    # each side file, a goal that needs it, and the steps its loss reruns: the step that writes it, and those that
    # read it
    DELETED = [
        ('build/modules/mod/a.d', FULL, [SHC]),
        ('build/stub/modules/mod/a.d', CHECK, [STUB_SHC]),
        ('build/stub/modules/mod/a.lis', CHECK, [STUB_SHC, MEASURE]),
        (A_READS, CHECK, [WRITE_A_READS, MEASURE]),
        (A_READS, FULL, [WRITE_A_READS, BODIES, SHC]),
        (A_BUILT_TABLE, CHECK, [MEASURE, HOLD]),
        (A_HELD, CHECK, [HOLD]),
        ('build/stub/modules/mod/a/a.inc', CHECK, [ZERO, STUB_SHC]),
        ('build/stub/modules/mod/a/text.inc', CHECK, [TEXTS, STUB_SHC]),
        ('build/stub/modules/mod/a/values.inc', CHECK, [VALUES, STUB_SHC]),
        ('build/gen/modules/mod/a/a.inc', FULL, [BODIES, SHC]),
        ('build/gen/modules/mod/a/text.inc', FULL, [BODIES, SHC]),
        ('build/gen/modules/mod/a/values.inc', FULL, [BODIES, SHC]),
        ('build/gen/modules/mod/a/bodies.txt', FULL, [BODIES, SHC]),
        ('build/gen/mod/d_0c900180.syms', GEN, [GENERATE, GEN_SHC]),
        ('build/gen/mod/d_0c900180.d', GEN, [GEN_SHC]),
        ('build/gen/mod/d_0c900180.c.d', GEN, [GENERATE, GEN_SHC]),
        ('build/gen/mod/content.txt', CONTENT, [WRITE_CONTENT]),
        ('build/gen/mod/content.ld', CONTENT, [WRITE_CONTENT]),
        ('build/gen/mod/content.bin', CONTENT, [WRITE_CONTENT]),
        ('build/gen/mod/content.d', CONTENT, [WRITE_CONTENT]),
        ('build/sdk/ninja/g.d', GRAFT, [GRAFT_SHC]),
        ('build/sdk/ninja/g_donor.d', GRAFT, [GRAFT_SHC]),
        ('build/sdk/ninja/g.lis', GRAFT, [GRAFT_SHC]),
        ('build/modules/mod/b.lis', CHECK, [ASM]),
    ]
    # each file a step reads, a goal that needs it, and the steps a change to it reruns
    CHANGED = [
        ('modules/mod/a.h', FULL, [WRITE_A_READS, BODIES, SHC]),
        ('modules/mod/a.c', CHECK, [TEXTS, VALUES, STUB_SHC, MEASURE, HOLD]),
        (A_TABLE, CHECK, [HOLD]),
        (A_TABLE, FULL, [BODIES, SHC]),
        ('orig/SONICADV/MOD.PRS', FULL, [VERIFY, DECODE, BODIES, SHC]),
        ('config/mod/splits.txt', GEN, [GENERATE, GEN_SHC]),
        ('config/mod/data.txt', GEN, [GENERATE, GEN_SHC]),
        ('config/mod/data.txt', CONTENT, [WRITE_CONTENT]),
        ('config/mod/splits.txt', FULL, [BODIES, SHC]),
        ('config/mod/symbols.txt', CONTENT, [WRITE_CONTENT]),
        ('sdk/ninja/d.h', GRAFT, [GRAFT_SHC]),
        ('tools/align.py', CHECK, [PATCHED_SHC]),
        ('build/tools/r32.id', CHECK, [SDK_LINK]),
        ('tools/code_ranges.py', IMAGE, [SPLICE]),
        ('tools/shc_object.py', CHECK, [SDK_LINK]),
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

    def test_generates_a_range_again_when_a_text_its_types_come_from_changes(self):
        # the .c.d of a range with a row of a C type names each text the generator read its type from
        with self.built() as root:
            (root / 'build/gen/mod/d_0c900180.c.d').write_text('build/gen/mod/d_0c900180.c: modules/mod/a.h\n'
                                                               'modules/mod/a.h:\n')
            settle(root)
            os.utime(root / 'modules/mod/a.h')
            self.assertReruns(root, GEN, [GENERATE, GEN_SHC])

    def test_a_gen_range_without_data_txt_stops_make(self):
        with self.built() as root:
            (root / 'config/mod/data.txt').unlink()
            status, output = make(root, '-n', GEN)
        self.assertNotEqual(status, 0, output)
        self.assertIn("No rule to make target 'config/mod/data.txt'", output)

    def test_compiles_and_generates_again_once_a_header_joins_roots(self):
        # a .d file names the headers its compile or generator found, and build/headers.txt those under ROOTS, any of
        # which a search could find first
        with self.built() as root:
            (root / 'modules/mod/b.h').write_text('')
            self.assertReruns(root, FULL, [SHC])
            self.assertReruns(root, GEN, [GENERATE, GEN_SHC])
            self.assertReruns(root, CONTENT, [WRITE_CONTENT])

    def test_writes_the_content_again_once_tools_content_py_changes_and_generates_no_range(self):
        # a change to tools/content.py reaches the content alone: tools/data.py writes the gen/ C
        with self.built() as root:
            os.utime(root / 'tools/content.py')
            status, output = make(root, '-n', GEN, CONTENT)
        self.assertEqual(status, 0, output)
        self.assertIn(WRITE_CONTENT, output)
        for step in (GENERATE, GEN_SHC):
            self.assertNotIn(step, output)

    def test_writes_a_content_again_for_another_image_s_data_txt_by_its_d_file_alone(self):
        # mod's content can take a name from two's gen/ range, whose type two's data.txt gives. A change to two's
        # data.txt rewrites mod's content only through the content's .d file, which names each data.txt the content
        # read. make dc isn't the only goal that reads the .d file: content.c asked for alone reruns when a file it
        # names changes, and not otherwise
        two = {'config/two/splits.txt': 'D  0x0ca00000  0x0ca00100\ngen/two/d_0ca00000.c  D  0x0ca00000  0x0ca00100\n',
               'config/two/symbols.txt': '', 'config/two/data.txt': '', 'orig/SONICADV/TWO.PRS': ''}
        for named, rewritten in (('', False), ('config/two/data.txt', True)):
            with self.subTest(named=named), checkout({**BUILT, **PINNED, **two,
                                                      'build/gen/mod/content.d': f'{CONTENT}: {named}\n'}) as root:
                status, output = make(root, '-n', CHECK)
                self.assertEqual(status, 0, output)
                settle(root)
                status, output = make(root, '-n', CONTENT)
                self.assertEqual(status, 0, output)
                self.assertNotIn(WRITE_CONTENT, output)
                os.utime(root / 'config/two/data.txt')
                status, output = make(root, '-n', CONTENT)
                self.assertEqual(status, 0, output)
                self.assertEqual(WRITE_CONTENT in output, rewritten, output)

    def test_a_content_waits_for_its_image_s_bodies_and_reads_again_those_its_d_file_names(self):
        # mod places a.c, whose bodies.txt gives the address of each text a.c declares. The content waits for it, and
        # a change to it rewrites the content only where the content's .d file names it, as it does once the content
        # copies a text of a.c's
        bodies = 'build/gen/modules/mod/a/bodies.txt'
        for named, rewritten in (('', False), (bodies, True)):
            with self.subTest(named=named), checkout({**BUILT, **PINNED,
                                                      'build/gen/mod/content.d': f'{CONTENT}: {named}\n'}) as root:
                status, output = make(root, '-n', CHECK)
                self.assertEqual(status, 0, output)
                settle(root)
                (root / bodies).unlink()
                status, output = make(root, '-n', CONTENT)
                self.assertEqual(status, 0, output)
                self.assertIn(BODIES, output)
                (root / bodies).write_text(BUILT[bodies])
                settle(root)
                os.utime(root / bodies)
                status, output = make(root, '-n', CONTENT)
                self.assertEqual(status, 0, output)
                self.assertEqual(WRITE_CONTENT in output, rewritten, output)

    # each file a step reads, a goal that reads it, a step a change to it reruns, and those it leaves alone: the
    # host tools read no SHC flag, so a compilers.mk edit rebuilds no tool and decodes no image again; and a tool
    # waits for the vendored check without depending on it, so the check's rerun rebuilds none
    APART = [
        ('config/compilers.mk', FULL, [SHC], ['tools/prskit/prskit.c', DECODE]),
        ('tools/tool.py', 'build/prskit', ['tools/vendor.py --check'], ['tools/prskit/prskit.c']),
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

    def test_rebuilds_no_shc_object_once_make_dc_s_configuration_changes(self):
        # the matching build doesn't read config/gcc.mk. A change to it leaves every SHC object and stub up to date,
        # and no goal of the matching build runs a step
        with self.built() as root:
            (root / 'config/gcc.mk').write_text((ROOT / 'config/gcc.mk').read_text() + 'GCCFLAGS += -fno-common\n')
            status, output = make(root, '-q', *sorted(path for path in BUILT if path.endswith('.obj')))
            self.assertEqual(status, 0, output)
            for goal in (CHECK, FULL, GEN, GRAFT, IMAGE):
                with self.subTest(goal):
                    status, output = make(root, '-n', goal)
                    self.assertEqual(status, 0, output)
                    self.assertEqual([step for step in STEPS if step in output], [], output)

    @contextlib.contextmanager
    def built(self):
        """BUILT's checkout, with the pinned tools in place, in which make -n prints none of STEPS for CHECK, FULL,
        GEN, GRAFT, IMAGE or CONTENT."""
        with checkout({**BUILT, **PINNED}) as root:
            # the first make writes build/deps.mk, which settle() then dates with the rest
            status, output = make(root, '-n', CHECK)
            self.assertEqual(status, 0, output)
            settle(root)
            for goal in (CHECK, FULL, GEN, GRAFT, IMAGE, CONTENT):
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
# the platform tag of the wheels the Makefile pins for each host, by what uname -sm prints there: x86-64 Linux's, an
# Apple-silicon Mac's and an Intel Mac's
PLATFORMS = dict(zip((LINUX, *MACS), ('manylinux_2_27_x86_64.manylinux_2_28_x86_64', 'macosx_11_0_arm64',
                                      'macosx_10_9_x86_64')))


def wheel_sums(uname):
    """The repository's tools/SHA256SUMS's lines for the wheels of clang-format and clang-tidy, in that order, that
    the Makefile pins for the host uname names."""
    return ''.join(line for line in (ROOT / 'tools/SHA256SUMS').read_text().splitlines(True)
                   if line.endswith(f'-{PLATFORMS[uname]}.whl\n'))


# the lines for LINUX's wheels, the host make() names unless a test names another, and the wheels
WHEEL_SUMS = wheel_sums(LINUX)
WHEELS = [line.split()[1] for line in WHEEL_SUMS.splitlines()]
# an install make takes for the one WHEELS pin: both tools, and WHEEL_SUMS in tools/SHA256SUMS and in the file an
# install writes them to last
INSTALLED = {**CLANG_TOOLS, 'tools/SHA256SUMS': WHEEL_SUMS, 'tools/llvm/wheels.txt': WHEEL_SUMS}
# lint's settings, among them modules/.clang-tidy, which clang-tidy reads for a.c and which inherits the root's; its
# tools, which make takes for installed; a.c's lint target, whose .d file names the header a.c includes; and the tags
# and guards checks'
LINT = {'.clang-format': '', '.clang-tidy': '', 'modules/.clang-tidy': 'InheritParentConfig: true\n',
        'compile_flags.txt': '-Imodules\n', **INSTALLED,
        'modules/mod/a.c': '#include "mod/a.h"\n', 'modules/mod/a.h': '', 'build/lint/modules/mod/a.c.tidy': '',
        'build/lint/modules/mod/a.c.d': ('build/lint/modules/mod/a.c.tidy: modules/mod/a.c modules/mod/a.h\n'
                                         'modules/mod/a.h:\n'),
        'build/lint/tags.ok': '', 'build/lint/guards.ok': ''}
# a.c's check, which writes its .d file as clang-tidy's compiler reads a.c
TIDY = ('tools/llvm/bin/clang-tidy --quiet --extra-arg=-Xclang=-dependency-file '
        '--extra-arg=-Xclang=build/lint/modules/mod/a.c.d.tmp --extra-arg=-Xclang=-MT '
        '--extra-arg=-Xclang=build/lint/modules/mod/a.c.tidy --extra-arg=-Xclang=-MP modules/mod/a.c')
TAGS = 'python3 tools/tags.py src/main.c'


class Lint(unittest.TestCase):
    def test_reruns_a_check_whose_dependency_file_is_gone(self):
        # only the .d file names a.c's header to its lint target, so a lint target whose .d file is gone reruns, as an
        # object does, and a header changed since can't pass unchecked
        with self.linted() as root:
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
        with self.linted() as root:
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

    def test_checks_the_format_tags_and_guards_of_the_engine_s_code_and_tidies_none_of_it(self):
        # clang-tidy reads the matching side of each guard, so it can't compile engine/dc/'s C, and the engine's flags
        # hold its own. Once b.c changes, format.ok checks b.c alone, and tags.ok and guards.ok check every file. Once
        # .clang-format changes, format.ok checks every file, the engine's C++ among them
        source, header = ('engine/dc/b.c', 'engine/dc/b.h')
        port = (source, header, 'engine/runtime/a.cpp', 'engine/include/a.h')
        with self.linted({**LINT, **SEARCHED, 'build/lint/format.ok': '', **dict.fromkeys(port, '')}) as root:
            os.utime(root / source)
            status, output = make(root, '-n', 'lint')
            self.assertEqual(status, 0, output)
            checks = ('tools/llvm/bin/clang-format --dry-run -Werror ', 'python3 tools/tags.py ',
                      'python3 tools/guards.py ')
            formatted, tagged, guarded = (next((line for line in output.splitlines() if line.startswith(check)),
                                               '').split() for check in checks)
            self.assertIn(source, formatted)
            self.assertNotIn(header, formatted)
            for path in port:
                self.assertIn(path, tagged)
                self.assertIn(path, guarded)
            self.assertNotIn('build/lint/engine/', output)
            os.utime(root / '.clang-format')
            status, output = make(root, '-n', 'lint')
            self.assertEqual(status, 0, output)
            formatted = next((line for line in output.splitlines() if line.startswith(checks[0])), '').split()
            for path in port:
                self.assertIn(path, formatted)
            status, output = make(root, '-n', 'format')
        self.assertEqual(status, 0, output)
        self.assertIn('tools/llvm/bin/clang-format -i ', output)
        for path in port:
            self.assertIn(f' {path}', output)

    def test_reruns_the_check_a_changed_file_feeds(self):
        with self.linted() as root:
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

    # each file a check reads that no .d file names, and the check its deletion reruns: a nested .clang-tidy, whose
    # parent's settings then apply, and a table
    DELETED = [
        ('modules/.clang-tidy', TIDY),
        ('config/mod/symbols.txt', TAGS),
    ]

    def test_reruns_the_check_a_deleted_file_fed(self):
        for path, check in self.DELETED:
            with self.subTest(path), self.linted() as root:
                (root / path).unlink()
                status, output = make(root, '-n', 'lint')
                self.assertEqual(status, 0, output)
                self.assertIn(check, output)

    def test_tidies_again_once_a_header_joins_roots(self):
        with self.linted() as root:
            (root / 'modules/mod/b.h').write_text('')
            status, output = make(root, '-n', 'lint')
        self.assertEqual(status, 0, output)
        self.assertIn(TIDY, output)

    @contextlib.contextmanager
    def linted(self, files=LINT):
        """The checkout of files, LINT unless given, settled once make -n lint has written the inventories of the
        tables and the .clang-tidy files."""
        with checkout(files) as root:
            status, output = make(root, '-n', 'lint')
            self.assertEqual(status, 0, output)
            settle(root)
            yield root


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
        # with no pinned tool in place, each goal of the matching build that builds runs curl to download them, and
        # tar and gzip to unpack the SHC releases. It asks for cmp, which holds a table of offsets to its stub build's
        with checkout() as root, host(*COMMANDS) as path:
            for goals in ((), ('check',), ('progress',), ('relink',), ('shift',), ('offsets',)):
                with self.subTest(goals=goals):
                    status, output = make(root, *goals, 'CC=no-such-cc', path=path)
                    self.assertFalse((root / 'build/deps.mk').exists())
                    self.assertNotEqual(status, 0, output)
                    self.assertIn('*** no-such-cc sha256sum timeout curl tar gzip cmp: not found; see README.md#build',
                                  output)

    def test_pc_needs_no_timeout_and_no_download_of_shc_on_any_host(self):
        # make pc runs no SHC: a body source's bodies come by its table of offsets. So it asks for no timeout, which
        # runs wibo, and for no curl, tar or gzip to fetch and unpack wibo and SHC's releases, though none is in
        # place, on Linux as on a Mac. The Makefile checks the host before it reads the engine's rules, of which
        # the checkout holds none
        for uname in (LINUX, *MACS):
            with self.subTest(uname), checkout() as root, host(*COMMANDS) as path:
                status, output = make(root, 'pc', 'CC=no-such-cc', path=path, uname=uname)
                self.assertNotEqual(status, 0, output)
                self.assertIn('*** no-such-cc sha256sum: not found; see README.md#build', output)

    def test_needs_no_download_command_once_the_tools_are_in_place(self):
        with checkout(PINNED) as root, host('curl', 'tar', 'gzip') as path:
            status, output = make(root, 'check', 'CC=no-such-cc', path=path)
        self.assertNotEqual(status, 0, output)
        self.assertIn('*** no-such-cc: not found; see README.md#build', output)

    def test_a_goal_of_the_matching_build_that_builds_needs_cmp(self):
        # the pinned tools are in place and the host lacks cmp alone. make review builds nothing, and needs none
        with checkout(PINNED) as root, host('cmp') as path:
            for goals in ((), ('check',), ('progress',), ('relink',), ('shift',), ('offsets',)):
                with self.subTest(goals=goals):
                    status, output = make(root, *goals, 'CC=true', path=path)
                    self.assertNotEqual(status, 0, output)
                    self.assertIn('*** cmp: not found; see README.md#build', output)
            status, output = make(root, 'review', 'CC=true', path=path)
            self.assertEqual(status, 0, output)
            self.assertNotIn('README.md#build', output)

    def test_needs_tar_and_gzip_for_an_shc_release_alone(self):
        releases = {path: text for path, text in PINNED.items() if path.endswith('/shc.exe')}
        with checkout(releases) as root, host('curl', 'tar', 'gzip') as path:
            status, output = make(root, 'check', 'CC=no-such-cc', path=path)
        self.assertNotEqual(status, 0, output)
        self.assertIn('*** no-such-cc curl: not found; see README.md#build', output)

    def test_needs_gzip_for_an_shc_release_only_where_tar_runs_it(self):
        # no pinned tool is in place, and the host has no gzip. GNU tar runs gzip to unpack a release, and so does a
        # libarchive tar that names no zlib. One that names zlib unpacks a release by itself
        for tar, lacks in ((GNU_TAR, ' gzip'), (f'{BSDTAR} {LIBLZMA} bz2lib/1.0.8', ' gzip'),
                           (f'{BSDTAR} {ZLIB} {LIBLZMA} bz2lib/1.0.8', '')):
            with self.subTest(tar), checkout() as root, host('gzip', tar=tar) as path:
                status, output = make(root, 'check', 'CC=no-such-cc', path=path)
                self.assertNotEqual(status, 0, output)
                self.assertIn(f'*** no-such-cc{lacks}: not found; see README.md#build', output)

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

    def test_checks_no_command_for_a_goal_that_names_a_file(self):
        # a goal that names a file gets no host check. An SHC object compiles under timeout, and no pinned tool is in
        # place, yet make names no command the host lacks: it goes on to fetch a tool
        with checkout(SEARCHED) as root, host(*COMMANDS) as path:
            status, output = make(root, 'build/modules/mod/a.obj', 'CC=no-such-cc', path=path)
        self.assertNotEqual(status, 0, output)
        self.assertNotIn('README.md#build', output)
        self.assertIn('  fetch  tools/', output)

    def test_names_a_make_without_grouped_targets_and_the_make_a_mac_can_install(self):
        # GNU make 4.3 added grouped targets, and grouped-target in .FEATURES with them. macOS's own make is 3.81,
        # so the stop names Homebrew's. It comes before make asks its host, so it reads the same on every host, and
        # on a Mac it comes before the refusal of make check
        with checkout() as root:
            for uname in (LINUX, *MACS):
                with self.subTest(uname):
                    status, output = make(root, '-n', 'check', '.FEATURES=target-specific order-only', uname=uname)
                    self.assertNotEqual(status, 0, output)
                    self.assertRegex(output, r'\AMakefile:\d+: \*\*\* the build needs GNU make 4\.3 or later, and this '
                                             r'is [\d.]+; on macOS, brew install make and run gmake; see '
                                             r'README\.md#build\.  Stop\.\n\Z')

    def test_names_a_python_before_3_8_and_takes_3_8(self):
        for version, needed in (((3, 7, 99, 'final', 0), True), ((3, 8, 0, 'final', 0), False)):
            versioned = {'bin/python3': python(f'sys.version_info = {version!r}')}
            with self.subTest(version=version), checkout(versioned) as root:
                (root / 'bin/python3').chmod(0o755)
                status, output = make(root, '-n', 'test', path=f'{root / "bin"}{os.pathsep}{PATH}')
                self.assertEqual(status != 0, needed, output)
                self.assertEqual('the build needs Python 3.8 or later, as python3' in output, needed, output)

    def test_the_engine_s_goals_name_a_python_before_3_9_and_take_3_9(self):
        # the engine's scripts use what 3.9 added, so a goal that reads the engine's rules stops on 3.8, which make
        # test takes
        for version, needed in (((3, 8, 99, 'final', 0), True), ((3, 9, 0, 'final', 0), False)):
            versioned = {**PINNED, **SEARCHED, **SLICE, 'bin/python3': python(f'sys.version_info = {version!r}')}
            with self.subTest(version=version), checkout(versioned) as root:
                (root / 'bin/python3').chmod(0o755)
                path = f'{root / "bin"}{os.pathsep}{PATH}'
                _, output = make(root, '-n', 'engine-test', path=path)
                self.assertEqual("the engine's goals need Python 3.9 or later, as python3" in output, needed, output)
                self.assertNotIn("the engine's goals need", make(root, '-n', 'test', path=path)[1])


# the goals a person types and the variables a person sets
GOALS = {'all', 'relink', 'shift', 'check', 'offsets', 'dc', 'dc-disc', 'dc-check', 'dc-order', 'native', 'pc', 'play',
         'engine-test', 'accept', 'test', 'lint',
         'review', 'format', 'progress', 'clean', 'help', 'stub-includes'}
VARIABLES = {'V', 'SEARCH', 'PACK', 'CC', 'WIBO', 'R10', 'R26', 'R32', 'SH_ELF'}


def written(root):
    """Each path make wrote under root's build/ but Python's bytecode. With the pinned tools in place, a goal
    that compiles writes build/deps.mk and stamps each tool."""
    return {path.relative_to(root) for path in (root / 'build').rglob('*') if 'pycache' not in path.parts}


# BUILT's tables and sources without the files a build of them writes, and the originals: a tree in which x86-64 Linux
# has a rule for each of MATCHING_SAMPLES
UNBUILT = {**{path: text for path, text in BUILT.items() if not path.startswith('build/')}, **SEARCHED, **ORIGINALS}
# a file of each kind the Makefile's MATCHING_FILES lists: an object on each route, a stub's, a gen/ range's and
# asmsh's; a link, a stub's and a gen/ range's; an image, its .PRS file and the .BIN that decodes to; an image's
# whole link, its shift's list and the list of them all; the table of offsets a stub build gives and its check; a
# stamp of wibo's and of a release's; and wibo and a release
MATCHING_SAMPLES = ('build/src/main.obj', 'build/sdk/ninja/s.obj', 'build/sdk/ninja/p.obj', 'build/sdk/ninja/g.obj',
                    'build/modules/mod/a.obj', 'build/stub/modules/mod/a.obj', 'build/gen/mod/d_0c900180.obj',
                    'build/modules/mod/b.obj', 'build/1st_read/src/main.bin', 'build/stub/mod/modules/mod/a.bin',
                    'build/mod/gen/mod/d_0c900180.bin', 'build/1ST_READ.BIN', 'build/SONICADV/MOD.PRS',
                    'build/SONICADV/MOD.BIN', 'build/relink/mod.bin', 'build/relink/mod/fixed.txt',
                    'build/relink/fixed.txt', A_BUILT_TABLE, A_HELD, 'build/tools/wibo.id', 'build/tools/r10.id',
                    WIBO, 'tools/shc-v5.0r26/shc.exe')


@contextlib.contextmanager
def scripted(files):
    """A checkout() of files in which each file whose text opens with #! is executable: a stand-in for one of the
    host's commands, in bin/, or for wibo."""
    with checkout(files) as root:
        for path, text in files.items():
            if isinstance(text, str) and text.startswith('#!'):
                (root / path).chmod(0o755)
        yield root


def held(root):
    """Each file under root but Python's bytecode, which a make that reads the tables writes under build/."""
    return sorted(path for path in root.rglob('*') if path.is_file() and 'pycache' not in path.parts)


# the spellings of what runs SHC, asmsh, lnk or cnvs: wibo, by its path or as the Makefile's wibo or WIBO; the suffix
# of the programs wibo runs; a release's directory; the Makefile's shc and asmsh, which run SHC and asmsh under wibo;
# and tools/link.py and tools/relink.py, which run asmsh, lnk and cnvs under it
RUNNERS = ('wibo', 'WIBO', '.exe', '(R10)', '(R26)', '(R32)', '$(call shc,', '$(call asmsh,', 'tools/link.py',
           'tools/relink.py')


class Mac(unittest.TestCase):
    def test_refuses_each_goal_of_the_matching_build_before_it_reads_or_writes_anything(self):
        # the build runs SHC under wibo on Linux on x86 alone, so on a Mac of either processor make refuses the goals
        # that need it: make, which is make all, and each goal of the matching build, under -n too, and whatever WIBO
        # names. It stops before the Python that reads the tables, and writes nothing, with the pinned tools in place
        # or with none to fetch. RAN stands in for python3, for the curl that would fetch a tool and for wibo
        self.assertEqual(sorted(MATCHING_GOALS), ['accept', 'all', 'check', 'offsets', 'progress', 'relink', 'shift'])
        for mac in MACS:
            for tools in ({**PINNED, WIBO: RAN}, {}):
                with self.subTest(mac, tools=bool(tools)), \
                        scripted({**UNBUILT, **tools, 'bin/python3': RAN, 'bin/curl': RAN}) as root:
                    path, before = path_with_bin(root), sorted(root.rglob('*'))
                    for goals in ((), *((goal,) for goal in MATCHING_GOALS), ('lint', 'check')):
                        for flags in ((), ('-n',), ('WIBO=bin/wibo',)):
                            with self.subTest(goals=goals, flags=flags):
                                status, output = make(root, *flags, *goals, path=path, uname=mac)
                                self.assertEqual(status, 2, output)
                                self.assertRegex(output, rf'\A{no_shc(*goals)}\Z')
                    self.assertEqual(sorted(root.rglob('*')), before)

    def test_refuses_each_file_of_the_matching_build_named_as_a_goal(self):
        # each of MATCHING_SAMPLES is a goal on x86-64 Linux. A Mac refuses it by the same line once make has listed
        # the tree's objects, links and images, before it writes build/deps.mk or a record, and with the originals in
        # place, so the stop is not the one for a missing disc. It refuses a wibo no host fetches, and a file of a
        # release's. One Mac has the pinned tools in place, and the other, under -n, none to fetch
        with checkout({**UNBUILT, **PINNED}) as root:
            status, output = make(root, '-n', *MATCHING_SAMPLES)
        self.assertEqual(status, 0, output)
        for mac, flags, tools in zip(MACS, ((), ('-n',)), ({**PINNED, WIBO: RAN}, {})):
            with self.subTest(mac), scripted({**UNBUILT, **tools, 'bin/curl': RAN}) as root:
                path, before = path_with_bin(root), held(root)
                for goal in (*MATCHING_SAMPLES, 'tools/wibo-macos', 'tools/shc-v5.0r32/asmsh.exe'):
                    with self.subTest(goal):
                        status, output = make(root, *flags, goal, path=path, uname=mac)
                        self.assertEqual(status, 2, output)
                        self.assertRegex(output, rf'\A{no_shc(goal)}\Z')
                self.assertEqual(held(root), before)

    def test_no_other_host_refuses_the_matching_build(self):
        # the refusal names macOS, so no other host may meet it. Each plans make check as x86-64 Linux does: 32-bit
        # x86 Linux, two hosts wibo-i686 isn't built for, and one whose uname prints nothing
        with checkout({**UNBUILT, **PINNED}) as root:
            linux = make(root, '-n', 'check')
            self.assertEqual(linux[0], 0, linux[1])
            for uname in ('Linux i686', 'Linux aarch64', 'FreeBSD amd64', ''):
                with self.subTest(uname):
                    self.assertEqual(make(root, '-n', 'check', uname=uname), linux)

    def test_a_rule_that_comes_to_depend_on_shc_stops_at_its_stamp_or_its_download_and_names_it(self):
        # no goal a Mac runs may reach SHC. Here make review comes to depend on a file SHC builds, on wibo's stamp or
        # on a download, as a rule that regresses would. Each rule that runs SHC, asmsh, lnk or cnvs depends on a
        # stamp, which stops make on a Mac, under -n too, and the line names the stamp: R32's for a stub and for a
        # link's object, R10's for an SDK source's object, and wibo's own. A download stops make alike. Nothing is
        # fetched, stamped or run
        reached = (('build/stub/modules/mod/a.obj', 'build/tools/r32.id'),
                   ('build/sdk/ninja/s.obj', 'build/tools/r10.id'),
                   ('build/1st_read/src/main.bin', 'build/tools/r32.id'),
                   ('build/tools/wibo.id', 'build/tools/wibo.id'), (WIBO, WIBO),
                   ('tools/shc-v5.0r26/shc.exe', 'tools/shc-v5.0r26/shc.exe'))
        for needs, stops in reached:
            for tools in ({**PINNED, WIBO: RAN}, {}):
                regressed = (ROOT / 'Makefile').read_text() + f'review: {needs}\n'
                with self.subTest(needs, tools=bool(tools)), \
                        scripted({**UNBUILT, **tools, 'Makefile': regressed, 'bin/curl': RAN}) as root:
                    path, before = path_with_bin(root), held(root)
                    for flags in ((), ('-n',)):
                        status, output = make(root, *flags, 'review', path=path, uname=MACS[0])
                        self.assertEqual(status, 2, output)
                        self.assertRegex(output, rf'\A{no_shc("review", reached=stops)}\Z')
                    self.assertEqual(held(root), before)

    def test_each_rule_that_runs_shc_asmsh_lnk_or_cnvs_waits_for_a_stamp(self):
        # a stamp stops a Mac's make, so no recipe there runs wibo so long as make reads a stamp before it runs the
        # recipe. RUNNERS spells what runs SHC, asmsh, lnk or cnvs. No makefile but the Makefile and
        # config/compilers.mk, whose rules make help reads, spells one outside a comment: make dc's and the engine's
        # spell none, but for the .exe of engine/mk/native.mk, the suffix of the PC game's own programs on Windows,
        # which no wibo runs. Of the repository's targets in make help's database, each whose recipe spells one
        # depends on a stamp, with two exceptions. An image's shift list depends on the image's whole link, which
        # depends on a stamp. And three rules put wibo and a release in place, the two that fetch them and the one
        # that stamps wibo: a Mac's Makefile has a stop for the recipe of each
        status, output = database()
        self.assertIn(status, (0, 1), output[-3000:])
        read = re.search(r'(?m)^MAKEFILE_LIST := (.*)$', output)[1].split()
        self.assertEqual(read, ['Makefile', 'config/compilers.mk'])
        for path in sorted(set(MAKEFILES) - set(read)):
            names = [name for name in RUNNERS if (path, name) != ('engine/mk/native.mk', '.exe')]
            for line in (ROOT / path).read_text().splitlines():
                for name in names:
                    self.assertNotIn(name, line.split('#')[0], path)
        stamps = set(re.search(r'(?m)^SHC_IDS := (.*)$', output)[1].split())
        self.assertEqual(len(stamps), 4, stamps)
        rules = {}
        for entry in output.split('\n\n'):
            lines = entry.splitlines()
            recipe = [line for line in lines if line.startswith('\t')]
            # a target's rule follows the lines that give its own variables, each TARGET: NAME := VALUE
            rule = [line for line in lines if line[:1] not in ('#', '\t') and ':' in line]
            if rule and recipe:
                target, _, prerequisites = rule[-1].partition(':')
                rules[target] = (set(prerequisites.replace('|', ' ').split()), '\n'.join(recipe))
        running = {target for target, (_, recipe) in rules.items() if any(name in recipe for name in RUNNERS)}
        stamped = {target for target in running if rules[target][0] & stamps}
        self.assertGreater(len(stamped), 10000)
        self.assertEqual({re.sub(r'/\w+/fixed', '/IMAGE/fixed', target) for target in running - stamped},
                         {'build/relink/IMAGE/fixed.txt', WIBO, 'tools/shc-%/shc.exe', 'build/tools/wibo.id'})
        self.assertEqual([target for target in running - stamped
                          if target.endswith('/fixed.txt') and not rules[target][0] & stamped], [])

    def test_an_image_stops_at_r32_s_stamp_where_make_reads_no_build_deps_mk(self):
        # an image's links reach it through build/deps.mk, which make reads for a goal that compiles. make review
        # compiles nothing, so an image it came to depend on would be spliced with no link behind it. Each image
        # depends on R32's stamp itself: a Mac stops at the stamp before any splice, and Linux's rule names it
        regressed = (ROOT / 'Makefile').read_text() + 'review: build/1ST_READ.BIN\n'
        with checkout({**UNBUILT, 'Makefile': regressed}) as root:
            status, output = make(root, '-n', 'review', uname=MACS[0])
            self.assertEqual(status, 2, output)
            self.assertRegex(output, rf'\n{no_shc("review", reached="build/tools/r32.id")}\Z')
            self.assertNotIn('tools/splice.py', output)
            status, output = make(root, '-pn', 'help')
        self.assertEqual(status, 0, output[-3000:])
        rules = dict(line.split(':', 1) for line in output.splitlines()
                     if line.startswith(('build/1ST_READ.BIN:', 'build/SONICADV/MOD.BIN:', 'build/SONICADV/MOD.PRS:')))
        self.assertEqual(len(rules), 3, output[-3000:])
        for rule in rules.values():
            self.assertIn('build/tools/r32.id', rule.split())

    def test_stops_a_make_that_rosetta_2_translates_whatever_its_goal(self):
        # on a Mac, sysctl.proc_translated is 1 in a process Rosetta 2 translates and 0 in any other. A make there
        # stops before any goal, the matching build's among them, under -n too. Linux has no Rosetta 2, and asks
        # no sysctl
        sysctl = '#!/bin/sh\n[ "$*" = "-n sysctl.proc_translated" ] || exit 2\ntouch asked\necho {}\n'.format
        said = 'the build needs a native make, and this one runs under Rosetta 2; see README.md#build'
        with scripted({**SEARCHED, 'bin/sysctl': sysctl(1)}) as root:
            path = path_with_bin(root)
            for mac in MACS:
                for goals in (('help',), ('test',), ('lint', 'format'), ('check',), ('dc',)):
                    for flags in ((), ('-n',)):
                        with self.subTest(mac, goals=goals, flags=flags):
                            status, output = make(root, *flags, *goals, path=path, uname=mac)
                            self.assertEqual(status, 2, output)
                            self.assertRegex(output, rf'\A{stopped(*goals, said=said)}\Z')
            (root / 'asked').unlink()
            self.assertEqual(make(root, 'help', path=path)[0], 0)
            self.assertFalse((root / 'asked').exists())
            (root / 'bin/sysctl').write_text(sysctl(0))
            self.assertEqual(make(root, 'help', path=path, uname=MACS[0])[0], 0)
            self.assertTrue((root / 'asked').exists())


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
        variables = [list(row) for row in re.findall(r'^## ([A-Z0-9_]+=\S+): (.+)$', text, re.M)]
        self.assertEqual(rows, goals + variables, output)
        self.assertEqual({goal for goal, _ in goals}, GOALS)
        self.assertLessEqual(max(map(len, output.splitlines())), 80, output)
        self.assertEqual({name.split('=')[0] for name, _ in variables}, VARIABLES)

    def test_ends_on_a_mac_by_naming_the_goals_it_doesn_t_run(self):
        # after the lines every host prints, and in their columns
        with checkout({**PINNED, **SEARCHED}) as root:
            status, linux = make(root, 'help')
            self.assertEqual(status, 0, linux)
            for mac in MACS:
                with self.subTest(mac):
                    last = f'{"not on macOS":20}{" ".join(MATCHING_GOALS)}\n'
                    self.assertEqual(make(root, 'help', uname=mac), (0, linux + last))
                    self.assertLessEqual(len(last.rstrip('\n')), 80)
            self.assertEqual(written(root), set())

    def test_refuses_a_setting_it_doesn_t_list_before_it_writes_anything(self):
        # no stamp keeps one, so what it changed would pass for what make builds without it; MAKE, HUNG and GRACE,
        # which the tests set, change no output, and SETTINGS holds no other name that make help doesn't list
        self.assertEqual(set(SETTINGS), VARIABLES | {'MAKE', 'HUNG', 'GRACE'})
        with checkout({**PINNED, **SEARCHED}) as root:
            for goals in ((), ('dc',), ('accept',)):
                with self.subTest(goals=goals):
                    status, output = make(root, '-n', *goals, 'CC=true', 'GCCFLAGS=-DVALUE=2')
                    self.assertNotEqual(status, 0, output)
                    self.assertRegex(output, r'\AMakefile:\d+: \*\*\* GCCFLAGS: make takes no such setting; make help '
                                             r'lists those it takes\.  Stop\.\n\Z')
            self.assertEqual(written(root), set())
            status, output = make(root, 'help', *(f'{name}=x' for name in (*VARIABLES, 'MAKE', 'HUNG', 'GRACE')))
        self.assertEqual(status, 0, output)


# the native slice's program, which make pc builds. A test that runs the slice's rules puts a stand-in in its
# place and gives make -o for it, so that make takes it as built
PLAY = 'build/engine/native/play'
# what lets make read the engine's own rules for native or pc in a checkout that holds none of the engine's sources:
# the rules; in bin/, a stand-in that does nothing for each command the slice's host check asks of PATH; the two files
# of the engine's that the slice's marked copy of the game's C depends on, and the record that the copy is current,
# so make writes none; and the slice's census with the script that writes it, so make writes none
SLICE = {**{path: (ROOT / path).read_text() for path in MAKEFILES if path.startswith('engine/')},
         **{f'bin/{command}': '#!/bin/sh\n' for command in ('clang', 'readelf', 'nm')},
         'engine/tools/marks.py': '', 'engine/dc/dreamcast.h': '', 'build/engine/native/marked.mk': '',
         'engine/tools/census.py': '', 'build/engine/native/census.txt': ''}


# the copies of play that make native has strip make, one without its symbol table and one whose table lacks iamb, a
# name the adapter finds a static by: strip's flags for each, the words make native holds its log to, and runner.c's
# sentence with those words, which such a play says as it starts, with KEEP after it, before it ends with 2
KEEP = 'the program must keep the symbol table it was linked with, which strip removes'
COPIES = {'stripped': ('', 'holds no symbol table',
                       "this program's own file holds no symbol table, where it finds the game's statics by name"),
          'lacking': ('-N iamb ', 'holds no object named iamb',
                      "this program's symbol table holds no object named iamb, a static of the game's it finds by "
                      'name')}
# stand-ins for a test that runs make native. play prints its arguments and ends with status.txt's status. strip adds
# its arguments to strip.txt and copies the program, its last argument, to the one after -o. A copy prints the said.
# file of its name and ends with the status the ended. file of its name holds: COPIES' sentence for it, and 2, where
# the test writes neither
STRIPPED = {'bin/strip': '#!/bin/sh\necho "$*" >> strip.txt\nwhile [ "$1" != -o ]; do shift; done\ncp "$3" "$2"\n',
            **{f'said.{copy}': f'sa1: {says}: {KEEP}\n' for copy, (_, _, says) in COPIES.items()},
            **{f'ended.{copy}': '2\n' for copy in COPIES},
            PLAY: ('#!/bin/sh\ncopy=${0##*/play}\n[ -z "$copy" ] || { cat "said$copy"; exit "$(cat "ended$copy")"; }\n'
                   'echo "sa1: $*"\nexit "$(cat status.txt)"\n')}


@contextlib.contextmanager
def sliced(files):
    """A scripted() checkout of the pinned tools, SLICE and files, settled. Until the caller is done, os.environ
    holds no variable named NATIVE or NATIVE_ and more, which engine/mk/native.mk's rules would take from it for
    how they build the slice, and names a C++ compiler that does nothing, as CC=true names such a C compiler. Yields
    the checkout's root and a PATH that searches its bin/ first."""
    env = {name: value for name, value in os.environ.items() if not re.match(r'NATIVE(_|\Z)', name)}
    with scripted({**PINNED, **SEARCHED, **SLICE, **files}) as root, \
            mock.patch.dict(os.environ, {**env, 'CXX': 'true'}, clear=True):
        settle(root)
        yield root, path_with_bin(root)


class Engine(unittest.TestCase):
    def test_reads_the_engine_s_rules_for_its_own_goals_alone(self):
        # so the matching build and the port's image take none of the engine's rules
        with checkout({**PINNED, **SEARCHED, 'engine/engine.mk': '$(error engine.mk read)\n'}) as root:
            for goals in ((), ('check',), ('dc',), ('dc-disc',), ('dc-check',), ('help',), ('test',)):
                with self.subTest(goals=goals):
                    _, output = make(root, '-n', *goals)
                    self.assertNotIn('engine.mk read', output)
            for goal in ('native', 'pc', 'play', 'engine-test', 'engine-sa1', 'build/engine/abi/c'):
                with self.subTest(goal=goal):
                    status, output = make(root, '-n', goal)
                    self.assertNotEqual(status, 0, output)
                    self.assertRegex(output, r'(?m)^engine/engine\.mk:1: \*\*\* engine\.mk read\.  Stop\.$')

    def test_native_fails_where_its_run_stops_short_of_its_frames_and_runs_it_again(self):
        # play ends with 0 once it has run its frames, and with another status where it stops short of them. The
        # run keeps its log either way, with that status as its last line, and make native holds the line to exit
        # 0: it fails where the line is another, and names the log. Such a log is never current, so the next make
        # runs play again, where a log that ends with exit 0 is current until play builds again. Nor is a log
        # current that no run finished: an empty one, or one whose last line only ends with 0. play is a stand-in
        # here, STRIPPED's, whose copies from strip each refuse as make native holds them to, so it runs each copy
        # once and their logs stay current. The menus' table is empty, so the slice's scan of it prints no error
        with sliced({**STRIPPED, 'config/advertise/splits.txt': '', 'status.txt': '3\n'}) as (root, path):
            log = root / 'build/engine/native/run.txt'
            ran = f'sa1: {root}/orig 600 1 0\n'
            run = f'  run    build/engine/native/run.txt\n{ran}'
            for copied in (''.join(f'  strip  build/engine/native/{copy}.txt\n' for copy in COPIES), ''):
                failed = (f'{run}exit 3\n{copied}build/engine/native/run.txt: the run ended with exit 3, where one '
                          'that reaches its frames ends with exit 0; read the log for where it stopped\n')
                status, output = make(root, '-o', PLAY, 'native', 'CC=true', path=path)
                self.assertEqual(status, 2, output)
                self.assertRegex(output, rf'\A{re.escape(failed)}.*: \*\*\* '
                                         r'\[engine/mk/native\.mk:\d+: native\] Error 1\n\Z')
                self.assertEqual(log.read_text(), f'{ran}exit 3\n')
            (root / 'status.txt').write_text('0\n')
            for printed in (f'{run}exit 0\n', ''):
                self.assertEqual(make(root, '-o', PLAY, 'native', 'CC=true', path=path), (0, printed))
                self.assertEqual(log.read_text(), f'{ran}exit 0\n')
            for unfinished in ('', 'sa1: stopped at 0\n'):
                log.write_text(unfinished)
                self.assertEqual(make(root, '-o', PLAY, 'native', 'CC=true', path=path), (0, f'{run}exit 0\n'))
                self.assertEqual(log.read_text(), f'{ran}exit 0\n')

    def test_native_stops_where_the_marked_copy_can_t_be_made_again(self):
        # the slice compiles marks.py's copy of the game's C, and make reads the copy's record as a makefile. A copy
        # older than marks.py is made again, and where marks.py fails, make stops and leaves the record as it was,
        # so the next make fails the same way: it never builds the older copy in the new one's place
        record = 'build/engine/native/marked.mk'
        fails = '#!/usr/bin/env python3\nimport sys\nsys.exit("src/a.c: no longer holds: a line")\n'
        with sliced({PLAY: '#!/bin/sh\n', 'config/advertise/splits.txt': '',
                     'engine/tools/marks.py': fails}) as (root, path):
            later = (root / record).stat().st_mtime + 10
            os.utime(root / 'engine/tools/marks.py', (later, later))
            for _ in range(2):
                status, output = make(root, '-o', PLAY, 'native', 'CC=true', path=path)
                self.assertEqual(status, 2, output)
                self.assertRegex(output, r'\A  marks  build/engine/native/src\nsrc/a\.c: no longer holds: a line\n'
                                         rf'.*: \*\*\* \[engine/mk/native\.mk:\d+: {re.escape(record)}\] Error 1\n')
                self.assertNotIn('  run ', output)
                self.assertEqual((root / record).read_text(), '')

    def test_native_fails_where_the_host_failed_a_read_of_the_disc_and_runs_it_again(self):
        # the game goes on from a read of the disc that the host failed, as it does from a sector the drive can't
        # read, so such a run can reach its frames and end with 0. gdfs.c names the file on a line of the log, "sa1:
        # can't read NAME on the disc: REASON", and make native fails on a log with such a line, names the log and
        # the first such line, and runs play again the next time. A line that holds those words past its start is
        # no such line, nor is one of another file than the disc's. play is a stand-in here, which prints its
        # arguments and said.txt, and ends with 0. Its copies from strip, STRIPPED's, each refuse as make native
        # holds them to, so it runs each copy once and their logs stay current
        play = ('#!/bin/sh\ncopy=${0##*/play}\n[ -z "$copy" ] || { cat "said$copy"; exit "$(cat "ended$copy")"; }\n'
                'echo "sa1: $*"\ncat said.txt\n')
        first = "sa1: can't read stg01.PRS on the disc: Input/output error"
        unread = f"{first}\nsa1: can't read set0100s.bin on the disc: it ends before the size it opened with\n"
        with sliced({**STRIPPED, PLAY: play, 'config/advertise/splits.txt': '',
                     'said.txt': unread}) as (root, path):
            log = root / 'build/engine/native/run.txt'
            ran = f'sa1: {root}/orig 600 1 0\n'
            run = f'  run    build/engine/native/run.txt\n{ran}'
            for copied in (''.join(f'  strip  build/engine/native/{copy}.txt\n' for copy in COPIES), ''):
                failed = (f'{run}{unread}exit 0\n{copied}build/engine/native/run.txt: the run reached its frames '
                          'past a read of the disc that the host failed, which the log\'s first such line names: '
                          f'{first}\n')
                status, output = make(root, '-o', PLAY, 'native', 'CC=true', path=path)
                self.assertEqual(status, 2, output)
                self.assertRegex(output, rf'\A{re.escape(failed)}.*: \*\*\* '
                                         r'\[engine/mk/native\.mk:\d+: native\] Error 1\n\Z')
                self.assertEqual(log.read_text(), f'{ran}{unread}exit 0\n')
            other = "sa1: no file sa1: can't read x on the disc: y\nsa1: can't read settings.txt\n"
            (root / 'said.txt').write_text(other)
            for printed in (f'{run}{other}exit 0\n', ''):
                self.assertEqual(make(root, '-o', PLAY, 'native', 'CC=true', path=path), (0, printed))
                self.assertEqual(log.read_text(), f'{ran}{other}exit 0\n')

    def test_the_slice_s_table_of_its_disc_is_each_original_file_s_line_of_sha256sums(self):
        # play holds the disc it is given to the build's: build/engine/native/disc.c gives it each original file's
        # path below orig/ and its hash, as SHA256SUMS lists them, and no file make builds. play links the table's
        # object. A SHA256SUMS that lists no original file fails the rule, as a table of no rows would hold a disc
        # to nothing, and the table stays as the last rule that passed wrote it
        table = 'build/engine/native/disc.c'
        built, first, module = (f'{digit * 64}  {name}\n' for digit, name in zip('123', (
            'build/1st_read/main.bin', 'orig/1ST_READ.BIN', 'orig/SONICADV/MOD.PRS')))
        written = ('#include "native.h"\nconst struct native_disc_sum native_disc_sums[] = {\n'
                   f'    {{"1ST_READ.BIN", "{"2" * 64}"}},\n    {{"SONICADV/MOD.PRS", "{"3" * 64}"}},\n}};\n'
                   'const int native_disc_sum_count = sizeof native_disc_sums / sizeof *native_disc_sums;\n')
        with sliced({'SHA256SUMS': built + first + module, 'config/advertise/splits.txt': ''}) as (root, path):
            self.assertEqual(make(root, table, 'CC=true', path=path), (0, f'  sums   {table}\n'))
            self.assertEqual((root / table).read_text(), written)
            (root / 'SHA256SUMS').write_text(built)
            status, output = make(root, table, 'CC=true', path=path)
            self.assertEqual(status, 2, output)
            self.assertRegex(output, rf'\A  sums   {re.escape(table)}\nSHA256SUMS: no line names a file of orig/\n'
                                     rf'.*: \*\*\* \[engine/mk/native\.mk:\d+: {re.escape(table)}\] Error 1\n\Z')
            self.assertEqual((root / table).read_text(), written)
            status, output = make(root, '-n', '-p', '-o', PLAY, 'native', 'CC=true', path=path)
        self.assertEqual(status, 0, output[:3000])
        self.assertIn('build/engine/native/disc.o', re.search(rf'(?m)^{re.escape(PLAY)}: (.*)$', output)[1].split())

    def test_native_fails_where_a_copy_of_play_that_lacks_its_symbols_does_not_refuse_to_start(self):
        # play finds some of the game's statics by name in its own symbol table, so a play whose file lacks the
        # table, or whose table lacks one of the names, starts no game: it says which, in runner.c's sentence for
        # it, and ends with 2. make native has strip make a copy of each kind, COPIES', runs each on the disc for
        # one vsync, and keeps its log with its status as the last line. It fails, naming the log, on a copy that
        # ends with another status, as one that ran its vsync or died in a draw would, and on one that ends with 2
        # over other words, as any play does that finds no disc, or as either copy does that says the other's
        # sentence. A log it fails is never current. One it passes is, until play builds again. strip and play
        # are STRIPPED's stand-ins here, and play's own run reaches its frames
        runner = re.sub(r'"\s*"', '', (ROOT / 'engine/native/runner.c').read_text())
        self.assertIn(f'"{KEEP}"', runner)
        for _, words, says in COPIES.values():
            self.assertIn(words, says)
            self.assertIn(f'"{says.replace("iamb", "%s")}: %s"', runner)
        self.assertIn('"iamb"', (ROOT / 'engine/sa1/records.c').read_text())
        with sliced({**STRIPPED, 'config/advertise/splits.txt': '', 'status.txt': '0\n'}) as (root, path):
            ahead = f'  run    build/engine/native/run.txt\nsa1: {root}/orig 600 1 0\nexit 0\n'
            for copy, (_, words, says) in COPIES.items():
                log = root / f'build/engine/native/{copy}.txt'
                strip = f'  strip  build/engine/native/{copy}.txt\n'
                refusal = f'sa1: {says}: {KEEP}\n'
                other = next(text for name, text in STRIPPED.items() if name.startswith('said.') and text != refusal)
                for said, status in ((f'sa1: no disc at {root}/orig\n', 2), (other, 2), ('', 2), (refusal, 0),
                                     (refusal, 3)):
                    with self.subTest(copy=copy, said=said, status=status):
                        (root / f'said.{copy}').write_text(said)
                        (root / f'ended.{copy}').write_text(f'{status}\n')
                        failed = (f'{ahead}{strip}build/engine/native/{copy}.txt.tmp: the copy of play that strip '
                                  f'made ended with exit {status}, where one that {words} says so and ends with exit '
                                  '2; read the log for what it did\n')
                        made, output = make(root, '-o', PLAY, 'native', 'CC=true', path=path)
                        self.assertEqual(made, 2, output)
                        self.assertRegex(output, rf'\A{re.escape(failed)}.*: \*\*\* \[engine/mk/native\.mk:\d+: '
                                                 rf'build/engine/native/{copy}\.txt\] Error 1\n\Z')
                        self.assertFalse(log.exists())
                        self.assertEqual((root / f'{log}.tmp').read_text(), f'{said}exit {status}\n')
                        ahead = ''
                # the next make runs this copy, which refuses as it must, ahead of what follows it
                (root / f'said.{copy}').write_text(refusal)
                (root / f'ended.{copy}').write_text('2\n')
                ahead = strip
            for printed in (ahead, ''):
                self.assertEqual(make(root, '-o', PLAY, 'native', 'CC=true', path=path), (0, printed))
            for copy, (flags, _, says) in COPIES.items():
                self.assertEqual((root / f'build/engine/native/{copy}.txt').read_text(),
                                 f'sa1: {says}: {KEEP}\nexit 2\n')
                self.assertEqual((root / f'{PLAY}.{copy}').read_text(), STRIPPED[PLAY])
                self.assertIn(f'{flags}-o {PLAY}.{copy} {PLAY}', (root / 'strip.txt').read_text().splitlines())

    def test_play_links_the_slice_s_host_c_and_wraps_the_pads_and_the_adapter_s_calls(self):
        # the slice's host C is engine/native/'s and engine/sa1/'s, but play.c, the front end's, which play's link
        # names itself. play links each of their objects, and takes the objects among its prerequisites, which hold
        # the inventory of them and what gives the link's command too. Its wraps are the pads' and the adapter's
        sources = {f'engine/{name}.c': '' for name in ('native/runner', 'native/bare', 'sa1/records', 'sa1/play')}
        obj = 'build/engine/native/obj/engine'
        with sliced({**sources, 'config/advertise/splits.txt': ''}) as (root, path):
            status, output = make(root, '-n', '-p', '-o', PLAY, 'native', 'CC=true', path=path)
        self.assertEqual(status, 0, output[:3000])
        names, recipe = re.search(rf'(?m)^{re.escape(PLAY)}: (.*)\n(?:#.*\n)*((?:\t.*\n)+)', output).groups()
        self.assertNotIn(' | ', names)
        self.assertLessEqual({f'{obj}/native/runner.o', f'{obj}/native/bare.o', f'{obj}/sa1/records.o',
                              f'{obj}/sa1/play.o'}, set(names.split()))
        self.assertEqual(recipe.splitlines()[-1],
                         '\t$(Q)$(NATIVE_CC) $(NATIVE_LINK_FLAGS) -o $@ $(filter %.o,$^) $(PC_LIBS) -lstdc++ -lm '
                         '$(NATIVE_CRT_END)')
        self.assertLessEqual({'build/engine/native/objects.txt', 'build/engine/native/request.txt',
                              'engine/mk/native.mk'}, set(names.split()))
        fragment = (ROOT / 'engine/mk/native.mk').read_text()
        self.assertEqual(re.findall(r'(?m)^(NATIVE_\w*WRAPS) := (.*)$', fragment), [(
            'NATIVE_WRAPS', 'pad_scan frame_wait njDrawTriangle3D njDrawPolygon2D njDrawLine3D njDrawTexture '
                            'njDrawPolygon kmRender')])

    def test_each_directory_s_c_plus_plus_builds_objects_in_a_directory_of_their_own(self):
        # a directory is its files' namespace, and make takes the first pattern rule whose source it finds: were
        # two directories' objects one directory's, a source of the second with the stem of one of the first's
        # would build no object. The PC backend, SA1's client and the runtime's core here each hold a same.cpp,
        # and each builds an object of its own: with the host's C++, which CXX names true here, and for Windows
        # with llvm-mingw's, under the slice's directory. The headers are the engine's, empty, since a rule
        # names those its sources read
        headers = {path.relative_to(ROOT).as_posix(): '' for path in (ROOT / 'engine').rglob('*.h')}
        sources = {f'engine/{directory}/same.cpp': '' for directory in ('pc', 'sa1', 'runtime')}
        windows = 'build/engine/native-windows'
        marked = {'config/advertise/splits.txt': '', f'{windows}/marked.mk': ''}
        for host, objects in (
                ({}, {'build/engine/replay': 'pc', 'build/engine/sa1': 'sa1', 'build/engine/runtime': 'runtime'}),
                ({'NATIVE_HOST': 'windows'}, {f'{windows}/{name}': name for name in ('pc', 'sa1', 'runtime')})):
            with self.subTest(host=host), sliced({**headers, **sources, **marked}) as (root, path), \
                    mock.patch.dict(os.environ, host):
                status, output = make(root, '-n', *(f'{made}/same.o' for made in objects), path=path)
                self.assertEqual(status, 0, output)
                for made, directory in objects.items():
                    self.assertRegex(output, rf'(?m) -c -o {made}/same\.o engine/{directory}/same\.cpp$')

    def test_the_windows_slice_ends_at_a_domain_past_0_and_names_the_setting(self):
        # COFF keeps an absolute address in 32 bits, so the slice for Windows links at the domain's 0 alone. make
        # ends as it reads the rules under any other NATIVE_DOMAIN, with the setting it was given and what to give
        windows = 'build/engine/native-windows'
        with sliced({'config/advertise/splits.txt': '', f'{windows}/marked.mk': ''}) as (root, path):
            for domain, refused in (('0x100000000', True), ('0', False)):
                with self.subTest(domain=domain), \
                        mock.patch.dict(os.environ, {'NATIVE_HOST': 'windows', 'NATIVE_DOMAIN': domain}):
                    status, output = make(root, '-n', f'{windows}/request.txt', path=path)
                    self.assertEqual(status, 2 * refused, output)
                    self.assertEqual(re.findall(r'(?m)^engine/mk/native\.mk:\d+: \*\*\* (.*)\.  Stop\.$', output), [
                        "NATIVE_DOMAIN=0x100000000: NATIVE_HOST=windows links the game at the domain's 0 alone, as "
                        'COFF keeps an absolute address in 32 bits: leave NATIVE_DOMAIN unset, or give it 0'] * refused)

    def test_the_slice_ends_where_the_disc_lies_under_a_path_with_a_space_and_says_how_to_link_it(self):
        # make's functions split a path at a space, so the directory orig/ links to would reach the runner as two
        # words. make ends as it reads the rules, and under a path without a space it reads them through
        with sliced({'config/advertise/splits.txt': ''}) as (root, path):
            for name, refused in (('a disc', True), ('disc', False)):
                with self.subTest(name=name), tempfile.TemporaryDirectory() as folder:
                    extract = Path(folder).resolve() / name
                    extract.mkdir()
                    (extract / '1ST_READ.BIN').write_bytes(b'')
                    if (root / 'orig').is_symlink():
                        (root / 'orig').unlink()
                    else:
                        shutil.rmtree(root / 'orig', ignore_errors=True)
                    (root / 'orig').symlink_to(extract)
                    status, output = make(root, '-n', 'build/engine/native/request.txt', path=path)
                    self.assertEqual(status, 2 * refused, output)
                    self.assertEqual(re.findall(r'(?m)^engine/mk/native\.mk:\d+: \*\*\* (.*)\.  Stop\.$', output), [
                        "orig/1ST_READ.BIN lies under a directory whose path holds a space, which make can't name: "
                        'link orig/ to the disc by a path without one'] * refused)

    def test_the_slice_s_stubs_assembly_and_programs_wait_on_a_record_of_native_explore(self):
        # stubs.py, asm2c.py and programs.py fail on what the slice's tables don't allow, and write past it under
        # NATIVE_EXPLORE=1, and under no other value: 0 fails as no setting does. Each one's output waits on a
        # record of the setting, which make writes anew as the setting changes, so a build without it runs each of
        # them again and links nothing that a build with it wrote. The record's first write is dated the epoch, and
        # rebuilds nothing
        made = [f'build/engine/native/{name}' for name in ('stubs.c', 'asm.c', 'programs.c')]
        record = 'build/engine/native/explore.id'
        with sliced({'config/advertise/splits.txt': ''}) as (root, path):
            for setting, held, flag, fresh in (('', 'fail', '', False), ('0', 'fail', '', False),
                                               ('1', 'explore', '--explore', True), ('', 'fail', '', True),
                                               ('yes', 'fail', '', True)):
                with self.subTest(setting=setting), mock.patch.dict(os.environ, {'NATIVE_EXPLORE': setting}):
                    status, output = make(root, '-n', '-p', '-o', PLAY, 'native', 'CC=true', path=path)
                    self.assertEqual(status, 0, output[:3000])
                    self.assertEqual((root / record).read_text(), f'{held}\n')
                    self.assertEqual((root / record).stat().st_mtime > 0, fresh)
                    self.assertEqual(re.search(r'(?m)^NATIVE_EXPLORING := ?(.*)$', output)[1], flag)
                    for target in made:
                        names, recipe = re.search(rf'(?m)^{re.escape(target)}: (.*)\n(?:#.*\n)*((?:\t.*\n)+)',
                                                  output).groups()
                        self.assertIn(record, names.split(), target)
                        self.assertIn('$(NATIVE_EXPLORING)', recipe, target)

    def test_pc_names_no_shc_and_writes_no_stamp_of_wibo_or_shc_on_any_host(self):
        # make pc runs no SHC, so it leaves the stamps of wibo and SHC's releases alone on Linux, where the build
        # runs SHC, as on a Mac. wibo and the releases are in place with no stamp of either, so a goal that may run
        # SHC would write each stamp as make reads the makefiles, under make -n too. A dry run passes, names no
        # stamp, SHC, asmsh or wibo, and writes no stamp. play counts as built here, so the run holds the goal,
        # and not the rules of the game's objects, which the checkout has no sources for
        stamps = [f'build/tools/{name}.id' for name in ('wibo', 'r10', 'r26', 'r32')]
        for uname in (LINUX, *MACS):
            with self.subTest(uname), sliced({PLAY: ''}) as (root, path):
                status, output = make(root, '-n', '-o', PLAY, 'pc', path=path, uname=uname)
                self.assertEqual(status, 0, output)
                for step in (*stamps, 'shc.exe', 'asmsh.exe', 'wibo'):
                    self.assertNotIn(step, output)
                for stamp in stamps:
                    self.assertFalse((root / stamp).exists(), stamp)

    def test_pc_fails_where_the_list_of_the_disc_s_files_fails_and_extracts_none_of_them(self):
        # where orig/ holds the disc's .gdi, make pc lists the .gdi's files in build/disc.txt and extracts each that
        # orig/ lacks, which the game reads as it runs. A list that fails, though it printed files first, fails make
        # pc, which then writes no build/disc.txt and extracts nothing. build/gdi is a stand-in here: asked for a
        # list, it prints listed.txt and ends with status.txt's status, and it writes each file it is asked to
        # extract, holding the file's name. make pc reads the tables, and would refuse an empty one for the menus,
        # so the checkout has none, and the slice's scan of it prints an error ahead of make's own lines
        gdi = ('#!/bin/sh\n[ -f "$2" ] || exit 2\n[ "$1" != list ] || { cat listed.txt; exit "$(cat status.txt)"; }\n'
               'dir=$3\nshift 3\n'
               'for f; do mkdir -p "$dir/$(dirname "$f")" && printf %s "$f" > "$dir/$f" || exit 1; done\n')
        disc = ('1ST_READ.BIN', 'SONICADV/MOD.PRS', 'SONICADV/E_SAI.PRS', 'SA1.MPG')
        files = {**ORIGINALS, 'orig/disc.gdi': '', 'build/gdi': gdi, 'build/tools/vendored.ok': '', PLAY: '',
                 'listed.txt': ''.join(f'{size:10}  {name}\n' for size, name in enumerate(disc)), 'status.txt': '2\n'}
        rest = {f'orig/{name}': name for name in disc[2:]}
        with sliced(files) as (root, path):
            status, output = make(root, '-o', PLAY, 'pc', 'CC=true', path=path)
            self.assertEqual(status, 2, output)
            self.assertRegex(output, r'(?m)^  gdi    orig/disc\.gdi\n.*: \*\*\* '
                                     r'\[engine/mk/native\.mk:\d+: build/disc\.txt\] Error 2\n\Z')
            self.assertFalse((root / 'build/disc.txt').exists())
            self.assertFalse([name for name in rest if (root / name).exists()])
            (root / 'status.txt').write_text('0\n')
            status, output = make(root, '-o', PLAY, 'pc', 'CC=true', path=path)
            self.assertEqual(status, 0, output)
            self.assertRegex(output, r'(?m)^  gdi    orig/disc\.gdi\n\Z')
            self.assertEqual((root / 'build/disc.txt').read_text(), ''.join(f'{name}\n' for name in disc))
            self.assertEqual({name: (root / name).read_text() for name in rest}, rest)


# play math's fragment, which alone names the library's sources, each build's objects and the tables it writes for
# the fast path
PLAY_MATH_MK = 'engine/mk/play_math.mk'
TABLES = ('build/engine/numeric/sine.h', 'build/engine/numeric/sine-console.h')
# the host's compile of a source of the library and its link of a test, under CC=true, and the slice's objects of
# the library
HOST = 'true -std=c17 -Wall -Wextra -Werror -pedantic -O2'
SLICE_MATH = 'build/engine/native/obj/engine/numeric'


def numeric(words):
    """The objects among words, a command's, that lie under a directory named numeric: play math's."""
    return [word for word in words if re.fullmatch(r'\S*/numeric/\S*\.o', word)]


class PlayMath(unittest.TestCase):
    maxDiff = None

    def test_no_file_but_play_math_mk_names_a_source_a_table_or_an_object_of_the_library(self):
        # config/parts.txt gives the library's sources their thing. play_math.mk names each build's objects and
        # compiles them, and a link takes its build's objects from it, so no other makefile and no script of the
        # engine's tests names a source of the library, a table the fragment writes, or an object by a source's
        # name, as math.o is math.c's. A fragment whose sources include the library's header names it as
        # PLAY_MATH_HEADER
        rows = parts.rows(ROOT / parts.PARTS)
        sources = [path for path in tracked('engine') if (row := parts.part(rows, path)) and row.thing == 'play-math']
        self.assertIn('engine/numeric/play_math.h', sources)
        self.assertIn('engine/numeric/fast.c', sources)
        fragment = (ROOT / PLAY_MATH_MK).read_text()
        self.assertRegex(fragment, r'(?m)^PLAY_MATH_HEADER\s*:= engine/numeric/play_math\.h$')
        stems = sorted(Path(path).stem for path in sources if path.endswith('.c'))
        objects = re.compile(rf'(?<![\w-])(?:{"|".join(stems)})\.o\b')
        self.assertEqual(set(objects.findall(fragment)), {'math.o', 'fast.o'})
        for path in (*(name for name in MAKEFILES if name != PLAY_MATH_MK), *tracked('engine/tests/*.py')):
            text = (ROOT / path).read_text()
            with self.subTest(path=path):
                self.assertEqual([name for name in (*sources, *TABLES) if name in text] + objects.findall(text), [],
                                 f"{path} names a source, a table or an object of play math's: take a build's "
                                 f'objects from {PLAY_MATH_MK}, as $(call play_math_reference,DIR) gives the '
                                 "reference's")

    def test_each_link_takes_the_objects_of_its_build_as_play_math_mk_names_them(self):
        # the fragment's rule compiles the library's objects under a directory for each compiler: the host's CC
        # under the engine's flags, and clang under the flags of the host's C for the slice. A console build lies
        # under console/ and compiles with SA_NUMERIC_CONSOLE, and the fast path alone searches the directory of
        # the tables, which the host's reference of each build writes. Each program links the objects of one
        # build: the fast path's test in either answer, asm2c.py's test the host's reference by PLAY_MATH, SA1's
        # adapter's test the slice's reference, and the slice the fast path
        engine = {name: (ROOT / name).read_text() for name in tracked('engine')}
        with checkout({**PINNED, **SEARCHED, **engine, 'config/advertise/splits.txt': ''}) as root:
            status, output = make(root, '-n', '-p', 'engine-play-math', 'engine-native-tools', 'CC=true')
        self.assertEqual(status, 0, output[:3000])
        dry, database = output.split('\n# Make data base', 1)
        commands = [line.split() for line in dry.replace('\\\n', ' ').splitlines()]
        compiled = {words[words.index('-o') + 1]: ' '.join(words) for words in commands if '-c' in words}
        self.assertEqual(compiled, {
            'build/engine/numeric/math.o': f'{HOST} -c -o build/engine/numeric/math.o engine/numeric/math.c',
            'build/engine/numeric/console/math.o':
                f'{HOST} -DSA_NUMERIC_CONSOLE -c -o build/engine/numeric/console/math.o engine/numeric/math.c',
            'build/engine/numeric/fast.o':
                f'{HOST} -Ibuild/engine/numeric -c -o build/engine/numeric/fast.o engine/numeric/fast.c',
            'build/engine/numeric/console/fast.o':
                f'{HOST} -DSA_NUMERIC_CONSOLE -Ibuild/engine/numeric -c -o build/engine/numeric/console/fast.o '
                'engine/numeric/fast.c'})
        linked = {words[words.index('-o') + 1]: numeric(words) for words in commands
                  if '-o' in words and '-c' not in words and numeric(words)}
        reference = ['build/engine/numeric/math.o']
        self.assertEqual(linked, {
            'build/engine/numeric/sine.h.bin': reference,
            'build/engine/numeric/sine-console.h.bin': ['build/engine/numeric/console/math.o'],
            'build/engine/tests/fast': ['build/engine/numeric/fast.o'],
            'build/engine/tests/fast-console': ['build/engine/numeric/console/fast.o']})
        # the fast path's test compiles one source for both builds, and its link takes the C library's math
        for words in commands:
            if '-o' in words and words[words.index('-o') + 1].startswith('build/engine/tests/'):
                target = words[words.index('-o') + 1]
                with self.subTest(target=target):
                    self.assertNotIn('-DSA_NUMERIC_CONSOLE', words)
                    self.assertEqual(words[-1], '-lm')
        self.assertIn(f"PLAY_MATH='{' '.join(reference)}' python3 engine/tests/native_tools.py", dry)
        # the slice's objects, by make's database: no goal here builds them
        rule = {target: re.search(rf'(?m)^{re.escape(target)}: (?!PLAY_MATH_TABLES)(.*)$', database)[1].split()
                for target in ('build/engine/tests/sa1', f'{SLICE_MATH}/fast.o', f'{SLICE_MATH}/math.o')}
        self.assertEqual(numeric(rule['build/engine/tests/sa1']), [f'{SLICE_MATH}/math.o'])
        # each waits on its source, the model math.c includes, the header and the fragment, and on what gives the
        # slice's commands: the engine's make files and the record of the slice's request. A name a rule gives twice
        # counts once
        needs = ['engine/numeric/sh4_fpu_approx.h', 'engine/numeric/play_math.h', PLAY_MATH_MK, 'engine/engine.mk',
                 'engine/mk/native.mk', 'build/engine/native/request.txt']
        self.assertEqual(list(dict.fromkeys(rule[f'{SLICE_MATH}/math.o'])), ['engine/numeric/math.c', *needs])
        self.assertEqual(list(dict.fromkeys(rule[f'{SLICE_MATH}/fast.o'])),
                         ['engine/numeric/fast.c', *needs, 'engine/numeric/math.c', TABLES[0]])
        self.assertIn(f'\n{SLICE_MATH}/fast.o: PLAY_MATH_TABLES := -Ibuild/engine/numeric\n', database)
        self.assertIn('\n\t$(Q)$(NATIVE_CC) $(NATIVE_HOST_CFLAGS) -O2 $(PLAY_MATH_TABLES) -c -o $@ $<\n', database)
        self.assertEqual(numeric(re.search(r'(?m)^NATIVE_OBJ := (.*)$', database)[1].split()), [f'{SLICE_MATH}/fast.o'])
        for flags in ('NATIVE_HOST_CFLAGS', 'NATIVE_SA1_CFLAGS'):
            with self.subTest(flags=flags):
                self.assertIn('-Iengine/numeric', re.search(rf'(?m)^{flags} := (.*)$', database)[1].split())


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


class Diagnostics(unittest.TestCase):
    def test_fails_on_each_diagnostic_but_one_admitted_and_where_the_log_can_t_be_read(self):
        # the Makefile's quiet, after cat of a log written as SHC writes one: each error or warning on a line of its
        # own, marked (E) or (W), among lines that are neither
        line = next(line for line in (ROOT / 'Makefile').read_text().splitlines() if line.startswith('quiet = '))
        warning = 'a.c(3) : C5313 (W) Unused variable "x"\n'
        error = 'a.c(4) : C2225 (E) Undeclared name "y"\n'
        with tempfile.TemporaryDirectory() as root:
            Path(root, 'Makefile').write_text(f'{line}\nout:\n\tcat log $(quiet)\n')
            for log, admitted, passes in (('a.c compiled\n', '', True), (warning, '', False), (error, '', False),
                                          (warning, 'C5313', True), (warning * 2, 'C5313', False),
                                          (warning + error, 'C5313', False)):
                with self.subTest(log=log, admitted=admitted):
                    Path(root, 'log').write_text(log)
                    status, output = make(root, '-B', f'ADMITTED={admitted}')
                    self.assertEqual(status == 0, passes, output)
                    if not passes:
                        self.assertIn(log, output)
            # an awk that exits 2, as awk does where it can't read the log
            Path(root, 'log').write_text('a.c compiled\n')
            Path(root, 'bin').mkdir()
            Path(root, 'bin/awk').write_text('#!/bin/sh\nexit 2\n')
            Path(root, 'bin/awk').chmod(0o755)
            status, output = make(root, '-B', path=f'{Path(root, "bin")}{os.pathsep}{PATH}')
            self.assertNotEqual(status, 0, output)


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


# GATE_MAKE(make=..., bin=...) is the text of bin/make, a stand-in for make that a candidate's tests run in MAKE's
# place. Given accept, it runs the make that make= names, with MAKE naming the stand-in, so accept runs each gate with
# the stand-in. That make's environment sets SH_ELF to gcc, as a person's shell may, and NATIVE and NATIVE_OPT, two of
# the variables engine/mk/native.mk's rules take from it, and NATIVE_X, which no rule reads: accept unsets a variable
# for its name, NATIVE or NATIVE_ and more, and keeps no list of them. Given a gate, the stand-in adds a line to
# bin/gates.txt: its arguments, then the name of each of MAKEFLAGS, MAKELEVEL, SH_ELF, NATIVE, NATIVE_OPT, NATIVE_X
# and V that its environment sets. It fails, as make does, where its arguments are bin/fails.txt's text
GATE_MAKE = ('#!/bin/sh\ncase " $* " in *" accept "*)\n'
             '    exec env SH_ELF=gcc NATIVE=out NATIVE_OPT=-O0 NATIVE_X=1 {make} MAKE={bin}/make "$@";; esac\n'
             'echo "$*${{MAKEFLAGS+ MAKEFLAGS}}${{MAKELEVEL+ MAKELEVEL}}${{SH_ELF+ SH_ELF}}${{NATIVE+ NATIVE}}'
             '${{NATIVE_OPT+ NATIVE_OPT}}${{NATIVE_X+ NATIVE_X}}${{V+ V}}" >> {bin}/gates.txt\n'
             '[ "$*" != "$(cat {bin}/fails.txt 2> /dev/null)" ] || exit 2\n').format
# the environment of a candidate's git and its make accept. It holds none of the host's GIT_ variables, as a hook's
# GIT_DIR, and none of the host's git configuration. It names an author and committer of its own
GIT_ENV = {**{name: value for name, value in os.environ.items() if not name.startswith('GIT_')},
           'GIT_CONFIG_GLOBAL': os.devnull, 'GIT_CONFIG_NOSYSTEM': '1', 'GIT_AUTHOR_NAME': 'a',
           'GIT_AUTHOR_EMAIL': 'a@a', 'GIT_COMMITTER_NAME': 'a', 'GIT_COMMITTER_EMAIL': 'a@a'}
# what a candidate's dc and dc-disc gates leave, which accept sums: the manifest, 1ST_READ.BIN's final ELF and a disc
GATED = {'build/dc/manifest.txt': 'a manifest\n', 'build/dc/link/1st_read/final.elf': 'an ELF\n',
         'build/dc/disc/disc.gdi': 'a gdi\n', 'build/dc/disc/track01.bin': 'a track\n'}
# the record's sum lines for GATED, in sha256sum's order
GATED_SUMS = [f'sum      {hashlib.sha256(text.encode()).hexdigest()}  {path}' for path, text in GATED.items()]


class Accept(unittest.TestCase):
    # accept's gates in the order it runs them, each with the SH_ELF its command line takes from GATE_MAKE's
    # environment
    GATES = ('lint', 'all', 'check', 'dc SH_ELF=gcc', 'dc-disc SH_ELF=gcc', 'pc', 'native', 'engine-test SH_ELF=gcc',
             'test SH_ELF=gcc')

    @contextlib.contextmanager
    def candidate(self):
        """A checkout committed to git, which ignores build/, with GATED's files and os.environ set to GIT_ENV. Until
        the caller is done, make() runs GATE_MAKE's make in MAKE's place, and that make runs MAKE for accept. Yields
        the checkout's root, its commit, a PATH that searches that make's directory first, and the directory."""
        with checkout({**SEARCHED, **GATED, '.gitignore': '/build/\n'}) as root, tempfile.TemporaryDirectory() as bin, \
                mock.patch.dict(os.environ, GIT_ENV, clear=True):
            stand_in = Path(bin, 'make')
            stand_in.write_text(GATE_MAKE(make=MAKE, bin=bin))
            stand_in.chmod(0o755)
            for command in (['init', '-q'], ['add', '-A'], ['commit', '-q', '-m', 'a candidate']):
                subprocess.run(['git', *command], cwd=root, check=True, stdout=subprocess.DEVNULL)
            commit = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=root, check=True, stdout=subprocess.PIPE,
                                    text=True).stdout.strip()
            with mock.patch.object(support, 'MAKE', str(stand_in)):
                yield root, commit, f'{bin}{os.pathsep}{PATH}', Path(bin)

    def test_runs_each_gate_in_turn_as_it_runs_alone_and_records_accepted_once_all_pass(self):
        # of make accept's flags and command-line variables, only -j reaches a gate. SH_ELF, from its environment,
        # reaches dc, dc-disc, engine-test and test alone, on their command lines, and test runs last. NATIVE,
        # NATIVE_OPT and NATIVE_X, from its environment, reach no gate, so pc, which runs once the port's disc is
        # built and takes nothing on its command line, builds the PC game make pc builds with no such variable set.
        # native runs next, ahead of engine-test, and takes nothing either
        with self.candidate() as (root, commit, path, bin):
            status, output = make(root, 'accept', 'V=1', path=path)
            self.assertEqual(status, 0, output)
            gates = (bin / 'gates.txt').read_text().splitlines()
            record = (root / 'build/accept.txt').read_text().splitlines()
        self.assertEqual(gates, [f'-j1 {command}' for command in self.GATES])
        self.assertEqual(record, [f'commit   {commit}',
                                  *(f'gate     0  {bin}/make -j1 {command}' for command in self.GATES), *GATED_SUMS,
                                  'accepted'])
        self.assertTrue(output.endswith('\n'.join([*record, ''])), output)

    def test_sums_what_the_gates_built_and_fails_without_accepted_where_one_is_gone(self):
        # sha256sum -c on the sum lines holds the files to the run that passed. With the disc gone, the gates pass but
        # the record names no sum and says no accepted
        with self.candidate() as (root, commit, path, bin):
            status, output = make(root, 'accept', path=path)
            self.assertEqual(status, 0, output)
            record = (root / 'build/accept.txt').read_text().splitlines()
            sums = ''.join(f'{line[9:]}\n' for line in record if line.startswith('sum      '))
            self.assertEqual(subprocess.run(['sha256sum', '-c', '--quiet', '-'], cwd=root, input=sums, text=True,
                                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT).returncode, 0)
            shutil.rmtree(root / 'build/dc/disc')
            status, output = make(root, 'accept', path=path)
            self.assertNotEqual(status, 0, output)
            record = (root / 'build/accept.txt').read_text().splitlines()
        self.assertEqual(len(record), 1 + len(self.GATES))
        self.assertFalse([line for line in record if not line.startswith(('commit ', 'gate     0  '))], record)

    def test_a_gate_that_fails_stops_the_run_and_its_record_says_so_and_not_accepted(self):
        # a second run fails at all, after a first that accepted. A third, on a tree that differs from its commit,
        # runs no gate. The record of each says where it stopped, and neither says accepted
        with self.candidate() as (root, commit, path, bin):
            status, output = make(root, 'accept', path=path)
            self.assertEqual(status, 0, output)
            (bin / 'gates.txt').unlink()
            (bin / 'fails.txt').write_text('-j1 all\n')
            status, output = make(root, 'accept', path=path)
            self.assertNotEqual(status, 0, output)
            self.assertEqual((bin / 'gates.txt').read_text().splitlines(), ['-j1 lint', '-j1 all'])
            self.assertEqual((root / 'build/accept.txt').read_text().splitlines(),
                             [f'commit   {commit}', f'gate     0  {bin}/make -j1 lint',
                              f'gate     2  {bin}/make -j1 all'])
            (bin / 'gates.txt').unlink()
            (root / 'config/kept.txt').write_text('# a change\n')
            status, output = make(root, 'accept', path=path)
            self.assertNotEqual(status, 0, output)
            self.assertIn(f'changed   M config/kept.txt\nbuild/accept.txt: the tree differs from {commit}\n', output)
            self.assertFalse((bin / 'gates.txt').exists())
            record = (root / 'build/accept.txt').read_text().splitlines()
        self.assertEqual(record, [f'commit   {commit}', 'changed   M config/kept.txt'])

    def test_refuses_another_goal(self):
        # make -j runs the goals at once, so accept's gates would race the other goal's writes
        with self.candidate() as (root, commit, path, bin):
            for goals in (('accept', 'dc'), ('check', 'accept')):
                with self.subTest(goals=goals):
                    status, output = make(root, '-n', *goals, path=path)
                    self.assertNotEqual(status, 0, output)
                    self.assertRegex(output, r'Makefile:\d+: \*\*\* make accept takes no other goal\.  Stop\.\n\Z')
            self.assertFalse((bin / 'gates.txt').exists())

    def test_fails_where_git_fails_after_the_gates_though_it_prints_the_commit(self):
        # after the gates, git rev-parse still prints HEAD, and git status fails having printed nothing. Judged by that
        # output alone, the tree matches the commit
        with self.candidate() as (root, commit, path, bin):
            real = shutil.which('git')
            Path(bin, 'git').write_text(f'#!/bin/sh\nif [ "$1" = status ]; then echo >> {bin}/status.txt; '
                                        f'[ "$(wc -l < {bin}/status.txt)" -lt 2 ] || exit 128; fi\nexec {real} "$@"\n')
            Path(bin, 'git').chmod(0o755)
            status, output = make(root, 'accept', path=path)
            self.assertNotEqual(status, 0, output)
            record = (root / 'build/accept.txt').read_text().splitlines()
        self.assertEqual(len(record), 1 + len(self.GATES))
        self.assertNotIn('accepted', record)

    def test_runs_no_gate_on_an_untracked_file_git_s_configuration_leaves_out(self):
        # status.showUntrackedFiles=no hides an untracked header from git status, though make's scans read it
        with self.candidate() as (root, commit, path, bin):
            subprocess.run(['git', 'config', 'status.showUntrackedFiles', 'no'], cwd=root, check=True)
            (root / 'src/a.h').write_text('')
            status, output = make(root, 'accept', path=path)
            self.assertNotEqual(status, 0, output)
            self.assertFalse((bin / 'gates.txt').exists())
            record = (root / 'build/accept.txt').read_text().splitlines()
        self.assertEqual(record, [f'commit   {commit}', 'changed  ?? src/a.h'])


class Roots(unittest.TestCase):
    def test_names_the_roots_images_py_names(self):
        # the Makefile's scans read ROOTS, and tools/census.py reads the sources under images.ROOTS
        with checkout(SEARCHED) as root:
            status, output = make(root, '-n', '-p', 'help')
        self.assertEqual(status, 0, output[-3000:])
        roots = [line.split(' := ', 1)[1].split() for line in output.splitlines() if line.startswith('ROOTS := ')]
        self.assertEqual(roots, [list(ROOTS)])


class Tables(unittest.TestCase):
    def test_reads_a_gen_row_as_config_does(self):
        # tools/tables.py splits a row at runs of whitespace, tabs as well as blanks, and skips any before its first
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

    def test_names_the_goal_the_host_builds_without_the_disc(self):
        # MOD.PRS decoded is a file make dc reads too, so a Mac doesn't refuse it. Without the disc, make stops
        # there as on Linux. For a goal that isn't make dc's, Linux names make check, which a Mac doesn't run, and a
        # Mac make dc-check
        none = 'orig/ holds none of the 6 files the build needs: 1ST_READ.BIN and 5 under SONICADV/.\n'
        put, _ = self.HOW.splitlines(True)
        on_a_mac = f"{put}  Without the disc, make dc-check builds and sums the port's objects that need none.\n"
        with checkout(self.TREE) as root:
            for uname, how in ((LINUX, self.HOW), *((mac, on_a_mac) for mac in MACS)):
                with self.subTest(uname):
                    status, output = make(root, '-n', 'build/gen/img/MOD.BIN', uname=uname)
                    self.assertNotEqual(status, 0, output)
                    self.assertRegex(output, rf'\A{re.escape(none + how)}Makefile:\d+: \*\*\* no disc in orig/\.  '
                                             r'Stop\.\n\Z')

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

    def test_needs_each_motion_file_as_an_original(self):
        # make holds a motion file to its line of SHA256SUMS, as it does each original, so it needs the file in orig/
        with checkout({**self.TREE, **ORIGINALS, **MOTION}) as root:
            status, output = make(root, '-n')
        self.assertNotEqual(status, 0, output)
        self.assertIn(f'\norig/ lacks 1 of the 7 files the build needs: SONICADV/X_MOT.PRS.\n{self.HOW}', f'\n{output}')

    def test_extracts_from_a_gdi_before_it_compiles(self):
        # extraction checks each file it extracts, so a wrong .gdi fails before any compile
        with checkout({'orig/disc.gdi': ''}) as root:
            status, output = make(root, '-n')
        lines = output.splitlines()
        extract = next(index for index, line in enumerate(lines) if line.startswith('build/gdi extract'))
        compiling = next(index for index, line in enumerate(lines) if 'shc.exe' in line)
        self.assertLess(extract, compiling, output)

    # build/gdi's stand-in: it finds the image by the name make gives it, and writes each file it is asked for as
    # ORIGINALS holds it
    EXTRACT = ('#!/bin/sh\n[ "$1" = extract ] && [ -f "$2" ] || exit 1\ndir=$3\nshift 3\n'
               'for f; do mkdir -p "$dir/$(dirname "$f")" && printf %s "$f" > "$dir/$f" || exit 1; done\n')
    # SHA256SUMS' line for each of ORIGINALS, by its path
    SUMS = {path: f'{hashlib.sha256(data).hexdigest()}  {path}\n' for path, data in sorted(ORIGINALS.items())}

    def test_extracts_from_a_gdi_whose_name_holds_quotes_or_a_dollar(self):
        # a .gdi takes whatever name its owner gives it
        for gdi in ("orig/Sonic Adventure (Tails' copy).gdi", 'orig/disc $HOME "two" `x`.gdi'):
            with self.subTest(gdi=gdi), checkout({**SEARCHED, gdi: '', 'build/gdi': self.EXTRACT,
                                                  'build/tools/vendored.ok': '',
                                                  'SHA256SUMS': ''.join(self.SUMS.values())}) as root:
                (root / 'build/gdi').chmod(0o755)
                settle(root)
                status, output = make(root, 'unpack')
                self.assertEqual(status, 0, output)
                self.assertEqual(output, f'  gdi    {gdi}\n')
                self.assertEqual({path: (root / path).read_bytes() for path in ORIGINALS}, ORIGINALS)

    def test_moves_no_file_into_orig_once_one_fails_its_check(self):
        # a file SHA256SUMS gives another hash fails the check, and so does one it gives no line: GNU's sha256sum -c
        # fails an empty list and macOS's passes one, so make gives it none. Either way make names the file, and
        # orig/ stays as it was
        wrong = 'orig/SONICADV/MOD.PRS'
        for line, message in ((f'{HASH}  {wrong}\n', "orig/disc.gdi: its SONICADV/MOD.PRS isn't the original"),
                              ('', f'{wrong}: not in SHA256SUMS')):
            with self.subTest(message), checkout({**SEARCHED, 'orig/disc.gdi': '', 'build/gdi': self.EXTRACT,
                                                  'build/tools/vendored.ok': '',
                                                  'SHA256SUMS': ''.join({**self.SUMS, wrong: line}.values())}) as root:
                (root / 'build/gdi').chmod(0o755)
                settle(root)
                status, output = make(root, 'unpack')
                self.assertNotEqual(status, 0, output)
                self.assertRegex(output, rf'\A  gdi    orig/disc\.gdi\n{re.escape(message)}\n'
                                         r'.*: \*\*\* \[Makefile:\d+: unpack\] Error 1\n\Z')
                self.assertEqual([path for path in ORIGINALS if (root / path).exists()], [])


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
            for tool in ('tools/relink.py', 'tools/link.py', 'tools/shc_object.py', 'tools/config.py',
                         'tools/images.py', 'tools/tables.py', 'tools/tool.py', 'tools/kept.py', 'tools/code_ranges.py'):
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


class Downloads(unittest.TestCase):
    # the text fetch prints after the path of a download tools/SHA256SUMS doesn't pin
    UNPINNED = ': tools/SHA256SUMS gives it no line, or more than one\n'
    # what make says, after its goals, on Linux on arm64, a host for which the Makefile pins no wheel
    NO_WHEEL = ('clang-format and clang-tidy need x86-64 Linux or macOS, whose wheels make pins, and this is '
                'Linux aarch64; see README.md#build')

    def test_names_each_download_as_it_starts(self):
        # MODULE's sources need R32 and wibo alone; tools/SHA256SUMS lists neither, so make refuses each before curl
        # runs, and -k goes on to the next. CC=true passes the host check, as nothing here compiles
        with checkout(SEARCHED) as root, host() as path:
            status, output = make(root, '-k', 'check', 'CC=true', path=path)
        self.assertNotEqual(status, 0, output)
        for download in ('tools/shc-v5.0r32.tar.gz', WIBO):
            self.assertIn(f'  fetch  {download}\n{download}{self.UNPINNED}', output)

    def test_pins_and_fetches_no_wibo_but_linux_s(self):
        # the build runs no SHC on a Mac, so tools/SHA256SUMS pins no wibo for one, and no rule fetches one. Linux's
        # comes from wibo's release by its own name
        pinned = [line.split()[1] for line in (ROOT / 'tools/SHA256SUMS').read_text().splitlines() if 'wibo' in line]
        self.assertEqual(pinned, [WIBO])
        with checkout(SEARCHED) as root:
            status, output = make(root, '-n', WIBO)
            self.assertEqual(status, 0, output)
            self.assertIn(f'curl -fsSL -o {WIBO}.part https://github.com/decompals/wibo/releases/download/1.2.0/'
                          f'{Path(WIBO).name} && ', output)
            status, output = make(root, '-n', 'tools/wibo-macos')
        self.assertNotEqual(status, 0, output)
        self.assertIn("No rule to make target 'tools/wibo-macos'", output)

    def test_moves_a_wheel_into_place_only_once_tools_sha256sums_lists_its_hash(self):
        # tools/SHA256SUMS gives clang-format's wheel the hash of CURL's file, and clang-tidy's no line, a line only
        # for a longer path, or another hash, so make moves the first into place, refuses the second and installs
        # neither
        for second, refusal in (('', WHEELS[1] + self.UNPINNED),
                                (listing([f'{WHEELS[1]}.old']), WHEELS[1] + self.UNPINNED),
                                (f'{HASH}  {WHEELS[1]}\n', f'{WHEELS[1]}.part: FAILED\n')):
            with self.subTest(refusal=refusal), \
                    checkout({**SEARCHED, 'bin/curl': CURL, 'tools/SHA256SUMS': listing(WHEELS[:1]) + second}) as root:
                (root / 'bin/curl').chmod(0o755)
                status, output = make(root, 'format', path=f'{root / "bin"}{os.pathsep}{PATH}')
                self.assertNotEqual(status, 0, output)
                self.assertIn(refusal, output)
                self.assertEqual((root / WHEELS[0]).read_bytes(), b'wheel')
                self.assertFalse((root / WHEELS[1]).exists())
                self.assertFalse((root / 'tools/llvm/bin').exists())

    def test_installs_clang_format_and_clang_tidy_from_their_host_s_wheels_alone_then_removes_the_wheels(self):
        # pip reads no index, and installs no package a wheel depends on; nothing reads a wheel once it is installed.
        # The Makefile pins a wheel of each tool for x86-64 Linux and for a Mac of either processor, and make takes
        # the two of the host uname names
        for uname in PLATFORMS:
            with self.subTest(uname), checkout(SEARCHED) as root:
                status, output = make(root, '-n', 'format', uname=uname)
                self.assertEqual(status, 0, output)
                wheels = [line.split()[1] for line in wheel_sums(uname).splitlines()]
                self.assertEqual([Path(wheel).name.split('-')[0] for wheel in wheels], ['clang_format', 'clang_tidy'])
                self.assertIn(f'pip install -q --no-index --no-deps --force-reinstall {" ".join(wheels)}\n'
                              f'rm {" ".join(wheels)}\n', output)

    def test_lint_and_format_stop_on_a_host_whose_wheels_it_doesn_t_pin_and_no_other_goal_does(self):
        # the Makefile pins the two tools' wheels for three hosts, and Linux on arm64 is none of them: it can't run
        # the programs x86-64 Linux's wheels hold. make stops before it writes anything, with the tools installed or
        # not. Any other goal runs there as on x86-64 Linux: only a Mac refuses the matching build
        for installed in ({}, INSTALLED):
            with self.subTest(installed=bool(installed)), checkout({**SEARCHED, **installed}) as root:
                for goals in (('lint',), ('format',), ('check', 'format')):
                    for flags in ((), ('-n',)):
                        with self.subTest(goals=goals, flags=flags):
                            status, output = make(root, *flags, *goals, uname='Linux aarch64')
                            self.assertEqual(status, 2, output)
                            self.assertRegex(output, rf'\A{stopped(*goals, said=self.NO_WHEEL)}\Z')
                self.assertEqual(written(root), set())
                self.assertEqual(make(root, '-n', 'check', uname='Linux aarch64'), make(root, '-n', 'check'))

    def test_a_file_that_needs_the_tools_stops_at_their_install_on_a_host_whose_wheels_it_doesn_t_pin(self):
        # a file make lint or make format builds is no goal make stops for as it starts. The rule that installs the
        # wheels stops make there instead, by the same line and under -n too, with the tools installed for another
        # host or not: it names no wheel for curl, which RAN stands in for, to fetch, and leaves an install as it was
        for installed in ({}, INSTALLED):
            with self.subTest(installed=bool(installed)), \
                    scripted({**SEARCHED, **installed, '.clang-format': '', 'bin/curl': RAN}) as root:
                path, before = path_with_bin(root), held(root)
                for goal in (*CLANG_TOOLS, 'build/lint/format.ok'):
                    for flags in ((), ('-n',)):
                        with self.subTest(goal, flags=flags):
                            status, output = make(root, *flags, goal, path=path, uname='Linux aarch64')
                            self.assertEqual(status, 2, output)
                            self.assertRegex(output, rf'\A{stopped(goal, said=self.NO_WHEEL)}\Z')
                self.assertEqual([file for file in held(root) if 'build' not in file.relative_to(root).parts], before)


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
    """Pin root's Makefile to wheels in WHEELS' place, and list them in its tools/SHA256SUMS. The Makefile names each
    wheel with $(WHEEL) for its host's platform tag."""
    text = (root / 'Makefile').read_text()
    for wheel, other in zip(WHEELS, wheels):
        wheel, other = (name.replace(PLATFORMS[LINUX], '$(WHEEL)') for name in (wheel, other))
        assert wheel in text, wheel
        text = text.replace(wheel, other)
    (root / 'Makefile').write_text(text)
    (root / 'tools/SHA256SUMS').write_text(listing(wheels))


def pips(root):
    """How many times make has run the pip of root's tools/llvm."""
    log = root / 'tools/llvm/pip.log'
    return len(log.read_text().splitlines()) if log.exists() else 0


def mismatch(path):
    """A pattern for the two lines sha256sum -c --quiet prints where path alone differs from its hash: the file's
    line and a warning. GNU's sha256sum prints them in that order, and macOS's prints the warning first."""
    line, warning = (re.escape(text) for text in (f'{path}: FAILED\n',
                                                  'sha256sum: WARNING: 1 computed checksum did NOT match\n'))
    return f'(?:{line}{warning}|{warning}{line})'


class Install(unittest.TestCase):
    def test_no_other_make_file_assigns_the_variable_the_install_s_rule_reads(self):
        # the rule that installs the wheels reads UNINSTALLED once make has read every make file, dc.mk and the
        # engine's among them, so an assignment in one of those would decide whether lint's tools install
        others = [path for path in MAKEFILES if path != 'Makefile']
        self.assertIn('dc.mk', others)
        self.assertEqual([path for path in others
                          if re.search(r'(?m)^\s*UNINSTALLED\s*[:+?]?=', (ROOT / path).read_text())], [])

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
            self.assertRegex(output, re.escape(f'  fetch  {WHEELS[1]}\n') + mismatch(f'{WHEELS[1]}.part'))
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
            yield root, f'{root / "bin"}{os.pathsep}{PATH}'

    def assertInstalls(self, root, path, wheels):
        """make format, in root, installs each tool from its wheel of wheels, and make format again runs no pip."""
        for runs in (1, 0):
            ran = pips(root)
            status, output = make(root, 'format', path=path)
            self.assertEqual(status, 0, output)
            self.assertEqual([(root / tool).read_text() if (root / tool).exists() else None for tool in CLANG_TOOLS],
                             [f'#!/bin/sh\n# {Path(wheel).name}\n' for wheel in wheels])
            self.assertEqual(pips(root) - ran, runs, output)


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
                self.assertRegex(output, '(?m)^' + mismatch('build/mod/modules/mod/b.bin'))
                self.assertNotIn('files match', output)

    def test_leaves_the_census_and_each_import_s_uses_to_make_dc(self):
        # make dc's link waits for the census and the uses of the imports, and the matching build's bytes rest on
        # neither. make check holds each import to its definition, in the objects it builds, and plain make doesn't
        for goals, definitions in (((), 0), (('check',), 1)):
            with self.subTest(goals=goals), self.built() as root:
                status, output = make(root, '-n', *goals, 'CC=true')
                self.assertEqual(status, 0, output)
                self.assertNotIn('tools/census.py', output)
                self.assertEqual(output.count('python3 tools/imports.py --definitions\n'), definitions)
                self.assertEqual(output.count('tools/imports.py'), definitions)

    def test_check_holds_each_import_to_the_definition_in_its_source_s_object(self):
        # 1ST_READ.BIN takes a_init from mod, whose tables put it at the start of a.c's code. make check passes
        # where a.c's object defines it there. Where the object defines it 8 bytes on, make check fails, naming
        # the import's row, before it checks a hash
        imported = {**HASHED, 'config/1st_read/imports.txt': 'a_init  mod\n'}
        for offset, passes in ((0, True), (8, False)):
            a_obj = obj('a', {'P': (0x10, 4, True)}, defines=[definition('_a_init', offset)])
            with self.subTest(offset=offset), self.built(files={**imported, 'build/modules/mod/a.obj': a_obj}) as root:
                status, output = make(root, 'check', 'CC=true')
                if passes:
                    self.assertEqual((status, output), (0, '  sums   3 files match SHA256SUMS\n'))
                    continue
                self.assertEqual(status, 2, output)
                self.assertEqual(output.splitlines()[0],
                                 'imports: config/1st_read/imports.txt:1: a_init: build/modules/mod/a.obj defines '
                                 'a_init at P + 0x8, and mod puts it at 0x0c900000, in P')
                self.assertNotIn('files match', output)

    def test_holds_each_motion_file_to_its_line_and_leaves_its_words_to_make_dc(self):
        # make holds the motion file to its line, as it does each original, and reads it no further. Its decoded image
        # holds 0x8c010000, an address in 1ST_READ.BIN's image. Nothing but the word's bits says it is a pointer, and
        # make dc's census fails on such a word. Plain make passes only because it runs no census
        listed = {**LISTED, 'orig/SONICADV/X_MOT.PRS': b'X_MOT'}
        files = {**HASHED, **listed, **MOTION,
                 'build/verified/SONICADV/X_MOT.PRS.ok': '',
                 'build/gen/img/X_MOT.BIN': (0x8C010000).to_bytes(4, 'little') + bytes(4),
                 'SHA256SUMS': ''.join(f'{hashlib.sha256(data).hexdigest()}  {path}\n'
                                       for path, data in sorted(listed.items()))}
        with self.built(files=files) as root:
            self.assertEqual(make(root, 'CC=true'), (0, '  sums   13 files match SHA256SUMS\n'))
            # make decodes no motion file for a step of its own
            (root / 'build/gen/img/X_MOT.BIN').unlink()
            status, output = make(root, '-n', 'CC=true')
        self.assertEqual(status, 0, output)
        self.assertNotIn('X_MOT', output)
        self.assertNotIn('tools/census.py', output)

    @contextlib.contextmanager
    def built(self, *args, files=HASHED):
        """The checkout of files, HASHED unless given, settled once make -n check with CC=true and args has stamped
        the tools and written build/deps.mk."""
        with checkout(files) as root:
            status, output = make(root, '-n', 'check', 'CC=true', *args)
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
        files = {**HASHED, WIBO: 'wheel', 'bin/curl': CURL, 'tools/SHA256SUMS': listing([WIBO])}
        with self.built(files=files) as root:
            (root / 'bin/curl').chmod(0o755)
            (root / WIBO).unlink()
            status, output = make(root, 'check', 'CC=true', path=f'{root / "bin"}{os.pathsep}{PATH}')
        self.assertEqual((status, output), (0, f'  fetch  {WIBO}\n  sums   3 files match SHA256SUMS\n'))

    def test_builds_the_vendored_tools_again_with_another_cc_and_again_with_the_one_before(self):
        # CC's stamp holds its command, and a make with the same CC leaves its date alone
        with self.built() as root:
            for cc, builds in (('other-cc', True), ('true', True), ('true', False)):
                status, output = make(root, '-n', 'build/prskit', f'CC={cc}')
                self.assertEqual(status, 0, output)
                self.assertEqual(f'{cc} -std=c99' in output, builds, (cc, output))
                settle(root)

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
        calls = [call for path in ('Makefile', 'config/compilers.mk', 'dc.mk', 'config/gcc.mk')
                 for call in shells((ROOT / path).read_text())
                 if 'python3' in call]
        self.assertTrue(calls)
        for call in calls:
            self.assertRegex(call, r'PYTHONPYCACHEPREFIX=\$\(PYTHONPYCACHEPREFIX\) python3 ')


class Quiet(unittest.TestCase):
    def test_hides_each_command_unless_v_is_set(self):
        # Q is @ unless V is set, so each command a recipe runs opens with it; a status line prints under either, and
        # a make function that runs no command needs neither
        text = ''.join((ROOT / path).read_text() for path in ('Makefile', 'dc.mk')).replace('\\\n', ' ')
        for line in re.findall(r'^\t.*', text, re.M):
            self.assertRegex(line, r'^\t\+?\$\$?\(Q\)|^\t@\$\$?\(call status,|^\t\$\((file|if|info|error) ')

    def test_a_make_the_tests_run_takes_no_setting_from_their_environment(self):
        # make test V=1 puts V=1 in the tests' environment, and a make that took it would print the commands whose
        # output the tests read. One that took SH_ELF or CC would run a tool the test doesn't name. MAKE=1 there
        # changes no make either: make() runs the one bound as the tests started
        probe = 'all:\n\t@echo $(foreach name,{},$(origin $(name)))\n'.format(' '.join(SETTINGS))
        with tree({'Makefile': probe}) as root, mock.patch.dict(os.environ, dict.fromkeys(SETTINGS, '1')):
            status, output = make(root)
        self.assertEqual(status, 0, output)
        self.assertNotIn('environment', output.split())


class Test(unittest.TestCase):
    def test_hands_the_tests_the_make_that_runs_it_and_a_dry_run_runs_no_test(self):
        # the tests run make in checkouts of their own, with the make the recipe names in MAKE. make runs a recipe
        # line that names $(MAKE) even under -n, so the recipe names it through a variable, and make -n test prints
        # its two commands and runs neither
        with checkout(SEARCHED) as root:
            status, output = make(root, '-n', 'test')
        self.assertEqual((status, output), (0, f"MAKE='{MAKE}' PYTHONPATH=tools:tests python3 -m unittest discover "
                                               '-s tests -q\npython3 tools/vendor.py --check\n'))

    def test_the_tests_run_the_make_their_environment_names_as_they_start_or_else_make(self):
        # by its path, so that each test runs one make whatever PATH the test gives it
        for named, bound in ((sys.executable, sys.executable), ('sh', shutil.which('sh')),
                             (None, shutil.which('make') or 'make')):
            env = {name: value for name, value in os.environ.items() if name != 'MAKE'}
            env['PYTHONPATH'] = os.pathsep.join(str(ROOT / path) for path in ('tools', 'tests'))
            if named:
                env['MAKE'] = named
            with self.subTest(named=named):
                found = subprocess.run([sys.executable, '-B', '-c', 'import support; print(support.MAKE)'],
                                       cwd=ROOT, env=env, check=True, stdout=subprocess.PIPE, text=True).stdout
                self.assertEqual(found, f'{bound}\n')

    def test_the_makefiles_ask_uname_for_their_host_once(self):
        # make() answers one question for the host a test names, the Makefile's, which assigns the answer to HOST.
        # Every other makefile reads HOST, so the host a test names is the host of every rule
        asked = [(path, line) for path in MAKEFILES for line in (ROOT / path).read_text().splitlines()
                 if 'uname' in line.split('#')[0]]
        self.assertEqual([path for path, _ in asked], ['Makefile'], asked)
        self.assertRegex(asked[0][1], r'\AHOST\s*:=\s*\$\(shell uname -sm\)\Z')

    def test_a_make_the_tests_run_reads_the_host_they_name_from_uname(self):
        # the Makefile takes its host from uname -sm. make() answers it with x86-64 Linux, or the host a test names,
        # ahead of the PATH it searches, so no test depends on the host that runs it. The stand-in fails on any other
        # question, which the host's own uname would answer for the host that runs the tests
        with tree({'Makefile': 'all:\n\t@echo $(shell uname -sm)\nsystem:\n\t@uname -s\n'}) as root, host() as path:
            self.assertEqual(make(root), (0, f'{LINUX}\n'))
            self.assertEqual(make(root, path=path), (0, f'{LINUX}\n'))
            for mac in MACS:
                self.assertEqual(make(root, uname=mac), (0, f'{mac}\n'))
            status, output = make(root, 'system')
            self.assertNotEqual(status, 0, output)
            self.assertEqual(output.splitlines()[0],
                             "uname: the tests' stand-in answers -sm alone, and this is uname -s")


if __name__ == '__main__':
    unittest.main()
