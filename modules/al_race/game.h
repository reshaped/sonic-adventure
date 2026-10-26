/* the Chao race's game, which race_game_create makes, and what the race's
 * objects ask of it */
#ifndef AL_RACE_GAME_H
#define AL_RACE_GAME_H
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/cards.h"
#include "al_race/cheer.h"
#include "al_race/race_camera.h"
#include "al_race/race_path.h"
#include "al_race/racer.h"
#include "al_race/standings.h"
#include "chao/chao.h"

struct race_places;
struct task;

/* the race's acts: in ACT_HALL the race is chosen and its Chao entered, and
 * in ACT_COURSE it is run */
#define ACT_HALL 0
#define ACT_COURSE 1

/* the game's states, named as its debug page labels them */
enum race_state {
    STATE_ENTRANCE_INIT,
    STATE_MEDAL_GET,
    STATE_VMS_ERROR_CHECK,
    STATE_ENTRANCE_RE_INIT,
    STATE_ENTRANCE_INIT2,
    STATE_ENTRANCE,
    STATE_HINT_BOX,
    STATE_HINT_BOX_WAIT,
    STATE_RACE_WINDOW_INI,
    STATE_RACE_WINDOW,
    STATE_CUPS_WINDOW_INI,
    STATE_CUPS_WINDOW,
    STATE_GARDEN_WINDOW_INI,
    STATE_GARDEN_WINDOW,
    STATE_CHAO_WINDOW_INI,
    STATE_CHAO_WINDOW,
    STATE_CHAO_WINDOW_SLIDE,
    STATE_CHAO_DEMO_0,
    STATE_CHAO_DEMO_1,
    STATE_CHAO_DEMO_2,
    STATE_CHAO_DEMO_3,
    STATE_INIT,
    STATE_RE_INIT,
    STATE_WAIT,
    STATE_RACE,
    STATE_RACEEND,
    STATE_COMMENDATION_INI,
    STATE_COMMENDATION_0,
    STATE_COMMENDATION_1,
    STATE_COMMENDATION_2
};

/* in a game's flags */
/* the course step goes back to the hall once race_run returns */
#define GAME_TO_HALL 0x1
/* race_suspend_player has suspended player 0. race_resume_player resumes
 * it and clears the flag */
#define GAME_SUSPENDED 0x2
/* the act's step asks for the other act. race_game_step clears it and
 * returns TRUE */
#define GAME_CHANGE_ACT 0x4
/* the hall's next STATE_ENTRANCE step goes to the course */
#define GAME_TO_COURSE 0x8
/* the crowd cheers a racer this step, and the followed lane holds */
#define GAME_CHEERING 0x10
/* an entry that is no rival has come first, in a race of RACE_RECORDED */
#define GAME_WON 0x20
/* GAME_WON's win is the records' first win of the race's kind.
 * STATE_ENTRANCE_INIT then gives the save the kind's emblem, where it lacks
 * it */
#define GAME_FIRST_WIN 0x40
/* a card entry's new time or win wasn't saved, as its card or its Chao was
 * gone. STATE_VMS_ERROR_CHECK shows a message for it until a press on pad
 * 0 clears it */
#define GAME_NOT_SAVED 0x80
/* each race_game_step takes the drives whose card is gone out of the
 * cards' remaining */
#define GAME_WATCH_CARDS 0x100
#define GAME_POLL_CARDS 0x200 /* each hall step polls the drives */
/* race_gates_fill sets it while the records hold a win of RACE_KIND_LAST. it
 * then asks for the first rival it makes in RIVAL_BLACK_EGG, and clears the
 * flag. in RACE_MATCHED, where the source's skills sum to 36000 or more,
 * race_gates_fill asks for every rival in RIVAL_SILVER_EGG or RIVAL_GOLD_EGG
 * instead. the flag then stays set */
#define GAME_RIVAL_3 0x400
/* a race has started since the game was made. STATE_ENTRANCE_INIT2 then
 * starts the hall's music again */
#define GAME_RACED 0x800

/* an entry's kind: a garden's Chao, by enum garden, a card's, or a rival */
#define ENTRY_CARD 3
#define ENTRY_RIVAL 4

/* what races in a lane: an entry's kind, and its index by kind */
struct race_entry {
    Uint8 kind;
    /* by kind: the Chao's place in its garden's list, the card's drive, or
     * the rival's record in the game's chao. race_clear_entries sets 8,
     * past every drive and record, until the entry is made */
    Uint8 index;
};

/* the race's time so far, and the kind's best time, drawn beside it while
 * best isn't null */
struct race_clock {
    struct race_time time;
    const struct race_time *best;
};

/* the best times the records keep for each kind of race */
#define RACE_BEST 3

/* a race's modes, as the race window offers them. a race of RACE_RECORDED or
 * RACE_MATCHED takes one entry, and of RACE_GROUP up to RACE_LANES. only
 * RACE_RECORDED records a win. the others' rivals come from the strongest
 * entered Chao */
enum { RACE_RECORDED, RACE_MATCHED, RACE_GROUP };

/* a race's kinds run from 0 to CHAO_RACES - 1. the cups window shows
 * RACE_KIND_LAST only while the records hold a win of each of the others */
#define RACE_KIND_LAST 4
/* the records' bit for a win of kind */
#define RACE_KIND_WON(kind) (1 << (kind))

/* chao_get_file_data's bytes, as the race keeps its records in them */
struct race_records {
    Uint8 mark[8]; /* the race's, as race_records_reset_unmarked writes it */
    /* RACE_KIND_WON(kind) for each kind an entry that is no rival has won */
    Uint32 flags;
    /* each kind's best times, the shortest first, and a name for each */
    Uint8 names[CHAO_RACES][RACE_BEST][CHAO_NAME_SIZE];
    struct race_time times[CHAO_RACES][RACE_BEST];
};

/* what race_game_create allocates, cleared */
struct race_game {
    /* a Chao for each drive, as race_load_cards reads them. a rival takes
     * the record of a drive no card entry has */
    struct chao chao[RACE_LANES];
    struct racer racers[RACE_LANES]; /* by lane */
    struct race_camera camera;
    /* the hall's places, which the game makes as it enters the hall and
     * frees as it leaves. null on the course, and where heap_calloc fails */
    struct race_places *places;
    struct race_records *records;
    struct race_entry entries[RACE_LANES]; /* by lane */
    struct race_standings standings;
    struct race_path path; /* the course and the paths beside it */
    struct race_clock clock;
    struct race_cards cards;
    struct race_cheer cheer;
    Sint8 lane; /* the lane the race follows */
    /* the lane followed before the first racer home took the camera in
     * RACE_CAMERA_FRONT. the game follows it again when that mode's timer
     * passes 150, unless another mode has taken the camera first */
    Sint8 saved_lane;
    Uint8 state; /* an enum race_state */
    /* the race window's choice: a race's mode, or 3 to go back */
    Sint8 mode;
    /* the race's, from 0 to 4, or 5 for back while the cups window shows */
    Sint8 kind;
    /* the choice the cursor was on when up or down took it to the last
     * choice. up or down there takes the cursor back to it */
    Sint8 saved_choice;
    Sint8 character; /* player 0's, as the game started */
    /* the lane STATE_COMMENDATION_0 looks at next for a record to save */
    Sint8 result_lane;
    Float average_distance; /* the racers' mean distance along the course */
    /* what guides the racers each STATE_RACE step: race_guide_racers */
    void (*guide)(struct race_game *game);
    Uint16 flags;
    Uint16 timer; /* steps, which most changes of state clear */
    Uint16 steps; /* race_game_step's count, which wraps from 0xffff to 0 */
    Uint8 unk_18c6[3];
    Sint8 entry_count; /* the entries the chao window has made */
    /* the course steps since the camera changed mode. in RACE_CAMERA_FOLLOW
     * and in STATE_WAIT, a change of the followed lane clears it too */
    Uint16 camera_timer;
    Uint8 card_drives; /* 1 << drive for each card entry */
    /* the emblem count's task while STATE_MEDAL_GET shows it, else null */
    struct task *emblem_display;
};

/* the game while one runs, or null */
extern struct race_game *race_game_current;

/* allocate the game, cleared, make it current, and set it up in
 * STATE_ENTRANCE_INIT with the menu slots. the card library restarts on
 * AL_MAIN.PRS's work and callbacks, so AL_MAIN.PRS must stay loaded until
 * race_game_free returns. return the game, or null when the heap can't give
 * it. with a game already current, return null, and the allocation leaks */
struct race_game *race_game_create(void);
/* run the hall's step or the course's, by the stage's act. TRUE when the
 * step asks for the other act, and a hall step then frees the places. game
 * must not be null */
Bool race_game_step(struct race_game *game);
/* call card_init through card_drives_end, then return if game is null.
 * otherwise make no game current and free game's path spans and places.
 * resume player 0 where the game suspended it, free the menu slots, close
 * the menu panel, and free the game */
void race_game_free(struct race_game *game);
/* the lane of the racer whose Chao's task is chao_task, or -1 */
int race_lane_of_task(struct task *chao_task);
/* release the camera, put it in mode at its first step, and clear the
 * camera timer */
void race_set_camera_mode(struct race_game *game, int mode);
/* store in pos where the Chao in lane stands, and return 1. with no game,
 * no Chao in lane or a lane above 7, store nothing and return 0. lane must
 * not be negative */
int race_get_racer_pos(NJS_POINT3 *pos, int lane);

/* the game's debug page, printed with njPrint */

/* print the state's label at column x of row y */
void race_print_state(struct race_game *game, int x, int y);
/* print the state's label at column 7 of row 11, and the constant 180 in
 * five hex digits at column 7 of row 5 */
void fun_0cb8c942(struct race_game *game);
/* print the state's number at column 7 of row 14 and the timer at row 15.
 * from row 17, racer_print_motion prints the followed racer */
void fun_0cb8c956(struct race_game *game);
/* read pad 0's debug buttons. y held sets the followed racer's
 * RACER_CHEERED. l gives its Chao a run of 9999, the other skills 9500 and
 * no race wins, and needs the racer's Chao. r sets GAME_WON. then print the
 * flags at row 4 and the cards that remain at row 6 */
void fun_0cb8c9e0(struct race_game *game);

/* each STATE_RACE step, give each racer its route's behavior. at the start,
 * a race of kind 0, 1 or RACE_KIND_LAST sends each racer toward its lane's
 * race_course target. on the way, a RACER_CURIOUS racer whose Chao touches a
 * small zone goes for the item dealt to its lane instead. once it reaches the
 * item, or fails to, it is no longer curious and takes the idle behavior */
void race_guide_racers(struct race_game *game);

/* ask for one of group's voices at random: one of its first ones, or with
 * later set, one of the rest. a group above 5 asks for none. group must not
 * be negative */
void race_request_voice(int group, int later);
/* print the 8, 16 or 32 bits at value, from the lowest, from column x of
 * row y. a set bit prints O, and a clear one X. the next row has ^ under
 * every fourth bit, from the first */
void race_print_bits8(void *value, int x, int y);
void race_print_bits16(void *value, int x, int y);
void race_print_bits32(void *value, int x, int y);

/* the functions from race_get_kind to race_cracker_kind read
 * race_game_current. with no game current, each returns 0 and changes
 * nothing, unless its own comment says otherwise */

/* store the race's kind, or for race_get_followed_lane the lane it follows,
 * and return 1 */
int race_get_kind(int *kind);
int race_get_followed_lane(int *lane);
/* with which 0, store the places' timer and return 1. 0 when there are no
 * places, or for any other which */
int fun_0cb8b510(int *timer, int which);
/* 1 while the camera is in RACE_CAMERA_AHEAD, RACE_CAMERA_COURSE_FIXED,
 * RACE_CAMERA_WATCH or RACE_CAMERA_FINISH, else 2 */
int fun_0cb8b566(void);
/* store the kind of lane's entry and return 1. 0 for a kind above
 * ENTRY_RIVAL. lane must be 0 to 7 */
int race_get_entry_kind(int *kind, int lane);
/* in a state the table lists for it, each returns 1. while there are
 * places, each also opens the window it names at its first state, as
 * race_open_race_window sets STATE_RACE_WINDOW_INI. it then clears the
 * game's timer and the places' timer and step. in any other state, each
 * returns 0:
 *
 *   race_open_race_window     STATE_ENTRANCE, STATE_CUPS_WINDOW
 *   race_open_cups_window     STATE_RACE_WINDOW, STATE_GARDEN_WINDOW
 *   race_open_garden_window   STATE_CUPS_WINDOW, STATE_GARDEN_WINDOW,
 *                             STATE_CHAO_WINDOW
 *   race_open_chao_window     STATE_GARDEN_WINDOW */
int race_open_race_window(void);
int race_open_cups_window(void);
int race_open_garden_window(void);
int race_open_chao_window(void);
/* 0 while a game is current, else -1 */
int fun_0cb8b7ea(void);
/* the racer in place, from 0 for the first, or null. place must be 0 to
 * 7 */
struct racer *race_racer_at_place(int place);
/* a value of the racer in place, by what. 0 gives its lane, and 1 its
 * distance along the course cut to a whole number. 2 to 5 give its Chao's
 * run, swim, fly and power. -1 for any other what, or with no game
 * current */
int fun_0cb8b860(int place, int what);
/* 2 when the first place's entry is no rival. else 1 when the second or
 * third place's is no rival, else 0 */
int race_cracker_kind(void);

/* how race_draw_digit draws a digit: its place, its scale and its color */
struct race_digit {
    Float x;
    Float y;
    Float sx;
    Float sy;
    NJS_ARGB color;
};

/* draw digit value, from digit set 1 when set isn't 0 and else set 0, at
 * digit's place, scale and color */
void race_draw_digit(struct race_digit *digit, Int value, int set);

/* the race's clock, and its records of the best times */

/* clear the clock's time */
void race_clock_clear(struct race_clock *clock);
/* point the clock's best at kind's shortest time in records */
void race_clock_set_best(struct race_clock *clock, struct race_records *records,
                         int kind);
/* count a step on the clock's time. 60 steps make a second, and the
 * minutes stop at 100 */
void race_clock_step(struct race_clock *clock);
/* draw the clock's time, and its best when best isn't null. then draw their
 * two labels, frames 7 and 8 of menu_sprites */
void race_clock_draw(struct race_clock *clock);
/* reset records unless their mark is the race's 8 bytes. a reset writes the
 * mark, clears the flags, and sets each kind's three best times to 8, 9 and
 * 10 minutes. it writes a default name's letters over the start of each
 * name */
void race_records_reset_unmarked(struct race_records *records);
/* where time is shorter than kind's third best, it takes that place, with
 * chao's name. it then moves up past each longer time */
void race_records_enter(struct race_records *records, struct race_time *time,
                        struct chao *chao, int kind);
/* draw kind's three best times, each with its name after it and frame 3,
 * 4 or 5 of menu_sprites before it. drawing a name sets its eighth byte to
 * null */
void race_records_draw(struct race_records *records, int kind);
/* copy the time at src to dst. race_time_copy takes dst first, and
 * race_time_copy_to takes src first */
void race_time_copy(struct race_time *dst, const struct race_time *src);
void race_time_copy_to(const struct race_time *src, struct race_time *dst);
/* compare two times: 1 when left is longer than right, 0 when it is
 * shorter, and -1 when the two are equal */
int race_time_compare(const struct race_time *left,
                      const struct race_time *right);
/* exchange the times at first and second, and return 1 */
int race_time_swap(struct race_time *first, struct race_time *second);

#endif
