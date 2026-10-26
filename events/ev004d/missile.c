/* Eggman's missile, in EV004D.PRS. as the MISSILE_ bits ask, its lamps
 * glow, it trails smoke and its band's texture rolls */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "ev004d/missile.h"
#include "event/event.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 4
#define TEXT_5_LENGTH 14
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 5
#include "ev004d/missile/text.inc"

struct missile_vars {
    Float glow; /* the lamps' alpha, the sine of glow_phase */
    Angle glow_phase;
    Sint16 scroll_speed; /* added each step to the v of every uv in the band */
    Sint16 exhaust_wait; /* steps to wait before the next puff of smoke */
};

#define MISSILE(work) ((struct missile_vars *)&(work)->vars)

#define BAND_MESHSET 2 /* the band, the model's third meshset */
#define BAND_UVS 40 /* uvs in the band's meshset */
#define EXHAUST_SIZE 1.0F /* a puff's size at the missile's scale of 1 */
#define EXHAUST_WAIT 4 /* exhaust_wait's start: a puff every 4 steps */
#define EXHAUST_RISE 0.2F /* the share of an upward speed a puff keeps */

static void ev004d_missile_scroll(struct task_work *work);
static void ev004d_missile_draw_lamps(struct task_work *work);
static void ev004d_missile_exhaust(struct task *task);

static NJS_TEXNAME ev004d_missile_textures[7] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0},
};
NJS_TEXLIST ev004d_missile_texlist = {
#include "ev004d/missile/ev004d_missile_texlist.inc"
};
static NJS_MATERIAL ev004d_missile_materials[7] = {
#include "ev004d/missile/ev004d_missile_materials.inc"
};
static Sint16 ev004d_missile_meshes0[24] = {
#include "ev004d/missile/ev004d_missile_meshes0.inc"
};
static Sint16 ev004d_missile_meshes1[86] = {
#include "ev004d/missile/ev004d_missile_meshes1.inc"
};
static Sint16 ev004d_missile_meshes2[44] = {
#include "ev004d/missile/ev004d_missile_meshes2.inc"
};
static Sint16 ev004d_missile_meshes3[22] = {
#include "ev004d/missile/ev004d_missile_meshes3.inc"
};
static Sint16 ev004d_missile_meshes4[19] = {
#include "ev004d/missile/ev004d_missile_meshes4.inc"
};
static Sint16 ev004d_missile_meshes5[19] = {
#include "ev004d/missile/ev004d_missile_meshes5.inc"
};
static Sint16 ev004d_missile_meshes6[14] = {
#include "ev004d/missile/ev004d_missile_meshes6.inc"
};
static NJS_COLOR ev004d_missile_uvs0[20] = {
#include "ev004d/missile/ev004d_missile_uvs0.inc"
};
static NJS_COLOR ev004d_missile_uvs1[76] = {
#include "ev004d/missile/ev004d_missile_uvs1.inc"
};
static NJS_COLOR ev004d_missile_uvs2[BAND_UVS] = {
#include "ev004d/missile/ev004d_missile_uvs2.inc"
};
static NJS_COLOR ev004d_missile_uvs3[20] = {
#include "ev004d/missile/ev004d_missile_uvs3.inc"
};
static NJS_COLOR ev004d_missile_uvs4[18] = {
#include "ev004d/missile/ev004d_missile_uvs4.inc"
};
static NJS_COLOR ev004d_missile_uvs5[18] = {
#include "ev004d/missile/ev004d_missile_uvs5.inc"
};
static NJS_COLOR ev004d_missile_uvs6[12] = {
#include "ev004d/missile/ev004d_missile_uvs6.inc"
};
static NJS_MESHSET ev004d_missile_meshsets[7] = {
#include "ev004d/missile/ev004d_missile_meshsets.inc"
};
static NJS_POINT3 ev004d_missile_points[105] = {
#include "ev004d/missile/ev004d_missile_points.inc"
};
static NJS_VECTOR ev004d_missile_normals[105] = {
#include "ev004d/missile/ev004d_missile_normals.inc"
};
static NJS_MODEL ev004d_missile_model = {
#include "ev004d/missile/ev004d_missile_model.inc"
};
static NJS_OBJECT ev004d_missile_object = {
#include "ev004d/missile/ev004d_missile_object.inc"
};

/* the lamps' texture, st_cornerlight, the fifth of the missile's textures.
 * ev004d_missile_lamp_anim maps all of it onto a sprite 32 by 32 about its
 * centre */
static NJS_TEXLIST ev004d_missile_lamp_texlist = {
#include "ev004d/missile/ev004d_missile_lamp_texlist.inc"
};
static NJS_TEXANIM ev004d_missile_lamp_anim = {
#include "ev004d/missile/ev004d_missile_lamp_anim.inc"
};
/* points in the missile's own frame. the lamps sit round its base, and the
 * smoke leaves along the line from ev004d_missile_exhaust_from to
 * ev004d_missile_exhaust_to */
static NJS_POINT3 ev004d_missile_lamps[4] = {
    {0.0F, -447.5F, -150.0F},
    {150.0F, -447.5F, 0.0F},
    {0.0F, -447.5F, 150.0F},
    {-150.0F, -447.5F, 0.0F},
};
static NJS_POINT3 ev004d_missile_exhaust_from = {0.0F, -350.0F, 0.0F};
static NJS_POINT3 ev004d_missile_exhaust_to = {0.0F, -370.0F, 0.0F};

/* no code uses these params. they keep event_props at its address in the
 * image */
static struct dust_params ev004d_missile_puff = {
    1.0F,
    0.038F,
    0.19F,
    0.06F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

static void ev004d_missile_destroy(struct task *task)
{
}

static void ev004d_missile_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njSetTexture(&ev004d_missile_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
    draw_tree(&ev004d_missile_object);
    njPopMatrix(1);
}

/* follow the paths in the event record, show what the MISSILE_ bits ask
 * for, and draw the model. only the step draws the lamps, so a paused game
 * shows none */
static void ev004d_missile_step(struct task *task)
{
    struct task_work *work = task->work;

    if (work->event->paths != NULL) {
        event_paths_step(task->work);
    }
    if (work->tick & MISSILE_SCROLL) {
        ev004d_missile_scroll(task->work);
    }
    if (work->tick & MISSILE_LAMPS) {
        ev004d_missile_draw_lamps(task->work);
    }
    if (work->tick & MISSILE_EXHAUST && MISSILE(work)->exhaust_wait == 0) {
        MISSILE(work)->exhaust_wait = EXHAUST_WAIT;
        ev004d_missile_exhaust(task);
    }
    if (work->tick & MISSILE_EXHAUST && MISSILE(work)->exhaust_wait != 0) {
        MISSILE(work)->exhaust_wait--;
    }
    ev004d_missile_draw(task);
}

static void ev004d_missile_init(struct task *task)
{
    struct task_work *work = task->work;

    work->tick = MISSILE_SCROLL | MISSILE_LAMPS;
    /* @hack redundant: scroll_speed set to 0, which the image stores at
     * 0x0cb832c0 before it stores 11 at 0x0cb832da. without it,
     * ev004d_missile_init runs 4 bytes short, and 31 of 35 words differ */
    MISSILE(work)->scroll_speed = 0;
    MISSILE(work)->exhaust_wait = EXHAUST_WAIT;
    MISSILE(work)->glow_phase = 0;
    work->timer = 20;
    work->scl.x = 1.0F;
    work->scl.y = 1.0F;
    work->scl.z = 1.0F;
    MISSILE(work)->scroll_speed = 11;
    event_alloc(task->work);
    task->step = ev004d_missile_step;
    task->draw = ev004d_missile_draw;
    task->destructor = ev004d_missile_destroy;
}

struct task *ev004d_missile_create(void)
{
    /* @hack spelling: the task through a local. SHC then calls task_create
     * and returns, and so saves PR (STS.L PR,@-R15 at 0x0cb832f2), as the
     * image does. returned directly, task_create becomes a tail jump,
     * ev004d_missile_create runs 8 bytes short, and 9 of 9 words differ */
    struct task *task = task_create(TASK_WORK, 3, ev004d_missile_init);

    return task;
}

/* roll the band's texture by moving the v of each of the band's uvs. the
 * model's own uvs change, and nothing puts them back */
static void ev004d_missile_scroll(struct task_work *work)
{
    NJS_COLOR *uv = ev004d_missile_meshsets[BAND_MESHSET].vertuv;
    int i;

    for (i = 0; i < BAND_UVS; i++) {
        uv->tex.v += MISSILE(work)->scroll_speed;
        uv++;
    }
}

/* draw the four lamps as sprites that face the camera, their alpha pulsing
 * with glow */
static void ev004d_missile_draw_lamps(struct task_work *work)
{
    Angle ang;
    NJS_SPRITE sprite;
    NJS_POINT3 lamp;
    int i;

    sprite.tlist = &ev004d_missile_lamp_texlist;
    sprite.tanim = &ev004d_missile_lamp_anim;
    MISSILE(work)->glow_phase += 0x200;
    MISSILE(work)->glow = njSin(MISSILE(work)->glow_phase);
    sprite.sx = sprite.sy = work->scl.x;
    sprite.p.x = 0.0F;
    sprite.p.y = 0.0F;
    sprite.p.z = 0.0F;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetTexture(&ev004d_missile_lamp_texlist);
    draw_set_material(MISSILE(work)->glow, 1.0F, 1.0F, 1.0F);
    for (i = 0; i < 4; i++) {
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
        njCalcPoint(NULL, &ev004d_missile_lamps[i], &lamp);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslateV(NULL, &lamp);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        njFogDisable();
        njDrawSprite3D(&sprite, 0, NJD_SPRITE_COLOR | NJD_SPRITE_ALPHA);
        njFogEnable();
        njPopMatrix(1);
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

void ev004d_missile_set_scale(struct task *task, Float scale)
{
    struct task_work *work = task->work;

    if (task != NULL) {
        work->scl.x = work->scl.y = work->scl.z = scale;
    }
}

void ev004d_missile_set_effects(struct task *task, int effects)
{
    struct task_work *work = task->work;

    if (task != NULL) {
        work->tick = effects;
    }
}

/* a puff of smoke's step. it plays its animation once, drifting and
 * growing, then dies */
static void ev004d_missile_exhaust_step(struct particle *particle,
                                        struct particle_pool *pool)
{
    particle->frame = floorf(particle->time);
    if (particle->frame >= (Sint16)pool->anim->frames) {
        particle->frame = pool->anim->frames - 1;
        particle_kill(particle);
        return;
    }
    particle->time += 0.4F;
    VECTOR_ADD(&particle->pos, &particle->speed);
    particle->scale += 0.01F;
}

/* a puff of smoke from the nozzle. its speed is the nozzle's unit
 * direction, with any upward part scaled by EXHAUST_RISE. it makes none
 * when bomb_pool.pool is null or the particles' free list is empty */
static void ev004d_missile_exhaust(struct task *task)
{
    NJS_POINT3 from;
    NJS_VECTOR way;
    struct task_work *work = task->work;
    struct particle *particle =
        particle_alloc(bomb_pool.pool, ev004d_missile_exhaust_step);
    Angle ang;

    if (particle == NULL) {
        return;
    }
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
    njCalcPoint(NULL, &ev004d_missile_exhaust_from, &from);
    njCalcPoint(NULL, &ev004d_missile_exhaust_to, &way);
    njPopMatrix(1);
    way.x -= from.x;
    way.y -= from.y;
    way.z -= from.z;
    njUnitVector(&way);
    particle->speed.x = way.x;
    particle->speed.y = way.y >= 0.0F ? way.y * EXHAUST_RISE : way.y;
    particle->speed.z = way.z;
    particle->scale = work->scl.x * EXHAUST_SIZE;
    particle->color.a = 1.0F;
    particle->color.r = 1.0F;
    particle->color.g = 0.8F;
    particle->color.b = 0.8F;
    particle->time = rand() * RAND_SCALE;
    particle->pos.x = from.x;
    particle->pos.y = from.y - (way.y * 30.0F);
    particle->pos.z = from.z;
}
