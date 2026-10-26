/* the boss's name, typed a letter at a time as a fight starts */
#ifndef HUD_BOSS_NAME_H
#define HUD_BOSS_NAME_H
#include <ninja.h>

/* the name's font, which the stages' texture tables load */
extern NJS_TEXLIST b32ascii_texlist;

/* type a copy of text at x, y after delay steps, centered for a negative
 * x, then hold it and fade it out. text needs 1 to 127 characters, and
 * delay at least 0. nothing checks that the task and its records were
 * allocated */
void boss_name_create(const char *text, int x, int y, int delay);

#endif
