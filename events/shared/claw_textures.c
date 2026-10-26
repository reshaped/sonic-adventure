/* the claw's texlist and the names of its seven textures. the materials and
 * strips are ones that each event's generated data points at */
#include <ninja.h>
#include <sg_xpt.h>

#include "shared/claw_textures.h"

#define TEXT_1_LENGTH 3
#define TEXT_2_LENGTH 5
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 7
#define TEXT_5_LENGTH 4
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 4
#include "shared/claw_textures/text.inc"

static NJS_TEXNAME event_claw_textures[7] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0}, {TEXT_4, 0, 0},
    {TEXT_5, 0, 0}, {TEXT_6, 0, 0}, {TEXT_7, 0, 0},
};
NJS_TEXLIST event_claw_texlist = {
#include "shared/claw_textures/event_claw_texlist.inc"
};
static NJS_MATERIAL event_claw_materials[4] = {
#include "shared/claw_textures/event_claw_materials.inc"
};
static Sint16 event_claw_strips_0[20] = {
#include "shared/claw_textures/event_claw_strips_0.inc"
};
static Sint16 event_claw_strips_1[23] = {
#include "shared/claw_textures/event_claw_strips_1.inc"
};
static Sint16 event_claw_strips_2[31] = {
#include "shared/claw_textures/event_claw_strips_2.inc"
};
