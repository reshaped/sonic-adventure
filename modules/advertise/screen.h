/* the menus' screens: the menus' task, which holds the screen the menus are
 * at, and the links between the screens */
#ifndef ADVERTISE_SCREEN_H
#define ADVERTISE_SCREEN_H

#include <sg_xpt.h>

#include "main.h"
#include "system/task.h"

/* the menu screens. SCREEN_NONE is no screen: it ends a list, and
 * screen_go(SCREEN_NONE) closes the menus */
enum screen {
    SCREEN_NONE,
    SCREEN_CHARACTER, /* the character wheel */
    SCREEN_FILE,
    SCREEN_TRIAL_STAGE,
    SCREEN_DRIVE,
    SCREEN_SOUND_TEST,
    SCREEN_CARD,
    SCREEN_MODE,
    SCREEN_OPTIONS,
    SCREEN_GAME_BUG,
    SCREEN_TRIAL_MINI_GAME,
    /* the menus' task's own states as it closes the menus. SCREEN_CLOSE
     * waits for the last screen to fade out. SCREEN_CLOSED sets the menus'
     * result, and SCREEN_DONE then holds */
    SCREEN_CLOSE,
    SCREEN_CLOSED,
    SCREEN_DONE
};

/* the screens a screen goes back to and goes on to, as enum screen numbers
 * them */
struct screen_links {
    int back;
    int next;
};

/* a screen of an entrance's list, with its links. a list ends at an entry
 * whose screen is SCREEN_NONE */
struct screen_entry {
    int screen;
    struct screen_links links;
};

/* a screen's own state, which its task keeps. a hidden screen opens once
 * ADVERTISE_WORK()->screen names it, and fades in until it is shown. a shown
 * screen closes once ADVERTISE_WORK()->last_screen names it, and fades out
 * until it is hidden again. the screen then sets ADVERTISE_WORK()->closed */
enum screen_mode {
    SCREEN_HIDDEN,
    SCREEN_OPENING,
    SCREEN_SHOWN,
    SCREEN_CLOSING
};

/* the menus' task's work */
struct advertise_work {
    const struct screen_entry *list; /* the entrance's list */
    /* the screen the menus are at, which opens. as the menus close, it takes
     * the states from SCREEN_CLOSE on */
    int screen;
    /* the screen before screen, which closes. it starts at SCREEN_NONE */
    int last_screen;
    /* the last screen to report itself shown, which each does at every step
     * it is shown. it starts as the list's first */
    int shown;
    /* what the menus close on: a menu_choice a screen sets, or OVERLAY_BACK
     * until one does */
    int choice;
    /* OVERLAY_RUNNING until the menus close. it is then choice, where choice
     * is OVERLAY_DONE or any menu_choice other than MENU_EXIT, and
     * OVERLAY_BACK otherwise */
    int result;
    int pending; /* the screen screen_go asked for */
    Sint16 timer; /* the steps left before the switch to pending, or 0 */
    Sint8 changing; /* 1 from screen_go until timer starts */
    /* 1 once a screen has faded out. the menus' task clears it at each step
     * it is at a screen */
    Uint8 closed;
    /* mode_hidden is 1 while mode select is hidden, and 0 from its first step
     * shown until it has faded out. wheel_hidden is the same for the wheel.
     * nothing reads either */
    Sint8 mode_hidden;
    Sint8 wheel_hidden;
};

/* the menus' task, which screen_create makes, or NULL */
extern struct task *advertise_task;

/* the menus' task's work. advertise_task must not be NULL */
#define ADVERTISE_WORK() ((struct advertise_work *)advertise_task->extra)

/* where screen_create starts the menus */
struct screen_start {
    enum menu_entrance entrance;
};

/* screen_create starts the menus' task, and returns it, or NULL. the menus
 * start at the first screen of start->entrance's list, whose screens' tasks
 * start 130 steps later. it also marks no texture loaded, clears the stage
 * picked for the trial, and creates the menu task. advertise_task must be NULL:
 * with a task there, screen_create never returns. screen_kill ends the menus'
 * task, if there is one, and every screen's task with it. it then sets
 * advertise_task to NULL */
struct task *screen_create(struct screen_start *start);
void screen_kill(void);
/* screen_go and screen_go_wheel need the menus' task live. screen_go stops the
 * music and voices at once, as advertise_sound_stop does. it switches to screen
 * on the tenth step the menus' task runs after the call. the screen the menus
 * were at then closes, and screen opens. screen_go(SCREEN_NONE) closes the
 * menus instead. a call before the switch replaces the screen of the call
 * before it, and starts the count again. screen_go_wheel sets the wheel's next
 * screen, which the wheel never reads, and switches to the wheel as screen_go
 * does */
void screen_go(int screen);
void screen_go_wheel(int next);

#endif
