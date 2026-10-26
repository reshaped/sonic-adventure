/* MOVIE.BIN, the movie player, which plays its lists of movies */
#ifndef MOVIE_MOVIE_H
#define MOVIE_MOVIE_H

/* MOVIE.BIN's first bytes, where movie_play calls it: movie_main(list) */
int movie_entry(int list);
/* play list, 0 to 8. a list ends at its last movie, or early at a movie
 * mwPlyCreateSofdec makes no player for or whose sound ADX fails to decode.
 * Start also ends the opening, list 0. the pads' quit is A, B, X, Y and
 * Start held together on a pad. in the opening it takes the console back to
 * the boot ROM, and movie_main never returns.
 *
 * return -1 for a list the pads' quit ended. otherwise return 1 for the
 * opening, and 0 for any other list */
int movie_main(int list);

#endif
