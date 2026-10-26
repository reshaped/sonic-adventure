/* the cart course's stage task, and the kinds of object its acts place */
#include <sg_xpt.h>

#include "collision/invisible.h"
#include "collision/world.h"
#include "enemy/saru.h"
#include "enemy/spina.h"
#include "enemy/uni.h"
#include "hud/hud.h"
#include "minicart/act_menu.h"
#include "minicart/arch.h"
#include "minicart/cart.h"
#include "minicart/cart_dash_panel.h"
#include "minicart/cart_jump_panel.h"
#include "minicart/dial.h"
#include "minicart/e_cart.h"
#include "minicart/goal_robot.h"
#include "minicart/intro.h"
#include "minicart/panel.h"
#include "minicart/planets.h"
#include "minicart/race.h"
#include "minicart/rocket.h"
#include "minicart/satellite.h"
#include "minicart/ship.h"
#include "minicart/signboard.h"
#include "minicart/start_robot.h"
#include "minicart/times.h"
#include "minicart/zone.h"
#include "object/item_box.h"
#include "object/item_box_air.h"
#include "object/placement.h"
#include "object/ring.h"
#include "object/ring_group.h"
#include "object/savepoint.h"
#include "sound/stream.h"
#include "stage/act.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/task_desc.h"

/* the stage task's record, which task_apply_desc puts in its motion */
struct cart_record {
    Sint8 mode;
};

enum cart_mode { CART_START, CART_RUN };

/* the steps the music task waits. it requests the music on the step after
 * them */
#define MUSIC_WAIT 10

static void cart_init(struct task *task, void *arg);
static void cart_run(struct task *task);
static void cart_destroy(struct task *task);

static const struct task_desc cart_desc = {sizeof(struct cart_record),
                                           TASK_WORK,
                                           cart_init,
                                           cart_run,
                                           NULL,
                                           cart_destroy};

/* the kinds' placement ranges, squared, as struct object_kind takes them */
#define NEAR (1000.0F * 1000.0F)
#define MIDDLE (1500.0F * 1500.0F)
#define FAR (2000.0F * 2000.0F)
#define FARTHER (3000.0F * 3000.0F)
#define FARTHEST (3500.0F * 3500.0F)

/* clang-format off */
struct object_kind cart_object_kinds[] = {
    {TASK_WORK, 3, KIND_RANGE, FAR, NULL, ring_step, "RING"},
    {TASK_WORK, 3, KIND_RANGE, FAR, NULL, item_box_step, "O ITEMBOX"},
    {TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA, 3, KIND_RANGE, FAR,
     NULL, item_box_air_step, "O ITEMBOX AIR"},
    {TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA, 3, KIND_RANGE, FAR,
     NULL, savepoint_step, "O Save Point"},
    {TASK_WORK, 3, KIND_RANGE, MIDDLE, NULL, dial_step, "DIAL"},
    {TASK_WORK, 3, 0, 0.0F, NULL, start_robot_step, "START ROBOT"},
    {TASK_WORK, 3, 0, 0.0F, NULL, goal_robot_step, "GOAL ROBOT"},
    {TASK_WORK | TASK_FORCE, 3, KIND_RANGE, MIDDLE, NULL, signboard_1_step,
     "SIGNBOARD1"},
    {TASK_WORK | TASK_FORCE, 3, KIND_RANGE, MIDDLE, NULL, signboard_2_step,
     "SIGNBOARD2"},
    {TASK_WORK | TASK_FORCE, 3, KIND_RANGE, MIDDLE, NULL, signboard_3_step,
     "SIGNBOARD3"},
    {TASK_WORK | TASK_FORCE, 3, KIND_RANGE, MIDDLE, NULL, signboard_4_step,
     "SIGNBOARD4"},
    {TASK_WORK | TASK_FORCE, 3, KIND_RANGE, MIDDLE, NULL, signboard_5_step,
     "SIGNBOARD5"},
    {TASK_WORK, 3, KIND_RANGE, MIDDLE, NULL, twinkle_panel_step, "ARROW"},
    {TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA, 3, KIND_RANGE, MIDDLE,
     NULL, cart_jump_panel_step, "O Jump Cart"},
    {TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA, 3, KIND_RANGE, MIDDLE,
     NULL, cart_dash_panel_step, "O Dash Cart"},
    {TASK_MOTION | TASK_WORK | TASK_FORCE | TASK_EXTRA, 3, KIND_RANGE, FAR,
     NULL, enemy_cart_step, "E Cart"},
    {TASK_WORK, 2, 0, 0.0F, NULL, goal_zone_step, "GOAL ZONE"},
    {TASK_WORK, 2, 0, 0.0F, NULL, previous_zone_step, "PREVIOUS ZONE"},
    {TASK_WORK, 2, 0, 0.0F, NULL, next_zone_step, "NEXT ZONE"},
    {TASK_WORK, 3, KIND_RANGE, FARTHER, NULL, arch_1_step, "O ARCH 1"},
    {TASK_WORK, 3, KIND_RANGE, FARTHER, NULL, arch_2_step, "O ARCH 2"},
    {TASK_WORK, 3, KIND_RANGE, FARTHER, NULL, light_1_step, "O LIGHT 1"},
    {TASK_WORK, 3, KIND_RANGE, FARTHER, NULL, light_2_step, "O LIGHT 2"},
    {TASK_WORK, 3, KIND_RANGE, FARTHER, NULL, meteorite_1_step,
     "O METEORITE 1"},
    {TASK_WORK, 3, KIND_RANGE, FARTHER, NULL, meteorite_2_step,
     "O METEORITE 2"},
    {TASK_WORK, 3, KIND_RANGE, FARTHER, NULL, meteorite_3_step,
     "O METEORITE 3"},
    {TASK_WORK, 3, KIND_RANGE, FARTHER, NULL, planet_1_step, "O PLANET 1"},
    {TASK_WORK, 3, KIND_RANGE, FARTHER, NULL, planet_2_step, "O PLANET 2"},
    {TASK_WORK, 3, KIND_RANGE, FARTHER, NULL, planet_3_step, "O PLANET 3"},
    {TASK_WORK, 3, KIND_RANGE, FARTHEST, NULL, pole_step, "O POLE"},
    {TASK_WORK | TASK_EXTRA, 3, KIND_RANGE, NEAR, NULL, twinkle_rocket_step,
     "O ROCKET"},
    {TASK_WORK, 3, KIND_RANGE, NEAR, NULL, satellite_step, "O SATELLITE"},
    {TASK_WORK, 3, KIND_RANGE, FARTHER, NULL, ship_step, "O SHIP"},
    {TASK_WORK, 3, KIND_RANGE, FAR, NULL, saru_start, "E SARU"},
    {TASK_WORK, 3, KIND_RANGE, FAR, NULL, spina_a_step, "SPINA A"},
    {TASK_WORK, 3, KIND_RANGE, FAR, NULL, spina_b_step, "SPINA B"},
    {TASK_WORK, 3, KIND_RANGE, FAR, NULL, spina_c_step, "SPINA C"},
    {TASK_WORK, 3, KIND_RANGE, FAR, NULL, uni_a_start, "E UNI A"},
    {TASK_WORK, 3, KIND_RANGE, FAR, NULL, uni_b_start, "E UNI B"},
    {TASK_WORK, 2, KIND_RANGE, FAR, NULL, invisible_box_step, "C CUBE"},
    {TASK_WORK, 3, KIND_RANGE, FAR, NULL, ring_group_step, "O_GRING"},
};
/* clang-format on */

/* count work->timer down. once it falls below 0, start the streams' task,
 * request the music work->mode names and end the task. the function stays
 * above cart_step. defined below it, the module's code section P builds 32
 * bytes shorter than the image's */
static void cart_music_wait_step(struct task *task)
{
    struct task_work *work = task->work;

    work->timer--;
    if (work->timer < 0) {
        task_create(0, 1, stream_init_step);
        stream_request_music(work->mode);
        task_destroy(task);
    }
}

/* under section HEAD, SHC writes cart_step to section PHEAD, which the
 * module's link places first. 1ST_READ.BIN's stage_steps names it by its
 * address there */
#pragma section HEAD
void cart_step(struct task *task)
{
    task_apply_desc(task, &cart_desc, NULL);
}
#pragma section

static void cart_init(struct task *task, void *arg)
{
    struct cart_record *record = (struct cart_record *)task->motion;
    struct task *music;

    record->mode = CART_START;
    music = task_create(TASK_WORK, 0, cart_music_wait_step);
    music->work->mode = MUSIC_CIRCUIT;
    music->work->timer = MUSIC_WAIT;
    cart_setup();
    world_set_near_radius(2000.0F);
    boost_start();
    cart_run(task);
}

/* start the race, and start it again in the act the act menu picks. while
 * the race runs, each step sets the time back to five minutes */
static void cart_run(struct task *task)
{
    struct cart_record *record = (struct cart_record *)task->motion;
    int by;

    switch (record->mode) {
    case CART_START:
        cart_race_create();
        intro_camera_start();
        boost_start();
        pad_enable();
        pad_disable_player(0);
        pause_enable();
        act_menu_clear();
        record->mode = CART_RUN;
        frame_pacing_init(1, 1);
        break;
    case CART_RUN:
        by = act_menu_choice();
        if (by != 0) {
            cart_race_destroy();
            act_change(by, MUSIC_CIRCUIT);
            record->mode = CART_START;
        }
        time_set_whole(5, 0);
        break;
    }
}

static void cart_destroy(struct task *task)
{
    world_reset_near_radius();
}
