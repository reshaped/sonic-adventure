"""The native slice's census: what in the game's C and assembly stands
between it and a 64-bit host, by kind, with counts and examples.

census.py CC OUT [SOURCE ...] compiles each C source under src/, modules/,
events/ and sdk/ but each SOURCE, C that only make dc compiles and the
slice never builds, with CC for the host, as the slice does: with
NONMATCHING defined and not DREAMCAST, so the non-matching side, with
intrinsics.h's declarations of SHC's intrinsics and none of the SH-4's own
instructions. It keeps the diagnostics of the warnings that mark a 32-bit
assumption, reads the sources and the .src files for the rest, and writes
the census to OUT.
"""

import re
import subprocess
import sys
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOTS = ['src', 'modules', 'events', 'sdk']
# the game's own include route, and the SDK's, as the Makefile's GAME_INC and
# SDK_INC give them, with build/gen/'s bodies, and where model.c and pace.c
# find the backend's headers, SA1's adapter's for the walk and the
# Dreamcast's for the frame's end, which the slice names the PC's, and
# model.c Ninja's Kamui words by their path from the root
GAME_INC = ['sdk/include', 'src', 'modules', 'events', 'build/gen/src', 'build/gen/modules', 'build/gen/events',
            'build/gen/sdk', 'engine', 'engine/include', 'engine/dc', '.']
SDK_INC = ['sdk/include', 'sdk', 'modules', '.', 'build/gen/sdk', 'build/gen/modules', 'build/gen/loader']
# the warnings that mark a 32-bit assumption. return-mismatch's `return;` in
# a function of a value is SHC's C, which compile_flags.txt keeps a warning
WARNINGS = ['-Wno-everything', '-Wno-error=return-mismatch', '-Wpointer-to-int-cast', '-Wvoid-pointer-to-int-cast',
            '-Wint-to-pointer-cast', '-Wint-to-void-pointer-cast', '-Wint-conversion', '-Wshorten-64-to-32',
            '-Wincompatible-pointer-types', '-Wimplicit-function-declaration']
DIAGNOSTIC = re.compile(r'^(?P<path>[^:\s]+):(?P<line>\d+):\d+: (?P<level>warning|error): (?P<text>.*?)'
                        r'(?: \[(?P<flag>-W[\w-]+)\])?$')
# the number of examples each kind shows
EXAMPLES = 3


def sources(left_out):
    return sorted(p for root in ROOTS for p in Path(root).rglob('*.c') if p.as_posix() not in left_out)


def compile_one(cc, path, resource):
    include = SDK_INC if path.parts[0] == 'sdk' else GAME_INC
    command = [cc, '-std=gnu89', '-ffreestanding', '-nostdinc', '-isystem', f'{resource}/include', '-fsyntax-only',
               '-DNONMATCHING', '-include', 'engine/native/intrinsics.h', *WARNINGS, *(f'-I{d}' for d in include),
               str(path)]
    run = subprocess.run(command, capture_output=True, text=True)
    found = [m.groupdict() for m in map(DIAGNOSTIC.match, run.stderr.splitlines()) if m]
    return path, run.returncode, found


def line_of(path, number, cache={}):
    if path not in cache:
        cache[path] = Path(path).read_text(errors='replace').splitlines()
    return cache[path][int(number) - 1].strip()


# a pointer that becomes an int, by what the int then does
def cast_use(text):
    if re.search(r'\bsound_\w+\s*\(', text):
        return "a pointer as a sound's owner"
    if re.search(r'\bva_(arg|start)\b', text):
        return "SHC's va_list, a pointer stepped as an int"
    if re.search(r'[&%]\s*(~|0x1f\b|31\b|32\b|0x3f\b|0xff\b|\()', text):
        return 'an address aligned or masked as a number'
    if re.search(r'\b(id|key|owner|seed|hash)\b', text):
        return 'a pointer as an identity'
    return 'a pointer held or compared as an int'


# a 64-bit value that becomes 32 bits, by where it came from. sg_xpt.h's
# Uint32 and Sint32 are long, and so Angle, and kamui.h's KMDWORD: the
# prelude gives the game's their SH-4 width. the compiler words a pointer
# difference as a long, and a value a source declares long the same way
def truncation_use(text):
    if re.search(r"\(aka '(const )?(unsigned )?long'\)", text):
        return "an SDK type of long, 64 bits on the host: Sint32, Uint32, Angle, KMDWORD"
    if "'unsigned long' to" in text or "'size_t'" in text:
        return 'a size_t into 32 bits: sizeof, strlen'
    if "'long' to" in text:
        return 'a long into 32 bits: a pointer difference, or a value declared long'
    return 'another 64-bit value into 32 bits'


# a constant address, by what lives there on the Dreamcast
def region(address):
    top = address >> 24
    if top in (0x0c, 0x8c, 0xac):
        return 'main RAM'
    if top in (0x04, 0x05, 0xa4, 0xa5, 0x10, 0x11):
        return "video RAM or the TA's FIFO"
    if top in (0x00, 0x80, 0xa0):
        return 'boot ROM, flash, or a register of the system bus, PVR, AICA or G1/G2'
    if top == 0xe0:
        return 'the store queues'
    if top in (0xff, 0x1f):
        return "the SH-4's on-chip registers"
    return 'other'


# an address ORed into P2, the SH-4's uncached view of memory
P2_OR = re.compile(r'0xa0000000U?L?\s*\|\s*\(Uint32\)|\(Uint32\)\(?\w+\)?\s*\|\s*0xa0000000')
ADDRESS_CAST = re.compile(r'\(\s*(?:(?:const|volatile|unsigned|signed|struct)\s+)*\w+(?:\s+\w+)*\s*\*+\s*\)\s*\(?'
                          r'\s*0x([0-9a-fA-F]{5,8})\b')
INLINE_ASM = re.compile(r'^\s*#pragma\s+inline_asm\s*\(\s*(\w+)')
PRAGMA_ASM = re.compile(r'^\s*#pragma\s+asm\b')
EXPORT = re.compile(r'^\s*\.EXPORT\s+_?(\w+)', re.I)
INTRINSIC = re.compile(r'\b(_builtin_\w+|set_imask|get_imask|get_vbr|set_fpscr|get_fpscr|prefetch|st_ext|ld_ext|'
                       r'trapa|tas)\s*\(')
PACK = re.compile(r'#pragma\s+pack\b|__attribute__\s*\(\(\s*packed')
ALIGNED = re.compile(r'__attribute__\s*\(\(\s*aligned')
SIZE_ASSERT = re.compile(r'_Static_assert\s*\(\s*sizeof')
DOUBLE = re.compile(r'\bdouble\b')
# a floating constant with no F, a double on the host
DOUBLE_CONSTANT = re.compile(r'(?<![\w.])(\d+\.\d*|\.\d+)([eE][+-]?\d+)?(?![\w.])')
TAG = re.compile(r'@bug (uninitialized|memory)\b')
# a kind's own struct laid over a record of a task's, whose words agree with
# the record's, and with each other overlay's, only where pointers are 32
# bits: a path task writes its path through path_vars at vars + 8, and
# Emerald Coast's camera reads it through its work's struct at + 16 on the
# host
OVERLAY = re.compile(r'#define\s+\w+\(\w+\)\s+\(\(struct \w+\s*\*\)\s*&?\(\w+\)->(work|vars|motion|force|extra)\)')
# a record walked as an array of pointers, which steps by a Uint32 field's
# width only where pointers are 32 bits, as player_anim_free_blend walks
# NJS_MDATA's pointers and counts
WORDS = re.compile(r'\(void \*\*\)')
# a struct's definition, and a member that is a pointer
STRUCT = re.compile(r'struct (\w+)\s*\{(.*?)\n\};', re.S)
POINTER = re.compile(r'\*\s*\w+\s*[\[;,]|\(\s*\*\s*\w+\s*\)')
# a pointer kept in the place of a field of another type
PARKED = re.compile(r'\*\(\s*(?:struct\s+)?\w+\s*\*\*\s*\)\s*&')
# a line of a block comment: one that opens it, or one that goes on with a *
# before a space, a / or the line's end, as the tree's comments run. a line
# that opens with a dereference is code
COMMENT = re.compile(r'^\s*(?:/\*|\*(?:[\s/]|$))')


def side(path):
    return 'SDK' if Path(path).parts[0] == 'sdk' else 'game'


class Census:
    def __init__(self):
        self.kinds = defaultdict(list)

    def add(self, kind, where, text):
        self.kinds[kind].append((where, text))

    def write(self, out, header):
        lines = [header, '']
        group = None
        for kind in sorted(self.kinds, key=lambda k: (k.split(':')[0], -len(self.kinds[k]), k)):
            head = kind.split(':')[0]
            if head != group:
                lines.append(head)
                group = head
            rows = self.kinds[kind]
            sides = Counter(side(w) for w, _ in rows)
            split = ', '.join(f'{n} {s}' for s, n in sorted(sides.items()))
            lines.append(f'  {len(rows):6}  {kind.split(":", 1)[1].strip()} ({split})')
            for where, text in rows[:EXAMPLES]:
                lines.append(f'          {where}: {text[:100]}')
        Path(out).write_text('\n'.join(lines) + '\n')


def main(cc, out, *left_out):
    census = Census()
    files = sources(set(left_out))
    # the non-matching code takes the compiler's own stddef.h and stdint.h
    resource = subprocess.run([cc, '-print-resource-dir'], capture_output=True, text=True, check=True).stdout.strip()
    with ThreadPoolExecutor(8) as pool:
        results = list(pool.map(lambda p: compile_one(cc, p, resource), files))
    failed = 0
    for path, code, found in results:
        failed += code != 0
        for d in found:
            where = f'{d["path"]}:{d["line"]}'
            flag = d['flag'] or ''
            text = line_of(d['path'], d['line'])
            if d['level'] == 'error' and flag != '-Wreturn-mismatch':
                census.add('compile: an error on the host', where, d['text'])
            elif flag == '-Wreturn-mismatch':
                census.add("compile: SHC's C: `return;` in a function of a value", where, text)
            elif flag in ('-Wpointer-to-int-cast', '-Wvoid-pointer-to-int-cast'):
                census.add(f'pointer width: {cast_use(text)}', where, text)
            elif flag in ('-Wint-to-pointer-cast', '-Wint-to-void-pointer-cast'):
                census.add('pointer width: a 32-bit int made a pointer', where, text)
            elif flag == '-Wshorten-64-to-32':
                census.add(f'pointer width: {truncation_use(d["text"])}', where, text)
            elif flag:
                census.add(f'compile: {flag[2:]}', where, d['text'])
    headers = sorted(p for root in ROOTS for p in Path(root).rglob('*.h'))
    for path in files + headers:
        source = path.suffix == '.c'
        lines = path.read_text(errors='replace').splitlines()
        for number, text in enumerate(lines, 1):
            where = f'{path}:{number}'
            if COMMENT.match(text):
                if source and TAG.search(text):
                    census.add(f'numbers and UB: @bug {TAG.search(text)[1]}, a read the host may answer otherwise',
                               where, text.strip())
                continue
            # a header's declarations of the intrinsics and the inline
            # assembly aren't uses; its layouts and addresses are
            if source and (m := INLINE_ASM.match(text)):
                census.add('assembly: an inline_asm function, its body SH-4 code', where, m[1])
            elif source and PRAGMA_ASM.match(text):
                census.add('assembly: a #pragma asm block', where, text.strip())
            for m in INTRINSIC.finditer(text) if source else ():
                census.add(f'intrinsics: {m[1].removeprefix("_builtin_")}', where, text.strip())
            if P2_OR.search(text):
                census.add("addresses: a function's or buffer's own address ORed into P2", where, text.strip())
            for m in ADDRESS_CAST.finditer(text):
                census.add(f'addresses: a constant address in {region(int(m[1], 16))}', where, text.strip())
            if m := OVERLAY.search(text):
                census.add(f"layout: a kind's struct laid over a task's {m[1]}", where, text.strip())
            if source and WORDS.search(text):
                census.add('layout: a record walked as an array of pointers', where, text.strip())
            if PARKED.search(text):
                census.add('layout: a pointer kept in the place of a field of another type', where, text.strip())
            if PACK.search(text):
                census.add('layout: a packed layout', where, text.strip())
            if ALIGNED.search(text):
                census.add('layout: an alignment attribute', where, text.strip())
            if SIZE_ASSERT.search(text):
                census.add('layout: a size assert', where, text.strip())
            if source and path.parts[0] != 'sdk' and '"' not in text and '//' not in text:
                code = text.split('/*')[0]
                if DOUBLE.search(code) or DOUBLE_CONSTANT.search(code):
                    census.add("numbers and UB: a double or a constant without F, a float under SHC's "
                               '-double=float', where, text.strip())
    # each struct an overlay lays over a task's record, once, by whether it
    # holds a pointer: those that do move their fields on the host. several
    # sources name a struct of their own alike, so a macro's struct is the
    # one its own file defines, and else the first that another file does
    bodies, macros, overlaid = defaultdict(dict), [], {}
    for path in files + headers:
        if path.parts[0] == 'sdk':
            continue
        text = path.read_text(errors='replace')
        for m in STRUCT.finditer(text):
            bodies[m[1]].setdefault(str(path), re.sub(r'/\*.*?\*/', '', m[2], flags=re.S))
        for m in OVERLAY.finditer(text):
            name = re.search(r'struct (\w+)', m[0])[1]
            macros.append((name, str(path), m[1], f'{path}:{text.count(chr(10), 0, m.start()) + 1}'))
    for name, path, record, where in macros:
        home = path if path in bodies[name] else min(bodies[name], default='')
        overlaid.setdefault((name, home, record), where)
    for (name, home, record), where in sorted(overlaid.items()):
        if POINTER.search(bodies[name].get(home, '')):
            census.add(f"layout: a struct laid over a task's {record} that holds a pointer, so its fields move on "
                       'the host', where, f'struct {name}')
    for path in sorted(p for root in ROOTS for p in Path(root).rglob('*.src')):
        lines = path.read_text(errors='replace').splitlines()
        exports = [m[1] for m in map(EXPORT.match, lines) if m]
        census.add('assembly: a .src file, its functions SH-4 code', str(path),
                   f'{len(lines)} lines, {len(exports)} exports: {" ".join(exports[:6])}')
    header = (f'census of {len(files)} C sources under {", ".join(ROOTS)} by {cc} for the host; '
              f'{failed} fail to compile')
    census.write(out, header)
    return 0


if __name__ == '__main__':
    sys.exit(main(*sys.argv[1:]))
