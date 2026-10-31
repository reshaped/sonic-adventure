/* the Egg Hornet's barrier. it turns through eight textures, and draws
 * twice over at two sizes */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm1/barrier.h"
#include "b_egm1/effect_textures.h"
#include "b_egm1/flight.h"
#include "b_egm1/util.h"
#include "draw/draw.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the bits of a material's attr_texId above its texture */
#define TEXID_ATTR 0xe0000000
/* the textures the barrier turns through. a power of two, as the step
 * wraps its index with a mask */
#define BARRIER_TEXTURES 8

/* the fade's change each step, and the steps each texture shows */
#define BARRIER_FADE 0.025F
#define BARRIER_INTERVAL 2

/* egm1barrier_texlist's eight entries. the stage's load fills them from
 * the egm1barrier pvm's textures, in order */
static NJS_TEXNAME egm1barrier_textures[BARRIER_TEXTURES];
NJS_TEXLIST egm1barrier_texlist = {
#include "b_egm1/barrier/egm1barrier_texlist.inc"
};

void fun_0c900bc0(void)
{
}

static void egg_hornet_barrier_set_texture(Uint32 texture)
{
    NJS_OBJECT *object = &egg_hornet_barrier_object;
    NJS_MODEL *model = object->model;
    NJS_MATERIAL *material = model->mats;

    material->attr_texId = (material->attr_texId & TEXID_ATTR) | texture;
}

void egg_hornet_barrier_init(struct barrier *barrier)
{
    NJS_OBJECT *object = &egg_hornet_barrier_object;
    NJS_MODEL *model = object->model;
    NJS_MATERIAL *material = model->mats;

    material->attrflags = (material->attrflags & ~(NJD_SA_MASK | NJD_DA_MASK)) |
                          NJD_SA_SRC | NJD_DA_ONE;
    barrier->state = BARRIER_OFF;
    barrier->request = BARRIER_KEEP;
    barrier->timer = 0;
    barrier->interval = BARRIER_INTERVAL;
    barrier->texture = 0;
}

void egg_hornet_barrier_step(struct barrier *barrier)
{
    switch (barrier->request) {
    case BARRIER_SHOW:
        if (barrier->state != BARRIER_FADING_IN) {
            barrier->fade = 0.0F;
            barrier->alpha_offset = -0.99F;
            barrier->unk_08 = 0.0F;
            barrier->state = BARRIER_FADING_IN;
        }
        break;
    case BARRIER_HIDE:
        if (barrier->state != BARRIER_FADING_OUT) {
            barrier->fade = 1.0F;
            barrier->alpha_offset = 0.0F;
            barrier->state = BARRIER_FADING_OUT;
        }
        break;
    }
    switch (barrier->state) {
    case BARRIER_FADING_IN:
        barrier->fade += BARRIER_FADE;
        if (barrier->fade >= 1.0F) {
            barrier->state = BARRIER_ON;
            barrier->request = BARRIER_KEEP;
        }
        barrier->alpha_offset = barrier->fade - 1.0F;
        break;
    case BARRIER_ON:
        break;
    case BARRIER_FADING_OUT:
        barrier->fade -= BARRIER_FADE;
        if (barrier->fade <= 0.0F) {
            barrier->state = BARRIER_OFF;
            barrier->request = BARRIER_KEEP;
        }
        barrier->alpha_offset = barrier->fade - 1.0F;
        break;
    default:
        break;
    }
    barrier->timer++;
    if (barrier->timer >= barrier->interval) {
        barrier->timer = 0;
        barrier->texture = (barrier->texture + 1) & (BARRIER_TEXTURES - 1);
    }
    egg_hornet_barrier_set_texture(barrier->texture);
}

void egg_hornet_barrier_draw(struct barrier *barrier)
{
    struct task_work *work;
    struct angles *ang;

    if (barrier->state != BARRIER_OFF) {
        draw_control_save();
        draw_attr_save();
        njControl3D(NJD_CONTROL_3D_CONSTANT_ATTR |
                    NJD_CONTROL_3D_OFFSET_MATERIAL);
        njSetConstantAttr(0xffffffff, NJD_FLAG_USE_ALPHA);
        draw_set_material(barrier->alpha_offset, 0.0F, 0.0F, 0.0F);
        njPushMatrix(NULL);
        work = barrier->owner->work;
        njTranslate(NULL, work->pos.x, work->pos.y - 10.0F, work->pos.z);
        ang = &barrier->owner->work->ang;
        if (egg_hornet_diving(barrier->owner)) {
            njRotateZXY(NULL, ang->x, ang->y, ang->z);
        } else {
            matrix_turn_zxy(NULL, ang);
        }
        njRotateX(NULL, -0x4000);
        njScale(NULL, 2.0F, 2.0F, 2.0F);
        njSetTexture(&egm1barrier_texlist);
#ifdef GCC
        /* the second picture's pass skips the boss's callback where step 1
         * drew the boss, so step 1 draws the barrier too, where port.h's
         * FRAME_DRAWS says */
        if (FRAME_DRAWS()) {
            present_untimed(TRUE);
            njSimpleDrawObject(&egg_hornet_barrier_object);
            present_untimed(FALSE);
        }
#else
        if (frame_step == 0) {
            njSimpleDrawObject(&egg_hornet_barrier_object);
        }
#endif
        njScale(NULL, 1.2F, 1.2F, 1.2F);
        njSetTexture(&egm1barrier_texlist);
#ifdef GCC
        if (FRAME_DRAWS()) {
            present_untimed(TRUE);
            njSimpleDrawObject(&egg_hornet_barrier_object);
            present_untimed(FALSE);
        }
#else
        if (frame_step == 0) {
            njSimpleDrawObject(&egg_hornet_barrier_object);
        }
#endif
        njPopMatrix(1);
        draw_attr_restore();
        draw_control_restore();
        draw_control_save();
        njControl3D(NJD_CONTROL_3D_CONSTANT_MATERIAL);
        draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
        draw_control_restore();
    }
}

Bool egg_hornet_barrier_request(struct barrier *barrier, int request)
{
    barrier->request = request;
    return TRUE;
}

void egg_hornet_barrier_set_owner(struct barrier *barrier, struct task *owner)
{
    barrier->owner = owner;
}
