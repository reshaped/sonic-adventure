/* Rider motion on Twinkle Park's rotating floors. */
#ifndef STG03_RIDE_H
#define STG03_RIDE_H

#include "player/player.h"
#include "system/task.h"

/* The player sweeps the displacement when it resolves this floor as support.
 * An actor touching the trigger may be standing on a different surface. */
static inline void twinkle_turn_riders(struct task *floor, Angle turn)
{
    struct task_forces *forces = floor->force;
    forces->individual_takeoff = TRUE;
    for (int i = 0; i < TASK_FORCES; i++) {
        struct task_force *force = &forces->riders[i];
        force->turn.y = 0;
        force->speed = (NJS_VECTOR){0};
        if (turn && player_stands_on(i, floor)) {
            const NJS_POINT3 *feet = &player_work[i]->pos;
            Float x = feet->x - floor->work->pos.x;
            Float z = feet->z - floor->work->pos.z;
            Float sine, cosine;
            _builtin_fsca(turn, &sine, &cosine);
            /* Rotate the radius about Y and subtract its starting position. */
            Float change = cosine - 1.0F;
            force->speed.x = change * x + sine * z;
            force->speed.z = change * z - sine * x;
            force->turn.y = -turn;
        }
    }
}

#endif
