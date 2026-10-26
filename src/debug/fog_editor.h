/* the fog editor: edits an act's fog tables, fog color and fog density in
 * place, and saves the tables to the development host or loads them back */
#ifndef DEBUG_FOG_EDITOR_H
#define DEBUG_FOG_EDITOR_H
#include <ninja.h>
#include <sg_xpt.h>

/* fog_editor_init readies the editor, and fog_editor_exec runs a step of
 * it. the first step after fog_editor_init opens the menu, and each step
 * after that runs it. a step that runs the menu, unless the menu exits on
 * it, hides the menu's buttons from the game as editor_take_buttons does.
 * those are the first pad's A, B, X and Y, its triggers and Start. the step
 * after the menu exits returns -1 and leaves the editor as fog_editor_init
 * does. every other step returns 0. the edit page needs the tables, the
 * color and the density set */
void fog_editor_init(void);
int fog_editor_exec(void);

/* forget the tables, the color and the density */
void fog_editor_reset(void);
/* does nothing */
void fun_8c0a8d62(void);
/* give the editor what it edits in place: count tables of FOG_TABLE_ENTRIES
 * levels, the color as ARGB, and the density. the editor borrows each until
 * fog_editor_reset or the next call that sets it. the file page saves the
 * tables' count * FOG_TABLE_ENTRIES bytes to the development host as
 * FOG_MR02.bin. it loads that file over them whole, so the file must fit in
 * them */
void fog_editor_set_tables(Uint8 *tables, int count);
void fog_editor_set_color(Uint32 *color);
void fog_editor_set_density(Float *density);
/* fill table from the editor's table at index: each entry is a level times
 * 1 / 255, so a level of 255 gives 1.0. without tables, or with index
 * outside them, write nothing */
void fun_8c0a8da0(NJS_FOG_TABLE table, int index);
/* fill table as fun_8c0a8da0 does, with each entry times scale. scale is
 * clamped to 0 to 1 first */
void fog_editor_make_table(NJS_FOG_TABLE table, int index, Float scale);

#endif
