/* the robots and props in the deck's two lanes. O ROBO A sidesteps a beat
 * about its home, and O ROBO B walks one. O BUKET, O BOX and O UEKI rest. while
 * the deck tilts, each slides with gravity, and the robots and the bucket
 * tumble. once the deck is level again, all but O UEKI right themselves, and
 * the robots walk home */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "effect/splash.h"
#include "object/common.h"
#include "object/placement.h"
#include "stage/stage.h"
#include "stg06/robo.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

extern NJS_OBJECT robo_a_object;
extern NJS_OBJECT robo_b_object;
extern NJS_OBJECT buket_object;
extern NJS_OBJECT box_object;
extern NJS_OBJECT ueki_object;
extern NJS_ACTION robo_a_action;
extern NJS_ACTION robo_b_action;

/* the square of the distance from the camera or player 0 within which a
 * robot or O BUKET stays */
#define ROBO_RANGE (1010.0F * 1010.0F)

/* the two lanes run along x, from LANE_X0 to LANE_X1. across z, one runs from
 * -LANE_Z1 to -LANE_Z0. a task with SIDE set takes the other, from LANE_Z0 to
 * LANE_Z1. robo_keep_in_lane keeps a task at least its radius inside its
 * lane */
#define LANE_X0 0.0F
#define LANE_X1 600.0F
#define LANE_Z0 370.0F
#define LANE_Z1 440.0F
#define SIDE(work) ((work)->tick)
/* a task's radius, in scl.z. the init sets SIDE when the set file's scale z is
 * above 0. it then writes the radius over scale z */
#define RADIUS(work) ((work)->scl.z)
#define ROBO_RADIUS 10.0F
#define BUKET_RADIUS 5.58F

/* a robot's beat runs from home out to the set file's scale x one way. it runs
 * out to the set file's scale y the other way. O ROBO A's beat runs across its
 * heading, with scale x at its heading less 90 degrees. O ROBO B's runs along
 * its heading, with scale x ahead. the init raises either scale to BEAT_LEAST
 * if it's less */
#define BEAT_LEAST 10.0F
/* O ROBO A sidesteps its beat SIDESTEP a step. O ROBO B walks its beat 1 a
 * step. both robots walk home WALK_HOME a step */
#define SIDESTEP 0.1F
#define WALK_HOME 1.0F
/* how far a robot turns a step, as Angles. O ROBO B turns about at TURN_ABOUT
 * at either end of its beat. a robot turns toward home at TURN_HOME. at home, O
 * ROBO A turns back to its yaw at TURN_BACK_A, and O ROBO B at TURN_BACK_B */
#define TURN_ABOUT 0x200
#define TURN_HOME 0x200
#define TURN_BACK_A 0x100
#define TURN_BACK_B 0x200
/* a robot rights itself RIGHT_ROBO a step about x and about z, and O BUKET
 * RIGHT_BUKET */
#define RIGHT_ROBO 0x200
#define RIGHT_BUKET 0x400

struct robo_vars {
    NJS_ACTION *action;
    Float frame;
    /* the heading the robot keeps on its beat, at first its heading at home.
     * O ROBO B's beat adds half a turn at either end */
    Angle yaw;
};

#define VARS(work) ((struct robo_vars *)&(work)->vars)

enum robo_mode {
    ROBO_INIT,
    ROBO_WALK,
    ROBO_SLIDE,
    ROBO_RIGHT, /* right itself and turn to face home */
    ROBO_HOME,
    ROBO_TURN_BACK,
    ROBO_STOP /* no code sets it */
};
enum buket_mode {
    BUKET_INIT = 0,
    BUKET_REST = 1,
    BUKET_SLIDE = 2,
    BUKET_RIGHT = 3,
    BUKET_STOP = 6 /* no code sets it */
};

static struct collision_shape robo_a_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OBJECTS |
        PUSH_FIRM(PUSH_PLAYERS | PUSH_OTHERS),
    {0.0F, 0.0F, 0.0F},
    4.6F,
    4.79F,
    0.0F,
    {0, 0, 0},
    0,
    NULL};
/* O ROBO B's capsule, turned a quarter turn about x to lie along z. O ROBO
 * A's stands upright */
static struct collision_shape robo_b_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OBJECTS |
        PUSH_FIRM(PUSH_PLAYERS | PUSH_OTHERS),
    {0.0F, 0.0F, 0.0F},
    4.6F,
    4.79F,
    0.0F,
    {NJM_DEG_ANG(90), 0, 0},
    0,
    NULL};
static struct collision_shape buket_shape = {
    SHAPE_SPHERE,
    0,
    PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OBJECTS |
        PUSH_FIRM(PUSH_PLAYERS | PUSH_OTHERS),
    {0.0F, 0.0F, 0.0F},
    BUKET_RADIUS,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL};

/* draw a prop's MODEL(work)->object at its position and angles. O BUKET, O
 * BOX and O UEKI share it */
static void prop_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_tree(MODEL(work)->object);
    njPopMatrix(1);
}

/* draw the robot's action at its frame. unless the set editor holds the robot,
 * step the frame on, back to 0 after the last. task->draw runs this too, so the
 * action plays while the game is paused */
static void robo_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_action(VARS(work)->action, VARS(work)->frame);
    njPopMatrix(1);
    if (!placement_editing(task)) {
        /* @hack spelling: the frame stepped in the test, which compares the sum
         * still in FR2 (0x0c90b812 to 0x0c90b82a); stepped ahead of it, 13 of
         * 78 words differ */
        if ((VARS(work)->frame += 1.0F) >=
            (Float)VARS(work)->action->motion->nbFrame) {
            VARS(work)->frame = 0.0F;
        }
    }
}

/* the robots' and props' destructor. its task_kill comes to nothing, since
 * task_destroy then frees the task */
static void robo_destroy(struct task *task)
{
    task_kill(task);
}

/* turn work->ang.y TURN_HOME toward home, the point in the task's extra.
 * return the turn still left, 0 once the robot faces home */
static int robo_face_home(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *home;
    NJS_POINT3 *goal;
    Angle heading;

    /* @hack redundant: home's and goal's addresses taken, which keeps both in
     * the frame: the image stores the extra's address twice (0x0c90b8ae,
     * 0x0c90b8b0); as one local whose address isn't taken, 38 of 40 words
     * differ */
    (void)&home;
    (void)&goal;
    goal = home = task->extra;
    heading = (Angle)(atan2f(goal->x - work->pos.x, goal->z - work->pos.z) *
                      ANGLE_PER_RADIAN);
    work->ang.y = angle_approach(work->ang.y, heading, TURN_HOME);
    return angle_distance(work->ang.y, heading);
}

/* move work->pos into the task's lane, at least RADIUS inside each edge */
static void robo_keep_in_lane(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: the point read through a byte cursor, stepped back after
     * and forward again to write it back from z; through the members, 19 of
     * 49 words differ */
    char *at = (char *)&work->pos;
    Float x;
    Float y;
    Float z;
    Float radius;
    Float min_x;
    Float max_x;
    Float min_z;
    Float max_z;

    /* @hack redundant: three tests of work that never run, which weigh it
     * so that work takes R5 and at R4 (0x0c90b8f6, 0x0c90b8f8); without
     * them, 23 of 49 words differ */
    if (0) {
        if (work) {
        }
        if (work) {
        }
        if (work) {
        }
    }
    radius = RADIUS(work);
    x = *(Float *)at;
    at += sizeof(Float);
    y = *(Float *)at;
    at += sizeof(Float);
    z = *(Float *)at;
    at += sizeof(Float);
    at -= 3 * sizeof(Float);
    min_x = LANE_X0 + radius;
    max_x = LANE_X1 - radius;
    if (!SIDE(work)) {
        min_z = -LANE_Z1 + radius;
        max_z = -LANE_Z0 - radius;
    } else {
        min_z = LANE_Z0 + radius;
        max_z = LANE_Z1 - radius;
    }
    if (x < min_x) {
        x = min_x;
    }
    if (x > max_x) {
        x = max_x;
    }
    if (z < min_z) {
        z = min_z;
    }
    if (z > max_z) {
        z = max_z;
    }
    at += 3 * sizeof(Float);
    at -= sizeof(Float);
    *(Float *)at = z;
    at -= sizeof(Float);
    *(Float *)at = y;
    at -= sizeof(Float);
    *(Float *)at = x;
}

/* O ROBO A's step. the robot sidesteps its beat across its heading, at
 * home's height. home is the point in the task's extra, where the init puts
 * its position */
void robo_a_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *home = task->extra;
    Angle ang;
    Float speed;
    /* @hack spelling: njSin called through sine, which loads its address into
     * R8 once (0x0c90ba44), and each call goes through R8; called by name, 365
     * of 523 words and 75 of 149 other words differ */
    Float (*sine)(Angle);
    /* @hack spelling: angle_approach called through approach, which loads its
     * address into R10 once (0x0c90ba42), and each call goes through R10;
     * called by name, 16 of 523 words and 4 of 149 other words differ */
    Angle (*approach)(Angle, Angle, Angle);

    if (placement_try_leave_by(task, ROBO_RANGE)) {
        return;
    }
    if (edit_mode) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        njEasyDrawObject(&robo_a_object);
        njPopMatrix(1);
    } else {
        approach = angle_approach;
        sine = njSin;
        switch (work->mode) {
        case ROBO_INIT:
            work->mode = ROBO_WALK;
            VARS(work)->action = &robo_a_action;
            task->destructor = robo_destroy;
            task->draw = robo_draw;
            collision_init(task, &robo_a_shape, COLLISION_OBJECT, 1, 1);
            if (work->scl.x < BEAT_LEAST) {
                work->scl.x = BEAT_LEAST;
            }
            if (work->scl.y < BEAT_LEAST) {
                work->scl.y = BEAT_LEAST;
            }
            if (RADIUS(work) > 0.0F) {
                SIDE(work) = 1;
            }
            RADIUS(work) = ROBO_RADIUS;
            home->x = work->pos.x;
            home->y = work->pos.y;
            home->z = work->pos.z;
            VARS(work)->yaw = work->ang.y;
            break;
        case ROBO_WALK:
            speed = SIDESTEP;
            switch (work->submode) {
            case 0:
                work->pos.x += sine(work->ang.y - NJM_DEG_ANG(90)) * speed;
                work->pos.z += njCos(work->ang.y - NJM_DEG_ANG(90)) * speed;
                if (njDistanceP2P(&work->pos, home) > work->scl.x) {
                    work->submode = 1;
                }
                break;
            case 1:
                work->pos.x -= sine(work->ang.y - NJM_DEG_ANG(90)) * speed;
                work->pos.z -= njCos(work->ang.y - NJM_DEG_ANG(90)) * speed;
                if (njDistanceP2P(&work->pos, home) < work->scl.y) {
                    work->submode = 2;
                }
                break;
            case 2:
                work->pos.x -= sine(work->ang.y - NJM_DEG_ANG(90)) * speed;
                work->pos.z -= njCos(work->ang.y - NJM_DEG_ANG(90)) * speed;
                if (njDistanceP2P(&work->pos, home) > work->scl.y) {
                    work->submode = 3;
                }
                break;
            case 3:
                work->pos.x += sine(work->ang.y - NJM_DEG_ANG(90)) * speed;
                work->pos.z += njCos(work->ang.y - NJM_DEG_ANG(90)) * speed;
                if (njDistanceP2P(&work->pos, home) < work->scl.x) {
                    work->submode = 0;
                }
                break;
            }
            if (world_gravity_ang_x || world_gravity_ang_z) {
                work->mode = ROBO_SLIDE;
            }
            robo_draw(task);
            collision_register(work);
            break;
        case ROBO_SLIDE:
            if (world_gravity_ang_x == 0 && world_gravity_ang_z == 0) {
                work->mode = ROBO_RIGHT;
            } else {
                work->pos.x += world_gravity.x;
                work->pos.z += world_gravity.z;
                work->ang.x -= world_gravity_ang_x >> 3;
                work->ang.z -= world_gravity_ang_z >> 3;
            }
            robo_draw(task);
            collision_register(work);
            break;
        case ROBO_RIGHT:
            work->ang.x = approach(work->ang.x, 0, RIGHT_ROBO);
            work->ang.z = approach(work->ang.z, 0, RIGHT_ROBO);
            if (!robo_face_home(task) && !work->ang.x && !work->ang.z) {
                work->mode = ROBO_HOME;
            }
            if (world_gravity_ang_x || world_gravity_ang_z) {
                work->mode = ROBO_SLIDE;
            }
            robo_draw(task);
            collision_register(work);
            break;
        case ROBO_HOME:
            speed = WALK_HOME;
            if (njDistanceP2P(&work->pos, home) <= speed) {
                work->pos.x = home->x;
                work->pos.y = home->y;
                work->pos.z = home->z;
                work->mode = ROBO_TURN_BACK;
            } else {
                work->pos.x += sine(work->ang.y) * speed;
                work->pos.z += njCos(work->ang.y) * speed;
            }
            if (world_gravity_ang_x || world_gravity_ang_z) {
                work->mode = ROBO_SLIDE;
            }
            robo_draw(task);
            collision_register(work);
            break;
        case ROBO_TURN_BACK:
            if (angle_distance(work->ang.y, VARS(work)->yaw) == 0) {
                work->mode = ROBO_WALK;
            } else {
                work->ang.y =
                    approach(work->ang.y, VARS(work)->yaw, TURN_BACK_A);
            }
            if (world_gravity_ang_x || world_gravity_ang_z) {
                work->mode = ROBO_SLIDE;
            }
            robo_draw(task);
            collision_register(work);
            break;
        case ROBO_STOP:
        default:
            break;
        }
        /* @hack redundant: a never-run test of the tilt, which leaves the
         * image's registers to the calls' addresses: collision_register's R9
         * and angle_approach's R10 (0x0c90ba3e, 0x0c90ba42), which otherwise
         * trade; without it, 15 of 523 words differ */
        if (0) {
            if (world_gravity_ang_x) {
            }
        }
        robo_keep_in_lane(task);
        work->pos.y = home->y;
    }
}

/* draw O ROBO B with the constant material's alpha at -1, then set it back
 * to 0. while the robot walks, its step sets NJD_FLAG_USE_ALPHA on the first
 * material of its model two children down. this draw clears it */
static void robo_b_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB material;

    material.a = -1.0F;
    material.r = 0.0F;
    material.g = 0.0F;
    material.b = 0.0F;
    njSetConstantMaterial(&material);
    robo_draw(task);
    material.a = 0.0F;
    njSetConstantMaterial(&material);
    VARS(work)->action->object->child->child->model->mats[0].attrflags &=
        ~NJD_FLAG_USE_ALPHA;
}

/* O ROBO B's step. the robot walks its beat along its heading, at home's
 * height, and turns about at either end. home is the point in the task's
 * extra, where the init puts its position */
void robo_b_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 *home = task->extra;
    Angle ang;
    Float speed;

    if (placement_try_leave_by(task, ROBO_RANGE)) {
        return;
    }
    if (edit_mode) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        njEasyDrawObject(&robo_b_object);
        njPopMatrix(1);
    } else {
        switch (work->mode) {
        case ROBO_INIT:
            work->mode = ROBO_WALK;
            VARS(work)->action = &robo_b_action;
            task->destructor = robo_destroy;
            task->draw = robo_draw;
            collision_init(task, &robo_b_shape, COLLISION_OBJECT, 1, 1);
            if (work->scl.x < BEAT_LEAST) {
                work->scl.x = BEAT_LEAST;
            }
            if (work->scl.y < BEAT_LEAST) {
                work->scl.y = BEAT_LEAST;
            }
            if (RADIUS(work) > 0.0F) {
                SIDE(work) = 1;
            }
            RADIUS(work) = ROBO_RADIUS;
            home->x = work->pos.x;
            home->y = work->pos.y;
            home->z = work->pos.z;
            VARS(work)->yaw = work->ang.y;
            break;
        case ROBO_WALK:
            switch (work->submode) {
            case 0:
                work->pos.x += njSin(work->ang.y);
                work->pos.z += njCos(work->ang.y);
                VARS(work)
                    ->action->object->child->child->model->mats[0]
                    .attrflags |= NJD_FLAG_USE_ALPHA;
                if (njDistanceP2P(&work->pos, home) > work->scl.x) {
                    work->submode = 1;
                    VARS(work)->yaw += NJM_DEG_ANG(180);
                }
                break;
            case 1:
                work->ang.y += TURN_ABOUT;
                if (angle_distance(work->ang.y, VARS(work)->yaw) == 0) {
                    work->submode = 2;
                }
                break;
            case 2:
                work->pos.x += njSin(work->ang.y);
                work->pos.z += njCos(work->ang.y);
                VARS(work)
                    ->action->object->child->child->model->mats[0]
                    .attrflags |= NJD_FLAG_USE_ALPHA;
                if (njDistanceP2P(&work->pos, home) < work->scl.y) {
                    work->submode = 3;
                }
                break;
            case 3:
                work->pos.x += njSin(work->ang.y);
                work->pos.z += njCos(work->ang.y);
                VARS(work)
                    ->action->object->child->child->model->mats[0]
                    .attrflags |= NJD_FLAG_USE_ALPHA;
                if (njDistanceP2P(&work->pos, home) > work->scl.y) {
                    work->submode = 4;
                    VARS(work)->yaw += NJM_DEG_ANG(180);
                }
                break;
            case 4:
                work->ang.y += TURN_ABOUT;
                if (angle_distance(work->ang.y, VARS(work)->yaw) == 0) {
                    work->submode = 5;
                }
                break;
            case 5:
                work->pos.x += njSin(work->ang.y);
                work->pos.z += njCos(work->ang.y);
                VARS(work)
                    ->action->object->child->child->model->mats[0]
                    .attrflags |= NJD_FLAG_USE_ALPHA;
                if (njDistanceP2P(&work->pos, home) < work->scl.x) {
                    work->submode = 0;
                }
                break;
            }
            if (world_gravity_ang_x || world_gravity_ang_z) {
                work->mode = ROBO_SLIDE;
            }
            robo_b_draw(task);
            collision_register(work);
            break;
        case ROBO_SLIDE:
            if (world_gravity_ang_x == 0 && world_gravity_ang_z == 0) {
                work->mode = ROBO_RIGHT;
            } else {
                work->pos.x += world_gravity.x;
                work->pos.z += world_gravity.z;
                work->ang.x -= world_gravity_ang_x;
                work->ang.z -= world_gravity_ang_z;
            }
            robo_draw(task);
            collision_register(work);
            break;
        case ROBO_RIGHT:
            work->ang.x = angle_approach(work->ang.x, 0, RIGHT_ROBO);
            work->ang.z = angle_approach(work->ang.z, 0, RIGHT_ROBO);
            if (!robo_face_home(task) && !work->ang.x && !work->ang.z) {
                work->mode = ROBO_HOME;
            }
            if (world_gravity_ang_x || world_gravity_ang_z) {
                work->mode = ROBO_SLIDE;
            }
            robo_draw(task);
            collision_register(work);
            break;
        case ROBO_HOME:
            speed = WALK_HOME;
            if (njDistanceP2P(&work->pos, home) <= speed) {
                work->pos.x = home->x;
                work->pos.y = home->y;
                work->pos.z = home->z;
                work->mode = ROBO_TURN_BACK;
            } else {
                work->pos.x += njSin(work->ang.y) * speed;
                work->pos.z += njCos(work->ang.y) * speed;
            }
            VARS(work)
                ->action->object->child->child->model->mats[0]
                .attrflags |= NJD_FLAG_USE_ALPHA;
            if (world_gravity_ang_x || world_gravity_ang_z) {
                work->mode = ROBO_SLIDE;
            }
            robo_b_draw(task);
            collision_register(work);
            break;
        case ROBO_TURN_BACK:
            if (angle_distance(work->ang.y, VARS(work)->yaw) == 0) {
                work->mode = ROBO_WALK;
            } else {
                work->ang.y =
                    angle_approach(work->ang.y, VARS(work)->yaw, TURN_BACK_B);
            }
            if (world_gravity_ang_x || world_gravity_ang_z) {
                work->mode = ROBO_SLIDE;
            }
            robo_draw(task);
            collision_register(work);
            break;
        case ROBO_STOP:
        default:
            break;
        }
        robo_keep_in_lane(task);
        work->pos.y = home->y;
    }
}

/* O BUKET's step. the bucket splashes as it slides. it keeps the height it
 * starts at, which the init saves in scl.y */
void buket_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;
    /* @hack spelling: world_gravity_ang_x read through tilt_x, which loads its
     * address into R12 ahead of the switch (0x0c90c5ee); read by name, 156 of
     * 222 words and 78 of 82 other words differ, and the function ends 32
     * bytes early */
    Angle *tilt_x;
    /* @hack spelling: collision_register called through collide, which loads
     * its address into R11 ahead of the switch (0x0c90c5f0); called by name, 2
     * of 222 words and 4 of 82 other words differ */
    void (*collide)(struct task_work *);

    if (placement_try_leave_by(task, ROBO_RANGE)) {
        return;
    }
    if (edit_mode) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        njEasyDrawObject(&buket_object);
        njPopMatrix(1);
    } else {
        collide = collision_register;
        tilt_x = &world_gravity_ang_x;
        switch (work->mode) {
        case BUKET_INIT:
            work->mode = BUKET_REST;
            MODEL(work)->object = &buket_object;
            task->destructor = robo_destroy;
            task->draw = prop_draw;
            /* NOLINTBEGIN(*-redundant-expression,*-literal-conversion) */
            /* @hack redundant: the kind as a select whose condition is
             * constant, which loads collision_init's address just before the
             * call (0x0c90c62a); as COLLISION_OBJECT, 8 of 222 words differ */
            /* @hack redundant: the count as a select whose condition is
             * constant, which loads its own 1 (MOV #1,R1 at 0x0c90c624)
             * rather than share the mode's in R10; as 1, 150 of 222 words and
             * 78 of 82 other words differ, and the function ends 32 bytes
             * early */
            collision_init(task, &buket_shape,
                           2.0F ? COLLISION_OBJECT : COLLISION_OBJECT, 1,
                           1.0F ? 1 : 1);
            /* NOLINTEND(*-redundant-expression,*-literal-conversion) */
            work->scl.y = work->pos.y;
            if (RADIUS(work) > 0.0F) {
                SIDE(work) = 1;
            }
            RADIUS(work) = BUKET_RADIUS;
            break;
        case BUKET_REST:
            if (*tilt_x || world_gravity_ang_z) {
                work->mode = BUKET_SLIDE;
            }
            prop_draw(task);
            collide(work);
            break;
        case BUKET_SLIDE:
            if (!*tilt_x && !world_gravity_ang_z) {
                work->mode = BUKET_RIGHT;
            } else {
                work->pos.x += world_gravity.x;
                work->pos.z += world_gravity.z;
                work->ang.x -= *tilt_x;
                work->ang.z -= world_gravity_ang_z;
            }
            splash_create(&work->pos, &world_gravity, 1.0F);
            prop_draw(task);
            collide(work);
            break;
        case BUKET_RIGHT:
            work->ang.x = angle_approach(work->ang.x, 0, RIGHT_BUKET);
            work->ang.z = angle_approach(work->ang.z, 0, RIGHT_BUKET);
            if (!work->ang.x && !work->ang.z) {
                work->mode = BUKET_REST;
            }
            if (*tilt_x || world_gravity_ang_z) {
                work->mode = BUKET_SLIDE;
            }
            prop_draw(task);
            collide(work);
            break;
        case BUKET_STOP:
        default:
            break;
        }
        robo_keep_in_lane(task);
        work->pos.y = work->scl.y;
    }
}

static struct collision_shape box_shape = {
    SHAPE_SPHERE,
    0,
    PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OBJECTS |
        PUSH_FIRM(PUSH_PLAYERS | PUSH_OTHERS),
    {0.0F, 0.0F, 0.0F},
    12.31F,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};

/* the square of the distance from the camera or player 0 within which O BOX
 * stays */
#define BOX_RANGE (1010.0F * 1010.0F)
#define BOX_RADIUS 12.313767F
/* how far O BOX rights itself a step, about x and about z */
#define RIGHT_BOX 0x400

enum box_mode {
    BOX_INIT = 0,
    BOX_REST = 1,
    BOX_SLIDE = 2,
    BOX_RIGHT = 3,
    BOX_STOP = 6 /* no code sets it */
};

/* O BOX's step. the box slides without tumbling. once the deck is level
 * after a slide, it rights any tilt the set file gave it. it keeps the height
 * it starts at, which the init saves in scl.y */
void box_step(struct task *task)
{
    /* @hack redundant: register on work, which gives collision_register's
     * address R12 and the tilt's R10 (0x0c90c84e, 0x0c90c850); without it,
     * the two trade and 6 of 211 words differ */
    register struct task_work *work = task->work;
    Angle ang;
    /* @hack spelling: world_gravity_ang_x read through tilt_x, which loads its
     * address into R10 ahead of the switch (0x0c90c850); read by name, 206 of
     * 211 words and 75 of 77 other words differ, and the function ends 32
     * bytes early */
    Angle *tilt_x;
    /* @hack spelling: collision_register called through collide, which loads
     * its address into R12 ahead of the switch (0x0c90c84e); called by name,
     * 178 of 211 words and 18 of 77 other words differ */
    void (*collide)(struct task_work *);

    if (placement_try_leave_by(task, BOX_RANGE)) {
        return;
    }
    if (edit_mode) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        njEasyDrawObject(&box_object);
        njPopMatrix(1);
    } else {
        tilt_x = &world_gravity_ang_x;
        collide = collision_register;
        switch (work->mode) {
        case BOX_INIT:
            work->mode = BOX_REST;
            MODEL(work)->object = &box_object;
            task->destructor = robo_destroy;
            /* NOLINTBEGIN(*-redundant-expression,*-literal-conversion) */
            /* @hack redundant: the draw as a select whose condition is
             * constant, which loads the call's 1 after the draw's address
             * (0x0c90c884), with the tilt's address in R10 and the mode's 1
             * in R11; as prop_draw, the 1 loads first, into R2, the two
             * trade, and 17 of 211 words differ */
            task->draw = 1.0F ? prop_draw : NULL;
            /* @hack redundant: the count as a test of constants, which loads
             * its own 1 (MOV #1,R1 at 0x0c90c884) rather than share the
             * mode's in R11; as 1, 75 of 211 words differ */
            collision_init(task, &box_shape, COLLISION_OBJECT, 1, 1.0F && 1);
            /* NOLINTEND(*-redundant-expression,*-literal-conversion) */
            work->scl.y = work->pos.y;
            if (RADIUS(work) > 0.0F) {
                SIDE(work) = 1;
            }
            RADIUS(work) = BOX_RADIUS;
            break;
        case BOX_REST:
            if (*tilt_x || world_gravity_ang_z) {
                work->mode = BOX_SLIDE;
                /* @hack redundant: a never-run store of BOX_INIT, which keeps
                 * SHC from storing the mode in the branch's delay slot, where
                 * the image doesn't; without it, 32 of 211 words differ */
                if (0) {
                    work->mode = BOX_INIT;
                }
            }
            /* @hack spelling: a jump to BOX_RIGHT's draw and collision, whose
             * one copy both cases share (BRA at 0x0c90c8ca to 0x0c90c9b8); with
             * the two calls here and a break, 111 of 211 words and 59 of 77
             * other words differ */
            goto draw;
        case BOX_SLIDE:
            if (!*tilt_x && !world_gravity_ang_z) {
                work->mode = BOX_RIGHT;
            } else {
                work->pos.x += world_gravity.x;
                work->pos.z += world_gravity.z;
            }
            prop_draw(task);
            collide(work);
            break;
        case BOX_RIGHT:
            work->ang.x = angle_approach(work->ang.x, 0, RIGHT_BOX);
            work->ang.z = angle_approach(work->ang.z, 0, RIGHT_BOX);
            if (!work->ang.x && !work->ang.z) {
                work->mode = BOX_REST;
            }
            if (*tilt_x || world_gravity_ang_z) {
                work->mode = BOX_SLIDE;
            }
        draw:
            prop_draw(task);
            collide(work);
            break;
        case BOX_STOP:
        default:
            break;
        }
        robo_keep_in_lane(task);
        work->pos.y = work->scl.y;
    }
}

/* the square of the distance from the camera or player 0 within which O UEKI
 * stays */
#define UEKI_RANGE (1010.0F * 1010.0F)
#define UEKI_RADIUS 11.57098F

enum ueki_mode {
    UEKI_INIT = 0,
    UEKI_REST = 1,
    UEKI_SLIDE = 2,
    UEKI_STOP = 6 /* no code sets it */
};

static struct collision_shape ueki_shape = {
    SHAPE_SPHERE,
    0,
    PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OBJECTS |
        PUSH_FIRM(PUSH_PLAYERS | PUSH_OTHERS),
    {0.0F, 0.0F, 0.0F},
    11.57F,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL,
};

/* O UEKI's step. it slides without tumbling, and never rights itself. it
 * keeps the height it starts at, which the init saves in scl.y */
void ueki_step(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (placement_try_leave_by(task, UEKI_RANGE)) {
        return;
    }
    if (edit_mode) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateY, work->ang.y, ang);
        texture_set_stage();
        njEasyDrawObject(&ueki_object);
        njPopMatrix(1);
    } else {
        /* @hack spelling: world_gravity_ang_x read through tilt_x, which loads
         * its address into R4 ahead of the switch (0x0c90ca86); read by name,
         * 60 of 172 words differ */
        Angle *tilt_x = &world_gravity_ang_x;

        switch (work->mode) {
        case UEKI_INIT:
            work->mode = UEKI_REST;
            MODEL(work)->object = &ueki_object;
            task->destructor = robo_destroy;
            /* NOLINTBEGIN(*-redundant-expression,*-literal-conversion) */
            /* @hack redundant: the draw as a select whose condition is
             * constant, which loads the call's 1 after the draw's address
             * (0x0c90cab2); as prop_draw, the 1 loads first, into R2, and 10
             * of 172 words differ */
            task->draw = 1.0F ? prop_draw : NULL;
            /* @hack redundant: the count as a test of constants, which loads
             * its own 1 (MOV #1,R1 at 0x0c90cab2) rather than share the
             * mode's in R12; as 1, 57 of 172 words differ */
            collision_init(task, &ueki_shape, COLLISION_OBJECT, 1, 1.0F && 1);
            /* NOLINTEND(*-redundant-expression,*-literal-conversion) */
            work->scl.y = work->pos.y;
            if (RADIUS(work) > 0.0F) {
                SIDE(work) = 1;
            }
            RADIUS(work) = UEKI_RADIUS;
            break;
        case UEKI_REST:
            if (*tilt_x || world_gravity_ang_z) {
                work->mode = UEKI_SLIDE;
            }
            prop_draw(task);
            collision_register(work);
            break;
        case UEKI_SLIDE:
            if (!*tilt_x && !world_gravity_ang_z) {
                work->mode = UEKI_REST;
            } else {
                work->pos.x += world_gravity.x;
                work->pos.z += world_gravity.z;
            }
            prop_draw(task);
            collision_register(work);
            break;
        case UEKI_STOP:
            break;
        }
        robo_keep_in_lane(task);
        work->pos.y = work->scl.y;
    }
}
