"""GCC's preprocessed C as tools/floats.py's tokens: the tokens
tools/csource.py's parse() reads outside the directives, each with the file
and line its line marker gives it, and how a message quotes a run of them.

GCC's line markers give the file and line each line of C comes from, so a
macro's constant lies where the macro expands. The tree's lines are those
of every file but a system header, which SHC never reads. marked() says
which files are system headers. A system header's lines keep their
constants and arithmetic as GCC reads them. A constant is a pp-number by
C's grammar, outside the line markers, the other directives GCC writes,
strings and characters.
"""

import re
from bisect import bisect_right
from pathlib import Path

from csource import LINE, Tokens, following, parse

# a header in GCC's own include directory, as a line marker names it
GCC_INCLUDE = re.compile(r'.*/lib/gcc/sh-elf/[^/]+/include/')


class Marker:
    """A line marker of GCC's preprocessed C, as the tokens after it take it:
    file, the file it names; offset, the number to add to a line of the C
    for its line in file; and system, whether file is a system header."""

    __slots__ = ('file', 'offset', 'system')

    def __init__(self, file, offset, system):
        self.file, self.offset, self.system = file, offset, system


class Marked(Tokens):
    """parse()'s Tokens of GCC's preprocessed C but its directives, with the
    line marker above each token: markers holds each Marker, and firsts the
    index of the first token after each. Where markers share a first token,
    the last of them applies. system says whether each token lies on a
    system header's line. values holds, by index, the value tools/floats.py
    reads for each floating constant on the tree's lines, whose text it may
    respell."""

    def __init__(self, tokens, firsts, markers):
        super().__init__(tokens)
        self.firsts, self.markers, self.values, self.system = firsts, markers, {}, []
        for marker, first, past in zip(markers, firsts, [*firsts[1:], len(tokens)]):
            self.system += [marker.system] * (past - first)

    def marker(self, at):
        return self.markers[bisect_right(self.firsts, at) - 1]

    def line(self, at):
        return self[at].line + self.marker(at).offset

    def where(self, at):
        return f'{self.marker(at).file}:{self.line(at)}'


def marked(text, path, system=()):
    """The Marked tokens of text, GCC's preprocessed C at path. A marker's
    file is a system header where it lies under a directory system names,
    once both resolve against the working directory, or in GCC's own
    include directory. A tree header that a system header includes is the
    tree's, though GCC's marker flags it as a system header too."""
    roots = [Path(root).resolve() for root in system]
    c, system_header = parse(text), {}
    tokens = c.parsed
    # the C's first line is line 1 of path, and the line after a marker's is
    # the line it names
    firsts, markers, at = [0], [Marker(path, 0, False)], 0
    for line, file, named in c.markers:
        at = following(tokens, line + 1, at, LINE)
        if file not in system_header:
            resolved = Path(file).resolve()
            system_header[file] = bool(GCC_INCLUDE.match(file)) or any(
                root == resolved or root in resolved.parents for root in roots)
        firsts.append(at)
        markers.append(Marker(file, named - line - 1, system_header[file]))
    return Marked(tokens, firsts, markers)


def quoted(toks):
    """toks, a run of tokens, as a message quotes it on one line: each one's
    text, and a space where the C parts two, as a line marker does."""
    return ''.join(token.text + ' ' * (after.start > token.end) for token, after in zip(toks, toks[1:])) + toks[-1].text
