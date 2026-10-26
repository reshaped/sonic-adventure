/* the jungle's fog. its color follows the time of day, and its table, from the
 * act's fog file, thins as the camera climbs or nears the pyramid */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/fog.h"
#include "camera/camera.h"
#include "debug/fog_editor.h"
#include "progress/progress.h"
#include "system/loader.h"
#include "system/task.h"

/* FOG_MR02.bin, which loader_read_stage_files reads into CAMERA_AREA_5. it
 * holds FOG_TABLES tables */
#define FOG_FILE CAMERA_AREA_5
#define FOG_TABLES 4

/* the fog thins over FADE as the camera climbs from a height of LOW. it also
 * thins as the camera comes from PYRAMID_FAR to PYRAMID_NEAR of the pyramid,
 * measured in x and z */
#define FADE 100.0F
#define LOW 120.0F
#define PYRAMID_X (-521.0F)
#define PYRAMID_Z (-1172.0F)
#define PYRAMID_NEAR 500.0F
#define PYRAMID_FAR 600.0F

/* the fog's density, which the fog editor edits in place */
static Float jungle_fog_density = -2400.0F;
/* the fog's color by time of day, TIME_DAY to TIME_NIGHT */
static Uint32 jungle_fog_colors[3] = {
#include "adv02/fog/jungle_fog_colors.inc"
};
/* the color jungle_fog_update chose last, or null before it chooses one */
static Uint32 *jungle_fog_color = NULL;

NJS_FOG_TABLE jungle_fog_table;
Uint32 jungle_fog_shown;

void jungle_fog_init(void)
{
    fog_editor_reset();
    fog_editor_set_tables(FOG_FILE, FOG_TABLES);
    fog_editor_set_color(jungle_fog_color);
    fog_editor_set_density(&jungle_fog_density);
    njSetFogDensity(njGenerateFogDensity(jungle_fog_density));
    njFogEnable();
}

/* the fog table's scale is a height term plus a pyramid term, clamped to 0 to
 * 1. the height term is 1 at or below LOW. above LOW it falls from 0 to -1
 * over FADE, then stays -1. the pyramid term is -1 within PYRAMID_NEAR of the
 * pyramid, in x and z. past PYRAMID_NEAR it rises by 1 over each FADE, and
 * beyond PYRAMID_FAR it is 1 */
void jungle_fog_update(int time)
{
    Float x;
    Float y;
    Float z;
    Float dy;
    Float dx;
    Float dz;
    Float distance;
    Float scale;

    switch (time) {
    case TIME_DAY:
        jungle_fog_color = &jungle_fog_colors[TIME_DAY];
        break;
    case TIME_EVENING:
        jungle_fog_color = &jungle_fog_colors[TIME_EVENING];
        break;
    case TIME_NIGHT:
        jungle_fog_color = &jungle_fog_colors[TIME_NIGHT];
        break;
    }
    fog_editor_set_color(jungle_fog_color);
    x = camera_work->pos.x;
    y = camera_work->pos.y;
    z = camera_work->pos.z;
    scale = 0.0F;
    dy = y - LOW;
    if (dy > FADE) {
        scale = -1.0F;
    } else if (dy > 0.0F) {
        scale -= dy * (1.0F / FADE);
    } else {
        scale = 1.0F;
    }
    dx = x - PYRAMID_X;
    dz = z - PYRAMID_Z;
    distance = njSqrt((dx * dx) + (dz * dz));
    if (distance < PYRAMID_NEAR) {
        scale -= 1.0F;
    } else if (distance > PYRAMID_FAR) {
        scale += 1.0F;
    } else {
        scale -= 1.0F - ((distance - PYRAMID_NEAR) * (1.0F / FADE));
    }
    if (scale < 0.0F) {
        scale = 0.0F;
    } else if (scale > 1.0F) {
        scale = 1.0F;
    }
    fog_editor_make_table(jungle_fog_table, 0, scale);
    jungle_fog_shown = *jungle_fog_color;
}
