"""Write make's dependencies of each image and declaration scan, and the
list of the C and the tables.

build/deps.mk makes each image depend on its splits.txt and on each source's
linked source, and for a source with bodies on its stub's too. The images
are those config/ describes. An image with gen/ ranges also has its
declaration scan, build/gen/<image>/declared.stamp, depend on the .c and .h
files the scan reads, its tables and 1ST_READ.BIN's symbols.txt.
build/sources.txt lists every .c and .h file under ROOTS, then every
config/*/*.txt, one path a line, so it changes only when one is added or
removed, and the declaration scans, which depend on it, rerun then. Each
file is rewritten only when its text changes, since make runs this whenever
it includes build/deps.mk.

Each object's header dependencies are in its .d file, not here.
"""

import argparse
import sys
from pathlib import Path

from config import CONFIG, FIRST, ROOTS, Config, content, directories, fail, files, save
from link import linked
from splice import has_bodies


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
    # each image config/ describes, as its file under build/, or for a .PRS
    # file the .BIN it decodes to, and its name
    return [(Path('build', file).with_suffix('.BIN'), name) for name, file in files().items()]


def main(argv):
    parser = argparse.ArgumentParser(prog='deps', description=__doc__.split('\n\n')[0])
    parser.add_argument('deps', metavar='DEPS', help="where to write make's dependencies, build/deps.mk")
    parser.add_argument('sources', metavar='SOURCES',
                        help='where to write the list of the C and the tables, build/sources.txt')
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
        if any(source.startswith('gen/') for source in config.sources()):
            read = sorted(path for directory in directories(config) for path in directory.rglob('*.[ch]'))
            # in 1ST_READ.BIN, its own symbols.txt is FIRST's: list it once
            tables = [config.splits, config.path / 'symbols.txt', FIRST / 'symbols.txt']
            lines.append(rule(Path('build/gen', name, 'declared.stamp'), [*read, *dict.fromkeys(tables)]))
    write(args.deps, ''.join(lines))
    listed = sorted(path.as_posix() for root in ROOTS for path in Path(root).rglob('*.[ch]'))
    listed += sorted(path.as_posix() for path in CONFIG.glob('*/*.txt'))
    write(args.sources, ''.join(f'{path}\n' for path in listed))


if __name__ == '__main__':
    main(sys.argv[1:])
