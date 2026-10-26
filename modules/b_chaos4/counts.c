/* the counts of Chaos 4's columns standing and wakes spreading */
#include <sg_xpt.h>

#include "b_chaos4/column.h"

static Uint8 columns_standing;
static Uint8 wakes_spreading;

void effect_counts_clear(void)
{
    wakes_spreading = 0;
    columns_standing = 0;
}

void column_count_add(void)
{
    columns_standing++;
}

void column_count_remove(void)
{
    columns_standing--;
}

int column_count(void)
{
    return columns_standing;
}

void wake_count_add(void)
{
    wakes_spreading++;
}

void wake_count_remove(void)
{
    wakes_spreading--;
}

int wake_count(void)
{
    return wakes_spreading;
}
