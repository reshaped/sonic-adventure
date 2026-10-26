/* O KNULAMP, a lamp under a flickering red glow, EFF_REGULAR's panlight. the
 * step keeps the glow's red, green and blue in work->scl's x, y and z */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv03/knuckles_lamp.h"
#include "draw/sprite.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT knuckles_lamp_object;

/* the glow's height above the lamp's position */
#define GLOW_HEIGHT 12.5F

/* each step, the glow's red, green and blue are these plus one random
 * flicker, from 0 up to FLICKER */
#define GLOW_RED 0.8F
#define GLOW_GREEN 0.1F
#define GLOW_BLUE 0.1F
#define FLICKER 0.2F

enum knuckles_lamp_mode { LAMP_INIT, LAMP_LIT };

static NJS_TEXLIST glow_texlist = {
#include "adv03/knuckles_lamp/glow_texlist.inc"
};
static NJS_TEXANIM glow_frames[1] = {
#include "adv03/knuckles_lamp/glow_frames.inc"
};
/* the constant material NJD_SPRITE_COLOR draws the glow in. the draw copies
 * work->scl into its red, green and blue, and its alpha stays 0.5.
 * glow_no_color clears the material once the glow is drawn */
static NJS_ARGB glow_color = {0.5F, 1.0F, 0.3F, 0.3F};
static NJS_SPRITE glow_sprite = {
#include "adv03/knuckles_lamp/glow_sprite.inc"
};
static NJS_ARGB glow_no_color = {0.0F, 0.0F, 0.0F, 0.0F};

static void knuckles_lamp_draw(struct task *task);

void knuckles_lamp_step(struct task *task)
{
    struct task_work *work = task->work;
    Float flicker;

    if (!placement_try_leave(task)) {
        switch (work->mode) {
        case LAMP_INIT:
            task->draw = knuckles_lamp_draw;
            work->mode = LAMP_LIT;
            break;
        case LAMP_LIT:
            flicker = rand() * (FLICKER * RAND_SCALE);
            work->scl.x = flicker + GLOW_RED;
            work->scl.y = flicker + GLOW_GREEN;
            work->scl.z = flicker + GLOW_BLUE;
            break;
        }
        knuckles_lamp_draw(task);
    }
}

static void knuckles_lamp_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_SPRITE *sprite = &glow_sprite;

    sprite->p.x = work->pos.x;
    sprite->p.y = work->pos.y + GLOW_HEIGHT;
    sprite->p.z = work->pos.z;
    glow_color.r = work->scl.x;
    glow_color.g = work->scl.y;
    glow_color.b = work->scl.z;
    njFogDisable();
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetConstantMaterial(&glow_color);
    sprite_set_depth(-10.0F);
    sprite_draw_3d(sprite, 0,
                   NJD_SPRITE_COLOR | NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
    njSetConstantMaterial(&glow_no_color);
    njFogEnable();
    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_tree(&knuckles_lamp_object);
    njPopMatrix(1);
}
