"""What a step reads of the port's features is the game's, as src/system/port.h says: the toggles, their chord, the
curtain's count, 16:9's window and whether the frames to come want the split. The game defines each once, under
src/, in a source that make dc and the native slice both compile and no SHC route takes, and no machine's side
defines one again: make dc's C and the PC's side of it, SA1's adapter, name the toggles, the chord, the curtain
and the split only where the lists say, and the slice's link wraps neither call that asks the mode. A name that is
a datum is assigned only in the functions the list says: frame_end sets the window and the split, and the chord, a
save's load and the PC's two settings set the toggles. What passes between the game's port.c and a machine's
overlay is declared apart, in src/system/chord.h, which they alone include. The link places port.c by the first
cold row of config/1st_read/kernel.txt. tests/test_wide.py holds the window's places, and engine/tests/pace.c runs
frame_end beside an engine of its own for when the frames want the split."""

import re
import unittest

import csource
from support import cwd
from test_make import database
from test_wide import ROOT, bodies, places, sources

# the source that defines each name a step reads of the port's features
DEFINED = {
    'port_toggles': 'src/system/port.c',
    'port_chord': 'src/system/port.c',
    'port_curtain': 'src/system/port.c',
    'port_curtain_shown': 'src/system/port.c',
    'port_curtain_level': 'src/system/port.c',
    'port_curtain_advance': 'src/system/port.c',
    'port_view_x': 'src/system/port.c',
    'port_view_aspect': 'src/system/pace.c',
    'port_split_wanted': 'src/system/pace.c',
}
# the sources that include src/system/chord.h: port.c, which defines the chord and the curtain's count, and each
# machine's overlay, which gives the chord the pad and draws by the count. ninja.h gives every other source port.h,
# and a name declared there is one more declaration ahead of every source's code
SEAM = 'src/system/chord.h'
SEAM_INCLUDED = ['engine/dc/overlay.c', 'engine/sa1/overlay.c', 'src/system/port.c']
# each place a name of the toggles, the chord, the curtain or the split appears, as (file, the function or macro
# that holds it, or None at file scope)
PLACES = {
    # the chord flips them, a save keeps and loads two, and frame_end asks the mode by them. A machine's side reads
    # them for its readout and its note, and the PC's turns two on from a player's settings, as a save does
    'port_toggles': {
        ('src/system/port.h', None),
        ('src/system/port.c', None),
        ('src/system/port.c', 'port_chord'),
        ('src/system/pace.c', 'frame_end'),
        ('src/save/save.c', 'save_load'),
        ('src/save/save.c', 'store_switches'),
        ('engine/dc/overlay.c', 'modes_write'),
        ('engine/dc/overlay.c', 'overlay_draw'),
        ('engine/dc/overlay.c', 'overlay_note'),
        ('engine/sa1/overlay.c', 'readout_measure'),
        ('engine/sa1/overlay.c', 'overlay_draw'),
        ('engine/sa1/overlay.c', 'overlay_page'),
        ('engine/sa1/overlay.c', 'overlay_note'),
        ('engine/sa1/overlay.c', 'sa1_wide_on'),
        ('engine/sa1/overlay.c', 'sa1_fast_on'),
    },
    # a machine gives it the first pad's record where it serves or samples the pad
    'port_chord': {
        ('src/system/chord.h', None),
        ('src/system/port.c', 'port_chord'),
        ('engine/dc/overlay.c', 'overlay_pad'),
        ('engine/sa1/overlay.c', 'sa1_chord'),
    },
    # the skip lowers and lifts the curtain and waits on it, frame_end moves its count on, and a machine draws it
    # by the count
    'port_curtain': {
        ('src/system/port.h', None),
        ('src/system/port.c', 'port_curtain'),
        ('src/event/skip.c', 'skip_end'),
        ('src/event/skip.c', 'skip_frame'),
    },
    'port_curtain_shown': {
        ('src/system/port.h', None),
        ('src/system/port.c', 'port_curtain_shown'),
        ('src/event/skip.c', 'skip_frame'),
    },
    'port_curtain_level': {
        ('src/system/chord.h', None),
        ('src/system/port.c', 'port_curtain_level'),
        ('engine/dc/overlay.c', 'overlay_draw'),
        ('engine/sa1/overlay.c', 'overlay_draw'),
    },
    'port_curtain_advance': {
        ('src/system/port.h', None),
        ('src/system/port.c', 'port_curtain_advance'),
        ('src/system/pace.c', 'frame_end'),
    },
    # frame_end sets it from the rate the engine applied, and the split frame's pacing reads it
    'port_split_wanted': {
        ('src/system/port.h', None),
        ('src/system/pace.c', None),
        ('src/system/pace.c', 'frame_end'),
        ('src/system/present.c', 'present_end'),
        ('src/system/present.c', 'present_shown'),
    },
}
# the functions that assign each datum a step reads, as (file, function): frame_end sets the window and the split
# once a frame, the chord flips the toggles, and a save's load and the PC's two settings turn on the two they keep.
# A definition's initialiser, at file scope, is no assignment
WRITERS = {
    'port_view_aspect': {('src/system/pace.c', 'frame_end')},
    'port_split_wanted': {('src/system/pace.c', 'frame_end')},
    'port_toggles': {
        ('src/system/port.c', 'port_chord'),
        ('src/save/save.c', 'save_load'),
        ('engine/sa1/overlay.c', 'sa1_wide_on'),
        ('engine/sa1/overlay.c', 'sa1_fast_on'),
    },
}
# the operators that assign to the name after them, and those that do to the name, or the element of it, before them
ASSIGNS_NEXT = {'++', '--'}
ASSIGNS = csource.ASSIGNMENTS | ASSIGNS_NEXT
# the kernel table whose cold rows place port.c, and the source of its first
KERNEL = 'config/1st_read/kernel.txt'
FIRST_COLD = 'src/system/port.c'


def assigned(tokens, i):
    """Whether the name at tokens[i] is assigned to: one of ASSIGNS follows it and its subscripts, or one of
    ASSIGNS_NEXT comes before it."""
    after, depth = i + 1, 0
    while after < len(tokens) and (depth or tokens[after].text == '['):
        depth += {'[': 1, ']': -1}.get(tokens[after].text, 0)
        after += 1
    return (after < len(tokens) and tokens[after].text in ASSIGNS) or (i > 0 and tokens[i - 1].text in ASSIGNS_NEXT)


@unittest.skipUnless((ROOT / '.git').exists(), 'not a git checkout, so no list of the files it tracks')
class Play(unittest.TestCase):
    def found(self, names):
        """Each place a name of names appears on the non-matching side, by name."""
        found = {name: set() for name in names}
        with cwd(ROOT):
            for path in sources(names):
                for token, _, holder in places(path, csource.GCC, names):
                    found[token.text].add((path, holder))
        return found

    def test_the_game_defines_each_name_a_step_reads_once(self):
        # a definition is a name at a C source's file scope, or in the function of its own name
        found = self.found(DEFINED)
        for name, source in DEFINED.items():
            with self.subTest(name=name):
                self.assertEqual(sorted(path for path, holder in found[name]
                                        if path.endswith('.c') and holder in (None, name)), [source])

    def test_each_name_appears_only_where_the_list_says(self):
        found = self.found(PLACES)
        for name, listed in PLACES.items():
            with self.subTest(name=name):
                self.assertEqual(found[name], listed)

    def test_each_datum_is_assigned_only_in_the_functions_the_list_says(self):
        # the places' list lets a machine's side read a name in a function, and a write there would be a second
        # copy of what the game sets
        found = {name: set() for name in WRITERS}
        with cwd(ROOT):
            for path in sources(WRITERS):
                for holder, tokens in bodies(path, csource.GCC):
                    for i, token in enumerate(tokens):
                        if token.kind == 'name' and token.text in WRITERS and assigned(tokens, i):
                            found[token.text].add((path, holder))
        for name, writers in WRITERS.items():
            with self.subTest(name=name):
                self.assertEqual(found[name], writers, f'{name} is assigned in a function WRITERS does not list, '
                                 'or not in one it does: the game sets what a step reads in these functions '
                                 'alone, so have any other read it, or call one of them')

    def test_port_c_is_the_kernel_tables_first_cold_row(self):
        # the cold objects lie in row order after the image's other code, so a change to an earlier row's object
        # moves a later row's. port.c holds port_view_x, which each on-screen test of an object's step calls
        cold = re.findall(r'(?m)^cold\s+(\S+)$', (ROOT / KERNEL).read_text())
        self.assertEqual(cold[:1], [FIRST_COLD], f"{KERNEL}'s first cold row names another source than {FIRST_COLD}: "
                         'keep its row first, so that no cold source that changes size moves port_view_x')

    def test_only_port_c_and_the_overlays_include_the_header_between_them(self):
        name = SEAM.rsplit('/', 1)[1]
        with cwd(ROOT):
            included = sorted(path for path in sources([name])
                              if re.search(rf'#\s*include\s*"[\w./]*{re.escape(name)}"', csource.code(path, csource.GCC)))
        self.assertEqual(included, SEAM_INCLUDED, f'{SEAM} declares what port.c and a machine\'s overlay give each '
                         'other, and another source includes it: declare what that source reaches in port.h')

    def test_make_dc_and_the_slice_compile_each_source_that_defines_one_and_shc_none(self):
        # config/compilers.mk's NONMATCHING_C is the game's C that make dc adds to 1ST_READ.BIN and the native slice
        # compiles with it, which the SHC routes leave out
        status, output = database()
        self.assertIn(status, (0, 1), output[-3000:])
        listed = re.search(r'(?m)^NONMATCHING_C := (.*)$', output)[1].split()
        objects = re.search(r'(?m)^R32_GAME_OBJ := (.*)$', output)[1].split()
        for source in sorted(set(DEFINED.values())):
            with self.subTest(source=source):
                self.assertIn(source, listed, f"config/compilers.mk's NONMATCHING_C lacks {source}, so make dc and "
                              'the native slice compile no definition of what it defines: add it there')
                self.assertNotIn(f'build/{source[:-2]}.obj', objects)

    def test_the_slice_wraps_neither_call_that_asks_the_mode(self):
        # a wrap of sa_present_window or sa_present_mode would hand the game's ask to the adapter first, which then
        # keeps a mode of its own
        lists = re.findall(r'(?m)^NATIVE_WRAPS := ((?:.*\\\n)*.*)$', (ROOT / 'engine/mk/native.mk').read_text())
        self.assertEqual(len(lists), 1, lists)
        wraps = ' '.join(lists)
        self.assertEqual([name for name in wraps.replace('\\\n', ' ').split() if name.startswith('sa_present')], [],
                         "engine/mk/native.mk's NATIVE_WRAPS wraps a call that asks the mode: "
                         'frame_end takes the window it asked and the rate applied, so the adapter needs neither')


if __name__ == '__main__':
    unittest.main()
