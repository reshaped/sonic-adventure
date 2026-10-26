/* common: what the menus chose, the game's calls on the npc, battles and
 * title cards, and the starts and steps the objects share */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "object/capsule.h"
#include "object/common.h"
#include "object/item_box_air.h"
#include "object/panel.h"
#include "object/placement.h"
#include "object/rocket.h"
#include "object/switch.h"
#include "object/target.h"
#include "player/npc.h"
#include "player/player.h"
#include "player/rival.h"
#include "save/save.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

static void object_tree_destroy(struct task *task);
static void object_tree_draw(struct task *task);
static void model_destroy(struct task *task);
static void model_draw(struct task *task);
static void model_draw_scaled(struct task *task);
static void model_step_solid(struct task *task);
static void model_step_solid_scaled(struct task *task);
static void world_model_step(struct task *task);
static void world_model_draw(struct task *task);
static void world_model_destroy(struct task *task);
static void world_model_destroy_tree(struct task *task);
static void action_destroy(struct task *task);
static void action_step(struct task *task);
static void action_draw(struct task *task);

/* a carried thing sweeps a ball through the world, of SMALL_RADIUS or
 * LARGE_RADIUS by its step. the ball's center starts SMALL_CENTER or
 * LARGE_CENTER up from the thing's pos, in its frame. where the ball stops,
 * the thing's pos goes a radius below the center, to the ball's bottom */
#define SMALL_RADIUS 2.5F
#define SMALL_CENTER 2.48F
#define LARGE_RADIUS 5.0F
#define LARGE_CENTER 4.9F

/* a player within a world model's radius plus WORLD_MODEL_REACH of its
 * task has the world test the model */
#define WORLD_MODEL_REACH 16.0F

/* the npc npc_start starts, an index into npc_names */
static int npc_choice = 0;
static char *npc_names[] = {
    "1.5P",
    "BOSS SONIC vs KNUCKLES",
    "BOSS SONIC vs E102",
    "BOSS KNUCKLES vs SONIC",
    "BOSS KNUCKLES vs MILES",
    "BOSS E102 vs SONIC",
    "BOSS E102 vs MILES",
};

struct menu_state menu_state;
Sint32 dat_8c793fc4;
struct save menu_saves[SAVE_COUNT];
struct task *npc_task;
Bool npc_disabled;
static enum battle_outcome battle_result;
static Sint32 dat_8c794db4;
/* the root of the tree that draws the world models' copies, in one
 * draw_tree with the stage's texlist. it draws nothing itself, and carries
 * NJD_EVAL_BREAK while it has no child, as draw_tree requires */
static NJS_OBJECT *object_tree;
/* the root's last child, or the root while it has none */
static NJS_OBJECT *object_tree_last;

void npc_choose(Uint32 loc)
{
    int count = sizeof(npc_names) / sizeof(npc_names[0]);

    if (per_info[1]->press & PDD_DGT_KR) {
        npc_choice++;
        if (npc_choice >= count) {
            npc_choice = 0;
        }
    } else if (per_info[1]->press & PDD_DGT_KL) {
        npc_choice--;
        if (npc_choice < 0) {
            npc_choice = count - 1;
        }
    }
    njPrint(loc, "NPC [%s]", npc_names[npc_choice]);
}

void npc_start(int unused, struct task *player)
{
    struct battle battle;

    if (npc_choice == 0) {
        npc_create(player);
        return;
    }
    switch (npc_choice) {
    case 1:
        battle.kind = BATTLE_SONIC_KNUCKLES;
        break;
    case 2:
        battle.kind = BATTLE_SONIC_E102;
        break;
    case 3:
        battle.kind = BATTLE_KNUCKLES_SONIC;
        break;
    case 4:
        battle.kind = BATTLE_KNUCKLES_TAILS;
        break;
    case 5:
        battle.kind = BATTLE_E102_SONIC;
        break;
    case 6:
        battle.kind = BATTLE_E102_TAILS;
        break;
    default:
        /* npc_choose keeps npc_choice within npc_names. any other value
         * spins here forever (BRA to itself at 0x8c05b5e0) */
        while (1) {
        }
    }
    battle.player = player;
    battle.rival = NULL;
    battle.unk_0c = 0;
    battle.finish = NULL;
    rival_create(&battle);
    rival_begin(&battle);
}

int menu_get_story(int character)
{
    /* @hack spelling: the story through a local, which SHC loads into R4
     * (MOV.B @R3,R4 at 0x8c05b608); returned directly, it loads into R0 and
     * 2 of 4 words differ */
    Uint8 story = menu_state.story;

    return story;
}

Bool npc_leave(void)
{
    return npc_remove_tails();
}

struct task *npc_return(void)
{
    return npc_create_tails();
}

void npc_disable(void)
{
    npc_disabled = TRUE;
}

void npc_enable(void)
{
    npc_disabled = FALSE;
}

struct task *battle_begin(struct battle *battle)
{
    return rival_begin(battle);
}

struct task *battle_create_rival(struct battle *battle)
{
    return rival_create(battle);
}

void battle_remove_rival(struct battle *battle)
{
    rival_remove(battle);
}

void battle_set_outcome(enum battle_outcome outcome)
{
    battle_result = outcome;
}

enum battle_outcome battle_outcome(void)
{
    return battle_result;
}

Sint16 title_card_load(int frames)
{
    return title_card_begin(frames);
}

Sint16 title_card_draw(int unused)
{
    return title_card_step();
}

void boot_finish(void)
{
    fill_bytes(&menu_state, 0, sizeof(menu_state));
    npc_task = NULL;
}

int menu_get_mode(void)
{
    return menu_state.mode;
}

struct save *menu_get_save(void)
{
    return &menu_saves[menu_state.save];
}

Bool carry_set_kind(struct task_work *work, struct task_motion *motion,
                    enum carry_kind kind)
{
    if (motion == NULL) {
        return FALSE;
    }
    switch (kind) {
    case CARRY_KIND_SMALL:
        CARRY_REACH(motion) = 5.0F;
        motion->gravity = 0.02F;
        work->flags |= CARRY_HOLDABLE | PLAYER_CAN_CARRY;
        break;
    case CARRY_KIND_MEDIUM:
        CARRY_REACH(motion) = 20.0F;
        motion->gravity = 0.1F;
        work->flags |= CARRY_HOLDABLE | PLAYER_CAN_CARRY | PLAYER_CAN_PUSH;
        break;
    case CARRY_KIND_LARGE:
        CARRY_REACH(motion) = 30.0F;
        motion->gravity = 0.2F;
        work->flags |= CARRY_HOLDABLE | PLAYER_CAN_CARRY | PLAYER_CAN_PUSH;
        break;
    case CARRY_KIND_SHAKE:
        CARRY_REACH(motion) = 5.0F;
        motion->gravity = 1.0F;
        work->flags |= CARRY_HOLDABLE | PLAYER_CAN_SHAKE;
        break;
    default:
        return FALSE;
    }
    work->id = kind;
    return TRUE;
}

void carry_step_small(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    NJS_VECTOR speed;
    NJS_VECTOR offset;
    NJS_POINT3 pos;
    struct angles ang;
    int hit;

    if (!(work->flags & CARRY_HELD)) {
        if (work->flags & CARRY_GROUND) {
            motion->accel.y = 0.0F;
            motion->speed.y = 0.0F;
        } else {
            motion->accel.y = -motion->gravity;
        }
        if (!(work->flags & CARRY_GROUND)) {
            motion->speed.x *= 0.996F;
            motion->speed.z *= 0.996F;
        } else {
            motion->speed.x *= 0.1F;
            motion->speed.z *= 0.1F;
            if (njAbs(motion->speed.x) <= 1.0F) {
                motion->speed.x = 0.0F;
            }
            if (njAbs(motion->speed.z) <= 1.0F) {
                motion->speed.z = 0.0F;
            }
        }
        motion->speed.y += motion->accel.y;
        if (motion->speed.y > 6.0F) {
            motion->speed.y = 6.0F;
        } else if (motion->speed.y < -6.0F) {
            motion->speed.y = -6.0F;
        }
        pos = work->pos;
        speed.x = motion->speed.x + motion->accel.x;
        speed.y = motion->speed.y + motion->accel.y;
        speed.z = motion->speed.z + motion->accel.z;
        ang = work->ang;
        offset.x = 0.0F;
        offset.y = SMALL_CENTER;
        offset.z = 0.0F;
        object_turn(work, &offset);
        njAddVector(&pos, &offset);
        if (!placement_editing(task)) {
            hit =
                world_sweep_ball(&pos, &speed, &ang, WORLD_WATER, SMALL_RADIUS);
            if (((work->flags & CARRY_WALLS) && (hit & HIT_GROUND)) ||
                (!(work->flags & CARRY_WALLS) && hit)) {
                work->pos.x = pos.x + speed.x;
                work->pos.y = pos.y + speed.y;
                work->pos.z = pos.z + speed.z;
                work->ang = ang;
                offset.x = 0.0F;
                offset.y = SMALL_RADIUS;
                offset.z = 0.0F;
                object_turn(work, &offset);
                njSubVector(&work->pos, &offset);
                work->flags |= CARRY_GROUND;
            } else {
                if ((work->flags & CARRY_WALLS) && (hit & HIT_WALL)) {
                    work->pos.x = pos.x + speed.x;
                    work->pos.y = pos.y + speed.y;
                    work->pos.z = pos.z + speed.z;
                    offset.x = 0.0F;
                    offset.y = SMALL_RADIUS;
                    offset.z = 0.0F;
                    object_turn(work, &offset);
                    njSubVector(&work->pos, &offset);
                    motion->speed.x *= 0.1F;
                    motion->speed.z *= 0.1F;
                } else {
                    work->pos.x += motion->speed.x;
                    work->pos.y += motion->speed.y;
                    work->pos.z += motion->speed.z;
                }
                work->ang.x = 0;
                work->ang.z = 0;
                work->flags &= ~CARRY_GROUND;
            }
        }
    } else {
        work->flags &= ~CARRY_GROUND;
    }
    if (!(work->flags & CARRY_HELD) && !placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, motion);
    }
}

void carry_step_large(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;
    NJS_VECTOR speed;
    NJS_VECTOR offset;
    NJS_POINT3 pos;
    struct angles ang;
    Float fall;
    int hit;

    if (!(work->flags & CARRY_HELD)) {
        if (work->flags & CARRY_GROUND) {
            fall = 0.0F;
            motion->speed.y = 0.0F;
        } else {
            fall = -motion->gravity;
        }
        if (!(work->flags & CARRY_GROUND)) {
            motion->speed.x *= 0.98F;
            motion->speed.z *= 0.98F;
        } else {
            motion->speed.x *= 0.1F;
            motion->speed.z *= 0.1F;
            if (njAbs(motion->speed.x) <= 1.0F) {
                motion->speed.x = 0.0F;
            }
            if (njAbs(motion->speed.z) <= 1.0F) {
                motion->speed.z = 0.0F;
            }
        }
        motion->speed.y += fall;
        if (motion->speed.y > 6.0F) {
            motion->speed.y = 6.0F;
        } else if (motion->speed.y < -6.0F) {
            motion->speed.y = -6.0F;
        }
        if (!(work->flags & CARRY_GROUND) || !frame_step) {
            pos = work->pos;
            speed.x = motion->speed.x + motion->accel.x;
            speed.y = motion->speed.y + motion->accel.y;
            speed.z = motion->speed.z + motion->accel.z;
            ang = work->ang;
            offset.x = 0.0F;
            offset.y = LARGE_CENTER;
            offset.z = 0.0F;
            object_turn(work, &offset);
            njAddVector(&pos, &offset);
            if (!placement_editing(task)) {
                hit = world_sweep_ball(&pos, &speed, &ang, WORLD_WATER,
                                       LARGE_RADIUS);
                if (((work->flags & CARRY_WALLS) && (hit & HIT_GROUND)) ||
                    (!(work->flags & CARRY_WALLS) && hit)) {
                    work->pos.x = pos.x + speed.x;
                    work->pos.y = pos.y + speed.y;
                    work->pos.z = pos.z + speed.z;
                    work->ang = ang;
                    offset.x = 0.0F;
                    offset.y = LARGE_RADIUS;
                    offset.z = 0.0F;
                    object_turn(work, &offset);
                    njSubVector(&work->pos, &offset);
                    work->flags |= CARRY_GROUND;
                } else {
                    if ((work->flags & CARRY_WALLS) && (hit & HIT_WALL)) {
                        work->pos.x = pos.x + speed.x;
                        work->pos.y = pos.y + speed.y;
                        work->pos.z = pos.z + speed.z;
                        offset.x = 0.0F;
                        offset.y = LARGE_RADIUS;
                        offset.z = 0.0F;
                        object_turn(work, &offset);
                        njSubVector(&work->pos, &offset);
                    } else {
                        work->pos.x += motion->speed.x;
                        work->pos.y += motion->speed.y;
                        work->pos.z += motion->speed.z;
                    }
                    work->ang.x = 0;
                    work->ang.z = 0;
                    work->flags &= ~CARRY_GROUND;
                }
            }
        }
    } else {
        work->flags &= ~CARRY_GROUND;
    }
    if (!(work->flags & CARRY_HELD) && !placement_editing(task)) {
        collision_register(work);
    }
    object_clear_touch(work, motion);
}

void carry_step_still(struct task *task)
{
    struct task_work *work = task->work;
    struct task_motion *motion = task->motion;

    collision_register(work);
    object_clear_touch(work, motion);
}

void object_clear_touch(struct task_work *work, struct task_motion *motion)
{
    work->flags &= ~COLLISION_TOUCHED;
    if (motion != NULL) {
        motion->accel.x = motion->accel.y = motion->accel.z = 0.0F;
    }
}

void object_turn_back(struct task_work *work, NJS_VECTOR *vector)
{
    NJS_VECTOR from;

    from.x = vector->x;
    from.y = vector->y;
    from.z = vector->z;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateY, -work->ang.y);
    TURN(njRotateX, -work->ang.x);
    TURN(njRotateZ, -work->ang.z);
    njCalcVector(NULL, &from, vector);
    njPopMatrix(1);
}

void object_turn(struct task_work *work, NJS_VECTOR *vector)
{
    NJS_VECTOR from;

    from.x = vector->x;
    from.y = vector->y;
    from.z = vector->z;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njCalcVector(NULL, &from, vector);
    njPopMatrix(1);
}

void object_init(void)
{
    NJS_OBJECT *root;
    struct task *task;

    object_tree = NULL;
    object_tree_last = NULL;
    root = heap_calloc(1, sizeof(NJS_OBJECT));
    if (root == NULL) {
        return;
    }
    object_tree = root;
    object_tree_last = root;
    root->evalflags = NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG |
                      NJD_EVAL_UNIT_SCL | NJD_EVAL_HIDE | NJD_EVAL_BREAK;
    root->child = NULL;
    root->sibling = NULL;
    root->model = NULL;
    /* @hack spelling: task assigned in the test, which tests task_create's R0
     * with the copy to R4 in the BT/S's slot (0x8c05bf46, 0x8c05bf4a);
     * assigned apart, SHC tests R4 and 3 of 60 words differ */
    if ((task = task_create(0, 3, object_tree_draw)) != NULL) {
        task->destructor = object_tree_destroy;
        task->draw = object_tree_draw;
    }
    placement_reset();
    panel_reset();
    rocket_reset();
    capsule_reset();
    target_reset();
    switch_reset();
    item_reset();
}

/* the tree's task's destructor: free the root's children, then the root,
 * leaving object_tree and object_tree_last null */
static void object_tree_destroy(struct task *task)
{
    NJS_OBJECT *object;

    object_tree_last = NULL;
    while (object_tree != NULL) {
        object = object_tree->child;
        if (object != NULL) {
            object_tree->child = object->sibling;
        } else {
            object = object_tree;
            object_tree = NULL;
        }
        heap_free(object);
    }
}

static void object_tree_draw(struct task *task)
{
    texture_set_stage();
    draw_tree(object_tree);
}

/* append object to the root's children, and point work's world model at
 * it. the root's first child clears its NJD_EVAL_BREAK */
static void object_tree_add(struct task_work *work, NJS_OBJECT *object)
{
    if (object_tree->child == NULL) {
        object_tree->child = object;
        object_tree->evalflags &= ~NJD_EVAL_BREAK;
    } else {
        object_tree_last->sibling = object;
    }
    WORLD_MODEL(work)->model = object;
    object_tree_last = object;
}

/* add a copy of work's world object to the tree, from the heap. returns
 * FALSE when there's no tree or no heap room, else TRUE */
static Bool object_tree_add_copy(struct task_work *work)
{
    NJS_OBJECT *object;

    if (object_tree == NULL) {
        return FALSE;
    }
    /* @hack spelling: object assigned in the test, which tests heap_alloc's R0
     * with the copy to R5 in the BF/S's slot (0x8c05c084, 0x8c05c088);
     * assigned apart, SHC tests R5 and 3 of 52 words differ */
    if ((object = heap_alloc(sizeof(NJS_OBJECT))) == NULL) {
        return FALSE;
    }
    *object = *WORLD_MODEL(work)->world;
    object_tree_add(work, object);
    return TRUE;
}

/* take object out of the tree and free it. object must be one of the
 * root's children: the walk for it doesn't test for the end of the list.
 * with the last child gone, the root carries NJD_EVAL_BREAK again */
static void object_tree_remove(NJS_OBJECT *object)
{
    NJS_OBJECT *prev = object_tree;

    if (prev == NULL) {
        return;
    }
    if (object == prev->child) {
        prev->child = object->sibling;
    } else {
        prev = prev->child;
        while (prev->sibling != object) {
            prev = prev->sibling;
        }
        prev->sibling = object->sibling;
    }
    if (object_tree_last == object) {
        object_tree_last = prev;
    }
    heap_free(object);
    if (object_tree->child == NULL) {
        object_tree->evalflags |= NJD_EVAL_BREAK;
    }
}

Bool model_start(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return TRUE;
    }
    if (texture_has_primary_stage_texlist()) {
        MODEL(work)->texlist = NULL;
    } else {
        njLoadTexture(MODEL(work)->texlist);
    }
    if (texture_has_primary_stage_texlist() && !placement_editing(task)) {
        task->step = model_draw;
        task->draw = model_draw;
        task->destructor = model_destroy;
    }
    model_draw(task);
    return FALSE;
}

static void model_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (MODEL(work)->texlist != NULL) {
        njReleaseTexture(MODEL(work)->texlist);
    }
}

static void model_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!texture_set_stage()) {
        njSetTexture(MODEL(work)->texlist);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
}

Bool model_start_scaled(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return TRUE;
    }
    if (texture_has_primary_stage_texlist()) {
        MODEL(work)->texlist = NULL;
    } else {
        njLoadTexture(MODEL(work)->texlist);
    }
    if (texture_has_primary_stage_texlist() && !placement_editing(task)) {
        task->step = model_draw_scaled;
        task->draw = model_draw_scaled;
        task->destructor = model_destroy;
    }
    model_draw_scaled(task);
    return FALSE;
}

static void model_draw_scaled(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!texture_set_stage()) {
        njSetTexture(MODEL(work)->texlist);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, work->scl.x + 1.0F, work->scl.y + 1.0F, work->scl.z + 1.0F);
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
}

Bool model_start_solid(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return TRUE;
    }
    if (texture_has_primary_stage_texlist()) {
        MODEL(work)->texlist = NULL;
    } else {
        njLoadTexture(MODEL(work)->texlist);
    }
    if (texture_has_primary_stage_texlist() && !placement_editing(task)) {
        task->destructor = model_destroy;
        task->step = model_step_solid;
        task->draw = model_draw;
    }
    model_draw(task);
    return FALSE;
}

static void model_step_solid(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
    if (task->destructor == model_destroy) {
        model_draw(task);
    }
}

Bool model_start_solid_scaled(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return TRUE;
    }
    if (texture_has_primary_stage_texlist()) {
        MODEL(work)->texlist = NULL;
    } else {
        njLoadTexture(MODEL(work)->texlist);
    }
    if (texture_has_primary_stage_texlist() && !placement_editing(task)) {
        task->destructor = model_destroy;
        task->step = model_step_solid_scaled;
        task->draw = model_draw_scaled;
    }
    model_draw_scaled(task);
    return FALSE;
}

static void model_step_solid_scaled(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_register(work);
        object_clear_touch(work, NULL);
    }
    if (task->destructor == model_destroy) {
        model_draw_scaled(task);
    }
}

Bool model_start_world(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;
    Sint32 flags;

    if (placement_try_leave(task)) {
        return TRUE;
    }
    if (texture_has_primary_stage_texlist()) {
        WORLD_MODEL(work)->texlist = NULL;
    } else {
        njLoadTexture(WORLD_MODEL(work)->texlist);
    }
    if (!placement_editing(task)) {
        object = world_object_alloc();
        WORLD_MODEL(work)->world = object;
        object->evalflags = WORLD_MODEL(work)->model->evalflags &
                            ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
        object->scl[0] = 1.0F;
        object->scl[1] = 1.0F;
        object->scl[2] = 1.0F;
        object->ang[0] = work->ang.x;
        object->ang[1] = work->ang.y;
        object->ang[2] = work->ang.z;
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->model = WORLD_MODEL(work)->model->model;
        object->child = WORLD_MODEL(work)->model->child;
        object->sibling = WORLD_MODEL(work)->model->sibling;
        flags = WORLD_HUGE | WORLD_SOLID;
        /* @hack spelling: an empty arm for a radius over 20, where flags keeps
         * the value set above, which SHC sets in the BT/S's slot (MOV #1,R12
         * at 0x8c05c606); with the store in that arm, SHC branches round it,
         * the function runs 2 bytes long and 58 of 134 words differ */
        if (object->model->r > 20.0F) {
        } else if (object->model->r > 10.0F) {
            flags = WORLD_LARGE | WORLD_SOLID;
        } else if (object->model->r > 3.0F) {
            flags = WORLD_MEDIUM | WORLD_SOLID;
        } else {
            flags = WORLD_SMALL | WORLD_SOLID;
        }
        world_add(flags, task, object);
    }
    if (texture_has_primary_stage_texlist() &&
        WORLD_MODEL(work)->world != NULL && object_tree_add_copy(work)) {
        task->destructor = world_model_destroy_tree;
        task->step = world_model_step;
    } else {
        task->destructor = world_model_destroy;
        task->step = world_model_step;
        task->draw = world_model_draw;
    }
    /* @hack redundant: a return of flags that never runs, which keeps flags in
     * R12, a saved register (0x8c05c606); without it, flags lives in R4 and 130
     * of 134 words differ */
    if (0) {
        return flags;
    }
    return FALSE;
}

/* pose the task's world object, if it has one, as the task. WORLD_TESTED
 * is then set while a player is within the model's radius plus
 * WORLD_MODEL_REACH of the task, else clear. a task the tree doesn't draw
 * draws itself here */
static void world_model_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object;

    if (placement_try_leave(task)) {
        return;
    }
    object = WORLD_MODEL(work)->world;
    if (object != NULL) {
        object->pos[0] = work->pos.x;
        object->pos[1] = work->pos.y;
        object->pos[2] = work->pos.z;
        object->ang[0] = work->ang.x;
        object->ang[1] = work->ang.y;
        object->ang[2] = work->ang.z;
        if (player_in_range(&work->pos, object->model->r + WORLD_MODEL_REACH)) {
            work->flags |= WORLD_TESTED;
        } else {
            work->flags &= ~WORLD_TESTED;
        }
    }
    if (task->destructor == world_model_destroy) {
        world_model_draw(task);
    }
}

static void world_model_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!texture_set_stage()) {
        njSetTexture(WORLD_MODEL(work)->texlist);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateX, work->ang.x);
    draw_tree(WORLD_MODEL(work)->model);
    njPopMatrix(1);
}

static void world_model_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (WORLD_MODEL(work)->texlist != NULL) {
        njReleaseTexture(WORLD_MODEL(work)->texlist);
    }
    if (WORLD_MODEL(work)->world != NULL) {
        world_remove(task, WORLD_MODEL(work)->world);
        world_object_free(WORLD_MODEL(work)->world);
    }
}

static void world_model_destroy_tree(struct task *task)
{
    struct task_work *work = task->work;

    if (WORLD_MODEL(work)->texlist != NULL) {
        njReleaseTexture(WORLD_MODEL(work)->texlist);
    }
    if (WORLD_MODEL(work)->world != NULL) {
        world_remove(task, WORLD_MODEL(work)->world);
        world_object_free(WORLD_MODEL(work)->world);
    }
    object_tree_remove(WORLD_MODEL(work)->model);
}

Bool model_start_action(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave(task)) {
        return TRUE;
    }
    if (texture_has_primary_stage_texlist()) {
        ACTION(work)->texlist = NULL;
    } else {
        njLoadTexture(ACTION(work)->texlist);
    }
    task->destructor = action_destroy;
    task->step = action_step;
    task->draw = action_draw;
    return FALSE;
}

static void action_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (ACTION(work)->texlist != NULL) {
        njReleaseTexture(ACTION(work)->texlist);
    }
}

static void action_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave(task)) {
        action_draw(task);
        ACTION(work)->frame += 1.0F;
        if (ACTION(work)->frame >=
            (Float)ACTION(work)->action->motion->nbFrame) {
            ACTION(work)->frame = 0.0F;
        }
    }
}

static void action_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (!texture_set_stage()) {
        njSetTexture(ACTION(work)->texlist);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    draw_action(ACTION(work)->action, ACTION(work)->frame);
    njPopMatrix(1);
}

void object_scale_shapes(struct task_work *work)
{
    Uint16 count = work->collision->count;
    Uint16 i;
    struct collision_shape *shape;

    for (i = 0; i < count; i++) {
        if (work->scl.x <= -1.0F) {
            work->scl.x = -1.0F;
        }
        if (work->scl.y <= -1.0F) {
            work->scl.y = -1.0F;
        }
        if (work->scl.z <= -1.0F) {
            work->scl.z = -1.0F;
        }
        /* @hack spelling: the shapes read, then offset by i, which reads
         * work->collision->shapes on each pass (MOV.L @(56,R4),R5 and
         * MOV.L @(12,R5),R5 at 0x8c05ca06 and 0x8c05ca0c); as &shapes[i],
         * SHC reads it once before the loop, the function runs 6 bytes short
         * and 79 of 81 words differ */
        shape = work->collision->shapes;
        shape += i;
        shape->radius = shape->radius * (work->scl.x + 1.0F);
        shape->size_y = shape->size_y * (work->scl.y + 1.0F);
        shape->size_z = shape->size_z * (work->scl.z + 1.0F);
        shape->center.x = shape->center.x * (work->scl.x + 1.0F);
        shape->center.y = shape->center.y * (work->scl.y + 1.0F);
        shape->center.z = shape->center.z * (work->scl.z + 1.0F);
    }
}

NJS_OBJECT *object_add_world(NJS_OBJECT *model, struct task *task, Sint32 flags)
{
    struct task_work *work = task->work;
    NJS_POINT3 *pos = &work->pos;
    struct angles *ang = &work->ang;
    NJS_OBJECT *object = world_object_alloc();

    object->evalflags =
        model->evalflags & ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
    object->model = model->model;
    object->child = model->child;
    object->sibling = model->sibling;
    object->pos[0] = pos->x;
    object->pos[1] = pos->y;
    object->pos[2] = pos->z;
    object->ang[0] = ang->x;
    object->ang[1] = ang->y;
    object->ang[2] = ang->z;
    world_add(flags, task, object);
    return object;
}
