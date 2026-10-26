/* the zones in which a player dies in the garden. its one act has one zone, a
 * model of one quad */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_garden01/death_zones.h"
#include "player/player.h"

static NJS_MATERIAL death_zone_materials[1] = {
#include "al_garden01/death_zones/death_zone_materials.inc"
};
static Sint16 death_zone_meshes[5] = {
#include "al_garden01/death_zones/death_zone_meshes.inc"
};
static NJS_MESHSET death_zone_meshsets[1] = {
#include "al_garden01/death_zones/death_zone_meshsets.inc"
};
static NJS_POINT3 death_zone_points[4] = {
#include "al_garden01/death_zones/death_zone_points.inc"
};
static NJS_VECTOR death_zone_normals[4] = {
#include "al_garden01/death_zones/death_zone_normals.inc"
};
static NJS_MODEL death_zone_model = {
#include "al_garden01/death_zones/death_zone_model.inc"
};
static NJS_OBJECT death_zone_object = {
#include "al_garden01/death_zones/death_zone_object.inc"
};
/* the act's zone, then one with a null object to end the list */
static struct death_zone death_zone_list[2] = {
#include "al_garden01/death_zones/death_zone_list.inc"
};
struct death_zone *egg_carrier_garden_death_zones[1] = {death_zone_list};
