/* the sky deck's wind, and three of the deck's objects. bands of wind sweep
 * along x in the first act. a gust blows round each task that asks for one,
 * as each cloud does. O_VENT is a vane the wind swings. O_DECAL is a scrap of
 * deck that the collapse blows away. O_SIGNAL is a lamp that pulses, and
 * fades out above the clouds */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/ground.h"
#include "collision/mesh.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg06/collapse.h"
#include "stg06/models.h"
#include "stg06/sky_deck.h"
#include "stg06/wind.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

extern NJS_OBJECT sky_deck_vent_object;
extern NJS_OBJECT decal_00_object;
extern NJS_OBJECT decal_01_object;
extern NJS_OBJECT decal_02_object;
extern NJS_OBJECT decal_03_object;
extern NJS_OBJECT decal_04_object;
extern NJS_OBJECT decal_05_object;
extern NJS_OBJECT decal_06_object;
extern NJS_OBJECT decal_07_object;
extern NJS_OBJECT decal_08_object;
extern NJS_OBJECT decal_09_object;
extern NJS_OBJECT decal_10_object;
extern NJS_OBJECT decal_11_object;
extern NJS_OBJECT decal_12_object;
extern NJS_OBJECT decal_13_object;
extern NJS_OBJECT decal_14_object;
extern NJS_OBJECT decal_15_object;
extern NJS_OBJECT decal_16_object;
extern NJS_OBJECT decal_17_object;

/* the most bands at once */
#define WIND_BANDS 10
/* a band starts at the wind's origin, and runs along x */
#define WIND_ORIGIN_X (-2000.0F)
#define WIND_ORIGIN_Z 0.0F
/* a band starts with BAND_LIFE steps left. bands start at least BAND_WAIT
 * steps apart */
#define BAND_LIFE 200
#define BAND_WAIT 20
/* the gust table's entries, and the reach of a new gust */
#define WIND_GUSTS 32
#define GUST_REACH 150.0F
/* the strongest the wind blows */
#define WIND_MOST 256.0F

/* player 0's lift is LIFT_SCALE times the wind's strength where it stands.
 * the probe is the point PROBE_LIFT above player 0. the lift scales by
 * LIFT_LOW_SHARE while the probe is less than LIFT_LOW above the ground below
 * it. it scales by LIFT_PUSHED_SHARE while player 0's accel.y is already
 * LIFT_PUSHED or more */
#define PROBE_LIFT 2.5F
#define LIFT_SCALE 0.004F
#define LIFT_LOW 100.0F
#define LIFT_LOW_SHARE 0.75F
#define LIFT_PUSHED 4.0F
#define LIFT_PUSHED_SHARE 0.4F

/* each object draws while a sphere of its radius round its position is in
 * view */
#define VENT_DRAW_RADIUS 250.0F
#define DECAL_DRAW_RADIUS 50.0F
#define SIGNAL_DRAW_RADIUS 20.0F

/* a vane swings toward its lean, a turn about z of VENT_LEAN times the
 * wind's strength */
#define VENT_LEAN 32.0F
/* a vane's turn about z runs from VENT_BACK to VENT_FORWARD. those are an
 * eighth of a turn against the wind and a quarter with it. at either end,
 * its turn speed reverses and halves */
#define VENT_BACK (-0x2000)
#define VENT_FORWARD 0x4000
#define VENT_BOUNCE (-0.5F)

/* the scrap models a decal can take */
#define DECALS 18
/* each step once the collapse reaches it, a decal rises DECAL_LIFT and
 * drifts DECAL_DRIFT along x. its turn about z falls by DECAL_TURN */
#define DECAL_LIFT 3.4F
#define DECAL_DRIFT 3.5F
#define DECAL_TURN 0x100

/* how far below the sky's point the clouds' level lies. a signal above that
 * level fades out by SIGNAL_FADE a step, and one below fades in */
#define CLOUD_LEVEL 1400.0F
#define SIGNAL_FADE 0.025F
/* a signal's sprite scale for each unit of its ang.y */
#define SIGNAL_SCALE 0.0625F

/* a band of wind, which spans x to x + width along x from the wind's origin.
 * it moves speed along x a step */
struct wind_band {
    /* the steps it has left. wind_band_move frees the band on finding this
     * below 0 */
    Sint32 life;
    Float x;
    Float speed;
    Float width;
    Float strength;
};

/* a gust, round its owner's x and z as sky_deck_wind_gust_set last left
 * them. the entry is free while owner is null */
struct wind_gust {
    struct task *owner;
    Float x;
    Float z;
    Float reach;
};

struct signal_vars {
    Uint8 unk_00[3];
    Uint8 steps; /* the steps it has run, mod 256, which time its pulse */
    Uint8 unk_04[4];
    Float alpha; /* how far it has faded in, from 0 to 1 */
};

#define SIGNAL(work) ((struct signal_vars *)&(work)->vars)
/* a vane's turn speed about z */
#define VENT_SPEED(work) ((work)->scl.z)
/* VENT_PUSH is the turn speed a vane gains a step while short of its lean.
 * VENT_PULL is the speed it loses otherwise. both come from its set entry */
#define VENT_PUSH(work) ((work)->scl.y)
#define VENT_PULL(work) ((work)->scl.x)

enum wind_mode { WIND_INIT, WIND_RUN };

static void sky_deck_vent_draw(struct task *task);
static void sky_deck_vent_destroy(struct task *task);
static void decal_draw(struct task *task);
static void decal_destroy(struct task *task);
static void signal_draw(struct task *task);
static void signal_destroy(struct task *task);
static void wind_band_start(int band);
static Bool wind_band_move(int band);

/* the scrap models, which a decal's scl.z picks */
static NJS_OBJECT *decal_objects[DECALS] = {
    &decal_00_object, &decal_01_object, &decal_02_object, &decal_03_object,
    &decal_04_object, &decal_05_object, &decal_06_object, &decal_07_object,
    &decal_08_object, &decal_09_object, &decal_10_object, &decal_11_object,
    &decal_12_object, &decal_13_object, &decal_14_object, &decal_15_object,
    &decal_16_object, &decal_17_object,
};

static NJS_TEXLIST signal_texlist = {
#include "stg06/wind/signal_texlist.inc"
};
static NJS_TEXANIM signal_frame = {
#include "stg06/wind/signal_frame.inc"
};
static NJS_SPRITE signal_sprite = {
#include "stg06/wind/signal_sprite.inc"
};

static const struct angles no_slope = {0, 0, 0};

static struct wind_band wind_bands[WIND_BANDS];
static struct wind_gust wind_gusts[WIND_GUSTS];

void sky_deck_wind_step(struct task *task)
{
    struct task_work *work = task->work;
    int i;
    int free;

    switch (work->mode) {
    case WIND_INIT:
        for (i = 0; i < WIND_BANDS; i++) {
            wind_bands[i].life = 0;
        }
        for (i = 0; i < WIND_GUSTS; i++) {
            wind_gusts[i].owner = NULL;
        }
        work->timer = 0;
        work->mode = WIND_RUN;
        break;
    case WIND_RUN:
        free = -1;
        for (i = 0; i < WIND_BANDS; i++) {
            if (wind_band_move(i)) {
                free = i;
            }
        }
        if (work->timer == 0) {
            if (free == -1) {
                work->timer = 1;
            } else {
                wind_band_start(free);
                work->timer = BAND_WAIT;
            }
        }
        work->timer--;
        break;
    }
}

static void wind_band_start(int band)
{
    wind_bands[band].life = BAND_LIFE;
    wind_bands[band].x = 0.0F;
    wind_bands[band].strength = RANDOM(20.0F, 40.0F);
    wind_bands[band].width = RANDOM(50.0F, 350.0F);
    wind_bands[band].speed = RANDOM(15.0F, 45.0F);
}

/* step the band. a call that finds its life below 0 sets it to 0. it then
 * returns TRUE: the band is free. any other call takes a step off the life,
 * moves the band and returns FALSE. until a start renews it, a band whose
 * life has run out moves on every other call. its life is 0 and -1 by
 * turns */
static Bool wind_band_move(int band)
{
    if (wind_bands[band].life-- < 0) {
        wind_bands[band].life = 0;
        return TRUE;
    }
    wind_bands[band].x += wind_bands[band].speed;
    return FALSE;
}

void sky_deck_wind_gust_set(struct task *task)
{
    int i;
    Bool found = FALSE;
    struct task_work *work = task->work;

    /* @bug logic: the loop takes the first entry that is task's or free
     * (the CMP/EQ at 0x0c905be0, then the TST at 0x0c905bee). a free entry
     * ahead of task's gust becomes a second gust of task's.
     * sky_deck_wind_gust_remove frees only that one, so the older gust
     * outlives task. its owner then dangles, and a later task at the same
     * address takes the gust as its own */
    for (i = 0; i < WIND_GUSTS; i++) {
        if (wind_gusts[i].owner == task) {
            found = TRUE;
            break;
        } else if (wind_gusts[i].owner == NULL) {
            wind_gusts[i].owner = task;
            wind_gusts[i].reach = GUST_REACH;
            found = TRUE;
            break;
        }
    }
    if (found) {
        wind_gusts[i].x = work->pos.x;
        wind_gusts[i].z = work->pos.z;
    }
}

void sky_deck_wind_gust_remove(struct task *task)
{
    int i;

    for (i = 0; i < WIND_GUSTS; i++) {
        if (wind_gusts[i].owner == task) {
            wind_gusts[i].owner = NULL;
            break;
        }
    }
}

/* x and z are taken from the wind's origin. each band whose life isn't 0
 * adds a hump across its width. the hump is 1 - cos over one turn, and peaks
 * at twice the band's strength. a band whose life has run out still moves on
 * every other call. it adds its hump after each such move. each gust adds a
 * dome, half its reach high at its center. a gust holds its owner's x and z
 * in the world, not from the origin. so each gust blows round the point
 * WIND_ORIGIN_X from its owner along x. the sum stops at WIND_MOST */
Float sky_deck_wind_strength(Float x, Float z)
{
    int i;
    Float strength;
    Float blow;
    Float left;
    Float width;
    Float phase;
    Float gust_x;
    Float gust_z;
    Float reach;
    Float dx;
    Float dz;

    strength = 0.0F;
    x -= WIND_ORIGIN_X;
    z -= WIND_ORIGIN_Z;
    for (i = 0; i < WIND_BANDS; i++) {
        if (wind_bands[i].life) {
            left = wind_bands[i].x;
            width = wind_bands[i].width;
            if (x > left && x < left + width) {
                phase = (x - left) * 65536.0F / width;
                blow = (njSin((int)phase - 0x4000) + 1.0F) *
                       wind_bands[i].strength;
                strength += blow;
            }
        }
    }
    for (i = 0; i < WIND_GUSTS; i++) {
        if (wind_gusts[i].owner) {
            gust_x = wind_gusts[i].x;
            gust_z = wind_gusts[i].z;
            reach = wind_gusts[i].reach;
            reach *= reach;
            if (vector_length_squared(x - gust_x, 0.0F, z - gust_z) < reach) {
                dx = x - gust_x;
                dz = z - gust_z;
                blow = njSqrt(reach - (dx * dx) - (dz * dz)) * 0.5F;
                strength += blow;
            }
        }
    }
    if (strength > WIND_MOST) {
        strength = WIND_MOST;
    }
    return strength;
}

/* the vane swings toward its lean. while short of it, its turn speed gains
 * VENT_PUSH a step. otherwise it loses VENT_PULL */
void sky_deck_vent_step(struct task *task)
{
    struct task_work *work = task->work;
    Float wind;
    int target;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case WIND_INIT:
        work->mode = WIND_RUN;
        VENT_SPEED(work) = 0.0F;
        task->draw = sky_deck_vent_draw;
        task->destructor = sky_deck_vent_destroy;
        break;
    case WIND_RUN:
        wind = sky_deck_wind_strength(work->pos.x, work->pos.z);
        target = (int)(wind * VENT_LEAN);
        if (work->ang.z < target) {
            VENT_SPEED(work) += VENT_PUSH(work);
        } else {
            VENT_SPEED(work) -= VENT_PULL(work);
        }
        work->ang.z += (int)VENT_SPEED(work);
        if (work->ang.z > VENT_FORWARD) {
            work->ang.z = VENT_FORWARD;
            VENT_SPEED(work) *= VENT_BOUNCE;
        }
        if (work->ang.z < VENT_BACK) {
            work->ang.z = VENT_BACK;
            VENT_SPEED(work) *= VENT_BOUNCE;
        }
        break;
    }
    sky_deck_vent_draw(task);
}

/* the set entry's ang.x is the vane's turn about z at rest, which its swing
 * adds to */
static void sky_deck_vent_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, VENT_DRAW_RADIUS)) {
        njSetTexture(&obj_skydeck_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z + work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&sky_deck_vent_object);
        njPopMatrix(1);
    }
}

static void sky_deck_vent_destroy(struct task *task)
{
}

void sky_deck_wind_lift_player(void)
{
    Float x = player_work[0]->pos.x;
    Float y = player_work[0]->pos.y + PROBE_LIFT;
    Float z = player_work[0]->pos.z;
    struct angles slope;
    Bool low = FALSE;
    Float ground;
    Float wind;

    slope = no_slope;
    ground = ground_below(x, y, z, &slope);
    if (y - ground < LIFT_LOW) {
        low = TRUE;
    }
    wind = sky_deck_wind_strength(x, z) * LIFT_SCALE;
    if (low) {
        wind *= LIFT_LOW_SHARE;
    }
    if (player_motion[0]->accel.y >= LIFT_PUSHED) {
        wind *= LIFT_PUSHED_SHARE;
    }
    player_motion[0]->accel.y += wind;
    if (player_motion[0]->accel.y > wind) {
        player_motion[0]->accel.y = wind;
    }
}

/* once the collapse reaches a decal, it blows away. if it leaves range
 * after that, its placement makes no more */
void decal_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        if (work->mode == WIND_RUN) {
            if (collapse_reached(work->pos.x, work->pos.y, work->pos.z)) {
                placement_kill(task);
            }
        }
        return;
    }
    switch (work->mode) {
    case WIND_INIT:
        work->mode = WIND_RUN;
        task->draw = decal_draw;
        task->destructor = decal_destroy;
        break;
    case WIND_RUN:
        if (collapse_reached(work->pos.x, work->pos.y, work->pos.z)) {
            work->pos.y += DECAL_LIFT;
            work->pos.x += DECAL_DRIFT;
            work->ang.z -= DECAL_TURN;
        }
        break;
    }
    decal_draw(task);
}

/* draw the scrap scl.z picks, clamped to the table, scaled by scl.y. the
 * light is divided by the scale, so the scrap shades as it would unscaled */
static void decal_draw(struct task *task)
{
    struct task_work *work = task->work;
    int index;
    NJS_OBJECT *object;
    Float scale;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                     DECAL_DRAW_RADIUS)) {
        index = (int)work->scl.z;
        if (index < 0) {
            index = 0;
        }
        if (index >= DECALS) {
            index = DECALS - 1;
        }
        object = decal_objects[index];
        njSetTexture(&obj_skydeck_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        scale = work->scl.y;
        njScale(NULL, scale, scale, scale);
        draw_divide_light(work->scl.y);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        draw_tree(object);
        draw_reset_light();
        njPopMatrix(1);
    }
}

static void decal_destroy(struct task *task)
{
}

void signal_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case WIND_INIT:
        work->mode = WIND_RUN;
        task->draw = signal_draw;
        task->destructor = signal_destroy;
        SIGNAL(work)->alpha = 0.0F;
        break;
    case WIND_RUN:
        break;
    }
    if (work->pos.y > sky_deck_sky_pos.y - CLOUD_LEVEL) {
        SIGNAL(work)->alpha -= SIGNAL_FADE;
        if (SIGNAL(work)->alpha < 0.0F) {
            SIGNAL(work)->alpha = 0.0F;
        }
    } else {
        SIGNAL(work)->alpha += SIGNAL_FADE;
        if (SIGNAL(work)->alpha > 1.0F) {
            SIGNAL(work)->alpha = 1.0F;
        }
    }
    SIGNAL(work)->steps++;
    signal_draw(task);
}

/* draw the signal as a sprite added onto what is drawn, with no fog. its set
 * entry gives its color in scl and its pulse's speed in ang.x. its sprite's
 * scale is ang.y times SIGNAL_SCALE. its alpha pulses between 0 and 1, times
 * its fade outside the set editor */
static void signal_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float size;
    Float alpha;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                     SIGNAL_DRAW_RADIUS)) {
        size = work->ang.y * SIGNAL_SCALE;
        alpha = (njSin(SIGNAL(work)->steps * work->ang.x) * 0.5F) + 0.5F;
        if (!placement_editing(task)) {
            alpha *= SIGNAL(work)->alpha;
        }
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_INVSRCCOLOR);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        draw_set_material(alpha, work->scl.x, work->scl.y, work->scl.z);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njFogDisable();
        signal_sprite.sx = size;
        signal_sprite.sy = size;
        njDrawSprite3D(&signal_sprite, 0,
                       NJD_SPRITE_COLOR | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
        njFogEnable();
        njPopMatrix(1);
    }
}

static void signal_destroy(struct task *task)
{
}
