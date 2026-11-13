#include "scene.h"

#include <math.h>
#include <string.h>

#include <algorithm>
#include <iterator>

#include "ninja.h"
#include "sa_sa1.h"

namespace pc {

namespace {

/* the most a picture holds, past which a draw is refused */
constexpr uint32_t MOST_TRIANGLES = 1U << 19;
constexpr uint32_t MOST_POINTS = 65536; /* a corner's index is 16 bits */
constexpr uint32_t WORDS =
    SA_PALETTES * SA_PALETTE_ENTRIES * 2 + SA_UNLIT_OFFSETS;

/* an ARGB color as the raster's vertex holds it, RGBA in memory */
uint32_t rgba(uint32_t argb)
{
    return ((argb >> 16) & 0xff) | (argb & 0xff00) | (argb & 0xff) << 16 |
           (argb & 0xff000000);
}

/* append triangles of one state to list. they join the last batch where it
 * has the same state */
void add(std::vector<Vertex> &list, std::vector<Batch> &batches,
         const State &state, const Vertex *corners, uint32_t count)
{
    if (batches.empty() ||
        memcmp(&batches.back().state, &state, sizeof state) != 0 ||
        batches.back().first + batches.back().count != list.size()) {
        Batch b;
        b.state = state;
        b.first = static_cast<uint32_t>(list.size());
        batches.push_back(b);
    }
    list.insert(list.end(), corners, corners + count);
    batches.back().count += count;
}

/* a 3 by 4 matrix by rows times one of floats, rounded to floats */
void carry_matrix(const double *c, const float *m, float *out)
{
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            double sum = j == 3 ? c[(i * 4) + 3] : 0.0;

            for (int k = 0; k < 3; k++) {
                sum += c[(i * 4) + k] * m[(k * 4) + j];
            }
            out[(i * 4) + j] = static_cast<float>(sum);
        }
    }
}

/* a 3 by 3 matrix by rows times a vector of floats */
void turn_vector(const double *t, const float *v, float *out)
{
    float in[3] = {v[0], v[1], v[2]};

    for (int i = 0; i < 3; i++) {
        out[i] =
            static_cast<float>((t[i * 3] * in[0]) + (t[(i * 3) + 1] * in[1]) +
                               (t[(i * 3) + 2] * in[2]));
    }
}

} // namespace

bool affine_inverse(const float *m, double *out)
{
    double a[3][3];

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            a[i][j] = m[(i * 4) + j];
        }
    }
    double c00 = (a[1][1] * a[2][2]) - (a[1][2] * a[2][1]);
    double c01 = (a[1][2] * a[2][0]) - (a[1][0] * a[2][2]);
    double c02 = (a[1][0] * a[2][1]) - (a[1][1] * a[2][0]);
    double det = (a[0][0] * c00) + (a[0][1] * c01) + (a[0][2] * c02);

    if (!(fabs(det) > 1e-30) || !isfinite(det)) {
        return false;
    }
    double inv[3][3] = {
        {c00 / det, ((a[0][2] * a[2][1]) - (a[0][1] * a[2][2])) / det,
         ((a[0][1] * a[1][2]) - (a[0][2] * a[1][1])) / det},
        {c01 / det, ((a[0][0] * a[2][2]) - (a[0][2] * a[2][0])) / det,
         ((a[0][2] * a[1][0]) - (a[0][0] * a[1][2])) / det},
        {c02 / det, ((a[0][1] * a[2][0]) - (a[0][0] * a[2][1])) / det,
         ((a[0][0] * a[1][1]) - (a[0][1] * a[1][0])) / det}};

    for (int i = 0; i < 3; i++) {
        double t = 0;

        for (int j = 0; j < 3; j++) {
            out[(i * 4) + j] = inv[i][j];
            t -= inv[i][j] * m[(j * 4) + 3];
        }
        out[(i * 4) + 3] = t;
    }
    return true;
}

void affine_product(const double *a, const double *b, double *out)
{
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 4; j++) {
            double sum = j == 3 ? a[(i * 4) + 3] : 0.0;

            for (int k = 0; k < 3; k++) {
                sum += a[(i * 4) + k] * b[(k * 4) + j];
            }
            out[(i * 4) + j] = sum;
        }
    }
}

bool mesh_view(const void *bytes, uint32_t size, MeshView &view)
{
    const auto *b = static_cast<const uint8_t *>(bytes);
    sa_mesh head;

    if (b == nullptr || size < sizeof head) {
        return false;
    }
    memcpy(&head, b, sizeof head);
    uint64_t at = sizeof head + uint64_t{head.points} * 12;
    uint64_t sets = at + uint64_t{head.normals} * 12;

    if ((head.normals != 0 && head.normals != head.points) ||
        head.points > MOST_POINTS || sets > size) {
        return false;
    }
    view = MeshView{};
    view.points = reinterpret_cast<const float *>(b + sizeof head);
    view.normals =
        head.normals != 0 ? reinterpret_cast<const float *>(b + at) : nullptr;
    view.sets = b + sets;
    view.count = head.points;
    view.meshsets = head.meshsets;
    at = sets;
    for (uint32_t i = 0; i < head.meshsets; i++) {
        sa_meshset set;

        if (at + sizeof set > size) {
            return false;
        }
        memcpy(&set, b + at, sizeof set);
        at += sizeof set;
        uint64_t next = at + ((uint64_t{set.corners} * 2 + 3) & ~uint64_t{3});
        if (set.flags & SA_MESHSET_UV) {
            next += uint64_t{set.corners} * 8;
        }
        if (set.corners % 3 != 0 || next > size) {
            return false;
        }
        for (uint32_t c = 0; c < set.corners; c++) {
            uint16_t corner;

            memcpy(&corner, b + at + c * 2, sizeof corner);
            if (corner >= head.points) {
                return false;
            }
        }
        view.triangles += set.corners / 3;
        if (set.material >= view.materials) {
            view.materials = set.material + 1U;
        }
        at = next;
    }
    return head.materials >= view.materials;
}

void Scene::open(uint32_t blend)
{
    blend_ = blend;
    vertices_.reserve(MOST_TRIANGLES * 3);
    batches_.reserve(1U << 16);
    /* a picture takes at most its own view, a home, and one for each of
     * World's lenses_ */
    lenses_.reserve(2 + LENSES);
    blend_list_.reserve(MOST_TRIANGLES);
    world_.resize(MOST_POINTS * 3);
    base_.resize(MOST_POINTS);
    offset_.resize(MOST_POINTS);
    env_.resize(MOST_POINTS * 2);
    intensity_.resize(MOST_POINTS);
    normal_.resize(MOST_POINTS * 3);
    lit_.resize(MOST_POINTS);
}

void Scene::begin(const sa_picture &picture)
{
    picture_ = picture;
    lenses_.clear();
    view(picture.view);
    home_ = 0;
    fog_ = sa_fog{};
    palettes_ = nullptr;
    triangle_count_ = 0;
    vertices_.clear();
    batches_.clear();
    blend_list_.clear();
    if (picture.back[0] != picture.back[1] ||
        picture.back[0] != picture.back[2]) {
        back_plane(picture.back);
    }
}

/* a background under everything that follows, with back's colors at the top
 * left, top right and bottom left. the bottom right takes what their plane
 * gives there */
void Scene::back_plane(const uint32_t back[3])
{
    uint32_t corner = 0;
    State state;

    for (uint32_t shift = 0; shift < 32; shift += 8) {
        int left = static_cast<int>(back[0] >> shift & 0xff);
        int right = static_cast<int>(back[1] >> shift & 0xff);
        int below = static_cast<int>(back[2] >> shift & 0xff);
        int sum = right + below - left;

        corner |= static_cast<uint32_t>(sum < 0     ? 0
                                        : sum > 255 ? 255
                                                    : sum)
                  << shift;
    }
    const float width = static_cast<float>(WIDTH);
    const float height = static_cast<float>(HEIGHT);
    /* Ninja's plane lies at 1/z of 0.00001 */
    const float z = 100000.0F;
    Vertex quad[6] = {{{0.0F, 0.0F, z}, {0.0F, 0.0F}, rgba(back[0]), 0},
                      {{width, 0.0F, z}, {0.0F, 0.0F}, rgba(back[1]), 0},
                      {{0.0F, height, z}, {0.0F, 0.0F}, rgba(back[2]), 0},
                      {{width, 0.0F, z}, {0.0F, 0.0F}, rgba(back[1]), 0},
                      {{0.0F, height, z}, {0.0F, 0.0F}, rgba(back[2]), 0},
                      {{width, height, z}, {0.0F, 0.0F}, rgba(corner), 0}};

    state.flags = BATCH_SCREEN | BATCH_OPAQUE |
                  static_cast<uint32_t>(SA_FOG_NONE) << BATCH_FOG_SHIFT;
    state.depth = SA_DEPTH_ALWAYS;
    state.depth_write = 0;
    add(vertices_, batches_, state, quad, 6);
}

bool Scene::room(uint32_t triangles) const
{
    return triangle_count_ + uint64_t{triangles} <= MOST_TRIANGLES;
}

uint32_t Scene::view(const sa_view &view)
{
    Lens lens{};

    lens.view = view;
    lens.home = NONE;
    lenses_.push_back(lens);
    return static_cast<uint32_t>(lenses_.size() - 1);
}

bool Scene::carries(uint32_t home, uint32_t index) const
{
    const float *from = lenses_[home].view.world_to_view;
    double inverse[12];

    return memcmp(from, lenses_[index].view.world_to_view,
                  sizeof lenses_[index].view.world_to_view) == 0 ||
           affine_inverse(from, inverse);
}

/* the lens that carries a draw under view index into the home's space. it
 * returns null where the views share a matrix or the home has no inverse,
 * which carries() refuses */
const Scene::Lens *Scene::carried(uint32_t index)
{
    Lens &lens = lenses_[index];

    if (lens.home != home_) {
        const float *from = look();
        const float *to = lens.view.world_to_view;
        double inverse[12];
        double own[12];
        double back[12] = {};

        lens.home = home_;
        lens.same = memcmp(from, to, sizeof lens.view.world_to_view) == 0 ||
                    !affine_inverse(from, inverse);
        if (!lens.same) {
            for (int i = 0; i < 12; i++) {
                own[i] = to[i];
            }
            affine_product(inverse, own, lens.carry);
            /* a light turns as a normal does, by the transpose of the
             * carry's inverse. where the lens's view has no inverse, the light
             * takes the carry's own 3 by 3. that is the same turn for rigid
             * views */
            bool turns = affine_inverse(to, inverse);

            if (turns) {
                for (int i = 0; i < 12; i++) {
                    own[i] = from[i];
                }
                affine_product(inverse, own, back);
            }
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    lens.turn[(i * 3) + j] =
                        turns ? back[(j * 4) + i] : lens.carry[(i * 4) + j];
                }
            }
        }
    }
    return lens.same ? nullptr : &lens;
}

/* the view z of a point in the home's space */
float Scene::view_z(const float *p) const
{
    const float *v = look();

    return v[8] * p[0] + v[9] * p[1] + v[10] * p[2] + v[11];
}

void Scene::draw(const sa_draw &d, const MeshView &mesh,
                 const sa_material *materials, const uint32_t *textures,
                 const sa_shade &shade, const sa_lights *lights, uint32_t view)
{
    const Lens *lens = carried(view);

    if (lens == nullptr) {
        placed(d, mesh, materials, textures, shade, lights);
        return;
    }
    sa_draw there = d;
    sa_shade lit = shade;
    sa_lights turned;

    carry_matrix(lens->carry, d.object_to_world, there.object_to_world);
    turn_vector(lens->turn, shade.light, lit.light);
    if (lights != nullptr) {
        turned = *lights;
        for (uint32_t i = 0; i < turned.count && i < SA_LIGHTS; i++) {
            sa_light &l = turned.light[i];
            float at[3] = {l.point[0], l.point[1], l.point[2]};

            for (int j = 0; j < 3; j++) {
                const double *row = &lens->carry[j * 4];

                l.point[j] =
                    static_cast<float>((row[0] * at[0]) + (row[1] * at[1]) +
                                       (row[2] * at[2]) + row[3]);
            }
            turn_vector(lens->turn, l.direction, l.direction);
        }
    }
    placed(there, mesh, materials, textures, lit,
           lights != nullptr ? &turned : nullptr);
}

void Scene::placed(const sa_draw &d, const MeshView &mesh,
                   const sa_material *materials, const uint32_t *textures,
                   const sa_shade &shade, const sa_lights *lights)
{
    const float *w = d.object_to_world;
    const float *v = look();
    const uint32_t *words = palettes_;
    /* the light in the model's space: its dots with w's columns, the
     * model's axes in world space */
    float light[3];
    for (int k = 0; k < 3; k++) {
        light[k] = w[k] * shade.light[0] + w[4 + k] * shade.light[1] +
                   w[8 + k] * shade.light[2];
    }
    uint32_t points = mesh.count;
    bool specular = (materials[0].flags & SA_MAT_SPECULAR) != 0;
    uint32_t pal = shade.palette & 7;
    float origin[3] = {w[3], w[7], w[11]};
    float draw_z = view_z(origin);
    uint32_t offsets = (pal + (specular ? 1 : 0)) * SA_PALETTE_ENTRIES * 2 + 1;

    for (uint32_t i = 0; i < points; i++) {
        const float *p = &mesh.points[i * 3];

        for (int j = 0; j < 3; j++) {
            world_[i * 3 + j] = w[j * 4] * p[0] + w[j * 4 + 1] * p[1] +
                                w[j * 4 + 2] * p[2] + w[j * 4 + 3];
        }
        lit_[i] = 0;
        if (mesh.normals == nullptr) {
            base_[i] = 0xffffffff;
            offset_[i] = 0;
            intensity_[i] = 1.0F;
            normal_[i * 3] = normal_[i * 3 + 1] = normal_[i * 3 + 2] = 0.0F;
            continue;
        }
        const float *n = &mesh.normals[i * 3];
        float dot = n[0] * light[0] + n[1] * light[1] + n[2] * light[2];
        intensity_[i] = ninja_intensity(shade, dot);
        /* a dot that is no number takes the first entry */
        float scaled = dot * 127.5F + 127.5F;
        int entry = scaled > 0.0F
                        ? (scaled < 255.0F ? static_cast<int>(scaled) : 255)
                        : 0;
        uint32_t at = offsets + entry * 2;
        base_[i] = words != nullptr
                       ? words[pal * SA_PALETTE_ENTRIES * 2 + entry * 2]
                       : 0;
        offset_[i] = words != nullptr && at < WORDS ? words[at] : 0;
        /* an env map's uv: the normal turned into view space */
        float wn[3];
        for (int j = 0; j < 3; j++) {
            wn[j] = w[j * 4] * n[0] + w[j * 4 + 1] * n[1] + w[j * 4 + 2] * n[2];
            normal_[i * 3 + j] = wn[j];
        }
        float vx = v[0] * wn[0] + v[1] * wn[1] + v[2] * wn[2];
        float vy = v[4] * wn[0] + v[5] * wn[1] + v[6] * wn[2];
        env_[i * 2] = vx * 0.5F + 0.5F;
        env_[i * 2 + 1] = vy * 0.5F + 0.5F;
    }
    const uint8_t *at = mesh.sets;
    /* SA1's shade: a meshset under SA_MAT_WHITE leaves the draw's points
     * white for itself and each meshset after it */
    bool white = false;

    for (uint32_t set = 0; set < mesh.meshsets; set++) {
        sa_meshset head;

        memcpy(&head, at, sizeof head);
        const auto *corners =
            reinterpret_cast<const uint16_t *>(at + sizeof head);
        const float *uv =
            head.flags & SA_MESHSET_UV
                ? reinterpret_cast<const float *>(at + meshset_uv_at(head))
                : nullptr;
        at += meshset_bytes(head);
        const sa_material &mat = materials[head.material];
        bool blended = (mat.flags & SA_MAT_BLEND) != 0;
        State state;
        uint32_t tex = 0;

        white = white || (mat.flags & SA_MAT_WHITE) != 0;
        if (mat.flags & SA_MAT_TEXTURE) {
            tex = textures[head.material];
        }
        state.flags = (tex != 0 ? BATCH_TEXTURE : 0) |
                      (mat.flags & SA_MAT_DECAL ? BATCH_DECAL : 0) |
                      (mat.flags & SA_MAT_OFFSET ? BATCH_OFFSET : 0) |
                      (blended ? 0 : BATCH_OPAQUE) |
                      (shade.mode == SA_SHADE_LIGHTS ? BATCH_NEAR : 0) |
                      static_cast<uint32_t>(shade.fog & 3) << BATCH_FOG_SHIFT;
        state.texture = tex;
        state.sample = sample_of(mat.filter, mat.mip_bias) |
                       (mat.flags & SA_MAT_CLAMP_U ? SAMPLE_CLAMP_U : 0) |
                       (mat.flags & SA_MAT_CLAMP_V ? SAMPLE_CLAMP_V : 0) |
                       (mat.flags & SA_MAT_FLIP_U ? SAMPLE_FLIP_U : 0) |
                       (mat.flags & SA_MAT_FLIP_V ? SAMPLE_FLIP_V : 0);
        state.blend = blended ? 1 : 0;
        state.src = blended ? mat.src : static_cast<uint8_t>(SA_BLEND_ONE);
        state.dst = blended ? mat.dst : static_cast<uint8_t>(SA_BLEND_ZERO);
        state.depth = shade.depth;
        state.depth_write = shade.flags & SA_SHADE_NO_DEPTH_WRITE ? 0 : 1;
        state.cull = mat.flags & SA_MAT_CULL ? 1 : 0;
        uint32_t alpha = static_cast<uint32_t>(mat.alpha) << 24;
        Vertex tri[3];

        for (uint32_t c = 0; c + 2 < head.corners; c += 3) {
            for (int k = 0; k < 3; k++) {
                uint32_t i = corners[c + k];
                Vertex &out = tri[k];
                uint32_t b = base_[i];
                uint32_t o = offset_[i];

                memcpy(out.at, &world_[i * 3], sizeof out.at);
                if (shade.mode == SA_SHADE_LIGHTS) {
                    /* a point is lit once for each material, as
                     * njModel.c's lit_key keeps it. a draw with no lights
                     * and an unlit material take the material's color */
                    if (lights == nullptr || (mat.flags & SA_MAT_UNLIT)) {
                        b = mat.diffuse;
                        o = 0;
                    } else {
                        if (lit_[i] != head.material + 1U) {
                            ninja_lit(*lights, mat, &world_[i * 3],
                                      &normal_[i * 3], base_[i], offset_[i]);
                            lit_[i] = head.material + 1U;
                        }
                        b = base_[i];
                        o = offset_[i];
                    }
                } else if (shade.mode == SA_SHADE_LINEAR &&
                           (mat.flags & SA_MAT_UNLIT)) {
                    b = mat.diffuse;
                    o = 0;
                } else if (shade.mode != SA_SHADE_PALETTE) {
                    b = ninja_scaled(mat.diffuse, intensity_[i]);
                    o = 0;
                } else if (mat.flags & SA_MAT_CONSTANT) {
                    b = alpha | (d.tint & 0xffffff);
                } else if (blended && ((mat.flags & SA_MAT_UNLIT) || white)) {
                    b = alpha | 0xffffff;
                } else if (blended) {
                    b = alpha | (b & 0xffffff);
                } else if (white) {
                    b |= 0xffffff;
                }
                if (shade.mode == SA_SHADE_PALETTE &&
                    (mat.flags & SA_MAT_UNLIT) &&
                    (mat.flags & SA_MAT_TEXTURE)) {
                    uint32_t word = SA_PALETTES * SA_PALETTE_ENTRIES * 2 + i;
                    o = word < WORDS ? words[word] : 0;
                }
                out.base = rgba(b);
                out.offset = rgba(o);
                if (mat.flags & SA_MAT_ENV) {
                    out.uv[0] = env_[i * 2];
                    out.uv[1] = env_[i * 2 + 1];
                } else if (uv != nullptr) {
                    out.uv[0] = uv[(c + k) * 2];
                    out.uv[1] = uv[(c + k) * 2 + 1];
                } else {
                    out.uv[0] = 0.0F;
                    out.uv[1] = 0.0F;
                }
            }
            if (blended) {
                BlendTriangle t{state,
                                {tri[0], tri[1], tri[2]},
                                draw_z,
                                (view_z(tri[0].at) + view_z(tri[1].at) +
                                 view_z(tri[2].at)) /
                                    3.0F,
                                false};
                blend_list_.push_back(t);
            } else {
                add(vertices_, batches_, state, tri, 3);
            }
            triangle_count_++;
        }
    }
}

void Scene::sprite(const sa_sprite &s, uint32_t texture, uint32_t view)
{
    const Lens *lens = s.flags & SA_SPRITE_SCREEN ? nullptr : carried(view);

    if (lens == nullptr) {
        placed(s, texture);
        return;
    }
    sa_sprite there = s;

    carry_matrix(lens->carry, s.object_to_world, there.object_to_world);
    placed(there, texture);
}

void Scene::placed(const sa_sprite &s, uint32_t texture)
{
    const float *w = s.object_to_world;
    const float *v = look();
    bool blended = (s.flags & SA_SPRITE_ALPHA) != 0;
    State state;
    float x[2] = {s.corner[0][0], s.corner[1][0]};
    float y[2] = {s.corner[0][1], s.corner[1][1]};
    float u[2] = {s.uv[0][0], s.uv[1][0]};
    float t[2] = {s.uv[0][1], s.uv[1][1]};
    float sine = sinf(s.angle);
    float cosine = cosf(s.angle);
    Vertex quad[4];

    if (s.flags & SA_SPRITE_HFLIP) {
        float swap = u[0];
        u[0] = u[1];
        u[1] = swap;
    }
    if (s.flags & SA_SPRITE_VFLIP) {
        float swap = t[0];
        t[0] = t[1];
        t[1] = swap;
    }
    state.flags = (texture != 0 ? BATCH_TEXTURE : 0) |
                  (blended ? 0 : BATCH_OPAQUE) |
                  static_cast<uint32_t>(s.fog & 3) << BATCH_FOG_SHIFT;
    state.texture = texture;
    state.sample = sample_of(s.filter, ADJUST_ONE);
    state.blend = blended ? 1 : 0;
    state.src = blended ? s.src : static_cast<uint8_t>(SA_BLEND_ONE);
    state.dst = blended ? s.dst : static_cast<uint8_t>(SA_BLEND_ZERO);
    state.depth = s.depth;
    float world[3];
    for (int j = 0; j < 3; j++) {
        world[j] = w[j * 4] * s.at[0] + w[j * 4 + 1] * s.at[1] +
                   w[j * 4 + 2] * s.at[2] + w[j * 4 + 3];
    }
    float view[3];
    for (int j = 0; j < 3; j++) {
        view[j] = v[j * 4] * world[0] + v[j * 4 + 1] * world[1] +
                  v[j * 4 + 2] * world[2] + v[j * 4 + 3];
    }
    bool screen = (s.flags & SA_SPRITE_SCREEN) != 0;
    bool facing = !screen && (s.flags & SA_SPRITE_SCALE) != 0;
    /* njDrawSprite2D's depth, with bias as Ninja's pri: 1/z is 1 where pri
     * is over -1, else -1/pri */
    float screen_z = 1.0F > -s.bias ? 1.0F : -s.bias;
    if (screen) {
        state.flags |= BATCH_SCREEN;
    }
    if (facing) {
        /* Ninja sends a facing sprite whole, or drops it where its z lies
         * outside the view's clips */
        float z = view[2] + s.bias;
        float far = picture_.view.far_z;
        if (!(z >= picture_.view.near_z) || (far > 0.0F && z > far)) {
            return;
        }
        state.flags |= BATCH_SCREEN;
    }
    /* the corners in the order engine/sa1/sprites.c's sprite_corners gives
     * them */
    for (int k = 0; k < 4; k++) {
        float cx = x[k >> 1] * s.scale[0];
        float cy = y[k & 1] * s.scale[1];
        float rx = cx;
        float ry = cy;
        Vertex &out = quad[k];

        if (s.flags & SA_SPRITE_ANGLE) {
            rx = cx * cosine - cy * sine;
            ry = cx * sine + cy * cosine;
        }
        if (screen) {
            /* the turned corner's pixel, placed on the screen by the
             * matrix's first two rows */
            float px = s.at[0] + rx;
            float py = s.at[1] + ry;
            out.at[0] = w[0] * px + w[1] * py + w[3];
            out.at[1] = w[4] * px + w[5] * py + w[7];
            out.at[2] = screen_z;
        } else if (facing) {
            /* Ninja scales the corner by the screen's distance over z, and x
             * by the picture's aspect too. focal over z is that scale, as
             * Ninja's y aspect is 1 */
            out.at[0] = picture_.view.focal[0] * view[0] / view[2] +
                        picture_.view.center[0] +
                        rx * (picture_.view.focal[0] / view[2]);
            out.at[1] = picture_.view.focal[1] * view[1] / view[2] +
                        picture_.view.center[1] +
                        ry * (picture_.view.focal[1] / view[2]);
            out.at[2] = view[2] + s.bias;
        } else {
            float p[3] = {s.at[0] + rx, s.at[1] + ry, s.at[2]};
            for (int j = 0; j < 3; j++) {
                out.at[j] = w[j * 4] * p[0] + w[j * 4 + 1] * p[1] +
                            w[j * 4 + 2] * p[2] + w[j * 4 + 3];
            }
        }
        out.uv[0] = u[k >> 1];
        out.uv[1] = t[k & 1];
        out.base = rgba(s.flags & SA_SPRITE_COLOR ? s.color : 0xffffffff);
        out.offset = 0;
    }
    Vertex tris[6] = {quad[0], quad[1], quad[2], quad[2], quad[1], quad[3]};
    if (blended) {
        float z = screen ? screen_z : facing ? view[2] + s.bias : view[2];
        blend_list_.push_back(
            BlendTriangle{state, {tris[0], tris[1], tris[2]}, z, z, screen});
        blend_list_.push_back(
            BlendTriangle{state, {tris[3], tris[4], tris[5]}, z, z, screen});
    } else {
        add(vertices_, batches_, state, tris, 6);
    }
    triangle_count_ += 2;
}

void Scene::triangles(const sa_triangles &t, const sa_point *points,
                      uint32_t texture, uint32_t view)
{
    const Lens *lens = t.flags & SA_TRIANGLES_SCREEN ? nullptr : carried(view);

    if (lens == nullptr) {
        placed(t, points, texture);
        return;
    }
    sa_triangles there = t;

    carry_matrix(lens->carry, t.object_to_world, there.object_to_world);
    placed(there, points, texture);
}

void Scene::placed(const sa_triangles &t, const sa_point *points,
                   uint32_t texture)
{
    const float *w = t.object_to_world;
    bool blended = (t.flags & SA_TRIANGLES_BLEND) != 0;
    bool screen = (t.flags & SA_TRIANGLES_SCREEN) != 0;
    uint32_t tex = t.flags & SA_TRIANGLES_TEXTURE ? texture : 0;
    State state;

    state.flags = (tex != 0 ? BATCH_TEXTURE : 0) |
                  (blended ? 0 : BATCH_OPAQUE) | (screen ? BATCH_SCREEN : 0) |
                  static_cast<uint32_t>(t.fog & 3) << BATCH_FOG_SHIFT;
    state.texture = tex;
    state.sample = sample_of(t.filter, ADJUST_ONE);
    state.blend = blended ? 1 : 0;
    state.src = blended ? t.src : static_cast<uint8_t>(SA_BLEND_ONE);
    state.dst = blended ? t.dst : static_cast<uint8_t>(SA_BLEND_ZERO);
    state.depth = t.depth;
    if (t.form == SA_TRIANGLES_LINES) {
        lines(t, points, state);
        return;
    }
    for (uint32_t i = 0; i < t.count; i++) {
        uint32_t corner[3];
        triangle_corners(t.form, i, corner);
        Vertex tri[3];
        for (int k = 0; k < 3; k++) {
            const sa_point &p = points[corner[k]];
            Vertex &out = tri[k];
            for (int j = 0; j < 3; j++) {
                out.at[j] = screen
                                ? p.at[j]
                                : w[j * 4] * p.at[0] + w[j * 4 + 1] * p.at[1] +
                                      w[j * 4 + 2] * p.at[2] + w[j * 4 + 3];
            }
            out.uv[0] = p.uv[0];
            out.uv[1] = p.uv[1];
            out.base = rgba(p.color);
            out.offset = 0;
        }
        if (blended) {
            float z = screen
                          ? (tri[0].at[2] + tri[1].at[2] + tri[2].at[2]) / 3.0F
                          : (view_z(tri[0].at) + view_z(tri[1].at) +
                             view_z(tri[2].at)) /
                                3.0F;
            blend_list_.push_back(
                BlendTriangle{state, {tri[0], tri[1], tri[2]}, z, z, screen});
        } else {
            add(vertices_, batches_, state, tri, 3);
        }
        triangle_count_++;
    }
}

/* njDrawLine3D's lines, as Ninja sends them. a line's strip reaches
 * LINE_SIDE pixels to one side and twice that to the other */
void Scene::lines(const sa_triangles &t, const sa_point *points,
                  const State &state)
{
    constexpr float LINE_SIDE = 0.525F;
    const float *w = t.object_to_world;
    const float *v = look();
    const float near = picture_.view.near_z;
    bool blended = (t.flags & SA_TRIANGLES_BLEND) != 0;

    for (uint32_t i = 0; i < t.count; i++) {
        float view[2][3];
        float uv[2][2];
        uint32_t color[2];
        for (int k = 0; k < 2; k++) {
            const sa_point &p = points[2 * i + k];
            float world[3];
            for (int j = 0; j < 3; j++) {
                world[j] = w[j * 4] * p.at[0] + w[j * 4 + 1] * p.at[1] +
                           w[j * 4 + 2] * p.at[2] + w[j * 4 + 3];
            }
            for (int j = 0; j < 3; j++) {
                view[k][j] = v[j * 4] * world[0] + v[j * 4 + 1] * world[1] +
                             v[j * 4 + 2] * world[2] + v[j * 4 + 3];
            }
            uv[k][0] = p.uv[0];
            uv[k][1] = p.uv[1];
            color[k] = p.color;
        }
        if (view[0][2] < near && view[1][2] < near) {
            continue;
        }
        for (int k = 0; k < 2; k++) {
            if (view[k][2] < near) {
                /* the line's other end, which lies past the plane */
                const float *kept = view[1 - k];
                float f = (near - kept[2]) / (view[k][2] - kept[2]);
                for (int j = 0; j < 3; j++) {
                    view[k][j] = kept[j] + (view[k][j] - kept[j]) * f;
                }
                uv[k][0] = uv[1 - k][0] + (uv[k][0] - uv[1 - k][0]) * f;
                uv[k][1] = uv[1 - k][1] + (uv[k][1] - uv[1 - k][1]) * f;
            }
        }
        float x[2];
        float y[2];
        for (int k = 0; k < 2; k++) {
            x[k] = picture_.view.focal[0] * view[k][0] / view[k][2] +
                   picture_.view.center[0];
            y[k] = picture_.view.focal[1] * view[k][1] / view[k][2] +
                   picture_.view.center[1];
        }
        float dx = x[0] - x[1];
        float dy = y[0] - y[1];
        float length = sqrtf(dx * dx + dy * dy);
        if (!(length > 0.0F)) {
            continue;
        }
        float side[2] = {dy / length * LINE_SIDE, -dx / length * LINE_SIDE};
        Vertex strip[4];
        for (int c = 0; c < 4; c++) {
            int k = c & 1;
            float reach = c < 2 ? -1.0F : 2.0F;
            Vertex &out = strip[c];
            out.at[0] = x[k] + side[0] * reach;
            out.at[1] = y[k] + side[1] * reach;
            out.at[2] = view[k][2];
            out.uv[0] = uv[k][0];
            out.uv[1] = uv[k][1];
            out.base = rgba(color[k]);
            out.offset = 0;
        }
        State screen = state;
        screen.flags |= BATCH_SCREEN;
        Vertex tris[6] = {strip[0], strip[1], strip[2],
                          strip[1], strip[2], strip[3]};
        if (blended) {
            float z = (view[0][2] + view[1][2]) / 2.0F;
            blend_list_.push_back(BlendTriangle{
                screen, {tris[0], tris[1], tris[2]}, z, z, false});
            blend_list_.push_back(BlendTriangle{
                screen, {tris[3], tris[4], tris[5]}, z, z, false});
        } else {
            add(vertices_, batches_, screen, tris, 6);
        }
        triangle_count_ += 2;
    }
}

void Scene::finish()
{
    /* the PowerVR's lists in order: opaque, then blend. a host's sort goes
     * from the farthest, and keeps the draws' order in a tie */
    if (blend_ != SA_PC_BLEND_PIXEL) {
        bool by_draw = blend_ == SA_PC_BLEND_DRAW;
        bool by_own = blend_ == SA_PC_BLEND_TRIANGLE;

        std::stable_sort(
            blend_list_.begin(), blend_list_.end(),
            [by_draw, by_own](const BlendTriangle &a, const BlendTriangle &b) {
                if (a.screen != b.screen) {
                    return b.screen;
                }
                if (a.screen) {
                    return a.own_z > b.own_z;
                }
                return by_draw  ? a.draw_z > b.draw_z
                       : by_own ? a.own_z > b.own_z
                                : false;
            });
    }
    for (const BlendTriangle &t : blend_list_) {
        add(vertices_, batches_, t.state, t.corners, 3);
    }
    blend_list_.clear();
}

void Scene::uniform(PictureUniform &u) const
{
    u = PictureUniform{};
    memcpy(u.view, look(), sizeof u.view);
    u.project[0] = picture_.view.focal[0];
    u.project[1] = picture_.view.focal[1];
    u.project[2] = picture_.view.center[0];
    u.project[3] = picture_.view.center[1];
    u.fog_color[0] = ((fog_.color >> 16) & 0xff) / 255.0F;
    u.fog_color[1] = ((fog_.color >> 8) & 0xff) / 255.0F;
    u.fog_color[2] = (fog_.color & 0xff) / 255.0F;
    u.fog_color[3] = 1.0F;
    u.fog[0] = fog_.density;
    u.fog[1] = picture_.view.near_z;
    /* an entry's two alphas, a float each */
    static_assert(sizeof(PictureUniform::table) / sizeof(float) ==
                      2 * (sizeof(sa_fog::table) / sizeof(uint16_t)),
                  "PictureUniform's table");
    for (size_t i = 0; i < std::size(fog_.table); i++) {
        u.table[i * 2] = static_cast<float>(fog_.table[i] >> 8);
        u.table[i * 2 + 1] = static_cast<float>(fog_.table[i] & 0xff);
    }
}

} // namespace pc
