"""Plan make gcc's link: write the spec of its images, which tools/ld.py
links by, from config/'s tables, the images' GCC objects and the memory
contract.

spec.py writes SPEC, the spec of every image config/ describes, in name
order. It reads config/'s tables, config/imports.txt among them, and the
sources tools/memory.py's memory contract reads. It reads each image's GCC
objects under build/gcc/, and each object an --add option gives the image.
It reads each archive an --archive option names, and libgcc.a from the
toolchain SH_ELF in the environment names, tools/sh-elf by default. For an
image that places a gen/ range, it reads the record tools/content.py writes
of the image's content, build/gen/IMAGE/content.txt.

With --list FILE, spec.py writes a line to FILE for each name nothing
binds: its image, its kind and the name, then gen/ where the image's
content alone takes it. spec.py writes FILE once every check but that of
the names nothing binds passes, and SPEC only where that one passes too.
SPEC and FILE must be two files, and neither may be an input of the link.

A view is the top byte of an address in RAM, 0x0c or 0x8c: RAM appears at
both 0x0c000000 and 0x8c000000, and the game uses both. An image's span runs
from its first section's start to its last's end, as Config.span() gives
it. Its interval runs from its origin, its tables' base, for the length of
its load area, as the memory contract gives them. An image is live with
another where a state of the memory contract holds both and lets its
images take names from one another. The contract's colive gives each image
the images live with it.

SPEC opens with a comment. Then each image takes these rows, in the forms
tools/gcclink.py gives, in this order:

- image: its origin and the length of its load area, and its entry, where
  memory.ENTRIES gives one;
- input: the GCC objects of its sources, in the order relink gives the
  sources. Its content, build/gcc/gen/IMAGE/content.o, holds the data of
  all its gen/ ranges, and stands where the first one does. Then come each
  object an --add option gives the image, in their order; each archive
  --archive names, in their order; and libgcc.a;
- import, alias and constant: a binding for each name the image takes from
  outside, as below;
- head: each record the image's loader reads at a fixed offset, as
  memory.records() gives them: the record's section in the tables, its
  source's GCC object and its GCC section;
- anchor: each of those records' symbol, its offset and, where the record
  has one, its size;
- tail: each section of the image's tail, which the link places past its
  bss, as memory.TAILS gives them: its section in the tables, its source's
  GCC object and its GCC section. No loader reads a tail at a fixed offset.
  In memory.FISH_IMAGE, STG08, a tail row follows for each allocated
  section of nonzero size of the fish models' object: in CFISH where the
  section's name starts .rodata, and in DFISH where it starts .data;
- kernel: each section of the image's kernel, as kernel() reads
  config/IMAGE/kernel.txt: its source's GCC object and the section that
  holds its function, in the table's order.

The link takes archive members as gcclink.py's selected() does. So each
image links its own copy of each member it takes, and no image's link waits
on another's for one. Each path an input row gives must be one gcclink.py
reads as a PATH.

A name an image takes from outside is one an input its link takes refers
to and none defines. A taken member's references count as an object's do.
An input's absolute or common definition binds the image's own references,
and no other image's. ld.py's script defines image_start and image_end, and
__startof_X, __endof_X and __sizeof_X, the bounds of each run of sections it
places. So none of these names takes a binding, or a line of FILE, and ld
fails on a run's bound that no run or input defines.

A name's address is the one the image's tables give it, as
Config.symbols() gives them. Where they give none, it is the one the
image's content's record gives. A word of the content that points where no
table names a definition takes the name unresolved.IMAGE.WORD.TARGET. WORD
is the word's address, and TARGET the address the word holds, which the
record gives the name.

An image provides a name at an address where its objects define the name
in an allocated section, as gcclink.exports() gives them. It doesn't where
its own symbols.txt gives the name an address other than that one, spelled
in the view of the image's base.

Each name an image takes from outside binds by the first of these rules
that fits:

- a reviewed row for the image and the name, or failing that, one for
  every image and the name. A row whose provider is another image imports
  the provider's symbol in the row's view. A row whose provider is the
  image makes the name an alias, a second name for the image's own symbol;
- an anchor the memory contract gives the image, which binds the name to
  the anchor's address as a constant;
- where the name has an address, an import of it. Its provider is the one
  image live with the image that provides the name there and whose span
  holds the address. The import takes the view the address spells, where
  the matching build binds the name.

The reviewed rows come from two tables. Each row of
config/1st_read/imports.txt, as config.imports() reads it, imports its
symbol from the image it names into 1ST_READ.BIN under its name. The import
takes the view that image's symbols.txt spells the symbol's address in.

A row of config/imports.txt is IMAGE NAME PROVIDER SYMBOL VIEW REASON.
IMAGE * makes it a row for every image that takes NAME and has no row of
its own for it. A row whose PROVIDER is its IMAGE is an alias, with VIEW -.
REASON says why the row is needed.

A hand-off is an import from an image the importer reads and then starts,
which never runs beside it. A row of config/imports.txt is a hand-off
where its REASON is hand-off: and a reason, and a row of
config/1st_read/imports.txt where it gives hand-off: and a reason.

A reviewed row must hold to these rules:

- its provider's objects define its symbol;
- where the name has an address, its provider's symbols.txt gives the
  symbol no address or that one, in either view;
- an import's view is the one the name's address spells, where the name
  has one;
- an import's provider is live with the image, but in a hand-off. A
  hand-off needs a state of the memory contract that holds the image whole
  and the provider's file, read and not yet started;
- it binds a name its image takes from outside. A row for every image does
  so where an image takes the name and has no row of its own for it.

A row of config/imports.txt can't give a name an earlier row of either
table gives its image. A row for every image can't give a name
config/1st_read/imports.txt gives 1ST_READ.BIN.

A content's record must list each name the content takes, and each global
or weak name it defines. Each name the record lists must be one the
content takes, or one it defines, statics included. A name the content
takes that the record gives a provider must bind as the record gives it:
to the provider's symbol, in the view the record's address spells, or
where the provider is the image itself, to the image's own symbol. A name
nothing binds is reported as such instead.

A name nothing binds takes the first of these kinds that fits:

- untabled, where it has no address;
- ambiguous, where several images live with the image provide it, and the
  spans of none or of several of them hold the address. It is ambiguous
  too where the image's span doesn't hold the address, and the spans of
  several images live with it do;
- storage, where neither the image's span nor that of an image live with
  it holds the address, and the image's interval does;
- nowhere, where no such span holds the address, and the image's interval
  doesn't either.

Otherwise the address has a holder: the image, where its span holds the
address, or else the one image live with it whose span does. The kind is
then the first of these that fits:

- second, where the holder's objects define another name its symbols.txt
  gives the address. The name is then a second name, which needs a row of
  config/imports.txt;
- content, where one of the holder's gen/ ranges holds the address;
- static, where one of the holder's objects defines a static of the name,
  which no other object can take by name;
- interior, where a range of one of the holder's sources holds the
  address, inside that source's object;
- storage, where none does.

spec.py fails on a reviewed row or a content's record that breaks a rule
above, and on:

- an --add for an image config/ doesn't describe;
- a SPEC and FILE that name one file, or either that names an input of the
  link;
- a toolchain that holds no libgcc.a, or several;
- an image whose objects aren't all built, naming each such image, how
  many of its objects are missing and the first three;
- a row of config/imports.txt that doesn't parse;
- a content's record that can't be read, or a row of one that record()
  refuses;
- a content's record that gives a name the tables give elsewhere, in
  either view;
- an address it takes a view from that lies in neither view;
- an input whose path isn't a PATH as gcclink.py spells one;
- an allocated section of nonzero size of the fish models' object whose
  name starts neither .rodata nor .data;
- a name nothing binds, naming each image that takes one, with how many of
  each kind it takes and the first three of each. The line ends with how
  many of them the image's content alone takes, where it takes any. A last
  line gives how many of the images take one.

It fails too as the memory contract, config/'s readers, config.py's order()
and elf.py's readers of objects and archives do.
"""

import argparse
import os
import re
import sys
from collections import Counter, namedtuple
from pathlib import Path

import elf
import gcclink
import memory
from config import HANDOFF, Config, gcc_object, handoffs, imports, order
from images import CONFIG, images
from tables import ADDRESS, hexa, named, rows
from tool import content, fail, save

IMPORTS = Path('config/imports.txt')
# the record tools/content.py writes beside an image's content's C
RECORD = 'build/gen/{image}/content.txt'
# the IMAGE of a row of config/imports.txt for every image, and the VIEW of
# an alias
EVERY = '*'
OWN = '-'
# the kinds of name nothing binds, in the order a report lists them
KINDS = ('content', 'second', 'static', 'interior', 'storage', 'ambiguous', 'untabled', 'nowhere')
# how many names a problem gives of a list it counts
MOST_SHOWN = 3

# what spec.py reads of an object. defined is the global or weak names it
# defines in its allocated sections, as gcclink.exports() gives them, and
# statics the local ones. Both give C names. taken is the names it leaves
# undefined, as the object spells them, so a C name keeps its leading
# underscore
Unit = namedtuple('Unit', 'defined statics taken')
# a reviewed row: its provider, its symbol and its view, OWN for an alias;
# where, its path:line; and whether it is a hand-off
Row = namedtuple('Row', 'provider symbol view where handoff')
# what bind() reads once for every image's bindings. Each field but
# contract, the memory contract, is by image. configs gives each image's
# Config. defined gives the names each image's objects define, and units its
# objects, as Units. spans gives each image's span, both ends in the 0x0c
# view. tables gives the address each image's own symbols.txt gives each
# name
Facts = namedtuple('Facts', 'configs defined units spans tables contract')
# a row of a content's record: its name and address; a definition's size; a
# taken name's provider and symbol; and where, its path:line. A field the row
# doesn't give is None
Recorded = namedtuple('Recorded', 'name address size provider symbol where')
# the name tools/content.py gives a word that points where no table names a
# definition: its image, the word's address and the address it holds
UNRESOLVED = re.compile(r'unresolved\.[a-z0-9_]+\.[0-9a-f]{8}\.[0-9a-f]{8}')


def unit(found):
    """The Unit of found, an object as elf.read() reads it."""
    allocated = elf.allocated_sections(found)
    statics = {symbol.name[1:] for symbol in found.symbols if symbol.bind == elf.LOCAL
               and symbol.section in allocated and symbol.name.startswith('_') and elf.names_definition(symbol)}
    taken = {symbol.name for symbol in found.symbols if symbol.section == elf.UNDEFINED and symbol.name}
    return Unit(set(gcclink.exports(found)), statics, taken)


def reviewed(known):
    """The reviewed rows, each a Row keyed by (IMAGE, NAME). A row for every
    image has IMAGE EVERY. known is the images config/ describes.
    config/1st_read/imports.txt's rows come first, then config/imports.txt's,
    in table order.

    Fails as config.imports() does, and on a row of
    config/1st_read/imports.txt whose address lies in neither view. Fails on
    a row of config/imports.txt, naming its path:line, where:
    - it has fewer than six fields, or its REASON is hand-off: alone;
    - its IMAGE is neither EVERY nor of known, or its PROVIDER isn't of
      known;
    - its NAME or SYMBOL isn't a C name;
    - its VIEW isn't OWN for an alias, or 0x0c or 0x8c for an import;
    - an earlier row gives its IMAGE its NAME, or its IMAGE is EVERY and
      config/1st_read/imports.txt gives 1ST_READ.BIN its NAME.
    """
    handed = handoffs()
    found = {('1st_read', name): Row(provider, symbol, view(address, where), where, name in handed)
             for name, provider, symbol, address, where in imports()}
    first = set(found)
    form = (f'not a row: IMAGE NAME PROVIDER SYMBOL VIEW REASON, IMAGE {EVERY} or an image config/ describes, '
            f'PROVIDER an image config/ describes, and VIEW 0x0c or 0x8c, or {OWN} where PROVIDER is IMAGE')
    for where, fields in rows(IMPORTS):
        if len(fields) < 6 or fields[5:] == [HANDOFF]:
            fail(f'{where}: {form}')
        image, name, provider, symbol, spelled, *reason = fields
        alias = provider == image
        if (image not in known and image != EVERY) or provider not in known \
                or not gcclink.C_NAME.fullmatch(name) or not gcclink.C_NAME.fullmatch(symbol) \
                or (spelled != OWN if alias else spelled not in gcclink.VIEWS):
            fail(f'{where}: {form}')
        if (image, name) in found or (image == EVERY and ('1st_read', name) in first):
            held = (image, name) if (image, name) in found else ('1st_read', name)
            fail(f'{where}: {held[0]} binds {name} already, at {found[held].where}')
        found[(image, name)] = Row(provider, symbol, spelled, where, reason[0] == HANDOFF)
    return found


def within(span, address):
    """Whether address lies in span, in either view. span is a (start, end)
    in the 0x0c view."""
    low, high = span
    return low <= memory.physical(address) < high


def view(address, who):
    """The view address spells, as a spec spells one: its top byte, 0x0c or
    0x8c. An address in neither view fails, naming who."""
    spelled = f'{address >> 24:#04x}'
    if spelled not in gcclink.VIEWS:
        fail(f"{who}: {hexa(address)} lies in neither of the spec's views, 0x0c and 0x8c")
    return spelled


def spelled_at(config, address):
    """address in the view of config's image's base, the view the image's
    tables spell its own addresses in."""
    return (address & ~memory.VIEW_BIT) | (config.base & memory.VIEW_BIT)


def shown(names):
    """The first MOST_SHOWN of names, joined by commas, with ... after them
    where more follow."""
    return ', '.join(names[:MOST_SHOWN]) + (', ...' if len(names) > MOST_SHOWN else '')


def inputs(config):
    """The GCC objects of config's image's sources, as gcc_object() names
    them, in the order relink gives the sources. The gen/ ranges share the
    image's content's object, which stands where the first one does. Fails
    as config.py's order() does."""
    return list(dict.fromkeys(gcc_object(source, config.name) for source in order(config)))


def record(config):
    """The rows of config's image's content's record, RECORD, each a
    Recorded, and none where the image places no gen/ range.

    A row is NAME ADDRESS SIZE TYPE for a definition, SIZE in decimal bytes.
    It is NAME ADDRESS - taken: PROVIDER SYMBOL TYPE for a name the content
    takes from outside, and SPELLING ADDRESS - unresolved: WHY for a word
    that points where no table names a definition. NAME and SYMBOL are C
    names, SPELLING is UNRESOLVED's, ADDRESS is 0x and eight lowercase hex
    digits, and PROVIDER is an image config/ describes.

    A record that can't be read fails, naming it. A row fails, naming its
    path:line, where it takes none of these forms, and where it gives a name
    an earlier row gives.
    """
    if not any(source.startswith('gen/') for source in config.sources()):
        return []
    found, seen, known = [], {}, set(images())
    for where, fields in rows(Path(RECORD.format(image=config.name))):
        defined = len(fields) >= 4 and fields[2].isdecimal()
        taken = len(fields) >= 7 and fields[2:4] == ['-', 'taken:']
        unresolved = len(fields) >= 5 and fields[2:4] == ['-', 'unresolved:']
        if not (defined or taken or unresolved) or not ADDRESS.fullmatch(fields[1]) \
                or not (UNRESOLVED if unresolved else gcclink.C_NAME).fullmatch(fields[0]) \
                or (taken and not (fields[4] in known and gcclink.C_NAME.fullmatch(fields[5]))):
            fail(f"{where}: not a row of a content's record")
        if fields[0] in seen:
            fail(f'{where}: {fields[0]} has a row already, at {seen[fields[0]]}')
        seen[fields[0]] = where
        provider, symbol = fields[4:6] if taken else (None, None)
        found.append(Recorded(fields[0], int(fields[1], 16), int(fields[2]) if defined else None, provider, symbol,
                              where))
    return found


def addresses(config, recorded):
    """Each name's address, by name, as config's image's tables give it. A
    name they don't give takes the address recorded, its content's record,
    gives. A row of the record that gives a name the tables give elsewhere,
    in either view, fails, naming both."""
    found = {name: address for address, name in config.symbols()}
    for row in recorded:
        if row.name not in found:
            found[row.name] = row.address
        elif memory.physical(row.address) != memory.physical(found[row.name]):
            fail(f"{row.where}: gives {row.name} at {hexa(row.address)}, and config/{config.name}/'s tables give it "
                 f'at {hexa(found[row.name])}')
    return found


def bind(known, contract, archives, added):
    """Each image's link and the names it takes that nothing binds, as
    (links, unbound) by image. known is the images config/ describes, and
    contract their memory contract. archives is the archives each image
    links ahead of libgcc.a. added is the objects each image links after its
    sources', by image.

    An image's link is its inputs, in link order, and its bindings:
    {'import': {name: (provider, symbol, view)}, 'alias': {name: symbol},
    'constant': {name: address}}. unbound gives each name nothing binds its
    kind, and whether only the image's content takes it.

    Fails as gcclink.libgcc() does, and on an image whose objects aren't all
    built. Fails too on a reviewed row or a content's record that breaks a
    rule of the module docstring's.
    """
    searched = [*archives, gcclink.libgcc()]
    configs = {image: Config(image) for image in known}
    objects, missing = {}, []
    for image in known:
        objects[image] = inputs(configs[image]) + added.get(image, [])
        lacking = [str(path) for path in objects[image] if not path.is_file()]
        if lacking:
            missing.append(f'{image}: waits for {len(lacking)} of its {len(objects[image])} objects, which make gcc '
                           f"didn't build: {shown(lacking)}")
    if missing:
        fail(*missing)
    paths = sorted({path for image in known for path in objects[image]})
    read = {path: unit(elf.read(content(path), path)) for path in paths}
    units = {image: [read[path] for path in objects[image]] for image in known}
    defined = {image: set().union(*(each.defined for each in units[image])) for image in known}
    spans = {image: tuple(map(memory.physical, config.span())) for image, config in configs.items()}
    tables = {image: {name: address for address, name in named(config.path / 'symbols.txt')}
              for image, config in configs.items()}
    facts = Facts(configs, defined, units, spans, tables, contract)
    reviewed_rows = reviewed(known)
    links, unbound, used, problems = {}, {}, set(), []
    for image in known:
        recorded = record(configs[image])
        tabled = addresses(configs[image], recorded)
        # each gen/ range's object is the image's content's
        generated = gcc_object('gen/', image)
        # selected() needs no roots. ld.py's script roots the symbols other
        # images import from this one and those its aliases name. This
        # image's objects define each of them, so no root takes an archive
        # member
        _, undefined = gcclink.selected(objects[image] + searched)
        bindings = {'import': {}, 'alias': {}, 'constant': {}}
        unbound[image] = {}
        for taken in sorted(undefined):
            wanted = elf.as_c_name(taken)
            if gcclink.boundary(wanted):
                continue
            key = next((key for key in ((image, wanted), (EVERY, wanted)) if key in reviewed_rows), None)
            if key:
                used.add(key)
            kind, value = binding(facts, image, wanted, tabled.get(wanted), reviewed_rows.get(key))
            if kind:
                bindings[kind][wanted] = value
            else:
                unbound[image][wanted] = (value, undefined[taken] == [str(generated)])
        if generated in read:
            problems += recorded_problems(image, recorded, read[generated], bindings, undefined, unbound[image])
        links[image] = (objects[image] + searched, bindings)
    stale = [f'{row.where}: ' + (f'no image without a row of its own takes {wanted}' if image == EVERY
                                 else f'{image} takes no {wanted}') + ' from outside, so the row binds nothing'
             for (image, wanted), row in reviewed_rows.items() if (image, wanted) not in used]
    if stale or problems:
        fail(*stale, *problems)
    return links, unbound


def contract_holds(contract, image, row):
    """Whether the memory contract holds row, a reviewed import into image.
    A hand-off needs a state that holds the image whole and the provider's
    file, read and not yet started. Any other import needs its provider live
    with the image."""
    if row.handoff:
        return any(state.images.get(image) == 'whole' and state.images.get(row.provider) == 'file'
                   for state in contract.states)
    return row.provider in contract.colive[image]


def binding(facts, image, wanted, address, row):
    """The binding of wanted, a name image takes from outside, by the first
    of the module docstring's rules that fits: its kind, 'import', 'alias'
    or 'constant', and its value. An import's value is (provider, symbol,
    view). An alias's is its symbol, and a constant's its address. A name
    nothing binds gives None and its kind.

    address is the name's, from image's tables or its content's record, or
    None. row is the reviewed row that gives the name, or None. A row that
    breaks a rule of the module docstring's fails, naming its path:line."""
    contract = facts.contract
    if row:
        given = facts.tables[row.provider].get(row.symbol)
        if address is not None and given is not None and memory.physical(address) != memory.physical(given):
            fail(f'{row.where}: config/{image}/ gives {wanted} at {hexa(address)}, and '
                 f'config/{row.provider}/symbols.txt gives {row.symbol} at {hexa(given)}')
        if row.symbol not in facts.defined[row.provider]:
            fail(f"{row.where}: {row.provider}'s objects don't define {row.symbol}")
        if row.provider == image:
            return 'alias', row.symbol
        if address is not None and view(address, row.where) != row.view:
            fail(f'{row.where}: config/{image}/ gives {wanted} at {hexa(address)}, in the '
                 f'{view(address, row.where)} view, and the row takes the {row.view} view')
        if not contract_holds(contract, image, row):
            fail(f"{row.where}: a hand-off, and no state of the memory contract holds {row.provider}'s file, read "
                 f'and not yet started, with {image}' if row.handoff else
                 f"{row.where}: {image} takes {wanted} from {row.provider}, which the memory contract's states never "
                 f'hold live with it, and the row gives no reason that opens with {HANDOFF}')
        return 'import', (row.provider, row.symbol, row.view)
    if wanted in contract.anchored.get(image, {}):
        return 'constant', contract.anchored[image][wanted]
    if address is None:
        return None, 'untabled'
    providers = []
    for other in contract.colive[image]:
        own = spelled_at(facts.configs[other], address)
        if wanted in facts.defined[other] and facts.tables[other].get(wanted, own) == own:
            providers.append(other)
    holders = [other for other in providers if within(facts.spans[other], address)]
    if len(holders) == 1:
        return 'import', (holders[0], wanted, view(address, f'{image}: {wanted}'))
    if len(providers) > 1:
        return None, 'ambiguous'
    return None, unbound_kind(facts, image, wanted, address)


def unbound_kind(facts, image, wanted, address):
    """The kind of wanted, a name image takes at address, where nothing
    binds it and no two images live with image provide it: the first of the
    module docstring's kinds that fits."""
    spans, contract = facts.spans, facts.contract
    if within(spans[image], address):
        holders = [image]
    else:
        holders = [other for other in contract.colive[image] if within(spans[other], address)]
    if not holders:
        origin = memory.physical(contract.origins[image])
        return 'storage' if origin <= memory.physical(address) < origin + contract.lengths[image] else 'nowhere'
    if len(holders) > 1:
        return 'ambiguous'
    holder = facts.configs[holders[0]]
    at = spelled_at(holder, address)
    if any(name != wanted and given == at and name in facts.defined[holder.name]
           for name, given in facts.tables[holder.name].items()):
        return 'second'
    if any(low <= at < high for low, high in holder.generated()):
        return 'content'
    if any(wanted in each.statics for each in facts.units[holder.name]):
        return 'static'
    if any(low <= at < high for _, _, low, high in holder.placements):
        return 'interior'
    return 'storage'


def recorded_problems(image, recorded, made, bindings, undefined, unbound):
    """The problems of recorded, image's content's record, against made and
    image's link. made is the content's object, as a Unit. bindings are the
    link's, and undefined the names its inputs refer to and none defines, as
    gcclink.selected() gives them. unbound is the names nothing binds."""
    listed = {row.name for row in recorded if row.size is None}
    problems = [f"{image}: its content takes {name}, which its record doesn't list"
                for name in sorted(map(elf.as_c_name, made.taken))
                if name not in listed]
    problems += [f"{image}: its content defines {name}, which its record doesn't list"
                 for name in sorted(made.defined - {row.name for row in recorded if row.size is not None})]
    owned = made.defined | made.statics
    for row in recorded:
        if row.size is not None:
            if row.name not in owned:
                problems.append(f"{image}: its content's record lists {row.name}, which the content doesn't define")
            continue
        # an unresolved word's name has no C name's underscore
        if (row.name if row.provider is None else f'_{row.name}') not in made.taken:
            problems.append(f"{image}: its content's record lists {row.name}, which the content doesn't take")
            continue
        if row.provider is None or row.name in unbound:
            continue
        got = bound_to(image, row.name, bindings, undefined)
        meant = (row.provider, row.symbol, None if row.provider == image else
                 view(row.address, f'{RECORD.format(image=image)}: {row.name}'))
        if got != meant:
            problems.append(f"{image}: its content takes {row.name} from {row.provider}'s {row.symbol} at "
                            f"{hexa(row.address)}, as its record gives it, and the link binds it "
                            + ('otherwise' if got is None else f"to {image}'s own {got[1]}" if got[2] is None
                               else f"to {got[0]}'s {got[1]} in the {got[2]} view"))
    return problems


def bound_to(image, wanted, bindings, undefined):
    """What image's link binds a reference to wanted to, from its bindings
    and undefined, the names its inputs refer to and none defines. An import
    gives its provider, symbol and view. An alias gives the image, its
    symbol and None. A name an input of the link defines gives the image,
    wanted and None. Any other binding, as a constant's, gives None."""
    if wanted in bindings['import']:
        return bindings['import'][wanted]
    if wanted in bindings['alias']:
        return (image, bindings['alias'][wanted], None)
    if f'_{wanted}' not in undefined:
        return (image, wanted, None)
    return None


def kernel(image):
    """The sections of image's kernel, config/IMAGE/kernel.txt, as a list of
    (object, section). A row is SOURCE FUNCTION. SOURCE is one of image's
    placements, as its tables spell it, and FUNCTION a C name. Each row
    gives the GCC object of SOURCE and the allocated section of it that
    defines FUNCTION, in the table's order. An image without the table has
    no kernel.

    A row fails, naming its path:line, where it doesn't parse, where its
    SOURCE is none of image's placements, where its object defines no
    FUNCTION in an allocated section, and where an earlier row's section
    holds FUNCTION too. It fails as content() and elf.read() do on the
    object."""
    path, found = CONFIG / image / 'kernel.txt', []
    if not path.exists():
        return found
    placed = Config(image).sources()
    for where, fields in rows(path):
        if len(fields) != 2 or not gcclink.C_NAME.fullmatch(fields[1]):
            fail(f'{where}: not a row: SOURCE FUNCTION')
        source, function = fields
        if source not in placed:
            fail(f'{where}: config/{image}/ places no {source}')
        obj = gcc_object(source, image)
        read = elf.read(content(obj), obj)
        allocated = elf.allocated_sections(read)
        sections = {allocated[symbol.section].name for symbol in read.symbols
                    if symbol.name == f'_{function}' and elf.names_definition(symbol) and symbol.section in allocated}
        if len(sections) != 1:
            fail(f'{where}: {obj} defines no {function} in an allocated section')
        section = sections.pop()
        if (obj, section) in found:
            fail(f"{where}: {function} lies in {obj}'s {section}, which an earlier row places")
        found.append((obj, section))
    return found


def lines(known, links, contract):
    """The spec's lines for known's images, in known's order. links is
    bind()'s, and contract the memory contract. An image's rows come in the
    module docstring's order, and its bindings of each kind in name order.
    An allocated section of nonzero size of the fish models' object whose
    name starts neither .rodata nor .data fails, naming the object."""
    out = []
    for image in known:
        paths, bindings = links[image]
        entry = memory.ENTRIES.get(image)
        out.append(f'image     {image}  {hexa(contract.origins[image])}  {hexa(contract.lengths[image])}'
                   + (f'  {entry}' if entry else ''))
        out += [f'input     {image}  {path}' for path in paths]
        for wanted, (provider, symbol, spelled) in sorted(bindings['import'].items()):
            out.append(f'import    {image}  {wanted}  {provider}  {symbol}  {spelled}')
        out += [f'alias     {image}  {wanted}  {symbol}' for wanted, symbol in sorted(bindings['alias'].items())]
        out += [f'constant  {image}  {wanted}  {hexa(value)}' for wanted, value in sorted(bindings['constant'].items())]
        records = memory.records(image, contract.events)
        for section, source, gcc, symbol, offset, size in records:
            out.append(f'head      {image}  {section}  {gcc_object(source, image)}  {gcc}')
        for section, source, gcc, symbol, offset, size in records:
            out.append(f'anchor    {image}  {symbol}  {offset:#x}' + (f'  {size}' if size is not None else ''))
        for section, source, gcc in memory.TAILS.get(image, ()):
            out.append(f'tail      {image}  {section}  {gcc_object(source, image)}  {gcc}')
        if image == memory.FISH_IMAGE:
            fish = gcc_object(memory.FISH_MODELS, image)
            for section in elf.read(content(fish), fish).sections:
                if section.flags & elf.ALLOC and section.size:
                    if not section.name.startswith(('.rodata', '.data')):
                        fail(f'{fish}: holds {section.name}, which the fish models of the tail leave out')
                    out.append(f'tail      {image}  {"CFISH" if section.name.startswith(".rodata") else "DFISH"}  '
                               f'{fish}  {section.name}')
        out += [f'kernel    {image}  {obj}  {section}' for obj, section in kernel(image)]
    return out


def report(unbound):
    """A line for each image of unbound that takes names nothing binds: how
    many, and how many of each kind, in KINDS' order, with the first
    MOST_SHOWN of each. Where the image's content alone takes any, the line
    ends with how many."""
    found = []
    for image, kinds in unbound.items():
        if not kinds:
            continue
        counts = Counter(kind for kind, _ in kinds.values())
        parts = [f'{kind} {counts[kind]} ({shown([wanted for wanted, (each, _) in kinds.items() if each == kind])})'
                 for kind in KINDS if counts[kind]]
        alone = sum(only for _, only in kinds.values())
        found.append(f'{image}: {len(kinds)} names nothing binds: ' + ', '.join(parts)
                     + (f'; its content alone takes {alone}' if alone else ''))
    return found


def same_file(one, other):
    """Whether paths one and other name one file: they share a real path,
    or both exist on one inode, as two hard links do."""
    if os.path.realpath(one) == os.path.realpath(other):
        return True
    try:
        return os.path.samefile(one, other)
    except OSError:
        return False


def main(argv):
    parser = argparse.ArgumentParser(prog='spec', description=__doc__.split('\n\n')[0])
    parser.add_argument('spec', metavar='SPEC', type=Path, help="where to write tools/ld.py's spec")
    parser.add_argument('--archive', metavar='PATH', type=Path, action='append', default=[],
                        help='an archive each image searches ahead of libgcc.a; given again, another, in order')
    parser.add_argument('--add', metavar=('IMAGE', 'PATH'), nargs=2, action='append', default=[],
                        help="an object that replaces no source's, which IMAGE links after its sources' objects; "
                             'given again, another, in order')
    parser.add_argument('--list', metavar='FILE', type=Path,
                        help="write each name nothing binds there, an image, a kind and a name to a line, with gen/ "
                             "after one only its image's content takes")
    args = parser.parse_args(argv)
    known = images()
    contract = memory.contract(known)
    added = {}
    for image, path in args.add:
        if image not in known:
            fail(f'--add {image} {path}: no image config/ describes {image}')
        added.setdefault(image, []).append(Path(path))
    if args.list and same_file(args.spec, args.list):
        fail('SPEC and --list name the same file')
    links, unbound = bind(known, contract, args.archive, added)
    read = [path for paths, _ in links.values() for path in paths]
    for output in [args.spec, *([args.list] if args.list else [])]:
        clash = next((path for path in read if same_file(output, path)), None)
        if clash is not None:
            fail(f'{output}: names {clash}, an input of the link')
    for path in dict.fromkeys(map(str, read)):
        if not gcclink.PATH.fullmatch(path):
            fail(f"{path}: not a path the spec can give: letters, digits, _, ., / and -, not starting with -")
    text = ''.join(f'{line}\n' for line in ["# make gcc's images, as tools/spec.py writes them",
                                            *lines(known, links, contract)])
    if args.list:
        save(args.list, ''.join(f'{image}  {kind}  {wanted}' + ('  gen/' if only else '') + '\n'
                                for image, kinds in unbound.items() for wanted, (kind, only) in kinds.items()).encode())
    problems = report(unbound)
    if problems:
        fail(*problems, f'{len(problems)} of the {len(known)} images take names nothing binds')
    save(args.spec, text.encode())


if __name__ == '__main__':
    main(sys.argv[1:])
