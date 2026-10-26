/* player start positions in the past, by act and entrance */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv03/start.h"
#include "stage/stage.h"
#include "system/task.h"

struct past_start {
    NJS_POINT3 pos;
    Angle ang_y;
};

/* each act's starts, by entrance */
static struct past_start act0_starts[] = {
    {{0.0F, 10.0F, 1400.0F}, 0xc000},
    {{0.0F, 100.0F, -800.0F}, 0},
};
static struct past_start act1_starts[] = {
    {{219.0F, -30.0F, 1777.0F}, 0xc000},
    {{0.0F, -71.0F, 820.0F}, 0xc000},
};
/* the starts by act. act 2 starts where act 1 does */
static struct past_start *past_starts[STAGE_ACTS] = {
    act0_starts,
    act1_starts,
    act1_starts,
};

/* under section HEAD, SHC writes the function to section PHEAD, after
 * past.c's two steps. 1ST_READ.BIN's stage_place_player calls it by its
 * address there, 0x0c900160 */
#pragma section HEAD
void past_place_player(struct task_work *work)
{
    struct past_start *start =
        &past_starts[(Uint8)stage_get_stage_act()][stage_get_entrance()];

    work->pos = start->pos;
    work->ang.x = 0;
    work->ang.y = start->ang_y;
    work->ang.z = 0;
}
#pragma section
