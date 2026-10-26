/* Station Square people and the field's script table */
#ifndef ADV00_PEOPLE_H
#define ADV00_PEOPLE_H

#include "event/script.h"

/* rows 0 to 11 pick the people's pages, and rows 12 to 19 the hint boxes' */
extern const struct script_row station_square_script_table[];

/* place the act's people whose rows' scripts pass, three at most.
 * 1ST_READ.BIN's field_people_place calls it as an event ends, and the crowd
 * task as an act starts. placing again before station_square_people_remove
 * leaves the people placed before untracked */
void station_square_people_place(void);

/* destroy the people station_square_people_place placed. 1ST_READ.BIN's
 * field_people_remove calls it as an event or a battle starts, and the crowd
 * task as the act changes */
void station_square_people_remove(void);

#endif
