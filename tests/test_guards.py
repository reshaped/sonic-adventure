import unittest

import guards
from support import tree

# a header and a source that test each switch by #ifdef, one with an #else, one nested in the other
FILES = {'sdk/include/late.h': '#ifndef LATE_H\n#define LATE_H\n#ifdef NONMATCHING\nint late;\n#endif\n#endif\n',
         'src/a.c': ('#include <late.h>\n#ifdef NONMATCHING\nint a;\n#else\nint b;\n#endif\n'
                     '#ifdef DREAMCAST\n#ifdef NONMATCHING\nint d;\n#endif\n#endif\n')}
# the rule a problem with a conditional's test says, guards.RULE
RULE = 'a guard tests NONMATCHING or DREAMCAST'


def check(files):
    """guards.py's problems with the files, a {path: text} map, each named to it, each without the tool's name."""
    with tree(files):
        try:
            guards.main(sorted(files))
        except SystemExit as failed:
            return [line.split(': ', 1)[1] for line in str(failed).splitlines()]
    return []


class Spellings(unittest.TestCase):
    def test_takes_an_ifdef_of_each_switch_with_an_else(self):
        self.assertEqual(check(FILES), [])

    def test_refuses_any_other_test_of_a_switch(self):
        for test in ('#ifndef NONMATCHING', '#if defined(DREAMCAST)', '#if NONMATCHING', '#ifdef X\n#elifdef DREAMCAST',
                     '#ifdef X\n#elifndef NONMATCHING', '#ifdef X\n#elif defined DREAMCAST'):
            with self.subTest(test):
                source = f'{test}\nint e;\n#endif\n'
                self.assertRegex('\n'.join(check({**FILES, 'src/e.c': source})),
                                 rf'src/e\.c:{test.count(chr(10)) + 1}: #\w+ of \w+: {RULE} by #ifdef alone')

    def test_refuses_an_elif_in_an_ifdef_of_a_switch(self):
        for later in ('#elif 1', '#elifdef X', '#elifndef X'):
            with self.subTest(later):
                source = f'#ifdef DREAMCAST\nint e;\n{later}\nint f;\n#endif\n'
                self.assertIn(f'src/e.c:3: #{later[1:].split()[0]} in an #ifdef of DREAMCAST, whose one other group '
                              'is #else', check({**FILES, 'src/e.c': source}))

    def test_refuses_a_conditional_that_names_matching_port_gcc_or_clang(self):
        for test in ('#ifdef MATCHING', '#ifdef PORT', '#ifdef GCC', '#ifndef __clang__', '#if defined(__clang__)',
                     '#ifdef X\n#elifdef GCC', '#ifdef X\n#elif defined MATCHING'):
            with self.subTest(test):
                source = f'{test}\nint e;\n#endif\n'
                self.assertRegex('\n'.join(check({**FILES, 'src/e.c': source})),
                                 rf'src/e\.c:{test.count(chr(10)) + 1}: #\w+ of (MATCHING|PORT|GCC|__clang__): '
                                 f'{RULE}$')

    def test_refuses_a_definition_of_a_switch(self):
        for directive in ('#define NONMATCHING', '#define DREAMCAST 1', '#undef NONMATCHING', '#undef DREAMCAST'):
            with self.subTest(directive):
                self.assertEqual(check({**FILES, 'src/e.c': f'{directive}\n'}),
                                 [f"src/e.c:1: #{directive[1:].split()[0]} of {directive.split()[1]}: only a build's "
                                  'flags define it'])


if __name__ == '__main__':
    unittest.main()
