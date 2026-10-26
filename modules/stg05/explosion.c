/* the explosion an E-100 series robot goes down in: rays, bombs, and a
 * sphere that grows and fades */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "draw/draw.h"
#include "effect/beam.h"
#include "effect/bomb.h"
#include "sound/sound.h"
#include "stg05/camera_sweep.h"
#include "stg05/explosion.h"
#include "stg05/explosion_textures.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the sphere, SPHERE_RADIUS in radius at a scale of 1 */
extern NJS_OBJECT explosion_sphere_object;

/* an explosion's record, in task->extra. task_destroy frees a task's extra,
 * so the record comes from the heap */
struct explosion {
    int age; /* the steps it has run, 1 on its first */
    NJS_POINT3 pos; /* params.pos points here with EXPLOSION_STILL */
    /* what swell gains each step the rays shine: half a turn over their
     * span */
    Angle swell_turn;
    /* the rays' phase. ray_length is its sine times params.ray_length */
    Angle swell;
    Float ray_length;
    Float radius; /* the sphere's radius now */
    Float growth; /* what the radius gains each step */
    /* the sphere's brightness: 1, falling to 0 from fade_start to fade_end */
    Float light;
    struct explosion_params params;
};

#define EXPLOSION(task) ((struct explosion *)(task)->extra)

/* radius of the sphere model at a scale of 1 */
#define SPHERE_RADIUS 63.0F
/* the textures the sphere's material steps through, one every 2 steps */
#define SPHERE_TEXTURES 8

/* the sound as the sphere starts to grow */
#define SE_EXPLOSION_GROW 138

/* the sphere's shape. it registers only with EXPLOSION_HURTS, and each
 * explosion's copy takes the sphere's radius from grow_start up to grow_end,
 * and keeps that radius after */
static struct collision_shape explosion_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE,
    SHAPE_HURTS_OBJECTS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_PLAYERS | GUARD(3) |
        SHAPE_ATTACK,
    PUSH_PLAYERS,
    {0.0F, 0.0F, 0.0F},
    0.0F,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL};
/* each ray's beam. e100_explosion_rays sets length and max_speed before
 * each create */
static struct beam_params explosion_ray = {
    20, {0.3F, 2.0F}, {0xffffffff, 0x00ffffff}, 91, 0.0F, 1.3F,
};

/* a point on the skin of a sphere of radius 1 */
static const NJS_POINT3 explosion_skin = {0.0F, 0.0F, 1.0F};

static void e100_explosion_draw(struct task *task);

/* advance the rays' phase, and cast a ray from params->pos while their
 * length is above 0 */
static void e100_explosion_rays(struct task *task)
{
    struct explosion *explosion = EXPLOSION(task);
    struct explosion_params *params = &explosion->params;

    explosion->swell += explosion->swell_turn;
    explosion->ray_length = njSin(explosion->swell) * params->ray_length;
    if (explosion->ray_length > 0.0F) {
        explosion_ray.length = explosion->ray_length;
        explosion_ray.max_speed = params->ray_turn;
        beam_create(params->pos, &explosion_ray);
    }
}

/* create a bomb at a random point within reach of params->pos on each axis.
 * the reach and the bomb's size swell and shrink with the rays */
static void e100_explosion_bomb(struct task *task)
{
    struct explosion *explosion = EXPLOSION(task);
    struct explosion_params *params = &explosion->params;
    NJS_POINT3 pos;
    Float size = njSin(explosion->swell) + 0.1F;
    Float reach = (size * 15.0F) + 5.0F;

    pos.x = params->pos->x + (rand() * (reach + reach) * RAND_SCALE) - reach;
    pos.y = params->pos->y + (rand() * (reach + reach) * RAND_SCALE) - reach;
    pos.z = params->pos->z + (rand() * (reach + reach) * RAND_SCALE) - reach;
    bomb_create(&pos, size * 0.45F);
}

/* create a bomb of size at a random point on a sphere of radius about pos.
 * util.h's TURN reads its angle expression twice. it tests the first read
 * for 0, and turns by the second. so each turn below draws two random
 * numbers */
static void e100_explosion_skin_bomb(NJS_POINT3 *pos, Float radius, Float size)
{
    NJS_POINT3 out;

    njPushMatrix(&_unit_matrix_);
    TURN(njRotateX, (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE));
    TURN(njRotateY, (Angle)(rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE));
    njScale(NULL, radius, radius, radius);
    njCalcPoint(NULL, (NJS_POINT3 *)&explosion_skin, &out);
    njPopMatrix(1);
    VECTOR_SUM(&out, &out, pos);
    bomb_create(&out, size);
}

/* from grow_start up to grow_end, the sphere grows by an even share of
 * params->radius each step. after that it keeps growing, by 5% less each
 * step. up to fade_start, a bomb bursts on its skin every 4 steps. past
 * fade_start, the sphere loses 0.5% of its radius each step after it grows.
 * its light then falls to 0 at fade_end */
static void e100_explosion_grow(struct task *task)
{
    struct task_work *work = task->work;
    struct explosion *explosion = EXPLOSION(task);
    struct explosion_params *params = &explosion->params;
    Float last;

    if (explosion->age < params->grow_start) {
        return;
    }
    if (explosion->age == params->grow_start) {
        sound_play(SE_EXPLOSION_GROW, 0, 0, 0);
    }
    if (explosion->age < params->grow_end) {
        last = explosion->radius;
        explosion->radius +=
            params->radius / (params->grow_end - params->grow_start);
        explosion->growth = explosion->radius - last;
        work->collision->shapes->radius = explosion->radius;
    } else {
        explosion->radius += explosion->growth;
        explosion->growth *= 0.95F;
    }
    if (explosion->age > params->fade_start) {
        explosion->radius *= 0.995F;
    } else if (explosion->age % 4 == 0) {
        e100_explosion_skin_bomb(params->pos, explosion->radius, 0.5F);
    }
    /* @hack spelling: the model through the sphere's address, a pool word the
     * draw's pool also holds (0x0c903c38); from the sphere itself, the pool
     * holds the member's address and 10 of 127 words differ */
    (&explosion_sphere_object)->model->mats->attr_texId =
        (explosion->age >> 1) % SPHERE_TEXTURES;
    if (explosion->age > params->fade_start) {
        explosion->light =
            1.0F - ((Float)(explosion->age - params->fade_start) /
                    (params->fade_end - params->fade_start));
    }
}

/* the explosion's step. it casts rays after ray_start and before ray_end,
 * and creates a bomb on every third of those steps. about half the bombs
 * play SE_BLAST. with EXPLOSION_HURTS it registers its shape at
 * params->pos. it sets the camera sweep's reach to the sphere's radius. it
 * ends once its age reaches both ray_end and fade_end */
static void e100_explosion_step(struct task *task)
{
    struct explosion *explosion = EXPLOSION(task);
    struct task_work *work = task->work;
    struct explosion_params *params = &explosion->params;
    NJS_POINT3 *pos;

    ++explosion->age;
    if (explosion->age >= params->ray_end &&
        explosion->age >= params->fade_end) {
        task_kill(task);
    }
    if (explosion->age > params->ray_start &&
        explosion->age < params->ray_end) {
        e100_explosion_rays(task);
        if (explosion->age % 3 == 0) {
            e100_explosion_bomb(task);
            if (rand() * RAND_SCALE <= 0.5F) {
                pos = params->pos;
                sound_play_at(SE_BLAST, 0, 0, -16, pos->x, pos->y, pos->z);
            }
        }
    }
    if (params->flags & EXPLOSION_HURTS) {
        VECTOR_COPY(&work->pos, params->pos);
        collision_register(work);
    }
    e100_explosion_grow(task);
    e100_explosion_draw(task);
    camera_sweep_set_reach(explosion->radius);
}

/* draw the sphere on a frame's first step only. it draws unlit at its light,
 * then again mirrored top to bottom */
static void e100_explosion_draw(struct task *task)
{
    struct explosion *explosion = EXPLOSION(task);
    struct explosion_params *params = &explosion->params;
    Float scale;

    if (frame_step != 0) {
        return;
    }
    njSetTexture(&mexplosion_texlist);
    scale = explosion->radius * (1.0F / SPHERE_RADIUS);
    njPushMatrix(NULL);
    draw_control_save();
    draw_attr_save();
    njControl3D(NJD_CONTROL_3D_CONSTANT_ATTR |
                NJD_CONTROL_3D_CONSTANT_MATERIAL);
    draw_set_material(0.0F, explosion->light, explosion->light,
                      explosion->light);
    draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT);
    njTranslateV(NULL, params->pos);
    njScale(NULL, scale, scale, scale);
    njDrawObject(&explosion_sphere_object);
    njScale(NULL, 1.0F, -1.0F, 1.0F);
    njDrawObject(&explosion_sphere_object);
    draw_attr_restore();
    draw_control_restore();
    njPopMatrix(1);
}

void e100_explosion_create(struct explosion_params *params)
{
    struct task *task;
    struct explosion *explosion;

    if ((task = task_create(TASK_WORK, 4, e100_explosion_step)) != NULL) {
        explosion = heap_alloc(sizeof(struct explosion));
        if (explosion != NULL) {
            collision_init(task, &explosion_shape, COLLISION_ENEMY, 1, 1);
            task->extra = explosion;
            explosion->age = 0;
            explosion->swell_turn =
                0x8000 / (params->ray_end - params->ray_start);
            explosion->swell = 0;
            explosion->ray_length = 0.0F;
            explosion->radius = 0.0F;
            explosion->light = 1.0F;
            explosion->params = *params;
            if (params->flags & EXPLOSION_STILL) {
                explosion->pos = *params->pos;
                explosion->params.pos = &explosion->pos;
            }
            task->draw = e100_explosion_draw;
        } else {
            task_destroy(task);
        }
    }
}
