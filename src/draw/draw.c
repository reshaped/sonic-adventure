/* the constant material, lines, and the highlight on what draws near the
 * players */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "player/player.h"
#include "system/task.h"

void draw_set_material(Float alpha, Float red, Float green, Float blue)
{
    NJS_ARGB color;

    color.a = alpha;
    color.r = red;
    color.g = green;
    color.b = blue;
    njSetConstantMaterial(&color);
}

void draw_line(Float x1, Float y1, Float z1, Float x2, Float y2, Float z2)
{
    NJS_POINT3COL line;
    NJS_POINT3 points[2];
    NJS_COLOR colors[2];

    points[0].x = x1;
    points[0].y = y1;
    points[0].z = z1;
    points[1].x = x2;
    points[1].y = y2;
    points[1].z = z2;
    colors[0].argb.a = 255;
    colors[0].argb.r = 255;
    colors[0].argb.g = 255;
    colors[0].argb.b = 255;
    colors[1].argb.a = 255;
    colors[1].argb.r = 136;
    colors[1].argb.g = 136;
    colors[1].argb.b = 187;
    line.p = points;
    line.col = colors;
    line.tex = NULL;
    line.num = 2;
    njDrawLine3D(&line, 1, NJD_DRAW_NORMAL);
}

void draw_line_colored(Float x1, Float y1, Float z1, const NJS_COLOR *color1,
                       Float x2, Float y2, Float z2, const NJS_COLOR *color2)
{
    NJS_POINT3COL line;
    NJS_POINT3 points[2];
    NJS_COLOR colors[2];

    points[0].x = x1;
    points[0].y = y1;
    points[0].z = z1;
    points[1].x = x2;
    points[1].y = y2;
    points[1].z = z2;
    colors[0] = *color1;
    colors[1] = *color2;
    line.p = points;
    line.col = colors;
    line.tex = NULL;
    line.num = 2;
    njDrawLine3D(&line, 1, NJD_DRAW_NORMAL);
}

void draw_line_v(NJS_POINT3 *from, NJS_POINT3 *to)
{
    NJS_POINT3COL line;
    NJS_POINT3 points[2];
    NJS_COLOR colors[2];

    points[0].x = from->x;
    points[0].y = from->y;
    points[0].z = from->z;
    points[1].x = to->x;
    points[1].y = to->y;
    points[1].z = to->z;
    colors[0].argb.a = 255;
    colors[0].argb.r = 255;
    colors[0].argb.g = 255;
    colors[0].argb.b = 255;
    colors[1].argb.a = 255;
    colors[1].argb.r = 136;
    colors[1].argb.g = 136;
    colors[1].argb.b = 187;
    line.p = points;
    line.col = colors;
    line.tex = NULL;
    line.num = 2;
    njDrawLine3D(&line, 1, NJD_DRAW_NORMAL);
}

void draw_highlight_near(struct task_work *work, Sint16 characters)
{
    NJS_ARGB color;
    Sint16 bit = 1 << player_get_character(0);

    if (characters & bit) {
        if (player_in_range(&work->pos, 20.0F)) {
            color.a = 0.0F;
            color.r = 0.5F;
            color.g = 0.5F;
            color.b = 0.5F;
        } else {
            color.a = 0.0F;
            color.r = 0.0F;
            color.g = 0.0F;
            color.b = 0.0F;
        }
        njSetConstantMaterial(&color);
    }
}
