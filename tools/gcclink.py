"""What the stages of make gcc's link share: the spec and its reader, the
names the link binds, what it takes from an archive, the files of its
passes, and the toolchain.

tools/spec.py plans the link and writes the spec. tools/ld.py links each
image by it, tools/verify.py checks the link, and tools/publish.py writes
each image's file. Each of those three reads the spec through read(), as
tools/manifest.py does. tools/census.py reads the exports() of make gcc's
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
    kernel    IMAGE  PATH  INPUT
    anchor    IMAGE  SYMBOL  OFFSET  [SIZE]

An image loads at ORIGIN, into a load area LENGTH bytes long. It runs from
ENTRY, or from ORIGIN where its row names none. Its input rows give its
objects and archives, in link order.

An import binds ALIAS, a name the image's inputs take from outside, to
SYMBOL as PROVIDER defines it. RAM's 16 MiB appear at both 0x0c000000 and
0x8c000000, and the game uses both (TOOLCHAIN.md#images). VIEW, 0x0c or
0x8c, says which of the two addresses ALIAS takes.

An alias makes ALIAS a second name for SYMBOL, as the image's own inputs
define it. A constant binds SYMBOL to VALUE, an address.

A head, tail or kernel row places INPUT, an input section of PATH, one of
the image's objects, where tools/ld.py's script says. An anchor holds
SYMBOL to lie OFFSET bytes past ORIGIN in the final link. Where the row
gives SIZE, SYMBOL must hold SIZE bytes.

IMAGE is lowercase letters, digits and _. ALIAS, SYMBOL and ENTRY are C
names, without the object format's leading underscore. SECTION is an SHC
section's name: a capital, then capitals and digits. INPUT is an SHC
section's name or a GCC section's, which starts with a dot. ORIGIN, LENGTH
and VALUE are 0x and eight lowercase hex digits. OFFSET and SIZE are
decimal without a leading zero, or 0x and lowercase hex digits.

PATH is letters, digits, _, ., / and -, and doesn't start with -. ld reads
a wildcard in a file name of its script as a pattern, and a leading - on
its command line as an option.

read() fails, naming the spec, where it can't read it. A line that isn't
UTF-8 fails, naming its path:line. So does a row:

- of none of these forms;
- whose image no image row above it names, or a second image row for one;
- that binds a name its image's imports, aliases or constants bind already;
- that binds a boundary of the link;
- whose PROVIDER isn't another image of the spec.

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
# the spellings of the spec's fields, with C_NAME and SECTION from tables.py
IMAGE = re.compile(r'[a-z0-9_]+')
NUMBER = re.compile(r'0x[0-9a-f]+|0|[1-9][0-9]*')
PATH = re.compile(r'[A-Za-z0-9_./][A-Za-z0-9_./-]*')
INPUT = re.compile(rf'\.[A-Za-z0-9_.]+|{SECTION.pattern}')
# each kind of row's form, by the kind
FORMS = {'image': 'image IMAGE ORIGIN LENGTH [ENTRY]', 'input': 'input IMAGE PATH',
         'import': 'import IMAGE ALIAS PROVIDER SYMBOL VIEW', 'alias': 'alias IMAGE ALIAS SYMBOL',
         'constant': 'constant IMAGE SYMBOL VALUE', 'head': 'head IMAGE SECTION PATH INPUT',
         'tail': 'tail IMAGE SECTION PATH INPUT', 'kernel': 'kernel IMAGE PATH INPUT',
         'anchor': 'anchor IMAGE SYMBOL OFFSET [SIZE]'}
# each field's spelling, by its name in FORMS. PROVIDER, ORIGIN, LENGTH and
# VALUE take any spelling here, and read() checks them
SPELLINGS = {'IMAGE': IMAGE, 'ALIAS': C_NAME, 'SYMBOL': C_NAME, 'ENTRY': C_NAME, 'PATH': PATH, 'SECTION': SECTION,
             'INPUT': INPUT, 'VIEW': re.compile('|'.join(VIEWS)), 'OFFSET': NUMBER, 'SIZE': NUMBER}
# a line of ld's map that places an input section: its name, address, size
# and file. ld puts a name too long for its column on a line of its own,
# which placements() joins to the next
PLACED = re.compile(r' ([^\s*]\S*) +0x([0-9a-f]+) +0x([0-9a-f]+) (.+)')

# each input linkables() has read, by its elf.stamp()
LINKABLES = {}
# what the link selects an input by, as linkable() reads it: the symbols it
# defines; those it leaves undefined by a global symbol, its strong
# references; and those it leaves undefined by a weak one, its weak
# references. Each is a set of names as the symbol table spells them
Linkable = namedtuple('Linkable', 'defined strong_references weak_references')

# an image as read() gives it. inputs is its PATHs, in link order. imports
# maps each ALIAS to its Import, aliases each ALIAS to its SYMBOL, and
# constants each SYMBOL to its VALUE. heads, tails and kernel are its
# Pieces, and anchors its Anchors, each in row order
Image = namedtuple('Image', 'name origin length entry inputs imports aliases constants heads tails kernel anchors')
Import = namedtuple('Import', 'provider symbol view')
# a head or tail row's SECTION, PATH and INPUT. A kernel row's SECTION is
# None
Piece = namedtuple('Piece', 'section path input')
# an anchor row's SYMBOL, OFFSET and SIZE. size is None where the row gives
# no SIZE
Anchor = namedtuple('Anchor', 'symbol offset size')
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


def boundary(name):
    """Whether name, a C name, is a boundary of the link: image_start,
    image_end or a run's bound."""
    return name in IMAGE_BOUNDS or BOUND.fullmatch(f'_{name}') is not None


def read(path):
    """The images of the spec at path, as Images by name, in the spec's
    order. A row fails, naming its path:line, as the module's docstring
    lists."""
    images, providers = {}, []
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
                                      fields[3] if len(fields) == 4 else None, [], {}, {}, {}, [], [], [], [])
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
            providers.append((where, image.name, fields[2]))
        elif kind == 'alias':
            image.aliases[fields[1]] = fields[2]
        elif kind == 'constant':
            image.constants[fields[1]] = address(fields[2], where)
        elif kind in ('head', 'tail'):
            (image.heads if kind == 'head' else image.tails).append(Piece(*fields[1:]))
        elif kind == 'kernel':
            image.kernel.append(Piece(None, *fields[1:]))
        else:
            image.anchors.append(Anchor(fields[1], int(fields[2], 0), int(fields[3], 0) if len(fields) == 4 else None))
    for where, consumer, provider in providers:
        if provider not in images or provider == consumer:
            fail(f'{where}: {provider}: not another image of the spec')
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
    index is."""
    defined = {symbol.name for symbol in found.symbols
               if symbol.bind in (elf.GLOBAL, elf.WEAK) and symbol.section != elf.UNDEFINED and symbol.name}
    refers = [symbol for symbol in found.symbols
              if symbol.bind in (elf.GLOBAL, elf.WEAK) and symbol.section == elf.UNDEFINED and symbol.name]
    return Linkable(defined, {symbol.name for symbol in refers if symbol.bind == elf.GLOBAL},
                    {symbol.name for symbol in refers if symbol.bind == elf.WEAK})


def selected(paths, roots=()):
    """What GNU ld takes from the inputs at paths, as a pair. paths are
    objects and archives, in link order. The link gives ld EXTERN for each
    of roots.

    The first is each input the link takes, as (path, member), in the order
    it takes them. The second maps each symbol no taken input defines to the
    inputs that refer to it, each by its map_name().

    The link takes each object. Of an archive, it takes each member that
    defines a symbol a root or a strong reference wants and nothing defines
    yet. It reads the members in file order, and again until it takes no
    more. A weak reference takes no member.

    It fails as linkables() does on each input."""
    taken, defined, wanted = [], set(), {}
    strong = {f'_{root}' for root in roots}

    def take(path, member, facts):
        taken.append((path, member))
        defined.update(facts.defined)
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
                if facts.defined & (strong - defined):
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
        fail(f"{SH_ELF}/{directory}/: holds {len(found)} {name}, {', '.join(map(str, found))}, and make gcc's "
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


def linked(out, name, stage):
    """The pass named stage of the image named name under out, as a Linked:
    its ELF and its map's placements. It fails as content() does, and as
    elf.read() and placements() do on the ELF and the map."""
    found, mapped = pass_file(out, name, stage, '.elf'), pass_file(out, name, stage, '.map')
    return Linked(elf.read(content(found), found), placements(content(mapped).decode('latin-1'), mapped))
