import unittest

import float_asm
import floats
from support import tree


class Assembly(unittest.TestCase):
    # 16777217.0 lies on a tie, which asmsh reads as the even 0x4b800000 and GNU as as 0x4b800001. 1.00000006 lies
    # 0.0033 of a step past the tie 1 + 2**-24, and asmsh reads it as the even 0x3f800000, GNU as as 0x3f800001.
    # 1.4S-45, below the normal floats, draws asmsh's warning 842. 3.4S+38 lies 0.24 of a step past 0x7f7fc99e, and
    # -0.1 0.8 past 0xbdcccccc, and each assembler reads each as its nearest float. 1.000000074 lies 0.6208 of a step
    # past 1.0, within an eighth of a step past the tie, and 1.000000075 0.6291 past it, beyond; both assemblers read
    # each as 0x3f800001, and the check fails on the first. 3.5S+38 lies past the largest float, which draws asmsh's
    # warning 840; and H'3F800000 is no F' constant
    TEXT = ('_constants:\n        .FDATA.S    F\'16777217.0\n        .FDATA.S    F\'1.0, F\'1.00000006\n'
            '        .FDATA.S    F\'3.4S+38,F\'-0.1\n        .FDATA.S    F\'1.4S-45 ; a comment\n'
            '_ties:  .FDATA.S    F\'16777219.0\n        .FDATA.S    F\'1.000000074, F\'1.000000075\n'
            '        .FDATA.S    F\'3.5S+38, H\'3F800000\n')

    def test_names_the_file_and_line_of_each_fdata_operand_it_fails_on(self):
        tie = 'on a tie or past one by less than an eighth of a step, where asmsh and GNU as can read two floats'
        self.assertEqual(float_asm.assembled(self.TEXT, 'a.src'), [
            f"a.src:2: F'16777217.0: {tie}",
            f"a.src:3: F'1.00000006: {tie}",
            "a.src:5: F'1.4S-45: past the normal floats",
            f"a.src:6: F'16777219.0: {tie}",
            f"a.src:7: F'1.000000074: {tie}",
            "a.src:8: F'3.5S+38: past the normal floats",
            "a.src:8: H'3F800000: no F' constant asmsh reads",
        ])

    def test_fails_on_a_constant_whose_nearest_float_lies_past_the_largest(self):
        # 3.4028235S+38 lies 0.17 of a step past the largest float, 0x7f7fffff, and 3.4028235675956350S+38 0.499, and
        # asmsh reads each as that float with no warning. 3.4028236S+38 lies 0.66 of a step past it, beyond the tie and
        # the eighth of a step after it, and asmsh warns 840 that it and its negation overflow
        text = ("        .FDATA.S    F'3.4028235S+38, F'-3.4028235S+38, F'3.4028235675956350S+38\n"
                "        .FDATA.S    F'3.4028236S+38, F'-3.4028236S+38\n")
        past = 'past the normal floats'
        self.assertEqual(float_asm.assembled(text, 'a.src'),
                         [f"a.src:2: F'3.4028236S+38: {past}", f"a.src:2: F'-3.4028236S+38: {past}"])

    def test_reads_a_constant_by_its_digits_and_builds_no_power_past_the_floats(self):
        # zero and 1 under exponents far past the floats, which the check reads with no power of ten computed, and
        # 1.0 after 5,000 leading zeros
        text = ("        .FDATA.S    F'0S+999999999, F'1S+999999999, F'1S-999999999\n"
                f"        .FDATA.S    F'0.{'0' * 5000}1S+5001\n")
        past = 'past the normal floats'
        self.assertEqual(float_asm.assembled(text, 'a.src'),
                         [f"a.src:1: F'1S+999999999: {past}", f"a.src:1: F'1S-999999999: {past}"])

    def test_passes_constants_both_read_as_one_float_and_fails_naming_the_tool(self):
        with tree({'a.src': '        .FDATA.S    F\'0.00390625, F\'3.4S+38\n', 'b.src': self.TEXT}):
            floats.main(['--assembly', 'a.src'])
            with self.assertRaisesRegex(SystemExit, r"\A[^\n]*: b\.src:2: F'16777217\.0: on a tie"):
                floats.main(['--assembly', 'b.src'])


if __name__ == '__main__':
    unittest.main()
