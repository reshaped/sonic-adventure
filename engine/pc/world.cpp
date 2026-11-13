#include "world.h"

#include <math.h>
#include <string.h>

#include <algorithm>

#include "sa_sa1.h"

namespace pc {

namespace {

/* the most draws, textures and points a picture keeps. a picture that
 * passes one keeps none, and Snapshot::full names the limit as SA_PC_FULL_*.
 * a kept draw's state, matrix and shade are 16-bit indices. their tables gain
 * at most an entry a draw */
constexpr uint32_t MOST_KEPT = 1U << 16;
constexpr uint32_t MOST_TEXTURES = 1U << 18;
constexpr uint32_t MOST_POINTS = 1U << 18;
constexpr uint32_t MOST_ENDED = 4096; /* tasks ending while a picture is open */
/* the most instances the scene keeps, past which more are left out */
constexpr uint32_t MOST_RESIDENTS = 1U << 16;
constexpr uint32_t STORE = 64U << 20; /* the most bytes store_ holds */
/* the most points of a mesh the game's code wrote that moves between two
 * pictures. scratch_ holds 3 floats of each point, then 3 of each normal */
constexpr uint32_t MESH_POINTS = 65536;
/* more steps than this between two pictures is no motion to show */
constexpr uint32_t MOST_STEPS = 8;
/* the steps a picture spans at the game's own 30 a second */
constexpr uint32_t FRAME_STEPS = 2;
/* no draw of a picture's */
constexpr uint32_t NONE = UINT32_MAX;
/* the dot of two rotations so near that a line serves for the arc between
 * them */
constexpr double NEAR_DOT = 0.9995;
/* a full turn of a sprite's angle, in radians */
constexpr float FULL_TURN = 6.28318531F;

/* a store's entries hold no padding */
static_assert(sizeof(MeshView) == (3 * sizeof(void *)) + 16, "MeshView");
static_assert(sizeof(KeptMesh) == sizeof(MeshView) + sizeof(void *) + 8,
              "KeptMesh");
static_assert(sizeof(KeptDraw) == 20, "KeptDraw");
static_assert(sizeof(KeptShade) == (2 * sizeof(void *)) + sizeof(sa_shade) + 4,
              "KeptShade");
static_assert(sizeof(KeptSprite) == 76, "KeptSprite");
static_assert(sizeof(KeptTriangles) == 16, "KeptTriangles");

bool before(const Key &a, const Key &b)
{
    if (a.instance != b.instance) {
        return a.instance < b.instance;
    }
    if (a.kind != b.kind) {
        return a.kind < b.kind;
    }
    if (a.named != b.named) {
        return a.named < b.named;
    }
    if (a.asset != b.asset) {
        return a.asset < b.asset;
    }
    return a.part < b.part;
}

bool same(const Key &a, const Key &b)
{
    return a.instance == b.instance && a.kind == b.kind && a.named == b.named &&
           a.asset == b.asset && a.part == b.part;
}

/* whether a draw's mesh is a writer's, one the game's code wrote. it is a
 * payload or a resource flagged SA_DRAW_WRITTEN */
bool written(sa_handle mesh, uint32_t flags)
{
    return mesh == SA_NULL || (flags & SA_DRAW_WRITTEN) != 0;
}

/* every draw no task owns counts as one drawer's, SA_INSTANCE_UNOWNED */
uint32_t owner(uint32_t instance)
{
    return instance & SA_INSTANCE_UNOWNED ? SA_INSTANCE_UNOWNED : instance;
}

/* whether instance names a scene's part */
bool of_scene(uint32_t instance)
{
    return (instance & (SA_INSTANCE_UNOWNED | SA_INSTANCE_SCENE)) ==
           SA_INSTANCE_SCENE;
}

/* whether key's asset and part are the thing and nth a mesh's draw gave.
 * World::draw keeps them unless the draw names no thing or a scene's part */
bool given(const Key &key)
{
    return key.named != 0;
}

/* the bytes a mesh's view reads, from its head to its last meshset's end */
size_t extent(const MeshView &mesh, const uint8_t *&first)
{
    const uint8_t *at = mesh.sets;

    first = reinterpret_cast<const uint8_t *>(mesh.points) - sizeof(sa_mesh);
    for (uint32_t set = 0; set < mesh.meshsets; set++) {
        sa_meshset head;

        memcpy(&head, at, sizeof head);
        at += meshset_bytes(head);
    }
    return static_cast<size_t>(at - first);
}

/* a writer's mesh's name, a hash of its points' count and its meshsets'
 * materials and corners. posing leaves these alone, so each pose of one model
 * has one name. the name is never 0 */
uint32_t shape_of(const MeshView &mesh)
{
    uint32_t id = 2166136261U;
    auto mix = [&id](uint32_t word) {
        for (int i = 0; i < 4; i++) {
            id = (id ^ ((word >> (i * 8)) & 0xff)) * 16777619U;
        }
    };
    const uint8_t *at = mesh.sets;

    mix(mesh.count);
    mix(mesh.meshsets);
    for (uint32_t set = 0; set < mesh.meshsets; set++) {
        sa_meshset head;

        memcpy(&head, at, sizeof head);
        mix(head.material);
        mix(head.corners);
        for (uint32_t c = 0; c < head.corners; c++) {
            uint16_t corner;

            memcpy(&corner, at + sizeof head + (c * 2), sizeof corner);
            mix(corner);
        }
        at += meshset_bytes(head);
    }
    return id != 0 ? id : 1;
}

/* the index of table's entry for e. only the last entry is reused, where
 * its bytes are e's */
template <typename T> uint16_t entry(std::vector<T> &table, const T &e)
{
    if (table.empty() || memcmp(&table.back(), &e, sizeof e) != 0) {
        table.push_back(e);
    }
    return static_cast<uint16_t>(table.size() - 1);
}

/* the same for a matrix, 12 floats an entry */
uint16_t matrix_entry(std::vector<float> &table, const float *m)
{
    size_t n = table.size();

    if (n == 0 || memcmp(&table[n - 12], m, sizeof(float) * 12) != 0) {
        table.insert(table.end(), m, m + 12);
    }
    return static_cast<uint16_t>((table.size() / 12) - 1);
}

/* whether s's last mesh is m, with the same MeshView, materials, name and
 * textures. a draw of m then shares that entry and adds no textures */
bool last_mesh(const Store &s, const KeptMesh &m, const uint32_t *textures)
{
    if (s.meshes.empty()) {
        return false;
    }
    const KeptMesh &last = s.meshes.back();

    return memcmp(&last.mesh, &m.mesh, sizeof m.mesh) == 0 &&
           last.materials == m.materials && last.name == m.name &&
           memcmp(s.textures.data() + last.textures, textures,
                  m.mesh.materials * sizeof *textures) == 0;
}

/* whether the resource named name went after the picture's draw i. marks
 * lie in their names' order, and a name's last mark is its latest */
bool went(const std::vector<Release> &marks, uint32_t name, uint32_t i)
{
    auto at = std::upper_bound(
        marks.begin(), marks.end(), name,
        [](uint32_t n, const Release &r) { return n < r.name; });

    return at != marks.begin() && (at - 1)->name == name &&
           i < (at - 1)->before;
}

/* kept draw i's matrix, object to world */
const float *matrix_of(const Snapshot &s, uint32_t i)
{
    return &s.store.matrices[s.kept[i].matrix * 12U];
}

/* the record kept draw i stands for, here and in the two below */
sa_draw draw_of(const Snapshot &s, uint32_t i)
{
    const sa_draw_kept &k = s.kept[i];
    const KeptDraw &e = s.store.draws[k.state];
    sa_draw d{};

    d.head = {SA_REC_DRAW, sizeof d};
    d.mesh = e.mesh;
    d.materials = e.materials;
    d.instance = s.instances[i];
    if (given(s.keys[i])) {
        d.thing = s.keys[i].asset;
        d.nth = s.keys[i].part;
    }
    d.generation = e.generation;
    d.flags = static_cast<uint16_t>(k.attr);
    d.tint = e.tint;
    d.payload = e.payload;
    d.shade = s.store.shades[k.shade].shade.shade;
    memcpy(d.object_to_world, matrix_of(s, i), sizeof d.object_to_world);
    return d;
}

sa_sprite sprite_of(const Snapshot &s, uint32_t i)
{
    const sa_draw_kept &k = s.kept[i];
    const KeptSprite &e = s.store.sprites[k.state];
    sa_sprite d{};

    d.head = {SA_REC_SPRITE, sizeof d};
    d.texture = k.geometry;
    d.instance = s.instances[i];
    memcpy(d.object_to_world, matrix_of(s, i), sizeof d.object_to_world);
    memcpy(d.at, e.at, sizeof d.at);
    memcpy(d.scale, e.scale, sizeof d.scale);
    memcpy(d.corner, e.corner, sizeof d.corner);
    memcpy(d.uv, e.uv, sizeof d.uv);
    d.angle = e.angle;
    d.bias = e.bias;
    d.color = e.color;
    d.flags = static_cast<uint8_t>(k.attr);
    d.depth = e.depth;
    d.fog = e.fog;
    d.src = e.src;
    d.dst = e.dst;
    d.filter = e.filter;
    return d;
}

sa_triangles triangles_of(const Snapshot &s, uint32_t i)
{
    const sa_draw_kept &k = s.kept[i];
    const KeptTriangles &e = s.store.triangles[k.state];
    sa_triangles d{};

    d.head = {SA_REC_TRIANGLES, sizeof d};
    d.texture = e.texture;
    d.instance = s.instances[i];
    memcpy(d.object_to_world, matrix_of(s, i), sizeof d.object_to_world);
    d.count = e.count;
    d.form = e.form;
    d.flags = static_cast<uint8_t>(k.attr);
    d.depth = e.depth;
    d.fog = e.fog;
    d.src = e.src;
    d.dst = e.dst;
    d.filter = e.filter;
    return d;
}

/* let go of a picture's draws and all they name, its room kept */
void empty(Snapshot &s)
{
    Store &t = s.store;

    s.kept.clear();
    s.instances.clear();
    s.keys.clear();
    s.groups.clear();
    s.by_key.clear();
    s.ended.clear();
    s.released.clear();
    s.released_textures.clear();
    s.scene_gone = 0;
    t.draws.clear();
    t.sprites.clear();
    t.triangles.clear();
    t.shades.clear();
    t.meshes.clear();
    t.matrices.clear();
    t.textures.clear();
    t.points.clear();
}

/* whether a picture shows kept draw i. a mesh the game's view test left out
 * shows nothing, though the scene keeps its instance */
bool shows(const Snapshot &s, uint32_t i)
{
    return s.kept[i].kind != KEPT_MESH || (s.kept[i].attr & SA_DRAW_OUT) == 0;
}

float lerp(float a, float b, float alpha)
{
    return a + ((b - a) * alpha);
}

/* a 3 by 4 matrix by rows as rotation, scale along its own axes and
 * translation */
struct Pose {
    double q[4]; /* w, x, y, z */
    double scale[3];
    double at[3];
};

/* false where m is no Pose, as a shear or a degenerate axis */
bool pose_of(const float *m, Pose &p)
{
    double r[3][3];

    for (int j = 0; j < 3; j++) {
        double x = m[j];
        double y = m[4 + j];
        double z = m[8 + j];
        double length = sqrt((x * x) + (y * y) + (z * z));

        if (!(length > 1e-12) || !isfinite(length)) {
            return false;
        }
        p.scale[j] = length;
        r[0][j] = x / length;
        r[1][j] = y / length;
        r[2][j] = z / length;
    }
    for (int i = 0; i < 3; i++) {
        p.at[i] = m[(i * 4) + 3];
    }
    double det = (r[0][0] * ((r[1][1] * r[2][2]) - (r[1][2] * r[2][1]))) -
                 (r[0][1] * ((r[1][0] * r[2][2]) - (r[1][2] * r[2][0]))) +
                 (r[0][2] * ((r[1][0] * r[2][1]) - (r[1][1] * r[2][0])));
    /* a mirror: the first axis turns over, and its scale with it */
    if (det < 0) {
        p.scale[0] = -p.scale[0];
        for (int i = 0; i < 3; i++) {
            r[i][0] = -r[i][0];
        }
    }
    for (int a = 0; a < 3; a++) {
        for (int b = a + 1; b < 3; b++) {
            double dot =
                (r[0][a] * r[0][b]) + (r[1][a] * r[1][b]) + (r[2][a] * r[2][b]);
            if (fabs(dot) > 1e-3) {
                return false;
            }
        }
    }
    double trace = r[0][0] + r[1][1] + r[2][2];
    double *q = p.q;

    if (trace > 0) {
        double s = sqrt(trace + 1.0) * 2.0;
        q[0] = 0.25 * s;
        q[1] = (r[2][1] - r[1][2]) / s;
        q[2] = (r[0][2] - r[2][0]) / s;
        q[3] = (r[1][0] - r[0][1]) / s;
    } else if (r[0][0] > r[1][1] && r[0][0] > r[2][2]) {
        double s = sqrt(1.0 + r[0][0] - r[1][1] - r[2][2]) * 2.0;
        q[0] = (r[2][1] - r[1][2]) / s;
        q[1] = 0.25 * s;
        q[2] = (r[0][1] + r[1][0]) / s;
        q[3] = (r[0][2] + r[2][0]) / s;
    } else if (r[1][1] > r[2][2]) {
        double s = sqrt(1.0 + r[1][1] - r[0][0] - r[2][2]) * 2.0;
        q[0] = (r[0][2] - r[2][0]) / s;
        q[1] = (r[0][1] + r[1][0]) / s;
        q[2] = 0.25 * s;
        q[3] = (r[1][2] + r[2][1]) / s;
    } else {
        double s = sqrt(1.0 + r[2][2] - r[0][0] - r[1][1]) * 2.0;
        q[0] = (r[1][0] - r[0][1]) / s;
        q[1] = (r[0][2] + r[2][0]) / s;
        q[2] = (r[1][2] + r[2][1]) / s;
        q[3] = 0.25 * s;
    }
    return true;
}

void matrix_of(const Pose &p, float *m)
{
    double w = p.q[0];
    double x = p.q[1];
    double y = p.q[2];
    double z = p.q[3];
    double r[3][3] = {{1 - (2 * ((y * y) + (z * z))), 2 * ((x * y) - (w * z)),
                       2 * ((x * z) + (w * y))},
                      {2 * ((x * y) + (w * z)), 1 - (2 * ((x * x) + (z * z))),
                       2 * ((y * z) - (w * x))},
                      {2 * ((x * z) - (w * y)), 2 * ((y * z) + (w * x)),
                       1 - (2 * ((x * x) + (y * y)))}};

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            m[(i * 4) + j] = static_cast<float>(r[i][j] * p.scale[j]);
        }
        m[(i * 4) + 3] = static_cast<float>(p.at[i]);
    }
}

/* the inverse of a 3 by 4 matrix by rows, false where it has none */
bool invert(const float *m, float *out)
{
    double inverse[12];

    if (!affine_inverse(m, inverse)) {
        return false;
    }
    for (int i = 0; i < 12; i++) {
        out[i] = static_cast<float>(inverse[i]);
    }
    return true;
}

/* whether two views are one matrix */
bool same_matrix(const sa_view &a, const sa_view &b)
{
    return memcmp(a.world_to_view, b.world_to_view, sizeof a.world_to_view) ==
           0;
}

/* whether kept draw i lies on the screen, where no view places it */
bool on_screen(const Snapshot &s, uint32_t i)
{
    const sa_draw_kept &k = s.kept[i];

    return (k.kind == KEPT_SPRITE && (k.attr & SA_SPRITE_SCREEN) != 0) ||
           (k.kind == KEPT_TRIANGLES && (k.attr & SA_TRIANGLES_SCREEN) != 0);
}

} // namespace

void blend_transform(const float *a, const float *b, float alpha, float *out)
{
    Pose pa;
    Pose pb;

    if (!pose_of(a, pa) || !pose_of(b, pb) ||
        (pa.scale[0] < 0) != (pb.scale[0] < 0)) {
        for (int i = 0; i < 12; i++) {
            out[i] = lerp(a[i], b[i], alpha);
        }
        return;
    }
    double dot = (pa.q[0] * pb.q[0]) + (pa.q[1] * pb.q[1]) +
                 (pa.q[2] * pb.q[2]) + (pa.q[3] * pb.q[3]);
    if (dot < 0) {
        dot = -dot;
        for (double &v : pb.q) {
            v = -v;
        }
    }
    double ka = 1.0 - alpha;
    double kb = alpha;

    if (dot < NEAR_DOT) {
        double theta = acos(dot);
        double sine = sin(theta);

        ka = sin((1.0 - alpha) * theta) / sine;
        kb = sin(alpha * theta) / sine;
    }
    Pose p;
    double length = 0;

    for (int i = 0; i < 4; i++) {
        p.q[i] = (ka * pa.q[i]) + (kb * pb.q[i]);
        length += p.q[i] * p.q[i];
    }
    length = sqrt(length);
    for (double &v : p.q) {
        v /= length;
    }
    for (int i = 0; i < 3; i++) {
        p.scale[i] = pa.scale[i] + ((pb.scale[i] - pa.scale[i]) * alpha);
        p.at[i] = pa.at[i] + ((pb.at[i] - pa.at[i]) * alpha);
    }
    matrix_of(p, out);
}

void blend_view(const float *a, const float *b, float alpha, float *out)
{
    float eye_a[12];
    float eye_b[12];
    float eye[12];

    if (!invert(a, eye_a) || !invert(b, eye_b)) {
        memcpy(out, b, sizeof(float) * 12);
        return;
    }
    blend_transform(eye_a, eye_b, alpha, eye);
    if (!invert(eye, out)) {
        memcpy(out, b, sizeof(float) * 12);
    }
}

void World::open(uint32_t releases)
{
    for (Snapshot *s : {&before_, &now_}) {
        Store &t = s->store;

        s->kept.reserve(MOST_KEPT);
        s->instances.reserve(MOST_KEPT);
        s->keys.reserve(MOST_KEPT);
        s->groups.reserve(MOST_KEPT);
        s->by_key.reserve(MOST_KEPT);
        s->ended.reserve(MOST_ENDED);
        s->released.reserve(releases);
        s->released_textures.reserve(releases);
        t.draws.reserve(MOST_KEPT);
        t.sprites.reserve(MOST_KEPT);
        t.triangles.reserve(MOST_KEPT);
        t.shades.reserve(MOST_KEPT);
        t.meshes.reserve(MOST_KEPT);
        t.matrices.reserve(MOST_KEPT * 12);
        t.textures.reserve(MOST_TEXTURES);
        t.points.reserve(MOST_POINTS);
    }
    residents_.reserve(MOST_RESIDENTS);
    merged_.reserve(MOST_RESIDENTS);
    store_.reserve(STORE);
    spare_.reserve(STORE);
    scratch_.resize(MESH_POINTS * 6);
    scratch_points_.resize(MOST_POINTS);
}

void World::begin(const sa_picture &picture, uint64_t serial)
{
    std::swap(before_, now_);
    now_.valid = false;
    now_.full = 0;
    now_.serial = serial;
    now_.picture = picture;
    now_.views[0] = picture.view;
    now_.fog = sa_fog{};
    now_.palettes = nullptr;
    empty(now_);
    open_ = true;
}

void World::forget()
{
    for (Snapshot *s : {&before_, &now_}) {
        s->valid = false;
        s->full = 0;
        empty(*s);
    }
}

/* whether the open picture has room for a draw, and its store for textures
 * and points more. the first draw that finds none fills the picture, which
 * then takes no draw */
bool World::room(uint32_t textures, uint32_t points)
{
    if (!open_ || now_.full != 0) {
        return false;
    }
    if (now_.kept.size() == MOST_KEPT) {
        now_.full = SA_PC_FULL_DRAWS;
    } else if (now_.store.textures.size() + textures > MOST_TEXTURES) {
        now_.full = SA_PC_FULL_TEXTURES;
    } else if (now_.store.points.size() + points > MOST_POINTS) {
        now_.full = SA_PC_FULL_POINTS;
    }
    return now_.full == 0;
}

/* keep k under key as instance's draw. matrix is its object to world */
void World::add(sa_draw_kept &k, uint32_t instance, const Key &key,
                const float *matrix)
{
    k.matrix = matrix_entry(now_.store.matrices, matrix);
    now_.kept.push_back(k);
    now_.instances.push_back(instance);
    now_.keys.push_back(key);
}

void World::draw(const sa_draw &d, const MeshView &mesh,
                 const sa_material *materials, const uint32_t *textures,
                 const sa_shade &shade, const sa_lights *lights)
{
    Store &s = now_.store;
    KeptMesh m{};
    KeptDraw e{};
    KeptShade lit{};
    sa_draw_kept k{};
    Key key;

    m.mesh = mesh;
    m.materials = materials;
    m.name = written(d.mesh, d.flags) ? shape_of(mesh) : d.mesh;
    bool again = last_mesh(s, m, textures);

    if (!room(again ? 0 : mesh.materials, 0)) {
        return;
    }
    if (!again) {
        m.textures = static_cast<uint32_t>(s.textures.size());
        s.textures.insert(s.textures.end(), textures,
                          textures + mesh.materials);
        s.meshes.push_back(m);
    }
    e.mesh = d.mesh;
    e.materials = d.materials;
    e.payload = d.payload;
    e.tint = d.tint;
    e.generation = d.generation;
    lit.lights = lights;
    lit.palettes = now_.palettes;
    lit.shade = shade;
    k.kind = KEPT_MESH;
    k.view = d.view;
    k.state = entry(s.draws, e);
    k.shade = entry(s.shades, lit);
    k.attr = d.flags;
    k.geometry = static_cast<uint32_t>(s.meshes.size() - 1);
    key.kind = KEPT_MESH;
    key.instance = owner(d.instance);
    /* a scene's part is keyed by its instance alone. another mesh is keyed
     * by the thing and nth its draw names. one whose draw names no thing is
     * keyed by its mesh's name, and index gives its part */
    if (d.thing != 0 && !of_scene(d.instance)) {
        key.named = 1;
        key.asset = d.thing;
        key.part = d.nth;
    } else if (!of_scene(d.instance)) {
        key.asset = m.name;
    }
    add(k, d.instance, key, d.object_to_world);
}

void World::sprite(const sa_sprite &s, uint32_t texture)
{
    if (!room(0, 0)) {
        return;
    }
    KeptSprite e{};
    sa_draw_kept k{};
    Key key;

    memcpy(e.at, s.at, sizeof e.at);
    memcpy(e.scale, s.scale, sizeof e.scale);
    memcpy(e.corner, s.corner, sizeof e.corner);
    memcpy(e.uv, s.uv, sizeof e.uv);
    e.angle = s.angle;
    e.bias = s.bias;
    e.color = s.color;
    e.raster = texture;
    e.depth = s.depth;
    e.fog = s.fog;
    e.src = s.src;
    e.dst = s.dst;
    e.filter = s.filter;
    k.kind = KEPT_SPRITE;
    k.view = s.view;
    k.state = entry(now_.store.sprites, e);
    k.attr = s.flags;
    k.geometry = s.texture;
    key.kind = KEPT_SPRITE;
    key.instance = owner(s.instance);
    key.asset = s.flags & SA_SPRITE_SCREEN ? 1 : 0;
    add(k, s.instance, key, s.object_to_world);
}

void World::triangles(const sa_triangles &t, const sa_point *points,
                      uint32_t count, uint32_t texture)
{
    if (!room(0, count)) {
        return;
    }
    Store &s = now_.store;
    KeptTriangles e{};
    sa_draw_kept k{};
    Key key;

    e.texture = t.texture;
    e.raster = texture;
    e.count = t.count;
    e.form = t.form;
    e.depth = t.depth;
    e.fog = t.fog;
    e.src = t.src;
    e.dst = t.dst;
    e.filter = t.filter;
    k.kind = KEPT_TRIANGLES;
    k.view = t.view;
    k.state = entry(s.triangles, e);
    k.attr = t.flags;
    k.geometry = static_cast<uint32_t>(s.points.size());
    s.points.insert(s.points.end(), points, points + count);
    key.kind = KEPT_TRIANGLES;
    key.instance = owner(t.instance);
    add(k, t.instance, key, t.object_to_world);
}

void World::ended(uint32_t instance)
{
    if (open_ && now_.ended.size() < MOST_ENDED) {
        now_.ended.push_back(instance);
    } else if (open_ && now_.full == 0) {
        now_.full = SA_PC_FULL_ENDED;
    }
    residents_.erase(std::remove_if(residents_.begin(), residents_.end(),
                                    [instance](const Resident &r) {
                                        return r.key.instance == instance;
                                    }),
                     residents_.end());
}

void World::released(sa_handle handle, uint32_t texture)
{
    /* the open picture's draws so far may name it. fold and keep read the
     * marks */
    if (open_) {
        auto before = static_cast<uint32_t>(now_.kept.size());

        now_.released.push_back({handle, before});
        if (texture != 0) {
            now_.released_textures.push_back({texture, before});
        }
    }
    residents_.erase(std::remove_if(residents_.begin(), residents_.end(),
                                    [handle](const Resident &r) {
                                        return r.mesh == handle ||
                                               r.materials == handle;
                                    }),
                     residents_.end());
    if (texture == 0) {
        return;
    }
    /* a resident that names the texture isn't drawn again until a picture
     * draws it. the raster may give the texture's place to another */
    for (Resident &r : residents_) {
        if (!(r.flags & RESIDENT_SHAPE)) {
            continue;
        }
        for (uint32_t i = 0; i < r.material_count; i++) {
            uint32_t named;

            memcpy(&named, store_.data() + r.textures + (i * sizeof named),
                   sizeof named);
            if (named == texture) {
                r.flags &= static_cast<uint8_t>(~RESIDENT_SHAPE);
                break;
            }
        }
    }
}

/* sort s's draws by key. a draw's group is the draws whose keys differ only
 * in part, and groups holds its size. where given(key) is false, a draw's
 * part is its place among its group's draws in the picture */
void World::index(Snapshot &s)
{
    std::vector<uint32_t> &order = s.by_key;

    order.clear();
    for (uint32_t i = 0; i < s.kept.size(); i++) {
        order.push_back(i);
    }
    std::sort(order.begin(), order.end(), [&s](uint32_t a, uint32_t b) {
        Key kx = s.keys[a];
        Key ky = s.keys[b];

        if (!given(kx)) {
            kx.part = a;
        }
        if (!given(ky)) {
            ky.part = b;
        }
        return before(kx, ky);
    });
    s.groups.assign(s.kept.size(), 0);
    for (size_t first = 0; first < order.size();) {
        size_t last = first;
        const Key &k = s.keys[order[first]];

        while (last < order.size() &&
               s.keys[order[last]].instance == k.instance &&
               s.keys[order[last]].kind == k.kind &&
               s.keys[order[last]].named == k.named &&
               s.keys[order[last]].asset == k.asset) {
            last++;
        }
        for (size_t i = first; i < last; i++) {
            if (!given(k)) {
                s.keys[order[i]].part = static_cast<uint32_t>(i - first);
            }
            s.groups[order[i]] = static_cast<uint32_t>(last - first);
        }
        first = last;
    }
    std::sort(s.ended.begin(), s.ended.end());
    for (std::vector<Release> *marks : {&s.released, &s.released_textures}) {
        std::sort(marks->begin(), marks->end(),
                  [](const Release &a, const Release &b) {
                      return a.name != b.name ? a.name < b.name
                                              : a.before < b.before;
                  });
    }
}

void World::end()
{
    if (!open_) {
        return;
    }
    open_ = false;
    now_.valid = true;
    /* a picture that filled a store keeps no draws and isn't folded. the
     * instances stay as the picture before left them */
    if (now_.full != 0) {
        empty(now_);
        return;
    }
    index(now_);
    fold();
}

/* the draw of s under key, or NONE */
uint32_t World::find(const Snapshot &s, const Key &key) const
{
    auto at = std::lower_bound(
        s.by_key.begin(), s.by_key.end(), key,
        [&s](uint32_t i, const Key &k) { return before(s.keys[i], k); });

    if (at == s.by_key.end() || !same(s.keys[*at], key)) {
        return NONE;
    }
    return *at;
}

bool World::drew(const Snapshot &s, uint32_t instance) const
{
    Key k;

    k.instance = instance;
    auto at = std::lower_bound(
        s.by_key.begin(), s.by_key.end(), k,
        [&s](uint32_t i, const Key &key) { return before(s.keys[i], key); });
    return at != s.by_key.end() && s.keys[*at].instance == instance;
}

/* fold the open picture's draws into the instances kept. a drawn instance
 * takes its new transform and keeps the one before. one the picture missed
 * stays only while it holds RESIDENT_SHAPE. a draw whose task ended while the
 * picture was open leaves no instance */
void World::fold()
{
    size_t r = 0;

    if (store_.size() > STORE / 2) {
        compact();
    }
    size_t k = 0;
    bool cut = (now_.picture.flags & SA_PICTURE_CUT) != 0;

    merged_.clear();
    while (r < residents_.size() || k < now_.by_key.size()) {
        uint32_t i = k < now_.by_key.size() ? now_.by_key[k] : NONE;
        const Key *key = i != NONE ? &now_.keys[i] : nullptr;
        Resident *old = r < residents_.size() ? &residents_[r] : nullptr;

        if (merged_.size() == MOST_RESIDENTS) {
            break;
        }
        if (key == nullptr || (old != nullptr && before(old->key, *key))) {
            if (old->flags & RESIDENT_SHAPE) {
                Resident kept = *old;

                kept.flags &= static_cast<uint8_t>(~RESIDENT_SEEN);
                merged_.push_back(kept);
            }
            r++;
            continue;
        }
        if (std::binary_search(now_.ended.begin(), now_.ended.end(),
                               key->instance)) {
            k++;
            continue;
        }
        /* only the picture's first draw of a scene's part places the part,
         * which stays one instance. a draw taken before its scene ended
         * places nothing */
        if (of_scene(key->instance) &&
            (key->part != 0 || i < now_.scene_gone)) {
            k++;
            continue;
        }
        Resident made;
        const sa_draw_kept &kept = now_.kept[i];
        const float *w = matrix_of(now_, i);
        bool mesh = kept.kind == KEPT_MESH;
        const KeptDraw *d = mesh ? &now_.store.draws[kept.state] : nullptr;
        bool payload = !mesh || written(d->mesh, kept.attr);
        sa_handle materials = mesh ? d->materials : SA_NULL;
        uint16_t generation = mesh ? d->generation : 0;

        /* a draw whose mesh or materials went after it makes no instance.
         * one already under its key outlived the release, so it stays as one
         * the picture missed */
        if (mesh && (went(now_.released, payload ? SA_NULL : d->mesh, i) ||
                     went(now_.released, materials, i))) {
            k++;
            continue;
        }
        bool held = old != nullptr && same(old->key, *key);

        if (held) {
            made = *old;
            r++;
        }
        memcpy(made.before, held ? made.now : w, sizeof made.before);
        memcpy(made.now, w, sizeof made.now);
        made.key = *key;
        /* a part held a mesh and materials before its first draw, which
         * may change them */
        if ((made.seen != 0 || (made.flags & RESIDENT_PART) != 0) &&
            (made.mesh != (payload ? SA_NULL : d->mesh) ||
             made.materials != materials)) {
            made.asset_version++;
        }
        made.mesh = payload ? SA_NULL : d->mesh;
        made.materials = materials;
        if (payload && made.seen != 0) {
            made.shape_version++;
        }
        made.flags = static_cast<uint8_t>(
            (made.flags & RESIDENT_PART) | RESIDENT_SEEN |
            (shows(now_, i) ? 0 : RESIDENT_OUT) |
            (cut || (made.seen != 0 && made.generation != generation)
                 ? RESIDENT_BROKE
                 : 0));
        made.generation = generation;
        made.seen = now_.serial;
        if (mesh && keep(made, i)) {
            made.flags |= RESIDENT_SHAPE;
        }
        merged_.push_back(made);
        k++;
    }
    std::swap(residents_, merged_);
}

/* keep in r what the open picture's draw i needs to be drawn again. it
 * returns false where one of its textures went after the draw, or the store
 * has no room */
bool World::keep(Resident &r, uint32_t i)
{
    const sa_draw_kept &k = now_.kept[i];
    const KeptMesh &m = now_.store.meshes[k.geometry];
    const uint32_t *textures = now_.store.textures.data() + m.textures;

    for (uint32_t t = 0;
         t < m.mesh.materials && !now_.released_textures.empty(); t++) {
        if (went(now_.released_textures, textures[t], i)) {
            return false;
        }
    }
    return hold(r, m.mesh, written(now_.store.draws[k.state].mesh, k.attr),
                m.materials, textures, now_.store.shades[k.shade].shade);
}

/* the same from a mesh, its materials, their textures and a shade. a
 * writer's mesh is copied where its bytes changed, and a resource's is lent.
 * it returns false where the store has no room */
bool World::hold(Resident &r, const MeshView &mesh, bool copied,
                 const sa_material *materials, const uint32_t *textures,
                 const sa_shade &shade)
{
    size_t texture_bytes = mesh.materials * sizeof(uint32_t);

    r.material_list = materials;
    r.material_count = mesh.materials;
    r.shade = shade;
    if (!copied) {
        r.lent = mesh;
        r.size = 0;
    } else {
        const uint8_t *first = nullptr;
        size_t size = extent(mesh, first);

        if (r.size != size ||
            memcmp(store_.data() + r.bytes, first, size) != 0) {
            if (store_.size() + size + texture_bytes > STORE) {
                return false;
            }
            r.bytes = static_cast<uint32_t>(store_.size());
            r.size = static_cast<uint32_t>(size);
            store_.insert(store_.end(), first, first + size);
        }
    }
    if (store_.size() + texture_bytes > STORE) {
        return false;
    }
    const auto *from = reinterpret_cast<const uint8_t *>(textures);
    r.textures = static_cast<uint32_t>(store_.size());
    store_.insert(store_.end(), from, from + texture_bytes);
    return true;
}

uint64_t World::part_bytes(const MeshView &mesh, uint32_t flags)
{
    const uint8_t *first = nullptr;

    return ((flags & SA_DRAW_WRITTEN) != 0 ? extent(mesh, first) : 0) +
           (mesh.materials * sizeof(uint32_t));
}

bool World::room_for_parts(uint32_t count, uint64_t bytes) const
{
    return residents_.size() + count <= MOST_RESIDENTS &&
           store_.size() + bytes <= STORE;
}

void World::part(const sa_draw &d, const MeshView &mesh,
                 const sa_material *materials, const uint32_t *textures,
                 const sa_shade &shade)
{
    Resident made;
    bool copied = (d.flags & SA_DRAW_WRITTEN) != 0;

    made.key.kind = KEPT_MESH;
    made.key.instance = d.instance;
    auto at = std::lower_bound(
        residents_.begin(), residents_.end(), made.key,
        [](const Resident &r, const Key &k) { return before(r.key, k); });
    bool held = at != residents_.end() && same(at->key, made.key);

    if (held) {
        made = *at;
        made.asset_version++;
    }
    made.mesh = copied ? SA_NULL : d.mesh;
    made.materials = d.materials;
    made.generation = d.generation;
    memcpy(made.now, d.object_to_world, sizeof made.now);
    memcpy(made.before, d.object_to_world, sizeof made.before);
    made.flags = RESIDENT_PART;
    if (hold(made, mesh, copied, materials, textures, shade)) {
        made.flags |= RESIDENT_SHAPE;
    }
    if (held) {
        *at = made;
    } else {
        residents_.insert(at, made);
    }
}

void World::lighting(const sa_lighting &l)
{
    lighting_ = l;
    lit_ = true;
}

void World::scene_ended()
{
    lit_ = false;
    if (open_) {
        now_.scene_gone = static_cast<uint32_t>(now_.kept.size());
    }
    residents_.erase(std::remove_if(residents_.begin(), residents_.end(),
                                    [](const Resident &r) {
                                        return of_scene(r.key.instance);
                                    }),
                     residents_.end());
}

/* move the bytes of each resident that holds RESIDENT_SHAPE to a fresh
 * store_. what gone residents and changed meshes left behind stays out */
void World::compact()
{
    spare_.clear();
    for (Resident &r : residents_) {
        if (!(r.flags & RESIDENT_SHAPE)) {
            r.size = 0;
            continue;
        }
        size_t textures = r.material_count * sizeof(uint32_t);
        uint32_t at = static_cast<uint32_t>(spare_.size());

        spare_.insert(spare_.end(), store_.begin() + r.bytes,
                      store_.begin() + r.bytes + r.size);
        r.bytes = at;
        at = static_cast<uint32_t>(spare_.size());
        spare_.insert(spare_.end(), store_.begin() + r.textures,
                      store_.begin() + r.textures + textures);
        r.textures = at;
    }
    std::swap(store_, spare_);
}

uint32_t World::overview(const sa_picture &view, Scene &out)
{
    uint32_t drawn = 0;

    out.begin(view);
    out.palettes(now_.palettes);
    for (const Resident &r : residents_) {
        MeshView mesh = r.lent;

        /* SA_SHADE_PALETTE reads the palettes a picture brings, so a part
         * given before the first picture waits for one */
        if (!(r.flags & RESIDENT_SHAPE) ||
            (r.mesh == SA_NULL &&
             !mesh_view(store_.data() + r.bytes, r.size, mesh)) ||
            mesh.meshsets == 0 || !out.room(mesh.triangles) ||
            (r.shade.mode == SA_SHADE_PALETTE && !out.has_palettes())) {
            continue;
        }
        sa_draw d{};

        d.mesh = r.mesh;
        d.materials = r.materials;
        d.tint = 0xffffffffU;
        memcpy(d.object_to_world, r.now, sizeof d.object_to_world);
        out.draw(d, mesh, r.material_list,
                 reinterpret_cast<const uint32_t *>(store_.data() + r.textures),
                 r.shade);
        drawn++;
    }
    out.finish();
    return drawn;
}

uint32_t World::resting() const
{
    uint32_t n = 0;

    for (const Resident &r : residents_) {
        n += (r.flags & RESIDENT_SEEN) == 0 ? 1 : 0;
    }
    return n;
}

uint32_t World::lights() const
{
    return lit_ ? 1 + lighting_.count : 0;
}

uint32_t World::parts() const
{
    uint32_t n = 0;

    for (const Resident &r : residents_) {
        n += (r.flags & RESIDENT_PART) != 0 ? 1 : 0;
    }
    return n;
}

uint32_t World::outside() const
{
    uint32_t n = 0;

    for (const Resident &r : residents_) {
        n += (r.flags & RESIDENT_OUT) != 0 ? 1 : 0;
    }
    return n;
}

bool World::joined() const
{
    return before_.valid && now_.valid && before_.full == 0 && now_.full == 0 &&
           (now_.picture.flags & SA_PICTURE_CUT) == 0 &&
           now_.picture.tick > before_.picture.tick &&
           now_.picture.tick - before_.picture.tick <= MOST_STEPS;
}

uint32_t World::steps() const
{
    uint32_t gap = now_.picture.tick - before_.picture.tick;

    if (!before_.valid || !now_.valid || gap == 0 || gap > MOST_STEPS) {
        return FRAME_STEPS;
    }
    return gap;
}

/* the scene's index of the view s's draw i shows under. home is the view of
 * the picture being drawn. a draw moving from before_'s draw from takes the
 * view alpha of the way between theirs. one with no way stands as far from
 * home as its view lay from its picture's own. a draw on the screen, or under
 * its pictures' own views, shows under home, index 0 */
uint32_t World::lens(const Snapshot &s, uint32_t i, uint32_t from, float alpha,
                     const sa_view &home, Scene &out)
{
    uint32_t to = s.kept[i].view;
    uint32_t row = from != NONE  ? LENS_UNPAIRED_ROWS + before_.kept[from].view
                   : &s == &now_ ? 1
                                 : 0;
    uint32_t slot = ((SA_VIEWS + 1) * row) + to;
    const sa_view &mine = s.views[to];
    bool own = same_matrix(mine, s.picture.view);
    sa_view made = home;

    if (on_screen(s, i)) {
        return 0;
    }
    if (lenses_[slot] != NONE) {
        return lenses_[slot];
    }
    lenses_[slot] = 0;
    if (from != NONE) {
        const sa_view &was = before_.views[before_.kept[from].view];

        if (!own || !same_matrix(was, before_.picture.view)) {
            blend_view(was.world_to_view, mine.world_to_view, alpha,
                       made.world_to_view);
            lenses_[slot] = out.view(made);
        }
    } else if (!own) {
        double inverse[12];
        double first[12];
        double second[12];
        double product[12];

        memcpy(made.world_to_view, mine.world_to_view,
               sizeof made.world_to_view);
        if (!same_matrix(home, s.picture.view) &&
            affine_inverse(s.picture.view.world_to_view, inverse)) {
            for (int k = 0; k < 12; k++) {
                first[k] = home.world_to_view[k];
                second[k] = mine.world_to_view[k];
            }
            affine_product(first, inverse, product);
            affine_product(product, second, first);
            for (int k = 0; k < 12; k++) {
                made.world_to_view[k] = static_cast<float>(first[k]);
            }
        }
        lenses_[slot] = out.view(made);
    }
    return lenses_[slot];
}

/* draw s's draw i into out under the scene's view of index view. a draw on
 * its way from before_'s draw from moves alpha of the way. s is then the
 * newer picture. what moves on no line, as a texture's frame or a uv, comes
 * from the nearer picture. the older is the nearer up to half */
void World::put(const Snapshot &s, uint32_t i, uint32_t from, float alpha,
                uint32_t view, Scene &out)
{
    bool older = from != NONE && alpha <= 0.5F;
    const Snapshot &own = older ? before_ : s;
    uint32_t c = older ? from : i;
    const sa_draw_kept &kc = own.kept[c];

    switch (s.kept[i].kind) {
    case KEPT_MESH: {
        sa_draw d = draw_of(own, c);
        const KeptMesh &m = own.store.meshes[kc.geometry];
        MeshView mesh = m.mesh;

        if (from != NONE) {
            blend_transform(matrix_of(before_, from), matrix_of(s, i), alpha,
                            d.object_to_world);
            const MeshView &a =
                before_.store.meshes[before_.kept[from].geometry].mesh;
            const MeshView &b = s.store.meshes[s.kept[i].geometry].mesh;

            /* a writer's mesh drawn again with the same shape moves its
             * points between the two. its normals keep the newer length */
            if (written(s.store.draws[s.kept[i].state].mesh, s.kept[i].attr) &&
                a.count == b.count && a.count <= MESH_POINTS &&
                a.meshsets == b.meshsets && a.triangles == b.triangles &&
                (a.normals == nullptr) == (b.normals == nullptr)) {
                float *points = scratch_.data();
                float *normals = points + (MESH_POINTS * 3);

                for (uint32_t p = 0; p < b.count * 3; p++) {
                    points[p] = lerp(a.points[p], b.points[p], alpha);
                }
                mesh.points = points;
                if (b.normals != nullptr) {
                    for (uint32_t p = 0; p < b.count; p++) {
                        const float *na = &a.normals[p * 3];
                        const float *nb = &b.normals[p * 3];
                        float n[3];
                        float la = 0;
                        float lb = 0;

                        for (int j = 0; j < 3; j++) {
                            n[j] = lerp(na[j], nb[j], alpha);
                            la += n[j] * n[j];
                            lb += nb[j] * nb[j];
                        }
                        float scale = la > 0 ? sqrtf(lb / la) : 1.0F;

                        for (int j = 0; j < 3; j++) {
                            normals[(p * 3) + j] = n[j] * scale;
                        }
                    }
                    mesh.normals = normals;
                }
            }
        }
        if (mesh.meshsets != 0 && out.room(mesh.triangles)) {
            const KeptShade &lit = own.store.shades[kc.shade];

            out.palettes(lit.palettes);
            out.draw(d, mesh, m.materials,
                     own.store.textures.data() + m.textures, lit.shade,
                     lit.lights, view);
        } else if (mesh.meshsets != 0) {
            unplaced_++;
        }
        break;
    }
    case KEPT_SPRITE: {
        sa_sprite sprite = sprite_of(own, c);

        if (from != NONE) {
            sa_sprite a = sprite_of(before_, from);
            sa_sprite b = sprite_of(s, i);
            float turn = b.angle - a.angle;

            blend_transform(a.object_to_world, b.object_to_world, alpha,
                            sprite.object_to_world);
            for (int j = 0; j < 3; j++) {
                sprite.at[j] = lerp(a.at[j], b.at[j], alpha);
            }
            for (int j = 0; j < 2; j++) {
                sprite.scale[j] = lerp(a.scale[j], b.scale[j], alpha);
            }
            turn = remainderf(turn, FULL_TURN);
            sprite.angle = a.angle + (turn * alpha);
        }
        if (out.room(2)) {
            out.sprite(sprite, own.store.sprites[kc.state].raster, view);
        } else {
            unplaced_++;
        }
        break;
    }
    default: {
        sa_triangles t = triangles_of(own, c);
        uint32_t count = sa_triangles_points(t.form, t.count);
        const sa_point *points = own.store.points.data() + kc.geometry;

        if (from != NONE) {
            sa_triangles a = triangles_of(before_, from);
            sa_triangles b = triangles_of(s, i);

            blend_transform(a.object_to_world, b.object_to_world, alpha,
                            t.object_to_world);
            if (a.form == b.form && a.count == b.count &&
                count <= scratch_points_.size()) {
                const sa_point *was =
                    before_.store.points.data() + before_.kept[from].geometry;
                const sa_point *is = s.store.points.data() + s.kept[i].geometry;

                for (uint32_t p = 0; p < count; p++) {
                    scratch_points_[p] = points[p];
                    for (int j = 0; j < 3; j++) {
                        scratch_points_[p].at[j] =
                            lerp(was[p].at[j], is[p].at[j], alpha);
                    }
                }
                points = scratch_points_.data();
            }
        }
        /* a line is two triangles on the screen */
        if (out.room(t.form == SA_TRIANGLES_LINES ? 2U * t.count : t.count)) {
            out.triangles(t, points, own.store.triangles[kc.state].raster,
                          view);
        } else {
            unplaced_++;
        }
        break;
    }
    }
}

/* whether the older picture's draw from and the newer's to are one instance
 * moving */
bool World::joins(uint32_t from, uint32_t to) const
{
    return from != NONE && before_.groups[from] == now_.groups[to] &&
           (now_.kept[to].kind != KEPT_MESH ||
            before_.store.draws[before_.kept[from].state].generation ==
                now_.store.draws[now_.kept[to].state].generation);
}

bool World::between(float alpha, Scene &out)
{
    sa_picture picture = now_.picture;
    bool joined_now = joined();

    counts_ = BetweenCounts{};
    unplaced_ = 0;
    if (now_.full != 0) {
        return false;
    }
    /* a picture short of a draw is no picture between, and counts as none */
    auto whole = [this, &out]() {
        out.finish();
        if (unplaced_ != 0) {
            counts_ = BetweenCounts{};
        }
        return unplaced_ == 0;
    };

    if (joined_now) {
        const sa_view &a = before_.picture.view;
        const sa_view &b = now_.picture.view;

        blend_view(a.world_to_view, b.world_to_view, alpha,
                   picture.view.world_to_view);
        for (int i = 0; i < 2; i++) {
            picture.view.focal[i] = lerp(a.focal[i], b.focal[i], alpha);
            picture.view.center[i] = lerp(a.center[i], b.center[i], alpha);
        }
    }
    out.begin(picture);
    out.fog(now_.fog);
    std::fill(std::begin(lenses_), std::end(lenses_), NONE);
    if (!joined_now) {
        counts_.cut = 1;
        /* the newer picture is drawn whole, as the game draws it, in its
         * first draw's view */
        if (!now_.kept.empty() &&
            !same_matrix(now_.views[now_.kept[0].view], picture.view)) {
            out.home(out.view(now_.views[now_.kept[0].view]));
        }
        for (uint32_t i = 0; i < now_.kept.size(); i++) {
            if (shows(now_, i)) {
                put(now_, i, NONE, 1.0F,
                    lens(now_, i, NONE, 1.0F, picture.view, out), out);
            }
        }
        return whole();
    }
    /* what has no way from one picture to the other shows as the nearer
     * picture has it. a draw on its way shows where the nearer's draw of it
     * passed the game's view test */
    bool newer = alpha > 0.5F;

    for (uint32_t i = 0; i < now_.kept.size(); i++) {
        uint32_t from = find(before_, now_.keys[i]);
        uint32_t instance = now_.keys[i].instance;

        if (joins(from, i)) {
            if (newer ? shows(now_, i) : shows(before_, from)) {
                put(now_, i, from, alpha,
                    lens(now_, i, from, alpha, picture.view, out), out);
                counts_.moved++;
            }
        } else if (!shows(now_, i)) {
            continue;
        } else if (instance != SA_INSTANCE_UNOWNED &&
                   !drew(before_, instance)) {
            if (newer) {
                put(now_, i, NONE, 1.0F,
                    lens(now_, i, NONE, 1.0F, picture.view, out), out);
            } else {
                counts_.born++;
            }
        } else if (newer) {
            put(now_, i, NONE, 1.0F,
                lens(now_, i, NONE, 1.0F, picture.view, out), out);
            counts_.snapped++;
        }
    }
    for (uint32_t i = 0; i < before_.kept.size(); i++) {
        uint32_t to = find(now_, before_.keys[i]);
        uint32_t instance = before_.keys[i].instance;

        if ((to != NONE && joins(i, to)) || !shows(before_, i)) {
            continue;
        }
        bool owned = instance != SA_INSTANCE_UNOWNED;

        if (owned && !drew(now_, instance) &&
            std::binary_search(before_.ended.begin(), before_.ended.end(),
                               instance)) {
            counts_.ended++;
        } else if (!newer) {
            put(before_, i, NONE, 0.0F,
                lens(before_, i, NONE, 0.0F, picture.view, out), out);
            if (owned && drew(now_, instance)) {
                counts_.snapped++;
            } else {
                counts_.stayed++;
            }
        }
    }
    return whole();
}

} // namespace pc
