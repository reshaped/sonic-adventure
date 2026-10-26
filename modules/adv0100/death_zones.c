/* the zones in which a player dies on the Egg Carrier's outside. each of acts
 * 0 to 5 has one zone, a model of one quad */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/death_zones.h"
#include "player/player.h"

static NJS_MATERIAL act0_death_zone_materials[1] = {
#include "adv0100/death_zones/act0_death_zone_materials.inc"
};
static Sint16 act0_death_zone_meshes[5] = {
#include "adv0100/death_zones/act0_death_zone_meshes.inc"
};
static NJS_MESHSET act0_death_zone_meshsets[1] = {
#include "adv0100/death_zones/act0_death_zone_meshsets.inc"
};
static NJS_POINT3 act0_death_zone_points[4] = {
#include "adv0100/death_zones/act0_death_zone_points.inc"
};
static NJS_VECTOR act0_death_zone_normals[4] = {
#include "adv0100/death_zones/act0_death_zone_normals.inc"
};
static NJS_MODEL act0_death_zone_model = {
#include "adv0100/death_zones/act0_death_zone_model.inc"
};
static NJS_OBJECT act0_death_zone_object = {
#include "adv0100/death_zones/act0_death_zone_object.inc"
};
static NJS_MATERIAL act1_death_zone_materials[1] = {
#include "adv0100/death_zones/act1_death_zone_materials.inc"
};
static Sint16 act1_death_zone_meshes[5] = {
#include "adv0100/death_zones/act1_death_zone_meshes.inc"
};
static NJS_MESHSET act1_death_zone_meshsets[1] = {
#include "adv0100/death_zones/act1_death_zone_meshsets.inc"
};
static NJS_POINT3 act1_death_zone_points[4] = {
#include "adv0100/death_zones/act1_death_zone_points.inc"
};
static NJS_VECTOR act1_death_zone_normals[4] = {
#include "adv0100/death_zones/act1_death_zone_normals.inc"
};
static NJS_MODEL act1_death_zone_model = {
#include "adv0100/death_zones/act1_death_zone_model.inc"
};
static NJS_OBJECT act1_death_zone_object = {
#include "adv0100/death_zones/act1_death_zone_object.inc"
};
static NJS_MATERIAL act2_death_zone_materials[1] = {
#include "adv0100/death_zones/act2_death_zone_materials.inc"
};
static Sint16 act2_death_zone_meshes[5] = {
#include "adv0100/death_zones/act2_death_zone_meshes.inc"
};
static NJS_MESHSET act2_death_zone_meshsets[1] = {
#include "adv0100/death_zones/act2_death_zone_meshsets.inc"
};
static NJS_POINT3 act2_death_zone_points[4] = {
#include "adv0100/death_zones/act2_death_zone_points.inc"
};
static NJS_VECTOR act2_death_zone_normals[4] = {
#include "adv0100/death_zones/act2_death_zone_normals.inc"
};
static NJS_MODEL act2_death_zone_model = {
#include "adv0100/death_zones/act2_death_zone_model.inc"
};
static NJS_OBJECT act2_death_zone_object = {
#include "adv0100/death_zones/act2_death_zone_object.inc"
};
static NJS_MATERIAL act3_death_zone_materials[1] = {
#include "adv0100/death_zones/act3_death_zone_materials.inc"
};
static Sint16 act3_death_zone_meshes[5] = {
#include "adv0100/death_zones/act3_death_zone_meshes.inc"
};
static NJS_MESHSET act3_death_zone_meshsets[1] = {
#include "adv0100/death_zones/act3_death_zone_meshsets.inc"
};
static NJS_POINT3 act3_death_zone_points[4] = {
#include "adv0100/death_zones/act3_death_zone_points.inc"
};
static NJS_VECTOR act3_death_zone_normals[4] = {
#include "adv0100/death_zones/act3_death_zone_normals.inc"
};
static NJS_MODEL act3_death_zone_model = {
#include "adv0100/death_zones/act3_death_zone_model.inc"
};
static NJS_OBJECT act3_death_zone_object = {
#include "adv0100/death_zones/act3_death_zone_object.inc"
};
static NJS_MATERIAL act4_death_zone_materials[1] = {
#include "adv0100/death_zones/act4_death_zone_materials.inc"
};
static Sint16 act4_death_zone_meshes[5] = {
#include "adv0100/death_zones/act4_death_zone_meshes.inc"
};
static NJS_MESHSET act4_death_zone_meshsets[1] = {
#include "adv0100/death_zones/act4_death_zone_meshsets.inc"
};
static NJS_POINT3 act4_death_zone_points[4] = {
#include "adv0100/death_zones/act4_death_zone_points.inc"
};
static NJS_VECTOR act4_death_zone_normals[4] = {
#include "adv0100/death_zones/act4_death_zone_normals.inc"
};
static NJS_MODEL act4_death_zone_model = {
#include "adv0100/death_zones/act4_death_zone_model.inc"
};
static NJS_OBJECT act4_death_zone_object = {
#include "adv0100/death_zones/act4_death_zone_object.inc"
};
static NJS_MATERIAL act5_death_zone_materials[1] = {
#include "adv0100/death_zones/act5_death_zone_materials.inc"
};
static Sint16 act5_death_zone_meshes[5] = {
#include "adv0100/death_zones/act5_death_zone_meshes.inc"
};
static NJS_MESHSET act5_death_zone_meshsets[1] = {
#include "adv0100/death_zones/act5_death_zone_meshsets.inc"
};
static NJS_POINT3 act5_death_zone_points[4] = {
#include "adv0100/death_zones/act5_death_zone_points.inc"
};
static NJS_VECTOR act5_death_zone_normals[4] = {
#include "adv0100/death_zones/act5_death_zone_normals.inc"
};
static NJS_MODEL act5_death_zone_model = {
#include "adv0100/death_zones/act5_death_zone_model.inc"
};
static NJS_OBJECT act5_death_zone_object = {
#include "adv0100/death_zones/act5_death_zone_object.inc"
};
/* each act's zones, ended by one with a null object. act6_death_zones holds
 * only that end */
static struct death_zone act0_death_zones[2] = {
#include "adv0100/death_zones/act0_death_zones.inc"
};
static struct death_zone act1_death_zones[2] = {
#include "adv0100/death_zones/act1_death_zones.inc"
};
static struct death_zone act2_death_zones[2] = {
#include "adv0100/death_zones/act2_death_zones.inc"
};
static struct death_zone act3_death_zones[2] = {
#include "adv0100/death_zones/act3_death_zones.inc"
};
static struct death_zone act4_death_zones[2] = {
#include "adv0100/death_zones/act4_death_zones.inc"
};
static struct death_zone act5_death_zones[2] = {
#include "adv0100/death_zones/act5_death_zones.inc"
};
static struct death_zone act6_death_zones[1] = {
#include "adv0100/death_zones/act6_death_zones.inc"
};
struct death_zone *outside_death_zones[7] = {
    act0_death_zones, act1_death_zones, act2_death_zones, act3_death_zones,
    act4_death_zones, act5_death_zones, act6_death_zones,
};
