/* the sound test's music, by kind */
#ifndef ADVERTISE_SOUND_TEST_MUSIC_H
#define ADVERTISE_SOUND_TEST_MUSIC_H

/* a track's title, and its music, as advertise_music_start takes it */
struct sound_test_track {
    const char *title;
    int music;
};

/* a kind's count tracks */
struct sound_test_list {
    const struct sound_test_track *tracks;
    int count;
};

/* the kinds of music, SOUND_TEST_MENU's first six items: the characters'
 * themes to the jingles. sound_test_music holds each one's tracks, in the
 * menu's order */
#define SOUND_TEST_LISTS 6

extern const struct sound_test_list sound_test_music[SOUND_TEST_LISTS];

#endif
