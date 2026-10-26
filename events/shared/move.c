/* an event's moves: the calls that make a mover and its move. mover.c
 * holds the mover's step */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "shared/move.h"
#include "shared/mover.h"
#include "system/task.h"
#include "system/util.h"

/* d degrees in angle units, plus a half and truncated toward 0 */
#define DEGREES_TO_ANGLE(d) ((Angle)(((d) * ANGLE_PER_DEGREE) + 0.5F))

static void event_mover_init(struct task *task);

/* the initializer puts the slots in data, where the image has them. without
 * one, SHC puts them in bss, which follows the module's file: in 21 of the 81
 * events that link move.c, EV00F7 among them, past 0x0cbc8000, where
 * event_load's zeroing ends */
struct task *event_movers[EVENT_MOVERS] = {NULL};

/* the first free slot, or -1 */
static int find_slot(void)
{
    int i;

    for (i = 0; i < EVENT_MOVERS; i++) {
        if (event_movers[i] == NULL) {
            return i;
        }
    }
    return -1;
}

void event_move(struct task *target, Float from_x, Float from_y, Float from_z,
                Float to_x, Float to_y, Float to_z, int steps)
{
    int slot = find_slot();
    struct event_move *move;
    struct task_work *work;

    if (slot < 0) {
        return;
    }
    event_movers[slot] = task_create(TASK_WORK, 5, event_mover_init);
    move = heap_alloc(sizeof(*move));
    work = event_movers[slot]->work;
    MOVER(work)->move = move;
    work->pos.x = from_x;
    work->pos.y = from_y;
    work->pos.z = from_z;
    move->target = target;
    move->from.x = from_x;
    move->from.y = from_y;
    move->from.z = from_z;
    move->to.x = to_x;
    move->to.y = to_y;
    move->to.z = to_z;
    move->ang.x = target->work->ang.x;
    move->ang.y = target->work->ang.y;
    move->ang.z = target->work->ang.z;
    move->steps = steps;
    move->step = 0;
    move->slot = slot;
    move->speed.x = (to_x - from_x) / steps;
    move->speed.y = (to_y - from_y) / steps;
    move->speed.z = (to_z - from_z) / steps;
    if (event_movers[slot] != NULL) {
        work = event_movers[slot]->work;
        work->mode = MOVER_LINE_START;
    }
}

void event_move_turn_degrees(struct task *target, Float from_x, Float from_y,
                             Float from_z, Float to_x, Float to_y, Float to_z,
                             Float ang_x, Float ang_y, Float ang_z,
                             Float to_ang_x, Float to_ang_y, Float to_ang_z,
                             int steps)
{
    int slot = find_slot();
    struct event_move *move;
    struct task_work *work;

    if (slot < 0) {
        return;
    }
    event_movers[slot] = task_create(TASK_WORK, 5, event_mover_init);
    move = heap_alloc(sizeof(*move));
    work = event_movers[slot]->work;
    MOVER(work)->move = move;
    work->pos.x = from_x;
    work->pos.y = from_y;
    work->pos.z = from_z;
    work->ang.x = DEGREES_TO_ANGLE(ang_x);
    work->ang.y = DEGREES_TO_ANGLE(ang_y);
    work->ang.z = DEGREES_TO_ANGLE(ang_z);
    move->target = target;
    move->from.x = from_x;
    move->from.y = from_y;
    move->from.z = from_z;
    move->to.x = to_x;
    move->to.y = to_y;
    move->to.z = to_z;
    move->ang.x = DEGREES_TO_ANGLE(ang_x);
    move->ang.y = DEGREES_TO_ANGLE(ang_y);
    move->ang.z = DEGREES_TO_ANGLE(ang_z);
    move->ang_to.x = DEGREES_TO_ANGLE(to_ang_x);
    move->ang_to.y = DEGREES_TO_ANGLE(to_ang_y);
    move->ang_to.z = DEGREES_TO_ANGLE(to_ang_z);
    move->steps = steps;
    move->step = 0;
    move->slot = slot;
    move->speed.x = (to_x - from_x) / steps;
    move->speed.y = (to_y - from_y) / steps;
    move->speed.z = (to_z - from_z) / steps;
    move->turn.x = DEGREES_TO_ANGLE((to_ang_x - ang_x) / steps);
    move->turn.y = DEGREES_TO_ANGLE((to_ang_y - ang_y) / steps);
    move->turn.z = DEGREES_TO_ANGLE((to_ang_z - ang_z) / steps);
    if (event_movers[slot] != NULL) {
        work = event_movers[slot]->work;
        work->mode = MOVER_TURN_START;
    }
}

void event_move_turn(struct task *target, Float from_x, Float from_y,
                     Float from_z, Float to_x, Float to_y, Float to_z,
                     Angle ang_x, Angle ang_y, Angle ang_z, Angle to_ang_x,
                     Angle to_ang_y, Angle to_ang_z, int steps)
{
    int slot = find_slot();
    struct event_move *move;
    struct task_work *work;

    if (slot < 0) {
        return;
    }
    event_movers[slot] = task_create(TASK_WORK, 5, event_mover_init);
    move = heap_alloc(sizeof(*move));
    work = event_movers[slot]->work;
    MOVER(work)->move = move;
    work->pos.x = from_x;
    work->pos.y = from_y;
    work->pos.z = from_z;
    work->ang.x = ang_x;
    work->ang.y = ang_y;
    work->ang.z = ang_z;
    move->target = target;
    move->from.x = from_x;
    move->from.y = from_y;
    move->from.z = from_z;
    move->to.x = to_x;
    move->to.y = to_y;
    move->to.z = to_z;
    move->ang.x = ang_x;
    move->ang.y = ang_y;
    move->ang.z = ang_z;
    move->ang_to.x = to_ang_x;
    move->ang_to.y = to_ang_y;
    move->ang_to.z = to_ang_z;
    move->steps = steps;
    move->step = 0;
    move->slot = slot;
    move->speed.x = (to_x - from_x) / steps;
    move->speed.y = (to_y - from_y) / steps;
    move->speed.z = (to_z - from_z) / steps;
    move->turn.x = (Float)(to_ang_x - ang_x) / steps;
    move->turn.y = (Float)(to_ang_y - ang_y) / steps;
    move->turn.z = (Float)(to_ang_z - ang_z) / steps;
    if (event_movers[slot] != NULL) {
        work = event_movers[slot]->work;
        work->mode = MOVER_TURN_START;
    }
}

void event_follow(struct task *target, Float x, Float y, Float z, int steps,
                  struct task *base)
{
    int slot = find_slot();
    struct event_move *move;
    struct task_work *work;

    if (slot < 0) {
        return;
    }
    event_movers[slot] = task_create(TASK_WORK, 5, event_mover_init);
    move = heap_alloc(sizeof(*move));
    work = event_movers[slot]->work;
    MOVER(work)->move = move;
    work->pos.x = target->work->pos.x;
    work->pos.y = target->work->pos.y;
    work->pos.z = target->work->pos.z;
    move->ang.x = target->work->ang.x;
    move->ang.y = target->work->ang.y;
    move->ang.z = target->work->ang.z;
    move->target = target;
    move->base = base;
    move->offset.x = x;
    move->offset.y = y;
    move->offset.z = z;
    move->steps = steps;
    move->step = 0;
    move->slot = slot;
    if (event_movers[slot] != NULL) {
        work = event_movers[slot]->work;
        work->mode = MOVER_FOLLOW_START;
    }
}

void event_follow_turn(struct task *target, Float x, Float y, Float z,
                       Angle ang_x, Angle ang_y, Angle ang_z, int steps,
                       struct task *base)
{
    int slot = find_slot();
    struct event_move *move;
    struct task_work *work;

    if (slot < 0) {
        return;
    }
    event_movers[slot] = task_create(TASK_WORK, 5, event_mover_init);
    move = heap_alloc(sizeof(*move));
    work = event_movers[slot]->work;
    MOVER(work)->move = move;
    work->pos.x = target->work->pos.x;
    work->pos.y = target->work->pos.y;
    work->pos.z = target->work->pos.z;
    work->ang.x = target->work->ang.x;
    work->ang.y = target->work->ang.y;
    work->ang.z = target->work->ang.z;
    move->target = target;
    move->base = base;
    move->offset.x = x;
    move->offset.y = y;
    move->offset.z = z;
    move->turn.x = (Float)(ang_x - target->work->ang.x) / steps;
    move->turn.y = (Float)(ang_y - target->work->ang.y) / steps;
    move->turn.z = (Float)(ang_z - target->work->ang.z) / steps;
    move->steps = steps;
    move->step = 0;
    move->slot = slot;
    if (event_movers[slot] != NULL) {
        work = event_movers[slot]->work;
        work->mode = MOVER_FOLLOW_TURN_START;
    }
}

void event_follow_angles(struct task *target, Float x, Float y, Float z,
                         Angle ang_x, Angle ang_y, Angle ang_z, int steps,
                         struct task *base)
{
    int slot = find_slot();
    struct event_move *move;
    struct task_work *work;

    if (slot < 0) {
        return;
    }
    event_movers[slot] = task_create(TASK_WORK, 5, event_mover_init);
    move = heap_alloc(sizeof(*move));
    work = event_movers[slot]->work;
    MOVER(work)->move = move;
    work->pos.x = target->work->pos.x;
    work->pos.y = target->work->pos.y;
    work->pos.z = target->work->pos.z;
    work->ang.x = target->work->ang.x;
    work->ang.y = target->work->ang.y;
    work->ang.z = target->work->ang.z;
    move->target = target;
    move->base = base;
    move->offset.x = x;
    move->offset.y = y;
    move->offset.z = z;
    move->turn.x = ang_x;
    move->turn.y = ang_y;
    move->turn.z = ang_z;
    move->steps = steps;
    move->step = 0;
    move->slot = slot;
    if (event_movers[slot] != NULL) {
        work = event_movers[slot]->work;
        work->mode = MOVER_FOLLOW_ANGLES_START;
    }
}

void event_follow_facing(struct task *target, Float x, Float y, Float z,
                         int steps, struct task *base)
{
    int slot = find_slot();
    struct event_move *move;
    struct task_work *work;

    if (slot < 0) {
        return;
    }
    event_movers[slot] = task_create(TASK_WORK, 5, event_mover_init);
    move = heap_alloc(sizeof(*move));
    work = event_movers[slot]->work;
    MOVER(work)->move = move;
    work->pos.x = target->work->pos.x;
    work->pos.y = target->work->pos.y;
    work->pos.z = target->work->pos.z;
    work->ang.x = target->work->ang.x;
    work->ang.y = target->work->ang.y;
    work->ang.z = target->work->ang.z;
    move->target = target;
    move->base = base;
    move->offset.x = x;
    move->offset.y = y;
    move->offset.z = z;
    MOVER(work)->distance = njSqrt((x * x) + (y * y) + (z * z));
    move->turn.x =
        (Angle)(atan2f(y, z) * ANGLE_PER_RADIAN) - move->base->work->ang.x;
    move->turn.y =
        (Angle)(atan2f(z, x) * ANGLE_PER_RADIAN) - move->base->work->ang.y;
    move->turn.z =
        (Angle)(atan2f(y, x) * ANGLE_PER_RADIAN) - move->base->work->ang.z;
    move->steps = steps;
    move->step = 0;
    move->slot = slot;
    if (event_movers[slot] != NULL) {
        work = event_movers[slot]->work;
        work->mode = MOVER_FOLLOW_ANGLES_START;
    }
}

void event_follow_weld0(struct task *target, int player, Float x, Float y,
                        Float z, int steps)
{
    int slot = find_slot();
    struct event_move *move;
    struct task_work *work;

    if (slot < 0) {
        return;
    }
    event_movers[slot] = task_create(TASK_WORK, 5, event_mover_init);
    move = heap_alloc(sizeof(*move));
    work = event_movers[slot]->work;
    MOVER(work)->move = move;
    work->pos.x = target->work->pos.x;
    work->pos.y = target->work->pos.y;
    work->pos.z = target->work->pos.z;
    move->target = target;
    move->player = player;
    move->offset.x = x;
    move->offset.y = y;
    move->offset.z = z;
    move->steps = steps;
    move->step = 0;
    move->slot = slot;
    if (event_movers[slot] != NULL) {
        work = event_movers[slot]->work;
        work->mode = MOVER_WELD0_START;
    }
}

void event_follow_weld1(struct task *target, int player, Float x, Float y,
                        Float z, int steps)
{
    int slot = find_slot();
    struct event_move *move;
    struct task_work *work;

    if (slot < 0) {
        return;
    }
    event_movers[slot] = task_create(TASK_WORK, 5, event_mover_init);
    move = heap_alloc(sizeof(*move));
    work = event_movers[slot]->work;
    MOVER(work)->move = move;
    work->pos.x = target->work->pos.x;
    work->pos.y = target->work->pos.y;
    work->pos.z = target->work->pos.z;
    move->target = target;
    move->player = player;
    move->offset.x = x;
    move->offset.y = y;
    move->offset.z = z;
    move->steps = steps;
    move->step = 0;
    move->slot = slot;
    if (event_movers[slot] != NULL) {
        work = event_movers[slot]->work;
        work->mode = MOVER_WELD1_START;
    }
}

void event_move_stop(struct task *target)
{
    struct event_move *move;
    int i;

    for (i = 0; i < EVENT_MOVERS; i++) {
        if (event_movers[i] != NULL) {
            move = MOVER(event_movers[i]->work)->move;
            if (move->target == target) {
                task_destroy(event_movers[i]);
                event_movers[i] = NULL;
            }
        }
    }
}

void event_move_stop_all(void)
{
    int i;

    for (i = 0; i < EVENT_MOVERS; i++) {
        if (event_movers[i] != NULL) {
            /* @hack redundant: a read of the move that nothing uses, which
             * loads it in task_destroy's delay slot (MOV.L @R2,R2 at 0x0cb8496a
             * in EV0007); without it, 80 of 94 words differ and the function
             * ends 40 bytes short */
            (void)MOVER(event_movers[i]->work)->move;
            task_destroy(event_movers[i]);
            event_movers[i] = NULL;
        }
    }
}

static void event_mover_init(struct task *task)
{
    task->step = event_mover_step;
    task->draw = event_mover_draw;
    task->destructor = event_mover_destroy;
}
