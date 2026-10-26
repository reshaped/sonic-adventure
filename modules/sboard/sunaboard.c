/* Sand Hill's stage task. it boards player 0, and starts the music after
 * MUSIC_WAIT. a step after the wait begins, it creates the act's tasks and
 * enables the pads and the pause. from then on, it widens the view with
 * the board's speed */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/invisible.h"
#include "object/balloon.h"
#include "object/capsule.h"
#include "object/dash_panel.h"
#include "object/dash_ring.h"
#include "object/iron_ball.h"
#include "object/item_box.h"
#include "object/kusa.h"
#include "object/panel.h"
#include "object/piece.h"
#include "object/placement.h"
#include "object/ring.h"
#include "object/ring_group.h"
#include "object/rocket.h"
#include "object/savepoint.h"
#include "object/spring.h"
#include "object/switch.h"
#include "object/toge_bubble.h"
#include "player/tails.h"
#include "player/player.h"
#include "player/sonic.h"
#include "sboard/act.h"
#include "sboard/big_cactus.h"
#include "sboard/pillar.h"
#include "sboard/sand_frog.h"
#include "sboard/sand_gate.h"
#include "sboard/sand_jump.h"
#include "sboard/sand_rock.h"
#include "sboard/small_cactus.h"
#include "sboard/sunaboard.h"
#include "sound/cue.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/task_desc.h"

struct sunaboard_record {
    Sint8 mode;
};

enum sunaboard_mode {
    SUNABOARD_BOARD,
    SUNABOARD_MUSIC,
    SUNABOARD_START,
    SUNABOARD_RUN
};

/* the music's wait, which music_start_later's task counts down once a
 * step and once a draw */
#define MUSIC_WAIT 8

/* the field of view widens from VIEW_ANGLE at rest to VIEW_ANGLE +
 * VIEW_WIDEN at FULL_SPEED and above */
#define VIEW_ANGLE NJM_DEG_ANG(70)
#define VIEW_WIDEN NJM_DEG_ANG(30)
#define FULL_SPEED (4.6F * 1.7F)

/* the stage task watches player 0's y. below FALL_Y, it ends the music and
 * makes the player fall out of the stage */
#define FALL_Y (-7405.0F)

static void sunaboard_init(struct task *task, void *arg);
static void sunaboard_run(struct task *task);
static void sunaboard_board(void);
static void sunaboard_watch(void);
static void sunaboard_widen(struct player *record, struct task_work *work);

/* the stage task's kind, in HEAD at 0x0c900190 */
#pragma section HEAD
static const struct task_desc sunaboard_desc = {sizeof(struct sunaboard_record),
                                                TASK_WORK,
                                                sunaboard_init,
                                                sunaboard_run,
                                                NULL,
                                                NULL};
#pragma section

/* the placement ranges, squared, of the kinds with KIND_RANGE */
#define NEAR (1200.0F * 1200.0F)
#define MIDDLE (2000.0F * 2000.0F)
#define FAR (2400.0F * 2400.0F)

/* clang-format off */
struct object_kind sunaboard_object_kinds[] = {
    {TASK_WORK, 3, 0, 0.0F, NULL, ring_step, "RING   "},
    {TASK_WORK, 3, 0, 0.0F, NULL, ring_group_step, "O GRING"},
    {TASK_WORK, 2, 0, 0.0F, NULL, spring_step, "SPRING "},
    {TASK_WORK, 2, 0, 0.0F, NULL, spring_b_step, "SPRINGB"},
    {TASK_MOTION | TASK_WORK, 3, 0, 0.0F, NULL, dash_panel_step, "O AXPNL"},
    {TASK_WORK | TASK_FORCE, 3, 0, 0.0F, NULL, iron_ball_init, "O IRONB"},
    {TASK_WORK, 3, 0, 0.0F, NULL, iron_ball_jg_init, "O FeBJG"},
    {TASK_WORK, 3, 0, 0.0F, NULL, toge_init, "O TOGE"},
    {TASK_MOTION | TASK_WORK, 3, 0, 0.0F, NULL, piece_start, "O EME P"},
    {TASK_WORK, 3, 0, 0.0F, NULL, capsule_start, "O RELEASE"},
    {TASK_WORK | TASK_FORCE, 3, 0, 0.0F, NULL, switch_step, "O SWITCH"},
    {TASK_WORK | TASK_EXTRA, 3, 0, 0.0F, NULL, kusa_step, "CMN KUSA"},
    {TASK_WORK | TASK_FORCE | TASK_EXTRA, 3, 0, 0.0F, NULL, dash_ring_step,
     "CMN_DRING"},
    {TASK_WORK, 3, 0, 0.0F, NULL, balloon_step, "O BALOON"},
    {TASK_WORK, 3, 0, 0.0F, NULL, item_box_step, "O ITEMBOX"},
    {TASK_WORK | TASK_FORCE | TASK_EXTRA, 2, 0, 0.0F, NULL, rocket_h_step,
     "Rocket H"},
    {TASK_WORK | TASK_FORCE | TASK_EXTRA, 2, 0, 0.0F, NULL, rocket_hs_step,
     "Rocket HS"},
    {TASK_WORK | TASK_FORCE | TASK_EXTRA, 2, 0, 0.0F, NULL, rocket_v_step,
     "Rocket V"},
    {TASK_WORK | TASK_FORCE | TASK_EXTRA, 2, 0, 0.0F, NULL, rocket_vs_step,
     "Rocket VS"},
    {TASK_WORK, 2, KIND_RANGE, MIDDLE, NULL, panel_start, "O JPanel"},
    {TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA, 3, 0, 0.0F, NULL,
     savepoint_step, "O Save Point"},
    {TASK_WORK, 3, 0, 0.0F, NULL, invisible_wall_step, "WALL   "},
    {TASK_WORK, 3, KIND_RANGE, NEAR, NULL, big_cactus_a_step, "BIG CACTUS A"},
    {TASK_WORK, 3, KIND_RANGE, NEAR, NULL, big_cactus_b_step, "BIG CACTUS B"},
    {TASK_WORK, 3, KIND_RANGE, NEAR, NULL, big_cactus_c_step, "BIG CACTUS C"},
    {TASK_MOTION | TASK_WORK, 3, KIND_RANGE, NEAR, NULL, small_cactus_a_step,
     "SMALL CACTUS A"},
    {TASK_MOTION | TASK_WORK, 3, KIND_RANGE, NEAR, NULL, small_cactus_b_step,
     "SMALL CACTUS B"},
    {TASK_WORK | TASK_FORCE, 3, KIND_RANGE, NEAR, NULL, sand_rock_step, "ROCK"},
    {TASK_WORK | TASK_FORCE, 3, KIND_RANGE, FAR, NULL, sand_rocky_head_step,
     "ROCKY HEAD"},
    {TASK_WORK | TASK_FORCE, 3, KIND_RANGE, FAR, NULL, pillar_s_step,
     "PILLAR S"},
    {TASK_WORK | TASK_FORCE, 3, KIND_RANGE, FAR, NULL, pillar_m_step,
     "PILLAR M"},
    {TASK_WORK | TASK_FORCE, 3, KIND_RANGE, FAR, NULL, pillar_l_step,
     "PILLAR L"},
    {TASK_WORK | TASK_FORCE, 3, KIND_RANGE, FAR, NULL, sand_snake_statue_step,
     "SNAKE STATUE"},
    {TASK_WORK | TASK_FORCE, 3, KIND_RANGE, FAR, NULL, sand_jump_step,
     "SAND JUMP"},
    {TASK_WORK | TASK_FORCE, 3, KIND_RANGE, FAR, NULL, sand_gate_step,
     "SAND GATE"},
    {TASK_WORK, 3, 0, 0.0F, NULL, sand_frog_step, "O SFROG "},
    {TASK_WORK, 2, 0, 0.0F, NULL, invisible_sphere_step, "COLLI SPHERE"},
    {TASK_WORK, 2, 0, 0.0F, NULL, invisible_cylinder_step, "COLLI CYLINDER"},
    {TASK_WORK, 2, 0, 0.0F, NULL, invisible_box_step, "COLLI CUBE"},
};
/* clang-format on */

/* in HEAD, at 0x0c900040. 1ST_READ.BIN's stage_steps holds that address
 * for the stage */
#pragma section HEAD
void sunaboard_step(struct task *task)
{
    task_apply_desc(task, &sunaboard_desc, NULL);
}
#pragma section

static void sunaboard_init(struct task *task, void *arg)
{
    struct sunaboard_record *record = (struct sunaboard_record *)task->motion;

    record->mode = SUNABOARD_BOARD;
}

static void sunaboard_run(struct task *task)
{
    struct sunaboard_record *record = (struct sunaboard_record *)task->motion;

    switch (record->mode) {
    case SUNABOARD_BOARD:
        sunaboard_board();
        record->mode = SUNABOARD_MUSIC;
        /* falls through: the music's wait starts on the step the player
         * boards */
    case SUNABOARD_MUSIC:
        if (music_start_later(MUSIC_SANDHILL, MUSIC_WAIT) != NULL) {
            record->mode = SUNABOARD_START;
        }
        break;
    case SUNABOARD_START:
        camera_set_perspective(VIEW_ANGLE);
        act_parent_create();
        pad_enable();
        pause_enable();
        record->mode = SUNABOARD_RUN;
        break;
    case SUNABOARD_RUN:
        sunaboard_watch();
        break;
    }
}

/* ask player 0 for PLAYER_MODE_BOARD, and create the board's task for
 * Sonic or Tails */
static void sunaboard_board(void)
{
    /* @hack spelling: the character through a local, which SHC copies to R4
     * in the first test's delay slot (MOV R0,R4 at 0x0c9031e0); switched on
     * directly, the copy goes, sunaboard_board runs 2 bytes short and 17 of
     * its 27 words differ */
    int character;

    player_set_mode(0, PLAYER_MODE_BOARD);
    character = get_character();
    switch (character) {
    case CHARACTER_SONIC:
        task_create(TASK_MOTION | TASK_WORK, 2, sonic_board_start);
        break;
    case CHARACTER_TAILS:
        task_create(TASK_MOTION | TASK_WORK, 2, tails_board_start);
        break;
    }
}

static void sunaboard_watch(void)
{
    struct player *record = player_get(0);
    struct task_work *work = player_work[0];
    NJS_POINT3 *pos = &work->pos;

    sunaboard_widen(record, work);
    if (pos->y < FALL_Y) {
        music_end();
        player_fall(0);
    }
}

/* set the field of view from record's speed ahead. it widens past
 * VIEW_ANGLE by VIEW_WIDEN times the square root of speed.x over
 * FULL_SPEED. from FULL_SPEED on, the root is 1 */
static void sunaboard_widen(struct player *record, struct task_work *work)
{
    Float speed = record->speed.x;
    Float share;

    if (speed >= FULL_SPEED) {
        share = 1.0F;
    } else {
        share = njSqrt(speed * (1.0F / FULL_SPEED));
    }
    camera_set_perspective((Angle)(VIEW_ANGLE + (VIEW_WIDEN * share)));
}
