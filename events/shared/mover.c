/* an event's mover: the task that carries out a move. once its steps run
 * out, it frees its slot and kills itself */
#include <ninja.h>

#include "player/player.h"
#include "shared/move.h"
#include "shared/mover.h"
#include "system/task.h"
#include "system/util.h"

/* free the mover's slot, and kill the mover */
/* @hack redundant: the test of task, which a step is never passed null.
 * without it in the eight cases that end a move, the step runs 132 bytes
 * short and 777 of 872 words differ */
#define END_MOVE(task, move)                                                   \
    do {                                                                       \
        if ((task) != NULL) {                                                  \
            event_movers[(move)->slot] = NULL;                                 \
            task_kill(task);                                                   \
        }                                                                      \
    } while (0)

void event_mover_step(struct task *task)
{
    struct task_work *work = task->work;
    struct event_move *move = MOVER(work)->move;
    struct task *player;
    struct player *record;
    Angle ang;
    NJS_VECTOR point;
    NJS_VECTOR moved;
    struct angles turned;

    switch (work->mode) {
    case MOVER_IDLE:
        break;
    case MOVER_LINE_START:
        work->mode = MOVER_LINE;
        break;
    case MOVER_LINE:
        work->pos.x += move->speed.x;
        work->pos.y += move->speed.y;
        work->pos.z += move->speed.z;
        move->target->work->pos.x = work->pos.x;
        move->target->work->pos.y = work->pos.y;
        move->target->work->pos.z = work->pos.z;
        move->target->work->ang.x = move->ang.x;
        move->target->work->ang.y = move->ang.y;
        move->target->work->ang.z = move->ang.z;
        if (++move->step >= move->steps) {
            work->mode = MOVER_LINE_END;
        }
        break;
    case MOVER_LINE_END:
        END_MOVE(task, move);
        break;
    case MOVER_TURN_START:
        work->mode = MOVER_TURN;
        break;
    case MOVER_TURN:
        work->pos.x += move->speed.x;
        work->pos.y += move->speed.y;
        work->pos.z += move->speed.z;
        work->ang.x += move->turn.x;
        work->ang.y += move->turn.y;
        work->ang.z += move->turn.z;
        move->target->work->ang.x = work->ang.x;
        move->target->work->ang.y = work->ang.y;
        move->target->work->ang.z = work->ang.z;
        move->target->work->pos.x = work->pos.x;
        move->target->work->pos.y = work->pos.y;
        move->target->work->pos.z = work->pos.z;
        if (++move->step >= move->steps) {
            work->mode = MOVER_TURN_END;
        }
        break;
    case MOVER_TURN_END:
        END_MOVE(task, move);
        break;
    case MOVER_FOLLOW_START:
        work->mode = MOVER_FOLLOW;
        break;
    case MOVER_FOLLOW:
        work->pos.x = move->base->work->pos.x + move->offset.x;
        work->pos.y = move->base->work->pos.y + move->offset.y;
        work->pos.z = move->base->work->pos.z + move->offset.z;
        move->target->work->pos.x = work->pos.x;
        move->target->work->pos.y = work->pos.y;
        move->target->work->pos.z = work->pos.z;
        move->target->work->ang.x = move->ang.x;
        move->target->work->ang.y = move->ang.y;
        move->target->work->ang.z = move->ang.z;
        if (++move->step >= move->steps) {
            work->mode = MOVER_FOLLOW_END;
        }
        break;
    case MOVER_FOLLOW_END:
        END_MOVE(task, move);
        break;
    /* @hack spelling: MOVER_FOLLOW_ANGLES's cases ahead of MOVER_FOLLOW_TURN's,
     * where the image has their code (MOVER_FOLLOW_ANGLES_START's at
     * 0x0cb84c92 in EV0007), since SHC lays the cases out in the source's
     * order. in enum order, 123 of 872 words differ */
    case MOVER_FOLLOW_ANGLES_START:
        work->mode = MOVER_FOLLOW_ANGLES;
        break;
    case MOVER_FOLLOW_ANGLES:
        work->pos.x = move->base->work->pos.x + move->offset.x;
        work->pos.y = move->base->work->pos.y + move->offset.y;
        work->pos.z = move->base->work->pos.z + move->offset.z;
        work->ang.x = move->base->work->ang.x + move->turn.x;
        work->ang.y = move->base->work->ang.y + move->turn.y;
        work->ang.z = move->base->work->ang.z + move->turn.z;
        move->target->work->pos.x = work->pos.x;
        move->target->work->pos.y = work->pos.y;
        move->target->work->pos.z = work->pos.z;
        move->target->work->ang.x = work->ang.x;
        move->target->work->ang.y = work->ang.y;
        move->target->work->ang.z = work->ang.z;
        if (++move->step >= move->steps) {
            work->mode = MOVER_FOLLOW_ANGLES_END;
        }
        break;
    case MOVER_FOLLOW_ANGLES_END:
        END_MOVE(task, move);
        break;
    case MOVER_FOLLOW_TURN_START:
        work->mode = MOVER_FOLLOW_TURN;
        break;
    case MOVER_FOLLOW_TURN:
        work->pos.x = move->base->work->pos.x + move->offset.x;
        work->pos.y = move->base->work->pos.y + move->offset.y;
        work->pos.z = move->base->work->pos.z + move->offset.z;
        work->ang.x += move->turn.x;
        work->ang.y += move->turn.y;
        work->ang.z += move->turn.z;
        move->target->work->pos.x = work->pos.x;
        move->target->work->pos.y = work->pos.y;
        move->target->work->pos.z = work->pos.z;
        move->target->work->ang.x = work->ang.x;
        move->target->work->ang.y = work->ang.y;
        move->target->work->ang.z = work->ang.z;
        if (++move->step >= move->steps) {
            work->mode = MOVER_FOLLOW_TURN_END;
        }
        break;
    case MOVER_FOLLOW_TURN_END:
        END_MOVE(task, move);
        break;
    case MOVER_FACING_START:
        work->mode = MOVER_FACING;
        break;
    case MOVER_FACING:
        turned.x = move->base->work->ang.x + move->turn.x;
        turned.y = move->base->work->ang.y + move->turn.y;
        turned.z = move->base->work->ang.z + move->turn.z;
        /* @bug uninitialized: njUnitVector reads point, which no path
         * through MOVER_FACING writes first (JSR at 0x0cb84e44 in EV0007).
         * no call starts MOVER_FACING, so the read never runs */
        njUnitVector(&point);
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateZ, turned.z, ang);
        TURN_VIA(njRotateX, turned.x, ang);
        TURN_VIA(njRotateY, turned.y, ang);
        njCalcVector(NULL, &point, &moved);
        njPopMatrix(1);
        work->pos.x = moved.x * MOVER(work)->distance;
        work->pos.y = moved.y * MOVER(work)->distance;
        work->pos.z = moved.z * MOVER(work)->distance;
        move->target->work->pos.x = work->pos.x;
        move->target->work->pos.y = work->pos.y;
        move->target->work->pos.z = work->pos.z;
        /* MOVER_FACING ends through MOVER_FOLLOW_TURN_END, whose case does
         * what MOVER_FACING_END's does */
        if (++move->step >= move->steps) {
            work->mode = MOVER_FOLLOW_TURN_END;
        }
        break;
    case MOVER_FACING_END:
        END_MOVE(task, move);
        break;
    case MOVER_WELD0_START:
        work->mode = MOVER_WELD0;
        break;
    case MOVER_WELD0:
        player = player_get_task(move->player);
        record = player_get(move->player);
        point.x = -record->weld_point0.x + move->offset.x;
        point.y = record->weld_point0.y + move->offset.y;
        point.z = record->weld_point0.z + move->offset.z;
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateZ, player->work->ang.z, ang);
        TURN_VIA(njRotateX, player->work->ang.x, ang);
        TURN_VIA(njRotateY, 0x8000 - player->work->ang.y, ang);
        njCalcVector(NULL, &point, &moved);
        njPopMatrix(1);
        work->pos.x = moved.x + player->work->pos.x;
        work->pos.y = moved.y + player->work->pos.y + record->consts.center_y;
        work->pos.z = moved.z + player->work->pos.z;
        move->target->work->pos.x = work->pos.x;
        move->target->work->pos.y = work->pos.y;
        move->target->work->pos.z = work->pos.z;
        if (++move->step >= move->steps) {
            work->mode = MOVER_WELD0_END;
        }
        break;
    case MOVER_WELD0_END:
        END_MOVE(task, move);
        break;
    case MOVER_WELD1_START:
        work->mode = MOVER_WELD1;
        break;
    case MOVER_WELD1:
        player = player_get_task(move->player);
        record = player_get(move->player);
        point.x = -record->weld_point1.x + move->offset.x;
        point.y = record->weld_point1.y + move->offset.y;
        point.z = record->weld_point1.z + move->offset.z;
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateZ, player->work->ang.z, ang);
        TURN_VIA(njRotateX, player->work->ang.x, ang);
        TURN_VIA(njRotateY, 0x8000 - player->work->ang.y, ang);
        njCalcVector(NULL, &point, &moved);
        njPopMatrix(1);
        work->pos.x = moved.x + player->work->pos.x;
        work->pos.y = moved.y + player->work->pos.y + record->consts.center_y;
        work->pos.z = moved.z + player->work->pos.z;
        move->target->work->pos.x = work->pos.x;
        move->target->work->pos.y = work->pos.y;
        move->target->work->pos.z = work->pos.z;
        if (++move->step >= move->steps) {
            work->mode = MOVER_WELD1_END;
        }
        break;
    case MOVER_WELD1_END:
        END_MOVE(task, move);
        break;
    }
}

void event_mover_draw(struct task *task)
{
}

void event_mover_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (MOVER(work)->move != NULL) {
        heap_free(MOVER(work)->move);
    }
}
