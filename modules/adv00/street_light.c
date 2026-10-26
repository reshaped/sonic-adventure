/* O GAITOU street light, whose lamp glows at night */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/street_light.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/sprite.h"
#include "object/placement.h"
#include "progress/progress.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/texture.h"
#include "system/util.h"

/* the post, whose children are a quad and then the lamp */
extern NJS_OBJECT street_light_object;

/* the quad's blend, under which its color adds to what lies behind it. init
 * sets it in the quad's material, but neither draw draws the quad */
#define QUAD_ATTR (NJD_SA_SRC | NJD_DA_ONE | NJD_FLAG_USE_ALPHA)

#define GLOW_HEIGHT 39.0F

/* 1 for a light started at night, which draws its glow, else 0 */
#define LIT(work) ((work)->submode)

static void street_light_step(struct task *task);
static void street_light_draw(struct task *task);
static void street_light_glow_draw(struct task *task);

/* clang-format off */
static struct collision_shape street_light_shape = {
    SHAPE_CYLINDER | SHAPE_UNTURNED, 0, PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
    {0.0F, 20.5F, 0.0F}, 1.5F, 20.5F, 0.0F, {0}, 0, NULL,
};
/* clang-format on */

static NJS_TEXANIM street_light_glow_texanim = {
#include "adv00/street_light/street_light_glow_texanim.inc"
};
static NJS_SPRITE street_light_glow = {
#include "adv00/street_light/street_light_glow.inc"
};

void street_light_init(struct task *task)
{
    struct task_work *work = task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    work->mode = 0;
    work->vars.f[0] = 0.0F;
    if (!placement_editing(task)) {
        collision_init(task, &street_light_shape, COLLISION_OBJECT, 1, 1);
    }
    street_light_object.child->model->mats[0].attrflags |= QUAD_ATTR;
    task->step = street_light_step;
    if (story_get_time() == TIME_NIGHT) {
        LIT(work) = 1;
        task->draw = street_light_glow_draw;
    } else {
        LIT(work) = 0;
        task->draw = street_light_draw;
    }
}

static void street_light_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        if (!placement_editing(task)) {
            collision_register(work);
        }
        if (LIT(work) == 1) {
            street_light_glow_draw(task);
        } else {
            street_light_draw(task);
        }
    }
}

static void street_light_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Angle ang;
    Float x;
    Float y;
    Float z;
    Float scale_x;
    Float scale_y;
    Float scale_z;
    Float *walk;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    object = &street_light_object;
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    njPushMatrix(NULL);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(object->model);
    njPopMatrix(1);
    /* @hack spelling: the walk to the lamp in two steps, which keeps work in
     * R13 and object in R14 (0x0c908e5c, 0x0c908e6e). in one, the two trade
     * and 13 of 86 words differ */
    object = object->child;
    object = object->sibling;
    /* @hack spelling: the lamp's position and scale read through walk, which
     * loads them with FMOV.S @R4+ (0x0c908ea8 to 0x0c908ebe). indexed, 51 of
     * 86 words differ */
    walk = object->pos;
    x = *walk++;
    y = *walk++;
    z = *walk;
    walk = object->scl;
    scale_x = *walk++;
    scale_y = *walk++;
    scale_z = *walk;
    njTranslate(NULL, x, y, z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, scale_x, scale_y, scale_z);
    draw_model(object->model);
    njPopMatrix(1);
}

/* draw the glow and then the light, on a frame's first step only */
static void street_light_glow_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!frame_step) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        draw_set_material(0.7F, 0.8F, 0.8F, 0.8F);
        street_light_glow.p.x = work->pos.x;
        street_light_glow.p.y = work->pos.y + GLOW_HEIGHT;
        street_light_glow.p.z = work->pos.z;
        sprite_set_depth(-3.0F);
        sprite_draw_3d(&street_light_glow, 0,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        street_light_draw(task);
    }
}
