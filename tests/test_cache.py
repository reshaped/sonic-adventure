import re
import tempfile
import unittest
from pathlib import Path

import elf
import test_ld

ROOT = Path(__file__).resolve().parents[1]
# the bytes that take each set of the SH-4's operand cache once, and the bytes of one of its lines
SIZE, LINE = 0x4000, 32


def probed(text):
    """Compile the C text with the toolchain's GCC, for its static assertions alone. A message from GCC fails it."""
    with tempfile.TemporaryDirectory() as root:
        Path(root, 'probe.c').write_text(text)
        test_ld.run('bin/sh-elf-gcc', '-ml', '-m4-single-only', '-std=gnu17', '-Wall', '-Werror', '-fsyntax-only',
                    'probe.c', root=root)


class Scratch(unittest.TestCase):
    def test_the_draws_scratch_follows_a_line_on_its_code_s_set_and_starts_past_the_palettes_sets(self):
        # model.c's SCRATCH_LINE, as GCC reads it with live.h's numbers for the cache, for a buffer on each of four
        # lines and code on each of nine: the line it gives lies under 16 KB into the buffer, on the code's set where
        # that is one of sets 256 to 510, on set 256 where it is one of the palettes' 256, and on set 510 where it
        # is the cache's last. draw_model_init starts the scratch on the next line, which so takes one of sets 257 to
        # 511, and none of the palettes', from the sweep's first whole line
        test_ld.toolchain(('bin/sh-elf-gcc',))
        cache = re.findall(r'(?m)^#define OC_(?:LINE|SETS|SIZE) .*$', (ROOT / 'engine/dc/sa1/live.h').read_text())
        model = (ROOT / 'src/draw/model.c').read_text()
        macros = re.findall(r'(?m)^#define SCRATCH_(?:SET|LINE)\(.*\\\n(?:.*\\\n)*.*$', model)
        self.assertEqual((len(cache), len(macros)), (3, 2))
        asserts = []
        for buffer in (0x8C300000, 0x8C300020, 0x8C302000, 0x8C303FE0):
            for line in (0, 1, 127, 255, 256, 310, 509, 510, 511):
                code = 0x8C010000 + line * LINE
                followed = min(max(line, SIZE // LINE // 2), SIZE // LINE - 2)
                wanted = (followed * LINE - buffer) % SIZE
                asserts.append(f'_Static_assert(SCRATCH_LINE({buffer:#x}U, {code:#x}U) == {wanted:#x}, '
                               f'"a buffer at {buffer:#x} and code on line {line}");')
                # the scratch, as draw_model_init starts it, and its set
                scratch = f'({buffer:#x}U + SCRATCH_LINE({buffer:#x}U, {code:#x}U) + OC_LINE)'
                asserts.append(f'_Static_assert(({scratch} & (OC_SIZE - 1)) / OC_LINE == {followed + 1}, '
                               f'"the scratch of a buffer at {buffer:#x} and code on line {line}");')
                self.assertIn(followed + 1, range(SIZE // LINE // 2 + 1, SIZE // LINE))
        probed('\n'.join([*cache, *macros, *asserts, '']))
        self.assertRegex(model, r'Uint32 sweep = \(\(Uint32\)polygon_sweep \+ OC_LINE - 1\) & ~\(OC_LINE - 1\);')
        self.assertIn('void *scratch = (void *)(buffer + SCRATCH_LINE(buffer, sweep) + OC_LINE);', model)


class Heap(unittest.TestCase):
    def test_the_heap_s_arena_starts_on_the_bytes_that_take_each_set_of_the_operand_cache_once(self):
        # task.c's GCC object, compiled under GCCFLAGS as make dc compiles it: the arena opens its .bss, which lies on
        # 16 KB, so the link starts the heap on set 0 whatever .bss it lays ahead. ninja.h takes SH4ZAM's headers
        test_ld.toolchain(('bin/sh-elf-gcc', 'bin/sh-elf-as'))
        if not (ROOT / 'tools/sh4zam/fetched.txt').is_file():
            self.skipTest('tools/sh4zam lacks the fetch make dc makes')
        with tempfile.TemporaryDirectory() as out:
            test_ld.run('bin/sh-elf-gcc', *test_ld.flags(), '-Isdk/include', '-Isrc', '-c', '-o', f'{out}/task.o',
                        'src/system/task.c', root=ROOT)
            found = elf.read(Path(out, 'task.o').read_bytes(), 'task.o')
        arena = next(symbol for symbol in found.symbols if symbol.name == '_heap_arena')
        section = found.sections[arena.section]
        self.assertEqual((section.name, section.align, arena.value % SIZE), ('.bss', SIZE, 0))


if __name__ == '__main__':
    unittest.main()
