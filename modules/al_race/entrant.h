/* the Chao race's entrants: their places, on the chao window's board and at
 * the hall's gates, and the Chao that take them */
#ifndef AL_RACE_ENTRANT_H
#define AL_RACE_ENTRANT_H
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "al_race/racer.h"
#include "system/task.h"

/* an entrant's flags */
#define ENTRANT_HELD 0x1 /* the race holds the place */
#define ENTRANT_CHAO 0x2 /* a Chao has come for the place */
#define ENTRANT_READY 0x4 /* the place is held and its Chao has come */
/* the Chao's step draws the Chao while this is set. each time entrant_step
 * finds the Chao in view, it sets this in ENTRANT_BOARD and the states
 * after, and clears it in those before. out of view, the flag keeps its
 * value */
#define ENTRANT_SHOW 0x10
/* the Chao's next step that draws the Chao draws its name too, and clears
 * this */
#define ENTRANT_NAME 0x20
/* the place shows an egg while any of these is set. it shows the gold egg
 * for ENTRANT_GOLD, else the silver for ENTRANT_PAL, else the black for
 * ENTRANT_BEGG, else the plain one */
#define ENTRANT_EGG 0x80
#define ENTRANT_BEGG 0x100
#define ENTRANT_PAL 0x200
#define ENTRANT_GOLD 0x400

/* a place's states. the race sets the even ones, and entrant_step tells
 * each to the place's Chao. to tell a state, it moves the state on to its
 * _TOLD state, the odd one after it, and leaves the Chao the state's
 * command */
enum entrant_state {
    ENTRANT_RESET, /* as entrant_init leaves it, and in STATE_ENTRANCE */
    ENTRANT_RESET_TOLD,
    ENTRANT_BOARD, /* on the chao window's board */
    ENTRANT_BOARD_TOLD,
    ENTRANT_CURSOR, /* on the board, with the cursor on it */
    ENTRANT_CURSOR_TOLD,
    ENTRANT_ENTERED, /* on the board, with its Chao entered */
    ENTRANT_ENTERED_TOLD,
    ENTRANT_GATES, /* at the hall's gates */
    ENTRANT_GATES_TOLD
};

/* the commands entrant_step leaves a place's Chao, one for each state it
 * tells. they are codes the Chao's step matches, in no order. the Chao's
 * step takes each but ENTRANT_TELL_RESET, which nothing takes */
#define ENTRANT_TELL_RESET 3
#define ENTRANT_TELL_BOARD 1
#define ENTRANT_TELL_CURSOR 15
#define ENTRANT_TELL_ENTERED 7
#define ENTRANT_TELL_GATES 2

/* a place, and the Chao that takes it */
struct entrant {
    Uint8 unk_00;
    /* the command entrant_step left the Chao, or 0 once the Chao takes it */
    Uint8 command;
    Uint8 state; /* an enum entrant_state */
    Uint8 unk_03;
    Uint16 flags;
    Uint8 unk_06[2];
    NJS_POINT3 pos;
    struct alife_chao *chao; /* the Chao, its task's motion, or NULL */
    struct task_work *work; /* the Chao's task's work, or NULL */
};

/* the race's places, one to a lane, and the state of the hall's windows.
 * the places stand on the chao window's board, and then at the hall's
 * gates */
struct race_places {
    struct entrant places[RACE_LANES];
    Uint8 unk_e0;
    /* the garden window's cursor, 0 to 4, with 4 for back. once the player
     * chooses one of the others, kind becomes the kind of Chao the chao
     * window shows. that is a garden's, by enum garden, or ENTRY_CARD for
     * the cards' */
    Sint8 kind;
    Sint8 cursor; /* on a place, 0 to 7, or 8 for back and 9 for start */
    Uint8 step; /* the open window's step, which opening a window sets to 0 */
    Uint16 timer; /* each hall step adds 1, and opening a window sets it to 0 */
};

/* an entrant's Chao keeps its place in its work's vars */
struct entrant_vars {
    Uint8 unk_00[8];
    struct entrant *entrant;
};

#define ENTRANT_VARS(work) ((struct entrant_vars *)&(work)->vars)

/* hold the place at pos, in ENTRANT_RESET with no command, and return it. a
 * Chao that came for the place before makes it ready. without one, the place
 * forgets any Chao it had */
struct entrant *entrant_init(struct entrant *entrant, NJS_POINT3 *pos);
/* give the place the Chao that comes for it: chao, at its task's motion, and
 * work, its task's work. a held place becomes ready */
void entrant_set_chao(struct entrant *entrant, struct task_work *work,
                      struct alife_chao *chao);
/* TRUE while the place is held and its Chao has come, else FALSE */
Bool entrant_ready(struct entrant *entrant);
/* free the place: clear its flags and forget its Chao */
void entrant_clear(struct entrant *entrant);
/* destroy the Chao's task if the place is ready, then free the place as
 * entrant_clear does */
void entrant_remove(struct entrant *entrant);
/* move the place to pos, in ENTRANT_RESET with no command */
void entrant_reset(struct entrant *entrant, NJS_POINT3 *pos);
/* draw the place's egg, if it shows one. while the place is ready and
 * draw_in_view finds its Chao, tell the Chao any state the race has set, and
 * set or clear ENTRANT_SHOW. telling ENTRANT_BOARD, ENTRANT_CURSOR or
 * ENTRANT_ENTERED also puts the Chao on the place */
void entrant_step(struct entrant *entrant);
/* TRUE while the place's Chao is entered, in ENTRANT_ENTERED or
 * ENTRANT_ENTERED_TOLD, else FALSE */
Bool entrant_entered(struct entrant *entrant);
/* TRUE while the cursor is on the place, in ENTRANT_CURSOR or
 * ENTRANT_CURSOR_TOLD, else FALSE */
Bool entrant_at_cursor(struct entrant *entrant);
/* TRUE while the place shows an egg, else FALSE */
Bool entrant_shows_egg(struct entrant *entrant);
/* TRUE if command is the one the place left its Chao, which this then
 * clears. FALSE otherwise */
Bool entrant_take_command(struct entrant *entrant, int command);
/* store in screen where the point 2.5 above the place falls on the screen,
 * with z 0 */
void entrant_screen_pos(struct entrant *entrant, NJS_POINT3 *screen);

#endif
