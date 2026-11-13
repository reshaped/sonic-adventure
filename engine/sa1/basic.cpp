#include "basic.h"

#include <string.h>

#include <algorithm>

#include "disc.h"
#include "rules.h"
#include "sa_engine.h"

namespace pc {

namespace {

/* NJS_MODEL's and NJS_MESHSET's fields, as the SH-4 lays them out */
enum {
    MODEL_POINTS = 0,
    MODEL_NORMALS = 4,
    MODEL_COUNT = 8,
    MODEL_MESHSETS = 12,
    MODEL_MESHSET_COUNT = 20,
    MODEL_MATERIAL_COUNT = 22,
    MESHSET_SIZE = 24,
    MESHSET_TYPE = 0,
    MESHSET_COUNT = 2,
    MESHSET_MESHES = 4,
    MESHSET_UV = 20
};
/* njtypes.h's NJD_TRIMESH_END, the bit above a strip's point count, set on
 * a reversed strip. no header this file includes defines it */
constexpr uint32_t STRIP_REVERSED = 0x8000;
/* the most points a model may have, as src/draw/model.h's DRAW_MOST_POINTS */
constexpr uint32_t MOST_POINTS = 2048;
/* the SH-4's physical address, the low 29 bits that an address in P1 and the
 * same one in P2 share */
constexpr uint32_t PHYSICAL = 0x1fffffff;

struct Reader {
    Bytes b;
    uint32_t base;

    /* the offset in the file of address, or the file's size where it
     * lies outside */
    uint32_t at(uint32_t address) const
    {
        uint32_t a = address & PHYSICAL;
        uint32_t lo = base & PHYSICAL;

        return a >= lo && a - lo < b.size ? a - lo : b.size;
    }
};

void strip(Meshset &out, const uint16_t *s, uint32_t n, bool reversed,
           const float *uv)
{
    for (uint32_t i = 0; i + 2 < n; i++) {
        bool flip = ((i & 1) != 0) != reversed;
        uint32_t a = flip ? i + 1 : i;
        uint32_t b = flip ? i : i + 1;
        uint32_t k[3] = {a, b, i + 2};

        for (uint32_t c : k) {
            out.corners.push_back(s[c]);
            if (uv != nullptr) {
                out.uv.push_back(uv[c * 2]);
                out.uv.push_back(uv[c * 2 + 1]);
            }
        }
    }
}

} // namespace

bool ninja_model(const std::vector<uint8_t> &file, uint32_t base,
                 uint32_t offset, Model &model, std::string &why)
{
    Reader r{{file.data(), static_cast<uint32_t>(file.size())}, base};
    Bytes &b = r.b;
    uint32_t points = r.at(b.u32(offset + MODEL_POINTS));
    uint32_t normals_address = b.u32(offset + MODEL_NORMALS);
    uint32_t count = b.u32(offset + MODEL_COUNT);
    uint32_t meshsets = r.at(b.u32(offset + MODEL_MESHSETS));
    uint32_t meshset_count = b.u16(offset + MODEL_MESHSET_COUNT);

    model.materials = b.u16(offset + MODEL_MATERIAL_COUNT);
    if (!b.ok || count > MOST_POINTS || points >= b.size ||
        meshsets >= b.size) {
        why = "a model whose points or meshsets leave its file";
        return false;
    }
    model.points.resize(count * 3);
    for (uint32_t i = 0; i < count * 3; i++) {
        model.points[i] = b.f32(points + i * 4);
    }
    if (normals_address != 0) {
        uint32_t normals = r.at(normals_address);

        model.normals.resize(count * 3);
        for (uint32_t i = 0; i < count * 3; i++) {
            model.normals[i] = b.f32(normals + i * 4);
        }
    }
    for (uint32_t m = 0; m < meshset_count; m++) {
        uint32_t set = meshsets + m * MESHSET_SIZE;
        uint32_t type = b.u16(set + MESHSET_TYPE);
        uint32_t polygons = b.u16(set + MESHSET_COUNT);
        uint32_t meshes = r.at(b.u32(set + MESHSET_MESHES));
        uint32_t uv_address = b.u32(set + MESHSET_UV);
        bool has_uv = uv_address != 0;
        uint32_t uvs = has_uv ? r.at(uv_address) : 0;
        Meshset out;
        uint32_t at = meshes;
        uint32_t corner = 0;
        uint32_t size = type >> MESHSET_TYPE_SHIFT == 0   ? 3
                        : type >> MESHSET_TYPE_SHIFT == 1 ? 4
                                                          : 0;
        std::vector<uint16_t> s;
        std::vector<float> uv;

        out.material = type & MATERIAL_ID;
        for (uint32_t p = 0; p < polygons && b.ok; p++) {
            uint32_t n = size;
            bool reversed = false;

            if (n == 0) {
                uint32_t head = b.u16(at);

                n = head & STRIP_COUNT;
                reversed = (head & STRIP_REVERSED) != 0;
                at += 2;
            }
            s.resize(n);
            uv.resize(n * 2);
            for (uint32_t k = 0; k < n; k++) {
                s[k] = b.u16(at + k * 2);
                if (s[k] >= count) {
                    why = "a polygon past the model's points";
                    return false;
                }
                if (has_uv) {
                    uint32_t t = uvs + (corner + k) * 4;

                    uv[k * 2] =
                        static_cast<float>(static_cast<int16_t>(b.u16(t))) *
                        INV_255;
                    uv[k * 2 + 1] =
                        static_cast<float>(static_cast<int16_t>(b.u16(t + 2))) *
                        INV_255;
                }
            }
            at += n * 2;
            corner += n;
            strip(out, s.data(), n, reversed, has_uv ? uv.data() : nullptr);
        }
        /* the game's drawers take a meshset's material from the model's
         * list whatever its count says */
        model.materials = std::max(model.materials, out.material + 1);
        model.meshsets.push_back(std::move(out));
    }
    if (!b.ok) {
        why = "a model that runs past its file";
        return false;
    }
    return true;
}

void mesh_bytes(const Model &model, std::vector<uint8_t> &out)
{
    sa_mesh head{};
    size_t at = sizeof head;

    head.points = static_cast<uint32_t>(model.points.size() / 3);
    head.normals = static_cast<uint32_t>(model.normals.size() / 3);
    head.meshsets = static_cast<uint32_t>(model.meshsets.size());
    head.materials = model.materials;
    out.resize(at + (model.points.size() + model.normals.size()) * 4);
    memcpy(out.data(), &head, sizeof head);
    memcpy(out.data() + at, model.points.data(), model.points.size() * 4);
    at += model.points.size() * 4;
    if (!model.normals.empty()) {
        memcpy(out.data() + at, model.normals.data(), model.normals.size() * 4);
        at += model.normals.size() * 4;
    }
    for (const Meshset &set : model.meshsets) {
        sa_meshset s{};
        /* the corners' bytes, padded to a multiple of 4 */
        size_t corner_bytes = (set.corners.size() * 2 + 3) & ~size_t{3};

        s.material = static_cast<uint16_t>(set.material);
        s.flags = set.uv.empty() ? 0 : SA_MESHSET_UV;
        s.corners = static_cast<uint32_t>(set.corners.size());
        out.resize(at + sizeof s + corner_bytes + set.uv.size() * 4);
        memcpy(out.data() + at, &s, sizeof s);
        at += sizeof s;
        memcpy(out.data() + at, set.corners.data(), set.corners.size() * 2);
        at += corner_bytes;
        if (!set.uv.empty()) {
            memcpy(out.data() + at, set.uv.data(), set.uv.size() * 4);
            at += set.uv.size() * 4;
        }
    }
}

} // namespace pc
