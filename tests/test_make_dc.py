import ast
import contextlib
import hashlib
import os
import re
import shlex
import shutil
import tempfile
import unittest
import unittest.mock
from pathlib import Path

import csource
import memory
import support
import test_ld
from support import (A_TABLE, BODIES, BSDTAR, BUILT, CC, COMMANDS, COMPILERS, CURL, FOUND, GNU_TAR, HASH, HASHED, HOLD,
                     LIBLZMA, LINUX, MACS, MAKE, MEASURE, MOD_DATA, MODULE, MOTION, NO_ROWS, ORIGINALS, PINNED, RAN,
                     SEARCHED, STUB_SHC, WIBO, ZLIB, depends, host, listing, make, path_with_bin, placing,
                     settle, tabled)

ROOT = Path(__file__).resolve().parents[1]
# a config/gcc.mk that names no source, with the flags of GNU as and SH4ZAM's include directory, which dc.mk reads
GCC_MK = ('GAS_CHECKS := --isa=sh4\nGASFLAGS := $(GAS_CHECKS) --no-pad-sections\n'
          'SH4ZAM_INCLUDE := tools/sh4zam/include\n')
# a config/dreamcast.mk whose engine/dc/b.c replaces mod's b.src, and one whose engine/dc/c.c replaces c.src
B_REPLACED = {'config/dreamcast.mk': 'DC_REPLACED := modules/mod/b.src:engine/dc/b.c\n'}
C_REPLACED = {'config/dreamcast.mk': 'DC_REPLACED := modules/mod/c.src:engine/dc/c.c\n'}
# the repository's line of config/gcc.mk that gives a C object GCC's intermediate code, for a test's config/gcc.mk
GCC_LTO = ''.join(line for line in (ROOT / 'config/gcc.mk').read_text().splitlines(True)
                  if line.startswith('GCC_LTO = '))
# cc1's and libgcc.a's paths in the toolchain, which hold GCC's version
CC1 = 'libexec/gcc/sh-elf/16.2.0/cc1'
LIBGCC = 'lib/gcc/sh-elf/16.2.0/libgcc.a'
# the programs of GCC's that the link-time compile runs, by their paths in the toolchain, beside cc1: lto1, the two
# programs the driver runs it and the linker through, and the linker's plugin
LINK_TIME_TOOLS = tuple(f'libexec/gcc/sh-elf/16.2.0/{name}'
                        for name in ('lto1', 'lto-wrapper', 'collect2', 'liblto_plugin.so'))
# the toolchain's files that make dc's stamps hash, by their paths in SH_ELF
SH_ELF_FILES = ('bin/sh-elf-gcc', CC1, 'bin/sh-elf-as', 'bin/sh-elf-ld', 'bin/sh-elf-ar', LIBGCC, *LINK_TIME_TOOLS)
# the toolchain make builds on first use, in place, each file empty
PINNED_TOOLCHAIN = {f'tools/sh-elf/{path}': '' for path in SH_ELF_FILES}


def gccflags():
    """A line of config/gcc.mk that sets GCCFLAGS to the flags the repository's config/gcc.mk gives it."""
    return f'GCCFLAGS := {shlex.join(test_ld.flags())}\n'


@contextlib.contextmanager
def checkout(files=None):
    """A tree make dc runs in: support.checkout()'s, with dc.mk, GCC_MK and files. Its config/imports.txt and
    config/declarations.txt are empty, and its tools/memory.py reads no source. Each file whose text opens with #! is
    executable."""
    files = {'dc.mk': (ROOT / 'dc.mk').read_text(), 'config/gcc.mk': GCC_MK, 'config/dreamcast.mk': '',
             'config/imports.txt': '',
             'config/declarations.txt': '', 'tools/memory.py': memory_of(), **(files or {})}
    with support.checkout(files) as root:
        for path, text in files.items():
            if isinstance(text, str) and text.startswith('#!'):
                (root / path).chmod(0o755)
        yield root


@contextlib.contextmanager
def stood_in(files=None):
    """A checkout() with what GCC_NEEDS stands in or puts in place, the disc's originals and files, settled."""
    with checkout({**SEARCHED, **GCC_NEEDS, **ORIGINALS, **(files or {})}) as root:
        settle(root)
        yield root


def memory_of(*sources):
    """tools/memory.py with sources for its SOURCES, the sources make dc's link depends on: none by default, as the
    checkout holds none of the tree's."""
    return (ROOT / 'tools/memory.py').read_text() + f'\nSOURCES = {tuple(sources)!r}\n'


def compiles(output):
    """Each GCC compile make -n prints, as the words its shell reads, by the object it writes. So a flag that
    gccflags() quoted is the flag itself."""
    lines = (shlex.split(line) for line in output.splitlines() if 'sh-elf/bin/sh-elf-gcc' in line.split())
    return {words[words.index('-o') + 1]: words for words in lines if '-o' in words}


def inline(function='main', tag='open: every instruction is one SHC writes for C'):
    """A C source that defines function as one inline body, with tag as the text of its @asm tag."""
    return (f'/* @asm {tag} */\n#pragma inline_asm({function})\nvoid {function}(void)\n'
            '{\n        RTS\n        NOP\n}\n')


def imported(tools):
    """Each of tools, names of tools/, and each module of tools/ one imports, directly or through another, as its
    path."""
    found, left = set(), [f'tools/{tool}.py' for tool in tools]
    while left:
        path = left.pop()
        if path not in found:
            found.add(path)
            for node in ast.walk(ast.parse((ROOT / path).read_text())):
                names = ([alias.name for alias in node.names] if isinstance(node, ast.Import) else
                         [node.module] if isinstance(node, ast.ImportFrom) and node.module else [])
                left += [f'tools/{name}.py' for name in names if (ROOT / f'tools/{name}.py').is_file()]
    return found

# make dc's link, its spec's writer and its publisher stood in by scripts that print nothing. The writer writes an
# empty spec, of no image, which tools/ld.py links without a pass. The publisher writes each image's final ELF and
# files, empty, and then the manifest, at the path its last argument gives
LINK = {'tools/spec.py': "import sys\nopen(sys.argv[-1], 'w').close()\n",
        'tools/publish.py': ("import sys\nfrom pathlib import Path\nsys.path[:0] = ['tools']\nimport images\n"
                             "for name, path in images.files().items():\n"
                             "    for each in (f'build/dc/link/{name}/final.elf', f'build/dc/{path}',\n"
                             "                 *([f'build/dc/{path[:-4]}.BIN'] if path.endswith('.PRS') else [])):\n"
                             "        Path(each).parent.mkdir(parents=True, exist_ok=True)\n"
                             "        Path(each).write_bytes(b'')\n"
                             "open(sys.argv[-1], 'w').write('a manifest\\n')\n")}
EMPTY_HASH = hashlib.sha256(b'').hexdigest()
# what make dc needs beyond a checkout(): its toolchain and build/shckit stood in by scripts, its link stood in by
# LINK, and what its checks read, in place. A test passes CC=true to pass the host check. Here the host's C compiler
# would only write main.c's reads, which are in place. A test that runs make dc for real gives each run before it
# CC=true as well: once CC changes, make writes the reads again, which true can't
GCC_NEEDS = {
    # the toolchain's files, empty but for the scripts below. The linker and libgcc.a stay empty: LINK's spec links no
    # image, and only a test that runs make -n gives an image a content to link
    **{f'sh-elf/{path}': '' for path in SH_ELF_FILES},
    # GCC writes the .d file -MF names, a rule of -MT's target and its source alone. Its preprocessor copies the
    # source, and its compile copies the C to the object
    'sh-elf/bin/sh-elf-gcc': ('#!/bin/sh\nwhile [ $# -gt 1 ]; do case $1 in -E) e=1;; -MF) d=$2;; -MT) t=$2;; '
                              '-o) out=$2;; esac; shift; done\n[ -z "$d" ] || echo "$t: $1" > "$d"\n'
                              'if [ "$e" ]; then cat "$1"; else cp "$1" "$out"; fi\n'),
    # GNU as and shckit's translate copy their input
    'sh-elf/bin/sh-elf-as': '#!/bin/sh\nwhile [ $# -gt 1 ]; do [ "$1" = -o ] && out=$2; shift; done\ncp "$1" "$out"\n',
    'build/shckit': '#!/bin/sh\n[ "$1" = translate ] && [ "$2" = -o ] && cp "$4" "$3"\n',
    # the archiver writes an empty archive
    'sh-elf/bin/sh-elf-ar': '#!/bin/sh\nshift\n: > "$1"\n',
    # settle() dates them after their sources, so make makes neither again
    'build/prskit': '',
    'build/tools/vendored.ok': '',
    # the pinned tools, with stamps that hold their hashes as make writes them, so no SHC runs
    **PINNED,
    **{f'build/tools/{name}.id': f'{EMPTY_HASH}  {tool}\n'
       for name, tool in (('wibo', WIBO),
                          *((release, f'tools/shc-v5.0{release}/shc.exe') for release in ('r10', 'r26', 'r32')))},
    # main.c's reads, which the census reads
    'build/reads/src/main.d': depends('build/reads/src/main.d', 'src/main.c'),
    # no name past the image, and no row in the census: census.py --dc reads 1ST_READ.BIN's GCC objects for a row of
    # symbols.txt that isn't placed, and here each object is a copy of its C
    'config/1st_read/symbols.txt': '0x8c0101a0  main\n0x8c0101a4  shared\n',
    'config/1st_read/census.txt': '',
    **LINK,
}
# b.src as one function, b, whose @asm tag is abi
ABI = ('        .EXPORT     _b\n\n        .SECTION    P,CODE,ALIGN=4\n\n'
       '; @asm abi: stack-written, MOV R4,R15 at 0x0c900020 sets the stack pointer\n_b:\n        RTS\n'
       '        MOV         R4,R15\n        .END\n')
# SH4ZAM's library as its fetch leaves it. It holds a LICENSE and a header, and the SH-4 back end's C and assembly,
# which the build compiles. It holds three sources the build leaves out too: the software back end's, shz_version.c
# and an .inc.c. tools/SHA256SUMS gives each file its line, and the fetch's record holds those lines
LIBRARY = {f'tools/sh4zam/{path}': '' for path in ('LICENSE', 'include/sh4zam/a.h', 'source/sh4/a.c', 'source/sh4/b.s',
                                                    'source/shz_version.c', 'source/sw/c.c', 'source/d.inc.c')}
LIBRARY['tools/SHA256SUMS'] = ''.join(f'{hashlib.sha256(text.encode()).hexdigest()}  {path}\n'
                                      for path, text in LIBRARY.items())
LIBRARY['tools/sh4zam/fetched.txt'] = LIBRARY['tools/SHA256SUMS']
# an archiver that keeps a set of members, as ar r does: it adds each one it names, and keeps the rest
AR = ('#!/bin/sh\nshift\nout=$1; shift\n{ [ -f "$out" ] && cat "$out"; for m; do echo "$m"; done; } | sort -u > '
      '"$out.new" && mv "$out.new" "$out"\n')
# a build/prskit whose compress writes "compressed" and then the file it is given
COMPRESSES = '#!/bin/sh\n[ "$1" = compress ] && { echo compressed; cat "$2"; } > "$3"\n'
# what make dc-check needs beyond a checkout(): of GCC_NEEDS, the toolchain, build/shckit and 1ST_READ.BIN's tables,
# which give the census no number to explain, COMPRESSES as build/prskit, and CC as bin/cc, the host's C compiler,
# which writes the reads. It holds no wibo and no release of SHC, no stamp or object of either, and no original
CHECK_NEEDS = {**{path: text for path, text in GCC_NEEDS.items()
                  if path.startswith(('sh-elf/', 'build/shckit', 'config/1st_read/'))},
               'build/prskit': COMPRESSES, 'build/tools/vendored.ok': '', 'bin/cc': CC}
# a tree of whose objects make dc-check leaves two to make dc: a.c's, which takes a body from the disc, and the
# content of mod, which has a gen/ range. It builds main.c's and b.src's, that of other.c, which DC_ADDED gives
# 1ST_READ.BIN, and LIBRARY's, which AR archives by name. prskit's library holds a line. Two of its sources have
# reads: a.c, a body source, and main.c, which 1ST_READ.BIN places. a.c has its table of offsets
A_BODY = 'static int a[1] = {\n#include "mod/a/a.inc"\n};\n'
PARTLY_CHECKED = tabled({
    'modules/mod/a.c': A_BODY,
    'config/mod/splits.txt': MODULE['config/mod/splits.txt'] + 'gen/mod/d_0c900180.c  D  0x0c900180  0x0c900200\n',
    **MOD_DATA, 'engine/dc/other.c': 'own other\n', 'config/dreamcast.mk': 'DC_ADDED := 1st_read:engine/dc/other.c\n',
    **LIBRARY, 'sh-elf/bin/sh-elf-ar': AR, 'tools/prskit/prskit.c': 'a library\n',
})
# the files make dc-check sums in that tree, in the order of their paths: each as build/prskit compresses it, the
# objects and the archive, and the toolchain's libgcc.a
SUMMED = ['build/dc/check/libsh4zam.a.prs', 'build/dc/check/prskit.c.prs', 'build/dc/engine/dc/other.o',
          'build/dc/modules/mod/b.o', 'build/dc/sh4zam/libsh4zam.a', 'build/dc/sh4zam/sh4/a.o',
          'build/dc/sh4zam/sh4/b.o', 'build/dc/src/main.o', f'sh-elf/{LIBGCC}']
# the objects among them, which the inventory of make dc-check's objects lists
SUMMED_OBJECTS = [path for path in SUMMED if path.endswith('.o')]
# the line make dc-check prints as it runs the import check, which it does once a file the check reads changes
IMPORT_LINE = '  import config/1st_read/imports.txt\n'
# the line make dc-check prints as it checks the tables of offsets of that many body sources against them
CHECKS_TABLES = '  offset {} tables\n'.format
# the line make dc-check prints as it runs the census, which it does on each run
CENSUS_LINE = '  census config/1st_read/census.txt\n'
# the lines make dc-check prints as it runs the import check, builds and sums them, checks a.c's table, and then
# runs the census
CHECKS = (IMPORT_LINE + '  gcc    src/main.c\n  gcc    engine/dc/other.c\n  gas    modules/mod/b.src\n'
          '  gcc    tools/sh4zam/source/sh4/a.c\n  gas    tools/sh4zam/source/sh4/b.s\n'
          '  ar     build/dc/sh4zam/libsh4zam.a\n  sums   build/dc/check.txt\n' + CHECKS_TABLES(1) + CENSUS_LINE)
# the reads make dc-check writes in that tree, each as CC writes it, and the one stub include they need, a.c's zero
# body
READS = {f'build/reads/{source[:-2]}.d': depends(f'build/reads/{source[:-2]}.d', source)
         for source in ('modules/mod/a.c', 'src/main.c')}
STUBBED = ['build/stub/modules/mod/a/a.inc']


def sums(root, paths):
    """sha256sum's lines for the files at paths in root, in their order: for each, its hash, two spaces and its
    path."""
    return ''.join(f'{hashlib.sha256((root / path).read_bytes()).hexdigest()}  {path}\n' for path in paths)


def differing(before, after):
    """The path of each line that differs between before and after, two texts of sums for the same paths."""
    return [was.split()[1] for was, now in zip(before.splitlines(), after.splitlines()) if was != now]


class Host(unittest.TestCase):
    def test_dc_names_each_command_it_lacks_before_any_step_and_no_timeout_or_download_of_shc_on_any_host(self):
        # cmp checks each content. make dc runs no SHC, so it asks for no timeout, which runs wibo, and for no curl,
        # tar or gzip to fetch and unpack wibo and SHC's releases, though none is in place, on Linux as on a Mac
        for uname in (LINUX, *MACS):
            with self.subTest(uname), checkout() as root, host(*COMMANDS) as path:
                status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=no-such-cc', path=path, uname=uname)
                self.assertFalse((root / 'build/deps.mk').exists())
                self.assertNotEqual(status, 0, output)
                self.assertIn('*** no-such-cc sha256sum cmp: not found; see README.md#build', output)

    def test_dc_check_needs_no_timeout_no_download_of_shc_and_no_cmp_on_any_host(self):
        # make dc-check runs no SHC and checks no content. So it asks for no timeout, which runs wibo, for no curl, tar
        # or gzip to fetch and unpack wibo and SHC's releases, though none is in place, and for no cmp. Where SH_ELF
        # names the toolchain make builds, which isn't in place either, it asks for what fetches and builds that
        for uname in (LINUX, *MACS):
            with self.subTest(uname), checkout() as root, host(*COMMANDS) as path:
                status, output = make(root, 'dc-check', 'SH_ELF=sh-elf', 'CC=no-such-cc', path=path, uname=uname)
                self.assertNotEqual(status, 0, output)
                self.assertIn('*** no-such-cc sha256sum: not found; see README.md#build', output)
                status, output = make(root, 'dc-check', 'CC=no-such-cc', path=path, uname=uname)
                self.assertNotEqual(status, 0, output)
                self.assertIn('*** no-such-cc sha256sum curl tar gzip xz c++: not found; see README.md#build', output)

    def test_dc_and_dc_check_take_no_need_from_a_goal_that_runs_shc_typed_beside_them(self):
        # on Linux make check runs SHC. Typed beside make dc or make dc-check, before it or after, it still asks for
        # timeout, and for curl, tar and gzip to fetch and unpack wibo and SHC's releases. make dc and make check
        # each ask for cmp too, make dc for its contents and make check for the tables of offsets, and the two typed
        # together name it once. make dc typed beside make dc-check asks for cmp and no more
        with checkout() as root, host(*COMMANDS) as path:
            for goals, shc in ((('dc', 'dc-check'), ''), (('dc-check', 'dc'), ''),
                               *((goals, ' timeout curl tar gzip') for goals in (
                                   ('check', 'dc-check'), ('dc-check', 'check'), ('dc', 'check'), ('check', 'dc')))):
                with self.subTest(goals=goals):
                    status, output = make(root, *goals, 'SH_ELF=sh-elf', 'CC=no-such-cc', path=path)
                    self.assertNotEqual(status, 0, output)
                    self.assertIn(f'*** no-such-cc sha256sum{shc} cmp: not found; see README.md#build', output)

    def test_dc_needs_tar_xz_gzip_and_cpp_to_build_its_toolchain_alone(self):
        # SHC's releases are in place, without the toolchain and then with it
        with checkout(PINNED) as root, host(*COMMANDS) as path:
            status, output = make(root, 'dc', 'CC=no-such-cc', path=path)
        self.assertNotEqual(status, 0, output)
        self.assertIn('*** no-such-cc sha256sum curl tar gzip xz c++ cmp: not found; see README.md#build', output)
        with checkout({**PINNED, 'tools/sh-elf/bin/sh-elf-gcc': ''}) as root, host('curl', 'tar', 'gzip', 'xz', 'c++') \
                as path:
            status, output = make(root, 'dc', 'CC=no-such-cc', path=path)
        self.assertNotEqual(status, 0, output)
        self.assertIn('*** no-such-cc: not found; see README.md#build', output)

    def test_dc_needs_gzip_and_xz_for_its_toolchain_only_where_tar_runs_them(self):
        # the toolchain isn't in place, and the host has neither gzip nor xz. GNU tar runs both to unpack the
        # toolchain's sources. A libarchive tar unpacks by itself what a library it names reads, zlib what gzip
        # compressed and liblzma what xz did, and runs the program where it names no such library. The rule is tar's,
        # and the same on every host
        for tar, lacks in ((GNU_TAR, ' gzip xz'), (f'{BSDTAR} {ZLIB} bz2lib/1.0.8', ' xz'),
                           (f'{BSDTAR} {LIBLZMA} bz2lib/1.0.8', ' gzip'), (f'{BSDTAR} {ZLIB} {LIBLZMA}', '')):
            for uname in (LINUX, *MACS):
                with self.subTest(tar=tar, uname=uname), checkout() as root, host('gzip', 'xz', tar=tar) as path:
                    status, output = make(root, 'dc-check', 'CC=no-such-cc', path=path, uname=uname)
                    self.assertNotEqual(status, 0, output)
                    self.assertIn(f'*** no-such-cc{lacks}: not found; see README.md#build', output)

    def test_dc_needs_cmp_for_each_content_s_check(self):
        # and so does dc-disc, whose link waits for each content's check
        for goal in ('dc', 'dc-disc'):
            with self.subTest(goal), checkout({**PINNED, **PINNED_TOOLCHAIN, 'orig/disc.gdi': ''}) as root, \
                    host('cmp') as path:
                status, output = make(root, goal, 'CC=no-such-cc', path=path)
                self.assertNotEqual(status, 0, output)
                self.assertIn('*** no-such-cc cmp: not found; see README.md#build', output)

    def test_dc_needs_curl_and_sha256sum_to_fetch_sh4zam_s_files(self):
        # the toolchain and SHC's releases are in place. tools/SHA256SUMS lists SH4ZAM's LICENSE, which is missing, or
        # in place with no record of its fetch
        files = {**PINNED, **PINNED_TOOLCHAIN, 'tools/SHA256SUMS': f'{HASH}  tools/sh4zam/LICENSE\n'}
        for library in ({}, {'tools/sh4zam/LICENSE': ''}):
            with self.subTest(library=library), checkout({**files, **library}) as root, \
                    host('curl', 'sha256sum') as path:
                status, output = make(root, 'dc', 'CC=no-such-cc', path=path)
                self.assertNotEqual(status, 0, output)
                self.assertIn('*** no-such-cc sha256sum curl: not found; see README.md#build', output)

class Originals(unittest.TestCase):
    def test_dc_stops_without_the_disc_before_any_compile(self):
        # GCC_NEEDS's toolchain is in place: a GCC that isn't there would stop make first
        with checkout({**SEARCHED, **GCC_NEEDS}) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertNotEqual(status, 0, output)
        self.assertRegex(output, r'\nMakefile:\d+: \*\*\* no disc in orig/\.  Stop\.\n\Z')
        self.assertNotIn('-fdiagnostics-plain-output', output)

    def test_dc_stops_without_the_disc_before_it_builds_the_toolchain_or_fetches_sh4zam(self):
        # SH_ELF is make's own, with no toolchain in place, and tools/SHA256SUMS lists a file of SH4ZAM's that isn't
        # fetched. make stops before it fetches either, and names make dc-check, which builds the port's objects that
        # need no disc, on Linux as on a Mac
        none = 'orig/ holds none of the 6 files the build needs: 1ST_READ.BIN and 5 under SONICADV/.\n'
        how = ("  Put them in orig/ as the disc names them, or put the disc's .gdi and its tracks there.\n"
               "  Without the disc, make dc-check builds and sums the port's objects that need none.\n")
        with checkout({**SEARCHED, **PINNED, 'tools/SHA256SUMS': f'{HASH}  tools/sh4zam/LICENSE\n'}) as root, \
                host() as path:
            for uname in (LINUX, *MACS):
                with self.subTest(uname=uname):
                    status, output = make(root, '-n', 'dc', 'CC=true', path=path, uname=uname)
                    self.assertNotEqual(status, 0, output)
                    self.assertRegex(output, rf'\A{re.escape(none + how)}Makefile:\d+: \*\*\* no disc in orig/\.  '
                                             r'Stop\.\n\Z')


class Downloads(unittest.TestCase):
    def test_fetches_the_toolchain_s_sources_only_as_tools_sha256sums_lists_them(self):
        # tools/SHA256SUMS lists none of them, so make dc refuses the first and builds no toolchain
        with checkout({**SEARCHED, **PINNED, **ORIGINALS}) as root, host() as path:
            status, output = make(root, '-k', 'dc', 'CC=true', path=path)
        self.assertNotEqual(status, 0, output)
        self.assertIn('  fetch  tools/sh-elf.src\ntools/sh-elf.src/binutils-2.47.tar.xz: tools/SHA256SUMS gives it no line, or more than one\n', output)
        self.assertNotIn('  build  tools/sh-elf', output)

    def test_fetches_the_toolchain_s_sources_again_under_b_and_not_once_it_is_in_place(self):
        fetch = 'curl -fsSL -o tools/sh-elf.src/binutils-2.47.tar.xz.part https://ftp.gnu.org/gnu/binutils/binutils-2.47.tar.xz'
        with checkout({**SEARCHED, **PINNED, **ORIGINALS}) as root:
            # the receipt make would write, read before the toolchain is in place
            for path, text in {**PINNED_TOOLCHAIN, 'tools/sh-elf/request.txt': receipt(root)}.items():
                (root / path).parent.mkdir(parents=True, exist_ok=True)
                (root / path).write_text(text)
            status, output = make(root, '-n', 'dc')
            self.assertEqual(status, 0, output)
            self.assertNotIn('tools/sh-elf.src', output)
            status, output = make(root, '-n', '-B', 'tools/sh-elf/bin/sh-elf-gcc')
        self.assertEqual(status, 0, output)
        self.assertIn(fetch, output)

    def test_builds_the_toolchain_from_its_sources_and_installs_it_once_whole(self):
        with self.toolchain() as (root, path):
            status, output = make(root, 'tools/sh-elf/bin/sh-elf-gcc', path=path)
            self.assertEqual(status, 0, output)
            self.assertEqual(output, '  fetch  tools/sh-elf.src\n  build  tools/sh-elf\n')
            self.assertTrue((root / 'tools/sh-elf/bin/sh-elf-as').exists())
            self.assertTrue((root / 'tools/sh-elf/bin/sh-elf-gcc').exists())
            for gone in ('tools/sh-elf.src', 'tools/sh-elf.part', 'build/toolchain'):
                self.assertFalse((root / gone).exists(), gone)
            prefix = f'--prefix={root / "tools/sh-elf.part"}'
            ran = (root / 'ran.log').read_text().splitlines()
        # GMP, MPFR and MPC unpack into GCC's source, which builds them. Each package configures in a directory of
        # its own and installs in tools/sh-elf.part
        self.assertEqual([line.split()[2] for line in ran if line.startswith('tar ')],
                         ['build/toolchain/binutils', 'build/toolchain/gcc', 'build/toolchain/gcc/gmp',
                          'build/toolchain/gcc/mpfr', 'build/toolchain/gcc/mpc'])
        configured = [line.split() for line in ran if line.startswith('configure ')]
        self.assertEqual([words[1:3] for words in configured], [['binutils.build', '../binutils/configure'],
                                                                 ['gcc.build', '../gcc/configure']])
        for words in configured:
            self.assertIn('--target=sh-elf', words)
            self.assertIn(prefix, words)
        self.assertIn('--enable-languages=c', configured[1])
        # all-gcc and configure-target-libgcc build in one make, which leaves their order to GCC's own Makefile.
        # libgcc_tm.h builds before the rest of libgcc
        made = [line for line in ran if line.startswith('make ')]
        self.assertEqual(made[:2], ['make binutils.build all', 'make binutils.build install'])
        self.assertEqual(sorted(made[2:4]), ['make gcc.build all-gcc', 'make gcc.build configure-target-libgcc'])
        self.assertEqual(made[4:], ['make libgcc libgcc_tm.h', 'make gcc.build all-target-libgcc',
                                    'make gcc.build install-gcc', 'make gcc.build install-target-libgcc'])

    def test_builds_the_toolchain_without_zstd_and_libgcc_a_without_its_build_s_times_users_or_paths(self):
        # neither configure looks for the host's zstd. binutils' ar, which archives libgcc.a, writes no date or owner
        # unless a command asks it to. GCC compiles libgcc with the checkout's path, as CURDIR gives it, mapped to "."
        with self.toolchain() as (root, path):
            status, output = make(root, 'tools/sh-elf/bin/sh-elf-gcc', path=path)
            self.assertEqual(status, 0, output)
            ran = (root / 'ran.log').read_text().splitlines()
            mapped = f'--with-debug-prefix-map={root}=.'
        binutils, gcc = (line.split() for line in ran if line.startswith('configure '))
        self.assertIn('--without-zstd', binutils)
        self.assertIn('--without-zstd', gcc)
        self.assertIn('--enable-deterministic-archives', binutils)
        self.assertIn(mapped, gcc)
        self.assertNotIn(mapped, binutils)

    def test_builds_the_toolchain_with_link_time_optimization_and_installs_none_that_lacks_a_program_of_it(self):
        # make dc's C objects carry GCC's intermediate code, which its link compiles. GCC's configure takes
        # --enable-lto, which has its build write lto1 and the linker's plugin, and binutils' --enable-plugins, by
        # which the linker loads one. A build that installed no lto1, or no plugin, fails naming it, and no toolchain
        # takes tools/sh-elf's place, nor a receipt
        with self.toolchain() as (root, path):
            status, output = make(root, 'tools/sh-elf/bin/sh-elf-gcc', path=path)
            self.assertEqual(status, 0, output)
            binutils, gcc = (line.split() for line in (root / 'ran.log').read_text().splitlines()
                             if line.startswith('configure '))
            self.assertIn('--enable-plugins', binutils)
            self.assertIn('--enable-lto', gcc)
            self.assertNotIn('--disable-lto', gcc)
            for word in ('--enable-plugins', '--enable-lto'):
                self.assertIn(f' {word} ', (root / 'tools/sh-elf/request.txt').read_text())
            for tool in LINK_TIME_TOOLS:
                self.assertTrue((root / 'tools/sh-elf' / tool).exists(), tool)
        for lacking in LINK_TIME_TOOLS:
            kept = tuple(tool for tool in LINK_TIME_TOOLS if tool != lacking)
            with self.subTest(lacking), self.toolchain(kept) as (root, path):
                status, output = make(root, 'tools/sh-elf/bin/sh-elf-gcc', path=path)
                self.assertNotEqual(status, 0, output)
                self.assertIn(f"tools/sh-elf.part: GCC's build installed no {Path(lacking).name}, which make dc's "
                              'link-time compile runs\n', output)
                self.assertFalse((root / 'tools/sh-elf').exists())
                self.assertFalse((root / 'tools/sh-elf.part/request.txt').exists())

    def test_builds_the_toolchain_under_the_path_gcc_maps_where_a_symlink_led_to_the_checkout(self):
        # GCC takes a compile's directory from PWD, which a shell keeps as the path that led to it, and maps the
        # checkout's path as CURDIR gives it, through no symlink. Here a symlink led to the checkout, and PWD says so.
        # Each package still configures with PWD under the path the map names
        with self.toolchain() as (root, path), tempfile.TemporaryDirectory() as links:
            link = Path(links, 'checkout')
            link.symlink_to(root)
            with unittest.mock.patch.dict(os.environ, {'PWD': str(link)}):
                status, output = make(link, 'tools/sh-elf/bin/sh-elf-gcc', path=path)
            self.assertEqual(status, 0, output)
            ran = (root / 'ran.log').read_text().splitlines()
            mapped = f'--with-debug-prefix-map={root}=.'
            under = [f'in {root / "build/toolchain" / package}' for package in ('binutils.build', 'gcc.build')]
        self.assertIn(mapped, next(line.split() for line in ran if line.startswith('configure gcc.build ')))
        self.assertEqual([line for line in ran if line.startswith('in ')], under)

    def test_builds_the_toolchain_with_the_jobs_make_takes(self):
        # make() passes -j1 before the test's -j. Each make of the toolchain is this make's own executable, MAKE, and
        # takes this make's -j and none of its other flags, such as -n
        for jobs in ('-j1', '-j3'):
            with self.subTest(jobs), self.toolchain() as (root, path):
                status, output = make(root, '-n', jobs, 'tools/sh-elf/bin/sh-elf-gcc', path=path)
                self.assertEqual(status, 0, output)
                self.assertIn(f' && {MAKE} {jobs} MAKEINFO=true && {MAKE} install MAKEINFO=true)', output)
                self.assertIn(f' && {MAKE} {jobs} all-gcc configure-target-libgcc MAKEINFO=true && '
                              f'{MAKE} {jobs} -C sh-elf/libgcc libgcc_tm.h && '
                              f'{MAKE} {jobs} all-target-libgcc MAKEINFO=true && ', output)

    def test_fetches_each_sh4zam_file_tools_sha256sums_lists_before_any_c_compile(self):
        # tools/SHA256SUMS gives SH4ZAM's LICENSE another hash than CURL's file, or its own
        for sums, fetched in ((f'{HASH}  tools/sh4zam/LICENSE\n', False), (listing(['tools/sh4zam/LICENSE']), True)):
            with self.subTest(fetched=fetched), stood_in({'bin/curl': CURL, 'tools/SHA256SUMS': sums}) as root:
                status, output = make(root, '-k', 'dc', 'SH_ELF=sh-elf', 'CC=true', path=path_with_bin(root))
                self.assertEqual(status == 0, fetched, output)
                self.assertIn('  fetch  tools/sh4zam/LICENSE\n', output)
                self.assertEqual('tools/sh4zam/LICENSE.part: FAILED\n' in output, not fetched, output)
                self.assertEqual('  gcc    src/main.c\n' in output, fetched, output)
                self.assertIn('  gas    modules/mod/b.src\n', output)
                # the fetch's record waits for every file
                self.assertEqual((root / 'tools/sh4zam/fetched.txt').exists(), fetched, output)

    def test_fetches_a_file_of_another_pin_again_before_any_c_compile(self):
        # LICENSE is in place from another pin, with no record of a fetch or with a record of that pin's line.
        # tools/SHA256SUMS gives it the hash of CURL's file, beside a line outside tools/sh4zam/. make dc fetches it
        # again before it compiles, removes old.h, which that pin listed and this one doesn't, and records SH4ZAM's
        # line alone, so the next make dc fetches nothing
        sums = listing(['tools/sh4zam/LICENSE'])
        files = {'bin/curl': CURL, 'tools/SHA256SUMS': sums + f'{HASH}  tools/other\n', 'tools/sh4zam/LICENSE': 'old',
                 'tools/sh4zam/include/old.h': 'old'}
        for record in ({}, {'tools/sh4zam/fetched.txt': f'{HASH}  tools/sh4zam/LICENSE\n'}):
            with self.subTest(record=record), stood_in({**files, **record}) as root:
                path = path_with_bin(root)
                status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true', path=path)
                self.assertEqual(status, 0, output)
                fetch = output.find('  fetch  tools/sh4zam/LICENSE\n')
                self.assertNotEqual(fetch, -1, output)
                self.assertLess(fetch, output.index('  gcc    src/main.c\n'), output)
                self.assertEqual((root / 'tools/sh4zam/LICENSE').read_bytes(), b'wheel')
                self.assertFalse((root / 'tools/sh4zam/include/old.h').exists())
                self.assertEqual((root / 'tools/sh4zam/fetched.txt').read_text(), sums)
                status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true', path=path)
                self.assertEqual(status, 0, output)
                self.assertNotIn('  fetch ', output)

    def test_fetches_every_file_again_after_a_fetch_cut_short(self):
        # pin A's files are in place, with the record of their fetch. Under pin B, LICENSE's fetch passes its line and
        # x.h's fails. Back under pin A, LICENSE holds B's bytes: make dc fetches it again, and refuses it before any
        # compile
        pin_a = ''.join(f'{hashlib.sha256(b"A").hexdigest()}  tools/sh4zam/{path}\n' for path in ('LICENSE', 'x.h'))
        pin_b = listing(['tools/sh4zam/LICENSE']) + f'{HASH}  tools/sh4zam/x.h\n'
        files = {'bin/curl': CURL, 'tools/sh4zam/LICENSE': 'A', 'tools/sh4zam/x.h': 'A',
                 'tools/sh4zam/fetched.txt': pin_a}
        with stood_in(files) as root:
            for sums, refused in ((pin_b, 'x.h'), (pin_a, 'LICENSE')):
                (root / 'tools/SHA256SUMS').write_text(sums)
                settle(root)
                status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true', path=path_with_bin(root))
                self.assertNotEqual(status, 0, output)
                self.assertIn(f'tools/sh4zam/{refused}.part: FAILED\n', output)
                self.assertNotIn('  gcc    src/main.c\n', output)

    def test_fetches_a_file_by_its_own_line_of_tools_sha256sums_not_one_for_a_longer_path(self):
        # a.h's line gives the hash of CURL's file, and a.hpp's, whose path starts with a.h's, another hash
        sums = listing(['tools/sh4zam/a.h']) + f'{HASH}  tools/sh4zam/a.hpp\n'
        with checkout({'bin/curl': CURL, 'tools/SHA256SUMS': sums}) as root:
            path = path_with_bin(root)
            status, output = make(root, 'tools/sh4zam/a.h', path=path)
            self.assertEqual(status, 0, output)
            self.assertEqual((root / 'tools/sh4zam/a.h').read_bytes(), b'wheel')
            status, output = make(root, 'tools/sh4zam/a.hpp', path=path)
        self.assertNotEqual(status, 0, output)
        self.assertIn('tools/sh4zam/a.hpp.part: FAILED\n', output)

    def test_refuses_a_toolchain_built_from_another_request_and_leaves_it_in_place(self):
        # the build writes its receipt last: the sources, their lines of tools/SHA256SUMS, the host and the
        # configures' flags. It names the checkout by the word CURDIR where GCC's configure took its path, and holds
        # no path, so each checkout of a host writes the same receipt. A toolchain whose receipt names another hash,
        # or that has none, as one built before receipts, stops make
        with self.toolchain() as (root, path):
            status, output = make(root, 'tools/sh-elf/bin/sh-elf-gcc', path=path)
            self.assertEqual(status, 0, output)
            written = (root / 'tools/sh-elf/request.txt').read_text()
            for word in ('gcc/gcc-16.2.0/gcc-16.2.0.tar.xz', 'tools/sh-elf.src/gcc-16.2.0.tar.xz',
                         f'host {LINUX} binutils', '--with-cpu=m4-single-only', '--disable-werror', '--without-zstd',
                         '--enable-deterministic-archives', '--enable-lto', '--with-debug-prefix-map=CURDIR=.'):
                self.assertIn(word, written)
            self.assertNotIn(str(root), written)
            self.assertEqual(make(root, 'tools/sh-elf/bin/sh-elf-gcc', path=path),
                             (0, f"{Path(MAKE).name}: 'tools/sh-elf/bin/sh-elf-gcc' is up to date.\n"))
            sums = (root / 'tools/SHA256SUMS').read_text()
            (root / 'tools/SHA256SUMS').write_text(HASH + sums[len(HASH):])
            status, output = make(root, '-n', 'tools/sh-elf/bin/sh-elf-gcc', path=path)
            self.assertNotEqual(status, 0, output)
            self.assertTrue(output.endswith(REFUSED), output)
            (root / 'tools/SHA256SUMS').write_text(sums)
            (root / 'tools/sh-elf/request.txt').unlink()
            status, output = make(root, '-n', 'tools/sh-elf/bin/sh-elf-gcc', path=path)
            self.assertNotEqual(status, 0, output)
            self.assertTrue(output.endswith(REFUSED), output)
            self.assertTrue((root / 'tools/sh-elf/bin/sh-elf-gcc').exists())

    def test_refuses_a_toolchain_built_without_link_time_optimization_before_it_reads_a_stamp(self):
        # a tools/sh-elf that a recipe built with GCC configured --disable-lto: its receipt names that flag and no
        # --enable-plugins, and it holds no lto1 and no plugin, which the linker's stamp hashes. Each goal that reads
        # dc.mk stops on the receipt, which names the remedy, and not on the stamp's refresh, which would name the
        # two files alone
        with checkout({**SEARCHED, **PINNED, **ORIGINALS}) as root:
            written = receipt(root)
            old = written.replace('--enable-lto', '--disable-lto').replace(' --enable-plugins', '')
            self.assertNotIn('lto', old.replace('--disable-lto', ''))
            for path in SH_ELF_FILES:
                if Path(path).name not in ('lto1', 'liblto_plugin.so'):
                    (root / 'tools/sh-elf' / path).parent.mkdir(parents=True, exist_ok=True)
                    (root / 'tools/sh-elf' / path).write_text('')
            (root / 'tools/sh-elf/request.txt').write_text(old)
            for goal in ('dc', 'dc-check', 'dc-order', 'build/dc/src/main.o', 'tools/sh-elf/bin/sh-elf-gcc'):
                with self.subTest(goal):
                    status, output = make(root, '-n', goal)
                    self.assertNotEqual(status, 0, output)
                    self.assertTrue(output.endswith(REFUSED), output)
                    self.assertNotIn('No such file', output)
            self.assertFalse((root / 'build/tools/ld.id').exists())
            self.assertTrue((root / 'tools/sh-elf/bin/sh-elf-gcc').exists())

    def test_refuses_a_toolchain_another_host_built_and_leaves_it_in_place(self):
        # the receipt names the host that built the toolchain, as uname names it to make. A toolchain copied from a
        # host uname names otherwise, whose programs are that host's, stops make, on a Mac of either processor too
        hosts = (LINUX, *MACS)
        for built in hosts:
            with self.subTest(built), self.toolchain() as (root, path):
                status, output = make(root, 'tools/sh-elf/bin/sh-elf-gcc', path=path, uname=built)
                self.assertEqual(status, 0, output)
                self.assertIn(f' host {built} binutils ', (root / 'tools/sh-elf/request.txt').read_text())
                for uname in hosts:
                    status, output = make(root, '-n', 'tools/sh-elf/bin/sh-elf-gcc', path=path, uname=uname)
                    self.assertEqual(status == 0, uname == built, output)
                    self.assertEqual(output.endswith(REFUSED), uname != built, output)
                self.assertTrue((root / 'tools/sh-elf/bin/sh-elf-gcc').exists())

    @contextlib.contextmanager
    def toolchain(self, installed=LINK_TIME_TOOLS):
        """A checkout with stand-ins for curl and tar, and the PATH that finds them. tools/SHA256SUMS gives each of the
        toolchain's sources the hash of CURL's file. tar unpacks each package as a configure, which writes a Makefile
        whose install writes the package's tools under the prefix configure takes, and one in sh-elf/libgcc that makes
        libgcc_tm.h. The install writes each of installed too, the link-time compile's programs unless given, by its
        path under the prefix. tar, configure and those Makefiles' make each add a line to ran.log, which names the
        directory each runs in by its last part. configure adds a second, "in" and that directory as PWD names it."""
        tarballs = [f'tools/sh-elf.src/{name}' for name in ('binutils-2.47.tar.xz', 'gcc-16.2.0.tar.xz',
                                                             'gmp-6.3.0.tar.xz', 'mpc-1.3.1.tar.gz', 'mpfr-4.2.1.tar.xz')]
        with checkout({**SEARCHED, 'bin/curl': CURL, 'tools/SHA256SUMS': listing(tarballs)}) as root:
            log = root / 'ran.log'
            made = f'\techo make $(notdir $(CURDIR)) $@ >> {log}'
            (root / 'bin/configure').write_text(
                f'#!/bin/sh\necho "configure ${{PWD##*/}} $0 $*" >> {log}\necho "in $PWD" >> {log}\n'
                'for a; do case $a in --prefix=*) prefix=${a#--prefix=};; esac; done\n'
                f"mkdir -p sh-elf/libgcc && printf '%s\\n' 'libgcc_tm.h:' '{made}' > sh-elf/libgcc/Makefile\n"
                f"printf '%s\\n' 'all all-gcc configure-target-libgcc all-target-libgcc:' '{made}' \\\n"
                f"    'install install-gcc install-target-libgcc:' '{made}' \\\n"
                '    "\tmkdir -p $prefix/bin $prefix/libexec/gcc/sh-elf/16.2.0 && touch $prefix/bin/sh-elf-as '
                '$prefix/bin/sh-elf-gcc' + ''.join(f' $prefix/{path}' for path in installed) + '" > Makefile\n')
            (root / 'bin/tar').write_text('#!/bin/sh\nwhile [ $# -gt 0 ]; do case $1 in -C) dir=$2; shift;; esac; shift; done\n'
                                          f'echo "tar -C $dir" >> {log}\ncp bin/configure "$dir/configure"\n')
            for script in ('bin/tar', 'bin/configure'):
                (root / script).chmod(0o755)
            yield root, path_with_bin(root)


# the line with which make refuses a toolchain whose receipt isn't the one dc.mk would write
REFUSED = ("tools/sh-elf: its request.txt names another build than dc.mk's; remove tools/sh-elf, and make builds it "
           'again.  Stop.\n')


def receipt(root):
    """The receipt make's build of the toolchain would write in root, as make -n prints its command."""
    status, output = make(root, '-n', 'tools/sh-elf/bin/sh-elf-gcc')
    return re.search(r"^printf '%s\\n' '(.*)' > tools/sh-elf\.part/request\.txt$", output, re.M)[1] + '\n'


class Gcc(unittest.TestCase):
    def test_compiles_each_c_source_and_assembles_each_src_file(self):
        with stood_in() as root:
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertEqual(output, '  gcc    src/main.c\n  gcc    modules/mod/a.c\n  gas    modules/mod/b.src\n'
                                     '  ar     build/dc/sh4zam/libsh4zam.a\n'
                                     '  census config/1st_read/census.txt\n  import config/1st_read/imports.txt\n'
                                     '  link   build/dc/link\n')
            for obj in ('src/main.o', 'modules/mod/a.o', 'modules/mod/b.o'):
                self.assertTrue((root / 'build/dc' / obj).exists(), obj)
        with stood_in() as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn('python3 tools/floats.py --assembly modules/mod/b.src\n'
                      'build/shckit translate -o build/dc/modules/mod/b.s modules/mod/b.src\n'
                      'sh-elf/bin/sh-elf-as --isa=sh4 --no-pad-sections -o build/dc/modules/mod/b.o '
                      'build/dc/modules/mod/b.s\n', output)
        # GCC's preprocessor writes the C, and the float pass, tools/floats.py, respells it as R32 reads it. GCC then
        # compiles it. The float pass takes SH4ZAM's include directory for system headers. The compile's log is kept,
        # and printed where GCC fails, and the .i is removed
        self.assertIn('-fdiagnostics-plain-output -MD -MP -MT build/dc/modules/mod/a.o -MF build/dc/modules/mod/a.d.tmp '
                      '-E modules/mod/a.c > build/dc/modules/mod/a.i && python3 tools/floats.py --shc r32 --system '
                      'tools/sh4zam/include build/dc/modules/mod/a.i && LC_ALL=C sh-elf/bin/sh-elf-gcc ', output)
        self.assertRegex(output, r' -fdiagnostics-plain-output -c -o build/dc/modules/mod/a\.o build/dc/modules/mod/a\.i\) '
                                 r'> build/dc/modules/mod/a\.o\.log 2>&1 \|\| \(cat build/dc/modules/mod/a\.o\.log; false\) && '
                                 r'mv build/dc/modules/mod/a\.d\.tmp build/dc/modules/mod/a\.d && '
                                 r'rm -f build/dc/modules/mod/a\.i\n')

    def test_reaches_no_shc_on_any_host_and_reads_a_body_source_s_bodies_by_its_table_and_its_reads(self):
        # make dc writes a body source's bodies from the disc by the source's table of offsets and its reads, so it
        # compiles no stub and needs no SHC, which the build runs on no Mac. a.c takes a body from the disc. wibo
        # and SHC's releases are in place with no stamp of either, so a goal that may run SHC would write each
        # stamp as make reads the makefiles, under make -n too. A dry run, on Linux as on a Mac, passes: the
        # bodies' command names the table and the reads, and the run names no stamp of wibo or SHC, no stub object
        # or listing, no SHC, no asmsh, no wibo and no download, and writes no stamp. make dc-disc, with the disc's
        # .gdi in place, does alike, and so does a make of a.c's object, a file under build/dc/
        files = {'modules/mod/a.c': A_BODY, A_TABLE: NO_ROWS}
        stamps = [f'build/tools/{name}.id' for name in ('wibo', 'r10', 'r26', 'r32')]
        for uname in (LINUX, *MACS):
            for goal, disc in (('dc', {}), ('dc-disc', {'orig/disc.gdi': '', 'build/gdi': ''}),
                               ('build/dc/modules/mod/a.o', {})):
                with self.subTest(uname, goal=goal), stood_in({**files, **disc}) as root:
                    for path in stamps:
                        (root / path).unlink()
                    status, output = make(root, '-n', goal, 'SH_ELF=sh-elf', uname=uname)
                    self.assertEqual(status, 0, output)
                    self.assertIn(f'python3 {BODIES}\n', output)
                    for step in (*stamps, 'build/stub/modules/mod/a.obj', 'build/stub/modules/mod/a.lis', 'shc.exe',
                                 'asmsh.exe', 'wibo', 'curl'):
                        self.assertNotIn(step, output)
                    for path in stamps:
                        self.assertFalse((root / path).exists(), path)

    def test_runs_gcc_in_the_c_locale(self):
        # GCC runs in the C locale, so its log reads the same on any host. GCC runs twice
        # for a C source an SHC builds, and once each for a replacement and for SH4ZAM's C. GNU as alone assembles
        # SH4ZAM's assembly
        with stood_in({**LIBRARY, 'engine/dc/b.c': '', **B_REPLACED}) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        runs = re.findall(r'(\S+) sh-elf/bin/sh-elf-gcc ', output)
        self.assertEqual(len(runs), 6, output)
        self.assertEqual(set(runs), {'(LC_ALL=C', 'LC_ALL=C'})

    def test_compiles_each_source_s_c_as_tools_floats_py_respells_and_folds_it(self):
        # GCC reads a.c's 0.1F as 0x3dcccccd, the float above SHC's 0x3dcccccc. The stand-in GCC copies the C to the
        # object, which holds the hexadecimal constant that names SHC's value, and c's quotient as SHC's 0x3e2aaaaa.
        # The .i is removed once the object is made
        files = {'modules/mod/a.c': '# 1 "modules/mod/a.c"\nfloat a = 0.1F, b = 0.5F, c = 1.0F / 6.0F;\n'}
        with stood_in(files) as root:
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertEqual((root / 'build/dc/modules/mod/a.o').read_text(),
                             '# 1 "modules/mod/a.c"\nfloat a = 0x1.999998p-4F, b = 0x1p-1F, c = 0x1.555554p-3F  ;\n')
            self.assertFalse((root / 'build/dc/modules/mod/a.i').exists())

    def test_fails_an_object_whose_floating_constant_shc_can_t_read_or_whose_initializer_the_fold_can_t(self):
        # and keeps the .i GCC's preprocessor wrote for it
        for c, problem in (('float a = 1e39F;', "1e39F: 2**128 or above, where SHC warns that its conversion overflows"),
                           ('float a = sqrtf(2.0F);', 'sqrtf(0x1p+1F): a call')):
            with self.subTest(c), stood_in({'modules/mod/a.c': f'# 1 "modules/mod/a.c"\n{c}\n'}) as root:
                status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
                self.assertNotEqual(status, 0, output)
                self.assertIn(f'floats: modules/mod/a.c:1: {problem}\n', output)
                self.assertFalse((root / 'build/dc/modules/mod/a.o').exists())
                self.assertEqual((root / 'build/dc/modules/mod/a.i').read_text(), f'# 1 "modules/mod/a.c"\n{c}\n')

    def test_fails_a_src_file_whose_fdata_constant_asmsh_and_gnu_as_can_read_as_two_floats(self):
        with stood_in({'modules/mod/b.src': "        .FDATA.S    F'16777217.0\n"}) as root:
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertFalse((root / 'build/dc/modules/mod/b.o').exists())
        self.assertNotEqual(status, 0, output)
        self.assertIn("floats: modules/mod/b.src:1: F'16777217.0: on a tie or past one by less than an eighth of a "
                      'step, where asmsh and GNU as can read two floats\n', output)

    def test_compiles_each_replacement_to_the_object_of_the_source_it_replaces(self):
        # engine/dc/b.c replaces b.src. GCC compiles the replacement searching the SDK's headers, the game's and the
        # root
        files = {'engine/dc/b.c': 'own b\n', **B_REPLACED}
        with stood_in(files) as root:
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            compiled = sorted(line for line in output.splitlines() if line.startswith('  gcc '))
            self.assertEqual(compiled, ['  gcc    engine/dc/b.c', '  gcc    modules/mod/a.c', '  gcc    src/main.c'])
            # the stand-in GCC copies the source to the object
            self.assertEqual((root / 'build/dc/modules/mod/b.o').read_text(), 'own b\n')
        with stood_in(files) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        # no SHC builds a replacement, so the float pass reads none of it, and GCC compiles it in one run
        self.assertIn(' -Isdk/include -Isrc -I. -fdiagnostics-plain-output -MD -MP -MT build/dc/modules/mod/b.o '
                      '-MF build/dc/modules/mod/b.d.tmp -c -o build/dc/modules/mod/b.o engine/dc/b.c) > ', output)
        self.assertNotIn('> build/dc/modules/mod/b.i', output)

    def test_leaves_the_replacements_to_make_dc_and_lint(self):
        # asmsh still assembles the .src file a replacement stands for, but no SHC route, progress or review names a
        # replacement
        files = {'engine/dc/b.c': '', **B_REPLACED, **PINNED}
        for goal in ('all', 'check', 'progress', 'review'):
            with self.subTest(goal), stood_in(files) as root:
                status, output = make(root, '-n', goal)
                self.assertEqual(status, 0, output)
                self.assertNotIn('engine/dc/', output)
                if goal in ('all', 'check'):
                    self.assertIn(r"asmsh.exe 'modules\mod\b.src'", output)

    def test_stops_on_an_entry_of_dc_replaced_that_names_no_placed_src_file_or_no_c_source(self):
        # other.src is no image's, main.c is no .src file nor ASMCODE's, none.c is missing, and an entry needs its C
        for entry in ('src/other.src:engine/dc/b.c', 'src/main.c:engine/dc/b.c', 'modules/mod/b.src:engine/dc/none.c',
                      'modules/mod/b.src'):
            files = {'engine/dc/b.c': '', 'config/dreamcast.mk': f'DC_REPLACED := {entry}\n'}
            with self.subTest(entry), stood_in(files) as root:
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
                self.assertNotEqual(status, 0, output)
                self.assertIn(f'{entry}: an entry of DC_REPLACED, and not P.src:C or P.c:C, P.src a .src file an image '
                              "places, P.c one of ASMCODE's and C a C source.  Stop.", output)
                # only make dc's goals check it
                status, output = make(root, '-n', 'check')
                self.assertEqual(status, 0, output)

    def test_compiles_each_source_dc_added_names_to_its_own_object_which_its_image_links(self):
        # each object lies under build/dc/ at its source's path. tools/spec.py takes it as one of its image's own,
        # and the census reads its source as its image's. more.c is NONMATCHING_C's, which no SHC route takes
        files = {'engine/dc/other.c': 'own other\n', 'modules/mod/more.c': 'own more\n',
                 'config/compilers.mk': f'{COMPILERS}NONMATCHING_C := modules/mod/more.c\n',
                 'config/dreamcast.mk': 'DC_ADDED := 1st_read:engine/dc/other.c mod:modules/mod/more.c\n'}
        with stood_in(files) as root:
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertIn('  gcc    engine/dc/other.c\n', output)
            self.assertEqual((root / 'build/dc/engine/dc/other.o').read_text(), 'own other\n')
            self.assertEqual((root / 'build/dc/modules/mod/more.o').read_text(), 'own more\n')
        with stood_in(files) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn(' --add 1st_read build/dc/engine/dc/other.o --add mod build/dc/modules/mod/more.o --list ',
                      output)
        for tool in ('census.py --dc', 'imports.py --uses'):
            self.assertIn(f'\npython3 tools/{tool} --add 1st_read engine/dc/other.c --add mod modules/mod/more.c\n',
                          output)
        # a replacement's entry goes to both tools ahead of the additions
        files = {**files, 'engine/dc/b.c': '', 'config/dreamcast.mk': B_REPLACED['config/dreamcast.mk']}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        for tool in ('census.py --dc', 'imports.py --uses'):
            self.assertIn(f'\npython3 tools/{tool} --replace modules/mod/b.src engine/dc/b.c\n', output)

    def test_stops_on_an_entry_of_dc_added_that_names_no_c_source_or_compiles_to_another_s_object(self):
        # an entry with no image, one for a missing source, two whose objects are a source's, and one whose object is
        # SH4ZAM's
        unnamed = ': an entry of DC_ADDED, and not IMAGE:C, C a C source'
        compiled = ": DC_ADDED names it, and it compiles to a source's object"
        for entry, problem in (('engine/dc/other.c', f'engine/dc/other.c{unnamed}'),
                               ('1st_read:engine/dc/none.c', f'1st_read:engine/dc/none.c{unnamed}'),
                               ('1st_read:src/main.c', f'src/main.c{compiled}'),
                               ('mod:modules/mod/a.c', f'modules/mod/a.c{compiled}'),
                               ('1st_read:sh4zam/sh4/a.c', "build/dc/sh4zam/sh4/a.o: two of make dc's rules "
                                                            'build it, by DC_PAIRED_OBJ DC_SH4ZAM_OBJ')):
            files = {**LIBRARY, 'config/dreamcast.mk': f'DC_ADDED := {entry}\n', 'engine/dc/other.c': '',
                     'sh4zam/sh4/a.c': ''}
            with self.subTest(entry), stood_in(files) as root:
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
                self.assertNotEqual(status, 0, output)
                self.assertIn(f'{problem}.  Stop.', output)
                # only make dc's goals read DC_ADDED
                status, output = make(root, '-n', 'check')
                self.assertEqual(status, 0, output)

    def test_builds_an_object_again_by_its_new_rule_once_its_entry_of_dc_replaced_leaves_or_joins(self):
        # each of b.o's rules names its own source alone, which settle() dates before the object. make -n removes no
        # object
        files = {'engine/dc/b.c': 'own b\n', **B_REPLACED}
        with self.built(files) as root:
            (root / 'config/dreamcast.mk').write_text('')
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertEqual((root / 'build/dc/modules/mod/b.o').read_text(), 'own b\n')
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertIn('  gas    modules/mod/b.src\n', output)
            self.assertEqual((root / 'build/dc/modules/mod/b.o').read_text(), '')
            (root / 'config/dreamcast.mk').write_text(B_REPLACED['config/dreamcast.mk'])
            settle(root)
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertIn('  gcc    engine/dc/b.c\n', output)
            self.assertEqual((root / 'build/dc/modules/mod/b.o').read_text(), 'own b\n')

    def test_forgets_a_replacement_s_old_source_once_build_dc_pairs_txt_is_gone(self):
        # a build with no build/dc/pairs.txt, whose replacement's object has a .d file that names its source where it
        # lay. make removes the .d file, and the object builds again from its source where it lies
        files = {'engine/dc/b.c': 'own b\n', **B_REPLACED}
        with self.built(files) as root:
            (root / 'build/dc/modules/mod/b.d').write_text('build/dc/modules/mod/b.o: old/modules/mod/b.c\n')
            (root / 'build/dc/pairs.txt').unlink()
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertIn('  gcc    engine/dc/b.c\n', output)

    def test_compiles_sh4zam_s_sh_4_back_end_alone_and_archives_it(self):
        # AR keeps a member until the archive goes. The rule removes the archive before each write, and the inventory
        # of its members writes it again, so another pin, which drops a.c and leaves b.s as it was, archives b.o alone
        with stood_in({**LIBRARY, 'sh-elf/bin/sh-elf-ar': AR}) as root:
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertIn('  gcc    tools/sh4zam/source/sh4/a.c\n  gas    tools/sh4zam/source/sh4/b.s\n', output)
            self.assertIn('  ar     build/dc/sh4zam/libsh4zam.a\n', output)
            archive = root / 'build/dc/sh4zam/libsh4zam.a'
            self.assertEqual(archive.read_text(), 'build/dc/sh4zam/sh4/a.o\nbuild/dc/sh4zam/sh4/b.o\n')
            (root / 'tools/sh4zam/source/sh4/a.c').unlink()
            sums = ''.join(line for line in LIBRARY['tools/SHA256SUMS'].splitlines(True)
                           if 'source/sh4/a.c' not in line)
            for path in ('tools/SHA256SUMS', 'tools/sh4zam/fetched.txt'):
                (root / path).write_text(sums)
            settle(root)
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertEqual(archive.read_text(), 'build/dc/sh4zam/sh4/b.o\n')
        with stood_in(LIBRARY) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertNotIn('source/sw/', output)
        self.assertNotIn('shz_version', output)
        self.assertNotIn('d.inc', output)
        # the C compiles in one run, with headers from sdk/include, and the float pass reads none of it
        a = compiles(output)['build/dc/sh4zam/sh4/a.o']
        self.assertEqual(a[a.index('-MF') + 1:a.index('-MF') + 5], ['build/dc/sh4zam/sh4/a.d.tmp', '-c', '-o',
                                                                     'build/dc/sh4zam/sh4/a.o'])
        self.assertEqual(a[a.index('-MF') + 5], 'tools/sh4zam/source/sh4/a.c)')
        self.assertIn('-Isdk/include', a)
        self.assertNotIn('-E', a)
        self.assertNotIn('tools/floats.py', a)
        # GNU as assembles the assembly as it is, little-endian under GAS_CHECKS. GASFLAGS' --no-pad-sections would
        # drop each section's padding
        self.assertIn(' && sh-elf/bin/sh-elf-as --isa=sh4 --little -o build/dc/sh4zam/sh4/b.o '
                      'tools/sh4zam/source/sh4/b.s\n', output)
        self.assertIn('sh-elf/bin/sh-elf-ar rcsD build/dc/sh4zam/libsh4zam.a build/dc/sh4zam/sh4/a.o '
                      'build/dc/sh4zam/sh4/b.o\n', output)

    def test_compiles_sh4zam_s_c_under_the_shared_sets_alone_and_the_tree_s_under_gccflags(self):
        # under the repository's flags, SH4ZAM's C takes each flag of the shared sets, which every C compile takes. It
        # takes none of the tree's sets, which compile its C and lay it out as SHC does: SHC never compiled the
        # library. It keeps the zeros an image's file holds, and its types keep the layout their headers declare. The
        # test's config/gcc.mk takes the repository's line that gives SH4ZAM's objects their flags
        shared, tree = ('GCC_TARGET', 'GCC_ZEROS_IN_DATA', 'GCC_WARNINGS'), ('GCC_AS_SHC', 'GCC_LAYOUT')
        sets = {name: test_ld.flags(name) for name in (*shared, *tree)}
        library = f'GCC_SH4ZAM_FLAGS := {shlex.join(test_ld.flags("GCC_SH4ZAM_FLAGS"))}\n' + ''.join(
            line for line in (ROOT / 'config/gcc.mk').read_text().splitlines(True)
            if line.startswith('build/dc/sh4zam/'))
        with stood_in({**LIBRARY, 'config/gcc.mk': GCC_MK + gccflags() + library}) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        compiled = compiles(output)
        a = compiled['build/dc/sh4zam/sh4/a.o']
        self.assertEqual([flag for flag in test_ld.flags() if flag not in compiled['build/dc/src/main.o']], [])
        self.assertEqual([flag for name in shared for flag in sets[name] if flag not in a], [])
        self.assertEqual([flag for name in tree for flag in sets[name] if flag in a], [])
        self.assertIn('-fno-zero-initialized-in-bss', a)
        self.assertNotIn('-fpack-struct=4', a)

    def test_compiles_an_sh4zam_object_again_whose_header_changes_or_whose_d_file_is_gone(self):
        compile_a = '-c -o build/dc/sh4zam/sh4/a.o tools/sh4zam/source/sh4/a.c)'
        with self.built({**LIBRARY, 'sdk/include/string.h': ''}) as root:
            (root / 'build/dc/sh4zam/sh4/a.d').write_text(
                depends('build/dc/sh4zam/sh4/a.o', 'tools/sh4zam/source/sh4/a.c', 'sdk/include/string.h'))
            settle(root)
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
            self.assertNotIn(compile_a, output)
            os.utime(root / 'sdk/include/string.h')
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
            self.assertEqual(status, 0, output)
            self.assertIn(compile_a, output)
            self.assertIn('sh-elf/bin/sh-elf-ar rcsD build/dc/sh4zam/libsh4zam.a ', output)
            settle(root)
            (root / 'build/dc/sh4zam/sh4/a.d').unlink()
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn(compile_a, output)

    def test_archives_sh4zam_s_objects_again_with_another_archiver_and_compiles_none(self):
        with self.built(LIBRARY) as root:
            with open(root / 'sh-elf/bin/sh-elf-ar', 'a') as file:
                file.write('\n# another\n')
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn('sh-elf/bin/sh-elf-ar rcsD build/dc/sh4zam/libsh4zam.a ', output)
        self.assertNotIn('sh-elf/bin/sh-elf-gcc', output)

    def test_links_every_image_once_its_objects_build_and_writes_the_manifest_last(self):
        # the link first removes the manifest and the disc. tools/spec.py writes the spec, tools/ld.py links by it, and
        # tools/verify.py checks what it links, the declarations under GCC_ABI. tools/publish.py writes the files, and
        # the manifest last. Each runs with the toolchain SH_ELF names
        with stood_in({'config/gcc.mk': f'{GCC_MK}GCC_ABI := -ml -m4-single-only\n'}) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        lines = output.splitlines()
        steps = ['rm -rf build/dc/manifest.txt build/dc/disc build/dc/disc.partial', 'mkdir -p build/dc/link',
                 'SH_ELF=sh-elf python3 tools/spec.py --archive build/dc/sh4zam/libsh4zam.a --list '
                 'build/dc/link/unbound.txt build/dc/link/spec.txt',
                 "SH_ELF=sh-elf python3 tools/ld.py --lto='' build/dc/link/spec.txt build/dc/link",
                 "SH_ELF=sh-elf python3 tools/verify.py --abi='-ml -m4-single-only' build/dc/link/spec.txt "
                 'build/dc/link',
                 'SH_ELF=sh-elf python3 tools/publish.py build/dc/link/spec.txt build/dc/link '
                 'build/dc/manifest.txt']
        at = lines.index(steps[0])
        self.assertEqual(lines[at:at + len(steps)], steps)
        self.assertLess(max(index for index, line in enumerate(lines) if 'sh-elf/bin/sh-elf-gcc' in line), at)

    def test_no_engine_make_file_has_a_target_under_build_dc_or_sets_gccflags_dc_added_or_dc_replaced(self):
        # a run that names an engine goal beside dc reads the engine's rules after dc.mk's, where a rule or a target's
        # variable for a file under build/dc/, or another GCCFLAGS, DC_ADDED or DC_REPLACED, could change what make dc
        # builds in that run alone. A recipe's line and a comment set nothing
        changes = r'(?m)^(?![#\t])(?:[^:=\n]*build/dc/[^:=\n]*:|.*\b(?:GCCFLAGS|DC_ADDED|DC_REPLACED)[ \t]*[:+?!]*=).*'
        for path in (ROOT / 'engine/engine.mk', *(ROOT / 'engine/mk').glob('*.mk')):
            with self.subTest(path=path.name):
                self.assertEqual(re.findall(changes, path.read_text()), [])

    def test_no_rule_turns_on_fast_math_and_none_but_make_dc_s_two_variables_link_time_optimization(self):
        # ENGINE.md#numbers: fast math crosses into the math play reads, so no rule takes it. Link-time optimization
        # compiles play math and picture math as one program, so the engine's rules and config/dreamcast.mk take
        # none. make dc takes it by two variables of config/gcc.mk alone, GCC_LTO for an object and GCC_LTO_LINK for
        # the link, and dc.mk names no such flag itself. The engine's rules and config/dreamcast.mk name neither
        # variable, by which a line of theirs could give an object the flags without spelling one
        fast = r'-ffast-math|-Ofast|-funsafe-math-optimizations|-ffinite-math-only|-fassociative-math|' \
               r'-freciprocal-math'
        whole = r'-flto|-fwhole-program|-fuse-linker-plugin'
        engine = (ROOT / 'config/dreamcast.mk', ROOT / 'engine/engine.mk', *(ROOT / 'engine/mk').glob('*.mk'))
        for path in (ROOT / 'dc.mk', ROOT / 'config/gcc.mk', *engine):
            with self.subTest(path=path.name):
                self.assertNotRegex(path.read_text(), fast)
        for path in (ROOT / 'dc.mk', *engine):
            with self.subTest(path=path.name):
                self.assertNotRegex(path.read_text(), whole)
        for path in engine:
            with self.subTest(path=path.name):
                self.assertNotRegex(path.read_text(), r'GCC_LTO|DC_LTO_OBJ')
        # dc.mk gives GCC_LTO to DC_LTO_OBJ's objects alone, and GCC_LTO_LINK to no compile: only to a tool of
        # tools/, in a recipe's line, as the one word of its --lto
        named = [line for line in (ROOT / 'dc.mk').read_text().splitlines()
                 if 'GCC_LTO' in line and not line.startswith('#')]
        self.assertEqual(named[0], '$(DC_LTO_OBJ): private GCCFLAGS += $(GCC_LTO)')
        self.assertIn("\t$(Q)SH_ELF=$(SH_ELF) python3 tools/ld.py --lto='$(GCC_LTO_LINK)' build/dc/link/spec.txt "
                      'build/dc/link', named[1:])
        for line in named[1:]:
            with self.subTest(line=line):
                self.assertRegex(line, r"^\t\$\(Q\)SH_ELF=\$\(SH_ELF\) python3 tools/\w+\.py --lto='\$\(GCC_LTO_LINK\)' "
                                       r'[^$]*$')
        named = [line.split()[0] for line in (ROOT / 'config/gcc.mk').read_text().splitlines()
                 if re.search(whole, line) and not line.startswith('#')]
        self.assertEqual(named, ['GCC_LTO', 'GCC_LTO_LINK'])

    def test_links_again_once_a_source_the_memory_contract_reads_changes_and_stops_once_one_goes(self):
        # MEMORY_SOURCES is tools/memory.py's SOURCES. The manifest depends on each, so a missing one stops make even
        # where the manifest is current
        sources = ['src/system/loader.h', 'src/progress/recorder.h']
        with stood_in({'tools/memory.py': memory_of(*sources), **dict.fromkeys(sources, '')}) as root:
            status, output = make(root, '-pqn', 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertIn(status, (0, 1), output[-3000:])
            self.assertEqual(re.search(r'(?m)^MEMORY_SOURCES := (.*)$', output)[1].split(), sources)
            rule = re.search(r'(?m)^build/dc/manifest\.txt: (.*)$', output)[1].split()
            self.assertEqual([word for word in rule if word in sources], sources)
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            (root / sources[1]).unlink()
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
        self.assertNotEqual(status, 0, output)
        self.assertIn(f"No rule to make target '{sources[1]}', needed by 'build/dc/manifest.txt'", output)

    def test_links_again_where_a_final_elf_or_a_file_the_manifest_owns_goes_and_nowhere_else(self):
        # LINK's publisher writes each image's final ELF and files, then the manifest
        with self.built() as root:
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertNotIn('link   build/dc/link', output)
            for gone in ('build/dc/link/mod/final.elf', 'build/dc/SONICADV/MOD.PRS', 'build/dc/SONICADV/MOD.BIN'):
                with self.subTest(gone):
                    (root / gone).unlink()
                    status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
                    self.assertEqual(status, 0, output)
                    self.assertIn('link   build/dc/link', output)
                    self.assertTrue((root / gone).exists())

    def test_searches_the_warm_rows_order_from_a_spec_of_its_own_once_the_objects_build_and_links_nothing(self):
        # make dc-order plans the link as make dc does, into build/dc/order/, and tools/order.py links the layout
        # passes it reads there, by the toolchain SH_ELF names. It runs after each object compiles, and runs no
        # link, check or publisher of make dc's, which a record out of date would fail
        with stood_in() as root:
            status, output = make(root, '-n', 'dc-order', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        lines = output.splitlines()
        steps = ['mkdir -p build/dc/order',
                 'SH_ELF=sh-elf python3 tools/spec.py --archive build/dc/sh4zam/libsh4zam.a --list '
                 'build/dc/order/unbound.txt build/dc/order/spec.txt',
                 'SH_ELF=sh-elf python3 tools/order.py build/dc/order/spec.txt build/dc/order']
        at = lines.index(steps[0])
        self.assertEqual(lines[at:at + len(steps)], steps)
        self.assertLess(max(index for index, line in enumerate(lines) if 'sh-elf/bin/sh-elf-gcc' in line), at)
        self.assertEqual([line for line in lines if re.search(r'tools/(ld|verify|publish|census|imports)\.py', line)], [])

    def test_searches_or_packs_the_warm_rows_whole_block_only_where_the_command_line_gives_search_or_pack(self):
        # SEARCH=1 has tools/order.py search the whole block, and PACK=1 has it lay the rows with no byte free among
        # them: each moves every warm row. SEARCH=0 and PACK=0 ask for neither, and neither does one in the
        # environment, which a person may not know is set
        ordered = 'SH_ELF=sh-elf python3 tools/order.py{} build/dc/order/spec.txt build/dc/order'
        with stood_in() as root:
            for given, flag in (('SEARCH=1', ' --search'), ('SEARCH=0', ''), ('SEARCH=', ''), ('PACK=1', ' --pack'),
                                ('PACK=0', ''), ('PACK=', '')):
                with self.subTest(given):
                    status, output = make(root, '-n', 'dc-order', 'SH_ELF=sh-elf', given)
                    self.assertEqual(status, 0, output)
                    self.assertEqual([line for line in output.splitlines() if 'tools/order.py' in line],
                                     [ordered.format(flag)])
            # make() gives make none of SETTINGS from the environment, so hand it these
            kept = [name for name in support.SETTINGS if name not in ('SEARCH', 'PACK')]
            with unittest.mock.patch.object(support, 'SETTINGS', kept), \
                    unittest.mock.patch.dict(os.environ, {'SEARCH': '1', 'PACK': '1'}):
                status, output = make(root, '-n', 'dc-order', 'SH_ELF=sh-elf')
            self.assertEqual(status, 0, output)
            self.assertEqual([line for line in output.splitlines() if 'tools/order.py' in line], [ordered.format('')])

    def test_links_again_once_the_heat_or_the_record_of_the_warm_rows_order_changes_joins_or_leaves(self):
        # both are tables of 1ST_READ.BIN's, so the link, which holds itself to them, runs again, and nothing compiles
        link = "python3 tools/ld.py --lto='' build/dc/link/spec.txt build/dc/link\n"
        tables = {'config/1st_read/heat.txt': 'frames  1\n', 'config/1st_read/order.txt': ''}
        for changed in tables:
            with self.subTest(changed), self.built(tables) as root:
                self.assertNotIn(link, make(root, '-n', 'dc', 'SH_ELF=sh-elf', 'CC=true')[1])
                with open(root / changed, 'a') as file:
                    file.write('\n# another\n')
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf', 'CC=true')
                self.assertEqual((status, link in output, compiles(output)), (0, True, {}), output)
                (root / changed).unlink()
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf', 'CC=true')
                self.assertEqual((status, link in output, compiles(output)), (0, True, {}), output)

    def test_links_again_once_a_kernel_row_an_import_s_provider_or_the_linker_changes(self):
        # the kernel table and the import tables are tables, and the linker's stamp holds its hash. An edit to a table
        # compiles nothing again, unless a source joins or leaves the kernel tables' rows, as LinkTime holds
        tables = {'config/1st_read/kernel.txt': '', 'config/1st_read/imports.txt': ''}
        link = "python3 tools/ld.py --lto='' build/dc/link/spec.txt build/dc/link\n"
        for changed in (*tables, 'config/imports.txt', 'sh-elf/bin/sh-elf-ld'):
            with self.subTest(changed), self.built(tables) as root:
                self.assertNotIn(link, make(root, '-n', 'dc', 'SH_ELF=sh-elf', 'CC=true')[1])
                with open(root / changed, 'a') as file:
                    file.write('\n# another\n')
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf', 'CC=true')
                self.assertEqual(status, 0, output)
                self.assertIn(link, output)
                if changed != 'sh-elf/bin/sh-elf-ld':
                    self.assertEqual(compiles(output), {})

    def test_links_again_and_compiles_no_c_source_once_a_program_of_the_link_time_compile_changes(self):
        # the link alone runs lto1, lto-wrapper, collect2 and the linker's plugin, so the linker's stamp holds their
        # hashes, and GCC's doesn't: a C object carries the code cc1 wrote, whichever lto1 compiles it later
        link = "python3 tools/ld.py --lto='' build/dc/link/spec.txt build/dc/link\n"
        for changed in LINK_TIME_TOOLS:
            with self.subTest(changed), self.built() as root:
                stamp = (root / 'build/tools/gcc.id').read_text()
                self.assertNotIn(link, make(root, '-n', 'dc', 'SH_ELF=sh-elf', 'CC=true')[1])
                with open(root / 'sh-elf' / changed, 'a') as file:
                    file.write('another\n')
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf', 'CC=true')
                self.assertEqual(status, 0, output)
                self.assertIn(link, output)
                self.assertEqual(compiles(output), {})
                self.assertEqual((root / 'build/tools/gcc.id').read_text(), stamp)
                self.assertIn(f'sh-elf/{changed}\n', (root / 'build/tools/ld.id').read_text())

    def test_a_link_that_fails_leaves_no_manifest(self):
        # a publisher that fails, after an earlier run wrote a manifest
        with self.built() as root:
            self.assertTrue((root / 'build/dc/manifest.txt').exists())
            (root / 'tools/publish.py').write_text('raise SystemExit(1)\n')
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertNotEqual(status, 0, output)
            self.assertFalse((root / 'build/dc/manifest.txt').exists())

    def test_builds_a_disc_from_the_manifest_of_the_link(self):
        # make dc-disc runs the census and the import check, links, and builds the disc from the manifest
        files = {'orig/disc.gdi': '', 'build/gdi': ''}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'dc-disc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        lines = output.splitlines()
        published = lines.index('SH_ELF=sh-elf python3 tools/publish.py build/dc/link/spec.txt build/dc/link '
                                'build/dc/manifest.txt')
        self.assertEqual(lines[-3:], ["printf '  %-6s %s\\n' 'disc' 'build/dc/disc'",
                                      'rm -rf build/dc/disc build/dc/disc.partial',
                                      "python3 tools/disc.py 'orig/disc.gdi' build/dc/manifest.txt build/dc/disc"])
        checks = [next(at for at, line in enumerate(lines) if check in line)
                  for check in ('tools/census.py --generated --disc', 'tools/census.py --dc', 'python3 tools/imports.py')]
        self.assertLess(max(checks), lines.index('rm -rf build/dc/manifest.txt build/dc/disc build/dc/disc.partial'))
        self.assertLess(published, len(lines) - 3)

    def test_fails_on_the_census_or_the_import_check_before_the_link_of_a_disc(self):
        # the census and the import check are prerequisites of the manifest, not goals beside it. One that fails stops
        # make dc-disc before the link, which leaves no manifest and so no disc
        disc = 'import pathlib, sys\npathlib.Path(sys.argv[-1]).mkdir()\n'
        for path, text in (('tools/census.py', "raise SystemExit('census: a number for RAM')\n"),
                           ('tools/imports.py', "raise SystemExit('imports: a use no stage holds')\n")):
            files = {'orig/disc.gdi': '', 'build/gdi': '', 'tools/disc.py': disc, path: text}
            with self.subTest(path), stood_in(files) as root:
                status, output = make(root, 'dc-disc', 'SH_ELF=sh-elf', 'CC=true')
                self.assertNotEqual(status, 0, output)
                self.assertNotIn('link   build/dc/link', output)
                self.assertFalse((root / 'build/dc/manifest.txt').exists())
                self.assertFalse((root / 'build/dc/disc').exists())

    def test_a_link_that_fails_after_a_good_one_leaves_no_disc(self):
        # a stand-in tools/disc.py writes a disc from a good link's manifest, and then tools/spec.py fails
        files = {'orig/disc.gdi': '', 'build/gdi': '',
                 'tools/disc.py': "import pathlib, sys\npathlib.Path(sys.argv[-1]).mkdir()\n"
                                  "pathlib.Path(sys.argv[-1], 'disc.txt').write_text('a disc\\n')\n"}
        with stood_in(files) as root:
            status, output = make(root, 'dc-disc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertTrue((root / 'build/dc/disc/disc.txt').exists())
            settle(root)
            (root / 'tools/spec.py').write_text('raise SystemExit(1)\n')
            status, output = make(root, 'dc-disc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertNotEqual(status, 0, output)
            self.assertFalse((root / 'build/dc/manifest.txt').exists())
            self.assertFalse((root / 'build/dc/disc').exists())

    def test_builds_a_disc_from_the_disc_s_gdi_alone(self):
        with stood_in() as root:
            status, output = make(root, '-n', 'dc-disc', 'SH_ELF=sh-elf')
        self.assertNotEqual(status, 0, output)
        self.assertRegex(output, r"dc\.mk:\d+: \*\*\* dc-disc needs the disc's \.gdi and its tracks in orig/, and "
                                 r'orig/ holds 0 \.gdi files\.  Stop\.\n\Z')

    def test_compiles_each_image_s_non_matching_build_before_the_census_reads_it(self):
        # census.py --dc reads every image's non-matching build, 1ST_READ.BIN's main.c and mod's a.c among them, so
        # made alone, the check compiles both objects first
        with stood_in() as root:
            status, output = make(root, '-n', 'SH_ELF=sh-elf', 'build/dc/census.ok')
        self.assertEqual(status, 0, output)
        lines = output.splitlines()
        for obj in ('build/dc/src/main.o', 'build/dc/modules/mod/a.o'):
            compile_ = next(at for at, line in enumerate(lines) if f' -o {obj} ' in line)
            self.assertLess(compile_, lines.index('python3 tools/census.py --dc'), output)

    def test_holds_1st_read_bin_s_reads_and_the_motion_files_to_the_census_before_its_non_matching_build(self):
        # the census check waits for the reads of 1ST_READ.BIN's main.c, which name what its compile reads, and for
        # each decoded motion file. It checks them, then the non-matching builds, and all before the link
        files = {**MOTION, 'orig/SONICADV/X_MOT.PRS': ''}
        with stood_in(files) as root:
            (root / 'build/reads/src/main.d').unlink()
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        lines = output.splitlines()
        steps = [next(at for at, line in enumerate(lines) if step in line)
                 for step in (' -MT build/reads/src/main.d ',
                              'prskit decompress orig/SONICADV/X_MOT.PRS', 'python3 tools/census.py --generated --disc',
                              'python3 tools/census.py --dc', 'rm -rf build/dc/manifest.txt ')]
        self.assertLess(max(steps[:2]), steps[2], output)
        self.assertEqual(sorted(steps[2:]), steps[2:], output)

    def test_holds_the_census_to_what_stands_for_each_file_1st_read_bin_s_matching_build_reads(self):
        # 1ST_READ.BIN places main.c, body.c, which takes a body from the disc, hero.src and a gen/ range. Beyond the
        # objects the link takes, the tables and the tools, the census check depends on hero.src itself, on each C
        # source's reads, on the range's C and on body.c's bodies.txt, which stands for its body. It depends on no
        # reads and no bodies.txt of mod's a.c, which takes a body too
        body = 'static int a[1] = {{\n#include "{}/a.inc"\n}};\n'.format
        files = {'config/1st_read/splits.txt': (MODULE['config/1st_read/splits.txt']
                                                + 'src/body.c  P  0x8c0101b0  0x8c0101c0\n'
                                                'src/hero.src  P  0x8c0101c0  0x8c0101d0\n'
                                                'gen/1st_read/d_8c1755e0.c  D  0x8c1755e0  0x8c1755f0\n'),
                 'config/1st_read/data.txt': '0x8c1755e0  words[4]\n', 'src/body.c': body('body'), 'src/hero.src': '',
                 'modules/mod/a.c': body('mod/a')}
        with stood_in(tabled(files)) as root:
            status, output = make(root, '-n', '-p', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output[-3000:])
        needs = re.search(r'(?m)^build/dc/census\.ok: (.*)$', output)[1].split()
        self.assertEqual(sorted(path for path in needs if path != 'build/tables.txt'
                                and not path.startswith(('build/dc/', 'config/', 'tools/'))),
                         ['build/gen/1st_read/d_8c1755e0.c', 'build/gen/src/body/bodies.txt', 'build/reads/src/body.d',
                          'build/reads/src/main.d', 'src/hero.src'])

    def test_holds_the_census_to_no_shc_object_and_checks_it_on_a_mac(self):
        # the census check waits for no object of SHC's. In a tree that holds none, with wibo stood in by RAN, the
        # check made alone compiles main.c's and a.c's GCC objects, runs both passes and runs no SHC, on Linux. On a
        # Mac, where every rule that runs SHC stops make, it passes too
        for uname in (LINUX, *MACS):
            with self.subTest(uname), stood_in({WIBO: RAN}) as root:
                status, output = make(root, '-n', 'SH_ELF=sh-elf', 'CC=true', 'build/dc/census.ok', uname=uname)
                self.assertEqual(status, 0, output)
                self.assertIn('\npython3 tools/census.py --generated --disc\npython3 tools/census.py --dc\n', output)
                self.assertNotIn('shc.exe', output)
                self.assertEqual(make(root, 'SH_ELF=sh-elf', 'CC=true', 'build/dc/census.ok', uname=uname),
                                 (0, '  gcc    modules/mod/a.c\n  gas    modules/mod/b.src\n  gcc    src/main.c\n'
                                     '  census config/1st_read/census.txt\n'))
                self.assertTrue((root / 'build/dc/census.ok').exists())
                self.assertFalse((root / 'ran').exists())

    def test_holds_1st_read_bin_s_imports_to_the_shc_objects_that_define_them_under_make_check_alone(self):
        # 1ST_READ.BIN takes a_init from mod, whose a.c defines it. make check holds the import to a.c's SHC object,
        # or its stub's where a.c has bodies, once it has built it. make dc's half of the import check reads the
        # import's uses and no object, so make dc compiles a.c with no SHC: neither its object nor its stub, whose
        # listing its bodies don't read. With bodies, a.c has its table of offsets, which make check holds too
        full = 'shc.exe modules/mod/a.c -objectfile=build/modules/mod/a.obj'
        for case, source, reads in (('object', '', full), ('stub', '#include "mod/a/a.inc"\n', STUB_SHC)):
            files = {'config/1st_read/imports.txt': 'a_init  mod\n', 'modules/mod/a.c': source}
            with self.subTest(case), stood_in(tabled(files)) as root:
                status, output = make(root, '-n', 'check')
                self.assertEqual(status, 0, output)
                lines = output.splitlines()
                built = next(at for at, line in enumerate(lines) if reads in line)
                self.assertLess(built, lines.index('python3 tools/imports.py --definitions'), output)
                self.assertNotIn('tools/imports.py --uses', output)
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                lines = output.splitlines()
                check = lines.index('python3 tools/imports.py --uses')
                link = lines.index('rm -rf build/dc/manifest.txt build/dc/disc build/dc/disc.partial')
                self.assertLess(check, link, output)
                self.assertNotIn('shc.exe', output)
                self.assertNotIn('tools/imports.py --definitions', output)

    def test_holds_1st_read_bin_s_uses_of_imports_with_no_object_built_on_any_host(self):
        # the import check reads the tables and the C, each source as SHC and as GCC compile it. Made alone, in a
        # tree with no wibo, no release of SHC and no stamp or object of either, it compiles nothing and passes, on
        # Linux as on a Mac. Its rule names what it reads, and a second make has nothing to check
        for uname in (LINUX, *MACS):
            with self.subTest(uname), checkout({**SEARCHED, **CHECK_NEEDS}) as root:
                settle(root)
                status, output = make(root, '-n', 'build/dc/imports.ok', 'SH_ELF=sh-elf', uname=uname)
                self.assertEqual(status, 0, output)
                self.assertEqual(output.splitlines()[1:], ['python3 tools/imports.py --uses',
                                                           'mkdir -p build/dc && touch build/dc/imports.ok'])
                self.assertEqual(make(root, 'build/dc/imports.ok', 'SH_ELF=sh-elf', uname=uname),
                                 (0, '  import config/1st_read/imports.txt\n'))
                self.assertTrue((root / 'build/dc/imports.ok').exists())
                self.assertEqual(make(root, 'build/dc/imports.ok', 'SH_ELF=sh-elf', uname=uname),
                                 (0, f"{Path(MAKE).name}: 'build/dc/imports.ok' is up to date.\n"))

    def test_writes_the_content_and_runs_each_check_and_the_link_again_once_a_module_a_tool_imports_changes(self):
        # each check depends on its tool and on each module of tools/ the tool imports, directly or through another.
        # The link depends on the tools of its stages and their modules alike, and the content of mod, whose gen/
        # range has it written, on tools/content.py and its modules
        files = {'config/mod/splits.txt': MODULE['config/mod/splits.txt'] + 'gen/mod/d_0c900180.c  D  0x0c900180  0x0c900200\n',
                 **MOD_DATA}
        with stood_in(files) as root:
            status, output = make(root, '-n', '-p', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output[-3000:])
        rules = dict(line.split(':', 1) for line in output.splitlines()
                     if line.startswith(('build/dc/census.ok:', 'build/dc/imports.ok:', 'build/dc/manifest.txt:',
                                         'build/gen/mod/content.c:')))
        for check, tools in (('build/dc/census.ok', ['census']), ('build/dc/imports.ok', ['imports']),
                             ('build/dc/manifest.txt', ['spec', 'ld', 'verify', 'publish']),
                             ('build/gen/mod/content.c', ['content'])):
            with self.subTest(check):
                self.assertEqual(sorted(imported(tools) - set(rules[check].split())), [])

    def test_checks_and_splits_a_source_again_once_a_tool_its_rule_runs_imports_changes(self):
        # b.src's object, and that of main.c, which tools/inline.py splits, depend on each tool their rules run and
        # each module of tools/ the tool imports, as the census does
        files = {'config/compilers.mk': FOUND, 'src/main.c': inline()}
        with stood_in(files) as root:
            status, output = make(root, '-n', '-p', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output[-3000:])
        rules = dict(line.split(':', 1) for line in output.splitlines()
                     if line.startswith(('build/dc/modules/mod/b.o:', 'build/dc/src/main.o:')))
        for obj, tools in (('build/dc/modules/mod/b.o', ['tags', 'floats']),
                           ('build/dc/src/main.o', ['inline', 'tags', 'floats'])):
            with self.subTest(obj):
                self.assertEqual(sorted(imported(tools) - set(rules[obj].split())), [])

    def test_runs_the_checks_and_the_link_again_once_a_table_goes_and_stops_once_census_txt_goes(self):
        # build/tables.txt lists the tables, so a deletion reruns what reads them, as an edit does; the census can't
        # run without census.txt, which it names
        with self.built({'config/1st_read/uses.txt': ''}) as root:
            (root / 'config/1st_read/uses.txt').unlink()
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
            self.assertEqual(status, 0, output)
            for check in ('python3 tools/census.py --dc', 'python3 tools/imports.py', 'python3 tools/publish.py '):
                self.assertIn(check, output)
            (root / 'config/1st_read/census.txt').unlink()
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertNotEqual(status, 0, output)
        self.assertIn("No rule to make target 'config/1st_read/census.txt', needed by 'build/dc/census.ok'", output)

    def test_compiles_each_c_object_again_once_a_header_joins_roots_or_a_directory_of_the_engine_s(self):
        # build/headers.txt holds the headers under ROOTS, and build/dc/headers.txt those of the engine's directories
        # a compile searches, any of which a search could find first
        for header in ('modules/mod/b.h', 'engine/dc/b.h', 'engine/dc/sa1/b.h', 'engine/include/b.h',
                       'engine/sa1/b.h'):
            with self.subTest(header), self.built() as root:
                (root / header).parent.mkdir(parents=True, exist_ok=True)
                (root / header).write_text('')
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                for obj in ('src/main.o', 'modules/mod/a.o'):
                    self.assertIn(f' -c -o build/dc/{obj} ', output)

    def test_reads_optimize_0_of_a_source_s_shc_flags_alone(self):
        files = {'config/compilers.mk': COMPILERS + 'build/dc/modules/mod/a.o: GAME += -optimize=0 -noinline\n'}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        compiled = compiles(output)
        self.assertIn('-O0', compiled['build/dc/modules/mod/a.o'])
        self.assertNotIn('-O0', compiled['build/dc/src/main.o'])
        self.assertNotIn('-noinline', compiled['build/dc/modules/mod/a.o'])

    def test_gives_a_gcc_object_the_flags_own_adds_to_its_source(self):
        # config/compilers.mk's own(), called as it is for src/system/binary.c's -optimize=0
        own = ''.join(line for line in (ROOT / 'config/compilers.mk').read_text().splitlines(True)
                      if line.startswith('own ='))
        self.assertTrue(own)
        with stood_in({'config/compilers.mk': COMPILERS + own + '$(call own,modules/mod/a,-optimize=0)\n'}) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        compiled = compiles(output)
        self.assertIn('-O0', compiled['build/dc/modules/mod/a.o'])
        self.assertNotIn('-O0', compiled['build/dc/src/main.o'])

    def test_names_the_gcc_object_beside_each_source_given_a_flag_make_dc_reads(self):
        # of SHC's flags, GCC reads -optimize=0 alone, as -O0, and the float pass reads -fpu=single, -double=float and
        # -aggressive=2. So each line of config/compilers.mk that gives a source's object one of them names its GCC
        # object too. So does each line that sets an object's RUNTIME, which can take -fpu=single away
        text = (ROOT / 'config/compilers.mk').read_text()
        lines = text.replace('\\\n', ' ').splitlines()
        read = ('-optimize=0', '-fpu=single', '-double=float', '-aggressive=2', 'RUNTIME :=')
        settings = [line.split(':')[0].split() for line in lines
                    if line.startswith('build/') and any(flag in line for flag in read)]
        self.assertTrue(settings)
        for targets in settings:
            for obj in targets:
                if obj.endswith('.obj') and not obj.startswith('build/stub/'):
                    self.assertIn(f'build/dc/{obj[len("build/"):-len(".obj")]}.o', targets, targets)

    def test_reads_each_source_s_floats_as_its_shc_release_does_under_its_flags(self):
        # each route's rule passes the float pass the release that builds the source, and the -fpu=single,
        # -double=float or -aggressive=2 its flags give. Each release builds a source here with a float flag and one
        # without, and a.c takes -aggressive=2 too. Each line that gives a source a flag names its GCC object too, as
        # config/compilers.mk's lines do. p.c and q.c are patched, and g.c is grafted with o.c as its donor, from which
        # GCC compiles no object. Of those, all but p.c hold inline assembly, as i.c does
        double = ' '.join(f'build/sdk/ninja/{name}.obj build/dc/sdk/ninja/{name}.o' for name in 'pqig')
        own = ('build/modules/mod/a.obj build/dc/modules/mod/a.o: GAME += -aggressive=2',
               'R26_C := sdk/shinobi/ov.c sdk/shinobi/ow.c',
               'build/sdk/ninja/f.obj build/dc/sdk/ninja/f.o: SDK += -double=float -size',
               'build/sdk/shinobi/ov.obj build/dc/sdk/shinobi/ov.o: SDK += -fpu=single',
               'RUNTIME_C := sdk/runtime/r.c sdk/runtime/w.c', 'RUNTIME := -cpu=sh4',
               'build/sdk/runtime/r.obj build/dc/sdk/runtime/r.o: RUNTIME += -fpu=single',
               'R32_C := sdk/shinobi/gd.c sdk/sofdec/adx.c',
               'build/sdk/shinobi/gd.obj build/dc/sdk/shinobi/gd.o: SDK += -fpu=single',
               'PATCHED_C := sdk/ninja/p.c sdk/ninja/q.c', 'PATCHES := align-pool',
               'GRAFTED_C := sdk/ninja/g.c', 'DONOR_C := sdk/ninja/o.c', f'{double}: SDK += -double=float')
        compilers = (COMPILERS.replace('GAME := -cpu=sh4', 'GAME := -cpu=sh4 -fpu=single')
                     + ''.join(f'{line}\n' for line in own) + FOUND[len(COMPILERS):])
        placed = ('ninja/f', 'ninja/d', 'shinobi/ov', 'shinobi/ow', 'runtime/r', 'runtime/w', 'shinobi/gd',
                  'sofdec/adx', 'ninja/p', 'ninja/i', 'ninja/q', 'ninja/g')
        rows = ''.join(f'sdk/{source}.c  P  0x0c9000{at:02x}  0x0c9000{at + 8:02x}\n'
                       for source, at in zip(placed, range(0x40, 0x100, 8)))
        files = {'config/compilers.mk': compilers, **{f'sdk/{source}.c': '' for source in placed},
                 **{f'sdk/ninja/{name}.c': inline(name) for name in 'iqgo'},
                 'config/mod/splits.txt': (MODULE['config/mod/splits.txt'] + rows
                                           + 'gen/mod/d_0c900180.c  D  0x0c900180  0x0c900200\n'), **MOD_DATA}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        routes = re.findall(r'python3 tools/floats\.py (.*?) --system tools/sh4zam/include build/dc/(\S+)\.i ', output)
        self.assertEqual({c: route for route, c in routes},
                         {'src/main': '--shc r32 -fpu=single', 'modules/mod/a': '--shc r32 -fpu=single -aggressive=2',
                          'sdk/ninja/f': '--shc r10 -double=float', 'sdk/ninja/d': '--shc r10',
                          'sdk/shinobi/ov': '--shc r26 -fpu=single', 'sdk/shinobi/ow': '--shc r26',
                          'sdk/runtime/r': '--shc r32 -fpu=single',
                          'sdk/runtime/w': '--shc r32', 'sdk/shinobi/gd': '--shc r32 -fpu=single',
                          'sdk/sofdec/adx': '--shc r32', 'gen/mod/content': '--shc r32 -fpu=single',
                          **{f'sdk/ninja/{name}': '--shc r10 -double=float' for name in 'pqig'}})

    def test_puts_each_datum_in_a_section_of_its_own_where_the_memory_contract_places_its_record(self):
        # the memory contract places a datum by the section its record names. It places the GD file system's library
        # handle so, where the startup reads handles. A datum's section is its own only under -fdata-sections, which
        # config/gcc.mk's line for its source's object gives it beside GCCFLAGS
        recorded = sorted({source for records in memory.RECORDS.values() for _, source, section, *_ in records
                           if section.startswith('.data.')})
        self.assertTrue(recorded)
        for source in recorded:
            obj = f'build/dc/{source[:-2]}.o'
            own = ''.join(line for line in (ROOT / 'config/gcc.mk').read_text().splitlines(True)
                          if line.startswith(f'{obj}: private GCCFLAGS += '))
            self.assertTrue(own, obj)
            # no route compiles a source of LOADER.BIN's SDK unless LOADER_SDK names it
            loader = f'LOADER_SDK := {source}\n' if source.startswith('loader/sdk/') else ''
            rows = MODULE['config/mod/splits.txt'] + f'{source}  P  0x0c900040  0x0c900050\n'
            files = {'config/compilers.mk': COMPILERS + loader, 'config/gcc.mk': GCC_MK + own, source: '',
                     'config/mod/splits.txt': rows}
            with self.subTest(source), stood_in(files) as root:
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                compiled = compiles(output)
                self.assertIn('-fdata-sections', compiled[obj])
                self.assertNotIn('-fdata-sections', compiled['build/dc/modules/mod/a.o'])

    def test_splits_a_source_with_inline_assembly_from_its_c_and_compiles_the_c(self):
        files = {'config/compilers.mk': FOUND, 'src/main.c': inline()}
        with stood_in(files) as root:
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertEqual(output, '  gcc    modules/mod/a.c\n  gcc    src/main.c (split)\n  gas    modules/mod/b.src\n'
                                     '  ar     build/dc/sh4zam/libsh4zam.a\n'
                                     '  census config/1st_read/census.txt\n  import config/1st_read/imports.txt\n'
                                     '  link   build/dc/link\n')
            # the stand-in GCC copies the C to the object, and the translator the text
            self.assertIn('__asm__(".include \\"build/dc/src/main_inline.s\\""', (root / 'build/dc/src/main.o').read_text())
            self.assertEqual((root / 'build/dc/src/main_inline.s').read_text(),
                             '; src/main.c:2: main\n        .EXPORT     _main\n_main:\n        RTS\n        NOP\n'
                             '        .ALIGN      4\n        RTS\n        NOP\n')
        with stood_in(files) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn('python3 tools/inline.py src/main.c build/dc/src/main.c build/dc/src/main_inline.src && '
                      'python3 tools/floats.py --assembly build/dc/src/main_inline.src && '
                      'if [ -s build/dc/src/main_inline.src ]; then build/shckit translate -o build/dc/src/main_inline.s '
                      'build/dc/src/main_inline.src; else rm -f build/dc/src/main_inline.s; fi\n', output)
        self.assertIn(' -iquote src -Isdk/include -Isrc -fdiagnostics-plain-output -MD -MP -MT build/dc/src/main.o '
                      '-MF build/dc/src/main.d.tmp -E build/dc/src/main.c > build/dc/src/main.i && ', output)

    def test_compiles_the_c_dc_replaced_gives_a_source_of_inline_assembly_in_its_place(self):
        # engine/dc/main.c replaces src/main.c, whose functions are inline bodies, so nothing splits it
        files = {'config/compilers.mk': FOUND, 'src/main.c': inline(), 'engine/dc/main.c': 'own main\n',
                 'config/dreamcast.mk': 'DC_REPLACED := src/main.c:engine/dc/main.c\n'}
        with stood_in(files) as root:
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertIn('  gcc    engine/dc/main.c\n', output)
            self.assertNotIn('(split)', output)
            self.assertEqual((root / 'build/dc/src/main.o').read_text(), 'own main\n')
            self.assertFalse((root / 'build/dc/src/main_inline.s').exists())

    def test_splits_a_source_whose_assembly_a_guard_leaves_to_shc_and_translates_no_text(self):
        # the non-matching side defines main in C. The split leaves an empty text, which shckit refuses, so it
        # translates none
        guarded = '#ifdef NONMATCHING\nvoid main(void)\n{\n}\n#else\n' + inline() + '#endif\n'
        with stood_in({'config/compilers.mk': FOUND, 'src/main.c': guarded}) as root:
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertIn('  gcc    src/main.c (split)\n', output)

    def test_translates_no_text_where_a_guard_keeps_each_block_of_a_source_from_gcc(self):
        # the source's one block lies in an #ifdef NONMATCHING's #else, so its text is empty. shckit refuses an empty
        # text, as its stand-in here does a split's. The split translates no text, and the C includes none
        block = ('#ifdef NONMATCHING\n#else\n/* @asm open: 8 bytes reserved */\n#pragma asm\n        .RES.B      8\n'
                 '        ;\n#pragma endasm\n#endif\n')
        files = {'config/compilers.mk': FOUND, 'src/main.c': 'void main(void)\n{\n}\n\n' + block,
                 'build/shckit': ('#!/bin/sh\n[ "$1" = translate ] && [ "$2" = -o ] || exit 1\n'
                                  'case $4 in *_inline.src) [ -s "$4" ] || exit 1 ;; esac\ncp "$4" "$3"\n')}
        with stood_in(files) as root:
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertEqual((root / 'build/dc/src/main_inline.src').read_text(), '')
            self.assertFalse((root / 'build/dc/src/main_inline.s').exists())
            self.assertNotIn('__asm__', (root / 'build/dc/src/main.o').read_text())

    def test_compiles_a_split_source_with_each_function_in_a_section_of_its_own(self):
        # tools/inline.py lets a body branch to one of its file's C functions. The branch lands right only where the
        # function starts a section of its own
        files = {'config/compilers.mk': FOUND, 'config/gcc.mk': GCC_MK + gccflags(), 'src/main.c': inline()}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        compiled = compiles(output)['build/dc/src/main.o']
        self.assertEqual([flag for flag in test_ld.flags() if flag not in compiled], [])
        self.assertIn('-ffunction-sections', compiled)

    def test_orders_each_object_as_gcc_will_but_a_content(self):
        # of make dc's compiles, only a content's keeps its source's order: GCC orders any other's definitions as
        # it will
        with stood_in({'config/gcc.mk': GCC_MK + gccflags()}) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertNotIn('-fno-toplevel-reorder', compiles(output)['build/dc/src/main.o'])

    def test_fuses_a_product_into_a_sum_where_the_source_s_release_does(self):
        # R32 and R26, the releases of main.c and ov.c, fuse a product into the sum that takes it, as -ffp-contract=on
        # does. R10, d.c's release, never fuses: njDistancePL2PL's @bug arm gives the image's infinity or NaN only
        # while each product rounds
        rows = ''.join(f'{source}  P  0x0c9000{at:02x}  0x0c9000{at + 8:02x}\n'
                       for source, at in (('sdk/ninja/d.c', 0x40), ('sdk/shinobi/ov.c', 0x50)))
        files = {'config/compilers.mk': FOUND + 'R26_C := sdk/shinobi/ov.c\n', 'config/gcc.mk': GCC_MK + gccflags(),
                 'sdk/ninja/d.c': '',
                 'sdk/shinobi/ov.c': '', 'config/mod/splits.txt': MODULE['config/mod/splits.txt'] + rows}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        compiled = compiles(output)
        contracts = {obj: [word for word in compiled[obj] if word.startswith('-ffp-contract=')][-1]
                     for obj in ('build/dc/src/main.o', 'build/dc/sdk/shinobi/ov.o', 'build/dc/sdk/ninja/d.o')}
        self.assertEqual(contracts, {'build/dc/src/main.o': '-ffp-contract=on',
                                     'build/dc/sdk/shinobi/ov.o': '-ffp-contract=on',
                                     'build/dc/sdk/ninja/d.o': '-ffp-contract=off'})

    def test_splits_the_grafted_source_and_builds_no_object_of_its_donor(self):
        # each holds an inline body. Only SHC's object of the grafted source holds the donor's functions
        files = {'config/compilers.mk': (COMPILERS + 'GRAFTED_C := sdk/ninja/g.c\nDONOR_C := sdk/ninja/d.c\n' +
                                         FOUND[len(COMPILERS):]),
                 'config/mod/splits.txt': MODULE['config/mod/splits.txt'] + 'sdk/ninja/g.c  P  0x0c900040  0x0c900050\n',
                 'sdk/ninja/g.c': inline('g'), 'sdk/ninja/d.c': inline('d')}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn('python3 tools/inline.py sdk/ninja/g.c build/dc/sdk/ninja/g.c ', output)
        self.assertIn(' -iquote sdk/ninja -Isdk/include -Isdk -Imodules -I. -Ibuild/gen/sdk -Ibuild/gen/modules '
                      '-Ibuild/gen/loader -fdiagnostics-plain-output -MD -MP -MT build/dc/sdk/ninja/g.o '
                      '-MF build/dc/sdk/ninja/g.d.tmp -E build/dc/sdk/ninja/g.c > ', output)
        self.assertNotIn('sdk/ninja/d.c', output)

    def test_takes_an_abi_function_only_where_its_object_names_its_contract(self):
        # b.src's function and main.c's body each fail without a contract, as does a contract that names neither
        cases = {
            'b': ({'modules/mod/b.src': ABI, 'config/mod/symbols.txt': MODULE['config/mod/symbols.txt'] + '0x0c900020  b\n'},
                  COMPILERS, 'modules/mod/b.src:6', 'modules/mod/b.o'),
            'main': ({'src/main.c': inline(tag='abi: stack-written, MOV R4,R15 at 0x8c0101a0 sets the stack pointer')},
                     FOUND, 'src/main.c:2', 'src/main.o'),
        }
        for name, (files, compilers, where, obj) in cases.items():
            with self.subTest(name), stood_in({**files, 'config/compilers.mk': compilers}) as root:
                status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
                self.assertNotEqual(status, 0, output)
                self.assertIn(f'tags: {where}: {name}: its @asm tag is abi, and no --contract names it\n', output)
                self.assertFalse((root / 'build/dc' / obj).exists())
                (root / 'config/gcc.mk').write_text(f'{GCC_MK}build/dc/{obj}: CONTRACTS := {name} gone\n')
                status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
                self.assertNotEqual(status, 0, output)
                self.assertIn('tags: gone: --contract names no function whose @asm tag is abi or that may not be '
                              'call-safe\n', output)
                (root / 'config/gcc.mk').write_text(f'{GCC_MK}build/dc/{obj}: CONTRACTS := {name}\n')
                status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
                self.assertEqual(status, 0, output)

    def test_makes_a_body_source_s_bodies_before_its_gcc_object(self):
        files = {'modules/mod/a.c': A_BODY, A_TABLE: NO_ROWS}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'build/dc/modules/mod/a.o', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertLess(output.index(BODIES), output.index('modules/mod/a.c > build/dc/modules/mod/a.i'), output)

    def test_holds_no_table_of_offsets_to_a_stub_build_s(self):
        # the bodies wait for the check of their source's table for a goal of the matching build alone. make dc
        # reads a.c's bodies by its table, and measures no table and holds none. Typed beside make check, it does
        # both before it reads a.c's bodies
        files = {'modules/mod/a.c': A_BODY, A_TABLE: NO_ROWS}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
            self.assertEqual(status, 0, output)
            self.assertIn(BODIES, output)
            for step in (MEASURE, HOLD):
                self.assertNotIn(step, output)
            status, output = make(root, '-n', 'dc', 'check', 'SH_ELF=sh-elf')
            self.assertEqual(status, 0, output)
            self.assertLess(output.index(MEASURE), output.index(HOLD), output)
            self.assertLess(output.index(HOLD), output.index(BODIES), output)

    def test_stops_on_a_body_source_with_no_table_of_offsets_and_says_what_writes_one_and_where(self):
        # make dc reads a.c's bodies by its table alone, so with none it stops by the Makefile's rule for a table
        # that isn't there, under make -n too, on Linux as on a Mac. The line names the table, and says that make
        # offsets writes it on x86 Linux. It reads no body and links nothing. make dc-disc, with the disc's .gdi in
        # place, stops alike
        files = {'modules/mod/a.c': A_BODY}
        for uname in (LINUX, *MACS):
            for goal, disc in (('dc', {}), ('dc-disc', {'orig/disc.gdi': '', 'build/gdi': ''})):
                for flags in ((), ('-n',)):
                    with self.subTest(uname, goal=goal, flags=flags), stood_in({**files, **disc}) as root:
                        status, output = make(root, *flags, goal, 'SH_ELF=sh-elf', 'CC=true', uname=uname)
                        self.assertEqual(status, 2, output)
                        self.assertRegex(output, r"Makefile:\d+: \*\*\* offsets/modules/mod/a\.txt: missing; it "
                                                 r"records where modules/mod/a\.c's data sits in the original\. make "
                                                 r'offsets writes it on x86 Linux; a renamed source takes a git mv; '
                                                 r'see TOOLCHAIN\.md#stub-and-full-builds\.  Stop\.\n\Z')
                        for step in ('tools/data.py --bodies', 'tools/spec.py'):
                            self.assertNotIn(step, output)
                        self.assertFalse((root / 'build/dc/manifest.txt').exists())

    def test_searches_the_include_directories_shc_searches(self):
        # a movie SDK member with inline assembly, the loader's own code, and patched sources with inline assembly
        # under sdk/ and src/. Each searches include directories other than its route's, which config/compilers.mk,
        # the Makefile or dc.mk gives it. 1ST_READ.BIN places the four, and n.c, which searches the game's, so each
        # has reads, which search what SHC and GCC do. A body source's reads search what its stub does: those of
        # a.c, of s.c, an SDK member, and of t.c, a patched source
        loader = ''.join(line for line in (ROOT / 'config/compilers.mk').read_text().splitlines(True)
                         if line.startswith('$(LOADER_C:%.c=build/%.obj) $(LOADER_C:%.c=build/dc/%.o) '
                                            '$(LOADER_C:%.c=build/reads/%.d): GAME_INC'))
        self.assertTrue(loader)
        bodies = {f'{stub}.c': f'static int a[1] = {{\n#include "{stub.split("/", 1)[1]}/a.inc"\n}};\n'
                  for stub in ('modules/mod/a', 'sdk/ninja/s', 'sdk/ninja/t')}
        files = {'config/compilers.mk': (COMPILERS + 'MOVIE_SDK := modules/movie/sdk/m.c\nLOADER_C := loader/l.c\n'
                                         'PATCHED_C := sdk/ninja/p.c src/q.c sdk/ninja/t.c\nPATCHES := align-pool\n' +
                                         FOUND[len(COMPILERS):] + loader),
                 'config/1st_read/splits.txt': (MODULE['config/1st_read/splits.txt'] +
                                                'modules/movie/sdk/m.c  P  0x8c0101b0  0x8c0101c0\n'
                                                'loader/l.c  P  0x8c0101c0  0x8c0101d0\n'
                                                'sdk/ninja/p.c  P  0x8c0101d0  0x8c0101e0\n'
                                                'src/q.c  P  0x8c0101e0  0x8c0101f0\n'
                                                'src/n.c  P  0x8c0101f0  0x8c010200\n'),
                 'config/mod/splits.txt': (MODULE['config/mod/splits.txt'] +
                                           'sdk/ninja/s.c  P  0x0c900040  0x0c900050\n'
                                           'sdk/ninja/t.c  P  0x0c900060  0x0c900070\n'),
                 'modules/movie/sdk/m.c': inline('m'), 'loader/l.c': '', 'sdk/ninja/p.c': inline('p'),
                 'src/q.c': inline('q'), 'src/n.c': '', **bodies}
        objects = ('modules/movie/sdk/m', 'loader/l', 'sdk/ninja/p', 'src/q', 'src/n')
        stubs = [source[:-2] for source in bodies]
        with stood_in(files) as root:
            status, output = make(root, '-n', *(f'build/{obj}.obj' for obj in objects),
                                  *(f'build/dc/{obj}.o' for obj in objects),
                                  *(f'build/stub/{stub}.obj' for stub in stubs),
                                  *(f'build/reads/{source}.d' for source in (*objects, *stubs)), 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        lines = output.splitlines()

        def searched(line):
            # the directories a compiler's command searches, as its -I words
            return [word for word in line.split() if word.startswith('-I')]

        def shc(source, wrote):
            # the directories SHC searches as it compiles source to the file wrote names, as GCC's -I words
            line = next(line for line in lines if f'shc.exe {source} ' in line and wrote in line)
            return ['-I' + directory for directory in re.search(r'SHC_INC=(\S+)', line)[1].split(',')]

        def reads(stem):
            # the directories the host's compiler searches as it writes the reads of the source at stem
            return searched(next(line for line in lines if f' -MT build/reads/{stem}.d ' in line))

        for obj in objects:
            with self.subTest(obj):
                gcc = next(line for line in lines if 'sh-elf-gcc' in line and f' -o build/dc/{obj}.o ' in line)
                self.assertEqual(shc(f'{obj}.c', f'build/{obj}.'), searched(gcc))
                self.assertEqual(shc(f'{obj}.c', f'build/{obj}.'), reads(obj))
        for stub in stubs:
            with self.subTest(stub):
                self.assertEqual(shc(f'{stub}.c', f'build/stub/{stub}.'), reads(stub))

    def test_writes_compiles_and_checks_the_content_of_an_image_with_a_gen_range(self):
        # tools/content.py writes its C from the disc. GCC's preprocessor reads it with the game's headers, and GCC
        # compiles what the float pass respells, each under the content's own flags. GNU ld links the object at the
        # image's addresses into a binary, which cmp holds to the disc's bytes. GCC writes the content's definitions
        # in their source's order, the image's, which the final link keeps. The gen/ range has no GCC object
        own = ''.join(line for line in (ROOT / 'config/gcc.mk').read_text().splitlines(True)
                      if line.startswith('build/dc/gen/%/content.o: private GCCFLAGS += '))
        self.assertTrue(own)
        files = {'config/gcc.mk': GCC_MK + own, **MOD_DATA,
                 'config/mod/splits.txt': MODULE['config/mod/splits.txt'] + 'gen/mod/d_0c900180.c  D  0x0c900180  0x0c900200\n'}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        output = output.replace('\\\n\t', '')
        line = next(line for line in output.splitlines() if 'sh-elf-gcc' in line and 'content.c' in line)
        preprocess, _, compile_ = line.partition(' build/gen/mod/content.c > build/dc/gen/mod/content.i && ')
        compile_ = compile_.partition(') > ')[0]
        self.assertIn('python3 tools/content.py build/gen/img/MOD.BIN build/gen/mod/content.c\n', output)
        self.assertEqual([flag for flag in preprocess.split() if flag.startswith(('-f', '-I'))],
                         ['-fdata-sections', '-fno-toplevel-reorder', '-ffp-contract=on', '-Isdk/include', '-Isrc',
                          '-Imodules', '-fdiagnostics-plain-output'])
        # the content's floats read as the game's sources' do, under R32 and GAME's flags, here without a float flag
        self.assertTrue(compile_.startswith('python3 tools/floats.py --shc r32 --system tools/sh4zam/include '
                                            'build/dc/gen/mod/content.i && '), line)
        self.assertEqual([flag for flag in compile_.split() if flag.startswith(('-f', '-I'))],
                         ['-fdata-sections', '-fno-toplevel-reorder', '-ffp-contract=on', '-fdiagnostics-plain-output'])
        self.assertRegex(output, r'-c -o build/dc/gen/mod/content\.o build/dc/gen/mod/content\.i\) > '
                                 r'build/dc/gen/mod/content\.o\.log 2>&1 \|\| ')
        self.assertIn('sh-elf/bin/sh-elf-ld -m shlelf --orphan-handling=error --fatal-warnings --oformat=binary '
                      '-T build/gen/mod/content.ld -o build/dc/gen/mod/content.bin build/dc/gen/mod/content.o '
                      '> build/dc/gen/mod/content.bin.log 2>&1 || (cat build/dc/gen/mod/content.bin.log; false)\n',
                      output)
        # the link writes a log of its own, and leaves the compile's
        self.assertEqual(output.count('> build/dc/gen/mod/content.o.log'), 1, output)
        self.assertIn('cmp -s build/dc/gen/mod/content.bin build/gen/mod/content.bin || ', output)
        self.assertNotIn('d_0c900180.o', output)

    def test_fetches_sh4zam_s_files_before_an_r26_content_replacement_or_sh4zam_compile(self):
        # an R26 member, a gen/ range, whose image then has a content, a replacement and one of SH4ZAM's are in
        # place. tools/SHA256SUMS lists SH4ZAM's LICENSE, which is missing, beside that source
        files = {'config/compilers.mk': COMPILERS + 'R26_C := sdk/shinobi/ov.c\n', 'sdk/shinobi/ov.c': '',
                 'config/mod/splits.txt': (MODULE['config/mod/splits.txt'] + 'sdk/shinobi/ov.c  P  0x0c900040  0x0c900050\n'
                                           'gen/mod/d_0c900180.c  D  0x0c900180  0x0c900200\n'), **MOD_DATA,
                 'engine/dc/b.c': '', **B_REPLACED, 'tools/sh4zam/source/sh4/a.c': '',
                 'tools/SHA256SUMS': f'{HASH}  tools/sh4zam/LICENSE\n{HASH}  tools/sh4zam/source/sh4/a.c\n'}
        for obj in ('build/dc/sdk/shinobi/ov.o', 'build/dc/gen/mod/content.o', 'build/dc/modules/mod/b.o',
                    'build/dc/sh4zam/sh4/a.o'):
            with self.subTest(obj), stood_in(files) as root:
                status, output = make(root, '-n', obj, 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                fetch = output.find('curl -fsSL -o tools/sh4zam/LICENSE.part ')
                self.assertNotEqual(fetch, -1, output)
                self.assertLess(fetch, output.index(f'-c -o {obj} '), output)

    def test_compiles_an_object_again_whose_header_changes_or_whose_d_file_is_gone(self):
        # a C source's object, and that of a replacement
        for obj, source in (('modules/mod/a', 'modules/mod/a.c'), ('modules/mod/b', 'engine/dc/b.c')):
            compile_ = f'-c -o build/dc/{obj}.o '
            replaced = B_REPLACED if source.startswith('engine/') else {}
            with self.subTest(source), self.built({source: '', f'{obj}.h': '', **replaced}) as root:
                (root / f'build/dc/{obj}.d').write_text(depends(f'build/dc/{obj}.o', source, f'{obj}.h'))
                settle(root)
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
                self.assertNotIn(compile_, output)
                os.utime(root / f'{obj}.h')
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                self.assertIn(compile_, output)
                settle(root)
                (root / f'build/dc/{obj}.d').unlink()
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                self.assertIn(compile_, output)

    def test_splits_a_source_again_whose_split_c_is_gone(self):
        files = {'config/compilers.mk': FOUND, 'src/main.c': inline()}
        with self.built(files) as root:
            (root / 'build/dc/src/main.d').write_text(depends('build/dc/src/main.o', 'build/dc/src/main.c'))
            settle(root)
            (root / 'build/dc/src/main.c').unlink()
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn('python3 tools/inline.py src/main.c build/dc/src/main.c', output)

    def test_keeps_the_last_whole_dependency_file_of_a_compile_that_fails(self):
        # a compile that writes a .d file make can't read, and fails, leaves the last whole one, so make compiles again
        with self.built() as root:
            gcc = root / 'sh-elf/bin/sh-elf-gcc'
            working = gcc.read_text()
            gcc.write_text('#!/bin/sh\nwhile [ $# -gt 1 ]; do [ "$1" = -MF ] && d=$2; shift; done\n'
                           '[ -z "$d" ] || echo "not make" > "$d"\nexit 1\n')
            (root / 'modules/mod/a.c').write_text((root / 'modules/mod/a.c').read_text() + '\n')
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertNotEqual(status, 0, output)
            gcc.write_text(working)
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
        self.assertEqual(status, 0, output)
        self.assertIn('gcc    modules/mod/a.c', output)

    def test_compiles_again_with_another_compiler_assembler_or_flags(self):
        # C sources, a .src file, a replacement in c.src's place, and SH4ZAM's sources
        files = {**LIBRARY, **placing('modules/mod/c.src'), 'modules/mod/c.src': '', 'engine/dc/c.c': '', **C_REPLACED}
        for changed in (f'sh-elf/{CC1}', 'sh-elf/bin/sh-elf-as', 'config/compilers.mk', 'config/gcc.mk',
                        'config/dreamcast.mk', 'Makefile', 'dc.mk'):
            with self.subTest(changed), self.built(files) as root:
                with open(root / changed, 'a') as file:
                    file.write('\n# another\n')
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                for obj in ('src/main.o', 'modules/mod/a.o', 'modules/mod/b.o', 'modules/mod/c.o', 'sh4zam/sh4/a.o',
                            'sh4zam/sh4/b.o'):
                    self.assertIn(f'-o build/dc/{obj} ', output)

    def test_a_change_to_dc_mk_or_config_gcc_mk_runs_no_shc_or_asmsh_again_for_make_check_beside_make_dc(self):
        # no SHC object depends on dc.mk or on config/gcc.mk, though make reads both for make dc. make check, typed
        # beside make dc, puts the SHC objects and a.c's stub among what make brings up to date. The first make
        # refreshes the stamps of wibo and SHC's releases, which the tree holds empty, so it plans SHC and asmsh
        # again. settle() then dates the stamps with the rest, so the tree counts as built: the second make plans
        # neither. Nor does the third, once either file changes
        def plans(root):
            status, output = make(root, '-n', 'dc', 'check', 'SH_ELF=sh-elf')
            self.assertEqual(status, 0, output)
            return [step for step in ('shc.exe', 'asmsh.exe') if step in output]

        for changed in ('dc.mk', 'config/gcc.mk'):
            with self.subTest(changed), stood_in({**BUILT, **PINNED}) as root:
                self.assertEqual(plans(root), ['shc.exe', 'asmsh.exe'])
                settle(root)
                self.assertEqual(plans(root), [])
                with open(root / changed, 'a') as file:
                    file.write('\n# another\n')
                self.assertEqual(plans(root), [])

    def test_compiles_each_c_source_again_once_floats_py_or_a_module_it_imports_changes(self):
        # and assembles each .src file again, whose constants the float pass checks. The float pass imports its
        # modules, tools/float_*.py, tools/tool.py and tools/layout.py
        for changed in ('tools/tool.py', 'tools/floats.py', 'tools/float_value.py', 'tools/layout.py'):
            with self.subTest(changed), self.built() as root:
                with open(root / changed, 'a') as file:
                    file.write('\n# another\n')
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                for obj in ('src/main.o', 'modules/mod/a.o', 'modules/mod/b.o'):
                    self.assertIn(f'-o build/dc/{obj} ', output)

    def test_compiles_no_c_source_again_once_tools_data_py_changes(self):
        # the float pass imports tools/tool.py and tools/layout.py, and not tools/data.py, which imports both
        with self.built() as root:
            with open(root / 'tools/data.py', 'a') as file:
                file.write('\n# another\n')
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertNotIn('-o build/dc/src/main.o ', output)

    def test_compiles_no_gcc_or_sh4zam_source_again_once_the_float_pass_or_a_module_it_imports_changes(self):
        # no SHC builds either, so the float pass reads neither's C. GNU as alone assembles SH4ZAM's assembly
        for changed in ('tools/tool.py', 'tools/floats.py', 'tools/layout.py'):
            with self.subTest(changed), self.built({**LIBRARY, 'engine/dc/b.c': '', **B_REPLACED}) as root:
                with open(root / changed, 'a') as file:
                    file.write('\n# another\n')
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                for obj in ('modules/mod/b.o', 'sh4zam/sh4/a.o', 'sh4zam/sh4/b.o'):
                    self.assertNotIn(f'-o build/dc/{obj} ', output)

    def test_stops_on_a_toolchain_without_libgcc_a_as_it_reads_its_stamp(self):
        # the linker's stamp names libgcc.a by a glob, which here matches no file, and stops make before any compile,
        # with the file sha256sum couldn't read named above make's line
        with stood_in() as root:
            (root / f'sh-elf/{LIBGCC}').unlink()
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
            self.assertNotEqual(status, 0, output)
            self.assertRegex(output, re.escape(f'sh-elf/{LIBGCC.replace("16.2.0", "*")}') + "'?: No such file")
            self.assertIn("build/tools/ld.id: can't refresh it from sh-elf/bin/sh-elf-ld sh-elf/bin/sh-elf-ar "
                          'sh-elf/lib/gcc/sh-elf/*/libgcc.a sh-elf/libexec/gcc/sh-elf/*/lto1 '
                          'sh-elf/libexec/gcc/sh-elf/*/lto-wrapper sh-elf/libexec/gcc/sh-elf/*/collect2 '
                          'sh-elf/libexec/gcc/sh-elf/*/liblto_plugin.so.  Stop.', output)
            self.assertNotIn('sh-elf/bin/sh-elf-gcc ', output)

    def test_stops_on_a_toolchain_that_holds_two_versions_of_gcc_and_names_them(self):
        # GCC's version is part of the paths of its headers, of cc1 and of libgcc.a, and the rules name each by a
        # glob that takes one version. with two, the second's headers would reach GCC as a source to compile
        with stood_in() as root:
            for version in ('15.2.0', '16.2.0'):
                (root / f'sh-elf/lib/gcc/sh-elf/{version}/include').mkdir(parents=True, exist_ok=True)
            for goal in ('dc', 'dc-check', 'dc-order'):
                status, output = make(root, '-n', goal, 'SH_ELF=sh-elf')
                self.assertNotEqual(status, 0, output)
                self.assertTrue(output.endswith(
                    'sh-elf: its lib/gcc/sh-elf/ holds more than one version of GCC, 15.2.0 16.2.0, and make dc '
                    'takes a toolchain of one.  Stop.\n'), output)
            shutil.rmtree(root / 'sh-elf/lib/gcc/sh-elf/15.2.0')
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
            self.assertEqual(status, 0, output)

    def test_stops_on_a_toolchain_without_a_program_of_the_link_time_compile_and_names_what_settles_it(self):
        # a toolchain built without link-time optimization holds no lto1 and no plugin, and one SH_ELF names has no
        # receipt to refuse it by. make stops before it refreshes the linker's stamp, which hashes each program, and
        # before any compile: it names each program the toolchain lacks, and the GCC that SH_ELF must name
        lacks = "sh-elf: its libexec/gcc/sh-elf/*/ holds no {}, which make dc's link-time compile runs; {}.  Stop.\n"
        named = 'SH_ELF must name a toolchain whose GCC was configured with --enable-lto'
        for gone in (LINK_TIME_TOOLS[:1], LINK_TIME_TOOLS[1:2], LINK_TIME_TOOLS[2:3], LINK_TIME_TOOLS[3:],
                     (LINK_TIME_TOOLS[0], LINK_TIME_TOOLS[3])):
            with self.subTest(gone), stood_in() as root:
                for path in gone:
                    (root / f'sh-elf/{path}').unlink()
                for goal in ('dc', 'dc-check', 'dc-order'):
                    status, output = make(root, '-n', goal, 'SH_ELF=sh-elf')
                    self.assertNotEqual(status, 0, output)
                    self.assertTrue(output.endswith(lacks.format(' '.join(Path(path).name for path in gone), named)),
                                    output)
                    self.assertNotIn('No such file', output)
                    self.assertNotIn('sh-elf/bin/sh-elf-gcc ', output)
        # make's own toolchain, built by this recipe, that has lost a program since: its receipt is dc.mk's, so the
        # receipt refuses nothing, and the remedy is to have make build it again
        with checkout({**SEARCHED, **PINNED, **ORIGINALS}) as root:
            for path, text in {**PINNED_TOOLCHAIN, 'tools/sh-elf/request.txt': receipt(root)}.items():
                (root / path).parent.mkdir(parents=True, exist_ok=True)
                (root / path).write_text(text)
            (root / f'tools/sh-elf/{LINK_TIME_TOOLS[0]}').unlink()
            status, output = make(root, '-n', 'dc')
            self.assertNotEqual(status, 0, output)
            self.assertTrue(output.endswith('tools/' + lacks.format(
                'lto1', 'remove tools/sh-elf, and make builds it again')), output)
            # the goal that builds the toolchain reads no stamp, and finds the driver in place
            self.assertEqual(make(root, 'tools/sh-elf/bin/sh-elf-gcc'),
                             (0, f"{Path(MAKE).name}: 'tools/sh-elf/bin/sh-elf-gcc' is up to date.\n"))

    def test_compiles_an_object_named_alone_again_with_another_compiler(self):
        # make build/dc/modules/mod/a.o, without make dc, refreshes GCC's stamp as make dc does
        with self.built() as root:
            with open(root / f'sh-elf/{CC1}', 'a') as file:
                file.write('\n# another\n')
            status, output = make(root, '-n', 'build/dc/modules/mod/a.o', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn('-o build/dc/modules/mod/a.o ', output)

    def test_keeps_its_stamp_and_needs_no_toolchain_for_a_goal_that_doesn_t_use_it(self):
        # make check, with GCC's C compiler changed, and with an SH_ELF that isn't there
        with self.built() as root:
            stamp = root / 'build/tools/gcc.id'
            text = stamp.read_text()
            with open(root / f'sh-elf/{CC1}', 'a') as file:
                file.write('\n# another\n')
            for gcc in ('SH_ELF=sh-elf', 'SH_ELF=custom'):
                with self.subTest(gcc):
                    status, output = make(root, '-n', 'check', gcc)
                    self.assertEqual(status, 0, output)
                    self.assertEqual(stamp.read_text(), text)

    def test_writes_a_body_source_s_bodies_again_with_another_table_and_not_with_another_shc(self):
        # a.c's GCC object waits for its bodies, which its table of offsets finds. No stub's listing does, so
        # another SHC compiles no stub and writes no body again, and its stamp stays as it was
        with stood_in({**BUILT, **PINNED}) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
            self.assertEqual(status, 0, output)
            settle(root)
            stamp = (root / 'build/tools/r32.id').read_text()
            (root / 'tools/shc-v5.0r32/shc.exe').write_text('another')
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
            self.assertEqual(status, 0, output)
            for step in (STUB_SHC, BODIES):
                self.assertNotIn(step, output)
            self.assertEqual((root / 'build/tools/r32.id').read_text(), stamp)
            os.utime(root / A_TABLE)
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn(BODIES, output)
        self.assertNotIn(STUB_SHC, output)

    @contextlib.contextmanager
    def built(self, files=None):
        """stood_in()'s checkout once make dc has built it and settle() has dated what it wrote."""
        with stood_in(files) as root:
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            settle(root)
            yield root


class LinkTime(unittest.TestCase):
    """make dc's link-time compile: the objects that carry GCC's intermediate code, and the link's command."""

    def carrying(self, output):
        """Each C object make -n compiles in output, by its path under build/dc/, with whether its compile takes
        -flto, which has it carry GCC's intermediate code."""
        return {path[len('build/dc/'):]: '-flto' in words for path, words in compiles(output).items()}

    def test_compiles_each_c_object_with_the_intermediate_code_but_the_object_of_a_source_a_row_names(self):
        # a row of a kernel, a warm row, a data row, a last row and a cold row each name a source whose object keeps
        # its own code, which the row places. A table names a source as its image's tables place it, so mod's names
        # b.src, whose object engine/dc/b.c compiles to. An over row and a window row name kernels and numbers, in
        # three fields or four, and a comment names nothing. main.c and free.c, which no row names, carry the code
        added = ('kernel', 'warm', 'data', 'last', 'cold', 'free')
        files = {**{f'engine/dc/{name}.c': '' for name in added}, 'engine/dc/b.c': '',
                 'config/dreamcast.mk': (B_REPLACED['config/dreamcast.mk'] + 'DC_ADDED := '
                                         + ' '.join(f'1st_read:engine/dc/{name}.c' for name in added) + '\n'),
                 'config/1st_read/kernel.txt': ('draw    engine/dc/kernel.c  step\nwindow  draw  4\n'
                                                'window  draw  4  1\nover    draw  draw\nover    draw  draw  mod\n'
                                                'warm    engine/dc/warm.c    step\ndata    engine/dc/data.c    datum\n'
                                                'last    engine/dc/last.c\ncold    engine/dc/cold.c  # src/main.c\n'
                                                '# cold  engine/dc/free.c\n'),
                 'config/mod/kernel.txt': 'pieces  modules/mod/b.src  b\nwarm    modules/mod/a.c    a_init\n',
                 'config/gcc.mk': GCC_MK + GCC_LTO}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertEqual(self.carrying(output), {
            'src/main.o': True, 'engine/dc/free.o': True, 'engine/dc/kernel.o': False, 'engine/dc/warm.o': False,
            'engine/dc/data.o': False, 'engine/dc/last.o': False, 'engine/dc/cold.o': False, 'modules/mod/a.o': False,
            'modules/mod/b.o': False})
        # GCC names the intermediate code's sections by a seed, which is the object's path, the same in any checkout
        main = compiles(output)['build/dc/src/main.o']
        self.assertIn('-frandom-seed=build/dc/src/main.o', main)
        self.assertIn('-ffat-lto-objects', main)

    def test_compiles_a_source_gcc_apart_names_without_the_intermediate_code(self):
        # a source that defines a datum another declares with another type stays out of the link-time compile,
        # which fails on such a pair: config/gcc.mk's GCC_APART names it, and its object keeps its own code
        files = {'engine/dc/apart.c': '', 'engine/dc/free.c': '',
                 'config/dreamcast.mk': 'DC_ADDED := 1st_read:engine/dc/apart.c 1st_read:engine/dc/free.c\n',
                 'config/1st_read/kernel.txt': '',
                 'config/gcc.mk': GCC_MK + GCC_LTO + 'GCC_APART := engine/dc/apart.c\n'}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        carrying = self.carrying(output)
        self.assertEqual((carrying['engine/dc/apart.o'], carrying['engine/dc/free.o'], carrying['src/main.o']),
                         (False, True, True))
        # the tree's own list names sources the tree holds
        apart = re.search(r'^GCC_APART := (.*)$', (ROOT / 'config/gcc.mk').read_text(), re.M).group(1).split()
        self.assertEqual([name for name in apart if not (ROOT / name).is_file()], [])

    def test_compiles_each_c_object_again_once_a_source_joins_or_leaves_the_kernel_tables_rows(self):
        # no C object depends on a table, so an edit to a row compiles nothing. Whether an object carries the code
        # depends on whether a row names its source, so each C object depends on build/dc/rows.txt, the record of the
        # sources the rows name, and compiles again once one joins or leaves: an incremental build then compiles
        # each object as a clean one does
        tables = {'config/1st_read/kernel.txt': '', 'config/mod/kernel.txt': 'warm  modules/mod/a.c  a_init\n',
                  'config/gcc.mk': GCC_MK + GCC_LTO}

        def clean(edited):
            with stood_in({**tables, **edited}) as fresh:
                status, output = make(fresh, '-n', 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            return self.carrying(output)

        self.assertEqual(clean({}), {'src/main.o': True, 'modules/mod/a.o': False})
        with stood_in(tables) as root:
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertEqual((root / 'build/dc/rows.txt').read_text(), 'modules/mod/a.c\n')
            settle(root)
            # another function of the same source, a comment, and a window
            edited = {'config/mod/kernel.txt': 'warm  modules/mod/a.c  a_start  # another\nwindow  pieces  4\n'}
            (root / 'config/mod/kernel.txt').write_text(edited['config/mod/kernel.txt'])
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual((status, compiles(output)), (0, {}), output)
            for edited in ({'config/mod/kernel.txt': ''},
                           {'config/mod/kernel.txt': '', 'config/1st_read/kernel.txt': 'cold  src/main.c\n'}):
                with self.subTest(edited):
                    for path, text in edited.items():
                        (root / path).write_text(text)
                    status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf', 'CC=true')
                    self.assertEqual(status, 0, output)
                    self.assertEqual(self.carrying(output), clean(edited))
                    status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
                    self.assertEqual(status, 0, output)
                    self.assertIn('  gcc    src/main.c\n  gcc    modules/mod/a.c\n', output)
                    settle(root)
                    status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf', 'CC=true')
                    self.assertEqual((status, compiles(output)), (0, {}), output)
            self.assertEqual((root / 'build/dc/rows.txt').read_text(), 'src/main.c\n')
            # with the record gone, nothing says which rows the objects were compiled under, so each compiles again,
            # whether or not a row left with it: main.c's object, built under its cold row, is none a clean build
            # of the tables without the row holds
            for edited in ({'config/mod/kernel.txt': '', 'config/1st_read/kernel.txt': 'cold  src/main.c\n'},
                           {'config/mod/kernel.txt': '', 'config/1st_read/kernel.txt': ''}):
                with self.subTest(gone=edited):
                    (root / 'build/dc/rows.txt').unlink()
                    for path, text in edited.items():
                        (root / path).write_text(text)
                    status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf', 'CC=true')
                    self.assertEqual(status, 0, output)
                    self.assertEqual(self.carrying(output), clean(edited))
                    status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
                    self.assertEqual(status, 0, output)
                    self.assertIn('  gcc    src/main.c\n  gcc    modules/mod/a.c\n', output)
                    settle(root)
                    status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf', 'CC=true')
                    self.assertEqual((status, compiles(output)), (0, {}), output)
            self.assertEqual((root / 'build/dc/rows.txt').read_text(), '\n')

    def test_the_compile_s_line_holds_shc_s_semantics_the_target_and_the_assembler_s_checks_and_fails_on_a_warning(self):
        # GCC_LTO_LINK as the repository's config/gcc.mk gives it. GCC runs its passes over the whole program under
        # the line's options, so the line holds each flag of GCC_AS_SHC, -frounding-math among them, and the zeros in
        # data. Its -m options are each of GCC_ABI's, the target every object compiles for. GNU as takes each of
        # GAS_CHECKS through -Wa, as for an object. A warning fails the compile, and so does a toolchain with no
        # plugin for the linker
        line = test_ld.flags('GCC_LTO_LINK')
        for name in ('GCC_AS_SHC', 'GCC_ZEROS_IN_DATA'):
            with self.subTest(name):
                self.assertEqual([flag for flag in test_ld.flags(name) if flag not in line], [])
        self.assertIn('-frounding-math', line)
        target = [flag for flag in test_ld.flags('GCC_ABI') if flag.startswith('-m')]
        self.assertEqual([flag for flag in line if flag.startswith('-m')], target)
        self.assertIn('-m4-single-only', target)
        checks = [f'-Wa,{check}' for check in test_ld.flags('GAS_CHECKS')]
        self.assertEqual([flag for flag in line if flag.startswith('-Wa,')], checks)
        self.assertIn('-Wa,--fatal-warnings', checks)
        for flag in ('-Werror', '-fuse-linker-plugin', '-flto-partition=1to1', '-ffunction-sections', '-fdata-sections'):
            with self.subTest(flag):
                self.assertIn(flag, line)
        # make dc's recipe gives tools/ld.py the line between single quotes, and ld.py splits it at spaces
        self.assertNotRegex(' '.join(line), r"['\\\"$`]")

    def test_gives_the_link_its_link_time_flags_as_one_word_where_they_are_one_flag_too(self):
        # tools/ld.py takes --lto=FLAGS as one word, so a FLAGS of one flag, which starts with a dash, is no option of
        # its own. The spec links no image, so the link runs no compile
        with stood_in({'config/gcc.mk': f'{GCC_MK}GCC_LTO_LINK := -O2\n'}) as root:
            status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertIn("SH_ELF=sh-elf python3 tools/ld.py --lto='-O2' build/dc/link/spec.txt build/dc/link\n", output)
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=true')
        self.assertEqual(status, 0, output)


class Check(unittest.TestCase):
    def test_builds_and_sums_what_make_dc_builds_with_neither_the_disc_nor_shc_on_any_host(self):
        # orig/ is empty, and the tree holds no wibo, no release of SHC and no stamp of either. make dc-check compiles
        # or assembles each of make dc's objects but a.c's and mod's content, and archives SH4ZAM's. build/prskit
        # compresses its library and the archive. build/dc/check.txt then holds sha256sum's line for each of those
        # files and for libgcc.a, in the order of their paths, which sort puts them in under the C locale. The
        # inventory of those objects lies under build/dc/check/, beside the compressed files, where no one takes it
        # for the sums. The check fetches nothing, as curl, which RAN stands in for, would. It extracts no disc where
        # orig/ holds a .gdi, writes no body from the disc and no content, and links nothing, on Linux as on a Mac.
        # Of make dc's checks it runs the import check, on the uses of the imports, ahead of its first compile, and
        # the census's first pass alone, with no option, as its last step
        archive = 'build/dc/sh4zam/sh4/a.o\nbuild/dc/sh4zam/sh4/b.o\n'
        for uname, disc in ((LINUX, {}), (LINUX, {'orig/disc.gdi': ''}), *((mac, {}) for mac in MACS)):
            files = {**SEARCHED, **CHECK_NEEDS, **PARTLY_CHECKED, **disc, 'bin/curl': RAN}
            with self.subTest(uname, disc=bool(disc)), checkout(files) as root:
                settle(root)
                path = path_with_bin(root)
                status, output = make(root, '-n', 'dc-check', 'SH_ELF=sh-elf', 'CC=bin/cc', path=path, uname=uname)
                self.assertEqual(status, 0, output)
                for step in ('curl', 'wibo', 'shc.exe', 'build/gdi', 'build/dc/modules/mod/a.o', 'build/dc/gen/',
                             'tools/data.py', 'tools/content.py', 'tools/imports.py --definitions',
                             'tools/spec.py'):
                    self.assertNotIn(step, output)
                self.assertLess(output.index('\npython3 tools/imports.py --uses --add 1st_read engine/dc/other.c\n'),
                                output.index('sh-elf/bin/sh-elf-gcc'), output)
                self.assertRegex(output, r'(?m)^LC_ALL=C sort -k 2 ')
                self.assertEqual(re.findall(r'(?m)^.*tools/census\.py.*$', output), ['python3 tools/census.py'])
                self.assertEqual(output.splitlines()[-1], 'python3 tools/census.py')
                self.assertEqual(make(root, 'dc-check', 'SH_ELF=sh-elf', 'CC=bin/cc', path=path, uname=uname),
                                 (0, CHECKS))
                self.assertEqual((root / 'build/dc/check.txt').read_text(), sums(root, SUMMED))
                self.assertEqual((root / 'build/dc/check/objects.txt').read_text().split(), SUMMED_OBJECTS)
                self.assertEqual((root / 'build/dc/sh4zam/libsh4zam.a').read_text(), archive)
                self.assertEqual((root / 'build/dc/check/libsh4zam.a.prs').read_text(), f'compressed\n{archive}')
                self.assertEqual((root / 'build/dc/check/prskit.c.prs').read_text(), 'compressed\na library\n')
                self.assertTrue((root / 'build/dc/imports.ok').exists())
                for unmade in ('ran', WIBO, 'build/tools/wibo.id', 'build/tools/r32.id', 'build/gen',
                               'build/unpack', 'build/dc/modules/mod/a.o', 'build/dc/gen', 'build/dc/census.ok',
                               'build/dc/link', 'build/dc/manifest.txt', 'build/dc/check.txt.tmp'):
                    self.assertFalse((root / unmade).exists(), unmade)

    def test_writes_the_reads_of_each_body_source_and_each_c_source_of_1st_read_bin_s_on_any_host(self):
        # the host's C compiler writes the reads: a.c's, once its zero body is in place, and main.c's, and no other
        # source's. That zero body is all build/stub/ then holds, for the reads need no stub built, on Linux as on a
        # Mac
        for uname in (LINUX, *MACS):
            with self.subTest(uname), checkout({**SEARCHED, **CHECK_NEEDS, **PARTLY_CHECKED}) as root:
                settle(root)
                status, output = make(root, '-n', 'dc-check', 'SH_ELF=sh-elf', 'CC=bin/cc', uname=uname)
                self.assertEqual(status, 0, output)
                self.assertEqual(re.findall(r'(?m)^bin/cc -MM .* -MT (\S+) ', output), list(READS))
                self.assertLess(output.index(f'echo 0 > {STUBBED[0]}'), output.index('bin/cc -MM '), output)
                self.assertEqual(make(root, 'dc-check', 'SH_ELF=sh-elf', 'CC=bin/cc', uname=uname), (0, CHECKS))
                self.assertEqual(self.held(root, 'build/reads'), READS)
                self.assertEqual(list(self.held(root, 'build/stub')), STUBBED)

    def test_holds_1st_read_bin_s_sources_to_the_census_on_each_run_and_fails_where_it_does(self):
        # no file records that the census passed, so each run checks it, once the sums and the reads are written.
        # main.c comes to hold a number for RAM past the image, which no row of the census explains. make dc-check
        # checks the imports again, compiles and sums main.c's object, and then fails, naming the number. It fails on
        # the next run too, which builds nothing, and passes once a row explains the number
        unexplained = 'census: src/main.c:1: 0x0cc00080, which no row of config/1st_read/census.txt explains\n'
        with self.checked() as root:
            (root / 'src/main.c').write_text('int far = 0x0cc00080;\n')
            for built in (IMPORT_LINE + '  gcc    src/main.c\n  sums   build/dc/check.txt\n', ''):
                status, output = self.check(root)
                self.assertEqual(status, 2, output)
                self.assertRegex(output, rf'\A{re.escape(built + CENSUS_LINE + unexplained)}.*: \*\*\* '
                                         r'\[dc\.mk:\d+: dc-check\] Error 1\n\Z')
            (root / 'config/1st_read/census.txt').write_text('src/main.c  0x0cc00080  1  data  the reason\n')
            status, output = self.check(root)
            self.assertEqual((status, output[-len(CENSUS_LINE):]), (0, CENSUS_LINE), output)

    def test_reads_each_body_source_s_table_of_offsets_and_none_again_once_its_source_changes_on_any_host(self):
        # a table records nothing of its source's text. a.c changes, in a token SHC compiles too: make dc-check
        # checks the imports again, reads no table again, and passes. A table with a row that isn't one fails,
        # naming the row, on that run and the next, and one that reads passes again. On Linux as on a Mac
        unread = 'offsets: offsets/modules/mod/a.txt:1: not a row: KIND NAME SECTION OFFSET SIZE\n'
        for uname in (LINUX, *MACS):
            with self.subTest(uname), checkout({**SEARCHED, **CHECK_NEEDS, **PARTLY_CHECKED}) as root:
                settle(root)
                self.assertEqual(self.check(root, uname), (0, CHECKS))
                settle(root)
                (root / 'modules/mod/a.c').write_text(A_BODY.replace('a[1]', 'a[2]'))
                self.assertEqual(self.check(root, uname), (0, IMPORT_LINE + CENSUS_LINE))
                settle(root)
                (root / A_TABLE).write_text('datum  a  D  0x00000000\n')
                for _ in range(2):
                    status, output = self.check(root, uname)
                    self.assertEqual(status, 2, output)
                    self.assertTrue(output.startswith(CHECKS_TABLES(1) + unread), output)
                (root / A_TABLE).write_text('datum  a  D  0x00000000  8\n')
                self.assertEqual(self.check(root, uname), (0, CHECKS_TABLES(1) + CENSUS_LINE))

    def test_stops_on_a_body_source_with_no_table_of_offsets_on_any_host(self):
        # by the Makefile's rule for a table that isn't there, under make -n too. A table under another name, as a
        # source renamed without its table leaves one, is no table of a.c's
        for uname in (LINUX, *MACS):
            with self.subTest(uname), checkout({**SEARCHED, **CHECK_NEEDS, **PARTLY_CHECKED}) as root:
                settle(root)
                (root / A_TABLE).rename(root / 'offsets/modules/mod/z.txt')
                for flags in ((), ('-n',)):
                    status, output = make(root, *flags, 'dc-check', 'SH_ELF=sh-elf', 'CC=bin/cc', uname=uname)
                    self.assertEqual(status, 2, output)
                    self.assertRegex(output, r"Makefile:\d+: \*\*\* offsets/modules/mod/a\.txt: missing; it records "
                                             r"where modules/mod/a\.c's data sits in the original\. make offsets "
                                             r'writes it on x86 Linux; a renamed source takes a git mv; see '
                                             r'TOOLCHAIN\.md#stub-and-full-builds\.  Stop\.\n\Z')
                    self.assertFalse((root / 'build/dc/offsets.ok').exists())

    def test_checks_the_tables_of_offsets_again_once_a_table_or_tools_offsets_py_changes(self):
        # the check depends on each body source's table, on the tables' inventory, on dc.mk, which gives its recipe,
        # and on tools/offsets.py and each module of tools/ it imports, directly or through another, and on no
        # other tool and no source. Those modules are ones the vendored tools' check imports too, which this tree
        # can't pass, so the table and tools/offsets.py alone change here. A source renamed with its table keeps the
        # table's date, as a table of main.c's does here once main.c takes a body: the inventory has make read the
        # tables again, and it names main.c's, whose row isn't one
        with self.checked() as root:
            status, output = make(root, '-pn', 'dc-check', 'SH_ELF=sh-elf', 'CC=bin/cc')
            self.assertEqual(status, 0, output[-3000:])
            needs = re.search(r'(?m)^build/dc/offsets\.ok: (.*)$', output)[1].split()
            self.assertEqual(sorted(needs), sorted({A_TABLE, 'build/dc/offsets.txt', 'dc.mk',
                                                    *imported(['offsets'])}))
            self.assertEqual((root / 'build/dc/offsets.txt').read_text().split(), [A_TABLE])
            for path in (A_TABLE, 'tools/offsets.py'):
                with self.subTest(path):
                    os.utime(root / path)
                    status, output = self.check(root)
                    self.assertEqual(status, 0, output)
                    self.assertIn(CHECKS_TABLES(1), output)
                    settle(root)
            (root / 'src/main.c').write_text('static int a[2] = {\n#include "main/a.inc"\n};\n')
            (root / 'offsets/src').mkdir()
            (root / 'offsets/src/main.txt').write_text('datum  a  D  0x00000000\n')
            settle(root)
            status, output = self.check(root)
            self.assertEqual(status, 2, output)
            self.assertIn(CHECKS_TABLES(2) + 'offsets: offsets/src/main.txt:1: not a row: KIND NAME SECTION OFFSET '
                          'SIZE\n', output)

    def test_sums_again_once_an_object_changes_leaves_or_joins_and_builds_nothing_otherwise(self):
        # a second run builds nothing, leaves the sums as they are, checks no import and runs the census alone. Once
        # main.c changes, its object's line alone changes. Once main.c takes a body from the disc, make dc-check
        # builds its object no longer: the inventory of its objects has the sums written again without the line,
        # though the object still lies there. With the body gone again, the line is back. Each change to main.c,
        # which the import check reads, runs that check again. A table of offsets joins the body sources' as main.c
        # takes its body, and leaves with it, and each time make dc-check checks the tables again
        main = 'build/dc/src/main.o'
        with self.checked() as root:
            first = (root / 'build/dc/check.txt').read_text()
            self.assertEqual(self.check(root), (0, CENSUS_LINE))
            self.assertEqual((root / 'build/dc/check.txt').read_text(), first)
            (root / 'src/main.c').write_text('\n')
            self.assertEqual(self.check(root), (0, IMPORT_LINE + '  gcc    src/main.c\n  sums   build/dc/check.txt\n'
                                                   + CENSUS_LINE))
            changed = (root / 'build/dc/check.txt').read_text()
            self.assertEqual(changed, sums(root, SUMMED))
            self.assertEqual(differing(first, changed), [main])
            settle(root)
            body = 'static int a[1] = {\n#include "main/a.inc"\n};\n'
            (root / 'src/main.c').write_text(body)
            (root / 'offsets/src').mkdir()
            (root / 'offsets/src/main.txt').write_text(NO_ROWS)
            self.assertEqual(self.check(root), (0, IMPORT_LINE + '  sums   build/dc/check.txt\n' + CHECKS_TABLES(2)
                                                   + CENSUS_LINE))
            self.assertEqual((root / 'build/dc/check.txt').read_text(),
                             sums(root, [path for path in SUMMED if path != main]))
            self.assertTrue((root / main).exists())
            self.assertEqual(self.check(root), (0, CENSUS_LINE))
            settle(root)
            (root / 'src/main.c').write_text('\n')
            self.assertEqual(self.check(root), (0, IMPORT_LINE + '  gcc    src/main.c\n  sums   build/dc/check.txt\n'
                                                   + CHECKS_TABLES(1) + CENSUS_LINE))
            self.assertEqual((root / 'build/dc/check.txt').read_text(), changed)

    def test_sums_again_with_another_libgcc_a_or_another_prskit_and_compiles_nothing(self):
        # the linker's stamp holds libgcc.a's hash, and the sums depend on it themselves, not only through the
        # archive, which does for its archiver. With another libgcc.a, make archives SH4ZAM's objects again, to the
        # same bytes, and libgcc.a's line alone changes. Each compressed file depends on build/prskit, so one whose
        # compress writes "packed" changes their two lines
        with self.checked() as root:
            status, output = make(root, '-pn', 'dc-check', 'SH_ELF=sh-elf', 'CC=bin/cc')
            self.assertEqual(status, 0, output[-3000:])
            self.assertIn('build/tools/ld.id', re.search(r'(?m)^build/dc/check\.txt: (.*)$', output)[1].split())
            first = (root / 'build/dc/check.txt').read_text()
            (root / f'sh-elf/{LIBGCC}').write_text('another\n')
            self.assertEqual(self.check(root), (0, '  ar     build/dc/sh4zam/libsh4zam.a\n'
                                                   '  sums   build/dc/check.txt\n' + CENSUS_LINE))
            second = (root / 'build/dc/check.txt').read_text()
            self.assertEqual(second, sums(root, SUMMED))
            self.assertEqual(differing(first, second), [f'sh-elf/{LIBGCC}'])
            settle(root)
            (root / 'build/prskit').write_text(COMPRESSES.replace('compressed', 'packed'))
            self.assertEqual(self.check(root), (0, '  sums   build/dc/check.txt\n' + CENSUS_LINE))
            third = (root / 'build/dc/check.txt').read_text()
            self.assertEqual(third, sums(root, SUMMED))
            self.assertEqual(differing(second, third), SUMMED[:2])

    def test_fails_and_writes_no_sums_where_sha256sum_can_t_read_a_file(self):
        # sha256sum's lines reach sort through a file. Through a pipe the shell would give sort's status alone, and
        # the sums would pass without the line of a file sha256sum failed on. A build/prskit whose compress makes a
        # directory gives sha256sum two files it can't read: make dc-check fails, naming each, and writes no sums
        makes_a_directory = {'build/prskit': '#!/bin/sh\n[ "$1" = compress ] && mkdir "$3"\n'}
        with checkout({**SEARCHED, **CHECK_NEEDS, **PARTLY_CHECKED, **makes_a_directory}) as root:
            settle(root)
            status, output = self.check(root)
            self.assertEqual(status, 2, output)
            for unread in SUMMED[:2]:
                self.assertIn(f'sha256sum: {unread}: Is a directory\n', output)
            self.assertFalse((root / 'build/dc/check.txt').exists())

    def test_stamps_no_wibo_or_release_of_shc_and_passes_where_a_setting_names_one_that_isn_t_there(self):
        # Linux runs SHC, and for a goal that may run it make stamps wibo and each release as it starts, and stops on
        # one a setting names that isn't there. make dc-check runs none. With the four in place it writes no stamp of
        # theirs, and with WIBO, R10, R26 and R32 naming files that aren't there it still passes. Typed beside make
        # check, which runs SHC, it leaves make check its stamps: make stops on the wibo that isn't there
        gone = [f'{setting}=gone/{setting.lower()}' for setting in ('WIBO', 'R10', 'R26', 'R32')]
        with checkout({**SEARCHED, **CHECK_NEEDS, **PARTLY_CHECKED, **PINNED}) as root:
            settle(root)
            self.assertEqual(self.check(root), (0, CHECKS))
            self.assertEqual(sorted(path.name for path in (root / 'build/tools').glob('*.id')),
                             ['cc.id', 'gcc.id', 'ld.id'])
            settle(root)
            self.assertEqual(make(root, 'dc-check', 'SH_ELF=sh-elf', 'CC=bin/cc', *gone), (0, CENSUS_LINE))
            for goals in (('check', 'dc-check'), ('dc-check', 'check')):
                with self.subTest(goals=goals):
                    status, output = make(root, '-n', *goals, 'SH_ELF=sh-elf', 'CC=bin/cc', *gone)
                    self.assertEqual(status, 2, output)
                    self.assertRegex(output, r'\AMakefile:\d+: \*\*\* gone/wibo: no such file\.  Stop\.\n\Z')

    def test_shares_its_objects_and_the_import_check_with_make_dc_and_neither_makes_the_other_s_again(self):
        # the objects make dc-check builds are make dc's own, by make dc's rules, and so is the import check: after
        # either goal, the other compiles none of them and checks no import again. make dc-check adds the two files
        # build/prskit compresses, the sums, the reads, main.c's here, and its pass of the census, and make dc its
        # census and the link. Each runs with the one C compiler, whose stamp build/prskit depends on
        with stood_in({'build/prskit': COMPRESSES, 'bin/cc': CC}) as root:
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=bin/cc')
            self.assertEqual(status, 0, output)
            settle(root)
            self.assertEqual(self.check(root), (0, '  sums   build/dc/check.txt\n' + CENSUS_LINE))
            self.assertEqual(list(self.held(root, 'build/reads')), ['build/reads/src/main.d'])
            self.assertEqual([line.split()[1] for line in (root / 'build/dc/check.txt').read_text().splitlines()],
                             ['build/dc/check/libsh4zam.a.prs', 'build/dc/check/prskit.c.prs',
                              'build/dc/modules/mod/a.o', 'build/dc/modules/mod/b.o', 'build/dc/sh4zam/libsh4zam.a',
                              'build/dc/src/main.o', f'sh-elf/{LIBGCC}'])
        with stood_in({'build/prskit': COMPRESSES, 'bin/cc': CC}) as root:
            self.assertEqual(self.check(root)[0], 0)
            settle(root)
            status, output = make(root, 'dc', 'SH_ELF=sh-elf', 'CC=bin/cc')
            self.assertEqual((status, output), (0, '  census config/1st_read/census.txt\n  link   build/dc/link\n'))

    def test_fails_on_an_import_or_a_use_of_one_that_nothing_holds_before_it_builds_on_any_host(self):
        # 1ST_READ.BIN takes a_init from mod, and main.c's main calls it for no stage, with no row of uses.txt: a
        # use the matching build can't tell from one of another image's a_init. Or it takes stray, whose address
        # no row of mod's splits.txt holds. make dc-check fails on either as it starts, naming it, with nothing
        # compiled and no sums, on Linux as on a Mac. With the use's row, or without the import, it passes
        use = {'config/1st_read/imports.txt': 'a_init  mod\n', 'src/main.c': 'void main(void)\n{\n    a_init();\n}\n'}
        stray = {'config/1st_read/imports.txt': 'stray  mod\n',
                 'config/mod/symbols.txt': MODULE['config/mod/symbols.txt'] + '0x0c900050  stray\n'}
        cases = (('use', use, 'src/main.c:3: main names a_init, which config/1st_read/imports.txt:1 takes from mod, '
                              'for no stage this source gives it, and config/1st_read/uses.txt gives it no row for '
                              'mod', {'config/1st_read/uses.txt': 'src/main.c  main  mod  a reason\n'}),
                 ('import', stray, 'config/1st_read/imports.txt:1: stray: no row of config/mod/splits.txt holds '
                                   '0x0c900050', {'config/1st_read/imports.txt': ''}))
        for uname in (LINUX, *MACS):
            for case, files, problem, held in cases:
                with self.subTest(uname, case=case), \
                        checkout({**SEARCHED, **CHECK_NEEDS, **PARTLY_CHECKED, **files}) as root:
                    settle(root)
                    status, output = make(root, 'dc-check', 'SH_ELF=sh-elf', 'CC=bin/cc', uname=uname)
                    self.assertEqual(status, 2, output)
                    self.assertEqual(output.splitlines()[:2], ['  import config/1st_read/imports.txt',
                                                               f'imports: {problem}'])
                    self.assertNotIn('  gcc ', output)
                    for unmade in ('build/dc/imports.ok', 'build/dc/src/main.o', 'build/dc/check.txt'):
                        self.assertFalse((root / unmade).exists(), unmade)
                    for path, text in held.items():
                        (root / path).write_text(text)
                    self.assertEqual(make(root, 'dc-check', 'SH_ELF=sh-elf', 'CC=bin/cc', uname=uname), (0, CHECKS))

    def test_checks_the_imports_again_once_a_file_the_check_reads_or_its_command_changes_joins_or_leaves(self):
        # the import check's rule names every C file under ROOTS, the C DC_ADDED gives make dc, engine/dc/'s headers,
        # the tables, the tool and its modules, and the two makefiles that give its command. A change to one runs
        # the check again, as does a header or a table that joins or leaves, through the inventories. With nothing
        # changed, make dc-check checks nothing
        check = '\npython3 tools/imports.py --uses --add 1st_read engine/dc/other.c\n'
        files = {'src/main.h': '', 'engine/dc/other.h': '', 'config/1st_read/uses.txt': ''}
        with self.checked(files) as root:
            self.assertNotIn('tools/imports.py', self.planned(root))
            for changed in ('src/main.c', 'src/main.h', 'modules/mod/a.c', 'engine/dc/other.c', 'engine/dc/other.h',
                            'config/1st_read/uses.txt', 'config/mod/symbols.txt', 'tools/imports.py',
                            'tools/stages.py', 'dc.mk', 'config/dreamcast.mk'):
                with self.subTest(changed=changed):
                    os.utime(root / changed)
                    self.assertIn(check, self.planned(root))
                    settle(root)
                    self.assertNotIn('tools/imports.py', self.planned(root))
            # a file that joins is dated as the check's other sources are, before the check last passed
            dated = (root / 'src/main.c').stat().st_mtime
            for joined in ('src/joined.h', 'engine/dc/joined.h', 'config/mod/joined.txt'):
                with self.subTest(joined=joined):
                    (root / joined).write_text('')
                    os.utime(root / joined, (dated, dated))
                    self.assertIn(check, self.planned(root))
                    settle(root)
            for left in ('src/main.h', 'engine/dc/other.h', 'config/1st_read/uses.txt'):
                with self.subTest(left=left):
                    (root / left).unlink()
                    self.assertIn(check, self.planned(root))
                    settle(root)

    def test_checks_the_imports_again_once_dc_added_gives_1st_read_bin_another_source(self):
        # config/dreamcast.mk comes to give 1ST_READ.BIN engine/dc/owner.c, and neither file is newer than the last
        # check. The record of make dc's pairs changes with the entry, and the check depends on it, so it runs again
        # and reads owner.c. With the entry gone again it runs once more, without it
        check = '\npython3 tools/imports.py --uses --add 1st_read engine/dc/other.c'
        with self.checked({'engine/dc/owner.c': 'own owner\n'}) as root:
            added = root / 'config/dreamcast.mk'
            given = added.read_text()

            def give(text):
                # config/dreamcast.mk with text, dated as it was
                dated = added.stat().st_mtime
                added.write_text(text)
                os.utime(added, (dated, dated))

            give(given.replace('other.c\n', 'other.c 1st_read:engine/dc/owner.c\n'))
            self.assertIn(f'{check} --add 1st_read engine/dc/owner.c\n', self.planned(root))
            settle(root)
            self.assertNotIn('tools/imports.py', self.planned(root))
            give(given)
            self.assertIn(f'{check}\n', self.planned(root))

    def check(self, root, uname=LINUX):
        """make dc-check's status and output in root on the host uname names, with the stand-in toolchain and CC as
        the host's C compiler."""
        return make(root, 'dc-check', 'SH_ELF=sh-elf', 'CC=bin/cc', uname=uname)

    def held(self, root, directory):
        """The text of each file under directory in root, by its path from root, in the paths' order."""
        return {path.relative_to(root).as_posix(): path.read_text()
                for path in sorted((root / directory).rglob('*')) if path.is_file()}

    def planned(self, root, *settings):
        """The commands make -n dc-check prints in root, with the stand-in toolchain and settings."""
        status, output = make(root, '-n', 'dc-check', 'SH_ELF=sh-elf', 'CC=bin/cc', *settings)
        self.assertEqual(status, 0, output)
        return output

    @contextlib.contextmanager
    def checked(self, files=None):
        """A checkout of CHECK_NEEDS, PARTLY_CHECKED and files once make dc-check has built it and settle() has
        dated what it wrote."""
        with checkout({**SEARCHED, **CHECK_NEEDS, **PARTLY_CHECKED, **(files or {})}) as root:
            settle(root)
            self.assertEqual(self.check(root), (0, CHECKS))
            settle(root)
            yield root


class Sums(unittest.TestCase):
    def test_passes_with_make_dc_s_configuration_broken(self):
        # a call left open in the repository's config/gcc.mk, which only make dc's goals read, stops make dc and
        # neither make nor make check
        broken = (ROOT / 'config/gcc.mk').read_text() + 'GCC_TARGET += $(call quoted,-fno-common\n'
        for goals, count in (((), 12), (('check',), 3)):
            with self.subTest(goals=goals), checkout(HASHED) as root:
                # make -n check, with the CC=true the tests give, stamps the tools and writes build/deps.mk, which
                # settle() then dates
                status, output = make(root, '-n', 'check', 'CC=true')
                self.assertEqual(status, 0, output)
                settle(root)
                (root / 'config/gcc.mk').write_text(broken)
                self.assertEqual(make(root, *goals, 'CC=true'), (0, f'  sums   {count} files match SHA256SUMS\n'))
                status, output = make(root, '-n', 'dc', 'SH_ELF=sh-elf')
                self.assertNotEqual(status, 0, output)
                self.assertRegex(output, r"\Aconfig/gcc\.mk:\d+: \*\*\* unterminated call to function 'call': "
                                         r"missing '\)'\.  Stop\.\n\Z")


class Starts(unittest.TestCase):
    def test_plans_each_link_with_the_output_sections_config_dreamcast_mk_starts_and_names_none_where_it_gives_none(self):
        # DC_STARTS goes to tools/spec.py as --starts, for make dc's spec and make dc-order's alike. Gcc's tests of
        # both commands hold that an empty DC_STARTS gives no option
        planned = ('SH_ELF=sh-elf python3 tools/spec.py --archive build/dc/sh4zam/libsh4zam.a --starts '
                   '.rodata,.data,.bss --list build/dc/{0}/unbound.txt build/dc/{0}/spec.txt')
        with stood_in({'config/dreamcast.mk': 'DC_STARTS := .rodata,.data,.bss\n'}) as root:
            for goal, directory in (('dc', 'link'), ('dc-order', 'order')):
                with self.subTest(goal):
                    status, output = make(root, '-n', goal, 'SH_ELF=sh-elf')
                    self.assertEqual(status, 0, output)
                    self.assertIn(planned.format(directory), output.splitlines())

    def test_a_starts_or_packed_row_of_a_kernel_table_names_no_source_that_keeps_its_own_code(self):
        # DC_KERNEL_SOURCES takes the source of each row that names one, whose object carries no intermediate code.
        # A starts row of one output section has as many fields as a last or cold row, and one of two as many as a
        # kernel or warm row, and neither names a source
        for starts in ('starts  .data', 'starts  .rodata  .data'):
            tables = {'config/1st_read/kernel.txt': f'draw  modules/mod/a.c  f\n{starts}\n',
                      'config/mod/kernel.txt': 'packed\n'}
            with self.subTest(starts), stood_in(tables) as root:
                status, output = make(root, '-pqn', 'dc', 'SH_ELF=sh-elf', 'CC=true')
                self.assertIn(status, (0, 1), output[-3000:])
                self.assertEqual(re.search(r'(?m)^DC_KERNEL_SOURCES := (.*)$', output)[1].split(),
                                 ['modules/mod/a.c'])


class Pragmas(unittest.TestCase):
    def test_gcc_compiles_no_pragma_but_those_it_knows_or_may_ignore(self):
        # GCC knows its own #pragma GCC. GCC_WARNINGS' -Wno-unknown-pragmas lets it ignore SHC's #pragma inline and
        # #pragma section, and tools/inline.py splits the inline assembly's pragmas off before GCC reads the C. Any
        # other pragma GCC doesn't know, such as #pragma global_register, sits on the matching side of its guard, where
        # GCC never reads it. A line whose group the tool can't tell counts as compiled
        known = {'GCC', 'inline', 'section', 'pack', 'inline_asm', 'asm', 'endasm'}
        found = []
        for top in ('src', 'modules', 'events', 'sdk', 'loader', 'gcc'):
            for path in sorted((ROOT / top).rglob('*.[ch]')):
                text = path.read_text(encoding='latin-1')
                if 'pragma' not in text:
                    continue
                takes = csource.compiled(csource.blanked(text).split('\n'))
                for directive in csource.parse(text).directives:
                    words = csource.pragma(directive)
                    if words and words[0] not in known and takes[directive.line - 1] is not False:
                        found.append(f'{path.relative_to(ROOT)}:{directive.line}: #pragma {words[0]}')
        self.assertEqual(found, [])


if __name__ == '__main__':
    unittest.main()
