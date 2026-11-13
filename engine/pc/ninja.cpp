#include "ninja.h"

#include <math.h>

#include <algorithm>
#include <iterator>

namespace pc {

float ninja_intensity(const sa_shade &shade, float dot)
{
    float product = -shade.diffuse * dot;

    if (shade.mode == SA_SHADE_RAMP && !(product > 0.0F)) {
        product = 0.0F;
    }
    return shade.ambient + product;
}

uint32_t ninja_scaled(uint32_t argb, float intensity)
{
    int held = static_cast<int>(intensity * 255.0F);
    uint32_t scale = static_cast<uint32_t>(held < 0     ? 0
                                           : held > 255 ? 255
                                                        : held);
    uint32_t out = argb & 0xff000000;

    for (uint32_t shift = 0; shift < 24; shift += 8) {
        out |= ((argb >> shift & 0xff) * scale / 255) << shift;
    }
    return out;
}

bool ninja_light_known(uint32_t type)
{
    /* the light types whose njLtSrcs.c calculators ninja_lit follows */
    switch (type) {
    case SA_LIGHT_AMBIENT:
    case SA_LIGHT_DIRECTION:
    case SA_LIGHT_AMBIENT | SA_LIGHT_DIRECTION:
    case SA_LIGHT_DIRECTION_SPECULAR:
    case SA_LIGHT_AMBIENT | SA_LIGHT_DIRECTION | SA_LIGHT_DIRECTION_SPECULAR:
    case SA_LIGHT_POINT:
    case SA_LIGHT_SIMPLE | SA_LIGHT_POINT:
    case SA_LIGHT_AMBIENT | SA_LIGHT_POINT:
    case SA_LIGHT_SIMPLE | SA_LIGHT_AMBIENT | SA_LIGHT_POINT:
        return true;
    default:
        return false;
    }
}

namespace {

/* value to the power Ninja's specular step names, as lt_power raises it */
float ninja_power(float value, uint32_t step)
{
    static const float powers[] = {0.0F,  0.5F,  1.0F,  1.5F,  2.0F,  3.0F,
                                   4.0F,  6.0F,  8.0F,  12.0F, 16.0F, 24.0F,
                                   32.0F, 48.0F, 64.0F, 96.0F, 128.0F};

    return powf(value, powers[std::min<size_t>(step, std::size(powers) - 1)]);
}

uint32_t ninja_pack(float a, float r, float g, float b)
{
    const float in[4] = {a, r, g, b};
    uint32_t out = 0;

    for (float c : in) {
        /* a value that is no number packs as 0 */
        float held = c > 0.0F ? (c < 1.0F ? c : 1.0F) : 0.0F;
        out = out << 8 | static_cast<uint32_t>(held * 255.0F);
    }
    return out;
}

} // namespace

void ninja_lit(const sa_lights &lights, const sa_material &material,
               const float p[3], const float n[3], uint32_t &base,
               uint32_t &offset)
{
    float mat[4];
    float spc[3];

    /* mat is _nj_light_material_'s diffuse, and spc is its specular. Ninja's
     * drawer sets its ambient equal to its diffuse, so mat scales a light's
     * ambient too */
    for (int k = 0; k < 4; k++) {
        mat[k] = static_cast<float>(material.diffuse >> (24 - 8 * k) & 0xff) /
                 255.0F;
    }
    for (int k = 0; k < 3; k++) {
        spc[k] = static_cast<float>(material.specular >> (16 - 8 * k) & 0xff) /
                 255.0F;
    }
    float diffuse[4] = {0.0F, 0.0F, 0.0F, 0.0F};
    float specular[3] = {0.0F, 0.0F, 0.0F};
    uint32_t count = std::min<uint32_t>(lights.count, SA_LIGHTS);

    for (uint32_t i = 0; i < count; i++) {
        const sa_light &l = lights.light[i];
        float dot = n[0] * l.direction[0] + n[1] * l.direction[1] +
                    n[2] * l.direction[2];

        if (l.type & SA_LIGHT_AMBIENT) {
            for (int k = 0; k < 3; k++) {
                diffuse[k + 1] += mat[k + 1] * l.ambient[k];
            }
        }
        if (l.type & SA_LIGHT_POINT) {
            /* as Ninja's lt_rgb_04 lights a point light */
            float d[3] = {l.point[0] - p[0], l.point[1] - p[1],
                          l.point[2] - p[2]};
            float square = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
            float toward = n[0] * d[0] + n[1] * d[1] + n[2] * d[2];

            if (!(l.far > square) || !(toward > 0.0F)) {
                continue;
            }
            float share = toward / sqrtf(square);
            if (square > l.near) {
                share *= l.near / square;
            }
            for (int k = 0; k < 3; k++) {
                diffuse[k + 1] += share * mat[k + 1] * l.diffuse[k];
            }
            continue;
        }
        /* the light shines the way it points, so a lit face's dot is below
         * 0 */
        if (!(l.type & (SA_LIGHT_DIRECTION | SA_LIGHT_DIRECTION_SPECULAR)) ||
            !(dot < 0.0F)) {
            continue;
        }
        for (int k = 0; k < 3; k++) {
            diffuse[k + 1] += -dot * mat[k + 1] * l.diffuse[k];
        }
        float square = dot * dot;

        if (!(l.type & SA_LIGHT_DIRECTION_SPECULAR) || !(square > l.outer)) {
            continue;
        }
        /* short of inner, the share is the square's power times
         * njCalcLightMaterials' rate, 1 over inner's power. where inner's
         * power isn't a normal float, the share is the power of the square
         * over inner */
        float share = 1.0F;
        if (l.inner > square) {
            float edge = ninja_power(l.inner, material.exponent);
            share = isnormal(edge)
                        ? ninja_power(square, material.exponent) / edge
                        : ninja_power(square / l.inner, material.exponent);
        }
        for (int k = 0; k < 3; k++) {
            specular[k] += share * spc[k] * l.specular[k + 1];
        }
        diffuse[0] += share * l.specular[0];
    }
    diffuse[0] += mat[0];
    if (material.flags & SA_MAT_TEXTURE) {
        base = ninja_pack(diffuse[0], diffuse[1], diffuse[2], diffuse[3]);
        offset = ninja_pack(0.0F, specular[0], specular[1], specular[2]);
        return;
    }
    base = ninja_pack(diffuse[0], diffuse[1] + specular[0],
                      diffuse[2] + specular[1], diffuse[3] + specular[2]);
    offset = 0;
}

void triangle_corners(uint8_t form, uint32_t i, uint32_t corners[3])
{
    uint32_t first = form == SA_TRIANGLES_LIST ? 3 * i : i;

    if (form == SA_TRIANGLES_FAN) {
        corners[0] = 0;
        corners[1] = 2 * i + 1;
        corners[2] = 2 * i + 2;
        return;
    }
    corners[0] = first;
    corners[1] = first + 1;
    corners[2] = first + 2;
}

} // namespace pc
