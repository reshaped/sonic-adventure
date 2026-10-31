/* Minigame Mogura, the Hedgehog Hammer's machine. for Amy, targets rise
 * from its eight holes to be hit. a board above it shows the time, the
 * score and the day's best. after a round, the score screen tallies the
 * hits. the game over screen then offers to restart the stage or to end the
 * act */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/fade.h"
#include "draw/helpers.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/explosion.h"
#include "effect/flame.h"
#include "enemy/enemy.h"
#include "hud/hud.h"
#include "main.h"
#include "object/placement.h"
#include "path/curve.h"
#include "player/player.h"
#include "progress/progress.h"
#include "save/card.h"
#include "save/save.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg00/mole.h"
#include "stg00/mole_textures.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texlists.h"
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 2
#define TEXT_3_LENGTH 6
#define TEXT_4_LENGTH 3
#define TEXT_5_LENGTH 5
#define TEXT_6_LENGTH 4
#define TEXT_7_LENGTH 5
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 15
#define TEXT_10_LENGTH 20
#define TEXT_11_LENGTH 15
#define TEXT_12_LENGTH 11
#include "stg00/mole/text.inc"

/* the machine, and each kind of target's object, motion, shape motion and
 * the models it breaks into. they draw with mole_texlist, MOGURATATAKI's
 * textures */
extern NJS_OBJECT mole_machine_object;
extern NJS_OBJECT mole_500_object;
extern NJS_MOTION mole_500_motion;
extern NJS_MOTION mole_500_shape;
extern NJS_OBJECT mole_100_object;
extern NJS_MOTION mole_100_motion;
extern NJS_MOTION mole_100_shape;
extern NJS_OBJECT mole_eggman_object;
extern NJS_MOTION mole_eggman_motion;
extern NJS_MOTION mole_eggman_shape;
extern NJS_MODEL mole_eggman_piece_1_model;
extern NJS_MODEL mole_eggman_piece_2_model;
extern NJS_MODEL mole_eggman_piece_3_model;
extern NJS_MODEL mole_eggman_piece_4_model;
extern NJS_MODEL mole_eggman_piece_5_model;
extern NJS_MODEL mole_eggman_piece_6_model;
extern NJS_MODEL mole_eggman_piece_7_model;
extern NJS_MODEL mole_100_piece_0_model;
extern NJS_MODEL mole_100_piece_1_model;
extern NJS_MODEL mole_100_piece_2_model;
extern NJS_MODEL mole_100_piece_3_model;
extern NJS_MODEL mole_100_piece_4_model;
extern NJS_MODEL mole_100_piece_5_model;
extern NJS_MODEL mole_100_piece_6_model;
extern NJS_MODEL mole_100_piece_7_model;
extern NJS_MODEL mole_100_piece_8_model;
extern NJS_MODEL mole_500_piece_0_model;
extern NJS_MODEL mole_500_piece_1_model;
extern NJS_MODEL mole_500_piece_2_model;
extern NJS_MODEL mole_500_piece_3_model;
extern NJS_MODEL mole_500_piece_4_model;
extern NJS_MODEL mole_500_piece_5_model;
extern NJS_MODEL mole_500_piece_6_model;
extern NJS_MODEL mole_500_piece_7_model;
extern NJS_MODEL mole_500_piece_8_model;

/* the machine's modes, in work->mode */
enum mole_mode {
    MOLE_INIT = 0,
    MOLE_WAIT = 1, /* until Amy starts a round */
    MOLE_READY = 2, /* MOLE_READY_STEPS, then the round starts */
    MOLE_PLAY = 3,
    MOLE_END = 4, /* the board's messages after a round */
    MOLE_RESULTS = 5, /* the score screen, then the game over */
    MOLE_STILL = 7, /* drawn only */
    MOLE_GONE = 8
};

enum mole_target_mode {
    TARGET_NEW, /* picks a kind and a wait */
    TARGET_WAIT, /* in its hole */
    TARGET_RISE,
    TARGET_UP, /* bobbing at the top */
    TARGET_SINK,
    TARGET_HIT, /* flames while its motion plays out, then breaks */
    TARGET_DROP
};

/* what a hit on each kind of target scores */
#define POINTS_500 500
#define POINTS_100 100
#define POINTS_EGGMAN (-200)

/* the messages the board scrolls after a round. mole_draw_board has a screen
 * for each, but mole_end shows only GAME OVER and HISCORE GET */
enum mole_message {
    MESSAGE_NONE = -1,
    MESSAGE_GAME_OVER,
    MESSAGE_CONGRATULATIONS,
    MESSAGE_ITEM, /* the warrior feather, or the long hammer */
    MESSAGE_HISCORE
};

/* mole_end's steps after a round, in end_step. the step after END_HISCORE
 * starts the results */
enum mole_end_step {
    END_GAME_OVER, /* GAME OVER, unless the score beats mole_best */
    END_BLANK_1, /* no message */
    END_BLANK_2, /* no message */
    END_HISCORE /* HISCORE GET for a score that beats mole_best */
};

/* what the board shows, in mole_shown and last_screen */
enum mole_screen {
    SCREEN_BEST, /* the day's best */
    SCREEN_START,
    SCREEN_COUNT, /* the time and the score */
    SCREEN_GAME_OVER,
    SCREEN_CONGRATULATIONS,
    SCREEN_ITEM,
    SCREEN_HISCORE,
    SCREEN_BLANK
};

/* mole_results' steps after the board's messages, in result */
enum mole_result {
    RESULT_SCORE = 0, /* save the score, and start the score screen */
    RESULT_TALLY = 1, /* until mole_score_done, then load the game over */
    RESULT_LOAD = 2, /* a wait, then the game over's music */
    RESULT_CHOOSE = 3, /* the game over screen, until A takes the choice */
    RESULT_BLACK = 6 /* black, once the choice is taken */
};

/* the game over's choices, in mole_choice */
enum game_over_choice { CHOICE_CONTINUE, CHOICE_QUIT };

/* the board's lines of text, which index mole_texts */
enum mole_line {
    LINE_BEST, /* TODAYS HI SCORE */
    LINE_SCORE, /* the best, or the score, in big digits */
    LINE_PT,
    LINE_EGGMAN,
    LINE_TIME,
    LINE_COUNT,
    LINE_TIME_LEFT,
    LINE_AMY,
    LINE_START,
    LINE_UNUSED,
    MOLE_LINES
};

/* the panel's flash after a hit that costs points, in flash. it counts down
 * from FLASH_RED, at which the panel turns red. at FLASH_BACK the panel
 * takes its color back */
enum mole_flash { FLASH_NONE, FLASH_BACK, FLASH_RED };

#define MOLE_TARGETS 8

/* the time_left a round starts with. mole_play takes one from it each step,
 * and one more for each target at TARGET_NEW. once it is 0 or less, no target
 * rises. the round ends once it falls below -MOLE_TIME_AFTER */
#define MOLE_TIME 2000
#define MOLE_TIME_AFTER 100
/* Eggman's best, where mole_best starts. while mole_best equals it, the
 * board names EGGMAN beside the best, and AMY once it doesn't */
#define MOLE_EGGMAN_BEST 2000
/* the steps MOLE_READY waits before the round starts, and the steps each
 * message after the round holds */
#define MOLE_READY_STEPS 10
#define MOLE_MESSAGE_STEPS 300
/* the steps the board's text waits once its screen changes */
#define MOLE_TEXT_WAIT 30
/* RESULT_LOAD moves on once result_steps, which it counts up each step,
 * passes RESULT_LOAD_WAIT */
#define RESULT_LOAD_WAIT 3

/* where the board sits, from the machine's position */
#define BOARD_X 0.0F
#define BOARD_Y 40.0F
#define BOARD_Z 0.0F

/* a target's height in its hole. a target at it or below isn't drawn */
#define TARGET_HIDDEN (-4.0F)
/* a target's height at the top of its rise */
#define TARGET_TOP 10.0F
/* how far out from the machine's middle the holes are */
#define HOLE_REACH 20.0F
/* the scale of a target's model across x and z */
#define TARGET_SCALE 1.5F

/* the radius of the sphere round the machine that must be in view to draw
 * it */
#define MOLE_VIEW 100.0F
/* the radius of the machine's collision cylinder. mole_wait also takes it as
 * the radius of a sphere at the machine's position. Amy's sphere must meet
 * that one to start a round */
#define MOLE_RADIUS 42.5F
/* the radius of Amy's sphere for that test */
#define AMY_RADIUS 3.0F

/* a new target waits HOLE_WAIT steps in its hole, and a random number fewer
 * than HOLE_WAIT_SPAN more. a risen one waits fewer than TOP_WAIT_SPAN at the
 * top */
#define HOLE_WAIT 60
#define HOLE_WAIT_SPAN 600.0F
#define TOP_WAIT_SPAN 60.0F
/* a target rises and sinks TARGET_SPEED a step, and a random part of
 * TARGET_SPEED_SPAN more */
#define TARGET_SPEED 1.2F
#define TARGET_SPEED_SPAN 0.1F
/* at the top, a target's height swings by its speed times the cosine of its
 * bob. the bob turns BOB_TURN a step, and the speed keeps BOB_KEEP of itself
 * each step */
#define BOB_KEEP 0.9F
#define BOB_TURN NJM_DEG_ANG(20)
/* a hit target's motion runs from HIT_FRAME, HIT_FRAME_STEP a step. a flame
 * rises from it every FLAME_STEPS steps until the motion ends */
#define HIT_FRAME 30.0F
#define HIT_FRAME_STEP 2.0F
#define FLAME_STEPS 10

/* the panel's half width under the best, the start and the count */
#define PANEL_NARROW 24
/* the panel's half width under a message */
#define PANEL_WIDE 60
/* how far the panel sits below the board's position */
#define PANEL_DROP 5.0F
/* the scale the board's sprites draw at */
#define BOARD_SCALE 0.2F
/* how far a message scrolls each draw, and where it starts again */
#define SCROLL_STEP 0.5F
#define SCROLL_END 100.0F
/* the places the best, the time and the score take */
#define SCORE_DIGITS 4

#define SE_MOLE_START 575
#define SE_MOLE_POP_500 573
#define SE_MOLE_POP 572
#define SE_MOLE_HIT 571
#define SE_MOLE_HIT_EGGMAN 574
/* njDrawPolygon2D's attributes for the quads: see-through, see-through with
 * a texture, and solid with a texture. the game over's banner draws solid */
#define SEE_THROUGH (NJD_DRAW_CONNECTED | NJD_FILL | NJD_TRANSPARENT)
#define SEE_THROUGH_TEXTURED (NJD_USE_TEXTURE | SEE_THROUGH)
#define SOLID_TEXTURED (NJD_USE_TEXTURE | NJD_DRAW_CONNECTED | NJD_FILL)

/* a target in one of the machine's holes */
struct mole_target {
    Sint32 mode; /* an enum mole_target_mode */
    Sint32 wait; /* steps until it rises or sinks, or steps since its hit */
    /* what its next hit scores. a hit adds it to the score, then zeroes it */
    Sint32 points;
    Sint8 kind; /* an enum mole_kind */
    Angle bob; /* its bob's angle, which turns BOB_TURN a step at the top */
    /* the distance path_axis_position takes along mole_rise_frames or
     * mole_sink_frames, in frames */
    Float distance;
    Float height; /* above the machine's position */
    Float speed; /* its rise or sink a step, and at the top how far it bobs */
    Float frame; /* the frame its motion draws at */
    struct task *solid; /* a child task whose collision pushes */
    struct task *sensor; /* a child task the hammer can hit */
};

/* a round of the game, and the board over the machine */
struct mole_game {
    Sint32 timer; /* steps left in MOLE_READY, or on a message */
    Sint32 time_left; /* falls from MOLE_TIME, past 0 at the round's end */
    Sint32 score;
    struct mole_target targets[MOLE_TARGETS];
    /* where the board draws: BOARD_X, BOARD_Y and BOARD_Z from the machine's
     * position, at the machine's angles */
    NJS_POINT3 pos;
    struct angles ang;
    Sint32 text_wait; /* steps before the board's text draws */
    Sint8 flash; /* an enum mole_flash */
    Sint8 end_step; /* mole_end's next step, an enum mole_end_step */
    Sint8 message; /* an enum mole_message */
    Sint8 last_screen; /* the screen this game's board drew last */
    Float scroll; /* how far the message has scrolled, up to SCROLL_END */
    Sint8 result; /* mole_results' step, an enum mole_result */
    Sint32 result_steps; /* the steps RESULT_LOAD has counted */
};

/* the machine task's extra: a pointer to its game, on the heap */
struct mole_extra {
    struct mole_game *game;
};

#define EXTRA(task) ((struct mole_extra *)(task)->extra)

/* the models a kind of target breaks into, NULL-ended for enemy_break */
struct mole_break {
    NJS_MODEL **pieces;
    Uint32 unk_04;
};

/* where a hole is, from the machine's middle */
struct mole_hole {
    Float x;
    Float z;
};

/* how a kind of target looks */
struct mole_look {
    NJS_OBJECT *object;
    NJS_MOTION *motion;
    NJS_MOTION *shape; /* its shape motion */
    Uint32 unk_0c;
};

/* where a line of the board's text goes */
struct mole_text {
    /* where the first letter goes */
    Float x;
    Float y;
    /* a letter's scale */
    Float sx;
    Float sy;
    /* the step from each letter to the next */
    Float dx;
    Float dy;
};

/* what the board's panel fades and resizes toward */
struct mole_panel_look {
    NJS_COLOR color;
    int width; /* half the panel's width */
};

static void mole_reset_hits(void);
static void mole_destroy(struct task *task);
static void mole_draw_game(struct task *task);
static struct mole_game *mole_alloc(void);
static void mole_target_step(struct task *task);
static void mole_wait(struct task *task, struct task_work *work);
static void mole_ready(struct task *task, struct task_work *work);
static void mole_end(struct task *task, struct task_work *work);
static void mole_play(struct task *task, struct task_work *work);
static void mole_place_targets(struct task *task, struct task_work *work);
static void mole_score_hits(struct task *task, struct task_work *work);
static void mole_draw_board(struct task *task, struct task_work *work);
static void mole_draw_sprite(NJS_SPRITE *sprite, Int glyph, Uint32 attr);
static void approach_byte(Uint8 *value, Uint8 target);
static void approach_float(Float *value, Float target);
static void mole_draw_panel(struct mole_panel_look *look);
static void mole_draw_number(struct mole_text *text, int value, int digits);
static void mole_draw_text(struct mole_text *text, char *string);
static void mole_draw_scroll(char *string, Float progress);
static void mole_results(struct task *task, struct task_work *work);
static void mole_save_score(void);
static void mole_load_best(void);
static void mole_draw_black(void);
static void mole_draw_game_over(int choice);

/* the game whose machine is being stepped or drawn. mole_step and
 * mole_draw_game set game from the task's extra first */
static struct mole_game *game;
/* the screen mole_draw_board picked last, for any machine's board. a
 * screen's text draws only when the draw before picked it too */
static Sint32 mole_shown;
/* the panel's four corner colors. mole_draw_panel moves the first, and
 * copies it to the other three */
static NJS_COLOR mole_panel_colors[4];
/* the first corner's color, which the panel takes back after a red flash */
/* @hack type: volatile, which makes each of mole_draw_panel's flash
 * cases load the color's address, at 0x0c902da0 and 0x0c902da8; without it,
 * SHC loads it once ahead of the switch, and 24 of 162 words differ */
static volatile NJS_COLOR mole_saved_color;
/* TRUE once the score screen has ended, as mole_score_done reports.
 * RESULT_SCORE clears it when it starts the screen */
static Bool mole_score_over;
/* the game over's glow, which mole_draw_game_over steps by 4 each draw. as a
 * Sint8 it wraps, so its magnitude rises to 128 and falls back to 0 */
static Sint8 mole_game_over_phase;

static NJS_MODEL *mole_eggman_pieces[] = {
    &mole_eggman_piece_0_model,
    &mole_eggman_piece_1_model,
    &mole_eggman_piece_2_model,
    &mole_eggman_piece_3_model,
    &mole_eggman_piece_4_model,
    &mole_eggman_piece_5_model,
    &mole_eggman_piece_6_model,
    &mole_eggman_piece_7_model,
    NULL,
};
static NJS_MODEL *mole_100_pieces[] = {
    &mole_100_piece_0_model, &mole_100_piece_1_model,
    &mole_100_piece_2_model, &mole_100_piece_3_model,
    &mole_100_piece_4_model, &mole_100_piece_5_model,
    &mole_100_piece_6_model, &mole_100_piece_7_model,
    &mole_100_piece_8_model, NULL,
};
static NJS_MODEL *mole_500_pieces[] = {
    &mole_500_piece_0_model, &mole_500_piece_1_model,
    &mole_500_piece_2_model, &mole_500_piece_3_model,
    &mole_500_piece_4_model, &mole_500_piece_5_model,
    &mole_500_piece_6_model, &mole_500_piece_7_model,
    &mole_500_piece_8_model, NULL,
};
static struct mole_break mole_breaks[MOLE_KINDS] = {
    {mole_500_pieces, 0},
    {mole_100_pieces, 0},
    {mole_eggman_pieces, 0},
};

/* the best the board shows under TODAYS HI SCORE. mole_load_best and each
 * new best raise it */
static Sint32 mole_best = MOLE_EGGMAN_BEST;

/* the machine's collision */
static struct collision_shape mole_machine_shape[1] = {
    {SHAPE_CYLINDER,
     SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS) | PUSH_ALL,
     {0.0F, 0.0F, 0.0F},
     MOLE_RADIUS,
     12.8F,
     0.0F,
     {0},
     0,
     NULL},
};
/* a target's solid's collision, then its sensor's */
static struct collision_shape mole_target_shapes[2] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_FIRM(PUSH_ENEMIES | PUSH_OTHERS) | PUSH_ALL,
     {0.0F, 0.0F, 0.0F},
     4.5F,
     10.0F,
     0.0F,
     {0},
     0,
     NULL},
    {SHAPE_CYLINDER | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS,
     PUSH_ALL,
     {0.0F, 0.0F, 0.0F},
     4.5F,
     10.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* where each hole's target draws. mole_place_targets puts the same holes'
 * collision by angle */
static struct mole_hole mole_holes[MOLE_TARGETS] = {
    {0.0F, -20.0F}, {14.0F, -14.0F}, {20.0F, 0.0F},  {14.0F, 14.0F},
    {0.0F, 20.0F},  {-14.0F, 14.0F}, {-20.0F, 0.0F}, {-14.0F, -14.0F},
};

/* the stops of a target's motion frame, for path_axis_position. as the
 * target rises and bobs, its frame runs from 1 to 30 and bounces back to 30
 * four times. as it sinks, the frame runs back to 1 */
static Float mole_rise_frames[] = {
    1.0F, 30.0F, 15.0F, 30.0F, 20.0F, 30.0F, 25.0F, 30.0F, 28.0F, 30.0F, -1.0F,
};
static Float mole_sink_frames[] = {30.0F, 1.0F, -1.0F};

static struct mole_look mole_looks[MOLE_KINDS] = {
    {&mole_500_object, &mole_500_motion, &mole_500_shape, 0},
    {&mole_100_object, &mole_100_motion, &mole_100_shape, 0},
    {&mole_eggman_object, &mole_eggman_motion, &mole_eggman_shape, 0},
};

/* where each of the board's lines goes, by enum mole_line */
static struct mole_text mole_texts[MOLE_LINES] = {
#include "stg00/mole/mole_texts.inc"
};

#define FONT_LETTERS 36

/* the board's font: 0 to 9, then A to Z, in mole_texlist's textures */
static NJS_TEXANIM mole_font[FONT_LETTERS] = {
#include "stg00/mole/mole_font.inc"
};

/* @hack type: a letter of the text read as a Uint8, so SHC zero-extends
 * each load with EXTU.B, at 0x0c903136 and 0x0c903258; without it, summed
 * over mole_draw_text and mole_draw_scroll, 157 of 314 words differ */
#define LETTER(string) (*(Uint8 *)(string))
/* the glyph of a capital letter, after the ten digits */
#define FONT_CAPITAL(letter) ((letter) - 'A' + 10)

/* the board's panel, a quad behind its text. mole_draw_panel sets the
 * corners' x to its half width */
static NJS_POINT3 mole_panel_points[4] = {
#include "stg00/mole/mole_panel_points.inc"
};
static NJS_POINT3COL mole_panel = {
#include "stg00/mole/mole_panel.inc"
};

/* the texanim a scrolling message draws each letter with. mole_draw_scroll
 * sets its texture for each letter, and the part of it that shows */
/* @hack type: volatile, which has mole_draw_scroll load the address of
 * each member it stores to, as texid's at 0x0c90326c; without it, 238 of 251
 * words differ */
static volatile NJS_TEXANIM mole_scroll_texanim = {
#include "stg00/mole/mole_scroll_texanim.inc"
};

/* the choice the game over screen marks, an enum game_over_choice */
static Sint32 mole_choice = CHOICE_CONTINUE;

/* start the round's tally in mole_hits, which the score screen reads: no
 * hits yet, and each kind's points */
static void mole_reset_hits(void)
{
    mole_hits[KIND_500].count = 0;
    mole_hits[KIND_500].points = POINTS_500;
    mole_hits[KIND_100].count = 0;
    mole_hits[KIND_100].points = POINTS_100;
    mole_hits[KIND_EGGMAN].count = 0;
    mole_hits[KIND_EGGMAN].points = POINTS_EGGMAN;
}

void mole_step(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    game = EXTRA(task)->game;
    if (placement_editing(task)) {
        if (work->mode == MOLE_INIT) {
            work->mode++;
            EXTRA(task)->game = mole_alloc();
            task->destructor = mole_destroy;
            task->draw = mole_draw_game;
            game = EXTRA(task)->game;
            game->pos.x = work->pos.x + BOARD_X;
            game->pos.y = work->pos.y + BOARD_Y;
            game->pos.z = work->pos.z + BOARD_Z;
            game->ang.x = work->ang.x;
            game->ang.y = work->ang.y;
            game->ang.z = work->ang.z;
            game->message = MESSAGE_NONE;
        }
        game = EXTRA(task)->game;
        mole_draw_game(task);
    } else {
        switch (work->mode) {
        case MOLE_INIT:
            work->mode = MOLE_WAIT;
            EXTRA(task)->game = mole_alloc();
            game = EXTRA(task)->game;
            task->destructor = mole_destroy;
            task->draw = mole_draw_game;
            collision_init(task, mole_machine_shape, COLLISION_OBJECT, 1, 1);
            game->pos.x = work->pos.x + BOARD_X;
            game->pos.y = work->pos.y + BOARD_Y;
            game->pos.z = work->pos.z + BOARD_Z;
            game->ang.x = work->ang.x;
            game->ang.y = work->ang.y;
            game->ang.z = work->ang.z;
            game->message = MESSAGE_NONE;
            for (i = 0; i < MOLE_TARGETS; i++) {
                game->targets[i].height = TARGET_HIDDEN;
                if (get_character() == CHARACTER_AMY) {
                    game->targets[i].solid =
                        task_create_child(TASK_WORK, mole_target_step, task);
                    collision_init(game->targets[i].solid,
                                   &mole_target_shapes[0], COLLISION_OBJECT, 1,
                                   1);
                    game->targets[i].sensor =
                        task_create_child(TASK_WORK, mole_target_step, task);
                    collision_init(game->targets[i].sensor,
                                   &mole_target_shapes[1], COLLISION_ITEM, 1,
                                   1);
                }
            }
            mole_load_best();
            break;
        case MOLE_WAIT:
            mole_draw_game(task);
            mole_wait(task, work);
            mole_place_targets(task, work);
            break;
        case MOLE_READY:
            mole_draw_game(task);
            mole_ready(task, work);
            mole_place_targets(task, work);
            break;
        case MOLE_PLAY:
            mole_draw_game(task);
            mole_play(task, work);
            mole_score_hits(task, work);
            mole_place_targets(task, work);
            break;
        case MOLE_END:
            mole_draw_game(task);
            mole_end(task, work);
            mole_place_targets(task, work);
            break;
        case MOLE_RESULTS:
            mole_draw_game(task);
            mole_results(task, work);
            mole_place_targets(task, work);
            break;
        case MOLE_STILL:
            mole_draw_game(task);
            break;
        /* @hack redundant: case MOLE_GONE, which shares the default's code,
         * keeps the image's CMP/EQ #8 at 0x0c900f76; without it, 408 of 515
         * words differ */
        case MOLE_GONE:
        default:
            placement_kill(task);
            break;
        }
    }
    if (game->text_wait > 0) {
        game->text_wait--;
    }
}

static void mole_destroy(struct task *task)
{
    if (EXTRA(task)->game != NULL) {
        heap_free(EXTRA(task)->game);
        EXTRA(task)->game = NULL;
    }
}

/* draw the machine, each target above TARGET_HIDDEN, and the board, while
 * the machine is in view */
static void mole_draw_game(struct task *task)
{
    struct task_work *work = task->work;
    int i;
    Angle ang;

    if (draw_in_view(work->pos.x, work->pos.y, work->pos.z, MOLE_VIEW)) {
        game = EXTRA(task)->game;
        njSetTexture(&mole_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_tree(&mole_machine_object);
        for (i = 0; i < MOLE_TARGETS; i++) {
            if (game->targets[i].height > TARGET_HIDDEN) {
                njPushMatrix(NULL);
                njTranslate(NULL, mole_holes[i].x, game->targets[i].height,
                            mole_holes[i].z);
                njScale(NULL, TARGET_SCALE, 1.0F, TARGET_SCALE);
                njEasyDrawShapeMotion(mole_looks[game->targets[i].kind].object,
                                      mole_looks[game->targets[i].kind].motion,
                                      mole_looks[game->targets[i].kind].shape,
                                      game->targets[i].frame);
                njPopMatrix(1);
            }
        }
        njPopMatrix(1);
        mole_draw_board(task, work);
    }
}

/* a game from the heap, zeroed */
static struct mole_game *mole_alloc(void)
{
    struct mole_game *record;
    Uint8 *bytes;
    int i;

    /* @hack redundant: an empty test of the record, which writes TST R0,R0 at
     * 0x0c901712 with no branch after it; without it, 19 of 27 words differ */
    if ((record = heap_alloc(sizeof(*record))) != NULL) {
    }
    bytes = (Uint8 *)record;
    /* NOLINTNEXTLINE(clang-diagnostic-sign-compare) */
    for (i = 0; i < sizeof(*record); i++) {
        bytes[i] = 0;
    }
    return record;
}

/* the step of a target's solid and sensor, which only carry collision.
 * mole_place_targets places and registers them */
static void mole_target_step(struct task *task)
{
}

/* start a round when Amy presses A at the machine */
static void mole_wait(struct task *task, struct task_work *work)
{
    int i;

    if (get_character() == CHARACTER_AMY &&
        (pad_records[0].press & PDD_DGT_TA) &&
        spheres_overlap(&player_work[0]->pos, &work->pos, AMY_RADIUS,
                        MOLE_RADIUS)) {
        for (i = 0; i < MOLE_TARGETS; i++) {
            game->targets[i].mode = TARGET_NEW;
        }
        game->time_left = MOLE_TIME;
        game->score = 0;
        work->mode = MOLE_READY;
        game->timer = MOLE_READY_STEPS;
        sound_play(SE_SWITCH, 0, 0, 0);
    }
}

static void mole_ready(struct task *task, struct task_work *work)
{
    if (game->timer-- <= 0) {
        work->mode = MOLE_PLAY;
        sound_play(SE_MOLE_START, 0, 0, 0);
        mole_reset_hits();
    }
}

/* step through a round's end, one end_step at a time. each message holds
 * MOLE_MESSAGE_STEPS, and a step with none moves on at once. a score that
 * doesn't beat mole_best shows GAME OVER. one that does becomes mole_best,
 * and shows HISCORE GET */
static void mole_end(struct task *task, struct task_work *work)
{
    if (game->timer > 0) {
        game->timer--;
        return;
    }
    game->message = MESSAGE_NONE;
    switch (game->end_step) {
    case END_GAME_OVER:
        if (game->score <= mole_best) {
            game->timer = MOLE_MESSAGE_STEPS;
            game->message = MESSAGE_GAME_OVER;
        }
        break;
    case END_BLANK_1:
        break;
    case END_BLANK_2:
        break;
    case END_HISCORE:
        if (game->score > mole_best) {
            mole_best = game->score;
            game->timer = MOLE_MESSAGE_STEPS;
            game->message = MESSAGE_HISCORE;
        }
        break;
    default:
        work->mode = MOLE_RESULTS;
        game->result = RESULT_SCORE;
        game->message = MESSAGE_NONE;
        break;
    }
    game->end_step++;
    game->scroll = 0.0F;
}

/* run the round. time_left falls by one each step, and by one more for
 * each target at TARGET_NEW. targets rise from their holes at random until
 * time_left reaches 0. the round ends once time_left falls below
 * -MOLE_TIME_AFTER */
static void mole_play(struct task *task, struct task_work *work)
{
    int i;

    if (game->time_left < -MOLE_TIME_AFTER) {
        work->mode = MOLE_END;
        game->timer = 0;
        game->end_step = END_GAME_OVER;
        /* @hack redundant: a return where control returns anyway, which ends
         * the branch in BRA and a NOP at 0x0c901972; without it, the store at
         * 0x0c901970 fills the delay slot, and 120 of 732 words differ */
        return;
    } else {
        game->time_left--;
        for (i = 0; i < MOLE_TARGETS; i++) {
            switch (game->targets[i].mode) {
            case TARGET_NEW:
                game->time_left--;
                game->targets[i].wait =
                    (int)(rand() * (HOLE_WAIT_SPAN * RAND_SCALE)) + HOLE_WAIT;
                game->targets[i].mode = TARGET_WAIT;
                game->targets[i].kind =
                    rand() * ((Float)MOLE_KINDS * RAND_SCALE);
                game->targets[i].height = TARGET_HIDDEN;
                game->targets[i].frame = 0.0F;
                game->targets[i].distance = 0.0F;
                switch (game->targets[i].kind) {
                case KIND_500:
                    game->targets[i].points = POINTS_500;
                    break;
                case KIND_100:
                    game->targets[i].points = POINTS_100;
                    break;
                case KIND_EGGMAN:
                    game->targets[i].points = POINTS_EGGMAN;
                    break;
                }
                break;
            case TARGET_WAIT:
                if (game->time_left > 0) {
                    game->targets[i].wait--;
                }
                if (game->targets[i].wait <= 0) {
                    game->targets[i].mode = TARGET_RISE;
                    game->targets[i].speed =
                        TARGET_SPEED +
                        (rand() * (TARGET_SPEED_SPAN * RAND_SCALE));
                    switch (game->targets[i].kind) {
                    case KIND_500:
                        sound_play(SE_MOLE_POP_500, 0, 0, 0);
                        break;
                    case KIND_100:
                    case KIND_EGGMAN:
                        sound_play(SE_MOLE_POP, 0, 0, 0);
                        break;
                    }
                }
                break;
            case TARGET_RISE:
                game->targets[i].height += game->targets[i].speed;
                /* @hack spelling: distance moved 1 in the call's argument and 2
                 * after the call, which adds FLDI1's 1 before the call, at
                 * 0x0c901c16, and FR12's 2 after it, at 0x0c901c2e; without
                 * it, as one move of 3, 43 of 732 words differ */
                game->targets[i].frame = path_axis_position(
                    mole_rise_frames, game->targets[i].distance++);
                game->targets[i].distance += 2.0F;
                if (game->targets[i].height >= TARGET_TOP) {
                    game->targets[i].height = TARGET_TOP;
                    game->targets[i].wait =
                        (int)(rand() * (TOP_WAIT_SPAN * RAND_SCALE));
                    game->targets[i].mode = TARGET_UP;
                }
                break;
            case TARGET_UP:
                game->targets[i].speed *= BOB_KEEP;
                game->targets[i].bob += BOB_TURN;
                game->targets[i].height =
                    TARGET_TOP +
                    (game->targets[i].speed * njCos(game->targets[i].bob));
                game->targets[i].wait--;
                /* @hack spelling: distance moved 1 in the call's argument and 2
                 * after the call, at 0x0c901d1a and 0x0c901d2e; without it,
                 * as one move of 3, 41 of 732 words differ */
                game->targets[i].frame = path_axis_position(
                    mole_rise_frames, game->targets[i].distance++);
                game->targets[i].distance += 2.0F;
                if (game->targets[i].wait <= 0) {
                    game->targets[i].mode = TARGET_SINK;
                    game->targets[i].speed =
                        TARGET_SPEED +
                        (rand() * (TARGET_SPEED_SPAN * RAND_SCALE));
                    game->targets[i].distance = 0.0F;
                }
                break;
            case TARGET_SINK:
                game->targets[i].height -= game->targets[i].speed;
                /* @hack spelling: distance moved 1 in the call's argument and 8
                 * after the call, at 0x0c901dd6 and 0x0c901df4; without it,
                 * as one move of 9, 280 of 732 words differ */
                game->targets[i].frame = path_axis_position(
                    mole_sink_frames, game->targets[i].distance++);
                game->targets[i].distance += 8.0F;
                if (game->targets[i].height < TARGET_HIDDEN) {
                    game->targets[i].mode = TARGET_NEW;
                }
                break;
            case TARGET_HIT:
                game->targets[i].frame += HIT_FRAME_STEP;
                if (game->targets[i].frame >
                    mole_looks[game->targets[i].kind].motion->nbFrame) {
                    NJS_POINT3 pos;

                    game->targets[i].frame =
                        mole_looks[game->targets[i].kind].motion->nbFrame -
                        0.1F;
                    game->targets[i].mode = TARGET_NEW;
                    /* enemy_break starts the pieces at work's position, so
                     * the machine's work takes the solid's for the call */
                    pos.x = work->pos.x;
                    pos.y = work->pos.y;
                    pos.z = work->pos.z;
                    work->pos.x = game->targets[i].solid->work->pos.x;
                    work->pos.y = game->targets[i].solid->work->pos.y;
                    work->pos.z = game->targets[i].solid->work->pos.z;
                    enemy_break(work, mole_breaks[game->targets[i].kind].pieces,
                                &mole_texlist);
                    work->pos.x = pos.x;
                    work->pos.y = pos.y;
                    work->pos.z = pos.z;
                } else if (game->targets[i].wait++ % FLAME_STEPS == 0) {
                    NJS_VECTOR speed;
                    NJS_POINT3 pos;

                    speed.x = 0.0F;
                    speed.y = 1.0F;
                    speed.z = 0.0F;
                    pos = game->targets[i].sensor->work->pos;
                    flame_create(&pos, &speed, 1.0F);
                }
                break;
            case TARGET_DROP:
                game->targets[i].height -= game->targets[i].speed;
                if (game->targets[i].height < TARGET_HIDDEN) {
                    game->targets[i].mode = TARGET_NEW;
                }
                break;
            }
        }
    }
}

/* register the machine's collision. for Amy, place each target's solid and
 * sensor at its hole and height, and register theirs. hole i is i - 2
 * eighths of a turn from the machine's x axis, as in mole_holes. the int
 * division is exact, since MOLE_TARGETS divides 360 */
static void mole_place_targets(struct task *task, struct task_work *work)
{
    int i;
    Angle ang;

    collision_register(work);
    if (get_character() == CHARACTER_AMY) {
        for (i = 0; i < MOLE_TARGETS; i++) {
            /* @hack spelling: ang set inside its first use, which keeps the
             * image's 4-byte frame (ADD #-4,R15 at 0x0c902034) and
             * collision_register's address in R10, at 0x0c902036. set in a
             * statement of its own before that use, the frame takes 8 and the
             * address R11, the function runs 544 bytes longer, and 250 of 272
             * words differ */
            /* NOLINTBEGIN(bugprone-integer-division) */
            game->targets[i].solid->work->pos.x =
                work->pos.x +
                (HOLE_REACH *
                 njCos(-work->ang.y + (ang = (360 * (i - 2) / MOLE_TARGETS) *
                                             ANGLE_PER_DEGREE)));
            /* NOLINTEND(bugprone-integer-division) */
            game->targets[i].solid->work->pos.y =
                work->pos.y + game->targets[i].height;
            game->targets[i].solid->work->pos.z =
                work->pos.z + (HOLE_REACH * njSin(-work->ang.y + ang));
            collision_register(game->targets[i].solid->work);
            game->targets[i].sensor->work->pos.x =
                game->targets[i].solid->work->pos.x;
            game->targets[i].sensor->work->pos.y =
                game->targets[i].solid->work->pos.y;
            game->targets[i].sensor->work->pos.z =
                game->targets[i].solid->work->pos.z;
            collision_register(game->targets[i].sensor->work);
        }
    }
}

/* score each target whose sensor the hammer hit. a target rising, up or
 * sinking explodes and goes to TARGET_HIT. a hit in any mode adds the
 * target's points to the score, then zeroes them. a target thus scores once
 * until TARGET_NEW, and only that hit counts in mole_hits. the score stays 0
 * or more */
static void mole_score_hits(struct task *task, struct task_work *work)
{
    int i;
    NJS_POINT3 pos;

    for (i = 0; i < MOLE_TARGETS; i++) {
        if (game->targets[i].sensor->work->flags & COLLISION_DAMAGED) {
            if (game->targets[i].mode == TARGET_RISE ||
                game->targets[i].mode == TARGET_SINK ||
                game->targets[i].mode == TARGET_UP) {
                /* explosion_create starts at work's position, so the
                 * machine's work takes the solid's for the call */
                pos.x = work->pos.x;
                pos.y = work->pos.y;
                pos.z = work->pos.z;
                work->pos.x = game->targets[i].solid->work->pos.x;
                work->pos.y = game->targets[i].solid->work->pos.y;
                work->pos.z = game->targets[i].solid->work->pos.z;
                explosion_create(work, 1.0F);
                work->pos.x = pos.x;
                work->pos.y = pos.y;
                work->pos.z = pos.z;
                game->targets[i].mode = TARGET_HIT;
                game->targets[i].wait = 0;
                game->targets[i].frame = HIT_FRAME;
                sound_play(SE_MOLE_HIT, 0, 0, 0);
                if (game->targets[i].kind == KIND_EGGMAN) {
                    sound_play(SE_MOLE_HIT_EGGMAN, 0, 0, 0);
                }
            }
            if (game->targets[i].points != 0) {
                mole_hits[game->targets[i].kind].count++;
            }
            game->score += game->targets[i].points;
            if (game->score < 0) {
                game->score = 0;
            }
            if (game->targets[i].points < 0) {
                game->flash = FLASH_RED;
            }
            game->targets[i].points = 0;
        }
    }
}

/* pick the screen from work->mode and the message, and draw the panel
 * toward its look, with its text. a screen's text draws only when mole_shown
 * already holds it. so the text skips the first draw after a change. a change
 * of this game's screen also starts text_wait again */
static void mole_draw_board(struct task *task, struct task_work *work)
{
    struct mole_panel_look look;

    switch (work->mode) {
    case MOLE_WAIT:
        look.color.argb.b = 255;
        look.color.argb.g = 32;
        look.color.argb.r = 64;
        look.color.argb.a = 128;
        look.width = PANEL_NARROW;
        mole_draw_panel(&look);
        if (mole_shown == SCREEN_BEST) {
            mole_draw_text(&mole_texts[LINE_BEST], TEXT_1);
            mole_draw_number(&mole_texts[LINE_SCORE], mole_best, SCORE_DIGITS);
            mole_draw_text(&mole_texts[LINE_PT], TEXT_2);
            if (mole_best == MOLE_EGGMAN_BEST) {
                mole_draw_text(&mole_texts[LINE_EGGMAN], TEXT_3);
            } else {
                mole_draw_text(&mole_texts[LINE_AMY], TEXT_4);
            }
        }
        mole_shown = SCREEN_BEST;
        break;
    case MOLE_READY:
        look.color.argb.b = 0;
        look.color.argb.g = 16;
        look.color.argb.r = 32;
        look.color.argb.a = 32;
        look.width = PANEL_NARROW;
        mole_draw_panel(&look);
        if (mole_shown == SCREEN_START) {
            mole_draw_text(&mole_texts[LINE_START], TEXT_5);
        }
        mole_shown = SCREEN_START;
        break;
    case MOLE_PLAY:
        look.color.argb.b = 255;
        look.color.argb.g = 0;
        look.color.argb.r = 16;
        look.color.argb.a = 16;
        look.width = PANEL_NARROW;
        mole_draw_panel(&look);
        if (mole_shown == SCREEN_COUNT) {
            mole_draw_text(&mole_texts[LINE_TIME], TEXT_6);
            mole_draw_text(&mole_texts[LINE_COUNT], TEXT_7);
            mole_draw_number(&mole_texts[LINE_TIME_LEFT], game->time_left,
                             SCORE_DIGITS);
            mole_draw_number(&mole_texts[LINE_SCORE], game->score,
                             SCORE_DIGITS);
            mole_draw_text(&mole_texts[LINE_PT], TEXT_2);
        }
        mole_shown = SCREEN_COUNT;
        break;
    case MOLE_END:
        switch (game->message) {
        case MESSAGE_GAME_OVER:
            look.color.argb.b = 0;
            look.color.argb.g = 0;
            look.color.argb.r = 32;
            look.color.argb.a = 32;
            look.width = PANEL_WIDE;
            mole_draw_panel(&look);
            if (mole_shown == SCREEN_GAME_OVER) {
                mole_draw_scroll(TEXT_8, game->scroll);
            }
            mole_shown = SCREEN_GAME_OVER;
            break;
        case MESSAGE_CONGRATULATIONS:
            look.color.argb.b = 0;
            look.color.argb.g = 32;
            look.color.argb.r = 0;
            look.color.argb.a = 32;
            look.width = PANEL_WIDE;
            mole_draw_panel(&look);
            if (mole_shown == SCREEN_CONGRATULATIONS) {
                mole_draw_scroll(TEXT_9, game->scroll);
            }
            mole_shown = SCREEN_CONGRATULATIONS;
            break;
        case MESSAGE_ITEM:
            look.color.argb.b = 32;
            look.color.argb.g = 0;
            look.color.argb.r = 0;
            look.color.argb.a = 32;
            look.width = PANEL_WIDE;
            mole_draw_panel(&look);
            /* FIGHTERS FEATHER GET until Amy has the feather, and then LONG
             * HAMMER GET */
            if (mole_shown == SCREEN_ITEM) {
                if (!event_flag_check(FLAG_AMY_WARRIOR_FEATHER)) {
                    mole_draw_scroll(TEXT_10, game->scroll);
                } else {
                    mole_draw_scroll(TEXT_11, game->scroll);
                }
            }
            mole_shown = SCREEN_ITEM;
            break;
        case MESSAGE_HISCORE:
            look.color.argb.b = 16;
            look.color.argb.g = 0;
            look.color.argb.r = 16;
            look.color.argb.a = 32;
            look.width = PANEL_WIDE;
            mole_draw_panel(&look);
            if (mole_shown == SCREEN_HISCORE) {
                mole_draw_scroll(TEXT_12, game->scroll);
            }
            mole_shown = SCREEN_HISCORE;
            break;
        default:
            look.color.argb.b = 0;
            look.color.argb.g = 0;
            look.color.argb.r = 0;
            look.color.argb.a = 0;
            look.width = PANEL_NARROW;
            mole_draw_panel(&look);
            mole_shown = SCREEN_BLANK;
            break;
        }
        break;
    }
    if (mole_shown != game->last_screen) {
        game->text_wait = MOLE_TEXT_WAIT;
    }
    game->last_screen = mole_shown;
    game->scroll += SCROLL_STEP;
    if (game->scroll >= SCROLL_END) {
        game->scroll = 0.0F;
    }
}

/* draw glyph of sprite at the board's position and angles, scaled by
 * BOARD_SCALE with y flipped, blending by adding. it draws nothing until
 * text_wait runs out */
static void mole_draw_sprite(NJS_SPRITE *sprite, Int glyph, Uint32 attr)
{
    Angle ang;

    if (game->text_wait > 0) {
        return;
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &game->pos);
    TURN_VIA(njRotateZ, game->ang.z, ang);
    TURN_VIA(njRotateX, game->ang.x, ang);
    TURN_VIA(njRotateY, game->ang.y, ang);
    njScale(NULL, BOARD_SCALE, -BOARD_SCALE, BOARD_SCALE);
    njDrawSprite3D(sprite, glyph, attr);
    njPopMatrix(1);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
}

/* move *value 4 toward target, stopping at target */
static void approach_byte(Uint8 *value, Uint8 target)
{
    int moved = *value;

    if (moved > target) {
        moved -= 4;
        if (moved < target) {
            moved = target;
        }
    } else if (moved < target) {
        moved += 4;
        if (moved > target) {
            moved = target;
        }
    }
    *value = moved;
}

/* move *value 2 toward target, stopping at target */
static void approach_float(Float *value, Float target)
{
    if (*value > target) {
        *value -= 2.0F;
        if (*value < target) {
            *value = target;
        }
    } else if (*value < target) {
        *value += 2.0F;
        if (*value > target) {
            *value = target;
        }
    }
}

/* draw the board's panel, PANEL_DROP below the board. each draw moves its
 * half width toward look's, and its color too. at FLASH_RED the color turns
 * red instead, and at FLASH_BACK it takes back the color it had before */
static void mole_draw_panel(struct mole_panel_look *look)
{
    Angle ang;

    switch (game->flash) {
    case FLASH_BACK:
        mole_panel_colors[0].color = mole_saved_color.color;
        break;
    case FLASH_RED:
        mole_saved_color.color = mole_panel_colors[0].color;
        mole_panel_colors[0].argb.b = 0;
        mole_panel_colors[0].argb.g = 0;
        mole_panel_colors[0].argb.r = 255;
        mole_panel_colors[0].argb.a = 192;
        break;
    default:
        approach_byte(&mole_panel_colors[0].argb.b, look->color.argb.b);
        approach_byte(&mole_panel_colors[0].argb.g, look->color.argb.g);
        approach_byte(&mole_panel_colors[0].argb.r, look->color.argb.r);
        approach_byte(&mole_panel_colors[0].argb.a, look->color.argb.a);
        break;
    }
    game->flash--;
    if (game->flash < 0) {
        game->flash = 0;
    }
    mole_panel_colors[1].color = mole_panel_colors[2].color =
        mole_panel_colors[3].color = mole_panel_colors[0].color;
    approach_float(&mole_panel_points[0].x, look->width);
    mole_panel_points[1].x = mole_panel_points[0].x;
    mole_panel_points[2].x = mole_panel_points[3].x = -mole_panel_points[0].x;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    njTranslate(NULL, game->pos.x, game->pos.y - PANEL_DROP, game->pos.z);
    TURN_VIA(njRotateZ, game->ang.z, ang);
    TURN_VIA(njRotateX, game->ang.x, ang);
    TURN_VIA(njRotateY, game->ang.y, ang);
    njDrawPolygon3D(&mole_panel, 4, NJD_FILL | NJD_TRANSPARENT);
    njPopMatrix(1);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
}

/* draw value, held to 0 through 99999, from the line's start in digits
 * places, a digit a place. a place above value's highest digit stays blank,
 * and the ones always draw */
static void mole_draw_number(struct mole_text *text, int value, int digits)
{
    NJS_SPRITE sprite;
    int digit;
    int limit;
    int i;

    sprite.tlist = &mole_texlist;
    sprite.tanim = mole_font;
    sprite.sx = text->sx;
    sprite.sy = text->sy;
    sprite.ang = 0;
    sprite.p.x = text->x;
    sprite.p.y = text->y;
    sprite.p.z = 0.0F;
    if (value >= 100000) {
        value = 99999;
    }
    if (value <= 0) {
        value = 0;
    }
    /* @bug logic: limit starts at 10, not 1 (MOV #10,R11 at 0x0c902f72).
     * after the loop it holds digits + 1 nines, not digits nines. for
     * SCORE_DIGITS, limit is 99999, and the clamp above keeps value at or
     * under it. the test then never holds (CMP/GT at 0x0c902f8e), and a
     * value of 10000 or more shows only its low four digits */
    limit = 10;
    for (i = 0; i < digits; i++) {
        limit *= 10;
    }
    limit--;
    if (value > limit) {
        value = limit;
    }
    if (digits >= 5) {
        /* @hack spelling: digit through its address, which keeps it in the
         * image's 64-byte frame (ADD #-36,R15 at 0x0c902f2e); without it, the
         * frame is 60, and 190 of 213 words differ */
        *&digit = value / 10000;
        if (value >= 10000) {
            mole_draw_sprite(&sprite, digit, NJD_SPRITE_ALPHA);
        }
        sprite.p.x += text->dx;
        sprite.p.y += text->dy;
    }
    if (digits >= 4) {
        digit = (value % 10000) / 1000;
        if (value >= 1000) {
            mole_draw_sprite(&sprite, digit, NJD_SPRITE_ALPHA);
        }
        sprite.p.x += text->dx;
        sprite.p.y += text->dy;
    }
    if (digits >= 3) {
        digit = (value % 1000) / 100;
        if (value >= 100) {
            mole_draw_sprite(&sprite, digit, NJD_SPRITE_ALPHA);
        }
        sprite.p.x += text->dx;
        sprite.p.y += text->dy;
    }
    if (digits >= 2) {
        digit = (value % 100) / 10;
        if (value >= 10) {
            mole_draw_sprite(&sprite, digit, NJD_SPRITE_ALPHA);
        }
        sprite.p.x += text->dx;
        sprite.p.y += text->dy;
    }
    if (digits >= 1) {
        digit = value % 10;
        mole_draw_sprite(&sprite, digit, NJD_SPRITE_ALPHA);
    }
}

/* draw string, of capitals and spaces, from the line's start, a letter a
 * place. a space leaves its place blank */
static void mole_draw_text(struct mole_text *text, char *string)
{
    NJS_SPRITE sprite;

    sprite.tlist = &mole_texlist;
    sprite.tanim = mole_font;
    sprite.sx = text->sx;
    sprite.sy = text->sy;
    sprite.ang = 0;
    sprite.p.x = text->x;
    sprite.p.y = text->y;
    sprite.p.z = 0.0F;
    while (*string) {
        if (LETTER(string) != ' ') {
            mole_draw_sprite(&sprite, FONT_CAPITAL(LETTER(string)),
                             NJD_SPRITE_ALPHA);
        }
        sprite.p.x += text->dx;
        sprite.p.y += text->dy;
        string++;
    }
}

/* draw string scrolled across the board's eight places, by progress out of
 * SCROLL_END. at 0 its first letter stands just past the right edge. at
 * SCROLL_END its last has gone past the left edge. a letter across an edge is
 * cut there */
static void mole_draw_scroll(char *string, Float progress)
{
    NJS_SPRITE sprite;
    Float offset;
    char *scan;
    int length;
    int place;

    sprite.tlist = &mole_texlist;
    sprite.tanim = (NJS_TEXANIM *)&mole_scroll_texanim;
    sprite.sy = 2.0F;
    sprite.ang = 0;
    sprite.p.z = 0.0F;
    scan = string;
    length = 0;
    while (*scan) {
        scan++;
        length++;
    }
    sprite.p.y = 0.0F;
    offset = (length + 8) * progress * 0.01F;
    place = 8;
    while (*string) {
        sprite.p.x = place - offset;
        /* @bug memory: a space indexes mole_font at FONT_CAPITAL(' '),
         * -23, so each branch reads mole_font[-23].texid, as MOV.W
         * @(16,R2),R0 does at 0x0c90326a. a space then draws nothing */
        /* @hack type: the letter's texture coordinates stored through plain
         * pointers that cast the volatile away, so SHC loads the members'
         * addresses ahead of the stores, from 0x0c903234; without it, 31 of
         * 251 words differ */
        if (sprite.p.x >= 0.0F && sprite.p.x < 7.0F) {
            sprite.sx = 2.0F;
            sprite.p.x = (sprite.p.x - 4.0F) * 64.0F;
            *(Sint16 *)&mole_scroll_texanim.u1 = 0;
            *(Sint16 *)&mole_scroll_texanim.v1 = 0;
            *(Sint16 *)&mole_scroll_texanim.u2 = 256;
            *(Sint16 *)&mole_scroll_texanim.v2 = 256;
#ifdef GCC
            /* repairs the read the tag names: GCC loads no texid for a
             * space, which draws nothing */
            if (LETTER(string) != ' ') {
                mole_scroll_texanim.texid =
                    mole_font[FONT_CAPITAL(LETTER(string))].texid;
            }
#else
            mole_scroll_texanim.texid =
                mole_font[FONT_CAPITAL(LETTER(string))].texid;
#endif
            if (LETTER(string) != ' ') {
                mole_draw_sprite(&sprite, 0, NJD_SPRITE_ALPHA);
            }
        } else if (sprite.p.x >= -1.0F && sprite.p.x < 0.0F) {
            *(Sint16 *)&mole_scroll_texanim.u1 = -(sprite.p.x * 256.0F);
            *(Sint16 *)&mole_scroll_texanim.u2 = 256;
            sprite.sx = (sprite.p.x + 1.0F) * 2.0F;
            sprite.p.x = -sprite.sx - 256.0F;
#ifdef GCC
            /* as above: no texid for a space */
            if (LETTER(string) != ' ') {
                mole_scroll_texanim.texid =
                    mole_font[FONT_CAPITAL(LETTER(string))].texid;
            }
#else
            mole_scroll_texanim.texid =
                mole_font[FONT_CAPITAL(LETTER(string))].texid;
#endif
            if (LETTER(string) != ' ') {
                mole_draw_sprite(&sprite, 0, NJD_SPRITE_ALPHA);
            }
        } else if (sprite.p.x >= 7.0F && sprite.p.x < 8.0F) {
            *(Sint16 *)&mole_scroll_texanim.u1 = 0;
            *(Sint16 *)&mole_scroll_texanim.u2 = (8.0F - sprite.p.x) * 256.0F;
            sprite.sx = (8.0F - sprite.p.x) * 2.0F;
            sprite.p.x = (sprite.p.x - 4.0F) * 64.0F;
#ifdef GCC
            /* as above: no texid for a space */
            if (LETTER(string) != ' ') {
                mole_scroll_texanim.texid =
                    mole_font[FONT_CAPITAL(LETTER(string))].texid;
            }
#else
            mole_scroll_texanim.texid =
                mole_font[FONT_CAPITAL(LETTER(string))].texid;
#endif
            if (LETTER(string) != ' ') {
                mole_draw_sprite(&sprite, 0, NJD_SPRITE_ALPHA);
            }
        }
        string++;
        place++;
    }
}

/* step through the round's results, by result. RESULT_SCORE saves the score
 * and starts the score screen. once mole_score_done reports the screen's
 * end, player 0's pad stops driving Amy, and the game over's textures load.
 * the game over's music starts after RESULT_LOAD's wait. on A, the choice
 * restarts the stage or ends the act with STAGE_END_CLEAR. the screen then
 * turns black. each step disables pausing */
static void mole_results(struct task *task, struct task_work *work)
{
    switch (game->result) {
    case RESULT_SCORE:
        mole_save_score();
        task_create(TASK_WORK, 1, score_step);
        game->result = RESULT_TALLY;
        game->result_steps = 0;
        mole_score_over = FALSE;
        break;
    case RESULT_TALLY:
        if (mole_score_over == TRUE) {
            game->result_steps = 0;
            game->result = RESULT_LOAD;
            mole_choice = CHOICE_CONTINUE;
            pad_disable_player(0);
            stream_destroy();
            texture_load_pvm("GAMEOVER", &game_over_texlist);
            njSetBackColor(0xff000000, 0xff000000, 0xff000000);
        }
        break;
    case RESULT_LOAD:
        game->result_steps++;
        if (game->result_steps > RESULT_LOAD_WAIT) {
            game->result = RESULT_CHOOSE;
            stream_reset();
            stream_request_interlude(MUSIC_CONTINUE);
        }
        break;
    case RESULT_CHOOSE:
        if ((pad_press_live[0] & PDD_DGT_KL) &&
            mole_choice != CHOICE_CONTINUE) {
            mole_choice = CHOICE_CONTINUE;
            sound_play(SE_MOVE, 0, 0, 0);
        } else if ((pad_press_live[0] & PDD_DGT_KR) &&
                   mole_choice != CHOICE_QUIT) {
            mole_choice = CHOICE_QUIT;
            sound_play(SE_MOVE, 0, 0, 0);
        }
        if (pad_press_live[0] & PDD_DGT_TA) {
            game_over_end();
            sound_play(SE_CHOOSE, 0, 0, 0);
            if (mole_choice == CHOICE_CONTINUE) {
                stage_restart();
            } else {
                stage_end(STAGE_END_CLEAR);
            }
            game_over_end();
            game->result = RESULT_BLACK;
            mole_draw_black();
        } else {
            mole_draw_game_over(mole_choice);
        }
        break;
    case RESULT_BLACK:
        mole_draw_black();
        break;
    }
    pause_disable();
}

/* rank the round's score among the save's three best, dropping the
 * lowest. add it to the hud's score too */
static void mole_save_score(void)
{
    if (saves[save_slot].mole_scores.score[0] < game->score) {
        saves[save_slot].mole_scores.score[2] =
            saves[save_slot].mole_scores.score[1];
        saves[save_slot].mole_scores.score[1] =
            saves[save_slot].mole_scores.score[0];
        saves[save_slot].mole_scores.score[0] = game->score;
    } else if (saves[save_slot].mole_scores.score[1] < game->score) {
        saves[save_slot].mole_scores.score[2] =
            saves[save_slot].mole_scores.score[1];
        saves[save_slot].mole_scores.score[1] = game->score;
    } else if (saves[save_slot].mole_scores.score[2] < game->score) {
        saves[save_slot].mole_scores.score[2] = game->score;
    }
    score_add(game->score);
}

/* raise mole_best to the save's best */
static void mole_load_best(void)
{
    if (mole_best < saves[save_slot].mole_scores.score[0]) {
        mole_best = saves[save_slot].mole_scores.score[0];
    }
}

/* draw black over the whole screen */
static void mole_draw_black(void)
{
    NJS_POINT2COL quad;
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    NJS_COLOR color;

    quad.p = points;
    quad.col = colors;
    quad.tex = NULL;
    quad.num = 4;
    color.argb.r = 0;
    color.argb.g = 0;
    color.argb.b = 0;
    color.argb.a = 255;
    quad.p[0].x = 0.0F;
    quad.p[0].y = 0.0F;
    quad.col[0].color = color.color;
    quad.p[1].x = SCREEN_WIDTH;
    quad.p[1].y = 0.0F;
    quad.col[1].color = color.color;
    quad.p[2].x = SCREEN_WIDTH;
    quad.p[2].y = SCREEN_HEIGHT;
    quad.col[2].color = color.color;
    quad.p[3].x = 0.0F;
    quad.p[3].y = SCREEN_HEIGHT;
    quad.col[3].color = color.color;
    njDrawPolygon2D(&quad, 4, -0.1F, SEE_THROUGH);
}

void mole_score_done(void)
{
    mole_score_over = TRUE;
}

/* draw the game over screen as game_over_step does, but over solid black
 * rather than its fade. it shows the banner and both choices, with a glow
 * behind the chosen one. the chosen one's frame pulses between red and blue,
 * and the other's is blue */
static void mole_draw_game_over(int choice)
{
    NJS_POINT2COL quad;
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    NJS_COLOR shade;
    NJS_COLOR *color;
    NJS_COLOR uv[4];
    Float glow;
    /* @hack type: volatile on blue, which puts chosen at R15+4, stored
     * at 0x0c90383a, and the 0 at 0x0c903888 in FR14; without it, chosen
     * takes R15+0, the floats shift from FR14 to FR12, and 163 of 554 words
     * differ */
    volatile Float blue;
    Float left;
    Float right;
    int offset;
    /* @hack type: volatile on chosen, which keeps it in the image's
     * 140-byte frame, stored at 0x0c90383a; without it, chosen takes R8, the
     * frame is 136, and 430 of 554 words differ */
    volatile int chosen = choice;

    glow = mole_game_over_phase;
    mole_game_over_phase = (mole_game_over_phase + 4) & 0xfc;
    if (glow < 0.0F) {
        glow = -glow;
    }
    glow *= 1.0F / 128.0F;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    quad.p = points;
    quad.col = colors;
    quad.tex = NULL;
    quad.num = 4;
    color = &shade;
    color->argb.r = 0;
    color->argb.g = 0;
    color->argb.b = 0;
    color->argb.a = 255;
    quad.p[0].x = 0.0F;
    quad.p[0].y = 0.0F;
    quad.col[0].color = color->color;
    quad.p[1].x = SCREEN_WIDTH;
    quad.p[1].y = 0.0F;
    quad.col[1].color = color->color;
    quad.p[2].x = SCREEN_WIDTH;
    quad.p[2].y = SCREEN_HEIGHT;
    quad.col[2].color = color->color;
    quad.p[3].x = 0.0F;
    quad.p[3].y = SCREEN_HEIGHT;
    quad.col[3].color = color->color;
    njDrawPolygon2D(&quad, 4, -1.05F, SEE_THROUGH);

    color->argb.b = color->argb.g = color->argb.r = 255;
    quad.tex = uv;
    njSetTexture(&game_over_texlist);
    njSetTextureNum(GAME_OVER_BANNER);
    points[0].x = 192.0F;
    points[0].y = 160.0F;
    colors[0].color = color->color;
    points[1].x = 448.0F;
    points[1].y = 160.0F;
    colors[1].color = color->color;
    points[2].x = 448.0F;
    points[2].y = 208.0F;
    colors[2].color = color->color;
    points[3].x = 192.0F;
    points[3].y = 208.0F;
    colors[3].color = color->color;
    uv[0].tex.u = 0;
    uv[0].tex.v = 0;
    uv[1].tex.u = 255;
    uv[1].tex.v = 0;
    uv[2].tex.u = 255;
    uv[2].tex.v = 255;
    uv[3].tex.u = 0;
    uv[3].tex.v = 255;
    njDrawPolygon2D(&quad, 4, -1.04F, SOLID_TEXTURED);

    /* @hack redundant: a never-run call of njDrawPolygon2D at depth 0.0F,
     * which keeps the 0.0F in FR14 (FLDI0 at 0x0c903888). with the call
     * below, it also keeps points' address in R8 (0x0c90386e) and 28 bytes of
     * fill before 0x0c903a20; without it, 409 of 554 words differ */
    if (0) {
        njDrawPolygon2D(&quad, 4, 0.0F, SEE_THROUGH);
    }
    /* @hack redundant: a second never-run call, at depth 160.0F. with the
     * call above, it keeps points' address in R8 (0x0c90386e) and 28 bytes of
     * fill before 0x0c903a20; without it, 401 of 554 words differ */
    if (0) {
        njDrawPolygon2D(&quad, 4, 160.0F, SEE_THROUGH);
    }

    /* each choice's words, over their back */
    quad.p[0].x = 160.0F;
    quad.p[0].y = 368.0F;
    quad.p[1].x = 288.0F;
    quad.p[1].y = 368.0F;
    quad.p[2].x = 288.0F;
    quad.p[2].y = 400.0F;
    quad.p[3].x = 160.0F;
    quad.p[3].y = 400.0F;
    njSetTextureNum(GAME_OVER_CONTINUE);
    njDrawPolygon2D(&quad, 4, -1.0F, SEE_THROUGH_TEXTURED);
    njSetTextureNum(GAME_OVER_BACK);
    njDrawPolygon2D(&quad, 4, -1.02F, SEE_THROUGH_TEXTURED);
    quad.p[0].x = 352.0F;
    quad.p[1].x = 480.0F;
    quad.p[2].x = 480.0F;
    quad.p[3].x = 352.0F;
    njSetTextureNum(GAME_OVER_QUIT);
    njDrawPolygon2D(&quad, 4, -1.0F, SEE_THROUGH_TEXTURED);
    njSetTextureNum(GAME_OVER_BACK);
    njDrawPolygon2D(&quad, 4, -1.02F, SEE_THROUGH_TEXTURED);

    /* the glow, from orange to yellow, behind the one chosen */
    njSetTextureNum(GAME_OVER_CURSOR);
    color->argb.b = 0;
    color->argb.g = 152.0F + (glow * 83.0F);
    color->argb.r = 236.0F + (glow * 19.0F);
    /* @hack spelling: offset set inside its first use, which keeps the frame
     * address R15+44 in R8 from 0x0c90386a to its use at 0x0c90393a; set in a
     * statement of its own before that use, 401 of 554 words differ */
    quad.p[0].x = left = 160 + (offset = 192 * chosen);
    quad.p[0].y = 352.0F;
    quad.col[0].color = color->color;
    quad.p[1].x = right = 288 + offset;
    quad.p[1].y = 352.0F;
    quad.col[1].color = color->color;
    quad.p[2].x = right;
    quad.p[2].y = 416.0F;
    quad.col[2].color = color->color;
    quad.p[3].x = left;
    quad.p[3].y = 416.0F;
    quad.col[3].color = color->color;
    njDrawPolygon2D(&quad, 4, -1.01F, SEE_THROUGH_TEXTURED);

    /* each choice's frame: blue, or pulsing if it's the one chosen */
    blue = glow * 255.0F;
    if (chosen == CHOICE_CONTINUE) {
        color->argb.b = 0.0F + blue;
        color->argb.g = 0;
        color->argb.r = 255.0F - blue;
    } else {
        color->argb.b = 255;
        color->argb.g = 0;
        color->argb.r = 0;
    }
    njSetTextureNum(GAME_OVER_FRAME);
    quad.p[0].x = 160.0F;
    quad.col[0].color = color->color;
    quad.p[1].x = 288.0F;
    quad.col[1].color = color->color;
    quad.p[2].x = 288.0F;
    quad.col[2].color = color->color;
    quad.p[3].x = 160.0F;
    quad.col[3].color = color->color;
    njDrawPolygon2D(&quad, 4, -1.02F, SEE_THROUGH_TEXTURED);
    if (chosen == CHOICE_QUIT) {
        color->argb.b = 0.0F + blue;
        color->argb.g = 0;
        color->argb.r = 255.0F - blue;
    } else {
        color->argb.b = 255;
        color->argb.g = 0;
        color->argb.r = 0;
    }
    quad.p[0].x += 192.0F;
    quad.col[0].color = color->color;
    quad.p[1].x += 192.0F;
    quad.col[1].color = color->color;
    quad.p[2].x += 192.0F;
    quad.col[2].color = color->color;
    quad.p[3].x += 192.0F;
    quad.col[3].color = color->color;
    njDrawPolygon2D(&quad, 4, -1.02F, SEE_THROUGH_TEXTURED);
}
