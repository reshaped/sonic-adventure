"""The declarations check, the last check tools/verify.py runs: each name
make gcc's images define or take must have types that agree in every object
that declares it, as GCC's debugging information gives them.

failures() checks the images of a spec as tools/ld.py linked them. GCC_ABI,
verify.py's FLAGS, gives the -m and -fpack-struct= options make gcc compiles
its C with, which the check's model of GCC's SH ABI assumes. Each C unit's
DW_AT_producer must give each of them, and no other -m or -fpack-struct
option. It must give none of -fno-pack-struct, -fpcc-struct-return,
-fshort-enums and -fshort-wchar either, which change how a call passes a
value or how a type lays out. The check fails on a unit compiled otherwise.

The check reads each input of each image's final pass, as the pass's map
names it: each object, and each archive member the link takes. Of the
libgcc.a of the toolchain SH_ELF names, tools/sh-elf by default, it reads
the members' symbols but not their DWARF. GCC compiles libgcc under the
toolchain's defaults, which DW_AT_producer doesn't record. So a declaration
of a name the link binds to a member of libgcc.a fails.

GCC compiles each C object with DWARF. The DWARF gives each external name
the object defines, and each it declares that its source uses. For each, it
gives the type GCC compiled the name with, and the file and line of its
declaration. The check reads a declaration only where its object holds the
name's symbol. Where GCC optimizes away every use of a name, the object
holds no symbol of it, so its declaration goes unchecked. So does one that
only sizeof reads, which has no entry. GCC declares a routine it calls on
its own at <built-in>, without the type of its calls, as for the memcpy a
structure's copy calls. The check skips that declaration. An object
assembled from make gcc's assembly holds no DWARF, so a name that assembly
defines has only the types its C declarations give it. An object GCC
compiled, as its .comment says, that defines or takes a name and holds no
DWARF fails.

A declaration binds as the link binds its symbol, which is its C name or the
linkage name an asm label gives it. The link binds a name to the definition
in the input whose own symbol the final pass places at the name's address.
So a weak definition the link passes over is no definition of the name, and
the check compares it as one more declaration.

The check compares the declarations that bind each name, between the inputs
of one image and across images. An import binds a name an image takes to its
provider's symbol, so the check compares the image's declarations with the
provider's. An alias binds a second name to the image's own symbol, as
config/imports.txt gives it. So the check compares the alias's declarations
with that symbol's. Each declaration must agree with the C definition the
link binds its name to. Two declarations can each agree with the definition
and still disagree with each other, as int (*)(int) and int (*)(float) each
agree with a definition void *. So the declarations that agree with the
definition must also agree with each other. Where the link binds the name to
no C definition, as where assembly defines it, each two of its declarations
must agree. The link must also place each object a declaration names on that
declaration's alignment, whatever defines it: GCC's paired FMOVs of an
NJS_MATRIX fault off 8.

Two types of a name disagree where GCC's code through one can give another
value than the image's code does, or fault where it doesn't. The matching
build, make's own, compiles the game's and the SDK's sources with SHC. It
shows that SHC's code for each object, compiled from the object's own
declarations, gives the image's bytes. make gcc holds each image's content,
build/gen/IMAGE/content.c, to the image's bytes too. -fpack-struct=4 keeps
SHC's layout under GCC. So under GCC, each such object reads or places each
part of the name at the offset the image does. That holds however another
object lays the name out. Neither the matching build nor the content shows
anything of the code under gcc/ and tools/: the port's sources, SH4ZAM's,
and the toolchain's headers where SH_ELF is tools/sh-elf.

A declaration is matched where its object's source lies outside gcc/ and
tools/. Its own place, and the place of each typedef, structure, union,
enumeration and member its type reaches, must lie outside them too. Each
such place must be a line of a file of the tree that the check can read, on
no guard's GCC side, as csource.py's gcc_side() finds it. A type reaches
itself, and each type its members, elements, pointees and parameters reach.
A structure or a union DWARF declares without its members counts for none.
Any other declaration is unmatched, as one whose file or line DWARF doesn't
give is. Two matched declarations disagree only where:

- one reaches past the bytes of the name's definition, or aligns to more
  than the definition does. GCC's link gives the definition other neighbours
  than the image's, and may place it where only its own alignment holds, so
  a wider load faults;
- a byte one type's layout covers is padding in the other. C gives padding
  no value, so GCC's code may leave or write another byte there than SHC's
  did;
- a byte both layouts cover is const or volatile in one alone. GCC's code
  may fold a read of a const byte, or keep its value across a call. It may
  also keep a value it read from a byte that isn't volatile where the source
  reads the byte again;
- two pointers to data that share a byte point to volatile data in one
  alone. GCC's code reads volatile data each time the source does;
- a call passes or returns a value in other registers or memory, or with
  another extension. That covers a call through two pointers to code that
  share a byte.

That is the matched rule. It is the check's policy for matched declarations,
not a proof that two C types are interchangeable. It rests on
-fno-strict-aliasing too, which GCC_AS_SHC gives each compile of the tree's
C, and which the check doesn't read from DW_AT_producer.

The matching build shows nothing of an unmatched declaration's layout. So
where either of two declarations is unmatched, the strict rule holds their
types to more. Past each one's outer extent, their elements must hold the
same parts at the same offsets. The elements of int[2][3] and int[6] don't,
nor do an int and a structure of two shorts. Two parts are the same only
with the same qualifiers. Two scalars are the same part where they have one
size and one kind: float, signed, unsigned or boolean. An enumeration's kind
is its integer's. So an int and a float aren't the same part. Two pointers
to data are the same part where they point to the same type, which int **
and volatile int ** don't. Two structures or unions are the same type where
they share a tag, or have none and the same size and members. Two pointers
to code are the same part where calls through them pass and return each
value alike, and both or neither give a prototype.

An object's type is its alignment and the parts its layout covers. A part is
a scalar or a pointer, with its qualifiers, and a qualifier on a structure
or a union lies on each part of it. An alignment DWARF gives holds even past
a word, as NJS_MATRIX's 8 does. Otherwise a scalar or a pointer aligns to
its size, and an array as its element. A structure or a union aligns to its
most aligned member. Each aligns to a word at most, as -fpack-struct=4 caps
it.

Under the matched rule, two parts that share a byte agree where they have
the same qualifiers, whatever their kinds and sizes. GCC's code loads and
stores each byte as it is, and -fno-strict-aliasing keeps GCC from assuming
that accesses through two types reach different bytes. So int[2][3] agrees
with int[6] and with a definition int[2][4]. A structure of two shorts
agrees with a definition int. An int disagrees with a definition short[2],
which aligns to 2. Two pointers to data must also both or neither point to
volatile data. Two pointers to code must also point to functions that agree.
A pointer to code agrees with a pointer to data, through which no call goes.
The const of the data a pointer points to changes no value GCC's code reads,
since another pointer may write the data. A byte agrees where each part over
it agrees with some part of the other type over it. So a member of a union
needs only one part of the other type to agree with. A declaration without
an outer extent covers one element at least, and an incomplete structure
covers no byte. Where no C object defines the name, the check compares no
size and no alignment.

A function's type is how a call passes its parameters, in order, and returns
its value. It also says whether the function takes more, as a variadic one
does. GCC's SH code passes a 4-byte float in a floating register, and under
-m4-single-only a double is such a float. It passes a structure of 4 bytes
the same way where the structure's one member with bytes lies at its start
and passes so. Such a member is such a float, such a structure, or an array
of one such element. GCC passes an integer narrower than 32 bits in a
general register, with the extension its caller or callee relies on. A
boolean passes as an unsigned integer. GCC passes any other value by its
size alone: an integer of 32 bits or more, whatever its signedness; an
8-byte float; a pointer, whatever it points to; any other structure; and
every union. It returns a value the same way, but it returns a structure or
a union through memory where GCC gives it no integer mode. That is where its
size isn't 1, 2, 4 or 8 bytes, or it aligns to less than both its size and a
word.

A K&R definition takes its parameters as C promotes them. A declaration
without a prototype agrees with a type by its value. It agrees with the
type's parameters where the type takes no more, and none of them is an
integer narrower than an int. A call through such a declaration passes each
argument as C promotes it. That is C17 6.7.6.3p15's rule, but for a float,
which promotes to a double that -m4-single-only passes alike. A function and
an object disagree, since the object reads the function's code as data.

A typedef names a type and makes none. restrict, which SH4ZAM's parameters
take, doesn't change how a call passes a value, so the check reads past it
in the values a call passes and returns. It fails on restrict where it reads
a type as storage: in laying out an object, or a structure or a union a
call passes or returns.

tools/dwarf.py reads each object's DWARF as GCC and GNU as write it for make
gcc, and fails on DWARF it doesn't read. An extent DWARF gives by a
reference or an expression varies, as a variable-length array's does. C
allows such an extent in an external name's type only behind a pointer. The
check reads it there as an unknown extent, and spells it [*]. The check
fails on an object, naming it, where dwarf.py doesn't read its DWARF, or
where its DWARF holds:

- on an entry the check reads, an attribute in a form it doesn't take for
  that attribute, such as a string by index. A reference outside its unit
  counts too, and so does a string its section doesn't hold whole: one
  past the section's end, or without its NUL;
- a reference the check follows to no entry, or references it follows that
  nest past Python's recursion limit, as a cycle of them does;
- a bit-field or an _Atomic type;
- a base type that is no float, boolean or integer, such as a _Complex;
- restrict on a type the check reads as storage, or on the target of a
  pointer there;
- a variable or a member without a type, or a type without a size where the
  check reads one;
- an array without a subrange, or with a negative extent;
- a member packed below its alignment.

No object make gcc links holds a variable or a member without a type. Nor
does one hold a type without a size, an array without a subrange or with a
negative extent, or a member packed below its alignment.

config/declarations.txt admits the problems where GCC's code still gives the
image's value, or where the game never runs the code. A row gives a name, a
file that declares it, and why. It admits each problem of that file's
declaration of the name, in every image that links the file. For a header,
it admits the declaration in each object that includes the header. Where two
declarations disagree with each other rather than with the name's
definition, a row for either admits their problem. A row that admits nothing
fails where the link holds its declaration, or where the check reads an
input compiled from its file. So a row fails where that input doesn't
declare its name, and passes where a partial link leaves the input out. A
row also fails where its file doesn't exist, and where another row gives its
name and file.

A failure names an image, and how many more give the same text. Then it
names the name, its two declarations and the types they give it. It names a
declaration by its file and line. Where the declaration's object has another
source, as a header's declaration does, it names that too. For two objects,
or two functions C spells alike, it ends with where their types differ. An
object differs from its definition by their sizes where it reaches past the
definition's bytes, and by their alignments where it aligns to more.
Otherwise two objects the strict rule fails differ by their elements. Two
the matched rule fails differ at two parts that share a byte and disagree,
one of each, and either part can be padding. Two functions differ at their
values where those differ. Otherwise they differ at the first of a prototype
one alone gives, their counts of parameters, more one alone takes, and a
parameter a call passes otherwise. A declaration off its alignment names its
alignment and the address the link places its name at. A declaration the
link binds to libgcc.a names the member that defines it.

A check that passes shows that these rules hold for each declaration GCC's
DWARF gives, not that every access or call the program makes agrees.
"""

import math
import os
from collections import namedtuple
from dataclasses import dataclass
from functools import cached_property
from itertools import combinations
from pathlib import Path

import dwarf
import elf
import gcclink
from csource import blanked, gcc_side
from dwarf import FLAGS, REFERENCES, STRINGS, Ate, At, Form, Tag, Unread
from tables import rows
from tool import content, fail

DECLARATIONS = Path('config/declarations.txt')
# the bytes of a general register
WORD = 4
# the bytes of the float GCC passes in a floating register, a double's too
# under -m4-single-only
FLOAT_SIZE = 4
# the sizes of the integer modes GCC can give a structure or a union
MODE_SIZES = (1, 2, 4, 8)
# options outside GCC_ABI that change how a call passes a value or how a
# type lays out. The check's model of GCC's SH ABI assumes none of them
FOREIGN = frozenset(('-fno-pack-struct', '-fpcc-struct-return', '-fshort-enums', '-fshort-wchar'))
# the tags of the types an object's storage holds, which type_entry() takes.
# A caller that reads restrict or a function's type gives type_entry() those
# tags too
TYPES = (Tag.ARRAY_TYPE, Tag.ENUMERATION_TYPE, Tag.POINTER_TYPE, Tag.STRUCTURE_TYPE, Tag.TYPEDEF, Tag.UNION_TYPE,
         Tag.BASE_TYPE, Tag.CONST_TYPE, Tag.VOLATILE_TYPE)
RECORDS = (Tag.STRUCTURE_TYPE, Tag.UNION_TYPE)
# a base type's kind, by its encoding
KINDS = {Ate.FLOAT: 'float', Ate.BOOLEAN: 'boolean', Ate.SIGNED: 'signed', Ate.SIGNED_CHAR: 'signed',
         Ate.UNSIGNED: 'unsigned', Ate.UNSIGNED_CHAR: 'unsigned'}
# the directories of the code that neither the matching build nor the
# content holds to an image's bytes: the port's sources under gcc/, and
# SH4ZAM's under tools/, where the toolchain's headers lie too by default
GCC_ONLY = ('gcc/', 'tools/')
# the forms of a constant the check reads
CONSTANTS = frozenset((Form.DATA1, Form.DATA2, Form.DATA4, Form.DATA8, Form.DATA16, Form.SDATA, Form.UDATA,
                       Form.IMPLICIT_CONST))
# the attributes that give an array's extent
EXTENTS = frozenset((At.UPPER_BOUND, At.COUNT))
# the forms of an extent that varies: a reference or an expression
VARYING = REFERENCES | {Form.EXPRLOC}
# the forms the check takes for each attribute it reads
READ = {At.NAME: STRINGS, At.LINKAGE_NAME: STRINGS, At.COMP_DIR: STRINGS, At.PRODUCER: STRINGS,
        At.ABSTRACT_ORIGIN: REFERENCES, At.STMT_LIST: frozenset((Form.SEC_OFFSET,)), At.TYPE: REFERENCES,
        At.SPECIFICATION: REFERENCES, **dict.fromkeys((At.PROTOTYPED, At.DECLARATION, At.EXTERNAL), FLAGS),
        **dict.fromkeys(EXTENTS, CONSTANTS | VARYING),
        **dict.fromkeys((At.BYTE_SIZE, At.BIT_SIZE, At.DATA_MEMBER_LOCATION, At.DECL_FILE, At.DECL_LINE, At.ENCODING,
                         At.INLINE, At.DATA_BIT_OFFSET, At.ALIGNMENT), CONSTANTS)}


@dataclass(frozen=True)
class Void:
    """The type void: the value of a function that returns none, or the
    storage of no byte."""

    def __str__(self):
        return 'void'


@dataclass(frozen=True)
class Incomplete:
    """A structure or a union that DWARF declares without its members, which
    covers no byte."""


@dataclass(frozen=True)
class Scalar:
    """An integer, a float or an enumeration in an object's storage. kind is
    float, signed, unsigned or boolean, and an enumeration's is its
    integer's. Only the strict rule reads kind."""
    size: int
    qualifiers: frozenset
    kind: str

    def __str__(self):
        return f'a {qualified(self.qualifiers)}scalar of {self.size} byte{"s" if self.size > 1 else ""}'


@dataclass(frozen=True)
class DataPointer:
    """A pointer to data in an object's storage. volatile says the data is
    volatile, which GCC's code reads and writes each time the source does.
    target is the data's type, as Unit.key() gives it, which only the
    strict rule reads."""
    size: int
    qualifiers: frozenset
    volatile: bool
    target: tuple

    def __str__(self):
        return f'a {qualified(self.qualifiers)}pointer to {"volatile " if self.volatile else ""}data'


@dataclass(frozen=True)
class CodePointer:
    """A pointer to code in an object's storage, with the Function it
    points to."""
    size: int
    qualifiers: frozenset
    function: 'Function'

    def __str__(self):
        return f'a {qualified(self.qualifiers)}pointer to code {self.function}'


@dataclass(frozen=True)
class Array:
    """An array: its element's storage, and each extent, outermost first.
    An extent is None where DWARF gives none, or one that varies."""
    element: object
    extents: tuple


@dataclass(frozen=True)
class Record:
    """A structure or a union: its bytes, and each member as its offset and
    its storage. Each member's parts take the record's qualifiers."""
    size: int
    members: tuple


@dataclass(frozen=True)
class Int:
    """A value a call passes in general registers by its size alone, as an
    integer of that size. Pointers and 8-byte floats pass so, and so do the
    structures and unions a call passes in no floating register."""
    size: int

    def __str__(self):
        return 'int' if self.size == WORD else f'{self.size}-byte int'


@dataclass(frozen=True)
class Narrow:
    """An integer narrower than 32 bits, which a call passes in a general
    register extended by its signedness."""
    size: int
    signed: bool

    def __str__(self):
        return f'{"signed" if self.signed else "unsigned"} {self.size}-byte int'


@dataclass(frozen=True)
class Float:
    """A 4-byte float, which a call passes in a floating register."""

    def __str__(self):
        return 'float'


@dataclass(frozen=True)
class Memory:
    """A value of size bytes that a call returns through memory."""
    size: int

    def __str__(self):
        return f'{self.size} bytes in memory'


@dataclass(frozen=True)
class Function:
    """How a call passes and returns a function's values. value is the
    value it returns. parameters holds each parameter's value in order, or
    is None for a declaration or a function type without a prototype. more
    says the function takes more, as a ... does. Each value is a Void, an
    Int, a Narrow, a Float or a Memory."""
    value: object
    parameters: 'tuple | None'
    more: bool

    def __str__(self):
        if self.parameters is None:
            return f'{self.value} (?)'
        parameters = [*map(str, self.parameters), *(['...'] if self.more else [])]
        return f'{self.value} ({", ".join(parameters)})'

    def promotes_alike(self):
        """Whether a call through a declaration without a prototype passes
        each parameter as the function takes it. It does where the function
        takes no more and no parameter is a Narrow, or gives no parameters."""
        return self.parameters is None or not (self.more or any(isinstance(each, Narrow) for each in self.parameters))

    def agrees(self, other):
        """Whether calls through self and through other pass and return
        values alike: the same value, the same parameters in order, and more
        in both or neither. Where either has no prototype, their parameters
        agree where both promote alike."""
        if self.value != other.value:
            return False
        if self.parameters is None or other.parameters is None:
            return self.promotes_alike() and other.promotes_alike()
        return (self.parameters, self.more) == (other.parameters, other.more)

    def difference(self, other):
        """A phrase that names where self and other, two functions that
        disagree, differ. It names the first of these that differs: the
        value; a prototype, which one alone gives; the count of parameters;
        more, which one alone takes; or the first parameter a call passes
        otherwise."""
        if self.value != other.value:
            return f'the value returns as {self.value} against {other.value}'
        if (self.parameters is None) != (other.parameters is None):
            return f'{"the first" if self.parameters is None else "the second"} gives no prototype'
        if len(self.parameters) != len(other.parameters):
            count = len(self.parameters)
            return f'{count} parameter{"" if count == 1 else "s"} against {len(other.parameters)}'
        if self.more != other.more:
            return f'{"the first" if self.more else "the second"} alone takes more'
        place, mine, theirs = next((place, mine, theirs) for place, (mine, theirs)
                                   in enumerate(zip(self.parameters, other.parameters), 1) if mine != theirs)
        return f'parameter {place} passes as {mine} against {theirs}'


@dataclass(frozen=True)
class Object:
    """An object's type: its storage, and the bytes it aligns to."""
    storage: object
    alignment: int

    def difference(self, other, defined, matched):
        """A phrase that names where self and other differ, or None where
        they agree. defined says other is the name's definition, and matched
        that the matched rule judges the two, not the strict rule. Against a
        definition, self differs by its size where it reaches past the
        definition's bytes, and by its alignment where it aligns to more.
        The strict rule then fails two whose elements past each outer extent
        differ, as element() gives them. The matched rule fails a byte where
        a part of one agrees with no part of the other, as clash() finds
        it."""
        least, extent = reach(self.storage), size(other.storage)
        if defined and None not in (least, extent) and least > extent:
            reached = f'{least} bytes' if size(self.storage) is not None else f'an element of {least} bytes'
            return f'{reached} against {extent}'
        if defined and self.alignment > other.alignment:
            return f'aligned to {self.alignment} bytes against {other.alignment}'
        if not matched and element(self.storage) != element(other.storage):
            return 'elements of other types'
        (first, _), (second, _) = flat(self.storage), flat(other.storage)
        # two runs of one element from offset 0 put each part over its twin,
        # where clash() would find nothing
        if first == second:
            return None
        # compare up to the smaller size known. Where neither is known, the
        # two repeat their elements in step after the least common multiple
        # of the elements' sizes
        known = [each for each in (size(self.storage), extent) if each is not None]
        end = min(known) if known else size(first) * size(second) // math.gcd(size(first), size(second))
        found = clash(self.storage, other.storage, end)
        if found is None:
            return None
        *parted, byte = found
        offsets = [byte if leaf is None else leaf.offset for leaf in parted]
        words = ['padding' if leaf is None else str(leaf.part) for leaf in parted]
        if offsets[0] == offsets[1]:
            return f'at offset {offsets[0]}, {words[0]} against {words[1]}'
        return f'{words[0]} at offset {offsets[0]} against {words[1]} at offset {offsets[1]}'


def qualified(qualifiers):
    """qualifiers as text, in name order, each followed by a space."""
    return ''.join(f'{qualifier} ' for qualifier in sorted(qualifiers))


def file_line(file, line):
    """A declaration's place as file:line, with ? for either DWARF doesn't
    give."""
    return f'{file or "?"}:{line or "?"}'


class Unit:
    """A compilation unit of an object's DWARF: its entries by offset, its
    source, the files its line table names by index, and its
    DW_AT_producer. Its methods raise Unread on DWARF the check doesn't
    read."""

    def __init__(self, dies, source, files, producer=None):
        self.dies, self.source, self.files, self.producer = dies, source, files, producer
        # what storage(), alignment(), key() and origins() have found, by
        # their arguments
        self.storages, self.alignments, self.keys, self.origin_sets = {}, {}, {}, {}

    def entry(self, ref):
        """The entry at offset ref. A reference to no entry is Unread, and
        so is an entry that holds an Unread."""
        die = self.dies.get(ref)
        if die is None:
            raise Unread(f"its DWARF refers to an entry at {ref:#x}, which it doesn't hold")
        if die.unread:
            raise die.unread
        return die

    def children(self, die, tag):
        """The children of die whose tag is tag."""
        return [self.entry(offset) for offset in die.children if self.dies[offset].tag == tag]

    def type_entry(self, ref, also=()):
        """The type entry at offset ref, or None for void. An entry whose tag
        is in neither TYPES nor also is Unread, as an _Atomic type's is."""
        die = None if ref is None else self.entry(ref)
        if die is not None and die.tag not in (*TYPES, *also):
            raise Unread(f"its DWARF gives a type of tag {die.tag:#x}, which decls.py doesn't read")
        return die

    def place(self, die):
        """The file and the line of die's declaration, each None where DWARF
        gives none. The file is as canonical() names it."""
        index, line = die.attrs.get(At.DECL_FILE), die.attrs.get(At.DECL_LINE)
        file = self.files[index] if index is not None and 0 <= index < len(self.files) else None
        return file, line if isinstance(line, int) and line > 0 else None

    def where(self, die):
        """The place of die's declaration, as file_line() spells it."""
        return file_line(*self.place(die))

    def origins(self, ref):
        """The places of the typedefs, structures, unions, enumerations and
        members the type ref reaches, as place() gives them: itself, and
        those its members, elements, pointees and parameters reach. A
        structure or a union DWARF declares without its members lays nothing
        out, so its place doesn't count."""
        if ref not in self.origin_sets:
            self.gather(ref, {}, [])
        return self.origin_sets[ref]

    def gather(self, ref, depths, pending):
        """The places the type ref reaches, as far as the walk has found
        them, and the least depth of a pending type that ref reaches.
        pending holds the types the walk has entered whose places
        origin_sets doesn't hold yet, and depths gives each one's index
        there. Types that reach each other, as a list's node and its pointer
        do, reach the same places. So the walk keeps their places once it
        leaves the first of them it entered: the one that reaches no type
        pending before it."""
        depth = depths[ref] = len(pending)
        pending.append(ref)
        found, least = set(), depth
        die = self.type_entry(ref, (Tag.RESTRICT_TYPE, Tag.SUBROUTINE_TYPE))
        reached = []
        if die is not None:
            if die.tag in (Tag.TYPEDEF, *RECORDS, Tag.ENUMERATION_TYPE) and At.DECLARATION not in die.attrs:
                found.add(self.place(die))
            reached.append(die.attrs.get(At.TYPE))
            for child in die.children:
                if self.dies[child].tag in (Tag.MEMBER, Tag.FORMAL_PARAMETER):
                    entry = self.entry(child)
                    if entry.tag == Tag.MEMBER:
                        found.add(self.place(entry))
                    reached.append(entry.attrs.get(At.TYPE))
        for each in reached:
            if each in self.origin_sets:
                found |= self.origin_sets[each]
            elif each in depths:
                least = min(least, depths[each])
            else:
                places, back = self.gather(each, depths, pending)
                found |= places
                least = min(least, back)
        if least == depth:
            for each in pending[depth:]:
                self.origin_sets[each] = frozenset(found)
            del pending[depth:]
        return found, least

    def unqualified(self, ref):
        """The entry the type ref names, past its typedefs and qualifiers,
        restrict among them."""
        die = self.type_entry(ref, (Tag.RESTRICT_TYPE,))
        if die is not None and die.tag in (Tag.TYPEDEF, Tag.CONST_TYPE, Tag.VOLATILE_TYPE, Tag.RESTRICT_TYPE):
            return self.unqualified(die.attrs.get(At.TYPE))
        return die

    def storage(self, ref, qualifiers=frozenset()):
        """The type ref names as an object's storage, with qualifiers on each
        of its parts: a Scalar, a DataPointer or a CodePointer; an Array; a
        Record, or an Incomplete for one DWARF declares without its members;
        or a Void."""
        key = (ref, qualifiers)
        if key not in self.storages:
            self.storages[key] = self.stored(self.type_entry(ref), qualifiers)
        return self.storages[key]

    def stored(self, die, qualifiers):
        """The storage of the type entry die, as storage() gives it."""
        if die is not None and die.tag == Tag.TYPEDEF:
            return self.storage(die.attrs.get(At.TYPE), qualifiers)
        if die is not None and die.tag in (Tag.CONST_TYPE, Tag.VOLATILE_TYPE):
            qualifier = 'const' if die.tag == Tag.CONST_TYPE else 'volatile'
            return self.storage(die.attrs.get(At.TYPE), qualifiers | {qualifier})
        if die is None:
            return Void()
        if die.tag == Tag.ARRAY_TYPE:
            return Array(self.storage(die.attrs.get(At.TYPE), qualifiers), tuple(self.extents(die)))
        if die.tag in RECORDS:
            if At.DECLARATION in die.attrs:
                return Incomplete()
            return Record(self.sized(die), tuple(self.members(die, qualifiers)))
        if die.tag == Tag.POINTER_TYPE:
            return self.pointer(die, qualifiers)
        return Scalar(self.sized(die), qualifiers, self.kind(die))

    def sized(self, die):
        """The bytes DWARF gives the type die. A type without them is
        Unread."""
        if At.BYTE_SIZE not in die.attrs:
            raise Unread(f"its DWARF gives {die.attrs.get(At.NAME, 'a type')} no size, which decls.py doesn't read")
        return die.attrs[At.BYTE_SIZE]

    def kind(self, die):
        """The kind of the base type or enumeration die, as Scalar gives it.
        An enumeration takes its integer's kind, or signed where DWARF gives
        it none. Any other encoding is Unread, as a _Complex's is."""
        if die.tag == Tag.ENUMERATION_TYPE:
            under = self.unqualified(die.attrs.get(At.TYPE)) if At.TYPE in die.attrs else None
            return 'signed' if under is None else self.kind(under)
        encoding = die.attrs.get(At.ENCODING)
        if encoding not in KINDS:
            raise Unread(f"its DWARF gives {die.attrs.get(At.NAME, 'a type')} the encoding {encoding}, which "
                         "decls.py doesn't read")
        return KINDS[encoding]

    def key(self, ref):
        """The type ref names, as the strict rule tells types apart. It
        looks past typedefs and restrict, and keeps each const and volatile.
        A base type or an enumeration is its kind and size. A pointer is
        what it points to, an array its element and extents, and a function
        what function() gives. A structure or a union is its tag, so a
        complete one is the same type as one declared alone, and as one
        another unit defines otherwise. One without a tag is its size and
        each member's offset and type. Its members never lead back to it,
        since C can name it only after its body."""
        if ref not in self.keys:
            die = self.type_entry(ref, (Tag.RESTRICT_TYPE, Tag.SUBROUTINE_TYPE))
            if die is None:
                found = ('void',)
            elif die.tag in (Tag.TYPEDEF, Tag.RESTRICT_TYPE):
                found = self.key(die.attrs.get(At.TYPE))
            elif die.tag in (Tag.CONST_TYPE, Tag.VOLATILE_TYPE):
                found = ('const' if die.tag == Tag.CONST_TYPE else 'volatile', self.key(die.attrs.get(At.TYPE)))
            elif die.tag in RECORDS and At.NAME in die.attrs:
                found = (int(die.tag), die.attrs[At.NAME])
            elif die.tag in RECORDS:
                members = tuple((member.attrs.get(At.DATA_MEMBER_LOCATION, 0), self.key(member.attrs.get(At.TYPE)))
                                for member in self.children(die, Tag.MEMBER))
                found = (int(die.tag), None, die.attrs.get(At.BYTE_SIZE), members)
            elif die.tag == Tag.POINTER_TYPE:
                found = ('pointer', self.key(die.attrs.get(At.TYPE)))
            elif die.tag == Tag.ARRAY_TYPE:
                found = ('array', self.key(die.attrs.get(At.TYPE)), tuple(self.extents(die)))
            elif die.tag == Tag.SUBROUTINE_TYPE:
                found = ('function', self.function(die))
            else:
                found = ('scalar', self.kind(die), self.sized(die))
            self.keys[ref] = found
        return self.keys[ref]

    def members(self, die, qualifiers):
        """Each member of the structure or union die, as its offset and its
        storage with qualifiers. A bit-field, a member without a type and
        one at a negative offset are Unread."""
        for member in self.children(die, Tag.MEMBER):
            where = self.where(member)
            if At.BIT_SIZE in member.attrs or At.DATA_BIT_OFFSET in member.attrs:
                raise Unread(f"{where} declares a bit-field, which decls.py doesn't read")
            if At.TYPE not in member.attrs:
                raise Unread(f"{where} declares a member without a type, which decls.py doesn't read")
            offset = member.attrs.get(At.DATA_MEMBER_LOCATION, 0)
            if offset < 0:
                raise Unread(f"{where} declares a member at offset {offset}, which decls.py doesn't read")
            yield offset, self.storage(member.attrs[At.TYPE], qualifiers)

    def pointer(self, die, qualifiers):
        """The pointer die as storage with qualifiers: a CodePointer where it
        points to a function, else a DataPointer."""
        target, volatile = self.pointee(die.attrs.get(At.TYPE))
        if target is not None and target.tag == Tag.SUBROUTINE_TYPE:
            return CodePointer(self.sized(die), qualifiers, self.function(target))
        return DataPointer(self.sized(die), qualifiers, volatile, self.key(die.attrs.get(At.TYPE)))

    def pointee(self, ref):
        """The entry a pointer's type ref names, past its typedefs, const and
        volatile, and whether any of those is volatile."""
        die = self.type_entry(ref, (Tag.SUBROUTINE_TYPE,))
        if die is None or die.tag not in (Tag.TYPEDEF, Tag.CONST_TYPE, Tag.VOLATILE_TYPE):
            return die, False
        target, volatile = self.pointee(die.attrs.get(At.TYPE))
        return target, volatile or die.tag == Tag.VOLATILE_TYPE

    def extents(self, die, varying=None):
        """Each extent of the array die, outermost first: its upper bound
        plus 1 or its count, None where it gives neither, or varying where
        the extent varies. An array without a subrange, or with a negative
        extent, is Unread."""
        subranges = self.children(die, Tag.SUBRANGE_TYPE)
        if not subranges:
            raise Unread("its DWARF gives an array no subrange, which decls.py doesn't read")
        for subrange in subranges:
            bound, count = subrange.attrs.get(At.UPPER_BOUND), subrange.attrs.get(At.COUNT)
            if VARYING.intersection(subrange.forms.get(each) for each in EXTENTS):
                yield varying
                continue
            extent = count if bound is None else bound + 1
            if extent is not None and extent < 0:
                raise Unread(f"its DWARF gives an array the extent {extent}, which decls.py doesn't read")
            yield extent

    def passed(self, ref, promoted=False, returned=False):
        """How GCC's SH code passes a value of the type ref names in a call,
        or, where returned, returns it. A 4-byte float, and a structure that
        floating() takes, pass as a Float. An integer narrower than 32 bits
        passes as a Narrow, or where promoted, as the int C promotes it to.
        void is a Void. Any other value passes as an Int of its size. A
        returned structure or union is a Memory instead where GCC gives it
        no integer mode: where its size isn't one of MODE_SIZES, or it
        aligns to less than both its size and a word. A structure or a union
        is Unread where storage() or alignment() can't lay it out."""
        die = self.unqualified(ref)
        if die is None:
            return Void()
        if die.tag == Tag.POINTER_TYPE:
            return Int(self.sized(die))
        if die.tag in RECORDS:
            self.storage(ref)
            width, aligned = self.sized(die), self.alignment(ref)
            if self.floating(die):
                return Float()
            if returned and (width not in MODE_SIZES or min(aligned, WORD) < min(width, WORD)):
                return Memory(width)
            return Int(width)
        kind, width = self.kind(die), self.sized(die)
        if kind == 'float' and width == FLOAT_SIZE:
            return Float()
        if kind == 'float' or width >= WORD:
            return Int(width)
        return Int(WORD) if promoted else Narrow(width, kind == 'signed')

    def floating(self, die):
        """Whether GCC passes a value of the type die, past its qualifiers,
        in a floating register. It does for a 4-byte float, and for an array
        of one element that it passes so. It does for a structure of 4 bytes
        too where its only member with bytes lies at its start and passes
        so."""
        if die is None:
            return False
        if die.tag == Tag.ARRAY_TYPE:
            return (math.prod(extent or 0 for extent in self.extents(die)) == 1
                    and self.floating(self.unqualified(die.attrs.get(At.TYPE))))
        if die.tag == Tag.STRUCTURE_TYPE:
            members = [member for member in self.children(die, Tag.MEMBER)
                       if size(self.storage(member.attrs.get(At.TYPE))) != 0]
            return (At.DECLARATION not in die.attrs and die.attrs.get(At.BYTE_SIZE) == FLOAT_SIZE and len(members) == 1
                    and members[0].attrs.get(At.DATA_MEMBER_LOCATION, 0) == 0
                    and self.floating(self.unqualified(members[0].attrs.get(At.TYPE))))
        return (die.tag == Tag.BASE_TYPE and die.attrs.get(At.ENCODING) == Ate.FLOAT
                and die.attrs.get(At.BYTE_SIZE) == FLOAT_SIZE)

    def alignment(self, ref, least=1):
        """The bytes a variable or a member of the type ref aligns to: least,
        the alignment DWARF gives the variable or the member, or the type's
        own where that is more. An alignment DWARF gives the type, as an
        aligned attribute writes one, holds past a word, as NJS_MATRIX's 8
        does. Otherwise a scalar or a pointer aligns to its size, and a
        structure or a union to its most aligned member. -fpack-struct=4
        caps each at a word. void aligns to 1. A typedef or a qualified type
        aligns as the type it names, and an array as its element. A member
        whose offset isn't a multiple of its alignment, up to a word, is
        Unread: packing below 4 gives one."""
        if ref not in self.alignments:
            self.alignments[ref] = self.aligned(self.type_entry(ref, (Tag.RESTRICT_TYPE,)))
        return max(least, self.alignments[ref])

    def aligned(self, die):
        """The alignment of the type entry die, as alignment() gives it."""
        if die is None:
            return 1
        if die.tag in RECORDS:
            found = 1
            for member in self.children(die, Tag.MEMBER):
                aligns = min(self.alignment(member.attrs.get(At.TYPE), member.attrs.get(At.ALIGNMENT, 1)), WORD)
                if member.attrs.get(At.DATA_MEMBER_LOCATION, 0) % aligns:
                    raise Unread(f"{self.where(member)} declares a member packed below its alignment, which "
                                 "decls.py doesn't read")
                found = max(found, aligns)
        elif die.tag in (Tag.TYPEDEF, Tag.CONST_TYPE, Tag.VOLATILE_TYPE, Tag.RESTRICT_TYPE, Tag.ARRAY_TYPE):
            found = self.alignment(die.attrs.get(At.TYPE))
        else:
            found = min(die.attrs.get(At.BYTE_SIZE) or 1, WORD)
        return max(die.attrs.get(At.ALIGNMENT, 1), found)

    def parameter_type(self, parameter):
        """The type of parameter, a formal parameter's entry: its own, or
        else its abstract origin's."""
        if At.TYPE in parameter.attrs or At.ABSTRACT_ORIGIN not in parameter.attrs:
            return parameter.attrs.get(At.TYPE)
        return self.parameter_type(self.entry(parameter.attrs[At.ABSTRACT_ORIGIN]))

    def function(self, die):
        """How a call of the function die passes and returns its values, as
        a Function, each value as passed() gives it. A declaration or a
        function type without a prototype gives no parameters, and a K&R
        definition gives them as C promotes them."""
        value = self.passed(die.attrs.get(At.TYPE), returned=True)
        prototyped = At.PROTOTYPED in die.attrs
        if not prototyped and (At.DECLARATION in die.attrs or die.tag == Tag.SUBROUTINE_TYPE):
            return Function(value, None, False)
        taken = tuple(self.passed(self.parameter_type(parameter), promoted=not prototyped)
                      for parameter in self.children(die, Tag.FORMAL_PARAMETER))
        return Function(value, taken, bool(self.children(die, Tag.UNSPECIFIED_PARAMETERS)))

    def spelled(self, ref, declarator=''):
        """The type ref names, spelled as C declares it around declarator.
        Each qualifier and pointer binds where C binds it: int *const for a
        const pointer, const int * for a pointer to const data, and int
        (*)[3] for a pointer to an array. Each qualifier spells once."""
        return self.spell(self.type_entry(ref, (Tag.RESTRICT_TYPE, Tag.SUBROUTINE_TYPE)), declarator)

    def spell(self, die, declarator=''):
        """The type of the entry die, as spelled() spells it."""
        if die is None or die.tag in (Tag.TYPEDEF, Tag.BASE_TYPE, *RECORDS, Tag.ENUMERATION_TYPE):
            if die is None:
                name = 'void'
            elif die.tag in (Tag.TYPEDEF, Tag.BASE_TYPE):
                name = die.attrs.get(At.NAME, '?')
            else:
                kind = {Tag.STRUCTURE_TYPE: 'struct', Tag.UNION_TYPE: 'union', Tag.ENUMERATION_TYPE: 'enum'}[die.tag]
                name = f'{kind} {die.attrs.get(At.NAME, "{...}")}'
            return name if not declarator else f'{name}{"" if declarator.startswith("[") else " "}{declarator}'
        if die.tag == Tag.RESTRICT_TYPE:
            return self.spelled(die.attrs.get(At.TYPE), declarator)
        if die.tag in (Tag.CONST_TYPE, Tag.VOLATILE_TYPE):
            qualifier = 'const' if die.tag == Tag.CONST_TYPE else 'volatile'
            inner = self.type_entry(die.attrs.get(At.TYPE), (Tag.RESTRICT_TYPE,))
            # the type this qualifier and the ones below it qualify
            under = inner
            while under is not None and under.tag in (Tag.CONST_TYPE, Tag.VOLATILE_TYPE, Tag.RESTRICT_TYPE):
                under = self.type_entry(under.attrs.get(At.TYPE), (Tag.RESTRICT_TYPE,))
            if under is not None and under.tag == Tag.POINTER_TYPE:
                return self.spell(inner, f'{qualifier} {declarator}'.rstrip())
            # C qualifies an array through its element, and GCC qualifies both
            # the array and its element, so the element alone spells it
            if under is not None and under.tag == Tag.ARRAY_TYPE and self.qualifies_element(under, die.tag):
                return self.spell(inner, declarator)
            return f'{qualifier} {self.spell(inner, declarator)}'
        if die.tag == Tag.POINTER_TYPE:
            target = self.type_entry(die.attrs.get(At.TYPE), (Tag.RESTRICT_TYPE, Tag.SUBROUTINE_TYPE))
            pointer = f'*{declarator}'
            if target is not None and target.tag in (Tag.ARRAY_TYPE, Tag.SUBROUTINE_TYPE):
                pointer = f'({pointer})'
            return self.spell(target, pointer)
        if die.tag == Tag.ARRAY_TYPE:
            extents = ''.join(f'[{"" if extent is None else extent}]' for extent in self.extents(die, '*'))
            return self.spelled(die.attrs.get(At.TYPE), f'{declarator}{extents}')
        parameters = [self.spelled(self.parameter_type(parameter))
                      for parameter in self.children(die, Tag.FORMAL_PARAMETER)]
        if At.PROTOTYPED in die.attrs and self.children(die, Tag.UNSPECIFIED_PARAMETERS):
            parameters.append('...')
        if At.PROTOTYPED in die.attrs and not parameters:
            parameters = ['void']
        called = f'{declarator}({", ".join(parameters)})' if declarator else f'({", ".join(parameters)})'
        return self.spelled(die.attrs.get(At.TYPE), called)

    def qualifies_element(self, array, tag):
        """Whether the element of the array entry array takes the qualifier
        whose tag is tag, past its inner arrays and its other qualifiers."""
        die = self.type_entry(array.attrs.get(At.TYPE), (Tag.RESTRICT_TYPE,))
        while die is not None and die.tag in (Tag.ARRAY_TYPE, Tag.CONST_TYPE, Tag.VOLATILE_TYPE) and die.tag != tag:
            die = self.type_entry(die.attrs.get(At.TYPE), (Tag.RESTRICT_TYPE,))
        return die is not None and die.tag == tag


def units(found):
    """The compilation units of the DWARF of found, an object as elf.read()
    reads it, each as a Unit. An object without DWARF has none. DWARF
    dwarf.py doesn't read raises Unread."""
    out = []
    for unit in dwarf.units({section.name: section.data for section in found.sections}, READ):
        root = unit.dies[unit.root].attrs
        under = root.get(At.COMP_DIR, '.')
        files = [None if each is None else canonical(os.path.join(under, each)) for each in unit.files]
        source = canonical(os.path.join(under, root[At.NAME])) if At.NAME in root else found.where
        out.append(Unit(unit.dies, source, files, root.get(At.PRODUCER)))
    return out


def canonical(path):
    """path as the tree names it: relative to the tree's root, where make gcc
    runs, with no . or .. part. A path outside the tree is absolute."""
    relative = os.path.relpath(path)
    return os.path.abspath(path) if relative == '..' or relative.startswith('../') else relative


def merged(unit, offset, seen=()):
    """The entry at offset, completed from the entry its
    DW_AT_specification or DW_AT_abstract_origin names, as merged()
    completes that one. A definition takes its name, type and parameters
    from the declaration before it, and a concrete instance from its
    abstract one. The entry takes each attribute it doesn't give, but
    DW_AT_declaration and DW_AT_inline, and the other entry's children
    where it gives no parameter of its own. A reference that comes back to
    an entry it left is Unread."""
    die = unit.entry(offset)
    origin = die.attrs.get(At.SPECIFICATION, die.attrs.get(At.ABSTRACT_ORIGIN))
    if origin is None:
        return die
    if origin in seen or origin == offset:
        raise Unread(f'its DWARF holds a specification or an abstract origin that comes back to {offset:#x}')
    before = merged(unit, origin, (*seen, offset))
    taken = {name: each for name, each in before.attrs.items() if name not in (At.DECLARATION, At.INLINE)}
    own = [child for child in die.children if unit.dies[child].tag in (Tag.FORMAL_PARAMETER,
                                                                      Tag.UNSPECIFIED_PARAMETERS)]
    children = die.children if own else [*die.children, *before.children]
    return die._replace(attrs={**taken, **die.attrs}, children=children)


# a declaration or a definition of a name in an input. name is the name as
# its source spells it, and symbol the C name the link binds it by. defines
# says it defines the name. shape is its type as the check compares it, an
# Object or a Function, and spelled its type as C spells it. file and line
# are where it lies, each None where DWARF gives none, and source is its
# unit's source. origins holds the places Unit.origins() gives for its type,
# and input names its input as the final map does
Declared = namedtuple('Declared', 'name symbol defines shape spelled file line source origins input')


def check_abi(unit, abi):
    """Raise Unread where the DW_AT_producer of unit, a C unit, gives a
    compile outside abi, GCC_ABI's options. A compile lies outside them
    without one of them, with another -m or -fpack-struct option, or with
    one of FOREIGN."""
    options = [each for each in unit.producer.split() if each.startswith('-')]
    missing = [each for each in abi if each not in options]
    other = [each for each in options if each in FOREIGN
             or each.startswith(('-m', '-fpack-struct')) and each not in abi]
    if missing or other:
        given = ', '.join([*(f'without {each}' for each in missing), *(f'with {each}' for each in other)])
        raise Unread(f"holds {unit.source} compiled {given}, outside the ABI decls.py reads, {' '.join(abi)}")


def symbol_of(linkage, name, symbols):
    """The symbol a declaration of name binds among symbols, its input's, or
    None where symbols holds none, as where GCC optimized every use away.
    linkage is its DW_AT_linkage_name, or None where it has none, and then
    the symbol is name after SH's underscore. GCC gives an asm label's
    symbol as its linkage name. It gives a builtin's C name instead, as
    memcpy for __builtin_memcpy. So a linkage name binds the symbol that
    spells it, with or without the underscore. One that symbols holds both
    ways is Unread."""
    spellings = (f'_{name}',) if linkage is None else (linkage, f'_{linkage}')
    found = [each for each in spellings if each in symbols]
    if len(found) > 1:
        raise Unread(f'gives {name} the linkage name {linkage}, which its symbols hold both as it is and after '
                     "SH's underscore")
    return found[0] if found else None


def declared(found, abi=None):
    """What found, an input as elf.read() reads it, holds: the source of each
    of its units, and each external name it declares or defines, as
    Declared. The check fails, naming the input, on DWARF it doesn't read.
    It also fails on an input GCC compiled, as its .comment says, that
    defines or takes a name and holds no DWARF. Where abi is given, it fails
    on a C unit compiled outside abi, as check_abi() has it."""
    sources, names = set(), []
    symbols = {symbol.name for symbol in found.symbols}
    try:
        read = units(found)
        if (not read and any(section.name == '.comment' and b'GCC: ' in section.data for section in found.sections)
                and any(symbol.bind in (elf.GLOBAL, elf.WEAK) for symbol in found.symbols)):
            raise Unread('GCC compiled it, as its .comment says, and it defines or takes a name with no DWARF to read '
                         'its declaration from')
        for unit in read:
            if abi is not None and (unit.producer or '').startswith('GNU C'):
                check_abi(unit, abi)
            sources.add(unit.source)
            for offset, die in unit.dies.items():
                if die.tag not in (Tag.VARIABLE, Tag.SUBPROGRAM):
                    continue
                entry = merged(unit, offset)
                if At.EXTERNAL not in entry.attrs or not entry.attrs.get(At.NAME):
                    continue
                # a routine GCC calls on its own, declared without the type of
                # its calls
                file, _ = unit.place(entry)
                if file is not None and os.path.basename(file) == '<built-in>':
                    continue
                name = entry.attrs[At.NAME]
                symbol = symbol_of(entry.attrs.get(At.LINKAGE_NAME), name, symbols)
                if symbol is None:
                    continue
                if not symbol.startswith('_'):
                    raise Unread(f"{unit.where(entry)} binds {name} by the symbol {symbol}, which no C name spells")
                if die.tag == Tag.VARIABLE:
                    if At.TYPE not in entry.attrs:
                        raise Unread(f"{unit.where(entry)} declares {name} without a type, which "
                                     "decls.py doesn't read")
                    shape = Object(unit.storage(entry.attrs.get(At.TYPE)),
                                   unit.alignment(entry.attrs.get(At.TYPE), entry.attrs.get(At.ALIGNMENT, 1)))
                    spelled, origins = unit.spelled(entry.attrs.get(At.TYPE)), unit.origins(entry.attrs.get(At.TYPE))
                else:
                    shape, spelled = unit.function(entry), unit.spell(entry)
                    origins = frozenset().union(unit.origins(entry.attrs.get(At.TYPE)) if At.TYPE in entry.attrs
                                                else (), *(unit.origins(unit.parameter_type(parameter)) for parameter
                                                           in unit.children(entry, Tag.FORMAL_PARAMETER)))
                defines = not ({At.DECLARATION, At.INLINE} & die.attrs.keys())
                names.append(Declared(name, symbol[1:], defines, shape, spelled, *unit.place(entry), unit.source,
                                      origins, found.where))
    except Unread as unread:
        fail(f'{found.where}: {unread}')
    except RecursionError:
        fail(f"{found.where}: its DWARF holds references that nest deeper than Python's recursion goes, as a cycle "
             "of them does")
    return sources, names


def agree(one, other, defined, matched):
    """Whether one and other, two shapes as Declared gives them, agree.
    defined says other is the name's definition, and matched that the
    matched rule judges them, not the strict rule. Two Functions agree as
    Function.agrees() has it, and two Objects where Object.difference()
    finds none. A Function and an Object disagree."""
    if isinstance(one, Object) and isinstance(other, Object):
        return one.difference(other, defined, matched) is None
    return isinstance(one, Function) and isinstance(other, Function) and one.agrees(other)


def reach(storage):
    """The bytes an object's storage, as Unit.storage() gives it, covers at
    least: its size, or one element's where its outer extent is unknown."""
    if isinstance(storage, Array) and storage.extents[0] is None:
        storage = Array(storage.element, (1, *storage.extents[1:]))
    return size(storage)


def element(storage):
    """An object's storage, as Unit.storage() gives it, past its outer
    extent: an array of its element of each inner extent, the element
    alone where there is none, or the storage itself where it is no
    array."""
    extents = ()
    while isinstance(storage, Array):
        extents += storage.extents
        storage = storage.element
    return Array(storage, extents[1:]) if len(extents) > 1 else storage


def flat(storage):
    """An object's storage, as Unit.storage() gives it, as a run of like
    elements: the element past every extent of its arrays, and the run's
    length, or None where an extent is unknown."""
    count = 1
    while isinstance(storage, Array):
        count = None if count is None or None in storage.extents else count * math.prod(storage.extents)
        storage = storage.element
    return storage, count


def size(storage):
    """The bytes an object's storage, as Unit.storage() gives it, covers, or
    None where an extent is unknown. An Incomplete and a Void cover none."""
    element, count = flat(storage)
    if isinstance(element, (Incomplete, Void)):
        return 0
    return None if count is None else count * element.size


# a part of an object's storage: its offset in bytes, and the part itself,
# a Scalar, a DataPointer or a CodePointer
Leaf = namedtuple('Leaf', 'offset part')


def leaves(storage, start, end, offset=0):
    """Each part of storage that covers a byte of [start, end), as a Leaf.
    storage is an object's storage, laid out from offset. The leaves come
    in order of offset, but a union's come member by member."""
    element, count = flat(storage)
    step = size(element)
    if not step:
        return
    index = max(0, (start - offset) // step)
    while (count is None or index < count) and offset + index * step < end:
        at = offset + index * step
        if isinstance(element, Record):
            for place, member in element.members:
                yield from leaves(member, start, end, at + place)
        else:
            yield Leaf(at, element)
        index += 1


def clash(one, other, end):
    """A byte before end where one and other, two objects' storages,
    disagree, or None where they agree. The byte comes as (one's leaf over
    it, other's leaf over it, the byte), with None for padding. A leaf
    disagrees over a byte where no leaf of the other storage over that byte
    agrees with it, as alike() has it. So a member of a union needs only one
    leaf of the other storage to agree with."""
    found = unpaired(one, other, end)
    if found is not None:
        return found
    found = unpaired(other, one, end)
    return None if found is None else (found[1], found[0], found[2])


def unpaired(one, other, end):
    """The first leaf of one, an object's storage, over a byte before end
    where no leaf of other agrees with it, as alike() has it. It comes as
    the leaf, other's first leaf over the byte or None for padding, and the
    byte. It is None where no leaf of one lies over such a byte."""
    for leaf in leaves(one, 0, end):
        stop = min(leaf.offset + leaf.part.size, end)
        near = list(leaves(other, leaf.offset, stop))
        for byte in range(leaf.offset, stop):
            over = [each for each in near if each.offset <= byte < each.offset + each.part.size]
            if not any(alike(leaf.part, each.part) for each in over):
                return leaf, next(iter(over), None), byte
    return None


def alike(one, other):
    """Whether two parts that cover a byte in common agree. They must be
    const and volatile alike. Two DataPointers must both or neither point
    to volatile data, and two CodePointers' functions must agree. A
    CodePointer agrees with a DataPointer, through which no call goes."""
    if one.qualifiers != other.qualifiers:
        return False
    if isinstance(one, DataPointer) and isinstance(other, DataPointer):
        return one.volatile == other.volatile
    if isinstance(one, CodePointer) and isinstance(other, CodePointer):
        return one.function.agrees(other.function)
    return True


# a problem: the image it lies in, the (name, file) of each declaration a
# row can admit it by, and its text
Problem = namedtuple('Problem', 'image declarations text')
# a declaration in an image's input: the image, and the declaration as
# Declared
Held = namedtuple('Held', 'image declared')
# an image's final pass, as ld.py leaves it. values holds the address of
# each external C name its ELF defines, those its link script assigns among
# them. inputs holds, by the map's name for each input it places, the
# input's path, an archive's for a member. definers holds, by C name, the
# input the pass binds the name to: the input whose own symbol of the name
# lies at the name's address
Final = namedtuple('Final', 'values inputs definers')
# what an input holds. defined holds each C name gcclink.exports() finds
# it defines, as its section's name and its value there. declarations holds
# its declarations, as declared() gives them, or is None for libgcc.a,
# whose DWARF the check doesn't read
Input = namedtuple('Input', 'defined declarations')


def location(declaration):
    """Where declaration lies, as file_line() spells it, followed by its
    object's source where that is another file, as for a header's
    declaration."""
    where = file_line(declaration.file, declaration.line)
    return where if declaration.file == declaration.source else f'{where} (in {declaration.source})'


def verb(declaration):
    return 'defines' if declaration.defines else 'declares'


def disagreeing(one, other, matched, defined):
    """The Problem of one against other, two declarations as Held that bind
    one name and disagree. defined says other is the name's definition, and
    matched that the matched rule judged them. A row for one's declaration
    admits the problem, and so does a row for other's unless other is the
    definition. The text ends with where two Objects differ, or two
    Functions that C spells alike."""
    mine, theirs = one.declared, other.declared
    whose = f"{other.image}'s " if other.image != one.image else ''
    what = 'it' if theirs.name == mine.name else theirs.name
    declarations = [(mine.name, mine.file)] + ([] if defined else [(theirs.name, theirs.file)])
    differs = ''
    if isinstance(mine.shape, Object) and isinstance(theirs.shape, Object):
        differs = f': {mine.shape.difference(theirs.shape, defined, matched)}'
    elif isinstance(mine.shape, Function) and isinstance(theirs.shape, Function) and mine.spelled == theirs.spelled:
        differs = f': {mine.shape.difference(theirs.shape)}'
    return Problem(one.image, declarations, (
        f'{mine.name}: {location(mine)} {verb(mine)} it {mine.spelled}, and {whose}{location(theirs)} '
        f'{verb(theirs)} {what} {theirs.spelled}{differs}'))


def unread(one, definer):
    """The Problem of one, a declaration as Held, whose name the link binds
    to the definition in definer. definer is a member of libgcc.a, whose
    DWARF the check doesn't read. A row for one's declaration admits it."""
    mine = one.declared
    return Problem(one.image, [(mine.name, mine.file)], (
        f'{mine.name}: {location(mine)} {verb(mine)} it {mine.spelled}, and the link binds it to {definer}, whose '
        "DWARF decls.py doesn't read"))


def misaligned(one, value):
    """The Problem of one, a declaration of an object as Held, whose name
    the link places at value, off its alignment. A row for one's
    declaration admits it."""
    mine = one.declared
    return Problem(one.image, [(mine.name, mine.file)], (
        f'{mine.name}: {location(mine)} {verb(mine)} it {mine.spelled}, which aligns to {mine.shape.alignment}, and '
        f'the link places it at {value:#010x}'))


class Check:
    """The check of a spec's images, which ld.py linked under out. abi is
    GCC_ABI's options, as check_abi() takes them, or None to leave each
    unit's options unchecked. runtime is the path of libgcc.a, whose DWARF
    the check doesn't read. The check reads each input and each file once."""

    def __init__(self, images, out, abi=None, runtime=None):
        self.images, self.out, self.abi, self.runtime = images, Path(out), abi, runtime
        # what final() and input() have read: each image's final pass, by its
        # name; each input, by the map's name for it; and the source of each
        # unit
        self.finals, self.inputs, self.sources = {}, {}, set()
        # each file matched_at() has read: its lines on GCC's side of a
        # guard, counting from 1, and its count of lines, or None where it
        # can't be read
        self.gcc_sides = {}
        # agree() of each pair of shapes agrees() has compared, by the pair,
        # its direction and its rule
        self.agreements = {}

    def input(self, where, path):
        """What the input the final map names where holds, as Input: the
        object at path, or the member of the archive at path that where
        names as path(MEMBER), as elf.opened() reads it. The sources of its
        units join sources, but libgcc.a's."""
        if where not in self.inputs:
            found = dict(elf.opened(path))[None if where == path else where[len(path) + 1:-1]]
            sections = {section.index: section.name for section in found.sections}
            defined = {name: (sections.get(symbol.section), symbol.value)
                       for name, symbol in gcclink.exports(found).items()}
            declarations = None
            if path != self.runtime:
                sources, declarations = declared(found, self.abi)
                self.sources |= sources
            self.inputs[where] = Input(defined, declarations)
        return self.inputs[where]

    def matched(self, declaration):
        """Whether declaration, as Declared, is matched: its object's source
        lies outside GCC_ONLY, and matched_at() takes its own place and each
        of its origins."""
        return not declaration.source.startswith(GCC_ONLY) and all(
            self.matched_at(file, line) for file, line in ((declaration.file, declaration.line), *declaration.origins))

    def matched_at(self, file, line):
        """Whether line of file, as Unit.place() gives them, can hold a
        matched declaration or type. It can where file lies in the tree
        outside GCC_ONLY, and the check can read it. line must be one of the
        file's, on no guard's GCC side, as csource.py's gcc_side() finds it. A
        place without a file or a line can't."""
        if file is None or line is None or os.path.isabs(file) or file.startswith(GCC_ONLY):
            return False
        if file not in self.gcc_sides:
            lines = blanked(content(file).decode('latin-1')).split('\n') if Path(file).is_file() else None
            self.gcc_sides[file] = None if lines is None else ({at + 1 for at in gcc_side(lines)}, len(lines))
        if self.gcc_sides[file] is None:
            return False
        guarded, count = self.gcc_sides[file]
        return line <= count and line not in guarded

    def final(self, name):
        """The final pass of the image named name, as Final, from its ELF and
        its map under out. An input's symbol lies at the address the map
        gives its section, plus its own value. The check fails on a map
        that places a file that is neither an input the spec gives the image
        nor a member of one. It fails on the pass as gcclink.linked() fails,
        and on the inputs as elf.opened() fails."""
        if name not in self.finals:
            passed, mapped = gcclink.linked(self.out, name, 'final'), gcclink.pass_file(self.out, name, 'final', '.map')
            values = {symbol.name[1:]: symbol.value for symbol in passed.elf.symbols
                      if symbol.bind in (elf.GLOBAL, elf.WEAK) and symbol.section != elf.UNDEFINED
                      and symbol.name.startswith('_')}
            given = set(self.images[name].inputs)
            addresses, inputs = {}, {}
            for piece in passed.placements:
                where = piece.file
                if where not in inputs:
                    archive = where[:where.rfind('(')] if where.endswith(')') else None
                    if where not in given and archive not in given:
                        fail(f"{mapped}: places {where}, which is no input of {name}'s, nor a member of one")
                    inputs[where] = where if where in given else archive
                addresses.setdefault((where, piece.section), piece.address)
            definers = {}
            for where, path in inputs.items():
                for symbol, (section, value) in self.input(where, path).defined.items():
                    at = addresses.get((where, section))
                    if at is not None and at + value == values.get(symbol):
                        definers.setdefault(symbol, where)
            self.finals[name] = Final(values, inputs, definers)
        return self.finals[name]

    @cached_property
    def names(self):
        """The declarations in the inputs of each image's final pass, but
        libgcc.a's, each as Held. They come by the image and the C name each
        one's symbol stands for. An import stands for its provider's symbol,
        and an alias for its image's own symbol. Any other name stands for
        itself in its image."""
        found = {}
        for name, image in self.images.items():
            stands_for = {**{alias: (name, symbol) for alias, symbol in image.aliases.items()},
                          **{alias: (wanted.provider, wanted.symbol) for alias, wanted in image.imports.items()}}
            for where, path in self.final(name).inputs.items():
                held = self.input(where, path)
                for each in held.declarations or ():
                    found.setdefault(stands_for.get(each.symbol, (name, each.symbol)), []).append(Held(name, each))
        return found

    def problems(self):
        """Each Problem of the spec's images. The check compares each
        declaration of a name with the C definition the link binds the name
        to, where it reads one. It then compares each two declarations that
        agree with the definition, the later against the earlier. Where it
        reads no definition, it compares each two of them all. Each
        declaration of a name the link binds to libgcc.a is a problem. So is
        each declaration of an object whose name the link places off its
        alignment."""
        found = []
        for (image, symbol), declarations in self.names.items():
            final = self.final(image)
            definer = final.definers.get(symbol)
            if definer is not None and final.inputs[definer] == self.runtime:
                found += [unread(each, definer) for each in declarations]
                continue
            definition = next((each for each in declarations if each.image == image and each.declared.defines
                               and each.declared.input == definer), None)
            rest = [each for each in declarations if each is not definition]
            if definition is not None:
                judged = [(one, self.disagreement(one, definition, True)) for one in rest]
                found += [problem for _, problem in judged if problem]
                rest = [one for one, problem in judged if not problem]
            # two declarations of one shape agree, and the matched rule passes
            # each two shapes the strict rule passes. So two declarations can
            # disagree only where the strict rule fails their shapes
            shapes = {}
            for place, each in enumerate(rest):
                shapes.setdefault(each.declared.shape, []).append(place)
            pairs = []
            for (one, ones), (other, others) in combinations(shapes.items(), 2):
                if not self.agrees(one, other, False, False):
                    pairs += [(min(a, b), max(a, b)) for a in ones for b in others]
            for earlier, later in sorted(pairs):
                problem = self.disagreement(rest[later], rest[earlier], False)
                if problem:
                    found.append(problem)
            for each in declarations:
                value, shape = self.final(each.image).values.get(each.declared.symbol), each.declared.shape
                if isinstance(shape, Object) and value is not None and value % shape.alignment:
                    found.append(misaligned(each, value))
        return found

    def disagreement(self, one, other, defined):
        """The Problem of one against other, two declarations as Held, or
        None where they agree. defined says other is the name's definition.
        The matched rule judges them where both are matched, and the strict
        rule otherwise. A pair the strict rule passes, the matched rule
        passes too, so matched() reads files only for the pairs the strict
        rule fails."""
        mine, theirs = one.declared, other.declared
        if self.agrees(mine.shape, theirs.shape, defined, False):
            return None
        matched = self.matched(mine) and self.matched(theirs)
        if matched and self.agrees(mine.shape, theirs.shape, defined, True):
            return None
        return disagreeing(one, other, matched, defined)

    def agrees(self, one, other, defined, matched):
        """agree() of two shapes, found once for each pair, direction and
        rule: the objects that include one header give the same pair many
        times."""
        key = (one, other, defined, matched)
        if key not in self.agreements:
            self.agreements[key] = agree(one, other, defined, matched)
        return self.agreements[key]


def admissions():
    """The rows of config/declarations.txt, as {(name, file): where the row
    lies}. The check fails on a line that isn't a row, and on a row for a
    file that doesn't exist. It also fails on a row that gives the name and
    file of a row above it."""
    found = {}
    for where, fields in rows(DECLARATIONS):
        if len(fields) < 3 or not gcclink.C_NAME.fullmatch(fields[0]):
            fail(f'{where}: not a row: NAME FILE REASON')
        file = canonical(fields[1])
        if not Path(file).is_file():
            fail(f'{where}: {fields[1]}: no such file')
        key = (fields[0], file)
        if key in found:
            fail(f'{where}: {" ".join(key)} has a row already, at {found[key]}')
        found[key] = where
    return found


def failures(images, out, abi):
    """The lines the check fails with on images, a spec's, as tools/ld.py
    linked them under out: one for each text of a problem no row of
    config/declarations.txt admits, naming the first image that gives it and
    how many more do, then one for each row that admits nothing. abi is
    GCC_ABI's options. It fails as gcclink.libgcc(), admissions() and Check
    do."""
    check = Check(images, out, abi, str(gcclink.libgcc()))
    admitted = admissions()
    used, grouped = set(), {}
    for problem in check.problems():
        admitting = [each for each in problem.declarations if each in admitted]
        used.update(admitting)
        if not admitting:
            grouped.setdefault(problem.text, {})[problem.image] = True
    problems = [f'{next(iter(giving))}{f" and {len(giving) - 1} more" if len(giving) > 1 else ""}: {text}'
                for text, giving in grouped.items()]
    held = {(each.declared.name, each.declared.file) for declarations in check.names.values()
            for each in declarations}
    return problems + [f'{where}: admits no disagreement, so the row is stale' for row, where in admitted.items()
                       if row not in used and (row in held or row[1] in check.sources)]
