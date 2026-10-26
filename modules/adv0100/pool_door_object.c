/* the pool door's root object, then a list of one texture and the texlist
 * that names it. nothing names the object or the texlist */
#include <ninja.h>

#define TEXT_1_LENGTH 10
#include "adv0100/pool_door_object/text.inc"

/* the object's model, generated. pool_door.c draws it */
extern NJS_MODEL pool_door_model;

static NJS_OBJECT dat_0cace6d0 = {
#include "adv0100/pool_door_object/dat_0cace6d0.inc"
};
static NJS_TEXNAME dat_0cace704[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST dat_0cace710 = {
#include "adv0100/pool_door_object/dat_0cace710.inc"
};
