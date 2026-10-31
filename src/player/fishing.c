/* the fishing game: Big's target and lure, their camera, the hud and score */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/mesh.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "hud/board.h"
#include "hud/hud.h"
#include "main.h"
#include "object/item_box.h"
#include "object/item_box_air.h"
#include "object/level_item.h"
#include "player/big.h"
#include "player/big_models.h"
#include "player/fish.h"
#include "player/fishing.h"
#include "player/fishing_line.h"
#include "player/player.h"
#include "progress/progress.h"
#include "save/card.h"
#include "save/save.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 11
#include "player/fishing/text.inc"

#define SE_CAST 797
#define SE_LURE_WATER 798
#define SE_LURE_GROUND 799
#define SE_REEL 800
#define SE_LURE_BOTTOM 809
#define SE_STRAIN 810
#define SE_CAST_SWING 1208

/* Big's voice line once the frog's catch has shown, at Twinkle Park and
 * Ice Cap */
#define VOICE_FROG 0x727

/* Big's act of Ice Cap, where he fishes in pens */
#define ICECAP_POND STAGE_ACT(STAGE_ICECAP, 3)

/* the depth the fishing game's hud draws at, each sprite a little apart */
#define HUD_DEPTH (-1.8F)

/* how far the lure flies a step, in x and z */
#define CAST_SPEED 3.0F

/* the reel's buttons held this step, as bits. an action button reels in
 * slowly, and a jump button fast, with an action button or without */
#define HELD_ACTION 0x1
#define HELD_JUMP 0x2

/* a trigger's pull, 0 to 255, on a stick's signed scale: -0x8000 at rest */
#define TRIGGER_AXIS(pull) (((pull) << 8) - 0x8000)

enum lure_mode {
    LURE_INIT,
    LURE_DANGLE, /* hanging from the rod's tip while Big aims */
    LURE_WAIT, /* dangling 10 steps after the cast, then thrown */
    LURE_FLY,
    LURE_LANDED, /* just down in the water, for the one step that moves it on */
    LURE_IN_WATER,
    LURE_FISH, /* with a fish on */
    LURE_BACK, /* back to the rod with no fish, 10 a step */
    LURE_CATCH /* back to the rod with a fish on, 2 a step */
};

enum target_mode { TARGET_INIT, TARGET_AIM };

enum hud_mode { HUD_INIT, HUD_SHOW };

enum banner_mode {
    BANNER_INIT,
    BANNER_IN,
    BANNER_WAIT,
    BANNER_OUT,
    BANNER_DONE
};

enum splash_mode { SPLASH_INIT, SPLASH_PLAY, SPLASH_FADE };

enum catch_mode { CATCH_INIT, CATCH_SHOW, CATCH_LEAVE };

/* the target task's extra */
struct target_extra {
    Float distance; /* how far out from player 0 it is */
    Float ground; /* the ground's height under it */
    Sint16 heading; /* Big's heading as it was put out */
    Uint8 unk_0a[2];
    /* the ground's slope under it, which its mark tilts by */
    Sint16 slope_x;
    Sint16 slope_z;
};

#define TARGET(task) ((struct target_extra *)(task)->extra)

/* the lure model's turns that lure_extra's sway keeps. SWAY_PHASE is the
 * phase of the model's rock about x. SWAY_ROLL is its turn about z, which
 * it takes on the bottom as its turn about x. SWAY_BOTTOM_Z is its turn
 * about z on the bottom, which nothing sets */
enum { SWAY_BOTTOM_Z, SWAY_PHASE, SWAY_ROLL };

/* the lure task's extra */
struct lure_extra {
    Uint8 unk_00[2];
    Sint16 sway[3];
    /* the line, from the heap */
    struct fishing_line *line;
    NJS_POINT3 *goal; /* where it flies to; freed with the lure */
};

#define LURE(task) ((struct lure_extra *)(task)->extra)

/* the lure task's work->vars */
struct lure_vars {
    Uint8 unk_00[4];
    /* the steps left to dangle after the cast. in flight, the steps flown,
     * which nothing reads */
    Sint32 steps;
};

#define LURE_VARS(work) ((struct lure_vars *)&(work)->vars)

/* a surface fishing_find_surfaces found */
struct near_surface {
    Sint32 flags; /* its object's WORLD_ flags */
    Angle ang_x; /* how it slopes */
    Angle ang_z;
    Float height;
};

/* the surfaces nearest a point, which fishing_find_surfaces' skip may leave
 * water out of. with none found, above's height is GROUND_FAR, below's
 * -GROUND_FAR and water's GROUND_FAR */
struct near_surfaces {
    struct near_surface above; /* the lowest above the point */
    struct near_surface below; /* the highest at or below it */
    struct near_surface water; /* the nearest water, above or below */
};

/* fishing_find_surfaces' skip, as bits: water left out of below, or out of
 * above. SKIP_NOTHING is a bit neither test reads, so water counts on both
 * sides */
#define SKIP_WATER_BELOW 0x1
#define SKIP_WATER_ABOVE 0x2
#define SKIP_NOTHING 0x4

/* the hit banner's work->vars. its pos.x is each half's distance from the
 * screen's middle */
struct banner_vars {
    Sint32 blinks; /* the steps fishing_banner_blink has run */
    Sint32 steps; /* the steps left to wait at the middle */
};

#define BANNER_VARS(work) ((struct banner_vars *)&(work)->vars)

/* a splash task's work->vars. its TASK_TIMER counts the steps to its next
 * sprite, or to its next fade */
struct splash_vars {
    Sint32 frame; /* the sprite it shows */
    Uint8 unk_04[4];
    Float alpha; /* 1, less a tenth for each fade */
};

#define SPLASH_VARS(work) ((struct splash_vars *)&(work)->vars)

/* a catch task's work->vars. its pos.x and pos.z hold where the weight
 * shows on the screen, and its scl.x the weight's size */
struct catch_vars {
    /* the fish's kind, or CATCH_SNAPPED for a fish the snapped line lost */
    Sint32 kind;
    Sint32 steps; /* the steps left to show the weight */
    Sint32 weight;
};

#define CATCH_VARS(work) ((struct catch_vars *)&(work)->vars)
#define CATCH_SNAPPED 999

/* a field's prize */
struct fishing_prize {
    Float chance; /* that fishing_give_prize gives it, from 0 to 1 */
    Sint32 kind; /* an item_kind, or PRIZE_NONE */
    void (*give)(void); /* or null */
};

#define PRIZE_NONE (-1)

/* how many ring counts a random-rings prize picks from: the same six as
 * item_box.c's random rings */
#define RANDOM_RING_COUNTS 6

extern NJS_OBJECT fishing_target_mark;
extern NJS_OBJECT fishing_target_object;
extern NJS_OBJECT big_lure_0;
extern NJS_OBJECT big_lure_1;
extern NJS_OBJECT big_lure_2;
extern NJS_OBJECT big_lure_3;
extern NJS_OBJECT big_lure_4;

static const Sint16 fishing_random_ring_counts[RANDOM_RING_COUNTS] = {
    1, 5, 10, 15, 20, 40};

/* the water's height at the lure, as the lure last found it. while Big
 * aims, the ground's height under the target */
static Float fishing_water = 0.0F;
/* the lure's distance from player 0, as fishing_measure_line last found
 * it */
static Float fishing_distance = 0.0F;
/* the line Big has out. the reels draw it in, and fishing_measure_line
 * keeps it from fishing_distance to 5 past it */
static Float fishing_line_out = 0.0F;
/* how hard the line pulls, from 0 to 1. at 1, the line snaps */
static Float fishing_tension = 0.0F;
/* the tension fishing_tension eases toward */
static Float fishing_tension_aim = 0.0F;
/* the reel's turn on the hud */
static Angle fishing_reel_ang = 0;
/* the steps till SE_STRAIN may play again */
static int fishing_warn_steps = 0;

/* the textures of a field's prizes. only fishing_prize_texlist names them,
 * and nothing reads that */
static NJS_TEXNAME fishing_prize_textures[] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0},
};
/* Big's lures, by how many he has */
static NJS_OBJECT *fishing_lures[] = {&big_lure_0, &big_lure_1, &big_lure_2,
                                      &big_lure_3, &big_lure_4};
/* how many of fishing_pens a cast can go into */
#define PENS 3
/* the pens of Ice Cap's pond. a lure cast within 70 of one of the first
 * PENS stays inside its walls, 40 out from it each way */
static NJS_POINT3 fishing_pens[] = {
    {1470.0F, -28.0F, 99.0F},
    {1924.37F, -20.0F, -30.0F},
    {2138.0F, -20.3F, -358.0F},
    {2165.4F, 252.26F, -88.23F},
};
static NJS_TEXLIST fishing_prize_texlist = {
#include "player/fishing/fishing_prize_texlist.inc"
};
/* the lure's shape, which hurts enemies */
/* clang-format off */
static struct collision_shape fishing_lure_shapes[] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(2) | GUARD(3) | SHAPE_HURTS_ENEMIES,
     PUSH_FIRM(PUSH_ALL), {2.0F, -1.0F, 0.0F}, 5.0F, 0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */
/* fishing_hud_texanims' sprites, which fishing_texlist holds */
enum hud_sprite {
    SPRITE_GAUGE, /* the tension gauge */
    SPRITE_REEL,
    /* castgauge2, the gauge's bar as a sprite, which nothing draws. a
     * polygon draws the bar */
    SPRITE_BAR,
    SPRITE_LIVES, /* zanki_r, which nothing draws */
    SPRITE_SCORE_LABEL,
    SPRITE_UNIT, /* the weight's unit */
    SPRITE_HIT_LEFT, /* the hit banner's halves */
    SPRITE_HIT_RIGHT,
    /* a field's label, in place of the score's: the lower part of its
     * texture */
    SPRITE_FIELD_LABEL,
    HUD_SPRITES
};

static NJS_TEXANIM fishing_hud_texanims[HUD_SPRITES] = {
#include "player/fishing/fishing_hud_texanims.inc"
};
/* the first 24 of con_regular_textures, as a texlist of the hud's own */
static NJS_TEXLIST fishing_regular_texlist = {
#include "player/fishing/fishing_regular_texlist.inc"
};
/* Big's head beside the lives, texture 19 of fishing_regular_texlist */
static NJS_TEXANIM fishing_head_texanims[1] = {
#include "player/fishing/fishing_head_texanims.inc"
};
static NJS_TEXANIM fishing_splash_texanims[8] = {
#include "player/fishing/fishing_splash_texanims.inc"
};

static void fishing_prize_5_rings(void);
static void fishing_prize_10_rings(void);
static void fishing_prize_random_rings(void);
static void fishing_prize_1up(void);

/* a field's prizes. fishing_give_prize draws from the second on, and the
 * sixth, PRIZE_NONE, takes the chance the rest leave */
static struct fishing_prize fishing_prizes[] = {
    {0.0F, PRIZE_NONE, NULL},
    {0.1F, ITEM_5RING, fishing_prize_5_rings},
    {0.05F, ITEM_10RING, fishing_prize_10_rings},
    {0.03F, ITEM_RANDOMRING, fishing_prize_random_rings},
    {0.01F, ITEM_1UP, fishing_prize_1up},
    {9.99F, PRIZE_NONE, NULL},
    {0.0F, 0, NULL},
};

int fishing_score;
Sint16 biggest_fish;
int biggest_fish_kind;
int fishing_record_weight;
Uint16 fishing_flags;
Float fishing_strain;
NJS_POINT3 fishing_cast_pos;
int fishing_fight_steps;
/* the phase of the ring count's pulse while there are no rings */
static Angle fishing_ring_blink;
/* the save fishing_save fills, then copies to the save slot */
static struct save fishing_save_copy;

static void fishing_target_draw(struct task *task);
static void fishing_target_destroy(struct task *task);
static void fishing_target_move(struct task *task);
static Bool fishing_steep(struct task *task, struct angles *ang);
static Bool fishing_upright(struct task *task, struct angles *ang);
static Bool fishing_out_of_reach(struct task *task, struct near_surfaces *info,
                                 Float x, Float y, Float z);
static void fishing_target_step(struct task *task);
static void fishing_lure_destroy(struct task *task);
static void fishing_banner_draw(struct task *task);
static void fishing_banner_blink(struct task *task);
static void fishing_hud_show(struct task *task);
static void fishing_hud_draw(struct task *task);
static void fishing_draw_reel(void);
static void fishing_draw_tension(void);
static void fishing_draw_line_out(Float distance);
static void fishing_draw_score(int score);
static void fishing_measure_line(NJS_POINT3 *from, NJS_POINT3 *to);
static void fishing_lure_show(struct task *task);
static void fishing_lure_draw(struct task *task);
static void fishing_rod_tip(struct task *task, NJS_POINT3 *pos);
static Bool fishing_lure_fly(struct task *task);
static Bool fishing_lure_return(struct task *task);
static Bool fishing_lure_at_rod(NJS_POINT3 *tip, NJS_POINT3 *pos);
static void fishing_lure_drift(struct task *task);
static void fishing_reel_slow(struct task *task, NJS_POINT3 *tip);
static void fishing_reel_fast(struct task *task, NJS_POINT3 *tip);
static void fishing_reel_turning(struct task *task, NJS_POINT3 *tip);
static void fishing_tension_step(struct task *task, NJS_VECTOR *move);
static Bool fishing_fish_pulls(struct task *task);
static Bool fishing_in_pen(void);
static void fishing_lure_swim(struct task *task);
static void fishing_lure_dangle(struct task *task, NJS_POINT3 *tip);
static void fishing_lure_reset(struct task *task);
static void fishing_lure_leave_water(struct task *task);
static Bool fishing_lure_in_water(struct task *task,
                                  struct near_surfaces *info);
static Bool fishing_mark_shelter_plug(void);
static void fishing_lure_step(struct task *task);
static void fishing_lure_home(struct task *task);
static Float fishing_find_surfaces(Float x, Float y, Float z, int skip,
                                   struct near_surfaces *info);
static Float fishing_find_surfaces_at(struct task_work *work, int skip,
                                      struct near_surfaces *info);
static void fishing_lure_sway(struct task *task);
static void fishing_lure_pitch(struct task *task);
static void fishing_lure_yaw(struct task *task);
static void fishing_lure_roll(struct task *task);
static void fishing_line_end(NJS_POINT3 *pos);
static void fishing_set_draw(void);
static void fishing_camera_step_point(struct camera_view *view);
static void fishing_target_camera(struct task *task);
static void fishing_lure_camera(struct task *task);
static void fishing_look_at_lure(struct task *task);
static void fishing_camera_clear(NJS_POINT3 *from, NJS_POINT3 *to);
static void fishing_camera_step_lure(struct camera_view *view);
static void fishing_banner_step(struct task *task);
static struct task *fishing_splash_create(struct task *parent);
static void fishing_splash_draw(struct task *task);
static void fishing_splash_destroy(struct task *task);
static void fishing_splash_step(struct task *task);
static int fishing_give_prize(void);
static Bool fishing_pen_sweep(NJS_POINT3 *pos, NJS_VECTOR *speed,
                              struct angles *ang, Float radius);
static Bool fishing_sweep(NJS_POINT3 *pos, NJS_VECTOR *speed,
                          struct angles *ang, Float radius);
static void fishing_score_catch(Float weight, int kind);
static void fishing_catch_draw(struct task *task);
static void fishing_catch_destroy(struct task *task);
static void fishing_catch_step(struct task *task);
static int fishing_save_stage(void);
static void fishing_save_score(struct save *save, int stage, Sint16 score);
static void fishing_save_fish(struct save *save, int stage, Sint16 weight,
                              int kind);
static void fishing_save(void);
static Float fishing_load_record(void);

struct task *fishing_create_target(struct task *big)
{
    struct task *task;

    task = task_create(TASK_MOTION | TASK_WORK | TASK_EXTRA, 2,
                       fishing_target_step);
    if (task != NULL) {
        task->work->pos = big->work->pos;
        task->work->ang = big->work->ang;
        TARGET(task)->heading = big->work->ang.y;
        TARGET(task)->distance = 20.0F;
        task->work->pos.x += njCos(task->work->ang.y) * TARGET(task)->distance;
        task->work->pos.y += 20.0F;
        task->work->pos.z += njSin(task->work->ang.y) * TARGET(task)->distance;
    }
    return task;
}

/* draw the target's mark on the ground, tilted to its slope, and the
 * target. the quad it fills is never drawn */
static void fishing_target_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct target_extra *extra = task->extra;
    NJS_POINT3COL quad;
    NJS_POINT3 points[4];
    NJS_COLOR colors[4];
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a point. the image's ADD #-100,R15 reserves them in its
     * 124-byte frame (0x8c0f0cf2); without them, 5 of 168 words differ */
    NJS_POINT3 unused;
    Float ground;
    Angle ang;
    NJS_COLOR *color;
    int i;

    quad.p = points;
    quad.col = colors;
    quad.num = 4;
    quad.p[0].x = -5.0F;
    quad.p[0].z = -5.0F;
    quad.p[1].x = 5.0F;
    quad.p[1].z = -5.0F;
    quad.p[2].x = 5.0F;
    quad.p[2].z = 5.0F;
    quad.p[3].x = -5.0F;
    quad.p[3].z = 5.0F;
    ground = extra->ground;
    quad.p[0].y = quad.p[1].y = quad.p[2].y = quad.p[3].y = 0.0F;
    color = quad.col;
    for (i = 0; i < 4; i++) {
        color[i].color = 0x80ffffff;
    }
    quad.tex = NULL;
    njSetTexture(&fishing_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, ground + 3.0F, work->pos.z);
    TURN_VIA(njRotateZ, extra->slope_z, ang);
    TURN_VIA(njRotateX, extra->slope_x, ang);
    TURN(njRotateY, -work->ang.y);
    draw_tree(&fishing_target_mark);
    njPopMatrix(1);
    color = quad.col;
    for (i = 0; i < 4; i++) {
        color[i].color = 0x80ff0000;
    }
    /* @hack redundant: color set to colors after both loops, though nothing
     * reads it, which has each loop load color from quad.col (MOV.L
     * @(4,R14),R4 at 0x8c0f0d66 and 0x8c0f0de0); without it, 127 of 168 words
     * differ */
    color = colors;
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN(njRotateY, work->ang.y);
    draw_tree(&fishing_target_object);
    njPopMatrix(1);
}

static void fishing_target_destroy(struct task *task)
{
}

/* move the target the way the stick points, 3 a step, onto the ground there. it
 * stays where it is when the place is out of reach, or the move met a wall. the
 * stick is that of the player its vars name, which nothing sets: player 0. the
 * target stands 20 above the ground, but at most 49.9 above player 0 */
static void fishing_target_move(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct target_extra *extra = task->extra;
    Angle ang = pad_sticks[PLAYER_VARS(work)->player].ang;
    Float stroke = pad_sticks[PLAYER_VARS(work)->player].stroke;
    struct angles slope;
    NJS_POINT3 pos;
    NJS_VECTOR move;
    struct near_surfaces info;
    Float x;
    Float y;
    Float z;

    slope.x = slope.y = slope.z = 0;
    pos = work->pos;
    if (stroke == 0.0F) {
        move.x = 0.0F;
        move.y = 0.0F;
        move.z = 0.0F;
    } else {
        move.x = njCos(ang) * 3.0F;
        move.y = 0.0F;
        move.z = njSin(ang) * 3.0F;
        world_sweep_all_hit(&pos, &move, &slope, 1.0F);
    }
    x = work->pos.x + move.x;
    y = work->pos.y + move.y;
    z = work->pos.z + move.z;
    fishing_find_surfaces(x, y, z, SKIP_NOTHING, &info);
    y = info.below.height;
    if (fishing_out_of_reach(task, &info, x, y, z) == FALSE &&
        fishing_steep(NULL, &slope) == FALSE) {
        work->pos.x = x;
        work->pos.z = z;
        extra->ground = y;
        extra->slope_x = info.below.ang_x;
        extra->slope_z = info.below.ang_z;
        y += 20.0F;
        if (y >= player->pos.y + 49.9F) {
            work->pos.y = player->pos.y + 49.9F;
        } else {
            work->pos.y = y;
        }
    }
    move.x = work->pos.x - player->pos.x;
    move.z = work->pos.z - player->pos.z;
    extra->distance = njScalor(&move);
}

/* TRUE when a surface sloped by ang is a wall or hangs over: its normal's
 * y is at most 0.01. with a task, TRUE only if the normal's dot with the
 * way from player 0 to the task also passes 0.7. fishing_target_move, the
 * one caller, passes no task */
static Bool fishing_steep(struct task *task, struct angles *ang)
{
    NJS_VECTOR up;
    NJS_VECTOR axis;
    NJS_MATRIX matrix;
    Float dot;

    up.x = up.z = axis.x = axis.z = 0.0F;
    up.y = axis.y = 1.0F;
    njUnitMatrix(&matrix);
    njRotateXYZ(&matrix, ang->x, ang->y, ang->z);
    njCalcVector(&matrix, &up, &up);
    dot = njInnerProduct(&up, &axis);
    if (dot <= 0.01F) {
        if (task == NULL) {
            return TRUE;
        }
        axis.x = task->work->pos.x - player_work[0]->pos.x;
        axis.y = 0.0F;
        axis.z = task->work->pos.z - player_work[0]->pos.z;
        njUnitVector(&axis);
        dot = njInnerProduct(&up, &axis);
        if (dot > 0.7F) {
            return TRUE;
        }
    }
    return FALSE;
}

/* TRUE when a surface sloped by ang stands straight up: its normal's y
 * lies within 0.01 of 0. task goes unused */
static Bool fishing_upright(struct task *task, struct angles *ang)
{
    NJS_VECTOR up;
    NJS_VECTOR axis;
    NJS_MATRIX matrix;
    Float dot;

    up.x = up.z = axis.x = axis.z = 0.0F;
    up.y = axis.y = 1.0F;
    njUnitMatrix(&matrix);
    njRotateXYZ(&matrix, ang->x, ang->y, ang->z);
    njCalcVector(&matrix, &up, &up);
    dot = njInnerProduct(&up, &axis);
    if (dot > 0.01F || dot < -0.01F) {
        return FALSE;
    }
    return TRUE;
}

/* TRUE when the target can't go to (x, y, z), on the ground info found
 * there. the point must lie 18 to 250 from player 0 in x and z, or 18 to
 * 350 with the power rod. its bearing from him must lie within 0x2b00,
 * about 60 degrees, of the target's heading. for some headings the test
 * passes more, as its tag says. the ground must slope at most a quarter
 * turn each way, and lie within 50 of his height */
static Bool fishing_out_of_reach(struct task *task, struct near_surfaces *info,
                                 Float x, Float y, Float z)
{
    struct task_work *player = player_work[0];
    struct player *record = player_motion[0]->record;
    NJS_VECTOR away;
    Float distance;
    Float reach;
    Float height;
    int ang;

    away.x = x - player->pos.x;
    away.y = 0.0F;
    away.z = z - player->pos.z;
    distance = njScalor(&away);
    reach = (record->upgrades & PLAYER_BIG_POWER_ROD) ? 350.0F : 250.0F;
    if (distance > reach || distance < 18.0F) {
        return TRUE;
    }
    ang = ((0x4000 - (Angle)(atan2f(away.x, away.z) * ANGLE_PER_RADIAN)) &
           0xffff) -
          TARGET(task)->heading;
    /* @bug logic: the heading is read sign-extended (MOV.W @(8,R1),R0 at
     * 0x8c0f126a), and the bearing runs 0 to 0xffff, so ang runs from
     * -0x7fff to 0x17fff. neither window (CMP/GT at 0x8c0f127c, CMP/GE at
     * 0x8c0f1288) refuses ang past 0x12b00. so for a heading from 0x8000
     * to 0xd4fe, some bearings more than 0x2b00 off it pass, up to half a
     * turn off for 0x8000. a heading of 0xc000 and a bearing of 0xf000 give
     * ang 0x13000, which passes */
    if (ang > 0x2b00 && ang < 0xd500) {
        return TRUE;
    }
    if (ang < -0x2b00 && ang > -0xd500) {
        return TRUE;
    }
    ang = info->below.ang_x;
    if (ang > 0x4000 || ang < -0x4000 || info->below.ang_z > 0x4000 ||
        info->below.ang_z < -0x4000) {
        return TRUE;
    }
    height = y - player->pos.y;
    if (height <= -50.0F || height >= 50.0F) {
        return TRUE;
    }
    return FALSE;
}

/* the target's step. as Big aims, it spins, the camera looks at it, the
 * stick moves it and player 0 turns to face it. it ends once
 * big_not_fishing gives TRUE */
static void fishing_target_step(struct task *task)
{
    struct task_work *work = task->work;
    struct target_extra *extra = task->extra;
    struct task_work *player = player_work[0];
    NJS_OBJECT *lure = fishing_lures[fishing_count_lures()]->child;

    switch (work->mode) {
    case TARGET_INIT:
        work->mode = TARGET_AIM;
        task->destructor = fishing_target_destroy;
        task->draw = fishing_target_draw;
        extra->ground = work->pos.y;
        extra->slope_z = 0;
        extra->slope_x = 0;
        lure->ang[0] = 0x1000;
        lure->evalflags &= ~NJD_EVAL_UNIT_ANG;
        break;
    case TARGET_AIM:
        fishing_water = extra->ground;
        work->ang.y += 0x200;
        fishing_target_camera(task);
        fishing_target_move(task);
        fishing_target_draw(task);
        player_motion[0]->aim.y = player_work[0]->ang.y =
            0x4000 - (Angle)(atan2f(work->pos.x - player->pos.x,
                                    work->pos.z - player->pos.z) *
                             ANGLE_PER_RADIAN);
        if (big_not_fishing(task)) {
            task_kill(task);
        }
        break;
    }
}

struct task *fishing_create_lure(struct task *big)
{
    struct task *task;
    struct fishing_line *line;
    NJS_POINT3 *goal;

    task =
        task_create(TASK_MOTION | TASK_WORK | TASK_EXTRA, 2, fishing_lure_step);
    /* @bug memory: each lure leaks its line. fishing_lure_destroy frees goal
     * (JSR to heap_free at 0x8c0f157a), but nothing frees the line heap_calloc
     * gives here (JSR at 0x8c0f1478) */
    line = heap_calloc(1, sizeof(struct fishing_line));
    /* @bug memory: the line goes into task's extra before the test for a
     * null task (MOV.L @(44,R14),R3 at 0x8c0f147c, TST R14,R14 at
     * 0x8c0f148a) */
    LURE(task)->line = line;
    goal = heap_calloc(5, sizeof(Float));
    /* @bug memory: a lure whose goal fails to allocate comes back all the
     * same, with a null goal its cast stores through (FMOV.S FR3,@R1 at
     * 0x8c0f3de6) */
    if (task != NULL && goal != NULL) {
        task->work->pos = big->work->pos;
        task->work->ang = big->work->ang;
        LURE(task)->goal = goal;
        if (big_lure != NULL) {
            fishing_flags = 0;
        }
        fishing_reel_ang = 0;
    }
    return task;
}

int fishing_count_lures(void)
{
    int count = 0;

    if (event_flag_check(FLAG_BIG_LURE_EGG_CARRIER)) {
        count++;
    }
    if (event_flag_check(FLAG_BIG_LURE_ICECAP)) {
        count++;
    }
    if (event_flag_check(FLAG_BIG_LURE_STATION_SQUARE)) {
        count++;
    }
    if (event_flag_check(FLAG_BIG_LURE_MYSTIC_RUINS)) {
        count++;
    }
    big_lure_count = count;
    return count;
}

static void fishing_lure_destroy(struct task *task)
{
    struct lure_extra *extra = task->extra;
    NJS_POINT3 *goal;

    sound_stop(SE_REEL);
    goal = extra->goal;
    if (goal != NULL) {
        heap_free(goal);
    }
    big_lure = NULL;
}

/* draw the hit banner's halves, each pos.x from the screen's middle */
static void fishing_banner_draw(struct task *task)
{
    NJS_SPRITE sprite;

    sprite.tlist = &fishing_texlist;
    sprite.tanim = fishing_hud_texanims;
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    sprite.p.x = SCREEN_MIDDLE_X - task->work->pos.x;
    sprite.p.y = 200.0F;
    njDrawSprite2D(&sprite, SPRITE_HIT_LEFT, HUD_DEPTH - 0.016F,
                   NJD_SPRITE_ALPHA);
    sprite.p.x = SCREEN_MIDDLE_X + 8.0F + task->work->pos.x;
    sprite.p.y = 208.0F;
    njDrawSprite2D(&sprite, SPRITE_HIT_RIGHT, HUD_DEPTH - 0.017F,
                   NJD_SPRITE_ALPHA);
}

/* count a step, and draw the banner on the even counts */
static void fishing_banner_blink(struct task *task)
{
    if (!(++BANNER_VARS(task->work)->blinks & 1)) {
        fishing_banner_draw(task);
    }
}

/* the hud task's step. on each frame's first step, it draws the score's
 * label, the weight's unit, the head beside the lives and the numbers.
 * rings_shown and lives_shown choose among them. it draws nothing on the
 * cart, in a field, or while hud_hidden is below 0 */
static void fishing_hud_show(struct task *task)
{
    NJS_SPRITE sprite;

    sprite.tlist = &fishing_texlist;
    sprite.tanim = fishing_hud_texanims;
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    if (cart_active()) {
        return;
    }
    if (frame_step != 0) {
        return;
    }
    if (stage_group() == STAGE_GROUP_FIELD) {
        return;
    }
    if (hud_hidden < 0) {
        return;
    }
#ifdef GCC
    /* 16:9 squeezes the score, the lives and the rings about the screen's
     * left edge */
    port_wide_anchor = WIDE_LEFT;
#endif
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njTextureFilterMode(NJD_TEXTUREFILTER_POINT_SAMPLE);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    if (rings_shown <= 0) {
        sprite.p.x = 48.0F;
        sprite.p.y = 64.0F;
        njDrawSprite2D(&sprite, SPRITE_SCORE_LABEL, HUD_DEPTH - 0.02F,
                       NJD_SPRITE_ALPHA);
        sprite.p.x = 184.0F;
        sprite.p.y = 47.0F;
        njDrawSprite2D(&sprite, SPRITE_UNIT, HUD_DEPTH - 0.02F,
                       NJD_SPRITE_ALPHA);
    }
    if (lives_shown >= 0) {
        sprite.tlist = &fishing_regular_texlist;
        sprite.tanim = fishing_head_texanims;
        sprite.p.x = 16.0F;
        sprite.p.y = 416.0F;
        njDrawSprite2D(&sprite, 0, -2.0F, NJD_SPRITE_ALPHA);
    }
    fishing_draw_score(fishing_score);
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
#ifdef GCC
    port_wide_anchor = WIDE_MIDDLE;
#endif
    if (stage_group() == STAGE_GROUP_FIELD) {
        rings_shown = -1;
        lives_shown = -1;
    } else {
        rings_shown = lives_shown = hud_hidden;
    }
}

/* the hud task's draw: as fishing_hud_show, but in a field too, where
 * SPRITE_FIELD_LABEL takes the place of the score's label and unit */
static void fishing_hud_draw(struct task *task)
{
    NJS_SPRITE sprite;

    sprite.tlist = &fishing_texlist;
    sprite.tanim = fishing_hud_texanims;
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    if (cart_active()) {
        return;
    }
#ifdef GCC
    /* the split frame's second picture replays the hud step 0 drew. this
     * draw is the pause's: in a field it shows the rings, which play hides,
     * and it moves the pulse of a ring count of 0, which step 0 reads */
    if (frame_redrawing) {
        return;
    }
#endif
    if (frame_step != 0) {
        return;
    }
    if (hud_hidden < 0) {
        return;
    }
#ifdef GCC
    port_wide_anchor = WIDE_LEFT;
#endif
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njTextureFilterMode(NJD_TEXTUREFILTER_POINT_SAMPLE);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    if (rings_shown <= 0) {
        if (stage_group() == STAGE_GROUP_FIELD) {
            sprite.p.x = 48.0F;
            sprite.p.y = 54.0F;
            njDrawSprite2D(&sprite, SPRITE_FIELD_LABEL, HUD_DEPTH - 0.02F,
                           NJD_SPRITE_ALPHA);
        } else {
            sprite.p.x = 48.0F;
            sprite.p.y = 64.0F;
            njDrawSprite2D(&sprite, SPRITE_SCORE_LABEL, HUD_DEPTH - 0.02F,
                           NJD_SPRITE_ALPHA);
            sprite.p.x = 184.0F;
            sprite.p.y = 47.0F;
            njDrawSprite2D(&sprite, SPRITE_UNIT, HUD_DEPTH - 0.02F,
                           NJD_SPRITE_ALPHA);
        }
    }
    if (lives_shown >= 0) {
        sprite.tlist = &fishing_regular_texlist;
        sprite.tanim = fishing_head_texanims;
        sprite.p.x = 16.0F;
        sprite.p.y = 416.0F;
        njDrawSprite2D(&sprite, 0, -2.0F, NJD_SPRITE_ALPHA);
    }
    fishing_draw_score(fishing_score);
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
#ifdef GCC
    port_wide_anchor = WIDE_MIDDLE;
#endif
    if (stage_group() == STAGE_GROUP_FIELD) {
        rings_shown = -1;
        lives_shown = -1;
    } else {
        rings_shown = lives_shown = hud_hidden;
    }
}

/* draw the reel, turned by fishing_reel_ang while the lure is in the water
 * and upright otherwise. nothing while big_lure is null */
static void fishing_draw_reel(void)
{
    NJS_SPRITE sprite;

    if (big_lure != NULL) {
        sprite.tlist = &fishing_texlist;
        sprite.tanim = fishing_hud_texanims;
        if (big_lure->work->mode < LURE_LANDED ||
            big_lure->work->mode > LURE_FISH) {
            sprite.ang = 0;
        } else {
            sprite.ang = fishing_reel_ang;
        }
        sprite.sx = 1.0F;
        sprite.sy = 1.0F;
        sprite.p.x = 592.0F;
        sprite.p.y = 394.0F;
        njDrawSprite2D(&sprite, SPRITE_REEL, HUD_DEPTH - 0.018F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
    }
}

/* draw the tension gauge, and its bar as a polygon, 135 tall at a tension
 * of 1. as the tension rises, the bar's top shades from blue to red, and
 * its base from cyan to yellow */
static void fishing_draw_tension(void)
{
    NJS_SPRITE sprite;
    NJS_POINT2COL bar;
    NJS_POINT2COL *poly;
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];
    int red;

    sprite.tlist = &fishing_texlist;
    sprite.tanim = fishing_hud_texanims;
    sprite.ang = 0;
    sprite.sx = 1.0F;
    sprite.sy = 1.0F;
    sprite.p.x = 576.0F;
    sprite.p.y = 328.0F;
    njDrawSprite2D(&sprite, SPRITE_GAUGE, HUD_DEPTH - 0.019F, NJD_SPRITE_ALPHA);
    /* a scale and SPRITE_BAR's top for the tension, which nothing draws:
     * the polygon below is the bar */
    sprite.sx = 1.0F;
    if (fishing_tension < 0.0F) {
        sprite.sy = 0.0F;
    } else if (fishing_tension >= 1.0F) {
        sprite.sy = 4.3F;
    } else {
        sprite.sy = fishing_tension * 4.3F;
        fishing_hud_texanims[SPRITE_BAR].v1 = (1.0F - fishing_tension) * 256.0F;
    }
    poly = &bar;
    poly->p = points;
    poly->col = colors;
    poly->num = 4;
    poly->p[0].x = poly->p[3].x = 574.0F;
    poly->p[0].y = poly->p[1].y = 368.0F;
    poly->p[1].x = poly->p[2].x = 606.0F;
    poly->p[2].y = poly->p[3].y = 368.0F - (fishing_tension * 135.0F);
    red = (int)(fishing_tension * 255.0F) & 0xff;
    poly->col[0].color = poly->col[1].color = 0xff00ffff + (red << 16) - red;
    poly->col[2].color = poly->col[3].color = 0xff0000ff + (red << 16) - red;
    poly->tex = NULL;
    njDrawPolygon2D(poly, 4, HUD_DEPTH - 0.02F, NJD_FILL);
}

/* draw distance, in tens, to 99 */
static void fishing_draw_line_out(Float distance)
{
    struct hud_number number;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    number.flags = NUMBER_CLAMPED | NUMBER_ZEROS;
    number.place = NUMBER_LEFT | NUMBER_TOP | NUMBER_DEPTH;
    number.unk_02 = 0;
    number.max = 99;
    number.value = (int)(distance * 0.1F);
    number.pos.x = 555.0F;
    number.pos.y = 418.0F;
    number.pos.z = 0.0F;
    number.scale = 1.0F;
    number.color = 0xffc0c0c0;
    hud_draw_number(&number);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* draw score, except in a field, then the lives and the rings. the ring count
 * pulses toward red while it is 0 */
static void fishing_draw_score(int score)
{
    struct hud_number number;
    /* @hack spelling: hud_draw_number called through draw, which keeps its
     * address in R14 (MOV.L at 0x8c0f1c1a, JSR @R14 at 0x8c0f1c50); called
     * by name, 16 of 131 words differ */
    /* @hack redundant: register, which puts the wave in FR4 (FMOV FR0,FR4
     * at 0x8c0f1d1e); without it, the wave takes FR5 and 6 of 131 words
     * differ */
    register void (*draw)(struct hud_number *number);
    int lives;
    int rings;
    Float wave;
    Float glow;
    int green;

    number.flags = NUMBER_CLAMPED;
    number.place = NUMBER_LEFT | NUMBER_TOP | NUMBER_DEPTH;
    number.unk_02 = 0;
    number.max = 99999;
    number.value = score;
    number.pos.x = 96.0F;
    number.pos.y = 32.0F;
    number.pos.z = 0.0F;
    number.scale = 1.0F;
    number.color = 0xffc0c0c0;
    draw = hud_draw_number;
    if (stage_group() != STAGE_GROUP_FIELD && rings_shown <= 0) {
        draw(&number);
    }
    lives = lives_count();
    number.flags = NUMBER_CLAMPED | NUMBER_ZEROS;
    number.max = 99;
    number.value = lives;
    number.pos.x = 49.0F;
    number.pos.y = 432.0F;
    if (lives_shown >= 0) {
        draw(&number);
    }
    rings = (Sint16)ring_count();
    number.flags = NUMBER_CLAMPED | NUMBER_ZEROS;
    number.max = 999;
    number.value = rings;
    number.pos.x = 46.0F;
    number.pos.y = (stage_group() == STAGE_GROUP_FIELD) ? 32.0F : 57.0F;
    if (rings == 0) {
        fishing_ring_blink += 0x400;
        wave = njSin(fishing_ring_blink);
        glow = 1.0F - (wave * wave);
    } else {
        glow = 1.0F;
    }
    number.color = 0xffc00000;
    green = (int)(glow * 192.0F);
    number.color += (green << 8) + green;
    if (rings_shown <= 0) {
        draw(&number);
    }
}

/* set fishing_distance to the distance from from to to, keep
 * fishing_line_out between it and 5 past it, and draw the line out */
static void fishing_measure_line(NJS_POINT3 *from, NJS_POINT3 *to)
{
    Float distance = njDistanceP2P(from, to);

    fishing_distance = distance;
    if (distance > fishing_line_out) {
        fishing_line_out = distance;
    }
    if (fishing_distance + 5.0F <= fishing_line_out) {
        fishing_line_out = fishing_distance + 5.0F;
    }
    fishing_draw_line_out(fishing_line_out);
}

/* the lure task's draw as it steps: the lure, unless the line snapped.
 * from LURE_FLY on, it draws the line and the hud's reel, gauge and line
 * out too, on each frame's first step. nothing while big_lure is null */
static void fishing_lure_show(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    NJS_POINT3 tip;
    NJS_POINT3 end;

    if (big_lure == NULL) {
        return;
    }
    fishing_line_end(&end);
    fishing_rod_tip(task, &tip);
    njSetTexture(&fishing_texlist);
    fishing_set_draw();
    if (!(fishing_flags & FISHING_SNAPPED)) {
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN(njRotateY, -work->ang.y);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateZ, work->ang.z);
        if (work->mode < LURE_FLY) {
            njTranslate(NULL, 0.6F, -0.3F, 0.0F);
        }
        njScale(NULL, 0.3F, 0.3F, 0.3F);
        draw_tree(fishing_lures[fishing_count_lures()]);
        njPopMatrix(1);
    }
    if (frame_step == 0) {
        if (work->mode >= LURE_FLY) {
            fishing_line_draw(LURE(task)->line);
        }
        /* @hack spelling: the mode read again through task->work, which the
         * image reloads from task (MOV.L @(32,R13),R2 at 0x8c0f1ec8);
         * through work, 17 of 142 words differ */
        if (task->work->mode >= LURE_FLY) {
            njSetTexture(&fishing_texlist);
#ifdef GCC
            /* 16:9 squeezes the reel, the gauge and the line-out count about
             * the screen's right edge */
            port_wide_anchor = WIDE_RIGHT;
#endif
            fishing_draw_reel();
            fishing_draw_tension();
            fishing_measure_line(&player->pos, &end);
#ifdef GCC
            port_wide_anchor = WIDE_MIDDLE;
#endif
        }
    }
}

/* the lure task's draw: as fishing_lure_show, but it draws while big_lure is
 * null too. it draws the line and the hud on every step of a frame */
static void fishing_lure_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    NJS_POINT3 tip;
    NJS_POINT3 end;

    fishing_line_end(&end);
    fishing_rod_tip(task, &tip);
    njSetTexture(&fishing_texlist);
    fishing_set_draw();
    if (!(fishing_flags & FISHING_SNAPPED)) {
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        TURN(njRotateY, -work->ang.y);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateZ, work->ang.z);
        if (work->mode < LURE_FLY) {
            njTranslate(NULL, 0.6F, -0.3F, 0.0F);
        }
        njScale(NULL, 0.3F, 0.3F, 0.3F);
        draw_tree(fishing_lures[fishing_count_lures()]);
        njPopMatrix(1);
    }
    if (work->mode >= LURE_FLY) {
        fishing_line_draw(LURE(task)->line);
    }
    /* @hack spelling: the mode read again through task->work, which the image
     * reloads from task (MOV.L @(32,R13),R2 at 0x8c0f2046); through work,
     * 24 of 130 words differ */
    if (task->work->mode >= LURE_FLY) {
        njSetTexture(&fishing_texlist);
#ifdef GCC
        port_wide_anchor = WIDE_RIGHT;
#endif
        fishing_draw_reel();
        fishing_draw_tension();
        fishing_measure_line(&player->pos, &end);
#ifdef GCC
        port_wide_anchor = WIDE_MIDDLE;
#endif
    }
}

/* set pos to the rod's tip, from player 0's weld_point4 in his model, turned
 * as he is about his center. the tip lies 16 back along weld_axis4 and 0.6
 * down. with the power rod, it lies 17 back, 1.8 down and 0.6 along z. task
 * goes unused */
static void fishing_rod_tip(struct task *task, NJS_POINT3 *pos)
{
    struct player *record = player_motion[0]->record;
    struct task_work *player = player_work[0];
    NJS_VECTOR point;
    NJS_VECTOR offset;
    Float back;

    njPushMatrix(&_unit_matrix_);
    point.x = point.z = 0.0F;
    point.y = record->consts.center_y;
    TURN(njRotateZ, player->ang.z);
    TURN(njRotateX, player->ang.x);
    TURN(njRotateY, 0x8000 - player->ang.y);
    njCalcVector(NULL, &point, pos);
    back = (record->upgrades & PLAYER_BIG_POWER_ROD) ? 17.0F : 16.0F;
    point.x = record->weld_point4.x - (back * record->weld_axis4.x);
    point.y = record->weld_point4.y - (back * record->weld_axis4.y);
    point.z = record->weld_point4.z - (back * record->weld_axis4.z);
    if (record->upgrades & PLAYER_BIG_POWER_ROD) {
        point.y -= 0.6F * 3.0F;
        point.z += 0.6F;
    } else {
        point.y -= 0.6F;
    }
    njCalcVector(NULL, &point, &offset);
    pos->x += player->pos.x + offset.x;
    pos->y += player->pos.y + offset.y;
    pos->z += player->pos.z + offset.z;
    njPopMatrix(1);
}

/* fly the lure along its speed, whose y falls 0.02 a step. TRUE, with the lure
 * where it was, when its sweep meets the world. otherwise it moves, and TRUE
 * when its collision hit something */
static Bool fishing_lure_fly(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    NJS_POINT3 pos;
    struct angles slope;

    slope.x = slope.y = slope.z = 0;
    motion->speed.y -= 0.02F;
    pos = work->pos;
    if (world_sweep_all_hit(&pos, &motion->speed, &slope, 0.5F)) {
        return TRUE;
    }
    VECTOR_ADD(&work->pos, &motion->speed);
    if (work->collision->flags & COLLISION_HIT) {
        return TRUE;
    }
    return FALSE;
}

/* move the lure toward the rod's tip, 10 a step as LURE_BACK, else 2.
 * TRUE once it lies within 5 of the tip on each axis */
static Bool fishing_lure_return(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    NJS_POINT3 tip;
    Float speed;

    fishing_rod_tip(task, &tip);
    motion->speed.x = tip.x - work->pos.x;
    motion->speed.y = tip.y - work->pos.y;
    motion->speed.z = tip.z - work->pos.z;
    njUnitVector(&motion->speed);
    speed = (work->mode == LURE_BACK) ? 10.0F : 2.0F;
    work->pos.x += motion->speed.x * speed;
    work->pos.y += motion->speed.y * speed;
    work->pos.z += motion->speed.z * speed;
    if (work->pos.x + 5.0F > tip.x && tip.x > work->pos.x - 5.0F &&
        work->pos.y + 5.0F > tip.y && tip.y > work->pos.y - 5.0F &&
        work->pos.z + 5.0F > tip.z && tip.z > work->pos.z - 5.0F) {
        return TRUE;
    }
    return FALSE;
}

/* TRUE when pos lies within 20 of tip in x and z, and at Ice Cap's pond,
 * less than 10 under the water */
static Bool fishing_lure_at_rod(NJS_POINT3 *tip, NJS_POINT3 *pos)
{
    NJS_VECTOR away;

    away.x = tip->x - pos->x;
    away.z = tip->z - pos->z;
    away.y = 0.0F;
    if (stage_get_stage_act() == ICECAP_POND) {
        if (njScalor(&away) < 20.0F && pos->y > fishing_water - 10.0F) {
            return TRUE;
        }
    } else {
        if (njScalor(&away) < 20.0F) {
            return TRUE;
        }
    }
    return FALSE;
}

/* slow the lure in x and z, by 0.05 a step to a stop. with a fish on, it
 * sinks 0.05 a step, and at random one step in five rises 0.05. with none,
 * it sinks 0.005 a step faster, to 0.05 a step, or 0.1 at Ice Cap's pond.
 * on the bottom, it neither sinks nor rises. the reel's sound stops */
static void fishing_lure_drift(struct task *task)
{
    struct task_motion *motion = task->motion;

    if (motion->speed.x > 0.0F) {
        motion->speed.x -= 0.05F;
    }
    if (motion->speed.x < 0.0F) {
        motion->speed.x += 0.05F;
    }
    if (motion->speed.x < 0.1F && motion->speed.x > -0.1F) {
        motion->speed.x = 0.0F;
    }
    if (motion->speed.z > 0.0F) {
        motion->speed.z -= 0.05F;
    }
    if (motion->speed.z < 0.0F) {
        motion->speed.z += 0.05F;
    }
    if (motion->speed.z < 0.1F && motion->speed.z > -0.1F) {
        motion->speed.z = 0.0F;
    }
    if (fishing_flags & FISHING_HOOKED) {
        if (fishing_flags & FISHING_BOTTOM) {
            motion->speed.y = 0.1F;
        } else if (rand() * RAND_SCALE < 0.8F) {
            motion->speed.y = -0.05F;
        } else {
            motion->speed.y = 0.05F;
        }
    } else {
        motion->speed.y -= 0.005F;
        if (stage_get_stage_act() == ICECAP_POND) {
            if (motion->speed.y < -0.1F) {
                motion->speed.y = -0.1F;
            }
        } else {
            if (motion->speed.y < -0.05F) {
                motion->speed.y = -0.05F;
            }
        }
    }
    if (fishing_flags & FISHING_BOTTOM) {
        motion->speed.y = 0.0F;
    }
    sound_stop(SE_REEL);
}

/* reel in 0.1 of line a step, turning the reel 10 degrees, with SE_REEL
 * each half turn. while the line out reaches the lure, the line is slack,
 * and the lure drifts. once the line is taut, the lure comes toward the
 * rod's tip 0.08 a step in x and z. it rises 0.02 a step, or 0.06 at Ice
 * Cap's pond. a hooked fish of weight w scales the 0.08 by 5000 / (w +
 * 5000). where fishing_lure_at_rod holds, the lure leaves the water. on a
 * taut line, a press of an action button twitches the lure */
static void fishing_reel_slow(struct task *task, NJS_POINT3 *tip)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    Float pace;
    Angle was;

    if (big_lure == NULL) {
        return;
    }
    if (big_catch != NULL && (fishing_flags & FISHING_HOOKED)) {
        pace = 5000.0F / (FISH_MOTION(big_catch)->weight + 5000.0F);
    } else {
        pace = 1.0F;
    }
    fishing_line_out -= 0.1F;
    was = fishing_reel_ang;
    fishing_reel_ang += NJM_DEG_ANG(10);
    if ((was ^ fishing_reel_ang) & 0x8000) {
        sound_stop(SE_REEL);
        sound_play(SE_REEL, 0, 0, 0);
    }
    if (fishing_distance <= fishing_line_out) {
        fishing_lure_drift(task);
        return;
    }
    motion->speed.x = tip->x - work->pos.x;
    motion->speed.z = tip->z - work->pos.z;
    if (fishing_lure_at_rod(tip, &work->pos)) {
        fishing_lure_leave_water(task);
    }
    njUnitVector(&motion->speed);
    motion->speed.x *= pace * 0.08F;
    motion->speed.z *= pace * 0.08F;
    if (stage_get_stage_act() == ICECAP_POND) {
        motion->speed.y = 0.06F;
    } else {
        motion->speed.y = 0.02F;
    }
    /* @hack spelling: the aim read through task->motion, which the image
     * reloads from task (MOV.L @(36,R13),R3 at 0x8c0f27aa); through motion,
     * task and work trade R12 and R13, and 33 of 153 words differ */
    work->ang.z = task->motion->aim.z + 0x2000;
    if (PAD_BUTTONS(pad_records[0].press, pad_action_buttons)) {
        fishing_flags |= FISHING_TWITCH;
    }
}

/* fishing_reel_slow, faster. it reels in 0.2 of line a step and turns the
 * reel 25 degrees. the lure comes 0.2 a step in x and z, and rises 0.06,
 * or 0.18 at Ice Cap's pond. a press of a jump button twitches it */
static void fishing_reel_fast(struct task *task, NJS_POINT3 *tip)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    Float pace;
    Angle was;

    if (big_lure == NULL) {
        return;
    }
    if (big_catch != NULL && (fishing_flags & FISHING_HOOKED)) {
        pace = 5000.0F / (FISH_MOTION(big_catch)->weight + 5000.0F);
    } else {
        pace = 1.0F;
    }
    fishing_line_out -= 0.2F;
    was = fishing_reel_ang;
    fishing_reel_ang += NJM_DEG_ANG(25);
    if ((was ^ fishing_reel_ang) & 0x8000) {
        sound_stop(SE_REEL);
        sound_play(SE_REEL, 0, 0, 0);
    }
    if (fishing_distance <= fishing_line_out) {
        fishing_lure_drift(task);
        return;
    }
    motion->speed.x = tip->x - work->pos.x;
    motion->speed.z = tip->z - work->pos.z;
    if (fishing_lure_at_rod(tip, &work->pos)) {
        fishing_lure_leave_water(task);
    }
    njUnitVector(&motion->speed);
    motion->speed.x *= pace * 0.2F;
    motion->speed.z *= pace * 0.2F;
    if (stage_get_stage_act() == ICECAP_POND) {
        motion->speed.y = 0.18F;
    } else {
        motion->speed.y = 0.06F;
    }
    /* @hack spelling: the aim read through task->motion, which the image
     * reloads from task (MOV.L @(36,R13),R3 at 0x8c0f294a); through motion,
     * 43 of 153 words differ */
    work->ang.z = task->motion->aim.z + 0x2000;
    if (PAD_BUTTONS(pad_records[0].press, pad_jump_buttons)) {
        fishing_flags |= FISHING_TWITCH;
    }
}

/* move the lure aside as Big turns the reel with the stick to one side.
 * off is the lure's bearing from him less his heading. a turn to the left
 * moves the lure only while off lies outside 30 to 180 degrees. a turn to
 * the right needs off outside 180 to 330. the line must also be within 5
 * of taut. the lure then moves toward the rod's tip and that side, about
 * 0.5 a step in x and z, and rises 0.04 a step. it turns 0x100 a step, to
 * at most 45 degrees off its aim. frame 15 of his turn twitches it.
 * otherwise the lure drifts, rising at most 0.015 a step with no fish on */
static void fishing_reel_turning(struct task *task, NJS_POINT3 *tip)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = player_motion[0]->record;
    struct task_work *player = player_work[0];
    NJS_VECTOR side;
    NJS_MATRIX matrix;
    int off;

    if (fishing_line_out <= fishing_distance + 5.0F) {
        VECTOR_DIFF(&motion->speed, tip, &work->pos);
        njUnitVector(&motion->speed);
        motion->speed.x *= 0.5F;
        motion->speed.y *= 0.5F;
        motion->speed.z *= 0.5F;
        /* @hack spelling: the aim here, and the lure's heading in both turns
         * below, read through task, which the image reloads (MOV.L
         * @(36,R13),R4 at 0x8c0f2a30, MOV.L @(32,R13),R2 at 0x8c0f2a7c and
         * 0x8c0f2ba2); through motion and work, 126 of 305 words differ */
        off = (Uint16)(task->motion->aim.y - player->ang.y);
        if (record->anim.action == ACTION_REEL_DOWN_LEFT ||
            record->anim.action == ACTION_REEL_LEFT ||
            record->anim.action == ACTION_FLOAT_REEL_DOWN_LEFT ||
            record->anim.action == ACTION_FLOAT_REEL_LEFT) {
            if (off < NJM_DEG_ANG(30) || off > NJM_DEG_ANG(180)) {
                side.x = 0.0F;
                side.y = 0.0F;
                side.z = 1.0F;
                njUnitMatrix(&matrix);
                TURN_MATRIX(njRotateY, &matrix, -task->work->ang.y);
                njCalcVector(&matrix, &side, &side);
                njAddVector(&motion->speed, &side);
                njUnitVector(&motion->speed);
            } else {
                motion->speed.x = 0.0F;
                motion->speed.z = 0.0F;
                if (motion->speed.y >= 0.02F) {
                    motion->speed.y = 0.02F;
                }
                fishing_lure_drift(task);
                return;
            }
            if (work->ang.y - motion->aim.y > -NJM_DEG_ANG(45)) {
                work->ang.y -= 0x100;
            }
        } else if (record->anim.action == ACTION_REEL_DOWN_RIGHT ||
                   record->anim.action == ACTION_REEL_RIGHT ||
                   record->anim.action == ACTION_FLOAT_REEL_DOWN_RIGHT ||
                   record->anim.action == ACTION_FLOAT_REEL_RIGHT) {
            if (off > NJM_DEG_ANG(330) || off < NJM_DEG_ANG(180)) {
                side.x = 0.0F;
                side.y = 0.0F;
                side.z = -1.0F;
                njUnitMatrix(&matrix);
                TURN_MATRIX(njRotateY, &matrix, -task->work->ang.y);
                njCalcVector(&matrix, &side, &side);
                njAddVector(&motion->speed, &side);
                njUnitVector(&motion->speed);
            } else {
                motion->speed.x = 0.0F;
                motion->speed.z = 0.0F;
                if (motion->speed.y >= 0.02F) {
                    motion->speed.y = 0.02F;
                }
                fishing_lure_drift(task);
                return;
            }
            if (work->ang.y - motion->aim.y < NJM_DEG_ANG(45)) {
                work->ang.y += 0x100;
            }
        } else {
            motion->speed.x = 0.0F;
            motion->speed.z = 0.0F;
            if (motion->speed.y >= 0.02F) {
                motion->speed.y = 0.02F;
            }
            fishing_lure_drift(task);
            return;
        }
    } else {
        if (motion->speed.y >= 0.02F) {
            motion->speed.y = 0.02F;
        }
        fishing_lure_drift(task);
        return;
    }
    motion->speed.x *= 0.5F;
    motion->speed.y = 0.04F;
    motion->speed.z *= 0.5F;
    /* @hack spelling: the aim read through task->motion, which the image
     * reloads from task (MOV.L @(36,R13),R3 at 0x8c0f2c78); through motion,
     * 24 of 305 words differ */
    work->ang.z = task->motion->aim.z + 0x1000;
    if (record->anim.frame == 15.0F) {
        fishing_flags |= FISHING_TWITCH;
    }
}

/* what a fish's pull of 0 or more adds to the tension */
/* @hack redundant: EASY_PULL is 0, which the image adds all the same (FLDI0
 * FR12 at 0x8c0f2d0e, FMAC at 0x8c0f2e0e, FADD at 0x8c0f2f1e); with neither
 * addition, 195 of fishing_tension_step's 415 words differ */
#define EASY_PULL 0.0F

/* set fishing_tension_aim from the hooked fish's weight and pull, the
 * buttons held and fishing_strain, and ease fishing_tension toward it. the
 * fish's pull is its speed times the cosine of its heading less the lure's
 * aim, and a negative pull raises the aim. the tension eases on a frame's
 * first step, by 0.02, or by 0.002 once within 0.02 of the aim. at a
 * tension of 1, the line snaps, the music changes, and the hooked fish's
 * weight shows as lost. from 0.8, SE_STRAIN plays every 120 steps. move is
 * the lure's move this step, to which a kick adds the fish's speed */
static void fishing_tension_step(struct task *task, NJS_VECTOR *move)
{
    struct task_motion *motion = task->motion;
    Float weight;
    Float pull;
    Float speed;
    Float gap;

    if (big_lure != NULL && big_catch != NULL) {
        /* @hack spelling: the weight tested bare, which loads the 0 into
         * FR2 and the weight into FR3 (0x8c0f2d2a, 0x8c0f2d2c); tested as
         * != 0.0F, the two trade and 3 of 415 words differ */
        if (big_catch->motion != NULL && FISH_MOTION(big_catch)->weight) {
            weight = FISH_MOTION(big_catch)->weight;
        } else {
            weight = 1000.0F;
        }
        /* @hack spelling: the product spelled out, which loads 0.001 into
         * FR5 and fishing_strain into FR4 (0x8c0f2d44, 0x8c0f2d52); with *=,
         * the two trade and 19 of 415 words differ */
        weight = weight * 0.0001F;
        if (fishing_flags & FISHING_HOOKED) {
            if (PAD_BUTTONS(pad_action_buttons | pad_jump_buttons,
                            pad_records[0].on)) {
                fishing_strain += 0.0005F;
            } else if (fishing_strain > 0.0F) {
                fishing_strain -= 0.001F;
            }
            if (fishing_flags & FISHING_PULL) {
                if (fishing_strain > 0.0F) {
                    fishing_strain -= 0.001F;
                }
            }
            /* each arm calls njScalor on move and drops its result (JSR at
             * 0x8c0f2daa and 0x8c0f2ee8) */
            if (fishing_flags & FISHING_KICK) {
                njScalor(move);
                speed = njScalor(&FISH_MOTION(big_catch)->speed);
                pull = speed * njCos(big_catch->work->ang.y - motion->aim.y);
                if (pull < 0.0F) {
                    fishing_tension_aim = (weight * 1.1F) - (pull * 2.1F);
                    fishing_tension_aim +=
                        (0.24F - weight) * rand() * (1.2F * RAND_SCALE);
                } else {
                    fishing_tension_aim = EASY_PULL + (weight * 2.0F);
                }
                if (fishing_tension_aim < 0.5F) {
                    if (pad_records[0].on & pad_jump_buttons) {
                        fishing_tension_aim += 0.5F;
                    } else if (pad_records[0].on & pad_action_buttons) {
                        fishing_tension_aim += 0.3F;
                    }
                }
                fishing_tension_aim += fishing_strain;
                if (fishing_fight_steps > 1800 && fishing_tension_aim > 0.8F) {
                    fishing_tension_aim -= 0.1F;
                }
                njAddVector(move, &FISH_MOTION(big_catch)->speed);
            } else {
                njScalor(move);
                speed = njScalor(&FISH_MOTION(big_catch)->speed);
                pull = speed * njCos(big_catch->work->ang.y - motion->aim.y);
                if (pull < 0.0F) {
                    weight -= pull * 2.0F;
                } else {
                    weight += EASY_PULL;
                }
                fishing_tension_aim = weight;
                if (fishing_tension_aim < 0.5F) {
                    if (pad_records[0].on & pad_jump_buttons) {
                        fishing_tension_aim += 0.5F;
                    } else if (pad_records[0].on & pad_action_buttons) {
                        fishing_tension_aim += 0.3F;
                    }
                }
                fishing_tension_aim += fishing_strain;
            }
        } else {
            if (fishing_strain > 0.0F) {
                fishing_strain -= 0.001F;
            }
            if (fishing_flags & FISHING_PULL) {
                fishing_tension_aim = 0.1F;
            } else {
                fishing_tension_aim = 0.2F;
            }
        }
    } else {
        fishing_tension_aim = 0.1F;
    }
    fishing_tension_aim += rand() * (0.1F * RAND_SCALE);
    if (frame_step == 0) {
        if (fishing_tension_aim != fishing_tension) {
            gap = fishing_tension_aim - fishing_tension;
            if (gap > 0.02F) {
                fishing_tension += 0.02F;
            } else if (gap < -0.02F) {
                fishing_tension -= 0.02F;
            } else if (gap > 0.002F) {
                fishing_tension += 0.002F;
            } else if (gap < -0.002F) {
                fishing_tension -= 0.002F;
            } else {
                fishing_tension = fishing_tension_aim;
            }
        }
        if (fishing_tension < 0.0F) {
            fishing_tension = 0.0F;
        } else if (fishing_tension >= 1.0F) {
            fishing_tension = 1.0F;
            if (big_lure != NULL) {
                fishing_flags |= FISHING_SNAPPED;
                stream_request_interlude(MUSIC_FISHMISS);
                if (big_catch != NULL) {
                    fishing_show_catch(FISH_MOTION(big_catch)->weight,
                                       CATCH_SNAPPED);
                }
            }
        }
    }
    if (fishing_warn_steps) {
        fishing_warn_steps--;
    }
    if (fishing_tension >= 0.8F) {
        if (!fishing_warn_steps) {
            sound_play(SE_STRAIN, 0, 0, 0);
            fishing_warn_steps = 120;
        }
    } else {
        sound_stop(SE_STRAIN);
    }
}

/* TRUE while the fish pulls the lure: whenever it kicks, and while it's
 * hooked with no reel button held, unless Big pulls the rod aside. FALSE
 * while big_lure is null */
static Bool fishing_fish_pulls(struct task *task)
{
    if (big_lure == NULL) {
        return FALSE;
    }
    if (fishing_flags & FISHING_KICK) {
        return TRUE;
    }
    if (fishing_flags & FISHING_PULL) {
        return FALSE;
    }
    if ((fishing_flags & FISHING_HOOKED) &&
        PAD_BUTTONS(pad_records[0].off, pad_action_buttons) ==
            (Sint32)pad_action_buttons &&
        (pad_records[0].off & pad_jump_buttons) == pad_jump_buttons) {
        return TRUE;
    }
    return FALSE;
}

/* TRUE at Ice Cap's pond while fishing_cast_pos lies within 70 of one of
 * the first PENS of fishing_pens */
static Bool fishing_in_pen(void)
{
    int i;

    if (stage_get_stage_act() != ICECAP_POND) {
        return FALSE;
    }
    for (i = 0; i < PENS; i++) {
        if (njDistanceP2P(&fishing_pens[i], &fishing_cast_pos) < 70.0F) {
            break;
        }
    }
    if (i == PENS) {
        return FALSE;
    }
    return TRUE;
}

/* move the lure in the water for a step: by the pulling fish's speed, or
 * else by the reels or the rod's pull. it stays between the bottom and the
 * water's top, and the line's tension follows. a pulling fish gets away 300
 * from Big, or 400 with the power rod. the lure leaves the water under a
 * surface within 2 of the water's top, outside a pen. with no fish pulling,
 * it also leaves at the top against an upright surface. it stays at Emerald
 * Coast, or while Big pulls the rod aside */
static void fishing_lure_swim(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    struct player *record = player_motion[0]->record;
    NJS_POINT3 tip;
    NJS_POINT3 pos;
    NJS_VECTOR move;
    struct near_surfaces info;
    struct angles wall_slope;
    struct angles floor_slope;
    Float radius;
    Float depth;
    Float length;
    Bool wall;
    Bool floor;
    int buttons;

    if (fishing_flags & FISHING_HOOKED) {
        radius = 5.5F;
    } else {
        radius = 0.5F;
    }
    fishing_find_surfaces_at(work, SKIP_WATER_BELOW | SKIP_WATER_ABOVE, &info);
    if (info.water.height == GROUND_FAR) {
        info.water.height = fishing_water;
    } else {
        fishing_water = info.water.height;
    }
    /* @hack spelling: depth set inside the test, which keeps it in the frame
     * (FMOV.S FR2,@(R0,R15) at 0x8c0f3314); set ahead of the test, the
     * frame shrinks by 4 bytes and 244 of 631 words differ */
    if ((depth = info.above.height - info.water.height) < 2.0F &&
        depth > -2.0F && !fishing_in_pen()) {
        work->pos.y = info.above.height;
        fishing_lure_leave_water(task);
        return;
    }
    if (fishing_fish_pulls(task)) {
        if (big_catch == NULL) {
            return;
        }
        move = FISH_MOTION(big_catch)->speed;
        wall_slope.x = wall_slope.y = wall_slope.z = 0;
        pos = work->pos;
        wall = world_sweep_all_hit(&pos, &move, &wall_slope, radius);
        floor_slope.x = floor_slope.y = floor_slope.z = 0;
        pos = work->pos;
        floor = fishing_sweep(&pos, &move, &floor_slope, radius);
        /* @hack spelling: the four tests joined by &, which calls
         * fishing_upright for both slopes whatever the sweeps found (JSR at
         * 0x8c0f33d4 and 0x8c0f33e0). each test gives TRUE or FALSE, and
         * fishing_upright changes nothing but its own locals, so & gives what
         * && does. with &&, fishing.c's code runs 32 bytes longer and 522 of
         * 631 words differ */
        if (wall & floor & fishing_upright(NULL, &floor_slope) &
            fishing_upright(NULL, &wall_slope)) {
            move.x = move.z = 0.0F;
            move.y *= 3.0F;
        }
        if (work->pos.y + move.y > info.water.height - radius - 0.1F &&
            !fishing_upright(task, &wall_slope)) {
            move.y = 0.0F;
            work->pos.y = info.water.height - radius - 0.1F;
        }
        if (work->pos.y + move.y <= info.below.height + radius) {
            length = njScalor(&move);
            if (work->pos.y != info.water.height - radius - 0.1F) {
                work->pos.y = info.below.height + radius;
                if (length <= 0.03F) {
                    move.x = move.y = move.z = 0.0F;
                }
            } else {
                move.x = move.y = move.z = 0.0F;
            }
        }
        motion->speed = move;
        work->pos.x += move.x;
        work->pos.y += move.y;
        work->pos.z += move.z;
        fishing_tension_step(task, &move);
        if (record->upgrades & PLAYER_BIG_POWER_ROD) {
            if (fishing_distance > 400.0F) {
                fishing_flags |= FISHING_LOST;
            }
        } else {
            if (fishing_distance > 300.0F) {
                fishing_flags |= FISHING_LOST;
            }
        }
    } else {
        fishing_rod_tip(task, &tip);
        if (big_lure != NULL && (fishing_flags & FISHING_PULL)) {
            fishing_reel_turning(task, &tip);
        } else {
            buttons = 0;
            if (pad_records[0].on & pad_action_buttons) {
                buttons = HELD_ACTION;
            }
            if (PAD_BUTTONS(pad_records[0].on, pad_jump_buttons)) {
                buttons |= HELD_JUMP;
            }
            switch (buttons) {
            case HELD_ACTION:
                fishing_reel_slow(task, &tip);
                break;
            case HELD_JUMP:
            case HELD_ACTION | HELD_JUMP:
                fishing_reel_fast(task, &tip);
                break;
            default:
                sound_stop(SE_REEL);
                fishing_lure_drift(task);
                break;
            }
        }
        move = motion->speed;
        wall_slope.x = wall_slope.y = wall_slope.z = 0;
        pos = work->pos;
        wall = world_sweep_all_hit(&pos, &move, &wall_slope, radius);
        if (stage_get_stage_act() == ICECAP_POND && wall_slope.x == 0 &&
            wall_slope.z == 0x8000) {
            wall = FALSE;
            move.y = motion->speed.y;
        }
        floor_slope.x = floor_slope.y = floor_slope.z = 0;
        pos = work->pos;
        floor = fishing_sweep(&pos, &move, &floor_slope, radius);
        /* @hack spelling: the four tests joined by &, as in the arm above (JSR
         * at 0x8c0f36be and 0x8c0f36ca). as there, & gives what && does. with
         * &&, 201 of 631 words differ */
        if (wall & floor & fishing_upright(NULL, &floor_slope) &
            fishing_upright(NULL, &wall_slope)) {
            move.x = move.z = 0.0F;
            move.y *= 3.0F;
        }
        if (work->pos.y + move.y > info.water.height - radius - 0.1F) {
            if (!(fishing_upright(task, &wall_slope) ||
                  fishing_upright(task, &floor_slope)) ||
                IN_STAGE(STAGE_BEACH) || (fishing_flags & FISHING_PULL)) {
                move.y = 0.0F;
                work->pos.y = info.water.height - radius - 0.1F;
                motion->speed.y = 0.0F;
            } else {
                fishing_lure_leave_water(task);
                return;
            }
        }
        if (work->pos.y + move.y <= info.below.height + radius) {
            length = njScalor(&move);
            work->pos.y = info.below.height + radius;
            if (big_lure != NULL) {
                fishing_flags |= FISHING_BOTTOM;
                if (!(fishing_flags & FISHING_SPLASH) && length >= 0.15F) {
                    fishing_splash_create(task);
                    sound_play(SE_LURE_BOTTOM, 0, 0, 0);
                    fishing_flags |= FISHING_SPLASH;
                }
                if (length <= 0.03F) {
                    move.x = move.y = move.z = 0.0F;
                }
            }
        }
        work->pos.x += move.x;
        work->pos.y += move.y;
        work->pos.z += move.z;
        fishing_tension_step(task, &move);
        fishing_tension_aim += rand() * (0.1F * RAND_SCALE);
        if (work->pos.y > info.water.height - radius) {
            fishing_lure_leave_water(task);
        }
    }
}

/* hang the lure from the rod's tip, facing as player 0 does, and rock it.
 * about x, it turns 0x200 a step, and back each time it passes 0x6000 or
 * 0xa000. about z, it turns 0x80 a step, and back past 0x3000 or 0x5000 */
static void fishing_lure_dangle(struct task *task, NJS_POINT3 *tip)
{
    Angle ang;
    Angle aim;
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];

    work->pos = *tip;
    work->ang.y = task->motion->aim.y = player->ang.y;
    /* @hack spelling: the turns read and stored through task->work, which the
     * image reloads from task (MOV.L @(32,R4),R5 at 0x8c0f38fc); through work,
     * which it holds in R6 from 0x8c0f38e2, 82 of 90 words differ */
    ang = task->work->ang.x;
    aim = task->motion->aim.x;
    if (ang < 0x5800 || ang > 0xa800) {
        ang = 0x5800;
    }
    if (ang < 0x6000 && ang > 0x5000) {
        aim = 0xa000;
    }
    if (ang > 0xa000 && ang < 0xb000) {
        aim = 0x6000;
    }
    if (aim < 0x8000) {
        ang = (Uint16)(ang - 0x200);
    } else {
        ang = (Uint16)(ang + 0x200);
    }
    task->motion->aim.x = aim;
    task->work->ang.x = ang;
    ang = task->work->ang.z;
    aim = task->motion->aim.z;
    if (ang < 0x2800 || ang > 0x5800) {
        ang = 0x5800;
    }
    if (ang < 0x3000 && ang > 0x2000) {
        aim = 0x5000;
    }
    if (ang > 0x5000 && ang < 0x6000) {
        aim = 0x3000;
    }
    if (aim < 0x4000) {
        ang = (Uint16)(ang - 0x80);
    } else {
        ang = (Uint16)(ang + 0x80);
    }
    task->motion->aim.z = aim;
    task->work->ang.z = ang;
}

/* put the lure back on the rod as LURE_DANGLE, with no line out and no tension,
 * and the reel at rest. the music comes back if a fish was hooked */
static void fishing_lure_reset(struct task *task)
{
    struct task_work *work = task->work;
    struct lure_extra *extra = task->extra;

    work->mode = LURE_DANGLE;
    extra->sway[SWAY_PHASE] = 0;
    extra->sway[SWAY_ROLL] = 0;
    fishing_tension_aim = fishing_tension = 0.0F;
    fishing_reel_ang = 0;
    fishing_line_out = 0.0F;
    fishing_distance = 0.0F;
    if (fishing_flags & FISHING_HOOKED) {
        stream_restart_music();
    }
    sound_stop(SE_REEL);
}

/* take the lure out of the water. a hooked fish within 150 of player 0
 * comes with it, as LURE_CATCH, and the camera looks at Big. a fish
 * further off is killed, and the lure comes back empty as LURE_BACK, as
 * with no fish, with the camera on it. at Hot Shelter, the camera looks at
 * Big either way. nothing in a field */
static void fishing_lure_leave_water(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];

    if (stage_group() == STAGE_GROUP_FIELD) {
        return;
    }
    sound_stop(SE_REEL);
    if (big_lure != NULL) {
        fishing_flags &= ~FISHING_CAMERA;
        if (work->mode <= LURE_LANDED) {
            camera_restore();
        } else {
            camera_restore_high();
        }
        if (fishing_flags & FISHING_HOOKED) {
            stream_restart_music();
            if (njDistanceP2P(&work->pos, &player->pos) < 150.0F) {
                work->mode = LURE_CATCH;
                camera_override(fishing_camera_step_point, 0, CAMERA_LOOK_AT);
                fishing_look_at_big(task);
            } else {
                work->mode = LURE_BACK;
                if (big_catch != NULL) {
                    task_kill(big_catch);
                }
                big_catch = NULL;
                fishing_flags &=
                    ~(FISHING_HOOKED | FISHING_LOST | FISHING_KICK);
                if (IN_STAGE(STAGE_SHELTER)) {
                    camera_override(fishing_camera_step_point, 0,
                                    CAMERA_LOOK_AT);
                    fishing_look_at_big(task);
                } else {
                    camera_override(camera_step_fishing_point, 0,
                                    CAMERA_LOOK_AT);
                    fishing_look_at_lure(task);
                }
            }
        } else {
            work->mode = LURE_BACK;
            if (IN_STAGE(STAGE_SHELTER)) {
                camera_override(fishing_camera_step_point, 0, CAMERA_LOOK_AT);
                fishing_look_at_big(task);
            } else {
                camera_override(camera_step_fishing_point, 0, CAMERA_LOOK_AT);
                fishing_look_at_lure(task);
            }
        }
    }
}

/* TRUE when the surface below or above the lure, as info gives them, is
 * water, and the lure's collision hit nothing */
static Bool fishing_lure_in_water(struct task *task, struct near_surfaces *info)
{
    if ((!(info->below.flags & WORLD_WATER) &&
         !(info->above.flags & WORLD_WATER)) ||
        (task->work->collision->flags & COLLISION_HIT)) {
        return FALSE;
    }
    return TRUE;
}

/* TRUE when big_catch is the plug of Hot Shelter, which stands at (750,
 * 75.5, -650). it then sets FISHING_SHELTER_PLUG. FALSE otherwise */
static Bool fishing_mark_shelter_plug(void)
{
    if (big_catch != NULL && IN_STAGE(STAGE_SHELTER) &&
        big_catch->work->pos.x == 750.0F && big_catch->work->pos.y == 75.5F &&
        big_catch->work->pos.z == -650.0F) {
        fishing_flags |= FISHING_SHELTER_PLUG;
        return TRUE;
    }
    return FALSE;
}

/* the lure task's step, by its lure_mode. each step, the lure rocks, and
 * FISHING_TWITCH and FISHING_BOTTOM clear */
static void fishing_lure_step(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: the goal read through extra in the cast and the flight,
     * which the image keeps in R9 (MOV.L @(44,R14),R9 at 0x8c0f3cda), and
     * the rest through LURE(task), which it reloads from task (MOV.L
     * @(44,R14),R2 at 0x8c0f3dd6); through extra alone, 741 of 814 words
     * differ */
    struct lure_extra *extra = task->extra;
    struct task_motion *motion = task->motion;
    struct player *record = player_motion[0]->record;
    NJS_VECTOR away;
    struct near_surfaces info;
    NJS_POINT3 tip;
    NJS_POINT3 end;
    Float distance;
    Float steps;
    int kind;

    fishing_rod_tip(task, &tip);
    if (work->mode != LURE_INIT) {
        fishing_lure_sway(task);
    }
    if (big_lure != NULL) {
        fishing_flags &= ~(FISHING_TWITCH | FISHING_BOTTOM);
    }
    switch (work->mode) {
    case LURE_INIT:
        fishing_lure_reset(task);
        task->destructor = fishing_lure_destroy;
        task->draw = fishing_lure_draw;
        break;
    case LURE_DANGLE:
        fishing_lure_dangle(task, &tip);
        fishing_lure_show(task);
        if (big_lure != NULL && (fishing_flags & FISHING_CAST)) {
            fishing_flags &= ~FISHING_CAST;
            work->mode = LURE_WAIT;
            LURE(task)->goal->x = BIG_TARGET(record)->work->pos.x;
            LURE(task)->goal->y = TARGET(BIG_TARGET(record))->ground;
            LURE(task)->goal->z = BIG_TARGET(record)->work->pos.z;
            task_kill(BIG_TARGET(record));
            LURE_VARS(work)->steps = 10;
            fishing_flags = 0;
        }
        break;
    case LURE_WAIT:
        fishing_lure_dangle(task, &tip);
        fishing_lure_show(task);
        if (--LURE_VARS(work)->steps) {
            break;
        }
        away.x = extra->goal->x - work->pos.x;
        away.y = 0.0F;
        away.z = extra->goal->z - work->pos.z;
        distance = njScalor(&away);
        steps = distance * (1.0F / CAST_SPEED);
        collision_init(task, fishing_lure_shapes, COLLISION_ATTACK, 1, 1);
        work->mode = LURE_FLY;
        motion->speed.x = away.x / steps;
        motion->speed.z = away.z / steps;
        motion->speed.y =
            ((extra->goal->y - work->pos.y) / steps) + (steps * 0.01F);
        motion->aim.x = 0;
        work->ang.z = 0;
        fishing_line_end(&end);
        if (LURE(task)->line != NULL) {
            fishing_line_reset(LURE(task)->line, &tip, &end);
        }
        sound_play(SE_CAST, 0, 0, 0);
        sound_play(SE_CAST_SWING, 0, 0, 0);
        break;
    case LURE_FLY:
        LURE_VARS(work)->steps++;
        if (fishing_lure_fly(task)) {
            fishing_find_surfaces_at(work, 0, &info);
            if (!fishing_lure_in_water(task, &info)) {
                if (big_lure != NULL) {
                    fishing_flags &= ~FISHING_CAMERA;
                }
                work->mode = LURE_BACK;
                sound_play(SE_LURE_GROUND, 0, 0, 0);
            } else {
                work->mode = LURE_LANDED;
                work->pos.x = extra->goal->x;
                work->pos.y = info.water.height - 0.6F;
                work->pos.z = extra->goal->z;
                /* @hack redundant: a goto where the else ends anyway, which
                 * loads njDistanceP2P's address into R3 for the else-if's call
                 * (MOV.L at 0x8c0f3fe0); without it, R2 takes it and 2 of
                 * 814 words differ */
                goto flown;
            }
        } else if (njDistanceP2P(extra->goal, &work->pos) < 3.0F) {
            work->mode = LURE_LANDED;
            VECTOR_COPY(&work->pos, extra->goal);
            fishing_find_surfaces_at(work, 0, &info);
            if (!fishing_lure_in_water(task, &info)) {
                if (big_lure != NULL) {
                    fishing_flags &= ~FISHING_CAMERA;
                }
                work->mode = LURE_BACK;
                sound_play(SE_LURE_GROUND, 0, 0, 0);
            } else {
                work->pos.y = info.water.height - 0.6F;
            }
        }
    flown:
        fishing_line_end(&end);
        if (LURE(task)->line != NULL) {
            fishing_line_step(LURE(task)->line, &tip, &end, FISHING_LINE_SLACK);
        }
        fishing_lure_show(task);
        collision_register(work);
        break;
    case LURE_BACK:
    case LURE_CATCH:
        if (fishing_lure_return(task)) {
            fishing_lure_home(task);
        } else {
            fishing_line_end(&end);
            if (LURE(task)->line != NULL) {
                fishing_line_step(LURE(task)->line, &tip, &end,
                                  FISHING_LINE_TAUT);
            }
            fishing_lure_show(task);
        }
        break;
    case LURE_LANDED:
        if (stage_group() != STAGE_GROUP_FIELD) {
            fishing_cast_pos = work->pos;
        }
        work->pos.y -= 0.5001F;
        work->mode = LURE_IN_WATER;
        fishing_lure_camera(task);
        fishing_lure_swim(task);
        fishing_line_end(&end);
        if (LURE(task)->line != NULL) {
            fishing_line_step(LURE(task)->line, &tip, &end, FISHING_LINE_SLACK);
        }
        fishing_lure_show(task);
        if (stage_group() == STAGE_GROUP_FIELD) {
            if (njDistanceP2P(&player_work[0]->pos, &fishing_cast_pos) >
                    20.0F &&
                (kind = fishing_give_prize()) > 0) {
                item_show(kind);
                fishing_cast_pos = player_work[0]->pos;
            }
            fishing_flags &= ~FISHING_CAMERA;
            work->mode = LURE_BACK;
            camera_restore();
        } else if (big_not_fishing(task)) {
            fishing_lure_reset(task);
        } else if (fishing_lure_at_rod(&tip, &work->pos)) {
            fishing_flags &= ~FISHING_CAMERA;
            work->mode = LURE_BACK;
        } else if (work->mode == LURE_IN_WATER) {
            if (big_lure != NULL) {
                fishing_flags |= FISHING_CAMERA;
            }
            fishing_cast_pos = work->pos;
            fishing_line_out = fishing_distance;
            VECTOR_SCALE(&motion->speed, &motion->speed, 0.25F);
            /* @hack spelling: the turn cleared through task->work, which has
             * the store share R0's 0 with sound_play's arguments (MOV #0,R0 at
             * 0x8c0f42a8, MOV.L @(32,R14),R3 at 0x8c0f42aa); through work,
             * the 0 takes R3, and 24 of 814 words differ */
            task->work->ang.x = 0;
            sound_play(SE_LURE_WATER, 0, 0, 0);
            camera_restore();
            camera_override_high(fishing_camera_step_lure, 0, CAMERA_LOOK_AT);
        }
        break;
    case LURE_IN_WATER:
        fishing_lure_camera(task);
        fishing_lure_swim(task);
        fishing_lure_pitch(task);
        fishing_lure_yaw(task);
        fishing_lure_roll(task);
        fishing_line_end(&end);
        if (LURE(task)->line != NULL) {
            fishing_line_step(LURE(task)->line, &tip, &end, FISHING_LINE_TAUT);
        }
        fishing_lure_show(task);
        if (TRIGGER_AXIS(pad_records[0].l) > PAD_DEAD_ZONE - 0x8000 &&
            TRIGGER_AXIS(pad_records[0].r) > PAD_DEAD_ZONE - 0x8000 &&
            !(fishing_flags & FISHING_HOOKED)) {
            fishing_flags |= FISHING_GIVE_UP;
            camera_restore_high();
            if (IN_STAGE(STAGE_SHELTER)) {
                camera_override(fishing_camera_step_point, 0, CAMERA_LOOK_AT);
                fishing_look_at_big(task);
            } else {
                camera_override(camera_step_fishing_point, 0, CAMERA_LOOK_AT);
                fishing_look_at_lure(task);
            }
        }
        if ((fishing_flags & FISHING_SHELTER_PLUG) && IN_STAGE(STAGE_SHELTER) &&
            !(fishing_flags & FISHING_HOOKED)) {
            fishing_flags |= FISHING_GIVE_UP;
            camera_restore_high();
            if (IN_STAGE(STAGE_SHELTER)) {
                camera_override(fishing_camera_step_point, 0, CAMERA_LOOK_AT);
                fishing_look_at_big(task);
            } else {
                camera_override(camera_step_fishing_point, 0, CAMERA_LOOK_AT);
                fishing_look_at_lure(task);
            }
            fishing_flags &= ~FISHING_SHELTER_PLUG;
        }
        if (fishing_flags & FISHING_GIVE_UP) {
            if (big_lure != NULL) {
                fishing_flags &= ~FISHING_CAMERA;
                work->mode = LURE_BACK;
                sound_stop(SE_REEL);
            }
        } else if (big_not_fishing(task)) {
            fishing_lure_reset(task);
        } else if (big_lure != NULL && (fishing_flags & FISHING_HOOKED)) {
            work->mode = LURE_FISH;
            fishing_mark_shelter_plug();
        }
        break;
    case LURE_FISH:
        fishing_lure_camera(task);
        fishing_lure_swim(task);
        fishing_lure_pitch(task);
        fishing_lure_yaw(task);
        fishing_lure_roll(task);
        fishing_line_end(&end);
        if (LURE(task)->line != NULL) {
            fishing_line_step(LURE(task)->line, &tip, &end, FISHING_LINE_TAUT);
        }
        fishing_lure_show(task);
        if (big_lure != NULL && !(fishing_flags & FISHING_HOOKED)) {
            work->mode = LURE_IN_WATER;
            stream_restart_music();
        } else if (big_not_fishing(task)) {
            fishing_lure_reset(task);
        }
        break;
    }
}

/* the lure is home: Big's fishing ends, and the lure goes back on the rod.
 * with a fish hooked, FISHING_LANDED sets, and the camera looks at Big */
static void fishing_lure_home(struct task *task)
{
    struct player *record = player_motion[0]->record;

    BIG_FLAGS(record) |= BIG_FISHING_OVER;
    if (big_lure != NULL) {
        fishing_flags &= ~FISHING_CAMERA;
        if (big_catch != NULL && (fishing_flags & FISHING_HOOKED)) {
            camera_restore();
            camera_override(fishing_camera_step_point, 0, CAMERA_LOOK_AT);
            fishing_flags |= FISHING_LANDED;
            fishing_look_at_big(task);
        }
    }
    fishing_lure_reset(task);
}

/* fill info with the surfaces nearest (x, y, z) among the world's objects
 * within 25 of it, and return below's height. skip leaves water out of above
 * or below. an object a task owns counts only while the point lies within its
 * model's radius, times its largest scale, in x and z. the speed.y of that
 * task's force raises the object's surfaces. world_objects keeps the objects
 * it listed */
static Float fishing_find_surfaces(Float x, Float y, Float z, int skip,
                                   struct near_surfaces *info)
{
    struct height_query query;
    NJS_VECTOR away;
    Angle above_ang_x;
    Angle above_ang_z;
    Sint32 above_flags;
    Angle below_ang_x;
    Angle below_ang_z;
    Sint32 below_flags;
    struct world_object *objects;
    NJS_OBJECT *object;
    Uint16 i;
    Float above;
    Float below;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Float height;

    query.pos.x = x;
    query.pos.y = y;
    query.pos.z = z;
    below = -GROUND_FAR;
    below_flags = 0;
    below_ang_z = 0;
    below_ang_x = 0;
    above_flags = 0;
    above_ang_z = 0;
    above_ang_x = 0;
    above = info->water.height = GROUND_FAR;
    info->water.flags = info->water.ang_z = info->water.ang_x = 0;
    objects = world_objects;
    world_list_near(x, y, z, 25.0F);
    for (i = 0; i < world_count; i++) {
        object = objects[i].object;
        if (objects[i].owner != NULL) {
            Float size;
            Float distance;
            Float reach;

            away.x = object->pos[0] - x;
            away.y = 0.0F;
            away.z = object->pos[2] - z;
            distance = njScalor(&away);
            size = object->scl[0];
            if (object->scl[1] > size) {
                size = object->scl[1];
            }
            if (object->scl[2] > size) {
                size = object->scl[2];
            }
            reach = size * object->model->r;
            if (distance > reach) {
                continue;
            }
        }
        if (polygon_find_heights(&query, objects[i].object, objects[i].flags)) {
            if (objects[i].owner != NULL && objects[i].owner->force != NULL) {
                query.above.level +=
                    ((struct task_force *)objects[i].owner->force)->speed.y;
                query.below.level +=
                    ((struct task_force *)objects[i].owner->force)->speed.y;
            }
            /* @hack redundant: a store to height, which nothing reads, that
             * keeps above_flags at @(20,R15) and below_flags at @(24,R15), as
             * their reads at 0x8c0f4898 and 0x8c0f48a2 show; without it, the
             * two trade and 8 of 311 words differ */
            /* @hack redundant: the select on the constant 1.0F, which picks
             * below; without it, 8 of 311 words differ */
            height = 1.0F ? below : query.below.level;
            if (query.above.found == TRUE && query.pos.y < query.above.level &&
                above > query.above.level) {
                if (objects[i].flags & WORLD_WATER) {
                    if (_builtin_fabsf(query.above.level - query.pos.y) <
                        _builtin_fabsf(info->water.height - query.pos.y)) {
                        info->water.height = query.above.level;
                        info->water.ang_x = query.above.ang_x;
                        info->water.ang_z = query.above.ang_z;
                        info->water.flags = objects[i].flags;
                    }
                    if (!(skip & SKIP_WATER_ABOVE)) {
                        above = query.above.level;
                        above_ang_x = query.above.ang_x;
                        above_ang_z = query.above.ang_z;
                        above_flags = objects[i].flags;
                    }
                } else {
                    above = query.above.level;
                    above_ang_x = query.above.ang_x;
                    above_ang_z = query.above.ang_z;
                    above_flags = objects[i].flags;
                }
            }
            if (query.below.found == TRUE && query.pos.y >= query.below.level &&
                query.below.level > below) {
                if (objects[i].flags & WORLD_WATER) {
                    if (_builtin_fabsf(query.below.level - query.pos.y) <
                        _builtin_fabsf(info->water.height - query.pos.y)) {
                        info->water.height = query.below.level;
                        info->water.ang_x = query.below.ang_x;
                        info->water.ang_z = query.below.ang_z;
                        info->water.flags = objects[i].flags;
                    }
                    if (!(skip & SKIP_WATER_BELOW)) {
                        below = query.below.level;
                        below_ang_x = query.below.ang_x;
                        below_ang_z = query.below.ang_z;
                        below_flags = objects[i].flags;
                    }
                } else {
                    below = query.below.level;
                    below_ang_x = query.below.ang_x;
                    below_ang_z = query.below.ang_z;
                    below_flags = objects[i].flags;
                }
            }
        }
    }
    info->above.height = above;
    info->above.ang_x = above_ang_x;
    info->above.ang_z = above_ang_z;
    info->above.flags = above_flags;
    info->below.height = below;
    info->below.ang_x = below_ang_x;
    info->below.ang_z = below_ang_z;
    info->below.flags = below_flags;
    return below;
}

/* fishing_find_surfaces, at the work's pos */
static Float fishing_find_surfaces_at(struct task_work *work, int skip,
                                      struct near_surfaces *info)
{
    struct height_query query;
    NJS_VECTOR away;
    Angle above_ang_x;
    Angle above_ang_z;
    Sint32 above_flags;
    Angle below_ang_x;
    Angle below_ang_z;
    Sint32 below_flags;
    struct world_object *objects;
    NJS_OBJECT *object;
    Uint16 i;
    Float above;
    Float below;
    Float x;
    Float y;
    Float z;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Float height;

    x = work->pos.x;
    y = work->pos.y;
    z = work->pos.z;
    query.pos.x = x;
    query.pos.y = y;
    query.pos.z = z;
    below = -GROUND_FAR;
    below_flags = 0;
    below_ang_z = 0;
    below_ang_x = 0;
    above_flags = 0;
    above_ang_z = 0;
    above_ang_x = 0;
    above = GROUND_FAR;
    info->water.height = above;
    info->water.flags = info->water.ang_z = info->water.ang_x = 0;
    objects = world_objects;
    /* @hack redundant: height, which nothing reads, takes a select on z,
     * which keeps x in FR12 and z in FR13 (0x8c0f48ec, 0x8c0f48f8); without
     * it, the two trade and 8 of 323 words differ */
    height = z ? query.pos.x : query.pos.z;
    world_list_near(x, y, z, 25.0F);
    for (i = 0; i < world_count; i++) {
        object = objects[i].object;
        if (objects[i].owner != NULL) {
            Float size;
            Float distance;
            Float reach;

            away.x = object->pos[0] - x;
            away.y = 0.0F;
            away.z = object->pos[2] - z;
            distance = njScalor(&away);
            size = object->scl[0];
            if (object->scl[1] > size) {
                size = object->scl[1];
            }
            if (object->scl[2] > size) {
                size = object->scl[2];
            }
            reach = size * object->model->r;
            if (distance > reach) {
                continue;
            }
        }
        if (polygon_find_heights(&query, objects[i].object, objects[i].flags)) {
            if (objects[i].owner != NULL && objects[i].owner->force != NULL) {
                query.above.level +=
                    ((struct task_force *)objects[i].owner->force)->speed.y;
                query.below.level +=
                    ((struct task_force *)objects[i].owner->force)->speed.y;
            }
            /* @hack redundant: a store to height, which nothing reads, that
             * keeps above_flags at @(20,R15) and below_flags at @(24,R15), as
             * their reads at 0x8c0f4b60 and 0x8c0f4b6e show; without it, the
             * two trade and 8 of 323 words differ */
            /* @hack redundant: the select on the constant 1.0F, which picks
             * below; without it, 8 of 323 words differ */
            height = 1.0F ? below : query.below.level;
            if (query.above.found == TRUE && query.pos.y < query.above.level &&
                above > query.above.level) {
                if (objects[i].flags & WORLD_WATER) {
                    if (_builtin_fabsf(query.above.level - query.pos.y) <
                        _builtin_fabsf(info->water.height - query.pos.y)) {
                        info->water.height = query.above.level;
                        info->water.ang_x = query.above.ang_x;
                        info->water.ang_z = query.above.ang_z;
                        info->water.flags = objects[i].flags;
                    }
                    if (!(skip & SKIP_WATER_ABOVE)) {
                        above = query.above.level;
                        above_ang_x = query.above.ang_x;
                        above_ang_z = query.above.ang_z;
                        above_flags = objects[i].flags;
                    }
                } else {
                    above = query.above.level;
                    above_ang_x = query.above.ang_x;
                    above_ang_z = query.above.ang_z;
                    above_flags = objects[i].flags;
                }
            }
            if (query.below.found == TRUE && query.pos.y >= query.below.level &&
                query.below.level > below) {
                if (objects[i].flags & WORLD_WATER) {
                    if (_builtin_fabsf(query.below.level - query.pos.y) <
                        _builtin_fabsf(info->water.height - query.pos.y)) {
                        info->water.height = query.below.level;
                        info->water.ang_x = query.below.ang_x;
                        info->water.ang_z = query.below.ang_z;
                        info->water.flags = objects[i].flags;
                    }
                    if (!(skip & SKIP_WATER_BELOW)) {
                        below = query.below.level;
                        below_ang_x = query.below.ang_x;
                        below_ang_z = query.below.ang_z;
                        below_flags = objects[i].flags;
                    }
                } else {
                    below = query.below.level;
                    below_ang_x = query.below.ang_x;
                    below_ang_z = query.below.ang_z;
                    below_flags = objects[i].flags;
                }
            }
        }
    }
    info->above.height = above;
    info->above.ang_x = above_ang_x;
    info->above.ang_z = above_ang_z;
    info->above.flags = above_flags;
    info->below.height = below;
    info->below.ang_x = below_ang_x;
    info->below.ang_z = below_ang_z;
    info->below.flags = below_flags;
    return below;
}

/* turn the lure's model within the lure. with a fish on, the model tips 80
 * degrees about x, and turns about y with the fish. on the bottom, its last
 * turn about z becomes its turn about x. otherwise it rocks 0x1000 either way
 * about x. about z, it turns 5 degrees a step to undo the lure's own turn
 * about z while the lure sits still. while the lure moves or dangles, or a
 * reel button is pressed, the model's turn about z goes back to 80 degrees */
static void fishing_lure_sway(struct task *task)
{
    NJS_OBJECT *lure = fishing_lures[fishing_count_lures()]->child;
    NJS_VECTOR flat;
    int ang;
    int off;
    Float speed;
    int phase;

    lure->evalflags &= ~NJD_EVAL_UNIT_ANG;
    if (big_lure == NULL) {
        return;
    }
    if (fishing_flags & FISHING_HOOKED) {
        lure->ang[0] = NJM_DEG_ANG(80);
        lure->ang[2] = 0;
        if (big_catch != NULL) {
            lure->ang[1] =
                big_catch->work->ang.y + big_lure->work->ang.y + 0xc000;
        } else {
            lure->ang[1] = -0x4000;
        }
    } else if (fishing_flags & FISHING_BOTTOM) {
        lure->ang[0] = LURE(task)->sway[SWAY_ROLL];
        lure->ang[1] = 0;
        lure->ang[2] = LURE(task)->sway[SWAY_BOTTOM_Z];
    } else {
        /* @hack spelling: the phase read, then advanced, in two statements,
         * which adds the read into 0x800's register (ADD R4,R1 at
         * 0x8c0f4c6a); as one statement, the add's operands swap and 2 of
         * 152 words differ */
        phase = LURE(task)->sway[SWAY_PHASE];
        phase = (Uint16)(phase + 0x800);
        LURE(task)->sway[SWAY_PHASE] = phase;
        lure->ang[0] = njSin(phase) * 4096.0F;
        ang = LURE(task)->sway[SWAY_ROLL];
        flat.x = task->motion->speed.x;
        flat.y = 0.0F;
        flat.z = task->motion->speed.z;
        speed = njScalor(&flat);
        if (speed > 0.1F || (fishing_flags & FISHING_BOTTOM) ||
            PAD_BUTTONS(pad_records[0].press,
                        pad_action_buttons | pad_jump_buttons) ||
            task->work->mode < LURE_FLY) {
            ang = NJM_DEG_ANG(80);
        } else {
            off = (Uint16)(task->work->ang.z + ang);
            if (off < 0x8000) {
                ang -= NJM_DEG_ANG(5);
            } else if (off > 0x8000) {
                ang += NJM_DEG_ANG(5);
            }
            if (off < NJM_DEG_ANG(5) || off > NJM_DEG_ANG(355)) {
                ang = -task->work->ang.z;
            }
        }
        LURE(task)->sway[SWAY_ROLL] = ang;
        lure->ang[2] = ang;
    }
}

/* set the lure's aim about y to its bearing from player 0, and tip the lure
 * about x. hooked, or with no speed in x and z, it levels 0x80 a step. on
 * the bottom, it tips 0x80 a step toward 0x1800. there a tilt past 0x1800,
 * even one below level, goes to 0x1800 at once, as the tag on its test
 * says. moving, it rocks 0x200 a step, to 0x800 either side of level. with
 * a jump button held, it rocks 0xc00 a step, to 0x1400. nothing while
 * big_lure is null */
static void fishing_lure_pitch(struct task *task)
{
    struct task_work *player = player_work[0];
    int ang;
    int aim;
    int limit;
    int step;
    int buttons;

    if (big_lure == NULL) {
        return;
    }
    task->motion->aim.y =
        0x4000 - (Angle)(atan2f(task->work->pos.x - player->pos.x,
                                task->work->pos.z - player->pos.z) *
                         ANGLE_PER_RADIAN);
    /* @hack spelling: the hooked arm stores its turn and returns, with an else
     * after the return, which keeps 0x8000 in R13 (MOV.L at 0x8c0f4db2);
     * with one store after the chain, R7 takes it and 21 of 182 words
     * differ */
    if (fishing_flags & FISHING_HOOKED) {
        ang = task->work->ang.x;
        if (ang > 0x8000) {
            ang += 0x80;
            if (ang >= 0x10000) {
                ang = 0;
            }
        } else {
            ang -= 0x80;
            if (ang <= 0) {
                ang = 0;
            }
        }
        task->work->ang.x = ang;
        return;
    } else {
        if (fishing_flags & FISHING_BOTTOM) {
            ang = task->work->ang.x;
            /* @bug logic: every store to the lure's ang.x before it
             * reaches the bottom leaves 0 to 0xffff, so this test (CMP/PZ at
             * 0x8c0f4e38) always holds. a tilt below level, 0xe800 to
             * 0xffff, goes to 0x1800, and the arm meant for it (0x8c0f4e4c)
             * never runs */
            if (ang >= 0) {
                ang += 0x80;
                if (ang > 0x1800) {
                    ang = 0x1800;
                }
            } else {
                ang -= 0x80;
                ang = (Uint16)ang;
                if (ang < 0xe800) {
                    ang = 0xe800;
                }
            }
        } else {
            ang = task->work->ang.x;
            if (task->motion->speed.x == 0.0F &&
                task->motion->speed.z == 0.0F) {
                if (ang > 0x8000) {
                    ang += 0x80;
                    if (ang >= 0x10000) {
                        ang = 0;
                    }
                } else {
                    ang -= 0x80;
                    if (ang <= 0) {
                        ang = 0;
                    }
                }
            } else {
                buttons = 0;
                if (pad_records[0].on & pad_action_buttons) {
                    buttons = HELD_ACTION;
                }
                if (PAD_BUTTONS(pad_records[0].on, pad_jump_buttons)) {
                    buttons |= HELD_JUMP;
                }
                switch (buttons) {
                case HELD_ACTION:
                default:
                    step = 0x200;
                    limit = 0x800;
                    break;
                case HELD_JUMP:
                case HELD_ACTION | HELD_JUMP:
                    step = 0xc00;
                    limit = 0x1400;
                    break;
                }
                aim = task->motion->aim.x;
                if (ang > limit && ang < limit + 0x3000) {
                    aim = 0x10000 - limit;
                }
                if (ang > 0xd000 - limit && ang < 0x10000 - limit) {
                    aim = limit;
                }
                if (aim < 0x8000) {
                    ang = (Uint16)(ang + step);
                } else {
                    ang = (Uint16)(ang - step);
                }
                task->motion->aim.x = aim;
            }
        }
        task->work->ang.x = ang;
    }
}

/* turn the lure 0x40 a step about y toward its aim, and onto it once
 * within 0x80 */
static void fishing_lure_yaw(struct task *task)
{
    Angle ang = task->work->ang.y;
    Angle aim = task->motion->aim.y;
    int off = (Uint16)(ang - aim);

    if (off < 0x8000) {
        ang -= 0x40;
    } else if (off > 0x8000) {
        ang += 0x40;
    }
    if (off < 0x80 || off > 0xff80) {
        ang = aim;
    }
    task->work->ang.y = ang;
}

/* turn the lure 0x80 a step about z toward its aim, and onto it once
 * within 0x80. the aim is level, but at Ice Cap's pond, off the bottom, it
 * is the slope of the way to the rod's tip. nothing while big_lure is
 * null */
static void fishing_lure_roll(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang = work->ang.z;
    NJS_VECTOR tip;
    Angle aim;
    int off;

    if (big_lure == NULL) {
        return;
    }
    if (!(fishing_flags & FISHING_BOTTOM) &&
        stage_get_stage_act() == ICECAP_POND) {
        fishing_rod_tip(task, &tip);
        tip.x -= work->pos.x;
        tip.y -= work->pos.y;
        tip.z -= work->pos.z;
        njUnitVector(&tip);
        aim = task->motion->aim.z =
            -(Angle)(asinf(tip.y) * ANGLE_PER_RADIAN) & 0xffff;
    } else {
        aim = task->motion->aim.z = 0;
    }
    off = (Uint16)(ang - aim);
    if (off < 0x8000) {
        ang -= 0x80;
    } else if (off > 0x8000) {
        ang += 0x80;
    }
    if (off < 0x80 || off > 0xff80) {
        ang = aim;
    }
    /* @hack spelling: the turn stored through task->work, which the image
     * reloads from task (MOV.L @(32,R13),R3 at 0x8c0f510c); through work,
     * task and work trade R12 and R13, and 20 of 101 words differ */
    task->work->ang.z = (Uint16)ang;
}

void fishing_hook_point(NJS_POINT3 *pos)
{
    NJS_MATRIX matrix;
    NJS_VECTOR hook;
    struct task_work *work;

    if (big_lure == NULL) {
        pos->x = pos->y = pos->z = 0.0F;
        return;
    }
    work = big_lure->work;
    hook.x = 0.6F;
    hook.y = -(0.3F * 1.5F);
    hook.z = 0.0F;
    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateZ, &matrix, work->ang.z);
    TURN_MATRIX(njRotateX, &matrix, work->ang.x);
    TURN_MATRIX(njRotateY, &matrix, -work->ang.y);
    njCalcVector(&matrix, &hook, pos);
    VECTOR_ADD(pos, &work->pos);
}

void fishing_catch_point(NJS_POINT3 *pos)
{
    NJS_MATRIX matrix;
    NJS_VECTOR offset;
    NJS_POINT3 base;
    struct task_work *work;

    if (big_lure == NULL) {
        pos->x = pos->y = pos->z = 0.0F;
        return;
    }
    work = big_lure->work;
    base = big_catch->work->pos;
    offset.x = -2.0F;
    offset.y = 1.5F;
    offset.z = 0.0F;
    njUnitMatrix(&matrix);
    njRotateXYZ(&matrix, work->ang.x, -work->ang.y, work->ang.z);
    njCalcVector(&matrix, &offset, pos);
    pos->x += base.x;
    pos->y += base.y;
    pos->z += base.z;
}

/* set pos to where the line meets the lure: its position plus (-0.6, 0.3,
 * 0), turned as the lure is. the origin while big_lure is null */
static void fishing_line_end(NJS_POINT3 *pos)
{
    NJS_MATRIX matrix;
    NJS_VECTOR end;
    struct task_work *work;

    if (big_lure == NULL) {
        pos->x = pos->y = pos->z = 0.0F;
        return;
    }
    work = big_lure->work;
    end.x = -0.6F;
    end.y = 0.3F;
    end.z = 0.0F;
    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateY, &matrix, -work->ang.y);
    TURN_MATRIX(njRotateX, &matrix, work->ang.x);
    TURN_MATRIX(njRotateZ, &matrix, work->ang.z);
    njCalcVector(&matrix, &end, pos);
    VECTOR_ADD(pos, &work->pos);
}

/* set the draw state for the lure: its model's own materials, not the
 * constant one, blended by their alpha */
static void fishing_set_draw(void)
{
    draw_control_clear(NJD_CONTROL_3D_CONSTANT_MATERIAL);
    draw_control_set(NJD_CONTROL_3D_OFFSET_MATERIAL);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* the camera's step that looks at camera_place's target from 50 beyond a
 * point 20 above player 0. camera_step_fishing_point does the same, then
 * pushes the camera out of walls, eases it and sweeps it, which this step
 * leaves out */
static void fishing_camera_step_point(struct camera_view *view)
{
    /* @hack redundant: a copy of camera_work's pos into 12 bytes of the
     * frame, which nothing reads (0x8c0f53e6 to 0x8c0f53f2); without it, 107
     * of 116 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    NJS_POINT3 unused;
    NJS_POINT3 away;

    unused = camera_work->pos;
    VECTOR_COPY(&camera_state->target, &camera_place.target);
    away.x = player_work[0]->pos.x - camera_state->target.x;
    away.y = player_work[0]->pos.y - camera_state->target.y + 20.0F;
    away.z = player_work[0]->pos.z - camera_state->target.z;
    njUnitVector(&away);
    away.x *= 50.0F;
    away.y *= 50.0F;
    away.z *= 50.0F;
    camera_state->pos.x = player_work[0]->pos.x + away.x;
    camera_state->pos.y = player_work[0]->pos.y + away.y + 20.0F;
    camera_state->pos.z = player_work[0]->pos.z + away.z;
    camera_state->ang_z = 0;
}

/* set camera_place's target to the target's place on the ground, but at
 * least 30 out from player 0 in x and z */
static void fishing_target_camera(struct task *task)
{
    NJS_POINT3 away;
    struct task_work *player = player_work[0];

    away.x = task->work->pos.x - player->pos.x;
    away.y = 0.0F;
    away.z = task->work->pos.z - player->pos.z;
    if (njScalor(&away) < 30.0F) {
        njUnitVector(&away);
        away.x *= 30.0F;
        away.z *= 30.0F;
    }
    camera_place.target.x = player->pos.x + away.x;
    camera_place.target.y = TARGET(task)->ground;
    camera_place.target.z = player->pos.z + away.z;
}

/* set camera_place's pos to the lure, and move its target toward a point 10
 * from the lure. while a fish holds the lure unhooked, the point lies toward
 * that fish. otherwise it lies along a vector whose x and z run from player 0
 * to the lure, and whose y is minus the lure's height. the target moves 10 a
 * step, and onto the point once within 5. nothing while big_lure is null */
static void fishing_lure_camera(struct task *task)
{
    NJS_POINT3 point;
    Float follow;
    Float distance;
    struct camera_place *place;
    /* @hack spelling: njSubVector called through subtract, set before follow,
     * which puts its pool word ahead of 10.0F's (0x8c0f55d8, 0x8c0f55dc);
     * called by name, the two trade and 2 of 149 words differ */
    void (*subtract)(NJS_POINT3 *from, NJS_POINT3 *by);

    if (big_lure == NULL) {
        return;
    }
    place = &camera_place;
    place->pos = task->work->pos;
    subtract = njSubVector;
    follow = 10.0F;
    if (big_catch != NULL && !(fishing_flags & FISHING_HOOKED)) {
        point = big_catch->work->pos;
        subtract(&point, &task->work->pos);
        distance = follow;
    } else {
        point = player_work[0]->pos;
        subtract(&point, &task->work->pos);
        distance = -10.0F;
        point.y = task->work->pos.y;
    }
    njUnitVector(&point);
    point.x = task->work->pos.x + (point.x * distance);
    point.y = task->work->pos.y + (point.y * distance);
    point.z = task->work->pos.z + (point.z * distance);
    if (njDistanceP2P(&point, &place->target) < 5.0F) {
        place->target.x = point.x;
        place->target.y = point.y;
        place->target.z = point.z;
    } else {
        subtract(&point, &place->target);
        njUnitVector(&point);
        place->target.x += point.x * follow;
        place->target.y += point.y * follow;
        place->target.z += point.z * follow;
    }
}

void fishing_look_at_big(struct task *task)
{
    NJS_POINT3 ahead;
    NJS_POINT3 offset;
    struct task_work *player = player_work[0];

    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, player->ang.z);
    TURN(njRotateX, player->ang.x);
    TURN(njRotateY, 0x8000 - player->ang.y);
    ahead.x = 100.0F;
    ahead.y = 0.0F;
    ahead.z = 0.0F;
    njCalcVector(NULL, &ahead, &offset);
    njPopMatrix(1);
    camera_place.target.x = offset.x + player->pos.x;
    camera_place.target.y = offset.y + player->pos.y;
    camera_place.target.z = offset.z + player->pos.z;
}

/* set camera_place's target 30 out from player 0 toward the lure in x and
 * z, at his height */
static void fishing_look_at_lure(struct task *task)
{
    NJS_POINT3 away;
    struct task_work *player = player_work[0];

    away.x = task->work->pos.x - player->pos.x;
    away.y = 0.0F;
    away.z = task->work->pos.z - player->pos.z;
    njUnitVector(&away);
    away.x *= 30.0F;
    away.z *= 30.0F;
    camera_place.target.x = player->pos.x + away.x;
    camera_place.target.y = player->pos.y;
    camera_place.target.z = player->pos.z + away.z;
}

/* sweep a ball of radius 4 from *from toward *to. at a hit, *to moves back
 * to the nearest contact, *from plus the move as world_sweep_all_hit leaves
 * them. the sweep changes *from too */
static void fishing_camera_clear(NJS_POINT3 *from, NJS_POINT3 *to)
{
    NJS_POINT3 move;

    move.x = to->x - from->x;
    move.y = to->y - from->y;
    move.z = to->z - from->z;
    if (world_sweep_all_hit(from, &move, NULL, 4.0F)) {
        to->x = from->x + move.x;
        to->y = from->y + move.y;
        to->z = from->z + move.z;
    }
}

/* the camera's step while the lure is in the water. it looks at the middle
 * of camera_place's pos and target, from twice their distance apart, and
 * moves at most 20 a step. it stays no lower than the point it looks at.
 * more than 5 under the water, a sweep from where it was keeps it clear of
 * walls */
static void fishing_camera_step_lure(struct camera_view *view)
{
    NJS_POINT3 lure;
    NJS_POINT3 point;
    NJS_POINT3 from;
    NJS_POINT3 move;
    NJS_POINT3 apart;
    Float distance;

    lure = camera_place.pos;
    point = camera_place.target;
    from = camera_work->pos;
    camera_state->target.x = (lure.x + point.x) * 0.5F;
    camera_state->target.y = (lure.y + point.y) * 0.5F;
    camera_state->target.z = (lure.z + point.z) * 0.5F;
    move.x = camera_state->pos.x - camera_state->target.x;
    move.y = camera_state->pos.y - camera_state->target.y;
    move.z = camera_state->pos.z - camera_state->target.z;
    apart.x = lure.x - point.x;
    apart.y = lure.y - point.y;
    apart.z = lure.z - point.z;
    distance = njScalor(&apart) * 2.0F;
    njUnitVector(&move);
    move.x = camera_state->target.x + (move.x * distance) - camera_state->pos.x;
    move.y = camera_state->target.y + (move.y * distance) - camera_state->pos.y;
    move.z = camera_state->target.z + (move.z * distance) - camera_state->pos.z;
    if (njScalor(&move) < 20.0F) {
        camera_state->pos.x += move.x;
        camera_state->pos.y += move.y;
        camera_state->pos.z += move.z;
    } else {
        njUnitVector(&move);
        camera_state->pos.x += move.x * 20.0F;
        camera_state->pos.y += move.y * 20.0F;
        camera_state->pos.z += move.z * 20.0F;
    }
    if (camera_state->pos.y < camera_state->target.y) {
        camera_state->pos.y = camera_state->target.y;
    }
    if (fishing_water - 5.0F > camera_state->pos.y) {
        fishing_camera_clear(&from, &camera_state->pos);
    }
}

Float fishing_get_water(void)
{
    return fishing_water;
}

void fishing_hud_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case HUD_INIT:
        task->draw = fishing_hud_draw;
        work->mode = HUD_SHOW;
        break;
    case HUD_SHOW:
        fishing_hud_show(task);
        break;
    }
}

/* the hit banner's step: its halves slide together 16 a step, wait 60
 * steps, slide on past each other, and the banner ends. it shows on every
 * other step */
static void fishing_banner_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case BANNER_INIT:
        task->draw = fishing_banner_draw;
        work->mode = BANNER_IN;
        work->pos.x = 420.0F;
        break;
    case BANNER_IN:
        work->pos.x -= 16.0F;
        if (work->pos.x < 0.0F) {
            work->pos.x = 0.0F;
            work->mode = BANNER_WAIT;
            BANNER_VARS(work)->steps = 60;
        }
        fishing_banner_blink(task);
        break;
    case BANNER_WAIT:
        if (--BANNER_VARS(work)->steps == 0) {
            work->mode = BANNER_OUT;
        }
        fishing_banner_blink(task);
        break;
    case BANNER_OUT:
        work->pos.x -= 16.0F;
        if (work->pos.x < -420.0F) {
            work->mode = BANNER_DONE;
        }
        fishing_banner_blink(task);
        break;
    case BANNER_DONE:
        task_kill(task);
        break;
    }
}

void fishing_start_fight(void)
{
    task_create(TASK_WORK, 3, fishing_banner_step);
    fishing_strain = 0.0F;
    stream_request_interlude(MUSIC_FISHING);
}

/* create a splash at parent's place, facing as it does. the task, or
 * null */
static struct task *fishing_splash_create(struct task *parent)
{
    struct task *task = task_create(TASK_WORK, 2, fishing_splash_step);

    if (task != NULL) {
        task->work->pos = parent->work->pos;
        task->work->ang.y = parent->work->ang.y;
        SPLASH_VARS(task->work)->frame = 0;
        TASK_TIMER(task->work) = 5;
        SPLASH_VARS(task->work)->alpha = 1.0F;
    }
    return task;
}

static void fishing_splash_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_SPRITE sprite;
    NJS_ARGB color;

    sprite.tlist = &fishing_texlist;
    sprite.tanim = fishing_splash_texanims;
    sprite.ang = 0;
    sprite.sx = 0.1F;
    sprite.sy = 0.1F;
    VECTOR_CLEAR(&sprite.p);
    njSetTexture(&fishing_texlist);
    color.b = 1.0F;
    color.g = 1.0F;
    color.r = 1.0F;
    if (work->mode == SPLASH_FADE) {
        color.a = SPLASH_VARS(work)->alpha;
    } else {
        color.a = 1.0F;
    }
    njSetConstantMaterial(&color);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    /* @hack spelling: the sprite read through task->work, which keeps task in
     * the frame (MOV.L R4,@R15 at 0x8c0f5c16); through work, the frame
     * shrinks by 4 bytes and 67 of 76 words differ */
    njDrawSprite3D(&sprite, SPLASH_VARS(task->work)->frame,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
    njPopMatrix(1);
}

static void fishing_splash_destroy(struct task *task)
{
}

/* the splash's step: its first seven sprites show five steps each, then
 * the last fades a tenth every ten steps, and the splash ends. its fade
 * clears FISHING_SPLASH */
static void fishing_splash_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SPLASH_INIT:
        task->destructor = fishing_splash_destroy;
        task->draw = fishing_splash_draw;
        work->mode = SPLASH_PLAY;
        TASK_TIMER(work) = 5;
        break;
    case SPLASH_PLAY:
        if (SPLASH_VARS(work)->frame == 7) {
            work->mode = SPLASH_FADE;
            TASK_TIMER(work) = 10;
            if (big_lure != NULL) {
                fishing_flags &= ~FISHING_SPLASH;
            }
            break;
        }
        if (--TASK_TIMER(work) == 0) {
            TASK_TIMER(work) = 5;
            SPLASH_VARS(work)->frame++;
        }
        fishing_splash_draw(task);
        break;
    case SPLASH_FADE:
        if (SPLASH_VARS(work)->alpha <= 0.0F) {
            task_kill(task);
        } else {
            if (--TASK_TIMER(work) == 0) {
                TASK_TIMER(work) = 10;
                SPLASH_VARS(work)->alpha -= 0.1F;
            }
            fishing_splash_draw(task);
        }
        break;
    }
}

Bool fishing_task_in_reach(struct task_work *work)
{
    struct near_task *near;
    Float distance;

    if (PLAYER_VARS(work)->player == 0) {
        near = near_tasks;
    } else {
        near = near_tasks_player1;
    }
    if (near->work == NULL) {
        return FALSE;
    }
    for (distance = near->distance; near->work != NULL; near++) {
        if (distance > near->distance) {
            distance = near->distance;
        }
    }
    if (distance > 50.0F * 50.0F) {
        return FALSE;
    }
    return TRUE;
}

void fishing_reset(void)
{
    fishing_score = 0;
    biggest_fish = 0;
    biggest_fish_kind = 0;
    fishing_record_weight = fishing_load_record();
    fishing_cast_pos.x = fishing_cast_pos.y = fishing_cast_pos.z = 0.0F;
}

/* give a prize at random, by fishing_prizes' chances, and return its
 * kind */
static int fishing_give_prize(void)
{
    int i = 0;
    Float sum = 0.0F;
    Float chance = rand() * RAND_SCALE;

    do {
        i++;
        sum += fishing_prizes[i].chance;
    } while (chance > sum);
    if (fishing_prizes[i].give != NULL) {
        fishing_prizes[i].give();
    }
    return fishing_prizes[i].kind;
}

static void fishing_prize_5_rings(void)
{
    ring_add(5);
    sound_play(SE_RING, 0, 0, 0);
}

static void fishing_prize_10_rings(void)
{
    ring_add(10);
    sound_play(SE_RING, 0, 0, 0);
}

static void fishing_prize_random_rings(void)
{
    int rings;

    ring_add(rings = fishing_random_ring_counts[(
                 int)(rand() * (RANDOM_RING_COUNTS * RAND_SCALE))]);
    /* @hack redundant: a call of ring_add that never runs. its use of rings
     * keeps rings in the frame (ADD #-4,R15 at 0x8c0f5edc, MOV.L R2,@R15 at
     * 0x8c0f5efa); without it, the frame goes, the function runs 6 bytes short,
     * and 25 of 27 words differ */
    if (0) {
        ring_add(rings);
    }
    sound_play(SE_RING, 0, 0, 0);
}

static void fishing_prize_1up(void)
{
    lives_add(1);
}

Float fishing_get_distance(void)
{
    return fishing_distance;
}

/* stop a ball of radius at the walls of the pen fishing_in_pen finds, 40 out
 * from its middle each way. TRUE when it meets one, with speed cut to the wall
 * and ang set to the wall's slope. away from the pens of Ice Cap's pond, there
 * is none to meet */
static Bool fishing_pen_sweep(NJS_POINT3 *pos, NJS_VECTOR *speed,
                              struct angles *ang, Float radius)
{
    int i;
    Bool hit;
    Float min_x;
    Float max_x;
    Float min_z;
    Float max_z;

    if (stage_get_stage_act() != ICECAP_POND) {
        return FALSE;
    }
    for (i = 0; i < PENS; i++) {
        if (njDistanceP2P(&fishing_pens[i], &fishing_cast_pos) < 70.0F) {
            break;
        }
    }
    if (i == PENS) {
        return FALSE;
    }
    min_x = fishing_pens[i].x - 40.0F;
    max_x = fishing_pens[i].x + 40.0F;
    min_z = fishing_pens[i].z - 40.0F;
    max_z = fishing_pens[i].z + 40.0F;
    hit = FALSE;
    if (pos->x + speed->x - radius <= min_x) {
        hit = TRUE;
        speed->x = min_x - pos->x + radius;
        ang->z = 0xc000;
    }
    if (pos->x + speed->x + radius >= max_x) {
        hit = TRUE;
        speed->x = max_x - pos->x - radius;
        ang->z = 0x4000;
    }
    if (pos->z + speed->z - radius <= min_z) {
        hit = TRUE;
        speed->z = min_z - pos->z + radius;
        ang->x = 0x4000;
    }
    if (pos->z + speed->z + radius >= max_z) {
        hit = TRUE;
        speed->z = max_z - pos->z - radius;
        ang->x = 0xc000;
    }
    return hit;
}

/* sweep a ball of radius from pos along speed, as world_sweep_ball does, but
 * skipping WORLD_WATER objects. TRUE at once where fishing_pen_sweep meets a
 * pen's wall. otherwise the ball sweeps through the world's objects as last
 * listed. TRUE at a hit, with pos, speed and ang the nearest contact's */
static Bool fishing_sweep(NJS_POINT3 *pos, NJS_VECTOR *speed,
                          struct angles *ang, Float radius)
{
    struct polygon_ball ball;
    struct polygon_ball best;
    struct angles offset;
    struct polygon_ball *contact;
    struct world_object *objects;
    struct task_force *force;
    struct task *owner;
    Angle slope_x;
    Angle slope_z;
    Angle flipped_x;
    Angle flipped_z;
    Angle gap_x;
    Angle gap_z;
    Uint16 i;
    Bool hit;
    Float nearest;
    Float distance;

    hit = FALSE;
    if (fishing_pen_sweep(pos, speed, ang, radius)) {
        return TRUE;
    }
    objects = world_objects;
    contact = &ball;
    offset.x = 0;
    offset.y = 0;
    offset.z = 0;
    if (ang == NULL) {
        contact->normal.x = 0.0F;
        contact->normal.y = 1.0F;
        contact->normal.z = 0.0F;
    } else {
        NJS_VECTOR normal;
        NJS_VECTOR up;

        up.x = 0.0F;
        up.y = 1.0F;
        up.z = 0.0F;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, ang->z);
        TURN(njRotateX, ang->x);
        TURN(njRotateY, ang->y);
        njCalcVector(NULL, &up, &normal);
        njPopMatrix(1);
        contact->normal = normal;
    }
    contact->pos = *pos;
    contact->radius = radius;
    contact->speed = *speed;
    for (i = 0; i < world_count; i++) {
        struct angles turn;
        NJS_VECTOR moved;

        if (objects[i].flags & WORLD_WATER) {
            continue;
        }
        if (polygon_sweep(contact, objects[i].object, objects[i].flags)) {
            if (ang != NULL && contact->hit.normal.y < 0.0F) {
                slope_x =
                    (Angle)(asinf(contact->hit.normal.z) * ANGLE_PER_RADIAN);
                slope_z =
                    -(Angle)(asinf(contact->hit.normal.x) * ANGLE_PER_RADIAN);
                flipped_x = (Uint16)(0x8000 - slope_x);
                flipped_z = (Uint16)(0x8000 - slope_z);
                gap_x = angle_distance(ang->x, flipped_x);
                gap_z = angle_distance(ang->z, flipped_z);
                if (gap_x < gap_z) {
                    contact->hit.ang_x = flipped_x;
                    contact->hit.ang_z = slope_z;
                } else {
                    contact->hit.ang_x = slope_x;
                    contact->hit.ang_z = flipped_z;
                }
            }
            owner = objects[i].owner;
            if (owner != NULL && owner->force != NULL) {
                force = owner->force;
                VECTOR_ADD(&contact->hit.pos, &force->speed);
                turn = force->turn;
            } else {
                turn.x = 0;
                turn.y = 0;
                turn.z = 0;
            }
            moved.x = contact->hit.pos.x - pos->x;
            moved.y = contact->hit.pos.y - pos->y;
            moved.z = contact->hit.pos.z - pos->z;
            if (!hit) {
                best = ball;
                nearest = njScalor(&moved);
                offset = turn;
            } else {
                distance = njScalor(&moved);
                if (nearest > distance) {
                    best = ball;
                    nearest = distance;
                    offset = turn;
                }
            }
            hit = TRUE;
        }
    }
    if (hit == TRUE) {
        contact = &best;
        *pos = contact->hit.pos;
        *speed = contact->hit.speed;
        if (ang != NULL) {
            ang->x = contact->hit.ang_x + offset.x;
            ang->y += offset.y;
            ang->z = contact->hit.ang_z + offset.z;
        }
    }
    return hit;
}

/* add a catch's weight to fishing_score, to 99990 at most. biggest_fish and
 * biggest_fish_kind take the catch if it's the heaviest */
static void fishing_score_catch(Float weight, int kind)
{
    fishing_score += weight;
    if (fishing_score > 99990) {
        fishing_score = 99990;
    }
    if (weight > biggest_fish) {
        biggest_fish = weight;
        biggest_fish_kind = kind;
    }
}

/* draw the catch's weight and its unit, on each frame's first step */
static void fishing_catch_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_SPRITE sprite;
    struct hud_number number;

    if (frame_step == 0) {
#ifdef GCC
        /* 16:9 squeezes the weight about a pivot that travels with it from
         * the screen's right edge to the left edge, the score's pivot. its
         * scale falls from 2 to 1 on the way, and a lost fish's stays 2 as
         * it leaves on the right */
        port_wide_anchor =
            WIDE_RIGHT + ((2.0F - work->scl.x) * (WIDE_LEFT - WIDE_RIGHT));
#endif
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
        sprite.tlist = &fishing_texlist;
        sprite.tanim = fishing_hud_texanims;
        sprite.sx = work->scl.x;
        sprite.sy = work->scl.x;
        sprite.p.x = work->pos.x;
        sprite.p.y = work->pos.z;
        njDrawSprite2D(&sprite, SPRITE_UNIT, HUD_DEPTH - 0.018F,
                       NJD_SPRITE_ALPHA);
        number.flags = NUMBER_SCALED | NUMBER_CLAMPED;
        number.place = NUMBER_LEFT | NUMBER_TOP | NUMBER_DEPTH;
        number.unk_02 = 0;
        number.max = 9999;
        /* @hack spelling: the weight read through task->work, which keeps task
         * in the frame (MOV.L R4,@R15 at 0x8c0f64ee); through work, the
         * frame shrinks by 4 bytes and 110 of 116 words differ */
        number.value = CATCH_VARS(task->work)->weight;
        number.pos.x = work->pos.x - (work->scl.x * 64.0F) - 16.0F;
        number.pos.y = work->pos.z - 16.0F;
        number.pos.z = 0.0F;
        number.scale = work->scl.x;
        number.color = 0xffc0c0c0;
        hud_draw_number(&number);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_BOTHSRCALPHA);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
#ifdef GCC
        port_wide_anchor = WIDE_MIDDLE;
#endif
    }
}

/* score the catch as its task ends, with SE_RING, but not a fish the line
 * lost. the frog's catch saves the score too */
static void fishing_catch_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (CATCH_VARS(work)->kind != CATCH_SNAPPED) {
        fishing_score_catch(CATCH_VARS(work)->weight, CATCH_VARS(work)->kind);
        if (CATCH_VARS(work)->kind == FISH_FROG) {
            fishing_save();
        }
        sound_play(SE_RING, 0, 0, 0);
    }
}

/* the catch task's step. the weight shows at twice its size for 60 steps.
 * over 60 more, it shrinks as it slides left, or for a fish the line lost,
 * slides right. at the end, the frog's catch at Twinkle Park and Ice Cap
 * plays VOICE_FROG */
static void fishing_catch_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case CATCH_INIT:
        task->draw = fishing_catch_draw;
        task->destructor = fishing_catch_destroy;
        work->mode = CATCH_SHOW;
        CATCH_VARS(work)->steps = 120;
        work->pos.x = 604.0F;
        work->pos.z = 40.0F;
        work->scl.x = 2.0F;
        break;
    case CATCH_SHOW:
        if (--CATCH_VARS(work)->steps == 60) {
            work->mode = CATCH_LEAVE;
        }
        fishing_catch_draw(task);
        break;
    case CATCH_LEAVE:
        if (--CATCH_VARS(work)->steps <= 0) {
            if (CATCH_VARS(work)->kind == FISH_FROG) {
                if (IN_STAGE(STAGE_TWINKLE) || IN_STAGE(STAGE_ICECAP)) {
                    stream_request_voice(VOICE_FROG);
                }
            }
            task_kill(task);
        }
        if (CATCH_VARS(work)->kind != CATCH_SNAPPED) {
            work->pos.x += -7.0F;
            work->pos.z += 7.0F / 60.0F;
            work->scl.x = 1.0F + (CATCH_VARS(work)->steps * (1.0F / 60.0F));
        } else {
            work->pos.x += 5.0F;
        }
        fishing_catch_draw(task);
        break;
    }
}

void fishing_show_catch(int weight, int kind)
{
    struct task *task = task_create(TASK_WORK, 3, fishing_catch_step);

    if (task != NULL) {
        CATCH_VARS(task->work)->weight = weight;
        CATCH_VARS(task->work)->kind = kind;
    }
}

/* Big's stage as the save numbers it: 0 at Emerald Coast, 1 at Twinkle Park and
 * 2 at Ice Cap. any other stage, as Hot Shelter, is 3 */
static int fishing_save_stage(void)
{
    if (IN_STAGE(STAGE_BEACH)) {
        return 0;
    } else if (IN_STAGE(STAGE_TWINKLE)) {
        return 1;
    } else if (IN_STAGE(STAGE_ICECAP)) {
        return 2;
    } else {
        return 3;
    }
    /* @hack redundant: a return no path reaches, whose MOV #0,R0 the
     * beach's test branches to for its own 0 (BT at 0x8c0f6822 to
     * 0x8c0f6880); without it, that test returns in place and 21 of 37
     * words differ */
    return 0;
}

/* put score among the stage's three best in the save, in order, if it
 * beats the third */
static void fishing_save_score(struct save *save, int stage, Sint16 score)
{
    if (save->fishing_scores[stage][0] < score) {
        save->fishing_scores[stage][2] = save->fishing_scores[stage][1];
        save->fishing_scores[stage][1] = save->fishing_scores[stage][0];
        save->fishing_scores[stage][0] = score;
    } else if (save->fishing_scores[stage][1] < score) {
        save->fishing_scores[stage][2] = save->fishing_scores[stage][1];
        save->fishing_scores[stage][1] = score;
    } else if (save->fishing_scores[stage][2] < score) {
        save->fishing_scores[stage][2] = score;
    }
}

/* keep a fish's weight and kind as the stage's biggest in the save, if it
 * is heavier */
static void fishing_save_fish(struct save *save, int stage, Sint16 weight,
                              int kind)
{
    if (save->biggest_fish[stage].weight < weight) {
        save->biggest_fish[stage].weight = weight;
        save->biggest_fish[stage].kind = kind;
    }
}

/* put the score, in tens, and the heaviest catch into the save slot's
 * records for the stage */
static void fishing_save(void)
{
    struct save *save;
    int stage;

    fishing_save_copy = saves[save_slot];
    save = &fishing_save_copy;
    stage = fishing_save_stage();
    if (stage < 0) {
        return;
    }
    fishing_save_score(save, stage, fishing_score / 10);
    fishing_save_fish(save, stage, biggest_fish, biggest_fish_kind);
    saves[save_slot] = fishing_save_copy;
}

/* the weight of the stage's biggest fish in the save slot, which it stores
 * in fishing_record_weight too */
static Float fishing_load_record(void)
{
    struct save *save = &saves[save_slot];
    int stage = fishing_save_stage();

    return fishing_record_weight = save->biggest_fish[stage].weight;
}
