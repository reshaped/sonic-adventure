/* a spotlight that follows above player 0 and shines back down at it. it
 * tilts with the player */
#include <ninja.h>
#include <sg_xpt.h>

#include "player/player.h"
#include "stage/stage.h"
#include "stg04/spotlight.h"
#include "system/task.h"
#include "system/util.h"

/* the light's modes, in its work->mode. no code sets SPOTLIGHT_DELETE */
enum spotlight_mode { SPOTLIGHT_INIT, SPOTLIGHT_FOLLOW, SPOTLIGHT_DELETE };

static void spotlight_start(struct task_work *work);
static void spotlight_follow(struct task_work *work);
static void spotlight_destroy(struct task *task);

static NJS_LIGHT spotlight;

void spotlight_step(struct task *task)
{
    struct task_work *work = task->work;
    int act = (Uint8)stage_get_stage_act();

    if (act != 0) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case SPOTLIGHT_INIT:
        spotlight_start(work);
        task->destructor = spotlight_destroy;
        break;
    case SPOTLIGHT_FOLLOW:
        spotlight_follow(work);
        break;
    case SPOTLIGHT_DELETE:
        njDeleteLight(&spotlight);
        /* @hack redundant: a goto where the case would fall into the default.
         * it puts the default's store and the call after the switch in R3 (MOV
         * #0,R3 at 0x0c90004c); without it, they take R2, and 4 of 48 words
         * differ */
        goto reset;
    default:
    reset:
        work->mode = SPOTLIGHT_INIT;
        break;
    }
    njLightOn(&spotlight);
    /* @hack redundant: a goto to a return at the end, which keeps the image's
     * call of njLightOn by JSR @R3 at 0x0c900054, and its RTS after; without
     * it, SHC jumps to njLightOn by JMP @R3, and 8 of 48 words differ */
    goto done;
done:
    return;
}

/* make the light, and give the task player 0's position and angles.
 * without player 0, the mode goes back to SPOTLIGHT_INIT, and the next step
 * makes the light again */
static void spotlight_start(struct task_work *work)
{
    struct task_work *player = player_work[0];

    work->mode = SPOTLIGHT_FOLLOW;
    njCreateLight(&spotlight, NJD_SPOT_LIGHT);
    njSetLightRange(&spotlight, 100.0F, 300.0F);
    njSetLightColor(&spotlight, 0.8F, 1.0F, 1.0F);
    njSetLightAngle(&spotlight, NJM_DEG_ANG(1), NJM_DEG_ANG(90));
    njSetLightIntensity(&spotlight, 0.0F, 1.0F, 1.0F);
    njSetLightAlpha(&spotlight, 0.5F);
    if (player) {
        work->pos = player->pos;
        work->ang = player->ang;
    } else {
        work->mode = SPOTLIGHT_INIT;
    }
}

/* move the light to an offset from player 0, and aim it back at the
 * player. the offset is 30 up and 10 along -x. the player's angles about z
 * and x turn it, but not its angle about y */
static void spotlight_follow(struct task_work *work)
{
    struct task_work *player = player_work[0];
    NJS_VECTOR dir;
    /* @hack redundant: down, set and never read. its initializer keeps the
     * image's copy of {0, -1, 0} from the constants into the frame at
     * R15+40 (0x0c90017a); without it, the constants run 12 bytes short and
     * spotlight_follow 32, and 113 of its 133 words differ */
    NJS_VECTOR down = {0.0F, -1.0F, 0.0F};
    NJS_POINT3 at;
    NJS_POINT3 offset = {-10.0F, 30.0F, 0.0F};
    /* @hack redundant: origin, set and never read. its initializer keeps
     * the image's copy of {0, 0, 0} from the constants into the frame at
     * R15+4 (0x0c90019c); without it, the constants run 12 bytes short and
     * spotlight_follow 32, and 113 of its 133 words differ */
    NJS_POINT3 origin = {0.0F, 0.0F, 0.0F};
    Angle ang;

    njPushMatrix(&_unit_matrix_);
    TURN_VIA(njRotateZ, player->ang.z, ang);
    TURN_VIA(njRotateX, player->ang.x, ang);
    njCalcPoint(NULL, &offset, &at);
    njPopMatrix(1);
    work->pos.x = player->pos.x + at.x;
    work->pos.y = player->pos.y + at.y;
    work->pos.z = player->pos.z + at.z;
    dir.x = player->pos.x - work->pos.x;
    dir.y = player->pos.y - work->pos.y;
    dir.z = player->pos.z - work->pos.z;
    njUnitVector(&dir);
    njSetLightAngle(&spotlight, NJM_DEG_ANG(10), NJM_DEG_ANG(90));
    njSetLightLocation(&spotlight, work->pos.x, work->pos.y, work->pos.z);
    njSetLightDirection(&spotlight, dir.x, dir.y, dir.z);
}

static void spotlight_destroy(struct task *task)
{
    njDeleteLight(&spotlight);
}
