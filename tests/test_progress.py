import contextlib
import io
import os
import unittest
from collections import Counter
from pathlib import Path
from unittest import mock

import code_ranges
import progress
from config import Config
from support import LOADER_SECTIONS, MODULE, tree
from test_link import obj

# the routes the Makefile gives the report, none of them naming a source
REPORT = ['--patched', '--grafted', '--inline']
# the code of 1ST_READ.BIN but main.c's 16 bytes, MODULE's only placement there, and the sections that hold it
FIRST_CODE = [(0x8C010000, 0x8C010030), (0x8C0100A0, 0x8C0101A0), (0x8C0101B0, 0x8C11A580),
              (0x8C11A580, 0x8C1283CC), (0x8C6143E0, 0x8C6785EC)]
FIRST_SECTIONS = ['DSGLH', 'P', 'P', 'P', 'PSG']
SEAM = dict(MODULE, **{
    'config/mod/splits.txt': ('P  0x0c900000  0x0c900100\n'
                             'modules/mod/a.c  P  0x0c900000  0x0c900030\n'
                             'modules/mod/b.src  P  0x0c900040  0x0c900100\n'),
})
TAIL = dict(SEAM, **{
    'config/mod/splits.txt': ('P  0x0c900000  0x0c900100\n'
                             'D  0x0c900100  0x0c900200\n'
                             'modules/mod/a.c  P  0x0c900000  0x0c900030\n'
                             'modules/mod/b.src  P  0x0c900040  0x0c9000fc\n'
                             'modules/mod/a.c  D  0x0c900100  0x0c900140\n'),
})
# SEAM with main.c's code over FIRST_CODE too, so that its placements and fill write every image's code
WHOLE = dict(SEAM, **{'config/1st_read/splits.txt': ''.join(
    f'src/main.c  P  0x{start:08x}  0x{end:08x}\n' for start, end in FIRST_CODE + [(0x8C0101A0, 0x8C0101B0)])})


def left(config):
    # the code of config's image that its placements and fill leave unwritten, as (start, end)
    return [(start, end) for _, _, start, end in code_ranges.unwritten(config, code_ranges.writes(config, progress.section))]


class Progress(unittest.TestCase):
    def setUp(self):
        patch = mock.patch.multiple(progress, PROGRAMS=2, section=lambda source, section: (None, 32))
        patch.start()
        self.addCleanup(patch.stop)

    def test_counts_only_code_not_generated_data(self):
        files = dict(SEAM, **{
            'config/mod/splits.txt': ('P  0x0c900000  0x0c900100\n'
                                     'PSG  0x0c900100  0x0c900140\n'
                                     'D  0x0c900140  0x0c900200\n'
                                     'modules/mod/a.c  P  0x0c900000  0x0c900040\n'
                                     'modules/mod/a.c  D  0x0c900140  0x0c900180\n'
                                     'gen/mod/d_0c900180.c  D  0x0c900180  0x0c900200\n'
                                     'modules/mod/b.src  PSG  0x0c900100  0x0c900120\n'),
        })
        with tree(files):
            images = progress.survey()
            self.assertEqual(left(images[0]), FIRST_CODE)
            self.assertEqual(left(images[1]), [(0x0C900040, 0x0C900100), (0x0C900120, 0x0C900140)])

    def test_counts_linker_fill_between_objects(self):
        for alignment, expected in ((32, []), (4, [(0x0C900030, 0x0C900040)])):
            with self.subTest(alignment=alignment), tree(SEAM):
                progress.section = lambda source, section: (None, alignment)
                self.assertEqual(left(progress.survey()[1]), expected)

    def test_counts_fill_before_the_next_section(self):
        for alignment, expected in ((8, []), (4, [(0x0C9000FC, 0x0C900100)])):
            with self.subTest(alignment=alignment), tree(TAIL):
                progress.section = lambda source, section: (None, alignment if section == 'D' else 32)
                self.assertEqual(left(progress.survey()[1]), expected)

    def test_counts_fill_before_a_section_no_object_starts(self):
        # data from the disc starts D, and a.c's data stands in for its first object: on 8, D starts where b.src's
        # code ends, rounded up
        rows = TAIL['config/mod/splits.txt'].replace('modules/mod/a.c  D  0x0c900100  0x0c900140',
                                                    'gen/mod/d_0c900100.c  D  0x0c900100  0x0c900120\n'
                                                    'modules/mod/a.c  D  0x0c900120  0x0c900140')
        for alignment, expected in ((8, []), (4, [(0x0C9000FC, 0x0C900100)])):
            with self.subTest(alignment=alignment), tree(dict(TAIL, **{'config/mod/splits.txt': rows})):
                progress.section = lambda source, section: (None, alignment if section == 'D' else 32)
                self.assertEqual(left(progress.survey()[1]), expected)

    def test_counts_entry_code_but_not_library_handles(self):
        progress.PROGRAMS = 3
        files = dict(SEAM, **{
            'config/loader/splits.txt': ''.join(f'{row}\n' for row in LOADER_SECTIONS),
            'config/loader/symbols.txt': '',
            'sdk/shinobi/ov_main.c': '',
        })
        files['config/1st_read/splits.txt'] += 'sdk/shinobi/ov_main.c  DSGLH  0x8c010060  0x8c010080\n'
        with tree(files):
            images = progress.survey()
            self.assertEqual([left(config)[:2] for config in images[:2]],
                             [[(0x8C010000, 0x8C010030), (0x8C0100A0, 0x8C0101A0)],
                              [(0x8CE00000, 0x8CE00030), (0x8CE000A0, 0x8CE001A0)]])

    def test_requires_every_program(self):
        progress.PROGRAMS = 3
        with tree(SEAM), self.assertRaisesRegex(SystemExit, 'config/ has 2 images, and progress expects 3'):
            progress.survey()

    def test_routes_each_source_by_its_build(self):
        patched, grafted, inline = {'sdk/p.c', 'sdk/g.c'}, {'sdk/g.c'}, {'sdk/p.c', 'sdk/g.c', 'sdk/i.c'}
        self.assertEqual([code_ranges.route(source, patched, grafted, inline)
                          for source in ('src/a.c', 'sdk/p.c', 'sdk/g.c', 'sdk/i.c', 'sdk/s.src', 'gen/mod/d_0c900000.c')],
                         ['c', 'shckit', 'grafted', 'inline', 'assembly', 'generated'])

    def test_uncovered_ranges(self):
        regions = [(0x00, 0x10), (0x20, 0x30)]
        self.assertEqual(code_ranges.uncovered(regions, [(0x25, 0x40), (0x02, 0x04), (0x04, 0x06), (0x08, 0x22)]),
                         [(0x00, 0x02), (0x06, 0x08), (0x22, 0x25)])
        self.assertEqual(code_ranges.uncovered(regions, []), regions)
        self.assertEqual(code_ranges.uncovered(regions, [(0x00, 0x30)]), [])
        self.assertEqual(code_ranges.overlap(regions, [(0x08, 0x22)]), 10)

    def test_reports_without_writing(self):
        with tree(WHOLE) as root:
            before = {path: path.read_bytes() for path in root.rglob('*') if path.is_file()}
            with contextlib.redirect_stdout(io.StringIO()) as out:
                progress.main(REPORT)
            after = {path: path.read_bytes() for path in root.rglob('*') if path.is_file()}
            self.assertEqual(after, before)
            self.assertIn('Code by what writes it', out.getvalue())

    def test_reports_the_code_by_what_writes_it(self):
        # a.c is C with inline assembly that build/shckit patches, or that tools/graft.py grafts and build/shckit
        # patches; 1ST_READ.BIN's code is main.c's
        a = 'modules/mod/a.c'
        for argv, shckit, grafted in ((['--patched', a, '--grafted', '--inline', a], 48, 0),
                                      (['--patched', '--grafted', a, '--inline', a], 0, 48)):
            out = io.StringIO()
            with self.subTest(argv=argv):
                with tree(WHOLE), contextlib.redirect_stdout(out):
                    progress.main(argv)
                self.assertEqual(out.getvalue(), (
                    'Code by what writes it, 1,558,120 bytes in all; a source several binaries place counts in each:\n'
                    '  c          1,557,864  C that SHC compiles to an object\n'
                    f'  shckit     {shckit:>9}  C whose R10 assembly build/shckit patches\n'
                    f'  grafted    {grafted:>9}  C whose R10 assembly tools/graft.py grafts and build/shckit patches\n'
                    '                        48 of the shckit and grafted code bytes are of sources with inline assembly\n'
                    '  inline             0  C with inline assembly, which R10 compiles to assembly\n'
                    '  assembly         192  .src files\n'
                    "  fill              16  the linker's fill, in the gaps it leaves\n"))

    def test_fails_on_code_no_placement_or_fill_writes(self):
        # SEAM's 1ST_READ.BIN places main.c's 16 bytes alone: the report names each range of its code nothing writes,
        # with its section, and prints nothing
        with tree(SEAM), contextlib.redirect_stdout(io.StringIO()) as out, self.assertRaises(SystemExit) as raised:
            progress.main(REPORT)
        self.assertEqual([line.split(': ', 1)[1] for line in str(raised.exception).splitlines()],
                         [f'config/1st_read/splits.txt: no source or fill writes 0x{start:08x} to 0x{end:08x}, code in {section}'
                          for (start, end), section in zip(FIRST_CODE, FIRST_SECTIONS)])
        self.assertEqual(out.getvalue(), '')

    def test_fails_where_the_categories_would_not_sum_to_the_code(self):
        # a gap over a.c's first 16 bytes would count them twice
        with tree(SEAM):
            config = Config('mod')
            self.assertEqual(sum(code_ranges.written(config, progress.section, set(), set(), set())[0].values()), 256)
            gaps = [('P', 0x0C900000, 0x0C900010), ('P', 0x0C900030, 0x0C900040)]
            with mock.patch.object(Config, 'linker_fill', lambda config, section: gaps), \
                    self.assertRaisesRegex(SystemExit, r'config/mod/splits\.txt: the categories count 272 of the '
                                                       r"image's 256 code bytes"):
                code_ranges.written(config, progress.section, set(), set(), set())

    def test_wants_the_routes(self):
        for argv in ([], REPORT[:1], REPORT[:2], ['--write', *REPORT[:2]]):
            with self.subTest(argv=argv), tree(SEAM), self.assertRaises(SystemExit), \
                    contextlib.redirect_stderr(io.StringIO()) as err:
                progress.main(argv)
            self.assertIn('progress needs --patched, --grafted and --inline, as the Makefile gives them', err.getvalue())


# twice's body, which R10 copies into a.c's text at each call
BODY = ["        MOV.L       #H'00000001,R1", '        FADD        FR0,FR0']
# a.c: inner, with no tag and no call, whose one line stands in each copy of twice's body, and so counts as twice's;
# twice; and an open #pragma asm block
A_C = ('/* clang-format off */\n#ifndef __clang__\n#pragma inline_asm(inner)\nstatic void inner(void)\n{\n' + BODY[1] +
       '\n}\n/* @asm operand: fipr-registers, FADD FR0,FR0 at 0x0c900004 */\n'
       '#pragma inline_asm(twice)\nstatic void twice(void)\n{\n' + ''.join(f'{line}\n' for line in BODY) +
       '}\n/* @asm open: every instruction is one SHC writes for C */\n#pragma asm\n        SETT\n#pragma endasm\n'
       '#endif\n/* clang-format on */\n')
# b.src: a size an .EQU gives, which holds no bytes; a word ahead of its first function; its first function, whose tag
# gives a basis but not to the space it reserves; and an open one
B_SRC = """        .EXPORT     _b_first
        .EXPORT     _b_second
B_SPACE:        .EQU    4
        .SECTION    P,CODE,ALIGN=32
        .DATA.L     H'00000000
; @asm opcode: swap, SWAP.W R4,R0 at 0x0c900044
_b_first:
        SWAP.W      R4,R0
        RTS
        NOP
        .RES.B      B_SPACE
; @asm open: every instruction is one SHC writes for C
_b_second:
        RTS
        NOP
        .DATAB.B    174,0
        .END
"""
# each source's P, as its object declares it, all on 32
SIZES = {'modules/mod/a.c': 0x30, 'modules/mod/b.src': 0xC0}
# each source's object under build/, which declares its P
OBJECTS = {str(Path('build', source).with_suffix('.obj')): obj(size, 32) for source, size in SIZES.items()}
# WHOLE's mod, with a.c's inline bodies and block and b.src's two functions, and a README.md that holds the markers
# between text
NUMBERED = dict(WHOLE, **{'modules/mod/a.c': A_C, 'modules/mod/b.src': B_SRC,
                          'README.md': '# Title\n\nAbove.\n\n<!-- progress -->\nold\n<!-- /progress -->\n\nBelow.\n'})
# README.md's rows for WHOLE's two images: 1ST_READ.BIN's, and mod's as a stage
AREAS = (('Main executable', ('1st_read',)), (progress.SDK, None), ('Stages', ('mod',)))
# the routes the Makefile gives, with a.c's inline assembly
NUMBERS = ['--patched', '--grafted', '--inline', 'modules/mod/a.c']
# mod's 256 code bytes: a.c's 48, of which twice's two copies, 8, and their pool word, 4, are assembly with a basis, and
# its block's SETT, 2, is open assembly; the linker's 16 of fill; and b.src's 192, of which its first function's
# instructions, 6, have a basis, and the space it reserves, 4, has none. The row counts the 240 its sources compile to,
# the fill left out: C's 34 are 14.1%, and assembly is the rest, 85.9%. Of the 202 of assembly's instructions and data,
# its 206 less the space reserved, the 18 with a basis floor to 8.9%.
# 1ST_READ.BIN's code is main.c's C, so its rows have no assembly, 0%. All's C floors to 99.9%, so its 206 bytes of
# assembly, under a tenth, show as the rest, 0.1%
BLOCK = [
    "**100% of the game's code builds from source, byte for byte.**",
    '',
    '| | Binaries | Code bytes | C | Assembly |',
    '|---|---:|---:|---:|---:|',
    '| Main executable | 1 | 1,090,528 | 100.0% | 0% |',
    '| SDK libraries | | 467,336 | 100.0% | 0% |',
    '| Stages | 1 | 240 | 14.1% | 85.9% |',
    '| **All** | **2** | **1,558,104** | **99.9%** | **0.1%** |',
    '',
    'Most of the assembly was handwritten in the original, not compiled from C. Compiler analysis and original symbol '
    'names already provide [evidence](TAGS.md#assembly) of handwritten origins for 8.9% of its instructions and data.',
    '',
    'See [Accounting](TOOLCHAIN.md#accounting) for methodology.',
]


def listing(rows):
    """asmsh's listing of rows, each (offset or None, code, line or None, text), laid out as asmsh lays them out,
    between a page's header and the list that follows the rows."""
    lines = ['*** SuperH RISC engine ASSEMBLER Ver. 4.0 ***    10/23/26 09:10:48                  PAGE     1',
             'PROGRAM NAME =', '']
    for number, (offset, code, line, text) in enumerate(rows, 1):
        where = '' if offset is None else f'{offset:08X}'
        lines.append(f'{number:5} {where:8} {code:17}{"" if line is None else line:>5}     {text}'.rstrip())
    return '\r\n'.join(lines + ['  *****TOTAL ERRORS       0', '', '*** CROSS REFERENCE LIST', ''])


def a_listing(copies, loads=()):
    """a.c's listing: a NOP, copies of twice's body, an RTS and its slot, then with any copies a pool, aligned to 4, of
    a word the copies' MOV.L lines load, and the lines of loads with them; its block's SETT; and an .ALIGN and a .RES
    that fill P."""
    rows = [(0, '', 1, '          .SECTION    P,CODE,ALIGN=4'), (0, '0009', 2, '          NOP')]
    line, at, loading = 3, 2, list(loads)
    for _ in range(copies):
        rows += [(at, 'D101', line, BODY[0]), (at + 2, 'F000', line + 1, BODY[1])]
        loading.append(line)
        line, at = line + 2, at + 4
    rows += [(at, '000B', line, '          RTS'), (at + 2, '0009', line + 1, '          NOP')]
    if loading:
        rows += [(None, '', None, progress.BEGIN), (at + 4, '0000', None, 'ALIGNMENT CODE'),
                 (at + 6, '00000001', None, f'DATA FOR SOURCE-LINE {",".join(map(str, loading))}'),
                 (None, '', None, progress.END)]
    rows += [(at + 10 if loading else at + 4, '0018', line + 2, '        SETT'),
             (0x20, '', line + 3, '          .ALIGN      32'), (0x20, '00000010', line + 4, '          .RES.B      16')]
    return listing(rows)


def b_listing(source=B_SRC):
    """The listing of b.src's text source, by default B_SRC: each of its lines, with its bytes, and its .EQU's value."""
    placed = {3: (None, '00000004'), 4: (0, ''), 5: (0, '00000000'), 7: (4, ''), 8: (4, '6049'), 9: (6, '000B'),
              10: (8, '0009'), 11: (10, '00000004'), 13: (14, ''), 14: (14, '000B'), 15: (16, '0009'), 16: (18, '00')}
    return listing([(*placed.get(line, (None, '')), line, text)
                    for line, text in enumerate(source.split('\n')[:-1], 1)])


class Numbers(unittest.TestCase):
    def setUp(self):
        patch = mock.patch.multiple(progress, PROGRAMS=2, AREAS=AREAS, MEASURED={},
                                    section=lambda source, section: (SIZES.get(source), 32))
        patch.start()
        self.addCleanup(patch.stop)

    @contextlib.contextmanager
    def built(self, a=None, files=None):
        """NUMBERED's tree, with OBJECTS, a.c's listing a, by default a_listing(2), and b.src's, then files, a
        {path: text or bytes} map."""
        files = {**NUMBERED, **OBJECTS, 'build/modules/mod/a.lis': a or a_listing(2),
                 'build/modules/mod/b.lis': b_listing(), **(files or {})}
        with tree(files) as root:
            yield root

    def test_writes_the_numbers_between_the_markers_alone(self):
        # a.c counts the same in INLINE_C alone, and with PATCHED_C or GRAFTED_C too
        a = 'modules/mod/a.c'
        for routes in (NUMBERS, ['--patched', a, '--grafted', '--inline', a], ['--patched', '--grafted', a, '--inline', a]):
            with self.subTest(routes=routes), self.built():
                progress.MEASURED.clear()
                progress.main(['--write', *routes])
                self.assertEqual(Path('README.md').read_text(),
                                 '# Title\n\nAbove.\n\n<!-- progress -->\n' + ''.join(f'{line}\n' for line in BLOCK) +
                                 '<!-- /progress -->\n\nBelow.\n')

    def test_floors_c_and_gives_assembly_the_rest_with_the_fill_left_out(self):
        for counts, shown in ((Counter(c=10000, code=10016), ['100.0%', '0%']),
                              (Counter(c=9999, assembly=1, code=10016), ['99.9%', '0.1%']),
                              (Counter(c=9000, assembly=1000, code=10016), ['90.0%', '10.0%'])):
            with self.subTest(counts=counts):
                self.assertEqual(progress.shares(counts), shown)

    def test_fails_on_a_row_whose_code_is_all_fill(self):
        counted = {'Main executable': Counter(code=12, c=10, assembly=2, programs=1), 'Stages': Counter(code=16)}
        with self.assertRaisesRegex(SystemExit, 'Stages: no C or assembly bytes, so no share of them'):
            progress.block(counted)

    def test_shows_a_share_of_none_some_under_a_tenth_and_more(self):
        self.assertEqual([progress.percent(progress.share(part, 10000), part) for part in (0, 9, 10, 9999, 10000)],
                         ['0%', '<0.1%', '0.1%', '99.9%', '100.0%'])

    def test_writes_nothing_new_a_second_time_and_then_checks(self):
        with self.built():
            progress.main(['--write', *NUMBERS])
            first = Path('README.md').read_bytes()
            progress.MEASURED.clear()
            progress.main(['--write', *NUMBERS])
            self.assertEqual(Path('README.md').read_bytes(), first)
            progress.main(['--check', *NUMBERS])

    def test_fails_on_stale_numbers_and_leaves_them(self):
        with self.built(), self.assertRaisesRegex(SystemExit, 'README.md: its numbers are stale; make progress '
                                                              'rewrites them'):
            try:
                progress.main(['--check', *NUMBERS])
            finally:
                self.assertEqual(Path('README.md').read_text(), NUMBERED['README.md'])

    def test_fails_without_one_pair_of_markers(self):
        for text in ('# Title\n', '<!-- progress -->\n', '<!-- /progress -->\n<!-- progress -->\n',
                     '<!-- progress -->\n<!-- progress -->\n<!-- /progress -->\n',
                     '<!-- progress -->\n<!-- /progress -->\n<!-- /progress -->\n'):
            with self.subTest(text=text), self.assertRaisesRegex(
                    SystemExit, 'README.md: its numbers go between one <!-- progress --> line and one '
                                '<!-- /progress --> line after it'):
                progress.rewritten(text, BLOCK)

    def test_fails_on_an_image_in_no_row_or_in_two(self):
        for areas, found in ((AREAS[:2] + (('Stages', ('stg*',)),), 'no row'),
                             ((('Main executable', ('1st_read', 'm*')),) + AREAS[1:], 'Main executable and Stages')):
            with self.subTest(found=found), self.built(), mock.patch.object(progress, 'AREAS', areas), \
                    self.assertRaisesRegex(SystemExit, f'config/mod: in {found} of AREAS'):
                progress.main(['--write', *NUMBERS])

    def test_counts_a_body_at_each_copy_and_its_pool_word_once(self):
        for copies in (1, 2, 3):
            with self.subTest(copies=copies), self.built(a_listing(copies)):
                progress.MEASURED.clear()
                config = Config('mod')
                self.assertEqual(progress.assembly(config, 'modules/mod/a.c', config.code()[0], progress.tags.Tables()),
                                 {'basis': 4 * copies + 4, 'open': 2})

    def test_fails_on_a_body_its_listing_lacks(self):
        with self.built(a_listing(0)), self.assertRaisesRegex(
                SystemExit, r'modules/mod/a\.c:9: #pragma inline_asm\(twice\): its lines stand nowhere in its '
                            r"object's listing, so its bytes can't be counted"):
            progress.main(['--write', *NUMBERS])

    def test_fails_on_a_pool_word_c_and_assembly_load(self):
        # the RTS, line 7, is R10's, and loads the word the body's copies load
        with self.built(a_listing(2, loads=[7])), self.assertRaisesRegex(
                SystemExit, r"lines 7, 3, 5 load the pool word at 0x10 in P, and they aren't all C, all with a basis or "
                            r'all open'):
            progress.main(['--write', *NUMBERS])

    def test_fails_where_its_rows_dont_fill_the_objects_section(self):
        # .RES.B 8 leaves a.c's P 8 bytes short of its object's
        short = a_listing(2).replace('00000010          ', '00000008          ')
        with self.built(short), self.assertRaisesRegex(SystemExit, r"its rows give P 0x28 bytes, and its object 0x30"):
            progress.main(['--write', *NUMBERS])

    def test_fails_where_a_row_doesnt_follow_the_one_before(self):
        # the pool's word starts 2 bytes past its alignment's end, or 2 bytes before it, and the .ALIGN after the
        # pool still brings P to its object's 0x30
        for start in (0x12, 0x0E):
            moved = a_listing(2).replace('00000010 00000001', f'{start:08X} 00000001')
            with self.subTest(start=start), self.built(moved), self.assertRaisesRegex(
                    SystemExit, r"build/modules/mod/a\.lis:14: its bytes don't follow those before them in their section"):
                progress.MEASURED.clear()
                progress.main(['--write', *NUMBERS])

    def test_fails_where_a_src_sources_listing_gives_other_bytes_than_its_rows(self):
        # b.src's object and listing give P 0xb0 bytes, and its row 0xc0
        short = B_SRC.replace('174,0', '158,0')
        with self.built(files={'modules/mod/b.src': short, 'build/modules/mod/b.lis': b_listing(short),
                               'build/modules/mod/b.obj': obj(0xB0, 32)}), self.assertRaisesRegex(
                    SystemExit, r'config/mod/splits\.txt: the listings give its \.src sources 176 of its code bytes, '
                                r'and its rows 192'):
            progress.main(['--write', *NUMBERS])

    def test_fails_on_a_repeat_count_in_other_than_decimal_digits(self):
        # asmsh also takes a count in hex, or a name an .EQU gives; its row's code is the value once, so only the
        # count gives the size
        for count in ("H'AE", 'B_SPACE'):
            source = B_SRC.replace('174,0', f'{count},0')
            with self.subTest(count=count), \
                    self.built(files={'modules/mod/b.src': source, 'build/modules/mod/b.lis': b_listing(source)}), \
                    self.assertRaisesRegex(SystemExit, r"build/modules/mod/b\.lis:19: progress reads a \.DATAB's count "
                                                       r'only in decimal digits'):
                progress.MEASURED.clear()
                progress.main(['--write', *NUMBERS])

    def test_fails_on_a_listing_cut_short(self):
        # a.c's listing, cut after its last row
        cut = a_listing(2).split('\r\n  *****TOTAL')[0]
        with self.built(cut), self.assertRaisesRegex(
                SystemExit, r'build/modules/mod/a\.lis: its rows end without its lists, so it may be cut short'):
            progress.main(['--write', *NUMBERS])

    def test_fails_on_a_line_that_is_no_row(self):
        # asmsh writes a row's code in capitals
        lower = a_listing(2).replace('0000000A 000B', '0000000A 000b')
        with self.built(lower), \
                self.assertRaisesRegex(SystemExit, r"build/modules/mod/a\.lis:10: not a row of asmsh's listing"):
            progress.main(['--write', *NUMBERS])

    def test_fails_on_bodies_whose_lines_overlap_or_coincide(self):
        # a second body, with no tag, so open: its lines across twice's two copies overlap both, and on twice's
        # lines, whose tag gives a basis, it would count those bytes as open too
        for lines, problem in (([BODY[1], BODY[0]], r'its lines overlap those of modules/mod/a\.c:9: #pragma '
                                                    r"inline_asm\(twice\) in its object's listing"),
                               (BODY, r'its lines are those of modules/mod/a\.c:9: #pragma inline_asm\(twice\) too, '
                                      r'which is basis')):
            other = ''.join(f'{line}\n' for line in ['#pragma inline_asm(other)', 'static void other(void)', '{', *lines, '}'])
            with self.subTest(lines=lines), self.built(files={'modules/mod/a.c': A_C.replace('#endif', f'{other}#endif')}), \
                    self.assertRaisesRegex(SystemExit, r'modules/mod/a\.c:19: #pragma inline_asm\(other\): ' + problem):
                progress.MEASURED.clear()
                progress.main(['--write', *NUMBERS])

    def test_reads_the_listing_beside_the_newer_object(self):
        # the newer object's listing is a_listing(2). The other listing is newer, and stale: one of a.c with no copy of
        # its body, whose lines then stand nowhere, beside the older object or beside none
        for fresh, stale, orphan in (('build', 'build/stub', False), ('build/stub', 'build', False),
                                     ('build/stub', 'build', True)):
            files = {f'{fresh}/modules/mod/a.obj': obj(0x30, 32), f'{stale}/modules/mod/a.obj': obj(0x30, 32),
                     f'{fresh}/modules/mod/a.lis': a_listing(2), f'{stale}/modules/mod/a.lis': a_listing(0)}
            with self.subTest(fresh=fresh, orphan=orphan), self.built(files=files):
                for path, time in ((f'{fresh}/modules/mod/a.obj', 2), (f'{stale}/modules/mod/a.obj', 1),
                                   (f'{fresh}/modules/mod/a.lis', 1), (f'{stale}/modules/mod/a.lis', 2)):
                    os.utime(path, ns=(time, time))
                if orphan:
                    os.remove(f'{stale}/modules/mod/a.obj')
                progress.MEASURED.clear()
                progress.main(['--write', *NUMBERS])

    def test_fails_where_the_newer_object_has_no_listing(self):
        # the stub is newer, and the full object's listing stands in for none of the stub's
        with self.built(files={'build/stub/modules/mod/a.obj': obj(0x30, 32)}), self.assertRaisesRegex(
                SystemExit, r'build/stub/modules/mod/a\.obj: no listing beside it, so rebuild it'):
            os.utime('build/modules/mod/a.obj', ns=(1, 1))
            os.utime('build/stub/modules/mod/a.obj', ns=(2, 2))
            progress.main(['--write', *NUMBERS])

    def test_fails_on_an_object_and_its_stub_that_differ_with_neither_newer(self):
        with self.built(files={'build/stub/modules/mod/a.obj': obj(0x30, 8)}), self.assertRaisesRegex(
                SystemExit, r'build/modules/mod/a\.obj and build/stub/modules/mod/a\.obj declare their sections '
                            r'differently and neither is newer; rebuild one'):
            for path in ('build/modules/mod/a.obj', 'build/stub/modules/mod/a.obj'):
                os.utime(path, ns=(1, 1))
            progress.main(['--write', *NUMBERS])


if __name__ == '__main__':
    unittest.main()
