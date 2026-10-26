/* MOVIE.BIN playback overlay */
#include <ninja.h>
#include <shinobi.h>

#include "draw/model.h"
#include "movie/movie.h"
#include "progress/movie.h"
#include "sound/stream.h"
#include "system/cache.h"
#include "system/frame.h"
#include "system/loader.h"

/* MOVIE.BIN loads at its entry, movie_entry */
#define MOVIE_LOAD ((void *)movie_entry)

int movie_request;

/* the vsync function is loader_poll_drive while the movie plays. a failed
 * read of MOVIE.BIN goes unchecked: the call runs whatever is at
 * MOVIE_LOAD */
int movie_play(int list)
{
    int result;

    stream_destroy();
    njSetBorderColor(0);
    njSetBackColor(0, 0, 0);
    njWaitVSync();
    njSetVSyncFunction(loader_poll_drive);
    loader_read_file("MOVIE.BIN", MOVIE_LOAD);
    syCacheICI();
    result = movie_entry(list);
    cache_ram_restore();
    draw_model_init();
    njSetVSyncFunction(frame_vsync);
    movie_request = 0;
    return result;
}
