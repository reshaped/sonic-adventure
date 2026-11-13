/* the shadow under the past's people and walkers */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv03/shadow.h"
#include "draw/model.h"
#include "object/models.h"
#include "system/texture.h"

/* the shadow's height above the matrix */
#define SHADOW_LIFT 0.6F
/* the shadow's scale in x and y for each unit of radius. its scale in z is
 * SHADOW_Z_SHARE times that */
#define SHADOW_SCALE 0.25F
#define SHADOW_Z_SHARE 0.8F

void shadow_draw_person(Float radius)
{
    Float scale;

    texture_set_regular();
    njTranslate(NULL, 0.0F, SHADOW_LIFT, 0.0F);
    njPushMatrix(NULL);
    scale = radius * SHADOW_SCALE;
    njScale(NULL, scale, scale, scale * SHADOW_Z_SHARE);
    draw_model(&shadow_model);
    njPopMatrix(1);
}
