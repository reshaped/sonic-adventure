/* the Egg Carrier inside's script table, and the people each act places */
#ifndef ADV0130_SCRIPTS_H
#define ADV0130_SCRIPTS_H

#include <ninja.h>
#include <sg_xpt.h>

#include "event/script.h"

/* a person to place: where it stands and faces, the row whose script picks
 * its lines, and its kind */
struct person_spot {
    NJS_POINT3 pos;
    Sint16 ang_x;
    Sint16 ang_y;
    Sint16 ang_z;
    Sint8 row; /* in egg_carrier_inside_script_table */
    Sint8 kind; /* in people.c's people_kinds */
};

/* an act's people. spots holds count of them, and is null for an act with
 * none */
struct person_list {
    const struct person_spot *spots;
    int count;
};

/* each act's people, by act. only act 3's list has any */
extern const struct person_list egg_carrier_inside_people_lists[6];

/* the people, Tikal's hints and the hint boxes take their lines from its
 * rows. egg_carrier_inside_people_init makes it the field's script table */
extern const struct script_row egg_carrier_inside_script_table[];

#endif
