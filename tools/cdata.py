"""C for an image's items, as tools/data.py and tools/content.py write it:
each definition's initializer, spelled so that SHC compiles it back to the
image's bytes. The caller hands it the image, the items, their names and
any declarations; it reads no file, and raises EmitError where Items
refuses the items or C can't give their bytes.
"""

import re
import struct
from bisect import bisect_left, bisect_right
from collections import namedtuple
from decimal import ROUND_CEILING, Context, Decimal, InvalidOperation, localcontext
from functools import partial
from math import prod

from image import (ACTION, CACTION, COLOR, DECLARED, GENERIC, KINDS, LAND, LAND_ENTRY, LAND_MOTION, MATERIAL, MDATA,
                   MESHSET, MKEY_P, MODEL, MOTION, OBJECT, SAME, SINGLE, STAGE_PATH, STAGE_PATH_POINT, TEXANIM, TEXLIST,
                   TEXNAME, Declared, f32, float_ok)
from layout import QUALIFIERS, Array, LayoutError, Pointer, Scalar, number, spelled


# The C emitter
#
# Write an image's data as C that SHC 5.0 compiles to the same bytes. The
# emitter reads the image as image.py's Memory, and its items as an Items,
# which checks them where the emitter takes them. A Writer turns them into a
# source and the names it takes from outside. It writes pointers as
# expressions of names, each float as the literal of the fewest significant
# digits that SHC's conversion gives back, a union's value without the braces
# SHC rejects, and each string escaped for SHC's preprocessor. It writes a
# value of any type Types lays out, or raises the error below. Bodies writes a
# body source's bodies, from the declarations its command hands it.
#
# The emitter raises EmitError, its one error, for:
#
# - items that Items refuses;
# - a float that no literal of up to 9 digits and within FLT_MAX gives back;
# - a pointer into the padding between definitions;
# - a byte no initializer writes, where it isn't zero;
# - a byte read outside the image;
# - a body that Bodies can't write, as its methods say.
#
# The error names the image, the address and the problem there.


class EmitError(Exception):
    """Bytes at an address of an image that C can't present as SHC needs them."""

    def __init__(self, image, address, problem):
        super().__init__(f'{image.name}: 0x{address:08x}: {problem}')


# the escapes of a text's bytes in C, and the characters that would continue
# a numeric one
ESCAPES = {0x00: '\\0', 0x07: '\\a', 0x09: '\\t', 0x0A: '\\n', 0x0D: '\\r', 0x22: '\\"', 0x5C: '\\\\'}
CONTINUE = {'0': '01234567', 'x': '0123456789abcdefABCDEF'}

# the names flags() gives a node's evalflags bits 0 to 5, as ninja.h spells
# them; with a higher bit set, it writes the evalflags in hex
EVAL = ('NJD_EVAL_UNIT_POS', 'NJD_EVAL_UNIT_ANG', 'NJD_EVAL_UNIT_SCL', 'NJD_EVAL_HIDE',
        'NJD_EVAL_BREAK', 'NJD_EVAL_ZXY_ANG')

# the names flags() gives a motion's type bits 0 to 2, as ninja.h spells
# them; with a higher bit set, it writes the type bits in hex
MTYPE = ('NJD_MTYPE_POS_0', 'NJD_MTYPE_ANG_1', 'NJD_MTYPE_SCL_2')

# a strip's first value holds its length in these bits
STRIP_LENGTH = 0x3FFF


def unqualified(ctype):
    """A type's qualifiers, and the type without them."""
    words = ctype.split()
    return {word for word in words if word in QUALIFIERS}, ' '.join(word for word in words if word not in QUALIFIERS)


def converts(natural, pointee):
    """Whether a pointer to natural initializes a pointer to pointee with no
    cast: pointee keeps every qualifier natural has, and is void or natural's
    type. A qualifier counts at its own level, so where natural is a pointer,
    pointee is void or a pointer to the type natural points to, qualifiers
    and all. SAME's two spellings of one struct count as one type."""
    def levels(ctype):
        # the qualifiers at each level, the base's first and the type's own
        # last, and the base, as SAME takes it
        base, *pointers = ctype.split('*')
        held, base = unqualified(base)
        return [held, *(set(pointer.split()) for pointer in pointers)], SAME.get(base, base)

    (*inner, held), natural = levels(natural)
    (*through, kept), pointee = levels(pointee)
    return held <= kept and (pointee == 'void' and not through or pointee == natural and inner == through)


def spell(word):
    """The decimal literal of the fewest significant digits, up to 9, that
    SHC's conversion reads back as the word, or None where none is
    supported. SHC's conversion truncates toward zero, so the literal's
    magnitude lies from the word's up to the next float's. FLT_MAX gets
    None: each literal of up to 9 digits that truncates to it lies above
    it. The calculation uses a private Decimal context and does not depend
    on the caller's context."""
    sign = '-' if word >> 31 else ''
    magnitude = word & 0x7FFFFFFF
    if not magnitude:
        return sign + '0.0F'
    if not float_ok(word):
        return None
    low, high = Decimal.from_float(f32(magnitude)), Decimal.from_float(f32(magnitude + 1))
    if high.is_infinite():
        # FLT_MAX: each decimal of up to 9 digits that truncates to it lies
        # above it, and whether SHC takes one is untested
        return None
    # Python's default precision and exponents, and no trap on a rounding
    with localcontext(Context(prec=28, Emin=-999999, Emax=999999, traps=[InvalidOperation])):
        for digits in range(1, 10):
            value = low.quantize(Decimal(1).scaleb(low.adjusted() - digits + 1), rounding=ROUND_CEILING)
            if value < high:
                break
        value = value.normalize()
        text = format(value, 'f') if -5 < value.adjusted() < 9 else format(value, 'e')
    if '.' not in text and 'e' not in text:
        text += '.0'
    return sign + text + 'F'


def payload(image, start, end):
    """The image's bytes from start to end. Raises EmitError where start
    passes end or any byte lies outside the image: a slice there would come
    back short or wrong."""
    if not image.base <= start <= end <= image.end:
        raise EmitError(image, start, f'{end - start} bytes to 0x{end:08x} lie outside the image')
    return image.payload[start - image.base:end - image.base]


class Items:
    """An image's items as the emitter takes them, in address order, checked
    against the ranges whose bytes they account for, each (start, end), and
    the names they must define, by address. Each item lies in a range,
    starts on its alignment, and spans a whole number of elements of a
    positive size, and one element where it is no array. Each lies in the
    image's bytes, but a Declared, which is only a pointer's target and can
    lie in bss. A byte of a range that no item holds is padding: the fill
    byte, in a run up to an item that it aligns, and no longer. Each
    required name starts an item. Raises EmitError, naming an address at
    fault, where any of this fails."""

    def __init__(self, image, items, ranges, names):
        self.image, self.items, ranges = image, list(items), sorted(ranges)
        starts, lows = [item.start for item in self.items], [low for low, _ in ranges]

        def name(address):
            return names.get(address) or f'dat_{address:08x}'

        for item in self.items:
            size, at = item.end - item.start, bisect_right(lows, item.start) - 1
            if size <= 0 or item.element <= 0 or size % item.element:
                raise EmitError(image, item.start, f'{name(item.start)} is {size} bytes, no whole number of '
                                                   f'{item.element}-byte {item.ctype}')
            if not item.array and size != item.element:
                raise EmitError(image, item.start, f'{name(item.start)} is one {item.ctype}, and {size} bytes')
            if item.start % item.alignment:
                raise EmitError(image, item.start, f'{name(item.start)} starts off {item.ctype}\'s alignment, '
                                                   f'{item.alignment}')
            if at < 0 or item.end > ranges[at][1]:
                raise EmitError(image, item.start, f'{name(item.start)}, to 0x{item.end:08x}, lies outside the ranges')
            if not isinstance(item, Declared):
                payload(image, item.start, item.end)
        for first, second in zip(self.items, self.items[1:]):
            if second.start < first.end:
                raise EmitError(image, second.start, f'{name(second.start)} starts before {name(first.start)} ends, '
                                                     f'at 0x{first.end:08x}')
        for low, high in ranges:
            cursor = low
            for item in self.items[bisect_left(starts, low):bisect_left(starts, high)]:
                if item.start > cursor and (item.start - cursor != (-cursor) % item.alignment
                                            or set(payload(image, cursor, item.start)) - {image.fill}):
                    raise EmitError(image, cursor, f'{item.start - cursor} bytes before {name(item.start)} that no '
                                                   'definition holds, and that are no fill aligning it')
                cursor = item.end
            if cursor < high:
                raise EmitError(image, cursor, f'{high - cursor} bytes to 0x{high:08x} that no definition holds')
        missing = sorted(names.keys() - set(starts))
        if missing:
            index = bisect_right(starts, missing[0]) - 1
            holder = self.items[index] if index >= 0 and missing[0] < self.items[index].end else None
            raise EmitError(image, missing[0], f'{names[missing[0]]} falls inside {holder.ctype} 0x{holder.start:08x}'
                            if holder else f'{names[missing[0]]} starts no definition')


# the C file a Writer writes, and the address of each name from outside the
# items that it declares extern, by the name
Emitted = namedtuple('Emitted', 'text outside')


class Writer:
    """The items as C: every pointer a name, every float a literal.

    items is an Items, which checks them. names is the image's names, by
    address, which name each item and each address outside them that a
    pointer takes."""

    def __init__(self, items, names, constant=False):
        self.image, self.items = items.image, items.items
        self.constant = constant  # whether the items are const, as data in section C is
        self.starts = [item.start for item in self.items]
        self.names = names
        self.uses = {}  # each name from outside: its address, and the types it's taken as
        self.ahead = set()  # the indexes of the items pointed at before they're defined
        self.index = 0  # the index of the item source() is writing; a pointer to a later one declares it ahead

    def name(self, address):
        return self.names.get(address) or f'dat_{address:08x}'

    def const(self, address):
        """Whether the item at address is defined const, as each is where
        constant is true."""
        return self.constant

    def unpack(self, code, address):
        """The number struct code reads at address, from payload()'s bytes."""
        return struct.unpack(f'<{code}', payload(self.image, address, address + struct.calcsize(code)))[0]

    def word(self, address):
        return self.unpack('I', address)

    def ref(self, address, pointee):
        r"""A constant for the pointer at address, as a pointee *.

        A null pointer is 0. A pointer into an item is written from the item's
        name: at its start, the name, or &name for one record; at an element,
        the element's address; elsewhere, a byte offset into it. Each is cast
        where C won't convert it to a pointee *, from the item's type as its
        definition gives it, const included. A pointer outside the items'
        span takes a name from outside: the address's name in names, or else
        fun_ for code or dat_ for data, then the address in hex. That name is
        left as \1name\2pointee\3 for source() to resolve, once every use
        has given the name its type.

        The name from outside goes into uses, with its address and the
        pointee. The index of an item after the one source() is writing goes
        into ahead.

        Raises EmitError for a pointer into the padding between definitions,
        or where address lies outside the image."""
        value = self.word(address)
        if value == 0:
            return '0'
        index = bisect_right(self.starts, value) - 1
        if index < 0 or not self.items[index].start <= value < self.items[index].end:
            if self.items[0].start <= value < self.items[-1].end:
                raise EmitError(self.image, address, f'a pointer to 0x{value:08x}, in the padding between definitions')
            name = self.names.get(value) or f'{"fun" if self.image.in_code(value) else "dat"}_{value:08x}'
            self.uses.setdefault(name, (value, set()))[1].add(pointee)
            return f'\1{name}\2{pointee}\3'
        item = self.items[index]
        if index > self.index:
            self.ahead.add(index)
        offset, base = value - item.start, self.name(item.start)
        # const after the type, as typed() writes it
        const = ' const' if self.const(item.start) else ''
        element_type = ('void' if item.kind in ('pointers', 'mixed') else item.ctype) + const
        if offset == 0:
            expression, natural = (base, element_type) if item.array else (f'&{base}', item.ctype + const)
        elif item.array and offset % item.element == 0:
            expression, natural = f'&{base}[{offset // item.element}]', element_type
        else:
            expression = f'(char *){"" if item.array else "&"}{base} + {offset}'
            natural = 'char'
            if pointee != 'char':
                expression = f'({expression})'
        if converts(natural, pointee):
            return expression
        return f'({pointee} *){expression}'

    def extern_type(self, name):
        """The type source() gives a name from outside in its extern
        declaration. It is 'code' for an address in code. Otherwise it is the
        one type the name's uses take other than void and char, or char where
        they take none or several."""
        address, pointees = self.uses[name]
        if self.image.in_code(address):
            return 'code'
        types = {SAME.get(pointee, pointee) for pointee in pointees} - {'void', 'char'}
        return types.pop() if len(types) == 1 else 'char'

    def resolve(self, match):
        name, pointee = match.group(1), match.group(2)
        ctype = self.extern_type(name)
        expression = f'&{name}' if ctype in SINGLE else name
        if pointee == 'void' or SAME.get(pointee, pointee) == ctype:
            return expression
        return f'({pointee} *){expression}'

    def check_padding(self, start, end):
        """Raise EmitError at the first byte from start to end that isn't
        zero: C gives padding no initializer, and SHC zeroes it."""
        for where, byte in enumerate(payload(self.image, start, end), start):
            if byte:
                raise EmitError(self.image, where, f'a pad byte of 0x{byte:02x}, which no initializer writes')

    def float(self, address):
        """The float at address as the literal spell() gives. Raises
        EmitError where spell() gives none."""
        word = self.word(address)
        text = spell(word)
        if text is None:
            raise EmitError(self.image, address,
                            f'0x{word:08x} is no float a decimal literal of up to 9 digits and within FLT_MAX spells')
        return text

    def initializer(self, layout, address):
        """The initializer of the value at address, laid out as layout.

        A struct or an array is in braces. A union is its first member's value,
        unbraced, as a material's colors are. Raises EmitError for a hole that
        holds a byte other than zero, and where float(), ref() or payload()
        does."""
        if isinstance(layout, Scalar):
            if layout.code == 'f':
                return self.float(address)
            if layout.code == 'c':
                return character(self.unpack('B', address))
            return integer(layout.code, self.unpack(layout.code, address))
        if isinstance(layout, Pointer):
            return self.ref(address, layout.pointee or 'void')
        if isinstance(layout, Array):
            step = layout.element.size
            values = [self.initializer(layout.element, address + step * index) for index in range(layout.count)]
        else:
            for at, size in layout.holes:
                self.check_padding(address + at, address + at + size)
            fields = layout.fields[:1] if layout.union else layout.fields
            values = [self.initializer(field, address + offset) for _, field, offset in fields]
            if layout.union:
                return values[0]
        return f'{{ {", ".join(values)} }}'

    def floats(self, address, count):
        return ', '.join(self.float(address + 4 * index) for index in range(count))

    def values(self, item):
        """Each element's initializer, and how many go on a line."""
        image, ref = self.image, self.ref
        u32, s32, u16, s16 = (partial(self.unpack, code) for code in 'IiHh')
        start, end, kind = item.start, item.end, item.kind
        steps = range(start, end, item.element)
        if kind == 'laid':
            layout = item.layout
            return [self.initializer(layout, address) for address in steps], \
                LINE[layout.code] if isinstance(layout, Scalar) else 4 if isinstance(layout, Pointer) else 1
        raw = payload(image, start, end)
        if item.single and end - start > item.element:
            # an array a source declares of a kind that is one record: each
            # record in turn
            values = []
            for address in steps:
                values += self.values(item.part(address, address + item.element))[0]
            return values, 1
        if kind == 'object':
            angles = ', '.join(str(s32(start + OBJECT.angle + 4 * i)) for i in range(3))
            return [f'{{\n    {flags(u32(start + OBJECT.evalflags), EVAL)}, '
                    f'{ref(start + OBJECT.model, "NJS_MODEL")},\n'
                    f'    {{ {self.floats(start + OBJECT.position, 3)} }},\n    {{ {angles} }},\n'
                    f'    {{ {self.floats(start + OBJECT.scale, 3)} }},\n'
                    f'    {ref(start + OBJECT.child, "NJS_OBJECT")}, '
                    f'{ref(start + OBJECT.sibling, "NJS_OBJECT")},\n}}'], 1
        if kind == 'model':
            return [f'{{\n    {ref(start + MODEL.points, "NJS_POINT3")}, '
                    f'{ref(start + MODEL.normals, "NJS_VECTOR")}, {s32(start + MODEL.point_count)},\n'
                    f'    {ref(start + MODEL.meshsets, "NJS_MESHSET")}, '
                    f'{ref(start + MODEL.materials, "NJS_MATERIAL")}, '
                    f'{u16(start + MODEL.meshset_count)}, {u16(start + MODEL.material_count)},\n'
                    f'    {{ {self.floats(start + MODEL.center, 3)} }}, {self.float(start + MODEL.radius)},\n}}'], 1
        if kind == 'motion':
            return [f'{{ {ref(start + MOTION.mdata, "void")}, {u32(start + MOTION.frames)}, '
                    f'{flags(u16(start + MOTION.bits), MTYPE)}, {u16(start + MOTION.interpolation)} }}'], 1
        if kind == 'action':
            return [f'{{ {ref(start + ACTION.object, "NJS_OBJECT")}, '
                    f'{ref(start + ACTION.motion, "NJS_MOTION")} }}'], 1
        if kind == 'caction':
            return [f'{{ {ref(start + CACTION.camera, "NJS_CAMERA")}, '
                    f'{ref(start + CACTION.motion, "NJS_MOTION")} }}'], 1
        if kind == 'texlist':
            return [f'{{ {ref(start + TEXLIST.names, "NJS_TEXNAME")}, {u32(start + TEXLIST.count)} }}'], 1
        if kind == 'land':
            shorts = ', '.join(str(s16(start + offset)) for offset in (LAND.count, LAND.motion_count, LAND.flags,
                                                                     LAND.users))
            return [f'{{ {shorts}, {self.float(start + LAND.clip)}, {ref(start + LAND.entries, "struct land_entry")}, '
                    f'{ref(start + LAND.motions, "struct land_motion")}, {ref(start + LAND.pvm, "char")}, '
                    f'{ref(start + LAND.texlist, "NJS_TEXLIST")}, {ref(start + LAND.file, "char")}, '
                    f'{ref(start + LAND.data, "void")} }}'], 1
        if kind == 'land_entry':
            entry = LAND_ENTRY
            return [f'{{ {{ {self.floats(address + entry.center, 3)} }}, {self.floats(address + entry.radius, 3)}, '
                    f'{ref(address + entry.object, "NJS_OBJECT")}, {s32(address + entry.blocks)}, '
                    f'{s32(address + entry.flags)} }}' for address in steps], 1
        if kind == 'land_motion':
            # its play is a union, which SHC takes without braces: its first
            # member, the action
            part = LAND_MOTION
            return [f'{{ {self.floats(address + part.frame, 3)}, {ref(address + part.object, "NJS_OBJECT")}, '
                    f'{ref(address + part.play, "NJS_ACTION")}, {ref(address + part.texlist, "NJS_TEXLIST")} }}'
                    for address in steps], 1
        if kind == 'path':
            path = STAGE_PATH
            # a byte of padding lies between ends and count
            self.check_padding(start + path.ends + 1, start + path.count)
            return [f'{{ {self.unpack("B", start + path.ends)}, {u16(start + path.count)}, '
                    f'{self.float(start + path.length)}, {ref(start + path.points, "struct path_point")}, '
                    f'{ref(start + path.step, "void")} }}'], 1
        if kind == 'path_point':
            point = STAGE_PATH_POINT
            return [f'{{ {s16(address + point.ang_x)}, {s16(address + point.ang_z)}, '
                    f'{self.float(address + point.length)}, {{ {self.floats(address + point.pos, 3)} }} }}'
                    for address in steps], 1
        if kind in ('points', 'normals', 'vertices'):
            return [f'{{ {self.floats(address, 3)} }}' for address in steps], 1
        if kind == 'material':
            # its colors are unions, which SHC takes without braces
            return [f'{{ 0x{u32(address + MATERIAL.diffuse):08x}, 0x{u32(address + MATERIAL.specular):08x}, '
                    f'{self.float(address + MATERIAL.exponent)}, {u32(address + MATERIAL.texture)}, '
                    f'0x{u32(address + MATERIAL.flags):08x} }}' for address in steps], 1
        if kind == 'meshset':
            return [f'{{ 0x{u16(address + MESHSET.type_material):04x}, '
                    f'{u16(address + MESHSET.mesh_count)}, '
                    f'{ref(address + MESHSET.strips, "Sint16")}, {ref(address + MESHSET.attrs, "Uint32")}, '
                    f'{ref(address + MESHSET.normals, "NJS_VECTOR")}, '
                    f'{ref(address + MESHSET.colors, "NJS_COLOR")}, '
                    f'{ref(address + MESHSET.uvs, "NJS_COLOR")} }}' for address in steps], 1
        if kind == 'strips':
            values = [s16(address) for address in steps]
            # a strip to a line, its count first, where the strips end exactly
            # at the item's end; else 12 values to a line
            lines, cursor = [], 0
            while cursor < len(values) and 0 < values[cursor] & STRIP_LENGTH <= len(values) - cursor - 1:
                count = (values[cursor] & STRIP_LENGTH) + 1
                lines.append(', '.join(map(str, values[cursor:cursor + count])))
                cursor += count
                if cursor == len(values):
                    return lines, 1
            return [str(value) for value in values], 12
        if kind == 'uv':
            return [f'UV({s16(address + COLOR.u)}, {s16(address + COLOR.v)})' for address in steps], 6
        if kind == 'vcolor':
            return [f'0x{u32(address):08x}' for address in steps], 4
        if kind in ('mkey_f', 'mkey_a'):
            key, values = KINDS[kind].layout, []
            for address in steps:
                keys = (self.floats(address + key.key, 3) if kind == 'mkey_f' else
                        ', '.join(str(s32(address + key.key + 4 * i)) for i in range(3)))
                values.append(f'{{ {u32(address + key.frame)}, {{ {keys} }} }}')
            return values, 1
        if kind == 'mkey_p':
            return [f'{{ {u32(address + MKEY_P.frame)}, {ref(address + MKEY_P.key, "void")} }}'
                    for address in steps], 2
        if kind == 'mdata':
            mdata, values = MDATA[item.factors], []
            for address in steps:
                pointers = ', '.join(ref(address + mdata.pointers + 4 * index, 'void') for index in range(item.factors))
                counts = ', '.join(str(u32(address + mdata.counts + 4 * index)) for index in range(item.factors))
                values.append(f'{{ {{ {pointers} }}, {{ {counts} }} }}')
            return values, 1
        if kind == 'texname':
            return [f'{{ {ref(address + TEXNAME.name, "void")}, 0x{u32(address + TEXNAME.attributes):x}, '
                    f'0x{u32(address + TEXNAME.address):x} }}' for address in steps], 1
        if kind == 'texanim':
            return [f'{{ {", ".join(str(s16(address + 2 * i)) for i in range(TEXANIM.size // 2))} }}'
                    for address in steps], 1
        if kind in ('pointers', 'mixed'):
            values = []
            for address in range(start, end, 4):
                word = u32(address)
                if image.is_address(word):
                    values.append(ref(address, 'void'))
                else:
                    values.append(f'(void *)0x{word:08x}' if word else '0')
            return values, 4
        if kind == 'floats':
            return [self.float(address) for address in range(start, end, 4)], 6
        width = item.element
        return [f'0x{int.from_bytes(raw[i:i + width], "little"):0{2 * width}x}'
                for i in range(0, len(raw), width)], 16 // width if width < 4 else 8

    def typed(self, ctype, name):
        """name declared as a ctype, with const after the type where the items
        are const, so that an array of pointers is const and not what they
        point at."""
        space = '' if ctype.endswith('*') else ' '
        return f'{ctype}{space}{"const " if self.constant else ""}{name}'

    def definition(self, item):
        name, count = self.name(item.start), (item.end - item.start) // item.element
        ctype = item.ctype
        if item.kind == 'string' and item.array:
            return f'{self.typed("char", name)}[{count}] = {quote(item.text.encode("latin-1"))};'
        if item.kind in GENERIC and item.array and not any(payload(self.image, item.start, item.end)):
            return f'{self.typed(ctype, name)}[{count}] = {{ 0 }};'
        values, per_line = self.values(item)
        if not item.array:
            return f'{self.typed(ctype, name)} = {values[0]};'
        return f'{self.typed(ctype, name)}[] = {{\n    ' + ',\n    '.join(wrap(values, per_line)) + ',\n};'

    def source(self, title, section=None, headers=()):
        """The items' C file, titled title, as an Emitted: the includes
        includes() gives, an extern declaration for each name from outside
        and each item pointed at ahead of its definition, then the
        definitions. Where the data takes a section, section names it to
        #pragma section around the definitions. Raises EmitError where an
        item's bytes can't be written as C."""
        body = []
        for self.index, item in enumerate(self.items):
            body.append(self.definition(item))
        text = re.sub('\1([^\2]*)\2([^\3]*)\3', self.resolve, '\n\n'.join(body))
        lines = [f'/* {title} */', '', *includes(self.items, headers), '']
        for name in sorted(self.uses):
            ctype = self.extern_type(name)
            if ctype == 'code':
                lines.append(f'extern void {name}();')
            else:
                lines.append(f'extern {ctype} {name}{"" if ctype in SINGLE else "[]"};')
        for index in sorted(self.ahead):
            item = self.items[index]
            lines.append(f'extern {self.typed(item.ctype, self.name(item.start))}{"[]" if item.array else ""};')
        if section:
            text = '\n'.join([*lines, '', f'#pragma section {section}', text, '#pragma section', ''])
        else:
            text = '\n'.join([*lines, '', text, ''])
        return Emitted(text, {name: address for name, (address, _) in self.uses.items()})


def includes(items, headers):
    """The include lines that open items' C: ninja.h's; collision/world.h's
    and path/path.h's where a land or a path needs them; then one for each
    of headers, a header a type of the items needs, in sorted order."""
    kinds = {item.kind for item in items}
    return ['#include <ninja.h>',
            *(['#include "collision/world.h"'] if kinds & {'land', 'land_entry', 'land_motion'} else []),
            *(['#include "path/path.h"'] if kinds & {'path', 'path_point'} else []),
            *(f'#include "{header}"' for header in sorted(headers))]


def flags(bits, names):
    """bits by their names, names[n] for bit n, joined by |. Zero is 0, and
    where a set bit has no name, all the bits are in hex."""
    if not bits or bits >> len(names):
        return f'0x{bits:x}' if bits else '0'
    return ' | '.join(name for bit, name in enumerate(names) if bits >> bit & 1)


def wrap(values, per_line):
    """values joined per_line to a line. Where per_line is 1 and the first
    value is a record of several lines, starting with { and a newline, each
    value's later lines are indented a level."""
    if per_line == 1 and values[0].startswith('{\n'):
        values = [value.replace('\n', '\n    ') for value in values]
    return [', '.join(values[i:i + per_line]) for i in range(0, len(values), per_line)]


def character(byte):
    """A byte as a character constant."""
    if byte == 0x27:
        return "'\\''"
    if byte in ESCAPES and byte != 0x22:
        return f"'{ESCAPES[byte]}'"
    return f"'{chr(byte)}'" if 0x20 <= byte < 0x7F else f"'\\x{byte:02x}'"


def quote(data):
    """Bytes as C strings, a new one after each newline. In a macro, SHC joins
    strings before it reads their escapes, so a character that would continue
    an escape is an escape too. Empty input is an empty string literal."""
    pieces, piece, escape = [], '', ''
    for byte in data:
        if byte in ESCAPES:
            char = ESCAPES[byte]
        elif byte == 0x3F and piece.endswith('?'):
            # ?? would start a trigraph
            char = '\\?'
        elif 0x20 <= byte < 0x7F and chr(byte) not in CONTINUE.get(escape, ''):
            char = chr(byte)
        else:
            char = f'\\x{byte:02x}'
        piece += char
        escape = char[1] if char[0] == '\\' and char[1] in CONTINUE else ''
        if byte == 0x0A:
            pieces.append(piece)
            piece, escape = '', ''
    pieces.append(piece)
    return ' \\\n    '.join(f'"{piece}"' for piece in pieces if piece) or '""'


def integer(code, value):
    """A signed integer in decimal, and an unsigned one's bits in hex."""
    return str(value) if code.islower() else f'0x{value:0{2 * struct.calcsize(code)}x}'


# A body source's bodies, which --bodies writes, and the types they take

# the kind of a body, by the type its source declares it with. A Sint16 body
# takes the kind of a meshset's strips, written a strip to a line where its
# values make strips
BODY_KINDS = {**DECLARED, 'NJS_COLOR': 'vcolor', 'NJS_MKEY_P': 'mkey_p', 'Sint16': 'strips'}
# the SDK's integer types: each one's struct code, and how many go on a line
INTEGERS = {'Sint8': ('b', 16), 'Uint8': ('B', 12), 'Sint16': ('h', 12), 'Uint16': ('H', 8),
            'Sint32': ('i', 8), 'Uint32': ('I', 6)}
# the scalar types a body can have that need no header to lay out, and each
# one's struct code
SCALARS = {**{ctype: code for ctype, (code, _) in INTEGERS.items()}, 'Float': 'f', 'char': 'c'}
# how many of a laid-out body's scalars go on a line, by struct code
LINE = {**dict(INTEGERS.values()), 'f': 6, 'c': 12}
# an array declaration's extents
EXTENT = re.compile(r'\[([^]\n]*)\]')


def element_size(ctype, types):
    """The size in bytes of an element of type ctype, its qualifiers aside.
    The Types lay out only a ctype that neither BODY_KINDS nor SCALARS
    holds. Raises LayoutError where they can't lay it out."""
    ctype = unqualified(ctype)[1]
    if ctype in BODY_KINDS:
        return KINDS[BODY_KINDS[ctype]].layout.size
    if ctype in SCALARS:
        return Scalar(SCALARS[ctype]).size
    return types.layout(ctype).size


def counted(extent, types):
    """An extent's count, from its text: a number, which needs no Types, or
    a constant expression the Types evaluate. Raises LayoutError where they
    can't."""
    tokens = spelled(extent)
    if len(tokens) == 1 and tokens[0][0].isdigit():
        return number(tokens[0])[0]
    return types.evaluate(tokens)


def nest(values, extents):
    """values, each an element's initializer, in nested braces, one level for
    each of extents. The extents run outer to inner, as a multidimensional
    array's inner extents do. None where an extent is 0 or doesn't divide
    the values it groups."""
    for extent in reversed(extents):
        if not extent or len(values) % extent:
            return None
        values = ['{\n    ' + ',\n    '.join(value.replace('\n', '\n    ') for value in values[at:at + extent]) + ',\n}'
                  for at in range(0, len(values), extent)]
    return values


class Bodies(Writer):
    """A body source's bodies as C. A null pointer is 0, and a pointer whose
    value the image doesn't take for an address is a cast number. Every
    other pointer is written with a name: one the source defines, or one the
    source or its headers declare.

    Bodies reads no file: the command hands it the facts. items is the
    source's data as an Items; names the image's names, by address; types
    the Types of the source and its headers; and functions the name of each
    function they declare. outside gives each name the source or its headers
    declare extern its type and its extent: the text between its brackets,
    '' for an array of no extent, or None for no array."""

    def __init__(self, items, names, types, outside, functions):
        super().__init__(items, names)
        self.types, self.outside, self.functions = types, outside, functions
        self.addresses = sorted(names)

    def extent(self, count, address):
        """An extent's count, as counted() gives it. Raises EmitError, naming
        address, where the Types can't evaluate it."""
        try:
            return counted(count, self.types)
        except LayoutError as error:
            raise EmitError(self.image, address, f'[{count}]: {error}') from None

    def ref(self, address, pointee):
        """A constant for the pointer at address, as a pointee *.

        - A nonzero value the image doesn't take for an address is written
          as a cast number: its windows and imports decide what is one.
        - A value outside the source's data is a name from outside, as
          elsewhere() gives it.
        - A pointer to the start of a union is written as the union's first
          member, where that is an array that converts to a pointee * with no
          cast.
        - A pointer into an array with inner extents, or a pointer to an
          array, names the element by its subscripts. A pointer to an array
          is cast as T (*)[N] where the array there isn't one of its
          pointee's element and extents.
        - Any other, null included, is Writer.ref()'s.

        Raises EmitError where the pointer reaches a datum whose declaration
        tools/data.py doesn't read, and where a pointer to an array holds no
        address, points inside an element, or points where the datum holds
        no array of its pointee."""
        value = self.word(address)
        if value and not self.image.is_address(value):
            # a value the image doesn't take for an address, such as an offset
            # a loader relocates
            if '[' in pointee:
                raise EmitError(self.image, address, f'a pointer to {pointee} holds 0x{value:08x}, no address')
            return f'({pointee} *)0x{value:08x}'
        index = bisect_right(self.starts, value) - 1
        if value and (index < 0 or not self.items[index].start <= value < self.items[index].end):
            # outside the source's data: the name at the value, or else the
            # nearest name below it, outside code, where the source or its
            # headers declare that name extern, and the offset past it.
            # elsewhere() fails unless the name's declared extent holds an
            # element at that offset
            name, offset = self.names.get(value), 0
            below = bisect_right(self.addresses, value) - 1
            if name is None and below >= 0 and self.names[self.addresses[below]] in self.outside \
                    and not self.image.in_code(value):
                name, offset = self.names[self.addresses[below]], value - self.addresses[below]
            return self.elsewhere(address, name or f'dat_{value:08x}', offset, pointee)
        item = self.items[index] if value else None
        if item is not None and item.declared is None:
            raise EmitError(self.image, address, f'a body points at {self.name(item.start)}, whose declaration '
                                                 "tools/data.py doesn't read")
        member = item.member if isinstance(item, Declared) else None
        if member and value == item.start and converts(member[0], pointee):
            # a union, by its first member, an array of what the pointer takes
            return f'{self.name(item.start)}.{member[1]}'
        # a pointer to an array: its element, and the extents it spans
        element_type, spanned = pointee.split('[')[0], pointee.count('[')
        if item is None or not item.inner and not spanned:
            return super().ref(address, pointee)
        if (value - item.start) % item.element:
            if spanned:
                raise EmitError(self.image, address,
                                f'a pointer to an array of {element_type} holds 0x{value:08x}, inside an element')
            return super().ref(address, pointee)
        # the element's subscripts, outer first. A pointer to an element takes
        # them all. A pointer to an array drops one from the end for each of
        # its pointee's extents, and each one it drops must be 0
        index, indices = (value - item.start) // item.element, []
        for count in reversed(item.inner):
            index, at = divmod(index, self.extent(count, address)) if index else (0, 0)
            indices.append(at)
        subscripts = [index, *reversed(indices)] if item.array else []
        kept = subscripts[:len(subscripts) - spanned]
        if spanned > len(subscripts) or any(subscripts[len(kept):]):
            raise EmitError(self.image, address, f'a pointer to an array of {element_type} holds 0x{value:08x}, where '
                                                 f'{self.name(item.start)} holds none')
        expression = f'&{self.name(item.start)}' + ''.join(f'[{at}]' for at in kept)
        # a cast only where C won't convert
        if not spanned:
            return expression if converts(item.ctype, element_type) else f'({pointee} *){expression}'
        # a pointer to an array: the extents of the array the expression
        # points at, a row's, or the whole datum's, whose outer extent counts
        # its rows. Its cast is T (*)[N]
        inner = [self.extent(count, address) for count in item.inner]
        shape = inner[len(kept) - 1:] if kept else [(item.end - item.start) // item.element // prod(inner), *inner]
        if converts(item.ctype, element_type) and self.spans(pointee, shape, address):
            return expression
        return f'({element_type} (*){pointee[len(element_type):]}){expression}'

    def spans(self, pointee, counts, address):
        """Whether an array of counts, outer first, has the extents pointee
        spells: as many, and each its count where both are given. Raises
        EmitError, naming address, where extent() does."""
        extents = EXTENT.findall(pointee)
        return len(extents) == len(counts) and all(count is None or not extent or self.extent(extent, address) == count
                                                   for extent, count in zip(extents, counts))

    def elsewhere(self, address, name, offset, pointee):
        """The constant for the pointer at address offset bytes into name, a
        name from outside the source, as a pointee *.

        A function the source or its headers declare is its name: ref() names
        one only at its start. A name the source or its headers declare
        extern is the name where it is an array, and its address otherwise. At
        a nonzero offset it is the address of its element there, which the
        declared extent must hold. Either is cast where C won't convert. A
        pointer to an array takes the name's address, cast as T (*)[N] unless
        the name is an array of the pointee's element, of the pointee's extent
        where both give one.

        Raises EmitError where a body points:

        - at a name neither the source nor its headers declare;
        - into a name with a pointer to an array;
        - into a name of a type the Types can't lay out;
        - into a name other than at an element of an array;
        - at an element of an array whose declarations give it no extent, or
          one that ends before the element."""
        if name in self.functions and name not in self.outside:
            return name
        if name not in self.outside:
            raise EmitError(self.image, address, f'a body points at {name}, which it doesn\'t declare')
        ctype, extent = self.outside[name]
        if '[' in pointee:
            if offset:
                raise EmitError(self.image, address, f'a body points {offset} bytes into {name} with a pointer to an '
                                                     'array')
            element_type = pointee.split('[')[0]
            if extent is not None and converts(ctype, element_type) \
                    and self.spans(pointee, [self.extent(extent, address) if extent else None], address):
                return f'&{name}'
            return f'({element_type} (*){pointee[len(element_type):]})&{name}'
        if not offset:
            expression = f'&{name}' if extent is None else name
        else:
            try:
                size = element_size(ctype, self.types)
            except LayoutError as error:
                raise EmitError(self.image, address, f'a body points into {name}, a {ctype}: {error}') from None
            if extent is None or offset % size:
                raise EmitError(self.image, address, f'a body points {offset} bytes into {name}, and not at an element '
                                                     'of it')
            if not extent or offset // size >= self.extent(extent, address):
                raise EmitError(self.image, address, f'a body points at element {offset // size} of {name}, and its '
                                                     f'declarations give {name} no extent that holds it')
            expression = f'&{name}[{offset // size}]'
        if converts(ctype, pointee):
            return expression
        return f'({pointee} *){expression}'

    def inc(self, name, item):
        """The text of name's .inc: item's values, every pointer resolved.
        Records in an array with inner extents go in the braces those extents
        take, and values other than records stay flat. A body that is no
        array is its one value, without a record's own braces.

        Raises EmitError where values(), ref() or elsewhere() does, as for a
        pointer to a name neither the source nor its headers declare, or to a
        datum of the source's whose type its declarations don't give, and
        where the elements don't fill the inner extents."""
        values, per_line = self.values(item)
        if item.inner and values[0].startswith('{'):
            # a record's braces would open an inner array instead, so records
            # take the inner extents' braces, and other values go flat
            values, per_line = nest(values, [self.extent(count, item.start) for count in item.inner]), 1
            if values is None:
                raise EmitError(self.image, item.start, f'{name}\'s {(item.end - item.start) // item.element} elements '
                                                        "don't fill its inner extents "
                                                        f'{"".join(f"[{count}]" for count in item.inner)}')
        if not item.array:
            # no array: the source's "= {" and "};" brace its one value, so a
            # record's own braces come off
            value = values[0].strip()
            value = value[1:-1].strip() if value.startswith('{') else value
            body = '\n'.join('    ' + line.strip() for line in value.splitlines() if line.strip())
        else:
            body = ',\n'.join(f'    {line}' for line in wrap(values, per_line))
        if body and not body.endswith(','):
            body += ','
        return body + '\n'
