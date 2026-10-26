/* the echidnas the object list names O PEOPLE, who stand where the set
 * places them, and the echidnas who walk act 0's route */
#ifndef ADV03_CROWD_H
#define ADV03_CROWD_H

#include "system/task.h"

/* read the current character's message file, and give script its lines table
 * and the field's script table. a file that can't be read leaves no lines
 * table. then create the crowd task. as each act starts, it starts the act's
 * people, and in act 0 the walkers on its route. when the task or its record
 * can't be allocated, the tables are set all the same, and no crowd task
 * runs */
void past_messages_load(void);
/* the step the object list names for O PEOPLE, which starts one of the set's
 * people, standing at its set position. a person doesn't start when its row
 * lies past the people's rows, its row picks no lines, or an allocation
 * fails. this stays the task's step then, and draws the person's plain model
 * each step the set editor is open */
void crowd_person_init(struct task *task);

#endif
