/* CardUV: seven models of scenery on the pinball tables, whose UVs scroll */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/model.h"
#include "stg09/card_uv.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "system/frame.h"
#include "system/task.h"

/* the models include red and blue neon, red clouds and a waterfall */
extern NJS_MODEL card_uv_model_0;
extern NJS_MODEL card_uv_model_1;
extern NJS_MODEL card_uv_model_2;
extern NJS_MODEL card_uv_model_3;
extern NJS_MODEL card_uv_model_4;
extern NJS_MODEL card_uv_model_5;
extern NJS_MODEL card_uv_model_6;
extern NJS_MESHSET card_uv_meshset_0;
extern NJS_MESHSET card_uv_meshset_1;
extern NJS_MESHSET card_uv_meshset_2;
extern NJS_MESHSET card_uv_meshset_3;
extern NJS_MESHSET card_uv_meshset_4;
extern NJS_MESHSET card_uv_meshset_5;
extern NJS_MESHSET card_uv_meshset_6;

#define CARD_UV_MODELS 7
/* the UVs of the seven rows together, which card_uv_draw's room must hold */
#define CARD_UV_UVS 128
/* the step counts the phase round 0 to CARD_UV_PHASES - 1 */
#define CARD_UV_PHASES 4096

static Sint16 card_uv_phase = 0;
static NJS_POINT3 card_uv_places[CARD_UV_MODELS] = {
#include "stg09/card_uv/card_uv_places.inc"
};
static struct uv_scroll card_uv_scrolls[CARD_UV_MODELS] = {
#include "stg09/card_uv/card_uv_scrolls.inc"
};
static NJS_MODEL *card_uv_models[CARD_UV_MODELS] = {
    &card_uv_model_0, &card_uv_model_1, &card_uv_model_2, &card_uv_model_3,
    &card_uv_model_4, &card_uv_model_5, &card_uv_model_6,
};

static void card_uv_destroy(struct task *task)
{
    uv_scroll_release(card_uv_scrolls, CARD_UV_MODELS);
}

static void card_uv_draw(struct task *task)
{
    NJS_TEX room[CARD_UV_UVS];
    int i;

    if (!frame_step) {
        casino_set_texture(CASINO_TEXTURES_ACTS_34);
        uv_scroll_move(card_uv_scrolls, CARD_UV_MODELS, room, card_uv_phase);
        for (i = 0; i < CARD_UV_MODELS; i++) {
            njPushMatrix(NULL);
            njTranslate(NULL, card_uv_places[i].x, card_uv_places[i].y,
                        card_uv_places[i].z);
            draw_model(card_uv_models[i]);
            njPopMatrix(1);
        }
        uv_scroll_restore(card_uv_scrolls, CARD_UV_MODELS);
    }
}

static void card_uv_run(struct task *task)
{
    card_uv_phase = (card_uv_phase + 1) & (CARD_UV_PHASES - 1);
    card_uv_draw(task);
}

void card_uv_step(struct task *task)
{
    card_uv_phase = 0;
    uv_scroll_save(card_uv_scrolls, CARD_UV_MODELS);
    task->step = card_uv_run;
    task->draw = card_uv_draw;
    task->destructor = card_uv_destroy;
}
