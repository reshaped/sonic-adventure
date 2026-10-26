/* texture animations of the land in the sewer, station and hotel acts */
#ifndef ADV00_LAND_ANIM_H
#define ADV00_LAND_ANIM_H

enum land_anim_index {
    LAND_ANIM_GESUI,
    LAND_ANIM_STATION_A,
    LAND_ANIM_STATION_B,
    LAND_ANIM_STATION_C,
    LAND_ANIM_HOTEL_A,
    LAND_ANIM_HOTEL_B,
    LAND_ANIM_HOTEL_C
};

/* start the land's texture animation anim. its task ends once the act
 * changes */
void land_anim_start(enum land_anim_index anim);

#endif
