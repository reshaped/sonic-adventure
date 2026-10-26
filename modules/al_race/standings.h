/* the race's standings: the lane in each place, and the board and marker
 * that show them */
#ifndef AL_RACE_STANDINGS_H
#define AL_RACE_STANDINGS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "al_race/racer.h"

struct race_standings {
    Uint8 lanes[RACE_LANES]; /* the lane in each place, from the first */
    Uint8 unk_08[RACE_LANES]; /* only ever cleared */
    Float scales[RACE_LANES]; /* each place's sprite's scale on the board */
    /* the followed lane's sprite's angle, while spin is under a full turn */
    Angle spin;
    Float marker_y; /* the marker's height */
    Float marker_speed; /* how far the marker rises a step */
    Uint8 marker_state; /* 0 until the marker has a height, then 1 */
    Uint8 marker_lane; /* the lane standings_draw_marker last took */
    /* the steps standings_update has run, which blink the board's pointer */
    Uint16 timer;
};

/* set standings up: each place holds the lane of its own number, and spin
 * is 0. the first three places' scales are 1.15, and the rest 0.85. the next
 * standings_draw_marker starts the marker at its pos's height. timer keeps its
 * count */
void standings_init(struct race_standings *standings);
/* swap lanes toward the racers' order, with racers at their goal ahead and
 * the rest by distance along the course. step the scales, the spin and the
 * timer. lane is unread */
void standings_update(struct race_standings *standings, struct racer *racers,
                      int lane);
/* draw the board: each place's lane, and the pointer at lane's place. lane
 * must be 0 to 7 */
void standings_draw(struct race_standings *standings, struct racer *racers,
                    int lane);
/* draw the marker at pos's x and z, at a height that springs toward pos's y
 * and stays within 5 of it. a marker_lane other than the last call's starts
 * the marker again at pos's height, at rest */
void standings_draw_marker(struct race_standings *standings, int marker_lane,
                           NJS_POINT3 *pos);
/* the lane in place rank, or -1 for a rank outside 0 to 7 */
int standings_lane(struct race_standings *standings, int rank);
/* the index of the first of entries' eight bytes that equals key, or -1 */
int standings_index_of(const Uint8 *entries, int key);

#endif
