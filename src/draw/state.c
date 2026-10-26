/* Ninja's draw state: its 3D control flags and constant attributes, saved
 * around a draw and set for a translucent one. also a task that plays a
 * sprite's texture animation */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/state.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/util.h"

/* the record sprite_anim_desc gives its task, in task->motion. the task
 * draws sprite->tanim[frame], and frame runs from 0 to count - 1 */
struct sprite_anim {
    Sint16 frame;
    Sint16 count;
    NJS_SPRITE *sprite;
    Uint32 attr; /* njDrawSprite3D's flags */
};

/* the arg sprite_anim_desc's creator passes, which sprite_anim_init copies */
struct sprite_anim_arg {
    Sint16 count;
    NJS_SPRITE *sprite;
    Uint32 attr;
};

#define SPRITE_ANIM(task) ((struct sprite_anim *)(task)->motion)

static void sprite_anim_init(struct task *task, void *arg);
static void sprite_anim_step(struct task *task);
static void sprite_anim_draw(struct task *task);

/* a task that draws its sprite at its work's pos, turned by its work's
 * ang. each step moves frame on by 1, back to 0 after count - 1 */
/* NOLINTNEXTLINE(clang-diagnostic-unused-const-variable) */
static const struct task_desc sprite_anim_desc = {
    sizeof(struct sprite_anim), TASK_WORK,        sprite_anim_init,
    sprite_anim_step,           sprite_anim_draw, NULL};
/* the constant material draw_state_restore sets. it is const so that it
 * lies with the constants, as in the image, where translucent_material lies
 * in data. njSetConstantMaterial takes a pointer to non-const, but only
 * reads it, so the cast that drops const is safe */
static const NJS_ARGB draw_state_no_material = {0.0F, 0.0F, 0.0F, 0.0F};

/* the constant material draw_translucent sets: white, at the alpha it takes */
static NJS_ARGB translucent_material = {1.0F, 1.0F, 1.0F, 1.0F};

static void sprite_turn_x(struct angles *ang)
{
    TURN(njRotateX, ang->x);
}

static void sprite_turn_y(struct angles *ang)
{
    TURN(njRotateY, ang->y);
}

static void sprite_turn_z(struct angles *ang)
{
    TURN(njRotateZ, ang->z);
}

void draw_state_save(void)
{
    draw_control_save();
    draw_attr_save();
}

void draw_state_restore(void)
{
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    draw_control_restore();
    draw_attr_restore();
    njSetConstantMaterial((NJS_ARGB *)&draw_state_no_material);
}

/* @hack spelling: draw_translucent's call to draw_constant_attr expands
 * inline (0x8c0a5fe2 to 0x8c0a5ff2). SHC still writes draw_constant_attr's
 * body (0x8c0a5fac), since it has external linkage. called, 29 of
 * draw_translucent's 30 words differ */
#pragma inline(draw_constant_attr)

void draw_constant_attr(Uint32 clear_attr, Uint32 set_attr)
{
    draw_control_set(NJD_CONTROL_3D_CONSTANT_ATTR);
    njSetConstantAttr(~clear_attr, set_attr);
}

void draw_translucent(Float alpha)
{
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    draw_control_clear(NJD_CONTROL_3D_OFFSET_MATERIAL);
    draw_control_set(NJD_CONTROL_3D_CONSTANT_MATERIAL);
    draw_constant_attr(0, NJD_FLAG_USE_ALPHA);
    translucent_material.a = alpha;
    njSetConstantMaterial(&translucent_material);
}

static void sprite_anim_init(struct task *task, void *arg)
{
    struct sprite_anim *anim = SPRITE_ANIM(task);
    struct sprite_anim_arg *from = arg;

    anim->frame = 0;
    anim->count = from->count;
    anim->sprite = from->sprite;
    anim->attr = from->attr;
}

static void sprite_anim_step(struct task *task)
{
    struct sprite_anim *anim = SPRITE_ANIM(task);

    sprite_anim_draw(task);
    anim->frame++;
    if (anim->frame >= anim->count) {
        anim->frame = 0;
    }
}

/* draw only on a frame's first step, while frame_step is 0 */
static void sprite_anim_draw(struct task *task)
{
    struct sprite_anim *anim;
    struct task_work *work;
    struct angles *ang;

    if (!frame_step) {
        work = task->work;
        anim = SPRITE_ANIM(task);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        ang = &work->ang;
        sprite_turn_z(ang);
        sprite_turn_y(ang);
        sprite_turn_x(ang);
        njDrawSprite3D(anim->sprite, anim->frame, anim->attr);
        njPopMatrix(1);
    }
}
