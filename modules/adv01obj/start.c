/* where the player starts on the Egg Carrier, outside or inside, by act
 * and entrance */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv01obj/start.h"
#include "stage/stage.h"
#include "system/task.h"

struct egg_carrier_start {
    NJS_POINT3 pos;
    Angle ang_y;
};

/* each outside act's starts, by entrance */
static struct egg_carrier_start outside_act0_starts[8] = {
#include "adv01obj/start/outside_act0_starts.inc"
};
static struct egg_carrier_start outside_act1_starts[] = {
    {{-270.0F, 570.0F, -400.0F}, 0},
    {{0.0F, 748.0F, 402.0F}, 0},
    {{356.0F, 569.0F, -422.0F}, 0xc000},
};
static struct egg_carrier_start outside_act2_starts[6] = {
#include "adv01obj/start/outside_act2_starts.inc"
};
static struct egg_carrier_start outside_act3_starts[] = {
    {{12.0F, 98.0F, -20.0F}, 0x8000},
    {{-67.0F, 98.0F, 0.0F}, 0x8000},
};
static struct egg_carrier_start outside_act4_starts[] = {
    {{-39.0F, 0.0F, -50.0F}, 0},
    {{181.0F, 0.0F, 111.0F}, 0x4000},
};
static struct egg_carrier_start outside_act5_starts[] = {
    {{25.0F, 10.0F, 25.0F}, 0},
    {{-18.0F, -66.0F, -12.0F}, 0},
    {{302.0F, 10.0F, 4.0F}, 0x8000},
};
static struct egg_carrier_start outside_act6_starts[] = {
    {{0.0F, 0.0F, 0.0F}, 0},
};

/* each inside act's starts, by entrance */
static struct egg_carrier_start inside_act0_starts[] = {
    {{-88.0F, 108.0F, 0.0F}, 0},
};
static struct egg_carrier_start inside_act1_starts[10] = {
#include "adv01obj/start/inside_act1_starts.inc"
};
static struct egg_carrier_start inside_act2_starts[] = {
    {{-65.0F, 0.0F, 0.0F}, 0},
    {{65.0F, 0.0F, 0.0F}, 0x8000},
};
static struct egg_carrier_start inside_act3_starts[] = {
    {{116.0F, 0.0F, 60.0F}, 0x8000},
    {{-100.0F, 0.0F, -10.0F}, 0},
};
static struct egg_carrier_start inside_act4_starts[] = {
    {{75.0F, 56.0F, 0.0F}, 0x8000},
};
static struct egg_carrier_start inside_act5_starts[] = {
    {{0.0F, 0.0F, 45.0F}, 0xc000},
    {{0.0F, 5.0F, 22.0F}, 0x4000},
};

static struct egg_carrier_start *outside_starts[STAGE_ACTS] = {
    outside_act0_starts, outside_act1_starts, outside_act2_starts,
    outside_act3_starts, outside_act4_starts, outside_act5_starts,
    outside_act6_starts,
};
static struct egg_carrier_start *inside_starts[STAGE_ACTS] = {
    inside_act0_starts, inside_act1_starts, inside_act2_starts,
    inside_act3_starts, inside_act4_starts, inside_act5_starts,
};

void egg_carrier_place_player(struct task_work *work)
{
    int stage = STAGE_OF(stage_get_stage_act());
    int act = (Uint8)stage_get_stage_act();
    struct egg_carrier_start *start;

    if (stage == STAGE_EGG_CARRIER) {
        start = &outside_starts[act][stage_get_entrance()];
    } else {
        start = &inside_starts[act][stage_get_entrance()];
    }
    work->pos = start->pos;
    work->ang.x = 0;
    work->ang.y = start->ang_y;
    work->ang.z = 0;
}
