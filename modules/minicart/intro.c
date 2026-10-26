/* the cart's intro: the camera flies a curve through the act's keys, then
 * the race's lights start */
#include "minicart/intro.h"
#include "camera/camera.h"
#include "camera/cart.h"
#include "minicart/cart.h"
#include "minicart/cart_debug.h"
#include "minicart/race.h"
#include "stage/stage.h"
#include "stg03/cart_routes.h"
#include "system/task.h"

static void intro_wait_step(struct task *task);

/* how many of camera_demo_keys' keys each act's path takes */
static int intro_key_counts[CART_ACTS] = {11, 11, 11, 11, 11, 11};

void intro_camera_start(void)
{
    int act = stage_get_stage_act() & 0xff;
    int records;

    task_create(TASK_WORK, 3, intro_wait_step);
    camera_override_mode(CAMERA_MODE_CART, 0);
    records = intro_path_make(camera_demo_keys[act], intro_key_counts[act]);
    camera_cart_action = CAMERA_CART_PLAY;
    /* the camera plays intro_path from record INTRO_STEPS through record
     * records - 2 * INTRO_STEPS. last counts from the first record it
     * plays */
    camera_place.args[0].keys = &intro_path[INTRO_STEPS];
    camera_place.args[1].last = records - (3 * INTRO_STEPS);
}

/* once the camera has played the path and cleared camera_cart_action,
 * start the race's lights and end */
static void intro_wait_step(struct task *task)
{
    if (camera_cart_action == 0) {
        task_kill(task);
        cart_race_start_lights();
    }
}
