"""Respell the decimal floating constants of a source's preprocessed C as
SHC reads them, fold its static initializers as SHC does, and rewrite the
products and += that GCC would fuse otherwise than SHC, by
tools/float_fuse.py's rule, for make gcc. Or check an assembly text's
floating constants.

    python3 tools/floats.py --shc RELEASE [FLAG]... [--system DIR]... C
    python3 tools/floats.py --assembly TEXT

C is a source's C as GCC's preprocessor writes it, such as
build/gcc/src/main.i, which the tool rewrites in place. RELEASE, r10, r26
or r32, is the SHC that builds the source, and each FLAG one of its
-fpu=single, -double=float and -aggressive=2 that its build takes, the
tool reading no other of its flags. Each DIR holds system headers,
as SH4ZAM's include directory does for make gcc. The tool fails on each
problem below and in its modules' docstrings, naming its file and line, and
then leaves C as it was. All it doesn't rewrite stays byte for byte.

What the tool holds to is what its tests and the tree's builds show: under
GCC 16.2 as gcc.mk pins it and GCCFLAGS as config/gcc.mk gives them, for
R10, R26 and R32 as config/compilers.mk builds each source, and for the
tree as it stands. It holds C to the rules its docstrings give and the
spellings they name, not to SHC: where neither reaches, GCC computes what C
writes, and the tool can't tell where that parts from SHC.

The tool reads C through tools/csource.py, and each float_*.py module it
imports gives its own rules.

Code and an automatic object's initializer keep their arithmetic as C
groups it. Under -frounding-math, which GCC calls experimental, GCC 16.2
leaves to run time each operation of constants whose value rounds, of the
kinds the tree's code holds. tests/test_floats.py's Gcc tests hold it to
that at each level and contraction make gcc gives, for an inexact sum,
difference, product, quotient and square root, a product inside a product,
and a product an integer conversion takes. The console's FPU rounds such an
operation toward zero in binary32, as each SHC release's folder rounds a
float. But SHC can read a run of floating * operations, or of + and -
operations, as one, through parentheses and a unary -, where a unary + ends
a run of *, as src/system/util.h's RADIANS_TO_ANGLE shows: it folds the
run's constants into one, left to right, applies that after the run's other
operands, and multiplies a run of *'s other operands in an order of its
own. The tool follows neither: it writes no run otherwise than C, and fails
on none. Under -aggressive=2 it fails on a division SHC computes by a
reciprocal no normal float holds exactly, as tools/float_fold.py says. A
source that needs SHC's value spells SHC's grouping, as x * (0.1F * 0.3F)
for SHC's x * 0.1F * 0.3F, which SHC compiles to the same code; the tool
can't tell a run whose C grouping gives another value than SHC's from one
that gives SHC's.

The tool writes C in four forms and no other: a decimal floating constant
as the hexadecimal one; an initializer the fold reads as its value; a
product P as the wrap; and L += R as the addition. None changes what an
lvalue designates or how often an object is read or written, as __typeof__
evaluates nothing, and none evaluates an operand a ?:, && or || skips: an
initializer it folds is constant, and a ?:, && or || stays whole. Each
orders what C leaves unsequenced only as C allows, but the addition: the +=
reads L after each call R makes, where the addition leaves that read
unsequenced with R, so a call in R that writes the object L names can write
it after the addition reads it. Where it fuses R into the addition,
GCC 16.2 reads L after R, as the addend of the multiply-add it builds, as
tests/test_floats.py's Gcc tests hold it to at -Os and -O2 for a call in R,
where L is a name and where it is *p.

GCC.md#floats gives what make gcc does with C and TEXT.
"""

import argparse
import sys

from float_asm import assembled
from float_edits import Plan
from float_fold import fold
from float_fuse import fuse
from float_scope import Scopes
from float_tokens import marked
from float_value import Route, floating, respelled
from layout import Types
from tool import content, fail, save


def translate(text, path, route, system=()):
    """text, GCC's preprocessed C at path, as the tool writes it under route,
    or None where it fails; and the problems, each naming its file and line.
    Where a floating constant on the tree's lines has a problem, the
    problems are those alone. translate() reads text as tokens once, whose
    values hold SHC's value of each such constant. system is as marked()
    takes it."""
    toks, problems = marked(text, path, system), []
    for at, token in enumerate(toks):
        if token.kind == 'number' and not toks.system[at] and floating(token.text):
            try:
                spelling, toks.values[at] = respelled(token.text, route)
                toks[at] = token._replace(text=spelling)
            except ValueError as error:
                problems.append(f'{toks.where(at)}: {token.text}: {error}')
    if problems:
        return None, problems
    types = Types([])
    file = types.take([token.text for token in toks])
    plan = Plan(toks)
    try:
        scopes = Scopes(toks, types, file)
        problems += fold(toks, scopes, route, plan)
        problems += fuse(toks, scopes, route, plan)
        written = plan.text(text)
    except ValueError as error:
        problems.append(str(error))
    except IndexError:
        problems.append(f'{toks.where(len(toks) - 1)}: C that ends inside a declaration')
    return (None if problems else written), problems


USAGE = '''%(prog)s --shc RELEASE [FLAG]... [--system DIR]... C
       %(prog)s --assembly TEXT'''


def main(argv):
    parser = argparse.ArgumentParser(prog='floats', usage=USAGE, description=__doc__.split('\n\n')[0])
    parser.add_argument('c', nargs='?', metavar='C', help="respell and fold C, a source's preprocessed C, in place")
    parser.add_argument('--shc', choices=('r10', 'r26', 'r32'), metavar='RELEASE',
                        help='the SHC release that builds the source: r10, r26 or r32')
    parser.add_argument('-fpu', choices=('single',),
                        help="FLAG: SHC's -fpu=single for the source, which R10 doesn't take")
    parser.add_argument('-double', choices=('float',), help="FLAG: SHC's -double=float for the source")
    parser.add_argument('-aggressive', choices=('2',),
                        help="FLAG: SHC's -aggressive=2 for the source, which the probes read under R32 alone")
    parser.add_argument('--system', action='append', default=[], metavar='DIR',
                        help="a directory of system headers, whose lines keep their constants as GCC reads them")
    parser.add_argument('--assembly', metavar='TEXT', help="check TEXT's .FDATA.S constants")
    args = parser.parse_args(argv)
    if args.assembly and (args.c or args.shc or args.fpu or args.double or args.aggressive or args.system):
        parser.error('--assembly takes no C, --shc, --system or flags')
    if not args.assembly and not (args.c and args.shc):
        parser.error('name --shc and C, or --assembly and TEXT')
    if args.shc == 'r10' and args.fpu:
        parser.error('-fpu=single: R10 takes -double=float alone')
    if args.aggressive and args.shc != 'r32':
        parser.error('-aggressive=2: the probes read it under R32 alone')
    if args.assembly:
        problems = assembled(content(args.assembly).decode('latin-1'), args.assembly)
        if problems:
            fail(*problems)
        return
    route = Route(args.shc, bool(args.fpu or args.double), bool(args.aggressive))
    text = content(args.c).decode('latin-1')
    written, problems = translate(text, args.c, route, args.system)
    if problems:
        fail(*problems)
    if written != text:
        save(args.c, written.encode('latin-1'))


if __name__ == '__main__':
    main(sys.argv[1:])
