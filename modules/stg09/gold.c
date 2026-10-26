/* O KIN: a heap of gold that rises as rings are deposited. O KINWALL: a
 * wall that stands until 400 rings are deposited */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "object/placement.h"
#include "player/player.h"
#include "stg09/casino.h"
#include "stg09/gold.h"
#include "stg09/helpers.h"
#include "stg09/sonic_statue.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT gold_heap_object;
extern NJS_MESHSET gold_heap_meshset_0;
extern NJS_MESHSET gold_heap_meshset_1;

/* the heap starts only within about 600 of the camera or player 0, and
 * destroys itself otherwise. placement_try_leave_by takes the range
 * squared */
#define RANGE ((600.0F * 600.0F) + 10.0F)
/* the heap runs and draws only while player 0 or 1 is within this radius */
#define NEAR 600.0F
/* the heap draws only while a sphere of this radius about it is in view */
#define VIEW_RADIUS 190.0F
/* the radius casino_world_add takes for the heap's copy in the collision
 * world. casino_world_test_near takes it plus how far the heap sits below
 * its position */
#define COPY_RADIUS 500.0F
/* the heap's box stands this much higher than gold_heap_shape places it,
 * and rises with the heap's height */
#define BOX_RISE 49.0F

/* the rings deposited that fill the heap. the wall stands while fewer are */
#define GOLD_FULL 400
/* the rings deposited that raise the Sonic statue to levels 1 to 4 */
#define GOLD_LEVEL_1 200
#define GOLD_LEVEL_2 1000
#define GOLD_LEVEL_3 2500
#define GOLD_LEVEL_4 5000

struct gold_heap_vars {
    NJS_OBJECT *object; /* the heap's copy in the collision world */
    Uint8 unk_04[4];
    /* the heap's offset up from its position: gold_heap_depth when empty, up
     * to 0 when full */
    Float height;
};

#define VARS(work) ((struct gold_heap_vars *)&(work)->vars)
/* the Sonic statue's level, 0 to 4 */
#define STATUE_LEVEL(work) ((work)->submode)

enum gold_wall_mode { WALL_INIT, WALL_RUN, WALL_EDITED };

static Float gold_heap_depth = -92.0F; /* an empty heap's height */
static int gold_heap_slide = 4; /* the UVs' scroll phase per unit of height */
/* the keeper's work, as gold.h defines the keeper, or NULL */
static struct task_work *gold_heap_keeper = NULL;
static struct uv_scroll gold_heap_scrolls[2] = {
#include "stg09/gold/gold_heap_scrolls.inc"
};
/* @hack type: an array of one, not the single collision_shape it is. SHC
 * then reads center.y 12 bytes past the array's address, with FMOV.S
 * @(R0,R1),FR3 at 0x0c914252. as a single shape, its pool word holds the
 * field's own address, and 33 of gold_heap_run's 133 words differ */
static struct collision_shape gold_heap_shape[1] = {
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, -8.0F, 0.0F},
        69.0F,
        8.0F,
        81.0F,
        {0},
        0,
        NULL,
    },
};
/* @hack type: an array of one, not the single collision_shape it is. it
 * gives njPushMatrix's address R9, njScale's R12 and njPopMatrix's R8 in
 * gold_wall_step (0x0c9144da, 0x0c914520, 0x0c914538). as a single shape,
 * the three take one another's registers, and 15 of gold_wall_step's 260
 * words differ */
static struct collision_shape gold_wall_shape[1] = {
    {
        SHAPE_WALL,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 0.0F, 0.0F},
        40.0F,
        30.0F,
        10.0F,
        {0},
        0,
        NULL,
    },
};

static void gold_heap_init(struct task *task);
static void gold_heap_destroy(struct task *task);
static void gold_heap_run(struct task *task);
static void gold_heap_draw(struct task *task);

void gold_heap_step(struct task *task)
{
    /* @hack redundant: work, read and never used, which keeps task in R14
     * across the range check (MOV R4,R14 at 0x0c9140a8). without it, 12 of
     * 16 words differ */
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        gold_heap_init(task);
    }
}

static void gold_heap_init(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        casino_world_add(task, &gold_heap_object, COPY_RADIUS, WORLD_MOVES);
        collision_init(task, gold_heap_shape, COLLISION_OBJECT, 1, 1);
    }
    task->draw = gold_heap_draw;
    task->step = gold_heap_run;
    task->destructor = gold_heap_destroy;
    work->timer = 0;
    STATUE_LEVEL(work) = 0;
    VARS(work)->height = 0.0F;
    if (!placement_editing(task)) {
        if (!gold_heap_keeper) {
            gold_heap_keeper = work;
        }
    }
    uv_scroll_save(gold_heap_scrolls, 2);
}

static void gold_heap_destroy(struct task *task)
{
    struct task_work *work = task->work;

    casino_world_remove(task);
    if (gold_heap_keeper == work) {
        gold_heap_keeper = NULL;
    }
    uv_scroll_release(gold_heap_scrolls, 2);
}

static void gold_heap_run(struct task *task)
{
    struct task_work *work = task->work;
    Float share;

    if (!player_in_range(&work->pos, NEAR)) {
        return;
    }
    gold_heap_draw(task);
    if (placement_editing(task)) {
        return;
    }
    if (player_get_character(0) == CHARACTER_KNUCKLES) {
        VARS(work)->height = 0.0F;
    } else {
        share = 0.0F;
        if (casino_rings_deposited < GOLD_FULL) {
            share = 1.0F - (casino_rings_deposited * (1.0F / GOLD_FULL));
        }
        share *= gold_heap_depth;
        VARS(work)->height = share;
    }
    casino_world_test_near(task,
                           _builtin_fabsf(VARS(work)->height) + COPY_RADIUS);
    if (VARS(work)->object) {
        /* @hack spelling: the copy raised through a pointer to its y, which SHC
         * builds with ADD #12,R4 at 0x0c914244. through the field, 5 of 133
         * words differ */
        Float *y = &VARS(work)->object->pos[1];

        *y += VARS(work)->height;
    }
    work->collision->shapes->center.y =
        VARS(work)->height + gold_heap_shape[0].center.y + BOX_RISE;
    collision_set_radius(work->collision);
    if (casino_rings_deposited >= GOLD_LEVEL_4) {
        STATUE_LEVEL(work) = 4;
    } else if (casino_rings_deposited >= GOLD_LEVEL_3) {
        STATUE_LEVEL(work) = 3;
    } else if (casino_rings_deposited >= GOLD_LEVEL_2) {
        STATUE_LEVEL(work) = 2;
    } else if (casino_rings_deposited >= GOLD_LEVEL_1) {
        STATUE_LEVEL(work) = 1;
    } else {
        STATUE_LEVEL(work) = 0;
    }
    sonic_statue_set_level(STATUE_LEVEL(work));
    collision_register(work);
    /* @hack spelling: the test for the wrap to 0, written <= 0 for the
     * image's CMP/PL (0x0c9142fe). the timer is unsigned, so the test holds
     * at 0 alone, and the decrement holds the timer at 65535. as == 0, the
     * test is TST, and 2 of 133 words differ */
    if (++TASK_TIMER(work) <= 0) {
        --TASK_TIMER(work);
    }
}

static void gold_heap_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_TEX room[100];
    Float height = VARS(work)->height;
    Float phase;
    Angle ang;

    if (!player_in_range(&work->pos, NEAR) || frame_step) {
        return;
    }
    if (!draw_in_view(work->pos.x, work->pos.y + height, work->pos.z,
                      VIEW_RADIUS)) {
        return;
    }
    phase = gold_heap_slide * height;
    uv_scroll_move(gold_heap_scrolls, 2, room, (int)phase);
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y + height, work->pos.z);
    TURN_VIA(njRotateY, work->ang.y, ang);
    draw_tree(&gold_heap_object);
    njPopMatrix(1);
    uv_scroll_restore(gold_heap_scrolls, 2);
}

int gold_heap_get_rings(void)
{
    if (gold_heap_keeper) {
        return casino_rings_deposited;
    }
    return 0;
}

Bool gold_heap_set_rings(int rings)
{
    if (gold_heap_keeper) {
        casino_rings_deposited = rings;
        return TRUE;
    }
    return FALSE;
}

/* the wall stands while fewer than GOLD_FULL rings are deposited, or while
 * no heap is the keeper */
void gold_wall_step(struct task *task)
{
    struct task_work *work = task->work;
    Float width;
    Float height;
    /* @hack redundant: depth, set and never read, which SHC drops. it keeps
     * width at sp+8 and height at sp+4 (0x0c9144e2, 0x0c9144ec), and gives
     * njPushMatrix's address R9 and njScale's R12 (0x0c9144da, 0x0c914520).
     * without it, width and height trade slots, the addresses take other
     * registers, and 20 of 260 words differ */
    Float depth; /* NOLINT(clang-diagnostic-unused-but-set-variable) */
    Angle ang;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        width = work->scl.x + 10.0F;
        height = work->scl.y + 10.0F;
        depth = work->scl.z + 10.0F;
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 0.0F, 10.0F);
        njScale(NULL, 0.1F, 0.1F, 2.0F);
        njEasyDrawObject(marker_cube_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 0.0F, 20.0F);
        njRotateX(NULL, 0x2000);
        njTranslate(NULL, 0.0F, 0.0F, -3.0F);
        njScale(NULL, 0.1F, 0.1F, 0.7F);
        njEasyDrawObject(marker_cube_object);
        njPopMatrix(1);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 0.0F, 20.0F);
        njRotateX(NULL, -0x2000);
        njTranslate(NULL, 0.0F, 0.0F, -3.0F);
        njScale(NULL, 0.1F, 0.1F, 0.7F);
        njEasyDrawObject(marker_cube_object);
        njPopMatrix(1);
        njScale(NULL, width * 0.2F, height * 0.2F, 0.1F);
        njEasyDrawObject(marker_cube_object);
        njPopMatrix(1);
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "->             ");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROT Y       ");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), "->             ");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> YOKO        ");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> TATE        ");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "->             ");
        work->mode = WALL_EDITED;
        return;
    }
    switch (work->mode) {
    case WALL_INIT:
        collision_init(task, gold_wall_shape, COLLISION_WALL, 1, 1);
        work->collision->shapes->radius = work->scl.x + 10.0F;
        work->collision->shapes->size_y = work->scl.y + 10.0F;
        work->collision->shapes->size_z = work->scl.z + 10.0F;
        collision_set_radius(work->collision);
        work->mode = WALL_RUN;
        break;
    case WALL_RUN:
        if (gold_heap_get_rings() < GOLD_FULL) {
            collision_register(work);
        }
        break;
    default:
        work->mode = WALL_INIT;
        break;
    }
}
