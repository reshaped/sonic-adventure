/* the debug flag, debug axes and lines, and the names of frame files and
 * stages */
#ifndef DEBUG_DEBUG_H
#define DEBUG_DEBUG_H
#include <ninja.h>
#include <sg_xpt.h>

struct task_work;

/* set or clear the debug flag, and return TRUE if it changed */
Bool debug_set_flag(Sint16 on);
/* draw axes 30 long each way at the work's position */
void debug_draw_axes(struct task_work *work);
/* draw an orange line from pos to pos + dir * length */
void debug_draw_line(NJS_POINT3 *pos, NJS_VECTOR *dir, Float length);
/* write the next frame's file name, from frame0000.bmp up in hex. nothing
 * reads the name */
void debug_name_frame(void);
/* does nothing */
void fun_8c035f18(void);
/* print the name of stage_act's stage and act at loc. stage_act packs
 * them as STAGE_ACT does. a stage or an act past the table prints ERROR,
 * and an act past its stage's names passes null to njPrintC */
void debug_print_stage_name(Int loc, Sint16 stage_act);

#endif
