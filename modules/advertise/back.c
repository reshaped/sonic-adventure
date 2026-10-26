/* the background most menu screens draw behind them, from ava_back's
 * eight textures */
#include <ninja.h>
#include <sg_xpt.h>

#include "advertise/back.h"
#include "advertise/textures.h"
#include "draw/screen.h"

static struct {
    Sint16 count;
    struct screen_part parts[8];
} back_parts = {
    8,
    {{0, 0, 0},
     {1, 0, 256},
     {2, 256, 0},
     {3, 256, 256},
     {4, 512, 0},
     {5, 512, 128},
     {6, 512, 256},
     {7, 512, 384}},
};

void back_draw(Float z)
{
    njSetTexture(&ava_back_texlist);
    screen_quad_color(0xffffffff);
    screen_parts_draw((struct screen_parts *)&back_parts, 0.0F, 0.0F, z);
}
