"""Write make gcc's files from its images' final links, check them against
the memory contract, and write the manifest that marks them a build.

    python3 tools/publish.py SPEC DIR MANIFEST

SPEC is tools/ld.py's spec, and DIR the directory tools/ld.py linked the
images under: each image's final link is DIR/IMAGE/final.elf. SH_ELF in the
environment names the toolchain's directory, tools/sh-elf by default.

For each image of the spec, publish writes, under build/gcc/ at the
original's path on the disc, the file its loader reads. A .PRS file's image
goes to the .BIN beside it, which build/prskit compresses to the .PRS. The
file runs from the image's origin to the end of its last section that holds
bytes. Where the memory contract has the file hold the image's bss, it runs
to the end of its last section, bss included:

- an event's, where the image ends past the bytes event_load zeroes;
- any other image's, where its original file holds any bss of its tables,
  but for an image strt1.src's tables start, whose startup zeroes it.

A byte of the file that no section's bytes give is 0.

publish then checks the images and their files, and fails naming every
problem the checks find:

- tools/memory.py's, of each image's end; the end of its body, where its
  tail, if any, starts; the end of its file, and that end in whole sectors,
  for a loader that reads the file whole; its PRS file's size in whole
  sectors; and the work past it, from the word of its bounds that points to
  the work;
- in an image strt1.src's tables start, _INITSCT must zero every byte of
  its bss and no other, through B to B32 and BSG to BSG32, and copy
  nothing, through R to R32 and RSG to RSG32;
- in MOVIE.BIN, each of PSGSFD01 to PSGSFD03 must start near the end of the
  section before it, as MPVASEC_CheckSectionSeq, which MPV_Init runs, takes
  near.

publish fails too on:

- a spec that can't be read, that tools/gcclink.py's read() refuses or
  that links an image config/ doesn't describe, or a tree tools/memory.py
  can't read the memory contract from;
- a toolchain that holds no cc1, or several;
- a final link that can't be read or isn't an ELF, or that holds a
  nonempty allocated section outside its image's interval, or two that
  overlap;
- a name publish reads that a final link defines more than once, or whose
  word there runs past its section's bytes, and an image_end, import,
  alias or constant of an image that its final link doesn't define;
- a file it can't write or remove, a directory it can't make, or a
  build/prskit it can't run or that fails to compress a file;
- a .PRS file that build/prskit fails to decompress, or decompresses to
  other bytes than the .BIN beside it;
- an image with work past it whose final link holds no word that points
  to the work;
- a tool, input or file it hashes for the manifest that can't be read, or
  a row of the manifest that tools/manifest.py can't write, as of a path
  with a space, which leaves the files it wrote and no manifest.

Last, publish writes MANIFEST, in tools/manifest.py's form:

- a comment that publish writes it once its checks pass;
- the hashes of sh-elf-gcc, its cc1, sh-elf-as and sh-elf-ld in the
  toolchain's directory, of build/prskit and of SPEC;
- for each image, its origin and length, as the spec gives them, and the
  end its final link gives it; the hash of each input; each import with its
  provider and symbol, each alias with its symbol, and each constant, each
  with the value its final link gives it; and the hashes of its final.elf
  and of each file publish writes for it.

It hashes each final link and each file it writes from the bytes it
checks, and each tool, the spec and each input as they lie once its checks
pass. The manifest doesn't record that make gcc's other checks ran, which
make runs before publish, or the memory contract's sources, which
tools/disc.py reads anew.

A run reads SPEC and the memory contract, finds the toolchain's cc1, and
fails where MANIFEST, or the partial one beside it, is the spec, a tool, an
input, a final link or a file the run writes. Then it removes MANIFEST, any
partial one beside it and each file it writes, and removes those files
again where writing or checking them fails or is interrupted. So a run that
fails before it removes them leaves them as a run before wrote them, and no
manifest names a file a failed run wrote. A hangup, ^\\ or TERM interrupts
a run as ^C does, and stops build/prskit before the files go. KILL, which
nothing catches, can leave a run's files and a partial manifest, but no
manifest it didn't finish. MANIFEST is written beside its place and then
renamed to it, so a manifest is whole wherever one is.
"""

import argparse
import hashlib
import os
import subprocess
import sys
from pathlib import Path

import elf
import gcclink
import memory
from config import Config
from images import MOVIE, files, images
from manifest import Built, Manifest, digest, text
from tables import aligned, hexa
from tool import content, fail, remove, save, stoppable

PRSKIT = Path('build/prskit')
OUT = Path('build/gcc')
# the runs _INITSCT zeroes, B through B32 and then BSG through BSG32, each
# as the C names of the words of strt1.src's tables that hold its start and
# its end
ZEROES = (('_B_BGN', '_B_END'), ('_BSG_BGN', '_BSG_END'))
# the runs _INITSCT copies initial data to, R through R32 and then RSG
# through RSG32, each as ZEROES gives one
COPIES = (('_D_BGN', '_D_END'), ('_DSG_BGN', '_DSG_END'))
# MOVIE.BIN's sections whose bounds MPVASEC_CheckSectionSeq reads, in
# order, and the bits the distance between two addresses may set for
# mpvasec.src's within to take them as near: 0 to 3 or 8 to 11 bytes apart
SEQUENCE = ('PSGSFD00', 'PSGSFD01', 'PSGSFD02', 'PSGSFD03')
NEAR_MASK = 11


def defined(found, name):
    """The symbol an ELF, found, defines for the C name name, or None where
    it defines none. One it defines more than once fails, naming found."""
    symbols = [symbol for symbol in found.symbols if symbol.name == f'_{name}' and symbol.section != elf.UNDEFINED]
    if len(symbols) > 1:
        fail(f'{found.where}: defines _{name} {len(symbols)} times')
    return symbols[0] if symbols else None


def word(found, name):
    """The little-endian 32-bit word an ELF, found, holds at the C name name.
    None where found defines no such name in a section, or defines it in a
    section that holds no bytes. Fails as defined() does, and, naming found,
    where the name lies in no section found holds, or its word runs past
    its section's bytes."""
    symbol = defined(found, name)
    if symbol is None or symbol.section == elf.ABSOLUTE:
        return None
    section = found.sections[symbol.section] if symbol.section < len(found.sections) else None
    if section is not None and section.kind == elf.NOBITS:
        return None
    if section is None or not 0 <= symbol.value - section.address <= len(section.data) - 4:
        fail(f"{found.where}: _{name}, at {hexa(symbol.value)}, names no whole word of a section's bytes")
    at = symbol.value - section.address
    return int.from_bytes(section.data[at:at + 4], 'little')


def bound(found, name):
    """The value of the symbol an ELF, found, defines for the C name name,
    or None where it defines none. Fails as defined() does."""
    symbol = defined(found, name)
    return None if symbol is None else symbol.value


def value(found, name):
    """The value bound() gives the C name name in an ELF, found. A name
    found defines no symbol for fails, naming found."""
    given = bound(found, name)
    if given is None:
        fail(f'{found.where}: defines no _{name}')
    return given


def payload(image, found, holds_bss):
    """The bytes of image's file, or for a .PRS file the bytes it
    decompresses to, from its final ELF, found. They run from image's
    origin to the highest end of an allocated section that holds bytes, or
    where holds_bss, of any nonempty allocated section, bss included. Each
    byte that no allocated section's bytes give is 0.

    Before it makes them, payload() fails, naming found, on a nonempty
    allocated section that runs outside image's interval, from its origin
    for its length, and on two that overlap."""
    allocated = sorted((section for section in found.sections if section.flags & elf.ALLOC and section.size),
                       key=lambda section: section.address)
    problems = [f'{found.where}: {section.name} runs from {hexa(section.address)} to '
                f"{hexa(section.address + section.size)}, outside {image.name}'s interval, {hexa(image.origin)} to "
                f'{hexa(image.origin + image.length)}' for section in allocated
                if not image.origin <= section.address <= section.address + section.size <= image.origin + image.length]
    problems += [f'{found.where}: {after.name} starts at {hexa(after.address)}, inside {before.name}, which ends at '
                 f'{hexa(before.address + before.size)}' for before, after in zip(allocated, allocated[1:])
                 if after.address < before.address + before.size]
    if problems:
        fail(*problems)
    stored = [section for section in allocated if section.kind != elf.NOBITS]
    ends = [section.address + section.size for section in (allocated if holds_bss else stored)]
    data = bytearray(max(ends, default=image.origin) - image.origin)
    for section in stored:
        data[section.address - image.origin:section.address - image.origin + section.size] = section.data
    return bytes(data)


def holds(name, terms, end):
    """Whether the file of the image name holds its bss, as the memory
    contract, terms, has it. end is image_end in the image's final link.
    An event's file holds its bss where end lies past the bytes event_load
    zeroes, which zeroes no more. The file of an image memory.STARTUP names
    holds none, since its startup zeroes it. Any other image's file holds
    its bss where its original file holds any bss of its tables."""
    if name in terms.events:
        return memory.physical(end) > terms.areas['event zeroing'].end
    return name not in memory.STARTUP and memory.holds_bss(Config(name))


def unheld(start, end, runs):
    """The first address from start up to end that none of runs, each
    (low, high), holds, or None where they hold each."""
    at = start
    while at < end:
        held = next((high for low, high in runs if low <= at < high), None)
        if held is None:
            return at
        at = held
    return None


def startup_problems(found):
    """The problems with how _INITSCT starts an image from strt1.src's
    tables in its final ELF, found. The ranges ZEROES' words bound must hold
    every byte of each bss section, and run forward over bss alone, and
    those COPIES' words bound must be empty. A problem names the first byte
    of a bss section the ranges leave out, the first byte a range zeroes
    that no bss section holds, a range that runs backward, or a copy that
    isn't empty. A missing word is a problem too, and where it is one of
    ZEROES', the only one."""
    problems, ranges = [], []
    for start, end in ZEROES:
        low, high = word(found, start), word(found, end)
        if low is None or high is None:
            return [f"its startup's tables give no {start} or {end}"]
        ranges.append((low, high))
    bss = [section for section in found.sections if section.flags & elf.ALLOC and section.kind == elf.NOBITS
           and section.size]
    for section in bss:
        left = unheld(section.address, section.address + section.size, ranges)
        if left is not None:
            problems.append(f'_INITSCT leaves {section.name} from {hexa(left)} unzeroed: B through B32 run '
                            + ' and BSG through BSG32 '.join(f'{hexa(low)} to {hexa(high)}' for low, high in ranges))
    held = [(section.address, section.address + section.size) for section in bss]
    for run, (low, high) in zip(('B', 'BSG'), ranges):
        past = unheld(low, high, held)
        if low > high:
            problems.append(f'_INITSCT zeroes {run} through {run}32 backward, from {hexa(low)} to {hexa(high)}')
        elif past is not None:
            problems.append(f'_INITSCT zeroes {hexa(past)}, which no bss section holds: {run} through {run}32 run '
                            f'{hexa(low)} to {hexa(high)}')
    for start, end in COPIES:
        low, high = word(found, start), word(found, end)
        if low is None or high is None or low != high:
            problems.append(f'_INITSCT copies {start} to {end}, which a GCC link holds empty: '
                            + (f'{hexa(low)} to {hexa(high)}' if low is not None and high is not None else 'absent'))
    return problems


def sequence_problems(found):
    """The problems MPVASEC_CheckSectionSeq would find in MOVIE.BIN's final
    ELF, found. Each section of SEQUENCE after the first must start near the
    end of the one before it: the distance between the two may set no bit
    outside NEAR_MASK. Where found gives the first of a pair no __endof_ or
    the second no __startof_, the pair is a problem too."""
    problems = []
    for before, after in zip(SEQUENCE, SEQUENCE[1:]):
        end, start = bound(found, f'_endof_{before}'), bound(found, f'_startof_{after}')
        if end is None or start is None:
            problems.append(f'its link gives no bounds for {before} or {after}, which MPVASEC_CheckSectionSeq reads')
        elif abs(start - end) & ~NEAR_MASK:
            problems.append(f'{after} starts at {hexa(start)}, and {before} ends at {hexa(end)}, '
                            f'{abs(start - end)} bytes apart: MPVASEC_CheckSectionSeq fails MPV_Init unless the two '
                            'lie 0 to 3 or 8 to 11 bytes apart')
    return problems


def compiler():
    """The path of the cc1 that make gcc's toolchain, SH_ELF, installs for
    its one release, in a directory of libexec/gcc/sh-elf/. A toolchain
    that holds none fails, naming its libexec/gcc/sh-elf/, and so does one
    that holds several, naming each: the manifest gives the hash of the one
    sh-elf-gcc runs."""
    return gcclink.installed('libexec/gcc/sh-elf', 'cc1')


def replace(path, data):
    """Write data, bytes, to a file beside path whose name is path's with
    .partial after it, then rename that file to path. path then holds either
    data whole or what it held before.

    A write that fails fails, naming the .partial file, and can leave part
    of data in it. A rename that fails removes the .partial file and fails,
    naming path. Where remove() fails then, replace() writes its problem to
    stderr."""
    partial = path.with_name(f'{path.name}.partial')
    save(partial, data)
    try:
        os.replace(partial, path)
    except OSError as error:
        try:
            remove([partial])
        except SystemExit as left:
            print(left, file=sys.stderr)
        fail(f'{path}: {error.strerror}')


def publish(spec, out, manifest):
    """Write the files of the images of the spec at path spec, from their
    final links under out. Then check them, and write manifest last. The
    module docstring gives the files, the checks and the manifest.

    publish() reads the spec and the memory contract, and fails on each
    image of the spec that config/ doesn't describe. It finds the
    toolchain's cc1, and fails where manifest or the .partial file beside
    it is a tool, the spec, an input, a final link or an image's file. Then,
    before it writes anything, it removes manifest, the .partial file and
    each image's files, as remove() does. Where write_images(), or value()
    on a name the manifest gives, fails or is interrupted, it removes the
    images' files again. Where remove() fails then, publish() writes its
    problems to stderr, and raises what stopped the run.

    Fails as gcclink.read(), memory.contract(), compiler(), write_images(),
    value(), tools/manifest.py's text() and replace() do, and where a tool,
    input or file it hashes can't be read."""
    images_of = gcclink.read(spec)
    known = images()
    terms = memory.contract(known)
    original = files()
    problems = [f"{spec}: links {name}, an image config/ doesn't describe" for name in images_of
                if name not in original]
    if problems:
        fail(*problems)
    tools = [gcclink.SH_ELF / 'bin/sh-elf-gcc', compiler(), gcclink.SH_ELF / 'bin/sh-elf-as',
             gcclink.SH_ELF / 'bin/sh-elf-ld', PRSKIT]
    paths, used = {}, [spec, *tools]
    for image in images_of.values():
        path = OUT / original[image.name]
        paths[image.name] = [path.with_suffix('.BIN'), path] if path.suffix == '.PRS' else [path]
        used += [*image.inputs, gcclink.final(out, image.name), *paths[image.name]]
    partial = manifest.with_name(f'{manifest.name}.partial')
    resolved = {Path(path).resolve() for path in used}
    problems = [f"{each}: a file publish reads or writes, which the manifest can't take the place of"
                for each in (manifest, partial) if each.resolve() in resolved]
    if problems:
        fail(*problems)
    owned = [path for image in paths.values() for path in image]
    remove([manifest, partial, *owned])
    try:
        finals, hashed = write_images(images_of, out, terms, paths)
        built = {}
        for image in images_of.values():
            found = finals[image.name]
            built[image.name] = Built(image.origin, image.length, value(found, 'image_end'), image.inputs,
                                      {alias: (wanted.provider, wanted.symbol, value(found, alias))
                                       for alias, wanted in image.imports.items()},
                                      {alias: (symbol, value(found, alias)) for alias, symbol in image.aliases.items()},
                                      {symbol: value(found, symbol) for symbol in image.constants},
                                      [gcclink.final(out, image.name), *paths[image.name]])
    except BaseException:
        try:
            remove(owned)
        except SystemExit as left:
            print(left, file=sys.stderr)
        raise
    named = dict.fromkeys([*tools, spec, *(path for each in built.values() for path in [*each.inputs, *each.files])])
    hashes = {path: hashed.get(path) or digest(path) for path in named}
    replace(manifest, text(Manifest(hashes, tools, spec, built)).encode())


def write_files(image, found, holds_bss, paths):
    """Write the file of image to paths[0]: payload()'s bytes of its final
    ELF, found, bss included where holds_bss. Where the original is a .PRS
    file, paths[1] is that .PRS, which build/prskit compresses paths[0] to.
    build/prskit then decompresses the bytes it reads back of the .PRS.
    Return the hash of each file it writes, by path, of the bytes it wrote or
    read back; the file's size in bytes; and the .PRS's in bytes, or None
    where the original isn't a .PRS file.

    Fails as payload() does, on a file it can't write or read back, on a
    build/prskit it can't run or that fails, and on a .PRS that decompresses
    to other bytes than paths[0]'s. Raises OSError on a directory it can't
    make."""
    data = payload(image, found, holds_bss)
    paths[0].parent.mkdir(parents=True, exist_ok=True)
    save(paths[0], data)
    hashes = {paths[0]: hashlib.sha256(data).hexdigest()}
    if len(paths) == 1:
        return hashes, len(data), None
    try:
        done = subprocess.run([str(PRSKIT), 'compress', *map(str, paths)], stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True, errors='replace')
    except OSError as error:
        fail(f"{PRSKIT}: can't run it: {error.strerror}")
    if done.returncode:
        fail(*done.stdout.splitlines(), f'{image.name}: {PRSKIT} compress failed')
    compressed = content(paths[1])
    try:
        done = subprocess.run([str(PRSKIT), 'decompress', '/dev/stdin', '/dev/stdout'], input=compressed,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    except OSError as error:
        fail(f"{PRSKIT}: can't run it: {error.strerror}")
    if done.returncode:
        fail(*done.stderr.decode(errors='replace').splitlines(),
             f'{image.name}: {PRSKIT} decompress of {paths[1]} failed')
    if done.stdout != data:
        fail(f'{image.name}: {paths[1]} decompresses to other bytes than {paths[0]}')
    hashes[paths[1]] = hashlib.sha256(compressed).hexdigest()
    return hashes, len(data), len(compressed)


def extent(image, found, terms, end, size, packed):
    """The memory.Extent of image, whose final ELF is found and whose end
    there is end. size is the bytes its file holds, and packed the bytes its
    .PRS holds, or None for none. terms is the memory contract. The body
    ends where found's .tail starts, or at end where found has no .tail. The
    work is None where memory.WORK doesn't name the image. Otherwise it
    starts at the address held by the word memory.WORK names, rounded up to
    memory.WORK_ALIGNMENT, and runs as many bytes as terms gives the
    image's work. Fails as word() does, and where found holds no such
    word."""
    body = next((section.address for section in found.sections if section.name == '.tail'), end)
    work = None
    if image.name in memory.WORK:
        name = memory.WORK[image.name][0]
        pointer = word(found, name)
        if pointer is None:
            fail(f'{image.name}: {found.where} holds no {name}, whose word points to its work')
        start = aligned(pointer, memory.WORK_ALIGNMENT)
        work = (start, start + terms.work[image.name])
    return memory.Extent(end, body, image.origin + size, image.origin + aligned(size, terms.sector),
                         None if packed is None else aligned(packed, terms.sector), work)


def write_images(images_of, out, terms, paths):
    """Write each image's files, check them as the module docstring says,
    and return (finals, hashes). finals is each image's final ELF, as
    elf.read() reads it, by the image's name. hashes is the hash of each
    final ELF and file of an image, by path, of the bytes the checks read.

    images_of is gcclink.read()'s images of the spec, out the directory of
    their final links, terms the memory contract, and paths each image's
    files, by name: the file payload() makes of out/IMAGE/final.elf, then,
    for a .PRS original, the .PRS build/prskit compresses it to.

    Where the checks find problems, write_images() fails once every image's
    files are written, naming them all. It fails sooner, at the first image
    where:
    - its final ELF can't be read or doesn't parse, or fails payload()'s,
      value()'s or word()'s checks;
    - one of its files can't be written, or build/prskit can't run or fails;
    - its .PRS decompresses to other bytes than its .BIN;
    - memory.WORK names it, and its final ELF holds no word that points to
      its work."""
    extents, problems, finals, hashes = {}, [], {}, {}
    for image in images_of.values():
        final = gcclink.final(out, image.name)
        linked = content(final)
        found = finals[image.name] = elf.read(linked, final)
        hashes[final] = hashlib.sha256(linked).hexdigest()
        end = value(found, 'image_end')
        files_hashed, size, packed = write_files(image, found, holds(image.name, terms, end), paths[image.name])
        hashes.update(files_hashed)
        extents[image.name] = extent(image, found, terms, end, size, packed)
        if image.name in memory.STARTUP:
            problems += [f'{image.name}: {problem}' for problem in startup_problems(found)]
        if image.name == MOVIE.name:
            problems += [f'{image.name}: {problem}' for problem in sequence_problems(found)]
    problems += memory.check(terms, extents)
    if problems:
        fail(*problems)
    return finals, hashes


def main(argv):
    parser = argparse.ArgumentParser(prog='publish', description=__doc__.split('\n\n')[0])
    parser.add_argument('spec', metavar='SPEC', type=Path, help="tools/ld.py's spec")
    parser.add_argument('out', metavar='DIR', type=Path, help='the directory tools/ld.py linked the images under')
    parser.add_argument('manifest', metavar='MANIFEST', type=Path, help='where to write the manifest, last')
    args = parser.parse_args(argv)
    try:
        publish(args.spec, args.out, args.manifest)
    except OSError as error:
        # a directory write_images() makes
        fail(f'{error.filename}: {error.strerror}')


if __name__ == '__main__':
    stoppable()
    main(sys.argv[1:])
