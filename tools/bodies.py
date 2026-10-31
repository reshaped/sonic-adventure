"""The bodies, texts and values a C source imports from the disc, as its
bodies.txt lists them, and where they lie in its linked source.

A C source that includes a .inc file has a stub build, and tools/data.py
--bodies writes its .inc files and its bodies.txt from its home, the first
image by name that links it. bodies.txt gives each item's address there,
its size and its type. tools/splice.py checks that the stub build's linked
source matches the full build's outside the items. tools/deps.py names the
stub's files in make's rules, and tools/content.py copies the texts.
TOOLCHAIN.md#stub-and-full-builds gives the two builds.
"""

import re
from pathlib import Path

from images import FIRST, ZEROED
from tables import address, hexa, rows
from tool import content, fail

# a C source that includes a .inc file, a body or its text.inc or values.inc,
# has a stub build; the Makefile's BODY_C and STUB_INC_FILES grep the same pattern
BODY = re.compile(r'include "[A-Za-z0-9_/]*\.inc"$', re.M)


def has_bodies(source):
    """Whether a C source imports bodies, texts or values through .inc files.
    A source that can't be read fails, naming it."""
    path = Path(source)
    return path.suffix == '.c' and path.exists() and BODY.search(content(path).decode(errors='replace')) is not None


def home(source, ranges, splits, images):
    """The ranges source's linked source holds in its home, the first image
    by name that links it, as the Makefile's home picks it for tools/data.py
    --bodies. images is every image's Config, in name order, as configs()
    gives them. bodies() turns bodies.txt's addresses there into offsets into
    those ranges joined in table order, so ranges, the source's in splits,
    must join the same way. 1ST_READ.BIN's file holds no bss, so where it is
    the home, another image's own bss can follow the ranges the two share."""
    def sizes(listed):
        return [(name, end - start) for name, start, end in listed]

    for config in images:
        held = config.held().get(source)
        if held:
            shared, rest = ranges[:len(held)], ranges[len(held):]
            bss = config.path == FIRST and all(name in ZEROED for name, _, _ in rest)
            if sizes(shared) != sizes(held) or (rest and not bss):
                fail(f'{source}: its ranges in {config.splits} differ in sections or sizes from those in {splits}')
            return held
    fail(f'{source}: no splits.txt lists it')


def bodies(source, path, held):
    """The bodies path lists, as (offset, size, text, name) in offset order.
    A row gives a body's address in source's home, and held is source's
    ranges there: offset is into them joined in table order, as source's
    linked source holds them, and text says whether the body is a text. A
    text's size leaves out its closing null. A body lies in one range, and
    no two share a byte."""
    spans = []
    for where, row in rows(path):
        if len(row) < 4:
            fail(f'{where}: not a row: NAME ADDRESS SIZE TYPE')
        name, field, size, *ctype = row
        start = address(field, where)
        if not size.isdigit() or not int(size):
            fail(f'{where}: {name}: {size} is no size')
        end, before = start + int(size), 0
        for _, low, high in held:
            if low <= start and end <= high:
                spans.append((before + start - low, int(size), ctype == ['text'], name))
                break
            before += high - low
        else:
            fail(f'{where}: {name}, {hexa(start)} to {hexa(end)}, lies in no one range of {source}')
    spans.sort()
    for first, second in zip(spans, spans[1:]):
        if second[0] < first[0] + first[1]:
            fail(f'{path}: {source}\'s {first[3]} and {second[3]} share bytes')
    return spans


def placed(offset, ranges):
    """The address at which byte offset of a linked source links, the
    linked source holding its ranges in table order."""
    for _, start, end in ranges:
        if offset < end - start:
            return start + offset
        offset -= end - start
