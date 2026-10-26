/* each character's play and clear counts, and the play history in the
 * console's flash memory */
#ifndef SYSTEM_HISTORY_H
#define SYSTEM_HISTORY_H
#include <sg_xpt.h>

/* the characters counted, in history_counts' order: Sonic, Tails,
 * Knuckles, Amy, E-102 and Big */
#define HISTORY_CHARACTERS 6

/* a character's HISTORY_SIZE bytes of history_counts hold its play count
 * and its clear count, at the offsets below. each is a big-endian Uint16.
 * a fifth byte follows, which no code reads or writes */
#define HISTORY_PLAYS 0
#define HISTORY_CLEARS 2
#define HISTORY_SIZE 5

extern Uint8 history_counts[HISTORY_CHARACTERS * HISTORY_SIZE];

/* start the flash library on history.c's work areas, turn its automatic
 * synchs off, and record the play's time, counting neither a save nor a
 * load */
void history_init(void);
/* add one to character's play count, or to its clear count. character is 0
 * to 5, in HISTORY_CHARACTERS' order, and any other is ignored. a count at
 * 0xffff wraps to 0 */
void history_count_play(int character);
void history_count_clear(int character);
/* record the play's time, as history_init does, and write the record to
 * flash memory with ufSynch */
void history_write(void);
/* start the flash library again on history_init's work areas. unlike
 * history_init, it neither turns automatic synchs off nor records a time */
void history_restart(void);
/* start the flash library again and read the running title's record. its
 * 24 play times go to times, and its first start, its previous start and
 * its start count to starts[0] to starts[2]. a failed read goes unchecked,
 * and the copies are then of a record never read */
void history_read(Uint16 *times, Uint32 *starts);

#endif
