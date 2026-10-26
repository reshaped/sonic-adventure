/* an event's sounds: held sounds in slots, and sounds played once */
#ifndef EVENT_SOUND_H
#define EVENT_SOUND_H

/* make count slots for held sounds, unless slots exist. the calls that take
 * a slot ignore one at or past count, and a negative one is unchecked. they
 * read through null when init's slots are missing: when the heap couldn't
 * give them, or after event_sound_stop_all */
void event_sound_init(int count);
/* kill every held sound and free the slots. each sound stops on its task's
 * next step */
void event_sound_stop_all(void);
/* hold sound effect se in slot for at most steps + 1 steps. a slot that
 * already holds a sound is left alone. the slot stays taken after the sound
 * ends, until event_sound_stop. when no task can be made, nothing plays and
 * the slot stays empty */
void event_sound_start(int slot, int se, int steps);
/* kill the held sound in slot, if there is one, and empty the slot. the
 * sound stops on its task's next step */
void event_sound_stop(int slot);
/* move the held sound's volume, pan or pitch to the value over steps
 * steps. steps 0 or below moves it at once. a move starts from the last
 * value set, 0 for a new sound. an empty slot is ignored */
void event_sound_fade_volume(int slot, int volume, int steps);
void event_sound_fade_pan(int slot, int pan, int steps);
void event_sound_fade_pitch(int slot, int pitch, int steps);

/* sound_play_for's updates for a sound event_sound_play plays, one a step */
#define EVENT_SOUND_PLAY_UPDATES 120

/* play sound effect se once, at volume, pan and pitch, under the next of
 * play.h's EVENT_SOUND_PLAY_IDS ids. it plays for at most
 * EVENT_SOUND_PLAY_UPDATES + 1 steps. the ids come from the calling thread's
 * data, so it must run on the event's thread. the event's end then stops
 * every sound played under them */
void event_sound_play(int se, int volume, int pan, int pitch);

/* an id a script can play its own sounds under, as with sound_play_at_for.
 * the scripts take ids from it to EVENT_SOUND_ID + 7, which lie in the event
 * image, loaded at 0x0cb80000. a held sound's id is its task's address. a
 * played sound's is an address in the event thread's data. both lie in
 * 1ST_READ.BIN's heap_arena, so a script's id never meets theirs */
#define EVENT_SOUND_ID 0x0cb80001

#endif
