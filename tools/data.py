"""Write the C that SHC compiles to the data the build takes from the disc.

    python3 tools/data.py IMAGE SOURCE OUT
    python3 tools/data.py --bodies IMAGE SOURCE TABLE READS OUT CONFIG
    python3 tools/data.py --offsets SOURCE LISTING READS OUT
    python3 tools/data.py --texts SOURCE OUT
    python3 tools/data.py --values SOURCE OUT

IMAGE is an image's file, uncompressed. CONFIG names config/CONFIG/. A
command with IMAGE and no CONFIG takes IMAGE's file name, lowercased and less
its suffix, as CONFIG.

With no option, the tool writes OUT, the C of SOURCE's gen/ range of IMAGE,
OUT's .syms, and its .c.d, make's rule for OUT from the sources and headers
it reads types from. The C defines each definition config/CONFIG/data.txt
gives the range: a kind as tools/items.py lays it out, and a C type as the
image's declarations do, with the header that defines it. It defines them
in D, in DHEAD under #pragma section HEAD, or const in C, by the section the
range's row lies in. Where the data points at an address outside the range
that the tables don't name, the C makes up a name for it, and OUT's .syms
gives each such name's address.

--bodies writes these files in OUT, the .inc files from the bytes IMAGE
holds where TABLE, SOURCE's table of offsets as tools/offsets.py reads one,
places SOURCE's data in SOURCE's rows:

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
every branch alike. It reads no listing, so it runs where no stub build
does.

--offsets writes OUT, SOURCE's table of offsets, as tools/offsets.py gives
one, from LISTING, the stub build's listing, which locates each datum SOURCE
names, the template of each of its automatic arrays that takes a body, and
each of its texts. --offsets reads no image and no table of an image's.

READS, which --bodies and --offsets take, is a dependency file the host
compiler writes for SOURCE, searching the include directories its stub build
searches, whose first rule names the headers SOURCE's types come from.

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
from images import ALONE, FIRST, FIRST_BASE, FIRST_END, FIRST_GAME_CODE, FIRST_SDK_CODE
from items import MESHSET, Declared, Image, Item, Laid, ReadError, defined, window
from layout import QUALIFIERS, Array, LayoutError, Pointer, Scalar, Types, spelled
from offsets import Offsets, differing, read, write
from shc_listing import listing, strings
from tool import content, fail, publish


# items.py reads the image's data, and cdata.py writes it as C. Neither
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
# a file's datum or a function's static whose initializer the source writes
# in braces, up to the brace: its qualifiers, type, name and extents. A
# function's automatic is no such datum, and a value in its initializer lies
# in none
WRITTEN = re.compile(r'^(?:[ \t]+static\s+|(?:static\s+)?)' + QUALIFIED + r'((?:struct\s+|union\s+)?\w+)\s+(\w+)\s*'
                     r'((?:\[[^]\n]*\])*)\s*=\s*(?=\{)', re.M)

# each command's command line, which -h and misuse print after "usage: "
USAGE = '''%(prog)s IMAGE SOURCE OUT
       %(prog)s --bodies IMAGE SOURCE TABLE READS OUT CONFIG
       %(prog)s --offsets SOURCE LISTING READS OUT
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
    the kinds tools/items.py lays out, or else Laid, of a type the image's
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
    prerequisites = sorted(path.as_posix() for path in config.declarations.texts) if typed else []
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
    publish(out.with_suffix('.c.d'), f'{out}:' + ''.join(f' {each}' for each in prerequisites) + '\n'
            + ''.join(f'{each}:\n' for each in prerequisites))
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
        fail(f'{depend}: missing; make writes a body source\'s reads, so run make, not this tool alone')
    rule = depend.read_text().replace('\\\n', ' ').split('\n', 1)[0]
    return [Path(name) for name in rule.partition(':')[2].split() if name.endswith('.h')]


class Source:
    """A body source as its stub build compiles it, which its offsets are
    measured from and read with: its text, and the types and declarations
    of it and of the headers its stub build reads. Its scans fail, naming
    the source, on what they can't take."""

    def __init__(self, source, reads):
        self.source = Path(source)
        # the source's text as uncommented() gives it, which every pass reads,
        # each scan of it through matches()
        written = content(self.source).decode(errors='replace')
        self.text = uncommented(written)
        # the directory the source's includes name its bodies in: its path
        # less its top directory and .c
        self.directory = Path(*self.source.parts[1:]).with_suffix('').as_posix()
        # the source's text and each header's that its reads name. The Types
        # read the headers, then the source as written, whose comments Types
        # reads as C does
        self.files = [(self.source, written), *((header, content(header).decode(errors='replace'))
                                                for header in headers(Path(reads)))]
        self.types = Types([text for _, text in self.files[1:]] + [written])
        # each datum's type as a pointer to it takes it, qualifiers kept, and
        # its extents. A union or a struct of no tag that the source defines
        # as it declares the datum is taken as union or struct, and as no
        # array. unions gives each such union's first member where that is an
        # array, and None otherwise
        declared = self.scan(DECLARATION)
        self.declarations = {name: (qualify(qualifiers, ctype) + (' *' if pointer else ''), EXTENT.findall(dims))
                             for qualifiers, ctype, pointer, name, dims in declared}
        self.unions = {name: MEMBER.match(members) for members, name in self.scan(UNION)}
        self.declarations.update({name: ('union', []) for name in self.unions})
        self.declarations.update({name: ('struct', []) for name, in self.scan(UNTAGGED)})
        # the names the source declares more than once, as two functions'
        # statics of one name are, or a datum on each side of a guard
        self.redeclared = {name for name, count in Counter(name for *_, name, _ in declared).items() if count > 1}

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

    def size(self, name):
        """The size in bytes of the datum name as the source declares it:
        its type's, times each of its extents. None where the source gives
        no one size to hold an offset's to: a name it doesn't declare or
        declares more than once, and a type or an extent the Types can't
        read, as a union's or a struct's of no tag, and an extent left
        empty."""
        if name not in self.declarations or name in self.redeclared:
            return None
        ctype, extents = self.declarations[name]
        try:
            size = element_size(ctype, self.types)
            for count in extents:
                size *= counted(count, self.types)
        except LayoutError:
            return None
        return size

    def arrays(self):
        """Each body the source gives a function's automatic array, in the
        source's order, as (name, function, size, qualifiers, type,
        extents). The array's function is the nearest function definition
        above it in the source, found by text and not by C's scopes. Its
        size is in bytes, and its qualifiers, type and extents are as
        BodySource.static_bodies() gives a static's.

        Fails on an array:

        - whose include isn't its name's .inc;
        - with no function definition above it;
        - of a type tools/data.py writes no body for, or with an extent
          extent() fails on."""
        functions = [(match.start(), match.group(1)) for match in matches(FUNCTION, self.text)]
        found = []
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
            found.append((name, within[-1], size, qualifiers, ctype, EXTENT.findall(dims)))
        return found

    def declared_texts(self):
        """The source's texts, as texts() gives them. Fails where it does,
        and where the source declares texts and doesn't include text.inc,
        or the reverse."""
        declared = texts(self.source)
        if bool(declared) != (f'#include "{self.directory}/text.inc"' in self.text):
            self.fail(f'declares texts, and includes {self.directory}/text.inc, or neither')
        return declared


def measured(source, lis):
    """The Offsets of source, a Source, as lis, its stub build's listing,
    gives them:

    - each named datum the listing holds, in its order;
    - each automatic array that takes a body, at its template: the one
      unnamed datum of the array's size that its function loads;
    - each text with its null, where its placeholder ends a string. asmsh's
      listing gives no strings, so a patched source can't take texts.

    Fails, naming the source, where:

    - Source.arrays() or Source.declared_texts() does;
    - an array's function loads no unnamed datum of its size, or several;
    - an array's name is a named datum's, or an earlier array's;
    - the stub build ends no string with a text's placeholder, or several;
    - a named datum's size isn't the one Source.size() gives it."""
    named, unnamed, loads = listing(lis)
    templates, placed = {}, {}
    for name, function, size, *_ in source.arrays():
        found = [unnamed[label] for label in sorted(loads.get(function, ()))
                 if label in unnamed and unnamed[label][2] == size]
        if len(found) != 1:
            source.fail(f'{function} loads {len(found)} templates of {name}\'s {size} bytes')
        if name in templates or any(name == datum for datum, *_ in named):
            source.fail(f'{name} names two bodies')
        templates[name] = found[0]
    declared_texts = source.declared_texts()
    found_strings = strings(lis) if declared_texts else []
    for name, _, length, placeholder in declared_texts:
        # a text ends its string, before the null
        mark = placeholder + b'\0'
        found = [(section, offset + len(data) - len(mark)) for section, offset, data in found_strings
                 if data.endswith(mark)]
        if len(found) != 1:
            source.fail(f'its stub build ends {len(found)} strings with {name}\'s placeholder')
        placed[name] = (*found[0], length + 1)
    for name, _, _, size in named:
        declared = source.size(name)
        if declared not in (None, size):
            source.fail(f'{name} is {size} bytes in {lis}, and {declared} as it declares it')
    return Offsets(named, templates, placed)


class BodySource(Source):
    """A body source, and what --bodies reads with it: its offsets, as its
    table records them, and the image and tables its bodies come from. Its
    passes fail, naming the file at fault, on what they can't take."""

    def __init__(self, path, source, reads, table, conf):
        super().__init__(source, reads)
        self.path, self.table, self.offsets = path, Path(table), read(table, source)
        self.config = Config(conf)
        self.image = self.config.image(path)
        # each named datum's section, offset and size, which
        # automatic_bodies() adds each array's template to, and the names two
        # statics share
        self.places, self.twice = self.offsets.named(), self.offsets.shared()
        # what a body can point at outside the source: a name the source or
        # a header declares extern, or a function they declare
        self.outside = agreed(externs(self.files, BODY_EXTERN))
        self.functions = {match.group(1) for _, text in self.files
                          for match in matches(DECLARED_FUNCTION, uncommented(text))}
        # each section's rows, and its start: its first row's, where link.py
        # places the object's section, and so where the offsets in it start
        self.rows, self.starts = {}, {}
        for section, start, end in self.config.ranges(self.source.as_posix()):
            self.rows.setdefault(section, []).append((start, end))
            self.starts.setdefault(section, start)

    def stale(self, problem):
        """Fail on problem, a difference between the source and its offsets,
        naming the source, then its table and what writes one, as
        offsets.differing() says."""
        self.fail(f'{problem}. {differing(self.table)}')

    def check_place(self, name, section, offset, size):
        """The address of the size bytes the offsets place for name, at
        offset in section. Fails where there are none, or where they lie in no
        section the image's tables place, outside that section's rows, outside
        the image, or in its code. Rows that meet hold bytes across them."""
        if size <= 0:
            self.fail(f'its offsets give {name} no bytes')
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

    def check_sizes(self):
        """Fail on a named datum to which the offsets give a size other than
        the one size() gives it, as a table can once the source or a header
        changes a type's size."""
        for name, _, _, size in self.offsets.data:
            declared = self.size(name)
            if declared not in (None, size):
                self.stale(f'{name} is {declared} bytes as it declares it, and its offsets give it {size}')

    def static_bodies(self):
        """Each body the source gives a datum of static storage, a file's or a
        function's, by name: its qualifiers, its type, with * for a pointer,
        and its extents. Each branch of a conditional can give a datum its
        body, where each declares it with the same qualifiers, type and
        extents.

        Fails on a body:

        - of a datum named text or values, whose .inc holds the texts or
          values;
        - whose include isn't its name's .inc;
        - of a name two statics share;
        - of an array with no extent;
        - of a datum the offsets don't place, or one check_place() fails
          on;
        - of a datum an earlier declaration gives its body, with other
          qualifiers, another type or other extents."""
        filled, declared = {}, {}
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
                self.stale(f'its offsets give {name} no place')
            self.check_place(name, *self.places[name])
            given = qualify(qualifiers, ctype) + dims
            if declared.setdefault(name, given) != given:
                self.fail(f'{name} takes its body as {declared[name]} and as {given}, and a body needs one type')
            filled[name] = qualifiers, ctype, EXTENT.findall(dims)
        return filled

    def automatic_bodies(self, filled):
        """Each body the source gives a function's automatic array, by name:
        its qualifiers, its type and its extents, as static_bodies() gives
        them.

        An array's body is the template SHC copies it from, which the
        offsets place under the array's name. The template's place goes into
        self.places under that name.

        Fails:

        - where arrays() does;
        - on an array the offsets give no template, or one of another size
          than the array's;
        - on an array whose name is in self.places or filled already: a
          named datum, an earlier automatic array, or a static body;
        - on an array whose template check_place() fails on;
        - on a template the offsets give for a name that is no such
          array's."""
        automatic = {}
        for name, _, size, qualifiers, ctype, extents in self.arrays():
            if name not in self.offsets.templates:
                self.stale(f'its offsets give {name} no template')
            if self.offsets.templates[name][2] != size:
                self.stale(f'{name} is {size} bytes as it declares it, and its offsets give its template '
                           f'{self.offsets.templates[name][2]}')
            if name in self.places or name in filled:
                self.fail(f'{name} names two bodies')
            self.places[name] = self.offsets.templates[name]
            self.check_place(name, *self.places[name])
            automatic[name] = qualifiers, ctype, extents
        for name in self.offsets.templates:
            if name not in automatic:
                self.stale(f'its offsets give {name} a template, and no automatic array of that name takes a body')
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
        the offset the datum's layout gives it. The datum is of static
        storage, a file's or a function's, and the offsets place it.

        Fails where:

        - value_numbers() does;
        - the source writes values and doesn't include values.inc, or the
          reverse;
        - a datum that holds a value has a type with no layout, an array of
          no extent, or an extent extent() fails on;
        - the datum's initializer is one initialized() fails on, doesn't
          close, or goes on past its braces;
        - two statics share the datum's name, the offsets don't place the
          datum, or no section the tables place holds it;
        - a value lies past the datum's size in the offsets, or check_place()
          fails on it;
        - a value lies in no such datum's initializer, as one in the
          initializer of a function's automatic does."""
        numbers = value_numbers(self.source)
        if bool(numbers) != (f'#include "{self.directory}/values.inc"' in self.text):
            self.fail(f'writes values, and includes {self.directory}/values.inc, or neither')
        held = {}
        tokens = [(token.start, token.text) for token in parse(self.text).tokens if token.kind != 'comment']
        positions = [start for start, _ in tokens]
        tokens = [token for _, token in tokens]
        for match in matches(WRITTEN, self.text if numbers else ''):
            qualifiers, ctype, name, dims = match.groups()
            at = bisect_left(positions, match.end())
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
            if name not in self.places:
                self.stale(f'its offsets give {name} no place')
            if self.places[name][0] not in self.starts:
                self.fail(f'{name} holds {found[0][0]}, and isn\'t in a section {self.config.splits} places')
            section, offset, size = self.places[name]
            for value, into, scalar in found:
                if into + scalar.size > size:
                    self.fail(f'{value} lies past the {size} bytes its offsets give {name}')
                held[value] = self.check_place(value, section, offset + into, scalar.size), scalar
        unheld = [f'VALUE_{number}' for number in numbers if f'VALUE_{number}' not in held]
        if unheld:
            self.fail(f'{unheld[0]} lies in no initializer of a datum it defines')
        return [(f'VALUE_{number}', *held[f'VALUE_{number}']) for number in numbers]

    def names(self):
        """The image's names by address, and the name of each datum of
        nonzero size the offsets place in a section the tables place. A
        datum's name replaces the tables' at its address, and of two data at
        one address the later in self.places wins."""
        names = dict(self.config.names)
        for name, (section, offset, size) in self.places.items():
            if section in self.starts and size:
                names[self.starts[section] + offset] = name
        return names

    def items(self, filled):
        """The source's data as an Items, and the bodies among them as (name,
        item, type). The data are every datum of nonzero size the offsets
        place in a section the tables place, each a range of its own that
        its name starts. The bodies come in self.places' order: the named
        data in the offsets' order, then the automatic arrays in the
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
        address and bytes on the disc. The offsets place each text with its
        null.

        Fails:

        - where declared_texts() does;
        - on a text the offsets don't place, or place in other than its
          length and its null;
        - on a text check_place() fails on with its null;
        - on a text whose bytes on the disc aren't of its length;
        - on a text the offsets place and the source doesn't declare."""
        placed = []
        for name, _, length, _ in self.declared_texts():
            if name not in self.offsets.texts:
                self.stale(f'its offsets give {name} no place')
            section, at, size = self.offsets.texts[name]
            if size != length + 1:
                self.stale(f'{name} is {length} bytes long, and its offsets give it and its null {size}')
            start = self.check_place(f'{name} with its null', section, at, size)
            body = self.image.payload[start - self.image.base:start - self.image.base + size]
            if body.find(0) != length:
                self.fail(f'{name} isn\'t {length} bytes long on the disc')
            placed.append((name, start, body[:length]))
        for name in self.offsets.texts:
            if all(name != text for text, _, _ in placed):
                self.stale(f'its offsets place {name}, and it declares no such text')
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


def bodies(path, source, table, reads, out, conf):
    """Write --bodies' group in OUT, from IMAGE where TABLE places SOURCE's
    data: each body's .inc, text.inc where SOURCE declares texts, values.inc
    where it writes values, and bodies.txt. TABLE gives SOURCE's offsets as
    offsets.read() reads them, READS names the headers SOURCE's types come
    from, and extract() writes the group from them. Fails where read()
    does, as on a table that is missing."""
    extract(BodySource(path, source, reads, table, conf), out)


def extract(body_source, out):
    """Write in OUT the group of body_source, a BodySource, from its image
    where its offsets place its data.

    The passes and the emitter run before anything is written, so a
    failure in one leaves the former group whole. An error the emitter
    raises fails, naming the source. publish_group() then replaces the
    group, and says what a failure there leaves."""
    body_source.check_sizes()
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
        fail(f'{body_source.source}: {error}')
    listed = [f'{name}  0x{item.start:08x}  {item.end - item.start}  {ctype}\n' for name, item, ctype in written]
    if placed:
        incs['text.inc'] = ''.join(f'#define {name} {quote(text)}\n' for name, _, text in placed)
    listed += [f'{name}  0x{start:08x}  {len(text)}  text\n' for name, start, text in placed]
    scalars = {code: ctype for ctype, code in SCALARS.items()}
    if values:
        incs['values.inc'] = ''.join(f'#define {name} {number}\n' for (name, _, _), number in zip(values, numbers))
    listed += [f'{name}  0x{start:08x}  {scalar.size}  {scalars[scalar.code]}\n' for name, start, scalar in values]
    publish_group(Path(out), incs, ''.join(listed))


def record(source, lis, reads, out):
    """Write OUT, SOURCE's table of offsets, from LISTING, its stub build's
    listing, and the headers READS names, as measured() reads them. Fails
    where measured() does, and where offsets.write() does, and writes no
    table then."""
    write(Path(out), measured(Source(source, reads), Path(lis)))


def main(argv):
    parser = argparse.ArgumentParser(prog='data', usage=USAGE, description=__doc__.split('\n\n')[0])
    parser.add_argument('generated', nargs='*', metavar='IMAGE SOURCE OUT',
                        help="write OUT, the C of SOURCE's gen/ range of IMAGE, and OUT's .syms")
    command = parser.add_mutually_exclusive_group()
    command.add_argument('--bodies', nargs=6, metavar=('IMAGE', 'SOURCE', 'TABLE', 'READS', 'OUT', 'CONFIG'),
                         help="write SOURCE's bodies, texts and values in OUT, from IMAGE where TABLE places them")
    command.add_argument('--offsets', nargs=4, metavar=('SOURCE', 'LISTING', 'READS', 'OUT'),
                         help="write OUT, SOURCE's table of offsets, from LISTING and the headers READS names")
    command.add_argument('--texts', nargs=2, metavar=('SOURCE', 'OUT'),
                         help="write OUT, the stub build's text.inc for SOURCE")
    command.add_argument('--values', nargs=2, metavar=('SOURCE', 'OUT'),
                         help="write OUT, the stub build's values.inc for SOURCE")
    args = parser.parse_args(argv)
    options = {bodies: args.bodies, record: args.offsets, text_stubs: args.texts, value_stubs: args.values}
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
