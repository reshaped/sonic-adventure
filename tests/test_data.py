import contextlib
import io
import re
import struct
import unittest
from decimal import Decimal
from pathlib import Path
from unittest import mock

from config import FIRST_FILE_END
import data
from data import (TOKEN, Array, Data, EmitError, Item, LayoutError, Memory, Pointer, ReadError, Record, Scalar, Types,
                  Writer, f32, mdata_keys, model_at, motion_at, number, object_at, spell, texlist_at)
from support import tree


XPT = '''typedef unsigned char Uint8;
typedef signed char Sint8;
typedef unsigned short Uint16;
typedef short Sint16;
typedef unsigned long Uint32;
typedef long Sint32;
typedef float Float;
enum bool { false, true };
typedef enum bool Bool;
'''


def types(*texts):
    return Types([XPT, *texts])


def offsets(layout):
    return [(name, offset) for name, _, offset in layout.fields]


def member(layout, name):
    return next(field for each, field, _ in layout.fields if each == name)


class Layouts(unittest.TestCase):
    def test_scalars_take_shcs_sizes(self):
        found = types()
        for ctype, code in (('char', 'c'), ('signed char', 'b'), ('Uint8', 'B'), ('short', 'h'), ('Uint16', 'H'),
                            ('int', 'i'), ('unsigned', 'I'), ('long', 'i'), ('Uint32', 'I'), ('Float', 'f'),
                            ('Bool', 'i'), ('enum bool', 'i')):
            with self.subTest(ctype=ctype):
                self.assertEqual(found.layout(ctype), Scalar(code))

    def test_a_struct_aligns_each_member_and_pads_its_end(self):
        layout = types('struct a { Uint8 b; Sint16 h; Uint8 c; Sint32 w; Sint8 e; };').layout('struct a')
        self.assertEqual(offsets(layout), [('b', 0), ('h', 2), ('c', 4), ('w', 8), ('e', 12)])
        self.assertEqual((layout.size, layout.alignment, layout.holes), (16, 4, ((1, 1), (5, 3), (13, 3))))

    def test_one_declaration_declares_each_member(self):
        layout = types('struct a { Float x, y; Uint8 *p, m[3]; };').layout('struct a')
        self.assertEqual(offsets(layout), [('x', 0), ('y', 4), ('p', 8), ('m', 12)])
        self.assertEqual(member(layout, 'p'), Pointer('Uint8'))
        self.assertEqual(member(layout, 'm'), Array(Scalar('B'), 3))

    def test_a_union_holds_its_largest_member_and_writes_its_first(self):
        layout = types('union u { Uint8 b; Uint32 w; Sint16 h[3]; };').layout('union u')
        self.assertTrue(layout.union)
        self.assertEqual((layout.size, layout.holes), (8, ((1, 7),)))

    def test_typedefs_of_records_arrays_and_pointers(self):
        found = types('typedef struct { Float x, y; } P2, *P2P;\ntypedef Float TABLE[4][2];\n'
                      'typedef struct node { struct node *next; P2 at; } NODE;')
        self.assertEqual(found.layout('P2').size, 8)
        # an unnamed struct has no name a cast can take
        self.assertEqual(found.layout('P2P'), Pointer(None))
        self.assertEqual(found.layout('TABLE'), Array(Array(Scalar('f'), 2), 4))
        self.assertEqual(offsets(found.layout('NODE')), [('next', 0), ('at', 4)])
        self.assertEqual(member(found.layout('NODE'), 'next'), Pointer('struct node'))

    def test_a_pointer_names_its_pointee_as_a_cast_would(self):
        layout = types('struct t; struct a { const Uint16 (*rows)[4]; struct t **list; void (*f)(int); '
                       'const Uint8 *bytes; };').layout('struct a')
        self.assertEqual([field for _, field, _ in layout.fields],
                         [Pointer('const Uint16[]'), Pointer('struct t *'), Pointer(None), Pointer('const Uint8')])

    def test_a_record_inside_a_record(self):
        layout = types('struct a { Uint8 b; struct { Sint16 h; union { Float f; Uint8 c; } u; } in; };').layout(
            'struct a')
        self.assertEqual(offsets(layout), [('b', 0), ('in', 4)])
        self.assertIsInstance(member(layout, 'in'), Record)
        self.assertEqual(layout.size, 12)

    def test_each_source_defines_its_own_record(self):
        # two files' struct a, as two sources of one image define them
        self.assertEqual(types('struct a { Sint32 w; Uint8 b; };').layout('struct a').size, 8)
        self.assertEqual(types('struct a { Uint8 b; };').layout('struct a').size, 1)

    def test_comments_strings_and_directives_hide_no_declaration(self):
        found = types('/* struct a { int x; }; */\nchar *s = "struct a { char x; }";\n#if 0\n#endif\n'
                      '// struct a { short x; };\nstruct a { Float f; };')
        self.assertEqual(member(found.layout('struct a'), 'f'), Scalar('f'))

    def test_a_comment_inside_a_macro_joins_its_lines(self):
        # a comment's line break ends no directive, spliced or not
        for macro in ('#define N \\\n    /* a comment of \\\n     * two lines */ \\\n    (1 + 2)\n',
                      '#define N (1 + /* a comment of\n     * two lines */ 2)\n'):
            with self.subTest(macro=macro):
                self.assertEqual(types(macro + 'struct a { char x[N]; };').layout('struct a').size, 3)

    def test_what_has_no_layout_fails_naming_why(self):
        found = types('struct bits { Uint32 flag : 1; };\nstruct d { double x; };\nstruct a { int x[]; };\n'
                      'typedef void FUNCTION(void);\n#define N (~0U >> 31)\nstruct s { char a[N]; };\n'
                      'struct n { char a[-1]; };')
        for ctype, reason in (('struct bits', 'flag is a bit-field'), ('struct d', 'double: no layout'),
                              ('struct a', r'int\[\]: no extent'), ('FUNCTION', 'a function'),
                              ('struct s', r'\[N\]: ~ 0U: -1 lies outside unsigned int'),
                              ('struct n', r'\[- 1\]: a negative extent'),
                              ('struct none', 'struct none: no definition'), ('NOTHING', 'NOTHING: no type'),
                              ('void', 'void: no layout')):
            with self.subTest(ctype=ctype), self.assertRaisesRegex(LayoutError, reason):
                found.layout(ctype)

    def test_a_tag_defined_two_ways_fails(self):
        found = types('struct a { Sint32 w; };', 'struct a { Uint8 b; };')
        with self.assertRaisesRegex(LayoutError, 'struct a: defined two ways'):
            found.layout('struct a')

    def test_a_typedef_defined_two_ways_fails(self):
        with self.assertRaisesRegex(LayoutError, '^T: defined two ways$'):
            types('typedef short T;', 'typedef long T;').layout('T')


class Constants(unittest.TestCase):
    def test_extents_take_macros_and_enumerators(self):
        found = types('#define ROWS 3\n#define COLS (ROWS * 2 + 1)\nenum { RED, GREEN = 4, BLUE, COUNT };\n'
                      'struct a { Uint8 grid[ROWS][COLS]; Sint16 colors[COUNT]; };')
        self.assertEqual(member(found.layout('struct a'), 'grid'), Array(Array(Scalar('B'), 7), 3))
        self.assertEqual(found.layout('struct a').size, 21 + 1 + 12)
        self.assertEqual(found.evaluate(['BLUE']), 5)

    def test_a_macro_or_an_enumerator_defined_two_ways_fails(self):
        found = types('#define N 1\nenum { E = 1 };', '#define N 2\nenum { E = 2 };')
        for name in ('N', 'E'):
            with self.subTest(name=name), self.assertRaisesRegex(LayoutError, f'^{name}: defined two ways$'):
                found.evaluate([name])

    def test_a_macro_expands_as_text(self):
        # as the preprocessor does, without parentheses of its own
        self.assertEqual(types('#define SUM 1 + 2').evaluate(['SUM', '*', '3']), 7)

    def test_c_arithmetic(self):
        # each as SHC R10 and R32 give it in an extent
        found = types()
        for text, value in (('-7 / 2', -3), ('-7 % 2', -1), ('7 % -2', 1), ('1 << 4 | 1', 17), ('~0 & 0xff', 255),
                            ('(2 + 3) * 4', 20), ('6 - 2 - 1', 3), ('1 < 2 && 2 < 3', 1), ('-16 >> 2', -4),
                            ('-3 >> 1', -2), ('-3 << 2', -12)):
            with self.subTest(text=text):
                self.assertEqual(found.evaluate(TOKEN.findall(text)), value)

    def test_unsigned_and_long_values_that_fit_their_types(self):
        # each as SHC R10 and R32 give it in an extent
        found = types()
        for text, value in (('12U + 40UL', 52), ('0x80000000U >> 28', 8), ('0xFFFFFFFF >> 28', 15),
                            ('3000000000 / 1000000000', 3), ('1 < 2U', 1), ('-1 && 1U', 1), ('5U - 3', 2),
                            ('-1L < 1U', 1), ('1 < 0xffffffffUL', 1), ('0x80000000L >> 31', 1), ('-1 >> 1U', -1),
                            ('(1 < 2U) - 2', -1), ('!0U - 2', -1)):
            with self.subTest(text=text):
                self.assertEqual(found.evaluate(TOKEN.findall(text)), value)

    def test_integer_constants_take_shcs_types(self):
        self.assertEqual([number(text) for text in ('0', '017', '0x1F', '12U', '40UL', '10L', '0x80000000',
                                                    '3000000000', '0x80000000L')],
                         [(0, 'int'), (15, 'int'), (31, 'int'), (12, 'unsigned int'), (40, 'unsigned long'),
                          (10, 'long'), (0x80000000, 'unsigned int'), (3000000000, 'unsigned int'),
                          (0x80000000, 'unsigned long')])
        for text, reason in (('1.5F', '1.5F: no integer constant'), ('10LL', '10LL: no integer constant'),
                             ('0x100000000', '0x100000000: wider than 32 bits')):
            with self.subTest(text=text), self.assertRaisesRegex(LayoutError, reason):
                number(text)

    def test_what_is_no_constant_fails(self):
        found = types('#define HALF(x) ((x) / 2)\n#define LOOP LOOP\n')
        for tokens, reason in ((['HALF', '(', '4', ')'], 'HALF: a function-like macro'), (['LOOP'], 'LOOP: no'),
                               (['1', '/', '0'], '1 / 0: a division by zero'), (['sizeof', 'x'], 'sizeof')):
            with self.subTest(tokens=tokens), self.assertRaisesRegex(LayoutError, reason):
                found.evaluate(tokens)

    def test_a_value_shc_gives_otherwise_fails_naming_its_operation(self):
        # SHC R10 and R32 give each one a value unbounded integers don't, as
        # they wrap, convert or shift
        found = types('enum { TOP = 0x7fffffff, OVER };')
        for text, reason in (('~0U >> 31', '~ 0U: -1 lies outside unsigned int'),
                             ('(~0U >> 31) + 1', '~ 0U: -1 lies outside unsigned int'),
                             ('0xffffffffU + 1U', '0xffffffffU + 1U: 4294967296 lies outside unsigned int'),
                             ('-1 < 1U', '- 1 < 1U: -1 lies outside unsigned int'),
                             ('1L < 0xffffffff', '1L < 0xffffffff: 4294967295 lies outside long'),
                             ('0x7fffffff + 1', '0x7fffffff + 1: 2147483648 lies outside int'),
                             ('1 << 31', '1 << 31: 2147483648 lies outside int'),
                             ('-2147483648', '- 2147483648: -2147483648 lies outside unsigned int'),
                             ('OVER', 'OVER: 2147483648 lies outside int'),
                             ('1 << 32', '1 << 32: a shift by 32, outside 0 to 31'),
                             ('1 << -1', '1 << - 1: a shift by -1, outside 0 to 31')):
            with self.subTest(text=text), self.assertRaisesRegex(LayoutError, re.escape(reason)):
                found.evaluate(TOKEN.findall(text))

    def test_what_shc_gives_no_value_fails(self):
        # SHC R10 and R32 fail each, a division by zero even on the side of &&
        # or || that C skips
        found = types()
        for text, reason in (('0 && (1 / 0)', '1 / 0: a division by zero'),
                             ('1 || (1 % 0)', '1 % 0: a division by zero'),
                             ('(-2147483647 - 1) / -1', '( - 2147483647 - 1 ) / - 1: 2147483648 lies outside int'),
                             ('(-2147483647 - 1) % -1',
                              '( - 2147483647 - 1 ) % - 1: its quotient 2147483648 lies outside int'),
                             ('10LL', '10LL: no integer constant'),
                             ('0x100000000', '0x100000000: wider than 32 bits')):
            with self.subTest(text=text), self.assertRaisesRegex(LayoutError, re.escape(reason)):
                found.evaluate(TOKEN.findall(text))

    def test_both_sides_of_and_are_evaluated(self):
        # SHC R10 and R32 give this 0, skipping its shift, which fails here
        with self.assertRaisesRegex(LayoutError, re.escape('1 << 32: a shift by 32, outside 0 to 31')):
            types().evaluate(TOKEN.findall('0 && (1 << 32)'))

    def test_an_enum_of_thousands_counts_on(self):
        # past Python's recursion limit
        found = types('enum many { ' + ', '.join(f'E{index}' for index in range(5000)) + ' };')
        self.assertEqual(found.evaluate(['E4999']), 4999)


BASE = 0x0C000000
# the sdk's scalar types, Ninja's types and ninja.h, which Types reads to lay
# out a body's type
SDK = [(Path(__file__).resolve().parents[1] / 'sdk/include' / name).read_text()
       for name in ('sg_xpt.h', 'njtypes.h', 'ninja.h')]


def words(*values):
    return struct.pack(f'<{len(values)}I', *values)


def image(payload):
    # the emitter's input: payload at BASE, no code, and main RAM's window
    # for an address
    return Memory('TEST.BIN', payload, BASE, (), (data.window(BASE),))


class Floats(unittest.TestCase):
    def test_unsupported_floats_raise_emit_errors_naming_them(self):
        # infinities, nans, subnormals and FLT_MAX, which only a decimal above
        # it spells in 9 digits
        for word in (0x7F800000, 0xFF800000, 0x7FC00000, 0x00000001, 0x807FFFFF, 0x7F7FFFFF, 0xFF7FFFFF):
            self.assertIsNone(spell(word))
            with self.assertRaisesRegex(EmitError, f'^TEST.BIN: 0x0c000010: 0x{word:08x} is no float'):
                Writer(image(bytes(16) + words(word)), [], {}).float(BASE + 0x10)

    def test_signed_zero_and_exact_bits(self):
        self.assertEqual(spell(0x00000000), '0.0F')
        self.assertEqual(spell(0x80000000), '-0.0F')
        self.assertEqual(spell(0x3F800000), '1.0F')
        self.assertEqual(spell(0xC0000000), '-2.0F')
        # R32 truncates a float literal toward zero, so each decimal from the
        # word's value up to the next word's reads back as the word
        for word in (0x00800000, 0x3DCCCCCD, 0x3EAAAAAB, 0x4479C000, 0x7F7FFFFE):
            text = spell(word)
            self.assertTrue(Decimal(f32(word)) <= Decimal(text[:-1]) < Decimal(f32(word + 1)), text)
            self.assertEqual(spell(word | 0x80000000), '-' + text)

    def test_writers_name_where_an_unsupported_float_lies(self):
        # a model whose radius is infinite
        model = image(bytes(36) + words(0x7F800000))
        with self.assertRaisesRegex(EmitError, '0x0c000024: 0x7f800000'):
            Writer(model, [Item(BASE, BASE + 40, 'model')], {}).source('a model')


class Truncation(unittest.TestCase):
    def test_an_item_past_the_image_fails_rather_than_coming_up_short(self):
        short = image(words(1, 2, 3, 4))
        for kind in ('words', 'bytes', 'points'):
            with self.subTest(kind=kind), self.assertRaisesRegex(
                    EmitError, '^TEST.BIN: 0x0c000000: 24 bytes to 0x0c000018 lie outside the image$'):
                Writer(short, [Item(BASE, BASE + 24, kind)], {}).source('short')

    def test_a_record_the_images_end_cuts_short_fails_at_the_member_past_it(self):
        # an item of each kind, a word long and all of the image, short of its
        # record: the first member it reads past the end, and that member's size
        short = image(words(0))
        for item, at, size in ((Item(BASE, BASE + 4, 'object'), 0x14, 4), (Item(BASE, BASE + 4, 'motion'), 4, 4),
                               (Item(BASE, BASE + 4, 'texlist'), 4, 4), (Item(BASE, BASE + 4, 'mkey_a'), 4, 4),
                               (Item(BASE, BASE + 4, 'texanim'), 4, 2), (Item(BASE, BASE + 4, 'mdata', 1), 4, 4)):
            with self.subTest(kind=item.kind), self.assertRaisesRegex(
                    EmitError, f'^TEST.BIN: 0x{BASE + at:08x}: {size} bytes to 0x{BASE + at + size:08x} lie outside '
                               'the image$'):
                Writer(short, [item], {}).source('short')

    def test_an_item_in_the_image_is_whole(self):
        whole = image(words(1, 2, 3, 4, 5, 6))
        self.assertIn('0x00000001, 0x00000002, 0x00000003, 0x00000004, 0x00000005, 0x00000006',
                      Writer(whole, [Item(BASE, BASE + 24, 'words')], {}).source('whole'))


class Pointers(unittest.TestCase):
    def test_a_pointer_into_padding_names_where_it_lies(self):
        # a word, a word no definition holds, a pointer to it, and a word
        padded = image(words(1, 0, BASE + 4, 2))
        items = [Item(BASE, BASE + 4, 'words'), Item(BASE + 8, BASE + 12, 'pointers'),
                 Item(BASE + 12, BASE + 16, 'words')]
        with self.assertRaisesRegex(EmitError, '^TEST.BIN: 0x0c000008: a pointer to 0x0c000004, in the padding '
                                               'between definitions$'):
            Writer(padded, items, {}).source('padded')


# each type a body takes from declarations, as a source defines it: the type,
# the definitions it needs past ninja.h, one record's bytes, and its
# initializer. 0.1F and 1.2F are SHC's bits, rounded toward zero.
LAID = [
    ('NJS_SPRITE', '', struct.pack('<3f2Ii2I', 1, -2, 0.5, 0x3DCCCCCC, 0x3DCCCCCC, 0x4000, 0, 0),
     '{ { 1.0F, -2.0F, 0.5F }, 0.1F, 0.1F, 16384, 0, 0 }'),
    ('NJS_POINT2', '', struct.pack('<2f', 320, -0.5), '{ 320.0F, -0.5F }'),
    ('NJS_POINT2COL', '', struct.pack('<4I', 0, 0, 0, 4), '{ 0, 0, 0, 0x00000004 }'),
    ('NJS_POINT3COL', '', struct.pack('<4I', 0, 0, 0, 2), '{ 0, 0, 0, 0x00000002 }'),
    ('NJS_TEX', '', struct.pack('<2h', 765, -510), '{ 765, -510 }'),
    ('NJS_BGRA', '', bytes([0xff, 0, 0x80, 0x40]), '{ 0xff, 0x00, 0x80, 0x40 }'),
    ('NJS_FOG_TABLE', '', struct.pack('<128f', *[1] * 127, 0.5), '{ ' + '1.0F, ' * 127 + '0.5F }'),
    ('int', '', struct.pack('<i', -3), '-3'),
    ('Angle', '', struct.pack('<i', 0x4000), '16384'),
    ('struct back_color', 'struct back_color { Uint32 col1; Uint32 col2; Uint32 col3; };',
     struct.pack('<3I', 0xff000000, 0xff808080, 0), '{ 0xff000000, 0xff808080, 0x00000000 }'),
    ('struct beam_texture', 'struct beam_texture { Uint32 texture; Uint32 color[2]; NJS_POINT2 uv[4]; };',
     struct.pack('<3I8f', 5, 0xffffffff, 0x00ffffff, 0, 1, 1, 1, 0, 0.25, 1, 0.25),
     '{ 0x00000005, { 0xffffffff, 0x00ffffff }, { { 0.0F, 1.0F }, { 1.0F, 1.0F }, { 0.0F, 0.25F }, '
     '{ 1.0F, 0.25F } } }'),
    ('struct cycle_entry', 'struct cycle_entry { NJS_OBJECT *object; Uint8 count; Uint8 materials[3]; Uint8 first; '
     'Uint8 unk_09; Sint16 delay; };', struct.pack('<I6Bh', 0, 2, 0, 1, 0, 30, 0, 8),
     '{ 0, 0x02, { 0x00, 0x01, 0x00 }, 0x1e, 0x00, 8 }'),
    ('struct cycle_kind', 'struct cycle_kind { NJS_MATERIAL *material; const Sint16 *textures; Sint16 count; '
     'Sint16 frames; };', struct.pack('<2I2h', 0, 0, 4, 6), '{ 0, 0, 4, 6 }'),
    ('struct frog_key', 'struct frog_key { Sint8 frames; Sint8 shape; };', struct.pack('<2b', 20, -1), '{ 20, -1 }'),
    ('struct lamps_kind', '#define LAMPS 4\nstruct lamp_row;\nstruct lamps_kind { const struct lamp_row *rows; '
     'Sint16 count; const Uint16 (*textures)[LAMPS]; const Uint8 *patterns; Sint16 pattern_count; Sint16 frames; };',
     struct.pack('<Ih2x2I2h', 0, 3, 0, 0, 71, 10), '{ 0, 3, 0, 0, 71, 10 }'),
    ('struct land_anim', 'struct land_anim_part;\nstruct land_anim { struct land_anim_part *parts; Uint8 count; '
     'Uint8 frames; Uint16 first; Uint16 rest; Uint16 delay; };', struct.pack('<I2B3H', 0, 3, 8, 40, 41, 4),
     '{ 0, 0x03, 0x08, 0x0028, 0x0029, 0x0004 }'),
    ('struct light_spec', 'struct light_spec { Sint32 type; Float specular, diffuse, ambient; NJS_ARGB color; '
     'NJS_POINT3 pos; NJS_VECTOR direction; Float near, far; Angle inner, outer; };',
     struct.pack('<i15f2i', 1, 0, 0, 0.25, 1, 0.5, 1, 0.75, 0, 0, 0, 0, -1, 0, 1, 65535, 1820, 5461),
     '{ 1, 0.0F, 0.0F, 0.25F, { 1.0F, 0.5F, 1.0F, 0.75F }, { 0.0F, 0.0F, 0.0F }, { 0.0F, -1.0F, 0.0F }, 1.0F, '
     '65535.0F, 1820, 5461 }'),
    ('struct lockon_strip', 'struct lockon_strip { NJS_POINT3 *points; NJS_COLOR *colors; Sint16 *indices; '
     'Sint16 count; NJS_POINT3COL lines; };', struct.pack('<3Ih2x4I', 0, 0, 0, 12, 0, 0, 0, 0),
     '{ 0, 0, 0, 12, { 0, 0, 0, 0x00000000 } }'),
    ('struct marker', 'struct marker { NJS_POINT3 *points; int count; Uint32 color; };',
     struct.pack('<IiI', 0, 2, 0xffff0000), '{ 0, 2, 0xffff0000 }'),
    ('struct menu_scale', 'struct menu_scale { Float x, y; };', struct.pack('<2f', 0.5, 1.25), '{ 0.5F, 1.25F }'),
    ('struct night_light', 'struct night_light { NJS_OBJECT *object; Uint32 texture; };', struct.pack('<2I', 0, 12),
     '{ 0, 0x0000000c }'),
    ('struct njb_action', 'struct njb_action { Uint32 object; Uint32 motion; };',
     struct.pack('<2I', 0x0c92e400, 0x0c92f000), '{ 0x0c92e400, 0x0c92f000 }'),
    ('struct ruin_water_part', 'struct ruin_water_part { NJS_OBJECT *object; Uint8 count; Uint8 materials[3]; '
     'Uint8 first; Sint16 delay; };', struct.pack('<I5Bxh', 0, 1, 2, 0, 0, 9, 6),
     '{ 0, 0x01, { 0x02, 0x00, 0x00 }, 0x09, 6 }'),
    ('struct signboard_kind', 'struct signboard_kind { const Sint16 *textures; Sint16 count; Sint16 delay; };',
     struct.pack('<I2h', 0, 3, 30), '{ 0, 3, 30 }'),
    ('struct sprite_frame', 'struct sprite_frame { Sint16 texture; Uint8 flags; Uint8 unk_03; Uint32 index; '
     'Float width; Float height; Float center_x; Float center_y; Float age; NJS_ARGB color; };',
     struct.pack('<h2BI2I7f', -2, 6, 0, 0xffffffff, 0x3F999999, 0x3F999999, 0, 0, 4, 1, 1, 0.5, 0.5),
     '{ -2, 0x06, 0x00, 0xffffffff, 1.2F, 1.2F, 0.0F, 0.0F, 4.0F, { 1.0F, 1.0F, 0.5F, 0.5F } }'),
    ('struct sprite_look', 'struct sprite_look { Float sx, sy; Angle ang; NJS_TEXLIST *tlist; NJS_TEXANIM *tanim; };',
     struct.pack('<2fi2I', 0.5, 0.5, -0x8000, 0, 0), '{ 0.5F, 0.5F, -32768, 0, 0 }'),
    ('struct stream_kind', 'struct stream_kind { NJS_OBJECT *object; Sint16 step; };', struct.pack('<Ih2x', 0, -6),
     '{ 0, -6 }'),
    ('struct texture_anim', 'struct texture_swap;\nstruct texture_anim { int period; int count; '
     'struct texture_swap *rows; Sint8 *frames; Sint8 frame; };', struct.pack('<2i2Ib3x', 4, 7, 0, 0, 0),
     '{ 4, 7, 0, 0, 0 }'),
    ('struct texture_cycle', 'struct texture_cycle { NJS_OBJECT *object; Uint8 delay; Uint8 count; '
     'Uint8 materials[2]; Uint16 first; Uint16 frames; Uint16 timer; Uint16 frame; };',
     struct.pack('<I4B4H', 0, 6, 2, 0, 1, 100, 4, 0, 0),
     '{ 0, 0x06, 0x02, { 0x00, 0x01 }, 0x0064, 0x0004, 0x0000, 0x0000 }'),
    ('struct texture_frame', 'struct texture_frame { Uint32 texture; Uint32 index; };', struct.pack('<2I', 17, 0),
     '{ 0x00000011, 0x00000000 }'),
    ('struct texture_swap', 'struct texture_swap { NJS_MATERIAL *material; Uint32 texture; Uint32 own; };',
     struct.pack('<3I', 0, 57, 0), '{ 0, 0x00000039, 0x00000000 }'),
    ('struct time_texture', 'struct time_texture { NJS_OBJECT *object; int material; Uint32 textures[3]; };',
     struct.pack('<Ii3I', 0, 1, 10, 11, 12), '{ 0, 1, { 0x0000000a, 0x0000000b, 0x0000000c } }'),
    ('struct uv_scroll', 'struct uv_scroll { NJS_MODEL *model; int count; Bool backward; };',
     struct.pack('<I2i', 0, 24, 1), '{ 0, 24, 1 }'),
    ('struct uv_scroll', 'struct uv_group;\nstruct uv_scroll { struct uv_group *groups; Sint16 count; Sint16 flags; '
     'Sint16 speed; Sint16 delay; Sint16 timer; Sint16 phase; };', struct.pack('<I6h', 0, 2, 1, -4, 3, 0, 0),
     '{ 0, 2, 1, -4, 3, 0, 0 }'),
    ('struct uv_scroll', 'struct uv_scroll { NJS_MESHSET *meshset; Sint16 count; Sint16 step_u; Sint16 step_v; '
     'Sint16 unk_0a; NJS_TEX *uvs; };', struct.pack('<I4hI', 0, 20, 0, 64, 0, 0), '{ 0, 20, 0, 64, 0, 0 }'),
    ('struct uv_shift', 'struct uv_shift { NJS_MESHSET *meshset; Sint16 count; Sint16 shift_u; Sint16 shift_v; '
     'Sint16 users; NJS_TEX *shifted; NJS_TEX *own; };', struct.pack('<I4h2I', 0, 36, 128, 0, 0, 0, 0),
     '{ 0, 36, 128, 0, 0, 0, 0 }'),
    ('struct vmu_frame', 'struct vmu_frame { const Uint8 *bitmap; Uint32 updates; };', struct.pack('<2I', 0, 2),
     '{ 0, 0x00000002 }'),
]


class Laid(unittest.TestCase):
    def test_each_type_a_body_takes_from_its_declarations(self):
        for ctype, definitions, record, initializer in LAID:
            with self.subTest(ctype=ctype, definitions=definitions):
                layout = Types([*SDK, definitions]).layout(ctype)
                self.assertEqual(layout.size, len(record))
                self.assertEqual(Writer(image(record), [], {}).initializer(layout, BASE), initializer)

    def test_a_union_is_its_first_member(self):
        layout = Types(SDK).layout('NJS_COLOR')
        self.assertEqual(Writer(image(words(0x80ff0000)), [], {}).initializer(layout, BASE), '0x80ff0000')

    def test_a_pad_byte_that_isnt_zero_fails(self):
        # a struct's padding, and a union's bytes past its first member
        for definitions, ctype, record, where in (('struct a { NJS_OBJECT *object; Sint16 step; };', 'struct a',
                                                   struct.pack('<Ihbb', 0, -6, 0, 1), '0x0c000007'),
                                                  ('union u { Uint16 h; Uint32 w; };', 'union u',
                                                   bytes([0, 0, 0, 1]), '0x0c000003')):
            layout = Types([*SDK, definitions]).layout(ctype)
            with self.subTest(ctype=ctype), self.assertRaisesRegex(
                    EmitError, f'{where}: a pad byte of 0x01, which no initializer writes'):
                Writer(image(record), [], {}).initializer(layout, BASE)


class Structures(unittest.TestCase):
    def test_the_table_lays_out_the_sdks_types_as_types_does(self):
        # each kind's type and each mdata's: its size and alignment, and where
        # the table names members, the offset of each of the type's, in
        # order. A union's are one member's own, as a color's u and v are
        found = Types(SDK)
        layouts = {kind.ctype: kind.layout for kind in data.KINDS.values()}
        layouts.update({f'NJS_MDATA{factors}': layout for factors, layout in data.MDATA.items()})
        for ctype, layout in layouts.items():
            with self.subTest(ctype=ctype):
                laid = found.layout(ctype)
                self.assertEqual((layout.size, layout.alignment), (laid.size, laid.alignment))
                named = [offset for name, offset in vars(layout).items() if name not in ('size', 'alignment')]
                if not named:
                    continue
                if laid.union:
                    records = [field for _, field, _ in laid.fields if isinstance(field, Record)]
                    self.assertIn(named, [[at for _, at in offsets(record)] for record in records])
                else:
                    self.assertEqual(named, [at for _, at in offsets(laid)])


class Reads(unittest.TestCase):
    def test_a_negative_size_is_rejected(self):
        loaded = Data('TEST.BIN', bytes(16), BASE)
        with self.assertRaisesRegex(ReadError, 'negative'):
            loaded.inside(BASE + 8, -4)
        with self.assertRaisesRegex(ReadError, 'negative'):
            loaded.need(BASE + 8, -4, 4, 'an array')

    def test_a_read_outside_the_image_is_rejected(self):
        # past its end, and before its start, which a slice would take from the end
        loaded = Data('TEST.BIN', words(1, 2), BASE)
        for read, address, what in ((loaded.u32, BASE + 8, 'a Uint32'), (loaded.u16, BASE + 7, 'a Uint16'),
                                    (loaded.s32, BASE - 4, 'an Sint32'), (loaded.s16, BASE - 2, 'an Sint16')):
            with self.subTest(what=what), self.assertRaisesRegex(
                    ReadError, f"^TEST.BIN: 0x{address:08x}: can't be {what}, whose"):
                read(address)
        with self.assertRaisesRegex(ReadError, "^TEST.BIN: 0x0c000004: can't be 2 words, whose 8 bytes lie outside"):
            loaded.words(BASE + 4, 2)

    def test_inside_is_the_image_and_no_more(self):
        loaded = Data('TEST.BIN', bytes(16), BASE)
        self.assertTrue(loaded.inside(BASE, 16))
        self.assertTrue(loaded.inside(BASE + 16, 0))
        self.assertFalse(loaded.inside(BASE, 17))
        self.assertFalse(loaded.inside(BASE - 1, 1))

    def test_a_misaligned_record_is_rejected(self):
        loaded = Data('TEST.BIN', bytes(64), BASE)
        with self.assertRaisesRegex(ReadError, "^TEST.BIN: 0x0c000002: can't be an object, not aligned to 4$"):
            object_at(loaded, BASE + 2)

    def test_a_record_past_the_image_is_rejected(self):
        loaded = Data('TEST.BIN', bytes(48), BASE)
        with self.assertRaisesRegex(ReadError,
                                    "^TEST.BIN: 0x0c000010: can't be a model, whose 40 bytes lie outside the image$"):
            model_at(loaded, BASE + 16)
        with self.assertRaisesRegex(ReadError, 'outside the image'):
            texlist_at(loaded, BASE - 8)

    def test_an_mdata_entry_past_the_image_is_rejected(self):
        # a motion of positions and angles, whose entries are 16 bytes
        loaded = Data('TEST.BIN', words(BASE + 12, 10, 0x3) + words(0, 0, 0, 0), BASE)
        motion = motion_at(loaded, BASE)
        self.assertEqual(mdata_keys(loaded, motion, BASE + 12), [(0x1, 0, 0), (0x2, 0, 0)])
        with self.assertRaisesRegex(ReadError, 'an mdata entry'):
            mdata_keys(loaded, motion, BASE + 16)


class Recognizer(unittest.TestCase):
    def test_an_actions_tree_counts_its_motions_mdata_past_a_ref(self):
        # two angle keys at 0x00; three mdata entries from 0x20, each naming
        # them; the motion at 0x38; a tree of three objects, each the last's
        # child, from 0x44; the action at 0xe0; then a word that points at it,
        # and one that holds the third entry's address, as a glyph might
        one = struct.unpack('<I', struct.pack('<f', 1.0))[0]

        def node(child):
            return words(0, 0, 0, 0, 0, 0, 0, 0, one, one, one, child, 0)

        payload = (words(0, 0, 0, 0, 1, 0, 0, 0) + words(BASE, 2) * 3 + words(BASE + 0x20, 2) + struct.pack('<HH', 2, 1)
                   + node(BASE + 0x78) + node(BASE + 0xAC) + node(0) + words(BASE + 0x44, BASE + 0x38)
                   + words(BASE + 0xE0, BASE + 0x30))
        found = data.Recognizer(data.Image('TEST.BIN', payload, BASE, (), (data.window(BASE),)), BASE,
                                BASE + len(payload))
        found.recognize()
        mdata = found.items[BASE + 0x20]
        self.assertEqual((mdata.kind, mdata.end), ('mdata', BASE + 0x38))


class CommandLine(unittest.TestCase):
    def test_each_command_line_runs_its_function_on_its_operands(self):
        for line, name in (('IMAGE SOURCE OUT', 'generate'), ('--declared CONFIG OUT', 'declarations'),
                           ('--rows IMAGE', 'rows'), ('--bodies IMAGE SOURCE LISTING OUT CONFIG', 'bodies'),
                           ('--texts SOURCE OUT', 'text_stubs'), ('--values SOURCE OUT', 'value_stubs')):
            with self.subTest(line=line), mock.patch.object(data, name) as function:
                data.main(line.split())
                function.assert_called_once_with(*(word for word in line.split() if not word.startswith('--')))

    def test_an_error_a_section_raises_fails_naming_it(self):
        image = Memory('TEST.BIN', b'', BASE)
        for error in (LayoutError('struct a: no layout'), EmitError(image, BASE, 'a float no literal gives back'),
                      ReadError(image, BASE, "can't be a model")):
            with self.subTest(error=error), mock.patch.object(data, 'rows', side_effect=error), \
                    self.assertRaisesRegex(SystemExit, f': {re.escape(str(error))}$'):
                data.main(['--rows', 'IMAGE'])

    def test_a_file_it_cant_read_fails_naming_it(self):
        # an image make hasn't decoded, a source, a stub listing and a header the dependency file names, each
        # missing from a tree that holds the rest
        stub_bodies = ['--bodies', 'TEST.BIN', 'src/a.c', 'build/stub/src/a.lis', 'out', 'test']
        for missing, args, files in (
                ('TEST.BIN', ['--rows', 'TEST.BIN'], GENERATE),
                ('src/a.c', ['--values', 'src/a.c', 'values.inc'], {}),
                ('build/stub/src/a.lis', stub_bodies, BODIES),
                ('src/a.h', stub_bodies, BODIES)):
            with self.subTest(missing=missing), tree({name: text for name, text in files.items() if name != missing}), \
                    self.assertRaisesRegex(SystemExit, f'^[^\\n]*: {re.escape(missing)}: No such file or directory$'):
                data.main(args)


class Texts(unittest.TestCase):
    def test_the_stub_takes_a_placeholder_for_each(self):
        with tree({'a.c': '#define TEXT_1_LENGTH 3\n'}):
            data.main(('--texts', 'a.c', 'text.inc'))
            self.assertEqual(Path('text.inc').read_text(), '#define TEXT_1 "\\x01\\0\\0"\n')

    def test_a_text_past_255_takes_the_least_byte_its_length_leaves(self):
        lengths = [1] * 254 + [2, 1, 2]
        source = ''.join(f'#define TEXT_{number}_LENGTH {length}\n' for number, length in enumerate(lengths, 1))
        with tree({'a.c': source}):
            placed = data.texts(Path('a.c'))
        self.assertEqual(placed[254][3], b'\xff\0')
        self.assertEqual([placeholder for *_, placeholder in placed[255:]], [b'\xff', b'\x01\0'])

    def test_a_comment_holds_no_text(self):
        with tree({'a.c': '#define TEXT_1_LENGTH 3\n/*\n#define TEXT_2_LENGTH 4\n*/\n'}):
            self.assertEqual([name for name, *_ in data.texts(Path('a.c'))], ['TEXT_1'])

    def test_more_than_255_of_one_length_fail(self):
        source = ''.join(f'#define TEXT_{number}_LENGTH 1\n' for number in range(1, 257))
        with tree({'a.c': source}), self.assertRaisesRegex(SystemExit, 'more than 255 texts of length 1'):
            data.texts(Path('a.c'))


class Publish(unittest.TestCase):
    def test_a_failed_write_leaves_the_old_file_whole(self):
        with tree({'a.c': 'old'}):
            # a lone surrogate, which write_text can't encode
            with self.assertRaises(UnicodeEncodeError):
                data.publish(Path('a.c'), '\udc80')
            self.assertEqual(Path('a.c').read_text(), 'old')
            self.assertEqual(sorted(path.name for path in Path().iterdir()), ['a.c'])

    def test_a_path_it_cant_write_fails_naming_it(self):
        with tree({'out/a': ''}):
            with self.assertRaisesRegex(SystemExit, r': out: Is a directory$'):
                data.publish(Path('out'), 'text')
            self.assertEqual(sorted(path.name for path in Path().iterdir()), ['out'])


# a gen/ range of a table that points past it, at a name and at an address with none
GENERATE = {
    'config/1st_read/symbols.txt': '',
    'config/test/splits.txt': 'D  0x0c000000  0x0c000100\ngen/test/d_0c000000.c  D  0x0c000000  0x0c000040\n',
    'config/test/symbols.txt': '0x0c000080  later\n',
    'TEST.BIN': words(BASE + 0x80, BASE + 0x90, 2, 3) + bytes(0xF0),
    'build/gen/test/declared.txt': '',
}
GENERATED = Path('build/gen/test/d_0c000000.c')


def generate():
    data.generate('TEST.BIN', 'gen/test/d_0c000000.c', str(GENERATED))


class Generate(unittest.TestCase):
    def test_writes_the_c_and_its_placeholders(self):
        with tree(GENERATE):
            generate()
            self.assertIn('later', GENERATED.read_text())
            self.assertEqual(GENERATED.with_suffix('.syms').read_text(), '0x0c000090  dat_0c000090\n')
            self.assertEqual(sorted(path.name for path in GENERATED.parent.iterdir()),
                             ['d_0c000000.c', 'd_0c000000.syms', 'declared.txt'])

    def test_a_range_past_the_image_fails(self):
        with tree(dict(GENERATE, **{'TEST.BIN': bytes(0x20)})), self.assertRaisesRegex(
                SystemExit, r'gen/test/d_0c000000\.c: 0x0c000000 to 0x0c000040 lies outside TEST\.BIN'):
            generate()

    def test_a_name_inside_a_string_fails_naming_the_source(self):
        string = data.Item(BASE + 0x100, BASE + 0x110, 'string', text=b'abc')
        with self.assertRaisesRegex(SystemExit, r'gen/test/d_0c000000\.c: mid, at 0x0c000104, falls inside char '
                                                r'0x0c000100'):
            data.cut([string], {BASE + 0x104: 'mid'}, 'gen/test/d_0c000000.c')

    def test_declared_txt_is_required(self):
        files = {path: text for path, text in GENERATE.items() if path != 'build/gen/test/declared.txt'}
        with tree(files), self.assertRaisesRegex(SystemExit, r'declared\.txt: missing'):
            generate()

    def test_the_c_comes_last(self):
        with tree(GENERATE), mock.patch('data.publish', wraps=data.publish) as publish:
            generate()
        self.assertEqual([call.args[0].name for call in publish.call_args_list], ['d_0c000000.syms', 'd_0c000000.c'])

    def test_a_row_in_d_names_no_section(self):
        with tree(GENERATE):
            generate()
            self.assertNotIn('#pragma section', GENERATED.read_text())

    def test_a_row_in_dhead_writes_its_data_under_head(self):
        splits = GENERATE['config/test/splits.txt'].replace('D ', 'DHEAD ')
        with tree(dict(GENERATE, **{'config/test/splits.txt': splits})):
            generate()
            text = GENERATED.read_text()
        before, pragma, after = text.partition('\n#pragma section HEAD\n')
        self.assertTrue(pragma)
        self.assertIn('extern', before)
        self.assertIn('later', after)
        self.assertTrue(after.endswith('\n#pragma section\n'))

    def test_a_row_in_d_defines_nothing_const(self):
        with tree(GENERATE):
            generate()
            self.assertNotIn('const', GENERATED.read_text())

    def test_a_row_in_constants_defines_its_data_const(self):
        splits = GENERATE['config/test/splits.txt'].replace('D ', 'C ')
        with tree(dict(GENERATE, **{'config/test/splits.txt': splits})):
            generate()
            text = GENERATED.read_text()
        self.assertNotIn('#pragma section', text)
        self.assertIn('void *const dat_0c000000[] = {', text)
        self.assertNotIn('extern void *const', text)

    def test_a_string_in_constants_is_const(self):
        files = dict(GENERATE, **{
            'config/test/splits.txt': 'C  0x0c000000  0x0c000100\ngen/test/d_0c000000.c  C  0x0c000000  0x0c000008\n',
            'TEST.BIN': b'abcdef\0\0' + words(BASE) + bytes(0xF4)})
        with tree(files):
            generate()
            self.assertIn('char const dat_0c000000[8] = "abcdef";', GENERATED.read_text())

    def test_a_row_that_ends_at_its_last_null_ends_with_a_string(self):
        # its last string is a char array, not words and a byte
        files = dict(GENERATE, **{
            'config/test/splits.txt': 'C  0x0c000000  0x0c000100\ngen/test/d_0c000000.c  C  0x0c000000  0x0c000007\n',
            'TEST.BIN': b'abcdef\0\0' + words(BASE) + bytes(0xF4)})
        with tree(files):
            generate()
            text = GENERATED.read_text()
        self.assertIn('char const dat_0c000000[7] = "abcdef";', text)
        self.assertNotIn('Uint8', text)

    def test_a_row_in_bss_fails(self):
        splits = GENERATE['config/test/splits.txt'].replace('D ', 'B ')
        with tree(dict(GENERATE, **{'config/test/splits.txt': splits})), self.assertRaisesRegex(
                SystemExit, r'gen/test/d_0c000000\.c: its row lies in B, and the C it takes defines data for D, DHEAD or C'):
            generate()


def label(offset, name):
    return f'D   {offset:08X}              L{offset + 1}:{" " * 28}; static: {name}'


def directive(offset, operands, op='.DATA.L'):
    return f'    {offset:08X} {"":10} {op:12}{operands}'


def stub(table=False, text=False, headers=('src/a.h',)):
    # a source of bodies, and its stub build's listing and dependencies
    lines = ['#include "a.h"', 'NJS_TEXLIST texlist = {', '#include "a/texlist.inc"', '};']
    if table:
        lines += ['Uint32 table[2] = {', '#include "a/table.inc"', '};']
    if text:
        lines += ['#define TEXT_1_LENGTH 4', '#include "a/text.inc"']
    listing = [label(0, 'texlist'), directive(0, "H'00000000,H'00000000"), label(8, 'table'),
               directive(8, "H'00000000,H'00000000")]
    if text:
        listing += ['D   00000010              L20:', directive(0x10, "H'01", '.DATA.B'), directive(0x11, '4,0', '.DATAB.B')]
    listing += ['DATA     SECTION (D):                               00000020 Byte(s)']
    return {
        'src/a.c': '\n'.join(lines) + '\n',
        'build/stub/src/a.lis': '\n'.join(listing) + '\n',
        'build/stub/src/a.d': f'build/stub/src/a.obj: src/a.c {" ".join(headers)} \\\n'
                              ' build/stub/src/a/texlist.inc\n\nsrc/a.h:\n',
    }


# a texture list of three names elsewhere, two words, and the text abcd
BODIES = {
    'config/1st_read/symbols.txt': '',
    'config/test/splits.txt': 'D  0x0c000000  0x0c000100\nsrc/a.c  D  0x0c000000  0x0c000020\n',
    'config/test/symbols.txt': '0x0c000080  names\n',
    'TEST.BIN': words(BASE + 0x80, 3, 7, 8) + b'abcd\0' + bytes(0xEB),
    'src/a.h': 'extern NJS_TEXNAME names[];\n',
    # a header the source doesn't include, which declares names another way
    'src/0.h': 'extern NJS_MODEL names;\n',
    **stub(table=True),
}
# the group a former run wrote, with a body the source has since dropped
FORMER = {'build/gen/src/a/texlist.inc': 'old\n', 'build/gen/src/a/dropped.inc': 'old\n',
          'build/gen/src/a/bodies.txt': 'old\n'}
GROUP = Path('build/gen/src/a')


def bodies():
    data.bodies('TEST.BIN', 'src/a.c', 'build/stub/src/a.lis', str(GROUP), 'test')


def group():
    return {path.name: path.read_text() for path in GROUP.iterdir()}


def assembled_listing(rows):
    # asmsh's listing of a D section of rows: (offset, text, code), or a
    # continuation's (offset, None, code)
    lines = ['\f*** SuperH RISC engine ASSEMBLER Ver. 4.0 ***    10/20/26 16:26:38', 'PROGRAM NAME =', '',
             f'    1 00000000 {"":16}     1               .SECTION    D,DATA,ALIGN=4']
    for number, (offset, text, *code) in enumerate(rows, 2):
        code = code[0] if code else ''
        if text is None:
            lines.append(f'      {offset:08X} {code}')
        elif text.startswith('L'):
            lines.append(f'{number:5} {offset:08X} {code:16} {number:5}     {text}')
        else:
            lines.append(f'{number:5} {offset:08X} {code:16} {number:5}               {text}')
    lines += ['*** SECTION DATA LIST', '', 'SECTION                          ATTRIBUTE    SIZE             START', '',
              'D                                REL-DATA    000000020        ']
    return '\n'.join(lines) + '\n'


def automatic(count):
    # BODIES, and a function whose array of count words takes its template
    # from the 8 bytes at 0x18, past a word it loads too
    files = dict(BODIES)
    files['src/a.c'] += f'static void f(void)\n{{\n    Uint32 pair[{count}] = {{\n#include "a/pair.inc"\n    }};\n}}\n'
    files['TEST.BIN'] = words(BASE + 0x80, 3, 7, 8, 0, 0, 0x11, 0x22) + bytes(0xE0)
    code = ['P   00000000              L1:                              ; function: f',
            f'    00000000 {"D201":10} {"MOV.L":12}L2,R2; L40', f'    00000002 {"D302":10} {"MOV.L":12}L2+4,R3; L41']
    templates = ['D   00000010              L41:', directive(0x10, "H'00000000"),
                 'D   00000018              L40:', directive(0x18, "H'00000000,H'00000000")]
    lines = files['build/stub/src/a.lis'].splitlines()
    files['build/stub/src/a.lis'] = '\n'.join(code + lines[:-1] + templates + lines[-1:]) + '\n'
    return files


class Bodies(unittest.TestCase):
    def test_replaces_the_former_group_with_each_body_and_bodies_txt(self):
        with tree(dict(BODIES, **FORMER)):
            bodies()
            self.assertEqual(group(), {
                'texlist.inc': '    names, 3,\n',
                'table.inc': '    0x00000007, 0x00000008,\n',
                'bodies.txt': 'texlist  0x0c000000  8  NJS_TEXLIST\ntable  0x0c000008  8  Uint32\n',
            })

    def test_takes_a_section_placed_in_rows_from_its_first_start_to_its_highest_end(self):
        # src/a.c's D in two rows, as link.py places it, from the first row's start to the highest end: the bodies
        # come out as from one row, texlist at the section's start and the text at its end
        for rows in (('0x0c000000  0x0c000010', '0x0c000010  0x0c000020'),
                     ('0x0c000000  0x0c000008', '0x0c000010  0x0c000020', '0x0c000008  0x0c000010')):
            with self.subTest(rows=rows):
                splits = 'D  0x0c000000  0x0c000100\n' + ''.join(f'src/a.c  D  {row}\n' for row in rows)
                with tree(dict(BODIES, **stub(table=True, text=True), **{'config/test/splits.txt': splits})):
                    bodies()
                    self.assertEqual(group()['texlist.inc'], '    names, 3,\n')
                    self.assertEqual(group()['bodies.txt'].splitlines()[:2],
                                     ['texlist  0x0c000000  8  NJS_TEXLIST', 'table  0x0c000008  8  Uint32'])
                    self.assertEqual(group()['text.inc'], '#define TEXT_1 "abcd"\n')

    def test_writes_texts(self):
        with tree(dict(BODIES, **stub(table=True, text=True))):
            bodies()
            self.assertEqual(group()['text.inc'], '#define TEXT_1 "abcd"\n')

    def test_a_text_after_a_string_starts_where_its_placeholder_does(self):
        # the stub's string is "ab" TEXT_1, and the disc's "abwxyz"
        files = dict(BODIES, **stub(table=True, text=True))
        files['build/stub/src/a.lis'] = files['build/stub/src/a.lis'].replace(
            directive(0x10, "H'01", '.DATA.B') + '\n' + directive(0x11, '4,0', '.DATAB.B'),
            '\n'.join((directive(0x10, '"ab"', '.SDATA'), directive(0x12, "H'01", '.DATA.B'), directive(0x13, '4,0', '.DATAB.B'))))
        files['TEST.BIN'] = words(BASE + 0x80, 3, 7, 8) + b'abwxyz\0' + bytes(0xE9)
        with tree(files):
            bodies()
            self.assertEqual(group()['text.inc'], '#define TEXT_1 "wxyz"\n')
            self.assertEqual(group()['bodies.txt'].splitlines()[-1], 'TEXT_1  0x0c000012  4  text')

    def test_a_body_past_the_image_fails_and_writes_nothing(self):
        with tree(dict(BODIES, **{'TEST.BIN': words(BASE + 0x80, 3, 7)})):
            with self.assertRaisesRegex(SystemExit, r'table, 0x0c000008 to 0x0c000010, lies outside TEST\.BIN'):
                bodies()
            self.assertFalse(GROUP.exists())

    def test_a_text_past_the_image_fails(self):
        files = dict(BODIES, **stub(table=True, text=True))
        with tree(dict(files, **{'TEST.BIN': words(BASE + 0x80, 3, 7, 8) + b'abc'})), self.assertRaisesRegex(
                SystemExit, r'TEXT_1 and its null, 0x0c000010 to 0x0c000015, lie outside'):
            bodies()

    def test_a_late_failure_leaves_the_former_group_whole(self):
        # a text the stub build has no placeholder for, found after the bodies
        files = dict(BODIES, **stub(table=True, text=True), **FORMER)
        files['build/stub/src/a.lis'] = files['build/stub/src/a.lis'].replace("H'01", "H'02")
        with tree(files):
            with self.assertRaisesRegex(SystemExit, 'ends 0 strings with TEXT_1'):
                bodies()
            self.assertEqual(group(), {'texlist.inc': 'old\n', 'dropped.inc': 'old\n', 'bodies.txt': 'old\n'})

    def test_a_failed_write_leaves_no_bodies_txt(self):
        with tree(dict(BODIES, **FORMER)):
            with mock.patch('data.publish', side_effect=OSError('disk full')), self.assertRaisesRegex(OSError, 'disk full'):
                bodies()
            self.assertNotIn('bodies.txt', group())

    def test_a_const_name_from_elsewhere_casts(self):
        with tree(dict(BODIES, **{'src/a.h': 'extern const NJS_TEXNAME names[];\n'})):
            bodies()
            self.assertEqual(group()['texlist.inc'], '    (NJS_TEXNAME *)names, 3,\n')

    def test_a_body_can_point_at_a_struct_typed_extern(self):
        with tree(dict(BODIES, **{'src/a.h': 'extern struct texname names[];\n'})):
            bodies()
            self.assertEqual(group()['texlist.inc'], '    (NJS_TEXNAME *)names, 3,\n')

    def test_names_come_from_the_headers_the_source_includes(self):
        with tree(dict(BODIES, **stub(headers=()))), \
                self.assertRaisesRegex(SystemExit, 'a body points at names, which it doesn\'t declare'):
            bodies()

    def test_conflicting_declarations_fail_naming_both(self):
        files = dict(BODIES, **stub(headers=('src/a.h', 'src/b.h')))
        files['src/b.h'] = '\nextern NJS_MODEL names;\n'
        with tree(files), self.assertRaisesRegex(
                SystemExit, r'src/b\.h:2: extern NJS_MODEL names; conflicts with src/a\.h:1: extern NJS_TEXNAME names\[\];'):
            bodies()

    def test_a_missing_dependency_file_fails(self):
        files = {path: text for path, text in BODIES.items() if path != 'build/stub/src/a.d'}
        with tree(files), self.assertRaisesRegex(SystemExit, r'build/stub/src/a\.d: missing'):
            bodies()

    def test_an_automatic_array_takes_the_template_its_function_loads(self):
        with tree(automatic(2)):
            bodies()
            self.assertEqual(group()['pair.inc'], '    0x00000011, 0x00000022,\n')
            self.assertEqual(group()['bodies.txt'].splitlines()[-1], 'pair  0x0c000018  8  Uint32')

    def test_an_automatic_array_with_no_template_of_its_size_fails(self):
        with tree(automatic(3)), self.assertRaisesRegex(SystemExit, "f loads 0 templates of pair's 12 bytes"):
            bodies()

    def test_an_automatic_arrays_extent_can_be_a_constant(self):
        files = automatic(2)
        files['src/a.c'] = files['src/a.c'].replace('pair[2]', 'pair[PAIR]') + '#define PAIR (1 + 1)\n'
        with tree(files):
            bodies()
            self.assertEqual(group()['pair.inc'], '    0x00000011, 0x00000022,\n')

    def test_an_assembled_stub_listing_places_the_bodies(self):
        # asmsh's listing, as a patched source's stub build writes it
        files = dict(BODIES, **FORMER)
        files['build/stub/src/a.lis'] = assembled_listing(
            [(0, 'L1:                              ; static: texlist'),
             (0, ".DATA.L     H'00000000", '00000000'),
             (4, '.DATAB.B    4,0', '00'),
             (8, 'L9:                              ; static: table'),
             (8, ".DATA.L     H'00000000,H'00000000", '0000000000000000')])
        with tree(files):
            bodies()
            self.assertEqual(group()['bodies.txt'],
                             'texlist  0x0c000000  8  NJS_TEXLIST\ntable  0x0c000008  8  Uint32\n')

    def test_an_assembled_listings_long_line_goes_on_in_its_code(self):
        # a line too long for the listing is cut short, and its code goes on
        # in rows of their own
        text = assembled_listing(
            [(0, 'L1:                              ; static: table'),
             (0, ".DATA.L     H'00000000,H'00000000,H'00000000,H'000", '0000000000000000'),
             (8, None, '0000000000000000')])
        with tree({'a.lis': text}):
            self.assertEqual(data.listing(Path('a.lis'))[0], {'table': ('D', 0, 16)})

    def test_a_long_global_names_comment_goes_on_in_a_line_of_its_own(self):
        # SHC writes the label of a global whose name has 31 characters or
        # more past its comment's column, and the comment on the next line
        name = 'a' * 31
        text = '\n'.join([f'D   00000000              _{name}:', f'{"":59}; static: {name}',
                          directive(0, "H'00000000,H'00000000"),
                          'DATA     SECTION (D):                               00000008 Byte(s)'])
        with tree({'a.lis': text + '\n'}):
            self.assertEqual(data.listing(Path('a.lis'))[0], {name: ('D', 0, 8)})

    def test_a_strings_bytes_give_its_datums_size(self):
        # a ; in a string, and the bytes c2 a3, which UTF-8 would read as one character
        text = '\n'.join([label(0, 'semi'), directive(0, '"a;b"', '.SDATA'), label(4, 'pound'),
                          directive(4, '"\xc2\xa3"', '.SDATA'),
                          'DATA     SECTION (D):                               00000008 Byte(s)'])
        with tree({'a.lis': (text + '\n').encode('latin-1')}):
            self.assertEqual(data.listing(Path('a.lis'))[0], {'semi': ('D', 0, 3), 'pound': ('D', 4, 2)})

    def test_a_string_holds_any_byte_but_lf(self):
        # 0x85, which str.splitlines() ends a line at, as TEXT_133's placeholder starts with it, in lines that end
        # in CRLF as SHC's do
        rows = ['D   00000000              L1:', directive(0, '"\x85@"', '.SDATA'), directive(2, "H'00", '.DATA.B'),
                'DATA     SECTION (D):                               00000004 Byte(s)']
        with tree({'a.lis': ('\r\n'.join(rows) + '\r\n').encode('latin-1')}):
            self.assertEqual(data.strings(Path('a.lis')), [('D', 0, b'\x85@\0')])

    def test_a_comment_holds_no_body_and_no_include(self):
        for commented in ('NJS_TEXLIST bogus = {\n#include "a/bogus.inc"\n};', '#include "a/stray.inc"'):
            files = dict(BODIES)
            files['src/a.c'] += f'/*\n{commented}\n*/\n'
            with self.subTest(commented=commented), tree(files):
                bodies()
                self.assertEqual(sorted(group()), ['bodies.txt', 'table.inc', 'texlist.inc'])

    def test_a_line_marker_starts_its_section(self):
        # -optimize=0 names P on a line marker ahead of a static function,
        # whose pool's label is then code's, and no template
        files = dict(BODIES)
        code = ['P                         ; File a.c      , Line 3        ; block',
                '    00000000              L1:                              ; function: f',
                f'    00000000 {"D007":10} {"MOV.L":12}L2,R0 ; _table',
                '    00000020              L2:',
                f'    00000020 {"<00000000>":10} {".DATA.L":12}_table']
        files['build/stub/src/a.lis'] = '\n'.join(code) + '\n' + files['build/stub/src/a.lis']
        with tree(files):
            bodies()
            self.assertEqual(group()['table.inc'], '    0x00000007, 0x00000008,\n')


def laid(lines, *held):
    # BODIES's names and headers, a source of lines whose stub listing
    # places each (name, bytes) of held in turn from 0x0c000000, and an
    # image that holds those bytes there
    files = dict(BODIES)
    files['config/test/splits.txt'] = 'D  0x0c000000  0x0c000100\nsrc/a.c  D  0x0c000000  0x0c000080\n'
    files['src/a.c'] = '\n'.join(['#include "a.h"', *lines]) + '\n'
    files['sdk/include/sg_xpt.h'], files['sdk/include/njtypes.h'], files['sdk/include/ninja.h'] = SDK
    files['build/stub/src/a.d'] = ('build/stub/src/a.obj: src/a.c sdk/include/sg_xpt.h sdk/include/njtypes.h '
                                   'sdk/include/ninja.h src/a.h\n')
    listing, payload = [], b''
    for name, raw in held:
        listing += [label(len(payload), name), directive(len(payload), f'{len(raw)},0', '.DATAB.B')]
        payload += raw
    listing.append('DATA     SECTION (D):                               00000080 Byte(s)')
    files['build/stub/src/a.lis'] = '\n'.join(listing) + '\n'
    files['TEST.BIN'] = payload + bytes(0x100 - len(payload))
    return files


def coded(files):
    # files, with code at 0x0c000100 whose first function the tables name step
    files['config/test/splits.txt'] = files['config/test/splits.txt'].replace(
        'D  0x0c000000  0x0c000100\n', 'D  0x0c000000  0x0c000100\nP  0x0c000100  0x0c000200\n')
    files['config/test/symbols.txt'] += '0x0c000100  step\n'
    files['TEST.BIN'] += bytes(0x100)
    return files


def body(name, lines, *held):
    # what --bodies writes for name
    with tree(laid(lines, *held)):
        bodies()
        return group()[f'{name}.inc']


class Records(unittest.TestCase):
    def test_a_record_the_source_defines_takes_a_body(self):
        lines = ['struct pair { NJS_POINT3 *at; Sint16 count; };',
                 'static NJS_POINT3 points[2] = {', '#include "a/points.inc"', '};',
                 'static struct pair pairs[2] = {', '#include "a/pairs.inc"', '};']
        held = [('points', struct.pack('<6f', 1, 2, 3, 4, 5, 6)),
                ('pairs', struct.pack('<Ih2xIh2x', BASE + 12, 1, BASE, 2))]
        with tree(laid(lines, *held)):
            bodies()
            self.assertEqual(group()['pairs.inc'], '    { &points[1], 1 },\n    { points, 2 },\n')
            self.assertEqual(group()['bodies.txt'].splitlines()[-1], 'pairs  0x0c000018  16  struct pair')

    def test_a_record_names_a_function_its_source_declares(self):
        lines = ['struct path { Sint16 count; void (*step)(void); };', 'void path_step(void);',
                 'static struct path paths[1] = {', '#include "a/paths.inc"', '};']
        files = laid(lines, ('paths', struct.pack('<h2xI', 3, BASE + 0xc0)))
        files['config/test/symbols.txt'] += '0x0c0000c0  path_step\n'
        with tree(files):
            bodies()
            self.assertEqual(group()['paths.inc'], '    { 3, path_step },\n')

    def test_a_record_names_a_function_that_takes_a_function_pointer(self):
        lines = ['struct path { Sint16 count; void (*step)(void); };', 'void path_step(void (*done)(int), int when);',
                 'static struct path paths[1] = {', '#include "a/paths.inc"', '};']
        files = laid(lines, ('paths', struct.pack('<h2xI', 3, BASE + 0xc0)))
        files['config/test/symbols.txt'] += '0x0c0000c0  path_step\n'
        with tree(files):
            bodies()
            self.assertEqual(group()['paths.inc'], '    { 3, path_step },\n')

    def test_a_record_naming_an_undeclared_function_fails(self):
        lines = ['struct path { Sint16 count; void (*step)(void); };',
                 'static struct path paths[1] = {', '#include "a/paths.inc"', '};']
        files = laid(lines, ('paths', struct.pack('<h2xI', 3, BASE + 0xc0)))
        files['config/test/symbols.txt'] += '0x0c0000c0  path_step\n'
        with tree(files), self.assertRaisesRegex(SystemExit, "a body points at path_step, which it doesn't declare"):
            bodies()

    def test_records_of_a_multidimensional_array_take_its_braces(self):
        lines = ['#define ROW 2', 'struct pair { Sint16 a, b; };',
                 'static struct pair grid[2][ROW] = {', '#include "a/grid.inc"', '};']
        self.assertEqual(body('grid', lines, ('grid', struct.pack('<8h', *range(1, 9)))),
                         '    {\n        { 1, 2 },\n        { 3, 4 },\n    },\n'
                         '    {\n        { 5, 6 },\n        { 7, 8 },\n    },\n')

    def test_an_inner_extent_in_octal_is_octal(self):
        lines = ['struct pair { Sint16 a, b; };', 'static struct pair grid[2][010] = {', '#include "a/grid.inc"', '};']
        rows = ('    {\n' + '        { 0, 0 },\n' * 8 + '    },\n') * 2
        self.assertEqual(body('grid', lines, ('grid', bytes(64))), rows)

    def test_records_that_dont_fill_the_inner_extents_fail(self):
        lines = ['struct pair { Sint16 a, b; };', 'static struct pair grid[2][3] = {', '#include "a/grid.inc"', '};']
        with tree(laid(lines, ('grid', bytes(16)))), self.assertRaisesRegex(
                SystemExit, r"grid's 4 elements don't fill its inner extents \[3\]"):
            bodies()

    def test_an_array_types_body_is_its_elements(self):
        lines = ['typedef Float TABLE[8];', 'static TABLE fog = {', '#include "a/fog.inc"', '};']
        self.assertEqual(body('fog', lines, ('fog', struct.pack('<8f', *range(8)))),
                         '    0.0F, 1.0F, 2.0F, 3.0F, 4.0F, 5.0F,\n    6.0F, 7.0F,\n')

    def test_a_functions_static_takes_a_body(self):
        lines = ['void f(void)', '{', '    static NJS_POINT2 at = {', '#include "a/at.inc"', '    };', '}']
        self.assertEqual(body('at', lines, ('at', struct.pack('<2f', 1.5, -2))), '    1.5F, -2.0F,\n')

    def test_two_functions_statics_of_one_name_take_no_body(self):
        lines = ['void f(void)', '{', '    static NJS_POINT2 at = {', '#include "a/at.inc"', '    };', '}',
                 'void g(void)', '{', '    static int at = 0;', '}']
        with tree(laid(lines, ('at', bytes(8)), ('at', bytes(4)))), self.assertRaisesRegex(
                SystemExit, 'at names two statics, and a body needs one'):
            bodies()

    def test_pointers_into_a_multidimensional_array_take_its_subscripts(self):
        lines = ['#define BYTES 4', 'struct frame { const Uint8 *bitmap; const Uint8 (*rows)[BYTES]; };',
                 'static const Uint8 pictures[3][BYTES] = {', '#include "a/pictures.inc"', '};',
                 'static struct frame frames[2] = {', '#include "a/frames.inc"', '};']
        held = [('pictures', bytes(range(12))), ('frames', words(BASE + 4, BASE, BASE + 8, BASE + 8))]
        self.assertEqual(body('frames', lines, *held),
                         '    { &pictures[1][0], &pictures[0] },\n    { &pictures[2][0], &pictures[2] },\n')

    def test_a_pointer_to_an_array_no_element_starts_fails_naming_where_it_lies(self):
        # a pointer to an array of BYTES that holds no address, or a byte of a
        # row past its start, and one to an array of points into a point
        rows = ['#define BYTES 4', 'struct frame { const Uint8 (*rows)[BYTES]; };',
                'static const Uint8 pictures[3][BYTES] = {', '#include "a/pictures.inc"', '};',
                'static struct frame frames[1] = {', '#include "a/frames.inc"', '};']
        spans = ['struct span { NJS_POINT3 (*rows)[2]; };', 'static NJS_POINT3 points[2][2] = {',
                 '#include "a/points.inc"', '};', 'static struct span spans[1] = {', '#include "a/spans.inc"', '};']
        for lines, held, error in (
                (rows, [('pictures', bytes(12)), ('frames', words(0x8428))],
                 'TEST.BIN: 0x0c00000c: a pointer to const Uint8[] holds 0x00008428, no address'),
                (rows, [('pictures', bytes(12)), ('frames', words(BASE + 6))],
                 'TEST.BIN: 0x0c00000c: a pointer to an array of const Uint8 holds 0x0c000006, where pictures '
                 'holds none'),
                (spans, [('points', bytes(48)), ('spans', words(BASE + 4))],
                 'TEST.BIN: 0x0c000030: a pointer to an array of NJS_POINT3 holds 0x0c000004, inside an element')):
            with self.subTest(error=error), tree(laid(lines, *held)), \
                    self.assertRaisesRegex(EmitError, f'^{re.escape(error)}$'):
                bodies()

    def test_a_pointer_to_an_element_of_a_name_from_elsewhere_names_the_element(self):
        # BODIES's tables put names at 0x0c000080, and a.h declares it an array of NJS_TEXNAME
        lines = ['struct entry { NJS_TEXNAME *name; };', 'static struct entry entries[2] = {',
                 '#include "a/entries.inc"', '};']
        self.assertEqual(body('entries', lines, ('entries', words(BASE + 0x98, BASE + 0x80))),
                         '    { &names[2] },\n    { names },\n')
        with tree(laid(lines, ('entries', words(BASE + 0x84, 0)))), self.assertRaisesRegex(
                SystemExit, 'a body points 4 bytes into names, and not at an element of it'):
            bodies()

    def test_a_pointer_to_a_function_a_header_declares_is_its_name(self):
        lines = ['struct kind { Sint16 level; void (*step)(void); };', 'static struct kind kinds[2] = {',
                 '#include "a/kinds.inc"', '};']
        files = laid(lines, ('kinds', struct.pack('<h2xIh2xI', 3, BASE + 0x90, 2, 0)))
        files['config/test/symbols.txt'] += '0x0c000090  step\n'
        files['src/a.h'] += 'void step(void);\n'
        with tree(files):
            bodies()
            self.assertEqual(group()['kinds.inc'], '    { 3, step },\n    { 2, 0 },\n')
        # no declaration, no name, and a prototype in a comment is none
        for header in (BODIES['src/a.h'], BODIES['src/a.h'] + '/*\nvoid step(void);\n*/\n'):
            files['src/a.h'] = header
            with self.subTest(header=header), tree(files), \
                    self.assertRaisesRegex(SystemExit, 'a body points at step, which it doesn\'t declare'):
                bodies()

    def test_an_array_of_a_kind_that_is_one_record_takes_each_record(self):
        lines = ['static NJS_TEXLIST lists[2] = {', '#include "a/lists.inc"', '};']
        self.assertEqual(body('lists', lines, ('lists', words(BASE + 0x80, 1, BASE + 0x8c, 2))),
                         '    { names, 1 },\n    { &names[1], 2 },\n')

    def test_an_array_a_meshset_takes_as_uvs_writes_uvs(self):
        # colors and uvs are both NJS_COLOR, and only the meshset's uvs
        # pointer makes uvs an array of uvs
        lines = ['static NJS_COLOR colors[1] = {', '#include "a/colors.inc"', '};', 'static NJS_COLOR uvs[1] = {',
                 '#include "a/uvs.inc"', '};', 'static NJS_MESHSET meshsets[1] = {', '#include "a/meshsets.inc"', '};']
        held = [('colors', words(0xff0000ff)), ('uvs', struct.pack('<2h', 1, -2)),
                ('meshsets', struct.pack('<2H5I', 0, 1, 0, 0, 0, BASE, BASE + 4))]
        with tree(laid(lines, *held)):
            bodies()
            self.assertEqual((group()['colors.inc'], group()['uvs.inc']), ('    0xff0000ff,\n', '    UV(1, -2),\n'))

    def test_a_pointer_into_an_array_the_source_declares_names_its_element(self):
        lines = ['static NJS_TEXNAME textures[3];', 'static NJS_TEXLIST texlist = {', '#include "a/texlist.inc"', '};']
        held = [('textures', bytes(36)), ('texlist', words(BASE + 24, 1))]
        self.assertEqual(body('texlist', lines, *held), '    &textures[2], 1,\n')

    def test_a_pointer_to_a_volatile_datum_casts(self):
        lines = ['static volatile NJS_TEX corner = {1, 2};', 'struct quad { NJS_TEX *at; };',
                 'static volatile struct quad quad = {', '#include "a/quad.inc"', '};']
        self.assertEqual(body('quad', lines, ('corner', struct.pack('<2h', 1, 2)), ('quad', words(BASE))),
                         '    (NJS_TEX *)&corner,\n')

    def test_a_pointer_no_memory_has_is_its_number(self):
        lines = ['static NJS_OBJECT arm = {', '#include "a/arm.inc"', '};']
        record = words(0, 0x8428, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x89c8, 0)
        self.assertIn('(NJS_MODEL *)0x00008428', body('arm', lines, ('arm', record)))

    def test_a_pointer_to_a_functions_static_takes_its_type(self):
        lines = ['struct quad { NJS_TEX *at; NJS_TEX *row; };', 'void f(void)', '{', '    static NJS_TEX corner = {1, 2};',
                 '    static NJS_TEX row[2] = {{1, 2}, {3, 4}};', '    static struct quad quad = {', '#include "a/quad.inc"',
                 '    };', '}']
        held = [('corner', bytes(4)), ('row', bytes(8)), ('quad', words(BASE, BASE + 8))]
        self.assertEqual(body('quad', lines, *held), '    &corner, &row[1],\n')

    def test_a_pointer_to_a_struct_of_no_tag_casts_its_address(self):
        lines = ['static struct {', '    NJS_TEX uv[2];', '} cloud = {{{1, 2}, {3, 4}}};', 'struct quad { NJS_TEX *at; };',
                 'static struct quad quad = {', '#include "a/quad.inc"', '};']
        self.assertEqual(body('quad', lines, ('cloud', bytes(8)), ('quad', words(BASE))), '    (NJS_TEX *)&cloud,\n')

    def test_a_pointer_to_a_datum_whose_declaration_goes_unread_fails(self):
        # the declarations give edge, a second declarator, no type, and give
        # corner, beside it, its own
        lines = ['static NJS_TEX corner = {1, 2}, edge = {3, 4};', 'struct quad { NJS_TEX *at; };',
                 'static struct quad quad = {', '#include "a/quad.inc"', '};']
        held = [('corner', bytes(4)), ('edge', bytes(4))]
        self.assertEqual(body('quad', lines, *held, ('quad', words(BASE))), '    &corner,\n')
        with tree(laid(lines, *held, ('quad', words(BASE + 4)))), self.assertRaisesRegex(
                SystemExit, "a body points at edge, whose declaration tools/data.py doesn't read"):
            bodies()

    def test_a_union_is_named_by_its_first_member(self):
        # at its start: past it, a pointer counts bytes from the union
        lines = ['static union {', '    NJS_TEX uv[2];', '    Sint16 coord[4];', '} uvs;',
                 'struct quad { NJS_TEX *at; Sint16 *coord; NJS_TEX *next; };',
                 'static struct quad quad = {', '#include "a/quad.inc"', '};']
        self.assertEqual(body('quad', lines, ('uvs', bytes(8)), ('quad', words(BASE, BASE, BASE + 4))),
                         '    uvs.uv, (Sint16 *)&uvs, (NJS_TEX *)((char *)&uvs + 4),\n')

    def test_a_pointer_that_drops_a_const_casts(self):
        lines = ['static const NJS_TEX corner = {1, 2};', 'struct quad { NJS_TEX *at; const NJS_TEX *kept; void *any; };',
                 'static struct quad quad = {', '#include "a/quad.inc"', '};']
        self.assertEqual(body('quad', lines, ('corner', struct.pack('<2h', 1, 2)), ('quad', words(BASE, BASE, BASE))),
                         '    (NJS_TEX *)&corner, &corner, (void *)&corner,\n')

    def test_an_array_of_pointers_takes_a_body(self):
        lines = ['static NJS_POINT3 points[2] = {', '#include "a/points.inc"', '};',
                 'static NJS_POINT3 *ends[2] = {', '#include "a/ends.inc"', '};']
        held = [('points', struct.pack('<6f', 1, 2, 3, 4, 5, 6)), ('ends', words(BASE + 12, 0))]
        with tree(laid(lines, *held)):
            bodies()
            self.assertEqual(group()['ends.inc'], '    &points[1], 0,\n')
            self.assertEqual(group()['bodies.txt'].splitlines()[-1], 'ends  0x0c000018  8  NJS_POINT3 *')

    def test_a_pointer_to_a_function_names_the_function(self):
        lines = ['void step(Sint32 count);', 'struct mode { Sint32 waits; void (*step)(Sint32 count); };',
                 'static struct mode modes[2] = {', '#include "a/modes.inc"', '};']
        with tree(coded(laid(lines, ('modes', words(0, BASE + 0x100, 1, 0))))):
            bodies()
            self.assertEqual(group()['modes.inc'], '    { 0, step },\n    { 1, 0 },\n')

    def test_a_pointer_into_a_function_fails(self):
        lines = ['void step(Sint32 count);', 'struct mode { Sint32 waits; void (*step)(Sint32 count); };',
                 'static struct mode mode = {', '#include "a/mode.inc"', '};']
        with tree(coded(laid(lines, ('mode', words(0, BASE + 0x104))))), self.assertRaisesRegex(
                SystemExit, "a body points at dat_0c000104, which it doesn't declare"):
            bodies()

    def test_an_array_of_no_extent_fails(self):
        lines = ['static NJS_POINT2 points[] = {', '#include "a/points.inc"', '};']
        with tree(laid(lines, ('points', bytes(8)))), self.assertRaisesRegex(
                SystemExit, "points needs an extent, and its stub build's zero body would give it one element"):
            bodies()

    def test_an_include_no_declaration_takes_fails(self):
        # a line between the #include and "};", and a qualifier after the *
        for lines in (['static NJS_POINT2 points[1] = {', '#include "a/points.inc"', '', '};'],
                      ['static NJS_POINT2 *const points[1] = {', '#include "a/points.inc"', '};']):
            with self.subTest(lines=lines), tree(laid(lines, ('points', bytes(8)))), self.assertRaisesRegex(
                    SystemExit, r'no declaration takes a/points\.inc: a body\'s #include has its declaration\'s '
                                r'"= \{" on the line before and "\};" on the line after, and the declaration puts any '
                                r'qualifiers before a type of one word or struct and a tag, then declares a datum of '
                                r'static storage, with one \* at most, or a function\'s automatic array, with no \*$'):
                bodies()

    def test_a_type_with_no_layout_fails_naming_why(self):
        lines = ['struct bits { Uint32 flag : 1; };', 'static struct bits bits = {', '#include "a/bits.inc"', '};']
        with tree(laid(lines, ('bits', bytes(4)))), self.assertRaisesRegex(
                SystemExit, 'bits is struct bits, which tools/data.py writes no body for: '
                            'struct bits: flag is a bit-field'):
            bodies()


# kinds whose numbers are values, and whose steps the source writes, and the
# bytes the image holds for them: 3, -2 and 1.5, then 0x80, 7 and -0.25
KIND = 'struct kind { Uint8 create; Sint16 attr; Float range; void (*step)(void); };'
KIND_SOURCE = [KIND, 'void ring_step(void);', '#include "a/values.inc"',
               'static struct kind kinds[2] = {', '    {VALUE_1, VALUE_2, VALUE_3, ring_step},',
               '    {VALUE_4, VALUE_5, VALUE_6, NULL},', '};']
HELD = ('kinds', struct.pack('<BxhfIBxhfI', 3, -2, 1.5, 0, 0x80, 7, -0.25, 0))


def values(lines, *held):
    # what --bodies writes for lines' values, and bodies.txt
    with tree(laid(lines, *held)):
        bodies()
        return group()['values.inc'], group()['bodies.txt']


class Values(unittest.TestCase):
    def test_each_value_is_the_number_at_its_member(self):
        self.assertEqual(values(KIND_SOURCE, HELD), (
            '#define VALUE_1 0x03\n#define VALUE_2 -2\n#define VALUE_3 1.5F\n'
            '#define VALUE_4 0x80\n#define VALUE_5 7\n#define VALUE_6 -0.25F\n',
            'VALUE_1  0x0c000000  1  Uint8\nVALUE_2  0x0c000002  2  Sint16\nVALUE_3  0x0c000004  4  Float\n'
            'VALUE_4  0x0c00000c  1  Uint8\nVALUE_5  0x0c00000e  2  Sint16\nVALUE_6  0x0c000010  4  Float\n'))

    def test_the_stub_takes_0_for_each(self):
        with tree({'a.c': '\n'.join(KIND_SOURCE) + '\n'}):
            data.main(('--values', 'a.c', 'values.inc'))
            self.assertEqual(Path('values.inc').read_text(), ''.join(f'#define VALUE_{n} 0\n' for n in range(1, 7)))

    def test_a_value_in_an_inner_aggregate_and_beside_a_body(self):
        # a record of a point and a count, and a body before it
        lines = ['struct spot { NJS_POINT3 at; Sint16 count; };', '#include "a/values.inc"',
                 'static NJS_POINT2 corner = {', '#include "a/corner.inc"', '};',
                 'static const struct spot spots[1][2] = {', '    {{{0.0F, VALUE_1, 0.0F}, 1}, {{0.0F}, VALUE_2}},', '};']
        held = [('corner', struct.pack('<2f', 1, 2)), ('spots', struct.pack('<3fh2x3fh2x', 0, 9, 0, 1, 0, 0, 0, 5))]
        self.assertEqual(values(lines, *held), (
            '#define VALUE_1 9.0F\n#define VALUE_2 5\n',
            'corner  0x0c000000  8  NJS_POINT2\nVALUE_1  0x0c00000c  4  Float\nVALUE_2  0x0c000024  2  Sint16\n'))

    def test_a_value_stands_alone(self):
        lines = KIND_SOURCE[:4] + ['    {VALUE_1 + 1, VALUE_2, VALUE_3, ring_step},'] + KIND_SOURCE[5:]
        with tree(laid(lines, HELD)), self.assertRaisesRegex(
                SystemExit, r'kinds: VALUE_1 stands alone for one number, not in VALUE_1 \+ 1'):
            bodies()

    def test_a_value_for_a_pointer_fails(self):
        lines = KIND_SOURCE[:5] + ['    {VALUE_4, VALUE_5, VALUE_6, VALUE_7},'] + KIND_SOURCE[6:]
        with tree(laid(lines, HELD)), self.assertRaisesRegex(
                SystemExit, 'kinds: VALUE_7 stands for a pointer, which the source writes itself'):
            bodies()

    def test_an_aggregate_beside_a_value_takes_braces(self):
        lines = ['struct spot { NJS_POINT3 at; Sint16 count; };', '#include "a/values.inc"',
                 'static struct spot spot = {0.0F, 0.0F, 0.0F, VALUE_1};']
        with tree(laid(lines, ('spot', bytes(16)))), self.assertRaisesRegex(
                SystemExit, 'spot: an aggregate whose initializer has no braces, beside a value'):
            bodies()

    def test_more_initializers_than_members_fail(self):
        lines = KIND_SOURCE[:5] + ['    {VALUE_4, VALUE_5, VALUE_6, NULL, NULL},'] + KIND_SOURCE[6:]
        with tree(laid(lines, HELD)), self.assertRaisesRegex(SystemExit, 'more initializers than the 4 its braces hold'):
            bodies()

    def test_a_value_no_initializer_holds_fails(self):
        lines = [*KIND_SOURCE, 'int f(void)', '{', '    return VALUE_7;', '}']
        with tree(laid(lines, HELD)), self.assertRaisesRegex(
                SystemExit, 'VALUE_7 lies in no initializer of a datum it defines'):
            bodies()

    def test_values_are_numbered_from_1_each_once(self):
        head = KIND_SOURCE[:4] + ['    {VALUE_1, VALUE_2, VALUE_3, ring_step},']
        for lines, error in (([*head, '    {VALUE_4, VALUE_5, VALUE_7, NULL},', '};'],
                              'its values are numbered from 1, with none missing'),
                             ([*head, '    {VALUE_1, VALUE_5, VALUE_6, NULL},', '};'],
                              'it writes VALUE_1 2 times, and a value is one number'),
                             (['#define RANGE VALUE_3', *KIND_SOURCE], 'a macro holds a value'),
                             (['#define RANGE /* a comment', ' */ VALUE_3', *KIND_SOURCE], 'a macro holds a value')):
            with self.subTest(error=error), tree({'a.c': '\n'.join(lines) + '\n'}), \
                    self.assertRaisesRegex(SystemExit, error):
                data.value_stubs('a.c', 'values.inc')

    def test_values_and_their_include_come_together(self):
        for lines, held in (([line for line in KIND_SOURCE if 'values.inc' not in line], HELD),
                            (['#include "a/values.inc"', 'static NJS_POINT2 at = {1.0F, 2.0F};'], ('at', bytes(8)))):
            with self.subTest(lines=lines), tree(laid(lines, held)), self.assertRaisesRegex(
                    SystemExit, 'writes values, and includes a/values.inc, or neither'):
                bodies()

    def test_values_cant_name_a_body(self):
        lines = ['static NJS_POINT2 values = {', '#include "a/values.inc"', '};']
        with tree(laid(lines, ('values', bytes(8)))), self.assertRaisesRegex(
                SystemExit, "a/values.inc holds its values, and can't be values's body"):
            bodies()


# an image whose generated range holds a name its own source declares
DECLARATIONS = {
    'config/1st_read/symbols.txt': '',
    'config/test/splits.txt': 'D  0x0c000000  0x0c000100\nmodules/test/a.c  D  0x0c000000  0x0c000040\n'
                              'gen/test/d_0c000040.c  D  0x0c000040  0x0c000080\n'
                              'modules/shared/c.c  D  0x0c000080  0x0c000100\n',
    'config/test/symbols.txt': '0x0c000040  model\n',
    'modules/test/a.c': 'extern NJS_MODEL model;\n',
    'modules/shared/c.c': '',
}
DECLARED_TXT = Path('build/gen/test/declared.txt')


class Declarations(unittest.TestCase):
    def declared(self, files):
        with tree(dict(DECLARATIONS, **files)):
            data.declarations('test', str(DECLARED_TXT))
            return DECLARED_TXT.read_text()

    def test_lists_the_types_the_sources_agree_on(self):
        self.assertEqual(self.declared({'modules/test/b.c': 'extern NJS_MODEL model[];\n'}), 'model  NJS_MODEL\n')

    def test_the_images_own_sources_decide_first(self):
        self.assertEqual(self.declared({'modules/shared/c.c': 'extern NJS_OBJECT model;\n'}), 'model  NJS_MODEL\n')

    def test_conflicting_types_fail_naming_both(self):
        with self.assertRaisesRegex(SystemExit, r'modules/test/b\.c:1: extern NJS_OBJECT model; conflicts with '
                                                r'modules/test/a\.c:1: extern NJS_MODEL model;'):
            self.declared({'modules/test/b.c': 'extern NJS_OBJECT model;\n'})

    def test_a_struct_typed_extern_gives_no_type(self):
        self.assertEqual(self.declared({'modules/test/a.c': 'extern struct model model;\n'}), '')

    def test_a_comment_declares_nothing_and_keeps_the_lines_after_it(self):
        self.assertEqual(self.declared({'modules/test/b.c': '/*\nextern NJS_OBJECT model;\n*/\n'}),
                         'model  NJS_MODEL\n')
        with self.assertRaisesRegex(SystemExit, r'modules/test/b\.c:4: extern NJS_OBJECT model; conflicts'):
            self.declared({'modules/test/b.c': '/*\n * a\n */\nextern NJS_OBJECT model;\n'})

    def test_declared_txt_is_written_only_where_its_bytes_differ(self):
        # bytes no text holds, as a damaged run can leave, and then the text it would write, which it leaves be
        self.assertEqual(self.declared({str(DECLARED_TXT): b'\xff'}), 'model  NJS_MODEL\n')
        with tree(dict(DECLARATIONS, **{str(DECLARED_TXT): 'model  NJS_MODEL\n'})), \
                mock.patch('data.publish') as publish:
            data.declarations('test', str(DECLARED_TXT))
        publish.assert_not_called()


class Rows(unittest.TestCase):
    def test_a_file_short_of_its_images_sections_fails(self):
        # test's D runs to 0x0c000100: the whole TEST.BIN reaches it, and a file cut short, as a compressed one is,
        # doesn't
        with tree(GENERATE), contextlib.redirect_stdout(io.StringIO()) as out:
            data.rows('TEST.BIN')
        self.assertTrue(out.getvalue().startswith('# test: 0x0c000000 to '))
        with tree(dict(GENERATE, **{'TEST.BIN': bytes(0x80)})), self.assertRaisesRegex(
                SystemExit, r": TEST\.BIN: ends at 0x0c000080, short of the end of test's last section that isn't bss, "
                            r"0x0c000100; IMAGE is the image's file, uncompressed$"):
            data.rows('TEST.BIN')

    def test_a_row_that_starts_before_the_game_data_owns_its_part_of_it(self):
        # src/a.c's row starts 4 bytes before 1ST_READ.BIN's game data, and src/b.c's runs to the data's end. The
        # gap between them holds words of no model, motion or path, and the file runs on to its end
        last = data.FIRST_GAME_DATA[1]
        files = {'config/1st_read/splits.txt': 'src/a.c  D  0x8c1755e0  0x8c175600\n'
                                               f'src/b.c  D  0x8c175640  {last:#010x}\n',
                 'config/1st_read/symbols.txt': '', 'src/a.c': '', 'src/b.c': '',
                 '1ST_READ.BIN': bytes(0x8C1755E0 - data.FIRST_BASE) + words(*range(1, 25))
                 + bytes(FIRST_FILE_END - 0x8C175640)}
        with tree(files), contextlib.redirect_stdout(io.StringIO()) as out:
            data.main(['--rows', '1ST_READ.BIN'])
        self.assertEqual(out.getvalue(), '# 1st_read: 0x8c175600 to 0x8c175640: no generated row; '
                                         'no recognized model, motion or path\n')

    def test_a_comment_declares_no_variable(self):
        # the variables whose bytes --rows keeps out of the rows it proposes
        files = dict(DECLARATIONS, **{'config/test/symbols.txt': '0x0c000010  count\n0x0c000020  kept\n',
                                      'modules/test/a.c': '/*\nextern Sint16 count;\n*/\nextern Sint16 kept;\n'})
        with tree(files):
            self.assertEqual(data.variables(data.Config('test')), {BASE + 0x20: 2})


if __name__ == '__main__':
    unittest.main()
