/* the Egg Carrier outside's script table, and the people each act places */
#ifndef ADV0100_SCRIPTS_H
#define ADV0100_SCRIPTS_H

#include <ninja.h>
#include <sg_xpt.h>

#include "event/script.h"

/* the kinds of person, by their places in people.c's people_kinds */
enum person_kind_id {
    PERSON_SONIC,
    PERSON_TAILS,
    PERSON_KNUCKLES,
    PERSON_AMY,
    PERSON_E102,
    PERSON_BIG,
    PERSON_EGGMOBILE,
    PERSON_CHAOS4,
    PERSON_KINDS
};

/* where a person stands, its row and its kind */
struct person_spot {
    NJS_POINT3 pos;
    Sint16 ang_x;
    Sint16 ang_y;
    Sint16 ang_z;
    Sint8 row; /* in egg_carrier_script_table */
    Sint8 kind; /* an enum person_kind_id */
};

/* an act's people: count spots, or null spots for none */
struct person_list {
    const struct person_spot *spots;
    int count;
};

/* each act's people. only acts 0 and 2 have any */
extern const struct person_list egg_carrier_people_lists[7];

#define EGG_CARRIER_SCRIPT_ROWS 26

/* the rows the people, Tikal's hints and the hint boxes run.
 * egg_carrier_people_init makes it the table script_pick runs */
extern const struct script_row
    egg_carrier_script_table[EGG_CARRIER_SCRIPT_ROWS];

#endif
