#!/usr/bin/env python3
"""The native slice's pass over the game's C.

marks.py tree DIR writes src/, modules/ and events/'s C and headers to DIR,
leaving the tree as it is, with SITES's text replaced and CONSOLE's guards
taken at their matching side, and lists in DIR's overlaid.txt the structs a
macro lays over a task's records, and those a data file holds. It removes
from DIR a copy of a source the tree holds no longer, so no include finds one.

marks.py mark DIR FILE.i marks a source's preprocessed C in place, once make
dc's float pass has read it, as that pass can't read __ptr32. marks.py ninja
FILE.i marks Ninja's models and motions alone, for the adapter's C.

963 macros lay one kind's struct over a task's vars, work, motion, force or
extra, and they agree with one another only where a pointer holds 4 bytes
(ENGINE.md#native-game). The pass marks each pointer member of every
struct such a macro or a cast names, of each struct one of those holds whole,
of the records themselves, and of the casts that park a pointer in another
type's field, so those words keep the SH-4's layout on a 64-bit host and
anything they point at lives in the 4 GB refs.py's domain gives the game.
The collision meshes keep the layout too, since mesh.src walks them, and so
does a heap block's header, since the heap splits blocks to 16 bytes, which a
wider one overruns. So does a struct a data file holds, which the game reads
whole to where make dc's link places it: its pointers are the SH-4's, into
the file where it lies in the domain. The sources that need more than a mark
are SITES's and MARKED_SITES's. A file that doesn't change keeps its time, so
make rebuilds only what the tree changed.

Each rewrite says how many times its text stands, and the pass ends where the
sources give another count. One of SITES's stands once in its source, or as
many times as a third field gives. One of MARKED_SITES's or HOOKS's stands in
a unit's preprocessed C that many times for each of its sources the unit
reads, as the compiler's line markers name them, and in no other unit. CONSOLE
gives each guard's count. A source of the tree names the Dreamcast's backend
only as BACKEND spells it. The tree's step ends at a source one of these
tables names that the tree lacks, and at a struct RECORDS or FILES names that
no source defines.

A mark the pass misses leaves a record wider than the SH-4's, which no count
of texts shows. LAYOUTS gives the SH-4's bytes of the records the pass marks
by name, and either command ends a unit with the compiler's own assertion of
them, for the records of each source the unit reads: mark of every such
record, and ninja of Ninja's alone, as a unit of the adapter's holds the
game's own records as the host lays them.

The slice compiles the non-matching side of each guard, as make dc does, so
the PC builds what make dc builds. The SH-4's own instructions stand under
DREAMCAST, which no host defines, but for CONSOLE's guards: the console's
rewrites of shipped C that keep a matching side, which the host takes. The
game's C names the Dreamcast's backend where it calls the engine, by its
header and its handle, and the tree names the PC's in their place.

A static table's 32-bit word that holds an address is no constant a 64-bit
host's link can make. The pass gives each such word a mark of its own, which
a constructor puts the address in over as the program starts.

A constant the float pass leaves unsuffixed is a double, which -double=float
makes a float, so the pass names it one"""
import re
import sys
from pathlib import Path

ROOTS = 'src', 'modules', 'events'
# what the game's C reaches by a path from its own directory outside the
# roots: port.h's include of the Dreamcast engine's declarations, which the
# PC gives too
REACHED = 'engine/dc/dreamcast.h',
MARK = '__ptr32 __uptr'
OVERLAY = re.compile(r'#define\s+\w+\(\w+\)\s+\(\(struct (\w+)\s*\*\)\s*&?\(\w+\)->(?:work|vars|motion|force|extra)\)')
# a struct a cast lays over a task's records in place
CAST = re.compile(r'\(\s*struct (\w+)\s*\*\s*\)\s*&?\s*\(?\s*\w+\)?\s*->\s*(?:work|vars|motion|force|extra)\b')
# a struct's definition, whose body holds no brace at a line's start: a
# typedef's ends at one, and its match goes no further
STRUCT = re.compile(r'(struct (\w+)\s*\{)((?:(?!\n\}).)*?)(\n\};)', re.S)
FUNCTION = re.compile(r'\(\s*\*\s*(\w+)\s*\)')
POINTER = re.compile(r'\*\s*(\w+)(\s*[;,\[])')
# a member that holds a struct whole, and one that holds an array of pointers
MEMBER = re.compile(r'^\s*struct (\w+)\s+\w+\s*(?:\[[^\]]*\]\s*)*;', re.M)
POINTERS = re.compile(r'\*\s*\w+\s*\[')
COMMENT = re.compile(r'/\*.*?\*/', re.S)
PARKED = re.compile(r'\*\(\s*((?:struct\s+)?\w+)\s*\*\*\s*\)\s*&')
RECORDS = {'task_work', 'task_motion', 'task_force', 'polygon', 'collision_mesh', 'heap_block', 'player'}
# the structs a data file holds, which the game reads whole to where make
# dc's link places it, so the file's pointers are the SH-4's into itself and
# references as they stand: a field's message file, at the anchor the memory
# contract gives it, and Sky Chase's paths, at EXTRA_AREA. the Mystic Ruins'
# paths read to the heap, whose offsets crowd.c makes pointers there
FILES = {'field_messages', 'script_row', 'script_page', 'shooting_scene', 'walk_path'}
# Ninja's models and motions, which a bank file holds in the SH-4's layout, as
# Zero's and E-101 mkII's do, by the typedef's name
NINJA = {'NJS_OBJECT', 'NJS_MODEL', 'NJS_MESHSET', 'NJS_MOTION', 'NJS_ACTION', 'NJS_MDATA1', 'NJS_MDATA2',
         'NJS_MDATA3', 'NJS_MDATA4', 'NJS_MKEY_P'}
TYPEDEF = re.compile(r'(typedef struct\s*\w*\s*\{)((?:(?!\n\}).)*?)(\n\}\s*(\w+)\s*;)', re.S)

# a rewrite is (the text, what stands in its place), and a third field where
# the text stands another count of times than once
SITES = {
    # clang's backend can't select a conditional that mixes the widths
    'src/player/e102_shot.c': [
        ('shot->player != NULL ? shot->player :', 'shot->player != NULL ? (struct task_work *)shot->player :'),
    ],
    # a repair's INFINITY, which make dc's finds through mathf.h's SH4ZAM
    'src/path/path.c': [('best = INFINITY;', 'best = __builtin_inff();')],
    # the vsync is the host's call: each wait runs one until it's over
    'src/system/frame.c': [
        ('    while (frame_countdown > 0) {\n    }\n',
         '    while (frame_countdown > 0) {\n        native_vsync();\n    }\n'),
        ('    while (frame_countdown > left) {\n    }\n',
         '    while (frame_countdown > left) {\n        native_vsync();\n    }\n'),
    ],
    # a hook stands at each task's flags, parent, alloc and destroy, and at
    # each free, which a thread's record hears of too. the heap lies where
    # stubs.py places heap_arena, make dc's address in the domain
    'src/system/task.c': [
        ('Uint8 heap_arena[HEAP_SIZE + sizeof(struct heap_block)]\n    __attribute__((aligned(HEAP_BOUNDARY)));\n',
         ''),
        ('void task_destroy(struct task *task)\n{\n    struct task *next;\n    struct task *prev;\n\n',
         'void task_destroy(struct task *task)\n{\n    struct task *next;\n    struct task *prev;\n\n'
         '    native_hook_task_destroy(task);\n    native_task_destroy(task);\n'),
        # the adapter keys a draw to the task whose step runs
        ('                present_run(task, step);\n#else', '                native_task_step(task, step);\n#else'),
        ('void task_draw_level(int level)\n{\n    struct task *task;\n    struct task *next;\n'
         '    void (*draw)(struct task *task);\n\n',
         'void task_draw_level(int level)\n{\n    struct task *task;\n    struct task *next;\n'
         '    void (*draw)(struct task *task);\n\n    native_draw_level(level);\n'),
        ('        present_draw(task, draw);\n#else', '        native_task_draw(task, draw);\n#else'),
        ('        present_draw(child, draw);\n#else', '        native_task_draw(child, draw);\n#else'),
        ('    task = task_alloc(level, step);\n',
         '    native_hook_task_flags(flags);\n    task = task_alloc(level, step);\n'),
        ('    task = task_free_list;\n    if (task == NULL) {\n        return NULL;\n    }\n    none = NULL;\n',
         '    task = task_free_list;\n    native_hook_task_alloc(task, level, step, task_current_root_step);\n'
         '    if (task == NULL) {\n        return NULL;\n    }\n    none = NULL;\n'),
        ('    if (mem == NULL) {\n        return;\n    }\n    block = mem;\n',
         '    if (mem == NULL) {\n        return;\n    }\n    native_thread_freed(mem);\n'
         '    native_hook_heap_free(mem);\n    block = mem;\n'),
        ('    child = task_create(flags, TASK_UNLINKED, step);\n',
         '    native_hook_task_parent(parent);\n    child = task_create(flags, TASK_UNLINKED, step);\n'),
        ('if ((child = task_create(create, TASK_UNLINKED, step), child != NULL)) {',
         'if ((native_hook_task_parent(parent), child = task_create(create, TASK_UNLINKED, step), child != NULL)) {'),
        # the host's tasks and block headers take twice the SH-4's bytes, and the
        # heap twice its room, which stubs.py's heap_arena holds
        ('#define HEAP_SIZE 0x90000', '#define HEAP_SIZE 0x120000'),
    ],
    # the adapter hears of the land the land's task takes, as the task is
    # about to draw it, and of the land it lets go: the act's scene
    'src/collision/world.c': [
        ('    sound_end_act();\n    world_load_act(world_stage, world_act);\n}\n',
         '    sound_end_act();\n    world_load_act(world_stage, world_act);\n'
         '    native_land_loaded(world_land, STAGE_ACT(world_stage, world_act), world_land_material);\n}\n'),
        ('void world_change_act(Sint16 by)\n{\n    world_release_act(world_stage, world_act);\n',
         'void world_change_act(Sint16 by)\n{\n    native_land_released();\n'
         '    world_release_act(world_stage, world_act);\n'),
        ('static void world_land_destroy(struct task *task)\n{\n    world_release_act(world_stage, world_act);\n',
         'static void world_land_destroy(struct task *task)\n{\n    native_land_released();\n'
         '    world_release_act(world_stage, world_act);\n'),
    ],
    # a sources file runs on past light_sources into the statics after them,
    # which only make dc's one object keeps in order. the names Mystic Ruins
    # and Casinopolis read go where make dc's link puts them, in the host's
    # spelling, which a top-level asm of clang's takes with no operands and
    # an object no common symbol. COFF types and sizes no symbol
    'src/save/card.c': [
        ('    char dat_8c6b9168[4096];\n} sources_file;', '    char dat_8c6b9168[4096];\n} sources_file __attribute__((nocommon));'),
        ('__asm__("\\t.global\\t_light_ry\\n"\n        "\\t.type\\t_light_ry, @object\\n"\n'
         '        "\\t.size\\t_light_ry, 4\\n"\n        "\\t.set\\t_light_ry, %a0\\n"\n'
         '        "\\t.global\\t_light_rz\\n"\n        "\\t.type\\t_light_rz, @object\\n"\n'
         '        "\\t.size\\t_light_rz, 4\\n"\n        "\\t.set\\t_light_rz, %a1\\n"\n'
         '        :\n        : "i"(&light_ry), "i"(&light_rz));\n',
         '__asm__("\\t.global\\tlight_ry\\n\\t.set\\tlight_ry, sources_file + 0x5a0\\n"\n'
         '        "\\t.global\\tlight_rz\\n\\t.set\\tlight_rz, sources_file + 0x5a4\\n");\n'
         '#ifdef __ELF__\n'
         '__asm__("\\t.type\\tlight_ry, @object\\n\\t.size\\tlight_ry, 4\\n"\n'
         '        "\\t.type\\tlight_rz, @object\\n\\t.size\\tlight_rz, 4\\n");\n'
         '#endif\n'),
    ],
    # a hook stands at the main loop's frame's end and after its wait, at a
    # stage's start, and at each stage step's end
    'src/main.c': [
        ('        frame_wait();\n    }\n    {\n',
         '        native_hook_frame_end();\n        frame_wait();\n        native_hook_frame_waited();\n'
         '    }\n    {\n'),
    ],
    'src/stage/stage.c': [
        ('static void stage_start(void)\n{\n', 'static void stage_start(void)\n{\n    native_hook_stage_start();\n'),
        # the stage's loop steps it at two places
        ('        stage_run_step();\n#ifdef NONMATCHING\n        present_step();\n',
         '        stage_run_step();\n        native_hook_stage_step();\n#ifdef NONMATCHING\n        present_step();\n',
         2),
    ],
    # texture_load_pvm writes each texture's address into its texlist's
    # names, and sprite_bind each frame's texture into its frames, which the
    # console doesn't keep from a write and a host's read-only data does
    'src/player/e102_hit.c': [(f'static const {kind}', f'static {kind}') for kind in (
        'NJS_TEXNAME e102hit_textures[17]', 'struct sprite_frame hit_flash_frames[17]',
        'struct sprite_frame hit_spark_frames[5]')],
    # a hook stands round each sound call, which bare.c defines by the
    # game's name: the game's own takes its name after native_real_
    'src/sound/sound.c': [(f'{kind} {name}(', f'{kind} native_real_{name}(') for kind, name in (
        ('int', 'sound_play'), ('int', 'sound_play_at'), ('int', 'sound_play_held'), ('int', 'sound_play_for'),
        ('int', 'sound_play_at_for'), ('int', 'sound_play_around'), ('void', 'sound_stop'), ('void', 'sound_stop_id'),
        ('void', 'sound_set_volume'), ('void', 'sound_set_pan'), ('void', 'sound_set_pitch'), ('void', 'sound_mute'),
        ('void', 'sound_unmute'))],
    # and round each load of a file, and one at each piece of music or voice
    # the game asks for, and at each PVM's load
    'src/sound/stream.c': [
        ('void stream_request_music(int music)\n{\n',
         'void stream_request_music(int music)\n{\n    native_hook_music_request(music);\n'),
        ('void stream_request_voice(int voice)\n{\n',
         'void stream_request_voice(int voice)\n{\n    native_hook_voice_request(voice);\n'),
    ],
    'src/system/loader.c': [(f'int {name}(char *name, void *dest)\n{{',
                             f'int native_real_{name}(char *name, void *dest)\n{{')
                            for name in ('loader_read_prs', 'loader_read_file')],
    'src/system/texture.c': [('void texture_load_pvm(char *name, NJS_TEXLIST *texlist)\n{\n',
                              'void texture_load_pvm(char *name, NJS_TEXLIST *texlist)\n{\n'
                              '    native_hook_texture_load(name);\n')],
    # only the memory contract names an event's head, and the shim reads it
    'events/shared/head.h': [('static struct event_module EVENT_HEAD = {EVENT_ENTRY',
                              'static struct event_module EVENT_HEAD __attribute__((used)) = {EVENT_ENTRY')],
}


# what only a host with wider pointers needs in a unit's preprocessed C, by
# the source that holds each text. a text two sources hold has one name here
HOLDER = ('struct task *parent, struct task **holder', f'struct task *parent, struct task *{MARK} *holder')
MATRIX = ('NJS_MATRIX *_nj_current_matrix_ptr_', f'NJS_MATRIX *{MARK} _nj_current_matrix_ptr_')
BLEND = ('    void **data = (void **)blend->mdata;', f'    void *{MARK} *data = (void *{MARK} *)blend->mdata;')
MARKED_SITES = {
    # a task's twelve bytes of variables, which each kind lays out its own
    # way: a union's, which no struct's mark reaches
    'src/system/task.h': [('    void *p[3];\n};', f'    void *{MARK} p[3];\n}};')],
    # a holder is a task's own record, whose word is marked
    'src/object/tikal_light.h': [HOLDER],
    'src/object/tikal_light.c': [
        (f'struct task **{MARK} holder;', f'struct task *{MARK} *{MARK} holder;'),
        HOLDER,
    ],
    # struct item's a table too, and __ptr32 takes no static initializer's
    # address
    'src/object/item_box.h': [(f'void (*{MARK} give)', 'void (*give)')],
    'sdk/include/ninja.h': [
        # create_light keeps a light's words as the SH-4 lays them
        ('NJS_LIGHT *light[32];', f'NJS_LIGHT *{MARK} light[32];'),
        # the matrix stack's top, which the assembly moves as a word of its own
        MATRIX,
    ],
    'sdk/ninja/njGlobal.c': [MATRIX],
    'sdk/include/njtypes.h': [
        ('    void *func;\n    Angle iang, oang;', f'    void *{MARK} func;\n    Angle iang, oang;'),
        # where a motion stands and where a motion link does, by Ninja's
        # layout: the keys of a node by factor
        ('    void **p;\n    Uint32 *nb;', f'    void *{MARK} *p;\n    Uint32 *nb;', 2),
    ],
    # a camera motion's record of five factors, which no typedef of Ninja's
    # names, and the local that njCameraAction and njCameraMotion each point
    # at a record's keys
    'sdk/ninja/njCameraMotion.c': [
        ('    void *p[5];\n    Uint32 nb[5];', f'    void *{MARK} p[5];\n    Uint32 nb[5];'),
        ('    void **keys;\n    void *data;', f'    void *{MARK} *keys;\n    void *data;', 2),
    ],
    # a blend's keys, which motion_blend_free and player_anim_free_blend step
    # word by word
    'src/draw/motion.c': [BLEND],
    'events/shared/motion.c': [BLEND],
    'src/player/player.c': [
        ('    void **keys = (void **)motion->mdata;', f'    void *{MARK} *keys = (void *{MARK} *)motion->mdata;'),
    ],
    # a message file's runs of lines, which script.c walks, by its
    # declaration and its definition
    'src/event/script.h': [('    char **lines;\n    Sint32 flag;', f'    char *{MARK} *lines;\n    Sint32 flag;')],
    'src/event/script.c': [
        ('static void run_lines(char **lines)', f'static void run_lines(char *{MARK} *lines)', 2),
        ('(char **)script_run(', f'(char *{MARK} *)script_run('),
        ('    struct script_page *page;\n    char **line;', f'    struct script_page *page;\n    char *{MARK} *line;'),
        ('line = (char **)run;', f'line = (char *{MARK} *)run;'),
    ],
    # Sky Chase's path file's first word, which points at its scenes
    'modules/shooting/shooting.c': [
        ('static struct shooting_scene **shooting_scene_file =\n    (struct shooting_scene **)0x0cb80000;',
         f'static struct shooting_scene *{MARK} *shooting_scene_file =\n'
         f'    (struct shooting_scene *{MARK} *)0x0cb80000;'),
    ],
}
# the adapter's hooks, on any host: Ninja's drawers take these names, and
# engine/sa1/ninja.c's functions the names they had, so a call of a drawer
# reaches the adapter, which records the model and calls the drawer
HOOKS = {
    'sdk/ninja/njModelE.c': [
        ('void _njEasyDrawModel(NJS_MODEL *model)\n{', 'void ninja_easy_draw_model(NJS_MODEL *model)\n{'),
    ],
    'sdk/ninja/njModelS.c': [
        ('void _njSimpleDrawModelSub(NJS_MODEL *model)\n{', 'void ninja_simple_draw_model(NJS_MODEL *model)\n{'),
    ],
}
# the console's rewrites of shipped C in the SH-4's own instructions that
# keep a matching side: the collision index's searches, the ball's sweep and
# the near list's walk. the host takes each one's matching side. a guard is
# known by its first line of code past its #ifdef, whatever comment opens it,
# with the count of guards that open so
CONSOLE = {
    'src/collision/polygon.c': {
        '    collision_mesh_pool_init_indexed(mesh_pool, MESHES);': 2,
        '            triangles = strip.count - 2;': 1,
        '#define NEAR_END (near_polygons + NEAR_POLYGONS)': 1,
        '                found = mesh_find(entry->object)->era == mesh_finds_era;': 1,
        '            entry = mesh_find(mesh->object)->first;': 1,
        '                collision_mesh_free_indexed(mesh);': 1,
        '            collision_mesh_take_buckets_indexed(mesh);': 1,
        '#define BOXES_FEWEST 16': 1,
    },
}
# a NONMATCHING guard's opening: the comments and blank lines past its
# #ifdef, then its first line of code
GUARD = re.compile(r'#ifdef NONMATCHING\n((?:[ \t]*/\*.*?\*/[ \t]*\n|[ \t]*\n)*)([^\n]*)\n', re.S)
# the Dreamcast backend's header and handle, where the game calls the
# engine, and the PC's, play's engine or null
BACKEND = [(re.compile(r'"sa_dc\.h"'), '"sa_pc.h"'), (re.compile(r'\bSA_DC_ENGINE\b'), 'native_engine')]
# the header in any spelling, which the code of a source BACKEND has read
# holds none of
DREAMCAST_HEADER = re.compile(r'\bsa_dc\.h\b')
# a source a unit's preprocessed C holds, as the compiler's line marker
# names it
MARKER = re.compile(r'^# \d+ "([^"]+)"', re.M)
# a struct's definition, by its head
DEFINED = re.compile(r'\bstruct (\w+)\s*\{')
# the SH-4's bytes of the records the pass marks by name, by the source that
# defines each: a task's, which each kind's struct lies over, a heap block's
# header, the collision meshes', a data file's structs and Ninja's models
# and motions, with a camera motion's record of five factors. a unit that
# reads the source ends with the compiler's assertion of them, so a member
# the marks miss fails the unit
LAYOUTS = {
    'src/system/task.h': {'union task_vars': 12, 'struct task_work': 64, 'struct task_motion': 64,
                          'struct task_force': 28},
    'src/system/task.c': {'struct heap_block': 16},
    'src/collision/polygon.h': {'struct polygon': 64, 'struct collision_mesh': 64},
    'src/event/script.h': {'struct script_page': 8, 'struct script_row': 8, 'struct field_messages': 8},
    'modules/shooting/shooting.h': {'struct shooting_scene': 16},
    'modules/adv02/crowd.c': {'struct walk_path': 8},
    'sdk/include/njtypes.h': {'NJS_OBJECT': 52, 'NJS_MODEL': 40, 'NJS_MESHSET': 24, 'NJS_MOTION': 12, 'NJS_ACTION': 8,
                              'NJS_MDATA1': 8, 'NJS_MDATA2': 16, 'NJS_MDATA3': 24, 'NJS_MDATA4': 32, 'NJS_MKEY_P': 8},
    'sdk/ninja/njCameraMotion.c': {'struct motion_data5': 40},
}
# a static initializer's address in a 32-bit word, and the mark that stands in
ADDRESS32 = re.compile(r'\(Sint32\)\s*')
OPERAND = re.compile(r'((?:\(\s*)*)(&?)\s*((?:\(\s*)*)([A-Za-z_]\w*(?:\s*\[[^\]]*\]|\s*\.\s*\w+)*)')
ADDRESS_MARK = 0x5a1a0000
LITERAL = re.compile(r'"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'')
# what a unit holds that is no code, whichever opens first: a comment, a
# string or a character. a string that holds a comment's opening opens none
NOT_CODE = re.compile(f'(?s:{COMMENT.pattern})|{LITERAL.pattern}')
# a line of the preprocessor's own, as a line marker is, which lies between
# two tokens as space does
DIRECTIVE = re.compile(r'^#.*', re.M)
LONG_FLOAT = re.compile(r'\blong\s+float\b')
# a constant the float pass leaves unsuffixed, whole: no part of a name
UNSUFFIXED = re.compile(r'(?<![\w.])(0x[0-9a-fA-F.]+p[-+]?[0-9]+)(?![0-9a-zA-Z.])')


def mark_lines(body):
    """body with its members' pointers marked, its comments left alone"""
    comments = []

    def hide(m):
        comments.append(m[0])
        return f'\0{len(comments) - 1}\0'

    code = '\n'.join(mark_code(line) for line in COMMENT.sub(hide, body).split('\n'))
    return re.sub(r'\0(\d+)\0', lambda m: comments[int(m[1])], code)


def mark_code(code):
    if '(' in code and FUNCTION.search(code):
        return FUNCTION.sub(lambda m: f'(*{MARK} {m[1]})', code)
    return POINTER.sub(lambda m: f'*{MARK} {m[1]}{m[2]}', code)


def ninja(text):
    """text with Ninja's models and motions marked"""
    return TYPEDEF.sub(lambda m: m[1] + (mark_lines(m[2]) if m[4] in NINJA else m[2]) + m[3], text)


def mark(text, names):
    text = STRUCT.sub(lambda m: m[1] + (mark_lines(m[3]) if m[2] in names else m[3]) + m[4], text)
    text = ninja(text)
    return PARKED.sub(lambda m: f'*({m[1]} *{MARK} *)&', text)


def blank(text):
    """text with its comments, strings and characters as spaces, its lines kept"""
    return NOT_CODE.sub(lambda m: re.sub(r'[^\n]', ' ', m[0]), text)


def in_code(text, rewrite):
    """text with each run of its code as rewrite gives it, and its comments,
    strings and characters as they stand"""
    out, last = [], 0
    for m in NOT_CODE.finditer(text):
        out += [rewrite(text[last:m.start()]), m[0]]
        last = m.end()
    return ''.join(out) + rewrite(text[last:])


def console(path, text):
    """text, path's, with each of CONSOLE's guards for it taken at its
    matching side"""
    wanted = CONSOLE.get(path, {})
    found = dict.fromkeys(wanted, 0)

    def take(m):
        if m[2] not in wanted:
            return m[0]
        found[m[2]] += 1
        return f'#if 0\n{m[1]}{m[2]}\n'

    text = GUARD.sub(take, text)
    for first, count in wanted.items():
        if found[first] != count:
            sys.exit(f'{path}: {found[first]} guards open with this code, where {count} should: {first.strip()}')
    return text


def whole(code, start, end):
    """whether code[start:end] is all of one initializer but the parentheses
    round it: a brace, a comma or a designator's = stands before it, and a
    comma or a brace after"""
    outer = 0
    while code[start - 1].isspace() or code[start - 1] == '(':
        outer += code[start - 1] == '('
        start -= 1
    if code[start - 1] not in '{,' and (code[start - 1] != '=' or code[start - 2] in '=!<>'):
        return False
    return bool(re.compile(r'(?:\s*\)){%d}\s*[,}]' % outer).match(code, end))


def addresses(text):
    """text with each static initializer's address in a 32-bit word marked,
    and a constructor that writes the addresses over the marks. an address
    that is part of a longer expression stays, which the compiler refuses"""
    if not ADDRESS32.search(text):
        return text
    code = blank(text)
    # the same with each line of the preprocessor's own as spaces, for where
    # an initializer starts and ends
    flat = DIRECTIVE.sub(lambda m: ' ' * len(m[0]), code)
    spans, depth, start = [], 0, None
    for i, c in enumerate(code):
        if c == '{':
            if depth == 0 and code[:i].rstrip().endswith('='):
                start = i
            depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0 and start is not None:
                spans.append((start, i))
                start = None
    writes, out, last = [], [], 0
    for start, end in spans:
        found = []
        for cast in ADDRESS32.finditer(code, start, end):
            m = OPERAND.match(code, cast.end())
            if not m:
                continue
            opened = m[1].count('(') + m[3].count('(')
            close = re.compile(r'(?:\s*\)){%d}' % opened).match(code, m.end())
            # a cast of a call, a member through a pointer, a sum or a side of
            # a choice is no word of an address alone. the operand is the
            # source's, where code holds a character or a string as spaces
            if close and whole(flat, cast.start(), close.end()):
                found.append((cast.start(), close.end(), m[2], text[m.start(4):m.end(4)]))
        if not found:
            continue
        name = re.search(r'(\w+)\s*(?:\[[^\]]*\]\s*)*=\s*$', code[:start])[1]
        for at, end_at, address, operand in found:
            mark = ADDRESS_MARK + len(writes)
            target = f'&({operand})' if address else f'(void *)({operand})'
            writes.append(f'    native_address32(&{name}, sizeof {name}, {mark:#x}, {target});\n')
            out.append(text[last:at] + f'(Sint32){mark:#x}')
            last = end_at
    if not writes:
        return text
    return (''.join(out) + text[last:] + '\nvoid native_address32(void *, unsigned long, int, void *);\n'
            '__attribute__((constructor)) static void native_addresses(void)\n{\n' + ''.join(writes) + '}\n')


def nested(names, sources):
    """names, with each struct a member of one of them holds whole, which lies
    over the same words, and so on down. a struct that holds an array of
    pointers stays the host's: a mark would change the type of every walk of
    the array, which a site gives by hand"""
    bodies = {}
    for text in sources.values():
        for m in STRUCT.finditer(COMMENT.sub('', text)):
            bodies.setdefault(m[2], []).append(m[3])
    names = set(names)
    todo = list(names)
    while todo:
        for body in bodies.get(todo.pop(), ()):
            for inner in MEMBER.findall(body):
                if inner not in names and not any(POINTERS.search(b) for b in bodies.get(inner, ())):
                    names.add(inner)
                    todo.append(inner)
    return names


def tree(dest):
    files = sorted(p for root in ROOTS for p in Path(root).rglob('*.[ch]'))
    sources = {p: p.read_text(errors='surrogateescape') for p in files}
    # a source that has moved, or a struct that has taken another name, would
    # leave its table's entry standing for nothing
    for path in (*SITES, *CONSOLE):
        if Path(path) not in sources:
            sys.exit(f'{path}: SITES or CONSOLE rewrites it, and the tree holds no such source')
    for path in (*MARKED_SITES, *HOOKS, *LAYOUTS):
        if not Path(path).is_file():
            sys.exit(f'{path}: MARKED_SITES, HOOKS or LAYOUTS names it, and the tree holds no such source')
    lost = (RECORDS | FILES) - {m[1] for text in sources.values() for m in DEFINED.finditer(text)}
    if lost:
        sys.exit(f'struct {min(lost)}: RECORDS or FILES marks it, and no source of the tree defines it')
    names = nested(RECORDS | FILES | {m[1] for text in sources.values()
                                      for m in (*OVERLAY.finditer(text), *CAST.finditer(text))}, sources)
    written = 0
    for path, text in sources.items():
        for old, replacement, *count in SITES.get(path.as_posix(), ()):
            text = replaced(path, text, old, replacement, *count)
        text = console(path.as_posix(), text)
        for pattern, replacement in BACKEND:
            text = pattern.sub(replacement, text)
        if DREAMCAST_HEADER.search(COMMENT.sub('', text)):
            sys.exit(f"{path}: names the Dreamcast backend's header, sa_dc.h, in a spelling BACKEND doesn't rewrite")
        written += put(dest / path, text)
    for path in REACHED:
        written += put(dest / path, Path(path).read_text())
    # a copy of a source the tree holds no longer, which an include would
    # still find
    stale = [p for root in ROOTS for p in (dest / root).rglob('*.[ch]') if p.relative_to(dest) not in sources]
    for path in stale:
        path.unlink()
    put(dest / 'overlaid.txt', '\n'.join(sorted(names)) + '\n')
    print(f'{len(names)} overlaid structs, {written} files written, {len(stale)} removed')


def replaced(path, text, old, replacement, count=1):
    """text, path's, with replacement in place of old, which stands in it
    count times"""
    found = text.count(old)
    if found != count:
        sys.exit(f'{path}: holds {found} of a text the pass rewrites {count} of: {old.splitlines()[0]}')
    return text.replace(old, replacement)


def reads(text):
    """whether text, a unit's preprocessed C, reads a source, as a function
    of the source's path. the compiler's line markers name each source the
    unit reads, the tree's copy under the directory make gives it"""
    read = set(MARKER.findall(text))
    return lambda source: any(name == source or name.endswith(f'/{source}') for name in read)


def rewritten(path, text, table):
    """text, the preprocessed C of the unit at path, with each of table's
    texts replaced. a text stands in it as many times as the sources the unit
    reads hold it, by table's counts, and a unit that reads none of them
    holds none"""
    here = reads(text)
    counts = {}
    for source, sites in table.items():
        for old, replacement, *count in sites:
            counts[old, replacement] = counts.get((old, replacement), 0) + (count[0] if count else 1) * here(source)
    for (old, replacement), count in counts.items():
        text = replaced(path, text, old, replacement, count)
    return text


def laid(text, only=None):
    """text, a unit's preprocessed C, and past it the compiler's assertion
    of the bytes LAYOUTS gives each record of a source the unit reads: every
    such record, or those only names"""
    here = reads(text)
    held = [f'_Static_assert(sizeof({record}) == {size}, "{record} isn\'t the SH-4\'s {size} bytes, which '
            f'marks.py\'s LAYOUTS gives it");\n'
            for source, records in LAYOUTS.items() if here(source)
            for record, size in records.items() if only is None or record in only]
    return text + ('' if text.endswith('\n') or not held else '\n') + ''.join(held)


def put(path, text):
    if path.exists() and path.read_text(errors='surrogateescape') == text:
        return 0
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, errors='surrogateescape')
    return 1


def preprocessed(dest, path):
    text = path.read_text(errors='surrogateescape')
    text = mark(text, set((dest / 'overlaid.txt').read_text().split()))
    text = rewritten(path, text, MARKED_SITES)
    text = addresses(text)
    text = rewritten(path, text, HOOKS)
    # SHC's long float is a double. a string that spells the type or a
    # constant is the program's data, and keeps its text
    text = in_code(text, lambda code: UNSUFFIXED.sub(r'\1F', LONG_FLOAT.sub('double', code)))
    path.write_text(laid(text), errors='surrogateescape')


def main():
    args = sys.argv[1:]
    if args[0] == 'tree':
        tree(Path(args[1]))
    elif args[0] == 'ninja':
        path = Path(args[1])
        text = path.read_text(errors='surrogateescape')
        path.write_text(laid(ninja(text), NINJA), errors='surrogateescape')
    else:
        preprocessed(Path(args[1]), Path(args[2]))


if __name__ == '__main__':
    main()
