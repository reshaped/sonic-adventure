/* the hiyoko, a big chick drawn from marker models. final's object list
 * names it E HIYOKO. it lays little chicks, bombs that hop after player 0
 * and go off */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "animal/kinds.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/explosion.h"
#include "enemy/enemy.h"
#include "enemy/hiyoko.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

enum hiyoko_mode {
    HIYOKO_INIT,
    HIYOKO_WANDER, /* it walks near home until it sees a player */
    HIYOKO_LAY, /* it faces player 0 and lays bombs until BOMB_COUNT are out */
    HIYOKO_FLEE, /* it walks away from player 0 until its bombs are gone */
    HIYOKO_SPRING, /* it springs about where a player hit it, then explodes */
    HIYOKO_EXPLODE,
    HIYOKO_GONE
};

enum bomb_mode {
    BOMB_INIT = 0,
    BOMB_WALK = 1, /* it hops toward player 0 */
    BOMB_FUSE = 2, /* a player lit it, and it blinks faster until it goes off */
    BOMB_BLAST = 4,
    BOMB_SPENT = 5,
    BOMB_GONE = 6
};

/* the hiyoko's bits in work->submode */
#define HIYOKO_FAR 0x1 /* it is farther than scl.x + MOVE_RANGE from home */
#define HIYOKO_SEES 0x2 /* enemy_sees_player found a player in sight */

/* a bomb's bit in work->flags: hiyoko_bomb_land found it on the floor */
#define BOMB_ON_GROUND 0x1

#define BOMB_COUNT 6 /* the most bombs the hiyoko has out at once */
/* the hiyoko lays a bomb each time its timer passes LAY_WAIT */
#define LAY_WAIT 25

#define MOVE_RANGE 100.0F
/* how far above home's height the floor under the hiyoko may lie */
#define LEVEL_RANGE 0.5F

#define WALK_SPEED 1.6F /* the hiyoko's speed along its heading */

/* a player within LIGHT_RANGE lights a bomb. a lit bomb stops hopping
 * while a player is within WAIT_RANGE */
#define LIGHT_RANGE 32.0F
#define WAIT_RANGE 20.0F

#define HOP_SPEED 1.2F /* a bomb's speed along its heading */
#define BOMB_GRAVITY 0.08F /* taken off a bomb's speed.y each step */

/* the ranges placement_try_leave_by takes, squared. the hiyoko stays
 * while the camera or player 0 is within 1800 of it, and a bomb while either
 * is within 1300 of it. either also stays while player 1 is Sonic and within
 * 40 */
#define HIYOKO_LEAVE_RANGE (1800.0F * 1800.0F)
#define BOMB_LEAVE_RANGE (1300.0F * 1300.0F)

/* bombs counts each bomb the hiyoko lays until the bomb reaches BOMB_GONE.
 * a failed task_create_child counts one too. a bomb that leaves range never
 * reaches BOMB_GONE. either keeps the hiyoko fleeing for good */
struct hiyoko_vars {
    Uint8 bombs;
};

struct hiyoko_bomb_vars {
    /* the steps left on its fuse, counting down, then the steps of its
     * blast, counting up */
    Uint16 steps;
    Uint8 unk_02[2];
    /* the steps into a blink. a blink shows the bomb for delay + 1 steps,
     * then hides it for 1 */
    Sint16 blink;
    Sint16 delay; /* one less after each blink, down to 0 */
};

#define DISTANCE2(a, b)                                                        \
    (SQUARE((a).x - (b).x) + SQUARE((a).y - (b).y) + SQUARE((a).z - (b).z))

#define VARS(work) ((struct hiyoko_vars *)&(work)->vars)
#define BOMB(work) ((struct hiyoko_bomb_vars *)&(work)->vars)

/* how far a bomb's timer runs before it next hops */
#define HOP_WAIT(work) ((work)->tick)
/* the phase of a bomb's sway from side to side, and how far the phase moves
 * a step */
#define SWAY(record) ((record)->heading)
#define SWAY_SPEED(record) ((record)->phase)
/* how much a blast's radius grows this step */
#define BLAST_GROWTH(work) ((work)->scl.x)

static void hiyoko_bomb_step(struct task *task);

/* clang-format off */
static struct collision_shape hiyoko_shapes[2] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_ENEMIES), {0.0F, 30.0F, 0.0F}, 10.0F, 6.0F, 0.0F, {0}, 0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE, ATTACK(1) | SHAPE_HURTS_PLAYERS,
     PUSH_FIRM(PUSH_ALL), {0.0F, 10.0F, 0.0F}, 10.0F, 0.5F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */
/* a bomb's body, then its blast, which is SHAPE_OFF until the bomb goes off */
/* clang-format off */
static struct collision_shape hiyoko_bomb_shapes[2] = {
    {SHAPE_SPHERE | SHAPE_HURTABLE, GUARD(1) | SHAPE_HURTS_PLAYERS, PUSH_ALL,
     {0.0F, 2.0F, 0.0F}, 4.0F, 0.0F, 0.0F, {0}, 0, NULL},
    {SHAPE_SPHERE | SHAPE_OFF | SHAPE_HURTABLE,
     ATTACK(2) | GUARD(3) | SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES,
     PUSH_FIRM(PUSH_ALL), {0.0F, 0.0F, 0.0F}, 20.0F, 0.0F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

static void hiyoko_init(struct task *task, struct task_work *work)
{
    struct lockon_target *record;

    collision_init(task, hiyoko_shapes, COLLISION_ENEMY, 1, 2);
    record = enemy_alloc_record(task, work);
    work->submode = 0;
    work->timer = 0;
    work->ang.x = 0;
    work->ang.z = 0;
    work->scl.x = 1.0F;
    work->scl.y = 1.0F;
    work->scl.z = 0.0F;
    VARS(work)->bombs = 0;
    record->sight_distance = 1200.0F * 1200.0F;
    record->near_distance = 40.0F * 40.0F;
    record->motion.speed.x = WALK_SPEED;
    record->turn_speed = 0x120;
    enemy_set_home(work, record);
    work->mode = HIYOKO_WANDER;
}

static void hiyoko_turn(struct task_work *work, struct lockon_target *record)
{
    /* @hack redundant: turned, which nothing reads, keeps the call that
     * sets it a JSR (0x8c101c88); without it, the call becomes a JMP, the
     * function runs 2 bytes short and 11 of 15 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    int turned;

    if (work->submode & HIYOKO_FAR) {
        enemy_turn_home(work, record);
    } else if (work->submode & HIYOKO_SEES) {
        turned = enemy_turn_to_player(work, record, 0);
    }
}

static void hiyoko_mark_far(struct task_work *work,
                            struct lockon_target *record)
{
    Float distance2 = enemy_home_distance2(work, record);
    Float range = work->scl.x + MOVE_RANGE;

    if (distance2 > range * range) {
        work->submode |= HIYOKO_FAR;
    } else {
        work->submode &= ~HIYOKO_FAR;
    }
}

static void hiyoko_explode(struct task_work *work)
{
    explosion_create_at(work->pos.x, work->pos.y + 5.0F, work->pos.z, 1.4F);
    enemy_scatter_debris(work, DEBRIS_COUNT);
    work->mode = HIYOKO_GONE;
}

static void hiyoko_knock_back(struct task_work *work,
                              struct lockon_target *record)
{
    struct task_work *player = player_work[0];
    NJS_VECTOR away;

    away.x = work->pos.x - player->pos.x;
    away.y = work->pos.y - player->pos.y;
    away.z = work->pos.z - player->pos.z;
    njUnitVector(&away);
    record->motion.speed.x = away.x;
    record->motion.speed.y = away.y;
    record->motion.speed.z = away.z;
    away.x *= -1.5F;
    away.y = -(away.y * 1.5F) + 2.5F;
    away.z *= -1.5F;
    player_set_speed(0, away.x, away.y, away.z);
}

/* a hit by a player's collision record sets the hiyoko springing about where
 * it is. it also knocks player 0 back, whichever player hit it. any other
 * hit, such as an attack's, makes the hiyoko explode */
static void hiyoko_take_hit(struct task_work *work,
                            struct lockon_target *record)
{
    struct collision_work *collision;
    struct collision_work *hit;

    enemy_ignore((void *)player_work);
    /* @hack spelling: collision and hit read ahead of the test, which puts
     * collision in R4 and hit in R6 (MOV.L @(24,R4),R6 at 0x8c101de2). with
     * hit read in the test, 6 of 37 words differ, and chained from work
     * ahead of it, 11 of 37 */
    collision = work->collision;
    hit = collision->hit;
    if (work->flags & COLLISION_DAMAGED) {
        if (hit->kind == COLLISION_PLAYER) {
            work->mode = HIYOKO_SPRING;
            work->timer = 0;
            enemy_set_home(work, record);
            hiyoko_knock_back(work, record);
        } else {
            work->mode = HIYOKO_EXPLODE;
            work->timer = 0;
        }
    }
}

static void hiyoko_lay_when_seen(struct task_work *work,
                                 struct lockon_target *record)
{
    if (enemy_sees_player(work, record)) {
        work->submode |= HIYOKO_SEES;
        work->mode = HIYOKO_LAY;
        return;
    }
    work->submode &= ~HIYOKO_SEES;
}

/* move by speed.x along the heading and by speed.z across it */
static void hiyoko_move(struct task_work *work, struct lockon_target *record)
{
    record->motion.speed.x += record->motion.accel.x;
    record->motion.speed.z += record->motion.accel.z;
    if (record->motion.speed.x > WALK_SPEED) {
        record->motion.speed.x = WALK_SPEED;
    } else if (record->motion.speed.x < 0.3F) {
        record->motion.speed.x = 0.3F;
    }
    work->pos.x += (record->motion.speed.x * njCos(work->ang.y)) +
                   (record->motion.speed.z * njSin(work->ang.y));
    work->pos.z += -(record->motion.speed.x * njSin(work->ang.y)) +
                   (record->motion.speed.z * njCos(work->ang.y));
}

static void hiyoko_draw(struct task_work *work, struct lockon_target *record)
{
    /* @hack spelling: each marker ball drawn through object, which keeps its
     * address in R8 across the calls (MOV.L @(0x8c1020ec),R8 at
     * 0x8c101fd8); passed straight, 229 of 451 words differ */
    NJS_OBJECT *object;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    /* @hack redundant: a discarded read of work->mode, which SHC drops but
     * which keeps work in R14 (MOV R4,R14 at 0x8c101f7c); without it, work
     * goes to the frame and 158 of 451 words differ */
    (void)work->mode;

    /* the body, and a wing at each side */
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 14.0F, 0.0F);
    njPushMatrix(NULL);
    njScale(NULL, 1.5F, 1.0F, 1.2F);
    draw_set_material(1.0F, 0.5F, 0.5F, 0.0F);
    object = &marker_ball_object;
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateY(NULL, -0x1000);
    njTranslate(NULL, 0.0F, 2.0F, 10.8F);
    njScale(NULL, 1.0F, 0.6F, 0.2F);
    draw_set_material(1.0F, 0.5F, 0.5F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateY(NULL, 0x1000);
    njTranslate(NULL, 0.0F, 2.0F, -10.8F);
    njScale(NULL, 1.0F, 0.6F, 0.2F);
    draw_set_material(1.0F, 0.5F, 0.5F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    /* a foot and a leg at each side */
    njPushMatrix(NULL);
    njRotateY(NULL, -0x1000);
    njTranslate(NULL, 2.0F, -12.0F, 5.0F);
    njScale(NULL, 0.8F, 0.2F, 0.5F);
    draw_set_material(1.0F, 0.5F, 0.25F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateY(NULL, -0x1000);
    njTranslate(NULL, 0.0F, -10.0F, 4.0F);
    njScale(NULL, 0.25F, 0.3F, 0.25F);
    draw_set_material(1.0F, 0.5F, 0.25F, 0.0F);
    draw_tree(&marker_cylinder_object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateY(NULL, 0x1000);
    njTranslate(NULL, 2.0F, -12.0F, -5.0F);
    njScale(NULL, 0.8F, 0.2F, 0.5F);
    draw_set_material(1.0F, 0.5F, 0.25F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateY(NULL, 0x1000);
    njTranslate(NULL, 0.0F, -10.0F, -4.0F);
    njScale(NULL, 0.25F, 0.3F, 0.25F);
    draw_set_material(1.0F, 0.5F, 0.25F, 0.0F);
    draw_tree(&marker_cylinder_object);
    njPopMatrix(1);
    njPopMatrix(1);

    /* the head, its beak and its eyes */
    njPushMatrix(NULL);
    njTranslate(NULL, 4.0F, 30.0F, 0.0F);
    njPushMatrix(NULL);
    njScale(NULL, 1.0F, 0.9F, 1.0F);
    draw_set_material(1.0F, 0.5F, 0.5F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njTranslate(NULL, 7.0F, -2.8F, 0.0F);
    njScale(NULL, 0.63F, 0.18F, 0.5F);
    draw_set_material(1.0F, 0.5F, 0.25F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateZ(NULL, 0x800);
    njRotateY(NULL, 0x1000);
    njTranslate(NULL, 9.4F, 0.0F, 0.0F);
    njScale(NULL, 0.3F, 1.5F, 1.5F);
    njPushMatrix(NULL);
    njScale(NULL, 0.15F, 0.15F, 0.15F);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    draw_tree(object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 1.55F, 0.0F, 0.0F);
    njScale(NULL, 0.01F, 0.06F, 0.06F);
    draw_set_material(1.0F, 0.0F, 0.0F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateZ(NULL, 0x800);
    njRotateY(NULL, -0x1000);
    njTranslate(NULL, 9.4F, 0.0F, 0.0F);
    njScale(NULL, 0.3F, 1.5F, 1.5F);
    njPushMatrix(NULL);
    njScale(NULL, 0.15F, 0.15F, 0.15F);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    draw_tree(object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 1.55F, 0.0F, 0.0F);
    njScale(NULL, 0.01F, 0.06F, 0.06F);
    draw_set_material(1.0F, 0.0F, 0.0F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);
    njPopMatrix(1);
    njPopMatrix(1);
    njPopMatrix(1);
}

/* stand the hiyoko 2 above the floor below it, and draw its shadow. where
 * that floor lies more than LEVEL_RANGE above home, put the hiyoko back
 * where it was */
static void hiyoko_keep_height(struct task_work *work,
                               struct lockon_target *record)
{
    Float ground = shadow_draw(work, 3.0F);

    work->pos.y = ground + 2.0F;
    /* @bug logic: the test compares the floor with pos.y + 2 just after
     * pos.y became the floor + 2, so it never holds and never puts the
     * hiyoko back (FCMP/GT FR3,FR15 at 0x8c10241e) */
    if (ground > work->pos.y + 2.0F) {
        enemy_restore_pos(work, record);
    }
    if (ground > record->home.y + LEVEL_RANGE) {
        enemy_restore_pos(work, record);
    }
}

/* draw the hiyoko as hiyoko_draw does, for the set editor. x, y and z go
 * unread */
static void hiyoko_edit_draw_body(struct task_work *work, Float x, Float y,
                                  Float z)
{
    /* @hack spelling: each marker ball drawn through object, which keeps its
     * address in R8 across the calls (MOV.L @(0x8c10264c),R8 at
     * 0x8c102524); passed straight, the object runs 32 bytes longer and 229
     * of 452 words differ */
    NJS_OBJECT *object;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);

    /* the body, and a wing at each side */
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 14.0F, 0.0F);
    njPushMatrix(NULL);
    njScale(NULL, 1.5F, 1.0F, 1.2F);
    draw_set_material(1.0F, 0.5F, 0.5F, 0.0F);
    object = &marker_ball_object;
    draw_tree(object);
    /* @hack redundant: a call of njTranslate that never runs. it puts
     * njTranslate's address in R11, and draw_set_material's and draw_tree's
     * in R10 and R9 (0x8c1024ee, 0x8c102514). without it, 28 of 452 words
     * differ */
    if (0) {
        njTranslate(NULL, 0.0F, 0.0F, 0.0F);
    }
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateY(NULL, -0x1000);
    njTranslate(NULL, 0.0F, 2.0F, 10.8F);
    njScale(NULL, 1.0F, 0.6F, 0.2F);
    draw_set_material(1.0F, 0.5F, 0.5F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateY(NULL, 0x1000);
    njTranslate(NULL, 0.0F, 2.0F, -10.8F);
    njScale(NULL, 1.0F, 0.6F, 0.2F);
    draw_set_material(1.0F, 0.5F, 0.5F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    /* a foot and a leg at each side */
    njPushMatrix(NULL);
    njRotateY(NULL, -0x1000);
    njTranslate(NULL, 2.0F, -12.0F, 5.0F);
    njScale(NULL, 0.8F, 0.2F, 0.5F);
    draw_set_material(1.0F, 0.5F, 0.25F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateY(NULL, -0x1000);
    njTranslate(NULL, 0.0F, -10.0F, 4.0F);
    njScale(NULL, 0.25F, 0.3F, 0.25F);
    draw_set_material(1.0F, 0.5F, 0.25F, 0.0F);
    draw_tree(&marker_cylinder_object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateY(NULL, 0x1000);
    njTranslate(NULL, 2.0F, -12.0F, -5.0F);
    njScale(NULL, 0.8F, 0.2F, 0.5F);
    draw_set_material(1.0F, 0.5F, 0.25F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateY(NULL, 0x1000);
    njTranslate(NULL, 0.0F, -10.0F, -4.0F);
    njScale(NULL, 0.25F, 0.3F, 0.25F);
    draw_set_material(1.0F, 0.5F, 0.25F, 0.0F);
    draw_tree(&marker_cylinder_object);
    njPopMatrix(1);
    njPopMatrix(1);

    /* the head, its beak and its eyes */
    njPushMatrix(NULL);
    njTranslate(NULL, 4.0F, 30.0F, 0.0F);
    njPushMatrix(NULL);
    njScale(NULL, 1.0F, 0.9F, 1.0F);
    draw_set_material(1.0F, 0.5F, 0.5F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njTranslate(NULL, 7.0F, -2.8F, 0.0F);
    njScale(NULL, 0.63F, 0.18F, 0.5F);
    draw_set_material(1.0F, 0.5F, 0.25F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateZ(NULL, 0x800);
    njRotateY(NULL, 0x1000);
    njTranslate(NULL, 9.4F, 0.0F, 0.0F);
    njScale(NULL, 0.3F, 1.5F, 1.5F);
    njPushMatrix(NULL);
    njScale(NULL, 0.15F, 0.15F, 0.15F);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    draw_tree(object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 1.55F, 0.0F, 0.0F);
    njScale(NULL, 0.01F, 0.06F, 0.06F);
    draw_set_material(1.0F, 0.0F, 0.0F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateZ(NULL, 0x800);
    njRotateY(NULL, -0x1000);
    njTranslate(NULL, 9.4F, 0.0F, 0.0F);
    njScale(NULL, 0.3F, 1.5F, 1.5F);
    njPushMatrix(NULL);
    njScale(NULL, 0.15F, 0.15F, 0.15F);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    draw_tree(object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 1.55F, 0.0F, 0.0F);
    njScale(NULL, 0.01F, 0.06F, 0.06F);
    draw_set_material(1.0F, 0.0F, 0.0F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);
    njPopMatrix(1);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void hiyoko_edit_draw(struct task_work *work)
{
    hiyoko_edit_draw_body(work, work->pos.x, work->pos.y, work->pos.z);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_set_material(0.2F, 0.0F, 1.0F, 0.0F);
    /* the cylinder is 20 across, so the disc's radius is 10 * scl.x +
     * MOVE_RANGE. hiyoko_mark_far tests scl.x + MOVE_RANGE */
    njScale(NULL, work->scl.x + (MOVE_RANGE * MARKER_FIT), 0.02F,
            work->scl.x + (MOVE_RANGE * MARKER_FIT));
    draw_tree(&marker_cylinder_object);
    njPopMatrix(1);
}

static void hiyoko_die(struct task *task, struct task_work *work,
                       struct lockon_target *record)
{
    animal_create_random(7, work->pos.x, work->pos.y + 15.0F, work->pos.z);
    enemy_end(task, work, record);
    placement_kill(task);
}

static void hiyoko_wander(struct task_work *work, struct lockon_target *record)
{
    hiyoko_lay_when_seen(work, record);
    hiyoko_mark_far(work, record);
    hiyoko_turn(work, record);
    hiyoko_move(work, record);
    hiyoko_keep_height(work, record);
    hiyoko_take_hit(work, record);
    hiyoko_draw(work, record);
    collision_register(work);
}

/* @hack spelling: enemy_turn_from_player through an inline function, which
 * puts collision_register's address in R3, not R2, in hiyoko_flee
 * (0x8c102a80); called directly, 2 of 32 words differ */
#pragma inline(hiyoko_turn_away)
static void hiyoko_turn_away(struct task_work *work,
                             struct lockon_target *record)
{
    enemy_turn_from_player(work, record, 0);
    /* @hack redundant: a return where the function ends, which also puts
     * collision_register's address in R3 in hiyoko_flee (0x8c102a80);
     * without it, 2 of 32 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

static void hiyoko_flee(struct task_work *work, struct lockon_target *record)
{
    hiyoko_turn_away(work, record);
    hiyoko_move(work, record);
    hiyoko_keep_height(work, record);
    hiyoko_take_hit(work, record);
    hiyoko_draw(work, record);
    collision_register(work);
    if (VARS(work)->bombs == 0) {
        work->mode = HIYOKO_LAY;
    }
}

static void hiyoko_lay_bomb(struct task *task, struct task_work *work)
{
    struct task *child;
    struct task_work *bomb;
    Angle ang;

    if (TASK_TIMER(work)++ > LAY_WAIT) {
        if (VARS(work)->bombs < BOMB_COUNT) {
            work->timer = 0;
            child = task_create_child(TASK_WORK, hiyoko_bomb_step, task);
            if (child != NULL) {
                ang = work->ang.y;
                bomb = child->work;
                bomb->pos.x = work->pos.x + (njCos(ang) * 15.0F);
                bomb->pos.y = work->pos.y + 25.0F;
                bomb->pos.z = work->pos.z + (njSin(-ang) * 15.0F);
                bomb->ang.x = 0;
                bomb->ang.y = ang;
                bomb->ang.z = 0;
                bomb->submode = 0;
                bomb->timer = 0;
                bomb->scl.x = 1.0F;
                bomb->scl.y = 1.0F;
                bomb->scl.z = 0.0F;
            }
            VARS(work)->bombs++;
        } else {
            work->mode = HIYOKO_FLEE;
        }
    }
}

static void hiyoko_lay(struct task *task, struct task_work *work,
                       struct lockon_target *record)
{
    hiyoko_lay_bomb(task, work);
    record->turn_speed = 0x400;
    enemy_turn_to_player(work, record, 0);
    hiyoko_keep_height(work, record);
    hiyoko_take_hit(work, record);
    hiyoko_draw(work, record);
    collision_register(work);
}

static void hiyoko_spring(struct task_work *work, struct lockon_target *record)
{
    NJS_VECTOR pull;

    pull.x = record->home.x - work->pos.x;
    pull.y = record->home.y - work->pos.y;
    pull.z = record->home.z - work->pos.z;
    pull.x *= 0.06F;
    pull.y *= 0.06F;
    pull.z *= 0.06F;
    record->motion.speed.x += pull.x;
    record->motion.speed.y += pull.y;
    record->motion.speed.z += pull.z;
    work->pos.x += record->motion.speed.x;
    work->pos.y += record->motion.speed.y;
    work->pos.z += record->motion.speed.z;
    if (TASK_TIMER(work)++ > 30) {
        work->mode = HIYOKO_EXPLODE;
    }
}

/* @hack spelling: enemy_end through an inline function. called directly,
 * hiyoko_step loads enemy_end's arguments in another order (MOV R12,R6 at
 * 0x8c102dce), and hiyoko_bomb_step ends in a jump to it. summed over the
 * two, 15 of 198 words differ */
#pragma inline(hiyoko_leave)
static void hiyoko_leave(struct task *task, struct task_work *work,
                         struct lockon_target *record)
{
    enemy_end(task, work, record);
    /* @hack redundant: a return where the function ends, which puts
     * task_destroy_children's address in R3, not R2, in hiyoko_step
     * (0x8c102dd6); without it, summed over hiyoko_step and
     * hiyoko_bomb_step, 2 of 198 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

void hiyoko_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct lockon_target *record = LOCKON_TARGET(task);

    if (edit_mode) {
        hiyoko_edit_draw(work);
        return;
    } else {
        /* @hack redundant: this else, after an arm that returns, puts
         * hiyoko_init's address in R3, not R2 (0x8c102d1a); without it, 8 of
         * 110 words differ */
        /* @hack redundant: a discarded read of player 0's mode, which SHC drops
         * but which keeps the image's load of player_work[0] (MOV.L @R4,R4
         * at 0x8c102ce0); without it, the object runs 32 bytes short and 106
         * of 110 words differ */
        (void)player->mode;
        switch (work->mode) {
        case HIYOKO_INIT:
            hiyoko_init(task, work);
            return;
        /* @hack redundant: an empty default, written second, which puts
         * hiyoko_explode's address in R1, not R3 (0x8c102d90); without it, 6
         * of 110 words differ, and written last, 4 of 110 */
        default:
            break;
        case HIYOKO_WANDER:
            hiyoko_wander(work, record);
            break;
        case HIYOKO_LAY:
            hiyoko_lay(task, work, record);
            break;
        case HIYOKO_FLEE:
            hiyoko_flee(work, record);
            break;
        case HIYOKO_SPRING:
            hiyoko_spring(work, record);
            hiyoko_draw(work, record);
            break;
        case HIYOKO_EXPLODE:
            hiyoko_explode(work);
            /* fall through */
        case HIYOKO_GONE:
            hiyoko_die(task, work, record);
            task->step = task_destroy_children;
            return;
        }
    }
    enemy_save_pos(work, record);
    njPrintD(NJM_LOCATION(40, 40), VARS(work)->bombs, 4);
    task_step_children(task);
    if (placement_try_leave_by(task, HIYOKO_LEAVE_RANGE)) {
        hiyoko_leave(task, work, record);
        task->step = task_destroy_children;
    }
}

static void hiyoko_bomb_init(struct task *task, struct task_work *work)
{
    struct lockon_target *record;

    collision_init(task, hiyoko_bomb_shapes, COLLISION_ENEMY, 1, 2);
    /* @hack redundant: a discarded read of task->work, which SHC drops but
     * which keeps task in R13, not the frame (MOV R4,R13 at 0x8c102e4e);
     * without it, 28 of 87 words differ */
    (void)task->work;
    work->submode = 0;
    /* @hack type: the timer cleared through TASK_TIMER, which holds 0 in
     * R11 and rand's address in R12 (0x8c102e60, 0x8c102e62); cleared as
     * work->timer, the two trade and 7 of 87 words differ */
    TASK_TIMER(work) = 0;
    HOP_WAIT(work) = (int)(rand() * (30.0F * RAND_SCALE)) + 30;
    work->scl.x = 1.0F;
    work->scl.y = 1.0F;
    work->scl.z = 0.0F;
    record = enemy_alloc_record(task, work);
    record->motion.speed.x = HOP_SPEED;
    SWAY(record) = (Angle)(rand() * (360.0F * ANGLE_PER_DEGREE * RAND_SCALE));
    record->turn_speed = 0x2b0;
    SWAY_SPEED(record) =
        (Angle)(rand() * (1.0F * ANGLE_PER_DEGREE * RAND_SCALE)) + 0x2c0;
    BOMB(work)->steps = 0;
    enemy_set_home(work, record);
    work->mode = BOMB_WALK;
}

static void hiyoko_bomb_go_off(struct task_work *work,
                               struct lockon_target *record)
{
    work->tick = 0;
    work->timer = 0;
    BOMB(work)->steps = 0;
    work->mode = BOMB_BLAST;
    work->collision->shapes->flags |= SHAPE_OFF;
    work->collision->shapes[1].flags &= ~SHAPE_OFF;
    work->collision->shapes[1].radius = 2.0F;
    BLAST_GROWTH(work) = 1.6F;
    enemy_scatter_debris(work, DEBRIS_COUNT);
}

/* only a hit by an enemy's collision record, such as another bomb's blast,
 * sets the bomb off */
static void hiyoko_bomb_take_hit(struct task_work *work,
                                 struct lockon_target *record)
{
    struct collision_work *hit;

    enemy_ignore((void *)player_work);
    hit = work->collision->hit;
    if (work->flags & COLLISION_DAMAGED) {
        if (hit->kind == COLLISION_ENEMY) {
            hiyoko_bomb_go_off(work, record);
        }
    }
}

static void hiyoko_bomb_hop(struct task_work *work,
                            struct lockon_target *record)
{
    record->motion.speed.x += record->motion.accel.x;
    record->motion.speed.y -= BOMB_GRAVITY;
    record->motion.speed.z += record->motion.accel.z;
    if (TASK_TIMER(work)++ > HOP_WAIT(work) && (work->flags & BOMB_ON_GROUND)) {
        record->motion.speed.y = 1.0F;
        work->timer = 0;
        HOP_WAIT(work) = (int)(rand() * (30.0F * RAND_SCALE)) + 60;
    }
    if (record->motion.speed.x > HOP_SPEED) {
        record->motion.speed.x = HOP_SPEED;
    } else if (record->motion.speed.x < HOP_SPEED) {
        record->motion.speed.x = HOP_SPEED;
    }
    work->ang.y += (Angle)(njSin(SWAY(record)) * (7.0F * ANGLE_PER_DEGREE));
    SWAY(record) += SWAY_SPEED(record);
    work->pos.x += (record->motion.speed.x * njCos(work->ang.y)) +
                   (record->motion.speed.z * njSin(work->ang.y));
    work->pos.y += record->motion.speed.y;
    work->pos.z += -(record->motion.speed.x * njSin(work->ang.y)) +
                   (record->motion.speed.z * njCos(work->ang.y));
}

static void hiyoko_bomb_draw(struct task_work *work,
                             struct lockon_target *record)
{
    /* @hack spelling: each marker ball drawn through object, which keeps its
     * address in R8 across the calls (MOV.L @(0x8c1032b0),R8 at
     * 0x8c103184); passed straight, 192 of 457 words differ */
    NJS_OBJECT *object;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    /* @hack redundant: a discarded read of work->mode, which SHC drops but
     * which keeps work in R14 (MOV R4,R14 at 0x8c10311c); without it, work
     * goes to the frame and 338 of 457 words differ */
    (void)work->mode;
    njScale(NULL, 0.2F, 0.2F, 0.2F);

    /* the body, and a wing at each side */
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 14.0F, 0.0F);
    njPushMatrix(NULL);
    njScale(NULL, 1.5F, 1.0F, 1.2F);
    draw_set_material(1.0F, 0.5F, 0.5F, 0.0F);
    object = &marker_ball_object;
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateY(NULL, -0x1000);
    njTranslate(NULL, 0.0F, 2.0F, 10.8F);
    njScale(NULL, 1.0F, 0.6F, 0.2F);
    draw_set_material(1.0F, 0.5F, 0.5F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateY(NULL, 0x1000);
    njTranslate(NULL, 0.0F, 2.0F, -10.8F);
    njScale(NULL, 1.0F, 0.6F, 0.2F);
    draw_set_material(1.0F, 0.5F, 0.5F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    /* a foot and a leg at each side */
    njPushMatrix(NULL);
    njRotateY(NULL, -0x1000);
    njTranslate(NULL, 2.0F, -12.0F, 5.0F);
    njScale(NULL, 0.8F, 0.2F, 0.5F);
    draw_set_material(1.0F, 0.5F, 0.25F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateY(NULL, -0x1000);
    njTranslate(NULL, 0.0F, -10.0F, 4.0F);
    njScale(NULL, 0.25F, 0.3F, 0.25F);
    draw_set_material(1.0F, 0.5F, 0.25F, 0.0F);
    draw_tree(&marker_cylinder_object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateY(NULL, 0x1000);
    njTranslate(NULL, 2.0F, -12.0F, -5.0F);
    njScale(NULL, 0.8F, 0.2F, 0.5F);
    draw_set_material(1.0F, 0.5F, 0.25F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateY(NULL, 0x1000);
    njTranslate(NULL, 0.0F, -10.0F, -4.0F);
    njScale(NULL, 0.25F, 0.3F, 0.25F);
    draw_set_material(1.0F, 0.5F, 0.25F, 0.0F);
    draw_tree(&marker_cylinder_object);
    njPopMatrix(1);
    njPopMatrix(1);

    /* the head, its beak and its eyes */
    njPushMatrix(NULL);
    njTranslate(NULL, 4.0F, 30.0F, 0.0F);
    njPushMatrix(NULL);
    njScale(NULL, 1.0F, 0.9F, 1.0F);
    draw_set_material(1.0F, 0.5F, 0.5F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njTranslate(NULL, 7.0F, -2.8F, 0.0F);
    njScale(NULL, 0.63F, 0.18F, 0.5F);
    draw_set_material(1.0F, 0.5F, 0.25F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateZ(NULL, 0x800);
    njRotateY(NULL, 0x1000);
    njTranslate(NULL, 9.7F, 0.0F, 0.0F);
    njScale(NULL, 0.3F, 1.5F, 1.5F);
    njPushMatrix(NULL);
    njScale(NULL, 0.15F, 0.15F, 0.15F);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    draw_tree(object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 1.55F, 0.0F, 0.0F);
    njScale(NULL, 0.01F, 0.06F, 0.06F);
    draw_set_material(1.0F, 0.0F, 0.0F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);
    njPopMatrix(1);

    njPushMatrix(NULL);
    njRotateZ(NULL, 0x800);
    njRotateY(NULL, -0x1000);
    njTranslate(NULL, 9.7F, 0.0F, 0.0F);
    njScale(NULL, 0.3F, 1.5F, 1.5F);
    njPushMatrix(NULL);
    njScale(NULL, 0.15F, 0.15F, 0.15F);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    draw_tree(object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 1.55F, 0.0F, 0.0F);
    njScale(NULL, 0.01F, 0.06F, 0.06F);
    draw_set_material(1.0F, 0.0F, 0.0F, 0.0F);
    draw_tree(object);
    njPopMatrix(1);
    njPopMatrix(1);
    njPopMatrix(1);
    njPopMatrix(1);
}

static void hiyoko_bomb_land(struct task_work *work,
                             struct lockon_target *record)
{
    struct angles slope;
    Float ground;

    ground = shadow_draw_slope(work, 1.2F, &slope);
    work->flags &= ~BOMB_ON_GROUND;
    if (ground > work->pos.y + 2.0F) {
        enemy_restore_pos(work, record);
    } else if (ground > work->pos.y) {
        work->flags |= BOMB_ON_GROUND;
        work->pos.y = ground;
        record->motion.speed.y *= -0.6F;
        if (record->motion.speed.y < 0.8F) {
            record->motion.speed.y = 0.0F;
        }
    }
}

static void hiyoko_bomb_light_when_near(struct task_work *work,
                                        struct lockon_target *record)
{
    int i;
    struct task_work *player;

    for (i = 0; i < 2; i++) {
        /* @hack spelling: player assigned in the test, which loads it into R6
         * and keeps the loop's count in R5 (MOV.L @R1+,R6 at 0x8c10366a);
         * assigned apart, the two trade and 8 of 49 words differ */
        if ((player = player_work[i]) != NULL) {
            if (DISTANCE2(player->pos, work->pos) < LIGHT_RANGE * LIGHT_RANGE) {
                work->mode = BOMB_FUSE;
                work->collision->shapes->push |= PUSH_FIRM(PUSH_ALL);
                BOMB(work)->delay = 5;
                BOMB(work)->steps = 40;
                break;
            }
        }
    }
}

/* 1 while player 0 or 1 is within WAIT_RANGE, else 0 */
static Sint8 hiyoko_bomb_player_near(struct task_work *work)
{
    int i;
    struct task_work *player;

    for (i = 0; i < 2; i++) {
        player = player_work[i];
        if (player != NULL) {
            if (DISTANCE2(player->pos, work->pos) < WAIT_RANGE * WAIT_RANGE) {
                return 1;
            }
        }
    }
    return 0;
}

#pragma inline(hiyoko_bomb_turn, hiyoko_bomb_fall)

/* @hack spelling: enemy_turn_to_player through an inline function. called
 * directly, hiyoko_bomb_fuse loads its arguments in another order (MOV
 * R14,R4 at 0x8c1037a2), and hiyoko_bomb_walk puts collision_register's
 * address in R2, not R3 (0x8c10372a). summed over the two, 5 of 106 words
 * differ */
static void hiyoko_bomb_turn(struct task_work *work,
                             struct lockon_target *record)
{
    enemy_turn_to_player(work, record, 0);
    /* @hack redundant: a return where the function ends, which puts
     * collision_register's address in R3, not R2, in hiyoko_bomb_walk
     * (0x8c10372a); without it, summed over hiyoko_bomb_walk and
     * hiyoko_bomb_fuse, 2 of 106 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

/* @hack spelling: the bomb's fall as an inline function, which copies
 * record to R4 and work to R5 in hiyoko_bomb_fuse (MOV R13,R4 at
 * 0x8c103750); written in place, 18 of 77 words differ */
static void hiyoko_bomb_fall(struct task_work *work,
                             struct lockon_target *record)
{
    record->motion.speed.y -= BOMB_GRAVITY;
    work->pos.y += record->motion.speed.y;
    /* @hack redundant: a return where the function ends, which puts
     * enemy_turn_to_player's address in R3, not R2, in hiyoko_bomb_fuse
     * (0x8c1037a0); without it, 2 of 77 words differ */
    /* NOLINTNEXTLINE(readability-redundant-control-flow) */
    return;
}

static void hiyoko_bomb_walk(struct task_work *work,
                             struct lockon_target *record)
{
    hiyoko_bomb_turn(work, record);
    hiyoko_bomb_hop(work, record);
    hiyoko_bomb_land(work, record);
    hiyoko_bomb_take_hit(work, record);
    hiyoko_bomb_draw(work, record);
    collision_register(work);
    hiyoko_bomb_light_when_near(work, record);
}

static void hiyoko_bomb_fuse(struct task_work *work,
                             struct lockon_target *record)
{
    if (hiyoko_bomb_player_near(work)) {
        hiyoko_bomb_fall(work, record);
    } else {
        hiyoko_bomb_turn(work, record);
        hiyoko_bomb_hop(work, record);
    }
    hiyoko_bomb_land(work, record);
    hiyoko_bomb_take_hit(work, record);
    if (BOMB(work)->blink++ > BOMB(work)->delay) {
        BOMB(work)->blink = 0;
        if (BOMB(work)->delay > 0) {
            BOMB(work)->delay--;
        }
    } else {
        hiyoko_bomb_draw(work, record);
    }
    if (BOMB(work)->steps-- < 1) {
        hiyoko_bomb_go_off(work, record);
    }
    collision_register(work);
}

/* draw the blast as a marker ball the size of its shape, red on every third
 * step. the ball turns 0x200 a step */
static void hiyoko_bomb_draw_blast(struct task_work *work,
                                   struct lockon_target *record)
{
    Float scale = work->collision->shapes[1].radius * MARKER_FIT;

    if (work->tick++ > 1) {
        work->tick = 0;
        draw_set_material(0.05F, 1.0F, 0.0F, 0.0F);
    } else {
        draw_set_material(0.05F, 0.5F, 0.5F, 0.0F);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, scale, scale, scale);
    draw_tree(&marker_ball_object);
    work->ang.y += 0x200;
    njPopMatrix(1);
}

static void hiyoko_bomb_blast(struct task_work *work,
                              struct lockon_target *record)
{
    /* @hack spelling: collision through a local, which loads it into R4
     * (MOV.L @(56,R14),R4 at 0x8c1038c8); chained, it goes to R3 and 5 of 43
     * words differ */
    struct collision_work *collision = work->collision;

    collision->shapes[1].radius += BLAST_GROWTH(work);
    BLAST_GROWTH(work) -= 0.04F;
    if (BLAST_GROWTH(work) < 0.0F) {
        BLAST_GROWTH(work) = 0.0F;
    }
    hiyoko_bomb_draw_blast(work, record);
    if (BOMB(work)->steps++ > 50) {
        work->mode = BOMB_SPENT;
    }
    collision_register(work);
}

static void hiyoko_bomb_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    struct lockon_target *record = LOCKON_TARGET(task);

    if (edit_mode) {
        return;
    }
    /* @hack redundant: a store to player->mode that never runs. it keeps the
     * image's load of player 0's work, which nothing uses (0x8c103968,
     * 0x8c103972). without it, the load goes, the function runs 4 bytes short
     * and 87 of 88 words differ */
    if (0) {
        player->mode = 0;
    }
    switch (work->mode) {
    case BOMB_INIT:
        hiyoko_bomb_init(task, work);
        return;
    case BOMB_WALK:
        hiyoko_bomb_walk(work, record);
        break;
    case BOMB_FUSE:
        hiyoko_bomb_fuse(work, record);
        break;
    case BOMB_BLAST:
        hiyoko_bomb_blast(work, record);
        break;
    case BOMB_SPENT:
        work->mode = BOMB_GONE;
        break;
    case BOMB_GONE:
        enemy_end(task, work, record);
        task_kill(task);
        VARS(task->parent->work)->bombs--;
        return;
    }
    enemy_save_pos(work, record);
    if (placement_try_leave_by(task, BOMB_LEAVE_RANGE)) {
        hiyoko_leave(task, work, record);
    }
}
