/* the Chao race's tuner, which steps Float values from the second pad */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "al_race/tune.h"
#include "system/frame.h"
#include "system/pad.h"

/* the most entries the tuner holds */
#define TUNE_COUNT 20

/* the index in per_info of the pad that tunes, the second */
#define TUNE_PAD 1

/* where the label and the value print */
#define TUNE_LABEL_AT NJM_LOCATION(0, 11)
#define TUNE_VALUE_AT NJM_LOCATION(0, 12)

/* the entry shown, and the index of the last entry. tune_reset sets
 * tune_last to -1 for none, but it starts at 0 */
static Sint8 tune_current;
static Sint8 tune_last;
static struct tune_entry *tune_entries[TUNE_COUNT];

void tune_reset(void)
{
    int i;

    for (i = 0; i < TUNE_COUNT; i++) {
        tune_entries[i] = NULL;
    }
    tune_current = 0;
    tune_last = -1;
}

void tune_add(struct tune_entry *entry)
{
    int i;

    if (entry != NULL) {
        for (i = 0; i < TUNE_COUNT; i++) {
            if (tune_entries[i] == NULL) {
                tune_entries[i] = entry;
                tune_last = i;
                break;
            }
        }
    }
}

void tune_step(void)
{
    struct tune_entry *entry;

    if (tune_last >= 0) {
        if (per_info[TUNE_PAD]->press & PDD_DGT_KL) {
            if (--tune_current < 0) {
                tune_current = tune_last;
            }
        }
        if (per_info[TUNE_PAD]->press & PDD_DGT_KR) {
            if (++tune_current > tune_last) {
                tune_current = 0;
            }
        }
        entry = tune_entries[tune_current];
        if (pad_held_live[TUNE_PAD] & PDD_DGT_KU) {
            *entry->value += entry->step;
        }
        if (pad_held_live[TUNE_PAD] & PDD_DGT_KD) {
            *entry->value -= entry->step;
        }
        if (frame_step == 0) {
            njPrintSize(8);
            njPrintC(TUNE_LABEL_AT, entry->label);
            njPrintF(TUNE_VALUE_AT, *entry->value, 5);
            njPrintSize(16);
        }
    }
}
