"""Check the tags in C and assembly sources: @asm, @hack and @bug.

Each inline body, #pragma asm block of assembly and .src function takes
one @asm tag, directly above it. A body or block opens at a directive: a
comment or a literal that mentions one opens neither. A run fails, naming
each problem, on:

- a tag of the wrong form or kind;
- one of those without its @asm tag or with two;
- a #pragma inline_asm spelled other than (NAME) or (NAME(size=N)), or
  with no braces below it;
- an @asm tag in C not directly above a body or block, or in a .src file
  not ending the comment above a label that starts a function;
- an @asm tag whose text doesn't open as its kind requires, or cites no
  address where its kind needs one;
- an address an @asm tag cites outside its allowed ranges: its
  function's, those of the functions of its source its text names, and for
  an inline body those of its callers, direct or through macros; or for a
  block, its source's rows;
- an if, or a while loop whose body opens with {, whose condition is a
  literal 0 or FALSE in any parentheses, or opens with one ahead of &&,
  without an @hack redundant tag above it.

--review lists each open tag as FILE:LINE, what it tags and its text, then
counts the bodies, blocks and functions, and those with a tag by its kind.
It fails on the same problems. TAGS.md#tags, TAGS.md#assembly,
TAGS.md#hacks-and-bugs and TAGS.md#what-lint-checks give the rules.

tools/inline.py makes each inline body GCC compiles a function of its own,
which returns where the body runs off its end. The function is call-safe
where it does what SHC's expansion of the body does.

--abi fails on the same problems, and on:

- an inline body or .src function whose @asm tag is abi, or an inline body
  that may not be call-safe, that no --contract names;
- a --contract that names no such function;
- a #pragma asm block GCC compiles whose @asm tag is abi, since a block
  defines no function for a --contract to name;
- an inline body --abi can't read, whatever --contract names.

A --contract names a function as C spells it. It names a .src function by
the label below its tag, where the source exports that label or the tables
name it, and otherwise by the first such label of the tag's run. A tag's
run is the labels, with no statement between them, that start the code
below it. A --contract without --abi fails. make gcc runs --abi on each
source it assembles or splits, with a --contract for each name in its
object's CONTRACTS, in config/gcc.mk. Each names a function whose GCC
callers meet its contract, or one that no GCC code calls.

--abi reads each body whose tag isn't abi, under GCC's calling convention,
and finds that its function may not be call-safe where the body:

- reads a register that no argument passes, or one that a call it makes
  can change;
- takes R15 above its entry, into its caller's frame;
- calls or returns with FPSCR's FR or SZ bit set, which GCC's code takes
  clear;
- returns with a register changed that GCC's callers keep across a call,
  R15 among them;
- leaves by a return, a jump or a branch out of the body, where a C
  function of its file goes on past a call of the body's function. In
  SHC's expansion, that way out leaves the caller.

A C function goes on past a call of the body's function unless it calls
that function by name, and ends at each such call, by returning its value
or with the call as its last statement.

--abi follows R15 through pushes, ADDs of an immediate, and pops into a
register other than R15. A pop restores a register only from the slot that
a push of its value at entry filled, with no store at @R15, @(disp,R15) or
@(R0,R15) since. --abi reads SP as R15, as asmsh does, and takes a BRAF as
going to any label of the body's code, or out of the body.

A body --abi passes may still not be call-safe. --abi takes a call as
returning, and as reading only its target, so it doesn't check the
registers the callee reads. It takes a load as reading only its address's
registers, so it misses a read of the caller's frame at an offset from
R15. It takes a push as saving the registers it stores, not reading them,
so it misses a register a call changed that the body pushes, pops back and
then reads. It misses a store to a pushed slot through a copy of R15.

--abi can't read a body with:

- a parameter of a type it can't place;
- an instruction it doesn't read, such as a BSRF, a BSR to a label of the
  body or a write to FPSCR;
- a delayed branch whose slot it can't place: one with no instruction
  below it, one whose slot a label marks or a directive stands above, and
  one whose slot branches, jumps, calls or returns;
- an instruction that no path reaches;
- two paths that meet with FR, SZ or R15 apart.

An inline body on the shipped side of a GCC guard, under #ifndef GCC or
under the #else or an #elif of #ifdef GCC, makes no function: GCC compiles
the guard's other side in its place. --abi reads no such body, and a
--contract can't name it. Nor does --abi read a C function there for its
calls of a body's function.

--inline checks no tag. It names each C source that holds a #pragma
inline_asm or #pragma asm, a directory standing for each .c file under it,
for the Makefile's INLINE_C and UNSETTLED. A pragma on the shipped side of
a GCC guard counts, since SHC compiles that side.
"""

import argparse
import os
import re
import sys
from bisect import bisect_left, bisect_right
from collections import namedtuple
from itertools import chain

from config import Config, configs
from csource import blanked, bodies, inline_sites, macro_directives, pairs, parse, pragma, regions, shipped_side
from images import CONFIG, images
from tables import hexa, named, table
from tool import content, fail

KINDS = ('opcode', 'operand', 'abi', 'epilogue', 'name', 'open')
# each kind's rules, one of which an opcode, operand or abi tag's text opens with: shckit inspect's, and for abi also
# three contracts with the places a function is entered from, which inspect, reading one function, can't see
RULES = {'opcode': ('rotcl', 'rotcr', 'swap', 'dmuls', 'dmulu', 'movca', 'fschg', 'cache-line', 'sett'),
         'operand': ('fipr-registers', 'ftrv-registers', 'fsca-registers', 'small-constant', 'zero-displacement',
                     'fschg-pairs'),
         'abi': ('stack-written', 'fpscr-save', 'stack-above-entry', 'return-below-entry', 'register-contract',
                 'exception-entry', 'exception-continuation')}
# the evidence an epilogue or name tag's text opens with: for epilogue, SHC's dead RTS after an inline body's return,
# or its alignment and RTS after a body that ends in neither a return nor a tail jump; for name, the member the
# shipped library names as assembly
EVIDENCE = {'epilogue': ('dead-return', 'fill'),
            'name': ('vmsasm_', 'lcdasm_', 'mpasm_', 'kmasm_workaround_for_sh_lnk_', 'kmasm_kamui_lib_')}
# a hack's kinds in the order that decides a hack fitting more than one, and a bug's
HACKS = ('implicit', 'type', 'redundant', 'spelling')
BUGS = ('memory', 'uninitialized', 'logic', 'arithmetic', 'mismatched-call')
# each tag's kinds, by the tag's name
TAGS = {'asm': KINDS, 'hack': HACKS, 'bug': BUGS}
# a tag's form: its name, its kind and a colon, then its text
GRAMMAR = re.compile(r'@(asm|hack|bug) ([a-z-]+): (\S.*)')
# what each @asm kind but open opens its text with, before a comma: a rule, or its evidence
OPENED = {kind: re.compile(rf'(?:{"|".join(words)}), \S') for kind, words in dict(RULES, **EVIDENCE).items()}
# the tag a comment's text opens, after a /** */ comment's stars
OPENING = re.compile(r'(\*+\s*)?@(asm|hack|bug)\b')
ADDRESS = re.compile(r'\b0x[0-9a-f]{8}\b', re.I)
WORD = re.compile(r'\w+')
# a 0 literal, decimal, octal or hex, with any suffix; or the SDK's FALSE
ZERO = re.compile(r'(?:0+|0[xX]0+)[uUlL]*|FALSE')
# the kinds of token a function's body, as definitions() gives it, leaves out
UNREAD = ('comment', 'string', 'character')
# in assembly: a label, but for a name .EQU or .ASSIGN gives a value; a label local to its function; the name an .EQU
# or .ASSIGN gives a value; the names an .EXPORT exports; a comment on its own line, indented or not; and one that
# starts a tag
LABEL = re.compile(r'(\w+):(?!\s*\.(?:EQU|ASSIGN)\b)', re.I)
LOCAL = re.compile(r'\?\w*:')
VALUE = re.compile(r'\w+:')
EXPORT = re.compile(r'\s+\.EXPORT\s+([^;]*)', re.I)
NOTE = re.compile(r'\s*;(.*)')
TAG = re.compile(r'\s*;\s*@(asm|hack|bug)\b')

# GCC's calling convention under GCCFLAGS' -ml -m4-single-only, by which --abi reads a body's function. A function
# takes its integer arguments in R4 to R7 and its float ones in FR4 to FR11, and returns in R0 or FR0. A call can
# change R0 to R7, FR0 to FR11, FPUL, T, MACH, MACL and PR. It keeps R8 to R14 and FR12 to FR15. --abi names a
# register of the FPU by its bank: FAn is the register FRn names at the body's entry, and FBn the one XFn names there.
# GCC allocates no FB register, so its callers keep none across a call
INTEGER_ARGUMENTS = ('R4', 'R5', 'R6', 'R7')
FLOAT_ARGUMENTS = tuple(f'FA{n}' for n in range(4, 12))
CLOBBERED = {*(f'R{n}' for n in range(8)), *(f'FA{n}' for n in range(12)), 'FPUL', 'T', 'MACH', 'MACL', 'PR'}
KEPT = {*(f'R{n}' for n in range(8, 15)), *(f'FA{n}' for n in range(12, 16))}
# the words that spell a parameter type --abi places in a register, in C or as the SDK's typedefs: an integer of 32
# bits or fewer, and a float. -m4-single-only makes a double a float
INTEGER_TYPES = {'char', 'short', 'int', 'long', 'signed', 'unsigned', 'Sint8', 'Uint8', 'Sint16', 'Uint16', 'Sint32',
                 'Uint32', 'Int', 'Bool', 'Angle'}
FLOAT_TYPES = {'float', 'double', 'Float'}
# the registers a body reads as the machine's, never as an argument: SR's and FPSCR's modes, GBR, the exception
# registers and the FB bank, which holds XMTRX. --abi trusts that XMTRX holds the current matrix at the entry of each
# body GCC compiles, as at SHC's expansion. Of GCC's own code, only sdk/include/ninja_zam.h's zam_ helpers and
# sdk/ninja/njModelS.c's environment strip helpers write XMTRX. Each leaves the matrix the shipped code leaves, and no
# body GCC compiles runs between a strip's START and its END. Any other SH4ZAM call that writes XMTRX, as
# shz_mat4x4_mult does, would break this where it leaves another matrix than the shipped code
MACHINE = {'SR', 'FPSCR', 'GBR', 'VBR', 'SSR', 'SPC', 'SGR', 'DBR', *(f'FB{n}' for n in range(16))}
# each instruction --abi reads, by its operands' roles, a letter each: r read, w written, b both, and l a label
ROLES = {roles: mnemonics.split() for roles, mnemonics in {
    'rw': 'MOV MOV.B MOV.W MOV.L MOVA MOVCA.L EXTU.B EXTU.W EXTS.B EXTS.W NEG NOT SWAP.B SWAP.W FMOV FMOV.S FLDS FSTS '
          'FTRC FLOAT FSCA LDS LDS.L LDC LDC.L STS STS.L STC STC.L',
    'rb': 'ADD ADDC ADDV SUB SUBC SUBV AND OR XOR SHAD SHLD FADD FSUB FMUL FDIV FIPR FTRV',
    'rr': 'CMP/EQ CMP/GE CMP/GT CMP/HI CMP/HS TST FCMP/EQ FCMP/GT',
    'rrb': 'FMAC',
    'b': 'DT SHLL SHLR SHAL SHAR SHLL2 SHLL8 SHLL16 SHLR2 SHLR8 SHLR16 ROTL ROTR ROTCL ROTCR FABS FNEG FSQRT FSRRA',
    'w': 'FLDI0 FLDI1 MOVT',
    'r': 'CMP/PL CMP/PZ JMP JSR BRAF PREF OCBI OCBP OCBWB',
    'l': 'BT BF BT/S BF/S BRA BSR',
    '': 'NOP RTS RTE CLRT SETT FSCHG FRCHG'}.items()}
ROLE = {mnemonic: roles for roles, mnemonics in ROLES.items() for mnemonic in mnemonics}
# the instructions that read T, and those that write it
READS_T = {'ADDC', 'SUBC', 'ROTCL', 'ROTCR', 'MOVT', 'BT', 'BF', 'BT/S', 'BF/S'}
WRITES_T = {*ROLES['rr'], 'ADDC', 'SUBC', 'ADDV', 'SUBV', 'DT', 'SHLL', 'SHLR', 'SHAL', 'SHAR', 'ROTL', 'ROTR',
            'ROTCL', 'ROTCR', 'CMP/PL', 'CMP/PZ', 'CLRT', 'SETT'}
# the instructions with a delay slot, the calls among them, and those --abi counts as leaving the body whatever their
# operands. A branch to a label leaves the body where the label marks neither an instruction of the body nor its end
DELAYED = {'BT/S', 'BF/S', 'BRA', 'BSR', 'JMP', 'JSR', 'BRAF', 'BSRF', 'RTS', 'RTE'}
CALLS = {'BSR', 'JSR', 'BSRF'}
LEAVES = {'JMP', 'BRAF', 'RTS', 'RTE'}
# an immediate, in decimal as #-4 or in hex as #H'10
IMMEDIATE = re.compile(r"#(?:(-?\d+)|H'([0-9A-F]+))$", re.I)
# an operand that names registers: a numbered one, which NUMBERED bounds, XMTRX or a special register
REGISTER = re.compile(r'(?:(R|FR|DR|XD|FV)(0|[1-9]\d*)|XMTRX|PR|MACH|MACL|FPUL|SR|FPSCR|GBR|VBR|SSR|SPC|SGR|DBR)$',
                      re.I)
# the numbers each prefix takes: R0 to R15, FR0 to FR15, the pairs DR0 to DR14 and XD0 to XD14 by twos, and the
# vectors FV0, FV4, FV8 and FV12. asmsh reads a name with any other number, or with a leading zero, as a symbol
NUMBERED = {'R': range(16), 'FR': range(16), 'DR': range(0, 16, 2), 'XD': range(0, 16, 2), 'FV': range(0, 16, 4)}
# an address: @Rn, @-Rn, @Rn+, or @(X,B) with Rn, GBR or PC as its base B. It reads Rn or GBR, and R0 too where X is R0
AT = re.compile(r'@(?:(-)?(R\d+)(\+)?|\((?:(R0)|[^,]+),(R\d+|GBR|PC)\))$', re.I)
# SP as a word of an operand, which asmsh reads as R15: no byte a symbol can hold stands on either side
SP = re.compile(r'(?<![\w.?$])SP(?![\w.?$])', re.I)


class Tables:
    """Where each source's functions lie: its rows' ranges, and the names in them."""

    def __init__(self, sources=None):
        """Read every image's tables, or where sources is given, only the rows that place those sources, from the images
        whose splits.txt places one of them. make gcc runs --abi on one source at a time, and reading every image's
        tables would take most of each run."""
        wanted = None if sources is None else set(sources)
        self.places = {}
        self.rows = {}
        # each source's functions: the names in its rows of code, whose sections' names start with P
        self.functions = {}
        read = configs() if wanted is None else [
            Config(name) for name in images() if any(row[0] in wanted for row in table(CONFIG / name / 'splits.txt'))]
        for config in read:
            names = sorted(named(config.path / 'symbols.txt'))
            for source, section, start, end in config.placements:
                if wanted is not None and source not in wanted:
                    continue
                inside = [(address, name) for address, name in names if start <= address < end]
                self.places.setdefault(source, []).append((inside, end))
                self.rows.setdefault(source, []).append((start, end))
                if section.startswith('P'):
                    self.functions.setdefault(source, set()).update(name for _, name in inside)

    def ranges(self, source, name):
        """Each [start, end) the tables give the function name in source, up to the next name or the row's end."""
        found = []
        for inside, end in self.places.get(source, []):
            for i, (address, symbol) in enumerate(inside):
                if symbol == name:
                    after = [later for later, _ in inside[i + 1:] if later > address]
                    found.append((address, after[0] if after else end))
        return found

    def mentioned(self, source, text):
        """Each [start, end) the tables give a function in source that text names."""
        names = self.functions.get(source, set())
        return [(start, end) for name in sorted(names & set(WORD.findall(text))) for start, end in self.ranges(source, name)]


def tag(where, text, problems):
    """The (tag, kind, text) of the tag a comment's text opens, or None. A malformed one and one in a /** */ comment
    are problems."""
    opening = OPENING.match(text)
    if opening is None:
        return None
    name = opening[2]
    if opening[1]:
        problems.append(f'{where}: a tag is a plain /* */ comment, not /** */')
        return None
    form = GRAMMAR.fullmatch(text)
    if not form or form[2] not in TAGS[name]:
        problems.append(f'{where}: not @{name} <kind>: <text>, with a kind of {", ".join(TAGS[name])}')
        return None
    if name == 'asm' and form[2] in RULES and not OPENED[form[2]].match(form[3]):
        problems.append(f'{where}: not @asm {form[2]}: <rule>, <text>, with a rule of {", ".join(RULES[form[2]])}')
        return None
    if name == 'asm' and form[2] in EVIDENCE and not OPENED[form[2]].match(form[3]):
        problems.append(f'{where}: not @asm {form[2]}: <evidence>, <text>, with evidence of '
                        f'{", ".join(EVIDENCE[form[2]])}')
        return None
    return form.groups()


def cited(where, kind, text, ranges, problems, scope='its function'):
    """Check the addresses a tag's text cites: at least one, but for an open tag, and each inside one of ranges, which
    are scope's."""
    addresses = [int(found, 16) for found in ADDRESS.findall(text)]
    if not addresses and kind != 'open':
        problems.append(f'{where}: @asm {kind} cites no address')
    for address in addresses:
        if not any(start <= address < end for start, end in ranges):
            problems.append(f'{where}: {hexa(address)} is outside {scope}')


def comments(text):
    """Each C comment: the line it ends on, the line it starts on, and its text
    as one line, without /* and */ or each later line's leading *, each run of
    whitespace one space and none at its ends. A line splice in it ends a
    line."""
    for token in parse(text).lexemes:
        if token.kind == 'comment' and token.text.startswith('/*'):
            spelled = text[token.start:token.end].replace('\\\n', '\n')
            body = re.sub(r'\n\s*\*(?!/)', ' ', spelled[2:-2])
            yield token.line + spelled.count('\n'), token.line, ' '.join(body.split())


def zero(tokens):
    """The 0 literal or FALSE a condition, as its tokens, comes to, alone or ahead of &&, and whether a && follows it;
    or None."""
    depth, first = 0, len(tokens)
    for i, token in enumerate(tokens):
        depth += (token in '([') - (token in ')]')
        if depth == 0 and token in ('||', '?', ','):
            return None
        if depth == 0 and token == '&&':
            first = min(first, i)
    head, more = tokens[:first], first < len(tokens)
    if len(head) == 1 and ZERO.fullmatch(head[0]):
        return head[0], more
    if len(head) > 2 and head[0] == '(' and pairs(head)[0] == len(head) - 1:
        inner = zero(head[1:-1])
        return inner and (inner[0], inner[1] or more)
    return None


def never_run(text):
    """Each if, and each while whose body opens with {, in C text, whose condition zero() reads as a 0 literal or
    FALSE, as its line and how it reads. An #if counts as neither."""
    c = parse(text)
    opened = {each.start for each in c.directives}
    for line, spelled in enumerate(c.blank.split('\n')):
        # a line whose text spells neither holds neither
        if 'if' not in spelled and 'while' not in spelled:
            continue
        found = [token for token in c.between(c.lines[line], c.lines[line + 1]) if token.kind != 'comment']
        for at, token in enumerate(found):
            if token.kind != 'name' or token.text not in ('if', 'while') or \
                    at and found[at - 1].text == '#' and found[at - 1].start in opened:
                continue
            after = (each.text for each in chain(found[at + 1:], c.onward(line + 1)) if each.kind != 'comment')
            if next(after, None) != '(':
                continue
            condition, depth = [], 1
            for each in after:
                depth += (each == '(') - (each == ')')
                if not depth:
                    break
                condition.append(each)
            zeroed = zero(condition)
            if zeroed and (token.text == 'if' or next(after, None) == '{'):
                yield token.line, f'{token.text} ({zeroed[0]}{" && ..." * zeroed[1]})'


def definitions(text):
    """The functions parse() finds in C text and its macros, as two {name: body} maps. A function's body is the texts
    of the tokens between its braces but comments and literals, and a macro's those macro_directives() gives."""
    c = parse(text)
    starts, functions = [token.start for token in c.tokens], {}
    for block in c.blocks:
        if block.name is not None:
            end = len(text) if block.closing is None else block.closing.start
            functions[block.name] = [token.text for token in c.tokens[bisect_right(starts, block.opening.start):
                                                                      bisect_left(starts, end)]
                                     if token.kind not in UNREAD]
    return functions, {name: macro[1] for _, name, macro in macro_directives(text) if macro is not None}


def calls(body, names):
    """Whether a body, as definitions() gives it, calls one of names, a set: names it, then opens a (."""
    return not names.isdisjoint(body) and any(word in names and after == '(' for word, after in zip(body, body[1:]))


def hosts(name, functions, macros):
    """The functions that call name, directly or through macros. A function that calls one of them is not one."""
    called = {name}
    while more := {macro for macro, body in macros.items() if macro not in called and calls(body, called)}:
        called |= more
    return [function for function, body in functions.items() if calls(body, called)]


def blocks(lines):
    """The lines that open a #pragma asm block holding more than assembler comments."""
    return [start + 1 for start, _, code in regions(lines) if code]


def registers(operand, fr, paired):
    """The registers an operand names, or None for a constant, a label or an address. fr is FPSCR's FR bit, which
    swaps the banks FRn and XFn name. paired is whether the instruction moves pairs, as an FMOV does with FPSCR's SZ
    bit set. FRn then names DRn for an even n, and XD(n-1) for an odd n."""
    match = REGISTER.match(operand)
    if not match:
        return None
    kind, n = (match[1] or match[0]).upper(), int(match[2] or 0)
    if kind in NUMBERED and n not in NUMBERED[kind]:
        return None
    if kind == 'R':
        return [f'R{n}']
    if kind == 'FR' and paired:
        kind, n = ('XD', n - 1) if n % 2 else ('DR', n)
    if kind not in ('FR', 'DR', 'XD', 'FV', 'XMTRX'):
        return [kind]
    bank = 'FB' if (kind in ('XD', 'XMTRX')) != fr else 'FA'
    return [f'{bank}{i}' for i in range(n, n + {'FR': 1, 'FV': 4, 'XMTRX': 16}.get(kind, 2))]


# an instruction's registers: those it reads, those it writes and those it pushes, each a list
Effects = namedtuple('Effects', 'reads writes pushed')


def effects(mnemonic, operands, fr, sz):
    """An instruction's Effects, or None for an instruction --abi doesn't read. fr and sz are FPSCR's FR and SZ bits.
    A push at @-R15 counts the registers it stores as read and as pushed. T counts as a register, and an RTS reads
    PR."""
    roles = ROLE.get(mnemonic)
    if roles is None or len(roles) != len(operands):
        return None
    paired = sz and mnemonic.startswith('FMOV')
    reads, writes = [], []
    for role, operand in zip(roles, operands):
        if address := AT.match(operand):
            base = [address[2] or address[5]] if (address[5] or '').upper() != 'PC' else []
            reads += base + ['R0'] * bool(address[4])
            writes += base * bool(address[1] or address[3])
        elif role != 'l' and (names := registers(operand, fr, paired)):
            reads += names * (role in 'rb')
            writes += names * (role in 'wb')
    reads += ['T'] * (mnemonic in READS_T) + ['PR'] * (mnemonic == 'RTS')
    writes += ['T'] * (mnemonic in WRITES_T)
    pushed = registers(operands[0], fr, paired) if [o.upper() for o in operands[1:]] == ['@-R15'] else None
    return Effects([read.upper() for read in reads], [write.upper() for write in writes], pushed or [])


def instructions(lines):
    """A body's instructions and labels, given its lines. Each instruction is (its index in lines, its mnemonic in
    capitals, its operands without blanks and with SP as R15). Each label maps to the index of the instruction below
    it. It maps to None where a directive comes first, and to the count of instructions where none follows."""
    found, labels, waiting = [], {}, []
    for i, line in enumerate(lines):
        statement = line.split(';')[0]
        while label := re.match(r'\s*([\w?]+):', statement):
            waiting.append(label[1])
            statement = statement[label.end():]
        mnemonic, _, rest = statement.strip().partition(' ')
        if not mnemonic:
            continue
        labels.update(dict.fromkeys(waiting, None if mnemonic.startswith('.') else len(found)))
        waiting = []
        if not mnemonic.startswith('.'):
            operands = re.split(r',(?![^(]*\))', ''.join(rest.split())) if rest.strip() else []
            found.append((i, mnemonic.upper(), [SP.sub('R15', operand) for operand in operands]))
    labels.update(dict.fromkeys(waiting, len(found)))
    return found, labels


def spelling(register):
    """A register as the body spells it at its entry: FA0 as FR0, and FB0 as XF0."""
    return {'FA': 'FR', 'FB': 'XF'}.get(register[:2], register[:2]) + register[2:]


def refusal(lines, code, labels, k):
    """Why --abi can't read code[k], or None where it can. lines are a body's lines, and code and labels its
    instructions and labels, as instructions() gives them. --abi doesn't read a BSR to a label of the body, a write to
    FPSCR, or an instruction ROLES doesn't hold with its operand count. It can't place a delayed branch's slot
    where no instruction follows the branch. It can't place one that a label marks or a directive stands above, or
    one that branches, jumps, calls or returns."""
    _, mnemonic, operands = code[k]
    # FR and SZ change only which registers an operand names, not whether effects() reads it or finds FPSCR written
    effect = effects(mnemonic, operands, False, False)
    if effect is None or 'FPSCR' in effect.writes or (mnemonic == 'BSR' and labels.get(operands[0]) is not None):
        return "which --abi doesn't read"
    if mnemonic not in DELAYED:
        return None
    if k + 1 == len(code) or k + 1 in labels.values() or code[k + 1][1] in DELAYED | {'BT', 'BF'}:
        return "whose slot --abi can't place"
    # instructions() keeps no directive, so a statement on a line between the branch's and its slot's is a directive or
    # a label
    if any(line.split(';')[0].strip() for line in lines[code[k][0] + 1:code[k + 1][0]]):
        return "whose slot --abi can't place"
    return None


def edges(code, labels):
    """For each of a body's instructions, the indexes of those control can reach next, with None for a way out of the
    body. code and labels are the body's instructions and labels, as instructions() gives them, and refusal() must
    pass each instruction. A delayed branch takes effect after its slot, so the slot holds the branch's edges."""
    after = [[k + 1] for k in range(len(code))]
    marked = sorted({to for to in labels.values() if to is not None})
    for k, (_, mnemonic, operands) in enumerate(code):
        to = labels.get(operands[0]) if ROLE.get(mnemonic) == 'l' else None
        slot = k + (mnemonic in DELAYED)
        if mnemonic in ('BT', 'BF', 'BT/S', 'BF/S'):
            after[slot] = [slot + 1, to]
        elif mnemonic == 'BRAF':
            after[slot] = [None, *marked]
        elif mnemonic in LEAVES or mnemonic == 'BRA':
            after[slot] = [to]
    return after


def moved(slots, offset):
    """The stack's slots once R15 moves offset bytes other than by a push: up where offset is positive, and down where
    it is negative. The slots are (size, registers) pairs, top last. A slot's registers are those whose values at
    entry the push that made it stored, or () for a slot a move made. A move up drops the slots it passes. Where it
    stops inside a slot, the rest of that slot becomes a slot of its own. A move down makes a slot of the bytes it
    passes."""
    while slots and offset > 0:
        offset -= slots[-1][0]
        slots = slots[:-1]
    return slots + ((-offset, ()),) if offset < 0 else slots


# what --abi knows on entry to a body's instruction, from the paths that reach it so far. defined holds the registers
# every path has set. clobbered maps each register a call can have changed since a path set it, and which the path
# hasn't written since, to the call's line. changed holds the kept registers a path has changed. slots holds the
# stack's slots, as moved() gives them, where every path leaves the same slots, and none where paths leave different
# ones. fr and sz are FPSCR's FR and SZ bits. stack is R15 less its value at entry. It is None once a path sets R15
# other than by a push, an ADD of an immediate or a pop into another register
State = namedtuple('State', 'defined clobbered changed slots fr sz stack')


def step(state, mnemonic, operands, line):
    """The state once an instruction at line runs from state, and each finding there, as (line, what the body does):
    a read of a register that no argument passes or that a call can have changed, and a move of R15 above its
    entry. effects() must read the instruction."""
    reads, writes, pushed = effects(mnemonic, operands, state.fr, state.sz)
    found = set()
    # a push saves the registers it stores and doesn't read them, so a body can save a kept register no argument passes
    for register in set(reads) - set(pushed):
        if register in state.clobbered:
            found.add((line, f'it reads {spelling(register)} at line {line}, which the call at line '
                             f'{state.clobbered[register]} can change'))
        elif register not in state.defined and register not in MACHINE:
            found.add((line, f'it reads {spelling(register)} at line {line}, which no argument passes'))
    upper = [operand.upper() for operand in operands]
    size = 8 if mnemonic.startswith('FMOV') and state.sz else {'.B': 1, '.W': 2}.get(mnemonic[-2:], 4)
    # a pop restores the registers it loads only from a slot that holds their whole values at entry
    popped = upper[:1] == ['@R15+']
    loaded = tuple(register for register in writes if register != 'R15')
    if popped and state.slots[-1:] == ((size, loaded),):
        changed = state.changed - set(loaded)
    else:
        changed = state.changed | (set(writes) & KEPT)
    # a store through R15 can write any slot
    stored = len(upper) > 1 and (upper[-1] == '@R15' or upper[-1].endswith(',R15)'))
    slots = () if stored else state.slots
    stack = state.stack
    if 'R15' in writes and stack is not None:
        immediate = IMMEDIATE.match(upper[0]) if mnemonic == 'ADD' and upper[1:] == ['R15'] else None
        if upper[1:] == ['@-R15']:
            stack -= size
            slots += ((size, tuple(pushed) if size >= 4 and set(pushed) <= KEPT - changed else ()),)
        elif popped and upper[1:] != ['R15']:
            stack += size
            slots = moved(slots, size)
        elif immediate:
            added = int(immediate[1]) if immediate[1] else int(immediate[2], 16)
            stack += added
            slots = moved(slots, added)
        else:
            stack, slots = None, ()
        if stack is not None and stack > 0:
            found.add((line, f"it takes R15 above its entry at line {line}, into its caller's frame"))
    clobbered = {register: at for register, at in state.clobbered.items() if register not in writes}
    return State(state.defined | set(writes), clobbered, changed, slots, state.fr ^ (mnemonic == 'FRCHG'),
                 state.sz ^ (mnemonic == 'FSCHG'), stack), found


def after_call(state, line, call):
    """The state once a call returns, and each finding there, as (line, what the body does). state is the state after
    the call's slot, line the slot's line and call the call's. A finding is a call made with FPSCR's FR or SZ bit set.
    The callee can change each register CLOBBERED holds. --abi takes R0 and FA0 as set after the call, since a callee
    returns its value in one of them."""
    found = {(line, f"its call at line {call} runs with FPSCR's {bit} bit set, which GCC's code takes clear")
             for bit, on in (('FR', state.fr), ('SZ', state.sz)) if on}
    clobbered = {**state.clobbered, **dict.fromkeys(state.defined & CLOBBERED - {'R0', 'FA0'}, call)}
    return state._replace(defined=state.defined - CLOBBERED | {'R0', 'FA0'}, clobbered=clobbered), found


def return_findings(state, line):
    """Each finding at a way out of the body at line, as (line, what the body does): a return with a register
    changed that GCC's callers keep, R15 among them, or with FPSCR's FR or SZ bit set."""
    changed = state.changed | ({'R15'} if state.stack != 0 else set())
    return ({(line, f'it returns with {spelling(register)} changed, which its GCC callers keep')
             for register in changed}
            | {(line, f"it returns with FPSCR's {bit} bit set, which GCC's code takes clear")
               for bit, on in (('FR', state.fr), ('SZ', state.sz)) if on})


def joined(old, new):
    """The state on entry to an instruction once a path that brings new joins the paths that brought old. It is None
    where the two have FR, SZ or R15 apart."""
    if (old.fr, old.sz, old.stack) != (new.fr, new.sz, new.stack):
        return None
    return old._replace(defined=old.defined & new.defined, clobbered={**new.clobbered, **old.clobbered},
                        changed=old.changed | new.changed, slots=old.slots if old.slots == new.slots else ())


def call_safety(path, name, lines, first, passed, problems):
    """The first instruction that leaves a body, and each finding: a place where GCC's function for the body may do
    otherwise than SHC's expansion, whatever the body's hosts do. lines are the body's lines, of which the first is
    line first of path, and passed the registers that pass the function's arguments. The instruction is (its line,
    its statement), or None where none leaves. The findings are sorted (line, what the body does) pairs, those that
    step(), after_call() and return_findings() give. The RTS that tools/inline.py writes after the body is a finding
    too where a call the body made can have changed the PR it reads.

    Where --abi can't read the body, call_safety() returns None and puts each reason on problems, naming path and its
    line. Each instruction refusal() names is a reason. Where refusal() names none, the reason is a place where two
    paths meet with FR, SZ or R15 apart. Where no paths meet so, the reason is the first instruction no path
    reaches."""
    code, labels = instructions(lines)
    statements = [' '.join(lines[i].split(';')[0].split()) for i, _, _ in code]
    refused = [f'{path}:{first + i}: {name}: {statements[k]}, {why}'
               for k, (i, _, _) in enumerate(code) if (why := refusal(lines, code, labels, k))]
    if refused:
        problems += refused
        return None
    after = edges(code, labels)
    # the first instruction that leaves the body: a jump, a return, or a branch that DELAYED's comment says leaves it
    leaving = next((k for k, (_, mnemonic, operands) in enumerate(code) if mnemonic in LEAVES or (
        ROLE.get(mnemonic) == 'l' and mnemonic != 'BSR' and labels.get(operands[0]) is None)), None)
    # each instruction's line, then the body's last line, which stands for the RTS that tools/inline.py writes after it
    numbers = [first + i for i, _, _ in code] + [first + len(lines) - 1]
    states = {0: State(frozenset([*passed, 'R15', 'PR']), {}, frozenset(), (), False, False, 0)}
    work, found = [0], set()
    while work:
        k = work.pop()
        state, line = states[k], numbers[k]
        if k == len(code):
            found |= return_findings(state, line)
            if 'PR' in state.clobbered:
                found.add((line, f'its return reads PR, which the call at line {state.clobbered["PR"]} can change'))
            continue
        state, more = step(state, *code[k][1:], line)
        found |= more
        # a call's slot runs before the callee
        if k and code[k - 1][1] in CALLS:
            state, more = after_call(state, line, numbers[k - 1])
            found |= more
        for to in after[k]:
            if to is None:
                found |= return_findings(state, line)
                continue
            new = joined(states[to], state) if to in states else state
            if new is None:
                problems.append(f"{path}:{numbers[to]}: {name}: two paths meet here with FPSCR's FR or SZ bits or "
                                "R15 apart, which --abi can't read")
                return None
            if new != states.get(to):
                states[to] = new
                work.append(to)
    unreached = next((k for k in range(len(code)) if k not in states), None)
    if unreached is not None:
        problems.append(f"{path}:{numbers[unreached]}: {name}: {statements[unreached]}, which no path of --abi's "
                        "reaches")
        return None
    return ((numbers[leaving], statements[leaving]) if leaving is not None else None), sorted(found)


def arguments(head):
    """The registers that pass a function's arguments, and the parameters whose type --abi can't place, as two lists,
    given its head, a Block's. Its pointers and integers take R4 to R7 in order, and its floats FA4 to FA11.
    INTEGER_TYPES and FLOAT_TYPES spell the integers' and the floats' types. A parameter past R7 or FA11 passes on the
    stack, in no register. --abi can't place a parameter of any other type, a long long among them."""
    spelled = [token.text for token in head]
    inside = head[spelled.index('(') + 1:len(spelled) - 1 - spelled[::-1].index(')')]
    # the parameters, split at each comma outside parentheses
    parameters, depth = [[]], 0
    for token in inside:
        depth += (token.text == '(') - (token.text == ')')
        if token.text == ',' and not depth:
            parameters.append([])
        else:
            parameters[-1].append(token)
    passed, unplaced = [], []
    for each in parameters if [token.text for token in inside] not in ([], ['void']) else []:
        written = [token.text for token in each]
        words = [word.text for word in each
                 if word.kind in ('name', 'number') and word.text not in ('const', 'volatile')]
        # the parameter's name, where it has one, follows its type
        if len(words) > 1 and words[-1] not in INTEGER_TYPES | FLOAT_TYPES:
            words.pop()
        if '*' in written or '[' in written or (words and set(words) <= INTEGER_TYPES and words.count('long') < 2):
            candidates = INTEGER_ARGUMENTS
        elif len(words) == 1 and words[0] in FLOAT_TYPES:
            candidates = FLOAT_ARGUMENTS
        else:
            unplaced.append(' '.join(written))
            continue
        passed += [register for register in candidates if register not in passed][:1]
    return passed, unplaced


def ends_on(name, body):
    """Whether a function's body, as definitions() gives it, calls name, and ends the function at each call: by
    returning the call's value, or with the call as its last statement. A body that calls name only through a macro
    doesn't."""
    sites = [at for at, (word, after) in enumerate(zip(body, body[1:])) if word == name and after == '(']
    return bool(sites) and all(at and body[at - 1] == 'return' or ';' in body[at:] and
                               body.index(';', at) == len(body) - 1 for at in sites)


def check_c(path, text, tables, problems, abi=None):
    """Check a C source's tags, and return each body and block as (line, what, its @asm tag's [kind, text]), with
    None for no tag. Where abi is a list, check_c() puts on it each body GCC compiles whose @asm tag is abi or that
    may not be call-safe. Each entry is (line, its function's name, why). A body --abi can't read goes on it too,
    with why None. Each reason --abi can't read a body goes on problems. So does a problem naming each #pragma asm
    block GCC compiles whose @asm tag is abi."""
    code = blanked(text)
    lines = code.split('\n')
    sites = {i + 1: name for i, name in inline_sites(lines).items()}
    problems.extend(f'{path}:{each.line}: not #pragma inline_asm(<function>) or '
                    '#pragma inline_asm(<function>(size=<n>))' for each in parse(code).directives
                    if (pragma(each) or [''])[0] == 'inline_asm' and each.line not in sites)
    inline_bodies = bodies(path, lines, problems)
    assembly = blocks(lines)
    functions, macros = definitions(code) if sites else ({}, {})
    # each body's and block's @asm tag's kind and text, by its line
    tagged = {}
    # each @hack tag's kind and first line, by the line directly below it
    hacks = {}
    for end, start, body in comments(text):
        found = tag(f'{path}:{start}', body, problems)
        if not found:
            continue
        name, *found = found
        if name == 'hack':
            hacks[end + 1] = found[0], start
        if name != 'asm':
            continue
        if end + 1 in tagged:
            what = f'#pragma inline_asm({sites[end + 1]})' if end + 1 in sites else '#pragma asm'
            problems.append(f'{path}:{start}: a second @asm tag for {what}')
            continue
        if end + 1 in assembly:
            tagged[end + 1] = found
            cited(f'{path}:{start}', *found, tables.rows.get(path, []), problems, 'its source\'s rows')
            continue
        name = sites.get(end + 1)
        if name is None:
            problems.append(f'{path}:{start}: an @asm tag not directly above a #pragma inline_asm or a #pragma asm '
                            'block')
            continue
        tagged[end + 1] = found
        ranges = tables.ranges(path, name)
        for host in hosts(name, functions, macros):
            ranges += tables.ranges(path, host)
        cited(f'{path}:{start}', *found, ranges + tables.mentioned(path, found[1]), problems)
    for line, test in never_run(text):
        below = line
        while below in hacks and hacks[below][0] != 'redundant':
            below = hacks[below][1]
        if below not in hacks:
            problems.append(f'{path}:{line}: {test} has no @hack redundant tag above it')
    if abi is not None:
        shipped = shipped_side(lines)
        # the functions and macros GCC compiles: one on a guard's shipped side calls no body's function
        functions, macros = definitions('\n'.join('' if i in shipped else each for i, each in enumerate(lines)))
        problems.extend(f'{path}:{line}: a #pragma asm block whose @asm tag is abi, which GCC compiles and no '
                        '--contract can name' for line in assembly
                        if line - 1 not in shipped and tagged.get(line, [None])[0] == 'abi')
        for at, name, _, opening, closing in inline_bodies:
            if at in shipped:
                continue
            if tagged.get(at + 1, [None])[0] == 'abi':
                abi.append((at + 1, name, 'its @asm tag is abi'))
                continue
            passed, unplaced = arguments(next(block.head for block in parse(code).blocks
                                              if block.opening.line - 1 == opening))
            problems.extend(f"{path}:{at + 2}: {name}: its parameter {each}, which --abi can't place"
                            for each in unplaced)
            safety = None if unplaced else call_safety(path, name, text.split('\n')[opening + 1:closing],
                                                        opening + 2, passed, problems)
            if safety is None:
                abi.append((at + 1, name, None))
                continue
            leaving, found = safety
            goes_on = [host for host in hosts(name, functions, macros) if not ends_on(name, functions[host])]
            if leaving and goes_on:
                found.append((leaving[0], f'its {leaving[1]} at line {leaving[0]} leaves it, and {goes_on[0]} goes '
                                          'on past its call'))
            if found:
                abi.append((at + 1, name, min(found)[1]))
    return sorted([(line, f'#pragma inline_asm({name})', tagged.get(line)) for line, name in sites.items()] +
                  [(line, '#pragma asm', tagged.get(line)) for line in assembly], key=lambda piece: piece[0])


def inline(text):
    """Whether C text holds a #pragma inline_asm or #pragma asm, as check_c
    reads them: R10 takes either only under -code=asmcode."""
    # each spells asm, so text that never does goes unread
    return 'asm' in text and any((pragma(each) or [''])[0] == 'inline_asm' or pragma(each) == ['asm']
                                 for each in parse(text).directives)


def sources(path):
    """The C source at path, or for a directory each .c file under it."""
    if not os.path.isdir(path):
        yield path
        return
    try:
        entries = list(os.scandir(path))
    except OSError as error:
        fail(f'{path}: {error.strerror}')
    for entry in entries:
        if entry.is_dir(follow_symlinks=False):
            yield from sources(entry.path)
        elif entry.name.endswith('.c'):
            yield entry.path


def symbol(label):
    """The name the image tables give a label: its C spelling, without the linker's leading underscore."""
    return label[1:] if label.startswith('_') else label


def starts(lines):
    """Each run of labels that starts code, as their lines' indexes: labels
    with no statement between them, and after them an instruction, not a
    directive such as .DATA.L. A local label in between is part of the code."""
    found, run = [], []
    for i, line in enumerate(lines):
        statement = line.split(';')[0]
        if match := LABEL.match(statement):
            run.append(i)
            statement = statement[match.end():]
        statement = statement.strip()
        if match := LOCAL.match(statement):
            statement = statement[match.end():].strip()
        if statement:
            if run and not statement.startswith('.') and not VALUE.match(statement):
                found.append(run)
            run = []
    return found


def check_src(path, text, tables, problems, abi=None):
    """Check an assembly source's tags, and return each function as (line, its names, its @asm tag's [kind, text]),
    with None for no tag. A function is a run of labels that starts code, as starts() gives them, with a label the
    source exports or the image tables name. Its names are those labels. A run with neither starts no function. Where
    abi is a list, each label tagged abi goes on it as (line, its function's name as a --contract names it, why)."""
    lines = text.split('\n')
    exported = {name.strip() for line in lines if (match := EXPORT.match(line)) for name in match[1].split(',')}
    # each label's line, and its function's ranges: those the tables give it
    # and the other labels of its run, which name the same code, or where they
    # give none, those of the last label above it they name
    functions, last = {}, []
    for i, line in enumerate(lines):
        if match := LABEL.match(line):
            last = functions[i] = tables.ranges(path, symbol(match[1])) or last
    # each run of labels that starts code, as two {index: label} maps: its labels, and those the source exports or the
    # tables name. A run starts a function where the second map holds a label
    runs = []
    for run in starts(lines):
        labels = {i: LABEL.match(lines[i])[1] for i in run}
        names = {i: label for i, label in labels.items() if label in exported or tables.ranges(path, symbol(label))}
        if own := [found for label in labels.values() for found in tables.ranges(path, symbol(label))]:
            functions.update(dict.fromkeys(labels, own))
        runs.append((labels, names))
    code = {i for labels, _ in runs for i in labels}
    # each label's @asm tag's kind and text, and where the tag starts, by the label's index
    tagged, at = {}, {}
    i = 0
    while i < len(lines):
        if not TAG.match(lines[i]):
            i += 1
            continue
        start, i = i, i + 1
        while i < len(lines) and NOTE.match(lines[i]) and not TAG.match(lines[i]):
            i += 1
        where = f'{path}:{start + 1}'
        found = tag(where, ' '.join(' '.join(NOTE.match(line)[1] for line in lines[start:i]).split()), problems)
        if TAG.match(lines[start])[1] != 'asm':
            continue
        if i == len(lines) or not LABEL.match(lines[i]):
            problems.append(f'{where}: an @asm tag not at the end of the comment above a label')
        elif i not in code:
            problems.append(f'{where}: an @asm tag above {LABEL.match(lines[i])[1]}, a label that starts no code')
        elif found:
            tagged[i], at[i] = found[1:], where
            cited(where, *found[1:], functions[i] + tables.mentioned(path, found[2]), problems)
    found = []
    for labels, names in runs:
        if len(second := [i for i in labels if i in tagged]) > 1:
            problems.append(f'{at[second[1]]}: a second @asm tag for {", ".join(labels.values())}')
        if not names:
            problems.extend(f'{at[i]}: an @asm tag above {label}, which starts no function'
                            for i, label in labels.items() if i in tagged)
            continue
        found.append((next(iter(names)) + 1, ', '.join(names.values()),
                      next((tagged[i] for i in labels if i in tagged), None)))
        if abi is not None:
            abi += [(i + 1, symbol(names.get(i) or next(iter(names.values()))), 'its @asm tag is abi') for i in labels
                    if tagged.get(i, [None])[0] == 'abi']
    return found


def summary(kinds):
    """The line --review ends on: how many bodies, blocks and functions there are, and how many have an @asm tag, by
    kind, given each one's tag's kind, or None."""
    tagged = [kind for kind in kinds if kind]
    each = ', '.join(f'{tagged.count(kind):,} {kind}' for kind in KINDS)
    return (f'{len(tagged):,} of {len(kinds):,} inline bodies, #pragma asm blocks and .src functions have an @asm tag '
            f'({each})')


def main(argv):
    parser = argparse.ArgumentParser(prog='tags', description=__doc__.split('\n\n')[0])
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--review', action='store_true',
                      help="also list each open @asm tag, then count the assembly by its tag's kind")
    mode.add_argument('--inline', action='store_true',
                      help='check no tag, but name each C source that holds a #pragma inline_asm or #pragma asm')
    mode.add_argument('--abi', action='store_true',
                      help='also fail on each function whose @asm tag is abi, and each inline body that may not be '
                           'call-safe, that no --contract names; on each --contract that names no such function; on '
                           'each #pragma asm block GCC compiles whose @asm tag is abi; and on each inline body it '
                           "can't read")
    parser.add_argument('--contract', metavar='NAME', action='append', default=[],
                        help='with --abi, a function, as C spells it, whose @asm tag is abi or whose inline body '
                             'may not be call-safe, and whose GCC callers meet its contract or that no GCC code '
                             'calls')
    parser.add_argument('paths', metavar='FILE', nargs='+',
                        help='a C or assembly source, named as splits.txt names it: src/system/thread.c, '
                             'not ./src/system/thread.c; for --inline, a directory stands for each .c file under it')
    args = parser.parse_args(argv)
    if args.contract and not args.abi:
        parser.error('--contract needs --abi')
    if args.inline:
        for path in args.paths:
            for source in sources(path):
                if inline(content(source).decode('latin-1')):
                    print(source)
        return
    tables = Tables(args.paths)
    problems, pieces, abi = [], [], []
    for path in args.paths:
        check = check_src if path.endswith('.src') else check_c
        entries = [] if args.abi else None
        pieces += [(path, *piece) for piece in check(path, content(path).decode('latin-1'), tables, problems,
                                                       entries)]
        abi += [(path, *entry) for entry in entries or []]
    problems += [f'{path}:{line}: {what} has no @asm tag' for path, line, what, found in pieces if found is None]
    if args.abi:
        problems += [f'{path}:{line}: {name}: {why}, and no --contract names it'
                     for path, line, name, why in abi if why is not None and name not in args.contract]
        nameable = {name for _, _, name, _ in abi}
        problems += [f'{name}: --contract names no function whose @asm tag is abi or that may not be call-safe'
                     for name in args.contract if name not in nameable]
    if args.review:
        for path, line, what, found in sorted(pieces, key=lambda piece: piece[:2]):
            if found and found[0] == 'open':
                print(f'{path}:{line}: {what}: {found[1]}')
        print(summary([found and found[0] for *_, found in pieces]))
    if problems:
        fail(*problems)


if __name__ == '__main__':
    main(sys.argv[1:])
