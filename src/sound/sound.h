/* sound: the sound driver, and the effects the game queues for it */
#ifndef SOUND_SOUND_H
#define SOUND_SOUND_H
#include <sg_xpt.h>

struct task;

/* effect numbers, se, that more than one file plays */
#define SE_RINGS_LOST 0
#define SE_MOVE 1 /* a menu's cursor moving */
#define SE_CHOOSE 2 /* a menu's choice taken */
#define SE_CANCEL 3 /* a menu's choice that backs out */
#define SE_MENU_OPEN 21 /* a menu opening, or a card coming into a drive */
#define SE_MENU_CLOSE 22 /* a menu closing, or a card leaving a drive */
#define SE_BEEP 4
#define SE_RING 7
#define SE_SPRING 9
#define SE_DASH 699 /* a dash panel sending a player off */
#define SE_TIKAL_CHIME 721 /* the ball of light that circles a character */
#define SE_ITEM 11 /* an item box's item, or a level item, taken */
#define SE_NOTICE 12 /* an emblem won, or a character made playable */
#define SE_SWITCH 13
#define SE_PUSH 15 /* a character pushing, which callers refresh each step */
#define SE_ICE_BREAK 16 /* a character breaking out of the ice */
#define SE_JUMP 17 /* the jump every character shares */
#define SE_SKID 18
#define SE_DOWN 23 /* a character going down */
#define SE_BLAST 26
#define SE_LAND 33
#define SE_GRAB 34
#define SE_SPIN 728 /* a spin as it charges */
#define SE_BOARD_ICECAP_GRIP_LEAST 225
#define SE_BOARD_ICECAP 226
#define SE_BOARD_ICECAP_GRIP_LOW 227
#define SE_BOARD_RAMP 230
#define SE_BOARD_JUMP 231
#define SE_BOARD_CRASH 235
#define SE_BOARD_SAND 690

/* effects of one stage's own bank, which only that stage's files play.
 * another stage's bank holds another sound at each number */
#define SE_MACHINE_RUN 69 /* Twinkle Park's: a machine running */
#define SE_CLUNK 85 /* Twinkle Park's: a latch's clunk */
#define SE_RUBBLE_BLAST 127 /* Red Mountain's: rubble blowing apart */
#define SE_ROCK_BLAST 128 /* Red Mountain's: rock blowing apart */
#define SE_CONFIRM 0x260 /* the Chao's stages': a thing done or chosen */
#define SE_CHAO_SQUEAK 0x266 /* the Chao's stages': a Chao's squeak */

/* the failures of a syCfg call that a retry loop counts before it gives up.
 * a loop that gives up reading the sound mode takes stereo */
#define CFG_TRIES 500000

/* read the driver into WORK_AREA, overwriting it, and start it. every slot
 * of the queue starts free, and the space is as the console is set */
void sound_system_init(void);
/* set the driver to stereo or mono, as the console's sound setting says. a
 * setting it can't read counts as stereo */
void sound_set_space(void);
/* load the stage's effects, and the character's voices and effects, as
 * sound_load_file does. it then resets the fx levels, qsound and the
 * channels. it ends sound_mute's hold on the queue, and nothing else of it */
void sound_load_stage(void);
/* read the file name into WORK_AREA, overwriting it, and send it to the
 * sound memory. sound_load_file sends a file of banks, and the others one
 * bank of their kind as number bank. name must be a file on the disc that
 * fits WORK_AREA in whole 2048-byte sectors. no failure is reported */
void sound_load_file(const char *name);
void sound_load_shot_bank(int bank, const char *name);
void sound_load_sequence_bank(int bank, const char *name);
void sound_load_program_bank(int bank, const char *name);
/* send the file of banks at data to the sound memory, as sound_load_file
 * does from WORK_AREA */
void sound_download(void *data);
/* play number from bank in slot now. slot must be below 38. slots 0 to 31
 * are midi channels, and each slot from 32 on plays sequences on a port of
 * its own. priority counts only on a sequence slot */
void sound_slot_play(int bank, int number, int priority, int slot);
/* stop slot now */
void sound_slot_stop(int slot);
/* set slot's pan now, from -128 to 127. a sequence slot ignores it */
void sound_slot_set_pan(int pan, int slot);
/* set slot's pitch now, as a midi bend from -8192 to 8191. a sequence slot
 * ignores it */
void sound_slot_set_pitch(int pitch, int slot);
/* send pos[0] to pos[6] to qsound as its channels' places, now */
void sound_set_qsound(Sint8 *pos);

/* requests to the queue, which the next sound_send_queue carries out. id is
 * any key the caller chooses, not a slot. a request that needs a slot takes
 * a free one, or one held at no more than priority. each request returns -1
 * when it fails, and no value when it succeeds */

/* queue se to play once, in a slot held at priority -1, which stops it after
 * 120 updates. volume is ignored, and se starts at volume 0 */
int sound_play(int se, int id, int priority, int volume);
/* the same at x, y, z. se starts at volume less a falloff by its distance
 * from the camera. an effect qsound places needs a camera, and fails
 * farther from it than its qsound range squared, 10000 or 40000 */
int sound_play_at(int se, int id, int priority, int volume, Float x, Float y,
                  Float z);
/* queue se to play until stopped, in a slot held at priority. volume is
 * ignored. once se starts, its slot takes no new volume, pan or pitch */
int sound_play_held(int se, int id, int priority, int volume);
/* refresh the slot playing se for id, or queue se in a new one, held at
 * priority with updates + 1 updates left. sound_send_queue counts them down
 * and then stops se. volume is ignored */
int sound_play_for(int se, int id, int priority, int volume, int updates);
/* the same at x, y, z. se plays at volume less a falloff by its distance
 * from the camera, which follows it at every update. an effect qsound places
 * needs a camera, and fails farther from it than its qsound range squared,
 * 10000 or 40000 */
int sound_play_at_for(int se, int id, int priority, int volume, Float x,
                      Float y, Float z, int updates);
/* as sound_play_at_for, but for any se it needs a camera, and fails farther
 * than radius from it */
int sound_play_around(int se, int id, int priority, int volume, Float x,
                      Float y, Float z, Float radius, int updates);
/* stop every slot playing se, or playing for id, at the next update */
void sound_stop(int se);
void sound_stop_id(int id);
/* set the volume, pan or pitch of every slot playing for id, from the next
 * update. the pitch is a midi bend. a volume set here replaces the falloff
 * by distance */
void sound_set_volume(int id, int volume);
void sound_set_pan(int id, int pan);
void sound_set_pitch(int id, int pitch);

/* silence every slot and the streams, and hold the queue. sound_unmute sends
 * each slot its last volume again, and lets the queue run */
void sound_mute(void);
void sound_unmute(void);
/* stop and free every slot, now */
void sound_kill_all(void);
/* carry out the queue, once a step of the game. it starts what the queue
 * holds, sends each playing slot what changed and counts its updates down.
 * it then sends the qsound places that changed, and an fx level asked for.
 * while muted, it does nothing */
void sound_send_queue(void);

/* fun_8c09a47e, sound_end_act, sound_pause and sound_resume do nothing.
 * the stage calls sound_end_act as an act ends, and sound_pause and
 * sound_resume around a pause */
void fun_8c09a47e(void);
void sound_end_act(void);
void sound_pause(void);
void sound_resume(int unused);

/* ask for an fx level on outputs 0 and 1: level, or the most, 15. the next
 * sound_send_queue sends it, and the outputs keep it until another is sent.
 * a request for -6 sends nothing */
void sound_request_fx(int level);
void sound_request_fx_max(void);
/* draw and step the sound test, a debug page, on pads 0 and 3. up and down
 * step the number from 0 to 99, and left and right the bank from 0 to 15. y
 * plays the pick on the test's slot, and its release stops the slot. bank 15
 * plays the disc's voice 594 + number instead, as a stream */
void sound_test(void);
/* load the character's voices, as sound_load_file does */
void sound_load_voice(void);
/* the steps of two sound objects a set file places, which need a camera.
 * each takes its settings from its placement's scl and ang. an fx zone asks
 * for fx level scl.y while the camera is within scl.x of it. an orbit plays
 * effect ang.x at volume scl.y, with its task as id, from a point that
 * circles it at radius scl.x. the point turns by ang.y each step, and the
 * effect plays only within 200 of the camera. each object draws a marker in
 * the editor */
void sound_fx_zone_step(struct task *task);
void sound_orbit_step(struct task *task);
/* load the voices of the alt character, battle_rival, or the field's alt
 * file on the Egg Carrier and in the Mystic Ruins. any other rival or field
 * loads nothing */
void sound_load_alt_voice(void);
void sound_load_alt_field(void);
/* play number from bank now on the sound test's slot, at volume 127. bank 15
 * plays music number instead, and banks from 16 on play the disc's voices,
 * 256 of them to every 16 banks */
void sound_play_bank(Uint8 bank, Uint8 number);

#endif
