import contextlib
import io
import unittest
from pathlib import Path
from unittest import mock

import code_ranges
import splice
from config import Config
from support import tree

# the original image, which the splicer only reads, and the image it builds
ORIGINAL, TARGET = 'orig/IMG.BIN', 'build/IMG.BIN'

# an image whose file stores P and half of B: a.c's code fills half of P, and z.c's bss half of B. Only where B starts
# on 16 does the linker's fill write the rest of P
IMAGE = {
    'config/img/splits.txt': ('P  0x0c900000  0x0c900010\n'
                              'B  0x0c900010  0x0c900020\n'
                              'modules/img/a.c  P  0x0c900000  0x0c900008\n'
                              'modules/img/z.c  B  0x0c900010  0x0c900018\n'),
    'config/img/symbols.txt': '',
    'modules/img/a.c': '',
    'modules/img/z.c': '',
    'build/img/modules/img/a.bin': bytes(range(1, 9)),
    'build/img/modules/img/z.bin': bytes(8),
    ORIGINAL: bytes(range(1, 9)) + bytes(16),
}

# IMAGE with its P as D, and a body in a.c's half
BODY = dict(IMAGE, **{
    'config/img/splits.txt': ('D  0x0c900000  0x0c900010\n'
                              'B  0x0c900010  0x0c900020\n'
                              'modules/img/a.c  D  0x0c900000  0x0c900008\n'
                              'modules/img/z.c  B  0x0c900010  0x0c900018\n'),
    'modules/img/a.c': '#include "img/a/Name.inc"\n',
    'build/gen/modules/img/a/bodies.txt': 'Name  0x0c900004  4  char\n',
    'build/stub/img/modules/img/a.bin': bytes(range(1, 5)) + bytes(4),
})

# a.c's code in P, then the linker's fill to D on 32, then a.c's data with a body 4 bytes in, 20 into its linked source
LATER = {
    'config/img/splits.txt': ('P  0x0c900000  0x0c900020\n'
                              'D  0x0c900020  0x0c900028\n'
                              'modules/img/a.c  P  0x0c900000  0x0c900010\n'
                              'modules/img/a.c  D  0x0c900020  0x0c900028\n'),
    'config/img/symbols.txt': '',
    'modules/img/a.c': '#include "img/a/name.inc"\n',
    'build/gen/modules/img/a/bodies.txt': 'name  0x0c900024  4  Uint32\n',
    'build/img/modules/img/a.bin': bytes(range(1, 17)) + bytes(range(0x21, 0x29)),
    'build/stub/img/modules/img/a.bin': bytes(range(1, 17)) + bytes(range(0x21, 0x25)) + bytes(4),
    ORIGINAL: bytes(range(1, 17)) + bytes(16) + bytes(range(0x21, 0x29)),
}


def on_16(source, section):
    # link.section's stand-in that starts each object's sections on 16, so B starts where IMAGE's and BODY's fill ends
    return None, 16


# two objects' code in one P, each on 32 bytes, with the linker's fill between; b.c's code ends P and the file
CODE = bytes(range(1, 9))
SEAMS = {
    'config/img/splits.txt': ('P  0x0c900000  0x0c900028\n'
                              'modules/img/a.c  P  0x0c900000  0x0c900008\n'
                              'modules/img/b.c  P  0x0c900020  0x0c900028\n'),
    'config/img/symbols.txt': '',
    'modules/img/a.c': '',
    'modules/img/b.c': '',
    'build/img/modules/img/a.bin': CODE,
    'build/img/modules/img/b.bin': CODE,
    ORIGINAL: CODE + bytes(24) + CODE,
}


# a.c's constants and bss end 5 and 4 bytes before b.c's, each on 8, and the file ends 2 bytes into the bss gap
DATA = {
    'config/img/splits.txt': ('P  0x0c900000  0x0c900010\n'
                              'C  0x0c900010  0x0c900020\n'
                              'B  0x0c900020  0x0c900040\n'
                              'modules/img/a.c  P  0x0c900000  0x0c900010\n'
                              'modules/img/a.c  C  0x0c900010  0x0c900013\n'
                              'modules/img/b.c  C  0x0c900018  0x0c900020\n'
                              'modules/img/a.c  B  0x0c900020  0x0c900024\n'
                              'modules/img/b.c  B  0x0c900028  0x0c900030\n'
                              'fill  0xff\n'),
    'config/img/symbols.txt': '',
    'modules/img/a.c': '',
    'modules/img/b.c': '',
    'build/img/modules/img/a.bin': bytes(range(1, 17)) + b'\x11\x12\x13' + bytes(4),
    'build/img/modules/img/b.bin': bytes(range(0x21, 0x29)) + bytes(8),
    ORIGINAL: bytes(range(1, 17)) + b'\x11\x12\x13' + b'\xff' * 5 + bytes(range(0x21, 0x29)) + bytes(6),
}
# each object's alignment in DATA, by source and section
ALIGNED = {('modules/img/a.c', 'P'): 32, ('modules/img/a.c', 'C'): 8, ('modules/img/b.c', 'C'): 8,
           ('modules/img/a.c', 'B'): 8, ('modules/img/b.c', 'B'): 8}


def aligned(source, section):
    # link.section's stand-in that starts each of DATA's objects on its alignment
    return None, ALIGNED[(source, section)]


# a.c's strings in C, then a gen/ range that continues its object, pads its strings on 4 and declares its object's end,
# then b.c's constants, on 32
DECLARED = {
    'config/img/splits.txt': ('C  0x0c900000  0x0c900028\n'
                              'modules/img/a.c  C  0x0c900000  0x0c900006\n'
                              'gen/img/d_0c900008.c  C  0x0c900008  0x0c90001b  continues 4  end\n'
                              'modules/img/b.c  C  0x0c900020  0x0c900028\n'),
    'config/img/symbols.txt': '',
    'modules/img/a.c': '',
    'modules/img/b.c': '',
    'build/img/modules/img/a.bin': b'abcde\0',
    'build/img/gen/img/d_0c900008.bin': b'fghijklmn\0\0\0opqrst\0',
    'build/img/modules/img/b.bin': bytes(range(1, 9)),
    ORIGINAL: b'abcde\0\0\0' + b'fghijklmn\0\0\0opqrst\0' + bytes(5) + bytes(range(1, 9)),
}
# DECLARED in an image whose fill byte is 0xff, as an event's is: the padding before the gen/ range holds it, as the
# padding inside the range and the linker's fill after it do
PADDED = b'abcde\0\xff\xff' + b'fghijklmn\0\xff\xffopqrst\0' + b'\xff' * 5 + bytes(range(1, 9))
DECLARED_FF = dict(DECLARED, **{'config/img/splits.txt': DECLARED['config/img/splits.txt'] + 'fill  0xff\n',
                                'build/img/gen/img/d_0c900008.bin': PADDED[8:0x1b], ORIGINAL: PADDED})


# an event's P with a source of each route, objects on 4 and the linker's
# fill before s.src, then D with data from the disc. p.c and i.c hold inline
# assembly
ROUTED = {
    'config/img/splits.txt': ('P  0x0c900000  0x0c900028\n'
                              'D  0x0c900028  0x0c900030\n'
                              'events/img/a.c  P  0x0c900000  0x0c900008\n'
                              'events/img/p.c  P  0x0c900008  0x0c900010\n'
                              'events/img/g.c  P  0x0c900010  0x0c900018\n'
                              'events/img/i.c  P  0x0c900018  0x0c90001e\n'
                              'events/img/s.src  P  0x0c900020  0x0c900028\n'
                              'gen/img/d_0c900028.c  D  0x0c900028  0x0c900030\n'),
    'config/img/symbols.txt': '',
    **{f'events/img/{name}': '' for name in ('a.c', 'p.c', 'g.c', 'i.c', 's.src')},
    **{f'build/img/events/img/{name}.bin': bytes(range(at, at + size))
       for name, at, size in (('a', 1, 8), ('p', 0x11, 8), ('g', 0x21, 8), ('i', 0x31, 6), ('s', 0x41, 8))},
    'build/img/gen/img/d_0c900028.bin': bytes(range(0x51, 0x59)),
    ORIGINAL: (bytes(range(1, 9)) + bytes(range(0x11, 0x19)) + bytes(range(0x21, 0x29)) + bytes(range(0x31, 0x37))
               + bytes(2) + bytes(range(0x41, 0x49)) + bytes(range(0x51, 0x59))),
}
# ROUTED's routes, as the Makefile's PATCHED_C, GRAFTED_C and INLINE_C give them
ROUTES = ['--patched', 'events/img/p.c', '--grafted', 'events/img/g.c', '--inline', 'events/img/p.c', 'events/img/i.c']


def seams(source, start):
    # SEAMS with its second object's code moved to source, at start, where P and the file end 8 bytes on; its first
    # object, a.c, lies in source's directory
    at = start - 0x0c900000
    first = f'{source.split("/")[0]}/img/a.c'
    return {
        'config/img/splits.txt': (f'P  0x0c900000  0x{start + 8:08x}\n'
                                  f'{first}  P  0x0c900000  0x0c900008\n'
                                  f'{source}  P  0x{start:08x}  0x{start + 8:08x}\n'),
        'config/img/symbols.txt': '',
        first: '',
        source: '',
        f'build/img/{Path(first).with_suffix(".bin")}': CODE,
        f'build/img/{Path(source).with_suffix(".bin")}': CODE,
        ORIGINAL: CODE + bytes(at - 8) + CODE,
    }


def spliced(files):
    # the image's bytes each category writes, once the image is spliced and reported
    with tree(files), contextlib.redirect_stdout(io.StringIO()):
        result = splice.splice(Path(ORIGINAL), Path(TARGET), set(), set(), set())
        splice.report(Path(TARGET), *result)
    return result[1]


def reported(files, *argv):
    # what splice.py prints as it builds TARGET, given argv
    with tree(files), contextlib.redirect_stdout(io.StringIO()) as out:
        splice.main([ORIGINAL, TARGET, *argv])
    return out.getvalue()


def built(files, *argv):
    # what splice.py fails with as it builds TARGET with argv, its lines without the tool's name, or None where it
    # builds it; then TARGET's bytes, or None where there is no TARGET
    with tree(files), contextlib.redirect_stdout(io.StringIO()):
        try:
            splice.main([ORIGINAL, TARGET, *argv])
            problem = None
        except SystemExit as error:
            problem = '\n'.join(line.split(': ', 1)[1] for line in str(error).splitlines())
        return problem, Path(TARGET).read_bytes() if Path(TARGET).exists() else None


class Splice(unittest.TestCase):
    def setUp(self):
        # link.section's stand-in: each object's sections on the alignment its code would have, 4 bytes in events
        # and 32 elsewhere, with no size, which Config.linker_fill doesn't read
        patch = mock.patch.object(splice, 'section', lambda source, section: (None, 4 if source.startswith('events/') else 32))
        patch.start()
        self.addCleanup(patch.stop)

    def test_writes_the_fill_between_objects(self):
        self.assertEqual(spliced(SEAMS)['fill'], 24)
        # event code starts each object on 4, so b.c would start at a.c's end, and nothing writes the bytes before it
        self.assertEqual(built(seams('events/img/b.c', 0x0c900018))[0],
                         f'{TARGET}: no source or fill writes 0x0c900008 to 0x0c900018, code in P')
        with self.assertRaisesRegex(SystemExit, r'orig/IMG\.BIN: 0x0c900008 to 0x0c900020, a gap the linker leaves in P, '
                                                r'holds bytes other than 0x00'):
            spliced(dict(SEAMS, **{ORIGINAL: CODE + bytes(23) + b'\xff' + CODE}))

    def test_writes_the_fill_before_the_next_section(self):
        # B starts on the 16 bytes z.c names, after a.c's P; on 32, it would start past 0x0c900010
        splice.section = on_16
        self.assertEqual(spliced(IMAGE)['fill'], 8)
        with self.assertRaisesRegex(SystemExit, r'orig/IMG\.BIN: 0x0c900008 to 0x0c900010, a gap the linker leaves in P, '
                                                r'holds bytes other than 0x00'):
            spliced(dict(IMAGE, **{ORIGINAL: bytes(range(1, 9)) + bytes(7) + b'\x09' + bytes(8)}))

    def test_writes_the_fill_in_every_section_and_0_in_bss(self):
        # the gap in C takes the fill byte, and the one in B 0, up to the file's end
        splice.section = aligned
        self.assertEqual(spliced(DATA)['fill'], 7)
        disc = DATA[ORIGINAL]
        for wrong, gap, name, byte in ((disc[:0x13] + bytes(5) + disc[0x18:], '0x0c900013 to 0x0c900018', 'C', 'ff'),
                                       (disc[:0x24] + b'\xff\xff', '0x0c900024 to 0x0c900026', 'B', '00')):
            with self.assertRaisesRegex(SystemExit, rf'orig/IMG\.BIN: {gap}, a gap the linker leaves in {name}, holds bytes other than 0x{byte}'):
                spliced(dict(DATA, **{ORIGINAL: wrong}))

    def test_rejects_a_section_past_the_files_end_outside_bss(self):
        # C runs to B's start at 0x0c900020, and the file ends at 0x0c900014
        files = dict(DATA, **{ORIGINAL: DATA[ORIGINAL][:0x14]})
        with self.assertRaisesRegex(SystemExit, r'config/img/splits\.txt: section C runs to 0x0c900020, '
                                                r'past the end of orig/IMG\.BIN, at 0x0c900014'):
            spliced(files)

    def test_rejects_sections_that_end_short_of_the_file(self):
        # P, which ends at 0x0c900028, would leave any code after it, to the file's end, out of the image's code
        files = dict(SEAMS, **{ORIGINAL: SEAMS[ORIGINAL] + bytes(24)})
        with self.assertRaisesRegex(SystemExit, r'config/img/splits\.txt: its sections end at 0x0c900028, '
                                                r'short of the end of orig/IMG\.BIN, at 0x0c900040'):
            spliced(files)

    def test_writes_the_fill_between_sections(self):
        # a.c's constants end C at 0x0c900013, and b.c's data starts D on 8, at 0x0c900018
        splice.section = lambda source, section: (None, 8)
        files = dict(DATA, **{'config/img/splits.txt': DATA['config/img/splits.txt']
                                 .replace('C  0x0c900010  0x0c900020', 'C  0x0c900010  0x0c900013\nD  0x0c900018  0x0c900020')
                                 .replace('b.c  C', 'b.c  D')})
        self.assertEqual(spliced(files)['fill'], 7)
        disc = DATA[ORIGINAL]
        with self.assertRaisesRegex(SystemExit, r'orig/IMG\.BIN: 0x0c900013 to 0x0c900018, a gap the linker leaves '
                                                r'between sections, holds bytes other than 0xff'):
            spliced(dict(files, **{ORIGINAL: disc[:0x13] + bytes(5) + disc[0x18:]}))

    def test_writes_the_fill_and_padding_gen_ranges_declare(self):
        # a.c's strings end at 0x0c900006, and the gen/ range's go on in its object on 4, at 0x0c900008; the range's
        # object ends at 0x0c90001b, and b.c's constants start on 32 at 0x0c900020. The padding counts with the range
        counts = spliced(DECLARED)
        self.assertEqual((counts['c'], counts['generated'], counts['fill']), (14, 21, 5))
        self.assertEqual(sum(counts.values()), 0x28)
        disc = DECLARED[ORIGINAL]
        for wrong, gap, what in ((disc[:6] + b'\0\1' + disc[8:], '0x0c900006 to 0x0c900008', 'padding a gen/ range declares in C'),
                                 (disc[:0x1b] + b'\0\0\0\0\1' + disc[0x20:], '0x0c90001b to 0x0c900020', 'a gap the linker leaves in C')):
            with self.subTest(gap=gap), self.assertRaisesRegex(SystemExit, rf'orig/IMG\.BIN: {gap}, {what}, holds bytes other than 0x00'):
                spliced(dict(DECLARED, **{ORIGINAL: wrong}))
        # without its declarations, nothing writes the padding or the fill
        undeclared = dict(DECLARED, **{'config/img/splits.txt': DECLARED['config/img/splits.txt'].replace('  continues 4  end', '')})
        self.assertEqual(built(undeclared)[0].splitlines(),
                         [f'{TARGET}: no source or fill writes 0x0c900006 to 0x0c900008, data in C',
                          f'{TARGET}: no source or fill writes 0x0c90001b to 0x0c900020, data in C'])

    def test_writes_the_images_fill_byte_as_declared_padding(self):
        # zeros in the padding before DECLARED_FF's gen/ range fail
        counts = spliced(DECLARED_FF)
        self.assertEqual((counts['c'], counts['generated'], counts['fill']), (14, 21, 5))
        with self.assertRaisesRegex(SystemExit, r'orig/IMG\.BIN: 0x0c900006 to 0x0c900008, padding a gen/ range declares '
                                                r'in C, holds bytes other than 0xff'):
            spliced(dict(DECLARED_FF, **{ORIGINAL: PADDED[:6] + bytes(2) + PADDED[8:]}))

    def test_checks_stored_bytes_and_counts_them(self):
        splice.section = on_16
        self.assertEqual(spliced(IMAGE)['c'], 16)

    def test_checks_a_body_whose_include_has_capitals(self):
        splice.section = on_16
        files = dict(BODY)
        counts = spliced(files)
        self.assertEqual(counts['c'], 12)
        self.assertEqual(counts['generated'], 4)
        files['build/stub/img/modules/img/a.bin'] = bytes(8)
        with self.assertRaisesRegex(SystemExit, r'0x0c900000, outside its bodies'):
            spliced(files)

    def test_checks_a_body_past_its_sources_first_range(self):
        self.assertEqual(spliced(LATER)['generated'], 4)

    def test_reads_a_bodys_address_in_its_sources_home(self):
        # home, first by name, places a.c at 0x0c800000, where bodies.txt gives its body
        splice.section = on_16
        files = dict(BODY, **{'config/home/splits.txt': 'D  0x0c800000  0x0c800010\nmodules/img/a.c  D  0x0c800000  0x0c800008\n',
                              'config/home/symbols.txt': '', 'build/gen/modules/img/a/bodies.txt': 'Name  0x0c800004  4  char\n'})
        self.assertEqual(spliced(files)['generated'], 4)

    def test_rejects_a_body_in_code(self):
        files = dict(BODY, **{'config/img/splits.txt': IMAGE['config/img/splits.txt']})
        with self.assertRaisesRegex(SystemExit, r'modules/img/a\.c: Name lies in code, at 0x0c900004, and a body is data from the disc'):
            spliced(files)
        # LATER's a.c with its code second in the table, and the body in it, 12 bytes into its linked source
        files = dict(LATER, **{'config/img/splits.txt': LATER['config/img/splits.txt'].replace(
            'modules/img/a.c  P  0x0c900000  0x0c900010\nmodules/img/a.c  D  0x0c900020  0x0c900028\n',
            'modules/img/a.c  D  0x0c900020  0x0c900028\nmodules/img/a.c  P  0x0c900000  0x0c900010\n'),
                               'build/gen/modules/img/a/bodies.txt': 'name  0x0c900004  4  Uint32\n',
                               'build/stub/img/modules/img/a.bin': bytes(range(1, 13)) + bytes(4) + bytes(range(0x21, 0x29))})
        with self.assertRaisesRegex(SystemExit, r'modules/img/a\.c: name lies in code, at 0x0c900004'):
            spliced(files)

    def test_rejects_a_stored_byte_the_disc_doesnt_have(self):
        files = dict(IMAGE, **{'build/img/modules/img/a.bin': bytes(range(2, 10))})
        with self.assertRaisesRegex(SystemExit, r'modules/img/a\.c: its byte at 0x0c900000 differs'):
            spliced(files)

    def test_reserves_bss_past_the_files_end(self):
        splice.section = on_16
        self.assertEqual(spliced(dict(IMAGE, **{ORIGINAL: bytes(range(1, 9)) + bytes(8)}))['c'], 8)
        # the file ends inside the bss: its stored half is checked, its rest reserved
        self.assertEqual(spliced(dict(IMAGE, **{ORIGINAL: bytes(range(1, 9)) + bytes(12)}))['c'], 12)

    def test_rejects_bss_past_the_end_that_isnt_zero(self):
        files = dict(IMAGE, **{ORIGINAL: bytes(range(1, 9)) + bytes(8), 'build/img/modules/img/z.bin': bytes(7) + b'\1'})
        with self.assertRaisesRegex(SystemExit, r"its bss past the end of orig/IMG\.BIN holds bytes that aren't 0"):
            spliced(files)

    def test_rejects_code_past_the_end_even_when_zero(self):
        files = dict(IMAGE, **{ORIGINAL: bytes(range(1, 9)),
                               'config/img/splits.txt': IMAGE['config/img/splits.txt'] + 'modules/img/b.c  P  0x0c900008  0x0c900010\n',
                               'modules/img/b.c': '', 'build/img/modules/img/b.bin': bytes(8)})
        with self.assertRaisesRegex(SystemExit, r'config/img/splits\.txt: section P runs to 0x0c900010, past the end'):
            spliced(files)

    def test_rejects_code_that_starts_inside_an_object(self):
        # on 32 bytes, after the linker's fill, the code starts an object of its own
        self.assertEqual(spliced(SEAMS)['c'], 16)
        with self.assertRaisesRegex(SystemExit, r"modules/img/b\.c: 0x0c900020 follows SHC's 00 09 fill"):
            spliced(dict(SEAMS, **{ORIGINAL: CODE + bytes(22) + b'\x00\x09' + CODE}))
        with self.assertRaisesRegex(SystemExit, r'modules/img/b\.c: 0x0c900018 is off 32 bytes'):
            spliced(seams('modules/img/b.c', 0x0c900018))
        # event code packs its objects on 4, but SHC's fill still lies only inside an object
        self.assertEqual(spliced(seams('events/img/b.c', 0x0c900008))['c'], 16)
        with self.assertRaisesRegex(SystemExit, r"events/img/b\.c: 0x0c900018 follows SHC's 00 09 fill"):
            spliced(dict(seams('events/img/b.c', 0x0c900018), **{ORIGINAL: CODE + bytes(14) + b'\x00\x09' + CODE}))
        # nor can it start in the fill, where no alignment check looks
        filled = dict(seams('events/img/b.c', 0x0c90001a), **{'build/img/events/img/b.bin': b'\x00\x09' + CODE[:6],
                                                              ORIGINAL: CODE + bytes(18) + b'\x00\x09' + CODE[:6]})
        with self.assertRaisesRegex(SystemExit, r"events/img/b\.c: 0x0c90001a holds SHC's 00 09 fill"):
            spliced(filled)

    def test_rejects_code_that_ends_inside_an_object(self):
        with self.assertRaisesRegex(SystemExit, r"modules/img/a\.c: 0x0c900008 precedes SHC's 00 09 fill"):
            spliced(dict(SEAMS, **{ORIGINAL: CODE + b'\x00\x09' + bytes(22) + CODE}))
        with self.assertRaisesRegex(SystemExit, r'modules/img/a\.c: 0x0c900008 is off 32 bytes and no fill follows it'):
            spliced(dict(SEAMS, **{ORIGINAL: CODE + b'\x0b\x00' + bytes(22) + CODE}))
        # a row that ends on 32 bytes, but in SHC's fill, with the object's next function after it
        head = CODE + b'\x00\x09' * 12
        with self.assertRaisesRegex(SystemExit, r"modules/img/a\.c: 0x0c90001e holds SHC's 00 09 fill"):
            spliced(dict(SEAMS, **{'config/img/splits.txt': 'P  0x0c900000  0x0c900040\nmodules/img/a.c  P  0x0c900000  0x0c900020\n',
                                   'build/img/modules/img/a.bin': head, ORIGINAL: head + CODE + bytes(24)}))

    def test_fails_on_code_no_source_or_fill_writes(self):
        # SEAMS's P run on past b.c's code to the file's end, and the rest of IMAGE's P, which no fill writes with B
        # on 32: the splice names each range, and builds no image
        longer = dict(SEAMS, **{'config/img/splits.txt': SEAMS['config/img/splits.txt'].replace(
            'P  0x0c900000  0x0c900028', 'P  0x0c900000  0x0c900040'), ORIGINAL: SEAMS[ORIGINAL] + bytes(24)})
        for files, run in ((longer, '0x0c900028 to 0x0c900040'), (IMAGE, '0x0c900008 to 0x0c900010')):
            with self.subTest(run=run):
                self.assertEqual(built(files), (f'{TARGET}: no source or fill writes {run}, code in P', None))
        whole = dict(IMAGE, **{'config/img/splits.txt': IMAGE['config/img/splits.txt'] + 'events/img/b.c  P  0x0c900008  0x0c900010\n',
                               'events/img/b.c': '', 'build/img/events/img/b.bin': bytes(8)})
        self.assertEqual(built(whole), (None, IMAGE[ORIGINAL]))

    def test_fails_on_a_stored_byte_nothing_writes(self):
        # p.c's code a byte past a.c's, off 4, and the gen/ range's data a byte short of D's end: the splice names the
        # byte, its section and whether it is code or data, and removes the copy of the original a build once left in
        # TARGET
        p, gen = 'build/img/events/img/p.bin', 'build/img/gen/img/d_0c900028.bin'
        for row, short, linked, run in (
                ('events/img/p.c  P  0x0c900008', 'events/img/p.c  P  0x0c900009', {p: ROUTED[p][1:]},
                 '0x0c900008 to 0x0c900009, code in P'),
                ('d_0c900028.c  D  0x0c900028  0x0c900030', 'd_0c900028.c  D  0x0c900028  0x0c90002f', {gen: ROUTED[gen][:-1]},
                 '0x0c90002f to 0x0c900030, data in D')):
            files = dict(ROUTED, **{'config/img/splits.txt': ROUTED['config/img/splits.txt'].replace(row, short),
                                    **linked, TARGET: ROUTED[ORIGINAL]})
            with self.subTest(run=run):
                self.assertEqual(built(files, *ROUTES), (f'{TARGET}: no source or fill writes {run}', None))

    def test_fails_each_range_of_data_no_source_or_fill_writes(self):
        # a.c and b.c leave D's bytes from 0x0c900008 to 0x0c900010 and from 0x0c900014 to the file's end
        files = dict(IMAGE, **{'config/img/splits.txt': ('D  0x0c900000  0x0c900018\n'
                                                         'modules/img/a.c  D  0x0c900000  0x0c900008\n'
                                                         'modules/img/b.c  D  0x0c900010  0x0c900014\n'),
                               'modules/img/b.c': '', 'build/img/modules/img/b.bin': bytes(4)})
        problem, after = built(files)
        self.assertEqual(problem.splitlines(), [f'{TARGET}: no source or fill writes 0x0c900008 to 0x0c900010, data in D',
                                                f'{TARGET}: no source or fill writes 0x0c900014 to 0x0c900018, data in D'])
        self.assertIsNone(after)

    def test_builds_the_image_from_the_sources_and_fill_alone(self):
        # the splicer reads the original only to check what the sources and the fill write. With every check passing,
        # an original that differs in every byte, left in TARGET too, gives the image the sources and the fill build:
        # code of each route, data, imported data and bss, the linker's fill of 0x00 and of 0xff, and declared padding
        # of each. Each fixture's original holds those bytes, so the image equals it
        usual = splice.section
        with mock.patch.object(splice, 'differs', return_value=None), mock.patch.object(splice, 'whole'):
            for name, files, section, argv in (('ROUTED', ROUTED, usual, ROUTES), ('BODY', BODY, on_16, ()),
                                               ('IMAGE', IMAGE, on_16, ()), ('DATA', DATA, aligned, ()),
                                               ('DECLARED', DECLARED, usual, ()), ('DECLARED_FF', DECLARED_FF, usual, ())):
                other = bytes(byte ^ 0xff for byte in files[ORIGINAL])
                with self.subTest(fixture=name), mock.patch.object(splice, 'section', section):
                    self.assertEqual(built(dict(files, **{ORIGINAL: other, TARGET: other}), *argv), (None, files[ORIGINAL]))

    def test_refuses_to_build_over_the_original(self):
        with tree(SEAMS), contextlib.redirect_stderr(io.StringIO()) as err, self.assertRaises(SystemExit):
            splice.main([ORIGINAL, f'orig/../{ORIGINAL}'])
        self.assertIn('orig/../orig/IMG.BIN is the original image, and the splice writes a new one', err.getvalue())

    def test_checks_the_image_without_reading_a_readme(self):
        argv = ['--verbose', '--patched', '--grafted', '--inline']
        out = io.StringIO()
        with tree(SEAMS), contextlib.redirect_stdout(out):
            splice.main([ORIGINAL, TARGET, *argv])
            self.assertEqual(Path(TARGET).read_bytes(), SEAMS[ORIGINAL])
        self.assertIn('  c                 16         16  C that SHC compiles to an object\n', out.getvalue())

    def test_default_output_is_one_line(self):
        self.assertEqual(reported(SEAMS), f'  splice {TARGET}\n')

    def test_counts_the_bytes_each_route_writes(self):
        with tree(ROUTED):
            _, image, code, mixed = splice.splice(Path(ORIGINAL), Path(TARGET), {'events/img/p.c'}, {'events/img/g.c'},
                                                  {'events/img/p.c', 'events/img/g.c', 'events/img/i.c'})
        self.assertEqual(image, {'c': 8, 'shckit': 8, 'grafted': 8, 'inline': 6, 'assembly': 8, 'generated': 8, 'fill': 2})
        self.assertEqual(code, dict(image, **{'generated': 0}))
        # p.c's and g.c's code: a shckit and a grafted source with inline assembly
        self.assertEqual(mixed, 16)

    def test_reports_the_image_and_its_code_by_what_writes_them(self):
        self.assertEqual(reported(ROUTED, '--verbose', *ROUTES), (
            f'{TARGET}: 48 bytes, 40 of them code: 40 game and 0 SDK\n'
            '                  code      image\n'
            '  c                  8          8  C that SHC compiles to an object\n'
            '  shckit             8          8  C whose R10 assembly build/shckit patches\n'
            '  grafted            8          8  C whose R10 assembly tools/graft.py grafts and build/shckit patches\n'
            '                                   8 of the shckit and grafted code bytes are of sources with inline assembly\n'
            '  inline             6          6  C with inline assembly, which R10 compiles to assembly\n'
            '  assembly           8          8  .src files\n'
            '  generated          0          8  data from the disc, which tools/data.py writes as C\n'
            "  fill               2          2  the linker's fill, in the gaps it leaves\n"))
        self.assertEqual(reported(ROUTED, *ROUTES), f'  splice {TARGET}\n')

    def test_counts_the_code_as_progress_does(self):
        # progress.py counts each image's code from its placements; the splicer, from the bytes it writes
        routes = {'events/img/p.c'}, {'events/img/g.c'}, {'events/img/p.c', 'events/img/g.c', 'events/img/i.c'}
        with tree(ROUTED):
            _, _, code, mixed = splice.splice(Path(ORIGINAL), Path(TARGET), *routes)
            self.assertEqual(code_ranges.written(Config('img'), splice.section, *routes), (code, mixed))

    def test_wants_the_routes_for_its_report(self):
        for argv in ([], ROUTES[:2], ROUTES[:4]):
            with self.subTest(argv=argv), self.assertRaises(SystemExit), contextlib.redirect_stderr(io.StringIO()) as err:
                reported(ROUTED, '--verbose', *argv)
            self.assertIn('--verbose needs --patched, --grafted and --inline, as the Makefile gives them', err.getvalue())

    def test_fails_a_byte_two_write_though_their_values_agree(self):
        # a gap that overlapped a.c's range would write its last 4 bytes, zeros, again, as zeros
        gaps = mock.patch.object(splice.Config, 'linker_fill',
                                 lambda config, section: [('P', 0x0c900004, 0x0c900008), ('P', 0x0c90001e, 0x0c900020)])
        files = dict(ROUTED, **{ORIGINAL: bytes(range(1, 5)) + bytes(4) + ROUTED[ORIGINAL][8:],
                                'build/img/events/img/a.bin': bytes(range(1, 5)) + bytes(4)})
        with gaps, self.assertRaisesRegex(SystemExit, r": build/IMG\.BIN: events/img/a\.c and the linker's fill in P "
                                                      r'both write 0x0c900004 to 0x0c900008$'):
            spliced(files)

    def test_reports_an_image_without_code(self):
        files = dict(IMAGE, **{'config/img/splits.txt': 'D  0x0c900000  0x0c900008\nmodules/img/a.c  D  0x0c900000  0x0c900008\n',
                               ORIGINAL: bytes(range(1, 9))})
        self.assertEqual(spliced(files)['c'], 8)

    def test_names_a_missing_linked_source(self):
        files = dict(IMAGE)
        del files['build/img/modules/img/z.bin']
        with self.assertRaisesRegex(SystemExit, r'build/img/modules/img/z\.bin: No such file'):
            spliced(files)


class Writers(unittest.TestCase):
    def test_names_each_writer_as_the_splicer_does(self):
        # DECLARED's a.c, the padding its gen/ range declares, the range, the linker's fill to b.c on 32, and b.c
        with tree(DECLARED):
            found = code_ranges.writers(Config('img'), lambda source, name: (None, 32, False))
        self.assertEqual(found, [(0x0C900000, 0x0C900006, 'modules/img/a.c'),
                                 (0x0C900006, 0x0C900008, 'the padding a gen/ range declares in C'),
                                 (0x0C900008, 0x0C90001B, 'gen/img/d_0c900008.c'),
                                 (0x0C90001B, 0x0C900020, "the linker's fill in C"),
                                 (0x0C900020, 0x0C900028, 'modules/img/b.c')])
        self.assertEqual(code_ranges.filled(None), "the linker's fill between sections")


class Compare(unittest.TestCase):
    def compare(self, data, stub, spans):
        with tree({'stub.bin': stub}):
            splice.compare('src/x.c', Path('stub.bin'), [('P', 0x1000, 0x1008)], data, spans)

    def test_excuses_a_body_its_stub_zeroes_and_a_text(self):
        self.compare(bytes(range(1, 9)), b'\1\2\0\0\x09\x09\7\x08', [(2, 2, False, 'b'), (4, 2, True, 't')])

    def test_rejects_a_body_its_stub_doesnt_zero(self):
        with self.assertRaisesRegex(SystemExit, r'holds a byte other than 0 at 0x00001002, in a body'):
            self.compare(bytes(range(1, 9)), b'\1\2\1\0\5\6\7\x08', [(2, 2, False, 'b')])

    def test_rejects_a_difference_outside_bodies(self):
        with self.assertRaisesRegex(SystemExit, r'differs from its own at 0x00001000, outside its bodies'):
            self.compare(bytes(range(1, 9)), b'\0\2\3\4\5\6\7\x08', [])

    def test_rejects_a_stub_of_another_length(self):
        with self.assertRaisesRegex(SystemExit, r'its stub build is 7 bytes, and its own 8'):
            self.compare(bytes(range(1, 9)), bytes(7), [])


class Differs(unittest.TestCase):
    def test_names_the_first_byte_that_differs_or_lies_past_the_end(self):
        # the original's bytes load at 0x100 and end at 0x104
        disc = bytes(range(1, 5))
        self.assertIsNone(splice.differs(disc, 0x100, 0x101, b'\2\3'))
        self.assertEqual(splice.differs(disc, 0x100, 0x101, b'\2\0\4'), 0x102)
        self.assertEqual(splice.differs(disc, 0x100, 0x102, b'\3\4\0'), 0x104)


class Check(unittest.TestCase):
    def setUp(self):
        # link.section's stand-in, on which the linker's fill writes the rest of IMAGE's P
        patch = mock.patch.object(splice, 'section', on_16)
        patch.start()
        self.addCleanup(patch.stop)

    def test_wants_each_linked_source_as_long_as_its_rows(self):
        with tree(IMAGE):
            splice.check()
        with tree(dict(IMAGE, **{'build/img/modules/img/a.bin': bytes(7)})), self.assertRaisesRegex(
                SystemExit, r'build/img/modules/img/a\.bin: 7 bytes, where config/img/splits\.txt places 8'):
            splice.check()
        files = dict(IMAGE)
        del files['build/img/modules/img/z.bin']
        with tree(files), self.assertRaisesRegex(
                SystemExit, r'config/img/splits\.txt: modules/img/z\.c is there, and nothing built build/img/modules/img/z\.bin'):
            splice.check()

    def test_reads_a_stub_for_a_source_with_bodies_and_skips_gen_ranges(self):
        # a.c's own linked source needs the disc, and so does the gen/ range in z.c's place
        files = dict(BODY, **{'config/img/splits.txt': BODY['config/img/splits.txt'].replace('modules/img/z.c', 'gen/img/d_0c900010.c')})
        del files['build/img/modules/img/a.bin']
        with tree(files):
            splice.check()

    def test_fails_on_code_the_placements_and_fill_leave_unwritten(self):
        # on 32, the end of a.c's code rounds up past B's start, so the fill writes none of the rest of P
        splice.section = lambda source, section: (None, 32)
        with tree(IMAGE), self.assertRaisesRegex(
                SystemExit, r': config/img/splits\.txt: no source or fill writes 0x0c900008 to 0x0c900010, code in P$'):
            splice.check()


if __name__ == '__main__':
    unittest.main()
