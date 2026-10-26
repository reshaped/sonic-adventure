/* the cast: the characters drawn over movie list 7, each running down a
 * path while its action loops. cast_fade's black fades the screen out over
 * them and the movie */
#ifndef MOVIE_CAST_H
#define MOVIE_CAST_H

/* the black over the cast and the movie: 0 for none, an alpha from 1 to
 * 254, or CAST_SOLID for opaque black */
extern int cast_fade;
#define CAST_SOLID (-1)

/* set the view the cast is drawn from, load the textures of the cast for
 * the player's character, and allocate its members' states. Knuckles and
 * Big each have a cast of their own, and any other character has Amy's and
 * Tails' */
void cast_load(void);
/* draw the cast at movie frame frame. a member whose cue is frame starts at
 * its path's first point. each running member is drawn, then moves on, and
 * stops once it reaches its path's last point. the black is drawn last, by
 * cast_fade. a frame below 0 draws nothing */
void cast_draw(int frame);
/* free the states cast_load allocated. the textures stay loaded */
void cast_free(void);

#endif
