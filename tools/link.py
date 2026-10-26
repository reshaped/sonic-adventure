"""Link an object where its image's tables place its source, with R32's
asmsh, lnk and cnvs.

The linked source is the bytes of the source's ranges, joined in table
order, but for 1ST_READ.BIN's bss, which its file doesn't store. A byte of
those ranges that the object leaves unwritten is the image's fill byte, or 0
in bss. A byte the object writes outside the source's ranges, or in
1ST_READ.BIN's bss, fails the link.

The link writes the linked source to the source's path under build/IMAGE/,
or under build/stub/IMAGE/ for a stub, with .bin for its suffix. The link's
intermediate files go beside the linked source. WIBO and R32 in the
environment, which the Makefile exports, name wibo and R32's directory, by
default tools/wibo-i686 and tools/shc-v5.0r32.

The link defines each name the object takes from outside at the address the
image's tables give it (Config.symbols). A generated source's .syms file may
add names or repeat bindings, but cannot change an existing name's address.
An address inside the source's ranges is the source's to define. A name the
object takes from outside binds there only as a second name that
SECOND_NAMES admits, at an address the tables give more than one name. A
name the object takes from outside that the link leaves unbound, such as one
the tables don't give, fails the link.

Each of the object's sections starts at its first range. A section that
holds bytes and has no range fails the link. An empty section with no range
starts at the extent tools/config.py gives it, if any, and lnk places one
with neither. Sections that start at one address start there in the
extents' order, then any the extents don't give. Where a section starts at
its extent's start and the extent has an end, the link reserves the rest of
the extent after the object's bytes. It reserves that space in a section of
the same name and kind, code or data, which lnk joins to the object's
section. STARTOF and SIZEOF then give the section-boundary tables the
image's bounds. TOOLCHAIN.md#image-tables gives the rules the source's
ranges keep.
"""

import argparse
import contextlib
import os
import re
import signal
import subprocess
import sys
from collections import Counter
from pathlib import Path, PureWindowsPath

from config import ZEROED, Config, aligned, configs, content, fail, hexa, named, save

WIBO = os.environ.get('WIBO', 'tools/wibo-i686')
R32 = Path(os.environ.get('R32', 'tools/shc-v5.0r32'))
# seconds before run() takes a tool for hung and stops it, as the Makefile's
# wibo does: a tool under wibo now and then deadlocks
HUNG = 600
# seconds a tool that run() stops has to exit on TERM before KILL, as with
# the Makefile's GRACE
GRACE = 10
SRECORD = re.compile(r'S[0-9](?:[0-9A-F]{2})+')
# the address width in bytes of each S-record kind cnvs writes, the only
# kinds srecords() takes. S0, the header, leaves the address unused. S1 to
# S3 write their data starting at it. S7 to S9, the end records, hold the
# start address alone.
ADDRESS_WIDTH = {0: 2, 1: 2, 2: 3, 3: 4, 7: 4, 8: 3, 9: 2}
# SHC object record kinds: sections, external references and end of object
SECTIONS, REFERENCES, END = 0x08, 0x0C, 0x7F
# the section tables that section() has read, by object path
OBJECTS = {}
# second names a source declares extern for one of its own functions, so
# that SHC loads the function's address from a second pool word. SHC pools
# each name once.
SECOND_NAMES = {'casino_ornament_step_still'}


def find(config, stem):
    """The source in config.sources() whose path, less its suffix, is stem, or None."""
    return next((source for source in config.sources() if Path(source).with_suffix('').as_posix() == stem), None)


def linked(source, image, root='build'):
    """Where the link writes source's linked source for image: the source's
    path under root/image, with .bin for its suffix. root is build, or
    build/stub for the stub build."""
    return Path(root, image, source).with_suffix('.bin')


def dos(path):
    # asmsh, lnk and cnvs interpret '/' as an option prefix
    return str(PureWindowsPath(path))


def stop(tool):
    """End tool's process group. TERM goes to the whole group. KILL then goes
    to what is left of it once tool exits, GRACE seconds pass or this process
    is interrupted."""
    try:
        os.killpg(tool.pid, signal.SIGTERM)
        tool.wait(GRACE)
    except (ProcessLookupError, subprocess.TimeoutExpired):
        pass
    finally:
        with contextlib.suppress(ProcessLookupError):
            os.killpg(tool.pid, signal.SIGKILL)


def run(*argv, stdin=None):
    """Run the tool at argv[0] under wibo, with the rest of argv as its
    arguments and stdin, if given, as its input.

    The run fails where wibo can't be run, and where the tool exits nonzero
    or writes a message. A tool that runs HUNG seconds is stopped, and the
    run fails."""
    name = Path(argv[0]).name
    # In a session of its own, the tool and what it starts form a process
    # group, which stop() can end without signalling make's
    try:
        tool = subprocess.Popen([WIBO, *map(str, argv)], stdin=None if stdin is None else subprocess.PIPE,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors='replace',
                                start_new_session=True)
    except OSError as error:
        fail(f"{WIBO}: can't run it: {error.strerror}")
    with tool:
        try:
            output = tool.communicate(stdin, timeout=HUNG)[0]
        except subprocess.TimeoutExpired:
            stop(tool)
            fail(f'{name} ran {HUNG} seconds and was stopped: wibo can deadlock, so run make again')
        except BaseException:
            # ^C, or a signal that stoppable() turns into SystemExit: each reaches this process, not the tool
            stop(tool)
            raise
    # lnk and asmsh exit 0 after a warning, so a message in either form fails
    # the run whatever the status: lnk's and cnvs's messages start '** ', and
    # asmsh marks its with (E), (W) or (F).
    problems = [line.strip(': ') for line in output.splitlines() if re.search(r'\*\* |\([EWF]\) ', line)]
    if tool.returncode or problems:
        status = [f'{name} exited with status {tool.returncode}'] if tool.returncode else []
        fail(*(problems or output.splitlines()), *status)


def srecords(path):
    """The bytes the S-records at path write, by address.

    A line fails where it isn't an S-record, its count or checksum is wrong,
    its kind is one cnvs doesn't write or it has no room for its address. The
    file fails unless it ends with one end record, S7 to S9, that holds its
    address alone. A second write to one address fails too."""
    written, ended = {}, False
    for number, record in enumerate(content(path).decode().splitlines(), 1):
        where = f'{path}:{number}'
        if ended or not SRECORD.fullmatch(record):
            fail(f'{where}: {"a record after the end" if ended else "not an S-record"}')
        raw = bytes.fromhex(record[2:])
        if raw[0] != len(raw) - 1 or sum(raw) & 0xFF != 0xFF:
            fail(f'{where}: its count or checksum is wrong')
        kind = int(record[1])
        if kind not in ADDRESS_WIDTH:
            fail(f'{where}: S{kind}, a record cnvs doesn\'t write')
        # raw holds the count, the address, any data and the checksum
        width = ADDRESS_WIDTH[kind]
        if len(raw) < width + 2:
            fail(f'{where}: S{kind} too short for its {width}-byte address')
        if kind in (7, 8, 9) and len(raw) > width + 2:
            fail(f'{where}: S{kind} with bytes after its {width}-byte address')
        if kind in (1, 2, 3):
            address = int.from_bytes(raw[1:width + 1], 'big')
            for offset, byte in enumerate(raw[width + 1:-1], address):
                if offset in written:
                    fail(f'{where}: writes {hexa(offset)} again')
                written[offset] = byte
        elif kind in (7, 8, 9):
            ended = True
    if not ended:
        fail(f'{path}: no end record')
    return written


def records(obj):
    """Each of the object's records, as (offset, bytes), in file order
    through the record that ends the object. A record fails where it is cut
    short, holds under 3 bytes or has a wrong checksum. Bytes after the end
    record fail too, as does an object without one."""
    # a record is a kind byte, a size byte that counts the whole record, the
    # body, and a checksum byte that brings the record's sum to 0xff. Bit
    # 0x80 of the kind ends a run of records, and the run of kind END ends
    # the object.
    data = content(obj)
    at = 0
    while at < len(data):
        if at + 2 > len(data) or at + data[at + 1] > len(data):
            fail(f'{obj}: the record at {at:#x} is cut short')
        record = data[at:at + data[at + 1]]
        if len(record) < 3:
            fail(f'{obj}: the record at {at:#x} has size {len(record)}')
        if sum(record) & 0xFF != 0xFF:
            fail(f'{obj}: the record at {at:#x} fails its checksum')
        yield at, record
        at += len(record)
        if record[0] == 0x80 | END:
            if at < len(data):
                fail(f'{obj}: bytes follow its end, at {at:#x}')
            return
    fail(f'{obj}: no end record')


def runs(obj):
    """Each run of records of one kind, as (offset, kind, body): the offset
    of the run's first record, the kind without bit 0x80, and the bodies of
    the run's records joined. A record of another kind inside a run fails."""
    # a bytearray grows in place, where bytes would copy the run so far at each record
    start, kind, body = 0, None, bytearray()
    for at, record in records(obj):
        this = record[0] & 0x7F
        if kind not in (None, this):
            fail(f'{obj}: the record at {at:#x} breaks a run of kind {kind:#04x}')
        kind = this
        body += record[2:-1]
        if record[0] & 0x80:
            yield start, kind, bytes(body)
            start, kind, body = at + len(record), None, bytearray()


def references(obj):
    """The names of the object's external references, as it spells them: a C
    name with its leading underscore. A references run that doesn't parse
    fails."""
    # an entry is 0xc0, the name's length and the name, and may cross a
    # record boundary within the run
    names = set()
    for start, kind, body in runs(obj):
        if kind != REFERENCES:
            continue
        at = 0
        while at < len(body):
            if at + 2 > len(body) or body[at] != 0xC0 or (end := at + 2 + body[at + 1]) > len(body):
                fail(f'{obj}: the references at {start:#x} don\'t parse')
            names.add(body[at + 2:end].decode('latin-1'))
            at = end
    return names


def section_table(obj):
    """The object's sections by name, each as (size, alignment, code). A
    sections run that doesn't parse fails, as do a name the table gives twice
    and an alignment not a power of two."""
    # a section entry is 16 bytes of attributes, size and alignment, then the
    # name's length and the name. The size is the big-endian 32-bit word at
    # offset 5, the alignment the one at 9, and bit 0x10 of byte 13 marks
    # data, not code.
    found = {}
    for start, kind, body in runs(obj):
        if kind != SECTIONS:
            continue
        at = 0
        while at < len(body):
            if at + 17 > len(body) or (end := at + 17 + body[at + 16]) > len(body):
                fail(f'{obj}: the sections at {start:#x} don\'t parse')
            name, alignment = body[at + 17:end].decode('latin-1'), int.from_bytes(body[at + 9:at + 13], 'big')
            if name in found:
                fail(f'{obj}: the sections at {start:#x} name {name} again')
            if not alignment or alignment & (alignment - 1):
                fail(f'{obj}: the sections at {start:#x} align {name} to {alignment}, not a power of two')
            found[name] = (int.from_bytes(body[at + 5:at + 9], 'big'), alignment, not body[at + 13] & 0x10)
            at = end
    return found


def section(source, name):
    """The (size, alignment, code) that source's object declares for the
    section called name. size includes the bytes the object leaves
    unwritten, and code is true for a code section.

    The object is the one the build compiled, under build/, or the stub the
    disc-free build compiled in its place, under build/stub/. Where both
    stand and declare the section differently, it is the one built later,
    since make rebuilds the one it checks whenever the source changes. Each
    object's section table is read once, into OBJECTS.

    A source with neither object fails, as do two objects that declare the
    section differently with neither newer, and an object without the
    section."""
    found = [obj for obj in (Path('build', source).with_suffix('.obj'), Path('build/stub', source).with_suffix('.obj'))
             if obj.exists()]
    if not found:
        fail(f'{source}: no object under build/ or build/stub/, so build it first')
    for obj in found:
        if obj not in OBJECTS:
            OBJECTS[obj] = section_table(obj)
    if len(found) == 2 and OBJECTS[found[0]].get(name) != OBJECTS[found[1]].get(name):
        built, stub = (obj.stat().st_mtime_ns for obj in found)
        if built == stub:
            fail(f'{found[0]} and {found[1]} name {name} differently and neither is newer; rebuild one')
        found = [found[0] if built > stub else found[1]]
    obj = found[0]
    if name not in OBJECTS[obj]:
        fail(f'{obj}: no {name} section')
    return OBJECTS[obj][name]


def symbols(config, *syms):
    """The image's (address, name) pairs, as config.symbols() gives them,
    then the pairs the .syms files of syms add, file by file. A .syms pair
    adds its name where no earlier pair gives it, and a repeat of an earlier
    pair adds nothing. A .syms pair that gives an earlier pair's name another
    address fails."""
    names = config.symbols()
    known = {name: address for address, name in names}
    for path in syms:
        for address, name in named(path):
            if name not in known:
                known[name] = address
                names.append((address, name))
            elif known[name] != address:
                fail(f'{path}: {name} is at {hexa(address)}, and the tables put it at {hexa(known[name])}')
    return names


def exported(names, used, ranges):
    """The (address, name) pairs in names that the object takes from outside,
    in names' order. used is the set of the object's references, as
    references() spells them. ranges are the source's ranges.

    A pair counts where _name is in used and either its address lies outside
    every range, or its name is in SECOND_NAMES and another pair in names
    has its address."""
    counts = Counter(address for address, _ in names)
    return [(address, name) for address, name in names
            if f'_{name}' in used and ((name in SECOND_NAMES and counts[address] > 1)
                                       or not any(start <= address < end for _, start, end in ranges))]


def layout(source, ranges, extents, declared):
    """Where the link of source's object starts each of the object's
    sections that has a row or an extent, and the space it reserves after
    the object's bytes, as a pair of lists. The starts are (address, names)
    in address order, as grouped() gives them. The reserved space is (name,
    size, alignment, code), in extents' order.

    ranges are source's rows, as (section, start, end) in table order.
    extents are the image's, as Config.extents() gives them. declared is the
    object's section table, as section_table() gives it. The module
    docstring gives the rules for each start and each reservation.

    Each of these fails, naming source: a row of a section the object
    doesn't declare; a section with bytes and no row; a section whose highest
    row doesn't end where the object's section does; and a section that
    starts at its extent and runs past the extent's end."""
    starts, ends = {}, {}
    for name, start, end in ranges:
        if name not in declared:
            fail(f'{source}: its {name} row names no section of its object')
        starts.setdefault(name, start)
        ends[name] = max(end, ends.get(name, end))
    # a row places each section that holds bytes, and the section's highest
    # row ends where the object's section does: the section's bytes are the
    # object's, unwritten ones included, and what follows is the linker's
    for name, (size, _, _) in declared.items():
        if name not in starts:
            if size:
                fail(f'{source}: its object\'s {name} holds {size:#x} bytes, and no row places it')
        elif ends[name] != starts[name] + size:
            fail(f'{source}: its {name} row ends at {hexa(ends[name])}, and its object\'s {name} at '
                 f'{hexa(starts[name] + size)}')
    reserved = []
    for name, start, end in extents:
        if name not in declared:
            continue
        starts.setdefault(name, start)
        if starts[name] != start or end is None:
            continue
        size, alignment, code = declared[name]
        # lnk starts the reserved space on the section's alignment, after the object's bytes
        after = aligned(start + size, alignment)
        if end < start + size:
            fail(f'{source}: its {name} runs past {hexa(end)}, where tools/config.py ends {name}')
        if after < end:
            reserved.append((name, end - after, alignment, code))
    return grouped(starts, extents), reserved


def grouped(starts, extents):
    """The sections of starts, a {name: start} map, grouped by start, as a
    list of (address, names) in address order. A group lists the sections
    the extents give in the extents' order, then any others. lnk starts a
    group's sections at its address one after another, in that order."""
    rank = {name: index for index, (name, _, _) in enumerate(extents)}
    found = {}
    for name, start in sorted(starts.items(), key=lambda item: (item[1], rank.get(item[0], len(rank)))):
        found.setdefault(start, []).append(name)
    return list(found.items())


def define(syms, names, reserved):
    """The path of the object assembled from syms: syms with .obj for its
    suffix.

    The object exports each of names, (address, name), as _name at that
    address, in an empty SYMBOLS section. It reserves each of reserved,
    (name, size, alignment, code), as size bytes on alignment in a section of
    that name, code where code is true and data otherwise.

    define() writes the object's assembly text to syms, then assembles it
    with asmsh. define() fails where syms can't be written, and as run()
    does."""
    lines = ['        .SECTION SYMBOLS,DATA,ALIGN=4']
    for address, name in names:
        lines += [f'        .EXPORT _{name}', f"_{name}: .EQU H'{address:08x}"]
    for name, size, alignment, code in reserved:
        lines += [f'        .SECTION {name},{"CODE" if code else "DATA"},ALIGN={alignment}', f'        .RES.B {size}']
    save(syms, '\n'.join([*lines, '        .END', '']).encode())
    obj = syms.with_suffix('.obj')
    run(R32 / 'asmsh.exe', dos(syms), f'-object={dos(obj)}', '-cpu=sh4', '-endian=little')
    return obj


def place(objects, starts, out, *subcommands):
    """The bytes a link of objects writes, by address.

    lnk links objects in their order and starts each group of starts,
    (address, names), at its address. subcommands are lnk commands, run
    before it starts the sections. cnvs then converts the link to S-records,
    which srecords() reads. lnk's output is out with .abs for its suffix, and
    cnvs's is out with .mot.

    place() fails as run() and srecords() do."""
    absolute, records = out.with_suffix('.abs'), out.with_suffix('.mot')
    # leading zeroes keep the addresses numeric in the linker command
    placing = ','.join(f'{",".join(group)}({address:08x})' for address, group in starts)
    commands = [*(f'input {dos(obj)}' for obj in objects), f'output {dos(absolute)}', 'form a', *subcommands,
                f'start {placing}', 'exit', '']
    run(R32 / 'lnk.exe', '-noecho', stdin='\n'.join(commands))
    run(R32 / 'cnvs.exe', dos(absolute), dos(records))
    return srecords(records)


def joined(config, source, written):
    """source's linked source for config's image: the bytes of the ranges
    config.held() gives source, joined in table order. written is the bytes
    the link wrote, by address, as place() gives them.

    A byte the link wrote comes from written. Where it wrote none, the byte
    is the image's fill byte, or 0 in bss.

    An address in written outside source's ranges fails. config.ranges()
    gives those ranges, and they include 1ST_READ.BIN's bss, which held()
    leaves out of the linked source. A written address in that bss fails
    too, since the linked source can't hold its byte."""
    ranges, held = config.ranges(source), config.held()[source]
    stray = [address for address in written if not any(start <= address < end for _, start, end in ranges)]
    if stray:
        fail(f'{source}: {len(stray)} bytes at {hexa(min(stray))} fall outside its ranges in {config.splits}')
    unstored = [address for address in written if not any(start <= address < end for _, start, end in held)]
    if unstored:
        fail(f"{source}: {len(unstored)} bytes at {hexa(min(unstored))} fall in 1ST_READ.BIN's bss, which its file "
             f"doesn't store")
    return b''.join(bytes(written.get(address, 0 if name in ZEROED else config.fill) for address in range(start, end))
                    for name, start, end in held)


def main(argv):
    parser = argparse.ArgumentParser(prog='link', description=__doc__.split('\n\n')[0])
    parser.add_argument('obj', metavar='OBJECT', type=Path,
                        help="a source's path under build/, or under build/stub/ for a stub, with .obj for its suffix")
    parser.add_argument('image', metavar='IMAGE', nargs='?',
                        help='the image to link for, config/IMAGE/; without it, the one image that places the source')
    args = parser.parse_args(argv)
    obj, image = args.obj, args.image
    if not obj.is_file():
        fail(f'{obj}: no such object')
    try:
        under = obj.resolve().relative_to(Path('build').resolve())
    except ValueError:
        fail(f'{obj}: not under build/')
    # lnk rejects an absolute path, so the commands name the object by its path under build/, as make does
    obj = Path('build', under)
    stub = under.parts[0] == 'stub'
    stem = (under.relative_to('stub') if stub else under).with_suffix('').as_posix()
    if image is None:
        found = [config for config in configs() if find(config, stem)]
        if len(found) != 1:
            fail(f'{stem}: {len(found) or "no"} splits.txt list it, so name the image')
        image = found[0].name
    config = Config(image)
    source = find(config, stem)
    if source is None:
        fail(f'{stem}: not in {config.splits}')
    out = linked(source, image, 'build/stub' if stub else 'build')
    try:
        out.parent.mkdir(parents=True, exist_ok=True)
    except OSError as error:
        # out's directory, or one above it, whichever mkdir couldn't make
        fail(f'{error.filename}: {error.strerror}')
    ranges = config.ranges(source)
    starts, reserved = layout(source, ranges, config.extents(), section_table(obj))

    names = symbols(config, *([obj.with_suffix('.syms')] if source.startswith('gen/') else []))
    # the tools allow one dot in a file name
    syms = define(out.with_name(out.stem + '_syms.src'), exported(names, references(obj), ranges), reserved)
    save(out, joined(config, source, place([obj, syms], starts, out)))


def stoppable():
    """Have a hangup, ^\\ or TERM raise SystemExit with status 128 plus the
    signal's number, as ^C raises KeyboardInterrupt. run() then stops its
    tool on any of them. Each reaches this process, in make's process group,
    but not the session that run() starts a tool in. A signal ignored at the
    start, as under nohup, stays ignored."""
    for number in (signal.SIGHUP, signal.SIGQUIT, signal.SIGTERM):
        if signal.getsignal(number) != signal.SIG_IGN:
            signal.signal(number, lambda received, _: sys.exit(128 + received))


if __name__ == '__main__':
    stoppable()
    main(sys.argv[1:])
