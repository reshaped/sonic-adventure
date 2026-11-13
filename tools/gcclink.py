"""What the stages of make dc's link share: the spec and its reader, the
names the link binds, what it takes from an archive, the files of its
passes, and the toolchain.

tools/spec.py plans the link and writes the spec. tools/ld.py links each
image by it, tools/verify.py checks the link, and tools/publish.py writes
each image's file. Each of those three reads the spec through read(), as
tools/manifest.py does. tools/census.py reads the exports() of make dc's
objects, before the link.

The spec is a table of the images and what each links. An image's own row
comes before its other rows. A # starts a comment, which runs to the line's
end, and a blank line is skipped. The rows take these forms:

    image     IMAGE  ORIGIN  LENGTH  [ENTRY]
    input     IMAGE  PATH
    import    IMAGE  ALIAS  PROVIDER  SYMBOL  VIEW
    alias     IMAGE  ALIAS  SYMBOL
    constant  IMAGE  SYMBOL  VALUE
    head      IMAGE  SECTION  PATH  INPUT
    tail      IMAGE  SECTION  PATH  INPUT
    kernel    IMAGE  KERNEL  PATH  INPUT
    window    IMAGE  KERNEL  LINES  [OFFSET]
    warm      IMAGE  PATH  INPUT
    data      IMAGE  PATH  INPUT
    over      IMAGE  KERNEL  KERNEL  [OTHER]
    last      IMAGE  PATH
    cold      IMAGE  PATH
    anchor    IMAGE  SYMBOL  OFFSET  [SIZE]
    heat      IMAGE  PATH  HASH
    text      IMAGE  ADDRESS
    searched  IMAGE  PATH  INPUT  PLACE  SIZE
    starts    IMAGE  OUTPUT  [OUTPUT]  [OUTPUT]

An image loads at ORIGIN, into a load area LENGTH bytes long. It runs from
ENTRY, or from ORIGIN where its row names none. Its input rows give its
objects and archives, in link order.

An import binds ALIAS, a name the image's inputs take from outside, to
SYMBOL as PROVIDER defines it. RAM's 16 MiB appear at both 0x0c000000 and
0x8c000000, and the game uses both (TOOLCHAIN.md#images). VIEW, 0x0c or
0x8c, says which of the two addresses ALIAS takes.

An alias makes ALIAS a second name for SYMBOL, as the image's own inputs
define it. A constant binds SYMBOL to VALUE, an address.

A head, tail, kernel, warm or data row places INPUT, an input section of
PATH, one of the image's objects, where tools/ld.py's script says. A kernel
row's KERNEL names the kernel it is part of. A window row gives KERNEL a
window of LINES lines of 32 bytes, whose code starts OFFSET lines in, or
at its start where the row gives no OFFSET. One whose KERNEL is warm
gives the warm rows theirs. An over row lets the
two KERNELs' code take one set of the SH-4's instruction cache, as ld.py
says. Where the row gives OTHER, another image of the spec, the second
KERNEL is OTHER's. A last row puts the last code of PATH, one of the
image's objects, ahead of the image's other last code, and a cold row puts
all of PATH's code after the last code and its data after the last data,
as ld.py says. An anchor holds SYMBOL to lie OFFSET bytes past ORIGIN in
the final link. Where the row gives SIZE, SYMBOL must hold SIZE bytes.

A heat row, a text row and the searched rows are the image's record of
where its warm rows lie, which tools/order.py writes. The heat row says
the places were costed by the heat at PATH, a file whose SHA-256 was HASH,
the text row that the image's .text started at ADDRESS, and a searched
row that INPUT, an input section of PATH, lies PLACE bytes into that
.text, and took SIZE bytes. The searched rows give each kernel row's
section and each warm row's. ld.py lays each warm row's section at its
PLACE, and holds the link to the rows, as it says.

A starts row names each OUTPUT, an output section of ld.py's script that
holds the image's data, that starts on the 16 KB of the SH-4's operand
cache, as ld.py says.

IMAGE, KERNEL and OTHER are lowercase letters, digits and _. ALIAS,
SYMBOL and ENTRY are C names, without the object format's leading
underscore. SECTION is an SHC section's name: a capital, then capitals and
digits. INPUT is an SHC section's name or a GCC section's, which starts
with a dot. ORIGIN, LENGTH and VALUE are 0x and eight lowercase hex digits.
LINES, OFFSET, PLACE, SIZE and ADDRESS are decimal without a leading zero,
or 0x and lowercase hex digits. HASH is 64 lowercase hex digits. OUTPUT is
one of STARTS: .rodata, .data or .bss.

PATH is letters, digits, _, ., / and -, and doesn't start with -. ld reads
a wildcard in a file name of its script as a pattern, and a leading - on
its command line as an option.

read() fails, naming the spec, where it can't read it. A line that isn't
UTF-8 fails, naming its path:line. So does a row:

- of none of these forms;
- whose image no image row above it names, or a second image row for one;
- that binds a name its image's imports, aliases or constants bind already;
- that binds a boundary of the link;
- that gives its image a second heat row, text row or starts row, or a
  second searched row of one PATH and INPUT;
- that names an OUTPUT twice;
- whose PROVIDER or OTHER isn't another image of the spec.

The boundaries of the link are image_start and image_end, and each run's
bounds, _startof_X, _endof_X and _sizeof_X, where X is the run's name.
tools/ld.py's script defines them all. An image's exports are the
definitions it offers another to import, as exports() gives them.

tools/ld.py links each image twice, a layout pass and then a final pass,
under DIR/IMAGE/. Each pass writes its script, its map and its ELF there,
as pass_file() names them, and linked() reads its ELF and map back.
tools/verify.py and tools/publish.py read the final pass, whose ELF
final() names. selected() gives the inputs GNU ld takes, archive members
among them, for the roots rooted() gives each image. SH_ELF in the
environment names the toolchain's directory, tools/sh-elf by default.
installed() finds a file the toolchain installs once, as libgcc() finds
its libgcc.a.

Where make dc's objects carry GCC's intermediate code, tools/ld.py first
compiles each image's whole, and writes, under DIR/IMAGE/lto/, the code
GCC kept of each object, at the object's own path there: its link-time
object. taken() reads the record of them, and link_time() gives the images
as GNU ld links them then.
"""

import os
import re
from collections import namedtuple
from pathlib import Path

import elf
from tables import NAME as C_NAME, SECTION, address, hexa, rows
from tool import content, fail

SH_ELF = Path(os.environ.get('SH_ELF', 'tools/sh-elf'))
# RAM's size, and where each of its two views starts, by VIEW's spelling
RAM = 0x01000000
VIEWS = {'0x0c': 0x0C000000, '0x8c': 0x8C000000}
# the C names the script defines at an image's start and end
IMAGE_BOUNDS = ('image_start', 'image_end')
# the word that names each of a run's bounds: its start, its end and its size
RUN_BOUNDS = ('startof', 'endof', 'sizeof')
# a run's bound, as the object format spells it: __startof_X, __endof_X or
# __sizeof_X, where X is the run's name
BOUND = re.compile(rf'__({"|".join(RUN_BOUNDS)})_({SECTION.pattern})')
# the start of the name of each section of the last code, which tools/ld.py
# links after all of an image's other code: one function's, as port.h's
# LINKED_LAST names it
LAST = '.text_last.'
# the same for the last data, which tools/ld.py links after all of an image's
# other data: one datum's, as port.h's LINKED_LAST_DATA names it
LAST_DATA = '.data_last.'
# the spellings of the spec's fields, with C_NAME and SECTION from tables.py
IMAGE = re.compile(r'[a-z0-9_]+')
NUMBER = re.compile(r'0x[0-9a-f]+|0|[1-9][0-9]*')
PATH = re.compile(r'[A-Za-z0-9_./][A-Za-z0-9_./-]*')
INPUT = re.compile(rf'\.[A-Za-z0-9_.]+|{SECTION.pattern}')
# the output sections of tools/ld.py's script that hold an image's data, in
# script order: those a starts row can name
STARTS = ('.rodata', '.data', '.bss')
# each kind of row's form, by the kind
FORMS = {'image': 'image IMAGE ORIGIN LENGTH [ENTRY]', 'input': 'input IMAGE PATH',
         'import': 'import IMAGE ALIAS PROVIDER SYMBOL VIEW', 'alias': 'alias IMAGE ALIAS SYMBOL',
         'constant': 'constant IMAGE SYMBOL VALUE', 'head': 'head IMAGE SECTION PATH INPUT',
         'tail': 'tail IMAGE SECTION PATH INPUT', 'kernel': 'kernel IMAGE KERNEL PATH INPUT',
         'window': 'window IMAGE KERNEL LINES [OFFSET]', 'warm': 'warm IMAGE PATH INPUT',
         'data': 'data IMAGE PATH INPUT',
         'over': 'over IMAGE KERNEL KERNEL [OTHER]', 'last': 'last IMAGE PATH', 'cold': 'cold IMAGE PATH',
         'anchor': 'anchor IMAGE SYMBOL OFFSET [SIZE]', 'heat': 'heat IMAGE PATH HASH', 'text': 'text IMAGE ADDRESS',
         'searched': 'searched IMAGE PATH INPUT PLACE SIZE', 'starts': 'starts IMAGE OUTPUT [OUTPUT] [OUTPUT]'}
# each field's spelling, by its name in FORMS. PROVIDER, ORIGIN, LENGTH and
# VALUE take any spelling here, and read() checks them
SPELLINGS = {'IMAGE': IMAGE, 'KERNEL': IMAGE, 'OTHER': IMAGE, 'ALIAS': C_NAME, 'SYMBOL': C_NAME, 'ENTRY': C_NAME,
             'PATH': PATH, 'SECTION': SECTION, 'INPUT': INPUT, 'VIEW': re.compile('|'.join(VIEWS)), 'LINES': NUMBER,
             'OFFSET': NUMBER, 'SIZE': NUMBER, 'PLACE': NUMBER, 'ADDRESS': NUMBER, 'HASH': re.compile(r'[0-9a-f]{64}'),
             'OUTPUT': re.compile('|'.join(map(re.escape, STARTS)))}
# a line of ld's map that places an input section: its name, address, size
# and file. ld puts a name too long for its column on a line of its own,
# which placements() joins to the next
PLACED = re.compile(r' ([^\s*]\S*) +0x([0-9a-f]+) +0x([0-9a-f]+) (.+)')

# the section index of a common symbol, SHN_COMMON, and the type of a
# function's symbol, STT_FUNC
COMMON, FUNCTION = 0xFFF2, 2
# each input linkables() has read, by its elf.stamp()
LINKABLES = {}
# what the link selects an input by, as linkable() reads it: the symbols it
# defines; those it leaves undefined by a global symbol, its strong
# references; and those it leaves undefined by a weak one, its weak
# references. Of the symbols it defines, common is those it holds as common
# symbols, strong_definitions those a global symbol defines otherwise, and
# strong_data those of them whose symbol is no function's. Each is a set of
# names as the symbol table spells them, and a Linkable built without the
# last three has none of them
Linkable = namedtuple('Linkable', 'defined strong_references weak_references common strong_definitions strong_data',
                      defaults=(frozenset(),) * 3)

# an image as read() gives it. inputs is its PATHs, in link order. imports
# maps each ALIAS to its Import, aliases each ALIAS to its SYMBOL, and
# constants each SYMBOL to its VALUE. heads, tails and kernel are its
# Pieces, over its over rows' pairs of KERNELs, each followed by OTHER where
# the row gives it, anchors its Anchors, windows its Windows, data its data
# rows' Pieces, last its last rows' PATHs, cold its cold rows', warm its
# warm rows' Pieces and searched its Searcheds, each in row order. heat
# holds its heat row's PATH and HASH, or nothing where it has none, and
# text is its text row's ADDRESS, or None where it has none. starts holds
# its starts row's OUTPUTs, in row order, and none where it has no such
# row, as an Image built without the field has none
Image = namedtuple('Image', 'name origin length entry inputs imports aliases constants heads tails kernel over '
                   'anchors windows data last cold warm searched heat text starts', defaults=(None, ()))
Import = namedtuple('Import', 'provider symbol view')
# a head or tail row's SECTION, PATH and INPUT, a kernel row's KERNEL, PATH
# and INPUT, or a warm or data row's PATH and INPUT. group is SECTION or
# KERNEL, whose rows stand together, or warm or data
Piece = namedtuple('Piece', 'group path input')
# a window row's KERNEL, LINES and OFFSET. offset is 0 where the row gives
# no OFFSET
Window = namedtuple('Window', 'kernel lines offset')
# an anchor row's SYMBOL, OFFSET and SIZE. size is None where the row gives
# no SIZE
Anchor = namedtuple('Anchor', 'symbol offset size')
# a searched row's PATH, INPUT, PLACE and SIZE
Searched = namedtuple('Searched', 'path input place size')
# a pass's link of an image: its Elf and its map's placements
Linked = namedtuple('Linked', 'elf placements')
# an input section the map places: its output section, its file as
# map_name() spells it, its name, its address and its size
Placed = namedtuple('Placed', 'output file section address size')


def fits(kind, fields):
    """Whether fields, a row's fields after its kind, take the form FORMS
    gives kind, each spelled as SPELLINGS gives. A kind FORMS doesn't give
    has no form."""
    if kind not in FORMS:
        return False
    form = FORMS[kind].split()[1:]
    least = sum(not field.startswith('[') for field in form)
    spellings = [SPELLINGS.get(field.strip('[]')) for field in form]
    return least <= len(fields) <= len(form) and all(
        spelling is None or spelling.fullmatch(field) for spelling, field in zip(spellings, fields))


def code(name):
    """Whether name, an input section's, names code: .text, a .text.* or
    .text_last.* section or one of SHC's P sections."""
    return (name == '.text' or name.startswith(('.text.', LAST))
            or (SECTION.fullmatch(name) is not None and name[0] == 'P'))


def boundary(name):
    """Whether name, a C name, is a boundary of the link: image_start,
    image_end or a run's bound."""
    return name in IMAGE_BOUNDS or BOUND.fullmatch(f'_{name}') is not None


def read(path):
    """The images of the spec at path, as Images by name, in the spec's
    order. A row fails, naming its path:line, as the module's docstring
    lists."""
    # others holds each row that names another image: its path:line, its
    # image and the other image
    images, others = {}, []
    for where, (kind, *fields) in rows(path):
        if kind in FORMS and kind != 'image' and fields and fields[0] not in images:
            fail(f'{where}: {fields[0]}: no image row above names it')
        if kind == 'input' and len(fields) == 2 and not PATH.fullmatch(fields[1]):
            fail(f'{where}: {fields[1]}: not a PATH: letters, digits, _, ., / and -, not starting with -')
        if not fits(kind, fields):
            forms = list(FORMS.values())
            fail(f'{where}: not a row: {", ".join(forms[:-1])}, or {forms[-1]}')
        if kind == 'image':
            if fields[0] in images:
                fail(f'{where}: {fields[0]} has an image row already')
            images[fields[0]] = Image(fields[0], address(fields[1], where), address(fields[2], where),
                                      fields[3] if len(fields) == 4 else None, inputs=[], imports={}, aliases={},
                                      constants={}, heads=[], tails=[], kernel=[], over=[], anchors=[], windows=[],
                                      data=[], last=[], cold=[], warm=[], searched=[], heat=[], text=None,
                                      starts=[])
            continue
        image = images[fields[0]]
        if kind in ('import', 'alias', 'constant'):
            if fields[1] in {*image.imports, *image.aliases, *image.constants}:
                fail(f'{where}: {image.name} binds {fields[1]} already')
            if boundary(fields[1]):
                fail(f'{where}: {fields[1]} is a boundary of the link')
        if kind == 'input':
            image.inputs.append(fields[1])
        elif kind == 'import':
            image.imports[fields[1]] = Import(*fields[2:])
            others.append((where, image.name, fields[2]))
        elif kind == 'alias':
            image.aliases[fields[1]] = fields[2]
        elif kind == 'constant':
            image.constants[fields[1]] = address(fields[2], where)
        elif kind in ('head', 'tail'):
            (image.heads if kind == 'head' else image.tails).append(Piece(*fields[1:]))
        elif kind == 'kernel':
            image.kernel.append(Piece(*fields[1:4]))
        elif kind == 'window':
            image.windows.append(Window(fields[1], int(fields[2], 0), int(fields[3], 0) if len(fields) == 4 else 0))
        elif kind in ('warm', 'data'):
            (image.warm if kind == 'warm' else image.data).append(Piece(kind, *fields[1:3]))
        elif kind == 'over':
            image.over.append(tuple(fields[1:]))
            if len(fields) == 4:
                others.append((where, image.name, fields[3]))
        elif kind in ('last', 'cold'):
            (image.last if kind == 'last' else image.cold).append(fields[1])
        elif kind == 'heat':
            if image.heat:
                fail(f'{where}: {image.name} has a heat row already')
            image.heat.extend(fields[1:])
        elif kind == 'text':
            if image.text is not None:
                fail(f'{where}: {image.name} has a text row already')
            images[image.name] = image._replace(text=int(fields[1], 0))
        elif kind == 'searched':
            if any(row[:2] == tuple(fields[1:3]) for row in image.searched):
                fail(f"{where}: {image.name} has a searched row of {fields[1]}'s {fields[2]} already")
            image.searched.append(Searched(fields[1], fields[2], int(fields[3], 0), int(fields[4], 0)))
        elif kind == 'starts':
            if image.starts:
                fail(f'{where}: {image.name} has a starts row already')
            twice = next((output for output in fields[1:] if fields[1:].count(output) > 1), None)
            if twice:
                fail(f'{where}: names {twice} twice')
            image.starts.extend(fields[1:])
        else:
            image.anchors.append(Anchor(fields[1], int(fields[2], 0), int(fields[3], 0) if len(fields) == 4 else None))
    for where, name, other in others:
        if other not in images or other == name:
            fail(f'{where}: {other}: not another image of the spec')
    return images


def in_view(value, view, where):
    """The address in view, a key of VIEWS, of the byte at value, an address
    in either view of RAM. A value in neither view fails, naming where."""
    for base in VIEWS.values():
        if base <= value < base + RAM:
            return VIEWS[view] + value - base
    fail(f'{where}: {hexa(value)} is outside RAM, so the {view} view has no address for it')


def exports(found):
    """found's exports, each as its Symbol, by its C name. found is an Elf.
    An export is a global or weak symbol that names a definition in an
    allocated section, and whose name starts with the object format's
    underscore. The absolute values a script assigns lie in no section, so
    none is an export. image_start, image_end and each run's start and end
    lie in allocated sections, and are left out: they are boundaries of the
    link, not exports."""
    allocated = elf.allocated_sections(found)
    return {symbol.name[1:]: symbol for symbol in found.symbols
            if symbol.bind in (elf.GLOBAL, elf.WEAK) and symbol.section in allocated
            and symbol.name.startswith('_') and elf.names_definition(symbol) and not boundary(symbol.name[1:])}


def placements(text, where):
    """Each input section that text places in its memory map, as a Placed,
    in map order. text is ld's map. It fails, naming where, on a text
    without a memory map, and on one that doesn't end with the line that
    names its output."""
    found, output, parts = [], None, text.split('\nLinker script and memory map\n', 1)
    if len(parts) != 2 or not re.search(r'\nOUTPUT\(.*\)\n\Z', parts[1]):
        fail(f"{where}: not a whole map of GNU ld's")
    for line in re.sub(r'^( [^\s*]\S*)\n(?= +0x)', r'\1', parts[1], flags=re.M).splitlines():
        if line[:1].strip():
            output = line.split()[0]
        placed = PLACED.fullmatch(line)
        if placed:
            found.append(Placed(output, placed[4].strip(), placed[1], int(placed[2], 16), int(placed[3], 16)))
    return found


def map_name(path, member):
    """The name ld's map gives an input: path for an object, and
    path(MEMBER) for an archive's member."""
    return str(path) if member is None else f'{path}({member})'


def linkables(path):
    """The input at path, as a list of (member, Linkable), read once while
    its elf.stamp() holds. It keeps no Elf, but takes one elf.opened() has
    read already. It fails as elf.stamp() and elf.unpacked() do."""
    key = elf.stamp(path)
    if key not in LINKABLES:
        LINKABLES[key] = [(member, linkable(found)) for member, found in elf.held(key) or elf.unpacked(path)]
    return LINKABLES[key]


def inputs(image):
    """Each of image's inputs as (path, member, Elf), in link order.
    elf.opened() reads each."""
    return [(path, member, found) for path in image.inputs for member, found in elf.opened(path)]


def linkable(found):
    """The Linkable of found, an input's Elf. The input defines each global
    or weak symbol in a section, absolute or common: each whose section
    index isn't SHN_UNDEF. It refers to each global or weak symbol whose
    index is. It holds as a common symbol each it defines whose index is
    SHN_COMMON. Its strong definitions are the others that a global symbol
    defines, and its strong data those of them whose symbol's type isn't
    STT_FUNC."""
    defines = [symbol for symbol in found.symbols
               if symbol.bind in (elf.GLOBAL, elf.WEAK) and symbol.section != elf.UNDEFINED and symbol.name]
    refers = [symbol for symbol in found.symbols
              if symbol.bind in (elf.GLOBAL, elf.WEAK) and symbol.section == elf.UNDEFINED and symbol.name]
    strong = [symbol for symbol in defines if symbol.bind == elf.GLOBAL and symbol.section != COMMON]
    return Linkable({symbol.name for symbol in defines},
                    {symbol.name for symbol in refers if symbol.bind == elf.GLOBAL},
                    {symbol.name for symbol in refers if symbol.bind == elf.WEAK},
                    {symbol.name for symbol in defines if symbol.section == COMMON},
                    {symbol.name for symbol in strong},
                    {symbol.name for symbol in strong if symbol.kind != FUNCTION})


def selected(paths, roots=()):
    """What GNU ld takes from the inputs at paths, as a pair. paths are
    objects and archives, in link order. The link gives ld EXTERN for each
    of roots.

    The first is each input the link takes, as (path, member), in the order
    it takes them. The second maps each symbol no taken input defines to the
    inputs that refer to it, each by its map_name().

    The link takes each object. Of an archive, it takes each member that
    defines a symbol a root or a strong reference wants and nothing defines
    yet. It takes each member, too, whose strong data define a symbol that
    an input taken so far holds as a common symbol, and that none defines
    by a strong definition, as linkable() gives them: the member's
    definition then replaces the common symbol. It reads the members in
    file order, and again until it takes no more. A weak reference takes
    no member.

    It fails as linkables() does on each input."""
    taken, defined, wanted = [], set(), {}
    strong = {f'_{root}' for root in roots}
    # the symbols the inputs taken so far hold as common symbols, and those
    # their strong definitions define, which are common no longer
    common, replaced = set(), set()

    def take(path, member, facts):
        taken.append((path, member))
        defined.update(facts.defined)
        common.update(facts.common)
        replaced.update(facts.strong_definitions)
        for symbol in [*facts.strong_references, *facts.weak_references]:
            wanted.setdefault(symbol, []).append(map_name(path, member))
            if symbol in facts.strong_references:
                strong.add(symbol)

    for path in paths:
        parts = linkables(path)
        if parts and parts[0][0] is None:
            take(path, *parts[0])
            continue
        left, more = list(parts), True
        while more:
            more = False
            for member, facts in list(left):
                if facts.defined & (strong - defined) or facts.strong_data & (common - replaced):
                    take(path, member, facts)
                    left.remove((member, facts))
                    more = True
    return taken, {symbol: wheres for symbol, wheres in wanted.items() if symbol not in defined}


def rooted(images):
    """Each image's roots, by its name: the SYMBOLs other images import from
    it, and the SYMBOLs of its aliases. Each comes once, where it first
    appears: images in the spec's order, and each image's imports, then its
    aliases, in row order."""
    roots = {name: {} for name in images}
    for image in images.values():
        for wanted in image.imports.values():
            roots[wanted.provider][wanted.symbol] = True
        for symbol in image.aliases.values():
            roots[image.name][symbol] = True
    return {name: list(found) for name, found in roots.items()}


def installed(directory, name):
    """The path of name in the one directory of directory, a path under
    SH_ELF, that holds it, as the toolchain installs a file for its one
    release. It fails, naming directory, where none holds it, and where
    several do, naming each."""
    found = sorted(SH_ELF.glob(f'{directory}/*/{name}'))
    if not found:
        fail(f'{SH_ELF}/{directory}/: no {name}, which the toolchain installs')
    if len(found) > 1:
        fail(f"{SH_ELF}/{directory}/: holds {len(found)} {name}, {', '.join(map(str, found))}, and make dc's "
             'toolchain installs one')
    return found[0]


def libgcc():
    """The libgcc.a that SH_ELF's toolchain installs, for its one release
    and multilib, in a directory of lib/gcc/sh-elf/. It fails, naming that
    directory, where the toolchain holds none. It fails where the toolchain
    holds several, naming each: which one the compiler links doesn't follow
    the directories' order."""
    return installed('lib/gcc/sh-elf', 'libgcc.a')


def pass_file(out, name, stage, suffix):
    """The file of the pass named stage of the image named name, under out:
    out/NAME/STAGE and suffix. suffix is .ld for its script, .map for its
    map and .elf for its ELF."""
    return Path(out, name, stage + suffix)


def final(out, name):
    """The final pass's ELF of the image named name under out, as ld.py
    writes it: out/NAME/final.elf."""
    return pass_file(out, name, 'final', '.elf')


# the directory under an image's own, DIR/IMAGE/, that holds its link-time
# objects, and the record of them there: a row for each object the
# link-time compile took, its path, then its link-time object's, or - where
# GCC kept nothing of it
LINK_TIME, OBJECTS = 'lto', 'objects.txt'


def taken(out, name):
    """What out/NAME/lto/objects.txt records of the link-time compile of
    the image named name: the path of its link-time object, or None where
    GCC kept nothing of it, for each object the compile took, by the
    object's path, in the record's order. It is empty where out holds no
    record, as after a link with no compile.

    It fails, naming the record's path:line, on a line that isn't a path
    and then a path or -, and on a second line for a path, and as
    content() does."""
    record, found = Path(out, name, LINK_TIME, OBJECTS), {}
    if not record.is_file():
        return found
    for number, line in enumerate(content(record).decode(errors='replace').splitlines(), 1):
        fields = line.split()
        if len(fields) != 2 or not all(field == '-' or PATH.fullmatch(field) for field in fields) \
                or fields[0] == '-':
            fail(f"{record}:{number}: not a row: an object's PATH, then its link-time object's PATH, or - where "
                 'GCC kept nothing of it')
        if fields[0] in found:
            fail(f'{record}:{number}: {fields[0]} has a row already')
        found[fields[0]] = None if fields[1] == '-' else fields[1]
    return found


def link_time(images, out):
    """images as GNU ld links them once tools/ld.py's link-time compile has
    run under out, by name. Where taken() gives an image link-time objects,
    each takes its object's place among the inputs, and an object GCC kept
    nothing of leaves them. A head or tail row of such an object places
    each section of its link-time object that is INPUT, or whose name is
    INPUT's then a dot: a section's data, each datum's section on its own.
    A head or tail row that places nothing then goes. A kernel, warm, data,
    last or cold row stands as it is: its object keeps its own code, and
    the compile takes none that carries the intermediate code, as ld.py's
    whole() says.

    It fails as taken() does. It fails, naming the image, on a head or
    tail row whose INPUT is no allocated section of its object, and as
    elf.opened() does."""
    found = {}
    for name, image in images.items():
        made = taken(out, name)
        found[name] = image._replace(
            inputs=[made.get(path, path) for path in image.inputs if made.get(path, path)],
            heads=placed(name, made, image.heads, 'head'),
            tails=placed(name, made, image.tails, 'tail')) if made else image
    return found


def placed(name, taken, pieces, label):
    """pieces, the head or tail rows of the image named name, label saying
    which, as link_time() gives them, where taken maps each object the
    link-time compile took to its link-time object, or to None."""
    kept = []
    for piece in pieces:
        if piece.path not in taken:
            kept.append(piece)
            continue
        if piece.input not in allocated(piece.path):
            fail(f"{name}: its {label} row places {piece.path}'s {piece.input}, and that object holds no such "
                 'section')
        made = taken[piece.path]
        kept += [Piece(piece.group, made, section) for section in (allocated(made) if made else ())
                 if section == piece.input or section.startswith(f'{piece.input}.')]
    return kept


def allocated(path):
    """The names of the allocated sections of the object at path, in its
    order. It fails as elf.opened() does."""
    return [section.name for _, obj in elf.opened(path) for section in obj.sections if section.flags & elf.ALLOC]


def compiled_whole(out, path):
    """The object path stands for, where path is a link-time object of an
    image's under out, as link_time() gives it, or else None."""
    parts, base = Path(path).parts, Path(out).parts
    if len(parts) > len(base) + 2 and parts[:len(base)] == base and parts[len(base) + 1] == LINK_TIME:
        return str(Path(*parts[len(base) + 2:]))
    return None


def linked(out, name, stage):
    """The pass named stage of the image named name under out, as a Linked:
    its ELF and its map's placements. It fails as content() does, and as
    elf.read() and placements() do on the ELF and the map."""
    found, mapped = pass_file(out, name, stage, '.elf'), pass_file(out, name, stage, '.map')
    return Linked(elf.read(content(found), found), placements(content(mapped).decode('latin-1'), mapped))
