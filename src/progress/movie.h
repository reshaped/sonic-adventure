/* MOVIE.BIN playback overlay */
#ifndef PROGRESS_MOVIE_H
#define PROGRESS_MOVIE_H

/* the movie GAME_MODE_MOVIE plays. 0 is a movie too: the mode, not this
 * number, says whether one plays */
extern int movie_request;

/* the lists after the opening, list 0. each plays one of MOVIE.BIN's movies:
 * MOVIE_SA1 plays SA1_600.SFD, and so on to MOVIE_SA8, but MOVIE_SA7 plays
 * SA7_300.SFD */
#define MOVIE_SA1 1
#define MOVIE_SA2 2
#define MOVIE_SA3 3
#define MOVIE_SA4 4
#define MOVIE_SA5 5
#define MOVIE_SA6 6
#define MOVIE_SA7 7
#define MOVIE_SA8 8

/* play list, an index in MOVIE.BIN's lists. the streams are destroyed and
 * MOVIE.BIN loaded before it plays. after it, the vsync function is
 * frame_vsync and movie_request is 0. return 1 after the opening, 0 after
 * any other list, and below 0 for the pads' quit */
int movie_play(int list);

#endif
