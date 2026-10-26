/* the menus' drawing: the screens' fading tints, and names, numbers, times
 * and marks from the menus' PVMs */
#ifndef ADVERTISE_DRAW_H
#define ADVERTISE_DRAW_H

#include <sg_xpt.h>

#include "save/save.h"

/* the windows act_panel_draw draws at a panel's right. PANEL_RECORD's holds
 * a record, PANEL_EMBLEMS_2's a mini game's two emblems, and PANEL_EMBLEMS_3's
 * an action stage's three */
enum panel_window { PANEL_RECORD, PANEL_EMBLEMS_2, PANEL_EMBLEMS_3 };

/* a screen's tint as it fades, for fade from 0 to 1. color_fade_in blends
 * from clear white at 0 to opaque white at 1. color_fade_out is white with
 * alpha (0.75 - fade) * 255, 191 at 0, and is 0 from 0.75 on */
Uint32 color_fade_in(Float fade);
Uint32 color_fade_out(Float fade);
/* draw an item plate in color, its middle at x and y, as menu_draw_row_item
 * draws one at scale 1 */
void item_draw(Uint32 color, Float x, Float y, Float z);
/* draw a trial list's row panel from ava_trialactsel at x and y, and at its
 * right the window that window names */
void act_panel_draw(Float x, Float y, Float z, enum panel_window window);
/* draw story's name in color, for stories 0 to 6. b_chnam_draw draws it from
 * b_chnam, with its top left at x and y. m_chnam_draw and tx_chnam_draw draw
 * it from their PVMs, centered there and scaled */
void b_chnam_draw(int story, Uint32 color, Float x, Float y, Float z);
void m_chnam_draw(int story, Uint32 color, Float x, Float y, Float z,
                  Float scale_x, Float scale_y);
void tx_chnam_draw(int story, Uint32 color, Float x, Float y, Float z,
                   Float scale_x, Float scale_y);
/* draw the mark of row, a row of the file screen's list, from ava_fscmn,
 * centered at x and y. the saves' rows, 0 to 2, show their numbers. the back
 * row, 3, shows an arrow */
void fscmn_draw(Uint8 row, Float x, Float y, Float z);
/* draw number's lowest digits in a row of digits places, right-aligned, with
 * the leftmost place's middle at x. leading zeros show when zeros is 1, and
 * number must not be negative. suuji_draw draws ava_suuji's digits, their
 * places a digit's width times spacing apart. new16no_draw draws ava_new16no's,
 * 18 dots apart, scaled by 1.1 times scale_x and scale_y. suuji_draw_all and
 * new16no_draw_all draw every digit number has */
void suuji_draw(int number, int digits, Uint8 zeros, Float x, Float y, Float z,
                Float spacing);
void suuji_draw_all(int number, Float x, Float y, Float z);
void new16no_draw(int number, int digits, Uint8 zeros, Float x, Float y,
                  Float z, Float scale_x, Float scale_y);
void new16no_draw_all(int number, Float x, Float y, Float z, Float scale_x,
                      Float scale_y);
/* draw ava_emblem's emblem, centered at x and y */
void ava_emblem_draw(Float x, Float y, Float z);
/* draw the arrow that shows a list scrolls on, pointing up when up is TRUE,
 * else down */
void scroll_arrow_draw(Bool up, Float x, Float y, Float z);
/* draw time from x as minutes, seconds and hundredths, two digits each, with
 * colons between. time_draw converts time's frames to hundredths, at 60
 * frames a second. time_draw_hundredths draws frames unconverted, as the
 * hundredths */
void time_draw(struct save_time *time, Float x, Float y, Float z);
void time_draw_hundredths(struct save_time *time, Float x, Float y, Float z);

#endif
