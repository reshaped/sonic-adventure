"""The rule gcc/src/system/port.h states for 16:9: who may read each aspect, that the game's stays 1, and where the 2D
draws' cover rule lies and their pivot and cover are put back."""

import subprocess
import unittest
from pathlib import Path

import csource
from support import cwd

ROOT = Path(__file__).resolve().parents[1]
# where GCC's C lies
C_ROOTS = ('src', 'sdk', 'modules', 'events', 'loader', 'gcc')
# each place a name of 16:9's aspects appears on GCC's side, as (file, the function or macro that holds it, or None
# at file scope)
READERS = {
    # the picture's: the draws, and wide_apply
    'port_draw_aspect': {
        ('events/shared/picture.c', 'event_picture_draw'),
        ('gcc/sdk/ninja/njSprite.c', None),
        ('gcc/sdk/ninja/njSprite.c', 'port_wide_squeezes'),
        ('gcc/sdk/ninja/njSprite.c', 'port_wide_x'),
        ('gcc/sdk/ninja/njSprite.c', 'sprite_clip_near'),
        ('gcc/sdk/ninja/njSprite.c', 'sprite_send_2d'),
        ('gcc/sdk/ninja/njSprite.c', 'sprite_send_plane'),
        ('gcc/sdk/ninja/njsprite.h', 'sprite_send_facing_at'),
        ('gcc/src/draw/model.c', 'draw_model_init'),
        ('gcc/src/system/port.h', None),
        ('gcc/src/system/wide.c', 'wide_apply'),
        ('sdk/ninja/njDrawLine3D.c', 'njDrawLine3D'),
        ('sdk/ninja/njModel.c', '_njCalcModel'),
        ('sdk/ninja/njModelE.c', '_njEasyDrawModel'),
        ('sdk/ninja/njModelE.c', 'njSetEasyLight'),
        ('sdk/ninja/njModelS.c', '_njSimpleCalcModel'),
        ('sdk/ninja/njtri_filled.c', 'njtri_filled'),
        ('sdk/ninja/njtri_filled_with_tex.c', 'njtri_filled_with_tex'),
        ('sdk/ninja/njtri_wiredpoly.c', 'njtri_wiredpoly'),
        ('sdk/ninja/njtri_wiredpoly_with_tex.c', 'njtri_wiredpoly_with_tex'),
        ('src/collision/world.c', 'world_land_list_visible'),
    },
    # the window's: wide_apply, and the window's five readers, two through port_view_x
    'port_view_aspect': {
        ('gcc/src/draw/model.c', 'draw_in_view'),
        ('gcc/src/system/port.h', None),
        ('gcc/src/system/wide.c', None),
        ('gcc/src/system/wide.c', 'port_view_x'),
        ('gcc/src/system/wide.c', 'wide_apply'),
        ('modules/shooting/reticle.c', 'RETICLE_LEFT'),
        ('modules/shooting/reticle.c', 'RETICLE_RIGHT'),
        ('src/player/npc.c', 'npc_watch'),
    },
    'port_view_x': {
        ('gcc/src/system/port.h', None),
        ('gcc/src/system/wide.c', 'port_view_x'),
        ('src/enemy/enemy.c', 'enemy_on_screen'),
        ('src/system/util.c', 'draw_on_screen'),
    },
}
# each place the cover rule appears: its declaration and definition, and the two draws that take it for a polygon
# without texture
COVER_RULE = {
    'port_wide_covers': {
        ('gcc/sdk/ninja/njSprite.c', 'port_wide_covers'),
        ('gcc/src/system/port.h', None),
        ('sdk/ninja/njDraw.c', 'njDrawPolygon'),
        ('sdk/ninja/njDrawPolygon2D.c', 'njDrawPolygon2D'),
    },
}
# the 2D draws' pivot and cover, each with the value that puts it back
PUT_BACK = {'port_wide_anchor': 'WIDE_MIDDLE', 'port_wide_cover': 'FALSE'}
# the functions that put back a pivot and a cover kept before, which is no set of either
KEEPERS = {('gcc/src/system/present.c', 'present_wide'), ('gcc/src/system/present.c', 'state_put')}
# the operators that assign to the name before them
ASSIGNMENTS = {'=', '*=', '/=', '+=', '-=', '++', '--'}
# the statements a break leaves, and those a continue leaves
BREAKS = {'for', 'while', 'do', 'switch'}
LOOPS = {'for', 'while', 'do'}


def sources(names):
    """The C files git tracks under C_ROOTS whose text holds one of names."""
    listed = subprocess.run(['git', 'ls-files', '-z', '--', *C_ROOTS], stdout=subprocess.PIPE, check=True).stdout
    found = []
    for path in listed.decode().split('\0'):
        if path.endswith(('.c', '.h')) and Path(path).is_file():
            text = Path(path).read_bytes()
            if any(name.encode() in text for name in names):
                found.append(path)
    return found


def places(path, compiler, names):
    """Each token of path's C for compiler that names one of names, as (token, the token after it, the function or
    the macro that holds it, or None at file scope)."""
    c = csource.parse(csource.code(path, compiler))
    spans = [(block.head[0].start if block.head else block.opening.start,
              block.closing.end if block.closing else len(c.joined), block.name) for block in c.blocks]
    defines = [(directive.start, directive.end, directive.words[0].text) for directive in c.directives
               if directive.name == 'define' and directive.words]
    tokens = [token for token in c.tokens if token.kind != 'comment']
    for i, token in enumerate(tokens):
        if token.kind == 'name' and token.text in names:
            holder = next((name for start, end, name in spans + defines if start <= token.start < end), None)
            yield token, tokens[i + 1] if i + 1 < len(tokens) else None, holder


def bodies(path, compiler):
    """Each function of path's C for compiler, as (its name, the tokens of its body)."""
    c = csource.parse(csource.code(path, compiler))
    tokens = [token for token in c.tokens if token.kind != 'comment']
    for block in c.blocks:
        if block.name is not None and block.closing is not None:
            yield block.name, [token for token in tokens if block.opening.start <= token.start < block.closing.end]


def opener(tokens, i):
    """The statement whose body the brace at tokens[i] opens, where it is one of BREAKS, or None."""
    if i > 0 and tokens[i - 1].text == 'do':
        return 'do'
    if i > 0 and tokens[i - 1].text == ')':
        depth = 0
        for j in range(i - 1, 0, -1):
            depth += {')': 1, '(': -1}.get(tokens[j].text, 0)
            if depth == 0:
                return tokens[j - 1].text if tokens[j - 1].text in BREAKS else None
    return None


def unbalanced(tokens):
    """The lines in tokens, a function's body, where a set of the pivot or the cover is open: at a return or a goto,
    at a break or a continue that leaves a block the set lies in, and at the body's end. A set is any assignment but
    the put back, += and a postfix ++ too. A put back closes a set only in the set's block or one around it, a set
    stays open past the end of its block, and a set while one is open keeps the open one's block."""
    found = []
    blocks = []
    # each set open, by name: its line, and how many of blocks lie around it
    open_sets = {}
    for i, token in enumerate(tokens):
        if token.text == '{':
            blocks.append(opener(tokens, i))
        elif token.text == '}':
            blocks.pop()
            open_sets = {name: (line, min(depth, len(blocks))) for name, (line, depth) in open_sets.items()}
        elif token.text in PUT_BACK and tokens[i + 1].text in ASSIGNMENTS:
            if [t.text for t in tokens[i + 1:i + 4]] != ['=', PUT_BACK[token.text], ';']:
                open_sets.setdefault(token.text, (token.line, len(blocks)))
            elif token.text in open_sets and len(blocks) <= open_sets[token.text][1]:
                del open_sets[token.text]
        elif token.text in ('return', 'goto') and open_sets:
            found.append(token.line)
        elif token.text in ('break', 'continue') and open_sets:
            left = BREAKS if token.text == 'break' else LOOPS
            inner = max((k for k, kind in enumerate(blocks) if kind in left), default=-1)
            if any(inner < depth for _, depth in open_sets.values()):
                found.append(token.line)
    return found + sorted(line for line, _ in open_sets.values())


@unittest.skipUnless((ROOT / '.git').exists(), 'not a git checkout, so no list of the files it tracks')
class Rule(unittest.TestCase):
    def found(self, names):
        """Each place a name of names appears on GCC's side, by name."""
        found = {name: set() for name in names}
        with cwd(ROOT):
            for path in sources(names):
                for token, _, holder in places(path, csource.GCC, names):
                    found[token.text].add((path, holder))
        return found

    def test_each_aspect_appears_only_where_port_h_says(self):
        found = self.found(READERS)
        for name, readers in READERS.items():
            with self.subTest(name=name):
                self.assertEqual(found[name], readers)

    def test_the_cover_rule_lies_only_in_the_two_draws(self):
        self.assertEqual(self.found(COVER_RULE), COVER_RULE)

    def test_each_set_of_the_pivot_or_the_cover_is_put_back_in_its_function(self):
        found = set()
        with cwd(ROOT):
            for path in sources(PUT_BACK):
                for name, tokens in bodies(path, csource.GCC):
                    if (path, name) not in KEEPERS:
                        found.update((path, name, line) for line in unbalanced(tokens))
        self.assertEqual(found, set())

    def test_only_njinitglobal_assigns_ninjas_aspect_under_gcc(self):
        name = '_nj_scr_aspect_x_'
        found = set()
        with cwd(ROOT):
            for path in sources([name]):
                for token, after, holder in places(path, csource.GCC, {name}):
                    if after is not None and after.text in ASSIGNMENTS:
                        found.add((path, holder))
        self.assertEqual(found, {('sdk/ninja/njSystem.c', 'njInitGlobal')})


if __name__ == '__main__':
    unittest.main()
