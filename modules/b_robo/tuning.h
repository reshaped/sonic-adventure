/* a debug menu that changes Float values from the second pad. no code in
 * the image calls it */
#ifndef B_ROBO_TUNING_H
#define B_ROBO_TUNING_H
#include <sg_xpt.h>

/* an entry of the menu, for the Float at value */
struct erobo_tuning {
    const char *label;
    Float *value;
    /* how far each erobo_tuning_step moves the value while up or down is
     * held */
    Float step;
};

/* empty the menu. erobo_tuning_step needs it to have run first */
void erobo_tuning_clear(void);
/* add entry to the menu, which keeps the pointer, not a copy. a null entry,
 * or a menu that holds 20 already, adds nothing */
void erobo_tuning_add(struct erobo_tuning *entry);
/* run the menu a step. pressing left or right on the second pad picks the
 * entry before or after, wrapping round. holding up or down moves the
 * entry's value by its step. on a frame's first step, the menu prints the
 * entry's label and value at the screen's left. it leaves the print size at
 * 16. it does nothing while the menu is empty */
void erobo_tuning_step(void);

#endif
