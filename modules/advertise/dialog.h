/* text dialogs: windows of lines drawn into a bitmap. a dialog grows open,
 * shows until closed, and shrinks away */
#ifndef ADVERTISE_DIALOG_H
#define ADVERTISE_DIALOG_H

#include <sg_xpt.h>

#include "system/task.h"

struct dialog_spec {
    char **lines;
    int count;
    int centered; /* 1 to grow from x and y as the middle, else the top left */
    Float x, y, z;
    int unk_18;
    int index; /* its texture's global index */
};

/* a dialog opens, shows, closes and is done. at its next step in
 * DIALOG_DONE, its task kills itself */
enum dialog_mode { DIALOG_OPEN, DIALOG_SHOWN, DIALOG_CLOSE, DIALOG_DONE };

/* a sample warning dialog. nothing in ADVERTISE or SUMMARY, the modules
 * that link dialog.c, shows it */
extern struct dialog_spec dialog_sample;

/* start a dialog's task at level in the task tree, and return it, or NULL.
 * the dialog draws spec's lines into a bitmap, and starts in DIALOG_OPEN.
 * spec is copied, and its lines are read only during the call. the lines
 * must fit a texture of 512 by 512 pixels. every dialog's bitmap draws into
 * one buffer, so a dialog created before another's bitmap is uploaded draws
 * over that bitmap's pixels */
struct task *dialog_create(int level, const struct dialog_spec *spec);
enum dialog_mode dialog_mode(struct task *task);
/* start closing a dialog in DIALOG_SHOWN, and return TRUE. in any other
 * mode, do nothing and return FALSE */
Bool dialog_close(struct task *task);
/* the pixel count of a width by height bitmap's texture, each side rounded
 * up to a power of two of 8 or more, plus 32. the 32 matches the bytes
 * dialog_pixels keeps for bitmap_init's move to a 32-byte boundary, though
 * here it adds to a count of 2-byte pixels. nothing in ADVERTISE or SUMMARY
 * calls it */
int dialog_pixels_size(int width, int height);

#endif
