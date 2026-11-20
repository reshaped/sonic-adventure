/* a ray-traced picture's input. the scene fills a RayPicture from a picture's
 * mesh draws instead of shading their points on the host: each draw becomes an
 * instance of a mesh the raster holds, with the materials it was drawn under.
 * the raster traces the instances, and rasterizes the picture's sprites,
 * triangles and screen draws over the result as batches. ENGINE.md#ray-tracing
 * describes the model. nothing here names a Vulkan type */
#ifndef ENGINE_PC_RAYS_H
#define ENGINE_PC_RAYS_H

#include <math.h>
#include <stdint.h>
#include <string.h>

#include <algorithm>
#include <vector>

namespace pc {

/* a mesh's triangles, for the device to build an acceleration structure from.
 * corners, uv and materials are flattened from the mesh's meshsets. points and
 * normals are read only during add_mesh */
struct RayMesh {
    const float *points = nullptr; /* three floats a point, in mesh space */
    const float *normals = nullptr; /* the same, or null for a mesh with none */
    uint32_t count = 0; /* points */
    std::vector<uint32_t> corners; /* three point indices a triangle */
    std::vector<float> uv; /* two floats a corner */
    /* a triangle's material, as an index into its draw's materials */
    std::vector<uint16_t> materials;
};

/* an instance's flags */
/* the picture drew it, so a camera ray can hit it. an instance without this
 * is outside the game's view, and only shadow, bounce and reflection rays hit
 * it */
constexpr uint32_t RAY_SEEN = 0x1;
/* it is a backdrop, such as a sky. it is shown unlit, as the game drew it, and
 * only camera and reflection rays hit it: it casts no shadow and lights
 * nothing */
constexpr uint32_t RAY_BACKDROP = 0x2;
constexpr uint32_t RAY_NORMALS = 0x4; /* its mesh has normals */
/* none of its materials blends or is emissive, so a ray's hit on it needs no
 * test in the shader */
constexpr uint32_t RAY_OPAQUE = 0x8;
/* its position in the picture before is unknown, so it has no motion vector and
 * no history */
constexpr uint32_t RAY_NEW = 0x10;
constexpr uint32_t RAY_FOG = 0x20; /* the picture's fog table applies to it */
/* it is a liquid's surface, which reflects */
constexpr uint32_t RAY_LIQUID = 0x40;
/* each of its materials blends, as glass's and water's do. the enclosure's
 * probes pass through it, and it casts no overhead shadow. a shadow ray is
 * still attenuated by its alpha */
constexpr uint32_t RAY_TRANSLUCENT = 0x80;
/* it is a part of the act's scene, or a draw as large as one, as a cave a
 * task draws. an instance without this is an object, such as a character,
 * and casts an overhead shadow */
constexpr uint32_t RAY_SCENERY = 0x100;
/* the game shades it by RayInstance's ambient and diffuse, as sa_shade's
 * SA_SHADE_RAMP or SA_SHADE_LINEAR does, and not from a ramp. with neither
 * flag, its light's ramp shades it */
constexpr uint32_t RAY_SHADE_RAMP = 0x200;
constexpr uint32_t RAY_SHADE_LINEAR = 0x400;

/* a material's flags, in addition to its sa_material's SA_MAT_* */
/* its color scales the texel. without this the texel is used as it is, as under
 * SA1's palettes, where only the light colors a point */
constexpr uint32_t RAY_MAT_COLOR = 0x10000;
/* the game computes no lighting for it, as for a texture with its lighting
 * painted in. its texel is its shade, which only shadows and occlusion
 * darken */
constexpr uint32_t RAY_MAT_PRELIT = 0x20000;
/* an unlit material that adds to what lies behind it. it is shown
 * unshaded, and a bounce ray that hits it takes its color times trace.comp's
 * GLOW */
constexpr uint32_t RAY_MAT_GLOW = 0x40000;

struct RayInstance {
    uint32_t mesh = 0; /* the raster's, from add_mesh */
    uint32_t materials = 0; /* its first's index in RayPicture::materials */
    uint32_t flags = 0; /* RAY_* */
    uint32_t light = 0; /* the picture's light that shades it */
    float object_to_world[12] = {};
    float before[12] = {}; /* the same in the picture before */
    /* its draw's sa_shade.light: the way the light travels, in world space, at
     * the length the game gave it. a point's ramp entry comes from the dot
     * product of this and the point's normal under object_to_world, neither
     * normalized. 0 where the shade has no light, or its light is not finite */
    float light_way[3] = {};
    /* under RAY_SHADE_RAMP or RAY_SHADE_LINEAR, its draw's sa_shade.ambient
     * and sa_shade.diffuse */
    float ambient = 0.0F;
    float diffuse = 0.0F;
    /* its identity from picture to picture, or 0 for a draw with none. RayPast
     * finds its matrix in the picture before by this */
    uint64_t name = 0;
};

struct RayMaterial {
    uint32_t texture = 0; /* the raster's, 0 for white */
    uint32_t sample = 0; /* SAMPLE_*, as a batch's State holds it */
    uint32_t flags = 0; /* SA_MAT_* and RAY_MAT_* */
    uint32_t color = 0xffffffff; /* ARGB */
    /* ARGB, the tint of its reflection under SA_MAT_ENV */
    uint32_t specular = 0;
    /* 1 + the light whose ramp holds its highlights, or 0 for a material with
     * none */
    uint32_t gloss = 0;
    uint8_t src = 0; /* SA_BLEND_*, under SA_MAT_BLEND */
    uint8_t dst = 0;
};

/* the range over which a texel of an SA_MAT_EMISSIVE material emits, by its
 * brightest channel: nothing at EMISSIVE_DIM or below, its whole color at
 * EMISSIVE_LIT or above, and a smoothstep between. trace.comp copies both */
constexpr float EMISSIVE_DIM = 0.35F;
constexpr float EMISSIVE_LIT = 0.75F;

/* a triangle of a lamp's shape, in world space: a corner, the edges from it
 * to the other two, and the triangle's area */
struct RayPatch {
    float a[3] = {};
    float ab[3] = {};
    float ac[3] = {};
    float area = 0.0F;
};

/* a lamp: a light the scene makes from each additive sprite in the world, a
 * sphere, and from each cell of an emissive material's triangles, which
 * shine from where they are */
struct RayLamp {
    float at[3] = {}; /* its center, in world space */
    /* its sprite's, or that of a disc with its triangles' area */
    float radius = 0.0F;
    /* in linear light: its light on a surface that touches it and faces it */
    float color[3] = {};
    /* the distance from its center to the farthest corner of its triangles.
     * 0 for a sprite's */
    float extent = 0.0F;
    /* its triangles, as a run of RayPicture::patches. a sprite's lamp, or
     * one whose triangles the picture had no room for, has none, and
     * shines as a sphere */
    uint32_t first = 0;
    uint32_t count = 0;
};
/* the most lamps a picture holds. where the scene makes more, the nearest to
 * the eye are kept */
constexpr uint32_t RAY_LAMPS = 2048;
/* the most triangles a picture's lamps hold between them */
constexpr uint32_t RAY_PATCHES = 1 << 16;
/* the least light of a lamp on a surface that counts, where 1 is display white.
 * a lamp's light on a surface that faces it is taken, for this, as color *
 * radius^2 / (distance^2 + radius^2), where the distance is lamp_far()'s:
 * from the lamp's shape, not its center, so that a long tube of neon counts
 * near its ends as it does near its middle. below the floor a lamp's light is
 * dropped, from 4 times the floor it is whole, and a smoothstep joins the two,
 * so a lamp lights nothing beyond lamp_reach(). the game's palette shade
 * already accounts for distant lamps. a tuning value, which the trace pass
 * reads from its uniforms */
constexpr float LAMP_FLOOR = 0.04F;
/* how far from its shape a lamp lights anything: its light fades out over the
 * farther half of this. a large lamp far overhead would otherwise tint a
 * whole room, whose color the game's palette already holds. a tuning value,
 * which the trace pass reads from its uniforms */
constexpr float LAMP_REACH = 30.0F;

/* how much nearer than its center a lamp counts as being: what its triangles
 * extend past a disc of their area. trace.comp subtracts it from the distance
 * to the center */
inline float lamp_slack(const RayLamp &lamp)
{
    return std::max(lamp.extent - lamp.radius, 0.0F);
}

/* the distance from a lamp's center past which it lights nothing: where its
 * brightest channel, on a surface that faces it, falls to LAMP_FLOOR, or
 * LAMP_REACH from its shape if that is nearer. 0 for a lamp too dim to light
 * anything */
inline float lamp_reach(const RayLamp &lamp)
{
    float most = std::max({lamp.color[0], lamp.color[1], lamp.color[2]});

    return most > LAMP_FLOOR
               ? lamp_slack(lamp) +
                     std::min(lamp.radius * sqrtf(most / LAMP_FLOOR - 1.0F),
                              LAMP_REACH)
               : 0.0F;
}

/* the lamps as the trace pass reads them: a hash grid over world space in
 * which each cell lists the lamps whose light reaches it, so a surface point
 * tests its own cell's lamps and no others. the grid is exact: a lamp lights
 * nothing beyond lamp_reach(), and is listed in every cell its reach
 * touches. words holds
 *
 *   0  the hash table's slot count, a power of two, or 0 for no lamps
 *   1  the lamp count
 *   2  1 / the cell size, as a float's bits
 *   3  the count of list entries
 *
 * then the hash table, LAMP_SLOT words a slot: a cell's x, y and z, and its
 * lamps as the index of its first list entry << 12 | their count, or
 * LAMP_EMPTY for an unused slot. a cell's slot is the first with its
 * coordinates by linear probing from lamp_slot(), and a cell with no slot
 * has no lamps. then the list, one lamp index an entry. then the lamps,
 * LAMP_WORDS words each: the center, the radius squared, the color and
 * lamp_slack(), as floats' bits, then the index of its first patch among the
 * patches and their count, and two spare words. then the patches, PATCH_WORDS
 * words each, as floats' bits: a corner, then the share of the lamp's area
 * that its patches up to this one hold, which is 1 at its last, then the two
 * edges, a spare word after each */
constexpr uint32_t LAMP_EMPTY = 0xffffffff;
constexpr uint32_t LAMP_HEAD = 4;
constexpr uint32_t LAMP_SLOT = 4;
constexpr uint32_t LAMP_WORDS = 12;
constexpr uint32_t PATCH_WORDS = 12;
/* the smallest cell size. a picture's cells are half its longest reach where
 * that is larger, so a lamp is listed in at most 5 cells an axis */
constexpr float LAMP_CELL = 64.0F;

inline uint32_t lamp_slot(int32_t x, int32_t y, int32_t z, uint32_t slots)
{
    return ((static_cast<uint32_t>(x) * 73856093U) ^
            (static_cast<uint32_t>(y) * 19349663U) ^
            (static_cast<uint32_t>(z) * 83492791U)) &
           (slots - 1);
}

inline void lamp_table(const std::vector<RayLamp> &lamps,
                       const std::vector<RayPatch> &patches,
                       std::vector<uint32_t> &words)
{
    struct Entry {
        int32_t cell[3];
        uint32_t lamp;
    };
    auto bits = [](float f) {
        uint32_t u;

        memcpy(&u, &f, sizeof u);
        return u;
    };
    std::vector<Entry> entries;
    float longest = 0.0F;

    for (const RayLamp &lamp : lamps) {
        longest = std::max(longest, lamp_reach(lamp));
    }
    float cell = std::max(LAMP_CELL, longest / 2.0F);
    float inverse = 1.0F / cell;

    for (uint32_t i = 0; i < lamps.size(); i++) {
        /* slightly beyond the reach, so that rounding leaves out no cell */
        float reach = lamp_reach(lamps[i]) * 1.001F + 0.01F;
        int32_t low[3], high[3];

        if (!(lamp_reach(lamps[i]) > 0.0F)) {
            continue;
        }
        for (int k = 0; k < 3; k++) {
            low[k] = static_cast<int32_t>(
                floorf((lamps[i].at[k] - reach) * inverse));
            high[k] = static_cast<int32_t>(
                floorf((lamps[i].at[k] + reach) * inverse));
        }
        for (int32_t z = low[2]; z <= high[2]; z++) {
            for (int32_t y = low[1]; y <= high[1]; y++) {
                for (int32_t x = low[0]; x <= high[0]; x++) {
                    entries.push_back(Entry{{x, y, z}, i});
                }
            }
        }
    }
    std::stable_sort(entries.begin(), entries.end(),
                     [](const Entry &a, const Entry &b) {
                         return std::lexicographical_compare(
                             a.cell, a.cell + 3, b.cell, b.cell + 3);
                     });
    uint32_t cells = 0;

    for (size_t i = 0; i < entries.size(); i++) {
        cells += i == 0 || memcmp(entries[i].cell, entries[i - 1].cell,
                                  sizeof entries[i].cell) != 0;
    }
    uint32_t slots = 0;

    if (cells != 0) {
        for (slots = 1; slots < cells * 2; slots *= 2) {
        }
    }
    uint32_t list = LAMP_HEAD + slots * LAMP_SLOT;
    uint32_t first = list + static_cast<uint32_t>(entries.size());
    uint32_t shapes = first + static_cast<uint32_t>(lamps.size()) * LAMP_WORDS;

    words.assign(shapes + patches.size() * PATCH_WORDS, 0);
    words[0] = slots;
    words[1] = static_cast<uint32_t>(lamps.size());
    words[2] = bits(inverse);
    words[3] = static_cast<uint32_t>(entries.size());
    for (uint32_t s = 0; s < slots; s++) {
        words[LAMP_HEAD + s * LAMP_SLOT + 3] = LAMP_EMPTY;
    }
    for (size_t i = 0; i < entries.size();) {
        size_t end = i + 1;

        while (end < entries.size() &&
               memcmp(entries[end].cell, entries[i].cell,
                      sizeof entries[i].cell) == 0) {
            end++;
        }
        const int32_t *c = entries[i].cell;
        uint32_t slot = lamp_slot(c[0], c[1], c[2], slots);

        while (words[LAMP_HEAD + slot * LAMP_SLOT + 3] != LAMP_EMPTY) {
            slot = (slot + 1) & (slots - 1);
        }
        uint32_t *at = &words[LAMP_HEAD + slot * LAMP_SLOT];

        memcpy(at, c, 3 * sizeof *at);
        at[3] = static_cast<uint32_t>(i) << 12 |
                static_cast<uint32_t>(std::min<size_t>(end - i, 4095));
        for (size_t k = i; k < end; k++) {
            words[list + k] = entries[k].lamp;
        }
        i = end;
    }
    for (size_t i = 0; i < lamps.size(); i++) {
        uint32_t *at = &words[first + i * LAMP_WORDS];

        for (int k = 0; k < 3; k++) {
            at[k] = bits(lamps[i].at[k]);
            at[4 + k] = bits(lamps[i].color[k]);
        }
        at[3] = bits(lamps[i].radius * lamps[i].radius);
        at[7] = bits(lamp_slack(lamps[i]));
        at[8] = lamps[i].first;
        at[9] = lamps[i].count;
        /* each patch's share of the lamp's area, summed along the run */
        double whole = 0.0;
        double sum = 0.0;

        for (uint32_t k = 0; k < lamps[i].count; k++) {
            whole += patches[lamps[i].first + k].area;
        }
        for (uint32_t k = 0; k < lamps[i].count; k++) {
            const RayPatch &patch = patches[lamps[i].first + k];
            uint32_t *to = &words[shapes + (lamps[i].first + k) * PATCH_WORDS];

            sum += patch.area;
            for (int j = 0; j < 3; j++) {
                to[j] = bits(patch.a[j]);
                to[4 + j] = bits(patch.ab[j]);
                to[8 + j] = bits(patch.ac[j]);
            }
            to[3] = bits(k + 1 == lamps[i].count
                             ? 1.0F
                             : static_cast<float>(sum / whole));
        }
    }
}

/* the lights a picture's instances choose from. SA1's are its palettes */
constexpr uint32_t RAY_LIGHTS = 8;
/* the entries of a light's ramp, from a normal that faces the light to one that
 * faces away. an entry is two ARGB colors: the game's shade, then the highlight
 * a glossy material adds. a ramp need not darken all the way: SA1's brighten
 * again for normals that face away */
constexpr uint32_t RAY_RAMP = 256;

struct RayPicture {
    std::vector<RayInstance> instances;
    std::vector<RayMaterial> materials;
    std::vector<RayLamp> lamps;
    std::vector<RayPatch> patches; /* the lamps' triangles */
    double time = 0.0; /* the picture's, in seconds */
    /* the time in seconds since the traced picture before, from RayPast::pair:
     * 0 for a cut picture, and never negative */
    float step = 0.0F;
    /* the inverse of the picture's view: view to world, a 3 by 4 matrix by
     * rows, whose last column is the eye */
    float eye[12] = {};
    /* the picture before's view, world to view, and its focal lengths and
     * center, from RayPast::pair. motion vectors are computed from them */
    float before_view[12] = {};
    float before_project[4] = {};
    /* the rays a pixel, at least 1 */
    uint32_t samples = 1;
    /* the unit direction to the sun in world space, where y is up. straight up
     * for a picture whose draws give no sun */
    float to_sun[3] = {0.0F, 1.0F, 0.0F};
    /* each light's ramp: RAY_RAMP entries of two colors */
    uint32_t ramps[RAY_LIGHTS * RAY_RAMP * 2] = {};
    /* the background's ARGB at the top left, top right and bottom left, from
     * sa_picture.back: the color of a ray that hits nothing */
    uint32_t back[3] = {};
    /* reuse nothing of the picture before: no history, and no eased enclosure.
     * set by RayPast::pair */
    bool cut = false;

    void clear()
    {
        instances.clear();
        materials.clear();
        lamps.clear();
        patches.clear();
        step = 0.0F;
        cut = false;
        to_sun[0] = to_sun[2] = 0.0F;
        to_sun[1] = 1.0F;
        /* the default ramp, for a picture that gives none: from white for a
         * normal that faces the light to gray 136, a quarter of white in
         * linear light, for one perpendicular to it or facing away, with no
         * highlight */
        for (uint32_t i = 0; i < RAY_LIGHTS * RAY_RAMP; i++) {
            uint32_t entry = i % RAY_RAMP;
            uint32_t gray =
                entry < RAY_RAMP / 2 ? 255 - entry * 119 / 127 : 136;

            ramps[i * 2] = 0xff000000 | gray << 16 | gray << 8 | gray;
            ramps[i * 2 + 1] = 0;
        }
    }
};

} // namespace pc

#endif
