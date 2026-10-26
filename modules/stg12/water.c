/* act 0's water: its level, and the start of its tasks */
#include <sg_xpt.h>

#include "stg12/dam.h"
#include "stg12/deep_water.h"
#include "stg12/floats.h"
#include "stg12/lower_water.h"
#include "stg12/upper_pool.h"
#include "stg12/water.h"
#include "system/task.h"

#define LOW_LEVEL (-80.0F)

enum shelter_water_state shelter_water_state;
Float shelter_water_level;

void shelter_water_start(void)
{
    shelter_water_level = LOW_LEVEL;
    shelter_water_state = SHELTER_WATER_LOW;
    task_create(TASK_WORK | TASK_FORCE, 3, shelter_lower_water_start);
    task_create(TASK_WORK, 3, shelter_dam_start);
    task_create(TASK_WORK, 3, shelter_upper_pool_start);
    task_create(TASK_WORK | TASK_FORCE, 3, shelter_deep_water_start);
    shelter_floats_start();
}
