"""C's expressions as tools/floats.py reads them: Reader reads the tokens
tools/float_tokens.py gives into Nodes, each indexing the tokens, by
tools/csource.py's Expressions, whose docstring gives the grammar it reads.

Reader takes a name for a typedef's only where tools/float_scope.py's
Scopes.typedef_at() says it names one where it stands, so that (F) * y
multiplies by an object F that hides a typedef F. A cast's and a type
name's node holds the type's Types term, or the LayoutError that says why
Types can't read it, as for a type name that holds a word of GNU_TYPES,
such as __typeof__ or _Complex, which GCC takes under -std=gnu17. Among an
opaque node's atoms, Reader takes float, double and each typedef's name as
type names, so that tools/float_fold.py fails on an opaque construct that
names a floating type.
"""

from functools import lru_cache

from csource import GNU_TYPES, Expressions
from layout import LayoutError


class Reader(Expressions):
    """Expressions of the Tokens toks from start to end. types, a Types,
    reads their type names, and typedef_at(name, at) says whether name names
    a typedef at toks[at]."""

    unread, untaken = 'C the fold can\'t read', 'an operator the fold doesn\'t take'

    def __init__(self, toks, types, typedef_at, start=0, end=None):
        super().__init__(toks, start, end)
        self.types, self.typedef_at = types, typedef_at

    def typename(self, text, at):
        return text in ('float', 'double') or self.typedef_at(text, at)

    def term(self, words):
        """The Types term of the type name whose texts are words, or the
        LayoutError that says why Types can't read it."""
        return term(self.types, tuple(words))


@lru_cache(maxsize=None)
def term(types, words):
    """The Types term of the type name whose texts are words, as types reads
    it, or the LayoutError that says why it can't. types reads no type name
    that defines a struct, union or enum, and its term names a typedef or a
    tag without resolving it. So one type name's term serves wherever it
    stands."""
    text, gnu = ' '.join(words), next((word for word in words if word in GNU_TYPES), None)
    if gnu:
        return LayoutError(f'{text}: {gnu}, a word Types doesn\'t read')
    try:
        return types.term(text)
    except LayoutError as error:
        return LayoutError(f'{text}: {error}')
