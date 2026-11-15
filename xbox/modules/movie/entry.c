/* MOVIE.BIN's entry, for the port: make pc compiles this in place of
 * modules/movie/entry.src, whose first bytes branch to the player */
#include "modules/movie/movie.h"

int movie_main(int list);

int movie_entry(int list)
{
    return movie_main(list);
}
