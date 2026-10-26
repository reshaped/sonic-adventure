/* the gate combo: more points for each sand gate passed soon after the last.
 * its count shows at the screen's top right */
#include <ninja.h>
#include <sg_xpt.h>

#include "hud/hud.h"
#include "player/e102_bonus.h"
#include "sboard/act.h"
#include "sboard/gate_combo.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"

/* the combo's record, in task->motion */
struct gate_combo_record {
    Sint16 steps; /* left before the combo runs out */
    Sint16 count;
};

/* a gate passed adds one to the combo's count, up to COMBO_MAX, and scores
 * COMBO_POINTS times the count. each gate leaves the combo COMBO_STEPS steps
 * to run. the count goes back to 0 once they run out */
#define COMBO_MAX 50
#define COMBO_STEPS 180
#define COMBO_POINTS 10

/* the combo's screen position, and the spacing of its glyphs. a count of
 * one digit starts one glyph further right. its digit then lands where a
 * count of two digits puts its last */
#define COMBO_X 528.0F
#define COMBO_Y 32.0F
#define COMBO_SPACING 32.0F
#define COMBO_X_ONE_DIGIT (COMBO_X + COMBO_SPACING)
/* the times sign drawn before the count */
#define COMBO_TIMES E102TIME_LOCKON0

static void gate_combo_init(struct task *task, void *arg);
static void gate_combo_step(struct task *task);
static void gate_combo_draw(struct task *task);
static void gate_combo_destroy(struct task *task);
static void gate_combo_draw_count(int count);
static Float gate_combo_draw_glyph(Float x, Float y, Float pri, int glyph);

static const struct task_desc gate_combo_desc = {
    sizeof(struct gate_combo_record),
    0,
    gate_combo_init,
    gate_combo_step,
    gate_combo_draw,
    gate_combo_destroy};
/* the glyphs draw with a white constant material, and the draw then sets it
 * back to all 0 */
static const NJS_ARGB gate_combo_white = {1.0F, 1.0F, 1.0F, 1.0F};
static const NJS_ARGB gate_combo_none = {0.0F, 0.0F, 0.0F, 0.0F};
/* the glyphs take these priorities by turns, the times sign the first */
static const Float gate_combo_pri[2] = {-2.001F, -2.0F};

/* the last combo task's record while that task lives, or NULL */
static struct gate_combo_record *gate_combo = NULL;
/* the sprite's one texanim over e102time_texlist, whose texid each glyph's
 * draw sets */
static NJS_TEXANIM gate_combo_frame = {
#include "sboard/gate_combo/gate_combo_frame.inc"
};
static NJS_SPRITE gate_combo_sprite = {
#include "sboard/gate_combo/gate_combo_sprite.inc"
};

struct task *gate_combo_create(void)
{
    /* @hack spelling: the result through task, which keeps the call a JSR
     * (0x0c905726); returned directly, it becomes a tail jump to
     * act_task_create, and 8 of 8 words differ */
    struct task *task = act_task_create(&gate_combo_desc, NULL);

    return task;
}

void gate_combo_add(void)
{
    struct gate_combo_record *record = gate_combo;

    if (record == NULL) {
        score_add(COMBO_POINTS);
        return;
    }
    record->count++;
    if (record->count > COMBO_MAX) {
        record->count = COMBO_MAX;
    }
    score_add(record->count * COMBO_POINTS);
    record->steps = COMBO_STEPS;
}

static void gate_combo_init(struct task *task, void *arg)
{
    struct gate_combo_record *record = (struct gate_combo_record *)task->motion;

    record->steps = 0;
    record->count = 0;
    gate_combo = record;
}

/* count the combo's steps down, and set its count back to 0 once they have
 * run out. then draw the combo */
static void gate_combo_step(struct task *task)
{
    struct gate_combo_record *record = (struct gate_combo_record *)task->motion;

    if (record->steps > 0) {
        record->steps--;
    } else {
        record->count = 0;
    }
    gate_combo_draw(task);
}

/* on a frame's first step, draw a count above 0. nothing draws while
 * hud_hidden is below 0 */
static void gate_combo_draw(struct task *task)
{
    struct gate_combo_record *record = (struct gate_combo_record *)task->motion;

    if (frame_step == 0 && hud_hidden >= 0 && record->count > 0) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njTextureFilterMode(NJD_TEXTUREFILTER_POINT_SAMPLE);
        njSetConstantMaterial((NJS_ARGB *)&gate_combo_white);
        gate_combo_draw_count(record->count);
        njSetConstantMaterial((NJS_ARGB *)&gate_combo_none);
        njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    }
}

/* forget the record only while gate_combo holds it, so that a newer combo's
 * stays */
static void gate_combo_destroy(struct task *task)
{
    struct gate_combo_record *record = (struct gate_combo_record *)task->motion;

    if (gate_combo == record) {
        gate_combo = NULL;
    }
}

/* draw the times sign, then the count's one or two digits. the last digit
 * always lands at the same x */
static void gate_combo_draw_count(int count)
{
    Float x = COMBO_X;
    Float y = COMBO_Y;
    int last;

    if (count < 10) {
        x = COMBO_X_ONE_DIGIT;
        x += gate_combo_draw_glyph(x, y, gate_combo_pri[0], COMBO_TIMES);
        last = 1;
    } else {
        x += gate_combo_draw_glyph(x, y, gate_combo_pri[0], COMBO_TIMES);
        x += gate_combo_draw_glyph(x, y, gate_combo_pri[1], count / 10);
        last = 0;
    }
    gate_combo_draw_glyph(x, y, gate_combo_pri[last], count % 10);
}

/* draw e102time_texlist's texture glyph at x and y, and return the spacing
 * to the next glyph's x */
static Float gate_combo_draw_glyph(Float x, Float y, Float pri, int glyph)
{
    gate_combo_frame.texid = glyph;
    gate_combo_sprite.p.x = x;
    gate_combo_sprite.p.y = y;
    njDrawSprite2D(&gate_combo_sprite, 0, pri, NJD_SPRITE_ALPHA);
    return COMBO_SPACING;
}
