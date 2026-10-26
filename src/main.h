/* the main loop: the game's start, its modes, and the hand-off to
 * LOADER.BIN */
#ifndef MAIN_H
#define MAIN_H
#include <ninja.h>
#include <sg_xpt.h>

/* what game_mode_step runs. MENU, SUMMARY, TUTORIAL and STAFF_ROLL load
 * their overlay and start it. they then move on to their _RUN mode, the
 * next value, which steps the overlay in the same call and every later
 * one. where the summary's start returns OVERLAY_EMPTY, SUMMARY moves to
 * the story in GAME_MODE_FIELD instead */
enum game_mode {
    GAME_MODE_BOOT = 0,
    GAME_MODE_LOGO = 1,
    GAME_MODE_TITLE = 2, /* the title, and the way to the stage select */
    GAME_MODE_STAGE_SELECT = 3,
    GAME_MODE_ACTION = 4, /* a stage with an end to reach */
    GAME_MODE_FIELD = 5, /* a field or a garden */
    /* the movie numbered movie_request. the opening, or the pads' quit, goes
     * on to the menus, and any other movie to a stage */
    GAME_MODE_MOVIE = 8,
    GAME_MODE_TRIAL = 9, /* a stage picked from the menus */
    GAME_MODE_MENU = 10, /* ADVERTISE's menus */
    GAME_MODE_MENU_RUN = 11,
    /* the summary of the story so far, then the story */
    GAME_MODE_SUMMARY = 14,
    GAME_MODE_SUMMARY_RUN = 15,
    GAME_MODE_TUTORIAL = 16, /* how to play, then the summary */
    GAME_MODE_TUTORIAL_RUN = 17,
    GAME_MODE_STAFF_ROLL = 18, /* the staff roll, then the logo */
    GAME_MODE_STAFF_ROLL_RUN = 19
};

/* what an overlay's start and step return. OVERLAY_EMPTY is -1 too, but
 * only the summary's start returns it */
enum overlay_result {
    /* the pads' quit, as pad_quit_pressed reports it. the main loop releases
     * the textures, resets Ninja and goes back to the menus' first screen */
    OVERLAY_RESET = -2,
    /* the player backs out of the overlay. the main loop goes from the menus
     * to the title, and from the summary, the tutorial or the staff roll to
     * the menus */
    OVERLAY_BACK = -1,
    OVERLAY_RUNNING = 0,
    OVERLAY_DONE = 1,
    /* the story has no page yet, so the summary has none to show */
    OVERLAY_EMPTY = -1
};

/* the values ADVERTISE's step returns besides overlay_result's */
enum menu_choice {
    MENU_TRIAL = 100, /* the stage or mini game picked for the trial */
    MENU_ADVENTURE = 101, /* the story of the character picked */
    /* the Internet item. the main loop leaves for LOADER.BIN, which starts
     * Dream Passport */
    MENU_INTERNET = 102,
    MENU_STAFF_ROLL = 103,
    MENU_TUTORIAL = 104,
    /* no menu screen chooses it, and the main loop ignores it */
    MENU_UNK_106 = 106,
    MENU_DEMO = 107, /* nobody pressed Start in time */
    /* the pads' quit on the card screen. the main loop ends the system and
     * exits to the boot ROM */
    MENU_EXIT = 108
};

/* which ADVERTISE menu screen to enter or resume */
enum menu_entrance {
    MENU_FROM_TOP = 0, /* at their first screen */
    /* backed out of the summary, the tutorial or the staff roll */
    MENU_FROM_SUMMARY = 1,
    MENU_FROM_TRIAL = 2 /* once the stage picked from the menus ends */
};

/* capacity of Ninja's texture memory list */
#define TEXTURE_COUNT 2048

/* the screen's size and middle, in pixels */
#define SCREEN_WIDTH 640.0F
#define SCREEN_HEIGHT 480.0F
#define SCREEN_MIDDLE_X (SCREEN_WIDTH / 2)
#define SCREEN_MIDDLE_Y (SCREEN_HEIGHT / 2)

/* SHC lays out bss in the order of each name's first declaration. each
 * name below is first declared here, even one only main.c uses */

extern enum menu_entrance menu_entrance;
/* the cable syCblCheckCable found at boot, which MOVIE.BIN reads too */
extern int cable_mode;
extern enum game_mode game_mode;
/* Ninja's texture memory list, which display_reset_full gives it */
extern NJS_TEXMEMLIST texmemlist[];
/* set once the disc's tray opens, or the disc changes */
extern Bool loader_error_flag;
/* system_boot_init clears dat_8c78b4d8 and dat_8c78b4dc, and nothing reads
 * either */
extern int dat_8c78b4d8;
extern int dat_8c78b4dc;
/* nothing reads or writes it */
extern int dat_8c78b4e0;
extern Float shape_buffer[]; /* the buffer njInitShape takes */
extern NJS_VIEW view;
extern NJS_SCREEN screen;
extern NJS_FOG_TABLE fog_table;

/* run the game a frame at a time until it leaves, by game_leave or the
 * menus' Internet item. then read LOADER.BIN, end the system and jump to
 * LOADER.BIN. it doesn't return */
void main(void);
/* end the main loop after this frame, for LOADER.BIN */
void game_leave(void);
/* set up the system, the streams, the play history and the disc's error
 * hook, as the game boots */
void game_init(void);
/* run a frame of game_mode. return -1 after the stage select's reset, and
 * otherwise 0. main discards the result */
int game_mode_step(void);
/* release the textures as game_release_textures does, then end the system.
 * nothing in 1ST_READ.BIN calls it */
void game_shutdown(void);
/* release the regular and character textures, and Ninja's texture memory,
 * as main leaves for LOADER.BIN */
void game_release_textures(void);
/* release every texture Ninja holds, as a stage loads */
void texture_release_all(void);
/* reset Ninja's matrix stack, its screen to 640x480 with a 70-degree
 * perspective, and its view. reset its texture memory, drawing modes, fog
 * and culling too, and reseed rand() with 0xdead0cab */
void display_reset_full(void);
/* start the system for the cable syCblCheckCable finds, VGA or NTSC, and
 * pace frames at two vsyncs and two steps. set up sound, Ninja's buffers,
 * the VMU and the memory cards, and point per_info at the four ports */
void system_boot_init(void);

#endif
