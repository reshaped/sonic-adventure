/* the menus the menu screens show, by number, and the calls that show them */
#ifndef ADVERTISE_MENUS_H
#define ADVERTISE_MENUS_H

#include <sg_xpt.h>

/* the menus, and the screens that use them */
enum advertise_menu {
    FILE_MENU, /* the file screen's: decide, cancel or erase the file */
    FILE_ERASE_MENU, /* whether to erase the file */
    MODE_MENU, /* mode select's: adventure, trial, options or internet */
    TRIAL_MENU, /* action stages or mini games */
    TRIAL_STAGE_MENU, /* go to the stage picked, or cancel */
    MINI_GAME_MENU, /* go to the mini game picked, or cancel */
    STORY_MENU, /* the wheel's: start the story, see its tutorial, or cancel */
    STORY_CLEARED_MENU, /* the same, with the staff roll, for a cleared story */
    LAST_STORY_MENU, /* the last story's, which has no tutorial */
    LAST_STORY_CLEARED_MENU,
    SPEAKERS_MENU, /* the options screen's: stereo or mono */
    MESSAGES_MENU, /* voices and text, or voices alone */
    SOUND_TEST_MENU, /* the sound test's kinds of sound, and back */
    DRIVE_MENU /* the drive screen's: start or cancel */
};

/* these do what menu_popup_default, menu_popup and menu_show do, for menu, an
 * advertise_menu. advertise_menu_popup has no prototype: it takes (int menu,
 * Sint8 cursor, Sint8 *disabled) */
void advertise_menu_popup_default(int menu);
/* NOLINTNEXTLINE(*-strict-prototypes,*-deprecated-non-prototype) */
void advertise_menu_popup();
void advertise_menu_show(int menu, Sint8 cursor, Sint8 *disabled);

#endif
