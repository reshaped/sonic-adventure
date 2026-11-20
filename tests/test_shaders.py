"""The ray tracing shaders' copies of the host's numbers. A shader includes no C header, so each flag, mask and count
that a shader and the host both read is written in both, and only this test keeps the two alike. COPIES names each
shader constant with the host's expression it copies, and a shader constant whose name says it is a copy is in
COPIES. Each image a shader declares has the binding, the format and the count that rays.cpp's HELD gives its name,
and each compute pass's work group is as wide as the host dispatches it."""

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SHADERS = ROOT / 'engine/pc/shaders'
# the host's sources that define what the shaders copy
HOST = ('engine/include/sa_engine.h', 'engine/pc/rays.h', 'engine/pc/rays.cpp', 'engine/pc/device.h')
# a shader's constant and the host's expression for it, over the host's names. // is Python's whole division
COPIES = {
    'trace.comp': {
        'MAT_TEXTURE': 'SA_MAT_TEXTURE', 'MAT_BLEND': 'SA_MAT_BLEND', 'MAT_ENV': 'SA_MAT_ENV',
        'MAT_DECAL': 'SA_MAT_DECAL', 'MAT_EMISSIVE': 'SA_MAT_EMISSIVE', 'MAT_COLOR': 'RAY_MAT_COLOR',
        'MAT_PRELIT': 'RAY_MAT_PRELIT', 'MAT_GLOW': 'RAY_MAT_GLOW', 'RAY_BACKDROP': 'RAY_BACKDROP',
        'RAY_NORMALS': 'RAY_NORMALS', 'RAY_NEW': 'RAY_NEW', 'RAY_FOG': 'RAY_FOG', 'RAY_LIQUID': 'RAY_LIQUID',
        'RAY_SHADE_RAMP': 'RAY_SHADE_RAMP', 'RAY_SHADE_LINEAR': 'RAY_SHADE_LINEAR', 'MASK_SEEN': 'MASK_SEEN',
        'MASK_HIDDEN': 'MASK_HIDDEN', 'MASK_BACKDROP': 'MASK_BACKDROP', 'MASK_OBJECT': 'MASK_OBJECT',
        'EMISSIVE_DIM': 'EMISSIVE_DIM', 'EMISSIVE_LIT': 'EMISSIVE_LIT',
        'LAMP_EMPTY': 'LAMP_EMPTY', 'LAMP_HEAD': 'LAMP_HEAD', 'LAMP_SLOT': 'LAMP_SLOT', 'LAMP_WORDS': 'LAMP_WORDS', 'PATCH_WORDS': 'PATCH_WORDS',
        'FACTOR_ONE': 'SA_BLEND_ONE', 'BLEND_OVER': 'SA_BLEND_SRC_ALPHA | SA_BLEND_INV_SRC_ALPHA << 3',
        'RAMP': 'RAY_RAMP', 'SIDE': 'RAY_RAMP // 2 - 1', 'LIGHTS': 'RAY_LIGHTS'},
    'enclosure.comp': {'MASK_SOLID': 'MASK_SOLID'},
    'compose.comp': {'UNFILTERED': 'UNFILTERED'},
}
# a shader constant of one of these names copies the host's
COPIED = re.compile(r'^(MAT|RAY|MASK|LAMP|PATCH|EMISSIVE)_|^UNFILTERED$')
# a shader's image and Vulkan's name for its format
FORMATS = {'rgba16f': 'VK_FORMAT_R16G16B16A16_SFLOAT', 'r32f': 'VK_FORMAT_R32_SFLOAT',
           'rgba8': 'VK_FORMAT_R8G8B8A8_UNORM'}
# the passes the host dispatches a GROUP by GROUP work group at a time
GROUPED = ('trace.comp', 'temporal.comp', 'atrous.comp', 'compose.comp', 'taa.comp')

COMMENT = re.compile(r'/\*.*?\*/|//[^\n]*', re.S)
NUMBER = re.compile(r'\b(0[xX][0-9a-fA-F]+|\d+\.\d*(?:[eE][+-]?\d+)?|\d+(?:[eE][+-]?\d+)?)[uUfF]?\b')
GLSL_CONSTANT = re.compile(r'^const (?:uint|int|float) (\w+) = ([^;]+);', re.M)
CXX_CONSTANT = re.compile(r'\bconstexpr [\w:<> ]+? (\w+) =\s*([^;{]+);')
ENUM = re.compile(r'\benum\s*\w*\s*\{([^}]*)\}')
IMAGE = re.compile(r'layout\(set = 2, binding = (\d+), (\w+)\) uniform (?:readonly |writeonly )?image2D '
                   r'(\w+)(?:\[(\d+)\])?;')
HELD = re.compile(r'\{(\d+), (\d+), (VK_FORMAT_\w+)\}[,}]*;? /\* (\w+) \*/')
LOCAL_SIZE = re.compile(r'layout\(local_size_x = (\d+), local_size_y = (\d+)\) in;')


def value(expression, names):
    """An expression of a shader's or the host's as a number, over names: its numbers less their suffixes, with C's
    operators as Python's."""
    return eval(NUMBER.sub(r'\1', expression), {'__builtins__': {}}, names)


def shader_constants(name):
    """Each constant a shader defines with a number, as {its name: the number}."""
    found = {}
    for constant, expression in GLSL_CONSTANT.findall(COMMENT.sub('', (SHADERS / name).read_text())):
        found[constant] = value(expression, found)
    return found


def host_constants():
    """Each constant and enumerator HOST's sources give a number that this can read, as {its name: the number}. An
    enumerator with no value of its own follows the one before it."""
    found = {}
    for path in HOST:
        text = COMMENT.sub('', (ROOT / path).read_text())
        for body in ENUM.findall(text):
            last = -1
            for entry in filter(None, (e.strip() for e in body.split(','))):
                name, _, expression = (part.strip() for part in entry.partition('='))
                try:
                    last = value(expression, found) if expression else last + 1
                except (NameError, SyntaxError, TypeError):
                    continue
                found[name] = last
        for name, expression in CXX_CONSTANT.findall(text):
            try:
                found[name] = value(expression, found)
            except (NameError, SyntaxError, TypeError):
                continue
    return found


class Shaders(unittest.TestCase):
    def test_a_shader_constant_has_the_value_of_the_host_s(self):
        host = host_constants()
        for shader, copies in COPIES.items():
            constants = shader_constants(shader)
            for name, expression in copies.items():
                with self.subTest(shader=shader, name=name):
                    self.assertIn(name, constants)
                    self.assertAlmostEqual(constants[name], value(expression, host), places=6)

    def test_a_shader_constant_named_as_a_copy_is_checked(self):
        for path in sorted(SHADERS.iterdir()):
            copied = {name for name in shader_constants(path.name) if COPIED.match(name)}
            self.assertEqual(copied - set(COPIES.get(path.name, ())), set(), path.name)

    def test_a_shader_s_image_is_the_one_the_host_holds_under_its_name(self):
        held = {name: (int(binding), int(count), form)
                for binding, count, form, name in HELD.findall((ROOT / 'engine/pc/rays.cpp').read_text())}
        self.assertGreater(len(held), 10)
        seen = set()
        for path in sorted(SHADERS.iterdir()):
            for binding, form, name, count in IMAGE.findall(path.read_text()):
                with self.subTest(shader=path.name, image=name):
                    self.assertEqual(held.get(name), (int(binding), int(count or 1), FORMATS[form]))
                seen.add(name)
        self.assertEqual(seen, set(held))

    def test_a_pass_s_work_group_is_the_host_s(self):
        group = host_constants()['GROUP']
        for shader in GROUPED:
            self.assertEqual(LOCAL_SIZE.findall((SHADERS / shader).read_text()), [(str(group), str(group))], shader)


if __name__ == '__main__':
    unittest.main()
