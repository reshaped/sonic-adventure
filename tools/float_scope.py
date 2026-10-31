"""The declarations of GCC's preprocessed C and the types of its
expressions, for tools/floats.py. Scopes is tools/csource.py's Walk of the
declarations and each function's statements, with the types
tools/layout.py's Types reads. It binds each name in the scope Walk gives
it, and gives the fold each initializer of an object of static storage and
each full expression of a function's code. It reads an expression's type
from the declarations in scope: a name's, a member's in its struct's or
union's definition, and a call's in its function's. category() names what
a type is.

lookup() gives what a name names by the innermost declaration in scope: an
object or a function, a typedef or an enumerator, so an object hides an
enumerator or a typedef of its name. It fails where no declaration in scope
names it, or where one in a block that the walk can't read can hide it.
typedef_at() tells Reader and the walk whether a name names a typedef by
the same declarations, passing over those the walk can't read. Where the
walk reads no declaration of the name, it goes by the typedefs Types reads.
"""

from collections import namedtuple

from csource import ASSIGNMENTS, SIZES, Unread, Walk
from float_tokens import quoted
from float_value import TYPES, floating, typed
from layout import LayoutError, basic, derive, type_name

# an integer type, by the struct code tools/layout.py's basic() gives it, and
# the type among INTEGER_TYPES that C's usual arithmetic conversions give it
Integer = namedtuple('Integer', 'code type')
# by its struct code, each integer type narrower than int, and the name a
# message gives it
NAMES = {'b': 'signed char', 'B': 'unsigned char', 'c': 'char', 'h': 'short', 'H': 'unsigned short'}
# what one declaration binds a name to: an 'object', which is an object or a
# function, a 'typedef' or an 'enumerator'; the Types term of its type, int
# for an enumerator; and the index where its scope opens and the one where
# it closes
Binding = namedtuple('Binding', 'kind term start end')
# the volatile qualifiers, and every qualifier a declarator can spell after a *
VOLATILE = {'volatile', '__volatile', '__volatile__'}
POINTER_QUALIFIERS = VOLATILE | {'const', '__const', '__const__', 'restrict', '__restrict', '__restrict__'}
# the Types terms of int and of each floating type
INT = ('base', ('int',))
FLOATING = {'float': ('base', ('float',)), 'double': ('base', ('double',)), 'long double': ('base', ('long', 'double'))}
# the type each floating builtin the tree's code calls returns, which GCC
# declares itself
BUILTINS = {'__builtin_fabsf': FLOATING['float'], '__builtin_sqrtf': FLOATING['float'],
            '__builtin_fmaf': FLOATING['float']}


def category(types, term):
    """What a type's term names once Types resolves it: a floating type, an
    Integer, 'pointer', or 'other' for an aggregate, a function or void. A
    LayoutError says where Types can't read it."""
    if isinstance(term, LayoutError):
        raise term
    term = types.resolve(term)
    if term[0] == 'enum':
        return Integer('i', 'int')
    if term[0] == 'pointer':
        return 'pointer'
    if term[0] != 'base':
        return 'other'
    words = list(term[1])
    if 'float' in words:
        return 'float'
    if 'double' in words:
        return 'long double' if 'long' in words else 'double'
    if 'void' in words:
        return 'other'
    code = basic(words).code
    if code in NAMES:
        return Integer(code, 'int')
    return Integer(code, ('unsigned ' if code == 'I' else '') + ('long' if 'long' in words else 'int'))


def shown(types, node):
    """Whether C shows node's value is floating: a floating constant, a cast
    to a floating type, or arithmetic or a conditional that one takes part
    in."""
    if node.kind == 'number':
        return floating(node.what)
    if node.kind == 'cast':
        try:
            return category(types, node.term) in TYPES
        except LayoutError:
            return False
    if node.kind == 'binary' and node.what not in ('+', '-', '*', '/'):
        return False
    if node.kind == 'unary' and node.what not in ('-', '+'):
        return False
    operands = node.operands[1:] if node.kind == 'conditional' else node.operands
    return node.kind in ('parentheses', 'unary', 'binary', 'conditional') and any(
        shown(types, operand) for operand in operands)


class Scopes(Walk):
    """The Walk of toks, GCC's preprocessed C as marked() gives it, whose
    texts types.take() read as file, and the types of the expressions that
    name what toks declares. Beside the walk's unread, bodies, declarators
    and expressions:

    - names: for each name, the Binding of each declaration of it;
    - statics: each initializer of an object of static storage on the
      tree's lines, at file scope and in a block whose declaration says
      static, as (start, end, term, why): the index of its first token and
      the one past its last, the Types term of its object's type and None,
      or None and why the walk can't read the declaration;
    - volatile_pointers: each declaration whose declarator qualifies a
      pointer volatile after its *, which Types doesn't keep. An object's
      entry is its name and where its scope opens, a typedef's its name, and
      a member's, once member() reads it, its struct's or union's term[:2]
      and its own name."""

    def __init__(self, toks, types, file):
        self.toks, self.types, self.file = toks, types, file
        self.names, self.statics, self.volatile_pointers = {}, [], set()
        super().__init__(toks)

    def specifiers(self, at):
        try:
            return self.types.specifiers(self.file, at)
        except LayoutError as error:
            raise Unread(error) from None

    def declarator(self, at, base):
        try:
            name, derivations, at = self.types.declarator(self.file, at)
        except LayoutError as error:
            raise Unread(error) from None
        return name, derive(base, derivations), bool(derivations) and derivations[-1][0] == 'function', at

    def declared(self, name, kind, term, span, start, end):
        self.names.setdefault(name, []).append(Binding(kind, term, start, end))
        if volatile_pointer([token.text for token in self.toks[span[0]:span[1]]]):
            self.volatile_pointers.add((name, start) if kind == 'object' else name)

    def enumerated(self, name, start, end):
        self.names.setdefault(name, []).append(Binding('enumerator', INT, start, end))

    def static(self, start, end, term, why):
        if not self.toks.system[start]:
            self.statics.append((start, end, term, why))

    def binding(self, name, at):
        """The Binding of the innermost declaration of name in scope at
        toks[at] that the walk reads, or None."""
        return max((found for found in self.names.get(name, ()) if found.start <= at < found.end),
                   key=lambda found: found.start, default=None)

    def lookup(self, name, at):
        """The Binding of the declaration of name in scope at toks[at], the
        innermost. A LayoutError says where a declaration the walk can't read
        can hide it, or no declaration in scope names it."""
        found = self.binding(name, at)
        for start, end, why in self.unread:
            if start <= at < end and (found is None or start > found.start):
                raise LayoutError(f'{name}, in the scope of a declaration the tool can\'t read: {why}')
        if found is None:
            raise LayoutError(f'{name}, which no declaration in scope names')
        return found

    def typedef_at(self, name, at):
        """Whether name names a typedef at toks[at]: whether the innermost
        declaration of it in scope there that the walk reads is a typedef,
        or, where the walk reads none, Types reads a typedef of it."""
        found = self.binding(name, at)
        return name in self.types.typedefs if found is None else found.kind == 'typedef'

    def constant(self, node):
        """Whether node is a constant expression SHC folds: numbers, character
        constants and enumerators, sizeof, and the casts and operators that
        take them, but a ?:, && or || with an operand C shows floating, which
        SHC applies as an operand that isn't constant. A name in BUILTINS
        names a function. A LayoutError says lookup() can't tell what another
        name names, where no other operand shows node isn't constant."""
        if node.kind in ('number', 'character'):
            return True
        if node.kind == 'name':
            return node.what not in BUILTINS and self.lookup(node.what, node.start).kind == 'enumerator'
        if node.kind == 'opaque':
            return node.what in SIZES
        if node.kind == 'unary' and node.what not in ('-', '+', '!', '~'):
            return False
        if (node.kind == 'conditional' or node.kind == 'binary' and node.what in ('&&', '||')) and any(
                shown(self.types, operand) for operand in node.operands):
            return False
        if node.kind not in ('parentheses', 'cast', 'unary', 'binary', 'conditional'):
            return False
        errors = []
        for operand in node.operands:
            try:
                if not self.constant(operand):
                    return False
            except LayoutError as error:
                errors.append(error)
        if errors:
            raise errors[0]
        return True

    def floating(self, node):
        """Whether node's value is floating. A LayoutError says why the tool
        can't tell."""
        return category(self.types, self.type(node)) in TYPES

    def type(self, node):
        """The Types term of node's type, its outer qualifiers and typedefs
        resolved, as far as the tool reads it: a floating type, a pointer's,
        an array's, a function's, a struct's or a union's. An integer's is
        its declared type for a name, a cast, a member or a call, and int for
        the rest. A LayoutError says why the tool can't read it."""
        resolve, operands = self.types.resolve, node.operands
        if node.kind == 'number':
            return FLOATING[typed(node.what)] if floating(node.what) else INT
        if node.kind == 'character':
            return INT
        if node.kind == 'string':
            return ('pointer', ('base', ('char',)))
        if node.kind == 'name':
            if node.what in BUILTINS:
                return ('function', BUILTINS[node.what])
            found = self.lookup(node.what, node.start)
            if found.kind == 'typedef':
                raise LayoutError(f'{node.what}, a typedef, where an expression stands')
            return INT if found.kind == 'enumerator' else resolve(found.term)
        if node.kind == 'parentheses':
            return self.type(operands[0])
        if node.kind == 'cast':
            if isinstance(node.term, LayoutError):
                raise node.term
            return resolve(node.term)
        if node.kind == 'unary':
            if node.what == '!':
                return INT
            if node.what == '&':
                return ('pointer', self.type(operands[0]))
            term = self.type(operands[0])
            return self.pointee(term, node) if node.what == '*' else term
        if node.kind == 'binary':
            if node.what in ('+', '-', '*', '/'):
                return self.usual(node, operands)
            return INT
        if node.kind == 'conditional':
            return self.usual(node, operands[1:])
        operator = node.what.partition(',')[0]
        if operator == 'a call':
            term = self.type(operands[0])
            term = resolve(term[1]) if term[0] == 'pointer' else term
            if term[0] != 'function':
                raise LayoutError(f'{quoted(self.toks[node.start:node.end])}, a call of no function')
            return resolve(term[1])
        if operator == 'a subscript':
            return self.pointee(self.type(operands[0]), node)
        if operator in ('.', '->'):
            term = self.type(operands[0])
            return resolve(self.member(self.pointee(term, node) if operator == '->' else term,
                                       self.toks[node.end - 1].text))
        if operator in ('++', '--'):
            return self.type(operands[0])
        if operator in SIZES:
            return INT
        if operator == 'a compound literal':
            return resolve(self.types.term(' '.join(token.text for token in
                                                    self.toks[node.start + 1:self.toks.closing(node.start)])))
        if operator == '(' and self.toks[operands[0].end].text in ASSIGNMENTS:
            # an assignment's type is its left operand's, where no comma
            # follows the assignment in the parentheses
            at = operands[0].end
            while at < node.end - 1 and self.toks[at].text != ',':
                at = self.toks.closing(at) + 1
            if at == node.end - 1:
                return self.type(operands[0])
        raise LayoutError(f'{quoted(self.toks[node.start:node.end]) if node.end > node.start else node.what}, C whose '
                          'type the tool doesn\'t read')

    def usual(self, node, operands):
        """The type of node, an arithmetic operation or a conditional of
        operands. Where one is floating, it is the widest floating type among
        those the tool reads, even where it can't read the others. Else a
        conditional's is its first arm's, and an operation's is the pointer's
        of a pointer and an integer, and int for two integers or two
        pointers."""
        terms, problems = [], []
        for operand in operands:
            try:
                terms.append(self.type(operand))
            except LayoutError as error:
                problems.append(error)
        kinds = [category(self.types, term) for term in terms]
        types = [type for type in kinds if type in TYPES]
        if types:
            return FLOATING[max(types, key=list(TYPES).index)]
        if problems:
            raise problems[0]
        pointers = [term for term in terms if term[0] in ('pointer', 'array')]
        if node.kind == 'conditional' or len(pointers) == 1:
            return terms[0] if node.kind == 'conditional' or terms[0] in pointers else terms[1]
        return INT

    def pointee(self, term, node):
        """The type of what term, a pointer's or an array's type, points to, or
        a function's own, as node reads it."""
        if term[0] in ('pointer', 'array'):
            return self.types.resolve(term[1])
        if term[0] == 'function' and node.kind == 'unary':
            return term
        raise LayoutError(f'{quoted(self.toks[node.start:node.end])}, which reads through no pointer')

    def member(self, term, name):
        """The Types term of the member name of term, a struct's or a union's
        type, where a bit-field and an unnamed struct's or union's members
        take part. A LayoutError says why the tool reads none."""
        what = type_name(term) or f'an unnamed {term[0]}'
        bodies = self.types.definitions.get(term[:2]) if term[0] in ('struct', 'union') else None
        if not bodies:
            raise LayoutError(f'.{name} of {what}, whose members the tool doesn\'t read')
        if len({tuple(self.types.files[file][open_ + 1:close]) for file, open_, close in bodies}) > 1:
            raise LayoutError(f'{what}: defined two ways')
        (file, open_, close), *_ = sorted(bodies)
        texts, at = self.types.files[file], open_ + 1
        while at < close:
            base, at = self.types.specifiers(file, at)
            resolved = self.types.resolve(base)
            if texts[at] == ';' and resolved[0] in ('struct', 'union'):
                # an unnamed struct's or union's members are its own
                try:
                    found = self.member(resolved, name)
                    if (resolved[:2], name) in self.volatile_pointers:
                        self.volatile_pointers.add((term[:2], name))
                    return found
                except LayoutError:
                    at += 1
                    continue
            while True:
                first = at
                member, derivations, at = self.types.declarator(file, at)
                if member == name:
                    if volatile_pointer(texts[first:at]):
                        self.volatile_pointers.add((term[:2], name))
                    return derive(base, derivations)
                # past a bit-field's width, and the attributes after it
                while texts[at] not in (',', ';'):
                    at = self.types.closing(texts, at) + 1 if texts[at] in '([' else at + 1
                at += 1
                if texts[at - 1] == ';':
                    break
        raise LayoutError(f'{what}: no member {name}')


def volatile_pointer(texts):
    """Whether texts, the texts of a declarator's tokens, qualify a pointer
    volatile after its *, which Types doesn't keep."""
    for at, text in enumerate(texts):
        after = at + 1
        while text == '*' and after < len(texts) and texts[after] in POINTER_QUALIFIERS:
            if texts[after] in VOLATILE:
                return True
            after += 1
    return False

