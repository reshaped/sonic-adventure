/* points lit as Ninja's drawers light them, and the corners of sa_triangles'
 * forms */
#ifndef ENGINE_PC_NINJA_H
#define ENGINE_PC_NINJA_H

#include <stdint.h>

#include "sa_engine.h"

namespace pc {

/* the intensity a point's dot with the light gives under shade's
 * SA_SHADE_RAMP or SA_SHADE_LINEAR. Ninja's light points the way it shines,
 * so a lit face's dot is below 0 */
float ninja_intensity(const sa_shade &shade, float dot);
/* argb with its red, green and blue times intensity, as the PowerVR's
 * intensity vertices take it. intensity is held to 0 to 1 */
uint32_t ninja_scaled(uint32_t argb, float intensity);

/* whether a backend lights a light of type as Ninja's full drawer does */
bool ninja_light_known(uint32_t type);
/* the base and offset ARGB colors Ninja's full drawer gives material under
 * lights. the point is at p with normal n, both in world space. each light
 * adds its share in queue order, and then the material's alpha adds. each
 * channel is held to 0 to 1. a textured material takes the highlight as its
 * offset. another adds it to its base and takes an offset of 0 */
void ninja_lit(const sa_lights &lights, const sa_material &material,
               const float p[3], const float n[3], uint32_t &base,
               uint32_t &offset);

/* triangle i's three points, for a form of triangles */
void triangle_corners(uint8_t form, uint32_t i, uint32_t corners[3]);

} // namespace pc

#endif
