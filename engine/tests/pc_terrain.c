/* Run the shared player cases inside the PC game's address domain. */
#include <ninja.h>

#include "camera/camera.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "player/player.h"
#include "player_terrain.h"
#include "stage/stage.h"
#include "stg00/land.h"
#include "system/frame.h"
#include "system/task.h"

static void install_land(NJS_OBJECT *object, Uint32 flags)
{
    static struct task_work work;
    static struct task land = {.work = &work};
    static struct land_entry entry;

    entry = (struct land_entry){.object = object, .flags = flags};
    practice_land = (struct land_table){.count = 1, .entries = &entry};
    world_compute_bounds();
    work.mode = 0;
    int previous_step = frame_step;
    frame_step = 1; /* Run the land task on a simulation step without a draw. */
    world_land_step(&land);
    frame_step = previous_step;
    collision_mesh_init();
    collision_mesh_update();
}

int pc_terrain_cases(const void *constants)
{
    static NJS_MATRIX stack[16] __attribute__((aligned(8)));
    static struct task_work camera;
    const struct character_constants *characters = constants;
    njInitMatrix(stack, 16, 0);
    njUnitMatrix(NULL);
    camera_work = &camera;
    debug_hidden = -1;
    stage_set_stage_act(STAGE_PRACTICE, 0);
    int failure =
        terrain_crest_case(install_land, &characters[CHARACTER_KNUCKLES]);
    if (failure) {
        return 100 + failure;
    }
    failure = terrain_tread_cases(install_land, &characters[CHARACTER_SONIC]);
    if (failure) {
        return 400 + failure;
    }
    failure = terrain_curb_case(install_land, &characters[CHARACTER_SONIC]);
    if (failure) {
        return 200 + failure;
    }
    failure = terrain_dash_case(install_land, &characters[CHARACTER_SONIC]);
    if (failure) {
        return 450 + failure;
    }
    failure = terrain_roll_case(install_land, characters);
    return failure ? 300 + failure : 0;
}
