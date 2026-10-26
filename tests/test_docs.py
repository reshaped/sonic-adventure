import collections
import os
import posixpath
import re
import subprocess
import unittest
from pathlib import Path

import vendor
from support import cwd, tree

ROOT = Path(__file__).resolve().parents[1]
FENCE = re.compile(r' {0,3}(`{3,}|~{3,})')
HEADING = re.compile(r' {0,3}#{1,6}(?:[ \t]+(.*?))?(?:[ \t]+#+)?[ \t]*')
# what a slug drops of inline markup: a code span's backticks, an image, a link's brackets and destination, an HTML
# tag, an escape's backslash and an underscore that opens or closes emphasis
INLINE = re.compile(r'(`+)(.+?)\1|!\[[^\]]*\]\([^)]*\)|\[([^\]]*)\]\([^)]*\)|</?[A-Za-z][^>]*>|\\(.)'
                    r'|(?<!\w)_+(?=\S)|(?<=\S)_+(?!\w)')
CODE = re.compile(r'(`+).+?\1')
# an inline link's or image's destination, or a reference definition's
LINK = re.compile(r'(?:\]\(\s*|^ {0,3}\[[^\]^][^\]]*\]:\s*)<?([^\s)>]+)')
# a URL, whose path names no file of the tree
URL = re.compile(r'[A-Za-z][\w+.-]*://\S*')
# a path to a Markdown file, # and an anchor
NAMED = re.compile(r'([\w./-]+\.md)#([\w-]+)')
# a link into a GitHub repository at a commit or branch, short of the punctuation that ends a sentence around it
GITHUB = re.compile(r'(https://github\.com/[\w.-]+/[\w.-]+)/(?:blob|tree)/([\w.-]*\w)'
                    r'(?:[/#?][^\s)>"\'`]*[^\s)>"\'`.,;:!?])?')


def read(path):
    # numbered as git and editors number them, where str.splitlines would also break at a form feed; a byte that
    # isn't UTF-8 cites nothing
    return Path(path).read_bytes().decode('utf-8', 'replace').split('\n')


def fenced(lines):
    """The numbers of the lines that fenced code blocks hold, fences included."""
    numbers, fence = set(), None
    for number, line in enumerate(lines, 1):
        opened = FENCE.match(line)
        if fence:
            numbers.add(number)
            if opened and opened[1].startswith(fence) and not line[opened.end():].strip():
                fence = None
        # backticks with another backtick after them open a code span, not a block
        elif opened and not (opened[1][0] == '`' and '`' in line[opened.end():]):
            numbers.add(number)
            fence = opened[1]
    return numbers


def slug(heading):
    """GitHub's slug of a heading's text: its inline markup dropped, then
    lowercase, with every character but a letter, digit, space, hyphen or
    underscore gone, and each space a hyphen."""
    text = INLINE.sub(lambda markup: markup[2] or markup[3] or markup[4] or '', heading)
    return re.sub(r'[^\w\- ]', '', text.lower()).replace(' ', '-')


def anchors(lines):
    """The anchor GitHub gives each # heading in a Markdown file's lines, out
    of code blocks: its slug, or for the nth repeat of a slug, the slug, a
    hyphen and n."""
    code, seen, found = fenced(lines), collections.Counter(), set()
    for number, line in enumerate(lines, 1):
        heading = number not in code and HEADING.fullmatch(line)
        if heading:
            name = slug(heading[1] or '')
            found.add(f'{name}-{seen[name]}' if seen[name] else name)
            seen[name] += 1
    return found


def linked(source, destination):
    """The path from the root and the anchor that a link's destination in
    the file source names, or None for a URL."""
    if re.match(r'[A-Za-z][\w+.-]*:|//', destination):
        return None
    link, _, anchor = destination.partition('#')
    if not link:
        return source, anchor
    # GitHub takes a link that starts with / from the repository's root
    base = '' if link.startswith('/') else posixpath.dirname(source)
    return posixpath.normpath(posixpath.join(base, link.lstrip('/'))), anchor


def unresolved(paths):
    """Each citation that fails to resolve in the files at paths, the files
    git tracks, other than a vendored tool's, as FILE:LINE: CITATION: why.

    A citation is a relative link or image in a Markdown file, from the
    file's directory, or from the root where it starts with /; or, in any
    file's text but a Markdown link or a URL, a path from the root to a
    Markdown file, then # and an anchor. It names a tracked file or
    directory, and its anchor a heading of its Markdown file. A link that
    starts REPOSITORY/blob/COMMIT or REPOSITORY/tree/COMMIT, where
    REPOSITORY is https://github.com/OWNER/NAME exactly as tools/SHA256SUMS
    spells a vendored tool's, names the commit tools/SHA256SUMS pins for
    that tool."""
    _, tools = vendor.read()
    pins = {repository: (name, commit) for name, (repository, commit, _) in tools.items()}
    # a vendored tool's citations name its own repository's files
    vendored = tuple(f'tools/{name}/' for name in tools)
    files = set(paths)
    directories = {str(parent) for path in paths for parent in Path(path).parents}
    headings = {path: anchors(read(path)) for path in paths if path.endswith('.md')}

    def why(path, anchor):
        if path not in files and path not in directories:
            return f'git tracks no {path}'
        if anchor and path not in headings:
            return f'{path} is not Markdown, so no heading has this anchor'
        if anchor and anchor not in headings[path]:
            return f'no heading of {path} has this anchor'
        return None

    found = []
    for path in paths:
        if path.startswith(vendored):
            continue
        lines = read(path)
        markdown = path.endswith('.md')
        # outside Markdown only NAMED, which needs .md#, and GITHUB, which needs ://, can cite
        if not markdown and not any('.md#' in line or '://' in line for line in lines):
            continue
        code = fenced(lines) if markdown else ()
        for number, line in enumerate(lines, 1):
            # the spans URLs and links hold, where a path is no citation in plain text
            cited, taken = [], [url.span() for url in URL.finditer(line)]
            if markdown and number not in code:
                for link in LINK.finditer(CODE.sub(lambda span: ' ' * len(span[0]), line)):
                    taken.append(link.span(1))
                    target = linked(path, link[1])
                    if target:
                        cited.append((link[1], *target))
            cited += [(name[0], posixpath.normpath(name[1]), name[2]) for name in NAMED.finditer(line)
                      if not any(start <= name.start() < end for start, end in taken)]
            for citation, target, anchor in cited:
                problem = why(target, anchor)
                if problem:
                    found.append(f'{path}:{number}: {citation}: {problem}')
            for link in GITHUB.finditer(line):
                pin = pins.get(link[1])
                if pin and link[2] != pin[1]:
                    found.append(f'{path}:{number}: {link[0]}: tools/SHA256SUMS pins {pin[0]} at {pin[1]}')
    return found


def tracked():
    """The files git tracks that the working tree holds."""
    listed = subprocess.run(['git', 'ls-files', '-z'], stdout=subprocess.PIPE, check=True).stdout.decode()
    return [path for path in listed.split('\0') if os.path.isfile(path)]


# the tree test reads this file too, so the fixtures build each anchored .md path and shckit link from these
PIN = '1' * 40
SHCKIT = 'https://github.com/reshaped/shckit'
GUIDE = 'GUIDE.md'
# the guide's anchors are guide, build, tags and tags-1
FILES = {
    'tools/SHA256SUMS': f'# {SHCKIT} {PIN}\n',
    GUIDE: '# Guide\n\n## Build\n\n## Tags\n\n```sh\n# a comment\n```\n\n### Tags\n',
    'config/a/splits.txt': '',
}


def check(files):
    """The citations that fail to resolve in a tree of FILES and files, which git tracks."""
    files = dict(FILES, **files)
    with tree(files):
        return unresolved(list(files))


class Anchors(unittest.TestCase):
    def test_gives_each_heading_the_anchor_github_gives_it(self):
        lines = ['# The `make check` gate', '## C for SHC: R10, *R26* and [R32](#releases) ##',
                 '### `<image>` tables', '#### snake_case, _emphasis_ and \\_escapes\\_', '##### <a id="x"></a>Hashes',
                 '###### Section-boundary tables & fill', '## % C and % assembly']
        self.assertEqual(anchors(lines), {'the-make-check-gate', 'c-for-shc-r10-r26-and-r32', 'image-tables',
                                          'snake_case-emphasis-and-_escapes_', 'hashes',
                                          'section-boundary-tables--fill', '-c-and--assembly'})

    def test_numbers_each_repeat_of_a_slug_in_document_order(self):
        self.assertEqual(anchors(['# Tags', '## Tags', '### Tags!', '#### Tag']), {'tags', 'tags-1', 'tags-2', 'tag'})

    def test_reads_no_heading_in_a_code_block(self):
        lines = ['# Toolchain', '````sh', '```', '# in the block', '````', '~~~', '# in another', '~~~',
                 '```a``` opens no block', '# Hashes']
        self.assertEqual(anchors(lines), {'toolchain', 'hashes'})


class Citations(unittest.TestCase):
    def test_accepts_citations_that_resolve(self):
        files = {
            'README.md': (f'[Tags]({GUIDE}#tags-1), [Build](./{GUIDE}#build), [tables](config/) and [up](#read-me), '
                          f'[rules]({SHCKIT}/blob/{PIN}/docs/inspect.md)\n\n# Read me\n\n[guide]: {GUIDE}#guide\n'),
            'docs/NOTES.md': f'# Notes\n\n[back](../{GUIDE}#build), [tables](/config/a/splits.txt) and [here](#notes)\n'
                             '`[not a link](GONE.md)`\n',
            'Makefile': (f'# see {GUIDE}#tags, as {SHCKIT}/blob/{PIN}/docs/{GUIDE}#nowhere says\n'
                         f'# shckit is vendored from {SHCKIT}/tree/{PIN}.\n'),
            'tools/shckit/LICENSE': f'{GUIDE}#nowhere, in shckit\'s own repository\n',
        }
        self.assertEqual(check(files), [])

    def test_reports_every_citation_that_fails_at_once(self):
        # a code block holds no link, though its text can cite, and a link after a code span is cited once; a form
        # feed starts no line, and a byte that isn't UTF-8 cites nothing
        files = {
            'README.md': (f'[gone](GONE.md), [build]({GUIDE}#biuld) and [top](#nowhere)\n'
                          f'[Tags]({GUIDE}#tags-2), [fill](config/a/splits.txt#fill) and `{GUIDE}#guid`\n'
                          f'```text\n[not a link](GONE.md), see {GUIDE}#nowhere\n```\n\n[old]: OLD.md\n'
                          f'`tools/SHA256SUMS` pins the tools, as [Hashes]({GUIDE}#hashes) says\n'),
            'Makefile': f'# see {GUIDE}#Build and gone/{GUIDE}#build\n',
            'tools/a.py': (f'# {SHCKIT}/tree/main/docs, at its head\n'
                           f'# {SHCKIT}/blob/{PIN[:7]}/docs/inspect.md, at its short hash\n'),
            'src/a.c': b'/* \x82\xa0 */\n\f\n' + f'/* {GUIDE}#seams */\n'.encode(),
        }
        self.assertEqual(check(files), [
            'README.md:1: GONE.md: git tracks no GONE.md',
            f'README.md:1: {GUIDE}#biuld: no heading of {GUIDE} has this anchor',
            'README.md:1: #nowhere: no heading of README.md has this anchor',
            f'README.md:2: {GUIDE}#tags-2: no heading of {GUIDE} has this anchor',
            'README.md:2: config/a/splits.txt#fill: config/a/splits.txt is not Markdown, so no heading has this anchor',
            f'README.md:2: {GUIDE}#guid: no heading of {GUIDE} has this anchor',
            f'README.md:4: {GUIDE}#nowhere: no heading of {GUIDE} has this anchor',
            'README.md:7: OLD.md: git tracks no OLD.md',
            f'README.md:8: {GUIDE}#hashes: no heading of {GUIDE} has this anchor',
            f'Makefile:1: {GUIDE}#Build: no heading of {GUIDE} has this anchor',
            f'Makefile:1: gone/{GUIDE}#build: git tracks no gone/{GUIDE}',
            f'tools/a.py:1: {SHCKIT}/tree/main/docs: tools/SHA256SUMS pins shckit at {PIN}',
            f'tools/a.py:2: {SHCKIT}/blob/{PIN[:7]}/docs/inspect.md: tools/SHA256SUMS pins shckit at {PIN}',
            f'src/a.c:3: {GUIDE}#seams: no heading of {GUIDE} has this anchor',
        ])


class Tree(unittest.TestCase):
    def test_every_citation_in_the_tree_resolves(self):
        if not (ROOT / '.git').exists():
            self.skipTest('not a git checkout, so no list of the files it tracks')
        with cwd(ROOT):
            found = unresolved(tracked())
        if found:
            self.fail('\n'.join(found))


if __name__ == '__main__':
    unittest.main()
