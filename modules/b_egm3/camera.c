/* camera mode EGM3, the Egg Viper fight's camera. each step runs the phase
 * that matches the boss's work mode. while player 0 has fallen below the
 * floor, it runs camera_fallen instead. most phases clear
 * egg_viper_camera_next as they start. each time the boss sets it, such a
 * phase moves on to its next subphase. the boss can also ask for a shake,
 * which lifts and lowers the view */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_egm3/camera.h"
#include "b_egm3/egg_viper.h"
#include "b_egm3/eggmobile3.h"
#include "camera/camera.h"
#include "player/player.h"
#include "system/task.h"

/* the camera's phases, named as camera_phase_names spells them. the boss's
 * work mode is the phase, so they keep the order of egg_viper.c's
 * viper_mode */
enum camera_phase {
    C_INIT,
    C_WAIT,
    C_LASER1,
    C_LASER2,
    C_LASER3,
    C_ENERGY,
    C_BOOMP,
    C_BOOM,
    C_HAKAID,
    C_HAKAI,
    C_BREAK,
    C_MAX, /* while player 0 has fallen. VIPER_MAX shares its number */
    CAMERA_PHASES
};

/* at or below FALLEN_Y, player 0 has fallen from the floor. camera_fallen
 * keeps the camera no lower than LOWEST_Y */
#define FALLEN_Y (-50.0F)
#define LOWEST_Y (-100.0F)

/* the camera stands BESIDE_X past player 0 in x, beside him or out wide.
 * in line with the boss or out wide, it stays within Z_REACH of him in z */
#define BESIDE_X 60.0F
#define Z_REACH 200.0F

/* camera_look_along holds its target's x to this band, on the boss's side
 * of x 0 */
#define ROW_INNER_X 236.0F
#define ROW_OUTER_X 283.99999F

/* how far a shake lifts the view, pos and target alike, on each of its
 * steps. the 0 ends the shake */
static Float shake_heights[] = {
    10.0F, 10.0F, -10.0F, -10.0F, 10.0F, 10.0F, -9.0F, -9.0F, 9.0F,
    9.0F,  -8.0F, -8.0F,  7.0F,   7.0F,  -6.0F, -6.0F, 5.0F,  5.0F,
    -4.0F, -4.0F, 3.0F,   3.0F,   -2.0F, -2.0F, 1.0F,  1.0F,  0.0F,
};

static int camera_last_phase; /* the phase the last step ran */
static int camera_subphase; /* where the phase is: the case its switch runs */
static int camera_shaking; /* 1 while a shake runs, else 0 */
static Bool camera_shake_asked; /* TRUE from an ask until its shake starts */
/* the half of the arena the camera works from: 0 below 0 in x or z, else 1.
 * camera_set_side_x and camera_set_side_z set it */
static int camera_side;
/* nothing reads or writes it */
static int dat_0c95b3a4;
static Float camera_boss_z; /* the boss's z as camera_pick_side took it */
/* the view as the phase left it, before the shake lifted it */
static Float steady_pos_x;
static Float steady_pos_y;
static Float steady_pos_z;
static Float steady_target_x;
static Float steady_target_y;
static Float steady_target_z;
static int camera_shake_steps; /* the shake's index in shake_heights */

static void camera_wait(struct task_work *player, struct task_work *boss);
static void camera_laser1(struct task_work *player, struct task_work *boss);
static void camera_laser2(struct task_work *player, struct task_work *boss);
static void camera_laser3(struct task_work *player, struct task_work *boss);
static void camera_energy(struct task_work *player, struct task_work *boss);
static void camera_boomp(struct task_work *player, struct task_work *boss);
static void camera_boom(struct task_work *player, struct task_work *boss);
static void camera_hakaid(struct task_work *player, struct task_work *boss);
static void camera_hakai(struct task_work *player, struct task_work *boss);
static void camera_break(struct task_work *player, struct task_work *boss);
static void camera_fallen(struct task_work *player, struct task_work *boss);
static Bool camera_go_beside(struct task_work *player, struct task_work *boss);
static Bool camera_go_beside_boss(struct task_work *player,
                                  struct task_work *boss);
static Bool camera_go_beside_boss_low(struct task_work *player,
                                      struct task_work *boss);
static Bool camera_go_behind(struct task_work *player, struct task_work *boss);
static Bool camera_go_above(struct task_work *player, struct task_work *boss);
static void camera_pick_side(struct task_work *player, struct task_work *boss);
static Bool camera_go_wide(struct task_work *player, struct task_work *boss);
static Bool camera_look_low(struct task_work *player, struct task_work *boss);
static Bool camera_look_high(struct task_work *player, struct task_work *boss);
static Bool camera_look_at_boss(struct task_work *player,
                                struct task_work *boss);
static Bool camera_go_past_boss(struct task_work *player,
                                struct task_work *boss);
static Bool camera_look_along(struct task_work *player, struct task_work *boss);
static Bool camera_turn_at(NJS_POINT3 *target, Float speed);
static Bool camera_move(NJS_POINT3 *pos);
static Bool camera_turn(NJS_POINT3 *target);
static Bool camera_look(NJS_POINT3 *target);
static void camera_restore_view(void);
static void camera_save_view(void);

/* #pragma section HEAD has SHC put the two tables below in DHEAD and the
 * names' strings in CHEAD. it puts eggmobile3_camera_step in PHEAD, which
 * opens the module. 1ST_READ.BIN's camera_mode_list names that step by its
 * address there */
#pragma section HEAD
/* each phase's step, or NULL for none */
static void (*camera_phases[CAMERA_PHASES])(struct task_work *player,
                                            struct task_work *boss) = {
    NULL,          camera_wait,   camera_laser1, camera_laser2,
    camera_laser3, camera_energy, camera_boomp,  camera_boom,
    camera_hakaid, camera_hakai,  camera_break,  camera_fallen,
};
/* the phases' names, which nothing reads */
static char *camera_phase_names[CAMERA_PHASES] = {
    "C_INIT",  "C_WAIT", "C_LASER1", "C_LASER2", "C_LASER3", "C_ENERGY",
    "C_BOOMP", "C_BOOM", "C_HAKAID", "C_HAKAI",  "C_BREAK",  "C_MAX",
};

void eggmobile3_camera_step(struct camera_view *view)
{
    struct task_work *player = player_work[0];
    struct task_work *boss = egg_viper_task->work;

    if (player->pos.y > FALLEN_Y) {
        camera_state->phase = boss->mode;
    } else {
        camera_state->phase = C_MAX;
    }
    if (camera_last_phase != camera_state->phase) {
        camera_last_phase = camera_state->phase;
        camera_subphase = 0;
    }
    camera_restore_view();
    if (camera_phases[camera_state->phase] != NULL) {
        camera_phases[camera_state->phase](player, boss);
    }
    camera_save_view();
}
#pragma section

/* set camera_side by work's x, or by its z */
/* @hack spelling: the two settings in helpers, which SHC expands where they
 * are called. in camera_laser3, camera_energy and camera_fallen, the
 * expansion first copies its argument to a register of its own, as MOV
 * R13,R4 at 0x0c909418 does. written in place, 101 of the 213 words of those
 * three and camera_pick_side differ */
static void camera_set_side_x(struct task_work *work)
{
    camera_side = work->pos.x < 0.0F ? 0 : 1;
}

static void camera_set_side_z(struct task_work *work)
{
    camera_side = work->pos.z < 0.0F ? 0 : 1;
}

void egg_viper_camera_reset(void)
{
    camera_last_phase = 0;
    camera_subphase = 0;
    camera_shaking = 0;
    camera_shake_asked = FALSE;
}

/* from 200 off player 0 in x and 50 above him, the camera closes in. it
 * ends low beside him, in line with the boss */
static void camera_wait(struct task_work *player, struct task_work *boss)
{
    switch (camera_subphase) {
    case 0:
        camera_state->pos.x = player_work[0]->pos.x + 200.0F;
        camera_state->pos.y = player_work[0]->pos.y + 50.0F;
        camera_state->pos.z = player_work[0]->pos.z + 0.0F;
        camera_state->target.x = player_work[0]->pos.x;
        camera_state->target.y = player_work[0]->pos.y;
        camera_state->target.z = player_work[0]->pos.z;
        camera_subphase++;
    case 1:
        camera_go_beside_boss_low(player, boss);
        camera_look_low(player, boss);
        break;
    }
}

/* the camera goes beside player 0. once there, it sets
 * egg_viper_camera_ready, which VIPER_LASER1 waits for */
static void camera_laser1(struct task_work *player, struct task_work *boss)
{
    switch (camera_subphase) {
    case 0:
        egg_viper_camera_ready = FALSE;
        camera_subphase++;
    case 1:
        if (camera_go_beside(player, boss)) {
            egg_viper_camera_ready = TRUE;
        }
        camera_look_low(player, boss);
        break;
    }
}

/* the camera goes beside player 0, then in line with the boss, then beside
 * him again */
static void camera_laser2(struct task_work *player, struct task_work *boss)
{
    switch (camera_subphase) {
    case 0:
        egg_viper_camera_next = FALSE;
        camera_subphase++;
    case 1:
        camera_go_beside(player, boss);
        camera_look_low(player, boss);
        if (egg_viper_camera_next) {
            egg_viper_camera_next = FALSE;
            camera_subphase++;
        }
        break;
    case 2:
        camera_go_beside_boss(player, boss);
        camera_look_low(player, boss);
        if (egg_viper_camera_next) {
            egg_viper_camera_next = FALSE;
            camera_subphase++;
        }
        break;
    case 3:
        camera_go_beside(player, boss);
        camera_look_low(player, boss);
        break;
    }
}

/* the camera goes beside player 0, then behind him in z, away from the
 * boss's half */
static void camera_laser3(struct task_work *player, struct task_work *boss)
{
    switch (camera_subphase) {
    case 0:
        egg_viper_camera_next = FALSE;
        camera_subphase++;
    case 1:
        camera_go_beside(player, boss);
        camera_look_low(player, boss);
        if (egg_viper_camera_next) {
            egg_viper_camera_next = FALSE;
            camera_subphase++;
        }
        break;
    case 2:
        camera_set_side_z(boss);
        camera_subphase++;
    case 3:
        camera_go_behind(player, boss);
        camera_look_low(player, boss);
        break;
    }
}

/* the camera waits a step, then goes out wide past player 0. from there,
 * he stands between it and the point at x 150 across x 0 from him. that
 * point lies at the z the boss had then. then the camera goes above him,
 * turning to him */
static void camera_energy(struct task_work *player, struct task_work *boss)
{
    switch (camera_subphase) {
    case 0:
        egg_viper_camera_next = FALSE;
        camera_subphase++;
    case 1:
        camera_subphase++;
        break;
    case 2:
        /* @hack spelling: the call through a comma, which SHC doesn't
         * expand. as a plain call, SHC expands camera_pick_side here,
         * camera_energy runs 16 bytes long, and 83 of its 85 words differ */
        /* NOLINTNEXTLINE(clang-diagnostic-unused-value) */
        (0, camera_pick_side)(player, boss);
        camera_subphase++;
    case 3:
        camera_go_wide(player, boss);
        camera_look_high(player, boss);
        if (egg_viper_camera_next) {
            egg_viper_camera_next = FALSE;
            camera_subphase++;
        }
        break;
    case 4:
        camera_set_side_x(player);
        camera_subphase++;
    case 5:
        camera_go_above(player, boss);
        camera_turn(&player->pos);
        break;
    }
}

/* the camera goes beside player 0, then past the boss in z, looking along
 * its row */
static void camera_boomp(struct task_work *player, struct task_work *boss)
{
    switch (camera_subphase) {
    case 0:
        egg_viper_camera_next = FALSE;
        camera_subphase++;
    case 1:
        camera_go_beside(player, boss);
        camera_look_low(player, boss);
        if (egg_viper_camera_next) {
            egg_viper_camera_next = FALSE;
            camera_subphase++;
        }
        break;
    case 2:
        camera_go_past_boss(player, boss);
        camera_look_along(player, boss);
        break;
    }
}

/* the camera goes past the boss in z, looking along its row, then beside
 * player 0 */
static void camera_boom(struct task_work *player, struct task_work *boss)
{
    switch (camera_subphase) {
    case 0:
        egg_viper_camera_next = FALSE;
        camera_subphase++;
    case 1:
        camera_go_past_boss(player, boss);
        camera_look_along(player, boss);
        if (egg_viper_camera_next) {
            egg_viper_camera_next = FALSE;
            camera_subphase++;
        }
        break;
    case 2:
        camera_go_beside(player, boss);
        camera_look_low(player, boss);
        break;
    }
}

/* the camera goes beside player 0, then in line with the boss */
static void camera_hakaid(struct task_work *player, struct task_work *boss)
{
    switch (camera_subphase) {
    case 0:
        egg_viper_camera_next = FALSE;
        camera_subphase++;
    case 1:
        camera_go_beside(player, boss);
        camera_look_low(player, boss);
        if (egg_viper_camera_next) {
            egg_viper_camera_next = FALSE;
            camera_subphase++;
        }
        break;
    case 2:
        camera_go_beside_boss(player, boss);
        camera_look_low(player, boss);
        break;
    }
}

/* the camera goes beside player 0, then past the boss in z, looking along
 * its row */
static void camera_hakai(struct task_work *player, struct task_work *boss)
{
    switch (camera_subphase) {
    case 0:
        egg_viper_camera_next = FALSE;
        camera_subphase++;
    case 1:
        camera_go_beside(player, boss);
        camera_look_low(player, boss);
        if (egg_viper_camera_next) {
            egg_viper_camera_next = FALSE;
            camera_subphase++;
        }
        break;
    case 2:
        camera_go_past_boss(player, boss);
        camera_look_along(player, boss);
        break;
    }
}

/* the camera goes in line with the boss, turning to it, then low beside
 * player 0 */
static void camera_break(struct task_work *player, struct task_work *boss)
{
    switch (camera_subphase) {
    case 0:
        egg_viper_camera_next = FALSE;
        camera_subphase++;
    case 1:
        camera_go_beside_boss(player, boss);
        camera_look_at_boss(player, boss);
        if (egg_viper_camera_next) {
            egg_viper_camera_next = FALSE;
            camera_subphase++;
        }
        break;
    case 2:
        camera_go_beside_boss_low(player, boss);
        camera_look_high(player, boss);
        break;
    }
}

/* the camera goes above player 0, turning to him, but no lower than
 * LOWEST_Y */
static void camera_fallen(struct task_work *player, struct task_work *boss)
{
    switch (camera_subphase) {
    case 0:
        camera_set_side_x(player);
        camera_subphase++;
    case 1:
        camera_go_above(player, boss);
        camera_turn(&player->pos);
        if (camera_state->pos.y < LOWEST_Y) {
            camera_state->pos.y = LOWEST_Y;
        }
        break;
    }
}

/* move the camera beside player 0. it stands BESIDE_X past him in x, away
 * from the middle, and level with him in z */
static Bool camera_go_beside(struct task_work *player, struct task_work *boss)
{
    NJS_POINT3 pos;

    pos.x = player->pos.x + (player->pos.x < 0.0F ? -BESIDE_X : BESIDE_X);
    pos.y = ((player->pos.y - 10.0F) * 0.3F) + 40.0F;
    pos.z = player->pos.z;
    return camera_move(&pos);
}

/* as camera_go_beside, but in line with the boss. its z puts it on the
 * line from the boss through player 0, at the camera's x. it stays within
 * Z_REACH of him in z */
static Bool camera_go_beside_boss(struct task_work *player,
                                  struct task_work *boss)
{
    NJS_POINT3 pos;

    pos.x = player->pos.x + (player->pos.x < 0.0F ? -BESIDE_X : BESIDE_X);
    pos.y = ((player->pos.y - 10.0F) * 0.3F) + 40.0F;
    pos.z = boss->pos.z - ((boss->pos.z - player->pos.z) *
                           (boss->pos.x - camera_state->pos.x) /
                           (boss->pos.x - player->pos.x));
    if (pos.z - player->pos.z > Z_REACH) {
        pos.z = player->pos.z + Z_REACH;
    }
    if (pos.z - player->pos.z < -Z_REACH) {
        pos.z = player->pos.z - Z_REACH;
    }
    return camera_move(&pos);
}

/* as camera_go_beside_boss, 10 lower */
static Bool camera_go_beside_boss_low(struct task_work *player,
                                      struct task_work *boss)
{
    NJS_POINT3 pos;

    pos.x = player->pos.x + (player->pos.x < 0.0F ? -BESIDE_X : BESIDE_X);
    pos.y = ((player->pos.y - 10.0F) * 0.3F) + 30.0F;
    pos.z = boss->pos.z - ((boss->pos.z - player->pos.z) *
                           (boss->pos.x - camera_state->pos.x) /
                           (boss->pos.x - player->pos.x));
    if (pos.z - player->pos.z > Z_REACH) {
        pos.z = player->pos.z + Z_REACH;
    }
    if (pos.z - player->pos.z < -Z_REACH) {
        pos.z = player->pos.z - Z_REACH;
    }
    return camera_move(&pos);
}

/* move the camera 50 from player 0 in z, away from camera_side's half,
 * level with him in x */
static Bool camera_go_behind(struct task_work *player, struct task_work *boss)
{
    NJS_POINT3 pos;

    pos.x = player->pos.x;
    pos.y = ((player->pos.y - 10.0F) * 0.3F) + 25.0F;
    pos.z = player->pos.z + (camera_side == 0 ? 50.0F : -50.0F);
    return camera_move(&pos);
}

/* move the camera 120 above player 0, and 80 from him in x away from
 * camera_side's half */
static Bool camera_go_above(struct task_work *player, struct task_work *boss)
{
    NJS_POINT3 pos;
    Float x = player->pos.x;

    /* @hack redundant: a test of x with nothing in it. without it,
     * camera_go_above runs 4 bytes short, and 31 of its 34 words differ */
    if (x < 0.0F) {
    }
    pos.x = x + (camera_side == 0 ? 80.0F : -80.0F);
    pos.y = player->pos.y + 120.0F;
    pos.z = player->pos.z;
    return camera_move(&pos);
}

/* set camera_side by player 0's x, and keep the boss's z for
 * camera_go_wide */
static void camera_pick_side(struct task_work *player, struct task_work *boss)
{
    camera_set_side_x(player);
    camera_boss_z = boss->pos.z;
}

/* move the camera out wide: BESIDE_X past player 0 in x, toward
 * camera_side's half. its z puts it on the line through him from
 * camera_boss_z at x 150, in the other half. it stays within Z_REACH of him
 * in z */
static Bool camera_go_wide(struct task_work *player, struct task_work *boss)
{
    NJS_POINT3 pos;
    Float far = camera_side == 0 ? 150.0F : -150.0F;

    pos.x = player->pos.x + (camera_side == 0 ? -BESIDE_X : BESIDE_X);
    pos.y = ((player->pos.y - 10.0F) * 0.7F) + 40.0F;
    if (pos.y < 30.0F) {
        pos.y = player->pos.y + 20.0F;
    }
    pos.z = camera_boss_z - ((camera_boss_z - player->pos.z) * (far - pos.x) /
                             (far - player->pos.x));
    if (pos.z - player->pos.z > Z_REACH) {
        pos.z = player->pos.z + Z_REACH;
    }
    if (pos.z - player->pos.z < -Z_REACH) {
        pos.z = player->pos.z - Z_REACH;
    }
    return camera_move(&pos);
}

/* look at once at player 0, at a height that moves three tenths as far as
 * his */
static Bool camera_look_low(struct task_work *player, struct task_work *boss)
{
    NJS_POINT3 target;

    target.x = player->pos.x;
    target.y = ((player->pos.y - 10.0F) * 0.3F) + 30.0F;
    target.z = player->pos.z;
    return camera_look(&target);
}

/* look at once at player 0, at a height that moves seven tenths as far as
 * his */
static Bool camera_look_high(struct task_work *player, struct task_work *boss)
{
    NJS_POINT3 target;

    target.x = player->pos.x;
    target.y = ((player->pos.y - 10.0F) * 0.7F) + 30.0F;
    target.z = player->pos.z;
    return camera_look(&target);
}

/* turn the camera toward the boss at 8 a step, but no lower than y 0 */
static Bool camera_look_at_boss(struct task_work *player,
                                struct task_work *boss)
{
    NJS_POINT3 target;

    target.x = boss->pos.x;
    target.y = boss->pos.y;
    if (target.y < 0.0F) {
        target.y = 0.0F;
    }
    target.z = boss->pos.z;
    return camera_turn_at(&target, 8.0F);
}

/* move the camera to the boss's x and 80 past player 0 in z, away from the
 * boss. it stays at least 150 from the boss in z */
static Bool camera_go_past_boss(struct task_work *player,
                                struct task_work *boss)
{
    NJS_POINT3 pos;

    pos.x = boss->pos.x;
    pos.y = ((player->pos.y - 10.0F) * 0.3F) + 60.0F;
    if (player->pos.z > boss->pos.z) {
        pos.z = player->pos.z + 80.0F;
        if (pos.z - boss->pos.z < 150.0F) {
            pos.z = boss->pos.z + 150.0F;
        }
    } else {
        pos.z = player->pos.z - 80.0F;
        if (pos.z - boss->pos.z > -150.0F) {
            pos.z = boss->pos.z - 150.0F;
        }
    }
    return camera_move(&pos);
}

/* turn the camera toward player 0. the target's x stays from ROW_INNER_X
 * to ROW_OUTER_X, on the boss's side of x 0 */
static Bool camera_look_along(struct task_work *player, struct task_work *boss)
{
    NJS_POINT3 target;

    target.x = player->pos.x;
    if (boss->pos.x > 0.0F) {
        if (target.x > ROW_OUTER_X) {
            target.x = ROW_OUTER_X;
        } else if (target.x < ROW_INNER_X) {
            target.x = ROW_INNER_X;
        }
    } else {
        if (target.x > -ROW_INNER_X) {
            target.x = -ROW_INNER_X;
        } else if (target.x < -ROW_OUTER_X) {
            target.x = -ROW_OUTER_X;
        }
    }
    target.y = ((player->pos.y - 10.0F) * 0.7F) + 40.0F;
    target.z = player->pos.z;
    return camera_turn(&target);
}

Bool camera_move_at(NJS_POINT3 *pos, Float speed)
{
    NJS_VECTOR move;
    Float near;
    Float across;

    move.x = player_work[0]->pos.x - pos->x;
    move.z = player_work[0]->pos.z - pos->z;
    near = (move.x * move.x) + (move.z * move.z);
    move.x = pos->x - camera_state->pos.x;
    move.y = pos->y - camera_state->pos.y;
    move.z = pos->z - camera_state->pos.z;
    if (!((move.x * move.x) + (move.y * move.y) + (move.z * move.z) >
          speed * speed)) {
        camera_state->pos.x = pos->x;
        camera_state->pos.y = pos->y;
        camera_state->pos.z = pos->z;
        return TRUE;
    }
    njUnitVector(&move);
    camera_state->pos.x += move.x * speed;
    camera_state->pos.y += move.y * speed;
    camera_state->pos.z += move.z * speed;
    move.x = camera_state->pos.x - player_work[0]->pos.x;
    move.z = camera_state->pos.z - player_work[0]->pos.z;
    across = (move.x * move.x) + (move.z * move.z);
    if (across != 0.0F && across < near) {
        move.y = 0.0F;
        njUnitVector(&move);
        across = njSqrt(near);
        camera_state->pos.x = player_work[0]->pos.x + (move.x * across);
        camera_state->pos.z = player_work[0]->pos.z + (move.z * across);
    }
    return FALSE;
}

/* move camera_state's target speed toward target, or onto target once
 * within speed. it returns TRUE then */
static Bool camera_turn_at(NJS_POINT3 *target, Float speed)
{
    NJS_VECTOR move;

    move.x = target->x - camera_state->target.x;
    move.y = target->y - camera_state->target.y;
    move.z = target->z - camera_state->target.z;
    if (!((move.x * move.x) + (move.y * move.y) + (move.z * move.z) >
          speed * speed)) {
        camera_state->target.x = target->x;
        camera_state->target.y = target->y;
        camera_state->target.z = target->z;
        return TRUE;
    } else {
        njUnitVector(&move);
        camera_state->target.x += move.x * speed;
        camera_state->target.y += move.y * speed;
        camera_state->target.z += move.z * speed;
        return FALSE;
    }
}

/* move the camera a tenth of the way to pos. once a tenth is under 0.01,
 * move it onto pos. as with camera_move_at, the camera ends no nearer player
 * 0 in x and z than pos is. the result is TRUE once a tenth is under 4, else
 * FALSE */
static Bool camera_move(NJS_POINT3 *pos)
{
    NJS_VECTOR move;
    Float near;
    Float across;
    Float speed;

    move.x = player_work[0]->pos.x - pos->x;
    move.z = player_work[0]->pos.z - pos->z;
    near = (move.x * move.x) + (move.z * move.z);
    move.x = pos->x - camera_state->pos.x;
    move.y = pos->y - camera_state->pos.y;
    move.z = pos->z - camera_state->pos.z;
    speed = njSqrt((move.x * move.x) + (move.y * move.y) + (move.z * move.z));
    speed *= 0.1F;
    if (speed < 0.01F) {
        camera_state->pos.x = pos->x;
        camera_state->pos.y = pos->y;
        camera_state->pos.z = pos->z;
        return TRUE;
    }
    njUnitVector(&move);
    camera_state->pos.x += move.x * speed;
    camera_state->pos.y += move.y * speed;
    camera_state->pos.z += move.z * speed;
    move.x = camera_state->pos.x - player_work[0]->pos.x;
    move.z = camera_state->pos.z - player_work[0]->pos.z;
    across = (move.x * move.x) + (move.z * move.z);
    if (across != 0.0F && across < near) {
        move.y = 0.0F;
        njUnitVector(&move);
        across = njSqrt(near);
        camera_state->pos.x = player_work[0]->pos.x + (move.x * across);
        camera_state->pos.z = player_work[0]->pos.z + (move.z * across);
    }
    if (speed < 4.0F) {
        return TRUE;
    }
    return FALSE;
}

/* move camera_state's target a tenth of the way to target. it returns TRUE
 * once a tenth is under 0.5 */
static Bool camera_turn(NJS_POINT3 *target)
{
    NJS_VECTOR move;
    Float speed;

    move.x = target->x - camera_state->target.x;
    move.y = target->y - camera_state->target.y;
    move.z = target->z - camera_state->target.z;
    speed = njSqrt((move.x * move.x) + (move.y * move.y) + (move.z * move.z)) *
            0.1F;
    if (speed != 0.0F) {
        njUnitVector(&move);
        camera_state->target.x += move.x * speed;
        camera_state->target.y += move.y * speed;
        camera_state->target.z += move.z * speed;
    }
    return speed < 0.5F;
}

/* look at target at once, and return TRUE */
static Bool camera_look(NJS_POINT3 *target)
{
    camera_state->target.x = target->x;
    camera_state->target.y = target->y;
    camera_state->target.z = target->z;
    return TRUE;
}

void egg_viper_camera_shake(void)
{
    camera_shake_asked = TRUE;
}

/* put back the steady view, which the last step's shake lifted */
static void camera_restore_view(void)
{
    camera_state->pos.x = steady_pos_x;
    camera_state->pos.y = steady_pos_y;
    camera_state->pos.z = steady_pos_z;
    camera_state->target.x = steady_target_x;
    camera_state->target.y = steady_target_y;
    camera_state->target.z = steady_target_z;
}

/* keep the view the phase left as the steady view. while a shake runs,
 * lift the view by its height for this step. a shake asked for begins here,
 * and lifts the view from the next step */
static void camera_save_view(void)
{
    steady_pos_x = camera_state->pos.x;
    steady_pos_y = camera_state->pos.y;
    steady_pos_z = camera_state->pos.z;
    steady_target_x = camera_state->target.x;
    steady_target_y = camera_state->target.y;
    steady_target_z = camera_state->target.z;
    switch (camera_shaking) {
    case 0:
        if (camera_shake_asked) {
            camera_shake_asked = FALSE;
            camera_shake_steps = 0;
            camera_shaking = 1;
        }
        break;
    case 1:
        camera_state->pos.y += shake_heights[camera_shake_steps];
        camera_state->target.y += shake_heights[camera_shake_steps];
        camera_shake_steps++;
        if (shake_heights[camera_shake_steps] == 0.0F) {
            camera_shaking = 0;
        }
        break;
    }
}
