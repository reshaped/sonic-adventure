/* the SEQ EDITOR, a program of the <SELECT PROGRAM> menu. its pages set the
 * event flags, the events played and the stages cleared. they go to a stage
 * or a section of the story, and show where player 0 is */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "debug/debug.h"
#include "debug/editor.h"
#include "debug/seq_editor.h"
#include "draw/draw.h"
#include "player/player.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */

/* the text's colors. TEXT_LIT colors most text, and each box's rim.
 * TEXT_DIM marks a value that is off, and each row of a stage list the
 * cursor isn't on. while a page runs, it also marks each menu row but that
 * page's. TEXT_CURSOR colors the cursor and each list's heading */
#define TEXT_LIT 0xffa0a0a0
#define TEXT_DIM 0xff606060
#define TEXT_CURSOR 0xffff0000
/* the section the story is in, on the SEQ page */
#define TEXT_CURRENT 0xff00ff00
/* each box's fill: black, half transparent */
#define BOX_FILL 0x7f000000

/* the menu's pages, the last of them EXIT */
#define SEQ_PAGES 7
#define SEQ_EXIT (SEQ_PAGES - 1)
/* the rows the FLAG, EVENT and SEQ lists show at once. each list scrolls
 * to keep the cursor from its window's row LIST_CURSOR_TOP to row
 * LIST_CURSOR_BOTTOM, counting from 0, where the list's ends allow */
#define LIST_ROWS 14
#define LIST_CURSOR_TOP 5
#define LIST_CURSOR_BOTTOM 8
/* the rows of the SCENE list, seq_scenes */
#define SEQ_SCENES 76
/* the rows COMPLT lists: STAGE_PRACTICE to STAGE_E101_R, then
 * STAGE_SHOOTING1's two acts */
#define SEQ_STAGES 28
/* the rows the SCENE and COMPLT lists show, and the middle one, which the
 * cursor is on */
#define STAGE_ROWS_SHOWN 11
#define STAGE_CURSOR_ROW (STAGE_ROWS_SHOWN / 2)

enum seq_mode {
    SEQ_OPEN = 2, /* opens the menu, with no page running */
    SEQ_RUN, /* runs seq_menu until it returns -1 */
    SEQ_CLOSE
};

/* what the SCENE page's pad chooses: L and R switch between the two */
enum seq_scene_mode { SCENE_STAGE, SCENE_ENTRANCE };
#define SCENE_MODES 2

/* a translucent box behind a page's text, with a rim */
struct seq_box {
    Float x, y;
    Float width, height;
    Uint32 color;
};

struct seq_page {
    char *name;
    int (*exec)(void); /* -1 goes back to the menu */
};

static int seq_menu(void);
static int seq_flag_page(void);
static int seq_event_page(void);
static int seq_scene_page(void);
static int seq_section_page(void);
static int seq_complete_page(void);
static int seq_debug_page(void);
static void seq_draw_box(struct seq_box *box);

/* nothing reads it */
static Sint16 seq_characters[6] = {
    CHARACTER_SONIC, CHARACTER_TAILS, CHARACTER_KNUCKLES,
    CHARACTER_AMY,   CHARACTER_E102,  CHARACTER_BIG,
};
/* the boxes behind the menu's text and each page's. SCENE and COMPLT
 * share seq_stage_box, behind their lists of stages, and FLAG and EVENT
 * share seq_flag_box */
static struct seq_box seq_menu_box = {48.0F, 112.0F, 128.0F, 144.0F, BOX_FILL};
static struct seq_box seq_scene_box = {208.0F, 48.0F, 320.0F, 64.0F, BOX_FILL};
static struct seq_box seq_stage_box = {208.0F, 144.0F, 320.0F, 256.0F,
                                       BOX_FILL};
static struct seq_box seq_flag_box = {208.0F, 144.0F, 160.0F, 288.0F, BOX_FILL};
static struct seq_box seq_section_box = {208.0F, 112.0F, 384.0F, 288.0F,
                                         BOX_FILL};
static struct seq_box seq_debug_box = {208.0F, 64.0F, 384.0F, 288.0F, BOX_FILL};
/* twelve of the hud's textures, from hyoji_head on, and 32 by 32 frames of
 * six of them. nothing reads either */
static NJS_TEXLIST seq_texlist = {
#include "debug/seq_editor/seq_texlist.inc"
};
static NJS_TEXANIM seq_texanims[6] = {
#include "debug/seq_editor/seq_texanims.inc"
};

/* EXIT has no page: the menu leaves before it would run one */
static struct seq_page seq_pages[SEQ_PAGES] = {
    {"FLAG", seq_flag_page},
    {"EVENT", seq_event_page},
    {"SCENE", seq_scene_page},
    {"COMPLT", seq_complete_page},
    {"SEQ", seq_section_page},
    {"DEBUG", seq_debug_page},
    {"EXIT", NULL},
};
static char *seq_off_on[2] = {"OFF", "ON"};
static char *seq_no_yes[2] = {"NO", "YES"};

/* the SCENE list's stages and acts. its last three rows are left at zero,
 * which is STAGE_PRACTICE's act 0 */
static Uint8 seq_scenes[SEQ_SCENES][2] = {
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
    {STAGE_SHOOTING1, 1},
    {STAGE_SUNABOARD, 0},
    {STAGE_ALIFE_SS, 0},
    {STAGE_ALIFE_EG, 0},
    {STAGE_ALIFE_MR, 0},
    {STAGE_ALIFE_RACE, 0},
};

static enum seq_mode seq_mode;
/* -1 in the menu, or the page that runs, which seq_menu_cursor names too */
static int seq_page;
static int seq_menu_cursor;
/* nothing reads or writes it */
static Uint32 dat_8c7aabac;
static int seq_scene_cursor;
static int seq_scene_entrance;
/* each list's cursor, and the first row its window shows */
static int seq_flag_cursor;
static int seq_flag_top;
static int seq_event_cursor;
static int seq_event_top;
static int seq_complete_cursor;
/* nothing reads or writes it */
static Uint32 dat_8c7aabcc;
static int seq_section_cursor;
static int seq_section_top;
static enum seq_scene_mode seq_scene_mode;

void seq_editor_init(void)
{
    seq_mode = SEQ_OPEN;
}

int seq_editor_exec(void)
{
    switch (seq_mode) {
    case SEQ_OPEN:
        seq_page = -1;
        seq_mode = SEQ_RUN;
        break;
    case SEQ_RUN:
        if (seq_menu() == -1) {
            seq_mode = SEQ_CLOSE;
        }
        njPrintColor(0xffffffff);
        break;
    case SEQ_CLOSE:
        seq_mode = SEQ_OPEN;
        return -1;
    }
    editor_take_buttons(0, PDD_DGT_TL | PDD_DGT_TR | PDD_DGT_TX | PDD_DGT_TY |
                               PDD_DGT_ST | PDD_DGT_TA | PDD_DGT_TB);
    return 0;
}

/* up and down move the menu's cursor, which stops at the ends. a choice
 * runs the page at the cursor, beside the dimmed menu, until the page
 * returns -1. cancel moves the cursor to EXIT. return -1 when EXIT is
 * chosen, otherwise 0 */
static int seq_menu(void)
{
    int i;

    seq_draw_box(&seq_menu_box);
    if (seq_page == -1) {
        if (pad_press_live[0] & PDD_DGT_KU) {
            seq_menu_cursor--;
        } else if (pad_press_live[0] & PDD_DGT_KD) {
            seq_menu_cursor++;
        }
        if (seq_menu_cursor < 0) {
            seq_menu_cursor = 0;
        } else if (seq_menu_cursor >= SEQ_PAGES) {
            seq_menu_cursor = SEQ_PAGES - 1;
        }
        if (PAD_BUTTONS(pad_press_live[0], pad_choose_buttons)) {
            if (seq_menu_cursor == SEQ_EXIT) {
                return -1;
            } else {
                seq_page = seq_menu_cursor;
            }
        } else if (PAD_BUTTONS(pad_press_live[0], pad_cancel_buttons)) {
            seq_menu_cursor = SEQ_EXIT;
        }
        njPrintColor(TEXT_LIT);
        for (i = 0; i < SEQ_PAGES; i++) {
            njPrintC(NJM_LOCATION(4, i + 8), seq_pages[i].name);
        }
        njPrintColor(TEXT_CURSOR);
        njPrintC(NJM_LOCATION(3, seq_menu_cursor + 8), ">");
    } else {
        if (seq_pages[seq_menu_cursor].exec() == -1) {
            seq_page = -1;
        }
        njPrintColor(TEXT_DIM);
        for (i = 0; i < SEQ_PAGES; i++) {
            if (i == seq_menu_cursor) {
                njPrintColor(TEXT_LIT);
            } else {
                njPrintColor(TEXT_DIM);
            }
            njPrintC(NJM_LOCATION(4, i + 8), seq_pages[i].name);
        }
    }
    return 0;
}

/* FLAG: the event flags, by number. up and down move the cursor a flag a
 * press, and left and right a flag each step while held. a choice turns the
 * cursor's flag on or off */
static int seq_flag_page(void)
{
    int move = 0;
    int i;
    int flag;
    int on;

    if (PAD_BUTTONS(pad_press_live[0], pad_cancel_buttons)) {
        return -1;
    } else if (PAD_BUTTONS(pad_press_live[0], pad_choose_buttons)) {
        sound_play(SE_CHOOSE, 0, 0, 0);
        event_flags[seq_flag_cursor] = !event_flags[seq_flag_cursor];
    }
    if ((pad_press_live[0] & PDD_DGT_KU) || (pad_held_live[0] & PDD_DGT_KL)) {
        move--;
    }
    if ((pad_press_live[0] & PDD_DGT_KD) || (pad_held_live[0] & PDD_DGT_KR)) {
        move++;
    }
    if (move < 0) {
        seq_flag_cursor += move;
        if (seq_flag_cursor < 0) {
            seq_flag_cursor = 0;
        }
        if (seq_flag_cursor < seq_flag_top + LIST_CURSOR_TOP) {
            seq_flag_top += move;
        }
    } else if (move > 0) {
        seq_flag_cursor += move;
        if (seq_flag_cursor > FLAG_COUNT - 1) {
            seq_flag_cursor = FLAG_COUNT - 1;
        }
        if (seq_flag_cursor > seq_flag_top + LIST_CURSOR_BOTTOM) {
            seq_flag_top += move;
        }
    }
    if (seq_flag_top < 0) {
        seq_flag_top = 0;
    } else if (seq_flag_top > FLAG_COUNT - LIST_ROWS) {
        seq_flag_top = FLAG_COUNT - LIST_ROWS;
    }
    for (i = 0, flag = seq_flag_top; i < LIST_ROWS; i++, flag++) {
        on = event_flags[flag] == 0 ? 0 : 1;
        if (seq_flag_cursor == flag) {
            njPrintColor(TEXT_CURSOR);
            njPrintC(NJM_LOCATION(13, i + 10), ">");
        }
        njPrintColor(TEXT_LIT);
        njPrintH(NJM_LOCATION(14, i + 10), flag, 4);
        if (on == 1) {
            njPrintColor(TEXT_LIT);
        } else {
            njPrintColor(TEXT_DIM);
        }
        njPrintC(NJM_LOCATION(19, i + 10), seq_off_on[on]);
    }
    seq_draw_box(&seq_flag_box);
    return 0;
}

/* EVENT: as FLAG, for whether each event has played */
static int seq_event_page(void)
{
    int move = 0;
    int i;
    int event;
    int on;

    if (PAD_BUTTONS(pad_press_live[0], pad_cancel_buttons)) {
        return -1;
    } else if (PAD_BUTTONS(pad_press_live[0], pad_choose_buttons)) {
        sound_play(SE_CHOOSE, 0, 0, 0);
        events_played[seq_event_cursor] = !events_played[seq_event_cursor];
    }
    if ((pad_press_live[0] & PDD_DGT_KU) || (pad_held_live[0] & PDD_DGT_KL)) {
        move--;
    }
    if ((pad_press_live[0] & PDD_DGT_KD) || (pad_held_live[0] & PDD_DGT_KR)) {
        move++;
    }
    if (move < 0) {
        seq_event_cursor += move;
        if (seq_event_cursor < 0) {
            seq_event_cursor = 0;
        }
        if (seq_event_cursor < seq_event_top + LIST_CURSOR_TOP) {
            seq_event_top += move;
        }
    } else if (move > 0) {
        seq_event_cursor += move;
        if (seq_event_cursor > EVENT_COUNT - 1) {
            seq_event_cursor = EVENT_COUNT - 1;
        }
        if (seq_event_cursor > seq_event_top + LIST_CURSOR_BOTTOM) {
            seq_event_top += move;
        }
    }
    if (seq_event_top < 0) {
        seq_event_top = 0;
    } else if (seq_event_top > EVENT_COUNT - LIST_ROWS) {
        seq_event_top = EVENT_COUNT - LIST_ROWS;
    }
    for (i = 0, event = seq_event_top; i < LIST_ROWS; i++, event++) {
        on = events_played[event] == 0 ? 0 : 1;
        if (seq_event_cursor == event) {
            njPrintColor(TEXT_CURSOR);
            njPrintC(NJM_LOCATION(13, i + 10), ">");
        }
        njPrintColor(TEXT_LIT);
        njPrintH(NJM_LOCATION(14, i + 10), event, 4);
        if (on == 1) {
            njPrintColor(TEXT_LIT);
        } else {
            njPrintColor(TEXT_DIM);
        }
        njPrintC(NJM_LOCATION(19, i + 10), seq_off_on[on]);
    }
    seq_draw_box(&seq_flag_box);
    return 0;
}

/* SCENE: a stage and act from the list, which wraps, and an entrance by
 * number. a choice fades out to the stage and act at the cursor, and sets
 * the entrance it starts at */
static int seq_scene_page(void)
{
    int i;
    int scene;
    int stage;
    int act;

    if (PAD_BUTTONS(pad_press_live[0], pad_cancel_buttons)) {
        return -1;
    } else if (PAD_BUTTONS(pad_press_live[0], pad_choose_buttons)) {
        Sint16 next_stage = seq_scenes[seq_scene_cursor][0];
        Sint16 next_act = seq_scenes[seq_scene_cursor][1];

        stage_fade_to(next_stage, next_act);
        stage_set_entrance(seq_scene_entrance);
        sound_play(SE_CHOOSE, 0, 0, 0);
    }
    if (pad_press_live[0] & PDD_DGT_TL) {
        seq_scene_mode--;
    } else if (pad_press_live[0] & PDD_DGT_TR) {
        seq_scene_mode++;
    }
    if (seq_scene_mode < 0) {
        seq_scene_mode = SCENE_MODES - 1;
    } else if (seq_scene_mode >= SCENE_MODES) {
        seq_scene_mode = 0;
    }
    switch (seq_scene_mode) {
    case SCENE_STAGE:
        njPrintColor(TEXT_CURSOR);
        njPrintC(NJM_LOCATION(14, 10), "SELECT STAGE");
        if ((pad_press_live[0] & PDD_DGT_KU) ||
            (pad_held_live[0] & PDD_DGT_KL)) {
            seq_scene_cursor--;
        } else if ((pad_press_live[0] & PDD_DGT_KD) ||
                   (pad_held_live[0] & PDD_DGT_KR)) {
            seq_scene_cursor++;
        }
        if (seq_scene_cursor < 0) {
            seq_scene_cursor = SEQ_SCENES - 1;
        } else if (seq_scene_cursor >= SEQ_SCENES) {
            seq_scene_cursor = 0;
        }
        for (scene = seq_scene_cursor + SEQ_SCENES - STAGE_CURSOR_ROW, i = 0;
             i < STAGE_ROWS_SHOWN; i++, scene++) {
            scene %= SEQ_SCENES;
            if (scene == seq_scene_cursor) {
                njPrintColor(TEXT_LIT);
            } else {
                njPrintColor(TEXT_DIM);
            }
            stage = seq_scenes[scene][0];
            act = seq_scenes[scene][1];
            debug_print_stage_name(NJM_LOCATION(14, i + 12),
                                   STAGE_ACT(stage, act));
        }
        break;
    case SCENE_ENTRANCE:
        njPrintColor(TEXT_CURSOR);
        njPrintC(NJM_LOCATION(14, 10), "SELECT ENTRANCE");
        if (pad_press_live[0] & PDD_DGT_KU) {
            seq_scene_entrance--;
        } else if (pad_press_live[0] & PDD_DGT_KD) {
            seq_scene_entrance++;
        }
        break;
    }
    seq_draw_box(&seq_scene_box);
    seq_draw_box(&seq_stage_box);
    njPrintColor(TEXT_LIT);
    njPrintC(NJM_LOCATION(14, 4), "STAGE:");
    debug_print_stage_name(NJM_LOCATION(20, 4),
                           STAGE_ACT(seq_scenes[seq_scene_cursor][0],
                                     seq_scenes[seq_scene_cursor][1]));
    njPrintC(NJM_LOCATION(14, 5), "ENTER:");
    njPrintD(NJM_LOCATION(20, 5), seq_scene_entrance, 2);
    return 0;
}

/* SEQ: the sections of the character's story, by name, with the story's
 * own section in TEXT_CURRENT. a choice puts the story before the section's
 * first step, and makes the section's stage, act and entrance the next. it
 * then ends the act as a clear. without player_record, the page shows only
 * its box */
static int seq_section_page(void)
{
    struct story_section *table;
    int length;
    int move;
    int i;
    int section;

    if (PAD_BUTTONS(pad_press_live[0], pad_cancel_buttons)) {
        return -1;
    }
    if (player_record != NULL) {
        move = 0;
        table = story_get_table(get_character());
        length = story_get_length(get_character());
        if (PAD_BUTTONS(pad_press_live[0], pad_choose_buttons)) {
            table += seq_section_cursor;
            player_record->seq = -1;
            player_record->section = seq_section_cursor;
            stage_set_entrance(table->entrance);
            stage_fade_to(table->stage, table->act);
            stage_end(STAGE_END_CLEAR);
            return -1;
        }
        if ((pad_press_live[0] & PDD_DGT_KU) ||
            (pad_held_live[0] & PDD_DGT_KL)) {
            move--;
        }
        if ((pad_press_live[0] & PDD_DGT_KD) ||
            (pad_held_live[0] & PDD_DGT_KR)) {
            move++;
        }
        if (move < 0) {
            seq_section_cursor += move;
            if (seq_section_cursor < 0) {
                seq_section_cursor = 0;
            }
            if (seq_section_cursor < seq_section_top + LIST_CURSOR_TOP) {
                seq_section_top += move;
            }
        } else if (move > 0) {
            seq_section_cursor += move;
            if (seq_section_cursor > length - 1) {
                seq_section_cursor = length - 1;
            }
            if (seq_section_cursor > seq_section_top + LIST_CURSOR_BOTTOM) {
                seq_section_top += move;
            }
        }
        if (length > LIST_ROWS) {
            if (seq_section_top < 0) {
                seq_section_top = 0;
            } else if (seq_section_top > length - LIST_ROWS) {
                seq_section_top = length - LIST_ROWS;
            }
            length = LIST_ROWS;
        } else {
            seq_section_top = 0;
        }
        njPrintColor(TEXT_CURSOR);
        njPrintC(NJM_LOCATION(18, 8), "SELECT SECTION");
        section = seq_section_top;
        for (i = 0; i < length; i++, section++) {
            if (seq_section_cursor == section) {
                njPrintColor(TEXT_CURSOR);
                njPrintC(NJM_LOCATION(13, i + 10), ">");
            }
            if (player_record->section == section) {
                njPrintColor(TEXT_CURRENT);
            } else {
                njPrintColor(TEXT_LIT);
            }
            njPrintC(NJM_LOCATION(14, i + 10), table[section].name);
        }
        njPrintC(NJM_LOCATION(14, 24), "SEQNO:");
        njPrintD(NJM_LOCATION(20, 24), player_record->seq, 3);
    }
    seq_draw_box(&seq_section_box);
    return 0;
}

/* COMPLT: whether the character has cleared each stage. a choice sets the
 * stage's count of clears to 1 if it is 0, and otherwise to 0. with X held,
 * a choice fades out to the stage's act 0 instead. the page takes each
 * row's number as its stage, even on rows 26 and 27, which name
 * STAGE_SHOOTING1's acts */
static int seq_complete_page(void)
{
    int character;
    int i;
    int row;
    int stage;
    int act;
    Sint8 *clears;

    character = get_character();
    if (PAD_BUTTONS(pad_press_live[0], pad_cancel_buttons)) {
        return -1;
    } else if (PAD_BUTTONS(pad_press_live[0], pad_choose_buttons)) {
        if (per_info[0]->on & PDD_DGT_TX) {
            sound_play(SE_CHOOSE, 0, 0, 0);
            story_go_to(seq_complete_cursor, 0);
            return -1;
        } else {
            stage_clears[character][seq_complete_cursor] =
                !stage_clears[character][seq_complete_cursor];
        }
    }
    njPrintColor(TEXT_CURSOR);
    njPrintC(NJM_LOCATION(14, 10), "SELECT STAGE");
    if ((pad_press_live[0] & PDD_DGT_KU) || (pad_held_live[0] & PDD_DGT_KL)) {
        seq_complete_cursor--;
    } else if ((pad_press_live[0] & PDD_DGT_KD) ||
               (pad_held_live[0] & PDD_DGT_KR)) {
        seq_complete_cursor++;
    }
    if (seq_complete_cursor < 0) {
        seq_complete_cursor = SEQ_STAGES - 1;
    } else if (seq_complete_cursor >= SEQ_STAGES) {
        seq_complete_cursor = 0;
    }
    /* @hack spelling: row and i set before clears. this gives i R10
     * (0x8c0ab412), keeps stage at @(4,R15) and act at @(8,R15) in the
     * loop's second unrolled copy and its remainder (0x8c0ab4b0 to
     * 0x8c0ab528, 0x8c0ab68c to 0x8c0ab6a6), and stores clears before the
     * MOV of STAGE_SHOOTING1 to R12 (0x8c0ab41c, 0x8c0ab41e); with both set
     * after clears, 52 of 415 words differ */
    row = seq_complete_cursor + SEQ_STAGES - STAGE_CURSOR_ROW;
    i = 0;
    clears = stage_clears[character];
    for (; i < STAGE_ROWS_SHOWN; i++, row++) {
        row %= SEQ_STAGES;
        if (row == seq_complete_cursor) {
            njPrintColor(TEXT_LIT);
        } else {
            njPrintColor(TEXT_DIM);
        }
        if (row == SEQ_STAGES - 2) {
            stage = STAGE_SHOOTING1;
            act = 0;
        } else if (row == SEQ_STAGES - 1) {
            stage = STAGE_SHOOTING1;
            act = 1;
        } else {
            stage = row;
            act = 0;
        }
        debug_print_stage_name(NJM_LOCATION(14, i + 12), STAGE_ACT(stage, act));
        njPrintC(NJM_LOCATION(28, i + 12), seq_no_yes[clears[row] != 0]);
    }
    seq_draw_box(&seq_stage_box);
    /* @bug uninitialized: seq_menu tests this page's result against -1
     * (0x8c0aa37c), and the page returns none. the image branches to
     * seq_draw_box last (0x8c0ab6f0), and draw_set_material and seq_draw_box
     * return without writing R0 again. the R0 seq_menu tests is 12, from
     * njSetConstantMaterial's MOV #12,R0 (0x8c6298a2). a return of 0 would
     * make the branch a call */
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

/* DEBUG: where player 0 is, and how it's turned */
static int seq_debug_page(void)
{
    if (PAD_BUTTONS(pad_press_live[0], pad_cancel_buttons)) {
        return -1;
    }
    njPrintC(NJM_LOCATION(15, 6), "PLPOS X:");
    njPrintC(NJM_LOCATION(21, 7), "Y:");
    njPrintC(NJM_LOCATION(21, 8), "Z:");
    njPrintF(NJM_LOCATION(24, 6), player_work[0]->pos.x, 4);
    njPrintF(NJM_LOCATION(24, 7), player_work[0]->pos.y, 4);
    njPrintF(NJM_LOCATION(24, 8), player_work[0]->pos.z, 4);
    njPrintC(NJM_LOCATION(15, 9), "PLANG X:");
    njPrintC(NJM_LOCATION(21, 10), "Y:");
    njPrintC(NJM_LOCATION(21, 11), "Z:");
    njPrintH(NJM_LOCATION(24, 9), player_work[0]->ang.x, 4);
    njPrintH(NJM_LOCATION(24, 10), player_work[0]->ang.y, 4);
    njPrintH(NJM_LOCATION(24, 11), player_work[0]->ang.z, 4);
    seq_draw_box(&seq_debug_box);
    return 0;
}

static void seq_draw_box(struct seq_box *box)
{
    NJS_COLOR colors[4];
    NJS_POINT2 points[4];
    NJS_POINT2COL polygon;

    polygon.p = points;
    polygon.col = colors;
    polygon.tex = NULL;
    polygon.num = 4;
    points[0].x = points[1].x = box->x;
    points[2].x = points[3].x = box->x + box->width;
    points[0].y = points[3].y = box->y;
    points[1].y = points[2].y = box->y + box->height;
    colors[0].color = colors[1].color = colors[2].color = colors[3].color =
        box->color;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njDrawPolygon2D(&polygon, 4, -1.0F, NJD_FILL | NJD_TRANSPARENT);
    colors[0].color = colors[1].color = colors[2].color = colors[3].color =
        TEXT_LIT;
    njDrawPolygon2D(&polygon, 4, -0.9F, NJD_DRAW_NORMAL);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}
