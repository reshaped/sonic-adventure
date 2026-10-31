import unittest

from cdata import Bodies, EmitError, Items, Writer
from image import Declared, Image, Item, Laid, window
from layout import Types
from test_image import BASE, END, NAMES, ROWS, TREE, run, tree, tree_items, words

# TREE's C, as the emitter writes it for D
TREE_C = '''/* tree */

#include <ninja.h>


NJS_POINT3 dat_0c000000[] = {
    { 1.0F, 2.0F, 3.0F },
    { 4.0F, 5.0F, 6.0F },
};

NJS_MODEL dat_0c000018 = {
    dat_0c000000, 0, 2,
    0, 0, 0, 0,
    { 0.0F, 0.0F, 0.0F }, 1.0F,
};

NJS_OBJECT tree = {
    0, &dat_0c000018,
    { 0.0F, 0.0F, 0.0F },
    { 0, 0, 0 },
    { 1.0F, 1.0F, 1.0F },
    0, 0,
};
'''


class Sources(unittest.TestCase):
    def test_writes_the_items_of_a_range_as_c(self):
        items = Items(tree(), tree_items(), [(BASE, END)], NAMES)
        emitted = Writer(items, NAMES).source('tree')
        self.assertEqual((emitted.text, emitted.outside), (TREE_C, {}))

    def test_an_item_set_short_of_its_range_is_refused(self):
        # TREE's items but the object, whose bytes then no item holds
        items = tree_items()[:2]
        with self.assertRaisesRegex(EmitError, '^TEST.BIN: 0x0c000040: 52 bytes to 0x0c000074 that no definition '
                                               'holds$'):
            Items(tree(), items, [(BASE, END)], {})

    def test_an_item_of_two_elements_that_is_no_array_is_refused(self):
        # two objects' bytes as one NJS_OBJECT, whose definition would hold
        # the first alone
        image = Image('TEST.BIN', bytes(104), BASE, (), (window(BASE),))
        with self.assertRaisesRegex(EmitError, '^TEST.BIN: 0x0c000000: dat_0c000000 is one NJS_OBJECT, and 104 bytes$'):
            Items(image, [Item(BASE, BASE + 104, 'object')], [(BASE, BASE + 104)], {})

    def test_a_scalar_of_zeros_is_declared_and_defined_as_one(self):
        # a pointer to a word of 0 that the row gives no count, which source()
        # declares ahead of its definition
        image = Image('TEST.BIN', words(BASE + 4, 0), BASE, (), (window(BASE),))
        items = Items(image, [Item(BASE, BASE + 4, 'pointers'), Item(BASE + 4, BASE + 8, 'words', array=False)],
                      [(BASE, BASE + 8)], {})
        text = Writer(items, {}).source('zero').text
        self.assertIn('extern Uint32 dat_0c000004;', text)
        self.assertIn('Uint32 dat_0c000004 = 0x00000000;', text)

    def test_a_consumer_writes_c_without_the_tree(self):
        # TREE's C, written in a directory that holds no table or source,
        # with none of config, csource, data and tags imported
        script = '\n'.join([
            'import sys', 'from cdata import Items, Writer', 'from image import Image, defined, window',
            f'image = Image("TEST.BIN", {TREE!r}, {BASE}, (), (window({BASE}),))',
            f'items = Items(image, [defined(image, {BASE} + at, kind, count) for at, kind, count in {ROWS}], '
            f'[({BASE}, {END})], {NAMES})',
            f'print(Writer(items, {NAMES}).source("tree").text, end="")',
            "print(sorted({'config', 'csource', 'data', 'tags'} & set(sys.modules)))"])
        self.assertEqual(run(script), TREE_C + '[]\n')


class Targets(unittest.TestCase):
    def test_a_body_takes_its_targets_from_the_declarations_it_is_handed(self):
        # a body of struct spot at 0x00 whose pointers point into counts, an
        # array the source defines in bss past the image, at step, a function
        # in code, and into table, an array a header declares extern
        types = Types(['struct spot { int *at; void (*step)(void); int *other; };'])
        spot = Laid(BASE, BASE + 12, types.layout('struct spot'), 'struct spot', False, ())
        counts = Declared(BASE + 0x100, BASE + 0x110, 'int', True, 4)
        image = Image('TEST.BIN', words(BASE + 0x104, BASE + 0x80, BASE + 0x208, 0), BASE, [(BASE + 0x80, BASE + 0x90)],
                      (window(BASE),))
        names = {BASE: 'spot', BASE + 0x80: 'step', BASE + 0x100: 'counts', BASE + 0x200: 'table'}
        items = Items(image, [spot, counts], [(BASE, BASE + 12), (BASE + 0x100, BASE + 0x110)],
                      {BASE: 'spot', BASE + 0x100: 'counts'})
        bodies = Bodies(items, names, types, {'table': ('int', '3')}, {'step'})
        self.assertEqual(bodies.inc('spot', spot), '    &counts[1], step, &table[2],\n')

    def test_a_body_names_an_element_of_a_name_from_outside_only_where_its_declared_extent_holds_it(self):
        # table's third element, where table's declaration gives no extent, or two elements
        types = Types(['struct spot { int *at; };'])
        spot = Laid(BASE, BASE + 4, types.layout('struct spot'), 'struct spot', False, ())
        image = Image('TEST.BIN', words(BASE + 0x208), BASE, (), (window(BASE),))
        for extent in ('', '2'):
            bodies = Bodies(Items(image, [spot], [(BASE, BASE + 4)], {BASE: 'spot'}), {BASE + 0x200: 'table'}, types,
                            {'table': ('int', extent)}, set())
            with self.subTest(extent=extent), \
                    self.assertRaisesRegex(EmitError, '^TEST.BIN: 0x0c000000: a body points at element 2 of table, and '
                                                      'its declarations give table no extent that holds it$'):
                bodies.inc('spot', spot)

    def test_a_body_that_points_at_a_name_it_isnt_handed_fails_naming_the_pointer(self):
        types = Types(['struct spot { int *at; };'])
        spot = Laid(BASE, BASE + 4, types.layout('struct spot'), 'struct spot', False, ())
        image = Image('TEST.BIN', words(BASE + 0x200), BASE, (), (window(BASE),))
        bodies = Bodies(Items(image, [spot], [(BASE, BASE + 4)], {BASE: 'spot'}), {BASE + 0x200: 'table'}, types, {},
                        set())
        with self.assertRaisesRegex(EmitError, "^TEST.BIN: 0x0c000000: a body points at table, which it doesn't "
                                               'declare$'):
            bodies.inc('spot', spot)


class Padding(unittest.TestCase):
    def test_an_item_of_bytes_takes_the_fill_that_aligns_the_next(self):
        # a byte, three bytes of fill and a word: no item holds the fill
        image = Image('TEST.BIN', b'\x07' + bytes(3) + words(9), BASE, (), (window(BASE),))
        items = Items(image, [Item(BASE, BASE + 1, 'bytes'), Item(BASE + 4, BASE + 8, 'words')], [(BASE, BASE + 8)],
                      {})
        self.assertIn('Uint8 dat_0c000000[] = {\n    0x07,\n};\n\nUint32 dat_0c000004[] = {\n    0x00000009,\n};',
                      Writer(items, {}).source('fill').text)

    def test_a_path_whose_padding_isnt_zero_is_refused(self):
        # the byte between a path's ends and its count
        image = Image('TEST.BIN', b'\x01\x7f' + bytes(14), BASE, (), (window(BASE),))
        items = Items(image, [Item(BASE, BASE + 16, 'path')], [(BASE, BASE + 16)], {})
        with self.assertRaisesRegex(EmitError, '^TEST.BIN: 0x0c000001: a pad byte of 0x7f, which no initializer '
                                               'writes$'):
            Writer(items, {}).source('path')


if __name__ == '__main__':
    unittest.main()
