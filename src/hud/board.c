/* the board stages' hud: a score under its own label, where the time
 * would be */
#include <ninja.h>
#include <sg_xpt.h>

#include "hud/board.h"
#include "main.h"
#include "stage/stage.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */

#define TEXT_1_LENGTH 13
#include "hud/board/text.inc"

/* the most the score shows, in its five digits */
#define SCORE_MAX 99999
/* the step in x from one digit to the next */
#define DIGIT_WIDTH 16.0F

static Float draw_score_digit(Float x, Float y, Float depth, Sint16 digit);

/* draw_score_digit rewrites these for each digit */
static NJS_TEXANIM score_digit_texanim = {
#include "hud/board/score_digit_texanim.inc"
};
static NJS_SPRITE score_digit_sprite = {
#include "hud/board/score_digit_sprite.inc"
};
static int dat_8c1da8d4 = 0;
static NJS_TEXNAME score_texnames[] = {
    {TEXT_1, 0, 0},
};
NJS_TEXLIST board_score_texlist = {
#include "hud/board/board_score_texlist.inc"
};

/* const, so that these sit in the constant section, as the image places
 * them. ninja's prototypes take no const, so the calls cast it away */

/* the depths the digits alternate between */
static const Float score_digit_depths[2] = {-2.001F, -2.0F};
static const NJS_ARGB score_color = {1.0F, 1.0F, 1.0F, 1.0F};
static const NJS_ARGB score_no_color = {0.0F, 0.0F, 0.0F, 0.0F};
static const NJS_TEXANIM score_label_texanims[1] = {
#include "hud/board/score_label_texanims.inc"
};
static const NJS_SPRITE score_label_sprite = {
#include "hud/board/score_label_sprite.inc"
};
static const NJS_TEXANIM rings_label_texanims[1] = {
#include "hud/board/rings_label_texanims.inc"
};
static const NJS_SPRITE rings_label_sprite = {
#include "hud/board/rings_label_sprite.inc"
};

void hud_draw_board_score(int score)
{
    Float x;
    Float y;
    int value;
    int place;
    int depth;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njTextureFilterMode(NJD_TEXTUREFILTER_POINT_SAMPLE);
    njSetConstantMaterial((NJS_ARGB *)&score_color);
    x = 92.0F;
    y = 33.0F;
    value = score;
    if (value > SCORE_MAX) {
        value = SCORE_MAX;
    }
    place = 10000;
    depth = 0;
    do {
        x += draw_score_digit(x, y, score_digit_depths[depth], value / place);
        value %= place;
        place /= 10;
        depth ^= 1;
    } while (place > 0);
    njSetConstantMaterial((NJS_ARGB *)&score_no_color);
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
}

void hud_draw_board_labels(void)
{
    njDrawSprite2D((NJS_SPRITE *)&score_label_sprite, 0, -2.001F,
                   NJD_SPRITE_ALPHA);
    njDrawSprite2D((NJS_SPRITE *)&rings_label_sprite, 0, -2.0F,
                   NJD_SPRITE_ALPHA);
}

Bool board_active(void)
{
    Bool active = FALSE;
    int stage_act = stage_get_stage_act();
    Uint8 stage = STAGE_OF(stage_act);

    if (stage == STAGE_SUNABOARD ||
        (stage == STAGE_ICECAP && (Uint8)stage_act == 2 &&
         game_mode == GAME_MODE_TRIAL)) {
        active = TRUE;
    }
    return active;
}

Bool cart_active(void)
{
    Bool active = FALSE;
    int stage_act = stage_get_stage_act();
    Uint8 stage = STAGE_OF(stage_act);

    if (stage == STAGE_CART) {
        active = TRUE;
    }
    return active;
}

/* draw digit at x, y and depth. returns the step in x to the next digit */
static Float draw_score_digit(Float x, Float y, Float depth, Sint16 digit)
{
    score_digit_texanim.texid = digit;
    score_digit_sprite.p.x = x;
    score_digit_sprite.p.y = y;
    njDrawSprite2D(&score_digit_sprite, 0, depth, NJD_SPRITE_ALPHA);
    return DIGIT_WIDTH;
}
