"""make gcc's fold of static initializers, for tools/floats.py: each one's
value as SHC folds it, written in its place where GCC could give another;
and under -aggressive=2, the divisions SHC computes otherwise, which it
refuses.

The tool folds each initializer of an object of static storage on the
tree's lines, at file scope and in a function, that holds a floating
constant or names a floating type, or that initializes a floating object
or one whose type it can't settle. It folds by SHC's rules, as the retail
values its tests cite bear them out: each operation's value rounds to its
type's format, one at a time in C's order, so 1.0F / 6.0F is 0x3e2aaaaa,
where GCC's is 0x3e2aaaab. It converts the value to the type of the object
it initializes, as SHC converts it. It follows that type, as
tools/layout.py's Types reads it, through the initializer's braces as C
does: a scalar, each element of an array, and each member of a struct and
a union's first.

The fold reads floating, integer and character constants, enumerators,
parentheses, casts, the unary + - ! and ~, the binary operators and ?:. It
reads a name as tools/float_scope.py's lookup() does, so a static const
object that hides an enumerator is an object, which it doesn't read. A
character constant is an int where it holds one character that plain char
holds signed and unsigned alike; the fold reads no other. Integers evaluate
as tools/layout.py evaluates SHC's integer constant expressions. An integer
converts to a floating type where C converts it, so (1 / 2) * 1.0F is 0.0F,
and the comparison 16777217 > 16777216.0F is 0: each release converts to
binary32 by truncating toward zero, and to binary64 exactly. A comparison,
!, && and || give an int, and ?: the arm its condition chooses, in the two
arms' type; both arms must fold. A cast to an integer type truncates a
floating operand toward zero and takes an integer one modulo an unsigned
type's range. The fold reads a double SHC holds as binary64 whole, rounds
each binary64 value as the release does, and cuts a value to GCC's format
only where it writes it.

Where GCC can give the value otherwise, as where the initializer holds
floating arithmetic, converts to its object's type with rounding, as
16777219 does to a float, or takes an integer that C17, below, types
otherwise, the tool writes the value in its place, in hexadecimal for a
floating type and in decimal for an integer type, and the lines it spans
stay. A call of one of GCC's infinities stays as written. The fold fails
on:

- anything floating it can't fold, such as a call, a name that names no
  enumerator there or a cast to a type Types doesn't read;
- an initializer it reads no value for, as where an address takes part,
  that a value GCC can give otherwise takes part in;
- a signed integer outside its type, a shift outside 0 to 31 and a
  division by zero;
- a value past its format, past the signed integer type it converts to, or
  a floating one past an unsigned type;
- a floating constant on a system header's line, which it doesn't read;
- an integer, a long double, a double SHC holds as binary64 or a floating
  value GCC can give otherwise, where it can't settle the type of the
  object it initializes, so that no conversion to the nearest passes
  unseen.

GCC reads C17, which types two integers otherwise than SHC: an unsuffixed
or L-suffixed decimal constant that int can't hold is a long long, where
SHC's is unsigned, and a long and an unsigned int convert to unsigned long,
where SHC's convert to long. So the tool fails on such a constant on the
tree's lines, as in x * -2147483648, which multiplies by -2**31 under GCC
and by 2**31 under SHC, but for one in an initializer the fold writes, and
one that, under a - or none, is the whole initializer of a static object
that isn't floating, which GCC converts to its type as SHC does.

Under -aggressive=2, SHC multiplies a floating value by a constant
divisor's reciprocal, truncated, where GCC divides: path_curve_sample's
FMUL by 0x3e2aaaaa at 0x8c09b0a4 in 1ST_READ.BIN takes curve.c's division
by 6.0F, and probes of R32 under the game's flags show it for a divisor
that is an int, a product of constants or negated. It folds a quotient of
constants as the division, as the fold does, in code as in a static
initializer, so 5.0F / 3.0F is 0x3fd55555 where the reciprocal gives
0x3fd55554. So under -aggressive=2, which the tool takes under R32 alone,
as the probes read it, the tool fails on each floating division on the
tree's lines of a function's code, a / or a /=, by a constant whose
reciprocal no normal float holds exactly, of a dividend that isn't
constant; and where it can't tell, as by sizeof, whose value the fold
doesn't read, or in C Reader doesn't read. It reads each full expression
tools/float_scope.py's walk gives, inside the brackets Reader takes whole,
and no division in a type name, as in an array's extent there.
"""

import re
from fractions import Fraction

from csource import SIZES
from float_expr import Reader
from float_scope import NAMES, Integer, category
from float_tokens import quoted
from float_value import FLOAT, HEXADECIMAL, TYPES, Past, floating, held, rounded, spelled, typed
from layout import INTEGER_TYPES, LayoutError, binary, in_type, number

# GCC's builtins for infinity, whose value is the same under any rounding
INFINITIES = ('__builtin_inff', '__builtin_inf', '__builtin_infl', '__builtin_huge_valf', '__builtin_huge_val',
              '__builtin_huge_vall')
# the comparisons, of floating values as of integers
COMPARISONS = {'<': Fraction.__lt__, '>': Fraction.__gt__, '<=': Fraction.__le__, '>=': Fraction.__ge__,
               '==': Fraction.__eq__, '!=': Fraction.__ne__}
# the arithmetic operators, of floating values
ARITHMETIC = {'+': Fraction.__add__, '-': Fraction.__sub__, '*': Fraction.__mul__, '/': Fraction.__truediv__}
# by its struct code, the values an integer type holds: plain char's those
# it holds signed and unsigned alike
VALUES = {'b': range(-0x80, 0x80), 'B': range(0x100), 'c': range(0x80), 'h': range(-0x8000, 0x8000),
          'H': range(0x10000), 'i': INTEGER_TYPES['int'], 'I': INTEGER_TYPES['unsigned int']}
# the integer types that SHC's usual arithmetic conversions give long, and
# C17's unsigned long
MIXED = {'long', 'unsigned int'}
# a character constant of one character: itself, an octal or a hexadecimal
# escape, or a simple escape, whose character ESCAPES gives
CHARACTER = re.compile(r"'(?:([^\\'\n])|\\([0-7]{1,3})|\\x([0-9a-fA-F]+)|\\(['\"?\\abfnrtv]))'")
ESCAPES = dict(zip('\'"?\\abfnrtv', b'\'"?\\\a\b\f\n\r\t\v'))


class Unsettled(Exception):
    """An initializer the fold can't follow along its object's type, and why."""


def scalar(term):
    """Whether a type's term, its typedefs resolved, is a scalar."""
    return term[0] in ('base', 'enum', 'pointer')


def widened(text):
    """Whether text, a pp-number, is a decimal constant, unsuffixed or
    L-suffixed, that int can't hold, which C17 makes a long long, where SHC
    makes it unsigned."""
    return bool(re.fullmatch(r'[1-9][0-9]*[lL]?', text)) and int(text.rstrip('lL')) not in VALUES['i']


class Initializer:
    """The scalar initializers of an object's initializer, along the object's
    type as C's rules for its braces give them. found holds each as (start,
    end, term, why): the index of its first token and the one past its
    last, the Types term of the type it initializes and None, or None and
    why the fold can't settle that type."""

    def __init__(self, toks, types):
        self.toks, self.types, self.found = toks, types, []

    def text(self, at):
        return self.toks[at].text

    def strings(self, at):
        """The index past the string literals at toks[at], at itself where none lies there."""
        while self.toks[at].kind == 'string' and self.text(at)[-1] == '"':
            at += 1
        return at

    def read(self, at, term):
        """Read the initializer at toks[at] of an object of term into found,
        and return the index past it. Where the fold can't follow it along
        term, each of its expressions goes into found with no type, and
        why."""
        mark = len(self.found)
        try:
            return self.one(at, term)
        except (Unsettled, LayoutError) as error:
            del self.found[mark:]
            end = self.toks.ending(at)
            self.unsettled(at, end, str(error))
            return end

    def unsettled(self, start, end, why):
        """Take each expression of toks[start:end] as one whose type the fold
        can't settle, for why."""
        self.found += [(at, stop, None, why) for at, stop in self.toks.listed(start, end)]

    def one(self, at, term):
        """Read one initializer at toks[at], of an object of term, and return
        the index past it."""
        term = self.types.resolve(term)
        if term[0] == 'function':
            raise Unsettled('a function, which no initializer initializes')
        if self.text(at) in ('.', '['):
            raise Unsettled('a designator, which the fold doesn\'t follow')
        characters = term[0] == 'array' and self.characters(term)
        if self.text(at) == '{':
            close = self.toks.closing(at)
            if scalar(term):
                end = self.toks.ending(at + 1)
                self.found.append((at + 1, end, term, None))
            elif characters and self.strings(at + 1) > at + 1:
                end = self.strings(at + 1)
            else:
                end = self.fill(at + 1, term, True)
            end += self.text(end) == ','
            if end != close:
                raise Unsettled('more initializers than its braces\' object holds')
            return close + 1
        if scalar(term):
            end = self.toks.ending(at)
            self.found.append((at, end, term, None))
            return end
        if characters and self.strings(at) > at:
            return self.strings(at)
        return self.fill(at, term, False)

    def characters(self, term):
        """Whether term, an array, is one of a character type, which a string initializes."""
        element = self.types.resolve(term[1])
        return element[0] == 'base' and 'char' in element[1]

    def fill(self, at, term, braced):
        """Initialize the members of term, an aggregate, from the list at
        toks[at]: to the list's } where braced, else as many as term holds.
        Return the index past the last initializer it takes."""
        if term[0] == 'array':
            try:
                count = None if term[2] is None else self.types.evaluate(term[2])
            except LayoutError:
                count = None
            if count is None and not braced:
                raise Unsettled('an array whose extent the fold can\'t read, its initializer in no braces of its own')
        else:
            of = [member for _, member in self.types.members(term[0], term[1])]
            # a union's initializer initializes its first member
            of = of[:1] if term[0] == 'union' else of
            count = len(of)
        index = 0
        while count is None or index < count:
            if index:
                if self.text(at) != ',' or self.text(at + 1) == '}':
                    return at
                at += 1
            if self.text(at) == '}':
                return at
            at = self.one(at, term[1] if term[0] == 'array' else of[index])
            index += 1
        return at


def infinite(node):
    """Whether node is a call of one of INFINITIES with no argument, in any
    parentheses and under any + or -."""
    while node.kind == 'parentheses' or node.kind == 'unary' and node.what in '+-':
        node = node.operands[0]
    return (node.kind == 'opaque' and node.what == 'a call' and len(node.operands) == 1
            and node.operands[0].kind == 'name' and node.operands[0].what in INFINITIES)


class Value:
    """A value the fold reads: its type, a floating type or one of
    INTEGER_TYPES, and its number, a Fraction or an int. A floating value's
    sign is apart, for a zero's. rewrite says GCC can give the value
    otherwise, so that the fold writes the value of an initializer it takes
    part in: where an operation or a conversion that rounds gave it, or
    where an integer type that C17 gives otherwise than SHC took part."""

    __slots__ = ('type', 'number', 'negative', 'rewrite')

    def __init__(self, type, number, negative=False, rewrite=False):
        self.type, self.number, self.negative, self.rewrite = type, number, negative, rewrite

    @property
    def floating(self):
        return self.type in TYPES


class Unread:
    """A value the fold doesn't read, what it is, and whether it is an
    integer, where it is no address or string."""

    def __init__(self, what, integer=False):
        self.what, self.integer = what, integer


class Fold:
    """SHC's folder under route, for the Nodes of toks, whose names scopes,
    the Scopes of toks, reads: an expression's value. saw_rewrite says a
    value that needs a rewrite took part in one that fold() gave, an Unread
    among them, so each initializer takes a Fold of its own."""

    def __init__(self, toks, scopes, route):
        self.toks, self.scopes, self.types, self.route, self.saw_rewrite = toks, scopes, scopes.types, route, False

    def floats(self, node):
        """Whether node holds a floating constant, or names a floating type."""
        if node.kind == 'number':
            return floating(node.what)
        if node.kind in ('cast', 'type'):
            try:
                if category(self.types, node.term) in TYPES:
                    return True
            except LayoutError:
                pass
        return any(self.floats(operand) for operand in node.operands)

    def needs_rewrite(self, *values):
        """Whether GCC can give another value than SHC for anything the values
        give: where one's rewrite says so, or SHC holds it wider than GCC
        does."""
        return any(value.rewrite or value.floating and self.route.wider(value.type) for value in values)

    def fold(self, node):
        """node's value, as value() gives it, noting in saw_rewrite where it
        needs a rewrite."""
        value = self.value(node)
        if isinstance(value, Value) and self.needs_rewrite(value):
            self.saw_rewrite = True
        return value

    def value(self, node):
        """node's value as SHC's folder gives it, or an Unread. A ValueError
        says why it has none: something floating the fold can't fold, or an
        operation or a conversion that gives SHC's folder no value."""
        if node.kind == 'number':
            return self.constant(node.start)
        if node.kind == 'character':
            return self.character(node.what)
        if node.kind == 'parentheses':
            return self.value(node.operands[0])
        if node.kind == 'name':
            try:
                found = self.scopes.lookup(node.what, node.start)
            except LayoutError as error:
                return Unread(str(error), integer=True)
            if found.kind != 'enumerator':
                return Unread(f'{node.what}, a name the fold can\'t read')
            try:
                number_, type = self.types.enumerator(node.what, frozenset({node.what}))
            except LayoutError as error:
                return Unread(f'{node.what}, an enumerator the fold can\'t read: {error}', integer=True)
            return Value(type, number_)
        if node.kind == 'cast':
            return self.cast(node)
        if node.kind == 'unary':
            return self.unary(node)
        if node.kind == 'binary':
            return self.binary(node)
        if node.kind == 'conditional':
            return self.conditional(node)
        # a string, a type name, or an opaque construct
        if self.floats(node):
            raise ValueError(node.what)
        return Unread('a string' if node.kind == 'string' else node.what, integer=node.what in SIZES)

    def constant(self, at):
        """The Value of the pp-number at toks[at]: a floating constant's, as
        tools/floats.py reads it into toks' values for one on the tree's
        lines; an integer constant's, as SHC types it, a rewrite where it is
        an unsuffixed or L-suffixed decimal one that int can't hold, which C17
        types long long and SHC unsigned; or an Unread where SHC holds no
        integer constant so wide. A ValueError says why it is no constant, or
        that a system header's line holds a floating one."""
        text = self.toks[at].text
        if floating(text):
            if at not in self.toks.values:
                raise ValueError(f'{text}, a floating constant on a system header\'s line, which the fold doesn\'t '
                                 'read')
            return Value(typed(text), self.toks.values[at])
        try:
            value, type = number(text)
        except LayoutError as error:
            if not re.fullmatch(r'(0[xX][0-9a-fA-F]+|[0-9]+)[uUlL]*', text):
                raise ValueError(f'{text}, no constant C spells') from None
            return Unread(str(error), integer=True)
        return Value(type, value, rewrite=widened(text))

    def character(self, text):
        """The Value of a character constant, an int, where it holds one
        character that plain char holds signed and unsigned alike, else an
        Unread naming it."""
        parts = CHARACTER.fullmatch(text)
        if parts:
            itself, octal, hexadecimal_, escape = parts.groups()
            if itself:
                code = ord(itself)
            elif escape:
                code = ESCAPES[escape]
            else:
                code = int(octal, 8) if octal else int(hexadecimal_, 16)
            if code in VALUES['c']:
                return Value('int', code)
        return Unread(f'{text}, a character constant the fold can\'t read', integer=True)

    def converted(self, value, type):
        """value converted to a floating type as SHC converts it: to binary32 by
        truncating it toward zero, and to binary64 exactly. It needs a rewrite
        where value does, where it differs from value as GCC's format holds
        it, which GCC converts, and where SHC holds the type as binary64 and no
        float holds it, which GCC's float double rounds to the nearest."""
        if value.type == type:
            return value
        form = self.route.formats[type]
        number_ = rounded(Fraction(value.number), FLOAT if form.bits == FLOAT.bits else form)
        negative = value.number < 0 or value.floating and value.negative
        source = held(value.number, value.type, self.route) if value.floating else value.number
        rewrite = value.rewrite or number_ != source or \
            self.route.wider(type) and held(number_, type, self.route) != number_
        return Value(type, number_, negative, rewrite=rewrite)

    def integer(self, value, target, what):
        """value converted to an Integer type, a floating value truncated
        toward zero, and an integer modulo an unsigned type's range, as C
        converts it. A ValueError says where the type can't hold it."""
        number_ = int(value.number)
        if not value.floating and target.code in ('B', 'H', 'I'):
            number_ %= len(VALUES[target.code])
        if number_ not in VALUES[target.code]:
            raise ValueError(f'{what}, which can\'t hold {value.number if value.floating else number_}')
        return Value(target.type, number_, rewrite=self.needs_rewrite(value))

    def cast(self, node):
        operand = node.operands[0]
        try:
            target = category(self.types, node.term)
        except LayoutError as error:
            if self.floats(operand):
                raise ValueError(f'a cast to {node.what}, a type the fold can\'t read: {error}') from None
            return Unread(f'a cast to {node.what}')
        if target in TYPES:
            value = self.value(operand)
            if isinstance(value, Unread):
                raise ValueError(value.what)
            return self.converted(value, target)
        if isinstance(target, Integer):
            value = self.fold(operand)
            if isinstance(value, Unread):
                return Unread(f'a cast to {node.what}', integer=True)
            return self.integer(value, target, f'a cast to {node.what}')
        if self.floats(operand):
            raise ValueError(f'a cast to {node.what}')
        return Unread(f'a cast to {node.what}')

    def unary(self, node):
        operator, operand = node.what, node.operands[0]
        if operator not in ('-', '+', '!', '~'):
            # & * ++ --, which give an address or no constant
            if self.floats(operand):
                raise ValueError(f'{operator}, an operator the fold doesn\'t take')
            return Unread(f'{operator}, an operator the fold doesn\'t take')
        value = self.fold(operand) if operator == '!' else self.value(operand)
        if isinstance(value, Unread):
            return Unread(value.what, integer=value.integer or operator == '!')
        if operator == '!':
            return Value('int', int(not value.number), rewrite=self.needs_rewrite(value))
        if value.floating:
            if operator == '~':
                raise ValueError('~, which takes no floating operand')
            if operator == '+':
                return value
            return Value(value.type, -value.number, not value.negative, rewrite=value.rewrite)
        # an integer, which - and ~ give in its type, as SHC's integers do
        number_ = {'-': -value.number, '+': value.number, '~': ~value.number}[operator]
        try:
            number_, type = in_type(f'{operator}{value.number}', number_, value.type)
        except LayoutError as error:
            raise ValueError(str(error)) from None
        return Value(type, number_, rewrite=value.rewrite)

    def binary(self, node):
        operator = node.what
        values = [self.fold(operand) for operand in node.operands]
        if any(isinstance(value, Value) and value.floating for value in values):
            for value in values:
                if isinstance(value, Unread):
                    raise ValueError(value.what)
            if operator in ('+', '-', '*', '/'):
                return self.arithmetic(operator, *values)
            if operator in COMPARISONS:
                a, b = self.usual(*values)
                return Value('int', int(COMPARISONS[operator](a.number, b.number)), rewrite=self.needs_rewrite(a, b))
            a, b = (Fraction(value.number) for value in values)
            if operator in ('&&', '||'):
                truth = bool(a) and bool(b) if operator == '&&' else bool(a) or bool(b)
                return Value('int', int(truth), rewrite=self.needs_rewrite(*values))
            raise ValueError(f'{operator}, which takes no floating operand')
        unread = [value for value in values if isinstance(value, Unread)]
        if unread:
            return Unread(unread[0].what, integer=operator not in ('+', '-') or all(value.integer for value in unread))
        text = ' '.join(token.text for token in self.toks[node.start:node.end])
        try:
            number_, type = binary(operator, (values[0].number, values[0].type), (values[1].number, values[1].type),
                                   text)
        except LayoutError as error:
            raise ValueError(str(error)) from None
        # C17 converts the MIXED types otherwise, but for a shift and a logical
        # operator, which convert neither
        mixed = operator not in ('<<', '>>', '&&', '||') and {value.type for value in values} == MIXED
        return Value(type, number_, rewrite=self.needs_rewrite(*values) or mixed)

    def conditional(self, node):
        # the condition chooses an arm, and both arms fold
        condition = self.fold(node.operands[0])
        arms = [self.fold(arm) for arm in node.operands[1:]]
        values = [condition, *arms]
        if any(isinstance(value, Value) and value.floating for value in values):
            for value in values:
                if isinstance(value, Unread):
                    raise ValueError(value.what)
        unread = [value for value in values if isinstance(value, Unread)]
        if unread:
            return Unread(unread[0].what, integer=all(value.integer for value in unread))
        chosen = arms[0] if condition.number else arms[1]
        rewrite = self.needs_rewrite(condition) or chosen.rewrite
        if any(value.floating for value in arms):
            # the arms' type, as C's usual arithmetic conversions give it
            type = max((value.type for value in arms if value.floating), key=list(TYPES).index)
            converted = self.converted(chosen, type)
            return Value(type, converted.number, converted.negative, rewrite=rewrite or converted.rewrite)
        type = max((value.type for value in arms), key=list(INTEGER_TYPES).index)
        rewrite = rewrite or {value.type for value in arms} == MIXED
        try:
            number_, type = in_type('?:', chosen.number, type)
        except LayoutError as error:
            raise ValueError(str(error)) from None
        return Value(type, number_, rewrite=rewrite)

    def usual(self, a, b):
        """a and b, one of them floating, converted to the type C's usual
        arithmetic conversions give them."""
        type = max((value.type for value in (a, b) if value.floating), key=list(TYPES).index)
        return self.converted(a, type), self.converted(b, type)

    def arithmetic(self, operator, a, b):
        """a operator b, for + - * /, under C's usual arithmetic conversions: its
        exact value rounded to its type's format, as SHC's folder gives it. A
        ValueError says why it gives none."""
        a, b = self.usual(a, b)
        type = a.type
        if operator == '/' and not b.number:
            raise ValueError('a division by zero')
        exact = ARITHMETIC[operator](a.number, b.number)
        if exact or operator in ('*', '/'):
            # a product's or a quotient's sign is its operands' together, a zero's too
            negative = exact < 0 if exact else a.negative != b.negative
        else:
            # a sum of zeros is negative only where both are, under truncation as
            # under rounding to the nearest; any other zero sum is positive
            negative = not a.number and not b.number and a.negative and (b.negative != (operator == '-'))
        return Value(type, rounded(exact, self.route.formats[type]), negative, rewrite=True)

    def settle(self, node, target, why):
        """The text that takes the place of the initializer expression whose
        node it is, or None where it keeps its own; target is the Types term
        of the type the expression initializes, or None, for why. A
        ValueError says where SHC's conversion gives no value the fold
        writes; where the fold reads no value for the whole and one GCC can
        give otherwise takes part; or where it can't settle the type that an
        integer, a long double, a double SHC holds as binary64 or a floating
        value GCC can give otherwise converts to. A call of one of
        INFINITIES keeps its own, since GCC gives its value exactly."""
        if infinite(node):
            return None
        floats = self.floats(node)
        if target is not None:
            try:
                target = category(self.types, target)
            except LayoutError as error:
                target, why = None, str(error)
            if not floats and target not in TYPES and target is not None:
                # no floating value converts there
                return None
        value = self.fold(node)
        if isinstance(value, Unread) and self.saw_rewrite:
            raise ValueError(f'{value.what}, beside a value GCC can give otherwise')
        if target is None:
            if isinstance(value, Unread) and value.integer or isinstance(value, Value) and (
                    not value.floating or value.type == 'long double' or self.needs_rewrite(value)):
                what = value.what if isinstance(value, Unread) else 'an integer' if not value.floating else \
                    f'a {value.type}'
                raise ValueError(f'{what}, which converts to a type the fold can\'t settle: {why}')
            return None
        if target in TYPES:
            if isinstance(value, Unread):
                raise ValueError(f'{value.what}, which converts to {target}')
            value = self.converted(value, target)
            if value.rewrite:
                return spelled(value, self.route)
            return None
        if isinstance(value, Unread):
            return None
        if target == 'pointer' and value.floating:
            raise ValueError(f'a {value.type}, which converts to a pointer')
        if isinstance(target, Integer):
            value = self.integer(value, target, f'a conversion to {NAMES.get(target.code, target.type)}')
            if value.rewrite:
                return str(value.number)
        return None


def fold(toks, scopes, route, plan):
    """Write into plan, as SHC folds it under route, each initializer of an
    object of static storage on the tree's lines of toks that GCC could give
    otherwise, as scopes, the Scopes of toks, gives them. toks' values hold
    each floating constant on the tree's lines, as tools/floats.py reads
    them. Return the problems, each naming its file and line: each
    initializer the fold can't settle, each decimal constant C17 types
    otherwise, and under -aggressive=2, each division reciprocals()
    refuses."""
    problems, settled = [], set()
    for start, end, term, why in scopes.statics:
        found = Initializer(toks, scopes.types)
        if term is None:
            found.unsettled(start, end, why)
        else:
            found.read(start, term)
        for start, end, target, why in found.found:
            leaf = toks[start:end]
            if not leaf:
                continue
            if plain(leaf, target, scopes.types):
                # a constant that, under a - or none, is the whole initializer
                # of an object that isn't floating, which GCC converts to its
                # type as SHC does
                if len(leaf) - (leaf[0].text == '-') == 1:
                    settled.add(end - 1)
                continue
            expression = Fold(toks, scopes, route)
            reader = Reader(toks, scopes.types, scopes.typedef_at, start, end)
            try:
                replacement = expression.settle(reader.read(), target, why)
            except (ValueError, LayoutError) as error:
                problems.append(f'{toks.where(start)}: {quoted(leaf)}: {error}')
                continue
            if replacement is not None:
                settled.update(range(start, end))
                plan.edit(start, end, [replacement])
    # each decimal constant on the tree's lines that int can't hold, but
    # those settled
    for at, token in enumerate(toks):
        if token.kind == 'number' and not toks.system[at] and widened(token.text) and at not in settled:
            problems.append(f'{toks.where(at)}: {token.text}: a decimal constant that int can\'t hold, which '
                            'C17 makes a long long, where SHC makes it unsigned')
    if route.reciprocal:
        problems += reciprocals(toks, scopes, route)
    return problems


def quotients(toks, scopes):
    """Each division of the full expressions scopes gives of toks, a / or a
    /= as Reader reads it, and inside each bracket that Reader takes whole
    but a statement expression's, whose own the walk gives; and the span of
    each part of an expression Reader doesn't read."""
    found, unread, spans = [], [], list(scopes.expressions)
    while spans:
        start, end = spans.pop()
        reader = Reader(toks, scopes.types, scopes.typedef_at, start, end)
        nodes = [reader.full()]
        if reader.at < end:
            unread.append((reader.at, end))
        while nodes:
            node = nodes.pop()
            if node.kind == 'opaque' and node.what in ('a call', 'a subscript'):
                nodes.append(node.operands[0])
                spans.append((node.operands[0].end + 1, node.end - 1))
            elif node.kind == 'opaque' and node.what == '(':
                if toks[node.start + 1].text != '{':
                    spans.append((node.start + 1, node.end - 1))
            elif node.kind == 'opaque' and node.what == 'a compound literal':
                spans += toks.listed(toks.closing(node.start) + 1, node.end)
            else:
                if node.kind == 'binary' and node.what == '/' or node.kind == 'assign' and node.what == '/=':
                    found.append(node)
                nodes += node.operands
    return found, unread


def reciprocals(toks, scopes, route):
    """The problems of the code scopes walks in toks, under route, which
    holds -aggressive=2: each division on the tree's lines by a constant
    whose reciprocal no normal float holds exactly, of a floating value that
    isn't constant, which SHC multiplies by that reciprocal truncated, and
    each division the tool can't read."""
    problems, fold = [], Fold(toks, scopes, route)
    found, unread = quotients(toks, scopes)
    for start, end in unread:
        if any(toks[at].text in ('/', '/=') and not toks.system[at] for at in range(start, end)):
            problems.append((start, f'{toks.where(start)}: {quoted(toks[start:end])}: C Reader '
                                    'doesn\'t read, which holds a division'))
    for node in found:
        dividend, divisor = node.operands
        if toks.system[dividend.end]:
            continue
        where = f'{toks.where(node.start)}: {quoted(toks[node.start:node.end])}: a division by a constant'
        try:
            # the quotient of a / is of its operands' type, and a /= divides in
            # its operands' too
            quotient = scopes.floating(node) if node.kind == 'binary' else \
                scopes.floating(dividend) or scopes.floating(divisor)
            if not quotient or not scopes.constant(divisor) or node.kind == 'binary' and scopes.constant(dividend):
                continue
            value = fold.value(divisor)
            if not isinstance(value, Value):
                raise ValueError(value.what)
            if reciprocated(value):
                continue
        except (LayoutError, ValueError) as error:
            problems.append((node.start, f'{where} the tool can\'t read, under -aggressive=2: {error}'))
            continue
        problems.append((node.start, f'{where} whose reciprocal no normal float holds exactly, which -aggressive=2 '
                                     'makes SHC multiply by'))
    return [problem for _, problem in sorted(problems)]


def reciprocated(value):
    """Whether a normal float holds the reciprocal of value, a Value,
    exactly."""
    if not value.number:
        return False
    reciprocal = 1 / Fraction(value.number)
    try:
        return rounded(reciprocal, FLOAT) == reciprocal
    except Past:
        return False


def plain(leaf, target, types):
    """Whether an initializer expression's tokens, of the type whose Types
    term target is, leave nothing to fold: where the type is no floating
    one, no floating constant, float, double or typedef name among them;
    where it is, one constant under a - or none, a hexadecimal one of the
    type, whose value tools/floats.py checks that GCC's format holds
    exactly, or an integer whose value, negated in its type under the -,
    every format holds."""
    try:
        kind = None if target is None else category(types, target)
    except LayoutError:
        return False
    if kind is None:
        return False
    if kind not in TYPES:
        return not any(token.kind == 'number' and floating(token.text) or token.text in ('float', 'double')
                       or token.text in types.typedefs for token in leaf)
    if len(leaf) - (leaf[0].text == '-') != 1 or leaf[-1].kind != 'number':
        return False
    parts = HEXADECIMAL.fullmatch(leaf[-1].text)
    if parts:
        return TYPES[kind] == parts[4].upper()
    try:
        value, type = number(leaf[-1].text)
    except LayoutError:
        return False
    if leaf[0].text == '-':
        # an unsigned one wraps, as -1U does to 4294967295
        value = in_type('-', -value, type)[0]
    return abs(value) < 2 ** FLOAT.bits
