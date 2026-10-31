"""Write make's dependencies of each binary, or with --gcc those of make
gcc's import check.

    python3 tools/deps.py DEPS
    python3 tools/deps.py --gcc DEPS

build/deps.mk makes each binary depend on its splits.txt and on each
source's linked source, and for a source with bodies on its stub's too. The
binaries are the images config/ describes except the motion files, which the
matching build doesn't rebuild.

build/gcc/deps.mk makes the import check, build/gcc/imports.ok, depend on
the object of each source whose placement holds the address of one of
1ST_READ.BIN's imports. For a source with bodies, the rule names the stub's
object, which builds without the disc. A gen/ range adds no object, since
the check reads none for an import there.

Each file is rewritten only when its text changes, since make runs this
whenever it includes build/deps.mk or build/gcc/deps.mk.

Each object's header dependencies are in its .d file, not here.
"""

import argparse
import sys
from pathlib import Path

from bodies import has_bodies
from config import Config, imports
from images import binaries, files
from link import linked
from tool import content, fail, save

# each command's command line, which -h and misuse print after "usage: "
USAGE = '''%(prog)s DEPS
       %(prog)s --gcc DEPS'''


def rule(target, prerequisites):
    return f'{target}: {" ".join(map(str, prerequisites))}\n'


def write(out, text):
    out = Path(out)
    if not out.exists() or content(out) != text.encode():
        try:
            out.parent.mkdir(parents=True, exist_ok=True)
        except OSError as error:
            # out's directory, or one above it, whichever mkdir couldn't make
            fail(f'{error.filename}: {error.strerror}')
        save(out, text.encode())


def images():
    # each binary, as its file under build/, or for a .PRS file the .BIN it
    # decodes to, and its name
    originals = files()
    return [(Path('build', originals[name]).with_suffix('.BIN'), name) for name in binaries()]


def checked_objects():
    """The objects the import check depends on, each once, in imports.txt's
    order."""
    opened, found = {}, []
    for _, image, _, address, _ in imports():
        if image not in opened:
            opened[image] = Config(image)
        source = next((source for source, _, low, high in opened[image].placements if low <= address < high), None)
        if source is not None and not source.startswith('gen/'):
            found.append(Path('build/stub' if has_bodies(source) else 'build', source).with_suffix('.obj'))
    return list(dict.fromkeys(found))


def main(argv):
    parser = argparse.ArgumentParser(prog='deps', usage=USAGE, description=__doc__.split('\n\n')[0])
    parser.add_argument('deps', metavar='DEPS',
                        help="where to write make's dependencies: build/deps.mk, or with --gcc build/gcc/deps.mk")
    parser.add_argument('--gcc', action='store_true', help="write the import check's dependencies to DEPS")
    args = parser.parse_args(argv)
    if args.gcc:
        write(args.deps, rule('build/gcc/imports.ok', checked_objects()))
        return
    lines = []
    for image, name in images():
        config = Config(name)
        bins = []
        for source in config.sources():
            bins.append(linked(source, config.name))
            if has_bodies(source):
                bins.append(linked(source, config.name, 'build/stub'))
        lines.append(rule(image, [config.splits, *bins]))
    write(args.deps, ''.join(lines))


if __name__ == '__main__':
    main(sys.argv[1:])
