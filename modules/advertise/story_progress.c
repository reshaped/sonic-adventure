/* how far each story has been played, in percent, as the character wheel
 * shows it */
#include "advertise/story_progress.h"
#include "event/play.h"
#include "progress/progress.h"

enum mark_kind { MARK_FLAG, MARK_EVENT };

struct progress_mark {
    int percent; /* the story's percent once the mark is reached */
    enum mark_kind kind;
    int number; /* an event flag for MARK_FLAG, an event for MARK_EVENT */
};

struct progress_marks {
    const struct progress_mark *marks;
    int count;
};

static const struct progress_mark sonic_marks[] = {
    {5, MARK_FLAG, FLAG_SONIC_CLEAR_CHAOS0},
    {10, MARK_FLAG, FLAG_SONIC_CLEAR_BEACH},
    {15, MARK_FLAG, FLAG_SONIC_CLEAR_EGGMOBILE1},
    {20, MARK_FLAG, FLAG_SONIC_CLEAR_WINDY},
    {25, MARK_FLAG, FLAG_SONIC_CLEAR_CASINO},
    {30, MARK_FLAG, FLAG_SONIC_CLEAR_ICECAP},
    {35, MARK_FLAG, FLAG_SONIC_MBOSS_KNUCKLES},
    {40, MARK_FLAG, FLAG_SONIC_CLEAR_CHAOS4},
    {45, MARK_FLAG, FLAG_SONIC_CLEAR_SHOOTING1},
    {50, MARK_FLAG, FLAG_SONIC_CLEAR_TWINKLE_PARK},
    {55, MARK_FLAG, FLAG_SONIC_CLEAR_HIGHWAY},
    {60, MARK_FLAG, FLAG_SONIC_CLEAR_MOUNTAIN},
    {65, MARK_FLAG, FLAG_SONIC_CLEAR_SHOOTING2},
    {70, MARK_FLAG, FLAG_SONIC_CLEAR_SKY_DECK},
    {75, MARK_FLAG, FLAG_SONIC_MBOSS_E102},
    {80, MARK_FLAG, FLAG_SONIC_CLEAR_CHAOS6},
    {85, MARK_FLAG, FLAG_SONIC_CLEAR_RUIN},
    {90, MARK_FLAG, FLAG_SONIC_CLEAR_FINAL_EGG},
    {100, MARK_FLAG, FLAG_CLEAR_SONIC},
};
static const struct progress_mark tails_marks[] = {
    {5, MARK_EVENT, 50},
    {10, MARK_FLAG, FLAG_TAILS_CLEAR_EGGMOBILE1},
    {20, MARK_FLAG, FLAG_TAILS_CLEAR_WINDY},
    {30, MARK_FLAG, FLAG_TAILS_CLEAR_CASINO},
    {40, MARK_FLAG, FLAG_TAILS_CLEAR_ICECAP},
    {45, MARK_FLAG, FLAG_TAILS_MBOSS_KNUCKLES},
    {50, MARK_FLAG, FLAG_TAILS_CLEAR_CHAOS4},
    {55, MARK_FLAG, FLAG_TAILS_CLEAR_SHOOTING1},
    {60, MARK_FLAG, FLAG_TAILS_CLEAR_SAND_BOARD},
    {70, MARK_FLAG, FLAG_TAILS_CLEAR_SHOOTING2},
    {80, MARK_FLAG, FLAG_TAILS_CLEAR_SKY_DECK},
    {85, MARK_FLAG, FLAG_TAILS_MBOSS_E102},
    {90, MARK_FLAG, FLAG_TAILS_CLEAR_HIGHWAY},
    {100, MARK_FLAG, FLAG_CLEAR_TAILS},
};
static const struct progress_mark knuckles_marks[] = {
    {10, MARK_EVENT, 130},
    {20, MARK_FLAG, FLAG_KNUCKLES_CLEAR_HIGHWAY},
    {30, MARK_FLAG, FLAG_KNUCKLES_CLEAR_CASINO},
    {40, MARK_FLAG, FLAG_KNUCKLES_CLEAR_CHAOS2},
    {50, MARK_FLAG, FLAG_KNUCKLES_CLEAR_MOUNTAIN},
    {60, MARK_FLAG, FLAG_KNUCKLES_MBOSS_SONIC},
    {70, MARK_FLAG, FLAG_KNUCKLES_CLEAR_CHAOS4},
    {80, MARK_FLAG, FLAG_KNUCKLES_CLEAR_RUIN},
    {90, MARK_FLAG, FLAG_KNUCKLES_CLEAR_SKY_DECK},
    {100, MARK_FLAG, FLAG_CLEAR_KNUCKLES},
};
static const struct progress_mark amy_marks[] = {
    {10, MARK_EVENT, 97},
    {20, MARK_FLAG, FLAG_AMY_CLEAR_TWINKLE_PARK},
    {30, MARK_FLAG, FLAG_AMY_HAMMER_WON},
    {40, MARK_FLAG, FLAG_AMY_CLEAR_HOT_SHELTER},
    {50, MARK_EVENT, 108},
    {60, MARK_EVENT, 109},
    {70, MARK_FLAG, FLAG_AMY_MR_ENTRANCE_FINAL_EGG},
    {80, MARK_FLAG, FLAG_AMY_CLEAR_FINAL_EGG},
    {90, MARK_EVENT, 126},
    {100, MARK_FLAG, FLAG_CLEAR_AMY},
};
static const struct progress_mark big_marks[] = {
    {10, MARK_EVENT, 209},
    {20, MARK_FLAG, FLAG_BIG_SS_TPARK_ELEVATOR},
    {30, MARK_FLAG, FLAG_BIG_CLEAR_TWINKLE_PARK},
    {40, MARK_FLAG, FLAG_BIG_MR_ENTRANCE_ICECAP},
    {50, MARK_FLAG, FLAG_BIG_CLEAR_ICECAP},
    {60, MARK_FLAG, FLAG_BIG_SS_HOTEL_POOL},
    {70, MARK_FLAG, FLAG_BIG_CLEAR_BEACH},
    {80, MARK_FLAG, FLAG_BIG_CLEAR_HOT_SHELTER},
    {90, MARK_EVENT, 336},
    {95, MARK_FLAG, FLAG_BIG_CLEAR_CHAOS6},
    {100, MARK_FLAG, FLAG_CLEAR_BIG},
};
static const struct progress_mark e102_marks[] = {
    {10, MARK_EVENT, 176},
    {20, MARK_FLAG, FLAG_E102_CLEAR_FINAL_EGG},
    {30, MARK_FLAG, FLAG_E102_CLEAR_E101},
    {40, MARK_FLAG, FLAG_E102_CLEAR_BEACH},
    {50, MARK_FLAG, FLAG_E102_MBOSS_SONIC},
    {60, MARK_FLAG, FLAG_E102_MR_ENTRANCE_WINDY},
    {70, MARK_FLAG, FLAG_E102_CLEAR_WINDY},
    {80, MARK_FLAG, FLAG_E102_CLEAR_MOUNTAIN},
    {90, MARK_FLAG, FLAG_E102_CLEAR_HOT_SHELTER},
    {100, MARK_FLAG, FLAG_CLEAR_E102},
};
static const struct progress_mark last_marks[] = {
    {100, MARK_FLAG, FLAG_CLEAR_LAST},
};

/* each story's marks, by enum wheel_story. a story's marks rise in percent */
static const struct progress_marks story_marks[] = {
    {sonic_marks, sizeof(sonic_marks) / sizeof(sonic_marks[0])},
    {tails_marks, sizeof(tails_marks) / sizeof(tails_marks[0])},
    {knuckles_marks, sizeof(knuckles_marks) / sizeof(knuckles_marks[0])},
    {amy_marks, sizeof(amy_marks) / sizeof(amy_marks[0])},
    {big_marks, sizeof(big_marks) / sizeof(big_marks[0])},
    {e102_marks, sizeof(e102_marks) / sizeof(e102_marks[0])},
    {last_marks, sizeof(last_marks) / sizeof(last_marks[0])},
};

int story_progress(int story)
{
    int percent = 0;
    int i;
    const struct progress_mark *mark;

    for (i = 0; i < story_marks[story].count; i++) {
        mark = &story_marks[story].marks[i];
        switch (mark->kind) {
        case MARK_FLAG:
            if (event_flag_check(mark->number)) {
                percent = mark->percent;
            }
            break;
        case MARK_EVENT:
            if (event_was_played(mark->number)) {
                percent = mark->percent;
            }
            break;
        }
    }
    return percent;
}
