/* CHAND__, Chaos 2's chandelier: six lit lamps, a shadow on the floor and
 * a lens flare at its middle. struck, it swings away from what struck it,
 * and the swing dies away */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos2/chandelier.h"
#include "b_chaos2/textures.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "effect/lens.h"
#include "object/common.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

#define SE_CHANDELIER 0x160

/* a strike swings it SWING_REACH from hanging straight. the swing's phase
 * goes round once in SWING_STEPS steps, and each step the swing keeps
 * SWING_DAMP of its reach */
#define SWING_REACH NJM_DEG_ANG(40)
#define SWING_STEPS 60
#define SWING_PACE (NJM_DEG_ANG(360) / SWING_STEPS)
#define SWING_DAMP 0.985F
/* a strike rings it and shakes the pad while its pitch and roll sum to
 * less than SWING_STILL either way */
#define SWING_STILL 0.1F

/* it has LAMPS lamps, and its shadow lies flat at a y of SHADOW_Y */
#define LAMPS 6
#define SHADOW_Y 1.0F

/* its record, at its task's extra. chaos2_chandelier_step allocates its
 * 0x164 bytes (MOV.W at 0x0c907342), and the module uses only the last three
 * fields */
struct chandelier {
    Uint8 unk_00[0x158];
    /* the swing's phase. the swing's angle is cos(phase) times its reach */
    Angle phase;
    Float pitch; /* the swing's reach about x */
    Float roll; /* the swing's reach about z */
};

static void chandelier_destroy(struct task *task);
static void chandelier_draw(struct task *task);
static void chandelier_run(struct task *task);

/* its lamps' color, and its shadow's */
#pragma section HEAD
static Float lamp_alpha = 1.0F;
static Float lamp_red = 1.0F;
static Float lamp_green = 1.0F;
static Float lamp_blue = 1.0F;
static Float shadow_alpha = 0.6F;
static Float shadow_red = 0.6F;
static Float shadow_green = 0.6F;
static Float shadow_blue = 0.6F;
#pragma section

static const Sint8 chandelier_shape_count = 1;

static NJS_MATERIAL chaos2_chandelier_materials[8] = {
#include "b_chaos2/chandelier/chaos2_chandelier_materials.inc"
};
static Sint16 chaos2_chandelier_meshes0[168] = {
#include "b_chaos2/chandelier/chaos2_chandelier_meshes0.inc"
};
static Sint16 chaos2_chandelier_meshes1[60] = {
#include "b_chaos2/chandelier/chaos2_chandelier_meshes1.inc"
};
static Sint16 chaos2_chandelier_meshes2[24] = {
#include "b_chaos2/chandelier/chaos2_chandelier_meshes2.inc"
};
static Sint16 chaos2_chandelier_meshes3[72] = {
#include "b_chaos2/chandelier/chaos2_chandelier_meshes3.inc"
};
static Sint16 chaos2_chandelier_meshes4[12] = {
#include "b_chaos2/chandelier/chaos2_chandelier_meshes4.inc"
};
static Sint16 chaos2_chandelier_meshes5[24] = {
#include "b_chaos2/chandelier/chaos2_chandelier_meshes5.inc"
};
static Sint16 chaos2_chandelier_meshes6[12] = {
#include "b_chaos2/chandelier/chaos2_chandelier_meshes6.inc"
};
static Sint16 chaos2_chandelier_meshes7[24] = {
#include "b_chaos2/chandelier/chaos2_chandelier_meshes7.inc"
};
static Sint16 chaos2_chandelier_meshes8[72] = {
#include "b_chaos2/chandelier/chaos2_chandelier_meshes8.inc"
};
static NJS_COLOR chaos2_chandelier_uvs0[168] = {
#include "b_chaos2/chandelier/chaos2_chandelier_uvs0.inc"
};
static NJS_COLOR chaos2_chandelier_uvs1[52] = {
#include "b_chaos2/chandelier/chaos2_chandelier_uvs1.inc"
};
static NJS_COLOR chaos2_chandelier_uvs2[18] = {
#include "b_chaos2/chandelier/chaos2_chandelier_uvs2.inc"
};
static NJS_COLOR chaos2_chandelier_uvs3[72] = {
#include "b_chaos2/chandelier/chaos2_chandelier_uvs3.inc"
};
static NJS_COLOR chaos2_chandelier_uvs4[12] = {
#include "b_chaos2/chandelier/chaos2_chandelier_uvs4.inc"
};
static NJS_COLOR chaos2_chandelier_uvs5[18] = {
#include "b_chaos2/chandelier/chaos2_chandelier_uvs5.inc"
};
static NJS_COLOR chaos2_chandelier_uvs6[10] = {
#include "b_chaos2/chandelier/chaos2_chandelier_uvs6.inc"
};
static NJS_COLOR chaos2_chandelier_uvs7[24] = {
#include "b_chaos2/chandelier/chaos2_chandelier_uvs7.inc"
};
static NJS_COLOR chaos2_chandelier_uvs8[60] = {
#include "b_chaos2/chandelier/chaos2_chandelier_uvs8.inc"
};
static NJS_MESHSET chaos2_chandelier_meshsets[9] = {
#include "b_chaos2/chandelier/chaos2_chandelier_meshsets.inc"
};
static NJS_POINT3 chaos2_chandelier_points[202] = {
#include "b_chaos2/chandelier/chaos2_chandelier_points.inc"
};
static NJS_VECTOR chaos2_chandelier_normals[202] = {
#include "b_chaos2/chandelier/chaos2_chandelier_normals.inc"
};
static NJS_MODEL chaos2_chandelier_model = {
#include "b_chaos2/chandelier/chaos2_chandelier_model.inc"
};
static NJS_OBJECT chaos2_chandelier_object = {
#include "b_chaos2/chandelier/chaos2_chandelier_object.inc"
};
static NJS_MATERIAL chaos2_chandelier_shadow_materials[1] = {
#include "b_chaos2/chandelier/chaos2_chandelier_shadow_materials.inc"
};
static Sint16 chaos2_chandelier_shadow_meshes0[27] = {
#include "b_chaos2/chandelier/chaos2_chandelier_shadow_meshes0.inc"
};
static NJS_COLOR chaos2_chandelier_shadow_uvs0[24] = {
#include "b_chaos2/chandelier/chaos2_chandelier_shadow_uvs0.inc"
};
static NJS_MESHSET chaos2_chandelier_shadow_meshsets[1] = {
#include "b_chaos2/chandelier/chaos2_chandelier_shadow_meshsets.inc"
};
static NJS_POINT3 chaos2_chandelier_shadow_points[16] = {
#include "b_chaos2/chandelier/chaos2_chandelier_shadow_points.inc"
};
static NJS_VECTOR chaos2_chandelier_shadow_normals[16] = {
#include "b_chaos2/chandelier/chaos2_chandelier_shadow_normals.inc"
};
static NJS_MODEL chaos2_chandelier_shadow_model = {
#include "b_chaos2/chandelier/chaos2_chandelier_shadow_model.inc"
};
static NJS_OBJECT chandelier_shadow_object = {
#include "b_chaos2/chandelier/chandelier_shadow_object.inc"
};

/* a cylinder round its lamps, which pushes every group of kinds and which
 * none of them pushes */
static struct collision_shape chandelier_shape[1] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, -12.0F, 0.0F},
     12.0F,
     7.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* lamp_sprite's frames: a lamp's glow, from the texture chaos_clight, and a
 * light on the floor, from chaos2_yukalight. chandelier_draw draws only the
 * first */
static NJS_TEXANIM chandelier_texanims[2] = {
#include "b_chaos2/chandelier/chandelier_texanims.inc"
};
static NJS_SPRITE lamp_sprite = {
#include "b_chaos2/chandelier/lamp_sprite.inc"
};

/* where its lamps hang and its flare shines, in its own frame. its shadow
 * lies off its x and z by shadow_reach's, turned by the swing */
static NJS_POINT3 lamps[LAMPS] = {
#include "b_chaos2/chandelier/lamps.inc"
};
static NJS_VECTOR shadow_reach = {0.0F, -60.0F, 0.0F};
static NJS_POINT3 flare_pos = {0.0F, -9.0F, 0.0F};

static void chandelier_destroy(struct task *task)
{
}

static void chandelier_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct chandelier *chandelier = task->extra;
    Float swing = njCos(chandelier->phase);
    Angle pitch = (Angle)(swing * chandelier->pitch);
    Angle roll = (Angle)(swing * chandelier->roll);
    Angle ang;
    int i;
    NJS_ARGB color;

    njSetTexture(MODEL(work)->texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z + roll, ang);
    TURN_VIA(njRotateX, work->ang.x + pitch, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(MODEL(work)->object);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    color.a = lamp_alpha;
    color.r = lamp_red;
    color.g = lamp_green;
    color.b = lamp_blue;
    njSetConstantMaterial(&color);
    for (i = 0; i < LAMPS; i++) {
        lamp_sprite.p = lamps[i];
        njDrawSprite3D(&lamp_sprite, 0,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
    }
    njPopMatrix(1);
    {
        NJS_ARGB shadow;
        NJS_VECTOR offset;

        njPushMatrix(NULL);
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateZ, work->ang.z + roll, ang);
        TURN_VIA(njRotateX, work->ang.x + pitch, ang);
        njCalcVector(NULL, &shadow_reach, &offset);
        njPopMatrix(1);
        njTranslate(NULL, work->pos.x + offset.x, SHADOW_Y,
                    work->pos.z + offset.z);
        draw_control_save();
        draw_attr_save();
        draw_control_set(NJD_CONTROL_3D_CONSTANT_ATTR |
                         NJD_CONTROL_3D_CONSTANT_MATERIAL);
        shadow.a = shadow_alpha;
        shadow.r = shadow_red;
        shadow.g = shadow_green;
        shadow.b = shadow_blue;
        njSetConstantMaterial(&shadow);
        njSetConstantAttr(~(NJD_SA_MASK | NJD_DA_MASK),
                          NJD_SA_ONE | NJD_DA_ONE | NJD_FLAG_USE_ALPHA);
        njSimpleDrawObject(&chandelier_shadow_object);
        draw_control_restore();
        draw_attr_restore();
        njPopMatrix(1);
    }
    {
        NJS_POINT3 flare;

        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z + roll, ang);
        TURN_VIA(njRotateX, work->ang.x + pitch, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njCalcPoint(NULL, &flare_pos, &flare);
        njPopMatrix(1);
        lens_draw(&flare);
    }
}

/* its step after the first. struck, it swings away from the task that
 * struck it, and the swing then dies away. it draws itself each step */
static void chandelier_run(struct task *task)
{
    struct task_work *work = task->work;
    struct chandelier *chandelier = task->extra;
    struct task_work *other;
    Angle heading;

    if (!placement_editing(task)) {
        if (work->collision->flags & COLLISION_HIT) {
            other = work->collision->hit->owner->work;
            /* @bug logic: pitch and roll are summed before the size is
             * taken (FADD and FABS at 0x0c907230 and 0x0c907236). the two
             * cancel when the striker's heading from it was 135 or 315
             * degrees, so that swing tests as still however wide it is, and
             * each strike rings it and shakes the pad again */
            if (_builtin_fabsf(chandelier->pitch + chandelier->roll) <
                SWING_STILL) {
                sound_play_at(SE_CHANDELIER, 0, 0, 0, work->pos.x, work->pos.y,
                              work->pos.z);
                vibration_start(0, 4, 15, 0);
            }
            heading = (Angle)(atan2f(other->pos.z - work->pos.z,
                                     other->pos.x - work->pos.x) *
                              ANGLE_PER_RADIAN);
            chandelier->pitch = njCos(heading) * SWING_REACH;
            chandelier->roll = njSin(heading) * SWING_REACH;
            chandelier->phase = 0;
        }
        collision_register(work);
    }
    chandelier->phase += SWING_PACE;
    chandelier->pitch *= SWING_DAMP;
    chandelier->roll *= SWING_DAMP;
    chandelier_draw(task);
}

void chaos2_chandelier_step(struct task *task)
{
    struct task_work *work = task->work;
    struct chandelier *chandelier;

    collision_init(task, chandelier_shape, COLLISION_OBJECT_MUTUAL, 1,
                   chandelier_shape_count);
    MODEL(work)->object = &chaos2_chandelier_object;
    MODEL(work)->texlist = &chaos2_object_texlist;
    task->destructor = chandelier_destroy;
    task->step = chandelier_run;
    task->draw = chandelier_draw;
    chandelier = task->extra = heap_alloc(sizeof(struct chandelier));
    chandelier->phase = 0;
    chandelier->pitch = chandelier->roll = 0.0F;
}
