/* Big as a player: his modes, his fishing and his carrying */
#ifndef PLAYER_BIG_H
#define PLAYER_BIG_H
#include <sg_xpt.h>

struct task;

/* @hack type: Big's fields in record->own, read through own.l, own.p and
 * own.f, so that SHC adds their offset to the record, as the image does
 * (ADD #124,R0 at 0x8c0e9eba). as the fields of a struct of his own in the
 * union, 195 of big_step's 1388 words differ, with 325 of
 * big_choose_mode's 4304 and 15 of fishing_lure_home's 47 */
#define BIG_FLAGS(record) ((record)->own.l[0])
/* the target task he aims the cast at, which the lure kills as he casts */
#define BIG_TARGET(record) (*(struct task **)&(record)->own.p[1])
#define BIG_LURE(record) ((record)->own.p[2])
/* how far his life belt is blown up: 0 flat, and 1 full */
#define BIG_BELT(record) ((record)->own.f[7])

/* the bits of BIG_FLAGS. big_step serves the first two as requests, and
 * clears each it serves */
#define BIG_START_FISHING 0x1 /* create the target he aims */
#define BIG_START_CAST 0x2 /* cast the lure at the target */
/* the lure is back at the rod, or his eyes went under water */
#define BIG_FISHING_OVER 0x4
/* the action button was let go before the cast */
#define BIG_CAST_RELEASED 0x8

/* Big's record->anim.action: a row of big.c's big_actions. the enum names
 * the rows his code sets or tests, and those a row names as its next */
enum big_action {
    ACTION_STAND = 0,
    ACTION_FIDGET_START = 1,
    ACTION_FIDGET = 2,
    ACTION_FIDGET_END = 3,
    ACTION_STAND_UPHILL = 4,
    ACTION_STAND_DOWNHILL = 5,
    ACTION_LAND = 6, /* landing at a stop */
    ACTION_WALK = 7,
    ACTION_TROT = 8,
    ACTION_RUN = 9,
    ACTION_SPRINT = 10,
    ACTION_FULL_SPEED = 11,
    ACTION_SPRING = 12, /* sent up by a spring, a launch or a ride's end */
    ACTION_FALL_START = 13,
    ACTION_FALL = 14,
    ACTION_JUMP = 15,
    ACTION_STOP = 16,
    ACTION_PUSH_START = 17,
    ACTION_PUSH = 18,
    ACTION_HURT = 19,
    ACTION_HURT_LAND = 20,
    ACTION_UPDRAFT = 21,
    ACTION_CRASH = 22,
    ACTION_CRASH_END = 23,
    ACTION_TURN = 24, /* from a stop, back the way the stick points */
    ACTION_SLIDE = 25,
    ACTION_LIFT = 26,
    ACTION_HOLD_RUN = 27,
    ACTION_HOLD_STOP = 28,
    ACTION_HOLD_TURN = 29,
    ACTION_HOLD_STAND = 30,
    ACTION_HOLD_WALK = 31,
    ACTION_HANG_ON = 32,
    ACTION_PULL = 33,
    ACTION_SHAKE = 34,
    ACTION_GRIP_START = 35,
    ACTION_GRIP = 36,
    ACTION_HOLD_JUMP = 37,
    ACTION_HOLD_FALL_START = 38,
    ACTION_HOLD_FALL = 39,
    ACTION_HOLD_LAND = 40,
    ACTION_THROW_AT = 41, /* at the nearest task he can aim at */
    ACTION_THROW = 42, /* straight ahead, with no task to aim at */
    ACTION_AIR_THROW_AT = 43,
    ACTION_AIR_THROW = 44,
    ACTION_PUT_DOWN = 45,
    ACTION_LEDGE = 46,
    ACTION_FLUNG = 47,
    ACTION_CLEAR = 48,
    ACTION_SWING = 49, /* his rod, swung as an attack */
    /* ACTION_REEL plays the reel's motion once after the cast, and holds its
     * last frame. after each turn of the reel, ACTION_REEL_HELD shows that
     * frame at once, as big.c's BIG_REEL_TURN makes it anim.start_frame */
    ACTION_REEL = 50,
    ACTION_REEL_HELD = 51,
    ACTION_CAST_START = 52, /* held while he aims */
    ACTION_CAST = 53,
    ACTION_CAST_SWING = 54, /* ACTION_CAST's motion, as an attack */
    /* a turn of the reel, by where the stick points on the screen */
    ACTION_REEL_DOWN = 55,
    ACTION_REEL_DOWN_RIGHT = 56,
    ACTION_REEL_DOWN_LEFT = 57,
    ACTION_REEL_RIGHT = 58,
    ACTION_REEL_LEFT = 59,
    /* showing the catch, by the fish's scl.y: at most 0, at most 1, or more */
    ACTION_CATCH_SMALL = 60,
    ACTION_CATCH_MEDIUM = 61,
    ACTION_CATCH_LARGE = 62,
    ACTION_SNAP = 63,
    ACTION_SNAPPED = 64,
    /* the cast and the reel as he floats in his life belt. ACTION_FLOAT_CAST
     * holds its last frame for the reel, which ACTION_FLOAT_REEL_HELD shows
     * after a turn */
    ACTION_FLOAT_CAST_START = 66,
    ACTION_FLOAT_CAST = 67,
    ACTION_FLOAT_REEL_HELD = 68,
    ACTION_FLOAT_REEL_DOWN = 69,
    ACTION_FLOAT_REEL_DOWN_RIGHT = 70,
    ACTION_FLOAT_REEL_DOWN_LEFT = 71,
    ACTION_FLOAT_REEL_RIGHT = 72,
    ACTION_FLOAT_REEL_LEFT = 73,
    /* holding, lifting and putting down a CARRY_KIND_SMALL task. a task of
     * any other kind takes ACTION_HOLD_STAND, ACTION_HOLD_WALK,
     * ACTION_HOLD_RUN, ACTION_LIFT and ACTION_PUT_DOWN */
    ACTION_HOLD_SMALL = 74,
    ACTION_HOLD_SMALL_WALK = 75,
    ACTION_HOLD_SMALL_RUN = 76,
    ACTION_LIFT_SMALL = 78,
    ACTION_PUT_DOWN_SMALL = 80,
    ACTION_SWIM_START = 86,
    ACTION_SWIM = 87,
    ACTION_DIVE_START = 88,
    ACTION_DIVE = 89,
    /* floating with his eyes under the water. in big.c's BIG_FLOAT, big_step
     * puts him in ACTION_FLOAT whenever the water is below his eyes */
    ACTION_FLOAT_UNDER_START = 90,
    ACTION_FLOAT_UNDER = 91,
    ACTION_FLOAT = 92,
    ACTION_HOLD_SHAKE_START = 93,
    ACTION_HOLD_SHAKE = 94,
    ACTION_HOLD_SHAKE_END = 95,
    ACTION_RIDE = 96,
    ACTION_BREATHE = 97,
    ACTION_DROWN = 98,
    ACTION_DOWN = 99,
    ACTION_DOWN_LAND = 100
};

/* player 0's lure task, or null while he has none */
extern struct task *big_lure;
/* the fish, or Hot Shelter's plug, that holds or claims the lure, or null.
 * it is on the hook only while fishing_flags has FISHING_HOOKED */
extern struct task *big_catch;
/* how many fish swim in the stage, five at most. Big zeroes it as he
 * starts */
extern int fishing_fish_count;
/* whether Hot Shelter's waiting fish may swim, which its dam sets. Big
 * clears it as he starts */
extern Bool fishing_shelter_ready;

/* the step of Big's player task */
void big_step(struct task *task);
/* FALSE while player 0 is in one of Big's fishing modes, from the cast to
 * the catch, and TRUE otherwise. player 0 must be Big. task is unused */
Bool big_not_fishing(struct task *task);

#endif
