/* Sonic's Station Square story section, SS SEWER - CASINO */
#include "adv00/script_sonic_sewer.h"
#include "adv00/station_square.h"
#include "progress/progress.h"
#include "progress/story.h"
#include "stage/stage.h"

static void script_sonic_sewer_init(void);
static void script_sonic_sewer_step(int seq);

struct story_script script_sonic_sewer = {script_sonic_sewer_init,
                                          script_sonic_sewer_step};

/* set evening at seq 0, and night from seq 3 on, the time seq 2's step sets as
 * Sonic leaves the sewers. seq 1 and 2 leave the time as it is */
static void script_sonic_sewer_init(void)
{
    struct story_record *record = player_record;

    if (record->seq == 0) {
        story_set_time(TIME_EVENING);
    } else if (record->seq >= 3) {
        story_set_time(TIME_NIGHT);
    }
}

/* the barricade goes and the sewers open in the station act. night falls as
 * Sonic leaves them for the office act, where the hotel's back and the
 * station's back open. once Casinopolis is cleared, events 374 and 9 play in
 * the casino act. the act then ends as a clear that returns to the casino
 * act */
static void script_sonic_sewer_step(int seq)
{
    switch (seq) {
    case 0:
        story_save();
        break;
    case 1:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_STATION)) {
            return;
        }
        event_flag_set(FLAG_SONIC_SS_BARRICADE);
        event_flag_set(FLAG_SONIC_SS_ENTRANCE_SEWER);
        break;
    case 2:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_GESUI)) {
            return;
        }
        if (!story_going_to(STAGE_STATION_SQUARE, STATION_SQUARE_OFFICE)) {
            return;
        }
        story_set_time(TIME_NIGHT);
        event_flag_set(FLAG_SONIC_SS_EXIT_SEWER);
        break;
    case 3:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_OFFICE)) {
            return;
        }
        event_flag_set(FLAG_SONIC_SS_HOTEL_BACK);
        event_flag_set(FLAG_SONIC_SS_STATION_BACK);
        story_save();
        break;
    case 4:
        if (!story_wait_at(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO)) {
            return;
        }
        if (!stage_cleared(STAGE_CASINO)) {
            return;
        }
        break;
    case 5:
        event_flag_set(FLAG_SONIC_SS_ICE_STONE);
        story_play_event(374);
        break;
    case 6:
        story_play_event(9);
        break;
    case 7:
        stage_fade_to(STAGE_STATION_SQUARE, STATION_SQUARE_CASINO);
        stage_end(STAGE_END_CLEAR);
        story_next_section();
        break;
    default:
        return;
    }
    story_next_seq();
}
