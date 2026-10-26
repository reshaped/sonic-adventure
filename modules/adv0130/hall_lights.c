/* the hall's light glows: a sprite at each of the hall's 34 lamps, all
 * pulsing together */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0130/egg_carrier_inside.h" /* NOLINT(misc-include-cleaner) */
#include "adv0130/hall_lights.h"
#include "draw/sprite.h"
#include "system/task.h"

/* work->scl.x holds the glows' scale, and work->ang.x the phase of their
 * pulse */
struct hall_lights_vars {
    NJS_POINT3 *places;
    Sint16 count;
    Float alpha; /* from 0.2 to 1, with the pulse's sine */
};

#define VARS(work) ((struct hall_lights_vars *)&(work)->vars)

enum hall_lights_mode { LIGHTS_INIT, LIGHTS_RUN };

static NJS_POINT3 hall_lights_places[34] = {
#include "adv0130/hall_lights/hall_lights_places.inc"
};
static int hall_lights_count = 34;
/* the sprite's frames, the textures edc_k_pointl and edc_k_pointred. the
 * glows draw only the first */
static NJS_TEXANIM hall_lights_frames[2] = {
#include "adv0130/hall_lights/hall_lights_frames.inc"
};
static NJS_SPRITE hall_lights_sprite = {
#include "adv0130/hall_lights/hall_lights_sprite.inc"
};
static NJS_ARGB hall_lights_no_color = {0.0F, 0.0F, 0.0F, 0.0F};

static NJS_ARGB hall_lights_color;

static void hall_lights_draw(struct task *task);

void egg_carrier_hall_lights_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case LIGHTS_INIT:
        work->scl.x = 0.5F;
        VARS(work)->places = hall_lights_places;
        VARS(work)->count = hall_lights_count;
        task->draw = hall_lights_draw;
        work->mode = LIGHTS_RUN;
        break;
    case LIGHTS_RUN:
        work->ang.x += 0x400;
        work->ang.x &= 0xffff;
        VARS(work)->alpha = (njSin(work->ang.x) * 0.4F) + 0.6F;
        break;
    }
    hall_lights_draw(task);
}

static void hall_lights_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_SPRITE *sprite = &hall_lights_sprite;
    NJS_POINT3 *place;
    int i;

    sprite->sx = sprite->sy = work->scl.x;
    place = VARS(work)->places;
    hall_lights_color.a = VARS(work)->alpha;
    hall_lights_color.r = 1.0F;
    hall_lights_color.g = 1.0F;
    hall_lights_color.b = 1.0F;
    njFogDisable();
    sprite_set_depth(-20.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetConstantMaterial(&hall_lights_color);
    for (i = 0; i < VARS(work)->count; i++) {
        sprite->p = *place++;
        sprite_draw_3d(sprite, 0,
                       NJD_SPRITE_COLOR | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
    }
    njSetConstantMaterial(&hall_lights_no_color);
    njFogEnable();
}
