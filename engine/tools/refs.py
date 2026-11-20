#!/usr/bin/env python3
"""The native slice's pass over the game's LLVM IR.

refs.py FILE.ll [--base N] [--check] [--host] lowers, in place, what
marks.py's __ptr32 leaves for the backend, before the optimizer runs. A marked
pointer is a 32-bit reference: the front end types every read, write and
address of one, so this pass sees each place a reference becomes a host
pointer.

No 64-bit backend of clang 21's, x86-64's or AArch64's, selects a call through
a reference past -O0, and x86-64's no reference to a static's address, so a
call widens its reference first, and a static's address is a relocated word. A
memory intrinsic takes host pointers.

A reference is an address in the game's 4 GB domain, which lies at --base, a
multiple of 4 GB, 0 by default. Past 0 the pass lowers every load, store and
cast of one itself, so the backend selects no 32-bit pointer at all: widening
puts base in the high half, null staying null, and narrowing keeps the low
half. So does every cast of the game's between a number and a pointer, since a
number that holds a pointer holds a reference, and a constant address is one,
and every reference a call with no prototype passes, which its callee takes as
a host pointer.

--check narrows through native_narrow, which refuses a pointer outside the
domain: the one place a host pointer could reach a task's words. --host reads
the slice's own C, whose numbers are the host's.

The pass reads the IR a line at a time, and knows a reference by the forms
below. LOWERED names the instructions it lowers one in, and KEPT those it
leaves the backend, which moves, steps and compares a reference as the 32-bit
word it is. It ends, naming the line, at a reference in any other instruction,
at one of LOWERED's in a form it can't lower where its base or --check has it
lower that one, past base 0 at a constant that makes a reference a host
pointer or a number of the host's width, or a number of the game's of another
width a host pointer, and at a function that takes a reference and then any
arguments, as a call of one reads as a call with no prototype does, under
--check at a native_narrow the module gives another type than the pass calls
it by, at an intrinsic on a reference that is none of the three it gives host
pointers, and at a pointer of another address space. So a form the compiler
comes to write that the pass has no rule for stops the build, where it would
reach the backend with no base or as a call it can't select.

The adapter and the shim find a static by its name, so the optimizer keeps
each as the source spells it, where it would otherwise fold a flag to a bool
of its own meaning."""
import re
import sys
from pathlib import Path

REF = 'ptr addrspace(271)'
# a reference's address space, which a call through one names with no ptr
SPACE = 'addrspace(271)'
# an instruction that names a reference, by its opcode: those the pass lowers
# a reference in, as line() does, and those the backend takes as they stand
LOWERED = {'load', 'store', 'call', 'addrspacecast', 'ptrtoint', 'inttoptr'}
KEPT = {'alloca', 'getelementptr', 'icmp', 'phi', 'select', 'ret'}
OPCODE = re.compile(r'^\s+(?:%[\w.]+ = )?(?:tail |musttail |notail )?([a-z_]+)\b')
# a pointer of an address space that is no reference's, an intrinsic whose
# name gives an operand a reference's, and the address space a call goes
# through, which no ptr stands before as it does a reference's type
OTHER_SPACE = re.compile(r'addrspace\((?!271\))')
INTRINSIC_REF = re.compile(r'@llvm\.[\w.]*\.p271\b')
CALLED = re.compile(r'(?<!ptr )addrspace\(271\) ')
# a call through a reference: the head up to the type, the type, the callee
CALL = re.compile(r'^(?P<head>\s*(?:%[\w.]+ = )?(?:tail |musttail |notail )?call )(?P<attrs>.*?)'
                  r'(?<!ptr )addrspace\(271\) (?P<type>.*?)(?P<callee>%[\w.]+)\((?P<rest>.*)$')
CAST = 'addrspacecast ('
CONSTANT_ADDRESS = 'inttoptr (i64 '
# a constant number as a pointer, a host pointer where it ends: to ptr
CONSTANT_POINTER = 'inttoptr (i'
# a constant reference as a number, the host's where it ends: to i64
CONSTANT_NUMBER = f'ptrtoint ({REF} '
WIDEN = re.compile(r'^\s*(?P<name>%[\w.]+) = addrspacecast ptr addrspace\(271\) (?P<value>%[\w.]+) to ptr(?P<rest>,.*)?$')
NARROW = re.compile(r'^\s*(?P<name>%[\w.]+) = addrspacecast ptr (?P<value>%[\w.]+) to ptr addrspace\(271\)(?P<rest>,.*)?$')
LOAD = re.compile(r'^(?P<head>\s*%[\w.]+ = load (?:volatile )?.*?, )ptr addrspace\(271\) (?P<value>%[\w.]+)(?P<rest>.*)$')
STORE = re.compile(r'^(?P<head>\s*store (?:volatile )?.*?, )ptr addrspace\(271\) (?P<value>%[\w.]+)(?P<rest>.*)$')
NUMBER = re.compile(r'^\s*(?P<name>%[\w.]+) = inttoptr i(?P<bits>\d+) (?P<value>%[\w.]+) to ptr(?P<rest>,.*)?$')
WIDE_NUMBER = re.compile(r'^\s*(?P<name>%[\w.]+) = ptrtoint ptr addrspace\(271\) (?P<value>%[\w.]+) to i64(?P<rest>,.*)?$')
# the memory intrinsics, whose names give their operands' address spaces, and
# say where one calls no function
BLOCK = re.compile(r'@llvm\.(memcpy|memmove|memset)(\.inline)?\.p\d+(?:\.p\d+)?\.i(\d+)')
BLOCK_REF = re.compile(r'ptr addrspace\(271\)( (?:noalias |nocapture |writeonly |readonly |align \d+ )*)(%[\w.]+)')
# a call's type with no prototype, up to its callee, which clang writes for
# x86-64's System V as its arguments' types and then ..., and a reference a
# call passes, with its attributes
UNPROTOTYPED = re.compile(r'\((?:[^()]|\([^()]*\))*\.\.\.\) (?:@[\w.$]+|%[\w.]+)\(')
ARGUMENT_REF = re.compile(r'ptr addrspace\(271\)((?: (?:noalias|nocapture|writeonly|readonly|noundef|nonnull|align \d+))*) '
                          r'(%[\w.]+|null)')
# a function that takes a reference and then any arguments, by its declaration
# or its definition: a call of one reads as a call with no prototype does
VARIADIC_REF = re.compile(r'(?:declare|define) [^@]*@[\w.$]+\(.*ptr addrspace\(271\).*\.\.\.\)')
# native_narrow's declaration or its definition, and one of the type the pass
# calls it by
NARROWS = re.compile(r'^(?:declare|define) .*@native_narrow\(', re.M)
NARROWS_TYPE = re.compile(r'(?:declare|define) (?:\S+ )*i32 @native_narrow\(ptr(?! addrspace)[^,]*\)')
STATIC = re.compile(r'^(@[\w.]+) = internal (?:unnamed_addr )?global ', re.M)
# the list of the names a module keeps from the optimizer, as clang writes it
# for a source that marks one used
USED = re.compile(r'^@llvm\.compiler\.used = appending global \[\d+ x ptr\] \[(.*)\], section "llvm\.metadata"$', re.M)


def closing(text, start):
    """the index past the parenthesis that closes the one before start"""
    depth = 1
    for i in range(start, len(text)):
        depth += {'(': 1, ')': -1}.get(text[i], 0)
        if depth == 0:
            return i + 1
    raise ValueError(text)


def operands(text):
    """an instruction's operands, past its opcode: text split at each comma
    no bracket holds"""
    found, depth, start = [], 0, 0
    for i, c in enumerate(text):
        depth += {'(': 1, '[': 1, '{': 1, '<': 1, ')': -1, ']': -1, '}': -1, '>': -1}.get(c, 0)
        if c == ',' and depth == 0:
            found.append(text[start:i].strip())
            start = i + 1
    return found + [text[start:].strip()]


def constant_number(line):
    """whether line holds a constant reference as a number of the host's
    width"""
    at = line.find(CONSTANT_NUMBER)
    while at >= 0:
        if line[at:closing(line, at + len('ptrtoint ('))].endswith(' to i64)'):
            return True
        at = line.find(CONSTANT_NUMBER, at + 1)
    return False


def constant_pointer(line):
    """whether line holds a constant number of another width than the host's
    as a host pointer"""
    at = line.find(CONSTANT_POINTER)
    while at >= 0:
        if not line.startswith(CONSTANT_ADDRESS, at) and \
                line[at:closing(line, at + len('inttoptr ('))].endswith(' to ptr)'):
            return True
        at = line.find(CONSTANT_POINTER, at + 1)
    return False


class Pass:
    def __init__(self, base=0, check=False, host=False):
        self.base = base
        self.check = check
        self.host = host
        self.serial = 0
        self.blocks = set()

    def name(self):
        self.serial += 1
        return f'%ref.{self.serial}'

    def constant_refs(self, line):
        """each static's address cast to a reference, as a relocated word"""
        at = line.find(CAST)
        while at >= 0:
            end = closing(line, at + len(CAST))
            inner = line[at + len(CAST):end - 1]
            if inner.startswith('ptr ') and inner.endswith(f' to {REF}'):
                number = f'ptrtoint ({inner[:-len(f" to {REF}")]} to i64)'
                if self.base:
                    number = f'sub (i64 {number}, i64 {self.base})'
                word = f'inttoptr (i32 trunc (i64 {number} to i32) to {REF})'
                line = line[:at] + word + line[end:]
                end = at + len(word)
            at = line.find(CAST, end)
        return line

    def constant_addresses(self, line):
        """each constant address but null, in the domain by its low half,
        which is null where it is 0, as it is for a number a value holds"""
        at = line.find(CONSTANT_ADDRESS)
        while at >= 0:
            end = closing(line, at + len('inttoptr ('))
            if (m := re.fullmatch(r'inttoptr \(i64 (-?\d+) to ptr\)', line[at:end])) and int(m[1]):
                low = int(m[1]) & 0xffffffff
                word = f'inttoptr (i64 {self.base | low} to ptr)' if low else 'null'
                line = line[:at] + word + line[end:]
                end = at + len(word)
            at = line.find(CONSTANT_ADDRESS, end)
        return line

    def word(self, ref, out):
        """a reference as its number, and that number at the host's width"""
        word, wide = self.name(), self.name()
        out += [f'  {word} = ptrtoint {REF} {ref} to i32', f'  {wide} = zext i32 {word} to i64']
        return word, wide

    def widen(self, ref, out):
        """a reference as a host pointer, for an access, which null faults
        either way"""
        _, wide = self.word(ref, out)
        target = self.name()
        if self.base:
            placed = self.name()
            out.append(f'  {placed} = or i64 {wide}, {self.base}')
            wide = placed
        out.append(f'  {target} = inttoptr i64 {wide} to ptr')
        return target

    def based(self, word, wide, out, name=None, rest=''):
        """the i64 that holds wide in the domain, null staying null: by name,
        with rest closing its instruction, where the caller gives them, and
        else by a name of the pass's own. returns the name"""
        placed, none = self.name(), self.name()
        picked = name or self.name()
        out += [f'  {placed} = or i64 {wide}, {self.base}', f'  {none} = icmp eq i32 {word}, 0',
                f'  {picked} = select i1 {none}, i64 0, i64 {placed}{rest}']
        return picked

    def place(self, word, wide, m, out):
        """the host pointer a number gives, null staying null"""
        out.append(f'  {m["name"]} = inttoptr i64 {self.based(word, wide, out)} to ptr{m["rest"] or ""}')

    def wide_number(self, m, out):
        """a reference as a host's number, as a pointer of the host's would
        give it: the difference of two pointers, or one kept in a long"""
        self.based(*self.word(m['value'], out), out, m['name'], m['rest'] or '')

    def arguments(self, line, out):
        """the references a call with no prototype passes, as host pointers,
        which its callee takes"""
        m = UNPROTOTYPED.search(line)
        if not m:
            return line
        args = ARGUMENT_REF.sub(lambda a: f'ptr{a[1]} {"null" if a[2] == "null" else self.pointer(a[2], out)}',
                               line[m.end():])
        return line[:m.start()] + m[0].replace(REF, 'ptr') + args

    def pointer(self, ref, out):
        """a reference as a host pointer, null staying null"""
        picked, target = self.based(*self.word(ref, out), out), self.name()
        out.append(f'  {target} = inttoptr i64 {picked} to ptr')
        return target

    def narrow(self, m, out):
        word = self.name()
        if self.check:
            out.append(f'  {word} = call i32 @native_narrow(ptr {m["value"]})')
        else:
            number = self.name()
            out += [f'  {number} = ptrtoint ptr {m["value"]} to i64', f'  {word} = trunc i64 {number} to i32']
        out.append(f'  {m["name"]} = inttoptr i32 {word} to {REF}{m["rest"] or ""}')

    def number(self, m, out):
        word, wide = self.name(), self.name()
        bits = int(m['bits'])
        out += [f'  {word} = add i32 {m["value"]}, 0' if bits == 32 else
                f'  {word} = {"trunc" if bits > 32 else "zext"} i{bits} {m["value"]} to i32',
                f'  {wide} = zext i32 {word} to i64']
        self.place(word, wide, m, out)

    def block(self, line, out):
        """a memory intrinsic, on host pointers"""
        def named(m):
            name = f'@llvm.{m[1]}{m[2] or ""}.p0{"" if m[1] == "memset" else ".p0"}.i{m[3]}'
            self.blocks.add(name)
            return name

        return BLOCK.sub(named, BLOCK_REF.sub(lambda m: f'ptr{m[1]}{self.widen(m[2], out)}', line))

    def line(self, line, out):
        if CAST in line:
            line = self.constant_refs(line)
        if self.base and not self.host and CONSTANT_ADDRESS in line:
            line = self.constant_addresses(line)
        if not line.startswith('  '):
            return line
        if ' call ' in f' {line}':
            if BLOCK.search(line):
                return self.block(line, out)
            if call := CALL.match(line):
                line = f'{call["head"]}{call["attrs"]}{call["type"]}{self.widen(call["callee"], out)}({call["rest"]}'
                return self.arguments(line, out) if self.base else line
            if self.base and REF in line:
                return self.arguments(line, out)
        if self.base or self.check:
            if m := NARROW.match(line):
                return self.narrow(m, out)
        if self.base:
            if m := WIDEN.match(line):
                return self.place(*self.word(m['value'], out), m, out)
            if m := WIDE_NUMBER.match(line):
                return self.wide_number(m, out)
            if m := LOAD.match(line) or STORE.match(line):
                return f'{m["head"]}ptr {self.widen(m["value"], out)}{m["rest"]}'
            if not self.host and (m := NUMBER.match(line)):
                return self.number(m, out)
        return line

    def unlowered(self, line):
        """what line, one the pass keeps of its input, holds that it has no
        rule for or should have lowered, or None"""
        if OTHER_SPACE.search(line):
            return "a pointer of an address space that is no reference's"
        if INTRINSIC_REF.search(line):
            return 'an intrinsic on a reference that the pass gives no host pointers'
        if self.base and CAST in line:
            return "a constant's cast of a reference to a host pointer, which takes no base"
        if self.base and constant_number(line):
            return "a constant reference as a host's number, which takes no base"
        if self.base and not self.host and constant_pointer(line):
            return "a constant number of another width than the host's as a host pointer, which takes no base"
        if self.base and VARIADIC_REF.match(line):
            return 'a function that takes a reference and then any arguments, whose call reads as one with no prototype'
        if self.check and NARROWS.match(line) and not NARROWS_TYPE.match(line):
            return 'native_narrow in another type than the one the pass calls it by'
        if not line.startswith('  '):
            return None
        opcode = OPCODE.match(line)
        opcode = opcode[1] if opcode else ''
        if SPACE in line and opcode not in LOWERED | KEPT:
            return f'a reference in {opcode or "a line that is no instruction"}'
        if opcode == 'call' and CALLED.search(line):
            return 'a call through a reference that no local value names'
        if opcode == 'call' and BLOCK.search(line) and REF in line:
            return "a memory intrinsic's reference in a form the pass doesn't know"
        if opcode == 'addrspacecast' and (self.base or self.check and line.split(',')[0].endswith(f' to {REF}')):
            return 'a cast of a value that is no local one'
        if not self.base:
            return None
        if opcode in ('load', 'store') and operands(line.split(f'{opcode} ', 1)[1])[1].startswith(REF):
            return f'a {opcode} through a reference that no local value names'
        if opcode == 'call' and (m := UNPROTOTYPED.search(line)) and REF in line[m.end():]:
            return 'a reference that no local value names, which a call with no prototype passes'
        if opcode == 'ptrtoint' and line.split(',')[0].endswith(' to i64') and REF in line:
            return "a reference that no local value names, as a host's number"
        if opcode == 'inttoptr' and not self.host and line.split(',')[0].endswith(' to ptr'):
            return 'a number that no local value names, as a host pointer'
        return None

    def declarations(self, text):
        """what the pass calls, declared: each memory intrinsic on host
        pointers, and under --check native_narrow, where text, the module,
        neither declares nor defines it"""
        return [f'declare void {name}(ptr, {"i8" if "memset" in name else "ptr"}, '
                f'i{name.rsplit(".i", 1)[1]}, i1 immarg)' for name in sorted(self.blocks)] + \
            (['declare i32 @native_narrow(ptr)'] if self.check and not NARROWS.search(text) else [])

    def run(self, text, where='refs.py'):
        """text lowered. where names the IR in what the pass says as it ends
        at a line it has no rule for"""
        out = []
        for number, line in enumerate(text.split('\n'), 1):
            kept = self.line(line, out)
            if kept is None or line.startswith('declare') and BLOCK.search(line):
                continue
            if what := self.unlowered(kept):
                sys.exit(f'{where}:{number}: {what}: {line.strip()}')
            out.append(kept)
        return keep_statics('\n'.join(out + self.declarations(text)), where)


def keep_statics(text, where='refs.py'):
    """text with each static in @llvm.compiler.used, so the optimizer keeps
    it as the source spells it. the list a module has takes them beside its
    own names. where names the IR where the pass ends at a list in another
    form"""
    statics = [f'ptr {name}' for name in STATIC.findall(text)]
    if not statics:
        return text
    held = USED.search(text)
    if not held and '@llvm.compiler.used' in text:
        sys.exit(f"{where}: @llvm.compiler.used in a form the pass doesn't know, which its statics can't join")
    names = operands(held[1]) if held else []
    names += [name for name in statics if name not in names]
    used = f'@llvm.compiler.used = appending global [{len(names)} x ptr] [{", ".join(names)}], section "llvm.metadata"'
    return text[:held.start()] + used + text[held.end():] if held else f'{text}\n{used}\n'


def main():
    path = Path(sys.argv[1])
    base = int(sys.argv[sys.argv.index('--base') + 1], 0) if '--base' in sys.argv else 0
    text = path.read_text(errors='surrogateescape')
    path.write_text(Pass(base, '--check' in sys.argv, '--host' in sys.argv).run(text, path), errors='surrogateescape')


if __name__ == '__main__':
    main()
