/* O TAIHOU cannon, which loads players and fires them off */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "adv0100/cannon.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the cannon's tree. the base's child is the barrel, the barrel's child is
 * the hatch, and the hatch's sibling is the part that recoils */
extern NJS_OBJECT cannon_object;

#define LOAD_RADIUS 9.0F
#define LOAD_HEIGHT 24.0F
#define HATCH_NEAR 40.0F
/* how far the barrel, the hatch and the cannon's heading turn each step,
 * and the hatch's angle open */
#define BARREL_STEP 0x180
#define HATCH_STEP 0x200
#define TURN_STEP 0x100
#define HATCH_OPEN 0x5000
/* the angle cannon_slam_hatch measures from. it lies past HATCH_OPEN, so a
 * hatch open all the way still shuts */
#define HATCH_SLAM (HATCH_OPEN + 0x100)
/* how far along z the recoiling part stands from its rest as the cannon
 * fires, and how far it comes back each step */
#define RECOIL 4.0F
#define RECOIL_STEP 0.4F
/* cannon_fire sets work->timer to 0. each CANNON_FIRED step tests the
 * timer, then counts it up. the step that finds it past FIRED_STEPS goes
 * back to waiting: the 62nd after the step the cannon fires on */
#define FIRED_STEPS 60
/* the glow starts GLOW_REACH along the line the cannon fires on, from
 * GLOW_HEIGHT above the base, and moves along it at GLOW_SPEED a step */
#define GLOW_REACH 23.0F
#define GLOW_HEIGHT 27.0F
#define GLOW_SPEED 3.0F
/* the speed the drawn path's fall gains at each point, and how far below the
 * base the last segment starts */
#define PATH_FALL 0.08F
#define PATH_DEPTH 200.0F

#define SE_FLY 557

/* an elevation, a heading and a speed to fire at */
struct cannon_setting {
    Angle elevation;
    Angle heading;
    Float speed;
};

/* setting[0] is the placement's, which the cannon fires by. setting[1] is
 * the preset's, which nothing reads */
struct cannon_settings {
    struct cannon_setting setting[2];
    Sint32 unk_18;
};

/* a preset, by the id a placement's ang.z gives. -1 ends the table */
struct cannon_preset {
    Sint32 id;
    Angle heading;
    Angle elevation;
    Float speed;
    Sint32 unk_10;
};

/* the cannon's own layout of task_work */
struct cannon_work {
    Sint8 mode;
    Sint8 unk_01;
    Uint8 unk_02;
    Uint8 held; /* a bit for each player it holds, player 0's lowest */
    Sint16 flags;
    Uint16 timer;
    Sint32 unk_08;
    NJS_OBJECT *object; /* its own copy of cannon_object's tree */
    struct cannon_settings *settings;
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT3 scl;
    struct collision_work *collision;
    struct event_work *event;
};

#define CANNON(task) ((struct cannon_work *)(task)->work)

enum cannon_mode {
    CANNON_EDIT, /* made while the set editor holds it */
    CANNON_WAIT,
    CANNON_LOAD,
    CANNON_AIM, /* raising the barrel to fire */
    CANNON_FIRED
};

enum shot_mode { SHOT_INIT, SHOT_FLY };

static void cannon_shot_step(struct task *task);

static struct collision_shape cannon_shapes[] = {
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {3.802F, 17.0F, 9.178F},
     5.358F,
     17.0F,
     3.0F,
     {0, 0x1000, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {9.178F, 17.0F, 3.802F},
     5.358F,
     17.0F,
     3.0F,
     {0, 0x3000, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {9.178F, 17.0F, -3.802F},
     5.358F,
     17.0F,
     3.0F,
     {0, 0x5000, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {3.802F, 17.0F, -9.178F},
     5.358F,
     17.0F,
     3.0F,
     {0, 0x7000, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-3.802F, 17.0F, -9.178F},
     5.358F,
     17.0F,
     3.0F,
     {0, 0x9000, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-9.178F, 17.0F, -3.802F},
     5.358F,
     17.0F,
     3.0F,
     {0, 0xb000, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-9.178F, 17.0F, 3.802F},
     5.358F,
     17.0F,
     3.0F,
     {0, 0xd000, 0},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {-3.802F, 17.0F, 9.178F},
     5.358F,
     17.0F,
     3.0F,
     {0, 0xf000, 0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 27.0F, -20.0F},
     5.8F,
     3.0F,
     0.0F,
     {0x4000, 0, 0},
     0,
     NULL},
    {SHAPE_CAPSULE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 36.0F, -13.0F},
     3.0F,
     5.0F,
     0.0F,
     {0, 0, 0x4000},
     0,
     NULL},
    {SHAPE_RECTANGLE,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 44.0F, 14.0F},
     7.0F,
     7.0F,
     0.001F,
     {0, 0, 0},
     0,
     NULL},
};
static struct cannon_preset cannon_presets[] = {
    {2, 0, 0x1000, 2.0F, 0},
    {5, 0x8000, 0x2000, 3.0F, 1},
    {-1, 0, 0, 0.0F, 2},
};
/* the glow it puts out as it fires, which cannon_fire places */
static struct dust_params cannon_glow = {
    3.0F,
    0.038F,
    0.19F,
    0.06F,
    0.02F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};
static struct collision_shape cannon_shot_shape = {
    SHAPE_SPHERE,
    SHAPE_HURTS_OBJECTS | ATTACK(2),
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    10.0F,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};

/* @hack spelling: the starts of CANNON_LOAD, CANNON_AIM and CANNON_WAIT
 * through inline functions. SHC expands each where it is called, copying task
 * to R4 and loading its work through the copy (0x0c924e3a, 0x0c924f2a,
 * 0x0c924fd2, 0x0c9250c2). stored through task in place of the calls, summed
 * over cannon_wait, cannon_load, cannon_fired and egg_carrier_cannon_step,
 * 60 of 278 words differ */
/* @hack spelling: the pragma, which has SHC expand cannon_start_load in
 * cannon_wait; without it, SHC writes cannon_start_load out ahead of the
 * other functions and calls it, and 36 of cannon_wait's 77 words differ */
#pragma inline(cannon_start_load)
static void cannon_start_load(struct task *task)
{
    struct cannon_work *work = CANNON(task);

    work->mode = CANNON_LOAD;
    work->held = 0;
}

static void cannon_start_aim(struct task *task)
{
    struct cannon_work *work = CANNON(task);

    work->mode = CANNON_AIM;
}

static void cannon_start_wait(struct task *task)
{
    struct cannon_work *work = CANNON(task);

    work->mode = CANNON_WAIT;
}

/* the index of the setting the cannon fires by, which is always 0 */
static int cannon_setting_index(void)
{
    int index;

    /* @hack redundant: index set in an && whose left side is 1, which SHC
     * keeps at sp+0 and scales by 12 with EXTS.B in cannon_get_setting
     * (0x0c9246aa to 0x0c9246ba); as return 0, 14 of cannon_get_setting's 14
     * words differ, and as int index = 0, 13 */
    1 && (index = 0);
    return index;
}

/* a copy of source's tree, with nodes of its own from the heap and source's
 * models. null for a null source */
static NJS_OBJECT *cannon_copy_tree(NJS_OBJECT *source)
{
    NJS_OBJECT *copy = NULL;

    if (source != NULL) {
        copy = heap_alloc(sizeof(NJS_OBJECT));
        *copy = *source;
        copy->child = cannon_copy_tree(source->child);
        copy->sibling = cannon_copy_tree(source->sibling);
    }
    return copy;
}

/* free a tree cannon_copy_tree made, leaving its models */
static void cannon_free_tree(NJS_OBJECT *object)
{
    if (object != NULL) {
        cannon_free_tree(object->child);
        cannon_free_tree(object->sibling);
        heap_free(object);
    }
}

/* set setting[0]'s elevation, heading and speed from the placement's ang.x,
 * ang.y and scl.x. the angles are taken as 16 bits, and the speed as no less
 * than 0 */
static void cannon_take_placement(struct task *task)
{
    /* @hack redundant: register on work, which keeps work in R4 and settings
     * in R5 (0x0c924682, 0x0c924686); without it, the two trade and 10 of 19
     * words differ */
    register struct cannon_work *work = CANNON(task);
    struct cannon_settings *settings = work->settings;

    settings->setting[0].heading = (Uint16)work->ang.y;
    settings->setting[0].elevation = (Uint16)work->ang.x;
    settings->setting[0].speed = work->scl.x;
    if (settings->setting[0].speed < 0.0F) {
        settings->setting[0].speed = 0.0F;
    }
}

static struct cannon_setting *cannon_get_setting(struct task *task)
{
    struct cannon_settings *settings = CANNON(task)->settings;
    struct cannon_setting *setting = &settings->setting[cannon_setting_index()];

    return setting;
}

/* set setting[1] from the preset whose id is ang.z, or from the table's end
 * when none is, then setting[0] from the placement */
static void cannon_load_settings(struct task *task)
{
    struct cannon_work *work = CANNON(task);
    struct cannon_settings *settings = work->settings;
    struct cannon_preset *preset;
    int id;

    for (preset = cannon_presets; (id = preset->id) != -1 && id != work->ang.z;
         preset++) {
    }
    settings->setting[1].heading = preset->heading;
    settings->setting[1].elevation = preset->elevation;
    settings->setting[1].speed = preset->speed;
    settings->unk_18 = preset->unk_10;
    cannon_take_placement(task);
}

static void cannon_destroy(struct task *task)
{
    struct cannon_work *work = CANNON(task);

    heap_free(work->settings);
    cannon_free_tree(work->object);
    /* @hack redundant: an empty test of placement_editing, which the image
     * calls and tests (TST at 0x0c92470e) before it returns; as a bare call,
     * which SHC makes a jump, 18 of 19 words differ */
    if (placement_editing(task)) {
    }
}

static void cannon_draw(struct task *task)
{
    struct cannon_work *work = CANNON(task);

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    texture_set_stage();
    draw_tree(work->object);
    njPopMatrix(1);
}

static void cannon_set_barrel(struct task *task, Angle ang)
{
    CANNON(task)->object->child->ang[0] = ang;
}

/* raise the barrel by BARREL_STEP, to no more than setting[0]'s elevation.
 * 1 once it stops there, else 0 */
static int cannon_raise_barrel(struct task *task)
{
    int done = 0;
    /* @hack redundant: register on copy, which keeps 0 in R6 and copy in R5
     * (0x0c924792, 0x0c924794); without it, the two trade and 4 of 35 words
     * differ */
    register struct task *copy = task;
    /* @hack redundant: register on work, which keeps work in R5
     * (0x0c92479c); without it, work takes R6 and 9 of 35 words differ */
    register struct cannon_work *work;
    int index;
    Angle ang;
    Angle limit;

    /* @hack redundant: index set as cannon_setting_index sets it, which SHC
     * keeps at sp+0 and scales by 12 with EXTS.B (0x0c924798 to 0x0c9247ac);
     * as index = 0, the function runs 70 bytes short and 34 of 35 words
     * differ */
    1 && (index = 0);
    /* @hack redundant: a never-run test of copy, a copy of task, which keeps
     * the copy (MOV R4,R5 at 0x0c924794); without it, 30 of 35 words
     * differ */
    if (1) {
    } else if (copy) {
    }
    work = CANNON(task);
    limit = work->settings->setting[index].elevation;
    ang = work->object->child->ang[0] + BARREL_STEP;
    if (ang > limit) {
        ang = limit;
        done = 1;
    }
    cannon_set_barrel(task, ang);
    return done;
}

static void cannon_lower_barrel(struct task *task)
{
    Angle ang = CANNON(task)->object->child->ang[0] - BARREL_STEP;

    if (ang < 0) {
        ang = 0;
    }
    cannon_set_barrel(task, ang);
}

/* set the barrel straight to setting[0]'s elevation */
static void cannon_set_barrel_to_setting(struct task *task)
{
    cannon_set_barrel(task, CANNON(task)->settings->setting[0].elevation);
}

static void cannon_set_hatch(struct task *task, Angle ang)
{
    CANNON(task)->object->child->child->ang[0] = ang;
}

static void cannon_close_hatch(struct task *task)
{
    Angle ang = CANNON(task)->object->child->child->ang[0] - HATCH_STEP;

    if (ang < 0) {
        ang = 0;
    }
    cannon_set_hatch(task, ang);
}

static void cannon_open_hatch(struct task *task)
{
    Angle ang = CANNON(task)->object->child->child->ang[0] + HATCH_STEP;

    if (ang > HATCH_OPEN) {
        ang = HATCH_OPEN;
    }
    cannon_set_hatch(task, ang);
}

/* shut the hatch by a quarter of how far it stands from HATCH_SLAM, a step
 * that grows as it shuts. 1 once it reaches 0, else 0 */
static int cannon_slam_hatch(struct task *task)
{
    int done = 0;
    Angle ang = CANNON(task)->object->child->child->ang[0];

    ang -= (HATCH_SLAM - ang) / 4;
    if (ang < 0) {
        ang = 0;
        done = 1;
    }
    cannon_set_hatch(task, ang);
    return done;
}

/* set how far along z the recoiling part stands from its rest */
static void cannon_set_recoil(struct task *task, Float recoil)
{
    CANNON(task)->object->child->child->sibling->pos[2] = recoil;
}

/* bring the recoiling part back toward its rest by RECOIL_STEP. 1 once it
 * is there, else 0 */
static int cannon_recover(struct task *task)
{
    int done = 0;
    Float recoil = CANNON(task)->object->child->child->sibling->pos[2];

    recoil -= RECOIL_STEP;
    if (recoil < 0.0F) {
        recoil = 0.0F;
        done = 1;
    }
    cannon_set_recoil(task, recoil);
    return done;
}

/* turn the cannon straight to setting[0]'s heading */
static void cannon_face(struct task *task)
{
    struct task *copy = task;
    struct cannon_work *work;
    int index;

    /* @hack redundant: a never-run test of copy, a copy of task, which keeps
     * the copy (MOV R4,R5 at 0x0c924958); without it, the function runs 2
     * bytes short and 15 of 17 words differ */
    if (1) {
    } else if (copy) {
    }
    /* @hack redundant: index set as cannon_setting_index sets it, at sp+0 and
     * scaled by 12 with EXTS.B (0x0c92495a to 0x0c92496a); as index = 0, the
     * function runs 22 bytes short and 17 of 17 words differ */
    1 && (index = 0);
    work = CANNON(task);
    /* @hack redundant: a never-run test of task, which keeps task in R4 while
     * work takes R5 (0x0c92495e); without it, 5 of 17 words differ */
    if (1) {
    } else if (task) {
    }
    work->ang.y = work->settings->setting[index].heading;
}

/* turn the cannon toward setting[0]'s heading the short way, by at most
 * TURN_STEP */
static void cannon_turn(struct task *task)
{
    struct cannon_work *work = CANNON(task);
    struct task *copy = task;
    int index;
    Angle target;
    int delta;

    /* @hack redundant: a never-run test of copy, a copy of task, which keeps
     * the copy (MOV R4,R6 at 0x0c924982); without it, SHC holds work in R14
     * and reads it once, the function runs 2 bytes long and 32 of 38 words
     * differ */
    if (1) {
    } else if (copy) {
    }
    /* @hack redundant: index set as cannon_setting_index sets it, at sp+0 and
     * scaled by 12 with EXTS.B (0x0c92497c to 0x0c92498e); as index = 0, the
     * function runs 10 bytes short and 38 of 38 words differ */
    1 && (index = 0);
    /* @hack spelling: the setting through task, whose work the image loads
     * again (0x0c924984); through work, 23 of 38 words differ */
    target = CANNON(task)->settings->setting[index].heading;
    delta = (Uint16)(target - work->ang.y);
    if (delta < 0x8000) {
        if (delta < TURN_STEP) {
            work->ang.y = target;
        } else {
            work->ang.y += TURN_STEP;
        }
    } else {
        if (delta > 0x10000 - TURN_STEP) {
            work->ang.y = target;
        } else {
            work->ang.y -= TURN_STEP;
        }
    }
}

/* fling each player the cannon holds along setting[0]'s line at its speed,
 * and make a shot task for each. then put out the glow along the same line,
 * and recoil */
static void cannon_fire(struct task *task)
{
    struct cannon_work *work = CANNON(task);
    int count = player_count();
    int i;
    struct cannon_setting *setting;
    NJS_VECTOR line;
    Float cosine;
    Float sine;
    Float cos_y;
    Float sin_y;

    work->mode = CANNON_FIRED;
    work->timer = 0;
    for (i = 0; i < count; i++) {
        if (work->held & (1 << i)) {
            /* the line is the heading turned a half turn, raised by the
             * elevation */
            setting = cannon_get_setting(task);
            line.x =
                njCos(setting->elevation) * njSin(setting->heading + 0x8000);
            line.y = njSin(setting->elevation);
            line.z =
                njCos(setting->elevation) * njCos(setting->heading + 0x8000);
            player_fling(i, setting->speed, &line);
            task_create(TASK_WORK, 3, cannon_shot_step);
        }
    }
    setting = cannon_get_setting(task);
    cosine = njCos(setting->elevation);
    sine = njSin(setting->elevation);
    cos_y = njCos(setting->heading + 0x8000);
    sin_y = njSin(setting->heading + 0x8000);
    cannon_glow.pos.x = work->pos.x + (GLOW_REACH * (cosine * sin_y));
    cannon_glow.pos.y = work->pos.y + (GLOW_REACH * sine) + GLOW_HEIGHT;
    cannon_glow.pos.z = work->pos.z + (GLOW_REACH * (cosine * cos_y));
    cannon_glow.speed.x = GLOW_SPEED * (cosine * sin_y);
    cannon_glow.speed.y = GLOW_SPEED * sine;
    cannon_glow.speed.z = GLOW_SPEED * (cosine * cos_y);
    dust_create_glow(&cannon_glow);
    cannon_set_recoil(task, RECOIL);
}

/* draw the path setting[0] gives, in the cannon's frame, as a line from point
 * to point. the first point is 29 above the base, and the path leaves it
 * along -z at the setting's speed and elevation. the path's fall gains
 * PATH_FALL at each point. the last line starts at the first point PATH_DEPTH
 * or more below the base. each point is opaque red, with a green that climbs
 * from 0 over 16 points and starts over. each climb starts a point further
 * out every third call */
static void cannon_draw_path(struct task *task)
{
    struct cannon_work *work = CANNON(task);
    int phase = -(work->timer++) / 3;
    struct cannon_settings *settings = work->settings;
    NJS_POINT3 points[2] = {
#include "adv0100/cannon/points.inc"
    };
    /* SHC refuses inner braces in a union's initializer, with error 2129 */
    /* NOLINTNEXTLINE(clang-diagnostic-missing-braces) */
    NJS_COLOR colors[2] = {0, 0};
    NJS_POINT3COL line;
    Float speed_z =
        -(settings->setting[0].speed * njCos(settings->setting[0].elevation));
    Float speed_y =
        settings->setting[0].speed * njSin(settings->setting[0].elevation);

    line.p = points;
    line.col = colors;
    line.num = 2;
    while (points[0].y > -PATH_DEPTH) {
        phase = (phase + 1) & 15;
        /* @hack redundant: both speeds times one, which the image multiplies
         * too, FLDI1 at 0x0c924c36 and FMUL at 0x0c924c3a and 0x0c924c42;
         * without them, 96 of 167 words differ */
        speed_z *= 1.0F;
        speed_y *= 1.0F;
        speed_y -= PATH_FALL;
        points[0] = points[1];
        points[1].z += speed_z;
        points[1].y += speed_y;
        colors[0] = colors[1];
        colors[1].color = (phase << 12) | 0xffff0000;
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        njDrawLine3D(&line, 1, 0);
        njPopMatrix(1);
    }
}

/* print the set editor's labels for the placement's fields, unless pad 1
 * holds both X and Y */
static void cannon_label(void)
{
    if (~pad_records[1].on & (PDD_DGT_TX | PDD_DGT_TY)) {
        njPrintSize(16);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "< GYOUKAKU >");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "< HOUGAKU >");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "< TAIHO ID >");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "< SHOSOKUDO >");
    } else {
        njPrintSize(16);
    }
}

static void cannon_edit(struct task *task)
{
    cannon_take_placement(task);
    cannon_set_barrel_to_setting(task);
    /* cannon_run has drawn the cannon this step already, and this draws it
     * again */
    cannon_draw(task);
    cannon_draw_path(task);
    cannon_label();
}

static void cannon_wait(struct task *task)
{
    /* @hack redundant: register on work, which puts task in R13 and work in
     * R14 (MOV R4,R13 at 0x0c924de4); without it, the two trade and 13 of 77
     * words differ */
    register struct cannon_work *work = CANNON(task);
    struct task_work *player = player_work[player_nearest(&work->pos)];
    /* @hack redundant: register on x, which keeps x in FR4 (FMOV.S
     * @(R0,R4),FR4 at 0x0c924dfc); without it, x takes FR6, y FR5 and z FR4,
     * and 12 of 77 words differ */
    register Float x = player->pos.x - work->pos.x;
    Float y = player->pos.y - work->pos.y;
    /* @hack redundant: register on z, which keeps z in FR5 and y in FR6
     * (0x0c924e00 and 0x0c924e0a); without it, the two trade and 8 of 77
     * words differ */
    register Float z = player->pos.z - work->pos.z;
    Float distance = (x * x) + (z * z);

    if (distance < SQUARE(LOAD_RADIUS) && y > 0.0F && y < LOAD_HEIGHT) {
        if (get_character() == CHARACTER_SONIC) {
            cannon_start_load(task);
        }
    } else if (distance < SQUARE(HATCH_NEAR)) {
        if (get_character() == CHARACTER_SONIC) {
            cannon_open_hatch(task);
        }
        collision_register((struct task_work *)work);
    } else {
        cannon_close_hatch(task);
    }
    cannon_turn(task);
    cannon_lower_barrel(task);
}

static void cannon_load(struct task *task)
{
    struct cannon_work *work = CANNON(task);
    int count = player_count();
    int i;

    for (i = 0; i < count; i++) {
        if (!(work->held & (1 << i))) {
            /* @hack spelling: the cannon's work again, through task, on each
             * pass (0x0c924ebc); through work, the function runs 2 bytes
             * short and 69 of 105 words differ */
            struct cannon_work *cannon = CANNON(task);
            struct task_work *player = player_work[i];
            Float x = player->pos.x - cannon->pos.x;
            Float y = player->pos.y - cannon->pos.y;
            Float z = player->pos.z - cannon->pos.z;
            Float distance = (x * x) + (z * z);

            if (distance < SQUARE(LOAD_RADIUS) && y > 0.0F && y < LOAD_HEIGHT) {
                /* @hack redundant: player read again, as the image reads it
                 * (0x0c924ef6); without it, the function runs 4 bytes short
                 * and 80 of 105 words differ */
                player = player_work[i];
                player->pos.x = cannon->pos.x;
                player->pos.y = cannon->pos.y + LOAD_HEIGHT;
                player->pos.z = cannon->pos.z;
                player_set_mode(i, PLAYER_MODE_STILL);
                cannon->held |= 1 << i;
            }
        }
    }
    if (cannon_slam_hatch(task)) {
        cannon_start_aim(task);
    }
    collision_register((struct task_work *)work);
}

static void cannon_aim(struct task *task)
{
    struct task_work *work = task->work;

    if (cannon_raise_barrel(task)) {
        sound_play(SE_BLAST, 0, 0, 0);
        cannon_fire(task);
    }
    collision_register(work);
}

static void cannon_fired(struct task *task)
{
    if (CANNON(task)->timer > FIRED_STEPS) {
        cannon_start_wait(task);
    }
    if (cannon_recover(task)) {
        collision_register(task->work);
    }
    CANNON(task)->timer++;
}

static void cannon_run(struct task *task)
{
    if (!placement_try_leave(task)) {
        cannon_draw(task);
        switch (task->work->mode) {
        case CANNON_EDIT:
            cannon_edit(task);
            break;
        case CANNON_WAIT:
            cannon_wait(task);
            break;
        case CANNON_LOAD:
            cannon_load(task);
            break;
        case CANNON_AIM:
            cannon_aim(task);
            break;
        case CANNON_FIRED:
            cannon_fired(task);
            break;
        }
    }
}

void egg_carrier_cannon_step(struct task *task)
{
    struct cannon_work *work = CANNON(task);
    struct cannon_settings *settings = heap_alloc(sizeof *settings);

    work->settings = settings;
    work->object = cannon_copy_tree(&cannon_object);
    cannon_load_settings(task);
    cannon_set_barrel(task, 0);
    cannon_set_hatch(task, 0);
    cannon_face(task);
    if (placement_editing(task)) {
        /* @hack spelling: a copy of task, which SHC moves to R4 first,
         * leaving the store for the slot of the call (0x0c92508e to
         * 0x0c925094); through task, 3 of 69 words differ */
        struct task *copy = task;
        struct cannon_work *cannon = CANNON(copy);

        cannon->mode = CANNON_EDIT;
        cannon_set_barrel_to_setting(copy);
    } else {
        work->ang.x = 0;
        work->ang.z = 0;
        work->scl.x = 0.0F;
        work->scl.y = 0.0F;
        work->scl.z = 0.0F;
        cannon_start_wait(task);
        collision_init(task, cannon_shapes, COLLISION_OBJECT, 1, 11);
    }
    task->destructor = cannon_destroy;
    task->step = cannon_run;
    task->draw = cannon_draw;
}

/* follow player 0 with the shot's sphere, and from the shot's second step
 * with its sound, until player 0 is on the ground */
static void cannon_shot_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];

    switch (work->mode) {
    case SHOT_INIT:
        collision_init(task, &cannon_shot_shape, COLLISION_OBJECT_MUTUAL, 1, 1);
        work->collision->radius = 40.0F;
        work->mode = SHOT_FLY;
        break;
    case SHOT_FLY:
        sound_play_for(SE_FLY, (int)work, 1, 0, 2);
        if (player->flags & PLAYER_ON_GROUND) {
            task_kill(task);
            return;
        }
        break;
    }
    work->pos = player->pos;
    collision_register(work);
}
