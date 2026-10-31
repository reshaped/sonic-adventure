"""Check make gcc's finished link: what each import, alias and constant
binds, where each anchor lies, which inputs each image takes, and whether
the declarations of each name agree.

    python3 tools/verify.py --abi FLAGS SPEC DIR

SPEC is the spec tools/spec.py writes, in the rows tools/gcclink.py gives,
and DIR the directory tools/ld.py linked its images under. verify reads
each image's final pass, its ELF DIR/IMAGE/final.elf and its map
DIR/IMAGE/final.map, and each input the spec gives the image. It writes
and removes nothing. make runs tools/publish.py, which writes the manifest,
only once verify passes.

Three checks of the link run first, and verify fails on every problem they
find. Each problem names its image, and the name, relocation or input at
fault.

- The binding check holds each import's value in final.elf to its
  provider's export of SYMBOL, in the provider's final.elf and in the
  import's view. It holds each alias's value to its SYMBOL's in the image's
  own final.elf, and each constant's to its VALUE. Each relocation against
  an import must be an R_SH_DIR32 whose word holds the import's value plus
  the word its input held. SH's ELF keeps an R_SH_DIR32's addend in the
  word it relocates: ld adds the symbol's value to the word, and leaves the
  relocation's own addend, which GNU as writes as 0, aside. So the check
  finds the input section the map places where the relocation lies, and
  reads the input's word at the relocation's offset in it. It refuses a
  relocation whose own addend isn't 0, in its input or in final.elf, and
  one it can't trace to one relocation of that input section. Each
  relocation against an import, in an input section the map places, must
  keep its record in final.elf.
- The anchor check holds each anchor's SYMBOL to name one symbol of
  final.elf, local or not, in an allocated section. That symbol must lie
  OFFSET bytes past ORIGIN and, where the row gives SIZE, hold SIZE bytes.
  So a run's start and end can be anchored, and its size, an absolute
  value, can't.
- The selection check holds each image's final map to place the inputs
  gcclink.py's selected() takes, objects and archive members alike, and no
  other. tools/spec.py binds what selected() leaves undefined, so the
  check holds spec.py's plan to the link GNU ld makes.

Then the declarations check runs, with FLAGS, and verify fails on each
line tools/decls.py's failures() gives.

verify fails too as gcclink.py's read() does on SPEC and its linked() on
each final pass, and as elf.py's opened() does on each input.
"""

import argparse
import sys
from pathlib import Path

import decls
import elf
import gcclink
from tables import hexa
from tool import fail


def misbound(images, finals):
    """The binding check's problems in finals, each image's final pass.

    - An import is a problem where its provider's final pass doesn't export
      its SYMBOL. So is one whose value isn't that export, in its view.
    - An alias whose value isn't its SYMBOL's in the image's own final pass
      is a problem. So is a constant whose value isn't its VALUE.
    - A value the script assigns that isn't absolute is a problem.
    - A relocation against an import that misrelocated() finds wrong is a
      problem. So is one in an input section the map places whose record
      the final pass doesn't keep there.

    It fails as gcclink.in_view() does, and as gcclink.inputs() does on
    each image's inputs."""
    problems, exported = [], {name: gcclink.exports(linked.elf) for name, linked in finals.items()}
    for image in images.values():
        linked = finals[image.name]
        symbols = {symbol.name: symbol for symbol in linked.elf.symbols}
        for alias, wanted in image.imports.items():
            provider = finals[wanted.provider].elf
            defined = exported[wanted.provider].get(wanted.symbol)
            taken = symbols.get(f'_{alias}')
            if defined is None:
                problems.append(f"{image.name}: {alias} takes {wanted.provider}'s {wanted.symbol}, which "
                                f"{provider.where} doesn't define")
                continue
            value = gcclink.in_view(defined.value, wanted.view, f'{image.name}: {alias}')
            if taken is None or taken.section != elf.ABSOLUTE or taken.value != value:
                problems.append(f"{image.name}: {alias} is {spelled(taken)} in {linked.elf.where}, and "
                                f"{wanted.provider}'s {wanted.symbol} in the {wanted.view} view is "
                                f'{hexa(value)} in {provider.where}')
        own = exported[image.name]
        expected = {**{alias: (own[symbol].value if symbol in own else None, f'its {symbol}')
                       for alias, symbol in image.aliases.items()},
                    **{symbol: (value, 'its constant') for symbol, value in image.constants.items()}}
        for name, (value, what) in expected.items():
            taken = symbols.get(f'_{name}')
            if value is None or taken is None or taken.section != elf.ABSOLUTE or taken.value != value:
                problems.append(f'{image.name}: {name} is {spelled(taken)} in {linked.elf.where}, and {what} is '
                                + (hexa(value) if value is not None else 'absent'))
        imported = {f'_{alias}' for alias in image.imports}
        objects = {gcclink.map_name(path, member): obj for path, member, obj in gcclink.inputs(image)}
        # the output section and address of each relocation of the final pass
        # against an import
        kept = set()
        for target, relocations in sorted(linked.elf.relocations.items()):
            section = linked.elf.sections[target]
            for relocation in relocations:
                symbol = linked.elf.symbols[relocation.symbol]
                if symbol.name in imported:
                    kept.add((section.name, relocation.offset))
                    problem = misrelocated(linked, section, relocation, symbol, objects)
                    if problem:
                        problems.append(f'{image.name}: {problem}')
        # the offset and name of each relocation against an import in an
        # input section, by the section's file, name and size
        sites = {}
        for file, obj in objects.items():
            for index, relocations in obj.relocations.items():
                section = obj.sections[index]
                sites.setdefault((file, section.name, section.size), []).extend(
                    (relocation.offset, obj.symbols[relocation.symbol].name) for relocation in relocations
                    if obj.symbols[relocation.symbol].name in imported)
        for placed in linked.placements:
            for offset, name in sites.get((placed.file, placed.section, placed.size), ()):
                if (placed.output, placed.address + offset) not in kept:
                    problems.append(f'{image.name}: the relocation at {hexa(placed.address + offset)}, against '
                                    f"{name[1:]}, from {placed.file}'s {placed.section}, is missing from "
                                    f'{linked.elf.where}')
    return problems


def spelled(symbol):
    """symbol, a Symbol of a final pass or None, as a failure spells it:
    'absent' for None, its value for an absolute Symbol, and its value with
    ', defined in the image' for any other."""
    if symbol is None:
        return 'absent'
    return hexa(symbol.value) + ('' if symbol.section == elf.ABSOLUTE else ', defined in the image')


def misrelocated(linked, section, relocation, symbol, objects):
    """What is wrong with relocation, or None where nothing is. relocation
    lies in section of linked, an image's final pass, against symbol, one of
    the image's imports. objects maps the gcclink.map_name() of each of the
    image's inputs to its Elf.

    The relocation is right where it is an R_SH_DIR32 whose word holds
    symbol's value plus the input's word: the word at the relocation's
    offset in the input section the map places where it lies. It is wrong
    where its own addend isn't 0, in its input or in linked, since ld leaves
    that addend aside. It is wrong where it lies in no input section the map
    places in section. It is wrong where it can't be traced to one
    relocation of that input section, of its type, against symbol, at its
    offset."""
    where = f'the relocation at {hexa(relocation.offset)}, against {symbol.name[1:]},'
    if relocation.kind != elf.DIR32:
        return (f"{where} is of type {relocation.kind}, which the binding check doesn't read: it reads R_SH_DIR32, "
                f'type {elf.DIR32}, alone')
    placed = next((placed for placed in linked.placements if placed.address <= relocation.offset
                   < placed.address + placed.size and placed.output == section.name), None)
    if placed is None:
        return f"{where} lies in no input section that {linked.elf.where}'s map places"
    offset = relocation.offset - placed.address
    found = objects.get(placed.file)
    candidates = [each for each in found.sections if each.name == placed.section
                  and each.size == placed.size] if found else []
    originals = [original for candidate in candidates for original in found.relocations.get(candidate.index, [])
                 if original.offset == offset and found.symbols[original.symbol].name == symbol.name
                 and original.kind == relocation.kind]
    if len(candidates) != 1 or len(originals) != 1:
        return f"{where} comes from no one relocation of {placed.file}'s {placed.section}"
    addend = originals[0].addend or relocation.addend
    if addend:
        return (f"{where} from {placed.file}'s {placed.section}, has an addend of its own, {addend}, which the "
                "binding check doesn't read: it reads the addend in the word, which ld adds alone")
    added = int.from_bytes(candidates[0].data[offset:offset + 4], 'little')
    at = relocation.offset - section.address
    word, value = int.from_bytes(section.data[at:at + 4], 'little'), (symbol.value + added) & 0xFFFFFFFF
    if word != value:
        return (f"{where} from {placed.file}'s {placed.section}, holds {hexa(word)}, and {symbol.name[1:]} plus "
                f'{added} is {hexa(value)}')
    return None


def unanchored(images, finals):
    """The anchor check's problems: one for each anchor whose SYMBOL doesn't
    lie OFFSET bytes past its image's origin in its final pass, of finals.
    Where the row gives SIZE, one that doesn't hold SIZE bytes is a problem.
    SYMBOL must name one symbol of the final pass that names a definition,
    local or not, in an allocated section. An anchor whose SYMBOL names none
    or several is a problem too."""
    problems = []
    for image in images.values():
        found = finals[image.name].elf
        allocated = elf.allocated_sections(found)
        for anchor in image.anchors:
            named = [symbol for symbol in found.symbols if symbol.name == f'_{anchor.symbol}'
                     and elf.names_definition(symbol) and symbol.section in allocated]
            where = f'{hexa(image.origin + anchor.offset)}, {anchor.offset:#x} bytes past its origin'
            if len(named) != 1:
                problems.append(f'{image.name}: {anchor.symbol} anchors it at {where}, and {found.where} names '
                                + ('no such symbol' if not named else f'{len(named)} by that name'))
            elif named[0].value != image.origin + anchor.offset:
                problems.append(f'{image.name}: {anchor.symbol} lies at {hexa(named[0].value)} in {found.where}, '
                                f'and its anchor puts it at {where}')
            elif anchor.size is not None and named[0].size != anchor.size:
                problems.append(f'{image.name}: {anchor.symbol} holds {named[0].size} bytes in {found.where}, and '
                                f'its anchor gives {anchor.size}')
    return problems


def unselected(images, finals):
    """The selection check's problems, each naming an input as
    gcclink.map_name() does. An input that an image's final pass places and
    gcclink.selected() doesn't take is one. So is one that selected() takes
    and the pass doesn't place. finals gives each image's final pass.
    selected() takes the image's inputs with its roots, as gcclink.rooted()
    gives them. It fails as selected() does."""
    problems, roots = [], gcclink.rooted(images)
    for name, image in images.items():
        taken = {gcclink.map_name(path, member) for path, member in gcclink.selected(image.inputs, roots[name])[0]}
        placed = {each.file for each in finals[name].placements}
        problems += [f"{name}: its final pass takes {each}, which selected() doesn't"
                     for each in sorted(placed - taken)]
        problems += [f"{name}: selected() takes {each}, which its final pass doesn't"
                     for each in sorted(taken - placed)]
    return problems


def main(argv):
    parser = argparse.ArgumentParser(prog='verify', description=__doc__.split('\n\n')[0])
    parser.add_argument('--abi', metavar='FLAGS', required=True,
                        help="GCC_ABI, the -m and -fpack-struct= options make gcc compiles its C with")
    parser.add_argument('spec', metavar='SPEC', help='the spec tools/spec.py writes')
    parser.add_argument('out', metavar='DIR', type=Path, help='the directory tools/ld.py linked its images under')
    args = parser.parse_args(argv)
    images = gcclink.read(args.spec)
    finals = {name: gcclink.linked(args.out, name, 'final') for name in images}
    problems = misbound(images, finals) + unanchored(images, finals) + unselected(images, finals)
    if problems:
        fail(*problems)
    problems = decls.failures(images, args.out, args.abi.split())
    if problems:
        fail(*problems)


if __name__ == '__main__':
    main(sys.argv[1:])
