/* player start positions in the Mystic Ruins, by act and entrance */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/start.h"
#include "stage/stage.h"
#include "system/task.h"

struct mystic_ruins_start {
    NJS_POINT3 pos;
    Angle ang_y;
};

/* each act's starts, by entrance */
static struct mystic_ruins_start mystic_ruins_station_starts[8] = {
#include "adv02/start/mystic_ruins_station_starts.inc"
};
static struct mystic_ruins_start mystic_ruins_island_starts[] = {
    {{-15.0F, 7.0F, -11.0F}, 0x7800},
    {{-1900.0F, -354.0F, 1628.0F}, 0},
    {{-1297.0F, 27.0F, 312.0F}, 0xde00},
};
static struct mystic_ruins_start mystic_ruins_jungle_starts[5] = {
#include "adv02/start/mystic_ruins_jungle_starts.inc"
};
static struct mystic_ruins_start mystic_ruins_base_starts[] = {
    {{0.0F, 108.0F, 143.0F}, 0xc000},
    {{0.0F, 108.0F, -146.0F}, 0x4000},
    {{142.0F, 108.0F, -15.0F}, 0x8000},
    {{0.0F, 0.0F, -146.0F}, 0x4000},
};
static struct mystic_ruins_start *mystic_ruins_starts[STAGE_ACTS] = {
    mystic_ruins_station_starts,
    mystic_ruins_island_starts,
    mystic_ruins_jungle_starts,
    mystic_ruins_base_starts,
};

void mystic_ruins_place_player(struct task_work *work)
{
    struct mystic_ruins_start *start =
        &mystic_ruins_starts[(Uint8)stage_get_stage_act()]
                            [stage_get_entrance()];

    work->pos = start->pos;
    work->ang.x = 0;
    work->ang.y = start->ang_y;
    work->ang.z = 0;
}
