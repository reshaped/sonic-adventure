/* the knock-back a cannon's shell gives player 0 as it hits the target player 0
 * stands on */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg06/knock_back.h"
#include "system/task.h"
#include "system/util.h"

/* player 0's speed ahead and up, and the steps its stick is ignored */
#define KNOCK_AHEAD 3.5F
#define KNOCK_UP 2.5F
#define KNOCK_STEPS 60
/* in act 0, player 0 goes along +z while the nearest player's z is below
 * this */
#define ACT0_STRAIGHT_Z 2000.0F
/* how far the nearest player rises */
#define KNOCK_LIFT 1.0F
/* the sound when the character is Sonic, and when it is any other */
#define SE_KNOCK_BACK_SONIC 0x459
#define SE_KNOCK_BACK 0x469

void sky_deck_knock_back(struct task_work *work)
{
    int number = player_nearest(&work->pos);
    struct task_work *nearest = player_work[number];
    NJS_VECTOR delta;
    NJS_VECTOR velocity;
    Angle rotation[3];

    delta.x = nearest->pos.x - work->pos.x;
    delta.y = 0.0F;
    delta.z = nearest->pos.z - work->pos.z;
    rotation[0] = 0;
    rotation[1] = (Angle)(atan2f(delta.x, delta.z) * ANGLE_PER_RADIAN);
    rotation[2] = 0;
    velocity.x = KNOCK_AHEAD;
    velocity.y = KNOCK_UP;
    velocity.z = 0.0F;
    if ((Uint8)stage_get_stage_act() == 0 && nearest->pos.z < ACT0_STRAIGHT_Z) {
        rotation[1] = 0;
    }
    player_send_flying(0, &velocity, rotation, KNOCK_STEPS);
    if (get_character() == CHARACTER_SONIC) {
        sound_play(SE_KNOCK_BACK_SONIC, 0, 0, 0);
    } else {
        sound_play(SE_KNOCK_BACK, 0, 0, 0);
    }
    nearest->pos.y += KNOCK_LIFT;
}
