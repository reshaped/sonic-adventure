/* the item box, O ITEMBOX in the object lists. a player or an attack opens
 * it, and the item its face shows is given */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "effect/snow.h"
#include "hud/hud.h"
#include "object/item_box.h"
#include "object/item_box_air.h"
#include "object/models.h"
#include "object/placement.h"
#ifdef GCC
#include "player/e102_shot.h"
#endif
#include "player/lockon.h"
#include "player/player.h"
#include "player/target.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg03/mirror.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* every item plays SE_ITEM. in E-102's game, the speed up also plays
 * SE_E102_SPEED_UP, and every other item SE_E102_ITEM */
#define SE_E102_ITEM 1196
#define SE_E102_SPEED_UP 1197

/* the box's work->mode */
enum item_box_mode {
    ITEM_BOX_INIT, /* starts the box the object lists place */
    ITEM_BOX_GROW, /* grows in */
    ITEM_BOX_WAIT, /* waits to be opened */
    ITEM_BOX_OPEN, /* opens, in one step */
    ITEM_BOX_OPENED, /* only its base stays */
    ITEM_BOX_STILL, /* it collides, and nothing more */
    ITEM_BOX_RESET /* starts over at ITEM_BOX_INIT */
};

/* a box's kind is its scl.x, its icon's turn in degrees its scl.y, and its
 * growth its scl.z */
#define KIND(work) ((work)->scl.x)
#define SPIN(work) ((work)->scl.y)
#define GROWTH(work) ((work)->scl.z)

/* a growing box's GROWTH rises GROW_STEP a step, to GROWN. until then the
 * box draws at scale GROWTH / GROWN */
#define GROW_STEP 0.11F
#define GROWN 5.0F

/* a waiting box's icon turns SPIN_STEP degrees a step */
#define SPIN_STEP 3.0F

static void item_box_grow(struct task_work *work);
static void item_box_wait(struct task *task);
static void item_box_open(struct task *task);
static void item_box_draw(struct task *task);
static void item_box_draw_open(struct task *task);
static void item_box_burst(struct task_work *work);
static void item_box_destroy(struct task *task);
static void item_give_speed_up(struct task_work *work);
static void item_give_5_rings(struct task_work *work);
static void item_give_10_rings(struct task_work *work);
static void item_give_random_rings(struct task_work *work);
static void item_give_1up(struct task_work *work);
static void item_give_invincibility(struct task_work *work);
static void item_give_barrier(struct task_work *work);
static void item_give_bomb(struct task_work *work);
static void item_give_magnetic_barrier(struct task_work *work);

static const NJS_VECTOR item_box_up = {0.0F, 1.0F, 0.0F};
static const Sint16 random_ring_counts[6] = {1, 5, 10, 15, 20, 40};

/* a cylinder for the box, which an attack can hurt, and a sphere at its top
 * for a player's touch */
/* clang-format off */
static struct collision_shape item_box_shapes[2] = {
    {SHAPE_CYLINDER | SHAPE_HURTABLE | LOCKON_NO_NUMBER, 0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 5.0F, 0.0F}, 5.0F, 5.0F, 0.0F, {0},
     0, NULL},
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 10.0F, 0.0F}, 5.0F,
     0.0F, 0.0F, {0}, 0, NULL},
};
/* the two cylinders of the base an open box keeps */
static struct collision_shape item_box_open_shapes[2] = {
    {SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 1.5F, 0.0F},
     6.0F, 1.5F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 1.8F, 0.0F},
     2.0F, 1.8F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* the texnames of dat_8c1d0510's two texlists, one texture each. they
 * start zeroed, with no file name */
static NJS_TEXNAME dat_8c794ec8[1];
static NJS_TEXNAME dat_8c794ed4[1];
static NJS_TEXLIST dat_8c1d0510[2] = {
#include "object/item_box/dat_8c1d0510.inc"
};

/* each kind's face, by its texture's name in OBJ_REGULAR */
struct item item_kinds[ITEM_COUNT] = {
    {35, item_give_speed_up}, /* item_speed */
    {72, item_give_invincibility}, /* item_muteki */
    {33, item_give_5_rings}, /* item_ring5 */
    {32, item_give_10_rings}, /* item_ring10 */
    {34, item_give_random_rings}, /* item_ringq */
    {71, item_give_barrier}, /* item_nbarria */
    {31, item_give_1up}, /* item_1up, until a draw sets the character's */
    {73, item_give_bomb}, /* item_explosion */
    {70, item_give_magnetic_barrier}, /* item_tbarria */
};
/* each character's extra-life face, by its texture's name in OBJ_REGULAR.
 * Eggman and Tikal have no face of their own, and show Knuckles' */
Uint32 item_1up_textures[CHARACTER_COUNT] = {
    31, /* Sonic: item_1up */
    0, /* Eggman: 1upknuck */
    4, /* Tails: 1uptails */
    0, /* Knuckles: 1upknuck */
    0, /* Tikal: 1upknuck */
    1, /* Amy: 1upamy */
    3, /* E-102: 1up102 */
    2, /* Big: 1upbig */
};

/* the kind of the box whose step or draw ran last. item_box_wait reads it
 * as its box's kind */
static Uint32 item_box_kind;
static Sint32 dat_8c794ee4;

void item_box_create(NJS_POINT3 *pos, enum item_kind kind)
{
    struct task *task = task_create(TASK_WORK, 3, item_box_step);
    struct task_work *work = task->work;

    work->pos = *pos;
    KIND(work) = kind;
    task->destructor = item_box_destroy;
    task->draw = item_box_draw;
    collision_init(task, item_box_shapes, COLLISION_ITEM, 1, 2);
    work->mode = ITEM_BOX_GROW;
    GROWTH(work) = 0.0F;
    task->motion = heap_calloc(1, sizeof(struct lockon_target));
}

void item_box_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if ((int)KIND(work) < 0) {
        KIND(work) = ITEM_COUNT - 1;
    } else if ((int)KIND(work) >= ITEM_COUNT) {
        KIND(work) = 0.0F;
    }
    item_box_kind = KIND(work);
    switch (work->mode) {
    case ITEM_BOX_INIT:
        if (task->placement != NULL) {
            placement_keep(task);
        }
        task->destructor = item_box_destroy;
        task->draw = item_box_draw;
        collision_init(task, item_box_shapes, COLLISION_ITEM, 1, 2);
        work->mode = ITEM_BOX_GROW;
        GROWTH(work) = 0.0F;
        task->motion = heap_calloc(1, sizeof(struct lockon_target));
        if (stage_get_stage_act() == STAGE_TWINKLE_MIRRORS) {
            twinkle_reflect(task);
        }
        if (placement_used(task)) {
            work->mode = ITEM_BOX_OPEN;
        }
        break;
    case ITEM_BOX_GROW:
        item_box_grow(work);
        break;
    case ITEM_BOX_WAIT:
        item_box_wait(task);
        break;
    case ITEM_BOX_OPEN:
        item_box_open(task);
        break;
    case ITEM_BOX_OPENED:
    case ITEM_BOX_STILL:
        collision_register(work);
        break;
    case ITEM_BOX_RESET:
    default:
        work->mode = ITEM_BOX_INIT;
        break;
    }
    if (work->mode == ITEM_BOX_INIT || work->mode == ITEM_BOX_WAIT ||
        work->mode == ITEM_BOX_GROW) {
        item_box_draw(task);
    } else if (work->mode == ITEM_BOX_OPEN || work->mode == ITEM_BOX_OPENED) {
        item_box_draw_open(task);
    }
    /* @hack redundant: the range checked again, its result tested to no
     * effect, which keeps the image's TST R0,R0 at 0x8c060548. without it,
     * the step runs 2 bytes short and 27 of its 185 words differ */
    if (placement_try_leave(task)) {
    }
    if (work->mode == ITEM_BOX_WAIT) {
        player_add_target(work->pos.x, work->pos.y, work->pos.z, 3.0F);
    }
    task_step_children(task);
}

static void item_box_grow(struct task_work *work)
{
    GROWTH(work) += GROW_STEP;
    if (GROWTH(work) >= GROWN) {
        work->mode = ITEM_BOX_WAIT;
        GROWTH(work) = 1.0F;
    }
}

/* a player's or an attack's touch opens the box when it is on the box's
 * top, or when it damages the box. a player that touched the top bounces
 * up, while off the ground. the item is given when player 0 opened the box.
 * in E-102's game, or with Tails as player 0 and player 1 not Sonic, it is
 * given whoever opened it */
static void item_box_wait(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *hit = work->collision->hit;

    if (edit_mode) {
        return;
    }
    if (work->collision->flags & COLLISION_HIT) {
        if (hit->kind == COLLISION_PLAYER || hit->kind == COLLISION_ATTACK) {
            /* @hack spelling: player_get_task called through a local, whose
             * address SHC loads once, into R13 (MOV.L at 0x8c0605de). called
             * by name, each call loads it again, and 140 of 144 words
             * differ */
            struct task *(*get_task)(Uint8 player) = player_get_task;

            if (work->collision->own_shape == 1) {
                work->mode = ITEM_BOX_OPEN;
                if (work->collision->hit->owner == get_task(0) &&
                    !(player_work[0]->flags & PLAYER_ON_LAND)) {
                    player_set_speed(0, 0.0F, 2.0F, 0.0F);
                } else if (work->collision->hit->owner == get_task(1) &&
                           /* @bug logic: player 1's bounce tests player
                            * 0's PLAYER_ON_LAND, as player 0's does (MOV.L
                            * @R0,R0 at 0x8c060670 reads player_work[0]).
                            * player 1 bounces only while player 0 is off
                            * the ground */
                           !(player_work[0]->flags & PLAYER_ON_LAND)) {
                    player_set_speed(1, 0.0F, 2.0F, 0.0F);
                }
            }
            if (work->flags & COLLISION_DAMAGED) {
                work->mode = ITEM_BOX_OPEN;
            }
            if (work->mode == ITEM_BOX_OPEN) {
                if (item_kinds[item_box_kind].give != NULL &&
                    (work->collision->hit->owner == get_task(0) ||
                     get_character() == CHARACTER_E102 ||
                     (player_get_character(0) == CHARACTER_TAILS &&
                      player_get_character(1) != CHARACTER_SONIC))) {
                    item_kinds[item_box_kind].give(work);
                    item_show(item_box_kind);
                }
            }
        }
    }
    collision_register(work);
    SPIN(work) += SPIN_STEP;
}

/* open the box. it bursts, unless its placement was used before, and marks
 * the placement used. then its lock-on goes, and its shapes become the
 * base's */
static void item_box_open(struct task *task)
{
    struct task_work *work = task->work;

    if (task->placement != NULL) {
        /* @hack spelling: an empty then-arm, which loads placement_set_used's
         * and lockon_release's addresses into R2 (MOV.L at 0x8c060722 and
         * 0x8c060734). as if (!placement_used(task)), they take R3 and 12 of
         * 43 words differ */
        if (placement_used(task)) {
        } else {
            placement_set_used(task);
            item_box_burst(work);
        }
    } else {
        item_box_burst(work);
    }
    lockon_release(task);
    collision_free(work);
    collision_init(task, item_box_open_shapes, COLLISION_OBJECT, 1, 2);
    work->mode = ITEM_BOX_OPENED;
}

/* draw the box, with its kind's icon turned SPIN degrees in the glass.
 * growing, it draws at scale GROWTH / GROWN. it first sets the character's
 * extra-life face in item_kinds */
static void item_box_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float scale;

    item_kinds[ITEM_1UP].texture = item_1up_textures[get_character()];
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    if (work->mode != ITEM_BOX_WAIT) {
        scale = GROWTH(work) * (1.0F / GROWN);
        njScale(NULL, scale, scale, scale);
    }
    draw_model(&item_box_base_model);
    njPushMatrix(NULL);
    draw_model(&item_box_glass_model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    draw_model(&item_box_top_model);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 7.5F, 0.0F);
    TURN(njRotateY, (Angle)(SPIN(work) * ANGLE_PER_DEGREE));
    item_box_kind = KIND(task->work);
    item_box_icon_model.mats->attr_texId = item_kinds[item_box_kind].texture;
    draw_model(&item_box_icon_model);
    njPopMatrix(2);
}

static void item_box_draw_open(struct task *task)
{
    struct task_work *work = task->work;

    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    draw_model(&item_box_base_model);
    njPopMatrix(1);
}

static void item_box_burst(struct task_work *work)
{
    NJS_POINT3 pos;

    pos = work->pos;
    pos.y += 5.0F;
    snow_create(&pos, &item_box_up, 1.0F);
}

static void item_box_destroy(struct task *task)
{
}

/* speed up the player that owns the touch that opened the box */
static void item_give_speed_up(struct task_work *work)
{
    struct collision_work *hit = work->collision->hit;

#ifdef GCC
    /* repairs the slot for E-102's shot. the image takes as the player the
     * byte at the shot's work + 8 (MOV.B @(8,R4),R0 at 0x8c0609b2), the low
     * byte of its shooter's work's address (MOV.L R4,@(8,R14) at
     * 0x8c0dcfb0). GCC gives a shot's speed up to the E-102 who fired it */
    struct task_work *shooter = e102_shot_shooter(hit->owner);

    player_give_speed_up(
        PLAYER_VARS(shooter != NULL ? shooter : hit->owner->work)->player);
#else
    player_give_speed_up(PLAYER_VARS(hit->owner->work)->player);
#endif
    if (get_character() == CHARACTER_E102) {
        sound_play(SE_E102_SPEED_UP, 0, 0, 0);
    }
    sound_play(SE_ITEM, 0, 0, 0);
}

static void item_give_5_rings(struct task_work *work)
{
    ring_add(5);
    if (get_character() == CHARACTER_E102) {
        sound_play(SE_E102_ITEM, 0, 0, 0);
    }
    sound_play(SE_ITEM, 0, 0, 0);
}

static void item_give_10_rings(struct task_work *work)
{
    ring_add(10);
    if (get_character() == CHARACTER_E102) {
        sound_play(SE_E102_ITEM, 0, 0, 0);
    }
    sound_play(SE_ITEM, 0, 0, 0);
}

static void item_give_random_rings(struct task_work *work)
{
    item_rings_given = random_ring_counts[(int)(rand() * (6 * RAND_SCALE))];
    ring_add(item_rings_given);
    if (get_character() == CHARACTER_E102) {
        sound_play(SE_E102_ITEM, 0, 0, 0);
    }
    sound_play(SE_ITEM, 0, 0, 0);
}

static void item_give_1up(struct task_work *work)
{
    lives_add(1);
    if (get_character() == CHARACTER_E102) {
        sound_play(SE_E102_ITEM, 0, 0, 0);
    }
    sound_play(SE_ITEM, 0, 0, 0);
}

static void item_give_invincibility(struct task_work *work)
{
    player_give_invincibility(0);
    if (get_character() == CHARACTER_E102) {
        sound_play(SE_E102_ITEM, 0, 0, 0);
    }
    sound_play(SE_ITEM, 0, 0, 0);
}

static void item_give_barrier(struct task_work *work)
{
    player_give_barrier(0);
    if (get_character() == CHARACTER_E102) {
        sound_play(SE_E102_ITEM, 0, 0, 0);
    }
    sound_play(SE_ITEM, 0, 0, 0);
}

static void item_give_bomb(struct task_work *work)
{
    if (get_character() == CHARACTER_E102) {
        sound_play(SE_E102_ITEM, 0, 0, 0);
    }
    sound_play(SE_ITEM, 0, 0, 0);
    item_explosion_start(&work->pos);
}

static void item_give_magnetic_barrier(struct task_work *work)
{
    player_give_magnetic_barrier(0);
    if (get_character() == CHARACTER_E102) {
        sound_play(SE_E102_ITEM, 0, 0, 0);
    }
    sound_play(SE_ITEM, 0, 0, 0);
}
