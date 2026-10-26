/* the Egg Viper and Eggman for EV0024. the event's calls set the boss's mode
 * or leave Eggman a message. the boss and Eggman leave each other messages
 * as each finishes its part */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm3/egg_viper.h"
#include "b_egm3/egg_viper_event.h"
#include "b_egm3/eggmobile3.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "enemy/enemy.h"
#include "system/task.h"
#include "system/util.h"

#define PATH_POINTS 5

enum event_viper_mode {
    VIPER_INIT,
    VIPER_WAIT,
    VIPER_OPEN,
    VIPER_SHUT,
    VIPER_FLY
};

enum event_eggman_mode {
    EGGMAN_INIT,
    EGGMAN_WAIT, /* he waits for EVENT_OPEN */
    EGGMAN_RISE,
    EGGMAN_UP, /* he waits for EVENT_TALK or EVENT_SINK */
    EGGMAN_TALK, /* he talks until EVENT_HUSH or EVENT_SINK */
    EGGMAN_SINK,
    EGGMAN_DOWN /* he waits for EVENT_SHUT */
};

/* the messages in event_viper_flags. each stays set until the task it is
 * for takes it and clears it */
#define EVENT_TALK 0x1 /* to Eggman, from egg_viper_event_talk */
#define EVENT_HUSH 0x2 /* to Eggman, from egg_viper_event_hush */
#define EVENT_SINK 0x4 /* to Eggman, from the boss as it starts to shut */
#define EVENT_DOWN 0x8 /* to the boss, from Eggman once he has sunk */
#define EVENT_OPEN 0x10 /* to Eggman, from the boss once it is open */
#define EVENT_SHUT 0x20 /* to Eggman, from the boss once it is shut */

/* how many frames of its motion the boss plays a step as it opens, and as
 * it shuts */
#define OPEN_SPEED 0.5F
#define SHUT_SPEED 1.0F
/* while Eggman talks, his motion plays back and forth between this frame
 * and its last */
#define TALK_FRAME 35.0F
/* the boss's flight adds 1 / FLY_STEPS to how far along its spline it is
 * each step. it turns by at most FLY_TURN, nearly 4 degrees, about x and
 * about y each step */
#define FLY_STEPS 300.0F
#define FLY_TURN 0x2d8

#define RECORD(task) ((struct egg_viper_record *)(task)->extra)

static void event_viper_step(struct task *task);
static void event_viper_init(struct task *task);
static void event_viper_wait(struct task *task);
static void event_viper_fly(struct task *task);
static void event_viper_open(struct task *task);
static void event_viper_shut(struct task *task);
static void event_viper_draw(struct task *task);
static void event_eggman_step(struct task *task);
static void event_eggman_draw(struct task *task);
static Bool event_motion_play(struct egg_viper_record *record);
static Bool event_motion_rewind(struct egg_viper_record *record);

static void (*event_viper_modes[])(struct task *task) = {
    event_viper_init, event_viper_wait, event_viper_open,
    event_viper_shut, event_viper_fly,
};

/* each segment's kind, which egg_viper_event_create ORs into its number in
 * the segment's work->tick */
/* clang-format off */
static Uint8 event_segment_kinds[EGG_VIPER_SEGMENTS] = {
    0, 0, EGG_VIPER_JOINT,
    0, 0, EGG_VIPER_JOINT,
    0, 0, EGG_VIPER_JOINT,
    0, 0, EGG_VIPER_JOINT,
    EGG_VIPER_TAIL,
};
/* clang-format on */

/* the points of the boss's flight, as offsets from where it starts to fly.
 * event_viper_fly sets path_x, path_y and path_z from them before the spline
 * reads any */
static Float path_offset_x[PATH_POINTS] = {0.0F, 0.0F, 100.0F, 0.0F, -100.0F};
static Float path_offset_y[PATH_POINTS] = {0.0F, 50.0F, 100.0F, 150.0F, 500.0F};
static Float path_offset_z[PATH_POINTS] = {0.0F, 100.0F, 0.0F, -100.0F, 0.0F};
static Float path_x[PATH_POINTS] = {0.0F, 0.0F, 100.0F, 0.0F, -100.0F};
static Float path_y[PATH_POINTS] = {0.0F, 50.0F, 100.0F, 150.0F, 500.0F};
static Float path_z[PATH_POINTS] = {0.0F, 100.0F, 0.0F, -100.0F, 0.0F};

/* no code uses this word. it keeps event_viper_start at its address in the
 * image */
static Sint32 dat_0c95b3c8;
static NJS_POINT3 event_viper_start;
static Sint32 event_viper_flags;
/* Eggman's task, which the boss starts as it opens, or NULL without one */
static struct task *event_eggman_task;
/* the mode the boss's step last ran. a step that finds the mode changed
 * starts it at submode 0 */
static Sint32 event_viper_last_mode;

/* the rest of event_path, which egg_viper_spline_init works out from the
 * points */
static Float path_knots[PATH_POINTS];
static Float path_ddx[PATH_POINTS];
static Float path_ddy[PATH_POINTS];
static Float path_ddz[PATH_POINTS];

static struct egg_viper_spline event_path = {
    path_x,   path_y,   path_z,   path_knots,
    path_ddx, path_ddy, path_ddz, PATH_POINTS,
};

void egg_viper_event_create(Float x, Float y, Float z, Angle ang_y)
{
    int i;
    struct task *task;

    event_viper_start.x = x;
    event_viper_start.y = y;
    event_viper_start.z = z;
    for (i = 0; i < EGG_VIPER_NEEDLES; i++) {
        task = egg_viper_needles[i] =
            task_create(TASK_WORK, 1, egg_viper_needle_step);
        enemy_alloc_record(task, task->work);
        task->extra = heap_calloc(1, sizeof(struct egg_viper_record));
        task->work->tick = i;
    }
    for (i = EGG_VIPER_SEGMENTS - 1; i >= 0; i--) {
        egg_viper_segments[i] = task =
            task_create(TASK_WORK, 1, egg_viper_segment_step);
        enemy_alloc_record(task, task->work);
        task->extra = heap_calloc(1, sizeof(struct egg_viper_record));
        task->work->tick = event_segment_kinds[i] | i;
        egg_viper_segment_pos[i] = &task->work->pos;
        egg_viper_segment_ang[i] = &task->work->ang;
    }
    egg_viper_task = task_create(TASK_WORK, 1, event_viper_step);
    egg_viper_task->extra = heap_calloc(1, sizeof(struct egg_viper_record));
    egg_viper_start_ang = ang_y;
    event_viper_flags = 0;
    event_eggman_task = NULL;
    event_viper_last_mode = 0;
}

void egg_viper_event_kill(void)
{
    int i;

    /* @bug memory: EV0024 calls this twice, in EVENT_PLAY and in
     * EVENT_END, and the second call passes task_kill the tasks the first
     * killed. a task that has since taken a killed task's slot in the pool
     * dies in its place. the calls are the JSRs at 0x0cb81112 and 0x0cb812be
     * in EV0024, and the kills start at the JSR at 0x0c90ab72 */
    for (i = 0; i < EGG_VIPER_NEEDLES; i++) {
        task_kill(egg_viper_needles[i]);
    }
    for (i = EGG_VIPER_SEGMENTS - 1; i >= 0; i--) {
        task_kill(egg_viper_segments[i]);
    }
    task_kill(egg_viper_task);
    if (event_eggman_task != NULL) {
        task_kill(event_eggman_task);
        event_eggman_task = NULL;
    }
}

void egg_viper_event_open(void)
{
    egg_viper_task->work->mode = VIPER_OPEN;
}

void egg_viper_event_talk(void)
{
    event_viper_flags |= EVENT_TALK;
}

void egg_viper_event_hush(void)
{
    event_viper_flags |= EVENT_HUSH;
}

void egg_viper_event_shut(void)
{
    egg_viper_task->work->mode = VIPER_SHUT;
}

void egg_viper_event_fly(void)
{
    egg_viper_task->work->mode = VIPER_FLY;
}

/* the boss's step. it runs the boss's mode, works out the matrices of its
 * tree's 51 nodes and draws it. segment 0 trails node 47, and Eggman rides
 * node EGG_VIPER_EGGMAN_NODE */
static void event_viper_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (work->mode != event_viper_last_mode) {
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        event_viper_last_mode = work->mode;
        work->submode = 0;
    }
    event_viper_modes[work->mode](task);
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    egg_viper_tree_matrices(egg_viper_objects[0], egg_viper_matrices);
    njPopMatrix(1);
    event_viper_draw(task);
}

static void event_viper_init(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = RECORD(task);

    work->submode = 0;
    record->shape = NULL;
    record->motion = NULL;
    record->flags = 0;
    task->destructor = NULL;
    task->draw = event_viper_draw;
    work->pos.x = event_viper_start.x;
    work->pos.y = event_viper_start.y;
    work->pos.z = event_viper_start.z;
    work->ang.x = 0;
    work->ang.y = egg_viper_start_ang;
    work->ang.z = 0;
    work->mode = VIPER_WAIT;
}

static void event_viper_wait(struct task *task)
{
}

/* fly along event_path from where the boss is, turning toward the way it
 * moves. at the spline's end the boss waits */
static void event_viper_fly(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = RECORD(task);
    NJS_POINT3 last;
    struct angles ang;
    int i;

    switch (work->submode) {
    case 0:
        record->motion = NULL;
        record->shape = NULL;
        for (i = 0; i < PATH_POINTS; i++) {
            path_x[i] = path_offset_x[i] + work->pos.x;
            path_y[i] = path_offset_y[i] + work->pos.y;
            path_z[i] = path_offset_z[i] + work->pos.z;
        }
        egg_viper_spline_init(&event_path);
        record->along = 0.0F;
        work->submode = 1;
    case 1:
        last = work->pos;
        record->along += 1.0F / FLY_STEPS;
        egg_viper_spline_point(&event_path, record->along, &work->pos);
        record->moved.x = work->pos.x - last.x;
        record->moved.y = work->pos.y - last.y;
        record->moved.z = work->pos.z - last.z;
        ang.y = (Angle)(atan2f(record->moved.x, record->moved.z) *
                        ANGLE_PER_RADIAN);
        ang.x = (Angle)(atan2f(-record->moved.y,
                               njSqrt((record->moved.x * record->moved.x) +
                                      (record->moved.z * record->moved.z))) *
                        ANGLE_PER_RADIAN);
        work->ang.x = egg_viper_angle_approach(work->ang.x, ang.x, FLY_TURN);
        work->ang.y = egg_viper_angle_approach(work->ang.y, ang.y, FLY_TURN);
        if (record->along >= 1.0F) {
            work->mode = VIPER_WAIT;
        }
        break;
    }
}

/* start Eggman's task, and play the boss's front opening to its end. then
 * EVENT_OPEN tells Eggman to rise, and the boss waits */
static void event_viper_open(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = RECORD(task);

    switch (work->submode) {
    case 0:
        record->shape = NULL;
        record->motion = NULL;
        work->submode = 1;
    case 1:
        record->motion = egg_viper_motions[EGG_VIPER_FRONT_OPEN];
        record->shape = NULL;
        record->frame = 0.0F;
        record->speed = OPEN_SPEED;
        event_eggman_task = task_create(TASK_WORK, 2, event_eggman_step);
        event_eggman_task->extra =
            heap_calloc(1, sizeof(struct egg_viper_record));
        work->submode = 2;
    case 2:
        if (event_motion_play(record)) {
            event_viper_flags |= EVENT_OPEN;
            work->mode = VIPER_WAIT;
        }
        break;
    }
}

/* EVENT_SINK tells Eggman to sink back. once EVENT_DOWN says he has, the
 * boss plays its front opening back to the start, then sends EVENT_SHUT */
static void event_viper_shut(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = RECORD(task);

    switch (work->submode) {
    case 0:
        record->motion = egg_viper_motions[EGG_VIPER_FRONT_OPEN];
        record->shape = NULL;
        work->submode = 1;
    case 1:
        event_viper_flags |= EVENT_SINK;
        work->submode = 2;
    case 2:
        if (event_viper_flags & EVENT_DOWN) {
            event_viper_flags &= ~EVENT_DOWN;
            work->submode = 3;
        }
        break;
    case 3:
        record->speed = SHUT_SPEED;
        work->submode = 4;
    case 4:
        if (event_motion_rewind(record)) {
            event_viper_flags |= EVENT_SHUT;
            work->mode = VIPER_WAIT;
        }
        break;
    }
}

/* draw the boss at its place and turn. a NULL motion draws its tree at rest,
 * and a NULL shape draws it posed but not morphed */
static void event_viper_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = RECORD(task);
    Angle ang;

    draw_attr_save();
    njSetTexture(&egm3mdl_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    if (record->motion == NULL) {
        draw_tree(egg_viper_objects[0]);
    } else if (record->shape == NULL) {
        draw_motion(egg_viper_objects[0], record->motion, record->frame);
    } else {
        draw_shape_motion(egg_viper_objects[0], record->motion, record->shape,
                          record->frame);
    }
    njPopMatrix(1);
    draw_attr_restore();
}

/* Eggman's step. he acts on his messages, and stands at node
 * EGG_VIPER_EGGMAN_NODE turned as the boss is. once the boss is shut, his
 * task kills itself */
static void event_eggman_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *viper = egg_viper_task;
    struct task_work *boss = viper->work;
    struct egg_viper_record *record = RECORD(task);

    switch (work->mode) {
    case EGGMAN_INIT:
        task->destructor = NULL;
        task->draw = event_eggman_draw;
        record->motion = egg_viper_eggman_motions[0];
        record->shape = NULL;
        record->frame = 0.0F;
        record->speed = 1.0F;
        work->mode = EGGMAN_WAIT;
        break;
    case EGGMAN_WAIT:
        if (event_viper_flags & EVENT_OPEN) {
            event_viper_flags &= ~EVENT_OPEN;
            work->mode = EGGMAN_RISE;
        }
        break;
    case EGGMAN_RISE:
        if (event_motion_play(record)) {
            record->speed = -record->speed;
            work->mode = EGGMAN_UP;
        }
        break;
    case EGGMAN_UP:
        if (event_viper_flags & EVENT_TALK) {
            event_viper_flags &= ~EVENT_TALK;
            work->mode = EGGMAN_TALK;
        }
        if (event_viper_flags & EVENT_SINK) {
            event_viper_flags &= ~EVENT_SINK;
            work->mode = EGGMAN_SINK;
        }
        break;
    case EGGMAN_TALK:
        if (record->speed > 0.0F) {
            if (event_motion_play(record)) {
                record->speed = -record->speed;
            }
        } else {
            event_motion_play(record);
            if (!(record->frame > TALK_FRAME)) {
                record->speed = -record->speed;
            }
        }
        if (event_viper_flags & EVENT_HUSH) {
            event_viper_flags &= ~EVENT_HUSH;
            work->mode = EGGMAN_UP;
        }
        if (event_viper_flags & EVENT_SINK) {
            event_viper_flags &= ~EVENT_SINK;
            work->mode = EGGMAN_SINK;
        }
        break;
    case EGGMAN_SINK:
        record->speed = 1.0F;
        if (event_motion_rewind(record)) {
            event_viper_flags |= EVENT_DOWN;
            work->mode = EGGMAN_DOWN;
        }
        break;
    case EGGMAN_DOWN:
        if (event_viper_flags & EVENT_SHUT) {
            event_viper_flags &= ~EVENT_SHUT;
            task_kill(task);
            event_eggman_task = NULL;
        }
        break;
    }
    if (event_eggman_task != NULL) {
        NJS_MATRIX *node =
            &egg_viper_node_matrices[EGG_VIPER_EGGMAN_NODE -
                                     EGG_VIPER_NODE_MATRICES_FIRST];

        work->pos.x = (*node)[12];
        work->pos.y = (*node)[13];
        work->pos.z = (*node)[14];
        work->ang.x = boss->ang.x;
        work->ang.y = boss->ang.y;
        work->ang.z = boss->ang.z;
        event_eggman_draw(task);
    }
}

static void event_eggman_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct egg_viper_record *record = RECORD(task);
    Angle ang;

    draw_attr_save();
    njSetTexture(&egm3mdl_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    draw_motion(egg_viper_eggman_objects[0], record->motion, record->frame);
    njPopMatrix(1);
    draw_attr_restore();
}

/* add the record's speed to its frame, up to its motion's last frame.
 * returns TRUE once the frame is there, and FALSE before */
static Bool event_motion_play(struct egg_viper_record *record)
{
    record->frame += record->speed;
    if (record->frame >= (Float)(record->motion->nbFrame - 1)) {
        record->frame = (Float)(record->motion->nbFrame - 1);
        return TRUE;
    }
    return FALSE;
}

/* take the record's speed from its frame, down to 0. returns TRUE once the
 * frame is there, and FALSE before */
static Bool event_motion_rewind(struct egg_viper_record *record)
{
    record->frame -= record->speed;
    if (record->frame <= 0.0F) {
        record->frame = 0.0F;
        return TRUE;
    }
    return FALSE;
}
