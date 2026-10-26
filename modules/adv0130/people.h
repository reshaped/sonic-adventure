/* the people of the Egg Carrier inside, whom player 0 can talk to. only act
 * 3 has one: E-102, whose script passes for Amy alone */
#ifndef ADV0130_PEOPLE_H
#define ADV0130_PEOPLE_H

/* make egg_carrier_inside_script_table the field's script table, and create
 * the people task. the task places the act's people at once. when the act
 * changes, it removes them, and places the new act's on its next step. when
 * the task or its record can't be had, the table is set all the same, but no
 * task places the people. the field task calls this as it starts */
void egg_carrier_inside_people_init(void);
/* place the act's people whose rows' scripts pass, three at most, in their
 * list's order. placing forgets the people placed before without destroying
 * them, so remove them first. 1ST_READ.BIN's field_people_place calls this
 * as an event ends. where task_create fails, it reads a work through the null
 * task */
void egg_carrier_inside_people_place(void);
/* destroy the people placed since the last egg_carrier_inside_people_place.
 * field_people_remove calls this as an event or a battle starts */
void egg_carrier_inside_people_remove(void);

#endif
