/* the Chao race's game. in the hall, its windows choose the race and enter
 * the Chao, which then stand at the gates. on the course, the game runs the
 * race and the crowd's cheer, then the commendation, which saves the card
 * entries' new times and wins */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "al_main/card.h"
#include "al_main/chao.h"
#include "al_race/cards.h"
#include "al_race/cheer.h"
#include "al_race/confetti.h"
#include "al_race/course.h"
#include "al_race/cracker.h"
#include "al_race/entrant.h"
#include "al_race/entrant_chao.h"
#include "al_race/fade.h"
#include "al_race/game.h"
#include "al_race/gate.h"
#include "al_race/menu.h"
#include "al_race/race_camera.h"
#include "al_race/race_chao.h"
#include "al_race/race_path.h"
#include "al_race/race_rival.h"
#include "al_race/racer.h"
#include "al_race/scenery.h"
#include "al_race/standings.h"
#include "chao/chao.h"
#include "collision/polygon.h"
#include "debug/editor.h"
#include "effect/sparkle.h"
#include "event/script.h"
#include "player/player.h"
#include "progress/emblem.h"
#include "save/card.h"
#include "save/save.h"
#include "sound/cue.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* the race's scripts, for script_set_table */
extern struct script_row race_script_table[];

/* where race_place_chao makes the Chao */
enum race_place {
    PLACE_BOARD, /* on the chao window's board: a garden's Chao or the cards' */
    PLACE_GATES, /* at the hall's gates, with rivals for the empty lanes */
    PLACE_STARTS /* at the course's starts, as racers */
};

/* each window's last choice, which goes back. the race window's follows
 * its three modes, and the cups window's its five kinds. the garden
 * window's follows the cards and the three gardens */
#define RACE_WINDOW_BACK 3
#define CUPS_WINDOW_BACK 5
#define GARDEN_WINDOW_BACK 4

/* the cursor's values for back and start, below the board's places 0 to 7 */
#define BOARD_BACK 8
#define BOARD_START 9

/* the steps STATE_WAIT follows each lane for, as it shows the racers */
#define LANE_SHOW_STEPS 48
/* STATE_RACE ends once the game's timer passes this, whoever has finished */
#define RACE_TIMEOUT 21000

/* each lane's cheer. while the crowd waits to choose a racer, a press
 * sounds the followed lane's. while the crowd cheers lane 6 or 7, a press
 * sounds SE_CHEER_6 or SE_CHEER_7, at random */
#define SE_CHEER_0 0x28c
#define SE_CHEER_1 0x28f
#define SE_CHEER_2 0x290
#define SE_CHEER_3 0x291
#define SE_CHEER_4 0x292
#define SE_CHEER_5 0x293
#define SE_CHEER_6 0x28d
#define SE_CHEER_7 0x28e

/* the hall's sounds */
#define SE_PICK 0x347 /* a place on the board is picked */
#define SE_ENTER 0x348 /* the Chao on the cursor's place is entered */
#define SE_TO_GATES 0x264 /* the entrants are put at the gates */

/* the course's sounds. SE_START_1 and SE_START_2 sound together as the race
 * starts */
#define SE_START_1 0x28b
#define SE_START_2 0x4f4
#define SE_FINISH 0x4f5 /* the racer in first place reaches its goal */
#define SE_WON 0x4f6 /* the commendation's, with GAME_WON */
/* the commendation's in RACE_RECORDED without GAME_WON, where the records'
 * flags pass STATE_COMMENDATION_INI's test, which its @bug logic
 * describes */
#define SE_NOT_WON 0x4f7
/* kept playing while the camera is in RACE_CAMERA_FINISH */
#define SE_WINNER 0x44a

/* while the crowd's cheer waits, a racer more than this ahead of the
 * racers' mean distance walks */
#define LEAD 160.0F

/* true when any of the four pads presses one of buttons */
#define ANY_PRESS(buttons)                                                     \
    ((per_info[0]->press & (buttons)) || (per_info[1]->press & (buttons)) ||   \
     (per_info[2]->press & (buttons)) || (per_info[3]->press & (buttons)))
/* true when any pad presses a, b, x or y, and the set editor is closed */
#define CHEER_PRESS()                                                          \
    (!edit_mode && ANY_PRESS(PDD_DGT_TA | PDD_DGT_TB | PDD_DGT_TX | PDD_DGT_TY))

static void race_hall_step(struct race_game *game);
static void race_course_step(struct race_game *game);
static void race_course_draw(struct race_game *game);
static void race_chao_start(struct task *task);
static void entrant_chao_start(struct task *task);
static int race_game_init(struct race_game *game);
static void race_clear_entries(struct race_game *game);
static int race_places_create(struct race_game *game);
static int race_places_free(struct race_game *game);
static int race_board_fill(struct race_game *game);
static int race_starts_fill(struct race_game *game);
static int race_gates_fill(struct race_game *game);
static int race_place_chao(struct race_game *game, int where);
static void race_run(struct race_game *game);
static void race_return_to_hall(struct race_game *game);
static void race_go_to_course(struct race_game *game);
static void race_rank(struct race_game *game);
static void race_average(struct race_game *game);
static void race_finish_racers(struct race_game *game);
static void race_finish_late_racers(struct race_game *game);
static void race_camera_input(struct race_game *game);
static void race_follow_next(struct race_game *game);
static void race_follow_previous(struct race_game *game);
static void race_draw_marker(struct race_game *game);
static int race_suspend_player(struct race_game *game);
static int race_resume_player(struct race_game *game);
static void race_cursor_cycle(Sint8 *choice, int last);
static void race_window_step(struct race_game *game);
static void cups_window_step(struct race_game *game);
static void garden_window_step(struct race_game *game);
static void chao_window_step(struct race_game *game);
static void chao_window_slide(struct race_game *game, int kind);
static void chao_demo_step(struct race_game *game);
static void race_cursor_step(struct race_game *game, Sint8 *choice, int last);
static void race_board_cursor(struct race_game *game);
static int race_board_right(struct race_places *places, int skip);
static int race_board_left(struct race_places *places, int skip);
static void race_cheer_init(struct race_cheer *cheer);
static int race_cheer_started(struct race_cheer *cheer);
static void race_cheer_step(struct race_cheer *cheer, struct race_game *game);
static void race_record_time(struct race_game *game, int lane);
static void race_record_win(struct race_game *game);
static void race_shuffle_entries(struct race_game *game);
static int race_in_commendation(struct race_game *game);

struct race_game *race_game_current = NULL;

/* the debug page's label for each enum race_state, then eight more. those
 * eight, from NO on, follow the enum's last state */
static const char race_state_labels[38][20] = {
    "ENTRANCE_INIT",
    "MEDAL_GET",
    "VMS_ERROR_CHECK",
    "ENTRANCE_RE_INIT",
    "ENTRANCE_INIT2",
    "ENTRANCE",
    "HINT_BOX",
    "HINT_BOX_WAIT",
    "RACE_WINDOW_INI",
    "RACE_WINDOW",
    "CUPS_WINDOW_INI",
    "CUPS_WINDOW",
    "GARDEN_WINDOW_INI",
    "GARDEN_WINDOW",
    "CHAO_WINDOW_INI",
    "CHAO_WINDOW",
    "CHAO_WINDOW_SLIDE",
    "CHAO_DEMO_0",
    "CHAO_DEMO_1",
    "CHAO_DEMO_2",
    "CHAO_DEMO_3",
    "INIT",
    "RE_INIT",
    "WAIT",
    "RACE",
    "RACEEND",
    "COMMENDATION_INI",
    "COMMENDATION_0",
    "COMMENDATION_1",
    "COMMENDATION_2",
    "NO",
    "CAMERA",
    "CAM EDITOR",
    "MOVE",
    "STATUS",
    "NODSP",
    "MESO",
    "MAX",
};

struct race_game *race_game_create(void)
{
    struct race_game *game = heap_calloc(1, sizeof(struct race_game));

    /* @bug memory: the game just allocated leaks while another is current.
     * BF/S at 0x0cb8c0d2 leaves for the NULL return at 0x0cb8c0f2 with it
     * unfreed */
    /* @hack redundant: an else after the arm that returns, which puts the store
     * of places' null through R3 (0x0cb8c0de); without it, R2 takes it, and
     * 2 of 29 words differ */
    if (race_game_current != NULL) {
        return NULL;
    } else {
        race_game_current = game;
        if (game != NULL) {
            game->places = NULL;
            if (race_game_init(game)) {
                return game;
            }
            /* @hack implicit: where race_game_init fails, R0 still holds
             * its 0 (0x0cb8c0e6), which alife_race_start stores
             * (0x0cb998ba); returned explicitly, 1 of 29 words differ */
#ifdef GCC
            /* keeps the value the tag names: R0's 0 */
            return NULL;
#else
            return; /* NOLINT(clang-diagnostic-return-mismatch) */
#endif
        }
    }
    return NULL;
}

Bool race_game_step(struct race_game *game)
{
    Uint16 act = stage_get_stage_act();

    act &= 0xff;
    if (act == ACT_HALL) {
        race_hall_step(game);
    } else {
        race_course_step(game);
    }
    if (game->steps != 0xffff) {
        game->steps++;
    } else {
        game->steps = 0;
    }
    if (game->flags & GAME_WATCH_CARDS) {
        race_cards_drop_absent(&game->cards);
    }
    if (game->flags & GAME_CHANGE_ACT) {
        if (act == ACT_HALL) {
            race_places_free(game);
        }
        game->flags &= ~GAME_CHANGE_ACT;
        return TRUE;
    }
    return FALSE;
}

/* run the hall's state a step: the entrance, the windows or the gates.
 * player 0 walks the entrance, and the windows choose the race and enter
 * its Chao. then step the places' entrants and the camera, and poll the
 * drives while GAME_POLL_CARDS is set */
static void race_hall_step(struct race_game *game)
{
    struct race_places *places = game->places;
    int i;
    int character;
    int stage;

    if (game == NULL) {
        return;
    }
    game->timer++;
    switch (game->state) {
    case STATE_ENTRANCE_INIT:
        race_set_camera_mode(game, RACE_CAMERA_HALL);
        scenery_create();
        race_clear_entries(game);
        pad_disable_player(0);
        game->flags &= ~GAME_WON;
        game->card_drives = 0;
        if (game->flags & GAME_FIRST_WIN) {
            game->flags &= ~GAME_FIRST_WIN;
            character = player_get_character(0);
            stage = stage_get_stage_act() >> 8;
            /* the race's kind is the hall's mission */
            if (emblem_check_mission(&saves[save_slot], character, stage,
                                     game->kind) == 1) {
                game->state = STATE_VMS_ERROR_CHECK;
                game->timer = 0;
            } else {
                emblem_set_mission(&saves[save_slot], character, stage,
                                   game->kind);
                game->emblem_display = emblem_start_count_display();
                game->state = STATE_MEDAL_GET;
                game->timer = 0;
            }
        } else {
            game->state = STATE_VMS_ERROR_CHECK;
            game->timer = 0;
        }
        break;
    case STATE_MEDAL_GET:
        if (game->timer > 120) {
            game->emblem_display = NULL;
            game->state = STATE_VMS_ERROR_CHECK;
            game->timer = 0;
        }
        break;
    case STATE_VMS_ERROR_CHECK:
        if (game->flags & GAME_NOT_SAVED) {
            menu_draw_message();
            if (per_info[0]->press & PAD_MASH_BUTTONS) {
                game->flags &= ~GAME_NOT_SAVED;
                game->state = STATE_ENTRANCE_INIT2;
                game->timer = 0;
            }
        } else {
            game->state = STATE_ENTRANCE_INIT2;
            game->timer = 0;
        }
        break;
    case STATE_ENTRANCE_RE_INIT:
        i = 0;
        do {
            places->places[i].state = 0;
        } while (++i < RACE_LANES);
        game->state = STATE_ENTRANCE;
        break;
    case STATE_ENTRANCE_INIT2:
        if (game->flags & GAME_RACED) {
            music_start_later(MUSIC_CHAOHALL, 10);
        }
        pad_enable_player(0);
        game->state = STATE_ENTRANCE;
        game->timer = 0;
        /* fall through */
    case STATE_ENTRANCE:
        if (game->flags & GAME_TO_COURSE) {
            game->flags &= ~GAME_TO_COURSE;
            race_go_to_course(game);
            return;
        }
        if (message_busy() == 1) {
            game->state = STATE_HINT_BOX;
            game->timer = 0;
        }
        break;
    case STATE_HINT_BOX:
        if (message_busy() != 1) {
            game->state = STATE_HINT_BOX_WAIT;
            game->timer = 0;
            pad_disable_player(0);
        }
        break;
    case STATE_HINT_BOX_WAIT:
        if (game->timer > 3) {
            game->state = STATE_ENTRANCE;
            game->timer = 0;
            pad_enable_player(0);
            race_set_camera_mode(game, RACE_CAMERA_HALL);
        }
        break;
    case STATE_RACE_WINDOW_INI:
        race_set_camera_mode(game, RACE_CAMERA_HALL_FIXED);
        race_suspend_player(game);
        game->state = STATE_RACE_WINDOW;
        /* fall through */
    case STATE_RACE_WINDOW:
        race_window_step(game);
        break;
    case STATE_CUPS_WINDOW_INI:
        game->state = STATE_CUPS_WINDOW;
        game->timer = 0;
        /* fall through */
    case STATE_CUPS_WINDOW:
        cups_window_step(game);
        break;
    case STATE_GARDEN_WINDOW_INI:
        game->state = STATE_GARDEN_WINDOW;
        game->timer = 0;
        /* fall through */
    case STATE_GARDEN_WINDOW:
        garden_window_step(game);
        break;
    case STATE_CHAO_WINDOW_INI:
        i = 0;
        do {
            places->places[i].state = 2;
        } while (++i < RACE_LANES);
        game->state = STATE_CHAO_WINDOW;
        game->timer = 0;
        /* fall through */
    case STATE_CHAO_WINDOW:
        chao_window_step(game);
        break;
    case STATE_CHAO_WINDOW_SLIDE:
        race_place_chao(game, PLACE_BOARD);
        game->state = STATE_CHAO_WINDOW_INI;
        game->timer = 0;
        /* @hack spelling: the places reached again through game, which loads
         * them for each store (0x0cb8c592, 0x0cb8c59a); through places, 97
         * of 430 words and 47 pool words differ, and the code after moves */
        game->places->step = 0;
        game->places->timer = 0;
        break;
    case STATE_CHAO_DEMO_0:
    case STATE_CHAO_DEMO_1:
    case STATE_CHAO_DEMO_2:
    case STATE_CHAO_DEMO_3:
        chao_demo_step(game);
    }
    /* @hack spelling: i cleared before the places' timer counts, which keeps i
     * in R13, the zero the switch's stores use, and entrant_step's address
     * in R10 (0x0cb8c5e8); counted first, 53 of 430 words and 1 pool word
     * differ */
    i = 0;
    places->timer++;
    for (; i < RACE_LANES; i++) {
        entrant_step(&places->places[i]);
    }
    race_camera_step(&game->camera, game);
    if (game->flags & GAME_POLL_CARDS) {
        card_drives_take_changes();
    }
}

/* run the course a step. outside the commendation, first collect the
 * collision meshes. race_run then runs the state, and GAME_TO_HALL takes
 * the game back to the hall. then take the camera's input, step each racer
 * and the camera, and draw the clock, the standings and the marker */
static void race_course_step(struct race_game *game)
{
    int i;

    if (game == NULL) {
        return;
    }
    if (race_in_commendation(game) != 1) {
        collision_mesh_collect_all();
    }
    race_run(game);
    if (game->flags & GAME_TO_HALL) {
        game->flags &= ~GAME_TO_HALL;
        race_return_to_hall(game);
    }
    race_camera_input(game);
    for (i = 0; i < RACE_LANES; i++) {
        racer_step(&game->racers[i], &game->path, game->kind);
    }
    race_camera_step(&game->camera, game);
    race_course_draw(game);
}

static void race_course_draw(struct race_game *game)
{
    race_clock_draw(&game->clock);
    standings_draw(&game->standings, game->racers, game->lane);
    race_draw_marker(game);
}

void race_game_free(struct race_game *game)
{
    card_drives_end();
    if (game != NULL) {
        race_game_current = NULL;
        race_path_free(&game->path);
        race_places_free(game);
        race_resume_player(game);
        menu_slots_free();
        menu_panel_close();
        heap_free(game);
    }
}

int race_get_racer_pos(NJS_POINT3 *pos, int lane)
{
    struct race_game *game = race_game_current;
    struct task_work *work;
    struct racer *racer;

    if (game != NULL && lane < RACE_LANES) {
        racer = &game->racers[lane];
        work = racer_work(racer);
        if (work != NULL) {
            pos->x = work->pos.x;
            pos->y = work->pos.y;
            pos->z = work->pos.z;
            return 1;
        }
    }
    return 0;
}

int race_lane_of_task(struct task *chao_task)
{
    int i;
    struct alife_chao *chao;
    struct task *owner;

    if (race_game_current == NULL || chao_task == NULL) {
        return -1;
    }
    for (i = 0; i < RACE_LANES; i++) {
        chao = racer_chao(&race_game_current->racers[i]);
        if (chao != NULL) {
            owner = chao->task;
            if (owner == chao_task) {
                return i;
            }
        }
    }
    return -1;
}

void race_print_state(struct race_game *game, int x, int y)
{
    njPrintC(NJM_LOCATION(x, y), race_state_labels[game->state]);
}

void fun_0cb8c942(struct race_game *game)
{
    race_print_state(game, 7, 11);
    njPrintH(NJM_LOCATION(7, 5), 180, 5);
}

void fun_0cb8c956(struct race_game *game)
{
    /* @hack redundant: register on kept, which only the dead code below
     * uses: it takes FR15, which the image saves and restores (0x0cb8c95e,
     * 0x0cb8c996); without it, kept takes a frame word, and 10 of 35 words
     * differ */
    register Float kept;

    /* @hack redundant: never-run uses of kept, which keep it; without them
     * and kept, 33 of 35 words differ */
    if (0) {
        kept = (Float)game->timer;
    }
    njPrintD(NJM_LOCATION(7, 14), game->state, 2);
    njPrintD(NJM_LOCATION(7, 15), game->timer, 5);
    racer_print_motion(&game->racers[game->lane], 7, 17);
    /* @hack redundant: the second never-run use of kept, which the tag above
     * counts with the first: it keeps kept in FR15 (0x0cb8c95e), as the image
     * has it; without it, 33 of 35 words differ */
    if (0) {
        game->timer = (int)kept;
    }
}

void fun_0cb8c9e0(struct race_game *game)
{
    struct racer *racer = &game->racers[game->lane];

    if (per_info[0]->on & PDD_DGT_TY) {
        racer->flags |= RACER_CHEERED;
    }
    if (per_info[0]->press & PDD_DGT_TL) {
        racer->chao->record->skills.run = 9999;
        racer->chao->record->skills.swim = 9500;
        racer->chao->record->skills.fly = 9500;
        racer->chao->record->skills.power = 9500;
        racer->chao->record->race_wins = 0;
    }
    if (per_info[0]->press & PDD_DGT_TR) {
        game->flags |= GAME_WON;
    }
    njPrintC(NJM_LOCATION(3, 4), "FLAG");
    race_print_bits16(&game->flags, 8, 4);
    /* @hack spelling: two statements as one comma expression, which keeps the
     * call of race_print_bits8 a JSR before the epilogue (0x0cb8ca8c); as
     * two, SHC makes it a tail call, and 6 of 91 words differ */
    njPrintC(NJM_LOCATION(3, 6), "VMS"),
        race_print_bits8(&game->cards.remaining, 8, 6);
}

/* the first step of a racing Chao's task, and of an entrant's Chao's: set
 * the Chao up, then give the task its step and destructor */
static void race_chao_start(struct task *task)
{
    race_chao_init(task);
    task->step = race_chao_step;
    task->destructor = race_chao_destroy;
}

static void entrant_chao_start(struct task *task)
{
    entrant_chao_init(task);
    task->step = entrant_chao_step;
    task->destructor = entrant_chao_destroy;
}

static int race_game_init(struct race_game *game)
{
    int i;

    script_set_table(race_script_table);
    race_places_create(game);
    game->kind = 0;
    game->lane = 0;
    game->average_distance = 0.0F;
    game->card_drives = 0;
    game->state = STATE_ENTRANCE_INIT;
    game->guide = race_guide_racers;
    game->steps = 0;
    game->character = player_get_character(0);
    race_camera_reset(&game->camera);
    race_path_init(&game->path, race_paths, 2,
                   race_course.distances[game->kind]);
    game->records = (struct race_records *)chao_get_file_data();
    race_records_reset_unmarked(game->records);
    for (i = 0; i < RACE_LANES; i++) {
        race_rival_init(&game->chao[i], NULL, RIVAL_RANDOM, i);
    }
    race_clock_clear(&game->clock);
    standings_init(&game->standings);
    menu_slots_create();
    card_drives_start();
    card_drives_refresh();
    race_fade_create();
    return 1;
}

static void race_clear_entries(struct race_game *game)
{
    int i;

    for (i = 0; i < RACE_LANES; i++) {
        game->entries[i].kind = ENTRY_RIVAL;
        game->entries[i].index = 8;
    }
    game->entry_count = 0;
}

static int race_places_create(struct race_game *game)
{
    struct race_places *places;

    game->places = heap_calloc(1, sizeof(struct race_places));
    if (game->places != NULL) {
        places = game->places;
        places->kind = 0;
        places->cursor = 0;
        return 1;
    }
    return 0;
}

static int race_places_free(struct race_game *game)
{
    if (game->places != NULL) {
        heap_free(game->places);
        game->places = NULL;
    }
    return 1;
}

/* the chao window's board: two rows of four places, the even ones in the
 * upper row. race_board_fill takes them through the frame of
 * RACE_CAMERA_HALL_FIXED's view, which the HALL_VIEW macros give */
static const NJS_POINT3 board_places[RACE_LANES] = {
#include "al_race/race_game/board_places.inc"
};

/* fill the board with the Chao of the places' kind, and return 1. a
 * garden's Chao take the places in its list's order until the list ends,
 * and a card's Chao takes its drive's place. each Chao gets an entrant's
 * task, but an egg's place shows the egg and takes no task. 0 with no
 * places. where a task can't be made, it returns 0 too, after the @bug
 * null below */
static int race_board_fill(struct race_game *game)
{
    struct race_places *places = game->places;
    struct chao_node *node;
    struct task *task;
    struct task_work *work;
    int place;
    NJS_MATRIX matrix;
    NJS_POINT3 pos;

    if (places == NULL) {
        return 0;
    }
#ifdef GCC
    /* GCC can't see that node is set wherever it is read: the loop reads it
     * only where kind isn't ENTRY_CARD, and the garden window's choice leaves
     * any other kind a garden's, whose case sets it */
    node = NULL;
#endif
    switch (places->kind) {
    case GARDEN_STATION_SQUARE:
    case GARDEN_EGG_CARRIER:
    case GARDEN_MYSTIC_RUINS:
        node = chao_get_list(places->kind);
        break;
    /* @hack redundant: a case for the cards that only breaks, whose CMP/EQ #3
     * the switch makes (0x0cb8cdb8); without it, 34 of 312 words differ */
    case ENTRY_CARD:
        break;
    }
    for (place = 0; place < RACE_LANES; place++) {
        njUnitMatrix(&matrix);
        njTranslate(&matrix, HALL_VIEW_X, HALL_VIEW_Y, HALL_VIEW_Z);
        njRotateXYZ(&matrix, HALL_VIEW_ANG_X, HALL_VIEW_ANG_Y, 0);
        njCalcPoint(&matrix, (NJS_POINT3 *)&board_places[place], &pos);
        if (places->kind != ENTRY_CARD) {
            if (node == NULL) {
                break;
            }
            if ((node->chao->type & 0x7f) != CHAO_EGG) {
                task = race_create_chao(entrant_chao_start, node->chao);
                node = node->next;
            } else {
                if (node->chao->flags & CHAO_GOLD) {
                    places->places[place].flags |= ENTRANT_GOLD;
                } else if (node->chao->flags & CHAO_PAL) {
                    places->places[place].flags |= ENTRANT_PAL;
                } else if (node->chao->flags & CHAO_BEGG) {
                    places->places[place].flags |= ENTRANT_BEGG;
                } else {
                    places->places[place].flags |= ENTRANT_EGG;
                }
                entrant_init(&places->places[place], &pos);
                node = node->next;
                continue;
            }
        } else {
            int type;

            if (!race_card_has_chao(&game->cards, place)) {
                continue;
            }
            type = game->chao[place].type & 0x7f;
            if (type == CHAO_NOTHING) {
                continue;
            }
            if (type == CHAO_EGG) {
                /* @bug memory: an egg on a card takes its look through node,
                 * which no path here sets: R13, as the caller left it, is read
                 * for the record at 0x0cb8cf88, and the record's flags at
                 * 0x0cb8cf8c. as the board slides to the cards, race_hall_step
                 * leaves R13 0 (MOV #0,R13 at 0x0cb8c200), so the first read is
                 * of address 4. as the cards are chosen, garden_window_step
                 * leaves it game->places (0x0cb8ed12), so the record is the
                 * word at place 0's flags */
#ifdef GCC
                /* repairs the read the tag names, whose record varies with
                 * timing. the egg takes its look from its card's Chao,
                 * game->chao[place], whose type the test above reads */
                if (game->chao[place].flags & CHAO_GOLD) {
                    places->places[place].flags |= ENTRANT_GOLD;
                } else if (game->chao[place].flags & CHAO_PAL) {
                    places->places[place].flags |= ENTRANT_PAL;
                } else if (game->chao[place].flags & CHAO_BEGG) {
                    places->places[place].flags |= ENTRANT_BEGG;
                } else {
                    places->places[place].flags |= ENTRANT_EGG;
                }
#else
                if (node->chao->flags & CHAO_GOLD) {
                    places->places[place].flags |= ENTRANT_GOLD;
                } else if (node->chao->flags & CHAO_PAL) {
                    places->places[place].flags |= ENTRANT_PAL;
                } else if (node->chao->flags & CHAO_BEGG) {
                    places->places[place].flags |= ENTRANT_BEGG;
                } else {
                    places->places[place].flags |= ENTRANT_EGG;
                }
#endif
                entrant_init(&places->places[place], &pos);
                continue;
            }
            task = race_create_chao(entrant_chao_start, &game->chao[place]);
        }
        /* @bug memory: with no task made, the null task is read for
         * its work at 0x0cb8d078, and the work written through, before the
         * test at 0x0cb8d0b0 */
        work = task->work;
        /* @hack spelling: pos through its address, which forms each member's
         * stack address apart (0x0cb8d06c, 0x0cb8d07a, 0x0cb8d086); as pos.x,
         * pos.y and pos.z, 64 of 312 words differ */
        work->pos.x = (&pos)->x;
        work->pos.y = (&pos)->y;
        work->pos.z = (&pos)->z;
        njCalcPoint(&matrix, (NJS_POINT3 *)&board_places[place], &work->pos);
        ENTRANT_VARS(work)->entrant =
            entrant_init(&places->places[place], &work->pos);
        if (task == NULL) {
            return 0;
        }
    }
    return 1;
}

/* make each lane's racer at the lane's start on the course, and return 1.
 * the lane's entry gives a garden's Chao or a card's. a rival takes the
 * next record of a drive no card entry has, in drive order. the racer of a
 * rival whose record has CHAO_BEGG takes RACER_LATE_START. 0 when a task can't
 * be made */
static int race_starts_fill(struct race_game *game)
{
    int begg_lane;
    int next;
    int lane;
    NJS_POINT3 *starts;

    next = 0;
    begg_lane = -1;
    starts = race_course.starts[game->kind];
    lane = 0;
    do {
        struct race_entry *entry = &game->entries[lane];
        struct chao_node *node;
        struct task *task;
        struct task_work *work;
        int i;

        if (entry->kind < GARDEN_COUNT) {
            node = chao_get_list(entry->kind);
            for (i = 0; i < entry->index; i++) {
                node = node->next;
            }
            task = race_create_chao(race_chao_start, node->chao);
        } else if (entry->kind == ENTRY_CARD) {
            struct chao *chao = &game->chao[entry->index];

            /* @hack spelling: the type cleared through chao, which keeps the
             * zero in R10 and lane in R11 (0x0cb8d11a, 0x0cb8d122); as one
             * statement, the two swap, and 32 of 183 words and 1 pool word
             * differ */
            chao->type &= 0x7f;
            task = race_create_chao(race_chao_start, &game->chao[entry->index]);
        } else {
            while (game->card_drives & (1 << next)) {
                next++;
                if (next >= RACE_LANES) {
                    next = 0;
                }
            }
            task = race_create_chao(race_chao_start, &game->chao[next]);
            if (game->chao[next].flags & CHAO_BEGG) {
                begg_lane = lane;
            }
            next++;
            if (next >= RACE_LANES) {
                next = 0;
            }
        }
        if (task == NULL) {
            return 0;
        }
        work = task->work;
        work->pos = starts[lane];
        RACER_VARS(work)->racer = racer_init(&game->racers[lane], lane);
        if (begg_lane >= 0) {
            game->racers[begg_lane].flags |= RACER_LATE_START;
        }
    } while (++lane < RACE_LANES);
    return 1;
}

/* shuffle the entries, then make each lane's entrant at its gate, and
 * return 1. 0 when a task can't be made. the lane's entry gives a garden's
 * Chao or a card's. a rival takes the next record of a drive no card entry
 * has, in drive order, and the entry's index takes the drive.
 * race_rival_init sets the record up from a source Chao, in one of its
 * modes.
 *
 * outside RACE_RECORDED, the source is the entered Chao whose skills have
 * the greatest sum, the last of equals. RACE_RECORDED has none. with a
 * source, each rival is asked for in RIVAL_VARIED. without one, it is asked
 * for in RIVAL_RANDOM, or in RIVAL_RANDOM_2 while the records hold a win of
 * RACE_KIND_LAST. while they do, the first rival is asked for in
 * RIVAL_BLACK_EGG instead, which race_rival_init makes RIVAL_RANDOM without
 * a source. in RACE_MATCHED with that win, a source whose skills sum to
 * 36000 or more overrides the rest. every rival is then asked for in
 * RIVAL_GOLD_EGG where each of the source's skills is 9999 or more, and else
 * in RIVAL_SILVER_EGG */
static int race_gates_fill(struct race_game *game)
{
    int next = 0;
    struct race_places *places = game->places;
    struct chao *best = NULL;
    int lane;
    int serial;

    race_shuffle_entries(game);
    if (game->mode != RACE_RECORDED) {
        int most = -1;

        for (lane = 0; lane < RACE_LANES; lane++) {
            struct race_entry *entry = &game->entries[lane];
            struct chao *chao;
            int sum;

            if (entry->kind < GARDEN_COUNT) {
                struct chao_node *node = chao_get_list(entry->kind);
                int i;

                for (i = 0; i < entry->index; i++) {
                    node = node->next;
                }
                chao = node->chao;
                sum = chao->skills.run + chao->skills.swim +
                      chao->skills.power + chao->skills.fly;
                if (sum >= most) {
                    best = node->chao;
                    most = sum;
                }
                continue;
            }
            if (entry->kind == ENTRY_CARD) {
                chao = &game->chao[entry->index];
                chao->type &= 0x7f;
                /* @hack redundant: chao taken again, unchanged, which reads the
                 * index again after the type's store (0x0cb8d3b8, 0x0cb8d3ba);
                 * without it, 191 of 422 words differ */
                chao = &game->chao[entry->index];
                sum = chao->skills.run + chao->skills.swim +
                      chao->skills.power + chao->skills.fly;
                if (sum >= most) {
                    best = &game->chao[entry->index];
                    most = sum;
                }
            }
        }
    }
    if (game->records->flags & RACE_KIND_WON(RACE_KIND_LAST)) {
        game->flags |= GAME_RIVAL_3;
    }
    for (lane = 0; lane < RACE_LANES; lane++) {
        struct race_entry *entry = &game->entries[lane];

        if (entry->kind == ENTRY_CARD) {
            game->card_drives |= 1 << entry->index;
        }
    }
    serial = (int)RANDOM(0.0F, 8.0F);
    for (lane = 0; lane < RACE_LANES; lane++) {
        struct race_entry *entry = &game->entries[lane];
        struct task *task;
        struct task_work *work;

        if (entry->kind < GARDEN_COUNT) {
            struct chao_node *node = chao_get_list(entry->kind);
            int i;

            for (i = 0; i < entry->index; i++) {
                node = node->next;
            }
            task = race_create_chao(entrant_chao_start, node->chao);
        } else if (entry->kind == ENTRY_CARD) {
            task =
                race_create_chao(entrant_chao_start, &game->chao[entry->index]);
        } else {
            int rival_mode;

            while (game->card_drives & (1 << next)) {
                next++;
                if (next >= RACE_LANES) {
                    next = 0;
                }
            }
            entry->index = next;
            serial++;
            if ((game->records->flags & RACE_KIND_WON(RACE_KIND_LAST)) &&
                game->mode == RACE_MATCHED && best != NULL &&
                best->skills.swim + best->skills.run + best->skills.fly +
                        best->skills.power >=
                    36000) {
                if (best->skills.swim >= 9999 && best->skills.run >= 9999 &&
                    best->skills.fly >= 9999 && best->skills.power >= 9999) {
                    rival_mode = RIVAL_GOLD_EGG;
                } else {
                    rival_mode = RIVAL_SILVER_EGG;
                }
            } else if (!(game->flags & GAME_RIVAL_3)) {
                if (best == NULL) {
                    if (game->records->flags & RACE_KIND_WON(RACE_KIND_LAST)) {
                        rival_mode = RIVAL_RANDOM_2;
                    } else {
                        rival_mode = RIVAL_RANDOM;
                    }
                } else {
                    rival_mode = RIVAL_VARIED;
                }
            } else {
                rival_mode = RIVAL_BLACK_EGG;
                game->flags &= ~GAME_RIVAL_3;
            }
            race_rival_init(&game->chao[next], best, rival_mode, serial);
            task = race_create_chao(entrant_chao_start, &game->chao[next]);
            next++;
            if (next >= RACE_LANES) {
                next = 0;
            }
        }
        if (task == NULL) {
            return 0;
        }
        work = task->work;
        gate_get_spot(&work->pos, lane);
        ENTRANT_VARS(work)->entrant =
            entrant_init(&places->places[lane], &work->pos);
    }
    return 1;
}

/* make the Chao where says, an enum race_place, and return 1. 0 when its
 * fill fails */
static int race_place_chao(struct race_game *game, int where)
{
    switch (where) {
    case PLACE_BOARD:
        if (!race_board_fill(game)) {
            return 0;
        }
        break;
    case PLACE_GATES:
        if (!race_gates_fill(game)) {
            return 0;
        }
        break;
    case PLACE_STARTS:
        if (!race_starts_fill(game)) {
            return 0;
        }
        break;
    }
    return 1;
}

/* run the course's state a step. STATE_WAIT follows each lane in turn, and
 * STATE_RACE runs the race. STATE_RACEEND lets the racers still running
 * finish, then fades out. the commendation saves each changed card entry's
 * Chao to its card. through the commendation's states, every racer faces
 * race_course's target 16 */
static void race_run(struct race_game *game)
{
    int i;

    game->timer++;
    if (race_in_commendation(game) == 1) {
        for (i = 0; i < RACE_LANES; i++) {
            racer_face_target(&game->racers[i], &race_course.targets[16]);
        }
    }
    switch (game->state) {
    case STATE_INIT:
        race_set_camera_mode(game, RACE_CAMERA_AHEAD);
        game->lane = 0;
        scenery_create();
        confetti_task_create();
        cracker_stop();
        race_suspend_player(game);
        i = 0;
        do {
            game->racers[i].state = RACER_TO_WAIT;
        } while (++i < RACE_LANES);
        game->state = STATE_WAIT;
        game->timer = 0;
        break;
    case STATE_RE_INIT:
        game->state = STATE_WAIT;
        game->timer = 0;
        break;
    case STATE_WAIT:
        if (game->timer % LANE_SHOW_STEPS == LANE_SHOW_STEPS - 1) {
            game->lane++;
            game->camera_timer = 0;
        }
        if (game->lane >= RACE_LANES) {
            game->lane = 0;
            race_set_camera_mode(game, RACE_CAMERA_COURSE_FIXED);
            i = 0;
            do {
                game->racers[i].state = RACER_TO_RACE;
            } while (++i < RACE_LANES);
            menu_show_start();
            sound_play(SE_START_1, 0, 0, 0);
            sound_play(SE_START_2, 0, 0, 0);
            menu_slot_look(0, LOOK_POP);
            game->flags |= GAME_RACED;
            game->state = STATE_RACE;
            game->timer = 0;
        }
        /* @hack redundant: never-run calls, which weigh menu_slot_look's
         * address above sound_play's: R9 against R8 (0x0cb8d7e8, 0x0cb8d7ec),
         * and the pool's order at 0x0cb8d858; without them, 166 of 590 words
         * and 36 pool words differ */
        if (0) {
            menu_slot_look(0, 0);
            menu_slot_look(0, 0);
            menu_slot_look(0, 0);
        }
        break;
    case STATE_RACE:
        race_clock_step(&game->clock);
        race_cheer_step(&game->cheer, game);
        /* @hack spelling: i stepped as i = i + 1, which gives R9 the
         * commendation's 28 (0x0cb8dc4a); as i++, R9 holds RACER_CHANGED
         * there, and 71 of 590 words and 9 pool words differ */
        for (i = 0; i < RACE_LANES; i = i + 1) {
            struct racer *racer = &game->racers[i];
            Float lead = racer->distance - game->average_distance;

            if (!racer_is_active(racer)) {
                continue;
            }
            if (lead > LEAD && race_cheer_started(&game->cheer) != 1) {
                if (racer->state != RACER_WALKING &&
                    racer->state != RACER_RESTING) {
                    racer->state = RACER_TO_WALK;
                }
            } else {
                if (racer->state == RACER_WALKING ||
                    (racer->state == RACER_RESTING && racer_place(racer))) {
                    racer->state = RACER_TO_RACE;
                }
            }
        }
        race_finish_racers(game);
        game->guide(game);
        if (game->timer > RACE_TIMEOUT) {
            game->state = STATE_RACEEND;
            game->timer = 0;
        }
        /* @hack redundant: a never-run store of 2, which keeps the 2
         * STATE_INIT gives the racers in R10 from the switch's head
         * (0x0cb8d7f4) through this case; without it, 152 of 590 words and
         * 10 pool words differ */
        if (0) {
            game->timer = 2;
        }
        race_average(game);
        race_rank(game);
        break;
    case STATE_RACEEND:
        race_finish_late_racers(game);
        if (game->timer == 100) {
            race_fade_out(35);
        }
        if (game->timer == 110) {
            music_stop();
        }
        if (game->timer > 2.8F * 60.0F) {
            game->state = STATE_COMMENDATION_INI;
            game->timer = 0;
            i = 0;
            do {
                racer_cancel_item(&game->racers[i]);
                game->racers[i].state = RACER_TO_PODIUM;
            } while (++i < RACE_LANES);
        }
        break;
    case STATE_COMMENDATION_INI:
        music_play(MUSIC_CHAOGOAL);
        cracker_start();
        if (game->flags & GAME_WON) {
            NJS_POINT3 pos;

            /* @hack spelling: pos through its address, which forms each
             * member's stack address apart (0x0cb8db06, 0x0cb8db12,
             * 0x0cb8db28); as pos.x, pos.y and pos.z, 39 of 590 words
             * differ */
            (&pos)->x = race_course.targets[8].x;
            (&pos)->y = race_course.targets[8].y;
            (&pos)->z = race_course.targets[8].z;
            sound_play(SE_WON, 0, 0, 0);
            pos.y += 10.0F;
            menu_spin_create(game->kind, &pos);
        } else if (game->mode == RACE_RECORDED &&
                   !(game->records->flags & game->kind)) {
            /* @bug logic: the records' flags are tested with the kind
             * itself, TST R2,R3 at 0x0cb8dbb2, where race_finish_racers sets
             * RACE_KIND_WON(kind). kind 0's test always passes, and the other
             * kinds test the bits of other kinds' wins */
            sound_play(SE_NOT_WON, 0, 0, 0);
        }
        i = game->standings.lanes[0];
        if (game->entries[i].kind != ENTRY_RIVAL) {
            if (game->flags & GAME_FIRST_WIN) {
                menu_show_get();
            } else {
                menu_show_cool();
            }
            menu_slot_look(0, LOOK_POP + 2);
        }
        race_fade_in(47);
        race_record_win(game);
        game->state = STATE_COMMENDATION_0;
        game->result_lane = 0;
        game->timer = 0;
        /* fall through */
    case STATE_COMMENDATION_0:
        for (;;) {
            if (game->racers[game->result_lane].flags & RACER_CHANGED) {
                struct race_entry *entry;
                struct alife_chao *chao;

                game->racers[game->result_lane].flags &= ~RACER_CHANGED;
                entry = &game->entries[game->result_lane];
                chao = racer_chao(&game->racers[game->result_lane]);
                if (entry->kind == ENTRY_CARD && chao != NULL) {
                    /* @hack redundant: a never-run test of chao, which
                     * weighs it into R11 (0x0cb8dca2) ahead of the 29 held in
                     * R10; without it, 8 of 590 words differ */
                    if (0) {
                        if (chao) {
                        }
                    }
                    if (race_card_remains(&game->cards, entry->index) == 1) {
                        race_card_save(&game->cards, entry->index,
                                       chao->record);
                        game->state = STATE_COMMENDATION_1;
                        return;
                    } else {
                        game->flags |= GAME_NOT_SAVED;
                    }
                } else {
                    game->flags |= GAME_NOT_SAVED;
                }
            }
            game->result_lane++;
            if (game->result_lane >= RACE_LANES) {
                game->state = STATE_COMMENDATION_2;
                break;
            }
        }
        /* @hack redundant: never-run stores of 8 and 29, which weigh both in
         * the function's register picks: this case holds 0xff7f, 29 and 28
         * in R8, R10 and R9 (0x0cb8dc46 to 0x0cb8dc4a); without the 29, 15
         * of 590 words differ, and without the 8, 74 and 10 pool words */
        if (0) {
            game->timer = 8;
            game->timer = 29;
        }
        break;
    case STATE_COMMENDATION_1:
        if (!race_cards_busy(&game->cards)) {
            game->state = STATE_COMMENDATION_0;
        }
        break;
    case STATE_COMMENDATION_2:
        if (edit_mode) {
            game->timer = 0;
        }
        if (game->timer > 630.0F) {
            menu_slots_clear();
            game->flags |= GAME_TO_HALL;
        }
        break;
    }
}

static void race_return_to_hall(struct race_game *game)
{
    int i;

    game->timer = 0;
    standings_init(&game->standings);
    race_path_set_target(&game->path, race_course.distances[game->kind]);
    race_places_create(game);
    scenery_destroy();
    for (i = 0; i < RACE_LANES; i++) {
        racer_destroy_chao(&game->racers[i]);
    }
    /* @hack redundant: an empty loop over the lanes, which the image runs
     * (0x0cb8de4a to 0x0cb8de50); without it, 28 of 75 words differ */
    for (i = 0; i < RACE_LANES; i++) {
    }
    race_resume_player(game);
    game->state = STATE_ENTRANCE_INIT;
    game->timer = 0;
    game->flags |= GAME_CHANGE_ACT;
}

static void race_go_to_course(struct race_game *game)
{
    struct race_places *places = game->places;
    int i;

    game->timer = 0;
    standings_init(&game->standings);
    race_clock_clear(&game->clock);
    race_clock_set_best(&game->clock, game->records, game->kind);
    race_set_camera_mode(game, RACE_CAMERA_WATCH);
    race_path_set_target(&game->path, race_course.distances[game->kind]);
    race_cheer_init(&game->cheer);
    if (places != NULL) {
        for (i = 0; i < RACE_LANES; i++) {
            entrant_remove(&places->places[i]);
        }
    }
    race_place_chao(game, PLACE_STARTS);
    i = 0;
    do {
        game->racers[i].state = RACER_TO_RESTART;
    } while (++i < RACE_LANES);
    scenery_destroy();
    game->state = STATE_INIT;
    game->timer = 0;
    game->flags |= GAME_CHANGE_ACT;
}

/* rank the racers, and give each its place */
static void race_rank(struct race_game *game)
{
    int i;

    standings_update(&game->standings, game->racers, game->lane);
    i = 0;
    do {
        int lane = standings_lane(&game->standings, i);

        game->racers[lane].place = i;
    } while (++i < RACE_LANES);
}

static void race_average(struct race_game *game)
{
    int i;

    game->average_distance = 0.0F;
    for (i = 0; i < RACE_LANES; i++) {
        game->average_distance += game->racers[i].distance;
    }
    game->average_distance *= 0.125F;
}

/* see the racers home, a step. an active racer past the course's distance
 * heads for the line, in RACER_FINISHING. a racer in RACER_FINISHING reaches
 * its goal, RACER_TO_GOAL, once race_path_at_target finds it at the path's
 * target. when the racer in first place reaches it, GOAL shows, and that
 * racer takes the camera in RACE_CAMERA_FRONT. saved_lane keeps the lane
 * followed before. an entry that is no rival enters the clock's time in the
 * records and in its Chao's. in RACE_RECORDED, an entry's first place sets
 * GAME_WON, and a first win of the kind sets GAME_FIRST_WIN and the records'
 * bit. once every entry that is no rival is at its goal, the state is
 * STATE_RACEEND */
static void race_finish_racers(struct race_game *game)
{
    Float finish = race_course.distances[game->kind];
    int done = RACE_LANES - game->entry_count;
    int i;

    i = 0;
    do {
        struct racer *racer = &game->racers[i];
        struct race_entry *entry = &game->entries[i];

        if (racer->distance > finish) {
            if (racer_is_active(racer)) {
                racer->state = RACER_FINISHING;
            }
        }
        if (racer->state == RACER_FINISHING &&
            race_path_at_target(&game->path, &racer->work->pos)) {
            racer->state = RACER_TO_GOAL;
            if (racer->place == 0) {
                menu_show_goal();
                menu_slot_look(0, LOOK_POP + 1);
                sound_play(SE_FINISH, 0, 0, 0);
                game->saved_lane = game->lane;
                game->lane = racer->lane;
                race_set_camera_mode(game, RACE_CAMERA_FRONT);
            }
            if (entry->kind == ENTRY_RIVAL) {
                continue;
            }
            race_records_enter(game->records, &game->clock.time,
                               racer->chao->record, game->kind);
            race_record_time(game, i);
            if (game->mode == RACE_RECORDED && !racer->place) {
                game->flags |= GAME_WON;
                if (!(game->records->flags & RACE_KIND_WON(game->kind))) {
                    game->flags |= GAME_FIRST_WIN;
                    game->records->flags |= RACE_KIND_WON(game->kind);
                }
            }
        }
        if (entry->kind != ENTRY_RIVAL && racer_at_goal(racer)) {
            done++;
        }
    } while (++i < RACE_LANES);
    if (done == RACE_LANES) {
        game->state = STATE_RACEEND;
        game->timer = 0;
    }
}

/* in STATE_RACEEND, move the racers still running to RACER_FINISHING and on
 * to RACER_TO_GOAL, as race_finish_racers does. this shows no GOAL, moves no
 * camera and enters no records */
static void race_finish_late_racers(struct race_game *game)
{
    Float finish = race_course.distances[game->kind];
    int i;

    i = 0;
    do {
        struct racer *racer = &game->racers[i];

        if (racer->distance > finish && racer_is_active(racer)) {
            racer->state = RACER_FINISHING;
        }
        if (racer->state == RACER_FINISHING) {
            if (race_path_at_target(&game->path, &racer->work->pos)) {
                racer->state = RACER_TO_GOAL;
            }
        }
    } while (++i < RACE_LANES);
}

void race_set_camera_mode(struct race_game *game, int mode)
{
    race_camera_release(&game->camera);
    game->camera.mode = mode;
    game->camera.phase = 0;
    game->camera_timer = 0;
}

/* steer the course's camera a step by its mode, from the pads and the
 * camera timer. in STATE_COMMENDATION_INI, first set RACE_CAMERA_FINISH.
 * while the crowd cheers or the set editor is open, the pads change
 * nothing:
 *
 *   RACE_CAMERA_WATCH         right and left on any pad follow the next
 *                             lane and the one before. down sets
 *                             RACE_CAMERA_FOLLOW
 *   RACE_CAMERA_FOLLOW        right and left as in RACE_CAMERA_WATCH, each
 *                             also clearing the camera timer. up sets
 *                             RACE_CAMERA_WATCH. while the racer plays,
 *                             turn toward heading 0
 *   RACE_CAMERA_COURSE_FIXED  set RACE_CAMERA_WATCH once the timer passes
 *                             138
 *   RACE_CAMERA_FRONT         set RACE_CAMERA_WATCH once the timer passes
 *                             150, and follow saved_lane again
 *   RACE_CAMERA_FINISH        follow the winner, and keep SE_WINNER playing
 *
 * RACE_CAMERA_AHEAD, RACE_CAMERA_FRONT, RACE_CAMERA_FRONT_2 and
 * RACE_CAMERA_BEHIND name the followed racer while the timer is below 45.
 * RACE_CAMERA_FOLLOW names it while the timer is 31 to 107, and
 * RACE_CAMERA_FINISH names the winner while it is below 180. last, clear
 * GAME_CHEERING and count the step on the timer */
static void race_camera_input(struct race_game *game)
{
    int mode = race_camera_get_mode(&game->camera);
    Sint16 *editor = &edit_mode;

    if (game->state == STATE_COMMENDATION_INI) {
        race_set_camera_mode(game, RACE_CAMERA_FINISH);
    }
    /* @hack redundant: a never-run store through editor, whose three references
     * give it R13 and mode R8, and leave the image's unused load of edit_mode's
     * address into R13 at 0x0cb8e3de; without it, 74 of 384 words and 22 of 144
     * other words differ */
    if (0) {
        *editor = *editor | mode;
    }
    switch (mode) {
    /* @hack redundant: a case for RACE_CAMERA_HALL, mode 1, that only breaks,
     * which starts the switch's table at mode 1 (0x0cb8e3fc); without it, the
     * table starts at 3, and 46 of 384 words and 12 of 144 other words
     * differ */
    case RACE_CAMERA_HALL:
        break;
    case RACE_CAMERA_AHEAD:
        if (game->camera_timer < 45.0F) {
            game->racers[game->lane].flags |= RACER_NAMED;
        }
        break;
    case RACE_CAMERA_COURSE_FIXED:
        if (game->camera_timer > 2.3F * 60.0F) {
            race_set_camera_mode(game, RACE_CAMERA_WATCH);
        }
        break;
    case RACE_CAMERA_WATCH:
        if ((game->flags & GAME_CHEERING) || edit_mode) {
            break;
        }
        if (ANY_PRESS(PDD_DGT_KR)) {
            race_follow_next(game);
        }
        /* @hack redundant: a never-run test of PDD_DGT_KR, which weighs it into
         * R12 ahead of PDD_DGT_KL in R11 (0x0cb8e3fa, 0x0cb8e3f8); without it,
         * 19 of 384 words differ */
        if (0) {
            if (mode & PDD_DGT_KR) {
            }
        }
        if (ANY_PRESS(PDD_DGT_KL)) {
            race_follow_previous(game);
        }
        if (ANY_PRESS(PDD_DGT_KD)) {
            race_set_camera_mode(game, RACE_CAMERA_FOLLOW);
        }
        break;
    case RACE_CAMERA_FOLLOW:
        if (racer_item_behavior(&game->racers[game->lane]) == 1) {
            race_camera_set_heading(&game->camera, 0);
        }
        if ((game->flags & GAME_CHEERING) || edit_mode) {
            break;
        }
        if (ANY_PRESS(PDD_DGT_KR)) {
            race_follow_next(game);
            game->camera_timer = 0;
        }
        if (ANY_PRESS(PDD_DGT_KL)) {
            race_follow_previous(game);
            game->camera_timer = 0;
        }
        if (ANY_PRESS(PDD_DGT_KU)) {
            race_set_camera_mode(game, RACE_CAMERA_WATCH);
        }
        if (game->camera_timer < 1.8F * 60.0F && game->camera_timer > 30.0F) {
            game->racers[game->lane].flags |= RACER_NAMED;
        }
        break;
    case RACE_CAMERA_FRONT:
        if (game->camera_timer > 150.0F) {
            game->lane = game->saved_lane;
            race_set_camera_mode(game, RACE_CAMERA_WATCH);
        }
        /* fall through */
    case RACE_CAMERA_FRONT_2:
    case RACE_CAMERA_BEHIND:
        if (game->camera_timer < 45.0F) {
            game->racers[game->lane].flags |= RACER_NAMED;
        }
        break;
    case RACE_CAMERA_FINISH:
        sound_play_for(SE_WINNER, (int)game, 1, 0, 2);
        if (game->camera_timer < 180.0F) {
            game->racers[game->standings.lanes[0]].flags |= RACER_NAMED;
        }
        game->lane = standings_lane(&game->standings, 0);
        break;
    /* @hack redundant: cases for RACE_CAMERA_NONE_2 and RACE_CAMERA_FREE, modes
     * 11 and 12, that only break, which run the switch's table to mode 12
     * (0x0cb8e3fe); without them, it tests mode by mode, the function starts at
     * 0x0cb8e38a, and 86 of 384 words and 22 of 144 other words differ */
    case RACE_CAMERA_NONE_2:
    case RACE_CAMERA_FREE:
        break;
    }
    game->flags &= ~GAME_CHEERING;
    game->camera_timer++;
}

static void race_follow_next(struct race_game *game)
{
    if (!(game->flags & GAME_CHEERING)) {
        if (++game->lane >= RACE_LANES) {
            game->lane = 0;
        }
    }
}

static void race_follow_previous(struct race_game *game)
{
    if (!(game->flags & GAME_CHEERING)) {
        if (--game->lane < 0) {
            game->lane = RACE_LANES - 1;
        }
    }
}

/* draw the standings' marker 8 above the followed racer's Chao, outside the
 * commendation */
static void race_draw_marker(struct race_game *game)
{
    struct task_work *work;
    NJS_POINT3 pos;

    if (race_in_commendation(game) == 1) {
        return;
    }
    work = racer_work(&game->racers[game->lane]);
    if (work != NULL) {
        /* @hack spelling: pos through its address, which forms each member's
         * stack address apart (0x0cb8e850, 0x0cb8e856, 0x0cb8e868); as
         * pos.x, pos.y and pos.z, 31 of 55 words differ */
        (&pos)->x = work->pos.x;
        (&pos)->y = work->pos.y;
        (&pos)->z = work->pos.z;
        pos.y += 8.0F;
        standings_draw_marker(&game->standings, game->lane + 1, &pos);
    }
}

/* suspend player 0, set GAME_SUSPENDED and return 1. while the flag is set
 * already, return 0 and do nothing. race_resume_player undoes the two, and
 * returns 0 and does nothing while the flag is clear */
static int race_suspend_player(struct race_game *game)
{
    if (!(game->flags & GAME_SUSPENDED)) {
        game->flags |= GAME_SUSPENDED;
        player_suspend(0);
        return 1;
    }
    return 0;
}

static int race_resume_player(struct race_game *game)
{
    if (game->flags & GAME_SUSPENDED) {
        game->flags &= ~GAME_SUSPENDED;
        player_resume(0);
        return 1;
    }
    return 0;
}

/* move *choice by pad 0's down and up, pressed or repeating. down takes the
 * next choice and up the one before, round from 0 to last. nothing while
 * the set editor is open */
static void race_cursor_cycle(Sint8 *choice, int last)
{
    if (edit_mode) {
        return;
    }
    if (pad_press_repeat[0] & PDD_DGT_KD) {
        if (++*choice > last) {
            *choice = 0;
        }
    }
    if (pad_press_repeat[0] & PDD_DGT_KU) {
        if (--*choice < 0) {
            *choice = last;
        }
    }
}

/* the race window: the three race modes, and back. a press of a on a mode
 * opens the cups window. back closes the window and puts player 0 in the
 * entrance at (2052, 5, 0). where the window puts up the panel, its icons
 * stay hidden while the panel grows */
static void race_window_step(struct race_game *game)
{
    struct race_places *places = game->places;
    int i;

    switch (places->step) {
    case 0:
        race_clear_entries(game);
        menu_show_modes();
        if (menu_panel_open()) {
            places->step = 1;
        } else {
            places->step = 2;
        }
        game->mode = RACE_RECORDED;
        places->timer = 0;
        break;
    case 1:
        if (places->timer < MENU_PANEL_STEPS) {
            for (i = 0; i < 5; i++) {
                menu_slot_look(i, LOOK_HIDDEN);
            }
        } else {
            for (i = 0; i < 5; i++) {
                menu_slot_look(i, LOOK_PLAIN);
            }
            places->step = 2;
        }
        break;
    /* @hack redundant: a case for step 2 beside the default, whose CMP/EQ #2
     * the switch makes (0x0cb8e99c); without it, 143 of 156 words and 73 pool
     * words differ, and the code after moves */
    case 2:
    default:
        menu_slot_look(game->mode, LOOK_PULSE_YELLOW);
        race_cursor_cycle(&game->mode, RACE_WINDOW_BACK);
        if (!edit_mode && (per_info[0]->press & PDD_DGT_TA)) {
            if (game->mode == RACE_WINDOW_BACK) {
                game->mode = RACE_RECORDED;
                menu_slots_clear();
                menu_panel_close();
                race_set_camera_mode(game, RACE_CAMERA_HALL);
                game->state = STATE_ENTRANCE_RE_INIT;
                race_resume_player(game);
                player_set_position(0, 2052.0F, 5.0F, 0.0F);
            } else {
                menu_slots_clear();
                race_open_cups_window();
            }
        }
        break;
    }
}

/* the cups window: the five kinds of race, and back. RACE_KIND_LAST shows
 * only while the records hold wins of kinds 0 to 3, and the cursor passes
 * over it otherwise. the cursor's kind shows its three best times. a press of a
 * on a kind opens the garden window and sets GAME_POLL_CARDS. back returns
 * to the race window */
static void cups_window_step(struct race_game *game)
{
    struct race_places *places = game->places;
    int first_four_won = 0;
    int i;
    int previous;
    int flags = game->records->flags;

    if ((flags & RACE_KIND_WON(0)) && (flags & RACE_KIND_WON(1)) &&
        (flags & RACE_KIND_WON(2)) && (flags & RACE_KIND_WON(3))) {
        first_four_won = 1;
    }
    switch (places->step) {
    case 0:
        menu_show_courses(first_four_won);
        game->kind = 0;
        places->step = 1;
        places->timer = 0;
        break;
    /* @hack redundant: a case for step 1 that only breaks, whose CMP/EQ #1 the
     * switch makes (0x0cb8eba4); without it, 52 of 168 words differ */
    case 1:
        break;
    }
    for (i = 0; i <= 4; i++) {
        menu_slot_look(i, LOOK_RED);
    }
    for (i = 5; i < 7; i++) {
        menu_slot_look(i, LOOK_PLAIN);
    }
    if (!first_four_won) {
        menu_slot_look(4, LOOK_HIDDEN);
    }
    if (game->kind != CUPS_WINDOW_BACK) {
        menu_slot_look(game->kind, LOOK_PULSE);
        race_records_draw(game->records, game->kind);
    } else {
        menu_slot_look(game->kind, LOOK_PULSE_YELLOW);
    }
    /* kind holds 0 to 5, and widening it to int keeps the value */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    previous = game->kind;
    race_cursor_step(game, &game->kind, CUPS_WINDOW_BACK);
    if (!first_four_won && game->kind == RACE_KIND_LAST) {
        if (previous == RACE_KIND_LAST - 1) {
            game->kind = CUPS_WINDOW_BACK;
        } else if (previous == CUPS_WINDOW_BACK) {
            game->kind = RACE_KIND_LAST - 1;
        }
    }
    menu_draw_course(game->kind);
    if (!edit_mode && (per_info[0]->press & PDD_DGT_TA)) {
        switch (game->kind) {
        case CUPS_WINDOW_BACK:
            menu_slots_clear();
            game->kind = 0;
            race_open_race_window();
            break;
        default:
            menu_slots_clear();
            race_open_garden_window();
            game->flags |= GAME_POLL_CARDS;
            break;
        }
    }
}

/* the entry kind of each of the garden window's choices but back, in the
 * order its slots show them */
static const Sint8 garden_kinds[4] = {
    ENTRY_CARD,
    GARDEN_STATION_SQUARE,
    GARDEN_MYSTIC_RUINS,
    GARDEN_EGG_CARRIER,
};

/* the garden window: the cards, the three gardens, and back. the window
 * reads the cards' Chao before it takes a choice. a press of a on a
 * choice, once the cards are idle, fills the board with its Chao and opens
 * the chao window. back clears GAME_POLL_CARDS and returns to the cups
 * window. a card put in or taken out clears the entries and opens the
 * window again */
static void garden_window_step(struct race_game *game)
{
    struct race_places *places = game->places;
    int i;

    switch (places->step) {
    case 0:
        menu_show_gardens();
        places->kind = 0;
        places->timer = 0;
        places->step = 1;
        break;
    case 1:
        if (!race_cards_busy(&game->cards)) {
            for (i = 0; i < 6; i++) {
                menu_slot_look(i, LOOK_HIDDEN);
            }
            race_load_cards(&game->cards, game->chao);
            for (i = 0; i < 6; i++) {
                menu_slot_look(i, LOOK_PLAIN);
            }
            places->step = 2;
        }
        break;
    case 2:
        if (!race_cards_busy(&game->cards)) {
            places->step = 3;
        }
        break;
    case 3:
        /* @hack type: the table's kind compared unsigned, which writes the
         * EXTU.B at 0x0cb8ee06; as a signed byte, 7 of 213 words differ */
        if (places->kind == GARDEN_WINDOW_BACK ||
            (Uint8)garden_kinds[places->kind] == ENTRY_CARD) {
            menu_slot_look(places->kind, LOOK_PULSE_YELLOW);
        } else {
            menu_slot_look(places->kind, LOOK_PULSE);
        }
        race_cursor_step(game, &places->kind, GARDEN_WINDOW_BACK);
        if (race_cards_changed(&game->cards)) {
            menu_slots_clear();
            race_open_garden_window();
            race_clear_entries(game);
            return;
        } else if (!edit_mode && (per_info[0]->press & PDD_DGT_TA)) {
            if (places->kind == GARDEN_WINDOW_BACK) {
                menu_slots_clear();
                places->kind = 0;
                game->flags &= ~GAME_POLL_CARDS;
                race_open_cups_window();
                return;
            } else {
                places->step = 4;
            }
        }
        break;
    case 4:
        if (race_cards_busy(&game->cards)) {
            break;
        }
        menu_slots_clear();
        places->kind = garden_kinds[places->kind];
        race_place_chao(game, PLACE_BOARD);
        race_open_chao_window();
        break;
    }
}

/* the chao window: the board's places, start and back. a press of a enters
 * the cursor's Chao, but not an egg, in the first lane whose entry is still
 * ENTRY_RIVAL. RACE_RECORDED and RACE_MATCHED take one entry, which puts the
 * cursor on start, and RACE_GROUP takes one a lane. b takes the cursor's Chao
 * out again. l and r slide the board to the entry kind before or after. start,
 * once an entry is made, sends the entrants to the gates when the cards are
 * idle. back returns to the garden window, as a card put in or taken out
 * does, which also clears the entries */
static void chao_window_step(struct race_game *game)
{
    struct race_places *places = game->places;
    int most = game->mode == RACE_GROUP ? RACE_LANES : 1;
    int i;
    int old_cursor;
    int left;
    int found;

    switch (places->step) {
    case 0:
        menu_show_entry();
        for (places->cursor = 0; places->cursor < RACE_LANES;
             places->cursor++) {
            if (entrant_ready(&places->places[places->cursor])) {
                break;
            }
        }
        places->timer = 0;
        places->step = 1;
        for (i = 0; i < 4; i++) {
            menu_slot_look(i, LOOK_PLAIN);
        }
        break;
    case 1:
        i = 0;
        do {
            struct race_entry *entry = &game->entries[i];

            /* the image compares the unsigned kind with the signed one. both
             * hold 0 to 4, so the signs agree */
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            if (entry->kind == places->kind) {
                places->places[entry->index].state = 6;
            }
        } while (++i < RACE_LANES);
        /* @hack spelling: a jump into the else arm, so the nested tests share
         * its one store of step 5, and the cursor is read again for the
         * call as at 0x0cb8f0da; as one test with &&, 41 of 751 words and 1
         * pool word differ */
        if (places->cursor < RACE_LANES) {
            if (!entrant_entered(&places->places[places->cursor])) {
                places->step = 2;
            } else {
                goto chosen;
            }
        } else {
        chosen:
            places->step = 5;
        }
        break;
    case 2:
        if (places->cursor < RACE_LANES) {
            places->places[places->cursor].state = 4;
            sound_play(SE_PICK, 0, 0, 0);
        }
        places->step = 5;
        break;
    case 3:
        if (places->cursor < RACE_LANES) {
            places->places[places->cursor].state = 2;
        }
        places->step = 5;
        break;
    case 4:
        if (places->cursor < RACE_LANES) {
            places->places[places->cursor].state = 6;
            sound_play(SE_ENTER, 0, 0, 0);
        }
        places->step = 5;
        break;
    /* @hack redundant: a case for step 5 that only breaks, whose CMP/EQ #5 the
     * switch makes (0x0cb8eff4); without it, 59 of 751 words differ */
    case 5:
        break;
    case 6:
        places->step = 7;
        /* fall through */
    case 7:
        if (!race_cards_busy(&game->cards)) {
            menu_slots_clear();
            menu_panel_close();
            for (i = 0; i < RACE_LANES; i++) {
                entrant_remove(&places->places[i]);
            }
            game->state = STATE_CHAO_DEMO_0;
            game->timer = 0;
        }
        break;
    }
    if (places->kind == ENTRY_CARD) {
        menu_draw_drives();
    }
    menu_draw_gardens(places->kind);
    /* the cursor holds 0 to 9, and widening it to int keeps the value */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    old_cursor = places->cursor;
    i = 0;
    do {
        NJS_POINT3 pos;

        if (entrant_ready(&places->places[i])) {
            if (i == places->cursor) {
                entrant_screen_pos(&places->places[i], &pos);
                menu_draw_sprite_at(pos.x, pos.y - 25.0F, 0.5F, 1);
                places->places[i].flags |= ENTRANT_NAME;
            }
            if (entrant_entered(&places->places[i])) {
                if (i != places->cursor) {
                    entrant_screen_pos(&places->places[i], &pos);
                }
                menu_draw_sprite_at(pos.x, pos.y, 1.0F, 0);
            }
        }
    } while (++i < RACE_LANES);
    {
        struct race_digit digit;

        left = most - game->entry_count;
        digit.x = 300.0F;
        digit.y = 355.0F;
        digit.sx = 0.85F;
        digit.sy = 1.0F;
        digit.color.a = 1.0F;
        if (left) {
            digit.color.r = digit.color.g = left * 0.125F;
            digit.color.b = 1.0F - (left * 0.125F);
        } else {
            digit.color.r = 1.0F;
            digit.color.g = digit.color.b = 0.0F;
        }
        race_draw_digit(&digit, left, 1);
    }
    switch (places->cursor) {
    case BOARD_START:
        menu_slot_look(0, LOOK_PULSE_YELLOW);
        break;
    case BOARD_BACK:
        menu_slot_look(1, LOOK_PULSE_YELLOW);
        break;
    }
    if (race_cards_changed(&game->cards)) {
        menu_slots_clear();
        for (i = 0; i < RACE_LANES; i++) {
            entrant_remove(&places->places[i]);
        }
        race_clear_entries(game);
        race_open_garden_window();
        return;
    }
    if (places->step == 6 || places->step == 7) {
        return;
    }
    if (edit_mode) {
        return;
    }
    if (per_info[0]->press & PDD_DGT_TL) {
        chao_window_slide(game, menu_garden_before(places->kind));
        return;
    }
    if (per_info[0]->press & PDD_DGT_TR) {
        chao_window_slide(game, menu_garden_after(places->kind));
        return;
    }
    if (per_info[0]->press & PDD_DGT_TA) {
        switch (places->cursor) {
        case BOARD_START:
            if (game->entry_count != 0) {
                places->step = 6;
            }
            break;
        case BOARD_BACK:
            menu_slots_clear();
            for (i = 0; i < RACE_LANES; i++) {
                entrant_remove(&places->places[i]);
            }
            race_open_garden_window();
            break;
        default:
            if (entrant_shows_egg(&places->places[places->cursor])) {
                break;
            }
            /* @hack spelling: the limit worked out again, which reads the
             * mode back and selects 8 or 1 at 0x0cb8f576 to 0x0cb8f584; as
             * most, 33 of 751 words differ */
            if (game->entry_count ==
                (game->mode == RACE_GROUP ? RACE_LANES : 1)) {
                break;
            }
            found = 0;
            for (i = 0; i < RACE_LANES; i++) {
                struct race_entry *entry = &game->entries[i];

                /* the image compares the unsigned kind and index with the
                 * signed kind and cursor. kinds hold 0 to 4, and the cursor
                 * 0 to 7 here, so the signs agree */
                /* NOLINTBEGIN(bugprone-signed-char-misuse) */
                if (entry->kind == places->kind) {
                    if (entry->index == places->cursor) {
                        /* NOLINTEND(bugprone-signed-char-misuse) */
                        found = 1;
                        break;
                    }
                }
            }
            if (!found) {
                for (i = 0; i < RACE_LANES; i++) {
                    struct race_entry *entry = &game->entries[i];

                    if (entry->kind == ENTRY_RIVAL) {
                        entry->kind = places->kind;
                        entry->index = places->cursor;
                        places->step = 4;
                        game->entry_count++;
                        if (game->mode != RACE_GROUP) {
                            places->places[places->cursor].state = 6;
                            sound_play(SE_ENTER, 0, 0, 0);
                            /* @hack spelling: the places reached again through
                             * game (0x0cb8f63e, 0x0cb8f640); through places,
                             * 6 of 751 words and 4 pool words differ */
                            game->places->cursor = BOARD_START;
                            return;
                        }
                        break;
                    }
                }
            }
        }
    }
    if (!edit_mode && (per_info[0]->press & PDD_DGT_TB)) {
        switch (places->cursor) {
        case BOARD_BACK:
        case BOARD_START:
            break;
        default:
            if (game->entry_count) {
                for (i = 0; i < RACE_LANES; i++) {
                    struct race_entry *entry = &game->entries[i];

                    /* the same comparison as in the search above */
                    /* NOLINTBEGIN(bugprone-signed-char-misuse) */
                    if (entry->kind == places->kind) {
                        if (entry->index == places->cursor) {
                            /* NOLINTEND(bugprone-signed-char-misuse) */
                            entry->kind = ENTRY_RIVAL;
                            entry->index = 8;
                            places->step = 2;
                            game->entry_count--;
                            break;
                        }
                    }
                }
            }
            break;
        }
    }
    race_board_cursor(game);
    if (old_cursor != places->cursor) {
        if (old_cursor < RACE_LANES) {
            if (!entrant_entered(&places->places[old_cursor])) {
                places->places[old_cursor].state = 2;
            }
        }
        if (places->cursor < RACE_LANES) {
            if (!entrant_entered(&places->places[places->cursor])) {
                places->step = 2;
            }
        }
    }
}

/* empty the board and set STATE_CHAO_WINDOW_SLIDE, whose step fills it
 * with kind's Chao */
static void chao_window_slide(struct race_game *game, int kind)
{
    struct race_places *places = game->places;
    int i;

    menu_slots_clear();
    for (i = 0; i < RACE_LANES; i++) {
        entrant_remove(&places->places[i]);
    }
    places->kind = kind;
    game->state = STATE_CHAO_WINDOW_SLIDE;
    game->timer = 0;
}

/* the entrants at the gates. STATE_CHAO_DEMO_0 sets GAME_WATCH_CARDS and
 * makes them there. STATE_CHAO_DEMO_1 waits for the places' timer to pass
 * 120, and STATE_CHAO_DEMO_2 then removes them. player 0 then has the hall
 * again, and GAME_TO_COURSE takes the game to the course from
 * STATE_ENTRANCE */
static void chao_demo_step(struct race_game *game)
{
    struct race_places *places = game->places;
    int i;

    switch (game->state) {
    case STATE_CHAO_DEMO_0:
        game->flags |= GAME_WATCH_CARDS;
        race_place_chao(game, PLACE_GATES);
        i = 0;
        do {
            places->places[i].state = 8;
        } while (++i < RACE_LANES);
        sound_play(SE_TO_GATES, 0, 0, 0);
        places->timer = 0;
        game->state = STATE_CHAO_DEMO_1;
        game->timer = 0;
        break;
    case STATE_CHAO_DEMO_1:
        if (places->timer > 120) {
            game->state = STATE_CHAO_DEMO_2;
            game->timer = 0;
        }
        break;
    case STATE_CHAO_DEMO_2:
        for (i = 0; i < RACE_LANES; i++) {
            entrant_remove(&places->places[i]);
        }
        race_set_camera_mode(game, RACE_CAMERA_HALL);
        race_resume_player(game);
        game->state = STATE_ENTRANCE_RE_INIT;
        game->timer = 0;
        game->flags |= GAME_TO_COURSE;
        break;
    /* @hack redundant: a case for STATE_CHAO_DEMO_3 that only breaks, whose
     * CMP/EQ #20 the switch makes (0x0cb8f84a); without it, 55 of 119 words
     * differ */
    case STATE_CHAO_DEMO_3:
        break;
    }
}

/* move *choice by pad 0, pressed or repeating: right to the next choice
 * and left to the one before, round from 0 to last. up or down takes it to
 * last and keeps the choice it leaves in saved_choice, or takes it from
 * last back to saved_choice. nothing while the set editor is open */
static void race_cursor_step(struct race_game *game, Sint8 *choice, int last)
{
    if (!edit_mode) {
        if (pad_press_repeat[0] & PDD_DGT_KR) {
            (*choice)++;
            if (*choice > last) {
                *choice = 0;
            }
        }
        if (pad_press_repeat[0] & PDD_DGT_KL) {
            (*choice)--;
            if (*choice < 0) {
                *choice = last;
            }
        }
        /* @hack spelling: up and down tested apart and or'd, which spills
         * the pad word to the frame and ANDs and ORs the two masks
         * (0x0cb8f970 to 0x0cb8f97c); as one mask, 45 of 49 words differ */
        if ((pad_press_repeat[0] & PDD_DGT_KU) |
            (pad_press_repeat[0] & PDD_DGT_KD)) {
            if (*choice != last) {
                game->saved_choice = *choice;
                *choice = last;
            } else {
                *choice = game->saved_choice;
            }
        }
    }
}

/* move the board's cursor by pad 0, pressed or repeating. the cursor stops
 * only on a ready place, or on start or back. the board is two rows of four
 * places, the even ones above, with start below its left half and back
 * below its right:
 *
 *   down   from an upper place, the first ready place in the lower row,
 *          searched rightward round the row from the place below. with
 *          none, and from a lower place, start or back, whichever is below
 *   up     from a lower place, the first ready place in the upper row,
 *          searched leftward round the row from the place above. from start
 *          or back, the lower row's rightmost ready place, or else the upper
 *          row's
 *   right  race_board_right, past the cursor's own place
 *   left   race_board_left, past the cursor's own place
 *
 * up with no place ready leaves the cursor where it was. nothing while the
 * set editor is open */
static void race_board_cursor(struct race_game *game)
{
    struct race_places *places = game->places;
    int previous;

    if (edit_mode) {
        return;
    }
    if (pad_press_repeat[0] & PDD_DGT_KD) {
        if (places->cursor < RACE_LANES) {
            if (places->cursor & 1) {
                if (places->cursor < 4) {
                    places->cursor = BOARD_START;
                } else {
                    places->cursor = BOARD_BACK;
                }
            } else {
                places->cursor++;
                if (!race_board_right(places, 0)) {
                    if (places->cursor < 4) {
                        places->cursor = BOARD_START;
                    } else {
                        places->cursor = BOARD_BACK;
                    }
                }
            }
        }
    }
    if (pad_press_repeat[0] & PDD_DGT_KU) {
        /* the cursor holds 0 to 9, and widening it to int keeps the value */
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        previous = places->cursor;
        if (places->cursor < RACE_LANES) {
            if (places->cursor & 1) {
                places->cursor--;
                if (!race_board_left(places, 0)) {
                    places->cursor = previous;
                }
            }
        } else {
            places->cursor = 7;
            if (!race_board_left(places, 0)) {
                places->cursor = 6;
                if (!race_board_left(places, 0)) {
                    places->cursor = previous;
                }
            }
        }
    }
    if (pad_press_repeat[0] & PDD_DGT_KR) {
        race_board_right(places, 1);
    }
    if (pad_press_repeat[0] & PDD_DGT_KL) {
        race_board_left(places, 1);
    }
}

/* move the board's cursor right round its row to a ready place, and return
 * 1. the search starts at the cursor's own place, or with skip set, at the
 * next. with none ready, return 0, the cursor where it was. on start or
 * back, swap the two and return 1 */
static int race_board_right(struct race_places *places, int skip)
{
    int count;
    int i;

    if (places->cursor < RACE_LANES) {
        if (skip) {
            places->cursor += 2;
            if (places->cursor >= RACE_LANES) {
                places->cursor -= RACE_LANES;
            }
            count = 3;
        } else {
            count = 4;
        }
        for (i = 0; i < count; i++) {
            if (entrant_ready(&places->places[places->cursor])) {
                return 1;
            }
            places->cursor += 2;
            if (places->cursor >= RACE_LANES) {
                places->cursor -= RACE_LANES;
            }
        }
        return 0;
    }
    if (places->cursor == BOARD_BACK) {
        places->cursor = BOARD_START;
    } else {
        places->cursor = BOARD_BACK;
    }
    return 1;
}

/* the same, leftward round the row */
static int race_board_left(struct race_places *places, int skip)
{
    int count;
    int i;

    if (places->cursor < RACE_LANES) {
        if (skip) {
            places->cursor -= 2;
            if (places->cursor < 0) {
                places->cursor += RACE_LANES;
            }
            count = 3;
        } else {
            count = 4;
        }
        for (i = 0; i < count; i++) {
            if (entrant_ready(&places->places[places->cursor])) {
                return 1;
            }
            places->cursor -= 2;
            if (places->cursor < 0) {
                places->cursor += RACE_LANES;
            }
        }
        return 0;
    }
    if (places->cursor == BOARD_BACK) {
        places->cursor = BOARD_START;
    } else {
        places->cursor = BOARD_BACK;
    }
    return 1;
}

/* set the cheer waiting until the game's timer passes a random count from
 * 1200 up to 1800 */
static void race_cheer_init(struct race_cheer *cheer)
{
    cheer->timer = RANDOM(1200.0F, 1800.0F);
    cheer->state = CHEER_WAIT;
    cheer->rounds = 0;
    cheer->presses = 0;
    cheer->sound_wait = 0;
}

/* 1 in CHEER_CHOSEN, CHEER_CHEERING and CHEER_DONE, else 0 */
static int race_cheer_started(struct race_cheer *cheer)
{
    if (!cheer->state) {
        return 0;
    }
    return 1;
}

/* run the crowd's cheer a step of the race.
 *
 * while the cheer waits, a press on any pad sounds the followed lane's
 * cheer, at most every fifth step. once the game's timer passes the wait,
 * the crowd chooses the later of two random places. once the seventh
 * place's racer is at its goal, and the lane chosen last isn't a rival's,
 * it chooses the last place instead. half the time it passes over a
 * rival's racer. it always passes over a racer at its goal, climbing, in
 * RACER_WALKING or in RACER_FINISHING. it then chooses again on the next
 * step.
 *
 * the chosen racer takes the camera in RACE_CAMERA_FOLLOW. a press within
 * 120 steps, while the race still follows its lane, starts the cheering.
 * the press ends the racer's effect, and lanes 0 to 5 ask race_request_voice
 * for a voice of the lane's group. the voice is a later one while the racer
 * has RACER_HELD. each step of the cheering sets RACER_CHEERED on the racer
 * and GAME_CHEERING, which holds the lane. every three presses make a
 * round, and restart a count of steps. the cheering ends once that count
 * passes 30, or after the fifth round. the crowd then waits 420 up to 900
 * more of the game's steps */
static void race_cheer_step(struct race_cheer *cheer, struct race_game *game)
{
    int last_place = 0;
    int place;
    int first;
    int second;
    NJS_POINT3 pos;

    if (cheer->state == CHEER_CHOSEN || cheer->state == CHEER_CHEERING) {
        /* @hack spelling: pos through its address, which forms each member's
         * stack address apart (0x0cb8fd10, 0x0cb8fd30, 0x0cb8fd50); as
         * pos.x, pos.y and pos.z, 609 of 636 words and 229 pool words
         * differ */
        (&pos)->x = game->racers[cheer->lane].work->pos.x;
        (&pos)->y = game->racers[cheer->lane].work->pos.y;
        (&pos)->z = game->racers[cheer->lane].work->pos.z;
        pos.x += RANDOM(-2.5F, 2.5F);
        pos.y += 2.5F;
        pos.z += RANDOM(-2.5F, 2.5F);
        sparkle_create_glint(&pos, 0.25F, 0.25F);
    }
    switch (cheer->state) {
    case CHEER_WAIT:
        if (game->entries[cheer->lane].kind != ENTRY_RIVAL) {
            int lane = game->standings.lanes[6];

            if (racer_at_goal(&game->racers[lane])) {
                last_place = 1;
            }
        }
        if (cheer->timer >= game->timer) {
            if (--cheer->sound_wait < 0) {
                cheer->sound_wait = 0;
                if (CHEER_PRESS()) {
                    int se;

                    switch (game->lane) {
                    case 0:
                        se = SE_CHEER_0;
                        break;
                    case 1:
                        se = SE_CHEER_1;
                        break;
                    case 2:
                        se = SE_CHEER_2;
                        break;
                    case 3:
                        se = SE_CHEER_3;
                        break;
                    case 4:
                        se = SE_CHEER_4;
                        break;
                    case 5:
                        se = SE_CHEER_5;
                        break;
                    case 6:
                        se = SE_CHEER_6;
                        break;
                    /* @hack redundant: a case for lane 7 beside the default,
                     * whose CMP/EQ #7 the switch makes (0x0cb8fef2); without
                     * it, 4 of 648 words differ */
                    case 7:
                    default:
                        se = SE_CHEER_7;
                        break;
                    }
                    cheer->sound_wait = 4;
                    sound_play(se, 0, 0, 0);
                }
            }
        } else {
            if (last_place != 1) {
                first = (int)RANDOM(0.0F, 8.0F);
                second = (int)RANDOM(0.0F, 8.0F);
                place = first > second ? first : second;
            } else {
                place = 7;
            }
            cheer->lane = standings_lane(&game->standings, place);
            if (game->entries[cheer->lane].kind == ENTRY_RIVAL) {
                int skip = (int)RANDOM(0.0F, 2.0F);

                if (skip) {
                    break;
                }
            }
            if (racer_at_goal(&game->racers[cheer->lane])) {
                break;
            }
            if (game->racers[cheer->lane].state == RACER_FINISHING) {
                break;
            } else if (game->racers[cheer->lane].state == RACER_WALKING) {
                break;
            } else if (game->racers[cheer->lane].flags & RACER_CLIMBING) {
                break;
            }
            game->lane = cheer->lane;
            race_set_camera_mode(game, RACE_CAMERA_FOLLOW);
            sound_play(SE_CONFIRM, 0, 0, 0);
            cheer->state = CHEER_CHOSEN;
            cheer->timer = 0;
        }
        break;
    case CHEER_CHOSEN:
        ++cheer->timer;
        /* the image compares the unsigned lane with the signed one. both
         * hold 0 to 7, so the signs agree */
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        if (cheer->lane == game->lane && cheer->timer < 120) {
            if (CHEER_PRESS()) {
                int lane = cheer->lane;

                if (lane >= 0 && lane <= 5) {
                    if (game->racers[cheer->lane].flags & RACER_HELD) {
                        race_request_voice(cheer->lane, 1);
                    } else {
                        race_request_voice(cheer->lane, 0);
                    }
                }
                cheer->rounds = 0;
                cheer->state = CHEER_CHEERING;
                cheer->presses = 0;
                cheer->timer = 0;
                cheer->sound_wait = 0;
                racer_clear_effect(&game->racers[cheer->lane]);
            }
        } else {
            cheer->state = CHEER_DONE;
        }
        break;
    case CHEER_CHEERING:
        game->racers[cheer->lane].flags |= RACER_CHEERED;
        game->flags |= GAME_CHEERING;
        if (--cheer->sound_wait < 0) {
            cheer->sound_wait = 0;
        }
        if (CHEER_PRESS()) {
            ++cheer->presses;
            if (cheer->sound_wait == 0 &&
                (cheer->lane == 6 || cheer->lane == 7)) {
                int voice = (int)RANDOM(0.0F, 2.0F);

                if (voice == 0) {
                    sound_play(SE_CHEER_6, 0, 0, 0);
                } else {
                    sound_play(SE_CHEER_7, 0, 0, 0);
                }
            }
        }
        if (cheer->presses >= 3) {
            cheer->presses = 0;
            cheer->timer = 0;
            ++cheer->rounds;
        }
        if (cheer->timer++ > 30 || cheer->rounds > 4) {
            cheer->state = CHEER_DONE;
        }
        break;
    case CHEER_DONE:
        cheer->timer = (rand() * (480.0F * RAND_SCALE)) + game->timer + 420.0F;
        cheer->state = CHEER_WAIT;
        break;
    }
}

/* make the clock's time the best of lane's Chao in the race's kind, where
 * it is shorter than that best. a best of zero stands for none, and takes
 * the time too. a card entry's racer then takes RACER_CHANGED */
static void race_record_time(struct race_game *game, int lane)
{
    struct race_entry *entry = &game->entries[lane];
    struct racer *racer = &game->racers[lane];
    struct race_time *time = &game->clock.time;
    /* @hack spelling: the kind's time indexed from the first one's address,
     * which spares the scaled index the EXTS.B SHC writes for
     * race_times[game->kind]; as that, 53 of 73 words and 1 pool word
     * differ */
    struct race_time *best = &(&racer->chao->record->race_times[0])[game->kind];
    struct race_time was;

    race_time_copy(&was, best);
    if (!(best->steps == 0 && best->seconds == 0 && best->minutes == 0) &&
        race_time_compare(time, &was) != 0) {
        return;
    }
    race_time_copy_to(time, best);
    if (entry->kind == ENTRY_CARD) {
        racer->flags |= RACER_CHANGED;
    }
}

/* clear GAME_WATCH_CARDS. then, with GAME_WON, set the kind's bit in the
 * race_wins of the first place's Chao. where the bit is new, a card entry's
 * racer takes RACER_CHANGED */
static void race_record_win(struct race_game *game)
{
    int lane;
    struct alife_chao *chao;
    struct race_entry *entry;

    game->flags &= ~GAME_WATCH_CARDS;
    if (!(game->flags & GAME_WON)) {
        return;
    }
    lane = standings_lane(&game->standings, 0);
    chao = racer_chao(&game->racers[lane]);
    if (chao != NULL) {
        if (!(chao->record->race_wins & (1 << game->kind))) {
            chao->record->race_wins |= 1 << game->kind;
            entry = &game->entries[lane];
            if (entry->kind == ENTRY_CARD) {
                game->racers[lane].flags |= RACER_CHANGED;
            }
        }
    }
}

/* the struct race_entry offset bytes past rows */
#define ENTRY_AT(rows, offset)                                                 \
    ((struct race_entry *)((unsigned char *)(rows) + (offset)))

/* shuffle the entries: each lane in turn trades its entry with that of a
 * lane chosen at random from itself to the last */
static void race_shuffle_entries(struct race_game *game)
{
    int i = 0;
    int j;
    unsigned long chosen;
    struct race_entry held;

    do {
        j = (int)(((Float)rand() * (Float)(RACE_LANES - i) * RAND_SCALE) +
                  (Float)i);
        /* @hack spelling: held's members through its address, here and where
         * the row takes them back below, which forms each one's stack
         * address apart (0x0cb90598, 0x0cb905ac); as held.kind and
         * held.index in all four, 74 of 87 words differ */
        (&held)->kind = game->entries[i].kind;
        (&held)->index = game->entries[i].index;
        /* @hack type: the chosen row by its byte offset, which keeps the base
         * copied before the offset is added (0x0cb905c2, 0x0cb905ce); as
         * entries[j], 17 of 87 words differ */
        /* @hack spelling: chosen set inside the row's address, which keeps i *
         * 2 in R12 and the offset in R11 (0x0cb9058c, 0x0cb905c0); set before
         * it, 36 of 87 words differ */
        game->entries[i].kind =
            ENTRY_AT(game->entries,
                     chosen = (unsigned long)j * sizeof(struct race_entry))
                ->kind;
        /* @hack redundant: a loop of one pass, which holds the row offsets in
         * saved registers; without it, 81 of 87 words differ */
        do {
            /* @hack spelling: a copy of chosen, which coalesces with it in
             * R11; without it, 82 of 87 words differ */
            unsigned long offset = chosen;

            game->entries[i].index = ENTRY_AT(game->entries, offset)->index;
            ENTRY_AT(game->entries, offset)->kind = (&held)->kind;
            ENTRY_AT(game->entries, offset)->index = (&held)->index;
            /* @hack redundant: never-run tests, which weigh game, i and
             * i * 2 into R14, R13 and R12; without them, 31 of 87 words
             * differ */
            if (0) {
                if (game->entries[i].kind) {
                }
            }
            /* @hack redundant: the second of the never-run tests the tag above
             * counts together, a test of i, which weighs i into R13 and i * 2
             * into R12 (0x0cb9058c, 0x0cb90596); without it, 23 of 87 words
             * differ */
            if (0) {
                if (i) {
                }
            }
            /* @hack redundant: the third of the never-run tests the tag above
             * counts together, a second test of i, which does the same as the
             * second; without it, 23 of 87 words differ */
            if (0) {
                if (i) {
                }
            }
            break;
        } while (0);
    } while (++i < RACE_LANES);
}

/* 1 in STATE_COMMENDATION_INI to STATE_COMMENDATION_2, else 0 */
static int race_in_commendation(struct race_game *game)
{
    if (game->state >= STATE_COMMENDATION_INI &&
        game->state <= STATE_COMMENDATION_2) {
        return 1;
    }
    return 0;
}
