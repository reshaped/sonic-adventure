/* the zones a player goes down in, in the Mystic Ruins: seven, each a model of
 * a few polygons. act 0 has four of them, and acts 1 to 3 one each */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/death_zones.h"
#include "player/player.h"

static NJS_MATERIAL death_zone_0_materials[1] = {
#include "adv02/death_zones/death_zone_0_materials.inc"
};
static Sint16 death_zone_0_meshes[5] = {
#include "adv02/death_zones/death_zone_0_meshes.inc"
};
static NJS_MESHSET death_zone_0_meshsets[1] = {
#include "adv02/death_zones/death_zone_0_meshsets.inc"
};
static NJS_POINT3 death_zone_0_points[4] = {
#include "adv02/death_zones/death_zone_0_points.inc"
};
static NJS_VECTOR death_zone_0_normals[4] = {
#include "adv02/death_zones/death_zone_0_normals.inc"
};
static NJS_MODEL death_zone_0_model = {
#include "adv02/death_zones/death_zone_0_model.inc"
};
static NJS_OBJECT death_zone_0_object = {
#include "adv02/death_zones/death_zone_0_object.inc"
};
static NJS_MATERIAL death_zone_1_materials[1] = {
#include "adv02/death_zones/death_zone_1_materials.inc"
};
static Sint16 death_zone_1_meshes[13] = {
#include "adv02/death_zones/death_zone_1_meshes.inc"
};
static NJS_MESHSET death_zone_1_meshsets[1] = {
#include "adv02/death_zones/death_zone_1_meshsets.inc"
};
static NJS_POINT3 death_zone_1_points[10] = {
#include "adv02/death_zones/death_zone_1_points.inc"
};
static NJS_VECTOR death_zone_1_normals[10] = {
#include "adv02/death_zones/death_zone_1_normals.inc"
};
static NJS_MODEL death_zone_1_model = {
#include "adv02/death_zones/death_zone_1_model.inc"
};
static NJS_OBJECT death_zone_1_object = {
#include "adv02/death_zones/death_zone_1_object.inc"
};
static NJS_MATERIAL death_zone_2_materials[1] = {
#include "adv02/death_zones/death_zone_2_materials.inc"
};
static Sint16 death_zone_2_meshes[5] = {
#include "adv02/death_zones/death_zone_2_meshes.inc"
};
static NJS_MESHSET death_zone_2_meshsets[1] = {
#include "adv02/death_zones/death_zone_2_meshsets.inc"
};
static NJS_POINT3 death_zone_2_points[4] = {
#include "adv02/death_zones/death_zone_2_points.inc"
};
static NJS_VECTOR death_zone_2_normals[4] = {
#include "adv02/death_zones/death_zone_2_normals.inc"
};
static NJS_MODEL death_zone_2_model = {
#include "adv02/death_zones/death_zone_2_model.inc"
};
static NJS_OBJECT death_zone_2_object = {
#include "adv02/death_zones/death_zone_2_object.inc"
};
static NJS_MATERIAL death_zone_3_materials[1] = {
#include "adv02/death_zones/death_zone_3_materials.inc"
};
static Sint16 death_zone_3_meshes[4] = {
#include "adv02/death_zones/death_zone_3_meshes.inc"
};
static NJS_MESHSET death_zone_3_meshsets[1] = {
#include "adv02/death_zones/death_zone_3_meshsets.inc"
};
static NJS_POINT3 death_zone_3_points[3] = {
#include "adv02/death_zones/death_zone_3_points.inc"
};
static NJS_VECTOR death_zone_3_normals[3] = {
#include "adv02/death_zones/death_zone_3_normals.inc"
};
static NJS_MODEL death_zone_3_model = {
#include "adv02/death_zones/death_zone_3_model.inc"
};
static NJS_OBJECT death_zone_3_object = {
#include "adv02/death_zones/death_zone_3_object.inc"
};
static NJS_MATERIAL death_zone_4_materials[1] = {
#include "adv02/death_zones/death_zone_4_materials.inc"
};
static Sint16 death_zone_4_meshes[11] = {
#include "adv02/death_zones/death_zone_4_meshes.inc"
};
static NJS_MESHSET death_zone_4_meshsets[1] = {
#include "adv02/death_zones/death_zone_4_meshsets.inc"
};
static NJS_POINT3 death_zone_4_points[7] = {
#include "adv02/death_zones/death_zone_4_points.inc"
};
static NJS_VECTOR death_zone_4_normals[7] = {
#include "adv02/death_zones/death_zone_4_normals.inc"
};
static NJS_MODEL death_zone_4_model = {
#include "adv02/death_zones/death_zone_4_model.inc"
};
static NJS_OBJECT death_zone_4_object = {
#include "adv02/death_zones/death_zone_4_object.inc"
};
static NJS_MATERIAL death_zone_5_materials[1] = {
#include "adv02/death_zones/death_zone_5_materials.inc"
};
static Sint16 death_zone_5_meshes[5] = {
#include "adv02/death_zones/death_zone_5_meshes.inc"
};
static NJS_MESHSET death_zone_5_meshsets[1] = {
#include "adv02/death_zones/death_zone_5_meshsets.inc"
};
static NJS_POINT3 death_zone_5_points[4] = {
#include "adv02/death_zones/death_zone_5_points.inc"
};
static NJS_VECTOR death_zone_5_normals[4] = {
#include "adv02/death_zones/death_zone_5_normals.inc"
};
static NJS_MODEL death_zone_5_model = {
#include "adv02/death_zones/death_zone_5_model.inc"
};
static NJS_OBJECT death_zone_5_object = {
#include "adv02/death_zones/death_zone_5_object.inc"
};
static NJS_MATERIAL death_zone_6_materials[1] = {
#include "adv02/death_zones/death_zone_6_materials.inc"
};
static Sint16 death_zone_6_meshes[5] = {
#include "adv02/death_zones/death_zone_6_meshes.inc"
};
static NJS_MESHSET death_zone_6_meshsets[1] = {
#include "adv02/death_zones/death_zone_6_meshsets.inc"
};
static NJS_POINT3 death_zone_6_points[4] = {
#include "adv02/death_zones/death_zone_6_points.inc"
};
static NJS_VECTOR death_zone_6_normals[4] = {
#include "adv02/death_zones/death_zone_6_normals.inc"
};
static NJS_MODEL death_zone_6_model = {
#include "adv02/death_zones/death_zone_6_model.inc"
};
static NJS_OBJECT death_zone_6_object = {
#include "adv02/death_zones/death_zone_6_object.inc"
};
/* each act's zones, ended by a null object */
static struct death_zone act0_death_zones[5] = {
#include "adv02/death_zones/act0_death_zones.inc"
};
static struct death_zone act1_death_zones[2] = {
#include "adv02/death_zones/act1_death_zones.inc"
};
static struct death_zone act2_death_zones[2] = {
#include "adv02/death_zones/act2_death_zones.inc"
};
static struct death_zone act3_death_zones[2] = {
#include "adv02/death_zones/act3_death_zones.inc"
};
/* a list for each act, 0 to 3. death_zone_step and knuckles_outside_zones
 * take the current act's */
struct death_zone *mystic_ruins_death_zones[4] = {
    act0_death_zones,
    act1_death_zones,
    act2_death_zones,
    act3_death_zones,
};
