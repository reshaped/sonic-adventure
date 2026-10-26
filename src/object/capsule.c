/* the capsule, O RELEASE in the object lists. a player standing on its
 * button sinks it. then the capsule breaks into flying pieces and lets three
 * animals go. CLEAR_DELAY steps later, it clears the stage */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "animal/kinds.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "hud/hud.h"
#include "object/capsule.h"
#include "object/common.h"
#include "object/models.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* CLEAR_DELAY is the steps from the break to the stage's clear, and
 * PIECES_TIME the steps the pieces fly */
#define CLEAR_DELAY 60
#define PIECES_TIME 120

/* each step, a flying piece gains PIECE_FALL in downward speed. the pieces
 * lose PIECE_FADE in alpha each step, 127 over PIECES_TIME */
#define PIECE_FALL 0.021777776F
#define PIECE_FADE 1.0583333F

/* the sound capsule_clear_stage plays as it clears the stage */
#define SE_CLEAR 730

/* over the button, a player is within BUTTON_REACH across and BUTTON_TOP
 * to BUTTON_HIGH above its pos. below BUTTON_STAND, it stands on the
 * button */
#define BUTTON_REACH 12.0F
#define BUTTON_TOP 27.0F
#define BUTTON_STAND 29.6F
#define BUTTON_HIGH 57.0F

/* the pushed button's depth changes by BUTTON_SINK each step, down to
 * BUTTON_DEPTH */
#define BUTTON_SINK (-0.25F)
#define BUTTON_DEPTH (-3.0F)

/* a strip's first index holds its length in these bits */
#define STRIP_LENGTH 0x3fff
/* the bits of type_matId below its NJD_MESHSET_ type: the material's
 * index */
#define MATERIAL_INDEX 0x3fff
/* the bits of attr_texId that capsule_cut_meshset takes as the texture's
 * number */
#define TEXTURE_NUMBER 0x1fffffff

/* capsule_release_animals sets the animals a third of a turn apart. each
 * stands ANIMAL_RADIUS out from the capsule and ANIMAL_HEIGHT above it */
#define ANIMAL_SPACING 0x5555
#define ANIMAL_RADIUS 8.5F
#define ANIMAL_HEIGHT 1.5F

/* a triangle of the capsule's body, flying out from where the body was */
struct capsule_piece {
    NJS_POINT3 pos; /* its corners' middle */
    struct angles ang;
    NJS_VECTOR speed; /* per step */
    Angle spin; /* added to ang.x each step */
    NJS_POINT3 points[3]; /* its corners, less pos */
    NJS_COLOR *uv; /* the first of its corners' uvs, in the meshset's */
    Uint32 texture; /* its texture's number, for njSetTextureNum */
    /* the pieces from this one to the last, so the first holds the total */
    Sint32 count;
};

/* the capsule's view of its task_work. capsule_start checks that pos, id,
 * collision and event sit where task_work keeps them */
struct capsule_work {
    Sint8 mode;
    Sint8 pieces_mode;
    Uint8 id;
    /* the player on the button, or -1 for none. once the button is pushed,
     * it keeps the player that pushed it */
    Sint8 player;
    Sint16 flags;
    Sint16 timer; /* the pieces' steps left to fly */
    struct capsule_piece *pieces; /* capsule_cut's, or NULL */
    Float alpha; /* the pieces' alpha, from 255 */
    Uint8 unk_10[4];
    struct angles ang;
    NJS_POINT3 pos;
    Sint32 wait; /* the steps left until the stage's clear */
    struct task *button; /* the button's task, or NULL once it is gone */
    Uint8 unk_34[4];
    struct collision_work *collision;
    struct event_work *event;
};

/* the button's view of its task_work. capsule_start checks that pos and
 * flags sit where task_work keeps them */
struct button_work {
    Sint8 mode;
    Uint8 unk_01[3];
    Sint16 flags;
    Uint8 unk_06[2];
    Float depth; /* the button's pos.y, less the capsule's */
    Uint8 unk_0c[20];
    NJS_POINT3 pos;
};

/* the capsule's work->mode */
enum capsule_mode {
    CAPSULE_WAIT, /* whole, until its button is down */
    CAPSULE_BREAK, /* breaks, in one step */
    CAPSULE_CLEAR, /* waits CLEAR_DELAY steps, then clears the stage */
    CAPSULE_DONE /* only its pieces move */
};

/* the capsule's work->pieces_mode, 0 before the break */
enum pieces_mode { PIECES_FLY = 1, PIECES_GONE };

/* the button's work->mode */
enum button_mode {
    BUTTON_UP, /* until a player stands on it */
    BUTTON_PUSHED, /* sinking, with both pads off */
    BUTTON_DOWN /* down, which breaks the capsule */
};

static void capsule_wait(struct task *task);
static void capsule_break(struct task *task);
static void capsule_clear(struct task *task);
static void capsule_done(struct task *task);
static void capsule_step(struct task *task);
static void capsule_draw(struct task *task);
static void capsule_destroy(struct task *task);
static void capsule_button_start(struct task *task);
static void capsule_button_step(struct task *task);
static void capsule_button_draw(struct task *task);
static void capsule_button_destroy(struct task *task);
static void capsule_shatter(struct task *task);
static void capsule_pieces_move(struct task *task);
static void capsule_pieces_draw(struct task *task);
static void capsule_pieces_free(struct task *task);
static struct capsule_piece *capsule_cut(NJS_MODEL *model);
static void capsule_button_hold(int player, NJS_POINT3 *pos);
static void capsule_release_animals(NJS_POINT3 *pos);

/* capsule_step runs the one for work->mode */
static void (*const capsule_modes[])(struct task *task) = {
    capsule_wait,
    capsule_break,
    capsule_clear,
    capsule_done,
};

static const NJS_POINT3 capsule_zero = {0.0F, 0.0F, 0.0F};

/* a sphere for the whole capsule, then one at each of its four sides */
/* clang-format off */
static struct collision_shape capsule_shapes[5] = {
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 7.0F, 0.0F}, 18.0F,
     0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {16.0F, 1.5F, 0.0F}, 5.0F,
     3.0F, 4.0F, {0}, 0, NULL},
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 1.5F, 16.0F}, 5.0F,
     3.0F, 5.0F, {0}, 0, NULL},
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {-16.0F, 1.5F, 0.0F},
     5.0F, 3.0F, 4.0F, {0}, 0, NULL},
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 1.5F, -16.0F},
     5.0F, 3.0F, 5.0F, {0}, 0, NULL},
};

static struct collision_shape capsule_button_shape = {
    SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 23.0F, 0.0F},
    12.0F, 7.0F, 0.0F, {0}, 0, NULL};
/* clang-format on */

/* TRUE once capsule_start makes a capsule the stage's, until
 * capsule_reset. only capsule_test_active reads it */
static Bool capsule_active = FALSE;
/* the stage's capsule task, or NULL */
static struct task *capsule_task = NULL;

/* where the stage's capsule stands. nothing reads it */
static NJS_POINT3 capsule_pos;

int capsule_get_player(void)
{
    struct task *task = capsule_task;
    struct capsule_work *work;

    if (task != NULL) {
        work = (struct capsule_work *)task->work;
        return work->player;
    }
    return -2;
}

void capsule_reset(void)
{
    capsule_task = NULL;
    capsule_active = FALSE;
}

void capsule_start(struct task *task)
{
    struct capsule_work *work = (struct capsule_work *)task->work;

    if (!WORK_SAME_PLACE(struct capsule_work, pos) ||
        !WORK_SAME_PLACE(struct capsule_work, id) ||
        !WORK_SAME_PLACE(struct capsule_work, collision) ||
        !WORK_SAME_PLACE(struct capsule_work, event) ||
        !WORK_SAME_PLACE(struct button_work, pos) ||
        !WORK_SAME_PLACE(struct button_work, flags)) {
        capsule_task = NULL;
        return;
    }
    work->pieces = NULL;
    work->player = -1;
    collision_init(task, capsule_shapes, COLLISION_OBJECT, 1, 5);
    work->button = task_create_child(TASK_WORK, capsule_button_start, task);
    capsule_button_start(work->button);
    if (!placement_editing(task)) {
        capsule_task = task;
        capsule_pos = work->pos;
        capsule_active = TRUE;
    }
    /* @hack redundant: a first store of the step, which the last one
     * overwrites (MOV.L R3,@(16,R14) at 0x8c065c68, then MOV.L R2,@(16,R14)
     * at 0x8c065c72). without it, 13 of 94 words differ */
    task->step = capsule_step;
    task->draw = capsule_draw;
    task->destructor = capsule_destroy;
    task->step = capsule_step;
}

void capsule_test_active(void)
{
    /* @hack redundant: an empty test of capsule_active, which keeps the
     * image's load and TST R3,R3 (0x8c065c7e, 0x8c065c82). without it, 4 of
     * 4 words differ */
    if (capsule_active) {
    }
}

struct task *capsule_get(void)
{
    return capsule_task;
}

static void capsule_clear_stage(struct task *task)
{
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    struct capsule_work *work;

    stage_clear();
    /* @hack redundant: work, set and never read. SHC drops the read, but
     * pushes task across stage_clear for it and drops it after (MOV.L
     * R4,@-R15 at 0x8c065ce6, ADD #4,R15 at 0x8c065cf0). without it, SHC
     * expands the function into capsule_clear and 12 of 12 words differ */
    work = (struct capsule_work *)task->work;
    sound_play(SE_CLEAR, 0, 0, 0);
}

static void capsule_wait(struct task *task)
{
    placement_try_leave(task);
}

static void capsule_break(struct task *task)
{
    struct capsule_work *work = (struct capsule_work *)task->work;

    work->mode++;
    capsule_shatter(task);
    capsule_release_animals(&work->pos);
    time_stop();
    pad_disable_player(PAD_BOTH_PLAYERS);
    work->wait = CLEAR_DELAY;
}

static void capsule_clear(struct task *task)
{
    struct capsule_work *work = (struct capsule_work *)task->work;

    if (--work->wait == 0) {
        work->mode++;
        capsule_clear_stage(task);
    }
    capsule_done(task);
}

static void capsule_done(struct task *task)
{
    struct capsule_work *work = (struct capsule_work *)task->work;

    if (work->pieces != NULL) {
        capsule_pieces_move(task);
    }
}

static void capsule_step(struct task *task)
{
    struct capsule_work *work = (struct capsule_work *)task->work;

    if (!placement_editing(task)) {
        task_step_children(task);
    }
    capsule_modes[work->mode](task);
    capsule_draw(task);
    collision_register((struct task_work *)work);
}

static void capsule_draw(struct task *task)
{
    struct capsule_work *work = (struct capsule_work *)task->work;

    if (frame_step == 0) {
        texture_set_regular();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        if (work->mode == CAPSULE_WAIT) {
            draw_model(&capsule_body_model);
        } else if (work->pieces != NULL) {
            capsule_pieces_draw(task);
        }
        draw_model(&capsule_base_model);
        njPopMatrix(1);
    }
}

static void capsule_destroy(struct task *task)
{
    capsule_pieces_free(task);
    capsule_task = NULL;
}

static void capsule_button_start(struct task *task)
{
    struct button_work *work = (struct button_work *)task->work;

    work->mode = BUTTON_UP;
    collision_init(task, &capsule_button_shape, COLLISION_OBJECT, 1, 1);
    task->draw = capsule_button_draw;
    task->destructor = capsule_button_destroy;
    task->step = capsule_button_step;
}

/* the player that stands on the button: player 0, or player 1 too when
 * the character is Tails. -1 for none */
static int capsule_button_find_player(struct task *task)
{
    struct button_work *work = (struct button_work *)task->work;
    NJS_POINT3 *pos = &work->pos;
    int last = get_character() == CHARACTER_TAILS;
    int i;
    NJS_POINT3 point;

    for (i = 0; i <= last; i++) {
        if (player_get_history(i, 0, &point, NULL)) {
            if (point_distance_squared_across(&point, pos) <
                    BUTTON_REACH * BUTTON_REACH &&
                point.y > pos->y + BUTTON_TOP &&
                point.y < pos->y + BUTTON_STAND) {
                return i;
            }
        }
    }
    return -1;
}

static void capsule_button_step(struct task *task)
{
    struct button_work *work = (struct button_work *)task->work;
    struct capsule_work *capsule_work;

    if (!placement_editing(task)) {
        capsule_work = (struct capsule_work *)task->parent->work;
        if (work->mode >= BUTTON_PUSHED ||
            (capsule_work->player = capsule_button_find_player(task)) >= 0) {
            if (work->mode == BUTTON_UP) {
                pad_disable_player(PAD_BOTH_PLAYERS);
                player_clear_speed(player_motion[0], player_get(0));
                player_clear_speed(player_motion[1], player_get(1));
                work->mode = BUTTON_PUSHED;
                sound_play(SE_SWITCH, 0, 0, 0);
            }
            work->depth += BUTTON_SINK;
            if (work->depth < BUTTON_DEPTH) {
                work->depth = BUTTON_DEPTH;
                if (work->mode == BUTTON_PUSHED) {
                    /* @hack redundant: the capsule's work read from the parent
                     * again, which the image loads again (MOV.L @(8,R13),R11
                     * and MOV.L @(32,R11),R11 at 0x8c06603a, 0x8c06603c).
                     * without it, 98 of 118 words differ */
                    capsule_work = (struct capsule_work *)task->parent->work;
                    if (capsule_work->mode < CAPSULE_BREAK) {
                        capsule_work->mode = CAPSULE_BREAK;
                    }
                    work->mode = BUTTON_DOWN;
                    vibration_start(capsule_work->player, 6, 30, 0);
                }
            }
        } else {
            capsule_button_hold(0, &work->pos);
        }
        /* @hack spelling: the capsule's work read from the parent again,
         * not through capsule_work, which the image loads again (0x8c0660a8,
         * 0x8c0660b0). through capsule_work, 108 of 118 words differ */
        work->pos.y =
            ((struct capsule_work *)task->parent->work)->pos.y + work->depth;
    }
    collision_register((struct task_work *)work);
    capsule_button_draw(task);
    /* @hack redundant: a return where the function ends, which keeps the
     * image's BRA to capsule_button_draw (0x8c0660cc). without it, SHC drops
     * the BRA and runs on into capsule_button_draw, and 2 of 118 words differ
     */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

static void capsule_button_draw(struct task *task)
{
    struct button_work *work = (struct button_work *)task->work;

    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_model(&capsule_button_model);
    njPopMatrix(1);
}

static void capsule_button_destroy(struct task *task)
{
    struct capsule_work *capsule_work =
        (struct capsule_work *)task->parent->work;

    capsule_work->button = NULL;
}

static void capsule_shatter(struct task *task)
{
    struct capsule_work *work = (struct capsule_work *)task->work;

    work->pieces = capsule_cut(&capsule_body_model);
    work->alpha = 255.0F;
    work->pieces_mode = PIECES_FLY;
    work->timer = PIECES_TIME;
}

static void capsule_pieces_move(struct task *task)
{
    struct capsule_work *work = (struct capsule_work *)task->work;
    struct capsule_piece *piece = work->pieces;
    int count;

    if (work->pieces_mode == PIECES_FLY) {
        count = piece->count;
        do {
            njAddVector(&piece->pos, &piece->speed);
            piece->speed.y -= PIECE_FALL;
            piece->ang.x += piece->spin;
            piece++;
        } while (--count != 0);
        work->alpha -= PIECE_FADE;
        if (--work->timer == 0) {
            work->pieces_mode = PIECES_GONE;
            capsule_pieces_free(task);
        }
    }
}

static void capsule_pieces_draw(struct task *task)
{
    struct capsule_work *work = (struct capsule_work *)task->work;
    struct capsule_piece *piece = work->pieces;
    int count = piece->count;
    NJS_COLOR colors[3];
    NJS_POINT3COL triangle;
    Uint32 texture = -1;
    int i;

    /* @hack spelling: the three channels stored in one chain, which stores b
     * first and keeps the 3 in R12 and the -1 in R13 (0x8c066238, 0x8c06621e,
     * 0x8c066222). as three statements, r goes first, the two take R13 and
     * R11, and 29 of 110 words differ */
    for (i = 0; i < 3; i++) {
        colors[i].argb.a = work->alpha;
        colors[i].argb.r = colors[i].argb.g = colors[i].argb.b = 0xff;
    }
    triangle.col = colors;
    triangle.num = 3;
    do {
        if (texture != piece->texture) {
            texture = piece->texture;
            njSetTextureNum(texture);
        }
        njPushMatrix(NULL);
        njTranslateV(NULL, &piece->pos);
        njRotateXYZ(NULL, piece->ang.x, piece->ang.y, piece->ang.z);
        triangle.tex = piece->uv;
        triangle.p = piece->points;
        njDrawTriangle3D(&triangle, 1,
                         NJD_USE_TEXTURE | NJD_TRANSPARENT | NJD_FILL);
        njPopMatrix(1);
        piece++;
    } while (--count != 0);
}

static void capsule_pieces_free(struct task *task)
{
    struct capsule_work *work = (struct capsule_work *)task->work;

    if (work->pieces != NULL) {
        heap_free(work->pieces);
        work->pieces = NULL;
    }
}

/* the triangles in a meshset's strips. the meshset must hold a strip */
static int capsule_meshset_triangles(NJS_MESHSET *meshset)
{
    int strips = meshset->nbMesh;
    int total = 0;
    Sint16 *strip = meshset->meshes;

    do {
        int triangles = (*strip & STRIP_LENGTH) - 2;

        total += triangles;
        strip += triangles + 3;
    } while (--strips != 0);
    return total;
}

/* the triangles in a model's meshsets. the model must hold a meshset */
static int capsule_model_triangles(NJS_MODEL *model)
{
    int meshsets = model->nbMeshset;
    int total = 0;
    NJS_MESHSET *meshset = model->meshsets;

    do {
        total += capsule_meshset_triangles(meshset);
        meshset++;
    } while (--meshsets != 0);
    return total;
}

/* fill a piece for each triangle of the meshset's strips, from piece on.
 * return the piece after the last. each piece flies out from the model's
 * origin. each piece's count is *count, which counts down by one a
 * piece */
static struct capsule_piece *capsule_cut_meshset(NJS_MODEL *model,
                                                 NJS_MESHSET *meshset,
                                                 struct capsule_piece *piece,
                                                 int *count)
{
    int strips = meshset->nbMesh;
    Sint16 *strip = meshset->meshes;
    NJS_COLOR *uv = meshset->vertuv;
    Uint32 texture =
        model->mats[meshset->type_matId & MATERIAL_INDEX].attr_texId &
        TEXTURE_NUMBER;
    int triangles;
    int i;
    NJS_POINT3 *points;
    NJS_POINT3 center;
    Float scale;
    Angle spin;

    /* @hack spelling: a for (;;) whose break tests strips at the bottom, which
     * puts texture at @(16,R15) and triangles at @(20,R15) (0x8c06640a,
     * 0x8c06641e). as a do-while, the two trade slots and 5 of 222 words
     * differ */
    for (;;) {
        triangles = (*strip++ & STRIP_LENGTH) - 2;
        do {
            points = piece->points;
            center = capsule_zero;
            for (i = 0; i < 3; i++) {
                int index = strip[i];

                /* @hack redundant: a loop of one pass around the copy, which
                 * has SHC walk the corners with two pointers, R13 for the
                 * stores and R12 for njAddVector (0x8c066438, 0x8c066436).
                 * without it, the stack frame is 4 bytes smaller and 206 of
                 * 222 words differ */
                do {
                    points[i].x = model->points[index].x;
                    points[i].y = model->points[index].y;
                    points[i].z = model->points[index].z;
                } while (0);
                njAddVector(&center, &points[i]);
            }
            /* @hack spelling: center through its address, which SHC forms
             * again at each use with MOV R15 and ADD #28 (0x8c06648e,
             * 0x8c066490). as center.x, SHC reads @(R0,R15) and 115 of 222
             * words differ */
            piece->pos.x = (&center)->x * (1.0F / 3.0F);
            piece->pos.y = (&center)->y * (1.0F / 3.0F);
            piece->pos.z = (&center)->z * (1.0F / 3.0F);
            scale = 0.005F + (rand() * (0.005F * RAND_SCALE));
            spin = 182.0F + (rand() * (182.0F / 16384.0F));
            piece->speed.x = (&center)->x * scale;
            piece->speed.y = (&center)->y * scale;
            piece->speed.z = (&center)->z * scale;
            piece->spin = spin;
            /* @hack spelling: the loop counted down, which DT counts in R13
             * from the 3 in R8 (MOV R8,R13 at 0x8c0664d4, DT R13 at
             * 0x8c066508). as a for over i, 173 of 222 words differ */
            i = 3;
            do {
                njSubVector(points, &piece->pos);
                points++;
            } while (--i != 0);
            piece->ang.x = piece->ang.y = piece->ang.z = 0;
            strip++;
            piece->uv = uv;
            piece->texture = texture;
            piece->count = *count;
            (*count)--;
            /* @hack spelling: piece = piece + 1, which keeps i at @R15 and
             * the 3 in R8 (0x8c066444, 0x8c0663dc). as piece++, i takes R8,
             * the 3 becomes an immediate, the stack frame is 4 bytes
             * smaller and 206 of 222 words differ */
            piece = piece + 1;
            uv++;
        } while (--triangles != 0);
        /* @hack spelling: strip = strip + 2, which keeps model in R9, strip
         * in R10 and the 3 in R8 (0x8c0663da, 0x8c0663ea, 0x8c0663dc). as
         * +=, they take R10, R8 and R9, and 13 of 222 words differ */
        strip = strip + 2;
        if (--strips == 0) {
            break;
        }
    }
    return piece;
}

/* cut capsule_body_model into a piece for each triangle. return the
 * pieces, or NULL when the heap has no room. model only sizes them: the
 * caller passes capsule_body_model too */
static struct capsule_piece *capsule_cut(NJS_MODEL *model)
{
    int count = capsule_model_triangles(model);
    struct capsule_piece *pieces = NULL;
    struct capsule_piece *piece;
    NJS_MODEL *body;
    NJS_MESHSET *meshset;
    int meshsets;

    piece = heap_alloc(count * sizeof(struct capsule_piece));
    if (piece != NULL) {
        pieces = piece;
        body = &capsule_body_model;
        meshsets = body->nbMeshset;
        meshset = body->meshsets;
        do {
            piece = capsule_cut_meshset(body, meshset, piece, &count);
            meshset++;
        } while (--meshsets != 0);
    }
    return pieces;
}

/* turn off the player's pad while it is over the button at pos.
 * elsewhere, turn the pad on again */
static void capsule_button_hold(int player, NJS_POINT3 *pos)
{
    NJS_POINT3 point;
    struct angles ang;
    Float dx;
    Float dz;
    Float sin;
    Float cos;

    player_get_history(player, 0, &point, &ang);
    dx = point.x - pos->x;
    dz = point.z - pos->z;
    if ((dx * dx) + (dz * dz) > BUTTON_REACH * BUTTON_REACH ||
        pos->y + BUTTON_TOP > point.y || point.y > pos->y + BUTTON_HIGH) {
        pad_enable_player(player);
        /* @hack spelling: a return in the first arm, not an else, which loads
         * pad_disable_player's address into R3, cos's address into R2 and
         * sin's into R3 (0x8c06668c to 0x8c066698). as an else, R2 and R3
         * trade each and 7 of 69 words differ */
        return;
    }
    pad_disable_player(player);
    /* @hack redundant: sin and cos, set and never read, which keep the
     * image's FSCA and its two stores (0x8c06669c to 0x8c0666a0). without
     * them, 66 of 69 words differ */
    _builtin_fsca(ang.y, &sin, &cos);
}

/* release three of the stage's animals around pos */
static void capsule_release_animals(NJS_POINT3 *pos)
{
    /* @hack spelling: the loop counted down from 3, which DT counts in R11
     * (MOV #3,R11 at 0x8c066700, DT R11 at 0x8c066732). as a for over i,
     * SHC unrolls it and 18 of 53 words differ */
    int i = 3;
    Angle ang = 0;
    Float sin;
    Float cos;

    do {
        _builtin_fsca(ang, &sin, &cos);
        ang += ANIMAL_SPACING;
        animal_create_random(0, pos->x + (sin * ANIMAL_RADIUS),
                             pos->y + ANIMAL_HEIGHT,
                             pos->z + (cos * ANIMAL_RADIUS));
    } while (--i != 0);
}
