/* 52 bytes that no word in the image or in 1ST_READ.BIN points at. they lay
 * out as a collision shape, as orca.c's orca_shapes just before them do.
 * the bytes don't prove the type */
#include "collision/collision.h"

static struct collision_shape dat_0cab0cc8[1] = {
#include "stg01/shape/dat_0cab0cc8.inc"
};
