/* the stories on the wheel of characters, and how many of them it shows */
#ifndef ADVERTISE_WHEEL_H
#define ADVERTISE_WHEEL_H

/* the stories, by their place on the wheel. STORIES is their number */
enum wheel_story {
    STORY_SONIC,
    STORY_TAILS,
    STORY_KNUCKLES,
    STORY_AMY,
    STORY_BIG,
    STORY_E102,
    STORY_LAST,
    STORIES
};

/* the number of stories the wheel shows. in the adventure it is STORIES once
 * the six characters' stories have ended, or while menu_state.all_open is set.
 * it is STORY_LAST otherwise */
int wheel_count(void);

#endif
