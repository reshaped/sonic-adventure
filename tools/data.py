"""Write the C that SHC compiles to the data the build takes from the disc.

    python3 tools/data.py IMAGE SOURCE OUT
    python3 tools/data.py --declared CONFIG OUT
    python3 tools/data.py --rows IMAGE
    python3 tools/data.py --bodies IMAGE SOURCE LISTING OUT CONFIG
    python3 tools/data.py --texts SOURCE OUT
    python3 tools/data.py --values SOURCE OUT

IMAGE is an image's file, uncompressed. CONFIG names config/CONFIG/. A
command with IMAGE and no CONFIG takes IMAGE's file name, lowercased and less
its suffix, as CONFIG.

With no option, the tool writes OUT, the C of SOURCE's gen/ range of IMAGE,
and OUT's .syms. The C defines the range's data in D, in DHEAD under #pragma
section HEAD, or const in C, by the section the range's row lies in. Where
the data points at an address outside the range that the tables don't name,
the C makes up a name for it, and OUT's .syms gives each such name's
address. The tool reads declared.txt beside OUT, which --declared writes.

--declared writes OUT, declared.txt: the type the extern declarations give
each name in CONFIG's gen/ ranges. It reads the .c and .h files under the
image's directories, its own first: for 1ST_READ.BIN, every source root but
sdk/; for another image, its own directory and each other modules/ or
events/ directory it links a source from. A name takes its type from the
first directory to declare it, and that directory's declarations of it must
agree.

--rows prints the gen/ rows it proposes for IMAGE's data: 1ST_READ.BIN's
game data, or another image's D. It proposes rows only where no row of a
source outside gen/ covers the data. A # line gives a range it proposes no
row for, and why.

--bodies writes these files in OUT, the .inc files from the bytes IMAGE
holds where LISTING, the stub build's listing, places SOURCE's data in
SOURCE's rows:

- OUT/NAME.inc for each body SOURCE declares;
- OUT/text.inc for SOURCE's texts;
- OUT/values.inc for SOURCE's values, each the number IMAGE holds in the
  member or element the value stands for in an initializer of SOURCE;
- OUT/bodies.txt, each body's, text's and value's name, address, size and
  type.

A name outside SOURCE that a body points at takes its type from the extern
declarations of SOURCE and its headers, which must agree. A function a body
points at must have a prototype there. --bodies reads the types SOURCE and
its headers declare without a preprocessor: it ignores conditionals, and
reads every branch alike.

--texts writes OUT, the stub build's text.inc: a distinct placeholder for
each of SOURCE's texts. --values writes OUT, the stub build's values.inc: 0
for each of SOURCE's values.

TOOLCHAIN.md#declared-data gives the rules a body keeps.
"""

import argparse
import contextlib
import copy
import os
import re
import struct
import sys
from array import array
from bisect import bisect_left, bisect_right
from collections import Counter, namedtuple
from decimal import ROUND_CEILING, Decimal
from functools import partial
from pathlib import Path

import config
from config import (ALONE, FIRST, FIRST_BASE, FIRST_END, FIRST_GAME_CODE, FIRST_GAME_DATA, FIRST_SDK_CODE, ZEROED,
                    content, directories, fail, table)


# SHC's layout of C types
#
# Lay out the C types a source and its headers define, as SHC lays them out.
# Types reads struct, union and enum definitions, typedefs and object-like
# macros without a preprocessor. Of the preprocessor's directives it reads
# only #define. It follows no #include, so the caller hands it each header's
# text. It ignores conditionals and reads every branch alike, so a name two
# branches define differently is a name defined two ways.
#
# A type is laid out only when asked for. One Types can't lay out raises
# LayoutError then:
#
# - long long, double, void, a bit-field or a function;
# - an incomplete type, or a name defined two ways;
# - a type name or a member Types can't read;
# - an extent Types can't evaluate, or a negative one.
#
# An extent is an integer constant expression without sizeof, a cast, a
# character constant, ?: or a function-like macro. Types evaluates it in
# unbounded integers, each value keeping the type SHC gives it, and the result
# is SHC's while every value lies in its type. LayoutError names each
# operation whose value SHC could give otherwise, or not at all:
#
# - a result outside its type;
# - an operand outside the type it converts to;
# - a shift by a count outside 0 to 31;
# - a division by zero.
#
# Both sides of && and || are evaluated, so these fail on the side C skips
# too, as a division by zero does in SHC.


class LayoutError(Exception):
    """A type the declarations don't lay out."""


# comments, and the strings and characters a comment can't start in
COMMENT = re.compile(r'"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'|/\*.*?\*/|//[^\n]*', re.S)
DEFINE = re.compile(r'#\s*define\s+(\w+)(\(?)(.*)$')
TOKEN = re.compile(r'[A-Za-z_]\w*|\.?[0-9](?:[eEpP][+-]|[\w.])*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
                   r'|<<|>>|<=|>=|==|!=|&&|\|\||->|\S')
NAME = re.compile(r'[A-Za-z_]\w*')
# C's type qualifiers, in the order a cast spells them
QUALIFIERS = ('const', 'volatile')
# the storage-class specifiers, typedef among them, which change no layout
STORAGE = {'static', 'extern', 'register', 'auto', 'typedef'}
BASIC = {'void', 'char', 'short', 'int', 'long', 'float', 'double', 'signed', 'unsigned'}
KEYWORDS = {*QUALIFIERS, *STORAGE, *BASIC, 'struct', 'union', 'enum', 'sizeof'}
# C's binary operators, by precedence
BINARY = {'||': 1, '&&': 2, '|': 3, '^': 4, '&': 5, '==': 6, '!=': 6, '<': 7, '>': 7, '<=': 7, '>=': 7,
          '<<': 8, '>>': 8, '+': 9, '-': 9, '*': 10, '/': 10, '%': 10}
# the values of a signed integer and an unsigned one, each 32 bits on SHC
INT = range(-0x80000000, 0x80000000)
UNSIGNED = range(0x100000000)
# SHC's integer types and their values, in the order of its usual arithmetic
# conversions: both operands convert to the later of their types. So a long
# and an unsigned int convert to long, where C89 converts both to unsigned
# long. SHC converts so in arithmetic too, as in the comparison
# TOOLCHAIN.md#c-for-shc names.
INTEGER_TYPES = {'int': INT, 'unsigned int': UNSIGNED, 'long': INT, 'unsigned long': UNSIGNED}


def uncommented(text, lines=True):
    """C's text with each comment a space, its strings and characters kept.
    With lines, the space keeps the comment's line breaks, so each line
    keeps its number. Without, a directive runs on past a comment, as C
    reads it."""
    def blank(match):
        found = match.group()
        if found[0] in '"\'':
            return found
        return ' ' + '\n' * found.count('\n') if lines else ' '

    return COMMENT.sub(blank, text)


class Scalar(namedtuple('Scalar', 'code')):
    """An arithmetic type, by its struct code, or c for plain char."""

    @property
    def size(self):
        return struct.calcsize(self.code)

    alignment = size


class Pointer(namedtuple('Pointer', 'pointee')):
    """A pointer, by its pointee as a cast spells it, qualifiers first.

    An array pointee is spelled as its element with [] for each extent.
    pointee is None for a function, an unnamed struct, union or enum, a
    pointer to an array, or a type built on one."""

    size = alignment = 4


class Array(namedtuple('Array', 'element count')):
    """An array, by its element's layout and its count."""

    @property
    def size(self):
        return self.element.size * self.count

    @property
    def alignment(self):
        return self.element.alignment


# a struct's or a union's layout: whether it's a union, its members as
# (name, layout, offset), its size, its alignment, and its holes. A hole is a
# run of bytes no initializer writes, as (offset, size): a struct's padding,
# or a union's bytes past its first member
Record = namedtuple('Record', 'union fields size alignment holes')


def basic(words):
    """The scalar a list of C's type words names. Raises LayoutError for
    long long, double or void."""
    count = {word: words.count(word) for word in words}
    if count.get('long', 0) > 1 or 'double' in count or 'void' in count:
        raise LayoutError(f'{" ".join(words)}: no layout')
    if 'float' in count:
        return Scalar('f')
    code = 'b' if 'char' in count else 'h' if 'short' in count else 'i'
    if 'unsigned' in count:
        return Scalar(code.upper())
    return Scalar('c' if code == 'b' and 'signed' not in count else code)


def type_name(term):
    """A type as Pointer's pointee spells it."""
    kind = term[0]
    if kind == 'base':
        return ' '.join(term[1])
    if kind == 'qualified':
        inner = type_name(term[1])
        return f'{" ".join(term[2])} {inner}' if inner else None
    if kind == 'typedef':
        return term[1]
    if kind in ('struct', 'union', 'enum'):
        return None if term[1].startswith('#') else f'{kind} {term[1]}'
    inner = type_name(term[1]) if kind in ('pointer', 'array') else None
    if kind == 'pointer':
        return inner + ' *' if inner and '[]' not in inner else None
    return inner + '[]' if inner else None


def number(text):
    """An integer constant's value and type, as SHC types it.

    The type is unsigned with a U, or where int can't hold the value, and
    long with an L. So a decimal constant that int can't hold is unsigned
    int, where C89 makes it unsigned long. Raises LayoutError for text that
    is no integer constant, or a value wider than 32 bits."""
    match = re.fullmatch(r'(0[xX][0-9a-fA-F]+|0[0-7]*|[1-9][0-9]*)([uU]?[lL]?|[lL][uU])', text)
    if not match:
        raise LayoutError(f'{text}: no integer constant')
    digits, suffix = match.group(1), match.group(2).lower()
    value = int(digits, 16 if digits[:2] in ('0x', '0X') else 8 if digits[0] == '0' else 10)
    if value not in UNSIGNED:
        raise LayoutError(f'{text}: wider than 32 bits')
    unsigned = 'u' in suffix or value not in INT
    return value, ('unsigned ' if unsigned else '') + ('long' if 'l' in suffix else 'int')


def in_type(text, value, ctype):
    """(value, ctype) as given, where value lies in ctype and so is SHC's
    value too. Raises LayoutError, naming text, the operation that gives the
    value, where it lies outside ctype."""
    if value not in INTEGER_TYPES[ctype]:
        raise LayoutError(f'{text}: {value} lies outside {ctype}')
    return value, ctype


def binary(op, left, right, text):
    """(value, type) of op on left and right, each (value, type). text is the
    operation, which an error names.

    Raises LayoutError where SHC could give another value or none:

    - an operand outside the type it converts to;
    - a result or a quotient outside its type;
    - a shift by a count outside 0 to 31;
    - a division by zero."""
    (a, a_type), (b, b_type) = left, right
    if op in ('&&', '||'):
        # an int, from each side's truth, with neither side converted
        return int(bool(a or b) if op == '||' else bool(a and b)), 'int'
    if op in ('<<', '>>'):
        # of the left side's type, whatever the count's
        if b not in range(32):
            raise LayoutError(f'{text}: a shift by {b}, outside 0 to 31')
        return in_type(text, a << b if op == '<<' else a >> b, a_type)
    # both sides convert to the later of their types, which has to hold them
    ctype = max(a_type, b_type, key=list(INTEGER_TYPES).index)
    for value in (a, b):
        in_type(text, value, ctype)
    if op in ('/', '%'):
        if not b:
            raise LayoutError(f'{text}: a division by zero')
        # SHC's quotient truncates toward zero, and has to lie in the type for
        # % too
        quotient = abs(a) // abs(b) * (1 if (a < 0) == (b < 0) else -1)
        if op == '%' and quotient not in INTEGER_TYPES[ctype]:
            raise LayoutError(f'{text}: its quotient {quotient} lies outside {ctype}')
        return in_type(text, quotient if op == '/' else a - b * quotient, ctype)
    value = {'|': a | b, '^': a ^ b, '&': a & b, '==': a == b, '!=': a != b, '<': a < b, '>': a > b,
             '<=': a <= b, '>=': a >= b, '+': a + b, '-': a - b, '*': a * b}[op]
    if op in ('==', '!=', '<', '>', '<=', '>='):
        return int(value), 'int'
    return in_type(text, value, ctype)


class Types:
    """The declarations in texts: the contents of a source and the headers
    it includes.

    Types represents a type as a term, one of:

    - ('base', type words);
    - ('typedef', name);
    - (kind, tag) for a struct, union or enum. One with no tag takes
      '#file:index' as its tag: file is its text's index in files, and index
      that of its opening brace among the text's tokens;
    - ('qualified', term, qualifiers);
    - ('pointer', term);
    - ('array', term, extent tokens), with None for the tokens of [];
    - ('function', term).
    """

    def __init__(self, texts):
        self.files = []  # each text's tokens
        self.macros = {}  # each object-like macro's bodies, as token tuples
        self.functions = set()  # the function-like macros, which no extent here expands
        self.enumerators = {}  # each enumerator's value tokens, None without an =, and the one before it
        self.typedefs = {}  # each typedef's terms
        self.definitions = {}  # the bodies of each struct, union and enum by (kind, tag), as (file, open, close)
        self.cache = {}
        for text in texts:
            self.read(text)

    def read(self, text):
        text = uncommented(text, lines=False)
        code = []
        for line in text.replace('\\\n', '').splitlines():
            directive = DEFINE.match(line.strip())
            if directive and directive.group(2):
                self.functions.add(directive.group(1))
            elif directive:
                self.macros.setdefault(directive.group(1), set()).add(tuple(TOKEN.findall(directive.group(3))))
            if not line.lstrip().startswith('#'):
                code.append(line)
        file = len(self.files)
        tokens = TOKEN.findall('\n'.join(code))
        self.files.append(tokens)
        for index, token in enumerate(tokens):
            try:
                if token == 'typedef':
                    self.typedef(file, index + 1)
                elif token in ('struct', 'union', 'enum'):
                    self.specifiers(file, index)
            except (LayoutError, IndexError):
                # a declaration Types can't read: a type that needs it fails to lay out
                continue

    def closing(self, tokens, index):
        """The index of the bracket that closes the one at index. Raises
        LayoutError where none does."""
        pair, depth = {'{': '}', '(': ')', '[': ']'}[tokens[index]], 0
        for at in range(index, len(tokens)):
            depth += tokens[at] == tokens[index]
            depth -= tokens[at] == pair
            if not depth:
                return at
        raise LayoutError(f'an unclosed {tokens[index]}')

    def define(self, kind, key, file, open_, close):
        self.definitions.setdefault((kind, key), set()).add((file, open_, close))
        if kind != 'enum':
            return
        tokens, at, before = self.files[file], open_ + 1, None
        while at < close:
            name, at = tokens[at], at + 1
            end = at
            # a value runs to the next comma outside its brackets
            while end < close and tokens[end] != ',':
                end = self.closing(tokens, end) + 1 if tokens[end] in '([{' else end + 1
            value = tuple(tokens[at + 1:end]) if at < close and tokens[at] == '=' else None
            self.enumerators.setdefault(name, set()).add((value, before))
            before, at = name, end + 1

    def specifiers(self, file, index):
        """A declaration's type, as a term, and the index where its declarators
        start. A struct, union or enum body among the specifiers is recorded as
        that type's definition, with an enum's enumerators. Raises LayoutError
        for a struct, union or enum with neither a tag nor a body, or for
        specifiers that give no type."""
        tokens, words, term, qualifiers = self.files[file], [], None, set()
        while index < len(tokens):
            token = tokens[index]
            if token in QUALIFIERS:
                qualifiers.add(token)
                index += 1
            elif token in STORAGE:
                index += 1
            elif token in ('struct', 'union', 'enum'):
                kind, key, index = token, None, index + 1
                if NAME.fullmatch(tokens[index]) and tokens[index] not in KEYWORDS:
                    key, index = tokens[index], index + 1
                if tokens[index] == '{':
                    close = self.closing(tokens, index)
                    key = key or f'#{file}:{index}'
                    self.define(kind, key, file, index, close)
                    index = close + 1
                if key is None:
                    raise LayoutError(f'{kind} with neither a tag nor a body')
                term = (kind, key)
            elif token in BASIC:
                words.append(token)
                index += 1
            elif term is None and not words and NAME.fullmatch(token) and token not in KEYWORDS:
                term, index = ('typedef', token), index + 1
            else:
                break
        if words:
            term = ('base', tuple(words))
        if term is None:
            raise LayoutError(f'{tokens[index] if index < len(tokens) else "the end"}: no type')
        if qualifiers:
            term = ('qualified', term, tuple(word for word in QUALIFIERS if word in qualifiers))
        return term, index

    def declarator(self, file, index):
        """A declarator's name, or None for an abstract one; its derivations;
        and the index where it ends.

        derive() applies the derivations, in order, to the type before the
        declarator: the specifiers' type, or for a declarator in parentheses,
        the type that the declarator around it builds. Raises LayoutError for
        a parenthesis that holds no declarator, or a bracket that doesn't
        close."""
        tokens, pointers = self.files[file], 0
        while tokens[index] in ('*', *QUALIFIERS):
            pointers += tokens[index] == '*'
            index += 1
        name, inner = None, []
        if tokens[index] == '(':
            name, inner, index = self.declarator(file, index + 1)
            if tokens[index] != ')':
                raise LayoutError(f'{tokens[index]}: no declarator')
            index += 1
        elif NAME.fullmatch(tokens[index]) and tokens[index] not in KEYWORDS:
            name, index = tokens[index], index + 1
        suffixes = []
        while index < len(tokens) and tokens[index] in ('[', '('):
            close = self.closing(tokens, index)
            extent = tuple(tokens[index + 1:close]) or None
            suffixes.append(('array', extent) if tokens[index] == '[' else ('function',))
            index = close + 1
        return name, [('pointer',)] * pointers + suffixes[::-1] + inner, index

    def typedef(self, file, index):
        tokens = self.files[file]
        base, index = self.specifiers(file, index)
        while True:
            name, derivations, index = self.declarator(file, index)
            if name is None or tokens[index] not in (',', ';'):
                raise LayoutError(f'a typedef of {name}, then {tokens[index]}')
            self.typedefs.setdefault(name, set()).add(derive(base, derivations))
            if tokens[index] == ';':
                return
            index += 1

    def members(self, kind, key):
        """A struct's or a union's members, as (name, term), in order. Raises
        LayoutError where it has no definition or two that differ, or a member
        that is a bit-field or that Types can't read."""
        bodies = self.definitions.get((kind, key))
        name = type_name((kind, key)) or f'an unnamed {kind}'
        if not bodies:
            raise LayoutError(f'{name}: no definition')
        if len({tuple(self.files[file][open_ + 1:close]) for file, open_, close in bodies}) > 1:
            raise LayoutError(f'{name}: defined two ways')
        (file, open_, close), *_ = sorted(bodies)
        tokens, index, found = self.files[file], open_ + 1, []
        while index < close:
            base, index = self.specifiers(file, index)
            while True:
                member, derivations, index = self.declarator(file, index)
                if tokens[index] == ':':
                    raise LayoutError(f'{name}: {member} is a bit-field')
                if member is None or tokens[index] not in (',', ';'):
                    raise LayoutError(f'{name}: a member {member}, then {tokens[index]}')
                found.append((member, derive(base, derivations)))
                index += 1
                if tokens[index - 1] == ';':
                    break
        return found

    def term(self, ctype):
        """A type name's term. Raises LayoutError where ctype is no type
        name."""
        index = len(self.files)
        self.files.append(TOKEN.findall(ctype) + [';'])
        try:
            base, at = self.specifiers(index, 0)
            name, derivations, at = self.declarator(index, at)
            if name is not None or self.files[index][at] != ';':
                raise LayoutError(f'{ctype}: no type name')
            return derive(base, derivations)
        finally:
            self.files.pop()

    def layout(self, term):
        """A type's layout, a Scalar, Pointer, Array or Record, from a term or
        a type name. Raises LayoutError where the declarations don't lay the
        type out."""
        if isinstance(term, str):
            term = self.term(term)
        if term not in self.cache:
            self.cache[term] = self.lay(term)
        return self.cache[term]

    def lay(self, term):
        kind = term[0]
        if kind == 'base':
            return basic(list(term[1]))
        if kind == 'qualified':
            return self.layout(term[1])
        if kind == 'typedef':
            terms = self.typedefs.get(term[1])
            if not terms:
                raise LayoutError(f'{term[1]}: no type')
            if len(terms) > 1:
                raise LayoutError(f'{term[1]}: defined two ways')
            return self.layout(next(iter(terms)))
        if kind == 'enum':
            return Scalar('i')
        if kind == 'pointer':
            return Pointer(type_name(term[1]))
        if kind == 'array':
            if term[2] is None:
                raise LayoutError(f'{type_name(term[1]) or "an array"}[]: no extent')
            extent = f'[{" ".join(term[2])}]'
            try:
                count = self.evaluate(term[2])
            except LayoutError as error:
                raise LayoutError(f'{extent}: {error}') from None
            if count < 0:
                raise LayoutError(f'{extent}: a negative extent')
            return Array(self.layout(term[1]), count)
        if kind == 'function':
            raise LayoutError('a function: no layout')
        fields, offset, alignment = [], 0, 1
        for name, member in self.members(kind, term[1]):
            layout = self.layout(member)
            alignment = max(alignment, layout.alignment)
            if kind == 'struct':
                offset = -(-offset // layout.alignment) * layout.alignment
            fields.append((name, layout, offset if kind == 'struct' else 0))
            offset = offset + layout.size if kind == 'struct' else max(offset, layout.size)
        size = -(-offset // alignment) * alignment
        if kind == 'union':
            written = [(0, fields[0][1].size)] if fields else []
        else:
            written = [(at, layout.size) for _, layout, at in fields]
        holes, cursor = [], 0
        for at, length in [*written, (size, 0)]:
            if at > cursor:
                holes.append((cursor, at - cursor))
            cursor = max(cursor, at + length)
        return Record(kind == 'union', tuple(fields), size, alignment, tuple(holes))

    def expand(self, tokens, seen=frozenset()):
        """tokens with their object-like macros replaced, as the preprocessor
        would replace them: no macro expands again inside its own expansion.
        Raises LayoutError for a macro defined two ways."""
        result = []
        for token in tokens:
            if token in self.macros and token not in seen:
                bodies = self.macros[token]
                if len(bodies) > 1:
                    raise LayoutError(f'{token}: defined two ways')
                result += self.expand(next(iter(bodies)), seen | {token})
            else:
                result.append(token)
        return result

    def evaluate(self, tokens, seen=frozenset()):
        """The value SHC gives an integer constant expression, with its macros
        and enumerators expanded. Raises LayoutError where SHC's value could
        differ, or where the tokens are no expression Types evaluates."""
        tokens = self.expand(tokens)
        (value, _), index = self.expression(tokens, 0, 1, seen)
        if index != len(tokens):
            raise LayoutError(f'{" ".join(tokens)}: no integer constant')
        return value

    def expression(self, tokens, index, least, seen):
        """(value, type) of the expression at index, as far as its binary
        operators bind at least as tightly as precedence least, and the index
        where it ends."""
        start = index
        value, index = self.unary(tokens, index, seen)
        while index < len(tokens) and BINARY.get(tokens[index], 0) >= least:
            op = tokens[index]
            right, index = self.expression(tokens, index + 1, BINARY[op] + 1, seen)
            value = binary(op, value, right, ' '.join(tokens[start:index]))
        return value, index

    def unary(self, tokens, index, seen):
        if index == len(tokens):
            raise LayoutError(f'{" ".join(tokens)}: no integer constant')
        token = tokens[index]
        if token in ('-', '+', '~', '!'):
            (value, ctype), end = self.unary(tokens, index + 1, seen)
            if token == '!':
                return (int(not value), 'int'), end
            return in_type(' '.join(tokens[index:end]), {'-': -value, '+': value, '~': ~value}[token], ctype), end
        if token == '(':
            value, index = self.expression(tokens, index + 1, 1, seen)
            if index == len(tokens) or tokens[index] != ')':
                raise LayoutError(f'{" ".join(tokens)}: no integer constant')
            return value, index + 1
        if token in self.enumerators and token not in seen:
            return self.enumerator(token, seen | {token}), index + 1
        if token in self.functions:
            raise LayoutError(f'{token}: a function-like macro')
        return number(token), index + 1

    def enumerator(self, name, seen):
        """(value, 'int') of an enumerator: its given value, or one more than
        the value of the enumerator before it, and 0 for a first with none
        given. A loop walks back rather than recursing, as an enum can be
        longer than Python's recursion is deep. Raises LayoutError for an
        enumerator defined two ways, or a value outside int or that evaluate()
        refuses."""
        asked, after = name, 0
        while True:
            values = self.enumerators[name]
            if len(values) > 1:
                raise LayoutError(f'{name}: defined two ways')
            (given, before), = values
            if given is not None or before is None:
                break
            name, after, seen = before, after + 1, seen | {before}
        return in_type(asked, after + (0 if given is None else self.evaluate(given, seen)), 'int')


def derive(base, derivations):
    """A declarator's type, from its specifiers' type and its derivations."""
    term = base
    for derivation in derivations:
        term = (derivation[0], term, *derivation[1:])
    return term


# The C emitter
#
# Write an image's data as C that SHC 5.0 compiles to the same bytes. The
# emitter reads the image as Memory: its bytes by linked address, its code
# ranges, its address windows, and the name its errors give it. An address
# window is a range where a word's value is taken as an address. An Item is
# one definition: its bytes, its kind, and how C presents it. A Writer turns a
# run of items into a source. It writes pointers as expressions of names, each
# float as the literal of the fewest significant digits that SHC's conversion
# gives back, a union's value without the braces SHC rejects, and each string
# escaped for SHC's preprocessor. It writes a value of any type Types lays
# out, or raises the error below.
#
# The emitter raises EmitError, its one error, for:
#
# - a float that no literal of up to 9 digits and within FLT_MAX gives back;
# - a pointer into the padding between definitions;
# - a byte no initializer writes, where it isn't zero;
# - a byte read outside the image.
#
# The error names the image, the address and the problem there.
#
# The table of structures gives each one's size, alignment and members'
# offsets. The readers and the recognizer read it too.


class EmitError(Exception):
    """Bytes at an address of an image that C can't present as SHC needs them."""

    def __init__(self, image, address, problem):
        super().__init__(f'{image.name}: 0x{address:08x}: {problem}')


class Memory:
    """An image's bytes by their linked addresses, with its code ranges, its
    address windows, and the name its errors give it."""

    def __init__(self, name, payload, base, code=(), windows=()):
        self.name = name
        self.payload = payload
        self.base = base
        self.end = base + len(payload)
        self.code = code  # the ranges of code: the image's own, and any other image's it points into
        self.windows = windows  # the address windows: the ranges where a word's value is taken as an address

    def in_code(self, address):
        return any(start <= address < end for start, end in self.code)

    def is_address(self, word):
        return any(start <= word < end for start, end in self.windows)


class Struct:
    """A structure's or a union's size and alignment. Each member this file
    reads by name is an attribute that gives the member's offset."""

    def __init__(self, size, alignment=4, **offsets):
        self.size, self.alignment = size, alignment
        vars(self).update(offsets)


# The structures this file reads and writes, as SHC lays them out. First come
# ninja.h's, an mdata by its count of factors and a motion's key by its type
# bit. Then come a stage's path and its points, as src/path/path.h has them,
# which only the recognizer reads. A motion's keys, in the order of their type
# bits, are position, angle, scale, vector, vertex, normal, target, roll,
# angle of view, light color, intensity, spot, point and quaternion; ninja.h
# types the first six.
OBJECT = Struct(52, evalflags=0, model=4, position=8, angle=20, scale=32, child=44, sibling=48)
MODEL = Struct(40, points=0, normals=4, point_count=8, meshsets=12, materials=16, meshset_count=20,
               material_count=22, center=24, radius=36)
MESHSET = Struct(24, type_material=0, mesh_count=2, strips=4, attrs=8, normals=12, colors=16, uvs=20)
MATERIAL = Struct(20, diffuse=0, specular=4, exponent=8, texture=12, flags=16)
POINT = Struct(12)
COLOR = Struct(4, u=0, v=2)
MOTION = Struct(12, mdata=0, frames=4, bits=8, interpolation=10)
MDATA = {factors: Struct(8 * factors, pointers=0, counts=4 * factors) for factors in range(1, 5)}
MKEY_F = Struct(16, frame=0, key=4)
MKEY_A = Struct(16, frame=0, key=4)
MKEY_P = Struct(8, frame=0, key=4)
KEYS = {0x1: MKEY_F, 0x2: MKEY_A, 0x4: MKEY_F, 0x8: MKEY_F, 0x10: MKEY_P, 0x20: MKEY_P, 0x40: Struct(16),
        0x80: Struct(8), 0x100: Struct(8), 0x200: Struct(8), 0x400: Struct(8), 0x800: Struct(20),
        0x1000: Struct(12), 0x2000: Struct(20)}
ACTION = Struct(8, object=0, motion=4)
CACTION = Struct(8, camera=0, motion=4)
TEXLIST = Struct(8, names=0, count=4)
TEXNAME = Struct(12, name=0, attributes=4, address=8)
TEXANIM = Struct(20, alignment=2)
STAGE_PATH = Struct(16, ends=0, count=2, length=4, points=8, step=12)
STAGE_PATH_POINT = Struct(20, floats=4)

# each kind's C type, its element's layout, and whether it's one record rather
# than an array. ninja.h's kinds come first, then the kinds of bytes no
# structure claims. An mdata isn't here: Item gives its type and layout by its
# factor count.
Kind = namedtuple('Kind', 'ctype layout single')
KINDS = {
    'object': Kind('NJS_OBJECT', OBJECT, True),
    'model': Kind('NJS_MODEL', MODEL, True),
    'motion': Kind('NJS_MOTION', MOTION, True),
    'action': Kind('NJS_ACTION', ACTION, True),
    'caction': Kind('NJS_CACTION', CACTION, True),
    'texlist': Kind('NJS_TEXLIST', TEXLIST, True),
    'meshset': Kind('NJS_MESHSET', MESHSET, False),
    'material': Kind('NJS_MATERIAL', MATERIAL, False),
    'points': Kind('NJS_POINT3', POINT, False),
    'normals': Kind('NJS_VECTOR', POINT, False),
    'vertices': Kind('NJS_POINT3', POINT, False),
    'strips': Kind('Sint16', Scalar('h'), False),
    'attrs': Kind('Uint32', Scalar('I'), False),
    'vcolor': Kind('NJS_COLOR', COLOR, False),
    'uv': Kind('NJS_COLOR', COLOR, False),
    'mkey_f': Kind('NJS_MKEY_F', MKEY_F, False),
    'mkey_a': Kind('NJS_MKEY_A', MKEY_A, False),
    'mkey_p': Kind('NJS_MKEY_P', MKEY_P, False),
    'texname': Kind('NJS_TEXNAME', TEXNAME, False),
    'texanim': Kind('NJS_TEXANIM', TEXANIM, False),
    'pointers': Kind('void *', Pointer('void'), False),
    'mixed': Kind('void *', Pointer('void'), False),
    'words': Kind('Uint32', Scalar('I'), False),
    'floats': Kind('Float', Scalar('f'), False),
    'halves': Kind('Uint16', Scalar('H'), False),
    'bytes': Kind('Uint8', Scalar('B'), False),
    'string': Kind('char', Scalar('c'), False),
}

# the kinds of bytes no structure claims, but for a string
GENERIC = {'pointers', 'mixed', 'words', 'floats', 'halves', 'bytes'}

# the types of the kinds that are one record
SINGLE = {kind.ctype for kind in KINDS.values() if kind.single}

# the kind of a name a source declares, by the type it declares it with
DECLARED = {'NJS_OBJECT': 'object', 'NJS_MODEL': 'model', 'NJS_MOTION': 'motion', 'NJS_ACTION': 'action',
            'NJS_CACTION': 'caction', 'NJS_TEXLIST': 'texlist', 'NJS_TEXNAME': 'texname',
            'NJS_MATERIAL': 'material', 'NJS_MESHSET': 'meshset', 'NJS_POINT3': 'points',
            'NJS_VECTOR': 'normals', 'NJS_MKEY_F': 'mkey_f', 'NJS_MKEY_A': 'mkey_a',
            'NJS_TEXANIM': 'texanim'}

# ninja.h spells one struct two ways
SAME = {'NJS_VECTOR': 'NJS_POINT3'}

# the escapes of a text's bytes in C, and the characters that would continue
# a numeric one
ESCAPES = {0x00: '\\0', 0x07: '\\a', 0x09: '\\t', 0x0A: '\\n', 0x0D: '\\r', 0x22: '\\"', 0x5C: '\\\\'}
CONTINUE = {'0': '01234567', 'x': '0123456789abcdefABCDEF'}

# the names flags() gives a node's evalflags bits 0 to 5, as ninja.h spells
# them; with a higher bit set, it writes the evalflags in hex
EVAL = ('NJD_EVAL_UNIT_POS', 'NJD_EVAL_UNIT_ANG', 'NJD_EVAL_UNIT_SCL', 'NJD_EVAL_HIDE',
        'NJD_EVAL_BREAK', 'NJD_EVAL_ZXY_ANG')

# the names flags() gives a motion's type bits 0 to 2, as ninja.h spells
# them; with a higher bit set, it writes the type bits in hex
MTYPE = ('NJD_MTYPE_POS_0', 'NJD_MTYPE_ANG_1', 'NJD_MTYPE_SCL_2')


def unqualified(ctype):
    """A type's qualifiers, and the type without them."""
    words = ctype.split()
    return {word for word in words if word in QUALIFIERS}, ' '.join(word for word in words if word not in QUALIFIERS)


def converts(natural, pointee):
    """Whether a pointer to natural initializes a pointer to pointee with no
    cast: pointee keeps every qualifier natural has, and is void or natural's
    type. SAME's two spellings of one struct count as one type."""
    held, natural = unqualified(natural)
    kept, pointee = unqualified(pointee)
    return held <= kept and (pointee == 'void' or SAME.get(natural, natural) == SAME.get(pointee, pointee))


def f32(word):
    return struct.unpack('<f', struct.pack('<I', word))[0]


def float_ok(word):
    """Whether word is zero or a normal float. This checks the word's shape
    alone: spell() can still find no literal for it, as for FLT_MAX."""
    exponent = (word >> 23) & 0xFF
    return exponent != 0xFF and (exponent != 0 or word & 0x7FFFFF == 0)


def spell(word):
    """The decimal literal of the fewest significant digits, up to 9, that
    SHC's conversion reads back as the word, or None where none is
    supported. SHC's conversion truncates toward zero, so the literal's
    magnitude lies from the word's up to the next float's. FLT_MAX gets
    None: each literal of up to 9 digits that truncates to it lies above
    it."""
    sign = '-' if word >> 31 else ''
    magnitude = word & 0x7FFFFFFF
    if not magnitude:
        return sign + '0.0F'
    if not float_ok(word):
        return None
    low, high = Decimal(f32(magnitude)), Decimal(f32(magnitude + 1))
    if high.is_infinite():
        # FLT_MAX: each decimal of up to 9 digits that truncates to it lies
        # above it, and whether SHC takes one is untested
        return None
    for digits in range(1, 10):
        value = low.quantize(Decimal(1).scaleb(low.adjusted() - digits + 1), rounding=ROUND_CEILING)
        if value < high:
            break
    value = value.normalize()
    text = format(value, 'f') if -5 < value.adjusted() < 9 else format(value, 'e')
    if '.' not in text and 'e' not in text:
        text += '.0'
    return sign + text + 'F'


def payload(image, start, end):
    """The image's bytes from start to end. Raises EmitError where start
    passes end or any byte lies outside the image: a slice there would come
    back short or wrong."""
    if not image.base <= start <= end <= image.end:
        raise EmitError(image, start, f'{end - start} bytes to 0x{end:08x} lie outside the image')
    return image.payload[start - image.base:end - image.base]


class Item:
    """One definition: elements of a kind, from start to end."""

    def __init__(self, start, end, kind, factors=0, text=None):
        self.start, self.end, self.kind = start, end, kind
        self.factors = factors  # an mdata's factor count, which gives its type and element size
        self.text = text  # a string's text, without its null
        self.array = not self.single  # written as an array, even of one
        self.declared = None  # the type a source declares it with

    def part(self, start, end):
        return Item(start, end, self.kind, self.factors, self.text)

    @property
    def ctype(self):
        if self.declared:
            return self.declared
        return f'NJS_MDATA{self.factors}' if self.kind == 'mdata' else KINDS[self.kind].ctype

    @property
    def element(self):
        return MDATA[self.factors].size if self.kind == 'mdata' else KINDS[self.kind].layout.size

    @property
    def alignment(self):
        return MDATA[self.factors].alignment if self.kind == 'mdata' else KINDS[self.kind].layout.alignment

    @property
    def single(self):
        return self.kind != 'mdata' and KINDS[self.kind].single


class Writer:
    """The items as C: every pointer a name, every float a literal."""

    def __init__(self, image, items, names, constant=False):
        self.image = image
        self.items = items
        self.constant = constant  # whether the items are const, as data in C is
        self.starts = [item.start for item in items]
        self.names = names
        self.uses = {}  # each name from outside: its address, and the types it's taken as
        self.ahead = set()  # the indexes of the items pointed at before they're defined
        self.index = 0  # the index of the item source() is writing; a pointer to a later one declares it ahead

    def name(self, address):
        return self.names.get(address) or f'dat_{address:08x}'

    def unpack(self, code, address):
        """The number struct code reads at address, from payload()'s bytes."""
        return struct.unpack(f'<{code}', payload(self.image, address, address + struct.calcsize(code)))[0]

    def word(self, address):
        return self.unpack('I', address)

    def ref(self, address, pointee):
        r"""A constant for the pointer at address, as a pointee *.

        A null pointer is 0. A pointer into an item is written from the item's
        name: at its start, the name, or &name for one record; at an element,
        the element's address; elsewhere, a byte offset into it. Each is cast
        where C won't convert it to a pointee *. A pointer outside the items'
        span takes a name from outside: the address's name in names, or else
        fun_ for code or dat_ for data, then the address in hex. That name is
        left as \1name\2pointee\3 for source() to resolve, once every use
        has given the name its type.

        The name from outside goes into uses, with its address and the
        pointee. The index of an item after the one source() is writing goes
        into ahead.

        Raises EmitError for a pointer into the padding between definitions,
        or where address lies outside the image."""
        value = self.word(address)
        if value == 0:
            return '0'
        index = bisect_right(self.starts, value) - 1
        if index < 0 or not self.items[index].start <= value < self.items[index].end:
            if self.items[0].start <= value < self.items[-1].end:
                raise EmitError(self.image, address, f'a pointer to 0x{value:08x}, in the padding between definitions')
            name = self.names.get(value) or f'{"fun" if self.image.in_code(value) else "dat"}_{value:08x}'
            self.uses.setdefault(name, (value, set()))[1].add(pointee)
            return f'\1{name}\2{pointee}\3'
        item = self.items[index]
        if index > self.index:
            self.ahead.add(index)
        offset, base = value - item.start, self.name(item.start)
        element_type = 'void' if item.kind in ('pointers', 'mixed') else item.ctype
        if offset == 0:
            expression, natural = (base, element_type) if item.array else (f'&{base}', item.ctype)
        elif item.array and offset % item.element == 0:
            expression, natural = f'&{base}[{offset // item.element}]', element_type
        else:
            expression = f'(char *){"" if item.array else "&"}{base} + {offset}'
            natural = 'char'
            if pointee != 'char':
                expression = f'({expression})'
        if converts(natural, pointee):
            return expression
        return f'({pointee} *){expression}'

    def extern_type(self, name):
        """The type source() gives a name from outside in its extern
        declaration. It is 'code' for an address in code. Otherwise it is the
        one type the name's uses take other than void and char, or char where
        they take none or several."""
        address, pointees = self.uses[name]
        if self.image.in_code(address):
            return 'code'
        types = {SAME.get(pointee, pointee) for pointee in pointees} - {'void', 'char'}
        return types.pop() if len(types) == 1 else 'char'

    def resolve(self, match):
        name, pointee = match.group(1), match.group(2)
        ctype = self.extern_type(name)
        expression = f'&{name}' if ctype in SINGLE else name
        if pointee == 'void' or SAME.get(pointee, pointee) == ctype:
            return expression
        return f'({pointee} *){expression}'

    def float(self, address):
        """The float at address as the literal spell() gives. Raises
        EmitError where spell() gives none."""
        word = self.word(address)
        text = spell(word)
        if text is None:
            raise EmitError(self.image, address,
                            f'0x{word:08x} is no float a decimal literal of up to 9 digits and within FLT_MAX spells')
        return text

    def initializer(self, layout, address):
        """The initializer of the value at address, laid out as layout.

        A struct or an array is in braces. A union is its first member's value,
        unbraced, as a material's colors are. Raises EmitError for a hole that
        holds a byte other than zero, and where float(), ref() or payload()
        does."""
        if isinstance(layout, Scalar):
            if layout.code == 'f':
                return self.float(address)
            if layout.code == 'c':
                return character(self.unpack('B', address))
            return integer(layout.code, self.unpack(layout.code, address))
        if isinstance(layout, Pointer):
            return self.ref(address, layout.pointee or 'void')
        if isinstance(layout, Array):
            step = layout.element.size
            values = [self.initializer(layout.element, address + step * index) for index in range(layout.count)]
        else:
            for at, size in layout.holes:
                for where, byte in enumerate(payload(self.image, address + at, address + at + size), address + at):
                    if byte:
                        raise EmitError(self.image, where, f'a pad byte of 0x{byte:02x}, which no initializer writes')
            fields = layout.fields[:1] if layout.union else layout.fields
            values = [self.initializer(field, address + offset) for _, field, offset in fields]
            if layout.union:
                return values[0]
        return f'{{ {", ".join(values)} }}'

    def floats(self, address, count):
        return ', '.join(self.float(address + 4 * index) for index in range(count))

    def values(self, item):
        """Each element's initializer, and how many go on a line."""
        image, ref = self.image, self.ref
        u32, s32, u16, s16 = (partial(self.unpack, code) for code in 'IiHh')
        start, end, kind = item.start, item.end, item.kind
        raw = payload(image, start, end)
        steps = range(start, end, item.element)
        if item.single and end - start > item.element:
            # an array a source declares of a kind that is one record: each
            # record in turn
            values = []
            for address in steps:
                record = copy.copy(item)
                record.start, record.end = address, address + item.element
                values += self.values(record)[0]
            return values, 1
        if kind == 'object':
            angles = ', '.join(str(s32(start + OBJECT.angle + 4 * i)) for i in range(3))
            return [f'{{\n    {flags(u32(start + OBJECT.evalflags), EVAL)}, '
                    f'{ref(start + OBJECT.model, "NJS_MODEL")},\n'
                    f'    {{ {self.floats(start + OBJECT.position, 3)} }},\n    {{ {angles} }},\n'
                    f'    {{ {self.floats(start + OBJECT.scale, 3)} }},\n'
                    f'    {ref(start + OBJECT.child, "NJS_OBJECT")}, '
                    f'{ref(start + OBJECT.sibling, "NJS_OBJECT")},\n}}'], 1
        if kind == 'model':
            return [f'{{\n    {ref(start + MODEL.points, "NJS_POINT3")}, '
                    f'{ref(start + MODEL.normals, "NJS_VECTOR")}, {s32(start + MODEL.point_count)},\n'
                    f'    {ref(start + MODEL.meshsets, "NJS_MESHSET")}, '
                    f'{ref(start + MODEL.materials, "NJS_MATERIAL")}, '
                    f'{u16(start + MODEL.meshset_count)}, {u16(start + MODEL.material_count)},\n'
                    f'    {{ {self.floats(start + MODEL.center, 3)} }}, {self.float(start + MODEL.radius)},\n}}'], 1
        if kind == 'motion':
            return [f'{{ {ref(start + MOTION.mdata, "void")}, {u32(start + MOTION.frames)}, '
                    f'{flags(u16(start + MOTION.bits), MTYPE)}, {u16(start + MOTION.interpolation)} }}'], 1
        if kind == 'action':
            return [f'{{ {ref(start + ACTION.object, "NJS_OBJECT")}, '
                    f'{ref(start + ACTION.motion, "NJS_MOTION")} }}'], 1
        if kind == 'caction':
            return [f'{{ {ref(start + CACTION.camera, "NJS_CAMERA")}, '
                    f'{ref(start + CACTION.motion, "NJS_MOTION")} }}'], 1
        if kind == 'texlist':
            return [f'{{ {ref(start + TEXLIST.names, "NJS_TEXNAME")}, {u32(start + TEXLIST.count)} }}'], 1
        if kind in ('points', 'normals', 'vertices'):
            return [f'{{ {self.floats(address, 3)} }}' for address in steps], 1
        if kind == 'material':
            # its colors are unions, which SHC takes without braces
            return [f'{{ 0x{u32(address + MATERIAL.diffuse):08x}, 0x{u32(address + MATERIAL.specular):08x}, '
                    f'{self.float(address + MATERIAL.exponent)}, {u32(address + MATERIAL.texture)}, '
                    f'0x{u32(address + MATERIAL.flags):08x} }}' for address in steps], 1
        if kind == 'meshset':
            return [f'{{ 0x{u16(address + MESHSET.type_material):04x}, '
                    f'{u16(address + MESHSET.mesh_count)}, '
                    f'{ref(address + MESHSET.strips, "Sint16")}, {ref(address + MESHSET.attrs, "Uint32")}, '
                    f'{ref(address + MESHSET.normals, "NJS_VECTOR")}, '
                    f'{ref(address + MESHSET.colors, "NJS_COLOR")}, '
                    f'{ref(address + MESHSET.uvs, "NJS_COLOR")} }}' for address in steps], 1
        if kind == 'strips':
            values = [s16(address) for address in steps]
            # a strip to a line, its count first, where the strips end exactly
            # at the item's end; else 12 values to a line
            lines, cursor = [], 0
            while cursor < len(values) and 0 < values[cursor] & 0x3FFF <= len(values) - cursor - 1:
                count = (values[cursor] & 0x3FFF) + 1
                lines.append(', '.join(map(str, values[cursor:cursor + count])))
                cursor += count
                if cursor == len(values):
                    return lines, 1
            return [str(value) for value in values], 12
        if kind == 'uv':
            return [f'UV({s16(address + COLOR.u)}, {s16(address + COLOR.v)})' for address in steps], 6
        if kind == 'vcolor':
            return [f'0x{u32(address):08x}' for address in steps], 4
        if kind in ('mkey_f', 'mkey_a'):
            key, values = KINDS[kind].layout, []
            for address in steps:
                keys = (self.floats(address + key.key, 3) if kind == 'mkey_f' else
                        ', '.join(str(s32(address + key.key + 4 * i)) for i in range(3)))
                values.append(f'{{ {u32(address + key.frame)}, {{ {keys} }} }}')
            return values, 1
        if kind == 'mkey_p':
            return [f'{{ {u32(address + MKEY_P.frame)}, {ref(address + MKEY_P.key, "void")} }}'
                    for address in steps], 2
        if kind == 'mdata':
            mdata, values = MDATA[item.factors], []
            for address in steps:
                pointers = ', '.join(ref(address + mdata.pointers + 4 * index, 'void') for index in range(item.factors))
                counts = ', '.join(str(u32(address + mdata.counts + 4 * index)) for index in range(item.factors))
                values.append(f'{{ {{ {pointers} }}, {{ {counts} }} }}')
            return values, 1
        if kind == 'texname':
            return [f'{{ {ref(address + TEXNAME.name, "void")}, 0x{u32(address + TEXNAME.attributes):x}, '
                    f'0x{u32(address + TEXNAME.address):x} }}' for address in steps], 1
        if kind == 'texanim':
            return [f'{{ {", ".join(str(s16(address + 2 * i)) for i in range(TEXANIM.size // 2))} }}'
                    for address in steps], 1
        if kind in ('pointers', 'mixed'):
            values = []
            for address in range(start, end, 4):
                word = u32(address)
                if image.is_address(word):
                    values.append(ref(address, 'void'))
                else:
                    values.append(f'(void *)0x{word:08x}' if word else '0')
            return values, 4
        if kind == 'floats':
            return [self.float(address) for address in range(start, end, 4)], 6
        width = item.element
        return [f'0x{int.from_bytes(raw[i:i + width], "little"):0{2 * width}x}'
                for i in range(0, len(raw), width)], 16 // width if width < 4 else 8

    def typed(self, ctype, name):
        """name declared as a ctype, with const after the type where the items
        are const, so that an array of pointers is const and not what they
        point at."""
        space = '' if ctype.endswith('*') else ' '
        return f'{ctype}{space}{"const " if self.constant else ""}{name}'

    def definition(self, item):
        name, count = self.name(item.start), (item.end - item.start) // item.element
        ctype = item.ctype
        if item.kind == 'string':
            return f'{self.typed("char", name)}[{count}] = {quote(item.text.encode())};'
        if item.kind in GENERIC and not any(payload(self.image, item.start, item.end)):
            return f'{self.typed(ctype, name)}[{count}] = {{ 0 }};'
        values, per_line = self.values(item)
        if not item.array:
            return f'{self.typed(ctype, name)} = {values[0]};'
        return f'{self.typed(ctype, name)}[] = {{\n    ' + ',\n    '.join(wrap(values, per_line)) + ',\n};'

    def source(self, title, section=None):
        """The items' C file, titled title: ninja.h's include, an extern
        declaration for each name from outside and each item pointed at ahead
        of its definition, then the definitions. Where the data takes a
        section, section names it to #pragma section around the definitions.
        Raises EmitError where an item's bytes can't be written as C."""
        body = []
        for self.index, item in enumerate(self.items):
            body.append(self.definition(item))
        text = re.sub('\1([^\2]*)\2([^\3]*)\3', self.resolve, '\n\n'.join(body))
        lines = [f'/* {title} */', '', '#include <ninja.h>', '']
        for name in sorted(self.uses):
            ctype = self.extern_type(name)
            if ctype == 'code':
                lines.append(f'extern void {name}();')
            else:
                lines.append(f'extern {ctype} {name}{"" if ctype in SINGLE else "[]"};')
        for index in sorted(self.ahead):
            item = self.items[index]
            lines.append(f'extern {self.typed(item.ctype, self.name(item.start))}{"[]" if item.array else ""};')
        if section:
            return '\n'.join([*lines, '', f'#pragma section {section}', text, '#pragma section', ''])
        return '\n'.join([*lines, '', text, ''])


def flags(bits, names):
    """bits by their names, names[n] for bit n, joined by |. Zero is 0, and
    where a set bit has no name, all the bits are in hex."""
    if not bits or bits >> len(names):
        return f'0x{bits:x}' if bits else '0'
    return ' | '.join(name for bit, name in enumerate(names) if bits >> bit & 1)


def wrap(values, per_line):
    """values joined per_line to a line. Where per_line is 1 and the first
    value is a record of several lines, starting with { and a newline, each
    value's later lines are indented a level."""
    if per_line == 1 and values[0].startswith('{\n'):
        values = [value.replace('\n', '\n    ') for value in values]
    return [', '.join(values[i:i + per_line]) for i in range(0, len(values), per_line)]


def character(byte):
    """A byte as a character constant."""
    if byte == 0x27:
        return "'\\''"
    if byte in ESCAPES and byte != 0x22:
        return f"'{ESCAPES[byte]}'"
    return f"'{chr(byte)}'" if 0x20 <= byte < 0x7F else f"'\\x{byte:02x}'"


def quote(data):
    """Bytes as C strings, a new one after each newline. In a macro, SHC joins
    strings before it reads their escapes, so a character that would continue
    an escape is an escape too."""
    pieces, piece, escape = [], '', ''
    for byte in data:
        if byte in ESCAPES:
            char = ESCAPES[byte]
        elif byte == 0x3F and piece.endswith('?'):
            # ?? would start a trigraph
            char = '\\?'
        elif 0x20 <= byte < 0x7F and chr(byte) not in CONTINUE.get(escape, ''):
            char = chr(byte)
        else:
            char = f'\\x{byte:02x}'
        piece += char
        escape = char[1] if char[0] == '\\' and char[1] in CONTINUE else ''
        if byte == 0x0A:
            pieces.append(piece)
            piece, escape = '', ''
    pieces.append(piece)
    return ' \\\n    '.join(f'"{piece}"' for piece in pieces if piece)


def integer(code, value):
    """A signed integer in decimal, and an unsigned one's bits in hex."""
    return str(value) if code.islower() else f'0x{value:0{2 * struct.calcsize(code)}x}'


# The Ninja readers
#
# Decode Ninja's structures at addresses the caller names in an image. The
# readers read the image as Data: Memory, with reads that check where their
# bytes lie. A structure's reader raises ReadError where the bytes can't be
# that structure, as where they lie outside the image or off the structure's
# alignment. A read raises it where its bytes lie outside the image. ReadError
# is the one error this section raises, and names the image, the address and
# what the bytes there can't be. Nothing here judges whether a value is
# plausible; the recognizer does. Chunk models are not supported.


Object = namedtuple('Object', 'evalflags model child sibling')
Model = namedtuple('Model', 'points normals point_count meshsets materials meshset_count material_count')
Meshset = namedtuple('Meshset', 'polygon mesh_count strips attrs normals colors uvs')
Motion = namedtuple('Motion', 'mdata frames bits interpolation')
Texlist = namedtuple('Texlist', 'names count')
Action = namedtuple('Action', 'object motion')


class ReadError(Exception):
    """Bytes at an address of an image that are not what the caller named."""

    def __init__(self, data, address, problem):
        super().__init__(f'{data.name}: 0x{address:08x}: {problem}')


class Data(Memory):
    """Memory, read as the readers read it: each read raises ReadError where
    its bytes lie outside the image."""

    def inside(self, address, size=1):
        if size < 0:
            raise ReadError(self, address, f'{size} bytes, and a size can\'t be negative')
        return self.base <= address and address + size <= self.end

    def need(self, address, size, alignment, what):
        """Raise ReadError, naming what, unless size bytes at address lie in
        the image and address is aligned to alignment."""
        if address % alignment:
            raise ReadError(self, address, f'can\'t be {what}, not aligned to {alignment}')
        if not self.inside(address, size):
            raise ReadError(self, address, f'can\'t be {what}, whose {size} bytes lie outside the image')

    def unpack(self, code, address, what):
        """The values struct code reads at address. Raises ReadError, naming
        what, where their bytes lie outside the image."""
        self.need(address, struct.calcsize(code), 1, what)
        return struct.unpack_from(code, self.payload, address - self.base)

    def u16(self, address):
        return self.unpack('<H', address, 'a Uint16')[0]

    def s16(self, address):
        return self.unpack('<h', address, 'an Sint16')[0]

    def u32(self, address):
        return self.unpack('<I', address, 'a Uint32')[0]

    def s32(self, address):
        return self.unpack('<i', address, 'an Sint32')[0]

    def words(self, address, count):
        return self.unpack(f'<{count}I', address, f'{count} words')


def object_at(data, address):
    data.need(address, OBJECT.size, OBJECT.alignment, 'an object')
    return Object(data.u32(address + OBJECT.evalflags), data.u32(address + OBJECT.model),
                  data.u32(address + OBJECT.child), data.u32(address + OBJECT.sibling))


def model_at(data, address):
    data.need(address, MODEL.size, MODEL.alignment, 'a model')
    return Model(data.u32(address + MODEL.points), data.u32(address + MODEL.normals),
                 data.u32(address + MODEL.point_count), data.u32(address + MODEL.meshsets),
                 data.u32(address + MODEL.materials), data.u16(address + MODEL.meshset_count),
                 data.u16(address + MODEL.material_count))


def model_arrays(model):
    """The arrays a model points at, as (kind, start, size), size in bytes:
    its points and its normals, point_count of each, then its materials and
    its meshsets. A null pointer gives no array."""
    arrays = [('points', model.points, model.point_count), ('normals', model.normals, model.point_count),
              ('material', model.materials, model.material_count), ('meshset', model.meshsets, model.meshset_count)]
    return [(kind, start, KINDS[kind].layout.size * count) for kind, start, count in arrays if start]


def meshset_at(data, address):
    data.need(address, MESHSET.size, MESHSET.alignment, 'a meshset')
    return Meshset(data.u16(address + MESHSET.type_material) >> 14, data.u16(address + MESHSET.mesh_count),
                   data.u32(address + MESHSET.strips), data.u32(address + MESHSET.attrs),
                   data.u32(address + MESHSET.normals), data.u32(address + MESHSET.colors),
                   data.u32(address + MESHSET.uvs))


def strips_end(data, meshset):
    """Where a meshset's strips end, and its total of vertices. Each mesh of
    a meshset of triangles has 3 vertices, and of quads 4. An n-gon or a strip
    opens with its count of vertices in the low 14 bits of a Uint16. Raises
    ReadError where such a count lies outside the image or off 2."""
    if meshset.polygon in (0, 1):
        total = meshset.mesh_count * (3 if meshset.polygon == 0 else 4)
        return meshset.strips + 2 * total, total
    end, total = meshset.strips, 0
    for _ in range(meshset.mesh_count):
        data.need(end, 2, 2, 'a strip')
        count = data.u16(end) & 0x3FFF
        end += 2 + 2 * count
        total += count
    return end, total


def meshset_arrays(meshset, end, total):
    """The arrays a meshset points at, as (kind, start, size), size in bytes:
    its strips up to end, an attribute for each mesh, and a normal, a color
    and a uv for each of its total vertices. A null pointer gives no
    array."""
    counted = [('attrs', meshset.attrs, meshset.mesh_count), ('normals', meshset.normals, total),
               ('vcolor', meshset.colors, total), ('uv', meshset.uvs, total)]
    arrays = [('strips', meshset.strips, end - meshset.strips),
              *((kind, start, KINDS[kind].layout.size * count) for kind, start, count in counted)]
    return [(kind, start, size) for kind, start, size in arrays if start]


def motion_at(data, address):
    data.need(address, MOTION.size, MOTION.alignment, 'a motion')
    kind = data.u16(address + MOTION.bits)
    if kind & ~0x3FFF:
        raise ReadError(data, address, f'can\'t be a motion, with type bits 0x{kind & ~0x3FFF:04x} '
                                       f'Ninja has no keys for')
    bits = tuple(bit for bit in KEYS if kind & bit)
    if not 1 <= len(bits) <= 4:
        raise ReadError(data, address, f'can\'t be a motion, with {len(bits)} factors where ninja.h types one to four')
    return Motion(data.u32(address + MOTION.mdata), data.u32(address + MOTION.frames), bits,
                  data.u16(address + MOTION.interpolation))


def mdata_keys(data, motion, entry):
    """One node's key arrays in a motion's mdata, as (bit, pointer, count)."""
    factors = len(motion.bits)
    mdata = MDATA[factors]
    data.need(entry, mdata.size, mdata.alignment, 'an mdata entry')
    pointers = data.words(entry + mdata.pointers, factors)
    counts = data.words(entry + mdata.counts, factors)
    return list(zip(motion.bits, pointers, counts))


def texlist_at(data, address):
    data.need(address, TEXLIST.size, TEXLIST.alignment, 'a texture list')
    return Texlist(data.u32(address + TEXLIST.names), data.u32(address + TEXLIST.count))


def action_at(data, address):
    data.need(address, ACTION.size, ACTION.alignment, 'an action')
    return Action(data.u32(address + ACTION.object), data.u32(address + ACTION.motion))


# The recognizer
#
# Find Ninja's structures in a range of data that no source outside gen/
# defines. The readers decode a structure at an address the caller names; the
# recognizer judges which addresses hold one. Its judgments are heuristics:
#
# - plausibility: counts within bounds, float values, and pointers that miss
#   the code;
# - extents the bytes don't give: an mdata takes at most an entry for each
#   node of its action's tree, or without one ends as a vertex array does,
#   at the next address the image points at or the caller names.
#
# The recognizer keeps a structure whose arrays fail those judgments without
# them. It covers the bytes no structure claims with items of the GENERIC
# kinds, or strings. A structure a reader raises ReadError on is one the
# recognizer doesn't take: it raises no error of its own.
#
# TOOLCHAIN.md#generated-ranges gives the limits of these inferences.


# the kind of each motion key ninja.h types, by its type bit: the kind whose
# layout in KINDS is the bit's in KEYS
KEY_KINDS = {bit: name for bit, key in KEYS.items() for name, kind in KINDS.items() if kind.layout is key}


def pairs(values):
    values = list(values)
    return zip(values, values[1:])


def nice_float(word):
    """Whether word is zero, or a normal float whose magnitude lies between
    1e-6 and 1e7. The range is the recognizer's guess at game data's floats,
    not a bound the data keeps."""
    if word & 0x7FFFFFFF == 0:
        return True
    return float_ok(word) and 1e-6 < abs(f32(word)) < 1e7


class Image(Data):
    """An image as the recognizer reads it: Data, with the linker's fill byte
    and with the tests the recognizer's judgments make of an address."""

    def __init__(self, name, payload, base, code, windows, fill=0):
        super().__init__(name, payload, base, code, windows)
        self.fill = fill  # the linker's byte where the compiler writes none

    def aligned_in_data(self, value, alignment=4):
        """Whether value is aligned, in the image, and not in its code."""
        return self.base <= value < self.end and value % alignment == 0 and not self.in_code(value)

    def string(self, address):
        """The text of 1 to 255 printable ASCII bytes and a null at address, or
        None."""
        if not self.inside(address):
            return None
        offset = address - self.base
        end = self.payload.find(b'\0', offset)
        if end < 0 or end == offset or end - offset > 255:
            return None
        raw = self.payload[offset:end]
        if any(not 0x20 <= byte < 0x7F for byte in raw):
            return None
        return raw.decode('ascii')


def references(image, low, high):
    """Every value from low to high that a 4-aligned word of the image
    holds."""
    words = array('I')
    words.frombytes(image.payload[:len(image.payload) // 4 * 4])
    if sys.byteorder != 'little':
        words.byteswap()
    return sorted({word for word in words if low <= word < high})


class Recognizer:
    """Find Ninja's structures in an image between low and high.

    The recognizer assumes that a structure lies after the arrays it points
    at: points before models, models before objects, keys and mdata before
    motions. So it tries the addresses the image points at, and those the
    caller names, from the top down. Each structure takes its arrays before
    they can pass for something else, as points can look like an object, and
    mdata like a model.
    """

    def __init__(self, image, low, high, named=()):
        self.image = image
        self.low, self.high = low, high
        self.items = {}
        # the addresses from low to high a word of the image holds, and the
        # named ones
        self.refs = sorted(set(references(image, low, high)) | set(named))
        self.ref_set = set(self.refs)
        self.named = sorted(named)  # the addresses the caller names

    def floats_ok(self, address, count):
        return all(float_ok(word) for word in self.image.words(address, count))

    def model_ok(self, address):
        image = self.image
        try:
            model = model_at(image, address)
        except ReadError:
            return False
        return (image.aligned_in_data(model.points)
                and (model.normals == 0 or image.aligned_in_data(model.normals))
                and 0 < model.point_count < 0x10000
                and (model.meshsets == 0 or image.aligned_in_data(model.meshsets))
                and (model.materials == 0 or image.aligned_in_data(model.materials))
                and model.meshset_count <= 2000 and model.material_count <= 2000
                and bool(model.meshsets) == bool(model.meshset_count)
                and bool(model.materials) == bool(model.material_count)
                and self.floats_ok(address + MODEL.center, 4)
                and f32(image.u32(address + MODEL.radius)) >= 0)

    def object_ok(self, address):
        image = self.image
        try:
            node = object_at(image, address)
        except ReadError:
            return False
        if not any(image.words(address, OBJECT.size // 4)) or node.evalflags >= 0x10000:
            return False
        if node.model and not self.model_ok(node.model):
            return False
        if not self.floats_ok(address + OBJECT.position, 3) or not self.floats_ok(address + OBJECT.scale, 3):
            return False
        if any(not -0x1000000 < image.s32(address + OBJECT.angle + 4 * i) < 0x1000000 for i in range(3)):
            return False
        return all(value == 0 or image.aligned_in_data(value) for value in (node.child, node.sibling))

    def motion_ok(self, address):
        image = self.image
        try:
            motion = motion_at(image, address)
        except ReadError:
            return False
        # interpolation holds the factor count in its low four bits, or 0 there
        return (image.aligned_in_data(motion.mdata) and 1 <= motion.frames <= 0x10000
                and (motion.interpolation & 0xF) in (0, len(motion.bits)) and self.mdata_ok(motion, motion.mdata))

    def keys_ok(self, address, count, frames, size):
        image = self.image
        if not image.aligned_in_data(address) or not 0 < count <= 100000:
            return False
        if not image.inside(address, count * size):
            return False
        previous = -1
        for index in range(count):
            frame = image.u32(address + index * size)
            if frame <= previous or frame > frames:
                return False
            previous = frame
        return True

    def mdata_ok(self, motion, entry):
        """Whether a node's entry in the motion's mdata names only plausible
        key arrays."""
        try:
            keys = mdata_keys(self.image, motion, entry)
        except ReadError:
            return False
        return all(self.keys_ok(pointer, count, motion.frames, KEYS[bit].size)
                   for bit, pointer, count in keys if pointer or count)

    def texlist_ok(self, address):
        image = self.image
        try:
            names, count = texlist_at(image, address)
        except ReadError:
            return False
        if not image.aligned_in_data(names) or not 1 <= count <= 512 or not image.inside(names, TEXNAME.size * count):
            return False
        # a pvm fills in some lists' names as it loads: they lie zeroed, just
        # before their list
        if names + TEXNAME.size * count == address and not any(image.payload[names - image.base:address - image.base]):
            return True
        return all(image.string(image.u32(name + TEXNAME.name)) is not None and image.u32(name + TEXNAME.address) == 0
                   for name in range(names, names + TEXNAME.size * count, TEXNAME.size))

    def action_ok(self, address):
        try:
            action = action_at(self.image, address)
        except ReadError:
            return False
        return self.object_ok(action.object) and self.motion_ok(action.motion)

    def path_ok(self, address):
        """Whether address plausibly holds a stage's path, as src/path/path.h
        has it: its ends, its count of points and its length, the points, and
        its task's step."""
        image = self.image
        if address % STAGE_PATH.alignment or not image.inside(address, STAGE_PATH.size) \
                or image.payload[address + STAGE_PATH.ends - image.base] > 1:
            return False
        count = image.u16(address + STAGE_PATH.count)
        length, points, step = (image.u32(address + offset)
                                for offset in (STAGE_PATH.length, STAGE_PATH.points, STAGE_PATH.step))
        return (1 < count < 0x4000 and nice_float(length) and f32(length) > 0
                and image.aligned_in_data(points) and image.inside(points, STAGE_PATH_POINT.size * count)
                and (step == 0 or image.in_code(step))
                and all(self.floats_ok(point + STAGE_PATH_POINT.floats, 4)
                        for point in range(points, points + STAGE_PATH_POINT.size * count, STAGE_PATH_POINT.size)))

    def add(self, item):
        """Whether the item is taken: now, or as the same item before.

        A new item is taken where it lies between low and high, starts
        aligned and spans whole elements, and where any named address after
        its start and before its end lies between two of an array's elements.
        It then goes into self.items."""
        if not self.low <= item.start < item.end <= self.high:
            return False
        if item.start % item.alignment or (item.end - item.start) % item.element:
            return False
        # a named address starts a definition of its own: no record spans one,
        # and an array splits only between its elements
        inside = self.named[bisect_right(self.named, item.start):bisect_left(self.named, item.end)]
        if any(item.single or (address - item.start) % item.element for address in inside):
            return False
        existing = self.items.get(item.start)
        if existing is not None:
            return existing.kind == item.kind and existing.end == item.end
        self.items[item.start] = item
        return True

    def next_ref(self, address):
        """The least of refs above address, or high."""
        later = bisect_right(self.refs, address)
        return self.refs[later] if later < len(self.refs) else self.high

    def tree(self, root):
        """The addresses of a tree's objects, depth first from root, the order
        its motions take them in. An object object_ok() refuses is left out,
        with every object reached only through it. Each object comes once, and
        the walk stops after 1024 objects."""
        order, pending, seen = [], [root], set()
        while pending and len(order) < 1024:
            address = pending.pop()
            if address in seen or not self.object_ok(address):
                continue
            seen.add(address)
            order.append(address)
            node = object_at(self.image, address)
            if node.sibling:
                pending.append(node.sibling)
            if node.child:
                pending.append(node.child)
        return order

    def take_object(self, address):
        for node in self.tree(address):
            self.add(Item(node, node + OBJECT.size, 'object'))
            model = object_at(self.image, node).model
            if model:
                self.take_model(model)

    def take_model(self, address):
        image = self.image
        if not self.model_ok(address) or not self.add(Item(address, address + MODEL.size, 'model')):
            return
        model = model_at(image, address)
        # an array is kept only where it lies in the image and, but for the
        # meshsets, passes fits(). The meshsets' arrays are kept only where
        # the model's meshsets are
        for kind, start, size in model_arrays(model):
            if not image.inside(start, size):
                continue
            if kind != 'meshset':
                if fits(image, kind, start, start + size):
                    self.add(Item(start, start + size, kind))
            elif self.add(Item(start, start + size, kind)):
                for index in range(model.meshset_count):
                    self.take_meshset(start + MESHSET.size * index)

    def take_meshset(self, address):
        image = self.image
        meshset = meshset_at(image, address)
        if not image.aligned_in_data(meshset.strips, 2):
            return
        try:
            end, total = strips_end(image, meshset)
        except ReadError:
            return
        if not image.inside(meshset.strips, end - meshset.strips):
            return
        # the strips are kept whole, normals where their floats pass
        # float_ok(), and the rest where they point into the image's data,
        # aligned
        for kind, start, size in meshset_arrays(meshset, end, total):
            if kind == 'strips':
                self.add(Item(start, start + size, kind))
            elif kind == 'normals':
                if image.aligned_in_data(start) and image.inside(start, size) and self.floats_ok(start, size // 4):
                    self.add(Item(start, start + size, kind))
            elif image.aligned_in_data(start):
                self.add(Item(start, start + size, kind))

    def take_motion(self, address, nodes=None):
        image = self.image
        if not self.motion_ok(address) or not self.add(Item(address, address + MOTION.size, 'motion')):
            return
        motion = motion_at(image, address)
        mdata, factors = motion.mdata, len(motion.bits)
        size = MDATA[factors].size
        # no field counts the mdata's entries: they end before the first entry
        # that isn't plausible, or at the end of the range or the image, and
        # an action's tree takes at most one entry for each of its nodes. With
        # no tree, they end at the next of refs. A ref doesn't cut a tree's
        # entries short: any word can make one, as a glyph in 1ST_READ.BIN's
        # font does
        limit = min(self.high if nodes is not None else self.next_ref(mdata), image.end)
        count = 0
        while (nodes is None or count < nodes) and mdata + size * (count + 1) <= limit \
                and self.mdata_ok(motion, mdata + size * count):
            count += 1
        if not count or not self.add(Item(mdata, mdata + size * count, 'mdata', factors)):
            return
        for index in range(count):
            for bit, pointer, keys in mdata_keys(image, motion, mdata + size * index):
                kind = KEY_KINDS.get(bit)
                if not pointer or kind is None:
                    continue
                if kind == 'mkey_f' and not all(self.floats_ok(key + MKEY_F.key, 3)
                                                for key in range(pointer, pointer + MKEY_F.size * keys, MKEY_F.size)):
                    continue
                self.add(Item(pointer, pointer + KEYS[bit].size * keys, kind))
                if kind == 'mkey_p':
                    for key in range(pointer, pointer + MKEY_P.size * keys, MKEY_P.size):
                        self.take_vertices(image.u32(key + MKEY_P.key))

    def take_vertices(self, address):
        if not self.image.aligned_in_data(address) or not self.low <= address < self.high:
            return
        end = min(self.next_ref(address), self.high)
        end = address + (end - address) // POINT.size * POINT.size
        if end > address and self.floats_ok(address, (end - address) // 4):
            self.add(Item(address, end, 'vertices'))

    def take_texlist(self, address):
        if not self.texlist_ok(address) or not self.add(Item(address, address + TEXLIST.size, 'texlist')):
            return
        texlist = texlist_at(self.image, address)
        self.add(Item(texlist.names, texlist.names + TEXNAME.size * texlist.count, 'texname'))

    def take_action(self, address):
        if not self.action_ok(address) or not self.add(Item(address, address + ACTION.size, 'action')):
            return
        action = action_at(self.image, address)
        self.take_object(action.object)
        self.take_motion(action.motion, len(self.tree(action.object)))

    def recognize(self):
        """The items between low and high, in address order: the structures
        found with their arrays, and generic()'s items for the bytes between
        them, as cover() gives them.

        It fills self.items as it takes structures. In address order, an item
        that starts inside the last one kept is dropped."""
        for ref in reversed(self.refs):
            if ref in self.items or ref % 4:
                continue
            if self.texlist_ok(ref):
                self.take_texlist(ref)
            elif self.action_ok(ref):
                self.take_action(ref)
            elif self.motion_ok(ref):
                self.take_motion(ref)
            elif self.model_ok(ref):
                self.take_model(ref)
            elif self.object_ok(ref):
                self.take_object(ref)
        accepted = []
        for item in sorted(self.items.values(), key=lambda item: item.start):
            if not accepted or item.start >= accepted[-1].end:
                accepted.append(item)
        return self.cover(accepted)

    def cover(self, items):
        """The items, and generic()'s items for the bytes between low and high
        that no item covers, in address order. The items must be in address
        order, none overlapping. A gap that is exactly the padding up to the
        next item's alignment, and holds only the fill byte, gets none: the
        compiler reserves it, and the linker fills it."""
        result, cursor = [], self.low
        for item in [*items, None]:
            stop = self.high if item is None else item.start
            padding = 0 if item is None else (-cursor) % item.alignment
            gap = self.image.payload[cursor - self.image.base:stop - self.image.base]
            if not (stop - cursor == padding and set(gap) <= {self.image.fill}):
                result.extend(self.generic(cursor, stop))
            if item is not None:
                result.append(item)
                cursor = item.end
        return result

    def generic(self, start, end):
        """Items of the GENERIC kinds, and strings, from start to end, cut at
        each of refs. A piece that starts with a string last_string() reads is
        that string. Otherwise it is bytes or a half up to 4-byte alignment, a
        word_item() for its whole words, and halves and bytes after them."""
        inside = self.refs[bisect_right(self.refs, start):bisect_left(self.refs, end)]
        result = []
        for low, high in pairs(sorted({start, end, *inside})):
            cursor = low
            while cursor < high:
                remaining = high - cursor
                text = self.last_string(cursor, high)
                if text is not None:
                    result.append(Item(cursor, high, 'string', text=text))
                    break
                if cursor % 2 or remaining == 1:
                    step = min(remaining, (-cursor) % 4 or 1)
                    result.append(Item(cursor, cursor + step, 'bytes'))
                elif cursor % 4 or remaining < 4:
                    step = 2
                    result.append(Item(cursor, cursor + step, 'halves'))
                else:
                    step = remaining // 4 * 4
                    result.append(self.word_item(cursor, cursor + step))
                cursor += step
        return result

    def last_string(self, start, end):
        """The text of a string from start to end, or None. The string starts
        on 4 at one of refs, and zeros follow its null to an end off 4, as a
        row's last string does where the row ends with it. word_item() reads
        a string that ends on 4."""
        image = self.image
        if start % 4 or not end % 4 or start not in self.ref_set:
            return None
        text = image.string(start)
        if text is None or start + len(text) + 1 > end or any(
                image.payload[start + len(text) - image.base:end - image.base]):
            return None
        return text

    def word_item(self, start, end):
        """An item for the whole words from start to end, of the first of these
        kinds that fits:

        - a string, where one starts at one of refs and zeros follow its null
          to end;
        - pointers or mixed, where a word lies in an address window;
        - floats, where some word isn't zero and each passes nice_float(),
          whose range is a guess;
        - words."""
        image = self.image
        text = image.string(start) if start in self.ref_set else None
        if text is not None and start + len(text) + 1 <= end and not any(
                image.payload[start + len(text) - image.base:end - image.base]):
            return Item(start, end, 'string', text=text)
        words = image.words(start, (end - start) // 4)
        addresses = [image.is_address(word) for word in words]
        if any(addresses):
            # an array whose every word is an address or zero holds
            # pointers, and one that holds other numbers too is mixed
            typed = all(address or not word for address, word in zip(addresses, words))
            return Item(start, end, 'pointers' if typed else 'mixed')
        if any(words) and all(nice_float(word) for word in words):
            return Item(start, end, 'floats')
        return Item(start, end, 'words')


def fits(image, kind, start, end):
    """Whether the bytes from start to end pass the kind's checks of shape:
    its floats pass float_ok(), and its pointers are null or point into main
    RAM, whatever their top three bits. The emitter can still refuse a value
    that passes. Any bytes pass as an mkey_a or a texanim, which hold only
    integers, and none as a kind without a rule here, such as strips."""
    def pointers(*offsets):
        return all(word == 0 or 0x0C000000 <= word & 0x1FFFFFFF < 0x0D000000
                   for word in (image.u32(start + offset) for offset in offsets))

    def floats(offset, count, step=0):
        return all(float_ok(word) for address in range(start, end, step or end - start)
                   for word in image.words(address + offset, count))

    if kind == 'object':
        return (pointers(OBJECT.model, OBJECT.child, OBJECT.sibling) and floats(OBJECT.position, 3)
                and floats(OBJECT.scale, 3))
    if kind == 'model':
        return pointers(MODEL.points, MODEL.normals, MODEL.meshsets, MODEL.materials) and floats(MODEL.center, 4)
    if kind == 'motion':
        return pointers(MOTION.mdata)
    if kind == 'texlist':
        return pointers(TEXLIST.names)
    if kind == 'action':
        return pointers(ACTION.object, ACTION.motion)
    if kind == 'caction':
        return pointers(CACTION.camera, CACTION.motion)
    if kind == 'texname':
        return all(pointers(address - start + TEXNAME.name) for address in range(start, end, TEXNAME.size))
    if kind == 'meshset':
        members = (MESHSET.strips, MESHSET.attrs, MESHSET.normals, MESHSET.colors, MESHSET.uvs)
        return all(pointers(*(address - start + offset for offset in members))
                   for address in range(start, end, MESHSET.size))
    if kind == 'material':
        return floats(MATERIAL.exponent, 1, MATERIAL.size)
    if kind in ('points', 'normals'):
        return floats(0, (end - start) // 4)
    if kind == 'mkey_f':
        return floats(MKEY_F.key, 3, MKEY_F.size)
    return kind in ('mkey_a', 'texanim')


# The commands
#
# The commands the docstring lists, and the command line that picks one. Only
# this section reads files or the tree's config; the sections above take the
# text and bytes it hands them. A command fails through config's fail, naming
# the file at fault, as main() does on an error from a section above or on a
# file a command can't read or write.


FIRST_CODE = tuple(sorted(FIRST_GAME_CODE + FIRST_SDK_CODE))
RAM = 0x1000000  # main RAM's size
# the sections a gen/ row can lie in. Each gives the name a row's C passes
# to #pragma section there, or None for no pragma. It also gives whether
# that C defines its data const. SHC writes data that isn't const to D, or
# to DHEAD under #pragma section HEAD, and const data to C
GEN_SECTIONS = {'D': (None, False), 'DHEAD': ('HEAD', False), 'C': (None, True)}
# --rows starts a gap's next row at the first 8-byte-aligned datum at least
# this far past the last row's start, not inside an extern variable, and
# with a quarter of this or more left in the gap
ROW_LENGTH = 0x40000

# an extern declaration of a type named by one identifier, const or not,
# with one extent or none: its const, type, name and extent. --declared
# types generated data from these. A declaration of a pointer or a volatile,
# or one with two extents, matches neither this nor BODY_EXTERN. --declared
# gives its name no type, and a body can't point at it
EXTERN = re.compile(r'^extern\s+(const\s+)?(\w+)\s+(\w+)\s*(\[[^]]*\])?\s*;', re.M)
# as EXTERN, and also a declaration of a struct type such as struct
# land_entry: the declarations a body can point at. --declared keeps to
# EXTERN, as the emitter lays out generated data only in the types DECLARED
# gives a kind, and no struct type is one of them
BODY_EXTERN = re.compile(r'^extern\s+(const\s+)?((?:struct\s+)?\w+)\s+(\w+)\s*(\[[^]]*\])?\s*;', re.M)
# a function's prototype at the start of a line: its name. A body can point
# at a function the source or its headers declare, as a path's step does. A
# parameter can be a function pointer, whose parentheses nest one deep
PROTOTYPE = re.compile(r'^(?:extern\s+)?[A-Za-z_][\w \t]*?[\s*](\w+)\((?:[^;{()]|\([^;{()]*\))*\);', re.M)
# an extern declaration of a pointer, or of an integer or float of a type
# listed here, that is no array: its type and name. --rows keeps the
# variable's bytes out of the rows it proposes
VARIABLE = re.compile(r'^extern\s+(?:(?:const|volatile)\s+)*((?:struct\s+)?\w+(?:\s*\*)+|(?:unsigned\s+)?'
                      r'(?:int|short|long|char|[SU]int(?:8|16|32)|Float|float|Angle|Bool))\s*(\w+)\s*;', re.M)
# integer types narrower than a word, and their sizes
SHORT = {'char': 1, 'Sint8': 1, 'Uint8': 1, 'short': 2, 'Sint16': 2, 'Uint16': 2}
# the kind of a body, by the type its source declares it with. A Sint16 body
# takes the kind of a meshset's strips, written a strip to a line where its
# values make strips
BODY_KINDS = {**DECLARED, 'NJS_COLOR': 'vcolor', 'NJS_MKEY_P': 'mkey_p', 'Sint16': 'strips'}
# the SDK's integer types: each one's struct code, and how many go on a line
INTEGERS = {'Sint8': ('b', 16), 'Uint8': ('B', 12), 'Sint16': ('h', 12), 'Uint16': ('H', 8),
            'Sint32': ('i', 8), 'Uint32': ('I', 6)}
# the scalar types a body can have that need no header to lay out, and each
# one's struct code
SCALARS = {**{ctype: code for ctype, (code, _) in INTEGERS.items()}, 'Float': 'f', 'char': 'c'}
# how many of a laid-out body's scalars go on a line, by struct code
LINE = {**dict(INTEGERS.values()), 'f': 6, 'c': 12}
# the bytes in a listing directive's .B, .W or .L unit
UNITS = {'B': 1, 'W': 2, 'L': 4}
# the banner asmsh's listing opens with, after a form feed
ASSEMBLED = re.compile(r'\f?\*\*\* SuperH RISC engine ASSEMBLER')
# a declaration's qualifiers
QUALIFIED = r'((?:(?:const|volatile)\s+)*)'
# a file's datum or a function's static whose initializer is a body, an
# included .inc: its qualifiers, type, pointer's *, name, extents and include
BODY = re.compile(r'^(?:[ \t]+static\s+|(?:static\s+)?)' + QUALIFIED + r'((?:struct\s+)?\w+)(?:\s*(\*)\s*|\s+)'
                  r'(\w+)\s*((?:\[[^]\n]*\])*)\s*=\s*\{\n#include "([^"]+)"\n[ \t]*\};', re.M)
# a function's automatic array whose initializer is a body too: its
# qualifiers, type, name, extents and include. Its body is the template SHC
# copies the array from
AUTO = re.compile(r'^[ \t]+(?!static\b)' + QUALIFIED + r'((?:struct\s+)?\w+)\s+(\w+)\s*((?:\[[^]\n]*\])+)\s*=\s*'
                  r'\{\n#include "([^"]+)"\n[ \t]*\};', re.M)
# an included body
INCLUDE = re.compile(r'^#include "([^"]+\.inc)"$', re.M)
# a declaration that defines a union: the union's members and the datum's name
UNION = re.compile(r'^(?:static\s+)?(?:(?:const|volatile)\s+)*union\s*\w*\s*\{\n(.*?)^\}\s*(\w+)\s*;', re.M | re.S)
# a union's first member, where it is an array: its element's type and its name
MEMBER = re.compile(r'\s*((?:struct\s+)?\w+)\s+(\w+)\s*\[')
# a file's declaration that defines a struct of no tag: the name of the datum
# it declares, where the datum is no array
UNTAGGED = re.compile(r'^(?:static\s+)?(?:(?:const|volatile)\s+)*struct\s*\{\n'
                      r'(?:(?!\}).*\n)*\}[ \t]*(\w+)[ \t]*[=;]', re.M)
# an array declaration's extents
EXTENT = re.compile(r'\[([^]\n]*)\]')
# a function definition, from the start of its line to its brace: its name
FUNCTION = re.compile(r'^(?:static\s+)?[A-Za-z_][\w \t*]*?\b(\w+)\([^;{]*\)\n\{', re.M)
# a file's declaration or a function's static: its qualifiers, type,
# pointer's *, name and extents
DECLARATION = re.compile(r'^(?:[ \t]+static\s+|(?:static\s+)?)' + QUALIFIED + r'((?:struct\s+)?\w+)\s*(\*?)\s*(\w+)\s*'
                         r'((?:\[[^]\n]*\])*)\s*[=;]', re.M)
# a text's length macro: the text's name, number and length
TEXT = re.compile(r'^#define (TEXT_([0-9]+))_LENGTH ([0-9]+)$', re.M)
# a value, VALUE_n, and its number n. A value stands alone for one arithmetic
# member or element in an initializer the source writes, and the full build's
# values.inc defines it as the number the disc holds there
VALUE = re.compile(r'\bVALUE_([0-9]+)\b')
# a definition whose initializer the source writes in braces, up to the
# brace: its qualifiers, type, name and extents
WRITTEN = re.compile(r'^[ \t]*(?:static\s+)?' + QUALIFIED + r'((?:struct\s+|union\s+)?\w+)\s+(\w+)\s*'
                     r'((?:\[[^]\n]*\])*)\s*=\s*(?=\{)', re.M)
# in a listing: a static's label, a datum's directive and its operands, the
# function a row of code begins, the labels a row names, and a row of an L
# label alone, as an unnamed datum or a string starts with. A .SDATA's
# operand is its string in quotes, which can hold a ;. Any other directive's
# operands run to the ; that starts a comment
STATIC = re.compile(r'\w+: +; static: (\w+)')
DIRECTIVE = re.compile(r'(\.(?:DATA|DATAB|RES|SDATA)(?:\.[BWL])?) +("(?:[^"]|"")*"|[^;]*)')
BEGINS = re.compile(r'; function: (\w+)')
LABELS = re.compile(r'\bL[0-9]+\b')
UNNAMED = re.compile(r'L[0-9]+: *')
# in SHC's listing: a section's size, a row's section, offset and the rest,
# and a row that goes on with the operands above it
SHC_SECTION = re.compile(r'\w+ +SECTION \((\w+)\): +([0-9A-F]+) Byte')
SHC_ROW = re.compile(r'(...) ([0-9A-F]{8}) [0-9A-F<> ]{10} *(.*)$')
SHC_MORE = re.compile(r' {13}[0-9A-F<> ]{10} *\+ +([^;]*)')
# in SHC's listing, the label of a global whose name has 31 characters or
# more, and its comment. The label runs past the comment's column, so the
# comment goes on the next line
SHC_WRAPPED = re.compile(r'^(.* _\w+:)\n +(; (?:static|function): \w+)$', re.M)

# each command's command line, which -h and misuse print after "usage: "
USAGE = '''%(prog)s IMAGE SOURCE OUT
       %(prog)s --declared CONFIG OUT
       %(prog)s --rows IMAGE
       %(prog)s --bodies IMAGE SOURCE LISTING OUT CONFIG
       %(prog)s --texts SOURCE OUT
       %(prog)s --values SOURCE OUT'''


def cut(items, names, source):
    """The items, each cut at every name inside it, so that the C defines
    every name. A name inside a record or a string, or inside an element of
    an array, fails, naming source."""
    result, order = [], sorted(names)
    for item in items:
        inside = order[bisect_right(order, item.start):bisect_left(order, item.end)]
        if inside and (item.single or item.kind == 'string'
                       or any((address - item.start) % item.element for address in inside)):
            fail(f'{source}: {names[inside[0]]}, at 0x{inside[0]:08x}, falls inside {item.ctype} 0x{item.start:08x}')
        result.extend(item.part(start, end) for start, end in pairs([item.start, *inside, item.end]))
    return result


def declare(image, items, names, declared):
    """The items, typed as declared.txt types their names, where the emitter
    has a kind for a name's type and the item at the name can take it.

    An item that has the type already takes it as it is. Generic data takes
    it where the kind fits, from the generic items that run unbroken from the
    name up to the next name. A record takes one element of them. An array
    runs to the first of their ends that falls on an element, or else covers
    their whole elements. The rest of the last item stays generic, and must
    keep its alignment and whole elements. Where any of this fails, the
    name's items stay as they were.

    A typed item of one element is an array only where declared.txt gives its
    name an extent. The list passed in is left as it was, but an item that
    has the type already is typed in place."""
    result = list(items)
    starts = [item.start for item in result]
    for address in sorted(names):
        if names[address] not in declared:
            continue
        ctype, array = declared[names[address]]
        kind = DECLARED.get(ctype)
        index = bisect_left(starts, address)
        if kind is None or index == len(result) or starts[index] != address:
            continue
        item = result[index]
        same = SAME.get(ctype, ctype) == SAME.get(item.ctype, item.ctype)
        if not same:
            element, single = KINDS[kind].layout.size, KINDS[kind].single
            run = []
            for other in result[index:]:
                if other.kind not in GENERIC or other.start != (run[-1].end if run else address) \
                        or (run and other.start in names):
                    break
                run.append(other)
            if not run:
                continue
            ends = [other.end for other in run if (other.end - address) % element == 0]
            end = address + element if single else ends[0] if ends else \
                address + (run[-1].end - address) // element * element
            covered = [other for other in run if other.start < end]
            last = covered[-1] if covered else None
            tail = [last.part(end, last.end)] if last and end < last.end else []
            if (end <= address or end > run[-1].end or not fits(image, kind, address, end)
                    or any(part.start % part.alignment or (part.end - part.start) % part.element for part in tail)):
                continue
            item = Item(address, end, kind)
            result[index:index + len(covered)] = [item, *tail]
            starts[index:index + len(covered)] = [item.start, *(part.start for part in tail)]
        item.declared = ctype
        if item.end - item.start == item.element:
            item.array = array
    return result


def window(base):
    """Main RAM's range, low and end, in the mirror that base lies in. A word
    of the image's data is taken for an address where its value lies here."""
    low = base & ~(RAM - 1)
    return low, low + RAM


class Config(config.Config):
    """config.Config, with the first name at each address, the data --rows divides, and the image as an Image."""

    def __init__(self, name):
        super().__init__(name)
        self.names = {}
        for address, symbol in self.symbols():
            self.names.setdefault(address, symbol)
        # the data --rows divides: 1ST_READ.BIN's game data, or another image's D
        if self.path == FIRST:
            self.data = FIRST_GAME_DATA
        else:
            self.data = next(((start, end) for name, start, end in self.sections if name == 'D'), None)

    def generated(self):
        """Each gen/ row's start and end."""
        return [(start, end) for source, _, start, end in self.placements if source.startswith('gen/')]

    def image(self, path):
        """The image at path as an Image, with the code ranges and fill byte
        config.py gives it. A word of the image is taken for an address where
        its value lies in window(base). An image other than 1ST_READ.BIN and
        those in ALONE points into 1ST_READ.BIN too. Its Image then also takes
        a value in 1ST_READ.BIN's range for an address, and 1ST_READ.BIN's
        code ranges join its own."""
        game, sdk = self.code()
        code, windows = (*game, *sdk), (window(self.base),)
        if self.path != FIRST and self.path not in ALONE:
            code += FIRST_CODE
            windows += ((FIRST_BASE, FIRST_END),)
        return Image(str(path), content(path), self.base, code, windows, self.fill)


def publish(path, text):
    """Replace the file at path with text whole, through a temporary file
    beside it, so that a reader finds the old text or the new. Missing
    directories are made. A file it can't write fails, naming it, and is
    left as it was."""
    temporary = path.with_name(path.name + '.tmp')
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        temporary.write_text(text)
        os.replace(temporary, path)
    except OSError as error:
        fail(f'{path}: {error.strerror}')
    finally:
        with contextlib.suppress(OSError):
            temporary.unlink()


def generate(path, source, out):
    """Write OUT, the C of SOURCE's gen/ range of IMAGE, and OUT's .syms.

    Fails where:

    - the tables give SOURCE no row, or several;
    - SOURCE's row lies in a section GEN_SECTIONS doesn't name, starts off 8
      bytes, or lies outside the image;
    - declared.txt is missing beside OUT;
    - cut() or the emitter fails."""
    out = Path(out)
    config = Config(Path(path).stem.lower())
    ranges = config.ranges(source)
    if len(ranges) != 1:
        fail(f'{source}: needs one row in {config.splits}')
    (section, low, high), = ranges
    if section not in GEN_SECTIONS:
        fail(f'{source}: its row lies in {section}, and the C it takes defines data for D, DHEAD or C')
    if low % 8:
        fail(f'{source}: R32 starts its data on 8 bytes, and 0x{low:08x} is {low % 8} past')
    image = config.image(path)
    if not image.inside(low, high - low):
        fail(f'{source}: 0x{low:08x} to 0x{high:08x} lies outside {path}, 0x{image.base:08x} to 0x{image.end:08x}')
    types = out.parent / 'declared.txt'
    if not types.exists():
        fail(f'{types}: missing, and tools/data.py --declared {config.name} writes it')

    declared = {name: (ctype, bool(dims)) for name, ctype, *dims in table(types)}
    named = {address: name for address, name in config.names.items() if low <= address < high}
    items = cut(Recognizer(image, low, high, named).recognize(), named, source)
    items = declare(image, items, named, declared)

    pragma, constant = GEN_SECTIONS[section]
    writer = Writer(image, items, config.names, constant)
    text = writer.source(f'{Path(path).name} from 0x{low:08x} to 0x{high:08x}, as tools/data.py reads it', pragma)
    known = set(config.names.values())
    placeholders = sorted((address, name) for name, (address, _) in writer.uses.items() if name not in known)
    # the .syms first, so that the C is no older than its .syms only once both
    # are written
    publish(out.with_suffix('.syms'), ''.join(f'0x{address:08x}  {name}\n' for address, name in placeholders))
    publish(out, text)


def variables(config):
    """The size in bytes of each extern variable VARIABLE reads in the .c and
    .h files under the image's directories, by its address in config.names.
    A name's first declaration gives its size."""
    sizes = {}
    for directory in directories(config):
        for path in sorted(directory.rglob('*.[ch]')):
            for ctype, name in VARIABLE.findall(uncommented(path.read_text(errors='replace'))):
                sizes.setdefault(name, 4 if '*' in ctype else SHORT.get(ctype.split()[-1], 4))
    return {address: sizes[name] for address, name in config.names.items() if name in sizes}


def externs(paths, pattern=EXTERN):
    """Each extern declaration that pattern reads in paths: its name, (its
    type, whether it has an extent), and where it is, as path:line: the
    declaration."""
    for path in paths:
        text = uncommented(path.read_text(errors='replace'))
        line, at = 1, 0
        for match in pattern.finditer(text):
            line, at = line + text.count('\n', at, match.start()), match.start()
            const, ctype, name, dims = match.groups()
            yield name, (('const ' if const else '') + ctype, bool(dims)), f'{path}:{line}: {match.group()}'


def agreed(declarations):
    """Each name's type and whether it is an array, from declarations as
    externs() yields them. A name is an array only where all its declarations
    give an extent. Two types for one name fail, naming each declaration
    that conflicts with the first."""
    types, arrays, problems = {}, {}, []
    for name, (ctype, array), where in declarations:
        first, there = types.setdefault(name, (ctype, where))
        if ctype != first:
            problems.append(f'{where} conflicts with {there}')
        arrays[name] = arrays.get(name, True) and array
    if problems:
        fail(*problems)
    return {name: (ctype, arrays[name]) for name, (ctype, _) in types.items()}


def declarations(name, out):
    """Write OUT, declared.txt: the type the extern declarations give each
    name in CONFIG's gen/ ranges, less its qualifiers, and [] for an array.
    A name takes its type from the first of CONFIG's directories whose .c
    and .h files declare it. OUT is written only where its bytes differ
    from the text's. Fails where one directory's declarations of a name
    disagree."""
    config, out = Config(name), Path(out)
    ranges = config.generated()
    wanted = {symbol for address, symbol in config.names.items() if any(low <= address < high for low, high in ranges)}
    found = {}
    for directory in directories(config):
        found.update(agreed(extern for extern in externs(sorted(directory.rglob('*.[ch]')))
                            if extern[0] in wanted and extern[0] not in found))
    text = ''.join(f'{symbol}  {unqualified(ctype)[1]}{"  []" if array else ""}\n'
                   for symbol, (ctype, array) in sorted(found.items()))
    if not out.exists() or content(out) != text.encode():
        publish(out, text)


def rows(path):
    """Print the gen/ rows proposed for IMAGE's data where no source outside
    gen/ holds it, and a # line saying why for each range of that data they
    leave out. That data lies in gaps, the stretches no such source's row
    covers. A gap of zeros alone or of fill alone gets no line, and neither
    does the linker's fill at a gap's ends. An image with no data to divide
    prints nothing. A file that ends before the end of the image's last
    section that isn't bss, as a compressed one does, fails."""
    config = Config(Path(path).stem.lower())
    image = config.image(path)
    stored = max(end for name, _, end in config.layout() if name not in ZEROED)
    if image.end < stored:
        fail(f'{path}: ends at 0x{image.end:08x}, short of the end of {config.name}\'s last section that isn\'t bss, '
             f'0x{stored:08x}; IMAGE is the image\'s file, uncompressed')
    if config.data is None:
        return
    first, last = config.data
    # a gen/ row owns nothing here: its data is proposed afresh. Every other D
    # placement owns the part of the data it covers. One can start before the
    # data, as 1ST_READ.BIN's game data starts inside D
    owned = sorted((max(first, start), min(last, end)) for source, section, start, end in config.placements
                   if section == 'D' and not source.startswith('gen/') and start < last and first < end)
    gaps, cursor = [], first
    for low, high in [*owned, (last, last)]:
        if low > cursor:
            gaps.append((cursor, low))
        cursor = max(cursor, high)
    # the extern variables in the gaps, with those that meet joined: no row
    # takes their bytes
    held = []
    for address, size in sorted(variables(config).items()):
        if any(low <= address < high for low, high in gaps):
            if held and held[-1][1] == address:
                held[-1][1] = address + size
            else:
                held.append([address, address + size])
    fill = bytes([image.fill])

    def skip(low, high, reason):
        print(f'# {config.name}: 0x{low:08x} to 0x{high:08x}: no generated row; {reason}')

    for low, high in gaps:
        payload = image.payload[low - image.base:high - image.base]
        if not payload.strip(b'\0') or not payload.strip(fill):
            continue
        # R32 starts its data on 8 bytes, so the fill before a gap's first
        # 8-byte boundary is the linker's. So are 4 bytes of fill before an
        # 8-byte end that no word points into
        while low % 8 and image.payload[low - image.base] == image.fill:
            low += 1
        if not high % 8 and not references(image, high - 4, high) and \
                image.payload[high - 4 - image.base:high - image.base] == fill * 4:
            high -= 4
        recognizer = Recognizer(image, low, high)
        items = recognizer.recognize()
        starts = [item.start for item in items if item.start % 8 == 0]
        if not starts:
            skip(low, high, 'no 8-byte-aligned datum')
            continue
        if starts[0] != low:
            skip(low, starts[0], 'no 8-byte-aligned datum')
        cuts = [starts[0]]
        for start in starts:
            if start - cuts[-1] >= ROW_LENGTH and high - start >= ROW_LENGTH // 4 and \
                    not any(a < start < b for a, b in held):
                cuts.append(start)
        # where a model, a motion or a stage path starts; a row needs one
        art = sorted([item.start for item in items if item.kind in ('model', 'motion')]
                     + [ref for ref in recognizer.refs if recognizer.path_ok(ref)])
        for start, end in pairs([*cuts, high]):
            if bisect_left(art, start) == bisect_left(art, end):
                skip(start, end, 'no recognized model, motion or path')
                continue
            for variable, after in [span for span in held if start <= span[0] < end] + [(end, end)]:
                if start < variable:
                    print(f'{f"gen/{config.name}/d_{start:08x}.c":38} D        0x{start:08x}  0x{variable:08x}')
                if variable == end:
                    break
                skip(variable, after, 'declared scalar variables')
                start = next((address for address in starts if after <= address < end), end)
                if after < start:
                    skip(after, start, 'no 8-byte-aligned datum')


def full_section(path, short, sizes):
    """The one section of a listing whose name starts with a row's three
    letters. Fails, naming path, where no section or several do."""
    full = [name for name in sizes if name[:3] == short]
    if len(full) != 1:
        fail(f'{path}: no one section starts {short}')
    return full[0]


def sdata(operand):
    """The bytes of a .SDATA's operand, as DIRECTIVE reads it: the string in
    its quotes, each doubled quote one quote. listing() sizes a string by
    them, and strings() takes them."""
    return operand.strip()[1:-1].replace('""', '"').encode('latin-1')


def listing(path):
    """From the stub's listing at path, SHC's or asmsh's for a patched source:

    - each named datum's section, offset and size, by its name;
    - the same for each unnamed datum outside the code, as an automatic
      array's template is, by its L label;
    - the L labels named in each function's code and pools, by the function;
    - the names two statics share.

    SHC's rows show at most three characters of a section's name, and
    assembled() cuts asmsh's full names to the same; full_section() resolves
    each against the section list. A listing is read one byte to a
    character.
    Both listings end each line in CRLF, which is read as LF, the line end
    SHC_WRAPPED matches, and only LF ends a line. A .SDATA's string keeps
    its semicolons and doubled quotes, and a comment starts only past it.
    Fails where full_section() does."""
    text = content(path).decode('latin-1').replace('\r\n', '\n')
    data, unnamed, sizes, loads = (assembled if ASSEMBLED.match(text) else compiled)(text)
    named, templates = {}, {}
    for found, held in ((data, named), (unnamed, templates)):
        for short, name, start, end in found:
            held[name] = full_section(path, short, sizes), start, end - start
    # two functions' statics can share a name
    twice = {name for name, count in Counter(datum[1] for datum in data).items() if count > 1}
    return named, templates, loads, twice


def compiled(text):
    """listing()'s named data, unnamed data, section sizes and loads, from
    SHC's listing."""
    data, sizes, section, datum, directive = [], {}, None, None, None
    unnamed, loads, function = [], {}, None
    text = SHC_WRAPPED.sub(r'\1 \2', text)

    def close():
        """Extend the datum over the last directive, its continuation lines
        too."""
        if datum is None or directive is None:
            return
        offset, op, operands = directive
        if op.startswith('.RES.') and datum[3] > datum[2]:
            # a .RES after the datum's values is SHC's padding before the next
            # datum
            return
        if op.startswith('.SDATA'):
            size = len(sdata(operands))
        elif op.startswith(('.DATAB.', '.RES.')):
            size = UNITS[op[-1]] * int(operands.split(',')[0])
        else:
            size = UNITS[op[-1]] * len([value for value in operands.split(',') if value.strip()])
        datum[3] = max(datum[3], offset + size)

    for line in text.split('\n'):
        # the section, the offset, the code and the rest, in their columns
        info = SHC_SECTION.match(line)
        row = SHC_ROW.match(line)
        more = SHC_MORE.match(line)
        # a section with no offset: the line marker -optimize=0 writes ahead
        # of a section's first code
        marker = re.match(r'(\w{1,3}) +; ', line)
        if info:
            sizes[info.group(1)] = int(info.group(2), 16)
        elif more and directive is not None:
            directive = (directive[0], directive[1], directive[2] + ',' + more.group(1))
        elif marker:
            close()
            directive, datum = None, None
            section = marker.group(1)
        elif row:
            close()
            directive = None
            section = row.group(1).strip() or section
            offset, rest = int(row.group(2), 16), row.group(3)
            label = STATIC.match(rest)
            op = DIRECTIVE.match(rest)
            in_code = section is not None and section.startswith('P')
            begins = BEGINS.search(rest)
            if in_code:
                function = begins.group(1) if begins else function
                loads.setdefault(function, set()).update(LABELS.findall(rest))
            if label:
                datum = [section, label.group(1), offset, offset]
                data.append(datum)
            elif UNNAMED.fullmatch(rest) and not in_code:
                datum = [section, rest.strip()[:-1], offset, offset]
                unnamed.append(datum)
            elif op and datum is not None:
                directive = (offset, op.group(1), op.group(2))
            else:
                # code, alignment, or a label of no datum's: the datum has ended
                datum = None
    close()
    return data, unnamed, sizes, loads


def assembled(text):
    """listing()'s named data, unnamed data, section sizes and loads, from
    asmsh's listing.

    A row is its line number, the offset, the code, and the source's line
    number and text. A row of an offset and code alone goes on with the code
    above it. The listing cuts the source's long lines short, so a datum's
    values are measured by their code. .SECTION starts a section, and the
    section list at the end gives the sizes."""
    data, sizes, section, datum, values = [], {}, None, None, False
    unnamed, loads, function = [], {}, None
    for line in text.split('\n'):
        size = re.match(r'(\w+) +REL-(?:CODE|DATA) +([0-9A-F]+)', line)
        row = re.match(r' *\d+ ([0-9A-F]{8}) ([0-9A-F]*) +\d+ (.*)$', line)
        more = re.match(r' {6}([0-9A-F]{8}) ([0-9A-F]+) *$', line)
        if size:
            sizes[size.group(1)] = int(size.group(2), 16)
        elif more:
            if datum is not None and values:
                datum[3] = max(datum[3], int(more.group(1), 16) + len(more.group(2)) // 2)
        elif row:
            offset, code, rest = int(row.group(1), 16), row.group(2), row.group(3).strip()
            values = False
            begun = re.match(r'\.SECTION +(\w+)', rest)
            if begun:
                section, datum = begun.group(1)[:3], None
                continue
            label = STATIC.match(rest)
            op = DIRECTIVE.match(rest)
            in_code = section is not None and section.startswith('P')
            begins = BEGINS.search(rest)
            if in_code:
                function = begins.group(1) if begins else function
                loads.setdefault(function, set()).update(LABELS.findall(rest))
            if label:
                datum = [section, label.group(1), offset, offset]
                data.append(datum)
            elif UNNAMED.fullmatch(rest) and not in_code:
                datum = [section, rest[:-1].strip(), offset, offset]
                unnamed.append(datum)
            elif op and datum is not None and op.group(1).startswith(('.DATAB.', '.RES.')):
                # a .RES after the datum's values is SHC's padding before the
                # next datum
                if not (op.group(1).startswith('.RES.') and datum[3] > datum[2]):
                    count = int(op.group(2).split(',')[0])
                    datum[3] = max(datum[3], offset + UNITS[op.group(1)[-1]] * count)
            elif op and datum is not None:
                datum[3] = max(datum[3], offset + len(code) // 2)
                values = True
            else:
                # code, alignment, or a label of no datum's: the datum has ended
                datum = None
    return data, unnamed, sizes, loads


def strings(path):
    """Each string SHC's listing at path holds, as its section, offset and
    bytes. A string is a row of an L label alone and the characters the
    .SDATA, .DATA.B and .DATAB.B rows below it give. The listing is read as
    listing() reads it. asmsh's listing, a patched source's, gives no
    strings, so a patched source can't take texts."""
    found, sizes, section, directive, string = [], {}, None, None, None

    def close():
        if directive is None:
            return
        op, operands = directive
        if op == '.SDATA':
            data = sdata(operands)
        elif op == '.DATA.B':
            data = bytes(int(value.strip()[2:], 16) for value in operands.split(',') if value.strip())
        else:
            count, value = (value.strip() for value in operands.split(',')[:2])
            data = bytes([int(value[2:], 16) if value.startswith("H'") else int(value)]) * int(count)
        string[2].extend(data)

    for line in content(path).decode('latin-1').replace('\r\n', '\n').split('\n'):
        info = SHC_SECTION.match(line)
        row = SHC_ROW.match(line)
        more = SHC_MORE.match(line)
        if info:
            sizes[info.group(1)] = int(info.group(2), 16)
        elif more and directive is not None:
            directive = (directive[0], directive[1] + ',' + more.group(1))
        elif row:
            close()
            directive = None
            section = row.group(1).strip() or section
            op = DIRECTIVE.match(row.group(3))
            if UNNAMED.fullmatch(row.group(3)):
                string = [section, int(row.group(2), 16), bytearray()]
                found.append(string)
            elif op and op.group(1) in ('.SDATA', '.DATA.B', '.DATAB.B') and string is not None:
                directive = op.groups()
            else:
                string = None
    close()
    return [(full_section(path, short, sizes), offset, bytes(data)) for short, offset, data in found]


def texts(source):
    """Each of source's texts in number order, as its name, number, length
    and the placeholder the stub build gives it.

    Fails, naming source, where:

    - the texts aren't numbered from 1 with none missing;
    - a text has no length;
    - more than 255 texts share one length."""
    found = [(name, int(number), int(length))
             for name, number, length in TEXT.findall(uncommented(content(source).decode(errors='replace')))]
    if sorted(number for _, number, _ in found) != list(range(1, len(found) + 1)):
        fail(f'{source}: its texts are numbered from 1, with none missing')
    if not all(length for _, _, length in found):
        fail(f'{source}: a text of no length')
    # SHC keeps one copy of equal strings, so no two texts may share a
    # placeholder. A placeholder is a byte and then zeros, so it differs from
    # every placeholder of another length and ends no other. The byte is the
    # text's number, or for a number past 255 the least byte no other text of
    # its length takes.
    taken = {}
    for _, number, length in found:
        if number <= 255:
            taken.setdefault(length, set()).add(number)
    placed = []
    for name, number, length in sorted(found, key=lambda text: text[1]):
        byte = number
        if number > 255:
            free = [byte for byte in range(1, 256) if byte not in taken.setdefault(length, set())]
            if not free:
                fail(f'{source}: more than 255 texts of length {length}')
            byte = free[0]
            taken[length].add(byte)
        placed.append((name, number, length, bytes([byte]) + bytes(length - 1)))
    return placed


def text_stubs(source, out):
    """Write OUT, the stub build's text.inc, which defines each of SOURCE's
    texts as its placeholder in one string literal. Fails where texts()
    does."""
    lines = []
    for name, _, length, placeholder in texts(Path(source)):
        zeros = '\\0' * (length - 1)
        lines.append(f'#define {name} "\\x{placeholder[0]:02x}{zeros}"\n')
    publish(Path(out), ''.join(lines))


def value_numbers(source):
    """The numbers of source's values, which run from 1 with none missing. A
    value stands alone for one number in an initializer the source writes,
    so the source writes each once and no macro of its own holds one.

    Fails, naming source, where:

    - a macro holds a value;
    - the source writes a value more than once;
    - the values aren't numbered from 1 with none missing."""
    code = uncommented(content(source).decode(errors='replace'), lines=False).replace('\\\n', '')
    if any(line.lstrip().startswith('#') and VALUE.search(line) for line in code.splitlines()):
        fail(f'{source}: a macro holds a value, which stands alone in an initializer')
    found = Counter(int(match.group(1)) for match in map(VALUE.fullmatch, TOKEN.findall(code)) if match)
    again = sorted(number for number, count in found.items() if count > 1)
    if again:
        fail(f'{source}: it writes VALUE_{again[0]} {found[again[0]]} times, and a value is one number')
    if sorted(found) != list(range(1, len(found) + 1)):
        fail(f'{source}: its values are numbered from 1, with none missing')
    return sorted(found)


def value_stubs(source, out):
    """Write OUT, the stub build's values.inc: 0 for each of SOURCE's values,
    as a body's stub is. Fails where value_numbers() does."""
    publish(Path(out), ''.join(f'#define VALUE_{number} 0\n' for number in value_numbers(Path(source))))


def initialized(tokens, at, layout, offset, found, where):
    """The index past the initializer at tokens[at]. The object it
    initializes, the datum being read or a member of it, is laid out as
    layout and lies offset bytes into the datum.

    A value stands alone for one scalar. In a datum that holds a value, each
    aggregate takes its initializer in braces, or else C would fill it from
    the list around it. A string or a text can stand for a char array's.

    Each value the initializer writes goes into found as (name, offset,
    layout). The offset and layout are those of the scalar the value stands
    for, and the offset counts from the datum's start.

    Fails, naming where, on:

    - a value in an expression, or for a pointer or an aggregate;
    - more initializers than the braces hold, or a token where , or } ends
      one;
    - an aggregate without braces, but for a char array's string or text.

    Raises IndexError, which placed_values() catches, where the initializer
    doesn't close."""
    if tokens[at] == '{':
        at += 1
        if isinstance(layout, Array):
            members = [(layout.element, offset + index * layout.element.size) for index in range(layout.count)]
        elif isinstance(layout, (Scalar, Pointer)):
            members = [(layout, offset)]
        else:
            members = [(field, offset + place) for _, field, place in layout.fields[:1 if layout.union else None]]
        for member, place in members:
            if tokens[at] == '}':
                break
            at = initialized(tokens, at, member, place, found, where)
            if tokens[at] == ',':
                at += 1
            elif tokens[at] != '}':
                fail(f'{where}: {tokens[at]} where , or }} ends an initializer')
        if tokens[at] != '}':
            fail(f'{where}: more initializers than the {len(members)} its braces hold')
        return at + 1
    # an expression, to the , or } that ends it outside its brackets
    end, depth = at, 0
    while depth or tokens[end] not in (',', '}'):
        depth += tokens[end] in ('(', '[')
        depth -= tokens[end] in (')', ']')
        end += 1
    expression = tokens[at:end]
    held = [token for token in expression if VALUE.fullmatch(token)]
    if held:
        if len(expression) != 1:
            fail(f'{where}: {held[0]} stands alone for one number, not in {" ".join(expression)}')
        if not isinstance(layout, Scalar):
            kind = 'a pointer, which the source writes itself' if isinstance(layout, Pointer) else 'an aggregate'
            fail(f'{where}: {held[0]} stands for {kind}, and a value is one number')
        found.append((held[0], offset, layout))
    elif not isinstance(layout, (Scalar, Pointer)):
        text = len(expression) == 1 and (expression[0].startswith('"') or re.fullmatch(r'TEXT_[0-9]+', expression[0]))
        if not (isinstance(layout, Array) and isinstance(layout.element, Scalar) and layout.element.size == 1
                and text):
            fail(f'{where}: an aggregate whose initializer has no braces, beside a value')
    return end


class Laid(Item):
    """A body whose type has no kind: a scalar, or a type the Types of the source and its headers lay out."""

    single = False

    def __init__(self, start, end, layout):
        self.layout = layout
        super().__init__(start, end, 'laid')

    @property
    def element(self):
        return self.layout.size

    @property
    def alignment(self):
        return self.layout.alignment


class Declared(Item):
    """A datum the source defines with no body. A pointer from a body into it
    takes its name and type, and a pointer into one whose type is None
    fails."""

    def __init__(self, start, end, ctype, array, step=None):
        super().__init__(start, end, 'bytes')
        self.declared, self.array = ctype, array
        self.step = step  # an element's size, or None to take the datum as one element
        self.member = None  # None, or for a union whose first member is an array: its element's type and name

    @property
    def element(self):
        return self.step or self.end - self.start


def element_size(ctype, types):
    """The size in bytes of an element of type ctype, its qualifiers aside.
    types() returns the Types, and is called only for a ctype that neither
    BODY_KINDS nor SCALARS holds. Raises LayoutError where the Types can't
    lay it out."""
    ctype = unqualified(ctype)[1]
    if ctype in BODY_KINDS:
        return KINDS[BODY_KINDS[ctype]].layout.size
    if ctype in SCALARS:
        return Scalar(SCALARS[ctype]).size
    return types().layout(ctype).size


def qualify(qualifiers, ctype):
    """ctype, after the qualifiers a declaration gives it, in QUALIFIERS' order: const, then volatile."""
    return ' '.join([*(word for word in QUALIFIERS if word in qualifiers.split()), ctype])


def nest(values, extents):
    """values, each an element's initializer, in nested braces, one level for
    each of extents. The extents run outer to inner, as a multidimensional
    array's inner extents do. None where an extent is 0 or doesn't divide
    the values it groups."""
    for extent in reversed(extents):
        if not extent or len(values) % extent:
            return None
        values = ['{\n    ' + ',\n    '.join(value.replace('\n', '\n    ') for value in values[at:at + extent]) + ',\n}'
                  for at in range(0, len(values), extent)]
    return values


def headers(depend):
    """The .h files the first rule of depend names. depend is the dependency
    file the Makefile has the host compiler write for a stub build's
    source. Fails where that file is missing."""
    if not depend.exists():
        fail(f'{depend}: missing, and the stub build writes it')
    rule = depend.read_text().replace('\\\n', ' ').split('\n', 1)[0]
    return [Path(name) for name in rule.partition(':')[2].split() if name.endswith('.h')]


class BodySource:
    """A body source, and what --bodies reads with it: its stub build's
    listing and dependencies, and the image and tables its bodies come from.
    Its passes fail, naming the file at fault, on what they can't take."""

    def __init__(self, path, source, lis, conf):
        self.path, self.source, self.lis = path, Path(source), Path(lis)
        self.config = Config(conf)
        self.image = self.config.image(path)
        # the source's text as uncommented() gives it, which every pass reads
        self.text = uncommented(content(self.source).decode(errors='replace'))
        # the directory the source's includes name its bodies in: its path
        # less its top directory and .c
        self.directory = Path(*self.source.parts[1:]).with_suffix('').as_posix()
        self.places, self.templates, self.loads, self.twice = listing(self.lis)
        # each section from its first row's start to its rows' highest end, as
        # link.py places the object
        self.starts, self.ends = {}, {}
        for section, start, end in self.config.ranges(self.source.as_posix()):
            self.starts.setdefault(section, start)
            self.ends[section] = max(end, self.ends.get(section, end))
        # each datum's type as a pointer to it takes it, qualifiers kept, and
        # its extents. A union or a struct of no tag that the source defines
        # as it declares the datum is taken as union or struct, and as no
        # array. unions gives each such union's first member where that is an
        # array, and None otherwise
        self.declarations = {name: (qualify(qualifiers, ctype) + (' *' if pointer else ''), EXTENT.findall(dims))
                             for qualifiers, ctype, pointer, name, dims in DECLARATION.findall(self.text)}
        self.unions = {name: MEMBER.match(members) for members, name in UNION.findall(self.text)}
        self.declarations.update({name: ('union', []) for name in self.unions})
        self.declarations.update({name: ('struct', []) for name in UNTAGGED.findall(self.text)})
        self.cached_types = None  # the Types, once a pass needs them

    def fail(self, problem):
        """Exit with status 1, naming the source and the problem."""
        fail(f'{self.source}: {problem}')

    def types(self):
        """The Types of the source and the headers its stub build's dependency
        file names, read once, when a pass first needs them."""
        if self.cached_types is None:
            included = [header.read_text(errors='replace') for header in headers(self.lis.with_suffix('.d'))]
            # the source as written, whose comments Types reads as C does
            self.cached_types = Types([*included, content(self.source).decode(errors='replace')])
        return self.cached_types

    def extent(self, count):
        """An extent's count, from its text: a number, which needs no Types,
        or a constant expression the Types evaluate. Fails where they can't."""
        tokens = TOKEN.findall(count)
        try:
            if len(tokens) == 1 and tokens[0][0].isdigit():
                return number(tokens[0])[0]
            return self.types().evaluate(tokens)
        except LayoutError as error:
            self.fail(f'[{count}]: {error}')

    def check_place(self, name, section, offset, size):
        """Fail where the size bytes the stub listing places for name, at
        offset in section, lie in no section the image's tables place, run
        past that section's end, or lie outside the image."""
        if section not in self.starts:
            self.fail(f'{name} isn\'t in a section {self.config.splits} places')
        start = self.starts[section] + offset
        if start + size > self.ends[section]:
            self.fail(f'{name} runs past its {section}\'s end in {self.config.splits}')
        if not self.image.inside(start, size):
            self.fail(f'{name}, 0x{start:08x} to 0x{start + size:08x}, lies outside {self.path}')

    def static_bodies(self):
        """Each body the source gives a datum of static storage, a file's or a
        function's, by name: its qualifiers, its type, with * for a pointer,
        and its extents.

        Fails on a body:

        - of a datum named text or values, whose .inc holds the texts or
          values;
        - whose include isn't its name's .inc;
        - of a name two statics share;
        - of an array with no extent;
        - of a datum the stub listing doesn't place, or one check_place()
          fails on."""
        filled = {}
        for qualifiers, ctype, pointer, name, dims, include in BODY.findall(self.text):
            ctype += ' *' if pointer else ''
            if name in ('text', 'values'):
                self.fail(f'{self.directory}/{name}.inc holds its {name if name == "values" else "texts"}, and '
                          f'can\'t be {name}\'s body')
            if include != f'{self.directory}/{name}.inc':
                self.fail(f'{name}\'s body is {self.directory}/{name}.inc, not {include}')
            if name in self.twice:
                self.fail(f'{name} names two statics, and a body needs one')
            if dims and not EXTENT.findall(dims)[0].strip():
                self.fail(f'{name} needs an extent, and its stub build\'s zero body would give it one element')
            if name not in self.places:
                self.fail(f'{name} isn\'t in a section {self.config.splits} places')
            self.check_place(name, *self.places[name])
            filled[name] = qualifiers, ctype, EXTENT.findall(dims)
        return filled

    def automatic_bodies(self, filled):
        """Each body the source gives a function's automatic array, by name:
        its qualifiers, its type and its extents, as static_bodies() gives
        them.

        An array's body is the template SHC copies it from: the one unnamed
        datum of the array's size that its function loads. Its function is
        the nearest function definition above it in the source, found by
        text and not by C's scopes. The template's place goes into
        self.places under the array's name.

        Fails on an array:

        - whose include isn't its name's .inc;
        - with no function definition above it;
        - of a type tools/data.py writes no body for, or with an extent
          extent() fails on;
        - whose function loads no unnamed datum of its size, or several;
        - whose name is in self.places or filled already: the listing's data,
          an earlier automatic array, or a static body;
        - whose template check_place() fails on."""
        functions = [(match.start(), match.group(1)) for match in FUNCTION.finditer(self.text)]
        automatic = {}
        for match in AUTO.finditer(self.text):
            qualifiers, ctype, name, dims, include = match.groups()
            within = [function for start, function in functions if start < match.start()]
            if include != f'{self.directory}/{name}.inc':
                self.fail(f'{name}\'s body is {self.directory}/{name}.inc, not {include}')
            if not within:
                self.fail(f'{name} needs a function')
            try:
                size = element_size(ctype, self.types)
            except LayoutError as error:
                self.fail(f'{name} is {ctype}, which tools/data.py writes no body for: {error}')
            for count in EXTENT.findall(dims):
                size *= self.extent(count)
            found = [self.templates[label] for label in sorted(self.loads.get(within[-1], ()))
                     if label in self.templates and self.templates[label][2] == size]
            if len(found) != 1:
                self.fail(f'{within[-1]} loads {len(found)} templates of {name}\'s {size} bytes')
            if name in self.places or name in filled:
                self.fail(f'{name} names two bodies')
            self.places[name] = found[0]
            self.check_place(name, *found[0])
            automatic[name] = qualifiers, ctype, EXTENT.findall(dims)
        return automatic

    def check_includes(self, filled):
        """Fail where the source includes an .inc that is neither the .inc of
        a body in filled nor the source's text.inc or values.inc. --bodies
        would write no such .inc, so only the compile would fail on it."""
        taken = {f'{self.directory}/{name}.inc' for name in filled} | {f'{self.directory}/text.inc',
                                                                       f'{self.directory}/values.inc'}
        for include in INCLUDE.findall(self.text):
            if include not in taken:
                self.fail(f'no declaration takes {include}: a body\'s #include has its declaration\'s "= {{" on '
                          f'the line before and "}};" on the line after, and the declaration puts any qualifiers '
                          f'before a type of one word or struct and a tag, then declares a datum of static storage, '
                          f'with one * at most, or a function\'s automatic array, with no *')

    def placed_values(self):
        """Each value the source writes, in number order, as its name, address
        and layout: the address of the member it stands for, and that
        member's scalar layout.

        The member lies in the datum whose initializer writes the value, at
        the offset the datum's layout gives it. The stub listing places the
        datum.

        Fails where:

        - value_numbers() does;
        - the source writes values and doesn't include values.inc, or the
          reverse;
        - a datum that holds a value has a type with no layout, an array of
          no extent, or an extent extent() fails on;
        - the datum's initializer is one initialized() fails on, doesn't
          close, or goes on past its braces;
        - two statics share the datum's name, or no section the tables place
          holds the datum;
        - a value lies past the datum's size in the listing, or outside the
          image;
        - a value lies in no datum's initializer."""
        numbers = value_numbers(self.source)
        if bool(numbers) != (f'#include "{self.directory}/values.inc"' in self.text):
            self.fail(f'writes values, and includes {self.directory}/values.inc, or neither')
        held = {}
        tokens = [(match.start(), match.group()) for match in TOKEN.finditer(self.text)]
        offsets = [start for start, _ in tokens]
        tokens = [token for _, token in tokens]
        for match in WRITTEN.finditer(self.text if numbers else ''):
            qualifiers, ctype, name, dims = match.groups()
            at = bisect_left(offsets, match.end())
            close, depth = at, 0
            while close < len(tokens):
                depth += tokens[close] == '{'
                depth -= tokens[close] == '}'
                if not depth:
                    break
                close += 1
            if not any(VALUE.fullmatch(token) for token in tokens[at:close]):
                continue
            try:
                layout = self.types().layout(ctype)
            except LayoutError as error:
                self.fail(f'{name} is {ctype}, which tools/data.py lays out no value in: {error}')
            for count in reversed(EXTENT.findall(dims)):
                if not count.strip():
                    self.fail(f'{name} needs an extent, to lay out the values it holds')
                layout = Array(layout, self.extent(count))
            found = []
            try:
                if initialized(tokens[:close + 1], at, layout, 0, found, f'{self.source}: {name}') != close + 1:
                    self.fail(f'{name}\'s initializer goes on past its braces')
            except IndexError:
                self.fail(f'{name}\'s initializer doesn\'t close')
            if name in self.twice:
                self.fail(f'{name} names two statics, and its values need one')
            if name not in self.places or self.places[name][0] not in self.starts:
                self.fail(f'{name} holds {found[0][0]}, and isn\'t in a section {self.config.splits} places')
            section, offset, size = self.places[name]
            for value, into, scalar in found:
                if into + scalar.size > size:
                    self.fail(f'{value} lies past {name}\'s {size} bytes in its stub listing')
                start = self.starts[section] + offset + into
                if not self.image.inside(start, scalar.size):
                    self.fail(f'{value}, 0x{start:08x} to 0x{start + scalar.size:08x}, lies outside {self.path}')
                held[value] = start, scalar
        unheld = [f'VALUE_{number}' for number in numbers if f'VALUE_{number}' not in held]
        if unheld:
            self.fail(f'{unheld[0]} lies in no initializer of a datum it defines')
        return [(f'VALUE_{number}', *held[f'VALUE_{number}']) for number in numbers]

    def names(self):
        """The image's names by address, and the name of each datum of
        nonzero size the stub listing places in a section the tables place. A
        datum's name replaces the tables' at its address, and of two data at
        one address the later in self.places wins."""
        names = dict(self.config.names)
        for name, (section, offset, size) in self.places.items():
            if section in self.starts and size:
                names[self.starts[section] + offset] = name
        return names

    def items(self, filled):
        """Every datum of nonzero size the stub listing places in a section
        the tables place, as items in address order, and the bodies among
        them as (name, item, type). The bodies come in self.places' order:
        the listing's data in its order, then the automatic arrays in the
        source's. A body's item is body()'s, and every other datum's is
        datum()'s. Fails where body() does, or where two data overlap."""
        items, written = [], []
        for name, (section, offset, size) in self.places.items():
            if section not in self.starts or not size:
                continue
            start = self.starts[section] + offset
            if name in filled:
                item = self.body(name, start, size, *filled[name])
                written.append((name, item, filled[name][1]))
            else:
                item = self.datum(name, start, size)
            items.append(item)
        items.sort(key=lambda item: item.start)
        names = self.names()
        for first, second in zip(items, items[1:]):
            if first.end > second.start:
                self.fail(f'{names[first.start]} and {names[second.start]} overlap')
        return items, written

    def datum(self, name, start, size):
        """A datum the source defines with no body, as a Declared item of the
        type the source's declarations give it, or of type None where they
        give none. Where its element's size is known and divides the datum's,
        an array steps by that size, so that a pointer into it names an
        element; otherwise the datum is one element."""
        ctype, extents = self.declarations.get(name, (None, []))
        step = None
        if extents:
            try:
                step = element_size(ctype, self.types)
            except LayoutError:
                pass
        item = Declared(start, start + size, ctype, bool(extents), step if step and not size % step else None)
        item.inner = extents[1:]
        if self.unions.get(name):
            item.member = self.unions[name].groups()
        return item

    def body(self, name, start, size, qualifiers, ctype, extents):
        """A body's item, with the qualifiers and extents its declaration
        gives it: an Item of its type's kind, or a Laid one of a scalar or of
        a type the Types of the source and its headers lay out. A type that
        is itself an array is laid out as its element. Fails where the type
        has no layout, or size is no whole number of its elements."""
        # each element's inner extents, which an aggregate's braces follow
        array, inner = bool(extents), extents[1:]
        factors = re.fullmatch(r'NJS_MDATA([1-4])', ctype)
        if factors:
            item = Item(start, start + size, 'mdata', int(factors.group(1)))
        elif ctype in BODY_KINDS:
            item = Item(start, start + size, BODY_KINDS[ctype])
        elif ctype in SCALARS:
            item = Laid(start, start + size, Scalar(SCALARS[ctype]))
        else:
            try:
                layout = self.types().layout(ctype)
            except LayoutError as error:
                self.fail(f'{name} is {ctype}, which tools/data.py writes no body for: {error}')
            # a type that is an array: lay out its element. Its extents follow
            # the declaration's, and where the declaration has none, the
            # type's first is the outer
            counts = []
            while isinstance(layout, Array):
                counts.append(str(layout.count))
                layout = layout.element
            inner = extents[1:] + counts if array else counts[1:]
            array = array or bool(counts)
            item = Laid(start, start + size, layout)
        if size % item.element:
            self.fail(f'{name} is {size} bytes, no whole number of {ctype}')
        # a pointer to it takes its type with its qualifiers, which C keeps
        item.declared = qualify(qualifiers, ctype)
        item.array, item.inner = array, inner
        return item

    def placed_texts(self):
        """Each text the source declares, in number order, as its name,
        address and bytes on the disc. A text lies where its placeholder ends
        a string in the stub build.

        Fails where:

        - texts() does;
        - the source declares texts and doesn't include text.inc, or the
          reverse;
        - the stub build ends no string with a text's placeholder, or
          several;
        - a text lies in no section the tables place;
        - a text and its null run past its section's end, or lie outside the
          image;
        - the disc's text isn't the text's length."""
        declared = texts(self.source)
        if bool(declared) != (f'#include "{self.directory}/text.inc"' in self.text):
            self.fail(f'declares texts, and includes {self.directory}/text.inc, or neither')
        found_strings = strings(self.lis) if declared else []
        placed = []
        for name, _, length, placeholder in declared:
            # a text ends its string, before the null
            mark = placeholder + b'\0'
            found = [(section, offset + len(data) - len(mark)) for section, offset, data in found_strings
                     if data.endswith(mark)]
            if len(found) != 1:
                self.fail(f'its stub build ends {len(found)} strings with {name}\'s placeholder')
            (section, at), = found
            if section not in self.starts:
                self.fail(f'{name} isn\'t in a section {self.config.splits} places')
            start = self.starts[section] + at
            if start + length >= self.ends[section]:
                self.fail(f'{name} runs past its {section}\'s end in {self.config.splits}')
            if not self.image.inside(start, length + 1):
                self.fail(f'{name} and its null, 0x{start:08x} to 0x{start + length + 1:08x}, lie outside {self.path}')
            body = self.image.payload[start - self.image.base:start - self.image.base + length + 1]
            if body.find(0) != length:
                self.fail(f'{name} isn\'t {length} bytes long on the disc')
            placed.append((name, start, body[:length]))
        return placed


class Bodies(Writer):
    """A body source's bodies as C. A null pointer is 0, and a pointer whose
    value lies outside the image's address windows is a cast number. Every
    other pointer is written with a name: one the source defines, or one the
    source or its headers declare."""

    def __init__(self, body_source, items, names):
        super().__init__(body_source.image, items, names)
        self.types, self.extent, self.fail = body_source.types, body_source.extent, body_source.fail
        # what a body can point at outside the source: a name the source or
        # its headers declare extern, or a function their prototypes declare
        included = [body_source.source, *headers(body_source.lis.with_suffix('.d'))]
        self.outside = agreed(externs(included, BODY_EXTERN))
        self.functions = {name for path in included
                          for name in PROTOTYPE.findall(uncommented(path.read_text(errors='replace')))}
        self.addresses = sorted(names)

    def values(self, item):
        if item.kind != 'laid':
            return super().values(item)
        layout = item.layout
        values = [self.initializer(layout, address) for address in range(item.start, item.end, item.element)]
        return values, LINE[layout.code] if isinstance(layout, Scalar) else 4 if isinstance(layout, Pointer) else 1

    def ref(self, address, pointee):
        r"""A constant for the pointer at address, as a pointee *.

        - A nonzero value outside the image's address windows is written as
          a cast number: the windows decide what is an address.
        - A value outside the source's data is left as
          \1name\2pointee\2offset\3, for inc() to resolve.
        - A pointer to the start of a union is written as the union's first
          member, where that is an array that converts to a pointee * with no
          cast.
        - A pointer into an array with inner extents, or a pointer to an
          array, names the element by its subscripts.
        - Any other, null included, is Writer.ref()'s.

        Fails where the pointer reaches a datum whose declaration
        tools/data.py doesn't read. Raises EmitError where a pointer to an
        array holds no address, points inside an element, or points where the
        datum holds no array of its pointee."""
        value = self.word(address)
        if value and not self.image.is_address(value):
            # a value outside the address windows, such as an offset a loader
            # relocates
            if '[]' in pointee:
                raise EmitError(self.image, address, f'a pointer to {pointee} holds 0x{value:08x}, no address')
            return f'({pointee} *)0x{value:08x}'
        index = bisect_right(self.starts, value) - 1
        if value and (index < 0 or not self.items[index].start <= value < self.items[index].end):
            # outside the source's data: the name at the value, or else the
            # nearest name below it and the offset past that name. That name
            # is a guess at the datum the value points into. It is made only
            # outside code, and only where the source or its headers declare
            # the name extern
            name, offset = self.names.get(value), 0
            below = bisect_right(self.addresses, value) - 1
            if name is None and below >= 0 and self.names[self.addresses[below]] in self.outside \
                    and self.image.is_address(value) and not self.image.in_code(value):
                name, offset = self.names[self.addresses[below]], value - self.addresses[below]
            return f'\1{name or f"dat_{value:08x}"}\2{pointee}\2{offset}\3'
        item = self.items[index] if value else None
        if item is not None and item.declared is None:
            self.fail(f'a body points at {self.name(item.start)}, whose declaration tools/data.py doesn\'t read')
        member = getattr(item, 'member', None)
        if member and value == item.start and converts(member[0], pointee):
            # a union, by its first member, an array of what the pointer takes
            return f'{self.name(item.start)}.{member[1]}'
        # a pointer to an array: its element, and the extents it spans
        element_type, spanned = pointee.replace('[]', ''), pointee.count('[]')
        if item is None or not item.inner and not spanned:
            return super().ref(address, pointee)
        if (value - item.start) % item.element:
            if spanned:
                raise EmitError(self.image, address,
                                f'a pointer to an array of {element_type} holds 0x{value:08x}, inside an element')
            return super().ref(address, pointee)
        # the element's subscripts, outer first. A pointer to an element takes
        # them all. A pointer to an array drops one from the end for each []
        # of its pointee, and each one it drops must be 0
        index, indices = (value - item.start) // item.element, []
        for count in reversed(item.inner):
            index, at = divmod(index, self.extent(count)) if index else (0, 0)
            indices.append(at)
        subscripts = [index, *reversed(indices)] if item.array else []
        kept = subscripts[:len(subscripts) - spanned]
        if spanned > len(subscripts) or any(subscripts[len(kept):]):
            raise EmitError(self.image, address, f'a pointer to an array of {element_type} holds 0x{value:08x}, where '
                                                 f'{self.name(item.start)} holds none')
        expression = f'&{self.name(item.start)}' + ''.join(f'[{at}]' for at in kept)
        # a pointer to an array takes no cast: its pointee, T[], keeps no extent
        # to spell one with. Another takes one only where C won't convert.
        if spanned or converts(item.ctype, pointee):
            return expression
        return f'({pointee} *){expression}'

    def resolve(self, match):
        """The constant for a name from outside, which ref() left for later as
        match's name, pointee and offset.

        A function the prototypes declare is its name, and a body may point
        only at its start. A name the source or its headers declare extern is
        the name where it is an array, and its address otherwise. At a nonzero
        offset it is the address of its element there. Either is cast where C
        won't convert, but a pointer to an array takes no cast.

        Fails where a body points:

        - at a name neither the source nor its headers declare;
        - into a function, past its start;
        - into a name with a pointer to an array;
        - into a name of a type the Types can't lay out;
        - into a name other than at an element of an array."""
        name, pointee, offset = match.group(1), match.group(2), int(match.group(3))
        if name in self.functions and name not in self.outside:
            return name
        if name not in self.outside:
            self.fail(f'a body points at {name}, which it doesn\'t declare')
        ctype, array = self.outside[name]
        if '[]' in pointee:
            # a pointer to an array, which takes no cast
            if offset:
                self.fail(f'a body points {offset} bytes into {name} with a pointer to an array')
            return name if array else f'&{name}'
        if not offset:
            expression = name if array else f'&{name}'
        else:
            try:
                size = element_size(ctype, self.types)
            except LayoutError as error:
                self.fail(f'a body points into {name}, a {ctype}: {error}')
            if not array or offset % size:
                self.fail(f'a body points {offset} bytes into {name}, and not at an element of it')
            expression = f'&{name}[{offset // size}]'
        if converts(ctype, pointee):
            return expression
        return f'({pointee} *){expression}'

    def inc(self, name, item, ctype):
        """The text of name's .inc: item's values, every pointer resolved.
        Records in an array with inner extents go in the braces those extents
        take, and values other than records stay flat. A body that is no
        array is its one value, without a record's own braces.

        Raises EmitError where values() does. Fails where:

        - the elements don't fill the inner extents;
        - the body is no array and holds more than one ctype;
        - ref() or resolve() does, as for a pointer to a name neither the
          source nor its headers declare, or to a datum of the source's whose
          type its declarations don't give."""
        values, per_line = self.values(item)
        if item.inner and values[0].startswith('{'):
            # a record's braces would open an inner array instead, so records
            # take the inner extents' braces, and other values go flat
            values, per_line = nest(values, [self.extent(count) for count in item.inner]), 1
            if values is None:
                self.fail(f'{name}\'s {(item.end - item.start) // item.element} elements don\'t fill its '
                          f'inner extents {"".join(f"[{count}]" for count in item.inner)}')
        if not item.array:
            if len(values) != 1:
                self.fail(f'{name} is one {ctype}, and {len(values)} long')
            # no array: the source's "= {" and "};" brace its one value, so a
            # record's own braces come off
            value = values[0].strip()
            value = value[1:-1].strip() if value.startswith('{') else value
            body = '\n'.join('    ' + line.strip() for line in value.splitlines() if line.strip())
        else:
            body = ',\n'.join(f'    {line}' for line in wrap(values, per_line))
        if body and not body.endswith(','):
            body += ','
        return re.sub('\1([^\2]*)\2([^\2]*)\2([0-9]+)\3', self.resolve, body) + '\n'


def retype_uvs(image, items):
    """Make uv the kind of each vcolor item that a meshset among items points
    at as its uvs. A meshset's colors and uvs are both NJS_COLOR *, so
    BODY_KINDS makes each a vcolor."""
    uvs = set()
    for item in items:
        if item.kind == 'meshset':
            uvs |= {image.u32(address + MESHSET.uvs) for address in range(item.start, item.end, item.element)}
    for item in items:
        if item.kind == 'vcolor' and item.start in uvs:
            item.kind = 'uv'


def publish_group(out, incs, listed):
    """Replace the group in out: the .inc files with incs, each one's text by
    its file name, and bodies.txt with listed. bodies.txt is removed first
    and written last. Between them, each .inc in out but not in incs is
    removed, and each of incs is published. A failure after bodies.txt is
    removed leaves none, though some .inc files may have changed by then. So
    make finds the whole group or rebuilds it."""
    out.mkdir(parents=True, exist_ok=True)
    (out / 'bodies.txt').unlink(missing_ok=True)
    for inc in out.glob('*.inc'):
        if inc.name not in incs:
            inc.unlink()
    for name, inc in incs.items():
        publish(out / name, inc)
    publish(out / 'bodies.txt', listed)


def bodies(path, source, lis, out, conf):
    """Write --bodies' group in OUT, from IMAGE where the stub listing places
    SOURCE's data: each body's .inc, text.inc where SOURCE declares texts,
    values.inc where it writes values, and bodies.txt.

    Every pass and the emitter run before anything is written, so a failure
    in one leaves the former group whole. publish_group() then replaces the
    group, and says what a failure there leaves."""
    body_source = BodySource(path, source, lis, conf)
    filled = body_source.static_bodies()
    filled.update(body_source.automatic_bodies(filled))
    body_source.check_includes(filled)
    values = body_source.placed_values()
    items, written = body_source.items(filled)
    retype_uvs(body_source.image, items)
    writer = Bodies(body_source, items, body_source.names())
    incs = {f'{name}.inc': writer.inc(name, item, ctype) for name, item, ctype in written}
    listed = [f'{name}  0x{item.start:08x}  {item.end - item.start}  {ctype}\n' for name, item, ctype in written]
    placed = body_source.placed_texts()
    if placed:
        incs['text.inc'] = ''.join(f'#define {name} {quote(text)}\n' for name, _, text in placed)
    listed += [f'{name}  0x{start:08x}  {len(text)}  text\n' for name, start, text in placed]
    scalars = {code: ctype for ctype, code in SCALARS.items()}
    if values:
        incs['values.inc'] = ''.join(f'#define {name} {writer.initializer(scalar, start)}\n'
                                     for name, start, scalar in values)
    listed += [f'{name}  0x{start:08x}  {scalar.size}  {scalars[scalar.code]}\n' for name, start, scalar in values]
    publish_group(Path(out), incs, ''.join(listed))


def main(argv):
    parser = argparse.ArgumentParser(prog='data', usage=USAGE, description=__doc__.split('\n\n')[0])
    parser.add_argument('generated', nargs='*', metavar='IMAGE SOURCE OUT',
                        help="write OUT, the C of SOURCE's gen/ range of IMAGE, and OUT's .syms")
    command = parser.add_mutually_exclusive_group()
    command.add_argument('--declared', nargs=2, metavar=('CONFIG', 'OUT'),
                         help="write OUT, declared.txt: the type the extern declarations give each name in CONFIG's "
                              "gen/ ranges")
    command.add_argument('--rows', nargs=1, metavar='IMAGE', help="print the gen/ rows proposed for IMAGE's data")
    command.add_argument('--bodies', nargs=5, metavar=('IMAGE', 'SOURCE', 'LISTING', 'OUT', 'CONFIG'),
                         help="write SOURCE's bodies, texts and values in OUT, from IMAGE where LISTING places them")
    command.add_argument('--texts', nargs=2, metavar=('SOURCE', 'OUT'),
                         help="write OUT, the stub build's text.inc for SOURCE")
    command.add_argument('--values', nargs=2, metavar=('SOURCE', 'OUT'),
                         help="write OUT, the stub build's values.inc for SOURCE")
    args = parser.parse_args(argv)
    options = {declarations: args.declared, rows: args.rows, bodies: args.bodies, text_stubs: args.texts,
               value_stubs: args.values}
    picked = [(function, operands) for function, operands in options.items() if operands]
    if picked and args.generated:
        parser.error(f'unrecognized arguments: {" ".join(args.generated)}')
    if not picked and len(args.generated) != 3:
        parser.error('name IMAGE, SOURCE and OUT, or one option and its operands')
    function, operands = picked[0] if picked else (generate, args.generated)
    try:
        function(*operands)
    except (LayoutError, EmitError, ReadError) as error:
        fail(error)
    except OSError as error:
        # a file a command reads or writes itself, past content() and publish()
        fail(f'{error.filename}: {error.strerror}')


if __name__ == '__main__':
    main(sys.argv[1:])
