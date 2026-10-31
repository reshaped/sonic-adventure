"""Read and check one image's tables, config/IMAGE/splits.txt, symbols.txt and
data.txt, as Config, and 1ST_READ.BIN's imports, config/1st_read/imports.txt.

TOOLCHAIN.md#image-tables gives the rows of splits.txt, symbols.txt and
imports.txt and their rules, and TOOLCHAIN.md#generated-ranges data.txt's.
Config holds an image's tables to what images.py fixes. A malformed row
fails, naming its file and line. imports() reads the imports, and
handoffs() the reason each hand-off among them gives. order() gives an
image's sources in link order, and gcc_object() names the object make gcc
compiles for a placement.
"""

from pathlib import Path

from images import (ALONE, CONFIG, EVENT_CONVERSIONS, EVENT_RUNTIME, EXTENTS, FIRST, FIRST_BASE, FIRST_CODE_SECTIONS,
                    FIRST_EMPTY, FIRST_FILE_END, FIRST_GAME_CODE, FIRST_SDK_CODE, FIRST_TAILS, LOADER, LOADER_GAME_CODE,
                    LOADER_SDK_CODE, ZEROED, images)
from tables import (BYTE, DEFINITION, GENERATED, NAME, SECTION, SOURCE, VOUCHED, address, aligned, boundaries, hexa,
                    interval, named, rows, spell)
from tool import fail

# the definitions 1ST_READ.BIN takes from the images that load beside it
IMPORTS = FIRST / 'imports.txt'
# the word that marks a hand-off row. In IMPORTS the row's reason follows
# it, and in config/imports.txt the row's reason opens with it. In a
# hand-off, the image that takes the name reads its provider's file and then
# starts it, so the two never run together.
HANDOFF = 'hand-off:'
# the most SHC aligns an item of an object's constants, data or bss to: R10
# starts every section on 4, and R32 aligns those items to at most 4
# (TOOLCHAIN.md#alignment)
ITEM_ALIGNMENT = 4


def imports():
    """config/1st_read/imports.txt's rows, as a list of (name, image,
    symbol, address, where) in table order. name is the name 1ST_READ.BIN's
    sources use for a definition in image, an image that loads beside it.
    symbol is image's own name for the definition. address is the address
    image's symbols.txt gives symbol, and where gives the row's path:line.
    A tree without the table imports nothing.

    A row is NAME IMAGE [SYMBOL] [hand-off: REASON], where NAME and SYMBOL
    are C names. A row without SYMBOL takes NAME as its SYMBOL. handoffs()
    gives each REASON.

    A row fails where it:
    - isn't of that form, or ends at hand-off:, with no reason;
    - names no image, a directory in config/, or names 1ST_READ.BIN;
    - gives a NAME that has a row already, here or in 1ST_READ.BIN's
      symbols.txt;
    - gives a SYMBOL that the image's symbols.txt gives no address.
    """
    known = set(images())
    if not IMPORTS.exists():
        return []
    own = {name for _, name in named(FIRST / 'symbols.txt')}
    form = f'not a row: NAME IMAGE [SYMBOL] [{HANDOFF} REASON]'
    tables, found, seen = {}, [], {}
    for where, row in rows(IMPORTS):
        given = row[:row.index(HANDOFF)] if HANDOFF in row else row
        if len(given) not in (2, 3) or row[-1] == HANDOFF:
            fail(f'{where}: {form}')
        name, image, symbol = given if len(given) == 3 else (*given, given[0])
        if not NAME.fullmatch(name) or not NAME.fullmatch(symbol):
            fail(f'{where}: {form}')
        if image not in known:
            fail(f'{where}: {image}: no such image, a directory in {CONFIG}/')
        if CONFIG / image == FIRST:
            fail(f'{where}: {name}: an import from 1ST_READ.BIN itself')
        if name in seen:
            fail(f'{where}: {name} has a row already, at {seen[name]}')
        if name in own:
            fail(f'{where}: {name} has a row in {FIRST / "symbols.txt"} already')
        seen[name] = where
        if image not in tables:
            tables[image] = {label: address for address, label in named(CONFIG / image / 'symbols.txt')}
        if symbol not in tables[image]:
            fail(f'{where}: {CONFIG / image / "symbols.txt"} gives {symbol} no address')
        found.append((name, image, symbol, tables[image][symbol], where))
    return found


def handoffs():
    """The reason each hand-off row of config/1st_read/imports.txt gives
    after hand-off:, by the row's name. A tree without the table has none.
    It checks no row: a caller holds the rows to their form with imports(),
    which fails a malformed one."""
    if not IMPORTS.exists():
        return {}
    return {row[0]: ' '.join(row[row.index(HANDOFF) + 1:]) for _, row in rows(IMPORTS) if HANDOFF in row}


class Config:
    """One image's sections, placements, fill byte and names, checked as they're read."""

    def __init__(self, name):
        self.path = CONFIG / name
        self.name = self.path.name
        self.splits = self.path / 'splits.txt'
        self.sections, self.placements, fills, at, rowed = [], [], [], {}, {}
        # the boundary declarations of each gen/ range that has any, by
        # source, as (its row's path:line, boundaries()'s map)
        self.declared = {}
        for where, row in rows(self.splits):
            if len(row) == 2 and row[0] == 'fill':
                if not BYTE.fullmatch(row[1]):
                    fail(f'{where}: {row[1]}: not a byte, 0x and two lowercase hex digits')
                fills.append((where, int(row[1], 16)))
            elif len(row) == 3 and SECTION.fullmatch(row[0]):
                if self.path == FIRST:
                    fail(f"{where}: a section row, and tools/images.py fixes 1ST_READ.BIN's sections")
                if row[0] in rowed:
                    fail(f'{where}: a second row for section {row[0]}, after {rowed[row[0]]}')
                rowed[row[0]] = where
                self.sections.append((row[0], *interval(*row[1:], where)))
            elif (len(row) >= 4 and SOURCE.fullmatch(row[0]) and SECTION.fullmatch(row[1])
                  and (declared := boundaries(row[4:])) is not None):
                placement = (row[0], row[1], *interval(*row[2:4], where))
                if placement in at:
                    fail(f'{where}: repeats the placement at {at[placement]}')
                self.placements.append(placement)
                at[placement] = where
                if declared:
                    if not GENERATED.fullmatch(row[0]):
                        fail(f'{where}: {row[0]} declares {" and ".join(map(spell, declared.items()))}, '
                             'and only a gen/ range declares its boundaries')
                    if declared.get('continues', 0) > ITEM_ALIGNMENT:
                        fail(f'{where}: {row[0]} declares continues {declared["continues"]}, and SHC aligns no item '
                             f"of an object's constants, data or bss past {ITEM_ALIGNMENT}")
                    self.declared[row[0]] = where, declared
            else:
                fail(f'{where}: not a row: SECTION START END, SOURCE SECTION START END [BOUNDARY...], or fill BYTE')
        if len(fills) > 1:
            fail(f'{fills[1][0]}: a second fill row, after {fills[0][0]}')
        self.fill = fills[0][1] if fills else 0
        self.check(at)

    def check(self, at):
        if not self.sections and self.path != FIRST:
            fail(f'{self.splits}: no section rows')
        game, sdk = self.code()
        ordered = sorted(self.sections, key=lambda row: row[1])
        for (name, _, end), (later, start, _) in zip(ordered, ordered[1:]):
            if start < end:
                fail(f'{self.splits}: section {later} starts at {hexa(start)}, inside {name}')
        stems, opened = {}, {}
        for placement in self.placements:
            source, name, start, end = placement
            where = at[placement]
            # link.py starts the object's section at the source's first row in it
            first = opened.setdefault((source, name), placement)
            if start < first[2]:
                fail(f'{where}: {source} starts below its first row in {name}, at {at[first]}')
            # only 1ST_READ.BIN has no section rows: its bss placements lie past
            # its file, and the rest inside it
            if self.sections:
                if not any(section == name and low <= start and end <= high for section, low, high in self.sections):
                    fail(f'{where}: {source} lies outside the section {name}')
            elif not (FIRST_FILE_END <= start if name in ZEROED else (FIRST_BASE <= start and end <= FIRST_FILE_END)):
                fail(f'{where}: {source} lies outside 1ST_READ.BIN\'s {"bss" if name in ZEROED else "file"}')
            generated = GENERATED.fullmatch(source)
            if source.startswith('gen/'):
                if not generated or generated[1] != self.name or int(generated[2], 16) != start:
                    fail(f'{where}: {source}: a gen/ range is gen/{self.name}/d_{start:08x}.c')
                if any(start < high and low < end for low, high in game + sdk):
                    fail(f'{where}: {source} lies in code, and a gen/ range is data from the disc')
            elif not Path(source).is_file():
                fail(f'{where}: {source}: no such file')
            stem = Path(source).with_suffix('')
            if stems.setdefault(stem, source) != source:
                fail(f'{where}: {source} and {stems[stem]} would build one object')
        owned = sorted(self.placements, key=lambda row: row[2])
        for first, second in zip(owned, owned[1:]):
            if second[2] < first[3]:
                fail(f'{at[second]}: {second[0]} starts inside {first[0]}, at {at[first]}')
        # the row of a section the extents give starts at its region()'s start
        # and spans the region. Past the region, it may run on only to the next
        # section's start. Where the image has section rows, each such section
        # that holds bytes has its own row. A counterpart can lie in its
        # section's row instead, as D32 can in D's. A placement in such a
        # section lies inside the section's bounds().
        bounds = self.bounds()
        starts = {low for _, low, _ in self.sections}
        for name, low, high in self.sections:
            if name not in bounds:
                continue
            bottom, top = self.region(name)
            if low != bottom or high < top or (high > top and high not in starts):
                fail(f'{self.splits}: section {name} runs {hexa(low)} to {hexa(high)}, '
                     f'and tools/images.py gives {hexa(bottom)} to {hexa(top)}')
        spanned = {name for row, _, _ in self.sections for name in (row, f'{row}32')}
        for name, (low, high) in bounds.items():
            if self.sections and low < high and name not in spanned:
                fail(f'{self.splits}: section {name} has no row, and tools/images.py gives {hexa(low)} to {hexa(high)}')
        for placement in self.placements:
            source, name, start, end = placement
            if name not in bounds:
                continue
            low, high = bounds[name]
            if not (low <= start and end <= high):
                fail(f'{at[placement]}: {source} lies outside {name}, {hexa(low)} to {hexa(high)}')

    def extents(self):
        """The sections the image's section-boundary tables bound, as (name,
        start, end) in link order: EXTENTS's entry for the image, or () for an
        image EXTENTS doesn't name. D's or DSG's end is None where D32 or DSG32
        follows it."""
        return EXTENTS.get(self.path, ())

    def bounds(self):
        """The (start, end) of each section the extents give, by name. Each is
        the section's extent, but where the extents end D or DSG with None, it
        runs to the start of D32 or DSG32."""
        found = {name: (start, end) for name, start, end in self.extents()}
        return {name: (start, found[f'{name}32'][0] if end is None else end) for name, (start, end) in found.items()}

    def region(self, name):
        """The (start, end) that the row of section name must span, for a
        section the extents give. It runs from the section's start to the end
        of its counterpart, name with 32 after it, such as D32 for D. Where the
        extents give no counterpart, or the image has a row for it, it runs to
        the section's own end."""
        bounds, tabled = self.bounds(), {section for section, _, _ in self.sections}
        counterpart = f'{name}32'
        alone = counterpart not in bounds or counterpart in tabled
        return bounds[name][0], bounds[name if alone else counterpart][1]

    def layout(self):
        """The image's sections as (name, start, end), in address order.

        For an image with section rows, these are its rows. 1ST_READ.BIN has
        none, so its sections are those the extents give that hold bytes, P
        and PSG as FIRST_CODE_SECTIONS gives them, and C and CSG. C and CSG
        start where P and PSG end, and each runs to the start of the next
        section that holds bytes."""
        if self.path != FIRST:
            return sorted(self.sections, key=lambda row: row[1])
        found = [(name, low, high) for name, (low, high) in self.bounds().items() if low < high]
        for name, low, high, later in FIRST_CODE_SECTIONS:
            after = min(start for _, start, _ in found if start >= high)
            found += [(name, low, high), (later, high, after)]
        return sorted(found, key=lambda row: row[1])

    def zeroed(self, address):
        """Whether address lies in the image's bss, the sections ZEROED
        names, as layout() gives them."""
        return any(name in ZEROED and low <= address < high for name, low, high in self.layout())

    def span(self):
        """The image's (start, end), from its first section's start to its
        last's end, as layout() gives them."""
        layout = self.layout()
        return min(start for _, start, _ in layout), max(end for _, _, end in layout)

    @property
    def base(self):
        """The linked address of the image's first byte."""
        return self.span()[0]

    def code(self):
        """The image's game code and SDK code, as two lists of (start, end).

        1ST_READ.BIN's and LOADER.BIN's are fixed in images.py. In another
        image, a section whose name starts with P is code: SDK code where the
        name starts with PSG, and game code otherwise. An event's runtime() is
        SDK code too, so the game code of the P that holds the runtime ends
        where the runtime starts."""
        if self.path == FIRST:
            return FIRST_GAME_CODE, FIRST_SDK_CODE
        if self.path == LOADER:
            return LOADER_GAME_CODE, LOADER_SDK_CODE
        code = [(name, start, end) for name, start, end in self.sections if name.startswith('P')]
        game = [(start, end) for name, start, end in code if not name.startswith('PSG')]
        sdk = [(start, end) for name, start, end in code if name.startswith('PSG')]
        runtime = self.runtime()
        if runtime:
            game = [(start, runtime[0]) if start <= runtime[0] < end else (start, end) for start, end in game]
            sdk.append(runtime)
        return game, sdk

    def linker_fill(self, section):
        """The gaps the tables show the linker leaves after objects and between
        sections, as (name, start, end) in address order. name is the section
        that holds the gap, or None for a gap between two section rows.

        section is link.section: section(source, name) gives the (size,
        alignment, code) that source's object declares for section name. Only
        the alignment is read.

        Inside a section, a gap runs from the end of a source's row to the next
        row. The gap is the linker's fill where the next row is another
        source's first in the section, and no source has rows in the section
        both before and after the gap. Such a source's object's section would
        run across it. The alignment of the later source's object must also
        round the end up to that row's start.

        A gen/ range's bytes give neither its object's end nor its alignment.
        Its boundary declarations stand in for them: end for the end of its
        object, and start N for the start of an object on N. No other gap runs
        from a gen/ range, or to one inside a section. TOOLCHAIN.md#fill says
        why.

        An end fits a section's start where alignments() gives the section at
        least one alignment, and each rounds the end up to that start. From a
        section's last placement, a gap runs to the next section's start where
        the placement's end fits that start. No such gap runs from a gen/
        range, even one that declares end. The gap's part inside the section's
        row comes back as the section's. Its part past the row comes back as a
        gap between rows, with name None. 1ST_READ.BIN has no section rows.
        FIRST_CODE_SECTIONS gives its code sections and the sections after
        them, and FIRST_TAILS gives its other sections whose last placement a
        gap runs from.

        The section rows and the fill between them tile the image, so a gap
        between two rows must be the linker's fill. Where a row ends short of
        the next, the row's end must fit the next section's start. Otherwise
        the extents must give the next section that start.

        A gap inside a section that these rules don't justify is left out. The
        splicer then fails its stored bytes as unwritten.

        A gap between rows that these rules don't justify fails, naming
        splits.txt. A boundary declaration and a FIRST_TAILS entry fail as
        gaps() gives.
        """
        return self.gaps(section)[0]

    def padding(self, section):
        """The padding before each gen/ range that declares continues N, inside
        the object the range continues, as (name, start, end) in address
        order. name is the section that holds the padding. section is as
        linker_fill() takes it.

        Padding runs from the end of a source's last row in a section to the
        next row, a gen/ range that declares continues N. The range continues
        that source's object, and starts at that end rounded up to N.

        It fails as gaps() gives."""
        return self.gaps(section)[1]

    def gaps(self, section):
        """linker_fill()'s gaps and padding()'s, as two lists. section is as
        linker_fill() takes it.

        No gap or padding inside a section has one source's rows in the section
        both before and after it. That source's object's section, from its
        first row to its last, would run across it.

        Between two adjacent rows of a section where either is a gen/ range,
        only boundary declarations bound a gap. Such a gap runs from a source's
        last row in the section, or from a gen/ range that declares end. It
        runs to the next row, which is one of:
        - a source's first row in the section, on its object's alignment;
        - a gen/ range that declares start N or continues N, on N.
        A range that declares continues N continues the object of the source
        before it, so no gap runs to one from a gen/ range.

        A gap between section rows fails as linker_fill() gives. In
        1ST_READ.BIN, a FIRST_TAILS entry fails, naming tools/images.py, where
        FIRST_EXTENTS doesn't give the section after it. A boundary
        declaration fails, naming its row, where:
        - the rows it bounds meet, with no gap between them;
        - the later row's start isn't the earlier row's end rounded up to the
          alignment above;
        - no gap uses it;
        - it is start N, alignments() gives the row's section at least one
          alignment, and N isn't one of them.
        """
        owned = sorted(self.placements, key=lambda row: row[2])
        # each source's first start and last end in each section
        first, final = {}, {}
        for source, name, start, end in owned:
            first.setdefault((source, name), start)
            final[(source, name)] = end
        # a range that declares start N opens an object on N. The call gives
        # alignments() the range's start in place of the section's. No source's
        # row starts there, since rows don't overlap, so alignments()'s first
        # case never applies.
        for source, label, start, _ in owned:
            where, declared = self.declared.get(source, (None, {}))
            aligns = self.alignments(label, start, section) if 'start' in declared else ()
            if aligns and declared['start'] not in aligns:
                starts = ' or '.join(map(str, sorted(aligns)))
                if any(row[1] == label and not GENERATED.fullmatch(row[0]) for row in owned):
                    fail(f"{where}: {source} declares start {declared['start']}, and the image's objects in {label} "
                         f'start on {starts}')
                fail(f"{where}: {source} declares start {declared['start']}, and with no source outside gen/ placed in "
                     f'{label}, an object there starts on {starts}')
        gaps, padding, used = [], [], set()
        # in each section, the furthest end of an object's section the walk has entered
        reach = {}
        for (source, name, _, end), (later, label, start, _) in zip(owned, owned[1:]):
            reach[name] = max(reach.get(name, 0), final[(source, name)])
            if label != name or (later, label) == (source, name) or first[(later, label)] != start or reach[name] > end:
                continue
            if not GENERATED.fullmatch(source) and not GENERATED.fullmatch(later):
                if end < start == aligned(end, section(later, label)[1]):
                    gaps.append((name, end, start))
                continue
            before = self.declared.get(source, (None, {}))[1]
            after = self.declared.get(later, (None, {}))[1]
            kind = next((kind for kind in ('start', 'continues') if kind in after), None)
            if GENERATED.fullmatch(later) and kind is None:
                continue
            if GENERATED.fullmatch(source) and ('end' not in before or kind == 'continues'):
                continue
            alignment = after[kind] if kind else section(later, label)[1]
            bounding = [(source, 'end')] * bool(GENERATED.fullmatch(source)) + [(later, kind)] * bool(kind)
            where = ' and '.join(self.declared[each][0] for each, _ in bounding)
            if end == start:
                fail(f'{where}: {source} ends where {later} starts, at {hexa(end)}, and no gap lies between them')
            if aligned(end, alignment) != start:
                fail(f'{where}: {source} ends at {hexa(end)}, which rounds up to {hexa(aligned(end, alignment))} '
                     f'on {alignment}, and {later} starts at {hexa(start)}')
            (padding if kind == 'continues' else gaps).append((name, end, start))
            used.update(bounding)
        for source, (where, declared) in self.declared.items():
            for declaration in declared.items():
                if (source, declaration[0]) not in used:
                    fail(f'{where}: {source} declares {spell(declaration)}, and no gap uses it')
        bounds = self.bounds()
        if self.path == FIRST:
            ends = [(name, top, later, top) for name, _, top, later in FIRST_CODE_SECTIONS]
            for name, later in FIRST_TAILS:
                if later not in bounds:
                    fail(f"tools/images.py: FIRST_TAILS runs the fill from {name}'s last placement to {later}'s start, "
                         f'and FIRST_EXTENTS gives {later} none')
                ends.append((name, bounds[later][0], later, bounds[later][0]))
        else:
            tiles = sorted(self.sections, key=lambda row: row[1])
            ends = [(name, top, later, start) for (name, _, top), (later, start, _) in zip(tiles, tiles[1:])]

        def fits(end, later, start):
            aligns = self.alignments(later, start, section)
            return bool(aligns) and all(aligned(end, align) == start for align in aligns)

        for name, top, later, start in ends:
            if top < start:
                if not fits(top, later, start) and bounds.get(later, (None, None))[0] != start:
                    fail(f'{self.splits}: no row holds {hexa(top)} to {hexa(start)}, between {name} and {later}, '
                         f'and no alignment {later} can start on puts its start there')
                gaps.append((None, top, start))
            last = max((row for row in owned if row[1] == name), key=lambda row: row[3], default=None)
            if last and not GENERATED.fullmatch(last[0]) and last[3] < top and fits(last[3], later, start):
                gaps.append((name, last[3], top))
        return sorted(gaps, key=lambda gap: gap[1]), sorted(padding, key=lambda gap: gap[1])

    def alignments(self, name, at, section):
        """The set of alignments section name can start on, where it starts at
        address at. section is as linker_fill() takes it.

        The linker starts a section on its first object's alignment, which no
        gen/ range's bytes show. The set holds the first of these that
        applies, gen/ ranges aside:
        - the alignment of a source's object, where that source's first row in
          the section starts at at;
        - else the alignment of the object of each source placed in the
          section, each standing in for the first object;
        - else, in 1ST_READ.BIN, that of the object FIRST_EMPTY names for the
          empty section;
        - else none, for a code section, one whose name starts with P;
        - else 4 and 8, on which R10 and R32 start constants, data or bss.

        Standing in assumes each placed object's alignment gives the section
        the start the first object's would, which can fail (TOOLCHAIN.md#fill).
        """
        own = {}
        for source, label, start, _ in self.placements:
            if label == name and not GENERATED.fullmatch(source):
                own[source] = min(start, own.get(source, start))
        sources = [source for source, start in own.items() if start == at] or list(own)
        if not sources and self.path == FIRST and name in FIRST_EMPTY:
            sources = [FIRST_EMPTY[name]]
        found = {section(source, name)[1] for source in sources}
        return found or (set() if name.startswith('P') else {4, 8})

    def runtime(self):
        """The runtime an event links after its own code, as (start, end): from
        the run's first name to the end of the P that holds it. None for an
        image whose symbols.txt gives neither EVENT_RUNTIME's first name nor
        EVENT_CONVERSIONS's.

        The run is EVENT_RUNTIME where symbols.txt gives its first name, and
        EVENT_CONVERSIONS otherwise. Every name of the run must be in
        symbols.txt, at strictly increasing addresses, inside that P. Other
        names may share their addresses or lie between them. A run that
        breaks this fails, naming symbols.txt."""
        own = dict((name, address) for address, name in named(self.path / 'symbols.txt'))
        group = next((group for group in (EVENT_RUNTIME, EVENT_CONVERSIONS) if group[0] in own), None)
        if group is None:
            return None
        start = own[group[0]]
        end = next((high for name, low, high in self.sections if name == 'P' and low <= start < high), None)
        members = [own.get(name) for name in group]
        if end is None or None in members or members != sorted(set(members)) or members[-1] >= end:
            fail(f'{self.path / "symbols.txt"}: no run of {", ".join(group)} in that order, '
                 f'from {group[0]} to the end of its P')
        return start, end

    def generated(self):
        """Each gen/ row's start and end."""
        return [(start, end) for source, _, start, end in self.placements if source.startswith('gen/')]

    def definitions(self):
        """The definitions of the image's data.txt, in address order, as
        (start, kind, count, vouched, where): kind is a kind or a C type,
        count an array's elements or None for one record, vouched whether the
        row vouches for its words in RAM, and where the row's path:line. A
        row is ADDRESS DEFINITION, and vouched after it where it vouches
        (TOOLCHAIN.md#generated-ranges). An image without data.txt has none.

        Fails, naming the row, where it is malformed, lies in no gen/ range,
        or starts at or below the row before it."""
        path, found, last = self.path / 'data.txt', [], -1
        if not path.exists():
            return found
        ranges = self.generated()
        for where, row in rows(path):
            vouched = len(row) > 2 and row[-1] == VOUCHED
            defined = DEFINITION.fullmatch(' '.join(row[1:len(row) - vouched]))
            if len(row) < 2 or not defined:
                fail(f'{where}: not a row: ADDRESS KIND, or KIND[COUNT] for an array, then {VOUCHED} where it '
                     'vouches')
            start = address(row[0], where)
            if not any(low <= start < high for low, high in ranges):
                fail(f'{where}: {hexa(start)} lies in no gen/ range of {self.splits}')
            if start <= last:
                fail(f'{where}: {hexa(start)} lies at or below {hexa(last)}, the row before it')
            last = start
            kind, count = defined.groups()
            found.append((start, kind.strip(), count and int(count), vouched, where))
        return found

    def ranges(self, source):
        """Each (section, start, end) the image places source at, in table order."""
        return [(section, start, end) for name, section, start, end in self.placements if name == source]

    def sources(self):
        """Every source the image places, in table order."""
        return list(dict.fromkeys(source for source, *_ in self.placements))

    def held(self):
        """Each source's ranges that its linked source holds, by source, as
        lists of (section, start, end) in table order.

        That is every range but 1ST_READ.BIN's bss, which the file doesn't
        store. A source with only those maps to an empty list.
        """
        held = {}
        for source, name, start, end in self.placements:
            ranges = held.setdefault(source, [])
            if self.path != FIRST or name not in ZEROED:
                ranges.append((name, start, end))
        return held

    def symbols(self):
        """The (address, name) pairs the image's code or data can use. They
        start with the rows of the image's own symbols.txt. For 1ST_READ.BIN,
        its imports() follow, at the addresses their images give them. For an
        image other than 1ST_READ.BIN and those in ALONE, 1ST_READ.BIN's rows
        follow. A row of 1ST_READ.BIN's is left out where it lies inside the
        image's span(), or where the image's own rows give its name."""
        own = named(self.path / 'symbols.txt')
        if self.path == FIRST:
            return own + [(address, name) for name, _, _, address, _ in imports()]
        if self.path in ALONE:
            return own
        low, high = self.span()
        given = {name for _, name in own}
        return own + [(address, name) for address, name in named(FIRST / 'symbols.txt')
                      if not low <= address < high and name not in given]


def configs():
    """Every image's Config, in name order."""
    return [Config(name) for name in images()]


def order(config):
    """The image's sources in the order tools/relink.py gives R32's lnk
    their objects: in each section, in the order its rows place them, and
    otherwise in table order. tools/spec.py gives GNU ld the GCC objects in
    the same order. The gen/ ranges share one GCC object, which stands where
    the first range does. Where the sections' orders form a cycle, fail,
    naming each step of it and the section that takes it."""
    rank = {source: index for index, source in enumerate(config.sources())}
    # for each source, the sources whose objects go before it, each with a
    # section that puts it there
    before, last = {source: {} for source in rank}, {}
    for source, name, _, _ in sorted(config.placements, key=lambda row: row[2]):
        if last.get(name, source) != source:
            before[source].setdefault(last[name], name)
        last[name] = source
    found, done = [], set()
    while len(found) < len(rank):
        ready = [source for source in rank if source not in done and before[source].keys() <= done]
        if not ready:
            # follow the waiting sources back until one repeats
            chain = [next(source for source in rank if source not in done)]
            while chain.count(chain[-1]) < 2:
                chain.append(next(earlier for earlier in before[chain[-1]] if earlier not in done))
            cycle = chain[chain.index(chain[-1]):]
            fail(f'{config.splits}: no one order of the objects keeps each section in its rows\' order: '
                 + ', '.join(f'{earlier} goes before {later} in {before[later][earlier]}'
                             for later, earlier in zip(cycle, cycle[1:])))
        source = min(ready, key=rank.get)
        found.append(source)
        done.add(source)
    return found


def gcc_object(source, image):
    """The object make gcc compiles for source, one of image's placements,
    as a Path. A gen/ range's is the object of image's content,
    build/gcc/gen/IMAGE/content.o. Any other source's lies at the source's
    path under build/gcc/, with .o for its suffix."""
    if source.startswith('gen/'):
        return Path('build/gcc/gen', image, 'content.o')
    return Path('build/gcc', source).with_suffix('.o')
