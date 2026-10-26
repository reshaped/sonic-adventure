/* O SIGNB poster board with three lamps that Knuckles breaks */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg04/billboard.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT billboard_board_object;
extern NJS_OBJECT billboard_cracked_object;
extern NJS_OBJECT billboard_lamp_object;
/* the board's textures with each of its five posters */
extern NJS_TEXNAME billboard_post01_texnames[3];
extern NJS_TEXNAME billboard_post07_texnames[3];
extern NJS_TEXNAME billboard_poster2_texnames[3];
extern NJS_TEXNAME billboard_poster3_texnames[3];
extern NJS_TEXNAME billboard_poster4_texnames[3];

/* the billboard stands BOARD_RISE above its set entry's position and
 * BOARD_BACK along z, which its heading doesn't turn */
#define BOARD_RISE 15.0F
#define BOARD_BACK (-1.5F)

/* a lamp's range for placement_try_leave_by, squared */
#define LAMP_RANGE (500.0F * 500.0F)

/* sound effects of a punch that cracks the board, and of the last punch, which
 * breaks it */
#define SE_CRACK 113
#define SE_BREAK 114

/* in the billboard's work->flags. BILLBOARD_SHAKEN is set while a punch shakes
 * the pieces. the first punch sets BILLBOARD_CRACKED, the second
 * BILLBOARD_CRACKED_TWICE, and the third BILLBOARD_BROKEN */
#define BILLBOARD_SHAKEN 0x100
#define BILLBOARD_CRACKED 0x200
#define BILLBOARD_CRACKED_TWICE 0x400
#define BILLBOARD_BROKEN 0x800

enum billboard_mode {
    BILLBOARD_INIT = 0,
    BILLBOARD_STAND = 1,
    BILLBOARD_SHAKE = 2,
    BILLBOARD_WRECKED = 5,
    BILLBOARD_GONE = 6 /* this file never sets it */
};

enum board_mode {
    BOARD_INIT = 0,
    BOARD_WHOLE = 1,
    BOARD_CRACKED = 3,
    BOARD_FLY = 5
};
enum lamp_mode { LAMP_INIT = 0, LAMP_HANG = 1, LAMP_LOOSE = 3, LAMP_FALL = 4 };

/* the billboard's work->scl holds the offset by which a punch shakes the
 * pieces away from player 0. a shake lasts eight steps */
#define SHAKE(work) ((work)->scl)
/* a falling piece's work->scl holds its velocity, which gains FALL_ACCEL
 * downward each step */
#define VELOCITY(work) ((work)->scl)
#define FALL_ACCEL 0.1F
/* a broken board's speed across x and z is BOARD_THROW times its offset from
 * player 0. its upward speed is BOARD_LIFT times a random number from
 * BOARD_LIFT_LEAST up to BOARD_LIFT_LEAST + 1 */
#define BOARD_THROW 0.25F
#define BOARD_LIFT 2.0F
#define BOARD_LIFT_LEAST 0.4F

static void billboard_piece_draw(struct task *task);
static void billboard_piece_destroy(struct task *task);
static void billboard_board_step(struct task *task);
static void billboard_lamp_step(struct task *task);
static void billboard_choose_texlist(struct task *task);
static void billboard_destroy(struct task *task);

/* the texture lists of the five posters */
static NJS_TEXLIST billboard_texlists[5] = {
#include "stg04/billboard/billboard_texlists.inc"
};

/* @hack type: the shape an array of one. billboard_step then saves R12 to R14
 * alone (0x0c9097c6 to 0x0c9097ca); as a single struct, SHC keeps a 1 in R11,
 * which it saves too, and 258 of 298 words differ */
static struct collision_shape billboard_shape[1] = {
    {SHAPE_RECTANGLE | SHAPE_HURTABLE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, -1.5F},
     25.0F,
     15.0F,
     1.5F,
     {0},
     0,
     NULL},
};

/* the three lamps along the board's top, 20 apart. each spec's var is the
 * lamp's object, which MODEL(work)->object holds */
static struct task_spec billboard_lamps[4] = {
    {billboard_lamp_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {-20.0F, 16.5F, 5.0F},
     (Sint32)&billboard_lamp_object},
    {billboard_lamp_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {0.0F, 16.5F, 5.0F},
     (Sint32)&billboard_lamp_object},
    {billboard_lamp_step,
     TASK_WORK,
     0,
     {0, 0, 0},
     {20.0F, 16.5F, 5.0F},
     (Sint32)&billboard_lamp_object},
    {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0},
};

/* draw a piece with its texlist, or the stage's when it has none. while player
 * 0 is Knuckles, the piece brightens with player 0 or 1 within 20 of it */
static void billboard_piece_draw(struct task *task)
{
    struct task_work *work = task->work;

    draw_highlight_near(work, 1 << CHARACTER_KNUCKLES);
    if (MODEL(work)->texlist == NULL) {
        texture_set_stage();
    } else {
        njSetTexture(MODEL(work)->texlist);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_model(MODEL(work)->object->model);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* take the task's object out of the collision world and return it to the
 * pool. for a piece, both calls change nothing. no piece puts its object in
 * the world, and the object is a model, not the pool's. the billboard calls
 * this too, with a null object */
static void billboard_piece_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        if (MODEL(work)->object != NULL) {
            world_remove(task, MODEL(work)->object);
            world_object_free(MODEL(work)->object);
        }
    }
}

/* the board's step. the first punch cracks the board, and the third throws it
 * away from player 0, tumbling */
static void billboard_board_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *billboard = task->parent->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BOARD_INIT:
        work->mode = BOARD_WHOLE;
        MODEL(work)->object = &billboard_board_object;
        MODEL(work)->texlist = MODEL(billboard)->texlist;
        task->destructor = billboard_piece_destroy;
        task->draw = billboard_piece_draw;
        break;
    case BOARD_WHOLE:
        if (!edit_mode) {
            if (billboard->flags & BILLBOARD_CRACKED) {
                work->mode = BOARD_CRACKED;
                MODEL(work)->object = &billboard_cracked_object;
                billboard_piece_draw(task);
            } else if (billboard->flags & BILLBOARD_SHAKEN) {
                work->pos.x += SHAKE(billboard).x;
                work->pos.z += SHAKE(billboard).z;
                billboard_piece_draw(task);
                work->pos.x -= SHAKE(billboard).x;
                work->pos.z -= SHAKE(billboard).z;
            } else {
                billboard_piece_draw(task);
            }
        } else {
            billboard_piece_draw(task);
        }
        break;
    case BOARD_CRACKED:
        if (billboard->flags & BILLBOARD_BROKEN) {
            work->mode = BOARD_FLY;
            VELOCITY(work).x =
                (work->pos.x - player_work[0]->pos.x) * BOARD_THROW;
            VELOCITY(work).z =
                (work->pos.z - player_work[0]->pos.z) * BOARD_THROW;
            VELOCITY(work).y =
                ((rand() * RAND_SCALE) + BOARD_LIFT_LEAST) * BOARD_LIFT;
            work->ang.x = (Angle)RANDOM_ANGLE() & 0xffff;
            work->ang.y = (Angle)RANDOM_ANGLE() & 0xffff;
            work->ang.z = (Angle)RANDOM_ANGLE() & 0xffff;
            billboard_piece_draw(task);
        } else if (billboard->flags & BILLBOARD_SHAKEN) {
            work->pos.x += SHAKE(billboard).x;
            work->pos.z += SHAKE(billboard).z;
            billboard_piece_draw(task);
            work->pos.x -= SHAKE(billboard).x;
            work->pos.z -= SHAKE(billboard).z;
        } else {
            billboard_piece_draw(task);
        }
        break;
    case BOARD_FLY:
        VELOCITY(work).y -= FALL_ACCEL;
        njAddVector(&work->pos, &VELOCITY(work));
        work->ang.x += 0x1000;
        work->ang.z += 0x250;
        billboard_piece_draw(task);
        break;
    }
}

/* a lamp's step. a lamp shakes with the board until the first punch's shake
 * ends, and then hangs loose. the second punch drops it, tumbling. it falls
 * straight down. the x and z of its velocity, set as it starts to fall, go
 * unread */
static void billboard_lamp_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *billboard = task->parent->work;

    if (placement_try_leave_by(task, LAMP_RANGE)) {
        return;
    }
    switch (work->mode) {
    case LAMP_INIT:
        work->mode = LAMP_HANG;
        MODEL(work)->texlist = NULL;
        task->destructor = billboard_piece_destroy;
        task->draw = billboard_piece_draw;
        work->ang.x = work->ang.z = 0;
        break;
    case LAMP_HANG:
        if (!edit_mode) {
            if (billboard->flags & BILLBOARD_SHAKEN) {
                work->pos.x += SHAKE(billboard).x;
                work->pos.z += SHAKE(billboard).z;
                billboard_piece_draw(task);
                work->pos.x -= SHAKE(billboard).x;
                work->pos.z -= SHAKE(billboard).z;
            } else {
                if (billboard->flags & BILLBOARD_CRACKED) {
                    work->mode = LAMP_LOOSE;
                }
                billboard_piece_draw(task);
            }
        } else {
            billboard_piece_draw(task);
        }
        break;
    case LAMP_LOOSE:
        if (billboard->flags & BILLBOARD_CRACKED_TWICE) {
            work->mode = LAMP_FALL;
            work->submode = 0;
            VELOCITY(work).x = (work->pos.x - player_work[0]->pos.x) * 0.125F;
            VELOCITY(work).z = (work->pos.z - player_work[0]->pos.z) * 0.125F;
            VELOCITY(work).y = rand() * RAND_SCALE;
            work->ang.x = (Angle)RANDOM_ANGLE() & 0xffff;
            work->ang.y = (Angle)RANDOM_ANGLE() & 0xffff;
            work->ang.z = (Angle)RANDOM_ANGLE() & 0xffff;
        }
        billboard_piece_draw(task);
        return;
    case LAMP_FALL:
        VELOCITY(work).y -= FALL_ACCEL;
        work->pos.y += VELOCITY(work).y;
        work->ang.x += 0x500;
        work->ang.y += 0x500;
        work->ang.z += 0x280;
        billboard_piece_draw(task);
        break;
    }
}

/* choose the poster by scale x: POST01 below 10, POST07 below 20, POSTER2 below
 * 30, POSTER3 below 40, and POSTER4 otherwise */
static void billboard_choose_texlist(struct task *task)
{
    struct task_work *work = task->work;
    NJS_TEXLIST *texlist = billboard_texlists;

    /* @hack spelling: the gotos to the one store, which keep each test in FR2
     * and FR3 (0x0c909762 to 0x0c9097a8); as an else-if chain, the later tests
     * take FR0 and FR1, and 8 of 35 words differ */
    if (work->scl.x < 10.0F) {
        goto chosen;
    }
    if (work->scl.x < 20.0F) {
        texlist += 1;
        goto chosen;
    }
    if (work->scl.x < 30.0F) {
        texlist += 2;
        goto chosen;
    }
    if (work->scl.x < 40.0F) {
        texlist += 3;
        goto chosen;
    }
    texlist += 4;
chosen:
    MODEL(work)->texlist = texlist;
}

static void billboard_destroy(struct task *task)
{
    task_destroy_children(task);
    billboard_piece_destroy(task);
}

/* while the set editor holds the billboard, print the poster choices and draw
 * the board. otherwise only Knuckles' punches move the board and the lamps */
void billboard_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    Float shake;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        pos.x = work->pos.x;
        pos.y = work->pos.y + BOARD_RISE;
        pos.z = work->pos.z + BOARD_BACK;
        njPrintC(NJM_LOCATION(12, 2), "        XSCL < 10.0: POST01");
        njPrintC(NJM_LOCATION(12, 3), "10.0 <= XSCL < 20.0: POST07");
        njPrintC(NJM_LOCATION(12, 4), "20.0 <= XSCL < 30.0: POSTER2");
        njPrintC(NJM_LOCATION(12, 5), "30.0 <= XSCL < 40.0: POSTER3");
        njPrintC(NJM_LOCATION(12, 6), "40.0 <= XSCL < 50.0: POSTER4");
        billboard_choose_texlist(task);
        njPushMatrix(NULL);
        njTranslateV(NULL, &pos);
        TURN(njRotateY, work->ang.y);
        njSetTexture(MODEL(work)->texlist);
        /* @hack spelling: the model reached through the object's address, which
         * loads that address from the pool and the model at @(4,R4)
         * (0x0c909862, 0x0c909866); as billboard_board_object.model, the
         * pool holds the field's own address, 1 of the pool's words differs
         * too, and 2 of 298 words differ */
        draw_model((&billboard_board_object)->model);
        njPopMatrix(1);
        return;
    }
    switch (work->mode) {
    case BILLBOARD_INIT:
        work->mode = BILLBOARD_STAND;
        work->pos.y += BOARD_RISE;
        work->pos.z += BOARD_BACK;
        work->ang.x = work->ang.z = 0;
        billboard_choose_texlist(task);
        collision_init(task, billboard_shape, COLLISION_OBJECT, 1, 1);
        task->destructor = billboard_destroy;
        task_create_child(TASK_WORK, billboard_board_step, task);
        task_create_children(billboard_lamps, task);
        break;
    case BILLBOARD_STAND:
        if (!edit_mode) {
            work->submode = 0;
            if (work->flags & (BILLBOARD_BROKEN | BILLBOARD_SHAKEN)) {
                work->flags &= ~BILLBOARD_SHAKEN;
            } else if (work->flags & COLLISION_DAMAGED) {
                if (PLAYER_VARS(work->collision->hit->owner->work)->character ==
                    CHARACTER_KNUCKLES) {
                    if (work->flags & BILLBOARD_CRACKED_TWICE) {
                        work->mode = BILLBOARD_WRECKED;
                        work->flags |= BILLBOARD_BROKEN;
                        sound_play(SE_BREAK, 0, 0, 0);
                    } else {
                        work->mode = BILLBOARD_SHAKE;
                        work->flags |= BILLBOARD_SHAKEN;
                        if (work->flags & BILLBOARD_CRACKED) {
                            work->flags |= BILLBOARD_CRACKED_TWICE;
                        } else {
                            work->flags |= BILLBOARD_CRACKED;
                        }
                        sound_play(SE_CRACK, 0, 0, 0);
                    }
                }
            }
            collision_register(work);
            task_step_children(task);
        } else {
            task_step_children(task);
        }
        break;
    case BILLBOARD_SHAKE:
        switch (work->submode++) {
        case 0:
        case 7:
            shake = 0.1F;
            break;
        case 1:
        case 2:
        case 5:
        case 6:
            shake = 0.2F;
            break;
        case 3:
        case 4:
            shake = 0.25F;
            break;
        case 8:
            work->mode = BILLBOARD_STAND;
            /* falls through: the step that ends the shake leaves it at 0 */
        default:
            shake = 0.0F;
            break;
        }
        SHAKE(work).x = shake * (work->pos.x - player_work[0]->pos.x);
        SHAKE(work).z = shake * (work->pos.z - player_work[0]->pos.z);
        task_step_children(task);
        collision_register(work);
        break;
    case BILLBOARD_WRECKED:
        task_step_children(task);
        break;
    case BILLBOARD_GONE:
    default:
        billboard_destroy(task);
        break;
    }
}
