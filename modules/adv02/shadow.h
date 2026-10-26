/* shadow under the field's people and walkers */
#ifndef ADV02_SHADOW_H
#define ADV02_SHADOW_H
#include <sg_xpt.h>

/* draw shadow_model with the regular texture list 0.6 units above the current
 * matrix, or with the current list while the regular slot is empty. it is
 * sized for a person of that radius: radius / 4 scales it in x and y, and
 * four fifths of that in z. the matrix stays raised by the 0.6 units */
void mystic_ruins_shadow_draw_person(Float radius);

#endif
