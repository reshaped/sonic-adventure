import unittest
from unittest import mock

import decls
import elf
import gcclink
import ld
import test_ld
import verify
from tables import hexa
from test_ld import importing, linking, words


def objects_alone(selected):
    """selected, as gcclink.selected(), less the archive members it takes."""
    def taking(paths, roots=()):
        taken, wanted = selected(paths, roots)
        return [each for each in taken if each[1] is None], wanted
    return taking


class Checks(test_ld.Fixture):
    def test_passes_the_link_ld_py_makes_and_reads_each_final_pass_as_ld_py_left_it(self):
        _, _, finals = ld.coordinate(self.images, self.out)
        self.assertEqual(verify.misbound(self.images, finals) + verify.unanchored(self.images, finals)
                         + verify.unselected(self.images, finals), [])
        self.assertEqual({name: gcclink.linked(self.out, name, 'final') for name in self.images}, finals)

    def test_runs_the_declarations_check_once_the_link_s_checks_pass(self):
        ld.main(['link.txt', str(self.out)])
        with mock.patch.object(decls, 'failures', return_value=[]) as declared:
            verify.main(['--abi=-ml -m4-single-only', 'link.txt', str(self.out)])
        declared.assert_called_once_with(self.images, self.out, ['-ml', '-m4-single-only'])
        with mock.patch.object(decls, 'failures', return_value=['main: a problem']), \
                self.assertRaisesRegex(SystemExit, r': main: a problem\Z'):
            verify.main(['--abi=-ml', 'link.txt', str(self.out)])

        def failing(spec, problem):
            # verify fails on problem alone, and checks no declaration
            with mock.patch.object(decls, 'failures', side_effect=AssertionError('checked the declarations')), \
                    self.assertRaisesRegex(SystemExit, rf'\A[^\n]*: {problem}\Z'):
                verify.main(['--abi=-ml', spec, str(self.out)])

        # a selection that takes no archive member fails the selection check on ovb's
        with mock.patch.object(gcclink, 'selected', objects_alone(gcclink.selected)):
            failing('link.txt', r"ovb: its final pass takes libovb\.a\(helper\.o\), which selected\(\) doesn't")
        # main's final pass takes ova_table 4 bytes past ova's, which fails the binding check
        values = ld.resolve(self.images, self.layouts(self.images))
        values['main']['ova_table'] += 4
        self.finals(self.images, values)
        failing('link.txt', r"main: ova_table is (0x0c9000[0-9a-f]{2}) in \S+main/final\.elf, and ova's ova_table in "
                            r'the 0x0c view is (?!\1)0x0c9000[0-9a-f]{2} in \S+ova/final\.elf')
        # head.o's function opens ova, so step lies past ova's origin, and this anchor of step at the origin fails the
        # anchor check
        (self.root / 'anchored.txt').write_text(test_ld.SPEC + 'anchor  ova  step  0\n')
        ld.main(['anchored.txt', str(self.out)])
        failing('anchored.txt', r'ova: step lies at 0x0c9000[0-9a-f]{2} in \S+, and its anchor puts it at 0x0c900000, '
                                r'0x0 bytes past its origin')

    def test_holds_each_anchor_to_its_place_and_size(self):
        # record.o's .data, ova's head record, lies at ova's origin: ova_record's 8 bytes, at the start of DHEAD's run
        images = linking(self.images, 'ova', 'ova.o', 'record.o')
        images = {**images, 'ova': images['ova']._replace(
            heads=[gcclink.Piece('DHEAD', 'record.o', '.data')],
            anchors=[gcclink.Anchor('ova_record', 0, 8), gcclink.Anchor('_startof_DHEAD', 0, None)])}
        finals = ld.coordinate(images, self.out)[2]
        self.assertEqual(verify.unanchored(images, finals), [])
        # an anchor fails the check where its offset or its size is another. So does one of a name the image lacks,
        # or of a run's size, which no allocated section holds
        for anchor, problem in ((gcclink.Anchor('ova_record', 4, 8),
                                 r'ova: ova_record lies at 0x0c900000 in \S+, and its anchor puts it at 0x0c900004, '
                                 r'0x4 bytes past its origin'),
                                (gcclink.Anchor('ova_record', 0, 4),
                                 r'ova: ova_record holds 8 bytes in \S+, and its anchor gives 4'),
                                (gcclink.Anchor('ova_none', 0, None),
                                 r'ova: ova_none anchors it at 0x0c900000, 0x0 bytes past its origin, and \S+ names '
                                 r'no such symbol'),
                                (gcclink.Anchor('_sizeof_DHEAD', 8, None),
                                 r'ova: _sizeof_DHEAD anchors it at 0x0c900008, 0x8 bytes past its origin, and \S+ '
                                 r'names no such symbol')):
            with self.subTest(anchor):
                wrong = {**images, 'ova': images['ova']._replace(anchors=[anchor])}
                problems = verify.unanchored(wrong, finals)
                self.assertEqual(len(problems), 1, problems)
                self.assertRegex(problems[0], rf'\A{problem}\Z')
        # so does an anchor of a name two symbols carry: record.o linked twice puts two ova_records in the head
        twice = {**images, 'ova': linking(images, 'ova', 'ova.o', 'record.o', 'record.o')['ova']._replace(
            anchors=[gcclink.Anchor('ova_record', 0, 8)])}
        problems = verify.unanchored(twice, ld.coordinate(twice, self.out)[2])
        self.assertEqual(len(problems), 1, problems)
        self.assertRegex(problems[0], r'\Aova: ova_record anchors it at 0x0c900000, 0x0 bytes past its origin, and '
                                      r'\S+ names 2 by that name\Z')

    def test_a_provider_moved_after_its_layout_pass_fails_the_binding_check_and_its_consumer_moved_doesn_t(self):
        # ova's final pass links ova_moved.o, whose constant moves ova_table on, where main took ova_table from ova's
        # layout pass. main's final pass linking late.o too moves main's own bss on, and no import
        layouts = self.layouts(self.images)
        values = ld.resolve(self.images, layouts)
        problems = verify.misbound(self.images, self.finals(linking(self.images, 'ova', 'ova_moved.o'), values))
        self.assertTrue(any(problem.startswith('main: ova_table is ') for problem in problems), problems)
        finals = self.finals(linking(self.images, 'main', 'main.o', 'late.o'), values)
        self.assertEqual(verify.misbound(self.images, finals), [])

    def test_a_wrong_import_value_fails_the_binding_check_alone(self):
        layouts = self.layouts(self.images)
        values = ld.resolve(self.images, layouts)
        values['main']['ova_table'] += 4
        finals = self.finals(self.images, values)
        self.assertEqual(ld.moved(layouts, finals), [])
        problems = verify.misbound(self.images, finals)
        self.assertEqual(len(problems), 1, problems)
        self.assertRegex(problems[0], r"^main: ova_table is (0x0c9000[0-9a-f]{2}) in .*main/final\.elf, and ova's "
                                      r'ova_table in the 0x0c view is (?!\1)0x0c9000[0-9a-f]{2} in .*ova/final\.elf$')

    def test_a_relocation_the_binding_check_can_t_trace_to_its_input_s_word_fails_it(self):
        # main.o's .data holds main_steps' two words, and main_slot's word as 12 under a relocation against ova_table.
        # Each case changes main's final pass as its name says. Placing the section in another output section, where
        # final.elf keeps no relocation, or at another size, fails each of its three words
        layouts = self.layouts(self.images)
        finals = self.finals(self.images, ld.resolve(self.images, layouts))
        main = finals['main']
        own = gcclink.exports(main.elf)
        slot, steps = own['main_slot'], own['main_steps'].value
        data = main.elf.sections[slot.section]
        at = slot.value - data.address
        word = int.from_bytes(data.data[at:at + 4], 'little')
        flipped = data._replace(data=data.data[:at] + bytes([data.data[at] ^ 1]) + data.data[at + 1:])
        step = next(index for index, symbol in enumerate(main.elf.symbols) if symbol.name == '_ova_step')
        imported = ((steps, 'ova_step'), (steps + 4, 'ovb_step'), (slot.value, 'ova_table'))

        def placing(change):
            return main._replace(placements=[change(placed) if (placed.file, placed.section) == ('main.o', '.data')
                                             else placed for placed in main.placements])

        def relocating(change):
            # main, with its final relocation at main_slot changed, or gone where change gives None
            relocations = [change(relocation) if relocation.offset == slot.value else relocation
                           for relocation in main.elf.relocations[slot.section]]
            return main._replace(elf=main.elf._replace(relocations={
                **main.elf.relocations, slot.section: [relocation for relocation in relocations if relocation]}))

        for case, linked, problems in (
                ('a bit flipped', main._replace(elf=main.elf._replace(
                    sections=[flipped if section is data else section for section in main.elf.sections])),
                 [f"main: the relocation at {hexa(slot.value)}, against ova_table, from main.o's .data, holds "
                  f'{hexa(word ^ 1)}, and ova_table plus 12 is {hexa(word)}']),
                ('another output section', placing(lambda placed: placed._replace(output='.rodata')),
                 [f"main: the relocation at {hexa(offset)}, against {name}, {problem}" for offset, name in imported
                  for problem in (f"lies in no input section that {main.elf.where}'s map places",
                                  f"from main.o's .data, is missing from {main.elf.where}")]),
                ('another size', placing(lambda placed: placed._replace(size=placed.size + 4)),
                 [f"main: the relocation at {hexa(offset)}, against {name}, comes from no one relocation of "
                  "main.o's .data" for offset, name in imported]),
                ('against another import', relocating(lambda relocation: relocation._replace(symbol=step)),
                 [f"main: the relocation at {hexa(slot.value)}, against ova_step, comes from no one relocation of "
                  "main.o's .data"]),
                ('with an addend of its own', relocating(lambda relocation: relocation._replace(addend=4)),
                 [f"main: the relocation at {hexa(slot.value)}, against ova_table, from main.o's .data, has an "
                  "addend of its own, 4, which the binding check doesn't read: it reads the addend in the word, which "
                  'ld adds alone']),
                ('gone', relocating(lambda relocation: None),
                 [f"main: the relocation at {hexa(slot.value)}, against ova_table, from main.o's .data, is "
                  f'missing from {main.elf.where}'])):
            with self.subTest(case):
                self.assertEqual(sorted(verify.misbound(self.images, {**finals, 'main': linked})), sorted(problems))

    def test_the_binding_check_takes_each_input_as_the_passes_read_it(self):
        # misbound() reads no input again, libovb.a's member among them
        layouts = self.layouts(self.images)
        finals = self.finals(self.images, ld.resolve(self.images, layouts))
        with mock.patch.object(elf, 'content', side_effect=AssertionError('read again')):
            self.assertEqual(verify.misbound(self.images, finals), [])

    def test_a_wrong_view_fails_the_binding_check(self):
        # main's final pass takes ova_step in ova's own view, 0x0c, where its import gives 0x8c
        layouts = self.layouts(self.images)
        wrong = importing(self.images, 'main', ova_step=self.images['main'].imports['ova_step']._replace(view='0x0c'))
        finals = self.finals(self.images, ld.resolve(wrong, layouts))
        self.assertEqual(ld.moved(layouts, finals), [])
        problems = verify.misbound(self.images, finals)
        self.assertEqual(len(problems), 1, problems)
        self.assertRegex(problems[0], r"^main: ova_step is 0x0c9000[0-9a-f]{2} in .*, and ova's step in the 0x8c "
                                      r'view is 0x8c9000[0-9a-f]{2} in ')

    def test_holds_each_final_pass_to_the_inputs_selected_takes(self):
        # main's import of ovb_helper roots it in ovb, so selected() takes libovb.a's helper.o, as ld does. A
        # selection that takes no member disagrees with ovb's final map
        finals = ld.coordinate(self.images, self.out)[2]
        self.assertEqual(verify.unselected(self.images, finals), [])
        with mock.patch.object(gcclink, 'selected', objects_alone(gcclink.selected)):
            self.assertEqual(verify.unselected(self.images, finals),
                             ["ovb: its final pass takes libovb.a(helper.o), which selected() doesn't"])

    def test_a_wrong_alias_or_constant_fails_the_binding_check(self):
        # the final pass binds main_count to main_counter and main_area to 0x8c0f0000, and the images the check reads
        # give main_call and 0x8c0f0004
        images = linking(self.images, 'main', 'main.o', 'names.o')
        images = {**images, 'main': images['main']._replace(aliases={'main_count': 'main_counter'},
                                                             constants={'main_area': 0x8C0F0000})}
        layouts = self.layouts(images)
        finals = self.finals(images, ld.resolve(images, layouts))
        own = gcclink.exports(finals['main'].elf)
        wrong = {**images, 'main': images['main']._replace(aliases={'main_count': 'main_call'},
                                                            constants={'main_area': 0x8C0F0004})}
        self.assertEqual(verify.misbound(wrong, finals), [
            f"main: main_count is {hexa(own['main_counter'].value)} in {finals['main'].elf.where}, and its "
            f"main_call is {hexa(own['main_call'].value)}",
            f"main: main_area is 0x8c0f0000 in {finals['main'].elf.where}, and its constant is 0x8c0f0004"])

    def test_refuses_any_other_relocation_against_an_import(self):
        # rel.o holds ova_step less its own address, an R_SH_REL32
        images = linking(self.images, 'main', 'main.o', 'rel.o')
        layouts = self.layouts(images)
        finals = self.finals(images, ld.resolve(images, layouts))
        relocated = gcclink.exports(finals['main'].elf)['main_rel'].value
        self.assertEqual(verify.misbound(images, finals), [
            f"main: the relocation at {hexa(relocated)}, against ova_step, is of type 2, which the binding check "
            "doesn't read: it reads R_SH_DIR32, type 1, alone"])

    def test_refuses_a_relocation_against_an_import_with_an_addend_of_its_own(self):
        # ld writes ova_table plus the word, 16, and leaves the relocation's own addend, 4, aside
        images = linking(self.images, 'main', 'main.o', 'reloc.o')
        layouts = self.layouts(images)
        finals = self.finals(images, ld.resolve(images, layouts))
        table = gcclink.exports(finals['ova'].elf)['ova_table'].value
        relocated = gcclink.exports(finals['main'].elf)['main_reloc'].value
        self.assertEqual(words(finals['main'], 'main_reloc', 1), [table + 16])
        self.assertEqual(verify.misbound(images, finals), [
            f"main: the relocation at {hexa(relocated)}, against ova_table, from reloc.o's .data.reloc, has an "
            "addend of its own, 4, which the binding check doesn't read: it reads the addend in the word, which ld "
            'adds alone'])


if __name__ == '__main__':
    unittest.main()
