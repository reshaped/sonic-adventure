/* jump panels, O JPanel: numbered panels that player 0 jumps along */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "object/common.h"
#include "object/models.h"
#include "object/panel.h"
#include "object/placement.h"
#include "player/player.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/texture.h"
#include "system/util.h"

/* panels 0 to 9, each kept in panel_tasks at its number */
#define PANEL_COUNT 10

/* the id in a panel's work, by which panel_touched knows a panel */
#define PANEL_ID 8

/* player 0 can jump from a panel numbered 2 or more only in its first
 * PANEL_WAIT steps touching the panel. the count of those steps stops at
 * PANEL_WAIT_MAX */
#define PANEL_WAIT 30
#define PANEL_WAIT_MAX 3600

/* the index in panel_shapes of a numbered panel's top, from which player 0
 * jumps, and of panel 0's one shape */
#define TOP_SHAPE 1
#define END_SHAPE 3

/* a flight's speed a step: at least FLY_SPEED, and gaining FLY_GAIN a step
 * while under FLY_TOP */
#define FLY_SPEED 9.0F
#define FLY_GAIN 0.5F
#define FLY_TOP 15.0F

/* a flight aims the center of the player's first shape at a point above the
 * panel, along the panel's up: the player's consts.center_y plus AIM_ABOVE.
 * it lands the player LAND_ABOVE above the panel */
#define AIM_ABOVE 3.8F
#define LAND_ABOVE 3.3F

/* a panel draws while the camera is within DRAW_RANGE of it */
#define DRAW_RANGE 500.0F

/* the spark's frames, each held for two steps */
#define SPARK_FRAMES 5

/* a panel's mode: whether a jump leaves it or flies to it */
enum panel_mode {
    PANEL_IDLE,
    PANEL_LEFT, /* the panel the player jumped from */
    PANEL_NEXT /* the panel the player flies to */
};

struct panel_vars {
    void *param; /* the object kind's param, which no panel reads */
    /* the steps player 0 has touched a panel numbered 2 or more without a
     * break */
    Sint32 touched;
};

#define VARS(work) ((struct panel_vars *)&(work)->vars)

/* a panel's number, from the set file's scale x, kept in the work's timer
 * and read unsigned */
#define NUMBER(work) TASK_TIMER(work)

static void panel_idle_all(void);
static void panel_read_number(struct task *task);
static void panel_count_touch(struct task *task);
static void panel_step(struct task *task);
static void panel_draw(struct task *task);
static void panel_destroy(struct task *task);
static void panel_spark_draw(struct task *task);
static void panel_spark_step(struct task *task);
static void panel_spark_start(struct task *task);

/* a numbered panel takes the first two shapes: its body, and its top. the
 * top is narrower than the body, reaches 0.8 higher and pushes nothing. the
 * third copies the first, and no panel takes it */
/* clang-format off */
static struct collision_shape panel_shapes[4] = {
    {SHAPE_CYLINDER2 | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 1.5F, 0.0F}, 8.2F, 3.0F, 0.0F, {0},
     0, NULL},
    {SHAPE_CYLINDER2 | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL),
     {0.0F, 1.5F, 0.0F}, 6.0F, 3.8F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER2 | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 1.5F, 0.0F}, 8.2F, 3.0F, 0.0F, {0},
     0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_NO_PUSH, 0, PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F}, 5.0F, 0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* a texlist for each panel number, of that digit's texture in CON_REGULAR */
static NJS_TEXLIST panel_number_textures[PANEL_COUNT] = {
#include "object/panel/panel_number_textures.inc"
};

/* the spark that flashes where the player lands: lens04, 05, 02, 06 and 03 */
static NJS_TEXLIST panel_spark_textures[SPARK_FRAMES] = {
#include "object/panel/panel_spark_textures.inc"
};
static NJS_TEXANIM panel_spark_texanims[SPARK_FRAMES] = {
#include "object/panel/panel_spark_texanims.inc"
};
/* panel_spark_draw gives it the texlist of the frame it draws */
static NJS_SPRITE panel_spark_sprite = {
#include "object/panel/panel_spark_sprite.inc"
};

static struct task *panel_tasks[PANEL_COUNT];

void panel_reset(void)
{
    int i;

    for (i = 0; i < PANEL_COUNT; i++) {
        panel_tasks[i] = NULL;
    }
}

static void panel_idle_all(void)
{
    int i;

    for (i = 0; i < PANEL_COUNT; i++) {
        /* @hack spelling: the continue past an empty slot, which keeps the loop
         * rolled (BT/S at 0x8c06196e); as an if around the store, SHC
         * unrolls it and 19 of 21 words differ */
        if (panel_tasks[i] == NULL) {
            continue;
        }
        panel_tasks[i]->work->mode = PANEL_IDLE;
    }
}

Bool panel_touched(struct task_work *work)
{
    struct collision_work *hit = work->collision->hit;
    struct task_work *panel;

    if (hit == NULL) {
        return FALSE;
    }
    if (PLAYER_VARS(work)->player != 0) {
        return FALSE;
    }
    panel = hit->owner->work;
    if (panel->id != PANEL_ID) {
        return FALSE;
    }
    return TRUE;
}

int panel_jump_ready_or_idle(struct task_work *work)
{
    struct collision_work *hit;
    struct task_work *panel;

    if (!panel_touched(work)) {
        if (PLAYER_VARS(work)->player == 0) {
            panel_idle_all();
        }
        return 0;
    }
    hit = work->collision->hit;
    panel = hit->owner->work;
    if (NUMBER(panel) == 0) {
        panel_idle_all();
        return -1;
    }
    if (work->collision->hit_shape != TOP_SHAPE) {
        panel_idle_all();
        return 0;
    }
    /* @hack type: touched compared as a Float, which the image converts with
     * FLOAT and tests with FCMP/GT (0x8c0619f4, 0x8c0619f6); compared as an
     * int, 20 of 51 words differ */
    if (VARS(panel)->touched >= (Float)PANEL_WAIT) {
        panel_idle_all();
        return 0;
    }
    return 1;
}

void panel_jump(struct task_work *work)
{
    struct collision_work *hit;
    struct task_work *panel;
    Sint16 number;

    panel_idle_all();
    hit = work->collision->hit;
    panel = hit->owner->work;
    number = NUMBER(panel);
    panel->mode = PANEL_LEFT;
    /* @hack spelling: the two ways to panel 0 tested apart, which loads
     * panel_tasks ahead of the tests and reads the next slot at @(4,R3)
     * (0x8c061a64, 0x8c061a72); joined with ||, SHC indexes it through R0
     * and 23 of 37 words differ */
    if (number == PANEL_COUNT - 1) {
        number = 0;
    } else if (panel_tasks[number + 1] == NULL) {
        number = 0;
    } else {
        number++;
    }
    panel_tasks[number]->work->mode = PANEL_NEXT;
}

int panel_fly(struct task_work *work, struct task_motion *motion,
              struct player *player)
{
    struct task_work *next;
    Sint16 i;
    struct task *spark;
    NJS_VECTOR way;
    NJS_VECTOR up;
    NJS_VECTOR offset;
    Angle ang_x;
    Angle ang_y;
    Angle ang_z;
    Float distance;
    Float speed;

    /* @hack spelling: the search as a do-while, which indexes panel_tasks by i
     * on each pass (MOV.L @(R0,R6),R3 at 0x8c061ab8); as a for loop, SHC
     * walks three pointers instead and 52 of 383 words differ */
    i = 0;
    next = NULL;
    do {
        if (panel_tasks[i] == NULL) {
            return 0;
        }
        if (panel_tasks[i]->work->mode == PANEL_NEXT) {
            next = panel_tasks[i]->work;
            break;
        }
    } while (++i < PANEL_COUNT);
    if (next == NULL) {
        return 0;
    }
    up.x = up.z = 0.0F;
    up.y = player->consts.center_y + AIM_ABOVE;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, next->ang.z);
    TURN(njRotateX, next->ang.x);
    TURN(njRotateY, next->ang.y);
    njCalcVector(NULL, &up, &way);
    njPopMatrix(1);
    /* @hack spelling: the offset added last, which sums into FR2 (FADD FR3,FR2
     * at 0x8c061b96, 0x8c061bae and 0x8c061bca); with +=, SHC sums into FR3
     * and 6 of 383 words differ */
    way.x = next->pos.x - work->collision->shapes->center.x + way.x;
    way.y = next->pos.y - work->collision->shapes->center.y + way.y;
    way.z = next->pos.z - work->collision->shapes->center.z + way.z;
    distance = njScalor(&way);
    speed = njScalor(&motion->speed);
    njUnitVector(&way);
    ang_x = next->ang.x;
    ang_z = next->ang.z;
    if (distance <= speed) {
        up.x = up.z = 0.0F;
        up.y = LAND_ABOVE;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, next->ang.z);
        TURN(njRotateX, next->ang.x);
        njCalcVector(NULL, &up, &offset);
        njPopMatrix(1);
        work->pos.x = next->pos.x + offset.x;
        work->pos.y = next->pos.y + offset.y;
        work->pos.z = next->pos.z + offset.z;
        work->ang.x = ang_x;
        work->ang.y = -next->ang.y;
        work->ang.z = ang_z;
        player_clear_speed(motion, player);
        work->flags |= PLAYER_ON_TASK;
        panel_idle_all();
        distance = 0.0F;
        if ((spark = task_create(TASK_WORK, 6, panel_spark_start)) != NULL) {
            spark->work->pos = work->pos;
            spark->work->ang = next->ang;
        }
    } else {
        if (speed < FLY_SPEED) {
            speed = FLY_SPEED;
        } else if (speed < FLY_TOP) {
            speed += FLY_GAIN;
        }
        way.x *= speed;
        way.y *= speed;
        way.z *= speed;
        motion->speed = way;
        work->pos.x += way.x;
        work->pos.y += way.y;
        work->pos.z += way.z;
        ang_z = -(Angle)(atan2f(way.x, way.y) * ANGLE_PER_RADIAN);
        ang_x = (Angle)(asinf(way.z) * ANGLE_PER_RADIAN);
        ang_y = (Angle)(atan2f(way.x, -way.z) * ANGLE_PER_RADIAN);
        work->ang.x = angle_approach(work->ang.x, ang_x, NJM_DEG_ANG(22.5));
        work->ang.y = angle_approach(work->ang.y, ang_y, NJM_DEG_ANG(22.5));
        work->ang.z = angle_approach(work->ang.z, ang_z, NJM_DEG_ANG(22.5));
        player_turn_back(work, &way);
        player->speed = way;
    }
    return (int)(distance * (1.0F / FLY_SPEED));
}

int panel_get_left(void)
{
    int i;
    int left = 0;

    for (i = 0; i < PANEL_COUNT; i++) {
        /* @hack spelling: the continue past an empty slot, as in panel_idle_all
         * (BT at 0x8c061eca); as an if around the test, 23 of 23 words
         * differ */
        if (panel_tasks[i] == NULL) {
            continue;
        }
        if (panel_tasks[i]->work->mode == PANEL_LEFT) {
            left = i;
        }
    }
    return left;
}

void panel_start(struct task *task)
{
    struct task_work *work = task->work;

    work->id = PANEL_ID;
    if (!placement_editing(task)) {
        panel_read_number(task);
        /* @hack spelling: the radius stored in each arm, which puts
         * collision_init's address in R3 and the count of 2 in R2
         * (0x8c061f1e, 0x8c061f20); stored once after the arms, the two
         * trade and 8 of 61 words differ */
        if (NUMBER(work) == 0) {
            collision_init(task, &panel_shapes[END_SHAPE], COLLISION_OBJECT, 1,
                           1);
            work->collision->radius = 10.3F;
        } else {
            collision_init(task, panel_shapes, COLLISION_OBJECT, 1, 2);
            work->collision->radius = 10.3F;
        }
        panel_tasks[NUMBER(work)] = task;
    }
    task->destructor = panel_destroy;
    task->draw = panel_draw;
    task->step = panel_step;
}

/* the number is scale x, truncated. a scale x below 0, or of 10 or more, is
 * set to 0 first */
static void panel_read_number(struct task *task)
{
    struct task_work *work = task->work;

    if (work->scl.x >= PANEL_COUNT || work->scl.x < 0.0F) {
        work->scl.x = 0.0F;
    }
    work->timer = njRoundOff(work->scl.x);
}

/* count the steps player 0 touches a panel numbered 2 or more, up to
 * PANEL_WAIT_MAX. any other step sets the count to 0 */
static void panel_count_touch(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *hit;

    if (work->collision != NULL) {
        /* @hack spelling: the hit read through task->work again (MOV.L
         * @(32,R4),R3 at 0x8c061fda); through work, 35 of 36 words differ */
        hit = task->work->collision->hit;
        if (hit != NULL && hit->kind == COLLISION_PLAYER &&
            PLAYER_VARS(hit->owner->work)->player == 0 && NUMBER(work) != 0 &&
            NUMBER(work) != 1) {
            if (VARS(work)->touched++ >= PANEL_WAIT_MAX) {
                VARS(work)->touched = PANEL_WAIT_MAX;
            }
        } else {
            VARS(work)->touched = 0;
        }
    }
}

/* the panel marked left registers no collision, so the player flying off
 * it doesn't touch it */
static void panel_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        panel_count_touch(task);
        if (work->mode != PANEL_LEFT) {
            collision_register(work);
        }
        object_clear_touch(work, NULL);
    } else {
        panel_read_number(task);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> ROTATION X");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATION Y");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "-> ROTATION Z");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> PANEL NUMBER");
    }
    panel_draw(task);
}

/* draw the panel while the camera is within DRAW_RANGE. panel 0 draws only
 * while the set editor is open */
static void panel_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (njDistanceP2P(&camera_work->pos, &work->pos) < DRAW_RANGE &&
        (NUMBER(work) != 0 || edit_mode != 0)) {
        draw_attr_save();
        draw_attr_clear(0, NJD_FLAG_IGNORE_SPECULAR);
        njPushMatrix(NULL);
        texture_set_regular();
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        if (NUMBER(work) == 0) {
            draw_tree(&panel_end_object);
        } else {
            draw_tree(&panel_object);
            njSetTexture(&panel_number_textures[NUMBER(work)]);
            draw_tree(&panel_number_object);
        }
        njPopMatrix(1);
        draw_attr_restore();
    }
}

/* empty the panel's slot, unless a later panel of its number holds it */
static void panel_destroy(struct task *task)
{
    Sint16 number = NUMBER(task->work);

    if (panel_tasks[number] == task) {
        panel_tasks[number] = NULL;
    }
}

/* draw only on a display frame's first step, frame_step 0 */
static void panel_spark_draw(struct task *task)
{
    struct task_work *work = task->work;
    int frame = work->tick >> 1;

    if (frame_step != 0) {
        return;
    }
    panel_spark_sprite.tlist = &panel_spark_textures[frame];
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njRotateX(NULL, NJM_DEG_ANG(90));
    njDrawSprite3D(&panel_spark_sprite, frame, NJD_SPRITE_ALPHA);
    njPopMatrix(1);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
}

static void panel_spark_step(struct task *task)
{
    struct task_work *work = task->work;

    if (++work->tick >= SPARK_FRAMES * 2) {
        task_kill(task);
    } else {
        panel_spark_draw(task);
    }
}

static void panel_spark_start(struct task *task)
{
    task->step = panel_spark_step;
    task->draw = panel_spark_draw;
    panel_spark_draw(task);
}
