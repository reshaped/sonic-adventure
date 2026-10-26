/* the sphere one of E-102's shots leaves where it ends, once his laser has
 * pierced its target. it swells and fades */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/model.h"
#include "player/e102_sphere.h"
#include "player/textures.h" /* NOLINT(misc-include-cleaner) */
#include "system/task.h"
#include "system/util.h"

/* its timer counts down from SPHERE_STEPS. with SPHERE_FADE steps left,
 * its shape turns off, and it fades after that */
#define SPHERE_STEPS 60
#define SPHERE_FADE 30

/* the mesh: BANDS bands from pole to pole. each is a strip of PAIRS pairs
 * of points round the sphere, and its first pair again to close it */
#define BANDS 8
#define PAIRS 8
/* the turn from one band's top edge to the next band's, and from one pair to
 * the next */
#define BAND_ANGLE (0x8000 / BANDS)
#define PAIR_ANGLE (0x10000 / PAIRS)
#define STRIP_POINTS (2 + (2 * PAIRS))

enum sphere_mode { SPHERE_INIT, SPHERE_GROW, SPHERE_KILL };

/* in work->vars. work->scl.x is the sphere's size, and work->scl.z its
 * opacity, which sets how open its bands are too */
struct sphere_vars {
    Uint8 unk_00[4];
    Sint32 scroll; /* how far the texture has scrolled in v */
    Float growth; /* added to its size each step */
};

#define VARS(work) ((struct sphere_vars *)&(work)->vars)

static void e102_sphere_build(void);
static void e102_sphere_scroll(int scroll);
static void e102_sphere_close(Float open);
static void e102_sphere_draw(struct task *task);

/* each pair's cosine and sine around the sphere */
static Float e102_sphere_ring[PAIRS * 2];
/* each band's pairs, the first point of each on the band's top edge */
static NJS_POINT3 e102_sphere_points[BANDS * PAIRS * 2];
/* the model's normals, which nothing sets: all zero */
static NJS_VECTOR e102_sphere_normals[BANDS * PAIRS * 2];
/* one strip per band: its count of points, then their indexes */
static Sint16 e102_sphere_strips[BANDS * (1 + STRIP_POINTS)];
static Sint16 e102_sphere_uvs[BANDS * STRIP_POINTS * 2];

static Float e102_sphere_size_max = 20.0F;

static NJS_MESHSET e102_sphere_meshset = {
#include "player/e102_sphere/e102_sphere_meshset.inc"
};
static NJS_MATERIAL e102_sphere_material = {
#include "player/e102_sphere/e102_sphere_material.inc"
};
static NJS_MODEL e102_sphere_model = {
#include "player/e102_sphere/e102_sphere_model.inc"
};
static NJS_TEXLIST e102_sphere_texlist = {
#include "player/e102_sphere/e102_sphere_texlist.inc"
};

static Bool e102_sphere_built = FALSE;

/* a ball the step keeps at 0.8 of the sphere's size */
/* clang-format off */
static struct collision_shape e102_sphere_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_UNTURNED,
    ATTACK(2) | SHAPE_HURTS_ENEMIES, PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F},
    0.0F, 0.0F, 0.0F, {0}, 0, NULL};
/* clang-format on */

/* make the ring, the points and the strips, and lay the texture
 * unscrolled, on the first call only */
static void e102_sphere_build(void)
{
    Float *ring;
    Float *point;
    Sint16 *strip;
    int i;
    int j;
    int index;
    int first;
    Float sin0;
    Float cos0;
    Float sin1;
    Float cos1;
    Float x;
    Float z;

    if (e102_sphere_built) {
        return;
    }
    e102_sphere_built = TRUE;
    ring = e102_sphere_ring;
    for (i = 0; i < PAIRS * PAIR_ANGLE; i += PAIR_ANGLE) {
        *ring++ = njCos(i);
        *ring++ = njSin(i);
    }
    point = (Float *)e102_sphere_points;
    for (i = 0; i < BANDS * BAND_ANGLE; i += BAND_ANGLE) {
        ring = e102_sphere_ring;
        sin0 = njSin(i);
        cos0 = njCos(i);
        sin1 = njSin(i + BAND_ANGLE);
        cos1 = njCos(i + BAND_ANGLE);
        for (j = 0; j < PAIRS * PAIR_ANGLE; j += PAIR_ANGLE) {
            x = *ring++;
            z = *ring++;
            *point++ = sin0 * x;
            *point++ = cos0;
            *point++ = sin0 * z;
            *point++ = sin1 * x;
            *point++ = cos1;
            *point++ = sin1 * z;
        }
    }
    strip = e102_sphere_strips;
    index = 0;
    for (i = 0; i < BANDS; i++) {
        first = index;
        *strip++ = STRIP_POINTS;
        for (j = 0; j < PAIRS; j++) {
            *strip++ = index++;
            *strip++ = index++;
        }
        *strip++ = first++;
        *strip++ = first;
    }
    e102_sphere_scroll(0);
}

/* lay the texture across each band, scrolled by scroll in v */
static void e102_sphere_scroll(int scroll)
{
    Sint16 *uv;
    int i;
    int j;
    int tu;
    int tv;

    uv = e102_sphere_uvs;
    tv = scroll;
    for (i = 0; i < BANDS; i++) {
        tu = 0;
        for (j = 0; j < PAIRS + 1; j++) {
            *uv++ = tu;
            *uv++ = tv;
            *uv++ = tu;
            *uv++ = tv + 63;
            tu += 64;
            tv += 64;
        }
    }
}

/* move each band's top edge to open of its height above its bottom edge.
 * at 1 the sphere is whole, and at 0 each band closes up */
static void e102_sphere_close(Float open)
{
    Float *ring;
    Float *point;
    int i;
    int j;
    Float sin0;
    Float cos0;
    Float sin1;
    Float cos1;
    Float x;
    Float z;

    point = (Float *)e102_sphere_points;
    for (i = 0; i < BANDS * BAND_ANGLE; i += BAND_ANGLE) {
        ring = e102_sphere_ring;
        sin0 = njSin(i);
        cos0 = njCos(i);
        sin1 = njSin(i + BAND_ANGLE);
        cos1 = njCos(i + BAND_ANGLE);
        sin0 = sin1 + ((sin0 - sin1) * open);
        cos0 = cos1 + ((cos0 - cos1) * open);
        for (j = 0; j < PAIRS * PAIR_ANGLE; j += PAIR_ANGLE) {
            x = *ring++;
            z = *ring++;
            *point++ = sin0 * x;
            *point++ = cos0;
            *point++ = sin0 * z;
            point += 3;
        }
    }
}

/* the sphere swells from 2 to e102_sphere_size_max, slowing as it grows.
 * its shape turns off with SPHERE_FADE steps left on its timer, and it
 * fades after that */
void e102_sphere_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SPHERE_INIT:
        TASK_TIMER(work) = SPHERE_STEPS;
        VARS(work)->scroll = 0;
        VARS(work)->growth = 5.0F;
        work->scl.x = 2.0F;
        work->scl.z = 1.0F;
        e102_sphere_build();
        collision_init(task, &e102_sphere_shape, COLLISION_ATTACK, 1, 1);
        task->draw = e102_sphere_draw;
        work->mode = SPHERE_GROW;
        break;
    case SPHERE_GROW:
        TASK_TIMER(work)--;
        if (TASK_TIMER(work) == 0) {
            work->mode = SPHERE_KILL;
            break;
        }
        if (TASK_TIMER(work) == SPHERE_FADE) {
            work->collision->shapes->flags |= SHAPE_OFF;
        } else if (TASK_TIMER(work) < SPHERE_FADE) {
            work->scl.z -= 0.033F;
        }
        VARS(work)->growth += 0.1F;
        VARS(work)->growth *= 0.8F;
        work->scl.x = work->scl.x + VARS(work)->growth < e102_sphere_size_max
                          ? work->scl.x + VARS(work)->growth
                          : e102_sphere_size_max;
        work->collision->shapes->radius = work->scl.x * 0.8F;
        VARS(work)->scroll += 10;
        break;
    case SPHERE_KILL:
        task_kill(task);
        return;
    }
    e102_sphere_draw(task);
    collision_register(work);
}

static void e102_sphere_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_MODEL *model;
    Float size;
    Float alpha;

    njSetTexture(&e102_sphere_texlist);
    e102_sphere_close(work->scl.z);
    e102_sphere_scroll(VARS(work)->scroll);
    njPushMatrix(NULL);
    model = &e102_sphere_model;
    size = work->scl.x;
    alpha = work->scl.z;
    model->mats->diffuse.argb.a = alpha * 255.0F;
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, size, size, size);
    draw_model(model);
    njPopMatrix(1);
}
