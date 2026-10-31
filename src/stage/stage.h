/* the current stage, act and character, and the frame steps that run a
 * stage to its end */
#ifndef STAGE_STAGE_H
#define STAGE_STAGE_H
#include <ninja.h>
#include <sg_xpt.h>

struct particle;
struct story_record;
struct task;
struct task_work;

/* the stage numbers, in this order: the hedgehog hammer, the action stages
 * to STAGE_ACTION_LAST and the bosses to STAGE_BOSS_LAST. then come the
 * fields, the mini games, the Chao gardens and the Chao race. no stage has
 * the number 11, 13 or 14 */
#define STAGE_PRACTICE 0
#define STAGE_BEACH 1
#define STAGE_WINDY 2
#define STAGE_TWINKLE 3
#define STAGE_HIGHWAY 4
#define STAGE_MOUNTAIN 5
#define STAGE_SKY_DECK 6
#define STAGE_RUIN 7
#define STAGE_ICECAP 8
#define STAGE_CASINO 9
#define STAGE_FINAL 10
#define STAGE_SHELTER 12
#define STAGE_ACTION_LAST 14
#define STAGE_CHAOS0 15
#define STAGE_CHAOS2 16
#define STAGE_CHAOS4 17
#define STAGE_CHAOS6 18
#define STAGE_CHAOS7 19
#define STAGE_EGGMOBILE1 20
#define STAGE_EGGMOBILE2 21
#define STAGE_EGGMOBILE3 22
#define STAGE_EGGMANROBO 23
#define STAGE_E101 24
#define STAGE_E101_R 25
#define STAGE_BOSS_LAST 25
#define STAGE_STATION_SQUARE 26
/* station square under two more numbers, which no stage select row
 * lists. they load its files, and stage_place_player places the player in
 * them as in station square */
#define STAGE_STATION_SQUARE_2 27
#define STAGE_STATION_SQUARE_3 28
#define STAGE_EGG_CARRIER 29 /* ADV01OBJ, with ADV0100 */
/* the egg carrier under two more numbers, which no stage select row
 * lists. the sound, the hud and other code treat them as the egg carrier,
 * but loader_read_stage_files has no files for them, and spins forever */
#define STAGE_EGG_CARRIER_2 30
#define STAGE_EGG_CARRIER_3 31
#define STAGE_EGG_CARRIER_INSIDE 32 /* ADV01OBJ, with ADV0130 */
#define STAGE_MYSTIC_RUINS 33
#define STAGE_PAST 34
#define STAGE_CART 35
#define STAGE_SHOOTING1 36
#define STAGE_SHOOTING2 37
#define STAGE_SUNABOARD 38
#define STAGE_ALIFE_SS 39
#define STAGE_ALIFE_EG 40
#define STAGE_ALIFE_MR 41
#define STAGE_ALIFE_RACE 42

/* STAGE_NONE, one past the last stage, ends a list of stages */
#define STAGE_COUNT 43
#define STAGE_NONE STAGE_COUNT
/* rows in a table by stage, one more for STAGE_NONE */
#define STAGE_ROWS (STAGE_COUNT + 1)
/* the acts each stage has room for in a table by stage and act */
#define STAGE_ACTS 8

/* a stage and act in one number: the stage in the high byte, and the act
 * in the low. masked with STAGE_MASK, the number is STAGE_ACT(stage, 0),
 * and STAGE_OF is the stage alone */
#define STAGE_ACT(stage, act) ((stage) << 8 | (act))
#define STAGE_MASK 0xff00
#define STAGE_OF(stage_act) (((stage_act) & STAGE_MASK) >> 8)
#define STAGE_TWINKLE_MIRRORS STAGE_ACT(STAGE_TWINKLE, 2)
/* whether the current stage is stage, in any act */
#define IN_STAGE(stage)                                                        \
    ((stage_get_stage_act() & STAGE_MASK) == STAGE_ACT((stage), 0))

/* the step a stage runs on: stage_step_field for the fields and the Chao
 * stages, and stage_step_action for the rest */
enum stage_group { STAGE_GROUP_FIELD = 1, STAGE_GROUP_ACTION = 2 };

/* how the act ends, which stage_end records for the stage's step */
enum stage_end {
    STAGE_END_NONE = 0,
    STAGE_END_CLEAR = 1,
    STAGE_END_MISS = 2, /* the player is down: back to the restart point */
    STAGE_END_GO_TO = 3, /* to the next stage and act */
    STAGE_END_RESTART = 4, /* the pause menu's restart */
    STAGE_END_RETRY = 5, /* the act from its start */
    STAGE_END_QUIT = 6
};

/* what stage_step_action and stage_step_field return, besides 0 while the
 * stage runs and the next stage's group as they go on to it. no
 * stage_group has the value of one of these */
enum stage_step {
    STAGE_STEP_RESET = -2, /* the pads asked to quit */
    STAGE_STEP_QUIT = -1, /* to the menus, or the next demo */
    STAGE_STEP_STAFF_ROLL = 10 /* the story is over */
};

/* how many particles the stage's effects share */
#define PARTICLE_COUNT 512

/* the time the boss trial took on one boss */
struct trial_time {
    Uint32 minutes;
    Uint32 seconds;
    Uint32 frames;
};
#define TRIAL_TIMES 5 /* Sonic's five bosses, the most a character has */

/* the mirrors a mirror_list holds, and the quads each mirror holds */
#define MIRRORS 4
#define MIRROR_QUADS 32

/* a quad of a mirror, placed in the world where the mirror's task
 * stands */
struct mirror_quad {
    NJS_VECTOR normal;
    NJS_POINT3 corners[4]; /* in order round its edge */
};

/* a mirror: its model's quads */
struct mirror {
    Sint32 count;
    struct mirror_quad quads[MIRROR_QUADS];
};

/* the mirrors mirror_add collects. count counts every call since
 * mirror_clear, and can pass MIRRORS */
struct mirror_list {
    Sint32 count;
    struct mirror mirrors[MIRRORS];
};

/* SHC lays out bss in the order of each name's first declaration. so this
 * header declares every bss global stage.c defines, those only stage.c
 * uses too, in the order the image holds them */

/* -1 from the start of a clear's event, and from any event's end until
 * play.c's event_end_step sets it back to 0. that comes a step after
 * event_end_step sets draw_off FALSE. it's 0 otherwise. while it's -1, a
 * clear leaves the act without a fade, as does a field's STAGE_END_GO_TO */
extern int event_ending;
/* what the stage's step does each frame: an enum stage_mode, which
 * stage.c defines */
extern Sint16 stage_mode;
extern Sint16 stage_previous;
extern Sint16 act_previous;
extern Sint16 stage_current;
extern Sint16 act_current;
/* the stage and act stage_load loaded: a change of act in place keeps them */
extern Sint16 stage_loaded;
extern Sint16 act_loaded;
extern Sint16 stage_next;
extern Sint16 act_next;
extern Sint16 entrance_next;
extern Uint16 character_current;
/* the misses, retries and restarts stage_step_action has counted since the
 * stage loaded, but for the cart's restarts. one that ends a trial counts
 * too. stage_step_field counts none */
extern Sint16 stage_retries;
extern Sint16 pause_choice; /* the pause menu's choice: enum pause_choice */
/* -1 while x and y, held together, hide the pause menu, and 0 otherwise */
extern Sint16 pause_hidden;
extern Sint16 continue_choice; /* 0 continues, 1 quits */
extern Sint16 continue_timer; /* frames until the continue screen quits */
/* the particles the stage's effects share. they're declared only after
 * effect/particle.h, which this header can't include. included here, it
 * stops SHC at a later include in some modules, such as adv03's people.c,
 * with 2500 (E) Illegal token */
#ifdef EFFECT_PARTICLE_H
extern struct particle stage_particles[PARTICLE_COUNT];
#endif
extern int dat_8c75b734;
/* the task that runs the stage module's step, or NULL in a stage without
 * one */
extern struct task *stage_task;

/* how many records collision_items and collision_enemies hold */
extern Uint16 collision_item_count;
extern Uint16 collision_enemy_count;
/* below 0 hides the collision's debug drawing and warnings. every start
 * of play sets it to -1, and no code sets it back */
extern Sint8 debug_hidden;
/* below 0 hides the hud and all else drawn over the game. in a stage,
 * each step sets it: -1 while an event is in control, and 0 otherwise */
extern Sint8 hud_hidden;
extern Sint8 lives_shown; /* below 0 hides the lives */
extern Sint8 rings_shown; /* below 0 hides the rings */
extern Sint8 lives;
extern Sint8 time_minutes;
extern Sint8 time_seconds;
extern Sint8 time_frames;
extern Sint16 rings;
/* the game's play time, in steps: game_mode_step adds frame_step_count to
 * it each frame, in every game mode. the save keeps it */
extern int play_time;
/* the calls of time_step since time_reset */
extern int time_steps;
extern int score;
/* points to count in at the end of the act */
extern int bonus_points;
/* how many of trial_times the boss trial has filled so far. a race for
 * mission A sets it to 1 */
extern int trial_count;
/* the share of sky chase's targets hit, in percent */
extern Float shoot_hit_rate;
/* the hints Knuckles has taken in the act */
extern int hints_used;
extern struct trial_time trial_times[TRIAL_TIMES];
extern struct mirror_list mirrors;
/* scratch for the sums of objects and the race */
extern NJS_VECTOR scratch_vector;
extern NJS_VECTOR scratch_vector2;
extern NJS_VECTOR scratch_vector3;
extern Uint8 dat_8c75d5dc[12];
extern NJS_POINT3 scratch_point;
extern NJS_POINT3 scratch_point2;
extern NJS_MATRIX scratch_matrix;
extern Uint8 dat_8c75d640[192];
/* a segment query the mountain's spiders cast at the world. it's declared
 * only after collision/polygon.h, which defines struct segment_query */
#ifdef COLLISION_POLYGON_H
extern struct segment_query scratch_segment_query;
#endif
/* the way gravity pulls, a unit vector: straight down unless a stage turns
 * it */
extern NJS_VECTOR world_gravity;
/* gravity's tilt about x and about z, which a player stands upright to */
extern Angle world_gravity_ang_x;
extern Angle world_gravity_ang_z;
/* the story record of the character being played, or NULL */
extern struct story_record *player_record;
extern int dat_8c75d758;
/* nonzero lets the pads' debug controls work. the title sets it to 1 when
 * start is pressed with x held, or when x and y are held together.
 * boot_new_game clears it */
extern Sint8 debug_unlocked;

void pause_enable(void);
void pause_disable(void);
/* start a new game as Sonic, from the memory card's saves if it has them.
 * it clears debug_unlocked and seeds rand */
void boot_new_game(void);
enum stage_group stage_group(void);
/* ready stage_current to run from its title card, and return its group */
enum stage_group stage_begin(void);
/* run a frame of the current stage: stage_step_action for a stage of
 * STAGE_GROUP_ACTION, and stage_step_field for one of STAGE_GROUP_FIELD.
 * each returns 0 while the stage runs, then a stage_group or a
 * STAGE_STEP */
Sint16 stage_step_action(void);
Sint16 stage_step_field(void);
/* record end, an enum stage_end, for the stage's step to act on. a clear
 * whose event hasn't played starts the event first, and ends the act once
 * the event is over */
void stage_end(Sint16 end);
/* play the effect se, or request the track music, on the fifth step of a
 * task made for it; nothing if task_create fails */
void sound_play_later(int se);
void music_play_later(int music);
/* the player has reached the act's end: disable player 0's pad and the
 * pause, stop the time, and start the character's clear. Sonic and E-102 start
 * theirs once on the ground, as do Tails after a won race and Amy from
 * STAGE_CHAOS0 on */
void stage_clear(void);

/* make character the one played. set_character also swaps in its
 * textures, motions and lives, and calls chao_reset. set_character_number
 * sets the number alone */
void set_character(Sint16 character);
void set_character_number(Sint16 character);
int get_character(void);

int stage_get_retries(void);
/* the current stage and act, and the previous, as STAGE_ACT packs them */
Uint16 stage_get_stage_act(void);
Uint16 stage_get_previous_stage_act(void);
/* set the current stage and act, keeping the old as previous. it loads
 * nothing and starts no act */
void stage_set_stage_act(Uint8 stage, Uint8 act);
/* end the act with STAGE_END_GO_TO, toward stage and act */
void stage_fade_to(Uint8 stage, Uint8 act);
/* end the act as the pause menu's restart does */
void stage_restart(void);
/* set the next stage and act. stage_get_next_stage_act gives them as
 * STAGE_ACT packs them, and stage_go_to_next makes them current, keeping the
 * current as previous. none of the three loads anything */
void stage_set_next_stage_act(Uint8 stage, Uint8 act);
Uint16 stage_get_next_stage_act(void);
void stage_go_to_next(void);
/* change act in place to act_current + by. it releases this act's
 * textures and land and loads the new act's, with download_end before and
 * download_start after. a new act at or above STAGE_ACTS becomes 0, and a
 * negative one goes unchecked. the act must be in the stage module already
 * loaded. outside the mystic ruins and the past, light_load_act loads the
 * act's lights, which turns the light editor off. story_steps returns to 0,
 * so story_step waits out two calls again */
void stage_change_act(Sint16 by);
/* the entrance the next stage starts at */
void stage_set_entrance(Sint8 entrance);
Sint8 stage_get_entrance(void);
/* place work's player and turn it, at the first of these places it has:
 * - the restart point kept for this act;
 * - the return point a quit from a level chose, which this then forgets;
 * - in a field, the place the field's module chooses;
 * - its character's start for the stage and act, or else the origin.
 *
 * the place then goes into the player's history. in a level, player 0's
 * place becomes the restart point. the character must be one of the six
 * playable.
 *
 * away from a restart point, the time resets while act_current is still
 * act_loaded, and E-102's time is set to three minutes. a time of E-102's
 * under a minute then becomes one minute, restart point or not */
void stage_place_player(struct task_work *work);
/* set the next stage, act and entrance for leaving the current stage after
 * a clear. the cart leads to the act of station square that twinkle park
 * leads back to. any other stage leads where the character's exits say,
 * which the story record, where there is one, keeps before the save is
 * written. movie_request takes the exit's movie, or 0 once the character's
 * story has ended. an exit first in the list leaves movie_request as it
 * was. the exits must list the stage: the search runs past the list's
 * end */
void stage_set_next_exit(void);
Bool stage_paused(void);
Bool stage_playing(void);
#ifdef GCC
/* TRUE in play, and in the fade from black that opens it, where play runs */
Bool stage_in_play(void);
/* TRUE zeroes the limit of stage_run_action's frame: a frame whose second
 * step ends with it TRUE takes one more step. a cone moving over a floor
 * sets it, and each frame starts with it FALSE */
extern Bool stage_limit_cleared;
#endif

#endif
