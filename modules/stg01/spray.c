/* the spray O DOLPHIN throws up: drops that fall and shrink as they fly */
#include <ninja.h>
#include <sg_xpt.h>

#include "effect/dust.h"
#include "effect/snow.h"
#include "stg01/spray.h"
#include "system/task.h"

#define TEXT_1_LENGTH 11
#include "stg01/spray/text.inc"

enum spray_mode { SPRAY_INIT, SPRAY_FLY, SPRAY_END };

/* a drop flies SPRAY_STEPS + 1 steps. each step it rises SPRAY_FALL less than
 * the step before, and loses SPRAY_FALL more of its size */
#define SPRAY_STEPS 60
#define SPRAY_FALL 0.04F

/* a drop with scl.z below 0 leaves a mist puff each time work->tick passes
 * MIST_WAIT: every sixth step */
#define MIST_WAIT 5

/* the drops' texture, and their sprite's one frame, which spans all of it */
static NJS_TEXNAME beach_spray_texnames[1] = {{TEXT_1, 0, 0}};
NJS_TEXLIST beach_spray_texlist = {
#include "stg01/spray/beach_spray_texlist.inc"
};
static NJS_TEXANIM beach_spray_frame[1] = {
#include "stg01/spray/beach_spray_frame.inc"
};

/* the params of the mist puffs the drops leave. beach_spray_draw sets pos and
 * scale before each puff */
static struct dust_params beach_spray_mist = {0.78F,
                                              0.01F,
                                              1.0F,
                                              0.06F,
                                              -0.02F,
                                              {0.0F, 0.0F, 0.0F},
                                              {0.0F, 0.0F, 0.0F},
                                              {0.7F, 1.0F, 1.0F, 1.0F}};

/* the sprite every drop draws, set up again by each drop's first step */
static NJS_SPRITE beach_spray_sprite;

static void beach_spray_draw(struct task_work *work);

void beach_spray_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SPRAY_INIT:
        beach_spray_sprite.tlist = &beach_spray_texlist;
        beach_spray_sprite.tanim = beach_spray_frame;
        beach_spray_sprite.sx = 0.07F;
        beach_spray_sprite.sy = 0.07F;
        beach_spray_sprite.p.x = 0.0F;
        beach_spray_sprite.p.y = 0.0F;
        beach_spray_sprite.p.z = 0.0F;
        work->mode = SPRAY_FLY;
        break;
    case SPRAY_FLY:
        work->tick++;
        work->pos.x += SPRAY_VARS(work)->speed_x;
        work->pos.y +=
            SPRAY_VARS(work)->speed_y - (TASK_TIMER(work) * SPRAY_FALL);
        work->pos.z += SPRAY_VARS(work)->speed_z;
        work->scl.x -= TASK_TIMER(work) * SPRAY_FALL;
        if (work->scl.x < 0.0F) {
            work->scl.x = 0.0F;
        }
        beach_spray_draw(work);
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > SPRAY_STEPS) {
            work->mode = SPRAY_END;
        }
        break;
    case SPRAY_END:
        task_kill(task);
        break;
    }
}

/* draw the drop at its size, and leave its mist */
static void beach_spray_draw(struct task_work *work)
{
    Float size;

    njSetTexture(&beach_spray_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    size = work->scl.x + 1.7F;
    njScale(NULL, size, size, size);
    njDrawSprite3D(&beach_spray_sprite, 0,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_ANGLE);
    njPopMatrix(1);
    if (work->scl.z < 0.0F) {
        if (work->tick > MIST_WAIT) {
            /* @hack redundant: down, which the image copies from its constants
             * (0x0c902d4e to 0x0c902d5e) and never reads. without it, 85 of 87
             * words differ and the constants are 12 bytes fewer */
            NJS_VECTOR down = {0.0F, -1.0F, 0.0F};

            beach_spray_mist.pos.x = work->pos.x;
            beach_spray_mist.pos.y = work->pos.y;
            beach_spray_mist.pos.z = work->pos.z;
            beach_spray_mist.scale = -work->scl.z * 0.02F;
            snow_create_from(&beach_spray_mist);
            work->tick = 0;
        }
    }
}
