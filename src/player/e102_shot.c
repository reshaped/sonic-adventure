/* E-102's shots at his lock-ons, and the trails, flashes and blasts that go
 * with them */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "event/play.h"
#include "object/placement.h"
#include "player/e102.h"
#include "player/e102_aim.h"
#include "player/e102_bonus.h"
#include "player/e102_shot.h"
#include "player/e102_smoke.h"
#include "player/e102_sphere.h"
#include "player/lockon.h"
#include "player/player.h"
#include "player/textures.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

#define SE_SHOT 791
#define SE_LASER_SHOT 792

#define SHOT_STEPS 600 /* the most steps a shot lasts */

/* how far a homing shot flies in a step. beyond STRAIGHT_RANGE of its lock-on,
 * it turns toward the lock-on by its pull, and nearer, it heads straight at it.
 * within CATCH_RANGE, it sits on the lock-on */
#define SHOT_SPEED 5.0F
#define STRAIGHT_RANGE 100.0F
#define CATCH_RANGE 16.0F

/* the speed and scale of the smoke that puffs from his sides as he fires */
#define SMOKE_SPEED 1.0F
#define SMOKE_SCALE 0.2F

/* his shot actions, by where the target lies */
enum shot_way {
    SHOT_AHEAD,
    SHOT_BEHIND,
    SHOT_MINUS_Z,
    SHOT_PLUS_Z,
    SHOT_LOW,
    SHOT_HIGH,
    SHOT_WAYS
};

/* his shot actions' frames. marking his lock-ons sets SHOT_AIMED, and the
 * shot leaves at SHOT_FIRE. his arm kicks back till SHOT_KICKED, or till
 * SHOT_KICK_CUT while a marked lock-on waits. he aims at that one from
 * SHOT_AIM_AGAIN */
#define SHOT_AIM_AGAIN 10.0F
#define SHOT_AIMED 14.0F
#define SHOT_FIRE 15.0F
#define SHOT_KICK_CUT 24.0F
#define SHOT_KICKED 30.0F

enum shot_mode { SHOT_START, SHOT_HOMING, SHOT_FREE };
enum fade_mode { FADE_START, FADE_OUT };

/* the work as a shot's trail sees it */
struct trail_work {
    Sint8 mode;
    Sint8 first; /* left on the shot's first step */
    Uint8 unk_02[4];
    Uint16 upgrades; /* E-102's upgrades when he fired */
    Float fade; /* from 1 to 0 */
    Sint8 player;
    Uint8 unk_0d[11];
    Angle ang_y;
    Angle ang_z;
    NJS_POINT3 pos;
};

/* the work as the flash at the muzzle sees it */
struct flash_work {
    Sint8 mode;
    Uint8 unk_01[5];
    Uint16 upgrades;
    Float size;
    Sint8 player;
    Uint8 unk_0d[19];
    NJS_POINT3 pos;
};

/* the work as the blast at the muzzle sees it */
struct blast_work {
    Sint8 mode;
    Uint8 unk_01[7];
    Float frame; /* which of its eight textures shows */
    Uint8 unk_0c[12];
    Angle ang_y;
    Angle ang_z;
    NJS_POINT3 pos;
};

#define TRAIL(task) ((struct trail_work *)(task)->work)
#define FLASH(task) ((struct flash_work *)(task)->work)
#define BLAST(task) ((struct blast_work *)(task)->work)

extern NJS_ACTION e102_shot_ahead_action;
extern NJS_ACTION e102_shot_behind_action;
extern NJS_ACTION e102_shot_minus_z_action;
extern NJS_ACTION e102_shot_plus_z_action;
extern NJS_ACTION e102_shot_low_action;
extern NJS_ACTION e102_shot_high_action;
extern NJS_OBJECT e102_trail_first;
extern NJS_OBJECT e102_trail;
extern NJS_OBJECT e102_blast;
/* e102_blast's material. shot_blast_step sets its texture, one of
 * E102BEAM's e_sw01 to e_sw08, through this array, whose own address the
 * image loads */
extern NJS_MATERIAL e102_blast_materials[];

static void shot_draw(struct task *task);
static void shot_destroy(struct task *task);
static void shot_step(struct task *task);
static void shot_trail_draw(struct task *task);
static void shot_trail_step(struct task *task);
static void shot_flash_draw(struct task *task);
static void shot_flash_step(struct task *task);
static void shot_blast_step(struct task *task);

/* the smoke's speed before e102_fire_effects sets its x and z. its y stays 0,
 * so the smoke leaves level */
static const NJS_VECTOR fire_smoke_speed = {0.0F, 0.0F, 0.0F};

/* the shot's frames and the flash's, indexed by whether he has his laser:
 * E102BEAM's beam and e_flash without it, and e102newbeam and e102newflash
 * with it */
static NJS_TEXANIM shot_texanims[2] = {
#include "player/e102_shot/shot_texanims.inc"
};
static NJS_TEXANIM shot_flash_texanims[2] = {
#include "player/e102_shot/shot_flash_texanims.inc"
};

static NJS_ACTION *shot_actions[SHOT_WAYS] = {
    &e102_shot_ahead_action,   &e102_shot_behind_action,
    &e102_shot_minus_z_action, &e102_shot_plus_z_action,
    &e102_shot_low_action,     &e102_shot_high_action,
};

static NJS_SPRITE shot_sprite = {
#include "player/e102_shot/shot_sprite.inc"
};
static NJS_SPRITE shot_flash_sprite = {
#include "player/e102_shot/shot_flash_sprite.inc"
};
static NJS_ARGB shot_material = {0.5F, 0.5F, 0.5F, 0.5F};

/* clang-format off */
static struct collision_shape shot_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE,
    ATTACK(3) | GUARD(3) | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
    PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 8.0F, 0.0F, 0.0F, {0}, 0, NULL};
/* clang-format on */

/* @hack type: the trail's NJS_ARGB as four floats, read through the first,
 * which has shot_trail_draw load red's, green's and blue's own addresses (pool
 * words at 0x8c0dd44c to 0x8c0dd454). as an NJS_ARGB, SHC reaches them from its
 * address, and 113 of 114 words differ */
#ifdef GCC
/* keeps the read the tag names: GCC's four are the members of one
 * NJS_ARGB */
static NJS_ARGB shot_trail_color = {1.0F, 0.0F, 0.0F, -1.0F};
#define shot_trail_material shot_trail_color.a
#define shot_trail_red shot_trail_color.r
#define shot_trail_green shot_trail_color.g
#define shot_trail_blue shot_trail_color.b
#else
static Float shot_trail_material = 1.0F;
static Float shot_trail_red = 0.0F;
static Float shot_trail_green = 0.0F;
static Float shot_trail_blue = -1.0F;
#endif
static NJS_ARGB shot_flash_material = {1.0F, 0.0F, 0.0F, -1.0F};
/* @hack type: the blast's NJS_ARGB as four floats, read through the first,
 * which has shot_blast_step load red's, green's and blue's own addresses (pool
 * words at 0x8c0dd7a8 to 0x8c0dd7b0). as an NJS_ARGB, SHC reaches them from its
 * address, and 24 of 137 words differ */
#ifdef GCC
/* keeps the read the tag names: GCC's four are the members of one
 * NJS_ARGB */
static NJS_ARGB shot_blast_color = {1.0F, 1.0F, 1.0F, 1.0F};
#define shot_blast_material shot_blast_color.a
#define shot_blast_red shot_blast_color.r
#define shot_blast_green shot_blast_color.g
#define shot_blast_blue shot_blast_color.b
#else
static Float shot_blast_material = 1.0F;
static Float shot_blast_red = 1.0F;
static Float shot_blast_green = 1.0F;
static Float shot_blast_blue = 1.0F;
#endif

/* the lock-on in his list whose target is nearest to work, or NULL for
 * none. it counts only lock-ons with a target still there and no shot at
 * them, and with marked set, only marked ones. it stores the count in
 * e102->unshot */
static struct task *nearest_lockon(struct task_work *work, struct e102 *e102,
                                   Bool marked)
{
    struct task **list;
    Float best;
    int count;
    struct task *found;
    int i;

    /* @hack spelling: count zeroed first, which has SHC set found from count's
     * register (MOV R10,R11 at 0x8c0dcd46). in declaration order, found
     * takes the zero from R4 as count does, and 6 of 76 words differ */
    count = 0;
    best = 999999.0F;
    list = e102->lockons;
    found = NULL;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    for (i = e102->lockon_count; i > 0; i--) {
        struct task *lockon = *list++;
        struct lockon_extra *extra = lockon->extra;
        Float distance;

        if (extra->shot != NULL) {
            continue;
        }
        if (extra->target == NULL) {
            continue;
        }
        if (extra->target->work == NULL) {
            continue;
        }
        if (marked && !extra->marked) {
            continue;
        }
        count++;
        distance = njDistanceP2P(&work->pos, &extra->target->work->pos);
        if (best > distance) {
            best = distance;
            found = lockon;
        }
    }
    e102->unshot = count;
    return found;
}

static void shot_draw(struct task *task)
{
    struct shot_work *shot = SHOT(task);
    int laser;

    if (frame_step != 0) {
        return;
    }
    njSetConstantMaterial(&shot_material);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    laser = (shot->upgrades & PLAYER_E102_LASER) ? 1 : 0;
    njTranslateV(NULL, &shot->pos);
    njSetTexture(&e102beam_texlist);
    njDrawSprite3D(&shot_sprite, laser, NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
    njPopMatrix(1);
}

/* the shot's lock-on can take another shot, and a shot with SHOT_PIERCED
 * leaves a sphere where it ends */
static void shot_destroy(struct task *task)
{
    struct shot_work *shot = SHOT(task);
    struct task *lockon = shot->lockon;
    struct lockon_extra *extra;
    struct task *sphere;

    if (lockon != NULL) {
        extra = lockon->extra;
        extra->shot = NULL;
    }
    if (shot->flags & SHOT_PIERCED) {
        sphere = task_create(TASK_WORK, 2, e102_sphere_step);
        if (sphere != NULL) {
            sphere->work->pos = shot->pos;
        }
    }
}

/* a shot homes in on its lock-on, and adds itself to e102_current's shots
 * on each step it homes. once let go, it flies straight on till it's out of
 * range. it leaves a trail each step. it ends after SHOT_STEPS steps, or once
 * its lock-on is gone before it's let go */
static void shot_step(struct task *task)
{
    struct shot_work *shot = SHOT(task);
    struct task_motion *motion = task->motion;
    struct task *lockon = shot->lockon;
    struct lockon_extra *extra;
    struct task_work *player;
    struct task *made;
    struct trail_work *trail;
    NJS_VECTOR to;
    Float distance;
    Float scale;
    int se;

    shot->steps++;
    if (shot->steps > SHOT_STEPS) {
        task_kill(task);
        return;
    }
    switch (shot->mode) {
    case SHOT_START:
        if (lockon == NULL) {
            task_kill(task);
            return;
        }
        shot->scl.x = 1.0F;
        motion->speed.x *= SHOT_SPEED;
        motion->speed.y *= SHOT_SPEED;
        motion->speed.z *= SHOT_SPEED;
        shot->pull = 0.0F;
        collision_init(task, &shot_shape, COLLISION_ATTACK, 1, 1);
        extra = lockon->extra;
        /* @hack spelling: player read before extra's test, which the image does
         * in the delay slot of the BF/S that tests it (MOV.L @(12,R5),R4 at
         * 0x8c0dcfa2). with extra NULL, that read is of address 12, and nothing
         * uses its value. reading player after the test does the same as long
         * as that read has no effect. read there, it moves below the BRA, and 7
         * of 411 words differ */
        player = extra->player;
        if (extra == NULL) {
            task_kill(task);
            return;
        }
        if (player == NULL) {
            player = player_work[0];
        }
        shot->player = player;
        shot->upgrades = extra->upgrades;
        se = (shot->upgrades & PLAYER_E102_LASER) ? SE_LASER_SHOT : SE_SHOT;
        if (e102_in_play) {
            sound_play_at(se, 0, 0, 0, shot->pos.x, shot->pos.y, shot->pos.z);
        }
        task->draw = shot_draw;
        task->destructor = shot_destroy;
        to = lockon->work->pos;
        njSubVector(&to, &shot->pos);
        distance = njScalor(&to);
        scale = SHOT_SPEED / distance;
        if (distance < STRAIGHT_RANGE) {
            if (distance >= SHOT_SPEED) {
                to.x *= scale;
                to.y *= scale;
                to.z *= scale;
            }
            motion->speed = to;
        } else {
            njUnitVector(&to);
            motion->speed.x = to.x * scale;
            motion->speed.y = to.y * scale;
            motion->speed.z = to.z * scale;
        }
        shot->mode = SHOT_HOMING;
        break;
    case SHOT_HOMING:
        if (lockon == NULL) {
            task_kill(task);
            return;
        }
        to = lockon->work->pos;
        njSubVector(&to, &shot->pos);
        distance = njScalor(&to);
        if (distance >= CATCH_RANGE) {
            if (distance >= STRAIGHT_RANGE) {
                scale = shot->pull / distance;
                to.x *= scale;
                to.y *= scale;
                to.z *= scale;
                njAddVector(&motion->speed, &to);
            } else {
                motion->speed = to;
            }
            njUnitVector(&motion->speed);
            motion->speed.x *= SHOT_SPEED;
            motion->speed.y *= SHOT_SPEED;
            motion->speed.z *= SHOT_SPEED;
            njAddVector(&shot->pos, &motion->speed);
        } else {
            shot->pos = lockon->work->pos;
        }
        e102_current->shots[e102_current->shot_count++] = task;
        break;
    case SHOT_FREE:
        if (placement_try_leave(task)) {
            task_kill(task);
            return;
        }
        njAddVector(&shot->pos, &motion->speed);
        break;
    }
    made = task_create(TASK_WORK, 2, shot_trail_step);
    if (made != NULL) {
        trail = TRAIL(made);
        trail->pos = shot->pos;
        trail->ang_y =
            0x8000 - (Angle)(atan2f(motion->speed.z, motion->speed.x) *
                             ANGLE_PER_RADIAN);
        trail->ang_z =
            -(Angle)(atan2f(motion->speed.y,
                            njSqrt((motion->speed.x * motion->speed.x) +
                                   (motion->speed.z * motion->speed.z))) *
                     ANGLE_PER_RADIAN);
        trail->first = shot->pull == 0.0F;
        player = shot->player != NULL ? shot->player : player_work[0];
        trail->player = PLAYER_VARS(player)->player;
        trail->upgrades = shot->upgrades;
    }
    shot->pull = shot->pull + 0.5F > 5.0F ? shot->pull + 0.5F : 5.0F;
    shot_draw(task);
    /* @hack spelling: shot passed as the work, which keeps the work in R14 for
     * the call (MOV R14,R4 at 0x8c0dd316). through task->work, SHC loads
     * it again, and 1 of 411 words differ */
    collision_register((struct task_work *)shot);
}

/* draw a trail, scaled 2.5 along its way, and thicker with his laser. its
 * color goes with its fade: from white to blue with his laser, and from
 * yellow to black without */
static void shot_trail_draw(struct task *task)
{
    struct trail_work *trail = TRAIL(task);

    if (frame_step != 0) {
        return;
    }
    draw_control_save();
    njControl3D(NJD_CONTROL_3D_CONSTANT_MATERIAL);
    njPushMatrix(NULL);
    njTranslateV(NULL, &trail->pos);
    TURN(njRotateY, trail->ang_y);
    TURN(njRotateZ, trail->ang_z);
    if (trail->upgrades & PLAYER_E102_LASER) {
        njScale(NULL, 2.5F, 1.25F, 1.25F);
        shot_trail_red = trail->fade;
        shot_trail_green = trail->fade;
        shot_trail_blue = 1.0F;
    } else {
        njScale(NULL, 2.5F, 1.0F, 1.0F);
        shot_trail_red = trail->fade;
        shot_trail_green = trail->fade;
        shot_trail_blue = 0.0F;
    }
    njSetConstantMaterial((NJS_ARGB *)&shot_trail_material);
    if (trail->first) {
        njSetTexture(&e102beam_texlist);
        njSimpleDrawObject(&e102_trail_first);
    } else {
        njSimpleDrawObject(&e102_trail);
    }
    njPopMatrix(1);
    draw_control_restore();
}

/* a trail fades out over ten steps */
static void shot_trail_step(struct task *task)
{
    struct trail_work *trail = TRAIL(task);

    switch (trail->mode) {
    case FADE_START:
        trail->fade = 1.0F;
        task->draw = shot_trail_draw;
        trail->mode = FADE_OUT;
        /* fall through */
    case FADE_OUT:
        trail->fade -= 0.1F;
        if (trail->fade <= 0.0F) {
            task_destroy(task);
            return;
        }
        break;
    }
    shot_trail_draw(task);
}

static void shot_flash_draw(struct task *task)
{
    struct flash_work *flash = FLASH(task);
    int laser;

    if (frame_step != 0) {
        return;
    }
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetConstantMaterial(&shot_flash_material);
    njSetTexture(&e102beam_texlist);
    njPushMatrix(NULL);
    laser = (flash->upgrades & PLAYER_E102_LASER) ? 1 : 0;
    njTranslateV(NULL, &flash->pos);
    shot_flash_sprite.sx = shot_flash_sprite.sy = flash->size;
    njDrawSprite3D(&shot_flash_sprite, laser,
                   NJD_SPRITE_SCALE | NJD_SPRITE_ALPHA);
    njPopMatrix(1);
}

/* a flash shrinks to nothing in four steps */
static void shot_flash_step(struct task *task)
{
    struct flash_work *flash = FLASH(task);

    switch (flash->mode) {
    case FADE_START:
        flash->size = 2.0F;
        flash->mode = FADE_OUT;
        /* fall through */
    case FADE_OUT:
        flash->size -= 0.5F;
        if (flash->size <= 0.0F) {
            task_destroy(task);
            return;
        }
        break;
    }
    shot_flash_draw(task);
}

/* a blast grows and dims through E102BEAM's e_sw01 to e_sw08, two steps
 * each but the first, and ends on its 16th step */
static void shot_blast_step(struct task *task)
{
    struct blast_work *blast = BLAST(task);
    Float scale;
    Float color;

    switch (blast->mode) {
    case FADE_START:
        blast->frame = 0.0F;
        blast->mode = FADE_OUT;
        /* fall through */
    case FADE_OUT:
        blast->frame += 0.5F;
        if (blast->frame >= 8.0F) {
            task_destroy(task);
            return;
        }
        break;
    }
    draw_control_save();
    njControl3D((_nj_control_3d_flag_ & ~NJD_CONTROL_3D_OFFSET_MATERIAL) |
                NJD_CONTROL_3D_CONSTANT_MATERIAL);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    scale = blast->frame * 0.125F;
    color = 1.5F - scale;
    shot_blast_red = color;
    shot_blast_green = color;
    shot_blast_blue = color;
    njSetConstantMaterial((NJS_ARGB *)&shot_blast_material);
    njSetTexture(&e102beam_texlist);
    njPushMatrix(NULL);
    scale *= 2.0F;
    njTranslateV(NULL, &blast->pos);
    TURN(njRotateY, blast->ang_y);
    TURN(njRotateZ, blast->ang_z);
    njScale(NULL, scale, scale, scale);
    e102_blast_materials[0].attr_texId = (Uint32)blast->frame;
    draw_tree(&e102_blast);
    njPopMatrix(1);
    draw_control_restore();
}

/* the shot action that turns him toward target. the quarter round him that
 * target lies in picks one of the four level actions. a target more than 45
 * degrees above him takes SHOT_HIGH, and one as far below takes SHOT_LOW. in
 * his back half, the two swap. skating, he always fires SHOT_LOW */
static enum shot_way shot_choose_way(struct task_work *work,
                                     struct task_work *target)
{
    Float dx;
    Float dy;
    Float dz;
    Angle yaw;
    Angle pitch;
    enum shot_way way;
    Bool behind;

    if (work->mode == E102_SKATE || work->mode == E102_WATER_SKATE) {
        return SHOT_LOW;
    }
    dx = target->pos.x - work->pos.x;
    dy = target->pos.y - work->pos.y;
    dz = target->pos.z - work->pos.z;
    yaw = ((Angle)(atan2f(dz, dx) * ANGLE_PER_RADIAN) - work->ang.y + 0x2000) &
          0xffff;
    pitch =
        -(Angle)(atan2f(dy, njSqrt((dx * dx) + (dz * dz))) * ANGLE_PER_RADIAN);
    switch (yaw & 0xc000) {
    case 0x4000:
        way = SHOT_PLUS_Z;
        break;
    case 0x8000:
        way = SHOT_BEHIND;
        break;
    case 0xc000:
        way = SHOT_MINUS_Z;
        break;
    default:
        way = SHOT_AHEAD;
        break;
    }
    behind = yaw >= 0x6000 && yaw <= 0xe000;
    if (pitch < -0x2000) {
        way = behind ? SHOT_LOW : SHOT_HIGH;
    }
    if (pitch > 0x2000) {
        way = behind ? SHOT_HIGH : SHOT_LOW;
    }
    return way;
}

void e102_shoot(struct task *task, struct player *record, struct e102 *e102)
{
    struct task_work *work = task->work;
    struct task *lockon;
    struct task *bonus;
    struct task *shot;
    struct lockon_extra *extra;
    struct task **list;
    int count;
    int i;

    switch (e102->shot_phase) {
    case E102_SHOT_START:
        e102->kick = 0.0F;
        e102->shot_phase = E102_SHOT_TURN;
        /* fall through */
    case E102_SHOT_TURN:
        lockon = nearest_lockon(work, e102, FALSE);
        if (lockon != NULL) {
            e102->shot_way = shot_choose_way(work, lockon->work);
        } else {
            e102->shot_way = SHOT_AHEAD;
        }
        e102->shot_action = shot_actions[e102->shot_way];
        if (lockon == NULL) {
            break;
        }
        if (e102->lockon_count <= 0) {
            break;
        }
        if (e102->aim_mode != E102_AIM_READY) {
            break;
        }
        e102->shot_phase = E102_SHOT_MARK;
        /* fall through */
    case E102_SHOT_MARK:
        e102->shot_frame = SHOT_AIMED;
        count = 0;
        /* @hack spelling: list from &e102->lockons[0], which puts task_create's
         * address in R2 (MOV.L @(0x8c0ddad4),R2 at 0x8c0dda1a). from
         * e102->lockons, R3 takes it, and 7 of 289 words differ */
        /* @hack spelling: list set after count, which has SHC store count to
         * the frame before list (0x8c0dda22, 0x8c0dda24). set before count,
         * list is stored first, and 5 of 289 words differ */
        list = &e102->lockons[0];
        bonus = task_create(TASK_WORK, 2, e102_bonus_step);
        if (bonus == NULL) {
            break;
        }
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        for (i = e102->lockon_count; i > 0; i--) {
            lockon = *list++;
            extra = lockon->extra;
            if (extra->shot != NULL) {
                continue;
            }
            if (extra->target == NULL) {
                continue;
            }
            if (extra->target->work == NULL) {
                continue;
            }
            extra->marked = TRUE;
            if (!e102_target_counts(extra->target)) {
                continue;
            }
            count++;
            extra->bonus = bonus;
        }
        BONUS(bonus)->volley[VOLLEY_SHOTS] = count;
        /* fall through */
    case E102_SHOT_NEXT:
        e102->kick = 0.0F;
        lockon = nearest_lockon(work, e102, TRUE);
        if (lockon != NULL) {
            e102->shot_way = shot_choose_way(work, lockon->work);
            e102->shot_action = shot_actions[e102->shot_way];
            e102->shot_phase = E102_SHOT_AIM;
        } else {
            e102->shot_phase = E102_SHOT_START;
            break;
        }
        /* fall through */
    case E102_SHOT_AIM:
        e102->shot_frame += 1.0F;
        if (e102->shot_frame < SHOT_FIRE) {
            break;
        }
        e102->shot_phase = E102_SHOT_KICK;
        lockon = nearest_lockon(work, e102, TRUE);
        if (lockon == NULL) {
            break;
        }
        extra = lockon->extra;
        if (extra->target == NULL) {
            break;
        }
        if (extra->target->step == NULL) {
            break;
        }
        shot = task_create(TASK_MOTION | TASK_WORK, 6, shot_step);
        if (shot != NULL) {
            PLAYER_VARS(shot->work)->player = PLAYER_VARS(task->work)->player;
            shot->work->pos = e102->shot_pos;
            shot->motion->speed = e102->shot_dir;
            SHOT(shot)->lockon = lockon;
            extra->shot = shot;
            e102_fire_effects(task, record, e102);
        }
        break;
    case E102_SHOT_KICK:
        e102->shot_frame += 1.0F;
        e102->kick = 1.0F - ((e102->shot_frame - SHOT_FIRE) *
                             (1.0F / (SHOT_KICKED - SHOT_FIRE)));
        /* @hack redundant: a break in each arm, which lets the two arms share
         * one store of E102_SHOT_NEXT (MOV.B R0,@(1,R14) at 0x8c0ddbf4).
         * without them, each arm makes its own, the function runs 2 bytes long,
         * and 110 of 289 words differ */
        if (nearest_lockon(work, e102, TRUE) != NULL) {
            if (e102->shot_frame >= SHOT_KICK_CUT) {
                e102->shot_frame = SHOT_AIM_AGAIN;
                e102->shot_phase = E102_SHOT_NEXT;
                break;
            }
        } else {
            if (e102->shot_frame >= SHOT_KICKED) {
                e102->shot_frame = 0.0F;
                e102->shot_phase = E102_SHOT_NEXT;
                break;
            }
        }
        break;
    }
    /* the lock-ons and the shots list themselves again next step */
    e102->lockon_count = e102->shot_count = 0;
}

#ifdef GCC
struct task_work *e102_shot_shooter(struct task *task)
{
    return task->destructor == shot_destroy ? SHOT(task)->player : NULL;
}
#endif

void e102_release_shot(struct task *shot, NJS_VECTOR *speed)
{
    SHOT(shot)->mode = SHOT_FREE;
    SHOT(shot)->lockon = NULL;
    if (speed != NULL) {
        shot->motion->speed = *speed;
    }
}

void e102_fire_effects(struct task *task, struct player *record,
                       struct e102 *e102)
{
    NJS_VECTOR speed;
    struct task_work *work;
    struct task *made;
    Angle ang;

    speed = fire_smoke_speed;
    work = task->work;
    made = task_create(TASK_WORK, 3, shot_flash_step);
    if (made != NULL) {
        made->work->pos = e102->shot_pos;
        FLASH(made)->player = PLAYER_VARS(task->work)->player;
        FLASH(made)->upgrades = record->upgrades;
    }
    ang = 0x8000 - work->ang.y;
    made = task_create(TASK_WORK, 3, shot_blast_step);
    if (made != NULL) {
        made->work->pos = e102->shot_pos;
        BLAST(made)->ang_y = ang;
    }
    speed.x = njCos(ang) * SMOKE_SPEED;
    speed.z = njSin(ang) * SMOKE_SPEED;
    e102_smoke_create(&e102->points[0], &speed, SMOKE_SCALE);
    e102_smoke_create(&e102->points[1], &speed, SMOKE_SCALE);
    if (work->id == PLAYER_ID && event_in_control() != TRUE) {
        vibration_start(0, 4, 15, 0);
    }
}
