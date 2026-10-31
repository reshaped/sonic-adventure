/* O ASIATO: the footprints a player leaves on ground with WORLD_FOOTPRINTS */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>
#include <string.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "player/e102.h"
#include "player/player.h"
#include "stg01/footprints.h"
#include "system/cache.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* @hack type: int, not stage.h's Uint16, so footprints_run compares the
 * result without extending it, as at 0x0c90650a. as stage.h declares it, 7
 * of footprints_run's 664 words and 1 of the 165 words of its pools and pad
 * differ */
#ifdef GCC
#include "stage/stage.h"
#else
extern int stage_get_stage_act(void);
#endif

extern NJS_OBJECT beach_footprint_0_object;
extern NJS_OBJECT beach_footprint_1_object;
extern NJS_OBJECT beach_big_footprint_0_object;
extern NJS_OBJECT beach_big_footprint_1_object;
extern NJS_OBJECT beach_e102_footprint_0_object;
extern NJS_OBJECT beach_e102_footprint_1_object;
extern NJS_OBJECT beach_e102_skate_print_object;

/* a player's feet, in the model's frame, as its record keeps them. weld
 * WELD_MARK + 2 keeps foot a in weld_point2, and WELD_MARK + 3 foot b in
 * weld_point3 */
#define FOOT_A(record) (&(record)->weld_point2)
#define FOOT_B(record) (&(record)->weld_point3)

#define FOOTPRINTS 100

/* footprint_place looks for ground below a point PROBE_HEIGHT above where a
 * print goes. the print lies a random LIFT to LIFT + LIFT_SPREAD above that
 * ground */
#define PROBE_HEIGHT 1.5F
#define LIFT 0.05F
#define LIFT_SPREAD 0.01F

/* footprints_run's halfway print for a skating foot needs dist under
 * HALFWAY_LIMIT */
#define HALFWAY_LIMIT 16.0F

struct footprint {
    NJS_POINT3 pos;
    struct angles ang;
    NJS_MODEL *model;
};

/* a ring of count prints, the oldest at prints[head] */
struct footprint_ring {
    struct footprint prints[FOOTPRINTS];
    int head;
    int count;
    /* the feet's points in the record as the last step left them, in the
     * model's frame */
    union point_floats last_b;
    union point_floats last_a;
    /* the player's position when the task last tried to place a print */
    NJS_POINT3 player_at_print;
};

/* a struct angles that half views as six Sint16s, two to each angle, the low
 * half first */
union halves {
    struct angles full;
    struct {
        Sint16 x;
        Sint16 x_high;
        Sint16 y;
        Sint16 y_high;
        Sint16 z;
        Sint16 z_high;
    } half;
};

/* work->vars as the task lays them out. work->submode holds the player's
 * number, and work->mode its print_kind */
struct footprints_vars {
    Uint8 unk_00[2];
    Uint16 stage; /* stage_get_stage_act's result as the prints started */
    Sint16 feet;
    Uint8 unk_06[2];
    struct footprint_ring *ring; /* on the heap */
};

/* the bits of feet. FOOT_B_DOWN and FOOT_A_DOWN mark a foot that came down
 * and hasn't risen since. FEET_AIRBORNE marks a player off the ground, and
 * clears the other two */
#define FOOT_B_DOWN 0x1
#define FOOT_A_DOWN 0x2
#define FEET_AIRBORNE 0x4

#define VARS(work) ((struct footprints_vars *)&(work)->vars)

/* how one character's feet leave prints */
struct footprint_kind {
    /* foot b's print and foot a's walking, then the same skating, for a kind
     * that skates */
    NJS_OBJECT *prints[4];
    void (*step)(struct task *task);
    /* a walking foot prints only more than stride from the newest print. a
     * skating player prints only more than stride from player_at_print */
    Float stride;
    /* a foot is down only while its y in the record is below down_height */
    Float down_height;
    NJS_VECTOR offset; /* moves the feet, in the player's frame */
};

/* the print kinds: PRINT_BIG for Big, PRINT_E102 for E-102, and
 * PRINT_DEFAULT for every other character */
enum print_kind { PRINT_DEFAULT, PRINT_BIG, PRINT_E102, PRINT_KINDS };

/* a print draws with FADE less alpha than the next newer print */
#define FADE 0.0095F

static void footprints_draw(struct task *task);
static void footprints_destroy(struct task *task);
static void footprints_edit(struct task *task);
static void footprints_run(struct task *task);

static struct footprint_kind footprint_kinds[PRINT_KINDS] = {
    {{&beach_footprint_0_object, &beach_footprint_1_object, NULL, NULL},
     footprints_run,
     4.0F,
     5.0F,
     {0.0F, 0.0F, 0.0F}},
    {{&beach_big_footprint_0_object, &beach_big_footprint_1_object, NULL, NULL},
     footprints_run,
     5.0F,
     7.0F,
     {0.0F, 0.0F, 0.0F}},
    {{&beach_e102_footprint_0_object, &beach_e102_footprint_1_object,
      &beach_e102_skate_print_object, &beach_e102_skate_print_object},
     footprints_run,
     6.0F,
     -8.3F,
     {-7.0F, 0.0F, 0.0F}}};

/* draw the prints, the newest first, each older one FADE fainter */
static void footprints_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct footprint_ring *ring = VARS(work)->ring;
    struct footprint *print;
    int count;
    int index;
    int i;
    Float alpha;

    texture_set_stage();
    count = ring->count;
    if (count == 0) {
        return;
    }
    index = ring->head + count - 1;
    if (index >= FOOTPRINTS) {
        index -= FOOTPRINTS;
    }
    print = &ring->prints[index];
    /* the newest print's alpha, 0.95F - 1.0F. that makes the image's
     * 0xbd4cccd0 at 0x0c90602c, where -0.05F makes 0xbd4ccccc */
    alpha = 0.95F - 1.0F;
    njPushMatrix(NULL);
    for (i = 0; i < count; i++) {
        matrix_load_parent();
        draw_set_material(alpha, 0.0F, 0.0F, 0.0F);
        njTranslateV(NULL, &print->pos);
        TURN(njRotateZ, print->ang.z);
        TURN(njRotateX, print->ang.x);
        TURN(njRotateY, print->ang.y + NJM_DEG_ANG(180));
        alpha -= FADE;
        draw_model(print->model);
        index--;
        /* at prints[0], this steps print back past the array, as the delay
         * slot at 0x0c905fd8 does. the wrap at 0x0c905fda then sets print to
         * the last */
        print--;
        if (index < 0) {
            print = &ring->prints[FOOTPRINTS - 1];
            index = FOOTPRINTS - 1;
        }
    }
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* set *foot_a and *foot_b to the player's feet in the world. the shape is
 * the player's first collision shape. a foot's x and z are the record's. its
 * y is minus the distance from the player's position to the shape's center.
 * offset, unless NULL, moves both feet. the feet then turn about the shape's
 * center by the player's x and z angles. their y angle is a half turn less
 * the player's. *angles gets the angles they turn by */
static void find_feet(int player, struct angles *angles, NJS_POINT3 *foot_a,
                      NJS_POINT3 *foot_b, NJS_VECTOR *offset)
{
    struct task_work *work = player_work[player];
    struct collision_shape *shape = work->collision->shapes;
    struct player *record = player_get(player);
    NJS_POINT3 point_b;
    NJS_POINT3 point_a;
    /* @hack redundant: 12 of the 48 bytes that ADD #-48,R15 reserves at
     * 0x0c906068. without them, 66 of 147 words differ */
    Uint8 unused[12];

    point_a = *FOOT_A(record);
    point_b = *FOOT_B(record);
    point_a.y = -njDistanceP2P(&shape->center, &work->pos);
    point_b.y = point_a.y;
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &shape->center);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, NJM_DEG_ANG(180) - work->ang.y);
    if (offset != NULL) {
        njTranslateV(NULL, offset);
    }
    njCalcPoint(NULL, &point_b, &point_b);
    njCalcPoint(NULL, &point_a, &point_a);
    njPopMatrix(1);
    *angles = work->ang;
    angles->y = NJM_DEG_ANG(180) - angles->y;
    *foot_b = point_b;
    *foot_a = point_a;
}

/* add a print of object's model at pos, turned by ang. it lies a random
 * LIFT to LIFT + LIFT_SPREAD higher than pos. once the ring is full, the
 * print replaces the oldest */
static void footprint_add(NJS_POINT3 *pos, struct angles *ang,
                          NJS_OBJECT *object, struct task_work *work)
{
    struct footprint_ring *ring = VARS(work)->ring;
    struct footprint *print;
    int index;

    index = ring->head + ring->count;
    if (index >= FOOTPRINTS) {
        index -= FOOTPRINTS;
    }
    print = &ring->prints[index];
    print->pos = *pos;
    print->pos.y += (rand() * (LIFT_SPREAD * RAND_SCALE)) + LIFT;
    print->ang = *ang;
    print->model = object->model;
    if (ring->count < FOOTPRINTS) {
        ring->count++;
    } else {
        ring->head++;
        if (ring->head >= FOOTPRINTS) {
            ring->head = 0;
        }
    }
}

/* add a print where the ground lies below a point PROBE_HEIGHT above pos,
 * when that ground has WORLD_FOOTPRINTS. the print takes ang's y and the
 * ground's slope */
static void footprint_place(NJS_POINT3 *pos, struct angles *ang,
                            NJS_OBJECT *object, struct task_work *work)
{
    struct sides_query query;
    struct surface *below;
    NJS_POINT3 at;
    struct angles turn;

    query.pos = *pos;
    query.pos.y += PROBE_HEIGHT;
    ground_find_sides(&query);
    below = &query.sides[SIDE_BELOW];
    if (below->found && (below->flags & WORLD_FOOTPRINTS)) {
        at = *pos;
        at.y = below->level;
        turn = *ang;
        turn.x = below->ang_x;
        turn.z = below->ang_z;
        footprint_add(&at, &turn, object, work);
        /* @hack redundant: a read of below that never runs. it compiles to
         * nothing. it keeps below in R14, saved at 0x0c9062a0, and makes the
         * function 2 bytes longer. without those bytes, footprints_start,
         * which follows, starts 2 early, and 14 of its 119 words differ.
         * without it, 75 of 75 words differ */
        if (0) {
            at.y = below->level;
        }
    }
}

void footprints_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;

    if (placement_editing(task)) {
        task->step = footprints_edit;
        return;
    }
    work->submode = (Sint8)work->scl.x;
    if (work->submode >= PLAYER_COUNT) {
        return;
    }
    player = player_work[work->submode];
    if (player == NULL) {
        return;
    }
    work->timer = 0;
    VARS(work)->stage = stage_get_stage_act();
    VARS(work)->feet = 0;
    VARS(work)->ring = heap_alloc(sizeof(struct footprint_ring));
    VARS(work)->ring->head = 0;
    VARS(work)->ring->count = 0;
    VARS(work)->ring->last_a.point = player->pos;
    VARS(work)->ring->last_b.point = player->pos;
    VARS(work)->ring->player_at_print = player->pos;
    /* the memset clears the stores above too. last_b, last_a and
     * player_at_print start at the origin, not at the player */
    memset(VARS(work)->ring, 0, sizeof(struct footprint_ring));
    switch (player_get_character(work->submode)) {
    default:
        work->mode = PRINT_DEFAULT;
        break;
    case CHARACTER_BIG:
        work->mode = PRINT_BIG;
        break;
    case CHARACTER_E102:
        work->mode = PRINT_E102;
        break;
    }
    task->draw = footprints_draw;
    task->step = footprint_kinds[work->mode].step;
    task->destructor = footprints_destroy;
}

static void footprints_destroy(struct task *task)
{
    struct task_work *work = task->work;

    heap_free(VARS(work)->ring);
}

static void footprints_edit(struct task *task)
{
    struct task_work *work = task->work;

    njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "PlayerID:");
    njPrintD(NJM_LOCATION(29, EDITOR_ROW_SCL_X), (Sint8)work->scl.x, 3);
}

/* the step once the prints start. it draws them. then it tries a print
 * under a foot that rises after coming down. the other foot must not be
 * down. the player must be on ground with WORLD_FOOTPRINTS. the foot must
 * lie more than a stride from the newest print. a skating E-102 leaves prints
 * its own way */
static void footprints_run(struct task *task)
{
    struct task_work *work = task->work;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int number = work->submode;
    struct player *record = player_get(number);
    struct footprint_ring *ring = VARS(work)->ring;
    struct footprint_kind *kind = &footprint_kinds[work->mode];
    struct task_work *player;
    NJS_POINT3 *last_pos;
    struct angles *last_ang;
    int index;
    int placed;
    union point_floats foot_a;
    union point_floats foot_b;
    struct angles angles;
    union point_floats mid;
    Sint16 *half;
    Float halved;

    if (VARS(work)->stage != stage_get_stage_act()) {
        task_kill(task);
        return;
    }
    player = player_work[number];
    if (player == NULL) {
        return;
    }
    if (record == NULL) {
        return;
    }
    footprints_draw(task);
    last_pos = NULL;
    last_ang = NULL;
    if (ring->count > 0) {
        index = ring->head + ring->count - 1;
        if (index >= FOOTPRINTS) {
            /* @hack spelling: -FOOTPRINTS added, an ADD #-100 at 0x0c906598.
             * with -=, a SUB of 100 in R3, and 1 of 664 words differ */
            index += -FOOTPRINTS;
        }
        if (index < 0) {
            index = FOOTPRINTS - 1;
        }
        last_pos = &ring->prints[index].pos;
        last_ang = &ring->prints[index].ang;
    }
    if (player->flags & PLAYER_ON_GROUND) {
        find_feet(number, &angles, &foot_a.point, &foot_b.point, &kind->offset);
        if (player->mode == E102_SKATE && kind->prints[2] != NULL &&
            kind->prints[3] != NULL) {
            Float dist;

            /* a skating E-102's prints. dist runs from a foot's point in the
             * record at the last step to the foot now. a foot below
             * down_height prints when the player's ground has
             * WORLD_FOOTPRINTS. dist and the player's move from
             * player_at_print must both pass a stride. before each print,
             * another can go halfway between those two points. that print
             * needs one in the ring, and dist under both two strides and
             * HALFWAY_LIMIT */
            /* @hack redundant: (void)&dist, which keeps dist in the stack
             * frame: FMOV.S FR0,@R15 at 0x0c90662a stores it. without it, 451
             * of 664 words differ */
            (void)&dist;
            dist = 0.0F;
            placed = 0;
            /* @hack spelling: the empty branch, with the test negated, which
             * has the distance call at 0x0c906624 go through R2. as if (test),
             * it goes through R3, and 2 of 664 words differ */
            if (!(FOOT_B(record)->y < kind->down_height &&
                  (record->ground_flags & WORLD_FOOTPRINTS))) {
            } else {
                /* @bug logic: dist runs from last_b, a point in the model's
                 * frame (stored at 0x0c906ae8), to foot_b, a point in the
                 * world (the call at 0x0c906624). dist is then about the
                 * foot's distance from the world's origin, not its move. away
                 * from the origin, the stride test always holds, and the
                 * halfway print, which needs dist under HALFWAY_LIMIT, never
                 * comes */
                if ((dist = njDistanceP2P(&ring->last_b.point, &foot_b.point)) >
                        kind->stride &&
                    njDistanceP2P(&ring->player_at_print, &player->pos) >
                        kind->stride) {
                    if (last_pos && dist < kind->stride * 2.0F &&
                        dist < HALFWAY_LIMIT) {
                        union halves turn;

                        /* @hack spelling: the walk through mid, last_b and
                         * foot_b by the cursors from, to and point. as fields,
                         * the code grows 32 bytes, and 620 of 664 words
                         * differ */
                        /* @hack spelling: to++ apart from the first
                         * subtraction, which puts from, to and point in R5, R4
                         * and R6 at 0x0c906676. with to++ in the subtraction,
                         * 20 of 664 words differ */
                        {
                            const Float *from = ring->last_b.elements;
                            const Float *to = foot_b.elements;
                            Float *point = mid.elements;

                            *point++ = *from++ - *to;
                            to++;
                            *point++ = *from++ - *to++;
                            *point = *from - *to;
                            point = mid.elements;
                            *point++ *= 0.5F;
                            *point++ *= 0.5F;
                            *point *= 0.5F;
                            point = mid.elements;
                            to = foot_b.elements;
                            *point++ += *to++;
                            *point++ += *to++;
                            *point += *to;
                        }
                        turn.full.x = angles.x - last_ang->x;
                        turn.full.y = angles.y - last_ang->y;
                        turn.full.z = angles.z - last_ang->z;
                        /* turn holds the differences from the newest print's
                         * angles to angles. the code below halves each
                         * difference's low Sint16 half, and keeps its high
                         * half. adding the newest print's angles back then
                         * takes turn halfway, the short way round (0x0c90671e
                         * to 0x0c906754) */
                        /* @hack spelling: the cursor half and the Float halved,
                         * which keep player in R9, loaded at 0x0c906566. as
                         * turn.half.x *= 0.5F and the like, the code runs 32
                         * bytes short, and 449 of 664 words differ */
                        half = &turn.half.x;
                        halved = *half;
                        halved *= 0.5F;
                        *half = halved;
                        half = &turn.half.y;
                        halved = *half;
                        halved *= 0.5F;
                        *half = halved;
                        half = &turn.half.z;
                        halved = *half;
                        halved *= 0.5F;
                        *half = halved;
                        turn.full.x += last_ang->x;
                        turn.full.y += last_ang->y;
                        turn.full.z += last_ang->z;
                        footprint_place(&mid.point, &turn.full, kind->prints[2],
                                        work);
                    }
                    footprint_place(&foot_b.point, &angles, kind->prints[2],
                                    work);
                    placed = 1;
                }
            }
            /* @hack spelling: the empty branch, with the test negated, which
             * has the distance call at 0x0c9067ae go through R2. as if (test),
             * it goes through R3, and 2 of 664 words differ */
            if (!(FOOT_A(record)->y < kind->down_height &&
                  (record->ground_flags & WORLD_FOOTPRINTS))) {
            } else {
                /* @bug logic: as for foot b, dist runs from last_a, in the
                 * model's frame (stored at 0x0c906ad6), to foot_a, in the
                 * world (the call at 0x0c9067ae) */
                if ((dist = njDistanceP2P(&ring->last_a.point, &foot_a.point)) >
                        kind->stride &&
                    njDistanceP2P(&ring->player_at_print, &player->pos) >
                        kind->stride) {
                    if (last_pos && dist < kind->stride * 2.0F &&
                        dist < HALFWAY_LIMIT) {
                        union halves turn;

                        /* @hack spelling: the walk through mid, last_a and
                         * foot_a by the cursors from, to and point. as fields,
                         * 593 of 664 words differ */
                        /* @hack spelling: point++ and to++ apart from the
                         * second subtraction, and to++ apart from the first
                         * add. they put from, to and point in R6, R5 and R4 at
                         * 0x0c906800, and point and to in R5 and R4 at
                         * 0x0c906842. with each ++ in its expression, 29 of
                         * 664 words differ */
                        {
                            const Float *from = ring->last_a.elements;
                            const Float *to = foot_a.elements;
                            Float *point = mid.elements;

                            *point++ = *from++ - *to++;
                            *point = *from++ - *to;
                            point++;
                            to++;
                            *point = *from - *to;
                            point = mid.elements;
                            *point++ *= 0.5F;
                            *point++ *= 0.5F;
                            *point *= 0.5F;
                            point = mid.elements;
                            to = foot_a.elements;
                            *point++ += *to;
                            to++;
                            *point++ += *to++;
                            *point += *to;
                        }
                        turn.full.x = angles.x - last_ang->x;
                        turn.full.y = angles.y - last_ang->y;
                        turn.full.z = angles.z - last_ang->z;
                        /* turn goes halfway as for foot b (0x0c9068b8 to
                         * 0x0c9068e8) */
                        /* @hack spelling: the cursor half and the Float halved,
                         * which hold each half in FR4, as at 0x0c9068bc. as
                         * turn.half.x *= 0.5F and the like, the code runs 32
                         * bytes short, and 280 of 664 words differ */
                        half = &turn.half.x;
                        halved = *half;
                        halved *= 0.5F;
                        *half = halved;
                        half = &turn.half.y;
                        halved = *half;
                        halved *= 0.5F;
                        *half = halved;
                        half = &turn.half.z;
                        halved = *half;
                        halved *= 0.5F;
                        *half = halved;
                        turn.full.x += last_ang->x;
                        turn.full.y += last_ang->y;
                        turn.full.z += last_ang->z;
                        footprint_place(&mid.point, &turn.full, kind->prints[3],
                                        work);
                        /* @hack redundant: a never-run test of last_pos, which
                         * compiles to nothing. it gives last_pos R8, cleared at
                         * 0x0c90657e, leaves number in the stack frame, stored
                         * at 0x0c9064e2, and keeps the function 32 bytes
                         * longer. without it, 555 of 664 words differ */
                        if (0) {
                            if (last_pos) {
                            }
                        }
                    }
                    footprint_place(&foot_a.point, &angles, kind->prints[3],
                                    work);
                    placed = 1;
                }
            }
            if (placed) {
                ring->player_at_print = player->pos;
            }
            VARS(work)->feet &= ~(FOOT_B_DOWN | FOOT_A_DOWN);
        } else {
            if (FOOT_B(record)->y < kind->down_height &&
                FOOT_B(record)->y < ring->last_b.point.y &&
                FOOT_B(record)->y <= FOOT_A(record)->y) {
                VARS(work)->feet |= FOOT_B_DOWN;
            } else if (FOOT_B(record)->y > ring->last_b.point.y) {
                if ((record->ground_flags & WORLD_FOOTPRINTS) &&
                    (VARS(work)->feet & FOOT_B_DOWN) &&
                    !(VARS(work)->feet & FOOT_A_DOWN)) {
                    if (!last_pos ||
                        njDistanceP2P(last_pos, &foot_b.point) > kind->stride) {
                        footprint_place(&foot_b.point, &angles, kind->prints[0],
                                        work);
                        ring->player_at_print = player->pos;
                    }
                }
                VARS(work)->feet &= ~FOOT_B_DOWN;
            }
            if (FOOT_A(record)->y < kind->down_height &&
                FOOT_A(record)->y < ring->last_a.point.y &&
                FOOT_A(record)->y <= FOOT_B(record)->y) {
                VARS(work)->feet |= FOOT_A_DOWN;
            } else if (FOOT_A(record)->y > ring->last_a.point.y) {
                if ((record->ground_flags & WORLD_FOOTPRINTS) &&
                    (VARS(work)->feet & FOOT_A_DOWN) &&
                    !(VARS(work)->feet & FOOT_B_DOWN)) {
                    if (!last_pos ||
                        njDistanceP2P(last_pos, &foot_a.point) > kind->stride) {
                        footprint_place(&foot_a.point, &angles, kind->prints[1],
                                        work);
                        ring->player_at_print = player->pos;
                    }
                }
                VARS(work)->feet &= ~FOOT_A_DOWN;
            }
        }
        VARS(work)->feet &= ~FEET_AIRBORNE;
    } else {
        VARS(work)->feet = FEET_AIRBORNE;
    }
    ring->last_a.point = *FOOT_A(record);
    ring->last_b.point = *FOOT_B(record);
}
