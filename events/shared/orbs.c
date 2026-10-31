/* an event's orbs: pieces of the Master Emerald, each with a glow. they
 * ring a glow over player 0, then fly to places above the gathering point.
 * they spiral down toward it, and a large glow there brightens and dims */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "hud/hud.h"
#include "object/models.h"
#include "player/player.h"
#include "shared/orbs.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the orbs' work modes, in the order they run */
#define ORBS_GLOW 0 /* the glow brightens over player 0 */
#define ORBS_RING 1 /* the orbs ring it as it dims */
#define ORBS_FLY 2 /* they fly to places above the gathering point, growing */
#define ORBS_SPIRAL 3 /* they spiral down toward the gathering point */
#define ORBS_FLASH 4 /* the large glow there brightens and dims */
#define ORBS_DONE 5 /* nothing moves or draws */

/* the steps each mode runs for */
#define GLOW_STEPS 60
#define RING_STEPS 60
#define FLY_STEPS 90
#define SPIRAL_STEPS 180
#define FLASH_STEPS 90

/* where the glow over player 0, and the middle of the orbs' ring, sit from
 * player 0's place */
#define OFFSET_X 0.0F
#define OFFSET_Y 4.0F
#define OFFSET_Z (-4.0F)
/* the gathering point. the orbs fly to places above it and spiral down
 * toward it. the large glow shows there */
#define GATHER_X (-2.9F)
#define GATHER_Y (-162.18F)
#define GATHER_Z 2149.59F
/* the ring over player 0 is the orbs' places above the gathering point,
 * scaled by this */
#define RING_SCALE 0.03F
/* how far each glow sits from what it lights, toward the camera, and its
 * scale. the large glow's are FLASH_TOWARD_CAMERA and FLASH_SCALE */
#define GLOW_TOWARD_CAMERA 3.0F
#define GLOW_SCALE 0.7F
#define FLASH_TOWARD_CAMERA 28.8F
#define FLASH_SCALE 6.5F
/* the share of an orb's radius the spiral takes away by its end */
#define SPIRAL_SHRINK 0.9F
/* a 64th of a turn. each step of the spiral turns an orb by this times the
 * share of the spiral gone */
#define SPIRAL_TURN 0x400
/* half a turn. the large glow's brightness is 1 plus the sine of an angle,
 * halved. the angle runs over this as the flash plays */
#define FLASH_ARC NJM_DEG_ANG(180)
/* the glows' color, scaled by their brightness */
#define GLOW_RED 0.0F
#define GLOW_GREEN 2.0F
#define GLOW_BLUE 0.6F

/* an orb. it flies to radius out from the gathering point, height above it
 * and ang around it. the ring over player 0 scales radius and height by
 * RING_SCALE */
struct event_orb {
    NJS_POINT3 pos;
    Float radius;
    Float height;
    Angle ang;
};

/* the orbs' work vars. the work's tick holds the orbs' count, and its timer
 * the mode's steps so far. scl.x holds the orbs' scale */
struct event_orbs_vars {
    Uint8 unk_00[4];
    /* of the glow over player 0 or of the large one, from 0 to 1 */
    Float brightness;
    struct event_orb *orbs;
};

#define VARS(work) ((struct event_orbs_vars *)&(work)->vars)

static NJS_MATERIAL event_orbs_glow_materials[1] = {
#include "shared/orbs/event_orbs_glow_materials.inc"
};
static Sint16 event_orbs_glow_meshes[4] = {
#include "shared/orbs/event_orbs_glow_meshes.inc"
};
static NJS_COLOR event_orbs_glow_uvs[4] = {
#include "shared/orbs/event_orbs_glow_uvs.inc"
};
/* @bug memory: the glow's model has 4 points but this one normal.
 * njSimpleDrawModel reads a normal for each point, so its last 3 reads run
 * past this array, into the meshset and the first point (the reads at
 * 0x8c62bba8 or 0x8c62bd64 in 1ST_READ.BIN) */
#ifdef GCC
/* repairs the read the tag names: GCC's glow holds a normal for each of its
 * 4 points, its one normal four times. its material ignores the light, so
 * no normal changes how it draws */
static NJS_VECTOR event_orbs_glow_normals[4] = {
#include "shared/orbs/event_orbs_glow_normals.inc"
    {0.0F, 1.0F, 0.0F},
    {0.0F, 1.0F, 0.0F},
    {0.0F, 1.0F, 0.0F},
};
#else
static NJS_VECTOR event_orbs_glow_normals[1] = {
#include "shared/orbs/event_orbs_glow_normals.inc"
};
#endif
static NJS_MESHSET event_orbs_glow_meshsets[1] = {
#include "shared/orbs/event_orbs_glow_meshsets.inc"
};
static NJS_POINT3 event_orbs_glow_points[4] = {
#include "shared/orbs/event_orbs_glow_points.inc"
};
static NJS_MODEL event_orbs_glow_model = {
#include "shared/orbs/event_orbs_glow_model.inc"
};
static NJS_OBJECT event_orbs_glow_object = {
#include "shared/orbs/event_orbs_glow_object.inc"
};

static struct task *event_orbs_task = NULL;

/* turn Ninja's matrix to face the camera, which lies x, y and z from its
 * origin. it turns about y by atan2f(x, z). it then turns about x by
 * atan2f(-y, sqrt(x * x + z * z)). each turn of 0 is skipped. the macro negates
 * y, and leaves x * x + z * z in squared */
/* @hack spelling: y's negation and squared's assignment inside the pitch test's
 * arguments, which negates y after the test's njSqrt call (0x0cb81a38,
 * 0x0cb81bca, 0x0cb81ce2 in EV0091); written out at each call, y is negated
 * before it, 391 of the draw's 512 words differ and it ends 8 bytes late */
/* @hack redundant: the unary + on x, which loads x into FR0 and copies it to
 * FR2 for the FMAC (0x0cb81a22, 0x0cb81a26 in EV0091); without it, the two
 * trade and 4 of 512 words differ */
#define FACE_CAMERA(x, y, z, squared)                                          \
    do {                                                                       \
        if ((Angle)(atan2f((x), (z)) * ANGLE_PER_RADIAN)) {                    \
            njRotateY(NULL, (Angle)(atan2f((x), (z)) * ANGLE_PER_RADIAN));     \
        }                                                                      \
        if ((Angle)(atan2f((y) = -(y),                                         \
                           njSqrt((squared) = (+(x) * (x)) + ((z) * (z)))) *   \
                    ANGLE_PER_RADIAN)) {                                       \
            njRotateX(NULL, (Angle)(atan2f((y), njSqrt(squared)) *             \
                                    ANGLE_PER_RADIAN));                        \
        }                                                                      \
    } while (0)

static void event_orbs_destroy(struct task *task)
{
    heap_free(VARS(task->work)->orbs);
}

static void event_orbs_draw(struct task *task)
{
    struct task_work *work = task->work;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int mode = work->mode;

    switch (mode) {
    case ORBS_RING:
    case ORBS_FLY:
    case ORBS_SPIRAL: {
        struct event_orb *orb = VARS(work)->orbs;
        int remaining;
        Float intensity;

        draw_set_palette(DRAW_PLAYER_PALETTE);
        draw_control_save();
        njControl3D(NJD_CONTROL_3D_CONSTANT_MATERIAL);
        intensity = (njSin(play_time_get() << 11) + 1.0F) * 0.5F;
        draw_set_material(1.0F, intensity * GLOW_RED, intensity * GLOW_GREEN,
                          intensity * GLOW_BLUE);
        for (remaining = work->tick; remaining != 0; remaining--) {
            Float x;
            Float y;
            Float z;
            Float squared;

            njPushMatrix(NULL);
            x = camera_work->pos.x - orb->pos.x;
            y = camera_work->pos.y - orb->pos.y;
            z = camera_work->pos.z - orb->pos.z;
            njTranslateV(NULL, &orb->pos);
            njScale(NULL, work->scl.x, work->scl.x, work->scl.x);
            texture_set_regular();
            draw_tree(&emerald_piece_object);
            /* @hack redundant: a never-run store to the brightness, which
             * keeps the glows' y, squared and -y at sp+32, sp+44 and sp+48
             * over player 0 (0x0cb81b72, 0x0cb81bb6, 0x0cb81bc8) and at
             * sp+52, sp+36 and sp+40 at the gathering point (0x0cb81c82,
             * 0x0cb81cd0, 0x0cb81ce0 in EV0091); without it, 12 of 512 words
             * differ */
            if (0) {
                VARS(work)->brightness = 3.0F;
            }
            FACE_CAMERA(x, y, z, squared);
            njTranslate(NULL, 0.0F, 0.0F, GLOW_TOWARD_CAMERA);
            njScale(NULL, GLOW_SCALE, GLOW_SCALE, GLOW_SCALE);
            texture_set_stage();
            njSimpleDrawModel(&event_orbs_glow_model);
            njPopMatrix(1);
            orb++;
        }
        draw_control_restore();
        draw_set_palette(0);
    } break;
    }
    switch (mode) {
    case ORBS_GLOW:
    case ORBS_RING: {
        Float x;
        Float y;
        Float z;
        Float squared;

        draw_set_material(1.0F, VARS(work)->brightness * GLOW_RED,
                          VARS(work)->brightness * GLOW_GREEN,
                          VARS(work)->brightness * GLOW_BLUE);
        njPushMatrix(NULL);
        x = camera_work->pos.x - work->pos.x;
        y = camera_work->pos.y - work->pos.y;
        z = camera_work->pos.z - work->pos.z;
        njTranslateV(NULL, &work->pos);
        FACE_CAMERA(x, y, z, squared);
        njTranslate(NULL, 0.0F, 0.0F, GLOW_TOWARD_CAMERA);
        njScale(NULL, GLOW_SCALE, GLOW_SCALE, GLOW_SCALE);
        texture_set_stage();
        draw_control_save();
        njControl3D(NJD_CONTROL_3D_CONSTANT_MATERIAL);
        njSimpleDrawModel(&event_orbs_glow_model);
        draw_control_restore();
        njPopMatrix(1);
    } break;
    case ORBS_FLASH: {
        Float intensity = VARS(work)->brightness;
        Float x;
        Float y;
        Float z;
        Float squared;

        draw_set_material(1.0F, intensity * GLOW_RED, intensity * GLOW_GREEN,
                          intensity * GLOW_BLUE);
        njPushMatrix(NULL);
        x = camera_work->pos.x - GATHER_X;
        y = camera_work->pos.y - GATHER_Y;
        z = camera_work->pos.z - GATHER_Z;
        njTranslate(NULL, GATHER_X, GATHER_Y, GATHER_Z);
        FACE_CAMERA(x, y, z, squared);
        njTranslate(NULL, 0.0F, 0.0F, FLASH_TOWARD_CAMERA);
        njScale(NULL, FLASH_SCALE, FLASH_SCALE, FLASH_SCALE);
        texture_set_stage();
        /* @hack redundant: a never-run call of texture_set_stage, which
         * weighs its address so that it, not njTranslate's, takes R8 for
         * the whole draw, and njSqrt's R11 (0x0cb81932, 0x0cb81934 in
         * EV0091); without it, 331 of 512 words differ and the draw ends 30
         * bytes short */
        if (0) {
            texture_set_stage();
        }
        draw_control_save();
        njControl3D(NJD_CONTROL_3D_CONSTANT_MATERIAL);
        njSimpleDrawModel(&event_orbs_glow_model);
        draw_control_restore();
        njPopMatrix(1);
    } break;
    }
}

/* @hack spelling: the step's mode read through an inline function, which keeps
 * 4.0 in FR12 and 0 in FR13 for the whole step (0x0cb81da8, 0x0cb81dba in
 * EV0091); read directly, the two trade and 70 of 427 words differ */
#pragma inline(event_orbs_mode)
static Sint8 event_orbs_mode(struct task_work *work)
{
    return work->mode;
}

static void event_orbs_step(struct task *task)
{
    struct task_work *work = task->work;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int mode = event_orbs_mode(work);

    switch (mode) {
    case ORBS_GLOW: {
        struct task_work *player = player_work[0];

        work->pos.x = player->pos.x + OFFSET_X;
        work->pos.y = player->pos.y + OFFSET_Y;
        work->pos.z = player->pos.z + OFFSET_Z;
        VARS(work)->brightness = TASK_TIMER(work) * (1.0F / GLOW_STEPS);
        if (++TASK_TIMER(work) >= GLOW_STEPS) {
            work->mode = ORBS_RING;
            work->timer = 0;
        }
    } break;
    case ORBS_RING: {
        struct task_work *player = player_work[0];
        struct event_orb *orb = VARS(work)->orbs;
        int remaining = work->tick;

        /* @hack spelling: a test, then a do-while, which keeps 0 in FR13
         * through the step (FLDI0 at 0x0cb81dba in EV0091); as a for loop, -4.0
         * takes FR13, 378 of 427 words differ, and through the pool the two
         * share, 37 of event_orbs_draw's 512 */
        if (remaining) {
            do {
                Float x = njSin(orb->ang) * orb->radius;
                Float z = njCos(orb->ang) * orb->radius;

                orb->pos.x = player->pos.x + (x * RING_SCALE) + OFFSET_X;
                orb->pos.y =
                    player->pos.y + (orb->height * RING_SCALE) + OFFSET_Y;
                orb->pos.z = player->pos.z + (z * RING_SCALE) + OFFSET_Z;
                orb++;
            } while (--remaining);
        }
        work->pos.x = player->pos.x + OFFSET_X;
        work->pos.y = player->pos.y + OFFSET_Y;
        work->pos.z = player->pos.z + OFFSET_Z;
        VARS(work)->brightness =
            1.0F - (TASK_TIMER(work) * (1.0F / RING_STEPS));
        work->scl.x = 0.5F;
        if (++TASK_TIMER(work) >= RING_STEPS) {
            work->mode = ORBS_FLY;
            work->timer = 0;
        }
    } break;
    case ORBS_FLY: {
        struct task_work *player = player_work[0];
        struct event_orb *orb = VARS(work)->orbs;
        Float progress = TASK_TIMER(work) * (1.0F / FLY_STEPS);
        Float eased = 1.0F - ((1.0F - progress) * (1.0F - progress));
        int remaining;

        for (remaining = work->tick; remaining != 0; remaining--) {
            Float x = njSin(orb->ang) * orb->radius;
            Float z = njCos(orb->ang) * orb->radius;
            Float px = player->pos.x + (x * RING_SCALE) + OFFSET_X;
            Float y = orb->height;
            Float py = player->pos.y + (y * RING_SCALE) + OFFSET_Y;
            Float pz = player->pos.z + (z * RING_SCALE) + OFFSET_Z;
            Float tx = x + GATHER_X;
            Float ty = y + GATHER_Y;
            Float tz = z + GATHER_Z;

            orb->pos.x = px + ((tx - px) * eased);
            orb->pos.y = py + ((ty - py) * eased);
            orb->pos.z = pz + ((tz - pz) * eased);
            orb++;
        }
        work->scl.x = 0.5F + (eased * 0.5F);
        if (++TASK_TIMER(work) >= FLY_STEPS) {
            work->mode = ORBS_SPIRAL;
            work->timer = 0;
        }
    } break;
    case ORBS_SPIRAL: {
        /* @hack spelling: work through a copy of its pointer, which keeps
         * the angle in FR12 and 0.9 in FR13 (0x0cb8207c, 0x0cb82056 in
         * EV0091); through work, the two trade and 4 of 427 words differ */
        struct task_work *copy = work;
        struct event_orb *orb = VARS(copy)->orbs;
        int remaining;

        for (remaining = copy->tick; remaining != 0; remaining--) {
            Uint16 timer = TASK_TIMER(copy);

            /* @hack redundant: an if (1) with empty arms, whose jump SHC
             * deletes once it has picked a scratch register, which gives
             * _divls's address R3 and the angle's load R2 (0x0cb8205a,
             * 0x0cb82062 in EV0091); without it, the two trade and 6 of 427
             * words differ */
            if (1) {
            } else {
            }
            {
                /* @hack type: the angle converted to Float through int,
                 * which changes no value but converts it into FR3 and
                 * copies it to FR12 (0x0cb82076, 0x0cb8207c in EV0091);
                 * converted from Uint16 straight, it converts into FR12, 133
                 * of 427 words differ and the step ends 4 bytes short */
                Float angle = (int)(Uint16)(orb->ang + ((timer * SPIRAL_TURN) /
                                                        SPIRAL_STEPS));
                Float progress = timer * (1.0F / SPIRAL_STEPS);
                Float radius_scale = 1.0F - (progress * SPIRAL_SHRINK);
                Float height_scale = 1.0F - (progress * progress);

                orb->pos.x = GATHER_X +
                             (radius_scale * njSin((Angle)angle) * orb->radius);
                orb->pos.y = GATHER_Y + (height_scale * orb->height);
                orb->pos.z = GATHER_Z +
                             (radius_scale * njCos((Angle)angle) * orb->radius);
                orb->ang = (Angle)angle;
            }
            orb++;
        }
        if (++TASK_TIMER(copy) >= SPIRAL_STEPS) {
            copy->mode = ORBS_FLASH;
            copy->timer = 0;
        }
    } break;
    case ORBS_FLASH:
        VARS(work)->brightness =
            (njSin((TASK_TIMER(work) * FLASH_ARC) / FLASH_STEPS) + 1.0F) * 0.5F;
        if (++TASK_TIMER(work) >= FLASH_STEPS) {
            work->mode = ORBS_DONE;
            work->timer = 0;
        }
        break;
    /* @hack redundant: case ORBS_DONE, which shares the default's code, keeps
     * the image's CMP/EQ #5 at 0x0cb81ddc in EV0091; without it, 387 of 427
     * words differ and the step ends 8 bytes short */
    case ORBS_DONE:
    default:
        return;
    }
    event_orbs_draw(task);
}

static struct task *event_orbs_create(int count)
{
    struct task *task;
    struct task_work *work;
    struct event_orb *orb;
    Uint32 left;

    task = task_create(TASK_WORK, 5, NULL);
    if (task != NULL) {
        task->step = event_orbs_step;
        task->draw = event_orbs_draw;
        task->destructor = event_orbs_destroy;
        work = task->work;
        work->mode = ORBS_GLOW;
        work->tick = count;
        work->timer = 0;
        orb = heap_alloc(count * sizeof(*orb));
        VARS(work)->orbs = orb;
        srand(1);
        for (left = count; left != 0; left--) {
            orb->radius = RANDOM(10.0F, 50.0F);
            orb->height = RANDOM(50.0F, 80.0F);
            orb->ang = (int)(left * NJM_DEG_ANG(360)) / count;
            orb++;
        }
    }
    return task;
}

void event_orbs_start(int count)
{
    if (event_orbs_task == NULL) {
        event_orbs_task = event_orbs_create(count);
    }
}

void event_orbs_kill(void)
{
    if (event_orbs_task != NULL) {
        task_kill(event_orbs_task);
        event_orbs_task = NULL;
    }
}
