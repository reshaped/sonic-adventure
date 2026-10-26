/* StarNeon, YajiNeon and Densyoku: a star, arrows and strings of lights in
 * neon, each flashing through its frames */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/model.h"
#include "stg09/neon.h"
#include "stg09/obj_casino_textures.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_MATERIAL neon_star_materials[];
extern NJS_MODEL neon_star_model;
extern NJS_MATERIAL neon_arrow_materials[];
extern NJS_MODEL neon_arrow_model;
extern NJS_MATERIAL neon_lights_materials[];
extern NJS_MODEL neon_lights_model;

/* kinds of neon: the star, three arrows, then three strings of lights */
#define NEON_STAR 0
#define NEON_ARROW 1
#define NEON_ARROW_LAST 3
#define NEON_LIGHTS 4

/* neon_texlist holds the star's 2 frames as textures 0 and 1, and each
 * arrow's 2 after them. each string's LIGHTS_FRAMES frames follow from
 * texture LIGHTS_TEXTURES on. a string's frame takes 2 textures, one for each
 * of its model's materials */
#define LIGHTS_FRAMES 3
#define LIGHTS_TEXTURES 8

#define KIND(work) ((work)->mode)
/* the texture frame it shows: 0 or 1, or 0 to 2 for a string */
#define FRAME(work) ((work)->tick)
#define CLOCK(work) ((work)->vars.f[0]) /* steps since its frame changed */
/* a frame shows for PERIOD + 2 steps, with PERIOD rounded down */
#define PERIOD(work) ((work)->scl.z)
/* which arrow or string: 0, 1, or any other value for the third */
#define VARIANT(work) ((work)->scl.y)

static void neon_destroy(struct task *task)
{
}

static void neon_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    Float scale;

    if (frame_step) {
        return;
    }
    if (KIND(work) == NEON_STAR) {
        neon_star_materials[0].attr_texId = FRAME(work);
    } else if (KIND(work) <= NEON_ARROW_LAST) {
        neon_arrow_materials[0].attr_texId = (KIND(work) * 2) + FRAME(work);
    } else {
        neon_lights_materials[0].attr_texId =
            ((((KIND(work) - NEON_LIGHTS) * LIGHTS_FRAMES) + FRAME(work)) * 2) +
            (LIGHTS_TEXTURES + 1);
        neon_lights_materials[1].attr_texId =
            ((((KIND(work) - NEON_LIGHTS) * LIGHTS_FRAMES) + FRAME(work)) * 2) +
            LIGHTS_TEXTURES;
    }
    njSetTexture(&neon_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    scale = work->scl.x;
    njScale(NULL, scale, scale, scale);
    if (KIND(work) == NEON_STAR) {
        draw_model(&neon_star_model);
    } else if (KIND(work) <= NEON_ARROW_LAST) {
        draw_model(&neon_arrow_model);
    } else {
        draw_model(&neon_lights_model);
    }
    njPopMatrix(1);
}

static void neon_run(struct task *task)
{
    struct task_work *work = task->work;

    if (CLOCK(work) > PERIOD(work)) {
        CLOCK(work) = 0.0F;
        if (KIND(work) <= NEON_ARROW_LAST) {
            FRAME(work) = (FRAME(work) + 1) & 1;
        } else {
            FRAME(work) = (FRAME(work) + 1) % LIGHTS_FRAMES;
        }
    } else {
        CLOCK(work) += 1.0F;
    }
    neon_draw(task);
}

static void neon_init(struct task *task)
{
    /* @hack spelling: each store through task->work, which SHC loads again for
     * each (MOV.L @(32,R4),R3 at 0x0c925440 and 0x0c92544a). through a
     * local work, 12 of neon_init's 14 words differ */
    CLOCK(task->work) = 0.0F;
    FRAME(task->work) = 0;
    task->step = neon_run;
    task->draw = neon_draw;
    task->destructor = neon_destroy;
}

void neon_star_step(struct task *task)
{
    struct task_work *work = task->work;

    KIND(work) = NEON_STAR;
    neon_init(task);
}

void neon_arrow_step(struct task *task)
{
    struct task_work *work = task->work;

    if (VARIANT(work) == 0.0F) {
        KIND(work) = NEON_ARROW;
    } else if (VARIANT(work) == 1.0F) {
        KIND(work) = NEON_ARROW + 1;
    } else {
        KIND(work) = NEON_ARROW + 2;
    }
    neon_init(task);
}

void neon_lights_step(struct task *task)
{
    struct task_work *work = task->work;

    if (VARIANT(work) == 0.0F) {
        KIND(work) = NEON_LIGHTS;
    } else if (VARIANT(work) == 1.0F) {
        KIND(work) = NEON_LIGHTS + 1;
    } else {
        KIND(work) = NEON_LIGHTS + 2;
    }
    neon_init(task);
}
