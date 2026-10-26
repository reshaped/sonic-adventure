/* the people the object list names O PEOPLE, who stand where the set places
 * them, and the walkers on the jungle's paths */
#ifndef ADV02_CROWD_H
#define ADV02_CROWD_H

#include "system/task.h"

/* read the field's message file: mr_mes_l.bin while FLAG_LAST_STORY is set,
 * else the current character's. give script its lines table and the field's
 * script table. a file that can't be opened leaves no lines table. then create
 * the crowd task. as each act starts, it starts the act's people, and in the
 * jungle the walkers on its paths. when the task or its record can't be
 * allocated, the tables are set all the same, and no crowd task runs */
void mystic_ruins_messages_load(void);
/* the step the object list names for O PEOPLE, which starts one of the set's
 * people, standing at its set position. a person doesn't start when its row
 * lies past the people's rows, its row picks no lines, or an allocation
 * fails. this stays the task's step then, and draws the person's plain model
 * each step the set editor is open */
void crowd_person_init(struct task *task);

#endif
