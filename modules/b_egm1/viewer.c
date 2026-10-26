/* the Egg Hornet on show, for a second pad to move and pose. on pad 1,
 * holding A, the d-pad moves the boss up, down and along x. holding B, up
 * and down turn its jets off and on, and left boosts them. right plays its
 * first turn. X closes the viewer */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "b_egm1/barrier.h"
#include "b_egm1/egg_hornet.h"
#include "b_egm1/flight.h"
#include "b_egm1/ground.h"
#include "b_egm1/jets.h"
#include "b_egm1/texlists.h"
#include "b_egm1/viewer.h"
#include "draw/draw.h"
#include "draw/motion.h"
#include "draw/tree.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

static struct task *viewer_boss_task;

static void viewer_boss_step(struct task *task);
static void viewer_boss_draw(struct task *task);
static void viewer_boss_destroy(struct task *task);

/* set the boss up as egg_hornet_create sets up the fight's. it starts at
 * the same place in EGG_HORNET_WAIT, with its barrier, flight and jets. it
 * takes no collision record and no camera, and faces along z */
static void viewer_boss_init(struct task *task)
{
    struct task_work *work = task->work;
    struct motion *motion;
    struct egg_hornet *record;
    /* @hack type: owner an array of one, which sits on the stack at R15 + 4
     * (MOV.L R12,@(4,R15) at 0x0c9022ce); as a plain local whose address is
     * passed, 17 of 68 words differ */
    struct task *owner[1];

    fun_0c900bc0();
    fun_0c904188();
    motion = motion_create(work, sizeof(struct egg_hornet));
    if (motion != NULL) {
        task->extra = motion;
        record = task->extra;
        work->pos.x = 832.0F;
        work->pos.y = 185.0F;
        work->pos.z = 1000.0F;
        work->ang.x = work->ang.y = work->ang.z = 0;
        work->mode = EGG_HORNET_WAIT;
        motion->wanted = 0;
        motion->current = 0;
        motion->actions = egg_hornet_actions;
        egg_hornet_barrier_init(&record->barrier);
        egg_hornet_flight_reset(task, work);
        owner[0] = task;
        egg_hornet_jets_create(owner);
        motion_step(work, motion);
    }
}

static void viewer_boss_step(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_hornet *record = task->extra;

    viewer_boss_draw(task);
    motion_step(work, &record->motion);
    egg_hornet_barrier_set_owner(&record->barrier, task);
    egg_hornet_barrier_step(&record->barrier);
    egg_hornet_shadow(task);
}

static void viewer_boss_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_hornet *record = task->extra;
    NJS_ACTION *action;

    egg_hornet_barrier_draw(&record->barrier);
    draw_control_save();
    njControl3D(NJD_CONTROL_3D_CONSTANT_MATERIAL);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njRotateZXY(NULL, work->ang.x, work->ang.y, work->ang.z);
    njScale(NULL, 2.0F, 2.0F, 2.0F);
    if (record->motion.state != MOTION_BLENDING) {
        action = record->motion.actions[record->motion.current].action;
    } else {
        action = record->motion.blend;
    }
    njSetTexture(&egg_hornet_texlist);
    draw_action(action, record->motion.frame);
    njSetTexture(&egm1eggman_texlist);
    njTranslate(NULL, 0.0F, -0.7300005F, 0.0F);
    switch (record->motion.current) {
    case ACTION_TURN_1:
        draw_action(&eggman_turn_1_action, record->motion.frame);
        break;
    case ACTION_TURN_2:
        draw_action(&eggman_turn_2_action, record->motion.frame);
        break;
    case ACTION_SHUDDER:
        draw_action(&eggman_shudder_action, record->motion.frame);
        break;
    default:
        draw_tree(&eggman_object);
        break;
    }
    njPopMatrix(1);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    draw_control_restore();
}

static void viewer_boss_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct motion *motion = task->extra;

    motion->state = MOTION_FREE;
    motion_step(work, motion);
    egg_hornet_jets_kill();
}

static struct task *viewer_boss_create(void)
{
    struct task *task;

    if ((viewer_boss_task = task = task_create(TASK_WORK | TASK_MOTION, 1,
                                               viewer_boss_step)) != NULL) {
        viewer_boss_init(task);
        task->step = viewer_boss_step;
        task->draw = viewer_boss_draw;
        task->destructor = viewer_boss_destroy;
    }
    return task;
}

/* the six functions from here stay external: as statics, they make the
 * source compile 0x80 bytes shorter */
void viewer_boss_kill(void)
{
    task_kill(viewer_boss_task);
}

void viewer_boss_set_action(Uint16 action)
{
    struct egg_hornet *record = viewer_boss_task->extra;

    record->motion.wanted = action;
}

Uint16 viewer_boss_get_action(void)
{
    struct egg_hornet *record = viewer_boss_task->extra;

    return record->motion.current;
}

void viewer_set_jets_mode(Uint8 mode)
{
    egg_hornet_jets_set_mode(mode);
}

Uint32 viewer_jets_done_steps(void)
{
    return egg_hornet_jets_done_steps();
}

struct task_work *viewer_boss_work(void)
{
    return viewer_boss_task->work;
}

static void viewer_step(struct task *task)
{
    struct task_work *work;

    if (per_info[1]->on & PDD_DGT_TA) {
        work = viewer_boss_work();
        if (per_info[1]->on & PDD_DGT_KU) {
            work->pos.y += 1.0F;
        }
        if (per_info[1]->on & PDD_DGT_KD) {
            work->pos.y -= 1.0F;
        }
        if (per_info[1]->on & PDD_DGT_KL) {
            work->pos.x -= 1.0F;
        }
        if (per_info[1]->on & PDD_DGT_KR) {
            work->pos.x += 1.0F;
        }
    }
    if (per_info[1]->on & PDD_DGT_TB) {
        if (per_info[1]->press & PDD_DGT_KU) {
            egg_hornet_jets_set_mode(JETS_OFF);
        }
        if (per_info[1]->press & PDD_DGT_KD) {
            egg_hornet_jets_set_mode(JETS_ON);
        }
        if (per_info[1]->press & PDD_DGT_KL) {
            egg_hornet_jets_set_mode(JETS_BOOST);
        }
        if (per_info[1]->press & PDD_DGT_KR) {
            viewer_boss_set_action(ACTION_TURN_1);
        }
    }
    if (per_info[1]->press & PDD_DGT_TX) {
        viewer_boss_kill();
        task_kill(task);
    }
}

void egg_hornet_viewer_create(void)
{
    struct task *task;
    /* @hack spelling: extra, a temporary, which copies the block to R4 for a
     * call that takes nothing (MOV R0,R4 at 0x0c902708); stored straight to
     * task->extra, 9 of 22 words differ */
    void *extra;

    if ((task = task_create(0, 3, viewer_step)) != NULL) {
        /* four bytes for the task's extra, which nothing reads */
        extra = heap_calloc(1, 4);
        task->extra = extra;
        viewer_boss_create();
        task->step = viewer_step;
    }
}
