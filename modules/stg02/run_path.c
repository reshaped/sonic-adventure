/* a path's step that catches players at a sphere and runs them along the
 * path */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "path/path.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg02/run_path.h"
#include "system/task.h"
#include "system/util.h"

/* work->vars, laid over struct path_vars. work->tick holds a bit for each
 * player the path has caught */
struct run_vars {
    Uint16 distance[2]; /* how far along the path players 0 and 1 are */
    Sint16 stage_act; /* the stage and act the path was made for */
    Sint16 released; /* a bit for each player the path has let go */
    struct path *path;
};

#define VARS(work) ((struct run_vars *)&(work)->vars)

/* the modes, 0 and 2 as the image stores them */
enum run_mode { RUN_START = 0, RUN_WAIT = 2 };

/* where the first step puts the task, and with it the sphere that catches
 * players */
#define RUN_X 5281.0F
#define RUN_Y (-6748.0F)
#define RUN_Z 881.0F

/* a caught player moves RUN_SPEED along the path each step. the path lets it
 * go once it passes RUN_END_MARGIN short of the end. it then has a speed of
 * RUN_KICK up and along its heading */
#define RUN_SPEED 10
#define RUN_END_MARGIN 10.0F
#define RUN_KICK 1.0F

/* a player farther than 200 from its point on the path has strayed. the
 * value is squared */
#define RUN_STRAY (200.0F * 200.0F)

/* @hack type: an array of one shape, not one shape, which loads
 * collision_init's 1 at its call (MOV #1,R3 at 0x0c90cd96); as one shape
 * passed by its address, SHC keeps the 1 in R12 from the step's start, and
 * 139 of 215 words differ */
static struct collision_shape windy_run_path_shapes[1] = {
    {SHAPE_SPHERE,
     0,
     PUSH_FIRM(PUSH_PLAYERS),
     {0.0F, 0.0F, 0.0F},
     20.0F,
     5.0F,
     10.0F,
     {0, 0, 0},
     0,
     NULL},
};

static Bool windy_run_path_place(struct path *path, Float distance,
                                 struct task_work *work, int player);

/* a player that touches the sphere is held in PLAYER_MODE_STILL_RUN. the path
 * carries it until it lets it go near the end. a player that strays
 * from the path isn't moved, and stays caught. a stray also ends the step's
 * loop over the players. so when player 0 strays, player 1 is neither caught
 * nor moved that step. the task kills itself once the stage or act
 * changes */
void windy_run_path_step(struct task *task)
{
    struct task_work *work = task->work;
    int player;
    struct task *rider;
    Angle ang;

    if (stage_get_stage_act() != VARS(work)->stage_act) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case RUN_START:
        work->mode = RUN_WAIT;
        work->timer = 0;
        work->scl.x = 0.0F;
        work->pos.x = RUN_X;
        work->pos.y = RUN_Y;
        work->pos.z = RUN_Z;
        VARS(work)->distance[0] = 0;
        VARS(work)->distance[1] = 0;
        VARS(work)->released = 0;
        collision_init(task, windy_run_path_shapes, COLLISION_OBJECT, 1, 1);
        work->scl.y = VARS(work)->path->length - RUN_END_MARGIN;
        break;
    case RUN_WAIT:
        for (player = 0; player < 2; player++) {
            if (!(work->tick & (1 << player))) {
                if (work->collision->flags & COLLISION_HIT) {
                    rider = player_get_task(player);
                    if (work->collision->hit->owner == rider) {
                        player_set_mode(player, PLAYER_MODE_STILL_RUN);
                        work->tick |= 1 << player;
                    }
                }
            } else if (!(VARS(work)->released & (1 << player))) {
                VARS(work)->distance[player] += RUN_SPEED;
                if (!windy_run_path_place(VARS(work)->path,
                                          VARS(work)->distance[player], work,
                                          player)) {
                    /* the bit is clear already: the test above found it so */
                    VARS(work)->released &= ~(1 << player);
                    break;
                }
                if (VARS(work)->distance[player] > work->scl.y) {
                    player_set_mode(player, PLAYER_MODE_RESET);
                    VARS(work)->released |= 1 << player;
                    rider = player_get_task(player);
                    if (rider != NULL) {
                        ang = rider->work->ang.y;
                        player_set_speed(player, njCos(ang) * RUN_KICK,
                                         RUN_KICK, -(njSin(ang) * RUN_KICK));
                    }
                }
            }
        }
        collision_register(work);
        break;
    }
}

/* put the player at distance along the path, turned to run along it, and
 * return TRUE. return FALSE, leaving the player, when it is farther than 200
 * from that point. a distance off the path leaves the player too, and
 * returns TRUE. work goes unread */
static Bool windy_run_path_place(struct path *path, Float distance,
                                 struct task_work *work, int player)
{
    struct path_sample sample;
    NJS_POINT3 pos;
    struct angles turn;
    NJS_VECTOR ahead;
    struct task_work *rider;

    sample.distance = distance;
    if (path_sample(path, &sample)) {
        pos = sample.pos_copy;
        rider = player_work[player];
        ahead.x = rider->pos.x - pos.x;
        ahead.y = rider->pos.y - pos.y;
        ahead.z = rider->pos.z - pos.z;
        if (njScalor2(&ahead) > RUN_STRAY) {
            return FALSE;
        }
        turn.x = sample.smooth_x;
        turn.z = sample.smooth_z;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateX, -turn.x);
        TURN(njRotateZ, -turn.z);
        njCalcVector(NULL, &sample.ahead, &ahead);
        njPopMatrix(1);
        turn.y = (Angle)(atan2f(ahead.x, ahead.z) * ANGLE_PER_RADIAN);
        rider->pos = pos;
        rider->ang.x = turn.x;
        rider->ang.y = -turn.y + 0x4000;
        rider->ang.z = turn.z;
    }
    return TRUE;
}
