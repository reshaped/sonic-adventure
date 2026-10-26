/* Amy's effects: hearts, the chicks round her head as she's dizzy, and her
 * hammer's shock */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/amy.h"
#include "player/amy_effects.h"
#include "player/player.h"
#include "player/textures.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define FALL_STEPS 30
#define DRIFT_STEPS 15
#define SHOCK_STEPS 30

/* the circle the chicks fly round player 0's head: its radius, and its
 * height above her position */
#define CHICK_RADIUS 3.0F
#define CHICK_HEIGHT 12.0F

/* a shock's radius as it starts, how far it spreads each step, and how far
 * it swells along y halfway through its life */
#define SHOCK_RADIUS 3.0F
#define SHOCK_GROWTH 0.5F
#define SHOCK_SWELL 3.0F

#define RING_QUADS 12
#define RING_TURN (0x10000 / RING_QUADS)
/* each corner of the ring is drawn in its shock_ring_colors times the
 * ring's life times this */
#define RING_COLOR_SCALE 0.031F

enum shock_mode { SHOCK_START, SHOCK_SPREAD };

/* each effect also puts work->scl to its own use. a heart moves by it each
 * step, and a shock is drawn scaled by it. chicks_shrink_step grows the
 * chicks by its x each step */
struct effect_vars {
    /* the part of its life a heart has lived, from 0 to 1. for a shock, the
     * part it has left, from 1 to 0 */
    Float life;
};

#define VARS(work) ((struct effect_vars *)&(work)->vars)

extern NJS_OBJECT amy_shock_object;

static void heart_fall_step(struct task *task);
static void heart_drift_step(struct task *task);
static void chicks_draw(struct task_work *work);
static void chicks_shrink_step(struct task *task);

/* the heart's sprite, which shows a_heart2 */
static NJS_TEXANIM heart_texanim = {
#include "player/amy_effects/heart_texanim.inc"
};
static NJS_SPRITE heart_sprite = {
#include "player/amy_effects/heart_sprite.inc"
};
/* the chick's sprite: a_piyo01, and a_piyo02, which chicks_draw flashes
 * over it */
static NJS_TEXANIM chick_texanims[2] = {
#include "player/amy_effects/chick_texanims.inc"
};
static NJS_SPRITE chick_sprite = {
#include "player/amy_effects/chick_sprite.inc"
};

/* the corners of one quad of the ring, which spans a twelfth of a turn */
static NJS_POINT3 shock_ring_points[4] = {
#include "player/amy_effects/shock_ring_points.inc"
};
/* the corners' colors, before amy_shock_ring_step scales them */
static NJS_COLOR shock_ring_colors[4] = {
#include "player/amy_effects/shock_ring_colors.inc"
};
/* the corners' colors as amy_shock_ring_step last scaled them */
static NJS_COLOR shock_ring_fade_colors[4];
/* the quad, in shock_ring_fade_colors */
/* @hack type: an array of one, whose address amy_shock_ring_step keeps in R9
 * across its draws (0x8c0e9a90); as a plain object, R9 holds
 * njDrawPolygon3D's address, the quad's is loaded at each draw, and 84 of
 * that function's 279 words differ */
static NJS_POINT3COL shock_ring_quad[1] = {
#include "player/amy_effects/shock_ring_quad.inc"
};

/* the step of a heart from amy_heart_burst. it falls and turns about y as
 * it fades, and swings about z. it lasts FALL_STEPS steps */
static void heart_fall_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->life += 1.0F / FALL_STEPS;
    if (VARS(work)->life >= 1.0F) {
        task_kill(task);
        return;
    }
    /* @hack spelling: -0.05F added, which keeps the image's FADD and its
     * -0.05F (0x8c0e8fba); as -= 0.05F, SHC writes an FSUB and 1 of 144
     * words differ */
    work->scl.y += -0.05F;
    VECTOR_ADD(&work->pos, &work->scl);
    work->ang.y += 0x400;
    if ((Uint16)work->ang.z > NJM_DEG_ANG(180)) {
        work->ang.x -= 0x100;
    } else {
        work->ang.x += 0x200;
    }
    work->ang.z += work->ang.x;
    if (frame_step == 0) {
        njSetTexture(&amy_eff_texlist);
        draw_set_material(1.0F - VARS(work)->life, 1.0F, 1.0F, 1.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        TURN(njRotateZ, work->ang.z);
        njDrawSprite3D(&heart_sprite, 0, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}

void amy_heart_burst(NJS_POINT3 *pos, Angle ang_x, Angle ang_z)
{
    NJS_POINT3 ring;
    NJS_VECTOR out;
    Angle ang;
    struct task *task;
    struct task_work *work;

    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, ang_z);
    TURN(njRotateX, ang_x);
    ring.x = 0.4358F;
    ring.y = 0.9F;
    ring.z = 0.0F;
    for (ang = 0; ang < 0x10000; ang += NJM_DEG_ANG(45)) {
        njRotateY(NULL, NJM_DEG_ANG(45));
        njCalcPoint(NULL, &ring, &out);
        if ((task = task_create(TASK_WORK, 6, heart_fall_step)) != NULL) {
            work = task->work;
            work->pos = *pos;
            work->pos.x += out.x * 5.0F;
            work->pos.y += out.y * 5.0F;
            work->pos.z += out.z * 5.0F;
            work->scl = out;
            work->ang.y = ang;
            work->ang.z = 0x7000;
        }
    }
    njPopMatrix(1);
}

/* the step of a heart from amy_heart_create. it drifts, slowing by a tenth
 * each step, and fades in and then out. it lasts DRIFT_STEPS steps */
static void heart_drift_step(struct task *task)
{
    struct task_work *work = task->work;
    Float alpha;

    VARS(work)->life += 1.0F / DRIFT_STEPS;
    if (VARS(work)->life >= 1.0F) {
        task_kill(task);
        return;
    }
    work->scl.x *= 0.9F;
    work->scl.y *= 0.9F;
    work->scl.z *= 0.9F;
    VECTOR_ADD(&work->pos, &work->scl);
    if (frame_step == 0) {
        njSetTexture(&amy_eff_texlist);
        alpha = VARS(work)->life - 0.5F;
        alpha = (0.25F - (alpha * alpha)) * 2.0F;
        draw_set_material(alpha, 1.0F, 1.0F, 1.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njDrawSprite3D(&heart_sprite, 0,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}

void amy_heart_create(NJS_POINT3 *pos)
{
    struct task *task;
    NJS_VECTOR way;

    if ((task = task_create(TASK_WORK, 6, heart_drift_step)) != NULL) {
        way.x = RANDOM(-0.5F, 0.5F);
        way.y = RANDOM(-0.5F, 0.5F);
        way.z = RANDOM(-0.5F, 0.5F);
        if (njScalor(&way) == 0.0F) {
            /* @hack spelling: way stored through &way, whose address SHC forms
             * again for each store (MOV R15,R2 and ADD #4,R2 at 0x8c0e9584
             * and again at 0x8c0e958e); stored as way.x, 48 of 103 words
             * differ */
            (&way)->x = 1.0F;
            (&way)->y = (&way)->z = 0.0F;
        } else {
            njUnitVector(&way);
        }
        task->work->scl.x = way.x * 0.25F;
        task->work->scl.y = way.y * 0.25F;
        task->work->scl.z = way.z * 0.25F;
        task->work->pos = *pos;
    }
}

/* draw the chicks round player 0's head, a third of a turn apart from
 * work->ang.y, on a frame's first step. each faces the camera and spins.
 * a_piyo02 flashes over each chick, its alpha the chick's distance from her
 * along x divided by CHICK_RADIUS */
static void chicks_draw(struct task_work *work)
{
    struct task_work *player = player_work[0];
    NJS_POINT3 pos;
    Angle ang;
    Float alpha;

    if (frame_step == 0) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njSetTexture(&amy_eff_texlist);
        /* @bug arithmetic: a third of 0x10000 truncates to 0x5555 (MOV.W at
         * 0x8c0e9758). three steps make 0xffff, one angle unit short of a
         * turn, so the loop draws a fourth chick, over the first */
        for (ang = work->ang.y; ang < work->ang.y + 0x10000;
             ang += 0x10000 / 3) {
            njPushMatrix(NULL);
            pos = player->pos;
            pos.x += njSin(ang) * CHICK_RADIUS;
            pos.z += njCos(ang) * CHICK_RADIUS;
            pos.y += CHICK_HEIGHT;
            njTranslateV(NULL, &pos);
            njPushMatrix(NULL);
            TURN(njRotateY, camera_work->ang.y);
            TURN(njRotateX, camera_work->ang.x);
            /* @hack spelling: -ang shifted left by 2, which SHC negates into
             * R3 and then shifts (NEG R14,R3 at 0x8c0e9706); as -ang * 4, it
             * copies, shifts and then negates, and 61 of 175 words differ */
            TURN(njRotateZ, (Angle)(-ang << 2));
            njDrawSprite3D(&chick_sprite, 0,
                           NJD_SPRITE_ALPHA | NJD_SPRITE_VFLIP);
            njPopMatrix(1);
            /* @hack spelling: the alpha in a statement of its own, which keeps
             * draw_set_material's address in R8 and njSin's in R11
             * (0x8c0e9680, 0x8c0e9682); as the call's argument, the two trade
             * and 6 of 175 words differ */
            alpha = _builtin_fabsf(njSin(ang));
            draw_set_material(alpha, 1.0F, 1.0F, 1.0F);
            njColorBlendingMode(NJD_SOURCE_COLOR,
                                NJD_COLOR_BLENDING_BOTHSRCALPHA);
            njDrawSprite3D(&chick_sprite, 1,
                           NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE |
                               NJD_SPRITE_COLOR);
            draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            njPopMatrix(1);
        }
    }
}

/* the step of the chicks once she's no longer dizzy. they grow a little
 * and then shrink away, and the task ends once their y scale is under
 * 0.1 or player 0 has no task */
static void chicks_shrink_step(struct task *task)
{
    struct task_work *work = task->work;

    if (player_work[0] == NULL) {
        task_kill(task);
        return;
    }
    work->scl.x -= 0.025F;
    chick_sprite.sx += work->scl.x;
    /* @hack spelling: the size tested where it's stored, which tests the sum
     * still in FR1 (FCMP/GT FR1,FR2 at 0x8c0e981c); stored in a statement of
     * its own, SHC loads it again, the function runs 4 bytes longer and 9 of
     * 38 words differ */
    if ((chick_sprite.sy += work->scl.x) < 0.1F) {
        task_kill(task);
    } else {
        chicks_draw(work);
    }
}

void amy_chicks_step(struct task *task)
{
    struct task_work *work = task->work;
    Float scale;

    if (player_work[0] == NULL) {
        task_kill(task);
        return;
    }
    if (amy_get_action(0) != AMY_DIZZY) {
        work->scl.x = 0.2F;
        chicks_shrink_step(task);
        task->step = chicks_shrink_step;
    } else {
        work->ang.y += 0x200;
        scale = (njSin(work->ang.y * 4) * 0.3F) + 1.0F;
        chick_sprite.sx = scale;
        chick_sprite.sy = scale;
        chicks_draw(work);
    }
}

/* the ring spreads from SHOCK_RADIUS as it fades, and its height swells to
 * SHOCK_SWELL and sinks back. it lasts SHOCK_STEPS steps */
void amy_shock_ring_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    /* @hack type: i a Uint16, which keeps the EXTU.W of the loop's test
     * (EXTU.W R5,R2 at 0x8c0e9a26); as an int, the function runs 32 bytes
     * longer and 140 of 279 words differ */
    Uint16 i;

    if (work->mode == SHOCK_START) {
        work->scl.x = SHOCK_RADIUS;
        VARS(work)->life = 1.0F;
        work->mode = SHOCK_SPREAD;
    }
    /* @hack spelling: -1.0F / SHOCK_STEPS added, which keeps the image's FADD
     * and its negative constant (0x8c0e9912); as -= 1.0F / SHOCK_STEPS, SHC
     * writes an FSUB and 1 of 279 words differ */
    /* @hack spelling: the life tested where it's stored, which tests the sum
     * still in FR2 (FCMP/GT FR2,FR1 at 0x8c0e9914); stored in a statement of
     * its own, SHC loads it again, njPushMatrix's address and RING_TURN trade
     * R11 and R12, and 27 of 279 words differ */
    if ((VARS(work)->life += -1.0F / SHOCK_STEPS) < 0.0F) {
        task_kill(task);
        return;
    }
    work->scl.x += SHOCK_GROWTH;
    work->scl.y = njSin((Angle)(VARS(work)->life * 32768.0F)) * SHOCK_SWELL;
    work->scl.z = work->scl.x;
    for (i = 0; i < 4; i++) {
        shock_ring_fade_colors[i].argb.r =
            VARS(work)->life * shock_ring_colors[i].argb.r * RING_COLOR_SCALE;
        shock_ring_fade_colors[i].argb.g =
            VARS(work)->life * shock_ring_colors[i].argb.g * RING_COLOR_SCALE;
        shock_ring_fade_colors[i].argb.b =
            VARS(work)->life * shock_ring_colors[i].argb.b * RING_COLOR_SCALE;
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    for (ang = 0; ang < RING_QUADS * RING_TURN; ang += RING_TURN) {
        njPushMatrix(NULL);
        TURN(njRotateY, ang);
        njScaleV(NULL, &work->scl);
        njDrawPolygon3D(shock_ring_quad, 4, NJD_FILL | NJD_TRANSPARENT);
        njPopMatrix(1);
    }
    njPopMatrix(1);
    placement_try_leave(task);
}

/* the wave, a model, spreads and swells as the ring does, and lasts as long */
void amy_shock_wave_step(struct task *task)
{
    struct task_work *work = task->work;

    if (work->mode == SHOCK_START) {
        work->scl.x = SHOCK_RADIUS;
        VARS(work)->life = 1.0F;
        work->mode = SHOCK_SPREAD;
    }
    /* @hack spelling: -1.0F / SHOCK_STEPS added, which keeps the image's FADD
     * and its negative constant (0x8c0e9bc4); as -= 1.0F / SHOCK_STEPS, SHC
     * writes an FSUB and 1 of 121 words differ */
    /* @hack spelling: the life tested where it's stored, which tests the sum
     * still in FR2 (FCMP/GT FR2,FR1 at 0x8c0e9bc6); stored in a statement of
     * its own, SHC loads it again, the code after runs a word later and 98
     * of 121 words differ */
    if ((VARS(work)->life += -1.0F / SHOCK_STEPS) < 0.0F) {
        task_kill(task);
        return;
    }
    work->scl.x += SHOCK_GROWTH;
    work->scl.y = njSin((Angle)(VARS(work)->life * 32768.0F)) * SHOCK_SWELL;
    work->scl.z = work->scl.x;
    if (frame_step == 0) {
        njSetTexture(&amy_eff_texlist);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        njScaleV(NULL, &work->scl);
        draw_tree(&amy_shock_object);
        njPopMatrix(1);
    }
    placement_try_leave(task);
}
