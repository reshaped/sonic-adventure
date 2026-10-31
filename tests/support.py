"""Build temporary input trees for tools that read the working directory, and run make in one."""

import contextlib
import hashlib
import os
import re
import subprocess
import tempfile
import time
from pathlib import Path

import vendor
from images import EVENT_ALIASES, ROOTS

ROOT = Path(__file__).resolve().parents[1]


@contextlib.contextmanager
def cwd(path):
    """path as the working directory, the old one back after."""
    old = os.getcwd()
    os.chdir(path)
    try:
        yield
    finally:
        os.chdir(old)


@contextlib.contextmanager
def tree(files):
    """A temporary working directory holding files, a {path: text or bytes} map."""
    with tempfile.TemporaryDirectory() as root:
        for name, content in files.items():
            path = Path(root, name)
            path.parent.mkdir(parents=True, exist_ok=True)
            if isinstance(content, bytes):
                path.write_bytes(content)
            else:
                path.write_text(content)
        with cwd(root):
            yield Path(root)


# a module's tables and sources, and 1ST_READ.BIN's, whose symbols.txt every module reads
MODULE = {
    'config/1st_read/splits.txt': 'src/main.c  P  0x8c0101a0  0x8c0101b0\n',
    'config/1st_read/symbols.txt': '0x8c0101a0  main\n0x0c900040  shadowed\n0x8c0101a4  shared\n',
    'config/1st_read/census.txt': '',
    'config/mod/splits.txt': ('P  0x0c900000  0x0c900100\n'
                              'D  0x0c900100  0x0c900200\n'
                              'B  0x0c900200  0x0c900300\n'
                              'modules/mod/a.c  P  0x0c900000  0x0c900010\n'
                              'modules/mod/a.c  D  0x0c900100  0x0c900110\n'
                              'modules/mod/b.src  P  0x0c900020  0x0c900030\n'
                              'fill  0xff\n'),
    'config/mod/symbols.txt': '0x0c900000  a_init\n0x0c900000  a_start\n0x0c900104  shared\n',
    'config/kept.txt': '',
    'src/main.c': '',
    'modules/mod/a.c': '',
    'modules/mod/b.src': '',
}

# LOADER.BIN's section rows: each section strt1's tables bound that holds bytes needs a row, but DSG32 and BSG32
# can lie in DSG's and BSG's, as they do here
LOADER_SECTIONS = ('DSGLH  0x8ce00000  0x8ce00080', 'DSGLE  0x8ce00080  0x8ce000a0', 'P  0x8ce000a0  0x8ce064f4',
                   'C  0x8ce064f4  0x8ce06cc0', 'D  0x8ce06cc0  0x8ce06d00', 'PSG  0x8ce06d00  0x8ce1f72c',
                   'CSG  0x8ce1f72c  0x8ce20340', 'DSG  0x8ce20340  0x8ce20a00', 'B  0x8ce20a00  0x8ce50040',
                   'BSG  0x8ce50040  0x8ce7e920')

# the tree the tests of memory.py, spec.py, publish.py and disc.py read. It holds the tables and C of 1ST_READ.BIN,
# two stages and an event, and the lines memory.py reads of the game's sources. loader.c reads stga.PRS for STAGE_A
# and stgb.PRS for STAGE_B, each to MODULE_AREA. It reads s_mot.PRS to MOTION_AREA. The tree has no tables for
# s_mot.PRS, so memory.py takes it for a motion file that no image is. demos[] plays recorded pads in STAGE_A alone.
# stage.c's clear_events gives ev0001 as Sonic clears STAGE_B's first act. Each stage's symbols.txt puts its
# stage_flag at 0x0c900010. 1ST_READ.BIN's symbols.txt puts the stage_flag main.c reads at 0x8c900010. The same RAM
# appears at 0x0c900010. 1ST_READ.BIN's symbols.txt puts main_count and main_count_alias at one address. main.c
# defines main_count and reads main_count_alias
GAME_TREE = {
    'src/system/loader.h': ('#define SECTOR_SIZE 2048\n#define WORK_AREA ((Uint8 *)0x0cd00000)\n'
                            '#define WORK_AREA_END ((Uint8 *)0x0cf00000)\n#define MODULE_AREA ((Uint8 *)0x0c900000)\n'
                            '#define MOTION_AREA ((Uint8 *)0x0cc00000)\n'
                            '#define SONIC_BOARD_AREA ((Uint8 *)0x0cb08000)\n'
                            '#define TAILS_BOARD_AREA ((Uint8 *)0x0cadc000)\n'
                            # the set and camera areas, in loader.h's order: areas 0 to 2 lie 32 KB apart, and areas
                            # 3 to 5 fill the 16 KB after each
                            + ''.join(f'#define {kind}_AREA_{n} ((Uint8 *){base + offset:#010x})\n'
                                      for kind, base in (('SET', 0x0CB68000), ('CAMERA', 0x0CB50000))
                                      for n, offset in enumerate((0, 0x8000, 0x10000, 0x4000, 0xC000, 0x14000)))),
    'src/system/loader.c': ('#define FIELD_AREA ((Uint8 *)0x0c920000)\n#define TIKAL_AREA ((Uint8 *)0x0cb00000)\n'
                            '#define EXTRA_AREA ((Uint8 *)0x0cb80000)\n\nvoid loader_read_stage_files(void)\n{\n'
                            '    switch (stage) {\n'
                            '    case STAGE_A:\n        loader_read_prs("stga.PRS", MODULE_AREA);\n        return;\n'
                            '    case STAGE_B:\n        loader_read_prs("stgb.PRS", MODULE_AREA);\n        return;\n'
                            '    }\n}\n\nvoid loader_read_character_motions(int character)\n{\n'
                            '    switch (character) {\n    case CHARACTER_SONIC:\n'
                            '        loader_read_prs("s_mot.PRS", MOTION_AREA);\n        break;\n    }\n}\n'),
    'src/event/play.c': ('#define EVENT_MODULE ((struct event_module *)0x0cb80000)\n'
                         '#define EVENT_MODULE_SIZE 0x48000\n'),
    'sdk/shinobi/strt2.c': '#define STACK_BOTTOM 0x8c00c000\n#define STACK_TOP 0x8c00f400\n',
    'src/system/sbinit.c': '#define HEAP_SIZE 0x100000\n',
    'loader/sbinit.c': '#define HEAP ((void *)0x8cf00000)\n#define HEAP_SIZE 0x100000\n',
    'sdk/include/shinobi.h': '#define BUD_CAPACITY_1MB (1 + 8 + 113)\n#define BUD_WORK_GAP 32\n',
    'src/object/download.c': ('#define MODEL_A_BUFFER ((struct model_file *)0x8cca4000)\n'
                              '#define MODEL_B_BUFFER ((struct model_file *)0x8cca6000)\n'),
    'src/sound/stream.c': ('#define STREAM_WORK 0x0cca8000\n#define MUSIC_WORK_SIZE 0x2d0c4\n'
                           '#define VOICE_WORK_SIZE 0xd884\n'),
    'modules/movie/movie.c': '#define WORK ((Sint8 *)0x8c900000)\n#define WORK_SIZE 0x300000\n',
    'modules/movie/cast.c': ''.join(f'texture_load_prs_file("{name}", &texlist);\n' for name in
                                    ('KNUCKLES.PRS', 'EV_TR2_BIG.PRS', 'AMY.PRS', 'MILES.PRS')),
    'src/progress/recorder.h': '#define DEMO_RECORDING ((struct pad_sample *)0x0cbe0000)\n',
    'src/system/texture.c': ('#ifdef GCC\n#define FILE_ROOM(size) (SECTORS(size) * SECTOR_SIZE)\n#else\n'
                             '#define FILE_ROOM(size) (((size) + 255) & ~255)\n#endif\n'),
    'src/progress/demo.c': ('static struct demo demos[DEMO_COUNT] = {\n'
                            '    {STAGE_A, 0, CHARACTER_SONIC, DEMO_NO_EVENT},\n};\n\nvoid demo_advance(void)\n{\n'
                            '    loader_read_file("K_SONIC.bin", DEMO_RECORDING);\n}\n'),
    'src/stage/stage.c': ('static struct stage_event entry_events[] = {\n    {STAGE_NONE, 0, 0, 0, 0, NULL},\n};\n\n'
                          'static struct stage_event clear_events[] = {\n'
                          '    {STAGE_B, 0, CHARACTER_SONIC, 1, 1, NULL},\n    {STAGE_NONE, 0, 0, 0, 0, NULL},\n};\n'),
    'config/1st_read/splits.txt': 'src/main.c  P  0x8c0101a0  0x8c0101b0\n',
    'config/1st_read/symbols.txt': ('0x8c0101a0  main\n0x8c1755e0  main_count\n0x8c1755e0  main_count_alias\n'
                                    '0x8c900010  stage_flag\n'),
    'config/stga/splits.txt': ('P  0x0c900000  0x0c900100\nD  0x0c900100  0x0c900200\nB  0x0c900200  0x0c900300\n'
                               'modules/stga/a.c  P  0x0c900000  0x0c900010\n'
                               'modules/stga/a.c  D  0x0c900100  0x0c900104\n'),
    'config/stga/symbols.txt': '0x0c900010  stage_flag\n',
    'config/stgb/splits.txt': ('P  0x0c900000  0x0c900100\nD  0x0c900100  0x0c900200\n'
                               'modules/stgb/b.c  P  0x0c900000  0x0c900010\n'
                               'modules/stgb/b.c  D  0x0c900100  0x0c900104\n'),
    'config/stgb/symbols.txt': '0x0c900010  stage_flag\n',
    'config/ev0001/splits.txt': ('DHEAD  0x0cb80000  0x0cb80008\nP  0x0cb80008  0x0cb80100\n'
                                 'events/ev0001/head.c  DHEAD  0x0cb80000  0x0cb80008\n'
                                 'events/ev0001/script.c  P  0x0cb80008  0x0cb80020\n'),
    'config/ev0001/symbols.txt': '0x0cb80008  ev0001_script\n',
    'config/imports.txt': '',
    'src/main.c': ('extern int stage_flag;\nextern int main_count_alias;\n\nint main_count = 1;\n\n'
                   'int main(void)\n{\n    return stage_flag + main_count_alias;\n}\n'),
    'modules/stga/a.c': ('int stage_flag = 1;\nint stage_bss;\n\n'
                         'int stage_a(void)\n{\n    return stage_flag + stage_bss;\n}\n'),
    'modules/stgb/b.c': 'int stage_flag = 2;\n\nint stage_b(void)\n{\n    return stage_flag;\n}\n',
    'events/ev0001/head.c': ('extern void ev0001_script(int phase);\n\nstruct head {\n    void (*entry)(int phase);\n'
                             '    void *textures;\n};\n\n#ifdef GCC\n'
                             'static struct head ev0001_head __attribute__((used));\n#endif\n'
                             'static struct head ev0001_head = {ev0001_script, 0};\n'),
    'events/ev0001/script.c': 'int ev0001_count;\n\nvoid ev0001_script(int phase)\n{\n    ev0001_count += phase;\n}\n',
}
# the C of GAME_TREE's images, which test_spec.py compiles to the GCC objects spec.py reads
GAME_TREE_C = ('src/main.c', 'modules/stga/a.c', 'modules/stgb/b.c', 'events/ev0001/head.c', 'events/ev0001/script.c')

# a compilers.mk that names no source, with the flags the Makefile's rules read
COMPILERS = 'GAME := -cpu=sh4\nSDK := -cpu=sh4\n'
# a SHA-256 hash that no file has
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


# the settings the Makefile takes. make puts each one its command line gives
# in its recipes' environment, as make test V=1 puts V
SETTINGS = re.search(r'(?m)^SETTINGS := (.*)$', (ROOT / 'Makefile').read_text())[1].split()


def make(root, *args, path=None):
    """make's status and output, stdout and stderr together, run in root
    with -j1 and then args. make runs in the C locale, so that a host tool's
    messages read as the tests expect, and with path for PATH when given. It
    takes no flags or makefiles from the environment, as a make running this
    or the shell may give them, and none of SETTINGS, such as the SH_ELF
    that names the tests' toolchain."""
    env = {name: value for name, value in os.environ.items()
           if name not in ('MAKEFLAGS', 'MFLAGS', 'MAKELEVEL', 'GNUMAKEFLAGS', 'MAKEFILES', *SETTINGS)}
    env['LC_ALL'] = 'C'
    if path is not None:
        env['PATH'] = path
    run = subprocess.run(['make', '-j1', *args], cwd=root, env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         text=True)
    return run.returncode, run.stdout


# each command but the C compiler that the Makefile's and gcc.mk's host
# checks can ask for
COMMANDS = ('sha256sum', 'timeout', 'curl', 'tar', 'gzip', 'xz', 'c++', 'cmp')


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


def depends(target, source, *headers):
    """A .d file's text for target, as the host compiler's -MM -MP writes it."""
    return f'{target}: {" ".join((source, *headers))}\n' + ''.join(f'{header}:\n' for header in headers)


# the tools make downloads on first use, in place
PINNED = {'tools/wibo-i686': '', **{f'tools/shc-v5.0{release}/shc.exe': '' for release in ('r10', 'r26', 'r32')}}
# a file in each directory the Makefile searches for sources, so that none of its scans prints an error
SEARCHED = {f'{root}/.gitkeep': '' for root in (*ROOTS, 'gcc')}
# the originals orig/ takes, each holding its own name
ORIGINALS = {f'orig/{name}': name.encode()
             for name in ('1ST_READ.BIN', 'SONICADV/MOD.PRS', *(f'SONICADV/{alias}.PRS' for alias in EVENT_ALIASES))}
# the tables of X_MOT.PRS, a motion file: its one section, D, holds no code. D is a gen/ range of eight bytes.
# make gcc's census check reads the file decoded, for addresses in 1ST_READ.BIN's image
MOTION = {'config/x_mot/splits.txt': ('D  0x0cc00000  0x0cc00008\n'
                                      'gen/x_mot/d_0cc00000.c  D  0x0cc00000  0x0cc00008\n'),
          'config/x_mot/symbols.txt': '', 'config/x_mot/data.txt': '0x0cc00000  words[2]\n'}
# COMPILERS, and config/compilers.mk's own lines that find the C with inline assembly and route it
FOUND = COMPILERS + ''.join(line for line in (ROOT / 'config/compilers.mk').read_text().splitlines(True)
                            if line.startswith(('inline_asm =', 'INLINE_C :=', 'ASMCODE :=')))


# a curl that writes "wheel" to the file -o names
CURL = '#!/bin/sh\nwhile [ $# -gt 1 ]; do [ "$1" = -o ] && out=$2; shift; done\nprintf wheel > "$out"\n'


def listing(paths):
    """tools/SHA256SUMS's lines for paths, each with the hash of CURL's file."""
    return ''.join(f'{hashlib.sha256(b"wheel").hexdigest()}  {path}\n' for path in paths)


# the data.txt of a gen/ range of MOD.PRS's D, from 0x0c900180 to 0x0c900200
MOD_DATA = {'config/mod/data.txt': '0x0c900180  words[32]\n'}
# a.c, a source that includes a header, a body, its texts and its values; a gen/ range in MOD.PRS's D; g.c, a
# grafted source, whose donor includes a header; p.c, a patched source; s.c, whose object R10 alone builds;
# MOD.PRS's original; and every file make writes in building them, their stubs and the other sources make check
# builds, in splicing MOD.BIN from them, and in writing MOD.PRS's content
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
    **MOD_DATA,
    'orig/SONICADV/MOD.PRS': '',
    **{path: '' for path in (
        'build/tools/wibo.id', 'build/tools/r10.id', 'build/tools/r26.id', 'build/tools/r32.id',
        'build/tools/vendored.ok', 'build/prskit',
        'build/src/main.obj', 'build/1st_read/src/main.bin', 'build/modules/mod/b.obj', 'build/mod/modules/mod/b.bin',
        *(f'build/stub/modules/mod/a/{inc}' for inc in A_INCS), 'build/stub/modules/mod/a.obj',
        'build/stub/modules/mod/a.lis', 'build/stub/mod/modules/mod/a.bin', 'build/verified/SONICADV/MOD.PRS.ok',
        'build/gen/img/MOD.BIN', *(f'build/gen/modules/mod/a/{inc}' for inc in A_INCS),
        'build/gen/modules/mod/a/bodies.txt', 'build/modules/mod/a.obj', 'build/mod/modules/mod/a.bin',
        'build/gen/mod/d_0c900180.syms', 'build/gen/mod/d_0c900180.c', 'build/gen/mod/d_0c900180.obj',
        'build/mod/gen/mod/d_0c900180.bin',
        *(f'build/gen/mod/content.{suffix}' for suffix in ('c', 'd', 'txt', 'ld', 'bin')),
        'build/shckit', 'build/sdk/ninja/g.obj', 'build/sdk/ninja/g.lis', 'build/mod/sdk/ninja/g.bin',
        'build/sdk/ninja/p.obj', 'build/sdk/ninja/p.lis', 'build/mod/sdk/ninja/p.bin', 'build/sdk/ninja/s.obj',
        'build/mod/sdk/ninja/s.bin', 'build/modules/mod/b.lis', 'build/SONICADV/MOD.BIN')},
    'build/src/main.d': depends('build/src/main.obj', 'src/main.c'),
    'build/stub/modules/mod/a.d': depends('build/stub/modules/mod/a.obj', 'modules/mod/a.c', 'modules/mod/a.h',
                                          *(f'build/stub/modules/mod/a/{inc}' for inc in A_INCS)),
    'build/modules/mod/a.d': depends('build/modules/mod/a.obj', 'modules/mod/a.c', 'modules/mod/a.h',
                                     *(f'build/gen/modules/mod/a/{inc}' for inc in A_INCS)),
    'build/gen/mod/d_0c900180.d': depends('build/gen/mod/d_0c900180.obj', 'build/gen/mod/d_0c900180.c'),
    'build/gen/mod/d_0c900180.c.d': 'build/gen/mod/d_0c900180.c:\n',
    'build/sdk/ninja/g.d': depends('build/sdk/ninja/g.obj', 'sdk/ninja/g.c'),
    'build/sdk/ninja/g_donor.d': depends('build/sdk/ninja/g.obj', 'sdk/ninja/d.c', 'sdk/ninja/d.h'),
    'build/sdk/ninja/p.d': depends('build/sdk/ninja/p.obj', 'sdk/ninja/p.c'),
    'build/sdk/ninja/s.d': depends('build/sdk/ninja/s.obj', 'sdk/ninja/s.c'),
}
# two steps of BUILT's, by the text make -n prints for each: the compile of a.c's stub, and the step that writes
# a.c's bodies
STUB_SHC = 'shc.exe modules/mod/a.c -sub=build/stub/modules/mod/a.obj.sub'
BODIES = 'tools/data.py --bodies build/gen/img/MOD.BIN modules/mod/a.c'

# a tree in which make and make check have only their hashes left to check: the originals; MODULE's objects and
# links, and MOD.PRS's decoded and rebuilt files, built; SHA256SUMS's line for each original, link and decoded file,
# in path order; and the pinned tools in place. make check's splice.py --check needs every image's code written,
# so a stand-in that passes takes its place; and its progress.py --check needs every image and README.md's numbers,
# so one that passes takes its place too
LISTED = {**ORIGINALS, 'build/1st_read/src/main.bin': bytes(16), 'build/mod/modules/mod/a.bin': bytes(32),
          'build/mod/modules/mod/b.bin': bytes(16), 'build/SONICADV/MOD.BIN': b'MOD'}
HASHED = {
    **PINNED, **SEARCHED, **LISTED, 'build/gen/img/MOD.BIN': b'MOD',
    'tools/splice.py': '', 'tools/progress.py': '',
    'SHA256SUMS': ''.join(f'{hashlib.sha256(data).hexdigest()}  {path}\n' for path, data in sorted(LISTED.items())),
    'build/1ST_READ.BIN': ORIGINALS['orig/1ST_READ.BIN'], 'build/SONICADV/MOD.PRS': ORIGINALS['orig/SONICADV/MOD.PRS'],
    **{path: '' for path in ('build/verified/1ST_READ.BIN.ok', 'build/verified/SONICADV/MOD.PRS.ok',
                             'build/tools/vendored.ok', 'build/prskit',
                             'build/src/main.obj', 'build/modules/mod/a.obj', 'build/modules/mod/b.obj',
                             'build/modules/mod/b.lis')},
    'build/src/main.d': depends('build/src/main.obj', 'src/main.c'),
    'build/modules/mod/a.d': depends('build/modules/mod/a.obj', 'modules/mod/a.c'),
}
