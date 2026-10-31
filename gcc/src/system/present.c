/* the port's split frame, which the 60 fps switch takes. a frame of play
 * runs its steps as SEGA's 30 fps runs them, and shows two pictures. the
 * first is what step 0 drew, rendered after step 0 and shown at the frame's
 * middle vsync. the second, after the last step, holds what step 1 drew,
 * then draws each other task as SEGA's pause draws it: by its callback, or,
 * where it has none or its callback is its step, by the draws its step made
 * on step 0, moved to the second picture's view. each picture also replays
 * the 2D the other half of the frame drew, and the sprites njDrawSprite3D
 * drew there for a step, moved to the picture's view, so that it holds what
 * the shipped picture holds: njDrawSprite3D draws on every step, shipped
 * too. the second picture's pass draws no such sprite for a task whose step
 * drew one after step 0, since its steps' own stand for it. no picture
 * replays a draw of a task killed or deleted with a destructor, which may
 * release what the draw takes. a picture must change nothing a step reads.
 * the pass keeps rand()'s state, puts back each task's work after its
 * callback, and a player's motion, record and shapes too, and six of SEGA's
 * entries refuse it, as port.h's frame_redrawing says. a callback that writes
 * anything else a step reads still breaks the rule.
 *
 * the second picture's view is the one the last step left, but where the
 * camera moves once a frame. a path camera poses itself on step 0 alone,
 * and the main camera takes the pose on each step, so camera_work moves
 * over the later steps alone, and each pose would show in two pictures.
 * where the last frame's camera moved so and this frame's stood over step
 * 0, step 1's models and sprite_draw_3d's sprites draw nothing, as port.h's
 * FRAME_DRAWS says, since their draws take step 1's view, and the pass
 * draws each task, but for the sprites it refuses. where the camera then
 * moves, the second picture takes the pose halfway between the first's and
 * camera_work's, except across a cut. what step 1 draws past its models and
 * sprite_draw_3d's sprites, njDrawSprite3D's among it, keeps step 1's
 * view. a camera that moved over step 0 alone, as Chaos 4's does while it
 * follows the player, showed its pose in the first picture, and a pose past
 * it would be a guess, so the first picture holds.
 *
 * the shadow functions draw by Ninja's easy draws, which present_record
 * never sees. the second picture takes the shadows the pass's callbacks
 * place, and for each other task those its step placed on step 1, as port.h's
 * present_shadow says.
 *
 * for 16:9, present_run and present_draw also put back the 2D draws' anchor
 * and cover around each task's step and callback, as port.h says.
 *
 * three rules pace it, and it carries two costs between frames. the first
 * picture shows where the middle vsync is still ahead with room, the PVR can
 * take it on time, no bitmap waits for its upload, step 1 and the pass fit the
 * frame's second half, and step 0's records and the last frame's later 2D and
 * sprites all fit. the second shows where the time left covers the pass and
 * its records all fit. a run of frames that fit engages the split, and a run
 * that don't ends it. a frame fits only where the records last kept all fit.
 * only a frame that may split keeps them, so a scene whose records overflow
 * holds the split off until a frame sets the costs back, as a stage's load
 * does. the steps are SEGA's, and timer 1, which times SEGA's extra step, stops
 * across the first picture, across step 1's draws, which shipped skips, and
 * while the split frame keeps a step's draw or forgets a deleted task. the
 * sound server runs at each picture's vsync, and step 0 sends its sounds after
 * the first, so they start a vsync later than at 30 fps. make gcc links this
 * into 1ST_READ.BIN after the image's sources */
#include <kamui.h>
#include <kmistat.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "draw/model.h"
#include "draw/sprite.h"
#include "effect/particle.h"
#include "gcc/sdk/ninja/njsprite.h"
#include "main.h"
#include "player/player.h"
#include "sdk/ninja/njkamui.h"
#include "system/bitmap.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/timer.h"
#include "system/util.h"

/* rand()'s state */
extern unsigned long _rnext;
/* the bitmaps that wait for their upload, which util.c's GCC side gives a
 * name outside util.c */
extern int texture_queue_count;
/* Ninja's current view, which njSetView sets, and its matrix stack's base,
 * which njClearMatrix writes */
extern NJS_VIEW *_nj_view_;
extern NJS_MATRIX *_nj_base_matrix_ptr_;

/* camera_action_state where no camera action gives the view, camera.c's
 * ACTION_NONE */
#define ACTION_NONE 0
/* a move over a frame's later steps past either of these is a cut, which
 * the second picture's pose doesn't span: 48 along an axis, what a player
 * at player.c's limit of 16 a step covers in a frame's three steps at most,
 * or an eighth of a turn about one, chosen, not measured */
#define CUT_MOVE 48.0F
#define CUT_TURN 0x2000

/* _nj_current_texture where no texture is current, as Ninja starts it */
#define NO_CURRENT_TEXTURE 0xffffffff

/* timer 1's start register, and its bit that runs timer 1, as timer.c
 * names them */
#define TSTR (*(volatile Uint8 *)0xffd80004)
#define STR1 0x02

/* the room the first picture's replay and submit take before the middle
 * vsync: a tenth of a vsync */
#define FIRST_ROOM (VSYNC_COUNTS / 10)
/* the room the second half takes past step 1 and the pass, for the hud, the
 * fades, the overlay and the submit: 0.15 of a vsync */
#define SECOND_ROOM (VSYNC_COUNTS * 3 / 20)
/* the room a render keeps past the last render's length, which the next
 * picture's can exceed: a tenth of a vsync */
#define RENDER_ROOM (VSYNC_COUNTS / 10)
/* the pass's and step 1's shares of step 0, in 256ths, that a stage
 * starts from. the pass's is about the middle of those stages learn under
 * Flycast: 134 in Speed Highway, 150 in Twinkle Park, 154 in Emerald Coast,
 * 170 in Chaos 0's fight, 180 in Station Square and 228 in the Egg
 * Walker's. step 1's is the most any of their split frames take at p90,
 * 0.57 of step 0 in Speed Highway, where their medians run from 0.11 in the
 * Egg Walker's fight to 0.52. a whole step 0 for the pass held Emerald
 * Coast at 30 fps, and for step 1 Station Square, though each splits once
 * it learns its own */
#define PASS_SHARE 160
#define STEP1_SHARE 146
/* the frames in a row that fit, which engage the split, and that don't,
 * which end it */
#define ENGAGE_FRAMES 15
#define DROP_FRAMES 3

/* the draws step 0 keeps for the second picture, the sprites the later
 * steps keep for the next frame's first, and the views each list's draws
 * were made under */
#define FIRST_MOST 384
#define LATER_MOST 256
#define VIEWS_MOST 4
/* each half's 2D draws, and the words their vertices take */
#define FLATS_MOST 96
#define FLAT_WORDS 1536
/* the slots for the tasks the second picture marks, one for each value of
 * the MARKS_BITS bits marks_slot hashes a task to */
#define MARKS_BITS 10
#define MARKS_MOST (1 << MARKS_BITS)
/* the most shapes the pass keeps for a player */
#define SHAPES_KEPT 8
/* the most shadows step 1 keeps for the pass */
#define SHADOWS_MOST 128

int frame_redrawing;
int frame_split;
int frame_later_draws;

/* what present_record keeps: nothing, the draws up to step 0's end, or the
 * 2D and njDrawSprite3D's sprites after it */
static enum { KEEP_NONE, KEEP_FIRST, KEEP_LATER } keeping;
/* the task whose step runs, which present_run sets, or whose callback the
 * pass calls, which present_draw sets, or null */
static struct task *present_task;

/* the costs carried between frames, each in 256ths of its frame's step 0:
 * step 1's, from the end of step 0, and the second picture's pass. step 1
 * draws only in a split frame, so only a split frame measures either. each
 * estimate is a share of this frame's step 0, from the end of the last
 * frame's wait, so it falls with step 0 as a heavy scene ends. a frame that
 * doesn't want the split or runs no stage loop, as a stage's load does, sets
 * each back to the share a stage starts from, so each stage learns its own.
 * a frame that drops the split keeps them */
static Uint32 step1_share = STEP1_SHARE;
static Uint32 pass_share = PASS_SHARE;
static Uint32 step0_cost;
/* whether the split is engaged, and the frames in a row that go against
 * it. planned is TRUE where the frame running may split: its switch was on and
 * the split engaged as the last frame ended */
static int engaged;
static int against;
static int planned;
/* TRUE where the last frame that kept records kept them all. a frame keeps
 * them only where it may split, so the frames after the split ends keep the
 * answer, as they keep the costs. a frame that sets the costs back sets it
 * back to TRUE */
static int records_fit = TRUE;
/* this frame's facts: a stage loop ran step 0, it ended with room for the
 * first picture, and it began after a frame that kept its later 2D and
 * sprites */
static int stage_ran;
static int first_fits;
static int later_kept;
/* TRUE where this frame's step 0 found the PVR able to take the first
 * picture on time, and step 0's reason for a held split */
static int render_fits;
static int held_reason;
/* when step 1 began, when the first picture's flip returned, and the
 * counts the flip waited for the middle vsync */
static Uint32 step1_start;
static Uint32 middle;
static Uint32 middle_wait;
/* TRUE where present_second dropped the second picture: where the pass or a
 * record didn't fit, or the camera moved over step 0 alone. present_end hands
 * it to present_hold as holding */
static int dropped;
static int holding;
/* TRUE where a record the second picture needs didn't fit: a mark, a
 * player's shapes, or a shadow */
static int lost;
/* TSTR as the outermost present_untimed on step 1 of a split frame found
 * it, and the begins not yet ended */
static Uint8 untimed_run;
static int untimed_depth;

/* the camera's pose: the work camera_work points at, its angles and
 * position, and the position of the view built from them, which
 * camera_add_offset's shakes move */
struct pose {
    struct task_work *work;
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT3 eye;
};
/* how the camera went from one pose to the next: as another camera or
 * none, or as the same camera, standing or moving */
enum went { WENT_ELSE, WENT_STOOD, WENT_MOVED };
/* the pose the last frame's steps left, and the pose step 0 left */
static struct pose start_pose;
static struct pose first_pose;
/* TRUE where the last frame's camera stood over step 0 and moved over the
 * later steps */
static int moved_later;
/* the second picture's view, from the pose between the pictures' */
static NJS_VIEW between;

/* the Ninja state a draw takes, with the 2D draws' pivot, cover and
 * pillarbox, which a kept draw keeps */
struct ninja_state {
    /* the current context's polygon header words */
    Uint32 header[4];
    Uint32 control;
    Uint32 attr_and;
    Uint32 attr_or;
    NJS_ARGB material;
    NJS_TEXLIST *texlist;
    Uint32 texture;
    Uint32 bank;
    Float anchor;
    Int cover;
    Int pillarbox;
};

/* a draw a picture to come may replay: the task whose step made it, the
 * draw and its attr, the view it was made under, the model and the shading
 * it took or the sprite, its frame and the depth it took, and the state and
 * matrix it took */
struct held {
    struct task *task;
    enum present_kind kind;
    Uint32 attr;
    int view;
    union {
        struct {
            NJS_MODEL *model;
            struct draw_shading shading;
        } model;
        struct {
            NJS_SPRITE sprite;
            NJS_TEXANIM frame;
            Float depth;
        } sprite;
    } what;
    struct ninja_state state;
    NJS_MATRIX matrix;
};

/* a list of such draws: the draws, the most it takes, how many it holds,
 * the views they were made under, and full, TRUE where a draw didn't fit */
struct draws {
    struct held *draw;
    int most;
    int count;
    NJS_MATRIX views[VIEWS_MOST];
    int view_count;
    int full;
};

/* a 2D draw kept for the other picture: the task whose step drew it, or
 * null, the draw, its arguments, a sprite and the frame it draws from its
 * texture animation, where its vertices start in words, and the state it
 * took */
struct flat {
    struct task *task;
    enum present_kind kind;
    Int count;
    Int a;
    Float pri;
    Uint32 attr;
    NJS_SPRITE sprite;
    NJS_TEXANIM frame;
    int words;
    struct ninja_state state;
};

/* a half's 2D draws, and their vertices. full is TRUE where a draw didn't
 * fit */
struct flats {
    struct flat flat[FLATS_MOST];
    int count;
    Uint32 words[FLAT_WORDS];
    int used;
    int full;
};

/* a task the second picture marks: DREW_LATER where its step drew models or
 * sprite_draw_3d's sprites on step 1, HAS_HELD where it has held draws, HOLDS
 * where the pass holds them, DREW_FLAT where its callback drew 2D in the pass,
 * SPRITES_LATER where its step drew njDrawSprite3D's sprites after step 0,
 * DREW_SPRITE where its callback drew some in the pass, and DREW_SHADOW where
 * its callback drew a shadow in the pass */
#define DREW_LATER 0x1
#define HOLDS 0x2
#define HAS_HELD 0x4
#define DREW_FLAT 0x8
#define SPRITES_LATER 0x10
#define DREW_SPRITE 0x20
#define DREW_SHADOW 0x40
static struct {
    struct task *task;
    Uint32 marks;
} marks[MARKS_MOST];
static int marks_used;

/* step 0's draws, which the second picture replays: each of njDrawSprite3D's
 * sprites, and a held task's other draws. the sprites njDrawSprite3D drew after
 * step 0, which the next frame's first picture replays */
static struct held first_held[FIRST_MOST];
static struct held later_held[LATER_MOST];
static struct draws first_draws = {first_held, FIRST_MOST};
static struct draws later_draws = {later_held, LATER_MOST};
/* step 0's 2D, which the second picture replays, and the 2D after step 0,
 * which the next frame's first picture replays */
static struct flats first_flats;
static struct flats later_flats;
/* a shadow step 1 of a split frame placed, and the task whose step placed
 * it, or null */
struct kept_shadow {
    struct shadow_place place;
    struct task *task;
};
/* the shadows step 1 placed, which the pass draws but for a task whose
 * callback drew one */
static struct kept_shadow shadows[SHADOWS_MOST];
static int shadows_count;

/* Ninja's draw state, as a picture the shipped frame doesn't draw finds it
 * and puts it back: the contexts, the view and its matrix, the current
 * matrix, the matrix stack's top and its depth, which njPopMatrix trusts,
 * the state a kept draw keeps, the renderer's shading and depth, and the
 * easy draws' light */
static struct {
    KMVERTEXCONTEXT contexts[NJD_CONTEXT_MAX];
    Uint32 color_modes[NJD_CONTEXT_MAX];
    KMVERTEXCONTEXT *context;
    Uint32 *color_mode;
    NJS_VIEW *view;
    NJS_MATRIX view_matrix;
    NJS_MATRIX matrix;
    NJS_MATRIX *matrix_ptr;
    Sint32 stack_size;
    struct ninja_state state;
    struct draw_shading shading;
    Float depth;
    NJS_VECTOR easy_light;
} saved;

static void state_get(struct ninja_state *state)
{
    const KMVERTEXCONTEXT *context = _nj_current_ctx_;

    state->header[0] = context->GLOBALPARAMBUFFER;
    state->header[1] = context->ISPPARAMBUFFER;
    state->header[2] = context->TSPPARAMBUFFER;
    state->header[3] = context->TexturePARAMBUFFER;
    state->control = _nj_control_3d_flag_;
    state->attr_and = _nj_constant_attr_and_;
    state->attr_or = _nj_constant_attr_or_;
    state->material = _nj_constant_material_;
    state->texlist = _nj_current_texlist;
    state->texture = _nj_current_texture;
    state->bank = _nj_current_texture_bank;
    state->anchor = port_wide_anchor;
    state->cover = port_wide_cover;
    state->pillarbox = port_pillarbox;
}

static void state_put(const struct ninja_state *state)
{
    KMVERTEXCONTEXT *context = _nj_current_ctx_;

    context->GLOBALPARAMBUFFER = state->header[0];
    context->ISPPARAMBUFFER = state->header[1];
    context->TSPPARAMBUFFER = state->header[2];
    context->TexturePARAMBUFFER = state->header[3];
    _nj_control_3d_flag_ = state->control;
    _nj_constant_attr_and_ = state->attr_and;
    _nj_constant_attr_or_ = state->attr_or;
    _nj_constant_material_ = state->material;
    _nj_current_texlist = state->texlist;
    _nj_current_texture = state->texture;
    _nj_current_texture_bank = state->bank;
    port_wide_anchor = state->anchor;
    port_wide_cover = state->cover;
    port_pillarbox = state->pillarbox;
}

static void ninja_save(void)
{
    memcpy(saved.contexts, _nj_context_, sizeof(saved.contexts));
    memcpy(saved.color_modes, _nj_ctx_color_mode_, sizeof(saved.color_modes));
    saved.context = _nj_current_ctx_;
    saved.color_mode = _nj_color_mode_;
    saved.view = _nj_view_;
    memcpy(saved.view_matrix, _nj_view_matrix_, sizeof(NJS_MATRIX));
    njGetMatrix(&saved.matrix);
    saved.matrix_ptr = _nj_current_matrix_ptr_;
    saved.stack_size = _stack_size_;
    state_get(&saved.state);
    draw_get_shading(&saved.shading);
    saved.depth = sprite_get_depth();
    saved.easy_light = *port_easy_light();
}

static void ninja_restore(void)
{
    memcpy(_nj_context_, saved.contexts, sizeof(saved.contexts));
    memcpy(_nj_ctx_color_mode_, saved.color_modes, sizeof(saved.color_modes));
    _nj_current_ctx_ = saved.context;
    _nj_color_mode_ = saved.color_mode;
    _nj_view_ = saved.view;
    memcpy(_nj_view_matrix_, saved.view_matrix, sizeof(NJS_MATRIX));
    _nj_current_matrix_ptr_ = saved.matrix_ptr;
    _stack_size_ = saved.stack_size;
    njSetMatrix(NULL, &saved.matrix);
    state_put(&saved.state);
    draw_put_shading(&saved.shading);
    sprite_set_depth(saved.depth);
    *port_easy_light() = saved.easy_light;
}

/* stop timer 1, and return TSTR as it was, which the caller writes back.
 * the split frame's own work stops it, so that SEGA's extra step times the
 * steps alone, as at 30 fps */
static Uint8 timer_stop(void)
{
    Uint8 run = TSTR;

    TSTR = run & ~STR1;
    return run;
}

/* task's first slot in marks: the top MARKS_BITS bits of a multiplicative
 * hash of its address */
static Uint32 marks_slot(const struct task *task)
{
    return ((Uint32)task >> 4) * 2654435761U >> (32 - MARKS_BITS);
}

/* task's marks, or null where it has none */
static Uint32 *marks_find(const struct task *task)
{
    Uint32 i = marks_slot(task);

    for (;; i = (i + 1) & (MARKS_MOST - 1)) {
        if (marks[i].task == task) {
            return &marks[i].marks;
        }
        if (marks[i].task == NULL) {
            return NULL;
        }
    }
}

/* mark task with mark. past three quarters of the table it marks nothing,
 * and the second picture is lost */
static void marks_add(struct task *task, Uint32 mark)
{
    Uint32 i = marks_slot(task);

    for (;; i = (i + 1) & (MARKS_MOST - 1)) {
        if (marks[i].task == task) {
            marks[i].marks |= mark;
            return;
        }
        if (marks[i].task == NULL) {
            break;
        }
    }
    if (marks_used >= MARKS_MOST * 3 / 4) {
        lost = TRUE;
        return;
    }
    marks_used++;
    marks[i].task = task;
    marks[i].marks = mark;
}

/* the index in list's views of the view a draw is made under, or -1 where
 * they are full */
static int view_index(struct draws *list)
{
    int i;

    for (i = list->view_count - 1; i >= 0; i--) {
        if (memcmp(list->views[i], _nj_view_matrix_, sizeof(NJS_MATRIX)) == 0) {
            return i;
        }
    }
    if (list->view_count >= VIEWS_MOST) {
        return -1;
    }
    memcpy(list->views[list->view_count], _nj_view_matrix_, sizeof(NJS_MATRIX));
    return list->view_count++;
}

/* keep in list a draw of present_task's step. a sprite keeps a copy of the
 * frame it draws from its texture animation, which a later draw may
 * change */
static void keep_draw(struct draws *list, enum present_kind kind,
                      const void *what, Int entry, Uint32 attr)
{
    const NJS_SPRITE *sprite = what;
    struct held *keep;
    int view;

    if (list->count >= list->most || (view = view_index(list)) < 0) {
        list->full = TRUE;
        return;
    }
    keep = &list->draw[list->count++];
    keep->task = present_task;
    keep->kind = kind;
    keep->attr = attr;
    keep->view = view;
    state_get(&keep->state);
    njGetMatrix(&keep->matrix);
    if (kind == PRESENT_MODEL) {
        keep->what.model.model = (NJS_MODEL *)what;
        draw_get_shading(&keep->what.model.shading);
        /* draw_tree, draw_action, draw_motion and draw_shape_motion draw
         * their models by draw_model_nested in one run from njGetkmPointer
         * to njSetkmPointer. only njSetkmPointer writes the run's header
         * back to the context. a model kept inside such a run takes a header
         * behind the texture set_texture made current, so a kept model
         * forgets that texture. its replay then sets each texture its
         * materials name */
        keep->state.texture = NO_CURRENT_TEXTURE;
    } else {
        keep->what.sprite.sprite = *sprite;
        keep->what.sprite.frame = sprite->tanim[entry];
        keep->what.sprite.depth = sprite_get_depth();
    }
}

/* the words a 2D draw's vertices take */
static int flat_words(enum present_kind kind, Int count, Uint32 attr)
{
    switch (kind) {
    case PRESENT_POLYGON_2D:
        /* each point, its color, and with a texture its uv */
        return count * (attr & NJD_USE_TEXTURE ? 4 : 3);
    case PRESENT_TEXTURE:
    case PRESENT_TEXTURE_MEMLIST:
        return count * (int)(sizeof(NJS_TEXTURE_VTX) / sizeof(Uint32));
    case PRESENT_POLYGON:
        return count * (int)(sizeof(NJS_POLYGON_VTX) / sizeof(Uint32));
    default:
        return 0;
    }
}

/* keep a 2D draw in flats with a copy of its vertices, or of its sprite and
 * the frame it draws from its texture animation. the next draw may change
 * that frame, as the board does for each digit of the score. flats is full
 * where the draw doesn't fit */
static void keep_flat(struct flats *flats, enum present_kind kind,
                      const void *what, Int count, Int a, Float pri,
                      Uint32 attr)
{
    int words = flat_words(kind, count, attr);
    struct flat *keep;
    Uint32 *to;

    if (flats->count >= FLATS_MOST || count < 0 ||
        words > FLAT_WORDS - flats->used) {
        flats->full = TRUE;
        return;
    }
    keep = &flats->flat[flats->count++];
    keep->task = present_task;
    keep->kind = kind;
    keep->count = count;
    keep->a = a;
    keep->pri = pri;
    keep->attr = attr;
    keep->words = flats->used;
    to = &flats->words[flats->used];
    flats->used += words;
    if (kind == PRESENT_POLYGON_2D) {
        const NJS_POINT2COL *p = what;

        memcpy(to, p->p, count * sizeof(NJS_POINT2));
        memcpy(to + count * 2, p->col, count * sizeof(NJS_COLOR));
        if (attr & NJD_USE_TEXTURE) {
            memcpy(to + count * 3, p->tex, count * sizeof(NJS_COLOR));
        }
    } else if (kind == PRESENT_SPRITE_2D) {
        keep->sprite = *(const NJS_SPRITE *)what;
        keep->frame = keep->sprite.tanim[a];
    } else {
        memcpy(to, what, words * sizeof(Uint32));
    }
    state_get(&keep->state);
}

/* keep present_record's draw for a picture to come, and mark its task. it
 * stays out of line, so that present_record's frequent early returns don't
 * save the registers it needs: inlined, they cost 17 cycles a call */
static void keep_step(enum present_kind kind, const void *what, Int count,
                      Int a, Float pri, Uint32 attr) __attribute__((noinline));
static void keep_step(enum present_kind kind, const void *what, Int count,
                      Int a, Float pri, Uint32 attr)
{
    struct task *task = present_task;

    if (kind >= PRESENT_POLYGON_2D) {
        keep_flat(keeping == KEEP_FIRST ? &first_flats : &later_flats, kind,
                  what, count, a, pri, attr);
    } else if (task == NULL) {
        return;
    } else if (keeping == KEEP_FIRST) {
        /* the second picture may replay each sprite of step 0's, and the
         * other draws of a task with no callback of its own, or whose
         * callback is its step */
        if (frame_step != 0) {
            return;
        }
        if (kind == PRESENT_NJ_SPRITE) {
            keep_draw(&first_draws, kind, what, a, attr);
        } else if (task->draw == NULL || task->draw == task->step) {
            marks_add(task, HAS_HELD);
            keep_draw(&first_draws, kind, what, a, attr);
        }
    } else if (kind == PRESENT_NJ_SPRITE) {
        /* njDrawSprite3D draws on every step, shipped too, so the shipped
         * picture holds this sprite beside step 0's. the next frame's first
         * picture replays it, and the pass draws the task no sprite */
        keep_draw(&later_draws, kind, what, a, attr);
        if (frame_split) {
            marks_add(task, SPRITES_LATER);
        }
    } else if (frame_split && frame_step == 1) {
        marks_add(task, DREW_LATER);
    }
}

void present_record(enum present_kind kind, const void *what, Int count, Int a,
                    Float pri, Uint32 attr)
{
    Uint8 run;

    if (frame_redrawing) {
        /* the pass replays none of step 0's 2D for a task it drew 2D for,
         * and none of step 0's sprites for a task it drew a sprite for */
        if (present_task == NULL) {
            return;
        }
        if (kind >= PRESENT_POLYGON_2D) {
            marks_add(present_task, DREW_FLAT);
        } else if (kind == PRESENT_NJ_SPRITE) {
            marks_add(present_task, DREW_SPRITE);
        }
        return;
    }
    if (keeping == KEEP_NONE) {
        return;
    }
    run = timer_stop();
    keep_step(kind, what, count, a, pri, attr);
    TSTR = run;
}

/* njSprite.c's call through port_sprite_record: present_record's, but
 * FALSE, with nothing recorded, where the pass would draw a sprite for a
 * task whose step drew one after step 0, which its steps' own stand for */
static Int present_sprite(enum present_kind kind, const void *what, Int count,
                          Int a, Float pri, Uint32 attr)
{
    const Uint32 *mark;

    if (frame_redrawing && kind == PRESENT_NJ_SPRITE && present_task != NULL) {
        mark = marks_find(present_task);
        if (mark != NULL && (*mark & SPRITES_LATER)) {
            return FALSE;
        }
    }
    present_record(kind, what, count, a, pri, attr);
    return TRUE;
}

void present_untimed(int begin)
{
    if (!frame_split || frame_step != 1) {
        return;
    }
    if (begin) {
        if (untimed_depth++ == 0) {
            untimed_run = timer_stop();
        }
    } else if (--untimed_depth == 0) {
        TSTR = untimed_run;
    }
}

void present_shadow(const struct shadow_place *place)
{
    struct kept_shadow *keep;
    Uint8 run;

    if (frame_redrawing) {
        shadow_draw_placed(place);
        if (present_task != NULL) {
            marks_add(present_task, DREW_SHADOW);
        }
    } else if (frame_step == 0) {
        shadow_draw_placed(place);
    } else if (frame_split && frame_step == 1) {
        run = timer_stop();
        if (shadows_count < SHADOWS_MOST) {
            keep = &shadows[shadows_count++];
            keep->place = *place;
            keep->task = present_task;
        } else {
            lost = TRUE;
        }
        TSTR = run;
    }
}

/* the task a kept draw names once its own is deleted with no destructor, so
 * that a task made in the freed slot takes none of its draws. no destructor
 * released the textures they take, so the pictures still replay its 2D and
 * the sprites njDrawSprite3D drew for it, as Chaos 0's sparks, which draw
 * once and delete themselves. it holds no marks, so the pass holds none of
 * its other draws */
static struct task deleted_task;

/* the task a kept draw names once its own is deleted with a destructor */
static struct task released_task;

/* TRUE where task was deleted, or is killed, with a destructor, which may
 * release what its draws take, so that no picture replays them */
static int released(const struct task *task)
{
    return task == &released_task ||
           (task != NULL && task->step == task_destroy &&
            task->destructor != NULL);
}

/* key each of list's draws that task made to deleted_task where task has no
 * destructor, and else to released_task */
static void draws_forget(struct draws *list, const struct task *task)
{
    struct task *now =
        task->destructor == NULL ? &deleted_task : &released_task;
    int i;

    for (i = 0; i < list->count; i++) {
        if (list->draw[i].task == task) {
            list->draw[i].task = now;
        }
    }
}

/* key each of flats' 2D draws that task made to deleted_task where task has
 * no destructor, and else to released_task. a draw that no task made keeps
 * null, and both pictures replay it */
static void flats_forget(struct flats *flats, const struct task *task)
{
    struct task *now =
        task->destructor == NULL ? &deleted_task : &released_task;
    int i;

    for (i = 0; i < flats->count; i++) {
        if (flats->flat[i].task == task) {
            flats->flat[i].task = now;
        }
    }
}

/* drop each shadow step 1 kept for task */
static void shadows_forget(const struct task *task)
{
    int kept = 0;
    int i;

    for (i = 0; i < shadows_count; i++) {
        if (shadows[i].task != task) {
            shadows[kept++] = shadows[i];
        }
    }
    shadows_count = kept;
}

void present_forget(struct task *task)
{
    Uint32 *mark;
    Uint8 run;

    if (keeping == KEEP_NONE) {
        return;
    }
    run = timer_stop();
    mark = marks_find(task);
    if (mark != NULL) {
        *mark = 0;
    }
    draws_forget(&first_draws, task);
    draws_forget(&later_draws, task);
    flats_forget(&first_flats, task);
    flats_forget(&later_flats, task);
    shadows_forget(task);
    TSTR = run;
}

static void present_wide(Float anchor, Int cover)
{
    port_wide_anchor = anchor;
    port_wide_cover = cover;
}

void present_run(struct task *task, void (*step)(struct task *task))
{
    struct task *outer = present_task;
    /* a root task's step starts and ends at WIDE_MIDDLE and FALSE, and a
     * child's, which its parent's step runs, at what the parent set */
    Float anchor = outer != NULL ? port_wide_anchor : WIDE_MIDDLE;
    Int cover = outer != NULL ? port_wide_cover : FALSE;

    if (frame_redrawing) {
        return;
    }
    present_wide(anchor, cover);
    present_task = task;
    step(task);
    present_task = outer;
    present_wide(anchor, cover);
}

/* the records of task's own that the pass puts back after its callback.
 * each task whose callback the pass calls has a whole task_work: the one
 * smaller work, Red Mountain's puffs' 44 bytes, is a child's with no
 * callback */
struct records {
    struct task_work work;
    struct task_motion motion;
    struct player record;
    struct collision_shape shapes[SHAPES_KEPT];
    /* the player's record, or null, and the shapes kept */
    struct player *player;
    int shapes_kept;
};

/* keep task's records, and return TRUE, or FALSE where a player's shapes
 * don't fit, which loses the second picture */
static int records_keep(struct records *keep, const struct task *task)
{
    int i;

    keep->player = NULL;
    keep->shapes_kept = 0;
    if (task->work != NULL) {
        keep->work = *task->work;
    }
    for (i = 0; i < PLAYER_COUNT; i++) {
        if (player_tasks[i] == task && player_records[i] != NULL) {
            keep->player = player_records[i];
        }
    }
    if (keep->player == NULL) {
        return TRUE;
    }
    keep->motion = *task->motion;
    keep->record = *keep->player;
    if (task->work != NULL && task->work->collision != NULL) {
        keep->shapes_kept = task->work->collision->count;
        if (keep->shapes_kept > SHAPES_KEPT) {
            lost = TRUE;
            return FALSE;
        }
        memcpy(keep->shapes, task->work->collision->shapes,
               keep->shapes_kept * sizeof(struct collision_shape));
    }
    return TRUE;
}

static void records_put(const struct records *keep, struct task *task)
{
    if (task->work != NULL) {
        *task->work = keep->work;
    }
    if (keep->player == NULL) {
        return;
    }
    *task->motion = keep->motion;
    *keep->player = keep->record;
    if (keep->shapes_kept != 0) {
        memcpy(task->work->collision->shapes, keep->shapes,
               keep->shapes_kept * sizeof(struct collision_shape));
    }
}

void present_draw(struct task *task, void (*draw)(struct task *task))
{
    struct task *outer = present_task;
    struct records keep;
    Uint32 *mark;

    /* a callback starts and ends at WIDE_MIDDLE and FALSE, since
     * task_draw_all, which calls each, runs outside every task */
    if (!frame_redrawing) {
        if (draw != NULL) {
            present_wide(WIDE_MIDDLE, FALSE);
            draw(task);
            present_wide(WIDE_MIDDLE, FALSE);
        }
        return;
    }
    /* a killed task draws nothing. the camera task's callback draws the
     * underwater tint, which each picture holds from both steps already, and
     * clears camera_in_dry_box, which a step sets */
    if (task->step == task_destroy ||
        (camera_work != NULL && task->work == camera_work)) {
        return;
    }
    mark = marks_find(task);
    if (mark != NULL && (*mark & DREW_LATER)) {
        return;
    }
    if (draw != NULL && draw != task->step) {
        if (!records_keep(&keep, task)) {
            return;
        }
        present_wide(WIDE_MIDDLE, FALSE);
        present_task = task;
        draw(task);
        present_task = outer;
        records_put(&keep, task);
        present_wide(WIDE_MIDDLE, FALSE);
        return;
    }
    if (mark != NULL) {
        *mark |= HOLDS;
    }
}

/* v turned by m's rotation, as a row times m's first three rows and
 * columns */
static NJS_VECTOR turned(const Float *m, const NJS_VECTOR *v)
{
    NJS_VECTOR to;

    to.x = v->x * m[0] + v->y * m[4] + v->z * m[8];
    to.y = v->x * m[1] + v->y * m[5] + v->z * m[9];
    to.z = v->x * m[2] + v->y * m[6] + v->z * m[10];
    return to;
}

/* the move from the view from to the view now: from's inverse times the
 * view now. a matrix made under from is a world times from, so the matrix
 * times the move is that world times the view now */
static void view_move(NJS_MATRIX *move, const NJS_MATRIX *from)
{
    NJS_MATRIX inverse;

    memcpy(inverse, *from, sizeof(NJS_MATRIX));
    njInvertMatrix(&inverse);
    memcpy(*move, _nj_view_matrix_, sizeof(NJS_MATRIX));
    njMultiMatrix(move, &inverse);
}

/* TRUE where a picture replays draw: in the first, each of the last frame's
 * later sprites, and in the second, each of step 0's sprites but where the
 * pass drew one for its task, and step 0's other draws where the pass holds
 * their task. neither replays a draw of a task killed or deleted with a
 * destructor, which may release the textures the draw takes */
static int replays(const struct held *draw, int second)
{
    const Uint32 *mark;

    if (released(draw->task)) {
        return FALSE;
    }
    if (!second) {
        return TRUE;
    }
    mark = marks_find(draw->task);
    if (draw->kind == PRESENT_NJ_SPRITE) {
        return mark == NULL || !(*mark & DREW_SPRITE);
    }
    return mark != NULL && (*mark & HOLDS);
}

/* draw again each of list's draws the picture replays, moved from the view
 * it was made under to the view now. second is TRUE for the second picture.
 * the current matrix goes back after, since particle_draw_all draws through
 * it */
static void draws_replay(struct draws *list, int second)
{
    NJS_MATRIX moves[VIEWS_MOST];
    NJS_MATRIX matrix;
    NJS_MATRIX current;
    struct draw_shading shading;
    NJS_SPRITE sprite;
    struct held *draw;
    int i;

    for (i = 0; i < list->view_count; i++) {
        view_move(&moves[i], &list->views[i]);
    }
    njGetMatrix(&current);
    for (draw = list->draw; draw < list->draw + list->count; draw++) {
        if (!replays(draw, second)) {
            continue;
        }
        memcpy(matrix, moves[draw->view], sizeof(NJS_MATRIX));
        njMultiMatrix(&matrix, &draw->matrix);
        njSetMatrix(NULL, &matrix);
        state_put(&draw->state);
        if (draw->kind == PRESENT_MODEL) {
            /* the light lies in view space, so it turns with the move */
            shading = draw->what.model.shading;
            shading.dir = turned(moves[draw->view], &shading.dir);
            shading.base = turned(moves[draw->view], &shading.base);
            draw_put_shading(&shading);
            draw_model(draw->what.model.model);
            continue;
        }
        sprite = draw->what.sprite.sprite;
        sprite.tanim = &draw->what.sprite.frame;
        if (draw->kind == PRESENT_SPRITE) {
            sprite_set_depth(draw->what.sprite.depth);
            sprite_draw_3d(&sprite, 0, draw->attr);
        } else {
            njDrawSprite3D(&sprite, 0, draw->attr);
        }
    }
    njSetMatrix(NULL, &current);
}

/* draw again each of flats' 2D draws but a released task's, and where fresh
 * is TRUE, those of a task the pass drew 2D for */
static void draw_flats(struct flats *flats, int fresh)
{
    const Uint32 *mark;
    struct flat *draw;
    Uint32 *words;
    NJS_POINT2COL points;
    NJS_SPRITE sprite;

    for (draw = flats->flat; draw < flats->flat + flats->count; draw++) {
        if (released(draw->task)) {
            continue;
        }
        if (fresh && draw->task != NULL) {
            mark = marks_find(draw->task);
            if (mark != NULL && (*mark & DREW_FLAT)) {
                continue;
            }
        }
        words = &flats->words[draw->words];
        state_put(&draw->state);
        switch (draw->kind) {
        case PRESENT_POLYGON_2D:
            points.p = (NJS_POINT2 *)words;
            points.col = (NJS_COLOR *)(words + draw->count * 2);
            points.tex = draw->attr & NJD_USE_TEXTURE
                             ? (NJS_COLOR *)(words + draw->count * 3)
                             : NULL;
            points.num = draw->count;
            njDrawPolygon2D(&points, draw->count, draw->pri, draw->attr);
            break;
        case PRESENT_TEXTURE:
            njDrawTexture((NJS_TEXTURE_VTX *)words, draw->count, draw->a,
                          (Int)draw->attr);
            break;
        case PRESENT_TEXTURE_MEMLIST:
            njDrawTextureMemList((NJS_TEXTURE_VTX *)words, draw->count, draw->a,
                                 (Int)draw->attr);
            break;
        case PRESENT_POLYGON:
            njDrawPolygon((NJS_POLYGON_VTX *)words, draw->count,
                          (Int)draw->attr);
            break;
        default:
            sprite = draw->sprite;
            sprite.tanim = &draw->frame;
            njDrawSprite2D(&sprite, 0, draw->pri, draw->attr);
            break;
        }
    }
}

/* the camera's pose now */
static void pose_get(struct pose *pose)
{
    pose->work = camera_work;
    if (camera_work != NULL) {
        pose->ang = camera_work->ang;
        pose->pos = camera_work->pos;
    }
    pose->eye.x = view.px;
    pose->eye.y = view.py;
    pose->eye.z = view.pz;
}

/* how the camera went from from to to */
static enum went pose_went(const struct pose *from, const struct pose *to)
{
    if (from->work == NULL || from->work != to->work) {
        return WENT_ELSE;
    }
    if (memcmp(&from->ang, &to->ang, sizeof(struct angles)) == 0 &&
        memcmp(&from->pos, &to->pos, sizeof(NJS_POINT3)) == 0) {
        return WENT_STOOD;
    }
    return WENT_MOVED;
}

/* TRUE where the move from from to to is a cut. a NaN fails each test of
 * the move, so it cuts */
static int pose_cut(const struct pose *from, const struct pose *to)
{
    Float x = to->pos.x - from->pos.x;
    Float y = to->pos.y - from->pos.y;
    Float z = to->pos.z - from->pos.z;
    Angle ang_x = angle_difference(from->ang.x, to->ang.x);
    Angle ang_y = angle_difference(from->ang.y, to->ang.y);
    Angle ang_z = angle_difference(from->ang.z, to->ang.z);

    return !(x <= CUT_MOVE && x >= -CUT_MOVE && y <= CUT_MOVE &&
             y >= -CUT_MOVE && z <= CUT_MOVE && z >= -CUT_MOVE) ||
           ang_x > CUT_TURN || ang_x < -CUT_TURN || ang_y > CUT_TURN ||
           ang_y < -CUT_TURN || ang_z > CUT_TURN || ang_z < -CUT_TURN;
}

/* TRUE where the view is the camera's, from camera_work: main.c's view,
 * which no camera action gives */
static int camera_viewing(void)
{
    return _nj_view_ == &view && camera_action_state == ACTION_NONE;
}

/* give camera_work the pose halfway between from and to, each angle by its
 * shortest way round, and make the view from it as camera.c's
 * camera_apply_view does, but at the position halfway between from's view
 * and to's, which keeps the shakes. the models' light and the easy draws'
 * turn with the view */
static void camera_between(const struct pose *from, const struct pose *to)
{
    struct task_work *work = camera_work;
    NJS_MATRIX *top = _nj_current_matrix_ptr_;
    Sint32 depth = _stack_size_;
    NJS_MATRIX base;
    NJS_MATRIX move;
    struct draw_shading shading;

    work->ang.x = from->ang.x + angle_difference(from->ang.x, to->ang.x) / 2;
    work->ang.y = from->ang.y + angle_difference(from->ang.y, to->ang.y) / 2;
    work->ang.z = from->ang.z + angle_difference(from->ang.z, to->ang.z) / 2;
    work->pos.x = (from->pos.x + to->pos.x) * 0.5F;
    work->pos.y = (from->pos.y + to->pos.y) * 0.5F;
    work->pos.z = (from->pos.z + to->pos.z) * 0.5F;
    between = view;
    njRotateViewZAbsolute(&between, work->ang.z);
    njRotateViewXAbsolute(&between, work->ang.x);
    njRotateViewYRelative(&between, work->ang.y);
    between.px = (from->eye.x + to->eye.x) * 0.5F;
    between.py = (from->eye.y + to->eye.y) * 0.5F;
    between.pz = (from->eye.z + to->eye.z) * 0.5F;
    /* njSetView empties the matrix stack. the pass's draws push above what
     * it held, so its depth, top and base go back, under the new view */
    memcpy(base, *_nj_base_matrix_ptr_, sizeof(NJS_MATRIX));
    njSetView(&between);
    _stack_size_ = depth;
    _nj_current_matrix_ptr_ = top;
    memcpy(*_nj_base_matrix_ptr_, base, sizeof(NJS_MATRIX));
    /* the lights lie in view space, so they turn with the move */
    view_move(&move, &saved.view_matrix);
    draw_get_shading(&shading);
    shading.dir = turned(move, &shading.dir);
    shading.base = turned(move, &shading.base);
    draw_put_shading(&shading);
    *port_easy_light() = turned(move, port_easy_light());
}

/* the pass's cost, as the last pass's share of step 0 gives it */
static Uint32 pass_cost(void)
{
    return step0_cost * pass_share / 256;
}

/* TRUE where step 1 and the pass fit the second half, as their shares of
 * step 0 give them */
static int second_fits(void)
{
    return step0_cost * step1_share / 256 + pass_cost() + SECOND_ROOM <=
           VSYNC_COUNTS;
}

/* TRUE where the PVR can take the first picture on time. Kamui starts a
 * render only at a vsync, so the render running now has to end by the
 * middle vsync, where the first picture's starts, and the first picture's
 * lists have to reach the tile accelerator by then. its own render then has
 * to end by the frame's last vsync, where the second's starts. the last
 * render's and transfer's lengths, as Kamui times them, stand in for the
 * next. Kamui keeps a render's start as TCNT0, which counts down from
 * syTmrGetCount's 0 */
static int pvr_fits(void)
{
    Uint32 started = ~kmiIntStatisticData.render_start;
    Uint32 length = kmiIntStatisticData.render_time;

    if (kmiDeviceExtension.render_busy != 0 &&
        (Sint32)(started + length - _nj_wait_vsync_out_) + RENDER_ROOM >
            VSYNC_COUNTS) {
        return FALSE;
    }
    return step0_cost + FIRST_ROOM + kmiIntStatisticData.ta_time <=
               VSYNC_COUNTS &&
           length + RENDER_ROOM <= VSYNC_COUNTS;
}

/* the first picture: the last frame's later sprites, moved to the view
 * now, its later 2D, and the overlay, over what step 0 drew, rendered and
 * shown at the middle vsync */
static void show_first(void)
{
    Uint8 run = timer_stop();
    Uint32 flip;

    ninja_save();
    draws_replay(&later_draws, FALSE);
    draw_flats(&later_flats, FALSE);
    /* the overlay draws outside pillarbox, as at the frame's end */
    port_pillarbox = FALSE;
    overlay_draw();
    ninja_restore();
    kmRender();
    _nj_calc_vertex_ = 0;
    _nj_calc_polygon_ = 0;
    _nj_draw_polygon_ = 0;
    _nj_vsync_count_++;
    flip = syTmrGetCount();
    kmFlipFrameBuffer();
    middle = syTmrGetCount();
    middle_wait = middle - flip;
    TSTR = run;
}

void present_step(void)
{
    Uint32 now = syTmrGetCount();

    if (frame_step == 1 && frame_split && step0_cost != 0) {
        step1_share = (now - step1_start) * 256 / step0_cost;
    }
    if (frame_step != 0) {
        return;
    }
    stage_ran = TRUE;
    step0_cost = now - _nj_wait_vsync_out_;
    first_fits = step0_cost + FIRST_ROOM <= VSYNC_COUNTS;
    render_fits = pvr_fits();
    /* pvr_fits counts step 0's time too, so a step 0 that overran its half
     * names the CPU, T, ahead of the PVR */
    held_reason = !first_fits      ? 'T'
                  : !render_fits   ? 'R'
                  : !second_fits() ? 'T'
                                   : 0;
    frame_split = FALSE;
    keeping = KEEP_NONE;
    /* a bitmap step 0 queued uploads at the frame's end, as at 30 fps, so a
     * frame with one waiting shows one picture */
    if (planned && later_kept && !lost && !first_flats.full &&
        !first_draws.full && first_fits && render_fits && second_fits() &&
        frame_interval == 2 && frame_step_count == 2 && frame_countdown == 1 &&
        draw_off != TRUE && texture_queue_count == 0) {
        show_first();
        frame_split = TRUE;
    }
    /* step 1's models and sprite_draw_3d's sprites draw for the second
     * picture, but where that picture may take a pose between the
     * pictures': where the last frame's camera moved over the later steps
     * alone, and this frame's stood over step 0 */
    pose_get(&first_pose);
    frame_later_draws =
        frame_split && !(moved_later && camera_viewing() &&
                         pose_went(&start_pose, &first_pose) == WENT_STOOD);
    later_flats.count = later_flats.used = later_flats.full = 0;
    later_draws.count = later_draws.view_count = later_draws.full = 0;
    shadows_count = 0;
    if (planned) {
        keeping = KEEP_LATER;
    }
    step1_start = syTmrGetCount();
}

void present_second(void)
{
    Uint32 start = syTmrGetCount();
    unsigned long rnext = _rnext;
    int step = frame_step;
    struct pose now;
    enum went first;
    enum went later;
    const Uint32 *mark;
    int halfway;
    int i;

    /* how the camera went over step 0, and over the later steps */
    pose_get(&now);
    first = pose_went(&start_pose, &first_pose);
    later = pose_went(&first_pose, &now);
    start_pose = now;
    moved_later = first == WENT_STOOD && later == WENT_MOVED;
    if (!frame_split) {
        return;
    }
    /* the first picture holds where the camera moved over step 0 alone,
     * where the pass doesn't fit, and where a record didn't. a picture that
     * lacks a record would show a draw in one picture alone */
    if ((first == WENT_MOVED && later == WENT_STOOD) || lost ||
        start - middle + pass_cost() + SECOND_ROOM > VSYNC_COUNTS) {
        dropped = TRUE;
        return;
    }
    /* where step 1's models and sprite_draw_3d's sprites drew nothing, a
     * camera that moved over the later steps takes the pose halfway, except
     * across a cut */
    halfway = !frame_later_draws && later == WENT_MOVED && camera_viewing() &&
              !pose_cut(&first_pose, &now);
    /* SEGA's pause draws at frame_step 0, as the draws' tests want */
    frame_redrawing = TRUE;
    frame_step = 0;
    ninja_save();
    if (halfway) {
        camera_between(&first_pose, &now);
    }
    task_draw_all();
    /* step 1's shadows, but those of a task whose callback drew one */
    for (i = 0; i < shadows_count; i++) {
        mark = shadows[i].task != NULL ? marks_find(shadows[i].task) : NULL;
        if (mark == NULL || !(*mark & DREW_SHADOW)) {
            shadow_draw_placed(&shadows[i].place);
        }
    }
    draws_replay(&first_draws, TRUE);
    particle_draw_all();
    draw_flats(&first_flats, TRUE);
    ninja_restore();
    if (halfway) {
        camera_work->ang = now.ang;
        camera_work->pos = now.pos;
    }
    frame_step = step;
    _rnext = rnext;
    frame_redrawing = FALSE;
    if (step0_cost != 0) {
        pass_share = (syTmrGetCount() - start) * 256 / step0_cost;
    }
    /* the first picture holds too where a record the pass needs didn't fit */
    if (lost) {
        dropped = TRUE;
    }
}

int present_reason(int want, int late)
{
    if (!want || !stage_ran) {
        return 0;
    }
    return late != 0 ? 'T' : held_reason;
}

int present_hold(void)
{
    if (!holding) {
        return FALSE;
    }
    holding = FALSE;
    /* the queue uploads once a frame, as at 30 fps, and the flip would wait
     * out the frame's first vsync */
    bitmap_flush_queue();
    while (frame_countdown == frame_interval) {
    }
    return TRUE;
}

Uint32 present_idle(void)
{
    return middle_wait;
}

void present_end(int want, int late)
{
    int fits;

    later_kept =
        keeping == KEEP_LATER && !later_flats.full && !later_draws.full;
    if (keeping == KEEP_LATER) {
        records_fit =
            later_kept && !lost && !first_flats.full && !first_draws.full;
    }
    /* a scene that overflows the records ends the split and keeps it off */
    fits = stage_ran && first_fits && render_fits && second_fits() &&
           late == 0 && records_fit;
    keeping = KEEP_NONE;
    holding = dropped;
    dropped = FALSE;
    if (!stage_ran) {
        frame_split = frame_later_draws = FALSE;
    }
    if (!want || !stage_ran) {
        engaged = against = 0;
        records_fit = TRUE;
        step1_share = STEP1_SHARE;
        pass_share = PASS_SHARE;
    } else if (engaged ? !fits : fits) {
        if (++against >= (engaged ? DROP_FRAMES : ENGAGE_FRAMES)) {
            engaged = !engaged;
            against = 0;
        }
    } else {
        against = 0;
    }
    planned = want && engaged;
    port_sprite_record = present_sprite;
    overlay_draw();
    first_flats.count = first_flats.used = first_flats.full = 0;
    first_draws.count = first_draws.view_count = first_draws.full = 0;
    lost = FALSE;
    middle_wait = 0;
    if (marks_used != 0) {
        memset(marks, 0, sizeof(marks));
        marks_used = 0;
    }
    stage_ran = first_fits = render_fits = FALSE;
    if (planned) {
        keeping = KEEP_FIRST;
    }
}
