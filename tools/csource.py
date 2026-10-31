"""The tree's one C reader, for tools/census.py, imports.py, tags.py,
inline.py, decls.py, progress.py, data.py, content.py, memory.py,
layout.py, stages.py and floats.py's modules.

parse() reads C as the preprocessor and the compiler's first pass do,
without expanding a macro or deciding a conditional: its tokens, each with
its kind and where it lies in the text as written; its directives; and its
blocks, each pair of braces at file scope, with its head and the function
whose body it holds, if any. A token a line splice parts is one token. A
comment is a token, and no token lies inside one. A name may hold a $, and
a raw string runs to its delimiter, as GCC reads GNU C, so GCC's
preprocessed C reads too, with its line markers. Every group of every
conditional is read, and where its groups disagree on the braces open
after them, the first group to change them decides. An inline body, a
function's below a #pragma inline_asm, and a #pragma asm block hold SHC's
assembly, so no { inside either opens a block, and an inline body closes at
its first }.

Three layers read what the compiler's parser reads, from a C's tokens
outside its directives: Tokens, a run of tokens with the scans of its
brackets; Expressions, which reads an expression into Nodes; and Walk,
which reads a file's declarations and each function's blocks and
statements, and leaves their types to its subclass.

A process reads a text once. parse() keeps each C it reads, and reads a
text that blank() or code() made as the C of the text it was made from,
less the tokens and lines they blanked or emptied, so a file is read once
in whatever form its readers ask for it. A C reads its comments, literals
and directives at once, and its other tokens and its blocks when asked.

The rest reads C through parse(): a file's text with its comments blank,
and for one compiler, the lines of each GCC guard's side it doesn't compile
blank too; its tokens, each with its line; its #define and #undef
directives; the text with each comment one blank; the matches of a scan
that start outside a string or character constant; and the files it
includes, transitively. In its text with its literals blank too, as
blanked() gives it: each guard's two sides, the lines make gcc's GCC
compile takes, each function's braces, and each inline body and #pragma
asm block. Only included() follows an #include.
"""

import atexit
import math
import os
import re
from bisect import bisect_left, bisect_right
from collections import namedtuple
from functools import cached_property, lru_cache
from itertools import accumulate, compress, count
from operator import itemgetter
from pathlib import Path

from layout import BASIC, QUALIFIERS, STORAGE
from tool import content, fail

# the compilers code() reads a file for, each of which compiles one side of
# each guard
SHC, GCC = 'SHC', 'GCC'
# the lexemes of C a scan finds first, in text whose line splices are
# joined: a comment, a string or a character constant, each # or ##, which
# may open a directive, and R", which may open a raw string. Each opens with
# a character of its own, which gives its kind, so the pattern names no
# group and a scan skips to it
LEXEME = re.compile(r'''
    /\*.*?\*/ | //[^\n]*
  | "(?:\\.|[^"\\\n])*"
  | '(?:\\.|[^'\\\n])*'
  | \#\#?
  | R"
''', re.S | re.X)
# each lexeme's kind, by its first character
LEXEMES = {'/': 'comment', '"': 'string', "'": 'character', '#': 'punctuator', 'R': 'string'}
# a string's or a character constant's prefix, at the end of the text
# searched, where no letter, digit, _ or $ before it makes it a name's end.
# A character constant takes no u8, and a raw string's prefix ends in R
PREFIX = re.compile(r'(?<![\w$])(?:u8|[uUL])\Z')
RAW_PREFIX = re.compile(r'(?<![\w$])(?:u8|[uUL])?R\Z')
# a raw string from its ", as GCC reads one in GNU C: its delimiter, of up
# to 16 characters, and its text, which no escape changes, to the first )
# the delimiter and a " follow
RAW = re.compile(r'"([^()\\\s]{0,16})\(.*?\)\1"', re.S)
# what lies between two lexemes, after the blanks before it, which a match
# takes so that a scan tries no blank alone: a pp-number, a name, as C89
# spells one and GCC with $ too, any other punctuator, longest first, and
# any other character but a blank. None can hold a lexeme's start, so the
# two read C as one scan would
WORD = re.compile(r'''\s*(?:
    (?P<number>\.?[0-9](?:[eEpP][+-]|[\w.])*)
  | (?P<name>[A-Za-z_$][A-Za-z0-9_$]*)
  | (?P<punctuator>\.\.\.|<<=|>>=|->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^]=|[][(){}.&*+\-~!/%<>^|?:;=,])
  | (?P<other>\S)
)''', re.X)
# the punctuators tokens() gives whole, and how it gives each other one of
# more than one character: as one of those and the characters after it, or
# as its characters, so += is + and =
WHOLE = {'<<', '>>', '<=', '>=', '==', '!=', '&&', '||', '->'}
PARTS = {spelled: ([spelled[:2], *spelled[2:]] if spelled[:2] in WHOLE else list(spelled))
         for spelled in ('...', '<<=', '>>=', '++', '--', '+=', '-=', '*=', '/=', '%=', '&=', '|=', '^=', '##')}
# C89's keywords, and GNU C's spellings of an attribute and of asm, none of
# which names a function
KEYWORDS = {'auto', 'break', 'case', 'char', 'const', 'continue', 'default', 'do', 'double', 'else', 'enum', 'extern',
            'float', 'for', 'goto', 'if', 'int', 'long', 'register', 'return', 'short', 'signed', 'sizeof', 'static',
            'struct', 'switch', 'typedef', 'union', 'unsigned', 'void', 'volatile', 'while', '__attribute__',
            '__attribute', '__asm__', '__asm', 'asm'}
# the directives that open a conditional, and those that open its groups
# after its first
OPENING = ('if', 'ifdef', 'ifndef')
LATER = ('elifdef', 'elifndef', 'elif', 'else')
# whether make gcc's GCC compile defines each macro compiled() reads an
# #ifdef or #ifndef of: GCC, which its -DGCC defines, and __clang__, which
# only clang defines
DEFINED = {'GCC': True, '__clang__': False}
# a header a file includes, and the directories an include's header is
# found in, after its own file's for a quoted include. They are the tree's
# directories the routes name to -I, in the Makefile's GAME_INC order, then
# SDK_INC's others
INCLUDED = re.compile(r'^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+\.h)[>"]', re.M)
INCLUDE_ROOTS = ('sdk/include', 'src', 'modules', 'events', 'sdk', '.')
# each text blank() and code() have made, oldest first, as (the text it was
# made from, the kinds of token blank in it, and the indexes of its lines
# emptied); and how many texts it holds
MADE, HELD = {}, 1 << 15
# a Token's text, its start and its line
TEXT, START, LINE = itemgetter(0), itemgetter(2), itemgetter(4)
# the brackets that open, and those that close, each of which closes any of
# the others; and each bracket, by whether it opens
OPENERS, CLOSERS = ('(', '[', '{'), (')', ']', '}')
BRACKETS = {**dict.fromkeys(OPENERS, True), **dict.fromkeys(CLOSERS, False)}
# what GNU C takes beside a declaration's specifiers and declarators: an
# attribute or an asm label, each with the parentheses after it, and a
# keyword ahead of the specifiers
ATTRIBUTES = {'__attribute__', '__attribute', '__asm__', '__asm', 'asm'}
GNU = {'__extension__', 'inline', '__inline', '__inline__'}
# the words that open a type name in a cast: C's, which layout.py gives
# but for struct, union, enum and _Bool, and the others GCC takes there
# under -std=gnu17
TYPE_WORDS = {*BASIC, *QUALIFIERS, 'struct', 'union', 'enum', '_Bool', '__signed', '__signed__', '__const',
              '__volatile', '__volatile__'}
GNU_TYPES = {'typeof', '__typeof', '__typeof__', '__typeof_unqual', '__typeof_unqual__', '_Atomic', '_Complex',
             '__complex', '__complex__', '__attribute', '__attribute__', '__const__', 'restrict', '__restrict',
             '__restrict__', '_Float32', '_Float64', '_Float32x'}
# the operators that give an integer constant, whose operand GCC doesn't
# evaluate
SIZES = ('sizeof', '_Alignof', '__alignof__', '__alignof')
# the assignment operators, which conditional() takes as no operator
ASSIGNMENTS = {'=', '*=', '/=', '%=', '+=', '-=', '<<=', '>>=', '&=', '^=', '|='}
# C's binary operators by precedence, the highest first
BINARY = {operator: 10 - rank for rank, operators in enumerate(
    (('*', '/', '%'), ('+', '-'), ('<<', '>>'), ('<', '>', '<=', '>='), ('==', '!='), ('&',), ('^',), ('|',),
     ('&&',), ('||',))) for operator in operators}
# the kinds of token a primary node is, none of which an expression comes
# before
PRIMARY = ('number', 'name', 'string', 'character')
# the words that open a declaration in a block, beside a typedef's name,
# and those that open an asm, which at file scope declares nothing
DECLARES = TYPE_WORDS | STORAGE | GNU | {'__attribute__', '__attribute'}
ASM = ('__asm__', '__asm', 'asm')


class Token(namedtuple('Token', 'text kind start end line')):
    """A token of C: its text, without the splices that part it; its kind,
    one of LEXEME's and WORD's; where it starts and ends in the text as
    written; and the line it starts on, counting from 1."""


class Directive(namedtuple('Directive', 'name words start end line last')):
    """A preprocessing directive: the text of the token after its #, or ''
    for none; the tokens after that one, without comments; where its #
    starts, and where the line end that ends it lies, or the text's length;
    and the lines its # and that end are on, counting from 1."""


class Block(namedtuple('Block', 'name head opening closing')):
    """A pair of braces at file scope: the name of the function whose body
    it holds, or None for a type's members or an initializer; the tokens of
    its head, from the first after the declaration or definition above it,
    without directives; its opening brace; and its closing brace, or None
    where none closes it."""


def parse(text):
    """text read as C, as the module docstring and C give it."""
    made = MADE.get(text)
    return read(text) if made is None else derived(text, *made)


@lru_cache(maxsize=None)
def read(text):
    """The C of text, as no reader made it."""
    return C(text)


# a process's exit collects what it holds over and over, so each C it read goes first, at once
atexit.register(read.cache_clear)


@lru_cache(maxsize=8)
def derived(text, source, kinds, lines):
    """The C of text, which blank() or code() made from source, as Made reads it."""
    return Made(text, read(source), kinds, lines)


def note(text, source, kinds, lines=frozenset()):
    """Hold text, which blank() or code() made from source by blanking each token of kinds and emptying lines, in
    MADE, and return it."""
    if text != source:
        if source in MADE:
            source, before, emptied = MADE[source]
            kinds, lines = tuple(sorted({*before, *kinds})), lines | emptied
        MADE[text] = source, kinds, lines
        if len(MADE) > HELD:
            del MADE[next(iter(MADE))]
    return text


class C:
    """C text as parse() reads it: lexemes, its comments, literals, and #
    and ## punctuators; tokens, all its tokens, comments among them;
    directives, its directives; blocks, its Blocks; parsed, the Tokens of
    its tokens outside its directives, but comments; and markers, its line
    markers, as GCC's preprocessor writes each, # N "FILE" FLAGS..., as (the
    line it lies on, FILE as written, N); each in order.

    A directive opens with a # that is the first token of its line, and
    runs to the line's end. Only a line end outside a comment ends a line,
    so a comment that holds one runs a directive on past it, as GCC reads
    one.

    A function's definition is a { at file scope after a head that ends
    with a ), or in the old form with the declarations of the parameters
    that ) closes the names of, and that holds no = outside parentheses.
    Its name is the head's first name, but a keyword, that a ( follows. Any
    other { at file scope opens a type's members or an initializer. A block
    closes at the } that matches its {, and an inline body, the first block
    below a #pragma inline_asm where it is a function's, at the first }
    inside it."""

    def __init__(self, text):
        pieces = text.split('\\\n')
        self.text, self.joined = text, ''.join(pieces)
        # where each splice was in joined
        self.cuts = list(accumulate(map(len, pieces[:-1])))
        # each lexeme's start and end in joined, and the line of joined it starts on, counting from 1
        self.marks, self.lexemes = marks, lexemes = [], []
        joined, at, line, new, search, searched = self.joined, 0, 1, tuple.__new__, LEXEME.search, 0
        while match := search(joined, searched):
            spelled, (start, end) = match[0], match.span()
            kind = LEXEMES[spelled[0]]
            if spelled == 'R"':
                # a raw string, where the R is its prefix's or the prefix's end; else the " is what it is alone
                prefix = RAW_PREFIX.search(joined, max(0, start - 2), start + 1)
                raw = prefix and RAW.match(joined, start + 1)
                if not raw:
                    searched = start + 1
                    continue
                start, end = prefix.start(), raw.end()
                spelled = joined[start:end]
            # a prefix ends right before its literal
            elif kind in ('string', 'character') and joined[start - 1:start] in ('u', 'U', 'L', '8') and \
                    (prefix := PREFIX.search(joined, max(0, start - 3), start)):
                if kind == 'string' or prefix[0] != 'u8':
                    start, spelled = prefix.start(), joined[prefix.start():end]
            line += joined.count('\n', at, start)
            marks.append((start, end, line))
            lexemes.append(self.token(spelled, kind, start, end, line) if self.cuts else
                           new(Token, (spelled, kind, start, end, line)))
            line += spelled.count('\n')
            at = searched = end
        # each directive, as its #, the tokens after it but comments, where the line end that ends it lies, or the
        # text's length, and that end's line; and as the indexes of its # and of the first lexeme past it, and where
        # in joined that line end lies
        self.opened, self.extents, stop = [], [], -1
        for index, token in enumerate(self.lexemes):
            if token.text == '#' and self.marks[index][0] >= stop and self.first(index):
                stop = self.direct(index)

    def token(self, text, kind, start, end, line):
        """The Token of text, of kind, from start to end in joined, on its line line there."""
        if not self.cuts:
            return tuple.__new__(Token, (text, kind, start, end, line))
        spliced = bisect_right(self.cuts, start)
        return tuple.__new__(Token, (text, kind, start + 2 * spliced, end + 2 * bisect_left(self.cuts, end),
                                     line + spliced))

    def first(self, index):
        """Whether only blanks and comments lie between the line end before the lexeme at index in joined and it."""
        start = self.marks[index][0]
        while True:
            index -= 1
            _, newline, tail = self.joined[self.marks[index][1] if index >= 0 else 0:start].rpartition('\n')
            if tail.strip():
                return False
            if newline or index < 0:
                return True
            if self.lexemes[index].kind != 'comment':
                return False
            start = self.marks[index][0]

    def direct(self, index):
        """Add the directive the # lexeme at index opens to opened, and return where in joined the line end that ends
        it lies."""
        marks, lexemes, (_, at, line) = self.marks, self.lexemes, self.marks[index]
        stop, words, later = self.end(at), [], index + 1
        # the lexemes before the line end, and a comment that holds it runs the directive on
        while later < len(marks) and marks[later][0] < stop:
            start, end, _ = marks[later]
            if at < start:
                words += self.words(at, start, line)
            if lexemes[later].kind != 'comment':
                words.append(lexemes[later])
            if end > stop:
                stop, line = self.end(end), line + self.joined.count('\n', at, end)
            at, later = end, later + 1
        if at < stop:
            words += self.words(at, stop, line)
        spliced = bisect_right(self.cuts, stop) if self.cuts else 0
        self.opened.append((self.lexemes[index], tuple(words), stop + 2 * spliced, line + spliced))
        self.extents.append((index, later, stop))
        return stop

    def end(self, at):
        """Where in joined the first line end at or after at lies, or joined's length."""
        found = self.joined.find('\n', at)
        return len(self.joined) if found < 0 else found

    def words(self, start, end, line):
        """The tokens WORD reads from start to end in joined, which lie on its line line."""
        if self.cuts:
            return [self.token(match[match.lastindex], match.lastgroup, match.start(match.lastindex), match.end(), line)
                    for match in scan(self.joined, start, end)]
        new = tuple.__new__
        return [new(Token, (match[match.lastindex], match.lastgroup, match.start(match.lastindex), match.end(), line))
                for match in scan(self.joined, start, end)]

    @cached_property
    def directives(self):
        return [directive(*each) for each in self.opened]

    @cached_property
    def tokens(self):
        return self.between(0, len(self.joined))

    @cached_property
    def blocks(self):
        return blocks(self.tokens, self.directives)

    @cached_property
    def parsed(self):
        return outside(self.tokens, self.directives, any(token.kind == 'comment' for token in self.lexemes))

    @cached_property
    def markers(self):
        return markers(self.directives)

    @cached_property
    def blank(self):
        """joined with each lexeme blank but for its line ends, so that WORD reads the rest, line for line."""
        found, at = [], 0
        for (start, end, _), token in zip(self.marks, self.lexemes):
            found += [self.joined[at:start], ' ' * (end - start) if '\n' not in token.text else
                      '\n'.join(' ' * len(line) for line in token.text.split('\n'))]
            at = end
        return ''.join([*found, self.joined[at:]])

    @cached_property
    def lines(self):
        """Where each line of joined starts in it, then its length plus one."""
        return list(accumulate(map((1).__add__, map(len, self.joined.split('\n'))), initial=0))

    @cached_property
    def firsts(self):
        """Where each lexeme starts in joined."""
        return [start for start, _, _ in self.marks]

    def between(self, start, end):
        """The tokens that start from start to end in joined, each a line's start or past joined's end, in order."""
        lines = self.lines
        if self.cuts:
            found = [self.token(match[match.lastindex], match.lastgroup, match.start(match.lastindex), match.end(),
                                bisect_right(lines, match.start(match.lastindex)))
                     for match in scan(self.blank, start, end)]
        else:
            # a match takes the blanks before its token, and so the line ends between the two
            new, line = tuple.__new__, bisect_right(lines, start)
            found = [new(Token, (match[match.lastindex], match.lastgroup, match.start(match.lastindex), match.end(),
                                 (line := line + match[0].count('\n'))))
                     for match in scan(self.blank, start, end)]
        lexemes = self.lexemes[bisect_left(self.firsts, start):bisect_left(self.firsts, end)]
        if lexemes:
            found += lexemes
            found.sort(key=START)
        return found

    def onward(self, line):
        """The tokens from the start of line in joined, counting from 0, a line of joined at a time, in order."""
        lines = self.lines
        for at in range(line, len(lines) - 1):
            yield from self.between(lines[at], lines[at + 1])

    def spelled(self, kinds, emptied, directives=True):
        """tokens() of the C, less each token of kinds, each on one of the lines emptied, by their indexes, and
        without directives, each of a directive."""
        blank, skipped = self.blank, set()
        if not directives and self.extents:
            found, at = [], 0
            for index, later, stop in self.extents:
                start = self.marks[index][0]
                # a directive's comment can hold a line end, which the lines past it count
                found += [blank[at:start], re.sub('[^\n]', ' ', blank[start:stop])]
                skipped.update(range(index, later))
                at = stop
            blank = ''.join([*found, blank[at:]])
        # a lexeme of kinds or a directive's is blank in blank, so the gap around it is one
        lines, cuts, found, at = self.lines, self.cuts, [], 0
        for index, ((start, end, _), token) in enumerate([*zip(self.marks, self.lexemes),
                                                          ((len(blank), None, None), None)]):
            if token is not None and (token.kind in kinds or index in skipped):
                continue
            if at < start and cuts:
                found += [(match[match.lastindex], bisect_right(lines, match.start(match.lastindex)) +
                           bisect_right(cuts, match.start(match.lastindex))) for match in scan(blank, at, start)]
            elif at < start:
                # a match takes the blanks before its token, and so the line ends between the two
                line = bisect_right(lines, at)
                found += [(match[match.lastindex], (line := line + match[0].count('\n')))
                          for match in scan(blank, at, start)]
            if token is not None:
                found.append((token.text, token.line))
            at = end
        if emptied:
            found = [each for each in found if each[1] - 1 not in emptied]
        # each punctuator PARTS splits, and the tokens around them
        split, at = [], 0
        for index in compress(count(), map(PARTS.__contains__, map(itemgetter(0), found))):
            spelled, line = found[index]
            split += [*found[at:index], *((word, line) for word in PARTS[spelled])]
            at = index + 1
        return split + found[at:] if split else found


def pairs(texts):
    """The index of the bracket that closes each of texts, the texts of a
    run of tokens, that opens, by its index, or None where none does. A
    bracket closes the one it pairs with, whatever their kinds: the last one
    still open before it."""
    found, opened = [None] * len(texts), []
    for at in compress(count(), map(BRACKETS.__contains__, texts)):
        if BRACKETS[texts[at]]:
            opened.append(at)
        elif opened:
            found[opened.pop()] = at
    return found


class Tokens(list):
    """A run of C's tokens, each with its text and kind, and the scans of
    its brackets that the readers share, which pair them once, as pairs()
    does."""

    @cached_property
    def closes(self):
        """The index of the bracket that closes each one that opens, by its index, or None where none does."""
        return pairs(list(map(TEXT, self)))

    def where(self, at):
        """Where the token at at lies, as a problem names it."""
        return f'line {self[at].line}'

    def closing(self, at):
        """The index of the bracket that closes the one at at: the first
        token from it at which as many brackets are open as before it, at
        itself for no bracket. A ValueError says where none does."""
        text, close = self[at].text, None
        if text in OPENERS:
            close = self.closes[at]
        elif text not in CLOSERS:
            return at
        else:
            depth = 0
            for index in range(at, len(self)):
                depth += (self[index].text in OPENERS) - (self[index].text in CLOSERS)
                if not depth:
                    return index
        if close is None:
            raise ValueError(f'{self.where(at)}: {text} with no bracket closing it')
        return close

    def past(self, at):
        """The index past the attributes and asm labels at at."""
        while at + 1 < len(self) and self[at].text in ATTRIBUTES and self[at + 1].text == '(':
            at = self.closing(at + 1) + 1
        return at

    def members(self, at):
        """Where the members of the struct, union or enum at at open, past
        its attributes and its tag, or None where it has none here."""
        index = at + 1
        while index < len(self) and self[index].kind == 'name':
            if self[index].text in ATTRIBUTES and self[index + 1].text == '(':
                index = self.closing(index + 1)
            index += 1
        return index if index < len(self) and self[index].text == '{' else None

    def ending(self, at):
        """The index past the initializer at at, braced or not: past the }
        that closes a {, else at the , } or ; after the expression, outside
        its brackets. An IndexError says the tokens end first."""
        if self[at].text == '{':
            return self.closing(at) + 1
        closes = self.closes
        while True:
            text = self[at].text
            if text in (',', ';', '}'):
                return at
            if text in OPENERS:
                # to its closing bracket, or where none closes it, to the last token
                at = len(self) - 1 if closes[at] is None else closes[at]
            elif text in CLOSERS:
                # a bracket that closes none opened here, past which the expression runs until as many are open
                depth = -1
                at += 1
                while depth or self[at].text not in (',', ';', '}'):
                    depth += (self[at].text in OPENERS) - (self[at].text in CLOSERS)
                    at += 1
                return at
            at += 1

    def listed(self, start, end):
        """Yield the first index of each expression from start to end, a
        braced list's, and the index past it: past its braces, its commas
        and each designator up to its =."""
        at = start
        while at < end:
            if self[at].text in ('{', '}', ','):
                at += 1
            elif self[at].text in ('.', '['):
                while self[at].text != '=':
                    at = self.closing(at) + 1 if self[at].text == '[' else at + 1
                at += 1
            else:
                stop = min(self.ending(at), end)
                yield at, stop
                at = stop


class Made:
    """The C of text, which blank() or code() made from source, a C: source's
    tokens, less each of kinds and each on one of lines, by their indexes,
    and its directives, less each on one of lines and each word of kinds.
    The positions of those after an emptied line move back by its length.
    The rest of a C it reads from text."""

    def __init__(self, text, source, kinds, lines):
        self.text, self.source, self.kinds, self.emptied = text, source, kinds, lines
        # how far each line of text starts before the line of source it was made from
        self.moved = lines and list(accumulate((len(line) if at in lines else 0
                                                for at, line in enumerate(source.text.split('\n'))), initial=0))

    def kept(self, tokens):
        """tokens less each of kinds and each on one of the lines emptied, moved to their places in text."""
        kinds, emptied, moved, new = self.kinds, self.emptied, self.moved, tuple.__new__
        if not emptied:
            return [token for token in tokens if token.kind not in kinds]
        return [new(Token, (text, kind, start - moved[line - 1], end - moved[line - 1], line))
                for text, kind, start, end, line in tokens if kind not in kinds and line - 1 not in emptied]

    @cached_property
    def lexemes(self):
        return self.kept(self.source.lexemes)

    @cached_property
    def tokens(self):
        return self.kept(self.source.between(0, len(self.source.joined)))

    @cached_property
    def directives(self):
        # a directive's words hold no comment, so one moves or leaves a word out only past an emptied line or for a
        # literal
        literals = {'string', 'character'} & set(self.kinds)
        if not self.emptied and not literals:
            return self.source.directives
        return [each if not (self.moved and self.moved[opening.line - 1]) and not
                (literals and any(word.kind in literals for word in words)) else
                directive(self.kept([opening])[0], tuple(self.kept(words)),
                          end - (self.moved[opening.line - 1] if self.moved else 0), last)
                for (opening, words, end, last), each in zip(self.source.opened, self.source.directives)
                if opening.line - 1 not in self.emptied]

    @cached_property
    def blocks(self):
        return blocks(self.tokens, self.directives)

    @cached_property
    def parsed(self):
        return outside(self.tokens, self.directives, any(token.kind == 'comment' for token in self.lexemes))

    @cached_property
    def markers(self):
        return markers(self.directives)

    def spelled(self, kinds, emptied, directives=True):
        return self.source.spelled({*self.kinds, *kinds}, self.emptied | emptied, directives)

    def __getattr__(self, name):
        return getattr(read(self.text), name)


def directive(opening, words, end, last):
    """The Directive whose # is the token opening, whose tokens after it are words, which ends at end, on line last."""
    return Directive(words[0].text if words else '', words[1:], opening.start, end, opening.line, last)


def outside(tokens, directives, comments):
    """The Tokens of a C's tokens that lie in none of its directives, in order, but comments where comments says
    it holds any."""
    found, at = Tokens(), 0
    for directive in directives:
        start = following(tokens, directive.start, at)
        found += tokens[at:start]
        at = following(tokens, directive.end, start)
    found += tokens[at:]
    return Tokens(token for token in found if token.kind != 'comment') if comments else found


def following(tokens, value, at, key=START):
    """The index of the first of tokens from at, which key orders, whose key is value or past it: by default, the
    first that starts at value or past it."""
    high = len(tokens)
    while at < high:
        middle = (at + high) // 2
        if key(tokens[middle]) < value:
            at = middle + 1
        else:
            high = middle
    return at


def markers(directives):
    """Each line marker among directives, as C gives its markers."""
    return [(each.line, each.words[0].text[1:-1], int(each.name)) for each in directives
            if each.name.isdigit() and each.words and each.words[0].text[0] == '"'
            and all(word.kind == 'number' for word in each.words[1:])]


def blocks(tokens, directives):
    """The Blocks of a C's tokens and directives, as C gives them."""
    # the tokens outside directives but comments, with each directive in its place among them
    items, index = [], 0
    for token in tokens:
        while index < len(directives) and directives[index].end <= token.start:
            items.append(directives[index])
            index += 1
        if token.kind != 'comment' and not (index < len(directives) and directives[index].start <= token.start):
            items.append(token)
    found = []
    # each brace open at the item, innermost last: the index in found of the block it opens, or None
    stack = []
    # each conditional open at the item, innermost last: the stack at its directive, and the stack left by its first
    # group to leave another, or None
    groups = []
    # file scope's tokens since its last }, and the index in it after its last ;
    run, start = [], 0
    # whether a #pragma inline_asm is above the next block, whether a #pragma asm block is open, and the index in
    # found of the inline body open at the item
    inline, assembly, body = False, False, None
    for item in items + directives[index:]:
        if isinstance(item, Directive):
            if item.name in OPENING:
                groups.append([list(stack), None])
            elif item.name in (*LATER, 'endif') and groups:
                if groups[-1][1] is None and stack != groups[-1][0]:
                    groups[-1][1] = stack
                if item.name != 'endif':
                    stack = list(groups[-1][0])
                else:
                    stack = stack if groups[-1][1] is None else groups[-1][1]
                    groups.pop()
            elif item.name == 'pragma':
                inline = inline or site(item) is not None
                assembly = pragma(item) == ['asm'] or assembly and pragma(item) != ['endasm']
        elif assembly:
            continue
        elif body is not None:
            if item.text == '}':
                found[body] = found[body]._replace(closing=item)
                body = None
        elif stack:
            if item.text == '{':
                stack.append(None)
            elif item.text == '}':
                opened = stack.pop()
                if opened is not None:
                    found[opened] = found[opened]._replace(closing=item)
                if not stack:
                    run, start = [], 0
        elif item.text == '{':
            head = definition(run, start) or (start, None)
            found.append(Block(head[1], run[head[0]:], item, None))
            if inline and head[1] is not None:
                body = len(found) - 1
            else:
                stack.append(len(found) - 1)
            run, start, inline = [], 0, False
        elif item.text == '}':
            run, start = [], 0
        else:
            run.append(item)
            start = len(run) if item.text == ';' else start
    return found


def definition(run, start):
    """Where the head of the function a { at file scope opens starts in run, file scope's tokens since its last }
    before the {, and the function's name; or None where the { opens no function. start is the index in run after its
    last ;. A head in prototype form is run[start:]. One in the old form runs back through its parameters'
    declarations, each ending in a ;, to the ) closing its parameters' names, which a name follows."""
    if (name := named(run[start:])) is not None:
        return start, name
    depth = 0
    for at in range(len(run) - 1, -1, -1) if start == len(run) else ():
        if run[at].text == ')' and depth == 0 and at + 1 < len(run) and run[at + 1].kind == 'name':
            opened = next((i + 1 for i in range(at, -1, -1) if run[i].text == ';'), 0)
            name = named(run[opened:at + 1])
            return None if name is None else (opened, name)
        depth += (run[at].text == ')') - (run[at].text == '(')
    return None


def named(head):
    """The name of the function whose declarator head ends, as parse() reads it, or None where head is no function's
    declarator: where it holds an = outside its parentheses, or doesn't end with a )."""
    depth = 0
    for token in head:
        depth += (token.text == '(') - (token.text == ')')
        if depth == 0 and token.text == '=':
            return None
    if not head or head[-1].text != ')':
        return None
    return next((token.text for token, after in zip(head, head[1:])
                 if token.kind == 'name' and token.text not in KEYWORDS and after.text == '('), None)


def pragma(directive):
    """The texts of the tokens after #pragma, where directive is a #pragma, else None."""
    return [word.text for word in directive.words] if directive.name == 'pragma' else None


def site(directive):
    """The function a #pragma inline_asm names, where directive is one in the one spelling SHC reads, with nothing
    between its tokens: inline_asm(<function>), or inline_asm(<function>(size=<n>)), n a decimal number. Else None."""
    words, spelled = directive.words, pragma(directive) or []
    if (spelled[:2] != ['inline_asm', '('] or len(words) not in (4, 9) or words[2].kind != 'name' or spelled[-1] != ')'
            or any(before.end != after.start for before, after in zip(words, words[1:]))):
        return None
    if len(words) == 9 and (spelled[3:6] != ['(', 'size', '='] or not spelled[6].isdigit() or spelled[7] != ')'):
        return None
    return spelled[2]


def blank(text, kinds):
    """text with each comment, string or character constant whose kind is in kinds blank: each character of it but a
    line end or the backslash of a line splice a space."""
    found, at = [], 0
    for token in parse(text).lexemes:
        if token.kind in kinds:
            found += [text[at:token.start], blanked_spelling(text[token.start:token.end])]
            at = token.end
    return note(''.join([*found, text[at:]]), text, tuple(kinds))


def blanked_spelling(spelled):
    """spelled with each character but a line end or the backslash of a line splice a space."""
    if '\n' not in spelled:
        return ' ' * len(spelled)
    lines = spelled.split('\n')
    return '\n'.join([*(' ' * (len(line) - 1) + '\\' if line.endswith('\\') else ' ' * len(line)
                         for line in lines[:-1]), ' ' * len(lines[-1])])


def scan(text, start, end):
    """WORD's matches from start to end in text, which stop at the last character there that isn't blank, where no
    blank WORD's search passes over runs it to the end."""
    return WORD.finditer(text, start, start + len(text[start:end].rstrip()))


def code(path, compiler=None):
    """The C text of the file at path, with each comment blank but for its
    line ends and line splices. Each token keeps its line and column, and
    each string and character constant stays as written.

    A GCC guard is a conditional on GCC, #ifdef GCC or #ifndef GCC. Its GCC
    side is the groups GCC can compile, and its shipped side those SHC can.
    Without a compiler, both sides stay. For compiler SHC, each line on a
    GCC side is empty, and for GCC each line on a shipped side. Every
    conditional directive keeps its lines. No other conditional is
    resolved: its groups stay unless they lie on an emptied side. A
    compiler other than SHC, GCC or None raises KeyError."""
    text = content(path).decode(errors='replace')
    kept = blank(text, ('comment',))
    if compiler is None:
        return kept
    lines = kept.split('\n')
    emptied = {SHC: gcc_side, GCC: shipped_side}[compiler](lines)
    for i in emptied:
        lines[i] = ''
    return note('\n'.join(lines), text, ('comment',), frozenset(emptied))


def blanked(text):
    """C text with each comment and literal blank, each of its characters
    but a line end or the backslash of a line splice a space, so that each
    line holds its code alone, where it stood, and a macro's lines still
    join."""
    return blank(text, ('comment', 'string', 'character'))


def uncommented(text, lines=True):
    """C text with each comment one blank, its strings and characters kept.
    With lines, the blank keeps the comment's line ends, so each line keeps
    its number. Without, a directive runs on past a comment, as C reads
    it."""
    found, at = [], 0
    for token in parse(text).lexemes:
        if token.kind == 'comment':
            found += [text[at:token.start], ' ' + '\n' * text.count('\n', token.start, token.end) if lines else ' ']
            at = token.end
    return ''.join([*found, text[at:]])


def tokens(text, directives=True):
    """Each C token of text, but for a comment, as (token, line), in order;
    without directives, but for each token of a directive too. A
    punctuator WHOLE doesn't hold is read as PARTS gives it, so += is + and
    =. line is the line of the token's first character, counting from 1."""
    return parse(text).spelled(('comment',), frozenset(), directives)


def macro_directives(text):
    """Each #define and #undef in text in order, as (line, name, macro).
    line is the directive's first line, counting from 1. macro is None for
    an #undef, and (parameters, body) for a #define. parameters is None for
    an object-like macro. For a function-like macro it is a tuple of the
    parameters' names, with __VA_ARGS__ for an unnamed ellipsis. body is a
    list of the body's tokens, as tokens() reads them."""
    found = []
    for directive in parse(text).directives:
        if directive.name not in ('define', 'undef') or not directive.words or directive.words[0].kind != 'name':
            continue
        name, rest = directive.words[0], directive.words[1:]
        if directive.name == 'undef':
            found.append((directive.line, name.text, None))
            continue
        parameters = None
        closing = next((at for at, word in enumerate(rest) if word.text == ')'), None)
        if rest and rest[0].text == '(' and rest[0].start == name.end and closing is not None:
            listed = [[]]
            for word in rest[1:closing]:
                if word.text == ',':
                    listed.append([])
                else:
                    listed[-1].append(word)
            parameters = tuple(each[0].text if each[0].kind == 'name' else '__VA_ARGS__' for each in listed if each)
            rest = rest[closing + 1:]
        found.append((directive.line, name.text, (parameters, [word for token in rest
                                                               for word in PARTS.get(token.text, (token.text,))])))
    return found


def matches(pattern, text):
    """Each match of pattern, a scan from a line's start, in text, C as
    uncommented() gives it, but for one that starts inside a string or
    character constant: a line can, where a splice carries a string onto
    it."""
    literals = [(token.start, token.end) for token in parse(text).lexemes if token.kind in ('string', 'character')]
    starts = [start for start, _ in literals]
    for match in pattern.finditer(text):
        index = bisect_right(starts, match.start()) - 1
        if index < 0 or literals[index][1] <= match.start():
            yield match


def included(path, texts, include=INCLUDED):
    """Add to texts the text of the file at path and of each file it
    includes, transitively, where include matches an #include directive's
    line, as INCLUDED, the default, matches a header's.

    A quoted include is found in its includer's directory and then, as an
    angle include is, in the first of INCLUDE_ROOTS that holds it, as the
    compiler finds it with those for its -I directories. One found in none
    is left out, as is an #include of a macro, which include doesn't match.
    Conditionals aren't evaluated, so every #include counts.

    texts holds each file once, by its path from the working directory with
    links and .. resolved, so that two spellings of a path read one file.
    A file already in texts isn't read again."""
    key = Path(os.path.relpath(path.resolve()))
    if key in texts:
        return
    texts[key] = content(path).decode(errors='replace')
    text = blank(texts[key], ('comment',))
    for each in parse(texts[key]).directives:
        match = include.match(text, text.rfind('\n', 0, each.start) + 1) if each.name == 'include' else None
        if match is None:
            continue
        # the character before the name opens the include, quoted or angle
        quoted = text[match.start(1) - 1] == '"'
        searched = ([path.parent] if quoted else []) + [Path(root) for root in INCLUDE_ROOTS]
        found = next((directory / match.group(1) for directory in searched
                      if (directory / match.group(1)).is_file()), None)
        if found is not None:
            included(found, texts, include)


def spans(lines):
    """Each function of blanked() C's lines, as parse() finds it: its name, and the indexes of the lines of its
    braces, len(lines) for its closing brace where none closes it."""
    for block in parse('\n'.join(lines)).blocks:
        if block.name is not None:
            yield block.name, block.opening.line - 1, closed(block, lines)


def closed(block, lines):
    """The index of the line of block's closing brace among lines, or len(lines) where none closes it."""
    return len(lines) if block.closing is None else block.closing.line - 1


def inline_sites(lines):
    """Each #pragma inline_asm in blanked() C's lines in the one spelling site() reads, as {the index of its line:
    the function it names}."""
    return {each.line - 1: name for each in parse('\n'.join(lines)).directives if (name := site(each))}


def bodies(path, lines, problems=None):
    """Each inline body in blanked() C's lines, as (the index of its #pragma inline_asm, the function the #pragma
    names, whether the head below it is static, the index of its {, the index of its } or len(lines) where none
    closes it). Its braces are the first Block's below the #pragma, which spans() names where it is a function's. A
    #pragma inline_asm with no block below it opens no body, and is a problem naming path and its line. Each such
    problem goes on problems, or where problems is None, the run fails, naming each."""
    found, missing, sites = [], [], inline_sites(lines)
    blocks = parse('\n'.join(lines)).blocks if sites else []
    for at, name in sites.items():
        block = next((each for each in blocks if each.opening.line - 1 > at), None)
        if block is None:
            missing.append(f'{path}:{at + 1}: #pragma inline_asm({name}), with no function after it')
            continue
        found.append((at, name, any(token.text == 'static' for token in block.head), block.opening.line - 1,
                      closed(block, lines)))
    if problems is not None:
        problems += missing
    elif missing:
        fail(*missing)
    return found


def conditionals(lines):
    """Each of blanked() C's lines that a conditional directive is on, by its index: on the directive's first line,
    its name and the name it tests, or ''; on each later one, None."""
    found = {}
    for each in parse('\n'.join(lines)).directives:
        if each.name in (*OPENING, *LATER, 'endif'):
            found.update(dict.fromkeys(range(each.line, each.last)))
            found[each.line - 1] = each.name, each.words[0].text if each.words and each.words[0].kind == 'name' else ''
    return found


def side(lines, directive):
    """gcc_side() where directive is 'ifdef', and shipped_side() where it is 'ifndef'."""
    other = {'ifdef': 'ifndef', 'ifndef': 'ifdef'}[directive]
    found, read = set(), conditionals(lines)
    # the conditionals open past the last directive's line, innermost last. Each is (the directive that opens it where
    # it tests GCC, else None, and whether the group past that line is on the side); and the index of the line past it
    opened, at = [], 0
    for i in [*sorted(read), len(lines)]:
        if any(inside for _, inside in opened):
            found.update(range(at, i))
        name, tested = read.get(i) or (None, None)
        at = i + 1
        if name in OPENING:
            test = name if name != 'if' and tested == 'GCC' else None
            opened.append((test, test == directive))
        elif name in LATER and opened:
            opened[-1] = opened[-1][0], opened[-1][0] == other
        elif name == 'endif' and opened:
            opened.pop()
    return found


def gcc_side(lines):
    """The indexes of blanked() C's lines on the GCC side of a guard, which GCC compiles and SHC skips: those in an
    #ifdef GCC's first group, and in an #ifndef GCC's later groups, however deep."""
    return side(lines, 'ifdef')


def shipped_side(lines):
    """The indexes of blanked() C's lines on the shipped side of a GCC guard, which SHC compiles and GCC skips: those
    in an #ifdef GCC's later groups, and in an #ifndef GCC's first group, however deep."""
    return side(lines, 'ifndef')


def compiled(lines):
    """Whether make gcc's GCC compile takes each of blanked() C's lines, in order: True, False, or None where a
    conditional it can't read decides. It reads #ifdef and #ifndef of a macro DEFINED gives, and #else; and an #elif,
    #elifdef or #elifndef below a group the compile takes, which leaves its own group out. It reads no #if, and no
    other #ifdef, #ifndef, #elif, #elifdef or #elifndef."""
    found, read = [], conditionals(lines)
    # for each conditional open at the last directive's line, innermost last: whether the compile takes its group
    # there, and whether it takes a group above that one, each None where the tool can't tell; and whether it takes
    # the line
    opened, taken = [], True
    for i in sorted(read):
        found += [taken] * (i - len(found))
        name, tested = read[i] or (None, None)
        if name in OPENING:
            test = None if name == 'if' else DEFINED.get(tested)
            if test is not None and name == 'ifndef':
                test = not test
            opened.append((test, test))
        elif name in LATER and opened:
            test = True if name == 'else' else None
            above = opened[-1][1]
            opened[-1] = False if above else test if above is False else None, True if above or test else None
        elif name == 'endif' and opened:
            opened.pop()
        groups = {group for group, _ in opened}
        taken = False if False in groups else None if None in groups else True
        found.append(taken)
    return found + [taken] * (len(lines) - len(found))


def regions(lines):
    """Each #pragma asm block in blanked() C's lines, as (the index of its #pragma asm, that of the first #pragma endasm
    below it or len(lines) where none is, and whether it holds more than assembler comments)."""
    found, directives = [], parse('\n'.join(lines)).directives
    for at, each in enumerate(directives):
        if pragma(each) == ['asm']:
            end = next((later.line - 1 for later in directives[at + 1:] if pragma(later) == ['endasm']), len(lines))
            found.append((each.line - 1, end, any(text.strip() and not text.strip().startswith(';')
                                                  for text in lines[each.line:end])))
    return found


class Node:
    """A node of an expression: its kind, its operator, or else its text; the
    indexes of its first token and of the one past its last; its operands;
    and for a cast or a type name, what its reader's term() gives of the
    type name."""

    __slots__ = ('kind', 'what', 'start', 'end', 'operands', 'term')

    def __init__(self, kind, what, start, end, operands=(), term=None):
        self.kind, self.what, self.start, self.end, self.operands, self.term = kind, what, start, end, operands, term


class Expressions:
    """The tokens of a Tokens from start to end, read into Nodes by C's
    grammar, each Node indexing the tokens. conditional() reads an
    expression to the first assignment or comma outside its brackets, as an
    initializer or a call's argument is, full() a whole one, its
    assignments and commas among it, and read() one to end, which an opaque
    node holds with what lies past it where anything does.

    A primary node is a number, a name, a string or a character constant; a
    cast, a unary or binary operator, a conditional, an assignment, a comma
    and parentheses take their operands; and the rest, an opaque node,
    takes each expression it reads, but a sizeof's or an _Alignof's, and
    where it reads no further, the numbers and type names after them, as
    atoms() gives them. It takes whole the brackets of a call, a subscript
    and a compound literal, and parentheses that hold no one expression. It
    takes parentheses as a cast's where their first word opens a type name,
    one of TYPE_WORDS or GNU_TYPES, or names a typedef where it stands, or
    where a name, a constant, a string or a { follows them.

    A subclass says what a name and a type name are: typedef_at(name, at)
    whether name names a typedef at the token at; typename(text, at)
    whether that word is a type name an opaque node takes; and term(words)
    what a cast's or a type name's node holds of the type name whose texts
    words are."""

    # what an opaque node says of C past the expression read() reads, and of
    # a postfix operator but a call's and a subscript's
    unread, untaken = 'C past the expression', 'an operator the reader doesn\'t take'

    def __init__(self, tokens, start=0, end=None):
        self.tokens, self.at, self.end = tokens, start, len(tokens) if end is None else end

    def within(self, start, end):
        """A reader of the same kind, of the tokens from start to end."""
        reader = object.__new__(type(self))
        reader.__dict__.update(self.__dict__)
        reader.at, reader.end = start, end
        return reader

    def text(self):
        return self.tokens[self.at].text if self.at < self.end else None

    def read(self):
        start, node = self.at, self.conditional()
        if self.at < self.end:
            return Node('opaque', self.unread, start, self.end, [node] + self.atoms(self.at, self.end))
        return node

    def full(self):
        """A full expression, with its assignments and commas."""
        start, node = self.at, self.assignment()
        while self.text() == ',':
            self.at += 1
            right = self.assignment()
            node = Node('comma', ',', start, self.at, [node, right])
        return node

    def assignment(self):
        start, node = self.at, self.conditional()
        if self.text() in ASSIGNMENTS:
            operator = self.text()
            self.at += 1
            right = self.assignment()
            node = Node('assign', operator, start, self.at, [node, right])
        return node

    def atoms(self, start, end):
        """The numbers among the tokens from start to end, and the type names
        typename() gives, as Nodes, which an opaque node takes as its
        operands."""
        found = []
        for index, token in enumerate(self.tokens[start:end], start):
            if token.kind == 'number':
                found.append(Node('number', token.text, index, index + 1))
            elif self.typename(token.text, index):
                found.append(Node('type', token.text, index, index + 1, term=self.term([token.text])))
        return found

    def conditional(self):
        start, node = self.at, self.binary(1)
        if self.text() != '?':
            return node
        self.at += 1
        then = self.conditional()
        if self.text() != ':':
            return Node('opaque', '?', start, self.at, [node, then])
        self.at += 1
        otherwise = self.conditional()
        return Node('conditional', '?', start, self.at, [node, then, otherwise])

    def binary(self, level):
        start, node = self.at, self.unary()
        while self.text() in BINARY and BINARY[self.text()] >= level:
            operator = self.text()
            self.at += 1
            right = self.binary(BINARY[operator] + 1)
            node = Node('binary', operator, start, self.at, [node, right])
        return node

    def cast(self):
        """What term() gives of the type name in the parentheses at the
        token, and the index of their closing one; or None where they hold
        an expression."""
        tokens = self.tokens
        first = tokens[self.at + 1] if self.at + 1 < self.end else None
        if first is None or first.kind != 'name':
            return None
        close = tokens.closing(self.at)
        after = tokens[close + 1] if close + 1 < self.end else None
        if not (first.text in TYPE_WORDS or first.text in GNU_TYPES or self.typedef_at(first.text, self.at + 1) or
                after is not None and (after.kind in PRIMARY or after.text == '{')):
            return None
        return self.term([token.text for token in tokens[self.at + 1:close]]), close

    def unary(self):
        start, text = self.at, self.text()
        cast = self.cast() if text == '(' else None
        if cast:
            type, close = cast
            self.at = close + 1
            if self.text() == '{':
                end = self.tokens.closing(self.at)
                operands = self.atoms(self.at, end)
                self.at = end + 1
                return Node('opaque', 'a compound literal', start, self.at, operands)
            operand = self.unary()
            return Node('cast', ' '.join(token.text for token in self.tokens[start + 1:close]), start, self.at,
                        [operand], type)
        if text in SIZES:
            self.at += 1
            if self.text() == '(':
                self.at = self.tokens.closing(self.at) + 1
            else:
                self.unary()
            return Node('opaque', text, start, self.at)
        if text in ('-', '+', '!', '~', '&', '*', '++', '--'):
            self.at += 1
            operand = self.unary()
            return Node('unary', text, start, self.at, [operand])
        return self.postfix(self.primary())

    def primary(self):
        start = self.at
        if start >= self.end:
            return Node('opaque', 'nothing', start, start)
        token = self.tokens[start]
        self.at += 1
        if token.text == '(':
            inner = self.conditional()
            if self.text() == ')':
                self.at += 1
                return Node('parentheses', '(', start, self.at, [inner])
            end = self.tokens.closing(start)
            operands = [inner] + self.atoms(self.at, end)
            self.at = end + 1
            return Node('opaque', '(', start, self.at, operands)
        if token.kind in PRIMARY:
            return Node(token.kind, token.text, start, self.at)
        return Node('opaque', token.text, start, self.at)

    def postfix(self, node):
        while self.text() in ('(', '[', '.', '->', '++', '--'):
            text, start = self.text(), self.at
            if text in ('(', '['):
                self.at = self.tokens.closing(start) + 1
            else:
                self.at += 1 if text in ('++', '--') else 2
            inner = self.within(start + 1, self.at - 1) if text in ('(', '[') else None
            operands = [node]
            while inner and inner.at < inner.end:
                operands.append(inner.conditional())
                inner.at += 1
            what = {'(': 'a call', '[': 'a subscript'}.get(text, f'{text}, {self.untaken}')
            node = Node('opaque', what, node.start, self.at, operands)
        return node


class Unread(Exception):
    """A declaration a Walk reads no further, and why."""


class Walk:
    """The declarations of tokens, a Tokens of a file's C, in one walk that
    reads each function's blocks and statements too:

    - unread: each span where a declaration the walk can't read can hide a
      name declared outside it, and why;
    - bodies and declarators: the spans of each function's body and of each
      declarator, which no statement holds;
    - expressions: the first index and the one past the last of each full
      expression of a function's statements: an expression statement's, a
      return's and each of the heads of an if, a for, a while and a switch,
      an automatic object's initializer, or each in its braces, and an asm
      statement's operands.

    A name's scope opens past its declarator, an enumerator's past it and
    its value, so that in enum { A = A + 1 } the value names an outer A; and
    it closes where the block, the function's body or the file that holds
    the declaration does: a struct's or a union's braces open none, and a
    for's declaration's runs past the for to its block's end. A name binds
    once what follows its declarator shows the walk read it. The walk reads
    an enum's members in a declaration's specifiers alone: one in a cast, a
    sizeof or a parameter's type binds no enumerator.

    A subclass reads the types. specifiers(at) gives the type the
    specifiers at at name and the index past them, and declarator(at, base)
    the name a declarator of base declares, or None, its type, whether that
    is a function's, and the index past it; each raises Unread where it
    can't read them. typedef_at(name, at) says whether name names a typedef
    at at. The walk hands it each name it binds, in scope from start to
    end: declared(name, kind, type, span, start, end) a declarator's of
    span, of kind 'object', an object or a function, or 'typedef', and
    enumerated(name, start, end) an enumerator; and static(start, end,
    type, why) each initializer of an object of static storage, at file
    scope or in a block whose declaration says static, of type, or None
    where the walk can't read the declaration, and then why."""

    def __init__(self, tokens):
        self.tokens, self.unread, self.bodies, self.declarators, self.expressions = tokens, [], [], [], []
        at = 0
        while at < len(tokens):
            if tokens[at].text in (';', *ASM):
                at = self.semicolon(at, len(tokens)) + 1
            else:
                at = self.declaration(at, len(tokens), True)
        self.bodies.sort()
        self.declarators.sort()

    def code(self, at):
        """Whether the token at at lies in a function's body outside its
        declarators: in its statements or an initializer, a static's
        too."""
        return inside(self.bodies, at) and not inside(self.declarators, at)

    def declares(self, at):
        """Whether a declaration opens at at, where a statement opens in a
        block: at a word that opens one, or a name of a typedef there, and no
        label."""
        text = self.tokens[at].text
        return text in DECLARES or self.tokens[at + 1].text != ':' and self.typedef_at(text, at)

    def block(self, start, end):
        """Read the declarations and the statements from start to end, a
        block, each declaration in scope to end."""
        at = start
        while at < end:
            at = self.statement(at, end)

    def statement(self, at, end):
        """Read the declaration or the statement at at, in a block that ends
        at end, and return the index past it: past a label, an else or a do,
        or the head of an if, a for, a while or a switch, where the statement
        that follows is the block's next."""
        tokens, text = self.tokens, self.tokens[at].text
        if self.declares(at):
            return self.declaration(at, end, False)
        if text == '{':
            close = tokens.closing(at)
            self.block(at + 1, close)
            return close + 1
        if text in ('if', 'for', 'while', 'switch'):
            part, close = at + 2, tokens.closing(at + 1)
            if text == 'for' and self.declares(part):
                part = self.declaration(part, end, False)
            while part < close:
                stop = self.semicolon(part, close)
                self.expression(part, stop)
                part = stop + 1
            return close + 1
        if text in ('else', 'do'):
            return at + 1
        if text == 'case':
            # past its :, each ? before which takes one of its own
            pending = 0
            while tokens[at].text != ':' or pending:
                pending += (tokens[at].text == '?') - (tokens[at].text == ':')
                at += 1
            return at + 1
        if text == 'default' or tokens[at].kind == 'name' and tokens[at + 1].text == ':':
            return at + 2
        if text in ASM:
            # past volatile, goto and inline; then each operand, its
            # constraint and its expression in parentheses
            opening = at + 1
            while tokens[opening].text != '(':
                opening += 1
            close = tokens.closing(opening)
            for index in range(opening + 1, close):
                if tokens[index].kind == 'string' and tokens[index + 1].text == '(':
                    self.expression(index + 2, tokens.closing(index + 1))
            return close + 1
        stop = self.semicolon(at, end)
        if text not in ('goto', 'break', 'continue'):
            self.expression(at + (text == 'return'), stop)
        return stop + 1

    def semicolon(self, at, end):
        """The index of the ; that ends the statement at at, outside its
        brackets, or end where none does before it."""
        depth, tokens = 0, self.tokens
        while at < end and (depth or tokens[at].text != ';'):
            depth += (tokens[at].text in OPENERS) - (tokens[at].text in CLOSERS)
            at += 1
        return at

    def expression(self, start, end):
        """Take the tokens from start to end as a full expression of a
        function's code, and read the block of each statement expression in
        it."""
        if start < end:
            self.expressions.append((start, end))
        at, tokens = start, self.tokens
        while at < end:
            if tokens[at].text == '(' and tokens[at + 1].text == '{':
                close = tokens.closing(at + 1)
                self.block(at + 2, close)
                at = close
            at += 1

    def initializer(self, start, end, static, type, why):
        """Take the tokens from start to end, an initializer of an object of
        type, to static() where static is true, or else its expressions as
        expressions."""
        if static:
            self.static(start, end, type, why)
        elif self.tokens[start].text == '{':
            for at, stop in self.tokens.listed(start, end):
                self.expression(at, stop)
        else:
            self.expression(start, end)

    def declaration(self, at, end, outer):
        """Read the declaration at at, in scope to end, at file scope where
        outer is true, and return the index past it: past its semicolon, or
        past the body of the function it defines."""
        tokens, start = self.tokens, at
        # past GNU's keywords and attributes, and the storage classes among
        # them, ahead of the type
        while tokens[at].text in GNU or tokens[at].text in STORAGE or tokens.past(at) > at:
            at = tokens.past(at) if tokens[at].text in ATTRIBUTES else at + 1
        if tokens[at].text == ';':
            # an attribute of a statement, as fallthrough is
            return at + 1
        # whether the declaration's objects are of static storage, and where
        # the rest the walk can't read starts
        static, resume = outer or any(token.text == 'static' for token in tokens[start:at]), start
        try:
            base, at = self.specifiers(at)
            kind = 'typedef' if any(token.text == 'typedef' for token in tokens[start:at]) else 'object'
            static = static or any(token.text == 'static' for token in tokens[start:at])
            self.enumerators(start, at, end)
            while True:
                first = at
                name, type, function, at = self.declarator(at, base)
                span = first, at
                at = tokens.past(at)
                self.declarators.append((first, at))
                # a name binds once what follows its declarator shows the walk
                # read it: an initializer, the next declarator, the end of the
                # declaration, or a function's body
                defines = outer and kind == 'object' and name is not None and function and \
                    (tokens[at].text == '{' or self.declares(at))
                if not defines and tokens[at].text not in ('=', ',', ';'):
                    raise Unread(f'{tokens[at].text} after a declarator')
                if name is not None:
                    self.declared(name, kind, type, span, at, end)
                if defines:
                    return self.function(name, first, at)
                if tokens[at].text == '=':
                    stop = tokens.ending(at + 1)
                    self.initializer(at + 1, stop, static, type, None)
                    at = resume = stop
                if tokens[at].text == ';':
                    return at + 1
                if tokens[at].text != ',':
                    raise Unread(f'{tokens[at].text} after an initializer')
                at += 1
        except (Unread, IndexError) as error:
            why = str(error)
        # a declaration at file scope declares each of its names once more,
        # with a type the same as each other declaration's; one in a block can
        # hide one outside it. Each initializer in the rest of it takes no type
        if not outer:
            self.unread.append((start, end, why))
        at, depth = resume, 0
        while at < len(tokens):
            text = tokens[at].text
            if text in ('struct', 'union', 'enum') and tokens.members(at) is not None:
                at = tokens.closing(tokens.members(at))
            elif text == '=' and not depth:
                stop = tokens.ending(at + 1)
                self.initializer(at + 1, stop, static, None, why)
                at = stop - 1
            elif text in ('(', '['):
                depth += 1
            elif text in (')', ']'):
                depth -= 1
            elif not depth and text == ';':
                return at + 1
            elif not depth and text == '{':
                close = tokens.closing(at)
                if outer:
                    self.unread.append((at, close, why))
                    self.bodies.append((at, close))
                self.block(at + 1, close)
                return close + 1
            at += 1
        return at

    def enumerators(self, start, stop, end):
        """Bind each enumerator of each enum whose members the tokens from
        start to stop, a declaration's specifiers, hold, in scope from past
        it and its value to end."""
        tokens = self.tokens
        for at in range(start, stop):
            opening = tokens.members(at) if tokens[at].text == 'enum' else None
            if opening is None:
                continue
            index, close = opening + 1, tokens.closing(opening)
            while index < close:
                after = tokens.ending(index)
                self.enumerated(tokens[index].text, after, end)
                index = after + 1

    def function(self, name, first, at):
        """Read the definition of the function name, whose declarator opens at
        first, from at, past the declarator: its parameters and any
        declarations of them before its body, each in scope to the body's
        end, and its body. Return the index past the body."""
        tokens, body = self.tokens, at
        while tokens[body].text != '{':
            body += 1
        close = tokens.closing(body)
        named = next(index for index in range(first, at) if tokens[index].text == name)
        opening = next(index for index in range(named + 1, at) if tokens[index].text == '(')
        index, stop = opening + 1, tokens.closing(opening)
        while index < stop and tokens[index].text != '...':
            try:
                base, index = self.specifiers(index)
                first = index
                parameter, type, _, index = self.declarator(index, base)
                if parameter is not None:
                    self.declared(parameter, 'object', type, (first, index), body, close)
            except (Unread, IndexError) as error:
                self.unread.append((body, close, f'a parameter of {name}: {error}'))
                break
            index = tokens.past(index) + 1
        # an old-style definition declares its parameters before its body
        while at < body:
            at = self.declaration(at, close, False)
        self.bodies.append((body, close))
        self.block(body + 1, close)
        return close + 1


def inside(spans, at):
    """Whether at lies in one of spans, sorted pairs of a first index and the
    one past the last, none of which overlap."""
    index = bisect_right(spans, (at, math.inf)) - 1
    return index >= 0 and at < spans[index][1]
