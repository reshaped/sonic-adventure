/* Zero's arms. each hand hangs on a chain of NODES nodes. a thrown hand
 * flies at the marker's lock and drags its chain. the chain's nodes pull on
 * each other as springs. a spread lays both chains out to the sides.
 * spheres along the spread chains hurt the player, and a light runs along
 * them */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_robo/arms.h"
#include "b_robo/banks.h"
#include "b_robo/effects.h"
#include "b_robo/erobo.h"
#include "b_robo/marker.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/sprite.h"
#include "object/models.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the modes erobo_arms_step moves on to, past erobo_arms_mode's */
enum {
    ARMS_THROWN = EROBO_ARMS_RETURN + 1, /* hand 0 flies at the lock */
    ARMS_RECALLED, /* hand 0 flies home */
    /* the links grow. it lasts one step, since erobo_arms_step takes
     * arms_unfurl's result as true at once */
    ARMS_UNFURL,
    ARMS_SPREAD, /* both chains stay laid out */
    ARMS_HOLD, /* as ARMS_SPREAD, for HOLD_STEPS steps */
    ARMS_HOMING /* both hands fly home */
};

/* the nodes of a chain, from node 0 at its hand to node LINKS at its arm's
 * end */
#define NODES 16
#define LINKS (NODES - 1)

/* how far from its arm's end a spread chain aims, across the deck */
#define REACH 230.0F
/* the length a link grows to, a sixteenth of REACH */
#define LINK_MOST (REACH / 16.0F)
/* the steps the links take to grow to LINK_MOST, which arms_unfurl counts
 * down */
#define UNFURL_STEPS 60
/* how many steps the spread holds after EROBO_ARMS_RETURN */
#define HOLD_STEPS 30
/* a spread hand aims a quarter turn off Zero's heading. hand 0 takes it
 * off, and hand 1 adds it */
#define SPREAD_TURN 0x4000

/* a flying hand's speed, and a returning hand's within HOME_NEAR of its
 * arm's end */
#define HAND_SPEED 4.0F
#define HAND_SLOW 1.0F
#define HOME_NEAR 30.0F
/* a returning hand is home once a step starts this near its arm's end */
#define HOME_REACHED 1.5F
/* a thrown hand turns back within LOCK_NEAR of the lock, across the deck.
 * it turns back past THROW_REACH from its arm's end too */
#define LOCK_NEAR 5.0F
#define THROW_REACH 200.0F
/* a flying hand turns its direction this part of the way toward its goal
 * each step. its goal is the lock or its arm's end */
#define HAND_STEER 0.175F
/* added to the y of a returning hand's old direction when its goal lies
 * exactly the other way. the blend toward the goal would keep that
 * direction otherwise */
#define REVERSE_LIFT 0.1F
/* vector_unit returns the reciprocal of a vector's length. past this, the
 * vector is shorter than 0.01 and has no direction to keep */
#define NO_DIRECTION 100.0F

/* a dragged chain's springs. each step, a node's velocity gains SPRING of
 * its distance to each neighbor, and loses DRAG of itself */
#define SPRING 0.7F
#define DRAG 0.15F
/* a laid chain's node turns this part of the way to the heading of the
 * node before it */
#define LAY_TURN 0.25F
/* how much lower a laid chain's node is than the one before, at a link
 * length of LINK_MOST */
#define LAY_SAG 0.8125F

/* how far above the floor below it a hand stays. NO_FLOOR is hand_floor
 * while the hand has no floor to keep above */
#define HAND_LIFT 5.0F
#define NO_FLOOR (-9999.9F)
/* the space between a thrown hand's puffs of dust */
#define DUST_GAP 2.0F

/* the offset of a link's object in the model bank */
#define LINK_OBJECT 0x161c
/* a link object's x scale for each unit of distance it spans */
#define LINK_SCALE 0.1F

/* sound effects, named as in src/enemy/zero.c */
#define SE_REEL 761
#define SE_LATCH 763

struct chain_node {
    NJS_POINT3 pos;
    struct angles ang;
    NJS_VECTOR velocity;
    NJS_VECTOR force; /* the springs' pull this step */
};

/* a point the light along a chain passes, and the spark there */
struct chain_spark {
    NJS_POINT3 pos;
    struct sprite sprite;
};

/* the steps the light takes to reach one spark further, or to fall back by
 * one */
#define LIGHT_GROW_STEPS 3
#define LIGHT_FADE_STEPS 4
/* how far a spark may sit off its node along each axis, and its scale */
#define SPARK_JITTER 5.0F
#define SPARK_SCALE 0.3F

/* each hand's objects, as offsets into the model bank */
#define HAND_OBJECTS 4
static const Uint32 hand_objects[2][HAND_OBJECTS] = {
    {0x89fc, 0x8994, 0x8960, 0x89c8},
    {0x8c6c, 0x8c04, 0x8bd0, 0x8c38},
};
/* the direction both hands fly in as they start home from a spread:
 * straight up */
static const NJS_VECTOR up = {0.0F, 1.0F, 0.0F};
static const NJS_POINT3 origin = {0.0F, 0.0F, 0.0F};

static int arms_mode = EROBO_ARMS_IDLE;

/* a sphere at a point in the world, which hurts players and can't be
 * pushed. chain_shapes holds one for each node of the two chains, hand 0's
 * and hand 1's alternating */
#define CHAIN_SHAPE                                                            \
    {SHAPE_SPHERE | SHAPE_IN_WORLD | SHAPE_HURTABLE,                           \
     SHAPE_HURTS_PLAYERS | SHAPE_ATTACK,                                       \
     PUSH_FIRM(PUSH_ALL),                                                      \
     {0.0F, 0.0F, 0.0F},                                                       \
     7.0F,                                                                     \
     0.0F,                                                                     \
     0.0F,                                                                     \
     {0, 0, 0},                                                                \
     0,                                                                        \
     NULL}
/* how far below its node a sphere sits */
#define SPHERE_DROP 5.0F
static struct collision_shape chain_shapes[2 * NODES] = {
    CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE,
    CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE,
    CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE,
    CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE,
    CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE,
    CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE, CHAIN_SHAPE,
    CHAIN_SHAPE, CHAIN_SHAPE,
};

/* the task that holds the spheres, from chain_shapes_create until it is
 * destroyed, else NULL */
static struct task *shapes_task = NULL;
/* while TRUE, the spheres' task registers them on each frame's first
 * step */
static Bool shapes_active = FALSE;
/* the light grows along the chains while TRUE, and dies back while FALSE */
static Bool sparks_growing = FALSE;
/* the task that draws the light, from sparks_create until it is destroyed,
 * else NULL */
static struct task *sparks_task = NULL;
/* how many of each chain's sparks the light reaches, counted from the
 * hand */
static int sparks_lit = 0;

static NJS_POINT3 hand_pos[2];
static NJS_VECTOR hand_velocity[2];
static struct angles hand_ang[2];
/* the least height of each hand. hand_draw sets it HAND_LIFT above the
 * floor below the hand. a spread's start sets NO_FLOOR, as does a throw's
 * unless the lock is too near to aim at */
static Float hand_floor[2];
static struct chain_node chains[2][NODES];
/* the links' length, which grows to LINK_MOST as a spread unfurls */
static Float link_length;
static int arms_timer;
/* whether each hand is back on its arm since its spread started */
static Bool hands_home[2];
static struct chain_spark sparks[2][NODES];

static void arms_spread_start(void);
static Bool arms_unfurl(void);
static void arms_spread(void);
static Bool hands_return(void);
static void hand_throw(int arm);
static Bool hand_fly(int arm);
static Bool hand_return(int arm);
static void chain_reset(int arm, NJS_POINT3 *pos);
static void chain_step(int arm, NJS_POINT3 *root, NJS_POINT3 *tip);
static void chain_aim(int arm, NJS_POINT3 *target);
static void chain_lay(int arm, NJS_POINT3 *root, NJS_POINT3 *target,
                      NJS_POINT3 *tip);
static void links_draw(int arm);
static void chain_shapes_clear(void);
static void chain_shape_set(int arm, int index, struct chain_node *node);
static void chain_shapes_create(void);
static void chain_shapes_destroy(struct task *task);
static void chain_shapes_step(struct task *task);
static void sparks_create(void);
static void sparks_destroy(struct task *task);

static void hand_show(int arm)
{
    const Uint32 *offset = hand_objects[arm];
    int count = HAND_OBJECTS;

    while (count--) {
        ((NJS_OBJECT *)(erobo_models + *offset++))->evalflags &= ~NJD_EVAL_HIDE;
    }
}

static void hand_hide(int arm)
{
    const Uint32 *offset = hand_objects[arm];
    int count = HAND_OBJECTS;

    while (count--) {
        ((NJS_OBJECT *)(erobo_models + *offset++))->evalflags |= NJD_EVAL_HIDE;
    }
}

void erobo_arms_set_mode(enum erobo_arms_mode mode)
{
    arms_mode = mode;
}

int erobo_arms_get_mode(void)
{
    return arms_mode;
}

void erobo_arms_start(void)
{
    arms_mode = EROBO_ARMS_IDLE;
    chain_shapes_create();
    sparks_create();
}

void erobo_arms_end(void)
{
}

void erobo_arms_step(void)
{
    switch (arms_mode) {
    case EROBO_ARMS_IDLE:
        break;
    case EROBO_ARMS_THROW:
        hand_throw(0);
        hand_hide(0);
        arms_mode = ARMS_THROWN;
        break;
    case EROBO_ARMS_SPREAD: {
        Bool on;

        arms_spread_start();
        hand_hide(0);
        hand_hide(1);
        chain_shapes_clear();
        /* @hack spelling: TRUE through a local, which both stores take from
         * R4 (set at 0x0c907316); stored directly, 48 of 186 words differ */
        on = TRUE;
        shapes_active = on;
        sparks_growing = on;
        erobo_sparkles_set_points(hand_pos);
        arms_mode = ARMS_UNFURL;
        break;
    }
    case EROBO_ARMS_RETURN: {
        NJS_VECTOR *base = hand_velocity;
        NJS_VECTOR *velocity = base;

        VECTOR_COPY(velocity, &up);
        velocity++;
        VECTOR_COPY(velocity, &up);
        arms_timer = HOLD_STEPS;
        arms_mode = ARMS_HOLD;
        /* @hack redundant: a test of base that never runs, with an empty
         * else. its read of base keeps base in R5 (loaded at 0x0c90732e) and
         * velocity in R4 (copied at 0x0c907334). its else has case
         * ARMS_THROWN load the ARMS_RECALLED it stores into R2 (MOV #5,R2 at
         * 0x0c90736e). without it, 8 of 186 words differ */
        if (0) {
            if (base) {
            } else {
            }
        }
        break;
    }
    case ARMS_THROWN:
        erobo_set_hand_shape(0, &hand_pos[0]);
        if (hand_fly(0)) {
            arms_mode = ARMS_RECALLED;
        }
        break;
    case ARMS_RECALLED:
        erobo_set_hand_shape(0, &hand_pos[0]);
        if (hand_return(0)) {
            hand_show(0);
            sound_play_at(SE_LATCH, 0, 0, 0, hand_pos[0].x, hand_pos[0].y,
                          hand_pos[0].z);
            arms_mode = EROBO_ARMS_IDLE;
        }
        break;
    case ARMS_UNFURL:
        erobo_set_hand_shape(0, &hand_pos[0]);
        erobo_set_hand_shape(1, &hand_pos[1]);
        if (arms_unfurl()) {
            arms_mode = ARMS_SPREAD;
        }
        break;
    case ARMS_SPREAD:
        arms_spread();
        break;
    case ARMS_HOLD:
        arms_spread();
        if (--arms_timer <= 0) {
            /* @hack spelling: FALSE through a local for the first store
             * alone, which gives the two stores R4 and its copy in R5
             * (0x0c907404); stored directly in both, 8 of 186 words
             * differ */
            Bool off = FALSE;

            shapes_active = off;
            sparks_growing = FALSE;
            arms_mode = ARMS_HOMING;
        }
        break;
    case ARMS_HOMING:
        if (hands_return()) {
            sound_play(SE_LATCH, 0, 0, 0);
            arms_mode = EROBO_ARMS_IDLE;
        }
        break;
    }
    switch (arms_mode) {
    case ARMS_THROWN:
    case ARMS_RECALLED:
    case ARMS_HOMING: {
        NJS_POINT3 *pos = &hand_pos[0];

        sound_play_at_for(SE_REEL, (int)pos, 1, 0, pos->x, pos->y, pos->z, 4);
        break;
    }
    }
}

/* draw a shadow on the floor below pos, at scale shrunk by pos's height
 * above the floor. returns the floor's height */
static Float shadow_draw_under(NJS_POINT3 *pos, Float scale)
{
    struct angles slope;
    Float floor = ground_below(pos->x, pos->y + 4.0F, pos->z, &slope);

    scale /= 1.0F + (njAbs(pos->y - floor) * 0.03F);
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslate(NULL, pos->x, floor + SHADOW_LIFT, pos->z);
    TURN(njRotateZ, slope.z);
    TURN(njRotateX, slope.x);
    njScale(NULL, scale, scale, scale);
    njEasyDrawObject(shadow_object);
    njPopMatrix(1);
    return floor;
}

/* draw hand 0 or 1 and its shadow, and set its hand_floor. while hand 0
 * flies at the lock, it leaves puffs of dust DUST_GAP apart. they fill the
 * way it came this step */
static void hand_draw(int arm)
{
    NJS_POINT3 *pos = &hand_pos[arm];
    NJS_VECTOR *velocity = &hand_velocity[arm];
    struct angles *ang = &hand_ang[arm];
    NJS_OBJECT *object;
    NJS_VECTOR step;
    NJS_VECTOR zero;
    NJS_POINT3 puff;
    Float length;

    if (arm == 0) {
        object = &erobo_arm0_object;
    } else {
        object = &erobo_arm1_object;
    }
    hand_floor[arm] = shadow_draw_under(pos, 1.0F) + HAND_LIFT;
    njSetTexture(erobo_textures);
    hand_show(arm);
    njPushMatrix(NULL);
    njTranslateV(NULL, pos);
    TURN(njRotateY, ang->y);
    TURN(njRotateZ, ang->z);
    njRotateX(NULL, 0x4000);
    draw_tree(object);
    njPopMatrix(1);
    hand_hide(arm);
    if (arms_mode == ARMS_THROWN) {
        /* @hack redundant: zero, which nothing reads (stores from
         * 0x0c907680); without it, 133 of 216 words differ */
        zero.x = zero.y = zero.z = 0.0F;
        VECTOR_COPY(&step, velocity);
        length = njUnitVector(&step);
        VECTOR_SCALE(&step, &step, -DUST_GAP);
        VECTOR_COPY(&puff, pos);
        /* @hack spelling: a do-while inside an if in place of a while, which
         * loads dust_create's address into R14 (0x0c907706) after the test
         * at 0x0c907702; as a while, SHC hoists the load above the test and 5
         * of 216 words differ */
        if (length > 0.0F) {
            do {
                dust_create(&puff, &step, 1.0F);
                VECTOR_SUM(&puff, &puff, &step);
                length -= DUST_GAP;
            } while (length > 0.0F);
        }
    }
}

void erobo_arms_draw(void)
{
    if (!frame_step) {
        switch (arms_mode) {
        case ARMS_THROWN:
        case ARMS_RECALLED:
            hand_draw(0);
            links_draw(0);
            break;
        case ARMS_UNFURL:
        case ARMS_SPREAD:
        case ARMS_HOLD:
        case ARMS_HOMING:
            if (!hands_home[0]) {
                hand_draw(0);
                links_draw(0);
            }
            if (!hands_home[1]) {
                hand_draw(1);
                links_draw(1);
            }
            break;
        }
    }
}

/* start hand 0 or 1's spread. the hand and its chain gather at its arm's
 * end. every node but the arm's end turns toward the hand's spread_target
 * at offset */
static void arm_spread_start(int arm, Angle offset)
{
    NJS_POINT3 *pos = &hand_pos[arm];
    NJS_VECTOR *velocity = &hand_velocity[arm];
    struct angles *ang = &hand_ang[arm];
    Float sine;
    Float cosine;
    NJS_POINT3 target;
    Angle heading;

    erobo_get_arm_end(arm, pos);
    heading = erobo_get_heading() + offset;
    _builtin_fsca(heading, &sine, &cosine);
    target.x = pos->x + (cosine * REACH);
    target.z = pos->z + (sine * REACH);
    target.y = pos->y;
    ang->y = -(Angle)(atan2f(velocity->z, velocity->x) * ANGLE_PER_RADIAN);
    ang->z = 0;
    chain_reset(arm, pos);
    chain_aim(arm, &target);
    hand_floor[arm] = NO_FLOOR;
    hands_home[arm] = FALSE;
}

static void arms_spread_start(void)
{
    arm_spread_start(0, -SPREAD_TURN);
    arm_spread_start(1, SPREAD_TURN);
    arms_timer = UNFURL_STEPS;
    link_length = 0.0F;
}

/* set out to the point REACH from pos across the deck, at offset from
 * Zero's heading */
static void spread_target(NJS_POINT3 *pos, Angle offset, NJS_POINT3 *out)
{
    Float sine;
    Float cosine;
    Angle heading;

    heading = erobo_get_heading() + offset;
    _builtin_fsca(heading, &sine, &cosine);
    out->x = pos->x + (cosine * REACH);
    out->z = pos->z + (sine * REACH);
    out->y = pos->y;
}

/* lay hand 0 or 1's chain from its arm's end toward its spread_target at
 * offset. the hand goes to the chain's end, facing along it */
static void arm_lay(int arm, Angle offset)
{
    NJS_POINT3 *pos = &hand_pos[arm];
    struct angles *ang = &hand_ang[arm];
    NJS_POINT3 root;
    NJS_POINT3 target;

    erobo_get_arm_end(arm, &root);
    spread_target(&root, offset, &target);
    chain_lay(arm, &root, &target, pos);
    ang->y = -chains[arm][0].ang.y;
}

/* grow the links a step and lay both chains. returns TRUE once the count
 * from UNFURL_STEPS runs out */
static Bool arms_unfurl(void)
{
    link_length += LINK_MOST / UNFURL_STEPS;
    link_length = CLAMP(link_length, 0.0F, LINK_MOST);
    arm_lay(0, -SPREAD_TURN);
    arm_lay(1, SPREAD_TURN);
    if (--arms_timer <= 0) {
        return TRUE;
    }
    /* @bug uninitialized: before the count runs out, the function returns
     * the R0 arm_lay left, 0x0c930e58 (loaded at 0x0c9079fa).
     * erobo_arms_step's test at 0x0c9073de takes it as true, so ARMS_UNFURL
     * ends after one step */
    /* NOLINTNEXTLINE(clang-diagnostic-return-type) */
}

/* lay both chains, and grow the links a step toward LINK_MOST */
static void arms_spread(void)
{
    arm_lay(0, -SPREAD_TURN);
    arm_lay(1, SPREAD_TURN);
    link_length += LINK_MOST / UNFURL_STEPS;
    link_length = CLAMP(link_length, 0.0F, LINK_MOST);
}

/* fly each hand that isn't home a step toward its arm's end. a hand that
 * gets home shows on Zero again. returns TRUE once both hands are home */
static Bool hands_return(void)
{
    Bool *home = &hands_home[0];

    if (*home != TRUE) {
        *home = hand_return(0);
        if (*home == TRUE) {
            hand_show(0);
        }
    }
    home = &hands_home[1];
    if (*home != TRUE) {
        *home = hand_return(1);
        if (*home == TRUE) {
            hand_show(1);
        }
    }
    /* @hack spelling: the test through home, set back to hands_home, which
     * copies the array's address from R13 to R4 for it (0x0c907b4e); through
     * hands_home, 4 of 41 words differ */
    home = hands_home;
    return home[0] == TRUE && home[1] == TRUE;
}

/* send hand 0 or 1 from its arm's end toward the marker's lock, at
 * HAND_SPEED. a lock too near the arm's end to aim at loses the marker
 * instead */
static void hand_throw(int arm)
{
    NJS_POINT3 *pos = &hand_pos[arm];
    NJS_VECTOR *velocity = &hand_velocity[arm];
    struct angles *ang = &hand_ang[arm];
    NJS_POINT3 lock;

    erobo_get_arm_end(arm, pos);
    erobo_marker_get_lock(&lock);
    VECTOR_DIFF(velocity, &lock, pos);
    if (vector_unit(velocity, velocity) < NO_DIRECTION) {
        VECTOR_SCALE(velocity, velocity, HAND_SPEED);
    } else {
        /* the idle mode set here never holds: erobo_arms_step sets
         * ARMS_THROWN once this returns. the hand flies all the same */
        erobo_marker_lose();
        arms_mode = EROBO_ARMS_IDLE;
        return;
    }
    ang->y = -(Angle)(atan2f(velocity->z, velocity->x) * ANGLE_PER_RADIAN);
    ang->z = 0;
    chain_reset(arm, pos);
    hand_floor[arm] = NO_FLOOR;
}

/* fly hand 0 or 1 a step toward the marker's lock, above the deck and its
 * hand_floor. returns TRUE once it is within LOCK_NEAR of the lock, or past
 * THROW_REACH */
static Bool hand_fly(int arm)
{
    NJS_POINT3 *pos = &hand_pos[arm];
    NJS_VECTOR *velocity = &hand_velocity[arm];
    struct angles *ang = &hand_ang[arm];
    NJS_POINT3 root;
    NJS_POINT3 lock;
    NJS_VECTOR previous;
    NJS_VECTOR toward;

    erobo_get_arm_end(arm, &root);
    erobo_marker_get_lock(&lock);
    VECTOR_COPY(&previous, velocity);
    if (vector_unit(&previous, &previous) > NO_DIRECTION) {
        previous.x = 1.0F;
        previous.y = 0.0F;
        previous.z = 0.0F;
    }
    VECTOR_DIFF(&toward, &lock, pos);
    if (vector_unit(&toward, &toward) > NO_DIRECTION) {
        VECTOR_COPY(&toward, &previous);
    }
    velocity->x = (previous.x * (1.0F - HAND_STEER)) + (toward.x * HAND_STEER);
    velocity->y = (previous.y * (1.0F - HAND_STEER)) + (toward.y * HAND_STEER);
    velocity->z = (previous.z * (1.0F - HAND_STEER)) + (toward.z * HAND_STEER);
    if (vector_unit(velocity, velocity) > NO_DIRECTION) {
        VECTOR_COPY(velocity, &toward);
    }
    VECTOR_SCALE(velocity, velocity, HAND_SPEED);
    VECTOR_SUM(pos, pos, velocity);
    if (pos->y <= DECK_FLOOR_Y) {
        pos->y = DECK_FLOOR_Y;
    }
    if (pos->y <= hand_floor[arm]) {
        pos->y = hand_floor[arm];
    }
    if (point_distance_squared(&root, pos) > THROW_REACH * THROW_REACH ||
        point_distance_squared_across(pos, &lock) < LOCK_NEAR * LOCK_NEAR) {
        return TRUE;
    }
    ang->y = -(Angle)(atan2f(velocity->z, velocity->x) * ANGLE_PER_RADIAN);
    ang->z = 0;
    chain_step(arm, &root, pos);
    return FALSE;
}

/* the heading from from to to, across the deck */
static Angle angle_to(NJS_POINT3 *to, NJS_POINT3 *from)
{
    Float dx = to->x - from->x;
    Float dz = to->z - from->z;

    return angle_arc_tan2(dz, dx);
}

/* fly hand 0 or 1 a step toward its arm's end, above the deck and its
 * hand_floor. within HOME_NEAR, it flies at HAND_SLOW. returns TRUE, and
 * stops the hand, once a step starts within HOME_REACHED of the arm's
 * end */
static Bool hand_return(int arm)
{
    NJS_POINT3 *pos = &hand_pos[arm];
    NJS_VECTOR *velocity = &hand_velocity[arm];
    struct angles *ang = &hand_ang[arm];
    NJS_POINT3 root;
    NJS_VECTOR previous;
    NJS_VECTOR toward;
    Float distance2;

    erobo_get_arm_end(arm, &root);
    distance2 = point_distance_squared(&root, pos);
    VECTOR_COPY(&previous, velocity);
    if (vector_unit(&previous, &previous) > NO_DIRECTION) {
        previous.x = 1.0F;
        previous.y = 0.0F;
        previous.z = 0.0F;
    }
    VECTOR_DIFF(&toward, &root, pos);
    if (vector_unit(&toward, &toward) > NO_DIRECTION) {
        VECTOR_COPY(&toward, &previous);
    }
    /* a hand flying exactly away from home would keep its direction through
     * the blend below. REVERSE_LIFT starts it turning, unless it flies
     * straight up or down */
    if (toward.x == -previous.x && toward.y == -previous.y &&
        toward.z == -previous.z) {
        previous.y += REVERSE_LIFT;
    }
    velocity->x = (previous.x * (1.0F - HAND_STEER)) + (toward.x * HAND_STEER);
    velocity->y = (previous.y * (1.0F - HAND_STEER)) + (toward.y * HAND_STEER);
    velocity->z = (previous.z * (1.0F - HAND_STEER)) + (toward.z * HAND_STEER);
    if (vector_unit(velocity, velocity) > NO_DIRECTION) {
        VECTOR_COPY(velocity, &toward);
    }
    if (distance2 > HOME_NEAR * HOME_NEAR) {
        VECTOR_SCALE(velocity, velocity, HAND_SPEED);
    } else {
        VECTOR_SCALE(velocity, velocity, HAND_SLOW);
    }
    VECTOR_SUM(pos, pos, velocity);
    if (pos->y <= DECK_FLOOR_Y) {
        pos->y = DECK_FLOOR_Y;
    }
    if (pos->y <= hand_floor[arm]) {
        pos->y = hand_floor[arm];
    }
    if (distance2 < HOME_REACHED * HOME_REACHED) {
        velocity->x = velocity->y = velocity->z = 0.0F;
        return TRUE;
    }
    chain_step(arm, &root, pos);
    ang->y = -angle_to(pos, &root);
    return FALSE;
}

/* put every node of hand 0 or 1's chain at pos, at rest */
static void chain_reset(int arm, NJS_POINT3 *pos)
{
    struct chain_node *node = chains[arm];
    int count = NODES;
    /* @hack spelling: 0.0F through a local, set ahead of the goto, which puts
     * the FLDI0 in the slot of the BRA at 0x0c908190; written in place, 7 of
     * 36 words differ */
    Float zero = 0.0F;

    /* @hack spelling: a goto into the do-while's test, which is the BRA at
     * 0x0c908190; as a while, SHC counts down with DT and no jump, and 30 of
     * 36 words differ */
    goto check;
    do {
        VECTOR_COPY(&node->pos, pos);
        node->velocity.x = node->velocity.y = node->velocity.z = zero;
        node->force.x = node->force.y = node->force.z = zero;
        node++;
        count--;
    check:;
    } while (count);
}

/* add to node's force its spring's pull toward neighbor, spring of the
 * distance between them */
static void node_pull(struct chain_node *node, struct chain_node *neighbor,
                      Float spring)
{
    NJS_VECTOR delta;

    VECTOR_DIFF(&delta, &neighbor->pos, &node->pos);
    VECTOR_SCALE(&delta, &delta, spring);
    VECTOR_SUM(&node->force, &node->force, &delta);
}

/* add to node's velocity its force, less drag of the velocity. then move
 * the node by its velocity, and clear its force */
static void node_move(struct chain_node *node, Float drag)
{
    NJS_VECTOR resistance;

    VECTOR_SCALE(&resistance, &node->velocity, drag);
    VECTOR_DIFF(&node->force, &node->force, &resistance);
    VECTOR_SUM(&node->velocity, &node->velocity, &node->force);
    VECTOR_SUM(&node->pos, &node->pos, &node->velocity);
    node->force.x = node->force.y = node->force.z = 0.0F;
}

/* pull each node of hand 0 or 1's chain toward its neighbors. move every
 * node but the arm's end, then pin the chain's ends at root and tip */
static void chain_step(int arm, NJS_POINT3 *root, NJS_POINT3 *tip)
{
    /* @hack spelling: the bound through a local, which SHC keeps in R13 (set
     * at 0x0c90830e); as LINKS in each test, 70 of 87 words differ */
    int count = LINKS;
    struct chain_node *node = chains[arm];
    int i;

    for (i = 0; i < count; i++) {
        node++;
        node_pull(node, node - 1, SPRING);
    }
    node = chains[arm];
    for (i = 0; i < count; i++) {
        node_pull(node, node + 1, SPRING);
        node++;
    }
    node = chains[arm];
    for (i = 0; i < count; i++) {
        node_move(node, DRAG);
        node++;
    }
    VECTOR_COPY(&chains[arm][LINKS].pos, root);
    VECTOR_COPY(&chains[arm][0].pos, tip);
}

/* turn the node's heading toward target */
static void node_aim(struct chain_node *node, NJS_POINT3 *target)
{
    NJS_VECTOR delta;

    VECTOR_DIFF(&delta, target, &node->pos);
    /* the image works out delta's length, and its length across the deck,
     * then uses neither (0x0c9083da, 0x0c9083e0) */
    vector_magnitude(&delta);
    vector_magnitude_across(&delta);
    node->ang.y = (Angle)(atan2f(delta.z, delta.x) * ANGLE_PER_RADIAN);
}

/* give each node of hand 0 or 1's chain the heading from node 0 to
 * target. the arm's end keeps its own */
static void chain_aim(int arm, NJS_POINT3 *target)
{
    struct chain_node *node = chains[arm];
    NJS_VECTOR delta;
    Angle heading;
    int i;
    /* @hack spelling: the bound through a local, which SHC keeps in R13 (set
     * at 0x0c908460); as LINKS in the test, 48 of 51 words differ */
    int count = LINKS;

    VECTOR_DIFF(&delta, target, &node->pos);
    heading = (Angle)(atan2f(delta.z, delta.x) * ANGLE_PER_RADIAN);
    for (i = 0; i < count; i++, node++) {
        node->ang.y = heading;
    }
}

/* lay hand 0 or 1's chain from root, its arm's end, which aims at target.
 * each node lies a link on from the one before, along that one's heading.
 * it turns LAY_TURN of the way to that heading. each lies lower than the
 * one before too, by LAY_SAG at LINK_MOST and in proportion below it. the
 * spheres follow the nodes, and tip takes node 0's place */
static void chain_lay(int arm, NJS_POINT3 *root, NJS_POINT3 *target,
                      NJS_POINT3 *tip)
{
    /* @hack spelling: the bound through a local, which SHC keeps in R9 (set
     * at 0x0c90851a); as LINKS in the test, 108 of 130 words differ */
    int count = LINKS;
    struct chain_node *node = &chains[arm][LINKS];
    struct chain_node *next;
    Float sine;
    Float cosine;
    Float height;
    Float drop;
    int i;
    Float length;

    VECTOR_COPY(&node->pos, root);
    node_aim(node, target);
    /* @hack spelling: drop's two products in statements of their own, with
     * height set between them, which keeps the image's two multiplies
     * (0x0c90853e, 0x0c908546); as one expression after height, 92 of 130
     * words differ */
    drop = link_length * (1.0F / LINK_MOST);
    height = root->y;
    drop = -(drop * LAY_SAG);
    for (i = 0; i < count; i++) {
        height += drop;
        next = node - 1;
        next->ang.y += angle_difference(next->ang.y, node->ang.y) * LAY_TURN;
        _builtin_fsca(node->ang.y, &sine, &cosine);
        /* @hack spelling: link_length through a local, which reads it once a
         * link (0x0c908580); read in each product, it is read again after
         * the store to next->pos.x, and 15 of 130 words differ */
        length = link_length;
        next->pos.x = node->pos.x + (cosine * length);
        next->pos.z = node->pos.z + (sine * length);
        next->pos.y = height;
        chain_shape_set(arm, i, node);
        node = next;
    }
    chain_shape_set(arm, i, node);
    VECTOR_COPY(tip, &chains[arm][0].pos);
}

/* draw the link from one node to the next. its object turns along them,
 * and stretches along x to span them */
static void link_draw(struct chain_node *from, struct chain_node *to)
{
    NJS_VECTOR delta;
    Float length;
    Float across;
    Float ratio;
    Angle yaw;
    Angle roll;

    VECTOR_DIFF(&delta, &to->pos, &from->pos);
    length = vector_magnitude(&delta);
    across = vector_magnitude_across(&delta);
    yaw = (Angle)(atan2f(delta.z, -delta.x) * ANGLE_PER_RADIAN);
    ratio = across / length;
    if (delta.y < 0.0F) {
        if (across < 0.01F) {
            roll = 0x4000;
        } else {
            roll = (Angle)(acosf(ratio) * ANGLE_PER_RADIAN);
        }
    } else {
        if (across < 0.01F) {
            roll = 0xc000;
        } else {
            roll = -(Angle)(acosf(ratio) * ANGLE_PER_RADIAN);
        }
    }
    length *= LINK_SCALE;
    njPushMatrix(NULL);
    njTranslateV(NULL, &from->pos);
    TURN(njRotateY, yaw);
    TURN(njRotateZ, roll);
    njScale(NULL, length, 1.0F, 1.0F);
    draw_tree((NJS_OBJECT *)(erobo_models + LINK_OBJECT));
    njPopMatrix(1);
}

static void links_draw(int arm)
{
    struct chain_node *node;
    int i;
    /* @hack spelling: the bound through a local, which SHC keeps in R12 (set
     * at 0x0c908766); as LINKS in the test, 12 of 27 words differ */
    int count = LINKS;

    for (node = chains[arm], i = 0; i < count; i++, node++) {
        link_draw(node, node + 1);
    }
}

/* move every sphere to the origin */
static void chain_shapes_clear(void)
{
    /* @hack type: i unsigned, which the image compares by CMP/HS
     * (0x0c9087f8); as an int, 1 of 43 words differs */
    unsigned int i;

    if (shapes_task) {
        /* @hack spelling: a do-while in place of a for, which clears a sphere a
         * pass; as a for, SHC unrolls it by two and 35 of 43 words differ */
        i = 0;
        do {
            VECTOR_COPY(&shapes_task->work->collision->shapes[i].center,
                        &origin);
            i++;
        } while (i < 2 * NODES);
    }
}

/* center the sphere of hand 0 or 1's node SPHERE_DROP below it, if the
 * spheres' task is there. index counts the nodes from the arm's end */
static void chain_shape_set(int arm, int index, struct chain_node *node)
{
    NJS_POINT3 *center;

    if (!shapes_task) {
        return;
    }
    center = &shapes_task->work->collision->shapes[arm + (index * 2)].center;
    VECTOR_COPY(center, &node->pos);
    center->y -= SPHERE_DROP;
}

static void chain_shapes_create(void)
{
    struct task *task;

    if ((task = task_create(TASK_WORK, 4, chain_shapes_step)) != NULL) {
        struct task_work *work = task->work;

        work->pos.x = DECK_MIDDLE_X;
        work->pos.y = DECK_FLOOR_Y;
        work->pos.z = DECK_MIDDLE_Z;
        collision_init(task, chain_shapes, COLLISION_OBJECT, 1, 2 * NODES);
        /* @hack spelling: the record through task->work, which the image loads
         * again after collision_init (0x0c9088b4); through work, SHC keeps it
         * in R13 and 37 of 44 words differ */
        task->work->collision->radius = 1000.0F;
        task->destructor = chain_shapes_destroy;
    }
    shapes_task = task;
}

static void chain_shapes_destroy(struct task *task)
{
    shapes_task = NULL;
    shapes_active = FALSE;
}

static void chain_shapes_step(struct task *task)
{
    if (frame_step) {
        return;
    }
    if (shapes_active) {
        collision_register(task->work);
    }
}

/* on a frame's first step, draw a chain of light from each lit spark to
 * the next. draw each lit spark too, but the last */
static void sparks_draw(struct task *task)
{
    struct task_work *work;
    struct chain_spark *spark;
    int arm;
    int i;

    if (frame_step) {
        return;
    }
    if (sparks_lit > 0) {
        work = task->work;
        for (arm = 0; arm < 2; arm++) {
            spark = sparks[arm];
            for (i = 0; i < sparks_lit - 1; i++, spark++) {
                erobo_chain_draw(&spark->pos, &spark[1].pos, TASK_TIMER(work),
                                 1, 1.0F);
                sprite_draw_at(&spark->sprite, &spark->pos);
            }
        }
    }
}

/* light one spark more every LIGHT_GROW_STEPS steps while sparks_growing,
 * or one fewer every LIGHT_FADE_STEPS steps. a chain's end sparks sit on
 * its end nodes. the rest sit up to SPARK_JITTER off theirs along each
 * axis, at random. each spark plays once the light reaches the one before
 * it */
static void sparks_step(struct task *task)
{
    struct task_work *work = task->work;
    struct chain_node *node;
    struct chain_spark *spark;
    NJS_VECTOR jitter;
    int arm;
    int i;

    if (sparks_growing) {
        if (++TASK_TIMER(work) % LIGHT_GROW_STEPS == 0) {
            if (++sparks_lit >= NODES) {
                sparks_lit = NODES;
            }
        }
    } else {
        if (++TASK_TIMER(work) % LIGHT_FADE_STEPS == 0) {
            if (--sparks_lit <= 0) {
                sparks_lit = 0;
            }
        }
    }
    if (sparks_lit > 0) {
        for (arm = 0; arm < 2; arm++) {
            spark = sparks[arm];
            node = chains[arm];
            VECTOR_COPY(&spark->pos, &node->pos);
            /* @hack spelling: a do-while in place of a for, which places a
             * spark a pass; as a for, SHC unrolls it and 160 of 171 words
             * differ */
            i = 2;
            do {
                spark++;
                node++;
                jitter.x = RANDOM(-SPARK_JITTER, SPARK_JITTER);
                jitter.y = RANDOM(-SPARK_JITTER, SPARK_JITTER);
                jitter.z = RANDOM(-SPARK_JITTER, SPARK_JITTER);
                VECTOR_SUM(&spark->pos, &node->pos, &jitter);
                if (sparks_lit + 1 >= i) {
                    sprite_step(&spark->sprite);
                }
                i++;
            } while (i < NODES);
            spark++;
            node++;
            VECTOR_COPY(&spark->pos, &node->pos);
        }
        sparks_draw(task);
    }
}

static void sparks_create(void)
{
    struct task *task;
    struct chain_spark *spark;
    int arm;
    int i;

    if ((task = task_create(TASK_WORK, 4, sparks_step)) != NULL) {
        task->draw = sparks_draw;
        task->destructor = sparks_destroy;
    }
    for (arm = 0; arm < 2; arm++) {
        spark = sparks[arm];
        for (i = 0; i < NODES; i++, spark++) {
            erobo_spark_set(&spark->sprite, &spark->pos, SPARK_SCALE, 0);
        }
    }
    sparks_lit = 0;
    sparks_growing = FALSE;
    sparks_task = task;
}

static void sparks_destroy(struct task *task)
{
    sparks_lit = 0;
    sparks_task = NULL;
}
