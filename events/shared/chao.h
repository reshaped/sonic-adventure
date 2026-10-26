/* an event's Chao, which draws the model each event defines */
#ifndef EVENT_CHAO_H
#define EVENT_CHAO_H

#include "system/task.h"

/* the textures a Chao's eyes show, by their index in event_chao_texlist:
 * al_eye, al_angrry, al_merry, al_sing and al_worry */
#define CHAO_EYES_PLAIN 2
#define CHAO_EYES_ANGRY 7
#define CHAO_EYES_MERRY 8
#define CHAO_EYES_SING 9
#define CHAO_EYES_WORRY 10

/* create a Chao, or return null when task_create fails. it has an event
 * record for the motions it plays and the paths it runs. its eyes show
 * CHAO_EYES_PLAIN until event_chao_set_eyes */
struct task *event_chao_create(void);
/* show texture, by its index in event_chao_texlist, in the Chao's eyes from
 * its next step. task may be null */
void event_chao_set_eyes(struct task *task, int texture);

#endif
