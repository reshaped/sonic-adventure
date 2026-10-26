/* the people of the Mystic Ruins' acts, whom player 0 can talk to */
#ifndef ADV02_PEOPLE_H
#define ADV02_PEOPLE_H

#include <ninja.h>
#include <sg_xpt.h>

/* where a person stands, and its row and kind */
struct person_spot {
    NJS_POINT3 pos;
    Sint16 ang_x;
    Sint16 ang_y;
    Sint16 ang_z;
    Sint8 row; /* in the field's script table */
    Sint8 kind;
};

/* an act's people */
struct person_list {
    const struct person_spot *spots;
    int count;
};

/* place up to three of the act's people, those whose rows pick lines, and
 * destroy them all. 1ST_READ.BIN's field_people_place calls the first as an
 * event ends, and field_people_remove the second as an event or battle starts.
 * the crowd task calls both as an act starts and changes */
void mystic_ruins_people_place(void);
void mystic_ruins_people_remove(void);

#endif
