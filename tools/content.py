"""Write make gcc's content of an image: the data of all its gen/ ranges as
one C file, which GCC compiles.

    python3 tools/content.py IMAGE OUT

IMAGE is an image's file, uncompressed. The tool reads the image's tables
in config/NAME/, where NAME is IMAGE's file name, lowercased and less its
suffix, and writes each definition data.txt gives the content: one of the
kinds tools/image.py lays out, or a type the image's declarations lay out.
It writes OUT, the image's content. A word of the content is a pointer only
where more than its bits says so:

- a structure of a kind data.txt gives has a pointer there;
- a type data.txt gives has a pointer there;
- its row of data.txt vouches for it.

A word in RAM that none of them types fails the content, unless it lies in
a text data.txt gives. Each pointer into RAM is a name, as Content gives
it. A float that a structure or a typed definition holds is the literal of
fewest significant digits that SHC's conversion reads back as its word. A
float whose word lies in RAM, through a view the content names or through
0x2c or P2's 0xac, fails the content. The bytes no structure claims that
hold no pointer are an array of their halves or bytes.

A name the content takes from another image's gen/ range takes the type
that image's data.txt gives it. The content finds each text a C source
declares in the bodies.txt tools/data.py --bodies writes for the source.
Beside OUT the tool writes the content's record, .txt; its check's link
script, .ld; .bin, the bytes that link must give; and .d, make's rule for
OUT from each file it reads. Content, the content's writer, extends
cdata.py's Writer.

GCC.md#content gives the content's rules.
"""

import argparse
import re
import struct
import sys
from bisect import bisect_left, bisect_right
from pathlib import Path

import bodies
import config
import data
from cdata import (EXTENT, INTEGERS, EmitError, Emitted, Items, Writer, converts, element_size, includes, payload,
                   quote, wrap)
from config import IMPORTS, configs, imports
from csource import included, matches, uncommented
from data import DECLARATION, GEN_SECTIONS, definitions, qualify
from image import GENERIC, KINDS, MDATA_KIND, RAM, SAME, SINGLE, Image, Item, ReadError, Struct
from images import CONFIG, FIRST, FIRST_BASE, FIRST_END, images
from layout import LayoutError, Scalar, Types, spelled
from memory import VIEW_BIT
from stages import STAGE_HEADER, STAGE_LOADER, stages
from tables import named
from tool import content, fail, publish

# 1ST_READ.BIN's table of src/path/path.h's struct path_list, by its name in
# symbols.txt, and a row's layout: each row keys an act's list of paths by
# its STAGE_ACT, up to the row keyed PATH_LIST_END
PATH_LISTS = 'path_lists'
PATH_LIST = Struct(8, stage_act=0, paths=4)
PATH_LIST_END = 0xFFFF
# the mask of an address's physical bits: all but its top three, which pick
# the view of memory it goes through. RAM_BASE is main RAM's physical
# address
PHYSICAL = 0x1FFFFFFF
RAM_BASE = 0x0C000000
# the views of main RAM a pointer of the content goes through, 0x0c and
# 0x8c, which VIEW_BIT alone tells apart. The content takes each of its
# words in either view for an address, and writes each such word that is a
# pointer as a name. The matching build's generator takes for an address
# only a word in the windows or at the imports' addresses that
# data.Config.image() gives
VIEWS = tuple((view, view + RAM) for view in (RAM_BASE, RAM_BASE | VIEW_BIT))
# the bit that takes RAM's 0x8c view to P2's 0xac, and its 0x0c view to 0x2c
P2_BIT = 0x20000000
# the kinds of bytes no structure claims and that hold no pointer, which the
# content writes as Blobs, and why they hold no address
BLOBS = {'floats': 'floats', 'words': 'no word an address', 'halves': 'a half word', 'bytes': 'bytes'}
# the SDK's 32-bit integer types, by struct code, to which the content casts
# an address a typed definition's integer of that code holds
CASTS = {code: ctype for ctype, (code, _) in INTEGERS.items() if struct.calcsize(code) == 4}


class Config(data.Config):
    """data.Config, with 1ST_READ.BIN's names from its own table alone, and
    the image's windows VIEWS. The images beside 1ST_READ.BIN share their
    addresses, so its content takes an import's name only where keyed()
    gives it."""

    def __init__(self, name):
        super().__init__(name)
        if self.path == FIRST:
            self.names = {}
            for address, symbol in named(FIRST / 'symbols.txt'):
                self.names.setdefault(address, symbol)

    def image(self, path):
        """The image data.Config.image() reads at path, with VIEWS for its
        windows. VIEWS hold every address that data.Config.image()'s windows
        and imports hold."""
        image = super().image(path)
        return Image(image.name, image.payload, image.base, image.code, VIEWS, image.fill)


def keyed(image):
    """The names that keys give words of 1ST_READ.BIN's path_lists, as
    {address: name}, read from image, 1ST_READ.BIN's. Each row's key is a
    STAGE_ACT. The bytes alone can't say which image's definition a pointer
    outside the image means. So a row's pointer there names the one import
    at its address from an image the key's stage reads on every pass, as
    stages() gives them. Those are the reads tools/imports.py holds a C use
    of an import to. A pointer at which no import lies takes no name here.
    With no row for path_lists in 1ST_READ.BIN's symbols.txt, there are
    none.

    Fails where:

    - only another image's import holds a pointer, or more than one of
      the stage's do, naming the row;
    - the rows run past the image before a row keyed PATH_LIST_END."""
    row = next((address for address, name in named(FIRST / 'symbols.txt') if name == PATH_LISTS), None)
    if row is None:
        return {}
    taken, loads, found = {}, stages(), {}
    for name, provider, _, address, _ in imports():
        taken.setdefault(address, []).append((name, provider))
    while True:
        if not image.inside(row, PATH_LIST.size):
            fail(f'{PATH_LISTS}: its rows run past {image.name} with no row keyed 0x{PATH_LIST_END:x}')
        key, pointer = image.u32(row + PATH_LIST.stage_act), image.u32(row + PATH_LIST.paths)
        if key == PATH_LIST_END:
            break
        if pointer and not FIRST_BASE <= pointer < FIRST_END:
            label, loaded = loads.get(key >> 8, (f'stage {key >> 8}', []))
            held = taken.get(pointer, [])
            ours = [(name, provider) for name, provider in held if provider in loaded]
            if len(ours) == 1:
                found[row + PATH_LIST.paths] = ours[0][0]
            elif held:
                fail(f'{PATH_LISTS}: its row at 0x{row:08x} keys 0x{pointer:08x} to {label}, act {key & 0xFF}, '
                     f'which reads {", ".join(loaded) or "no image"} on every pass, and {IMPORTS} takes '
                     + ' and '.join(f'{name} from {provider}' for name, provider in ours or held) + ' there')
        row += PATH_LIST.size
    return found


class Blob(Item):
    """Bytes no structure claims that hold no pointer, as one array. reason
    says why they hold no address, as BLOBS gives their kind. The array is
    of halves where data.txt gives halves, which GCC aligns to 2, and of
    bytes otherwise. SH's GCC aligns an array of bytes to 4, which would
    part halves at 2 past 4 from the definition before them."""

    def __init__(self, item):
        super().__init__(item.start, item.end, 'halves' if item.kind == 'halves' else 'bytes')
        self.aligned, self.reason = item.alignment, BLOBS[item.kind]

    @property
    def alignment(self):
        return self.aligned


def in_ram(word):
    """Whether a word points into main RAM, through any view of it."""
    return RAM_BASE <= word & PHYSICAL < RAM_BASE + RAM


class Content(Writer):
    """An image's content as C for GCC. A pointer into the content is a
    name, an element of one, or a byte offset into a definition. A pointer
    outside it takes the name keys gives its word, or else the name the
    tables give its address, or an element of the name below it. The
    content declares each such name with the type provided() gives it. A
    void or char pointer to the start of a text a C source declares, where
    no table names it, takes a copy of the text, as provided.text() finds
    it: GCC's string literals take no name. Any other pointer outside the
    content into RAM is unresolved: it takes a spelling no C identifier
    spells, so that the image's link fails on it. A pointer outside RAM is
    its number. Each string is a char array, and each blob an array of its
    halves or bytes, as Blob gives it.

    items is an Items over the content's ranges, and names the image's names,
    by address. ranges are the content's, each (start, end, whether its data
    is const); keys the names path_lists' keys give its words, by the word's
    address; and provided the image's Providers."""

    def __init__(self, items, names, ranges, keys, provided):
        super().__init__(items, names)
        self.ranges = ranges
        self.keys = keys
        self.provided = provided
        self.addresses = sorted(names)
        # each text the content copies, by its address: its source, its name there and its bytes
        self.copies = {}
        self.unresolved = {}  # each unresolved word's address and the address it holds, by its C name
        self.types = {}  # each name the content takes: its provider, its symbol there, its type and whether an array

    def const(self, address):
        """Whether the content's data at address, which a range must hold, is
        const."""
        return next(constant for start, end, constant in self.ranges if start <= address < end)

    def ref(self, address, pointee):
        """As Writer.ref, but a pointer outside the content takes a name as
        the class gives it, and a word outside RAM is its number.

        A text it copies goes into copies, and the pointer is its copy, cast
        at once. An unresolved word goes into unresolved. Each name from
        outside, an unresolved word's included, goes into uses with its
        address and the pointee.

        Raises EmitError as Writer.ref does, and for a pointer:

        - into a range's padding between definitions;
        - into RAM through a view the content doesn't name;
        - of 1ST_READ.BIN's into an image beside it, where no key names it."""
        value = self.word(address)
        index = bisect_right(self.starts, value) - 1
        if not value or index >= 0 and value < self.items[index].end:
            return super().ref(address, pointee)
        if any(start <= value < end for start, end, _ in self.ranges):
            raise EmitError(self.image, address, f'a pointer to 0x{value:08x}, in the padding between definitions')
        if not self.image.is_address(value):
            if in_ram(value):
                raise EmitError(self.image, address, f'0x{value:08x} points into RAM through a view the content '
                                                     "doesn't name")
            return f'({pointee} *)0x{value:08x}'
        name, offset = self.keys.get(address), 0
        beside = name is None and self.provided.config.path == FIRST and not self.provided.holds(FIRST.name, value) \
            and self.provided.beside(value)
        if beside:
            # the images beside 1ST_READ.BIN share their addresses, so the
            # address alone, or a name 1ST_READ.BIN's own table gives it,
            # can't say which image's definition the word means
            raise EmitError(self.image, address, f'0x{value:08x} points into {", ".join(beside)}, where '
                            "1ST_READ.BIN takes a definition only as the key of the word's row gives it, and no key "
                            'gives this word one')
        name, element = name or self.names.get(value), None
        below = bisect_right(self.addresses, value) - 1
        if name is None and below >= 0:
            # an element of the array the nearest name below the address
            # starts, where that name's C definition holds the address
            start = self.addresses[below]
            element = self.provided.element(self.names[start], start, value)
            if element is not None:
                name, offset = self.names[start], value - start
        if name is None:
            text = self.provided.text(value) if pointee in ('void', 'char') else None
            if text is not None:
                source, label, size = text
                self.copies[value] = source, label, payload(self.image, value, value + size)
                return f'({pointee} *)dat_{value:08x}'
            name = f'unresolved_{address:08x}'
            self.unresolved[name] = (address, value)
        self.uses.setdefault(name, (value - offset, set()))[1].add(pointee)
        return f'\1{name}\2{pointee}\2{element or 0}\3'

    def extern(self, name):
        """The type of a name from outside, as (ctype, whether it is an
        array): ('char', True) for an unresolved word's name, and else the
        type provided() gave it, 'code' for an address in its provider's
        code."""
        if name in self.unresolved:
            return 'char', True
        return self.types[name][2:]

    def resolve(self, match):
        """The constant for a name from outside, which ref() left for later
        with the index of its element, or 0, as a pointee * takes it. Code
        takes a cast unless pointee is void. Any other name is itself where
        it is an array, else &name, and an element past its first
        &name[index]; each takes a cast where C won't convert it."""
        name, pointee, element = match.group(1), match.group(2), int(match.group(3))
        ctype, array = self.extern(name)
        if ctype == 'code':
            return name if pointee == 'void' else f'({pointee} *){name}'
        expression = f'&{name}[{element}]' if element else name if array else f'&{name}'
        return expression if converts(ctype, pointee) else f'({pointee} *){expression}'

    def float(self, address):
        """As Writer.float, but raises EmitError for a word in RAM through a
        view the content names, or through 0x2c or P2's 0xac: nothing says
        whether its bits are a pointer. Each such float is less than 1e-11 in
        size. A float through RAM's other views is 2^25 or more, as game
        data's can be."""
        word = self.word(address)
        if self.image.is_address(word & ~P2_BIT):
            item = self.items[self.index]
            raise EmitError(self.image, address, f'0x{word:08x} lies in RAM, in {self.name(item.start)}, a '
                                                 f'{item.ctype}, whose type gives it a float, and nothing says '
                                                 'whether it is a pointer')
        return super().float(address)

    def initializer(self, layout, address):
        """As Writer.initializer, but for a 32-bit integer whose word lies in
        RAM, through a view the content names. Where its row of data.txt
        vouches for it, its value is the address, cast to the SDK type of its
        struct code, Sint32 or Uint32. Raises EmitError for any other such
        integer: its type says a number, and nothing says whether its bits are
        a pointer."""
        if not (isinstance(layout, Scalar) and layout.code in CASTS and self.image.is_address(self.word(address))):
            return super().initializer(layout, address)
        item = self.items[self.index]
        if item.vouched:
            return f'({CASTS[layout.code]}){self.ref(address, "void")}'
        raise EmitError(self.image, address, f'0x{self.word(address):08x} lies in RAM, in {self.name(item.start)}, '
                                             f'a {item.ctype}, whose type gives it a number, and nothing says '
                                             'whether it is a pointer: vouched on its row of data.txt says so')

    def definition(self, item):
        """An item's definition, const where its range's data is: a string as
        a char array, a blob as an array of its halves or bytes, and any other
        item as Writer writes it. Raises EmitError for a word in RAM, through
        any view, in a blob, in an array of pointers or mixed words whose row
        doesn't vouch for it, or across blobs: nothing says whether it is a
        pointer. In an array whose row vouches for it, it raises EmitError for
        a word in RAM through a view the content doesn't name, as ref() does
        for a pointer."""
        self.constant = self.const(item.start)
        name, size = self.name(item.start), item.end - item.start
        if (isinstance(item, Blob) or item.kind in ('pointers', 'mixed')) and not item.vouched:
            for address in range(item.start + (-item.start) % 4, item.end - 3, 4):
                word = self.word(address)
                if in_ram(word):
                    raise EmitError(self.image, address, f'0x{word:08x} lies in RAM, in {name}, an array of no '
                                                         'known structure, and nothing says whether it is a '
                                                         'pointer: a type data.txt gives, or vouched on its row, '
                                                         'says so')
        if isinstance(item, Blob) and item.start % 4:
            # the word on 4 that the blob starts inside, split across rows of data.txt. The loop above reads
            # only the words one blob holds whole
            low = item.start - item.start % 4
            spanned = [each for each in self.items[max(bisect_right(self.starts, low) - 1, 0):
                                                   bisect_left(self.starts, low + 4)] if each.end > low]
            word = self.word(low)
            if (in_ram(word) and all(isinstance(each, Blob) for each in spanned)
                    and sum(min(each.end, low + 4) - max(each.start, low) for each in spanned) == 4):
                raise EmitError(self.image, low, f'0x{word:08x} lies in RAM, across '
                                + ' and '.join(self.name(each.start) for each in spanned)
                                + ', arrays of no known structure, and nothing says whether it is a pointer: a type '
                                  'data.txt gives says so')
        if item.kind in ('pointers', 'mixed') and item.vouched:
            for address in range(item.start, item.end, 4):
                word = self.word(address)
                if in_ram(word) and not self.image.is_address(word):
                    raise EmitError(self.image, address, f'0x{word:08x} points into RAM through a view the content '
                                                         "doesn't name")
        if not isinstance(item, Blob):
            return super().definition(item)
        layout, raw = KINDS[item.kind].layout, payload(self.image, item.start, item.end)
        values = [f'0x{value:0{2 * layout.size}x}' for value, in struct.iter_unpack(f'<{layout.code}', raw)] \
            if any(raw) else ['0']
        return (f'{self.typed(item.ctype, name)}[{size // layout.size}] = {{\n    '
                + ',\n    '.join(wrap(values, 16 // layout.size)) + ',\n};')

    def source(self, title, headers=()):
        """The items' C file, titled title, as an Emitted:

        - ninja.h's include, collision/world.h's and path/path.h's where a
          land or a path needs them, and one for each of headers;
        - an extern declaration for each name from outside, an unresolved
          word's under its spelling, and for each item pointed at ahead of its
          definition;
        - a static copy of each text the content copies;
        - the definitions.

        It records in types what provided() gives each name taken from
        outside, which placed() reads.

        Raises EmitError where an item's bytes can't be written as C, and
        fails where provided() does."""
        body = []
        for self.index, item in enumerate(self.items):
            body.append(self.definition(item))
        for name, (value, pointees) in self.uses.items():
            if name not in self.unresolved:
                self.types[name] = self.provided(name, value, pointees)
        text = re.sub('\1([^\2]*)\2([^\2]*)\2([0-9]+)\3', self.resolve, '\n\n'.join(body))
        lines = [f'/* {title} */', '', *includes(self.items, headers), '']
        for name in sorted(self.uses):
            ctype, array = self.extern(name)
            if ctype == 'code':
                lines.append(f'extern void {name}();')
            elif name in self.unresolved:
                lines.append(f'extern char {name}[] __asm__("{self.spelling(name)}");')
            else:
                space = '' if ctype.endswith('*') else ' '
                lines.append(f'extern {ctype}{space}{name}{"[]" if array else ""};')
        for index in sorted(self.ahead):
            item = self.items[index]
            self.constant = self.const(item.start)
            lines.append(f'extern {self.typed(item.ctype, self.name(item.start))}{"[]" if item.array else ""};')
        for value, (_, _, copied) in sorted(self.copies.items()):
            lines.append(f'static char const dat_{value:08x}[{len(copied) + 1}] = {quote(copied)};')
        return Emitted('\n'.join([*lines, '', text, '']), {name: value for name, (value, _) in self.uses.items()})

    def spelling(self, name):
        """An unresolved word's symbol: its image, its address and the
        address it holds, with no leading underscore and dots between. No C
        identifier spells it, and the link that fails on it names all
        three."""
        address, value = self.unresolved[name]
        return f'unresolved.{Path(self.image.name).stem.lower()}.{address:08x}.{value:08x}'

    def placed(self):
        """The content's record, as (name, address, size, what it holds): each
        definition and copy, then by address each name the content takes from
        outside and each unresolved word's spelling. What a definition whose
        row vouches for it holds ends in the row's path:line. A name taken has
        size None, and what it holds is 'taken: ' and then its provider, its
        symbol there and its type. A spelling has size None, and what it holds
        is 'unresolved: ' and then the word's address and why."""
        found = []
        for item in self.items:
            size = item.end - item.start
            if isinstance(item, Blob):
                what = f'{item.kind}: non-address data, {item.reason}'
            elif item.kind == 'string':
                what = 'text'
            elif item.kind in ('pointers', 'mixed'):
                what = 'void *[]'
            else:
                what = item.ctype + ('[]' if item.array else '')
            what += f': {item.vouched}' if item.vouched else ''
            found.append((self.name(item.start), item.start, size, what))
        for value, (source, label, copied) in sorted(self.copies.items()):
            found.append((f'dat_{value:08x}', value, len(copied) + 1, f'text: a copy of {label}, which {source} '
                                                                       'declares'))
        for name, (value, _) in sorted(self.uses.items(), key=lambda use: use[1][0]):
            if name in self.unresolved:
                found.append((self.spelling(name), value, None, 'unresolved: the word at '
                              f'0x{self.unresolved[name][0]:08x} points where no table names a definition'))
            else:
                provider, symbol, ctype, array = self.types[name]
                found.append((name, value, None, f'taken: {provider} {symbol} {ctype}{"[]" if array else ""}'))
        return found


class Providers:
    """The definitions an image's content takes from outside it: for each
    name, its provider, the image that defines it; that image's own name for
    it; and the type its definition gives it. text() gives the texts the
    image's C sources declare. read collects each file read to find them."""

    def __init__(self, config):
        self.config = config
        self.configs = {config.name: config}
        self.imported = {name: (image, symbol) for name, image, symbol, _, _ in imports()} \
            if config.path == FIRST else {}
        # each other image's own names, by address, read once a name lies in
        # neither the content's image's span nor 1ST_READ.BIN's
        self.owned = None
        self.rows = {}  # the type each row of a provider's data.txt gives, by the row's address
        self.defined = {}  # each C source's definitions
        self.texts = {}  # each C source's texts in the image, as {address: (name, length)}
        self.homes = None  # every image's Config, read once a text's source needs its home
        self.source_types = {}  # the Types of each C source element() reads, by its path
        self.read = set()

    def image(self, name):
        """The tables of the image name, which read its splits.txt alone."""
        if name not in self.configs:
            self.configs[name] = config.Config(name)
            self.read.add(self.configs[name].splits)
        return self.configs[name]

    def owners(self, name, value):
        """Each (image, symbol) that can give name at value, as the class gives
        them, from the first of these that applies:

        - for 1ST_READ.BIN, an import's image and symbol;
        - the content's own image, where its span holds value;
        - 1ST_READ.BIN, where its span does;
        - each other image whose own table gives name there, in its span.

        The files it reads join read."""
        if name in self.imported:
            return [self.imported[name]]
        for image in (self.config.name, FIRST.name):
            if self.holds(image, value):
                return [(image, name)]
        if self.owned is None:
            self.owned = {image: owned_names(CONFIG / image / 'symbols.txt')
                          for image in images() if image not in (self.config.name, FIRST.name)}
            self.read |= {CONFIG / image / 'symbols.txt' for image in self.owned}
        return [(image, name) for image, table in self.owned.items() if name in table.get(value, ())
                and self.holds(image, value)]

    def provider(self, name, value):
        """(image, symbol) for name at value: the one owners() gives, or None
        where it gives none or several."""
        owners = self.owners(name, value)
        return owners[0] if len(owners) == 1 else None

    def holds(self, image, value):
        """Whether value lies in image's span, through either view of RAM."""
        low, high = self.image(image).span()
        return low <= value < high or low <= value ^ VIEW_BIT < high

    def beside(self, value):
        """The images but 1ST_READ.BIN whose span holds value."""
        return [image for image in images() if image != FIRST.name and self.holds(image, value)]

    def holder(self, image, symbol, value):
        """The source of image's that holds symbol at value: the one whose row
        holds value. Where no row does and value lies in bss, it is the one C
        source of image's that defines symbol, as a source defines under GCC
        the storage the tables place where no source holds it
        (GCC.md#guards). None where neither gives one. Two C sources
        that define symbol fail, naming both."""
        config = self.image(image)
        held = next((source for source, _, start, end in config.placements if start <= value < end), None)
        if held is not None or not config.zeroed(value):
            return held
        found = [source for source in config.sources() if source.endswith('.c') and not source.startswith('gen/')
                 and symbol in self.definitions(source)]
        if len(found) > 1:
            fail(f'{image}: {symbol}, at 0x{value:08x}, where no row of {config.splits} lies: '
                 + ' and '.join(found) + ' define it')
        return found[0] if found else None

    def typed(self, image, symbol, value):
        """The type, as (ctype, whether it is an array), that image's
        definition of symbol at value gives it: ('code', False) in its code;
        for a C source holder() gives, that source's definition; or for a
        gen/ range, the type the image's row of data.txt at value gives, as
        row_type() spells it. None where none of them gives one. The files it
        reads join read. It fails where holder() does, or where the image's
        data.txt does."""
        config = self.image(image)
        if any(start <= value < end for ranges in config.code() for start, end in ranges):
            return 'code', False
        holder = self.holder(image, symbol, value)
        if holder and holder.endswith('.c') and not holder.startswith('gen/'):
            defined = self.definitions(holder).get(symbol)
            return defined and defined[:2]
        if holder and holder.startswith('gen/'):
            if image not in self.rows:
                self.read.add(config.path / 'data.txt')
                self.rows[image] = {start: (row_type(kind), count is not None)
                                    for start, kind, count, _, _ in config.definitions()}
            return self.rows[image].get(value)
        return None

    def text(self, value):
        """(source, name, length) for the text a C source of the content's
        image declares at value, or None where no C source's row holds value
        or its source declares no text there. tools/data.py --bodies records
        each text in the source's bodies.txt, at its address in the source's
        home, the first image by name that links it, and the text lies as far
        into the source's ranges here as there, as tools/bodies.py places a
        body. The files it reads join read. Fails where a body source's
        bodies.txt is missing or malformed, or where bodies.py's home()
        fails."""
        source = next((source for source, _, start, end in self.config.placements if start <= value < end), None)
        if source is None or not source.endswith('.c') or source.startswith('gen/'):
            return None
        if source not in self.texts:
            self.read.add(Path(source))
            self.texts[source] = {}
            if bodies.has_bodies(source):
                path = Path('build/gen', source).with_suffix('') / 'bodies.txt'
                if self.homes is None:
                    self.homes = configs()
                    self.read |= {config.splits for config in self.homes}
                self.read.add(path)
                ranges = self.config.held()[source]
                held = bodies.home(source, ranges, self.config.splits, self.homes)
                self.texts[source] = {bodies.placed(offset, ranges): (name, size)
                                      for offset, size, text, name in bodies.bodies(source, path, held) if text}
        found = self.texts[source].get(value)
        return found and (source, *found)

    def definitions(self, source):
        """Each datum a C source declares, a file's or a function's static, by
        name, as DECLARATION reads them: its type, its qualifiers and any
        pointer's * among it, whether it is an array, and its extents. The
        source joins read."""
        if source not in self.defined:
            self.read.add(Path(source))
            text = uncommented(content(source).decode(errors='replace'))
            self.defined[source] = {found: (qualify(qualifiers, ctype) + (' *' if pointer else ''), bool(dims), dims)
                                    for qualifiers, ctype, pointer, found, dims
                                    in (match.groups() for match in matches(DECLARATION, text))}
        return self.defined[source]

    def element(self, name, start, value):
        """The index of the element of name's definition that starts at
        value, where the tables give name at start, or None. The definition
        must be an array of one extent, in the C source holder() gives. That
        source's Types lay out the element and evaluate the extent, which
        must hold the element. The files it reads join read."""
        held = self.provider(name, start)
        if held is None:
            return None
        holder = self.holder(*held, start)
        if not holder or not holder.endswith('.c') or holder.startswith('gen/'):
            return None
        ctype, array, dims = self.definitions(holder).get(held[1], (None, False, ''))
        extents = EXTENT.findall(dims)
        if not array or len(extents) != 1:
            return None
        if holder not in self.source_types:
            texts = {}
            included(Path(holder), texts)
            self.read |= set(texts)
            self.source_types[holder] = Types(list(texts.values()))
        types = self.source_types[holder]
        try:
            index, rest = divmod(value - start, element_size(ctype, types))
            count = types.evaluate(spelled(extents[0]))
        except LayoutError:
            return None
        return index if not rest and index < count else None

    def __call__(self, name, value, pointees):
        """(image, symbol, ctype, whether it is an array) for a name the
        content takes at value, whose pointers take it as pointees. The type
        is its definition's, or where none is known, the one type its pointers
        take it as other than void and char, which is an array unless it is
        the type of a kind that is one record. Fails where no image or several
        define the name there, naming them, or where no definition types it
        and its pointers, void and char aside, take it as no type or as
        several."""
        owners = self.owners(name, value)
        if len(owners) != 1:
            fail(f'{self.config.name}: {name}, at 0x{value:08x}, which its content takes: '
                 + (' and '.join(image for image, _ in owners) + ' define it there' if owners else
                    'no image defines it there'))
        held, = owners
        typed = self.typed(*held, value)
        if typed is None:
            types = {SAME.get(pointee, pointee) for pointee in pointees} - {'void', 'char'}
            if len(types) != 1:
                fail(f'{self.config.name}: {name}, at 0x{value:08x}, which its content takes from {held[0]}: no '
                     'definition or pointer types it')
            ctype, = types
            typed = ctype, ctype not in SINGLE
        return (*held, *typed)


def row_type(kind):
    """The C type a row of data.txt gives its definition, past its count: a
    kind's, an mdata's by its factors, or the C type itself."""
    mdata = MDATA_KIND.fullmatch(kind)
    return f'NJS_MDATA{mdata[1]}' if mdata else KINDS[kind].ctype if kind in KINDS else kind


def owned_names(path):
    """A symbols.txt's names, as {address: set of names}."""
    found = {}
    for address, name in named(path):
        found.setdefault(address, set()).add(name)
    return found


class Placer(Writer):
    """A Writer that writes no C, and records the address of each word it
    would write as a pointer."""

    def __init__(self, items):
        super().__init__(items, {})
        self.placed = set()

    def ref(self, address, pointee):
        self.placed.add(address)
        return '0'

    def float(self, address):
        return '0'


def pointer_words(name, path):
    """The addresses of the words that the content of image name, at path,
    places as pointers. They are each pointer, null or not, of a structure
    of a kind data.txt gives or of a C type it gives, as definitions() gives
    them, and no word of an array of pointers or mixed words. Fails on the
    image's first row that vouches, whose words the content would place as
    pointers too, where definitions() does, and where Items refuses the
    items or the emitter can't read a word."""
    config = Config(name)
    image = config.image(path)
    ranges = config.generated()
    items = definitions(config, image)
    vouched = next((item.vouched for item in items if item.vouched), None)
    if vouched:
        fail(f'{vouched}: vouched, in {config.name}, whose pointers the census checks without it')
    named = {address: name for address, name in config.names.items()
             if any(low <= address < high for low, high in ranges)}
    try:
        writer = Placer(Items(image, items, ranges, named))
        for item in items:
            if item.kind not in GENERIC and item.kind != 'string':
                writer.values(item)
    except EmitError as error:
        fail(error)
    return writer.placed


def check(image, ranges, placed, copies):
    """The content's check, as (script, bytes): the script of a link that
    places each definition and copy at its address, and the bytes that link
    must give. ranges are the content's, each (start, end). placed holds
    Content.placed()'s rows, and copies Content's copies.

    The link places the definitions in a section for each range and copy,
    and each name the content takes from outside at its address. The bytes
    are the image's over each range and copy, and zeros between."""
    defined = sorted((address, name) for name, address, size, _ in placed if size is not None)
    script = [f"/* {Path(image.name).name}'s content at its addresses, as tools/content.py writes it for make gcc */",
              'SECTIONS', '{']
    # SUBALIGN(1) places each definition's input section at the offset the
    # script gives it, whatever its alignment. Each section's gaps take its
    # fill byte: the image's for a range, 0 for a copy
    spans = [(low, high, f'{image.fill:#04x}') for low, high in ranges]
    spans += [(value, value + len(copied) + 1, '0x00') for value, (_, _, copied) in sorted(copies.items())]
    for low, high, fill in spans:
        script.append(f'  .{low:08x} 0x{low:08x} : SUBALIGN(1) {{')
        script += [f'    . = 0x{address - low:x}; *(.data.{name} .rodata.{name})'
                   for address, name in defined[bisect_left(defined, (low,)):bisect_left(defined, (high,))]]
        script.append(f'    . = 0x{high - low:x};\n  }} ={fill}')
    # the sections GCC writes empty, and those SH's backend makes for
    # relocations against names the script assigns, none of which may hold a
    # byte; and the compiler's name and DWARF's sections, which load nowhere
    script += ['  .empty : { *(.text .data .bss .rodata .rela.*) }', '  /DISCARD/ : { *(.comment) *(.debug_*) }', '}',
               'ASSERT(SIZEOF(.empty) == 0, "the content holds data no definition places")',
               *(f'{"" if what.startswith("unresolved:") else "_"}{name} = 0x{address:08x};'
                 for name, address, size, what in placed if size is None)]
    first, last = min(low for low, _, _ in spans), max(high for _, high, _ in spans)
    expected = bytearray(last - first)
    for low, high, _ in spans:
        expected[low - first:high - first] = payload(image, low, high)
    return '\n'.join(script) + '\n', bytes(expected)


def gcc_content(path, out):
    """Write OUT, the C make gcc compiles for IMAGE's content: the data its
    gen/ ranges hold, no source's, as Content writes it, each definition as
    definitions() gives it from data.txt. Beside OUT, write:

    - .txt, a row for each of Content.placed();
    - .ld and .bin, the content's check, as check() gives them;
    - .d, make's rule for OUT, from each file it reads.

    Fails where:

    - the image has no gen/ range;
    - a gen/ range lies in a section GEN_SECTIONS doesn't name, or outside
      the image;
    - definitions(), keyed(), Providers or the emitter fails."""
    out = Path(out)
    config = Config(Path(path).stem.lower())
    rows = sorted((low, high, section) for source, section, low, high in config.placements
                  if source.startswith('gen/'))
    if not rows:
        fail(f'{config.splits}: no gen/ range, so no content')
    image = config.image(path)
    for low, high, section in rows:
        if section not in GEN_SECTIONS:
            fail(f'{config.splits}: 0x{low:08x} to 0x{high:08x} lies in {section}, and the C it takes defines '
                 'data for D, DHEAD or C')
        if not image.inside(low, high - low):
            fail(f'{config.splits}: 0x{low:08x} to 0x{high:08x} lies outside {path}, '
                 f'0x{image.base:08x} to 0x{image.end:08x}')
    keys = keyed(image) if config.path == FIRST else {}
    ranges = [(low, high) for low, high, _ in rows]
    names = config.names
    declarations = config.declarations
    providers = Providers(config)
    items = [Blob(item) if item.kind in BLOBS else item for item in definitions(config, image)]
    named = {address: name for address, name in names.items() if any(low <= address < high for low, high in ranges)}
    writer = Content(Items(image, items, ranges, named), names,
                     [(low, high, GEN_SECTIONS[section][1]) for low, high, section in rows], keys, providers)
    headers = declarations.headers(items)
    text = writer.source(f"{Path(path).name}'s content, as tools/content.py writes it for GCC", headers).text
    placed = writer.placed()
    script, expected = check(image, ranges, placed, writer.copies)
    read = {Path(path), config.splits, config.path / 'symbols.txt', config.path / 'data.txt', FIRST / 'symbols.txt',
            *declarations.texts, *providers.read}
    read |= {each for each in (IMPORTS, STAGE_LOADER, STAGE_HEADER) if each.exists()}
    listed = sorted(each.as_posix() for each in read)
    publish(out.with_suffix('.d'), f'{out}: {" ".join(listed)}\n' + ''.join(f'{each}:\n' for each in listed))
    publish(out.with_suffix('.txt'), ''.join(f'{name}  0x{address:08x}  {"-" if size is None else size}  {what}\n'
                                             for name, address, size, what in placed))
    publish(out.with_suffix('.ld'), script)
    publish(out.with_suffix('.bin'), expected)
    publish(out, text)


def main(argv):
    parser = argparse.ArgumentParser(prog='content', description=__doc__.split('\n\n')[0])
    parser.add_argument('image', metavar='IMAGE', help="an image's file, uncompressed")
    parser.add_argument('out', metavar='OUT', help="where to write the content's C, with its record, link script and "
                                                   'bytes beside it')
    args = parser.parse_args(argv)
    try:
        gcc_content(args.image, args.out)
    except (LayoutError, EmitError, ReadError) as error:
        fail(error)
    except OSError as error:
        # a file the tool reads or writes itself, past content() and publish()
        fail(f'{error.filename}: {error.strerror}')


if __name__ == '__main__':
    main(sys.argv[1:])
