/* the way down from the station to the room below it, and the ladder back up */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/ladder.h"
#include "adv02/station_ladder.h"
#include "camera/camera.h"
#include "draw/model.h"
#include "path/path.h"
#include "path/steps.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* where the ladder stands in the room below, and the y angle it faces */
#define LADDER_X (-60.0F)
#define LADDER_Y (-400.0F)
#define LADDER_Z 1222.0F
#define LADDER_ANG 0x6800
/* the ladder task runs only while the camera is within 200 units of the ladder
 * in x and z. the range is squared */
#define RUN_RANGE (200.0F * 200.0F)

/* the ladder's path has RUNGS points, RUNG_SPACING apart up the ladder's y.
 * the lowest lies FOOT_Y above the ladder's position, and each lies FACE_Z
 * along its z. player 0 coming down takes hold at point START_RUNG */
#define RUNGS 33
#define RUNG_SPACING 3.0F
#define FOOT_Y 6.0F
#define FACE_Z (-2.55F)
#define START_RUNG 10
/* the ladder draws its model twice, the second MODEL_HEIGHT above the first */
#define MODEL_HEIGHT 100.0F

/* going up puts player 0 here, by the station, facing TOP_ANG */
#define TOP_X (-110.0F)
#define TOP_Y 65.0F
#define TOP_Z 1138.0F
#define TOP_ANG 0x9800

/* player 0 on the ladder's path above this y goes up to the station */
#define LADDER_TOP (-340.0F)

/* the fade's change a step, and the steps the screen stays black */
#define FADE_SPEED 0.02F
#define BLACK_STEPS 20

/* the task's modes. work->submode holds the way player 0 is taken */
enum station_ladder_mode {
    LADDER_INIT,
    LADDER_WAIT,
    LADDER_BLACK, /* player 0 moves while the screen stays black */
    LADDER_FADE_IN,
    LADDER_FADE_OUT
};
enum station_ladder_way { LADDER_DOWN, LADDER_UP };

struct station_ladder_vars {
    struct task *ladder; /* the child task that builds the ladder's path */
    Uint8 unk_04[4];
    Float fade; /* 0 for black, 1 for clear */
};

#define VARS(work) ((struct station_ladder_vars *)&(work)->vars)

/* the ladder task's modes. it builds its path on its first step with the camera
 * in RUN_RANGE */
enum ladder_mode { BUILD_PATH, PATH_BUILT };

/* the ladder task's path and its points, which the task allocates from the heap
 * as it builds the path. its destructor frees them */
struct ladder_vars {
    Uint8 unk_00[4];
    struct path_point *points;
    struct path *path;
};

#define LADDER(work) ((struct ladder_vars *)&(work)->vars)

static void station_ladder_fade(Float fade);
static Bool station_ladder_at_top(struct task *task);
static void station_ladder_go_down(struct task *task);
static void station_ladder_go_up(struct task *task);
static void station_ladder_ladder_step(struct task *task);
static void station_ladder_ladder_destroy(struct task *task);
static void station_ladder_ladder_draw(struct task *task);

/* player 0 goes down on walking into the cylinder about this point, of radius
 * WAY_DOWN_REACH and reaching WAY_DOWN_HEIGHT above and below it */
static NJS_POINT3 station_ladder_way_down = {-54.0F, 70.0F, 1176.0F};
#define WAY_DOWN_REACH 30.0F
#define WAY_DOWN_HEIGHT 30.0F

/* player 0 goes down on walking into the way down, and up on climbing the
 * ladder above LADDER_TOP. the screen fades to black and back, and player 0
 * moves while it is black. player 0's pad is off from the fade out's start to
 * the fade in's end */
void station_ladder_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *ladder;
    struct task_work *ladder_work;
    Bool there;

    switch (work->mode) {
    case LADDER_INIT:
        task->destructor = task_destroy_children;
        if ((ladder = task_create_child(TASK_WORK, station_ladder_ladder_step,
                                        task)) != NULL) {
            ladder_work = ladder->work;
            ladder_work->pos.x = LADDER_X;
            ladder_work->pos.y = LADDER_Y;
            ladder_work->pos.z = LADDER_Z;
            ladder_work->ang.y = LADDER_ANG;
        }
        VARS(work)->ladder = ladder;
        work->mode = LADDER_WAIT;
        break;
    case LADDER_WAIT:
        there = player_in_cylinder(&station_ladder_way_down, WAY_DOWN_REACH,
                                   WAY_DOWN_HEIGHT) == 1;
        if (there) {
            work->submode = LADDER_DOWN;
            pad_disable_player(0);
            VARS(work)->fade = 1.0F;
            work->mode = LADDER_FADE_OUT;
        } else if (station_ladder_at_top(task)) {
            work->submode = LADDER_UP;
            pad_disable_player(0);
            VARS(work)->fade = 1.0F;
            work->mode = LADDER_FADE_OUT;
        }
        break;
    case LADDER_FADE_OUT:
        VARS(work)->fade -= FADE_SPEED;
        if (VARS(work)->fade <= 0.0F) {
            VARS(work)->fade = 0.0F;
            work->timer = BLACK_STEPS;
            work->mode = LADDER_BLACK;
        }
        station_ladder_fade(VARS(work)->fade);
        break;
    case LADDER_BLACK:
        if (work->timer == 0) {
            if (work->submode == LADDER_DOWN) {
                station_ladder_go_down(task);
            } else {
                station_ladder_go_up(task);
            }
            work->mode = LADDER_FADE_IN;
        } else {
            work->timer = work->timer - 1;
        }
        station_ladder_fade(VARS(work)->fade);
        break;
    case LADDER_FADE_IN:
        VARS(work)->fade += FADE_SPEED;
        if (VARS(work)->fade >= 1.0F) {
            VARS(work)->fade = 1.0F;
            pad_enable_player(0);
            work->mode = LADDER_WAIT;
        }
        station_ladder_fade(VARS(work)->fade);
        break;
    }
    task_step_children(task);
}

/* draw black over the whole screen, opaque at fade 0 and clear at 1. it draws
 * only on a frame's first step */
static void station_ladder_fade(Float fade)
{
    NJS_POINT2COL quad;
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    Uint32 color;

    if (frame_step) {
        return;
    }
    quad.p = points;
    quad.col = colors;
    quad.tex = NULL;
    quad.num = 4;
    color = (1.0F - fade) * 255.0F;
    color <<= 24;
    points[0].x = 0.0F;
    points[0].y = 0.0F;
    colors[0].color = color;
    points[1].x = 0.0F;
    points[1].y = 480.0F;
    colors[1].color = color;
    points[2].x = 640.0F;
    points[2].y = 480.0F;
    colors[2].color = color;
    points[3].x = 640.0F;
    points[3].y = 0.0F;
    colors[3].color = color;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njDrawPolygon2D(&quad, 4, -10.0F, NJD_FILL | NJD_TRANSPARENT);
}

/* TRUE while player 0, and not player 1, is on the ladder's path above
 * LADDER_TOP. FALSE until the ladder task has built its path */
static Bool station_ladder_at_top(struct task *task)
{
    struct task_work *work = task->work;
    struct task *ladder = VARS(work)->ladder;
    struct path *path = LADDER(ladder->work)->path;

    if (player_on_path(path) == 1) {
        if (player_work[0]->pos.y > LADDER_TOP) {
            return TRUE;
        }
    }
    return FALSE;
}

static void station_ladder_go_down(struct task *task)
{
    struct task_work *work = task->work;
    struct task *ladder = VARS(work)->ladder;
    struct path *path = LADDER(ladder->work)->path;
    struct angles ang;

    ang.x = 0;
    ang.y = ladder->work->ang.y;
    ang.z = 0;
    player_set_position(0, LADDER_X, LADDER_Y, LADDER_Z);
    player_ledge_path(0, path, START_RUNG, &ang);
}

static void station_ladder_go_up(struct task *task)
{
    player_set_mode(0, PLAYER_MODE_RESET);
    player_set_position(0, TOP_X, TOP_Y, TOP_Z);
    player_set_angles(0, 0, TOP_ANG, 0);
}

static void station_ladder_ladder_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *child;
    struct task_work *child_work;
    struct path_point *points;
    struct path_point *point;
    struct path *path;
    NJS_POINT3 rung;
    NJS_POINT3 offset;
    Angle ang;
    int i;

    if (((work->pos.x - camera_work->pos.x) *
         (work->pos.x - camera_work->pos.x)) +
            ((work->pos.z - camera_work->pos.z) *
             (work->pos.z - camera_work->pos.z)) >
        RUN_RANGE) {
        return;
    }
    switch (work->mode) {
    case BUILD_PATH:
        task->destructor = station_ladder_ladder_destroy;
        task->draw = station_ladder_ladder_draw;
        child = task_create_child(TASK_WORK, path_ledge_step, task);
        if (child != NULL) {
            child_work = child->work;
            PATH_VARS(child_work)->stage_act = stage_get_stage_act();
            child_work->ang.y = work->ang.y;
        }
        offset.x = 0.0F;
        offset.y = FOOT_Y;
        offset.z = FACE_Z;
        njPushMatrix(&_unit_matrix_);
        TURN_VIA(njRotateY, work->ang.y, ang);
        points = heap_alloc(sizeof(struct path_point) * RUNGS);
        LADDER(work)->points = points;
        point = points;
        for (i = 0; i < RUNGS; i++, point++) {
            point->ang_x = 0;
            point->ang_z = 0;
            point->length = RUNG_SPACING;
            njCalcPoint(NULL, &offset, &rung);
            point->pos.x = work->pos.x + rung.x;
            point->pos.y = work->pos.y + rung.y;
            point->pos.z = work->pos.z + rung.z;
            offset.y += RUNG_SPACING;
        }
        njPopMatrix(1);
        path = heap_alloc(sizeof(struct path));
        LADDER(work)->path = path;
        path->ends = PATH_STOP;
        path->count = RUNGS;
        path->length = (RUNGS - 1) * RUNG_SPACING;
        path->points = points;
        path->step = path_ledge_step;
        /* @bug memory: the test above allows for task_create_child's NULL, and
         * this store doesn't. the read of child's work goes through it, and
         * the path is written through what it reads (MOV.L @(32,R8),R3 and
         * MOV.L R4,@(16,R3) at 0x0c91494e and 0x0c914950, R8 the child) */
        PATH_VARS(child->work)->path = path;
        work->mode = PATH_BUILT;
        break;
    case PATH_BUILT:
        break;
    }
    task_step_children(task);
    station_ladder_ladder_draw(task);
}

static void station_ladder_ladder_destroy(struct task *task)
{
    struct task_work *work = task->work;

    heap_free(LADDER(work)->path);
    heap_free(LADDER(work)->points);
    task_destroy_children(task);
}

static void station_ladder_ladder_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(ladder_object->model);
    njTranslate(NULL, 0.0F, MODEL_HEIGHT, 0.0F);
    draw_model(ladder_object->model);
    njPopMatrix(1);
}
