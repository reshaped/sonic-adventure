/* lens flares: a light's glare, and the ghosts strung from the light through
 * the middle of the screen */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "draw/draw.h"
#include "effect/lens.h"
#include "main.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

#define LENS_ATTR (NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR)

/* the frames the light itself draws as, lens01, and its glare, panlight */
#define LENS_LIGHT 0
#define LENS_GLARE 7

#define GHOST_END (-1)

/* the glare draws while facing passes GLARE_FACING. in lens_draw, facing is
 * the cosine of the light's angle off the view, so the glare draws within
 * about 8 degrees. the glare's glow is ((facing - 0.98) * 50) squared, 1
 * head on. the glare draws at an alpha of 0.9 times its glow, and a scale
 * of GLARE_SCALE times it */
#define GLARE_FACING 0.99F
#define GLARE_SCALE 20.0F

/* the ghosts' glow is ((facing - GHOST_FACING) * 3) to the fourth power,
 * and 0 while facing is GHOST_FACING or less. in lens_draw, it rises from 0
 * at about 53 degrees off the view, through 1 at about 21, to about 2.07
 * head on */
#define GHOST_FACING 0.6F

/* a ghost of the flare. it draws at an alpha of alpha * the ghosts' glow.
 * its scale is size * (1 - (1 - glow) * shrink), not below 0 */
struct lens_ghost {
    Sint32 texture; /* in lens_texlist, or GHOST_END */
    Float along; /* from the light, 0, through the screen's middle, 1 */
    Float size;
    Float shrink;
    Float alpha;
    Float depth; /* njDrawSprite2D's pri */
};

static void lens_step(struct task *task);
static void lens_destroy(struct task *task);

/* EFF_REGULAR's lens01 to 07, then panlight */
NJS_TEXLIST lens_texlist = {
#include "effect/lens/lens_texlist.inc"
};
static NJS_TEXANIM lens_frames[8] = {
#include "effect/lens/lens_frames.inc"
};
static NJS_SPRITE lens_sprite = {
#include "effect/lens/lens_sprite.inc"
};
/* the ghosts, ending at a GHOST_END texture */
static struct lens_ghost lens_ghosts[11] = {
#include "effect/lens/lens_ghosts.inc"
};

static void lens_start(struct task *task)
{
    task->step = lens_step;
    task->destructor = lens_destroy;
}

void lens_draw(NJS_POINT3 *pos)
{
    NJS_POINT2 screen;
    NJS_VECTOR view; /* the way the camera looks */
    NJS_VECTOR to_light;
    struct task_work *camera;
    struct lens_ghost *ghost;
    Float facing;
    Float glow;
    Float size;

    camera = camera_work;
    to_light.x = pos->x - camera->pos.x;
    to_light.y = pos->y - camera->pos.y;
    to_light.z = pos->z - camera->pos.z;
    njUnitVector(&to_light);
    /* @hack spelling: the matrix and forward in a scope of their own, whose
     * stack space across takes once it ends: the frame is 116 bytes
     * (ADD #-116,R15 at 0x8c048bc0); declared with the function's locals, it
     * is 124 and 27 of 305 words differ */
    {
        NJS_MATRIX matrix;
        NJS_VECTOR forward;

        forward.x = 0.0F;
        forward.y = 0.0F;
        forward.z = -1.0F;
        njUnitMatrix(&matrix);
        TURN_MATRIX(njRotateY, &matrix, camera->ang.y);
        TURN_MATRIX(njRotateX, &matrix, camera->ang.x);
        njCalcVector(&matrix, &forward, &view);
    }
    facing = njInnerProduct(&view, &to_light);
    if (facing > 0.0F) {
        NJS_POINT2 across;

        njSetTexture(&lens_texlist);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njFogDisable();
        njProjectScreen(NULL, pos, &screen);
        if (facing > GLARE_FACING) {
            glow = (facing - 0.98F) * 50.0F;
            glow = glow * glow;
            draw_set_material(0.9F * glow, 1.0F, 1.0F, 1.0F);
            lens_sprite.p.x = screen.x;
            lens_sprite.p.y = screen.y;
            lens_sprite.sx = glow * GLARE_SCALE;
            lens_sprite.sy = glow * GLARE_SCALE;
            njDrawSprite2D(&lens_sprite, LENS_GLARE, -2.0F, LENS_ATTR);
        }
        if (facing > GHOST_FACING) {
            glow = (facing - GHOST_FACING) * 3.0F;
        } else {
            glow = 0.0F;
        }
        glow = glow * glow * glow * glow;
        across.x = -screen.x + SCREEN_MIDDLE_X;
        across.y = -screen.y + SCREEN_MIDDLE_Y;
        draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
        lens_sprite.p.x = screen.x;
        lens_sprite.p.y = screen.y;
        lens_sprite.sx = 1.0F;
        lens_sprite.sy = 1.0F;
        njDrawSprite2D(&lens_sprite, LENS_LIGHT, -3.0F, LENS_ATTR);
        for (ghost = lens_ghosts; ghost->texture != GHOST_END; ghost++) {
            size = ghost->size * (1.0F - ((1.0F - glow) * ghost->shrink));
            if (size < 0.0F) {
                size = 0.0F;
            }
            draw_set_material(glow * ghost->alpha, 1.0F, 1.0F, 1.0F);
            lens_sprite.p.x = screen.x + (across.x * ghost->along);
            lens_sprite.p.y = screen.y + (across.y * ghost->along);
            lens_sprite.sx = size;
            lens_sprite.sy = size;
            njDrawSprite2D(&lens_sprite, ghost->texture, ghost->depth,
                           LENS_ATTR);
        }
    }
    njFogEnable();
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    lens_sprite.p.x = 0.0F;
    lens_sprite.p.y = 0.0F;
    lens_sprite.sx = 1.0F;
    lens_sprite.sy = 1.0F;
}

static void lens_step(struct task *task)
{
    lens_draw(&task->work->pos);
    /* @hack redundant: a return where the function ends, which makes the frame
     * with ADD #-4,R15 and stores task in it across the call (0x8c048e90,
     * 0x8c048e92); without it, a push does both, the call becomes a tail
     * branch, the function runs 2 bytes short and 6 of 6 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

static void lens_destroy(struct task *task)
{
}

void lens_create(NJS_POINT3 *pos)
{
    struct task *task = task_create(TASK_WORK, 4, lens_start);

    if (task != NULL) {
        struct task_work *work = task->work;

        work->pos = *pos;
    }
}

void lens_draw_facing(NJS_POINT3 *pos, NJS_VECTOR *dir)
{
    NJS_POINT2 screen;
    NJS_VECTOR view; /* the way the camera looks */
    NJS_VECTOR to_light;
    struct task_work *camera;
    struct lens_ghost *ghost;
    Float facing;
    Float glow;
    Float size;

    camera = camera_work;
    to_light.x = pos->x - camera->pos.x;
    to_light.y = pos->y - camera->pos.y;
    to_light.z = pos->z - camera->pos.z;
    njUnitVector(&to_light);
    /* @hack spelling: the matrix and forward in a scope of their own, as in
     * lens_draw: the frame is 120 bytes (ADD #-120,R15 at 0x8c048f16);
     * declared with the function's locals, it is 128, the function runs 2
     * bytes long and 175 of 303 words differ */
    {
        NJS_MATRIX matrix;
        NJS_VECTOR forward;

        forward.x = 0.0F;
        forward.y = 0.0F;
        forward.z = -1.0F;
        njUnitMatrix(&matrix);
        TURN_MATRIX(njRotateY, &matrix, camera->ang.y);
        TURN_MATRIX(njRotateX, &matrix, camera->ang.x);
        njCalcVector(&matrix, &forward, &view);
    }
    facing = -njInnerProduct(&view, &to_light) * njInnerProduct(dir, &to_light);
    if (facing > 0.0F) {
        NJS_POINT2 across;

        njSetTexture(&lens_texlist);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njProjectScreen(NULL, pos, &screen);
        if (facing > GLARE_FACING) {
            glow = (facing - 0.98F) * 50.0F;
            glow = glow * glow;
            draw_set_material(0.9F * glow, 1.0F, 1.0F, 1.0F);
            lens_sprite.p.x = screen.x;
            lens_sprite.p.y = screen.y;
            lens_sprite.sx = glow * GLARE_SCALE;
            lens_sprite.sy = glow * GLARE_SCALE;
            njDrawSprite2D(&lens_sprite, LENS_GLARE, -2.0F, LENS_ATTR);
        }
        if (facing > GHOST_FACING) {
            glow = (facing - GHOST_FACING) * 3.0F;
        } else {
            glow = 0.0F;
        }
        glow = glow * glow * glow * glow;
        across.x = -screen.x + SCREEN_MIDDLE_X;
        across.y = -screen.y + SCREEN_MIDDLE_Y;
        draw_set_material(glow, 1.0F, 1.0F, 1.0F);
        lens_sprite.p.x = screen.x;
        lens_sprite.p.y = screen.y;
        lens_sprite.sx = 1.0F;
        lens_sprite.sy = 1.0F;
        for (ghost = lens_ghosts; ghost->texture != GHOST_END; ghost++) {
            size = ghost->size * (1.0F - ((1.0F - glow) * ghost->shrink));
            if (size < 0.0F) {
                size = 0.0F;
            }
            draw_set_material(glow * ghost->alpha, 1.0F, 1.0F, 1.0F);
            lens_sprite.p.x = screen.x + (across.x * ghost->along);
            lens_sprite.p.y = screen.y + (across.y * ghost->along);
            lens_sprite.sx = size;
            lens_sprite.sy = size;
            njDrawSprite2D(&lens_sprite, ghost->texture, ghost->depth,
                           LENS_ATTR);
        }
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    lens_sprite.p.x = 0.0F;
    lens_sprite.p.y = 0.0F;
    lens_sprite.sx = 1.0F;
    lens_sprite.sy = 1.0F;
}
