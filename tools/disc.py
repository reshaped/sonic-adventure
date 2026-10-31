"""Build a disc from the original's .gdi and its tracks, with the files make
gcc's link wrote in their originals' places, and check it.

    python3 tools/disc.py GDI MANIFEST DIR

GDI is the original's .gdi, its tracks beside it, MANIFEST the manifest
tools/publish.py writes once the checks it names pass, and DIR the
directory the disc is written to.

Each image's file takes its original's place on the disc: its .PRS file,
which publish compressed from its image, or its .BIN, as the original is
stored, under build/gcc/ at the original's path. Each event that repeats
another byte for byte, as EVENT_ALIASES in tools/images.py records, takes
the file of the event it repeats. The manifest must give every image
config/ describes: the disc would otherwise keep the image's original,
whose references into the original images no longer hold.

A file grows where it needs more sectors than its original holds. The boot
file, 1ST_READ.BIN, keeps its last sector, as build/gdi rebuild requires,
so it starts later where it needs fewer sectors. Any other file keeps its
first sector unless it grows. Files that grow move in path order, each to
the start of the first free run, by sector, that holds it, and the rest of
that run stays free. The free runs are at first those build/gdi map calls
unreferenced. Each file that grows then frees the sectors it held, which
join any free run they meet. So no two files take the same sectors, and no
file the disc keeps gives any up.

disc.py checks the following in this order. Each group, between blank
lines, names every problem it finds, and disc.py stops after the first
group that finds one.

- a DIR or DIR.partial that exists: disc.py removes nothing it didn't
  write, and make gcc-disc removes both before it runs disc.py;

- a manifest tools/manifest.py's recorded() refuses, for the problems it
  names;

- an image config/ describes that the manifest leaves out, or one the
  manifest names that config/ doesn't describe;

- an image whose file the manifest doesn't name;

- an image the manifest gives another origin or length than the memory
  contract does;
- a .PRS file whose whole sectors, in the contract's sector, pass the work
  area loader_read_prs reads it to;
- a .BIN file whose whole sectors pass its image's interval, as its loader
  reads it whole in sectors to the image's origin;
- an image that doesn't run forward from its origin to the end the
  manifest gives it, within its interval;

- a line of build/gdi map's output for GDI that disc.py can't read;

- a file the memory contract gives a size by hand, as SHA256SUMS holds no
  line for it, that the original disc doesn't hold under SONICADV/ at that
  size;

- a file to replace that the original disc doesn't hold;
- a boot file that grows;
- another file that grows, where no free run holds it;

- a replaced file whose bytes on the new disc don't have the hash the
  manifest gives its source;
- a replaced file that doesn't start where the plan puts it;
- a file the new disc keeps that lies elsewhere, or holds another size,
  than on the original.

disc.py fails too where tools/memory.py can't read the memory contract
from the tree, where build/gdi can't run or fails, and where it can't read
or write a file.

build/gdi rebuild writes the disc to DIR.partial, and checks the disc's
structure and the files it changed against the plan. disc.py reads each
file it replaced back out of that disc, into DIR.partial/check, which it
removes after. Last it writes disc.txt in DIR.partial: the hash of the
manifest's bytes it read, then for each file it replaced, its bytes,
sectors and first sector, the bytes its loader reads in whole sectors
against what holds them, the work area for a .PRS or the image's interval
for a .BIN, and the image's bytes against its interval. Then it renames
DIR.partial to DIR.

DIR appears only once every check passes. A failure removes DIR.partial,
and where it can't, names the file it can't remove. A hangup, ^\\ or TERM
interrupts a run as ^C does, and stops build/gdi before DIR.partial goes.
KILL, which nothing catches, can leave DIR.partial.
"""

import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
from collections import namedtuple
from pathlib import Path

import memory
from images import EVENT_ALIASES, files, images
from manifest import digest, recorded
from tables import aligned, hexa
from tool import content, fail, save, stoppable

GDI = Path('build/gdi')
OUT = Path('build/gcc')
# the bytes of a sector of the disc, as build/gdi map counts them
SECTOR = 2048
# the two forms of a line of build/gdi map's. NAMED gives a file's or
# directory's first sector, sectors, bytes and path, with (boot) after the
# boot file's path. A directory's path ends in /. UNNAMED gives any other
# run's first sector, sectors and what holds it
NAMED = re.compile(r' *(\d+) +(\d+) +(\d+)  (\S.*?)( \(boot\))?')
UNNAMED = re.compile(r' *(\d+) +(\d+) {13}(\S.*)')
FREE = 'unreferenced'

# a run of the disc's sectors, as a line of build/gdi map's gives it: start,
# its first sector, and count, how many. A file's or directory's run has its
# size in bytes, its path and whether it is the boot file, and kind None.
# Any other run has size 0, path None and kind, what holds it
Region = namedtuple('Region', 'start count size path boot kind')


def sectors(size):
    """The number of SECTOR-byte sectors a file of size bytes takes."""
    return aligned(size, SECTOR) // SECTOR


def read(data, path, known):
    """The manifest in data, the bytes of the manifest at path, as
    tools/manifest.py's recorded() reads it. known is the images config/
    describes. Fails as recorded() does, and with a line for each image of
    known that the manifest leaves out, and for each it names that known
    doesn't hold."""
    found = recorded(data, path)
    problems = [f'{path}: names no image {image}, which config/ describes' for image in known
                if image not in found.images]
    problems += [f"{path}: names image {image}, which config/ doesn't describe" for image in found.images
                 if image not in known]
    if problems:
        fail(*problems)
    return found


def replacements(images_of):
    """The file that replaces each file of the disc, by the path of the file
    it replaces, in path order. images_of is a manifest's images, each a
    tools/manifest.py Built, by name. Each image of images_of takes the file
    at its original's path under OUT, a .PRS or a .BIN as the original is
    stored. Each alias of EVENT_ALIASES whose event images_of holds takes
    that event's file. Fails, naming each image whose files in images_of
    hold no such file."""
    originals, found, problems = files(), {}, []
    for image, each in images_of.items():
        path = OUT / originals[image]
        if str(path) not in each.files:
            problems.append(f'{image}: the manifest names no {path}, its file on the disc')
            continue
        found[originals[image]] = path
    for alias, event in EVENT_ALIASES.items():
        if f'SONICADV/{event}.PRS' in found:
            found[f'SONICADV/{alias}.PRS'] = found[f'SONICADV/{event}.PRS']
    if problems:
        fail(*problems)
    return dict(sorted(found.items()))


def regions(text, where):
    """The runs of sectors that text, build/gdi map's output for where,
    lists, as Region in its order. A line in neither of NAMED's and
    UNNAMED's forms fails, naming where and the line's number."""
    found = []
    for number, line in enumerate(text.splitlines(), 1):
        named, unnamed = NAMED.fullmatch(line), UNNAMED.fullmatch(line)
        if named:
            found.append(Region(int(named[1]), int(named[2]), int(named[3]), named[4], bool(named[5]), None))
        elif unnamed:
            found.append(Region(int(unnamed[1]), int(unnamed[2]), 0, None, False, unnamed[3]))
        else:
            fail(f"{where}:{number}: not a line of build/gdi map's")
    return found


def file_runs(mapped):
    """The run of each file of a disc, by the file's path. mapped is the
    disc's runs, as regions() gives them. A directory's run, whose path ends
    in /, is no file's."""
    return {region.path: region for region in mapped if region.path and not region.path.endswith('/')}


def joined(runs):
    """runs, each [start, count], as new lists sorted by start, each joined to
    the run before it where the two meet, and each empty one left out."""
    found = []
    for start, count in sorted(runs):
        if found and found[-1][0] + found[-1][1] == start:
            found[-1][1] += count
        elif count:
            found.append([start, count])
    return found


def plan(mapped, sizes):
    """The new first sector of each file that moves, by disc path. sizes
    gives each file's new size in bytes, by disc path, and mapped the disc's
    runs, as regions() reads them from build/gdi map, which refuses a disc
    whose runs share a sector.

    A file other than the boot file keeps its start where its sectors still
    hold it. The files that need more sectors take free runs in path order:
    each takes the first sectors of the first free run that holds it, and
    its old extent then joins the free runs. The free runs start as those
    the map calls unreferenced, and each is joined to the runs it meets. The
    boot file takes no free run: it keeps its last sector, and starts later
    where it needs fewer sectors.

    The plan fails with a line for each of these:
    - a path the disc holds no file at;
    - a boot file that needs more sectors than it holds;
    - a file that no free run holds."""
    on_disc = file_runs(mapped)
    free = joined([region.start, region.count] for region in mapped if region.kind == FREE)
    placed, problems = {}, []
    for path, size in sorted(sizes.items()):
        if path not in on_disc:
            problems.append(f'{path}: no file of the disc')
            continue
        old, need = on_disc[path], sectors(size)
        if old.boot:
            if need > old.count:
                problems.append(f'{path}: the boot file keeps its end, and it needs {need} sectors, where it holds '
                                f'{old.count}')
            elif need < old.count:
                placed[path] = old.start + old.count - need
            continue
        if need <= old.count:
            continue
        run = next((run for run in free if run[1] >= need), None)
        if run is None:
            problems.append(f'{path}: needs {need} sectors, {need - old.count} more than it holds, and no run of the '
                            "disc's free sectors holds them")
            continue
        placed[path] = run[0]
        run[0] += need
        run[1] -= need
        free = joined(free + [[old.start, old.count]])
    if problems:
        fail(*problems)
    return placed


def limits(images_of, terms):
    """The problems with each image of images_of, a manifest's images as
    replacements() takes them, and its file against the memory contract
    terms, and what holds each, as (problems, lines). lines gives, by the
    file's disc path, the file's bytes in whole sectors against what holds
    them, and the image's bytes against its interval.

    An image whose origin or length in images_of isn't the contract's has
    that problem alone, and no line. Otherwise a .PRS file's whole sectors, in
    the contract's sector, must fit the work area, and a .BIN file's its
    image's interval, and the image must run forward from its origin to its
    end, within its interval."""
    originals, problems, lines = files(), [], {}
    work = terms.areas['work'].end - terms.areas['work'].start
    for image, each in images_of.items():
        path = OUT / originals[image]
        if each.origin != terms.origins[image] or each.length != terms.lengths[image]:
            problems.append(f'{image}: the manifest gives it {hexa(each.origin)} and {each.length:#x} bytes, and the '
                            f'memory contract {hexa(terms.origins[image])} and {terms.lengths[image]:#x}')
            continue
        loaded, size = aligned(len(content(path)), terms.sector), each.end - each.origin
        limit, holder = (work, 'the work area') if path.suffix == '.PRS' else (each.length, 'its interval')
        if loaded > limit:
            problems.append(f'{path}: takes {loaded:#x} bytes in whole sectors, and {holder} holds {limit:#x}')
        if size < 0:
            problems.append(f'{image}: ends at {hexa(each.end)}, before its origin')
        elif size > each.length:
            problems.append(f'{image}: runs to {hexa(each.end)}, past its interval, which ends at '
                            f'{hexa(each.origin + each.length)}')
        lines[originals[image]] = (f'sectors {loaded:#x} of {holder} {limit:#x}, image {size:#x} of its interval '
                                   f'{each.length:#x}')
    return problems, lines


def check_sized_files(on_disc):
    """The problems with the file sizes the memory contract gives by hand,
    against on_disc, the original disc's file runs as file_runs() gives
    them, as lines. Each file memory.MESSAGES, memory.CAST and memory.BANKS
    give a size must be a file of the disc under SONICADV/ of that many
    bytes."""
    problems = []
    for name, size in [*memory.MESSAGES.values(), *memory.CAST.items(), *sum(memory.BANKS.values(), ())]:
        path = f'SONICADV/{name}'
        if path not in on_disc:
            problems.append(f'{path}: no file of the disc, and the memory contract gives it {size} bytes')
        elif on_disc[path].size != size:
            problems.append(f'{path}: holds {on_disc[path].size} bytes on the disc, and the memory contract gives it '
                            f'{size}')
    return problems


def gdi(*argv):
    """What build/gdi run with argv prints, on stdout and stderr together,
    each byte that isn't UTF-8 replaced. A build/gdi that can't run fails,
    naming it, and a run that exits nonzero fails with what it printed, then
    a line naming its command."""
    try:
        done = subprocess.run([str(GDI), *map(str, argv)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              encoding='utf-8', errors='replace')
    except OSError as error:
        fail(f"{GDI}: can't run it: {error.strerror}")
    if done.returncode:
        fail(*done.stdout.splitlines(), f'{GDI} {argv[0]} failed')
    return done.stdout


def removed(path):
    """Remove the directory at path and all it holds, where it exists. One
    it can't remove fails, naming the first file it can't remove."""
    try:
        shutil.rmtree(path)
    except FileNotFoundError:
        pass
    except OSError as error:
        fail(f'{error.filename}: {error.strerror}')


def check_rebuilt(disc, check, on_disc, sources, placed, hashes):
    """The problems with the rebuilt disc at disc, as lines. sources gives
    each file the rebuild replaced, by disc path, as the path of its
    source, and hashes gives the manifest's hash of each source, by that
    path.

    Each replaced file, read back out of the disc into the directory check,
    must have the hash of its source. check_rebuilt() makes check, and
    removes it as removed() does once it has read each file. A failure before
    then leaves it: build() puts it in DIR.partial, which a failure removes.
    The disc's map must start each replaced file where placed puts it, or
    where on_disc, the original's file runs as file_runs() gives them, does
    where placed doesn't. Each file the disc keeps must have the first
    sector, sectors and bytes on_disc gives it.

    Fails at once:
    - where check exists, or can't be made, and as removed() does;
    - as gdi() does, where build/gdi extract or map fails;
    - as digest() does, on a file extract leaves out;
    - as regions() does, on a line of the rebuilt disc's map that doesn't
      parse."""
    problems = []
    try:
        check.mkdir()
    except OSError as error:
        fail(f'{check}: {error.strerror}')
    gdi('extract', disc, check, *sources)
    for path, source in sources.items():
        if digest(check / path) != hashes[str(source)]:
            problems.append(f"{path}: the disc's bytes aren't {source}'s, whose hash the manifest gives")
    removed(check)
    now = file_runs(regions(gdi('map', disc), disc))
    for region in on_disc.values():
        got = now.get(region.path)
        if region.path in sources:
            start = placed.get(region.path, region.start)
            if got is None or got.start != start:
                problems.append(f'{region.path}: starts at {got.start if got else "no sector"}, and the plan puts it '
                                f'at {start}')
        elif got is None or (got.start, got.count, got.size) != (region.start, region.count, region.size):
            problems.append(f'{region.path}: a file the disc keeps, at {region.start}, {region.count} sectors and '
                            f'{region.size} bytes, lies at {got.start if got else "no sector"} with '
                            f'{got.count if got else 0} sectors and {got.size if got else 0} bytes')
    return problems


def build(original, manifest, out):
    """Build the disc at out, from the original's .gdi, original, and the files
    the manifest at manifest names, and check it, as the module docstring
    gives. The disc is built at out.partial, which takes out's name once
    every check passes. out and out.partial must not exist, and a failure or
    an interrupt removes out.partial, check and all, as removed() does.
    Where removed() fails then, build() writes its problem to stderr, and
    raises what stopped the run."""
    partial = Path(f'{out}.partial')
    problems = [f"{each}: exists, and disc.py removes nothing it didn't write" for each in (out, partial)
                if os.path.lexists(each)]
    if problems:
        fail(*problems)
    known = images()
    terms = memory.contract(known)
    listed = content(manifest)
    found = read(listed, manifest, known)
    sources = replacements(found.images)
    problems, usage = limits(found.images, terms)
    if problems:
        fail(*problems)
    mapped = regions(gdi('map', original), original)
    on_disc = file_runs(mapped)
    problems = check_sized_files(on_disc)
    if problems:
        fail(*problems)
    sizes = {path: len(content(source)) for path, source in sources.items()}
    placed = plan(mapped, sizes)
    edits = [field for path, source in sources.items() for field in ('--replace', path, source)]
    edits += [field for path, start in placed.items() for field in ('--place', path, start)]
    try:
        gdi('rebuild', original, partial, *edits)
        disc = partial / Path(original).name
        problems = check_rebuilt(disc, partial / 'check', on_disc, sources, placed, found.hashes)
        if problems:
            fail(*problems)
        lines = [f"# the disc tools/disc.py builds from {manifest}: the manifest's hash, then each file it replaced, "
                 'its bytes, sectors and first sector, and what holds it against its limit',
                 f'manifest  {hashlib.sha256(listed).hexdigest()}  {manifest}']
        for path, source in sources.items():
            lines.append(f'file  {path}  {sizes[path]}  {sectors(sizes[path])}  '
                         f'{placed.get(path, on_disc[path].start)}  {usage[str(source.relative_to(OUT))]}')
        save(partial / 'disc.txt', ''.join(f'{line}\n' for line in lines).encode())
        try:
            os.replace(partial, out)
        except OSError as error:
            fail(f'{partial}: {error.strerror}')
    except BaseException:
        try:
            removed(partial)
        except SystemExit as left:
            print(left, file=sys.stderr)
        raise


def main(argv):
    parser = argparse.ArgumentParser(prog='disc', description=__doc__.split('\n\n')[0])
    parser.add_argument('original', metavar='GDI', type=Path, help="the original disc's .gdi, its tracks beside it")
    parser.add_argument('manifest', metavar='MANIFEST', type=Path, help="make gcc's manifest")
    parser.add_argument('out', metavar='DIR', type=Path, help='where to write the disc')
    args = parser.parse_args(argv)
    build(args.original, args.manifest, args.out)


if __name__ == '__main__':
    stoppable()
    main(sys.argv[1:])
