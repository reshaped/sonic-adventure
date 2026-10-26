/* the race's splits and lap times, the save's best, and the start boost */
#include <sg_xpt.h>
#include <shinobi.h>

#include "hud/hud.h"
#include "minicart/cart_time.h"
#include "minicart/e_cart.h"
#include "minicart/race.h"
#include "minicart/times.h"
#include "save/minigame.h"
#include "save/save.h"
#include "system/pad.h"
#include "system/task.h"

#define CART_LAPS 3

#define BOOST_STEPS 60

static void boost_hold(struct task *task);
static void boost_watch(struct task *task);

/* in hundredths, the run's time as each lap ended. the last lap's split is
 * the run's time */
static int cart_splits[CART_LAPS] = {0, 0, 0};
/* in hundredths, each lap's own time */
static int cart_laps[CART_LAPS] = {0, 0, 0};

/* the play time of the last press of A. once the race goes, it holds the
 * steps from that press to the go */
static int boost_press;
/* the boost that press earned, for cart_set_boost */
static Float boost;

/* give the cart the boost each step until the timer passes BOOST_STEPS, then
 * destroy the task */
static void boost_hold(struct task *task)
{
    cart_set_boost(boost);
    if (TASK_TIMER(task->work)++ > BOOST_STEPS) {
        task_kill(task);
    }
}

/* note each press of A by player 0. once the race goes, the last press
 * earns a boost if it came at most 2 steps before. the later the press, the
 * bigger the boost and the cart's afterimages. then the task holds the
 * boost */
static void boost_watch(struct task *task)
{
    if (per_info[0]->press & PDD_DGT_TA) {
        boost_press = play_time_get();
    }
    if (cart_race_mode() == RACE_GO) {
        boost_press = play_time_get() - boost_press;
        switch (boost_press) {
        case 0:
            boost = 0.1F;
            cart_afterimage_start(0.3F);
            break;
        case 1:
            boost = 0.06F;
            cart_afterimage_start(0.2F);
            break;
        case 2:
            boost = 0.03F;
            cart_afterimage_start(0.1F);
            break;
        default:
            boost = 0.0F;
            break;
        }
        task->step = boost_hold;
    }
}

void boost_start(void)
{
    task_create(TASK_WORK, 3, boost_watch);
    boost_press = 0;
}

/* in the module's head, at the addresses 1ST_READ.BIN's hud and save
 * call */
#pragma section HEAD
struct cart_time cart_get_time(void)
{
    return cart_get_split(CART_LAPS - 1);
}

struct cart_time cart_get_split(Sint8 lap)
{
    struct cart_time time;

    time = cart_time_of(cart_splits[lap]);
    return time;
}

struct cart_time cart_get_lap(Sint8 lap)
{
    struct cart_time time;

    time = cart_time_of(cart_laps[lap]);
    return time;
}
#pragma section

int cart_best_time(void)
{
    return cart_best_split(CART_LAPS - 1);
}

/* the save keeps the best time, the best run's last split, in time[0]. that
 * run's first two splits follow the save's best times. each field is clamped
 * to its range */
int cart_best_split(Sint8 lap)
{
    struct save_cart_times times;
    struct save_time *time;
    int minutes;
    int seconds;
    int hundredths;
    int value;

    cart_copy_times(&times);
    if (lap == CART_LAPS - 1) {
        time = &times.time[0];
    } else {
        /* @hack type: the splits reached as an array of their own at
         * &times.time[SAVE_BEST], which adds lap's offset to that address
         * (ADD #13,R3 at 0x0c90a788); as &times.time[SAVE_BEST + lap], SHC
         * adds the 9 bytes after, cart_best_split is 2 bytes longer and 43
         * of 62 words differ */
        time = &(*(struct save_time(*)[SAVE_CART_SPLITS]) &
                 times.time[SAVE_BEST])[lap];
    }
    minutes = (int)time->minutes;
    if (minutes < 0) {
        minutes = 0;
    } else if (minutes >= 100) {
        minutes = 99;
    }
    seconds = (int)time->seconds;
    if (seconds < 0) {
        seconds = 0;
    } else if (seconds >= 60) {
        seconds = 59;
    }
    hundredths = (int)time->frames;
    if (hundredths < 0) {
        hundredths = 0;
    } else if (hundredths >= 100) {
        hundredths = 99;
    }
    value = (((minutes * 60) + seconds) * 100) + hundredths;
    return value;
}

void cart_set_split(Sint8 lap, int time)
{
    cart_splits[lap] = time;
}

void cart_set_lap(Sint8 lap, int time)
{
    cart_laps[lap] = time;
}
