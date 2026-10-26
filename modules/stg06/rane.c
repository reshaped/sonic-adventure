/* the deck's sections, O RANE A to F and O ANA A and B. four kinds carry a
 * lift. a section whose scale y is below 0 breaks away once player 0 comes
 * near. it rumbles as its panels swing open, then drops, holds and sinks
 * until it explodes */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/explosion.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg06/aclift.h"
#include "stg06/rane.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT rane_a_object;
extern NJS_OBJECT rane_b_object;
extern NJS_OBJECT rane_c_object;
extern NJS_OBJECT rane_d_object;
extern NJS_OBJECT rane_e_object;
extern NJS_OBJECT rane_f_object;
extern NJS_OBJECT ana_a_object;
extern NJS_OBJECT ana_b_object;
extern NJS_OBJECT rane_a_world_object;
extern NJS_OBJECT rane_c_world_object;
extern NJS_OBJECT rane_d_world_object;
extern NJS_OBJECT rane_e_world_object;
extern NJS_OBJECT rane_f_world_object;
extern NJS_OBJECT ana_a_world_object;
extern NJS_OBJECT rane_panel_object;

/* the square of the distance from the camera or player 0 within which a
 * section stays */
#define RANE_RANGE (3010.0F * 3010.0F)
/* the world tests a section while a player is within RANE_NEAR past its
 * world model's radius */
#define RANE_NEAR 50.0F
#define RANE_FLAGS (WORLD_LARGE | WORLD_MOVES)

/* a section that breaks away starts to rumble once player 0 is within
 * RUMBLE_NEAR of it */
#define RUMBLE_NEAR 300.0F
/* a section rumbles, drops and holds in turn, each until its timer passes
 * the mode's count. it drops DROP_SPEED a step */
#define RUMBLE_STEPS 30
#define DROP_STEPS 10
#define HOLD_STEPS 60
#define DROP_SPEED 2.0F
/* a section then sinks SINK_SLOW a step, or SINK_FAST below a height of
 * SINK_DEEP. below SINK_GONE it is gone, in an explosion of scale
 * GONE_BLAST */
#define SINK_SLOW 0.5F
#define SINK_FAST 4.0F
#define SINK_DEEP (-80.0F)
#define SINK_GONE (-350.0F)
#define GONE_BLAST 15.0F
/* the sounds a section plays as it starts to rumble, as it drops and as it
 * is gone */
#define SE_RUMBLE 160
#define SE_DROP 176
#define SE_GONE 177
#define RANE_VOLUME 64

/* while its section rumbles, a panel turns its yaw toward PANEL_OPEN by
 * PANEL_SPEED a step. it also sinks PANEL_SINK a step */
#define PANEL_OPEN NJM_DEG_ANG(90)
#define PANEL_SPEED NJM_DEG_ANG(1.5)
#define PANEL_SINK 0.1F

struct rane_vars {
    NJS_OBJECT *world;
    NJS_OBJECT *object;
};

#define VARS(work) ((struct rane_vars *)&(work)->vars)

/* a section's work->mode, which each of its panels copies on its own step */
enum rane_mode {
    RANE_INIT,
    RANE_WAIT,
    RANE_RUMBLE,
    RANE_DROP,
    RANE_HOLD,
    RANE_SINK,
    RANE_STOP
};

/* the kinds, by the submode each kind's step sets */
enum {
    RANE_A,
    RANE_B,
    RANE_C,
    RANE_D,
    RANE_E,
    RANE_F,
    ANA_A,
    ANA_B,
    RANE_KINDS
};

/* a kind of section: what it draws, and whether it carries a lift and
 * where */
struct rane_kind {
    NJS_OBJECT *object;
    Bool lift;
    NJS_VECTOR lift_pos; /* the lift's offset from the section */
};

static void rane_panel_step(struct task *task);

static struct rane_kind rane_kinds[RANE_KINDS] = {
    {&rane_a_object, FALSE, {0.0F, 0.0F, 0.0F}},
    {&rane_b_object, FALSE, {0.0F, 0.0F, 0.0F}},
    {&rane_c_object, FALSE, {0.0F, 0.0F, 0.0F}},
    {&rane_d_object, TRUE, {0.0F, 99.0F, -38.0F}},
    {&rane_e_object, FALSE, {0.0F, 0.0F, 0.0F}},
    {&rane_f_object, TRUE, {-90.0F, 69.0F, -38.0F}},
    {&ana_a_object, TRUE, {30.0F, 99.0F, -38.0F}},
    {&ana_b_object, TRUE, {0.0F, 79.0F, -38.0F}},
};

/* each kind's world shape. O RANE B's and O ANA B's are the trees they
 * draw */
static NJS_OBJECT *rane_world_objects[RANE_KINDS] = {
    &rane_a_world_object, &rane_b_object,       &rane_c_world_object,
    &rane_d_world_object, &rane_e_world_object, &rane_f_world_object,
    &ana_a_world_object,  &ana_b_object,
};

static struct collision_shape rane_panel_shapes[1] = {
    {SHAPE_CYLINDER,
     0,
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 2.0F, 0.0F},
     3.86F,
     2.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

/* a panel's spec: the panel stands at x, y, z from its section. the spec's var,
 * the panel's tree, becomes the work's first var, which rane_vars names world.
 * rane_destroy is a panel's destructor too. it finds that tree neither in the
 * world nor in world_object_alloc's pool, so it changes nothing. NO_PANEL ends
 * a table of specs */
#define PANEL_TREE ((Sint32)(&rane_panel_object))
#define PANEL(x, y, z)                                                         \
    {rane_panel_step, TASK_WORK, 0, {0, 0, 0}, {(x), (y), (z)}, PANEL_TREE}
#define NO_PANEL {NULL, 0, 0, {0, 0, 0}, {0.0F, 0.0F, 0.0F}, 0}

static struct task_spec rane_panels_a[3] = {
    PANEL(95.0F, 80.0F, 115.0F),
    PANEL(95.0F, 80.0F, -115.0F),
    NO_PANEL,
};
static struct task_spec rane_panels_b[5] = {
    PANEL(-65.0F, 80.0F, 115.0F),
    PANEL(-65.0F, 80.0F, -115.0F),
    PANEL(65.0F, 80.0F, 115.0F),
    PANEL(65.0F, 80.0F, -115.0F),
    NO_PANEL,
};
static struct task_spec rane_panels_c[3] = {
    PANEL(-155.0F, 50.0F, 115.0F),
    PANEL(-155.0F, 50.0F, -115.0F),
    NO_PANEL,
};

static struct dust_params rane_dust = {
    0.7F,
    0.07F,
    0.1F,
    0.06F,
    0.05F,
    {0.0F, 4.0F, 0.0F},
    {0.0F, 1.0F, 0.0F},
    {0.32F, 0.8F, 0.8F, 1.0F},
};

static void rane_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_tree(VARS(work)->object);
    njPopMatrix(1);
}

/* free the section's world object, unless the set editor holds the task.
 * freeing takes it out of the world and returns it to world_object_alloc's
 * pool. then destroy the section's children */
static void rane_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task) && VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        /* @bug memory: after rane_remove, this frees the world object a
         * second time, by the call at 0x0c90cee2. rane_remove frees the object
         * and leaves world set, and the task it kills is destroyed on its next
         * step */
        world_object_free(VARS(work)->world);
    }
    task_destroy_children(task);
}

/* do as rane_destroy does, then placement_kill the task. its placement then
 * makes no more, and the task is destroyed on its next step. world stays set */
static void rane_remove(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task) && VARS(work)->world != NULL) {
        world_remove(task, VARS(work)->world);
        world_object_free(VARS(work)->world);
    }
    task_destroy_children(task);
    placement_kill(task);
}

static void rane_add_world(struct task *task, int kind)
{
    struct task_work *work = task->work;
    NJS_OBJECT *shape = rane_world_objects[kind];
    NJS_OBJECT *world;

    VARS(work)->world = world = world_object_alloc();
    world->evalflags =
        shape->evalflags & ~(NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG);
    world->scl[0] = world->scl[1] = world->scl[2] = 1.0F;
    world->ang[0] = work->ang.x;
    world->ang[1] = work->ang.y;
    world->ang[2] = work->ang.z;
    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    world->model = shape->model;
    world->child = shape->child;
    world->sibling = shape->sibling;
    world_add(RANE_FLAGS, task, world);
}

static void rane_update_world(struct task_work *work)
{
    NJS_OBJECT *world = VARS(work)->world;

    world->pos[0] = work->pos.x;
    world->pos[1] = work->pos.y;
    world->pos[2] = work->pos.z;
    world->ang[0] = work->ang.x;
    world->ang[1] = work->ang.y;
    world->ang[2] = work->ang.z;
    if (player_in_range(&work->pos, VARS(work)->world->model->r + RANE_NEAR)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
}

static void rane_step(struct task *task)
{
    struct task_work *work = task->work;
    struct rane_kind *kind;
    Uint8 number;

    switch (work->mode) {
    case RANE_INIT:
        work->mode = RANE_WAIT;
        number = work->submode;
        work->ang.x = work->ang.y = work->ang.z = 0;
        task->destructor = rane_destroy;
        task->draw = rane_draw;
        /* @hack spelling: kind set inside the store of its object, where SHC
         * keeps it on the stack (MOV.L R3,@R15 at 0x0c90d128, read back at
         * 0x0c90d134); set apart, it takes a register and 162 of 345 words
         * differ */
        VARS(work)->object = (kind = &rane_kinds[number])->object;
        rane_add_world(task, number);
        if (kind->lift == TRUE) {
            if (task_create_child(TASK_WORK, aclift_step, task) != NULL) {
                njAddVector(&task->child->work->pos,
                            &rane_kinds[number].lift_pos);
                task->child->work->scl.z = work->scl.z;
                task->child->work->ang.y += 0x8000;
            }
        }
        switch (number) {
        case RANE_A:
        case ANA_A:
            task_create_children(rane_panels_a, task);
            break;
        case RANE_C:
        case RANE_D:
            task_create_children(rane_panels_b, task);
            break;
        case RANE_E:
        case RANE_F:
            task_create_children(rane_panels_c, task);
            break;
        }
        break;
    case RANE_WAIT:
        rane_update_world(work);
        if (work->scl.y < 0.0F) {
            if (njDistanceP2P(&work->pos, &player_work[0]->pos) < RUMBLE_NEAR) {
                sound_play_at(SE_RUMBLE, 0, 0, RANE_VOLUME, work->pos.x,
                              work->pos.y, work->pos.z);
                work->mode = RANE_RUMBLE;
            }
        }
        rane_draw(task);
        task_step_children(task);
        break;
    case RANE_RUMBLE:
        if (++TASK_TIMER(work) > RUMBLE_STEPS) {
            sound_play_at(SE_DROP, 0, 0, RANE_VOLUME, work->pos.x, work->pos.y,
                          work->pos.z);
            work->mode = RANE_DROP;
            work->timer = 0;
        }
        rane_update_world(work);
        draw_highlight_near(work, ALL_CHARACTERS);
        rane_draw(task);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        task_step_children(task);
        break;
    case RANE_DROP:
        if (++TASK_TIMER(work) > DROP_STEPS) {
            work->mode = RANE_HOLD;
            work->timer = 0;
        } else {
            work->pos.y -= DROP_SPEED;
        }
        rane_update_world(work);
        draw_highlight_near(work, ALL_CHARACTERS);
        rane_draw(task);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        task_step_children(task);
        break;
    case RANE_HOLD:
        if (++TASK_TIMER(work) > HOLD_STEPS) {
            work->mode = RANE_SINK;
            work->timer = 0;
        }
        rane_update_world(work);
        draw_highlight_near(work, ALL_CHARACTERS);
        rane_draw(task);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        task_step_children(task);
        break;
    case RANE_SINK:
        if (work->pos.y < SINK_GONE) {
            explosion_create_at(work->pos.x, work->pos.y, work->pos.z,
                                GONE_BLAST);
            rane_remove(task);
            sound_play_at(SE_GONE, 0, 0, RANE_VOLUME, work->pos.x, work->pos.y,
                          work->pos.z);
        } else if (work->pos.y < SINK_DEEP) {
            work->pos.y -= SINK_FAST;
        } else {
            work->pos.y -= SINK_SLOW;
        }
        /* @bug memory: on the step that calls rane_remove (0x0c90d338), this
         * writes the freed world object's pos and ang, and reads its model's
         * r, by the call at 0x0c90d37a. rane_remove frees the object but
         * leaves world set */
        rane_update_world(work);
        draw_highlight_near(work, ALL_CHARACTERS);
        rane_draw(task);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        task_step_children(task);
        break;
    case RANE_STOP:
        break;
    }
}

/* the steps O RANE A to F and O ANA A and B name. each puts its kind in
 * work->submode and runs rane_step while the section is in RANE_RANGE. in the
 * set editor, it only draws its kind's section, unturned */
void rane_a_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANE_RANGE)) {
        if (edit_mode) {
            if (VARS(work)->object == NULL) {
                VARS(work)->object = rane_kinds[RANE_A].object;
            }
            work->ang.x = work->ang.y = work->ang.z = 0;
            rane_draw(task);
        } else {
            work->submode = RANE_A;
            rane_step(task);
        }
    }
}

void rane_b_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANE_RANGE)) {
        if (edit_mode) {
            if (VARS(work)->object == NULL) {
                VARS(work)->object = rane_kinds[RANE_B].object;
            }
            work->ang.x = work->ang.y = work->ang.z = 0;
            rane_draw(task);
        } else {
            work->submode = RANE_B;
            rane_step(task);
        }
    }
}

void rane_c_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANE_RANGE)) {
        if (edit_mode) {
            if (VARS(work)->object == NULL) {
                VARS(work)->object = rane_kinds[RANE_C].object;
            }
            work->ang.x = work->ang.y = work->ang.z = 0;
            rane_draw(task);
        } else {
            work->submode = RANE_C;
            rane_step(task);
        }
    }
}

void rane_d_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANE_RANGE)) {
        if (edit_mode) {
            if (VARS(work)->object == NULL) {
                VARS(work)->object = rane_kinds[RANE_D].object;
            }
            work->ang.x = work->ang.y = work->ang.z = 0;
            rane_draw(task);
        } else {
            work->submode = RANE_D;
            rane_step(task);
        }
    }
}

void rane_e_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANE_RANGE)) {
        if (edit_mode) {
            if (VARS(work)->object == NULL) {
                VARS(work)->object = rane_kinds[RANE_E].object;
            }
            work->ang.x = work->ang.y = work->ang.z = 0;
            rane_draw(task);
        } else {
            work->submode = RANE_E;
            rane_step(task);
        }
    }
}

void rane_f_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANE_RANGE)) {
        if (edit_mode) {
            if (VARS(work)->object == NULL) {
                VARS(work)->object = rane_kinds[RANE_F].object;
            }
            work->ang.x = work->ang.y = work->ang.z = 0;
            rane_draw(task);
        } else {
            work->submode = RANE_F;
            rane_step(task);
        }
    }
}

void ana_a_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANE_RANGE)) {
        if (edit_mode) {
            if (VARS(work)->object == NULL) {
                VARS(work)->object = rane_kinds[ANA_A].object;
            }
            work->ang.x = work->ang.y = work->ang.z = 0;
            rane_draw(task);
        } else {
            work->submode = ANA_A;
            rane_step(task);
        }
    }
}

void ana_b_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANE_RANGE)) {
        if (edit_mode) {
            if (VARS(work)->object == NULL) {
                VARS(work)->object = rane_kinds[ANA_B].object;
            }
            work->ang.x = work->ang.y = work->ang.z = 0;
            rane_draw(task);
        } else {
            work->submode = ANA_B;
            rane_step(task);
        }
    }
}

/* a panel takes its section's mode each step. while the section rumbles, the
 * panel swings open, sinks and sheds dust. it drops with the section, then
 * sinks SINK_SLOW a step, even once the section sinks SINK_FAST */
static void rane_panel_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case RANE_INIT:
        work->mode = RANE_WAIT;
        task->destructor = rane_destroy;
        task->draw = rane_draw;
        VARS(work)->object = &rane_panel_object;
        collision_init(task, rane_panel_shapes, COLLISION_OBJECT, 1, 1);
        break;
    case RANE_WAIT:
        work->mode = task->parent->work->mode;
        collision_register(work);
        rane_draw(task);
        break;
    case RANE_RUMBLE:
        work->mode = task->parent->work->mode;
        work->ang.y = angle_approach(work->ang.y, PANEL_OPEN, PANEL_SPEED);
        work->pos.y -= PANEL_SINK;
        collision_register(work);
        rane_dust.speed.y = -world_gravity.y;
        rane_dust.pos = work->pos;
        dust_create_glow(&rane_dust);
        rane_draw(task);
        break;
    case RANE_DROP:
        work->mode = task->parent->work->mode;
        work->pos.y -= DROP_SPEED;
        collision_register(work);
        draw_highlight_near(work, ALL_CHARACTERS);
        rane_draw(task);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        break;
    case RANE_HOLD:
        work->mode = task->parent->work->mode;
        collision_register(work);
        draw_highlight_near(work, ALL_CHARACTERS);
        rane_draw(task);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        break;
    case RANE_SINK:
        work->mode = task->parent->work->mode;
        work->pos.y -= SINK_SLOW;
        collision_register(work);
        draw_highlight_near(work, ALL_CHARACTERS);
        rane_draw(task);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        break;
    case RANE_STOP:
        break;
    }
}
