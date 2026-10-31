"""Set four functions of SDK 1.00J's camera motion in R10's text of 1.20J's,
as the object at 0x8c6537e8 in 1ST_READ.BIN holds them.

That object is njCameraMotion_, the SDK member the graft rebuilds. It holds
DONOR's apply_channels, roll_channel, njCameraAction and njCameraMotion as
apply_old_channels, old_roll_channel, njCameraActionOld and
njCameraMotionOld, after NEW's njCameraMotion and before the pool that ends
NEW's code. OUT gets NEW with the four there, each line as DONOR has it but
for the symbols it names:

- a label the four define: an exported function's, its new name, which OUT
  exports; each other, the next number past NEW's labels;
- a function or static of DONOR's that SHARED or STATE names: NEW's of
  that name;
- a symbol IMPORTS names: itself, which NEW must import too;
- a load of an item of one of DONOR's pools: an item appended to NEW's
  last pool, one for each width and expression, the words first, then the
  longs, each in the order of its first load.

OUT is text for use-assembler-pools, which the grafted source's PATCHES
must include, and which has asmsh lay out every literal anew. Its loads name
each item by its offset from its pool's label, and a long can lie off 4: the
four move NEW's last pool by their size, and the appended longs follow the
words.

DONOR's pools and its other functions stay behind. Each text must hold
FUNCTIONS, in that order. A symbol the walk can't bind fails, as do a name
SHARED, STATE or IMPORTS gives that the four don't use, two statics of one
text with a name STATE gives, and a pool among the four's statements. So do
a load of part of an item and a MOVA of one.

NEW's last function and each of the four must end with a delayed return or
unconditional branch and an instruction for its slot. That checks how each
ends, not every path through it. Every check runs before OUT is written.
"""

import argparse
import itertools
import re
import sys

from tool import content, fail, save

# the functions of each text, in order
FUNCTIONS = ('prepare_camera', 'apply_channels', 'find_camera_key', 'find_camera_angle_key', 'linear_camera_angle',
             'linear_camera_vector', 'cubic_camera_vector', 'position_channel', 'direction_channel',
             'interest_channel', 'roll_channel', 'angle_channel', 'njCameraAction', 'njCameraMotion')
# DONOR's functions the member keeps, in its order, and its names for them
KEPT = (('apply_channels', 'apply_old_channels'), ('roll_channel', 'old_roll_channel'),
        ('njCameraAction', 'njCameraActionOld'), ('njCameraMotion', 'njCameraMotionOld'))
# the functions and statics of DONOR's that the four use, each bound to NEW's of the same name
SHARED = ('angle_channel', 'direction_channel', 'find_camera_angle_key', 'interest_channel', 'linear_camera_angle',
          'position_channel', 'prepare_camera')
STATE = ('factor_count', 'motion_type', 'playback', 'working_camera')
# the symbols the four import, which NEW imports too
IMPORTS = ('_njRollCameraInterest', '_njSetCamera')

FUNCTION = re.compile(r'([\w$]+): +; function: (\w+) *$')
STATIC = re.compile(r'([\w$]+): +; static: (\w+) *$')
LABEL = re.compile(r'([A-Za-z_$][\w$]*):')
LOCAL = re.compile(r'L(\d+)')
POOL = re.compile(r'(L\d+): *$')
ITEM = re.compile(r' +\.(DATA\.W|DATA\.L|RES\.W) +(\S+) *$')
SECTION = re.compile(r' +\.SECTION +\w+,(\w+)')
LISTED = re.compile(r' +\.(IMPORT|EXPORT) +(\S+) *$')
# a name in an operand or a comment, but for the radix of a constant such as H'0080
SYMBOL = re.compile(r"(?<![\w$'])([A-Za-z_$][\w$]*)(?![\w$'])")
REGISTER = re.compile(r'(?:R\d+(?:_BANK)?|FR\d+|DR\d+|XD\d+|FV\d+|XMTRX|PR|MACH|MACL|GBR|VBR|SR|SSR|SPC|SGR|DBR|FPUL'
                      r'|FPSCR)', re.I)
LOAD = re.compile(r'( +)(MOV\.[WL])( +)(L\d+)(?:\+(\d+))?(,R\d+) *$')
NUMBER = re.compile(r"(?:[HBDOQ]'[0-9A-F]+|\d+)", re.I)
SIZE = {'DATA.W': 2, 'DATA.L': 4, 'RES.W': 2}
# delayed returns and unconditional branches: the next instruction is their slot
ENDS = ('RTS', 'RTE', 'BRA', 'BRAF', 'JMP')
# SHC's columns: a comment starts no nearer the line's start than the label field's end
COLUMN = 33


def code(line):
    return line.partition(';')[0]


class Text:
    """An R10 text: its lines and line ending, its code section's span, its
    functions as (name, label, first line, end of statements, pools), its
    pools as (label, first line, end, items), its statics by name, the labels
    it defines, what it imports, and its .EXPORT lines.

    Line indexes count from 0, and each end is exclusive. A function's pools
    are every pool between its header and the next function or the code
    section's end, not just those after its statements."""

    def __init__(self, path):
        try:
            text = content(path).decode('ascii')
        except UnicodeDecodeError:
            fail(f'{path}: not R10 assembly text: a byte past ASCII')
        self.path = path
        self.eol = '\r\n' if '\r\n' in text else '\n'
        self.lines = text.split(self.eol)
        if self.lines[-1] == '':
            self.lines.pop()
        sections = [(i, match[1]) for i, line in enumerate(self.lines) if (match := SECTION.match(line))]
        codes = [i for i, kind in sections if kind == 'CODE']
        if len(codes) != 1:
            fail(f'{path}: {len(codes)} code sections, not 1')
        self.start = codes[0] + 1
        self.end = next((i for i, _ in sections if i > codes[0]), len(self.lines))
        self.labels = {match[1] for line in self.lines if (match := LABEL.match(line))}
        statics = [(i + 1, match[2], match[1]) for i, line in enumerate(self.lines) if (match := STATIC.match(line))]
        for name in STATE:
            numbers = [number for number, static, _ in statics if static == name]
            if len(numbers) > 1:
                fail(f'{path}:{numbers[1]}: a second static {name}; line {numbers[0]} is the first')
        self.statics = {name: label for _, name, label in statics}
        listed = [(i, match[1], match[2]) for i, line in enumerate(self.lines) if (match := LISTED.match(line))]
        self.imports = {name for _, kind, name in listed if kind == 'IMPORT'}
        self.exports = [i for i, kind, _ in listed if kind == 'EXPORT']
        self.pools = []
        i = self.start
        while i < self.end:
            pool = self.pool(i)
            if pool:
                self.pools.append(pool)
            i = pool[2] if pool else i + 1
        heads = [(i, match[1], match[2]) for i in range(self.start, self.end) if (match := FUNCTION.match(self.lines[i]))]
        self.functions = []
        for n, (head, label, name) in enumerate(heads):
            stop = heads[n + 1][0] if n + 1 < len(heads) else self.end
            pools = [pool for pool in self.pools if head < pool[1] < stop]
            last = max(i for i in range(head, stop) if code(self.lines[i]).strip() and not self.inside(i, pools))
            self.functions.append((name, label, head, last + 1, pools))
        names = tuple(function[0] for function in self.functions)
        if names != FUNCTIONS:
            fail(f'{path}: functions {", ".join(names)}, not njCameraMotion_\'s {", ".join(FUNCTIONS)}')

    def pool(self, i):
        """The pool whose label is line i, or None. A .RES.W of more than one word ends it."""
        if not POOL.match(self.lines[i]) or i + 1 >= self.end or not ITEM.match(self.lines[i + 1]):
            return None
        items, offset, end = [], 0, i + 1
        while end < self.end and (match := ITEM.match(self.lines[end])) and (match[1] != 'RES.W' or match[2] == '1'):
            items.append((offset, match[1], match[2]))
            offset += SIZE[match[1]]
            end += 1
        return POOL.match(self.lines[i])[1], i, end, items

    @staticmethod
    def inside(i, pools):
        return any(start <= i < end for _, start, end, _ in pools)

    def function(self, name):
        return next(function for function in self.functions if function[0] == name)

    def ends_in_slot(self, function):
        """Whether the function ends with a delayed return or unconditional branch and an instruction for its
        slot. This checks how it ends, not every path through it."""
        _, _, head, end, _ = function
        last = [code(line).split() for line in self.lines[head:end] if code(line).strip()][-2:]
        return (len(last) == 2 and not LABEL.match(self.lines[head:end][-1])
                and last[0][0].upper() in ENDS and not last[1][0].startswith('.'))


def commented(body, column, comment):
    """body with comment at column, or past body's end if it runs there."""
    return body.ljust(max(column, len(body) + 1)) + ';' + comment


def graft(new, donor):
    """NEW's text with DONOR's four set before its last pool."""
    last = new.functions[-1]
    final = last[4][-1] if last[4] else None
    if (final is None or final[2] != new.end
            or any(code(line).strip() for line in new.lines[last[3]:final[1]]) or not new.ends_in_slot(last)):
        fail(f'{new.path}: no pool after {last[0]}\'s last statement, a branch\'s slot, to the end of the code')
    if new.exports and max(new.exports) >= new.start:
        fail(f'{new.path}:{max(new.exports) + 1}: an .EXPORT after the code section starts')
    kept = [donor.function(name) for name, _ in KEPT]
    for name, label, head, end, pools in kept:
        if any(start < end for _, start, _, _ in pools):
            fail(f'{donor.path}: a pool among {name}\'s statements')
        if not donor.ends_in_slot((name, label, head, end, pools)):
            fail(f'{donor.path}: {name} does not end with a delayed return or unconditional branch and its slot')
    exported = {donor.lines[i].split()[1] for i in donor.exports}

    # the labels the four define, past NEW's: their own first, in order, then each inside them
    fresh = itertools.count(max(int(match[1]) for label in new.labels if (match := LOCAL.fullmatch(label))) + 1)
    renamed = {}
    for (name, label, *_), (_, member) in zip(kept, KEPT):
        if label.startswith('_') and label not in exported:
            fail(f'{donor.path}: {name}\'s label {label} is global but not exported')
        renamed[label] = f'_{member}' if label.startswith('_') else f'L{next(fresh)}'
    for _, _, head, end, _ in kept:
        for line in donor.lines[head + 1:end]:
            if (match := LABEL.match(line)) and match[1] not in renamed:
                renamed[match[1]] = f'L{next(fresh)}'
    functions = {label: name for name, label, *_ in donor.functions}
    statics = {label: name for name, label in donor.statics.items()}
    pools = {pool[0]: pool for pool in donor.pools}
    labels = {name: label for name, label, *_ in new.functions}
    used = set()

    def bind(symbol, where):
        """The symbol in OUT for DONOR's symbol, a label, function, static or import."""
        if symbol in renamed:
            return renamed[symbol]
        if symbol in functions and functions[symbol] in SHARED:
            used.add(functions[symbol])
            return labels[functions[symbol]]
        if symbol in statics and statics[symbol] in STATE:
            if statics[symbol] not in new.statics:
                fail(f'{new.path}: no static {statics[symbol]}, which {where} uses')
            used.add(statics[symbol])
            return new.statics[statics[symbol]]
        if symbol in donor.imports and symbol in IMPORTS:
            if symbol not in new.imports:
                fail(f'{new.path}: no .IMPORT of {symbol}, which {where} uses')
            used.add(symbol)
            return symbol
        name = functions.get(symbol) or statics.get(symbol) or symbol
        fail(f'{where}: {name}, which no table here binds')

    def comment_symbol(symbol):
        # a comment's symbol as the operands bind it, where they would
        if symbol in renamed:
            return renamed[symbol]
        if symbol in functions and functions[symbol] in SHARED:
            return labels[functions[symbol]]
        if symbol in statics and statics[symbol] in STATE:
            return new.statics.get(statics[symbol], symbol)
        return symbol

    def rename(text):
        return SYMBOL.sub(lambda match: comment_symbol(match[1]), text)

    # each line of the four, or for a load of a pool's item, its parts until the items are placed
    literals = {}
    copied = []
    for (name, label, head, end, _), (_, member) in zip(kept, KEPT):
        for i in range(head, end):
            line = donor.lines[i]
            where = f'{donor.path}:{i + 1}'
            statement, semicolon, comment = line.partition(';')
            if not statement.strip():
                copied.append(statement + semicolon + rename(comment))
            elif match := LABEL.match(statement):
                if statement[match.end():].strip():
                    fail(f'{where}: a label line the walk can\'t read')
                if i == head:
                    comment = comment.replace(f' function: {name}', f' function: {member}')
                head_text = renamed[match[1]] + ':'
                copied.append(head_text.ljust(COLUMN) + ';' + rename(comment) if semicolon
                              else head_text.ljust(len(statement)))
            else:
                mnemonic, _, operands = statement.strip().partition(' ')
                if mnemonic.startswith('.'):
                    fail(f'{where}: a directive, {mnemonic}, among {name}\'s statements')
                found = [symbol for symbol in SYMBOL.findall(operands) if not REGISTER.fullmatch(symbol)]
                if found and found[0] in pools:
                    load = LOAD.fullmatch(statement)
                    pool = pools[found[0]]
                    offset = int(load[5] or 0) if load else None
                    kind = 'DATA.W' if load and load[2] == 'MOV.W' else 'DATA.L'
                    item = next((item for item in pool[3] if item[0] == offset), None)
                    if load is None or item is None or item[1] != kind:
                        fail(f'{where}: no load of a whole item of {pool[0]}')
                    if (kind, item[2]) not in literals:
                        literals[kind, item[2]] = bind_expression(item[2], bind, where)
                    copied.append((load[1] + load[2] + load[3], (kind, item[2]), load[6], rename(comment) if
                                   semicolon else None, len(statement)))
                else:
                    bound = {symbol: bind(symbol, where) for symbol in found}
                    body = SYMBOL.sub(lambda match: bound.get(match[1], match[1]), statement)
                    copied.append(commented(body.rstrip(), len(statement), rename(comment)) if semicolon else body)
    unused = [name for name in SHARED + STATE + IMPORTS if name not in used]
    if unused:
        fail(f'{donor.path}: the four use no {", ".join(unused)}')

    # the items, placed after the pool's own: the words, then the longs
    offset = sum(SIZE[kind] for _, kind, _ in final[3])
    appended, placed = [], {}
    for width in ('DATA.W', 'DATA.L'):
        for (kind, expression), bound in literals.items():
            if kind == width:
                placed[kind, expression] = f'{final[0]}+{offset}'
                appended.append((kind, bound))
                offset += SIZE[kind]
    for i, line in enumerate(copied):
        if isinstance(line, tuple):
            head_text, literal, register, comment, column = line
            body = head_text + placed[literal] + register
            copied[i] = body if comment is None else commented(body, column, comment)

    exports = [f'          .EXPORT     {renamed[label]}' for _, label, *_ in kept if label.startswith('_')]
    at = max(new.exports) + 1 if new.exports else new.start - 1
    items = [f'          {"." + kind:<12}{value}' for kind, value in appended]
    lines = [*new.lines[:at], *exports, *new.lines[at:final[1]], *copied, *new.lines[final[1]:final[2]], *items,
             *new.lines[final[2]:]]
    return new.eol.join(lines) + new.eol


def bind_expression(expression, bind, where):
    """A pool item's expression, a constant or a symbol plus constants, with its symbol bound."""
    terms = re.findall(r'([+-]?)([^+-]+)', expression)
    symbols = [(sign, term) for sign, term in terms if not NUMBER.fullmatch(term)]
    if ''.join(sign + term for sign, term in terms) != expression or len(symbols) > 1 or (
            symbols and (symbols[0][0] == '-' or not SYMBOL.fullmatch(symbols[0][1]))):
        fail(f'{where}: loads {expression}, which is no constant and no symbol plus constants')
    return SYMBOL.sub(lambda match: bind(match[1], where), expression)


def main(argv):
    parser = argparse.ArgumentParser(prog='graft', description=__doc__.split('\n\n')[0])
    parser.add_argument('new', metavar='NEW', help="R10's -code=asmcode text of sdk/ninja/njCameraMotion.c")
    parser.add_argument('donor', metavar='DONOR', help="R10's text of sdk/ninja/njCameraMotionOld.c, compiled alike")
    parser.add_argument('out', metavar='OUT', help="where to write NEW with DONOR's four functions set in it")
    args = parser.parse_args(argv)
    text = graft(Text(args.new), Text(args.donor))
    save(args.out, text.encode('ascii'))


if __name__ == '__main__':
    main(sys.argv[1:])
