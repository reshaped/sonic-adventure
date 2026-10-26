/* O SAND ENTRY wall of sand in the jungle that pours away once the sand switch
 * is pressed */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv02/sand_entry.h"
#include "adv02/sand_switch.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "effect/dust.h"
#include "player/player.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT sand_entry_object;
extern NJS_MOTION sand_entry_motion;
extern NJS_MOTION sand_entry_shape_motion;

/* the entry's position and its angle about z. the first step sets both,
 * overriding the set entry's */
#define ENTRY_X (-1583.5F)
#define ENTRY_Y 59.61F
#define ENTRY_Z (-65.08F)
#define ENTRY_ANG_Z (-12933)

/* the pour plays sand_entry_motion to LAST_FRAME, POUR_SPEED motion frames a
 * step */
#define LAST_FRAME 58.99F
#define POUR_SPEED 0.1F

/* the step draws the entry only while player 0 is within DRAW_RANGE. under the
 * pause menu, task_draw_all draws it at any distance */
#define DRAW_RANGE 500.0F

enum sand_entry_mode { ENTRY_INIT, ENTRY_STANDING, ENTRY_POURING, ENTRY_GONE };

/* work->submode is TRUE once the sand is gone. the step registers the entry's
 * collision only while it is FALSE */
#define SAND_GONE(work) ((work)->submode)

struct sand_entry_vars {
    Uint8 unk_00[8];
    Float frame;
};

#define VARS(work) ((struct sand_entry_vars *)&(work)->vars)

static void sand_entry_destroy(struct task *task);
static void sand_entry_draw(struct task *task);

/* the wall's box is centered in world coordinates, and the entry's angle about
 * z doesn't turn it */
static struct collision_shape sand_entry_shape[1] = {
    {SHAPE_RECTANGLE | SHAPE_IN_WORLD | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS),
     {-1581.0F, 67.0F, -66.0F},
     22.0F,
     30.0F,
     50.0F,
     {0},
     0,
     NULL},
};

/* the params dust_create gives a puff, except for lift. lift is -0.01 here
 * in place of 0.025, so these puffs sink. the step sets pos and scale before
 * each puff */
static struct dust_params sand_entry_dust = {
    1.0F,
    0.038F,
    0.19F,
    0.06F,
    -0.01F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

/* the position, angles and scale njGetDrawMotion writes for the marker's one
 * node, from sand_entry_motion at the pour's frame */
static Float sand_entry_front_pos[3];
static Angle sand_entry_front_ang[3];
static Float sand_entry_front_scl[3];
static NJS_MOTION_DATA sand_entry_front = {
    sand_entry_front_pos,
    sand_entry_front_ang,
    sand_entry_front_scl,
};

void sand_entry_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    switch (work->mode) {
    case ENTRY_INIT:
        work->ang.z = ENTRY_ANG_Z;
        work->pos.x = ENTRY_X;
        work->pos.y = ENTRY_Y;
        work->pos.z = ENTRY_Z;
        collision_init(task, sand_entry_shape, COLLISION_OBJECT, 1, 1);
        task->draw = sand_entry_draw;
        task->destructor = sand_entry_destroy;
        if (sand_switch_flag()) {
            VARS(work)->frame = LAST_FRAME;
            SAND_GONE(work) = TRUE;
            work->mode = ENTRY_GONE;
        } else {
            VARS(work)->frame = 0.0F;
            work->mode = ENTRY_STANDING;
        }
        break;
    case ENTRY_STANDING:
        if (sand_switch_flag()) {
            work->scl = work->pos;
            work->mode = ENTRY_POURING;
        }
        break;
    case ENTRY_POURING: {
        struct dust_params *dust;

        VARS(work)->frame += POUR_SPEED;
        if (VARS(work)->frame > LAST_FRAME) {
            VARS(work)->frame = LAST_FRAME;
            work->collision->shapes->flags |= SHAPE_OFF;
            SAND_GONE(work) = TRUE;
            work->mode = ENTRY_GONE;
        }
        /* @bug logic: the puffs start within 75 units of the world's origin,
         * not at the sand, which stands 1586 units from it. njGetDrawMotion
         * writes the motion's own values, which the matrix the step sets
         * around the call doesn't move. the motion's x runs from 0 to 3.6 and
         * its y from 0 to -49.5, and its z stays 0 (keys at 0x0cb10b34) */
        njPushMatrix(NULL);
        njUnitMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        njGetDrawMotion(&marker_object, &sand_entry_motion, &sand_entry_front,
                        VARS(work)->frame);
        njPopMatrix(1);
        dust = &sand_entry_dust;
        dust->pos.x = sand_entry_front.pos[0];
        dust->pos.y =
            sand_entry_front.pos[1] + (((rand() * RAND_SCALE) - 0.5F) * 40.0F);
        dust->pos.z =
            sand_entry_front.pos[2] + (((rand() * RAND_SCALE) - 0.5F) * 50.0F);
        dust->scale = (rand() * (1.0F / 65536.0F)) + 0.5F;
        dust_create_from(dust);
        break;
    }
    case ENTRY_GONE:
        break;
    }
    if (player_in_range(&work->pos, DRAW_RANGE) == 1) {
        sand_entry_draw(task);
    }
    /* @hack spelling: the test written == FALSE, under which SHC loads
     * collision_register's address into R2 for the JMP @R2 at 0x0c90f370. as
     * !, it takes R3, and 2 of 218 words differ */
    if (SAND_GONE(work) == FALSE) {
        collision_register(work);
    }
}

static void sand_entry_destroy(struct task *task)
{
}

static void sand_entry_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    draw_shape_motion(&sand_entry_object, &sand_entry_motion,
                      &sand_entry_shape_motion, VARS(work)->frame);
    njPopMatrix(1);
}
