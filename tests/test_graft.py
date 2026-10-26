import re
import unittest
from pathlib import Path

import graft
from support import tree


# the functions each text holds, in order
FUNCTIONS = ('prepare_camera', 'apply_channels', 'find_camera_key', 'find_camera_angle_key', 'linear_camera_angle',
             'linear_camera_vector', 'cubic_camera_vector', 'position_channel', 'direction_channel',
             'interest_channel', 'roll_channel', 'angle_channel', 'njCameraAction', 'njCameraMotion')


def ins(mnemonic, operands='', comment=None):
    """An instruction line as R10 writes one, a comment at column 33 or past the operands."""
    line = f'          {mnemonic:<12}{operands}'.rstrip()
    return line if comment is None else f'{line} '.ljust(33) + f'; {comment}'


def label(name, comment=None):
    return f'{name}:'.ljust(33) + (f'; {comment}' if comment else ' ')


def directive(name, value=''):
    return f'          {"." + name:<12}{value}'.rstrip()


def statics(*pairs):
    lines = [directive('SECTION', 'BSG,DATA,ALIGN=4')]
    for name, at in pairs:
        lines += [label(at, f'static: {name}'), directive('RES.L', '1')]
    return lines


def text(functions, labels, pool, header, after):
    """header, each function, a return or its lines, under its label, then pool and after."""
    lines = [*header, directive('SECTION', 'PSG,CODE,ALIGN=4')]
    for name in FUNCTIONS:
        lines += [label(labels[name], f'function: {name}'), *functions.get(name, [ins('RTS'), ins('NOP')])]
    return [*lines, *pool, *after, directive('END')]


def numbered(first):
    """Each function's label: its own number from first, but for the two exported functions."""
    labels = {name: f'L{number}' for number, name in enumerate(FUNCTIONS[:-2], first)}
    return dict(labels, njCameraAction='_njCameraAction', njCameraMotion='_njCameraMotion')


# NEW: each function a return, then the pool that ends the code, a pad and a long, and the statics, L91 to L94
NEW_LABELS = numbered(1)
NEW = text({}, NEW_LABELS, [label('L90'), directive('RES.W', '1'), directive('DATA.L', 'L91')],
           [directive('IMPORT', '_njRollCameraInterest'), directive('IMPORT', '_njSetCamera'),
            directive('EXPORT', '_njCameraAction'), directive('EXPORT', '_njCameraMotion')],
           statics(('playback', 'L91'), ('factor_count', 'L92'), ('motion_type', 'L93'), ('working_camera', 'L94')))

# DONOR: the four kept functions call each function SHARED names, and load each static STATE names and each symbol
# IMPORTS names, from a pool after apply_channels and the pool that ends the code
DONOR_LABELS = numbered(21)
DONOR_KEPT = {
    'apply_channels': [
        ins('STS.L', 'PR,@-R15'), ins('BSR', DONOR_LABELS['position_channel']), ins('NOP'),
        ins('MOV.W', 'L80,R3', "H'0080"), ins('MOV.L', 'L80+4,R5', "H'00000014+L71"),
        ins('BSR', DONOR_LABELS['interest_channel']), ins('NOP'),
        ins('BSR', DONOR_LABELS['direction_channel']), ins('NOP'),
        ins('BSR', DONOR_LABELS['roll_channel']), ins('NOP'), ins('BSR', DONOR_LABELS['angle_channel']), ins('NOP'),
        ins('MOV.L', 'L80+8,R4', 'L74'), ins('LDS.L', '@R15+,PR'), ins('MOV.L', 'L80+12,R3', '_njSetCamera'),
        ins('JMP', '@R3'), ins('NOP'),
        label('L80'), directive('DATA.W', "H'0080"), directive('RES.W', '1'), directive('DATA.L', "H'00000014+L71"),
        directive('DATA.L', 'L74'), directive('DATA.L', '_njSetCamera')],
    'roll_channel': [
        ins('STS.L', 'PR,@-R15'), ins('BSR', DONOR_LABELS['find_camera_angle_key']), ins('NOP'),
        ins('BSR', DONOR_LABELS['linear_camera_angle']), ins('NOP'), ins('TST', 'R0,R0'), ins('BT', 'L60'),
        ins('MOV.L', 'L81+12,R4', 'L74'), ins('MOV.L', 'L81+8,R3', '_njRollCameraInterest'), ins('JSR', '@R3'),
        ins('NOP'), label('L60'), ins('LDS.L', '@R15+,PR'), ins('RTS'), ins('NOP')],
    'njCameraAction': [
        ins('STS.L', 'PR,@-R15'), ins('BSR', DONOR_LABELS['prepare_camera']), ins('NOP'),
        ins('MOV.L', 'L81+16,R2', 'L72'), ins('MOV.L', 'L81+4,R3', 'L73'), ins('MOV.W', 'L81,R0', "H'0080"),
        ins('BRA', DONOR_LABELS['apply_channels']), ins('LDS.L', '@R15+,PR')],
    'njCameraMotion': [
        ins('STS.L', 'PR,@-R15'), ins('BSR', DONOR_LABELS['prepare_camera']), ins('NOP'),
        ins('MOV.W', 'L81+2,R0', "H'0100"), ins('BRA', DONOR_LABELS['apply_channels']), ins('LDS.L', '@R15+,PR')],
}
DONOR = text(DONOR_KEPT, DONOR_LABELS,
             [label('L81'), directive('DATA.W', "H'0080"), directive('DATA.W', "H'0100"), directive('DATA.L', 'L73'),
              directive('DATA.L', '_njRollCameraInterest'), directive('DATA.L', 'L74'), directive('DATA.L', 'L72')],
             [directive('IMPORT', '_njRollCameraInterest'), directive('IMPORT', '_njSetCamera'),
              directive('IMPORT', '_njSetCameraAngle'), directive('EXPORT', '_njCameraAction'),
              directive('EXPORT', '_njCameraMotion')],
             statics(('playback', 'L71'), ('factor_count', 'L72'), ('motion_type', 'L73'), ('working_camera', 'L74')))

# the four as OUT holds them: each call of a shared function NEW's, each kept label its new one, numbered past NEW's
# L94 but for the exported functions', and each load an item appended to L90, whose own items take 6 bytes
GRAFTED = [
    label('L95', 'function: apply_old_channels'),
    ins('STS.L', 'PR,@-R15'), ins('BSR', NEW_LABELS['position_channel']), ins('NOP'),
    ins('MOV.W', 'L90+6,R3', "H'0080"), ins('MOV.L', 'L90+10,R5', "H'00000014+L91"),
    ins('BSR', NEW_LABELS['interest_channel']), ins('NOP'), ins('BSR', NEW_LABELS['direction_channel']), ins('NOP'),
    ins('BSR', 'L96'), ins('NOP'), ins('BSR', NEW_LABELS['angle_channel']), ins('NOP'),
    ins('MOV.L', 'L90+14,R4', 'L94'), ins('LDS.L', '@R15+,PR'), ins('MOV.L', 'L90+18,R3', '_njSetCamera'),
    ins('JMP', '@R3'), ins('NOP'),
    label('L96', 'function: old_roll_channel'),
    ins('STS.L', 'PR,@-R15'), ins('BSR', NEW_LABELS['find_camera_angle_key']), ins('NOP'),
    ins('BSR', NEW_LABELS['linear_camera_angle']), ins('NOP'), ins('TST', 'R0,R0'), ins('BT', 'L97'),
    ins('MOV.L', 'L90+14,R4', 'L94'), ins('MOV.L', 'L90+22,R3', '_njRollCameraInterest'), ins('JSR', '@R3'),
    ins('NOP'), label('L97'), ins('LDS.L', '@R15+,PR'), ins('RTS'), ins('NOP'),
    label('_njCameraActionOld', 'function: njCameraActionOld'),
    ins('STS.L', 'PR,@-R15'), ins('BSR', NEW_LABELS['prepare_camera']), ins('NOP'),
    ins('MOV.L', 'L90+26,R2', 'L92'), ins('MOV.L', 'L90+30,R3', 'L93'), ins('MOV.W', 'L90+6,R0', "H'0080"),
    ins('BRA', 'L95'), ins('LDS.L', '@R15+,PR'),
    label('_njCameraMotionOld', 'function: njCameraMotionOld'),
    ins('STS.L', 'PR,@-R15'), ins('BSR', NEW_LABELS['prepare_camera']), ins('NOP'),
    ins('MOV.W', 'L90+8,R0', "H'0100"), ins('BRA', 'L95'), ins('LDS.L', '@R15+,PR')]
# the items appended to L90: one for each width and expression, the words, then the longs, each width in the order
# of its first load, DONOR's statics NEW's
ITEMS = [directive('DATA.W', "H'0080"), directive('DATA.W', "H'0100"), directive('DATA.L', "H'00000014+L91"),
         directive('DATA.L', 'L94'), directive('DATA.L', '_njSetCamera'), directive('DATA.L', '_njRollCameraInterest'),
         directive('DATA.L', 'L92'), directive('DATA.L', 'L93')]


def sources(new, donor):
    """The two texts, each line ending as R10 ends them."""
    return {'new.src': ''.join(f'{line}\r\n' for line in new), 'donor.src': ''.join(f'{line}\r\n' for line in donor)}


def run(new=NEW, donor=DONOR):
    """OUT's lines, without the CR LF that ends each, as NEW's do."""
    with tree(sources(new, donor)):
        graft.main(['new.src', 'donor.src', 'out.src'])
        return Path('out.src').read_bytes().decode('ascii').split('\r\n')[:-1]


def replaced(lines, old, new, nth=-1):
    """lines with the nth of the lines equal to old, the last by default, replaced by new, a line or a list."""
    at = [i for i, line in enumerate(lines) if line == old][nth]
    return [*lines[:at], *(new if isinstance(new, list) else [new]), *lines[at + 1:]]


class Graft(unittest.TestCase):
    def test_sets_the_four_before_the_pool_that_ends_the_code(self):
        exports = NEW.index(directive('SECTION', 'PSG,CODE,ALIGN=4'))
        pool = NEW.index(label('L90'))
        self.assertEqual(run(), [
            *NEW[:exports], directive('EXPORT', '_njCameraActionOld'), directive('EXPORT', '_njCameraMotionOld'),
            *NEW[exports:pool], *GRAFTED, *NEW[pool:pool + 3], *ITEMS, *NEW[pool + 3:]])

    def test_refuses_another_members_text(self):
        donor = replaced(DONOR, label(DONOR_LABELS['linear_camera_angle'], 'function: linear_camera_angle'),
                         label(DONOR_LABELS['linear_camera_angle'], 'function: linear_angle'))
        with self.assertRaisesRegex(SystemExit, r"donor\.src: functions .*, linear_angle, .*, not njCameraMotion_'s"):
            run(donor=donor)

    def test_refuses_a_symbol_no_table_binds(self):
        donor = replaced(DONOR, ins('BSR', DONOR_LABELS['angle_channel']),
                         ins('BSR', DONOR_LABELS['cubic_camera_vector']))
        with self.assertRaisesRegex(SystemExit, r'donor\.src:\d+: cubic_camera_vector, which no table here binds'):
            run(donor=donor)

    def test_refuses_a_name_the_four_dont_use(self):
        donor = replaced(DONOR, ins('BSR', DONOR_LABELS['angle_channel']), ins('NOP'))
        with self.assertRaisesRegex(SystemExit, r'donor\.src: the four use no angle_channel'):
            run(donor=donor)

    def test_refuses_an_import_imports_does_not_name(self):
        new = replaced(NEW, directive('IMPORT', '_njSetCamera'),
                       [directive('IMPORT', '_njSetCamera'), directive('IMPORT', '_njSetCameraAngle')])
        donor = replaced(DONOR, directive('DATA.L', '_njSetCamera'),
                         [directive('DATA.L', '_njSetCamera'), directive('DATA.L', '_njSetCameraAngle')])
        donor = replaced(donor, ins('MOV.L', 'L80+12,R3', '_njSetCamera'),
                         [ins('MOV.L', 'L80+12,R3', '_njSetCamera'), ins('MOV.L', 'L80+16,R2', '_njSetCameraAngle')])
        with self.assertRaisesRegex(SystemExit, r'donor\.src:\d+: _njSetCameraAngle, which no table here binds'):
            run(new=new, donor=donor)

    def test_refuses_a_static_new_lacks(self):
        new = replaced(NEW, label('L94', 'static: working_camera'), label('L94', 'static: camera'))
        with self.assertRaisesRegex(SystemExit, r'new\.src: no static working_camera, which donor\.src:\d+ uses'):
            run(new=new)

    def test_refuses_two_statics_of_a_name_state_gives(self):
        # R10's ; static: comment gives the variable's name alone, so two functions' statics can share one
        for path, lines, name in (('new', NEW, 'working_camera'), ('donor', DONOR, 'playback')):
            first = [line.endswith(f'; static: {name}') for line in lines].index(True) + 1
            twice = replaced(lines, directive('END'), [label('L99', f'static: {name}'), directive('RES.L', '1'),
                                                       directive('END')])
            with self.subTest(path), self.assertRaisesRegex(
                    SystemExit, rf'{path}\.src:{len(lines)}: a second static {name}; line {first} is the first'):
                run(**{path: twice})

    def test_refuses_an_import_new_lacks(self):
        new = replaced(NEW, directive('IMPORT', '_njRollCameraInterest'), [])
        with self.assertRaisesRegex(SystemExit, r'new\.src: no \.IMPORT of _njRollCameraInterest'):
            run(new=new)

    def test_refuses_a_pool_among_a_kept_functions_statements(self):
        donor = replaced(DONOR, ins('MOV.L', 'L80+8,R4', 'L74'),
                         [ins('BRA', 'L61'), ins('NOP'), label('L82'), directive('DATA.L', 'L71'), label('L61'),
                          ins('MOV.L', 'L80+8,R4', 'L74')])
        with self.assertRaisesRegex(SystemExit, r"donor\.src: a pool among apply_channels's statements"):
            run(donor=donor)

    def test_refuses_a_directive_among_a_kept_functions_statements(self):
        for name, value in (('DATA.L', "H'DEADBEEF"), ('ALIGN', '4')):
            donor = replaced(DONOR, ins('MOV.L', 'L80+8,R4', 'L74'),
                             [ins('MOV.L', 'L80+8,R4', 'L74'), directive(name, value)])
            with self.subTest(directive=name), self.assertRaisesRegex(
                    SystemExit, rf"donor\.src:\d+: a directive, \.{re.escape(name)}, among apply_channels's "
                                r'statements'):
                run(donor=donor)

    def test_refuses_a_load_of_part_of_an_item_or_a_mova(self):
        for load in (ins('MOV.W', 'L80+4,R5'), ins('MOV.L', 'L80+2,R5'), ins('MOVA', 'L80+4,R0')):
            donor = replaced(DONOR, ins('MOV.L', 'L80+4,R5', "H'00000014+L71"), load)
            with self.subTest(load=load.split()), \
                    self.assertRaisesRegex(SystemExit, r'donor\.src:\d+: no load of a whole item of L80'):
                run(donor=donor)

    def test_refuses_a_label_line_that_holds_a_statement(self):
        donor = replaced(DONOR, label('L60'), 'L60:      NOP')
        with self.assertRaisesRegex(SystemExit, r"donor\.src:\d+: a label line the walk can't read"):
            run(donor=donor)

    def test_refuses_a_pool_item_that_is_no_constant_and_no_symbol_plus_constants(self):
        for expression in ('-L73', 'L73+L72', 'L73*2', 'L73+'):
            donor = replaced(DONOR, directive('DATA.L', 'L73'), directive('DATA.L', expression))
            with self.subTest(expression=expression), self.assertRaisesRegex(
                    SystemExit, rf'donor\.src:\d+: loads {re.escape(expression)}, which is no constant and no symbol plus '
                                r'constants'):
                run(donor=donor)

    def test_refuses_a_byte_past_ascii(self):
        # in a comment ahead of the code, which OUT doesn't take
        files = sources(NEW, DONOR)
        files['donor.src'] = b'; caf\xe9\r\n' + files['donor.src'].encode()
        with tree(files), self.assertRaisesRegex(SystemExit, r'donor\.src: not R10 assembly text: a byte past ASCII'):
            graft.main(['new.src', 'donor.src', 'out.src'])

    def test_refuses_a_function_without_a_final_branch_and_slot(self):
        donor = replaced(DONOR, ins('JMP', '@R3'), ins('JSR', '@R3'))
        with self.assertRaisesRegex(SystemExit, r'donor\.src: apply_channels does not end with a delayed return or '
                                                r'unconditional branch and its slot'):
            run(donor=donor)
        new = replaced(NEW, ins('RTS'), ins('NOP'))
        with self.assertRaisesRegex(SystemExit, r"new\.src: no pool after njCameraMotion's last statement"):
            run(new=new)
        # a branch followed by a label or a directive, so that its slot is whatever comes next
        for slot in (label('L89'), directive('ALIGN', '4')):
            new = replaced(NEW, ins('NOP'), slot)
            with self.subTest(slot=slot.split()), \
                    self.assertRaisesRegex(SystemExit, r"new\.src: no pool after njCameraMotion's last statement"):
                run(new=new)

    def test_refuses_any_but_one_pool_after_the_last_statement_to_the_end(self):
        # njCameraMotion's only pool among its statements, behind a branch, and two pools after them
        at = NEW.index(label('L90'))
        pool = NEW[at:at + 3]
        for shape, new in (('among', [*NEW[:at - 2], ins('BRA', 'L89'), ins('NOP'), *pool, label('L89'),
                                      *NEW[at - 2:at], *NEW[at + 3:]]),
                           ('two', [*NEW[:at], label('L88'), directive('DATA.L', 'L92'), *NEW[at:]])):
            with self.subTest(shape), \
                    self.assertRaisesRegex(SystemExit, r"new\.src: no pool after njCameraMotion's last statement"):
                run(new=new)

    def test_refuses_a_global_label_not_exported(self):
        donor = replaced(DONOR, directive('EXPORT', '_njCameraMotion'), [])
        with self.assertRaisesRegex(SystemExit,
                                    r"donor\.src: njCameraMotion's label _njCameraMotion is global but not exported"):
            run(donor=donor)

    def test_refuses_an_export_after_the_code_section_starts(self):
        # after the code section's directive, after a function's return, and in the data section after the code
        for line in (directive('SECTION', 'PSG,CODE,ALIGN=4'), ins('RTS'), directive('SECTION', 'BSG,DATA,ALIGN=4')):
            new = replaced(NEW, directive('EXPORT', '_njCameraMotion'), [])
            new = replaced(new, line, [line, directive('EXPORT', '_njCameraMotion')], nth=0)
            at = new.index(directive('EXPORT', '_njCameraMotion')) + 1
            with self.subTest(after=line.split()), \
                    self.assertRaisesRegex(SystemExit, rf'new\.src:{at}: an \.EXPORT after the code section starts'):
                run(new=new)

    def test_writes_nothing_when_it_fails(self):
        donor = replaced(DONOR, ins('BSR', DONOR_LABELS['angle_channel']), ins('NOP'))
        with tree(sources(NEW, donor)):
            with self.assertRaises(SystemExit):
                graft.main(['new.src', 'donor.src', 'out.src'])
            self.assertFalse(Path('out.src').exists())


if __name__ == '__main__':
    unittest.main()
