"""Translate a C source's SH-4 inline bodies to C, for make pc.

Usage: sh2c.py SOURCE [--call SYMBOL=TEMPLATE]... [--dynamic TEMPLATE]
               [--declare TEXT]... [--only NAME]... [--global Rn=NAME]...
               [--skip-defined FILE] [--literal VALUE=EXPRESSION]...

SOURCE is a C source of the tree with #pragma inline_asm bodies, as
sdk/ninja/njLtSrcs.c. The tool prints a C definition of each body, or of
each --only names, with its source's head, that runs the body's
instructions in order, each as C over the registers it models:

- R0 to R15 as Uint32s, the FPU's two banks of 16 Floats, which FRCHG
  swaps, T, FPUL, MACL and MACH, and XMTRX, the port's (xbox/include/sh4.h), which
  the C reads at entry where the body uses the back bank and writes back
  before each call and at each return;
- R15 as a stack of its own, which the body's pushes and pops reach;
- the arguments in R4 to R7 and FR4 to FR11, as SHC passes them, and the
  return value in R0 or FR0;
- literal loads, MOV.L #sym and #imm, and MOVA and PC-relative loads of the
  body's own data, which the C holds as static arrays, each by its label;
- branches as gotos, each delayed branch reading its condition or target
  before its slot runs, and BRAF through a MOVA table of label offsets,
  .DATA.W ?L-?B, as a switch;
- each JSR and BSR as a C call: a known symbol by --call's TEMPLATE, and
  any other by --dynamic's. A TEMPLATE names registers as R4 and FR4, and
  CALLEE, in --dynamic's, the register JSR jumps through. A call keeps
  every register the C models, as the C locals are, where the callee could
  change those SHC's convention lets it.

--skip-defined leaves out each body that FILE, the source's split C,
defines already, as a function or a macro: the GCC side's own spelling.
--literal gives a literal pool's constant VALUE, as SHC writes it (H'FF000038),
as a C EXPRESSION, for an address the port moves: Ninja's model drawers load
the store queues' QACR0 at 0xff000038, which the Xbox's flash ROM covers, so
make pc gives them port_qacr's address.

--global maps a register that the source keeps a C global in, as
njModel.c's R8, onto that global: the body reads it at entry and writes it
back at its end. FSCHG's moves of pairs follow it in text order, which the bodies keep
within straight runs. --declare adds TEXT ahead of the definitions, for a
name the body takes that no header declares. The tool fails on an
instruction it doesn't model, naming the body and the line.
"""

import argparse
import re
import struct
import sys

PRAGMA = re.compile(r'^#pragma inline_asm\((\w+)')
LABEL = re.compile(r'^(\??\w+):\s*(.*)$')
INSTRUCTION = re.compile(r'^\s+([A-Z][A-Z0-9./]*)\s*(.*?)\s*(?:;.*)?$')
DELAYED = {'BT/S', 'BF/S', 'BRA', 'BSR', 'BRAF', 'BSRF', 'JMP', 'JSR', 'RTS'}


def number(text):
    """an immediate's value: H'.., decimal, or 0x.."""
    text = text.strip()
    negative = text.startswith('-')
    text = text.lstrip('-')
    if text.upper().startswith("H'"):
        value = int(text[2:], 16)
    else:
        value = int(text, 0)
    return -value if negative else value


def float_bits(text):
    """an .FDATA.S value's bits, the float nearest it"""
    text = text.strip().upper()
    if text.startswith("F'"):
        text = text[2:]
    text = text.replace('S', 'E')
    return struct.unpack('<I', struct.pack('<f', float(text)))[0]


def c_name(symbol):
    """a C name from an assembly symbol, which SHC prefixes with _"""
    return symbol[1:] if symbol.startswith('_') else symbol


class Body:
    """one inline body: its head, lines, labels and data"""

    def __init__(self, name, head, lines):
        self.name, self.head, self.lines = name, head, lines
        self.labels = []  # labels in order
        self.data = {}  # label -> list of (kind, value)
        self.parse()

    def parse(self):
        """sort the lines into code and the data the labels hold"""
        code, current, fresh = [], None, False
        for raw in self.lines:
            line = raw.split(';')[0].rstrip() if not raw.lstrip().startswith(';') else ''
            if not line.strip():
                continue
            match = LABEL.match(line)
            if match:
                current = match[1]
                fresh = True
                code.append(('label', current))
                line = match[2]
                if not line.strip():
                    continue
                line = '        ' + line
            match = INSTRUCTION.match(line) or re.match(r'^\s+(\.[A-Z.]+)\s*(.*)$', line, re.I)
            if not match:
                raise SystemExit(f'sh2c.py: {self.name}: no instruction in {raw!r}')
            op, args = match[1].upper(), match[2].strip()
            if op.startswith('.'):
                if op in ('.IMPORT', '.EXPORT', '.ALIGN', '.SECTION'):
                    continue
                values = [x.strip() for x in args.split(',')]
                kind = {'.DATA.L': 'L', '.DATA.W': 'W', '.DATA.B': 'B', '.FDATA.S': 'F', '.RES.W': 'RW',
                        '.RES.L': 'RL', '.RES.B': 'RB'}.get(op)
                if kind is None:
                    raise SystemExit(f'sh2c.py: {self.name}: no model of {raw!r}')
                if not fresh:
                    # padding between the code and the next label's data
                    continue
                self.data.setdefault(current, []).extend((kind, v) for v in values)
                continue
            fresh = False
            code.append((op, args))
        self.code = code


def bodies(path):
    """the source's inline bodies"""
    text = open(path, encoding='utf-8').read().splitlines()
    found = []
    for i, line in enumerate(text):
        match = PRAGMA.match(line)
        if not match:
            continue
        name = match[1]
        j = i + 1
        while not re.search(rf'\b{name}\s*\(', text[j]):
            j += 1
        head = []
        while text[j].strip() != '{':
            head.append(text[j])
            j += 1
        k = j + 1
        while text[k] != '}':
            k += 1
        found.append(Body(name, ' '.join(h.strip() for h in head), text[j + 1:k]))
    return found


# the names a body's C declares for its registers and state
RESERVED = {'r', 'bank', 'front', 'fr', 't', 'cond', 'fpul', 'macl', 'mach', 'target', 'stack'}


def safe_head(head):
    """head with each parameter a register's name would hide renamed, as
    pack_argb_body's r"""
    def rename(match):
        return match[1] + match[2] + '_arg' + match[3]
    open_at = head.index('(')
    params = re.sub(r'(\W|^)(' + '|'.join(sorted(RESERVED)) + r')(\s*(?:,|\)|$))', rename, head[open_at:])
    return head[:open_at] + params


def parameters(head):
    """the head's return type, name and (type, name) parameters"""
    match = re.match(r'^(.*?)\b(\w+)\s*\((.*)\)\s*$', head)
    if not match:
        raise SystemExit(f'sh2c.py: no head in {head!r}')
    ret, name, params = match[1].strip(), match[2], match[3].strip()
    found = []
    if params and params != 'void':
        for p in params.split(','):
            p = p.strip()
            pm = re.match(r'^(.*?)(\w+)$', p)
            found.append((pm[1].strip(), pm[2]))
    return ret, name, found


class Translate:
    """one body's C"""

    def __init__(self, body, calls, dynamic, globals_=None, literals=None):
        self.body, self.calls, self.dynamic = body, calls, dynamic
        self.globals = globals_ or {}
        self.literals = literals or {}
        self.out = []
        self.sz = False
        self.uses_back = any(op in ('FRCHG', 'FTRV') or 'XD' in args for op, args in body.code)
        self.labels = {arg for op, arg in body.code if op == 'label'}
        self.temps = 0

    def emit(self, text):
        self.out.append('    ' + text)

    def reg(self, text):
        """a general register operand as C"""
        m = re.fullmatch(r'R(\d+)', text.strip(), re.I)
        if not m:
            raise SystemExit(f'sh2c.py: {self.body.name}: no register in {text!r}')
        return f'r[{int(m[1])}]'

    def fr(self, text):
        m = re.fullmatch(r'FR(\d+)', text.strip(), re.I)
        if not m:
            raise SystemExit(f'sh2c.py: {self.body.name}: no FPU register in {text!r}')
        return f'fr[{int(m[1])}]'

    def label(self, name):
        return 'L_' + name.lstrip('?_')

    def data_name(self, label):
        return f'data_{self.body.name}_{self.label(label)[2:]}'

    def data_address(self, label):
        return f'(Uint32){self.data_name(label)}'

    def literal(self, text):
        """a #literal's value as C: a number, a symbol's address, or a data
        label's"""
        text = text.strip()
        if not text.startswith('#'):
            raise SystemExit(f'sh2c.py: {self.body.name}: no literal in {text!r}')
        text = text[1:]
        m = re.fullmatch(r'(_\w+)([+-].+)?', text)
        if m:
            offset = number(m[2]) if m[2] else 0
            return f'((Uint32)&{c_name(m[1])} + {offset})'
        if text.startswith('?'):
            return self.data_address(text)
        if number(text) & 0xffffffff in self.literals:
            return f'(Uint32)({self.literals[number(text) & 0xffffffff]})'
        return f'(Uint32)0x{number(text) & 0xffffffff:x}u'

    def address(self, operand, size):
        """an @ operand's address as C, and the C that moves its register
        after, or before for @-Rn"""
        operand = operand.strip()
        if m := re.fullmatch(r'@\(R0,(R\d+)\)', operand, re.I):
            return f'(r[0] + {self.reg(m[1])})', None, None
        if m := re.fullmatch(r'@\((\w+),(R\d+)\)', operand, re.I):
            return f'({self.reg(m[2])} + {number(m[1])})', None, None
        if m := re.fullmatch(r'@-(R\d+)', operand, re.I):
            r = self.reg(m[1])
            return r, f'{r} -= {size};', None
        if m := re.fullmatch(r'@(R\d+)\+', operand, re.I):
            r = self.reg(m[1])
            return r, None, f'{r} += {size};'
        if m := re.fullmatch(r'@(R\d+)', operand, re.I):
            return self.reg(m[1]), None, None
        raise SystemExit(f'sh2c.py: {self.body.name}: no address in {operand!r}')

    def access(self, size, signed=True):
        return {4: 'MEM32', 2: 'MEMS16' if signed else 'MEM16', 1: 'MEMS8' if signed else 'MEM8'}[size]

    def mov(self, size, args):
        """MOV.L, MOV.W and MOV.B"""
        src, dst = self.split(args)
        if src.startswith('#'):
            value = self.literal(src)
            if size == 2 and not src[1:].startswith('_') and not src[1:].startswith('?'):
                value = f'(Uint32)(Sint32)(Sint16){value}'
            self.emit(f'{self.reg(dst)} = {value};')
        elif src.startswith('?') or src.startswith('_'):
            self.emit(f'{self.reg(dst)} = (Uint32){self.access(size)}({self.data_address(src)});')
        elif src.startswith('@'):
            addr, before, after = self.address(src, size)
            if before:
                self.emit(before)
            self.emit(f'{self.reg(dst)} = (Uint32){self.access(size)}({addr});')
            if after and not (src.endswith('+') and re.fullmatch(r'@(R\d+)\+', src).group(1) == dst):
                self.emit(after)
        else:
            addr, before, after = self.address(dst, size)
            value = self.reg(src)
            if before:
                self.emit(f'{{ Uint32 v = {value}; {before} {self.access(size, False)}({addr}) = '
                          f'({"Uint32" if size == 4 else "Uint16" if size == 2 else "Uint8"})v; }}')
                return
            self.emit(f'{self.access(size, False)}({addr}) = '
                      f'({"Uint32" if size == 4 else "Uint16" if size == 2 else "Uint8"}){value};')
            if after:
                self.emit(after)

    def split(self, args):
        return [x.strip() for x in re.split(r',(?![^(]*\))', args)]

    def fmov(self, args):
        """FMOV and FMOV.S, by SZ"""
        src, dst = self.split(args)
        if self.sz:
            pair = lambda name: self.pair(name)
            if src.startswith('@'):
                addr, before, after = self.address(src, 8)
                a, b = pair(dst)
                if before:
                    self.emit(before)
                self.emit(f'{a} = MEMF({addr}); {b} = MEMF({addr} + 4);')
                if after:
                    self.emit(after)
            elif dst.startswith('@'):
                addr, before, after = self.address(dst, 8)
                a, b = pair(src)
                if before:
                    self.emit(before)
                self.emit(f'MEMF({addr}) = {a}; MEMF({addr} + 4) = {b};')
                if after:
                    self.emit(after)
            else:
                (a, b), (c, d) = pair(src), pair(dst)
                self.emit(f'{{ Float x = {a}, y = {b}; {c} = x; {d} = y; }}')
            return
        if src.startswith('@'):
            addr, before, after = self.address(src, 4)
            if before:
                self.emit(before)
            self.emit(f'{self.fr(dst)} = MEMF({addr});')
            if after:
                self.emit(after)
        elif dst.startswith('@'):
            addr, before, after = self.address(dst, 4)
            if before:
                self.emit(before)
            self.emit(f'MEMF({addr}) = {self.fr(src)};')
            if after:
                self.emit(after)
        else:
            self.emit(f'{self.fr(dst)} = {self.fr(src)};')

    def pair(self, name):
        """a register pair under SZ: DRn, XDn, or FRn as its encoding names"""
        m = re.fullmatch(r'(FR|DR|XD)(\d+)', name.strip(), re.I)
        kind, n = m[1].upper(), int(m[2])
        if kind == 'FR':
            kind, n = ('XD', n - 1) if n % 2 else ('DR', n)
        bank = 'fr' if kind == 'DR' else 'bank[1 - front]'
        return f'{bank}[{n}]', f'{bank}[{n + 1}]'

    def call(self, target, symbol):
        """a JSR or BSR to symbol, or through target's register"""
        template = self.calls.get(symbol) if symbol else None
        if template is None:
            if self.dynamic is None:
                raise SystemExit(f'sh2c.py: {self.body.name}: a call of {symbol or target} with no template')
            template = self.dynamic.replace('CALLEE', target)
        if self.uses_back:
            self.emit('sh4_xmtrx_set(bank[1 - front]);')
        self.emit(re.sub(r'\bFR(\d+)\b', r'fr[\1]', re.sub(r'\bR(\d+)\b', r'r[\1]', template)) + ';')
        if self.uses_back:
            self.emit('sh4_xmtrx_get(bank[1 - front]);')

    def known(self):
        """each general register's constant symbol, as MOV.L #_sym,Rn last
        set it, along the text"""
        return {}

    def step(self, op, args, constants):
        """one instruction's C"""
        a = self.split(args) if args else []
        f2 = lambda: (self.fr(a[0]), self.fr(a[1]))
        r2 = lambda: (self.reg(a[0]), self.reg(a[1]))
        if op == 'NOP':
            return
        if op in ('PREF', 'OCBI', 'OCBWB', 'OCBP'):
            # the caches and the store queues: on x86 the queues are RAM's
            # view, which plain stores reach, and the caches keep coherent
            return
        if op == 'MOVCA.L':
            self.mov(4, args)
            return
        if op == 'MOV' and a[0].startswith('#'):
            self.emit(f'{self.reg(a[1])} = (Uint32)(Sint32)(Sint8){number(a[0][1:])};')
        elif op == 'MOV':
            s, d = r2()
            self.emit(f'{d} = {s};')
        elif op in ('MOV.L', 'MOV.W', 'MOV.B'):
            self.mov({'MOV.L': 4, 'MOV.W': 2, 'MOV.B': 1}[op], args)
        elif op == 'MOVA' and a[0] == getattr(self, 'braf_label', None):
            # a jump table's base: the switch at its BRAF reads no address
            self.emit('r[0] = 0;')
        elif op == 'MOVA' and a[0].startswith('#'):
            # SHC's MOVA #imm: the constant goes in the pool, and R0 takes
            # its address
            self.temps += 1
            self.emit(f'static Uint32 pool_{self.temps} = 0x{number(a[0][1:]) & 0xffffffff:x}u;')
            self.emit(f'r[0] = (Uint32)&pool_{self.temps};')
        elif op == 'MOVA':
            self.emit(f'r[0] = {self.data_address(a[0])};')
        elif op == 'ADD' and a[0].startswith('#'):
            self.emit(f'{self.reg(a[1])} += (Uint32)(Sint32)(Sint8){number(a[0][1:])};')
        elif op in ('ADD', 'SUB', 'AND', 'OR', 'XOR') and not a[0].startswith('#'):
            s, d = r2()
            self.emit(f'{d} {dict(ADD="+", SUB="-", AND="&", OR="|", XOR="^")[op]}= {s};')
        elif op in ('AND', 'OR', 'XOR') and a[0].startswith('#'):
            self.emit(f'r[0] {dict(AND="&", OR="|", XOR="^")[op]}= (Uint32){number(a[0][1:]) & 0xff};')
        elif op == 'NEG':
            s, d = r2()
            self.emit(f'{d} = 0u - {s};')
        elif op == 'TST' and a[0].startswith('#'):
            self.emit(f't = (r[0] & {number(a[0][1:]) & 0xff}u) == 0;')
        elif op == 'TST':
            s, d = r2()
            self.emit(f't = ({s} & {d}) == 0;')
        elif op == 'CMP/EQ' and a[0].startswith('#'):
            self.emit(f't = r[0] == (Uint32)(Sint32)(Sint8){number(a[0][1:])};')
        elif op in ('CMP/EQ', 'CMP/GE', 'CMP/GT', 'CMP/HS', 'CMP/HI'):
            s, d = r2()
            c = {'CMP/EQ': f'{d} == {s}', 'CMP/GE': f'(Sint32){d} >= (Sint32){s}',
                 'CMP/GT': f'(Sint32){d} > (Sint32){s}', 'CMP/HS': f'{d} >= {s}', 'CMP/HI': f'{d} > {s}'}[op]
            self.emit(f't = {c};')
        elif op in ('CMP/PZ', 'CMP/PL'):
            r = self.reg(a[0])
            self.emit(f't = (Sint32){r} {">=" if op == "CMP/PZ" else ">"} 0;')
        elif op == 'DT':
            r = self.reg(a[0])
            self.emit(f'{r} -= 1; t = {r} == 0;')
        elif op in ('SHLL', 'SHAL'):
            r = self.reg(a[0])
            self.emit(f't = {r} >> 31; {r} <<= 1;')
        elif op == 'SHLR':
            r = self.reg(a[0])
            self.emit(f't = {r} & 1; {r} >>= 1;')
        elif op == 'SHAR':
            r = self.reg(a[0])
            self.emit(f't = {r} & 1; {r} = (Uint32)((Sint32){r} >> 1);')
        elif m := re.fullmatch(r'SHL([LR])(2|8|16)', op):
            r = self.reg(a[0])
            self.emit(f'{r} {"<<" if m[1] == "L" else ">>"}= {m[2]};')
        elif op == 'ROTCL':
            r = self.reg(a[0])
            self.emit(f'{{ Uint32 c = {r} >> 31; {r} = {r} << 1 | (Uint32)t; t = (int)c; }}')
        elif op == 'ROTL':
            r = self.reg(a[0])
            self.emit(f't = {r} >> 31; {r} = {r} << 1 | (Uint32)t;')
        elif op in ('EXTU.B', 'EXTU.W', 'EXTS.B', 'EXTS.W'):
            s, d = r2()
            cast = {'EXTU.B': 'Uint8', 'EXTU.W': 'Uint16', 'EXTS.B': 'Sint8', 'EXTS.W': 'Sint16'}[op]
            self.emit(f'{d} = (Uint32)(Sint32)({cast}){s};')
        elif op == 'SWAP.W':
            s, d = r2()
            self.emit(f'{d} = {s} << 16 | {s} >> 16;')
        elif op == 'SWAP.B':
            s, d = r2()
            self.emit(f'{d} = ({s} & 0xffff0000u) | ({s} & 0xff) << 8 | ({s} >> 8 & 0xff);')
        elif op == 'MULS.W':
            s, d = r2()
            self.emit(f'macl = (Uint32)((Sint32)(Sint16){d} * (Sint32)(Sint16){s});')
        elif op == 'MOVT':
            self.emit(f'{self.reg(a[0])} = (Uint32)t;')
        elif op == 'STS' and a[0].upper() in ('MACL', 'MACH', 'FPUL'):
            self.emit(f'{self.reg(a[1])} = {a[0].lower()};')
        elif op == 'LDS' and a[1].upper() in ('MACL', 'MACH', 'FPUL'):
            self.emit(f'{a[1].lower()} = {self.reg(a[0])};')
        elif op == 'STS.L' and a[0].upper() == 'PR':
            self.emit('r[15] -= 4; MEM32(r[15]) = 0;')
        elif op == 'LDS.L' and a[1].upper() == 'PR':
            self.emit('r[15] += 4;')
        elif op in ('FMOV', 'FMOV.S'):
            self.fmov(args)
        elif op in ('FLDI0', 'FLDI1'):
            self.emit(f'{self.fr(a[0])} = {"0.0F" if op == "FLDI0" else "1.0F"};')
        elif op in ('FADD', 'FSUB', 'FMUL', 'FDIV'):
            s, d = f2()
            self.emit(f'{d} = {d} {dict(FADD="+", FSUB="-", FMUL="*", FDIV="/")[op]} {s};')
        elif op == 'FMAC':
            _, s, d = (self.fr(x) for x in a)
            self.emit(f'{d} = sh4_fmac(fr[0], {s}, {d});')
        elif op == 'FNEG':
            r = self.fr(a[0])
            self.emit(f'{r} = -{r};')
        elif op == 'FABS':
            r = self.fr(a[0])
            self.emit(f'{r} = __builtin_fabsf({r});')
        elif op == 'FSQRT':
            r = self.fr(a[0])
            self.emit(f'{r} = __builtin_sqrtf({r});')
        elif op == 'FSRRA':
            r = self.fr(a[0])
            self.emit(f'{r} = _builtin_fsrra({r});')
        elif op == 'FCMP/GT':
            s, d = f2()
            self.emit(f't = {d} > {s};')
        elif op == 'FCMP/EQ':
            s, d = f2()
            self.emit(f't = {d} == {s};')
        elif op == 'FIPR':
            m, n = (int(re.fullmatch(r'FV(\d+)', x, re.I)[1]) for x in a)
            terms = ', '.join(f'fr[{m + k}]' for k in range(4)) + ', ' + ', '.join(f'fr[{n + k}]' for k in range(4))
            self.emit(f'fr[{n + 3}] = sh4_fipr({terms});')
        elif op == 'FTRV':
            n = int(re.fullmatch(r'FV(\d+)', a[1], re.I)[1])
            self.emit(f'sh4_ftrv(bank[1 - front], &fr[{n}], &fr[{n}]);')
        elif op == 'FSCA':
            n = int(re.fullmatch(r'DR(\d+)', a[1], re.I)[1])
            self.emit(f'_builtin_fsca((Angle)fpul, &fr[{n}], &fr[{n + 1}]);')
        elif op == 'FLOAT':
            self.emit(f'{self.fr(a[1])} = (Float)(Sint32)fpul;')
        elif op == 'FTRC':
            self.emit(f'fpul = (Uint32)sh4_ftrc({self.fr(a[0])});')
        elif op == 'FLDS':
            self.emit(f'{{ union {{ Float f; Uint32 u; }} v; v.f = {self.fr(a[0])}; fpul = v.u; }}')
        elif op == 'FSTS':
            self.emit(f'{{ union {{ Float f; Uint32 u; }} v; v.u = fpul; {self.fr(a[1])} = v.f; }}')
        elif op == 'FRCHG':
            self.emit('front = 1 - front; fr = bank[front];')
        elif op == 'FSCHG':
            self.sz = not self.sz
        elif op in ('BT', 'BF'):
            self.emit(f'if ({"" if op == "BT" else "!"}t) goto {self.label(a[0])};')
        else:
            raise SystemExit(f'sh2c.py: {self.body.name}: {op} {args} is no instruction the tool models')
        # a MOV.L #_sym,Rn, or a load of a pool label that holds one symbol,
        # makes Rn that symbol's for a JSR @Rn
        if op == 'MOV.L' and a and a[0].startswith('#_'):
            constants[a[1].upper()] = a[0][1:].split('+')[0]
        elif (op == 'MOV.L' and a and a[0].startswith('?') and len(self.body.data.get(a[0], [])) == 1
              and re.fullmatch(r'_\w+', self.body.data[a[0]][0][1])):
            constants[a[1].upper()] = self.body.data[a[0]][0][1]
        elif a and len(a) == 2 and re.fullmatch(r'R\d+', a[-1], re.I):
            constants.pop(a[-1].upper(), None)

    def branch(self, op, args, slot, constants):
        """a delayed branch, its slot first"""
        a = self.split(args) if args else []
        if op in ('BT/S', 'BF/S'):
            self.emit(f'cond = {"" if op == "BT/S" else "!"}t;')
            self.step(*slot, constants)
            self.emit(f'if (cond) goto {self.label(a[0])};')
        elif op == 'BRA':
            self.step(*slot, constants)
            self.emit(f'goto {self.label(a[0])};')
        elif op == 'RTS':
            self.step(*slot, constants)
            self.emit('goto done;')
        elif op in ('JSR', 'JMP'):
            target = self.reg(re.fullmatch(r'@(R\d+)', a[0], re.I)[1])
            symbol = constants.get(re.fullmatch(r'@(R\d+)', a[0], re.I)[1].upper())
            self.emit(f'target = {target};')
            self.step(*slot, constants)
            self.call('target', symbol)
            if op == 'JMP':
                self.emit('goto done;')
        elif op == 'BSR':
            self.step(*slot, constants)
            self.call(None, a[0])
        elif op == 'BRAF':
            table = self.braf_table
            if table is None:
                raise SystemExit(f'sh2c.py: {self.body.name}: a BRAF through no table')
            self.emit(f'target = {self.reg(a[0])};')
            self.step(*slot, constants)
            self.emit('switch (target) {')
            for k, (label, _) in enumerate(table):
                self.emit(f'case {k}: goto {self.label(label)};')
            self.emit('}')
            self.emit('goto done;')
        else:
            raise SystemExit(f'sh2c.py: {self.body.name}: no model of {op}')

    def run(self):
        """the body's C, as lines"""
        ret, name, params = parameters(safe_head(self.body.head))
        ints = floats = 0
        self.emit('Uint32 r[16] = {0};')
        self.emit('Float bank[2][16] = {{0}};')
        self.emit('int front = 0;')
        self.emit('Float *fr = bank[0];')
        self.emit('int t = 0;')
        self.emit('int cond = 0;')
        self.emit('Uint32 fpul = 0, macl = 0, mach = 0, target = 0;')
        self.emit('Uint32 stack[256];')
        self.emit('(void)cond; (void)macl; (void)mach; (void)target; (void)fpul; (void)t;')
        self.emit('r[15] = (Uint32)&stack[256];')
        if self.uses_back:
            self.emit('sh4_xmtrx_get(bank[1]);')
        for number_, name_ in self.globals.items():
            self.emit(f'r[{number_}] = (Uint32){name_};')
        for kind, pname in params:
            if kind.replace(' ', '') in ('Float', 'float'):
                self.emit(f'fr[{4 + floats}] = {pname};')
                floats += 1
            else:
                self.emit(f'r[{4 + ints}] = (Uint32){pname};')
                ints += 1
        # a BRAF's table: the data a MOVA ahead of it addresses, if it holds
        # label differences
        self.braf_table = None
        for label, values in self.body.data.items():
            if values and all(k == 'W' and re.fullmatch(r'\?\w+-\?\w+', v) for k, v in values):
                self.braf_table = [tuple(v.split('-')) for _, v in values]
                self.braf_label = label
        code = self.body.code
        constants = {}
        i = 0
        while i < len(code):
            op, args = code[i]
            if op == 'label':
                self.out.append(f'{self.label(args)}:;')
                constants.clear()
                i += 1
                continue
            if op in DELAYED:
                j = i + 1
                while code[j][0] == 'label':
                    raise SystemExit(f'sh2c.py: {self.body.name}: a label in a delay slot')
                if code[j][0] in DELAYED:
                    raise SystemExit(f'sh2c.py: {self.body.name}: a branch in a delay slot')
                if op == 'BRAF':
                    # the table's MOV.W loads the label's offset: as its index
                    pass
                self.branch(op, args, code[j], constants)
                i = j + 1
                continue
            if (self.braf_table and op == 'MOV.W' and '@(R0,' in args.upper()):
                # a load from the BRAF table: its index, from the byte offset
                src, dst = self.split(args)
                index = self.reg(re.fullmatch(r'@\(R0,(R\d+)\)', src, re.I)[1])
                self.emit(f'{self.reg(dst)} = {index} / 2;')
                i += 1
                continue
            self.step(op, args, constants)
            i += 1
        self.out.append('done:;')
        for number_, name_ in self.globals.items():
            self.emit(f'{name_} = (__typeof__({name_}))r[{number_}];')
        if self.uses_back:
            self.emit('sh4_xmtrx_set(bank[1 - front]);')
        if ret.replace('static', '').strip() in ('Float', 'float'):
            self.emit('return fr[0];')
        elif ret.replace('static', '').strip() not in ('void',):
            self.emit(f'return ({ret.replace("static", "").strip()})r[0];')
        return self.out

    def data(self):
        """the body's labelled data as static arrays"""
        out = []
        for label, values in self.body.data.items():
            if label == getattr(self, 'braf_label', None):
                continue
            words = []
            kinds = {kind for kind, _ in values}
            if kinds <= {'W', 'RW'} or kinds <= {'B', 'RB'}:
                element = 'Uint16' if kinds <= {'W', 'RW'} else 'Uint8'
                for kind, value in values:
                    words += ['0'] * number(value) if kind.startswith('R') else [f'({element}){number(value)}']
                out.append(f'static {element} {self.data_name(label)}[] = {{{", ".join(words)}}};')
                continue
            for kind, value in values:
                if kind == 'L':
                    if m := re.fullmatch(r'(_\w+)([+-].+)?', value):
                        offset = f' + {number(m[2])}' if m[2] else ''
                        words.append(f'((Uint32)&{c_name(m[1])}{offset})')
                    else:
                        words.append(f'(Uint32)0x{number(value) & 0xffffffff:x}u')
                elif kind == 'F':
                    words.append(f'0x{float_bits(value):08x}u')
                elif kind in ('RL',):
                    words += ['0'] * number(value)
                else:
                    raise SystemExit(f'sh2c.py: {self.body.name}: no model of {kind} data at {label}')
            out.append(f'static Uint32 {self.data_name(label)}[] = {{{", ".join(words)}}};')
        return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source')
    parser.add_argument('--call', action='append', default=[])
    parser.add_argument('--dynamic')
    parser.add_argument('--declare', action='append', default=[])
    parser.add_argument('--only', action='append', default=[])
    parser.add_argument('--global', dest='globals', action='append', default=[])
    parser.add_argument('--skip-defined')
    parser.add_argument('--literal', dest='literals', action='append', default=[])
    args = parser.parse_args()
    defined = set()
    if args.skip_defined:
        # the names the split C defines already, as a function with a body
        # or a macro: the GCC side's spellings of bodies
        text = open(args.skip_defined).read()
        defined |= set(re.findall(r'^[^\n;#(]*?\b(\w+)\s*\([^;{()]*\)[ \t]*\n\{', text, re.M))
        defined |= set(re.findall(r'^#define\s+(\w+)', text, re.M))
    calls = dict(c.split('=', 1) for c in args.call)
    globals_ = {int(g.split('=', 1)[0].upper().lstrip('R')): g.split('=', 1)[1] for g in args.globals}
    literals = {number(v.split('=', 1)[0]) & 0xffffffff: v.split('=', 1)[1] for v in args.literals}
    print(f'/* written by tools/sh2c.py from {args.source}: edit the bodies, not this */')
    print('#include "xbox/include/sh4.h"')
    print('#define MEM32(a) (*(Uint32 *)(a))')
    print('#define MEM16(a) (*(Uint16 *)(a))')
    print('#define MEMS16(a) (*(Sint16 *)(a))')
    print('#define MEM8(a) (*(Uint8 *)(a))')
    print('#define MEMS8(a) (*(Sint8 *)(a))')
    print('#define MEMF(a) (*(Float *)(a))')
    for text in args.declare:
        print(text)
    for body in bodies(args.source):
        if (args.only and body.name not in args.only) or body.name in defined:
            continue
        t = Translate(body, calls, args.dynamic, globals_, literals)
        lines = t.run()
        for line in t.data():
            print(line)
        print(safe_head(body.head))
        print('{')
        print('\n'.join(lines))
        print('}')
    print('#undef MEM32\n#undef MEM16\n#undef MEMS16\n#undef MEM8\n#undef MEMS8\n#undef MEMF')


if __name__ == '__main__':
    sys.exit(main())
