"""An image's bytes for tools/data.py and tools/content.py, and what they
hold: the layouts of Ninja's structures and the game's lands and paths,
and the item a row of data.txt gives. A read raises ReadError where its
bytes lie outside the image, and defined() where a string row's bytes
aren't its text, a null and zeros.
"""

import copy
import re
import struct
from collections import namedtuple

from layout import Pointer, Scalar


# The image and its items
#
# An image is Memory: its bytes by linked address, its code ranges, its
# address windows, its fill byte, its imports, and the name its errors give
# it. An address window is a range where a word's value is taken as an
# address. An import is a definition in an image loaded beside 1ST_READ.BIN
# that 1ST_READ.BIN's sources use by name. A word's value is taken as an
# address too where it is an import's address. An Item is one definition:
# its bytes, its kind, and how C presents it.
#
# The table of structures gives each one's size, alignment and members'
# offsets. cdata.py's emitter reads it.


class Memory:
    """An image's bytes by their linked addresses, with its code ranges, its
    address windows, its fill byte, its imports' addresses, and the name its
    errors give it."""

    def __init__(self, name, payload, base, code=(), windows=(), fill=0, imports=frozenset()):
        self.name = name
        self.payload = payload
        self.base = base
        self.end = base + len(payload)
        self.code = code  # the ranges of code: the image's own, and any other image's it points into
        self.windows = windows  # the address windows: the ranges where a word's value is taken as an address
        self.fill = fill  # the linker's byte where the compiler writes none
        self.imports = imports  # its imports' addresses, where a word's value is taken as an address too

    def in_code(self, address):
        return any(start <= address < end for start, end in self.code)

    def is_address(self, word):
        return word in self.imports or any(start <= word < end for start, end in self.windows)


RAM = 0x1000000  # main RAM's size


def window(base):
    """Main RAM's range, low and end, in the mirror that base lies in. A word
    of the image's data is taken for an address where its value lies here."""
    low = base & ~(RAM - 1)
    return low, low + RAM


class ReadError(Exception):
    """Bytes at an address of an image that are not what the caller named."""

    def __init__(self, image, address, problem):
        super().__init__(f'{image.name}: 0x{address:08x}: {problem}')


class Image(Memory):
    """Memory, with reads that raise ReadError where their bytes lie outside
    the image."""

    def inside(self, address, size=1):
        if size < 0:
            raise ReadError(self, address, f'{size} bytes, and a size can\'t be negative')
        return self.base <= address and address + size <= self.end

    def u32(self, address):
        if not self.inside(address, 4):
            raise ReadError(self, address, 'can\'t be a Uint32, whose 4 bytes lie outside the image')
        return struct.unpack_from('<I', self.payload, address - self.base)[0]


class Struct:
    """A structure's or a union's size and alignment. Each member the emitter
    reads by name is an attribute that gives the member's offset."""

    def __init__(self, size, alignment=4, **offsets):
        self.size, self.alignment = size, alignment
        vars(self).update(offsets)


# The structures the emitter writes, as SHC lays them out. First come
# ninja.h's, an mdata by its count of factors. Then come a stage's path and
# its points, as src/path/path.h has them, and an act's land, its entries
# and its moving parts, as src/collision/world.h has them.
OBJECT = Struct(52, evalflags=0, model=4, position=8, angle=20, scale=32, child=44, sibling=48)
MODEL = Struct(40, points=0, normals=4, point_count=8, meshsets=12, materials=16, meshset_count=20,
               material_count=22, center=24, radius=36)
MESHSET = Struct(24, type_material=0, mesh_count=2, strips=4, attrs=8, normals=12, colors=16, uvs=20)
MATERIAL = Struct(20, diffuse=0, specular=4, exponent=8, texture=12, flags=16)
POINT = Struct(12)
COLOR = Struct(4, u=0, v=2)
MOTION = Struct(12, mdata=0, frames=4, bits=8, interpolation=10)
MDATA = {factors: Struct(8 * factors, pointers=0, counts=4 * factors) for factors in range(1, 5)}
MKEY_F = Struct(16, frame=0, key=4)
MKEY_A = Struct(16, frame=0, key=4)
MKEY_P = Struct(8, frame=0, key=4)
ACTION = Struct(8, object=0, motion=4)
CACTION = Struct(8, camera=0, motion=4)
TEXLIST = Struct(8, names=0, count=4)
TEXNAME = Struct(12, name=0, attributes=4, address=8)
TEXANIM = Struct(20, alignment=2)
STAGE_PATH = Struct(16, ends=0, count=2, length=4, points=8, step=12)
STAGE_PATH_POINT = Struct(20, ang_x=0, ang_z=2, length=4, pos=8)
LAND = Struct(36, count=0, motion_count=2, flags=4, users=6, clip=8, entries=12, motions=16, pvm=20, texlist=24,
              file=28, data=32)
LAND_ENTRY = Struct(36, center=0, radius=12, size_y=16, size_z=20, object=24, blocks=28, flags=32)
# a land's moving part. Its play is a union: the action, or under world.h's
# LAND_FILE the motion
LAND_MOTION = Struct(24, frame=0, speed=4, frames=8, object=12, play=16, texlist=20)

# each kind's C type, its element's layout, and whether it's one record rather
# than an array by default. ninja.h's kinds come first, then a land's and a
# path's, then the kinds of bytes no structure claims. An mdata isn't here:
# Item gives its type and layout by its factor count.
Kind = namedtuple('Kind', 'ctype layout single')
KINDS = {
    'object': Kind('NJS_OBJECT', OBJECT, True),
    'model': Kind('NJS_MODEL', MODEL, True),
    'motion': Kind('NJS_MOTION', MOTION, True),
    'action': Kind('NJS_ACTION', ACTION, True),
    'caction': Kind('NJS_CACTION', CACTION, True),
    'texlist': Kind('NJS_TEXLIST', TEXLIST, True),
    'meshset': Kind('NJS_MESHSET', MESHSET, False),
    'material': Kind('NJS_MATERIAL', MATERIAL, False),
    'points': Kind('NJS_POINT3', POINT, False),
    'normals': Kind('NJS_VECTOR', POINT, False),
    'vertices': Kind('NJS_POINT3', POINT, False),
    'strips': Kind('Sint16', Scalar('h'), False),
    'attrs': Kind('Uint32', Scalar('I'), False),
    'vcolor': Kind('NJS_COLOR', COLOR, False),
    'uv': Kind('NJS_COLOR', COLOR, False),
    'mkey_f': Kind('NJS_MKEY_F', MKEY_F, False),
    'mkey_a': Kind('NJS_MKEY_A', MKEY_A, False),
    'mkey_p': Kind('NJS_MKEY_P', MKEY_P, False),
    'texname': Kind('NJS_TEXNAME', TEXNAME, False),
    'texanim': Kind('NJS_TEXANIM', TEXANIM, False),
    'land': Kind('struct land_table', LAND, True),
    'land_entry': Kind('struct land_entry', LAND_ENTRY, False),
    'land_motion': Kind('struct land_motion', LAND_MOTION, False),
    'path': Kind('struct path', STAGE_PATH, True),
    'path_point': Kind('struct path_point', STAGE_PATH_POINT, False),
    'pointers': Kind('void *', Pointer('void'), False),
    'mixed': Kind('void *', Pointer('void'), False),
    'words': Kind('Uint32', Scalar('I'), False),
    'floats': Kind('Float', Scalar('f'), False),
    'halves': Kind('Uint16', Scalar('H'), False),
    'bytes': Kind('Uint8', Scalar('B'), False),
    'string': Kind('char', Scalar('c'), False),
}

# the kinds of bytes no structure claims, but for a string
GENERIC = {'pointers', 'mixed', 'words', 'floats', 'halves', 'bytes'}

# the types of the kinds that are one record by default
SINGLE = {kind.ctype for kind in KINDS.values() if kind.single}

# the kind of a name a source declares, by the type it declares it with
DECLARED = {'NJS_OBJECT': 'object', 'NJS_MODEL': 'model', 'NJS_MOTION': 'motion', 'NJS_ACTION': 'action',
            'NJS_CACTION': 'caction', 'NJS_TEXLIST': 'texlist', 'NJS_TEXNAME': 'texname',
            'NJS_MATERIAL': 'material', 'NJS_MESHSET': 'meshset', 'NJS_POINT3': 'points',
            'NJS_VECTOR': 'normals', 'NJS_MKEY_F': 'mkey_f', 'NJS_MKEY_A': 'mkey_a',
            'NJS_TEXANIM': 'texanim'}

# ninja.h spells one struct two ways
SAME = {'NJS_VECTOR': 'NJS_POINT3'}


def f32(word):
    return struct.unpack('<f', struct.pack('<I', word))[0]


def float_ok(word):
    """Whether word is zero or a normal float. This checks the word's shape
    alone: cdata.py's spell() can still find no literal for it, as for
    FLT_MAX."""
    exponent = (word >> 23) & 0xFF
    return exponent != 0xFF and (exponent != 0 or word & 0x7FFFFF == 0)


class Item:
    """One definition: elements of a kind, from start to end."""

    def __init__(self, start, end, kind, factors=0, text=None, declared=None, array=None, inner=()):
        self.start, self.end, self.kind = start, end, kind
        self.factors = factors  # an mdata's factor count, which gives its type and element size
        self.text = text  # a string's text, without its null
        self.declared = declared  # the type a source declares it with, or None
        self.array = not self.single if array is None else array  # written as an array, even of one
        # the texts of each element's inner extents, outer first, where a
        # source declares an array of arrays
        self.inner = inner
        self.vouched = None  # the path:line of the row of data.txt that vouches for it, or None

    def part(self, start, end):
        """The item from start to end, all else as it is."""
        part = copy.copy(self)
        part.start, part.end = start, end
        return part

    @property
    def ctype(self):
        if self.declared:
            return self.declared
        return f'NJS_MDATA{self.factors}' if self.kind == 'mdata' else KINDS[self.kind].ctype

    @property
    def element(self):
        return MDATA[self.factors].size if self.kind == 'mdata' else KINDS[self.kind].layout.size

    @property
    def alignment(self):
        return MDATA[self.factors].alignment if self.kind == 'mdata' else KINDS[self.kind].layout.alignment

    @property
    def single(self):
        return self.kind != 'mdata' and KINDS[self.kind].single


class Laid(Item):
    """An item whose type has no kind: a scalar, or a type a Types lays out."""

    single = False

    def __init__(self, start, end, layout, declared, array, inner):
        self.layout = layout
        super().__init__(start, end, 'laid', declared=declared, array=array, inner=inner)

    @property
    def element(self):
        return self.layout.size

    @property
    def alignment(self):
        return self.layout.alignment


class Declared(Item):
    """A datum the source defines with no body. A pointer from a body into it
    takes its name and type, and a pointer into one whose type is None
    fails."""

    def __init__(self, start, end, ctype, array, step=None, inner=(), member=None):
        super().__init__(start, end, 'bytes', declared=ctype, array=array, inner=inner)
        self.step = step  # an element's size, or None to take the datum as one element
        self.member = member  # None, or for a union whose first member is an array: its element's type and name

    @property
    def element(self):
        return self.step or self.end - self.start


# an mdata's kind as a row of data.txt gives it: mdata, then its count of
# factors
MDATA_KIND = re.compile(r'mdata([1-4])')


def defined(image, start, kind, count):
    """The Item a row of data.txt gives, from start: count elements of kind,
    as an array, or one where count is None. An mdata's kind gives its
    factors, and a string's text is its bytes to the first null, one to a
    character as latin-1 decodes them. None where kind is no kind of KINDS
    or an mdata's, as a C type isn't. Raises ReadError where a string's
    bytes hold no null, or a byte other than 0 after it, since its C zeroes
    the bytes past its text."""
    mdata = MDATA_KIND.fullmatch(kind)
    if not mdata and kind not in KINDS:
        return None
    factors = int(mdata[1]) if mdata else 0
    element = MDATA[factors].size if mdata else KINDS[kind].layout.size
    item = Item(start, start + element * (count or 1), 'mdata' if mdata else kind, factors, array=count is not None)
    if item.kind == 'string':
        null = image.payload.find(b'\0', start - image.base, item.end - image.base) \
            if image.inside(start, item.end - start) else -1
        if null < 0:
            raise ReadError(image, start, f"can't be a text of {item.end - start} bytes, its null among them")
        tail = image.payload[null:item.end - image.base]
        if tail.strip(b'\0'):
            stray = null + len(tail) - len(tail.lstrip(b'\0'))
            raise ReadError(image, image.base + stray, f"0x{image.payload[stray]:02x} follows the text's null, and "
                            'only zeros can')
        item.text = image.payload[start - image.base:null].decode('latin-1')
    return item
