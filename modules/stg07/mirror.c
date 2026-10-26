/* act 1's mirror room. player 0 aims a light from a mirror stand,
 * O RMIRROR. the aim ends once it finds a big mirror, O BIGMR. the find
 * lights the objects whose AIM_ID is the MY_ID of the stand aimed from:
 * - a snake statue, O_MRSNA, catches fire;
 * - a pillar, O HOGHASIRA, shows through the fog until an aim from another
 *   stand finds a big mirror;
 * - a stand takes the light, and the room's fog reaches from player 0 as far
 *   as that stand.
 * a fog switch, O FOG_SW, holds the fog close or lets it go. a glow,
 * O FOG_LI, shows while its AIM_ID names the stand last aimed from */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/glare.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg07/fire.h"
#include "stg07/mirror.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: int, not stage.h's Uint16, so SHC compares the result without
 * extending it, as at 0x0c9093b6. as stage.h declares it, 51 of 143 words
 * differ */
extern int stage_get_stage_act(void);

extern NJS_OBJECT mirror_stand_object;
extern NJS_MODEL mirror_glass_model;
extern NJS_OBJECT big_mirror_object;
extern NJS_MODEL snake_statue_model;
extern NJS_MODEL ruin_light_glow_model;
extern NJS_MODEL ruin_pillar_2_model;
extern NJS_FOG_TABLE ruin_fog;

/* ids in the set entry's scale. MY_ID is a stand's own id, and only stands
 * have one. AIM_ID is the MY_ID of the stand whose aim reaches the object */
#define MY_ID(work) ((work)->scl.x)
#define AIM_ID(work) ((work)->scl.y)
/* a fog switch's scale, which sizes its sphere */
#define FOG_SWITCH_SCALE(work) ((work)->scl.y)

struct mirror_vars {
    Uint8 unk_00[4];
    Sint16 puff; /* steps since its last puff in MIRROR_SPARKLE */
    /* 1 from the step after the stand took mirror_lit_pos, and stays 1 */
    Sint16 lit;
};

struct big_mirror_vars {
    Float glow; /* grows by 0.5 each step it shines. no code reads it */
};

struct fog_vars {
    Uint8 unk_00[2];
    /* stage_get_stage_act's result as the task started. the task ends once
     * the stage or act changes */
    Uint16 stage_act;
    /* nothing sets it but to 0. away from the room, the step leaves the fog
     * as it is while unk_04 or restored is nonzero */
    Uint8 unk_04;
    /* 1 once the step has made the fog white away from the room. it is 0 while
     * player 0 is in the room */
    Uint8 restored;
};

#define MIRROR(work) ((struct mirror_vars *)&(work)->vars)
#define BIG_MIRROR(work) ((struct big_mirror_vars *)&(work)->vars)
#define FOG(work) ((struct fog_vars *)&(work)->vars)

/* a stand's work->mode. no code sets MIRROR_SPARKLE, which puffs dust every
 * PUFF_STEPS + 1 steps */
enum mirror_mode {
    MIRROR_START,
    MIRROR_WAIT,
    MIRROR_AIM,
    MIRROR_SPARKLE,
    MIRROR_STILL
};

/* a stand's work->submode once an aim from it has found a big mirror. no
 * player can aim from it again */
#define MIRROR_USED 1
/* a big mirror's work->submode once an aim has found it */
#define BIG_MIRROR_FOUND 1

/* the sound the TAIHO camera plays while its stick turns it. the stand stops
 * it as the aim ends */
#define SE_TAIHO_TURN 197

/* a stand stays placed within 5000 of the camera or player 0, and a big
 * mirror within 1000. placement_try_leave_by takes these squared */
#define MIRROR_RANGE 25000000.0F
#define BIG_MIRROR_RANGE 1000000.0F

/* the TAIHO camera's place sits AIM_CAMERA_HEIGHT above the stand */
#define AIM_CAMERA_HEIGHT 10.0F
/* an aim holds player 0 AIM_STAND_OFF from the stand */
#define AIM_STAND_OFF 10.0F
/* a big mirror's mark lies MARK_HEIGHT above it. the aim finds the mirror
 * once its mark projects within AIM_REACH of the screen's point AIM_X,
 * AIM_Y */
#define AIM_X 340.0F
#define AIM_Y 240.0F
#define AIM_REACH 40.0F
#define MARK_HEIGHT 12.0F

/* how far above a snake statue its fire starts, and how far ahead along its
 * y angle */
#define FIRE_HEIGHT 10.0F
#define FIRE_AHEAD 2.0F

/* the steps between a stand's puffs in MIRROR_SPARKLE, and a puff's scale */
#define PUFF_STEPS 20
#define PUFF_SCALE 2.0F

/* a found big mirror's glare: how far above the mirror it shines from, and
 * its size */
#define GLARE_HEIGHT 20.0F
#define GLARE_SIZE 11.0F

/* player 0 is in the mirror room while within ROOM_REACH of the room's
 * middle */
#define ROOM_REACH 820.0F

/* the fog's depths are z values, so a depth's distance is its negation. the
 * fog starts at FOG_NEAR, and away from the room, its far depth is
 * FOG_FAR_AWAY */
#define FOG_NEAR (-10.0F)
#define FOG_FAR_AWAY (-1600.0F)
/* the black fog's far depth steps FOG_STEP toward its target each step, but
 * comes no nearer than FOG_NEAREST */
#define FOG_NEAREST (-108.0F)
#define FOG_STEP 10.0F
/* the target while the fog is held. it lies nearer than FOG_NEAREST, so a
 * held fog draws in to FOG_NEAREST */
#define FOG_HELD (-70.0F)
/* fog_far's first value. it lies nearer than FOG_NEAREST, and the fog task's
 * first step moves it FOG_STEP farther */
#define FOG_FIRST_FAR (-100.0F)

/* a fog switch's sphere has radius (scale + 1) * SWITCH_REACH */
#define SWITCH_REACH 14.0F

/* mirror_lit_pos before any stand takes it */
#define FIRST_LIT_X 62.0F
#define FIRST_LIT_Y 127.0F
#define FIRST_LIT_Z (-56.0F)

/* a stand sets it to 59 as it ends an aim that found a big mirror.
 * ruin_act1_fog_destroy sets it to 0. no code reads it */
static int dat_0ca58ae0 = 0;
/* clang-format off */
static struct collision_shape mirror_shape = {
    SHAPE_SPHERE, 0, PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 6.0F, 0.0F,
    10.0F, {0}, 0, NULL,
};
/* clang-format on */
/* 1 while a player aims from a stand */
static Uint8 mirror_aiming = 0;
/* 1 from an aim's find of a big mirror until the stand aimed from ends the
 * aim */
static Uint8 mirror_found = 0;
/* the MY_ID of the stand a player aims, or last aimed, from. 0 before the
 * first aim */
static Uint8 mirror_aim_id = 0;
/* the MY_ID of the stand whose aim last found a big mirror, or 0 before the
 * first find. the light reaches the objects whose AIM_ID it is */
static Uint8 mirror_lit_id = 0;
/* the place of the stand that last took it. unless the fog holds, the
 * room's fog reaches from player 0 as far as this place */
static NJS_POINT3 mirror_lit_pos = {FIRST_LIT_X, FIRST_LIT_Y, FIRST_LIT_Z};
/* the room's fog is held while fog_held is 1 or a player aims. a held fog's far
 * depth draws in to FOG_NEAREST. fog_held starts at 1, and a find of a big
 * mirror clears it. it flips each time player 0 steps off a fog switch */
static Uint8 fog_held = 1;
/* the room's fog's far depth */
static Float fog_far = FOG_FIRST_FAR;
/* no word in the image or in 1ST_READ.BIN points at it */
static Float dat_0ca58b30 = -1800.0F;
/* clang-format off */
static struct collision_shape fog_switch_shape = {
    SHAPE_SPHERE, 0, PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 10.0F, 0.0F,
    0.0F, {0}, 0, NULL,
};
static struct collision_shape hoghasira_shape = {
    SHAPE_RECTANGLE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, -49.0F, 0.0F},
    17.0F, 60.0F, 17.0F, {0}, 0, NULL,
};
/* clang-format on */
/* the fog's color. 1ST_READ.BIN's frame_vsync passes it to njSetFogColor
 * each frame of act 1 */
static Uint32 ruin_fog_color;
/* no word in the image or in 1ST_READ.BIN points at these */
static Uint8 dat_0ca6722c[12];
/* where the stand a player aims from stands. mirror_step writes it, and no
 * code reads it */
static NJS_POINT3 mirror_aim_pos;

static void mirror_aim_at(struct task *task);
static void mirror_draw(struct task *task);
static void mirror_destroy(struct task *task);
static void mirror_snake_draw(struct task *task);
static void mirror_snake_destroy(struct task *task);
static void big_mirror_destroy(struct task *task);
static void big_mirror_draw(struct task *task);
static void fog_approach(Float target, Float step, struct task *task);
static void ruin_act1_fog_destroy(struct task *task);
static void fog_light_draw(struct task *task);
static void hoghasira_draw(struct task *task);

/* a mirror stand. once player 0 touches it, he aims from it through the TAIHO
 * camera, held still beside it. the stand's y angle follows the camera's, half
 * a turn round. the aim ends on the stand's step after the aim finds a big
 * mirror. no player can aim from the stand again. during any aim, the stand
 * whose AIM_ID is mirror_lit_id takes mirror_lit_pos each step. that stand is
 * lit from its next step on. a lit stand puffs dust while a player aims from
 * the stand its AIM_ID names */
void mirror_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *player;

    if (placement_try_leave_by(task, MIRROR_RANGE)) {
        return;
    }
    switch (work->mode) {
    case MIRROR_START:
        /* @hack redundant: 1s selected by a constant condition, which take R3
         * of their own (0x0c908af6). as 1, they share R11, which the store of
         * MIRROR_WAIT takes, and 150 of 314 words differ */
        /* NOLINTBEGIN(misc-redundant-expression) */
        collision_init(task, &mirror_shape, COLLISION_OBJECT, 0.0F ? 1 : 1,
                       0.0F ? 1 : 1);
        /* NOLINTEND(misc-redundant-expression) */
        task->destructor = mirror_destroy;
        task->draw = mirror_draw;
        MIRROR(work)->puff = 0;
        MIRROR(work)->lit = 0;
        work->mode = MIRROR_WAIT;
        break;
    case MIRROR_WAIT:
        if (work->tick == 2) {
            MIRROR(work)->lit = 1;
        }
        if (mirror_aiming == 1) {
            if (mirror_lit_id == AIM_ID(work)) {
                work->tick = 2;
                mirror_lit_pos.x = work->pos.x;
                mirror_lit_pos.y = work->pos.y;
                mirror_lit_pos.z = work->pos.z;
            }
            if (mirror_aim_id == AIM_ID(work) && MIRROR(work)->lit == 1) {
                NJS_VECTOR still = {0.0F, 0.0F, 0.0F};

                dust_create(&work->pos, &still, PUFF_SCALE);
            }
        }
        mirror_draw(task);
        if (work->collision->flags & COLLISION_HIT) {
            work->collision->flags &= ~COLLISION_HIT;
            if (work->submode == MIRROR_USED) {
                break;
            }
            player = player_get_task(0);
            if (work->collision->hit->owner != player) {
                break;
            }
            work->mode = MIRROR_AIM;
            camera_place.pos.x = work->pos.x;
            camera_place.pos.y = work->pos.y + AIM_CAMERA_HEIGHT;
            camera_place.pos.z = work->pos.z;
            camera_place.ang.x = 0;
            camera_place.ang.y = work->ang.y;
            camera_place.ang.z = 0;
            player_set_mode(0, PLAYER_MODE_STILL);
            camera_override_mode(CAMERA_MODE_TAIHO, 0);
            player_clear_speed(player_motion[0], player_get(0));
        } else {
            collision_register(work);
        }
        break;
    case MIRROR_AIM:
        mirror_aim_pos.x = work->pos.x;
        mirror_aim_pos.y = work->pos.y;
        mirror_aim_pos.z = work->pos.z;
        mirror_aiming = 1;
        mirror_draw(task);
        mirror_aim_id = (Uint8)MY_ID(work);
        player_set_position(
            0, work->pos.x + (njCos(work->ang.y) * AIM_STAND_OFF), work->pos.y,
            work->pos.z - (njSin(work->ang.y) * AIM_STAND_OFF));
        work->ang.y = camera_work->ang.y + NJM_DEG_ANG(180);
        if (mirror_found == 1) {
            work->submode = MIRROR_USED;
            dat_0ca58ae0 = 59;
            mirror_aiming = 0;
            work->mode = MIRROR_WAIT;
            mirror_found = 0;
            camera_restore();
            sound_stop(SE_TAIHO_TURN);
            player_set_mode(0, PLAYER_MODE_RESET);
        }
        break;
    case MIRROR_SPARKLE: {
        NJS_VECTOR still = {0.0F, 0.0F, 0.0F};

        ++MIRROR(work)->puff;
        if (MIRROR(work)->puff > PUFF_STEPS) {
            MIRROR(work)->puff = 0;
            dust_create(&work->pos, &still, PUFF_SCALE);
        }
    }
        mirror_draw(task);
        break;
    case MIRROR_STILL:
        break;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<-MY ID");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<-AIM ID");
    }
}

/* find the big mirror once its mark projects within AIM_REACH of AIM_X,
 * AIM_Y. the find sets mirror_lit_id and mirror_found, clears fog_held, and
 * sets the mirror's submode to BIG_MIRROR_FOUND */
static void mirror_aim_at(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT2 screen;
    NJS_POINT3 mark;
    Float off;

    mark = work->pos;
    mark.y += MARK_HEIGHT;
    njPushMatrix(NULL);
    njProjectScreen(NULL, &mark, &screen);
    njPopMatrix(1);
    off = ((AIM_X - screen.x) * (AIM_X - screen.x)) +
          ((AIM_Y - screen.y) * (AIM_Y - screen.y));
    off = njSqrt(off);
    if (off < AIM_REACH) {
        mirror_lit_id = (Uint8)AIM_ID(work);
        fog_held = 0;
        work->submode = BIG_MIRROR_FOUND;
        mirror_found = 1;
    }
}

/* draw the stand and its glass. while a player aims from any stand, the
 * glass turns by the camera's z and x angles, negated */
static void mirror_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_tree(&mirror_stand_object);
    if (mirror_aiming == 1) {
        TURN_VIA(njRotateZ, -camera_work->ang.z, ang);
        TURN_VIA(njRotateX, -camera_work->ang.x, ang);
    }
    draw_model(&mirror_glass_model);
    njPopMatrix(1);
}

static void mirror_destroy(struct task *task)
{
}

enum mirror_snake_mode {
    SNAKE_START,
    SNAKE_WAIT,
    SNAKE_UNUSED,
    SNAKE_BURN,
    SNAKE_STILL
};

/* a snake statue. once mirror_lit_id is its AIM_ID, it starts a fire above
 * it, once. the set editor holds it unlit */
void mirror_snake_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SNAKE_START:
        collision_init(task, &mirror_shape, COLLISION_OBJECT, 1, 1);
        task->destructor = mirror_snake_destroy;
        task->draw = mirror_snake_draw;
        work->mode = work->tick = 1;
        break;
    case SNAKE_WAIT:
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<-AIM ID");
        }
        mirror_snake_draw(task);
        if (!placement_editing(task)) {
            if (mirror_lit_id == AIM_ID(work)) {
                work->mode = SNAKE_BURN;
                /* @hack redundant: a 1 selected by a constant condition, which
                 * SHC loads where it stores it (0x0c909002). as 1, SHC keeps
                 * one 1 in R12 through the whole step, and 53 of 118 words
                 * differ */
                /* NOLINTNEXTLINE(*-branch-clone,*-redundant-expression) */
                work->tick = 0.0F ? 1 : 1;
                task = task_create(TASK_WORK, 2, ruin_fire_step);
                task->work->pos.x =
                    work->pos.x + (njSin(work->ang.y) * FIRE_AHEAD);
                task->work->pos.y = work->pos.y + FIRE_HEIGHT;
                task->work->pos.z =
                    work->pos.z + (njCos(work->ang.y) * FIRE_AHEAD);
            }
        }
        break;
    case SNAKE_BURN:
        mirror_snake_draw(task);
        break;
    case SNAKE_STILL:
        break;
    }
}

static void mirror_snake_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&snake_statue_model);
    njPopMatrix(1);
}

static void mirror_snake_destroy(struct task *task)
{
}

enum big_mirror_mode {
    BIG_MIRROR_START = 0,
    BIG_MIRROR_RUN = 1,
    BIG_MIRROR_STILL = 4
};

/* a big mirror. while a player aims from the stand its AIM_ID names, the aim
 * can find it. once found, it draws a glare GLARE_HEIGHT above it each step.
 * the glare points a quarter turn round from the mirror's y angle, and a
 * little down */
void big_mirror_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color = {-1.0F, 1.0F, 0.0F, 0.0F};
    NJS_VECTOR dir;
    NJS_POINT3 pos;
    Angle ang;

    if (placement_try_leave_by(task, BIG_MIRROR_RANGE)) {
        return;
    }
    switch (work->mode) {
    case BIG_MIRROR_START:
        task->destructor = big_mirror_destroy;
        task->draw = big_mirror_draw;
        work->mode = BIG_MIRROR_RUN;
        break;
    case BIG_MIRROR_RUN:
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<-AIM ID");
        }
        if (mirror_aiming == 1 && mirror_aim_id == AIM_ID(work)) {
            mirror_aim_at(task);
        }
        if (work->submode == BIG_MIRROR_FOUND) {
            dir.x = 0.0F;
            dir.y = -0.1F;
            dir.z = 1.0F;
            BIG_MIRROR(work)->glow += 0.5F;
            njPushMatrix(&_unit_matrix_);
            TURN_VIA(njRotateY, work->ang.y + NJM_DEG_ANG(90), ang);
            njCalcPoint(NULL, &dir, &dir);
            njPopMatrix(1);
            njUnitVector(&dir);
            pos.x = work->pos.x;
            pos.y = work->pos.y + GLARE_HEIGHT;
            pos.z = work->pos.z;
            color.r = 1.0F;
            color.g = 1.0F;
            color.b = 1.0F;
            glare_draw(&pos, &dir, GLARE_SIZE, &color);
        }
        big_mirror_draw(task);
        break;
    case BIG_MIRROR_STILL:
        break;
    }
}

static void big_mirror_destroy(struct task *task)
{
}

/* draw the mirror with Ninja's fog off, then turn the fog on */
static void big_mirror_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njFogDisable();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&big_mirror_object);
    njPopMatrix(1);
    njFogEnable();
}

enum fog_mode { FOG_START, FOG_RUN, FOG_STILL };

void ruin_act1_fog_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];

    switch (work->mode) {
    case FOG_START:
        work->mode = FOG_RUN;
        fog_approach(FOG_NEAREST, FOG_STEP, task);
        task->destructor = ruin_act1_fog_destroy;
        FOG(work)->unk_04 = 0;
        FOG(work)->stage_act = stage_get_stage_act();
        work->timer = 1;
        break;
    case FOG_RUN:
        if (FOG(work)->stage_act != stage_get_stage_act()) {
            task_kill(task);
        }
        {
            NJS_POINT3 room = {206.0F, 116.0F, 621.0F};
            Float lit_distance;

            if (player == NULL) {
                break;
            }
            if (njDistanceP2P(&room, &player->pos) > ROOM_REACH) {
                /* @hack spelling: the goto past the fog's reset, which puts
                 * ruin_fog_color's address in R3 and 0xffffffff in R2 for the
                 * store at 0x0c909402. as an if round the reset, the two
                 * swap, and 5 of 143 words differ */
                if (FOG(work)->unk_04 || FOG(work)->restored) {
                    goto held;
                }
                ruin_fog_color = 0xffffffff;
                njGenerateFogTable3(ruin_fog, FOG_NEAR, FOG_FAR_AWAY);
                FOG(work)->restored = 1;
            held:
                work->timer = 1;
                work->tick = 0;
            } else {
                FOG(work)->restored = 0;
                lit_distance = njDistanceP2P(&player->pos, &mirror_lit_pos);
                if (fog_held == 0) {
                    if (mirror_aiming == 0) {
                        fog_approach(-lit_distance, FOG_STEP, task);
                    } else {
                        fog_approach(FOG_HELD, FOG_STEP, task);
                    }
                } else {
                    fog_approach(FOG_HELD, FOG_STEP, task);
                }
            }
        }
        break;
    case FOG_STILL:
        break;
    }
}

/* move fog_far by step toward the depth target, but no nearer than
 * FOG_NEAREST. then remake ruin_fog black, from FOG_NEAR to fog_far. task is
 * unused */
static void fog_approach(Float target, Float step, struct task *task)
{
    if (fog_far < target) {
        fog_far += step;
    } else {
        fog_far -= step;
    }
    if (fog_far > FOG_NEAREST) {
        fog_far = FOG_NEAREST;
    }
    ruin_fog_color = 0;
    njGenerateFogTable3(ruin_fog, FOG_NEAR, fog_far);
}

enum fog_switch_mode {
    FOG_SWITCH_START = 0,
    FOG_SWITCH_RUN = 1,
    FOG_SWITCH_STILL = 2,
    FOG_SWITCH_END = 4
};

/* a fog switch. each time player 0 steps off it, it flips fog_held. while the
 * set editor is open, it draws a marker its sphere's size */
void fog_switch_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task *player;
    /* @hack type: a vector, of which only x is used, for the 12 bytes the
     * image's frame reserves (ADD #-12,R15 at 0x0c909534). as a Float, the
     * frame loses them, and 93 of 157 words differ */
    NJS_VECTOR scale;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case FOG_SWITCH_START:
        work->tick = 0;
        work->timer = 0;
        /* @hack redundant: 1s selected by a constant condition, which take R3
         * of their own (0x0c90956a). as 1, they share R12, which the store of
         * FOG_SWITCH_RUN takes, and 24 of 157 words differ */
        /* NOLINTBEGIN(misc-redundant-expression) */
        collision_init(task, &fog_switch_shape, COLLISION_OBJECT, 0.0F ? 1 : 1,
                       0.0F ? 1 : 1);
        /* NOLINTEND(misc-redundant-expression) */
        work->mode = FOG_SWITCH_RUN;
        work->collision->shapes->radius =
            (FOG_SWITCH_SCALE(work) + 1.0F) * SWITCH_REACH;
        break;
    case FOG_SWITCH_RUN:
        if (work->collision->flags & COLLISION_HIT) {
            work->collision->flags &= ~COLLISION_HIT;
            player = player_get_task(0);
            if (work->collision->hit->owner == player) {
                work->timer = 1;
            }
        } else {
            if (TASK_TIMER(work) == 1) {
                if (fog_held == 1) {
                    fog_held = 0;
                } else {
                    fog_held = 1;
                }
                /* @hack spelling: the timer reset in each arm. as one reset
                 * after the if, SHC loads 0 into R4 and 1 into R12 in the other
                 * order (0x0c909548 and 0x0c90954e), and 2 of 157 words
                 * differ */
                work->timer = 0;
            } else {
                work->timer = 0;
            }
        }
        collision_register(work);
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<- SCALE");
        }
        if (placement_editing(task) || edit_mode == 1) {
            scale.x = FOG_SWITCH_SCALE(work) + 1.0F;
            njPushMatrix(NULL);
            njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
            njScale(NULL, scale.x, scale.x, scale.x);
            draw_tree(&marker_object);
            njPopMatrix(1);
        }
        break;
    case FOG_SWITCH_STILL:
        break;
    case FOG_SWITCH_END:
        task_kill(task);
        break;
    }
}

/* the fog task's destructor. it clears the aim and the find, puts
 * mirror_lit_pos back at its first place, and holds the fog */
static void ruin_act1_fog_destroy(struct task *task)
{
    struct task_work *work = task->work;

    mirror_found = 0;
    FOG(work)->unk_04 = 0;
    FOG(work)->restored = 0;
    dat_0ca58ae0 = 0;
    mirror_aiming = 0;
    mirror_aim_id = 0;
    mirror_lit_id = 0;
    mirror_lit_pos.x = FIRST_LIT_X;
    mirror_lit_pos.y = FIRST_LIT_Y;
    mirror_lit_pos.z = FIRST_LIT_Z;
    fog_held = 1;
}

enum fog_light_mode { FOG_LIGHT_START, FOG_LIGHT_RUN };

/* a glow, which draws while its AIM_ID is mirror_aim_id */
void fog_light_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case FOG_LIGHT_START:
        work->mode = FOG_LIGHT_RUN;
        break;
    case FOG_LIGHT_RUN:
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<-AIM ID");
        }
        if (AIM_ID(work) == mirror_aim_id) {
            fog_light_draw(task);
        }
        break;
    }
}

/* draw the glow with Ninja's fog off, then turn the fog on */
static void fog_light_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    njFogDisable();
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&ruin_light_glow_model);
    njPopMatrix(1);
    njFogEnable();
}

enum hoghasira_mode { HOGHASIRA_START, HOGHASIRA_RUN };

void hoghasira_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case HOGHASIRA_START:
        work->mode = HOGHASIRA_RUN;
        collision_init(task, &hoghasira_shape, COLLISION_OBJECT, 1, 1);
        break;
    case HOGHASIRA_RUN:
        if (placement_editing(task)) {
            njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<-AIM ID");
        }
        hoghasira_draw(task);
        collision_register(work);
        break;
    }
}

/* draw the pillar, with Ninja's fog off while its AIM_ID is mirror_lit_id.
 * the fog is on after, either way */
static void hoghasira_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    texture_set_stage();
    if (AIM_ID(work) == mirror_lit_id) {
        njFogDisable();
    } else {
        njFogEnable();
    }
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_model(&ruin_pillar_2_model);
    njPopMatrix(1);
    if (AIM_ID(work) == mirror_lit_id) {
        njFogEnable();
    } else {
        njFogDisable();
    }
    njFogEnable();
}
