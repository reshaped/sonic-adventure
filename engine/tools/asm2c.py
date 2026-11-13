#!/usr/bin/env python3
"""SH-4 assembly as host C: asm2c.py OUT.c SOURCE... [--explore]

The game's assembly gives play its math, and the SDK's inline bodies its
matrices: FTRV, FIPR, FSCA and FSRRA in orders C can't name. A hand port would
guess. This writes the same instructions as C statements over a register file,
so a host runs SEGA's own text through the compatible math of play_math.h, and
the matched sources stay as they are.

A SOURCE is a .src file, or a C file whose #pragma inline_asm bodies it takes.
FILE.src:NAME,NAME takes those functions of the file alone, and SKIP names the
functions that stay the shim's. Each label that doesn't start
with ? begins a function, as a BSR or a JSR reaches it. The C the game calls
is a wrapper by the signature above a .src function, or the C declaration of an
inline body: pointers and integers go in R4 to R7, floats in FR4 to FR11, and
the result comes back in R0 or FR0, as SHC's calls do. An address is 32 bits,
so all the code reaches lies in the domain's 4 GB: the slice's image, stack and
heap do, and sh4.h's loads and stores and a pointer the C takes back add its
base.

A delay slot runs before the branch it follows. A conditional branch tests T
before its slot runs, and a JSR, a JMP or a BRAF reads its register before its
slot runs. Data in the text is an array of its own, which PC-relative loads and
MOVA name. A table of label differences that a BRAF reads holds each label's
number here, and the BRAF switches on it.

A function the C reaches that this writes no translation of is the host's: one
SKIP names, one a FILE.src's NAMEs leave out, or one --explore left out. Its
call goes to the host's function of its name, as a JSR to the game's C does.

An instruction this doesn't know ends the build: asm2c.py names each function
that holds one, and each NAME a FILE.src holds no function of, writes no OUT
and fails. Given --explore it writes OUT without those functions and says so,
so a build reaches its next missing name: engine/mk/native.mk's
NATIVE_EXPLORE, under which stubs.py gives each a stub"""
import re
import sys
from pathlib import Path

# the shim's: their pools carry a sign test that lives on P1's addresses, or
# they are drawing's
SKIP = {
    '_polygon_pool_init', '_collision_mesh_add_polygon', '_collision_mesh_pool_init', '_collision_mesh_alloc',
    '_mesh_release', '_collision_mesh_free', '_collision_mesh_clear_buckets', '_polygon_alloc',
    '_collision_mesh_take_buckets', '_fill_bytes', '_fill_blocks',
}
# what a JSR reaches in the game's C, and how: its host name, and its result
# and arguments, f for a float
CALLS = {'_atan2': ('native_atan2', 'f', 'ff'), '_asinf': ('asinf', 'f', 'f'), '_acosf': ('acosf', 'f', 'f'),
         '_floorf': ('floorf', 'f', 'f'), '_modff': ('modff', 'f', 'fp'),
         '_light_scale_color': ('light_scale_color', 'v', 'ppf')}
# data a source labels that the shim defines, which the assembly reaches by
# name
EXTERN_DATA = {'_collision_mesh_list'}

REG = re.compile(r'^R(\d+)$')
FR = re.compile(r'^FR(\d+)$')
DR = re.compile(r'^DR(\d+)$')
XD = re.compile(r'^XD(\d+)$')
FV = re.compile(r'^FV(\d+)$')
# a data directive's bytes an operand, or for .RES a unit of its count
DATA = {'.DATA.L': 4, '.DATA.W': 2, '.DATA.B': 1, '.FDATA.S': 4, '.RES.L': 4, '.RES.W': 2, '.RES.B': 1}
# a difference of two labels, which a table holds for a BRAF
DIFFERENCE = re.compile(r'\?\w+-\?\w+')


class Unknown(Exception):
    pass


def c_name(label):
    return re.sub(r'\W', '_', label.lstrip('?'))


def host_name(symbol):
    return symbol[1:] if symbol.startswith('_') else symbol


class Unit:
    """the labels and constants of one source"""

    def __init__(self, name):
        self.name = name
        self.equ = {}
        self.data = {}  # label: its array's C name


class Program:
    def __init__(self):
        self.functions = {}  # label: (C body, wrapper signature or None)
        self.externs = set()
        self.tables = []  # (unit, array, parts): a part is (size, directive, operand), or (bytes,) for a .RES
        self.failed = []
        self.code = set()  # each function's label, seen before its body
        self.cases = {}  # label: the number a table of differences holds for it
        self.constants = set()  # the literals a MOVA of a constant points at
        self.reached = set()  # each function's label that the C written calls or takes the address of

    def function(self, label):
        """the C name of label's function, which write() defines: its
        translation, or a call of the host's where this wrote none"""
        self.reached.add(label)
        return f'f_{c_name(label)}'

    def case_of(self, label):
        return self.cases.setdefault(label, len(self.cases))

    def braf_cases(self, scope):
        return ' '.join(f'case {number}: goto L_{c_name(label)};' for label, number in self.cases.items()
                        if label.startswith(f'?{scope}__'))


def split_operands(text):
    out, depth, cur = [], 0, ''
    for ch in text:
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
        if ch == ',' and depth == 0:
            out.append(cur.strip())
            cur = ''
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def number(unit, text):
    """a constant expression's value as an int, or None where a symbol's in it"""
    text = text.strip()
    text = re.sub(r"[Hh]'([0-9A-Fa-f]+)", lambda m: str(int(m[1], 16)), text)
    text = re.sub(r"[Dd]'(\d+)", r'\1', text)
    text = re.sub(r"[Bb]'([01]+)", lambda m: str(int(m[1], 2)), text)
    for name, value in unit.equ.items():
        text = re.sub(rf'\b{re.escape(name)}\b', f'({value})', text)
    if re.fullmatch(r'[\d\s+\-*/()<>]+', text):
        return eval(text.replace('/', '//'), {})  # the source's own constant
    return None


def address(unit, text, program):
    """text, a symbol and an offset, as a C expression of its 32-bit address"""
    m = re.fullmatch(r'([?\w]+)\s*([+-].*)?', text)
    if not m:
        raise Unknown(f'address {text}')
    name, offset = m[1], m[2] or ''
    if offset:
        offset = number(unit, offset)
        if offset is None:
            raise Unknown(f'offset {text}')
    if name in unit.data and name not in EXTERN_DATA:
        base = f'(uint32_t)(uintptr_t){unit.data[name]}'
    elif name in program.code and name not in EXTERN_DATA:
        base = f'(uint32_t)(uintptr_t){program.function(name)}'
    else:
        program.externs.add(host_name(name))
        base = f'(uint32_t)(uintptr_t)&e_{host_name(name)}'
    return f'({base} + {offset})' if offset else base


def reg(text):
    m = REG.match(text)
    if not m:
        raise Unknown(f'register {text}')
    return f'R[{m[1]}]'


def fr(text):
    m = FR.match(text)
    if not m:
        raise Unknown(f'float register {text}')
    return int(m[1])


def memory(unit, program, text, size):
    """an operand's address, what runs before it, and what runs after it"""
    m = re.fullmatch(r'@R(\d+)', text)
    if m:
        return f'R[{m[1]}]', '', ''
    m = re.fullmatch(r'@R(\d+)\+', text)
    if m:
        return f'R[{m[1]}]', '', f'R[{m[1]}] += {size};'
    m = re.fullmatch(r'@-R(\d+)', text)
    if m:
        return f'R[{m[1]}]', f'R[{m[1]}] -= {size};', ''
    m = re.fullmatch(r'@\(R0,R(\d+)\)', text)
    if m:
        return f'(R[0] + R[{m[1]}])', '', ''
    m = re.fullmatch(r'@\(R0,GBR\)', text)
    if m:
        return '(R[0] + GBR)', '', ''
    m = re.fullmatch(r'@\((.+),R(\d+)\)', text)
    if m:
        disp = number(unit, m[1])
        if disp is None:
            raise Unknown(f'displacement {text}')
        return f'(R[{m[2]}] + {disp})', '', ''
    m = re.fullmatch(r'@\((.+),GBR\)', text)
    if m:
        disp = number(unit, m[1])
        if disp is None:
            raise Unknown(f'displacement {text}')
        return f'(GBR + {disp})', '', ''
    raise Unknown(f'memory {text}')


LOAD = {1: 'S8', 2: 'S16', 4: 'U32'}
SIZE = {'B': 1, 'W': 2, 'L': 4}
BINARY = {'ADD': '+=', 'SUB': '-=', 'AND': '&=', 'OR': '|=', 'XOR': '^='}
SHIFTS = {'SHLL2': '<<= 2', 'SHLL8': '<<= 8', 'SHLL16': '<<= 16',
          'SHLR2': '>>= 2', 'SHLR8': '>>= 8', 'SHLR16': '>>= 16'}
FLOAT2 = {'FADD': '+=', 'FSUB': '-=', 'FMUL': '*=', 'FDIV': '/='}
# the instructions the C leaves out. an LDC or an STC that comes to these is
# of a control register the C doesn't keep. LDC.L and STC.L aren't among
# them: each steps the register it loads from or stores through
NO_OP = {'NOP', 'PREF', 'OCBI', 'OCBP', 'OCBWB', 'LDC', 'STC'}


def move(unit, program, op, args):
    size = SIZE[op[-1]] if '.' in op else 4
    src, dst = args
    if src.startswith('#'):
        value = src[1:]
        n = number(unit, value)
        if n is None:
            return f'{reg(dst)} = {address(unit, value, program)};'
        if op == 'MOV':
            return f'{reg(dst)} = (uint32_t)(int32_t)(int8_t){n & 0xff};'
        if op == 'MOV.W':
            return f'{reg(dst)} = (uint32_t)(int32_t)(int16_t){n & 0xffff};'
        return f'{reg(dst)} = (uint32_t){n & 0xffffffff}u;'
    if src.startswith('@'):
        at, before, after = memory(unit, program, src, size)
        return f'{before} {{ uint32_t a_ = {at}; {after} {reg(dst)} = (uint32_t){LOAD[size]}(a_); }}'
    if dst.startswith('@'):
        at, before, after = memory(unit, program, dst, size)
        return f'{{ uint32_t v_ = {reg(src)}; {before} W{8 * size}({at}, v_); {after} }}'
    if REG.match(src) and op == 'MOV':
        return f'{reg(dst)} = {reg(src)};'
    if re.fullmatch(r'[?\w]+(\s*[+-].*)?', src):
        return f'{reg(dst)} = (uint32_t){LOAD[size]}({address(unit, src, program)});'
    raise Unknown(f'{op} {src},{dst}')


def pair(name):
    """the two words of a register that FPSCR.SZ makes 64 bits wide: DRn or
    XDn, or as an FRn spells them, an even one the front bank's and an odd one
    the back's"""
    m = DR.match(name) or XD.match(name)
    if m:
        bank = 'F' if name[0] == 'D' else 'X'
        return f'{bank}.u[{m[1]}]', f'{bank}.u[{int(m[1]) + 1}]'
    n = fr(name)
    return (f'F.u[{n}]', f'F.u[{n + 1}]') if n % 2 == 0 else (f'X.u[{n - 1}]', f'X.u[{n}]')


def fmove_with(unit, program, args, regs, size):
    """an FMOV of size bytes, regs giving an operand's words"""
    src, dst = args
    if src.startswith('@'):
        at, before, after = memory(unit, program, src, size)
        loads = ' '.join(f'{r} = U32(a_ + {4 * i});' for i, r in enumerate(regs(dst)))
        return f'{before} {{ uint32_t a_ = {at}; {after} {loads} }}'
    if dst.startswith('@'):
        at, before, after = memory(unit, program, dst, size)
        stores = ' '.join(f'W32(a_ + {4 * i}, {r});' for i, r in enumerate(regs(src)))
        return f'{before} {{ uint32_t a_ = {at}; {stores} {after} }}'
    return ' '.join(f'{d} = {s};' for d, s in zip(regs(dst), regs(src)))


def fmove(unit, program, op, args):
    """an FMOV by what FPSCR.SZ holds as it runs. DRn and XDn are 64 bits in the
    name, and an FRn is 32 bits but for SZ, which FSCHG turns"""
    if any(DR.match(a) or XD.match(a) for a in args):
        return fmove_with(unit, program, args, pair, 8)
    return (f'if (SZ) {{ {fmove_with(unit, program, args, pair, 8)} }} else '
            f'{{ {fmove_with(unit, program, args, lambda name: (f"F.u[{fr(name)}]",), 4)} }}')


def immediate(unit, op, operand):
    """op's immediate operand, a # and a constant expression, as its 32 bits.
    an ADD's and a CMP/EQ's is a byte the SH-4 extends by its sign, as it
    does a MOV's, so #H'FF is -1 there"""
    value = number(unit, operand[1:])
    if value is None:
        raise Unknown(f'{op} {operand}')
    if op in ('ADD', 'CMP/EQ') and 0x80 <= value <= 0xff:
        value -= 0x100
    return value & 0xffffffff


def instruction(unit, program, op, args):
    if op in ('LDC', 'STC') and 'GBR' in args:
        return f'GBR = {reg(args[0])};' if op == 'LDC' else f'{reg(args[1])} = GBR;'
    if op in NO_OP:
        return ';'
    if op == 'MOV' or op in ('MOV.B', 'MOV.W', 'MOV.L'):
        return move(unit, program, op, args)
    if op in ('FMOV', 'FMOV.S'):
        return fmove(unit, program, op, args)
    if op in BINARY:
        a, b = args
        if a.startswith('#'):
            return f'{reg(b)} {BINARY[op]} (uint32_t){immediate(unit, op, a)}u;'
        return f'{reg(b)} {BINARY[op]} {reg(a)};'
    if op in SHIFTS:
        return f'{reg(args[0])} {SHIFTS[op]};'
    if op == 'MOVA' and args[0].startswith('#'):
        value = immediate(unit, op, args[0])
        program.constants.add(value)
        return f'R[0] = (uint32_t)(uintptr_t)&k_{value:08x};'
    if op == 'MOVA':
        return f'R[0] = {address(unit, args[0], program)};'
    if op == 'MOVT':
        return f'{reg(args[0])} = T;'
    if op == 'TST':
        a, b = args
        if a.startswith('#'):
            return f'T = ({reg(b)} & {immediate(unit, op, a)}u) == 0;'
        return f'T = ({reg(b)} & {reg(a)}) == 0;'
    if op == 'CMP/EQ':
        a, b = args
        if a.startswith('#'):
            return f'T = {reg(b)} == (uint32_t){immediate(unit, op, a)}u;'
        return f'T = {reg(b)} == {reg(a)};'
    if op == 'CMP/GE':
        return f'T = (int32_t){reg(args[1])} >= (int32_t){reg(args[0])};'
    if op == 'CMP/GT':
        return f'T = (int32_t){reg(args[1])} > (int32_t){reg(args[0])};'
    if op == 'CMP/HS':
        return f'T = {reg(args[1])} >= {reg(args[0])};'
    if op == 'CMP/HI':
        return f'T = {reg(args[1])} > {reg(args[0])};'
    if op == 'CMP/PL':
        return f'T = (int32_t){reg(args[0])} > 0;'
    if op == 'CMP/PZ':
        return f'T = (int32_t){reg(args[0])} >= 0;'
    if op == 'DT':
        return f'{reg(args[0])} -= 1; T = {reg(args[0])} == 0;'
    if op == 'NEG':
        return f'{reg(args[1])} = 0u - {reg(args[0])};'
    if op == 'NOT':
        return f'{reg(args[1])} = ~{reg(args[0])};'
    if op == 'EXTS.W':
        return f'{reg(args[1])} = (uint32_t)(int32_t)(int16_t){reg(args[0])};'
    if op == 'EXTS.B':
        return f'{reg(args[1])} = (uint32_t)(int32_t)(int8_t){reg(args[0])};'
    if op == 'EXTU.W':
        return f'{reg(args[1])} = (uint16_t){reg(args[0])};'
    if op == 'EXTU.B':
        return f'{reg(args[1])} = (uint8_t){reg(args[0])};'
    if op == 'SWAP.W':
        return f'{reg(args[1])} = ({reg(args[0])} >> 16) | ({reg(args[0])} << 16);'
    if op == 'SWAP.B':
        a, b = reg(args[0]), reg(args[1])
        return f'{b} = ({a} & 0xffff0000u) | (({a} & 0xff) << 8) | (({a} >> 8) & 0xff);'
    if op == 'SHLL':
        r = reg(args[0])
        return f'T = {r} >> 31; {r} <<= 1;'
    if op == 'SHLR':
        r = reg(args[0])
        return f'T = {r} & 1; {r} >>= 1;'
    if op == 'SHAR':
        r = reg(args[0])
        return f'T = {r} & 1; {r} = (uint32_t)((int32_t){r} >> 1);'
    if op == 'ROTR':
        r = reg(args[0])
        return f'T = {r} & 1; {r} = ({r} >> 1) | ({r} << 31);'
    if op == 'ROTL':
        r = reg(args[0])
        return f'T = {r} >> 31; {r} = ({r} << 1) | ({r} >> 31);'
    if op == 'ROTCL':
        r = reg(args[0])
        return f'{{ uint32_t t_ = {r} >> 31; {r} = ({r} << 1) | T; T = t_; }}'
    if op == 'ROTCR':
        r = reg(args[0])
        return f'{{ uint32_t t_ = {r} & 1; {r} = ({r} >> 1) | (T << 31); T = t_; }}'
    if op == 'SHLD':
        s, n = reg(args[0]), reg(args[1])
        return (f'if ((int32_t){s} >= 0) {n} <<= ({s} & 31); else if (({s} & 31) == 0) {n} = 0; '
                f'else {n} >>= (32 - ({s} & 31));')
    if op == 'SHAD':
        s, n = reg(args[0]), reg(args[1])
        return (f'if ((int32_t){s} >= 0) {n} <<= ({s} & 31); else if (({s} & 31) == 0) '
                f'{n} = (uint32_t)((int32_t){n} >> 31); else {n} = (uint32_t)((int32_t){n} >> (32 - ({s} & 31)));')
    if op == 'SETT':
        return 'T = 1;'
    if op == 'CLRT':
        return 'T = 0;'
    if op == 'DMULS.L':
        return (f'{{ int64_t p_ = (int64_t)(int32_t){reg(args[0])} * (int32_t){reg(args[1])}; '
                'MACL = (uint32_t)p_; MACH = (uint32_t)(p_ >> 32); }')
    if op == 'DMULU.L':
        return (f'{{ uint64_t p_ = (uint64_t){reg(args[0])} * {reg(args[1])}; '
                'MACL = (uint32_t)p_; MACH = (uint32_t)(p_ >> 32); }')
    if op == 'MULU.W':
        return f'MACL = (uint32_t)(uint16_t){reg(args[0])} * (uint16_t){reg(args[1])};'
    if op == 'MULS.W':
        return f'MACL = (uint32_t)((int32_t)(int16_t){reg(args[0])} * (int16_t){reg(args[1])});'
    if op == 'MUL.L':
        return f'MACL = {reg(args[0])} * {reg(args[1])};'
    if op == 'LDS':
        return f'{args[1]} = {reg(args[0])};'
    if op == 'STS':
        return f'{reg(args[1])} = {args[0]};'
    if op == 'LDS.L':
        at, before, after = memory(unit, program, args[0], 4)
        return f'{before} {args[1]} = U32({at}); {after}'
    if op == 'STS.L':
        at, before, after = memory(unit, program, args[1], 4)
        return f'{before} W32({at}, {args[0]}); {after}'
    if op == 'FLDI0':
        return f'F.u[{fr(args[0])}] = 0;'
    if op == 'FLDI1':
        return f'F.u[{fr(args[0])}] = 0x3f800000u;'
    if op == 'FNEG':
        return f'F.u[{fr(args[0])}] ^= 0x80000000u;'
    if op == 'FABS':
        return f'F.u[{fr(args[0])}] &= 0x7fffffffu;'
    if op in FLOAT2:
        return f'F.f[{fr(args[1])}] {FLOAT2[op]} F.f[{fr(args[0])}];'
    if op == 'FMAC':
        n = fr(args[2])
        return f'F.f[{n}] = sa_fmac(F.f[0], F.f[{fr(args[1])}], F.f[{n}]);'
    if op == 'FSQRT':
        return f'F.f[{fr(args[0])}] = sa_fsqrt(F.f[{fr(args[0])}]);'
    if op == 'FSRRA':
        return f'F.f[{fr(args[0])}] = sa_fsrra(F.f[{fr(args[0])}]);'
    if op == 'FCMP/GT':
        return f'T = sa_fcmp_gt(F.f[{fr(args[1])}], F.f[{fr(args[0])}]);'
    if op == 'FCMP/EQ':
        return f'T = sa_fcmp_eq(F.f[{fr(args[1])}], F.f[{fr(args[0])}]);'
    if op == 'FTRC':
        return f'FPUL = (uint32_t)sa_ftrc(F.f[{fr(args[0])}]);'
    if op == 'FLOAT':
        return f'F.f[{fr(args[1])}] = sa_float((int32_t)FPUL);'
    if op == 'FLDS':
        return f'FPUL = F.u[{fr(args[0])}];'
    if op == 'FSTS':
        return f'F.u[{fr(args[1])}] = FPUL;'
    if op == 'FSCA':
        n = int(DR.match(args[1])[1])
        return f'sa_fsca(FPUL, &F.f[{n}]);'
    if op == 'FIPR':
        a, b = int(FV.match(args[0])[1]), int(FV.match(args[1])[1])
        return f'F.f[{b + 3}] = sa_fipr(&F.f[{b}], &F.f[{a}]);'
    if op == 'FTRV':
        n = int(FV.match(args[1])[1])
        return f'{{ float o_[4]; sa_ftrv(X.f, &F.f[{n}], o_); memcpy(&F.f[{n}], o_, sizeof o_); }}'
    if op == 'FRCHG':
        return 'frchg();'
    if op == 'FSCHG':
        return 'SZ ^= 1;'
    raise Unknown(f'{op} {",".join(args)}')


def parse_line(line):
    """a source line's label, mnemonic and operands"""
    code = line.split(';')[0].rstrip()
    label = None
    m = re.match(r'^([?_A-Za-z][?\w]*):\s*(.*)$', code)
    if m:
        label, code = m[1], ' ' + m[2]
    m = re.match(r'^\s+(\S+)\s*(.*)$', code)
    if not m:
        return label, None, []
    return label, m[1].upper() if m[1].startswith('.') else m[1], split_operands(m[2])


def scoped(text):
    """text with each ? label named for its scope, which runs from one label
    without a ? to the next, as SHC's assembler reads them"""
    out, scope = [], ''
    for line in text.split('\n'):
        m = re.match(r'^(_\w+):', line)
        if m:
            scope = m[1]
        code, sep, comment = line.partition(';')
        out.append(re.sub(r'\?(\w+)', lambda m: f'?{scope}__{m[1]}', code) + sep + comment)
    return '\n'.join(out)


def split_units(text):
    """(label, op, args, line) for each line of an assembly text"""
    return [(*parse_line(line), line) for line in scoped(text).split('\n')]


def data_runs(lines):
    """each run of data in lines, by its first label: a directive's name and
    operands, and any label on its line. a label alone begins a new run. an
    .ALIGN is a directive of its run where data follows it there: one ahead
    of the run's data aligns the run, whose array is aligned already, and
    one after its last datum aligns what follows the run"""
    runs, i = {}, 0
    while i < len(lines):
        label, op = lines[i][:2]
        if label and (op is None or op in DATA or op == '.ALIGN'):
            entries, aligns, j = [], [], i
            while j < len(lines):
                other, directive, args, _ = lines[j]
                if directive in DATA:
                    entries += [*aligns, (directive, args, other)]
                    aligns = []
                elif directive == '.ALIGN':
                    aligns += [(directive, args, None)] * bool(entries)
                elif directive is not None or (other and entries):
                    break
                j += 1
            if entries:
                runs[label] = entries
                i = j
                continue
        i += 1
    return runs


def run_parts(unit, entries):
    """a run's bytes, in order, and where each label on its lines falls. an
    .ALIGN leaves bytes up to its boundary, counted from the run's start:
    write() starts each array on 32 bytes, so no boundary may be wider"""
    parts, labels, offset = [], {}, 0
    for op, args, label in entries:
        if op == '.ALIGN':
            boundary = number(unit, args[0])
            if boundary not in (1, 2, 4, 8, 16, 32):
                sys.exit(f'asm2c: {unit.name}: .ALIGN {args[0]} inside a run of data, which starts on 32 bytes')
            gap = -offset % boundary
            parts += [(gap,)] * bool(gap)
            offset += gap
            continue
        if label:
            labels[label] = offset
        if op.startswith('.RES'):
            size = number(unit, args[0]) * DATA[op]
            parts.append((size,))
            offset += size
        else:
            parts += [(DATA[op], op, operand) for operand in args]
            offset += DATA[op] * len(args)
    return parts, labels


def translate(unit, program, lines, wrappers, only=()):
    """lines' functions into program, or only those of only. wrappers gives
    a label's signature"""
    for label, op, args, _ in lines:
        if op == '.EQU' and label and (value := number(unit, args[0])) is not None:
            unit.equ[label] = value
    runs = data_runs(lines)
    for label, entries in runs.items():
        array = f'd_{unit.name}_{c_name(label)}'
        parts, labels = run_parts(unit, entries)
        for other, offset in labels.items():
            unit.data[other] = f'((char *){array} + {offset})' if offset else array
        unit.data[label] = array
        program.tables.append((unit, array, parts))
    bodies, current = {}, None
    for label, op, args, _ in lines:
        if label and label not in unit.data and not label.startswith('?') and op != '.EQU':
            current = label
            bodies[current] = []
        if current and op not in DATA:
            bodies[current].append((label, op, args))
    names = list(bodies)
    program.failed += [f'{unit.name}: {name}: no function of the source' for name in sorted(set(only) - set(names))]
    for name, following in zip(names, names[1:] + [None]):
        if name in SKIP or (only and name not in only):
            continue
        reached = set(program.reached)
        try:
            program.functions[name] = (emit_function(unit, program, name, bodies[name], following), wrappers.get(name))
        except Unknown as e:
            # what it alone reached, no C reaches
            program.reached = reached
            program.failed.append(f'{unit.name}: {name}: {e}')


def emit_function(unit, program, name, body, nxt):
    """a function's C, each instruction a statement, a delay slot first"""
    out = []
    known = {}  # a register's symbol, as MOV.L #symbol last set it
    # a line that holds neither a label nor an instruction is none of the function's
    flat = [(l, o, a) for l, o, a in body if (l or o) and (o is None or not o.startswith('.'))]
    i = 0
    while i < len(flat):
        label, op, args = flat[i]
        if label and label.startswith('?'):
            out.append(f'L_{c_name(label)}:;')
            known = {}
        if op is None:
            i += 1
            continue
        if op in ('BT', 'BF'):
            cond = 'T' if op == 'BT' else '!T'
            target = args[0]
            out.append(jump(unit, program, f'if ({cond})', target, name))
            i += 1
            continue
        if op in ('BT/S', 'BF/S'):
            cond = 't_' if op == 'BT/S' else '!t_'
            after = past(flat, i, runs_on=True)
            slot = delay(unit, program, flat, i, known)
            out.append(f'{{ int t_ = T; {slot} {jump(unit, program, f"if ({cond})", args[0], name)} }}')
            i = after
            continue
        if op == 'BRA':
            slot = delay(unit, program, flat, i, known)
            out.append(f'{slot} {jump(unit, program, "", args[0], name)}')
            i = past(flat, i, runs_on=False)
            continue
        if op == 'BSR':
            after = past(flat, i, runs_on=True)
            slot = delay(unit, program, flat, i, known)
            out.append(f'{slot} {c_call(args[0]) if args[0] in CALLS else f"{program.function(args[0])}();"}')
            # the function it calls may write any register
            known = {}
            i = after
            continue
        if op == 'BSRF':
            raise Unknown('BSRF')
        if op in ('JSR', 'JMP'):
            r = int(REG.match(args[0][1:])[1])
            # the branch reads its register before the slot, which may write it
            symbol = known.get(r)
            after = past(flat, i, runs_on=op == 'JSR')
            slot = delay(unit, program, flat, i, known)
            out.append(call(program, r, symbol, slot) + (' return;' if op == 'JMP' else ''))
            # the function it reaches may write any register
            known = {}
            i = after
            continue
        if op == 'BRAF':
            r = int(REG.match(args[0])[1])
            slot = delay(unit, program, flat, i, known)
            out.append(f'{{ uint32_t c_ = R[{r}]; {slot} switch (c_) {{ {program.braf_cases(name)} default: abort(); }} }}')
            i = past(flat, i, runs_on=False)
            continue
        if op == 'RTS':
            slot = delay(unit, program, flat, i, known)
            out.append(f'{slot} return;')
            i = past(flat, i, runs_on=False)
            continue
        track(unit, op, args, known)
        out.append(instruction(unit, program, op, args))
        i += 1
    # a function whose last line isn't a branch's delay slot runs into the next
    ends = [o for _, o, _ in flat if o][-2:]
    if nxt and not (set(ends) & {'RTS', 'BRA', 'JMP'}):
        out.append(f'{program.function(nxt)}();')
    return out


def track(unit, op, args, known):
    """known, each register's symbol, after op: a MOV.L of a symbol names its
    register, and any other instruction whose last operand is a register
    leaves that one no name, as it may write it. a JSR through a register of
    no name goes by the register's value. emit_function() leaves no register
    a name past a BSR, a JSR or a JMP"""
    if op == 'MOV.L' and args[0].startswith('#') and REG.match(args[1]):
        known[int(REG.match(args[1])[1])] = args[0][1:]
    elif args and REG.match(args[-1]):
        known.pop(int(REG.match(args[-1])[1]), None)


def slot_of(flat, i):
    """where the delay slot of the branch at flat[i] stands, past each line
    that holds a label alone, and whether a label names it"""
    j = i + 1
    while j < len(flat) and flat[j][1] is None:
        j += 1
    return j, any(label for label, _, _ in flat[i + 1:j + 1])


def past(flat, i, runs_on):
    """the line to write after the branch at flat[i] and its slot. a slot
    that a label names is where another branch lands too. past a branch that
    never comes back it is written again, under its label. a call and a
    conditional branch run on from their slot, where that would run it twice"""
    j, named = slot_of(flat, i)
    if not named:
        return j + 1
    if runs_on:
        op = flat[i][1]
        raise Unknown('a label on a conditional delay slot' if op in ('BT/S', 'BF/S') else
                      f'a label on the delay slot of a {op}')
    return i + 1


def delay(unit, program, flat, i, known):
    j, _ = slot_of(flat, i)
    if j >= len(flat):
        raise Unknown(f'no delay slot after its {flat[i][1]}')
    _, op, args = flat[j]
    track(unit, op, args, known)
    return instruction(unit, program, op, args)


def jump(unit, program, cond, target, name):
    if target.startswith('?'):
        return f'{cond} goto L_{c_name(target)};'
    return f'{cond} {{ {program.function(target)}(); return; }}'


def c_call(symbol):
    """a call of CALLS' symbol, its arguments as SHC passes them"""
    host, result, args = CALLS[symbol]
    ints = iter(range(4, 8))
    floats = iter(range(4, 12))
    passed = ', '.join(f'F.f[{next(floats)}]' if kind == 'f' else f'(void *)widened(R[{next(ints)}])'
                       for kind in args)
    return f'{"F.f[0] = " if result == "f" else ""}{host}({passed});'


def call(program, r, symbol, slot):
    """a JSR's or a JMP's slot, and its call to where register r pointed
    before the slot ran: symbol, where a MOV.L named it, or r's value"""
    if symbol in CALLS:
        return f'{slot} {c_call(symbol)}'
    if symbol and symbol in program.code:
        return f'{slot} {program.function(symbol)}();'
    return f'{{ uint32_t j_ = R[{r}]; {slot} native_jsr(j_); }}'


def parse_signature(result, args):
    """(result kind, argument kinds): f, p for a pointer or integer, v for void.
    an inline body's static says nothing of its result, and a const or a
    volatile nothing of a kind: a const float is a float, and a pointer to
    one is a pointer"""
    result = re.sub(r'\b(static|const|volatile)\b', '', result)
    kind = 'f' if re.fullmatch(r'\s*(Float|float)\s*', result) else 'v' if result.strip() == 'void' else 'p'
    kinds = ''
    if args.strip() not in ('', 'void'):
        for a in split_operands(args):
            a = re.sub(r'\b(const|volatile)\b', '', a)
            kinds += 'f' if re.match(r'\s*(Float|float)\s+\w+$', a) else 'p'
    return kind, kinds


def src_wrappers(text):
    """each exported function's signature, from the comment above it"""
    wrappers = {}
    lines = text.split('\n')
    for i, line in enumerate(lines):
        m = re.match(r'^;\s*(\w[\w\s\*]*?[\s\*])(\w+)\((.*)$', line)
        if not m:
            continue
        args = m[3]
        j = i
        while ')' not in args and j + 1 < len(lines):
            j += 1
            args += ' ' + lines[j].lstrip('; ')
        args = args[:args.rindex(')')] if ')' in args else args
        wrappers['_' + m[2]] = parse_signature(m[1], args)
    return wrappers


def read_src(source, program):
    path, _, only = source.partition(':')
    text = Path(path).read_text(errors='replace')
    unit = Unit(re.sub(r'\W', '_', Path(path).stem))
    translate(unit, program, split_units(text), src_wrappers(text), {f'_{name}' for name in only.split(',') if name})


INLINE = re.compile(r'#pragma inline_asm\((\w+)(?:\(size=\d+\))?\)\n(.*?)\n\}', re.S)


def inline_bodies(text):
    """(name, declaration, assembly) of each inline body of a C text: its
    assembly under a label of its own name, its comments out"""
    for m in INLINE.finditer(text):
        sig, _, body = m[2].partition('\n{')
        yield m[1], sig, '_' + m[1] + ':\n' + re.sub(r'/\*.*?\*/', '', body, flags=re.S)


def read_inline(path, program):
    for name, sig, assembly in inline_bodies(Path(path).read_text(errors='replace')):
        sm = re.search(r'([\w\s\*]*?[\s\*])' + name + r'\s*\((.*)\)', sig, re.S)
        wrapper = parse_signature(sm[1], sm[2]) if sm else None
        translate(Unit(name), program, split_units(assembly), {'_' + name: wrapper})


def table_init(program):
    """the lines that fill the tables, as the game starts"""
    lines = ['void native_asm_init(void)', '{']
    for unit, array, parts in program.tables:
        offset = 0
        for part in parts:
            if len(part) == 1:
                offset += part[0]
                continue
            size, op, operand = part
            ctype = {4: 'uint32_t', 2: 'uint16_t', 1: 'uint8_t'}[size]
            if op == '.FDATA.S':
                lines.append(f'    {{ float f_ = {float_literal(operand)}; memcpy({array} + {offset}, &f_, 4); }}')
            elif (value := number(unit, operand)) is not None:
                lines.append(f'    *({ctype} *)({array} + {offset}) = {value & ((1 << (8 * size)) - 1)}u;')
            elif DIFFERENCE.fullmatch(operand):
                lines.append(f'    *({ctype} *)({array} + {offset}) = {program.case_of(operand.split("-")[0])}u;')
            else:
                lines.append(f'    *(uint32_t *)({array} + {offset}) = {address(unit, operand, program)};')
            offset += size
    return lines + ['}']


def wrapper_lines(name, signature):
    """the C the game calls, which loads SHC's registers and runs the function.
    an integer past R7 goes on the stack, the first at @R15, as SHC passes it"""
    result, args = signature
    params, loads, stacked = [], [], []
    ints = iter(range(4, 8))
    floats = iter(range(4, 12))
    for i, kind in enumerate(args):
        if kind == 'f':
            params.append(f'float a{i}')
            loads.append(f'F.f[{next(floats)}] = a{i};')
        else:
            params.append(f'uintptr_t a{i}')
            if (r := next(ints, None)) is not None:
                loads.append(f'R[{r}] = narrow(a{i});')
            else:
                stacked.append(f'W32(R[15] + {4 * len(stacked)}, narrow(a{i}));')
    returns = {'f': 'float', 'v': 'void', 'p': 'uintptr_t'}[result]
    give = {'f': 'return F.f[0];', 'v': '', 'p': 'return widened(R[0]);'}[result]
    run = f'f_{c_name(name)}();'
    if stacked:
        size = 4 * len(stacked)
        run = f'R[15] -= {size}; {" ".join(stacked)} {run} R[15] += {size};'
    return [f'{returns} {host_name(name)}({", ".join(params) or "void"})', '{',
            f'    {" ".join(loads)} enter(); {run} leave(); {give}', '}', '']


def write(path, program):
    # the tables' symbols join the externs, so they fill first
    init = table_init(program)
    # a function the C reaches that this wrote no translation of is the host's
    hosts = sorted(program.reached - set(program.functions))
    program.externs.update(host_name(name) for name in hosts)
    out = ['/* written by engine/tools/asm2c.py: SH-4 assembly as C, a line an instruction */', '#include "sh4.h"', '']
    for _, array, parts in program.tables:
        size = sum(part[0] for part in parts)
        out.append(f'static unsigned char {array}[{(size + 7) & ~7}] __attribute__((aligned(32)));')
    for host, result, args in CALLS.values():
        params = ', '.join('float' if kind == 'f' else 'void *' for kind in args)
        out.append(f'{"float" if result == "f" else "void"} {host}({params});')
    out += [f'static uint32_t k_{value:08x} = {value}u;' for value in sorted(program.constants)]
    out += [f'extern char e_{extern} __asm__("{extern}");' for extern in sorted(program.externs)]
    out += [f'static void f_{c_name(name)}(void);' for name in [*program.functions, *hosts]]
    out.append('')
    for name, (body, wrapper) in program.functions.items():
        out += [f'static void f_{c_name(name)}(void)', '{', *('    ' + line for line in body), '}', '']
        if wrapper:
            out += wrapper_lines(name, wrapper)
    for name in hosts:
        out += [f'static void f_{c_name(name)}(void)', '{',
                f'    native_jsr((uint32_t)(uintptr_t)&e_{host_name(name)});', '}', '']
    # the matrix the game's FTRVs turn by, which the adapter reads at a draw
    out += ['float *native_xmtrx(void)', '{', '    return X.f;', '}', '']
    # where a JSR through a register lands
    out += ['void native_jsr(uint32_t target)', '{']
    out += [f'    if (target == (uint32_t)(uintptr_t)f_{c_name(name)}) {{ f_{c_name(name)}(); return; }}'
            for name in [*program.functions, *hosts]]
    # a function CALLS names, by its own signature, as a call by name gives it
    out += [f'    if (target == (uint32_t)(uintptr_t)&e_{host_name(symbol)}) {{ {c_call(symbol)} return; }}'
            for symbol in sorted(CALLS) if host_name(symbol) in program.externs]
    # any other is the game's C: SHC's R4 to R7 and FR4 to FR7 are the
    # integer and float arguments of x86-64's System V calls in order,
    # whichever a function takes, as each class has registers of its own
    # there. a register widens as a pointer, whose low half a number keeps,
    # and the float result is read where those calls leave it. Windows and
    # every other processor pass them another way, so there such a JSR ends
    # the run
    out += ['#if defined(__x86_64__) && !defined(_WIN32)',
            '    uintptr_t r_ = ((uintptr_t (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, float, float, float, float))'
            'widened(target))(widened(R[4]), widened(R[5]), widened(R[6]), widened(R[7]), F.f[4], F.f[5], F.f[6], '
            'F.f[7]);',
            '    __asm__ volatile("movss %%xmm0, %0" : "=m"(F.f[0]));', '    R[0] = (uint32_t)r_;',
            '#else',
            '    fprintf(stderr, "sa1: a JSR reached %#x, the game\'s C, which asm2c.py\'s C calls by x86-64\'s '
            'System V convention alone: run it on such a host, or lift the function the JSR calls\\n", '
            '(unsigned)target);',
            '    abort();',
            '#endif', '}', '']
    Path(path).write_text('\n'.join(out + init) + '\n')


def float_literal(text):
    value = re.sub(r"^F'", '', text.strip()).replace('S', 'e')
    return f'{value}F' if re.search(r'[.eE]', value) else f'{value}.0F'


def prescan(source, program):
    """see a source's functions, and number its BRAF tables, before its code
    names them: each text as its translation reads it, so a label has one
    scope in both"""
    text = Path(source).read_text(errors='replace')
    for text in [text] if '.src' in source else [assembly for _, _, assembly in inline_bodies(text)]:
        text = scoped(text)
        program.code.update(m[1] for m in re.finditer(r'^(_\w+):', text, re.M))
        for m in DIFFERENCE.finditer(text):
            program.case_of(m[0].split('-')[0])


def main():
    out, *sources = [arg for arg in sys.argv[1:] if arg != '--explore']
    program = Program()
    for source in sources:
        prescan(source.partition(':')[0], program)
    for source in sources:
        (read_src if '.src' in source else read_inline)(source, program)
    for failure in program.failed:
        print(f'asm2c: {failure}')
    if program.failed and '--explore' not in sys.argv:
        sys.exit(f'asm2c: {len(program.failed)} functions it can\'t write as C, and no {out}')
    write(out, program)
    print(f'asm2c: {len(program.functions)} functions' +
          f', explored: {len(program.failed)} left out' * bool(program.failed))


if __name__ == '__main__':
    main()
