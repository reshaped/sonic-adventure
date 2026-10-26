/* shadow under the field's people and walkers */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/shadow.h"
#include "draw/model.h"
#include "object/models.h"
#include "system/texture.h"

/* the shadow's height above the matrix */
#define PERSON_SHADOW_LIFT 0.6F
/* the shadow's scale in x and y for each unit of radius, and its scale in z
 * as a share of that */
#define SHADOW_SCALE 0.25F
#define SHADOW_SCALE_Z 0.8F

void shadow_draw_person(Float radius)
{
    Float scale;

    texture_set_regular();
    njTranslate(NULL, 0.0F, PERSON_SHADOW_LIFT, 0.0F);
    njPushMatrix(NULL);
    scale = radius * SHADOW_SCALE;
    njScale(NULL, scale, scale, scale * SHADOW_SCALE_Z);
    draw_model(&shadow_model);
    njPopMatrix(1);
}
