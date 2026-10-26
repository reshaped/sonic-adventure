/* the Egg Hornet near the ground: its nodes' places, its shadow, and dust
 * and clods where it lands */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm1/effect_textures.h"
#include "b_egm1/egg_hornet.h"
#include "b_egm1/ground.h"
#include "b_egm1/texlists.h"
#include "b_egm1/util.h"
#include "collision/ground.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "system/task.h"
#include "system/util.h"

/* the node whose place the shadow falls under */
#define SHADOW_NODE 0
/* shadow_draw's scale: the shadow's size on a floor right under the node */
#define SHADOW_SCALE 16.0F

#define WHOLE_TURN NJM_DEG_ANG(360)

struct dust_ring {
    NJS_POINT3 center;
    Uint8 mode; /* an enum ground_mode */
};

#define RING_RADIUS 44.0F
#define RING_RISE 0.6F
#define RING_SCALE 3.0F

#define PUFFS 4
#define PUFF_RADIUS 12.0F
#define PUFF_RISE 0.8F
#define PUFF_SCALE 20.0F

struct clod {
    NJS_OBJECT *object;
    NJS_TEXLIST *texlist;
    NJS_VECTOR speed;
    struct angles spin;
    Float scale;
};

struct clod_config {
    NJS_OBJECT *object;
    NJS_TEXLIST *texlist;
    NJS_POINT3 *pos;
    NJS_VECTOR *speed;
    Float scale;
    /* random_spin's range for the clod's spin about each axis, in a step */
    Angle spin;
};

struct clods {
    NJS_POINT3 origin;
    Sint32 steps; /* the steps the clods' task has run */
    Uint8 mode; /* an enum ground_mode */
};

/* what each step adds to a clod's upward speed. CLOD_SPIN is random_spin's
 * range for a clod's spin about each axis in a step. the spin is up to 4
 * degrees either way */
#define CLOD_GRAVITY (-0.1F)
#define CLOD_SPIN NJM_DEG_ANG(8)

/* by a clod's size: its scale, its first upward speed, and how far out it
 * starts */
#define CLOD_SIZES 3
static const Float clod_scales[CLOD_SIZES] = {0.5F, 1.0F, 0.6F};
static const Float clod_rises[CLOD_SIZES] = {1.0F, 0.7F, 0.8F};
static const Float clod_radii[CLOD_SIZES] = {40.0F, 32.0F, 36.8F};

/* how fast a clod flies outward, and the two kinds of clod */
#define CLOD_KINDS 2
static Float clod_speed = 1.0F;
static NJS_OBJECT *clod_objects[CLOD_KINDS] = {&clod_a_object, &clod_b_object};
static NJS_TEXLIST *clod_texlists[CLOD_KINDS] = {&egm1tsuchi_texlist,
                                                 &egm1tsuchi_texlist};

/* the ring and the clods made last, which dust_ring_set_mode and
 * clods_set_mode reach */
static struct task *dust_ring_task;
static struct task *clods_task;

Bool egg_hornet_node_matrix(struct task *task, Uint16 node, NJS_MATRIX *matrix)
{
    struct egg_hornet *record = task->extra;
    NJS_ACTION *action = record->motion.actions[record->motion.current].action;

    pose_action(NULL, action, record->motion.frame, POSE_LAST_NODE, NULL);
    return pose_get_matrix(node, matrix);
}

void egg_hornet_shadow(struct task *task)
{
    struct task_work *work = task->work;
    NJS_MATRIX matrix;
    NJS_POINT3 pos;

    VECTOR_COPY(&pos, &work->pos);
    njPushMatrix(NULL);
    njUnitMatrix(NULL);
    egg_hornet_place(task);
    egg_hornet_node_matrix(task, SHADOW_NODE, &matrix);
    njGetTranslation(&matrix, &work->pos);
    njPopMatrix(1);
    shadow_draw(work, SHADOW_SCALE);
    VECTOR_COPY(&work->pos, &pos);
}

static void dust_ring_step(struct task *task)
{
    struct dust_ring *ring = task->extra;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Angle ang;

    switch (ring->mode) {
    case GROUND_ON:
        ang = random_spin(WHOLE_TURN);
        pos.x = ring->center.x + (njCos(ang) * RING_RADIUS);
        pos.y = ring->center.y;
        pos.z = ring->center.z + (njSin(ang) * RING_RADIUS);
        speed.x = 0.0F;
        speed.y = RING_RISE;
        speed.z = 0.0F;
        dust_create(&pos, &speed, RING_SCALE);
        break;
    default:
        task_kill(task);
        break;
    }
}

void dust_ring_create(NJS_POINT3 *center)
{
    struct task *task;
    struct dust_ring *ring;

    if ((dust_ring_task = task = task_create(0, 3, dust_ring_step)) != NULL) {
        ring = heap_calloc(1, sizeof(struct dust_ring));
        task->extra = ring;
        ring->center = *center;
        ring->mode = GROUND_ON;
        task->step = dust_ring_step;
    }
}

void dust_ring_set_mode(enum ground_mode mode)
{
    struct dust_ring *ring = dust_ring_task->extra;

    ring->mode = mode;
}

void landing_puffs(NJS_POINT3 *origin)
{
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Angle ang;
    int i;

    for (i = 0; i < PUFFS; i++) {
        ang = random_spin(WHOLE_TURN);
        pos.x = origin->x + (njCos(ang) * PUFF_RADIUS);
        pos.y = origin->y;
        pos.z = origin->z + (njSin(ang) * PUFF_RADIUS);
        speed.x = 0.0F;
        speed.y = PUFF_RISE;
        speed.z = 0.0F;
        dust_create(&pos, &speed, PUFF_SCALE);
    }
}

static void clod_draw(struct task *task);

static void clod_step(struct task *task)
{
    struct task_work *work = task->work;
    struct clod *clod = task->extra;

    work->pos.x = work->pos.x + clod->speed.x;
    work->pos.y = work->pos.y + clod->speed.y;
    work->pos.z = work->pos.z + clod->speed.z;
    clod->speed.y += CLOD_GRAVITY;
    work->ang.x = work->ang.x + clod->spin.x;
    work->ang.y = work->ang.y + clod->spin.y;
    work->ang.z = work->ang.z + clod->spin.z;
    if (work->pos.y < 0.0F) {
        task_kill(task);
        return;
    }
    clod_draw(task);
}

static void clod_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct clod *clod = task->extra;
    Float scale;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njRotateZXY(NULL, work->ang.x, work->ang.y, work->ang.z);
    scale = clod->scale;
    njScale(NULL, scale, scale, scale);
    njSetTexture(clod->texlist);
    draw_tree(clod->object);
    njPopMatrix(1);
}

static void clod_create(struct clod_config *config)
{
    struct task *task;
    struct task_work *work;
    struct clod *clod;

    if ((task = task_create(TASK_WORK, 3, clod_step)) != NULL) {
        work = task->work;
        task->extra = clod = heap_calloc(1, sizeof(struct clod));
        clod->object = config->object;
        clod->texlist = config->texlist;
        work->pos.x = config->pos->x;
        work->pos.y = config->pos->y;
        work->pos.z = config->pos->z;
        clod->speed.x = config->speed->x;
        clod->speed.y = config->speed->y;
        clod->speed.z = config->speed->z;
        clod->scale = config->scale;
        clod->spin.x = random_spin(config->spin);
        clod->spin.y = random_spin(config->spin);
        clod->spin.z = random_spin(config->spin);
        work->ang.x = random_spin(WHOLE_TURN);
        work->ang.y = random_spin(WHOLE_TURN);
        work->ang.z = random_spin(WHOLE_TURN);
        task->step = clod_step;
        task->draw = clod_draw;
    }
}

/* make a clod every second step. steps is even then, so size runs 2, 1 and
 * 0 in turn. kind is always 0 */
static void clods_step(struct task *task)
{
    struct clods *clods = task->extra;
    struct clod_config config;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Angle ang;
    int size;
    int kind;

    switch (clods->mode) {
    case GROUND_ON:
        if ((++clods->steps & 1) == 0) {
            ang = random_spin(WHOLE_TURN);
            size = clods->steps % CLOD_SIZES;
            pos.x = clods->origin.x + (njCos(ang) * clod_radii[size]);
            pos.y = clods->origin.y;
            pos.z = clods->origin.z + (njSin(ang) * clod_radii[size]);
            speed.x = njCos(ang) * clod_speed;
            speed.y = clod_rises[size];
            speed.z = njSin(ang) * clod_speed;
            config.pos = &pos;
            config.speed = &speed;
            config.scale = clod_scales[size];
            config.spin = CLOD_SPIN;
            kind = clods->steps % CLOD_KINDS;
            config.object = clod_objects[kind];
            config.texlist = clod_texlists[kind];
            clod_create(&config);
        }
        break;
    default:
        task_kill(task);
        break;
    }
}

void clods_create(NJS_POINT3 *pos)
{
    struct task *task;
    struct clods *clods;

    if ((clods_task = task = task_create(0, 3, clods_step)) != NULL) {
        clods = heap_calloc(1, sizeof(struct clods));
        task->extra = clods;
        clods->origin = *pos;
        clods->mode = GROUND_ON;
        clods->steps = 0;
        task->step = clods_step;
    }
}

void clods_set_mode(enum ground_mode mode)
{
    struct clods *clods = clods_task->extra;

    clods->mode = mode;
}
