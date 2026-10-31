"""Lay out the C types a source and its headers define, as SHC lays them
out, and evaluate SHC's integer constant expressions.

Types reads struct, union and enum definitions, typedefs and object-like
macros without a preprocessor. Of the preprocessor's directives it reads
only #define. It follows no #include, so the caller hands it each header's
text. It ignores conditionals and reads every branch alike, so a name two
branches define differently is a name defined two ways.

A type is laid out only when asked for. One Types can't lay out raises
LayoutError then:

- long long, double, void, a bit-field or a function;
- an array without an extent, or a typedef, struct or union without a
  definition;
- a name defined two ways, a member named twice, a typedef that leads
  back to itself, or a type that holds itself;
- a type name or a member Types can't read;
- an extent Types can't evaluate, or a negative one.

An extent is an integer constant expression without sizeof, a cast, a
character constant, ?: or a function-like macro. Types evaluates it in
unbounded integers, each value keeping the type SHC gives it. An unsigned
value wraps modulo 2**32, as C's does. The result is SHC's as long as each
signed value lies in its type. LayoutError names each operation whose value
SHC could give otherwise, or not at all:

- a signed result or quotient outside its type;
- an operand outside the signed type it converts to;
- a shift by a count outside 0 to 31;
- a division by zero.

Both sides of && and || are evaluated, so these fail on the side C skips
too, as a division by zero does in SHC.
"""

import re
import struct
from collections import Counter, namedtuple


class LayoutError(Exception):
    """A type the declarations don't lay out, or an expression Types can't
    evaluate as SHC does."""


# C's type qualifiers, in the order a qualified term lists them
QUALIFIERS = ('const', 'volatile')
# the storage-class specifiers, typedef among them, which change no layout
STORAGE = {'static', 'extern', 'register', 'auto', 'typedef'}
BASIC = {'void', 'char', 'short', 'int', 'long', 'float', 'double', 'signed', 'unsigned'}
KEYWORDS = {*QUALIFIERS, *STORAGE, *BASIC, 'struct', 'union', 'enum', 'sizeof'}
# C's binary operators and their precedence: a higher one binds tighter
BINARY = {'||': 1, '&&': 2, '|': 3, '^': 4, '&': 5, '==': 6, '!=': 6, '<': 7, '>': 7, '<=': 7, '>=': 7,
          '<<': 8, '>>': 8, '+': 9, '-': 9, '*': 10, '/': 10, '%': 10}
# the values of a signed integer and an unsigned one, each 32 bits on SHC
INT = range(-0x80000000, 0x80000000)
UNSIGNED = range(0x100000000)
# SHC's integer types and their values, in the order of its usual arithmetic
# conversions: both operands convert to the later of their types. So a long
# and an unsigned int convert to long, where C89 converts both to unsigned
# long. SHC converts so in arithmetic, not only in the comparison
# TOOLCHAIN.md#c-for-shc gives.
INTEGER_TYPES = {'int': INT, 'unsigned int': UNSIGNED, 'long': INT, 'unsigned long': UNSIGNED}


def spelled(text):
    """The text of each token of C text outside its comments and directives,
    as csource.py's tokens() reads it."""
    from csource import tokens
    return [word for word, _ in tokens(text, directives=False)]


def identifier(token):
    """Whether token, a token's text, is a name, as C89 spells one."""
    return token.isidentifier() and token.isascii()


class Scalar(namedtuple('Scalar', 'code')):
    """An arithmetic type, by its format character in Python's struct
    module. Plain char takes c, which keeps it apart from signed char's b."""

    @property
    def size(self):
        return struct.calcsize('<' + self.code)

    alignment = size


class Pointer(namedtuple('Pointer', 'pointee')):
    """A pointer, by its pointee as a cast spells it, qualifiers first. A
    qualifier written after a * is left out, so the pointee of int * const *p
    is int *.

    An array pointee is spelled as its element, then each extent in brackets
    in its declaration's order, its tokens one space apart. A typedef's name
    stands for any type in this spelling. Without one, pointee is None for
    what the spelling can't express: a function, an unnamed struct, union or
    enum, a pointer to an array, or a type built on one."""

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
# run of bytes, as (offset, size), that no initializer sets. A struct's holes
# are its padding between and after its members, and a member's own lie in
# its layout. A union's initializer sets only its first member, so the bytes
# past it are a hole
Record = namedtuple('Record', 'union fields size alignment holes')


def basic(words):
    """The scalar a list of C's type words names. Raises LayoutError for
    long long, double or void."""
    count = Counter(words)
    if count.get('long', 0) > 1 or 'double' in count or 'void' in count:
        raise LayoutError(f'{" ".join(words)}: no layout')
    if 'float' in count:
        return Scalar('f')
    code = 'b' if 'char' in count else 'h' if 'short' in count else 'i'
    if 'unsigned' in count:
        return Scalar(code.upper())
    return Scalar('c' if code == 'b' and 'signed' not in count else code)


def type_name(term):
    """A term as Pointer's pointee spells it, or None where Pointer's pointee
    is None."""
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
        return inner + ' *' if inner and '[' not in inner else None
    if not inner:
        return None
    # an array of arrays spells its own extent ahead of its element's
    element, bracket, extents = inner.partition('[')
    return f'{element}[{" ".join(term[2] or ())}]{bracket}{extents}'


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
    """(value, ctype), with value as ctype holds it: modulo 2**32 for an
    unsigned type, as C's unsigned arithmetic wraps, and unchanged for a
    signed one. text is the operation that gives the value, which an error
    names. Raises LayoutError where a signed value lies outside its type."""
    if ctype.startswith('unsigned'):
        return value % len(UNSIGNED), ctype
    if value not in INTEGER_TYPES[ctype]:
        raise LayoutError(f'{text}: {value} lies outside {ctype}')
    return value, ctype


def binary(op, left, right, text):
    """(value, type) of op on left and right, each (value, type). text is the
    operation, which an error names.

    Raises LayoutError where SHC could give another value or none:

    - an operand outside the signed type it converts to;
    - a signed result or quotient outside its type;
    - a shift by a count outside 0 to 31;
    - a division by zero."""
    (a, a_type), (b, b_type) = left, right
    if op in ('&&', '||'):
        # neither side converts to the other's type, so -1 && 1U is 1
        return int(bool(a or b) if op == '||' else bool(a and b)), 'int'
    if op in ('<<', '>>'):
        # a shift takes the left side's type, whatever the count's
        if b not in range(32):
            raise LayoutError(f'{text}: a shift by {b}, outside 0 to 31')
        return in_type(text, a << b if op == '<<' else a >> b, a_type)
    # both sides convert to the later of their types. An unsigned type wraps
    # them, and a signed one must hold them
    ctype = max(a_type, b_type, key=list(INTEGER_TYPES).index)
    a, b = (in_type(text, value, ctype)[0] for value in (a, b))
    if op in ('/', '%'):
        if not b:
            raise LayoutError(f'{text}: a division by zero')
        # SHC's quotient truncates toward zero. % fails where the quotient
        # lies outside the type, as / does
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
    """The declarations in texts, each a C source's or a header's contents.
    A name's definitions are gathered from them all, whatever C's scopes.

    Types represents a type as a term, one of:

    - ('base', type words);
    - ('typedef', name);
    - (kind, tag) for a struct, union or enum;
    - ('qualified', term, qualifiers);
    - ('pointer', term);
    - ('array', term, extent tokens), with None for the tokens of [];
    - ('function', term).

    A struct, union or enum with no tag takes '#file:index' as its tag. file
    is the index in files of the token list it lies in, and index that of
    its opening brace in that list.

    tools/float_fold.py reads these terms to follow an initializer through
    its object's type.
    """

    def __init__(self, texts):
        self.files = []  # each token list take() has read, at the index it returns
        self.macros = {}  # the distinct bodies of each object-like macro, as token tuples
        self.functions = set()  # the function-like macros, which no extent here expands
        # each enumerator's distinct (value tokens, enumerator before it) pairs. The tokens are None without an =,
        # and the enumerator before it is None with an = or for the first
        self.enumerators = {}
        self.typedefs = {}  # the distinct terms of each typedef
        self.definitions = {}  # the bodies of each struct, union and enum by (kind, tag), as (file, open, close)
        self.cache = {}
        self.laying = set()  # the terms layout() has begun and not finished
        for text in texts:
            self.read(text)

    def read(self, text):
        # tools/image.py and cdata.py take this module without csource.py,
        # which read() and spelled() alone need
        from csource import macro_directives
        for _, name, macro in macro_directives(text):
            if macro and macro[0] is not None:
                self.functions.add(name)
            elif macro:
                self.macros.setdefault(name, set()).add(tuple(macro[1]))
        self.take(spelled(text))

    def take(self, tokens):
        """Read the declarations in tokens, C's tokens without a directive,
        as a new file. Returns the file's index in files."""
        file = len(self.files)
        self.files.append(tokens)
        for index, token in enumerate(tokens):
            try:
                if token == 'typedef':
                    self.typedef(file, index + 1)
                elif token in ('struct', 'union', 'enum'):
                    self.specifiers(file, index)
            except (LayoutError, IndexError):
                # Types skips a declaration it can't read, as if it weren't there
                continue
        return file

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
            self.enumerators.setdefault(name, set()).add((value, before if value is None else None))
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
                if identifier(tokens[index]) and tokens[index] not in KEYWORDS:
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
            elif term is None and not words and identifier(token) and token not in KEYWORDS:
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

        derive() applies the derivations, in order, to the specifiers' type.
        Those of a declarator in parentheses apply to the type the declarator
        around it builds. Raises LayoutError for a parenthesis that holds no
        declarator, or a bracket that doesn't close."""
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
        elif identifier(tokens[index]) and tokens[index] not in KEYWORDS:
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
        LayoutError where it has no definition or two that differ, a member
        that is a bit-field or that Types can't read, or two members of one
        name, as the branches of a conditional inside it declare."""
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
                if any(member == each for each, _ in found):
                    raise LayoutError(f'{name}: two members named {member}')
                found.append((member, derive(base, derivations)))
                index += 1
                if tokens[index - 1] == ';':
                    break
        return found

    def term(self, ctype):
        """A type name's term. Raises LayoutError where ctype is no type
        name, or one that defines a struct, union or enum."""
        tokens = spelled(ctype) + [';']
        if '{' in tokens:
            # the definition would lie in tokens this call doesn't keep
            raise LayoutError(f'{ctype}: a type name with a body')
        index = len(self.files)
        self.files.append(tokens)
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
            if term in self.laying:
                raise LayoutError(f'{type_name(term) or "a type"}: holds itself')
            self.laying.add(term)
            try:
                self.cache[term] = self.lay(term)
            finally:
                self.laying.remove(term)
        return self.cache[term]

    def resolve(self, term):
        """term with its qualifiers dropped and its typedefs followed, down to
        a term that is neither. Raises LayoutError for a typedef with no type,
        one defined two ways, or one that leads back to itself."""
        seen = set()
        while term[0] in ('qualified', 'typedef'):
            if term[0] == 'qualified':
                term = term[1]
                continue
            if term[1] in seen:
                raise LayoutError(f'{term[1]}: leads back to itself')
            terms = self.typedefs.get(term[1])
            if not terms:
                raise LayoutError(f'{term[1]}: no type')
            if len(terms) > 1:
                raise LayoutError(f'{term[1]}: defined two ways')
            seen.add(term[1])
            term = next(iter(terms))
        return term

    def lay(self, term):
        term = self.resolve(term)
        kind = term[0]
        if kind == 'base':
            return basic(list(term[1]))
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
        """(value, type) of the expression at index, and the index where it
        ends. least is the lowest precedence in BINARY of an operator the
        expression takes, so a looser one ends it."""
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
