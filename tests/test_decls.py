import re
import tempfile
import unittest
from unittest import mock
from pathlib import Path

import decls
import dwarf
import elf
import gcclink
import ld
import test_ld
import verify
from support import cwd

# the fixture's sources, by path, in groups by what they show
SOURCES = {
    # the definitions most sources declare. limit follows a declaration of it, and sum takes a pointer to a
    # variable-length array
    'def.c': '''\
struct pair {
    int count;
    short step;
};

struct hidden {
    int x;
};
extern const int limit;
const int limit = 3;
struct pair pair = {1, 2};
unsigned int word = 4;
struct hidden hidden_one;
struct hidden *chain = &hidden_one;
struct hidden *handle = &hidden_one;
const unsigned long bits = 0x3f800000;
short *cursor;
signed char sign = -1;
short *steps;

int take(int x)
{
    return x + limit;
}

void old(base, scale)
short base;
float scale;
{
    pair.count = base * (int)scale;
}

int narrow(short x)
{
    return x;
}

int vary(int x, ...)
{
    return x;
}

volatile int *flag;
int (*getter)(void);

int fill(int *dest)
{
    return *dest;
}

int flagged(_Bool on)
{
    return on;
}

struct cell {
    float f;
};

float weight(struct cell c)
{
    return c.f;
}

int table[4];
int rows[2][4];

struct grid {
    int cells[2][3];
} grid;

struct whole {
    int a;
} whole;

struct sized {
    _Alignas(4) char a;
} sized;

struct moved {
    _Alignas(4) char a;
    char b;
} moved;

struct counted {
    int a;
    char b;
} counted;

union single {
    int i;
} single;

volatile int ticks;
int (*const hook)(void) = 0;

signed char tone(void)
{
    return -1;
}

unsigned int count_of(void)
{
    return 1;
}

int poll(volatile int *p)
{
    return *p;
}

static int helper(int x)
{
    return x + 1;
}

int use_helper(int x)
{
    return helper(x);
}

const struct pair fixed = {5, 6};

_Bool ready(void)
{
    return 1;
}

int *where(void)
{
    return &pair.count;
}

typedef int handler(void);
handler *on_event;

enum shade {
    DARK,
    LIGHT
} shade = LIGHT;

struct twin {
    float a;
    float b;
};

float twin_sum(struct twin t)
{
    return t.a + t.b;
}

struct quad {
    char c[4];
};

struct quad quad_of(int x)
{
    struct quad q = {{x, x, x, x}};
    return q;
}

int quad_sum(struct quad q)
{
    return q.c[0] + q.c[3];
}

long long wide(long long x)
{
    return x;
}

struct wrap {
    struct cell c[1];
};

float unwrap(struct wrap w)
{
    return w.c[0].f;
}

struct sized sized_of(void)
{
    return sized;
}

struct triple {
    int a[3];
};

struct triple triple_of(int x)
{
    struct triple t = {{x, x, x}};
    return t;
}

short halves[2] = {1, 2};

union reg {
    volatile int v;
    int plain;
} reg;

int sum(int n, int (*rows)[n])
{
    return rows[1][n - 1];
}
float plain[16];
int counter;
int **deep;
void *callback;
''',
    # declarations of def.c's names that agree with their definitions. Where one's type isn't def.c's, the comment
    # beside it names the two
    'agrees.c': '''\
struct pair {
    int count;
    short step;
};

struct hidden;
struct whole;

struct single {
    int i;
};

struct twin {
    int a;
    int b;
};

struct word {
    int w;
};

struct six {
    short s[6];
};

union reg {
    volatile int v;
    int plain;
};

struct holder {
    union reg r;
};

extern const int limit;
extern struct pair pair;
extern int take(int x);
extern void old(); /* no prototype, for a K&R definition, which takes its parameters as C promotes them */
extern long word; /* an unsigned int as a long */
extern struct hidden *chain; /* a pointer to a structure declared alone */
extern void *handle; /* a pointer to data of another type */
extern const float bits; /* an unsigned long as a float */
extern unsigned char sign; /* a signed char as an unsigned one */
extern int *steps; /* a pointer to data of another type */
extern int (*getter)(); /* a pointer to code without its prototype */
extern int fill(int *restrict dest); /* a restrict pointer */
extern int table[]; /* arrays without their outer extent */
extern int rows[][4];
extern struct whole whole; /* a structure declared alone */
extern struct single single; /* a union's one member as a structure's */
extern long count_of(void); /* an unsigned int as a long */
extern int poll(int *p); /* a pointer to data that isn't volatile */
extern unsigned char ready(void); /* a _Bool as an unsigned char */
extern int (*on_event)(); /* a pointer to a typedef's function as one without a prototype */
extern int shade; /* an enumeration as an int */
extern void *const hook; /* a const pointer to code as a const one to data */
extern struct holder reg; /* a union of a volatile int and an int as a structure's one member */
/* values GCC passes alike */
extern float weight(float c); /* a structure of a float as a float */
extern float twin_sum(struct twin t); /* two floats as two ints */
extern int quad_sum(struct word w); /* four chars as an int */
extern long double wide(long double x); /* 8-byte integers as 8-byte floats */
extern float unwrap(float w); /* a structure of an array of one structure of a float as a float */
extern struct word sized_of(void); /* a char aligned on 4 as an int, returned in R0 */
extern struct six triple_of(int x); /* three ints as six shorts, returned through memory */

/* statics, which the check doesn't read: helper, of another type than
 * def.c's helper, and peek, whose parameter points to a restrict pointer, as
 * an SH4ZAM inline helper's can */
static float helper(float x)
{
    return x * 2.0F;
}

static int peek(int *restrict *at)
{
    return **at;
}

int use(void)
{
    old(1, 2.0F);
    return take(limit) + pair.step + (int)word + (chain != handle) + (int)bits + sign + *steps + getter() +
           fill(&pair.count) + table[1] + rows[1][3] + (&whole != 0) + single.i + (int)count_of() + poll(&pair.count) +
           (int)helper(1.0F) + ready() + on_event() + shade + peek(&steps) + reg.r.plain;
}

float use_by_value(struct twin t, struct word w)
{
    return weight(1.0F) + twin_sum(t) + (float)quad_sum(w) + (float)wide(1.0L) + (float)(hook != 0) + unwrap(1.0F) +
           (float)sized_of().w + (float)triple_of(1).s[0];
}
''',
    # declarations of one of def.c's names in another type, whose bytes or calls GCC's code reads alike
    'stride.c': 'extern int rows[2][3];\n\nint use_stride(void)\n{\n    return rows[1][0];\n}\n',
    'open.c': 'extern int rows[][3];\n\nint use_open(void)\n{\n    return rows[1][0];\n}\n',
    'flat.c': 'extern int rows[8];\n\nint use_flat(void)\n{\n    return rows[4];\n}\n',
    'grid.c': ('struct grid {\n    int cells[3][2];\n};\n\nextern struct grid grid;\n\n'
               'int use_grid(void)\n{\n    return grid.cells[1][1];\n}\n'),
    'whole.c': ('struct whole;\n\nextern const struct whole whole;\n\n'
                'const void *use_whole(void)\n{\n    return &whole;\n}\n'),
    'sized.c': ('struct sized {\n    char a;\n};\n\nextern struct sized sized;\n\n'
                'int use_sized(void)\n{\n    return sized.a;\n}\n'),
    'pieces.c': ('struct pieces {\n    short low;\n    short high;\n};\n\nextern struct pieces word;\n\n'
                 'int use_pieces(void)\n{\n    return word.high;\n}\n'),
    'pointer.c': 'extern int cursor;\n\nint use_pointer(void)\n{\n    return cursor;\n}\n',
    'tail.c': ('struct tail {\n    float f;\n    int none[0];\n};\n\nextern float weight(struct tail c);\n\n'
               'float use_tail(struct tail c)\n{\n    return weight(c);\n}\n'),
    'opaque.c': ('struct opaque;\n\nextern struct opaque word;\n\n'
                 'const void *use_opaque(void)\n{\n    return &word;\n}\n'),
    'rack.c': ('struct grid {\n    int *cells[2][3];\n};\n\nextern struct grid grid;\n\n'
               'int use_rack(void)\n{\n    return *grid.cells[1][1];\n}\n'),
    'firm.c': 'struct pair;\n\nextern struct pair fixed;\n\nconst void *use_firm(void)\n{\n    return &fixed;\n}\n',
    'where.c': 'extern int where(void);\n\nint use_where(void)\n{\n    return where();\n}\n',
    # two declarations of def.c's callback that each agree with it, but not with each other
    'cb_int.c': 'extern int (*callback)(int);\n\nint use_cb_int(void)\n{\n    return callback(1);\n}\n',
    'cb_float.c': 'extern int (*callback)(float);\n\nint use_cb_float(void)\n{\n    return callback(1.0F);\n}\n',
    # declarations of one of def.c's names in a type the check fails
    'return.c': 'extern float take(int x);\n\nfloat use_return(void)\n{\n    return take(1);\n}\n',
    'parameter.c': 'extern int take(float x);\n\nint use_parameter(void)\n{\n    return take(1.0F);\n}\n',
    'variadic.c': 'extern int take(int x, ...);\n\nint use_variadic(void)\n{\n    return take(1);\n}\n',
    'tone.c': 'extern unsigned char tone(void);\n\nint use_tone(void)\n{\n    return tone();\n}\n',
    'volatile.c': 'extern int ticks;\n\nint use_volatile(void)\n{\n    return ticks + ticks;\n}\n',
    'hook.c': 'extern int (*hook)();\n\nint use_hook(void)\n{\n    return hook();\n}\n',
    'count.c': 'extern int table[8];\n\nint use_count(void)\n{\n    return table[7];\n}\n',
    'moved.c': ('struct moved {\n    char a;\n    _Alignas(2) char b;\n};\n\nextern struct moved moved;\n\n'
                'int use_moved(void)\n{\n    return moved.b;\n}\n'),
    'counted.c': ('struct counted {\n    int a;\n    char b;\n    char c;\n};\n\nextern struct counted counted;\n\n'
                  'int use_counted(void)\n{\n    return counted.c;\n}\n'),
    'aligned.c': 'extern int halves;\n\nint use_aligned(void)\n{\n    return halves;\n}\n',
    'deep.c': 'extern int table[][5];\n\nint use_deep(void)\n{\n    return table[0][4];\n}\n',
    'qualifier.c': 'extern int limit;\n\nvoid use_qualifier(void)\n{\n    limit = 4;\n}\n',
    'member.c': ('struct pair {\n    int count;\n    int step;\n};\n\nextern struct pair pair;\n\n'
                 'int use_member(void)\n{\n    return pair.step;\n}\n'),
    'flag.c': 'extern int *flag;\n\nint use_flag(void)\n{\n    return *flag;\n}\n',
    'getter.c': 'extern float (*getter)(void);\n\nfloat use_getter(void)\n{\n    return getter();\n}\n',
    'promote.c': ('extern int narrow();\nextern int vary();\nextern int flagged();\n\n'
                  'int use_promote(void)\n{\n    return narrow(1) + vary(2) + flagged(1);\n}\n'),
    'cell.c': ('struct cell {\n    int i;\n};\n\nextern float weight(struct cell c);\n\n'
               'float use_cell(struct cell c)\n{\n    return weight(c);\n}\n'),
    'shell.c': ('union cell {\n    float f;\n};\n\nextern float weight(union cell c);\n\n'
                'float use_shell(union cell c)\n{\n    return weight(c);\n}\n'),
    'vla.c': ('extern float sum(int n, int (*rows)[*]);\n\n'
              'float use_vla(int (*rows)[2])\n{\n    return sum(2, rows);\n}\n'),
    'quad.c': ('struct word {\n    int w;\n};\n\nextern struct word quad_of(int x);\n\n'
               'int use_quad(void)\n{\n    return quad_of(1).w;\n}\n'),
    'fixed.c': ('struct pair {\n    int count;\n    short step;\n};\n\nextern struct pair fixed;\n\n'
                'int use_fixed(void)\n{\n    return fixed.count;\n}\n'),
    'matrix.c': ('typedef float matrix[16] __attribute__((aligned(8)));\nextern matrix plain;\n\n'
                 'float use_matrix(void)\n{\n    return plain[0];\n}\n'),
    # declarations the strict rule judges. Each lies under gcc/ or on GCC's side of a guard, takes a type GCC's side
    # defines or lays out, or lies in gone.c, which the fixture removes. The matched rule judges kind.c and pointee.c,
    # twins of gcc/kind.c and gcc/pointee.c
    'gcc/open.c': 'extern int rows[][4];\n\nint use_gcc_open(void)\n{\n    return rows[1][0];\n}\n',
    'gcc/flat.c': 'extern int rows[8];\n\nint use_gcc_flat(void)\n{\n    return rows[4];\n}\n',
    'typeguard.c': ('#ifdef GCC\nstruct twin {\n    short low;\n    short high;\n};\n#else\nstruct twin {\n'
                    '    int both;\n};\n#endif\nextern struct twin word;\n\nint use_typeguard(void)\n{\n'
                    '    return word.high;\n}\n'),
    'memberguard.c': ('struct pieces {\n#ifdef GCC\n    short low;\n    short high;\n#else\n    int both;\n#endif\n};\n'
                      'extern struct pieces word;\n\nint use_memberguard(void)\n{\n    return word.high;\n}\n'),
    'kind.c': 'extern float counter;\n\nfloat use_kind(void)\n{\n    return counter;\n}\n',
    'gcc/kind.c': 'extern float counter;\n\nfloat use_gcc_kind(void)\n{\n    return counter;\n}\n',
    'pointee.c': 'extern volatile int **deep;\n\nint use_pointee(void)\n{\n    return **deep;\n}\n',
    'gcc/pointee.c': 'extern volatile int **deep;\n\nint use_gcc_pointee(void)\n{\n    return **deep;\n}\n',
    'guarded.c': ('struct pieces {\n    short low;\n    short high;\n};\n\n#ifdef GCC\nextern struct pieces word;\n'
                  '#endif\n\nint use_guarded(void)\n{\n    return word.high;\n}\n'),
    'gone.c': 'extern int rows[8];\n\nint use_gone(void)\n{\n    return rows[4];\n}\n',
    # types the check doesn't read
    'bits.c': ('struct flags {\n    int x : 3;\n};\n\nextern struct flags field;\n\n'
               'int use_bits(void)\n{\n    return field.x;\n}\n'),
    'atomic.c': 'extern _Atomic int counter;\n\nint use_atomic(void)\n{\n    return counter;\n}\n',
    'complex.c': ('extern _Complex float spin(void);\n\n'
                  'float use_complex(void)\n{\n    return __real__ spin();\n}\n'),
    'restrict.c': 'extern int *restrict spot;\n\nint use_restrict(void)\n{\n    return *spot;\n}\n',
    'packed.c': ('struct tight {\n    char c;\n    int i;\n} __attribute__((packed));\nextern struct tight tight;\n\n'
                 'int use_packed(void)\n{\n    return tight.i;\n}\n'),
    # names no C object the link takes defines. provide.s defines given. stray.c defines it too, but the link leaves
    # libstray.a's stray.o out. held.s defines the names the sources after it take, and those of bits.c, complex.c,
    # restrict.c and packed.c
    'provide.s': '        .text\n        .global _given\n_given:\n        rts\n        nop\n',
    'given.c': 'extern int given(int x);\n\nint use_given(void)\n{\n    return given(1);\n}\n',
    'other.c': 'extern float given(int x);\n\nfloat use_other(void)\n{\n    return given(1);\n}\n',
    'stray.c': 'float given(int x)\n{\n    return x;\n}\n',
    'held.s': ('        .text\n        .global _shared\n        .global _spin\n_shared:\n_spin:\n'
               '        rts\n        nop\n        .data\n        .balign 8\n        .space 4\n'
               '        .global _skew\n_skew:  .space 64\n        .global _stash\n_stash: .space 32\n'
               '        .global _ring\n_ring:  .space 24\n        .global _field\n_field: .space 8\n'
               '        .global _spot\n_spot:  .space 8\n        .global _tight\n_tight: .space 8\n'),
    'skew.c': ('typedef float matrix[16] __attribute__((aligned(8)));\nextern matrix skew;\n\n'
               'float use_skew(void)\n{\n    return skew[0];\n}\n'),
    'unprototyped.c': 'extern void shared();\n\nvoid use_unprototyped(void)\n{\n    shared();\n}\n',
    'integer.c': 'extern void shared(int x);\n\nvoid use_integer(void)\n{\n    shared(1);\n}\n',
    'real.c': 'extern void shared(float x);\n\nvoid use_real(void)\n{\n    shared(1.0F);\n}\n',
    'few.c': 'extern int stash[4];\n\nint use_few(void)\n{\n    return stash[3];\n}\n',
    'many.c': 'extern int stash[8];\n\nint use_many(void)\n{\n    return stash[7];\n}\n',
    'ring2.c': ('struct two {\n    short a;\n    const short b;\n};\n\nextern struct two ring[];\n\n'
                'int use_ring2(void)\n{\n    return ring[1].b;\n}\n'),
    'ring3.c': ('struct three {\n    short a;\n    const short b;\n    short c;\n};\n\nextern struct three ring[];\n\n'
                'int use_ring3(void)\n{\n    return ring[1].b;\n}\n'),
    # names the link binds other than to one object's strong definition. second.c declares second, which ALIAS
    # makes a second name for def.c's take
    'second.c': 'extern float second(int x);\n\nfloat use_second(void)\n{\n    return second(1);\n}\n',
    # a weak definition, which the link passes over for a strong one
    'weak.c': 'int value[8] __attribute__((weak));\n\nint use_weak(void)\n{\n    return value[7];\n}\n',
    'strong.c': 'int value;\n',
    # archives' members, which the link adds where an input refers to a name they define
    'root.c': 'extern int first(void);\n\nint use_root(void)\n{\n    return first();\n}\n',
    'chain1.c': 'extern int second(float x);\n\nint first(void)\n{\n    return second(1.0F);\n}\n',
    'chain2.c': 'int second(int x)\n{\n    return x;\n}\n',
    'chain3.c': 'float first(void)\n{\n    return 1.0F;\n}\n',
    'prov.c': 'int provided(int x)\n{\n    return x;\n}\n',
    'cons.c': 'extern float provided(int x);\n\nfloat use_cons(void)\n{\n    return provided(1);\n}\n',
    # an asm label, and a declaration whose one use GCC drops
    'word.c': 'int shared_word;\n',
    'labeled.c': ('extern volatile int private_name __asm__("_shared_word");\n\nint use_labeled(void)\n{\n'
                  '    return private_name;\n}\n'),
    'gcc/dead.c': 'extern float counter;\n\nint use_dead(void)\n{\n    float x = counter;\n    return 1;\n}\n',
    # a name libgcc.a defines, and a routine GCC declares at <built-in>
    'clz.c': 'extern int __clzsi2(unsigned x);\n\nint use_clz(unsigned x)\n{\n    return __clzsi2(x);\n}\n',
    'copy.c': 'void *memcpy(void *to, const void *from, unsigned long size)\n{\n    return to;\n}\n',
    'copier.c': ('void use_copier(void *to, const void *from, unsigned long size)\n{\n'
                 '    __builtin_memcpy(to, from, size);\n}\n'),
    # a header's declaration in two objects
    'sub/header.h': 'extern int take(float x);\n',
    'header1.c': '#include "sub/header.h"\n\nint use_header1(void)\n{\n    return take(1.0F);\n}\n',
    'header2.c': '#include "sub/header.h"\n\nint use_header2(void)\n{\n    return take(2.0F);\n}\n',
    # DWARF in other shapes: a line table GNU as writes, units the link refuses, a form and an extent no other source
    # gives, and an object with none
    'sub/early.h': 'extern float take(int x);\n',
    'early.c': '#include "sub/early.h"\n\nfloat use_early(void)\n{\n    return take(1);\n}\n',
    'dwarf4.c': 'extern float take(int x);\n\nfloat use_dwarf4(void)\n{\n    return take(1);\n}\n',
    'split.c': 'extern float take(int x);\n\nfloat use_split(void)\n{\n    return take(1);\n}\n',
    'banner.c': 'static const char banner[300] = "banner";\n\nint use_banner(void)\n{\n    return banner[1];\n}\n',
    'zero.c': 'extern int halves[0];\n\nint *use_zero(void)\n{\n    return halves;\n}\n',
    'empty.c': '',
    # a unit compiled outside GCC_ABI
    'loose.c': 'extern int counter;\n\nint use_loose(void)\n{\n    return counter;\n}\n',
}
# the flags a source takes past make gcc's
FLAGS = {
    # at -O0, GNU as writes the line table, of version 3
    'early.c': ['-O0'],
    # a unit of DWARF 4, which dwarf.py doesn't read
    'dwarf4.c': ['-gdwarf-4'],
    # a skeleton unit, the rest of the DWARF split out
    'split.c': ['-gsplit-dwarf'],
    # an option outside GCC_ABI that changes how a call passes a value
    'loose.c': ['-mrenesas'],
}
# an image of def.o, held.o and the object given
SPEC = 'image  main  0x8c010000  0x00010000\ninput  main  def.o\ninput  main  held.o\ninput  main  {}\n'
# main of def.o, and ovl of return.o, which imports main's take and declares it otherwise
IMPORT = ('image  main  0x8c010000  0x00010000\ninput  main  def.o\nimage  ovl  0x0c900000  0x00010000\n'
          'input  ovl  return.o\nimport  ovl  take  main  take  0x8c\n')
# SPEC's image of second.o, with a second name for take, which second.c declares otherwise
ALIAS = SPEC.format('second.o') + 'alias  main  second  take\n'


def at(source, text):
    """source:line for the first line of SOURCES[source] that holds text, as a problem names a declaration there."""
    return f'{source}:{next(number for number, line in enumerate(SOURCES[source].splitlines(), 1) if text in line)}'


# where def.c defines take and weight, as many problems name them
TAKE = at('def.c', 'int take(')
WEIGHT = at('def.c', 'float weight(')


def unit(*entries):
    """A Unit of entries, each (tag, attributes, children), at offsets 1, 2 and on, as dwarf.py reads them."""
    dies = {offset: dwarf.Die(tag, attrs, {}, list(children), None) for offset, (tag, attrs, children)
            in enumerate(entries, 1)}
    return decls.Unit(dies, 'unit.c', ['unit.c'])


class Types(unittest.TestCase):
    def test_fails_on_an_array_without_a_subrange_or_of_a_negative_extent(self):
        base = (decls.Tag.BASE_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.ENCODING: decls.Ate.SIGNED}, ())
        with self.assertRaisesRegex(dwarf.Unread, 'no subrange'):
            unit(base, (decls.Tag.ARRAY_TYPE, {decls.At.TYPE: 1}, ())).storage(2)
        bound = (decls.Tag.SUBRANGE_TYPE, {decls.At.UPPER_BOUND: -2}, ())
        with self.assertRaisesRegex(dwarf.Unread, 'the extent -1'):
            unit(base, (decls.Tag.ARRAY_TYPE, {decls.At.TYPE: 1}, (3,)), bound).storage(2)

    def test_fails_on_a_type_without_a_size_or_of_an_encoding_it_doesnt_read(self):
        with self.assertRaisesRegex(dwarf.Unread, 'no size'):
            unit((decls.Tag.BASE_TYPE, {decls.At.ENCODING: decls.Ate.SIGNED}, ())).storage(1)
        with self.assertRaisesRegex(dwarf.Unread, 'the encoding 3'):
            unit((decls.Tag.BASE_TYPE, {decls.At.BYTE_SIZE: 8, decls.At.ENCODING: 3}, ())).storage(1)

    def test_takes_an_enumeration_as_its_integer_and_tells_kinds_apart(self):
        unsigned = (decls.Tag.BASE_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.ENCODING: decls.Ate.UNSIGNED}, ())
        read = unit(unsigned, (decls.Tag.ENUMERATION_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.TYPE: 1}, ()),
                    (decls.Tag.BASE_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.ENCODING: decls.Ate.FLOAT}, ()))
        self.assertEqual(read.storage(2), read.storage(1))
        self.assertNotEqual(read.storage(3), read.storage(1))

    def test_tells_records_without_a_tag_apart_by_their_members(self):
        # 3 and 5 are structures of one int, 1, and 7 a structure of one float, 2, each without a tag
        records = []
        for at, member in ((3, 1), (5, 1), (7, 2)):
            records += [(decls.Tag.STRUCTURE_TYPE, {decls.At.BYTE_SIZE: 4}, (at + 1,)),
                        (decls.Tag.MEMBER, {decls.At.TYPE: member}, ())]
        read = unit((decls.Tag.BASE_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.ENCODING: decls.Ate.SIGNED}, ()),
                    (decls.Tag.BASE_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.ENCODING: decls.Ate.FLOAT}, ()), *records)
        self.assertEqual(read.key(3), read.key(5))
        self.assertNotEqual(read.key(3), read.key(7))

    def test_gives_each_type_of_a_cycle_every_place_it_reaches(self):
        # 1 is a list's node, whose member, 2, is a pointer to it, 3. Each order of asking gives both every place
        def node():
            return unit((decls.Tag.STRUCTURE_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.DECL_FILE: 0,
                                                    decls.At.DECL_LINE: 1}, (2,)),
                        (decls.Tag.MEMBER, {decls.At.TYPE: 3, decls.At.DECL_FILE: 0, decls.At.DECL_LINE: 2}, ()),
                        (decls.Tag.POINTER_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.TYPE: 1}, ()))
        for first, second in ((1, 3), (3, 1)):
            with self.subTest(first=first):
                read = node()
                self.assertEqual([read.origins(first), read.origins(second)], [{('unit.c', 1), ('unit.c', 2)}] * 2)

    def test_fails_on_a_record_a_call_passes_that_storage_cant_lay_out(self):
        # 2 is a structure of a bit-field, and 5 a union of a restrict pointer. 7 and 9 are functions that take them
        read = unit((decls.Tag.BASE_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.ENCODING: decls.Ate.SIGNED}, ()),
                    (decls.Tag.STRUCTURE_TYPE, {decls.At.BYTE_SIZE: 4}, (3,)),
                    (decls.Tag.MEMBER, {decls.At.TYPE: 1, decls.At.BIT_SIZE: 3}, ()),
                    (decls.Tag.POINTER_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.TYPE: 1}, ()),
                    (decls.Tag.UNION_TYPE, {decls.At.BYTE_SIZE: 4}, (6,)),
                    (decls.Tag.MEMBER, {decls.At.TYPE: 10}, ()),
                    (decls.Tag.SUBPROGRAM, {decls.At.PROTOTYPED: True}, (8,)),
                    (decls.Tag.FORMAL_PARAMETER, {decls.At.TYPE: 2}, ()),
                    (decls.Tag.SUBPROGRAM, {decls.At.PROTOTYPED: True}, (11,)),
                    (decls.Tag.RESTRICT_TYPE, {decls.At.TYPE: 4}, ()),
                    (decls.Tag.FORMAL_PARAMETER, {decls.At.TYPE: 5}, ()))
        for function, failure in ((7, 'declares a bit-field'), (9, 'a type of tag 0x37')):
            with self.subTest(failure), self.assertRaisesRegex(dwarf.Unread, failure):
                read.function(read.entry(function))

    def test_fails_on_a_packing_option_outside_the_abi(self):
        abi = ('-ml', '-m4-single-only', '-fpack-struct=4')
        decls.check_abi(decls.Unit({}, 'unit.c', [], f'GNU C17 16.2.0 {" ".join(abi)}'), abi)
        for option in ('-fpack-struct', '-fno-pack-struct'):
            with self.subTest(option), self.assertRaisesRegex(dwarf.Unread, f'compiled with {option},'):
                decls.check_abi(decls.Unit({}, 'unit.c', [], f'GNU C17 16.2.0 {" ".join(abi)} {option}'), abi)


class Inheriting(unittest.TestCase):
    # entries 1 and 2: an int and a float
    TYPES = ((decls.Tag.BASE_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.ENCODING: decls.Ate.SIGNED}, ()),
             (decls.Tag.BASE_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.ENCODING: decls.Ate.FLOAT}, ()))

    def test_takes_a_declaration_s_parameters_where_its_definition_gives_none(self):
        # 3 is a declaration of int f(float), its parameter at 4, and 5 its definition, which gives no parameters
        read = unit(*self.TYPES,
                    (decls.Tag.SUBPROGRAM, {decls.At.NAME: 'f', decls.At.EXTERNAL: True, decls.At.DECLARATION: True,
                                           decls.At.PROTOTYPED: True, decls.At.TYPE: 1}, (4,)),
                    (decls.Tag.FORMAL_PARAMETER, {decls.At.TYPE: 2}, ()),
                    (decls.Tag.SUBPROGRAM, {decls.At.SPECIFICATION: 3}, ()))
        entry = decls.merged(read, 5)
        self.assertEqual(entry.attrs[decls.At.NAME], 'f')
        self.assertNotIn(decls.At.DECLARATION, entry.attrs)
        self.assertEqual(read.function(entry), decls.Function(decls.Int(4), (decls.Float(),), False))

    def test_takes_an_abstract_instance_s_name_type_and_parameters_types(self):
        # 3 is g's abstract instance, its parameter at 4, and 5 a concrete instance, whose parameter at 6 has 4 for its
        # origin
        read = unit(*self.TYPES,
                    (decls.Tag.SUBPROGRAM, {decls.At.NAME: 'g', decls.At.EXTERNAL: True, decls.At.INLINE: 1,
                                           decls.At.PROTOTYPED: True, decls.At.TYPE: 1}, (4,)),
                    (decls.Tag.FORMAL_PARAMETER, {decls.At.TYPE: 2}, ()),
                    (decls.Tag.SUBPROGRAM, {decls.At.ABSTRACT_ORIGIN: 3}, (6,)),
                    (decls.Tag.FORMAL_PARAMETER, {decls.At.ABSTRACT_ORIGIN: 4}, ()))
        entry = decls.merged(read, 5)
        self.assertEqual(entry.attrs[decls.At.NAME], 'g')
        self.assertNotIn(decls.At.INLINE, entry.attrs)
        self.assertEqual(read.function(entry), decls.Function(decls.Int(4), (decls.Float(),), False))

    def test_fails_on_a_specification_that_comes_back_to_its_entry(self):
        read = unit((decls.Tag.SUBPROGRAM, {decls.At.SPECIFICATION: 2}, ()),
                    (decls.Tag.SUBPROGRAM, {decls.At.SPECIFICATION: 1}, ()))
        with self.assertRaisesRegex(dwarf.Unread, 'comes back to'):
            decls.merged(read, 1)


class Spelling(unittest.TestCase):
    def test_binds_each_qualifier_and_pointer_where_c_binds_it(self):
        # 1, an int; 2, int *; 3, int *const; 4, const int; 5, const int *; 6 and 7, int[3]; 8, int (*)[3]
        read = unit((decls.Tag.BASE_TYPE, {decls.At.NAME: 'int', decls.At.BYTE_SIZE: 4,
                                          decls.At.ENCODING: decls.Ate.SIGNED}, ()),
                    (decls.Tag.POINTER_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.TYPE: 1}, ()),
                    (decls.Tag.CONST_TYPE, {decls.At.TYPE: 2}, ()),
                    (decls.Tag.CONST_TYPE, {decls.At.TYPE: 1}, ()),
                    (decls.Tag.POINTER_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.TYPE: 4}, ()),
                    (decls.Tag.SUBRANGE_TYPE, {decls.At.UPPER_BOUND: 2}, ()),
                    (decls.Tag.ARRAY_TYPE, {decls.At.TYPE: 1}, (6,)),
                    (decls.Tag.POINTER_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.TYPE: 7}, ()))
        self.assertEqual([read.spelled(ref) for ref in (2, 3, 5, 7, 8)],
                         ['int *', 'int *const', 'const int *', 'int[3]', 'int (*)[3]'])

    def test_spells_each_qualifier_of_an_array_or_a_pointer_once(self):
        # 7 is volatile const int[3] as GCC gives it: a volatile const array of volatile const ints, its element at 3.
        # 10 is int *const volatile, each qualifier over the one below it
        read = unit((decls.Tag.BASE_TYPE, {decls.At.NAME: 'int', decls.At.BYTE_SIZE: 4,
                                          decls.At.ENCODING: decls.Ate.SIGNED}, ()),
                    (decls.Tag.CONST_TYPE, {decls.At.TYPE: 1}, ()),
                    (decls.Tag.VOLATILE_TYPE, {decls.At.TYPE: 2}, ()),
                    (decls.Tag.ARRAY_TYPE, {decls.At.TYPE: 3}, (5,)),
                    (decls.Tag.SUBRANGE_TYPE, {decls.At.UPPER_BOUND: 2}, ()),
                    (decls.Tag.CONST_TYPE, {decls.At.TYPE: 4}, ()),
                    (decls.Tag.VOLATILE_TYPE, {decls.At.TYPE: 6}, ()),
                    (decls.Tag.POINTER_TYPE, {decls.At.BYTE_SIZE: 4, decls.At.TYPE: 1}, ()),
                    (decls.Tag.CONST_TYPE, {decls.At.TYPE: 8}, ()),
                    (decls.Tag.VOLATILE_TYPE, {decls.At.TYPE: 9}, ()))
        self.assertEqual([read.spelled(7), read.spelled(10)], ['volatile const int[3]', 'int *const volatile'])

    def test_names_where_two_calls_differ(self):
        int_, float_ = decls.Int(4), decls.Float()
        for one, other, words in (
                (decls.Function(int_, (int_,), False), decls.Function(float_, (int_,), False),
                 'the value returns as int against float'),
                (decls.Function(int_, None, False), decls.Function(int_, (int_,), False),
                 'the first gives no prototype'),
                (decls.Function(int_, (int_,), False), decls.Function(int_, (int_, int_), False),
                 '1 parameter against 2'),
                (decls.Function(int_, (int_,), True), decls.Function(int_, (int_,), False),
                 'the first alone takes more'),
                (decls.Function(int_, (int_,), False), decls.Function(int_, (float_,), False),
                 'parameter 1 passes as int against float')):
            with self.subTest(words):
                self.assertEqual(one.difference(other), words)


class Places(unittest.TestCase):
    def test_names_a_path_as_the_tree_does(self):
        with tempfile.TemporaryDirectory() as root, cwd(Path(root)):
            for path in ('gcc/x.c', './gcc/x.c', 'src/../gcc/x.c', str(Path(root, 'gcc/x.c'))):
                with self.subTest(path):
                    self.assertEqual(decls.canonical(path), 'gcc/x.c')
            self.assertEqual(decls.canonical('/elsewhere/x.c'), '/elsewhere/x.c')
            self.assertEqual(decls.canonical('../x.c'), str(Path.cwd().parent / 'x.c'))

    def test_takes_the_matched_rule_only_at_a_known_line_of_a_file_in_the_tree(self):
        with tempfile.TemporaryDirectory() as root, cwd(Path(root)):
            Path('x.c').write_text('int a;\n#ifdef GCC\nint b;\n#endif\n')
            Path('tools').mkdir()
            Path('tools/x.h').write_text('int c;\n')
            check = decls.Check({}, '.')
            self.assertTrue(check.matched_at('x.c', 1))
            for file, line in (('x.c', 3), ('x.c', 99), ('x.c', None), (None, 1), ('gcc/x.c', 1), ('tools/x.h', 1),
                               (str(Path(root, 'x.c')), 1), ('gone.c', 1)):
                with self.subTest(file=file, line=line):
                    self.assertFalse(check.matched_at(file, line))


# functions that pass and return values in some of the ways the check's model of GCC's SH ABI takes
WITNESS = '''\
struct one {
    float f;
};
struct two {
    float a, b;
};
struct three {
    int a, b, c;
};
float take_float(float x) { return x * 2.0F; }
float take_one(struct one s) { return s.f * 2.0F; }
float take_two(struct two s) { return s.a * 2.0F; }
struct three make_three(int x) { struct three t = {x, x + 1, x + 2}; return t; }
int widen(short x) { return x; }
int call_widen(int y) { return widen(y) + 1; }
'''


class Witness(unittest.TestCase):
    def test_gcc_passes_and_returns_values_as_the_model_takes_them(self):
        missing = [tool for tool in test_ld.TOOLS if not (test_ld.TOOLCHAIN / tool).is_file()]
        if missing and not test_ld.GIVEN:
            raise unittest.SkipTest(f'{test_ld.TOOLCHAIN} lacks {", ".join(missing)}, which make gcc builds')
        with tempfile.TemporaryDirectory() as root:
            Path(root, 'witness.c').write_text(WITNESS)
            test_ld.run('bin/sh-elf-gcc', *test_ld.flags(), '-fno-inline', '-S', '-o', 'witness.s', 'witness.c',
                        root=root)
            code = {}
            for line in Path(root, 'witness.s').read_text().splitlines():
                if re.match(r'_\w+:', line):
                    name = line[1:-1]
                elif line.startswith('\t') and not line.startswith('\t.'):
                    code.setdefault(name, []).append(' '.join(line.split()))
        # a float, and a structure of one float, pass in FR4, and a float returns in FR0
        self.assertIn('fadd fr4,fr4', code['take_float'])
        self.assertIn('fmov fr4,fr0', code['take_float'])
        self.assertIn('fadd fr4,fr4', code['take_one'])
        # a structure of two floats passes by its size, in R4 and R5
        self.assertIn('lds r4,fpul', code['take_two'])
        # a structure of 12 bytes returns through memory, at the address in R2
        self.assertIn('mov.l r4,@r2', code['make_three'])
        # a caller extends a short, and its callee takes it as it is
        self.assertIn('exts.w r4,r4', code['call_widen'])
        self.assertNotIn('exts.w r4,r0', code['widen'])


class Check(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        missing = [tool for tool in test_ld.TOOLS if not (test_ld.TOOLCHAIN / tool).is_file()]
        if missing and not test_ld.GIVEN:
            raise unittest.SkipTest(f'{test_ld.TOOLCHAIN} lacks {", ".join(missing)}, which make gcc builds')
        if missing:
            raise AssertionError(f'{test_ld.TOOLCHAIN}, which SH_ELF names, lacks {", ".join(missing)}')
        # ld.py links the fixture with the toolchain, and the check leaves its libgcc.a unread
        patched = mock.patch.object(gcclink, 'SH_ELF', test_ld.TOOLCHAIN)
        patched.start()
        cls.addClassCleanup(patched.stop)
        cls.abi = test_ld.flags('GCC_ABI')
        cls.temporary = tempfile.TemporaryDirectory()
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.root = Path(cls.temporary.name)
        # DWARF names each source by its path under the fixture's directory, as make gcc's prefix map names one by its
        # path under the tree. GCC takes the later of two maps that match, so this one holds even where the directory
        # lies in the tree
        compiled = [*test_ld.flags(), f'-fdebug-prefix-map={cls.root.resolve()}=.']
        for name, text in SOURCES.items():
            (cls.root / name).parent.mkdir(exist_ok=True)
            (cls.root / name).write_text(text)
        for name in (name for name in SOURCES if name.endswith(('.c', '.s'))):
            test_ld.run('bin/sh-elf-gcc', *compiled, *FLAGS.get(name, []), '-c', '-o',
                        str(Path(name).with_suffix('.o')), name, root=cls.root)
        (cls.root / 'gone.c').unlink()
        # an archive of each list of members. chain1.o defines first and refers to chain2.o's second, and chain3.o
        # defines first otherwise
        for name, members in (('def', ['def.o']), ('return', ['return.o']), ('stray', ['stray.o']),
                              ('prov', ['prov.o']), ('chain', ['chain1.o', 'chain2.o']), ('other', ['chain3.o'])):
            test_ld.run('bin/sh-elf-ar', 'rc', f'lib{name}.a', *members, root=cls.root)
        # word.o without its DWARF
        test_ld.run('bin/sh-elf-objcopy', '--strip-debug', 'word.o', 'bare.o', root=cls.root)
        # return.o with its abbreviations zeroed
        raw = (cls.root / 'return.o').read_bytes()
        table = next(section.data for section in elf.read(raw, 'return.o').sections if section.name == '.debug_abbrev')
        start = raw.index(table)
        (cls.root / 'parse.o').write_bytes(raw[:start] + bytes(len(table)) + raw[start + len(table):])
        # zero.o with its array's DW_AT_type referring to the array itself or to no entry, or given in DW_FORM_ref_addr.
        # DW_AT_type is the array entry's first attribute, after its one-byte abbreviation code. The abbreviation
        # gives DW_AT_type's form first too, after the tag and the children flag
        raw = (cls.root / 'zero.o').read_bytes()
        found = elf.read(raw, 'zero.o')
        sections = {section.name: section.data for section in found.sections}
        array = next(offset for offset, die in decls.units(found)[0].dies.items() if die.tag == decls.Tag.ARRAY_TYPE)
        reference = raw.index(sections['.debug_info']) + array + 1
        form = raw.index(bytes((decls.Tag.ARRAY_TYPE, 1, decls.At.TYPE, decls.Form.REF4)),
                         raw.index(sections['.debug_abbrev'])) + 3
        for name, at, value in (('cycle.o', reference, array.to_bytes(4, 'little')),
                                ('dangling.o', reference, (0x1000).to_bytes(4, 'little')),
                                ('far.o', form, bytes((decls.Form.REF_ADDR,)))):
            (cls.root / name).write_bytes(raw[:at] + value + raw[at + len(value):])

    def linked(self, spec):
        """The images spec gives, as gcclink.read() reads them, linked under the fixture's link/ as make gcc links them.
        Each image takes libgcc.a after its inputs, as spec.py gives it."""
        images = [line.split()[1] for line in spec.splitlines() if line.startswith('image')]
        (self.root / 'spec.txt').write_text(spec + ''.join(f'input  {image}  {gcclink.libgcc()}\n' for image in images))
        with cwd(self.root):
            images = gcclink.read('spec.txt')
            ld.coordinate(images, Path('link'))
        return images

    def problems(self, spec):
        """The text of each problem the check finds in the images spec gives, linked as linked() links them."""
        images = self.linked(spec)
        with cwd(self.root):
            return [problem.text for problem in decls.Check(images, 'link', runtime=str(gcclink.libgcc())).problems()]

    def admitted(self, spec, rows):
        """What verify.py's main() makes of the images spec gives, linked as linked() links them. rows is
        config/declarations.txt's text, or None for no table. It returns None where the check passes, and else its
        failure's text."""
        self.linked(spec)
        (self.root / 'config').mkdir(exist_ok=True)
        table = self.root / 'config/declarations.txt'
        table.unlink(missing_ok=True)
        if rows is not None:
            table.write_text(rows)
            self.addCleanup(table.unlink, missing_ok=True)
        with cwd(self.root):
            try:
                verify.main([f'--abi={" ".join(self.abi)}', 'spec.txt', 'link'])
            except SystemExit as failure:
                return str(failure)
        return None

    def test_passes_declarations_that_agree_with_their_definitions(self):
        self.assertEqual(self.problems(SPEC.format('agrees.o')), [])

    def test_passes_declarations_whose_bytes_and_calls_gcc_reads_alike(self):
        # each object declared reads only bytes its definition holds, with the same qualifiers, and aligns to no more
        # than the definition. where.c's call takes an int from R0, where where() returns its pointer. tail.c's
        # passes its structure in FR4, as GCC passes the float it holds
        for source in ('stride.c', 'open.c', 'flat.c', 'grid.c', 'rack.c', 'sized.c', 'pieces.c', 'whole.c', 'firm.c',
                       'opaque.c', 'pointer.c', 'where.c', 'tail.c'):
            with self.subTest(source):
                self.assertEqual(self.problems(SPEC.format(Path(source).with_suffix('.o'))), [])

    def test_holds_code_the_matching_build_doesnt_prove_to_its_elements(self):
        # past the outer extent, def.c's rows holds int[4] elements, as gcc/open.c's does, where gcc/flat.c's and
        # gone.c's hold ints. guarded.c's word is a structure of two shorts, where def.c's is an unsigned int
        self.assertEqual(self.problems(SPEC.format('gcc/open.o')), [])
        rows = f'{at("def.c", "int rows[2][4]")} defines it int[2][4]: elements of other types'
        for source, problem in (('gcc/flat.c', f'rows: gcc/flat.c:1 declares it int[8], and {rows}'),
                                ('guarded.c', f'word: {at("guarded.c", "extern")} declares it struct pieces, and '
                                              f'{at("def.c", "int word")} defines it unsigned int: elements of other '
                                              'types'),
                                ('gone.c', f'rows: gone.c:1 declares it int[8], and {rows}')):
            with self.subTest(source):
                self.assertEqual(self.problems(SPEC.format(Path(source).with_suffix('.o'))), [problem])

    def test_holds_code_the_matching_build_doesnt_prove_to_its_scalars_kinds_and_its_pointers_targets(self):
        # kind.c declares counter a float, where def.c defines it an int. pointee.c declares deep volatile int **,
        # where def.c defines it int **. Each agrees byte for byte, but under gcc/ the strict rule tells their
        # elements apart
        for source in ('kind.c', 'pointee.c'):
            with self.subTest(source):
                self.assertEqual(self.problems(SPEC.format(Path(source).with_suffix('.o'))), [])
        for source, problem in (
                ('gcc/kind.c', f'counter: gcc/kind.c:1 declares it float, and {at("def.c", "int counter")} defines it '
                               'int: elements of other types'),
                ('gcc/pointee.c', f'deep: gcc/pointee.c:1 declares it volatile int **, and {at("def.c", "int **deep")} '
                                  'defines it int **: elements of other types')):
            with self.subTest(source):
                self.assertEqual(self.problems(SPEC.format(Path(source).with_suffix('.o'))), [problem])

    def test_passes_by_the_matched_rule_each_pair_the_strict_rule_passes(self):
        # disagreement() asks the matched rule only of the pairs the strict rule fails, so the matched rule must pass
        # each pair the strict rule passes
        refused = {'bits.c', 'atomic.c', 'restrict.c', 'complex.c', 'packed.c', 'dwarf4.c', 'split.c', 'gone.c'}
        shapes = set()
        with cwd(self.root):
            for source in SOURCES:
                if source.endswith('.c') and source not in refused:
                    obj = str(Path(source).with_suffix('.o'))
                    shapes.update(each.shape for each in decls.declared(elf.read(Path(obj).read_bytes(), obj))[1])
        for one in shapes:
            for other in shapes:
                for defined in (True, False):
                    if decls.agree(one, other, defined, False):
                        self.assertTrue(decls.agree(one, other, defined, True), (one, other, defined))

    def test_holds_declarations_that_agree_with_their_definition_to_each_other(self):
        # def.c's pointer to data agrees with both pointers to code, but their calls pass the argument otherwise
        self.assertEqual(self.problems(SPEC.format('cb_int.o') + 'input  main  cb_float.o\n'), [
            'callback: cb_float.c:1 declares it int (*)(float), and cb_int.c:1 declares it int (*)(int): at offset 0, '
            'a pointer to code int (float) against a pointer to code int (int)'])
        # each alone agrees with it
        for source in ('cb_int.o', 'cb_float.o'):
            with self.subTest(source):
                self.assertEqual(self.problems(SPEC.format(source)), [])

    def test_holds_a_declaration_whose_type_gccs_side_defines_to_its_elements(self):
        # typeguard.c declares word unguarded, of a structure that GCC's side of a guard defines, and memberguard.c of
        # one whose members GCC's side declares
        self.assertEqual(self.problems(SPEC.format('typeguard.o')), [
            f'word: {at("typeguard.c", "extern")} declares it struct twin, and {at("def.c", "int word")} defines it '
            'unsigned int: elements of other types'])
        self.assertEqual(self.problems(SPEC.format('memberguard.o')), [
            f'word: {at("memberguard.c", "extern")} declares it struct pieces, and {at("def.c", "int word")} defines '
            'it unsigned int: elements of other types'])

    def test_keeps_an_alignment_dwarf_gives_past_a_word(self):
        # matrix.c's type aligns to 8, as NJS_MATRIX does, where def.c's float[16] aligns to 4
        self.assertEqual(self.problems(SPEC.format('matrix.o'))[0],
                         f'plain: matrix.c:2 declares it matrix, and {at("def.c", "float plain[16]")} defines it '
                         'float[16]: aligned to 8 bytes against 4')

    def test_fails_on_a_declaration_whose_name_the_link_places_off_its_alignment(self):
        # held.s places skew 4 bytes past a multiple of 8, and skew.c's type aligns to 8. The check holds a
        # declaration to its alignment whatever defines the name, here assembly
        problems = self.problems(SPEC.format('skew.o'))
        value = gcclink.exports(elf.read((self.root / 'link/main/final.elf').read_bytes(), 'final.elf'))['skew'].value
        self.assertEqual((value % 8, problems), (4, [
            f'skew: skew.c:2 declares it matrix, which aligns to 8, and the link places it at {value:#010x}']))

    def test_fails_on_a_member_packed_below_its_alignment(self):
        with self.assertRaisesRegex(SystemExit, r"packed\.o: packed\.c:3 declares a member packed below its "
                                                r"alignment, which decls\.py doesn't read\Z"):
            self.problems(SPEC.format('packed.o'))

    def test_fails_on_an_object_past_its_definitions_bytes_or_aligned_to_more(self):
        # deep.c's table reads 20 bytes in its first element, and aligned.c loads halves with a MOV.L, which faults
        # where GCC's link places halves off 4
        for source, problem in (('count.c', f'table: count.c:1 declares it int[8], and {at("def.c", "int table[4]")} '
                                            'defines it int[4]: 32 bytes against 16'),
                                ('deep.c', f'table: deep.c:1 declares it int[][5], and {at("def.c", "int table[4]")} '
                                           'defines it int[4]: an element of 20 bytes against 16'),
                                ('aligned.c', f'halves: aligned.c:1 declares it int, and {at("def.c", "short halves")} '
                                              'defines it short int[2]: aligned to 4 bytes against 2')):
            with self.subTest(source):
                self.assertEqual(self.problems(SPEC.format(Path(source).with_suffix('.o'))), [problem])

    def test_fails_on_a_byte_one_object_reads_and_the_other_holds_as_padding(self):
        # GCC's code can store a structure member by member and leave its padding as it was. *p = t for def.c's
        # struct counted stores its int and its char alone (mov.l r5,@r4 and mov.b r0,@(4,r4))
        for source, problem in (('counted.c', f'counted: {at("counted.c", "extern")} declares it struct counted, and '
                                              f'{at("def.c", "} counted;")} defines it struct counted: at offset 5, a '
                                              'scalar of 1 byte against padding'),
                                ('moved.c', f'moved: {at("moved.c", "extern")} declares it struct moved, and '
                                            f'{at("def.c", "} moved;")} defines it struct moved: at offset 2, a scalar '
                                            'of 1 byte against padding'),
                                ('member.c', f'pair: {at("member.c", "extern")} declares it struct pair, and '
                                             f'{at("def.c", "struct pair pair =")} defines it struct pair: a scalar of '
                                             '4 bytes at offset 4 against padding at offset 6')):
            with self.subTest(source):
                self.assertEqual(self.problems(SPEC.format(Path(source).with_suffix('.o'))), [problem])

    def test_fails_on_a_const_or_volatile_byte_or_a_volatile_pointee_of_one_object_alone(self):
        for source, problem in (('qualifier.c', f'limit: qualifier.c:1 declares it int, and '
                                                f'{at("def.c", "limit = 3")} defines it const int: at offset 0, a '
                                                'scalar of 4 bytes against a const scalar of 4 bytes'),
                                ('volatile.c', f'ticks: volatile.c:1 declares it int, and {at("def.c", "int ticks")} '
                                               'defines it volatile int: at offset 0, a scalar of 4 bytes against a '
                                               'volatile scalar of 4 bytes'),
                                # a const record's qualifier lies on each member
                                ('fixed.c', f'fixed: {at("fixed.c", "extern")} declares it struct pair, and '
                                            f'{at("def.c", "pair fixed")} defines it const struct pair: at offset 0, a '
                                            'scalar of 4 bytes against a const scalar of 4 bytes'),
                                ('hook.c', f'hook: hook.c:1 declares it int (*)(), and {at("def.c", "(*const hook)")} '
                                           'defines it int (*const)(void): at offset 0, a pointer to code int (?) '
                                           'against a const pointer to code int ()'),
                                # GCC's code reads the data a pointer points to each time where it is volatile
                                ('flag.c', f'flag: flag.c:1 declares it int *, and {at("def.c", "int *flag")} defines '
                                           'it volatile int *: at offset 0, a pointer to data against a pointer to '
                                           'volatile data')):
            with self.subTest(source):
                self.assertEqual(self.problems(SPEC.format(Path(source).with_suffix('.o'))), [problem])

    def test_fails_on_a_call_that_passes_or_returns_a_value_otherwise(self):
        for source, problem in (('return.c', f'take: return.c:1 declares it float (int), and {TAKE} defines it int '
                                             '(int)'),
                                ('parameter.c', f'take: parameter.c:1 declares it int (float), and {TAKE} defines it '
                                                'int (int)'),
                                ('variadic.c', f'take: variadic.c:1 declares it int (int, ...), and {TAKE} defines it '
                                               'int (int)'),
                                ('tone.c', f'tone: tone.c:1 declares it unsigned char (void), and '
                                           f'{at("def.c", "char tone(")} defines it signed char (void)'),
                                # a call through getter.c's pointer takes a float from FR0, and the function
                                # def.c's pointer points to returns an int in R0
                                ('getter.c', f'getter: getter.c:1 declares it float (*)(void), and '
                                             f'{at("def.c", "(*getter)")} defines it int (*)(void): at offset 0, a '
                                             'pointer to code float () against a pointer to code int ()'),
                                # GCC passes def.c's structure of a float in FR4, and cell.c's structure of an int and
                                # shell.c's union of a float in R4
                                ('cell.c', f'weight: {at("cell.c", "extern")} declares it float (struct cell), and '
                                           f'{WEIGHT} defines it float (struct cell): parameter 1 passes as int '
                                           'against float'),
                                ('shell.c', f'weight: {at("shell.c", "extern")} declares it float (union cell), and '
                                            f'{WEIGHT} defines it float (struct cell)'),
                                # GCC returns quad.c's structure of an int in R0, and def.c's structure of four chars,
                                # which aligns to 1, through memory
                                ('quad.c', f'quad_of: {at("quad.c", "extern")} declares it struct word (int), and '
                                           f'{at("def.c", "quad quad_of(")} defines it struct quad (int)'),
                                # GCC gives the extent of def.c's array by a reference, and vla.c's by an expression
                                ('vla.c', f'sum: vla.c:1 declares it float (int, int (*)[*]), and '
                                          f'{at("def.c", "int sum(")} defines it int (int, int (*)[*])')):
            with self.subTest(source):
                self.assertEqual(self.problems(SPEC.format(Path(source).with_suffix('.o'))), [problem])

    def test_fails_on_a_declaration_without_a_prototype_whose_parameters_a_promotion_changes(self):
        # a call through promote.c's declarations passes an int where narrow takes a short and flagged a _Bool, and
        # no more where vary takes more, as its ... says
        self.assertEqual(self.problems(SPEC.format('promote.o')), [
            f'flagged: {at("promote.c", "flagged")} declares it int (), and {at("def.c", "int flagged(")} '
            'defines it int (_Bool)',
            f'vary: {at("promote.c", "vary")} declares it int (), and {at("def.c", "int vary(")} defines it int '
            '(int, ...)',
            f'narrow: {at("promote.c", "narrow")} declares it int (), and {at("def.c", "int narrow(")} defines it '
            'int (short int)'])

    def test_fails_on_dwarf_it_doesnt_read(self):
        for source, failure in (('bits.c', rf"bits\.o: {re.escape(at('bits.c', ': 3'))} declares a bit-field, which "
                                           r"decls\.py doesn't read"),
                                ('atomic.c', r"atomic\.o: its DWARF gives a type of tag 0x47, which decls\.py doesn't "
                                             r"read"),
                                ('restrict.c', r"restrict\.o: its DWARF gives a type of tag 0x37, which decls\.py "
                                               r"doesn't read"),
                                ('complex.c', r"complex\.o: its DWARF gives complex float the encoding 3, which "
                                              r"decls\.py doesn't read")):
            with self.subTest(source), self.assertRaisesRegex(SystemExit, rf'\A[^\n]*: {failure}\Z'):
                self.problems(SPEC.format(Path(source).with_suffix('.o')))

    def test_reads_a_form_make_gccs_objects_dont_hold_and_an_extent_given_as_a_count(self):
        # banner.o gives a value in DW_FORM_block2, as DW_AT_const_value, an attribute the check doesn't read
        with cwd(self.root):
            found = elf.read((self.root / 'banner.o').read_bytes(), 'banner.o')
        table = next(section.data for section in found.sections if section.name == '.debug_abbrev')
        self.assertIn(decls.Form.BLOCK2, {form for _, _, specs in dwarf.abbreviations(table, 0).values()
                                          for _, form, _ in specs})
        self.assertEqual(self.problems(SPEC.format('banner.o')), [])
        # GCC gives the extent of zero.c's halves, 0, as DW_AT_count
        self.assertEqual(self.problems(SPEC.format('zero.o')), [
            f'halves: zero.c:1 declares it int[0], and {at("def.c", "short halves")} defines it short int[2]: aligned '
            'to 4 bytes against 2'])

    def test_fails_on_an_import_or_a_second_name_whose_declaration_disagrees_with_its_definition(self):
        self.assertEqual(self.problems(IMPORT), [
            f"take: return.c:1 declares it float (int), and main's {TAKE} defines it int (int)"])
        self.assertEqual(self.problems(ALIAS), [
            f'second: second.c:1 declares it float (int), and {TAKE} defines take int (int)'])

    def test_holds_each_declaration_of_a_name_no_c_object_defines_to_each_other_across_images(self):
        # given's assembly holds no DWARF, so the check compares its importers' declarations with its own image's,
        # or with each other
        provided = ('image  main  0x8c010000  0x00010000\ninput  main  provide.o\n{}'
                    'image  ovl  0x0c900000  0x00010000\ninput  ovl  {}\nimport  ovl  given  main  given  0x8c\n{}')
        self.assertEqual(self.problems(provided.format('input  main  given.o\n', 'other.o', '')), [
            "given: other.c:1 declares it float (int), and main's given.c:1 declares it int (int)"])
        other = 'image  ova  0x0c900000  0x00010000\ninput  ova  other.o\nimport  ova  given  main  given  0x8c\n'
        self.assertEqual(self.problems(provided.format('', 'given.o', other)), [
            "given: other.c:1 declares it float (int), and ovl's given.c:1 declares it int (int)"])
        # with no definition, no count is compared, so many.c's eight ints agree with few.c's four
        self.assertEqual(self.problems(
            'image  main  0x8c010000  0x00010000\ninput  main  few.o\ninput  main  many.o\ninput  main  held.o\n'), [])
        # the check compares two arrays of no outer extent over the least common multiple of their elements' sizes:
        # 12 bytes, for ring2.c's 4 and ring3.c's 6. There ring2.c's second const short lies over a short of
        # ring3.c's that isn't const
        self.assertEqual(self.problems(
            'image  main  0x8c010000  0x00010000\ninput  main  ring2.o\ninput  main  ring3.o\ninput  main  held.o\n'), [
            f'ring: {at("ring3.c", "extern")} declares it struct three[], and {at("ring2.c", "extern")} declares '
            'it struct two[]: at offset 6, a scalar of 2 bytes against a const scalar of 2 bytes'])
        # no definition binds shared, so the check compares its declarations with each other, whichever comes
        # first. unprototyped.c's agrees with both others, which disagree
        self.assertEqual(self.problems(
            'image  main  0x8c010000  0x00010000\ninput  main  unprototyped.o\ninput  main  integer.o\n'
            'input  main  held.o\n'
            'input  main  real.o\n'), [
            'shared: real.c:1 declares it void (float), and integer.c:1 declares it void (int)'])
        # a row for either of two declarations that disagree admits their problem, whichever comes first. Rows for
        # both admit it too, and neither is stale
        for first, second in (('integer.o', 'real.o'), ('real.o', 'integer.o')):
            spec = (f'image  main  0x8c010000  0x00010000\ninput  main  {first}\ninput  main  {second}\n'
                    'input  main  held.o\n')
            for rows in ('shared  integer.c  a reason\n', 'shared  real.c  a reason\n',
                         'shared  integer.c  a reason\nshared  real.c  another\n'):
                with self.subTest(first=first, rows=rows):
                    self.assertIsNone(self.admitted(spec, rows))

    def test_names_the_object_whose_header_declares_a_name_otherwise_and_a_row_for_the_header_admits_each(self):
        spec = SPEC.format('header1.o') + 'input  main  header2.o\n'
        self.assertEqual(self.problems(spec), [
            f'take: sub/header.h:1 (in header1.c) declares it int (float), and {TAKE} defines it int (int)',
            f'take: sub/header.h:1 (in header2.c) declares it int (float), and {TAKE} defines it int (int)'])
        self.assertIsNone(self.admitted(spec, 'take  sub/header.h  every includer\n'))

    def test_reads_the_line_table_gnu_as_writes_at_o0(self):
        # GNU as numbers early.o's files from 1, and gives sub/early.h its directory
        with cwd(self.root):
            table = next(section.data for section in elf.read((self.root / 'early.o').read_bytes(), 'early.o').sections
                         if section.name == '.debug_line')
        self.assertEqual(int.from_bytes(table[4:6], 'little'), 3)
        self.assertEqual(self.problems(SPEC.format('early.o')), [
            f'take: sub/early.h:1 (in early.c) declares it float (int), and {TAKE} defines it int (int)'])

    def test_reads_an_archives_member_that_defines_a_name_an_object_declares_and_no_object_defines(self):
        # libdef.a's def.o defines take, which return.o declares and doesn't define
        self.assertEqual(self.problems('image  main  0x8c010000  0x00010000\ninput  main  return.o\n'
                                       'input  main  libdef.a\n'), [
            f'take: return.c:1 declares it float (int), and {TAKE} defines it int (int)'])
        # def.o refers to no name libreturn.a's return.o defines, so the link leaves return.o out, and its declaration
        # of take goes unread. provide.o defines given, so the link leaves out libstray.a's stray.o too, which defines
        # given otherwise
        self.assertEqual(self.problems(SPEC.format('libreturn.a')), [])
        self.assertEqual(self.problems('image  main  0x8c010000  0x00010000\ninput  main  provide.o\n'
                                       'input  main  given.o\ninput  main  libstray.a\n'), [])

    def test_reads_each_member_the_link_takes_and_no_other(self):
        main = 'image  main  0x8c010000  0x00010000\ninput  main  root.o\n'
        # root.o refers to first, so the link adds libchain.a's chain1.o, which refers to chain2.o's second.
        # libother.a's chain3.o, which defines first otherwise, then goes unread. With libother.a first, the link adds
        # chain3.o instead
        self.assertEqual(self.problems(main + 'input  main  libchain.a\ninput  main  libother.a\n'), [
            'second: chain1.c:1 declares it int (float), and chain2.c:1 defines it int (int)'])
        self.assertEqual(self.problems(main + 'input  main  libother.a\ninput  main  libchain.a\n'), [
            'first: root.c:1 declares it int (void), and chain3.c:1 defines it float (void)'])
        # no object of main's refers to provided, but ovl imports it, so the link adds libprov.a's prov.o
        self.assertEqual(self.problems(
            'image  main  0x8c010000  0x00010000\ninput  main  libprov.a\nimage  ovl  0x0c900000  0x00010000\n'
            'input  ovl  cons.o\nimport  ovl  provided  main  provided  0x8c\n'), [
            "provided: cons.c:1 declares it float (int), and main's prov.c:1 defines it int (int)"])

    def test_takes_the_definition_the_link_binds_a_name_to(self):
        # the link binds value to strong.c's in either order, and the check compares weak.c's definition, of 32
        # bytes, with it as a declaration
        for first, second in (('weak.o', 'strong.o'), ('strong.o', 'weak.o')):
            with self.subTest(first=first):
                self.assertEqual(self.problems(f'image  main  0x8c010000  0x00010000\ninput  main  {first}\n'
                                               f'input  main  {second}\n'), [
                    'value: weak.c:1 defines it int[8], and strong.c:1 defines it int: 32 bytes against 4'])

    def test_binds_a_declaration_as_the_link_binds_its_symbol(self):
        # labeled.c's asm label binds private_name to word.c's shared_word
        self.assertEqual(self.problems('image  main  0x8c010000  0x00010000\ninput  main  labeled.o\n'
                                       'input  main  word.o\n'), [
            'private_name: labeled.c:1 declares it volatile int, and word.c:1 defines shared_word int: at offset 0, '
            'a volatile scalar of 4 bytes against a scalar of 4 bytes'])
        # GCC declares the memcpy its __builtin_memcpy calls at <built-in>, without the type of the call, so the
        # check reads no declaration there to hold to copy.c's memcpy
        self.assertEqual(self.problems('image  main  0x8c010000  0x00010000\ninput  main  copier.o\n'
                                       'input  main  copy.o\n'), [])
        # GCC drops gcc/dead.c's one load of counter and keeps its declaration, a float, where def.c defines an int.
        # gcc/dead.o holds no symbol of counter for the declaration to bind
        self.assertEqual(self.problems(SPEC.format('gcc/dead.o')), [])

    def test_fails_on_c_without_dwarf_and_a_name_it_binds_to_libgcc(self):
        # bare.o is word.o without its DWARF. empty.c defines and takes nothing, so GCC gives it no DWARF to read
        with self.assertRaisesRegex(SystemExit, r"\A[^\n]*: bare\.o: GCC compiled it, as its \.comment says, and "
                                                r"it defines or takes a name with no DWARF to read its declaration "
                                                r"from\Z"):
            self.problems(SPEC.format('bare.o'))
        self.assertEqual(self.problems(SPEC.format('empty.o')), [])
        # the link binds clz.c's __clzsi2 to libgcc.a's, whose units GCC compiles under the toolchain's defaults
        (problem,) = self.problems('image  main  0x8c010000  0x00010000\ninput  main  clz.o\n')
        self.assertRegex(problem, r"\A__clzsi2: clz\.c:1 declares it int \(unsigned int\), and the link binds it to "
                                  r"\S*/libgcc\.a\(_clzsi2\.o\), whose DWARF decls\.py doesn't read\Z")

    def test_fails_on_a_unit_or_a_reference_it_doesnt_read_or_dwarf_that_doesnt_parse(self):
        # the link fails on sections only DWARF 4 and a split unit hold, so the check reads such units only in an
        # input read alone
        for name, failure in (('dwarf4.o', 'holds a unit of DWARF 4, and dwarf\\.py reads DWARF 5 alone'),
                              ('split.o', 'holds a DWARF unit of kind 4, and dwarf\\.py reads compilation units '
                                          'alone')):
            with self.subTest(name), cwd(self.root):
                with self.assertRaisesRegex(SystemExit, rf'\A[^\n]*: {name}: {failure}\Z'):
                    decls.declared(elf.read(Path(name).read_bytes(), name))
        for name, failure in (('far.o', "its DWARF gives DW_AT_type in DW_FORM_ref_addr, "
                                        "which the check doesn't read"),
                              ('dangling.o', 'its DWARF gives DW_AT_type a reference outside its unit'),
                              ('cycle.o', "its DWARF holds references that nest deeper than Python's recursion goes, "
                                          "as a cycle of them does"),
                              ('parse.o', "its DWARF holds an entry of abbreviation 1, which its table doesn't give")):
            with self.subTest(name), self.assertRaisesRegex(SystemExit, rf'\A[^\n]*: {name}: {failure}\Z'):
                self.problems(SPEC.format(name))

    def test_a_row_admits_its_declaration_and_fails_where_it_admits_none(self):
        spec = SPEC.format('qualifier.o') + 'input  main  agrees.o\n'
        problem = (rf'main: limit: qualifier\.c:1 declares it int, and {re.escape(at("def.c", "limit = 3"))} defines '
                   r'it const int: at offset 0, a scalar of 4 bytes against a const scalar of 4 bytes')
        self.assertRegex(self.admitted(spec, ''), rf'\A[^\n]*: {problem}\Z')
        self.assertIsNone(self.admitted(spec, 'limit  qualifier.c  the reason\n'))
        # agrees.c's limit agrees, so its row is stale. qualifier.c declares no limits, and the spec links its object,
        # so that row is stale too. The spec leaves member.c's object out, as a partial link can, so its row isn't
        self.assertRegex(self.admitted(spec, 'limit  qualifier.c  the reason\npair  member.c  another\n'
                                             'limit  agrees.c  a third\nlimits  qualifier.c  a fourth\n'),
                         r'\A[^\n]*: config/declarations\.txt:3: admits no disagreement, so the row is stale\n'
                         r'[^\n]*: config/declarations\.txt:4: admits no disagreement, so the row is stale\Z')
        # a line that isn't a row fails, and so does a row for a file that doesn't exist. So do a row that gives the
        # name and file of a row above it, and a missing table
        for rows, failure in (('limit  qualifier.c\n', r'config/declarations\.txt:1: not a row: NAME FILE REASON'),
                              ('limit  nowhere.c  the reason\n',
                               r'config/declarations\.txt:1: nowhere\.c: no such file'),
                              ('limit  qualifier.c  the reason\nlimit  qualifier.c  again\n',
                               r'config/declarations\.txt:2: limit qualifier\.c has a row already, at '
                               r'config/declarations\.txt:1'),
                              ('limit  qualifier.c  the reason\nlimit  ./qualifier.c  again\n',
                               r'config/declarations\.txt:2: limit qualifier\.c has a row already, at '
                               r'config/declarations\.txt:1'),
                              (None, r'config/declarations\.txt: No such file or directory')):
            with self.subTest(rows):
                self.assertRegex(self.admitted(spec, rows), rf'\A[^\n]*: {failure}\Z')

    def test_fails_on_a_c_unit_compiled_outside_the_abi_it_reads(self):
        self.assertIsNone(self.admitted(SPEC.format('agrees.o'), ''))
        self.assertRegex(self.admitted(SPEC.format('loose.o'), ''),
                         r"\A[^\n]*: loose\.o: holds loose\.c compiled with -mrenesas, outside the ABI "
                         r"decls\.py reads, -ml -m4-single-only -fpack-struct=4\Z")

    def test_names_an_image_and_how_many_more_give_the_same_text(self):
        both = ('image  main  0x8c010000  0x00010000\ninput  main  def.o\ninput  main  qualifier.o\n'
                'image  ovl  0x0c900000  0x00010000\ninput  ovl  def.o\ninput  ovl  qualifier.o\n')
        self.assertRegex(self.admitted(both, ''), rf'\A[^\n]*: main and 1 more: limit: qualifier\.c:1 declares it '
                                                  rf'int, and {re.escape(at("def.c", "limit = 3"))} defines it const '
                                                  r'int: at offset 0, a scalar of 4 bytes against a const scalar of 4 '
                                                  r'bytes\Z')
        # a row admits its declaration in every image that links it
        self.assertIsNone(self.admitted(both, 'limit  qualifier.c  the reason\n'))


if __name__ == '__main__':
    unittest.main()
