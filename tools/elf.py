"""Read ELF objects and links, as GNU as and ld write them, and the archives
GNU ar makes of objects.

read() gives an ELF file's sections, symbols and relocations, members() an
archive's members, and opened() an object or an archive, read once while
its real path, modification time and size hold. tools/spec.py,
tools/ld.py, tools/verify.py, tools/decls.py and tools/publish.py read make
gcc's objects and links through them, and tools/census.py its objects.
"""

import os
import struct
from collections import namedtuple

from tool import content, fail

# the section types read() treats apart: SHT_SYMTAB, SHT_RELA and SHT_NOBITS
SYMTAB, RELA, NOBITS = 2, 4, 8
# SHF_ALLOC, the flag of a section that takes memory as the image runs
ALLOC = 2
# the symbol bindings: STB_LOCAL, STB_GLOBAL and STB_WEAK
LOCAL, GLOBAL, WEAK = 0, 1, 2
# the symbol types that name a section and a source file: STT_SECTION and
# STT_FILE
SECTION_SYMBOL, FILE_SYMBOL = 3, 4
# the section indexes of an undefined symbol and an absolute one: SHN_UNDEF
# and SHN_ABS
UNDEFINED, ABSOLUTE = 0, 0xFFF1
# R_SH_DIR32, the relocation type that adds its symbol's value to the word
# it relocates
DIR32 = 1
# the bytes that open an archive
ARCHIVE = b'!<arch>\n'
# an archive member's header: the member's name, date, owner, group, mode
# and size, then the two bytes that end the header
MEMBER = struct.Struct('16s12s6s6s8s10s2s')

# each input opened() has read, as unpacked() gives it, by its stamp()
OPENED = {}
# an ELF file as read() gives it, and its parts. where names the file.
# relocations maps a section's index to the Relocations that apply to it.
# Each kind is the part's ELF type. A Section's align is 0 or 1 for none,
# and a NOBITS section's data is empty. A Symbol's section is a section's
# index or a reserved one, as ABSOLUTE is, and a Relocation's symbol is a
# symbol's index. The value of a symbol in a section and a relocation's
# offset are offsets into the section in an object, and addresses in a link
Elf = namedtuple('Elf', 'where sections symbols relocations')
Section = namedtuple('Section', 'index name kind flags address size align data')
Symbol = namedtuple('Symbol', 'name value size section kind bind')
Relocation = namedtuple('Relocation', 'offset symbol kind addend')


def as_c_name(name):
    """The C name a symbol's name spells: name less the object format's
    leading underscore, or name itself where it has none."""
    return name[1:] if name.startswith('_') else name


def allocated_sections(found):
    """The sections of found, an Elf, that take memory as the image runs,
    by index."""
    return {section.index: section for section in found.sections if section.flags & ALLOC}


def names_definition(symbol):
    """Whether symbol names a definition: whether its section index isn't
    SHN_UNDEF, and its type is any but STT_SECTION and STT_FILE, which name
    a section and a source file."""
    return symbol.section != UNDEFINED and symbol.kind not in (SECTION_SYMBOL, FILE_SYMBOL)


def read(data, where):
    """data, the bytes of the ELF file where names, as an Elf.

    It fails, naming where, on data that isn't a little-endian 32-bit ELF
    file. It fails on a section that runs past the file's end. It fails on a
    name that runs past its string table, as every name does in a string
    table with no bytes in the file. It fails on two lists of relocations
    for one section, and on a list for a section the file doesn't hold. It
    fails on a relocation against a symbol the file doesn't hold."""
    try:
        if data[:6] != b'\x7fELF\x01\x01':
            raise ValueError
        # the ELF header's e_shoff, e_shentsize, e_shnum and e_shstrndx
        *_, headers_at, _, _, _, _, size, count, names = struct.unpack_from('<16sHHIIIIIHHHHHH', data)
        headers = [struct.unpack_from('<10I', data, headers_at + size * index) for index in range(count)]
        if any(kind != NOBITS and offset + length > len(data) for _, kind, _, _, offset, length, *_ in headers):
            raise ValueError
        stored = [b'' if kind == NOBITS else data[offset:offset + length]
                  for _, kind, _, _, offset, length, *_ in headers]

        def text(table, offset):
            # the NUL-terminated string at offset in section table
            return stored[table][offset:stored[table].index(b'\0', offset)].decode('latin-1')

        sections = [Section(index, text(names, name), kind, flags, place, length, align, stored[index])
                    for index, (name, kind, flags, place, _, length, _, _, align, _) in enumerate(headers)]
        symbols, relocations = [], {}
        # a symbol table's link is its string table, and a list of
        # relocations' info is the section it applies to
        for (_, kind, _, _, _, _, strings, target, _, _), entries in zip(headers, stored):
            if kind == SYMTAB:
                symbols = [Symbol(text(strings, name), value, length, index, info & 0xF, info >> 4)
                           for name, value, length, info, _, index in struct.iter_unpack('<IIIBBH', entries)]
            elif kind == RELA:
                if target in relocations or target >= count:
                    raise ValueError
                relocations[target] = [Relocation(offset, info >> 8, info & 0xFF, added)
                                       for offset, info, added in struct.iter_unpack('<IIi', entries)]
        if any(relocation.symbol >= len(symbols) for listed in relocations.values() for relocation in listed):
            raise ValueError
    except (ValueError, struct.error, IndexError):
        fail(f"{where}: not an ELF file of GNU as's or ld's")
    return Elf(where, sections, symbols, relocations)


def members(data, where):
    """The bytes of each member of data, the archive where names, by its
    name, in file order. The archive is as GNU ar writes one. Its / member
    holds the symbol table, and its // member the names too long for a
    header. Neither is a member.

    It fails, naming where, where data doesn't start with ARCHIVE, or a
    member runs past its end. It fails on a header cut short, or one that
    doesn't end as GNU ar ends one. A header's size must be decimal digits.
    A long name runs from its offset in the // member to the next /\\n
    there, and an offset with none after it fails. Two members of one name
    fail too."""
    found, long_names, at = {}, b'', len(ARCHIVE)
    try:
        if not data.startswith(ARCHIVE):
            raise ValueError
        while at < len(data):
            name, _, _, _, _, size, end = MEMBER.unpack_from(data, at)
            if end != b'`\n' or not size.rstrip().isdigit():
                raise ValueError
            name, size, at = name.decode('latin-1').rstrip(), int(size), at + MEMBER.size
            # a member starts on an even offset
            body, at = data[at:at + size], at + size + size % 2
            if len(body) != size:
                raise ValueError
            if name == '//':
                long_names = body
                continue
            if name == '/':
                continue
            if name[1:].isdigit():
                start = int(name[1:])
                name = long_names[start:long_names.index(b'/\n', start)].decode('latin-1')
            name = name.rstrip('/')
            if name in found:
                fail(f'{where}: holds two members named {name}')
            found[name] = body
    except (ValueError, struct.error):
        fail(f"{where}: not an archive of GNU ar's")
    return found


def stamp(path):
    """The key an input at path is read once by: its real path,
    modification time and size. An input it can't stat fails, naming it."""
    try:
        status = os.stat(path)
    except OSError as error:
        fail(f'{path}: {error.strerror}')
    return os.path.realpath(path), status.st_mtime_ns, status.st_size


def unpacked(path):
    """The input at path, as a list of (member, Elf). An object gives one,
    with member None. An archive gives one for each member, in file order,
    with member its name. It fails as content(), members() and read() do."""
    data = content(path)
    if data.startswith(ARCHIVE):
        return [(name, read(body, f'{path}({name})')) for name, body in members(data, path).items()]
    return [(None, read(data, path))]


def opened(path):
    """unpacked(path), read once while the input's stamp() holds. It fails
    as stamp() and unpacked() do."""
    key = stamp(path)
    if key not in OPENED:
        OPENED[key] = unpacked(path)
    return OPENED[key]


def held(key):
    """The input whose stamp() is key, as opened() has read it, or None
    where opened() hasn't read it."""
    return OPENED.get(key)
