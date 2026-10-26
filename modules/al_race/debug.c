/* the race's debug buttons, on the second pad */
#include <shinobi.h>

#include "al_race/cracker.h"
#include "al_race/debug.h"
#include "al_race/game.h"
#include "al_race/race_camera.h"
#include "system/pad.h"

#define DEBUG_PAD 1 /* the second pad's index */

void race_debug_step(struct race_game *game)
{
    if (pad_records[DEBUG_PAD].press & PDD_DGT_TA) {
        if (cracker_on()) {
            cracker_stop();
        } else {
            cracker_start();
        }
    }
    if (pad_records[DEBUG_PAD].press & PDD_DGT_TL) {
        race_set_camera_mode(game, RACE_CAMERA_FINISH);
    }
}
