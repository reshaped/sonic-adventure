import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from image import Image, Item, ReadError, defined, window

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
BASE = 0x0C000000


def floats(*values):
    return struct.pack(f'<{len(values)}f', *values)


def words(*values):
    return struct.pack(f'<{len(values)}I', *values)


# an object, which the tables name tree, of a model of two points: the
# points at 0x00, the model at 0x18 and the object at 0x40, to 0x74
TREE = (floats(1, 2, 3, 4, 5, 6) + words(BASE, 0, 2, 0, 0) + struct.pack('<2H', 0, 0) + floats(0, 0, 0, 1)
        + words(0, BASE + 0x18) + floats(0, 0, 0) + words(0, 0, 0) + floats(1, 1, 1) + words(0, 0))
END = BASE + len(TREE)
NAMES = {BASE + 0x40: 'tree'}
# TREE's rows of data.txt, each (offset, kind, count)
ROWS = ((0, 'points', 2), (0x18, 'model', None), (0x40, 'object', None))


def tree():
    # TREE as an image: no code, and main RAM's window for an address
    return Image('TEST.BIN', TREE, BASE, (), (window(BASE),))


def tree_items():
    # TREE's definitions, as its rows give them
    return [defined(tree(), BASE + at, kind, count) for at, kind, count in ROWS]


def run(script):
    """What script prints, run with tools/ alone on the module path, in an
    empty directory: no table and no source is there to read."""
    with tempfile.TemporaryDirectory() as empty:
        return subprocess.run([sys.executable, '-B', '-c', script], cwd=empty, env={'PYTHONPATH': str(TOOLS)},
                              capture_output=True, text=True, check=True).stdout


class Consumer(unittest.TestCase):
    def test_a_consumer_takes_the_image_without_the_tree(self):
        # TREE, defined by its rows in a directory that holds no table or
        # source, with none of cdata, config, csource, data and tags imported
        script = '\n'.join([
            'import sys', 'from image import Image, defined, window',
            f'image = Image("TEST.BIN", {TREE!r}, {BASE}, (), (window({BASE}),))',
            f'print([(defined(image, {BASE} + at, kind, count).end - {BASE}) for at, kind, count in {ROWS}])',
            "print(sorted({'cdata', 'config', 'csource', 'data', 'tags'} & set(sys.modules)))"])
        self.assertEqual(run(script), '[24, 64, 116]\n[]\n')


class Items(unittest.TestCase):
    def test_a_part_keeps_all_but_its_bounds(self):
        point = Item(BASE, BASE + 24, 'points', declared='NJS_VECTOR', array=False)
        part = point.part(BASE + 12, BASE + 24)
        self.assertEqual((part.start, part.end, part.kind, part.ctype, part.array),
                         (BASE + 12, BASE + 24, 'points', 'NJS_VECTOR', False))
        self.assertEqual((point.start, point.end), (BASE, BASE + 24))


class Defined(unittest.TestCase):
    def test_a_row_gives_its_kind_count_and_form(self):
        image = tree()
        for kind, count, item in (('object', None, (0x40, 0x74, 'object', 0, False)),
                                  ('object', 1, (0x40, 0x74, 'object', 0, True)),
                                  ('points', 2, (0, 0x18, 'points', 0, True)),
                                  ('mdata2', 3, (0, 0x30, 'mdata', 2, True))):
            with self.subTest(kind=kind, count=count):
                start = BASE + item[0]
                found = defined(image, start, kind, count)
                self.assertEqual((found.start - BASE, found.end - BASE, found.kind, found.factors, found.array), item)

    def test_a_text_is_its_bytes_to_its_null(self):
        # any byte but the null, as latin-1 reads it, then zeros; a null past
        # the row, or past the image; and a byte other than 0 after the null
        self.assertEqual(defined(Image('TEST.BIN', b'a\n\x82\xa0\0\0\0\0', BASE), BASE, 'string', 8).text,
                         'a\n\x82\xa0')
        for payload in (b'abc\0', b'abc'):
            with self.subTest(payload=payload), self.assertRaisesRegex(ReadError, "can't be a text of 3 bytes"):
                defined(Image('TEST.BIN', payload, BASE), BASE, 'string', 3)
        with self.assertRaisesRegex(ReadError, "^TEST.BIN: 0x0c000003: 0x58 follows the text's null"):
            defined(Image('TEST.BIN', b'a\0\0X\0', BASE), BASE, 'string', 5)

    def test_a_c_type_is_no_kind(self):
        for kind in ('struct death_zone', 'NJS_MDATA2', 'mdata5', 'typed'):
            with self.subTest(kind=kind):
                self.assertIsNone(defined(tree(), BASE, kind, 1))


if __name__ == '__main__':
    unittest.main()
