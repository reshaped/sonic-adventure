import unittest
from pathlib import Path

import align
import code_ranges
from config import Config
from support import cwd, tree
from test_make import make

ROOT = Path(__file__).resolve().parents[1]

# R10's text of a source with code and data, as it writes one: CRLF line ends, columns of blanks, and the code
# section declared once, on 4
TEXT = ('          .IMPORT     _njPushMatrix\r\n'
        '          .EXPORT     _draw_action_link\r\n'
        '          .SECTION    P,CODE,ALIGN=4\r\n'
        '_draw_action_link:               ; function: draw_action_link\r\n'
        '          RTS\r\n'
        '          NOP\r\n'
        '          .SECTION    D,DATA,ALIGN=4\r\n'
        "_steps:                          ; static: steps\r\n"
        "          .DATA.L     H'00000000\r\n"
        '          .END\r\n')


def aligned(text, section='P', alignment='32'):
    """OUT's bytes, as align.py writes them from text, given as latin-1, for section and alignment."""
    with tree({'in.src': text.encode('latin-1')}):
        align.main([section, alignment, 'in.src', 'out.src'])
        return Path('out.src').read_bytes().decode('latin-1')


class Rewrite(unittest.TestCase):
    def test_changes_the_sections_alignment_alone(self):
        self.assertEqual(aligned(TEXT), TEXT.replace('P,CODE,ALIGN=4', 'P,CODE,ALIGN=32'))
        self.assertEqual(aligned(TEXT, 'D', '8'), TEXT.replace('D,DATA,ALIGN=4', 'D,DATA,ALIGN=8'))

    def test_keeps_every_other_byte(self):
        # LF line ends, a comment after the operands, a Shift JIS comment and no final line end
        text = ('          .SECTION    P,CODE,ALIGN=4 ; code\n'
                '          RTS                     ; \x82\xa0\n'
                '          NOP')
        self.assertEqual(aligned(text), text.replace('ALIGN=4', 'ALIGN=32'))

    def test_takes_the_directive_in_either_case(self):
        self.assertEqual(aligned(TEXT.replace('.SECTION    P', '.section    P')),
                         TEXT.replace('.SECTION    P,CODE,ALIGN=4', '.section    P,CODE,ALIGN=32'))


class Refusals(unittest.TestCase):
    def rejects(self, pattern, text=TEXT, section='P', alignment='32'):
        # align.py fails with pattern, and writes no OUT
        with tree({'in.src': text.encode('latin-1')}):
            with self.assertRaisesRegex(SystemExit, pattern):
                align.main([section, alignment, 'in.src', 'out.src'])
            self.assertFalse(Path('out.src').exists())

    def test_rejects_a_section_no_line_declares(self):
        self.rejects(r'in\.src: no \.SECTION line declares C', section='C')
        # PSG is no declaration of P
        self.rejects(r'in\.src: no \.SECTION line declares P', TEXT.replace('P,CODE', 'PSG,CODE'))
        # a directive in a comment declares nothing
        self.rejects(r'in\.src: no \.SECTION line declares B', TEXT + '; .SECTION    B,DATA,ALIGN=4\r\n', 'B')

    def test_rejects_declarations_that_differ(self):
        again = TEXT.replace('          .END', '          .SECTION    P,CODE,ALIGN=8\r\n          .END')
        self.rejects(r'in\.src:10: declares P as P,CODE,ALIGN=8, and line 3 as P,CODE,ALIGN=4', again)
        # lines end at LF alone: 0x85, a Shift JIS lead byte, ends none
        self.rejects(r'in\.src:11: declares P as P,CODE,ALIGN=8, and line 4 as P,CODE,ALIGN=4', '; \x85\x40\r\n' + again)

    def test_rejects_an_alignment_no_larger_than_the_declared(self):
        for alignment in ('4', '2', '1'):
            with self.subTest(alignment=alignment):
                self.rejects(rf'in\.src:3: P is on 4 already, and {alignment} is no larger', alignment=alignment)

    def test_rejects_an_alignment_that_is_no_power_of_two(self):
        self.rejects(r'24: not a power of two', alignment='24')
        self.rejects(r'0: not a power of two', alignment='0')
        self.rejects(r'0x20: no alignment, a number of bytes', alignment='0x20')
        # a digit str.isdigit() takes and int() doesn't, and more digits than any alignment has
        self.rejects(r'²: no alignment, a number of bytes', alignment='²')
        self.rejects(r'4{10}: no alignment, a number of bytes', alignment='4' * 10)

    def test_rejects_a_declaration_without_one_alignment(self):
        self.rejects(r'in\.src:3: P,CODE gives no ALIGN= operands, not one', TEXT.replace('P,CODE,ALIGN=4', 'P,CODE'))
        self.rejects(r'in\.src:3: P,CODE,ALIGN=4,ALIGN=4 gives 2 ALIGN= operands, not one',
                     TEXT.replace('P,CODE,ALIGN=4', 'P,CODE,ALIGN=4,ALIGN=4'))

    def test_rejects_a_change_to_any_line_but_the_declaration(self):
        # asmsh warns where a later declaration gives P another alignment, so a second one would change too
        again = TEXT.replace('          .END', '          .SECTION    P,CODE,ALIGN=4\r\n          .END')
        self.rejects(r'in\.src:10: declares P again, and would change with line 3; the tool changes one line', again)


# a makefile that reads the repository's config/compilers.mk and prints each patched or grafted object and its ALIGN
PROBE = ('include config/compilers.mk\n'
         'OBJECTS := $(patsubst %.c,build/%.obj,$(PATCHED_C) $(GRAFTED_C))\n'
         'all: $(OBJECTS)\n'
         '$(OBJECTS): FORCE\n'
         "\t@echo '$@:$(ALIGN)'\n"
         'FORCE:\n'
         '.PHONY: all FORCE\n')


def settings():
    """Each object the repository builds on a patched or grafted route, by its path under build/, and the ALIGN its
    config/compilers.mk gives it, or ''."""
    with tree({'probe.mk': PROBE}) as probe:
        status, output = make(ROOT, '-s', '-f', str(probe / 'probe.mk'))
    if status:
        raise AssertionError(output)
    return dict(line.split(':', 1) for line in output.splitlines())


class Setting(unittest.TestCase):
    def test_raises_action_link_cs_code_section_alone(self):
        # action.c's object, the Ninja motion members' and the grafted njCameraMotion.c's keep the 4 R10 declares
        found = settings()
        self.assertEqual({obj: value for obj, value in found.items() if value}, {'build/src/draw/action_link.obj': 'P 32'})
        for obj in ('build/src/draw/action.obj', 'build/sdk/ninja/njMotion.obj', 'build/sdk/ninja/njMotLinkSrc.obj',
                    'build/sdk/ninja/njCameraMotion.obj'):
            self.assertEqual(found[obj], '', obj)

    def test_lets_the_fill_rule_write_the_28_bytes_before_action_link_c(self):
        # 1ST_READ.BIN's tables, each object's sections on 4, but action_link.c's code on the alignment its ALIGN
        # gives: from the end of action.c's code, the linker's fill runs to action_link.c's. On R10's 4, those 28
        # bytes are code nothing writes.
        given = settings()['build/src/draw/action_link.obj']
        self.assertRegex(given, r'^[A-Z][A-Z0-9]* [0-9]+$')
        section, alignment = given.split()
        with cwd(ROOT):
            config = Config('1st_read')
        gap = (0x8C0419C4, 0x8C0419E0)
        for value, fill in ((int(alignment), True), (4, False)):
            def declared(source, name):
                return None, value if (source, name) == ('src/draw/action_link.c', section) else 4
            with self.subTest(alignment=value):
                self.assertEqual((section, *gap) in config.linker_fill(declared), fill)
                left = [(start, end) for _, _, start, end in code_ranges.unwritten(config, code_ranges.writes(config, declared))]
                self.assertEqual(gap in left, not fill)


if __name__ == '__main__':
    unittest.main()
