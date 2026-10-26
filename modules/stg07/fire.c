/* O FIRE, flames that rise round a glow. each step the fire is in view, it
 * makes a flame and draws the glow. its flames grow with scl.y */
#include <ninja.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/dust.h"
#include "effect/flame.h"
#include "effect/panlight.h"
#include "object/placement.h"
#include "stg07/fire.h"
#include "system/task.h"

/* a fire's work->mode. no code sets FIRE_OUT */
enum fire_mode { FIRE_START, FIRE_BURN, FIRE_OUT };

/* draw_in_view tests a sphere of FIRE_VIEW_RADIUS round the fire. a
 * flame's scale is FLAME_SCALE times (scl.y + 1) */
#define FIRE_VIEW_RADIUS 10.0F
#define FLAME_SCALE 0.12F

/* clang-format off */
static struct collision_shape fire_shape = {
    SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F},
    1.5F, 0.0F, 0.0F, {0}, 0, NULL,
};
/* clang-format on */

static NJS_TEXANIM fire_glow_frame = {
#include "stg07/fire/fire_glow_frame.inc"
};
static struct dust_params fire_flames = {
    FLAME_SCALE,
    0.01F,
    0.29F,
    0.065F,
    0.005F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

/* the glow sprite every fire shares. each fire's start sets it up the same
 * way */
static NJS_SPRITE fire_glow;

static void fire_destroy(struct task *task);
static void fire_glow_draw(struct task *task);
static void fire_glow_init(struct task *task);

void ruin_fire_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        switch (work->mode) {
        case FIRE_START:
            fire_glow_init(task);
            task->destructor = fire_destroy;
            work->mode = FIRE_BURN;
            break;
        case FIRE_BURN:
            if (draw_in_view(work->pos.x, work->pos.y, work->pos.z,
                             FIRE_VIEW_RADIUS)) {
                if (placement_editing(task)) {
                    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y),
                             "SCL Y -> FIRE SCL");
                }
                ++work->timer;
                fire_flames.pos = work->pos;
                fire_flames.scale = (work->scl.y + 1.0F) * FLAME_SCALE;
                flame_create_from(&fire_flames);
                fire_glow_draw(task);
            }
            break;
        case FIRE_OUT:
            break;
        }
    }
}

static void fire_destroy(struct task *task)
{
}

/* draw the glow at the fire with fog off, added to what's behind it in a
 * dim amber */
static void fire_glow_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;

    njSetTexture(&panlight_texlist);
    color.a = -1.0F;
    color.r = 0.14F;
    color.g = 0.11F;
    color.b = 0.0F;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njFogDisable();
    njDrawSprite3D(&fire_glow, 0,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR |
                       NJD_SPRITE_ANGLE);
    njPopMatrix(1);
    njFogEnable();
    color.a = 0.0F;
    color.b = 0.0F;
    color.g = 0.0F;
    color.r = 0.0F;
    njSetConstantMaterial(&color);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
}

static void fire_glow_init(struct task *task)
{
    fire_glow.tlist = &panlight_texlist;
    fire_glow.tanim = &fire_glow_frame;
    fire_glow.sx = 0.35F;
    fire_glow.sy = 0.35F;
    fire_glow.p.x = 0.0F;
    fire_glow.p.y = 0.0F;
    fire_glow.p.z = 0.0F;
}
