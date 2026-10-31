"""Link a set of images with GNU ld, each to an ELF of its own, and bind
each name one image takes from another to its definition there.

ld.py reads SPEC, the spec tools/spec.py writes, in the rows
tools/gcclink.py gives, and writes each image's passes under DIR/IMAGE/.
SH_ELF in the environment names the toolchain's directory, tools/sh-elf by
default, and ld.py runs its bin/sh-elf-ld.

The script lays GCC's sections and SHC's out in four output sections. An
SHC section's kind is the first letter of its name.

- .text takes .text, .text.* and SHC's P sections, its code.
- .rodata takes .rodata, .rodata.* and SHC's C sections.
- .data takes .data, .data.*, SHC's D sections and its R sections. An R
  section is a RAM copy of data that SHC's startup makes.
- .bss takes .bss, .bss.*, COMMON and SHC's B sections.

The script lays each output section out in runs. A run is a stretch of an
output section that can take bounds: __startof_X at its start, __endof_X
at its end and __sizeof_X, its size, where X is the run's name.

An output section's first run is named for its first kind: P, C, D or B.
That run takes GCC's sections, the SHC sections of its name, and each other
SHC section of the output's kinds that no input bounds. An input bounds a
section X where it takes __startof_X, __endof_X or __sizeof_X from outside.
Each other name an input bounds, where an input holds an SHC section of the
name, takes a run of its own, after the first, in name order. That run
holds those sections. A first run has bounds whether or not an input holds
a section of its name. The link fails on a bound no run or input defines.

ld lays out the sections of each run of .text, .rodata, .data and .bss
file by file, in link order. So an object's code stays together, but for
what kernel rows take, and a branch from its inline assembly reaches its
C. .bss's first run then takes COMMON, after every file's sections.
Sections GCC marks mergeable, as .rodata.str1.4 and .rodata.cst16, are the
exception. ld gathers each such name's sections where the first lies, and
keeps one copy of a string or constant that repeats.

A head row puts INPUT, an input section of PATH, in .head, which opens the
image. A tail row puts it in .tail, which closes the image. PATH is one of
the image's objects, spelled as its input row spells it. .head and .tail
take their rows' sections in row order. So a record a loader reads at a
fixed offset, or one that ends the image, lies where the rows put it. The
rows of one SECTION make one run, and must stand together, all in the head
or all in the tail.

A kernel row puts INPUT, an input section of PATH, at the start of .text's
first run, ahead of the image's other code. The kernel's sections lie there
in row order. Each must be code: .text, a .text.* section or an SHC code
section. An object whose code is SHC's sections and GCC's both, as inline
assembly's is, gives the kernel all of its code or none of it. The kernel
must touch at most 256 lines of 32 bytes, the 8 KB of the SH-4's
instruction cache, so that no two of its lines take one set of the cache.
The script asserts it, so a kernel that touches more fails the link. The
kernel changes the order of no other section: its objects' other sections,
and every other file's, keep the order they take without it.

Every run has bounds but a head or tail run whose name another run takes.
SHC's assembly writes them as STARTOF X, (STARTOF X)+(SIZEOF X) and SIZEOF
X (GCC.md#coordinated-link). __startof_X and __endof_X lie in the
run's output section. The script assigns __sizeof_X outside every output
section, so it is an absolute value. image_start lies at the image's start,
and image_end at its end. The runs' bounds, image_start and image_end are
the boundaries of the link, which the script defines. No import takes one.

A run with bounds starts on the largest alignment of its sections, so no
padding lies between __startof_X and its first section. ld starts each
output section on the largest alignment of its sections, so the first run
of .text, .rodata, .data and .bss starts on its own. The script aligns each
other run that has bounds.

The script places .comment, the compiler's name, at address 0, where it
loads nowhere. It places each DWARF section DEBUG lists there too, under
its own name. ld fails on any other DWARF section, as one the script
doesn't place.

ld.py fails on SPEC as gcclink.py's read() does.

ld.py links each image twice with ld -q --no-relax, from the same inputs,
under DIR/IMAGE/. -q keeps the relocations in the ELF, for
tools/verify.py's binding check. Every image's layout pass runs, then
resolve, then every image's final pass.

- The layout pass writes each import's provisional value, its provider's
  origin in its view, to provisional.ld. It links by layout.ld, which
  includes provisional.ld, into layout.map and layout.elf. The script gives
  the load area as a MEMORY region. It places .head, .text, .rodata, .data,
  .bss and .tail there, in that order.
- resolve binds each import to its provider's export of SYMBOL, in the
  provider's layout.elf and in the import's view. tools/gcclink.py says
  what an export is. The absolute values the script assigns lie in no
  section, so none is an export.
- The final pass writes resolve's values to imports.ld. It links by
  final.ld into final.map and final.elf. final.ld includes imports.ld where
  layout.ld includes provisional.ld, and lays the image out alike.

ld collects no section in either pass, so each section of an input the
link takes stays. The script roots, by EXTERN, each name another image
imports from the image, and each SYMBOL of its aliases. A root pulls in the
archive member that defines it.

A pass fails where ld fails or prints anything. ld fails on a warning, on a
name the image takes that nothing binds, and on a section the script
doesn't place. It fails on an image that starts past ORIGIN or runs past
its load area, and on a dynamic relocation. It fails on an input that
defines a name the image binds or a boundary of the link, and on an alias
whose SYMBOL nothing defines.

ld.py fails a pass on a weak reference that nothing defines: it has no
policy for one. It fails on a head, tail or kernel row whose PATH is none
of the image's objects, or whose INPUT is no allocated section of PATH. It
fails on two rows that place one input section, on a SECTION whose rows
stand apart, on a kernel row whose INPUT is no code, and on a kernel that
takes part of an object's code that is SHC's sections and GCC's both.
resolve fails on an import whose provider's layout.elf doesn't export
SYMBOL, and on an alias whose SYMBOL the image's own layout.elf doesn't
export.

The layout check then runs, and ld.py fails on every problem it finds,
each naming its image and the section, name or input section at fault. It
holds each image's signature in its final pass to its signature in its
layout pass. The signature holds each allocated section's place, size and
flags, and whether it is zeroed. It holds the place, size and section of
each name defined in an allocated section, local or not, and of each
boundary of the link, wherever it lies. It holds the place and size of
each input section the map places in an allocated section, so it holds
each archive member the link takes. So each import's value, which resolve
read from its provider's layout pass, holds in the provider's final pass.
tools/verify.py then checks the final passes.

A final.elf ld.py leaves has passed the layout check alone. tools/verify.py
checks the rest, and make has tools/publish.py write the manifest, which
marks a checked link, only once verify passes. So that the final passes in
DIR are one run's, ld.py removes the final.elf of each directory in DIR as
it starts, before it reads SPEC. It removes them again where the link
fails, or where ^C, ^\\, a hangup or TERM stops it. A run that stops part
way then leaves no final.elf of an earlier run beside its own. KILL, which
no process can catch, can leave a run's own final.elf files behind.

ld.py fails, naming each final.elf it can't remove, once it has tried the
rest. It fails too on a symlink in DIR to a directory, through which it
removes nothing. Where the run has failed or been stopped already, ld.py
names them, and then exits as that failure or stop would.
"""

import argparse
import fnmatch
import subprocess
import sys
from collections import namedtuple
from pathlib import Path

import elf
import gcclink
from tables import hexa
from tool import fail, remove, save, stoppable

# the SH-4's instruction cache: its lines, and the bytes of each. It holds
# one line a set, and takes an address's set from its bits 5 to 12, so 256
# lines in a row take each set once
ICACHE_LINES, ICACHE_LINE = 256, 32
# the file of each pass's import values, by the pass's name, which is also
# the stem of its script's, map's and ELF's names
PASSES = {'layout': 'provisional.ld', 'final': 'imports.ld'}
# the output sections between .head and .tail, in order. Each gives its
# name, the patterns of GCC's sections it takes, and the kinds of SHC's. Its
# first kind names its first run
OUTPUTS = (('.text', '.text .text.*', 'P'), ('.rodata', '.rodata .rodata.*', 'C'),
           ('.data', '.data .data.*', 'DR'), ('.bss', '.bss .bss.*', 'B'))
# the DWARF sections the script places at 0: those of make gcc's objects and
# of libgcc.a's members. ld fails on any other, as an orphan
DEBUG = ('.debug_aranges', '.debug_info', '.debug_abbrev', '.debug_line', '.debug_frame', '.debug_str',
         '.debug_line_str', '.debug_loclists', '.debug_rnglists')
SCRIPT = '''\
/* {name}'s {stage} pass, as tools/ld.py links it */
MEMORY
{{
  area (rwx) : ORIGIN = {origin}, LENGTH = {length}
}}
INCLUDE "{assigned}"
{named}{roots}SECTIONS
{{
{sections}  ASSERT(_image_start == ORIGIN(area), "it starts past its origin, {origin}")
  ASSERT(_image_end <= ORIGIN(area) + LENGTH(area), "it runs past its load area")
{sizes}  /* where SH's backend would put a dynamic relocation against a name the
     script assigns, as an import: a static link writes none. An SHC
     section's relocations take its name after .rela, and none of those
     lands here either */
  .rela.dyn : {{ *(.rela.*) *(.rela[A-Z]*) }}
  ASSERT(SIZEOF(.rela.dyn) == 0, "it holds a dynamic relocation")
  .comment 0 : {{ *(.comment) }}
{debug}}}
'''

# a run of the script: its name where it takes bounds, or else None; the
# alignment the script gives its start, or 1 for none; and the input
# section statements it holds
Run = namedtuple('Run', 'name align statements')


def provisional(images, image):
    """The value each of image's imports takes in its layout pass, by ALIAS:
    its provider's origin, in the import's view. It fails as
    gcclink.in_view() does."""
    return {alias: gcclink.in_view(images[wanted.provider].origin, wanted.view, f'{image.name}: {alias}')
            for alias, wanted in image.imports.items()}


def code(name):
    """Whether name, an input section's, names code: .text, a .text.*
    section or one of SHC's P sections."""
    return (name == '.text' or name.startswith('.text.')
            or (gcclink.SECTION.fullmatch(name) is not None and name[0] == 'P'))


def file_pattern(path):
    """path as a pattern of ld's that matches path alone: its last character
    in brackets. ld opens a file the script names outright ahead of the
    command line's, which would move the file's other sections ahead of the
    files before it."""
    return f'"{path[:-1]}[{path[-1]}]"'


def check_rows(image, objects):
    """Fail, naming image, on every problem of its head, tail and kernel rows
    at once. objects maps the path of each of image's objects to its
    allocated sections, by name.

    A row fails where PATH is no object of image's inputs, or INPUT is no
    allocated section of it. Two rows that place one input section fail. A
    SECTION whose rows don't stand together, all in the head or all in the
    tail, fails. A kernel row fails where INPUT is no code, and the kernel
    fails where it takes some of an object's code and not all, and the
    object's code is SHC's sections and GCC's both."""
    pieces = image.heads + image.tails + image.kernel
    problems = [f"{image.name}: its {piece.section or 'kernel'} row places {piece.path}'s {piece.input}, "
                + ('and that object holds no such section' if piece.path in objects else
                   'and that is no object of its inputs')
                for piece in pieces if piece.input not in objects.get(piece.path, ())]
    placed = [(piece.path, piece.input) for piece in pieces]
    problems += [f"{image.name}: its rows place {path}'s {name} twice" for path, name in sorted(set(placed))
                 if placed.count((path, name)) > 1]
    # None stands between the head's rows and the tail's, so a SECTION on
    # both sides of it stands apart
    order = [piece.section for piece in image.heads] + [None] + [piece.section for piece in image.tails]
    for name in sorted(set(order) - {None}):
        at = [index for index, section in enumerate(order) if section == name]
        if at != list(range(at[0], at[-1] + 1)):
            problems.append(f'{image.name}: its rows split {name} into runs apart')
    problems += [f"{image.name}: its kernel row places {piece.path}'s {piece.input}, which is no code"
                 for piece in image.kernel if piece.path in objects and not code(piece.input)]
    for path in sorted({piece.path for piece in image.kernel if piece.path in objects}):
        held = {name for name, section in objects[path].items() if code(name) and section.size}
        taken = {piece.input for piece in image.kernel if piece.path == path}
        if held - taken and any(name.startswith('.') for name in held) and any(name[0] == 'P' for name in held):
            problems.append(f"{image.name}: its kernel takes some of {path}'s code, which is SHC's sections and "
                            "GCC's both, and leaves " + ', '.join(sorted(held - taken)))
    if problems:
        fail(*problems)


def runs(image, found):
    """Each output section of image's script as (output, runs), in order.
    Each run is a Run. found is image's inputs, as gcclink.inputs() gives
    them. It fails as check_rows() does."""
    objects = {}
    for path, member, obj in found:
        if member is None:
            objects.setdefault(path, {section.name: section for section in obj.sections if section.flags & elf.ALLOC})
    check_rows(image, objects)
    placed = {(piece.path, piece.input) for piece in image.heads + image.tails}
    tailed = {(piece.path, piece.input) for piece in image.tails}
    # the largest alignment of each name's allocated SHC sections, by the
    # name. A section a head or tail row places counts for none
    aligned = {}
    for path, member, obj in found:
        for section in obj.sections:
            if (section.flags & elf.ALLOC and gcclink.SECTION.fullmatch(section.name)
                    and (member is not None or (path, section.name) not in placed)):
                aligned[section.name] = max(aligned.get(section.name, 1), section.align)
    bounded = {match[2] for _, _, obj in found for symbol in obj.symbols if symbol.section == elf.UNDEFINED
               for match in [gcclink.BOUND.fullmatch(symbol.name)] if match}

    def excluded(pattern):
        # the objects from which a tail row takes a section that pattern
        # matches
        return sorted({path for path, name in tailed if fnmatch.fnmatchcase(name, pattern)})

    def statement(*patterns):
        # one input section statement of patterns, which leaves each tail
        # row's section to .tail. ld gives an input section to the first
        # statement that matches it, and .tail's come last. EXCLUDE_FILE
        # leaves out every section of its files that its pattern matches, so
        # the statement names each other such section of those files
        others = sorted({name for pattern in patterns for path in excluded(pattern) for name in objects[path]
                         if fnmatch.fnmatchcase(name, pattern) and (path, name) not in tailed})
        parts = []
        for pattern in [*patterns, *others]:
            files = ' '.join(f'"{path}"' for path in excluded(pattern))
            parts.append(f'EXCLUDE_FILE({files}) {pattern}' if files else pattern)
        return f'*({" ".join(parts)})'

    # the kernel's statements, which open .text's first run, and the
    # assertion that it touches no more lines than the instruction cache
    # holds
    kernel = [f'{file_pattern(piece.path)}({piece.input})' for piece in image.kernel]
    if kernel:
        kernel.append(f'ASSERT((ABSOLUTE(.) - 1) / {ICACHE_LINE} - ABSOLUTE(__startof_P) / {ICACHE_LINE} < '
                      f'{ICACHE_LINES}, "its kernel touches more than the {ICACHE_LINES} lines of the SH-4\'s '
                      'instruction cache");')
    outputs = []
    for output, gcc, kinds in OUTPUTS:
        names = sorted(name for name in aligned if name[0] in kinds)
        own = [name for name in names if name in bounded and name != kinds[0]]
        # the script aligns no first run: ld starts an output section on the
        # largest alignment of its sections, which covers its first run's
        first = Run(kinds[0], 1, (kernel if output == '.text' else [])
                    + [statement(*gcc.split(), *(name for name in names if name not in own))]
                    + (['*(COMMON)'] if output == '.bss' else []))
        outputs.append((output, [first, *(Run(name, aligned[name], [statement(name)]) for name in own)]))
    taken = {run.name for _, output_runs in outputs for run in output_runs}
    rows_aligned = {}
    for piece in image.heads + image.tails:
        if piece.section not in taken:
            rows_aligned[piece.section] = max(rows_aligned.get(piece.section, 0),
                                              objects[piece.path][piece.input].align)

    def grouped(pieces):
        # the runs of pieces, in row order. Rows next to each other whose
        # runs take no bounds share one
        made = []
        for piece in pieces:
            name = piece.section if piece.section in rows_aligned else None
            if not made or made[-1].name != name:
                made.append(Run(name, rows_aligned.get(name, 1), []))
            made[-1].statements.append(f'{file_pattern(piece.path)}({piece.input})')
        return made

    return ([('.head', grouped(image.heads))] if image.heads else []) + outputs + (
        [('.tail', grouped(image.tails))] if image.tails else [])


def sections_text(image, found):
    """The text of the output sections of image's script, and the names of
    the runs that take bounds, in script order, as a pair. found is image's
    inputs, as gcclink.inputs() gives them. Each output section lies in
    area, the script's MEMORY region. image_start opens the first, and
    image_end closes the last. It fails as runs() does."""
    lines, with_bounds, outputs = [], [], runs(image, found)
    for number, (output, output_runs) in enumerate(outputs):
        lines += [f'  {output} :', '  {']
        if number == 0:
            lines.append('    _image_start = .;')
        for run in output_runs:
            if run.name is None:
                lines += [f'    {statement}' for statement in run.statements]
                continue
            if run.align > 1:
                lines.append(f'    . = ALIGN({run.align});')
            lines += [f'    __startof_{run.name} = .;', *(f'    {statement}' for statement in run.statements),
                      f'    __endof_{run.name} = .;']
            with_bounds.append(run.name)
        if number == len(outputs) - 1:
            lines.append('    _image_end = .;')
        lines.append('  } > area')
    return ''.join(f'{line}\n' for line in lines), with_bounds


def imports_text(image, values, stage):
    """The text of the file of image's import values for its pass named
    stage. values maps each import's ALIAS to its value. Each value's
    assignment follows an assertion that no input defines ALIAS, and names
    the definition it takes in a comment."""
    lines = [f"/* {image.name}'s imports, for its {stage} pass */"]
    for alias, value in values.items():
        wanted = image.imports[alias]
        lines += [f'ASSERT(!DEFINED(_{alias}), "its inputs define {alias}, which it imports")',
                  f"_{alias} = {hexa(value)}; /* {wanted.provider}'s {wanted.symbol} */"]
    return '\n'.join([*lines, ''])


def script_text(image, roots, assigned, stage):
    """The text of image's script for its pass named stage. It includes
    assigned, the file of the pass's import values, and roots each name of
    roots. It fails as gcclink.inputs() and sections_text() do."""
    # the aliases and constants, which the script assigns alike in both
    # passes
    named = ''.join(f'ASSERT(!DEFINED(_{alias}), "its inputs define {alias}, which it aliases")\n'
                    f'ASSERT(DEFINED(_{symbol}), "{alias} is a second name for its {symbol}, which nothing defines")\n'
                    f'_{alias} = DEFINED(_{symbol}) ? ABSOLUTE(_{symbol}) : 0; /* its {symbol} */\n'
                    for alias, symbol in image.aliases.items())
    named += ''.join(f'ASSERT(!DEFINED(_{symbol}), "its inputs define {symbol}, which it binds to {hexa(value)}")\n'
                     f'_{symbol} = {hexa(value)};\n' for symbol, value in image.constants.items())
    sections, with_bounds = sections_text(image, gcclink.inputs(image))
    named += ''.join(f'ASSERT(!DEFINED(_{name}), "its inputs define {name}, a boundary of its link")\n'
                     for name in (*gcclink.IMAGE_BOUNDS,
                                  *(f'_{bound}_{run}' for run in with_bounds for bound in gcclink.RUN_BOUNDS)))
    sizes = ''.join(f'  __sizeof_{name} = __endof_{name} - __startof_{name};\n' for name in with_bounds)
    debug = ''.join(f'  {name} 0 : {{ *({name}) }}\n' for name in DEBUG)
    return SCRIPT.format(name=image.name, stage=stage, origin=hexa(image.origin), length=hexa(image.length),
                         assigned=assigned, named=named, roots=''.join(f'EXTERN(_{root})\n' for root in roots),
                         sections=sections, sizes=sizes, debug=debug)


def link(image, roots, values, out, stage):
    """image's pass named stage, layout or final, linked under out/IMAGE/,
    as a Linked. values maps each of image's imports to its value, by ALIAS,
    and the script roots each name of roots.

    The pass writes imports_text() to the file PASSES names, and
    script_text() to STAGE.ld. ld writes its map, STAGE.map, and its ELF,
    STAGE.elf. The pass removes the ELF an earlier run left before ld runs,
    so that a pass ld fails leaves none.

    It fails, naming the image, where ld fails or prints anything. It fails
    on a weak reference that nothing defines, and removes the ELF. It fails
    naming a directory it can't make, a file it can't write, read or
    remove, and ld where ld can't run. It fails as script_text() does, and
    as gcclink.linked() does on ld's ELF and map."""
    directory = out / image.name
    try:
        directory.mkdir(parents=True, exist_ok=True)
    except OSError as error:
        fail(f'{error.filename}: {error.strerror}')
    assigned = directory / PASSES[stage]
    script, mapped, linked = (gcclink.pass_file(out, image.name, stage, suffix) for suffix in ('.ld', '.map', '.elf'))
    save(assigned, imports_text(image, values, stage).encode())
    save(script, script_text(image, roots, assigned, stage).encode())
    # ld starts each line it prints with its real path, which the failure
    # below takes off
    linker = (gcclink.SH_ELF / 'bin/sh-elf-ld').resolve()
    remove([linked])
    # --no-warn-rwx-segments: the image loads into RAM whole, so a segment
    # both writable and executable is no fault
    command = [linker, '-EL', '-q', '--no-relax', '--orphan-handling=error', '--fatal-warnings',
               '--no-warn-rwx-segments', '-e', f'_{image.entry}' if image.entry else hexa(image.origin),
               '-T', script, '-Map', mapped, '-o', linked, *image.inputs]
    try:
        done = subprocess.run(list(map(str, command)), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                              errors='replace')
    except OSError as error:
        fail(f"{linker}: can't run it: {error.strerror}")
    if done.returncode or done.stdout:
        prefix = f'{linker}: '
        fail(*(f'{image.name}: {line[len(prefix):] if line.startswith(prefix) else line}'
               for line in done.stdout.splitlines()), f'{image.name}: its {stage} pass failed')
    passed = gcclink.linked(out, image.name, stage)
    weak = [elf.as_c_name(symbol.name) for symbol in passed.elf.symbols
            if symbol.bind == elf.WEAK and symbol.section == elf.UNDEFINED]
    if weak:
        remove([linked])
        fail(*(f'{image.name}: {name} is a weak reference that nothing defines, and ld.py has no policy for one'
               for name in weak), f'{image.name}: its {stage} pass failed')
    return passed


def resolve(images, layouts):
    """Each image's import values, by image and ALIAS. An import's value is
    its provider's export of SYMBOL in the provider's layout pass, as
    layouts gives it, spelled in the import's view. An export is one
    gcclink.exports() gives.

    It fails as gcclink.in_view() does. It fails, at once, on each import
    whose SYMBOL its provider's layout pass doesn't export, and each alias
    whose SYMBOL the image's own doesn't."""
    exported = {name: gcclink.exports(linked.elf) for name, linked in layouts.items()}
    bound, problems = {}, []
    for image in images.values():
        bound[image.name] = {}
        for alias, wanted in image.imports.items():
            if wanted.symbol in exported[wanted.provider]:
                bound[image.name][alias] = gcclink.in_view(exported[wanted.provider][wanted.symbol].value,
                                                           wanted.view, f'{image.name}: {alias}')
                continue
            why = (': it imports it too' if wanted.symbol in images[wanted.provider].imports else
                   ': it is a boundary of its link, not an export' if gcclink.boundary(wanted.symbol) else '')
            problems.append(f"{image.name}: {alias} takes {wanted.provider}'s {wanted.symbol}, which "
                            f"{layouts[wanted.provider].elf.where} doesn't define{why}")
        for alias, symbol in image.aliases.items():
            if symbol not in exported[image.name]:
                problems.append(f'{image.name}: {alias} is a second name for its {symbol}, which '
                                f"{layouts[image.name].elf.where} doesn't define")
    if problems:
        fail(*problems)
    return bound


def signature(linked):
    """What the layout check compares of linked, a pass, as a description of
    each item, by the item.

    - Each allocated section gives its start, end and flags, and whether it
      is zeroed. Its item is its name.
    - Each name defined in an allocated section, local or not, gives the
      value, size and section of each symbol of that name. So does each
      boundary of the link, wherever it lies. Its item is the name less one
      leading underscore.
    - Each input section the map places in an allocated section gives the
      address, size and output section of each placement. Its item is
      FILE's SECTION."""
    found = linked.elf
    allocated = elf.allocated_sections(found)
    described = {section.name: f'{hexa(section.address)} to {hexa(section.address + section.size)}, flags '
                               f'{section.flags:#x}' + (', zeroed' if section.kind == elf.NOBITS else '')
                 for section in allocated.values()}
    places = {}
    for symbol in found.symbols:
        name = elf.as_c_name(symbol.name)
        if elf.names_definition(symbol) and (symbol.section in allocated or gcclink.boundary(name)):
            section_name = allocated[symbol.section].name if symbol.section in allocated else 'no section'
            places.setdefault(name, []).append(f'{hexa(symbol.value)}, {symbol.size} bytes of {section_name}')
    outputs = {section.name for section in allocated.values()}
    for placed in linked.placements:
        if placed.output in outputs:
            places.setdefault(f"{placed.file}'s {placed.section}", []).append(
                f'{hexa(placed.address)}, {placed.size} bytes of {placed.output}')
    described.update((item, '; '.join(sorted(each))) for item, each in places.items())
    return described


def moved(layouts, finals):
    """The layout check's problems: one for each item of an image's
    signature that its two passes describe otherwise, or that one lacks.
    layouts and finals give each image's layout pass and final pass."""
    problems = []
    for name, layout in layouts.items():
        old, new = signature(layout), signature(finals[name])
        for item in sorted(old.keys() | new.keys()):
            if old.get(item) != new.get(item):
                problems.append(f"{name}: {item} is {old.get(item, 'absent')} in {layout.elf.where}, and "
                                f"{new.get(item, 'absent')} in {finals[name].elf.where}")
    return problems


def clear(out):
    """Remove the final.elf of each directory in out, whether or not the
    spec names its image. Once it has tried each, it fails, naming each
    final.elf it can't remove. It fails too on each symlink in out to a
    directory, through which it removes nothing."""
    problems = []
    for each in out.glob('*'):
        if each.is_symlink() and each.is_dir():
            problems.append(f'{each}: a symlink to a directory, through which ld.py removes nothing')
            continue
        try:
            gcclink.final(out, each.name).unlink()
        except (FileNotFoundError, NotADirectoryError):
            pass  # it holds no final.elf, or it isn't a directory
        except OSError as error:
            problems.append(f'{error.filename}: {error.strerror}')
    if problems:
        fail(*problems)


def coordinate(images, out):
    """Link images, the spec's, under out, and hold each final pass to its
    layout pass. It returns the import values, layout passes and final
    passes, each by image, as resolve() and link() give them. Every layout
    pass runs, then resolve(), then every final pass, then the layout check.

    So that the final passes in out are one run's, coordinate() removes
    the final.elf of each directory in out, as clear() does, before the
    first pass. It removes them again where anything raises, as a failure,
    ^C, or a signal that stoppable() turns into SystemExit does. A run that
    stops part way then leaves no final.elf of an earlier run beside its
    own. Where clear() fails then, coordinate() writes its problems to
    stderr. It then raises what stopped the run.

    It fails as clear(), provisional(), link() and resolve() do. It fails on
    every problem the layout check finds, at once."""
    clear(out)
    try:
        roots = gcclink.rooted(images)
        layouts = {name: link(image, roots[name], provisional(images, image), out, 'layout')
                   for name, image in images.items()}
        bound = resolve(images, layouts)
        finals = {name: link(image, roots[name], bound[name], out, 'final') for name, image in images.items()}
        problems = moved(layouts, finals)
        if problems:
            fail(*problems)
    except BaseException:
        try:
            clear(out)
        except SystemExit as left:
            print(left, file=sys.stderr)
        raise
    return bound, layouts, finals


def main(argv):
    parser = argparse.ArgumentParser(prog='ld', description=__doc__.split('\n\n')[0])
    parser.add_argument('spec', metavar='SPEC', help='the images, their inputs and their imports, a row each')
    parser.add_argument('out', metavar='DIR', type=Path, help='the directory the passes write under, DIR/IMAGE/')
    args = parser.parse_args(argv)
    clear(args.out)
    coordinate(gcclink.read(args.spec), args.out)


if __name__ == '__main__':
    stoppable()
    main(sys.argv[1:])
