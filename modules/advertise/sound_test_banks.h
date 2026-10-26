/* the sound test's banks. each has a file, as sound_load_file loads it, which
 * holds driver banks: the sound driver's own. the test plays a bank's sounds
 * in groups */
#ifndef ADVERTISE_SOUND_TEST_BANKS_H
#define ADVERTISE_SOUND_TEST_BANKS_H

#include <sg_xpt.h>

/* a group's count sounds, each by its number in the driver bank the group
 * plays from, as sound_slot_play takes it */
struct sound_test_group {
    Uint8 *numbers;
    int count;
};

/* a bank's count groups. driver_banks[g] is the driver bank group g plays
 * from, as sound_slot_play takes it */
struct sound_test_bank {
    struct sound_test_group *groups;
    Uint8 *driver_banks;
    int count;
};

#define SOUND_TEST_BANKS 40

/* the name of each bank's file, and its groups, by the number the screen
 * shows for the bank */
extern char *sound_test_bank_files[SOUND_TEST_BANKS];
extern struct sound_test_bank *sound_test_banks[SOUND_TEST_BANKS];

#endif
