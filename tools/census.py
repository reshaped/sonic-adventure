"""Check config/1st_read/census.txt, 1ST_READ.BIN's census, against the
numbers for RAM it explains and against make gcc's objects, and check the
motion files' addresses in 1ST_READ.BIN's image.

    python3 tools/census.py [--generated] [--disc]
    python3 tools/census.py --gcc [--add IMAGE SOURCE ...]

make gcc's link lays out 1ST_READ.BIN and the images beside it anew. A
number 1ST_READ.BIN holds for a place outside its image stays right only
where that place keeps its address. The census explains each such number
by a row, whose fields and dispositions the table's header gives. A row
explains the numbers of its file in its span, which lies in one view of
RAM: 0x0c, 0x8c or 0xac. It counts the numbers in its span, not which they
are, so a number that takes another's place passes.

Without --gcc, the census reads the files 1ST_READ.BIN's SHC build reads.
Its candidates are their numbers for RAM past 1ST_READ.BIN's image. The
files are:

- each source 1ST_READ.BIN's splits.txt places, other than a gen/ range;
- each .h, .inc and .c file under SOURCE_ROOTS that a C source's .d file
  names;
- config/1st_read/symbols.txt, each of whose rows gives a number;
- with --generated, the C make writes from the disc for 1ST_READ.BIN's gen/
  ranges;
- with --generated, each file under build/gen/ that a full build's .d file
  names (TOOLCHAIN.md#stub-and-full-builds).

With --gcc, the census reads every image's GCC side instead. An image's GCC
side is the C make gcc compiles for each source the image's splits.txt
places, other than a gen/ range. That C is the source, or the C in gcc/
that replaces a .src file. A .src file that nothing replaces is read as
assembly. Each file under SOURCE_ROOTS that a compile's .d file names is
read too. So is each SOURCE an --add gives the image, a C source in gcc/
that the image links after its own. A GCC side's candidates are its numbers
for RAM at an address some image's symbols.txt gives a name. The number and
the address may each lie in any of VIEWS, and in 1ST_READ.BIN's image too.
The link places that name anew, so the number keeps its meaning only where
a row of its file says why.

A number of assembly is H'hex or decimal, outside comments and strings. A
number of C is an integer constant outside comments and strings, or a sum,
as below. The census without --gcc reads C on both sides of each GCC guard,
and --gcc reads GCC's side alone. A macro's body is read where the macro is
defined, so a macro used alone is no number of the file that uses it. A
file under build/gen/ holds only its integer constants.

A sum is MACRO + N or MACRO - N, where N is an integer constant and a
definition of MACRO gives MACRO a number. Such a definition takes no
parameters, and its body holds one integer constant: a number for RAM,
past the image without --gcc. The sum is a number of the file that holds
it, and its value is MACRO's number plus or minus N. MACRO's definitions
are those of each compile that reads the sum's file, on SHC's side of each
guard without --gcc and on GCC's with it. The census takes no value for a
sum, and fails, where:

- MACRO's definitions aren't all one, or one is an #undef;
- the sum is no whole operand, as where a * or a [ stands beside it;
- the sum's type isn't one in which N counts bytes: an integer of 32 bits
  or more, which holds an address whole, or a pointer to a byte.

The sum's type is that of MACRO's body, the number or a cast of it, and a
cast before MACRO replaces it. A body that holds more than the number and
its cast has no type, and leaves the sum none even with a cast before
MACRO. A ) before MACRO that closes no cast to names and stars leaves the
sum no type too. A sum spelled otherwise, as (AREA) + 4 or 4 + AREA, is no
number.

The census without --gcc counts each row past the image of a file it reads,
or of one under build/gen/. To count a row is to hold its COUNT to the
number of candidates the row explains. --gcc counts each other row: one in
the image or below it, or of a file the census without --gcc doesn't read.
It reads 1ST_READ.BIN's SHC .d files to tell the two apart. Either command
line fails on:

- a row out of form or across the image's end, and two rows of one file
  that span one number;
- a candidate no row of its file explains;
- a row it counts whose COUNT isn't the number of candidates it explains.
  Without --generated, the census counts no row of a file under build/gen/;
- a sum whose value it can't take;
- a C source of 1ST_READ.BIN's with no .d file under build/ or build/stub/,
  and a .d file that opens with no rule's TARGET:.

With --disc, the census reads the motion files too
(TOOLCHAIN.md#motion-files), each as make gcc decodes it to build/gen/img/.
make gcc compiles a motion file's data from its content, C that
tools/content.py writes (GCC.md#content). The link binds each pointer the
content places to its definition's new address, and leaves any other word
as the disc holds it. So the census fails on each word of a motion file
that holds an address in 1ST_READ.BIN's image where its content places no
pointer, as tools/content.py's pointer_words() gives them. The address may
lie in any of VIEWS. A word is 32 bits, little-endian and 4-aligned in the
file. The census fails too on a motion file make gcc hasn't decoded.

The link places each name an object defines, so a name keeps its address
only where no object defines it. --gcc fails too on:

- a placed row at an address symbols.txt gives no name;
- a name symbols.txt gives at a placed row's address that the GCC object of
  the row's source doesn't define global;
- a census row of symbols.txt, not placed, whose span holds a name that a
  GCC object of 1ST_READ.BIN's defines global;
- a row it counts of a file that no image's GCC side reads;
- an --add for an image config/ doesn't describe;
- a .d file make gcc hasn't written, or an object a row of symbols.txt
  needs that make gcc hasn't built.

The census sees no number for RAM split across constants, encoded otherwise
or built at run time, and no float's bits. --gcc sees no number inside an
object past its name's address. Of the disc's files, only --disc reads any,
and only the motion files.

make gcc runs both command lines once the objects they read are built.
"""

import argparse
import os
import re
import struct
import sys
from collections import Counter, namedtuple
from pathlib import Path

import elf
import layout
from config import Config, gcc_object
from content import pointer_words
from csource import GCC, SHC, code, macro_directives, pairs, tokens
from gcclink import RAM, exports
from images import CONFIG, FIRST, FIRST_BASE, FIRST_END, ROOTS, files, images, motions
from tables import ADDRESS, GENERATED, NAME, address, hexa, named, rows
from tool import content, fail

CENSUS = FIRST / 'census.txt'
SYMBOLS = FIRST / 'symbols.txt'
# the starts of the views the game and its SDK reach RAM through: P0's and
# P1's, which the cache serves, and P2's, which it doesn't. Each start is a
# multiple of RAM's size, so an address's offset into RAM is address % RAM
VIEWS = (0x0C000000, 0x8C000000, 0xAC000000)
# 1ST_READ.BIN's image, as offsets into RAM
IMAGE = range(FIRST_BASE % RAM, FIRST_END % RAM)
# the directories whose files the census reads where a .d file names them:
# ROOTS, and gcc/, which holds the C GCC compiles in place of a .src file or
# after an image's own sources
SOURCE_ROOTS = (*ROOTS, 'gcc')
# each disposition, with the number of fields it takes before the reason
DISPOSITIONS = {'anchor': 3, 'placed': 1, 'data': 0}
COUNT = re.compile(r'[1-9][0-9]*')
ASSEMBLY_NUMBER = re.compile(r"\bH'([0-9A-Fa-f]+)|\b([0-9]+)\b")
ASSEMBLY_STRING = re.compile(r'"[^"\n]*"')
# the names that spell a byte, and those that spell an integer of 32 bits
# or more, which holds an address whole. Either may take a sign of SIGNS
BYTE = {'char', 'Sint8', 'Uint8'}
INTEGER = {'int', 'long', 'Sint32', 'Uint32'}
SIGNS = {'signed', 'unsigned'}
# the tokens that join MACRO + N to more beside it, so that it is no whole
# operand: JOINS_BEFORE before MACRO, and JOINS_AFTER after N. A + or - makes
# the sum part of a longer one. Each other token binds tighter than the
# sum's + or -, as a cast's ) does
JOINS_BEFORE = {'*', '/', '%', '+', '-', '~', '!', 'sizeof', '.', '->', ')'}
JOINS_AFTER = {'*', '/', '%', '+', '-', '[', '(', '.', '->'}

# a row of the census. It spans the numbers of file from start up to end,
# which is start + 1 for a row without END. count is how many it says the
# file holds there. source is a placed row's SOURCE, and None for any
# other. at is the row's path:line
Row = namedtuple('Row', 'file start end count disposition source at')


def ram(value):
    """Whether value is an address of RAM in one of VIEWS."""
    return any(view <= value < view + RAM for view in VIEWS)


def past(value):
    """Whether value is an address of RAM past 1ST_READ.BIN's image, in one
    of VIEWS."""
    return ram(value) and value % RAM >= IMAGE.stop


def census(sources):
    """config/1st_read/census.txt's rows, each a Row, in the table's order.
    sources are those 1ST_READ.BIN's splits.txt places.

    A row out of the form the table's header gives fails, and so does:

    - a row whose END isn't past its START, or whose numbers aren't
      addresses of RAM in one view;
    - a row whose numbers lie on both sides of the image's end, since its
      START alone decides which command line counts it;
    - a row of symbols.txt or of a file under build/gen/ that starts in
      1ST_READ.BIN's image or below it. The census takes no number of
      either there;
    - an anchor whose numbers lie outside FROM to TO, TO included;
    - a placed row that isn't symbols.txt's, has an END, or names a SOURCE
      1ST_READ.BIN doesn't place;
    - a row that spans a number an earlier row of its file spans.

    The file a row names need not exist."""
    found = []
    for at, fields in rows(CENSUS):
        form = (f'{at}: not a row: FILE START [END] COUNT, then anchor NAME FROM TO, placed SOURCE or data, then '
                'the reason')
        if len(fields) < 5:
            fail(form)
        file, start, *rest = fields
        start = address(start, at)
        end = address(rest.pop(0), at) if ADDRESS.fullmatch(rest[0]) else None
        count, disposition, *rest = rest
        if not COUNT.fullmatch(count) or disposition not in DISPOSITIONS or len(rest) <= DISPOSITIONS[disposition]:
            fail(form)
        last = start if end is None else end - 1
        if not (ram(start) and start <= last and start // RAM == last // RAM):
            fail(f'{at}: {hexa(start)}{f" to {hexa(end)}" if end is not None else ""}: not a span of RAM, in one '
                 'view')
        if past(start) != past(last):
            fail(f"{at}: {hexa(start)} to {hexa(end)}: its numbers lie on both sides of 1ST_READ.BIN's image's end")
        if (file == str(SYMBOLS) or file.startswith('build/gen/')) and not past(start):
            fail(f"{at}: {hexa(start)} lies in 1ST_READ.BIN's image or below it, where the census takes no number "
                 f'of {file}')
        if disposition == 'anchor':
            low, high = (address(field, at) for field in rest[1:3])
            if not low <= start <= last <= high:
                fail(f'{at}: its numbers lie outside its extent, {hexa(low)} to {hexa(high)}')
        if disposition == 'placed':
            if file != str(SYMBOLS) or end is not None:
                fail(f"{at}: a placed row is {SYMBOLS}'s, at one name's address")
            if rest[0] not in sources:
                fail(f"{at}: {rest[0]}: no source {FIRST / 'splits.txt'} places")
        row = Row(file, start, last + 1, int(count), disposition, rest[0] if disposition == 'placed' else None, at)
        other = next((other for other in found if other.file == file and other.start < row.end
                      and row.start < other.end), None)
        if other is not None:
            fail(f'{at}: its numbers meet those of the row at {other.at}')
        found.append(row)
    return found


def integer(word):
    """The value of word, a C token, as an integer constant, or None."""
    # an integer constant starts with a digit, and most tokens don't, so
    # testing that first spares layout.number() a raise for each
    if not word[:1].isdigit():
        return None
    try:
        return layout.number(word)[0]
    except layout.LayoutError:
        return None


def bare(words):
    """words, tokens' texts, less each pair of parentheses that encloses them
    all."""
    while words[:1] == ['('] and pairs(words)[0] == len(words) - 1:
        words = words[1:-1]
    return words


def cast(words):
    """The type that words, tokens holding one integer constant, cast it to,
    as the cast's names and stars. The type is () where words are the
    integer alone. It is None where they are no cast of the integer, as
    *(char *)0x8c00007e isn't. It is None too where they cast it to more
    than names and stars. Parentheses may enclose the integer, the whole,
    or both."""
    words = bare(words)
    if len(words) == 1:
        return ()
    close = pairs(words)[0] if words[0] == '(' else None
    operand = bare(words[close + 1:]) if close is not None else []
    if len(operand) != 1 or integer(operand[0]) is None:
        return None
    kind = words[1:close]
    return tuple(kind) if all(word == '*' or NAME.fullmatch(word) for word in kind) else None


def bytewise(kind):
    """Whether + 1 moves a value of kind, a type as cast() gives one, by one
    byte. It does for (), an integer constant's own type, for an integer of
    32 bits or more and for a pointer to a byte. A qualifier changes
    nothing."""
    words = [word for word in kind if word not in layout.QUALIFIERS]
    if words[-1:] == ['*']:
        return bool(set(words[:-1]) & BYTE) and set(words[:-1]) <= BYTE | SIGNS
    return set(words) <= INTEGER | SIGNS


def defined(text, kept):
    """What the C text defines each macro as, by name, as the set of what its
    #defines and #undefs give it. A #define without parameters whose body
    holds one integer constant, a number kept() takes, gives (value, kind):
    kind is the type cast() gives the body. Any other #define, and an
    #undef, gives None."""
    found = {}
    for _, name, macro in macro_directives(text):
        words = macro[1] if macro is not None and macro[0] is None else []
        values = [value for value in map(integer, words) if value is not None]
        found.setdefault(name, set()).add((values[0], cast(words)) if len(values) == 1 and kept(values[0]) else None)
    return found


def macros(units, texts, kept=past):
    """The macros each file of units reads, by path, as {name: values}.

    A unit is the files of one compile: its source, then each file it reads.
    texts holds each C file's text by path. A file's macros are those that
    any file of a unit holding it defines or undefines. values is the set of
    what those files define the macro as, as defined() gives each. Only the
    macros that some file of texts defines as a number kept() takes are
    given."""
    each = {path: defined(text, kept) for path, text in texts.items()}
    # compiled() reads only a macro whose values hold a number, so no other
    # need be gathered
    numeric = {name for found in each.values() for name, values in found.items() if values - {None}}
    found = {}
    for unit in units:
        made = {}
        for path in unit:
            for name, values in each.get(path, {}).items():
                if name in numeric:
                    made.setdefault(name, set()).update(values)
        for path in unit:
            for name, values in made.items():
                found.setdefault(path, {}).setdefault(name, set()).update(values)
    return found


def assembled(path, kept=past):
    """The numbers of the assembly at path that kept() takes, each as
    (value, place), place its path:line."""
    found = []
    for line, text in enumerate(content(path).decode(errors='replace').splitlines(), 1):
        for match in ASSEMBLY_NUMBER.finditer(ASSEMBLY_STRING.sub('""', text).split(';')[0]):
            value = int(match.group(1), 16) if match.group(1) else int(match.group(2))
            if kept(value):
                found.append((value, f'{path}:{line}'))
    return found


def compiled(path, text, macros, kept=past):
    """The numbers that kept() takes in text, the C of the file at path.
    Each is (value, place), place its path:line.

    The numbers are text's integer constants and its sums. A sum is
    MACRO + N or MACRO - N, where N is an integer constant and MACRO's
    values in macros hold a number. macros is the file's macros, as
    macros() gives them. A sum fails, naming its path:line, where the census
    takes no value for it:

    - where MACRO's values hold more than the number;
    - where JOINS_BEFORE holds the token before MACRO, or before a cast
      before it, or JOINS_AFTER the token after N;
    - where bytewise() doesn't take its type, or it has none.

    The sum's type is the one cast() gives MACRO's body, and a cast before
    MACRO replaces it. The sum has no type where cast() gives the body none,
    or where a ) before MACRO closes no cast to names and stars."""
    words, opening = tokens(text), None
    found = [(value, f'{path}:{line}') for value, line in ((integer(word), line) for word, line in words)
             if value is not None and kept(value)]
    for at, ((name, line), (sign, _), (number, _)) in enumerate(zip(words, words[1:], words[2:])):
        values = macros.get(name, set())
        offset = integer(number) if sign in ('+', '-') and values - {None} else None
        if offset is None:
            continue
        use = f'{path}:{line}: {name} {sign} {number}'
        if len(values) > 1:
            fail(f'{use}: a compile that reads {path} defines {name} otherwise too, so the census takes no value for '
                 'it')
        ((base, kind),) = values
        before = words[at - 1][0] if at else ''
        after = words[at + 3][0] if at + 3 < len(words) else ''
        if before == ')':
            # a cast to names and stars opens at the ( this ) closes, and the
            # token before it is the one before the sum
            if opening is None:
                opening = {close: opened for opened, close in enumerate(pairs([word for word, _ in words]))
                           if close is not None}
            opened = opening.get(at - 1, -1)
            is_cast = opened >= 0 and words[opened][0] == '(' and all(
                word == '*' or NAME.fullmatch(word) for word, _ in words[opened + 1:at - 1])
            kind = tuple(word for word, _ in words[opened + 1:at - 1]) if is_cast and kind is not None else None
            before = words[opened - 1][0] if is_cast and opened else ''
        if before in JOINS_BEFORE or after in JOINS_AFTER:
            fail(f'{use}: the {before if before in JOINS_BEFORE else after} beside it makes it no whole operand, so '
                 'the census takes no value for it')
        if kind is None or not bytewise(kind):
            fail(f'{use}: {name} there is neither an integer nor a pointer to a byte, as the census reads it, so it '
                 'takes no value for it')
        value = base + offset if sign == '+' else base - offset
        if kept(value):
            found.append((value, f'{path}:{line}'))
    return found


def dependencies(path, roots):
    """The .h, .inc and .c files the .d file at path names under the
    directories roots, each once. Each is as os.path.normpath() spells it,
    so ./src/a.h and src/b/../a.h are both src/a.h. A file that opens with
    no rule's TARGET: fails."""
    words = content(path).decode().replace('\\\n', ' ').split()
    if not words or not words[0].endswith(':'):
        fail(f'{path}: not a .d file: TARGET: PREREQUISITE ...')
    named_files = (os.path.normpath(word.rstrip(':')) for word in words[1:])
    return list(dict.fromkeys(name for name in named_files if name.endswith(('.h', '.inc', '.c'))
                              and name.startswith(tuple(f'{root}/' for root in roots))))


def flat(units):
    """The files of units, each once, in order."""
    return list(dict.fromkeys(path for unit in units for path in unit))


def shc_files(sources):
    """The files 1ST_READ.BIN's SHC build reads, as (units, generated).
    sources are those 1ST_READ.BIN's splits.txt places.

    units holds a unit, as macros() takes it, for each source other than a
    gen/ range. A unit holds the source, then for a C source each file under
    SOURCE_ROOTS that its .d file names. generated holds the C under build/
    of each gen/ range, and each file under build/gen/ that a full build's
    .d file names. A C source with no .d file fails."""
    units, generated = [], {}
    for source in sources:
        if GENERATED.fullmatch(source):
            generated[str(Path('build', source))] = None
            continue
        units.append([source])
        if not source.endswith('.c'):
            continue
        full, stub = (Path(root, source).with_suffix('.d') for root in ('build', 'build/stub'))
        built = [path for path in (full, stub) if path.exists()]
        if not built:
            fail(f'{source}: no .d file under build/ or build/stub/, so build it first')
        # the newer .d file names the files the source's compile reads now
        units[-1] += dependencies(max(built, key=lambda path: path.stat().st_mtime_ns), SOURCE_ROOTS)
        if full.exists():
            generated.update(dict.fromkeys(dependencies(full, ('build/gen',))))
    return units, list(generated)


def candidates(units, generated):
    """The candidates of 1ST_READ.BIN's SHC build, by file. Each file's are
    a list of (value, place), place its path:line.

    They are the numbers past the image of each file of units and of
    generated, and of symbols.txt. A .src file's are those assembled()
    gives. A C file's are those compiled() gives, reading both sides of
    each guard, with the macros SHC's side defines. A file of generated
    reads no macro."""
    paths = flat(units)
    texts = {path: code(path) for path in paths if not path.endswith('.src')}
    known = macros(units, {path: code(path, compiler=SHC) for path in texts})
    found = {path: compiled(path, texts[path], known.get(path, {})) if path in texts else assembled(path)
             for path in paths}
    found.update({path: compiled(path, code(path), {}) for path in generated})
    # rows() gives each row's path:line, and named() its address, failing on
    # a row out of form
    found[str(SYMBOLS)] = [(value, place) for (place, _), (value, _) in zip(rows(SYMBOLS), named(SYMBOLS))
                           if past(value)]
    return found


def gcc_files(name, added):
    """The files the GCC side of image name reads, as (units, assembly).
    added is the C in gcc/ that the image links after its own sources.

    units holds a unit, as macros() takes it, for each C source GCC compiles
    for the image. Those are each C source its splits.txt places other than
    a gen/ range, and the C in gcc/ that replaces each .src file it places.
    The sources of added come last. A unit holds the source, then each file
    under SOURCE_ROOTS that its .d file names. assembly holds each .src file
    that nothing replaces. A .d file make gcc hasn't written fails."""
    compiles, assembly = [], []
    for source in Config(name).sources():
        replaced = Path('gcc', source).with_suffix('.c')
        if GENERATED.fullmatch(source):
            continue
        if source.endswith('.src') and not replaced.exists():
            assembly.append(source)
            continue
        compiles.append((str(replaced) if source.endswith('.src') else source,
                         gcc_object(source, name).with_suffix('.d')))
    compiles += [(source, Path('build', source).with_suffix('.d')) for source in added]
    units = []
    for source, depends in compiles:
        if not depends.exists():
            fail(f'{depends}: missing, and make gcc writes it')
        units.append([source, *dependencies(depends, SOURCE_ROOTS)])
    return units, assembly


def gcc_candidates(added):
    """The candidates of every image's GCC side, by file, as candidates()
    gives them, with a list for each file gcc_files() gives. added gives
    each image's added sources, by image.

    A C file's candidates are those compiled() gives on GCC's side of each
    guard, with the macros GCC's side defines. A .src file's are those
    assembled() gives. Each is a number for RAM, in any of VIEWS, whose
    offset into RAM is that of an address some image's symbols.txt gives a
    name."""
    units, assembly = [], []
    for name in images():
        image_units, image_assembly = gcc_files(name, added.get(name, ()))
        units += image_units
        assembly += image_assembly
    texts = {path: code(path, compiler=GCC) for path in flat(units)}
    # a macro's number may be any address of RAM, since a sum may reach a
    # name's address from one that isn't
    known = macros(units, texts, ram)
    name_offsets = {value % RAM for name in images() for value, _ in named(CONFIG / name / 'symbols.txt')
                    if ram(value)}

    def at_name(value):
        return ram(value) and value % RAM in name_offsets

    found = {path: compiled(path, text, known.get(path, {}), at_name) for path, text in texts.items()}
    found.update({path: assembled(path, at_name) for path in assembly})
    return found


def unplaced():
    """A line for each word of a motion file that holds an address in
    1ST_READ.BIN's image where its content places no pointer, as
    tools/content.py's pointer_words() gives them. The address may lie in
    any of VIEWS. A word is 32 bits, little-endian and 4-aligned in the
    file. A motion file make gcc hasn't decoded to build/gen/img/ fails,
    naming it."""
    lines, originals = [], files()
    for name in motions():
        original = originals[name]
        decoded = Path('build/gen/img', Path(original).with_suffix('.BIN').name)
        if not decoded.exists():
            fail(f'{decoded}: missing, and make gcc decodes it from orig/{original} with build/prskit')
        payload = content(decoded)
        words = struct.iter_unpack('<I', payload[:len(payload) // 4 * 4])
        hits = [(4 * index, word) for index, (word,) in enumerate(words) if ram(word) and word % RAM in IMAGE]
        if not hits:
            continue
        base, pointers = Config(name).base, pointer_words(name, decoded)
        lines += [f"orig/{original}: the word at offset {at:#x} holds {hexa(word)}, an address in 1ST_READ.BIN's "
                  "image, and its content places it as no pointer"
                  for at, word in hits if base + at not in pointers]
    return lines


def misplaced(table, sources, added):
    """A line for each fault of table's rows of symbols.txt against make
    gcc's objects of 1ST_READ.BIN. sources are those 1ST_READ.BIN's
    splits.txt places, and added is the C in gcc/ that 1ST_READ.BIN links
    after its own sources. The faults are:

    - a placed row at an address symbols.txt gives no name;
    - a name symbols.txt gives at a placed row's address that the GCC object
      of the row's source doesn't define global;
    - a row of symbols.txt, not placed, whose span holds a name that a GCC
      object defines global. The objects are those of sources, the
      content's among them, and of added.

    An object a row needs that make gcc hasn't built fails, naming it."""
    names, lines, objects = {}, [], {}
    for value, name in named(SYMBOLS):
        names.setdefault(value, []).append(name)

    def global_names(obj):
        if obj not in objects:
            if not obj.exists():
                fail(f'{obj}: missing, and make gcc builds it')
            objects[obj] = exports(elf.read(content(obj), obj))
        return objects[obj]

    linked = list(dict.fromkeys([*(gcc_object(source, FIRST.name) for source in sources),
                                 *(Path('build', source).with_suffix('.o') for source in added)]))
    for row in table:
        if row.disposition == 'placed':
            obj = gcc_object(row.source, FIRST.name)
            exported = global_names(obj)
            if row.start not in names:
                lines.append(f'{row.at}: {SYMBOLS} gives {hexa(row.start)} no name')
            lines += [f'{row.at}: {obj} defines no global {name}, which {SYMBOLS} gives {hexa(row.start)}'
                      for name in names.get(row.start, []) if name not in exported]
        elif row.file == str(SYMBOLS):
            for value in sorted(value for value in names if row.start <= value < row.end):
                for name in names[value]:
                    obj = next((obj for obj in linked if name in global_names(obj)), None)
                    if obj:
                        lines.append(f'{row.at}: {obj} defines global {name}, which {SYMBOLS} gives {hexa(value)}, '
                                     f"so the link places it, and the row's disposition is {row.disposition}, not "
                                     'placed')
    return lines


def problems(table, found, counted):
    """A line for each candidate of found, as candidates() gives them, that
    no row of table in its file explains. A line too for each row of
    counted whose COUNT isn't the number of them it explains."""
    lines, counts = [], Counter()
    for file, hits in sorted(found.items()):
        own = [row for row in table if row.file == file]
        for value, place in hits:
            row = next((row for row in own if row.start <= value < row.end), None)
            if row is None:
                lines.append(f'{place}: {hexa(value)}, which no row of {CENSUS} explains')
            else:
                counts[row] += 1
    for row in counted:
        if not counts[row]:
            lines.append(f'{row.at}: {row.file} holds no number the row spans')
        elif counts[row] != row.count:
            lines.append(f'{row.at}: the row counts {row.count} of the numbers it spans, and {row.file} holds '
                         f'{counts[row]}')
    return lines


def main(argv):
    parser = argparse.ArgumentParser(prog='census', description=__doc__.split('\n\n')[0])
    parser.add_argument('--generated', action='store_true',
                        help="read 1ST_READ.BIN's generated data too, which make writes from the disc")
    parser.add_argument('--disc', action='store_true',
                        help="check the motion files' words that hold an address in 1ST_READ.BIN's image too")
    parser.add_argument('--gcc', action='store_true',
                        help="check the census against make gcc's objects and every image's GCC side instead")
    parser.add_argument('--add', metavar=('IMAGE', 'SOURCE'), nargs=2, action='append', default=[],
                        help='with --gcc, a C source in gcc/ that IMAGE links after its own, which --gcc reads as '
                             "IMAGE's GCC side; each --add gives one")
    args = parser.parse_args(argv)
    if args.gcc and (args.generated or args.disc):
        parser.error('--generated and --disc add to the census without --gcc, so --gcc takes neither')
    if args.add and not args.gcc:
        parser.error('--add gives a GCC side a source, which --gcc alone reads')
    added = {}
    for name, source in args.add:
        added.setdefault(name, []).append(source)
    known = images()
    unknown = [name for name in added if name not in known]
    if unknown:
        fail(f'{", ".join(unknown)}: no such image, a directory in {CONFIG}/')
    sources = Config(FIRST.name).sources()
    table = census(sources)
    units, generated = shc_files(sources)
    shc_paths = {str(SYMBOLS), *flat(units)}
    # the rows the census without --gcc counts: each past the image, of a
    # file it reads or under build/gen/. --gcc counts the rest
    shc_rows = {row for row in table
                if past(row.start) and (row.file in shc_paths or row.file.startswith('build/gen/'))}
    if args.gcc:
        lines = misplaced(table, sources, added.get(FIRST.name, ()))
        found = gcc_candidates(added)
        gcc_rows = [row for row in table if row not in shc_rows]
        lines += problems(table, found, [row for row in gcc_rows if row.file in found])
        lines += [f"{row.at}: the row is --gcc's, and no image's GCC side reads {row.file}" for row in gcc_rows
                  if row.file not in found]
    else:
        found = candidates(units, generated if args.generated else [])
        lines = problems(table, found, [row for row in table if row in shc_rows
                                        and (args.generated or not row.file.startswith('build/gen/'))])
        if args.disc:
            lines += unplaced()
    if lines:
        fail(*lines)


if __name__ == '__main__':
    main(sys.argv[1:])
