/* the gardens' fruit trees, the fruit they drop, and FLOWER */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/alife.h"
#include "al_main/entry.h"
#include "al_main/models.h"
#include "al_main/tree.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "enemy/enemy.h"
#include "object/common.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

/* alife_camera_distance_squared, under a second name declared to return int.
 * alife_camera_distance_squared returns the camera's squared distance to pos in
 * FR0, and never writes R0. fruit_show reads the int from R0, so it gets
 * whatever R0 held at the call */
extern int alife_camera_distance_squared_int(NJS_POINT3 *pos);

/* a tree has two branches, each with at most one fruit. branch 0's fruit
 * grows in work->scl.x and branch 1's in work->scl.z. in the work's flags,
 * TREE_FRUIT_0 and TREE_FRUIT_1 mark the branches that have a fruit.
 * TREE_FELL_0 and TREE_FELL_1 mark a branch whose fruit a shake has just
 * dropped. the same step starts that fruit falling and clears the mark */
#define TREE_FRUIT_0 0x400
#define TREE_FRUIT_1 0x800
#define TREE_FELL_0 0x1000
#define TREE_FELL_1 0x2000

/* a fruit grows from 0 to 1 by GROWTH a step. it shows ripe from RIPE on,
 * and a shake can drop it once it passes RIPE */
#define GROWTH (1.0F / 12000.0F)
#define RIPE 0.98F
/* a bare branch waits a random number of steps below this to start a
 * fruit */
#define LONGEST_WAIT 12000.0F
/* no branch starts a fruit while this many fruit are in the garden */
#define FRUIT_LIMIT 10
/* the kind of fruit a tree drops */
#define FRUIT_OF_TREE 10
/* the sounds of a shake that swings the tree, and of a fruit that bounces
 * on the ground */
#define SE_SHAKE 0x259
#define SE_BOUNCE 0x25a
/* an entry kind that no alife_entry_add call gives. one that a player holds
 * against a tree ripens the tree's fruit faster */
#define ALIFE_RIPENER 30

struct tree_vars {
    Uint8 unk_00[4];
    /* the frame of garden_tree_action the tree shows, from 0 to 8. the swing
     * pulls it back toward 4, where the tree stands still */
    Float frame;
    Float swing; /* how far frame moves a step */
};

#define VARS(work) ((struct tree_vars *)&(work)->vars)

/* the tree's record, at its task's motion, which carry_set_kind takes as a
 * task_motion. shakes_0 and shakes_1 count the shakes each branch's ripe
 * fruit holds on through. wait_0 and wait_1 count the steps until each bare
 * branch starts a fruit */
struct tree_motion {
    Uint8 unk_00[0x10];
    /* the push of a player shaking the tree this step, at most 1 long.
     * carry_step_still clears it */
    NJS_VECTOR shake;
    Sint32 shakes_0;
    Sint32 shakes_1;
    Sint32 wait_0;
    Sint32 wait_1;
    Uint8 unk_2c[8];
    Float carry_reach; /* carry_set_kind's reach, at task_motion.drive */
    Uint8 unk_38[4];
    Float gravity;
};

#define TREE_MOTION(task) ((struct tree_motion *)(task)->motion)

extern NJS_MODEL dat_0ca7deb4;
extern NJS_MODEL dat_0ca7e3a8;
extern NJS_MODEL dat_0ca7e8d4;
extern NJS_MODEL dat_0ca7eda4;
extern NJS_MODEL dat_0ca7f274;
extern NJS_MODEL dat_0ca7f744;
extern NJS_MODEL dat_0ca7fc14;
extern NJS_MODEL dat_0ca801d0;

/* an object for kind 1's model, dat_0ca801d0, which no word in the image
 * points to. SHC 5.0 R32, which builds this file, starts each object's data
 * on 8. alife_fruit_models starts straight after this object, 4 bytes past
 * 8, so it opens no object of its own, and this object is this file's */
static NJS_OBJECT dat_0ca801f8 = {
#include "al_main/tree/dat_0ca801f8.inc"
};
/* a fruit's model, by its kind. kind 0 has none, and kinds 8 and 9 share
 * one */
NJS_MODEL *alife_fruit_models[11] = {
    NULL,          &dat_0ca801d0, &dat_0ca7eda4,    &dat_0ca7f744,
    &dat_0ca7f274, &dat_0ca7fc14, &dat_0ca7deb4,    &dat_0ca7e8d4,
    &dat_0ca7e3a8, &dat_0ca7e3a8, &tree_fruit_ripe,
};
Float alife_fruit_scales[11] = {
#include "al_main/tree/alife_fruit_scales.inc"
};
struct collision_shape garden_tree_shapes[2] = {
#include "al_main/tree/garden_tree_shapes.inc"
};

void garden_tree_init(struct task *task)
{
    struct task_work *work = task->work;
    struct tree_motion *motion = TREE_MOTION(task);

    if (!placement_editing(task)) {
        collision_init(task, garden_tree_shapes, COLLISION_OBJECT, 1, 2);
    }
    work->scl.x = 0.0F;
    work->scl.z = 0.0F;
    motion->wait_0 = (Uint32)(rand() * (LONGEST_WAIT * RAND_SCALE));
    motion->wait_1 = (Uint32)(rand() * (LONGEST_WAIT * RAND_SCALE));
    motion->shakes_0 = (Sint8)(rand() * (7.0F * RAND_SCALE)) + 3;
    motion->shakes_1 = (Sint8)(rand() * (7.0F * RAND_SCALE)) + 3;
    work->flags &= ~(TREE_FRUIT_0 | TREE_FRUIT_1 | TREE_FELL_0 | TREE_FELL_1);
    /* @hack spelling: shape 0's levels through garden_tree_shapes->levels,
     * which SHC reads and writes through R10, the array's address, at
     * 0x0c918c58 and 0x0c918c5e; as garden_tree_shapes[0].levels, 18 of 173
     * words differ */
    garden_tree_shapes->levels =
        (garden_tree_shapes->levels & ~SHAPE_GUARD) | SHAPE_GUARD;
    garden_tree_shapes[1].levels =
        (garden_tree_shapes[1].levels & ~SHAPE_GUARD) | SHAPE_GUARD;
    if (rand() * RAND_SCALE < 0.5F) {
        work->flags |= TREE_FRUIT_0;
        work->scl.x = rand() * RAND_SCALE;
    }
    if (rand() * RAND_SCALE < 0.5F) {
        work->flags |= TREE_FRUIT_1;
        work->scl.z = rand() * RAND_SCALE;
    }
    carry_set_kind(work, (struct task_motion *)motion, CARRY_KIND_SHAKE);
    alife_entry_add(task, ALIFE_PLACES, ALIFE_TREE, 10.0F);
    VARS(work)->swing = 0.0F;
    VARS(work)->frame = 4.0F;
    task->destructor = garden_tree_destroy;
    task->step = garden_tree_step;
    task->draw = garden_tree_draw;
}

/* how far the tree leans for each unit of its shake's push */
#define LEAN (6.0F * ANGLE_PER_DEGREE)

/* every 17th step that a shake pushes the tree more than 0.5, the tree
 * swings, and each ripe fruit counts down its shakes. a ripe fruit falls at
 * the shake after its last. a bare branch starts a fruit as its wait runs
 * out, unless FRUIT_LIMIT fruit are in the garden. the wait then runs on
 * below 0, and the branch stays bare for 2^32 steps */
void garden_tree_step(struct task *task)
{
    struct task_work *work = task->work;
    struct tree_motion *motion = TREE_MOTION(task);
    struct alife_entry *entry = task->extra;
    Float magnitude;
    Angle tilt;
    Angle yaw;
    Angle reverse_yaw;
    Angle angle;
    /* @hack spelling: growth_0 and growth_1 hold the addresses of work->scl.x
     * and work->scl.z, which SHC keeps in R12 and R13 (0x0c918dce, 0x0c918dd0);
     * through work, the code after the function starts 32 bytes earlier, and
     * 707 of 795 words differ */
    Float *growth_0;
    Float *growth_1;

    njSetTexture(&alife_objects_texlist);
    draw_highlight_near(work, ALL_CHARACTERS);
    magnitude = njScalor(&motion->shake);
    tilt = (Angle)(magnitude * LEAN);
    yaw = (Angle)(atan2f(motion->shake.x, motion->shake.z) * ANGLE_PER_RADIAN);
    growth_0 = &work->scl.x;
    growth_1 = &work->scl.z;
    if (magnitude > 0.5F && TASK_TIMER(work)++ > 15) {
        if (_builtin_fabsf(VARS(work)->swing) < 0.1F) {
            VARS(work)->swing = 0.2F;
        } else {
            VARS(work)->swing *= 3.0F;
            if (VARS(work)->swing > 2.0F) {
                VARS(work)->swing = 2.0F;
            } else if (VARS(work)->swing < -2.0F) {
                VARS(work)->swing = -2.0F;
            }
        }
        work->timer = 0;
        sound_play(SE_SHAKE, 0, 0, 0);
        if ((work->flags & TREE_FRUIT_0) && *growth_0 > RIPE &&
            motion->shakes_0-- == 0) {
            work->flags |= TREE_FELL_0;
            work->flags &= ~TREE_FRUIT_0;
            *growth_0 = 0.0F;
            motion->shakes_0 = (Sint8)(rand() * (5.0F * RAND_SCALE)) + 4;
        }
        if ((work->flags & TREE_FRUIT_1) && *growth_1 > RIPE &&
            motion->shakes_1-- == 0) {
            work->flags |= TREE_FELL_1;
            work->flags &= ~TREE_FRUIT_1;
            *growth_1 = 0.0F;
            motion->shakes_1 = (Sint8)(rand() * (6.0F * RAND_SCALE)) + 4;
        }
    }
    VARS(work)->swing -= (VARS(work)->frame - 4.0F) * 0.1F;
    VARS(work)->swing *= 0.975F;
    VARS(work)->frame += VARS(work)->swing;
    if (VARS(work)->frame < 0.0F) {
        VARS(work)->frame = 0.0F;
    } else if (VARS(work)->frame > 8.0F) {
        VARS(work)->frame = 8.0F;
    }

    if (entry->flags & ENTRY_TOUCHING) {
        Sint8 count = entry->hit_count;
        struct alife_entry **end = entry->hits + count;
        struct alife_entry **hit;
        for (hit = entry->hits; hit < end; hit++) {
            if ((*hit)->kind == ALIFE_RIPENER) {
                if ((*hit)->task->work->flags & CARRY_HELD) {
                    if (work->flags & TREE_FRUIT_0) {
                        *growth_0 += 0.001F;
                    }
                    if (work->flags & TREE_FRUIT_1) {
                        *growth_1 += 0.001F;
                    }
                    break;
                }
            }
        }
    }
    /* @hack spelling: reverse_yaw assigned in the test, which loads 1.5 into
     * FR12 at 0x0c91902c ahead of the negation and its store; assigned ahead
     * of the if, 3 of 795 words differ */
    if ((reverse_yaw = -yaw, work->flags & TREE_FRUIT_0)) {
        *growth_0 += GROWTH;
        if (*growth_0 > 1.0F) {
            *growth_0 = 1.0F;
        }
    } else {
        if (motion->wait_0-- == 0 && alife_fruit_count < FRUIT_LIMIT) {
            work->flags |= TREE_FRUIT_0;
            motion->wait_0 = (Uint32)(rand() * (LONGEST_WAIT * RAND_SCALE));
        }
        if (work->flags & TREE_FELL_0) {
            /* @hack spelling: origin declared ahead of position, which puts
             * origin at sp+48 and position at sp+36 for njCalcPoint
             * (0x0c919182, 0x0c919184); declared after it, 4 of 795 words
             * differ */
            NJS_POINT3 origin = {0.0F, 0.0F, 0.0F};
            NJS_POINT3 position;
            NJS_VECTOR velocity = {-0.5F, 0.1F, -0.5F};

            velocity.x += rand() * RAND_SCALE;
            velocity.z += rand() * RAND_SCALE;
            njPushMatrix(&_unit_matrix_);
            njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
            TURN(njRotateY, yaw);
            TURN(njRotateX, tilt);
            TURN(njRotateY, reverse_yaw);
            TURN_VIA(njRotateY, work->ang.y, angle);
            njTranslate(NULL, 0.0F, 28.32F, (*growth_0 * 1.5F) + 1.0F);
            njRotateX(NULL, -0x2000);
            njCalcPoint(NULL, &origin, &position);
            njPopMatrix(1);
            alife_fruit_create(&position, &velocity, FRUIT_OF_TREE);
            work->flags &= ~TREE_FELL_0;
        }
    }
    if (work->flags & TREE_FRUIT_1) {
        *growth_1 += GROWTH;
        if (*growth_1 > 1.0F) {
            *growth_1 = 1.0F;
        }
    } else {
        if (motion->wait_1-- == 0 && alife_fruit_count < FRUIT_LIMIT) {
            work->flags |= TREE_FRUIT_1;
            motion->wait_1 = (Uint32)(rand() * (LONGEST_WAIT * RAND_SCALE));
        }
        if (work->flags & TREE_FELL_1) {
            /* @hack spelling: origin declared ahead of position, which puts
             * origin at sp+48 and position at sp+36 for njCalcPoint
             * (0x0c91936a, 0x0c91936c); declared after it, 4 of 795 words
             * differ */
            NJS_POINT3 origin = {0.0F, 0.0F, 0.0F};
            NJS_POINT3 position;
            NJS_VECTOR velocity = {-0.5F, 0.1F, -0.5F};

            velocity.x += rand() * RAND_SCALE;
            velocity.z += rand() * RAND_SCALE;
            njPushMatrix(&_unit_matrix_);
            njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
            TURN(njRotateY, yaw);
            TURN(njRotateX, tilt);
            TURN(njRotateY, reverse_yaw);
            TURN_VIA(njRotateY, work->ang.y, angle);
            njRotateY(NULL, 0x5000);
            njTranslate(NULL, 0.0F, 27.3F, (*growth_1 * 1.5F) + 1.0F);
            njRotateX(NULL, -0x2000);
            njCalcPoint(NULL, &origin, &position);
            njPopMatrix(1);
            alife_fruit_create(&position, &velocity, FRUIT_OF_TREE);
            work->flags &= ~TREE_FELL_1;
        }
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, yaw);
    TURN(njRotateX, tilt);
    TURN(njRotateY, reverse_yaw);
    TURN_VIA(njRotateY, work->ang.y, angle);
    draw_action(&garden_tree_action, VARS(work)->frame);
    if (work->flags & TREE_FRUIT_0) {
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 28.32F, (*growth_0 * 1.5F) + 1.0F);
        njRotateX(NULL, -0x2000);
        njScale(NULL, *growth_0, *growth_0, *growth_0);
        if (*growth_0 < 0.5F) {
            draw_model(&tree_fruit_stage_0);
        } else if (*growth_0 < 0.8F) {
            draw_model(&tree_fruit_stage_1);
        } else if (*growth_0 < RIPE) {
            draw_model(&tree_fruit_stage_2);
        } else {
            draw_model(&tree_fruit_ripe);
        }
        njPopMatrix(1);
    }
    if (work->flags & TREE_FRUIT_1) {
        njPushMatrix(NULL);
        njRotateY(NULL, 0x5000);
        njTranslate(NULL, 0.0F, 27.3F, (*growth_1 * 1.5F) + 1.0F);
        njRotateX(NULL, -0x2000);
        njScale(NULL, *growth_1, *growth_1, *growth_1);
        if (*growth_1 < 0.5F) {
            draw_model(&tree_fruit_stage_0);
        } else if (*growth_1 < 0.8F) {
            draw_model(&tree_fruit_stage_1);
        } else if (*growth_1 < RIPE) {
            draw_model(&tree_fruit_stage_2);
        } else {
            draw_model(&tree_fruit_ripe);
        }
        njPopMatrix(1);
    }
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    carry_step_still(task);
}

void garden_tree_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct tree_motion *motion = TREE_MOTION(task);
    Angle tilt;
    Angle yaw;
    Angle angle;
    /* @hack spelling: growth_0 and growth_1 hold the addresses of work->scl.x
     * and work->scl.z, which SHC keeps in R13 and R11 (0x0c919662, 0x0c919670);
     * through work, the code after the function starts 32 bytes later, and 210
     * of 221 words differ */
    Float *growth_0;
    Float *growth_1;
    /* @hack spelling: scale, branch 0's growth for njScale, which keeps tilt in
     * R8 (STS FPUL,R8 at 0x0c919652); as *growth_0, tilt goes to the stack,
     * and the function runs 2 bytes long and 129 of 221 words differ */
    Float scale;
    /* @hack spelling: magnitude, the shake's length, which keeps tilt in R8 at
     * 0x0c919652; read in place, tilt goes to the stack, and the function
     * runs 2 bytes long and 99 of 221 words differ */
    Float magnitude;
    /* @hack spelling: frame, the tree's frame for draw_action, which keeps
     * tilt in R8 at 0x0c919652; read in place, tilt goes to the stack, and
     * the function runs 2 bytes long and 129 of 221 words differ */
    Float frame;

    magnitude = njScalor(&motion->shake);
    tilt = (Angle)(magnitude * LEAN);
    yaw = (Angle)(atan2f(motion->shake.x, motion->shake.z) * ANGLE_PER_RADIAN);
    growth_0 = &work->scl.x;
    growth_1 = &work->scl.z;
    njSetTexture(&alife_objects_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, yaw);
    /* @hack spelling: TURN written out as an if, which keeps njPushMatrix's
     * address in R9 and njTranslate's in R10 (0x0c919672, 0x0c91967a);
     * through TURN, the two trade and 8 of 221 words differ */
    if (tilt) {
        njRotateX(NULL, tilt);
    }
    TURN(njRotateY, -yaw);
    TURN_VIA(njRotateY, work->ang.y, angle);
    frame = VARS(work)->frame;
    draw_action(&garden_tree_action, frame);
    if (work->flags & TREE_FRUIT_0) {
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 28.32F, (*growth_0 * 1.5F) + 1.0F);
        njRotateX(NULL, -0x2000);
        scale = *growth_0;
        njScale(NULL, scale, scale, scale);
        if (*growth_0 < 0.5F) {
            draw_model(&tree_fruit_stage_0);
        } else if (*growth_0 < 0.8F) {
            draw_model(&tree_fruit_stage_1);
        } else if (*growth_0 < RIPE) {
            draw_model(&tree_fruit_stage_2);
        } else {
            draw_model(&tree_fruit_ripe);
        }
        njPopMatrix(1);
    }
    if (work->flags & TREE_FRUIT_1) {
        njPushMatrix(NULL);
        njRotateY(NULL, 0x5000);
        njTranslate(NULL, 0.0F, 27.3F, (*growth_1 * 1.5F) + 1.0F);
        njRotateX(NULL, -0x2000);
        njScale(NULL, *growth_1, *growth_1, *growth_1);
        /* @bug logic: past the first test, branch 1's fruit takes its
         * model by branch 0's growth, read through R13 at 0x0c919842 and
         * 0x0c919856, where garden_tree_step reads branch 1's own */
        if (*growth_1 < 0.5F) {
            draw_model(&tree_fruit_stage_0);
        } else if (*growth_0 < 0.8F) {
            draw_model(&tree_fruit_stage_1);
        } else if (*growth_0 < RIPE) {
            draw_model(&tree_fruit_stage_2);
        } else {
            draw_model(&tree_fruit_ripe);
        }
        njPopMatrix(1);
    }
    njPopMatrix(1);
}

void garden_tree_destroy(struct task *task)
{
    struct alife_entry *entry = task->extra;

    alife_entry_remove(entry);
}

/* the work's mode. alife_fruit_create starts a fruit in FRUIT_FALL, and
 * nothing in this file sets FRUIT_REST */
enum fruit_mode { FRUIT_REST = 1, FRUIT_FALL, FRUIT_HELD, FRUIT_EATEN };

/* the fruit's kind, in its work's tick */
#define FRUIT_KIND(work) ((work)->tick)

/* an angle set as a holder takes the fruit. while the holder keeps it, each
 * step sets the fruit's yaw to the holder's yaw plus this one, with player
 * 0's yaw negated */
#define HELD_TURN(record) ((record)->motion.aim.y)

/* the downward speed a fruit gains each step */
#define GRAVITY 0.05F
/* the share of its speed along x and z that a fruit keeps each step on the
 * ground */
#define GROUND_KEEP 0.7F

/* fruit_shapes[2] hurts enemies. fruit_fall turns it on while the fruit
 * moves faster than this, and off otherwise */
#define HIT_SPEED 0.5F

/* clang-format off */
static const struct collision_shape fruit_shapes[3] = {
    {SHAPE_CYLINDER | SHAPE_NO_PUSH | SHAPE_IGNORE_ENEMIES |
         SHAPE_IGNORE_OBJECTS,
     SHAPE_GUARD, PUSH_FIRM(PUSH_ALL), {0.0F, 1.5F, 0.0F}, 10.2F, 3.0F, 0.0F,
     {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE, SHAPE_HURTS_PLAYERS | SHAPE_GUARD,
     PUSH_ENEMIES | PUSH_OBJECTS, {0.0F, 2.8F, 0.0F}, 2.0F, 2.8F, 0.0F, {0}, 0,
     NULL},
    {SHAPE_SPHERE | SHAPE_OFF | SHAPE_HURTABLE,
     SHAPE_HURTS_ENEMIES | SHAPE_GUARD | ATTACK(2), 0, {0.0F, 2.8F, 0.0F}, 3.0F,
     0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* one step of the fruit's flight. accel lasts the one step, and a fruit on
 * top of something takes no GRAVITY */
static void fruit_move(struct task_work *work, struct lockon_target *record)
{
    record->motion.speed.x += record->motion.accel.x;
    if (work->flags & COLLISION_ON_TOP) {
        record->motion.speed.y += record->motion.accel.y;
    } else {
        record->motion.speed.y += record->motion.accel.y - GRAVITY;
    }
    record->motion.speed.z += record->motion.accel.z;
    work->pos.x += record->motion.speed.x;
    work->pos.y += record->motion.speed.y;
    work->pos.z += record->motion.speed.z;
    record->motion.accel.x = 0.0F;
    record->motion.accel.y = 0.0F;
    record->motion.accel.z = 0.0F;
}

/* draw the fruit, 1.2 higher while a player holds it, unless it is out of
 * view */
static void fruit_show(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    Float scale = work->scl.x * alife_fruit_scales[FRUIT_KIND(work)];
    Angle ang;

    /* @bug mismatched-call: the cull reads the camera's squared distance from
     * R0 as an int (LDS R0,FPUL and FLOAT at 0x0c919962 and 0x0c919968), though
     * alife_camera_distance_squared returns it in FR0 and never writes R0. R0
     * still holds 44, scl.x's offset, from MOV #44,R0 at 0x0c919958 before the
     * call at 0x0c91995e, so no fruit is culled for its distance */
    if (alife_camera_distance_squared_int(&work->pos) > 10000.0F) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        return;
    }
    njSetTexture(&alife_objects_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    if (work->flags & CARRY_HELD) {
        njTranslate(NULL, 0.0F, 1.2F, 0.0F);
    }
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    njScale(NULL, scale, scale, scale);
    draw_model(alife_fruit_models[FRUIT_KIND(work)]);
    njPopMatrix(1);
}

/* the fruit turns onto its side and lies there. a push starts it falling,
 * a Chao's hold starts FRUIT_EATEN, and a player's starts FRUIT_HELD */
static void fruit_rest(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    struct collision_work *collision = work->collision;
    struct alife_entry *entry = task->extra;

    switch (work->submode) {
    case 0:
        entry->flags &= ~ENTRY_HITTING;
        work->submode++;
        /* fallthrough */
    case 1:
        if (collision->flags & COLLISION_PUSHED) {
            enemy_collide(work, record);
            work->mode = FRUIT_FALL;
            work->submode = 0;
            work->timer = 0;
        }
        work->ang.x = angle_approach(work->ang.x, 0x4000, 0x400);
        work->ang.z = angle_approach(work->ang.z, 0, 0x400);
        if (entry->flags & ENTRY_HELD) {
            work->mode = FRUIT_EATEN;
            work->submode = 0;
            work->timer = 0;
        } else if (work->flags & CARRY_HELD) {
            work->mode = FRUIT_HELD;
            work->submode = 0;
            work->timer = 0;
        } else {
            collision_register(work);
        }
        break;
    }
}

/* the fruit falls and bounces, slowing on the ground. it sounds SE_BOUNCE
 * on each step it is on the ground. from its 7th step in FRUIT_FALL, it
 * sounds only while |speed.x * speed.y * speed.z| is 0.1 or more. a Chao's
 * hold or a player's takes it out of the fall */
static void fruit_fall(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    struct alife_entry *entry = task->extra;

    switch (work->submode) {
    case 0:
        entry->flags &= ~(ENTRY_LET_GO | ENTRY_HELD);
        work->submode++;
        /* fallthrough */
    case 1:
        fruit_move(work, record);
        enemy_collide(work, record);
        if (njScalor(&record->motion.speed) > HIT_SPEED) {
            work->collision->shapes[2].flags &= ~SHAPE_OFF;
        } else {
            work->collision->shapes[2].flags |= SHAPE_OFF;
        }
        if (record->flags & ENEMY_ON_GROUND) {
            record->motion.speed.x *= GROUND_KEEP;
            record->motion.speed.z *= GROUND_KEEP;
            if (TASK_TIMER(work) > 5 &&
                _builtin_fabsf(record->motion.speed.x * record->motion.speed.y *
                               record->motion.speed.z) < 0.1F) {
                break;
            }
            sound_play_at(SE_BOUNCE, 0, 0, -20, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        break;
    }
    if (entry->flags & ENTRY_HELD) {
        work->mode = FRUIT_EATEN;
        work->submode = 0;
        work->timer = 0;
    } else if (work->flags & CARRY_HELD) {
        work->mode = FRUIT_HELD;
        work->submode = 0;
        work->timer = 0;
    } else {
        collision_register(work);
    }
}

/* the fruit in player 0's hands turns as the player turns. a Chao's hold
 * takes it from player 0, and the fruit falls once the player lets go */
static void fruit_held(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    struct alife_entry *entry = task->extra;

    switch (work->submode) {
    case 0:
        if (work->flags & CARRY_HELD) {
            HELD_TURN(record) = -player_work[0]->ang.y - work->ang.y;
        }
        entry->flags &= ~ENTRY_HITTING;
        entry->held = NULL;
        work->submode++;
        /* fallthrough */
    case 1:
        if (work->flags & CARRY_HELD) {
            work->ang.y = -player_work[0]->ang.y + HELD_TURN(record);
            if (entry->flags & ENTRY_HELD) {
                player_work[0]->flags &= ~PLAYER_HOLDING;
                player_get(0)->held = NULL;
                work->flags &= ~CARRY_HELD;
                work->mode = FRUIT_EATEN;
                work->submode = 0;
                work->timer = 0;
            }
        } else {
            work->mode = FRUIT_FALL;
            work->submode = 0;
            work->timer = 0;
        }
        break;
    }
}

/* the fruit in a Chao's hands turns as the Chao turns. each ENTRY_LET_GO
 * the Chao sets on the fruit's entry is a bite, which takes a quarter of the
 * fruit's size. once its bites leave nothing, the fruit sets ENTRY_LET_GO
 * on the Chao's entry, and on the next step it ends. it falls if the Chao
 * lets go first */
static void fruit_eaten(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    struct alife_entry *entry = task->extra;
    struct alife_entry *holder;

    switch (work->submode) {
    case 0:
        if ((entry->flags & ENTRY_HELD) && entry->held) {
            holder = entry->held;
            if (holder && holder->task) {
                HELD_TURN(record) = holder->task->work->ang.y - work->ang.y;
            }
        } else {
            work->mode = FRUIT_FALL;
            work->submode = 0;
            work->timer = 0;
        }
        entry->flags &= ~ENTRY_HITTING;
        work->submode++;
        /* fallthrough */
    case 1:
        if ((entry->flags & ENTRY_HELD) && entry->held) {
            holder = entry->held;
            if (holder) {
                work->ang.y = holder->task->work->ang.y + HELD_TURN(record);
            }
            if (entry->flags & ENTRY_LET_GO) {
                work->scl.x -= 0.25F;
                work->timer = 0;
                if (work->scl.x <= 0.0F) {
                    work->submode++;
                }
                entry->flags &= ~ENTRY_LET_GO;
            }
        } else {
            work->mode = FRUIT_FALL;
            work->submode = 0;
            work->timer = 0;
            entry->flags &= ~(ENTRY_LET_GO | ENTRY_HELD);
        }
        break;
    case 2:
        if (entry->held) {
            entry->held->flags |= ENTRY_LET_GO;
        }
        work->submode++;
        break;
    case 3:
        if (entry->held) {
            entry->held->held = NULL;
        }
        alife_entry_remove(entry);
        task_kill(task);
        break;
    }
}

static void fruit_step(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    switch (work->mode) {
    case FRUIT_REST:
        fruit_rest(task, record);
        break;
    case FRUIT_FALL:
        fruit_fall(task, record);
        break;
    case FRUIT_HELD:
        fruit_held(task, record);
        break;
    case FRUIT_EATEN:
        fruit_eaten(task, record);
        break;
    }
    fruit_show(task, record);
    work->timer++;
}

static void fruit_draw(struct task *task)
{
    struct lockon_target *record = LOCKON_TARGET(task);

    fruit_show(task, record);
}

static void fruit_destroy(struct task *task)
{
    alife_fruit_count--;
    if (alife_fruit_count < 0) {
        alife_fruit_count = 0;
    }
}

static void fruit_init(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    collision_init(task, fruit_shapes, COLLISION_ENEMY, 1, 3);
    task->work->collision->flags &= ~COLLISION_TARGET;
    record->flags |= ENEMY_KEEP_ON_LEDGE | ENEMY_SHADOW;
    alife_entry_add(task, ALIFE_PLACES, ALIFE_FRUIT, 3.0F);
    record->probe_offset.x = 0.0F;
    record->probe_offset.y = 3.0F;
    record->probe_offset.z = 0.0F;
    record->top = 1.0F;
    record->radius = 3.0F;
    record->bottom = -5.0F;
    record->wall_bounce = 0.8F;
    record->floor_bounce = 0.6F;
    record->friction = 1.0F;
    record->unk_c4 = 1.0F;
    record->shadow_size = 0.3F;
    record->shadow_ratio = 1.0F;
    enemy_set_home(work, record);
    carry_set_kind(work, task->motion, CARRY_KIND_SMALL);
    record->motion.aim.x = 0;
    record->motion.aim.y = 0;
    record->motion.aim.z = 0;
    record->motion.unk_28 = 0;
    work->scl.x = 1.0F;
    alife_fruit_count++;
    task->step = fruit_step;
    task->destructor = fruit_destroy;
    task->draw = fruit_draw;
}

void alife_fruit_create(NJS_POINT3 *pos, NJS_VECTOR *velocity, Sint8 kind)
{
    struct task *task = task_create(TASK_WORK, 4, fruit_init);
    struct task_work *work = task->work;
    struct lockon_target *record = enemy_alloc_record(task, work);

    task->motion = &record->motion;
    work->pos = *pos;
    record->motion.speed = *velocity;
    FRUIT_KIND(work) = kind;
    work->mode = FRUIT_FALL;
    work->submode = 0;
    work->timer = 0;
}

/* mode 0 moves the work to mode 1. in mode 1, each step tests the camera's
 * distance from the flower and ignores the result */
void garden_flower_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case 0:
        work->mode++;
        return;
    case 1:
        if (placement_far_from_camera_by(task, SQUARE(500.0F))) {
            return;
        }
        break;
    default:
        return;
    }
}
