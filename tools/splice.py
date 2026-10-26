"""Verify linked sources against an original image and build the image from them.

ORIGINAL is the original image: its file in orig/, or for a .PRS file the
.BIN that build/gen/img/ holds decoded. TARGET's stem names the image, as
config/ does. The splicer builds TARGET from a buffer of zeros: each
source's bytes, the linker's fill and the padding gen/ ranges declare, as
config/<image>/splits.txt places them. It reads ORIGINAL only to check them
and to take the image's size from it, and writes no byte of it to TARGET. A
stored byte none of them writes fails, naming the image, the range, its
section and whether it is code or data. TARGET is removed first and written
only once every check passes, so a failed check leaves none; a failed
write can leave part of one.

Checks cover source lengths and bytes, linker fill and declared padding,
object boundaries in game code, bss beyond the file, imported bodies, that
no byte is written twice and that every stored byte is written.
See TOOLCHAIN.md for the layout and declared-data contracts.

--check validates linked-source lengths without the disc, using stubs for
sources with imported bodies and skipping generated ranges, and that each
image's placements and fill write every byte of its code. --verbose reports
the bytes of the image and of its code by what writes them, each source by
its route. --patched, --grafted and --inline give the routes; --verbose needs them,
and they do not alter verification.
"""

import argparse
import errno
import os
import re
import sys
from pathlib import Path

from code_ranges import CATEGORIES, check_code, filled, overlap, padded, problems, route, site, unwritten
from config import FIRST, ZEROED, Config, address, configs, content, fail, hexa, rows, save
from link import linked, section

# a C source that includes a .inc file, a body or its text.inc or values.inc,
# has a stub build; the Makefile's BODY_C and STUB_INC_FILES grep the same pattern
BODY = re.compile(r'include "[A-Za-z0-9_/]*\.inc"$', re.M)
# SHC's fill inside an object, around its pools: a NOP, 09 00, with its bytes swapped
SHC_FILL = b'\x00\x09'


def placed(offset, ranges):
    """The address at which byte offset of a linked source links, the
    linked source holding its ranges in table order."""
    for _, start, end in ranges:
        if offset < end - start:
            return start + offset
        offset -= end - start


def bodies(source, path, held):
    """The bodies path lists, as (offset, size, text, name) in offset order.
    A row gives a body's address in source's home, and held is source's
    ranges there: offset is into them joined in table order, as source's
    linked source holds them, and text says whether the body is a text. A
    body lies in one range, and no two share a byte."""
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


def compare(source, stub, ranges, data, spans):
    """Check that the stub build's linked source matches the source's own
    outside its bodies, and holds 0 in each body but a text, whose placeholder
    goes unchecked. The first byte that doesn't fails, naming its address."""
    stub = content(stub)
    if len(stub) != len(data):
        fail(f'{source}: its stub build is {len(stub):,} bytes, and its own {len(data):,}')
    # by byte: 0 outside the bodies, 1 in a body, 2 in a text
    mask = bytearray(len(data))
    for offset, size, text, _ in spans:
        mask[offset:offset + size] = (b'\2' if text else b'\1') * size
    for index, (own, stubbed, body) in enumerate(zip(data, stub, mask)):
        if body != 2 and stubbed != (0 if body else own):
            problem = 'holds a byte other than 0' if body else 'differs from its own'
            where = 'in a body' if body else 'outside its bodies'
            fail(f'{source}: its stub build {problem} at {hexa(placed(index, ranges))}, {where}')


def has_bodies(source):
    """Whether a C source imports bodies, texts or values through .inc files."""
    path = Path(source)
    return path.suffix == '.c' and path.exists() and BODY.search(path.read_text(errors='replace')) is not None


def home(source, ranges, splits, images):
    """The ranges source's linked source holds in its home, the first image
    by name that links it, as the Makefile's home picks it for tools/data.py
    --bodies. bodies() turns bodies.txt's addresses there into offsets into
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


def length(path, data, ranges, splits):
    """Fail where the linked source at path, data, isn't as long as its
    ranges, the rows of splits that place it."""
    want = sum(end - start for _, start, end in ranges)
    if len(data) != want:
        fail(f'{path}: {len(data):,} bytes, where {splits} places {want:,}')


def check():
    """--check: each linked source, the stub build's for a body
    source, is as long as its rows, and each image's placements and fill
    write every byte of its code. gen/ ranges go unchecked, since only the
    disc gives their bytes. The first problem fails, naming it."""
    for config in configs():
        for source, ranges in config.held().items():
            if source.startswith('gen/'):
                continue
            path = linked(source, config.name, 'build/stub' if has_bodies(source) else 'build')
            if not path.exists():
                fail(f'{config.splits}: {source} is there, and nothing built {path}')
            length(path, path.read_bytes(), ranges, config.splits)
        check_code(config, section)


def differs(disc, base, start, data):
    """The address of the first byte of data, placed at start, that differs
    from the original's, disc, whose first byte links at base, or that lies
    past its end; None where none does."""
    held = disc[start - base:start - base + len(data)]
    if held == data:
        return None
    return start + next((at for at, (own, old) in enumerate(zip(data, held)) if own != old), len(held))


def whole(source, start, end, disc, base, fill, game_code):
    """Check source's code boundaries, [start, end), against the image's
    padding and alignment. SHC's fill lies only inside an
    object, so on neither side of either end. Game code but the events',
    which build without -extra=a=800, starts each object on 32 bytes: the
    flag aligns R32's code to 32, and each object of game code that R10
    builds or asmsh assembles starts on 32 in every image, though some
    declare 4 or 16. So code that ends off 32 has the linker's fill after it,
    or ends the file. asmsh pads with a real NOP, so in assembled code only
    alignment shows an object's ends. Each end of the image's game code is
    already an object's end, so it goes unchecked. A failure names the
    address."""
    def at(address):
        return disc[address - base:address - base + 2] if address >= base else b''

    aligned = not source.startswith('events/')
    if all(start != low for low, _ in game_code):
        if at(start - 2) == SHC_FILL:
            fail(f'{source}: {hexa(start)} follows SHC\'s 00 09 fill, so its object starts before it')
        if at(start) == SHC_FILL:
            fail(f'{source}: {hexa(start)} holds SHC\'s 00 09 fill, so its object starts before it')
        if aligned and start % 32:
            fail(f'{source}: {hexa(start)} is off 32 bytes, so its object starts before it')
    if all(end != high for _, high in game_code):
        if at(end) == SHC_FILL:
            fail(f'{source}: {hexa(end)} precedes SHC\'s 00 09 fill, so its object ends after it')
        if at(end - 2) == SHC_FILL:
            fail(f'{source}: {hexa(end - 2)} holds SHC\'s 00 09 fill, so its object ends after it')
        if aligned and end % 32 and at(end) not in (b'', bytes([fill]) * 2):
            fail(f'{source}: {hexa(end)} is off 32 bytes and no fill follows it, so its object ends after it')


def once(target, writes):
    """Fail, naming both, where two of writes, each (start, end, writer),
    write a byte in common, even with equal values."""
    last = None
    for start, end, writer in sorted(writes):
        if last and start < last[1]:
            fail(f'{target}: {last[2]} and {writer} both write {hexa(start)} to {hexa(min(end, last[1]))}')
        if not last or end > last[1]:
            last = start, end, writer


def splice(original, target, patched, grafted, inline):
    """Build target's image from a buffer of zeros: its sources' bytes, the
    linker's fill and the padding its gen/ ranges declare, each checked
    against original's bytes. patched, grafted and inline give each source's
    route, as code_ranges.route takes them. Return the image; the bytes of
    the image, and those of its code, by the category of CATEGORIES that
    writes them, as two maps that sum to the image's size and to its code's;
    and the code bytes of the sources with inline assembly that build/shckit
    patches. A byte that two sources, or a source and the fill, write fails,
    naming both, and so does each range of stored bytes none of them writes."""
    config = Config(target.stem.lower())
    base = config.base
    game_code, sdk_code = config.code()

    # the original's bytes: each source and gap is checked against them, and
    # they give the image its size, but no byte of them goes in it
    disc = content(original)
    stored = base + len(disc)
    image = bytearray(len(disc))
    # the section rows tile the image from its base, as Config.linker_fill
    # holds them to; they run on to the file's end, and only bss past it
    for name, start, end in config.sections:
        if end > stored and name not in ZEROED:
            fail(f'{config.splits}: section {name} runs to {hexa(end)}, past the end of {original}, at {hexa(stored)}')
    last = max((end for _, _, end in config.sections), default=stored)
    if last < stored:
        fail(f'{config.splits}: its sections end at {hexa(last)}, short of the end of {original}, at {hexa(stored)}')
    # each range a source, the fill or declared padding writes, as (start, end, writer)
    writes = []
    # the bytes each category writes, in the image and in its code
    built, code = dict.fromkeys(CATEGORIES, 0), dict.fromkeys(CATEGORIES, 0)
    regions = game_code + sdk_code
    mixed = 0
    # every image's tables, read once for all the sources with bodies
    images = []
    for source, ranges in config.held().items():
        path = linked(source, config.name)
        data = content(path)
        length(path, data, ranges, config.splits)
        imported = 0
        if has_bodies(source):
            images = images or configs()
            spans = bodies(source, Path('build/gen', source).with_suffix('') / 'bodies.txt', home(source, ranges, config.splits, images))
            compare(source, linked(source, config.name, 'build/stub'), ranges, data, spans)
            imported = sum(size for _, size, _, _ in spans)
            for offset, size, _, name in spans:
                at = placed(offset, ranges)
                if overlap(game_code + sdk_code, [(at, at + size)]):
                    fail(f'{source}: {name} lies in code, at {hexa(at)}, and a body is data from the disc')
        kind = route(source, patched, grafted, inline)
        for name, start, end in ranges:
            chunk, data = data[:end - start], data[end - start:]
            if start < base:
                fail(f'{source}: {hexa(start)} lies before {original}, at {hexa(base)}')
            # only bss can run past the file's end, and there it holds 0
            if end > stored:
                if name not in ZEROED:
                    fail(f'{source}: {name} {hexa(max(start, stored))} to {hexa(end)} lies past the end of {original}')
                if any(chunk[max(start, stored) - start:]):
                    fail(f'{source}: its bss past the end of {original} holds bytes that aren\'t 0')
                chunk, end = chunk[:max(0, stored - start)], max(start, stored)
            at = differs(disc, base, start, chunk)
            if at is not None:
                fail(f'{source}: its byte at {hexa(at)} differs from the disc\'s')
            if overlap(game_code, [(start, end)]):
                whole(source, start, end, disc, base, config.fill, game_code)
            image[start - base:end - base] = chunk
            writes.append((start, end, source))
            built[kind] += end - start
            code[kind] += overlap(regions, [(start, end)])
            if kind in ('shckit', 'grafted') and source in inline:
                mixed += overlap(regions, [(start, end)])
        # bodies, texts and values come from the disc, so count as generated whatever the source's kind
        built[kind] -= imported
        built['generated'] += imported
    # each gap the linker leaves holds the image's fill byte, or 0 in bss;
    # the file stores it up to its end, past which only bss runs
    for name, start, end in config.linker_fill(section):
        end = max(start, min(end, stored))
        byte = 0 if name in ZEROED else config.fill
        fill = bytes([byte]) * (end - start)
        if differs(disc, base, start, fill) is not None:
            fail(f'{original}: {hexa(start)} to {hexa(end)}, a gap the linker leaves {site(name)}, '
                 f'holds bytes other than {byte:#04x}')
        image[start - base:end - base] = fill
        writes.append((start, end, filled(name)))
        built['fill'] += end - start
        code['fill'] += overlap(regions, [(start, end)])
    # the padding before a gen/ range that continues a source's object holds
    # the image's fill byte, and counts with the range, as generated; a gen/
    # range lies in no code
    for name, start, end in config.padding(section):
        fill = bytes([config.fill]) * (end - start)
        if differs(disc, base, start, fill) is not None:
            fail(f'{original}: {hexa(start)} to {hexa(end)}, padding a gen/ range declares in {name}, '
                 f'holds bytes other than {config.fill:#04x}')
        image[start - base:end - base] = fill
        writes.append((start, end, padded(name)))
        built['generated'] += end - start
    once(target, [write for write in writes if write[0] < write[1]])
    # the original fills no gap: a stored byte none of them writes fails
    runs = unwritten(config, [(start, end) for start, end, _ in writes], stored)
    if runs:
        fail(*problems(target, runs))
    # each stored byte has one writer, so the categories sum to the image's bytes and its code's
    stored_code = overlap(regions, [(base, stored)])
    if sum(built.values()) != len(disc) or sum(code.values()) != stored_code:
        fail(f'{target}: its categories count {sum(built.values()):,} of its {len(disc):,} bytes, '
             f'and {sum(code.values()):,} of its {stored_code:,} code bytes')
    return image, built, code, mixed


def report(target, image, built, code, mixed, verbose=False):
    """Print the splice's status line, or with verbose its bytes and its
    code's by what writes them, each source by its route."""
    if not verbose:
        print(f'  splice {target}')
        return
    config = Config(target.stem.lower())
    game, sdk = (overlap(regions, [(config.base, config.base + len(image))]) for regions in config.code())
    print(f'{target}: {len(image):,} bytes, {game + sdk:,} of them code: {game:,} game and {sdk:,} SDK')
    print(f'  {"":<9}  {"code":>9}  {"image":>9}')
    for category, what in CATEGORIES.items():
        print(f'  {category:<9}  {code[category]:>9,}  {built[category]:>9,}  {what}')
        if category == 'grafted' and mixed:
            print(f'{"":<35}{mixed:,} of the shckit and grafted code bytes are of sources with inline assembly')


def main(argv):
    parser = argparse.ArgumentParser(prog='splice', description=__doc__.split('\n\n')[0])
    parser.add_argument('original', metavar='ORIGINAL', type=Path, nargs='?',
                        help='the original image: its file in orig/, or the .BIN build/gen/img/ decodes a .PRS file to')
    parser.add_argument('target', metavar='TARGET', type=Path, nargs='?',
                        help='the image to build under build/, such as build/1ST_READ.BIN')
    parser.add_argument('--check', action='store_true',
                        help="check each link's length, and that every code byte is written, without the disc")
    parser.add_argument('--verbose', action='store_true',
                        help="report the image's bytes, and its code's, by what writes them")
    parser.add_argument('--patched', nargs='*', metavar='SOURCE', help="the Makefile's PATCHED_C")
    parser.add_argument('--grafted', nargs='*', metavar='SOURCE', help="the Makefile's GRAFTED_C")
    parser.add_argument('--inline', nargs='*', metavar='SOURCE', help="the Makefile's INLINE_C")
    args = parser.parse_args(argv)
    routes = (args.patched, args.grafted, args.inline)
    if args.check and (args.original or args.target):
        parser.error('--check takes no image: it checks every image without the disc')
    if args.check:
        check()
        return
    if args.target is None:
        parser.error('name an original image and the image to build, or --check')
    if args.target.resolve() == args.original.resolve():
        parser.error(f'{args.target} is the original image, and the splice writes a new one')
    if args.verbose and None in routes:
        parser.error('--verbose needs --patched, --grafted and --inline, as the Makefile gives them')
    # unlink fails on a directory with EISDIR on Linux but EPERM on macOS, so
    # name a directory the same way on both
    if args.target.is_dir():
        fail(f'{args.target}: {os.strerror(errno.EISDIR)}')
    try:
        args.target.unlink(missing_ok=True)
    except OSError as error:
        fail(f'{args.target}: {error.strerror}')
    image, *counts = splice(args.original, args.target, *(set(names or ()) for names in routes))
    save(args.target, image)
    report(args.target, image, *counts, args.verbose)


if __name__ == '__main__':
    main(sys.argv[1:])
