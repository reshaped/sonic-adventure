/* the beads that rise inside Chaos 2's ball. each bead circles as it rises,
 * and fades in and out */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos2/beads.h"
#include "b_chaos2/boss.h"
#include "camera/camera.h"
#include "chaos/ribbon.h"
#include "draw/motion.h"
#include "system/task.h"
#include "system/util.h"

/* a bead starts on the ball's lower surface, within BEAD_REACH of the
 * ball's middle on x and on z */
#define BEAD_REACH (BALL_RADIUS * 0.7F)
/* a bead's alpha rises FADE_STEP a step as it fades in, and falls as much
 * as it fades out */
#define FADE_STEP 0.05F

/* the record at a bead's task->extra, which chaos2_beads_create
 * allocates */
struct bead {
    Float scale; /* its sprite's scale at full alpha */
    Float alpha; /* from 0 to 1. the sprite shrinks with it */
    Float radius; /* the radius of the circle it runs round, 1 to 2 */
    Angle ang; /* its angle round that circle */
    Angle spin; /* the angle it turns a step, 2 to 10 degrees */
    Float speed; /* the y it rises a step, 0.3 to 0.8 */
    Float top; /* the y over the boss's at which it starts to fade out */
};

enum bead_mode { BEAD_START, BEAD_FADE_IN, BEAD_RISE, BEAD_FADE_OUT };

/* draw the bead's sprite facing the camera. the sprite stands at boss_work's
 * pos, plus the bead's work->pos and its place round its circle */
static void bead_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct bead *bead = task->extra;
    NJS_VECTOR offset;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &boss_work->pos);
    offset.x = bead->radius * njCos(bead->ang);
    offset.z = bead->radius * njSin(bead->ang);
    offset.y = 0.0F;
    njAddVector(&offset, &work->pos);
    njTranslateV(NULL, &offset);
    ribbon_sprite.sx =
        bead->scale * (1.0F + (0.3F * njSin(bead->ang))) * bead->alpha;
    ribbon_sprite.sy =
        bead->scale * (1.0F + (0.3F * njCos(bead->ang))) * bead->alpha;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njSetConstantMaterial(&ribbon_color);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    njDrawSprite3D(&ribbon_sprite, 0,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
    njPopMatrix(1);
}

static void bead_step(struct task *task)
{
    struct task_work *work = task->work;
    struct bead *bead = task->extra;
    Float across_squared;
    Float depth;

    switch (work->mode) {
    case BEAD_START:
        work->pos.x = BEAD_REACH - (rand() * (BEAD_REACH / 16384.0F));
        work->pos.z = BEAD_REACH - (rand() * (BEAD_REACH / 16384.0F));
        across_squared =
            (work->pos.x * work->pos.x) + (work->pos.z * work->pos.z);
        depth = njSqrt((BALL_RADIUS * BALL_RADIUS) - across_squared);
        work->pos.y = BALL_RADIUS - depth;
        bead->speed = 0.3F + (rand() * (1.0F / 65536.0F));
        bead->top = (depth * 1.4F) - (bead->speed * 20.0F);
        bead->alpha = 0.0F;
        bead->ang = 0;
        bead->spin = (Angle)((0.2F + (rand() * (0.8F * RAND_SCALE))) *
                             (ANGLE_PER_DEGREE * 10.0F));
        bead->radius = 1.0F + (rand() * RAND_SCALE);
        task->draw = bead_draw;
        work->mode = BEAD_FADE_IN;
        /* fall through */
    case BEAD_FADE_IN:
        bead->alpha += FADE_STEP;
        if (bead->alpha >= 1.0F) {
            work->mode = BEAD_RISE;
        }
        break;
    case BEAD_RISE:
        if (work->pos.y >= bead->top) {
            work->mode = BEAD_FADE_OUT;
        }
        break;
    case BEAD_FADE_OUT:
        bead->alpha -= FADE_STEP;
        if (bead->alpha <= 0.0F) {
            task_destroy(task);
            return;
        }
        break;
    }
    bead->ang += bead->spin;
    work->pos.y += bead->speed;
    bead_draw(task);
}

void chaos2_beads_create(struct task *parent, int count, Float scale)
{
    int i;
    int beads;
    struct task *child;
    struct bead *bead;

    beads = (int)(((0.5F + (rand() * (1.0F / 65536.0F))) * count) + 0.1F);
    for (i = 0; i < beads; i++) {
        child = task_create_child(TASK_WORK, bead_step, parent);
        if (child) {
            child->extra = heap_alloc(sizeof(struct bead));
            bead = child->extra;
            bead->scale = scale * (1.0F + (rand() * (1.0F / 65536.0F)));
        }
    }
}
