"""Check SHA256SUMS against the paths make expects it to list, and write
the lines sha256sum -c checks.

EXPECTED holds those paths, the Makefile's EXPECTED, separated by
whitespace: the original files config/ implies and the build/ files make
checks against their own lines.

Reject each of the following, naming every one found:

- a line that is not a hash, two spaces and a path;
- a path that sorts before the line above it, as LC_ALL=C sort -k 2
  orders the file;
- a path listed more than once;
- an expected path not listed;
- a path listed but not expected: an original file that is no image
  config/ describes and no event that repeats one, a file make rebuilds
  from an original, or a build/ file make checks against no line of its own;
- an event config.EVENT_ALIASES gives as repeating another, listed with a
  hash other than that event's.

With OUT, write there the lines sha256sum -c checks: each line of
SHA256SUMS, then for each image's original file its hash beside the path of
the file make rebuilds from it under build/, so that the original's one line
holds both files to that hash. Each such original's line is then expected,
whether or not EXPECTED lists it.

TOOLCHAIN.md#hashes gives what the file records, and
CONTRIBUTING.md#what-passing-means the evidence a change to a line carries.
"""

import argparse
import re
import sys
from collections import Counter

from config import EVENT_ALIASES, fail, files, lines, save

# a line as sha256sum writes one: its hash and its path
LINE = re.compile(r'([0-9a-f]{64})  (\S+)')


def main(argv):
    parser = argparse.ArgumentParser(prog='sums', description=__doc__.split('\n\n')[0])
    parser.add_argument('expected', metavar='EXPECTED',
                        help='the file of the paths make expects SHA256SUMS to list, build/expected.txt')
    parser.add_argument('out', metavar='OUT', nargs='?',
                        help='where to write the lines sha256sum -c checks, build/sums.txt')
    args = parser.parse_args(argv)
    expected = {path for _, line in lines(args.expected) for path in line.split()}
    rebuilt = set(files().values())
    if args.out is not None:
        expected |= {f'orig/{file}' for file in rebuilt}
    listed = Counter()
    hashes, problems = {}, []
    above = None  # the path on the line above, where that line has one
    for number, line in lines('SHA256SUMS'):
        match = LINE.fullmatch(line)
        if not match:
            problems.append(f'SHA256SUMS:{number}: not a hash, two spaces and a path')
            above = None
            continue
        digest, path = match.groups()
        # sort -k 2's key is the two spaces and the path, the same spaces on every line, and the C locale compares
        # bytes, which UTF-8 orders as str orders code points
        if above is not None and path < above:
            problems.append(f'SHA256SUMS:{number}: {path} sorts before the line above it; LC_ALL=C sort -k 2 orders it')
        listed[path] += 1
        hashes[path] = digest
        above = path
    problems += [f'{path}: in SHA256SUMS {count} times' for path, count in sorted(listed.items()) if count > 1]
    problems += [f'{path}: not in SHA256SUMS' for path in sorted(expected - set(listed))]
    for alias, event in EVENT_ALIASES.items():
        repeat, original = f'orig/SONICADV/{alias}.PRS', f'orig/SONICADV/{event}.PRS'
        if repeat in hashes and original in hashes and hashes[repeat] != hashes[original]:
            problems.append(f'{repeat}: in SHA256SUMS with a hash other than {original}\'s, which it repeats')
    for path in sorted(set(listed) - expected):
        if path.startswith('orig/'):
            problems.append(f'{path}: in SHA256SUMS, and config/ describes no such file')
        elif path.startswith('build/') and path[len('build/'):] in rebuilt:
            problems.append(f'{path}: in SHA256SUMS, and make checks it against orig/{path[len("build/"):]}\'s line')
        else:
            problems.append(f'{path}: in SHA256SUMS, and make expects no line for it')
    if problems:
        fail(*problems)
    if args.out is not None:
        checked = [*(f'{digest}  {path}' for path, digest in hashes.items()),
                   *(f'{hashes[f"orig/{file}"]}  build/{file}' for file in sorted(rebuilt))]
        save(args.out, ''.join(f'{line}\n' for line in checked).encode())


if __name__ == '__main__':
    main(sys.argv[1:])
