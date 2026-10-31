"""How a table's rows read, for each tool that reads a table.

lines() splits a text file into lines as awk does, and rows() and table()
split each line of a table into its fields, at runs of whitespace, without
comments or blank lines. The patterns spell a row's fields, address(),
interval() and boundaries() read them, and named() reads a symbols.txt's
rows. Every range is [start, end) in linked addresses. lines() fails on a
line that isn't UTF-8, and address(), interval() and named() on a field or
row out of its form, each naming its path:line.

TOOLCHAIN.md#image-tables gives the rows of splits.txt and symbols.txt and
their rules, and TOOLCHAIN.md#generated-ranges data.txt's.
"""

import re

from tool import content, fail

ADDRESS = re.compile(r'0x[0-9a-f]{8}')
BYTE = re.compile(r'0x[0-9a-f]{2}')
# a boundary declaration's alignment, in decimal. It takes at most nine
# digits, so that int() reads any it matches.
ALIGNMENT = re.compile(r'[1-9][0-9]{0,8}')
SECTION = re.compile(r'[A-Z][A-Z0-9]*')
NAME = re.compile(r'[A-Za-z_][A-Za-z0-9_]*')
# a place in a C source, as uses.txt names one: a table, a function, or a
# table's row, TABLE[N]. N takes at most nine digits, as ALIGNMENT does.
PLACE = re.compile(r'([A-Za-z_][A-Za-z0-9_]*)(?:\[(0|[1-9][0-9]{0,8})\])?')
# a definition, as a row of an image's data.txt gives it past its address:
# a kind or a C type, and [COUNT] for an array of COUNT. COUNT takes at most
# nine digits, as ALIGNMENT does.
DEFINITION = re.compile(r'([^[\]]+)(?:\[([1-9][0-9]{0,8})\])?')
# vouched, ending a row of data.txt, says each word of the row's definition
# whose value lies in RAM is an address. Only make gcc's content acts on it
# (TOOLCHAIN.md#generated-ranges).
VOUCHED = 'vouched'
# a placement's source, and a gen/ range's: gen/<image>/d_<address>.c
SOURCE = re.compile(r'(?:[a-z0-9_]+/)+[A-Za-z0-9_]+\.(?:c|src)')
GENERATED = re.compile(r'gen/([a-z0-9_]+)/d_([0-9a-f]{8})\.c')


def lines(path):
    """Yield each line of the text file at path as its number and its text.
    A line ends at a newline alone, as awk ends one. A line that isn't
    UTF-8 fails, naming its path:line."""
    found = content(path).split(b'\n')
    if found[-1] == b'':
        found.pop()
    for number, line in enumerate(found, 1):
        try:
            yield number, line.decode()
        except UnicodeDecodeError:
            fail(f"{path}:{number}: a byte that isn't UTF-8")


def rows(path):
    """Yield each row of a table as its path:line and its fields, without comments or blank lines."""
    for number, line in lines(path):
        fields = line.split('#')[0].split()
        if fields:
            yield f'{path}:{number}', fields


def table(path):
    """Yield each row of a table as its fields."""
    return (fields for _, fields in rows(path))


def hexa(address):
    """address as the tables write one: 0x and eight lowercase hex digits."""
    return f'0x{address:08x}'


def aligned(address, alignment):
    """address rounded up to a multiple of alignment."""
    return -(-address // alignment) * alignment


def address(field, where):
    """The address a table's field gives. A field that isn't 0x and eight
    lowercase hex digits fails, naming where."""
    if not ADDRESS.fullmatch(field):
        fail(f'{where}: {field}: not an address, 0x and eight lowercase hex digits')
    return int(field, 16)


def interval(start, end, where):
    """A row's [start, end), from its two address fields. One that holds no
    byte fails, naming where."""
    start, end = address(start, where), address(end, where)
    if start >= end:
        fail(f'{where}: {hexa(start)} to {hexa(end)} holds no byte')
    return start, end


def boundaries(fields):
    """The boundary declarations in fields, a placement row's fields after its
    two addresses, as {kind: alignment}.

    A declaration is end, whose alignment is None, or start N or continues N,
    N a power of two of at most nine decimal digits. Each kind appears at most
    once, and start and continues never together. TOOLCHAIN.md#fill gives each
    kind's meaning.

    {} where fields is empty, and None where it breaks these rules."""
    found, rest = {}, list(fields)
    while rest:
        kind = rest.pop(0)
        if kind in found or kind not in ('end', 'start', 'continues'):
            return None
        if kind == 'end':
            found[kind] = None
            continue
        alignment = rest.pop(0) if rest else ''
        if not ALIGNMENT.fullmatch(alignment) or int(alignment) & (int(alignment) - 1) or found.keys() - {'end'}:
            return None
        found[kind] = int(alignment)
    return found


def spell(declaration):
    """A boundary declaration, (kind, alignment), as its row spells it."""
    kind, alignment = declaration
    return kind if alignment is None else f'{kind} {alignment}'


def named(path):
    """A symbols.txt's (address, name) rows. Two names can share an address; a name has one row."""
    found, seen = [], {}
    for where, row in rows(path):
        if len(row) != 2 or not NAME.fullmatch(row[1]):
            fail(f'{where}: not a row: ADDRESS NAME')
        if row[1] in seen:
            fail(f'{where}: {row[1]} has a row already, at {seen[row[1]]}')
        seen[row[1]] = where
        found.append((address(row[0], where), row[1]))
    return found
