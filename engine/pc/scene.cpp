#include "scene.h"

#include <float.h>
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

/* a color's channel, 0 to 255, in linear light */
float linear(uint32_t channel)
{
    return powf(static_cast<float>(channel & 0xff) / 255.0F, 2.2F);
}

/* how a material samples its texture, as State's sample holds it */
uint32_t sample_of(const sa_material &m)
{
    return pc::sample_of(m.filter, m.mip_bias) |
           (m.flags & SA_MAT_CLAMP_U ? SAMPLE_CLAMP_U : 0) |
           (m.flags & SA_MAT_CLAMP_V ? SAMPLE_CLAMP_V : 0) |
           (m.flags & SA_MAT_FLIP_U ? SAMPLE_FLIP_U : 0) |
           (m.flags & SA_MAT_FLIP_V ? SAMPLE_FLIP_V : 0);
}

/* the palette a draw's points take their offset colors from under
 * SA_SHADE_PALETTE: the shade's, or the one after it where the draw's first
 * material is under SA_MAT_SPECULAR. that may be SA_PALETTES, which is no
 * palette */
uint32_t offset_palette(const sa_shade &shade, const sa_material *materials)
{
    return (shade.palette & 7U) +
           (materials[0].flags & SA_MAT_SPECULAR ? 1 : 0);
}

/* a mesh's meshsets, in order. next() moves to the first and then to each
 * after it, and returns false past the last. head, corners and uv are then
 * the meshset's own: its corners, three a triangle, and two floats of uv a
 * corner, or null where it has none */
struct Meshsets {
    explicit Meshsets(const MeshView &mesh) : at(mesh.sets), left(mesh.meshsets)
    {
    }
    bool next()
    {
        if (left == 0) {
            return false;
        }
        left--;
        memcpy(&head, at, sizeof head);
        corners = reinterpret_cast<const uint16_t *>(at + sizeof head);
        uv = head.flags & SA_MESHSET_UV
                 ? reinterpret_cast<const float *>(at + meshset_uv_at(head))
                 : nullptr;
        at += meshset_bytes(head);
        return true;
    }

    sa_meshset head{};
    const uint16_t *corners = nullptr;
    const float *uv = nullptr;
    const uint8_t *at; /* the next meshset */
    uint32_t left; /* the meshsets from there */
};

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
    if (head.points != 0) {
        float low[3], high[3];
        float square = 0.0F;

        memcpy(low, view.points, sizeof low);
        memcpy(high, view.points, sizeof high);
        for (uint32_t i = 1; i < head.points; i++) {
            for (int k = 0; k < 3; k++) {
                low[k] = std::min(low[k], view.points[i * 3 + k]);
                high[k] = std::max(high[k], view.points[i * 3 + k]);
            }
        }
        for (int k = 0; k < 3; k++) {
            square += (high[k] - low[k]) * (high[k] - low[k]);
        }
        view.extent = isfinite(square) ? 0.5F * sqrtf(square) : 0.0F;
    }
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

void RayPast::pair(RayPicture &p, const float *view, const float *project)
{
    auto by_name = [](const Place &a, const Place &b) {
        return a.name < b.name;
    };

    next_.clear();
    for (RayInstance &instance : p.instances) {
        if (instance.name == 0) {
            continue;
        }
        Place now;

        now.name = instance.name;
        memcpy(now.at, instance.object_to_world, sizeof now.at);
        next_.push_back(now);
        if (!held_) {
            continue;
        }
        auto was =
            std::lower_bound(places_.begin(), places_.end(), now, by_name);

        if (was != places_.end() && was->name == now.name) {
            memcpy(instance.before, was->at, sizeof instance.before);
            instance.flags &= ~RAY_NEW;
        }
    }
    if (held_) {
        memcpy(p.before_view, view_, sizeof view_);
        memcpy(p.before_project, project_, sizeof project_);
    } else {
        memcpy(p.before_view, view, sizeof view_);
        memcpy(p.before_project, project, sizeof project_);
    }
    p.cut = !held_;
    p.step =
        held_ && p.time > time_ ? static_cast<float>(p.time - time_) : 0.0F;
    /* a name two instances share pairs with the first, in each picture */
    std::stable_sort(next_.begin(), next_.end(), by_name);
    memcpy(next_view_, view, sizeof next_view_);
    memcpy(next_project_, project, sizeof next_project_);
    next_time_ = p.time;
}

void RayPast::keep()
{
    places_.swap(next_);
    memcpy(view_, next_view_, sizeof view_);
    memcpy(project_, next_project_, sizeof project_);
    time_ = next_time_;
    held_ = true;
}

void LampMemory::hold(std::vector<RayLamp> &lamps,
                      std::vector<RayPatch> &patches,
                      const std::vector<uint64_t> &owners,
                      const std::vector<uint64_t> &drawn, const float *view,
                      const float *project, bool learn)
{
    /* the margin inside the screen's edges, as a share of its width and height,
     * where a position doesn't count as in view. the game's own culling can
     * drop an object that still overlaps the edge, so a lamp near the edge
     * doesn't show that its object is gone */
    constexpr float EDGE = 0.15F;
    /* the most lamps kept of the draws a picture lacks. the nearest to the eye
     * are kept */
    constexpr size_t MOST = 2048;
    auto seen = [view](const RayLamp &l, float *p) {
        for (int k = 0; k < 3; k++) {
            p[k] = view[k * 4] * l.at[0] + view[k * 4 + 1] * l.at[1] +
                   view[k * 4 + 2] * l.at[2] + view[k * 4 + 3];
        }
    };
    auto shown = [project, &seen](const RayLamp &l) {
        float p[3];

        seen(l, p);
        if (!(p[2] > 1.0F)) {
            return false;
        }
        float x = project[2] + project[0] * p[0] / p[2];
        float y = project[3] + project[1] * p[1] / p[2];

        return x > WIDTH * EDGE && x < WIDTH * (1.0F - EDGE) &&
               y > HEIGHT * EDGE && y < HEIGHT * (1.0F - EDGE);
    };
    /* a lamp with its triangles moved to the end of to, from their run of
     * from, or with none where room is given and they pass it */
    auto moved = [](RayLamp lamp, const std::vector<RayPatch> &from,
                    std::vector<RayPatch> &to, size_t room) {
        if (to.size() + lamp.count > room) {
            lamp.count = 0;
        }
        uint32_t first = static_cast<uint32_t>(to.size());

        to.insert(to.end(), from.begin() + lamp.first,
                  from.begin() + lamp.first + lamp.count);
        lamp.first = lamp.count != 0 ? first : 0;
        return lamp;
    };
    std::vector<Held> next;

    for (const Held &h : held_) {
        if (std::binary_search(drawn.begin(), drawn.end(), h.owner) ||
            shown(h.lamp)) {
            continue;
        }
        lamps.push_back(moved(h.lamp, shapes_, patches, RAY_PATCHES));
        next.push_back(h);
    }
    if (!learn) {
        return;
    }
    if (next.size() > MOST) {
        auto far = [&seen](const Held &h) {
            float p[3];

            seen(h.lamp, p);
            return p[0] * p[0] + p[1] * p[1] + p[2] * p[2];
        };

        std::nth_element(
            next.begin(), next.begin() + MOST, next.end(),
            [&far](const Held &a, const Held &b) { return far(a) < far(b); });
        next.resize(MOST);
    }
    std::vector<RayPatch> shapes;

    for (Held &h : next) {
        h.lamp = moved(h.lamp, shapes_, shapes, SIZE_MAX);
    }
    for (size_t i = 0; i < owners.size(); i++) {
        if (owners[i] != 0) {
            next.push_back(
                Held{owners[i], moved(lamps[i], patches, shapes, SIZE_MAX)});
        }
    }
    std::stable_sort(
        next.begin(), next.end(),
        [](const Held &a, const Held &b) { return a.owner < b.owner; });
    held_.swap(next);
    shapes_.swap(shapes);
}

void ray_mesh(const MeshView &view, RayMesh &out)
{
    out = RayMesh{};
    out.points = view.points;
    out.normals = view.normals;
    out.count = view.count;
    out.corners.reserve(view.triangles * 3);
    out.uv.reserve(view.triangles * 6);
    out.materials.reserve(view.triangles);
    for (Meshsets set(view); set.next();) {
        for (uint32_t c = 0; c + 2 < set.head.corners; c += 3) {
            for (uint32_t k = 0; k < 3; k++) {
                const float *uv = set.uv;

                out.corners.push_back(set.corners[c + k]);
                out.uv.push_back(uv != nullptr ? uv[(c + k) * 2] : 0.0F);
                out.uv.push_back(uv != nullptr ? uv[(c + k) * 2 + 1] : 0.0F);
            }
            out.materials.push_back(set.head.material);
        }
    }
}

/* SA1's palettes are a traced picture's lights: a palette's entries are a
 * light's ramp, unchanged */
void Scene::palettes(const uint32_t *words)
{
    static_assert(SA_PALETTES == RAY_LIGHTS, "a light for each palette");
    static_assert(SA_PALETTE_ENTRIES == RAY_RAMP, "a shade an entry");
    /* a picture between gives its palettes again at each draw */
    bool given = words == palettes_;

    palettes_ = words;
    if (tracing_ && words != nullptr && !given) {
        memcpy(rays_.ramps, words, sizeof rays_.ramps);
    }
}

void Scene::open(uint32_t blend, const std::vector<TextureColors> *colors)
{
    blend_ = blend;
    colors_ = colors;
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

void Scene::begin(const sa_picture &picture, double time)
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
    tracing_ = samples_ != 0;
    if (tracing_) {
        /* a traced picture's background is the color of a ray that hits
         * nothing, so it takes no back plane */
        rays_.clear();
        rays_.samples = samples_;
        rays_.time = time;
        memcpy(rays_.back, picture.back, sizeof rays_.back);
        sunned_ = false;
        parts_.clear();
        lamp_owners_.clear();
        drawn_.clear();
        return;
    }
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
    take(d, mesh, materials, textures, shade, lights, view, true);
}

void Scene::hidden(const sa_draw &d, const MeshView &mesh,
                   const sa_material *materials, const uint32_t *textures,
                   const sa_shade &shade, uint32_t view)
{
    if (tracing_) {
        take(d, mesh, materials, textures, shade, nullptr, view, false);
    }
}

/* take a mesh's draw, carried from the view of index view into the home's
 * space. seen says the picture shows it. in a traced picture it is an
 * instance where the raster holds its mesh for ray tracing. else it is
 * batches where seen, and nothing where not */
void Scene::take(const sa_draw &d, const MeshView &mesh,
                 const sa_material *materials, const uint32_t *textures,
                 const sa_shade &shade, const sa_lights *lights, uint32_t view,
                 bool seen)
{
    bool traced = tracing_ && mesh.rays != 0;

    /* record the part as given to the picture, whether or not the draw becomes
     * an instance */
    if (tracing_ && of_scene(d.instance)) {
        auto at = std::lower_bound(parts_.begin(), parts_.end(), d.instance);

        if (at == parts_.end() || *at != d.instance) {
            parts_.insert(at, d.instance);
        }
    }
    if (!traced && !seen) {
        return;
    }
    const Lens *lens = carried(view);

    if (lens == nullptr) {
        if (traced) {
            instance(d, mesh, materials, textures, shade, seen);
        } else {
            placed(d, mesh, materials, textures, shade, lights);
        }
        return;
    }
    sa_draw there = d;
    sa_shade lit = shade;
    sa_lights turned;

    carry_matrix(lens->carry, d.object_to_world, there.object_to_world);
    turn_vector(lens->turn, shade.light, lit.light);
    if (traced) {
        instance(there, mesh, materials, textures, lit, seen);
        return;
    }
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

bool Scene::holds(uint32_t instance) const
{
    return std::binary_search(parts_.begin(), parts_.end(), instance);
}

/* add a mesh's draw to the traced picture as an instance: its mesh, its
 * transform and its materials as the draw took them, with no lighting computed.
 * a material's color is its texel's under white light. seen says the picture
 * drew it, so camera rays may hit it */
void Scene::instance(const sa_draw &d, const MeshView &mesh,
                     const sa_material *materials, const uint32_t *textures,
                     const sa_shade &shade, bool seen)
{
    /* a traced picture casts its own shadows, so the game's stand-in for
     * one is left out */
    if (d.flags & SA_DRAW_SHADOW) {
        return;
    }
    /* a draw as large as this, by its mesh's extent under its matrix, is
     * scenery, as a cave or a building a task draws: a place, and no object
     * that stands in one */
    constexpr float SCENERY = 50.0F;
    RayInstance instance;
    bool part = of_scene(d.instance);
    const float *w = d.object_to_world;
    float scale = 0.0F;

    for (int k = 0; k < 3; k++) {
        scale = std::max(scale, sqrtf(w[k] * w[k] + w[4 + k] * w[4 + k] +
                                      w[8 + k] * w[8 + k]));
    }
    bool scenery = part || !(mesh.extent * scale < SCENERY);
    bool palette = shade.mode == SA_SHADE_PALETTE;
    /* whether the shade's mode leaves a material under SA_MAT_UNLIT unlit,
     * as Ninja's simple and full drawers do. under SA_SHADE_RAMP the flag
     * changes nothing, and a palette has rules of its own */
    bool unlit = shade.mode == SA_SHADE_LINEAR || shade.mode == SA_SHADE_LIGHTS;
    bool opaque = true;
    bool translucent = true;
    bool emissive = false;
    uint32_t gloss = offset_palette(shade, materials);
    /* the flag of a material the game computes no lighting for: one that
     * adds to what lies behind it is RAY_MAT_GLOW, and another, whose
     * lighting is in its texel, RAY_MAT_PRELIT */
    auto unshaded = [](const sa_material &m) {
        return (m.flags & SA_MAT_BLEND) && m.dst == SA_BLEND_ONE
                   ? RAY_MAT_GLOW
                   : RAY_MAT_PRELIT;
    };

    instance.mesh = mesh.rays;
    instance.materials = static_cast<uint32_t>(rays_.materials.size());
    instance.flags = (seen ? RAY_SEEN : 0) | RAY_NEW |
                     (mesh.normals != nullptr ? RAY_NORMALS : 0) |
                     (shade.fog == SA_FOG_TABLE ? RAY_FOG : 0) |
                     (d.flags & SA_DRAW_BACKDROP ? RAY_BACKDROP : 0) |
                     (d.flags & SA_DRAW_LIQUID ? RAY_LIQUID : 0) |
                     (scenery ? RAY_SCENERY : 0);
    instance.light = palette ? shade.palette & 7U : 0;
    if (shade.mode == SA_SHADE_RAMP || shade.mode == SA_SHADE_LINEAR) {
        instance.flags |=
            shade.mode == SA_SHADE_RAMP ? RAY_SHADE_RAMP : RAY_SHADE_LINEAR;
        instance.ambient = shade.ambient;
        instance.diffuse = shade.diffuse;
    }
    memcpy(instance.object_to_world, d.object_to_world,
           sizeof instance.object_to_world);
    memcpy(instance.before, d.object_to_world, sizeof instance.before);
    if (isfinite(shade.light[0]) && isfinite(shade.light[1]) &&
        isfinite(shade.light[2])) {
        memcpy(instance.light_way, shade.light, sizeof instance.light_way);
    }
    /* a draw's name is its drawer, its thing and its nth, and a scene's
     * part's is its instance alone. a draw with no thing has none. the
     * draws no task owns are one drawer's, whatever number each carries */
    if (d.thing != 0 || part) {
        uint32_t drawer =
            d.instance & SA_INSTANCE_UNOWNED ? SA_INSTANCE_UNOWNED : d.instance;

        instance.name = (static_cast<uint64_t>(drawer) << 32 | d.thing) *
                            0x9e3779b97f4a7c15ULL +
                        d.nth + 1;
    }
    /* a part of the act's scene is given to every picture, so the lamp
     * memory keeps nothing of it */
    if (!part && instance.name != 0) {
        drawn_.push_back(instance.name);
    }
    for (uint32_t i = 0; i < mesh.materials; i++) {
        const sa_material &m = materials[i];
        RayMaterial out;
        bool textured = (m.flags & SA_MAT_TEXTURE) != 0;
        bool blended = (m.flags & SA_MAT_BLEND) != 0;
        bool emits = textured && (m.flags & SA_MAT_EMISSIVE) != 0;

        out.texture = textured ? textures[i] : 0;
        out.sample = sample_of(m);
        out.flags = m.flags;
        if (!palette) {
            out.color = m.diffuse;
            out.flags |= RAY_MAT_COLOR;
        } else if (m.flags & SA_MAT_CONSTANT) {
            out.color =
                static_cast<uint32_t>(m.alpha) << 24 | (d.tint & 0xffffff);
            out.flags |= RAY_MAT_COLOR;
        } else {
            out.color = static_cast<uint32_t>(m.alpha) << 24 | 0xffffff;
        }
        /* the game computes no lighting for a material that shows its texel
         * alone. under a palette it computes none for one that takes the draw's
         * tint or white, nor for one that blends and is unlit. under Ninja's
         * simple and full drawers it computes none for an unlit one. the full
         * drawer lights the rest from its own list of lights, which a traced
         * picture is not given, so those are taken as prelit too */
        if ((textured && (m.flags & SA_MAT_DECAL)) ||
            (palette && ((m.flags & (SA_MAT_CONSTANT | SA_MAT_WHITE)) ||
                         (blended && (m.flags & SA_MAT_UNLIT)))) ||
            (unlit && (m.flags & SA_MAT_UNLIT)) ||
            shade.mode == SA_SHADE_LIGHTS) {
            out.flags |= unshaded(m);
        }
        if (m.flags & SA_MAT_ENV) {
            /* an environment-mapped material tints its reflection by the
             * map's mean color */
            out.specular = colors(out.texture).mean;
        } else if (palette && textured && (m.flags & SA_MAT_OFFSET) &&
                   gloss < SA_PALETTES) {
            /* its highlight is the offset color a point takes, from the
             * palette placed() takes it from */
            out.gloss = gloss + 1;
        }
        out.src = m.src;
        out.dst = m.dst;
        /* a ray's hit needs a test of the texel where the material blends
         * or is emissive. a material under SA_MAT_CULL needs none: a ray
         * hits both sides of every triangle */
        opaque = opaque && !blended && !emits;
        translucent = translucent && blended;
        emissive = emissive || emits;
        rays_.materials.push_back(out);
    }
    if (palette) {
        /* SA1's white carries on: a meshset under SA_MAT_WHITE leaves the
         * draw's points white for itself and each meshset after it, whatever
         * that one's material. the trace pass holds one material a slot, so
         * a material that meshsets on both sides of the first white one
         * name counts as white for all of them */
        bool white = false;

        for (Meshsets set(mesh); set.next();) {
            const sa_material &m = materials[set.head.material];

            white = white || (m.flags & SA_MAT_WHITE) != 0;
            if (white) {
                rays_.materials[instance.materials + set.head.material].flags |=
                    unshaded(m);
            }
        }
    }
    if (opaque) {
        instance.flags |= RAY_OPAQUE;
    }
    if (translucent) {
        instance.flags |= RAY_TRANSLUCENT;
    }
    if (emissive && !(d.flags & SA_DRAW_BACKDROP)) {
        owner_ = part ? 0 : instance.name;
        emitters(d, mesh, materials, textures);
        owner_ = 0;
    }
    /* the sun lies opposite the light of the first draw the picture shows
     * whose shade has one, whatever the shade's mode. a backdrop gives
     * none */
    if (seen && !sunned_ && !(d.flags & SA_DRAW_BACKDROP)) {
        double way[3] = {shade.light[0], shade.light[1], shade.light[2]};
        double size =
            sqrt((way[0] * way[0]) + (way[1] * way[1]) + (way[2] * way[2]));

        if (size > 0.0 && isfinite(size)) {
            for (int k = 0; k < 3; k++) {
                rays_.to_sun[k] = static_cast<float>(-way[k] / size);
            }
            sunned_ = true;
        }
    }
    rays_.instances.push_back(instance);
    if (seen) {
        triangle_count_ += mesh.triangles;
    }
}

TextureColors Scene::colors(uint32_t texture) const
{
    return colors_ != nullptr && texture < colors_->size() ? (*colors_)[texture]
                                                           : TextureColors{};
}

/* add a lamp to the traced picture. one with no light or no size, or whose
 * position or radius is not finite, is left out */
void Scene::add_lamp(const RayLamp &lamp)
{
    if (isfinite(lamp.at[0]) && isfinite(lamp.at[1]) && isfinite(lamp.at[2]) &&
        isfinite(lamp.radius) && lamp.radius > 0.0F &&
        lamp.color[0] + lamp.color[1] + lamp.color[2] > 0.0F) {
        rays_.lamps.push_back(lamp);
        lamp_owners_.push_back(owner_);
    }
}

/* make lamps of the triangles of a draw's emissive materials. each such
 * material's triangles in the draw are grouped by their centroids into
 * ceil(extent / CELL) cells along each axis, CELLS at most, where extent is
 * what the centroids span: a cell is CELL across or less, and wider where
 * the extent passes CELLS * CELL. each cell is a lamp at the area-weighted
 * centroid of its triangles, with the radius of a disc of their area and the
 * color of the texture's emission, and it holds the triangles, which are
 * where its light leaves from. so a row of bulbs, or a tube of neon, lights
 * what is near each part of it */
void Scene::emitters(const sa_draw &d, const MeshView &mesh,
                     const sa_material *materials, const uint32_t *textures)
{
    constexpr float CELL = 40.0F;
    constexpr int CELLS = 4;
    /* a lamp's light over its texels' display values: a light source is
     * brighter than a display shows it. a tuning value */
    constexpr float BRIGHTER = 2.0F;
    const float *w = d.object_to_world;

    for (uint32_t slot = 0; slot < mesh.materials; slot++) {
        const sa_material &m = materials[slot];
        uint32_t emission = colors(textures[slot]).emission;

        if (!(m.flags & SA_MAT_EMISSIVE) || !(m.flags & SA_MAT_TEXTURE) ||
            emission == 0) {
            continue;
        }
        float low[3] = {FLT_MAX, FLT_MAX, FLT_MAX};
        float high[3] = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

        emitting_.clear();
        for (Meshsets set(mesh); set.next();) {
            if (set.head.material != slot) {
                continue;
            }
            for (uint32_t c = 0; c + 2 < set.head.corners; c += 3) {
                float p[3][3];
                Emitting made{};

                for (int k = 0; k < 3; k++) {
                    const float *local = &mesh.points[set.corners[c + k] * 3];

                    for (int j = 0; j < 3; j++) {
                        p[k][j] = w[j * 4] * local[0] +
                                  w[j * 4 + 1] * local[1] +
                                  w[j * 4 + 2] * local[2] + w[j * 4 + 3];
                    }
                }
                for (int j = 0; j < 3; j++) {
                    made.patch.a[j] = p[0][j];
                    made.patch.ab[j] = p[1][j] - p[0][j];
                    made.patch.ac[j] = p[2][j] - p[0][j];
                    made.centroid[j] = (p[0][j] + p[1][j] + p[2][j]) / 3.0F;
                }
                const float *u = made.patch.ab;
                const float *v = made.patch.ac;
                float cross[3] = {u[1] * v[2] - u[2] * v[1],
                                  u[2] * v[0] - u[0] * v[2],
                                  u[0] * v[1] - u[1] * v[0]};

                made.patch.area =
                    0.5F * sqrtf(cross[0] * cross[0] + cross[1] * cross[1] +
                                 cross[2] * cross[2]);
                /* a draw's matrix may take a triangle past what a float
                 * holds */
                if (!(made.patch.area > 0.0F) || !isfinite(made.patch.area) ||
                    !isfinite(made.centroid[0]) ||
                    !isfinite(made.centroid[1]) ||
                    !isfinite(made.centroid[2])) {
                    continue;
                }
                for (int j = 0; j < 3; j++) {
                    low[j] = std::min(low[j], made.centroid[j]);
                    high[j] = std::max(high[j], made.centroid[j]);
                }
                emitting_.push_back(made);
            }
        }
        if (emitting_.empty()) {
            continue;
        }
        int cells[3];
        float share = static_cast<float>(emission >> 24) / 255.0F;

        for (int j = 0; j < 3; j++) {
            cells[j] =
                static_cast<int>(std::clamp(ceilf((high[j] - low[j]) / CELL),
                                            1.0F, static_cast<float>(CELLS)));
        }
        for (Emitting &triangle : emitting_) {
            triangle.cell = 0;
            for (int j = 0; j < 3; j++) {
                float across = high[j] - low[j];
                /* how many cells along the centroid lies. the farthest
                 * centroid lies cells[j] along, and takes the last cell */
                float along = across > 0.0F
                                  ? (triangle.centroid[j] - low[j]) / across *
                                        static_cast<float>(cells[j])
                                  : 0.0F;
                int nth = along < static_cast<float>(cells[j] - 1)
                              ? static_cast<int>(along)
                              : cells[j] - 1;

                triangle.cell = triangle.cell * CELLS + nth;
            }
        }
        std::stable_sort(emitting_.begin(), emitting_.end(),
                         [](const Emitting &a, const Emitting &b) {
                             return a.cell < b.cell;
                         });
        for (size_t at = 0; at < emitting_.size();) {
            size_t end = at;
            double sum[4] = {};
            RayLamp made;

            for (; end < emitting_.size() &&
                   emitting_[end].cell == emitting_[at].cell;
                 end++) {
                const Emitting &triangle = emitting_[end];

                for (int k = 0; k < 3; k++) {
                    sum[k] += static_cast<double>(triangle.centroid[k]) *
                              triangle.patch.area;
                }
                sum[3] += triangle.patch.area;
            }
            for (int k = 0; k < 3; k++) {
                made.at[k] = static_cast<float>(sum[k] / sum[3]);
                made.color[k] =
                    BRIGHTER * linear(emission >> (16 - k * 8)) * share;
            }
            made.radius = sqrtf(static_cast<float>(sum[3]) / 3.14159265F);
            if (rays_.patches.size() + (end - at) <= RAY_PATCHES) {
                made.first = static_cast<uint32_t>(rays_.patches.size());
                made.count = static_cast<uint32_t>(end - at);
            }
            for (size_t t = at; t < end; t++) {
                const RayPatch &patch = emitting_[t].patch;

                for (int corner = 0; corner < 3; corner++) {
                    float far = 0.0F;

                    for (int k = 0; k < 3; k++) {
                        float off = patch.a[k] +
                                    (corner == 1 ? patch.ab[k] : 0.0F) +
                                    (corner == 2 ? patch.ac[k] : 0.0F) -
                                    made.at[k];

                        far += off * off;
                    }
                    made.extent = std::max(made.extent, sqrtf(far));
                }
                if (made.count != 0) {
                    rays_.patches.push_back(patch);
                }
            }
            add_lamp(made);
            at = end;
        }
    }
}

/* an additive sprite in the world is a light's glare, or a spark. a traced
 * picture takes it for a lamp at the sprite's point, sa_sprite.at, of its color
 * times its texture's mean, with a quarter of its width and height together for
 * a radius */
void Scene::lamp(const sa_sprite &s, uint32_t texture, const float *at)
{
    /* the gain on a lamp's color over its sprite's. a tuning value */
    constexpr float GLARE = 8.0F;
    uint32_t color = s.flags & SA_SPRITE_COLOR ? s.color : 0xffffffff;
    uint32_t texel = colors(texture).mean;
    float share = s.src == SA_BLEND_SRC_ALPHA
                      ? static_cast<float>(color >> 24) / 255.0F *
                            static_cast<float>(texel >> 24) / 255.0F
                      : 1.0F;
    float wide = fabsf((s.corner[1][0] - s.corner[0][0]) * s.scale[0]);
    float high = fabsf((s.corner[1][1] - s.corner[0][1]) * s.scale[1]);
    RayLamp made;

    memcpy(made.at, at, sizeof made.at);
    made.radius = (wide + high) / 4.0F;
    if (!(s.flags & SA_SPRITE_SCALE)) {
        const float *w = s.object_to_world;

        made.radius *= sqrtf(w[0] * w[0] + w[4] * w[4] + w[8] * w[8]);
    }
    for (int k = 0; k < 3; k++) {
        int shift = 16 - k * 8;

        made.color[k] =
            GLARE * linear(color >> shift) * linear(texel >> shift) * share;
    }
    add_lamp(made);
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
    uint32_t pal = shade.palette & 7;
    float origin[3] = {w[3], w[7], w[11]};
    float draw_z = view_z(origin);
    uint32_t offsets =
        offset_palette(shade, materials) * SA_PALETTE_ENTRIES * 2 + 1;

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
    /* SA1's shade: a meshset under SA_MAT_WHITE leaves the draw's points
     * white for itself and each meshset after it */
    bool white = false;

    for (Meshsets set(mesh); set.next();) {
        const sa_meshset &head = set.head;
        const uint16_t *corners = set.corners;
        const float *uv = set.uv;
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
        state.sample = sample_of(mat);
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
    if (tracing_ && !screen && blended && s.dst == SA_BLEND_ONE) {
        lamp(s, texture, world);
    }
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
    if (!tracing_) {
        return;
    }
    /* camera rays leave the eye of the view the batches are drawn through.
     * a view with no inverse looks from the world's origin */
    static const float origin[12] = {1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F,
                                     0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F};
    double inverse[12];
    bool turned = affine_inverse(look(), inverse);

    for (int i = 0; i < 12; i++) {
        rays_.eye[i] = turned ? static_cast<float>(inverse[i]) : origin[i];
    }
    if (memory_ != nullptr) {
        const float project[4] = {
            picture_.view.focal[0], picture_.view.focal[1],
            picture_.view.center[0], picture_.view.center[1]};

        std::sort(drawn_.begin(), drawn_.end());
        memory_->hold(rays_.lamps, rays_.patches, lamp_owners_, drawn_, look(),
                      project, learns_);
    }
    if (rays_.lamps.size() <= RAY_LAMPS) {
        return;
    }
    /* a picture that makes more lamps than it holds keeps the nearest to the
     * eye */
    const float *eye = rays_.eye;
    auto far = [eye](const RayLamp &l) {
        float to[3] = {l.at[0] - eye[3], l.at[1] - eye[7], l.at[2] - eye[11]};

        return (to[0] * to[0]) + (to[1] * to[1]) + (to[2] * to[2]);
    };

    std::nth_element(
        rays_.lamps.begin(), rays_.lamps.begin() + RAY_LAMPS, rays_.lamps.end(),
        [&far](const RayLamp &a, const RayLamp &b) { return far(a) < far(b); });
    rays_.lamps.resize(RAY_LAMPS);
}

void Scene::pair(RayPast &past)
{
    if (tracing_) {
        const float project[4] = {
            picture_.view.focal[0], picture_.view.focal[1],
            picture_.view.center[0], picture_.view.center[1]};

        past.pair(rays_, look(), project);
    }
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
