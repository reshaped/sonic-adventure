/* Final Egg's animated texture, and O Texture, a square that shows it */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/tree.h"
#include "stg10/texture.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* @hack type: stage_get_stage_act declared to return an int, where stage.h's
 * returns a Uint16. SHC then copies the result unextended (MOV R0,R3 at
 * 0x0c911978) for the compare at 0x0c91197e. as a Uint16, SHC extends it
 * with EXTU.W, and 1 of final_texture_step's 51 words differs */
extern int stage_get_stage_act(void);

/* a square 180 units a side, in the plane of x and z */
extern NJS_OBJECT final_texture_panel_object;

/* the texture's width and height, in pixels */
#define SIDE 64
#define PALETTES 6

enum panel_mode { PANEL_INIT, PANEL_RUN };

enum texture_mode { TEXTURE_INIT, TEXTURE_RUN, TEXTURE_SLEEP };

/* the texture task's work. TASK_TIMER(work) holds the stage and act it
 * started in */
struct texture_vars {
    Uint8 phase[4]; /* each wave's phase, an index into final_texture_sines */
};

#define VARS(work) ((struct texture_vars *)&(work)->vars)

static void final_texture_panel_draw(struct task *task);
static void final_texture_panel_destroy(struct task *task);
static void final_texture_step(struct task *task);
static void final_texture_init(struct task *task);
static void final_texture_destroy(struct task *task);
static void final_texture_load(void);
static void final_texture_reload(void);
static void final_texture_make_sines(void);
static void final_texture_reset(struct task *task);
static void final_texture_paint(struct task *task);

static NJS_TEXINFO final_texture_info;
static NJS_TEXNAME final_texture_name;
/* njSin over one turn in 256 samples, each mapped from -1 to 1 onto 0 to
 * 64 */
static Uint8 final_texture_sines[256];
static Uint16 final_texture_pixels[SIDE * SIDE];
/* the phases of waves 2 and 3 at the pixel being painted. each row starts
 * them at the work's phase[2] and phase[3] */
static Uint8 final_texture_phase2;
static Uint8 final_texture_phase3;

NJS_TEXLIST final_texture_texlist = {
#include "stg10/texture/final_texture_texlist.inc"
};

/* final_texture_palettes' six palettes, 256 ARGB1555 colors each. they are
 * defined in the image's order, palette 2 first, at 0x0cb03c48 */
static Uint16 final_texture_palette2[256] = {
#include "stg10/texture/final_texture_palette2.inc"
};
static Uint16 final_texture_palette3[256] = {
#include "stg10/texture/final_texture_palette3.inc"
};
static Uint16 final_texture_palette4[256] = {
#include "stg10/texture/final_texture_palette4.inc"
};
static Uint16 final_texture_palette0[256] = {
#include "stg10/texture/final_texture_palette0.inc"
};
static Uint16 final_texture_palette1[256] = {
#include "stg10/texture/final_texture_palette1.inc"
};
static Uint16 final_texture_palette5[256] = {
#include "stg10/texture/final_texture_palette5.inc"
};

static Uint16 *final_texture_palettes[PALETTES] = {
    final_texture_palette0, final_texture_palette1, final_texture_palette2,
    final_texture_palette3, final_texture_palette4, final_texture_palette5,
};

static Float final_texture_panel_alpha = 1.0F;
static int final_texture_hold_count = 0;
/* the latest texture task to run its first step, or NULL once any task has
 * ended */
static struct task *final_texture_task = NULL;

void final_texture_panel_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case PANEL_INIT:
        task->draw = final_texture_panel_draw;
        task->destructor = final_texture_panel_destroy;
        final_texture_hold();
        work->mode = PANEL_RUN;
        break;
    case PANEL_RUN:
        final_texture_panel_draw(task);
        break;
    }
}

static void final_texture_panel_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        draw_attr_save();
        draw_attr_set(0, NJD_FLAG_USE_ALPHA);
        draw_set_material(final_texture_panel_alpha, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njPushMatrix(NULL);
        njSetTexture(&final_texture_texlist);
        njTranslateV(NULL, &work->pos);
        njRotateXYZ(NULL, work->ang.x, work->ang.y, work->ang.z);
        njScaleV(NULL, &work->scl);
        draw_tree(&final_texture_panel_object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
    }
}

static void final_texture_panel_destroy(struct task *task)
{
    final_texture_release();
}

static void final_texture_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case TEXTURE_INIT:
        final_texture_task = task;
        final_texture_init(task);
        TASK_TIMER(work) = stage_get_stage_act();
        break;
    case TEXTURE_RUN:
        if (stage_get_stage_act() != TASK_TIMER(work)) {
            task_kill(task);
            return;
        }
        if (frame_step == 0) {
            final_texture_paint(task);
            final_texture_reload();
        }
        break;
    case TEXTURE_SLEEP:
        break;
    }
}

void final_texture_start(void)
{
    task_create(TASK_WORK, 1, final_texture_step);
}

void final_texture_hold(void)
{
    /* @hack spelling: the two tests nested. SHC then loads final_texture_task
     * once, at 0x0c9119be, for the test and the store. as one test with &&,
     * SHC loads it again for the store, the function runs 4 bytes long and 14
     * of 15 words differ */
    if (final_texture_hold_count == 0) {
        if (final_texture_task != NULL) {
            final_texture_task->work->mode = TEXTURE_RUN;
        }
    }
    final_texture_hold_count++;
}

void final_texture_release(void)
{
    /* @hack spelling: the two tests nested, as in the hold, for the one load
     * of final_texture_task at 0x0c9119e0. as one test with &&, the function
     * runs 4 bytes long and 10 of 15 words differ */
    if (--final_texture_hold_count == 0) {
        if (final_texture_task != NULL) {
            final_texture_task->work->mode = TEXTURE_SLEEP;
        }
    }
}

static void final_texture_init(struct task *task)
{
    struct task_work *work = task->work;

    task->destructor = final_texture_destroy;
    final_texture_load();
    final_texture_make_sines();
    final_texture_reset(task);
    final_texture_paint(task);
    work->mode = TEXTURE_SLEEP;
}

static void final_texture_destroy(struct task *task)
{
    njReleaseTexture(&final_texture_texlist);
    final_texture_task = NULL;
}

static void final_texture_load(void)
{
    njSetTextureInfo(&final_texture_info, final_texture_pixels,
                     NJD_TEXFMT_RECTANGLE | NJD_TEXFMT_ARGB_1555, SIDE, SIDE);
    njSetTextureName(&final_texture_name, &final_texture_info, 1,
                     NJD_TEXATTR_TYPE_MEMORY | NJD_TEXATTR_GLOBALINDEX);
    njLoadTexture(&final_texture_texlist);
    njSetTexture(&final_texture_texlist);
}

static void final_texture_reload(void)
{
    njReleaseTexture(&final_texture_texlist);
    njLoadTexture(&final_texture_texlist);
    njSetTexture(&final_texture_texlist);
}

static void final_texture_make_sines(void)
{
    int i;
    Angle angle;

    for (i = 0, angle = 0; i < 256; i++, angle += 0x100) {
        final_texture_sines[i] = (Uint8)((njSin(angle) + 1.0F) * 32.0F);
    }
}

static void final_texture_reset(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->phase[0] = 0;
    VARS(work)->phase[1] = 0;
    VARS(work)->phase[2] = 0;
    VARS(work)->phase[3] = 0;
}

/* paint a plasma. each pixel is the entry, in the palette scale y picks, at
 * the sum of four waves' samples. waves 0 and 1 step down the rows, and waves
 * 2 and 3 along each row. each call moves the waves on */
static void final_texture_paint(struct task *task)
{
    struct task_work *work = task->work;
    int which = (int)work->scl.y;
    Uint16 *pixel = final_texture_pixels;
    Uint16 *palette;
    Uint8 phase0;
    Uint8 phase1;
    int row;
    int column;

    /* @bug memory: the test lets a palette index of 6 through, from a
     * scale y of 6 up to 7 (0x0c911baa). the load at 0x0c911bbe then takes
     * the word past final_texture_palettes, final_texture_panel_alpha's 1.0F,
     * as a palette's address */
    if (which > PALETTES || which < 0) {
        which = 0;
    }
    palette = final_texture_palettes[which];
    phase0 = VARS(work)->phase[0];
    phase1 = VARS(work)->phase[1];
    for (row = 0; row < SIDE; row++) {
        final_texture_phase2 = VARS(work)->phase[2];
        final_texture_phase3 = VARS(work)->phase[3];
        for (column = 0; column < SIDE; column++) {
            Uint8 index = final_texture_sines[phase0] +
                          final_texture_sines[phase1] +
                          final_texture_sines[final_texture_phase2] +
                          final_texture_sines[final_texture_phase3];

            *pixel++ = palette[index];
            final_texture_phase3 += 3;
            final_texture_phase2++;
        }
        phase0 += 2;
        phase1++;
    }
    VARS(work)->phase[0]++;
    VARS(work)->phase[1] -= 2;
    VARS(work)->phase[2] += 3;
    VARS(work)->phase[3] -= 4;
}
