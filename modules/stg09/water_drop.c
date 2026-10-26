/* water drops that fall, bounce off the floor and player 0, and splash */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "effect/dust.h"
#include "player/player.h"
#include "stg09/casino.h"
#include "stg09/water_drop.h"
#include "stg09/water_smoke.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

struct drop {
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Sint16 life; /* steps left */
    /* the steps to the drop's own splash. a drop without one starts at twice
     * its life, and is gone before the count reaches 0. any splash leaves the
     * count below 0, so a drop splashes at most once */
    Sint16 splash;
    Sint16 ang; /* its sprite's */
    Float floor;
    struct drop *newer;
    struct drop *older;
};

static Float water_drop_gravity = -0.02F;
/* the sprite's scale across and up */
static Float water_drop_width = 0.2F;
static Float water_drop_height = 0.2F;
/* a drop bounces off the floor only while falling faster than this. a
 * bounce off the head or a weld point splashes only when the drop leaves it
 * rising faster */
static Float water_drop_bounce_speed = 0.1F;
static Float water_drop_drag = 0.93F; /* the share of its speed kept */
/* the share of its fall a bounce turns into rise. water_drop_bounce is for
 * the floor and the weld points, and water_drop_head_bounce for the head */
static Float water_drop_bounce = 0.7F;
static Float water_drop_head_bounce = 0.5F;
/* the radii of the balls at player 0's weld points and head, as shares of
 * player 0's radius */
static Float water_drop_weld_radius = 0.25F;
static Float water_drop_head_radius = 0.6F;
static Sint32 water_drop_life = 150;
/* a splash's scale for each unit of the drop's speed, and its alpha. the bounce
 * values serve the splash a bounce makes. the own values serve a drop's own
 * splash */
static Float water_drop_bounce_scale = 0.3F;
static Float water_drop_own_scale = 0.4F;
static Float water_drop_bounce_alpha = 0.15F;
static Float water_drop_own_alpha = 0.15F;
static Float water_drop_splash_growth = 0.001F;
static Float water_drop_splash_rate = 0.07F;
static Float water_drop_splash_lift = 0.002F;
/* the chances that a drop has a splash of its own, and that a bounce off the
 * floor or the head splashes */
static Float water_drop_own_chance = 0.4F;
static Float water_drop_bounce_chance = 0.1F;
static NJS_TEXANIM water_drop_texanim = {
#include "stg09/water_drop/water_drop_texanim.inc"
};
/* every splash's puff keeps a pointer to these and moves by them. each
 * splash rewrites them before it makes its puff */
static struct dust_params water_drop_splash_params = {
    0.3F,
    0.001F,
    0.07F,
    0.06F,
    0.002F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.2F, 1.0F, 1.0F, 1.0F},
};
static struct task *water_drop_task = NULL;
static struct drop *water_drop_newest = NULL;

static void water_drop_destroy(struct task *task);
static void water_drop_draw(struct task *task);
static void water_drop_run(struct task *task);

static void water_drop_start(struct task *task)
{
    task->draw = water_drop_draw;
    task->step = water_drop_run;
    task->destructor = water_drop_destroy;
}

static void water_drop_remove(struct drop *drop)
{
    if (!drop->newer) {
        water_drop_newest = drop->older;
    } else {
        drop->newer->older = drop->older;
    }
    if (drop->older) {
        drop->older->newer = drop->newer;
    }
    heap_free(drop);
}

static void water_drop_destroy(struct task *task)
{
    struct drop *drop;
    struct drop *next;

    for (drop = water_drop_newest; drop; drop = next) {
        next = drop->older;
        water_drop_remove(drop);
    }
    if (water_drop_task == task) {
        water_drop_task = NULL;
    }
}

static void water_drop_draw(struct task *task)
{
    NJS_SPRITE sprite;
    struct drop *drop = water_drop_newest;

    if (frame_step > 0) {
        return;
    }
    sprite.tlist = &obj_casino9_texlist;
    sprite.tanim = &water_drop_texanim;
    /* @hack redundant: the width and height times 1, FLDI1 and FMUL at
     * 0x0c91509c, 0x0c9150ae and 0x0c9150b6; without it, 43 of 57 words
     * differ */
    sprite.sx = water_drop_width * 1.0F;
    sprite.sy = water_drop_height * 1.0F;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    for (; drop; drop = drop->older) {
        sprite.ang = drop->ang;
        sprite.p = drop->pos;
        njDrawSprite3D(&sprite, 0,
                       NJD_SPRITE_ANGLE | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
    }
}

/* where the player's weld points 0 and 1 are in the world */
static void water_drop_weld_points(int player, NJS_POINT3 *point0,
                                   NJS_POINT3 *point1)
{
    struct task_work *work = player_work[player];
    struct collision_shape *shape = work->collision->shapes;
    struct player *record = player_get(player);
    NJS_POINT3 p1;
    NJS_POINT3 p0;
    /* @hack redundant: an NJS_POINT3 never used, whose 12 bytes of frame give
     * the image's ADD #-52,R15 (0x0c915144); without it, 10 of 112 words
     * differ */
    NJS_POINT3 unused;

    p0 = record->weld_point0;
    p1 = record->weld_point1;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &shape->center);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, NJM_DEG_ANG(180) - work->ang.y);
    njCalcPoint(NULL, &p1, &p1);
    njCalcPoint(NULL, &p0, &p0);
    njPopMatrix(1);
    *point1 = p1;
    *point0 = p0;
}

static void water_drop_splash_bounce(Float x, Float y, Float z, Float speed)
{
    NJS_VECTOR rise;
    NJS_POINT3 pos;
    Float *at;

    at = VECTOR_FLOATS(&pos);
    *at++ = x;
    *at++ = y;
    *at = z;
    at = VECTOR_FLOATS(&rise);
    *at++ = 0.0F;
    *at++ = 0.04F;
    *at = 0.0F;
    water_drop_splash_params.scale =
        water_drop_bounce_scale * _builtin_fabsf(speed);
    water_drop_splash_params.growth = water_drop_splash_growth;
    water_drop_splash_params.rate = water_drop_splash_rate;
    water_drop_splash_params.lift = water_drop_splash_lift;
    water_drop_splash_params.pos = pos;
    water_drop_splash_params.speed = rise;
    water_drop_splash_params.color.a = water_drop_bounce_alpha;
    water_smoke_create_from(&water_drop_splash_params);
}

static void water_drop_splash_own(Float x, Float y, Float z, Float speed)
{
    NJS_VECTOR rise;
    NJS_POINT3 pos;
    Float *at;

    at = VECTOR_FLOATS(&pos);
    *at++ = x;
    *at++ = y;
    *at = z;
    at = VECTOR_FLOATS(&rise);
    *at++ = 0.0F;
    *at++ = 0.03F;
    *at = 0.0F;
    water_drop_splash_params.scale =
        water_drop_own_scale * _builtin_fabsf(speed);
    water_drop_splash_params.growth = water_drop_splash_growth;
    water_drop_splash_params.rate = water_drop_splash_rate;
    water_drop_splash_params.lift = water_drop_splash_lift;
    water_drop_splash_params.pos = pos;
    water_drop_splash_params.speed = rise;
    water_drop_splash_params.color.a = water_drop_own_alpha;
    water_smoke_create_from(&water_drop_splash_params);
}

/* draw the drops, then move each. a drop is freed once its life runs out or
 * it falls below its floor. with no drop, the task kills itself */
static void water_drop_run(struct task *task)
{
    struct task_work *work = player_work[0];
    struct player *record = player_get(0);
    struct drop *drop = water_drop_newest;
    struct drop *next;
    Angle camera_y;
    /* @hack redundant: an NJS_POINT3 never used, whose 12 bytes of frame give
     * the image's ADD #-104,R15 (0x0c9153b8); without it, 2 of 442 words
     * differ */
    NJS_POINT3 unused;
    NJS_POINT3 weld0;
    NJS_POINT3 weld1;
    Float *at;
    Float head_radius2;
    Float weld_radius2;
    Float head_x;
    Float head_y;
    Float head_z;
    Float weld1_x;
    Float weld1_y;
    Float weld1_z;
    Float weld0_x;
    Float weld0_y;
    Float weld0_z;
    Float bottom;
    Float top;
    Float height;
    Float radius;
    Float x;
    Float y;
    Float z;
    Float vx;
    Float vy;
    Float vz;
    Float floor;
    Float next_y;
    Float dx;
    Float dy;
    Float dz;

    if (!drop) {
        task_kill(task);
    }
    water_drop_draw(task);
    /* @hack redundant: a test of task that never runs, which keeps task in R11
     * (MOV R4,R11 at 0x0c9153bc); without it, task takes @(16,R15), the
     * function runs 2 bytes short and 117 of 442 words differ */
    if (0) {
        if (task) {
        }
    }
    /* @hack redundant: a read of record->consts.radius left unused, which keeps
     * head_x at @(40,R15) (0x0c9153f0); without it, 115 of 442 words
     * differ */
    (void)record->consts.radius;
    /* @hack spelling: the player's pos walked as Floats through VECTOR_FLOATS,
     * for the head, as the image's post-increments read it; through the
     * members, the function runs 32 bytes long and 400 of 442 words
     * differ */
    at = VECTOR_FLOATS(&work->pos);
    /* @hack redundant: two tests of at that never run, which keep head_x and
     * next in the image's frame slots: with one, head_x takes @(60,R15);
     * without them, head_x takes @(64,R15) and next @(60,R15), the function
     * runs 2 bytes short and 108 of 442 words differ */
    if (0) {
        if (at) {
        }
        if (at) {
        }
    }
    head_radius2 = record->consts.radius * water_drop_head_radius;
    head_x = *at++;
    /* @hack spelling: height set inside head_y's sum, which gives weld0's dx,
     * dy and dz FR6, FR5 and FR4 (0x0c91566e to 0x0c91567e); set in a statement
     * of its own, they take FR5, FR4 and FR6, and 12 of 442 words differ */
    head_y = *at++ + (height = record->consts.height) - head_radius2;
    head_z = *at;
    head_radius2 = head_radius2 * head_radius2;
    bottom = work->pos.y;
    top = bottom + height;
    water_drop_weld_points(0, &weld0, &weld1);
    weld1_x = weld1.x;
    weld1_y = weld1.y;
    weld1_z = weld1.z;
    weld0_x = weld0.x;
    weld0_y = weld0.y;
    weld0_z = weld0.z;
    radius = record->consts.radius * water_drop_weld_radius;
    weld_radius2 = radius * radius;
    camera_y = camera_work->ang.y;
    /* @hack redundant: a store of 90 degrees to drop->ang that never runs,
     * which keeps head_x and next in the image's frame slots; without it,
     * head_x takes @(64,R15) and next @(60,R15), the function runs 2 bytes
     * short and 95 of 442 words differ */
    if (0) {
        drop->ang = NJM_DEG_ANG(90);
    }
    /* @hack redundant: a test of camera_y that never runs, which keeps head_x
     * and next in the image's frame slots; without it, head_x takes @(64,R15)
     * and next @(60,R15), the function runs 2 bytes short and 107 of 442 words
     * differ */
    if (0) {
        if (camera_y) {
        }
    }
    for (; drop; drop = next) {
        next = drop->older;
        floor = drop->floor;
        /* @hack spelling: the drop's pos walked the same way; through the
         * members, 276 of 442 words differ */
        at = VECTOR_FLOATS(&drop->pos);
        x = *at++;
        y = *at++;
        z = *at;
        /* @hack spelling: its speed the same way; through the members, 262 of
         * 442 words differ */
        at = VECTOR_FLOATS(&drop->speed);
        vx = *at++ * water_drop_drag;
        vy = *at++ * water_drop_drag;
        vz = *at * water_drop_drag;
        /* @hack redundant: a copy of vz to vx that never runs, which keeps both
         * in registers; without it, vx takes @(0,R15) and vz @(4,R15), next
         * takes @(60,R15), the function runs 12 bytes long and 169 of 442
         * words differ */
        if (0) {
            vx = vz;
        }
        /* @hack redundant: a test of z that never runs, which keeps head_x and
         * next in the image's frame slots; without it, head_x takes @(64,R15)
         * and next @(60,R15), the function runs 2 bytes short and 113 of 442
         * words differ */
        if (0) {
            if (z) {
            }
        }
        vy += water_drop_gravity;
        /* @hack spelling: next_y summed ahead of the test, which puts the
         * image's FADD FR15,FR4 in the delay slot at 0x0c9154c6; summed in the
         * test, 237 of 442 words differ */
        next_y = y + vy;
        if (vy < -water_drop_bounce_speed && next_y <= floor) {
            vy = -(water_drop_bounce * vy);
            x += vx;
            y = floor;
            z += vz;
            if (drop->splash > 0 &&
                rand() * RAND_SCALE <= water_drop_bounce_chance) {
                water_drop_splash_bounce(x, y, z, vy);
                drop->splash = -1;
            }
        } else {
            if (y >= bottom && y <= top) {
                if (vy < 0.0F) {
                    if (y > head_y) {
                        dx = head_x - x;
                        /* @hack redundant: a store of y to head_y that never
                         * runs, after which dy loads head_y again at
                         * 0x0c9155ac; without it, dy copies the test's FR2, the
                         * function runs 12 bytes long and 308 of 442 words
                         * differ */
                        if (0) {
                            head_y = y;
                        }
                        dy = head_y - y;
                        dz = head_z - z;
                        /* @hack redundant: a test of dx that never runs, which
                         * keeps head_x and head_z in the image's frame slots;
                         * without it, head_x takes @(60,R15) and head_z
                         * @(40,R15), and 30 of 442 words differ */
                        if (0) {
                            if (dx) {
                            }
                        }
                        if (head_radius2 >= (dx * dx) + (dy * dy) + (dz * dz)) {
                            vy = -(water_drop_head_bounce * vy);
                            if (vy > water_drop_bounce_speed &&
                                drop->splash > 0 &&
                                rand() * RAND_SCALE <=
                                    water_drop_bounce_chance) {
                                water_drop_splash_bounce(x, y, z, vy);
                                drop->splash = -1;
                            }
                        }
                    }
                    if (vy < 0.0F) {
                        if (y > weld1_y) {
                            dx = weld1_x - x;
                            /* @hack redundant: a store of y to weld1_y that
                             * never runs, after which dy loads weld1_y again at
                             * 0x0c91561c; without it, dy copies the test's FR2,
                             * the function runs 12 bytes long and 255 of 442
                             * words differ */
                            if (0) {
                                weld1_y = y;
                            }
                            dy = weld1_y - y;
                            dz = weld1_z - z;
                            if (weld_radius2 >=
                                (dx * dx) + (dy * dy) + (dz * dz)) {
                                vy = -(water_drop_bounce * vy);
                                if (vy > water_drop_bounce_speed &&
                                    drop->splash > 0) {
                                    water_drop_splash_bounce(x, y, z, vy);
                                    drop->splash = -1;
                                }
                            }
                        }
                        if (vy < 0.0F && y > weld0_y) {
                            dx = weld0_x - x;
                            /* @hack redundant: a store of y to weld0_y that
                             * never runs, which keeps weld0_y, weld0_x and
                             * weld0_z in the image's frame slots; without it,
                             * they take @(16,R15), @(48,R15) and @(52,R15), the
                             * function runs 2 bytes short and 104 of 442 words
                             * differ */
                            if (0) {
                                weld0_y = y;
                            }
                            dy = weld0_y - y;
                            dz = weld0_z - z;
                            if (weld_radius2 >=
                                (dx * dx) + (dy * dy) + (dz * dz)) {
                                vy = -(water_drop_bounce * vy);
                                if (vy > water_drop_bounce_speed &&
                                    drop->splash > 0) {
                                    water_drop_splash_bounce(x, y, z, vy);
                                    drop->splash = -1;
                                }
                            }
                        }
                    }
                }
            }
            y += vy;
            x += vx;
            z += vz;
        }
        /* @hack spelling: pos stored back the same way, as the image's address
         * steps store it; through the members, 52 of 442 words differ */
        at = VECTOR_FLOATS(&drop->pos);
        *at++ = x;
        *at++ = y;
        *at = z;
        /* @hack spelling: and speed; through the members, 50 of 442 words
         * differ */
        at = VECTOR_FLOATS(&drop->speed);
        *at++ = vx;
        *at++ = vy;
        *at = vz;
        if (--drop->life < 0 || y < floor) {
            water_drop_remove(drop);
        } else {
            drop->ang = (Angle)(atan2f(vy, (vx * njCos(camera_y)) +
                                               (vz * njSin(camera_y))) *
                                ANGLE_PER_RADIAN) +
                        NJM_DEG_ANG(90);
            if (drop->splash-- == 0) {
                water_drop_splash_own(x, y, z, vy);
            }
        }
    }
}

#define JITTER() ((rand() * RAND_SCALE) - 0.5F)

Bool water_drop_add(NJS_POINT3 *pos, NJS_VECTOR *speed, Float spread,
                    Float floor)
{
    struct drop *drop;
    NJS_VECTOR velocity;

    if (!water_drop_task) {
        water_drop_task = task_create(TASK_WORK, 3, water_drop_start);
        if (!water_drop_task) {
            return FALSE;
        }
        water_drop_task->work->pos = *pos;
    }
    drop = heap_alloc(sizeof(struct drop));
    if (drop) {
        velocity.x = speed->x + (2.0F * (JITTER() * spread));
        velocity.y = speed->y + (2.0F * (JITTER() * spread));
        velocity.z = speed->z + (2.0F * (JITTER() * spread));
        drop->floor = floor;
        drop->life = water_drop_life;
        drop->pos = *pos;
        drop->speed = velocity;
        drop->ang = 0;
        if (rand() * RAND_SCALE <= water_drop_own_chance) {
            drop->splash =
                ((Float)water_drop_life * rand() * (0.95F * RAND_SCALE)) +
                (water_drop_life * 0.05F);
        } else {
            drop->splash = water_drop_life * 2;
        }
        if (!water_drop_newest) {
            water_drop_newest = drop;
            drop->newer = NULL;
            drop->older = NULL;
        } else {
            drop->newer = NULL;
            drop->older = water_drop_newest;
            water_drop_newest->newer = drop;
            water_drop_newest = drop;
        }
        return TRUE;
    }
    return FALSE;
}
