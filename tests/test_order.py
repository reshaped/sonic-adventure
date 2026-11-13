import hashlib
import io
import re
import shutil
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from itertools import combinations, permutations
from pathlib import Path
from unittest import mock

import gcclink
import ld
import order
from test_ld import Fixture

# a heat of two routes, of 10 frames and of 4: two stretches of ova's step, one of main's start, and one in no image
HEAT = '''\
# a heat
frames  10  4
code  ova  ova.o  .text.step
0     8   30  8   # its entry
0x8   4   10  0
code  main  main.o  .text.start
12    2   0   4
fixed
0x8c000100  6  20  2
'''


def stays(*routes):
    """What Costs takes of the code that stays: for each route, each line's heat by line. Each of routes maps a set to
    the heats of the lines that stay on it, which lie a cache apart from line 0 up, under the lines the tests lay
    sections on."""
    return [{at + ld.ICACHE_LINES * place: heat for at, heats in route.items() for place, heat in enumerate(heats)}
            for route in routes]


def pairs(lines):
    """What lines, heats by line, cost each other: for each two on one set, the lesser heat."""
    return sum(min(lines[one], lines[other]) for one, other in combinations(lines, 2)
               if one % ld.ICACHE_LINES == other % ld.ICACHE_LINES)


def cost(sections, stays, lies):
    """What the module's docstring gives sections to cost where lies has them, each (index, address), counted a pair
    at a time: the pairs of every line once the sections lie, less the pairs of the code that stays alone."""
    lines = [dict(each) for each in stays]
    for index, at in lies:
        for offset, size, heats in sections[index].stretches:
            for line in range(order.line_of(at + offset), order.line_of(at + offset + size - 1) + 1):
                for route, heat in enumerate(heats):
                    lines[route][line] = max(lines[route].get(line, 0), heat)
    return sum(pairs(after) - pairs(before) for before, after in zip(stays, lines))


def follow(start, sections, laid):
    """Where each of sections lies, laid from start in the order laid, each after the last on its alignment: (index,
    address) for each."""
    lies, at = [], start
    for index in laid:
        at = -(-at // sections[index].align) * sections[index].align
        lies.append((index, at))
        at += sections[index].size
    return lies


def first(start, sections, stays):
    """The order search()'s first order lays sections in from start, by cost(): at each place the section left that
    costs least there, the first of those that cost the same."""
    left, found = list(range(len(sections))), []
    while left:
        found.append(min(left, key=lambda index: cost(sections, stays, follow(start, sections, [*found, index]))))
        left.remove(found[-1])
    return found


def drawn(seed):
    """A function that gives a whole number under its argument, another at each call, in an order seed decides. Its
    arithmetic is whole numbers', so a layout drawn by it is the same on every host and under every Python."""
    state = seed

    def under(limit):
        nonlocal state
        state = (state * 6364136223846793005 + 1442695040888963407) % (1 << 64)
        return (state >> 33) % limit
    return under


class Heat(unittest.TestCase):
    def read(self, text):
        """The heat whose text is text, and its path."""
        root = tempfile.TemporaryDirectory()
        self.addCleanup(root.cleanup)
        path = Path(root.name, 'heat.txt')
        path.write_text(text)
        return order.heat(path)[0], path

    def test_reads_each_route_s_frames_each_section_s_stretches_and_the_fixed_ones(self):
        self.assertEqual(self.read(HEAT)[0], order.Heat(
            (10, 4), {('ova', 'ova.o', '.text.step'): [(0, 8, (30, 8)), (8, 4, (10, 0))],
                      ('main', 'main.o', '.text.start'): [(12, 2, (0, 4))]}, [(0x8c000100, 6, (20, 2))]))

    def test_reads_an_archive_member_s_path_as_the_map_spells_it_and_a_section_s_rows_wherever_they_stand(self):
        read, _ = self.read('frames 1\ncode  main  lib/libovb.a(helper.o)  .text.ovb_helper\n0  2  1\n'
                            'code  main  main.o  P\ncode  main  lib/libovb.a(helper.o)  .text.ovb_helper\n4  2  3\n')
        self.assertEqual(read.code, {('main', 'lib/libovb.a(helper.o)', '.text.ovb_helper'): [(0, 2, (1,)), (4, 2, (3,))],
                                     ('main', 'main.o', 'P'): []})

    def test_refuses_a_row_naming_its_line(self):
        row = 'not a row: code IMAGE PATH INPUT, fixed, or after one of them OFFSET SIZE RUNS...'
        cases = {'no frames row first': ('code  ova  ova.o  .text.step\n', ':1: not a row: frames FRAMES...'),
                 'a frames row of no route': ('frames\n', ':1: not a row: frames FRAMES...'),
                 'a FRAMES that is no number': ('frames  ten\n', ':1: not a row: frames FRAMES...'),
                 'a FRAMES of 0': ('frames  10  0\n', ':1: a FRAMES of 0'),
                 'a stretch ahead of the first code or fixed row': ('frames  10\n0  2  30\n', f':2: {row}'),
                 'a code row whose INPUT is no section': ('frames  10\ncode  ova  ova.o  step\n', f':2: {row}'),
                 'a code row whose PATH is a pattern': ('frames  10\ncode  ova  o*.o  .text.step\n', f':2: {row}'),
                 'a code row without its INPUT': ('frames  10\ncode  ova  ova.o\n', f':2: {row}'),
                 'a fixed row with a field': ('frames  10\nfixed  0x8c000100\n', f':2: {row}'),
                 'a RUNS that is no number': ('frames  10\nfixed\n0x8c000100  2  3.5\n', f':3: {row}'),
                 'a stretch without its SIZE': ('frames  10\nfixed\n0x8c000100\n', f':3: {row}'),
                 'a RUNS too few': ('frames  10  4\n\nfixed\n0x8c000100  2  30\n',
                                    ':4: 1 RUNS, and the first row gives 2 FRAMES'),
                 'a RUNS too many': ('frames  10\ncode  ova  ova.o  .text.step\n0  2  30  4\n',
                                     ':3: 2 RUNS, and the first row gives 1 FRAMES'),
                 'a SIZE of 0': ('frames  10\nfixed\n0x8c000100  0  30\n', ':3: a SIZE of 0'),
                 'no row': ('# a heat\n', ': no row: frames FRAMES...')}
        for case, (text, problem) in cases.items():
            with self.subTest(case), self.assertRaisesRegex(SystemExit, f'heat.txt{re.escape(problem)}$'):
                self.read(text)

    def test_gives_the_hash_of_the_bytes_it_read_the_heat_from(self):
        # the file is read once: a heat written again as soon as it is read gives the first one's rows and hash
        _, path = self.read(HEAT)

        def content(read):
            data = read.read_bytes()
            read.write_text('frames  1\n')
            return data

        with mock.patch.object(order, 'content', content):
            read, digest = order.heat(path)
        self.assertEqual((read.frames, digest), ((10, 4), hashlib.sha256(HEAT.encode()).hexdigest()))
        self.assertEqual(path.read_text(), 'frames  1\n')

    def test_fails_on_a_heat_it_can_t_read(self):
        with self.assertRaisesRegex(SystemExit, 'absent/heat.txt: No such file or directory$'):
            order.heat(Path('absent/heat.txt'))
        # a line that isn't UTF-8 fails as a table's does, naming it
        _, path = self.read('frames  1\n')
        path.write_bytes(b'frames  1\nfixed  # \xff\n')
        with self.assertRaisesRegex(SystemExit, r"heat.txt:2: a byte that isn't UTF-8$"):
            order.heat(path)

    def test_weighs_each_route_s_runs_over_the_least_frames_each_route_s_divide(self):
        self.assertEqual(order.weights((10, 4)), ((2, 5), 20))
        self.assertEqual(order.weights((7,)), ((1,), 7))


class Search(unittest.TestCase):
    def test_lays_a_section_where_its_hot_line_takes_no_set_of_a_hot_line_that_stays(self):
        # a line of heat 9 stays on set 1. The first section's line is as hot and the second's has heat 1, so the
        # second takes set 1. The order that stood, the first there, cost 9
        sections = [order.Section(32, 4, [(0, 2, (9,))]), order.Section(32, 4, [(0, 2, (1,))])]
        self.assertEqual(order.search(0x8c010020, sections, stays({1: [9]}), [0, 1]), ([1, 0], 9, 1))

    def test_a_pair_costs_the_lesser_heat_and_a_line_has_the_most_of_the_stretches_that_touch_it(self):
        # stretches of heats 3 and 5 on the section's first line, against lines of 4 and 7 on its set: 4 and 5. The
        # second runs 2 bytes into the next line, whose set holds a line of 6: 5 more
        sections = [order.Section(64, 4, [(0, 4, (3,)), (28, 6, (5,))])]
        self.assertEqual(order.search(0x8c010000, sections, stays({0: [7, 4], 1: [6]}), [0]), ([0], 14, 14))

    def test_counts_the_warm_sections_lines_against_each_other_and_adds_up_the_routes(self):
        # each section fills the cache, so their first lines share a set in either order: 2 on the first route and 3
        # on the second, whose line that stays lies apart on set 5
        sections = [order.Section(8192, 4, [(0, 2, (2, 3))]), order.Section(8192, 4, [(0, 2, (6, 4))])]
        self.assertEqual(order.search(0x8c010000, sections, stays({}, {5: [9]}), [1, 0]), ([1, 0], 5, 5))

    def test_lays_each_section_on_its_alignment_as_ld_does(self):
        # after the first's 2 bytes the second, on 32, starts on the next line, whose set a hot line takes. Laid
        # first it starts on set 0
        sections = [order.Section(2, 2, []), order.Section(4, 32, [(0, 2, (5,))])]
        self.assertEqual(order.search(0x8c010000, sections, stays({1: [5]}), [0, 1]), ([1, 0], 5, 0))

    def test_takes_the_first_of_the_sections_that_cost_the_same_at_a_place(self):
        # the hot section stood on the set of a line as hot. The other two cost nothing there, or on the next set
        sections = [order.Section(32, 4, []), order.Section(32, 4, []), order.Section(32, 4, [(0, 2, (9,))])]
        self.assertEqual(order.search(0x8c010000, sections, stays({0: [9]}), [2, 1, 0]), ([0, 1, 2], 9, 0))

    def test_moves_a_section_the_first_step_laid_where_another_place_costs_less(self):
        # the first step takes a at set 0, since it costs nothing there, then b on set 1 against the 5 that stays,
        # and c on set 2: 5. A pass then moves b past c, off set 1, onto set 2: 0
        a, b, c = order.Section(32, 4, []), order.Section(32, 4, [(0, 2, (9,))]), order.Section(32, 4, [])
        self.assertEqual(order.search(0x8c010000, [a, b, c], stays({1: [5], 0: [9]}), [1, 0, 2]), ([0, 2, 1], 9, 0))

    def test_a_line_s_set_is_its_address_s_in_either_view_of_ram(self):
        self.assertEqual((order.line_of(0x8c010020), order.line_of(0x0c010020)), (0x801, 0x801))

    def test_keeps_the_order_that_stood_unless_the_one_it_found_costs_less(self):
        # the rows stood in the order that costs least, 2. The first step and the moves after it end on an order that
        # costs 10, from which no one move costs less
        sections = [order.Section(64, 4, [(0, 2, (9,))]), order.Section(32, 4, [(0, 2, (1,))]),
                    order.Section(32, 4, [(0, 2, (9,))])]
        kept = stays({0: [9], 1: [1], 3: [9], 4: [3]})
        self.assertEqual(order.search(0x8c010000, sections, kept, [1, 2, 0]), ([1, 2, 0], 2, 2))
        # an order that stood at 18 gives way to the one found
        self.assertEqual(order.search(0x8c010000, sections, kept, [0, 1, 2]), ([1, 2, 0], 18, 2))

    def test_starts_from_the_order_that_stood_and_from_shuffled_orders_too_and_takes_the_least_they_end_on(self):
        # six sections of a line each, with a hot line apiece, against lines that stay on five sets. The first order
        # and its moves end on 14 a frame. From the order that stood, at 18, the moves end on 13, and from one of
        # eight shuffles on 12, and the same on each run
        sections = [order.Section(32, 4, [(0, 2, (heat,))]) for heat in (4, 7, 1, 9, 4, 8)]
        kept = stays({3: [1], 2: [9], 1: [2, 3], 5: [2, 5], 7: [6, 9]})
        stood = [0, 2, 5, 1, 4, 3]
        self.assertEqual(order.search(0x8c010000, sections, kept, stood, 0), ([5, 2, 0, 1, 3, 4], 18, 13))
        self.assertEqual(order.search(0x8c010000, sections, kept, stood, 8), ([1, 5, 4, 0, 3, 2], 18, 12))
        self.assertEqual(order.search(0x8c010000, sections, kept, stood, 8), ([1, 5, 4, 0, 3, 2], 18, 12))
        # an order that stood where the first order's moves end has them end no lower
        self.assertEqual(order.search(0x8c010000, sections, kept, [0, 2, 4, 1, 3, 5], 0), ([0, 2, 4, 1, 3, 5], 14, 14))

    def test_shuffles_by_whole_numbers_so_an_order_is_the_same_on_every_host(self):
        self.assertEqual([order.shuffled(6, seed) for seed in (1, 2)], [[1, 5, 4, 0, 3, 2], order.shuffled(6, 2)])
        self.assertEqual([sorted(order.shuffled(count, 7)) for count in (0, 1, 9)], [[], [0], list(range(9))])
        self.assertNotEqual(order.shuffled(6, 1), order.shuffled(6, 2))


class Settle(unittest.TestCase):
    START = 0x8c010040

    def settle(self, sections, places, *routes):
        """What settle() gives sections from START, with places their offsets from START, or None, and routes the
        code that stays, as stays() takes it: each section's offset from START, and the indexes that took a place."""
        kept = stays(*(routes or ({},)))
        lies, anew = order.settle(self.START, sections, order.Costs(sections, kept),
                                  [None if place is None else self.START + place for place in places])
        return [at - self.START for at in lies], anew

    def test_keeps_each_section_where_it_fits_and_moves_only_one_that_runs_past_the_next(self):
        # three sections packed, as a link without a record lays them: each fits, the last with nothing after it
        sections = [order.Section(36, 4, []), order.Section(14, 2, []), order.Section(8, 4, [])]
        self.assertEqual(self.settle(sections, [0, 0x24, 0x34]), ([0, 0x24, 0x34], []))
        # the first at 40 bytes runs 4 past the second, and takes the first line a spare line past the last's. The
        # other two stay, and so does a section that shrank
        grown = [order.Section(40, 4, []), order.Section(10, 2, []), order.Section(8, 4, [])]
        self.assertEqual(self.settle(grown, [0, 0x24, 0x34]), ([0x60, 0x24, 0x34], [0]))
        # the second at 20 bytes runs past the third, and moves. The first, at 40, fits under the third once the
        # second has gone
        self.assertEqual(self.settle([grown[0], order.Section(20, 2, []), grown[2]], [0, 0x24, 0x34]),
                         ([0, 0x60, 0x34], [1]))
        # two that outgrew the third move, in the sections' order, each a spare line past the one before
        both = [order.Section(60, 4, []), order.Section(20, 2, []), order.Section(8, 4, [])]
        self.assertEqual(self.settle(both, [0, 0x24, 0x34]), ([0x60, 0xc0, 0x34], [0, 1]))

    def test_a_section_with_no_place_one_off_its_alignment_or_one_under_the_start_takes_a_place(self):
        sections = [order.Section(8, 4, []), order.Section(8, 4, []), order.Section(8, 4, []), order.Section(8, 64, [])]
        # the fourth keeps its place, and the others take the free lines under it, each with its spare line
        self.assertEqual(self.settle(sections, [None, 0x22, -4, 0x100]), ([0, 0x40, 0x80, 0x100], [0, 1, 2]))
        # a section on 64 takes a place on its alignment
        self.assertEqual(self.settle(sections, [0, 0x40, 0x80, None]), ([0, 0x40, 0x80, 0xc0], [3]))

    def test_a_section_that_ends_past_its_window_s_end_takes_a_place_under_it_or_none_where_none_is_free(self):
        # the second section's 40 bytes end 8 past a window of six lines. The line past the first's spare line is
        # free, and the section ends under the window's end there. Under an end of four lines it ends past it
        sections = [order.Section(36, 4, []), order.Section(40, 4, [])]
        costs, places = order.Costs(sections, stays({})), [self.START, self.START + 0xa0]
        self.assertEqual(order.settle(self.START, sections, costs, places), (places, []))
        self.assertEqual(order.settle(self.START, sections, costs, places, self.START + 0xc0),
                         ([self.START, self.START + 0x60], [1]))
        self.assertEqual(order.settle(self.START, sections, costs, places, self.START + 0x80),
                         ([self.START, None], [1]))

    def test_takes_the_free_place_that_costs_least_among_the_rows_or_past_the_last_and_the_lowest_of_equals(self):
        # sections stand at the start and five lines on, and the lines between are free from the second, a spare
        # line past the first's, to the third, which leaves a spare line under the next section. One more past the
        # last is free. The new section's line is hot
        stand = [order.Section(32, 4, [(0, 2, (9,))]), order.Section(32, 4, [(0, 2, (9,))])]
        new = order.Section(32, 4, [(0, 2, (5,))])
        cases = {'the lowest where none costs': ({}, 0x40), 'past a line that stays on its set': ({4: [5]}, 0x60),
                 'past the last where each place among the rows costs': ({4: [5], 5: [1]}, 0xe0),
                 'the least where each costs': ({4: [5], 5: [2], 9: [3]}, 0x60)}
        for case, (kept, place) in cases.items():
            with self.subTest(case):
                self.assertEqual(self.settle([*stand, new], [0, 0xa0, None], kept), ([0, 0xa0, place], [2]))
        # the sections that have places cost a new one too. The second fills the cache from the third line, so the
        # place past it lies a cache on from the first's hot line, and costs 5 against it: more than the 3 of the
        # free line between them, and less than its 6 against two lines that stay there
        long = [stand[0], order.Section(0x1f60, 4, []), new]
        self.assertEqual(self.settle(long, [0, 0x80, None], {4: [3]}), ([0, 0x80, 0x40], [2]))
        self.assertEqual(self.settle(long, [0, 0x80, None], {4: [3, 3]}), ([0, 0x80, 0x2000], [2]))

    def test_costs_a_free_place_against_the_lines_as_they_lie_where_code_that_meets_within_a_line_is_one_line(self):
        # two sections of 2 bytes and heat 4 lie on the start's line, on set 2, and one with no heat two caches on.
        # A line of heat 6 stays on every other set, so the new section's line, of heat 5, costs 5 on any of them.
        # A cache on from the start it costs 4, against the one line the two share, so it takes that place
        two = [order.Section(2, 2, [(0, 2, (4,))]), order.Section(2, 2, [(0, 2, (4,))]), order.Section(32, 4, []),
               order.Section(32, 4, [(0, 2, (5,))])]
        others = {at: [6] for at in range(ld.ICACHE_LINES) if at != 2}
        self.assertEqual(self.settle(two, [0, 2, 0x4000, None], others), ([0, 2, 0x4000, 0x2000], [3]))
        # a section and the code that stays on its line are one line too: of heat 4, where the section has 2 of it
        one = [order.Section(2, 2, [(0, 2, (2,))]), *two[2:]]
        kept = [{**stays(others)[0], order.line_of(self.START): 4}]
        self.assertEqual(order.settle(self.START, one, order.Costs(one, kept),
                                      [self.START + 16, self.START + 0x4000, None]),
                         ([self.START + 16, self.START + 0x4000, self.START + 0x2000], [2]))
        # two that lie a cache apart are two lines on the set, so the place a cache past them costs 8, and the new
        # section takes the lowest of the others
        self.assertEqual(self.settle(two, [0, 0x2000, 0x6000, None], others), ([0, 0x2000, 0x6000, 0x40], [3]))


class Cost(unittest.TestCase):
    def counted(self, sections, held, lies):
        """What sections cost where lies has them, each (index, address), against held, the code that stays, once a
        Costs of them is held to cost(): of() for them all, in the order of lies and in rising and falling order of
        their addresses, and for each in turn beside(), which is what cost() has it add, and of() with it less of()
        without."""
        costs, whole = order.Costs(sections, held), cost(sections, held, lies)
        rising = sorted(lies, key=lambda lie: lie[1])
        self.assertEqual([costs.of(each) for each in (lies, rising, rising[::-1])], [whole] * 3)
        taken, done = costs.alone(), []
        for index, at in lies:
            adds = cost(sections, held, [*done, (index, at)]) - cost(sections, held, done)
            self.assertEqual((costs.beside(taken, index, at), costs.of([*done, (index, at)]) - costs.of(done)),
                             (adds, adds))
            costs.take(taken, index, at)
            done.append((index, at))
        return whole

    def test_sections_that_share_a_line_are_one_line_of_the_most_of_their_heats(self):
        # two sections of 2 bytes and heat 9 lie on one line, so there is no pair, and the second adds nothing
        hot, mild = order.Section(2, 2, [(0, 2, (9,))]), order.Section(2, 2, [(0, 2, (3,))])
        self.assertEqual(self.counted([hot, hot], [{}], [(0, 0x8c010000), (1, 0x8c010002)]), 0)
        self.assertEqual(order.search(0x8c010000, [hot, hot], [{}], [0, 1]), ([0, 1], 0, 0))
        # a third's last line lies a cache on from that line and has heat 4. It pairs with the line once, whose
        # heat is the hotter section's 9, whichever of the two lies first
        three = [mild, hot, order.Section(8192, 32, [(0x1fe0, 2, (4,))])]
        for stood in ([0, 1, 2], [1, 0, 2]):
            self.assertEqual(self.counted(three, [{}], follow(0x8c010000, three, stood)), 4)
            self.assertEqual(order.search(0x8c010000, three, [{}], stood)[1], 4)

    def test_a_line_shared_with_the_code_that_stays_costs_what_the_section_adds_to_its_heat(self):
        # the section starts 16 bytes into a line of heat 5 that stays, and a line of heat 9 stays on its set. With
        # heat 3 the section adds nothing to the line. With heat 7 the line's pair with the 9 costs 7, not 5, and
        # with heat 12 it costs 9
        held = [{0x800: 5, 0x900: 9}]
        for heat, costs in ((3, 0), (7, 2), (12, 4)):
            with self.subTest(heat=heat):
                section = order.Section(8, 4, [(0, 2, (heat,))])
                self.assertEqual(self.counted([section], held, [(0, 0x8c010010)]), costs)
                self.assertEqual(order.search(0x8c010010, [section], held, [0]), ([0], costs, costs))

    def test_costs_each_order_as_the_pairs_counted_one_at_a_time_do(self):
        tail = order.Section(34, 2, [(0, 2, (2, 0)), (30, 4, (7, 3))])
        head = order.Section(6, 2, [(0, 4, (5, 9))])
        whole = order.Section(8192, 4, [(4, 2, (6, 6)), (8188, 4, (1, 8))])
        cases = {
            'one line': (0x8c010000, [head, head, head], [{}, {}]),
            'the next line, and a line a cache on': (0x8c010000, [tail, head, whole], [{}, {}]),
            'one set and other lines': (0x8c010000, [whole, whole, head], stays({0: [4, 7], 1: [5]}, {0: [8]})),
            'a gap for an alignment': (0x8c010002, [head, order.Section(40, 32, [(0, 40, (3, 4))]), tail],
                                       stays({1: [2]}, {2: [6]})),
            'the last set and the first': (0x8c011fe0 + 30, [head, tail, head], stays({255: [6]}, {0: [4]})),
            'a line of the code that stays, at either end': (
                0x8c010010, [head, tail, order.Section(10, 2, [(8, 2, (4, 4))])],
                [{0x800: 6, 0x802: 3, 0x900: 8, 0xa02: 5}, {0x800: 2, 0x802: 9, 0x900: 8, 0xa00: 1}]),
            'two lines of the code that stays on one set': (
                0x8c010010, [whole, tail], [{0x800: 3, 0x900: 2, 0xa00: 9}, {0x800: 7, 0x900: 1}]),
            'a line of the code that stays, within a section': (
                0x8c010000, [order.Section(96, 4, [(40, 2, (4, 8))]), head],
                [{0x801: 6, 0x901: 5}, {0x801: 2, 0x901: 9}]),
            # each section adds nothing to the line that stays, so the first step takes the first
            'a cooler section on a hotter line that stays': (
                0x8c010010, [order.Section(8, 4, [(4, 2, (2, 0))]), order.Section(8, 4, [(0, 2, (1, 0))])],
                [{0x800: 9, 0x900: 9}, {}]),
            # the first step lays the first two on one line, then the last, which costs 3 against the line that
            # stays. The third costs 5 there, against the first's 9
            'a cooler section on a hotter section\'s line': (
                0x8c010000, [order.Section(2, 2, [(0, 2, (9, 0))]), order.Section(2, 2, [(0, 2, (1, 0))]),
                             order.Section(8192, 4, [(8188, 4, (5, 0))]), order.Section(64, 4, [(60, 2, (9, 0))])],
                [{0xa02: 3}, {}]),
        }
        for case, (start, sections, held) in cases.items():
            with self.subTest(case):
                for stood in permutations(range(len(sections))):
                    before = self.counted(sections, held, follow(start, sections, stood))
                    found, *costs = order.search(start, sections, held, list(stood))
                    self.assertEqual(costs, [before, cost(sections, held, follow(start, sections, found))])
                # the first order search() starts from takes each section at its place as cost() has it, which the
                # sections it takes first show
                with mock.patch.object(order.Costs, 'take', autospec=True, side_effect=order.Costs.take) as took:
                    order.search(start, sections, held, list(range(len(sections))), 0)
                self.assertEqual([taken.args[2] for taken in took.call_args_list[:len(sections)]],
                                 first(start, sections, held))

    def layout(self, seed):
        """A small layout that seed decides, as (sections, stays, lies), over two routes: two to five sections of
        up to three stretches each, at addresses that rise by each one's size and by a gap of no byte, of some, of
        a line or of a cache, listed in an order of their own; and code that stays on the lines of the sections'
        stretches, on lines a cache from those, and on lines near the first section."""
        under = drawn(seed)

        def one(*among):
            return among[under(len(among))]

        sections = []
        for _ in range(2 + under(4)):
            size, stretches = one(2, 6, 6, 30, 34, 64, 100, 8192), []
            for _ in range(one(0, 1, 1, 2, 3)):
                offset = one(0, 0, under(size), size - 1)
                stretches.append((offset, min(one(2, 4, 40), size - offset), (under(10), under(10))))
            sections.append(order.Section(size, one(1, 2, 2, 4, 4, 32), stretches))
        lies, at = [], 0x8c010000 + one(0, 2, 30, 0x1fe0)
        for index, section in enumerate(sections):
            at = -(-(at + one(0, 0, 0, 0, 2, 30, 32, 8192)) // section.align) * section.align
            lies.append((index, at))
            at += section.size
        lines = [order.line_of(at + offset) + one(0, 0, ld.ICACHE_LINES, -ld.ICACHE_LINES)
                 for index, at in lies for offset, _, _ in sections[index].stretches]
        lines += [order.line_of(lies[0][1]) + under(8) for _ in range(4)]
        held = [{one(*lines): 1 + under(12) for _ in range(under(6))} for _ in range(2)]
        return sections, held, [lies[index] for index in order.shuffled(len(lies), seed)]

    def test_costs_sections_at_any_addresses_as_the_pairs_counted_one_at_a_time_do(self):
        # sixty-four layouts, each drawn by its number. In 16 of them two sections have a hot line in common, in 43
        # a section has one that code that stays lies on, and 40 cost something
        met = stayed = costly = 0
        for seed in range(1, 65):
            sections, held, lies = self.layout(seed)
            with self.subTest(seed=seed):
                costly += self.counted(sections, held, lies) > 0
            hot = [{(route, line) for offset, size, heats in sections[index].stretches
                    for line in range(order.line_of(at + offset), order.line_of(at + offset + size - 1) + 1)
                    for route, heat in enumerate(heats) if heat} for index, at in lies]
            met += any(one & other for one, other in combinations(hot, 2))
            stayed += any(line in held[route] for each in hot for route, line in each)
        self.assertEqual((met, stayed, costly), (16, 43, 40))


class Staying(unittest.TestCase):
    def test_gives_the_lines_of_the_image_s_kernels_and_of_the_fixed_code_by_line_as_the_link_places_them(self):
        # ova's step, a kernel row, lies on line 0x48100, set 0, in the other view, and its probe on the next line. Its
        # unused, which no row places, and main's start stay nowhere, wherever a link has them. A stretch past its
        # section's end counts for nothing, and a line takes the most of the stretches that touch it on each route
        image = gcclink.Image('ova', *[None] * 19)._replace(kernel=[gcclink.Piece('draw', 'ova.o', '.text.step'),
                                                                    gcclink.Piece('draw', 'ova.o', '.text.ova_probe')])
        passed = gcclink.Linked(None, [gcclink.Placed('.text', 'ova.o', '.text.step', 0x0c902000, 0x20),
                                       gcclink.Placed('.text', 'ova.o', '.text.ova_probe', 0x0c902020, 8),
                                       gcclink.Placed('.text', 'ova.o', '.text.ova_unused', 0x0c902040, 0x20),
                                       gcclink.Placed('.text', 'ova.o', '.text.step', 0x0c902060, 0x20)])
        heat = order.Heat((1, 1), {('ova', 'ova.o', '.text.step'): [(0, 2, (5, 0)), (4, 2, (3, 7)), (0x1e, 4, (9, 9))],
                                   ('ova', 'ova.o', '.text.ova_probe'): [(0, 2, (6, 6)), (6, 2, (2, 8))],
                                   ('ova', 'ova.o', '.text.ova_unused'): [(0, 2, (9, 9))],
                                   ('main', 'main.o', '.text.start'): [(0, 2, (9, 9))]}, [(0x8c000040, 2, (1, 0))])
        self.assertEqual(order.staying(image, heat, passed),
                         ([{0x48100: 5, 0x48101: 6, 2: 1}, {0x48100: 7, 0x48101: 8}], 5, 6))


# ova's rows for the tests that search: record's 8 bytes are its head, so its .text starts a line past its origin,
# small is the draw kernel, in a window of two lines, and large's 40 bytes and pad's 32 are warm rows, large first
KERNEL = '''\
# ova's kernels
draw   small.s  small
window draw  2

# its warm rows
warm   large.s  large     # the large one
# between them
warm   pad.s    pad
'''
ROWS = '''\
input     ova  record.o
input     ova  small.o
input     ova  large.o
input     ova  pad.o
head      ova  DHEAD  record.o  .data
kernel    ova  draw  small.o  .text.small
window    ova  draw  2
warm      ova  large.o  .text.large
warm      ova  pad.o  .text.pad
'''


class Reorder(Fixture):
    """order.py over the fixture's images, whose spec gives ova ROWS. Each test writes ova's tables under config/ in
    the fixture's directory, where its objects lie."""

    def setUp(self):
        super().setUp()
        # the fixture compiles each source to an object beside it, where make dc compiles one under build/dc/
        beside = mock.patch.object(order, 'gcc_object', lambda source, image: Path(source).with_suffix('.o'))
        beside.start()
        self.addCleanup(beside.stop)
        table = self.root / 'config/ova'
        table.mkdir(parents=True, exist_ok=True)
        self.addCleanup(lambda: [each.unlink() for each in table.glob('*')])
        (table / 'kernel.txt').write_text(KERNEL)
        self.spec = Path(f'{self.out}.txt')
        self.spec.write_text(Path('link.txt').read_text() + ROWS)
        self.addCleanup((self.root / self.spec).unlink)

    def run_order(self, heat, *flags):
        """What order.py prints for the test's spec, with heat as ova's heat, given flags."""
        Path('config/ova/heat.txt').write_text(heat)
        printed = io.StringIO()
        with redirect_stdout(printed):
            order.main([*flags, str(self.spec), str(self.out)])
        return printed.getvalue()

    def searched(self):
        """The fields of each row of ova's record after its header."""
        record = Path('config/ova/order.txt').read_text()
        self.assertEqual(record[:len(order.HEADER)], order.HEADER)
        return [line.split() for line in record[len(order.HEADER):].splitlines()]

    def carry(self, rows=None):
        """Write the test's spec as tools/spec.py would from ova's tables: its warm rows in the order of rows, the
        record's fields as searched() gives them, by default the record's own, and the record's rows."""
        rows = self.searched() if rows is None else rows
        warm = ''.join(f'warm      ova  {path}  {section}\n' for _, path, section, _, _ in rows[3:])
        kept = ''.join(line for line in ROWS.splitlines(keepends=True) if not line.startswith('warm'))
        record = f'heat  ova  config/ova/heat.txt  {rows[0][1]}\n' + ''.join(
            f'{" ".join([row[0], "ova", *row[1:]])}\n' for row in rows[1:])
        self.spec.write_text(Path('link.txt').read_text() + kept + warm + record)

    # large's first line is hot, and a fixed line stays on set 3, where the window's end, three lines past ova's
    # origin, puts the first warm row. A stretch past large's end and those of main's code, which no row of ova's
    # places, count for nothing
    HOT = ('frames  2\ncode  ova  large.o  .text.large\n0  4  8\n0x1000  2  8\ncode  main  main.o  .text.start\n'
           '0  2  6\ncode  main  main.o  .text.absent\n0  2  6\nfixed\n0x8c000060  2  6\n')
    GIVES = ("ova: 2 warm rows; the heat gives 5 stretches, 3 of them of its rows' sections or fixed, and the link "
             "holds 2 of those; the rows' pairs cost {} a frame where they lie, summed over the routes\n")
    # what follows the lines that say what a record held otherwise, and those of an image that had no record
    WAS = "ova: the rows' pairs cost {} a frame as its record had them and the kernels, by the heat as it is now\n"
    ANEW = ('ova: it had no record, so each of its 2 warm rows took a place anew, by no search: a change to measure, '
            'where a record was lost\n')
    # what says where the rows end, where the record had its rows end, and how far the end lies from there
    ENDS = ('ova: its warm rows end {} bytes into .text, and its record had them end {} bytes in, so their end lies '
            '{}, and the code after them follows it\n')

    def test_gives_each_warm_row_without_a_place_one_on_a_line_with_a_spare_line_and_a_link_then_lays_it_there(self):
        # with no record, large and then pad, in (PATH, INPUT) order, take the first line past the last row's spare
        # line: the window's end, and three lines on, past large's two lines and its spare one. Each row's line in
        # the table keeps its place and its comment
        self.assertEqual(self.run_order(self.HOT),
                         self.GIVES.format(3) + "ova: large.o's .text.large takes a place 0x40 bytes into .text, its "
                         "first\nova: pad.o's .text.pad takes a place 0xa0 bytes into .text, its first\n" + self.ANEW)
        self.assertEqual(Path('config/ova/kernel.txt').read_text(), KERNEL)
        rows = self.searched()
        self.assertEqual(rows, [['heat', hashlib.sha256(self.HOT.encode()).hexdigest()], ['text', '0x0c900020'],
                                ['searched', 'small.o', '.text.small', '0x0', '0x8'],
                                ['searched', 'large.o', '.text.large', '0x40', '0x28'],
                                ['searched', 'pad.o', '.text.pad', '0xa0', '0x20']])
        # the link without the record fails, and by it lays each row at its place
        with self.assertRaisesRegex(SystemExit, 'ova: its warm rows have no record of their places, '
                                                'config/ova/order.txt; make dc-order writes one, and gives every row '
                                                'a place anew, so restore a record that was lost$'):
            ld.coordinate(gcclink.read(self.spec), self.out)
        self.carry()
        _, layouts, _ = ld.coordinate(gcclink.read(self.spec), self.out)
        self.assertEqual({each.section: each.address - 0x0c900020 for each in layouts['ova'].placements
                          if each.section in ('.text.large', '.text.pad')}, {'.text.large': 0x40, '.text.pad': 0xa0})
        # a second run, by the tables the first wrote, moves nothing and changes neither
        tables = [Path(f'config/ova/{name}').read_text() for name in ('kernel.txt', 'order.txt')]
        self.assertEqual(self.run_order(self.HOT), self.GIVES.format(3))
        self.assertEqual([Path(f'config/ova/{name}').read_text() for name in ('kernel.txt', 'order.txt')], tables)

    def test_moves_a_row_that_outgrew_its_place_alone_and_writes_the_rows_in_their_places_order(self):
        # the record has large 0x40 bytes in and pad a line on, where large's 40 bytes run 8 past it, as where large
        # grew. The link fails naming large. order.py links without the record, moves large past pad's spare line,
        # and leaves pad's row and the kernel's as they were
        self.run_order(self.HOT)
        rows = self.searched()
        tight = [*rows[:4], ['searched', 'pad.o', '.text.pad', '0x60', '0x20']]
        self.carry(tight)
        with self.assertRaisesRegex(SystemExit, r"ova: large.o's \.text\.large takes 40 bytes from its place, 0x40 "
                                                r"bytes into \.text, 8 past the next warm row's place in "
                                                'config/ova/order.txt; make dc-order gives it another place$'):
            ld.coordinate(gcclink.read(self.spec), self.out)
        self.assertEqual(self.run_order(self.HOT),
                         self.GIVES.format(0) + "ova: large.o's .text.large takes a place 0xa0 bytes into .text, and "
                         'fits its own, 0x40 bytes in, no longer\n'
                         + self.ENDS.format('0xc8', '0x80', '72 bytes on') + self.WAS.format(3))
        self.assertEqual(self.searched(),
                         [*tight[:3], tight[4], ['searched', 'large.o', '.text.large', '0xa0', '0x28']])
        self.assertEqual(Path('config/ova/kernel.txt').read_text(),
                         KERNEL.replace('warm   large.s  large     # the large one', 'warm   pad.s    pad', 1)
                         .replace('# between them\nwarm   pad.s    pad',
                                  '# between them\nwarm   large.s  large     # the large one'))
        self.carry()
        ld.coordinate(gcclink.read(self.spec), self.out)

    def test_ends_the_line_of_a_warm_row_that_leaves_the_end_of_a_table_without_a_last_newline(self):
        # the table ends with pad's row and no newline. large, which outgrew its place under pad, moves past it, so
        # pad's row takes large's line and large's the last: each is a line of its own still, with its comment
        self.run_order(self.HOT)
        self.carry([*self.searched()[:4], ['searched', 'pad.o', '.text.pad', '0x60', '0x20']])
        head = KERNEL[:KERNEL.index('warm   large')]
        large, pad = 'warm   large.s  large     # the large one', 'warm   pad.s    pad  # the pad'
        Path('config/ova/kernel.txt').write_text(f'{head}{large}\n{pad}')
        self.run_order(self.HOT)
        self.assertEqual(Path('config/ova/kernel.txt').read_text(), f'{head}{pad}\n{large}\n')
        self.assertEqual([row[1:4] for row in self.searched()[3:]], [['pad.o', '.text.pad', '0x60'],
                                                                     ['large.o', '.text.large', '0xa0']])

    def test_moves_no_row_that_fits_and_records_the_kernels_the_sizes_text_and_the_heat_again(self):
        # the record has the kernel at another size, large at 36 bytes, a row no table has, .text a line back and
        # another heat's hash. large's 40 bytes fit under pad still, so each row keeps its place. order.py says each
        # thing the record held otherwise, that the rows end 80 bytes short of where the row no table has did, and
        # what the rows cost as the record had them: nothing, a set short of the fixed line's
        self.run_order(self.HOT)
        rows = self.searched()
        stale = [['heat', '0' * 64], ['text', '0x0c900000'], [*rows[2][:4], '0x4'], [*rows[3][:4], '0x24'], rows[4],
                 ['searched', 'ova.o', '.text.step', '0x100', '0x10']]
        self.carry(stale)
        self.spec.write_text(self.spec.read_text().replace('warm      ova  ova.o  .text.step\n', ''))
        self.assertEqual(self.run_order(self.HOT), self.GIVES.format(3)
                         + "ova: config/ova/heat.txt isn't the heat its record was written by\n"
                         "ova: .text starts at 0x0c900020, and its record had it at 0x0c900000, so each kernel and "
                         "warm row's line lies 1 of the cache's sets on\n"
                         "ova: small.o's .text.small lies 0x0 bytes into .text and takes 8, and its record had it 0x0 "
                         'bytes in, taking 4\n'
                         "ova: its record held ova.o's .text.step, which no kernel or warm row places now\n"
                         + self.ENDS.format('0xc0', '0x110', '80 bytes back') + self.WAS.format(0))
        self.assertEqual(self.searched(), rows)
        self.assertEqual(Path('config/ova/kernel.txt').read_text(), KERNEL)

    def test_records_the_hash_of_the_heat_it_costed_the_rows_by_where_the_heat_changes_while_it_links(self):
        # a heat with no stretch takes the heat's place while the layout pass links. The rows cost what the heat
        # order.py read gives them, and the record takes that heat's hash, so a link fails on the heat written since
        link = ld.link

        def linked(*arguments):
            Path('config/ova/heat.txt').write_text('frames  2\n')
            return link(*arguments)

        with mock.patch.object(ld, 'link', linked):
            self.assertIn(self.GIVES.format(3), self.run_order(self.HOT))
        self.assertEqual(self.searched()[0], ['heat', hashlib.sha256(self.HOT.encode()).hexdigest()])
        self.carry()
        with self.assertRaisesRegex(SystemExit, "ova: config/ova/heat.txt isn't the heat config/ova/order.txt was "
                                                'written by; make dc-order records the rows again$'):
            ld.coordinate(gcclink.read(self.spec), self.out)

    def test_says_how_far_the_rows_end_from_where_the_record_had_its_last_row_end_by_its_place_and_size(self):
        # the record has pad, the last row, at 24 bytes. pad's 32 keep its place, so no row moves, and the rows end 8
        # bytes past where the record had pad end
        self.run_order(self.HOT)
        rows = self.searched()
        short = [*rows[:4], [*rows[4][:4], '0x18']]
        self.carry(short)
        self.assertEqual(self.run_order(self.HOT),
                         self.GIVES.format(3) + self.ENDS.format('0xc0', '0xb8', '8 bytes on'))
        self.assertEqual(self.searched(), rows)
        # in a window of six lines the code after the rows starts at the window's end, wherever the rows end
        self.carry(short)
        self.window(6)
        self.assertEqual(self.run_order(self.HOT), self.GIVES.format(3))
        self.assertEqual(self.searched(), rows)

    def test_says_where_text_starts_now_and_what_the_rows_cost_before_when_a_head_that_grew_moved_them_all(self):
        # more's 64 bytes in the head move .text two lines on, and large off the fixed line's set. The link fails
        # naming .text, and order.py, which moves no row, says where .text lay and what the rows cost there
        self.run_order(self.HOT)
        self.carry()
        rows = self.searched()
        self.spec.write_text(self.spec.read_text() + 'input     ova  more.o\nhead      ova  DHEAD  more.o  .data\n')
        with self.assertRaisesRegex(SystemExit, r'ova: \.text starts at 0x0c900060, and config/ova/order.txt has it at '
                                                '0x0c900020; make dc-order records the rows again$'):
            ld.coordinate(gcclink.read(self.spec), self.out)
        self.assertEqual(self.run_order(self.HOT), self.GIVES.format(0)
                         + 'ova: .text starts at 0x0c900060, and its record had it at 0x0c900020, so each kernel and '
                         "warm row's line lies 2 of the cache's sets on\n" + self.WAS.format(3))
        self.assertEqual(self.searched(), [rows[0], ['text', '0x0c900060'], *rows[2:]])
        # a .text that moves the cache's 8 KB keeps each line's set, and one that moves part of a line none
        moved = gcclink.read(self.spec)['ova']
        for by, how in ((0x2000, "and each kernel and warm row's line keeps its set"),
                        (4, 'so each kernel and warm row lies on other lines')):
            self.assertEqual(order.differs(moved._replace(text=0x0c900060 - by), rows[0][1], 0x0c900060, []),
                             [f'ova: .text starts at 0x0c900060, and its record had it at {0x0c900060 - by:#010x}, '
                              f'{how}'])

    def test_gives_a_new_row_a_place_among_the_rows_that_have_theirs_and_moves_none_of_them(self):
        # the record has pad alone, five lines past the window's end. large, a new row, takes the lines under it:
        # the window's end costs 3 against the fixed line, and the next line nothing
        self.run_order(self.HOT)
        rows = self.searched()
        self.carry([*rows[:3], ['searched', 'pad.o', '.text.pad', '0xe0', '0x20']])
        self.spec.write_text(self.spec.read_text() + 'warm      ova  large.o  .text.large\n')
        Path('config/ova/kernel.txt').write_text(KERNEL.replace('warm   large.s  large     # the large one\n', '')
                                                 + 'warm   large.s  large\n')
        self.assertEqual(self.run_order(self.HOT), self.GIVES.format(0) + "ova: large.o's .text.large takes a place "
                         '0x60 bytes into .text, its first\n' + self.WAS.format(0))
        self.assertEqual(self.searched()[3:], [['searched', 'large.o', '.text.large', '0x60', '0x28'],
                                               ['searched', 'pad.o', '.text.pad', '0xe0', '0x20']])
        self.assertEqual(Path('config/ova/kernel.txt').read_text(),
                         KERNEL.replace('warm   large.s  large     # the large one\n', '')
                         .replace('warm   pad.s    pad\n', 'warm   large.s  large\n') + 'warm   pad.s    pad\n')

    def test_searches_the_whole_block_when_asked_and_writes_what_it_found_only_where_that_costs_less(self):
        # large lies at the window's end, on the fixed line's set: 3 a frame. The search gives each row a slot, its
        # lines and a spare one, and finds pad's slot first, which puts large two lines on: 0. The rows end where the
        # search leaves them, 24 bytes short of pad's end in the record
        self.run_order(self.HOT)
        self.carry()
        self.assertEqual(self.run_order(self.HOT, '--search'),
                         self.GIVES.format(3) + self.ENDS.format('0xa8', '0xc0', '24 bytes back')
                         + 'ova: a search of the whole block lays the rows where their pairs cost 0 a frame, and '
                         'moves 2 of them\n')
        self.assertEqual(self.searched()[3:], [['searched', 'pad.o', '.text.pad', '0x40', '0x20'],
                                               ['searched', 'large.o', '.text.large', '0x80', '0x28']])
        self.assertEqual(Path('config/ova/kernel.txt').read_text(),
                         KERNEL.replace('warm   large.s  large     # the large one', 'warm   pad.s    pad', 1)
                         .replace('# between them\nwarm   pad.s    pad',
                                  '# between them\nwarm   large.s  large     # the large one'))
        self.carry()
        ld.coordinate(gcclink.read(self.spec), self.out)
        # a second search takes the order the rows lie in, pad and then large, as the order that stood. It finds
        # nothing that costs less, and changes neither table
        tables = [Path(f'config/ova/{name}').read_text() for name in ('kernel.txt', 'order.txt')]
        with mock.patch.object(order, 'search', wraps=order.search) as searched:
            self.assertEqual(self.run_order(self.HOT, '--search'),
                             self.GIVES.format(0) + 'ova: a search of the whole block finds no places that cost less, '
                             'and moves no row\n')
        self.assertEqual(searched.call_args.args[3], [1, 0])
        self.assertEqual([Path(f'config/ova/{name}').read_text() for name in ('kernel.txt', 'order.txt')], tables)
        # rows that lie packed, large after pad at no cost, stay packed: their slots would cost no less
        rows = self.searched()
        packed = [*rows[:4], ['searched', 'large.o', '.text.large', '0x60', '0x28']]
        self.carry(packed)
        self.assertIn('finds no places that cost less', self.run_order(self.HOT, '--search'))
        self.assertEqual(self.searched(), packed)

    def test_packs_the_rows_in_the_table_s_order_when_asked_with_no_byte_free_and_a_link_then_lays_them_so(self):
        # the rows lie in their slots, large at the window's end and pad three lines on. Packed, pad follows large's
        # 40 bytes, and the rows end 56 bytes sooner
        self.run_order(self.HOT)
        self.carry()
        packs = ("ova: its 2 warm rows lie in the table's order, each after the one before on its alignment, and 1 of "
                 'them moved: a change to measure\n')
        self.assertEqual(self.run_order(self.HOT, '--pack'), self.GIVES.format(3) + packs
                         + self.ENDS.format('0x88', '0xc0', '56 bytes back') + self.WAS.format(3))
        rows = self.searched()
        self.assertEqual(rows[3:], [['searched', 'large.o', '.text.large', '0x40', '0x28'],
                                    ['searched', 'pad.o', '.text.pad', '0x68', '0x20']])
        self.assertEqual(Path('config/ova/kernel.txt').read_text(), KERNEL)
        self.carry()
        ld.coordinate(gcclink.read(self.spec), self.out)
        # a record that has pad where large's 40 bytes run 8 past it, as where large grew, keeps large where it
        # stands in the order, and moves pad the 8 bytes
        self.carry([*rows[:4], ['searched', 'pad.o', '.text.pad', '0x60', '0x20']])
        self.assertEqual(self.run_order(self.HOT, '--pack'), self.GIVES.format(3) + packs
                         + self.ENDS.format('0x88', '0x80', '8 bytes on') + self.WAS.format(3))
        self.assertEqual(self.searched(), rows)
        # an image without a record has its rows recorded where a link without one lays them
        Path('config/ova/order.txt').unlink()
        self.spec.write_text(Path('link.txt').read_text() + ROWS)
        self.assertEqual(self.run_order(self.HOT, '--pack'), self.GIVES.format(3)
                         + "ova: its 2 warm rows lie in the table's order, each after the one before on its "
                         'alignment, as a link without a record lays them\n')
        self.assertEqual(self.searched(), rows)
        # a search keeps its slots, so the two can't be asked for at once
        with self.assertRaises(SystemExit), redirect_stderr(io.StringIO()) as said:
            self.run_order(self.HOT, '--pack', '--search')
        self.assertIn('not allowed with argument', said.getvalue())

    def window(self, lines):
        """Give ova's warm rows a window of lines lines, in its table and in the test's spec."""
        Path('config/ova/kernel.txt').write_text(KERNEL + f'window warm  {lines}\n')
        kept = ''.join(line for line in self.spec.read_text().splitlines(keepends=True)
                       if line.split()[:3] != ['window', 'ova', 'warm'])
        self.spec.write_text(kept + f'window    ova  warm  {lines}\n')

    def follows(self):
        """Where a link by the test's spec starts ova.o's code, which follows ova's warm rows and head.o's 4 bytes of
        code, in bytes into .text."""
        _, layouts, _ = ld.coordinate(gcclink.read(self.spec), self.out)
        return min(each.address for each in layouts['ova'].placements
                   if each.output == '.text' and each.file == 'ova.o') - 0x0c900020

    def test_a_row_that_outgrew_its_place_takes_one_under_its_window_s_end_and_moves_no_code_after_the_window(self):
        # the warm rows have six lines from the window's end, 0x40 bytes in, so the code after them starts 0x100
        # bytes in. large, which outgrew its place under pad, takes the place past pad's spare line, and ends under
        # 0x100
        self.run_order(self.HOT)
        rows = self.searched()
        tight = [*rows[:4], ['searched', 'pad.o', '.text.pad', '0x60', '0x20']]
        self.carry([*rows[:3], tight[4], ['searched', 'large.o', '.text.large', '0x80', '0x28']])
        self.window(6)
        self.assertEqual(self.follows(), 0x104)
        self.carry(tight)
        self.window(6)
        self.assertEqual(self.run_order(self.HOT),
                         self.GIVES.format(0) + "ova: large.o's .text.large takes a place 0xa0 bytes into .text, and "
                         'fits its own, 0x40 bytes in, no longer\n' + self.WAS.format(3))
        self.carry()
        self.window(6)
        self.assertEqual(self.follows(), 0x104)
        self.assertEqual(Path('config/ova/kernel.txt').read_text().splitlines()[-1], 'window warm  6')

    def test_fails_where_no_place_under_the_window_s_end_is_free_and_writes_no_rows_laid_past_it(self):
        self.run_order(self.HOT)
        rows = self.searched()
        tight = [*rows[:4], ['searched', 'pad.o', '.text.pad', '0x60', '0x20']]
        wider = ('a wider window in config/ova/kernel.txt holds more, and one wider by a multiple of 256 lines leaves '
                 'the code after it on its sets')
        # four lines end 0xc0 bytes in, 8 short of large's 40 bytes from the place past pad's spare line
        self.carry(tight)
        self.window(4)
        with self.assertRaisesRegex(SystemExit, r": ova: no place under the end of its warm rows' window, 0xc0 bytes "
                                                r"into \.text, is free for large\.o's \.text\.large, which takes 40 "
                                                r'bytes\n.*: ova: make dc-order PACK=1 lays the rows with no byte free '
                                                f'among them, and {wider}$'):
            self.run_order(self.HOT)
        self.assertEqual(self.searched(), rows)
        # packed, the rows end 0x88 bytes in, and the code after the window stays
        self.assertIn('1 of them moved', self.run_order(self.HOT, '--pack'))
        self.assertEqual([row[3] for row in self.searched()[3:]], ['0x40', '0x68'])
        self.carry()
        self.window(4)
        self.assertEqual(self.follows(), 0xc4)
        # a search's slots end 0xa8 bytes in, 8 past three lines, so none of what it found is written
        packed = self.searched()
        self.window(3)
        past = ('ova: a search of the whole block lays the rows 8 bytes past the end of their window, so none of it is '
                'written: the tables are as a run without the search writes them\n')
        self.assertEqual(self.run_order(self.HOT, '--search'), self.GIVES.format(3) + past)
        self.assertEqual(self.searched(), packed)
        # the run writes the tables still, as one without the search does: by a heat measured again the rows keep
        # their places, and the record takes the heat's hash
        again = self.HOT + '# measured again\n'
        self.assertEqual(self.run_order(again, '--search'), self.GIVES.format(3)
                         + "ova: config/ova/heat.txt isn't the heat its record was written by\n" + self.WAS.format(3)
                         + past)
        packed = [['heat', hashlib.sha256(again.encode()).hexdigest()], *packed[1:]]
        self.assertEqual(self.searched(), packed)
        # two lines end 8 short of the rows packed
        self.window(2)
        with self.assertRaisesRegex(SystemExit, ': ova: its warm rows, laid with no byte free among them, run 8 bytes '
                                                r'past the end of their window, 0x80 bytes into \.text; ' f'{wider}$'):
            self.run_order(self.HOT, '--pack')
        self.assertEqual(self.searched(), packed)

    def test_lays_the_warm_rows_after_the_kernels_where_the_image_has_no_window(self):
        # without the window, .text starts at the head's end, 8 bytes past the origin and on no line, and the rows at
        # small's end, 8 bytes in. large takes the first line past it, which starts 0x18 bytes into .text
        self.spec.write_text(Path('link.txt').read_text() + ROWS.replace('window    ova  draw  2\n', ''))
        Path('config/ova/kernel.txt').write_text(KERNEL.replace('window draw  2\n', ''))
        self.run_order('frames  2\n')
        self.assertEqual([row[1:] for row in self.searched()[2:]], [['small.o', '.text.small', '0x0', '0x8'],
                                                                    ['large.o', '.text.large', '0x18', '0x28'],
                                                                    ['pad.o', '.text.pad', '0x78', '0x20']])
        self.assertEqual(self.searched()[1], ['text', '0x0c900008'])
        # a record that has large under the kernel's end gives it a place past it, the free line under pad
        self.carry([*self.searched()[:3], ['searched', 'large.o', '.text.large', '0x4', '0x28'], self.searched()[4]])
        self.spec.write_text(self.spec.read_text().replace('window    ova  draw  2\n', ''))
        self.assertIn("large.o's .text.large takes a place 0x18 bytes into .text, and fits its own, 0x4 bytes in, no "
                      'longer', self.run_order('frames  2\n'))

    def test_fails_on_a_heat_that_names_no_image_of_the_spec_and_on_a_table_of_other_warm_rows(self):
        with self.assertRaisesRegex(SystemExit, r'config/ova/heat.txt: names ovc, and \S+\.txt gives no such image$'):
            self.run_order('frames  2\ncode  ovc  ova.o  .text.step\n0  4  8\n')
        Path('config/ova/kernel.txt').write_text(KERNEL + 'warm   ova.c    ova_unused\n')
        with self.assertRaisesRegex(SystemExit, r'config/ova/kernel.txt: holds 3 warm rows, and \S+\.txt gives its '
                                                'image 2; the spec is of another table$'):
            self.run_order('frames  2\n')
        self.assertFalse(Path('config/ova/order.txt').exists())

    def test_fails_on_a_table_of_as_many_warm_rows_that_aren_t_the_spec_s_row_for_row_before_it_links(self):
        # the spec gives ova large's row and then pad's, as the table had them when it was written. A table that has
        # as many rows since, and another row where the spec has one, is another table: order.py names the first such
        # row, and neither links nor writes
        large, pad = 'warm   large.s  large     # the large one', 'warm   pad.s    pad'
        swapped = KERNEL.replace(large, pad, 1).replace(f'# between them\n{pad}', f'# between them\n{large}')
        cases = {'the rows the other way round': (swapped, 6, "large.o's .text.large"),
                 'another row in a row\'s place': (KERNEL.replace(pad, 'warm   ova.c    ova_unused'), 8,
                                                  "pad.o's .text.pad"),
                 'a function the row\'s object defines nowhere': (KERNEL.replace(pad, 'warm   pad.s    large'), 8,
                                                                 "pad.o's .text.pad"),
                 'the row\'s function, of another source': (KERNEL.replace(pad, 'warm   kept.s   pad'), 8,
                                                           "pad.o's .text.pad"),
                 'a row without its FUNCTION': (KERNEL.replace(large, 'warm   large.s'), 6, "large.o's .text.large")}
        for case, (table, line, section) in cases.items():
            with self.subTest(case):
                Path('config/ova/kernel.txt').write_text(table)
                with self.assertRaisesRegex(SystemExit, rf'config/ova/kernel.txt:{line}: not the warm row \S+\.txt '
                                                        f'gives its image there, of {re.escape(section)}; the spec is '
                                                        'of another table$'):
                    self.run_order('frames  2\n')
                self.assertEqual(Path('config/ova/kernel.txt').read_text(), table)
                self.assertEqual([Path('config/ova/order.txt').exists(), self.out.exists()], [False, False])
        # a table whose comments and spaces alone changed is the spec's still
        edited = KERNEL.replace('     # the large one', '  # the larger one').replace('# between them\n', '')
        Path('config/ova/kernel.txt').write_text(edited)
        self.run_order('frames  2\n')
        self.assertEqual(Path('config/ova/kernel.txt').read_text(), edited)
        self.assertEqual([row[1:3] for row in self.searched()[3:]],
                         [['large.o', '.text.large'], ['pad.o', '.text.pad']])

    def test_fails_on_a_spec_none_of_whose_images_has_a_warm_row(self):
        with self.assertRaisesRegex(SystemExit, 'link.txt: no image has a warm row$'):
            order.main(['link.txt', str(self.out)])

    def test_removes_the_record_of_an_image_whose_warm_rows_have_gone_which_a_link_would_hold_it_to(self):
        # the table's warm rows go, and the spec carries the record still: the link fails and names make dc-order,
        # which removes the record, and the link of the spec without it passes
        self.run_order(self.HOT)
        self.carry()
        kept = ''.join(line for line in self.spec.read_text().splitlines(keepends=True) if not line.startswith('warm'))
        self.spec.write_text(kept)
        with self.assertRaisesRegex(SystemExit, r"ova: config/ova/order.txt holds large.o's \.text\.large, which no "
                                                'kernel or warm row places; make dc-order records the rows again$'):
            ld.coordinate(gcclink.read(self.spec), self.out)
        self.assertEqual(self.run_order(self.HOT), 'ova: no warm row, so its record goes\n')
        self.assertFalse(Path('config/ova/order.txt').exists())
        self.spec.write_text(''.join(line for line in kept.splitlines(keepends=True)
                                     if line.split()[0] not in ('heat', 'text', 'searched')))
        ld.coordinate(gcclink.read(self.spec), self.out)

    # large's first line is hot, a fixed line stays on the set the window's end gives the first warm row, and main's
    # start and ova's own step, which no row places, are hot too
    APART = ('frames  2\ncode  ova  large.o  .text.large\n0  4  8\ncode  main  main.o  .text.start\n0  2  6\n'
             'code  ova  ova.o  .text.step\n0  2  6\nfixed\n0x8c000060  2  6\n')

    def tables(self, spec):
        """What order.py prints for spec, a spec's text, from ova's tables as setUp() wrote them, once it has given
        each row a place and searched the whole block, with its tables after."""
        Path('config/ova/kernel.txt').write_text(KERNEL)
        Path('config/ova/order.txt').unlink(missing_ok=True)
        self.spec.write_text(spec)
        return (self.run_order(self.APART, '--search'), *(Path(f'config/ova/{name}').read_text()
                                                         for name in ('kernel.txt', 'order.txt')))

    def test_costs_an_order_the_same_wherever_the_code_no_row_places_lies(self):
        # 72 bytes linked ahead of main.o move main's start two lines on, and 8 linked ahead of ova.o move ova's step
        base = Path('link.txt').read_text()
        stood = self.tables(base + ROWS)
        self.assertIn("the heat gives 4 stretches, 2 of them of its rows' sections or fixed, and the link holds 2 of "
                      "those; the rows' pairs cost 3 a frame where they lie", stood[0])
        self.assertIn('a search of the whole block lays the rows where their pairs cost 0 a frame', stood[0])
        moved = base.replace('input   main  main.o\n', 'input   main  pad.o\ninput   main  large.o\ninput   main  main.o\n')
        moved = moved.replace('input   ova   ova.o\n', 'input   ova   kept.o\ninput   ova   ova.o\n')
        self.assertEqual(moved.count('.o\n'), base.count('.o\n') + 3)
        self.assertEqual(self.tables(moved + ROWS), stood)

    def test_links_the_image_s_own_layout_pass_alone_in_a_load_area_that_runs_to_the_end_of_ram(self):
        # ova's 16 bytes of load area hold none of its code, so its link fails, and the search reads the pass still,
        # linked in the load area ld.py's own trial of a pass takes: from ova's origin to the end of RAM in its view.
        # main, which the heat names, links nothing
        short = Path('link.txt').read_text().replace('image   ova   0x0c900000  0x00010000',
                                                     'image   ova   0x0c900000  0x00000010')
        self.spec.write_text(short + ROWS)
        with self.assertRaisesRegex(SystemExit, 'ova: it runs past its load area'):
            ld.coordinate(gcclink.read(self.spec), self.out)
        shutil.rmtree(self.out)
        link, areas = ld.link, set()

        def linked(image, *arguments):
            areas.add((image.name, image.origin, image.length))
            return link(image, *arguments)

        with mock.patch.object(ld, 'link', linked):
            self.assertIn('ova: 2 warm rows; ', self.tables(short + ROWS)[0])
        self.assertEqual(areas, {('ova', 0x0c900000, 0x00700000)})
        self.assertEqual(sorted(each.name for each in self.out.iterdir()), ['ova'])

    def test_a_stretch_past_its_warm_row_s_end_weighs_on_no_row(self):
        # large is 40 bytes, and its one stretch lies 0x100 bytes in, where the heat's code had it. From the window's
        # end that is set 11, where a fixed line stays, and it is the code of whatever row follows large
        heat = 'frames  2\ncode  ova  large.o  .text.large\n0x100  2  8\nfixed\n0x8c000160  2  6\n'
        self.assertIn("and the link holds 1 of those; the rows' pairs cost 0 a frame where they lie",
                      self.run_order(heat))
        self.assertEqual(Path('config/ova/kernel.txt').read_text(), KERNEL)

    def test_rows_that_meet_within_a_line_are_one_line_to_the_cost(self):
        # packed, pad follows large's 40 bytes, so large's last 8 bytes and pad's first lie on one line, on set 4,
        # where a fixed line of heat 3 a frame stays. Each row has a stretch of heat 4 a frame on the line: one line
        # of heat 4, and one pair, which costs 3
        heat = ('frames  2\ncode  ova  large.o  .text.large\n0x24  4  8\ncode  ova  pad.o  .text.pad\n0  2  8\n'
                'fixed\n0x8c000080  2  6\n')
        costs = "and the link holds 3 of those; the rows' pairs cost 3 a frame where they lie"
        self.assertIn(costs, self.run_order(heat, '--pack'))
        self.assertEqual([row[3] for row in self.searched()[3:]], ['0x40', '0x68'])
        # the rows keep those places, and cost the same there
        self.carry()
        self.assertIn(costs, self.run_order(heat))
        self.assertEqual([row[3] for row in self.searched()[3:]], ['0x40', '0x68'])
