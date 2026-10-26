/* the past's people and the field's script table */
#ifndef ADV03_PEOPLE_H
#define ADV03_PEOPLE_H

#include "event/script.h"

/* the field's script table, which past_messages_load gives script_set_table.
 * its rows decide which people past_people_place places, and pick no pages */
extern const struct script_row past_script_table[];

/* place the act's people whose rows' scripts pass, 12 at most.
 * 1ST_READ.BIN's field_people_place calls it as an event ends, and the crowd
 * task as an act starts. placing again before past_people_remove leaves the
 * people placed before untracked */
void past_people_place(void);

/* destroy the people past_people_place placed, the last placed first.
 * 1ST_READ.BIN's field_people_remove calls it as an event or a battle starts,
 * and the crowd task as the act changes */
void past_people_remove(void);

#endif
