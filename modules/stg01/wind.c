/* O BEWIND box of rising wind that lifts player 0 */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg01/wind.h"
#include "system/task.h"
#include "system/util.h"

/* a unit cube about the origin: its twelve edges, each a pair of points */
static NJS_POINT3 beach_wind_cube[24] = {
#include "stg01/wind/beach_wind_cube.inc"
};

/* the set editor draws the box in white, and a unit cube at its position in
 * yellow. both draw as lines between pairs of beach_wind_cube's points */
static NJS_COLOR beach_wind_white[24] = {
#include "stg01/wind/beach_wind_white.inc"
};
static NJS_POINT3COL beach_wind_box[1] = {
#include "stg01/wind/beach_wind_box.inc"
};
static NJS_COLOR beach_wind_yellow[24] = {
#include "stg01/wind/beach_wind_yellow.inc"
};
static NJS_POINT3COL beach_wind_origin[1] = {
#include "stg01/wind/beach_wind_origin.inc"
};

/* the mode is WIND_EDIT while the set editor holds the task */
enum beach_wind_mode { WIND_INIT = 0, WIND_BLOW = 2, WIND_EDIT = 9 };

/* the step holds scl's sizes within BOX_MIN to BOX_MAX, and the power within
 * POWER_MIN to POWER_MAX. it does so as the task starts and in the set
 * editor */
#define BOX_MIN 10.0F
#define BOX_MAX 200.0F
/* the wind's power, ang.x's low byte. the set editor's label says 1 to 10 */
#define POWER(work) ((Uint8)(work)->ang.x)
#define POWER_MIN 1
#define POWER_MAX 8
struct beach_wind_vars {
    /* dwell[i] counts the steps in a row the loop's pass i has found its
     * player in the box. it stops at DWELL_MAX */
    Uint8 dwell[4];
    Uint8 unk_04[2];
    /* in the set editor, its low 7 bits are the lowest slice's height above
     * the box's bottom. it rises by the power each step, so the slices climb.
     * the low bits wrap at SLICE_GAP */
    Sint8 slice;
    Uint8 unk_07;
    Uint8 unk_08[4];
};

#define VARS(work) ((struct beach_wind_vars *)&(work)->vars)

#define DWELL_MAX 32
/* the wind lifts a player only after more than this many steps in a row in
 * the box */
#define DWELL_WAIT 2

/* the set editor draws level slices of the box this far apart */
#define SLICE_GAP 128

/* each step player 0 is in the box, put it in the updraft. after more than
 * DWELL_WAIT steps there, the updraft pushes it. the push is (power - its
 * upward speed) / 10 */
void beach_wind_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;
    struct task_motion *motion;
    struct player *record;
    Uint8 i;
    Float top;
    Float bottom;
    Float y;
    Float lift;
    Float ax;
    Float az;
    Float bx;
    Float bz;
    Float half_ax;
    Float half_az;
    Float half_bx;
    Float half_bz;
    Float edge_x;
    Float edge_z;
    Float dx;
    Float dz;
    Angle ang;

    work->tick = 1;
    if (placement_try_leave(task)) {
        return;
    }
    top = work->pos.y + (work->scl.y * 0.5F);
    bottom = work->pos.y - (work->scl.y * 0.5F);
    if (work->mode == WIND_INIT || work->mode == WIND_EDIT) {
        if (work->scl.x < BOX_MIN) {
            work->scl.x = BOX_MIN;
        } else if (work->scl.x > BOX_MAX) {
            work->scl.x = BOX_MAX;
        }
        if (work->scl.y < BOX_MIN) {
            work->scl.y = BOX_MIN;
        } else if (work->scl.y > BOX_MAX) {
            work->scl.y = BOX_MAX;
        }
        if (work->scl.z < BOX_MIN) {
            work->scl.z = BOX_MIN;
        } else if (work->scl.z > BOX_MAX) {
            work->scl.z = BOX_MAX;
        }
        if (POWER(work) < POWER_MIN) {
            work->ang.x = POWER_MIN;
        } else if (POWER(work) > POWER_MAX) {
            work->ang.x = POWER_MAX;
        }
    }
    switch (work->mode) {
    case WIND_INIT:
        work->vars.l[0] = 0;
        work->mode = WIND_BLOW;
        VARS(work)->unk_08[0] = 0;
        break;
    case WIND_BLOW:
        /* @bug memory: the loop steps player and motion by their own
         * sizes, so its second pass reads the 64 bytes after player 0's work
         * and motion, not player 1's. the image loads player_motion[0] and
         * player_work[0] at 0x0c90d520 and 0x0c90d522, and steps them with
         * ADD #64 at 0x0c90d6ea and 0x0c90d6ee. with player_work[0] null, the
         * second pass reads a position at address 96 (0x0c90d56a) */
        player = player_work[0];
        motion = player_motion[0];
        for (i = 0; i < 2; i++, player++, motion++) {
            if (player == NULL) {
                continue;
            }
            if (player->pos.y > top || player->pos.y < bottom) {
                VARS(work)->dwell[i] = 0;
                continue;
            }
            /* a is the box's x edge and b its z edge, each turned by
             * -ang.y */
            ax = njCos(work->ang.y) * work->scl.x;
            az = njSin(work->ang.y) * work->scl.x;
            bx = -(njSin(work->ang.y) * work->scl.z);
            bz = njCos(work->ang.y) * work->scl.z;
            half_ax = ax * 0.5F;
            half_az = az * 0.5F;
            half_bx = bx * 0.5F;
            half_bz = bz * 0.5F;
            /* a player outside any of the four sides is out of the box */
            edge_x = -bx;
            edge_z = -bz;
            dx = player->pos.x - work->pos.x - half_ax - half_bx;
            dz = player->pos.z - work->pos.z - half_az - half_bz;
            if ((edge_z * dx) - (edge_x * dz) < 0.0F) {
                VARS(work)->dwell[i] = 0;
                continue;
            }
            edge_x = -ax;
            edge_z = -az;
            dx = player->pos.x - work->pos.x - half_ax + half_bx;
            dz = player->pos.z - work->pos.z - half_az + half_bz;
            if ((edge_z * dx) - (edge_x * dz) < 0.0F) {
                VARS(work)->dwell[i] = 0;
                continue;
            }
            edge_x = bx;
            edge_z = bz;
            dx = player->pos.x - work->pos.x + half_ax + half_bx;
            dz = player->pos.z - work->pos.z + half_az + half_bz;
            if ((edge_z * dx) - (edge_x * dz) < 0.0F) {
                VARS(work)->dwell[i] = 0;
                continue;
            }
            edge_x = ax;
            edge_z = az;
            dx = player->pos.x - work->pos.x + half_ax - half_bx;
            dz = player->pos.z - work->pos.z + half_az - half_bz;
            if ((edge_z * dx) - (edge_x * dz) < 0.0F) {
                VARS(work)->dwell[i] = 0;
                continue;
            }
            if (++VARS(work)->dwell[i] > DWELL_MAX) {
                VARS(work)->dwell[i] = DWELL_MAX;
            }
            record = motion->record;
            if (VARS(work)->dwell[i] > DWELL_WAIT) {
                lift = (POWER(work) - record->speed.y) * 0.1F;
            } else {
                lift = 0.0F;
            }
            player_updraft(i, 0.0F, lift, 0.0F);
        }
        break;
    case WIND_EDIT:
        /* in the set editor, draw a unit cube at the position, the box, and
         * the box's slices. the slices rise by the power each step. then label
         * the fields. once the editor lets the task go, it starts again */
        if (!placement_editing(task)) {
            work->mode = WIND_INIT;
        }
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njDrawLine3D(beach_wind_origin, 12, 0);
        TURN_VIA(njRotateY, -work->ang.y, ang);
        njScale(NULL, work->scl.x, work->scl.y, work->scl.z);
        njDrawLine3D(beach_wind_box, 12, 0);
        njPopMatrix(1);
        VARS(work)->slice += work->ang.x;
        y = bottom + (VARS(work)->slice & (SLICE_GAP - 1));
        while (y < top) {
            njPushMatrix(NULL);
            njTranslate(NULL, work->pos.x, y, work->pos.z);
            TURN_VIA(njRotateY, -work->ang.y, ang);
            njScale(NULL, work->scl.x, 0.0F, work->scl.z);
            njDrawLine3D(beach_wind_box, 12, 0);
            njPopMatrix(1);
            y += SLICE_GAP;
        }
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "<- POWER{ 1-10}");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "<- COLLIRotateY");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<- COLLISIZE X");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- COLLISIZE Y");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<- COLLISIZE Z");
        return;
    }
    if (placement_editing(task)) {
        work->mode = WIND_EDIT;
    }
}
