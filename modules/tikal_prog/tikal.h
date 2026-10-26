/* Tikal as a player character. TIKAL_PROG.PRS holds her task's step and her
 * texlist, so it must stay loaded while anything holds either */
#ifndef TIKAL_PROG_TIKAL_H
#define TIKAL_PROG_TIKAL_H
#include <ninja.h>

#include "system/task.h"

/* Tikal's player task, which event scripts create with event_create_player */
void tikal_player_step(struct task *task);

/* her textures, which texture_load_character loads from TIKAL.PRS */
extern NJS_TEXLIST tikal_texlist;

#endif
