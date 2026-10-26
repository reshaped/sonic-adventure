/* the cage the object list names O MONKEY CAGE: a cage with a monkey inside,
 * which Sonic, E-102 and Knuckles break open */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "adv02/monkey_cage.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/pose.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "enemy/enemy.h"
#include "enemy/saru.h"
#include "object/placement.h"
#include "player/lockon.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the cage's nodes. the monkey walks about the inside node, and the plus and
 * minus x parts hang from the swinging part. the pieces fly apart when the
 * cage blows up */
extern NJS_OBJECT monkey_cage_object;
extern NJS_OBJECT monkey_cage_inside_object;
extern NJS_OBJECT monkey_cage_swing_object;
extern NJS_OBJECT monkey_cage_swing_plus_x_object;
extern NJS_OBJECT monkey_cage_swing_minus_x_object;
extern NJS_OBJECT monkey_cage_piece_1;
extern NJS_OBJECT monkey_cage_piece_2;
extern NJS_OBJECT monkey_cage_piece_3;
extern NJS_OBJECT monkey_cage_piece_4;
extern NJS_OBJECT monkey_cage_piece_5;
extern NJS_OBJECT monkey_cage_piece_6;
extern NJS_OBJECT monkey_cage_piece_7;
extern NJS_OBJECT monkey_cage_piece_8;
extern NJS_OBJECT monkey_cage_piece_9;
extern NJS_OBJECT monkey_cage_piece_10;
extern NJS_OBJECT monkey_cage_piece_11;
extern NJS_OBJECT monkey_cage_piece_12;
extern NJS_OBJECT monkey_cage_piece_13;

/* translate Ninja's current matrix by the position of the node that node
 * points at */
/* @hack spelling: the node passed by its address, which SHC pools and adds
 * pos's offset to, ADD #8,R10 at 0x0c906316; passed by name and read as
 * (node).pos, monkey_cage_draw shrinks, the functions after it move, and 152
 * of its 450 words differ */
/* @hack spelling: the position read through pos_, which SHC forms once and
 * reads the three floats from (0x0c906316 to 0x0c90631e); read as
 * (node)->pos[0] and so on, monkey_cage_draw grows, the functions after it
 * move, and 377 of its 450 words differ */
/* @hack type: the matrix as a bare 0, not NULL, which puts i in R8 and the
 * piece's position in R9 in monkey_cage_draw's loop over the pieces
 * (0x0c90632c, 0x0c906342); as NULL, the two trade and 8 of its 450 words
 * differ */
#define TRANSLATE_TO(node)                                                     \
    do {                                                                       \
        Float *pos_ = (node)->pos;                                             \
                                                                               \
        njTranslate(0, pos_[0], pos_[1], pos_[2]);                             \
    } while (0)

/* placement_try_leave_by ends the task once the camera and player 0 are
 * both beyond 500 */
#define RANGE (500.0F * 500.0F)

#define RADIUS 50.0F /* the collision record's radius */

/* the cage's pieces, and with the swinging part, all the parts that fly */
#define CAGE_PIECES 13
#define PIECES (CAGE_PIECES + 1)

/* the steps the parts fly before the cage is gone */
#define BLOWN_STEPS 240

/* each step, a flying part falls PIECE_GRAVITY faster, and keeps PIECE_DRAG
 * of its speed and SPIN_DRAG of its spin. it trails dust every DUST_STEPS
 * steps */
#define PIECE_GRAVITY 0.027222222F
#define PIECE_DRAG 0.985F
#define SPIN_DRAG 0.96F
#define DUST_STEPS 8
/* how much darker the hanging parts grow each step, once the cage is blown
 * apart */
#define FADE_SPEED (1.0F / 55.0F)

#define SE_BLOW_UP 26

/* the node of the monkey's tree the tail hangs from, and the tail's segments,
 * each TAIL_SPACING along x past the last */
#define TAIL_NODE 37
#define TAIL_SEGMENTS 8
#define TAIL_SPACING 0.6336667F

/* the turn about z at each segment of the tail, which waves with
 * work->timer */
#define TAIL_TURN(work)                                                        \
    (1536.0F + (njSin((Uint16)(work)->timer * 2073.6F) * 1024.0F))

/* the floats of an NJS_MATRIX */
#define MATRIX_FLOATS 16

/* the swinging part's swing: its phase, in angle units each step, and its
 * widest angle */
#define SWING_SPEED 320
#define SWING_ANGLE 10240.0F

/* the farthest the monkey walks from the cage's center */
#define WALK_RADIUS 5.0F
/* the monkey's turn each step: walking along the edge, and facing about or
 * turning around */
#define WALK_TURN 1024
#define FACE_TURN 768
/* the monkey's walking speed gains WALK_GAIN each step and keeps WALK_DRAG of
 * itself. it goes no higher than WALK_TOP */
#define WALK_GAIN 0.015F
#define WALK_DRAG 0.95F
#define WALK_TOP 0.24F

/* a move's steps, chosen by rand. a walk takes WALK_STEPS and fewer than
 * WALK_EXTRA more, 120 to 719 in all. a face takes FACE_STEPS and fewer than
 * FACE_EXTRA more, 200 to 559. the first walk takes FIRST_WALK_STEPS */
#define WALK_STEPS 120.0F
#define WALK_EXTRA 600.0F
#define FACE_STEPS 200.0F
#define FACE_EXTRA 360.0F
#define FIRST_WALK_STEPS 240
/* how far a turn around may miss the about-face, either way */
#define TURN_SPREAD 8192.0F

/* the action frames the monkey advances each step */
#define FRAME_SPEED 0.45F
#define BLEND_SPEED 0.06F /* the blend's progress each step */

/* the cage's modes, in work->mode. the cage's own code never sets
 * MONKEY_CAGE_KILL, whose step kills the placement */
enum monkey_cage_mode { MONKEY_CAGE_INIT, MONKEY_CAGE_RUN, MONKEY_CAGE_KILL };

/* the monkey's actions, in monkey_cage_actions order */
enum monkey_cage_action {
    ACTION_JUMP,
    ACTION_FLIP,
    ACTION_WALK,
    ACTION_THROW,
    ACTION_HOP,
    ACTION_FIDGET
};

/* the monkey's moves. MOVE_FACE turns it to atan2f(z, -x) of its position in
 * the cage, and MOVE_TURN turns it around */
enum monkey_cage_move { MOVE_WALK, MOVE_FACE, MOVE_TURN };

/* the cage's states, in work->submode */
enum monkey_cage_state { CAGE_SHUT, CAGE_BLOWN, CAGE_GONE };

struct monkey_cage_piece {
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    struct angles ang;
    struct angles spin; /* added to ang each step */
};

/* the cage's record. monkey_cage_init allocates it from the heap, and
 * monkey_cage_destroy frees it */
struct monkey_cage {
    int index; /* the cage's index, 0 to MONKEY_CAGES - 1 */
    int move; /* the monkey's current monkey_cage_move */
    NJS_POINT3 pos; /* the monkey's position, relative to the inside node */
    Float speed; /* the monkey's walking speed */
    struct angles ang; /* the monkey's angles */
    Angle swing; /* the swinging part's y angle */
    int steps; /* the steps since the cage blew apart */
    Float fade; /* the hanging parts' darkening, from 0 down to -1 */
    struct monkey_cage_piece pieces[PIECES];
    Sint16 target; /* the y angle the monkey turns around to */
    Uint8 unk_2d2[2];
    Bool blending;
    Float blend; /* the blend's progress into work->tick's action, 0 to 1 */
    int previous; /* the action the monkey blends from */
    NJS_MOTION_LINK link;
};

/* work->tick holds the monkey_cage_action the monkey plays */
struct monkey_cage_vars {
    Float frame;
    /* the steps until the next move starts, or negative while a turn runs */
    Sint16 wait;
    Sint8 next; /* the next monkey_cage_move */
    Uint8 unk_07;
    struct monkey_cage *cage;
};

#define VARS(work) ((struct monkey_cage_vars *)&(work)->vars)

static void monkey_cage_init(struct task *task);
static void monkey_cage_read_index(struct task *task);
static void monkey_cage_move(struct task *task);
static void monkey_cage_draw(struct task *task);
static void monkey_cage_destroy(struct task *task);
static void monkey_cage_blend_to(struct task_work *work, int action);
static void monkey_cage_blend_step(struct task_work *work);
static void monkey_cage_draw_monkey(struct task_work *work);

static NJS_OBJECT *monkey_cage_pieces[CAGE_PIECES] = {
    &monkey_cage_piece_1,  &monkey_cage_piece_2,  &monkey_cage_piece_3,
    &monkey_cage_piece_4,  &monkey_cage_piece_5,  &monkey_cage_piece_6,
    &monkey_cage_piece_7,  &monkey_cage_piece_8,  &monkey_cage_piece_9,
    &monkey_cage_piece_10, &monkey_cage_piece_11, &monkey_cage_piece_12,
    &monkey_cage_piece_13,
};
static NJS_ACTION *monkey_cage_actions[] = {
    &saru_jump_action,  &saru_flip_action, &saru_walk_action,
    &saru_throw_action, &saru_hop_action,  &saru_fidget_action,
};
/* a capsule round the cage, which an attack stronger than its guard hurts. it
 * pushes every group, and no group can push it */
static struct collision_shape monkey_cage_shapes[1] = {
    {SHAPE_CAPSULE | SHAPE_HURTABLE,
     GUARD(1),
     PUSH_FIRM(PUSH_ALL) | PUSH_ALL,
     {0.0F, 13.7F, 2.0F},
     11.0F,
     6.5F,
     0.0F,
     {0},
     0,
     NULL},
};
/* dust each flying piece trails */
static struct dust_params monkey_cage_dust = {
    1.0F,
    0.1F,
    0.2F,
    0.2F,
    0.3F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.4F, 1.0F, 1.0F, 1.0F},
};

struct task_work *monkey_cages[MONKEY_CAGES] = {NULL, NULL};

/* each character's event flag for each cage, or -1 where it has none. the
 * second cage's flags are the characters' Red Mountain entrance flags */
static int monkey_cage_flags[CHARACTER_COUNT][MONKEY_CAGES] = {
    {-1, FLAG_SONIC_MR_ENTRANCE_MOUNTAIN},
    {-1, -1},
    {-1, -1},
    {FLAG_KNUCKLES_MR_MONKEY_CAGE_A_BOMB, FLAG_KNUCKLES_MR_ENTRANCE_MOUNTAIN},
    {-1, -1},
    {-1, -1},
    {-1, FLAG_E102_MR_ENTRANCE_MOUNTAIN},
    {-1, -1},
};
/* the point the cage blows up from, relative to the node inside */
static NJS_POINT3 monkey_cage_blast = {0.0F, 0.0F, 0.0F};

/* the base of the monkey's tail as posed by its action, and by the two actions
 * it blends between */
static NJS_MATRIX monkey_cage_tail;
static NJS_MATRIX monkey_cage_tail_from;
static NJS_MATRIX monkey_cage_tail_to;

/* while the set editor holds a cage, each step reads its index again and
 * labels the editor's fields */
void monkey_cage_step(struct task *task)
{
    struct task_work *work = task->work;
    struct monkey_cage *cage;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        switch (work->mode) {
        case MONKEY_CAGE_INIT:
            monkey_cage_init(task);
            work = task->work;
            cage = VARS(work)->cage;
            monkey_cages[cage->index] = work;
            break;
        case MONKEY_CAGE_RUN:
            monkey_cage_draw(task);
            monkey_cage_move(task);
            break;
        case MONKEY_CAGE_KILL:
            placement_kill(task);
            break;
        }
    } else {
        switch (work->mode) {
        case MONKEY_CAGE_INIT:
            monkey_cage_init(task);
            break;
        case MONKEY_CAGE_RUN:
            monkey_cage_read_index(task);
            monkey_cage_draw(task);
            monkey_cage_move(task);
            /* @hack redundant: a goto to the next statement, which has SHC load
             * debug_print's address into R3 and the labels' into R2
             * (0x0c9057e2, 0x0c9057e6); without it, the two trade and 8 of
             * the function's 80 words differ */
            goto labels;
        labels:
            debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
            debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> ID(0-1)");
            break;
        }
    }
}

/* the cage's guard stays the 1 its shape gives it for every character but
 * E-102, for whom it is 0 */
static void monkey_cage_init(struct task *task)
{
    struct task_work *work = task->work;
    struct monkey_cage *cage;
    int character;

    task->draw = monkey_cage_draw;
    task->destructor = monkey_cage_destroy;
    enemy_alloc_record(task, work);
    collision_init(task, monkey_cage_shapes, COLLISION_ITEM, 1, 1);
    if (work->collision) {
        character = get_character();
        if (character == CHARACTER_SONIC) {
            work->collision->shapes->levels =
                (work->collision->shapes->levels & ~SHAPE_GUARD) | GUARD(1);
        } else if (character == CHARACTER_E102) {
            /* @hack spelling: the &= written out, which reads the byte
             * through @(R0,R4) with 4 in R0 (0x0c9058a8, 0x0c9058ac); as &=,
             * it reads @(4,R3), the function ends 2 bytes early, and 56 of
             * its 97 words differ */
            work->collision->shapes->levels =
                work->collision->shapes->levels & ~SHAPE_GUARD;
        }
    }
    /* @bug memory: the radius goes through work->collision outside the test of
     * it above, FMOV.S FR3,@(R0,R3) at 0x0c9058be. a cage without a collision
     * record stores 50.0F at address 8 */
    work->collision->radius = RADIUS;
    work->timer = 0;
    VARS(work)->frame = 0.0F;
    cage = VARS(work)->cage = heap_alloc(sizeof(struct monkey_cage));
    cage->pos.x = 0.0F;
    cage->pos.y = 0.0F;
    cage->pos.z = 0.0F;
    cage->ang.x = 0;
    cage->ang.y = 0;
    cage->ang.z = 0;
    cage->move = MOVE_WALK;
    cage->speed = 0.0F;
    VARS(work)->wait = FIRST_WALK_STEPS;
    VARS(work)->next = MOVE_FACE;
    cage->swing = 0;
    monkey_cage_blend_to(work, ACTION_WALK);
    monkey_cage_read_index(task);
    work->mode = MONKEY_CAGE_RUN;
}

/* read the cage's index from scale x, clamped to the cages. a cage whose flag
 * is set is CAGE_GONE, and any other CAGE_SHUT */
static void monkey_cage_read_index(struct task *task)
{
    struct task_work *work = task->work;
    struct monkey_cage *cage = VARS(work)->cage;

    cage->index = work->scl.x;
    if (cage->index < 0) {
        cage->index = 0;
    } else if (cage->index >= MONKEY_CAGES) {
        cage->index = MONKEY_CAGES - 1;
    }
    work->submode = monkey_cage_flag(cage->index) ? CAGE_GONE : CAGE_SHUT;
}

/* while the cage is shut, the monkey makes one move at a time inside it. it
 * walks within WALK_RADIUS of the center, faces about, or turns around.
 * Sonic's attack or E-102's shot sets the cage's flag, and the kill switch
 * sets it for Knuckles. once the flag is set, the cage blows apart, and its
 * parts fly for BLOWN_STEPS */
static void monkey_cage_move(struct task *task)
{
    struct task_work *work = task->work;
    struct monkey_cage *cage = VARS(work)->cage;
    /* a walking step copies this, puts -speed in its x, and turns it by the
     * monkey's y angle */
    static const NJS_POINT3 zero = {0.0F, 0.0F, 0.0F};
    NJS_POINT3 point;
    NJS_VECTOR speed;
    Float x;
    Float y;
    Float z;
    Float length;
    Angle ang;
    int turn;
    Sint16 diff;
    struct monkey_cage_piece *piece;
    NJS_OBJECT **pieces;
    int i;
    /* @hack spelling: njPopMatrix called through pop, which loads its address
     * once, into R8 at 0x0c9059aa; called by name, 25 of the function's 880
     * words differ */
    Bool (*pop)(Uint32 count) = njPopMatrix;

    if (work->submode == CAGE_SHUT) {
        x = cage->pos.x;
        y = cage->pos.y;
        z = cage->pos.z;
        if (cage->move == MOVE_WALK) {
            if (!cage->blending) {
                point = zero;
                point.x = -cage->speed;
                njPushMatrix(&_unit_matrix_);
                TURN(njRotateY, cage->ang.y);
                njCalcVector(NULL, &point, &speed);
                pop(1);
                monkey_cage_blend_to(work, ACTION_WALK);
                x += speed.x;
                y += speed.y;
                z += speed.z;
                cage->speed += WALK_GAIN;
                cage->speed *= WALK_DRAG;
                if (cage->speed > WALK_TOP) {
                    cage->speed = WALK_TOP;
                }
                if ((length = (x * x) + (z * z)) > WALK_RADIUS * WALK_RADIUS) {
                    length = WALK_RADIUS / njSqrt(length);
                    x *= length;
                    z *= length;
                    /* at the edge, turn the monkey to walk along it, the way
                     * round it was going. the sign of the cross product of
                     * its position and speed gives that way */
                    if (!((x * speed.z) - (z * speed.x) < 0.0F)) {
                        ang = atan2f(x, z) * ANGLE_PER_RADIAN;
                        diff = ang - cage->ang.y;
                        if ((turn = diff) > WALK_TURN) {
                            cage->ang.y += WALK_TURN;
                        } else if (turn < -WALK_TURN) {
                            cage->ang.y -= WALK_TURN;
                        } else {
                            cage->ang.y = (Sint16)ang;
                        }
                    } else {
                        ang = atan2f(-x, -z) * ANGLE_PER_RADIAN;
                        diff = ang - cage->ang.y;
                        if ((turn = diff) > WALK_TURN) {
                            cage->ang.y += WALK_TURN;
                        } else if (turn < -WALK_TURN) {
                            cage->ang.y -= WALK_TURN;
                        } else {
                            cage->ang.y = (Sint16)ang;
                        }
                    }
                }
            }
        } else if (cage->move == MOVE_FACE) {
            ang = atan2f(z, -x) * ANGLE_PER_RADIAN;
            diff = ang - cage->ang.y;
            if ((turn = diff) > FACE_TURN) {
                /* @hack redundant: a never-run store of turn to the cage's
                 * move, which keeps the image's R4, R5 and R6 (0x0c905bda to
                 * 0x0c905be0); without it, the three rotate and 9 of 880 words
                 * differ */
                if (0) {
                    cage->move = turn;
                }
                if (!cage->blending) {
                    cage->ang.y += FACE_TURN;
                    monkey_cage_blend_to(work, ACTION_FIDGET);
                }
            } else if (turn < -FACE_TURN) {
                if (!cage->blending) {
                    cage->ang.y -= FACE_TURN;
                    monkey_cage_blend_to(work, ACTION_FIDGET);
                }
            } else if (!cage->blending) {
                cage->ang.y = (Sint16)ang;
                monkey_cage_blend_to(work, ACTION_FIDGET);
            }
        } else if (cage->move == MOVE_TURN) {
            diff = cage->target - cage->ang.y;
            if ((turn = diff) > FACE_TURN) {
                if (!cage->blending) {
                    cage->ang.y += FACE_TURN;
                    monkey_cage_blend_to(work, ACTION_FIDGET);
                }
            } else if (turn < -FACE_TURN) {
                if (!cage->blending) {
                    cage->ang.y -= FACE_TURN;
                    monkey_cage_blend_to(work, ACTION_FIDGET);
                }
            } else if (!cage->blending) {
                cage->ang.y = cage->target;
                VARS(work)->wait = 0;
                VARS(work)->next = MOVE_WALK;
            }
        }
        cage->pos.x = x;
        cage->pos.y = y;
        cage->pos.z = z;
        /* once a move's wait runs out, the next move starts. after a walk
         * comes a face. after a face comes a turn or a walk, at even odds. a
         * turn lasts until the monkey faces its target, and then a walk
         * comes */
        if (VARS(work)->wait == 0) {
            if (!cage->blending) {
                /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
                cage->move = VARS(work)->next;
                switch (cage->move) {
                case MOVE_WALK:
                    VARS(work)->wait =
                        WALK_STEPS + (rand() * (WALK_EXTRA * RAND_SCALE));
                    VARS(work)->next = MOVE_FACE;
                    cage->speed = 0.0F;
                    break;
                case MOVE_FACE:
                    if (rand() * RAND_SCALE < 0.5F) {
                        VARS(work)->wait =
                            FACE_STEPS + (rand() * (FACE_EXTRA * RAND_SCALE));
                        VARS(work)->next = MOVE_TURN;
                    } else {
                        VARS(work)->wait =
                            FACE_STEPS + (rand() * (FACE_EXTRA * RAND_SCALE));
                        VARS(work)->next = MOVE_WALK;
                    }
                    break;
                case MOVE_TURN:
                    cage->target =
                        (cage->ang.y + 0x8000) +
                        (((rand() * (2.0F * RAND_SCALE)) - 1.0F) * TURN_SPREAD);
                    VARS(work)->wait = -1;
                    VARS(work)->next = MOVE_TURN;
                    break;
                }
            }
        } else {
            --VARS(work)->wait;
        }
        VARS(work)->frame += FRAME_SPEED;
        if (VARS(work)->frame >
            monkey_cage_actions[work->tick]->motion->nbFrame - 1.0F) {
            VARS(work)->frame -=
                monkey_cage_actions[work->tick]->motion->nbFrame - 1.0F;
        } else if (VARS(work)->frame < 0.0F) {
            VARS(work)->frame +=
                monkey_cage_actions[work->tick]->motion->nbFrame - 1.0F;
        }
        monkey_cage_blend_step(work);
        cage->swing = njSin((Uint16)work->timer * SWING_SPEED) * SWING_ANGLE;
        switch (player_get_character(0)) {
        case CHARACTER_SONIC:
            if (work->flags & COLLISION_DAMAGED) {
                monkey_cage_set_flag(cage->index, TRUE);
            }
            break;
        case CHARACTER_E102:
            if (work->flags & COLLISION_DAMAGED) {
                monkey_cage_set_flag(cage->index, TRUE);
                lockon_release(task);
            }
            break;
        }
        if (monkey_cage_flag(cage->index)) {
            /* each piece flies outward from the cage's center, and up */
            piece = cage->pieces;
            pieces = monkey_cage_pieces;
            for (i = CAGE_PIECES; i; ++piece, --i) {
                NJS_OBJECT *object = *pieces++;
                Float *pos;
                /* @hack spelling: the cage's nodes read through their
                 * addresses, here and below, which SHC pools, as
                 * monkey_cage_object's at 0x0c906100, and reads the fields
                 * through (0x0c905ff4 to 0x0c905ff8); read by name, 180 of the
                 * function's 880 words and 3 pool words differ */
                Float *center = (&monkey_cage_object)->pos;

                x = center[0] + (pos = object->pos)[0];
                y = center[1] + pos[1];
                z = center[2] + pos[2];
                /* @hack redundant: a never-run read of pos[1] into y, which
                 * keeps monkey_cage_object's address in R5, as the image loads
                 * it at 0x0c905f70 (MOV.L @(0x0c906100),R5); without it, R4
                 * and R5 trade and 15 of 880 words differ */
                if (0) {
                    y = pos[1];
                }
                piece->pos.x = x;
                piece->pos.y = y;
                piece->pos.z = z;
                piece->speed.x = x * 0.1F;
                piece->speed.y = 1.0F + (y * 0.1F);
                piece->speed.z = z * 0.1F;
                piece->ang.x = 0;
                piece->ang.y = 0;
                piece->ang.z = 0;
                piece->spin.x = rand() * 0.125F;
                piece->spin.y = rand() * 0.125F;
                piece->spin.z = rand() * 0.125F;
            }
            /* the swinging part starts at its x in all three coordinates,
             * not at its own y and z */
            x = (&monkey_cage_object)->pos[0] +
                (&monkey_cage_swing_object)->pos[0];
            y = z = x;
            piece->pos.x = x;
            piece->pos.y = y;
            piece->pos.z = z;
            piece->speed.x = x * 0.1F;
            piece->speed.y = 1.0F + (y * 0.1F);
            piece->speed.z = z * 0.1F;
            /* @hack redundant: a never-run store of 0.1 to the piece's speed.x,
             * which keeps the image's FR14 and FR15 (0x0c905fc0, 0x0c905f74);
             * without it, the two trade and 12 of 880 words differ */
            if (0) {
                piece->speed.x = 0.1F;
            }
            piece->ang.x = 0;
            piece->ang.y = 0;
            piece->ang.z = 0;
            piece->spin.x = rand() * 0.125F;
            piece->spin.y = rand() * 0.125F;
            piece->spin.z = rand() * 0.125F;
            njPushMatrix(&_unit_matrix_);
            njTranslateV(NULL, &work->pos);
            TURN(njRotateY, work->ang.y);
            {
                Float *center = (&monkey_cage_object)->pos;
                Float *inside = (&monkey_cage_inside_object)->pos;

                njTranslate(NULL, center[0] + inside[0], center[1] + inside[1],
                            center[2] + inside[2]);
            }
            njCalcPoint(NULL, &monkey_cage_blast, &point);
            bomb_create(&point, 1.0F);
            sound_play(SE_BLOW_UP, 0, 0, 0);
            pop(1);
            cage->steps = 0;
            cage->fade = 0.0F;
            work->submode = CAGE_BLOWN;
        }
        collision_register(work);
        ++work->timer;
    } else if (work->submode == CAGE_BLOWN) {
        /* the parts fall, slow, spin down and trail dust */
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        piece = cage->pieces;
        for (i = PIECES; i; ++piece, --i) {
            njAddVector(&piece->pos, &piece->speed);
            piece->ang.x += piece->spin.x;
            piece->ang.y += piece->spin.y;
            piece->ang.z += piece->spin.z;
            piece->speed.y -= PIECE_GRAVITY;
            piece->speed.x *= PIECE_DRAG;
            piece->speed.y *= PIECE_DRAG;
            piece->speed.z *= PIECE_DRAG;
            piece->spin.x *= SPIN_DRAG;
            piece->spin.y *= SPIN_DRAG;
            piece->spin.z *= SPIN_DRAG;
            if (!(cage->steps % DUST_STEPS)) {
                monkey_cage_dust.scale = 2.0F;
                njCalcPoint(NULL, &piece->pos, &monkey_cage_dust.pos);
                njCalcVector(NULL, &piece->speed, &monkey_cage_dust.speed);
                dust_create_from(&monkey_cage_dust);
            }
        }
        pop(1);
        cage->fade -= FADE_SPEED;
        if (cage->fade < -1.0F) {
            cage->fade = -1.0F;
        }
        ++cage->steps;
        if (cage->steps >= BLOWN_STEPS) {
            work->submode = CAGE_GONE;
        }
    }
}

/* while the cage is shut, draw it whole, with the swinging part and the
 * monkey inside. once it is blown apart, draw the flying parts, and the
 * hanging parts as they darken */
static void monkey_cage_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct monkey_cage *cage = VARS(work)->cage;
    struct monkey_cage_piece *piece;
    NJS_OBJECT **pieces;
    NJS_OBJECT *object;
    int i;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    texture_set_stage();
    if (work->submode == CAGE_SHUT) {
        TRANSLATE_TO(&monkey_cage_object);
        /* @hack spelling: the cage's nodes read through their addresses, here
         * and below, which SHC pools and reads model 4 bytes past, as MOV.L
         * @(4,R4),R4 at 0x0c906328; read by name, 23 of the function's 450
         * words differ, and so do 14 words of its pool and 12 words of
         * monkey_cage_destroy */
        draw_model((&monkey_cage_object)->model);
        pieces = monkey_cage_pieces;
        i = 0;
        do {
            object = *pieces++;
            /* @hack redundant: a never-run read of the piece's ang[0], which
             * keeps the image's R9 and R10 (0x0c906330, 0x0c90633e); without
             * it, the two trade and 11 of 450 words differ */
            if (0) {
                ang = object->ang[0];
            }
            njPushMatrix(NULL);
            njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
            draw_model(object->model);
            njPopMatrix(1);
        } while (++i < CAGE_PIECES);
        njPushMatrix(NULL);
        TRANSLATE_TO(&monkey_cage_swing_object);
        TURN(njRotateY, cage->swing);
        draw_model((&monkey_cage_swing_object)->model);
        njPushMatrix(NULL);
        TRANSLATE_TO(&monkey_cage_swing_plus_x_object);
        draw_model((&monkey_cage_swing_plus_x_object)->model);
        njPopMatrix(1);
        TRANSLATE_TO(&monkey_cage_swing_minus_x_object);
        draw_model((&monkey_cage_swing_minus_x_object)->model);
        njPopMatrix(1);
        monkey_cage_draw_monkey(work);
    } else if (work->submode == CAGE_BLOWN) {
        piece = cage->pieces;
        pieces = monkey_cage_pieces;
        for (i = 0; i < CAGE_PIECES; ++i) {
            object = *pieces++;
            njPushMatrix(NULL);
            njTranslateV(NULL, &piece->pos);
            TURN(njRotateX, piece->ang.x);
            TURN(njRotateY, piece->ang.y);
            TURN(njRotateZ, piece->ang.z);
            draw_model(object->model);
            njPopMatrix(1);
            ++piece;
        }
        piece = &cage->pieces[CAGE_PIECES];
        njPushMatrix(NULL);
        njTranslateV(NULL, &piece->pos);
        TURN(njRotateX, piece->ang.x);
        TURN(njRotateY, piece->ang.y);
        TURN(njRotateZ, piece->ang.z);
        draw_model((&monkey_cage_swing_object)->model);
        draw_set_material(0.0F, cage->fade, cage->fade, cage->fade);
        njPushMatrix(NULL);
        TRANSLATE_TO(&monkey_cage_swing_plus_x_object);
        draw_model((&monkey_cage_swing_plus_x_object)->model);
        njPopMatrix(1);
        TRANSLATE_TO(&monkey_cage_swing_minus_x_object);
        draw_model((&monkey_cage_swing_minus_x_object)->model);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPopMatrix(1);
    }
    njPopMatrix(1);
}

static void monkey_cage_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct monkey_cage *cage = VARS(work)->cage;

    /* @hack spelling: the slot reached as the array's address plus the index,
     * which reads the index twice, MOV.L @R4,R3 at 0x0c9066c8 and MOV.L @R4,R1
     * at 0x0c9066d4; as monkey_cages[cage->index], the functions after it
     * move, and 15 of its 17 words differ */
    if (work == *(&monkey_cages[0] + cage->index)) {
        *(&monkey_cages[0] + cage->index) = NULL;
    }
    heap_free(cage);
}

/* start blending the monkey's action into another, which plays from its first
 * frame. an action the monkey already plays changes nothing */
static void monkey_cage_blend_to(struct task_work *work, int action)
{
    struct monkey_cage *cage = VARS(work)->cage;
    /* @hack redundant: 16 bytes of frame no instruction reads or writes, ADD
     * #-16,R15 at 0x0c906740; without them, the frame is gone, the functions
     * after it move, and 43 of its 43 words differ */
    NJS_MOTION_LINK link;

    if (action != work->tick) {
        cage->previous = work->tick;
        cage->link.motion[0] = monkey_cage_actions[work->tick]->motion;
        cage->link.motion[1] = monkey_cage_actions[action]->motion;
        cage->link.frame[0] = VARS(work)->frame;
        cage->link.frame[1] = 0.0F;
        work->tick = action;
        VARS(work)->frame = 0.0F;
        cage->blending = TRUE;
        cage->blend = 0.0F;
    }
}

static void monkey_cage_blend_step(struct task_work *work)
{
    struct monkey_cage *cage = VARS(work)->cage;

    if (cage->blending) {
        cage->blend += BLEND_SPEED;
        if (cage->blend > 1.0F) {
            cage->blending = FALSE;
        }
    }
}

/* draw the monkey at the inside node, and then its tail. the tail is
 * TAIL_SEGMENTS of the body's prism from the tail's base, each turned
 * TAIL_TURN past the last, and the arm's prism ends it. while the monkey
 * blends between actions, so does the tail's base */
static void monkey_cage_draw_monkey(struct task_work *work)
{
    struct monkey_cage *cage = VARS(work)->cage;
    NJS_ACTION_LINK action;
    Float *from;
    Float *to;
    Float *out;
    int i;
    Float blend;
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    Angle ang;
    /* @hack spelling: njTranslate called through translate, which loads its
     * address once, into R11 at 0x0c906808; called by name, 4 of the
     * function's 309 words and 4 pool words differ */
    void (*translate)(NJS_MATRIX *matrix, Float x, Float y, Float z);

    /* @hack redundant: a never-run read of work->ang.x, which, as the read of
     * ang.z below does, keeps the image's registers, 8 in R8 and njScale's
     * address in R3 among them (0x0c906942, 0x0c906950); without it, the
     * function runs 8 bytes long, the functions after it move, and 175 of 309
     * words differ */
    if (0) {
        ang = work->ang.x;
    }
    /* @hack redundant: a never-run read of work->ang.z, which, as the read of
     * ang.x above does, keeps the image's registers, 8 in R8 and njScale's
     * address in R3 among them (0x0c906942, 0x0c906950); without it, the
     * function runs 8 bytes long, the functions after it move, and 175 of 309
     * words differ */
    if (0) {
        ang = work->ang.z;
    }
    njPushMatrix(NULL);
    translate = njTranslate;
    {
        /* @hack spelling: the inside node read through its address, which SHC
         * pools at 0x0c90688c and adds pos's offset to, ADD #8,R10 at
         * 0x0c90680a; read by name, 50 of the function's 309 words and a pool
         * word differ */
        Float *pos = (&monkey_cage_inside_object)->pos;

        translate(NULL, pos[0], pos[1], pos[2]);
    }
    njTranslateV(NULL, &cage->pos);
    TURN(njRotateY, cage->ang.y);
    njSetTexture(&e_saru_texlist);
    if (!cage->blending) {
        draw_action(monkey_cage_actions[work->tick], VARS(work)->frame);
        pose_action(NULL, monkey_cage_actions[work->tick], VARS(work)->frame,
                    TAIL_NODE, &monkey_cage_tail);
    } else {
        action.object = &saru_object;
        action.motionlink = &cage->link;
        draw_action_link(&action, cage->blend);
        pose_action(NULL, monkey_cage_actions[cage->previous],
                    cage->link.frame[0], TAIL_NODE, &monkey_cage_tail_from);
        pose_action(NULL, monkey_cage_actions[work->tick], cage->link.frame[1],
                    TAIL_NODE, &monkey_cage_tail_to);
        out = monkey_cage_tail;
        from = monkey_cage_tail_from;
        to = monkey_cage_tail_to;
        blend = cage->blend;
        for (i = 0; i < MATRIX_FLOATS; ++i) {
            *out++ = (blend * (*to++ - *from)) + *from;
            ++from;
        }
    }
    njPushMatrix(&monkey_cage_tail);
    for (i = 0; i < TAIL_SEGMENTS; ++i) {
        njPushMatrix(NULL);
        njScale(NULL, 0.4F, 1.0F, 1.0F);
        draw_model(&saru_body_prism_model);
        njPopMatrix(1);
        translate(NULL, TAIL_SPACING, 0.0F, 0.0F);
        /* @hack redundant: the turn tested again inside its own test, which
         * computes it three times, with njSin at 0x0c90697e, 0x0c90699c and
         * 0x0c9069ba; as one TURN, the functions after it move, and 189 of
         * its 309 words differ */
        TAIL_TURN(work) != 0.0F
            ? (TAIL_TURN(work) != 0.0F ? njRotateZ(NULL, TAIL_TURN(work))
                                       : (void)0)
            : (void)0;
    }
    njScale(NULL, 0.8F, 1.0F, 1.0F);
    draw_model(&saru_arm_prism_model);
    njPopMatrix(1);
    njPopMatrix(1);
    /* @hack redundant: a never-run draw of the arm's prism, which keeps the
     * image's registers, 8 in R8 and njScale's address in R3 among them
     * (0x0c906942, 0x0c906950); without it, the function runs 8 bytes long, the
     * functions after it move, and 177 of 309 words differ */
    if (0) {
        draw_model(&saru_arm_prism_model);
    }
}

void monkey_cage_set_flag(int cage, Bool on)
{
    int character = player_get_character(0);
    int flag;

    if ((flag = monkey_cage_flags[character][cage]) >= 0) {
        event_flags[flag] = on;
    }
}

Bool monkey_cage_flag(int cage)
{
    int character = player_get_character(0);
    int flag;

    if ((flag = monkey_cage_flags[character][cage]) >= 0) {
        return event_flags[flag];
    }
    return FALSE;
}
