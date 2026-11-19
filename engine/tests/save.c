/* the game's saves, src/save/save.c, and the toggles' chord,
 * src/system/port.c, on a host with a game of the test's own. a save keeps
 * 16:9 and 60 fps as they stood when it was stored. its load gives each
 * toggle that state, but for a toggle the player has set by the chord since
 * boot. engine-test runs this. it returns 0, or prints the line of the first
 * check that fails and returns 1 */
#include <stdio.h>

#include <ninja.h>
#include <shinobi.h>

#include "hud/hud.h"
#include "progress/progress.h"
#include "save/card.h"
#include "save/minigame.h"
#include "save/save.h"
#include "stage/stage.h"
#include "system/chord.h"
#include "system/timer.h"

/* what a save keeps and loads of the game's progress, which no check reads */
int play_time;
Sint8 lives;
int save_slot;
Uint8 event_flags[FLAG_COUNT];
Sint8 events_played[EVENT_COUNT];
Uint8 dat_8c7a2718[CHARACTER_COUNT];
Sint8 stage_clears[CHARACTER_COUNT][STAGE_COUNT];
struct story_record story_records[CHARACTER_COUNT];
int get_character(void)
{
    return CHARACTER_SONIC;
}
void set_character_number(Sint16 character)
{
    (void)character;
}

/* what save_reset and save_reset_all call. no check resets a save */
void board_reset_icecap_scores(struct save *save)
{
    (void)save;
}
void board_reset_sunaboard_scores(struct save *save)
{
    (void)save;
}
void cart_reset_times(struct save *save)
{
    (void)save;
}
void ring_reset(void)
{
}

/* what port.c reads of the game, and calls of a machine's overlay */
NJS_SCREEN _nj_screen_;
Bool draw_off;
Float port_view_aspect = 1.0F;
int overlay_page(void)
{
    return FALSE;
}
void overlay_note(int toggle)
{
    (void)toggle;
}

#define CHECK(test)                                                            \
    do {                                                                       \
        if (!(test)) {                                                         \
            return __LINE__;                                                   \
        }                                                                      \
    } while (0)

/* the saves: one stored with 16:9 on, one with 60 fps on, and one the checks
 * store into. each holds SAVE_NO_MESSAGES too, a bit no load gives a toggle */
enum { WIDE_SAVE, FAST_SAVE, STORED };

static void load(int slot)
{
    save_slot = slot;
    save_load();
}

/* the flags a store leaves in STORED */
static int store(void)
{
    save_slot = STORED;
    save_store();
    return saves[STORED].flags;
}

/* hold the chord with a press of direction, then let go of it all */
static void chord(Uint32 direction)
{
    PDS_PERIPHERAL pad = {0};

    pad.on = PDD_DGT_TL | PDD_DGT_TR | direction;
    port_chord(&pad);
    pad.on = 0;
    port_chord(&pad);
}

static int check(void)
{
    saves[WIDE_SAVE].flags = SAVE_NO_MESSAGES | SAVE_WIDE;
    saves[FAST_SAVE].flags = SAVE_NO_MESSAGES | SAVE_FAST;
    saves[STORED].flags = SAVE_NO_MESSAGES | SAVE_WIDE;
    /* the player has set neither toggle. each load gives both its save's
     * states, so a save turns off what the one before turned on */
    load(WIDE_SAVE);
    CHECK(port_toggles[TOGGLE_WIDE] && !port_toggles[TOGGLE_FAST]);
    load(FAST_SAVE);
    CHECK(!port_toggles[TOGGLE_WIDE] && port_toggles[TOGGLE_FAST]);
    load(WIDE_SAVE);
    CHECK(port_toggles[TOGGLE_WIDE] && !port_toggles[TOGGLE_FAST]);
    /* the chord turns 16:9 off and 60 fps on. a load of the save that holds
     * the opposite of each leaves both. a store keeps them */
    chord(PDD_DGT_KR);
    chord(PDD_DGT_KD);
    load(WIDE_SAVE);
    CHECK(!port_toggles[TOGGLE_WIDE] && port_toggles[TOGGLE_FAST]);
    CHECK(store() == (SAVE_NO_MESSAGES | SAVE_FAST));
    /* the chord turns them back, and the other save leaves them so */
    chord(PDD_DGT_KR);
    chord(PDD_DGT_KD);
    load(FAST_SAVE);
    CHECK(port_toggles[TOGGLE_WIDE] && !port_toggles[TOGGLE_FAST]);
    CHECK(store() == (SAVE_NO_MESSAGES | SAVE_WIDE));
    return 0;
}

int main(void)
{
    int line = check();

    if (line != 0) {
        fprintf(stderr, "save: line %d\n", line);
    }
    return line != 0;
}
