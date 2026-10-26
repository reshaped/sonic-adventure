/* Tails' trails: the quads his tails leave as he attacks with them */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "object/placement.h"
#include "player/tails_trail.h"
#include "system/task.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 8
#include "player/tails_trail/text.inc"

/* each trail's life, in steps: the outer one's, then the inner one's */
#define TRAIL_STEPS (0.6F * 30.0F)
#define INNER_STEPS (0.4F * 30.0F)
/* the age in steps from which an outer trail strikes, and the age at which
 * it stops */
#define STRIKE_START (0.3F * 30.0F)
#define STRIKE_END ((0.3F * 30.0F) + (0.1F * 30.0F))

/* @hack spelling: the middle of his body in the task's extra, by index, which
 * adds each part's offset to the extra's address (ADD #4,R4 at 0x8c0c7d70);
 * by member through TAILS_TRAIL_CENTER, SHC loads each at an offset in R0,
 * and 243 of the 950 words of the two functions that use it differ */
#define CENTER(task, i) (((Float *)(task)->extra)[i])

/* set out to the shades from and to blended by t, then faded by u. the
 * trails' steps pass u as 1 - t */
#define FADE_COLOR(out, from, to, t, u)                                        \
    {                                                                          \
        (out).argb.r = (u) * (((u) * (from).r) + ((t) * (to).r));              \
        (out).argb.g = (u) * (((u) * (from).g) + ((t) * (to).g));              \
        (out).argb.b = (u) * (((u) * (from).b) + ((t) * (to).b));              \
    }

/* the corners and colors of the quad each outer trail draws, and of the
 * one each inner trail draws. a step fills them just before it draws them.
 * corner 0 is where the tail's end was, and corner 1 where it is. corner 2
 * is the middle of his body now, and corner 3 where it was */
static NJS_POINT3 tails_trail_points[4];
static NJS_COLOR tails_trail_colors[4];
static NJS_POINT3 tails_inner_points[4];
static NJS_COLOR tails_inner_colors[4];

/* no code refers to these texlists or texanims, and only the texlists
 * refer to the textures. they open this object's data */
static NJS_TEXNAME tails_attack_texture = {TEXT_1, 0, 0};
static NJS_TEXLIST tails_attack_texlist = {
#include "player/tails_trail/tails_attack_texlist.inc"
};
static NJS_TEXANIM tails_attack_texanim = {
#include "player/tails_trail/tails_attack_texanim.inc"
};
static NJS_TEXNAME tails_kira_textures[4] = {
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
    {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},
};
static NJS_TEXLIST tails_kira_texlist = {
#include "player/tails_trail/tails_kira_texlist.inc"
};
static NJS_TEXANIM tails_kira_texanims[4] = {
#include "player/tails_trail/tails_kira_texanims.inc"
};

/* the shades an outer trail blends, as tails_inner_shades are an inner
 * trail's. it blends corners 0 and 1 between the first two, and corners 2
 * and 3 between the last two */
static NJS_BGRA tails_trail_shades[4] = {
#include "player/tails_trail/tails_trail_shades.inc"
};
static NJS_POINT3COL tails_trail_quad = {
#include "player/tails_trail/tails_trail_quad.inc"
};
static NJS_BGRA tails_inner_shades[4] = {
#include "player/tails_trail/tails_inner_shades.inc"
};
static NJS_POINT3COL tails_inner_quad = {
#include "player/tails_trail/tails_inner_quad.inc"
};

/* the ball the outer trail strikes enemies with, which its step places past
 * the tail's end */
/* clang-format off */
static struct collision_shape tails_trail_shape = {
    SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,
    ATTACK(1) | SHAPE_HURTS_ENEMIES, PUSH_FIRM(PUSH_OBJECTS),
    {0.0F, 0.0F, 0.0F}, 6.0F, 0.0F, 0.0F, {0}, 0, NULL};
/* clang-format on */

/* step an outer trail: draw its quad, fading, until it is TRAIL_STEPS
 * steps old. on its first step, the quad's body edge closes in on its tail
 * edge. on each later one, the tail edge moves out from the body edge. from
 * STRIKE_START steps old until STRIKE_END, it strikes past the tail's end */
void tails_trail_step(struct task *task)
{
    struct tails_trail_work *work = TAILS_TRAIL_WORK(task);
    NJS_POINT3 tip;
    Float t0;
    Float t1;
    Float u0;
    Float u1;

    if (work->timer == 0) {
        /* @hack spelling: the moves toward the tail's end written out, which
         * load each part once and copy it (FMOV FR5,FR2 at 0x8c0c7d64); as
         * +=, SHC loads it twice, the function runs 32 bytes long and 448 of
         * 510 words differ */
        CENTER(task, 0) =
            CENTER(task, 0) + ((work->to.x - CENTER(task, 0)) * 0.4F);
        CENTER(task, 1) =
            CENTER(task, 1) + ((work->to.y - CENTER(task, 1)) * 0.4F);
        CENTER(task, 2) =
            CENTER(task, 2) + ((work->to.z - CENTER(task, 2)) * 0.4F);
        work->center.x =
            work->center.x + ((work->from.x - work->center.x) * 0.4F);
        work->center.y =
            work->center.y + ((work->from.y - work->center.y) * 0.4F);
        work->center.z =
            work->center.z + ((work->from.z - work->center.z) * 0.4F);
        collision_init(task, &tails_trail_shape, COLLISION_OBJECT, 1, 1);
    } else {
        work->to.x += (work->to.x - CENTER(task, 0)) * 0.01F;
        work->to.y += (work->to.y - CENTER(task, 1)) * 0.01F;
        work->to.z += (work->to.z - CENTER(task, 2)) * 0.01F;
        work->from.x += (work->from.x - work->center.x) * 0.01F;
        work->from.y += (work->from.y - work->center.y) * 0.01F;
        work->from.z += (work->from.z - work->center.z) * 0.01F;
        if (work->timer >= STRIKE_START && work->timer < STRIKE_END) {
            tip.x = ((work->to.x - CENTER(task, 0)) * 0.6F) + work->to.x;
            tip.y = ((work->to.y - CENTER(task, 1)) * 0.6F) + work->to.y;
            tip.z = ((work->to.z - CENTER(task, 2)) * 0.6F) + work->to.z;
            work->collision->shapes->center = tip;
            collision_register((struct task_work *)work);
        }
    }
    if (++work->timer >= TRAIL_STEPS) {
        task_kill(task);
        return;
    }
    t0 = work->timer * (1.0F / TRAIL_STEPS);
    t1 = (work->timer + 1) * (1.0F / TRAIL_STEPS);
    u0 = 1.0F - t0;
    u1 = 1.0F - t1;
    FADE_COLOR(tails_trail_colors[0], tails_trail_shades[0],
               tails_trail_shades[1], t1, u1);
    FADE_COLOR(tails_trail_colors[1], tails_trail_shades[0],
               tails_trail_shades[1], t0, u0);
    FADE_COLOR(tails_trail_colors[3], tails_trail_shades[2],
               tails_trail_shades[3], t1, u1);
    FADE_COLOR(tails_trail_colors[2], tails_trail_shades[2],
               tails_trail_shades[3], t0, u0);
    tails_trail_points[0] = work->from;
    tails_trail_points[1] = work->to;
    tails_trail_points[2].x = CENTER(task, 0);
    tails_trail_points[2].y = CENTER(task, 1);
    tails_trail_points[2].z = CENTER(task, 2);
    /* @hack spelling: the middle of his body before copied float by float, for
     * the image's FMOV.S copies (0x8c0c8134 on); as a struct, SHC copies it
     * in MOV.L words, the function runs 12 bytes short and 88 of 510 words
     * differ */
    tails_trail_points[3].x = work->center.x;
    tails_trail_points[3].y = work->center.y;
    tails_trail_points[3].z = work->center.z;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njDrawPolygon3D(&tails_trail_quad, 4, NJD_FILL | NJD_TRANSPARENT);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    placement_try_leave(task);
}

/* step an inner trail as an outer one, until it is INNER_STEPS steps old.
 * its edges close in further and move apart faster, and it strikes
 * nothing */
void tails_trail_inner_step(struct task *task)
{
    struct tails_trail_work *work = TAILS_TRAIL_WORK(task);
    Float t0;
    Float t1;
    Float u0;
    Float u1;

    if (work->timer == 0) {
        /* @hack spelling: the moves toward the tail's end written out, as in
         * tails_trail_step (FMOV FR5,FR2 at 0x8c0c81a0); as +=, the function
         * runs 32 bytes long and 393 of 440 words differ */
        CENTER(task, 0) =
            CENTER(task, 0) + ((work->to.x - CENTER(task, 0)) * 0.7F);
        CENTER(task, 1) =
            CENTER(task, 1) + ((work->to.y - CENTER(task, 1)) * 0.7F);
        CENTER(task, 2) =
            CENTER(task, 2) + ((work->to.z - CENTER(task, 2)) * 0.7F);
        work->center.x =
            work->center.x + ((work->from.x - work->center.x) * 0.7F);
        work->center.y =
            work->center.y + ((work->from.y - work->center.y) * 0.7F);
        work->center.z =
            work->center.z + ((work->from.z - work->center.z) * 0.7F);
    } else {
        work->to.x += (work->to.x - CENTER(task, 0)) * 0.2F;
        work->to.y += (work->to.y - CENTER(task, 1)) * 0.2F;
        work->to.z += (work->to.z - CENTER(task, 2)) * 0.2F;
        work->from.x += (work->from.x - work->center.x) * 0.2F;
        work->from.y += (work->from.y - work->center.y) * 0.2F;
        work->from.z += (work->from.z - work->center.z) * 0.2F;
    }
    if (++work->timer >= INNER_STEPS) {
        task_kill(task);
        return;
    }
    t0 = work->timer * (1.0F / INNER_STEPS);
    t1 = (work->timer + 1) * (1.0F / INNER_STEPS);
    u0 = 1.0F - t0;
    u1 = 1.0F - t1;
    FADE_COLOR(tails_inner_colors[0], tails_inner_shades[0],
               tails_inner_shades[1], t1, u1);
    FADE_COLOR(tails_inner_colors[1], tails_inner_shades[0],
               tails_inner_shades[1], t0, u0);
    FADE_COLOR(tails_inner_colors[3], tails_inner_shades[2],
               tails_inner_shades[3], t1, u1);
    FADE_COLOR(tails_inner_colors[2], tails_inner_shades[2],
               tails_inner_shades[3], t0, u0);
    tails_inner_points[0] = work->from;
    tails_inner_points[1] = work->to;
    tails_inner_points[2].x = CENTER(task, 0);
    tails_inner_points[2].y = CENTER(task, 1);
    tails_inner_points[2].z = CENTER(task, 2);
    /* @hack spelling: the middle of his body before copied float by float, as
     * in tails_trail_step (0x8c0c84ea on); as a struct, 137 of 440 words
     * differ */
    tails_inner_points[3].x = work->center.x;
    tails_inner_points[3].y = work->center.y;
    tails_inner_points[3].z = work->center.z;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njDrawPolygon3D(&tails_inner_quad, 4, NJD_FILL | NJD_TRANSPARENT);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    placement_try_leave(task);
}
