/* O BROOBJ and O BROCU objects that break into fading pieces */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "effect/bomb.h"
#include "enemy/enemy.h"
#include "object/placement.h"
#include "object/target.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg02/breakables.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT windy_breakable_object;
extern NJS_OBJECT windy_breakable_piece_1_object;
extern NJS_OBJECT windy_breakable_piece_2_object;
extern NJS_OBJECT windy_breakable_piece_3_object;
extern NJS_OBJECT windy_breakable_piece_4_object;
extern NJS_OBJECT windy_breakable_piece_5_object;
extern NJS_OBJECT windy_breakable_piece_6_object;
extern NJS_OBJECT windy_breakable_piece_7_object;
extern NJS_OBJECT windy_breakable_piece_8_object;
extern NJS_OBJECT windy_breakable_piece_9_object;

/* O BROCU's models, which cubes.c draws too */
extern NJS_OBJECT windy_small_cube_object;
extern NJS_OBJECT windy_medium_cube_object;
extern NJS_OBJECT windy_large_cube_object;

struct breakable_vars {
    Float unk_00; /* the starts clear it, and nothing reads it */
    Float spin; /* O BROOBJ's child's turn about y, in degrees */
};

struct piece_vars {
    Uint8 unk_00[4];
    NJS_OBJECT *part; /* the part the piece draws */
    /* added to the alpha of the piece's material: 0, then less as it fades */
    Float fade;
};

struct piece_motion {
    Uint8 unk_00[4];
    NJS_VECTOR speed;
    Uint8 unk_10[24];
    Angle spin_x; /* turn about x each step */
    Angle spin_y; /* turn about y each step */
};

#define VARS(work) ((struct breakable_vars *)&(work)->vars)
#define PIECE(work) ((struct piece_vars *)&(work)->vars)
#define PIECE_MOTION(task) ((struct piece_motion *)(task)->motion)

enum breakable_mode { BREAKABLE_STAND, BREAKABLE_BROKEN };

enum pieces_mode { PIECES_START = 0, PIECES_RUN = 2, PIECES_DONE = 3 };

/* a break plays SE_BREAK, and makes a puff PUFF_HEIGHT above the object at
 * PUFF_SCALE */
#define SE_BREAK 64
#define PUFF_HEIGHT 10.0F
#define PUFF_SCALE 0.4F

/* both objects draw only while a sphere of this radius at their position is
 * in view */
#define VIEW_RADIUS 10.0F

/* the pieces run until their timer passes PIECE_STEPS. each piece falls
 * PIECE_GRAVITY faster each step. once it has flown more than
 * PIECE_FADE_AFTER steps, it fades by PIECE_FADE a step */
#define PIECE_STEPS 300
#define PIECE_GRAVITY 0.1F
#define PIECE_FADE 0.02F
#define PIECE_FADE_AFTER 20

/* a cube breaks for any character but E-102 only while player 0 moves faster
 * than this */
#define CUBE_BREAK_SPEED 1.2F

/* cube sizes, from scl.x */
enum cube_size { CUBE_LARGE, CUBE_MEDIUM, CUBE_SMALL };

/* the parts the pieces are made of. O BROOBJ whole and its nine pieces come
 * first. the large, medium and small cubes follow, the small one twice */
static NJS_OBJECT *windy_breakable_parts[14] = {
    &windy_breakable_object,         &windy_breakable_piece_1_object,
    &windy_breakable_piece_2_object, &windy_breakable_piece_3_object,
    &windy_breakable_piece_4_object, &windy_breakable_piece_5_object,
    &windy_breakable_piece_6_object, &windy_breakable_piece_7_object,
    &windy_breakable_piece_8_object, &windy_breakable_piece_9_object,
    &windy_large_cube_object,        &windy_medium_cube_object,
    &windy_small_cube_object,        &windy_small_cube_object,
};

/* each break's span of windy_breakable_parts, from its first part. every part
 * of a span but the first makes a piece. so O BROOBJ breaks into pieces 1 to
 * 8. O BROCU, whatever its size, breaks into the large and medium cubes */
#define BROOBJ_FIRST 0
#define BROOBJ_SPAN 9
#define CUBE_FIRST 9
#define CUBE_SPAN 3

/* a sphere no code reads */
static struct collision_shape dat_0ca13c20 = {
    SHAPE_SPHERE | SHAPE_HURTABLE | LOCKON_NO_NUMBER,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 15.0F, 0.0F},
    5.0F,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};

/* O BROOBJ's sphere, which attacks can hurt */
static struct collision_shape windy_breakable_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE | LOCKON_NO_NUMBER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    12.0F,
    10.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};

/* O BROCU's spheres, which attacks can hurt. they are the large cube's, the
 * medium's and the small's, then a copy of the large's. a small cube takes the
 * last two. for E-102, every cube takes the large's, as COLLISION_OBJECT
 * rather than COLLISION_ITEM */
static struct collision_shape windy_breakable_cube_shapes[4] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1),
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 10.0F, 0.0F},
     9.0F,
     10.0F,
     4.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1),
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 5.0F, 0.0F},
     9.0F,
     5.0F,
     5.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1),
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 5.0F, 0.0F},
     4.0F,
     4.0F,
     4.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     ATTACK(1),
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 10.0F, 0.0F},
     9.0F,
     10.0F,
     4.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* the ids a cube's scl.z may hold, which no code checks */
#define CUBE_IDS 30

/* TRUE for each cube id once a cube of that id has broken. the other cubes of
 * the id then break too, while player 0 moves fast enough. a cube clears its
 * id's flag as it starts */
static Bool windy_breakable_cube_broken[CUBE_IDS];

static void windy_breakable_cube_destroy_e102(struct task *task);
static void windy_breakable_run(struct task *task);
static void windy_breakable_draw(struct task *task);
static void windy_breakable_destroy(struct task *task);
static void windy_breakable_cube_destroy(struct task *task);
static void windy_breakable_pieces_step(struct task *task);
static void windy_breakable_pieces_destroy(struct task *task);
static void windy_breakable_piece_start(struct task *task);
static void windy_breakable_piece_step(struct task *task);
static void windy_breakable_piece_draw(struct task *task);
static void windy_breakable_piece_destroy(struct task *task);
static void windy_breakable_cube_run(struct task *task);
static void windy_breakable_cube_draw(struct task *task);

void windy_breakable_step(struct task *task)
{
    struct task_work *work = task->work;

    enemy_alloc_record(task, work);
    work->mode = BREAKABLE_STAND;
    work->timer = 0;
    VARS(work)->unk_00 = 0.0F;
    if (!placement_editing(task)) {
        collision_init(task, &windy_breakable_shape, COLLISION_ITEM, 1, 1);
    }
    task->step = windy_breakable_run;
    task->draw = windy_breakable_draw;
    task->destructor = windy_breakable_destroy;
}

void windy_breakable_cube_step(struct task *task)
{
    struct task_work *work = task->work;
    int id = (int)work->scl.z;

    if (get_character() != CHARACTER_E102) {
        enemy_alloc_record(task, work);
    }
    work->mode = BREAKABLE_STAND;
    work->timer = 0;
    VARS(work)->unk_00 = 0.0F;
    if (!placement_editing(task)) {
        if (get_character() != CHARACTER_E102) {
            windy_breakable_cube_broken[id] = FALSE;
            switch ((int)work->scl.x) {
            case CUBE_LARGE:
                collision_init(task, &windy_breakable_cube_shapes[0],
                               COLLISION_ITEM, 1, 1);
                break;
            case CUBE_MEDIUM:
                collision_init(task, &windy_breakable_cube_shapes[1],
                               COLLISION_ITEM, 1, 1);
                break;
            case CUBE_SMALL:
                collision_init(task, &windy_breakable_cube_shapes[2],
                               COLLISION_ITEM, 1, 2);
                break;
            default:
                collision_init(task, &windy_breakable_cube_shapes[0],
                               COLLISION_ITEM, 1, 1);
                break;
            }
        } else {
            collision_init(task, &windy_breakable_cube_shapes[0],
                           COLLISION_OBJECT, 1, 1);
        }
    }
    task->step = windy_breakable_cube_run;
    task->draw = windy_breakable_cube_draw;
    if (get_character() != CHARACTER_E102) {
        task->destructor = windy_breakable_cube_destroy;
    } else {
        task->destructor = windy_breakable_cube_destroy_e102;
    }
}

static void windy_breakable_cube_destroy_e102(struct task *task)
{
}

/* turn the child, and break once a hit damages the object. when the
 * character is Sonic, the break throws player 0 up */
static void windy_breakable_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    struct task *pieces;
    struct task_work *pieces_work;

    if (placement_try_leave(task)) {
        return;
    }
    VARS(work)->spin += work->scl.x;
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- ROTATE_SPEED");
        windy_breakable_draw(task);
    } else {
        switch (work->mode) {
        case BREAKABLE_STAND:
            if (enemy_take_hit(work, LOCKON_TARGET(task))) {
                sound_play(SE_BREAK, 0, 0, 0);
                pos.x = work->pos.x;
                pos.y = work->pos.y + PUFF_HEIGHT;
                pos.z = work->pos.z;
                bomb_create(&pos, PUFF_SCALE);
                work->mode = BREAKABLE_BROKEN;
                pieces = task_create(TASK_WORK | TASK_MOTION, 3,
                                     windy_breakable_pieces_step);
                pieces_work = pieces->work;
                pieces_work->tick = BROOBJ_SPAN;
                pieces_work->submode = BROOBJ_FIRST;
                pieces_work->mode = PIECES_START;
                pieces_work->ang.y = work->ang.y;
                pieces_work->pos.x = work->pos.x;
                pieces_work->pos.y = work->pos.y;
                pieces_work->pos.z = work->pos.z;
                if (get_character() == CHARACTER_SONIC) {
                    player_set_speed(0, 0.0F, 2.5F, 0.0F);
                }
            } else {
                collision_register(work);
            }
            windy_breakable_draw(task);
            break;
        case BREAKABLE_BROKEN:
            placement_kill(task);
            break;
        }
    }
}

/* draw O BROOBJ with its child turned by the spin about y */
static void windy_breakable_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *child;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, VIEW_RADIUS)) {
        child = windy_breakable_parts[0]->child;
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        draw_model(windy_breakable_parts[0]->model);
        njPushMatrix(NULL);
        njTranslate(NULL, child->pos[0], child->pos[1], child->pos[2]);
        TURN(njRotateY, (Angle)(VARS(work)->spin * ANGLE_PER_DEGREE));
        draw_model(child->model);
        njPopMatrix(1);
        njPopMatrix(1);
    }
}

static void windy_breakable_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

static void windy_breakable_cube_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    enemy_end(task, work, record);
}

/* the pieces of one break. its creator sets work->submode to the first part
 * of the break's span and work->tick to the span's length. the first step
 * makes a child piece of each part of the span but the first. each piece
 * starts where its part sits in the whole */
static void windy_breakable_pieces_step(struct task *task)
{
    struct task_work *work = task->work;
    int i;
    int count;
    Float x;
    Float y;
    Float z;
    Angle ang;
    struct task *child;
    struct task_work *piece;
    NJS_OBJECT *part;

    switch (work->mode) {
    case PIECES_START:
        count = work->tick;
        x = work->pos.x;
        y = work->pos.y;
        z = work->pos.z;
        ang = work->ang.y;
        for (i = 1; i < count; i++) {
            child = task_create_child(TASK_WORK | TASK_MOTION,
                                      windy_breakable_piece_start, task);
            piece = child->work;
            part = windy_breakable_parts[work->submode + i];
            PIECE(piece)->part = part;
            piece->ang.y = ang;
            piece->pos.x = x + part->pos[0];
            piece->pos.y = y + part->pos[1];
            piece->pos.z = z + part->pos[2];
        }
        work->timer = 0;
        task->destructor = windy_breakable_pieces_destroy;
        work->mode = PIECES_RUN;
        break;
    case PIECES_RUN:
        TASK_TIMER(work)++;
        task_step_children(task);
        if (TASK_TIMER(work) > PIECE_STEPS) {
            task_kill(task);
            work->mode = PIECES_DONE;
        }
        break;
    case PIECES_DONE:
        break;
    }
}

static void windy_breakable_pieces_destroy(struct task *task)
{
}

/* send the piece up at 2 to 3.5 a step. its speed along x and along z is up
 * to 0.8 a step either way. it turns about x and y at random speeds */
static void windy_breakable_piece_start(struct task *task)
{
    struct task_work *work = task->work;
    struct piece_motion *motion = PIECE_MOTION(task);

    work->timer = 0;
    PIECE(work)->fade = 0.0F;
    motion->speed.x = RANDOM(-0.5F, 0.5F) * 1.6F;
    motion->speed.y = RANDOM(2.0F, 3.5F);
    motion->speed.z = RANDOM(-0.5F, 0.5F) * 1.6F;
    motion->spin_y = RANDOM(-0.5F, 0.5F) * 2000.0F;
    motion->spin_x = RANDOM(-0.5F, 0.5F) * 3500.0F;
    task->step = windy_breakable_piece_step;
    task->draw = windy_breakable_piece_draw;
    task->destructor = windy_breakable_piece_destroy;
}

/* move and turn the piece under PIECE_GRAVITY, and fade it once it has flown
 * more than PIECE_FADE_AFTER steps */
static void windy_breakable_piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct piece_motion *motion = PIECE_MOTION(task);

    work->pos.x += motion->speed.x;
    work->pos.y += motion->speed.y;
    work->pos.z += motion->speed.z;
    motion->speed.y -= PIECE_GRAVITY;
    work->ang.y += motion->spin_y;
    work->ang.x += motion->spin_x;
    if (TASK_TIMER(work) > PIECE_FADE_AFTER) {
        PIECE(work)->fade -= PIECE_FADE;
    } else {
        TASK_TIMER(work)++;
    }
    windy_breakable_piece_draw(task);
}

static void windy_breakable_piece_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    draw_set_material(PIECE(work)->fade, 0.0F, 0.0F, 0.0F);
    draw_model(PIECE(work)->part->model);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    draw_attr_restore();
    njPopMatrix(1);
}

static void windy_breakable_piece_destroy(struct task *task)
{
}

/* break the cube. for any character but E-102, the cube breaks on a hit or
 * once its id's flag is set. it breaks only while player 0 moves faster than
 * CUBE_BREAK_SPEED. only a break the flag sets off plays SE_BREAK. while
 * player 0 moves that fast and the cube stands, the step registers its record
 * twice */
static void windy_breakable_cube_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task *pieces;
    /* @hack spelling: collision_register through a pointer, which SHC holds in
     * R10, saved at 0x0c90b5e8; without it, 240 of 244 words differ */
    void (*enter)(struct task_work *work);
    struct task_work *pieces_work;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- PARTS");
        if (get_character() == CHARACTER_E102) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- DYNAMAITE ID");
        }
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<- BROKEN ID");
        /* @hack redundant: a never-run call of njPrintC, which keeps the
         * image's register allocation: 1 in R11 and the converted index in R12
         * (0x0c90b64c, 0x0c90b6d8), which otherwise trade; without it, 8 of 244
         * words differ */
        if (0) {
            njPrintC(0, NULL);
        }
        windy_breakable_cube_draw(task);
    } else {
        switch (work->mode) {
        case BREAKABLE_STAND:
            enter = collision_register;
            if (get_character() == CHARACTER_E102) {
                if (target_cleared((int)work->scl.y)) {
                    NJS_POINT3 pos;

                    sound_play(SE_BREAK, 0, 0, 0);
                    pos.x = work->pos.x;
                    pos.y = work->pos.y + PUFF_HEIGHT;
                    pos.z = work->pos.z;
                    bomb_create(&pos, PUFF_SCALE);
                    work->mode = BREAKABLE_BROKEN;
                    pieces = task_create(TASK_WORK | TASK_MOTION, 3,
                                         windy_breakable_pieces_step);
                    pieces_work = pieces->work;
                    pieces_work->tick = CUBE_SPAN;
                    pieces_work->submode = CUBE_FIRST;
                    pieces_work->mode = PIECES_START;
                    pieces_work->ang.y = work->ang.y;
                    pieces_work->pos.x = work->pos.x;
                    pieces_work->pos.y = work->pos.y;
                    pieces_work->pos.z = work->pos.z;
                }
            } else {
                int id = (int)work->scl.z;
                struct player *player = player_get(0);
                NJS_VECTOR speed;
                Float pace;

                speed.x = player->speed.x;
                speed.y = player->speed.y;
                speed.z = player->speed.z;
                pace = njScalor(&speed);
                if (pace > CUBE_BREAK_SPEED) {
                    if (enemy_take_hit(work, LOCKON_TARGET(task)) ||
                        windy_breakable_cube_broken[id] == TRUE) {
                        NJS_POINT3 pos;

                        if (windy_breakable_cube_broken[id] == TRUE) {
                            sound_play(SE_BREAK, 0, 0, 0);
                        }
                        windy_breakable_cube_broken[id] = TRUE;
                        pos.x = work->pos.x;
                        pos.y = work->pos.y + PUFF_HEIGHT;
                        pos.z = work->pos.z;
                        bomb_create(&pos, PUFF_SCALE);
                        work->mode = BREAKABLE_BROKEN;
                        pieces = task_create(TASK_WORK | TASK_MOTION, 3,
                                             windy_breakable_pieces_step);
                        pieces_work = pieces->work;
                        pieces_work->tick = CUBE_SPAN;
                        pieces_work->submode = CUBE_FIRST;
                        pieces_work->mode = PIECES_START;
                        pieces_work->ang.y = work->ang.y;
                        pieces_work->pos.x = work->pos.x;
                        pieces_work->pos.y = work->pos.y;
                        pieces_work->pos.z = work->pos.z;
                    } else {
                        enter(work);
                    }
                }
            }
            enter(work);
            windy_breakable_cube_draw(task);
            break;
        case BREAKABLE_BROKEN:
            placement_kill(task);
            break;
        }
    }
}

/* draw the cube scl.x picks. the draw pops one matrix more than it pushes */
static void windy_breakable_cube_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, VIEW_RADIUS)) {
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        /* @hack spelling: each model through its object's address, which keeps
         * the objects in the pool words, as at 0x0c90b8c8, and loads the model
         * once after the switch, at 0x0c90b8d8; without it, 25 of 89 words
         * differ */
        switch ((int)work->scl.x) {
        case CUBE_LARGE:
            draw_model((&windy_large_cube_object)->model);
            break;
        case CUBE_MEDIUM:
            draw_model((&windy_medium_cube_object)->model);
            break;
        case CUBE_SMALL:
            draw_model((&windy_small_cube_object)->model);
            break;
        }
        njPopMatrix(1);
        njPopMatrix(1);
    }
}
