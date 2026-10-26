/* the menus the menu screens show, by enum advertise_menu, and the calls
 * that show them */
#include <sg_xpt.h>

#include "advertise/file_select.h"
#include "advertise/menus.h"
#include "advertise/mini_game_select.h"
#include "advertise/mode_select.h"
#include "advertise/textures.h"
#include "advertise/trial_stage_select.h"
#include "hud/menu.h"

static const struct menu_item file_items[] = {
    {-155.0F, 0.0F, DLG_DECIDE},
    {0.0F, 0.0F, DLG_CANCEL},
    {155.0F, 0.0F, DLG_ERASE},
};
static const struct menu_item file_erase_items[] = {
    {-84.0F, 9.0F, DLG_DECIDE},
    {84.0F, 9.0F, DLG_CANCEL},
};
static const struct menu_item mode_items[] = {
    {4.0F, -60.0F, TITLE_ADVENTURE},
    {4.0F, -20.0F, TITLE_TRIAL},
    {4.0F, 20.0F, TITLE_OPTIONS},
    {4.0F, 60.0F, TITLE_INTERNET},
};
static const struct menu_item trial_items[] = {
    {4.0F, -30.0F, TITLE_ACTION},
    {4.0F, 30.0F, TITLE_MINI_GAME},
};
static const struct menu_item go_items[] = {
    {90.0F, 0.0F, DLG_DECIDE},
    {212.0F, 0.0F, DLG_CANCEL},
};
static const struct menu_item story_items[] = {
    {-155.0F, 0.0F, DLG_START},
    {0.0F, 0.0F, DLG_TUTORIAL},
    {155.0F, 0.0F, DLG_CANCEL},
};
static const struct menu_item story_cleared_items[] = {
    {-198.0F, 0.0F, DLG_START},
    {-66.0F, 0.0F, DLG_TUTORIAL},
    {66.0F, 0.0F, DLG_ENDING},
    {198.0F, 0.0F, DLG_CANCEL},
};
static const struct menu_item last_story_items[] = {
    {-155.0F, 0.0F, DLG_START},
    {155.0F, 0.0F, DLG_CANCEL},
};
static const struct menu_item last_story_cleared_items[] = {
    {-155.0F, 0.0F, DLG_START},
    {0.0F, 0.0F, DLG_ENDING},
    {155.0F, 0.0F, DLG_CANCEL},
};
static const struct menu_item speakers_items[] = {
    {-114.0F, 0.0F, OPTION_STEREO},
    {114.0F, 0.0F, OPTION_MONO},
};
static const struct menu_item messages_items[] = {
    {-114.0F, 0.0F, OPTION_VOICE_TEXT},
    {114.0F, 0.0F, OPTION_VOICE},
};
static const struct menu_item sound_test_items[] = {
    {0.0F, -140.0F, SNDTEST_CHARACTER}, {0.0F, -104.0F, SNDTEST_ACTION},
    {0.0F, -68.0F, SNDTEST_ADVENTURE},  {0.0F, -32.0F, SNDTEST_EVENT},
    {0.0F, 4.0F, SNDTEST_OTHER},        {0.0F, 40.0F, SNDTEST_JINGLE},
    {0.0F, 76.0F, SNDTEST_EFFECTS},     {0.0F, 136.0F, SNDTEST_BACK},
};
static const struct menu_item drive_items[] = {
    {-155.0F, 0.0F, DLG_START},
    {155.0F, 0.0F, DLG_CANCEL},
};

#define DIALOG_FRAME 0x97008740
#define TITLE_FRAME 0x7812b4ff

static const struct menu advertise_menus[] = {
    {MENU_ROW, NULL, &ava_dlg_texlist, file_items, &menu_default_sounds,
     DIALOG_FRAME, DIALOG_FRAME, 320.0F, 365.0F, 20.0F, 568.0F, 80.0F, 1.625F,
     0.8F, 3, 1},
    {MENU_ROW, file_erase_menu_draw, &ava_dlg_texlist, file_erase_items,
     &menu_default_sounds, DIALOG_FRAME, DIALOG_FRAME, 244.0F, 143.0F, 20.0F,
     336.0F, 136.0F, 1.625F, 0.8F, 2, 1},
    {MENU_COLUMN, mode_menu_draw, &ava_title_texlist, mode_items,
     &menu_default_sounds, TITLE_FRAME, TITLE_FRAME, 316.0F, 236.0F, 20.0F,
     232.0F, 200.0F, 0.719F, 0.5F, 4, 4},
    {MENU_COLUMN, trial_menu_draw, &ava_title_texlist, trial_items,
     &menu_default_sounds, TITLE_FRAME, TITLE_FRAME, 316.0F, 236.0F, 20.0F,
     232.0F, 160.0F, 0.719F, 0.5F, 2, 2},
    {MENU_ROW, trial_stage_menu_draw, &ava_dlg_texlist, go_items,
     &menu_default_sounds, DIALOG_FRAME, DIALOG_FRAME, 302.0F, 204.5F, 20.0F,
     568.0F, 88.0F, 1.625F, 0.8F, 2, 1},
    {MENU_ROW, mini_game_menu_draw, &ava_dlg_texlist, go_items,
     &menu_default_sounds, DIALOG_FRAME, DIALOG_FRAME, 302.0F, 204.5F, 20.0F,
     568.0F, 88.0F, 1.625F, 0.8F, 2, 1},
    {MENU_ROW, NULL, &ava_dlg_texlist, story_items, &menu_default_sounds,
     DIALOG_FRAME, DIALOG_FRAME, 302.0F, 182.0F, 20.0F, 568.0F, 80.0F, 1.625F,
     0.8F, 3, 2},
    {MENU_ROW, NULL, &ava_dlg_texlist, story_cleared_items,
     &menu_default_sounds, DIALOG_FRAME, DIALOG_FRAME, 302.0F, 182.0F, 20.0F,
     568.0F, 80.0F, 1.625F, 0.8F, 4, 3},
    {MENU_ROW, NULL, &ava_dlg_texlist, last_story_items, &menu_default_sounds,
     DIALOG_FRAME, DIALOG_FRAME, 302.0F, 182.0F, 20.0F, 568.0F, 80.0F, 1.625F,
     0.8F, 2, 1},
    {MENU_ROW, NULL, &ava_dlg_texlist, last_story_cleared_items,
     &menu_default_sounds, DIALOG_FRAME, DIALOG_FRAME, 302.0F, 182.0F, 20.0F,
     568.0F, 80.0F, 1.625F, 0.8F, 3, 2},
    {MENU_ROW, NULL, &ava_option_texlist, speakers_items, &menu_default_sounds,
     DIALOG_FRAME, DIALOG_FRAME, 320.0F, 365.0F, 20.0F, 568.0F, 80.0F, 1.625F,
     0.8F, 2, 2},
    {MENU_ROW, NULL, &ava_option_texlist, messages_items, &menu_default_sounds,
     DIALOG_FRAME, DIALOG_FRAME, 320.0F, 365.0F, 20.0F, 568.0F, 80.0F, 2.438F,
     0.8F, 2, 2},
    {MENU_COLUMN, NULL, &ava_sndtest_texlist, sound_test_items,
     &menu_default_sounds, 0x7800297c, 0x781864ff, 320.0F, 264.0F, 20.0F,
     352.0F, 336.0F, 0.813F, 0.375F, 8, 7},
    {MENU_ROW, NULL, &ava_dlg_texlist, drive_items, &menu_default_sounds,
     DIALOG_FRAME, DIALOG_FRAME, 320.0F, 360.0F, 20.0F, 568.0F, 80.0F, 1.625F,
     0.8F, 2, 1},
};

void advertise_menu_popup_default(int menu)
{
    menu_popup_default(&advertise_menus[menu]);
}

/* @hack type: advertise_menu_popup defined old-style. options_run then
 * passes its cursors as ints (MOV R0,R5 at 0x8c906972), and the function
 * narrows the cursor to a Sint8 (MOV.B R0,@(4,R15) at 0x8c903e3c). with a
 * prototype, options_run also keeps each cursor in its frame, runs longer,
 * the functions after it move, and 215 of its 321 words differ */
/* clang-format off */
/* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
void advertise_menu_popup(menu, cursor, disabled)
    int menu;
    Sint8 cursor;
    Sint8 *disabled;
/* clang-format on */
{
    menu_popup(&advertise_menus[menu], cursor, disabled);
}

void advertise_menu_show(int menu, Sint8 cursor, Sint8 *disabled)
{
    menu_show(&advertise_menus[menu], cursor, disabled);
}
