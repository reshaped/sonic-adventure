/* the shadow under the past's people and walkers */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv03/shadow.h"
#include "draw/model.h"
#include "object/models.h"
#include "system/texture.h"

void shadow_draw_person(Float radius)
{
    Float scale;

    texture_set_regular();
    njTranslate(NULL, 0.0F, 0.6F, 0.0F);
    njPushMatrix(NULL);
    scale = radius * 0.25F;
    njScale(NULL, scale, scale, scale * 0.8F);
    draw_model(&shadow_model);
    njPopMatrix(1);
}
