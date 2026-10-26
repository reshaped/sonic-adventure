/* Knuckles' Mystic Ruins story section, M EMERALD */
#include <ninja.h>

#include "adv02/mystic_ruins.h"
#include "adv02/script_knuckles_m_emerald.h"
#include "player/player.h"
#include "progress/movie.h"
#include "progress/playable.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

/* the Egg Carrier entrance player 0 arrives at */
#define EGG_CARRIER_ENTRANCE 4

static void script_knuckles_m_emerald_init(void);
static void script_knuckles_m_emerald_step(int seq);

/* player 0 starts event 146 within 200 units of this point across, and 50 above
 * or below it, in the jungle */
static NJS_POINT3 script_knuckles_m_emerald_point = {62.0F, 0.0F, -1736.0F};

struct story_script script_knuckles_m_emerald = {
    script_knuckles_m_emerald_init, script_knuckles_m_emerald_step};

static void script_knuckles_m_emerald_init(void)
{
    story_set_time(TIME_DAY);
}

/* event 145 plays, and then event 146. the story then goes to the Egg Carrier,
 * and MOVIE_SA8 plays on the way unless Knuckles' story is cleared */
static void script_knuckles_m_emerald_step(int seq)
{
    switch (seq) {
    case 0:
        story_save();
        break;
    case 1:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_ISLAND)) {
            return;
        }
        story_play_event(145);
        break;
    case 2:
        playable_unlock(CHARACTER_E102);
        break;
    case 3:
        player_record->unk_01 = 2;
        event_flag_set(FLAG_KNUCKLES_MR_APPEAR_FINAL_EGG);
        event_flag_clear(FLAG_KNUCKLES_TRAIN);
        event_flag_clear(FLAG_KNUCKLES_UNK_84);
        break;
    case 4:
        if (!story_wait_at(STAGE_MYSTIC_RUINS, MYSTIC_RUINS_JUNGLE)) {
            return;
        }
        if (player_in_cylinder(&script_knuckles_m_emerald_point, 200.0F,
                               50.0F) == 1) {
            story_play_event(146);
        } else {
            return;
        }
        break;
    case 5:
        if (event_flags[FLAG_CLEAR_KNUCKLES]) {
            movie_request = 0;
        } else {
            movie_request = MOVIE_SA8;
        }
        stage_set_entrance(EGG_CARRIER_ENTRANCE);
        story_go_to(STAGE_EGG_CARRIER, 0);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
