"""Write the C that SHC compiles to the data the build takes from the disc.

    python3 tools/data.py IMAGE SOURCE OUT
    python3 tools/data.py --bodies IMAGE SOURCE LISTING OUT CONFIG
    python3 tools/data.py --texts SOURCE OUT
    python3 tools/data.py --values SOURCE OUT

IMAGE is an image's file, uncompressed. CONFIG names config/CONFIG/. A
command with IMAGE and no CONFIG takes IMAGE's file name, lowercased and less
its suffix, as CONFIG.

With no option, the tool writes OUT, the C of SOURCE's gen/ range of IMAGE,
OUT's .syms, and its .c.d, make's rule for OUT from the sources and headers
it reads types from. The C defines each definition config/CONFIG/data.txt
gives the range: a kind as tools/image.py lays it out, and a C type as the
image's declarations do, with the header that defines it. It defines them
in D, in DHEAD under #pragma section HEAD, or const in C, by the section the
range's row lies in. Where the data points at an address outside the range
that the tables don't name, the C makes up a name for it, and OUT's .syms
gives each such name's address.

--bodies writes these files in OUT, the .inc files from the bytes IMAGE
holds where LISTING, the stub build's listing, places SOURCE's data in
SOURCE's rows:

- OUT/NAME.inc for each body SOURCE declares;
- OUT/text.inc for SOURCE's texts;
- OUT/values.inc for SOURCE's values, each the number IMAGE holds in the
  member or element the value stands for in an initializer of SOURCE;
- OUT/bodies.txt, each body's, text's and value's name, address, size and
  type.

A name outside SOURCE that a body points at takes its type from the extern
declarations of SOURCE and its headers, which must agree. A function a body
points at must be declared there. --bodies reads the types SOURCE and its
headers declare without a preprocessor: it ignores conditionals, and reads
every branch alike.

--texts writes OUT, the stub build's text.inc: a distinct placeholder for
each of SOURCE's texts. --values writes OUT, the stub build's values.inc: 0
for each of SOURCE's values.

TOOLCHAIN.md#declared-data gives the rules a body keeps.
"""

import argparse
import re
import sys
from bisect import bisect_left
from collections import Counter
from functools import cached_property
from pathlib import Path

import config
from cdata import (BODY_KINDS, EXTENT, SCALARS, Bodies, EmitError, Items, Writer, counted, element_size, quote,
                   unqualified)
from csource import INCLUDE_ROOTS, included, matches, parse, uncommented
from image import MESHSET, Declared, Image, Item, Laid, ReadError, defined, window
from images import ALONE, FIRST, FIRST_BASE, FIRST_END, FIRST_GAME_CODE, FIRST_SDK_CODE
from layout import QUALIFIERS, Array, LayoutError, Pointer, Scalar, Types, spelled
from tool import content, fail, publish


# image.py reads the image's data, and cdata.py writes it as C. Neither
# reads a file or the tree's config: the commands read the files and the
# tables, and hand them the bytes, items and declarations they take.
# A command fails through tool.py's fail, naming the file at fault, as main()
# does on an error either raises or on a file a command can't read or write.


FIRST_CODE = tuple(sorted(FIRST_GAME_CODE + FIRST_SDK_CODE))
# the sections a gen/ row can lie in. Each gives the name a row's C passes
# to #pragma section there, or None for no pragma. It also gives whether
# that C defines its data const. SHC writes data that isn't const to D, or
# to DHEAD under #pragma section HEAD, and const data to C
GEN_SECTIONS = {'D': (None, False), 'DHEAD': ('HEAD', False), 'C': (None, True)}
# an extern declaration of a type named by one identifier, or struct and a
# tag, const or not, with one extent or none: its const, type, name and
# extent, the declarations a body can point at. A declaration of a pointer
# or a volatile, or one with two extents, matches none, so a body can't
# point at such a name outside the source
BODY_EXTERN = re.compile(r'^extern\s+(const\s+)?((?:struct\s+)?\w+)\s+(\w+)\s*(\[[^]]*\])?\s*;', re.M)
# a function declaration at the start of a line: its name. A body can
# point at a function the source or its headers declare, as a path's step
# does. The scan reads no parameter's type, so void f(); declares f too. A
# parameter can be a function pointer, whose parentheses nest one deep
DECLARED_FUNCTION = re.compile(r'^(?:extern\s+)?[A-Za-z_][\w \t]*?[\s*](\w+)\((?:[^;{()]|\([^;{()]*\))*\);', re.M)
# the bytes in a listing directive's .B, .W or .L unit
UNITS = {'B': 1, 'W': 2, 'L': 4}
# the banner asmsh's listing opens with, after a form feed
ASSEMBLED = re.compile(r'\f?\*\*\* SuperH RISC engine ASSEMBLER')
# a declaration's qualifiers
QUALIFIED = r'((?:(?:const|volatile)\s+)*)'
# a file's datum or a function's static whose initializer is a body, an
# included .inc: its qualifiers, type, pointer's *, name, extents and include
BODY = re.compile(r'^(?:[ \t]+static\s+|(?:static\s+)?)' + QUALIFIED + r'((?:struct\s+)?\w+)(?:\s*(\*)\s*|\s+)'
                  r'(\w+)\s*((?:\[[^]\n]*\])*)\s*=\s*\{\n#include "([^"]+)"\n[ \t]*\};', re.M)
# a function's automatic array whose initializer is a body too: its
# qualifiers, type, name, extents and include. Its body is the template SHC
# copies the array from
AUTO = re.compile(r'^[ \t]+(?!static\b)' + QUALIFIED + r'((?:struct\s+)?\w+)\s+(\w+)\s*((?:\[[^]\n]*\])+)\s*=\s*'
                  r'\{\n#include "([^"]+)"\n[ \t]*\};', re.M)
# an included body
INCLUDE = re.compile(r'^#include "([^"]+\.inc)"$', re.M)
# a declaration that defines a union: the union's members and the datum's name
UNION = re.compile(r'^(?:static\s+)?(?:(?:const|volatile)\s+)*union\s*\w*\s*\{\n(.*?)^\}\s*(\w+)\s*;', re.M | re.S)
# a union's first member, where it is an array: its element's type and its name
MEMBER = re.compile(r'\s*((?:struct\s+)?\w+)\s+(\w+)\s*\[')
# a file's declaration that defines a struct of no tag: the name of the datum
# it declares, where the datum is no array
UNTAGGED = re.compile(r'^(?:static\s+)?(?:(?:const|volatile)\s+)*struct\s*\{\n'
                      r'(?:(?!\}).*\n)*\}[ \t]*(\w+)[ \t]*[=;]', re.M)
# a function definition, from the start of its line to its brace: its name
FUNCTION = re.compile(r'^(?:static\s+)?[A-Za-z_][\w \t*]*?\b(\w+)\([^;{]*\)\n\{', re.M)
# a file's declaration or a function's static: its qualifiers, type,
# pointer's *, name and extents
DECLARATION = re.compile(r'^(?:[ \t]+static\s+|(?:static\s+)?)' + QUALIFIED + r'((?:struct\s+)?\w+)\s*(\*?)\s*(\w+)\s*'
                         r'((?:\[[^]\n]*\])*)\s*[=;]', re.M)
# a text's length macro: the text's name, number and length
TEXT = re.compile(r'^#define (TEXT_([0-9]+))_LENGTH ([0-9]+)$', re.M)
# a value, VALUE_n, and its number n. A value stands alone for one arithmetic
# member or element in an initializer the source writes, and the full build's
# values.inc defines it as the number the disc holds there
VALUE = re.compile(r'\bVALUE_([0-9]+)\b')
# a definition whose initializer the source writes in braces, up to the
# brace: its qualifiers, type, name and extents
WRITTEN = re.compile(r'^[ \t]*(?:static\s+)?' + QUALIFIED + r'((?:struct\s+|union\s+)?\w+)\s+(\w+)\s*'
                     r'((?:\[[^]\n]*\])*)\s*=\s*(?=\{)', re.M)
# in a listing: a static's label, a datum's directive and its operands, the
# function a row of code begins, the labels a row names, and a row of an L
# label alone, as an unnamed datum or a string starts with. A .SDATA's
# operand is its string in quotes, which can hold a ;. Any other directive's
# operands run to the ; that starts a comment
STATIC = re.compile(r'\w+: +; static: (\w+)')
QUOTED = re.compile(r'"(?:[^"]|"")*"')
DIRECTIVE = re.compile(r'(\.(?:DATA|DATAB|RES|SDATA)(?:\.[BWL])?) +(' + QUOTED.pattern + '|[^;]*)')
# a .DATAB's or .RES's count, and a byte of .DATA.B or .DATAB.B, as SHC and
# asmsh write them
COUNT = re.compile(r'[0-9]+')
BYTE = re.compile(r"H'([0-9A-F]{1,2})|([0-9]{1,3})")
BEGINS = re.compile(r'; function: (\w+)')
LABELS = re.compile(r'\bL[0-9]+\b')
UNNAMED = re.compile(r'L[0-9]+: *')
# in SHC's listing: a section's size, a row's section, offset and the rest,
# a row that goes on with the operands above it, and the line marker
# -optimize=0 writes ahead of a section's first code, which gives the
# section with no offset
SHC_SECTION = re.compile(r'\w+ +SECTION \((\w+)\): +([0-9A-F]+) Byte')
SHC_ROW = re.compile(r'(...) ([0-9A-F]{8}) [0-9A-F<> ]{10} *(.*)$')
SHC_MORE = re.compile(r' {13}[0-9A-F<> ]{10} *\+ +([^;]*)')
SHC_MARKER = re.compile(r'(\w{1,3}) +; ')
# in SHC's listing, the label of a global whose name has 31 characters or
# more, and its comment. The label runs past the comment's column, so the
# comment goes on the next line. compiled() joins the two, and leaves a blank
# line in the comment's place, so that each line keeps its number
SHC_WRAPPED = re.compile(r'^(.* _\w+:)\n +(; (?:static|function): \w+)$', re.M)

# each command's command line, which -h and misuse print after "usage: "
USAGE = '''%(prog)s IMAGE SOURCE OUT
       %(prog)s --bodies IMAGE SOURCE LISTING OUT CONFIG
       %(prog)s --texts SOURCE OUT
       %(prog)s --values SOURCE OUT'''


class Config(config.Config):
    """config.Config, with the first name at each address, the image as an
    Image, and the image's Declarations."""

    def __init__(self, name):
        super().__init__(name)
        self.names = {}
        for address, symbol in self.symbols():
            self.names.setdefault(address, symbol)

    def image(self, path):
        """The image at path as an Image, with the code ranges and fill byte
        config.py gives it. In an image other than 1ST_READ.BIN, a word is
        taken for an address where its value lies in window(base). An image
        other than 1ST_READ.BIN and those in ALONE points into 1ST_READ.BIN
        too. Its Image then also takes a value in 1ST_READ.BIN's range for an
        address, and 1ST_READ.BIN's code ranges join its own. 1ST_READ.BIN's
        Image takes a value for an address only in its own range, or where it
        is an import's address, as config.imports() gives it. The images
        beside it share the rest of RAM, so a value there names no one
        definition by itself."""
        game, sdk = self.code()
        code, windows, imports = (*game, *sdk), (window(self.base),), frozenset()
        if self.path == FIRST:
            windows = ((FIRST_BASE, FIRST_END),)
            imports = frozenset(address for _, _, _, address, _ in config.imports())
        elif self.path not in ALONE:
            code += FIRST_CODE
            windows += ((FIRST_BASE, FIRST_END),)
        return Image(str(path), content(path), self.base, code, windows, self.fill, imports)

    @cached_property
    def declarations(self):
        """The image's Declarations, read once a row's type needs them."""
        return Declarations(self)


class Declarations:
    """The C sources an image links, and the headers they include,
    transitively, as included() finds them, with the Types of them all,
    which lay out the types data.txt gives. Only the image's own sources
    count, as another image's can give a type of the same name another
    layout."""

    def __init__(self, config):
        self.texts = {}
        for source in config.sources():
            if source.endswith('.c') and not source.startswith('gen/'):
                included(Path(source), self.texts)
        self.paths = list(self.texts)  # the texts' paths, by their file's index in the Types
        self.types = Types(list(self.texts.values()))

    def header(self, ctype):
        """The header to include for the struct or union ctype names, past its
        qualifiers and stars: the least path of the texts that define it, as
        its path below the first of INCLUDE_ROOTS that holds it. None for a
        type that is no struct or union, one no text defines, or one a header
        under sdk/include defines: of those headers the C includes ninja.h
        alone. A type whose least path is a .c file fails, naming the file."""
        _, base = unqualified(ctype.split('*')[0].strip())
        if not base.startswith(('struct ', 'union ')):
            return None
        kind, tag = base.split()
        files = {self.paths[file] for file, _, _ in self.types.definitions.get((kind, tag), ())}
        if not files:
            return None
        path = min(files)
        if path.suffix != '.h':
            fail(f'{path}: defines {base}, which data.txt gives, and only a header can give it')
        root = next((Path(root) for root in INCLUDE_ROOTS if Path(root) in path.parents), None)
        return path.relative_to(root).as_posix() if root and root != Path('sdk/include') else None

    def headers(self, items):
        """The headers to include for items' Laid types, as header() gives them."""
        return {self.header(item.ctype) for item in items if isinstance(item, Laid)} - {None}


def definitions(config, image, within=None):
    """The items of the definitions config's data.txt gives, read from
    image, and with within only those that start in it. An item is of one of
    the kinds tools/image.py lays out, or else Laid, of a type the image's
    declarations lay out. Its vouched is its row's path:line where the row
    vouches, and None otherwise. Fails, naming the row, where neither lays
    out its kind, where image holds no text at a text's row, or where a row
    vouches for a kind other than pointers or mixed, or a C type."""
    items = []
    for start, kind, count, vouched, where in config.definitions():
        if within and not within[0] <= start < within[1]:
            continue
        try:
            item = defined(image, start, kind, count)
        except ReadError as error:
            fail(f'{where}: {error}')
        if item is None:
            try:
                layout = config.declarations.types.layout(kind)
            except LayoutError as error:
                fail(f"{where}: {kind}: no kind, and no type the image's declarations lay out: {error}")
            item = Laid(start, start + layout.size * (count or 1), layout, kind, count is not None, ())
        elif vouched and item.kind not in ('pointers', 'mixed'):
            fail(f'{where}: {kind}: vouched, and only pointers, mixed or a C type holds words a row vouches for')
        item.vouched = where if vouched else None
        items.append(item)
    return items


def generate(path, source, out):
    """Write OUT, the C of SOURCE's gen/ range of IMAGE, OUT's .syms and its
    .c.d.

    Fails where:

    - the tables give SOURCE no row, or several;
    - SOURCE's row lies in a section GEN_SECTIONS doesn't name, starts off 8
      bytes, or lies outside the image;
    - data.txt gives the range no row, or definitions() fails on a row;
    - the emitter raises EmitError, as Items does for a name no item can
      start. The failure names SOURCE."""
    out = Path(out)
    config = Config(Path(path).stem.lower())
    ranges = config.ranges(source)
    if len(ranges) != 1:
        fail(f'{source}: needs one row in {config.splits}')
    (section, low, high), = ranges
    if section not in GEN_SECTIONS:
        fail(f'{source}: its row lies in {section}, and the C it takes defines data for D, DHEAD or C')
    if low % 8:
        fail(f'{source}: R32 starts its data on 8 bytes, and 0x{low:08x} is {low % 8} past')
    image = config.image(path)
    if not image.inside(low, high - low):
        fail(f'{source}: 0x{low:08x} to 0x{high:08x} lies outside {path}, 0x{image.base:08x} to 0x{image.end:08x}')
    items = definitions(config, image, (low, high))
    if not items:
        fail(f'{source}: no row of {config.path / "data.txt"} lies in its range')
    named = {address: name for address, name in config.names.items() if low <= address < high}
    # the texts the rows of C types read their types from, and the headers
    # that define those types
    typed = any(isinstance(item, Laid) for item in items)
    read = sorted(path.as_posix() for path in config.declarations.texts) if typed else []
    headers = config.declarations.headers(items) if typed else ()

    pragma, constant = GEN_SECTIONS[section]
    try:
        emitted = Writer(Items(image, items, [(low, high)], named), config.names, constant).source(
            f'{Path(path).name} from 0x{low:08x} to 0x{high:08x}, as tools/data.py reads it', pragma, headers)
    except EmitError as error:
        fail(f'{source}: {error}')
    known = set(config.names.values())
    placeholders = sorted((address, name) for name, address in emitted.outside.items() if name not in known)
    # the .syms and .c.d first, so that the C is no older than either only
    # once all three are written
    publish(out.with_suffix('.syms'), ''.join(f'0x{address:08x}  {name}\n' for address, name in placeholders))
    publish(out.with_suffix('.c.d'), f'{out}:' + ''.join(f' {each}' for each in read) + '\n'
            + ''.join(f'{each}:\n' for each in read))
    publish(out, emitted.text)


def externs(texts, pattern):
    """Each extern declaration that pattern reads in texts, each a file's
    path and its text: its name, (its type, its extent), and where it is,
    as path:line: the declaration. The extent is the text between its
    brackets, or None where it has none. Comments and literals cannot supply
    declarations; locations refer to the original source lines."""
    for path, text in texts:
        text = uncommented(text)
        line, at = 1, 0
        for match in matches(pattern, text):
            line, at = line + text.count('\n', at, match.start()), match.start()
            const, ctype, name, dims = match.groups()
            extent = None if dims is None else dims[1:-1].strip()
            yield name, (('const ' if const else '') + ctype, extent), f'{path}:{line}: {match.group()}'


def agreed(declarations):
    """Each name's type and extent, from declarations as externs() yields
    them. A name has an extent, and is an array, only where all its
    declarations give one, and its extent is the first that isn't empty. Two
    types for one name fail, naming each declaration that conflicts with the
    first."""
    types, extents, problems = {}, {}, []
    for name, (ctype, extent), where in declarations:
        first, there = types.setdefault(name, (ctype, where))
        if ctype != first:
            problems.append(f'{where} conflicts with {there}')
        held = extents.get(name, '')
        extents[name] = None if held is None or extent is None else held or extent
    if problems:
        fail(*problems)
    return {name: (ctype, extents[name]) for name, (ctype, _) in types.items()}


def full_section(where, given, sizes, abbreviated=True):
    """The one section of a listing that a row gives: the one whose name
    starts with the row's three letters, or, where the row's name isn't
    abbreviated, the one of that name. Fails, naming where, the row's path
    and line, where no section or several do."""
    full = [name for name in sizes if (name[:3] if abbreviated else name) == given]
    if len(full) != 1:
        fail(f'{where}: no one section {"starts" if abbreviated else "is named"} {given}')
    return full[0]


def sdata(operand, where):
    """The bytes of a .SDATA's operand, as DIRECTIVE reads it: the string in
    its quotes, each doubled quote one quote. listing() sizes a string by
    them, and strings() takes them. Fails, naming where, the row's path and
    line, on an operand that isn't one whole string."""
    if not QUOTED.fullmatch(operand.strip()):
        fail(f'{where}: .SDATA {operand.strip()}: no whole string')
    return operand.strip()[1:-1].replace('""', '"').encode('latin-1')


def directive_count(operands, where):
    """The count a .DATAB's or .RES's operands start with. Fails, naming
    where, the row's path and line, on one that isn't COUNT's."""
    found = operands.split(',')[0].strip()
    if not COUNT.fullmatch(found):
        fail(f'{where}: {operands.strip()}: its count isn\'t a decimal number')
    return int(found)


def byte(value, where):
    """The byte a .DATA.B's or .DATAB.B's value gives. Fails, naming
    where, the row's path and line, on one that isn't BYTE's or is past
    255."""
    found = BYTE.fullmatch(value.strip())
    if found and found.group(1):
        return int(found.group(1), 16)
    if not found or int(found.group(2)) > 255:
        fail(f'{where}: {value.strip()}: no byte')
    return int(found.group(2))


def listing(path):
    """From the stub's listing at path, SHC's or asmsh's for a patched source:

    - each named datum's section, offset and size, by its name;
    - the same for each unnamed datum outside the code, as an automatic
      array's template is, by its L label;
    - the L labels named in each function's code and pools, by the function;
    - the names two statics share.

    SHC rows abbreviate section names to three letters; its section table
    gives the full names. asmsh's .SECTION directives give full names,
    which are kept. A listing is read one byte to a character.
    Both listings end each line in CRLF, which is read as LF, the line end
    SHC_WRAPPED matches, and only LF ends a line. A .SDATA's string keeps
    its semicolons and doubled quotes, and a comment starts only past it.

    Fails, naming the row's path and line, where the row names no one
    section, where a datum runs past its section, or where a .SDATA's
    operand isn't one whole string or a .DATAB's or .RES's count isn't a
    decimal number. A listing that gives no section's size fails, naming
    it."""
    text = content(path).decode('latin-1').replace('\r\n', '\n')
    abbreviated = not ASSEMBLED.match(text)
    data, unnamed, sizes, loads = (compiled if abbreviated else assembled)(text, path)
    if not sizes:
        fail(f'{path}: gives no section\'s size, as SHC\'s and asmsh\'s listings do')
    named, templates = {}, {}
    for found, held in ((data, named), (unnamed, templates)):
        for section, name, start, end, line in found:
            full = full_section(f'{path}:{line}', section, sizes, abbreviated)
            if end > sizes[full]:
                fail(f'{path}:{line}: {name} runs past its section, {full}, of {sizes[full]} bytes')
            held[name] = full, start, end - start
    # two functions' statics can share a name
    twice = {name for name, count in Counter(datum[1] for datum in data).items() if count > 1}
    return named, templates, loads, twice


def compiled(text, path):
    """listing()'s named data, unnamed data, section sizes and loads, from
    SHC's listing at path. A datum is its section's letters, name, start,
    end and line."""
    data, sizes, section, datum, directive = [], {}, None, None, None
    unnamed, loads, function = [], {}, None
    text = SHC_WRAPPED.sub(r'\1 \2\n', text)

    def close():
        """Extend the datum over the last directive, its continuation lines
        too."""
        if datum is None or directive is None:
            return
        offset, op, operands, where = directive
        if op.startswith('.RES.') and datum[3] > datum[2]:
            # a .RES after the datum's values is SHC's padding before the next
            # datum
            return
        if op.startswith('.SDATA'):
            size = len(sdata(operands, where))
        elif op.startswith(('.DATAB.', '.RES.')):
            size = UNITS[op[-1]] * directive_count(operands, where)
        else:
            size = UNITS[op[-1]] * len([value for value in operands.split(',') if value.strip()])
        datum[3] = max(datum[3], offset + size)

    for number, line in enumerate(text.split('\n'), 1):
        # the section, the offset, the code and the rest, in their columns
        info = SHC_SECTION.match(line)
        row = SHC_ROW.match(line)
        more = SHC_MORE.match(line)
        marker = SHC_MARKER.match(line)
        if info:
            sizes[info.group(1)] = int(info.group(2), 16)
        elif more and directive is not None:
            directive = (directive[0], directive[1], directive[2] + ',' + more.group(1), directive[3])
        elif marker:
            close()
            directive, datum = None, None
            section = marker.group(1)
        elif row:
            close()
            directive = None
            section = row.group(1).strip() or section
            offset, rest = int(row.group(2), 16), row.group(3)
            label = STATIC.match(rest)
            op = DIRECTIVE.match(rest)
            in_code = section is not None and section.startswith('P')
            begins = BEGINS.search(rest)
            if in_code:
                function = begins.group(1) if begins else function
                loads.setdefault(function, set()).update(LABELS.findall(rest))
            if label:
                datum = [section, label.group(1), offset, offset, number]
                data.append(datum)
            elif UNNAMED.fullmatch(rest) and not in_code:
                datum = [section, rest.strip()[:-1], offset, offset, number]
                unnamed.append(datum)
            elif op and datum is not None:
                directive = (offset, op.group(1), op.group(2), f'{path}:{number}')
            else:
                # code, alignment, or a label of no datum's: the datum has ended
                datum = None
    close()
    return data, unnamed, sizes, loads


def assembled(text, path):
    """listing()'s named data, unnamed data, section sizes and loads, from
    asmsh's listing at path, as compiled() gives them, but with each
    section's full name.

    A row is its line number, the offset, the code, and the source's line
    number and text. A row of an offset and code alone goes on with the code
    above it. The listing cuts the source's long lines short, so a datum's
    values are measured by their code. .SECTION starts a section, and the
    section list at the end gives the sizes."""
    data, sizes, section, datum, values = [], {}, None, None, False
    unnamed, loads, function = [], {}, None
    for number, line in enumerate(text.split('\n'), 1):
        size = re.match(r'(\w+) +REL-(?:CODE|DATA) +([0-9A-F]+)', line)
        row = re.match(r' *\d+ ([0-9A-F]{8}) ([0-9A-F]*) +\d+ (.*)$', line)
        more = re.match(r' {6}([0-9A-F]{8}) ([0-9A-F]+) *$', line)
        if size:
            sizes[size.group(1)] = int(size.group(2), 16)
        elif more:
            if datum is not None and values:
                datum[3] = max(datum[3], int(more.group(1), 16) + len(more.group(2)) // 2)
        elif row:
            offset, code, rest = int(row.group(1), 16), row.group(2), row.group(3).strip()
            values = False
            begun = re.match(r'\.SECTION +(\w+)', rest)
            if begun:
                section, datum = begun.group(1), None
                continue
            label = STATIC.match(rest)
            op = DIRECTIVE.match(rest)
            in_code = section is not None and section.startswith('P')
            begins = BEGINS.search(rest)
            if in_code:
                function = begins.group(1) if begins else function
                loads.setdefault(function, set()).update(LABELS.findall(rest))
            if label:
                datum = [section, label.group(1), offset, offset, number]
                data.append(datum)
            elif UNNAMED.fullmatch(rest) and not in_code:
                datum = [section, rest[:-1].strip(), offset, offset, number]
                unnamed.append(datum)
            elif op and datum is not None and op.group(1).startswith(('.DATAB.', '.RES.')):
                # a .RES after the datum's values is SHC's padding before the
                # next datum
                if not (op.group(1).startswith('.RES.') and datum[3] > datum[2]):
                    count = directive_count(op.group(2), f'{path}:{number}')
                    datum[3] = max(datum[3], offset + UNITS[op.group(1)[-1]] * count)
            elif op and datum is not None:
                datum[3] = max(datum[3], offset + len(code) // 2)
                values = True
            else:
                # code, alignment, or a label of no datum's: the datum has ended
                datum = None
    return data, unnamed, sizes, loads


def strings(path):
    """Each string SHC's listing at path holds, as its section, offset and
    bytes. A string is a row of an L label alone outside the code, and the
    characters the .SDATA, .DATA.B and .DATAB.B rows below it give, each
    from where the one before it ends. The listing is read as listing()
    reads it. asmsh's listing, a patched source's, gives no strings, so a
    patched source can't take texts.

    Fails, naming the row's path and line, where a row gives a string's
    bytes in a form SHC doesn't write, or from an offset other than where
    the string's bytes so far end, or where a string runs past its
    section."""
    found, sizes, section, directive, string = [], {}, None, None, None

    def close():
        if directive is None:
            return
        offset, op, operands, where = directive
        if offset != string[1] + len(string[2]):
            fail(f'{where}: {op} starts at {offset:08X}, and its string\'s bytes end at '
                 f'{string[1] + len(string[2]):08X}')
        if op == '.SDATA':
            data = sdata(operands, where)
        elif op == '.DATA.B':
            data = bytes(byte(value, where) for value in operands.split(',') if value.strip())
        else:
            if operands.count(',') != 1:
                fail(f'{where}: .DATAB.B {operands.strip()}: not a count and a byte')
            data = bytes([byte(operands.split(',')[1], where)]) * directive_count(operands, where)
        string[2].extend(data)

    for number, line in enumerate(content(path).decode('latin-1').replace('\r\n', '\n').split('\n'), 1):
        info = SHC_SECTION.match(line)
        row = SHC_ROW.match(line)
        more = SHC_MORE.match(line)
        marker = SHC_MARKER.match(line)
        if info:
            sizes[info.group(1)] = int(info.group(2), 16)
        elif more and directive is not None:
            directive = (directive[0], directive[1], directive[2] + ',' + more.group(1), directive[3])
        elif marker:
            close()
            directive, string = None, None
            section = marker.group(1)
        elif row:
            close()
            directive = None
            section = row.group(1).strip() or section
            offset = int(row.group(2), 16)
            op = DIRECTIVE.match(row.group(3))
            in_code = section is not None and section.startswith('P')
            if UNNAMED.fullmatch(row.group(3)) and not in_code:
                string = [section, offset, bytearray(), number]
                found.append(string)
            elif op and op.group(1) in ('.SDATA', '.DATA.B', '.DATAB.B') and string is not None:
                directive = (offset, *op.groups(), f'{path}:{number}')
            else:
                string = None
    close()
    placed = []
    for short, offset, data, number in found:
        section = full_section(f'{path}:{number}', short, sizes)
        if offset + len(data) > sizes[section]:
            fail(f'{path}:{number}: its string runs past its section, {section}, of {sizes[section]} bytes')
        placed.append((section, offset, bytes(data)))
    return placed


def texts(source):
    """Each of source's texts in number order, as its name, number, length
    and the placeholder the stub build gives it.

    Fails, naming source, where:

    - the texts aren't numbered from 1 with none missing;
    - a text has no length;
    - more than 255 texts share one length."""
    text = uncommented(content(source).decode(errors='replace'))
    found = [(match.group(1), int(match.group(2)), int(match.group(3))) for match in matches(TEXT, text)]
    if sorted(number for _, number, _ in found) != list(range(1, len(found) + 1)):
        fail(f'{source}: its texts are numbered from 1, with none missing')
    if not all(length for _, _, length in found):
        fail(f'{source}: a text of no length')
    # SHC keeps one copy of equal strings, so no two texts may share a
    # placeholder. A placeholder is a byte and then zeros, so it differs from
    # every placeholder of another length and ends no other. The byte is the
    # text's number, or for a number past 255 the least byte no other text of
    # its length takes.
    taken = {}
    for _, number, length in found:
        if number <= 255:
            taken.setdefault(length, set()).add(number)
    placed = []
    for name, number, length in sorted(found, key=lambda text: text[1]):
        byte = number
        if number > 255:
            free = [byte for byte in range(1, 256) if byte not in taken.setdefault(length, set())]
            if not free:
                fail(f'{source}: more than 255 texts of length {length}')
            byte = free[0]
            taken[length].add(byte)
        placed.append((name, number, length, bytes([byte]) + bytes(length - 1)))
    return placed


def text_stubs(source, out):
    """Write OUT, the stub build's text.inc, which defines each of SOURCE's
    texts as its placeholder in one string literal. Fails where texts()
    does."""
    lines = []
    for name, _, length, placeholder in texts(Path(source)):
        zeros = '\\0' * (length - 1)
        lines.append(f'#define {name} "\\x{placeholder[0]:02x}{zeros}"\n')
    publish(Path(out), ''.join(lines))


def value_numbers(source):
    """The numbers of source's values, which run from 1 with none missing. A
    value stands alone for one number in an initializer the source writes,
    so the source writes each once and no macro of its own holds one.

    Fails, naming source, where:

    - a macro holds a value;
    - the source writes a value more than once;
    - the values aren't numbered from 1 with none missing."""
    code = uncommented(content(source).decode(errors='replace'), lines=False).replace('\\\n', '')
    if any(line.lstrip().startswith('#') and VALUE.search(line) for line in code.splitlines()):
        fail(f'{source}: a macro holds a value, which stands alone in an initializer')
    found = Counter(int(match.group(1)) for match in map(VALUE.fullmatch, spelled(code)) if match)
    again = sorted(number for number, count in found.items() if count > 1)
    if again:
        fail(f'{source}: it writes VALUE_{again[0]} {found[again[0]]} times, and a value is one number')
    if sorted(found) != list(range(1, len(found) + 1)):
        fail(f'{source}: its values are numbered from 1, with none missing')
    return sorted(found)


def value_stubs(source, out):
    """Write OUT, the stub build's values.inc: 0 for each of SOURCE's values,
    as a body's stub is. Fails where value_numbers() does."""
    publish(Path(out), ''.join(f'#define VALUE_{number} 0\n' for number in value_numbers(Path(source))))


def initialized(tokens, at, layout, offset, found, where):
    """The index past the initializer at tokens[at]. The object it
    initializes, the datum being read or a member of it, is laid out as
    layout and lies offset bytes into the datum.

    A value stands alone for one scalar. In a datum that holds a value, each
    aggregate takes its initializer in braces, or else C would fill it from
    the list around it. A string or a text can stand for a char array's.

    Each value the initializer writes goes into found as (name, offset,
    layout). The offset and layout are those of the scalar the value stands
    for, and the offset counts from the datum's start.

    Fails, naming where, on:

    - a value in an expression, or for a pointer or an aggregate;
    - more initializers than the braces hold, or a token where , or } ends
      one;
    - an aggregate without braces, but for a char array's string or text.

    Raises IndexError, which placed_values() catches, where the initializer
    doesn't close."""
    if tokens[at] == '{':
        at += 1
        if isinstance(layout, Array):
            members = [(layout.element, offset + index * layout.element.size) for index in range(layout.count)]
        elif isinstance(layout, (Scalar, Pointer)):
            members = [(layout, offset)]
        else:
            members = [(field, offset + place) for _, field, place in layout.fields[:1 if layout.union else None]]
        for member, place in members:
            if tokens[at] == '}':
                break
            at = initialized(tokens, at, member, place, found, where)
            if tokens[at] == ',':
                at += 1
            elif tokens[at] != '}':
                fail(f'{where}: {tokens[at]} where , or }} ends an initializer')
        if tokens[at] != '}':
            fail(f'{where}: more initializers than the {len(members)} its braces hold')
        return at + 1
    # an expression, to the , or } that ends it outside its brackets
    end, depth = at, 0
    while depth or tokens[end] not in (',', '}'):
        depth += tokens[end] in ('(', '[')
        depth -= tokens[end] in (')', ']')
        end += 1
    expression = tokens[at:end]
    held = [token for token in expression if VALUE.fullmatch(token)]
    if held:
        if len(expression) != 1:
            fail(f'{where}: {held[0]} stands alone for one number, not in {" ".join(expression)}')
        if not isinstance(layout, Scalar):
            kind = 'a pointer, which the source writes itself' if isinstance(layout, Pointer) else 'an aggregate'
            fail(f'{where}: {held[0]} stands for {kind}, and a value is one number')
        found.append((held[0], offset, layout))
    elif not isinstance(layout, (Scalar, Pointer)):
        text = len(expression) == 1 and (expression[0].startswith('"') or re.fullmatch(r'TEXT_[0-9]+', expression[0]))
        if not (isinstance(layout, Array) and isinstance(layout.element, Scalar) and layout.element.size == 1
                and text):
            fail(f'{where}: an aggregate whose initializer has no braces, beside a value')
    return end


def qualify(qualifiers, ctype):
    """ctype, after the qualifiers a declaration gives it, in QUALIFIERS' order: const, then volatile."""
    return ' '.join([*(word for word in QUALIFIERS if word in qualifiers.split()), ctype])


def headers(depend):
    """The .h files the first rule of depend names. depend is the dependency
    file the Makefile has the host compiler write for a stub build's
    source. Fails where that file is missing."""
    if not depend.exists():
        fail(f'{depend}: missing, and the stub build writes it')
    rule = depend.read_text().replace('\\\n', ' ').split('\n', 1)[0]
    return [Path(name) for name in rule.partition(':')[2].split() if name.endswith('.h')]


class BodySource:
    """A body source, and what --bodies reads with it: its stub build's
    listing and dependencies, the headers they name, and the image and
    tables its bodies come from. Its passes fail, naming the file at fault,
    on what they can't take."""

    def __init__(self, path, source, lis, conf):
        self.path, self.source, self.lis = path, Path(source), Path(lis)
        self.config = Config(conf)
        self.image = self.config.image(path)
        # the source's text as uncommented() gives it, which every pass reads,
        # each scan of it through matches()
        written = content(self.source).decode(errors='replace')
        self.text = uncommented(written)
        # the directory the source's includes name its bodies in: its path
        # less its top directory and .c
        self.directory = Path(*self.source.parts[1:]).with_suffix('').as_posix()
        self.places, self.templates, self.loads, self.twice = listing(self.lis)
        # the source's text and each header's that its stub build's dependency
        # file names. The Types read the headers, then the source as written,
        # whose comments Types reads as C does. What a body can point at
        # outside the source is a name they declare extern, or a function
        # they declare
        texts = [(self.source, written), *((header, content(header).decode(errors='replace'))
                                           for header in headers(self.lis.with_suffix('.d')))]
        self.types = Types([text for _, text in texts[1:]] + [written])
        self.outside = agreed(externs(texts, BODY_EXTERN))
        self.functions = {match.group(1) for _, text in texts
                          for match in matches(DECLARED_FUNCTION, uncommented(text))}
        # each section's rows, and its start: its first row's, where link.py
        # places the object's section, and so where the listing's offsets in
        # it start
        self.rows, self.starts = {}, {}
        for section, start, end in self.config.ranges(self.source.as_posix()):
            self.rows.setdefault(section, []).append((start, end))
            self.starts.setdefault(section, start)
        # each datum's type as a pointer to it takes it, qualifiers kept, and
        # its extents. A union or a struct of no tag that the source defines
        # as it declares the datum is taken as union or struct, and as no
        # array. unions gives each such union's first member where that is an
        # array, and None otherwise
        self.declarations = {name: (qualify(qualifiers, ctype) + (' *' if pointer else ''), EXTENT.findall(dims))
                             for qualifiers, ctype, pointer, name, dims in self.scan(DECLARATION)}
        self.unions = {name: MEMBER.match(members) for members, name in self.scan(UNION)}
        self.declarations.update({name: ('union', []) for name in self.unions})
        self.declarations.update({name: ('struct', []) for name, in self.scan(UNTAGGED)})

    def fail(self, problem):
        """Exit with status 1, naming the source and the problem."""
        fail(f'{self.source}: {problem}')

    def scan(self, pattern):
        """The groups of each match of pattern in the source's text, as
        matches() gives them."""
        return [match.groups() for match in matches(pattern, self.text)]

    def extent(self, count):
        """An extent's count, as counted() gives it. Fails where the Types
        can't evaluate it."""
        try:
            return counted(count, self.types)
        except LayoutError as error:
            self.fail(f'[{count}]: {error}')

    def check_place(self, name, section, offset, size):
        """The address of the size bytes the stub listing places for name, at
        offset in section. Fails where there are none, or where they lie in no
        section the image's tables place, outside that section's rows, outside
        the image, or in its code. Rows that meet hold bytes across them."""
        if size <= 0:
            self.fail(f'{name} has no bytes in its stub listing')
        if section not in self.starts:
            self.fail(f'{name} isn\'t in a section {self.config.splits} places')
        start = end = self.starts[section] + offset
        for low, high in sorted(self.rows[section]):
            if low <= end < high:
                end = high
        if end < start + size:
            self.fail(f'{name}, 0x{start:08x} to 0x{start + size:08x}, lies outside its {section}\'s rows in '
                      f'{self.config.splits}')
        if not self.image.inside(start, size):
            self.fail(f'{name}, 0x{start:08x} to 0x{start + size:08x}, lies outside {self.path}')
        if any(low < start + size and start < high for low, high in self.image.code):
            self.fail(f'{name}, 0x{start:08x} to 0x{start + size:08x}, lies in {self.path}\'s code')
        return start

    def static_bodies(self):
        """Each body the source gives a datum of static storage, a file's or a
        function's, by name: its qualifiers, its type, with * for a pointer,
        and its extents.

        Fails on a body:

        - of a datum named text or values, whose .inc holds the texts or
          values;
        - whose include isn't its name's .inc;
        - of a name two statics share;
        - of an array with no extent;
        - of a datum the stub listing doesn't place, or one check_place()
          fails on."""
        filled = {}
        for qualifiers, ctype, pointer, name, dims, include in self.scan(BODY):
            ctype += ' *' if pointer else ''
            if name in ('text', 'values'):
                self.fail(f'{self.directory}/{name}.inc holds its {name if name == "values" else "texts"}, and '
                          f'can\'t be {name}\'s body')
            if include != f'{self.directory}/{name}.inc':
                self.fail(f'{name}\'s body is {self.directory}/{name}.inc, not {include}')
            if name in self.twice:
                self.fail(f'{name} names two statics, and a body needs one')
            if dims and not EXTENT.findall(dims)[0].strip():
                self.fail(f'{name} needs an extent, and its stub build\'s zero body would give it one element')
            if name not in self.places:
                self.fail(f'{name} isn\'t in a section {self.config.splits} places')
            self.check_place(name, *self.places[name])
            filled[name] = qualifiers, ctype, EXTENT.findall(dims)
        return filled

    def automatic_bodies(self, filled):
        """Each body the source gives a function's automatic array, by name:
        its qualifiers, its type and its extents, as static_bodies() gives
        them.

        An array's body is the template SHC copies it from: the one unnamed
        datum of the array's size that its function loads. Its function is
        the nearest function definition above it in the source, found by
        text and not by C's scopes. The template's place goes into
        self.places under the array's name.

        Fails on an array:

        - whose include isn't its name's .inc;
        - with no function definition above it;
        - of a type tools/data.py writes no body for, or with an extent
          extent() fails on;
        - whose function loads no unnamed datum of its size, or several;
        - whose name is in self.places or filled already: the listing's data,
          an earlier automatic array, or a static body;
        - whose template check_place() fails on."""
        functions = [(match.start(), match.group(1)) for match in matches(FUNCTION, self.text)]
        automatic = {}
        for match in matches(AUTO, self.text):
            qualifiers, ctype, name, dims, include = match.groups()
            within = [function for start, function in functions if start < match.start()]
            if include != f'{self.directory}/{name}.inc':
                self.fail(f'{name}\'s body is {self.directory}/{name}.inc, not {include}')
            if not within:
                self.fail(f'{name} needs a function')
            try:
                size = element_size(ctype, self.types)
            except LayoutError as error:
                self.fail(f'{name} is {ctype}, which tools/data.py writes no body for: {error}')
            for count in EXTENT.findall(dims):
                size *= self.extent(count)
            found = [self.templates[label] for label in sorted(self.loads.get(within[-1], ()))
                     if label in self.templates and self.templates[label][2] == size]
            if len(found) != 1:
                self.fail(f'{within[-1]} loads {len(found)} templates of {name}\'s {size} bytes')
            if name in self.places or name in filled:
                self.fail(f'{name} names two bodies')
            self.places[name] = found[0]
            self.check_place(name, *found[0])
            automatic[name] = qualifiers, ctype, EXTENT.findall(dims)
        return automatic

    def check_includes(self, filled):
        """Fail where the source includes an .inc that is neither the .inc of
        a body in filled nor the source's text.inc or values.inc. --bodies
        would write no such .inc, so only the compile would fail on it."""
        taken = {f'{self.directory}/{name}.inc' for name in filled} | {f'{self.directory}/text.inc',
                                                                       f'{self.directory}/values.inc'}
        for include, in self.scan(INCLUDE):
            if include not in taken:
                self.fail(f'no declaration takes {include}: a body\'s #include has its declaration\'s "= {{" on '
                          f'the line before and "}};" on the line after, and the declaration puts any qualifiers '
                          f'before a type of one word or struct and a tag, then declares a datum of static storage, '
                          f'with one * at most, or a function\'s automatic array, with no *')

    def placed_values(self):
        """Each value the source writes, in number order, as its name, address
        and layout: the address of the member it stands for, and that
        member's scalar layout.

        The member lies in the datum whose initializer writes the value, at
        the offset the datum's layout gives it. The stub listing places the
        datum.

        Fails where:

        - value_numbers() does;
        - the source writes values and doesn't include values.inc, or the
          reverse;
        - a datum that holds a value has a type with no layout, an array of
          no extent, or an extent extent() fails on;
        - the datum's initializer is one initialized() fails on, doesn't
          close, or goes on past its braces;
        - two statics share the datum's name, or no section the tables place
          holds the datum;
        - a value lies past the datum's size in the listing, or check_place()
          fails on it;
        - a value lies in no datum's initializer."""
        numbers = value_numbers(self.source)
        if bool(numbers) != (f'#include "{self.directory}/values.inc"' in self.text):
            self.fail(f'writes values, and includes {self.directory}/values.inc, or neither')
        held = {}
        tokens = [(token.start, token.text) for token in parse(self.text).tokens if token.kind != 'comment']
        offsets = [start for start, _ in tokens]
        tokens = [token for _, token in tokens]
        for match in matches(WRITTEN, self.text if numbers else ''):
            qualifiers, ctype, name, dims = match.groups()
            at = bisect_left(offsets, match.end())
            close, depth = at, 0
            while close < len(tokens):
                depth += tokens[close] == '{'
                depth -= tokens[close] == '}'
                if not depth:
                    break
                close += 1
            if not any(VALUE.fullmatch(token) for token in tokens[at:close]):
                continue
            try:
                layout = self.types.layout(ctype)
            except LayoutError as error:
                self.fail(f'{name} is {ctype}, which tools/data.py lays out no value in: {error}')
            for count in reversed(EXTENT.findall(dims)):
                if not count.strip():
                    self.fail(f'{name} needs an extent, to lay out the values it holds')
                layout = Array(layout, self.extent(count))
            found = []
            try:
                if initialized(tokens[:close + 1], at, layout, 0, found, f'{self.source}: {name}') != close + 1:
                    self.fail(f'{name}\'s initializer goes on past its braces')
            except IndexError:
                self.fail(f'{name}\'s initializer doesn\'t close')
            if name in self.twice:
                self.fail(f'{name} names two statics, and its values need one')
            if name not in self.places or self.places[name][0] not in self.starts:
                self.fail(f'{name} holds {found[0][0]}, and isn\'t in a section {self.config.splits} places')
            section, offset, size = self.places[name]
            for value, into, scalar in found:
                if into + scalar.size > size:
                    self.fail(f'{value} lies past {name}\'s {size} bytes in its stub listing')
                held[value] = self.check_place(value, section, offset + into, scalar.size), scalar
        unheld = [f'VALUE_{number}' for number in numbers if f'VALUE_{number}' not in held]
        if unheld:
            self.fail(f'{unheld[0]} lies in no initializer of a datum it defines')
        return [(f'VALUE_{number}', *held[f'VALUE_{number}']) for number in numbers]

    def names(self):
        """The image's names by address, and the name of each datum of
        nonzero size the stub listing places in a section the tables place. A
        datum's name replaces the tables' at its address, and of two data at
        one address the later in self.places wins."""
        names = dict(self.config.names)
        for name, (section, offset, size) in self.places.items():
            if section in self.starts and size:
                names[self.starts[section] + offset] = name
        return names

    def items(self, filled):
        """The source's data as an Items, and the bodies among them as (name,
        item, type). The data are every datum of nonzero size the stub
        listing places in a section the tables place, each a range of its own
        that its name starts. The bodies come in self.places' order: the
        listing's data in its order, then the automatic arrays in the
        source's. A body's item is body()'s, as retype_uvs() retypes it, and
        every other datum's is datum()'s. Fails where body() does, and raises
        EmitError where Items does, as where two data overlap."""
        items, written = [], []
        for name, (section, offset, size) in self.places.items():
            if section not in self.starts or not size:
                continue
            start = self.starts[section] + offset
            if name in filled:
                item = self.body(name, start, size, *filled[name])
                written.append((name, item, filled[name][1]))
            else:
                item = self.datum(name, start, size)
            items.append(item)
        items.sort(key=lambda item: item.start)
        retype_uvs(self.image, items)
        names = self.names()
        return Items(self.image, items, [(item.start, item.end) for item in items],
                     {item.start: names[item.start] for item in items}), written

    def datum(self, name, start, size):
        """A datum the source defines with no body, as a Declared item of the
        type the source's declarations give it, or of type None where they
        give none. Where its element's size is known and divides the datum's,
        an array steps by that size, so that a pointer into it names an
        element; otherwise the datum is one element."""
        ctype, extents = self.declarations.get(name, (None, []))
        step = None
        if extents:
            try:
                step = element_size(ctype, self.types)
            except LayoutError:
                pass
        member = self.unions.get(name)
        return Declared(start, start + size, ctype, bool(extents), step if step and not size % step else None,
                        extents[1:], member and member.groups())

    def body(self, name, start, size, qualifiers, ctype, extents):
        """A body's item, with the qualifiers and extents its declaration
        gives it: an Item of its type's kind, or a Laid one of a scalar or of
        a type the Types of the source and its headers lay out. A type that
        is itself an array is laid out as its element. Fails where the type
        has no layout. Items checks that size is a whole number of its
        elements."""
        # each element's inner extents, which an aggregate's braces follow. A
        # pointer to it takes its type with its qualifiers, which C keeps
        array, inner, declared = bool(extents), extents[1:], qualify(qualifiers, ctype)
        factors = re.fullmatch(r'NJS_MDATA([1-4])', ctype)
        if factors or ctype in BODY_KINDS:
            kind, count = ('mdata', int(factors.group(1))) if factors else (BODY_KINDS[ctype], 0)
            return Item(start, start + size, kind, count, declared=declared, array=array, inner=inner)
        if ctype in SCALARS:
            return Laid(start, start + size, Scalar(SCALARS[ctype]), declared, array, inner)
        try:
            layout = self.types.layout(ctype)
        except LayoutError as error:
            self.fail(f'{name} is {ctype}, which tools/data.py writes no body for: {error}')
        # a type that is an array: lay out its element. Its extents follow the
        # declaration's, and where the declaration has none, the type's first
        # is the outer
        counts = []
        while isinstance(layout, Array):
            counts.append(str(layout.count))
            layout = layout.element
        return Laid(start, start + size, layout, declared, array or bool(counts),
                    extents[1:] + counts if array else counts[1:])

    def placed_texts(self):
        """Each text the source declares, in number order, as its name,
        address and bytes on the disc. A text lies where its placeholder ends
        a string in the stub build.

        Fails where:

        - texts() does;
        - the source declares texts and doesn't include text.inc, or the
          reverse;
        - the stub build ends no string with a text's placeholder, or
          several;
        - check_place() fails on a text with its null;
        - the disc's text isn't the text's length."""
        declared = texts(self.source)
        if bool(declared) != (f'#include "{self.directory}/text.inc"' in self.text):
            self.fail(f'declares texts, and includes {self.directory}/text.inc, or neither')
        found_strings = strings(self.lis) if declared else []
        placed = []
        for name, _, length, placeholder in declared:
            # a text ends its string, before the null
            mark = placeholder + b'\0'
            found = [(section, offset + len(data) - len(mark)) for section, offset, data in found_strings
                     if data.endswith(mark)]
            if len(found) != 1:
                self.fail(f'its stub build ends {len(found)} strings with {name}\'s placeholder')
            (section, at), = found
            start = self.check_place(f'{name} with its null', section, at, length + 1)
            body = self.image.payload[start - self.image.base:start - self.image.base + length + 1]
            if body.find(0) != length:
                self.fail(f'{name} isn\'t {length} bytes long on the disc')
            placed.append((name, start, body[:length]))
        return placed


def retype_uvs(image, items):
    """Make uv the kind of each vcolor item that a meshset among items points
    at as its uvs. A meshset's colors and uvs are both NJS_COLOR *, so
    BODY_KINDS makes each a vcolor."""
    uvs = set()
    for item in items:
        if item.kind == 'meshset':
            uvs |= {image.u32(address + MESHSET.uvs) for address in range(item.start, item.end, item.element)}
    for item in items:
        if item.kind == 'vcolor' and item.start in uvs:
            item.kind = 'uv'


def publish_group(out, incs, listed):
    """Replace the group in out: the .inc files with incs, each one's text by
    its file name, and bodies.txt with listed. bodies.txt is removed first
    and written last. Between them, each .inc in out but not in incs is
    removed, and each of incs is published. A failure after bodies.txt is
    removed leaves none, though some .inc files may have changed by then. So
    make finds the whole group or rebuilds it."""
    out.mkdir(parents=True, exist_ok=True)
    (out / 'bodies.txt').unlink(missing_ok=True)
    for inc in out.glob('*.inc'):
        if inc.name not in incs:
            inc.unlink()
    for name, inc in incs.items():
        publish(out / name, inc)
    publish(out / 'bodies.txt', listed)


def bodies(path, source, lis, out, conf):
    """Write --bodies' group in OUT, from IMAGE where the stub listing places
    SOURCE's data: each body's .inc, text.inc where SOURCE declares texts,
    values.inc where it writes values, and bodies.txt.

    The passes and the emitter run before anything is written, so a
    failure in one leaves the former group whole. An error the emitter
    raises fails, naming SOURCE. publish_group() then replaces the group,
    and says what a failure there leaves."""
    body_source = BodySource(path, source, lis, conf)
    filled = body_source.static_bodies()
    filled.update(body_source.automatic_bodies(filled))
    body_source.check_includes(filled)
    values = body_source.placed_values()
    placed = body_source.placed_texts()
    try:
        items, written = body_source.items(filled)
        writer = Bodies(items, body_source.names(), body_source.types, body_source.outside, body_source.functions)
        incs = {f'{name}.inc': writer.inc(name, item) for name, item, _ in written}
        numbers = [writer.initializer(scalar, start) for _, start, scalar in values]
    except EmitError as error:
        fail(f'{source}: {error}')
    listed = [f'{name}  0x{item.start:08x}  {item.end - item.start}  {ctype}\n' for name, item, ctype in written]
    if placed:
        incs['text.inc'] = ''.join(f'#define {name} {quote(text)}\n' for name, _, text in placed)
    listed += [f'{name}  0x{start:08x}  {len(text)}  text\n' for name, start, text in placed]
    scalars = {code: ctype for ctype, code in SCALARS.items()}
    if values:
        incs['values.inc'] = ''.join(f'#define {name} {number}\n' for (name, _, _), number in zip(values, numbers))
    listed += [f'{name}  0x{start:08x}  {scalar.size}  {scalars[scalar.code]}\n' for name, start, scalar in values]
    publish_group(Path(out), incs, ''.join(listed))


def main(argv):
    parser = argparse.ArgumentParser(prog='data', usage=USAGE, description=__doc__.split('\n\n')[0])
    parser.add_argument('generated', nargs='*', metavar='IMAGE SOURCE OUT',
                        help="write OUT, the C of SOURCE's gen/ range of IMAGE, and OUT's .syms")
    command = parser.add_mutually_exclusive_group()
    command.add_argument('--bodies', nargs=5, metavar=('IMAGE', 'SOURCE', 'LISTING', 'OUT', 'CONFIG'),
                         help="write SOURCE's bodies, texts and values in OUT, from IMAGE where LISTING places them")
    command.add_argument('--texts', nargs=2, metavar=('SOURCE', 'OUT'),
                         help="write OUT, the stub build's text.inc for SOURCE")
    command.add_argument('--values', nargs=2, metavar=('SOURCE', 'OUT'),
                         help="write OUT, the stub build's values.inc for SOURCE")
    args = parser.parse_args(argv)
    options = {bodies: args.bodies, text_stubs: args.texts, value_stubs: args.values}
    picked = [(function, operands) for function, operands in options.items() if operands]
    if picked and args.generated:
        parser.error(f'unrecognized arguments: {" ".join(args.generated)}')
    if not picked and len(args.generated) != 3:
        parser.error('name IMAGE, SOURCE and OUT, or one option and its operands')
    function, operands = picked[0] if picked else (generate, args.generated)
    try:
        function(*operands)
    except (LayoutError, EmitError, ReadError) as error:
        fail(error)
    except OSError as error:
        # a file a command reads or writes itself, past content() and publish()
        fail(f'{error.filename}: {error.strerror}')


if __name__ == '__main__':
    main(sys.argv[1:])
