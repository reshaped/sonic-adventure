/* O SMOKE, a fire that puffs smoke and throws flames */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv03/smoke.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "effect/flame.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

/* the steps between puffs, which the task's timer counts */
#define PUFF_STEPS 16

/* a puff's age counts its steps of growth from 0. the step that finds it at
 * PUFF_GROWN still grows the puff, then turns it to fading */
#define PUFF_GROWN 100

/* within FLAME_NEAR of the camera the fire throws a flame on every step.
 * within FLAME_FAR, it throws one on the steps its timer is odd */
#define FLAME_NEAR 100.0F
#define FLAME_FAR 500.0F

/* the fire's own pool of puffs, which smoke_destroy kills */
#define POOL(work) ((work)->vars.p[0])

enum smoke_mode { SMOKE_INIT, SMOKE_BURN };

/* a puff's life: it grows, then fades */
enum puff_mode { PUFF_GROW, PUFF_FADE };

struct puff_vars {
    /* @hack type: age as an array of one, which has SHC form its address in
     * a register for each access (ADD #68,R3 at 0x0c902236, ADD #68,R0 at
     * 0x0c902348); as one Sint16, SHC reaches it through @(R0,Rn), and,
     * summed over the two functions that use it, 152 of 291 words differ */
    Sint16 age[1];
};

#define PUFF(particle) ((struct puff_vars *)&(particle)->vars)

/* EFF_REGULAR's roketthrust01 to 08 */
static NJS_TEXLIST smoke_puff_texlist = {
#include "adv03/smoke/smoke_puff_texlist.inc"
};
/* each frame takes its texture with v from 255 at its top to 0 at its
 * bottom, so a puff draws the texture upside down */
static NJS_TEXANIM smoke_puff_frames[8] = {
#include "adv03/smoke/smoke_puff_frames.inc"
};
static struct particle_anim smoke_puff_anim = {
    &smoke_puff_texlist, smoke_puff_frames, 8, NJD_COLOR_BLENDING_SRCALPHA,
    NJD_COLOR_BLENDING_INVSRCALPHA};

/* a sphere about the fire, which pushes every group back and which none can
 * push */
static struct collision_shape smoke_shape[1] = {
    {SHAPE_SPHERE,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 15.0F, 0.0F},
     30.0F,
     0.0F,
     0.0F,
     {0},
     0,
     NULL},
};

static void smoke_destroy(struct task *task);
static void smoke_puff_step(struct particle *particle,
                            struct particle_pool *pool);
static void smoke_flame_create(struct task_work *work);
static void smoke_flame_step(struct particle *particle,
                             struct particle_pool *pool);

void past_smoke_step(struct task *task)
{
    struct task_work *work = task->work;
    struct particle *puff;
    struct particle_pool *pool;
    Float x;
    Float z;
    Float distance;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SMOKE_INIT:
        collision_init(task, smoke_shape, COLLISION_OBJECT, 1, 1);
        /* @hack spelling: the pool through a local, which SHC copies to R4
         * (MOV R0,R4 at 0x0c90216e); stored directly, the function runs 2
         * bytes short and 168 of 204 words differ */
        pool = particle_pool_create(&smoke_puff_anim, 0);
        POOL(work) = pool;
        task->destructor = smoke_destroy;
        work->mode = SMOKE_BURN;
        break;
    case SMOKE_BURN:
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) == PUFF_STEPS) {
            puff = particle_alloc(POOL(work), smoke_puff_step);
            if (puff != NULL) {
                puff->life = PUFF_GROW;
                x = ((rand() * RAND_SCALE) - 0.5F) * 10.0F;
                z = ((rand() * RAND_SCALE) - 0.5F) * 10.0F;
                puff->pos.x = work->pos.x + x;
                puff->pos.y = work->pos.y;
                puff->pos.z = work->pos.z + z;
                puff->speed.x = x * 0.1F;
                puff->speed.y = 0.0F;
                puff->speed.z = z * 0.1F;
                puff->frame = (int)(rand() * (1.0F / 8192.0F));
                puff->ang = (int)(((rand() * RAND_SCALE) - 0.5F) * 65536.0F);
                puff->scale = 1.0F + (rand() * (0.5F * RAND_SCALE));
                PUFF(puff)->age[0] = 0;
                puff->color.a = 0.8F;
                puff->color.r = 1.0F;
                puff->color.g = 0.3F;
                puff->color.b = 0.3F;
            }
            TASK_TIMER(work) = 0;
        }
        distance = njDistanceP2P(&camera_work->pos, &work->pos);
        if (distance < FLAME_NEAR) {
            smoke_flame_create(work);
        } else if (distance < FLAME_FAR && (TASK_TIMER(work) & 1)) {
            smoke_flame_create(work);
        }
        break;
    }
    collision_register(work);
}

static void smoke_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (POOL(work) != NULL) {
        particle_pool_kill(POOL(work));
    }
}

static void smoke_puff_step(struct particle *particle,
                            struct particle_pool *pool)
{
    switch (particle->life) {
    case PUFF_GROW:
        if (PUFF(particle)->age[0]++ == PUFF_GROWN) {
            particle->life = PUFF_FADE;
        }
        if (particle->color.r > 0.3F) {
            particle->color.r -= 0.01F;
        }
        particle->scale += 0.02F;
        break;
    case PUFF_FADE:
        particle->color.a -= 0.005F;
        if (particle->color.a < 0.0F) {
            particle_kill(particle);
            return;
        }
        particle->scale += 0.04F;
        break;
    }
    if (particle->ang > 0) {
        particle->ang += 64;
    } else {
        particle->ang -= 64;
    }
    particle->speed.y += 0.05F;
    particle->speed.y *= 0.97F;
    particle->pos.y += particle->speed.y;
}

static void smoke_flame_create(struct task_work *work)
{
    struct particle *flame = particle_alloc(flame_pool.pool, smoke_flame_step);

    if (flame != NULL) {
        flame->life = 30 + (int)(rand() * (30.0F * RAND_SCALE));
        flame->frame = (int)(rand() * (8.0F * RAND_SCALE));
        flame->ang = 0;
        flame->scale = 0.05F + (rand() * (0.05F * RAND_SCALE));
        flame->depth = -10.0F;
        flame->pos.x = work->pos.x + (((rand() * RAND_SCALE) - 0.5F) * 20.0F);
        flame->pos.y = work->pos.y + (rand() * (40.0F * RAND_SCALE)) + 10.0F;
        flame->pos.z = work->pos.z + (((rand() * RAND_SCALE) - 0.5F) * 20.0F);
        flame->speed.x = ((rand() * RAND_SCALE) - 0.5F) * 3.0F;
        flame->speed.y = 0.5F + (rand() * (2.0F * RAND_SCALE));
        flame->speed.z = ((rand() * RAND_SCALE) - 0.5F) * 3.0F;
        flame->color.a = 0.8F;
        flame->color.r = 1.0F;
        flame->color.g = 0.5F;
        flame->color.b = 0.5F;
    }
}

static void smoke_flame_step(struct particle *particle,
                             struct particle_pool *pool)
{
    char *speed;
    char *pos;
    Float x;
    Float y;
    Float z;
    Float drag;
    Float px;
    Float py;
    Float pz;

    if (particle->life == 0) {
        particle_kill(particle);
        return;
    }
    /* @hack spelling: speed read whole through a byte cursor before it is
     * written back; through the members, SHC orders the loads and stores
     * otherwise and 31 of 37 words differ */
    speed = (char *)&particle->speed;
    drag = 0.96F;
    x = *(Float *)speed * drag;
    speed += sizeof(Float);
    y = *(Float *)speed;
    speed += sizeof(Float);
    z = *(Float *)speed * drag;
    speed += sizeof(Float);
    y *= drag;
    y += 0.05F;
    *(Float *)(speed -= sizeof(Float)) = z;
    *(Float *)(speed -= sizeof(Float)) = y;
    *(Float *)(speed -= sizeof(Float)) = x;
    /* @hack spelling: pos the same way; through the members, 23 of 37 words
     * differ */
    pos = (char *)&particle->pos;
    px = *(Float *)pos;
    pos += sizeof(Float);
    py = *(Float *)pos;
    pos += sizeof(Float);
    pz = *(Float *)pos;
    pos += sizeof(Float);
    px += x;
    py += y;
    pz += z;
    pos -= sizeof(Float);
    *(Float *)pos = pz;
    pos -= sizeof(Float);
    *(Float *)pos = py;
    pos -= sizeof(Float);
    *(Float *)pos = px;
    particle->life--;
}
