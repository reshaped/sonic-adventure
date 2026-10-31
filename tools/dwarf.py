"""Read the DWARF of make gcc's objects for decls.py: each compilation
unit's entries, and the files its line table names.

dwarf.py reads 32-bit little-endian DWARF with 4-byte addresses, as make
gcc's GCC and GNU as write it. Each unit must be a compilation unit of
DWARF 5, whose top holds one entry, of DW_TAG_compile_unit, with every
other entry below it. A line table must be of version 5 or 3. GCC writes
version 5, and GNU as writes version 3 where GCC leaves the lines to it, as
at -O0.

In an object, an offset from one DWARF section into another takes a
relocation. SH's GNU as gives the relocation an addend of 0 and writes the
offset into the section's own bytes. dwarf.py reads each offset there and
applies no relocation.

Each read is bounded by what holds it: an attribute by its unit, a string
by its section, and a line table's directories and files by the table's
header. dwarf.py raises Unread, saying what it can't read, on:

- a unit or a line table other than those above;
- a form DWARF 5 doesn't define, DW_FORM_indirect naming itself, or an
  implicit constant where no abbreviation holds its value;
- a value, a unit or a line table cut short, or one that runs past what
  holds it;
- an abbreviation table that lies past its section, an abbreviation code
  given twice, or a children flag other than 0 or 1;
- an entry whose code its abbreviation table doesn't give;
- a list of children that doesn't close, a null entry outside any list,
  or a unit whose top isn't one compilation unit's entry;
- a version 5 line table without directory 0, or a line table that puts a
  file in a directory outside its list;
- a version 5 line table's entry without a path, or with a path or a
  directory index in a form LINE_FORMS doesn't give it.

The caller names each attribute it reads and the forms it takes for it,
and an entry keeps only those. Such an attribute is unread where the
entry gives it in another form. It is unread too where it is a string its
section doesn't hold whole, or a reference outside its unit. An entry
leaves its unread attributes out of attrs, and its unread holds an Unread
for the first. The caller must raise it before reading attrs, or it reads
an unread attribute as one the entry doesn't give. An unread attribute of
an entry the caller never reads fails nothing.
"""

import posixpath
import struct
from collections import namedtuple
from enum import IntEnum


class Dwarf(IntEnum):
    """A DWARF constant. Each subclass is named for its constants' prefix,
    so that str() spells a constant as DWARF does, such as DW_FORM_strx1."""

    def __str__(self):
        return f'DW_{type(self).__name__.upper()}_{self.name.lower()}'


class Ut(Dwarf):
    """The unit type dwarf.py reads."""
    COMPILE = 0x01


class Tag(Dwarf):
    """The tags dwarf.py and decls.py read."""
    ARRAY_TYPE = 0x01
    ENUMERATION_TYPE = 0x04
    FORMAL_PARAMETER = 0x05
    MEMBER = 0x0D
    POINTER_TYPE = 0x0F
    COMPILE_UNIT = 0x11
    STRUCTURE_TYPE = 0x13
    SUBROUTINE_TYPE = 0x15
    TYPEDEF = 0x16
    UNION_TYPE = 0x17
    UNSPECIFIED_PARAMETERS = 0x18
    SUBRANGE_TYPE = 0x21
    BASE_TYPE = 0x24
    CONST_TYPE = 0x26
    SUBPROGRAM = 0x2E
    VARIABLE = 0x34
    VOLATILE_TYPE = 0x35
    RESTRICT_TYPE = 0x37


class At(Dwarf):
    """The attributes dwarf.py and decls.py read."""
    NAME = 0x03
    BYTE_SIZE = 0x0B
    BIT_SIZE = 0x0D
    STMT_LIST = 0x10
    COMP_DIR = 0x1B
    INLINE = 0x20
    PRODUCER = 0x25
    PROTOTYPED = 0x27
    UPPER_BOUND = 0x2F
    ABSTRACT_ORIGIN = 0x31
    COUNT = 0x37
    DATA_MEMBER_LOCATION = 0x38
    DECL_FILE = 0x3A
    DECL_LINE = 0x3B
    DECLARATION = 0x3C
    ENCODING = 0x3E
    EXTERNAL = 0x3F
    SPECIFICATION = 0x47
    TYPE = 0x49
    DATA_BIT_OFFSET = 0x6B
    LINKAGE_NAME = 0x6E
    ALIGNMENT = 0x88


class Form(Dwarf):
    """DWARF 5's forms."""
    ADDR = 0x01
    BLOCK2 = 0x03
    BLOCK4 = 0x04
    DATA2 = 0x05
    DATA4 = 0x06
    DATA8 = 0x07
    STRING = 0x08
    BLOCK = 0x09
    BLOCK1 = 0x0A
    DATA1 = 0x0B
    FLAG = 0x0C
    SDATA = 0x0D
    STRP = 0x0E
    UDATA = 0x0F
    REF_ADDR = 0x10
    REF1 = 0x11
    REF2 = 0x12
    REF4 = 0x13
    REF8 = 0x14
    REF_UDATA = 0x15
    INDIRECT = 0x16
    SEC_OFFSET = 0x17
    EXPRLOC = 0x18
    FLAG_PRESENT = 0x19
    STRX = 0x1A
    ADDRX = 0x1B
    REF_SUP4 = 0x1C
    STRP_SUP = 0x1D
    DATA16 = 0x1E
    LINE_STRP = 0x1F
    REF_SIG8 = 0x20
    IMPLICIT_CONST = 0x21
    LOCLISTX = 0x22
    RNGLISTX = 0x23
    REF_SUP8 = 0x24
    STRX1 = 0x25
    STRX2 = 0x26
    STRX3 = 0x27
    STRX4 = 0x28
    ADDRX1 = 0x29
    ADDRX2 = 0x2A
    ADDRX3 = 0x2B
    ADDRX4 = 0x2C


class Ate(Dwarf):
    """The base type encodings decls.py reads."""
    BOOLEAN = 0x02
    FLOAT = 0x04
    SIGNED = 0x05
    SIGNED_CHAR = 0x06
    UNSIGNED = 0x07
    UNSIGNED_CHAR = 0x08


class Lnct(Dwarf):
    """The contents of a DWARF 5 line table's entries that dwarf.py reads."""
    PATH = 0x01
    DIRECTORY_INDEX = 0x02


# the bytes of an address in make gcc's objects, which target the 32-bit
# SH-4
ADDRESS_SIZE = 4
# a DWARF 5 compilation unit's header: its length, version, unit type and
# address size, and its abbreviation table's offset in .debug_abbrev. The
# length counts the bytes past its own 4
UNIT_HEADER = struct.Struct('<IHBBI')
# the length that opens a unit of 64-bit DWARF
DWARF64 = 0xFFFFFFFF
# the bytes of each form of a fixed width in 32-bit DWARF
WIDTHS = {Form.FLAG_PRESENT: 0,
          Form.DATA1: 1, Form.REF1: 1, Form.FLAG: 1, Form.STRX1: 1, Form.ADDRX1: 1,
          Form.DATA2: 2, Form.REF2: 2, Form.STRX2: 2, Form.ADDRX2: 2, Form.STRX3: 3, Form.ADDRX3: 3,
          Form.DATA4: 4, Form.REF4: 4, Form.STRP: 4, Form.LINE_STRP: 4, Form.SEC_OFFSET: 4, Form.REF_ADDR: 4,
          Form.REF_SUP4: 4, Form.STRP_SUP: 4, Form.STRX4: 4, Form.ADDRX4: 4, Form.ADDR: ADDRESS_SIZE,
          Form.DATA8: 8, Form.REF8: 8, Form.REF_SIG8: 8, Form.REF_SUP8: 8, Form.DATA16: 16}
# the forms of an LEB128 number
LEB128 = frozenset((Form.SDATA, Form.UDATA, Form.REF_UDATA, Form.STRX, Form.ADDRX, Form.LOCLISTX, Form.RNGLISTX))
# the forms of a block, by the bytes of the length before it, or None for
# an LEB128 length
BLOCKS = {Form.BLOCK1: 1, Form.BLOCK2: 2, Form.BLOCK4: 4, Form.BLOCK: None, Form.EXPRLOC: None}
# the forms that give a string inline, or by its offset in .debug_str or
# .debug_line_str
STRINGS = frozenset((Form.STRING, Form.STRP, Form.LINE_STRP))
# the forms that give a reference by its offset in its unit
REFERENCES = frozenset((Form.REF1, Form.REF2, Form.REF4, Form.REF8, Form.REF_UDATA))
FLAGS = frozenset((Form.FLAG, Form.FLAG_PRESENT))
# the forms dwarf.py takes for the contents of a DWARF 5 line table's
# entries it reads, those DWARF 5 gives each: a string for a path, and a
# 1-byte, 2-byte or LEB128 constant for a directory's index
LINE_FORMS = {Lnct.PATH: STRINGS, Lnct.DIRECTORY_INDEX: frozenset((Form.DATA1, Form.DATA2, Form.UDATA))}


class Unread(Exception):
    """DWARF that dwarf.py or its caller doesn't read. Its message follows
    the object's name, as in "a.o: holds a unit of DWARF 4"."""


# a debugging information entry. attrs gives the value of each attribute
# the caller reads that the entry gives, but an unread one: a string as its
# text, a reference as the offset in .debug_info of the entry it names, a
# flag as True, a block as its bytes, and any other as its number. forms
# gives the form of each, unread ones included. Neither map holds a flag
# given false, which DWARF reads as a flag left out. children holds the
# offsets of the entry's children, in order. unread is the Unread of its
# first unread attribute, or None
Die = namedtuple('Die', 'tag attrs forms children unread')
# a compilation unit. dies gives its entries by their offsets in
# .debug_info, and root is the offset of its top entry. files gives the
# path of each file its line table names, by the number DW_AT_decl_file
# gives it. A path is relative to the compilation's directory, the unit's
# DW_AT_comp_dir, unless it is absolute. files[0] is None where the table
# numbers files from 1. Without a DW_AT_stmt_list the caller reads, files
# is empty
Unit = namedtuple('Unit', 'dies root files')


def fixed(data, at, width, end):
    """The little-endian number of width bytes at data[at], before end, and
    the offset past it."""
    if at + width > end:
        raise Unread('its DWARF holds a value cut short')
    return int.from_bytes(data[at:at + width], 'little'), at + width


def leb128(data, at, end, signed=False):
    """The LEB128 number at data[at], before end, and the offset past it."""
    value = shift = 0
    while True:
        if at >= end:
            raise Unread('its DWARF holds a number cut short')
        byte = data[at]
        at += 1
        value |= (byte & 0x7F) << shift
        shift += 7
        if byte < 0x80:
            return value - (1 << shift) if signed and byte & 0x40 else value, at


def string(data, at, end):
    """The NUL-terminated string at data[at], before end, and the offset
    past its NUL. The string decodes as Latin-1, which takes every byte."""
    stop = data.find(b'\0', at, end)
    if stop < 0:
        raise Unread('its DWARF holds a string cut short')
    return data[at:stop].decode('latin-1'), stop + 1


def decode(data, at, end, form, implicit=None):
    """The value of form at data[at], before end, as (form, value, offset
    past it). For DW_FORM_indirect, the form returned is the one it names.
    The value of a block is its bytes, of an inline string its text, and of
    any other form a number. implicit is the value an implicit constant's
    abbreviation gives, and without it an implicit constant is Unread."""
    if form in WIDTHS:
        value, at = fixed(data, at, WIDTHS[form], end)
        return form, value, at
    if form == Form.IMPLICIT_CONST:
        if implicit is None:
            raise Unread(f'its DWARF gives {Form.IMPLICIT_CONST} where no abbreviation holds its value')
        return form, implicit, at
    if form == Form.STRING:
        text, at = string(data, at, end)
        return form, text, at
    if form in LEB128:
        value, at = leb128(data, at, end, signed=form == Form.SDATA)
        return form, value, at
    if form in BLOCKS:
        if BLOCKS[form] is None:
            length, at = leb128(data, at, end)
        else:
            length, at = fixed(data, at, BLOCKS[form], end)
        if at + length > end:
            raise Unread('its DWARF holds a block cut short')
        return form, data[at:at + length], at + length
    if form == Form.INDIRECT:
        form, at = leb128(data, at, end)
        if form == Form.INDIRECT:
            raise Unread(f'its DWARF names {Form.INDIRECT} indirectly, which DWARF 5 forbids')
        return decode(data, at, end, form)
    raise Unread(f"its DWARF holds form {form:#x}, which DWARF 5 doesn't define")


def abbreviations(data, at):
    """The abbreviation table at data[at], by code. Each abbreviation is
    (tag, children, attributes). children is whether its entries have
    children. attributes holds (attribute, form, implicit) for each of its
    attributes. implicit is an implicit constant's value, or None for any
    other form."""
    end, found = len(data), {}
    while True:
        code, at = leb128(data, at, end)
        if not code:
            return found
        if code in found:
            raise Unread(f'its DWARF gives abbreviation {code} twice')
        tag, at = leb128(data, at, end)
        children, at = fixed(data, at, 1, end)
        if children not in (0, 1):
            raise Unread(f'its DWARF gives abbreviation {code} the children flag {children}')
        specs = []
        while True:
            name, at = leb128(data, at, end)
            form, at = leb128(data, at, end)
            if not name and not form:
                break
            implicit = None
            if form == Form.IMPLICIT_CONST:
                implicit, at = leb128(data, at, end, signed=True)
            specs.append((name, form, implicit))
        found[code] = (tag, bool(children), specs)


def units(sections, read):
    """The compilation units of an object's DWARF, as Unit. sections gives
    the bytes of each of the object's sections by name. An object without
    .debug_info has no unit. read gives, for each attribute the caller
    reads, the forms it takes for it. units() raises the Unread each unit's
    top entry holds."""
    info = sections.get('.debug_info', b'')
    abbrev = sections.get('.debug_abbrev', b'')
    strings = {Form.STRP: sections.get('.debug_str', b''), Form.LINE_STRP: sections.get('.debug_line_str', b'')}
    found, start = [], 0
    while start < len(info):
        if start + UNIT_HEADER.size > len(info):
            raise Unread('holds a unit cut short')
        length, version, kind, address_size, abbrev_offset = UNIT_HEADER.unpack_from(info, start)
        end = start + 4 + length
        if length == DWARF64:
            raise Unread('holds a unit of 64-bit DWARF, and dwarf.py reads 32-bit DWARF alone')
        if end > len(info):
            raise Unread('holds a unit that runs past its section')
        if end < start + UNIT_HEADER.size:
            raise Unread('holds a unit shorter than its header')
        if version != 5:
            raise Unread(f'holds a unit of DWARF {version}, and dwarf.py reads DWARF 5 alone')
        if kind != Ut.COMPILE:
            raise Unread(f'holds a DWARF unit of kind {kind}, and dwarf.py reads compilation units alone')
        if address_size != ADDRESS_SIZE:
            raise Unread(f'holds a DWARF unit of {address_size}-byte addresses, and dwarf.py reads 4-byte ones '
                         'alone')
        if abbrev_offset >= len(abbrev):
            raise Unread('holds a unit whose abbreviation table lies past .debug_abbrev')
        dies, root = entries(info, start, end, abbreviations(abbrev, abbrev_offset), strings, read)
        top = dies[root]
        if top.tag != Tag.COMPILE_UNIT:
            raise Unread(f'holds a unit whose entry has tag {top.tag:#x}, not {Tag.COMPILE_UNIT}')
        if top.unread:
            raise top.unread
        table = top.attrs.get(At.STMT_LIST)
        files = [] if table is None else file_names(sections.get('.debug_line', b''), table, strings)
        found.append(Unit(dies, root, files))
        start = end
    return found


def entries(info, start, end, abbrevs, strings, read):
    """The entries of the unit at info[start:end], as Die by offset, and the
    offset of the one entry at its top. abbrevs is the unit's abbreviation
    table as abbreviations() reads it. strings gives the sections a string's
    offset points into, by the form that gives the offset. read is as
    units() gives it."""
    found, root, at = {}, None, start + UNIT_HEADER.size
    # the offsets of the entries whose children the loop is reading,
    # innermost last
    parents = []
    while at < end:
        offset = at
        code, at = leb128(info, at, end)
        if not code:
            if not parents:
                raise Unread('its DWARF holds a null entry outside any list of children')
            parents.pop()
            continue
        if code not in abbrevs:
            raise Unread(f"its DWARF holds an entry of abbreviation {code}, which its table doesn't give")
        if root is not None and not parents:
            raise Unread('its DWARF holds a second entry at the top of a unit')
        tag, children, specs = abbrevs[code]
        attrs, forms, unread = {}, {}, None
        for name, form, implicit in specs:
            form, raw, at = decode(info, at, end, form, implicit)
            taken = read.get(name)
            if taken is None or (form == Form.FLAG and not raw):
                continue
            forms[name] = form
            if form not in taken:
                unread = unread or Unread(f"its DWARF gives {At(name)} in {Form(form)}, which the check doesn't read")
            elif form in strings:
                try:
                    attrs[name], _ = string(strings[form], raw, len(strings[form]))
                except Unread:
                    unread = unread or Unread(f"its DWARF gives {At(name)} a string its section doesn't hold whole")
            elif form in REFERENCES:
                if UNIT_HEADER.size <= raw < end - start:
                    attrs[name] = start + raw
                else:
                    unread = unread or Unread(f'its DWARF gives {At(name)} a reference outside its unit')
            else:
                attrs[name] = True if form in FLAGS else raw
        found[offset] = Die(tag, attrs, forms, [], unread)
        if root is None:
            root = offset
        else:
            found[parents[-1]].children.append(offset)
        if children:
            parents.append(offset)
    if parents:
        raise Unread("its DWARF holds a list of children that doesn't close")
    if root is None:
        raise Unread('holds a unit without an entry')
    return found, root


def file_names(data, at, strings):
    """The path of each file the line table at data[at] names, in a list
    indexed by the file's number. A path is relative to the compilation's
    directory unless it is absolute. A table of version 5 numbers its files
    from 0. Its directory 0 is the compilation's, whatever path it gives
    that directory. A table of version 3 numbers its files from 1, so the
    list returned starts with None. Its directory 0 is the compilation's
    too, and its own list of directories starts at 1. strings are as units()
    gives them."""
    length, at = fixed(data, at, 4, len(data))
    end = at + length
    if end > len(data):
        raise Unread('holds a line table that runs past its section')
    version, at = fixed(data, at, 2, end)
    if version not in (3, 5):
        raise Unread(f'holds a line table of version {version}, and dwarf.py reads versions 3 and 5 alone')
    if version == 5:
        address_size, at = fixed(data, at, 1, end)
        if address_size != ADDRESS_SIZE:
            raise Unread(f'holds a line table of {address_size}-byte addresses')
        at += 1  # past segment_selector_size
    header_length, at = fixed(data, at, 4, end)
    stop = at + header_length
    if stop > end:
        raise Unread('holds a line table whose header runs past it')
    # past the fields from minimum_instruction_length to line_range: five
    # bytes, or four in version 3, which lacks
    # maximum_operations_per_instruction
    at += 5 if version == 5 else 4
    opcode_base, at = fixed(data, at, 1, stop)
    # past standard_opcode_lengths, a byte for each opcode from 1 to
    # opcode_base - 1
    at += opcode_base - 1
    if version == 3:
        directories, files = [''], []
        while continues(data, at, stop, 'directories'):
            directory, at = string(data, at, stop)
            directories.append(directory)
        at += 1
        while continues(data, at, stop, 'files'):
            path, at = string(data, at, stop)
            index, at = leb128(data, at, stop)
            _, at = leb128(data, at, stop)  # its modification time
            _, at = leb128(data, at, stop)  # its length
            files.append((path, index))
    else:
        directory_entries, at = entry_list(data, at, stop, strings)
        if not directory_entries:
            raise Unread('holds a line table without directory 0')
        file_entries, _ = entry_list(data, at, stop, strings)
        directories = [''] + [entry[Lnct.PATH] for entry in directory_entries[1:]]
        files = [(entry[Lnct.PATH], entry.get(Lnct.DIRECTORY_INDEX, 0)) for entry in file_entries]
    paths = [None] if version == 3 else []
    for path, index in files:
        if not 0 <= index < len(directories):
            raise Unread(f'holds a line table that puts {path} in directory {index}, outside its list')
        paths.append(posixpath.join(directories[index], path))
    return paths


def continues(data, at, stop, what):
    """Whether data[at] starts another entry of a version 3 line table's
    list of what, its directories or its files, which a 0 byte ends. A list
    that reaches stop before its 0 is Unread, naming what."""
    if at >= stop:
        raise Unread(f"holds a line table whose {what} don't end in its header")
    return data[at] != 0


def entry_list(data, at, stop, strings):
    """The entries of a DWARF 5 line table's list of directories or of
    files, at data[at], before stop, and the offset past them. The list
    gives the contents of each entry and their forms, then the entries.
    Each entry is a dict of the contents LINE_FORMS names. An entry without
    a path, or with a path or directory index in a form LINE_FORMS doesn't
    give it, is Unread."""
    count, at = fixed(data, at, 1, stop)
    formats = []
    for _ in range(count):
        content, at = leb128(data, at, stop)
        form, at = leb128(data, at, stop)
        formats.append((content, form))
    total, at = leb128(data, at, stop)
    found = []
    for _ in range(total):
        entry = {}
        for content, form in formats:
            form, raw, at = decode(data, at, stop, form)
            if content not in LINE_FORMS:
                continue
            if form not in LINE_FORMS[content]:
                raise Unread(f"its line table gives {Lnct(content)} in {Form(form)}, which the check doesn't read")
            entry[content] = string(strings[form], raw, len(strings[form]))[0] if form in strings else raw
        if Lnct.PATH not in entry:
            raise Unread('its line table gives an entry without a path')
        found.append(entry)
    return found, at
