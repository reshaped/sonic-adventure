/* O BURNING, flames that fly out and up from a ring round the object */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv03/burning.h"
#include "effect/flame.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/util.h"

/* the steps from one flame to the next, a power of 2 to mask the timer by */
#define FLAME_STEPS 8
/* a flame starts at a random sprite frame below START_FRAMES and plays
 * FRAME_RATE frames a step. it ends once its frame reaches END_FRAME, so it
 * never shows the last two of flame_pool's ten frames */
#define START_FRAMES 3.0F
#define FRAME_RATE 0.15F
#define END_FRAME 8
/* a flame's scale starts at random from START_SCALE up to START_SCALE + 1.
 * its sprite's angle is random, within TILT / 2 either side of 0 */
#define START_SCALE 1.8F
#define TILT 4096.0F
/* each step, a flame keeps SPEED_KEPT of its speed, then gains LIFT of upward
 * speed. its scale loses SHRINK a step */
#define SPEED_KEPT 0.94F
#define LIFT 0.05F
#define SHRINK 0.02F

#define FLAME_DEPTH (-20.0F)

enum burning_mode { BURNING_INIT, BURNING_RUN };

struct burning_vars {
    Uint8 unk_00[4];
    Sint16 around; /* the next flame's angle round the ring */
    Sint16 rise_phase; /* its sine sets the next flame's upward speed */
};

#define VARS(work) ((struct burning_vars *)&(work)->vars)

static NJS_ARGB flame_color = {1.0F, 1.0F, 1.0F, 1.0F};

static void burning_flame_step(struct particle *particle,
                               struct particle_pool *pool);

void burning_step(struct task *task)
{
    struct task_work *work = task->work;
    struct particle *flame;
    Float x;
    Float y;
    Float z;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case BURNING_INIT:
        work->mode = BURNING_RUN;
        break;
    case BURNING_RUN:
        if ((TASK_TIMER(work)++ & (FLAME_STEPS - 1)) == 0) {
            flame = particle_alloc(flame_pool.pool, burning_flame_step);
            if (flame != NULL) {
                /* the flame's offset on the ring is also its outward speed */
                x = njSin(VARS(work)->around) * work->scl.x;
                z = njCos(VARS(work)->around) * work->scl.x;
                VARS(work)->around += work->ang.x;
                y = (njSin(VARS(work)->rise_phase) + 1.0F) * work->scl.y;
                VARS(work)->rise_phase += work->ang.y;
                flame->time = rand() * (START_FRAMES * RAND_SCALE);
                flame->pos.x = work->pos.x + x;
                flame->pos.y = work->pos.y;
                flame->pos.z = work->pos.z + z;
                flame->scale = (rand() * RAND_SCALE) + START_SCALE;
                flame->ang = ((rand() * RAND_SCALE) - 0.5F) * TILT;
                flame->speed.x = x + (((rand() * RAND_SCALE) - 0.5F) * 2.0F);
                flame->speed.y = y;
                flame->speed.z = z + (((rand() * RAND_SCALE) - 0.5F) * 2.0F);
                flame->depth = FLAME_DEPTH;
                flame->color = flame_color;
            }
        }
        break;
    }
}

static void burning_flame_step(struct particle *particle,
                               struct particle_pool *pool)
{
    char *cursor;
    Float x;
    Float y;
    Float z;
    Float px;
    Float py;
    Float pz;

    particle->frame = particle->time;
    if (particle->frame >= END_FRAME) {
        particle->frame = END_FRAME - 1;
        particle_kill(particle);
        return;
    }
    particle->time += FRAME_RATE;
    /* @hack spelling: speed read whole through a byte cursor before it is
     * written back; through the members, SHC orders the loads and stores
     * otherwise and 33 of 55 words differ */
    cursor = (char *)&particle->speed;
    x = *(Float *)cursor;
    cursor += sizeof(Float);
    y = *(Float *)cursor;
    cursor += sizeof(Float);
    z = *(Float *)cursor;
    cursor += sizeof(Float);
    y *= SPEED_KEPT;
    x *= SPEED_KEPT;
    z *= SPEED_KEPT;
    y += LIFT;
    cursor -= sizeof(Float);
    *(Float *)cursor = z;
    cursor -= sizeof(Float);
    *(Float *)cursor = y;
    cursor -= sizeof(Float);
    *(Float *)cursor = x;
    /* @hack spelling: pos the same way; through the members, 27 of 55
     * words differ */
    cursor = (char *)&particle->pos;
    px = *(Float *)cursor;
    cursor += sizeof(Float);
    py = *(Float *)cursor;
    cursor += sizeof(Float);
    pz = *(Float *)cursor;
    cursor += sizeof(Float);
    px += x;
    py += y;
    pz += z;
    cursor -= sizeof(Float);
    *(Float *)cursor = pz;
    cursor -= sizeof(Float);
    *(Float *)cursor = py;
    cursor -= sizeof(Float);
    *(Float *)cursor = px;
    particle->scale -= SHRINK;
}
