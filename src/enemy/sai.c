/* the sai, a rhino on treads. it drives about until it sees a player, then
 * revs and charges at player 0. beach's, windy's and icecap's object lists
 * name it E SAITO */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "animal/kinds.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/explosion.h"
#include "enemy/enemy.h"
#include "enemy/sai.h"
#include "enemy/sai_textures.h"
#include "object/placement.h"
#ifdef GCC
#include "object/simple.h"
#endif
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_MODEL sai_front_model;
extern NJS_MODEL sai_burner_model;
extern NJS_MODEL sai_side_plus_z_model;
extern NJS_MODEL sai_side_minus_z_model;
extern NJS_MODEL sai_board_model;
/* @hack type: the treads as arrays, so that the code pools each model's
 * address and reads meshsets 12 bytes past it (MOV.L @(12,R4),R4 at
 * 0x8c0fe06e). as plain models, SHC pools the address of meshsets instead.
 * summed over sai_setup, sai_roll_treads and sai_animate, which name them,
 * 151 of 455 words then differ */
extern NJS_MODEL sai_tread_plus_z_model[];
extern NJS_MODEL sai_tread_minus_z_model[];
extern NJS_MODEL sai_head_burner_plus_z_model;
extern NJS_MODEL sai_horn_model;
extern NJS_MODEL sai_head_burner_minus_z_model;
extern NJS_MODEL sai_head_model;
extern NJS_MODEL sai_body_model;
extern NJS_COLOR sai_tread_plus_z_uvs[];
extern NJS_COLOR sai_tread_minus_z_uvs[];
/* @hack type: an array, so that the code pools the action's address and
 * reads motion 4 bytes past it (MOV.L @(4,R3),R2 at 0x8c0fdfc0). as a plain
 * action, SHC pools the address of motion instead, and the object runs 32
 * bytes short. summed over the four functions that name it, 317 of 524
 * words then differ */
extern NJS_ACTION sai_drive_action[];
extern NJS_MKEY_F sai_rev_pos_keys[];
extern NJS_MKEY_A sai_rev_ang_keys[];
extern NJS_ACTION sai_rev_action;
/* @hack type: an array, so that sai_edit_draw pools the material's address
 * and stores attr_texId 12 bytes past it (MOV.L R0,@(12,R3) at 0x8c0ff136).
 * as a plain material, SHC pools the address of attr_texId instead. summed
 * over sai_animate_jet and sai_edit_draw, which name it, 8 of 80 words then
 * differ */
extern NJS_MATERIAL sai_jet_materials[];
extern NJS_OBJECT sai_jet_object;

/* the sai's work->mode */
enum sai_mode {
    SAI_INIT = 0,
    /* it drives ahead until its collision record touches an object's. it
     * then goes back to last_pos in x and z, and turns to sai_aim_random's
     * aim */
    SAI_DRIVE = 1,
    SAI_TURN = 2, /* it stops and turns toward aim_y, then drives */
    /* it stops and turns to face player 0, revving and throwing up dust,
     * until its timer passes REV_STEPS. sai_look starts it from SAI_DRIVE
     * or SAI_TURN */
    SAI_REV = 3,
    /* it charges, turning toward the target sai_look set, until player 0 is
     * behind it or its timer passes CHARGE_STEPS */
    SAI_CHARGE = 4,
    SAI_BRAKE = 5, /* it brakes to a stop, then revs again */
    /* enemy_take_hit found a hit. the sai rolls on under its last push,
     * unsteered and unregistered for collision, until hit_steps passes 10 */
    SAI_HIT = 7,
    SAI_BURST = 8 /* it bursts, frees an animal and goes */
};

#define REV_STEPS 100
#define CHARGE_STEPS 180

/* the rev action's root has a position key and an angle key for each of
 * its REV_KEYS frames */
#define REV_KEYS 16

#define JET_ALPHA_MIN 35
#define JET_ALPHA_MAX 252

/* sai_drive caps SPEED at DRIVE_SPEED before sai_move adds PUSH, and
 * sai_move caps it at TOP_SPEED after */
#define DRIVE_SPEED 0.48F
#define TOP_SPEED 3.4F
/* how far sai_turn turns the heading toward aim_y in a step */
#define TURN_SPEED 0xf0
/* sai_aim_random picks its spot in a square ROAM_SPAN across, centered on
 * home. ROAM_SPAN is the float just below 8000 */
#define ROAM_SPAN 7999.9996F
/* the treads' uvs scroll this far in a step for each unit of SPEED, and
 * this far for each unit of turn_speed. turn_speed scrolls the two treads
 * opposite ways */
#define TREAD_SPEED_ROLL 20.0F
#define TREAD_TURN_ROLL 0.026179927F

/* the sai's bytes in work->vars */
struct sai_vars {
    Uint8 hit_steps; /* the steps SAI_HIT has run */
    Uint8 hit; /* sai_take_hit sets it to 1, and sai.c never reads it */
    Uint8 unk_02[2];
    /* sai_setup's heap copies of the treads' uvs, which sai_roll_treads
     * scrolls */
    NJS_COLOR *tread_plus_z_uvs;
    NJS_COLOR *tread_minus_z_uvs;
};

#define VARS(work) ((struct sai_vars *)&(work)->vars)

/* the sai draws with njScale(scl.x, scl.y, scl.x), and keeps the speed of
 * its scale's spring in scl.z */
#define SPRING_SPEED(work) ((work)->scl.z)

/* the record's fields the sai spends its own way, under names of its own */
/* the jet's alpha, 255 for opaque, in the first byte of motion.record.
 * only a player task keeps a pointer there */
#define JET_ALPHA(record) (*(Sint8 *)&(record)->motion.record)
#define SPEED(record) ((record)->motion.speed.x) /* along its heading */
#define CLIMB(record) ((record)->motion.speed.y) /* its speed up */
#define PUSH(record) ((record)->motion.speed.z) /* added to SPEED each step */
/* the jet's shift across the sai's heading, which follows the rev action's
 * root while it revs */
#define JET_SWAY(record) ((record)->motion.slip)

#define SE_REV 821
#define SE_CHARGE 822

/* a texture list of bf01 to bf03, whose entries it shares with
 * e_sai_texlist, for sai_bf_sprite's frames */
static NJS_TEXLIST sai_bf_texlist = {
#include "enemy/sai/sai_bf_texlist.inc"
};
static NJS_TEXANIM sai_bf_frames[3] = {
#include "enemy/sai/sai_bf_frames.inc"
};
static NJS_POINT3 sai_bf_points[3] = {
    {-7.7553F, 11.21926F, 3.67874F},
    {-7.04521F, 15.14826F, 0.0F},
    {-7.7553F, 11.21926F, -3.67874F},
};

/* the sai's body, and its horn ahead of it */
/* clang-format off */
static struct collision_shape sai_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL,
     {-1.5F, 6.5F, 0.0F}, 8.5F, 8.5F, 4.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(2) | GUARD(1) | SHAPE_HURTS_PLAYERS,
     PUSH_ALL, {8.0F, 4.5F, 0.0F}, 4.5F, 4.5F, 4.0F, {0}, 0, NULL},
};
/* clang-format on */

NJS_MODEL *sai_debris_models[] = {
    &sai_front_model,
    &sai_burner_model,
    &sai_side_plus_z_model,
    &sai_side_minus_z_model,
    &sai_board_model,
    sai_tread_plus_z_model,
    sai_tread_minus_z_model,
    &sai_head_burner_plus_z_model,
    &sai_horn_model,
    &sai_head_burner_minus_z_model,
    &sai_head_model,
    &sai_body_model,
    NULL,
};

/* a constant in memory, which sai_setup reads for collision_init's count
 * (MOV.B @R2,R3 at 0x8c0fdf78) */
static const Uint8 sai_shape_count = sizeof(sai_shapes) / sizeof(sai_shapes[0]);

/* sai_setup fills it in, and nothing draws it */
static NJS_SPRITE sai_bf_sprite;

/* TRUE while other is within sight_distance of the sai, else FALSE. nothing
 * calls it. it isn't static, since SHC would leave a static out */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
Bool sai_sees(struct task_work *work, struct task_work *other,
              struct lockon_target *record)
{
    Float distance2 = SQUARE(other->pos.x - work->pos.x) +
                      SQUARE(other->pos.y - work->pos.y) +
                      SQUARE(other->pos.z - work->pos.z);

    if (distance2 < record->sight_distance) {
        Angle heading = work->ang.y;
        Angle aim = (Angle)(atan2f(other->pos.x - work->pos.x,
                                   other->pos.z - work->pos.z) *
                            ANGLE_PER_RADIAN) -
                    NJM_DEG_ANG(90);
        int difference = (Uint16)angle_difference(aim, heading);

        /* @bug logic: every difference passes the test, since one not
         * above -80 degrees is below 80 (CMP/GT R2,R4 at 0x8c0fdf10). so
         * the near_distance test below never runs (0x8c0fdf1a) */
        if (difference > -NJM_DEG_ANG(80) || difference < NJM_DEG_ANG(80)) {
            return TRUE;
        }
    } else {
        return FALSE;
    }
    if (distance2 < record->near_distance) {
        return TRUE;
    }
    return FALSE;
}

static void sai_setup(struct task *task, struct task_work *work)
{
    NJS_SPRITE *sprite = &sai_bf_sprite;
    struct lockon_target *record;
    struct angles slope;

    collision_init(task, sai_shapes, COLLISION_ENEMY, 1, sai_shape_count);
    record = enemy_alloc_record(task, work);
    work->mode = SAI_DRIVE;
    work->timer = 0;
    work->scl.x = 1.0F;
    work->scl.y = 1.0F;
    SPRING_SPEED(work) = 0.0F;
    record->flags |= ENEMY_KEEP_ON_LEDGE | ENEMY_SHADOW | ENEMY_TILT;
    record->frame =
        (Float)rand() * (Float)sai_drive_action->motion->nbFrame * RAND_SCALE;
    record->sight_distance = 100.0F * 100.0F;
    record->near_distance = 15.0F * 15.0F;
    SPEED(record) = 0.0F;
    PUSH(record) = 0.0F;
    record->probe_offset.x = 0.0F;
    record->probe_offset.y = 8.0F;
    record->probe_offset.z = 0.0F;
    record->top = 6.0F;
    record->radius = 15.0F;
    record->bottom = -8.0F;
    /* on the ground, enemy_collide scales motion.speed.x and z by friction.
     * for the sai they are SPEED and PUSH, which 1 leaves as they are */
    record->friction = 1.0F;
    record->floor_bounce = 0.2F;
    record->home = work->pos;
    record->last_pos = work->pos;
    sprite->tlist = &sai_bf_texlist;
    sprite->tanim = sai_bf_frames;
    sprite->sx = 0.19F;
    sprite->sy = 0.19F;
    sprite->p.x = 0.0F;
    sprite->p.y = 0.0F;
    sprite->p.z = 0.0F;
#ifdef GCC
    /* repairs the treads' short copies. the image's hold nbMesh * 3 uvs, 42,
     * as if each tread's 14 meshes were triangles (SHLL2 R0 at 0x8c0fe07a,
     * SHLL2 R4 at 0x8c0fe14c). its strips draw 56, so each draw of a tread
     * reads 14 uvs past the copy, the next heap block's header among them.
     * GCC's copies hold all 56 */
    if ((VARS(work)->tread_plus_z_uvs =
             heap_alloc(meshset_count_points(sai_tread_plus_z_model->meshsets) *
                        sizeof(NJS_TEX))) != NULL) {
        njMemCopy(VARS(work)->tread_plus_z_uvs,
                  sai_tread_plus_z_model->meshsets->vertuv,
                  meshset_count_points(sai_tread_plus_z_model->meshsets) *
                      sizeof(NJS_TEX));
    }
    if ((VARS(work)->tread_minus_z_uvs = heap_alloc(
             meshset_count_points(sai_tread_minus_z_model->meshsets) *
             sizeof(NJS_TEX))) != NULL) {
        njMemCopy(VARS(work)->tread_minus_z_uvs,
                  sai_tread_minus_z_model->meshsets->vertuv,
                  meshset_count_points(sai_tread_minus_z_model->meshsets) *
                      sizeof(NJS_TEX));
    }
#else
    /* @hack spelling: the heap copy assigned in the test, which tests R0
     * itself (TST R0,R0 at 0x8c0fe080). assigned apart, SHC tests R0 moved
     * to R3, the function runs 2 bytes longer, and 70 of 216 words differ */
    /* @hack spelling: the size as nbMesh * sizeof(NJS_TEX) * 3, which shifts
     * it before its move to R4 (SHLL2 R0 at 0x8c0fe07a). as nbMesh * 3 *
     * sizeof(NJS_TEX), as the other sizes are written, 2 of 216 words
     * differ */
    if ((VARS(work)->tread_plus_z_uvs = heap_alloc(
             sai_tread_plus_z_model->meshsets->nbMesh * sizeof(NJS_TEX) * 3)) !=
        NULL) {
        njMemCopy(VARS(work)->tread_plus_z_uvs,
                  sai_tread_plus_z_model->meshsets->vertuv,
                  sai_tread_plus_z_model->meshsets->nbMesh * 3 *
                      sizeof(NJS_TEX));
    }
    /* @hack spelling: the heap copy assigned in the test, which tests R0
     * itself (TST R0,R0 at 0x8c0fe14e). assigned apart, SHC tests R0 moved
     * to R3, the function runs 2 bytes longer, and 41 of 216 words differ */
    if ((VARS(work)->tread_minus_z_uvs =
             heap_alloc(sai_tread_minus_z_model->meshsets->nbMesh * 3 *
                        sizeof(NJS_TEX))) != NULL) {
        njMemCopy(VARS(work)->tread_minus_z_uvs,
                  sai_tread_minus_z_model->meshsets->vertuv,
                  sai_tread_minus_z_model->meshsets->nbMesh * 3 *
                      sizeof(NJS_TEX));
    }
#endif
    work->pos.y = ground_below(work->pos.x, work->pos.y, work->pos.z, &slope);
    work->ang.z = 0;
    work->ang.x = 0;
}

/* NOLINTBEGIN(clang-diagnostic-tautological-constant-out-of-range-compare) */
static void sai_flare_jet(struct task_work *work, struct lockon_target *record)
{
    JET_ALPHA(record) += 2;
    /* @bug logic: JET_ALPHA is a signed byte, which never passes
     * JET_ALPHA_MAX (CMP/GT R3,R2 at 0x8c0fe1aa). so the clamp never runs,
     * and an alpha that climbs past 127 wraps round to the negatives */
    if (JET_ALPHA(record) > JET_ALPHA_MAX) {
        JET_ALPHA(record) = (Sint8)JET_ALPHA_MAX;
    }
}
/* NOLINTEND(clang-diagnostic-tautological-constant-out-of-range-compare) */

static void sai_dim_jet(struct task_work *work, struct lockon_target *record)
{
    JET_ALPHA(record) -= 8;
    if (JET_ALPHA(record) < JET_ALPHA_MIN) {
        JET_ALPHA(record) = JET_ALPHA_MIN;
    }
}

/* set aim_y a quarter turn off the heading toward a random spot near
 * home */
static void sai_aim_random(struct task_work *work, struct lockon_target *record)
{
    NJS_POINT3 to;

    to.x = record->home.x + (RANDOM(-0.5F, 0.5F) * ROAM_SPAN);
    to.z = record->home.z + (RANDOM(-0.5F, 0.5F) * ROAM_SPAN);
    /* @bug logic: aim_y keeps atan2f's angle from +z. sai_aim_home
     * takes NJM_DEG_ANG(90) off the same angle to make a heading, and this
     * store skips it (MOV.L R3,@(R0,R13) at 0x8c0fe272) */
    record->aim_y = (Angle)(atan2f(to.x - work->pos.x, to.z - work->pos.z) *
                            ANGLE_PER_RADIAN);
}

/* set aim_y to the heading toward home. nothing calls it. it isn't static,
 * since SHC would leave a static out */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
void sai_aim_home(struct task_work *work, struct lockon_target *record)
{
    NJS_POINT3 to;

    to.x = record->home.x;
    to.z = record->home.z;
    record->aim_y = (Angle)(atan2f(to.x - work->pos.x, to.z - work->pos.z) *
                            ANGLE_PER_RADIAN) -
                    NJM_DEG_ANG(90);
}

static void sai_drive(struct task_work *work, struct lockon_target *record)
{
    struct collision_work *collision;
    struct collision_work *hit;

    enemy_ignore((void *)player_work);
    collision = work->collision;
    /* @hack spelling: hit read through work->collision again, not through
     * collision, which keeps collision in a frame slot (MOV.L R3,@R15 at
     * 0x8c0fe314). read through collision, the function runs 10 bytes short
     * and 53 of 56 words differ */
    hit = work->collision->hit;
    PUSH(record) = 0.02F;
    if (SPEED(record) > DRIVE_SPEED) {
        SPEED(record) = DRIVE_SPEED;
    }
    record->turn_speed = 0;
    if ((collision->flags & COLLISION_HIT) && hit->kind == COLLISION_OBJECT) {
        sai_aim_random(work, record);
        work->mode = SAI_TURN;
        work->timer = 0;
        work->pos.x = record->last_pos.x;
        work->pos.z = record->last_pos.z;
    }
    sai_dim_jet(work, record);
}

static void sai_turn(struct task_work *work, struct lockon_target *record)
{
    SPEED(record) = 0.0F;
    work->ang.y = angle_approach(work->ang.y, record->aim_y, TURN_SPEED);
    record->turn_speed = TURN_SPEED;
    /* @bug logic: the test reads angle_difference's signed turn, not its
     * size (CMP/GE R2,R0 at 0x8c0fe3a0). a turn the negative way passes it
     * at once, so the sai drives on after one step of TURN_SPEED */
    if (angle_difference(work->ang.y, record->aim_y) < 0x800) {
        work->mode = SAI_DRIVE;
        work->timer = 0;
    }
}

static void sai_explode(struct task_work *work)
{
    explosion_create_at(work->pos.x, work->pos.y + 5.0F, work->pos.z, 1.4F);
    enemy_break(work, sai_debris_models, &e_sai_texlist);
    enemy_scatter_debris(work, DEBRIS_COUNT);
}

static void sai_take_hit(struct task_work *work, struct lockon_target *record)
{
    if (enemy_take_hit(work, record)) {
        work->mode = SAI_HIT;
        VARS(work)->hit_steps = 0;
        VARS(work)->hit = 1;
        SPRING_SPEED(work) = 0.35F;
    }
}

/* start SAI_REV when enemy_sees_player sees a player. it sets the charge's
 * target to player 0's position if the sai is on screen, or else to one of
 * four spots near player 0. sai_look doesn't reset work->timer,
 * so the rev ends once the timer, counting from the drive or turn's start,
 * passes REV_STEPS */
static void sai_look(struct task_work *work, struct lockon_target *record,
                     struct task_work *player)
{
    /* @bug logic: the test compares the height between the sai and
     * player with sight_distance, a squared distance of 100 * 100 (FCMP/GT
     * FR2,FR3 at 0x8c0fe456). so it fails only a player 10000 or more above
     * or below */
    if (njAbs(work->pos.y - player->pos.y) < record->sight_distance) {
        if (enemy_sees_player(work, record)) {
            work->mode = SAI_REV;
            if (enemy_on_screen(&work->pos)) {
                record->target = player_work[0]->pos;
                return;
            }
            enemy_target_near_player(work, record, 0,
                                     (int)(rand() * (4.0F * RAND_SCALE)));
        }
    }
}

/* set JET_SWAY and record->phase to the shift and turn of the rev action's
 * root at record->frame. the action doesn't draw the jet, so sai_draw_jet
 * shifts and turns it by them to move with the body. then puff dust back
 * from each side */
static void sai_rev_shake(struct task_work *work, struct lockon_target *record)
{
    Uint32 frame;
    Uint32 next;
    Float along;
    Float diff;
    Angle turn;
    NJS_POINT3 minus_z;
    NJS_POINT3 plus_z;
    NJS_POINT3 point;
    NJS_VECTOR direction;
    NJS_VECTOR velocity;

    frame = (Uint32)record->frame;
    /* @hack spelling: next wrapped after the fraction is taken, which keeps
     * frame + 1 in R11 across njFraction's call (ADD #1,R11 at 0x8c0fe56a).
     * wrapped at once, 86 of 184 words differ */
    next = frame + 1;
    along = njFraction(record->frame);
    next %= REV_KEYS;
    diff = sai_rev_pos_keys[next].key[2] - sai_rev_pos_keys[frame].key[2];
    JET_SWAY(record) = sai_rev_pos_keys[frame].key[2] + (diff * along);
    turn = sai_rev_ang_keys[next].key[1] - sai_rev_ang_keys[frame].key[1];
    /* the turn's share scales by the float just below 1, not by 1 */
    record->phase = sai_rev_ang_keys[frame].key[1] +
                    (Angle)((Float)turn * along * 0.9999999404F);
    minus_z.x = -4.0F;
    minus_z.y = 1.0F;
    minus_z.z = -4.0F;
    plus_z.x = -4.0F;
    plus_z.y = 1.0F;
    plus_z.z = 4.0F;
    direction.x = -3.0F;
    direction.y = 0.0F;
    direction.z = 0.0F;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y + record->phase);
    njCalcPoint(NULL, &minus_z, &point);
    njCalcVector(NULL, &direction, &velocity);
    dust_create(&point, &velocity, 0.7F);
    njCalcPoint(NULL, &plus_z, &point);
    njCalcVector(NULL, &direction, &velocity);
    dust_create(&point, &velocity, 0.7F);
    njPopMatrix(1);
}

static void sai_rev(struct task_work *work, struct task_work *player,
                    struct lockon_target *record)
{
    Angle heading;

    PUSH(record) = -0.5F;
    record->turn_speed = 0;
    heading = work->ang.y;
    record->aim_y = (Angle)(atan2f(player->pos.x - work->pos.x,
                                   player->pos.z - work->pos.z) *
                            ANGLE_PER_RADIAN) -
                    NJM_DEG_ANG(90);
    angle_difference(record->aim_y, heading);
    work->ang.y = angle_approach(heading, record->aim_y, 0x300);
    sai_dim_jet(work, record);
    if (TASK_TIMER(work) % 2) {
        sai_rev_shake(work, record);
    }
    if (TASK_TIMER(work) > REV_STEPS) {
        work->timer = 0;
        work->mode = SAI_CHARGE;
    }
}

static void sai_trail_dust(struct task_work *work)
{
    NJS_POINT3 minus_z;
    NJS_POINT3 plus_z;
    NJS_POINT3 point;
    NJS_VECTOR velocity;

    if (TASK_TIMER(work) % 2) {
        minus_z.x = -1.0F;
        minus_z.y = 0.0F;
        minus_z.z = -4.0F;
        plus_z.x = -1.0F;
        plus_z.y = 0.0F;
        plus_z.z = 4.0F;
        velocity.x = 0.0F;
        velocity.y = 0.0F;
        velocity.z = 0.0F;
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y);
        njCalcPoint(NULL, &minus_z, &point);
        dust_create(&point, &velocity, 0.7F);
        njCalcPoint(NULL, &plus_z, &point);
        dust_create(&point, &velocity, 0.7F);
        njPopMatrix(1);
    }
}

static void sai_charge(struct task_work *work, struct lockon_target *record)
{
    struct task_work *player = player_work[0];
    NJS_VECTOR facing;
    NJS_VECTOR offset;
    Float dot;

    record->turn_speed = 0x40;
    enemy_turn_to_target(work, record);
    if (TASK_TIMER(work) % 2) {
        sai_trail_dust(work);
    }
    PUSH(record) = 0.1F;
    sai_flare_jet(work, record);
    facing.x = njCos(work->ang.y);
    facing.y = 0.0F;
    facing.z = -njSin(work->ang.y);
    offset.x = player->pos.x - work->pos.x;
    offset.y = 0.0F;
    offset.z = player->pos.z - work->pos.z;
    dot = njInnerProduct(&facing, &offset);
    if (dot < 0.0F || TASK_TIMER(work) > CHARGE_STEPS) {
        work->mode = SAI_BRAKE;
        work->timer = 0;
    }
}

static void sai_brake(struct task_work *work, struct lockon_target *record)
{
    sai_dim_jet(work, record);
    record->turn_speed = 0;
    PUSH(record) = -0.15F;
    sai_trail_dust(work);
    if (SPEED(record) < 0.5F) {
        SPEED(record) = 0.0F;
        PUSH(record) = 0.0F;
        work->mode = SAI_REV;
    }
}

/* scroll this sai's copies of the tread uvs, and point the shared tread
 * models at them. sai_animate points the models back at their own uvs only
 * on a step it draws the sai. until then, every draw of the models uses
 * this sai's copies */
static void sai_roll_treads(struct task_work *work,
                            struct lockon_target *record)
{
    NJS_COLOR *uv;
    Uint16 count;
    int scroll;
    int i;

    uv = VARS(work)->tread_plus_z_uvs;
#ifdef GCC
    /* repairs the tread's scroll: it scrolls every uv sai_setup's copy holds,
     * the 56 its strips draw. the image scrolls nbMesh * 3, 42, and draws heap
     * words past them, which vary */
    count = meshset_count_points(sai_tread_plus_z_model->meshsets);
#else
    count = sai_tread_plus_z_model->meshsets->nbMesh * 3;
#endif
    scroll = (int)((SPEED(record) * TREAD_SPEED_ROLL) +
                   ((Float)record->turn_speed * TREAD_TURN_ROLL));
    for (i = 0; i < count; i++) {
        uv->tex.v += scroll;
        uv++;
    }
    sai_tread_plus_z_model->meshsets->vertuv = VARS(work)->tread_plus_z_uvs;
    uv = VARS(work)->tread_minus_z_uvs;
#ifdef GCC
    /* repairs the other tread's scroll, as the first's */
    count = meshset_count_points(sai_tread_minus_z_model->meshsets);
#else
    count = sai_tread_minus_z_model->meshsets->nbMesh * 3;
#endif
    scroll = (int)((SPEED(record) * TREAD_SPEED_ROLL) -
                   ((Float)record->turn_speed * TREAD_TURN_ROLL));
    for (i = 0; i < count; i++) {
        uv->tex.v += scroll;
        uv++;
    }
    sai_tread_minus_z_model->meshsets->vertuv = VARS(work)->tread_minus_z_uvs;
}

static void sai_move(struct task_work *work, struct lockon_target *record)
{
    SPEED(record) += PUSH(record);
    if (SPEED(record) > TOP_SPEED) {
        SPEED(record) = TOP_SPEED;
    }
    if (SPEED(record) < 0.0F) {
        SPEED(record) = 0.0F;
    }
    CLIMB(record) += -(CLIMB(record) * 0.01F) - 0.4F;
    work->pos.x += SPEED(record) * njCos(work->ang.y);
    work->pos.y += CLIMB(record);
    work->pos.z -= SPEED(record) * njSin(work->ang.y);
}

/* draw the jet at JET_ALPHA, blended additively, shifted and turned as
 * sai_rev_shake set it. the loop transforms sai_bf_points as it does the
 * jet, and discards each result: nothing draws sai_bf_sprite */
static void sai_draw_jet(struct task_work *work, struct lockon_target *record)
{
    NJS_POINT3 point;
    int i;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    draw_set_material((Float)JET_ALPHA(record) * (1.0F / 255.0F), 1.0F, 1.0F,
                      1.0F);
    for (i = 0; i < 3; i++) {
        njSetTexture(&sai_bf_texlist);
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateZ, work->ang.z);
        TURN(njRotateX, work->ang.x);
        TURN(njRotateY, work->ang.y + record->phase);
        njTranslate(NULL, 0.0F, 0.0F, JET_SWAY(record));
        njCalcPoint(NULL, &sai_bf_points[i], &point);
        njPopMatrix(1);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y + record->phase);
    njTranslate(NULL, 0.0F, 0.0F, JET_SWAY(record));
    njSetTexture(&e_sai_texlist);
    draw_tree(&sai_jet_object);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* cycle the jet's texture through jet01 to jet03 by the tick, and set the
 * jet material's alpha to JET_ALPHA + 3 */
static void sai_animate_jet(struct task_work *work,
                            struct lockon_target *record)
{
    sai_jet_materials->attr_texId = work->tick % 3;
    sai_jet_materials->diffuse.argb.a = JET_ALPHA(record) + 3;
}

/* spring scl.y back toward 1 at SPRING_SPEED. each call damps SPRING_SPEED
 * by 0.12 of itself and pulls it by 0.32 of 1 - scl.y. scl.x becomes
 * 2 - scl.y, and neither is bounded */
static void sai_spring_scale(struct task_work *work)
{
    Float stretch = work->scl.y - 1.0F;

    SPRING_SPEED(work) += -(SPRING_SPEED(work) * 0.12F) - (stretch * 0.32F);
    work->scl.y += SPRING_SPEED(work);
    work->scl.x = 2.0F - work->scl.y;
}

/* spring the sai's scale, and draw its jet. on a step draw_in_view passes,
 * also draw the sai, advance its action, and point the tread models back at
 * their own uvs. the frame wraps at the drive action's count of frames,
 * which the rev action shares */
static void sai_animate(struct task_work *work, struct lockon_target *record)
{
    sai_spring_scale(work);
    if (work->mode != SAI_REV) {
        JET_SWAY(record) = 0.0F;
        record->phase = 0;
    }
    sai_animate_jet(work, record);
    sai_draw_jet(work, record);
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 10.0F)) {
        return;
    }
    njSetTexture(&e_sai_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.x);
    if (work->mode == SAI_REV) {
        draw_action(&sai_rev_action, record->frame);
        record->frame += 0.75F;
    } else {
        draw_action(sai_drive_action, record->frame);
        record->frame += 0.5F;
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
    sai_tread_plus_z_model->meshsets->vertuv = sai_tread_plus_z_uvs;
    sai_tread_minus_z_model->meshsets->vertuv = sai_tread_minus_z_uvs;
    if (record->frame > (Float)sai_drive_action->motion->nbFrame) {
        record->frame -= (Float)sai_drive_action->motion->nbFrame;
    }
}

static void sai_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_target *record = LOCKON_TARGET(task);

    njSetTexture(&e_sai_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, work->scl.x, work->scl.y, work->scl.x);
    if (work->mode == SAI_REV) {
        draw_action(&sai_rev_action, record->frame);
    } else {
        draw_action(sai_drive_action, record->frame);
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
}

/* enemy_collide under the sai's name. nothing calls it. it isn't static,
 * since SHC would leave a static out */
/* NOLINTNEXTLINE(misc-use-internal-linkage) */
void sai_collide(struct task_work *work, struct lockon_target *record)
{
    enemy_collide(work, record);
}

static void sai_edit_draw(struct task_work *work)
{
    njPrintC(NJM_LOCATION(0, 25), "EDIT");
    njSetTexture(&e_sai_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    draw_tree(sai_drive_action->object);
    draw_set_material(0.0F, 1.0F, 1.0F, 1.0F);
    draw_tree(&sai_jet_object);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    sai_jet_materials->attr_texId = work->tick % 3;
    njPopMatrix(1);
}

static void sai_end(struct task *task, struct task_work *work,
                    struct lockon_target *record)
{
    if (VARS(work)->tread_plus_z_uvs != NULL) {
        heap_free(VARS(work)->tread_plus_z_uvs);
    }
    if (VARS(work)->tread_minus_z_uvs != NULL) {
        heap_free(VARS(work)->tread_minus_z_uvs);
    }
    enemy_end(task, work, record);
}

static void sai_update(struct task_work *work, struct lockon_target *record)
{
    sai_roll_treads(work, record);
    sai_move(work, record);
    enemy_collide(work, record);
    sai_take_hit(work, record);
    sai_animate(work, record);
}

/* register the sai for collision while it is in range. out of range, free
 * its tread copies: placement_try_leave has the task destroy itself on
 * its next step */
static void sai_register_or_leave(struct task *task, struct task_work *work,
                                  struct lockon_target *record)
{
    if (placement_try_leave(task)) {
        sai_end(task, work, record);
    } else {
        collision_register(work);
    }
}

void sai_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct lockon_target *record = LOCKON_TARGET(task);

    if (edit_mode) {
        sai_edit_draw(work);
        return;
    } else {
        /* @hack redundant: this else, after an arm that returns, puts
         * sai_setup's address in R3, not R2 (0x8c0ff292); without it, 12 of
         * 214 words differ */
        switch (work->mode) {
        case SAI_INIT:
            sai_setup(task, work);
            task->draw = sai_draw;
            return;
        case SAI_DRIVE:
            sai_drive(work, record);
            sai_look(work, record, player);
            sai_update(work, record);
            sai_register_or_leave(task, work, record);
            break;
        case SAI_TURN:
            sai_turn(work, record);
            sai_look(work, record, player);
            sai_update(work, record);
            sai_register_or_leave(task, work, record);
            break;
        case SAI_REV:
            sai_rev(work, player, record);
            sai_update(work, record);
            sai_register_or_leave(task, work, record);
            sound_play_for(SE_REV, (int)task, 1, 0, 2);
            break;
        case SAI_CHARGE:
            sai_charge(work, record);
            sai_update(work, record);
            sai_register_or_leave(task, work, record);
            sound_play_for(SE_CHARGE, (int)task, 1, 0, 2);
            break;
        case SAI_BRAKE:
            sai_brake(work, record);
            sai_update(work, record);
            sai_register_or_leave(task, work, record);
            break;
        case SAI_HIT:
            sai_roll_treads(work, record);
            sai_move(work, record);
            enemy_collide(work, record);
            sai_animate(work, record);
            if (VARS(work)->hit_steps++ > 10) {
                work->mode = SAI_BURST;
            }
            break;
        case SAI_BURST:
            sai_explode(work);
            animal_create_random(2, work->pos.x, work->pos.y + 15.0F,
                                 work->pos.z);
            sai_end(task, work, record);
            placement_kill(task);
            return;
        }
    }
    work->tick++;
    TASK_TIMER(work)++;
    /* from the sai's first turn on, its body is firm against all four push
     * groups, which hold every kind of shape but an attack's */
    if (work->mode == SAI_TURN) {
        work->collision->shapes->push |= PUSH_FIRM(PUSH_ALL);
    }
    record->last_pos.x = work->pos.x;
    record->last_pos.y = work->pos.y;
    record->last_pos.z = work->pos.z;
}
