/* MISSILE standing rocket with a scrolling texture, pulsing glows and steam */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "object/placement.h"
#include "stg04/missile.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT highway_missile_object;
/* the model's meshsets, of which SCROLL_MESHSET's UVs scroll */
#define SCROLL_MESHSET 2
extern NJS_MESHSET highway_missile_meshsets[];
/* OBJ_HIGHWAY2's texture names, among them the corner light's glow, which
 * the missile's glows take too */
extern NJS_TEXNAME obj_highway2_textures[54];

/* destroyed beyond 3000 units of the camera and player 0 */
#define MISSILE_RANGE (3000.0F * 3000.0F)

#define SCROLL_UVS 40

#define GLOWS 4
/* the glows' phase advances a 128th of a turn each step */
#define GLOW_STEP 0x200

/* the steps the steam's timer waits before each puff, so the missile puffs
 * every sixth step */
#define STEAM_WAIT 5
#define STEAM_SCALE 8.0F

enum missile_mode { MISSILE_INIT = 0, MISSILE_STAND = 1 };

struct missile_vars {
    Float glow; /* the glows' alpha */
    Angle phase;
    /* speed is how far the texture scrolls along v each step, in UV units.
     * highway_missile_init zeroes all of word, then sets speed from scale x */
    union {
        Sint32 word;
        Sint16 speed;
    } scroll;
};

#define VARS(work) ((struct missile_vars *)&(work)->vars)

static void highway_missile_destroy(struct task *task);
static void highway_missile_draw(struct task *task);
static void highway_missile_run(struct task *task);
static void highway_missile_init(struct task *task);
static void highway_missile_scroll(struct task_work *work);
static void highway_missile_draw_glows(struct task_work *work);
static void highway_missile_puff(struct task_work *work);

static NJS_TEXLIST highway_missile_texlist = {
#include "stg04/missile/highway_missile_texlist.inc"
};
static NJS_TEXANIM highway_missile_texanim[1] = {
#include "stg04/missile/highway_missile_texanim.inc"
};

/* where the glows stand around the missile's foot, in the missile's own
 * coordinates */
static NJS_POINT3 highway_missile_glows[GLOWS] = {
    {0.0F, -447.5F, -150.0F},
    {150.0F, -447.5F, 0.0F},
    {0.0F, -447.5F, 150.0F},
    {-150.0F, -447.5F, 0.0F},
};

/* where the steam starts, and a point below it that gives its heading, in the
 * missile's own coordinates */
static NJS_POINT3 highway_missile_steam_from = {0.0F, -350.0F, 0.0F};
static NJS_POINT3 highway_missile_steam_to = {0.0F, -370.0F, 0.0F};

/* white steam that grows, slows and rises. highway_missile_puff sets its
 * scale to STEAM_SCALE before each puff, so no puff takes the 2.5 here */
static struct dust_params highway_missile_steam = {
    2.5F,
    0.058F,
    0.15F,
    0.17F,
    0.455F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 1.0F, 1.0F, 1.0F},
};

static void highway_missile_destroy(struct task *task)
{
}

static void highway_missile_draw(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_tree(&highway_missile_object);
    njPopMatrix(1);
}

static void highway_missile_run(struct task *task)
{
    /* @hack spelling: task->work read for each call, which SHC loads again into
     * R4 in each call's slot (MOV.L @(32,R14),R4 at 0x0c91180c, 0x0c911810
     * and 0x0c911814). through a local work, 9 of highway_missile_run's 13
     * words differ */
    highway_missile_scroll(task->work);
    highway_missile_draw_glows(task->work);
    highway_missile_puff(task->work);
    highway_missile_draw(task);
}

static void highway_missile_init(struct task *task)
{
    /* @hack spelling: each store through task->work, which SHC loads again for
     * each, five times from 0x0c91181e to 0x0c911838 (MOV.L @(32,R4),Rn).
     * through a local work, 22 of highway_missile_init's 22 words differ */
    task->work->mode = MISSILE_STAND;
    VARS(task->work)->scroll.word = 0;
    VARS(task->work)->phase = 0;
    task->work->timer = STEAM_WAIT;
    VARS(task->work)->scroll.speed = (int)task->work->scl.x;
    task->draw = highway_missile_draw;
    task->destructor = highway_missile_destroy;
}

/* each step draws the missile twice, before and after its glows. while the
 * set editor holds the missile, it runs at any range. each step then takes the
 * scroll speed from scale x again */
void highway_missile_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        switch (work->mode) {
        case MISSILE_INIT:
            highway_missile_init(task);
            break;
        case MISSILE_STAND:
            if (!placement_try_leave_by(task, MISSILE_RANGE)) {
                highway_missile_draw(task);
                highway_missile_run(task);
            }
            break;
        }
    } else {
        switch (work->mode) {
        case MISSILE_INIT:
            highway_missile_init(task);
            break;
        case MISSILE_STAND:
            VARS(work)->scroll.speed = (int)work->scl.x;
            highway_missile_draw(task);
            highway_missile_run(task);
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> UV SCROLL SPEED");
            break;
        }
    }
}

static void highway_missile_scroll(struct task_work *work)
{
    /* @hack spelling: the mesh set reached through meshset, so the pool holds
     * the mesh set's own address, and the load reads vertuv 20 bytes past it
     * (MOV.L @(20,R5),R5 at 0x0c911914). through
     * highway_missile_meshsets[SCROLL_MESHSET].vertuv, the pool holds
     * vertuv's address, and 2 of highway_missile_scroll's 20 words differ */
    NJS_MESHSET *meshset = &highway_missile_meshsets[SCROLL_MESHSET];
    NJS_COLOR *uv = meshset->vertuv;
    int i;

    for (i = 0; i < SCROLL_UVS; i++) {
        uv->tex.v += VARS(work)->scroll.speed;
        uv++;
    }
}

/* advance the glows' phase, and draw the four glows facing the camera,
 * unfogged. each glow adds its color to what lies behind it, at an alpha of
 * the phase's sine */
static void highway_missile_draw_glows(struct task_work *work)
{
    NJS_SPRITE sprite;
    NJS_POINT3 pos;
    int i;

    sprite.tlist = &highway_missile_texlist;
    sprite.tanim = highway_missile_texanim;
    VARS(work)->phase += GLOW_STEP;
    VARS(work)->glow = njSin(VARS(work)->phase);
    sprite.sx = sprite.sy = 1.0F;
    sprite.p.x = 0.0F;
    sprite.p.y = 0.0F;
    sprite.p.z = 0.0F;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetTexture(&highway_missile_texlist);
    draw_set_material(VARS(work)->glow, 1.0F, 1.0F, 1.0F);
    for (i = 0; i < GLOWS; i++) {
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        njScale(NULL, 1.0F, 1.0F, 1.0F);
        njCalcPoint(NULL, &highway_missile_glows[i], &pos);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslateV(NULL, &pos);
        TURN(njRotateY, camera_work->ang.y);
        TURN(njRotateX, camera_work->ang.x);
        njFogDisable();
        njDrawSprite3D(&sprite, 0, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        njFogEnable();
        njPopMatrix(1);
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* every sixth step, puff steam from the missile's foot, heading down the
 * missile's own y axis */
static void highway_missile_puff(struct task_work *work)
{
    NJS_POINT3 pos;
    NJS_VECTOR speed;

    if (work->timer) {
        work->timer--;
        return;
    }
    work->timer = STEAM_WAIT;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njCalcPoint(NULL, &highway_missile_steam_from, &pos);
    njCalcPoint(NULL, &highway_missile_steam_to, &speed);
    njPopMatrix(1);
    highway_missile_steam.pos.x = pos.x;
    highway_missile_steam.pos.y = pos.y;
    highway_missile_steam.pos.z = pos.z;
    speed.x -= pos.x;
    speed.y -= pos.y;
    speed.z -= pos.z;
    njUnitVector(&speed);
    highway_missile_steam.speed.x = speed.x;
    highway_missile_steam.speed.y = speed.y;
    highway_missile_steam.speed.z = speed.z;
    highway_missile_steam.scale = STEAM_SCALE;
    dust_create_from(&highway_missile_steam);
}
