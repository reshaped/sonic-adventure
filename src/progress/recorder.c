/* recorder: record pad 0 into memory, or play a recording back over it, for
 * the demos */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "debug/host.h"
#include "player/player.h"
#include "progress/demo.h"
#include "progress/recorder.h"
#include "stage/stage.h"
#include "system/pad.h"

/* the last sample a recording fills from pad 0. PAD_SAMPLE_END fits in the
 * 128 KB after it */
#define RECORD_LAST ((struct pad_sample *)0x0cbfffe8)
/* the last sample a playback reads, past the recording's 128 KB */
#define PLAY_LAST ((struct pad_sample *)0x0cd60000)

Bool recording_on = FALSE;
enum recorder_mode recording_mode = RECORDER_RECORD;
enum recording_kind recording_kind = RECORDING_OFF;
int demo_next = 0;
struct pad_sample *recording_next = DEMO_RECORDING;
Uint32 recording_buttons;
void *dat_8c751454;

void recording_update(void)
{
    struct pad_sample *sample;
    Uint32 old;
    Uint32 on;
    Uint32 changed;
    int size;

    if (!recording_on) {
        return;
    }
    /* a demo of an event plays no recording */
    if (demo_event >= 0) {
        return;
    }
    sample = recording_next;
    switch (recording_mode) {
    case RECORDER_RECORD:
        if (recording_next > RECORD_LAST || (pad_records[0].on & PDD_DGT_TX)) {
            recording_mode = RECORDER_DONE;
            sample->on = PAD_SAMPLE_END;
            sample++;
            size = (Uint8 *)sample - (Uint8 *)DEMO_RECORDING;
            switch (get_character()) {
            case CHARACTER_SONIC:
                host_save_file("K_SONIC.BIN", DEMO_RECORDING, size);
                break;
            case CHARACTER_TAILS:
                host_save_file("K_MILES.BIN", DEMO_RECORDING, size);
                break;
            case CHARACTER_KNUCKLES:
                host_save_file("K_KNUCKLES.BIN", DEMO_RECORDING, size);
                break;
            case CHARACTER_AMY:
                host_save_file("K_AMY.BIN", DEMO_RECORDING, size);
                break;
            case CHARACTER_E102:
                host_save_file("K_E102.BIN", DEMO_RECORDING, size);
                break;
            case CHARACTER_BIG:
                host_save_file("K_BIG.BIN", DEMO_RECORDING, size);
                break;
            /* @hack redundant: the empty default keeps per_info[0] in R3 and
             * its on in R2 for the copy below (0x8c0360d6, 0x8c0360d8); without
             * it, the two trade and 3 of 187 words differ */
            default:
                break;
            }
        } else {
            sample->on = per_info[0]->on;
            sample->l = per_info[0]->l;
            sample->r = per_info[0]->r;
            sample->x1 = per_info[0]->x1;
            sample->y1 = per_info[0]->y1;
        }
        sample++;
        recording_next = sample;
        break;
    case RECORDER_PLAY:
        if (recording_next > PLAY_LAST || sample->on == PAD_SAMPLE_END) {
            recording_on = FALSE;
            stage_end(STAGE_END_QUIT);
        } else {
            on = sample->on;
            old = recording_buttons;
            per_info[0]->on = (per_info[0]->on & PDD_DGT_ST) | on;
            per_info[0]->l = sample->l;
            per_info[0]->r = sample->r;
            per_info[0]->x1 = sample->x1;
            per_info[0]->y1 = sample->y1;
            changed = old ^ on;
            per_info[0]->press =
                (per_info[0]->press & PDD_DGT_ST) | (on & changed);
            per_info[0]->off = ~on;
            per_info[0]->release = old & changed;
            recording_buttons = on;
        }
        sample++;
        recording_next = sample;
        break;
    case RECORDER_DONE:
        /* print recording_next in hex. the call that ended the recording
         * left it a sample past the end of the file it saved */
        njPrintH(NJM_LOCATION(28, 12), (Uint32)sample, 8);
        break;
    }
}
