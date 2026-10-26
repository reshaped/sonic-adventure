/* the task of the Chao that takes an entrant's place, and the modes its
 * entrant's commands set */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h> /* NOLINT(misc-include-cleaner) */

#include "al_main/chao.h"
#include "al_main/chao_name.h"
#include "al_main/face.h"
#include "al_main/icon.h"
#include "al_main/motion.h"
#include "al_main/physics.h"
#include "al_main/shape.h"
#include "al_main/status.h"
#include "al_race/entrant.h"
#include "al_race/entrant_chao.h"
#include "al_race/game.h"
#include "al_race/race_chao.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

/* the Chao's modes, which its entrant's commands set */
enum entrant_chao_mode {
    ENTRANT_CHAO_IDLE, /* as the task starts: no mode's code runs */
    ENTRANT_CHAO_PLACED, /* on the board: fun_0cb83ec0's motion 8 */
    ENTRANT_CHAO_DANCE, /* under the cursor: entrant_chao_dance */
    ENTRANT_CHAO_STOP, /* entered: race_chao_stop's stop, and a cry */
    ENTRANT_CHAO_POSE /* at the gates: entrant_chao_pose */
};

#define NAME_RISE 4.8F /* the name's height above the Chao */

/* the cries a dancing Chao picks one of at random: SE_DANCE_FIRST up to
 * SE_DANCE_END, never SE_DANCE_END itself */
#define SE_DANCE_FIRST 840
#define SE_DANCE_END 843
/* the three cries an entered Chao picks one of at random */
#define SE_ENTERED_A 854
#define SE_ENTERED_B 877
#define SE_ENTERED_C 883
/* the steps from a window's opening, on race_places' timer, in which an
 * entered Chao doesn't cry out */
#define CRY_WAIT 5

static void entrant_chao_obey(struct task_work *work, struct alife_chao *chao);

/* draw the Chao's name NAME_RISE above its work's pos, at scale, in color */
static void entrant_chao_draw_name(struct task_work *work,
                                   struct alife_chao *chao, Float scale,
                                   NJS_COLOR *color)
{
    NJS_POINT3 pos;

    /* @hack spelling: pos stored through &pos, whose address SHC works out
     * again for each store (0x0cb80ee6 to 0x0cb80f0a), as the image does. as
     * pos.x, pos.y and pos.z, 35 of 38 words differ */
    (&pos)->x = work->pos.x;
    (&pos)->y = work->pos.y;
    (&pos)->z = work->pos.z;
    pos.y += NAME_RISE;
    chao_name_draw_at(chao->record, &pos, scale, color);
}

struct alife_chao *entrant_chao_init(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;

    work->tick = 0;
    work->timer = 0;
    chao->flags |= CHAO_NO_FAR_TREES;
    SET_MODE(work, ENTRANT_CHAO_IDLE);
    collision_init(task, chao_collisions, COLLISION_ENEMY, 1, 2);
    if (work->ang.z) {
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 12;
    }
    alife_motion_init(work, chao);
    alife_terrain_init(work, chao);
    fun_0c90d634(work, chao);
    alife_icon_clear(work, chao);
    alife_face_init(work, chao);
    alife_shape_init(work, chao);
    alife_shape_blend(work, chao);
    alife_shape_set_parts(work, chao);
    alife_shape_set_wins(work, chao);
    alife_set_home(work, chao);
    entrant_set_chao(ENTRANT_VARS(work)->entrant, work, chao);
    return chao;
}

void entrant_chao_step(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;
    struct entrant *entrant = ENTRANT_VARS(work)->entrant;
    NJS_COLOR color;

    chao->scale = 1.0F;
    /* @hack redundant: a return and an else together, which load
     * alife_face_step's address into R1 (0x0cb81062), as the image does.
     * with either alone, SHC loads it into R3, and 4 of 88 words differ */
    if (edit_mode) {
        return;
    } else {
        alife_face_step(work, chao);
        entrant_chao_obey(work, chao);
        alife_motion_step(work, chao);
        if (entrant->flags & ENTRANT_SHOW) {
            alife_chao_show(work, chao);
            if (entrant->flags & ENTRANT_NAME) {
                color.argb.a = 255;
                color.argb.r = 0;
                color.argb.g = 0;
                color.argb.b = 0;
                entrant_chao_draw_name(work, chao, 1.0F, &color);
                entrant->flags &= ~ENTRANT_NAME;
            }
        }
        alife_save_pos(work, chao);
        work->timer++;
        work->tick++;
        if (TASK_TIMER(work) > 0x7fff) {
            work->timer = 0x1000;
        }
        collision_register(work);
        task_step_children(task);
    }
}

void entrant_chao_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;

    entrant_clear(ENTRANT_VARS(work)->entrant);
    fun_0c9002ac(work, chao);
}

/* dance as race_chao_dance_moves does, with a new motion every 23 to 52 steps.
 * each time it starts one, show ICON_SYMBOL_5 and, while the cursor is still on
 * the place, cry out */
static void entrant_chao_dance(struct task_work *work, struct alife_chao *chao)
{
    struct entrant *entrant = ENTRANT_VARS(work)->entrant;

    /* @hack spelling: the submode tested by a switch of one case, which tests
     * it with CMP/EQ #0,R0 (0x0cb8116c); as an if, SHC tests it with TST R0,R0
     * and 1 of 54 words differ */
    switch (work->submode) {
    case 0:
        /* @bug logic: the test of the work's timer against 300, CMP/GE at
         * 0x0cb81178, always holds. race_chao_dance_moves clears the timer as
         * it starts each motion, at 0x0cb84ab2, and starts one every 23 to 52
         * steps */
        if (TASK_TIMER(work) < 300) {
            if (entrant_at_cursor(entrant)) {
                int se = RANDOM(SE_DANCE_FIRST, SE_DANCE_END);

                sound_play(se, 0, 0, 0);
            }
            chao->icon.kind = ICON_SYMBOL_5;
            chao->icon.symbol.state = 0;
            chao->icon.ball.state = 0;
            chao->icon.timer = 30;
        }
        break;
    }
    race_chao_dance_moves(work, chao);
}

/* take motion 29 at twice its speed, then, from the mode's next step, draw
 * the Chao at 2.3 times its size */
static void entrant_chao_pose(struct task_work *work, struct alife_chao *chao)
{
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 29;
        chao->terrain.query_height = 7.0F;
        chao->motion.rate = 2.0F;
        work->submode++;
        work->timer = RANDOM(0, 60);
        break;
    case 1:
        chao->scale = 2.3F;
        break;
    }
}

/* take the command the entrant left, setting the mode it calls for, or
 * else run the mode */
static void entrant_chao_obey(struct task_work *work, struct alife_chao *chao)
{
    struct entrant *entrant = ENTRANT_VARS(work)->entrant;
    int timer;

    if (entrant_take_command(entrant, ENTRANT_TELL_BOARD)) {
        SET_MODE(work, ENTRANT_CHAO_PLACED);
        return;
    }
    if (entrant_take_command(entrant, ENTRANT_TELL_CURSOR)) {
        SET_MODE(work, ENTRANT_CHAO_DANCE);
        return;
    }
    if (entrant_take_command(entrant, ENTRANT_TELL_ENTERED)) {
        SET_MODE(work, ENTRANT_CHAO_STOP);
        return;
    }
    if (entrant_take_command(entrant, ENTRANT_TELL_GATES)) {
        SET_MODE(work, ENTRANT_CHAO_POSE);
        return;
    }
    switch (work->mode) {
    case ENTRANT_CHAO_PLACED:
        fun_0cb83ec0(work, chao);
        break;
    case ENTRANT_CHAO_DANCE:
        entrant_chao_dance(work, chao);
        break;
    case ENTRANT_CHAO_STOP:
        /* on the mode's first run, cry out, but not within CRY_WAIT steps
         * of a window's opening */
        if (fun_0cb8b510(&timer, 0) && timer > CRY_WAIT) {
            int pick;

            /* @hack spelling: a jump past the cry, which loads rand's address
             * into R3 (0x0cb81330), as the image does. as a test around the
             * cry, SHC loads it into R1, and 2 of 140 words differ */
            if (work->submode) {
                goto stop;
            }
            pick = RANDOM(0, 3);
            switch (pick) {
            case 0:
                sound_play(SE_ENTERED_A, 0, 0, 0);
                break;
            case 1:
                sound_play(SE_ENTERED_B, 0, 0, 0);
                break;
            default:
                sound_play(SE_ENTERED_C, 0, 0, 0);
                break;
            }
        }
    stop:
        race_chao_stop(work, chao);
        break;
    case ENTRANT_CHAO_POSE:
        entrant_chao_pose(work, chao);
        break;
    }
}
