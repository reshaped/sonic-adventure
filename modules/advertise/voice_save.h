/* the writer of the nine voice files, which nothing calls */
#ifndef ADVERTISE_VOICE_SAVE_H
#define ADVERTISE_VOICE_SAVE_H

/* write the nine voice files to the card in drive A1, each with its icons and
 * voice_save_data as its data. it stops at the first file the backup library
 * refuses: buSaveFile refuses one while the drive is busy. it waits for each
 * save it starts to end, but reads no save's result */
void voices_save(void);

#endif
