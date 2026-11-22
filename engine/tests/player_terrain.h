#ifndef TESTS_PLAYER_TERRAIN_H
#define TESTS_PLAYER_TERRAIN_H

#include <ninja.h>

struct character_constants;

/* Install this object as static land and rebuild the collision meshes. */
typedef void (*terrain_install)(NJS_OBJECT *object, Uint32 flags);

int terrain_tread_cases(terrain_install install,
                        const struct character_constants *sonic);
int terrain_curb_case(terrain_install install,
                      const struct character_constants *sonic);
int terrain_dash_case(terrain_install install,
                      const struct character_constants *sonic);
int terrain_roll_case(terrain_install install,
                      const struct character_constants *characters);
int terrain_crest_case(terrain_install install,
                       const struct character_constants *knuckles);

#endif
