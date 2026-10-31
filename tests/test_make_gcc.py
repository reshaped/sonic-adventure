import ast
import contextlib
import hashlib
import os
import re
import shlex
import unittest
from pathlib import Path

import csource
import memory
import support
import test_ld
from support import (BODIES, BUILT, COMMANDS, COMPILERS, CURL, FOUND, HASH, HASHED, MOD_DATA, MODULE, MOTION, ORIGINALS,
                     PINNED, SEARCHED, STUB_SHC, depends, host, listing, make, placing, settle)

ROOT = Path(__file__).resolve().parents[1]
# a config/gcc.mk that names no source, with the flags of GNU as and SH4ZAM's include directory, which gcc.mk reads
GCC_MK = ('GAS_CHECKS := --isa=sh4\nGASFLAGS := $(GAS_CHECKS) --no-pad-sections\n'
          'SH4ZAM_INCLUDE := tools/sh4zam/include\n')
# cc1's and libgcc.a's paths in the toolchain, which hold GCC's version
CC1 = 'libexec/gcc/sh-elf/16.2.0/cc1'
LIBGCC = 'lib/gcc/sh-elf/16.2.0/libgcc.a'
# the toolchain's files that make gcc's stamps hash, by their paths in SH_ELF
SH_ELF_FILES = ('bin/sh-elf-gcc', CC1, 'bin/sh-elf-as', 'bin/sh-elf-ld', 'bin/sh-elf-ar', LIBGCC)
# the toolchain make builds on first use, in place, each file empty
PINNED_TOOLCHAIN = {f'tools/sh-elf/{path}': '' for path in SH_ELF_FILES}


def gccflags():
    """A line of config/gcc.mk that sets GCCFLAGS to the flags the repository's config/gcc.mk gives it."""
    return f'GCCFLAGS := {shlex.join(test_ld.flags())}\n'


@contextlib.contextmanager
def checkout(files=None):
    """A tree make gcc runs in: support.checkout()'s, with gcc.mk, GCC_MK and files. Its config/imports.txt and
    config/declarations.txt are empty, and its tools/memory.py reads no source. Each file whose text opens with #! is
    executable."""
    files = {'gcc.mk': (ROOT / 'gcc.mk').read_text(), 'config/gcc.mk': GCC_MK, 'config/imports.txt': '',
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
    """tools/memory.py with sources for its SOURCES, the sources make gcc's link depends on: none by default, as the
    checkout holds none of the tree's."""
    return (ROOT / 'tools/memory.py').read_text() + f'\nSOURCES = {tuple(sources)!r}\n'


def path_with_bin(root):
    """A PATH that searches root's bin/ first, where a test's stand-ins for the host's commands lie."""
    return f'{root / "bin"}{os.pathsep}{os.environ["PATH"]}'


def compiles(output):
    """Each GCC compile make -n prints, as its words, by the object it writes."""
    return {words[words.index('-o') + 1]: words for words in map(str.split, output.splitlines())
            if 'sh-elf/bin/sh-elf-gcc' in words and '-o' in words}


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

# make gcc's link, its spec's writer and its publisher stood in by scripts that print nothing. The writer writes an
# empty spec, of no image, which tools/ld.py links without a pass. The publisher writes each image's final ELF and
# files, empty, and then the manifest, at the path its last argument gives
LINK = {'tools/spec.py': "import sys\nopen(sys.argv[-1], 'w').close()\n",
        'tools/publish.py': ("import sys\nfrom pathlib import Path\nsys.path[:0] = ['tools']\nimport images\n"
                             "for name, path in images.files().items():\n"
                             "    for each in (f'build/gcc/link/{name}/final.elf', f'build/gcc/{path}',\n"
                             "                 *([f'build/gcc/{path[:-4]}.BIN'] if path.endswith('.PRS') else [])):\n"
                             "        Path(each).parent.mkdir(parents=True, exist_ok=True)\n"
                             "        Path(each).write_bytes(b'')\n"
                             "open(sys.argv[-1], 'w').write('a manifest\\n')\n")}
EMPTY_HASH = hashlib.sha256(b'').hexdigest()
# what make gcc needs beyond a checkout(): its toolchain and build/shckit stood in by scripts, its link stood in by
# LINK, and what its checks read, in place. A test passes CC=true to pass the host check: nothing here runs the host's
# C compiler
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
       for name, tool in (('wibo', 'tools/wibo-i686'),
                          *((release, f'tools/shc-v5.0{release}/shc.exe') for release in ('r10', 'r26', 'r32')))},
    # main.c's SHC object, with its .d file and listing, which the census and the import check read
    'build/src/main.obj': '',
    'build/src/main.d': depends('build/src/main.obj', 'src/main.c'),
    'build/src/main.lis': '',
    # no name past the image, and no row in the census: census.py --gcc reads 1ST_READ.BIN's GCC objects for a row of
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


class Host(unittest.TestCase):
    def test_gcc_names_each_command_it_lacks_before_any_step(self):
        # no pinned tool is in place. curl downloads them, tar and gzip unpack SHC's releases, and cmp checks each
        # content
        with checkout() as root, host(*COMMANDS) as path:
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=no-such-cc', path=path)
            self.assertFalse((root / 'build/deps.mk').exists())
            self.assertFalse((root / 'build/gcc/deps.mk').exists())
        self.assertNotEqual(status, 0, output)
        self.assertIn('*** no-such-cc sha256sum timeout curl tar gzip cmp: not found; see README.md#build', output)

    def test_gcc_needs_tar_xz_gzip_and_cpp_to_build_its_toolchain_alone(self):
        # SHC's releases are in place, without the toolchain and then with it
        with checkout(PINNED) as root, host(*COMMANDS) as path:
            status, output = make(root, 'gcc', 'CC=no-such-cc', path=path)
        self.assertNotEqual(status, 0, output)
        self.assertIn('*** no-such-cc sha256sum timeout curl tar gzip xz c++ cmp: not found; see README.md#build',
                      output)
        with checkout({**PINNED, 'tools/sh-elf/bin/sh-elf-gcc': ''}) as root, host('curl', 'tar', 'gzip', 'xz', 'c++') \
                as path:
            status, output = make(root, 'gcc', 'CC=no-such-cc', path=path)
        self.assertNotEqual(status, 0, output)
        self.assertIn('*** no-such-cc: not found; see README.md#build', output)

    def test_gcc_needs_cmp_for_each_content_s_check(self):
        # and so does gcc-disc, whose link waits for each content's check
        for goal in ('gcc', 'gcc-disc'):
            with self.subTest(goal), checkout({**PINNED, **PINNED_TOOLCHAIN, 'orig/disc.gdi': ''}) as root, \
                    host('cmp') as path:
                status, output = make(root, goal, 'CC=no-such-cc', path=path)
                self.assertNotEqual(status, 0, output)
                self.assertIn('*** no-such-cc cmp: not found; see README.md#build', output)

    def test_gcc_needs_curl_and_sha256sum_to_fetch_sh4zam_s_files(self):
        # the toolchain and SHC's releases are in place. tools/SHA256SUMS lists SH4ZAM's LICENSE, which is missing, or
        # in place with no record of its fetch
        files = {**PINNED, **PINNED_TOOLCHAIN, 'tools/SHA256SUMS': f'{HASH}  tools/sh4zam/LICENSE\n'}
        for library in ({}, {'tools/sh4zam/LICENSE': ''}):
            with self.subTest(library=library), checkout({**files, **library}) as root, \
                    host('curl', 'sha256sum') as path:
                status, output = make(root, 'gcc', 'CC=no-such-cc', path=path)
                self.assertNotEqual(status, 0, output)
                self.assertIn('*** no-such-cc sha256sum curl: not found; see README.md#build', output)


class Originals(unittest.TestCase):
    def test_gcc_stops_without_the_disc_before_any_compile(self):
        # GCC_NEEDS's toolchain is in place: a GCC that isn't there would stop make first
        with checkout({**SEARCHED, **GCC_NEEDS}) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertNotEqual(status, 0, output)
        self.assertRegex(output, r'\nMakefile:\d+: \*\*\* no disc in orig/\.  Stop\.\n\Z')
        self.assertNotIn('-fdiagnostics-plain-output', output)


class Downloads(unittest.TestCase):
    def test_fetches_the_toolchain_s_sources_only_as_tools_sha256sums_lists_them(self):
        # tools/SHA256SUMS lists none of them, so make gcc refuses the first and builds no toolchain
        with checkout({**SEARCHED, **PINNED, **ORIGINALS}) as root, host() as path:
            status, output = make(root, '-k', 'gcc', 'CC=true', path=path)
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
            status, output = make(root, '-n', 'gcc')
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
            prefix = f'--prefix={root.resolve() / "tools/sh-elf.part"}'
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
        # all-gcc and all-target-libgcc build in one make, which leaves their order to GCC's own Makefile
        made = [line for line in ran if line.startswith('make ')]
        self.assertEqual(made[:2], ['make binutils.build all', 'make binutils.build install'])
        self.assertEqual(sorted(made[2:4]), ['make gcc.build all-gcc', 'make gcc.build all-target-libgcc'])
        self.assertEqual(made[4:], ['make gcc.build install-gcc', 'make gcc.build install-target-libgcc'])

    def test_builds_the_toolchain_with_the_jobs_make_takes(self):
        # make() passes -j1 before the test's -j. Each make of the toolchain takes this make's -j and none of its
        # other flags, such as -n
        for jobs in ('-j1', '-j3'):
            with self.subTest(jobs), self.toolchain() as (root, path):
                status, output = make(root, '-n', jobs, 'tools/sh-elf/bin/sh-elf-gcc', path=path)
                self.assertEqual(status, 0, output)
                self.assertIn(f' && make {jobs} MAKEINFO=true && make install MAKEINFO=true)', output)
                self.assertIn(f' && make {jobs} all-gcc all-target-libgcc MAKEINFO=true && ', output)

    def test_fetches_each_sh4zam_file_tools_sha256sums_lists_before_any_c_compile(self):
        # tools/SHA256SUMS gives SH4ZAM's LICENSE another hash than CURL's file, or its own
        for sums, fetched in ((f'{HASH}  tools/sh4zam/LICENSE\n', False), (listing(['tools/sh4zam/LICENSE']), True)):
            with self.subTest(fetched=fetched), stood_in({'bin/curl': CURL, 'tools/SHA256SUMS': sums}) as root:
                status, output = make(root, '-k', 'gcc', 'SH_ELF=sh-elf', 'CC=true', path=path_with_bin(root))
                self.assertEqual(status == 0, fetched, output)
                self.assertIn('  fetch  tools/sh4zam/LICENSE\n', output)
                self.assertEqual('tools/sh4zam/LICENSE.part: FAILED\n' in output, not fetched, output)
                self.assertEqual('  gcc    src/main.c\n' in output, fetched, output)
                self.assertIn('  gas    modules/mod/b.src\n', output)
                # the fetch's record waits for every file
                self.assertEqual((root / 'tools/sh4zam/fetched.txt').exists(), fetched, output)

    def test_fetches_a_file_of_another_pin_again_before_any_c_compile(self):
        # LICENSE is in place from another pin, with no record of a fetch or with a record of that pin's line.
        # tools/SHA256SUMS gives it the hash of CURL's file, beside a line outside tools/sh4zam/. make gcc fetches it
        # again before it compiles, removes old.h, which that pin listed and this one doesn't, and records SH4ZAM's
        # line alone, so the next make gcc fetches nothing
        sums = listing(['tools/sh4zam/LICENSE'])
        files = {'bin/curl': CURL, 'tools/SHA256SUMS': sums + f'{HASH}  tools/other\n', 'tools/sh4zam/LICENSE': 'old',
                 'tools/sh4zam/include/old.h': 'old'}
        for record in ({}, {'tools/sh4zam/fetched.txt': f'{HASH}  tools/sh4zam/LICENSE\n'}):
            with self.subTest(record=record), stood_in({**files, **record}) as root:
                path = path_with_bin(root)
                status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true', path=path)
                self.assertEqual(status, 0, output)
                fetch = output.find('  fetch  tools/sh4zam/LICENSE\n')
                self.assertNotEqual(fetch, -1, output)
                self.assertLess(fetch, output.index('  gcc    src/main.c\n'), output)
                self.assertEqual((root / 'tools/sh4zam/LICENSE').read_bytes(), b'wheel')
                self.assertFalse((root / 'tools/sh4zam/include/old.h').exists())
                self.assertEqual((root / 'tools/sh4zam/fetched.txt').read_text(), sums)
                status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true', path=path)
                self.assertEqual(status, 0, output)
                self.assertNotIn('  fetch ', output)

    def test_fetches_every_file_again_after_a_fetch_cut_short(self):
        # pin A's files are in place, with the record of their fetch. Under pin B, LICENSE's fetch passes its line and
        # x.h's fails. Back under pin A, LICENSE holds B's bytes: make gcc fetches it again, and refuses it before any
        # compile
        pin_a = ''.join(f'{hashlib.sha256(b"A").hexdigest()}  tools/sh4zam/{path}\n' for path in ('LICENSE', 'x.h'))
        pin_b = listing(['tools/sh4zam/LICENSE']) + f'{HASH}  tools/sh4zam/x.h\n'
        files = {'bin/curl': CURL, 'tools/sh4zam/LICENSE': 'A', 'tools/sh4zam/x.h': 'A',
                 'tools/sh4zam/fetched.txt': pin_a}
        with stood_in(files) as root:
            for sums, refused in ((pin_b, 'x.h'), (pin_a, 'LICENSE')):
                (root / 'tools/SHA256SUMS').write_text(sums)
                settle(root)
                status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true', path=path_with_bin(root))
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
        # the build writes its receipt last: the sources, their lines of tools/SHA256SUMS and the configures' flags. A
        # toolchain whose receipt names another hash, or that has none, as one built before receipts, stops make
        refused = ("tools/sh-elf: its request.txt names another build than gcc.mk's; remove tools/sh-elf, and make "
                   'builds it again.  Stop.\n')
        with self.toolchain() as (root, path):
            status, output = make(root, 'tools/sh-elf/bin/sh-elf-gcc', path=path)
            self.assertEqual(status, 0, output)
            written = (root / 'tools/sh-elf/request.txt').read_text()
            for word in ('gcc/gcc-16.2.0/gcc-16.2.0.tar.xz', 'tools/sh-elf.src/gcc-16.2.0.tar.xz',
                         '--with-cpu=m4-single-only', '--disable-werror'):
                self.assertIn(word, written)
            self.assertEqual(make(root, 'tools/sh-elf/bin/sh-elf-gcc', path=path),
                             (0, "make: 'tools/sh-elf/bin/sh-elf-gcc' is up to date.\n"))
            sums = (root / 'tools/SHA256SUMS').read_text()
            (root / 'tools/SHA256SUMS').write_text(HASH + sums[len(HASH):])
            status, output = make(root, '-n', 'tools/sh-elf/bin/sh-elf-gcc', path=path)
            self.assertNotEqual(status, 0, output)
            self.assertTrue(output.endswith(refused), output)
            (root / 'tools/SHA256SUMS').write_text(sums)
            (root / 'tools/sh-elf/request.txt').unlink()
            status, output = make(root, '-n', 'tools/sh-elf/bin/sh-elf-gcc', path=path)
            self.assertNotEqual(status, 0, output)
            self.assertTrue(output.endswith(refused), output)
            self.assertTrue((root / 'tools/sh-elf/bin/sh-elf-gcc').exists())

    @contextlib.contextmanager
    def toolchain(self):
        """A checkout with stand-ins for curl and tar, and the PATH that finds them. tools/SHA256SUMS gives each of the
        toolchain's sources the hash of CURL's file. tar unpacks each package as a configure, which writes a Makefile
        whose install writes the package's tools under the prefix configure takes. tar, configure and that Makefile's
        make each add a line to ran.log, which names the directory each runs in by its last part."""
        tarballs = [f'tools/sh-elf.src/{name}' for name in ('binutils-2.47.tar.xz', 'gcc-16.2.0.tar.xz',
                                                             'gmp-6.3.0.tar.xz', 'mpc-1.3.1.tar.gz', 'mpfr-4.2.1.tar.xz')]
        with checkout({**SEARCHED, 'bin/curl': CURL, 'tools/SHA256SUMS': listing(tarballs)}) as root:
            log = root.resolve() / 'ran.log'
            made = f'\techo make $(notdir $(CURDIR)) $@ >> {log}'
            (root / 'bin/configure').write_text(
                f'#!/bin/sh\necho "configure ${{PWD##*/}} $0 $*" >> {log}\n'
                'for a; do case $a in --prefix=*) prefix=${a#--prefix=};; esac; done\n'
                f"printf '%s\\n' 'all all-gcc all-target-libgcc:' '{made}' \\\n"
                f"    'install install-gcc install-target-libgcc:' '{made}' \\\n"
                '    "\tmkdir -p $prefix/bin && touch $prefix/bin/sh-elf-as $prefix/bin/sh-elf-gcc" > Makefile\n')
            (root / 'bin/tar').write_text('#!/bin/sh\nwhile [ $# -gt 0 ]; do case $1 in -C) dir=$2; shift;; esac; shift; done\n'
                                          f'echo "tar -C $dir" >> {log}\ncp bin/configure "$dir/configure"\n')
            for script in ('bin/tar', 'bin/configure'):
                (root / script).chmod(0o755)
            yield root, path_with_bin(root)


def receipt(root):
    """The receipt make's build of the toolchain would write in root, as make -n prints its command."""
    status, output = make(root, '-n', 'tools/sh-elf/bin/sh-elf-gcc')
    return re.search(r"^printf '%s\\n' '(.*)' > tools/sh-elf\.part/request\.txt$", output, re.M)[1] + '\n'


class Gcc(unittest.TestCase):
    def test_compiles_each_c_source_and_assembles_each_src_file(self):
        with stood_in() as root:
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertEqual(output, '  gcc    src/main.c\n  gcc    modules/mod/a.c\n  gas    modules/mod/b.src\n'
                                     '  ar     build/gcc/sh4zam/libsh4zam.a\n'
                                     '  census config/1st_read/census.txt\n  import config/1st_read/imports.txt\n'
                                     '  link   build/gcc/link\n')
            for obj in ('src/main.o', 'modules/mod/a.o', 'modules/mod/b.o'):
                self.assertTrue((root / 'build/gcc' / obj).exists(), obj)
        with stood_in() as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn('python3 tools/floats.py --assembly modules/mod/b.src\n'
                      'build/shckit translate -o build/gcc/modules/mod/b.s modules/mod/b.src\n'
                      'sh-elf/bin/sh-elf-as --isa=sh4 --no-pad-sections -o build/gcc/modules/mod/b.o '
                      'build/gcc/modules/mod/b.s\n', output)
        # GCC's preprocessor writes the C, and the float pass, tools/floats.py, respells it as R32 reads it. GCC then
        # compiles it. The float pass takes SH4ZAM's include directory for system headers. The compile's log is kept,
        # and printed where GCC fails, and the .i is removed
        self.assertIn('-fdiagnostics-plain-output -MD -MP -MT build/gcc/modules/mod/a.o -MF build/gcc/modules/mod/a.d.tmp '
                      '-E modules/mod/a.c > build/gcc/modules/mod/a.i && python3 tools/floats.py --shc r32 --system '
                      'tools/sh4zam/include build/gcc/modules/mod/a.i && LC_ALL=C sh-elf/bin/sh-elf-gcc ', output)
        self.assertRegex(output, r' -fdiagnostics-plain-output -c -o build/gcc/modules/mod/a\.o build/gcc/modules/mod/a\.i\) '
                                 r'> build/gcc/modules/mod/a\.o\.log 2>&1 \|\| \(cat build/gcc/modules/mod/a\.o\.log; false\) && '
                                 r'mv build/gcc/modules/mod/a\.d\.tmp build/gcc/modules/mod/a\.d && '
                                 r'rm -f build/gcc/modules/mod/a\.i\n')

    def test_runs_gcc_in_the_c_locale(self):
        # GCC runs in the C locale, so its log reads the same on any host. GCC runs twice
        # for a C source an SHC builds, and once each for a source in gcc/ and for SH4ZAM's C. GNU as alone assembles
        # SH4ZAM's assembly
        with stood_in({**LIBRARY, 'gcc/modules/mod/b.c': ''}) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
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
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertEqual((root / 'build/gcc/modules/mod/a.o').read_text(),
                             '# 1 "modules/mod/a.c"\nfloat a = 0x1.999998p-4F, b = 0x1p-1F, c = 0x1.555554p-3F  ;\n')
            self.assertFalse((root / 'build/gcc/modules/mod/a.i').exists())

    def test_fails_an_object_whose_floating_constant_shc_can_t_read_or_whose_initializer_the_fold_can_t(self):
        # and keeps the .i GCC's preprocessor wrote for it
        for c, problem in (('float a = 1e39F;', "1e39F: 2**128 or above, where SHC warns that its conversion overflows"),
                           ('float a = sqrtf(2.0F);', 'sqrtf(0x1p+1F): a call')):
            with self.subTest(c), stood_in({'modules/mod/a.c': f'# 1 "modules/mod/a.c"\n{c}\n'}) as root:
                status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
                self.assertNotEqual(status, 0, output)
                self.assertIn(f'floats: modules/mod/a.c:1: {problem}\n', output)
                self.assertFalse((root / 'build/gcc/modules/mod/a.o').exists())
                self.assertEqual((root / 'build/gcc/modules/mod/a.i').read_text(), f'# 1 "modules/mod/a.c"\n{c}\n')

    def test_fails_a_src_file_whose_fdata_constant_asmsh_and_gnu_as_can_read_as_two_floats(self):
        with stood_in({'modules/mod/b.src': "        .FDATA.S    F'16777217.0\n"}) as root:
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertFalse((root / 'build/gcc/modules/mod/b.o').exists())
        self.assertNotEqual(status, 0, output)
        self.assertIn("floats: modules/mod/b.src:1: F'16777217.0: on a tie or past one by less than an eighth of a "
                      'step, where asmsh and GNU as can read two floats\n', output)

    def test_compiles_each_source_in_gcc_to_the_object_of_the_source_it_replaces(self):
        # gcc/ replaces b.src. GCC compiles the replacement searching the SDK's headers, the game's and the root
        files = {'gcc/modules/mod/b.c': 'own b\n'}
        with stood_in(files) as root:
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            compiled = sorted(line for line in output.splitlines() if line.startswith('  gcc '))
            self.assertEqual(compiled, ['  gcc    gcc/modules/mod/b.c', '  gcc    modules/mod/a.c', '  gcc    src/main.c'])
            # the stand-in GCC copies the source to the object
            self.assertEqual((root / 'build/gcc/modules/mod/b.o').read_text(), 'own b\n')
        with stood_in(files) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        # no SHC builds a source in gcc/, so the float pass reads none of it, and GCC compiles it in one run
        self.assertIn(' -Isdk/include -Isrc -I. -fdiagnostics-plain-output -MD -MP -MT build/gcc/modules/mod/b.o '
                      '-MF build/gcc/modules/mod/b.d.tmp -c -o build/gcc/modules/mod/b.o gcc/modules/mod/b.c) > ', output)
        self.assertNotIn('> build/gcc/modules/mod/b.i', output)

    def test_leaves_gcc_s_sources_to_make_gcc_and_lint(self):
        # asmsh still assembles the .src file a replacement stands for, but no SHC route, progress or review names a
        # source in gcc/
        files = {'gcc/modules/mod/b.c': '', **PINNED}
        for goal in ('all', 'check', 'progress', 'review'):
            with self.subTest(goal), stood_in(files) as root:
                status, output = make(root, '-n', goal)
                self.assertEqual(status, 0, output)
                self.assertNotIn('gcc/', output)
                if goal in ('all', 'check'):
                    self.assertIn(r"asmsh.exe 'modules\mod\b.src'", output)

    def test_stops_on_a_source_in_gcc_that_replaces_no_src_file(self):
        # other.c stands for no source, and main.c for a C source
        for own in ('gcc/src/other.c', 'gcc/src/main.c'):
            with self.subTest(own), stood_in({own: ''}) as root:
                status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
                self.assertNotEqual(status, 0, output)
                self.assertIn(f"{own}: replaces no .src file, and GCC_ADDED doesn't name it.  Stop.", output)
                # only make gcc's goals check it
                status, output = make(root, '-n', 'check')
                self.assertEqual(status, 0, output)

    def test_compiles_each_source_gcc_added_names_to_its_own_object_which_its_image_links(self):
        # each object lies at its source's path. tools/spec.py takes it as one of its image's own, and the census
        # reads its source as its image's
        files = {'gcc/src/other.c': 'own other\n', 'gcc/modules/mod/more.c': 'own more\n',
                 'config/gcc.mk': f'{GCC_MK}GCC_ADDED := 1st_read:gcc/src/other.c mod:gcc/modules/mod/more.c\n'}
        with stood_in(files) as root:
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertIn('  gcc    gcc/src/other.c\n', output)
            self.assertEqual((root / 'build/gcc/src/other.o').read_text(), 'own other\n')
            self.assertEqual((root / 'build/gcc/modules/mod/more.o').read_text(), 'own more\n')
        with stood_in(files) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn(' --add 1st_read build/gcc/src/other.o --add mod build/gcc/modules/mod/more.o --list ', output)
        for tool in ('census.py --gcc', 'imports.py'):
            self.assertIn(f'\npython3 tools/{tool} --add 1st_read gcc/src/other.c --add mod gcc/modules/mod/more.c\n',
                          output)

    def test_stops_on_an_entry_of_gcc_added_that_names_no_source_of_gcc_s_own(self):
        # an entry with no image, one for a source outside gcc/, two whose objects are a source's, and one whose
        # object is SH4ZAM's
        unnamed = ': an entry of GCC_ADDED, and not IMAGE:SOURCE, SOURCE a C source in gcc/'
        compiled = ": GCC_ADDED names it, and it compiles to a source's object"
        for entry, problem in (('gcc/src/other.c', f'gcc/src/other.c{unnamed}'),
                               ('1st_read:src/main.c', f'1st_read:src/main.c{unnamed}'),
                               ('1st_read:gcc/src/main.c', f'gcc/src/main.c{compiled}'),
                               ('mod:gcc/modules/mod/b.c', f'gcc/modules/mod/b.c{compiled}'),
                               ('1st_read:gcc/sh4zam/sh4/a.c', "build/gcc/sh4zam/sh4/a.o: two of make gcc's rules "
                                                                'build it, by GCC_PORT_OBJ GCC_SH4ZAM_OBJ')):
            files = {**LIBRARY, 'config/gcc.mk': f'{GCC_MK}GCC_ADDED := {entry}\n'}
            if entry.split(':')[-1].startswith('gcc/'):
                files[entry.split(':')[-1]] = ''
            with self.subTest(entry), stood_in(files) as root:
                status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
                self.assertNotEqual(status, 0, output)
                self.assertIn(f'{problem}.  Stop.', output)
                # only make gcc's goals read GCC_ADDED
                status, output = make(root, '-n', 'check')
                self.assertEqual(status, 0, output)

    def test_builds_an_object_again_by_its_new_rule_once_its_source_leaves_gcc_or_joins_it(self):
        # each of b.o's rules names its own source alone, which settle() dates before the object. make -n removes no
        # object
        files = {'gcc/modules/mod/b.c': 'own b\n'}
        with self.built(files) as root:
            (root / 'gcc/modules/mod/b.c').unlink()
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
            self.assertEqual(status, 0, output)
            self.assertEqual((root / 'build/gcc/modules/mod/b.o').read_text(), 'own b\n')
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertIn('  gas    modules/mod/b.src\n', output)
            self.assertEqual((root / 'build/gcc/modules/mod/b.o').read_text(), '')
            (root / 'gcc/modules/mod/b.c').write_text('own b\n')
            settle(root)
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertIn('  gcc    gcc/modules/mod/b.c\n', output)
            self.assertEqual((root / 'build/gcc/modules/mod/b.o').read_text(), 'own b\n')

    def test_forgets_a_port_object_s_old_source_once_build_gcc_port_txt_is_gone(self):
        # a build with no build/gcc/port.txt, whose object of gcc/'s has a .d file that names its source where it lay.
        # make removes the .d file, and the object builds again from its source where it lies
        files = {'gcc/modules/mod/b.c': 'own b\n'}
        with self.built(files) as root:
            (root / 'build/gcc/modules/mod/b.d').write_text('build/gcc/modules/mod/b.o: old/modules/mod/b.c\n')
            (root / 'build/gcc/port.txt').unlink()
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertIn('  gcc    gcc/modules/mod/b.c\n', output)

    def test_compiles_sh4zam_s_sh_4_back_end_alone_and_archives_it(self):
        # AR keeps a member until the archive goes. The rule removes the archive before each write, and the inventory
        # of its members writes it again, so another pin, which drops a.c and leaves b.s as it was, archives b.o alone
        with stood_in({**LIBRARY, 'sh-elf/bin/sh-elf-ar': AR}) as root:
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertIn('  gcc    tools/sh4zam/source/sh4/a.c\n  gas    tools/sh4zam/source/sh4/b.s\n', output)
            self.assertIn('  ar     build/gcc/sh4zam/libsh4zam.a\n', output)
            archive = root / 'build/gcc/sh4zam/libsh4zam.a'
            self.assertEqual(archive.read_text(), 'build/gcc/sh4zam/sh4/a.o\nbuild/gcc/sh4zam/sh4/b.o\n')
            (root / 'tools/sh4zam/source/sh4/a.c').unlink()
            sums = ''.join(line for line in LIBRARY['tools/SHA256SUMS'].splitlines(True)
                           if 'source/sh4/a.c' not in line)
            for path in ('tools/SHA256SUMS', 'tools/sh4zam/fetched.txt'):
                (root / path).write_text(sums)
            settle(root)
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertEqual(archive.read_text(), 'build/gcc/sh4zam/sh4/b.o\n')
        with stood_in(LIBRARY) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertNotIn('source/sw/', output)
        self.assertNotIn('shz_version', output)
        self.assertNotIn('d.inc', output)
        # the C compiles in one run, with headers from sdk/include, and the float pass reads none of it
        a = compiles(output)['build/gcc/sh4zam/sh4/a.o']
        self.assertEqual(a[a.index('-MF') + 1:a.index('-MF') + 5], ['build/gcc/sh4zam/sh4/a.d.tmp', '-c', '-o',
                                                                     'build/gcc/sh4zam/sh4/a.o'])
        self.assertEqual(a[a.index('-MF') + 5], 'tools/sh4zam/source/sh4/a.c)')
        self.assertIn('-Isdk/include', a)
        self.assertNotIn('-E', a)
        self.assertNotIn('tools/floats.py', a)
        # GNU as assembles the assembly as it is, little-endian under GAS_CHECKS. GASFLAGS' --no-pad-sections would
        # drop each section's padding
        self.assertIn(' && sh-elf/bin/sh-elf-as --isa=sh4 --little -o build/gcc/sh4zam/sh4/b.o '
                      'tools/sh4zam/source/sh4/b.s\n', output)
        self.assertIn('sh-elf/bin/sh-elf-ar rcsD build/gcc/sh4zam/libsh4zam.a build/gcc/sh4zam/sh4/a.o '
                      'build/gcc/sh4zam/sh4/b.o\n', output)

    def test_compiles_sh4zam_s_c_under_the_shared_sets_alone_and_the_tree_s_under_gccflags(self):
        # under the repository's flags, SH4ZAM's C takes each flag of the shared sets, which every C compile takes. It
        # takes none of the tree's sets, which compile its C and lay it out as SHC does: SHC never compiled the
        # library. It keeps the zeros an image's file holds, and its types keep the layout their headers declare. The
        # test's config/gcc.mk takes the repository's line that gives SH4ZAM's objects their flags
        shared, tree = ('GCC_TARGET', 'GCC_ZEROS_IN_DATA', 'GCC_WARNINGS'), ('GCC_AS_SHC', 'GCC_LAYOUT')
        sets = {name: test_ld.flags(name) for name in (*shared, *tree)}
        library = f'GCC_SH4ZAM_FLAGS := {shlex.join(test_ld.flags("GCC_SH4ZAM_FLAGS"))}\n' + ''.join(
            line for line in (ROOT / 'config/gcc.mk').read_text().splitlines(True)
            if line.startswith('build/gcc/sh4zam/'))
        with stood_in({**LIBRARY, 'config/gcc.mk': GCC_MK + gccflags() + library}) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        compiled = compiles(output)
        a = compiled['build/gcc/sh4zam/sh4/a.o']
        self.assertEqual([flag for flag in test_ld.flags() if flag not in compiled['build/gcc/src/main.o']], [])
        self.assertEqual([flag for name in shared for flag in sets[name] if flag not in a], [])
        self.assertEqual([flag for name in tree for flag in sets[name] if flag in a], [])
        self.assertIn('-fno-zero-initialized-in-bss', a)
        self.assertNotIn('-fpack-struct=4', a)

    def test_compiles_an_sh4zam_object_again_whose_header_changes_or_whose_d_file_is_gone(self):
        compile_a = '-c -o build/gcc/sh4zam/sh4/a.o tools/sh4zam/source/sh4/a.c)'
        with self.built({**LIBRARY, 'sdk/include/string.h': ''}) as root:
            (root / 'build/gcc/sh4zam/sh4/a.d').write_text(
                depends('build/gcc/sh4zam/sh4/a.o', 'tools/sh4zam/source/sh4/a.c', 'sdk/include/string.h'))
            settle(root)
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
            self.assertNotIn(compile_a, output)
            os.utime(root / 'sdk/include/string.h')
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
            self.assertEqual(status, 0, output)
            self.assertIn(compile_a, output)
            self.assertIn('sh-elf/bin/sh-elf-ar rcsD build/gcc/sh4zam/libsh4zam.a ', output)
            settle(root)
            (root / 'build/gcc/sh4zam/sh4/a.d').unlink()
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn(compile_a, output)

    def test_archives_sh4zam_s_objects_again_with_another_archiver_and_compiles_none(self):
        with self.built(LIBRARY) as root:
            with open(root / 'sh-elf/bin/sh-elf-ar', 'a') as file:
                file.write('\n# another\n')
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn('sh-elf/bin/sh-elf-ar rcsD build/gcc/sh4zam/libsh4zam.a ', output)
        self.assertNotIn('sh-elf/bin/sh-elf-gcc', output)

    def test_links_every_image_once_its_objects_build_and_writes_the_manifest_last(self):
        # the link first removes the manifest and the disc. tools/spec.py writes the spec, tools/ld.py links by it, and
        # tools/verify.py checks what it links, the declarations under GCC_ABI. tools/publish.py writes the files, and
        # the manifest last. Each runs with the toolchain SH_ELF names
        with stood_in({'config/gcc.mk': f'{GCC_MK}GCC_ABI := -ml -m4-single-only\n'}) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        lines = output.splitlines()
        steps = ['rm -rf build/gcc/manifest.txt build/gcc/disc build/gcc/disc.partial', 'mkdir -p build/gcc/link',
                 'SH_ELF=sh-elf python3 tools/spec.py --archive build/gcc/sh4zam/libsh4zam.a --list '
                 'build/gcc/link/unbound.txt build/gcc/link/spec.txt',
                 'SH_ELF=sh-elf python3 tools/ld.py build/gcc/link/spec.txt build/gcc/link',
                 "SH_ELF=sh-elf python3 tools/verify.py --abi='-ml -m4-single-only' build/gcc/link/spec.txt "
                 'build/gcc/link',
                 'SH_ELF=sh-elf python3 tools/publish.py build/gcc/link/spec.txt build/gcc/link '
                 'build/gcc/manifest.txt']
        at = lines.index(steps[0])
        self.assertEqual(lines[at:at + len(steps)], steps)
        self.assertLess(max(index for index, line in enumerate(lines) if 'sh-elf/bin/sh-elf-gcc' in line), at)

    def test_links_again_once_a_source_the_memory_contract_reads_changes_and_stops_once_one_goes(self):
        # MEMORY_SOURCES is tools/memory.py's SOURCES. The manifest depends on each, so a missing one stops make even
        # where the manifest is current
        sources = ['src/system/loader.h', 'src/progress/recorder.h']
        with stood_in({'tools/memory.py': memory_of(*sources), **dict.fromkeys(sources, '')}) as root:
            status, output = make(root, '-pqn', 'gcc', 'SH_ELF=sh-elf')
            self.assertIn(status, (0, 1), output[-3000:])
            self.assertEqual(re.search(r'(?m)^MEMORY_SOURCES := (.*)$', output)[1].split(), sources)
            rule = re.search(r'(?m)^build/gcc/manifest\.txt: (.*)$', output)[1].split()
            self.assertEqual([word for word in rule if word in sources], sources)
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            (root / sources[1]).unlink()
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
        self.assertNotEqual(status, 0, output)
        self.assertIn(f"No rule to make target '{sources[1]}', needed by 'build/gcc/manifest.txt'", output)

    def test_links_again_where_a_final_elf_or_a_file_the_manifest_owns_goes_and_nowhere_else(self):
        # LINK's publisher writes each image's final ELF and files, then the manifest
        with self.built() as root:
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertNotIn('link   build/gcc/link', output)
            for gone in ('build/gcc/link/mod/final.elf', 'build/gcc/SONICADV/MOD.PRS', 'build/gcc/SONICADV/MOD.BIN'):
                with self.subTest(gone):
                    (root / gone).unlink()
                    status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
                    self.assertEqual(status, 0, output)
                    self.assertIn('link   build/gcc/link', output)
                    self.assertTrue((root / gone).exists())

    def test_a_link_that_fails_leaves_no_manifest(self):
        # a publisher that fails, after an earlier run wrote a manifest
        with self.built() as root:
            self.assertTrue((root / 'build/gcc/manifest.txt').exists())
            (root / 'tools/publish.py').write_text('raise SystemExit(1)\n')
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertNotEqual(status, 0, output)
            self.assertFalse((root / 'build/gcc/manifest.txt').exists())

    def test_builds_a_disc_from_the_manifest_of_the_link(self):
        # make gcc-disc runs the census and the import check, links, and builds the disc from the manifest
        files = {'orig/disc.gdi': '', 'build/gdi': ''}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'gcc-disc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        lines = output.splitlines()
        published = lines.index('SH_ELF=sh-elf python3 tools/publish.py build/gcc/link/spec.txt build/gcc/link '
                                'build/gcc/manifest.txt')
        self.assertEqual(lines[-3:], ["printf '  %-6s %s\\n' 'disc' 'build/gcc/disc'",
                                      'rm -rf build/gcc/disc build/gcc/disc.partial',
                                      "python3 tools/disc.py 'orig/disc.gdi' build/gcc/manifest.txt build/gcc/disc"])
        checks = [next(at for at, line in enumerate(lines) if check in line)
                  for check in ('tools/census.py --generated --disc', 'tools/census.py --gcc', 'python3 tools/imports.py')]
        self.assertLess(max(checks), lines.index('rm -rf build/gcc/manifest.txt build/gcc/disc build/gcc/disc.partial'))
        self.assertLess(published, len(lines) - 3)

    def test_fails_on_the_census_or_the_import_check_before_the_link_of_a_disc(self):
        # the census and the import check are prerequisites of the manifest, not goals beside it. One that fails stops
        # make gcc-disc before the link, which leaves no manifest and so no disc
        disc = 'import pathlib, sys\npathlib.Path(sys.argv[-1]).mkdir()\n'
        for path, text in (('tools/census.py', "raise SystemExit('census: a number for RAM')\n"),
                           ('tools/imports.py', "raise SystemExit('imports: a use no stage holds')\n")):
            files = {'orig/disc.gdi': '', 'build/gdi': '', 'tools/disc.py': disc, path: text}
            with self.subTest(path), stood_in(files) as root:
                status, output = make(root, 'gcc-disc', 'SH_ELF=sh-elf', 'CC=true')
                self.assertNotEqual(status, 0, output)
                self.assertNotIn('link   build/gcc/link', output)
                self.assertFalse((root / 'build/gcc/manifest.txt').exists())
                self.assertFalse((root / 'build/gcc/disc').exists())

    def test_a_link_that_fails_after_a_good_one_leaves_no_disc(self):
        # a stand-in tools/disc.py writes a disc from a good link's manifest, and then tools/spec.py fails
        files = {'orig/disc.gdi': '', 'build/gdi': '',
                 'tools/disc.py': "import pathlib, sys\npathlib.Path(sys.argv[-1]).mkdir()\n"
                                  "pathlib.Path(sys.argv[-1], 'disc.txt').write_text('a disc\\n')\n"}
        with stood_in(files) as root:
            status, output = make(root, 'gcc-disc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertTrue((root / 'build/gcc/disc/disc.txt').exists())
            settle(root)
            (root / 'tools/spec.py').write_text('raise SystemExit(1)\n')
            status, output = make(root, 'gcc-disc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertNotEqual(status, 0, output)
            self.assertFalse((root / 'build/gcc/manifest.txt').exists())
            self.assertFalse((root / 'build/gcc/disc').exists())

    def test_builds_a_disc_from_the_disc_s_gdi_alone(self):
        with stood_in() as root:
            status, output = make(root, '-n', 'gcc-disc', 'SH_ELF=sh-elf')
        self.assertNotEqual(status, 0, output)
        self.assertRegex(output, r"gcc\.mk:\d+: \*\*\* gcc-disc needs the disc's \.gdi and its tracks in orig/, and "
                                 r'orig/ holds 0 \.gdi files\.  Stop\.\n\Z')

    def test_compiles_each_image_s_gcc_side_before_the_census_reads_it(self):
        # census.py --gcc reads every image's GCC side, 1ST_READ.BIN's main.c and mod's a.c among them, so made alone,
        # the check compiles both objects first
        with stood_in() as root:
            status, output = make(root, '-n', 'SH_ELF=sh-elf', 'build/gcc/census.ok')
        self.assertEqual(status, 0, output)
        lines = output.splitlines()
        for obj in ('build/gcc/src/main.o', 'build/gcc/modules/mod/a.o'):
            compile_ = next(at for at, line in enumerate(lines) if f' -o {obj} ' in line)
            self.assertLess(compile_, lines.index('python3 tools/census.py --gcc'), output)

    def test_holds_1st_read_bin_s_shc_side_and_the_motion_files_to_the_census_before_its_gcc_side(self):
        # the census check waits for 1ST_READ.BIN's SHC objects, whose .d files name what each compile read, and for
        # each decoded motion file. It checks them, then the GCC side, and all before the link
        files = {**MOTION, 'orig/SONICADV/X_MOT.PRS': ''}
        with stood_in(files) as root:
            (root / 'build/src/main.obj').unlink()
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        lines = output.splitlines()
        steps = [next(at for at, line in enumerate(lines) if step in line)
                 for step in ('shc.exe src/main.c -objectfile=build/src/main.obj',
                              'prskit decompress orig/SONICADV/X_MOT.PRS', 'python3 tools/census.py --generated --disc',
                              'python3 tools/census.py --gcc', 'rm -rf build/gcc/manifest.txt ')]
        self.assertLess(max(steps[:2]), steps[2], output)
        self.assertEqual(sorted(steps[2:]), steps[2:], output)

    def test_holds_1st_read_bin_s_imports_to_the_shc_objects_that_define_them_before_the_link(self):
        # 1ST_READ.BIN takes a_init from mod, whose a.c defines it. The import check waits for a.c's SHC object, or
        # its stub's where a.c has bodies, as make check builds it. make gcc compiles neither for its own C
        full = 'shc.exe modules/mod/a.c -objectfile=build/modules/mod/a.obj'
        for case, source, reads, skips in (('object', '', full, STUB_SHC),
                                           ('stub', '#include "mod/a/a.inc"\n', STUB_SHC, full)):
            files = {'config/1st_read/imports.txt': 'a_init  mod\n', 'modules/mod/a.c': source}
            with self.subTest(case), stood_in(files) as root:
                status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                lines = output.splitlines()
                built = next(at for at, line in enumerate(lines) if reads in line)
                check = lines.index('python3 tools/imports.py')
                link = lines.index('rm -rf build/gcc/manifest.txt build/gcc/disc build/gcc/disc.partial')
                self.assertLess(built, check, output)
                self.assertLess(check, link, output)
                self.assertNotIn(skips, output)

    def test_holds_1st_read_bin_s_imports_to_its_gcc_objects(self):
        # the check reads 1ST_READ.BIN's C as GCC compiles it too. Made alone, it compiles main.c's object and
        # 1ST_READ.BIN's content, and no object of mod's
        gen = 'gen/1st_read/d_8c1755e0.c  D  0x8c1755e0  0x8c1755f0\n'
        with stood_in({'config/1st_read/splits.txt': MODULE['config/1st_read/splits.txt'] + gen,
                       'config/1st_read/data.txt': '0x8c1755e0  words[4]\n'}) as root:
            status, output = make(root, '-n', 'SH_ELF=sh-elf', 'build/gcc/imports.ok')
        self.assertEqual(status, 0, output)
        self.assertEqual(set(compiles(output)), {'build/gcc/src/main.o', 'build/gcc/gen/1st_read/content.o'})

    def test_writes_the_import_check_s_dependencies_for_make_gcc_s_goals_alone(self):
        # make check's build/deps.mk holds none of make gcc's rules. make gcc's goals have tools/deps.py --gcc write the
        # import check's to build/gcc/deps.mk
        with stood_in({'config/1st_read/imports.txt': 'a_init  mod\n'}) as root:
            status, output = make(root, '-n', 'check')
            self.assertEqual(status, 0, output)
            self.assertNotIn('build/gcc/', (root / 'build/deps.mk').read_text())
            self.assertFalse((root / 'build/gcc/deps.mk').exists())
            status, output = make(root, '-n', 'build/gcc/src/main.o', 'SH_ELF=sh-elf')
            self.assertEqual(status, 0, output)
            self.assertEqual((root / 'build/gcc/deps.mk').read_text(),
                             'build/gcc/imports.ok: build/modules/mod/a.obj\n')

    def test_writes_the_content_and_runs_each_check_and_the_link_again_once_a_module_a_tool_imports_changes(self):
        # each check depends on its tool and on each module of tools/ the tool imports, directly or through another.
        # The link depends on the tools of its stages and their modules alike, and the content of mod, whose gen/
        # range has it written, on tools/content.py and its modules
        files = {'config/mod/splits.txt': MODULE['config/mod/splits.txt'] + 'gen/mod/d_0c900180.c  D  0x0c900180  0x0c900200\n',
                 **MOD_DATA}
        with stood_in(files) as root:
            status, output = make(root, '-n', '-p', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output[-3000:])
        rules = dict(line.split(':', 1) for line in output.splitlines()
                     if line.startswith(('build/gcc/census.ok:', 'build/gcc/imports.ok:', 'build/gcc/manifest.txt:',
                                         'build/gen/mod/content.c:')))
        for check, tools in (('build/gcc/census.ok', ['census']), ('build/gcc/imports.ok', ['imports']),
                             ('build/gcc/manifest.txt', ['spec', 'ld', 'verify', 'publish']),
                             ('build/gen/mod/content.c', ['content'])):
            with self.subTest(check):
                self.assertEqual(sorted(imported(tools) - set(rules[check].split())), [])

    def test_checks_and_splits_a_source_again_once_a_tool_its_rule_runs_imports_changes(self):
        # b.src's object, and that of main.c, which tools/inline.py splits, depend on each tool their rules run and
        # each module of tools/ the tool imports, as the census does
        files = {'config/compilers.mk': FOUND, 'src/main.c': inline()}
        with stood_in(files) as root:
            status, output = make(root, '-n', '-p', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output[-3000:])
        rules = dict(line.split(':', 1) for line in output.splitlines()
                     if line.startswith(('build/gcc/modules/mod/b.o:', 'build/gcc/src/main.o:')))
        checks = ('tags', 'floats', 'config', 'csource', 'layout')
        for obj, tools in (('build/gcc/modules/mod/b.o', checks), ('build/gcc/src/main.o', ('inline',) + checks)):
            with self.subTest(obj):
                for tool in tools:
                    self.assertIn(f'tools/{tool}.py', rules[obj].split())

    def test_runs_the_checks_and_the_link_again_once_a_table_goes_and_stops_once_census_txt_goes(self):
        # build/tables.txt lists the tables, so a deletion reruns what reads them, as an edit does; the census can't
        # run without census.txt, which it names
        with self.built({'config/1st_read/uses.txt': ''}) as root:
            (root / 'config/1st_read/uses.txt').unlink()
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
            self.assertEqual(status, 0, output)
            for check in ('python3 tools/census.py --gcc', 'python3 tools/imports.py', 'python3 tools/publish.py '):
                self.assertIn(check, output)
            (root / 'config/1st_read/census.txt').unlink()
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertNotEqual(status, 0, output)
        self.assertIn("No rule to make target 'config/1st_read/census.txt', needed by 'build/gcc/census.ok'", output)

    def test_compiles_each_c_object_again_once_a_header_joins_roots_or_gcc(self):
        # build/headers.txt holds the headers under ROOTS, and build/gcc/headers.txt gcc/'s, any of which a search
        # could find first
        for header in ('modules/mod/b.h', 'gcc/modules/mod/b.h'):
            with self.subTest(header), self.built() as root:
                (root / header).parent.mkdir(parents=True, exist_ok=True)
                (root / header).write_text('')
                status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                for obj in ('src/main.o', 'modules/mod/a.o'):
                    self.assertIn(f' -c -o build/gcc/{obj} ', output)

    def test_reads_optimize_0_of_a_source_s_shc_flags_alone(self):
        files = {'config/compilers.mk': COMPILERS + 'build/gcc/modules/mod/a.o: GAME += -optimize=0 -noinline\n'}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        compiled = compiles(output)
        self.assertIn('-O0', compiled['build/gcc/modules/mod/a.o'])
        self.assertNotIn('-O0', compiled['build/gcc/src/main.o'])
        self.assertNotIn('-noinline', compiled['build/gcc/modules/mod/a.o'])

    def test_gives_the_flags_own_gives_a_source_to_its_gcc_object(self):
        # config/compilers.mk's own(), called as it is for src/system/binary.c's -optimize=0
        own = ''.join(line for line in (ROOT / 'config/compilers.mk').read_text().splitlines(True)
                      if line.startswith('own ='))
        self.assertTrue(own)
        with stood_in({'config/compilers.mk': COMPILERS + own + '$(call own,modules/mod/a,-optimize=0)\n'}) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        compiled = compiles(output)
        self.assertIn('-O0', compiled['build/gcc/modules/mod/a.o'])
        self.assertNotIn('-O0', compiled['build/gcc/src/main.o'])

    def test_names_the_gcc_object_beside_each_source_given_a_flag_make_gcc_reads(self):
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
                    self.assertIn(f'build/gcc/{obj[len("build/"):-len(".obj")]}.o', targets, targets)

    def test_reads_each_source_s_floats_as_its_shc_release_does_under_its_flags(self):
        # each route's rule passes the float pass the release that builds the source, and the -fpu=single,
        # -double=float or -aggressive=2 its flags give. Each release builds a source here with a float flag and one
        # without, and a.c takes -aggressive=2 too. Each line that gives a source a flag names its GCC object too, as
        # config/compilers.mk's lines do. p.c and q.c are patched, and g.c is grafted with o.c as its donor, from which
        # GCC compiles no object. Of those, all but p.c hold inline assembly, as i.c does
        double = ' '.join(f'build/sdk/ninja/{name}.obj build/gcc/sdk/ninja/{name}.o' for name in 'pqig')
        own = ('build/modules/mod/a.obj build/gcc/modules/mod/a.o: GAME += -aggressive=2',
               'R26_C := sdk/shinobi/ov.c sdk/shinobi/ow.c',
               'build/sdk/ninja/f.obj build/gcc/sdk/ninja/f.o: SDK += -double=float -size',
               'build/sdk/shinobi/ov.obj build/gcc/sdk/shinobi/ov.o: SDK += -fpu=single',
               'RUNTIME_C := sdk/runtime/r.c sdk/runtime/w.c', 'RUNTIME := -cpu=sh4',
               'build/sdk/runtime/r.obj build/gcc/sdk/runtime/r.o: RUNTIME += -fpu=single',
               'R32_C := sdk/shinobi/gd.c sdk/sofdec/adx.c',
               'build/sdk/shinobi/gd.obj build/gcc/sdk/shinobi/gd.o: SDK += -fpu=single',
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
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        routes = re.findall(r'python3 tools/floats\.py (.*?) --system tools/sh4zam/include build/gcc/(\S+)\.i ', output)
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
            obj = f'build/gcc/{source[:-2]}.o'
            own = ''.join(line for line in (ROOT / 'config/gcc.mk').read_text().splitlines(True)
                          if line.startswith(f'{obj}: private GCCFLAGS += '))
            self.assertTrue(own, obj)
            # no route compiles a source of LOADER.BIN's SDK unless LOADER_SDK names it
            loader = f'LOADER_SDK := {source}\n' if source.startswith('loader/sdk/') else ''
            rows = MODULE['config/mod/splits.txt'] + f'{source}  P  0x0c900040  0x0c900050\n'
            files = {'config/compilers.mk': COMPILERS + loader, 'config/gcc.mk': GCC_MK + own, source: '',
                     'config/mod/splits.txt': rows}
            with self.subTest(source), stood_in(files) as root:
                status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                compiled = compiles(output)
                self.assertIn('-fdata-sections', compiled[obj])
                self.assertNotIn('-fdata-sections', compiled['build/gcc/modules/mod/a.o'])

    def test_splits_a_source_with_inline_assembly_from_its_c_and_compiles_the_c(self):
        files = {'config/compilers.mk': FOUND, 'src/main.c': inline()}
        with stood_in(files) as root:
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertEqual(output, '  gcc    modules/mod/a.c\n  gcc    src/main.c (split)\n  gas    modules/mod/b.src\n'
                                     '  ar     build/gcc/sh4zam/libsh4zam.a\n'
                                     '  census config/1st_read/census.txt\n  import config/1st_read/imports.txt\n'
                                     '  link   build/gcc/link\n')
            # the stand-in GCC copies the C to the object, and the translator the text
            self.assertIn('__asm__(".include \\"build/gcc/src/main_inline.s\\""', (root / 'build/gcc/src/main.o').read_text())
            self.assertEqual((root / 'build/gcc/src/main_inline.s').read_text(),
                             '; src/main.c:2: main\n        .EXPORT     _main\n_main:\n        RTS\n        NOP\n'
                             '        .ALIGN      4\n        RTS\n        NOP\n')
        with stood_in(files) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn('python3 tools/inline.py src/main.c build/gcc/src/main.c build/gcc/src/main_inline.src && '
                      'python3 tools/floats.py --assembly build/gcc/src/main_inline.src && '
                      'if [ -s build/gcc/src/main_inline.src ]; then build/shckit translate -o build/gcc/src/main_inline.s '
                      'build/gcc/src/main_inline.src; else rm -f build/gcc/src/main_inline.s; fi\n', output)
        self.assertIn(' -iquote src -Isdk/include -Isrc -fdiagnostics-plain-output -MD -MP -MT build/gcc/src/main.o '
                      '-MF build/gcc/src/main.d.tmp -E build/gcc/src/main.c > build/gcc/src/main.i && ', output)

    def test_splits_a_source_whose_assembly_a_gcc_guard_leaves_to_shc_and_translates_no_text(self):
        # GCC's side defines main in C. The split leaves an empty text, which shckit refuses, so it translates none
        guarded = '#ifdef GCC\nvoid main(void)\n{\n}\n#else\n' + inline() + '#endif\n'
        with stood_in({'config/compilers.mk': FOUND, 'src/main.c': guarded}) as root:
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertIn('  gcc    src/main.c (split)\n', output)

    def test_translates_no_text_where_a_guard_keeps_each_block_of_a_source_from_gcc(self):
        # the source's one block lies under #ifndef GCC, so its text is empty. shckit refuses an empty text, as its
        # stand-in here does a split's. The split translates no text, and the C includes none
        block = ('#ifndef GCC\n/* @asm open: 8 bytes reserved */\n#pragma asm\n        .RES.B      8\n        ;\n'
                 '#pragma endasm\n#endif\n')
        files = {'config/compilers.mk': FOUND, 'src/main.c': 'void main(void)\n{\n}\n\n' + block,
                 'build/shckit': ('#!/bin/sh\n[ "$1" = translate ] && [ "$2" = -o ] || exit 1\n'
                                  'case $4 in *_inline.src) [ -s "$4" ] || exit 1 ;; esac\ncp "$4" "$3"\n')}
        with stood_in(files) as root:
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            self.assertEqual((root / 'build/gcc/src/main_inline.src').read_text(), '')
            self.assertFalse((root / 'build/gcc/src/main_inline.s').exists())
            self.assertNotIn('__asm__', (root / 'build/gcc/src/main.o').read_text())

    def test_compiles_a_split_source_with_each_function_in_a_section_of_its_own(self):
        # tools/inline.py lets a body branch to one of its file's C functions. The branch lands right only where the
        # function starts a section of its own
        files = {'config/compilers.mk': FOUND, 'config/gcc.mk': GCC_MK + gccflags(), 'src/main.c': inline()}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        compiled = compiles(output)['build/gcc/src/main.o']
        self.assertEqual([flag for flag in test_ld.flags() if flag not in compiled], [])
        self.assertIn('-ffunction-sections', compiled)

    def test_orders_each_object_as_gcc_will_but_a_content(self):
        # of make gcc's compiles, only a content's keeps its source's order: GCC orders any other's definitions as
        # it will
        with stood_in({'config/gcc.mk': GCC_MK + gccflags()}) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertNotIn('-fno-toplevel-reorder', compiles(output)['build/gcc/src/main.o'])

    def test_fuses_a_product_into_a_sum_where_the_source_s_release_does(self):
        # R32 and R26, the releases of main.c and ov.c, fuse a product into the sum that takes it, as -ffp-contract=on
        # does. R10, d.c's release, never fuses: njDistancePL2PL's @bug arm gives the image's infinity or NaN only
        # while each product rounds
        rows = ''.join(f'{source}  P  0x0c9000{at}  0x0c9000{at + 8}\n'
                       for source, at in (('sdk/ninja/d.c', 40), ('sdk/shinobi/ov.c', 50)))
        files = {'config/compilers.mk': FOUND + 'R26_C := sdk/shinobi/ov.c\n', 'config/gcc.mk': GCC_MK + gccflags(),
                 'sdk/ninja/d.c': '',
                 'sdk/shinobi/ov.c': '', 'config/mod/splits.txt': MODULE['config/mod/splits.txt'] + rows}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        compiled = compiles(output)
        contracts = {obj: [word for word in compiled[obj] if word.startswith('-ffp-contract=')][-1]
                     for obj in ('build/gcc/src/main.o', 'build/gcc/sdk/shinobi/ov.o', 'build/gcc/sdk/ninja/d.o')}
        self.assertEqual(contracts, {'build/gcc/src/main.o': '-ffp-contract=on',
                                     'build/gcc/sdk/shinobi/ov.o': '-ffp-contract=on',
                                     'build/gcc/sdk/ninja/d.o': '-ffp-contract=off'})

    def test_splits_the_grafted_source_and_builds_no_object_of_its_donor(self):
        # each holds an inline body. Only SHC's object of the grafted source holds the donor's functions
        files = {'config/compilers.mk': (COMPILERS + 'GRAFTED_C := sdk/ninja/g.c\nDONOR_C := sdk/ninja/d.c\n' +
                                         FOUND[len(COMPILERS):]),
                 'config/mod/splits.txt': MODULE['config/mod/splits.txt'] + 'sdk/ninja/g.c  P  0x0c900040  0x0c900050\n',
                 'sdk/ninja/g.c': inline('g'), 'sdk/ninja/d.c': inline('d')}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn('python3 tools/inline.py sdk/ninja/g.c build/gcc/sdk/ninja/g.c ', output)
        self.assertIn(' -iquote sdk/ninja -Isdk/include -Isdk -Imodules -I. -Ibuild/gen/sdk -Ibuild/gen/modules '
                      '-Ibuild/gen/loader -fdiagnostics-plain-output -MD -MP -MT build/gcc/sdk/ninja/g.o '
                      '-MF build/gcc/sdk/ninja/g.d.tmp -E build/gcc/sdk/ninja/g.c > ', output)
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
                status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
                self.assertNotEqual(status, 0, output)
                self.assertIn(f'tags: {where}: {name}: its @asm tag is abi, and no --contract names it\n', output)
                self.assertFalse((root / 'build/gcc' / obj).exists())
                (root / 'config/gcc.mk').write_text(f'{GCC_MK}build/gcc/{obj}: CONTRACTS := {name} gone\n')
                status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
                self.assertNotEqual(status, 0, output)
                self.assertIn('tags: gone: --contract names no function whose @asm tag is abi or that may not be '
                              'call-safe\n', output)
                (root / 'config/gcc.mk').write_text(f'{GCC_MK}build/gcc/{obj}: CONTRACTS := {name}\n')
                status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
                self.assertEqual(status, 0, output)

    def test_makes_a_body_source_s_bodies_before_its_gcc_object(self):
        files = {'modules/mod/a.c': 'static int a[1] = {\n#include "mod/a/a.inc"\n};\n'}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'build/gcc/modules/mod/a.o', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertLess(output.index(BODIES), output.index('modules/mod/a.c > build/gcc/modules/mod/a.i'), output)

    def test_searches_the_include_directories_shc_searches(self):
        # a movie SDK member with inline assembly, the loader's own code, and patched sources with inline assembly
        # under sdk/ and src/. Each searches include directories other than its route's, which config/compilers.mk,
        # the Makefile or gcc.mk gives it
        loader = ''.join(line for line in (ROOT / 'config/compilers.mk').read_text().splitlines(True)
                         if line.startswith('$(LOADER_C:%.c=build/%.obj) $(LOADER_C:%.c=build/gcc/%.o): GAME_INC'))
        self.assertTrue(loader)
        files = {'config/compilers.mk': (COMPILERS + 'MOVIE_SDK := modules/movie/sdk/m.c\nLOADER_C := loader/l.c\n'
                                         'PATCHED_C := sdk/ninja/p.c src/q.c\nPATCHES := align-pool\n' +
                                         FOUND[len(COMPILERS):] + loader),
                 'config/mod/splits.txt': (MODULE['config/mod/splits.txt'] + 'modules/movie/sdk/m.c  P  0x0c900040  0x0c900050\n'
                                           'loader/l.c  P  0x0c900060  0x0c900070\n'
                                           'sdk/ninja/p.c  P  0x0c900080  0x0c900090\nsrc/q.c  P  0x0c9000a0  0x0c9000b0\n'),
                 'modules/movie/sdk/m.c': inline('m'), 'loader/l.c': '', 'sdk/ninja/p.c': inline('p'),
                 'src/q.c': inline('q')}
        objects = ('modules/movie/sdk/m', 'loader/l', 'sdk/ninja/p', 'src/q', 'modules/mod/a')
        with stood_in(files) as root:
            status, output = make(root, '-n', *(f'build/{obj}.obj' for obj in objects),
                                  *(f'build/gcc/{obj}.o' for obj in objects), 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        for obj in objects:
            with self.subTest(obj):
                shc = next(line for line in output.splitlines() if 'shc.exe' in line and f'{obj}.c' in line)
                gcc = next(line for line in output.splitlines() if 'sh-elf-gcc' in line and f' -o build/gcc/{obj}.o ' in line)
                self.assertEqual(['-I' + directory for directory in re.search(r'SHC_INC=(\S+)', shc)[1].split(',')],
                                 [word for word in gcc.split() if word.startswith('-I')])

    def test_writes_compiles_and_checks_the_content_of_an_image_with_a_gen_range(self):
        # tools/content.py writes its C from the disc. GCC's preprocessor reads it with the game's headers, and GCC
        # compiles what the float pass respells, each under the content's own flags. GNU ld links the object at the
        # image's addresses into a binary, which cmp holds to the disc's bytes. GCC writes the content's definitions
        # in their source's order, the image's, which the final link keeps. The gen/ range has no GCC object
        own = ''.join(line for line in (ROOT / 'config/gcc.mk').read_text().splitlines(True)
                      if line.startswith('build/gcc/gen/%/content.o: private GCCFLAGS += '))
        self.assertTrue(own)
        files = {'config/gcc.mk': GCC_MK + own, **MOD_DATA,
                 'config/mod/splits.txt': MODULE['config/mod/splits.txt'] + 'gen/mod/d_0c900180.c  D  0x0c900180  0x0c900200\n'}
        with stood_in(files) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        output = output.replace('\\\n\t', '')
        line = next(line for line in output.splitlines() if 'sh-elf-gcc' in line and 'content.c' in line)
        preprocess, _, compile_ = line.partition(' build/gen/mod/content.c > build/gcc/gen/mod/content.i && ')
        compile_ = compile_.partition(') > ')[0]
        self.assertIn('python3 tools/content.py build/gen/img/MOD.BIN build/gen/mod/content.c\n', output)
        self.assertEqual([flag for flag in preprocess.split() if flag.startswith(('-f', '-I'))],
                         ['-fdata-sections', '-fno-toplevel-reorder', '-ffp-contract=on', '-Isdk/include', '-Isrc',
                          '-Imodules', '-fdiagnostics-plain-output'])
        # the content's floats read as the game's sources' do, under R32 and GAME's flags, here without a float flag
        self.assertTrue(compile_.startswith('python3 tools/floats.py --shc r32 --system tools/sh4zam/include '
                                            'build/gcc/gen/mod/content.i && '), line)
        self.assertEqual([flag for flag in compile_.split() if flag.startswith(('-f', '-I'))],
                         ['-fdata-sections', '-fno-toplevel-reorder', '-ffp-contract=on', '-fdiagnostics-plain-output'])
        self.assertRegex(output, r'-c -o build/gcc/gen/mod/content\.o build/gcc/gen/mod/content\.i\) > '
                                 r'build/gcc/gen/mod/content\.o\.log 2>&1 \|\| ')
        self.assertIn('sh-elf/bin/sh-elf-ld -m shlelf --orphan-handling=error --fatal-warnings --oformat=binary '
                      '-T build/gen/mod/content.ld -o build/gcc/gen/mod/content.bin build/gcc/gen/mod/content.o '
                      '> build/gcc/gen/mod/content.bin.log 2>&1 || (cat build/gcc/gen/mod/content.bin.log; false)\n',
                      output)
        # the link writes a log of its own, and leaves the compile's
        self.assertEqual(output.count('> build/gcc/gen/mod/content.o.log'), 1, output)
        self.assertIn('cmp -s build/gcc/gen/mod/content.bin build/gen/mod/content.bin || ', output)
        self.assertNotIn('d_0c900180.o', output)

    def test_fetches_sh4zam_s_files_before_an_r26_content_zam_or_sh4zam_compile(self):
        # an R26 member, a gen/ range, whose image then has a content, a source in gcc/ and one of SH4ZAM's are in
        # place. tools/SHA256SUMS lists SH4ZAM's LICENSE, which is missing, beside that source
        files = {'config/compilers.mk': COMPILERS + 'R26_C := sdk/shinobi/ov.c\n', 'sdk/shinobi/ov.c': '',
                 'config/mod/splits.txt': (MODULE['config/mod/splits.txt'] + 'sdk/shinobi/ov.c  P  0x0c900040  0x0c900050\n'
                                           'gen/mod/d_0c900180.c  D  0x0c900180  0x0c900200\n'), **MOD_DATA,
                 'gcc/modules/mod/b.c': '', 'tools/sh4zam/source/sh4/a.c': '',
                 'tools/SHA256SUMS': f'{HASH}  tools/sh4zam/LICENSE\n{HASH}  tools/sh4zam/source/sh4/a.c\n'}
        for obj in ('build/gcc/sdk/shinobi/ov.o', 'build/gcc/gen/mod/content.o', 'build/gcc/modules/mod/b.o',
                    'build/gcc/sh4zam/sh4/a.o'):
            with self.subTest(obj), stood_in(files) as root:
                status, output = make(root, '-n', obj, 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                fetch = output.find('curl -fsSL -o tools/sh4zam/LICENSE.part ')
                self.assertNotEqual(fetch, -1, output)
                self.assertLess(fetch, output.index(f'-c -o {obj} '), output)

    def test_compiles_an_object_again_whose_header_changes_or_whose_d_file_is_gone(self):
        # a C source's object, and that of a source in gcc/
        for obj, source in (('modules/mod/a', 'modules/mod/a.c'), ('modules/mod/b', 'gcc/modules/mod/b.c')):
            compile_ = f'-c -o build/gcc/{obj}.o '
            with self.subTest(source), self.built({source: '', f'{obj}.h': ''}) as root:
                (root / f'build/gcc/{obj}.d').write_text(depends(f'build/gcc/{obj}.o', source, f'{obj}.h'))
                settle(root)
                status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
                self.assertNotIn(compile_, output)
                os.utime(root / f'{obj}.h')
                status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                self.assertIn(compile_, output)
                settle(root)
                (root / f'build/gcc/{obj}.d').unlink()
                status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                self.assertIn(compile_, output)

    def test_splits_a_source_again_whose_split_c_is_gone(self):
        files = {'config/compilers.mk': FOUND, 'src/main.c': inline()}
        with self.built(files) as root:
            (root / 'build/gcc/src/main.d').write_text(depends('build/gcc/src/main.o', 'build/gcc/src/main.c'))
            settle(root)
            (root / 'build/gcc/src/main.c').unlink()
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn('python3 tools/inline.py src/main.c build/gcc/src/main.c', output)

    def test_keeps_the_last_whole_dependency_file_of_a_compile_that_fails(self):
        # a compile that writes a .d file make can't read, and fails, leaves the last whole one, so make compiles again
        with self.built() as root:
            gcc = root / 'sh-elf/bin/sh-elf-gcc'
            working = gcc.read_text()
            gcc.write_text('#!/bin/sh\nwhile [ $# -gt 1 ]; do [ "$1" = -MF ] && d=$2; shift; done\n'
                           '[ -z "$d" ] || echo "not make" > "$d"\nexit 1\n')
            (root / 'modules/mod/a.c').write_text((root / 'modules/mod/a.c').read_text() + '\n')
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertNotEqual(status, 0, output)
            gcc.write_text(working)
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
        self.assertEqual(status, 0, output)
        self.assertIn('gcc    modules/mod/a.c', output)

    def test_compiles_again_with_another_compiler_assembler_or_flags(self):
        # C sources, a .src file, a source in gcc/ in c.src's place, and SH4ZAM's sources
        files = {**LIBRARY, **placing('modules/mod/c.src'), 'modules/mod/c.src': '', 'gcc/modules/mod/c.c': ''}
        for changed in (f'sh-elf/{CC1}', 'sh-elf/bin/sh-elf-as', 'config/compilers.mk', 'config/gcc.mk', 'Makefile',
                        'gcc.mk'):
            with self.subTest(changed), self.built(files) as root:
                with open(root / changed, 'a') as file:
                    file.write('\n# another\n')
                status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                for obj in ('src/main.o', 'modules/mod/a.o', 'modules/mod/b.o', 'modules/mod/c.o', 'sh4zam/sh4/a.o',
                            'sh4zam/sh4/b.o'):
                    self.assertIn(f'-o build/gcc/{obj} ', output)

    def test_compiles_no_shc_object_again_once_gcc_mk_or_config_gcc_mk_changes(self):
        # the census and the import check read 1ST_READ.BIN's SHC object, whose command neither file gives
        for changed in ('gcc.mk', 'config/gcc.mk'):
            with self.subTest(changed), self.built() as root:
                with open(root / changed, 'a') as file:
                    file.write('\n# another\n')
                status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                self.assertIn('-o build/gcc/src/main.o ', output)
                self.assertNotIn('shc.exe', output)

    def test_compiles_each_c_source_again_once_floats_py_or_a_module_it_imports_changes(self):
        # and assembles each .src file again, whose constants the float pass checks. The float pass imports its
        # modules, tools/float_*.py, tools/tool.py and tools/layout.py
        for changed, objects in (('tools/tool.py', ('src/main.o', 'modules/mod/a.o', 'modules/mod/b.o')),
                                 ('tools/floats.py', ('src/main.o', 'modules/mod/a.o', 'modules/mod/b.o')),
                                 ('tools/float_value.py', ('src/main.o', 'modules/mod/a.o', 'modules/mod/b.o')),
                                 ('tools/layout.py', ('src/main.o', 'modules/mod/a.o', 'modules/mod/b.o'))):
            with self.subTest(changed), self.built() as root:
                with open(root / changed, 'a') as file:
                    file.write('\n# another\n')
                status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                for obj in objects:
                    self.assertIn(f'-o build/gcc/{obj} ', output)

    def test_compiles_no_c_source_again_once_tools_data_py_changes(self):
        # the float pass imports tools/tool.py and tools/layout.py, and not tools/data.py, which imports both
        with self.built() as root:
            with open(root / 'tools/data.py', 'a') as file:
                file.write('\n# another\n')
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertNotIn('-o build/gcc/src/main.o ', output)

    def test_compiles_no_gcc_or_sh4zam_source_again_once_the_float_pass_or_a_module_it_imports_changes(self):
        # no SHC builds either, so the float pass reads neither's C. GNU as alone assembles SH4ZAM's assembly
        for changed in ('tools/tool.py', 'tools/floats.py', 'tools/layout.py'):
            with self.subTest(changed), self.built({**LIBRARY, 'gcc/modules/mod/b.c': ''}) as root:
                with open(root / changed, 'a') as file:
                    file.write('\n# another\n')
                status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
                self.assertEqual(status, 0, output)
                for obj in ('modules/mod/b.o', 'sh4zam/sh4/a.o', 'sh4zam/sh4/b.o'):
                    self.assertNotIn(f'-o build/gcc/{obj} ', output)

    def test_stops_on_a_toolchain_without_libgcc_a_as_it_reads_its_stamp(self):
        # the linker's stamp names libgcc.a by a glob, which here matches no file
        with stood_in() as root:
            (root / f'sh-elf/{LIBGCC}').unlink()
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertNotEqual(status, 0, output)
        self.assertIn("build/tools/ld.id: can't refresh it from sh-elf/bin/sh-elf-ld sh-elf/bin/sh-elf-ar "
                      'sh-elf/lib/gcc/sh-elf/*/libgcc.a.  Stop.', output)

    def test_compiles_an_object_named_alone_again_with_another_compiler(self):
        # make build/gcc/modules/mod/a.o, without make gcc, refreshes GCC's stamp as make gcc does
        with self.built() as root:
            with open(root / f'sh-elf/{CC1}', 'a') as file:
                file.write('\n# another\n')
            status, output = make(root, '-n', 'build/gcc/modules/mod/a.o', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn('-o build/gcc/modules/mod/a.o ', output)

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

    def test_compiles_a_body_source_s_stub_again_with_another_shc(self):
        # a.c's GCC object waits for its bodies, which its stub's listing finds, and SHC compiles the stub
        with stood_in({**BUILT, **PINNED}) as root:
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
            self.assertEqual(status, 0, output)
            settle(root)
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
            self.assertEqual(status, 0, output)
            self.assertNotIn(STUB_SHC, output)
            (root / 'tools/shc-v5.0r32/shc.exe').write_text('another')
            status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
        self.assertEqual(status, 0, output)
        self.assertIn(STUB_SHC, output)

    @contextlib.contextmanager
    def built(self, files=None):
        """stood_in()'s checkout once make gcc has built it and settle() has dated what it wrote."""
        with stood_in(files) as root:
            status, output = make(root, 'gcc', 'SH_ELF=sh-elf', 'CC=true')
            self.assertEqual(status, 0, output)
            settle(root)
            yield root


class Sums(unittest.TestCase):
    def test_passes_with_make_gcc_s_configuration_broken(self):
        # a call left open in the repository's config/gcc.mk, which only make gcc's goals read, stops make gcc and
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
                status, output = make(root, '-n', 'gcc', 'SH_ELF=sh-elf')
                self.assertNotEqual(status, 0, output)
                self.assertRegex(output, r"\Aconfig/gcc\.mk:\d+: \*\*\* unterminated call to function 'call': "
                                         r"missing '\)'\.  Stop\.\n\Z")


class Pragmas(unittest.TestCase):
    def test_gcc_compiles_no_pragma_but_those_it_knows_or_may_ignore(self):
        # GCC_WARNINGS' -Wno-unknown-pragmas lets GCC ignore SHC's #pragma inline and #pragma section, and
        # tools/inline.py splits the inline assembly's pragmas off before GCC reads the C. Any other pragma GCC doesn't
        # know, such as #pragma global_register, sits on the shipped side of its guard, where GCC never reads it. A
        # line whose group the tool can't tell counts as compiled
        known = {'inline', 'section', 'pack', 'inline_asm', 'asm', 'endasm'}
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
