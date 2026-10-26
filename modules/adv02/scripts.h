/* the Mystic Ruins' people, by act, and the field's script table */
#ifndef ADV02_SCRIPTS_H
#define ADV02_SCRIPTS_H

#include "adv02/people.h"
#include "event/script.h"

#define MYSTIC_RUINS_SCRIPT_ROWS 20

/* the people of each of the field's four acts, indexed by the act */
extern const struct person_list mystic_ruins_people_lists[];
/* the field's script table, which mystic_ruins_messages_load hands to
 * script_set_table. a person_spot's row indexes it, and that row's script
 * picks the person's pages */
extern const struct script_row
    mystic_ruins_script_table[MYSTIC_RUINS_SCRIPT_ROWS];

#endif
