/* O FIRE, a fire sprite that plays bigfire01 to 16, two steps each, blended
 * additively over what lies behind it */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv03/effect_textures.h" /* NOLINT(misc-include-cleaner) */
#include "adv03/fire.h"
#include "draw/sprite.h"
#include "object/placement.h"
#include "system/task.h"

/* the fire's sprite frames. frame n shows texture n of past_fire_texlist */
extern NJS_TEXANIM past_fire_frames[];

/* the fire plays its FIRE_FRAMES sprite frames round and round, FIRE_SPEED
 * of a frame each step */
#define FIRE_FRAMES 16
#define FIRE_SPEED 0.5F

enum fire_mode { FIRE_INIT, FIRE_BURN };

struct fire_vars {
    Uint8 unk_00[8];
    Float frame;
};

#define VARS(work) ((struct fire_vars *)&(work)->vars)

static NJS_SPRITE fire_sprite = {
#include "adv03/fire/fire_sprite.inc"
};
/* the constant material NJD_SPRITE_COLOR draws the fire in, white.
 * fire_no_color clears the material once the fire is drawn */
static NJS_ARGB fire_color = {1.0F, 1.0F, 1.0F, 1.0F};
static NJS_ARGB fire_no_color = {0.0F, 0.0F, 0.0F, 0.0F};

void fire_step(struct task *task)
{
    struct task_work *work = task->work;
    int frame;

    if (!placement_try_leave(task)) {
        switch (work->mode) {
        case FIRE_INIT:
            work->mode = FIRE_BURN;
            break;
        case FIRE_BURN:
            VARS(work)->frame += FIRE_SPEED;
            frame = (int)VARS(work)->frame;
            if (frame >= FIRE_FRAMES) {
                VARS(work)->frame = 0.0F;
                frame = 0;
            }
            njFogDisable();
            njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
            njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
            njSetConstantMaterial(&fire_color);
            sprite_set_depth(-10.0F);
            fire_sprite.p = work->pos;
            /* with NJD_SPRITE_SCALE the sprite faces the screen, and
             * sprite_draw_3d scales a facing sprite's height by sx too. scl.y,
             * copied into sy, goes unread */
            fire_sprite.sx = work->scl.x;
            fire_sprite.sy = work->scl.y;
            sprite_draw_3d(&fire_sprite, frame,
                           NJD_SPRITE_COLOR | NJD_SPRITE_VFLIP |
                               NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
            njSetConstantMaterial(&fire_no_color);
            njFogEnable();
            break;
        }
    }
}
