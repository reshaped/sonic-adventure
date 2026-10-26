/* a debug menu that changes Float values from the second pad */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "b_robo/tuning.h"
#include "system/frame.h"
#include "system/pad.h"

#define ENTRIES 20

/* the index of the entry the menu shows and changes */
static Sint8 tuning_picked;
/* the index of the last entry added, or -1 for none */
static Sint8 tuning_last;
static struct erobo_tuning *tuning_entries[ENTRIES];

void erobo_tuning_clear(void)
{
    int i;

    for (i = 0; i < ENTRIES; i++) {
        tuning_entries[i] = NULL;
    }
    tuning_picked = 0;
    tuning_last = -1;
}

void erobo_tuning_add(struct erobo_tuning *entry)
{
    int i;

    if (entry != NULL) {
        for (i = 0; i < ENTRIES; i++) {
            if (tuning_entries[i] == NULL) {
                tuning_entries[i] = entry;
                tuning_last = i;
                break;
            }
        }
    }
}

void erobo_tuning_step(void)
{
    struct erobo_tuning *entry;

    if (tuning_last >= 0) {
        if (per_info[1]->press & PDD_DGT_KL) {
            if (--tuning_picked < 0) {
                tuning_picked = tuning_last;
            }
        }
        if (per_info[1]->press & PDD_DGT_KR) {
            if (++tuning_picked > tuning_last) {
                tuning_picked = 0;
            }
        }
        entry = tuning_entries[tuning_picked];
        if (pad_held_live[1] & PDD_DGT_KU) {
            *entry->value += entry->step;
        }
        if (pad_held_live[1] & PDD_DGT_KD) {
            *entry->value -= entry->step;
        }
        if (frame_step == 0) {
            njPrintSize(8);
            njPrintC(NJM_LOCATION(0, 11), entry->label);
            njPrintF(NJM_LOCATION(0, 12), *entry->value, 5);
            njPrintSize(16);
        }
    }
}
