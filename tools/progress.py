"""Report what writes the images' code, and write README.md's numbers: how
much of that code is C and how much is assembly.

Without --write or --check, progress prints a report: the bytes of all
images' code together, by what writes them. Each source's bytes count under its route, and
the linker's fill under fill. A source that several images place counts in
each. --write rewrites README.md's numbers, the lines between its
<!-- progress --> and <!-- /progress --> lines. --check fails where those
lines differ from what --write would write. Every run needs --patched,
--grafted and --inline: the Makefile's PATCHED_C, GRAFTED_C and INLINE_C,
which route the C sources. make progress runs the report and then --write,
and make check runs --check. TOOLCHAIN.md#c-and-assembly defines the
numbers.

The report reads config/'s tables and the alignments each placed source's
object declares. --write and --check also read each placed .src file and
each placed source in INLINE_C, for its @asm tags and the lines of its
functions, bodies and blocks, and the listing asmsh writes beside its
object, to tell C from assembly. make check builds those objects and
listings without the disc, and progress reads nothing of the disc's.

Every run fails, naming each range, where an image's placements and fill
leave a byte of its code unwritten. The numbers' headline, 100%, rests on
that check. The numbers also fail where a listing doesn't give an inline
body's, a #pragma asm block's or a .src function's bytes.
"""

import argparse
import fnmatch
import re
import sys
from collections import Counter
from pathlib import Path

import csource
import tags
from code_ranges import CATEGORIES, overlap, written
from config import Config
from images import binaries
from link import section
from shc_object import section_table
from tool import content, fail, save

# the number of binaries config/ describes: a repeated event file shares its event's image
PROGRAMS = 247
README = Path('README.md')
OPENING, CLOSING = '<!-- progress -->', '<!-- /progress -->'
# README.md's table rows, in order, each with fnmatch patterns of the config/ names of the images whose game code it
# counts. An image that no row's patterns match, or that more than one row's match, fails. The SDK libraries' row has no
# patterns: it counts every image's SDK code instead
SDK = 'SDK libraries'
AREAS = (('Main executable', ('1st_read',)),
         (SDK, None),
         ('Stages', ('stg*', 'minicart', 'shooting', 'sboard')),
         ('Bosses', ('b_*',)),
         ('Fields and Chao', ('adv0*', 'al_*', 'tikal_prog')),
         ('Events', ('ev*',)),
         ('Menus and system', ('advertise', 'summary', 'loader', 'movie')))
# the routes of C sources, whose inline assembly numbers() takes out of C
C = ('c', 'shckit', 'grafted', 'inline')

# asmsh's listing. A row gives a row number, an offset, code and the number of a line of the text asmsh read. From
# column TEXT, it gives that line, cut at column WIDTH. A row with no line number is one of two kinds. A pool's row has
# asmsh's own text: the pool's bounds, the branch past it and its slot, its alignment, or a word with the lines that
# load it. A row of an offset and code alone continues the row above. Page headers, totals and the lists after the
# rows are not rows
TEXT, WIDTH = 42, 132
HEADER = re.compile(r'\f?\*\*\* SuperH RISC engine ASSEMBLER|PROGRAM NAME =|  \*{5}TOTAL (?:ERRORS|WARNINGS) ')
LISTS = re.compile(r'\f?\*\*\* (?:CROSS REFERENCE|SECTION DATA) LIST')
NUMBER = re.compile(r' *\d*')
OFFSET = re.compile(r'[0-9A-F]{8}| {8}')
CODE = re.compile(r'[0-9A-F]* *')
BEGIN, END = '***** BEGIN-POOL *****', '***** END-POOL *****'
POOLED = re.compile(r'DATA FOR SOURCE-LINE (\d+(?:,\d+)*)')
AROUND = ('BRA TO END-POOL', 'NOP', 'ALIGNMENT CODE')
# DIRECTIVE matches a directive's start, after any label. .SECTION starts a section. .RES reserves space, and its
# row's code gives the space's size. .DATAB repeats a value. Its .B, .W or .L suffix gives the unit, which is a byte
# without one. Its first operand gives the count, in decimal. .ALIGN skips bytes, and its row's offset is past them
DIRECTIVE = r'\s*(?:[\w?]+:)?\s*\.'
STARTS = re.compile(DIRECTIVE + r'SECTION\s+(\w+)', re.I)
RESERVES = re.compile(DIRECTIVE + r'RES(?:\.[BWL])?\s', re.I)
REPEATS = re.compile(DIRECTIVE + r'DATAB(?:\.([BWL]))?\s+([^,]+?)\s*,', re.I)
ALIGNS = re.compile(DIRECTIVE + r'ALIGN\s', re.I)
UNITS = {'B': 1, 'W': 2, 'L': 4}

# measured()'s runs for each source, by the source, so that a listing is read once however many images and regions
# count it
MEASURED = {}


def survey():
    """Every binary's Config, in name order. Fails unless config/ describes
    PROGRAMS binaries. A repeated event file has no image of its own here."""
    images = [Config(name) for name in binaries()]
    if len(images) != PROGRAMS:
        fail(f'config/ has {len(images)} binaries, and progress expects {PROGRAMS}')
    return images


def report(images, routes):
    """The report's text: all images' code together, in bytes by what writes
    it. Each source's bytes count under its route, which code_ranges.route()
    gives from routes, (patched, grafted, inline). Where any shckit or
    grafted bytes are of sources with inline assembly, a line after grafted's
    counts them a second time. Data from the disc lies in no code, so
    generated takes no row. Fails as written() does."""
    counts, mixed = dict.fromkeys(CATEGORIES, 0), 0
    for config in images:
        each, inline = written(config, section, *routes)
        counts = {category: counts[category] + each[category] for category in CATEGORIES}
        mixed += inline
    lines = [f'Code by what writes it, {sum(counts.values()):,} bytes in all; '
             'a source several binaries place counts in each:']
    for category, what in CATEGORIES.items():
        if category == 'generated':
            continue
        lines.append(f'  {category:<9}  {counts[category]:>9,}  {what}')
        if category == 'grafted' and mixed:
            lines.append(f'{"":<24}{mixed:,} of the shckit and grafted code bytes are of sources with inline assembly')
    return '\n'.join(lines) + '\n'


def listing(source):
    """The listing asmsh wrote beside source's object, and the object's
    sections, as (path, sections), sections as shc_object.section_table()
    gives them. The object is under build/ or build/stub/. make check builds a
    stub under build/stub/ in place of a source that imports data. Where
    both stand, it is the newer, as link.section takes the newer of two
    that differ. Of two that agree with neither newer, it is the one under
    build/. The listing is the object's own, so its rows and the sections
    are of one build. Fails where neither object stands, where two that
    differ are neither newer, and where the object has no listing beside
    it."""
    found = [obj for obj in (Path('build', source).with_suffix('.obj'), Path('build/stub', source).with_suffix('.obj'))
             if obj.exists()]
    if not found:
        fail(f'{source}: no object under build/ or build/stub/, so build it first')
    times, tables = [obj.stat().st_mtime_ns for obj in found], [section_table(obj) for obj in found]
    if len(found) == 2 and times[0] == times[1] and tables[0] != tables[1]:
        fail(f'{found[0]} and {found[1]} declare their sections differently and neither is newer; rebuild one')
    at = times.index(max(times))
    path = found[at].with_suffix('.lis')
    if not path.exists():
        fail(f'{found[at]}: no listing beside it, so rebuild it')
    return path, tables[at]


def rows(path):
    """The rows of the listing at path, up to the lists after them, as
    (listing line, offset, code, line number, text). listing line is the
    number of the listing's line the row is on. offset and line number are
    None where the row gives none. Page headers, totals and blank lines
    are skipped. Fails on a nonblank line that is no row, no page header and
    no total, and on a listing that ends before its lists."""
    found = []
    for number, raw in enumerate(content(path).decode('latin-1').split('\r\n'), 1):
        if LISTS.match(raw):
            return found
        if not raw.strip() or HEADER.match(raw):
            continue
        row = raw.ljust(TEXT)
        if not (NUMBER.fullmatch(row[:5]) and OFFSET.fullmatch(row[6:14]) and CODE.fullmatch(row[15:32])
                and NUMBER.fullmatch(row[32:37]) and row[5] + row[14] + row[37:TEXT] == ' ' * 7):
            fail(f'{path}:{number}: not a row of asmsh\'s listing')
        found.append((number, int(row[6:14], 16) if row[6:14].strip() else None, row[15:32].strip(),
                      int(row[32:37]) if row[32:37].strip() else None, row[TEXT:].rstrip()))
    fail(f'{path}: its rows end without its lists, so it may be cut short')


def lines_and_runs(path, sections):
    """The texts and runs of the listing at path, as (texts, runs). texts
    gives the text of each line asmsh read, as the listing cuts it, by the
    line's number. runs gives each section's bytes, by the section's name, as
    (start, end, owners) runs in offset order. start and end are offsets in
    the object's section. owners are the numbers of the lines the bytes are
    of.

    The listing gives each row's code and the lines it is of: its own line
    number, or for a pool word the lines that load it. Two rules assign the
    rows that give neither. The branch past a pool, its slot and its
    alignment are of the line asmsh put the pool after. A row of an offset
    and code alone is of the row above's lines.

    sections gives the object's sections, as shc_object.section_table()
    does. For each section the listing gives bytes in, its runs must tile the
    size the object declares, starting at 0. Fails on:
    - a run in no section, or one that doesn't follow the one before it in
      its section;
    - a section that sections lacks, or whose runs don't end at its size;
    - a .DATAB whose count isn't in decimal digits;
    - a line that is no row of asmsh's listing, as TEXT's comment gives its
      rows;
    - a listing that ends before its lists."""
    texts, runs, ends = {}, {}, {}
    name, last, pool, owners = None, None, None, None
    for number, offset, code, line, text in rows(path):
        if line is not None:
            texts[line], last, owners = text, line, (line,)
            started = STARTS.match(text)
            name = started[1] if started else name
            if offset is None:
                # a row without an offset adds no bytes: an .EQU's or .ASSIGN's code is its value
                continue
            repeated = REPEATS.match(text)
            if RESERVES.match(text):
                size = int(code or '0', 16)
            elif repeated:
                if not repeated[2].isdecimal():
                    fail(f'{path}:{number}: progress reads a .DATAB\'s count only in decimal digits')
                size = UNITS[(repeated[1] or 'B').upper()] * int(repeated[2])
            elif ALIGNS.match(text):
                # the bytes the alignment skips run from the end of the section's last run to its row's offset
                offset, size = ends.get(name, 0), offset - ends.get(name, 0)
            else:
                size = len(code) // 2
        elif text == BEGIN:
            pool = last
            continue
        elif text == END and pool is not None:
            pool = None
            continue
        elif pool is not None and POOLED.fullmatch(text):
            owners, size = tuple(map(int, POOLED.fullmatch(text)[1].split(','))), len(code) // 2
        elif pool is not None and text in AROUND:
            owners, size = (pool,), len(code) // 2
        elif not text and offset is not None and owners:
            size = len(code) // 2
        else:
            fail(f'{path}:{number}: not a row of asmsh\'s listing')
        if not size:
            continue
        if name is None or offset != ends.get(name, 0) or size < 0:
            fail(f'{path}:{number}: its bytes don\'t follow those before them in their section')
        runs.setdefault(name, []).append((offset, offset + size, owners))
        ends[name] = offset + size
    for name, end in ends.items():
        if name not in sections:
            fail(f'{path}: its rows give {name} {end:#x} bytes, and its object no {name} section')
        if end != sections[name][0]:
            fail(f'{path}: its rows give {name} {end:#x} bytes, and its object {sections[name][0]:#x}')
    return texts, runs


def construct(lines, ends, line, what):
    """The lines of the inline body or #pragma asm block that a site of
    tags.check_c() opens at line, or None where it has no end. ends holds
    the index of each #pragma endasm's line, in order. The site is a block
    where what is #pragma asm, and a body otherwise. A block's lines run
    from below line to the first #pragma endasm. A body's lines lie between
    its function's braces. The braces are found by text alone: the first
    line below line that is { and nothing else, and the next that is } and
    nothing else."""
    if what == '#pragma asm':
        end = next((at for at in ends if at >= line), None)
        return None if end is None else lines[line:end]
    opening = next((at for at in range(line, len(lines)) if lines[at] == '{'), None)
    closing = None if opening is None else next((at for at in range(opening, len(lines)) if lines[at] == '}'), None)
    return None if closing is None else lines[opening + 1:closing]


def cut(line):
    """line as a listing gives its text: tabs expanded to stops of 8, cut to
    the WIDTH - TEXT columns the listing shows, and trailing blanks dropped."""
    return line.expandtabs(8)[:WIDTH - TEXT].rstrip()


def standing(kind):
    """What an @asm kind makes the bytes of a body, block or function: open
    for the open kind or no tag, and basis for any other."""
    return 'open' if kind in (None, 'open') else 'basis'


def bodies(source, texts, tables):
    """The kind of each line of texts that is of one of the C source's inline
    bodies or #pragma asm blocks, by the line's number. The kind is basis or
    open, as the body's or block's @asm tag gives it. texts is
    lines_and_runs()'s, from the source's listing.

    A body is found by its text alone. It counts at each place in texts where
    its lines, cut as the listing cuts them, stand on consecutive lines,
    whatever wrote those lines there. Bodies and blocks are placed longest
    first, and one whose lines lie inside another's is the other's.

    Fails on:
    - a body or block with no end;
    - a body or block whose lines stand nowhere in texts;
    - two whose lines overlap, with neither inside the other;
    - two on the same lines, one with a basis and one open."""
    text = content(source).decode('latin-1')
    lines, sites = text.split('\n'), []
    ends = [each.line - 1 for each in csource.parse(text).directives if csource.pragma(each) == ['endasm']]
    for line, what, found in tags.check_c(source, text, tables, []):
        body = construct(lines, ends, line, what)
        if body is None:
            fail(f'{source}:{line}: {what} has no end')
        if body:
            sites.append((f'{source}:{line}: {what}', [cut(each) for each in body], standing(found and found[0])))
    starts = {}
    for line, text in texts.items():
        starts.setdefault(text, []).append(line)
    taken = []
    for where, body, kind in sorted(sites, key=lambda site: -len(site[1])):
        places = [at for at in starts.get(body[0], ())
                  if all(texts.get(at + offset) == each for offset, each in enumerate(body))]
        if not places:
            fail(f'{where}: its lines stand nowhere in its object\'s listing, so its bytes can\'t be counted')
        for at in places:
            span = (at, at + len(body))
            over = [other for other in taken if other[0] < span[1] and span[0] < other[1]]
            if not over:
                taken.append((*span, kind, where))
            elif len(over) > 1 or not over[0][0] <= span[0] < span[1] <= over[0][1]:
                fail(f'{where}: its lines overlap those of {over[0][3]} in its object\'s listing')
            elif span == over[0][:2] and kind != over[0][2]:
                fail(f'{where}: its lines are those of {over[0][3]} too, which is {over[0][2]}')
    return {line: kind for start, end, kind, _ in taken for line in range(start, end)}


def functions(source, texts, tables):
    """The kind of each line of texts, by the line's number: basis or open,
    as the @asm tag of the function the line is in gives it. texts is the
    .src source's lines as lines_and_runs() reads them from its listing. The
    functions are those tags.check_src() finds. A function's lines run from
    its first label to the next function's first label. A line ahead of the
    first function is open."""
    text = content(source).decode('latin-1')
    labels = tags.starts(text.split('\n'))
    firsts = sorted((next(run for run in labels if line - 1 in run)[0] + 1, standing(found and found[0]))
                    for line, _, found in tags.check_src(source, text, tables, []))
    found, at, kind = {}, 0, 'open'
    for line in sorted(texts):
        while at < len(firsts) and firsts[at][0] <= line:
            kind, at = firsts[at][1], at + 1
        found[line] = kind
    return found


def measured(source, tables):
    """The bytes of source's object that assembly writes, by section name, as
    (start, end, kind) runs in offset order. start and end are offsets in the
    object's section, and kind is basis, open or reserved. Every section the
    listing gives bytes in has an entry, empty where none of its bytes are
    assembly.

    The listing gives the runs and the lines each run is of, as
    lines_and_runs() reads them. A run's kind is that of its lines, and a
    pool word's lines are those that load it. A .src source's lines are all
    assembly, of the kinds functions() gives them. A C source's assembly is
    the lines of its inline bodies and #pragma asm blocks, of the kinds
    bodies() gives them. Its other lines are C, and their bytes count in no
    run. Space a .RES reserves is reserved under any tag: it holds no
    instruction or datum a basis could speak to. Two runs that meet with one kind are merged into one.

    The result is kept in MEASURED, and a later call for source returns it
    without reading the listing again.

    Fails as listing(), lines_and_runs() and bodies() do. Fails too where a
    pool word is loaded by lines of C and of assembly, or by lines with a
    basis and open lines."""
    if source in MEASURED:
        return MEASURED[source]
    path, sections = listing(source)
    texts, runs = lines_and_runs(path, sections)
    kinds = functions(source, texts, tables) if source.endswith('.src') else bodies(source, texts, tables)
    kinds = {line: 'reserved' if RESERVES.match(texts[line]) else kind for line, kind in kinds.items()}
    found = {}
    for name, each in runs.items():
        merged = found.setdefault(name, [])
        for start, end, owners in each:
            classes = {kinds.get(line) for line in owners}
            if len(classes) > 1:
                fail(f'{path}: lines {", ".join(map(str, owners))} load the pool word at {start:#x} in {name}, and '
                     'they aren\'t all C, all with a basis or all open')
            kind = classes.pop()
            if kind and merged and merged[-1][1:] == (start, kind):
                merged[-1] = (merged[-1][0], end, kind)
            elif kind:
                merged.append((start, end, kind))
    MEASURED[source] = found
    return found


def assembly(config, source, regions, tables):
    """How many bytes of regions source's assembly writes in config's image,
    as a Counter of basis, open and reserved. regions are (start, end) pairs of linked
    addresses. measured() gives the assembly's runs as offsets in each
    section of the object. A section starts at source's first row in it, as
    link.py links the object. A byte counts only inside source's own rows in
    that section. Fails as measured() does, and where the object's listing
    gives bytes in a section that no row of config's splits.txt places."""
    placed, starts, found = config.ranges(source), {}, Counter()
    for name, start, _ in placed:
        starts.setdefault(name, start)
    for name, runs in measured(source, tables).items():
        if name not in starts:
            fail(f'{source}: its object\'s {name} holds assembly, and no row of {config.splits} places it')
        own = [(start, end) for row, start, end in placed if row == name]
        for start, end, kind in runs:
            low, high = starts[name] + start, starts[name] + end
            found[kind] += overlap(regions, [(max(low, a), min(high, b)) for a, b in own if max(low, a) < min(high, b)])
    return found


def area(config):
    """The name of README.md's table row that counts config's image's game
    code: the one row of AREAS whose patterns match the image's name. Fails
    where no row's patterns match it, or more than one row's do."""
    found = [name for name, patterns in AREAS
             if patterns and any(fnmatch.fnmatchcase(config.name, pattern) for pattern in patterns)]
    if len(found) != 1:
        fail(f'{config.path}: in {" and ".join(found) or "no row"} of AREAS')
    return found[0]


def numbers(images, routes):
    """README.md's numbers: a Counter for each row of AREAS, by the row's
    name. A row's Counter gives programs, the number of images it counts,
    and these, in bytes of its code:
    - code, all of it, the linker's fill included;
    - c, the bytes of the C categories, less the inline assembly of the
      sources in INLINE_C;
    - assembly, the .src files' bytes and that inline assembly;
    - basis, the part of assembly with a basis;
    - reserved, the part of assembly that is space a .RES reserves.

    Each image's game code counts in the row area() gives it, and its SDK
    code in SDK's row, which counts no programs. The categories' bytes come
    from the placements, as code_ranges.written() counts them. The inline
    assembly's bytes and the basis come from the listings, as assembly()
    counts them. routes are as report() takes them.

    Fails as tags.Tables(), area(), written() and assembly() do. Fails too
    where the listings give an image's .src sources other bytes of its code
    than its rows do."""
    _, _, inline = routes
    tables, counted = tags.Tables(), {name: Counter() for name, _ in AREAS}
    for config in images:
        game, sdk = config.code()
        row = area(config)
        counted[row]['programs'] += 1
        for name, regions in ((row, game), (SDK, sdk)):
            each, _ = written(config, section, *routes, regions)
            src, hosted = Counter(), Counter()
            for source in config.sources():
                if source.endswith('.src'):
                    src += assembly(config, source, regions, tables)
                elif source in inline:
                    hosted += assembly(config, source, regions, tables)
            if sum(src.values()) != each['assembly']:
                fail(f'{config.splits}: the listings give its .src sources {sum(src.values()):,} of its code bytes, '
                     f'and its rows {each["assembly"]:,}')
            counts = counted[name]
            counts['code'] += sum(each.values())
            counts['c'] += sum(each[category] for category in C) - sum(hosted.values())
            counts['assembly'] += each['assembly'] + sum(hosted.values())
            counts['basis'] += src['basis'] + hosted['basis']
            counts['reserved'] += src['reserved'] + hosted['reserved']
    return counted


def share(part, whole):
    """part's share of whole in tenths of a percent, floored, so that 1000 is every byte."""
    return 1000 * part // whole


def percent(tenths, part):
    """A share as README.md writes it. tenths is the share in tenths
    of a percent, and part is the number of bytes it counts. The share is 0%
    where part is 0, and <0.1% where part is some bytes but tenths is 0.
    Otherwise it is tenths as a percent to one decimal place, such as 37.4%."""
    if not part:
        return '0%'
    if not tenths:
        return '<0.1%'
    return f'{tenths // 10}.{tenths % 10}%'


def compiled(counts):
    """A row's C and assembly bytes, the Code bytes README.md's table gives.
    They are its code bytes less the linker's fill."""
    return counts['c'] + counts['assembly']


def shares(counts):
    """A row's C and Assembly cells, from its counts. C's share of the
    compiled() bytes is floored to a tenth of a percent, and assembly's is
    the rest, so the two make 100%."""
    c = share(counts['c'], compiled(counts))
    return [percent(c, counts['c']), percent(1000 - c, counts['assembly'])]


def row(*cells):
    """A row of README.md's table, with cells in order and an empty cell
    written as a space."""
    return '|' + '|'.join(f' {cell} ' if cell else ' ' for cell in cells) + '|'


def block(counted, patched=0):
    """README.md's lines between its markers, from counted, numbers()'s
    counts, and patched, how many C sources take shckit's patches, patched
    and grafted. The lines give the headline, the table, an italic line
    under it with a note on those sources where there are any, which C's
    heading marks, and a link to the accounting, then the basis's share of
    the assembly's instructions and data, the space it reserves left out.
    The table has a row for each row of
    counted, then one for All. Fails where a row or All has no C or assembly
    bytes, or where All's assembly is all reserved space, leaving no share to
    take."""
    total = sum(counted.values(), Counter())
    for name, counts in (*counted.items(), ('All', total)):
        if not compiled(counts):
            fail(f'{name}: no C or assembly bytes, so no share of them')
    written = total['assembly'] - total['reserved']
    if not written:
        fail('no assembly but the space it reserves, so no share of it with a basis')
    lines = ["**100% of the game's code builds from source, byte for byte.**", '',
             row('', 'Binaries', 'Code bytes', 'C\\*' if patched else 'C', 'Assembly'), '|---|---:|---:|---:|---:|']
    for name, counts in counted.items():
        lines.append(row(name, '' if name == SDK else f'{counts["programs"]:,}', f'{compiled(counts):,}',
                         *shares(counts)))
    lines.append(row(*(f'**{cell}**' for cell in ('All', f'{total["programs"]:,}', f'{compiled(total):,}',
                                                   *shares(total)))))
    basis = percent(share(total['basis'], written), total['basis'])
    note = (f"\\*SHC's assembly for {patched} of the C sources matches once "
            '[shckit](https://github.com/reshaped/shckit) patches it ([Patched R10](TOOLCHAIN.md#patched-r10)). '
            if patched else '')
    lines += ['', f'*{note}[Accounting](TOOLCHAIN.md#accounting) explains the percentages.*', '',
              'Most of the assembly was handwritten in the original, not compiled from C. Compiler analysis and '
              'original symbol names already provide [evidence](TAGS.md#assembly) of handwritten origins for '
              f'{basis} of its instructions and data.']
    return lines


def rewritten(text, lines):
    """README.md's text with lines in place of the lines between its markers,
    which are one OPENING line and one CLOSING line below it. Fails unless
    text has exactly one of each, with CLOSING below OPENING."""
    old = text.split('\n')
    openings = [at for at, line in enumerate(old) if line == OPENING]
    closings = [at for at, line in enumerate(old) if line == CLOSING]
    if len(openings) != 1 or len(closings) != 1 or closings[0] < openings[0]:
        fail(f'{README}: its numbers go between one {OPENING} line and one {CLOSING} line after it')
    return '\n'.join(old[:openings[0] + 1] + lines + old[closings[0]:])


def main(argv):
    parser = argparse.ArgumentParser(prog='progress', description=__doc__.split('\n\n')[0])
    parser.add_argument('--patched', nargs='*', metavar='SOURCE', help="the Makefile's PATCHED_C")
    parser.add_argument('--grafted', nargs='*', metavar='SOURCE', help="the Makefile's GRAFTED_C")
    parser.add_argument('--inline', nargs='*', metavar='SOURCE', help="the Makefile's INLINE_C")
    goal = parser.add_mutually_exclusive_group()
    goal.add_argument('--write', action='store_true', help="rewrite README.md's numbers instead of reporting")
    goal.add_argument('--check', action='store_true', help="fail where README.md's numbers are stale instead")
    args = parser.parse_args(argv)
    if None in (args.patched, args.grafted, args.inline):
        parser.error('progress needs --patched, --grafted and --inline, as the Makefile gives them')
    routes = [set(args.patched), set(args.grafted), set(args.inline)]
    if not (args.write or args.check):
        print(report(survey(), routes), end='')
        return
    text = content(README).decode()
    new = rewritten(text, block(numbers(survey(), routes), len(routes[0] | routes[1])))
    if new != text and args.check:
        fail(f'{README}: its numbers are stale; make progress rewrites them')
    if new != text:
        save(README, new.encode())


if __name__ == '__main__':
    main(sys.argv[1:])
