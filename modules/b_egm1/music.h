/* the fight's music, asked for a few steps into the stage */
#ifndef B_EGM1_MUSIC_H
#define B_EGM1_MUSIC_H

/* forget every stream request, then ask for MUSIC_BOSSALL a few steps on.
 * nothing is asked for when the waiting task can't be made, and its
 * count's allocation is unchecked */
void eggmobile1_music_start(void);
/* does nothing. eggmobile1_end calls it as the stage ends */
void eggmobile1_music_stop(void);

#endif
