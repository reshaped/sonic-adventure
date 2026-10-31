/* an event's picture. all pictures share one sprite and one texanim. each
 * draw points them at its own texlist and texture */
#include <ninja.h>
#include <sg_xpt.h>

#include "shared/picture.h"
#include "system/task.h"

/* the picture's view of its task's work. from byte 8 on, its fields lie
 * where task_work keeps vars, ang, pos and scl */
struct picture_work {
    Uint8 unk_00[8];
    Uint8 blend[2]; /* indexed by NJD_SOURCE_COLOR and NJD_DESTINATION_COLOR */
    Uint8 unk_0a[6];
    NJS_TEXLIST *texlist;
    Sint32 steps; /* left until alpha reaches target */
    Uint32 attr;
    Sint32 texture;
    Float alpha;
    Float target;
    Float depth;
    Float r, g, b;
};

/* 640 by 480 pixels about the screen's middle, at (320, 240) */
static NJS_TEXANIM event_picture_texanim = {
#include "shared/picture/event_picture_texanim.inc"
};
static NJS_SPRITE event_picture_sprite = {
#include "shared/picture/event_picture_sprite.inc"
};

#ifdef GCC
/* a picture nearer than this depth is a still. the events draw stills in
 * front of their scenes, at -1 to -5, and backdrops behind them, at -700 to
 * -1001 */
#define STILL_LIMIT (-10.0F)

/* a band of black from x0 to x1, down the screen, at the picture's alpha
 * and depth */
static void event_picture_band(struct picture_work *work, Float x0, Float x1)
{
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    NJS_POINT2COL band;
    int i;

    points[0].x = points[3].x = x0;
    points[1].x = points[2].x = x1;
    points[0].y = points[1].y = 0.0F;
    points[2].y = points[3].y = 480.0F;
    for (i = 0; i < 4; i++) {
        colors[i].color = (Uint32)(work->alpha * 255.0F) << 24;
    }
    band.p = points;
    band.col = colors;
    band.tex = NULL;
    band.num = 4;
    njDrawPolygon2D(&band, 4, work->depth, NJD_FILL | NJD_TRANSPARENT);
}
#endif

static void event_picture_destroy(struct task *task)
{
}

static void event_picture_draw(struct task *task)
{
    struct picture_work *work = (struct picture_work *)task->work;
    NJS_ARGB color;

    color.a = work->alpha;
    color.r = work->r;
    color.g = work->g;
    color.b = work->b;
    njColorBlendingMode(NJD_SOURCE_COLOR, work->blend[NJD_SOURCE_COLOR]);
    njColorBlendingMode(NJD_DESTINATION_COLOR,
                        work->blend[NJD_DESTINATION_COLOR]);
    njSetConstantMaterial(&color);
    event_picture_sprite.tlist = work->texlist;
    event_picture_texanim.texid = work->texture;
#ifdef GCC
    /* under the port's 16:9 a picture keeps its shape. a still that covers
     * the scene squeezes about the middle, and black bands drawn past its
     * left and right edges squeeze onto the screen's sides. outside
     * pillarbox, a backdrop or a still whose color adds to the scene's fills
     * the screen. it keeps x and scales y by 4/3 about the middle, so a band
     * at its top and bottom falls off the screen */
    if (port_wide_squeezes() && work->depth > STILL_LIMIT &&
        work->blend[NJD_DESTINATION_COLOR] != NJD_COLOR_BLENDING_ONE) {
        njDrawSprite2D(&event_picture_sprite, 0, work->depth, work->attr);
        event_picture_band(work, -320.0F, 0.0F);
        event_picture_band(work, 640.0F, 960.0F);
        return;
    }
    port_wide_cover = TRUE;
    if (!port_wide_squeezes()) {
        event_picture_sprite.sy = 1.0F / port_draw_aspect;
    }
#endif
    njDrawSprite2D(&event_picture_sprite, 0, work->depth, work->attr);
#ifdef GCC
    event_picture_sprite.sy = 1.0F;
    port_wide_cover = FALSE;
#endif
}

/* while steps are left, move alpha 1 / steps of the way to its target.
 * then draw the picture */
static void event_picture_step(struct task *task)
{
    struct picture_work *work = (struct picture_work *)task->work;
    int steps = work->steps;

    if (steps > 0) {
        work->alpha = ((work->alpha * (steps - 1)) + work->target) / steps;
        work->steps = steps - 1;
    }
    event_picture_draw(task);
}

struct task *event_picture_create(NJS_TEXLIST *texlist)
{
    struct task *task = task_create(TASK_WORK, 5, NULL);
    struct picture_work *work;

    if (task != NULL) {
        task->step = event_picture_step;
        task->draw = event_picture_draw;
        task->destructor = event_picture_destroy;
        work = (struct picture_work *)task->work;
        work->blend[NJD_SOURCE_COLOR] = NJD_COLOR_BLENDING_SRCALPHA;
        work->blend[NJD_DESTINATION_COLOR] = NJD_COLOR_BLENDING_INVSRCALPHA;
        work->texlist = texlist;
        work->alpha = 0.0F;
        work->target = 0.0F;
        work->depth = -1.0F;
        work->steps = 0;
        work->attr = NJD_SPRITE_COLOR | NJD_SPRITE_ALPHA;
        work->texture = 0;
        work->r = 1.0F;
        work->g = 1.0F;
        work->b = 1.0F;
    }
    return task;
}

void event_picture_set_color(struct task *task, Float red, Float green,
                             Float blue)
{
    struct picture_work *work = (struct picture_work *)task->work;

    work->r = red;
    work->g = green;
    work->b = blue;
}

void event_picture_fade(struct task *task, Float alpha, int steps)
{
    struct picture_work *work = (struct picture_work *)task->work;

    work->target = alpha;
    work->steps = steps;
}

void event_picture_set_depth(struct task *task, Float depth)
{
    struct picture_work *work = (struct picture_work *)task->work;

    work->depth = depth;
}

void event_picture_set_texture(struct task *task, int texture)
{
    struct picture_work *work = (struct picture_work *)task->work;

    work->texture = texture;
}

void event_picture_set_blend(struct task *task, int which, Uint8 mode)
{
    struct picture_work *work = (struct picture_work *)task->work;

    work->blend[which] = mode;
}

void event_picture_set_height(Sint16 height)
{
    event_picture_texanim.sy = height;
}
