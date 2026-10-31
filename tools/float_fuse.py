"""Wrap each product the tool's rule of SHC's fusion leaves unfused, and
write as an addition each += of a product the rule fuses where GCC
wouldn't, for tools/floats.py on R26's and R32's routes. R10 fuses no
product, so fuse() takes nothing on its route.

The tool takes R26's and R32's fusion as this rule, which probes of each
case under R32 bear out. A floating run of + and - holds its operands in
C's order, with a constant sum or difference as one operand. SHC adds the
fold of the run's constants last, negated where C subtracts it, as in
a * b - 0.5F. It fuses a product the run adds that isn't constant, by
FMAC, where the run adds an operand before it that isn't constant; where
none comes before it and the run adds the next; or where every other
operand is constant. A binary -'s right operand is negative in the run,
and so are a unary -'s operand and a product with a negated factor, where
the - negates a floating value. An assignment bounds a run, so
c - -(a * b) adds the product, where x -= -(a * b) subtracts it negated.

GCC, under the -ffp-contract=on gcc.mk gives an object R26 or R32 builds,
fuses a product a run subtracts, or subtracts from, too, by FNEG and FMAC.
It also fuses a constant product whose value rounds, which SHC folds, as
-frounding-math leaves the product to run time. So the tool wraps each
floating product of a function's statements and automatic objects'
initializers that the rule doesn't fuse as

    ({ __typeof__(P) __floats_p = P; __floats_p; })

whose temporary ends GCC's contraction, as a statement does, where
({ P; }) doesn't. It reads the types of a run's operands and a product's
from the declarations in scope. It fails where it can't tell whether a
product it wraps is floating, whether an operand of the run is constant
or floating, or whether what a - negates is floating. SHC orders some
runs by what their operands are, as it adds the product to l1 in
-(l2 - f1 * f2) + l1 but subtracts c from it in -(c - a * b) + d, and the
tool reads each in C's order. A run goes through no comma, so
c - (d, a * b) keeps GCC's FNEG and FMAC.

The rule is the tool's. Where it leaves a product, GCC fuses it as its own
contraction reads the expression, and the tool can't tell whether SHC
fuses it alike. The rule follows none of these: which of two products
that open a run SHC fuses, and into which; a product SHC adds to the run
before it where C groups it with a sibling, as it adds a * b to x in
x + (a * b + y); a run SHC orders by its operands' declarations; and a
product SHC shares between statements, fuses across them, or fuses once
it inlines a function.

GCC evaluates a compound assignment's right operand first, and fuses no
product into the assignment, where the operand holds a call, an inline
function's included, a statement expression, the tool's wrap among them,
++, --, an assignment or a volatile object, as probes show. It still fuses
a statement expression's product into a local's member. So where R is
such a floating product, and SHC holds R and L in binary32, which FMAC
takes, the tool writes L += R as an addition, which GCC fuses:

    L = L + (R)

where L is a name, which names one object however R runs, and else

    ({ __typeof__(L) *__floats_l = &(L); *__floats_l = *__floats_l + (R); })

which evaluates L once, as the += does, and ahead of R, as SHC does. So
where R changes what L names, as a call that moves a pointer L reads does,
the addition reads and writes the object L named before R. The tool takes
an L that holds no side effect: a name, or parentheses, a cast, a unary &
or *, a member access or a subscript by a name or a constant, of one of
them. No object L reads may be volatile, a pointer a declarator qualifies
volatile after its * included, which Types doesn't keep. The tool fails on
any other L, and where it can't read L or the types of what L and R name.

SHC fuses no product into a -=, nor into a += of a constant product, which
it folds, of one its -s negate, or of an integer, which it converts, adds
to and converts back. So the tool wraps each such product. Where R is a
run of + and -, R32 fuses its products as it does a run's alone, then adds
or subtracts R, as GCC does, so the tool reads that run as any other.

In a product it wraps and a += it writes as an addition, the tool fails on
what it doesn't read outside the brackets Reader takes whole: a cast to a
type Types doesn't read, C Reader reads as no expression, and a keyword
Reader reads as a name. It also fails on a name, a constant, a string or a
{ right after one, where Reader stopped inside its last operand, as no
expression comes before any of them.
"""

from csource import ASSIGNMENTS, GNU, KEYWORDS, PRIMARY, SIZES
from float_expr import Reader
from float_scope import BUILTINS, category, shown
from float_tokens import quoted
from float_value import FLOAT
from layout import LayoutError

# the opaque nodes Reader reads, by what each is up to a comma: a postfix
# operator's, which takes what comes before it, a call, a subscript, a
# member access, ++ and --; parentheses that hold no one expression; a
# compound literal; and SIZES' operators. Any other is C Reader reads as no
# expression
POSTFIX = {'a call', 'a subscript', '.', '->', '++', '--'}
OPAQUE = {*POSTFIX, '(', 'a compound literal', *SIZES}

# the tokens after which an expression opens: punctuators, the assignments,
# and the keywords before a statement's
OPENS = {';', '{', '}', '(', '[', ',', '?', ':', *ASSIGNMENTS, 'return', 'case', 'else', 'do'}
# the words that open no expression: C's and GNU's keywords but sizeof
WORDS = (KEYWORDS - {'sizeof'}) | GNU | {'_Bool'}


def fuse(toks, scopes, route, plan):
    """Take into plan each product the tool wraps and each += it writes as an
    addition under route on the tree's lines of toks, GCC's preprocessed C as
    marked() gives it, whose declarations scopes, its Scopes, reads; and give
    the problems, each naming its file and line."""
    if route.release not in ('r26', 'r32'):
        return []
    chains = Chains(toks, scopes, route)
    chains.read(0, len(toks))
    for left, right in chains.compounds.values():
        chains.compound(left, right)
    chains.write(plan)
    return chains.problems


class Chains:
    """What the tool rewrites on the tree's lines of toks under route, as
    fuse() takes them: in wraps, each product it wraps; in written, each +=
    it writes as an addition; and in problems, each it can't rewrite, naming
    its file and line."""

    def __init__(self, toks, scopes, route):
        self.toks, self.scopes, self.types, self.route, self.problems = toks, scopes, scopes.types, route, []
        # the span of each node unsure() took a problem for
        self.unsure_of = set()
        # each binary + or - and unary - that a run of + and - takes, as the
        # index of its first token and the index past its last
        self.summed = set()
        # the index past each product to wrap, by the index of its first token
        self.wraps = {}
        # where each initializer of an object of static storage the fold reads
        # starts, and the index past it, which read() skips: each is constant
        self.folded = {start: end for start, end, _, _ in scopes.statics}
        # each += written as an addition, by the index of its first token and
        # the index past its last: its pieces, as Plan.edit() takes them
        self.written = {}
        # the left operand of each += and -=, by its operator's index; and
        # each += assigned() leaves for compound(), as its left operand, or
        # None where the tool reads none, and its right one, by the right
        # one's first token's index
        self.lefts, self.compounds = {}, {}
        # the index of each ) that closes the head of an if, a for, a while
        # or a switch, after which a statement opens
        self.heads = {toks.closing(at + 1) for at, token in enumerate(toks[:-1])
                      if token.text in ('if', 'for', 'while', 'switch') and toks[at + 1].text == '('}

    def constant(self, node):
        """Whether scopes shows node is a constant expression SHC folds; where
        it can't tell, False, and unsure() takes the problem."""
        try:
            return self.scopes.constant(node)
        except LayoutError as error:
            self.unsure(node, f'C the tool can\'t tell is constant: {error}')
            return False

    def stripped(self, node):
        """node without the parentheses, the unary +s and the casts to a
        floating type around it."""
        while node.kind == 'parentheses' or node.kind == 'unary' and node.what == '+' or \
                node.kind == 'cast' and shown(self.types, node):
            node = node.operands[0]
        return node

    def wrap(self, node):
        """Wrap node, through parentheses, a unary + and casts to a floating
        type, where it is a floating product on the tree's lines of a
        function's code; or, where the tool can't tell whether it is
        floating, take the problem that says why."""
        node = self.stripped(node)
        if node.kind != 'binary' or node.what != '*' or \
                self.toks.system[node.operands[0].end] or not self.scopes.code(node.start):
            return
        product = quoted(self.toks[node.start:node.end])
        problem = f'{self.toks.where(node.start)}: {product}: a product SHC doesn\'t fuse'
        try:
            if not self.scopes.floating(node):
                return
        except LayoutError as error:
            self.problems.append(f'{problem}, whose type the tool can\'t tell: {error}')
            return
        unread = self.unread(node)
        if unread:
            self.problems.append(f'{problem}, beside {unread}')
            return
        self.wraps.setdefault(node.start, set()).add(node.end)

    def nonfloating(self, node):
        """Whether scopes shows that node's value isn't floating; where it
        can't tell, False, and unsure() takes the problem."""
        try:
            return not self.scopes.floating(node)
        except LayoutError as error:
            self.unsure(node, f'C the tool can\'t tell is floating: {error}')
            return False

    def unsure(self, node, why):
        """Take the problem why says, once for node, where node lies on the
        tree's lines of a function's code."""
        if not self.toks.system[node.start] and self.scopes.code(node.start) and \
                (node.start, node.end) not in self.unsure_of:
            self.unsure_of.add((node.start, node.end))
            self.problems.append(f'{self.toks.where(node.start)}: {quoted(self.toks[node.start:node.end])}: {why}')

    def signed(self, node, negative=False, whole=True):
        """The operands of the run of + and - that node opens, in C's order,
        each with whether it is negative in the run. The run goes through
        parentheses, a unary + and casts to a floating type, and through each
        unary -, binary + and binary - of a floating value, which it takes
        into summed. A constant sum or difference, which SHC folds whole, is
        one operand where whole is True. A product is negative where an odd
        number of -s negate its parts."""
        while node.kind == 'parentheses' or node.kind == 'unary' and node.what == '+' or \
                node.kind == 'cast' and shown(self.types, node) or \
                node.kind == 'unary' and node.what == '-' and not self.nonfloating(node.operands[0]):
            if node.kind == 'unary' and node.what == '-':
                negative = not negative
                self.summed.add((node.start, node.end))
            node = node.operands[0]
        if node.kind == 'binary' and node.what in ('+', '-') and not (whole and self.constant(node)) and \
                not self.nonfloating(node):
            self.summed.add((node.start, node.end))
            left, right = node.operands
            return self.signed(left, negative, whole) + self.signed(right, negative != (node.what == '-'), whole)
        return [(node, negative != (node.kind == 'binary' and node.what == '*' and self.negated(node)))]

    def negated(self, node):
        """Whether an odd number of unary -s of floating operands negate the
        parts of node's run of * and /, through parentheses, a unary + and
        casts to a floating type."""
        node = self.stripped(node)
        if node.kind == 'unary' and node.what == '-' and not self.nonfloating(node.operands[0]):
            return not self.negated(node.operands[0])
        if node.kind == 'binary' and node.what in ('*', '/'):
            return self.negated(node.operands[0]) != self.negated(node.operands[1])
        return False

    def take(self, root):
        """Wrap each product of the run of + and - that root opens that the
        rule doesn't fuse. Where root is constant, which SHC folds whole, that
        is each product of its run, which GCC fuses at run time where its
        value rounds."""
        terms = self.signed(root, whole=not self.constant(root))
        nonconstant = [(leaf, negative) for leaf, negative in terms if not self.constant(leaf)]
        signs = [negative for _, negative in nonconstant]
        for index, (leaf, negative) in enumerate(nonconstant):
            before, after = signs[:index], signs[index + 1:index + 2]
            if negative or (all(before) if before else any(after)):
                self.wrap(leaf)
        for leaf, _ in terms:
            if self.constant(leaf):
                self.wrap(leaf)

    def read(self, start, end):
        """Wrap each product of each expression of toks[start:end] that SHC
        doesn't fuse, and of each one inside its parentheses and braces that
        Reader reads no further, and take each += of one for compound()."""
        reader, at = Reader(self.toks, self.types, self.scopes.typedef_at, start, end), start
        while at < end:
            token = self.toks[at]
            if at in self.folded:
                at = self.folded[at]
                continue
            # a brace opens no expression but the next, as a statement's or an
            # initializer's, and an if's, a for's, a while's or a switch's
            # head's ) opens its statement
            if token.text in WORDS or token.text in ('{', '}') or at > start and \
                    self.toks[at - 1].text not in OPENS and at - 1 not in self.heads:
                at += 1
                continue
            reader.at = at
            node = reader.conditional()
            # Reader reads each operand of a += or -= as an expression of its
            # own, or as an opaque node's operand, as in a call's argument
            if reader.at < end and self.toks[reader.at].text in ('+=', '-='):
                self.lefts[reader.at] = node
            if at:
                self.assigned(node)
            pending = [node]
            while pending:
                inner = pending.pop()
                if inner.kind == 'opaque' and inner.what == '(':
                    self.read(inner.start + 1, inner.end - 1)
                elif inner.kind == 'opaque' and inner.what == 'a compound literal':
                    self.read(self.toks.closing(inner.start) + 2, inner.end - 1)
                if inner.kind == 'opaque':
                    for operand in inner.operands:
                        if operand.end < end and self.toks[operand.end].text in ('+=', '-='):
                            self.lefts[operand.end] = operand
                    for operand in inner.operands:
                        self.assigned(operand)
                elif (inner.kind == 'binary' and inner.what in ('+', '-') or
                      inner.kind == 'unary' and inner.what == '-') and \
                        (inner.start, inner.end) not in self.summed and \
                        any(token.text == '*' for token in self.toks[inner.start:inner.end]):
                    self.take(inner)
                pending += inner.operands
            at = max(reader.at, at + 1)

    def assigned(self, node):
        """Where node is the right operand of a += or -=, wrap it where it is a
        product SHC doesn't fuse into the assignment: a -='s, which subtracts
        it, or a constant one, which SHC folds, or one an odd number of -s
        negate. Take any other right operand of a += for compound(), which
        keeps only a product."""
        operator = self.toks[node.start - 1].text
        if operator in ('-=', '+='):
            if operator == '-=' or self.constant(node) or self.negated(node):
                self.wrap(node)
            else:
                # the whole operand comes ahead of an opaque node's first
                # operand, which starts where it does
                self.compounds.setdefault(node.start, (self.lefts.get(node.start - 1), node))

    def compound(self, left, right):
        """Write the += of left and right as an addition where right is a
        floating product of a function's code on the tree's lines that holds
        what effect() names, and SHC holds left and right in binary32. Wrap
        right instead where left isn't floating, as SHC converts left and
        fuses nothing into it. Where the tool can't read left or the types it
        needs, or doesn't take left, take the problem that says why."""
        product = self.stripped(right)
        if product.kind != 'binary' or product.what != '*' or \
                self.toks.system[product.operands[0].end] or not self.scopes.code(product.start):
            return
        problem = f'{self.toks.where((left or right).start)}: {quoted(self.toks[(left or right).start:right.end])}: '
        try:
            if not self.scopes.floating(product):
                return
            if left is None:
                self.problems.append(problem + 'a product SHC can fuse, beside a left operand the tool doesn\'t read')
                return
            if not self.scopes.floating(left):
                self.wrap(right)
                return
            effect = self.effect(right)
            formats = {self.route.formats[category(self.types, self.scopes.type(node))]
                       for node in (left, product)}
            if effect is None or formats != {FLOAT}:
                return
            why = self.untaken(left)
        except LayoutError as error:
            self.problems.append(problem + f'a product SHC can fuse, whose types the tool can\'t read: {error}')
            return
        if why:
            self.problems.append(problem + f'a product SHC fuses that holds {effect}, which GCC fuses only as an '
                                           f'addition, beside a left operand the tool doesn\'t take: {why}')
            return
        unread = self.unread(left) or self.unread(right)
        if unread:
            self.problems.append(problem + f'a product SHC fuses, beside {unread}')
            return
        # a name names one object however right runs; another left operand
        # GCC evaluates once, through a pointer of a name C reserves for the
        # implementation, so that right can't move what the addition writes
        held, added = (left.start, left.end), (right.start, right.end)
        self.written[left.start, right.end] = [held, ' = ', held, ' + (', added, ')'] if left.kind == 'name' else \
            ['({ __typeof__(', held, ') *__floats_l = &(', held, '); *__floats_l = *__floats_l + (', added, '); })']

    def effect(self, node):
        """What node holds that GCC evaluates ahead of the += it is the right
        operand of, as it does each side effect: a call, an inline function's
        too, a statement expression, a product the tool wraps, ++, --, an
        assignment or a volatile object; or None. A LayoutError says why the
        tool can't tell whether an object is volatile."""
        for at in range(node.start, node.end):
            text = self.toks[at].text
            if text in ('++', '--'):
                return text
            if text in ASSIGNMENTS:
                return 'an assignment'
            if text == '(' and self.toks[at + 1].text == '{':
                return 'a statement expression'
            if any(past <= node.end for past in self.wraps.get(at, ())):
                return 'a product the tool wraps'
        pending = [node]
        while pending:
            inner = pending.pop()
            if inner.kind == 'opaque' and inner.what == 'a call':
                return 'a call'
            pending += inner.operands
        held = self.volatile(node)
        return held and f'{quoted(self.toks[held.start:held.end])}, a volatile object'

    def untaken(self, node):
        """Why the tool doesn't take node, a +='s left operand, or None where
        it does. It takes a form shape() takes where no object node reads is
        volatile. A LayoutError says why the tool can't tell."""
        why = self.shape(node)
        if why:
            return why
        held = self.volatile(node)
        return held and f'{quoted(self.toks[held.start:held.end])}, a volatile object'

    def shape(self, node):
        """Why node isn't a left operand the tool takes, or None where it is:
        a name, or parentheses, a cast, a unary & or *, a member access or a
        subscript by a name or a constant, of one of them."""
        what = node.what.partition(',')[0] if node.kind == 'opaque' else node.what
        text = quoted(self.toks[node.start:node.end])
        if node.kind in ('parentheses', 'cast') or node.kind == 'unary' and what in ('&', '*') or \
                node.kind == 'opaque' and what in ('.', '->', 'a subscript'):
            index = node.operands[1:]
            if what == 'a subscript' and not (len(index) == 1 and (index[0].kind == 'name' or
                                                                   self.constant(index[0]))):
                return f'{text}, a subscript by an expression'
            return self.shape(node.operands[0])
        if node.kind != 'name' or self.constant(node):
            kinds = {'a call': 'a call', '++': 'an increment', '--': 'a decrement'}
            return f'{text}, {kinds.get(what, "none of the left operands the tool takes")}'
        return None

    def volatile(self, node):
        """An object node reads that is volatile, through its typedefs, or
        None: a name, a member, or what a pointer points to. A pointer a
        declarator qualifies volatile after its *, which Types doesn't keep,
        counts too. A LayoutError says why the tool can't tell."""
        scopes, pending = self.scopes, [node]
        while pending:
            inner = pending.pop()
            pending += inner.operands
            what = inner.what.partition(',')[0] if inner.kind == 'opaque' else None
            terms, qualified = [], False
            if inner.kind == 'name' and inner.what not in BUILTINS:
                found = scopes.lookup(inner.what, inner.start)
                if found.kind == 'object':
                    terms.append(found.term)
                    qualified = (inner.what, found.start) in scopes.volatile_pointers
            elif what in ('.', '->', 'a subscript') or inner.kind == 'unary' and inner.what == '*':
                outer = scopes.type(inner.operands[0])
                if what != '.':
                    terms.append(outer[1] if outer[0] in ('pointer', 'array') else outer)
                if what in ('.', '->'):
                    whole = self.types.resolve(terms[-1]) if what == '->' else outer
                    name = self.toks[inner.end - 1].text
                    terms.append(scopes.member(whole, name))
                    qualified = (whole[:2], name) in scopes.volatile_pointers
            if qualified or any(volatile(self.types, term, scopes.volatile_pointers) for term in terms):
                return inner
        return None

    def unread(self, node):
        """What the tool doesn't read in node, a product to wrap or an operand
        of a += to write as an addition, outside the brackets of what Reader
        takes as opaque, which bound it: a cast to a type the tool doesn't
        read; C Reader reads as no expression, or a keyword it reads as a
        name; or the token after node, where it opens an operand, as no
        expression comes before; or None."""
        after = self.toks[node.end] if node.end < len(self.toks) else None
        if after is not None and (after.kind in PRIMARY or after.text == '{'):
            return f'{after.text} after it, which follows no expression'
        pending = [node]
        while pending:
            inner = pending.pop()
            text = quoted(self.toks[inner.start:inner.end]) if inner.end > inner.start else 'a missing operand'
            if inner.kind == 'cast':
                try:
                    category(self.types, inner.term)
                except LayoutError as error:
                    return f'{text}, a cast to a type the tool doesn\'t read: {error}'
            elif inner.kind == 'opaque' and inner.what.partition(',')[0] not in OPAQUE or \
                    inner.kind == 'name' and inner.what in WORDS:
                return f'{text}, C Reader doesn\'t read'
            if inner.kind != 'opaque':
                pending += inner.operands[::-1]
            elif inner.what.partition(',')[0] in POSTFIX:
                # what a postfix operator takes, outside its brackets
                pending.append(inner.operands[0])
        return None

    def write(self, plan):
        """Take each product the tool wraps and each += it writes as an
        addition into plan. A wrap is a statement expression that holds the
        product P in a temporary, of a name C reserves for the
        implementation, which ends GCC's contraction as a statement does:
        ({ __typeof__(P) __floats_p = P; __floats_p; })."""
        for start, ends in self.wraps.items():
            for end in ends:
                plan.edit(start, end, ['({ __typeof__(', (start, end), ') __floats_p = ', (start, end),
                                       '; __floats_p; })'])
        for (start, end), pieces in self.written.items():
            plan.edit(start, end, pieces)


def volatile(types, term, typedefs):
    """Whether term, a Types term, is volatile, through the typedefs it
    names. A typedef in typedefs is volatile, as a declarator qualifies it
    volatile after its *, which Types doesn't keep. A LayoutError says why
    Types can't resolve term."""
    types.resolve(term)
    while term[0] in ('qualified', 'typedef'):
        if term[0] == 'qualified' and 'volatile' in term[2] or term[0] == 'typedef' and term[1] in typedefs:
            return True
        term = term[1] if term[0] == 'qualified' else next(iter(types.typedefs[term[1]]))
    return False
