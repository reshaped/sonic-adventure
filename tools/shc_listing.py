"""Read a listing SHC or asmsh writes: where each datum lies in its object's
sections, the labels each function's code names, and each string.

SHC lists the object it compiles under -listfile and -show=object, and
asmsh the one it assembles under -list. tools/data.py reads a body source's
stub listing through these, for where each of its data lies.
"""

import re

from tool import content, fail


# the bytes in a listing directive's .B, .W or .L unit
UNITS = {'B': 1, 'W': 2, 'L': 4}
# the banner asmsh's listing opens with, after a form feed
ASSEMBLED = re.compile(r'\f?\*\*\* SuperH RISC engine ASSEMBLER')
# in a listing: a static's label, a datum's directive and its operands, the
# function a row of code begins, the labels a row names, and a row of an L
# label alone, as an unnamed datum or a string starts with. A .SDATA's
# operand is its string in quotes, which can hold a ;, where a comment
# follows the string or nothing does. Any other operands run to the ; that
# starts a comment, as do those of a .SDATA with other text past its string,
# which sdata() fails on
STATIC = re.compile(r'\w+: +; static: (\w+)')
QUOTED = re.compile(r'"(?:[^"]|"")*"')
DIRECTIVE = re.compile(r'(\.(?:DATA|DATAB|RES|SDATA)(?:\.[BWL])?) +(' + QUOTED.pattern + r'(?=\s*(?:;|$))|[^;]*)')
BEGINS = re.compile(r'; function: (\w+)')
LABELS = re.compile(r'\bL[0-9]+\b')
UNNAMED = re.compile(r'(L[0-9]+): *')
# a .DATAB's or .RES's count, and a byte of .DATA.B or .DATAB.B, as SHC and
# asmsh write them
COUNT = re.compile(r'[0-9]+')
BYTE = re.compile(r"H'([0-9A-F]{1,2})|([0-9]{1,3})")
# in SHC's listing: a section's size, a row's section, offset and the rest,
# a row that goes on with the operands above it, and the line marker
# -optimize=0 writes ahead of a section's first code, which gives the
# section with no offset
SHC_SECTION = re.compile(r'\w+ +SECTION \((\w+)\): +([0-9A-F]+) Byte')
SHC_ROW = re.compile(r'(...) ([0-9A-F]{8}) [0-9A-F<> ]{10} *(.*)$')
SHC_MORE = re.compile(r' {13}[0-9A-F<> ]{10} *\+ +([^;]*)')
SHC_MARKER = re.compile(r'(\w{1,3}) +; ')
# in SHC's listing, the label of a global whose name has 31 characters or
# more, and its comment. The label runs past the comment's column, so the
# comment goes on the next line. compiled() joins the two, and leaves a blank
# line in the comment's place, so that each line keeps its number
SHC_WRAPPED = re.compile(r'^(.* _\w+:)\n +(; (?:static|function): \w+)$', re.M)


def full_section(where, given, sizes, abbreviated=True):
    """The one section of a listing that a row gives: the one whose name
    starts with the row's three letters, or, where the row's name isn't
    abbreviated, the one of that name. Fails, naming where, the row's path
    and line, where no section or several do."""
    full = [name for name in sizes if (name[:3] if abbreviated else name) == given]
    if len(full) != 1:
        fail(f'{where}: no one section {"starts" if abbreviated else "is named"} {given}')
    return full[0]


def sdata(operand, where):
    """The bytes of a .SDATA's operand, as DIRECTIVE reads it: the string in
    its quotes, each doubled quote one quote. listing() sizes a string by
    them, and strings() takes them. Fails, naming where, the row's path and
    line, on an operand that isn't one whole string."""
    if not QUOTED.fullmatch(operand.strip()):
        fail(f'{where}: .SDATA {operand.strip()}: no whole string')
    return operand.strip()[1:-1].replace('""', '"').encode('latin-1')


def unit(op, where):
    """The bytes in the unit op names, a .DATA, .DATAB or .RES as DIRECTIVE
    reads it. Fails, naming where, the row's path and line, on one that
    names no .B, .W or .L."""
    if op[-2:-1] != '.':
        fail(f'{where}: {op}: no .B, .W or .L unit')
    return UNITS[op[-1]]


def directive_count(operands, where):
    """The count a .DATAB's or .RES's operands start with. Fails, naming
    where, the row's path and line, on one that isn't COUNT's."""
    found = operands.split(',')[0].strip()
    if not COUNT.fullmatch(found):
        fail(f'{where}: {operands.strip()}: its count isn\'t a decimal number')
    return int(found)


def byte(value, where):
    """The byte a .DATA.B's or .DATAB.B's value gives. Fails, naming
    where, the row's path and line, on one that isn't BYTE's or is past
    255."""
    found = BYTE.fullmatch(value.strip())
    if found and found.group(1):
        return int(found.group(1), 16)
    if not found or int(found.group(2)) > 255:
        fail(f'{where}: {value.strip()}: no byte')
    return int(found.group(2))


def listing(path):
    """From the listing at path, SHC's or asmsh's:

    - each named datum, as its name, section, offset and size, in the
      listing's order. Two functions' statics can share a name, and each
      has its own;
    - each unnamed datum outside the code, as an automatic array's template
      is: its section, offset and size, by its L label;
    - the L labels named in each function's code and pools, by the function.

    SHC rows abbreviate section names to three letters; its section table
    gives the full names. asmsh's .SECTION directives give full names,
    which are kept. A listing is read one byte to a character.
    Both listings end each line in CRLF, which is read as LF, the line end
    SHC_WRAPPED matches, and only LF ends a line. A .SDATA's string keeps
    its semicolons and doubled quotes, and a comment starts only past it.

    Fails, naming the row's path and line, where the row names no one
    section, where a datum runs past its section, or where a .DATAB's or
    .RES's count isn't a decimal number, or a directive whose unit sizes a
    datum names no .B, .W or .L. SHC's listing fails so too where a .SDATA's
    operand isn't one whole string. asmsh's cuts a long string short, so a
    .SDATA there is sized by its code, whatever its operand. A listing that
    gives no section's size fails, naming it."""
    text = content(path).decode('latin-1').replace('\r\n', '\n')
    abbreviated = not ASSEMBLED.match(text)
    data, unnamed, sizes, loads = (compiled if abbreviated else assembled)(text, path)
    if not sizes:
        fail(f'{path}: gives no section\'s size, as SHC\'s and asmsh\'s listings do')

    def placed(section, name, start, end, line):
        full = full_section(f'{path}:{line}', section, sizes, abbreviated)
        if end > sizes[full]:
            fail(f'{path}:{line}: {name} runs past its section, {full}, of {sizes[full]} bytes')
        return name, full, start, end - start

    named = [placed(*datum) for datum in data]
    return named, {label: tuple(place) for label, *place in (placed(*datum) for datum in unnamed)}, loads


def compiled(text, path):
    """listing()'s named data, unnamed data, section sizes and loads, from
    SHC's listing at path. A datum is its section's letters, name, start,
    end and line."""
    data, sizes, section, datum, directive = [], {}, None, None, None
    unnamed, loads, function = [], {}, None
    text = SHC_WRAPPED.sub(r'\1 \2\n', text)

    def close():
        """Extend the datum over the last directive, its continuation lines
        too."""
        if datum is None or directive is None:
            return
        offset, op, operands, where = directive
        if op.startswith('.RES') and datum[3] > datum[2]:
            # a .RES after the datum's values is SHC's padding before the next
            # datum
            return
        if op.startswith('.SDATA'):
            size = len(sdata(operands, where))
        elif op.startswith(('.DATAB', '.RES')):
            size = unit(op, where) * directive_count(operands, where)
        else:
            size = unit(op, where) * len([value for value in operands.split(',') if value.strip()])
        datum[3] = max(datum[3], offset + size)

    for number, line in enumerate(text.split('\n'), 1):
        # the section, the offset, the code and the rest, in their columns
        info = SHC_SECTION.match(line)
        row = SHC_ROW.match(line)
        more = SHC_MORE.match(line)
        marker = SHC_MARKER.match(line)
        if info:
            sizes[info.group(1)] = int(info.group(2), 16)
        elif more and directive is not None:
            directive = (directive[0], directive[1], directive[2] + ',' + more.group(1), directive[3])
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
            alone = UNNAMED.fullmatch(rest)
            op = DIRECTIVE.match(rest)
            in_code = section is not None and section.startswith('P')
            begins = BEGINS.search(rest)
            if in_code:
                function = begins.group(1) if begins else function
                loads.setdefault(function, set()).update(LABELS.findall(rest))
            if label:
                datum = [section, label.group(1), offset, offset, number]
                data.append(datum)
            elif alone and not in_code:
                datum = [section, alone.group(1), offset, offset, number]
                unnamed.append(datum)
            elif op and datum is not None:
                directive = (offset, op.group(1), op.group(2), f'{path}:{number}')
            else:
                # code, alignment, or a label of no datum's: the datum has ended
                datum = None
    close()
    return data, unnamed, sizes, loads


def assembled(text, path):
    """listing()'s named data, unnamed data, section sizes and loads, from
    asmsh's listing at path, as compiled() gives them, but with each
    section's full name.

    A row is its line number, the offset, the code, and the source's line
    number and text. A row of an offset and code alone goes on with the code
    above it. The listing cuts the source's long lines short, so a datum's
    values are measured by their code. .SECTION starts a section, and the
    section list at the end gives the sizes."""
    data, sizes, section, datum, in_values = [], {}, None, None, False
    unnamed, loads, function = [], {}, None
    for number, line in enumerate(text.split('\n'), 1):
        size = re.match(r'(\w+) +REL-(?:CODE|DATA) +([0-9A-F]+)', line)
        row = re.match(r' *\d+ ([0-9A-F]{8}) ([0-9A-F]*) +\d+ (.*)$', line)
        more = re.match(r' {6}([0-9A-F]{8}) ([0-9A-F]+) *$', line)
        if size:
            sizes[size.group(1)] = int(size.group(2), 16)
        elif more:
            if datum is not None and in_values:
                datum[3] = max(datum[3], int(more.group(1), 16) + len(more.group(2)) // 2)
        elif row:
            offset, code, rest = int(row.group(1), 16), row.group(2), row.group(3).strip()
            in_values = False
            begun = re.match(r'\.SECTION +(\w+)', rest)
            if begun:
                section, datum = begun.group(1), None
                continue
            label = STATIC.match(rest)
            alone = UNNAMED.fullmatch(rest)
            op = DIRECTIVE.match(rest)
            in_code = section is not None and section.startswith('P')
            begins = BEGINS.search(rest)
            if in_code:
                function = begins.group(1) if begins else function
                loads.setdefault(function, set()).update(LABELS.findall(rest))
            if label:
                datum = [section, label.group(1), offset, offset, number]
                data.append(datum)
            elif alone and not in_code:
                datum = [section, alone.group(1), offset, offset, number]
                unnamed.append(datum)
            elif op and datum is not None and op.group(1).startswith(('.DATAB', '.RES')):
                # a .RES after the datum's values is SHC's padding before the
                # next datum
                if not (op.group(1).startswith('.RES') and datum[3] > datum[2]):
                    where = f'{path}:{number}'
                    datum[3] = max(datum[3], offset + unit(op.group(1), where) * directive_count(op.group(2), where))
            elif op and datum is not None:
                datum[3] = max(datum[3], offset + len(code) // 2)
                in_values = True
            else:
                # code, alignment, or a label of no datum's: the datum has ended
                datum = None
    return data, unnamed, sizes, loads


def strings(path):
    """Each string SHC's listing at path holds, as its section, offset and
    bytes. A string is a row of an L label alone outside the code, and the
    characters the .SDATA, .DATA.B and .DATAB.B rows below it give, each
    from where the one before it ends. A label whose rows give no
    characters is an unnamed datum's, as an automatic array's template is,
    and gives no string. The listing is read as listing() reads it. asmsh's
    listing gives no strings.

    Fails, naming the row's path and line, where a row gives a string's
    bytes in a form SHC doesn't write, or from an offset other than where
    the string's bytes so far end, or where a string runs past its
    section."""
    found, sizes, section, directive, string = [], {}, None, None, None

    def close():
        if directive is None:
            return
        offset, op, operands, where = directive
        if offset != string[1] + len(string[2]):
            fail(f'{where}: {op} starts at {offset:08X}, and its string\'s bytes end at '
                 f'{string[1] + len(string[2]):08X}')
        if op == '.SDATA':
            data = sdata(operands, where)
        elif op == '.DATA.B':
            data = bytes(byte(value, where) for value in operands.split(',') if value.strip())
        else:
            if operands.count(',') != 1:
                fail(f'{where}: .DATAB.B {operands.strip()}: not a count and a byte')
            data = bytes([byte(operands.split(',')[1], where)]) * directive_count(operands, where)
        string[2].extend(data)

    for number, line in enumerate(content(path).decode('latin-1').replace('\r\n', '\n').split('\n'), 1):
        info = SHC_SECTION.match(line)
        row = SHC_ROW.match(line)
        more = SHC_MORE.match(line)
        marker = SHC_MARKER.match(line)
        if info:
            sizes[info.group(1)] = int(info.group(2), 16)
        elif more and directive is not None:
            directive = (directive[0], directive[1], directive[2] + ',' + more.group(1), directive[3])
        elif marker:
            close()
            directive, string = None, None
            section = marker.group(1)
        elif row:
            close()
            directive = None
            section = row.group(1).strip() or section
            offset = int(row.group(2), 16)
            op = DIRECTIVE.match(row.group(3))
            in_code = section is not None and section.startswith('P')
            if UNNAMED.fullmatch(row.group(3)) and not in_code:
                string = [section, offset, bytearray(), number]
                found.append(string)
            elif op and op.group(1) in ('.SDATA', '.DATA.B', '.DATAB.B') and string is not None:
                directive = (offset, *op.groups(), f'{path}:{number}')
            else:
                string = None
    close()
    placed = []
    for short, offset, data, number in found:
        if not data:
            continue
        section = full_section(f'{path}:{number}', short, sizes)
        if offset + len(data) > sizes[section]:
            fail(f'{path}:{number}: its string runs past its section, {section}, of {sizes[section]} bytes')
        placed.append((section, offset, bytes(data)))
    return placed
