"""What writes an image's bytes, and the bytes nothing writes.

Each stored byte has one writer: a source's placement, the linker's fill or
the padding a gen/ range declares. A byte none of them writes fails the
build, which names its range.
"""

from config import fail, hexa

# the categories of what writes an image's bytes, in the order the reports list
# them: each one's name, and the description the reports print beside it. A
# source's code counts under its route, and the linker's fill under fill. Of an
# image's bytes, splice.py counts as generated the data a source imports from
# the disc and the padding a gen/ range declares
CATEGORIES = {
    'c': 'C that SHC compiles to an object',
    'shckit': 'C whose R10 assembly build/shckit patches',
    'grafted': 'C whose R10 assembly tools/graft.py grafts and build/shckit patches',
    'inline': 'C with inline assembly, which R10 compiles to assembly',
    'assembly': '.src files',
    'generated': 'data from the disc, which tools/data.py writes as C',
    'fill': "the linker's fill, in the gaps it leaves",
}


def route(source, patched, grafted, inline):
    """The category of the bytes source builds, from the first of these that
    applies: generated for a gen/ range, assembly for a .src file, grafted
    for a source in grafted, shckit for one in patched, inline for one in
    inline, and c for any other. patched, grafted and inline are the
    Makefile's PATCHED_C, GRAFTED_C and INLINE_C."""
    if source.startswith('gen/'):
        return 'generated'
    if source.endswith('.src'):
        return 'assembly'
    if source in grafted:
        return 'grafted'
    if source in patched:
        return 'shckit'
    return 'inline' if source in inline else 'c'


def overlap(regions, ranges):
    """The number of bytes that lie both in a range of regions and in one of
    ranges, each a list of (start, end) pairs. The pairs of each list must
    be disjoint, or a byte counts more than once."""
    return sum(max(0, min(end, b) - max(start, a)) for start, end in ranges for a, b in regions)


def uncovered(regions, held):
    """The parts of regions that no range of held covers, as (start, end)
    pairs in the order of regions. held may be in any order."""
    held = sorted(held)
    runs = []
    for low, high in regions:
        at = low
        for start, end in held:
            if start < high and at < end:
                if at < start:
                    runs.append((at, start))
                at = end
        if at < high:
            runs.append((at, high))
    return runs


def site(name):
    """Where a range lies, as the reports name it: in section name, or
    between section rows where name is None."""
    return f'in {name}' if name else 'between sections'


def filled(name):
    """The linker's fill at site(name), as the splicer names that writer."""
    return f"the linker's fill {site(name)}"


def padded(name):
    """The padding a gen/ range declares in section name, as the splicer
    names that writer."""
    return f'the padding a gen/ range declares in {name}'


def writers(config, section):
    """Every range the tables give a writer in config's image, its bss
    included, as (start, end, writer) in address order. The writers are the
    image's placements, the linker's fill and the padding its gen/ ranges
    declare. A placement's writer is its source, and filled() and padded()
    name the others. section(source, name) gives the size and alignment a
    source's object declares for a section, as link.section does. Fails as
    Config.gaps() does."""
    fill, padding = config.gaps(section)
    found = [(start, end, source) for source, _, start, end in config.placements]
    found += [(start, end, filled(name)) for name, start, end in fill]
    found += [(start, end, padded(name)) for name, start, end in padding]
    return sorted(found)


def writes(config, section):
    """The ranges writers() gives, as (start, end)."""
    return [(start, end) for start, end, _ in writers(config, section)]


def pieces(config, stored):
    """The image's bytes from its base to stored, a linked address, as
    (kind, section, start, end) pieces in address order. The pieces are cut
    at both ends of each section config.layout() gives and of each code
    region. kind is code or data. section is the name of the layout section
    the piece lies in, or None for a piece between two section rows."""
    game, sdk = config.code()
    regions, tiles = game + sdk, config.layout()
    edges = {config.base, stored}
    edges.update(at for _, low, high in tiles for at in (low, high))
    edges.update(at for low, high in regions for at in (low, high))
    edges = sorted(at for at in edges if config.base <= at <= stored)
    found = []
    for start, end in zip(edges, edges[1:]):
        kind = 'code' if any(low <= start < high for low, high in regions) else 'data'
        name = next((name for name, low, high in tiles if low <= start < high), None)
        found.append((kind, name, start, end))
    return found


def unwritten(config, held, stored=None):
    """The parts of pieces(config, stored) that no range of held covers, as
    pieces() gives them. held is (start, end) pairs. With stored None, only
    the code pieces count, up to the end of the last code region, which
    config gives without the file."""
    game, sdk = config.code()
    end = max((high for _, high in game + sdk), default=config.base) if stored is None else stored
    runs = []
    for kind, name, start, top in pieces(config, end):
        if stored is None and kind != 'code':
            continue
        runs.extend((kind, name, low, high) for low, high in uncovered([(start, top)], held))
    return runs


def problems(where, runs):
    """A line for fail() to print for each of runs, as unwritten() gives
    them. Each line names where, then the run's range, its kind and its
    site."""
    return [f'{where}: no source or fill writes {hexa(start)} to {hexa(end)}, {kind} {site(name)}'
            for kind, name, start, end in runs]


def check_code(config, section):
    """Check that the ranges writers() gives write every byte of config's
    code, as Config.code() gives it. section is as writers() takes it. Fails
    as writers() and Config.code() do, and where the ranges leave a byte
    unwritten, with a line naming each unwritten range."""
    runs = unwritten(config, writes(config, section))
    if runs:
        fail(*problems(config.splits, runs))


def written(config, section, patched, grafted, inline, regions=None):
    """config's code bytes by what writes them, as (counts, mixed). counts
    maps each name in CATEGORIES to the bytes that category writes. mixed is
    how many of the shckit and grafted bytes, the code build/shckit patches,
    are of sources with inline assembly.

    regions is a list of disjoint (start, end) pairs of the image's code,
    such as its game code alone, or None for all of it. Both results count
    only its bytes, and counts sums to its size. A source's bytes count under its
    route, as route() gives it from patched, grafted and inline, and the
    linker's fill counts under fill. section is as writers() takes it.

    Fails as check_code() does, on all of the image's code whatever regions
    is. Fails too where counts would not sum to the size of regions, naming
    both sums."""
    check_code(config, section)
    game, sdk = config.code()
    regions = sorted(game + sdk if regions is None else regions)
    counts, mixed = dict.fromkeys(CATEGORIES, 0), 0
    for source, _, start, end in config.placements:
        kind = route(source, patched, grafted, inline)
        counts[kind] += overlap(regions, [(start, end)])
        if kind in ('shckit', 'grafted') and source in inline:
            mixed += overlap(regions, [(start, end)])
    counts['fill'] = overlap(regions, [(start, end) for _, start, end in config.linker_fill(section)])
    # a byte two writers write counts twice
    code = sum(end - start for start, end in regions)
    if sum(counts.values()) != code:
        fail(f'{config.splits}: the categories count {sum(counts.values()):,} of the image\'s {code:,} code bytes')
    return counts, mixed
