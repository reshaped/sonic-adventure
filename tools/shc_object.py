"""Read an object SHC or asmsh writes: its records, the runs they form, and
the sections, definitions and external references those runs give.

tools/link.py and tools/relink.py link the matching build's objects by what
these read, and tools/progress.py reads each object's sections. make gcc's
import check, tools/imports.py, reads the sections and definitions of the
matching build's objects, which define what 1ST_READ.BIN imports.
"""

from tool import content, fail

# the kinds of record these read: the object's sections, its external
# references, its definitions and the end of the object
SECTIONS, REFERENCES, DEFINITIONS, END = 0x08, 0x0C, 0x14, 0x7F
# the bit of a definition's attribute that marks a name defined at an
# address, as an .EQU defines one, rather than in a section
ABSOLUTE = 0x40


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
    """The object's sections by name, in the order its section table gives
    them, each as (size, alignment, code). size includes the bytes the object
    leaves unwritten, and code is true for a code section. A sections run
    that doesn't parse fails, as do a name the table gives twice and an
    alignment not a power of two."""
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


def definitions(obj):
    """The names the object defines, as it spells them: a C name with its
    leading underscore. Each maps to (section, value). section is the number
    of the name's section, counting from 0 in the order of the object's
    section table. value is the name's offset in that section. For a name
    defined at an address, section is None and value is that address. A
    definitions run that doesn't parse fails."""
    # an entry is the section's number in 2 bytes, an attribute, the value,
    # in 4 bytes or, where the attribute has ABSOLUTE set, in as many as the
    # byte after it gives, then the name's length and the name. An ABSOLUTE
    # entry's section number is 0 and names no section
    names = {}
    for start, kind, body in runs(obj):
        if kind != DEFINITIONS:
            continue
        at = 0
        while at < len(body):
            absolute = at + 3 < len(body) and body[at + 2] & ABSOLUTE
            value_at = at + (4 if absolute else 3)
            length_at = value_at + (body[at + 3] if absolute else 4)
            if length_at >= len(body) or (end := length_at + 1 + body[length_at]) > len(body):
                fail(f'{obj}: the definitions at {start:#x} don\'t parse')
            section = None if absolute else int.from_bytes(body[at:at + 2], 'big')
            name = body[length_at + 1:end].decode('latin-1')
            names[name] = section, int.from_bytes(body[value_at:length_at], 'big')
            at = end
    return names
