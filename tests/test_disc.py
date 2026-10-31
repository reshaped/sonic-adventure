import contextlib
import errno
import hashlib
import io
import os
import shutil
import signal
import subprocess
import sys
import time
import unittest
from pathlib import Path
from unittest import mock

import disc
import manifest
import memory
import test_manifest
from images import images
from support import GAME_TREE, ROOT, tree

# a disc's map, as build/gdi map prints it: the volume's first sectors, a directory, two files a build replaces, one
# it keeps, a run of free sectors and the boot file
MAP = '''\
  45000      16             system area
  45016       2             volume descriptors
  45018       1       2048  SONICADV/
  45019      10      20480  SONICADV/A.BIN
  45029       4       8192  SONICADV/B.PRS
  45033       6      12288  SONICADV/KEPT.BIN
  45039      30             unreferenced
  45069      30      61440  1ST_READ.BIN (boot)
'''
# a disc's map whose first file, A.BIN, meets the free sectors after it
MEETING = '''\
  45000      16             system area
  45016       2             volume descriptors
  45018       1       2048  SONICADV/
  45019       4       8192  SONICADV/A.BIN
  45023       3             unreferenced
  45026       2       4096  SONICADV/B.BIN
  45028       6             unreferenced
  45034      30      61440  1ST_READ.BIN (boot)
'''
# stga's and ev0001's files, as make gcc writes them: each image's .BIN, its .PRS and its final link's ELF
FILES = {'build/gcc/SONICADV/STGA.BIN': b'stga image', 'build/gcc/SONICADV/STGA.PRS': b'stga file',
         'build/gcc/SONICADV/EV0001.BIN': b'ev0001 image', 'build/gcc/SONICADV/EV0001.PRS': b'ev0001 file',
         'build/gcc/link/stga/final.elf': b'stga elf', 'build/gcc/link/ev0001/final.elf': b'ev0001 elf'}


# FILES, with 1ST_READ.BIN's and stgb's: the boot file in its two sectors, for a disc of every image GAME_TREE's tables
# describe
WHOLE = {**FILES, 'build/gcc/1ST_READ.BIN': bytes(4096), 'build/gcc/link/1st_read/final.elf': b'main elf',
         'build/gcc/SONICADV/STGB.BIN': b'stgb image', 'build/gcc/SONICADV/STGB.PRS': b'stgb file',
         'build/gcc/link/stgb/final.elf': b'stgb elf'}


def published(images=('1st_read', 'ev0001', 'stga', 'stgb'), files=WHOLE):
    """manifest.txt, as tools/publish.py writes it, and the spec, the tools and the final links it gives hashes, by
    path. The spec links images, each at its origin and length in GAME_TREE's memory contract, and the manifest gives
    each an end 16 bytes past its origin; its final link, as files holds it or else of its name; and a file row for
    each other of files whose path names it."""
    origins = {'1st_read': (0x8C010000, 0x8F0000), 'stga': (0x0C900000, 0x280000), 'stgb': (0x0C900000, 0x280000),
               'stgc': (0x0C900000, 0x280000), 'ev0001': (0x0CB80000, 0x80000)}
    finals = {image: f'build/gcc/link/{image}/final.elf' for image in images}
    linked = {'build/gcc/link/spec.txt': ''.join(f'image  {image}  {disc.hexa(origins[image][0])}  '
                                                 f'{disc.hexa(origins[image][1])}\n' for image in images).encode(),
              **test_manifest.TOOLS, **{path: files.get(path, image.encode()) for image, path in finals.items()}}
    built = {image: manifest.Built(*origins[image], origins[image][0] + 16, [], {}, {}, {},
                                   [finals[image], *(path for path in files if f'/{image.upper()}.' in path)])
             for image in images}
    hashes = {path: hashlib.sha256(data).hexdigest() for path, data in {**files, **linked}.items()}
    found = manifest.Manifest(hashes, list(test_manifest.TOOLS), 'build/gcc/link/spec.txt', built)
    return {**linked, 'manifest.txt': manifest.text(found)}


def built(origin, length, end):
    """An image of a manifest, as tools/manifest.py's recorded() gives one, of no input, binding or file."""
    return manifest.Built(origin, length, end, [], {}, {}, {}, [])


class Plan(unittest.TestCase):
    def test_moves_each_file_that_grows_in_path_order_to_the_lowest_free_run_that_holds_it(self):
        # A.BIN, given after B.PRS, moves first and takes the first 12 of the 30 unreferenced sectors. The free runs are
        # then the 10 sectors A.BIN left and the 18 after its new ones. B.PRS takes the start of the first run where it
        # needs 6 sectors, and of the second where it needs 11
        mapped = disc.regions(MAP, 'map')
        for need, start in ((6, 45019), (11, 45051)):
            with self.subTest(need=need):
                placed = disc.plan(mapped, {'SONICADV/B.PRS': need * 2048 - 1, 'SONICADV/A.BIN': 12 * 2048})
                self.assertEqual(placed, {'SONICADV/A.BIN': 45039, 'SONICADV/B.PRS': start})

    def test_joins_the_extent_a_file_leaves_to_the_free_sectors_it_meets(self):
        # A.BIN moves to the second unreferenced run, and its 4 sectors and the 3 unreferenced after them hold B.BIN's
        # 7, which neither holds alone
        placed = disc.plan(disc.regions(MEETING, 'map'), {'SONICADV/A.BIN': 5 * 2048, 'SONICADV/B.BIN': 7 * 2048})
        self.assertEqual(placed, {'SONICADV/A.BIN': 45028, 'SONICADV/B.BIN': 45019})

    def test_fails_where_the_free_sectors_left_hold_no_file_that_grows(self):
        # A.BIN takes 12 of the 30 unreferenced sectors. The free runs are then the 10 it left and the 18 after its new
        # ones, and neither holds B.PRS's 19
        with self.assertRaisesRegex(SystemExit, ": SONICADV/B.PRS: needs 19 sectors, 15 more than it holds, and no "
                                                "run of the disc's free sectors holds them$"):
            disc.plan(disc.regions(MAP, 'map'), {'SONICADV/A.BIN': 12 * 2048, 'SONICADV/B.PRS': 19 * 2048})

    def test_keeps_the_start_of_a_file_its_sectors_hold(self):
        self.assertEqual(disc.plan(disc.regions(MAP, 'map'), {'SONICADV/A.BIN': 10 * 2048, 'SONICADV/B.PRS': 1}),
                         {})

    def test_keeps_the_boot_file_s_end_and_fails_where_it_grows(self):
        self.assertEqual(disc.plan(disc.regions(MAP, 'map'), {'1ST_READ.BIN': 28 * 2048}), {'1ST_READ.BIN': 45071})
        with self.assertRaisesRegex(SystemExit, ': 1ST_READ.BIN: the boot file keeps its end, and it needs 31 '
                                                'sectors, where it holds 30$'):
            disc.plan(disc.regions(MAP, 'map'), {'1ST_READ.BIN': 30 * 2048 + 1})

    def test_fails_on_a_path_the_disc_holds_no_file_at(self):
        # a directory's path names no file either
        for path in ('SONICADV/C.BIN', 'SONICADV/'):
            with self.subTest(path), self.assertRaisesRegex(SystemExit, f': {path}: no file of the disc$'):
                disc.plan(disc.regions(MAP, 'map'), {path: 1})

    def test_takes_the_run_of_each_file_and_of_no_directory(self):
        self.assertEqual(list(disc.file_runs(disc.regions(MAP, 'map'))),
                         ['SONICADV/A.BIN', 'SONICADV/B.PRS', 'SONICADV/KEPT.BIN', '1ST_READ.BIN'])

    def test_fails_on_a_line_that_isn_t_build_gdi_map_s(self):
        with self.assertRaisesRegex(SystemExit, ': map:2: not a line of build/gdi map'):
            disc.regions('  45000      16             system area\nsomething else\n', 'map')


class Manifest(unittest.TestCase):
    def test_takes_each_image_s_file_and_gives_an_alias_its_event_s(self):
        with tree({**GAME_TREE, **WHOLE, **published()}):
            found = disc.read(Path('manifest.txt').read_bytes(), Path('manifest.txt'), images()).images
            self.assertEqual(disc.replacements(found), {
                '1ST_READ.BIN': Path('build/gcc/1ST_READ.BIN'),
                'SONICADV/EV0001.PRS': Path('build/gcc/SONICADV/EV0001.PRS'),
                'SONICADV/STGA.PRS': Path('build/gcc/SONICADV/STGA.PRS'),
                'SONICADV/STGB.PRS': Path('build/gcc/SONICADV/STGB.PRS')})
            with mock.patch.dict(disc.EVENT_ALIASES, {'EV0054': 'EV0001'}, clear=True):
                self.assertEqual(disc.replacements(found)['SONICADV/EV0054.PRS'], Path('build/gcc/SONICADV/EV0001.PRS'))

    def test_fails_on_a_manifest_that_leaves_an_image_out_or_holds_one_config_doesn_t_describe(self):
        # each manifest is its spec's whole
        for files, problem in ((published(images=('1st_read', 'ev0001', 'stga')), 'names no image stgb, which config/ '
                                                                                   'describes'),
                               (published(images=('1st_read', 'ev0001', 'stga', 'stgb', 'stgc')),
                                "names image stgc, which config/ doesn't describe")):
            with self.subTest(problem), tree({**GAME_TREE, **WHOLE, **files}), \
                    self.assertRaisesRegex(SystemExit, f': manifest.txt: {problem}$'):
                disc.read(Path('manifest.txt').read_bytes(), Path('manifest.txt'), images())

    def test_fails_on_a_manifest_tools_manifest_py_refuses(self):
        # a file whose hash isn't the manifest's, and a line that is no row
        with tree({**GAME_TREE, **WHOLE, **published(), 'build/gcc/SONICADV/STGA.PRS': b'another'}), \
                self.assertRaisesRegex(SystemExit, ': manifest.txt: build/gcc/SONICADV/STGA.PRS: no file with the hash '
                                                   'the manifest gives it$'):
            disc.read(Path('manifest.txt').read_bytes(), Path('manifest.txt'), images())
        files = published()
        with tree({**GAME_TREE, **WHOLE, **files, 'manifest.txt': files['manifest.txt'] + 'fiel     stga  a  b\n'}), \
                self.assertRaisesRegex(SystemExit, f": manifest.txt:{len(files['manifest.txt'].splitlines()) + 1}: not "
                                                   'a row: '):
            disc.read(Path('manifest.txt').read_bytes(), Path('manifest.txt'), images())

    def test_fails_on_an_image_whose_file_the_manifest_doesn_t_name(self):
        files = {path: data for path, data in WHOLE.items() if path != 'build/gcc/SONICADV/STGA.PRS'}
        with tree({**GAME_TREE, **files, **published(files=files)}), \
                self.assertRaisesRegex(SystemExit, ': stga: the manifest names no build/gcc/SONICADV/STGA.PRS, its '
                                                   'file on the disc$'):
            disc.replacements(disc.read(Path('manifest.txt').read_bytes(), Path('manifest.txt'), images()).images)


class Limits(unittest.TestCase):
    def test_holds_a_prs_file_to_the_work_area_and_its_image_to_its_interval(self):
        # the work area holds 0x200000 bytes, and stga's interval 0x280000 from 0x0c900000
        for size, end, problems in ((0x200000, 0x0CB80000, []),
                                    (0x200001, 0x0CB80001, ['build/gcc/SONICADV/STGA.PRS: takes 0x200800 bytes in '
                                                            'whole sectors, and the work area holds 0x200000',
                                                            'stga: runs to 0x0cb80001, past its interval, which ends '
                                                            'at 0x0cb80000'])):
            with self.subTest(size=hex(size)), tree({**GAME_TREE, **FILES, 'build/gcc/SONICADV/STGA.PRS': bytes(size)}):
                found, held = disc.limits({'stga': built(0x0C900000, 0x280000, end),
                                           'ev0001': built(0x0CB80000, 0x80000, 0x0CB80010)}, memory.contract(images()))
                self.assertEqual(found, problems)
        self.assertEqual(held['SONICADV/EV0001.PRS'], 'sectors 0x800 of the work area 0x200000, image 0x10 of its '
                                                      'interval 0x80000')

    def test_fails_on_an_image_that_ends_before_its_origin(self):
        with tree({**GAME_TREE, **FILES}):
            found, _ = disc.limits({'stga': built(0x0C900000, 0x280000, 0x0C8FFFFF),
                                    'ev0001': built(0x0CB80000, 0x80000, 0x0CB80000)}, memory.contract(images()))
        self.assertEqual(found, ['stga: ends at 0x0c8fffff, before its origin'])

    def test_fails_on_an_image_the_manifest_gives_another_origin_or_length_than_the_contract(self):
        with tree({**GAME_TREE, **FILES}):
            found, _ = disc.limits({'stga': built(0x0C910000, 0x280000, 0x0C910010),
                                    'ev0001': built(0x0CB80000, 0x7F000, 0x0CB80010)}, memory.contract(images()))
        self.assertEqual(found, ['stga: the manifest gives it 0x0c910000 and 0x280000 bytes, and the memory contract '
                                 '0x0c900000 and 0x280000',
                                 'ev0001: the manifest gives it 0x0cb80000 and 0x7f000 bytes, and the memory contract '
                                 '0x0cb80000 and 0x80000'])

    def test_holds_a_bin_file_its_loader_reads_whole_in_sectors_to_its_image_s_interval(self):
        # 1ST_READ.BIN's interval, made to end 4 bytes past its file's first sector: the image fits it, and the
        # second sector its loader reads runs past it
        with tree({**GAME_TREE, 'build/gcc/1ST_READ.BIN': bytes(0x801)}):
            terms = memory.contract(images())
            terms = terms._replace(lengths={**terms.lengths, '1st_read': 0x804})
            found, _ = disc.limits({'1st_read': built(0x8C010000, 0x804, 0x8C010801)}, terms)
        self.assertEqual(found, ['build/gcc/1ST_READ.BIN: takes 0x1000 bytes in whole sectors, and its interval holds '
                                 '0x804'])
        # the sectors are the memory contract's: in sectors of 0x1000 bytes, 0x1001 bytes take two, which an interval of
        # 0x1804 bytes doesn't hold, and in the disc's 0x800 would
        with tree({**GAME_TREE, 'build/gcc/1ST_READ.BIN': bytes(0x1001)}):
            terms = memory.contract(images())
            terms = terms._replace(lengths={**terms.lengths, '1st_read': 0x1804}, sector=0x1000)
            found, _ = disc.limits({'1st_read': built(0x8C010000, 0x1804, 0x8C011001)}, terms)
        self.assertEqual(found, ['build/gcc/1ST_READ.BIN: takes 0x2000 bytes in whole sectors, and its interval holds '
                                 '0x1804'])


# build/gdi, stood in: map prints the map beside the image, and extract copies each path from the files beside it
GDI = ('#!/bin/sh\ncase $1 in\nmap) cat "$2.map" ;;\nextract) image=$2 dir=$3; shift 3\n    for path; do mkdir -p '
       '"$dir/$(dirname "$path")" && cp "$image.files/$path" "$dir/$path" || exit 1; done ;;\nesac\n')


# the files the memory contract gives a size by hand, as the original disc's map gives them
SIZED = """\
  45034      15      29580  SONICADV/SS_MES_S.BIN
  45049       5       9652  SONICADV/MR_MES_S.BIN
  45054       2       2680  SONICADV/PAST_MES_K.BIN
  45056      14      27423  SONICADV/KNUCKLES.PRS
  45070     122     248920  SONICADV/EV_TR2_BIG.PRS
  45192      33      67183  SONICADV/AMY.PRS
  45225      19      37906  SONICADV/MILES.PRS
  45244     148     301560  SONICADV/EROBO.PB
  45392     145     295156  SONICADV/EROBO.NB
  45537     151     308288  SONICADV/E101R.PB
  45688      89     180920  SONICADV/E101R.NB
"""
# GAME_TREE's disc, as build/gdi map prints it: a file for each image, the boot file in two sectors, and SIZED
PORT_MAP = """\
  45000      16             system area
  45016       2             volume descriptors
  45018       1       2048  SONICADV/
  45019       1          9  SONICADV/EV0001.PRS
  45020       1          9  SONICADV/STGA.PRS
  45021       1          9  SONICADV/STGB.PRS
  45022      10             unreferenced
  45032       2       4096  1ST_READ.BIN (boot)
""" + SIZED
# build/gdi, stood in again: rebuild copies the disc, its map and its files to the directory it names, each file a
# --replace gives in its place
REBUILDING = GDI.replace('esac\n', 'rebuild) image=$2 out=$3; shift 3; mkdir -p "$out" && cp "$image" "$image.map" "$out/" '
                                   '&& cp -r "$image.files" "$out/" || exit 1\n'
                                   '    while [ $# -gt 0 ]; do [ "$1" = --replace ] && '
                                   '{ cp "$3" "$out/$(basename "$image").files/$2" || exit 1; }; shift 3; done ;;\nesac\n')


def original(gdi=REBUILDING, files=None, written=None):
    """GAME_TREE with files over it, and build/gdi stood in by gdi. make gcc's files are WHOLE's, with written over
    them, and their manifest.txt. The original's .gdi, sa1.gdi, has its map, PORT_MAP, beside it, and a file of
    b'original' for each image's file the map lists."""
    written = {**WHOLE, **(written or {})}
    return {**GAME_TREE, **written, **published(files=written), 'build/gdi': gdi, 'sa1.gdi': '',
            'sa1.gdi.map': PORT_MAP, **{f'sa1.gdi.files/{path}': b'original' for path in (
                '1ST_READ.BIN', 'SONICADV/EV0001.PRS', 'SONICADV/STGA.PRS', 'SONICADV/STGB.PRS')}, **(files or {})}


class Built(unittest.TestCase):
    def test_writes_the_disc_with_the_manifest_s_hash_in_disc_txt(self):
        files = original()
        with tree(files) as root:
            (root / 'build/gdi').chmod(0o755)
            disc.build(Path('sa1.gdi'), Path('manifest.txt'), Path('disc'))
            lines = (root / 'disc/disc.txt').read_text().splitlines()
            self.assertEqual((root / 'disc/sa1.gdi.files/SONICADV/STGB.PRS').read_bytes(), b'stgb file')
        self.assertEqual(lines[1], f'manifest  {hashlib.sha256(files["manifest.txt"].encode()).hexdigest()}  '
                                   'manifest.txt')
        self.assertEqual([line.split()[1] for line in lines[2:]], ['1ST_READ.BIN', 'SONICADV/EV0001.PRS',
                                                                   'SONICADV/STGA.PRS', 'SONICADV/STGB.PRS'])

    def test_a_hangup_or_term_stops_build_gdi_and_leaves_no_disc(self):
        # disc.py's command line, with a rebuild that holds once it makes disc.partial, until the signal stops disc.py,
        # which stops build/gdi too
        holding = REBUILDING.replace('rebuild) image=$2', 'rebuild) mkdir "$3" && echo $$ > held.pid && '
                                                          'mv held.pid held && exec sleep 30; image=$2')
        for number, status in ((signal.SIGHUP, 129), (signal.SIGTERM, 143)):
            with self.subTest(number.name), tree(original(holding)) as root:
                (root / 'build/gdi').chmod(0o755)
                building = subprocess.Popen([sys.executable, str(ROOT / 'tools/disc.py'), 'sa1.gdi', 'manifest.txt',
                                             'disc'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                while not Path('held').exists() and building.poll() is None:
                    time.sleep(0.01)
                building.send_signal(number)
                self.assertEqual(building.wait(10), status)
                with self.assertRaises(ProcessLookupError):
                    os.kill(int(Path('held').read_text()), 0)
                self.assertFalse((root / 'disc').exists() or (root / 'disc.partial').exists())

    def test_gives_disc_txt_the_hash_of_the_manifest_s_bytes_it_read(self):
        # the rebuild stood in changes the manifest as it runs, once disc.py has read and checked it
        changing = REBUILDING.replace('rebuild) image=$2', 'rebuild) echo "# another" >> manifest.txt; image=$2')
        files = original(changing)
        with tree(files) as root:
            (root / 'build/gdi').chmod(0o755)
            disc.build(Path('sa1.gdi'), Path('manifest.txt'), Path('disc'))
            lines = (root / 'disc/disc.txt').read_text().splitlines()
        self.assertEqual(lines[1], f'manifest  {hashlib.sha256(files["manifest.txt"].encode()).hexdigest()}  '
                                   'manifest.txt')

    def test_fails_on_a_source_that_changes_once_the_manifest_s_hash_is_read(self):
        # the rebuild stood in writes STGB.PRS anew before it copies it: the disc holds the file's new bytes, which
        # aren't the ones whose hash the manifest gives
        racing = REBUILDING.replace('rebuild) image=$2', 'rebuild) printf "stgb anew" > build/gcc/SONICADV/STGB.PRS; '
                                                         'image=$2')
        with tree(original(racing)) as root:
            (root / 'build/gdi').chmod(0o755)
            with self.assertRaisesRegex(SystemExit, ": SONICADV/STGB.PRS: the disc's bytes aren't "
                                                    "build/gcc/SONICADV/STGB.PRS's, whose hash the manifest gives$"):
                disc.build(Path('sa1.gdi'), Path('manifest.txt'), Path('disc'))
            self.assertEqual((root / 'build/gcc/SONICADV/STGB.PRS').read_bytes(), b'stgb anew')
            self.assertFalse((root / 'disc').exists() or (root / 'disc.partial').exists())

    def test_writes_a_partial_disc_it_can_t_remove_to_stderr_and_keeps_the_failure_that_stopped_the_run(self):
        # the source changes as the rebuild runs, so the check fails, and then disc.partial's disc can't be removed
        racing = REBUILDING.replace('rebuild) image=$2', 'rebuild) printf "stgb anew" > build/gcc/SONICADV/STGB.PRS; '
                                                         'image=$2')
        rmtree = shutil.rmtree

        def removing(path, *args, **kwargs):
            if Path(path) == Path('disc.partial') and not (args or kwargs):
                raise PermissionError(errno.EACCES, os.strerror(errno.EACCES), 'disc.partial/sa1.gdi')
            rmtree(path, *args, **kwargs)

        with tree(original(racing)) as root:
            (root / 'build/gdi').chmod(0o755)
            with mock.patch.object(disc.shutil, 'rmtree', removing), \
                    contextlib.redirect_stderr(io.StringIO()) as printed, \
                    self.assertRaisesRegex(SystemExit, ": SONICADV/STGB.PRS: the disc's bytes aren't "
                                                       "build/gcc/SONICADV/STGB.PRS's, whose hash the manifest gives$"):
                disc.build(Path('sa1.gdi'), Path('manifest.txt'), Path('disc'))
            self.assertRegex(printed.getvalue(), ': disc.partial/sa1.gdi: Permission denied\n$')

    def test_fails_on_a_build_gdi_it_can_t_run_or_that_fails_and_on_a_disc_it_can_t_rename(self):
        # a build/gdi that isn't there, and one that prints a byte that isn't UTF-8 and fails
        with tree({}), self.assertRaisesRegex(SystemExit, ": build/gdi: can't run it: No such file or directory$"):
            disc.gdi('map', 'sa1.gdi')
        with tree({'build/gdi': '#!/bin/sh\nprintf "\\377 no room\\n"\nexit 1\n'}) as root:
            (root / 'build/gdi').chmod(0o755)
            with self.assertRaisesRegex(SystemExit, ': \ufffd no room\n.*: build/gdi map failed$'):
                disc.gdi('map', 'sa1.gdi')

        def refused(source, target):
            raise OSError(errno.EIO, os.strerror(errno.EIO), str(source))

        with tree(original()) as root:
            (root / 'build/gdi').chmod(0o755)
            with mock.patch.object(disc.os, 'replace', refused), \
                    self.assertRaisesRegex(SystemExit, ': disc.partial: Input/output error$'):
                disc.main(['sa1.gdi', 'manifest.txt', 'disc'])
            self.assertFalse((root / 'disc').exists() or (root / 'disc.partial').exists())

    def test_fails_on_a_file_the_rebuild_didn_t_replace_and_leaves_no_disc(self):
        # the rebuild stood in skips STGB.PRS's --replace, so the disc keeps its original bytes
        skipping = REBUILDING.replace('[ "$1" = --replace ] && ', '[ "$1" = --replace ] && [ "$2" != SONICADV/STGB.PRS ] && ')
        with tree(original(skipping)) as root:
            (root / 'build/gdi').chmod(0o755)
            with self.assertRaisesRegex(SystemExit, ": SONICADV/STGB.PRS: the disc's bytes aren't "
                                                    "build/gcc/SONICADV/STGB.PRS's, whose hash the manifest gives$"):
                disc.build(Path('sa1.gdi'), Path('manifest.txt'), Path('disc'))
            self.assertFalse((root / 'disc').exists() or (root / 'disc.partial').exists())

    def test_places_a_file_that_grows_where_the_plan_puts_it(self):
        # STGB.PRS takes 2 sectors, where it held 1, so it moves to the unreferenced run's first. The rebuild stood in
        # records its arguments and leaves the map as it was, which check_rebuilt() refuses, so the test leaves
        # check_rebuilt() to Rebuilt and reads the --place and disc.txt
        keeping = REBUILDING.replace('rebuild) image=$2', 'rebuild) printf "%s\\n" "$@" > rebuild.txt; image=$2')
        with tree(original(keeping, written={'build/gcc/SONICADV/STGB.PRS': bytes(2049)})) as root, \
                mock.patch.object(disc, 'check_rebuilt', lambda *args: []):
            (root / 'build/gdi').chmod(0o755)
            disc.build(Path('sa1.gdi'), Path('manifest.txt'), Path('disc'))
            argv = (root / 'rebuild.txt').read_text().splitlines()
            lines = (root / 'disc/disc.txt').read_text().splitlines()
        self.assertIn(['--place', 'SONICADV/STGB.PRS', '45022'], [argv[at:at + 3] for at in range(len(argv))])
        self.assertIn('file  SONICADV/STGB.PRS  2049  2  45022  ', [line[:len('file  SONICADV/STGB.PRS  2049  2  45022  ')]
                                                                    for line in lines])

    def test_fails_on_a_disc_directory_that_exists_and_removes_nothing(self):
        # DIR names the directory of the original's files, then a disc a run before wrote, beside the partial one of a
        # run that stopped
        for out, held in (('sa1.gdi.files', ['sa1.gdi.files']), ('disc', ['disc', 'disc.partial'])):
            left = {'disc/disc.txt': 'a disc', 'disc.partial/sa1.gdi': ''}
            with self.subTest(out), tree(original(files=left)) as root:
                (root / 'build/gdi').chmod(0o755)
                files = {path: path.read_bytes() for path in root.rglob('*') if path.is_file()}
                with self.assertRaisesRegex(SystemExit, '\n.*'.join(f": {each}: exists, and disc.py removes nothing it "
                                                                    "didn't write" for each in held) + '$'):
                    disc.build(Path('sa1.gdi'), Path('manifest.txt'), Path(out))
                self.assertEqual({path: path.read_bytes() for path in root.rglob('*') if path.is_file()}, files)

    def test_fails_on_a_file_the_memory_contract_sizes_otherwise_and_leaves_no_disc(self):
        # the original disc's SS_MES_S.BIN holds a byte more than MESSAGES gives, and it holds no E101R.NB
        mapped = PORT_MAP.replace('29580  SONICADV/SS_MES_S.BIN', '29581  SONICADV/SS_MES_S.BIN').replace(
            '  45688      89     180920  SONICADV/E101R.NB\n', '')
        with tree(original(files={'sa1.gdi.map': mapped})) as root:
            (root / 'build/gdi').chmod(0o755)
            with self.assertRaisesRegex(SystemExit, ': SONICADV/SS_MES_S.BIN: holds 29581 bytes on the disc, and the '
                                                    'memory contract gives it 29580\n.*: SONICADV/E101R.NB: no file of '
                                                    'the disc, and the memory contract gives it 180920 bytes$'):
                disc.build(Path('sa1.gdi'), Path('manifest.txt'), Path('disc'))
            self.assertFalse((root / 'disc').exists() or (root / 'disc.partial').exists())

    def test_fails_on_a_file_past_its_limit_and_leaves_no_disc(self):
        # STGA.PRS's sectors overrun the work area loader_read_prs reads it to
        with tree(original(written={'build/gcc/SONICADV/STGA.PRS': bytes(0x200001)})) as root:
            (root / 'build/gdi').chmod(0o755)
            with self.assertRaisesRegex(SystemExit, ': build/gcc/SONICADV/STGA.PRS: takes 0x200800 bytes in whole '
                                                    'sectors, and the work area holds 0x200000$'):
                disc.build(Path('sa1.gdi'), Path('manifest.txt'), Path('disc'))
            self.assertFalse((root / 'disc').exists() or (root / 'disc.partial').exists())


class Rebuilt(unittest.TestCase):
    def check(self, rebuilt, written, hashed=None):
        """The problems check_rebuilt() finds with a disc whose map is rebuilt and whose files hold written, by path,
        against MAP. The build replaced A.BIN with a.bin, which the plan moves to 45039, and B.PRS with b.prs, which
        keeps its place. The manifest gives each the hash of its file's bytes, or of the bytes hashed gives it."""
        sources = {'SONICADV/A.BIN': Path('a.bin'), 'SONICADV/B.PRS': Path('b.prs')}
        hashes = {path: hashlib.sha256(data).hexdigest() for path, data in
                  (hashed or {'a.bin': b'new a', 'b.prs': b'new b'}).items()}
        files = {'build/gdi': GDI, 'a.bin': b'new a', 'b.prs': b'new b', 'disc/sa1.gdi': '',
                 'disc/sa1.gdi.map': rebuilt, **{f'disc/sa1.gdi.files/{path}': data for path, data in written.items()}}
        with tree(files) as root:
            (root / 'build/gdi').chmod(0o755)
            found = disc.check_rebuilt(Path('disc/sa1.gdi'), Path('check'), disc.file_runs(disc.regions(MAP, 'map')),
                                       sources,
                                 {'SONICADV/A.BIN': 45039}, hashes)
            self.assertFalse((root / 'check').exists())
            return found

    def test_takes_a_disc_whose_replaced_files_hold_their_sources_where_the_plan_puts_them(self):
        rebuilt = MAP.replace('  45019      10      20480  SONICADV/A.BIN\n', '').replace(
            '  45039      30             unreferenced\n', '  45039       1          5  SONICADV/A.BIN\n')
        written = {'SONICADV/A.BIN': b'new a', 'SONICADV/B.PRS': b'new b'}
        self.assertEqual(self.check(rebuilt, written), [])
        self.assertEqual(self.check(rebuilt, {**written, 'SONICADV/B.PRS': b'old b'}),
                         ["SONICADV/B.PRS: the disc's bytes aren't b.prs's, whose hash the manifest gives"])
        # b.prs holds the disc's bytes, and the manifest's hash gave it others
        self.assertEqual(self.check(rebuilt, written, {'a.bin': b'new a', 'b.prs': b'old b'}),
                         ["SONICADV/B.PRS: the disc's bytes aren't b.prs's, whose hash the manifest gives"])

    def test_fails_on_a_check_directory_it_can_t_remove(self):
        rmtree = shutil.rmtree

        def removing(path, *args, **kwargs):
            if Path(path) == Path('check') and not (args or kwargs):
                raise PermissionError(errno.EACCES, os.strerror(errno.EACCES), 'check/SONICADV')
            rmtree(path, *args, **kwargs)

        with mock.patch.object(disc.shutil, 'rmtree', removing), \
                self.assertRaisesRegex(SystemExit, ': check/SONICADV: Permission denied$'):
            self.check(MAP, {'SONICADV/A.BIN': b'new a', 'SONICADV/B.PRS': b'new b'})

    def test_fails_on_a_check_directory_it_didn_t_make_and_keeps_it(self):
        with tree({'build/gdi': GDI, 'check/mine.txt': 'mine'}) as root:
            (root / 'build/gdi').chmod(0o755)
            with self.assertRaisesRegex(SystemExit, ': check: File exists$'):
                disc.check_rebuilt(Path('disc/sa1.gdi'), Path('check'), disc.file_runs(disc.regions(MAP, 'map')),
                                   {}, {}, {})
            self.assertEqual((root / 'check/mine.txt').read_text(), 'mine')

    def test_fails_where_a_file_the_disc_keeps_moved_or_changed_size(self):
        rebuilt = MAP.replace('  45019      10      20480  SONICADV/A.BIN\n', '').replace(
            '  45033       6      12288  SONICADV/KEPT.BIN\n  45039      30             unreferenced\n',
            '  45033       6          5  SONICADV/A.BIN\n  45039       6      12288  SONICADV/KEPT.BIN\n')
        written = {'SONICADV/A.BIN': b'new a', 'SONICADV/B.PRS': b'new b'}
        self.assertEqual(self.check(rebuilt, written), [
            'SONICADV/A.BIN: starts at 45033, and the plan puts it at 45039',
            'SONICADV/KEPT.BIN: a file the disc keeps, at 45033, 6 sectors and 12288 bytes, lies at 45039 with 6 '
            'sectors and 12288 bytes'])
        # where it lay, with another size
        rebuilt = MAP.replace('  45019      10      20480  SONICADV/A.BIN\n', '').replace(
            '  45033       6      12288  SONICADV/KEPT.BIN\n  45039      30             unreferenced\n',
            '  45033       5      10240  SONICADV/KEPT.BIN\n  45039       1          5  SONICADV/A.BIN\n')
        self.assertEqual(self.check(rebuilt, written), [
            'SONICADV/KEPT.BIN: a file the disc keeps, at 45033, 6 sectors and 12288 bytes, lies at 45033 with 5 '
            'sectors and 10240 bytes'])


if __name__ == '__main__':
    unittest.main()
