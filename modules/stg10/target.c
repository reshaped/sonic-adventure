/* targets: boards with a character on them that break into pieces when hit,
 * and their box. O Target's board rides a cart back and forth. O UP TARGET1's
 * rises from a box that opens as player 0 comes near. 0 UP TARGET2, spelled
 * with a zero, is the box alone, shut */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/glare.h"
#include "enemy/enemy.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/e102_bonus.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg10/pom.h"
#include "stg10/target.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT target_cart_object;

#define SE_DOORS_OPENED 283
#define SE_RUMBLE 284
#define SE_HIT 285

/* the box's doors shrink along z to OPEN_DEPTH of their depth in OPEN_STEPS
 * steps. the board then rises RISE_STEP a step for RISE_STEPS steps. a pop-up
 * whose board broke before starts open and risen */
#define OPEN_DEPTH 0.1F
#define OPEN_STEPS 7
#define RISE_STEP 2.5F
#define RISE_STEPS 12
#define RISE (RISE_STEP * RISE_STEPS)
/* a box opens once player 0 comes within scale x of it. a scale x below
 * MIN_RANGE takes DEFAULT_RANGE instead */
#define MIN_RANGE 0.1F
#define DEFAULT_RANGE 100.0F

/* the cart moves CART_SPEED units a step, and its wheel turns WHEEL_TURN a
 * unit. the stage clears FINISH_STEPS steps after the run ends at
 * MOVING_DONE */
#define CART_SPEED 0.5F
#define WHEEL_TURN 2454.2073F
#define FINISH_STEPS 30

#define SWING_STEP 0x400 /* the swing's phase step, a turn each 64 steps */
/* the board's tilt at the swing's peak, an eighth of a turn */
#define SWING_TILT 8192.0F
#define SWING_KEPT 0.99F /* the share of its size the swing keeps a step */
/* the cart leaves the far end of its path once the swing's phase is within
 * SWING_WINDOW past 0. it leaves the start within SWING_WINDOW past
 * START_MARK, a half turn */
#define SWING_WINDOW 0x800
#define START_MARK 0x8000

#define GLARE_STEP 0x400 /* the glare's turn a step */

#define POP_UP_HITS 1 /* the hits a pop-up's board takes */
#define CART_HITS 3 /* the hits a cart's board takes */
/* the pieces a cart's board drops at each of its first two hits */
#define HIT_PIECES 2

/* a cart's board is turned a quarter back about y, and twice a pop-up's
 * size */
#define CART_BOARD_TURN (-0x4000)
#define CART_BOARD_SCALE 2.0F

/* a falling piece turns PIECE_TURN about x a step and gains GRAVITY of fall
 * speed a step. it is gone after PIECE_STEPS steps */
#define PIECE_TURN 0x38e
#define GRAVITY 0.027222222F
#define PIECE_STEPS 120
/* a piece's speed once it falls is PIECE_SPREAD times its place on the
 * board, plus PIECE_PUSH along z */
#define PIECE_SPREAD 0.3F
#define PIECE_PUSH (-1.0F)

/* the character on a board. scale y picks one, 1 for FACE_SONIC to 3 for
 * FACE_TAILS */
enum target_face { FACE_SONIC, FACE_KNUCKLES, FACE_TAILS };

/* a pop-up's modes; a cart takes the last two */
enum target_mode {
    TARGET_WAIT, /* for player 0 to come within range */
    TARGET_OPEN,
    TARGET_RISE,
    TARGET_UP, /* risen, with its glare turning */
    TARGET_DONE, /* its board broken, now or before */
    TARGET_RIDING /* a cart whose board stands */
};

/* the moving target's modes. at each end of its path the cart waits for the
 * swing's phase to come round, then starts back with a fresh swing */
enum moving_target_mode {
    MOVING_OUT,
    MOVING_AT_END,
    MOVING_BACK,
    MOVING_AT_START,
    MOVING_DONE /* its cart's board broken */
};

/* what target_draw_tree does at a node, by the node's place in its tree */
enum target_node {
    NODE_PLAIN,
    NODE_DOORS, /* scale along z by the doors' scale */
    NODE_RISER, /* raise the board */
    NODE_MOUNT, /* tilt back and draw the board */
    NODE_WHEEL /* turn about x by the wheel's turn */
};

enum target_piece_state { PIECE_ON, PIECE_FALLING, PIECE_GONE };

/* the pop-up's and the cart's view of their task_work, which must keep
 * task_work's fields at their offsets */
struct target_work {
    Sint8 mode;
    Sint8 face; /* the board's: a pop-up's from scale y, a cart's FACE_SONIC */
    Uint8 id;
    Sint8 on_cart; /* 1 for a cart, 0 for a pop-up */
    Sint16 flags;
    Sint16 timer;
    Float rise; /* how far the board has risen */
    struct task *board; /* null once it is gone */
    Angle glare; /* the glare's turn */
    struct angles ang;
    NJS_POINT3 pos;
    Float range; /* scale x: how near player 0 comes to open the box */
    /* scale y: 1 to 3 picks the face, and a value outside 1 to 3.99 one at
     * random */
    Float choice;
    union {
        Float doors; /* a pop-up's doors' scale along z */
        Angle wheel; /* a cart's wheel's turn */
    } scl_z;
    struct collision_work *collision;
    struct event_work *event;
};

/* the moving target's view of its task_work */
struct moving_target_work {
    Sint8 mode;
    Uint8 unk_01;
    Uint8 id;
    Uint8 unk_03;
    Sint16 flags;
    Sint16 timer;
    struct task *cart; /* null once it is gone */
    Float swing; /* the swing's size, which dies away */
    Angle phase; /* the swing's phase */
    struct angles ang;
    NJS_POINT3 pos; /* the path's start; it runs along z, turned by angle y */
    Float length; /* scale x: the path's length */
    Float along; /* scale y: how far along it the cart is, 0 to 1 */
    Uint8 unk_34[4];
    struct collision_work *collision;
    struct event_work *event;
};

struct target_piece {
    NJS_MODEL *model;
    union point_floats pos;
    union point_floats speed;
    struct angles ang;
    Uint8 unk_28[4];
    Sint32 state;
    Sint32 timer; /* the steps it has left to fall */
};

/* the board's view of its task_work */
struct target_board_work {
    Sint8 mode;
    Sint8 hits; /* the hits left before it breaks */
    Uint8 id;
    Uint8 unk_03;
    Sint16 flags;
    Sint16 count; /* how many pieces it has */
    struct target_piece *pieces; /* from the heap, which its destructor frees */
    /* target_board_draw_all, though no code calls through it */
    void (*draw_all)(struct task *task);
    Uint8 unk_10[4];
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT3 scl;
    struct collision_work *collision;
    struct event_work *event;
};

static void target_draw_held_board(struct task *task);
static void target_draw_tree(struct task *task, NJS_OBJECT *object, int *node);
static void target_spin_glare(struct task *task);
static void target_draw_glare(struct target_work *work, NJS_POINT3 *from);
static void target_cart_start(struct task *task);
static void moving_target_swing(struct moving_target_work *work);
static void moving_target_travel(struct task *task);
static void moving_target_path_point(struct moving_target_work *work,
                                     Float along, NJS_POINT3 *point);
static void moving_target_finish(struct task *task);
static void moving_target_run(struct task *task);
static void moving_target_destroy(struct task *task);
static void target_cart_place(struct task *task, NJS_POINT3 *point, int wheel,
                              Angle tilt);
static void target_board_follow(struct task *task, Angle tilt);
static void target_cart_step(struct task *task);
static void target_turn_wheel(struct task *task);
static void target_cart_draw(struct task *task);
static void target_cart_destroy(struct task *task);
static void pop_up_target_wait(struct task *task);
static void pop_up_target_open(struct task *task);
static void pop_up_target_rise(struct task *task);
static void pop_up_target_run(struct task *task);
static void target_scale_doors(struct task *task);
static void target_raise_board(struct task *task);
static void target_mount_board(struct task *task);
static void pop_up_target_draw(struct task *task);
static void target_box_destroy(struct task *task);
static void target_board_draw(struct task *task);
static void target_board_start(struct task *task);
static void target_board_fall(struct task *task);
static void target_board_release(struct target_piece *piece, int count);
static void target_board_step(struct task *task);
static void target_board_draw_all(struct task *task);
static void target_board_destroy(struct task *task);
static void target_board_fill(struct target_piece **piece, NJS_OBJECT *object);
static void target_board_alloc(struct task *task);
static void target_box_run(struct task *task);

/* the pieces of each face's board, one for each node of its tree */
static const Sint8 target_face_pieces[3] = {9, 9, 11};
/* the indexes of the pieces a cart's board drops at its first hit, then at
 * its second */
static const Sint8 target_hit_pieces[4] = {3, 7, 5, 8};
static void (*const target_node_draws[5])(struct task *task) = {
    NULL, target_scale_doors, target_raise_board, target_mount_board,
    target_turn_wheel};
/* each node's enum target_node, by its place in the box's tree and in the
 * cart's. target_nodes picks a table by on_cart */
static const Sint8 target_box_nodes[6] = {NODE_PLAIN, NODE_DOORS, NODE_DOORS,
                                          NODE_RISER, NODE_PLAIN, NODE_MOUNT};
static const Sint8 target_cart_nodes[6] = {NODE_PLAIN, NODE_PLAIN, NODE_WHEEL,
                                           NODE_PLAIN, NODE_PLAIN, NODE_PLAIN};
static const Sint8 *const target_nodes[2] = {target_box_nodes,
                                             target_cart_nodes};
static const NJS_VECTOR target_glare_dir = {0.0F, 0.0F, 1.0F};

/* the set editor's labels for scale y, by face, then for a face at random */
static const char *target_face_labels[4] = {"<- SONIC", "<- KNUCKLES",
                                            "<- TALES", "<- RANDOM"};
static NJS_OBJECT *target_faces[3] = {
    &target_sonic_object, &target_knuckles_object, &target_tails_object};
static struct collision_shape target_box_shapes[1] = {
    {SHAPE_CYLINDER2 | SHAPE_TURN_YZX,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {-0.5F, 5.0F, -0.5F},
     8.0F,
     5.0F,
     12.5F,
     {0},
     0,
     NULL},
};
static struct collision_shape target_board_shapes[1] = {
    {SHAPE_CAPSULE | SHAPE_TURN_YZX | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 3.0F},
     5.0F,
     6.0F,
     0.0F,
     {0},
     0,
     NULL},
};
static NJS_ARGB target_glare_color = {0.4F, 1.0F, 0.2F, 0.4F};
/* where the glare shines from, in the cart's frame and the box's */
static NJS_POINT3 target_cart_glare_from = {9.099814F, 15.50146F, -0.01807F};
static NJS_POINT3 target_box_glare_from = {0.0F, 36.5F, -6.5F};

/* the box's run is its step and its draw both */
void target_box_step(struct task *task)
{
    collision_init(task, target_box_shapes, COLLISION_OBJECT, 1, 1);
    task->step = target_box_run;
    task->draw = task->step;
    task->destructor = target_box_destroy;
}

/* O Target's task is the path. it makes its cart as a child, and the cart
 * makes its board unless the board broke before */
void moving_target_step(struct task *task)
{
    struct moving_target_work *work = (struct moving_target_work *)task->work;

    /* the work must keep task_work's own fields where task_work does */
    /* @hack redundant: pos is tested twice, as the image's first and fifth
     * compares both test its offset (0x0c913c9e, 0x0c913cd0); tested once,
     * 33 of 63 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct moving_target_work, pos) ||
        !WORK_SAME_PLACE(struct moving_target_work, id) ||
        !WORK_SAME_PLACE(struct moving_target_work, collision) ||
        !WORK_SAME_PLACE(struct moving_target_work, event) ||
        !WORK_SAME_PLACE(struct moving_target_work, pos) ||
        !WORK_SAME_PLACE(struct moving_target_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    work->mode = MOVING_OUT;
    work->ang.z = 0;
    work->cart = task_create_child(TASK_WORK, target_cart_start, task);
    target_cart_start(work->cart);
    task->step = moving_target_run;
    task->draw = NULL;
    task->destructor = moving_target_destroy;
}

void pop_up_target_step(struct task *task)
{
    struct target_work *work = (struct target_work *)task->work;

    /* @hack redundant: pos is tested twice, as the image's first and fifth
     * compares both test its offset (0x0c913d1c, 0x0c913d5e); tested once,
     * target.c's code runs 32 bytes short and 105 of 146 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct target_work, pos) ||
        !WORK_SAME_PLACE(struct target_work, id) ||
        !WORK_SAME_PLACE(struct target_work, collision) ||
        !WORK_SAME_PLACE(struct target_work, event) ||
        !WORK_SAME_PLACE(struct target_work, pos) ||
        !WORK_SAME_PLACE(struct target_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    collision_init(task, target_box_shapes, COLLISION_OBJECT, 1, 1);
    if (placement_used(task)) {
        work->mode = TARGET_DONE;
        work->rise = RISE;
        work->scl_z.doors = OPEN_DEPTH;
    } else {
        work->mode = TARGET_WAIT;
        work->rise = 0.0F;
        work->scl_z.doors = 1.0F;
        if (work->range < MIN_RANGE) {
            work->range = DEFAULT_RANGE;
        }
        work->on_cart = 0;
        if (work->choice < 1.0F || work->choice > 3.99F) {
            work->face = rand() * (3.0F * RAND_SCALE);
        } else {
            work->face = (Sint8)work->choice - 1.0F;
        }
        work->board = task_create_child(TASK_WORK, target_board_start, task);
    }
    task->step = pop_up_target_run;
    task->draw = pop_up_target_draw;
    task->destructor = target_box_destroy;
}

static void target_draw_held_board(struct task *task)
{
    struct target_work *work = (struct target_work *)task->work;

    if (work->board) {
        target_board_draw(work->board);
    }
}

/* draw object's tree in the current matrix. *node is object's place in the
 * whole tree, depth first. it picks the target_node_draws entry that runs
 * before object's model */
static void target_draw_tree(struct task *task, NJS_OBJECT *object, int *node)
{
    struct target_work *work = (struct target_work *)task->work;
    Sint8 kind;

    njPushMatrix(NULL);
    kind = target_nodes[work->on_cart][*node];
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    if (target_node_draws[kind]) {
        target_node_draws[kind](task);
    }
    if (object->model) {
        draw_model(object->model);
    }
    if (object->child) {
        ++*node;
        target_draw_tree(task, object->child, node);
    }
    njPopMatrix(1);
    if (object->sibling) {
        ++*node;
        target_draw_tree(task, object->sibling, node);
    }
}

static void target_spin_glare(struct task *task)
{
    struct target_work *work = (struct target_work *)task->work;

    work->glare = (Uint16)(work->glare + GLARE_STEP);
}

/* draw a glare at from, a point in the box's frame or the cart's. it shines
 * along z, turned about y by work->glare */
static void target_draw_glare(struct target_work *work, NJS_POINT3 *from)
{
    NJS_POINT3 pos;
    NJS_VECTOR dir;
    Angle ang;

    dir = target_glare_dir;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    if (work->on_cart == 0) {
        TURN_VIA(njRotateZ, work->ang.z, ang);
    }
    njCalcPoint(NULL, from, &pos);
    njUnitMatrix(NULL);
    TURN_VIA(njRotateY, work->glare, ang);
    njCalcVector(NULL, &dir, &dir);
    njPopMatrix(1);
    glare_draw(&pos, &dir, 1.0F, &target_glare_color);
}

static void target_cart_start(struct task *task)
{
    struct target_work *work = (struct target_work *)task->work;

    /* @hack redundant: pos is tested twice, as the image's first and fifth
     * compares both test its offset (0x0c914092, 0x0c9140c4); tested once,
     * 43 of 72 words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct target_work, pos) ||
        !WORK_SAME_PLACE(struct target_work, id) ||
        !WORK_SAME_PLACE(struct target_work, collision) ||
        !WORK_SAME_PLACE(struct target_work, event) ||
        !WORK_SAME_PLACE(struct target_work, pos) ||
        !WORK_SAME_PLACE(struct target_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    work->face = FACE_SONIC;
    work->on_cart = 1;
    if (placement_used(task->parent)) {
        work->mode = TARGET_DONE;
    } else {
        work->mode = TARGET_RIDING;
        work->board = task_create_child(TASK_WORK, target_board_start, task);
    }
    task->step = target_cart_step;
    task->draw = target_cart_draw;
    task->destructor = target_cart_destroy;
}

/* the board's tilt: a swing that dies away */
static void moving_target_swing(struct moving_target_work *work)
{
    Float sine;
    Float cosine;

    /* @hack spelling: the phase stepped and masked in two statements. SHC then
     * ANDs it with 0xffff from the pool (AND R5,R2 at 0x0c914150), and reads
     * it back for the sine. as one, SHC masks it with EXTU.W, the function
     * runs 2 bytes short and 34 of 36 words differ */
    work->phase += SWING_STEP;
    work->phase &= 0xffff;
    _builtin_fsca(work->phase, &sine, &cosine);
    work->ang.z = (Angle)(work->swing * sine * SWING_TILT) & 0xffff;
    work->swing *= SWING_KEPT;
}

static void moving_target_travel(struct task *task)
{
    struct moving_target_work *work = (struct moving_target_work *)task->work;

    switch (work->mode) {
    case MOVING_OUT:
        work->along += CART_SPEED / work->length;
        if (work->along > 1.0F) {
            sound_stop(SE_RUMBLE);
            work->along = 1.0F;
            ++work->mode;
        }
        break;
    case MOVING_AT_END:
        if (work->phase > 0 && work->phase < SWING_WINDOW) {
            sound_play(SE_RUMBLE, 0, 0, 0);
            work->swing = 1.0F;
            ++work->mode;
        }
        break;
    case MOVING_BACK:
        work->along -= CART_SPEED / work->length;
        if (work->along < 0.0F) {
            sound_stop(SE_RUMBLE);
            work->along = 0.0F;
            ++work->mode;
        }
        break;
    case MOVING_AT_START:
        if (work->phase > START_MARK &&
            work->phase < START_MARK + SWING_WINDOW) {
            sound_play(SE_RUMBLE, 0, 0, 0);
            work->swing = 1.0F;
            work->mode = MOVING_OUT;
        }
        break;
    case MOVING_DONE:
        work->timer = FINISH_STEPS;
        sound_stop(SE_RUMBLE);
        task->step = moving_target_finish;
        break;
    }
    moving_target_swing(work);
}

/* set *point to the point at along on the path, a share of its length from
 * the start */
static void moving_target_path_point(struct moving_target_work *work,
                                     Float along, NJS_POINT3 *point)
{
    Angle ang;

    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslate(NULL, 0.0F, 0.0F, work->length * along);
    njGetTranslation(NULL, point);
    njPopMatrix(1);
}

static void moving_target_finish(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (work->timer > 0) {
            if (--work->timer == 0) {
                stage_clear();
            }
        }
        task_step_children(task);
    }
}

/* move the cart and the board, and in the set editor draw the path */
static void moving_target_run(struct task *task)
{
    struct moving_target_work *work = (struct moving_target_work *)task->work;
    NJS_POINT3 point;
    NJS_POINT3 end;

    if (!placement_try_leave(task)) {
        moving_target_travel(task);
        moving_target_path_point(work, work->along, &point);
        target_cart_place(work->cart, &point,
                          (int)(work->length * work->along * WHEEL_TURN),
                          work->ang.z);
        if (placement_editing(task)) {
            moving_target_path_point(work, 1.0F, &end);
            draw_line(work->pos.x, work->pos.y, work->pos.z, end.x, end.y,
                      end.z);
            work->cart->work->ang = work->ang;
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<-PATH LENGTH");
            njPrintColor(0xffff0000);
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "(NO EDIT)");
            njPrintColor(0xffc0c0c0);
        }
        task_step_children(task);
    }
}

static void moving_target_destroy(struct task *task)
{
    sound_stop(SE_RUMBLE);
}

/* put the cart, task, at point with its wheel turned to wheel. its board
 * follows, tilted by tilt. a cart whose board is broken ends its moving
 * target's run instead. a NULL cart does nothing */
static void target_cart_place(struct task *task, NJS_POINT3 *point, int wheel,
                              Angle tilt)
{
    if (task) {
        struct target_work *work = (struct target_work *)task->work;

        if (work->mode != TARGET_DONE) {
            work->pos = *point;
            work->scl_z.wheel = wheel;
            target_board_follow(work->board, tilt);
        } else {
            struct moving_target_work *parent =
                (struct moving_target_work *)task->parent->work;

            parent->mode = MOVING_DONE;
        }
    }
}

static void target_board_follow(struct task *task, Angle tilt)
{
    if (task) {
        struct task_work *work = task->work;

        /* @bug logic: nothing sets the board's mode, which stays 0, so
         * this test for TARGET_DONE always holds (CMP/EQ #4,R0 at
         * 0x0c9144e8) */
        if (work->mode != TARGET_DONE) {
            struct task_work *parent = task->parent->work;

            work->ang.z = tilt;
            work->pos = parent->pos;
        }
    }
}

/* the cart's step draws it too */
static void target_cart_step(struct task *task)
{
    target_spin_glare(task);
    task_step_children(task);
    target_cart_draw(task);
}

static void target_turn_wheel(struct task *task)
{
    struct target_work *work = (struct target_work *)task->work;
    Angle ang;

    TURN_VIA(njRotateX, work->scl_z.wheel, ang);
}

/* the cart with its board, and the cart's glare while the board stands */
static void target_cart_draw(struct task *task)
{
    struct target_work *work = (struct target_work *)task->work;
    int node;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    node = 0;
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    target_draw_tree(task, &target_cart_object, &node);
    njRotateY(NULL, CART_BOARD_TURN);
    njScale(NULL, CART_BOARD_SCALE, CART_BOARD_SCALE, CART_BOARD_SCALE);
    target_draw_held_board(task);
    njPopMatrix(1);
    if (work->mode != TARGET_DONE) {
        target_draw_glare(work, &target_cart_glare_from);
    }
}

static void target_cart_destroy(struct task *task)
{
    struct moving_target_work *parent =
        (struct moving_target_work *)task->parent->work;

    parent->cart = NULL;
}

static void pop_up_target_wait(struct task *task)
{
    struct target_work *work = (struct target_work *)task->work;
    NJS_POINT3 pos;
    Float distance;

    player_get_history(0, 0, &pos, NULL);
    njSubVector(&pos, &work->pos);
    distance = njScalor2(&pos);
    if (distance < work->range * work->range) {
        work->timer = OPEN_STEPS;
        ++work->mode;
    }
}

static void pop_up_target_open(struct task *task)
{
    struct target_work *work = (struct target_work *)task->work;

    work->scl_z.doors += -(1.0F - OPEN_DEPTH) / OPEN_STEPS;
    if (--work->timer == 0) {
        work->timer = RISE_STEPS;
        sound_play(SE_DOORS_OPENED, 0, 0, 0);
        ++work->mode;
    }
}

static void pop_up_target_rise(struct task *task)
{
    struct target_work *work = (struct target_work *)task->work;

    work->rise += RISE_STEP;
    if (--work->timer == 0) {
        ++work->mode;
    }
}

/* each pop-up mode's step, by enum target_mode. the image's constant
 * section, C, holds it after moving_target_run's strings, so it is defined
 * after that function */
static void (*const pop_up_target_modes[5])(struct task *task) = {
    pop_up_target_wait, pop_up_target_open, pop_up_target_rise,
    target_spin_glare, NULL};

/* the pop-up's step draws it too */
static void pop_up_target_run(struct task *task)
{
    struct target_work *work = (struct target_work *)task->work;
    int choice;

    if (!placement_try_leave(task)) {
        if (pop_up_target_modes[work->mode]) {
            pop_up_target_modes[work->mode](task);
        }
        collision_register((struct task_work *)work);
        pop_up_target_draw(task);
        task_step_children(task);
        if (placement_editing(task)) {
            choice = (int)work->choice;
            /* angle y's label says X, though angle y turns the box about y */
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "<-Angle X");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "<-Angle Z");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- OPEN RANGE");
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y),
                     target_face_labels[(choice < 1.0F || choice > 4.0F)
                                            ? 3
                                            : choice - 1]);
            njPrintColor(0xffff0000);
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<- ANIME  (NO EDIT)");
            njPrintColor(0xffc0c0c0);
        }
    }
}

static void target_scale_doors(struct task *task)
{
    struct target_work *work = (struct target_work *)task->work;

    njScale(NULL, 1.0F, 1.0F, work->scl_z.doors);
}

static void target_raise_board(struct task *task)
{
    struct target_work *work = (struct target_work *)task->work;

    njTranslate(NULL, 0.0F, work->rise, 0.0F);
}

/* tilt back by angle z, so the board stands as the box tilts */
static void target_mount_board(struct task *task)
{
    struct target_work *work = (struct target_work *)task->work;
    Angle ang;

    TURN_VIA(njRotateZ, -work->ang.z, ang);
    target_draw_held_board(task);
}

/* the box and its board on a frame's first step, and the glare while the
 * board is up */
static void pop_up_target_draw(struct task *task)
{
    struct target_work *work = (struct target_work *)task->work;
    int node;
    Angle ang;

    if (!frame_step) {
        texture_set_stage();
        njPushMatrix(NULL);
        node = 0;
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        target_draw_tree(task, &target_box_object, &node);
        njPopMatrix(1);
        if (work->mode == TARGET_UP) {
            target_draw_glare(work, &target_box_glare_from);
        }
    }
}

static void target_box_destroy(struct task *task)
{
}

/* the pieces not gone, in the current matrix: those still on the board
 * tilted by its angle z */
static void target_board_draw(struct task *task)
{
    struct target_board_work *work = (struct target_board_work *)task->work;
    struct target_piece *piece;
    int count;
    Angle ang;

    if (work->pieces) {
        count = work->count;
        piece = work->pieces;
        do {
            int state = piece->state;

            if (state < PIECE_GONE) {
                njPushMatrix(NULL);
                if (state == PIECE_ON) {
                    TURN_VIA(njRotateZ, work->ang.z, ang);
                }
                njTranslateV(NULL, &piece->pos.vector);
                njRotateXYZ(NULL, piece->ang.x, piece->ang.y, piece->ang.z);
                draw_model(piece->model);
                njPopMatrix(1);
            }
            ++piece;
        } while (--count);
    }
}

/* a cart's board turns and scales its shape to match the board the cart
 * draws */
static void target_board_start(struct task *task)
{
    struct target_board_work *work = (struct target_board_work *)task->work;
    struct target_work *parent = (struct target_work *)task->parent->work;

    enemy_alloc_record(task, (struct task_work *)work);
    collision_init(task, target_board_shapes, COLLISION_ITEM, 1, 1);
    if (parent->on_cart == 0) {
        work->ang.x = 0;
        work->ang.z = 0;
        work->hits = POP_UP_HITS;
    } else {
        struct collision_shape *shape = work->collision->shapes;

        work->ang.y += CART_BOARD_TURN;
        shape->radius *= CART_BOARD_SCALE;
        shape->size_y *= CART_BOARD_SCALE;
        work->hits = CART_HITS;
    }
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    work->count = target_face_pieces[parent->face];
    target_board_alloc(task);
    work->draw_all = target_board_draw_all;
    task->step = target_board_step;
    task->draw = NULL;
    task->destructor = target_board_destroy;
}

/* move the falling pieces, each gone after PIECE_STEPS steps, and destroy
 * the board once all are gone */
static void target_board_fall(struct task *task)
{
    struct target_board_work *work = (struct target_board_work *)task->work;
    struct target_piece *piece = work->pieces;
    int count = work->count;
    int finished = 1;

    do {
        int state = piece->state;

        if (state == PIECE_FALLING) {
            njAddVector(&piece->pos.vector, &piece->speed.vector);
            piece->ang.x += PIECE_TURN;
            piece->speed.vector.y -= GRAVITY;
            if (--piece->timer == 0) {
                piece->state = state = PIECE_GONE;
            }
        }
        if (state != PIECE_GONE) {
            finished = 0;
        }
        ++piece;
    } while (--count);
    if (finished) {
        task_destroy(task);
    }
}

static void target_board_release(struct target_piece *piece, int count)
{
    do {
        if (piece->state == PIECE_ON) {
            piece->state = PIECE_FALLING;
            piece->timer = PIECE_STEPS;
        }
        ++piece;
    } while (--count);
}

/* where a hit's pom goes off, from the board's position. the image's
 * constant section, C, holds it after pop_up_target_run's strings, so it is
 * defined after that function */
static const NJS_POINT3 target_blast_from = {-4.5F, -10.0F, 5.0F};

/* @hack spelling: the hit counted in a helper SHC expands. SHC then keeps
 * PIECE_FALLING in R7 and PIECE_STEPS in R14 for the loop that drops a hit's
 * two pieces (0x0c914cd0, 0x0c914cd2). written in place, the two trade and 6
 * of target_board_step's 208 words differ */
static void target_count_hit(struct lockon_extra *extra)
{
    /* @hack spelling: unk_02 passed through a local, which has SHC load extra
     * from the frame into R4 (MOV.L @R15,R4 at 0x0c914c6a). passed as
     * extra->unk_02, extra loads into R1 and is copied to R4 and to a second
     * frame slot, target_board_step runs 32 bytes long and 129 of its 208
     * words differ */
    Sint8 unk = extra->unk_02;

    e102_bonus_hit(unk);
}

/* a hit from a player or an attack sets off a pom and drops pieces. the last
 * hit drops the rest. it marks as used the placement of the pop-up, or of a
 * cart's moving target. on a pop-up's board with a lock-on, the last hit also
 * counts toward E-102's volley bonus. a pop-up's board takes no hit until its
 * box rises, and then follows the riser */
static void target_board_step(struct task *task)
{
    struct target_board_work *work = (struct target_board_work *)task->work;
    struct target_work *parent = (struct target_work *)task->parent->work;
    struct collision_work *other = work->collision->hit;
    Angle ang;

    if ((work->flags & COLLISION_DAMAGED) &&
        (other->kind == COLLISION_PLAYER || other->kind == COLLISION_ATTACK)) {
        NJS_POINT3 pos;

        pos = target_blast_from;
        work->flags &= ~COLLISION_DAMAGED;
        sound_play(SE_HIT, 0, 0, 0);
        njAddVector(&pos, &work->pos);
        final_pom_create(&pos, 0.8F);
        if (--work->hits == 0) {
            parent->mode = TARGET_DONE;
            task->step = target_board_fall;
            target_board_release(work->pieces, work->count);
            if (parent->on_cart == 1) {
                placement_set_used(task->parent->parent);
            } else {
                struct lockon_target *target;
                struct task *lockon;
                struct lockon_extra *extra;

                if ((target = LOCKON_TARGET(task)) &&
                    (lockon = target->lockon) && (extra = lockon->extra)) {
                    target_count_hit(extra);
                }
                placement_set_used(task->parent);
            }
        } else {
            int i;
            int hit = CART_HITS - 1 - work->hits;
            struct target_piece *pieces = work->pieces;

            for (i = 0; i < HIT_PIECES; ++i) {
                pieces[target_hit_pieces[(hit * HIT_PIECES) + i]].state =
                    PIECE_FALLING;
                pieces[target_hit_pieces[(hit * HIT_PIECES) + i]].timer =
                    PIECE_STEPS;
            }
            target_board_fall(task);
        }
        lockon_release(task);
    } else {
        /* @hack spelling: the cart's case as a branch of its own, empty. SHC
         * then reads the parent's mode into R3, and the matrix calls' addresses
         * into R3 and then R2 (0x0c914d1a to 0x0c914d38). as one test of the
         * pop-up, on_cart != 1, R2 and R3 trade places there and 11 of 208
         * words differ */
        if (parent->on_cart == 1) {
            /* the cart places its board, in target_board_follow */
        } else if (parent->mode >= TARGET_RISE) {
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &parent->pos);
            TURN_VIA(njRotateY, parent->ang.y, ang);
            TURN_VIA(njRotateZ, parent->ang.z, ang);
            njTranslate(NULL, 0.0F, parent->rise - 10.0F, 3.0F);
            njGetTranslation(NULL, &work->pos);
            njPopMatrix(1);
        } else {
            return;
        }
        target_board_fall(task);
        collision_register((struct task_work *)work);
    }
}

/* every piece in the current matrix, whatever its state */
static void target_board_draw_all(struct task *task)
{
    struct target_board_work *work = (struct target_board_work *)task->work;
    struct target_piece *piece;
    int count;

    if (work->pieces) {
        count = work->count;
        piece = work->pieces;
        do {
            njPushMatrix(NULL);
            njTranslateV(NULL, &piece->pos.vector);
            njRotateXYZ(NULL, piece->ang.x, piece->ang.y, piece->ang.z);
            draw_model(piece->model);
            njPopMatrix(1);
            ++piece;
        } while (--count);
    }
}

static void target_board_destroy(struct task *task)
{
    struct target_work *parent = (struct target_work *)task->parent->work;
    struct target_board_work *work = (struct target_board_work *)task->work;

    if (work->pieces) {
        heap_free(work->pieces);
    }
    parent->board = NULL;
}

/* fill a piece for each node of object's tree, from *piece on, and leave
 * *piece past them. a piece starts where its node sits in the current matrix,
 * with its node's angles */
static void target_board_fill(struct target_piece **piece, NJS_OBJECT *object)
{
    (*piece)->model = object->model;
    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    njRotateXYZ(NULL, object->ang[0], object->ang[1], object->ang[2]);
    njGetTranslation(NULL, &(*piece)->pos.point);
    /* @hack spelling: the floats walked by two cursors, in a scope of their
     * own. SHC then reads pos with post-increment loads (FMOV.S @R4+,FR3 at
     * 0x0c914eb2, FMOV.S @R4+,FR2 at 0x0c914ec0). by member or by subscript,
     * the function runs 4 bytes long and 63 of 92 words differ */
    {
        Float *pos = (*piece)->pos.elements;
        Float *speed = (*piece)->speed.elements;

        *speed = *pos * PIECE_SPREAD;
        ++pos;
        ++speed;
        *speed = *pos * PIECE_SPREAD;
        ++pos;
        ++speed;
        *speed = *pos * PIECE_SPREAD;
    }
    (*piece)->speed.vector.z += PIECE_PUSH;
    (*piece)->ang.x = object->ang[0];
    (*piece)->ang.y = object->ang[1];
    (*piece)->ang.z = object->ang[2];
    ++*piece;
    if (object->child) {
        target_board_fill(piece, object->child);
    }
    njPopMatrix(1);
    if (object->sibling) {
        target_board_fill(piece, object->sibling);
    }
}

/* give the board its face's pieces, from the heap */
static void target_board_alloc(struct task *task)
{
    struct target_board_work *work = (struct target_board_work *)task->work;
    struct target_work *parent = (struct target_work *)task->parent->work;
    struct target_piece *piece;

    /* @hack spelling: the pieces assigned in the test. SHC tests them in R0
     * (TST R0,R0 at 0x0c914f30), and stores them in the branch's delay slot.
     * assigned first, they are copied to R2 to be tested, the function runs 2
     * bytes long and 24 of 36 words differ */
    /* @bug memory: when heap_calloc fails, pieces stays NULL, which only the
     * draws and the destructor test. target_board_fall then reads each
     * piece's state through it on each call (MOV.L @(44,R14),R0 at
     * 0x0c914ad4). target_board_release reads it there on the last hit
     * (0x0c914b9a). a hit that drops two pieces writes their states and
     * timers through it (0x0c914ce4, 0x0c914cee) */
    if ((work->pieces =
             heap_calloc(work->count, sizeof(struct target_piece)))) {
        piece = work->pieces;
        njPushMatrix(&_unit_matrix_);
        target_board_fill(&piece, target_faces[parent->face]);
        njPopMatrix(1);
    }
}

/* the box's step and its draw. on a frame's first step within range, it
 * draws the box with its two doors shut. it then registers its collision
 * record */
static void target_box_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = &target_box_object;
    Angle ang;

    if (!frame_step && !placement_try_leave(task)) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        draw_model(object->model);
        object = object->child;
        njPushMatrix(NULL);
        njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
        draw_model(object->model);
        njPopMatrix(1);
        object = object->sibling;
        njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
        draw_model(object->model);
        njPopMatrix(1);
        collision_register(work);
    }
}
