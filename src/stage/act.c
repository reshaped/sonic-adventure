/* act changes in place, without ending the stage */
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/world.h"
#include "object/placement.h"
#include "player/reset.h"
#include "sound/cue.h"
#include "stage/act.h"
#include "stage/stage.h"

void act_change(Sint16 by, int music)
{
    music_end();
    world_change_act(by);
    placement_change_act(by);
    camera_change_act(by);
    stage_change_act(by);
    player_place_start(0);
    if (music >= 0) {
        music_start(music);
    }
}
