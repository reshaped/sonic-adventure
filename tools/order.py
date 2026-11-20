"""Give each of an image's warm rows a place, and record the places and
what they were costed against.

order.py reads SPEC, the spec tools/spec.py writes, and for each of its
images with warm rows, config/IMAGE/heat.txt. It links the image's layout
pass under DIR/IMAGE/, as tools/ld.py does, but without the image's record
and in a load area that runs to the end of RAM, as ld.py's lifted() gives
it, so that a row that outgrew its place, or an image too long for its
own area, still gives its rows' sizes. It
then writes config/IMAGE/order.txt, the record that gives each warm row's
section its place, which ld.py lays the rows by and holds a link to, and
the warm rows of config/IMAGE/kernel.txt in their places' order, each
where a warm row stood. It measures nothing: the heat is a measurement,
and a cost a guess from it.

heat.txt gives how often a frame ran each stretch of code, on each route
it was measured on. Its first row is frames FRAMES..., the frames
measured on each route. A row code IMAGE PATH INPUT opens the stretches
of INPUT, an input section of IMAGE's object PATH, and a row fixed those
of code that lies in no image. Each other row is a stretch, OFFSET SIZE
RUNS...: SIZE bytes of instructions that start OFFSET bytes into the
section, or at the address OFFSET, each of which the frames of each route
ran RUNS times, a RUNS for each FRAMES. Where a function was entered less
often than one of its instructions ran, as one that loops, the stretch
gives the entries, since a visit loads a line once. PATH is spelled as
ld's map spells a file, but a member of the toolchain's libgcc.a, which
is libgcc.a(MEMBER). A stretch keeps its place in its section through a
link that moves the section, so a heat serves later links until the code
that is hot changes.

A stretch's heat on a route is its RUNS a frame. A line is 32 bytes of
the SH-4's instruction cache, which holds one line a set and 256 sets,
and a line's heat is the most of the stretches that touch it, in
whichever sections they lie: sections that meet within a line share it,
as a section and the code that stays can. Two lines on one set that take
turns evict each other, and the heat gives no order of runs, so a pair
of lines on one set costs the lesser heat of the two. What the rows cost
where they lie is the sum of every pair with the warm rows' sections
there, less the sum of every pair of the code that stays alone, over the
routes. So it counts each section's lines against those of the code that
stays, and against those of the other warm rows' sections, and for a
line a section shares with the code that stays, only what the section
adds to the line's heat. What one row costs at a place is what laying it
there adds to that sum. The code that stays is the image's own kernels',
where the layout pass has them, and the fixed stretches. The cost reads
no other code. The image's other code, and each other image's, lies
where the size of the code linked ahead of it puts it, and the link-time
compile lays it out again, so the record holds none of it. So each
address the cost reads is one order.txt holds, and a link that ld.py
passes is the link the places were costed for, with the link-time
compile or without it.

A warm row's section keeps the place the record gives it while it fits
there: on its alignment, at or past the warm rows' start, which is the
end of the last window or of the kernels, and ending at or under the next
row's place. So order.py moves no row that fits, whatever else changed. It
writes the record again, with the kernels where the link has them now,
and gives a place only to a row that has none, as a new one, or that fits
its own no longer, as one that outgrew the bytes up to the next row. Such
a row takes the free place that costs least against the code that stays
and the rows that have places, and the lowest of those that cost the
same. A free place starts on a line, a spare line past the last line of
the row before it, and leaves a spare line after the row's own last line,
so the row can grow into its spare line and move nothing. The free places
are those among the rows, as where a row left, and the first past the last
row. The rows without places take theirs in (PATH, INPUT) order. A row
fits its place no longer where it outgrew the bytes up to the next row's
place, where its alignment rose off its place, or where the warm rows'
start moved past it, as where a window widened or, in an image without
windows, a kernel grew. Nothing else moves a warm row, and then that row
alone moves.

Where a window row of kernel.txt names warm, the warm rows have a window:
ld.py starts the image's other code at its end, so a row that moves
within it moves none of that code. A row then keeps its place only where
it ends at or under the window's end, and a place is free only where the
row would. order.py fails where a row has no place free, and where --pack
would lay the rows past the window's end, and writes nothing then: a
wider window holds more, and one wider by a multiple of the cache's 256
lines leaves the code after it on its sets. Where a search would lay the
rows past the window's end, order.py writes none of what it found, and
writes the tables as it does without --search.

Given --search, order.py then searches the whole block. It gives each
section a slot, its lines and a spare line, and lays the slots out from
the warm rows' start, each on a line. It starts from several orders. For
the first it takes, at each place in turn, the section left that costs
least there against the code that stays and the sections laid already,
the first in (PATH, INPUT) order where several cost the same. The second
is the order the rows lie in, and SHUFFLES more are shuffles of the (PATH,
INPUT) order by a generator of its own, the same on every host. From each
it takes each section in turn and tries it at each place up to REACH
places either side of where it stands, and keeps each move that costs
less, until a pass over the sections moves none. It takes the least of
the orders these end on, the first where several cost the same, and
writes it only where it costs less than the rows cost where they lay.
The cost is a guess: it knows which lines a frame enters and not in what
order, so it can't rank two layouts that cost about the same. A search
moves every row, so what it writes is a change to measure, and only a
measurement says whether it is faster.

A row that takes a place leaves the bytes it held free, and takes as many
again where it goes, so the rows' moves lengthen the image, and can leave
it too long for its load area. Given --pack, order.py searches nothing and
keeps no place: it gives each row the place the pass, which links without
the record, lays it at, in the table's order, after the row before it and
on its alignment, with no byte free among the rows. That frees the most
room, moves each row after the first free byte, and is a change to
measure. Given it where a row has just outgrown its place, before any
other run, it leaves the row where it stands in the order, and moves the
rows after it by the bytes it grew.

order.txt opens with heat SHA256, the SHA-256 of heat.txt, and text
ADDRESS, the address the image's .text starts at.
Then a row, searched PATH INPUT PLACE SIZE, follows for each kernel row's
section and each warm row's, in the table's order: the section lies PLACE
bytes into the image's .text and takes SIZE bytes. The fixed stretches lie
at addresses of their own, so the sets they share with the rows follow
.text's start, which the image's head, ahead of it, moves when it grows,
and the image's origin when that moves.

order.py prints, for each image, how many stretches the heat gives, how
many of them lie in the image's kernel and warm rows' sections or are
fixed, and how many of those the link still holds, with what the rows
cost where they lie, summed over the routes. A stretch of a section no
kernel or warm row places, or one that runs past its section's end,
counts for nothing. A line follows for each thing the record held
otherwise than order.py writes it: another heat, another address of
.text, a kernel row's section at another place or size, and a section no
row places now. Each is something the places were costed against, so the
cost has changed though no warm row moved. A line follows for each row it
gave a place. Where any of these lines follows, another gives what the
rows cost as the record had them and the kernels, by the heat as it is
now, beside what they cost where they lie. An image that had no record
takes a line that says every row took a place anew. Where the warm rows
have no window, and end elsewhere, as order.py writes them, than the
record had its rows end, by the PLACE and SIZE it gave each, a line says
how far that end moved: the code after them follows it. Given --search a
line says what the search found and whether it wrote it, and given --pack
one says how many rows moved.

order.py fails on SPEC as gcclink.py's read() does, on a link as ld.py's
link() does, and on:

- a SPEC none of whose images has warm rows or a record. It removes the
  record of an image that has one and no warm row, which a link would
  hold the image to;
- a heat it can't read, or one with no row;
- a heat whose first row is no frames row, or gives a FRAMES of 0;
- a row of a heat that is none of these forms, or a stretch ahead of the
  first code or fixed row;
- a stretch with a RUNS too few or too many for the first row's FRAMES,
  or a SIZE of 0;
- a heat that names an image SPEC doesn't;
- a kernel.txt whose warm rows aren't SPEC's for the image, as where the
  table changed after SPEC was written: one that holds more of them or
  fewer, or a row that isn't warm SOURCE FUNCTION for the row SPEC has
  in its place, SOURCE the source make dc compiles to that row's PATH
  and FUNCTION a name PATH defines in that row's INPUT and in no other
  allocated section, naming the first such row's path:line;
- a warm row that fits its place no longer, or a new one, for which no
  place under the end of the warm rows' window is free, naming each;
- --pack where the rows, laid with no byte free, run past that end.
"""

import argparse
import hashlib
import re
import sys
from bisect import bisect_left
from collections import namedtuple
from math import gcd
from pathlib import Path

import elf
import gcclink
import ld
from config import gcc_object
from images import CONFIG
from tables import hexa
from tool import content, fail, publish, remove, stoppable

# how many places either side of a section's own the search tries it at,
# and how many shuffled orders it starts from besides its first and the one
# the rows lie in
REACH, SHUFFLES = 16, 8
# the bytes a row that takes a place leaves free after its last line
SPARE = ld.ICACHE_LINE
# an address past every place, for the end of warm rows without a window
RAM_END = 1 << 32
# the words that open a heat's rows of a section's stretches and of the
# fixed ones
CODE, FIXED = 'code', 'fixed'
# a PATH as a heat spells one: as gcclink.py's, then (MEMBER) for an
# archive's member
MAPPED = re.compile(rf'(?:{gcclink.PATH.pattern})(?:\([A-Za-z0-9_.-]+\))?')
# the spellings of a code row's IMAGE, PATH and INPUT
SECTION = (gcclink.IMAGE, MAPPED, gcclink.INPUT)
# the comment that opens order.txt
HEADER = '''\
# Where each of kernel.txt's warm rows lies, and what its place was costed
# against, as tools/order.py wrote it. heat is the SHA-256 of heat.txt,
# the measured heat the cost read, and text the address the image's .text
# started at. A searched row, searched PATH INPUT PLACE SIZE, follows for
# each kernel row's section and each warm row's, in kernel.txt's order:
# INPUT, an input section of the object PATH, lies PLACE bytes into that
# .text and took SIZE bytes. tools/ld.py lays each warm row's section at
# its PLACE, so a row that grows within the bytes up to the next row's
# PLACE moves nothing. It fails a link where a warm row runs past the
# next row's PLACE, where heat.txt has another hash, where .text starts
# elsewhere, and where a kernel row's section lies elsewhere or is
# another size, since the places were costed against the kernels and the
# heat's fixed stretches, and no other code. make dc-order then writes
# this table again, and says what it held otherwise: it moves no warm row
# that fits where it lies, and gives a row that doesn't, or a new one, a
# place on a line's start with a spare line after it. make dc-order
# SEARCH=1 also searches the whole block, and writes what it finds where
# that costs less, and make dc-order PACK=1 lays the rows in kernel.txt's
# order with no byte free among them. Each is a change to measure, since
# it moves rows that fit where they lie.
'''

# a heat as heat() reads it: the frames measured on each route; code, which
# maps (IMAGE, PATH, INPUT) to its stretches, each (OFFSET, SIZE, runs), runs
# a tuple with a RUNS for each route; and fixed, the fixed stretches alike,
# OFFSET an address
Heat = namedtuple('Heat', 'frames code fixed')
# a warm row's section for search(): its size and alignment in bytes, and
# its stretches, each (offset, size, heats), heats a tuple with a heat for
# each route. Each stretch ends at or under the section's size
Section = namedtuple('Section', 'size align stretches')
# a section at an address, as Costs.lay() gives it. Its lines on a route
# are those its stretches give a heat there. lines gives, for each route,
# each line's heat, by line, under the line's set. span is the first of
# its lines and the last, on any route, or None for a section with none.
# Its lines that no code that stays lies on are in apart, what they cost
# against the code that stays, and in own, which gives them for each
# route, each its set and heat as one number. rest gives its other lines
# for each route, each (line, heat)
Laid = namedtuple('Laid', 'lines span apart own rest')


def heat(path):
    """The heat at path, as a Heat, and the SHA-256 of the file, as (heat,
    digest). It reads the file once, so the digest is of the bytes the
    Heat is read from. It fails, naming the row's path:line, as the
    module's docstring lists, and naming path on a heat with no row."""
    data = content(path)
    frames, code, fixed, under = None, {}, [], None
    # each row as tables.py's rows() gives one, but of these bytes: rows() reads the file itself
    for number, line in enumerate(data.split(b'\n'), 1):
        where = f'{path}:{number}'
        try:
            fields = line.decode().split('#')[0].split()
        except UnicodeDecodeError:
            fail(f"{where}: a byte that isn't UTF-8")
        if not fields:
            continue
        if frames is None:
            if (fields[0] != 'frames' or len(fields) < 2
                    or not all(gcclink.NUMBER.fullmatch(each) for each in fields[1:])):
                fail(f'{where}: not a row: frames FRAMES...')
            frames = tuple(int(each, 0) for each in fields[1:])
            if 0 in frames:
                fail(f'{where}: a FRAMES of 0')
        elif fields == [FIXED]:
            under = fixed
        elif (fields[0] == CODE and len(fields) == 4
              and all(spelling.fullmatch(field) for spelling, field in zip(SECTION, fields[1:]))):
            under = code.setdefault(tuple(fields[1:]), [])
        elif under is None or len(fields) < 2 or not all(gcclink.NUMBER.fullmatch(each) for each in fields):
            fail(f'{where}: not a row: code IMAGE PATH INPUT, fixed, or after one of them OFFSET SIZE RUNS...')
        elif len(fields) - 2 != len(frames):
            fail(f'{where}: {len(fields) - 2} RUNS, and the first row gives {len(frames)} FRAMES')
        else:
            offset, size, *runs = (int(each, 0) for each in fields)
            if not size:
                fail(f'{where}: a SIZE of 0')
            under.append((offset, size, tuple(runs)))
    if frames is None:
        fail(f'{path}: no row: frames FRAMES...')
    return Heat(frames, code, fixed), hashlib.sha256(data).hexdigest()


def weights(frames):
    """What multiplies each route's RUNS into a heat, and the frames the
    heats are then over, the least number each of frames divides, as
    (weights, frames). So each heat is a whole number, and a cost, a sum of
    them, is the same in whatever order it adds up."""
    least = 1
    for each in frames:
        least = least * each // gcd(least, each)
    return tuple(least // each for each in frames), least


def line_of(address):
    """The line of the instruction cache address lies on, in either view
    of RAM."""
    return (address & (gcclink.RAM - 1)) // ld.ICACHE_LINE


def pairs(heats):
    """What lines of heats, on one set, cost each other: the lesser heat
    of each two. In falling order a line pairs with each line before it,
    and is the lesser of each pair."""
    return sum(each * rank for rank, each in enumerate(sorted(heats, reverse=True)))


class Costs:
    """What sections, Sections, cost where they lie, as the module's
    docstring gives a cost. stays gives, for each route, the heat of each
    line of the code that stays, by the line, as staying() does."""

    def __init__(self, sections, stays):
        self.sections, self.stays, self.routes = sections, stays, range(len(stays))
        # the heats of the lines that stay on each set, in rising order, and
        # each list with the sums of its first heats
        self.held = [{} for _ in self.routes]
        for route in self.routes:
            for line, each in stays[route].items():
                self.held[route].setdefault(line % ld.ICACHE_LINES, []).append(each)
        self.summed = [{on: [0] for on in each} for each in self.held]
        for route in self.routes:
            for on, heats in self.held[route].items():
                heats.sort()
                for each in heats:
                    self.summed[route][on].append(self.summed[route][on][-1] + each)
        # a heat fits under shift bits, so a set and a heat make one number
        self.shift = max([each.bit_length() for section in sections for _, _, heats in section.stretches
                          for each in heats] + [each.bit_length() for lines in stays for each in lines.values()] + [1])
        self.mask = (1 << self.shift) - 1
        self.laid = {}

    def against(self, route, on, heat):
        """What a line of heat on the set on costs against the lines of
        the code that stays there, on route: the heats under its own, and
        its own for every other."""
        heats = self.held[route].get(on)
        if not heats:
            return 0
        below = bisect_left(heats, heat)
        return self.summed[route][on][below] + heat * (len(heats) - below)

    def lay(self, index, at):
        """sections[index] laid at at, an address, as a Laid. A line keeps
        its number, so that a line two sections lie on, or a section and
        the code that stays, is one line to of() and to beside()."""
        if (index, at) not in self.laid:
            lines, apart, own, rest = [], 0, [], []
            for route in self.routes:
                heats = {}
                for offset, size, each in self.sections[index].stretches:
                    if each[route]:
                        for line in range(line_of(at + offset), line_of(at + offset + size - 1) + 1):
                            heats[line] = max(heats.get(line, 0), each[route])
                sets = {}
                for line, each in heats.items():
                    sets.setdefault(line % ld.ICACHE_LINES, {})[line] = each
                lines.append(sets)
                free = {line: each for line, each in heats.items() if line not in self.stays[route]}
                apart += sum(self.against(route, line % ld.ICACHE_LINES, each) for line, each in free.items())
                own.append([(line % ld.ICACHE_LINES) << self.shift | each for line, each in free.items()])
                rest.append([(line, each) for line, each in heats.items() if line not in free])
            hot = [line for sets in lines for each in sets.values() for line in each]
            self.laid[(index, at)] = Laid(lines, (min(hot), max(hot)) if hot else None, apart, own, rest)
        return self.laid[(index, at)]

    def of(self, lies):
        """What the sections cost where lies has them, each (index,
        address): what the lines on each set cost each other with the
        sections there, less what the lines of the code that stays cost
        each other alone."""
        laid = [self.lay(index, at) for index, at in lies]
        # sections that come in rising order, each one's lines past the
        # one before's, as a search lays its slots, share no line with each
        # other. Then each one's apart and own stand as lay() counted them
        spans = [each.span for each in laid if each.span]
        clear = all(one[1] < other[0] for one, other in zip(spans, spans[1:]))
        total = sum(each.apart for each in laid) if clear else 0
        for route in self.routes:
            # lines gives each line that is costed against the code that
            # stays already, and shared the heat of each other line, by line
            lines, shared = [], {}
            if clear:
                for each in laid:
                    lines += each.own[route]
                    if each.rest[route]:
                        shared.update(each.rest[route])
            else:
                for each in laid:
                    for own in each.lines[route].values():
                        for line, heat in own.items():
                            if heat > shared.get(line, 0):
                                shared[line] = heat
            # a line that sections share is one line, of the most of their
            # heats and of the heat of the code that stays on it. stayed
            # gives, by set, the heats of the code that stays on such lines
            stayed = {}
            for line, heat in shared.items():
                stay, on = self.stays[route].get(line, 0), line % ld.ICACHE_LINES
                total += self.against(route, on, max(heat, stay))
                lines.append(on << self.shift | max(heat, stay))
                if stay:
                    stayed.setdefault(on, []).append(stay)
            # in falling order a line pairs with each line before it on its
            # set, and is the lesser of each pair
            lines.sort(reverse=True)
            last, rank = None, 0
            for line in lines:
                if line >> self.shift != last:
                    last, rank = line >> self.shift, 0
                total += (line & self.mask) * rank
                rank += 1
            # the sums above count each heat of stayed as a line that
            # stays, apart from the line laid on it. They are one line, so
            # take back what the heat costs against each line laid on its
            # set. Take back too what it costs against the other lines that
            # stay, which the code that stays costs alone, and a pair of
            # two such heats once
            for on, heats in stayed.items():
                total -= sum(min(line & self.mask, heat) for line in lines if line >> self.shift == on
                             for heat in heats)
                total -= sum(self.against(route, on, heat) - heat for heat in heats) - pairs(heats)
        return total

    def alone(self):
        """The lines of the code that stays alone, as take() and beside()
        take the lines that lie: for each route, the heat of each line, by
        line, under the line's set."""
        taken = [{} for _ in self.routes]
        for route in self.routes:
            for line, each in self.stays[route].items():
                taken[route].setdefault(line % ld.ICACHE_LINES, {})[line] = each
        return taken

    def take(self, taken, index, at):
        """Add the lines of sections[index], laid at at, to taken, which
        alone() gave: a line that lies already takes the most of its heat
        and the section's."""
        for route, sets in enumerate(self.lay(index, at).lines):
            for on, own in sets.items():
                lie = taken[route].setdefault(on, {})
                for line, each in own.items():
                    lie[line] = max(lie.get(line, 0), each)

    def beside(self, taken, index, at):
        """What laying sections[index] at at adds to the cost of the
        sections taken, which is of() with it less of() without: on each
        set its lines lie on, what the set's lines cost each other with
        them, less what they cost without. taken is as take() leaves it."""
        total = 0
        for route, sets in enumerate(self.lay(index, at).lines):
            for on, own in sets.items():
                lie = taken[route].get(on, {})
                both = dict(lie)
                for line, each in own.items():
                    both[line] = max(both.get(line, 0), each)
                total += pairs(both.values()) - pairs(lie.values())
        return total


def packed(start, sections, order):
    """Where each of sections lies, laid from start, an address, in order,
    a list of their indexes, each after the last on its alignment, as ld
    lays input sections: (index, address) for each, in order."""
    lies, at = [], start
    for index in order:
        at = -(-at // sections[index].align) * sections[index].align
        lies.append((index, at))
        at += sections[index].size
    return lies


def shuffled(count, seed):
    """The numbers under count, in an order that seed, a whole number,
    decides. Its arithmetic is whole numbers', so the order is the same on
    every host and under every Python."""
    order, state = list(range(count)), seed
    for last in range(count - 1, 0, -1):
        state = (state * 6364136223846793005 + 1442695040888963407) % (1 << 64)
        other = (state >> 33) % (last + 1)
        order[last], order[other] = order[other], order[last]
    return order


def search(start, sections, stays, stood, shuffles=SHUFFLES):
    """The order to lay sections, Sections, from start, an address, as a
    list of their indexes, with the cost of stood, an order of them, and of
    the one found, as (order, before, after). stays is as Costs takes it.
    The module's docstring says how the search goes: it starts from its
    first order, from stood, and from shuffles shuffled ones. Where several
    sections cost the same at a place, the first order takes the first of
    them in sections. Where the order found costs no less than stood, stood
    is the order, and its cost both."""
    costs = Costs(sections, stays)

    def cost(trial):
        return costs.of(packed(start, sections, trial))

    def first():
        left, found, at = list(range(len(sections))), [], start
        taken = costs.alone()
        while left:
            index = min(left, key=lambda index: costs.beside(taken, index, packed(at, sections, [index])[0][1]))
            at = packed(at, sections, [index])[0][1]
            costs.take(taken, index, at)
            left.remove(index)
            found.append(index)
            at += sections[index].size
        return found

    def moved(found):
        # found once no move of one section costs less, with its cost
        best, moving = cost(found), True
        while moving:
            moving = False
            for index in list(found):
                place = found.index(index)
                for other in range(max(0, place - REACH), min(len(found), place + REACH + 1)):
                    if other == place:
                        continue
                    trial = found[:place] + found[place + 1:]
                    trial.insert(other, index)
                    costs_now = cost(trial)
                    if costs_now < best:
                        best, found, place, moving = costs_now, trial, other, True
        return found, best

    before = cost(stood)
    found, best = moved(first())
    for begin in (list(stood), *(shuffled(len(sections), seed) for seed in range(1, shuffles + 1))):
        other, least = moved(begin)
        if least < best:
            found, best = other, least
    return (found, before, best) if best < before else (list(stood), before, before)


def settle(start, sections, costs, places, end=None):
    """Where each of sections lies once each has a place it fits, as a
    list of their addresses, with the indexes of those that took a place,
    in the order they took them: (lies, anew). places gives each section's
    address, or None for one that has none, start is the warm rows' start,
    an address, end the end of their window, or None where they have none,
    and costs a Costs of sections.

    A section keeps its address where that is on its alignment and at or
    past start, and the section ends at or under the address of the next
    one that keeps its own, or under end. Each other takes a place in turn,
    as the module's docstring says: the free place that costs least, the
    lowest of those that cost the same. A section holds the bytes from its
    address to SPARE bytes past its last line, and a place is free where
    the bytes the section would hold there are none that another holds,
    and the section ends at or under end there. A section no place is free
    for lies at None."""
    line = ld.ICACHE_LINE

    def held(index, at):
        return at, -(-(at + sections[index].size) // line) * line + SPARE

    lies, ceiling = {}, end
    # from the last down, so a section that runs past the next one's address
    # is the one that moves
    for index in sorted((index for index, at in enumerate(places) if at is not None),
                        key=lambda index: (places[index], sections[index].size), reverse=True):
        at = places[index]
        if at >= start and not at % sections[index].align and (ceiling is None or at + sections[index].size <= ceiling):
            lies[index] = ceiling = at
    taken = costs.alone()
    for index, at in lies.items():
        costs.take(taken, index, at)
    holds = [held(index, at) for index, at in lies.items()]
    anew = [index for index in range(len(sections)) if index not in lies]
    for index in anew:
        step = max(sections[index].align, line)
        at, free, last = -(-start // step) * step, [], max([high for _, high in holds] + [start])
        while (not free or free[-1] < last) and (end is None or at + sections[index].size <= end):
            high = held(index, at)[1]
            over = [above for low, above in holds if low < high and at < above]
            if over:
                at = -(-max(over) // step) * step
                continue
            free.append(at)
            at += step
        lies[index] = min(free, key=lambda at: (costs.beside(taken, index, at), at)) if free else None
        if free:
            costs.take(taken, index, lies[index])
            holds.append(held(index, lies[index]))
    return [lies[index] for index in range(len(sections))], anew


def staying(image, weighed, passed):
    """The heat of each line of the code that stays while image's warm
    rows' sections move, for each route by line, as Costs takes them, with
    how many of the heat's stretches lie in that code and how many of those
    the link holds, as (stays, held, given). The code that stays is image's
    kernel rows' sections, where passed, its layout pass, places them, and
    the fixed stretches. weighed is the heat, its runs as heats. A stretch
    that runs past its section's end counts for nothing: the code has
    changed since the heat was measured. So does a kernel row's section
    that passed doesn't place, as one a record that passed stands for
    didn't hold."""
    routes = range(len(weighed.frames))
    lines = [{} for _ in routes]
    held = given = len(weighed.fixed)

    def take(address, size, heats):
        for line in range(line_of(address), line_of(address + size - 1) + 1):
            for route in routes:
                lines[route][line] = max(lines[route].get(line, 0), heats[route])

    # reversed, for the first of an input section the map places twice
    places = {(each.file, each.section): each for each in reversed(passed.placements) if each.output == '.text'}
    for piece in image.kernel:
        place = places.get((piece.path, piece.input))
        stretches = weighed.code.get((image.name, piece.path, piece.input), [])
        given += len(stretches)
        for offset, size, heats in stretches:
            if place and offset + size <= place.size:
                held += 1
                take(place.address + offset, size, heats)
    for address, size, heats in weighed.fixed:
        take(address, size, heats)
    return [{line: each for line, each in lines[route].items() if each} for route in routes], held, given


def warm_lines(path, spec, image, objects):
    """The lines of the kernel table at path, and the index of each that
    is a warm row. Each line ends with a newline, which the last takes
    where the table gives it none, so that a row that leaves the table's
    end ends its line where it goes. objects maps the PATH of each of the
    warm rows spec, the spec's path, gives image, an Image, to its Elf.
    It fails where the table's warm rows aren't those rows, as the
    module's docstring says: naming path where they aren't as many, and
    the path:line of the first that isn't the spec's row in its place."""
    lines = content(path).decode().splitlines(keepends=True)
    if lines and not lines[-1].endswith('\n'):
        lines[-1] += '\n'
    at = [index for index, line in enumerate(lines) if line.split('#')[0].split()[:1] == ['warm']]
    if len(at) != len(image.warm):
        fail(f'{path}: holds {len(at)} warm rows, and {spec} gives its image {len(image.warm)}; the spec is of '
             'another table')
    for index, piece in zip(at, image.warm):
        named, found = lines[index].split('#')[0].split()[1:], objects[piece.path]
        allocated = elf.allocated_sections(found)
        # the sections of PATH that define FUNCTION, as tools/spec.py's kernel() finds a row's
        if (len(named) != 2 or str(gcc_object(named[0], image.name)) != piece.path
                or {allocated[symbol.section].name for symbol in found.symbols
                    if symbol.name == f'_{named[1]}' and elf.names_definition(symbol)
                    and symbol.section in allocated} != {piece.input}):
            fail(f"{path}:{index + 1}: not the warm row {spec} gives its image there, of {piece.path}'s "
                 f'{piece.input}; the spec is of another table')
    return lines, at


def record(digest, text, placed):
    """The text of an order.txt: HEADER, the heat's SHA-256 digest, text,
    .text's address, and a searched row for each of placed, (PATH, INPUT,
    place, size)."""
    paths, inputs = (max(len(each[field]) for each in placed) for field in (0, 1))
    return HEADER + f'heat      {digest}\ntext      {hexa(text)}\n' + ''.join(
        f'searched  {path:{paths}}  {section:{inputs}}  {place:#7x}  {size:#x}\n'
        for path, section, place, size in placed)


def differs(image, digest, address, kernel):
    """The lines that say what image's record held otherwise than order.py
    writes it now, but for its warm rows' places: none for an image without
    a record. digest is the heat's SHA-256, address .text's, and kernel the
    kernel rows' sections, as ld.placed() gives them. Each is something the
    places were costed against, so a line says a cost has changed though no
    warm row moved."""
    if not image.searched:
        return []
    said, then = [], {(row.path, row.input): row for row in image.searched}
    if not image.heat:
        said.append(f'{image.name}: its record named no heat')
    elif image.heat[1] != digest:
        said.append(f"{image.name}: {image.heat[0]} isn't the heat its record was written by")
    if image.text is None:
        said.append(f"{image.name}: its record didn't say where .text starts")
    elif image.text != address:
        # a move of whole lines takes each line that many sets on, round the cache
        lines, part = divmod(address - image.text, ld.ICACHE_LINE)
        said.append(f'{image.name}: .text starts at {hexa(address)}, and its record had it at {hexa(image.text)}, '
                    + ('so each kernel and warm row lies on other lines' if part else
                       f"so each kernel and warm row's line lies {lines % ld.ICACHE_LINES} of the cache's sets on"
                       if lines % ld.ICACHE_LINES else "and each kernel and warm row's line keeps its set"))
    for path, name, place, size in kernel:
        row = then.pop((path, name), None)
        if row is None:
            said.append(f"{image.name}: its record didn't hold {path}'s {name}, a kernel row's section")
        elif (row.place, row.size) != (place, size):
            said.append(f"{image.name}: {path}'s {name} lies {place:#x} bytes into .text and takes {size}, and its "
                        f'record had it {row.place:#x} bytes in, taking {row.size}')
    return said


def reorder(spec, images, image, out, whole=False, pack=False):
    """Give each of image's warm rows that has none a place, and write its
    kernel.txt and order.txt, as the module's docstring says. images is the
    images of spec, the spec's path, and out the directory the layout
    passes go under. Where whole is set, it searches the whole block too,
    and writes what the search found where that costs less. Where pack is
    set, it gives each row the place the pass does instead. It returns the
    lines main() prints.

    It links image's own layout pass and no other, without image's
    searched rows, so its warm rows follow each other in row order and one
    that outgrew its place fails no link. It reads of the pass only .text's
    start, the kernel rows' sections' places and each row's section's
    size, which are alike with the link-time compile and without it."""
    table, path = CONFIG / image.name / 'kernel.txt', CONFIG / image.name / 'heat.txt'
    # each warm row's object, by its PATH: the table's rows are held to them, and each section is aligned as its own
    # says
    objects = {}
    for piece in image.warm:
        if piece.path not in objects:
            objects[piece.path] = elf.read(content(piece.path), piece.path)
    lines, at = warm_lines(table, spec, image, objects)
    read, digest = heat(path)
    named = sorted({name for name, _, _ in read.code} - set(images))
    if named:
        fail(f'{path}: names {", ".join(named)}, and {spec} gives no such image')
    scale, frames = weights(read.frames)

    def weigh(stretches):
        return [(offset, size, tuple(count * weight for count, weight in zip(runs, scale)))
                for offset, size, runs in stretches]

    weighed = Heat(read.frames, {key: weigh(stretches) for key, stretches in read.code.items()}, weigh(read.fixed))
    passed = ld.link(ld.lifted(image)._replace(searched=[]), gcclink.rooted(images)[image.name],
                     ld.provisional(images, image), out, 'layout')
    text = next(each for each in elf.allocated_sections(passed.elf).values() if each.name == '.text')
    placed = ld.placed(image, passed)
    kernel, warm = placed[:len(image.kernel)], placed[len(image.kernel):]
    aligns = {each: {section.name: max(section.align, 1) for section in found.sections}
              for each, found in objects.items()}
    # the sections in (PATH, INPUT) order, so the table's own order decides
    # nothing: known gives each one's row
    known = sorted(range(len(warm)), key=lambda row: warm[row][:2])
    stretches = [weighed.code.get((image.name, *warm[row][:2]), []) for row in known]
    # a stretch past its row's end counts for nothing, as in staying(): it is other code's by now, and would weigh
    # on the row laid after
    sections = [Section(warm[row][3], aligns[warm[row][0]][warm[row][1]],
                        [(offset, size, heats) for offset, size, heats in each if offset + size <= warm[row][3]])
                for row, each in zip(known, stretches)]
    # the warm rows start at the end of the kernels' last window, or the kernels', and end with their own window
    closing = [window for window in image.windows if window.kernel == ld.WARM]
    start = text.address + (sum(window.lines for window in image.windows if window.kernel != ld.WARM) * ld.ICACHE_LINE
                            if image.windows else max([place + size for _, _, place, size in kernel] + [0]))
    end = start + closing[0].lines * ld.ICACHE_LINE if closing else None
    stays, held, given = staying(image, weighed, passed)
    given += sum(len(each) for each in stretches)
    held += sum(len(section.stretches) for section in sections)
    # where the record has each section, as an address in this link
    recorded = {(row.path, row.input): text.address + row.place for row in image.searched}
    had = [recorded.get(warm[row][:2]) for row in known]
    costs = Costs(sections, stays)
    wider = (f'a wider window in {table} holds more, and one wider by a multiple of {ld.ICACHE_LINES} lines leaves '
             'the code after it on its sets')
    if pack:
        # the pass links without the record, so it lays the rows in the table's order with no byte free among them
        lies, anew = [text.address + warm[row][2] for row in known], []
        past = max([at + section.size for at, section in zip(lies, sections)] + [start]) - (end or RAM_END)
        if past > 0:
            fail(f'{image.name}: its warm rows, laid with no byte free among them, run {past} bytes past the end of '
                 f'their window, {end - text.address:#x} bytes into .text; {wider}')
    else:
        lies, anew = settle(start, sections, costs, had, end)
        lost = [index for index in anew if lies[index] is None]
        if lost:
            fail(*(f"{image.name}: no place under the end of its warm rows' window, {end - text.address:#x} bytes "
                   f"into .text, is free for {warm[known[index]][0]}'s {warm[known[index]][1]}, which takes "
                   f'{sections[index].size} bytes' for index in lost),
                 f'{image.name}: make dc-order PACK=1 lays the rows with no byte free among them, and {wider}')
    stand = costs.of(enumerate(lies))
    gives = len(weighed.fixed) + sum(len(each) for each in weighed.code.values())
    said = [f"{image.name}: {len(warm)} warm rows; the heat gives {gives} stretches, {given} of them of its rows' "
            f"sections or fixed, and the link holds {held} of those; the rows' pairs cost {stand / frames:,.0f} a "
            'frame where they lie, summed over the routes']
    said += differs(image, digest, text.address, kernel)
    gone = sorted({(row.path, row.input) for row in image.searched} - {each[:2] for each in placed})
    said += [f"{image.name}: its record held {each[0]}'s {each[1]}, which no kernel or warm row places now"
             for each in gone]
    said += [f"{image.name}: {warm[known[index]][0]}'s {warm[known[index]][1]} takes a place "
             f'{lies[index] - text.address:#x} bytes into .text, '
             + ('its first' if had[index] is None else f'and fits its own, {had[index] - text.address:#x} bytes in, '
                                                        'no longer') for index in anew]
    if pack:
        moved = sum(now != then for now, then in zip(lies, had))
        said.append(f"{image.name}: its {len(warm)} warm rows lie in the table's order, each after the one before on "
                    'its alignment, ' + ('as a link without a record lays them' if not image.searched else
                                         f'and {moved} of them moved: a change to measure' if moved else
                                         'where each lay already'))
    elif not image.searched:
        said.append(f'{image.name}: it had no record, so each of its {len(warm)} warm rows took a place anew, by no '
                    'search: a change to measure, where a record was lost')
    # the line that says where the rows end goes here, ahead of the next two, once a search has moved the rows or
    # left them
    ending = len(said)
    if image.searched and len(said) > 1:
        # the record as it stood: its kernels and its rows where it had them, from where it had .text start
        base = text.address if image.text is None else image.text
        stood = gcclink.Linked(None, [gcclink.Placed('.text', row.path, row.input, base + row.place, row.size)
                                      for row in image.searched])
        then = {(row.path, row.input): base + row.place for row in image.searched}
        was = Costs(sections, staying(image, weighed, stood)[0]).of(
            (index, then[warm[row][:2]]) for index, row in enumerate(known) if warm[row][:2] in then)
        said.append(f"{image.name}: the rows' pairs cost {was / frames:,.0f} a frame as its record had them and the "
                    'kernels, by the heat as it is now')
    if whole:
        line = ld.ICACHE_LINE
        slots = [section._replace(size=-(-section.size // line) * line + SPARE, align=max(section.align, line))
                 for section in sections]
        stood = sorted(range(len(sections)), key=lambda index: (lies[index], sections[index].size))
        found, _, least = search(start, slots, stays, stood)
        sought = dict(packed(start, slots, found))
        past = max([sought[index] + section.size for index, section in enumerate(sections)] + [start]) - (
            end or RAM_END)
        if past > 0:
            said.append(f'{image.name}: a search of the whole block lays the rows {past} bytes past the end of their '
                        'window, so none of it is written: the tables are as a run without the search writes them')
        elif least < stand:
            said.append(f"{image.name}: a search of the whole block lays the rows where their pairs cost "
                        f'{least / frames:,.0f} a frame, and moves '
                        f'{sum(sought[index] != lies[index] for index in sought)} of them')
            lies = [sought[index] for index in range(len(sections))]
        else:
            said.append(f'{image.name}: a search of the whole block finds no places that cost less, and moves no row')
    if image.searched and end is None:
        # where the record had its rows end, by the place and size it gave each, and where the warm rows end now.
        # The record's kernel rows lie under its warm rows, so its end is a warm row's too. The image's other code
        # follows the end, each section on its alignment, so the line gives no bytes that code moves
        ended = text.address + max(row.place + row.size for row in image.searched)
        ends = max(at + section.size for at, section in zip(lies, sections))
        if ends != ended:
            said.insert(ending, f'{image.name}: its warm rows end {ends - text.address:#x} bytes into .text, and '
                        f'its record had them end {ended - text.address:#x} bytes in, so their end lies '
                        f'{abs(ends - ended)} bytes ' + ('on' if ends > ended else 'back')
                        + ', and the code after them follows it')
    # the rows in their places' order, a section of no bytes ahead of one
    # that shares its place
    order = sorted(range(len(sections)), key=lambda index: (lies[index], sections[index].size))
    searched = list(kernel) + [(*warm[known[index]][:2], lies[index] - text.address, sections[index].size)
                               for index in order]
    # each warm row's line takes the row the order gives its place
    rows_now = [lines[index] for index in at]
    for index, row in zip(at, (known[index] for index in order)):
        lines[index] = rows_now[row]
    publish(table, ''.join(lines))
    publish(CONFIG / image.name / 'order.txt', record(digest, text.address, searched))
    return said


def main(argv):
    parser = argparse.ArgumentParser(prog='order', description=__doc__.split('\n\n')[0])
    parser.add_argument('spec', metavar='SPEC', help='the images, their inputs and their rows')
    parser.add_argument('out', metavar='DIR', type=Path, help='the directory the layout passes go under, DIR/IMAGE/')
    how = parser.add_mutually_exclusive_group()
    how.add_argument('--search', action='store_true',
                     help='search the whole block too, and write what that finds where it costs less')
    how.add_argument('--pack', action='store_true',
                     help="lay the rows in the table's order with no byte free among them, which frees the most room")
    args = parser.parse_args(argv)
    images = gcclink.read(args.spec)
    if not any(image.warm or image.searched for image in images.values()):
        fail(f'{args.spec}: no image has a warm row')
    for image in images.values():
        if image.warm:
            print('\n'.join(reorder(args.spec, images, image, args.out, args.search, args.pack)))
        elif image.searched:
            # a record is of the warm rows' places, and a link holds an
            # image to the one it has
            remove([CONFIG / image.name / 'order.txt'])
            print(f'{image.name}: no warm row, so its record goes')


if __name__ == '__main__':
    stoppable()
    main(sys.argv[1:])
