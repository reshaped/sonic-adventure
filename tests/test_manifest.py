import hashlib
import unittest
from pathlib import Path

from manifest import Built, Manifest, recorded, text
from support import tree

# the spec of a link of two images, as tools/gcclink.py reads one: ev0001 takes stage_flag from stga, names its count
# ev0001_tally too, and binds pad_state to an address
SPEC = '''\
image     stga  0x0c900000  0x00280000
input     stga  build/dc/modules/stga/a.o
image     ev0001  0x0cb80000  0x00080000
input     ev0001  build/dc/events/ev0001/head.o
input     ev0001  build/libgcc.a
import    ev0001  stage_flag  stga  stage_flag  0x0c
alias     ev0001  ev0001_tally  ev0001_count
constant  ev0001  pad_state  0x8c5f0000
'''
# the tools a manifest gives the hashes of, in order: make dc's toolchain's, and build/prskit
TOOLS = {'tools/sh-elf/bin/sh-elf-gcc': b'gcc',
         **{f'tools/sh-elf/libexec/gcc/sh-elf/16.2.0/{name}': name.encode()
            for name in ('cc1', 'lto1', 'lto-wrapper', 'collect2', 'liblto_plugin.so')},
         'tools/sh-elf/bin/sh-elf-as': b'as', 'tools/sh-elf/bin/sh-elf-ld': b'ld', 'build/prskit': b'prskit'}
# stga's link-time object, which tools/ld.py's link-time compile wrote for a.o, and the record of that compile:
# ev0001's link ran none
COMPILED = 'build/dc/link/stga/lto/build/dc/modules/stga/a.o'
RECORD = 'build/dc/link/stga/lto/objects.txt'
# the files the link's manifest names: the tools, the spec, the inputs, stga's link-time object, and each image's
# final link and the files tools/publish.py writes for it. The record is no file of the manifest's
FILES = {**TOOLS, 'build/dc/link/spec.txt': SPEC.encode(), 'build/dc/modules/stga/a.o': b'a', COMPILED: b'a whole',
         'build/dc/events/ev0001/head.o': b'head', 'build/libgcc.a': b'libgcc',
         'build/dc/link/stga/final.elf': b'stga elf', 'build/dc/SONICADV/STGA.BIN': b'stga image',
         'build/dc/SONICADV/STGA.PRS': b'stga file', 'build/dc/link/ev0001/final.elf': b'ev0001 elf',
         'build/dc/SONICADV/EV0001.BIN': b'ev0001 file'}
LINKED = Manifest(
    {path: hashlib.sha256(data).hexdigest() for path, data in FILES.items()}, list(TOOLS),
    'build/dc/link/spec.txt',
    {'stga': Built(0x0C900000, 0x280000, 0x0C900010, ['build/dc/modules/stga/a.o'], [COMPILED], {}, {}, {},
                   ['build/dc/link/stga/final.elf', 'build/dc/SONICADV/STGA.BIN', 'build/dc/SONICADV/STGA.PRS']),
     'ev0001': Built(0x0CB80000, 0x80000, 0x0CB80010, ['build/dc/events/ev0001/head.o', 'build/libgcc.a'], [],
                     {'stage_flag': ('stga', 'stage_flag', 0x0C900040)}, {'ev0001_tally': ('ev0001_count', 0x0CB80020)},
                     {'pad_state': 0x8C5F0000}, ['build/dc/link/ev0001/final.elf', 'build/dc/SONICADV/EV0001.BIN'])})
WRITTEN = text(LINKED)


def read(written, files=None):
    """The Manifest recorded() reads from written, a manifest's text, as manifest.txt, in a tree of FILES and RECORD
    with files over them."""
    with tree({**FILES, RECORD: f'build/dc/modules/stga/a.o {COMPILED}\n', **(files or {})}):
        return recorded(written.encode(), Path('manifest.txt'))


class Written(unittest.TestCase):
    def test_writes_each_row_in_its_form_and_reads_back_what_it_wrote(self):
        lines = WRITTEN.splitlines()
        digest = hashlib.sha256(b'ev0001 elf').hexdigest()
        self.assertTrue(lines[0].startswith('# '))
        self.assertEqual([Path(path).name for path in TOOLS],
                         ['sh-elf-gcc', 'cc1', 'lto1', 'lto-wrapper', 'collect2', 'liblto_plugin.so', 'sh-elf-as',
                          'sh-elf-ld', 'prskit'])
        self.assertEqual(lines[1:11], [*(f'tool     {LINKED.hashes[path]}  {path}' for path in TOOLS),
                                      f'spec     {LINKED.hashes["build/dc/link/spec.txt"]}  build/dc/link/spec.txt'])
        for row in (f'compiled stga  {hashlib.sha256(b"a whole").hexdigest()}  {COMPILED}',
                    'image    ev0001  0x0cb80000  0x00080000  0x0cb80010',
                    'bind     ev0001  stage_flag  stga  stage_flag  0x0c900040',
                    'alias    ev0001  ev0001_tally  ev0001_count  0x0cb80020', 'constant ev0001  pad_state  0x8c5f0000',
                    f'file     ev0001  {digest}  build/dc/link/ev0001/final.elf'):
            self.assertIn(row, lines)
        self.assertEqual(read(WRITTEN), LINKED)

    def test_takes_a_file_two_images_share_where_its_rows_give_one_hash(self):
        shared, spec = 'build/dc/modules/stga/a.o', SPEC + 'input     ev0001  build/dc/modules/stga/a.o\n'
        found = LINKED._replace(
            hashes={**LINKED.hashes, 'build/dc/link/spec.txt': hashlib.sha256(spec.encode()).hexdigest()},
            images={**LINKED.images, 'ev0001': LINKED.images['ev0001']._replace(
                inputs=['build/dc/events/ev0001/head.o', 'build/libgcc.a', shared])})
        self.assertEqual(read(text(found), {'build/dc/link/spec.txt': spec}), found)

    def test_writes_no_row_whose_field_its_form_doesn_t_take(self):
        found = LINKED._replace(tools=['build/a tool'], hashes={**LINKED.hashes, 'build/a tool': '0' * 64})
        with self.assertRaisesRegex(SystemExit, f': tool {"0" * 64} build/a tool: not a row of the form tool DIGEST '
                                                'PATH$'):
            text(found)


class Read(unittest.TestCase):
    def assertRefused(self, written, problem, files=None):
        with self.assertRaisesRegex(SystemExit, f'(?m): {problem}$'):
            read(written, files)

    def test_fails_on_a_line_that_isn_t_a_row_spelled_as_the_manifest_spells_it(self):
        # a kind of no form; a field too many; a field of no form, an address without 0x or in capitals; fields
        # parted otherwise; a comment after a row; a blank line; and a last line with no newline
        image = 'image    stga  0x0c900000  0x00280000  0x0c900010\n'
        line = WRITTEN.splitlines(True).index(image) + 1
        for spelled in ('fiel     stga  0x0c900000  0x00280000  0x0c900010\n',
                        'image    stga  0x0c900000  0x00280000  0x0c900010  0x0c900010\n',
                        'image    stga  8c900000  0x00280000  0x0c900010\n',
                        'image    stga  0x0C900000  0x00280000  0x0c900010\n',
                        'image stga 0x0c900000 0x00280000 0x0c900010\n',
                        'image    stga  0x0c900000  0x00280000  0x0c900010  # stga\n', '\n'):
            with self.subTest(spelled):
                self.assertRefused(WRITTEN.replace(image, spelled), f'manifest.txt:{line}: not a row: tool DIGEST PATH, '
                                                                    '.*, or file NAME DIGEST PATH, spelled as '
                                                                    'tools/manifest.py writes it')
        self.assertRefused(WRITTEN[:-1], f'manifest.txt:{len(WRITTEN.splitlines())}: not a row: .*')

    def test_fails_on_a_row_of_an_image_its_image_row_doesn_t_head(self):
        lines = WRITTEN.splitlines(True)
        stga, ev0001 = (next(at for at, line in enumerate(lines) if line.split()[:2] == fields)
                        for fields in (['file', 'stga'], ['image', 'ev0001']))
        # a file row of stga's below ev0001's image row, and an input row of ev0001's above it
        moved = lines[:stga] + lines[stga + 1:] + [lines[stga]]
        self.assertRefused(''.join(moved), f'manifest.txt:{len(moved)}: stga: not the image of the image row above it')
        moved = lines[:ev0001] + [lines[ev0001 + 1], lines[ev0001]] + lines[ev0001 + 2:]
        self.assertRefused(''.join(moved), f'manifest.txt:{ev0001 + 1}: ev0001: not the image of the image row above '
                                           'it')

    def test_fails_on_a_second_spec_or_image_row_and_a_name_an_image_binds_twice(self):
        lines = WRITTEN.splitlines(True)
        spec = next(line for line in lines if line.startswith('spec '))
        self.assertRefused(WRITTEN.replace(spec, spec * 2), f'manifest.txt:{lines.index(spec) + 2}: a second spec row')
        self.assertRefused(WRITTEN + 'image    stga  0x0c900000  0x00280000  0x0c900010\n',
                           f'manifest.txt:{len(lines) + 1}: stga has an image row already')
        self.assertRefused(WRITTEN + 'constant ev0001  stage_flag  0x0c900040\n',
                           f'manifest.txt:{len(lines) + 1}: ev0001 binds stage_flag already')

    def test_fails_on_a_path_whose_rows_give_it_two_hashes(self):
        # the second row's hash is the file's, and the first's isn't
        file = f'file     ev0001  {LINKED.hashes["build/dc/SONICADV/EV0001.BIN"]}  build/dc/SONICADV/EV0001.BIN\n'
        self.assertRefused(WRITTEN.replace(file, file.replace(LINKED.hashes['build/dc/SONICADV/EV0001.BIN'], '0' * 64)
                                           + file),
                           f'manifest.txt:{len(WRITTEN.splitlines()) + 1}: build/dc/SONICADV/EV0001.BIN: a row above '
                           'gives it another hash')

    def test_fails_on_link_time_objects_that_aren_t_the_ones_the_record_beside_the_final_link_gives(self):
        # the image links each link-time object the record names, so a manifest that leaves one out, names one more
        # or names them in another order doesn't give the hash of each object whose code the image holds. A link
        # with no compile leaves no record, and its manifest no compiled row
        row = next(line for line in WRITTEN.splitlines(True) if line.startswith('compiled '))
        other = row.replace(COMPILED, f'{COMPILED[:-3]}b.o')
        refused = (f'manifest.txt: gives stga other link-time objects than {RECORD} records; make dc links again '
                   'and writes both')
        both = f'build/dc/modules/stga/a.o {COMPILED}\nbuild/dc/modules/stga/b.o {COMPILED[:-3]}b.o\n'
        files = {f'{COMPILED[:-3]}b.o': b'a whole'}
        for case, written, record in (('one left out', WRITTEN.replace(row, ''), None),
                                      ('one more', WRITTEN.replace(row, row + other), None),
                                      ('another order', WRITTEN.replace(row, other + row), both),
                                      ('no record', WRITTEN, ''),
                                      ('an object GCC kept nothing of', WRITTEN, 'build/dc/modules/stga/a.o -\n')):
            with self.subTest(case):
                self.assertRefused(written, refused, {**files, **({} if record is None else {RECORD: record})})
        self.assertEqual(read(WRITTEN.replace(row, row + other), {**files, RECORD: both}).images['stga'].compiled,
                         [COMPILED, f'{COMPILED[:-3]}b.o'])
        self.assertEqual(read(WRITTEN.replace(row, ''), {RECORD: 'build/dc/modules/stga/a.o -\n'}).images['stga'].compiled,
                         [])
        # a row of the record that is none fails as tools/gcclink.py's taken() does, and a link-time object whose
        # hash isn't the row's as any file does
        self.assertRefused(WRITTEN, f'{RECORD}:1: not a row: .*', {RECORD: 'a.o\n'})
        self.assertRefused(WRITTEN, f'manifest.txt: {COMPILED}: no file with the hash the manifest gives it',
                           {COMPILED: b'another'})

    def test_fails_on_a_manifest_without_its_tools_in_order_its_spec_or_an_image_s_final_link(self):
        lines = WRITTEN.splitlines(True)
        gcc, cc1 = (next(line for line in lines if line.rstrip().endswith(name)) for name in ('/sh-elf-gcc', '/cc1'))
        final = next(line for line in lines if line.rstrip().endswith('/stga/final.elf'))
        event = next(line for line in lines if line.rstrip().endswith('/ev0001/final.elf'))
        rest = 'lto1, lto-wrapper, collect2, liblto_plugin.so, sh-elf-as, sh-elf-ld, prskit'
        wanted = f'not sh-elf-gcc, cc1, {rest}, in order'
        for written, problem in (
                (''.join(line for line in lines if not line.startswith('tool ')),
                 f'its tool rows name none, {wanted}'),
                (''.join(line for line in lines if not line.rstrip().endswith(('/lto1', '/liblto_plugin.so'))),
                 f'its tool rows name sh-elf-gcc, cc1, lto-wrapper, collect2, sh-elf-as, sh-elf-ld, prskit, {wanted}'),
                (WRITTEN.replace(cc1, ''),
                 f'its tool rows name sh-elf-gcc, {rest}, {wanted}'),
                (WRITTEN.replace(gcc + cc1, cc1 + gcc),
                 f'its tool rows name cc1, sh-elf-gcc, {rest}, {wanted}'),
                (''.join(line for line in lines if not line.startswith('spec ')), 'holds no spec row'),
                (WRITTEN.replace(final, ''), 'gives stga no final link, stga/final.elf, as its first file row'),
                (WRITTEN.replace(event, '') + event,
                 'gives ev0001 no final link, ev0001/final.elf, as its first file row')):
            with self.subTest(problem):
                self.assertRefused(written, f'manifest.txt: {problem}')

    def test_fails_on_a_file_whose_hash_isn_t_the_manifest_s(self):
        # a tool's, the spec's, an input's and a file's, and one the manifest names that is missing
        for path in ('build/prskit', 'build/dc/link/spec.txt', 'build/libgcc.a', 'build/dc/SONICADV/STGA.PRS'):
            with self.subTest(path):
                self.assertRefused(WRITTEN, f'manifest.txt: {path}: no file with the hash the manifest gives it',
                                   {path: b'another'})
        with tree({path: data for path, data in FILES.items() if path != 'build/libgcc.a'}), \
                self.assertRaisesRegex(SystemExit, '(?m): manifest.txt: build/libgcc.a: no file with the hash the '
                                                   'manifest gives it$'):
            recorded(WRITTEN.encode(), Path('manifest.txt'))

    def test_fails_on_images_that_aren_t_the_spec_s(self):
        # each manifest below gives every file its own hash, so it reaches the comparison with the spec
        def without(*fields):
            return ''.join(line for line in WRITTEN.splitlines(True) if line.split()[:len(fields)] != list(fields))

        def swapped(*fields):
            # the two rows that start with fields, exchanged
            lines = WRITTEN.splitlines(True)
            first, second = (at for at, line in enumerate(lines) if line.split()[:len(fields)] == list(fields))
            lines[first], lines[second] = lines[second], lines[first]
            return ''.join(lines)

        for written, problem in (
                (''.join(line for line in WRITTEN.splitlines(True) if line.split()[1] != 'stga'),
                 'names no image stga, which its spec links'),
                (without('input', 'ev0001', LINKED.hashes['build/libgcc.a']), 'gives ev0001 other inputs than its spec'),
                (swapped('input', 'ev0001'), 'gives ev0001 other inputs than its spec'),
                (WRITTEN.replace('stage_flag  stga  stage_flag', 'stage_flag  stgb  stage_flag'),
                 'gives ev0001 other imports than its spec'),
                (WRITTEN.replace('stga  stage_flag  0x', 'stga  stage_flags  0x'),
                 'gives ev0001 other imports than its spec'),
                (WRITTEN.replace('ev0001_count', 'ev0001_total'), 'gives ev0001 other aliases than its spec'),
                (without('bind'), 'gives ev0001 other imports than its spec'),
                (without('alias'), 'gives ev0001 other aliases than its spec'),
                (without('constant'), 'gives ev0001 other constants than its spec'),
                (WRITTEN.replace('0x0c900000  0x00280000', '0x0c910000  0x00280000'),
                 'gives stga another origin than its spec'),
                (WRITTEN.replace('0x0c900000  0x00280000', '0x0c900000  0x00270000'),
                 'gives stga another length than its spec'),
                (WRITTEN.replace('pad_state  0x8c5f0000', 'pad_state  0x8c5f0004'),
                 'gives ev0001 other constants than its spec')):
            with self.subTest(problem):
                self.assertRefused(written, f'manifest.txt: {problem}')
        final = f'file     stgb  {hashlib.sha256(b"stgb elf").hexdigest()}  build/dc/link/stgb/final.elf\n'
        self.assertRefused(WRITTEN + 'image    stgb  0x0c900000  0x00280000  0x0c900010\n' + final,
                           "manifest.txt: names image stgb, which its spec doesn't link",
                           {'build/dc/link/stgb/final.elf': b'stgb elf'})


if __name__ == '__main__':
    unittest.main()
