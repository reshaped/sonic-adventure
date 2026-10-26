/* Chaos 4's sweep. he rises to the swamp's surface and reaches out. his
 * arm then swings round as it stretches to three times its length, dips,
 * and shrinks back. the swept arm is a model of its own, drawn in place of
 * his body's arm. it is a chain of joints with a spring in each but the
 * hand's, and his shapes 4 and 5 ride its last two joints */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos4/boss.h"
#include "b_chaos4/swamp.h"
#include "b_chaos4/sweep.h"
#include "chaos/chaos.h"
#include "chaos/effect.h"
#include "collision/collision.h"
#include "draw/model.h"
#include "draw/motion.h"
#include "sound/sound.h"
#include "system/task.h"
#include "system/util.h"

/* the arm's joints, from the shoulder to the hand. the joints hold no
 * models. each joint but the first has a sibling, a part that draws in the
 * matrix of the joint before it */
extern NJS_OBJECT sweep_joint0;
extern NJS_OBJECT sweep_joint1;
extern NJS_OBJECT sweep_joint2;
extern NJS_OBJECT sweep_joint3;
extern NJS_OBJECT sweep_joint4;
extern NJS_OBJECT sweep_joint5;

enum sweep_mode {
    SWEEP_START,
    SWEEP_RISE,
    SWEEP_REACH,
    SWEEP_HOLD,
    SWEEP_STRETCH,
    SWEEP_SHRINK,
    SWEEP_END
};

#define SE_SWEEP 0x174 /* as the arm starts to swing, and again as it dips */

/* he rises toward SWAMP_Y, the swamp's surface, for RISE_TIME + 1 steps.
 * his speed's y gains RISE_SPEED as he starts */
#define RISE_TIME 30
#define RISE_SPEED 1.0F

/* as the arm starts to swing, each spring's spin is set to SWING_SPIN
 * about y alone. as it dips at full stretch, each spring's spin about z
 * gains DIP_SPIN */
#define SWING_SPIN NJM_DEG_ANG(-5)
#define DIP_SPIN NJM_DEG_ANG(-3)

/* once the reach has played, the arm's stretch grows by STRETCH_GROWTH a
 * step to STRETCH_FULL. the stretch then shrinks by STRETCH_SHRINK a step
 * back to 1 */
#define STRETCH_GROWTH 0.015F
#define STRETCH_FULL 3.0F
#define STRETCH_SHRINK 0.045F

/* from SWEEP_RISE to SWEEP_SHRINK, the chance a step that
 * chaos_effect_trail leaves a piece and a spark at his shape 5 */
#define TRAIL_CHANCE 0.7F

/* each frame, a spring's spin keeps SPRING_KEEP of itself and loses
 * SPRING_PULL of the spring's turn, which pulls it back to its rest */
#define SPRING_KEEP 0.97F
#define SPRING_PULL 0.01F
/* the kick adds KICK_SPIN to each spring's spin about y in turn. it kicks
 * spring 0 on the first frame the arm draws, and each next spring
 * KICK_FRAMES frames later */
#define KICK_SPIN NJM_DEG_ANG(10)
#define KICK_FRAMES 4

#define HALF_TURN NJM_DEG_ANG(180)

static void draw_part(NJS_OBJECT *object);

/* the arm's joints, the hand's last, ended by NULL */
static NJS_OBJECT *sweep_joints[] = {
    &sweep_joint0, &sweep_joint1, &sweep_joint2, &sweep_joint3,
    &sweep_joint4, &sweep_joint5, NULL,
};

/* the shoulder's turn off his body as chaos4_sweep_shapes applies it: z by
 * -1080, x by 8532 and y by -1440 */
static const struct angles shoulder_ang = {8532, -1440, -1080};
/* the points on joints 4 and 5 that his shapes 4 and 5 take */
static const NJS_POINT3 joint4_point = {0.0F, 0.0F, 0.0F};
static const NJS_POINT3 joint5_point = {0.0F, 0.0F, 0.0F};

/* his shapes go on, and he turns toward player 0 as he rises, then reaches
 * out. at the reach's last frame, the swept arm takes the place of his
 * body's and swings round as it stretches. at full stretch it dips, then
 * shrinks back, and the mode ends once his motion is back at rest */
void chaos4_sweep(struct task_work *work, struct task_motion *motion,
                  struct chaos *chaos)
{
    /* @hack spelling: copy, a second local holding chaos, which keeps chaos
     * on the stack (MOV.L R11,@R15 at 0x0c903e9c) and sets target.y through
     * it (MOV.L @R15,R3 at 0x0c903eea); through chaos, 101 of 305 words
     * differ */
    struct chaos *copy = chaos;
    struct chaos4 *record = CHAOS4_VARS(work)->record;
    struct chaos4_arm *arm = &record->arm;
    int i;
    struct collision_work *collision;

    /* @hack redundant: a never-run test of the submode against SWEEP_SHRINK,
     * which tips SHC's registers for 5 and 0 to the image's: 5 in R9 and 0 in
     * R10 (0x0c903e98, 0x0c903eac); without it, the two trade and 13 of 305
     * words differ */
    if (0) {
        if (work->submode == SWEEP_SHRINK) {
        }
    }
    switch (work->submode) {
    case SWEEP_START:
        chaos4_set_shapes(work, SHAPES_ON);
        record->turn = TURN_PLAYER;
        copy->target.y = SWAMP_Y;
        record->speed.y += RISE_SPEED;
        CHAOS4_VARS(work)->timer = RISE_TIME;
        swamp_ripple_add(&work->pos);
        sound_play(SE_LASH, 0, 0, 0);
        chaos->motion.wanted = ACTION_RISE;
        work->submode = SWEEP_RISE;
    case SWEEP_RISE:
        if (--CHAOS4_VARS(work)->timer < 0) {
            chaos->motion.wanted = ACTION_REACH;
            record->move = MOVE_STILL;
            record->speed.x = record->speed.y = record->speed.z = 0.0F;
            work->submode = SWEEP_REACH;
        }
        break;
    case SWEEP_REACH:
        if (chaos->motion.current == ACTION_REACH) {
            work->submode = SWEEP_HOLD;
        }
        break;
    case SWEEP_HOLD: {
        /* @hack spelling: the action reached through three locals, which
         * index the table in R4 (EXTU.W R0,R4 at 0x0c903f76, ADD R3,R4 at
         * 0x0c903f7c); read in the test as
         * chaos->motion.actions[chaos->motion.wanted].action, 80 of 305
         * words differ */
        int playing = chaos->motion.wanted;
        struct motion_entry *entry = &chaos->motion.actions[playing];
        NJS_ACTION *action = entry->action;

        if (chaos->motion.frame >= action->motion->nbFrame - 1.0F) {
            arm->stretch = 1.0F;
            chaos4_hide_tree(&chaos4_body_arm, TRUE);
            record->draw |= DRAW_ARM;
            record->turn = TURN_NONE;
            sound_play(SE_SWEEP, 0, 0, 0);
            for (i = 0; i < SWEEP_SPRINGS; i++) {
                struct angles *spin = &arm->spin[i];

                spin->x = spin->y = spin->z = 0;
                spin->y += SWING_SPIN;
            }
            work->submode = SWEEP_STRETCH;
        }
        break;
    }
    case SWEEP_STRETCH:
        arm->stretch += STRETCH_GROWTH;
        if (arm->stretch >= STRETCH_FULL) {
            for (i = 0; i < SWEEP_SPRINGS; i++) {
                struct angles *spin = &arm->spin[i];

                spin->z = spin->z + DIP_SPIN;
            }
            sound_play(SE_SWEEP, 0, 0, 0);
            work->submode = SWEEP_SHRINK;
        }
        break;
    case SWEEP_SHRINK:
        arm->stretch -= STRETCH_SHRINK;
        if (arm->stretch <= 1.0F) {
            arm->stretch = 1.0F;
            chaos4_hide_tree(&chaos4_body_arm, FALSE);
            record->draw &= ~DRAW_ARM;
            /* his task's step skips motion_step while MOTION_HOLDING is
             * set, as ACTION_REACH's hold left it */
            motion_flags &= ~MOTION_HOLDING;
            chaos->motion.wanted = ACTION_BACK;
            work->submode = SWEEP_END;
        }
        break;
    case SWEEP_END:
        if (chaos->motion.current == ACTION_REST) {
            work->submode = MODE_DONE;
        }
        break;
    }
    switch (work->submode) {
    case SWEEP_RISE:
    case SWEEP_REACH:
    case SWEEP_HOLD:
    case SWEEP_STRETCH:
    case SWEEP_SHRINK:
        collision = work->collision;
        if (rand() * RAND_SCALE < TRAIL_CHANCE) {
            chaos_effect_trail(&collision->shapes[5].center,
                               &collision->shapes[5].center, 1);
        }
        break;
    }
    if (work->submode == MODE_DONE) {
        if (work->mode != BOSS_SWIM) {
            boss_next_mode = BOSS_SWIM;
            boss_last_mode = work->mode;
        }
        work->submode = SWEEP_START;
    }
}

void chaos4_hide_tree(NJS_OBJECT *object, Bool hide)
{
    if (hide) {
        object->evalflags |= NJD_EVAL_HIDE;
    } else {
        object->evalflags &= ~NJD_EVAL_HIDE;
    }
    if (object->child != NULL) {
        chaos4_hide_tree(object->child, hide);
    }
    if (object->sibling != NULL) {
        chaos4_hide_tree(object->sibling, hide);
    }
}

/* draw object, its children and its later siblings, each node's model by
 * draw_model. it reads no evalflags, so a node set to hide draws too */
static void draw_part(NJS_OBJECT *object)
{
    Angle ang;

    njPushMatrix(NULL);
    njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
    TURN_VIA(njRotateZ, object->ang[2], ang);
    TURN_VIA(njRotateX, object->ang[0], ang);
    TURN_VIA(njRotateY, object->ang[1], ang);
    njScale(NULL, object->scl[0], object->scl[1], object->scl[2]);
    if (object->model != NULL) {
        draw_model(object->model);
    }
    if (object->child != NULL) {
        draw_part(object->child);
    }
    njPopMatrix(1);
    if (object->sibling != NULL) {
        draw_part(object->sibling);
    }
}

/* step the springs, then draw the joints from the current matrix,
 * stretched along x by stretch. each joint draws its sibling, its part,
 * before it enters its own matrix, as Ninja draws a sibling */
void chaos4_sweep_draw(struct task_work *work)
{
    struct chaos4 *record;
    struct chaos4_arm *arm;
    struct angles pull;
    struct angles *turn;
    struct angles *spin;
    NJS_OBJECT *joint;
    Angle ang;
    int i;
    int node = 0;
    int depth = 0;

    /* @hack spelling: record and arm set after the declarations, which loads
     * the record (MOV.L @(16,R4),R14 at 0x0c9042c4) after SPRING_KEEP goes
     * into FR5 and before SPRING_PULL goes into FR4; initialized where they
     * are declared, the two trade registers and 92 of 737 words differ */
    record = CHAOS4_VARS(work)->record;
    arm = &record->arm;
    for (i = 0; i < SWEEP_SPRINGS; i++) {
        turn = &arm->turn[i];
        spin = &arm->spin[i];
        spin->x = (Angle)(spin->x * SPRING_KEEP);
        spin->y = (Angle)(spin->y * SPRING_KEEP);
        spin->z = (Angle)(spin->z * SPRING_KEEP);
        /* @hack spelling: pull written and read through its address, each time
         * through a fresh copy of R15 plus 8 (MOV R15,R2 and ADD #8,R2 at
         * 0x0c90430e, again at 0x0c90431e); as members, chaos4_sweep_draw
         * runs 96 bytes short, and 725 of 737 words differ */
        (&pull)->x = (Angle)(turn->x * SPRING_PULL);
        (&pull)->y = (Angle)(turn->y * SPRING_PULL);
        (&pull)->z = (Angle)(turn->z * SPRING_PULL);
        spin->x = spin->x - (&pull)->x;
        spin->y = spin->y - (&pull)->y;
        spin->z = spin->z - (&pull)->z;
        turn->x = turn->x + spin->x;
        turn->y = turn->y + spin->y;
        turn->z = turn->z + spin->z;
        /* @bug logic: the kick runs once a fight, in his first sweep. kick and
         * kick_timer start at 0 with his record and nothing resets them, so
         * once the kick has passed the last spring (ADD #1 at 0x0c9043a0), no
         * later sweep's kick test holds (CMP/EQ at 0x0c904382) */
        if (arm->kick == i && --arm->kick_timer < 0) {
            arm->kick++;
            arm->kick_timer = KICK_FRAMES;
            arm->spin[i].y += KICK_SPIN;
        }
    }
    njPushMatrix(NULL);
    njScale(NULL, arm->stretch, 1.0F, 1.0F);
    for (; (joint = sweep_joints[node]) != NULL; node++) {
        /* @bug memory: sweep_joints holds six joints and turn five,
         * so the hand's joint, the sixth, turns by turn[5], which is
         * spin[0] (turn from node at 0x0c904846 to 0x0c904856, read at
         * 0x0c90487e) */
        turn = &arm->turn[node];
        depth++;
        if (joint->sibling != NULL) {
            draw_part(joint->sibling);
        }
        njPushMatrix(NULL);
        njTranslate(NULL, joint->pos[0], joint->pos[1], joint->pos[2]);
        TURN_VIA(njRotateZ, joint->ang[2] + turn->z, ang);
        TURN_VIA(njRotateX, joint->ang[0] + turn->x, ang);
        TURN_VIA(njRotateY, joint->ang[1] + turn->y, ang);
        njScale(NULL, joint->scl[0], joint->scl[1], joint->scl[2]);
    }
    njPopMatrix(depth);
    njPopMatrix(1);
}

/* build the arm's matrices from the unit matrix: boss point 7, his
 * angles, shoulder_ang and the stretch, then each joint turned as
 * chaos4_sweep_draw turns it. shapes 4 and 5 take the world places of
 * joints 4 and 5 */
void chaos4_sweep_shapes(struct task_work *work)
{
    struct chaos4 *record;
    struct chaos4_arm *arm;
    struct angles *turn;
    NJS_OBJECT *joint;
    struct collision_work *collision;
    /* @hack type: ang a Uint32, not an Angle, which keeps each joint's
     * angle in R8 and passes it in R5 (ADD R3,R8 at 0x0c904a74, MOV R8,R5 at
     * 0x0c904a7c); as an Angle, it takes R5 and 85 of 219 words differ */
    Uint32 ang;
    int node = 0;
    int depth = 0;

    /* @hack spelling: record and arm set after the declarations, which zeroes
     * node and depth in R4 ahead of the record's load (MOV #0,R4 at
     * 0x0c904934, MOV.L @(16,R12),R4 at 0x0c90493c); initialized where they
     * are declared, the zero takes R5 and 12 of 219 words differ */
    record = CHAOS4_VARS(work)->record;
    arm = &record->arm;
    if (record->draw & DRAW_ARM) {
        njPushMatrix(&_unit_matrix_);
        njTranslateV(NULL, &boss_points[7].pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, HALF_TURN - work->ang.y, ang);
        njPushMatrix(NULL);
        /* @bug logic: his shapes 4 and 5 go on an arm turned z by -1080 and y
         * by -1440, where boss.c's draw_arm turns the drawn arm z by -1440 and
         * y by -1080. the two read the same three numbers otherwise: this turns
         * z by shoulder_ang's z and y by its y (MOV.L @(8,R14) at 0x0c90499e,
         * @(4,R14) at 0x0c9049be), and draw_arm z by shoulder_angles[1] and y
         * by [2] (0x0c901a02, 0x0c901a22) */
        TURN_VIA(njRotateZ, shoulder_ang.z, ang);
        TURN_VIA(njRotateX, shoulder_ang.x, ang);
        TURN_VIA(njRotateY, shoulder_ang.y, ang);
        njScale(NULL, arm->stretch, 1.0F, 1.0F);
        /* @hack redundant: a never-run test of njCalcPoint's address, which
         * puts it in the image's register, R9 (0x0c9049e0), and leaves
         * njPushMatrix's address loaded at each call (0x0c904950); without it,
         * njPushMatrix's takes R9, njCalcPoint's is loaded into R1 at each
         * call, sp+4 and sp+8 trade, and 138 of 219 words differ */
        if (0) {
            if (&njCalcPoint) {
            }
        }
        for (; (joint = sweep_joints[node]) != NULL; node++) {
            /* @bug memory: as in chaos4_sweep_draw, the hand's joint
             * turns by turn[5], which is spin[0] (turn from node at
             * 0x0c904a40 to 0x0c904a50, read at 0x0c904a72) */
            turn = &arm->turn[node];
            depth++;
            njPushMatrix(NULL);
            njTranslate(NULL, joint->pos[0], joint->pos[1], joint->pos[2]);
            /* @hack spelling: the turns as three if statements, not TURN_VIA's
             * conditional expressions, each a sum in R8 that TST R8,R8 tests
             * before its call (0x0c904a76, 0x0c904a88, 0x0c904a9a); through
             * TURN_VIA, chaos4_sweep_shapes runs 32 bytes long and 156 of 219
             * words differ */
            if ((ang = joint->ang[2] + turn->z) != 0) {
                njRotateZ(NULL, ang);
            }
            if ((ang = joint->ang[0] + turn->x) != 0) {
                njRotateX(NULL, ang);
            }
            if ((ang = joint->ang[1] + turn->y) != 0) {
                njRotateY(NULL, ang);
            }
            njScale(NULL, joint->scl[0], joint->scl[1], joint->scl[2]);
            /* @hack spelling: both joints compared as sweep_joints[node], not
             * joint, which keeps work in R12 (MOV R4,R12 at 0x0c904932) and
             * njPushMatrix's address loaded at each call (0x0c904950,
             * 0x0c904996); compared as joint, 155 of 219 words differ */
            if (sweep_joints[node] == &sweep_joint4) {
                collision = work->collision;
                njCalcPoint(NULL, (NJS_POINT3 *)&joint4_point,
                            &collision->shapes[4].center);
            }
            if (sweep_joints[node] == &sweep_joint5) {
                collision = work->collision;
                njCalcPoint(NULL, (NJS_POINT3 *)&joint5_point,
                            &collision->shapes[5].center);
            }
        }
        njPopMatrix(depth);
        njPopMatrix(2);
    }
}
