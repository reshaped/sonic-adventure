/* glare: the glow of a light that shines toward the camera */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "draw/draw.h"
#include "effect/glare.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/util.h"

NJS_TEXLIST glare_texlist = {
#include "effect/glare/glare_texlist.inc"
};
static NJS_TEXANIM glare_frame = {
#include "effect/glare/glare_frame.inc"
};
static NJS_SPRITE glare_sprite = {
#include "effect/glare/glare_sprite.inc"
};

void glare_draw(NJS_POINT3 *pos, NJS_VECTOR *dir, Float size, NJS_ARGB *color)
{
    NJS_VECTOR at;
    NJS_VECTOR to_light;
    NJS_VECTOR back; /* level, out of the camera's back */
    Float alpha;
    Float scale;
    Float distance;
    NJS_VECTOR *eye = &camera_work->pos;

    to_light.x = pos->x - eye->x;
    to_light.y = pos->y - eye->y;
    to_light.z = pos->z - eye->z;
    njUnitVector(&to_light);
    back.x = njSin(camera_work->ang.y);
    back.y = 0.0F;
    back.z = njCos(camera_work->ang.y);
    if (njInnerProduct(&to_light, &back) < 0.0F) {
        Float facing = njInnerProduct(dir, &back);

        if (facing < 0.0F) {
            alpha = 0.0F;
        } else {
            facing = facing * facing;
            alpha = (facing / (1.4F - facing)) - 0.4F;
        }
        alpha *= color->a;
    } else {
        alpha = 0.0F;
    }
    scale = size * alpha * 1.5F;
    distance = size * (scale + 10.0F);
    to_light.x *= distance;
    to_light.y *= distance;
    to_light.z *= distance;
    at.x = pos->x - to_light.x;
    at.y = pos->y - to_light.y;
    at.z = pos->z - to_light.z;
    draw_set_material(alpha, color->r, color->g, color->b);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &at);
    TURN(njRotateY, camera_work->ang.y);
    TURN(njRotateX, camera_work->ang.x);
    njScale(NULL, scale, scale, scale);
    njFogDisable();
    njDrawSprite3D(&glare_sprite, 0, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njFogEnable();
    njPopMatrix(1);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}
