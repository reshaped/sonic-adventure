"""Check the guards in C: each tests NONMATCHING or DREAMCAST, the two
switches a build sets, by #ifdef alone.

make dc, the build of the non-matching code, defines NONMATCHING, and both
Dreamcast builds, SHC's and make dc, define DREAMCAST
(PORTS.md#switches). A run fails, naming each problem, on:

- a test of NONMATCHING or DREAMCAST other than an #ifdef: an #ifndef, an
  #elifdef or #elifndef of one, or an #if or #elif that names one;
- an #elif, #elifdef or #elifndef in an #ifdef of one, whose one other
  group is its #else;
- a conditional that names MATCHING, PORT, GCC or __clang__;
- a #define or #undef of NONMATCHING or DREAMCAST, which only a build's
  flags define.
"""

import argparse
import sys

from csource import parse
from tool import content, fail

# the switches, and the names a guard never tests: the matching side is
# NONMATCHING's #else, and a guard names what a build is for, never its
# compiler
SWITCHES = ('NONMATCHING', 'DREAMCAST')
REFUSED = ('MATCHING', 'PORT', 'GCC', '__clang__')
OPENING = ('if', 'ifdef', 'ifndef')
LATER = ('elif', 'elifdef', 'elifndef')
# the rule a problem with a conditional's test says after it names the test
RULE = 'a guard tests NONMATCHING or DREAMCAST'


def named(directive):
    """The names among a directive's words."""
    return {word.text for word in directive.words if word.kind == 'name'}


def tested(directive):
    """The name an #ifdef tests, or None."""
    return directive.words[0].text if directive.name == 'ifdef' and directive.words else None


def check(path, directives, problems):
    """Put on problems each of the module docstring's problems in the file at path, whose directives are
    directives."""
    # for each conditional open, the switch an #ifdef that opens it tests, else None
    opened = []
    for each in directives:
        where, names = f'{path}:{each.line}', named(each)
        if each.name in (*OPENING, *LATER):
            if names & set(REFUSED):
                problems.append(f'{where}: #{each.name} of {", ".join(sorted(names & set(REFUSED)))}: {RULE}')
            if names & set(SWITCHES) and each.name != 'ifdef':
                problems.append(f'{where}: #{each.name} of {", ".join(sorted(names & set(SWITCHES)))}: {RULE} '
                                'by #ifdef alone')
        if each.name in OPENING:
            opened.append(tested(each) if tested(each) in SWITCHES else None)
        elif each.name in LATER and opened and opened[-1]:
            problems.append(f'{where}: #{each.name} in an #ifdef of {opened[-1]}, whose one other group is #else')
        elif each.name == 'endif' and opened:
            opened.pop()
        if each.name in ('define', 'undef') and each.words and each.words[0].text in SWITCHES:
            problems.append(f"{where}: #{each.name} of {each.words[0].text}: only a build's flags define it")


def main(argv):
    parser = argparse.ArgumentParser(prog='guards', description=__doc__.split('\n\n')[0])
    parser.add_argument('paths', metavar='FILE', nargs='+', help='a C or C++ source or header, as make lint names it')
    problems = []
    for path in parser.parse_args(argv).paths:
        check(path, parse(content(path).decode(errors='replace')).directives, problems)
    if problems:
        fail(*problems)


if __name__ == '__main__':
    main(sys.argv[1:])
