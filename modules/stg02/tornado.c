/* O TOROUT: the tornado that carries player 0 off at the end of act 0 */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "draw/helpers.h"
#include "draw/model.h"
#include "effect/wind.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg02/bridge.h"
#include "stg02/debris.h"
#include "stg02/tornado.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

extern NJS_OBJECT windy_tornado_object;
extern NJS_OBJECT windy_tornado_object_1;
extern NJS_OBJECT windy_tornado_object_2;

/* the tornado drifts TORNADO_DRIFT a step in x and z, toward its eye,
 * windy_tornado_eye. TORNADO_RANGE is squared. the tornado destroys itself once
 * the camera and player 0 are farther than 1050 away */
#define TORNADO_DRIFT 1.85F
#define TORNADO_RANGE (1050.0F * 1050.0F)

/* each run step, the phase rises PHASE_STEP, and the view rocks up and down
 * by up to ROCK_HEIGHT */
#define PHASE_STEP 4.0F
#define ROCK_HEIGHT 1.5F
/* pad 0 vibrates on each step windy_tornado_rumble passes RUMBLE_WAIT, every
 * 62 steps */
#define RUMBLE_WAIT 60
/* a round of debris starts on each step the timer passes DEBRIS_WAIT, every
 * fourth step. work->tick counts the rounds. on round THROW_ROUND, the
 * tornado throws three bridge sections at itself. on the round the count
 * passes LAST_ROUND, it throws one from itself at its eye. the count then
 * starts again from 0, so each throw comes once in 31 rounds */
#define DEBRIS_WAIT 3
#define LAST_ROUND 30
#define THROW_ROUND 15

/* once player 0 is within CARRY_REACH of the tornado in x and z, the tornado
 * carries it round. the player circles CARRY_RADIUS out, PHASE_STEP degrees a
 * step, and rises CARRY_RISE a step. act 0 ends on the step the carry's count
 * passes CARRY_STEPS, its 302nd */
#define CARRY_REACH 190.0F
#define CARRY_RADIUS 130.0F
#define CARRY_RISE 0.9F
#define CARRY_STEPS 300
/* before that, player 0 is caught within TRAP_REACH of windy_tornado_trap. a
 * caught player is drawn PULL_SPEED a step toward a point off the tornado. the
 * point lies PULL_OFFSET out from the tornado in x and in z. it lies
 * PULL_HEIGHT above the tornado, plus PULL_RISE for each step the player has
 * been drawn */
#define TRAP_REACH 90.0F
#define PULL_SPEED 5.5F
#define PULL_OFFSET 110.0F
#define PULL_HEIGHT 190.0F
#define PULL_RISE 3.8F

#define SE_TORNADO 62

/* an angle that makes one full turn each time the phase advances by period */
#define CYCLE(phase, period) ((Angle)((phase) * (65536.0F / (period))))

/* no step here sets TORNADO_END, in which the tornado destroys its children */
enum tornado_mode { TORNADO_INIT, TORNADO_RUN, TORNADO_END };

/* windy_tornado_carry's stages, in work->submode. no step sets CARRY_DONE, in
 * which the carry does nothing */
enum carry_mode { CARRY_NONE, CARRY_ROUND, CARRY_DONE };

struct tornado_vars {
    /* rises PHASE_STEP a step. the layers' turns, the view's rocking and
     * player 0's place around the tornado follow it */
    Float phase;
    Sint8 caught; /* TRUE once player 0 is caught and drawn in */
    Uint8 unk_05[3];
    Sint16 carried; /* steps player 0 has been carried around */
};

#define VARS(work) ((struct tornado_vars *)&(work)->vars)

static Bool windy_tornado_finished = FALSE;
/* the tornado's work->pos, from its first run step on. nothing sets it back to
 * null */
static NJS_POINT3 *windy_tornado_pos = NULL;
/* the steps since the tornado last vibrated pad 0 */
static int windy_tornado_rumble = 0;
/* how far the point player 0 is drawn toward has risen */
static Float windy_tornado_rise = 0.0F;

static void windy_tornado_destroy(struct task *task);
static void windy_tornado_draw(struct task *task);
static void windy_tornado_carry(struct task *task);

/* the first step sets the wind blowing, and starts the tornado's sound and
 * its spiral of bridge pieces. each run step drifts the tornado toward its
 * eye and rocks the view. it vibrates pad 0 now and then. it starts rounds of
 * debris. a round is two pieces that fly into the tornado. one starts near
 * the camera, while there is a camera, and one at a fixed point. the step
 * then draws the tornado and carries player 0 */
void windy_tornado_step(struct task *task)
{
    static const NJS_POINT3 windy_tornado_eye = {3254.0F, -421.0F, -1665.0F};
    static const NJS_VECTOR rock_still = {0.0F, 0.0F, 0.0F};
    struct task_work *work = task->work;
    struct task *debris;
    struct task_work *piece;
    NJS_POINT3 eye;
    NJS_POINT3 drift;
    NJS_VECTOR rock;
    Float offset;

    eye = windy_tornado_eye;
    if (placement_try_leave_by(task, TORNADO_RANGE)) {
        return;
    }
    switch (work->mode) {
    case TORNADO_INIT:
        task->destructor = windy_tornado_destroy;
        task->draw = windy_tornado_draw;
        work->mode = TORNADO_RUN;
        windy_tornado_finished = FALSE;
        task_create_child(TASK_WORK, windy_tornado_spiral_step, task);
        wind_set(2, NULL);
        sound_play_held(SE_TORNADO, (int)task, 1, 0);
        break;
    case TORNADO_RUN:
        rock = rock_still;
        rock.y = ROCK_HEIGHT * njSin(CYCLE(VARS(work)->phase, 18.0F));
        camera_add_offset(&rock);
        task_step_children(task);
        point_toward(&work->pos, &eye, TORNADO_DRIFT, &drift);
        work->pos.x = drift.x;
        work->pos.z = drift.z;
        windy_tornado_pos = &work->pos;
        VARS(work)->phase += PHASE_STEP;
        work->timer++;
        if (windy_tornado_rumble++ > RUMBLE_WAIT) {
            vibration_start_fading(0, 4, 59, 6);
            windy_tornado_rumble = 0;
        }
        if (TASK_TIMER(work) > DEBRIS_WAIT) {
            work->timer = 0;
            if ((debris = task_create(TASK_WORK, 5,
                                      windy_tornado_debris_step)) != NULL) {
                piece = debris->work;
                if (camera_work != NULL) {
                    offset = njSin((Angle)(rand() * RAND_SCALE * 360.0F *
                                           ANGLE_PER_DEGREE)) *
                             90.0F;
                    piece->pos.x = camera_work->pos.x + offset;
                    piece->pos.y = camera_work->pos.y + offset;
                    piece->pos.z = eye.z + offset;
                    /* @bug logic: the test never holds, since rand() is
                     * never negative (FCMP/GT at 0x0c900e0e). so no piece's
                     * scl.z is 1, which would draw the sai, the rhino enemy,
                     * in its place */
                    if (rand() * RAND_SCALE < 0.0F) {
                        piece->scl.z = 1.0F;
                    }
                }
            }
            if ((debris = task_create(TASK_WORK, 5,
                                      windy_tornado_debris_step)) != NULL) {
                piece = debris->work;
                piece->pos.x = 3592.0F;
                piece->pos.y = RANDOM(-400.0F, -350.0F);
                piece->pos.z = -1402.0F;
                /* @bug logic: like the test above, this one never holds
                 * (FCMP/GT at 0x0c900e5a) */
                if (rand() * RAND_SCALE < 0.0F) {
                    piece->scl.z = 1.0F;
                }
            }
            work->tick++;
            if (work->tick > LAST_ROUND) {
                work->tick = 0;
                if ((debris = task_create(TASK_WORK, 5,
                                          windy_bridge_debris_step)) != NULL) {
                    piece = debris->work;
                    DEBRIS(piece)->reach = 100.0F;
                    piece->pos.x = work->pos.x;
                    piece->pos.y = -379.0F;
                    piece->pos.z = work->pos.z;
                    piece->scl.x = eye.x;
                    piece->scl.y = eye.y;
                    piece->scl.z = eye.z;
                    DEBRIS(piece)->still = TRUE;
                }
            }
            if (work->tick == THROW_ROUND) {
                if ((debris = task_create(TASK_WORK, 5,
                                          windy_bridge_debris_step)) != NULL) {
                    piece = debris->work;
                    DEBRIS(piece)->reach = 150.0F;
                    piece->pos.x = eye.x;
                    piece->pos.y = eye.y;
                    piece->pos.z = eye.z;
                    piece->scl.x = work->pos.x;
                    piece->scl.y = -420.0F;
                    piece->scl.z = work->pos.z;
                }
                if ((debris = task_create(TASK_WORK, 5,
                                          windy_bridge_debris_step)) != NULL) {
                    piece = debris->work;
                    DEBRIS(piece)->reach = 120.0F;
                    piece->pos.x = 3592.0F;
                    piece->pos.y = -400.0F;
                    piece->pos.z = -1402.0F;
                    piece->scl.x = work->pos.x;
                    piece->scl.y = work->pos.y + 220.0F;
                    piece->scl.z = work->pos.z;
                }
                if ((debris = task_create(TASK_WORK, 5,
                                          windy_bridge_debris_step)) != NULL) {
                    piece = debris->work;
                    DEBRIS(piece)->reach = 120.0F;
                    piece->pos.x = 3262.0F;
                    piece->pos.y = -409.0F;
                    piece->pos.z = -1932.0F;
                    piece->scl.x = work->pos.x;
                    piece->scl.y = work->pos.y + 220.0F;
                    piece->scl.z = work->pos.z;
                }
            }
        }
        windy_tornado_draw(task);
        windy_tornado_carry(task);
        break;
    case TORNADO_END:
        task_destroy_children(task);
        break;
    }
}

static void windy_tornado_destroy(struct task *task)
{
    sound_stop(SE_TORNADO);
    vibration_start(0, 0, 0, 0);
    task_destroy_children(task);
}

static void windy_tornado_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *layer1 = &windy_tornado_object_1;
    NJS_OBJECT *layer2 = &windy_tornado_object_2;
    NJS_OBJECT *layer0 = &windy_tornado_object;
    Angle ang;

    texture_set_stage();
    njFogDisable();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, -CYCLE(VARS(work)->phase, 120.0F), ang);
    draw_model(layer0->model);
    njPushMatrix(NULL);
    njTranslate(NULL, layer1->pos[0], layer1->pos[1], layer1->pos[2]);
    TURN_VIA(njRotateY, -CYCLE(VARS(work)->phase, 180.0F), ang);
    draw_model(layer1->model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, layer2->pos[0], layer2->pos[1], layer2->pos[2]);
    TURN_VIA(njRotateY, -CYCLE(VARS(work)->phase, 360.0F), ang);
    draw_model(layer2->model);
    njPopMatrix(1);
    njPopMatrix(1);
    njFogEnable();
}

Bool windy_tornado_distance(NJS_POINT3 *pos, Float *distance)
{
    NJS_POINT3 *at;

    *distance = 0.0F;
    at = windy_tornado_pos;
    if (at != NULL) {
        *distance = njSqrt(SQUARE(at->x - pos->x) + SQUARE(at->z - pos->z));
        return TRUE;
    }
    return FALSE;
}

Bool windy_tornado_get_pos(NJS_POINT3 *pos)
{
    if (windy_tornado_pos != NULL) {
        pos->x = windy_tornado_pos->x;
        pos->y = windy_tornado_pos->y;
        pos->z = windy_tornado_pos->z;
        return TRUE;
    }
    return FALSE;
}

/* carry player 0 round the tornado once it comes within CARRY_REACH. until
 * then, draw player 0 in once it is caught */
static void windy_tornado_carry(struct task *task)
{
    static const NJS_POINT3 windy_tornado_trap = {3181.0F, -341.0F, -1589.0F};
    struct task_work *work = task->work;
    struct task_work *player;
    NJS_POINT3 at;
    NJS_POINT3 pos;
    NJS_POINT3 trap;
    Float across;
    Float distance;

    trap = windy_tornado_trap;
    switch (work->submode) {
    case CARRY_NONE:
        if (windy_tornado_get_pos(&at)) {
            pos = player_work[0]->pos;
            if (windy_tornado_distance(&pos, &across)) {
                distance = njDistanceP2P(&trap, &pos);
                /* @hack spelling: caught compared with TRUE in the else if,
                 * CMP/EQ #1 at 0x0c90132a. caught starts 0 in the zeroed work,
                 * and only TRUE is stored to it, so a plain test of it is the
                 * same: as that test, 2 of 229 words differ */
                if (across < CARRY_REACH) {
                    VARS(work)->phase = 90.0F;
                    work->submode = CARRY_ROUND;
                } else if (distance < TRAP_REACH ||
                           VARS(work)->caught == TRUE) {
                    player_set_mode(0, PLAYER_MODE_STILL);
                    at.x += PULL_OFFSET;
                    at.z += PULL_OFFSET;
                    at.y += PULL_HEIGHT;
                    windy_tornado_rise += PULL_RISE;
                    at.y += windy_tornado_rise;
                    point_toward(&player_work[0]->pos, &at, PULL_SPEED,
                                 &player_work[0]->pos);
                    player_set_mode(0, PLAYER_MODE_RESET);
                    player_set_speed(0, 0.0F, 0.0F, 0.0F);
                    player_updraft(0, 0.0F, 0.0F, 0.0F);
                    VARS(work)->caught = TRUE;
                }
            }
        }
        break;
    case CARRY_ROUND:
        windy_tornado_rise = 0.0F;
        if (windy_tornado_get_pos(&at)) {
            player_set_mode(0, PLAYER_MODE_STILL);
            player = player_work[0];
            player->pos.x = at.x + (CARRY_RADIUS *
                                    njCos(-CYCLE(VARS(work)->phase, 360.0F)));
            player->pos.z = at.z + (CARRY_RADIUS *
                                    njSin(-CYCLE(VARS(work)->phase, 360.0F)));
            player->pos.y += CARRY_RISE;
            player_set_mode(0, PLAYER_MODE_RESET);
            player_updraft(0, 0.0F, 0.092F, 0.0F);
            if (VARS(work)->carried++ > CARRY_STEPS) {
                windy_tornado_finished = TRUE;
            }
        }
        break;
    case CARRY_DONE:
        break;
    }
}

Bool windy_tornado_done(void)
{
    /* @hack spelling: the test's result returned as TRUE or FALSE through an
     * if, not returned itself, which SHC writes as CMP/EQ #1 and MOVT R0 at
     * 0x0c9014a8 and 0x0c9014aa; as a return of the test, 3 of 6 words
     * differ */
    if (windy_tornado_finished == TRUE) {
        return TRUE;
    }
    return FALSE;
}
