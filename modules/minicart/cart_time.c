/* the race's time in hundredths of a second, and in minutes, seconds and
 * hundredths */
#include "minicart/cart_time.h"
#include "save/minigame.h"

#define STEPS_PER_SECOND 60

int cart_hundredths_of(int steps)
{
    int thousandths = steps * 1000 / STEPS_PER_SECOND;
    int hundredths = thousandths / 10;

    if (thousandths % 10 >= 5) {
        hundredths++;
    }
    return hundredths;
}

struct cart_time cart_time_of(int hundredths)
{
    struct cart_time time;
    int minutes;

    if (hundredths > 0) {
        time.sign = 1;
    } else if (hundredths == 0) {
        time.sign = 0;
    } else {
        time.sign = -1;
        hundredths = -hundredths;
    }
    minutes = hundredths / 6000;
    if (minutes >= 100) {
        time.minutes = 99;
        time.seconds = 59;
        time.hundredths = 99;
    } else {
        time.minutes = minutes;
        time.seconds = hundredths / 100 % 60;
        time.hundredths = hundredths % 100;
    }
    return time;
}
