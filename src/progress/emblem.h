/* emblems: their bits in the save, the fields' pickups and the count
 * display */
#ifndef PROGRESS_EMBLEM_H
#define PROGRESS_EMBLEM_H
#include <ninja.h>
#include <sg_xpt.h>

struct save;
struct task;

/* mission grades, ordered hardest to easiest */
enum mission { MISSION_A, MISSION_B, MISSION_C };

/* the emblem's textures, which the fields load */
extern NJS_TEXLIST emblem_texlist;

/* clear all emblem bits */
void emblem_clear_all(struct save *save);
/* TRUE if save has emblem number. number must be from 0 to
 * EMBLEM_BYTES * 8 - 1, which neither this nor the setters check */
Bool emblem_check(struct save *save, int number);
/* the number of emblems the save has: the bits set in its emblem map */
int emblem_count(struct save *save);
/* the emblem number of character's mission at stage. a stage without
 * missions gives 0, which is also Sonic's A emblem for STAGE_BEACH. an
 * action stage where character has no missions gives another stage's
 * number */
int emblem_mission_number(int character, int stage, int mission);
/* set or check story and mission emblems. in GAME_MODE_FIELD, setting one
 * also calls save_update_write, which starts writing the saves to the
 * memory card. elsewhere it changes only save */
void emblem_set_story(struct save *save, int character);
Bool emblem_check_story(struct save *save, int character);
Bool emblem_check_mission(struct save *save, int character, int stage,
                          int mission);
void emblem_set_mission(struct save *save, int character, int stage,
                        int mission);
/* the step of a field's emblem pickup. its scl.x picks which of the
 * field's four emblems it is: 0 to 3, by its whole part. a pickup whose
 * emblem the save has removes itself. otherwise it turns by ang.x each step
 * until a player touches it. the touch sets its bit, vibrates the player's
 * pad and starts a shine: the emblem grows and fades where it was, while
 * the count shows */
void emblem_step(struct task *task);
/* destroy and reset the streams, and start a task that shows the emblem
 * count over a dark screen. it shows the save's total less one, then the
 * total blinking, then fades. the pads are off while it shows, and it waits
 * undrawn while hud_hidden is below 0. return the task, or null when it
 * can't be made, which leaves the count's textures loaded */
struct task *emblem_start_count_display(void);

#endif
