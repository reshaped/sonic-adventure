"""Names. The things and the bins config/parts.txt's head defines are tools/parts.py's THINGS and BINS, each once and
no other."""

import re
import unittest
from pathlib import Path

import parts
from support import cwd
from tables import lines

ROOT = Path(__file__).resolve().parents[1]
# the lists in config/parts.txt's head, by the word that opens each, and the names each defines. tools/parts.py's
# tuples are the one place that sets a thing's or a bin's name, since the code spells one wherever a rule hangs on
# it: rows() refuses any other and has a both say why, and test_zam.py and engine/tests/boundary.py ask for a bin or a
# thing by its name. The head says what each is, in comment lines, which no tool reads, so the head is what is held
# to the tuples
LISTS = {'THING': parts.THINGS, 'BIN': parts.BINS}
# a list's opening line, a line that defines a name, three spaces in, and one that runs a definition on, further in
OPENS = re.compile(r'# (\w+) is one of:')
DEFINES = re.compile(r'#   (\S+)')
RUNS_ON = re.compile(r'#    +\S')


def defined(table):
    """The names each list in table defines, a table's lines as lines() gives them, as {the word that opens the
    list: [(the line's number, the name)]}, and the line that ends each list, as {the word: the line's number}. A
    list is the lines after its opening line that each define a name or run a definition on, and any other line
    ends it. A list the table ends in has no such line."""
    found, ends, word = {}, {}, None
    for number, line in table:
        opened, named = OPENS.fullmatch(line), DEFINES.match(line)
        if word and named:
            found[word].append((number, named[1]))
        elif opened or not RUNS_ON.match(line):
            if word:
                ends[word] = number
            word = opened[1] if opened else None
            if opened:
                found.setdefault(word, [])
    return found, ends


def drift(found, ends):
    """What found and ends, defined()'s of config/parts.txt, leave wrong of LISTS, as lines: a name a list defines
    that its tuple lacks, a name it defines a second time, and a name of the tuple's that it doesn't define, with
    the line that ends the list, where one does."""
    problems = []
    for word, names in LISTS.items():
        kind, first = word.lower(), {}
        for number, given in found.get(word, ()):
            if given not in names:
                problems.append(f"{parts.PARTS}:{number}: {given}: the head defines a {kind} that tools/parts.py's "
                                f'{word}S lacks: take it out of the head, or add it to {word}S')
            elif given in first:
                problems.append(f'{parts.PARTS}:{number}: {given}: the head defines the {kind} a second time, after '
                                f'{parts.PARTS}:{first[given]}: say what it is once')
            first.setdefault(given, number)
        ended = (f', above {parts.PARTS}:{ends[word]}, which ends the list: it neither defines a name three spaces '
                 'in nor runs a definition on further in') if word in ends else ''
        problems += [f"{parts.PARTS}: {missing}: the head defines no such {kind}, and tools/parts.py's {word}S has "
                     f'it: say what it is under "# {word} is one of:"{ended}' for missing in names
                     if missing not in first]
    return problems


# what a head that defines each of the tuples' names once gives defined(): the things from line 18 and the bins from
# line 41, and the line after each list, which ends it
WHOLE = {'THING': list(enumerate(parts.THINGS, 18)), 'BIN': list(enumerate(parts.BINS, 41))}
ENDS = {'THING': 18 + len(parts.THINGS), 'BIN': 41 + len(parts.BINS)}
AT = 'config/parts.txt'
# what drift() says of the line that ends a list short of a name, after the line's place
ENDED = 'which ends the list: it neither defines a name three spaces in nor runs a definition on further in'


class Things(unittest.TestCase):
    maxDiff = None

    def test_reads_the_names_each_list_of_a_head_defines_and_the_line_that_ends_each(self):
        # a line three spaces in defines a name in a list alone, and a blank comment or one a space in ends one
        table = ['# What each source is part of.',
                 '#   three spaces in, and in no list',
                 '# THING is one of:',
                 "#   game       SEGA's game, and the port's code in it: its fixes, and",
                 '#              16:9',
                 "#   play-math  the SH-4's float operations",
                 '#',
                 '#   sdk        past the end of the list',
                 '# BIN is one of:',
                 '#   play     a game step reads what the file computes',
                 '# path  thing  bin',
                 '#   none     past the end of the list',
                 'src/  game  play']
        self.assertEqual(defined(enumerate(table, 1)),
                         ({'THING': [(4, 'game'), (6, 'play-math')], 'BIN': [(10, 'play')]}, {'THING': 7, 'BIN': 11}))
        self.assertEqual(defined(enumerate(table[:2], 1)), ({}, {}))
        # the next list's opening line ends a list too, and no line ends a list the table ends in
        self.assertEqual(defined(enumerate([*table[2:4], *table[8:10]], 1)),
                         ({'THING': [(2, 'game')], 'BIN': [(4, 'play')]}, {'THING': 3}))

    def test_finds_nothing_wrong_where_each_list_defines_its_tuple_s_names_once(self):
        self.assertEqual(drift(WHOLE, ENDS), [])
        # in any order, and in lists that no line ends
        self.assertEqual(drift({word: names[::-1] for word, names in WHOLE.items()}, {}), [])

    def test_names_a_name_the_tuple_lacks_one_defined_twice_and_one_not_defined(self):
        thing, held = parts.THINGS[0], parts.BINS[-1]
        found = {'THING': [*WHOLE['THING'], (27, 'engine'), (28, thing), (29, 'Game')], 'BIN': WHOLE['BIN'][:-1]}
        lacks = "the head defines a thing that tools/parts.py's THINGS lacks: take it out of the head, or add it to " \
                'THINGS'
        self.assertEqual(drift(found, {'THING': 30, 'BIN': 44}), [
            f'{AT}:27: engine: {lacks}',
            f'{AT}:28: {thing}: the head defines the thing a second time, after {AT}:18: say what it is once',
            f'{AT}:29: Game: {lacks}',
            f"{AT}: {held}: the head defines no such bin, and tools/parts.py's BINS has it: say what it is under "
            f'"# BIN is one of:", above {AT}:44, {ENDED}'])
        # a bin among the things is no thing
        found = {**WHOLE, 'THING': [*WHOLE['THING'], (27, held)]}
        self.assertEqual(drift(found, {**ENDS, 'THING': 28}), [f'{AT}:27: {held}: {lacks}'])

    def test_names_each_name_of_a_list_the_head_lacks(self):
        self.assertEqual(drift({'THING': WHOLE['THING']}, {'THING': ENDS['THING']}),
                         [f"{AT}: {held}: the head defines no such bin, and tools/parts.py's BINS has it: say what it "
                          'is under "# BIN is one of:"' for held in parts.BINS])

    def test_names_the_line_that_ends_a_list_short_of_the_names_below_it(self):
        # a blank comment in a list, and a name two spaces in: each ends the list, and a name below it is none of
        # the list's
        first, *rest = parts.THINGS
        for stray in ('#', f'#  {rest[0]}  two spaces in'):
            table = ['# THING is one of:', f'#   {first}  what it is', stray,
                     *(f'#   {thing}  what it is' for thing in rest),
                     '# BIN is one of:', *(f'#   {held}  what it is' for held in parts.BINS)]
            with self.subTest(stray=stray):
                self.assertEqual(drift(*defined(enumerate(table, 1))),
                                 [f"{AT}: {thing}: the head defines no such thing, and tools/parts.py's THINGS has "
                                  f'it: say what it is under "# THING is one of:", above {AT}:3, {ENDED}'
                                  for thing in rest])

    def test_the_table_s_head_defines_each_thing_and_each_bin_of_the_tool_s_once_and_no_other(self):
        with cwd(ROOT):
            self.assertEqual(drift(*defined(lines(parts.PARTS))), [])


if __name__ == '__main__':
    unittest.main()
