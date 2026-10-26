"""Check the vendored tools' files against tools/SHA256SUMS, or refresh one
tool's from a commit of its repository.

tools/SHA256SUMS lists the downloads, then each vendored tool: a line
# REPOSITORY COMMIT, for a repository named TOOL, then a line for each file
copied from that commit into tools/TOOL/: its SHA-256, two spaces and its
path. Each path has one line. sha256sum -c takes the # lines for comments.

--check requires a header for each of TOOLS and no other; a line for each
file the Makefile compiles or includes from tools/TOOL/, and its LICENSE;
no file in tools/TOOL/ without a line; and each listed file's recorded
hash. make test runs it, and the Makefile builds no tool until it passes.

A refresh takes the commit's 40 hex digits: git fetches the commit from the
tool's repository, and the files --check requires and any others the
tool's lines list are set to the commit's bytes and recorded under it. It
writes only the files that change, tools/SHA256SUMS among them, and prints
each one's path. git names each object by its hash, so the commit's files
are the same from any clone of the repository; url.<base>.insteadOf points
git at another, such as a local one. Only a refresh uses the network.
"""

import argparse
import hashlib
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

from config import content, fail, lines, save
from sums import LINE

SUMS = Path('tools/SHA256SUMS')
# the tools the Makefile builds, each from its library, TOOL.c and TOOL.h,
# and its command, main.c
TOOLS = ('gdi', 'prskit', 'shckit')
HEADER = re.compile(r'# (\S+) ([0-9a-f]{40})')


def read():
    """The download lines of tools/SHA256SUMS, and each vendored tool by name
    as [repository, commit, {path: hash}], in the file's order."""
    downloads, tools, files, first = [], {}, None, {}
    for number, line in lines(SUMS):
        header, entry = HEADER.fullmatch(line), LINE.fullmatch(line)
        if header:
            repository, commit = header.groups()
            name = repository.rsplit('/', 1)[-1]
            if name in tools:
                fail(f'{SUMS}:{number}: a second header for {name}')
            files = {}
            tools[name] = [repository, commit, files]
        elif not entry:
            fail(f'{SUMS}:{number}: not a hash, two spaces and a path, or # REPOSITORY COMMIT')
        elif first.setdefault(entry[2], number) != number:
            fail(f'{SUMS}:{number}: a second line for {entry[2]}; line {first[entry[2]]} is the first')
        elif files is None:
            downloads.append(line)
        elif entry[2].startswith(f'tools/{name}/'):
            files[entry[2]] = entry[1]
        else:
            fail(f'{SUMS}:{number}: {entry[2]} lies outside tools/{name}/')
    return downloads, tools


def write(downloads, tools):
    """Write the downloads' lines and each tool's to tools/SHA256SUMS, unless
    it holds them already, and return whether it wrote."""
    lines = list(downloads)
    for repository, commit, files in tools.values():
        lines += [f'# {repository} {commit}', *(f'{digest}  {path}' for path, digest in files.items())]
    text = ''.join(f'{line}\n' for line in lines)
    if content(SUMS).decode() == text:
        return False
    save(SUMS, text.encode())
    return True


def sha256(path):
    try:
        return hashlib.sha256(Path(path).read_bytes()).hexdigest()
    except FileNotFoundError:
        return None
    except OSError as error:
        fail(f'{path}: {error.strerror}')


def copied(tool):
    """The files tools/SHA256SUMS must list under the tool: those the
    Makefile compiles or includes from tools/TOOL/, and its LICENSE."""
    return [f'tools/{tool}/{name}' for name in ('LICENSE', f'{tool}.c', f'{tool}.h', 'main.c')]


def check(tools):
    problems = [f'{SUMS}: no # REPOSITORY COMMIT line for {name}' for name in TOOLS if name not in tools]
    problems += [f'{SUMS}: {name}: not a tool the Makefile builds' for name in tools if name not in TOOLS]
    for name, (_, commit, files) in tools.items():
        restore = f'python3 tools/vendor.py {name} {commit}'
        problems += [f'{path}: no line in {SUMS}; {restore} records it' for path in copied(name) if path not in files]
        problems += [f'{path}: not the file {SUMS} records; {restore} restores it'
                     for path, digest in files.items() if sha256(path) != digest]
        here = Path('tools', name)
        found = sorted(path.as_posix() for path in here.iterdir()) if here.is_dir() else []
        problems += [f'{path}: in {here}/, and {SUMS} lists no such file' for path in found if path not in files]
    if problems:
        fail(*problems)


def git(clone, *args, env=None):
    # --git-dir, since -C yields to a GIT_DIR in the environment, as in a hook;
    # env drops the hook's other variables for its repository, which --git-dir
    # doesn't override
    try:
        run = subprocess.run(['git', f'--git-dir={clone}', *args], stdout=subprocess.PIPE, env=env)
    except OSError as error:
        fail(f"git: can't run it: {error.strerror}")
    if run.returncode:
        # git has written the reason to stderr
        fail(f'git {" ".join(args)} exited with status {run.returncode}')
    return run.stdout


def fetch(repository, commit, names):
    """The bytes the commit holds at each path in names, in their order. git
    runs without the variables git names as a repository's own, such as
    GIT_COMMON_DIR, so a fetch from a hook leaves the hook's repository as it
    was."""
    with tempfile.TemporaryDirectory() as clone:
        local = git(clone, 'rev-parse', '--local-env-vars').decode().split()
        env = {name: value for name, value in os.environ.items() if name not in local}
        git(clone, 'init', '-q', '--bare', env=env)
        git(clone, 'fetch', '-q', '--depth=1', repository, commit, env=env)
        return [git(clone, 'cat-file', 'blob', f'{commit}:{name}', env=env) for name in names]


def refresh(downloads, tools, name, commit):
    """Set the tool's files and their lines in tools/SHA256SUMS to the
    commit's, and print the path of each file that changes."""
    if name not in tools:
        fail(f'{name}: not a vendored tool; {SUMS} records {", ".join(tools)}')
    if not re.fullmatch(r'[0-9a-f]{40}', commit):
        fail(f'{commit}: not a commit; give its 40 hex digits')
    repository, _, files = tools[name]
    paths = sorted(set(files) | set(copied(name)))
    copies = fetch(repository, commit, [Path(path).relative_to(f'tools/{name}').as_posix() for path in paths])
    digests = {path: hashlib.sha256(copy).hexdigest() for path, copy in zip(paths, copies)}
    try:
        # a tool whose directory is gone takes it back, once every file is fetched
        Path('tools', name).mkdir(exist_ok=True)
    except OSError as error:
        fail(f'tools/{name}: {error.strerror}')
    for path, copy in zip(paths, copies):
        if sha256(path) != digests[path]:
            save(path, copy)
            print(path)
    tools[name][1:] = [commit, digests]
    if write(downloads, tools):
        print(SUMS)


def main(argv):
    parser = argparse.ArgumentParser(prog='vendor', usage='%(prog)s [-h] (--check | TOOL COMMIT)',
                                     description=__doc__.split('\n\n')[0])
    parser.add_argument('--check', action='store_true',
                        help="check every vendored tool's files against tools/SHA256SUMS")
    parser.add_argument('tool', metavar='TOOL', nargs='?', help='the vendored tool to refresh, such as shckit')
    parser.add_argument('commit', metavar='COMMIT', nargs='?', help='the commit to refresh it from, its 40 hex digits')
    args = parser.parse_args(argv)
    if (args.check and args.tool is not None) or (not args.check and args.commit is None):
        parser.error('give --check, or TOOL and COMMIT')
    downloads, tools = read()
    if args.check:
        check(tools)
    else:
        refresh(downloads, tools, args.tool, args.commit)


if __name__ == '__main__':
    main(sys.argv[1:])
