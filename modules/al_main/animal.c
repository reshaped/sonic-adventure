/* the animals loose in a garden, which player 0 and the Chao pick up and
 * let go */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/animal.h"
#include "al_main/entry.h"
#include "al_main/sparkle.h"
#include "animal/animal.h"
#include "animal/textures.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "enemy/enemy.h"
#include "object/common.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* one of a kind's actions, and the speed SPEED takes while it plays */
struct animal_action {
    NJS_ACTION *action;
    Float speed; /* the motion frames it advances each frame */
};

/* the actions alife_animal_actions holds for each kind: loose, held by a
 * player and held by a Chao. the loose action is the kind's action in
 * animal/animal.h, which the stages' animals play too */
enum { ACTION_LOOSE, ACTION_HELD, ACTION_HELD_BY_CHAO, ACTIONS };

/* each kind's actions held by a player and held by a Chao. the deer,
 * ANIMAL_BANB, has one for both */
extern NJS_ACTION goma_held_action;
extern NJS_ACTION goma_chao_held_action;
extern NJS_ACTION pen_held_action;
extern NJS_ACTION pen_chao_held_action;
extern NJS_ACTION rako_held_action;
extern NJS_ACTION rako_chao_held_action;
extern NJS_ACTION kuja_held_action;
extern NJS_ACTION kuja_chao_held_action;
extern NJS_ACTION tuba_held_action;
extern NJS_ACTION tuba_chao_held_action;
extern NJS_ACTION oum_held_action;
extern NJS_ACTION oum_chao_held_action;
extern NJS_ACTION banb_held_action;
extern NJS_ACTION usa_held_action;
extern NJS_ACTION usa_chao_held_action;
extern NJS_ACTION wara_held_action;
extern NJS_ACTION wara_chao_held_action;
extern NJS_ACTION gori_held_action;
extern NJS_ACTION gori_chao_held_action;
extern NJS_ACTION lion_held_action;
extern NJS_ACTION lion_chao_held_action;
extern NJS_ACTION zou_held_action;
extern NJS_ACTION zou_chao_held_action;
extern NJS_ACTION mogu_held_action;
extern NJS_ACTION mogu_chao_held_action;
extern NJS_ACTION koar_held_action;
extern NJS_ACTION koar_chao_held_action;
extern NJS_ACTION suka_held_action;
extern NJS_ACTION suka_chao_held_action;

static struct animal_action alife_animal_actions[ANIMAL_KINDS][ACTIONS] = {
    {{&goma_action, 0.9F},
     {&goma_held_action, 0.9F},
     {&goma_chao_held_action, 0.6F}},
    {{&pen_action, 0.9F},
     {&pen_held_action, 0.5F},
     {&pen_chao_held_action, 0.6F}},
    {{&rako_action, 0.6F},
     {&rako_held_action, 0.6F},
     {&rako_chao_held_action, 0.6F}},
    {{&kuja_action, 0.6F},
     {&kuja_held_action, 0.6F},
     {&kuja_chao_held_action, 0.6F}},
    {{&tuba_action, 0.6F},
     {&tuba_held_action, 0.6F},
     {&tuba_chao_held_action, 0.6F}},
    {{&oum_action, 0.6F},
     {&oum_held_action, 0.6F},
     {&oum_chao_held_action, 0.6F}},
    {{&banb_action, 1.0F},
     {&banb_held_action, 0.6F},
     {&banb_held_action, 0.6F}},
    {{&usa_action, 0.4F},
     {&usa_held_action, 0.6F},
     {&usa_chao_held_action, 0.6F}},
    {{&wara_action, 1.0F},
     {&wara_held_action, 0.6F},
     {&wara_chao_held_action, 0.6F}},
    {{&gori_action, 0.8F},
     {&gori_held_action, 1.1F},
     {&gori_chao_held_action, 0.6F}},
    {{&lion_action, 1.2F},
     {&lion_held_action, 0.6F},
     {&lion_chao_held_action, 0.6F}},
    {{&zou_action, 0.9F},
     {&zou_held_action, 0.6F},
     {&zou_chao_held_action, 0.6F}},
    {{&mogu_action, 1.0F},
     {&mogu_held_action, 0.6F},
     {&mogu_chao_held_action, 0.6F}},
    {{&koar_action, 0.6F},
     {&koar_held_action, 0.6F},
     {&koar_chao_held_action, 0.6F}},
    {{&suka_action, 1.0F},
     {&suka_held_action, 0.6F},
     {&suka_chao_held_action, 0.6F}},
};

static NJS_TEXLIST *alife_animal_texlists[ANIMAL_KINDS] = {
    &goma_texlist, &pen_texlist,  &rako_texlist, &kuja_texlist, &tuba_texlist,
    &oum_texlist,  &banb_texlist, &usa_texlist,  &wara_texlist, &gori_texlist,
    &lion_texlist, &zou_texlist,  &mogu_texlist, &koar_texlist, &suka_texlist,
};

enum animal_mode {
    ANIMAL_LOOSE,
    ANIMAL_THROWN, /* thrown or let go, until it settles */
    ANIMAL_HELD,
    ANIMAL_HELD_BY_CHAO
};

/* what a loose animal does, as animal_wander chooses. BEHAVE_WALK and
 * BEHAVE_FLY add TURN_SPEED to its heading each step, and BEHAVE_WALK_BACK
 * and BEHAVE_FLY_BACK take it away */
enum animal_behavior {
    BEHAVE_STAND,
    BEHAVE_WALK,
    BEHAVE_WALK_BACK,
    BEHAVE_HOP,
    BEHAVE_FLY,
    BEHAVE_FLY_BACK,
    BEHAVE_GLIDE
};

#define TURN_SPEED 0x180

/* an animal's state in work->vars. its kind, an enum animal_kind, is
 * work->tick */
struct animal_vars {
    Uint8 unk_00[4];
    NJS_TEXLIST *texlist;
    /* @hack type: an array of one, which has SHC form behavior's address in
     * a register for each store (MOV R14,R2 and ADD #16,R2 at 0x0c91802a,
     * 0x0c91802e); as a scalar, it stores through @(R0,R14), animal_wander
     * runs 32 bytes short and 286 of 336 words differ */
    Sint8 behavior[1]; /* an enum animal_behavior */
    /* a walker's steps to its next choice of behavior. a bird counts it down
     * without end. it chooses each time the bits of WAIT_CHOOSE reach 0, and
     * its flight bobs while WAIT_BOB is set */
    Sint8 wait;
    /* steps to its next cry. its bits of CRY_GLITTER also time
     * animal_glitter */
    Sint16 cry_wait;
};

#define VARS(work) ((struct animal_vars *)&(work)->vars)

/* bits of wait and cry_wait */
#define WAIT_CHOOSE 0x0f
#define WAIT_BOB 0x80
#define CRY_GLITTER 0x1f

/* how fast the animal's action plays, kept in the record's motion's
 * gravity */
#define SPEED(record) ((record)->motion.gravity)

/* in work->flags, once a Chao has let the animal go. it glitters no
 * more */
#define ANIMAL_HANDLED 0x8000

/* the speed an animal gains downward each step, and the share of its x and
 * z speed it keeps each step on the ground */
#define GRAVITY 0.05F
#define GROUND_KEEP 0.7F

/* a loose animal a Chao has held goes once it has been off screen for more
 * than GONE_STEPS steps */
#define GONE_STEPS 200
/* a thrown animal goes loose after THROWN_STEPS steps, settled or not */
#define THROWN_STEPS 200
/* a held animal turns toward its holder's heading while its timer is below
 * FACE_END, then keeps to it. held, the timer counts 2 a step, so the turn
 * takes 15 steps */
#define FACE_END 30

/* each kind's cry, by enum animal_kind */
static const int animal_cries[ANIMAL_KINDS] = {
    638, 638, 638, 639, 640, 641, 642, 642, 642, 643, 644, 645, 642, 642, 642,
};

/* clang-format off */
static const struct collision_shape animal_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_NO_PUSH | SHAPE_IGNORE_ENEMIES |
         SHAPE_IGNORE_OBJECTS | SHAPE_KEEPS_HIT,
     SHAPE_GUARD, PUSH_FIRM(PUSH_ALL), {0.0F, 1.5F, 0.0F}, 10.0F, 3.0F, 0.0F,
     {0}, 0, NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE, SHAPE_HURTS_PLAYERS | SHAPE_GUARD,
     PUSH_ENEMIES | PUSH_OBJECTS, {0.0F, 2.8F, 0.0F}, 2.0F, 2.8F, 0.0F, {0}, 0,
     NULL},
};
/* clang-format on */

static void animal_move(struct task_work *work, struct lockon_target *record)
{
    record->motion.speed.x += record->motion.accel.x;
    record->motion.speed.y += record->motion.accel.y - GRAVITY;
    record->motion.speed.z += record->motion.accel.z;
    work->pos.x += record->motion.speed.x;
    work->pos.y += record->motion.speed.y;
    work->pos.z += record->motion.speed.z;
    record->motion.accel.x = 0.0F;
    record->motion.accel.y = 0.0F;
    record->motion.accel.z = 0.0F;
}

/* on a frame's first step, advance the action by SPEED, wrapping at its
 * end. while the record blends, advance the blend instead. once the blend
 * passes 1, the next action and its frame take over */
static void animal_play(struct lockon_target *record)
{
    if (frame_step) {
        return;
    }
    if (record->flags & ENEMY_BLENDING) {
        record->blend += record->blend_speed;
        if (record->blend > 1.0F) {
            record->blend = 0.0F;
            record->frame = record->next_frame;
            record->action = record->next_action;
            record->flags &= ~ENEMY_BLENDING;
        }
    } else {
        record->frame += SPEED(record);
        if (!(record->action->motion->nbFrame > record->frame)) {
            record->frame -= record->action->motion->nbFrame;
        }
    }
}

/* draw the animal at 1.2 times its scale, while it is in view and within
 * 140 of the camera */
static void animal_show(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    Float width = work->scl.x * 1.2F;
    Float height = work->scl.y * 1.2F;
    Float distance;
    Angle ang;

    enemy_spring_scale(work, record);
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        return;
    }
    distance = njDistanceP2P(&work->pos, &camera_work->pos);
    if (distance > 140.0F) {
        return;
    }
    njSetTexture(VARS(work)->texlist);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, width, height, width);
    if (record->flags & ENEMY_BLENDING) {
        NJS_MOTION_LINK link;
        NJS_ACTION_LINK action;

        action.motionlink = &link;
        action.object = record->action->object;
        link.motion[0] = record->action->motion;
        link.motion[1] = record->next_action->motion;
        link.frame[0] = record->frame;
        link.frame[1] = record->next_frame;
        draw_action_link(&action, record->blend);
    } else {
        draw_action(record->action, record->frame);
    }
    njPopMatrix(1);
}

/* choose and run a loose animal's behavior. a bird, ANIMAL_KUJA, TUBA or
 * OUM, chooses each 16 steps. it flies or flies back at 1 in 10 each and
 * glides at 1 in 20, and otherwise keeps what it does. ANIMAL_GORI, LION,
 * ZOU and MOGU stand, walk or walk back at even odds. the rest stand, walk,
 * walk back or hop, at even odds too. a walker chooses again after 16 to
 * 111 steps */
static void animal_wander(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    Float chance;
    Float limit;
    Float sine;
    Float cosine;
    Angle ang;
    int i;

    switch (work->tick) {
    case ANIMAL_KUJA:
    case ANIMAL_TUBA:
    case ANIMAL_OUM:
        /* @hack spelling: the decrement spelled out, which forms wait's
         * address in R1 for its store (0x0c918004 to 0x0c91800a); with --,
         * SHC stores through @(R0,R14), the function runs 4 bytes short and
         * 87 of 336 words differ */
        if (!((VARS(work)->wait = VARS(work)->wait - 1) & WAIT_CHOOSE)) {
            chance = rand() * RAND_SCALE;
            if (chance < 0.1F) {
                VARS(work)->behavior[0] = BEHAVE_FLY;
            } else if (chance < 0.2F) {
                VARS(work)->behavior[0] = BEHAVE_FLY_BACK;
            } else if (chance < 0.25F) {
                VARS(work)->behavior[0] = BEHAVE_GLIDE;
            }
        }
        break;
    case ANIMAL_GORI:
    case ANIMAL_LION:
    case ANIMAL_ZOU:
    case ANIMAL_MOGU:
        /* @hack spelling: the decrement spelled out, which counts wait in R1
         * (0x0c91805c to 0x0c918064); with --, it takes R3 and 5 of 336
         * words differ */
        if ((VARS(work)->wait = VARS(work)->wait - 1) <= 0) {
            i = 0;
            limit = 1.0F / 3.0F;
            chance = rand() * RAND_SCALE;
            /* @hack redundant: the continue and the break, which store the
             * choice through R2 (0x0c91808c to 0x0c918092); with a break alone,
             * it takes R3 and 3 of 336 words differ */
            while (chance > limit) {
                limit += 1.0F / 3.0F;
                i++;
                if (i < BEHAVE_WALK_BACK) {
                    continue;
                }
                break;
            }
            VARS(work)->behavior[0] = i;
            VARS(work)->wait = (int)(rand() * (96.0F * RAND_SCALE)) + 16;
        }
        break;
    default:
        /* @hack spelling: the decrement spelled out, which counts wait in R1
         * (0x0c9180e2 to 0x0c9180ea); with --, it takes R3 and 5 of 336
         * words differ */
        if ((VARS(work)->wait = VARS(work)->wait - 1) <= 0) {
            i = 0;
            limit = 0.25F;
            chance = rand() * RAND_SCALE;
            /* @hack redundant: the continue and the break, which store the
             * choice through R2 (0x0c91810e to 0x0c918114); with a break alone,
             * it takes R3 and 3 of 336 words differ */
            while (chance > limit) {
                /* @hack spelling: the sum spelled out, which keeps limit in
                 * FR15 and 0.25 in FR14 (FADD FR14,FR15 at 0x0c918108); with
                 * +=, the two trade and 6 of 336 words differ */
                limit = limit + 0.25F;
                i++;
                if (i < BEHAVE_HOP) {
                    continue;
                }
                break;
            }
            VARS(work)->behavior[0] = i;
            VARS(work)->wait = (int)(rand() * (96.0F * RAND_SCALE)) + 16;
        }
        break;
    }
    switch (VARS(work)->behavior[0]) {
    case BEHAVE_STAND:
        break;
    case BEHAVE_WALK:
    case BEHAVE_WALK_BACK:
        ang = work->ang.y - NJM_DEG_ANG(90);
        sine = njSin(ang);
        cosine = njCos(ang);
        if (work->tick == ANIMAL_USA) {
            record->motion.accel.x =
                (cosine * 0.01F) -
                (record->motion.speed.x * _builtin_fabsf(sine) * 0.01F);
            record->motion.accel.z =
                -(sine * 0.01F) -
                (record->motion.speed.z * _builtin_fabsf(cosine) * 0.01F);
        } else {
            record->motion.accel.x =
                (cosine * 0.048F) -
                (record->motion.speed.x * _builtin_fabsf(sine) * 0.01F);
            record->motion.accel.z =
                -(sine * 0.048F) -
                (record->motion.speed.z * _builtin_fabsf(cosine) * 0.01F);
        }
        if (VARS(work)->behavior[0] == BEHAVE_WALK) {
            work->ang.y += TURN_SPEED;
        } else {
            work->ang.y -= TURN_SPEED;
        }
        break;
    case BEHAVE_HOP:
        record->motion.accel.y = 0.5F;
        VARS(work)->behavior[0] = BEHAVE_STAND;
        break;
    case BEHAVE_FLY:
    case BEHAVE_FLY_BACK:
        ang = work->ang.y - NJM_DEG_ANG(90);
        sine = njSin(ang);
        cosine = njCos(ang);
        record->motion.accel.x =
            (cosine * 0.01F) -
            (record->motion.speed.x * _builtin_fabsf(sine) * 0.02F);
        record->motion.accel.z =
            -(sine * 0.01F) -
            (record->motion.speed.z * _builtin_fabsf(cosine) * 0.02F);
        if (VARS(work)->behavior[0] == BEHAVE_FLY) {
            work->ang.y += TURN_SPEED;
        } else {
            work->ang.y -= TURN_SPEED;
        }
        if (VARS(work)->wait & WAIT_BOB) {
            record->motion.accel.y =
                (njSin(VARS(work)->wait * 1024) * 0.005F) + 0.05F;
        } else {
            record->motion.accel.y = 0.045F;
        }
        break;
    case BEHAVE_GLIDE:
        record->motion.accel.y = 0.045F;
        break;
    }
}

/* a sparkle beside the animal each 32 steps, until ANIMAL_HANDLED */
static void animal_glitter(struct task_work *work)
{
    NJS_POINT3 pos;
    Angle ang;

    if (!(work->flags & ANIMAL_HANDLED) &&
        (VARS(work)->cry_wait & CRY_GLITTER) == 1) {
        ang = (Angle)(rand() * 2.0F);
        pos = work->pos;
        pos.y += 2.7F;
        pos.x += njCos(ang) * 2.0F;
        pos.z += njSin(ang) * 2.0F;
        alife_sparkle_create(&pos, 1.0F);
    }
}

static void animal_loose(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    struct alife_entry *entry = task->extra;

    switch (work->submode) {
    case 0:
        record->action = alife_animal_actions[work->tick][ACTION_LOOSE].action;
        SPEED(record) = alife_animal_actions[work->tick][ACTION_LOOSE].speed;
        work->submode++;
        /* falls through, so the animal wanders from the step it goes loose */
    case 1:
        animal_wander(task, record);
        if (enemy_on_screen(&work->pos)) {
            work->timer = 0;
        }
        if (!(work->flags & CARRY_HOLDABLE)) {
            if (TASK_TIMER(work) > GONE_STEPS) {
                alife_entry_remove(entry);
                task_kill(task);
            }
        }
        animal_move(work, record);
        enemy_collide(work, record);
        if (record->flags & ENEMY_ON_GROUND) {
            record->motion.speed.x *= GROUND_KEEP;
            record->motion.speed.z *= GROUND_KEEP;
        }
        if (entry->flags & ENTRY_HELD) {
            work->mode = ANIMAL_HELD_BY_CHAO;
            work->submode = 0;
            work->timer = 0;
        } else if (work->flags & CARRY_HELD) {
            work->mode = ANIMAL_HELD;
            work->submode = 0;
            work->timer = 0;
        } else {
            collision_register(work);
        }
        break;
    }
}

static void animal_thrown(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    struct alife_entry *entry = task->extra;

    switch (work->submode) {
    case 0:
        record->action = alife_animal_actions[work->tick][ACTION_LOOSE].action;
        SPEED(record) = alife_animal_actions[work->tick][ACTION_LOOSE].speed;
        work->submode++;
        entry->held = NULL;
        /* falls through, so the animal moves from the step it's thrown */
    case 1:
        animal_move(work, record);
        work->ang.x = 0;
        work->ang.z = 0;
        enemy_collide(work, record);
        if (record->flags & ENEMY_ON_GROUND) {
            record->motion.speed.x *= GROUND_KEEP;
            record->motion.speed.z *= GROUND_KEEP;
        }
        if (njScalor(&record->motion.speed) < 0.1F ||
            TASK_TIMER(work) > THROWN_STEPS) {
            work->mode = ANIMAL_LOOSE;
            work->submode = 0;
            work->timer = 0;
        }
        break;
    }
    if (entry->flags & ENTRY_HELD) {
        work->mode = ANIMAL_HELD_BY_CHAO;
        work->submode = 0;
        work->timer = 0;
    } else if (work->flags & CARRY_HELD) {
        work->mode = ANIMAL_HELD;
        work->submode = 0;
        work->timer = 0;
    } else {
        collision_register(work);
    }
    /* @hack redundant: the return, which puts the mode's store in the delay
     * slot of the BRA before it (0x0c918594, 0x0c918596); without it, a NOP
     * fills the slot, the function runs 2 bytes long and 13 of 116 words differ
     */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* held by player 0, turning toward the player's heading. a Chao can take
 * it from the player. let go, it is thrown at a speed of no more than 2 */
static void animal_held(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    struct alife_entry *entry = task->extra;

    switch (work->submode) {
    case 0:
        record->action = alife_animal_actions[work->tick][ACTION_HELD].action;
        SPEED(record) = alife_animal_actions[work->tick][ACTION_HELD].speed;
        entry->held = NULL;
        work->submode++;
        /* falls through, so the animal turns from the step it's taken */
    case 1:
        if (work->flags & CARRY_HELD) {
            if (TASK_TIMER(work)++ < FACE_END) {
                work->ang.y = angle_approach(
                    work->ang.y, NJM_DEG_ANG(90) - player_work[0]->ang.y,
                    0x800);
            } else {
                work->ang.y = NJM_DEG_ANG(90) - player_work[0]->ang.y;
            }
            /* a Chao that takes the animal sets ENTRY_HELD */
            if (entry->flags & ENTRY_HELD) {
                player_work[0]->flags &= ~PLAYER_HOLDING;
                player_get(0)->held = NULL;
                work->flags &= ~CARRY_HELD;
                work->mode = ANIMAL_HELD_BY_CHAO;
                work->submode = 0;
                work->timer = 0;
            }
        } else {
            if (njScalor(&record->motion.speed) > 2.0F) {
                njUnitVector(&record->motion.speed);
                record->motion.speed.x *= 2.0F;
                record->motion.speed.y *= 2.0F;
                record->motion.speed.z *= 2.0F;
            }
            work->mode = ANIMAL_THROWN;
            work->submode = 0;
            work->timer = 0;
        }
        break;
    }
}

/* held by a Chao, turning toward the Chao's heading. it loses
 * CARRY_HOLDABLE, so no player takes it again. let go, it is thrown, and
 * takes ANIMAL_HANDLED */
static void animal_held_by_chao(struct task *task, struct lockon_target *record)
{
    struct task_work *work = task->work;
    struct alife_entry *entry = task->extra;
    struct alife_entry *holder;

    switch (work->submode) {
    case 0:
        record->action =
            alife_animal_actions[work->tick][ACTION_HELD_BY_CHAO].action;
        SPEED(record) =
            alife_animal_actions[work->tick][ACTION_HELD_BY_CHAO].speed;
        work->flags &= ~CARRY_HOLDABLE;
        work->submode++;
        /* falls through, so the animal turns from the step it's taken */
    case 1:
        /* the Chao sets ENTRY_HELD again each step it holds on */
        if (entry->flags & ENTRY_HELD) {
            holder = entry->held;
            if (holder) {
                if (TASK_TIMER(work)++ < FACE_END) {
                    work->ang.y = angle_approach(
                        work->ang.y,
                        holder->task->work->ang.y - NJM_DEG_ANG(90), 0x800);
                } else {
                    work->ang.y = holder->task->work->ang.y - NJM_DEG_ANG(90);
                }
            }
            entry->flags &= ~ENTRY_HELD;
        } else {
            work->flags |= ANIMAL_HANDLED;
            work->mode = ANIMAL_THROWN;
            work->submode = 0;
            work->timer = 0;
        }
        break;
    }
}

/* run the animal's mode, then play and draw its action. it cries again
 * after 120 to 1319 steps */
static void animal_step(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    switch (work->mode) {
    case ANIMAL_LOOSE:
        animal_loose(task, record);
        break;
    case ANIMAL_THROWN:
        animal_thrown(task, record);
        break;
    case ANIMAL_HELD:
        animal_held(task, record);
        break;
    case ANIMAL_HELD_BY_CHAO:
        animal_held_by_chao(task, record);
        break;
    }
    animal_glitter(work);
    animal_play(record);
    animal_show(task, record);
    work->timer++;
    if (--VARS(work)->cry_wait < 0) {
        VARS(work)->cry_wait = (int)RANDOM(120.0F, 1320.0F);
        sound_play_at(animal_cries[work->tick], 0, 0, 0, work->pos.x,
                      work->pos.y, work->pos.z);
    }
}

static void animal_draw(struct task *task)
{
    struct lockon_target *record = LOCKON_TARGET(task);

    animal_show(task, record);
}

static void animal_destroy(struct task *task)
{
}

/* the first step: the animal's shapes, ground, shadow, action and entry.
 * carry_set_kind's gravity takes the place of SPEED, which the thrown
 * mode's first step sets again */
static void animal_init(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    collision_init(task, animal_shapes, COLLISION_ENEMY, 1, 2);
    task->work->collision->flags &= ~COLLISION_TARGET;
    record->flags |= ENEMY_KEEP_ON_LEDGE | ENEMY_SHADOW;
    record->probe_offset.x = 0.0F;
    record->probe_offset.y = 3.0F;
    record->probe_offset.z = 0.0F;
    record->top = 1.0F;
    record->radius = 2.5F;
    record->bottom = -3.0F;
    record->wall_bounce = 1.0F;
    record->floor_bounce = 0.5F;
    record->friction = 1.0F;
    record->unk_c4 = 1.0F;
    record->shadow_size = 0.3F;
    record->shadow_ratio = 1.0F;
    record->action = alife_animal_actions[work->tick][ACTION_LOOSE].action;
    SPEED(record) = alife_animal_actions[work->tick][ACTION_LOOSE].speed;
    record->frame = 0.0F;
    VARS(work)->texlist = alife_animal_texlists[work->tick];
    task->step = animal_step;
    task->destructor = animal_destroy;
    task->draw = animal_draw;
    enemy_set_home(work, record);
    alife_entry_add(task, ALIFE_ANIMALS, ALIFE_ANIMAL, 3.0F);
    carry_set_kind(work, task->motion, CARRY_KIND_SMALL);
    work->flags &= ~ANIMAL_HANDLED;
}

void alife_animal_create(NJS_POINT3 *pos, NJS_VECTOR *velocity, Angle ang,
                         Sint8 kind)
{
    struct task *task = task_create(TASK_WORK, 3, animal_init);
    struct task_work *work = task->work;
    struct lockon_target *record = enemy_alloc_record(task, work);

    task->motion = &record->motion;
    work->pos = *pos;
    work->ang.y = ang;
    record->motion.speed = *velocity;
    if (kind > ANIMAL_KINDS - 1) {
        kind = ANIMAL_MOGU;
    } else if (kind < 0) {
        kind = ANIMAL_MOGU;
    }
    work->tick = kind;
    work->mode = ANIMAL_THROWN;
    work->submode = 0;
    work->timer = 0;
}
