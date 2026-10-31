import contextlib
import errno
import hashlib
import io
import os
import signal
import subprocess
import sys
import time
import unittest
from pathlib import Path
from unittest import mock

import elf
import gcclink
import ld
import manifest
import memory
import publish
import spec
import test_ld
import test_spec
from config import Config
from images import images
from support import GAME_TREE, cwd, tree
from tables import aligned, hexa

ROOT = Path(__file__).resolve().parents[1]
# a stand-in for build/prskit, whose compress writes the .PRS as a copy of the .BIN, and whose decompress copies the
# .PRS back
PRSKIT = '#!/bin/sh\ncat "$2" > "$3"\n'
# SHT_PROGBITS, the type of a section that holds bytes
PROGBITS = 1


def section(index, name, address, data=b'', size=None, kind=PROGBITS, flags=elf.ALLOC):
    """An allocated section, as elf.read() reads one: PROGBITS with data, or NOBITS of size."""
    return elf.Section(index, name, kind, flags, address, len(data) if size is None else size, 1, data)


def symbol(name, value, index=elf.ABSOLUTE, bind=elf.GLOBAL):
    """A symbol of the C name name, as elf.read() reads one, with value in the section at index, which defaults to
    elf.ABSOLUTE."""
    return elf.Symbol(f'_{name}', value, 0, index, 0, bind)


def linked(*sections, symbols=()):
    """An ELF, as elf.read() reads one, of sections and symbols, the null section first."""
    return elf.Elf('final.elf', [elf.Section(0, '', 0, 0, 0, 0, 0, b''), *sections], list(symbols), {})


# an image's code, its bss and a record after it, as a link lays them out from 0x0c900000
CODE = section(1, '.text', 0x0C900000, b'\x09\x00\x0b\x00')
BSS = section(2, '.bss', 0x0C900004, size=8, kind=elf.NOBITS)
TAIL = section(3, '.tail', 0x0C90000C, b'\x0c\x00\x90\x0c')
IMAGE = gcclink.Image('stga', 0x0C900000, 0x1000, None, [], {}, {}, {}, [], [], [], [])


def startup(*ranges):
    """An ELF with 0x100 bytes of bss at 0x8c010100, and a startup's tables, whose words hold ranges, pairs of bounds,
    in order: B_BGN to B_END, BSG_BGN to BSG_END, D_BGN to D_END and DSG_BGN to DSG_END."""
    words = b''.join(value.to_bytes(4, 'little') for pair in ranges for value in pair)
    names = ('_B_BGN', '_B_END', '_BSG_BGN', '_BSG_END', '_D_BGN', '_D_END', '_DSG_BGN', '_DSG_END')
    return linked(section(1, 'C', 0x8C010000, words), section(2, '.bss', 0x8C010100, size=0x100, kind=elf.NOBITS),
                  symbols=[symbol(name, 0x8C010000 + 4 * index, 1) for index, name in enumerate(names)])


class Payloads(unittest.TestCase):
    def test_writes_the_bytes_through_the_last_that_a_section_holds(self):
        # an image whose loader or startup zeroes its bss, or whose original holds none, ends with its code
        self.assertEqual(publish.payload(IMAGE, linked(CODE, BSS), False), CODE.data)

    def test_holds_the_bss_as_zeros_where_the_file_holds_it_or_a_record_follows_it(self):
        self.assertEqual(publish.payload(IMAGE, linked(CODE, BSS), True), CODE.data + bytes(8))
        self.assertEqual(publish.payload(IMAGE, linked(CODE, BSS, TAIL), False), CODE.data + bytes(8) + TAIL.data)

    def test_holds_the_bss_by_each_loader_s_contract(self):
        # _INITSCT zeroes 1ST_READ.BIN's and LOADER.BIN's bss, so their files hold none. event_load zeroes an event's
        # bss up to 0x0cbc8000. EV0002's bss ends at 0x0cba6398, so its file holds none, and EV0034's runs on to
        # 0x0cbfe9e4, so its file holds it. SUMMARY's original holds its bss ahead of its constants, and STG01's ahead
        # of its fish models, so their files hold it. STG02's original ends where its bss starts, so its file holds none
        with cwd(ROOT):
            terms = memory.contract(images())
            ends = {name: Config(name).span()[1] for name in ('1st_read', 'loader', 'ev0002', 'ev0034', 'summary',
                                                              'stg01', 'stg02')}
            self.assertEqual({name: publish.holds(name, terms, end) for name, end in ends.items()},
                             {'1st_read': False, 'loader': False, 'ev0002': False, 'ev0034': True, 'summary': True,
                              'stg01': True, 'stg02': False})


    def test_fails_on_a_section_outside_the_image_s_interval_and_on_two_that_overlap(self):
        # IMAGE's interval runs 0x1000 bytes from 0x0c900000. A section that ends where it starts, or starts where it
        # ends, lies outside it, and one that starts where another ends overlaps none
        for sections, problem in (
                ((section(4, '.below', 0x0C8FFFFC, bytes(4)), CODE),
                 ".below runs from 0x0c8ffffc to 0x0c900000, outside stga's interval, 0x0c900000 to 0x0c901000"),
                ((CODE, section(4, '.past', 0x0C901000, size=4, kind=elf.NOBITS)),
                 ".past runs from 0x0c901000 to 0x0c901004, outside stga's interval, 0x0c900000 to 0x0c901000"),
                ((CODE, section(4, '.over', 0x0C900002, b'BB')),
                 '.over starts at 0x0c900002, inside .text, which ends at 0x0c900004')):
            with self.subTest(problem), self.assertRaisesRegex(SystemExit, f': final.elf: {problem}$'):
                publish.payload(IMAGE, linked(*sections), True)
        self.assertEqual(publish.payload(IMAGE, linked(CODE, section(4, '.next', 0x0C900004, b'BB')), False),
                         CODE.data + b'BB')


class Lookups(unittest.TestCase):
    def test_reads_a_whole_word_and_fails_on_one_its_section_s_bytes_don_t_hold(self):
        tables = section(1, 'C', 0x8C010000, b'\x01\x02\x03\x04\x7f')
        bss = section(2, '.bss', 0x8C010100, size=8, kind=elf.NOBITS)

        def at(value, index=1):
            return linked(tables, bss, symbols=[symbol('name', value, index)])

        self.assertEqual(publish.word(at(0x8C010000), 'name'), 0x04030201)
        self.assertIsNone(publish.word(at(0x8C010100, 2), 'name'))
        self.assertIsNone(publish.word(linked(tables), 'name'))
        # a byte left of the section's, none, a word before it, and the index ELF gives a common symbol
        for value, index in ((0x8C010004, 1), (0x8C010005, 1), (0x8C00FFFC, 1), (0x8C010000, 0xFFF2)):
            with self.subTest(value=hex(value), index=index), self.assertRaisesRegex(
                    SystemExit, f": final.elf: _name, at {hexa(value)}, names no whole word of a section's bytes$"):
                publish.word(at(value, index), 'name')

    def test_fails_on_a_name_defined_twice_and_takes_an_undefined_one_for_none(self):
        # a local _image_end ahead of the script's, and one the link leaves undefined
        twice = linked(CODE, symbols=[symbol('image_end', 0x0C900008, 1, elf.LOCAL), symbol('image_end', 0x0C900010)])
        for lookup in (publish.bound, publish.word, publish.value):
            with self.subTest(lookup.__name__), \
                    self.assertRaisesRegex(SystemExit, ': final.elf: defines _image_end 2 times$'):
                lookup(twice, 'image_end')
        undefined = linked(symbols=[symbol('image_end', 0, elf.UNDEFINED)])
        self.assertIsNone(publish.bound(undefined, 'image_end'))
        with self.assertRaisesRegex(SystemExit, ': final.elf: defines no _image_end$'):
            publish.value(undefined, 'image_end')


class Startup(unittest.TestCase):
    def test_takes_a_startup_that_zeroes_every_byte_of_bss_and_copies_nothing(self):
        found = startup((0x8C010100, 0x8C010180), (0x8C010180, 0x8C010200), (0x8C010000, 0x8C010000),
                        (0x8C010000, 0x8C010000))
        self.assertEqual(publish.startup_problems(found), [])

    def test_fails_on_bss_its_ranges_leave_out_and_on_a_copy(self):
        found = startup((0x8C010100, 0x8C010180), (0x8C0101C0, 0x8C010200), (0x8C010000, 0x8C010004),
                        (0x8C010000, 0x8C010000))
        self.assertEqual(publish.startup_problems(found), [
            '_INITSCT leaves .bss from 0x8c010180 unzeroed: B through B32 run 0x8c010100 to 0x8c010180 and BSG '
            'through BSG32 0x8c0101c0 to 0x8c010200',
            '_INITSCT copies _D_BGN to _D_END, which a GCC link holds empty: 0x8c010000 to 0x8c010004'])
        found = startup((0x8C010100, 0x8C010180), (0x8C010180, 0x8C010200), (0x8C010000, 0x8C010000),
                        (0x8C010000, 0x8C010008))
        self.assertEqual(publish.startup_problems(found), [
            '_INITSCT copies _DSG_BGN to _DSG_END, which a GCC link holds empty: 0x8c010000 to 0x8c010008'])
        # tables without the copies' words
        found = found._replace(symbols=found.symbols[:4])
        self.assertEqual(publish.startup_problems(found), [
            f'_INITSCT copies {start} to {end}, which a GCC link holds empty: absent'
            for start, end in (('_D_BGN', '_D_END'), ('_DSG_BGN', '_DSG_END'))])


    def test_fails_on_a_range_it_zeroes_past_the_bss_or_backward(self):
        # B through B32 from the tables' words, which hold bytes, BSG through BSG32 to the top of the address space,
        # and B through B32 backward, ahead of BSG through BSG32, which holds the bss whole
        empty = ((0x8C010000, 0x8C010000), (0x8C010000, 0x8C010000))
        for ranges, problem in (
                (((0x8C010000, 0x8C010180), (0x8C010180, 0x8C010200)),
                 '_INITSCT zeroes 0x8c010000, which no bss section holds: B through B32 run 0x8c010000 to 0x8c010180'),
                (((0x8C010100, 0x8C010180), (0x8C010180, 0xFFFFFFFF)),
                 '_INITSCT zeroes 0x8c010200, which no bss section holds: BSG through BSG32 run 0x8c010180 to '
                 '0xffffffff'),
                (((0x8C010200, 0x8C010100), (0x8C010100, 0x8C010200)),
                 '_INITSCT zeroes B through B32 backward, from 0x8c010200 to 0x8c010100')):
            with self.subTest(problem):
                self.assertEqual(publish.startup_problems(startup(*ranges, *empty)), [problem])


class Sequence(unittest.TestCase):
    def test_takes_each_section_near_the_end_of_the_one_before_it_as_mpvasec_s_within_does(self):
        # within takes 0 to 3 or 8 to 11 bytes apart for near, either way. The check reads the bounds of each section's
        # run in the link, __startof_ and __endof_, which mpvasec.src's tables hold. It holds each pair to within, the
        # last pair among them, by their distance, not the bits the two addresses differ in: 0x8ceb01fe and 0x8ceb0200
        # are near
        def bounds(gap=0, last=0, end=0x8CEB0200):
            return {'PSGSFD00': (0x8CEB0000, 0x8CEB0080), 'PSGSFD01': (0x8CEB0080, 0x8CEB0100),
                    'PSGSFD02': (0x8CEB0100 + gap, end), 'PSGSFD03': (0x8CEB0200 + last, 0x8CEB0300)}

        far = ('{} starts at {}, and {} ends at {}, {} bytes apart: MPVASEC_CheckSectionSeq fails MPV_Init unless '
               'the two lie 0 to 3 or 8 to 11 bytes apart').format
        for case, given, problems in (
                ('next', bounds(), []), ('8 apart', bounds(8), []), ('2 back', bounds(last=-2), []),
                ('4 apart', bounds(4), [far('PSGSFD02', '0x8ceb0104', 'PSGSFD01', '0x8ceb0100', 4)]),
                ('12 apart', bounds(12), [far('PSGSFD02', '0x8ceb010c', 'PSGSFD01', '0x8ceb0100', 12)]),
                ('12 apart, the last pair', bounds(last=12),
                 [far('PSGSFD03', '0x8ceb020c', 'PSGSFD02', '0x8ceb0200', 12)]),
                ('2 apart, unaligned', bounds(end=0x8CEB01FE), [])):
            with self.subTest(case):
                symbols = [symbol(f'_{edge}of_{name}', value)
                           for name, (start, end) in given.items() for edge, value in (('start', start), ('end', end))]
                self.assertEqual(publish.sequence_problems(linked(symbols=symbols)), problems)


class Toolchain(unittest.TestCase):
    def test_takes_the_toolchain_s_one_cc1_and_fails_on_none_or_several(self):
        cc1 = 'sh-elf/libexec/gcc/sh-elf/16.2.0/cc1'
        for files, problem in (({'sh-elf/bin/sh-elf-gcc': ''}, 'no cc1, which the toolchain installs'),
                               ({cc1: '', 'sh-elf/libexec/gcc/sh-elf/16.1.0/cc1': ''},
                                'holds 2 cc1, .*/16.1.0/cc1, .*/16.2.0/cc1, and make gcc\'s toolchain installs one')):
            with self.subTest(problem), tree(files) as root, mock.patch.object(gcclink, 'SH_ELF', root / 'sh-elf'), \
                    self.assertRaisesRegex(SystemExit, f': {root}/sh-elf/libexec/gcc/sh-elf/: {problem}$'):
                publish.compiler()
        with tree({cc1: ''}) as root, mock.patch.object(gcclink, 'SH_ELF', root / 'sh-elf'):
            self.assertEqual(publish.compiler(), root / cc1)


class Published(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        test_spec.Generator.setUpClass()
        cls.objects = test_spec.Generator.objects

    @contextlib.contextmanager
    def linked(self, prskit=PRSKIT, objects=None):
        """GAME_TREE with stga and ev0001 linked under build/gcc/link/, from spec.py's spec cut to their rows, as
        1ST_READ.BIN's head takes an object of strt1.src's, which GAME_TREE lacks, and publish's memory contract cut to
        the two, so that its check is of each image the spec holds. prskit stands in for build/prskit, and ev0001's
        script takes stage_flag from stga and ev0001_tally, a second name for its count. objects replace or add to the
        tree's GCC objects, by path."""
        files = {'config/ev0001/symbols.txt': GAME_TREE['config/ev0001/symbols.txt'] + '0x0c900010  stage_flag\n',
                 'config/imports.txt': (test_spec.PROVIDER
                                        + 'ev0001  stage_flag  stga  stage_flag  0x0c  the event plays on the beach\n'
                                        'ev0001  ev0001_tally  ev0001  ev0001_count  -  the count under a second '
                                        'name\n'),
                 'build/gcc/events/ev0001/script.o': test_spec.Generator.tallying}
        with tree({**GAME_TREE, **self.objects, **files, **(objects or {}), 'prskit': prskit}) as root, \
                mock.patch.object(gcclink, 'SH_ELF', test_ld.TOOLCHAIN), \
                mock.patch.object(publish, 'PRSKIT', root / 'prskit'), \
                mock.patch.object(publish, 'images', lambda: ['ev0001', 'stga']):
            (root / 'prskit').chmod(0o755)
            Path('build/gcc/link').mkdir(parents=True)
            spec.main(['build/gcc/link/spec.txt'])
            rows = Path('build/gcc/link/spec.txt').read_text().splitlines(True)
            Path('build/gcc/link/spec.txt').write_text(''.join(row for row in rows if row.startswith('#')
                                                               or row.split()[1] in ('stga', 'ev0001')))
            ld.main(['build/gcc/link/spec.txt', 'build/gcc/link'])
            yield root

    def test_writes_each_image_s_file_and_then_the_manifest(self):
        with self.linked() as root:
            publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
            lines = (root / 'manifest.txt').read_text().splitlines()
            # tools/manifest.py's reader takes it whole, and its writer writes it again byte for byte
            written = (root / 'manifest.txt').read_bytes()
            self.assertEqual(manifest.text(manifest.recorded(written, 'manifest.txt')).encode(), written)
            files = {line.split()[3]: line.split()[2] for line in lines if line.startswith('file ')}
            for path in ('build/gcc/SONICADV/STGA.BIN', 'build/gcc/SONICADV/STGA.PRS', 'build/gcc/SONICADV/EV0001.BIN',
                         'build/gcc/link/ev0001/final.elf'):
                self.assertEqual(files[path], hashlib.sha256((root / path).read_bytes()).hexdigest(), path)
            inputs = [(name, hashlib.sha256(Path(path).read_bytes()).hexdigest(), path)
                      for name, image in gcclink.read('build/gcc/link/spec.txt').items() for path in image.inputs]
            event = (root / 'build/gcc/SONICADV/EV0001.BIN').read_bytes()
            found = elf.read((root / 'build/gcc/link/ev0001/final.elf').read_bytes(), 'final.elf')
            stga = elf.read((root / 'build/gcc/link/stga/final.elf').read_bytes(), 'final.elf')
        self.assertIn(f'image    stga  0x0c900000  0x00280000  {hexa(publish.bound(stga, "image_end"))}', lines)
        # the tools, prskit among them, and the spec
        self.assertEqual({Path(line.split()[2]).name for line in lines if line.startswith('tool ')},
                         {'sh-elf-gcc', 'cc1', 'sh-elf-as', 'sh-elf-ld', 'prskit'})
        self.assertIn('spec.txt', [Path(line.split()[2]).name for line in lines if line.startswith('spec ')])
        # each image's inputs, in link order
        self.assertEqual([tuple(line.split()[1:]) for line in lines if line.startswith('input ')], inputs)
        # the import binds where stga's final link defines stage_flag, and the alias where ev0001's defines its count
        flag, count = gcclink.exports(stga)['stage_flag'], gcclink.exports(found)['ev0001_count']
        self.assertIn(f'bind     ev0001  stage_flag  stga  stage_flag  {hexa(flag.value)}', lines)
        self.assertIn(f'alias    ev0001  ev0001_tally  ev0001_count  {hexa(count.value)}', lines)
        # the event's head opens its file, which holds no bss: event_load zeroes it
        head = next(symbol for symbol in found.symbols if symbol.name == '_ev0001_head')
        bss = next(section for section in found.sections if section.name == '.bss')
        self.assertEqual(head.value, 0x0CB80000)
        self.assertEqual(len(event), bss.address - 0x0CB80000)

    def test_fails_where_the_manifest_would_take_the_place_of_a_file_it_reads_or_writes_and_removes_nothing(self):
        # the spec, an input, a final link and a file publish writes, and a spec that the .partial file beside the
        # manifest links to
        with self.linked() as root:
            publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
            Path('manifest.txt.partial').symlink_to(root / 'build/gcc/link/spec.txt')
            files = {path: path.read_bytes() for path in root.rglob('*') if path.is_file()}
            taken = gcclink.read('build/gcc/link/spec.txt')['stga'].inputs[0]
            for manifest_, problem in (('build/gcc/link/spec.txt', 'build/gcc/link/spec.txt'), (taken, taken),
                                      ('build/gcc/link/stga/final.elf', 'build/gcc/link/stga/final.elf'),
                                      ('build/gcc/SONICADV/STGA.BIN', 'build/gcc/SONICADV/STGA.BIN'),
                                      ('manifest.txt', 'manifest.txt.partial')):
                with self.subTest(manifest_), self.assertRaisesRegex(
                        SystemExit, f": {problem}: a file publish reads or writes, which the manifest can't take the "
                                    'place of$'):
                    publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', manifest_])
                self.assertEqual({path: path.read_bytes() for path in root.rglob('*') if path.is_file()}, files)

    def test_fails_on_an_image_of_the_spec_config_doesn_t_describe_and_removes_nothing(self):
        with self.linked() as root:
            publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
            spec = Path('build/gcc/link/spec.txt')
            spec.write_text('image     zz  0x0c900000  0x00001000\n' + spec.read_text())
            files = {path: path.read_bytes() for path in root.rglob('*') if path.is_file()}
            with self.assertRaisesRegex(SystemExit, ": build/gcc/link/spec.txt: links zz, an image config/ doesn't "
                                                    'describe$'):
                publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
            self.assertEqual({path: path.read_bytes() for path in root.rglob('*') if path.is_file()}, files)

    def test_a_manifest_that_can_t_take_its_name_leaves_none(self):
        def refused(source, target):
            raise OSError(errno.EIO, os.strerror(errno.EIO))

        with self.linked() as root:
            with mock.patch.object(publish.os, 'replace', refused), \
                    self.assertRaisesRegex(SystemExit, ': manifest.txt: Input/output error$'):
                publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
            self.assertEqual(sorted(path.name for path in root.glob('manifest.txt*')), [])
            # and where the partial manifest can't be removed either, the rename's failure is the run's
            unlink = Path.unlink

            def unlinking(path, missing_ok=False):
                if path.name == 'manifest.txt.partial' and path.exists():
                    raise PermissionError(errno.EACCES, os.strerror(errno.EACCES), str(path))
                unlink(path, missing_ok)

            with mock.patch.object(publish.os, 'replace', refused), mock.patch.object(Path, 'unlink', unlinking), \
                    contextlib.redirect_stderr(io.StringIO()) as printed, \
                    self.assertRaisesRegex(SystemExit, ': manifest.txt: Input/output error$'):
                publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
            self.assertRegex(printed.getvalue(), ': manifest.txt.partial: Permission denied\n$')

    def test_holds_a_startup_s_image_and_movie_bin_to_their_tables(self):
        # stga's link, taken for an image strt1.src's tables start, gives none of their words, and taken for
        # MOVIE.BIN, none of the bounds MPVASEC_CheckSectionSeq reads
        for patched, problem in ((mock.patch.object(memory, 'STARTUP', ('stga',)),
                                  "stga: its startup's tables give no _B_BGN or _B_END"),
                                 (mock.patch.object(publish, 'MOVIE', Path('config/stga')),
                                  'stga: its link gives no bounds for PSGSFD00 or PSGSFD01, which '
                                  'MPVASEC_CheckSectionSeq reads')):
            with self.subTest(problem), self.linked(), patched, self.assertRaisesRegex(SystemExit, f'(?m): {problem}$'):
                publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])

    def test_holds_each_prs_file_and_the_work_past_an_image_to_the_memory_contract(self):
        # the stand-in for prskit writes each .PRS as its .BIN and 2 MB and a byte more, which the work area doesn't
        # hold, and decompresses it to the .BIN
        large = ('#!/bin/sh\ncase $1 in compress) { cat "$2"; head -c 2097153 /dev/zero; } > "$3";; '
                 '*) head -c -2097153 "$2" > "$3";; esac\n')
        with self.linked(large), self.assertRaisesRegex(SystemExit, '(?m): stga: its PRS file takes 0x200800 bytes '
                                                                    'in whole sectors, and the work area '
                                                                    'loader_read_prs reads it to holds 0x200000$'):
            publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
        # stga's stage_work_end points to work past its end, as Zero's eggmanrobo_bss_end does to its banks: the
        # checks take the work from there, rounded up to 32, as long as the banks' longer file, EROBO.PB, in whole
        # sectors. ev0001, which memory.WORK doesn't name, has none. memory.TAILS makes a.c's .data stga's tail, which
        # the link puts in .tail, where the body ends. ev0001's file ends at its bss, short of its end and of its last
        # sector's
        stga = {'build/gcc/modules/stga/a.o': test_spec.compiled(
            GAME_TREE['modules/stga/a.c'] + '\nchar *stage_work_end = (char *)0x8ccf0004;\n')}
        extents = {}

        def check(terms, found):
            extents.update(found)
            return []

        with mock.patch.dict(memory.WORK, {'stga': ('stage_work_end', 'EROBO')}), \
                mock.patch.dict(memory.TAILS, {'stga': (('DTAIL', 'modules/stga/a.c', '.data'),)}), \
                self.linked(objects=stga) as root, mock.patch.object(memory, 'check', check):
            publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
            tail = next(section for section in elf.read((root / 'build/gcc/link/stga/final.elf').read_bytes(),
                                                        'final.elf').sections if section.name == '.tail')
            event = len((root / 'build/gcc/SONICADV/EV0001.BIN').read_bytes())
        self.assertEqual(extents['stga'].work, (0x8CCF0020, 0x8CCF0020 + 0x4A000))
        self.assertIsNone(extents['ev0001'].work)
        self.assertEqual(extents['stga'].body, tail.address)
        self.assertLess(extents['stga'].body, extents['stga'].end)
        self.assertEqual((extents['ev0001'].stored, extents['ev0001'].sectors),
                         (0x0CB80000 + event, 0x0CB80000 + aligned(event, 0x800)))
        self.assertLess(extents['ev0001'].stored, extents['ev0001'].end)
        self.assertNotEqual(extents['ev0001'].stored, extents['ev0001'].sectors)

    def test_tries_to_remove_each_file_and_keeps_the_failure_that_stopped_the_run(self):
        # STGA.BIN can't be removed once write_images() writes it. The check fails the run, which then removes the other
        # files, writes STGA.BIN's failure to stderr, and raises the check's
        unlink, stuck = Path.unlink, Path('build/gcc/SONICADV/STGA.BIN')

        def unlinking(path, missing_ok=False):
            if path in (stuck, Path('manifest.txt')) and path.exists():
                raise PermissionError(errno.EACCES, os.strerror(errno.EACCES), str(path))
            unlink(path, missing_ok)

        with self.linked() as root:
            with mock.patch.object(memory, 'check', lambda terms, extents: ['stga: a problem']), \
                    mock.patch.object(Path, 'unlink', unlinking), \
                    contextlib.redirect_stderr(io.StringIO()) as printed, \
                    self.assertRaisesRegex(SystemExit, ': stga: a problem$'):
                publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
            self.assertEqual(sorted((root / 'build/gcc/SONICADV').glob('*')), [root / stuck])
            self.assertRegex(printed.getvalue(), ': build/gcc/SONICADV/STGA.BIN: Permission denied\n$')
            # a run that can't remove the manifest a run before wrote removes each file it writes, then fails
            stuck.unlink()
            publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
            with mock.patch.object(Path, 'unlink', unlinking), \
                    self.assertRaisesRegex(SystemExit, ': manifest.txt: Permission denied\n.*: '
                                                       'build/gcc/SONICADV/STGA.BIN: Permission denied$'):
                publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
            self.assertEqual(sorted((root / 'build/gcc/SONICADV').glob('*')), [root / stuck])

    def test_a_hangup_or_term_stops_build_prskit_and_leaves_no_file(self):
        # publish.py's command line, with a build/prskit that holds the compress of ev0001, the first image, until the
        # signal stops publish.py, which stops build/prskit too
        for number, status in ((signal.SIGHUP, 129), (signal.SIGTERM, 143)):
            with self.subTest(number.name), self.linked() as root:
                Path('build/prskit').write_text('#!/bin/sh\necho $$ > held.pid && mv held.pid held\nexec sleep 30\n')
                Path('build/prskit').chmod(0o755)
                publishing = subprocess.Popen([sys.executable, str(ROOT / 'tools/publish.py'),
                                               'build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'],
                                              env=dict(os.environ, SH_ELF=str(test_ld.TOOLCHAIN)),
                                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                while not Path('held').exists() and publishing.poll() is None:
                    time.sleep(0.01)
                publishing.send_signal(number)
                self.assertEqual(publishing.wait(10), status)
                with self.assertRaises(ProcessLookupError):
                    os.kill(int(Path('held').read_text()), 0)
                self.assertEqual(list((root / 'build/gcc/SONICADV').glob('*')), [])
                self.assertFalse((root / 'manifest.txt').exists())

    def test_fails_on_a_prs_file_build_prskit_fails_to_write(self):
        # the stand-in for prskit writes the .PRS and then fails
        failing = '#!/bin/sh\ncp "$2" "$3"\necho no room\nexit 1\n'
        with self.linked(failing), self.assertRaisesRegex(SystemExit, r'(?m): no room\n.*: ev0001: \S+/prskit '
                                                                      'compress failed$'):
            publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
        # and where what it prints isn't UTF-8
        failing = '#!/bin/sh\nprintf "\\377 no room\\n"\nexit 1\n'
        with self.linked(failing), self.assertRaisesRegex(SystemExit, '(?m): \ufffd no room\n.*: ev0001: \\S+/prskit '
                                                                      'compress failed$'):
            publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])

    def test_fails_on_a_prs_file_that_doesn_t_decompress_to_its_bin(self):
        # the stand-in for prskit decompresses a .PRS to its .BIN and more, and then fails to decompress it. Each run
        # fails at ev0001, the first image, and leaves no file and no manifest
        more = '#!/bin/sh\ncase $1 in compress) cat "$2" > "$3";; *) { cat "$2"; echo more; } > "$3";; esac\n'
        refused = ('#!/bin/sh\ncase $1 in compress) cat "$2" > "$3";; *) echo "prskit: $2: a copy reaches back past the '
                   'start of the output at 0x0" >&2; exit 1;; esac\n')
        for prskit, problem in (
                (more, r'ev0001: build/gcc/SONICADV/EV0001\.PRS decompresses to other bytes than '
                       r'build/gcc/SONICADV/EV0001\.BIN'),
                (refused, r'prskit: /dev/stdin: a copy reaches back past the start of the output at 0x0\n'
                          r'.*: ev0001: \S+/prskit decompress of build/gcc/SONICADV/EV0001\.PRS failed')):
            with self.subTest(problem), self.linked(prskit) as root:
                with self.assertRaisesRegex(SystemExit, f'(?m): {problem}$'):
                    publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
                self.assertEqual(list((root / 'build/gcc/SONICADV').glob('*')), [])
                self.assertFalse((root / 'manifest.txt').exists())

    def test_hashes_each_final_link_and_file_from_the_bytes_its_checks_read(self):
        # stga's final link changes once write_images() reads it, as the memory check runs: the manifest gives the hash
        # of the bytes publish checked, which the file no longer holds
        def changing(terms, extents):
            final = Path('build/gcc/link/stga/final.elf')
            checked.append(final.read_bytes())
            final.write_bytes(checked[0] + b'more')
            return []

        checked = []
        with self.linked() as root, mock.patch.object(memory, 'check', changing):
            publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
            rows = [line.split() for line in (root / 'manifest.txt').read_text().splitlines()]
        self.assertIn(['file', 'stga', hashlib.sha256(checked[0]).hexdigest(), 'build/gcc/link/stga/final.elf'], rows)

    def test_a_failed_check_leaves_no_manifest_and_no_file(self):
        with self.linked() as root:
            publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
            with mock.patch.object(memory, 'check', lambda terms, extents: ['stga: a problem']), \
                    self.assertRaisesRegex(SystemExit, ': stga: a problem$'):
                publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
            self.assertFalse((root / 'manifest.txt').exists())
            self.assertEqual(list((root / 'build/gcc/SONICADV').glob('*')), [])

    def test_a_name_the_final_link_doesn_t_define_leaves_no_manifest_and_no_file(self):
        # a constant of ev0001's that the spec gives once ld.py has linked it
        with self.linked() as root:
            publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
            spec = Path('build/gcc/link/spec.txt')
            spec.write_text(spec.read_text() + 'constant  ev0001  pad_spare  0x8c5f0004\n')
            with self.assertRaisesRegex(SystemExit, ': build/gcc/link/ev0001/final.elf: defines no _pad_spare$'):
                publish.main(['build/gcc/link/spec.txt', 'build/gcc/link', 'manifest.txt'])
            self.assertFalse((root / 'manifest.txt').exists())
            self.assertEqual(list((root / 'build/gcc/SONICADV').glob('*')), [])


if __name__ == '__main__':
    unittest.main()
