"""Write make's dependencies of each binary.

    python3 tools/deps.py DEPS

build/deps.mk makes each binary depend on its splits.txt and on each
source's linked source, and for a source with bodies on its stub's too. The
binaries are the images config/ describes except the motion files, which the
matching build doesn't rebuild.

The file is rewritten only when its text changes, since make runs this
whenever it includes build/deps.mk.

Each object's header dependencies are in its .d file, not here.
"""

import argparse
import sys
from pathlib import Path

from bodies import has_bodies
from config import Config
from images import binaries, files
from link import linked
from tool import content, fail, save


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


def main(argv):
    parser = argparse.ArgumentParser(prog='deps', description=__doc__.split('\n\n')[0])
    parser.add_argument('deps', metavar='DEPS', help="where to write make's dependencies, build/deps.mk")
    args = parser.parse_args(argv)
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
