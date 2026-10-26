/* the dust an E-100 series robot raises as it hovers. it is a copy of
 * E-102's e102_hover_dust and its params */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "effect/dust.h"
#include "stg05/hover_dust.h"
#include "system/task.h"
#include "system/util.h"

/* the hover's dust: pale blue puffs that glow. before each puff's create,
 * e100_hover_dust sets its scale, pos and speed here */
static struct dust_params hover_dust = {
    1.0F,
    0.03F,
    0.19F,
    0.05F,
    0.025F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.3F, 0.8F, 0.8F, 1.0F},
};

void e100_hover_dust(struct task_work *work, Float ground)
{
    Float height = work->pos.y - ground;
    Angle heading = work->ang.y;
    Float back_x = -njCos(heading) * 10.0F;
    Float back_z = -njSin(heading) * 10.0F;

    if (height > 0.0F && height < 30.0F) {
        Angle ang =
            (Angle)(rand() * RAND_SCALE * (2 * NJD_PI) * ANGLE_PER_RADIAN);
        Float speed = (21.0F - height) * 0.25F;

        hover_dust.scale = 0.5F + (speed * 0.1F);
        hover_dust.pos.x = work->pos.x + back_x + (njCos(ang) * 7.0F);
        hover_dust.pos.y = ground + 1.5F;
        hover_dust.pos.z = work->pos.z + back_z + (njSin(ang) * 7.0F);
        hover_dust.speed.x = speed * ((rand() * RAND_SCALE) - 0.5F);
        hover_dust.speed.y = 0.0F;
        hover_dust.speed.z = speed * ((rand() * RAND_SCALE) - 0.5F);
        dust_create_glow(&hover_dust);
    }
}
