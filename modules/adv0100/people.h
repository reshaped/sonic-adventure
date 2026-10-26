/* the Egg Carrier outside's people, whom player 0 can talk to */
#ifndef ADV0100_PEOPLE_H
#define ADV0100_PEOPLE_H

/* make egg_carrier_script_table the table script_pick runs, and create the
 * people task. the task places the act's people at once. on the step it
 * finds the act changed, it destroys them, and on the next it places the new
 * act's. the field task calls this on its first step */
void egg_carrier_people_init(void);
/* place the first three of the act's people whose rows' scripts pass, each
 * as a task of its own. a person whose row picks no pages shows no lines.
 * place forgets the people placed before, so call remove first. remove
 * destroys the people the last place placed. 1ST_READ.BIN's
 * field_people_place calls place as an event ends, and field_people_remove
 * calls remove as an event or a battle starts */
void egg_carrier_people_place(void);
void egg_carrier_people_remove(void);

#endif
