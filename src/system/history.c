/* each character's play and clear counts, and the play history in the
 * console's flash memory */
#include <sg_uf.h>
#include <sg_xpt.h>

#include "system/history.h"
#include "system/loader.h"

/* the cap a count's add is tested against */
#define COUNT_MAX 0xffff

/* ufInitSystem's third and fourth work areas, which the safe refresh
 * copies partitions through, back to back at WORK_AREA */
#define HISTORY_WORK3 WORK_AREA
#define HISTORY_WORK4 (WORK_AREA + UFD_WORK3_SIZ)

#define PLAY_TIMES 24

/* ufPlayTime's est and carry, as this file passes them: no rating of the
 * play, and no progress */
#define NO_RATING 0xff
#define NO_PROGRESS 0xff

Uint8 history_counts[HISTORY_CHARACTERS * HISTORY_SIZE];
/* ufInitSystem's first two work areas: the flash manager's work, and its
 * one-packet buffer */
static Uint32 history_work1[UFD_WORK1_SIZ / 4];
static Uint32 history_work2[UFD_WORK2_SIZ / 4];

void history_init(void)
{
    /* @hack redundant: 240 bytes of frame that no code reads, of a type
     * unknown, declared as a UFS_AllPlayHistory. the image's MOV.W #-240 and
     * ADD R0,R15 reserve them (0x8c116be2, 0x8c116be4); without them, 21 of 22
     * words differ */
    UFS_AllPlayHistory history;

    ufInitSystem(history_work1, history_work2, HISTORY_WORK3, HISTORY_WORK4);
    ufAutoSynch(0);
    ufPlayTime(UFD_PLAY, NO_RATING, NO_PROGRESS, NULL);
}

void history_count_play(int character)
{
    Uint16 count;
    int i;

    if (character < 0 || character >= HISTORY_CHARACTERS) {
        return;
    }
    i = (character * HISTORY_SIZE) + HISTORY_PLAYS;
    count = (history_counts[i] << 8) + history_counts[i + 1];
    /* @bug arithmetic: the cap is tested after the add, which has already
     * wrapped a count at 0xffff to 0 (EXTU.W R4,R3 at 0x8c116c3a, then
     * CMP/GE at 0x8c116c3c) */
    count++;
    if (count >= COUNT_MAX) {
        count = COUNT_MAX;
    }
    history_counts[i] = count >> 8;
    history_counts[i + 1] = count % 256;
}

void history_count_clear(int character)
{
    Uint16 count;
    int i;

    if (character < 0 || character >= HISTORY_CHARACTERS) {
        return;
    }
    i = (character * HISTORY_SIZE) + HISTORY_CLEARS;
    count = (history_counts[i] << 8) + history_counts[i + 1];
    /* @bug arithmetic: as in history_count_play, a count at 0xffff wraps to
     * 0 (EXTU.W R4,R3 at 0x8c116cf0, then CMP/GE at 0x8c116cf2) */
    count++;
    if (count >= COUNT_MAX) {
        count = COUNT_MAX;
    }
    history_counts[i] = count >> 8;
    history_counts[i + 1] = count % 256;
}

void history_write(void)
{
    ufPlayTime(UFD_PLAY, NO_RATING, NO_PROGRESS, NULL);
    ufSynch();
}

void history_restart(void)
{
    ufInitSystem(history_work1, history_work2, HISTORY_WORK3, HISTORY_WORK4);
}

void history_read(Uint16 *times, Uint32 *starts)
{
    UFS_AllPlayHistory history;
    int i;

    ufInitSystem(history_work1, history_work2, HISTORY_WORK3, HISTORY_WORK4);
    ufReadAllHead(NULL, &history);
    for (i = 0; i < PLAY_TIMES; i++) {
        times[i] = history.PlayTime[i];
    }
    starts[0] = history.FirstStartTime;
    starts[1] = history.PrevStartTime;
    starts[2] = history.StartCount;
}
