/* player start positions in Station Square, by act and entrance */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv00/start.h"
#include "stage/stage.h"
#include "system/task.h"

struct station_square_start {
    NJS_POINT3 pos;
    Angle ang_y;
};

/* each act's starts, by entrance */
static struct station_square_start office_starts[] = {
    {{270.0F, 4.0F, 277.0F}, 0x4000},
    {{510.0F, 0.0F, 880.0F}, 0x8000},
    {{475.0F, 0.0F, 636.0F}, -0x8000},
};
static struct station_square_start casino_starts[] = {
    {{15.0F, 66.0F, 1365.0F}, -0x52ea},
    {{-610.0F, 0.0F, 914.0F}, 0x2000},
    {{-444.0F, 0.0F, 1510.0F}, -0x2000},
    {{-109.0F, 140.0F, 1196.0F}, -0x8000},
};
static struct station_square_start gesui_starts[] = {
    {{397.0F, -55.0F, 994.0F}, 0x4000},
};
static struct station_square_start station_starts[7] = {
#include "adv00/start/station_starts.inc"
};
static struct station_square_start hotel_starts[6] = {
#include "adv00/start/hotel_starts.inc"
};
static struct station_square_start twinkle_starts[] = {
    {{668.6F, 50.0F, 1772.0F}, 0},
    {{794.0F, 50.0F, 1772.0F}, -0x8000},
    {{757.0F, 50.0F, 1726.0F}, 0x4000},
};
static struct station_square_start *station_square_starts[] = {
    office_starts,  casino_starts, gesui_starts,
    station_starts, hotel_starts,  twinkle_starts,
};

/* under section HEAD, SHC writes the function to section PHEAD, which the
 * module's link places first. 1ST_READ.BIN's stage_place_player calls it by its
 * address there */
#pragma section HEAD
void station_square_place_player(struct task_work *work)
{
    struct station_square_start *start =
        &station_square_starts[(Uint8)stage_get_stage_act()]
                              [stage_get_entrance()];

    work->pos = start->pos;
    work->ang.x = 0;
    work->ang.y = start->ang_y;
    work->ang.z = 0;
}
#pragma section
