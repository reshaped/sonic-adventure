/* VMS, the gardens' memory card station, which takes a Chao or an egg off a
 * card into the garden and puts one onto a card */
#ifndef AL_MAIN_VMS_H
#define AL_MAIN_VMS_H
#include <sg_xpt.h>

#include "system/task.h"

/* the object list's step for the VMS. task needs the work and motion that
 * TASK_WORK and TASK_MOTION allocate. it sets the VMS's own step, draw and
 * destructor, and the card menu's fixed view of the VMS. it clears
 * garden_camera_fixed and alife_paused. while the set editor holds the
 * task, the VMS only draws. a failed alife_entry_add goes unchecked, and
 * vms_put then reads through a null entry */
void vms_init(struct task *task);
/* clear bit 0x80 of *type, in place. then return whether a card's slot may
 * keep the type: 1 for CHAO_CHILD to CHAO_EGG and for CHAO_PARTNER, and 0 for
 * any other */
int card_admit_type(Uint8 *type);

#endif
