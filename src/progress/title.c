/* the title: its picture, the cards that choose one of six characters, each
 * character's guide, and a debug menu */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "debug/debug.h"
#include "draw/draw.h"
#include "hud/hud.h"
#include "main.h"
#include "player/player.h"
#include "progress/demo.h"
#include "progress/progress.h"
#include "progress/recorder.h"
#include "progress/title.h"
#include "save/mail.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/history.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texlists.h"
#include "system/texture.h"
#include "system/timer.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 11
#define TEXT_4_LENGTH 11
#define TEXT_5_LENGTH 11
#define TEXT_6_LENGTH 11
#define TEXT_7_LENGTH 13
#define TEXT_8_LENGTH 13
#define TEXT_9_LENGTH 13
#define TEXT_10_LENGTH 13
#define TEXT_11_LENGTH 13
#define TEXT_12_LENGTH 13
#define TEXT_13_LENGTH 13
#define TEXT_14_LENGTH 13
#define TEXT_15_LENGTH 13
#define TEXT_16_LENGTH 13
#define TEXT_17_LENGTH 13
#define TEXT_18_LENGTH 13
#define TEXT_19_LENGTH 13
#define TEXT_20_LENGTH 13
#define TEXT_21_LENGTH 13
#define TEXT_22_LENGTH 13
#define TEXT_23_LENGTH 13
#define TEXT_24_LENGTH 13
#define TEXT_25_LENGTH 13
#define TEXT_26_LENGTH 13
#define TEXT_27_LENGTH 13
#define TEXT_28_LENGTH 13
#define TEXT_29_LENGTH 13
#define TEXT_30_LENGTH 13
#define TEXT_31_LENGTH 12
#define TEXT_32_LENGTH 12
#define TEXT_33_LENGTH 12
#define TEXT_34_LENGTH 12
#define TEXT_35_LENGTH 12
#define TEXT_36_LENGTH 12
#define TEXT_37_LENGTH 11
#define TEXT_38_LENGTH 11
#define TEXT_39_LENGTH 11
#define TEXT_40_LENGTH 11
#define TEXT_41_LENGTH 11
#define TEXT_42_LENGTH 11
#define TEXT_43_LENGTH 12
#define TEXT_44_LENGTH 12
#define TEXT_45_LENGTH 12
#define TEXT_46_LENGTH 12
#define TEXT_47_LENGTH 12
#define TEXT_48_LENGTH 12
#define TEXT_49_LENGTH 11
#define TEXT_50_LENGTH 11
#define TEXT_51_LENGTH 11
#define TEXT_52_LENGTH 11
#define TEXT_53_LENGTH 11
#define TEXT_54_LENGTH 11
#define TEXT_55_LENGTH 12
#define TEXT_56_LENGTH 12
#define TEXT_57_LENGTH 12
#define TEXT_58_LENGTH 12
#define TEXT_59_LENGTH 12
#define TEXT_60_LENGTH 12
#define TEXT_61_LENGTH 11
#define TEXT_62_LENGTH 11
#define TEXT_63_LENGTH 11
#define TEXT_64_LENGTH 11
#define TEXT_65_LENGTH 11
#define TEXT_66_LENGTH 11
#define TEXT_67_LENGTH 12
#define TEXT_68_LENGTH 12
#define TEXT_69_LENGTH 12
#define TEXT_70_LENGTH 12
#define TEXT_71_LENGTH 12
#define TEXT_72_LENGTH 12
#define TEXT_73_LENGTH 11
#define TEXT_74_LENGTH 11
#define TEXT_75_LENGTH 11
#define TEXT_76_LENGTH 11
#define TEXT_77_LENGTH 11
#define TEXT_78_LENGTH 11
#define TEXT_79_LENGTH 12
#define TEXT_80_LENGTH 12
#define TEXT_81_LENGTH 12
#define TEXT_82_LENGTH 12
#define TEXT_83_LENGTH 12
#define TEXT_84_LENGTH 12
#define TEXT_85_LENGTH 11
#define TEXT_86_LENGTH 11
#define TEXT_87_LENGTH 11
#define TEXT_88_LENGTH 11
#define TEXT_89_LENGTH 11
#define TEXT_90_LENGTH 11
#define TEXT_91_LENGTH 12
#define TEXT_92_LENGTH 12
#define TEXT_93_LENGTH 12
#define TEXT_94_LENGTH 12
#define TEXT_95_LENGTH 12
#define TEXT_96_LENGTH 12
#define TEXT_97_LENGTH 11
#define TEXT_98_LENGTH 11
#define TEXT_99_LENGTH 11
#define TEXT_100_LENGTH 11
#define TEXT_101_LENGTH 11
#define TEXT_102_LENGTH 11
#include "progress/title/text.inc"

/* title_step's modes. TITLE_X_TO_Y fades X's screen out to white, then
 * sets the mode that shows Y, which fades Y in from white.
 * TITLE_SHOW_TO_DEMO and TITLE_RULES_TO_START fade the loading screen in
 * instead, then go to TITLE_DEMO and TITLE_START. the debug menu uses
 * TITLE_SHOW, TITLE_CARDS, TITLE_START and TITLE_QUIT, with TITLE_CARDS as
 * its character column */
enum title_mode {
    TITLE_DONE = 0, /* set with a result. a step here goes to TITLE_QUIT */
    TITLE_SHOW = 1, /* the picture, until start */
    TITLE_SHOW_TO_DEMO = 2,
    TITLE_SHOW_TO_CARDS = 3,
    TITLE_CARDS_TO_SHOW = 4, /* once the cards time out */
    TITLE_CARDS_TO_GUIDE = 5,
    TITLE_CONTROLS_TO_RULES = 6,
    TITLE_SHOW_TO_GUIDE = 7, /* past the cards, to title_preset's guide */
    TITLE_RULES_TO_START = 8,
    TITLE_CARDS = 9, /* the cards, which choose a character */
    TITLE_DEBUG_STAGES = 10,
    TITLE_DEBUG_STORIES = 11,
    TITLE_START = 12, /* start the story or the stage picked */
    /* Station Square's Chao garden. no code sets this mode */
    TITLE_GARDEN = 13,
    TITLE_QUIT = 14, /* to the stage select */
    TITLE_DEMO = 15, /* start the next demo */
    TITLE_CONTROLS = 16, /* a guide's first page */
    TITLE_RULES = 17 /* a guide's second page */
};

/* the characters to choose, in the order the cards show them */
enum title_choice {
    CHOICE_SONIC,
    CHOICE_TAILS,
    CHOICE_AMY,
    CHOICE_E102,
    CHOICE_BIG,
    CHOICE_KNUCKLES,
    CHOICES
};

/* the picture, and each page of a guide, in six pieces, three across and
 * two down */
#define TITLE_PIECES 6

/* each choice's card, in four pieces, two across and two down */
#define CARD_PIECES 4
/* how much each draw of the cards fades one that isn't chosen */
#define CARD_FADE 0.2F

/* title_timer counts up to SHOW_FRAMES and CARDS_FRAMES, and down from
 * PAGE_FRAMES. the picture shows for SHOW_FRAMES + 1 frames before a demo.
 * the cards go back to the picture after CARDS_FRAMES + 1 frames without a
 * move. each page of a guide shows for PAGE_FRAMES + 1 frames, unless a or
 * start chooses first */
#define SHOW_FRAMES 150
#define CARDS_FRAMES 600
#define PAGE_FRAMES 900

#define FADE_SPEED 24 /* how far a fade moves each frame */
#define FADE_END (-30) /* a fade out ends below this level, past black */

/* the frames before the title's music starts */
#define MUSIC_WAIT 10

/* the voices a press of start plays on the picture while a is held.
 * VOICE_LEAVING plays as the title writes its mail to the card and leaves
 * for LOADER.BIN. VOICE_NO_SETTINGS plays when the console holds no
 * internet settings to send the mail with */
#define VOICE_LEAVING 0x497
#define VOICE_NO_SETTINGS 0x499

#define DEBUG_STAGES 76 /* the rows of title_debug_stages */
#define DEBUG_ROWS 13 /* the stage rows the menu shows at once */
#define DEBUG_ROWS_ABOVE 5 /* of those, the rows above the pick */
#define DEBUG_CHARACTERS 6 /* the menu's characters, and its stories */
#define DEBUG_DIM 0.4F /* the brightness the character column dims to */
#define DEBUG_DIM_STEP 0.02F /* how much it dims each frame */

/* the debug menu's print colors. COLUMN_PICK prints a column's pick and
 * COLUMN_TEXT its other rows, each _DIM in the columns not in use. each of
 * the menu's draws leaves COLOR_WHITE set */
#define COLUMN_PICK 0xff00ff00
#define COLUMN_TEXT 0xffdfdfdf
#define COLUMN_PICK_DIM 0xff00bf00
#define COLUMN_TEXT_DIM 0xff7f7f7f
#define COLOR_ARROW 0xffff0000
#define COLOR_WHITE 0xffffffff

/* the arrows hide while this bit of title_debug_frames is set */
#define ARROW_BLINK 0x10

static void title_fade_draw_white(void);
static void title_draw_loading(void);
static Sint16 title_debug_step(void);

/* @hack type: an enum title_mode held unsigned, which keeps the 1 that
 * title_setup stores in R13 for njPopMatrix(1) (MOV R13,R4 at 0x8c09fa60);
 * as an int, 101 of title_setup's 103 words differ, and the functions that
 * read it build the same */
static Uint32 title_mode;
/* no code names it, but it holds its place in bss */
static int dat_8c7a97d4;
/* TRUE runs the debug menu in the title's place. title_setup sets it FALSE,
 * and no code sets it TRUE */
static Bool title_debug;
/* title_setup sets them to 1 and 0, and no code reads them */
static int dat_8c7a97dc;
static int dat_8c7a97e0;
static Sint8 title_debug_stage;
/* a CHOICE_, or in the debug menu an index to title_debug_characters */
static Sint8 title_choice;
static Sint8 title_music_wait;
/* the frames a mode has shown, or has left to show */
static int title_timer;
/* each card's alpha: 1 while it's chosen. once it isn't, each draw takes
 * CARD_FADE from it, down to 0 */
static Float title_card_alpha[CHOICES];
/* the task that blinks PRESS START over a guide. title_setup sets it to
 * NULL, and killing the task leaves it set */
static struct task *title_press_start;
/* title_setup works out a point into it, and no code reads it */
static NJS_POINT3 dat_8c7a9808;
static Sint8 title_debug_story;
/* the picture's brightness behind the debug menu. title_debug_reset sets
 * it to 1, and the character column dims it to DEBUG_DIM */
static Float title_debug_brightness;
static int title_debug_frames;
static int title_fade_level;

static NJS_TEXNAME title_textures[TITLE_PIECES] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
};
static NJS_TEXLIST title_texlist = {
#include "progress/title/title_texlist.inc"
};
/* the cards of Sonic, Tails and Knuckles */
static NJS_TEXNAME c_select1_textures[12] = {
    {TEXT_7, 0, 0},  {TEXT_8, 0, 0},  {TEXT_9, 0, 0},  {TEXT_10, 0, 0},
    {TEXT_11, 0, 0}, {TEXT_12, 0, 0}, {TEXT_13, 0, 0}, {TEXT_14, 0, 0},
    {TEXT_15, 0, 0}, {TEXT_16, 0, 0}, {TEXT_17, 0, 0}, {TEXT_18, 0, 0},
};
static NJS_TEXLIST c_select1_texlist = {
#include "progress/title/c_select1_texlist.inc"
};
/* the cards of E-102, Amy and Big */
static NJS_TEXNAME c_select2_textures[12] = {
    {TEXT_19, 0, 0}, {TEXT_20, 0, 0}, {TEXT_21, 0, 0}, {TEXT_22, 0, 0},
    {TEXT_23, 0, 0}, {TEXT_24, 0, 0}, {TEXT_25, 0, 0}, {TEXT_26, 0, 0},
    {TEXT_27, 0, 0}, {TEXT_28, 0, 0}, {TEXT_29, 0, 0}, {TEXT_30, 0, 0},
};
static NJS_TEXLIST c_select2_texlist = {
#include "progress/title/c_select2_texlist.inc"
};
/* each character's guide: the six pieces of its controls page, then the
 * six of its rules page */
static NJS_TEXNAME tgs_sousa_s_textures[12] = {
    {TEXT_31, 0, 0}, {TEXT_32, 0, 0}, {TEXT_33, 0, 0}, {TEXT_34, 0, 0},
    {TEXT_35, 0, 0}, {TEXT_36, 0, 0}, {TEXT_37, 0, 0}, {TEXT_38, 0, 0},
    {TEXT_39, 0, 0}, {TEXT_40, 0, 0}, {TEXT_41, 0, 0}, {TEXT_42, 0, 0},
};
static NJS_TEXLIST tgs_sousa_s_texlist = {
#include "progress/title/tgs_sousa_s_texlist.inc"
};
static NJS_TEXNAME tgs_sousa_t_textures[12] = {
    {TEXT_43, 0, 0}, {TEXT_44, 0, 0}, {TEXT_45, 0, 0}, {TEXT_46, 0, 0},
    {TEXT_47, 0, 0}, {TEXT_48, 0, 0}, {TEXT_49, 0, 0}, {TEXT_50, 0, 0},
    {TEXT_51, 0, 0}, {TEXT_52, 0, 0}, {TEXT_53, 0, 0}, {TEXT_54, 0, 0},
};
static NJS_TEXLIST tgs_sousa_t_texlist = {
#include "progress/title/tgs_sousa_t_texlist.inc"
};
static NJS_TEXNAME tgs_sousa_k_textures[12] = {
    {TEXT_55, 0, 0}, {TEXT_56, 0, 0}, {TEXT_57, 0, 0}, {TEXT_58, 0, 0},
    {TEXT_59, 0, 0}, {TEXT_60, 0, 0}, {TEXT_61, 0, 0}, {TEXT_62, 0, 0},
    {TEXT_63, 0, 0}, {TEXT_64, 0, 0}, {TEXT_65, 0, 0}, {TEXT_66, 0, 0},
};
static NJS_TEXLIST tgs_sousa_k_texlist = {
#include "progress/title/tgs_sousa_k_texlist.inc"
};
static NJS_TEXNAME tgs_sousa_e_textures[12] = {
    {TEXT_67, 0, 0}, {TEXT_68, 0, 0}, {TEXT_69, 0, 0}, {TEXT_70, 0, 0},
    {TEXT_71, 0, 0}, {TEXT_72, 0, 0}, {TEXT_73, 0, 0}, {TEXT_74, 0, 0},
    {TEXT_75, 0, 0}, {TEXT_76, 0, 0}, {TEXT_77, 0, 0}, {TEXT_78, 0, 0},
};
static NJS_TEXLIST tgs_sousa_e_texlist = {
#include "progress/title/tgs_sousa_e_texlist.inc"
};
static NJS_TEXNAME tgs_sousa_b_textures[12] = {
    {TEXT_79, 0, 0}, {TEXT_80, 0, 0}, {TEXT_81, 0, 0}, {TEXT_82, 0, 0},
    {TEXT_83, 0, 0}, {TEXT_84, 0, 0}, {TEXT_85, 0, 0}, {TEXT_86, 0, 0},
    {TEXT_87, 0, 0}, {TEXT_88, 0, 0}, {TEXT_89, 0, 0}, {TEXT_90, 0, 0},
};
static NJS_TEXLIST tgs_sousa_b_texlist = {
#include "progress/title/tgs_sousa_b_texlist.inc"
};
static NJS_TEXNAME tgs_sousa_a_textures[12] = {
    {TEXT_91, 0, 0}, {TEXT_92, 0, 0},  {TEXT_93, 0, 0},  {TEXT_94, 0, 0},
    {TEXT_95, 0, 0}, {TEXT_96, 0, 0},  {TEXT_97, 0, 0},  {TEXT_98, 0, 0},
    {TEXT_99, 0, 0}, {TEXT_100, 0, 0}, {TEXT_101, 0, 0}, {TEXT_102, 0, 0},
};
static NJS_TEXLIST tgs_sousa_a_texlist = {
#include "progress/title/tgs_sousa_a_texlist.inc"
};

/* the choice that start takes straight to its guide, past the cards, or -1
 * for none. choosing a card with x and y held sets it, and no code clears
 * it */
static int title_preset = -1;
/* six stages and acts that no code reads */
static Uint16 dat_8c1d9e8c[6] = {
    STAGE_ACT(STAGE_HIGHWAY, 0),  STAGE_ACT(STAGE_MOUNTAIN, 0),
    STAGE_ACT(STAGE_CHAOS0, 0),   STAGE_ACT(STAGE_HIGHWAY, 2),
    STAGE_ACT(STAGE_MOUNTAIN, 0), STAGE_ACT(STAGE_CHAOS2, 0),
};
static Uint16 title_characters[CHOICES] = {
    CHARACTER_SONIC, CHARACTER_TAILS, CHARACTER_AMY,
    CHARACTER_E102,  CHARACTER_BIG,   CHARACTER_KNUCKLES,
};
/* the picture's pieces, each flipped left to right and laid right to left,
 * so the picture shows its textures mirrored */
static NJS_TEXANIM title_texanims[TITLE_PIECES] = {
#include "progress/title/title_texanims.inc"
};
static NJS_TEXANIM title_card_texanims[CHOICES * CARD_PIECES] = {
#include "progress/title/title_card_texanims.inc"
};
static char *title_guide_files[CHOICES] = {
    "TGS_SOUSA_S", "TGS_SOUSA_T", "TGS_SOUSA_A",
    "TGS_SOUSA_E", "TGS_SOUSA_B", "TGS_SOUSA_K",
};
static NJS_TEXLIST *title_guide_texlists[CHOICES] = {
    &tgs_sousa_s_texlist, &tgs_sousa_t_texlist, &tgs_sousa_a_texlist,
    &tgs_sousa_e_texlist, &tgs_sousa_b_texlist, &tgs_sousa_k_texlist,
};
static NJS_TEXANIM title_controls_texanims[TITLE_PIECES] = {
#include "progress/title/title_controls_texanims.inc"
};
static NJS_TEXANIM title_rules_texanims[TITLE_PIECES] = {
#include "progress/title/title_rules_texanims.inc"
};
static NJS_TEXANIM title_press_start_texanim[1] = {
#include "progress/title/title_press_start_texanim.inc"
};
/* nonzero once start leaves the picture. TITLE_SHOW then fades the picture
 * in from white, not black */
static int title_fade_white = 0;
static int title_guide_music[CHOICES] = {
    MUSIC_ADVSONIC, MUSIC_ADVMILES, MUSIC_ADVAMY,
    MUSIC_ADVE102,  MUSIC_ADVBIG,   MUSIC_ADVKNKLS,
};
/* the debug menu's stages and acts. its last three rows are zeros, which
 * the menu lists as STAGE_PRACTICE's act 0 */
static Uint8 title_debug_stages[DEBUG_STAGES][2] = {
    {STAGE_PRACTICE, 0},
    {STAGE_PRACTICE, 1},
    {STAGE_PRACTICE, 2},
    {STAGE_BEACH, 0},
    {STAGE_BEACH, 1},
    {STAGE_BEACH, 2},
    {STAGE_WINDY, 0},
    {STAGE_WINDY, 1},
    {STAGE_WINDY, 2},
    {STAGE_TWINKLE, 0},
    {STAGE_TWINKLE, 1},
    {STAGE_TWINKLE, 2},
    {STAGE_HIGHWAY, 0},
    {STAGE_HIGHWAY, 1},
    {STAGE_HIGHWAY, 2},
    {STAGE_MOUNTAIN, 0},
    {STAGE_MOUNTAIN, 1},
    {STAGE_MOUNTAIN, 2},
    {STAGE_SKY_DECK, 0},
    {STAGE_SKY_DECK, 1},
    {STAGE_SKY_DECK, 2},
    {STAGE_RUIN, 0},
    {STAGE_RUIN, 1},
    {STAGE_RUIN, 2},
    {STAGE_ICECAP, 0},
    {STAGE_ICECAP, 1},
    {STAGE_ICECAP, 2},
    {STAGE_ICECAP, 3},
    {STAGE_CASINO, 0},
    {STAGE_CASINO, 1},
    {STAGE_CASINO, 2},
    {STAGE_CASINO, 3},
    {STAGE_FINAL, 0},
    {STAGE_FINAL, 1},
    {STAGE_FINAL, 2},
    {STAGE_CHAOS0, 0},
    {STAGE_CHAOS2, 0},
    {STAGE_CHAOS4, 0},
    {STAGE_EGGMOBILE1, 0},
    {STAGE_EGGMANROBO, 0},
    {STAGE_STATION_SQUARE, 0},
    {STAGE_STATION_SQUARE, 1},
    {STAGE_STATION_SQUARE, 2},
    {STAGE_STATION_SQUARE, 3},
    {STAGE_STATION_SQUARE, 4},
    {STAGE_STATION_SQUARE, 5},
    {STAGE_EGG_CARRIER, 0},
    {STAGE_EGG_CARRIER, 1},
    {STAGE_EGG_CARRIER, 2},
    {STAGE_EGG_CARRIER, 3},
    {STAGE_EGG_CARRIER, 4},
    {STAGE_EGG_CARRIER, 5},
    {STAGE_EGG_CARRIER_INSIDE, 0},
    {STAGE_EGG_CARRIER_INSIDE, 1},
    {STAGE_EGG_CARRIER_INSIDE, 2},
    {STAGE_EGG_CARRIER_INSIDE, 3},
    {STAGE_EGG_CARRIER_INSIDE, 4},
    {STAGE_EGG_CARRIER_INSIDE, 5},
    {STAGE_MYSTIC_RUINS, 0},
    {STAGE_MYSTIC_RUINS, 1},
    {STAGE_MYSTIC_RUINS, 2},
    {STAGE_MYSTIC_RUINS, 3},
    {STAGE_PAST, 0},
    {STAGE_PAST, 1},
    {STAGE_PAST, 2},
    {STAGE_CART, 0},
    {STAGE_SHOOTING1, 0},
    {STAGE_SHOOTING2, 0},
    {STAGE_SUNABOARD, 0},
    {STAGE_ALIFE_SS, 0},
    {STAGE_ALIFE_EG, 0},
    {STAGE_ALIFE_MR, 0},
    {STAGE_ALIFE_RACE, 0},
    {0, 0},
    {0, 0},
    {0, 0},
};
/* the debug menu's characters, in an order of its own, which its story
 * column follows too */
static char title_debug_names[DEBUG_CHARACTERS][10] = {
    "SONIC", "MILES", "KNUCKLES", "BIG", "AMY", "E-102",
};
static Uint16 title_debug_characters[DEBUG_CHARACTERS] = {
    CHARACTER_SONIC, CHARACTER_TAILS, CHARACTER_KNUCKLES,
    CHARACTER_BIG,   CHARACTER_AMY,   CHARACTER_E102,
};
/* title_debug_reset clears it, and no code reads it */
static int dat_8c1da32c = 0;
/* the loading screen's pieces: the two halves of its picture, then four
 * that stretch over the screen to its left, right, top and bottom. no code
 * draws the seventh, centered piece */
static NJS_TEXANIM title_loading_texanims[7] = {
#include "progress/title/title_loading_texanims.inc"
};

static void title_draw(void)
{
    NJS_SPRITE sprite;
    int i;

    sprite.tlist = &title_texlist;
    sprite.tanim = title_texanims;
    njSetTexture(sprite.tlist);
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    sprite.p.x = SCREEN_MIDDLE_X;
    sprite.p.y = 0.0F;
    for (i = 0; i < TITLE_PIECES; i++) {
        njDrawSprite2D(&sprite, i, -10.0F, 0);
    }
}

/* draw the cards, each over the whole screen: the chosen one, and in front
 * of it the others as they fade out. each call moves title_card_alpha */
static void title_draw_cards(void)
{
    NJS_SPRITE sprite;
    int i;
    int j;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    for (i = 0; i < CHOICES; i++) {
        if (title_choice == i) {
            title_card_alpha[i] = 1.0F;
        } else {
            title_card_alpha[i] -= CARD_FADE;
            if (title_card_alpha[i] < 0.0F) {
                title_card_alpha[i] = 0.0F;
            }
        }
        if (i == CHOICE_SONIC || i == CHOICE_TAILS || i == CHOICE_KNUCKLES) {
            sprite.tlist = &c_select1_texlist;
        } else {
            sprite.tlist = &c_select2_texlist;
        }
        sprite.tanim = title_card_texanims;
        njSetTexture(sprite.tlist);
        draw_set_material(title_card_alpha[i], 1.0F, 1.0F, 1.0F);
        for (j = 0; j < CARD_PIECES; j++) {
            sprite.sx = 1.25F;
            sprite.sy = 1.0F;
            sprite.p.x = SCREEN_MIDDLE_X;
            sprite.p.y = 0.0F;
            njDrawSprite2D(&sprite, (i * CARD_PIECES) + j,
                           -10.0F - title_card_alpha[i],
                           NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        }
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

static void title_draw_controls(void)
{
    NJS_SPRITE sprite;
    int i;

    sprite.tlist = title_guide_texlists[title_choice];
    sprite.tanim = title_controls_texanims;
    njSetTexture(sprite.tlist);
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    sprite.p.x = SCREEN_MIDDLE_X;
    sprite.p.y = 0.0F;
    for (i = 0; i < TITLE_PIECES; i++) {
        njDrawSprite2D(&sprite, i, -10.0F, 0);
    }
}

static void title_draw_rules(void)
{
    NJS_SPRITE sprite;
    int i;

    sprite.tlist = title_guide_texlists[title_choice];
    sprite.tanim = title_rules_texanims;
    njSetTexture(sprite.tlist);
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    sprite.p.x = SCREEN_MIDDLE_X;
    sprite.p.y = 0.0F;
    for (i = 0; i < TITLE_PIECES; i++) {
        njDrawSprite2D(&sprite, i, -10.0F, 0);
    }
}

/* the press start task keeps its alpha in scl.z */
#define PRESS_START_ALPHA(work) ((work)->scl.z)
/* the step of the blink's phase, TASK_TIMER(work), each frame: one blink
 * takes 32 frames */
#define PRESS_START_BLINK 0x800

static void title_press_start_step(struct task *task)
{
    NJS_SPRITE sprite;
    NJS_ARGB color;
    struct task_work *work = task->work;

    sprite.tlist = &press_start_texlist;
    sprite.tanim = title_press_start_texanim;
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    sprite.p.x = 512.0F;
    sprite.p.y = 448.0F;
    TASK_TIMER(work) += PRESS_START_BLINK;
    PRESS_START_ALPHA(work) = (njSin(TASK_TIMER(work)) * 0.5F) + 0.5F;
    njSetTexture(&press_start_texlist);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    color.a = PRESS_START_ALPHA(work);
    color.r = 1.0F;
    color.g = 1.0F;
    color.b = 1.0F;
    njSetConstantMaterial(&color);
    njDrawSprite2D(&sprite, 0, -1.0F, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    color.a = 0.0F;
    color.r = 0.0F;
    color.g = 0.0F;
    color.b = 0.0F;
    njSetConstantMaterial(&color);
}

static void title_press_start_destroy(struct task *task)
{
    njReleaseTexture(&press_start_texlist);
}

void title_setup(void)
{
    NJS_POINT3 point;

    menu_begin();
    title_debug = FALSE;
    draw_off = TRUE;
    title_mode = TITLE_SHOW;
    heap_init();
    task_init();
    stream_destroy();
    title_press_start = NULL;
    title_fade_white = 0;
    dat_8c7a97dc = 1;
    dat_8c7a97e0 = 0;
    frame_pacing_init(2, 2);
    title_fade_set(0);
    njSetBackColor(0xff000000, 0xff000000, 0xff000000);
    texture_load_pvm("TITLE", &title_texlist);
    title_debug_stage = 0;
    title_choice = 0;
    title_music_wait = 0;
    title_timer = 0;
    recording_kind = RECORDING_OFF;
    sound_load_stage();
    point.x = -6613.86F;
    point.y = -10572.42F;
    point.z = -1697.97F;
    njPushMatrix(&_unit_matrix_);
    njTranslate(NULL, -1482.63F, -4582.89F, 5921.56F);
    njRotateY(NULL, -12683);
    njCalcPoint(NULL, &point, &dat_8c7a9808);
    njPopMatrix(1);
}

/* move the choice with left and right or the triggers, wrapping at the
 * ends. a move restarts title_timer */
static void title_move_choice(void)
{
    int step = 0;

    if (pad_press_live[0] & (PDD_DGT_TL | PDD_DGT_KL)) {
        step = -1;
        sound_play(SE_MOVE, 0, 0, 0);
        title_timer = 0;
    }
    if (pad_press_live[0] & (PDD_DGT_TR | PDD_DGT_KR)) {
        step = 1;
        sound_play(SE_MOVE, 0, 0, 0);
        title_timer = 0;
    }
    title_choice += step;
    if (title_choice >= CHOICES) {
        title_choice = 0;
    } else if (title_choice < 0) {
        title_choice = CHOICES - 1;
    }
}

/* set the choice's stage and act, to play as a demo of recorded pads. with
 * a held, the recorder plays the recording at DEMO_RECORDING back. with b
 * held, even with a, it records one there */
static void title_set_stage(void)
{
    Uint8 stage;
    Uint8 act;

    if (title_choice == CHOICE_BIG) {
        stage = STAGE_BEACH;
        act = 2;
    } else if (title_choice == CHOICE_AMY) {
        stage = STAGE_SHELTER;
        act = 0;
    } else if (title_choice == CHOICE_E102) {
        stage = STAGE_WINDY;
        act = 0;
    } else if (title_choice == CHOICE_KNUCKLES) {
        stage = STAGE_MOUNTAIN;
        act = 2;
    } else if (title_choice == CHOICE_TAILS) {
        stage = STAGE_ICECAP;
        act = 2;
    } else {
        stage = STAGE_HIGHWAY;
        act = 0;
    }
    stage_set_stage_act(stage, act);
    if (per_info[0]->on & PDD_DGT_TA) {
        recording_kind = RECORDING_PLAY;
    }
    if (per_info[0]->on & PDD_DGT_TB) {
        recording_kind = RECORDING_RECORD;
    }
    demo_event = DEMO_NO_EVENT;
    demo_playing = TRUE;
    hud_reset();
    progress_init();
    demo_set_flags();
}

Sint16 title_step(void)
{
    int entry;
    Uint16 plays;
    Uint16 clears;
    Uint16 *chosen;
    int character;
    Uint8 *counts;
    Uint8 *clear_counts;
    int at;
    int speed;
    int step;

    if (title_debug) {
        return title_debug_step();
    }
    if (title_music_wait >= MUSIC_WAIT) {
        if (title_music_wait == MUSIC_WAIT) {
            stream_reset();
            stream_request_music(MUSIC_TICAL);
            title_music_wait++;
        }
    } else {
        title_music_wait++;
    }
    task_step_all();
    /* @hack spelling: the fade's step through two locals, set as the switch
     * reads the mode, which keeps it in the frame at @R15 (MOV.L R0,@R15 at
     * 0x8c09fcf0); passed as FADE_SPEED, 111 of 751 words differ */
    switch (speed = FADE_SPEED, step = speed, title_mode) {
    case TITLE_SHOW:
        pad_enable();
        pad_scan();
        title_fade_step(step);
        if (title_timer++ == SHOW_FRAMES) {
            title_mode = TITLE_SHOW_TO_DEMO;
            stream_destroy();
            texture_load_pvm("LOADING", &loading_texlist);
        }
        if ((pad_held_live[0] & PDD_DGT_TA) &&
            (pad_press_live[0] & PDD_DGT_ST)) {
            if (save_isp_found()) {
                stream_request_voice(VOICE_LEAVING);
                stream_destroy();
#ifdef GCC
                /* repairs the call the tag below names: no Chao is in hand at
                 * the title, so GCC writes no Chao's mail, only the save's */
#else
                /* @bug mismatched-call: save_write_chao_mail takes the Chao to
                 * write as its second argument, but the call passes only
                 * the first (JSR at 0x8c09fdae). the mail's Chao is
                 * whatever R5 addresses as stream_destroy leaves it */
                /* NOLINTBEGIN(clang-diagnostic-cast-function-type-mismatch) */
                ((Bool (*)(int))save_write_chao_mail)(0);
                /* NOLINTEND(clang-diagnostic-cast-function-type-mismatch) */
#endif
                save_write_player_mail(0);
                game_leave();
            } else {
                stream_request_voice(VOICE_NO_SETTINGS);
            }
        } else if (pad_press_live[0] & PDD_DGT_ST) {
            if (pad_held_live[0] & PDD_DGT_TX) {
                debug_unlocked = 1;
            }
            if (title_preset >= 0) {
                title_choice = title_preset;
                title_mode = TITLE_SHOW_TO_GUIDE;
            } else {
                title_mode = TITLE_SHOW_TO_CARDS;
            }
            title_fade_white = -1;
            stream_destroy();
            sound_play(SE_CHOOSE, 0, 0, 0);
        }
        if (title_fade_white) {
            title_fade_draw_white();
        } else {
            title_fade_draw();
        }
        title_draw();
        if ((pad_held_live[0] & (PDD_DGT_TX | PDD_DGT_TY)) ==
            (PDD_DGT_TX | PDD_DGT_TY)) {
            debug_unlocked = 1;
            title_mode = TITLE_QUIT;
            stream_destroy();
        }
        break;
    case TITLE_SHOW_TO_DEMO:
        if (title_fade_step(-FADE_SPEED)) {
            title_mode = TITLE_DEMO;
        }
        title_draw();
        title_draw_loading();
        break;
    case TITLE_SHOW_TO_CARDS:
        if (title_fade_step(-FADE_SPEED)) {
            title_mode = TITLE_CARDS;
            njReleaseTexture(&title_texlist);
            texture_load_pvm("C_SELECT1", &c_select1_texlist);
            texture_load_pvm("C_SELECT2", &c_select2_texlist);
            stream_reset();
            stream_request_music(MUSIC_CHARACTR);
            title_timer = 0;
        }
        title_draw();
        title_fade_draw_white();
        break;
    case TITLE_CARDS_TO_SHOW:
        if (title_fade_step(-FADE_SPEED)) {
            title_mode = TITLE_SHOW;
            title_timer = 0;
            njReleaseTexture(&c_select1_texlist);
            njReleaseTexture(&c_select2_texlist);
            texture_load_pvm("TITLE", &title_texlist);
        }
        title_draw_cards();
        title_fade_draw_white();
        break;
    case TITLE_CARDS_TO_GUIDE:
        if (title_fade_step(-FADE_SPEED)) {
            title_mode = TITLE_CONTROLS;
            title_timer = PAGE_FRAMES;
            njReleaseTexture(&c_select1_texlist);
            njReleaseTexture(&c_select2_texlist);
            texture_load_pvm(title_guide_files[title_choice],
                             title_guide_texlists[title_choice]);
            texture_load_pvm("PRESSSTART", &press_start_texlist);
            title_press_start =
                task_create(TASK_WORK, 1, title_press_start_step);
            title_press_start->destructor = title_press_start_destroy;
            stream_reset();
            stream_request_music(title_guide_music[title_choice]);
        }
        title_draw_cards();
        title_fade_draw_white();
        break;
    case TITLE_CONTROLS_TO_RULES:
        if (title_fade_step(-FADE_SPEED)) {
            title_mode = TITLE_RULES;
            title_timer = PAGE_FRAMES;
        }
        title_draw_controls();
        title_fade_draw_white();
        break;
    case TITLE_SHOW_TO_GUIDE:
        if (title_fade_step(-FADE_SPEED)) {
            title_mode = TITLE_CONTROLS;
            title_timer = PAGE_FRAMES;
            njReleaseTexture(&title_texlist);
            texture_load_pvm(title_guide_files[title_choice],
                             title_guide_texlists[title_choice]);
            texture_load_pvm("PRESSSTART", &press_start_texlist);
            title_press_start =
                task_create(TASK_WORK, 1, title_press_start_step);
            title_press_start->destructor = title_press_start_destroy;
        }
        title_draw();
        title_fade_draw_white();
        break;
    case TITLE_RULES_TO_START:
        if (title_fade_step(-FADE_SPEED)) {
            title_mode = TITLE_START;
            /* @bug memory: TITLE_RULES killed the task (0x8c0a0522), and
             * the next task_step_all freed it, so this kill stores into a
             * freed task (0x8c0a0106) */
            if (title_press_start != NULL) {
                task_kill(title_press_start);
            }
            njReleaseTexture(title_guide_texlists[title_choice]);
        }
        title_draw_rules();
        title_draw_loading();
        break;
    case TITLE_CARDS:
        title_fade_step(step);
        title_fade_draw_white();
        if (title_timer++ == CARDS_FRAMES) {
            title_mode = TITLE_CARDS_TO_SHOW;
        }
        pad_enable();
        pad_scan();
        /* up, y and b held, without x or a, print the choice's plays and
         * clears */
        if ((pad_records[0].on & PDD_DGT_KU) &&
            (pad_records[0].on & PDD_DGT_TY) &&
            (pad_records[0].on & PDD_DGT_TB) &&
            !(pad_records[0].on & PDD_DGT_TX) &&
            !(pad_records[0].on & PDD_DGT_TA)) {
            /* the choice's index among history's six characters, which
             * leave out Eggman and Tikal. times HISTORY_SIZE, entry becomes
             * the offset of the choice's counts in history_counts */
            entry = title_characters[title_choice];
            if (entry > CHARACTER_TIKAL) {
                entry--;
            }
            if (entry > CHARACTER_EGGMAN) {
                entry--;
            }
            entry *= HISTORY_SIZE;
            /* @hack spelling: the counts read through pointers and an index of
             * their own, which work out the clears' address in R6 as the
             * entry's offset plus 2, then plus history_counts' address
             * (0x8c0a01ea to 0x8c0a01ee); subscripted, 76 of 751 words
             * differ */
            counts = history_counts;
            at = entry + HISTORY_CLEARS;
            clear_counts = &counts[at];
            clears = (clear_counts[0] << 8) + clear_counts[1];
            plays = (counts[entry] << 8) + counts[entry + 1];
            njPrintC(NJM_LOCATION(28, 24), "- HISTORY -");
            njPrintC(NJM_LOCATION(28, 25), "PLAY :     ");
            njPrintC(NJM_LOCATION(28, 26), "CLEAR:     ");
            njPrintD(NJM_LOCATION(34, 25), plays, 5);
            njPrintD(NJM_LOCATION(34, 26), clears, 5);
        }
        title_move_choice();
        if (PAD_BUTTONS(pad_press_live[0], pad_choose_buttons)) {
            if ((pad_held_live[0] & PDD_DGT_TX) &&
                (pad_held_live[0] & PDD_DGT_TY)) {
                sound_play(SE_NOTICE, 0, 0, 0);
                /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
                title_preset = title_choice;
            } else {
                sound_play(SE_CHOOSE, 0, 0, 0);
            }
            title_mode = TITLE_CARDS_TO_GUIDE;
            stream_destroy();
        }
        title_draw_cards();
        break;
    case TITLE_QUIT:
        title_mode = TITLE_DONE;
        njReleaseTexture(&title_texlist);
        njReleaseTexture(&c_select1_texlist);
        njReleaseTexture(&c_select2_texlist);
        return TITLE_STAGE_SELECT;
    case TITLE_START:
        title_mode = TITLE_DONE;
        task_destroy_all();
        stream_destroy();
        njReleaseTexture(&title_texlist);
        njReleaseTexture(&c_select1_texlist);
        njReleaseTexture(&c_select2_texlist);
        /* @hack spelling: the choice's character through a pointer, whose
         * address the image works out in R14 before the test (0x8c0a0334 to
         * 0x8c0a0342); subscripted at each use, 26 of 751 words differ */
        chosen = &title_characters[title_choice];
        /* with b or a held, play the choice's stage through the recorder.
         * otherwise start its story */
        if ((per_info[0]->on & PDD_DGT_TB) || (per_info[0]->on & PDD_DGT_TA)) {
            set_character(*chosen);
            title_set_stage();
        } else {
            character = *chosen;
            story_enable();
            set_character(character);
            story_set_character(character);
            story_set_next_stage(player_record->section);
        }
        hud_hidden = 0;
        title_draw_loading();
        return TITLE_PLAY;
    case TITLE_GARDEN:
        title_mode = TITLE_DONE;
        stage_set_stage_act(STAGE_ALIFE_SS, 0);
        task_destroy_all();
        stream_destroy();
        njReleaseTexture(&title_texlist);
        njReleaseTexture(&c_select1_texlist);
        njReleaseTexture(&c_select2_texlist);
        set_character(title_characters[title_choice]);
        hud_hidden = 0;
        title_draw_loading();
        return TITLE_PLAY;
    case TITLE_DEMO:
        title_mode = TITLE_DONE;
        njReleaseTexture(&title_texlist);
        stream_destroy();
        task_destroy_all();
        demo_advance();
        title_draw_loading();
        return TITLE_PLAY;
    case TITLE_CONTROLS:
        title_fade_step(step);
        title_fade_draw_white();
        pad_enable();
        pad_scan();
        if (title_timer-- == 0 ||
            PAD_BUTTONS(pad_press_live[0], pad_choose_buttons)) {
            title_mode = TITLE_CONTROLS_TO_RULES;
            sound_play(SE_CHOOSE, 0, 0, 0);
            title_timer = PAGE_FRAMES;
            entry = title_characters[title_choice];
            if (entry > CHARACTER_TIKAL) {
                entry--;
            }
            if (entry > CHARACTER_EGGMAN) {
                entry--;
            }
            history_count_play(entry);
        }
        title_draw_controls();
        break;
    case TITLE_RULES:
        title_fade_step(step);
        title_fade_draw_white();
        pad_enable();
        pad_scan();
        if (title_timer-- == 0 ||
            PAD_BUTTONS(pad_press_live[0], pad_choose_buttons)) {
            title_mode = TITLE_RULES_TO_START;
            sound_play(SE_CHOOSE, 0, 0, 0);
            if (title_press_start != NULL) {
                task_kill(title_press_start);
            }
            stream_destroy();
            texture_load_pvm("LOADING", &loading_texlist);
        }
        title_draw_rules();
        break;
    default:
        title_mode = TITLE_QUIT;
        break;
    }
    return 0;
}

/* draw the picture at title_debug_brightness */
static void title_draw_dim(void)
{
    NJS_SPRITE sprite;
    Float brightness = title_debug_brightness;
    int i;

    sprite.tlist = &title_texlist;
    sprite.tanim = title_texanims;
    njSetTexture(sprite.tlist);
    draw_set_material(0.0F, brightness, brightness, brightness);
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    sprite.p.x = SCREEN_MIDDLE_X;
    sprite.p.y = 0.0F;
    for (i = 0; i < TITLE_PIECES; i++) {
        njDrawSprite2D(&sprite, i, -10.0F, NJD_SPRITE_COLOR);
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

static void title_debug_draw_characters(void)
{
    Uint32 normal;
    Uint32 selected;
    int row;
    int i;

    if (title_mode == TITLE_CARDS) {
        selected = COLUMN_PICK;
        normal = COLUMN_TEXT;
    } else {
        selected = COLUMN_PICK_DIM;
        normal = COLUMN_TEXT_DIM;
    }
    njPrintColor(normal);
    njPrintC(NJM_LOCATION(4, 8), "CHARACTER");
    row = 10;
    for (i = 0; i < DEBUG_CHARACTERS; i++) {
        if (i == title_choice) {
            njPrintColor(selected);
        } else {
            njPrintColor(normal);
        }
        njPrintC(NJM_LOCATION(4, row), title_debug_names[i]);
        row++;
    }
    njPrintColor(COLOR_WHITE);
}

static void title_debug_draw_stages(void)
{
    Uint32 normal;
    Uint32 selected;
    int line;
    int row;
    int i;
    int stage;
    int act;

    if (title_mode == TITLE_DEBUG_STAGES) {
        selected = COLUMN_PICK;
        normal = COLUMN_TEXT;
    } else {
        selected = COLUMN_PICK_DIM;
        normal = COLUMN_TEXT_DIM;
    }
    njPrintColor(normal);
    njPrintC(NJM_LOCATION(16, 8), "STAGE SELECT");
    line = title_debug_stage + (DEBUG_STAGES - DEBUG_ROWS_ABOVE);
    row = 10;
    for (i = 0; i < DEBUG_ROWS; i++) {
        line %= DEBUG_STAGES;
        if (line == title_debug_stage) {
            njPrintColor(selected);
        } else {
            njPrintColor(normal);
        }
        stage = title_debug_stages[line][0];
        act = title_debug_stages[line][1];
        debug_print_stage_name(NJM_LOCATION(16, row), STAGE_ACT(stage, act));
        line++;
        row++;
    }
    njPrintColor(COLOR_WHITE);
}

static void title_debug_draw_stories(void)
{
    Uint32 normal;
    Uint32 selected;
    int row;
    int i;

    if (title_mode == TITLE_DEBUG_STORIES) {
        selected = COLUMN_PICK;
        normal = COLUMN_TEXT;
    } else {
        selected = COLUMN_PICK_DIM;
        normal = COLUMN_TEXT_DIM;
    }
    njPrintColor(normal);
    njPrintC(NJM_LOCATION(6, 8), "ADV SEQUENCE");
    row = 10;
    for (i = 0; i < DEBUG_CHARACTERS; i++) {
        if (i == title_debug_story) {
            njPrintColor(selected);
        } else {
            njPrintColor(normal);
        }
        njPrintC(NJM_LOCATION(6, row), title_debug_names[i]);
        njPrintC(NJM_LOCATION(14, row), "STORY");
        row++;
    }
    njPrintColor(COLOR_WHITE);
}

/* blink an arrow at the screen's side, toward the other columns: the left
 * one beside the stories, and the right one beside the rest */
static void title_debug_draw_left_arrow(void)
{
    if (!(title_debug_frames & ARROW_BLINK)) {
        njPrintColor(COLOR_ARROW);
        njPrintC(NJM_LOCATION(2, 16), "<<");
        njPrintColor(COLOR_WHITE);
    }
}

static void title_debug_draw_right_arrow(void)
{
    if (!(title_debug_frames & ARROW_BLINK)) {
        njPrintColor(COLOR_ARROW);
        njPrintC(NJM_LOCATION(34, 16), ">>");
        njPrintColor(COLOR_WHITE);
    }
}

void title_debug_reset(void)
{
    title_mode = TITLE_SHOW;
    title_debug_stage = 0;
    title_choice = 0;
    title_debug_story = 0;
    title_music_wait = 0;
    title_debug_brightness = 1.0F;
    dat_8c1da32c = 0;
}

/* run a frame of the debug menu, which picks a character, a stage and a
 * story, each in a column of its own. return 0 while it runs, then a
 * title_result. up and down move a column's pick. in the stages and the
 * stories, left and right move it too, a row each frame they're held. the
 * triggers move between the columns */
static Sint16 title_debug_step(void)
{
    int character;

    title_debug_frames++;
    switch (title_mode) {
    case TITLE_SHOW:
        pad_enable();
        pad_scan();
        if (pad_records[0].on & PDD_DGT_ST) {
            title_mode = TITLE_CARDS;
        } else if ((pad_held_live[0] & (PDD_DGT_TX | PDD_DGT_TY)) ==
                   (PDD_DGT_TX | PDD_DGT_TY)) {
            title_mode = TITLE_QUIT;
        }
        title_draw_dim();
        break;
    case TITLE_DEBUG_STAGES:
        pad_enable();
        pad_scan();
        if ((pad_press_live[0] & PDD_DGT_KU) ||
            (pad_held_live[0] & PDD_DGT_KL)) {
            title_debug_stage--;
            sound_play(SE_MOVE, 0, 0, 0);
        } else if ((pad_press_live[0] & PDD_DGT_KD) ||
                   (pad_held_live[0] & PDD_DGT_KR)) {
            title_debug_stage++;
            sound_play(SE_MOVE, 0, 0, 0);
        }
        if (title_debug_stage >= DEBUG_STAGES) {
            title_debug_stage = 0;
        } else if (title_debug_stage < 0) {
            title_debug_stage = DEBUG_STAGES - 1;
        }
        if (pad_press_live[0] & PDD_DGT_TR) {
            title_mode = TITLE_DEBUG_STORIES;
        } else if (pad_press_live[0] & PDD_DGT_TL) {
            title_mode = TITLE_CARDS;
        }
        if (PAD_BUTTONS(pad_press_live[0], pad_choose_buttons)) {
            title_mode = TITLE_START;
            sound_play(SE_CHOOSE, 0, 0, 0);
        } else if ((pad_held_live[0] & (PDD_DGT_TX | PDD_DGT_TY)) ==
                   (PDD_DGT_TX | PDD_DGT_TY)) {
            title_mode = TITLE_QUIT;
        }
        title_draw_dim();
        title_debug_draw_characters();
        title_debug_draw_stages();
        title_debug_draw_right_arrow();
        break;
    case TITLE_CARDS:
        pad_enable();
        pad_scan();
        if (title_debug_brightness <= DEBUG_DIM) {
            title_debug_brightness = DEBUG_DIM;
        } else {
            /* @hack spelling: -DEBUG_DIM_STEP added, an FADD of -0.02 from
             * the pool (0x8c0a0d16); with -=, an FSUB of 0.02, and 1 of 392
             * words differ */
            title_debug_brightness += -DEBUG_DIM_STEP;
        }
        if (pad_press_live[0] & PDD_DGT_KU) {
            title_choice--;
            sound_play(SE_MOVE, 0, 0, 0);
        } else if (pad_press_live[0] & PDD_DGT_KD) {
            title_choice++;
            sound_play(SE_MOVE, 0, 0, 0);
        }
        if (title_choice >= DEBUG_CHARACTERS) {
            title_choice = 0;
        } else if (title_choice < 0) {
            title_choice = DEBUG_CHARACTERS - 1;
        }
        if (pad_press_live[0] & PDD_DGT_TR) {
            title_mode = TITLE_DEBUG_STAGES;
        }
        if (PAD_BUTTONS(pad_press_live[0], pad_choose_buttons)) {
            title_mode = TITLE_START;
            sound_play(SE_CHOOSE, 0, 0, 0);
        } else if ((pad_held_live[0] & (PDD_DGT_TX | PDD_DGT_TY)) ==
                   (PDD_DGT_TX | PDD_DGT_TY)) {
            title_mode = TITLE_QUIT;
        }
        title_draw_dim();
        title_debug_draw_characters();
        title_debug_draw_stages();
        title_debug_draw_right_arrow();
        break;
    case TITLE_DEBUG_STORIES:
        pad_enable();
        pad_scan();
        if ((pad_press_live[0] & PDD_DGT_KU) ||
            (pad_held_live[0] & PDD_DGT_KL)) {
            title_debug_story--;
            sound_play(SE_MOVE, 0, 0, 0);
        } else if ((pad_press_live[0] & PDD_DGT_KD) ||
                   (pad_held_live[0] & PDD_DGT_KR)) {
            title_debug_story++;
            sound_play(SE_MOVE, 0, 0, 0);
        }
        if (title_debug_story >= DEBUG_CHARACTERS) {
            title_debug_story = 0;
        } else if (title_debug_story < 0) {
            title_debug_story = DEBUG_CHARACTERS - 1;
        }
        if (pad_press_live[0] & PDD_DGT_TL) {
            title_mode = TITLE_DEBUG_STAGES;
        }
        if (PAD_BUTTONS(pad_press_live[0], pad_choose_buttons)) {
            /* the story set here is the character column's pick, not this
             * column's. TITLE_START then turns the story off with
             * story_disable */
            character = title_debug_characters[title_choice];
            set_character(character);
            story_set_character(character);
            story_set_next_stage(0);
            title_mode = TITLE_START;
            sound_play(SE_CHOOSE, 0, 0, 0);
        } else if ((pad_held_live[0] & (PDD_DGT_TX | PDD_DGT_TY)) ==
                   (PDD_DGT_TX | PDD_DGT_TY)) {
            title_mode = TITLE_QUIT;
        }
        title_draw_dim();
        title_debug_draw_stories();
        title_debug_draw_left_arrow();
        break;
    case TITLE_QUIT:
        title_mode = TITLE_DONE;
        njReleaseTexture(&title_texlist);
        return TITLE_STAGE_SELECT;
    case TITLE_START:
        story_disable();
        character = title_debug_characters[title_choice];
        stage_set_stage_act(title_debug_stages[title_debug_stage][0],
                            title_debug_stages[title_debug_stage][1]);
        set_character(character);
        title_mode = TITLE_DONE;
        njReleaseTexture(&title_texlist);
        return TITLE_PLAY;
    default:
        title_mode = TITLE_QUIT;
        break;
    }
    return 0;
}

void title_fade_set(int level)
{
    title_fade_level = level;
}

int title_fade_step(int step)
{
    if (title_fade_level < FADE_END && step < 0) {
        title_fade_level = 0;
        return -1;
    }
    if (title_fade_level > 255 && step > 0) {
        title_fade_level = 255;
        return 1;
    }
    title_fade_level += step;
    return 0;
}

void title_fade_draw(void)
{
    NJS_COLOR colors[4];
    NJS_POINT2 points[4];
    NJS_POINT2COL polygon;
    int alpha = 255 - title_fade_level;

    if (alpha <= 0) {
        return;
    }
    if (alpha >= 255) {
        alpha = 255;
    }
    /* @hack spelling: alpha = alpha << 24, which puts &points[1] in R7 and
     * &points[2] in R2 (0x8c0a1002 to 0x8c0a1008); as <<=, the two trade
     * and 13 of 66 words differ */
    alpha = alpha << 24;
    points[0].x = points[1].x = 0.0F;
    points[2].x = points[3].x = SCREEN_WIDTH;
    points[0].y = points[3].y = 0.0F;
    points[1].y = points[2].y = SCREEN_HEIGHT;
    colors[0].color = colors[1].color = colors[2].color = colors[3].color =
        alpha;
    polygon.p = points;
    polygon.col = colors;
    polygon.tex = NULL;
    polygon.num = 4;
    njDrawPolygon2D(&polygon, 4, -0.01F, NJD_FILL | NJD_TRANSPARENT);
}

/* draw the fade in white. once its alpha is full, it draws opaque, without
 * NJD_TRANSPARENT */
static void title_fade_draw_white(void)
{
    NJS_COLOR colors[4];
    NJS_POINT2 points[4];
    NJS_POINT2COL polygon;
    int alpha = 255 - title_fade_level;
    Uint32 color;
    Uint32 attr;

    if (alpha <= 0) {
        return;
    }
    if (alpha >= 255) {
        alpha = 255;
    }
    color = alpha << 24 | 0xffffff;
    points[0].x = points[1].x = 0.0F;
    points[2].x = points[3].x = SCREEN_WIDTH;
    points[0].y = points[3].y = 0.0F;
    points[1].y = points[2].y = SCREEN_HEIGHT;
    colors[0].color = colors[1].color = colors[2].color = colors[3].color =
        color;
    polygon.p = points;
    polygon.col = colors;
    polygon.tex = NULL;
    polygon.num = 4;
    attr = color == 0xffffffff ? NJD_FILL : NJD_FILL | NJD_TRANSPARENT;
    njDrawPolygon2D(&polygon, 4, -0.01F, attr);
}

/* draw the loading screen with the fade's alpha: opaque at a level of 0,
 * and clear at 255 */
static void title_draw_loading(void)
{
    NJS_SPRITE sprite;
    /* @hack redundant: 16 bytes of frame below sprite that no code reads, of a
     * type unknown, declared as an NJS_ARGB. the image's ADD #-48,R15 reserves
     * them (0x8c0a114e); without them, 141 of 159 words differ */
    NJS_ARGB unused;
    int alpha = 255 - title_fade_level;
    int i;

    if (alpha <= 0) {
        return;
    }
    if (alpha >= 255) {
        alpha = 255;
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(alpha * (1.0F / 255.0F), 1.0F, 1.0F, 1.0F);
    sprite.tlist = &loading_texlist;
    sprite.tanim = title_loading_texanims;
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    sprite.p.x = SCREEN_MIDDLE_X;
    sprite.p.y = SCREEN_MIDDLE_Y;
    for (i = 0; i < 2; i++) {
        njDrawSprite2D(&sprite, i, -0.9F, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    }
    sprite.sx = 24.0F;
    sprite.sy = 60.0F;
    sprite.p.x = 192.0F;
    sprite.p.y = SCREEN_MIDDLE_Y;
    njDrawSprite2D(&sprite, 2, -1.0F, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    sprite.p.x = 448.0F;
    sprite.p.y = SCREEN_MIDDLE_Y;
    njDrawSprite2D(&sprite, 3, -1.0F, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    sprite.sx = 32.0F;
    sprite.sy = 22.0F;
    sprite.p.x = SCREEN_MIDDLE_X;
    sprite.p.y = 176.0F;
    njDrawSprite2D(&sprite, 4, -1.0F, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    sprite.p.x = SCREEN_MIDDLE_X;
    sprite.p.y = 304.0F;
    njDrawSprite2D(&sprite, 5, -1.0F, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}
