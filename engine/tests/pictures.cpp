/* the PC backend's resources and pictures, over a raster that only counts.
 * the test holds each fence a device would. a release ends a handle at once,
 * and a draw that names it is refused. a picture keeps what it took, and its
 * bytes come back only once it retires. a picture the test's device refuses
 * stays open to end again. SA1's client ends it before it begins another.
 * the client's textures come from two files the test writes under TMPDIR or
 * /tmp and removes. it returns 0, or prints the line of the first check that
 * fails and returns 1. given the path of the capture the adapter's test
 * wrote, it checks the client's reading of that alone */
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <string>
#include <vector>

#include "client.h"
#include "raster.h"
#include "sa_capture.h"
#include "sa_client.h"
#include "sa_pc.h"
#include "world.h"

namespace {

/* the device, as a test holds it */
struct Fake {
    /* each raster frame's fence. a rate adds four rooms for the pictures
     * between and one for the overview. so 9 holds four pictures in flight */
    bool busy[9] = {};
    uint32_t waits = 0;
    uint32_t next_texture = 1;
    std::vector<uint32_t> freed;
    uint32_t vertices = 0;
    uint32_t screen_batches = 0;
    uint32_t blend_batches = 0;
    pc::PictureUniform uniform{};
    /* each submit's frame, first x, first batch's texture, every x and
     * every base color */
    std::vector<uint32_t> frames;
    std::vector<float> first_x;
    std::vector<uint32_t> first_texture;
    std::vector<std::vector<float>> xs;
    std::vector<std::vector<uint32_t>> bases;
    std::vector<pc::Vertex> last; /* the last submit's points */
    std::vector<uint32_t> samples; /* and how each of its batches samples */
    uint32_t lost = 0; /* what a retired picture's lists left out, */
    uint32_t cut = 0; /* and its pixels */
    bool wide = false; /* as the raster was last told */
    /* submit takes the next takes pictures, then refuses the next refuses.
     * broken is why the device draws no more, or null while it draws */
    uint32_t takes = 0;
    uint32_t refuses = 0;
    const char *broken = nullptr;
    bool window = false; /* whether an engine that asks a window opens one */
} fake;

} // namespace

namespace pc {

struct Raster::Impl {};

Raster::~Raster() = default;

bool Raster::open(const RasterOptions &, std::string &)
{
    impl_ = nullptr;
    return true;
}

std::string Raster::device_name() const
{
    return "none";
}

uint32_t Raster::add_texture(const Image &, std::string &)
{
    return fake.next_texture++;
}

void Raster::free_texture(uint32_t texture)
{
    fake.freed.push_back(texture);
}

bool Raster::submit(uint32_t frame, const PictureUniform &picture, uint32_t,
                    const std::vector<Vertex> &vertices,
                    const std::vector<Batch> &batches, bool, std::string &why)
{
    if (fake.broken != nullptr) {
        why = fake.broken;
        return false;
    }
    if (fake.takes != 0) {
        fake.takes--;
    } else if (fake.refuses != 0) {
        fake.refuses--;
        why = "the test's device refuses the picture";
        return false;
    }
    fake.busy[frame] = true;
    fake.vertices = static_cast<uint32_t>(vertices.size());
    fake.frames.push_back(frame);
    fake.first_x.push_back(vertices.empty() ? NAN : vertices[0].at[0]);
    fake.first_texture.push_back(batches.empty() ? 0
                                                 : batches[0].state.texture);
    fake.xs.emplace_back();
    fake.bases.emplace_back();
    for (const Vertex &v : vertices) {
        fake.xs.back().push_back(v.at[0]);
        fake.bases.back().push_back(v.base);
    }
    fake.last = vertices;
    fake.uniform = picture;
    fake.screen_batches = 0;
    fake.blend_batches = 0;
    fake.samples.clear();
    for (const Batch &b : batches) {
        fake.screen_batches += b.state.flags & BATCH_SCREEN ? 1 : 0;
        fake.blend_batches += b.state.blend;
        fake.samples.push_back(b.state.sample);
    }
    return true;
}

void Raster::wide(bool on)
{
    fake.wide = on;
}

bool Raster::broken(std::string &why) const
{
    if (fake.broken == nullptr) {
        return false;
    }
    why = fake.broken;
    return true;
}

bool Raster::done(uint32_t frame)
{
    return !fake.busy[frame];
}

void Raster::wait(uint32_t frame)
{
    fake.waits += fake.busy[frame] ? 1 : 0;
    fake.busy[frame] = false;
}

uint32_t Raster::lost(uint32_t)
{
    return fake.lost;
}

uint32_t Raster::cut(uint32_t)
{
    return fake.cut;
}

uint64_t Raster::gpu_ns(uint32_t)
{
    return 0;
}

bool Raster::read(uint32_t, void *)
{
    return false;
}

/* the window the backend asks for, in place of its own, so the test needs
 * no display. it opens where fake.window says. it gives no
 * pad and no sound, and the platform never asks it to end */
struct Window {};

Window *window_open(const char *, uint32_t, uint32_t, uint32_t, const char *,
                    const Image *, std::string &why)
{
    static Window one;

    if (!fake.window) {
        why = "the test opens no window";
        return nullptr;
    }
    return &one;
}

void window_close(Window *)
{
}

void window_alert(const char *, const char *)
{
}

bool window_pump(Window *)
{
    return false;
}

sa_result window_pad(Window *, uint32_t, sa_pad &)
{
    return SA_EMPTY;
}

sa_result window_rumble(Window *, uint32_t, uint16_t, uint16_t, uint32_t,
                        uint32_t)
{
    return SA_EMPTY;
}

bool window_sound_open(Window *, SoundRender, void *, std::string &why)
{
    why = "the test's window has no sound device";
    return false;
}

void window_sound_close(Window *)
{
}

} // namespace pc

#define CHECK(test)                                                            \
    do {                                                                       \
        if (!(test)) {                                                         \
            return __LINE__;                                                   \
        }                                                                      \
    } while (0)

namespace {

/* the game's 59.94 steps a second, in sa_pc_options.rate's thousandths */
constexpr uint32_t STEP_RATE = 59940;

/* a handle's slot and its generation, as sa_engine.h packs them: the slot in
 * the low 20 bits, and the generation in the 8 above */
constexpr uint32_t slot_of(sa_handle handle)
{
    return handle & 0xfffffU;
}

constexpr uint32_t generation_of(sa_handle handle)
{
    return handle >> 20 & 0xffU;
}

alignas(32) uint8_t block[1 << 22];
/* a client refused its end for good, which the process keeps */
sa_pc_client *stays;

/* a mesh of one triangle, with normals, in SA_FORMAT_MESH's form */
struct Triangle {
    sa_mesh mesh = {3, 3, 1, 1};
    float points[9] = {0, 0, 1, 1, 0, 1, 0, 1, 1};
    float normals[9] = {0, 0, 1, 0, 0, 1, 0, 0, 1};
    sa_meshset set = {0, 0, 3};
    uint16_t corners[4] = {0, 1, 2, 0};
};

/* a mesh of two meshsets over its three points, the second's material the
 * second of a draw's list */
struct TwoSets {
    sa_mesh mesh = {3, 3, 2, 2};
    float points[9] = {0, 0, 1, 1, 0, 1, 0, 1, 1};
    float normals[9] = {0, 0, 1, 0, 0, 1, 0, 0, 1};
    sa_meshset first = {0, 0, 3};
    uint16_t first_corners[4] = {0, 1, 2, 0};
    sa_meshset second = {1, 0, 3};
    uint16_t second_corners[4] = {0, 1, 2, 0};
};

struct Pixel {
    sa_pixels size = {1, 1};
    uint8_t rgba[4] = {255, 255, 255, 255};
};

sa_result make(sa_engine *engine, uint32_t kind, uint32_t format,
               const void *data, uint32_t bytes, sa_handle *handle)
{
    sa_resource_desc desc{};

    desc.kind = kind;
    desc.format = format;
    desc.data = data;
    desc.bytes = bytes;
    return sa_resource_create(engine, &desc, handle);
}

/* bytes a test lends, on pages of their own. after take_back a read of them
 * faults, so a read past their retirement shows */
struct Page {
    void *bytes = MAP_FAILED;
    size_t size = 0;

    Page(const void *from, size_t count) : size(count)
    {
        bytes = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANON, -1, 0);
        if (bytes != MAP_FAILED) {
            memcpy(bytes, from, size);
        }
    }
    Page(const Page &) = delete;
    Page &operator=(const Page &) = delete;
    ~Page()
    {
        if (bytes != MAP_FAILED) {
            munmap(bytes, size);
        }
    }
    bool take_back() { return mprotect(bytes, size, PROT_NONE) == 0; }
};

/* the kinds of the next events, packed base 10, oldest first */
uint32_t polled(sa_engine *engine)
{
    sa_event event;
    uint32_t kinds = 0;

    while (sa_event_poll(engine, &event) == SA_OK) {
        kinds = kinds * 10 + event.kind;
    }
    return kinds;
}

/* take the events that wait, and add each retirement's handle to retired */
void take(sa_engine *engine, std::vector<sa_handle> &retired)
{
    sa_event event;

    while (sa_event_poll(engine, &event) == SA_OK) {
        if (event.kind == SA_EVENT_RETIRED) {
            retired.push_back(event.handle);
        }
    }
}

sa_picture picture_at(uint32_t tick)
{
    sa_picture made{};

    made.head = {SA_REC_PICTURE, sizeof made};
    made.tick = tick;
    return made;
}

/* a record of the capture's, its head filled, with bytes after it */
template <typename Record>
std::vector<uint8_t> record(uint16_t kind, Record body,
                            const std::vector<uint8_t> &more = {})
{
    std::vector<uint8_t> bytes(sizeof body + more.size());

    body.head = {kind, static_cast<uint16_t>(bytes.size())};
    memcpy(bytes.data(), &body, sizeof body);
    memcpy(bytes.data() + sizeof body, more.data(), more.size());
    return bytes;
}

/* an NJS_MODEL at SA_CAP_INLINE_BASE, as a writer puts one in a capture.
 * its three points follow the 40-byte model. its one meshset, at 76, holds
 * one strip of three at 100 */
std::vector<uint8_t> inline_model()
{
    std::vector<uint8_t> model(120, 0);
    auto put32 = [&model](uint32_t at, uint32_t v) {
        memcpy(&model[at], &v, sizeof v);
    };
    auto put16 = [&model](uint32_t at, uint16_t v) {
        memcpy(&model[at], &v, sizeof v);
    };
    put32(0, SA_CAP_INLINE_BASE + 40);
    put32(8, 3);
    put32(12, SA_CAP_INLINE_BASE + 76);
    put16(20, 1);
    put16(22, 1);
    put16(76, 0xc000);
    put16(78, 1);
    put32(80, SA_CAP_INLINE_BASE + 100);
    put16(100, 3);
    put16(102, 0);
    put16(104, 1);
    put16(106, 2);
    return model;
}

/* an ARGB color as the raster takes it, red lowest */
uint32_t as_raster(uint32_t argb)
{
    return (argb & 0xff00ff00) | (argb >> 16 & 0xff) | (argb & 0xff) << 16;
}

/* an NJS_MODEL at SA_CAP_INLINE_BASE whose points shade from a palette.
 * its three points have normals along z. it has a meshset for each of
 * materials, in order. each is one strip of corners points, the three in
 * turn */
std::vector<uint8_t> inline_sets(const std::vector<uint16_t> &materials,
                                 uint32_t corners = 3)
{
    const float points[9] = {0, 0, 1, 1, 0, 1, 0, 1, 1};
    const uint32_t sets = static_cast<uint32_t>(materials.size());
    const uint32_t at_sets = 112;
    const uint32_t at_strips = at_sets + 24 * sets;
    const uint32_t strip_bytes = (2 * (1 + corners) + 3) & ~3U;
    std::vector<uint8_t> model(at_strips + strip_bytes * sets, 0);
    uint16_t count = 0;
    auto put32 = [&model](uint32_t at, uint32_t v) {
        memcpy(&model[at], &v, sizeof v);
    };
    auto put16 = [&model](uint32_t at, uint16_t v) {
        memcpy(&model[at], &v, sizeof v);
    };
    auto putf = [&model](uint32_t at, float v) {
        memcpy(&model[at], &v, sizeof v);
    };

    put32(0, SA_CAP_INLINE_BASE + 40);
    put32(4, SA_CAP_INLINE_BASE + 76);
    put32(8, 3);
    put32(12, SA_CAP_INLINE_BASE + at_sets);
    put16(20, static_cast<uint16_t>(sets));
    for (int i = 0; i < 9; i++) {
        putf(40 + 4 * static_cast<uint32_t>(i), points[i]);
    }
    for (uint32_t i = 0; i < 3; i++) {
        putf(76 + 12 * i + 8, 1.0F);
    }
    for (uint32_t i = 0; i < sets; i++) {
        uint32_t set = at_sets + 24 * i;
        uint32_t strip = at_strips + strip_bytes * i;

        count = materials[i] >= count ? materials[i] + 1 : count;
        put16(set, static_cast<uint16_t>(0xc000 | materials[i]));
        put16(set + 2, 1);
        put32(set + 4, SA_CAP_INLINE_BASE + strip);
        put16(strip, static_cast<uint16_t>(corners));
        for (uint32_t k = 0; k < corners; k++) {
            put16(strip + 2 + 2 * k, static_cast<uint16_t>(k % 3));
        }
    }
    put16(22, count);
    return model;
}

/* a client fed a capture's records draws a mesh a writer made. the first
 * draw names a view the picture's records give, and the raster looks through
 * it. a draw record that ends before its view draws under the picture's own
 * view */
int client_records(sa_engine *engine)
{
    const char *const no_roots[] = {nullptr};
    sa_pc_client *client = sa_pc_client_create(engine, no_roots);
    std::vector<uint8_t> model = inline_model();
    sa_cap_mesh mesh{};
    sa_cap_material material{};
    sa_cap_materials list{};
    uint32_t material_id = 0;
    std::vector<uint8_t> ids(sizeof material_id, 0);
    sa_shade shade{};
    static sa_cap_palettes palettes;
    sa_cap_picture picture{};
    sa_picture_view view{};
    sa_draw draw{};
    sa_cap_end end{};

    mesh.format = SA_MESH_NINJA_INLINE;
    list.count = 1;
    /* the picture's own view at the world's origin, and one 5 along x */
    for (int i = 0; i < 12; i += 5) {
        picture.view.world_to_view[i] = 1;
    }
    view.view = 1;
    view.value = picture.view;
    view.value.world_to_view[3] = -5;
    draw.payload = SA_NO_PAYLOAD;
    draw.view = 1;
    for (const std::vector<uint8_t> &r :
         {record(SA_CAP_MESH, mesh, model), record(SA_CAP_MATERIAL, material),
          record(SA_CAP_MATERIALS, list, ids), record(SA_REC_SHADE, shade),
          record(SA_CAP_PALETTES, palettes), record(SA_CAP_PICTURE, picture),
          record(SA_REC_VIEW, view), record(SA_REC_DRAW, draw)}) {
        CHECK(sa_pc_client_record(client, r.data()) == SA_OK);
    }
    /* the draw again, its record cut before its view */
    std::vector<uint8_t> before = record(SA_REC_DRAW, draw);
    sa_record_head head = {SA_REC_DRAW, offsetof(sa_draw, view)};

    memcpy(before.data(), &head, sizeof head);
    CHECK(sa_pc_client_record(client, before.data()) == SA_OK);
    /* too short for its kind: refused, and the picture stays whole */
    head.size = sizeof head;
    CHECK(sa_pc_client_record(client, &head) == SA_ERR_INVALID);
    std::vector<uint8_t> last = record(SA_CAP_END, end);
    CHECK(sa_pc_client_record(client, last.data()) == SA_PC_ENDED);
    CHECK(fake.vertices == 6 && fake.uniform.view[3] == -5 &&
          fake.xs.back()[3] - fake.xs.back()[0] == 5);
    /* the client waits for its pictures' fences itself as it ends */
    CHECK(sa_pc_client_destroy(client) == SA_OK && !fake.busy[0] &&
          !fake.busy[1]);
    return 0;
}

/* a client fed an act's scene passes it on by the scene's calls. its parts
 * and lights belong to no picture, though their records lie in one. a draw
 * under a part's name draws the part. freeing a material, or texture 0,
 * takes nothing from a list a part reads. the act's end ends the parts and
 * the lights */
int client_scene()
{
    const char *const no_roots[] = {nullptr};
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;
    sa_pc_stats stats{};
    sa_cap_mesh mesh{};
    sa_cap_texture texture{};
    sa_cap_material material{};
    sa_cap_materials list{};
    uint32_t material_id = 0;
    std::vector<uint8_t> ids(sizeof material_id, 0);
    sa_shade shade{};
    static sa_cap_palettes palettes;
    sa_cap_picture picture{};
    sa_scene scene{};
    sa_cap_scene_end whole{};
    sa_lighting lights{};
    sa_draw part{};
    sa_cap_life life{};
    sa_cap_end end{};

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 8;
    desc.events = 16;
    desc.platform = &options;
    options.rate = STEP_RATE;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    sa_pc_client *client = sa_pc_client_create(engine, no_roots);
    /* give a picture's records, or those between two. each gives SA_OK but
     * the last, which gives last. after a picture's end, read its figures */
    auto give = [&](const std::vector<std::vector<uint8_t>> &records,
                    sa_result last) {
        for (bool &b : fake.busy) {
            b = false;
        }
        for (size_t i = 0; i < records.size(); i++) {
            if (sa_pc_client_record(client, records[i].data()) !=
                (i + 1 == records.size() ? last : SA_OK)) {
                return false;
            }
        }
        return last != SA_PC_ENDED || sa_pc_stats_read(engine, &stats) == SA_OK;
    };

    mesh.format = SA_MESH_NINJA_INLINE;
    material.texture = SA_CAP_NONE;
    list.count = 1;
    scene.scene = 0x0100;
    whole.scene = scene.scene;
    whole.parts = 2;
    lights.direction[1] = -1;
    lights.count = 2;
    part.payload = SA_NO_PAYLOAD;
    part.instance = SA_INSTANCE_SCENE | 0;
    sa_draw other = part;

    other.instance = SA_INSTANCE_SCENE | 1;
    /* the scene lies inside a picture, and is none of it */
    CHECK(give(
        {record(SA_CAP_MESH, mesh, inline_model()),
         record(SA_CAP_TEXTURE, texture), record(SA_CAP_MATERIAL, material),
         record(SA_CAP_MATERIALS, list, ids), record(SA_REC_SHADE, shade),
         record(SA_CAP_PALETTES, palettes), record(SA_CAP_PICTURE, picture),
         record(SA_CAP_SCENE, scene), record(SA_CAP_SCENE_LIGHTING, lights),
         record(SA_REC_DRAW, part), record(SA_REC_DRAW, other),
         record(SA_CAP_SCENE_END, whole), record(SA_CAP_END, end)},
        SA_PC_ENDED));
    CHECK(stats.draws == 0 && stats.parts == 2 && stats.resting == 2);
    /* the one light and its two sources */
    CHECK(stats.lights == 3);
    /* material 0 goes, then texture 0, which a list entry without a
     * material record would name. the list and the parts that read it stay */
    life.kind = SA_CAP_MATERIAL;
    sa_cap_life gone = life;

    gone.kind = SA_KIND_TEXTURE;
    CHECK(give({record(SA_CAP_FREE, life), record(SA_CAP_FREE, gone)}, SA_OK));
    picture.picture = 1;
    picture.tick = 2;
    end.picture = 1;
    CHECK(give({record(SA_CAP_PICTURE, picture), record(SA_REC_DRAW, other),
                record(SA_CAP_END, end)},
               SA_PC_ENDED));
    CHECK(stats.draws == 1 && stats.parts == 2 && stats.resting == 1);
    CHECK(stats.lights == 3);
    /* the act goes, and its parts and its lights with it */
    picture.picture = 2;
    picture.tick = 4;
    end.picture = 2;
    CHECK(give({record(SA_CAP_SCENE_ENDED, scene),
                record(SA_CAP_PICTURE, picture), record(SA_CAP_END, end)},
               SA_PC_ENDED));
    CHECK(stats.parts == 0 && stats.resting == 0 && stats.lights == 0);
    CHECK(sa_pc_client_destroy(client) == SA_OK);
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* a capture gives the palettes again at each change, under a new id. a
 * picture names the latest. a value may come again. the client keeps the
 * latest version alone, so it holds one version's bytes however many came.
 * each picture shades from the version it names, even one naming the latest
 * again with no new record. a draw in a picture that names an ended version
 * is refused */
int palette_versions()
{
    const char *const no_roots[] = {nullptr};
    const uint32_t colors[3] = {0x80102030, 0x80405060, 0x80708090};
    const uint32_t pictures = 2000;
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;
    sa_cap_mesh mesh{};
    sa_cap_material material{};
    sa_cap_materials list{};
    uint32_t material_id = 0;
    std::vector<uint8_t> ids(sizeof material_id, 0);
    sa_shade shade{};
    static sa_cap_palettes palettes;
    sa_cap_picture picture{};
    sa_draw draw{};
    sa_cap_end end{};
    uint32_t given = 0;
    uint32_t lent = 0;

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 8;
    desc.events = 16;
    desc.platform = &options;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    sa_pc_client *client = sa_pc_client_create(engine, no_roots);
    auto *held = reinterpret_cast<pc::Client *>(client);
    /* show picture number of the one draw, under palettes id. color is its
     * first point's as the raster takes it, or 0 where it has no point */
    auto show = [&](uint32_t number, uint32_t id, uint32_t *color) {
        picture.picture = number;
        picture.tick = 10 + 2 * number;
        picture.palettes = id;
        end.picture = number;
        fake.last.clear();
        for (bool &b : fake.busy) {
            b = false;
        }
        if (sa_pc_client_record(
                client, record(SA_CAP_PICTURE, picture).data()) != SA_OK ||
            sa_pc_client_record(client, record(SA_REC_DRAW, draw).data()) !=
                SA_OK ||
            sa_pc_client_record(client, record(SA_CAP_END, end).data()) !=
                SA_PC_ENDED) {
            return false;
        }
        *color = fake.last.empty() ? 0 : fake.last[0].base;
        return true;
    };
    mesh.format = SA_MESH_NINJA_INLINE;
    material.texture = SA_CAP_NONE;
    list.count = 1;
    for (int i = 0; i < 12; i += 5) {
        picture.view.world_to_view[i] = 1;
        draw.object_to_world[i] = 1;
    }
    draw.payload = SA_NO_PAYLOAD;
    for (const std::vector<uint8_t> &r :
         {record(SA_CAP_MESH, mesh, inline_sets({0})),
          record(SA_CAP_MATERIAL, material),
          record(SA_CAP_MATERIALS, list, ids), record(SA_REC_SHADE, shade)}) {
        CHECK(sa_pc_client_record(client, r.data()) == SA_OK);
    }
    CHECK(held->names.palettes.bytes() == 0);
    for (uint32_t i = 0; i < pictures; i++) {
        uint32_t color = 0;

        /* three pictures of four give new palettes, of three values in
         * turn. the fourth names the third's again */
        if (i % 4 != 3) {
            palettes.palettes = given;
            for (uint32_t entry = 0; entry < SA_PALETTE_ENTRIES; entry++) {
                palettes.words[entry * 2] = colors[given % 3];
            }
            given++;
            CHECK(sa_pc_client_record(
                      client, record(SA_CAP_PALETTES, palettes).data()) ==
                  SA_OK);
        }
        CHECK(show(i, given - 1, &color) &&
              color == as_raster(colors[(given - 1) % 3]));
        /* the client holds one version of all it was given. it lends
         * nothing past the first picture's mesh and list */
        CHECK(held->names.palettes.bytes() == sizeof(sa_palettes) &&
              held->totals.palettes_given == given);
        lent = i == 0 ? held->held : lent;
        CHECK(held->held == lent && held->lent.size() == 2);
    }
    CHECK(given == pictures - pictures / 4 && held->failures.empty());
    /* the version before the latest is gone. the client counts a picture
     * that names it as a failure, and the runtime refuses its draw */
    uint32_t color = 0;

    CHECK(show(pictures, given - 2, &color) && color == 0);
    CHECK(held->failures.size() == 1 && held->totals.refused == 1);
    CHECK(show(pictures + 1, given - 1, &color) &&
          color == as_raster(colors[(given - 1) % 3]));
    CHECK(held->names.palettes.bytes() == sizeof(sa_palettes) &&
          held->totals.palettes_given == given);
    for (bool &b : fake.busy) {
        b = false;
    }
    sa_pc_client_destroy(client);
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* write at path a PVM of one texture, under name and global index. the
 * texture is a 4 by 4 of ARGB4444 whose every byte is texel. it returns
 * false where it can't write */
bool write_pvm(const std::string &path, const char *name, uint32_t index,
               uint8_t texel)
{
    /* the header's 12 bytes, an entry of its number, its name and its
     * index, and the texture's chunk */
    const uint32_t entry = 2 + 28 + 4;
    std::vector<uint8_t> pvm(12 + entry + 16 + 32, 0);
    uint8_t *chunk = &pvm[12 + entry];
    const uint32_t header = 4 + entry;
    const uint32_t length = 8 + 32;
    FILE *file = fopen(path.c_str(), "wb");

    if (file == nullptr) {
        return false;
    }
    memcpy(&pvm[0], "PVMH", 4);
    memcpy(&pvm[4], &header, sizeof header);
    pvm[8] = 0x1 | 0x8; /* each entry holds a name and a global index */
    pvm[10] = 1;
    memcpy(&pvm[14], name, strlen(name));
    memcpy(&pvm[14 + 28], &index, sizeof index);
    memcpy(chunk, "PVRT", 4);
    memcpy(chunk + 4, &length, sizeof length);
    chunk[8] = 2; /* ARGB4444 */
    chunk[9] = 0x01; /* twiddled */
    chunk[12] = chunk[14] = 4;
    memset(chunk + 16, texel, 32);
    bool wrote = fwrite(pvm.data(), 1, pvm.size(), file) == pvm.size();

    return fclose(file) == 0 && wrote;
}

/* a client over the disc at root. its A.PVM and B.PVM hold a texture of one
 * global index and name. A's texels are white and B's clear. a record's
 * texture comes from the file it names by sa_name_id of the upper case path.
 * a record that names no file takes A's, the first by name. a record whose
 * file lacks the texture makes none, and the client counts it */
int textures_of(const std::string &root)
{
    const char *const roots[] = {root.c_str(), nullptr};
    const struct {
        uint32_t file;
        int texel; /* each byte of its texels, or -1 for none made */
    } each[] = {{sa_name_id("SONICADV/B.PVM"), 0x00},
                {sa_name_id("SONICADV/A.PVM"), 0xff},
                {0, 0xff},
                {sa_name_id("SONICADV/C.PVM"), -1}};
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;
    sa_cap_texture texture{};
    sa_cap_life life{};

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 8;
    desc.events = 16;
    desc.platform = &options;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    sa_pc_client *client = sa_pc_client_create(engine, roots);
    auto *held = reinterpret_cast<pc::Client *>(client);

    CHECK(held->pvrs.count() == 2);
    texture.global_index = 79;
    memcpy(texture.name, "twin", 4);
    life.kind = SA_KIND_TEXTURE;
    for (uint32_t i = 0; i < 4; i++) {
        texture.texture = i;
        texture.file = each[i].file;
        life.id = i;
        CHECK(sa_pc_client_record(
                  client, record(SA_CAP_TEXTURE, texture).data()) == SA_OK);
        CHECK(sa_pc_client_record(client, record(SA_CAP_LOAD, life).data()) ==
              SA_OK);
        sa_handle handle = held->names.textures[i].handle;

        if (each[i].texel < 0) {
            CHECK(handle == SA_NULL && held->failures.size() == 1);
            continue;
        }
        CHECK(handle != SA_NULL && held->failures.empty());
        const std::vector<uint8_t> &lent = held->lent[handle];

        CHECK(lent.size() == sizeof(sa_pixels) + 64);
        CHECK(std::count(lent.begin() + sizeof(sa_pixels), lent.end(),
                         each[i].texel) == 64);
    }
    CHECK(sa_pc_client_destroy(client) == SA_OK);
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* textures_of, over a disc made under TMPDIR or /tmp, and removed */
int named_textures()
{
    const char *tmp = getenv("TMPDIR");
    std::string made =
        std::string(tmp != nullptr && *tmp != '\0' ? tmp : "/tmp") +
        "/pictures.XXXXXX";

    CHECK(mkdtemp(&made[0]) != nullptr);
    const std::string dir = made + "/SONICADV";
    /* the later by name is written first */
    int line = mkdir(dir.c_str(), 0700) == 0 &&
                       write_pvm(dir + "/B.PVM", "twin", 79, 0x00) &&
                       write_pvm(dir + "/A.PVM", "twin", 79, 0xff)
                   ? textures_of(made)
                   : __LINE__;

    unlink((dir + "/A.PVM").c_str());
    unlink((dir + "/B.PVM").c_str());
    rmdir(dir.c_str());
    rmdir(made.c_str());
    return line;
}

/* the colors the raster takes, a corner at a time, for count triangles or
 * lines of form. a line goes as two triangles, whose six
 * corners take its two ends in turn */
std::vector<uint32_t> corner_colors(uint8_t form, uint32_t count,
                                    const std::vector<sa_point> &points)
{
    const uint32_t ends = form == SA_TRIANGLES_LINES ? 2 : 3;
    const uint32_t each = form == SA_TRIANGLES_LINES ? 6 : 3;
    std::vector<uint32_t> colors;

    for (uint32_t i = 0; i < count; i++) {
        for (uint32_t k = 0; k < each; k++) {
            const uint32_t end = k % ends;
            const uint32_t first = form == SA_TRIANGLES_LIST    ? 3 * i
                                   : form == SA_TRIANGLES_STRIP ? i
                                                                : 2 * i;
            /* a fan's every triangle opens at the fan's first point */
            const uint32_t at =
                form == SA_TRIANGLES_FAN && end == 0 ? 0 : first + end;

            colors.push_back(as_raster(points[at].color));
        }
    }
    return colors;
}

/* a client takes whole what a writer fits to a record's size. a draw too
 * large for one record's points comes in several, parted as sa_capture.h
 * says. each form is tried at the most a record holds, one more, and half as
 * many again. each draws every triangle once, in order, its corners
 * in order. a mesh whose record fills SA_CAP_RECORD_MOST draws every
 * triangle */
int record_bounds()
{
    const char *const no_roots[] = {nullptr};
    const uint32_t most = static_cast<uint32_t>(SA_CAP_POINTS_MOST);
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;
    static sa_cap_palettes palettes;
    sa_cap_picture picture{};
    sa_cap_end end{};

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 8;
    desc.events = 16;
    desc.platform = &options;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    sa_pc_client *client = sa_pc_client_create(engine, no_roots);
    auto *held = reinterpret_cast<pc::Client *>(client);
    auto give = [&](const std::vector<uint8_t> &r, sa_result result) {
        return sa_pc_client_record(client, r.data()) == result;
    };
    auto begin = [&]() {
        picture.picture++;
        picture.tick += 2;
        end.picture = picture.picture;
        for (bool &b : fake.busy) {
            b = false;
        }
        return give(record(SA_CAP_PICTURE, picture), SA_OK);
    };

    /* a view down z from the origin, which a line goes to the screen by */
    for (int i = 0; i < 12; i += 5) {
        picture.view.world_to_view[i] = 1;
    }
    picture.view.focal[0] = picture.view.focal[1] = 320;
    picture.view.center[0] = 320;
    picture.view.center[1] = 240;
    picture.view.near_z = 1;
    picture.view.far_z = 60000;
    CHECK(give(record(SA_CAP_PALETTES, palettes), SA_OK));
    for (uint8_t form : {SA_TRIANGLES_STRIP, SA_TRIANGLES_LIST,
                         SA_TRIANGLES_FAN, SA_TRIANGLES_LINES}) {
        /* step is the points each triangle or line adds past the one
         * before. whole is the most one record holds. a strip's part that
         * more follow is even */
        const uint32_t step = form == SA_TRIANGLES_LIST    ? 3
                              : form == SA_TRIANGLES_STRIP ? 1
                                                           : 2;
        const uint32_t whole = form == SA_TRIANGLES_LIST    ? most / 3
                               : form == SA_TRIANGLES_FAN   ? (most - 1) / 2
                               : form == SA_TRIANGLES_LINES ? most / 2
                                                            : most - 2;

        for (uint32_t count : {whole, whole + 1, whole + whole / 2}) {
            std::vector<sa_point> points(sa_triangles_points(form, count));
            sa_triangles draw{};
            uint32_t parts = 0;

            for (uint32_t i = 0; i < points.size(); i++) {
                points[i].at[0] = static_cast<float>(i);
                points[i].at[2] = 10;
                points[i].color = 0xff000000 | i;
            }
            draw.texture = SA_CAP_NONE;
            draw.form = form;
            for (int i = 0; i < 12; i += 5) {
                draw.object_to_world[i] = 1;
            }
            CHECK(begin());
            for (uint32_t first = 0; first < count; parts++) {
                uint32_t left = count - first;
                uint32_t part = sa_triangles_points(form, left) <= most ? left
                                : form == SA_TRIANGLES_STRIP ? whole & ~1U
                                                             : whole;
                std::vector<sa_point> own(points.begin() + step * first,
                                          points.begin() + step * first +
                                              sa_triangles_points(form, part));
                std::vector<uint8_t> tail(own.size() * sizeof own[0]);

                if (form == SA_TRIANGLES_FAN) {
                    own[0] = points[0];
                }
                memcpy(tail.data(), own.data(), tail.size());
                CHECK(sizeof draw + tail.size() <= SA_CAP_RECORD_MOST);
                draw.count = static_cast<uint16_t>(part);
                CHECK(give(record(SA_CAP_TRIANGLES, draw, tail), SA_OK));
                first += part;
            }
            CHECK(parts == (count == whole ? 1U : 2U));
            CHECK(give(record(SA_CAP_END, end), SA_PC_ENDED));
            const std::vector<uint32_t> colors =
                corner_colors(form, count, points);

            CHECK(fake.last.size() == colors.size());
            for (size_t i = 0; i < colors.size(); i++) {
                CHECK(fake.last[i].base == colors[i]);
            }
        }
    }
    /* a model of three points with normals and one meshset takes 136 bytes
     * ahead of its strip. the strip's first word counts the corners after */
    const uint32_t room =
        SA_CAP_RECORD_MOST - static_cast<uint32_t>(sizeof(sa_cap_mesh));
    const uint32_t corners = (room - 136) / 2 - 1;
    std::vector<uint8_t> model = inline_sets({0}, corners);
    sa_cap_mesh mesh{};
    sa_cap_material material{};
    sa_cap_materials list{};
    uint32_t material_id = 0;
    std::vector<uint8_t> ids(sizeof material_id, 0);
    sa_shade shade{};
    sa_draw draw{};

    CHECK(sizeof mesh + model.size() == SA_CAP_RECORD_MOST);
    mesh.format = SA_MESH_NINJA_INLINE;
    material.texture = SA_CAP_NONE;
    list.count = 1;
    draw.payload = SA_NO_PAYLOAD;
    for (int i = 0; i < 12; i += 5) {
        draw.object_to_world[i] = 1;
    }
    for (const std::vector<uint8_t> &r :
         {record(SA_CAP_MESH, mesh, model), record(SA_CAP_MATERIAL, material),
          record(SA_CAP_MATERIALS, list, ids), record(SA_REC_SHADE, shade)}) {
        CHECK(give(r, SA_OK));
    }
    CHECK(begin() && give(record(SA_REC_DRAW, draw), SA_OK) &&
          give(record(SA_CAP_END, end), SA_PC_ENDED));
    CHECK(fake.last.size() == 3 * (corners - 2));
    CHECK(held->failures.empty() && held->totals.refused == 0);
    for (bool &b : fake.busy) {
        b = false;
    }
    sa_pc_client_destroy(client);
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* a client tries each handle a capture's free let go, in a draw of the
 * next picture. it counts what the runtime does with each. a mesh under a
 * live list, a list alone and a texture are each refused. none is taken.
 * a list freed with the texture it names goes untried, as the dead texture
 * alone would refuse it. a mesh goes untried once the client holds no list
 * to draw it under */
int client_released()
{
    const char *const no_roots[] = {nullptr};
    /* a twiddled 4 by 4, as a PVRT chunk holds a texture the game made */
    const uint8_t chunk[16] = {'P', 'V', 'R', 'T', 40, 0, 0, 0,
                               2,   1,   0,   0,   4,  0, 4, 0};
    std::vector<uint8_t> texels(sizeof chunk + 32, 0);
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;
    sa_cap_texture texture{};
    sa_cap_texels part{};
    sa_cap_mesh mesh{};
    sa_cap_material material{};
    sa_cap_materials list{};
    std::vector<uint8_t> ids(sizeof(uint32_t), 0);
    sa_shade shade{};
    static sa_cap_palettes palettes;
    sa_cap_picture picture{};
    sa_draw draw{};
    sa_cap_life life{};
    sa_cap_end end{};

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 8;
    desc.events = 16;
    desc.platform = &options;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    sa_pc_client *client = sa_pc_client_create(engine, no_roots);
    auto *held = reinterpret_cast<pc::Client *>(client);
    auto give = [&](const std::vector<uint8_t> &r, sa_result result) {
        return sa_pc_client_record(client, r.data()) == result;
    };
    /* a free of kind's id, and the next picture, which draws nothing */
    auto freed = [&](uint32_t kind, uint32_t id) {
        life.kind = kind;
        life.id = id;
        picture.picture++;
        picture.tick += 2;
        end.picture = picture.picture;
        for (bool &b : fake.busy) {
            b = false;
        }
        return give(record(SA_CAP_FREE, life), SA_OK) &&
               give(record(SA_CAP_PICTURE, picture), SA_OK) &&
               give(record(SA_CAP_END, end), SA_PC_ENDED);
    };

    memcpy(texels.data(), chunk, sizeof chunk);
    mesh.format = SA_MESH_NINJA_INLINE;
    list.count = 1;
    for (int i = 0; i < 12; i += 5) {
        picture.view.world_to_view[i] = 1;
    }
    draw.payload = SA_NO_PAYLOAD;
    CHECK(give(record(SA_CAP_TEXTURE, texture), SA_OK) &&
          give(record(SA_CAP_TEXELS, part, texels), SA_OK));
    /* two meshes, each under its own list. the first list's material names
     * the texture, and the second's names none */
    for (uint32_t i = 0; i < 2; i++) {
        mesh.mesh = i;
        material.material = i;
        material.flags = i == 0 ? SA_MAT_TEXTURE : 0;
        material.texture = i == 0 ? texture.texture : SA_CAP_NONE;
        list.materials = i;
        memcpy(ids.data(), &i, sizeof i);
        CHECK(give(record(SA_CAP_MESH, mesh, inline_model()), SA_OK) &&
              give(record(SA_CAP_MATERIAL, material), SA_OK) &&
              give(record(SA_CAP_MATERIALS, list, ids), SA_OK));
    }
    CHECK(give(record(SA_REC_SHADE, shade), SA_OK) &&
          give(record(SA_CAP_PALETTES, palettes), SA_OK));
    /* a picture draws both, and the client makes the two meshes, the two
     * lists and the texture */
    CHECK(give(record(SA_CAP_PICTURE, picture), SA_OK));
    for (uint32_t i = 0; i < 2; i++) {
        draw.mesh = i;
        draw.materials = i;
        CHECK(give(record(SA_REC_DRAW, draw), SA_OK));
    }
    CHECK(give(record(SA_CAP_END, end), SA_PC_ENDED));
    CHECK(held->totals.created == 5 && held->totals.released == 0 &&
          held->totals.stale_refused == 0);
    /* the first mesh goes while a list is live: tried, and refused */
    CHECK(freed(SA_KIND_MESH, 0) && held->totals.released == 1 &&
          held->totals.stale_refused == 1);
    /* the second list goes alone: tried, and refused */
    CHECK(freed(SA_KIND_MATERIAL, 1) && held->totals.released == 2 &&
          held->totals.stale_refused == 2);
    /* the texture goes, and the first list with it: the count rises by the
     * texture's try alone */
    CHECK(freed(SA_KIND_TEXTURE, 0) && held->totals.released == 4 &&
          held->totals.stale_refused == 3);
    /* the second mesh goes with no list live: untried */
    CHECK(freed(SA_KIND_MESH, 1) && held->totals.released == 5 &&
          held->totals.stale_refused == 3);
    CHECK(held->totals.stale_taken == 0 && held->totals.refused == 0 &&
          held->failures.empty());
    for (bool &b : fake.busy) {
        b = false;
    }
    CHECK(sa_pc_client_destroy(client) == SA_OK);
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* a client reads the capture the adapter's test wrote. the backend
 * draws whole what the adapter writes at a record's bound. each record's
 * size is a multiple of 4. each record reads, and at each picture the client
 * holds the palettes it names. a record of the most points one holds marks a
 * draw in parts. the raster gets each triangle of that picture's records
 * once, its corners in order. the mesh that fills its record, one strip in
 * the adapter's test, draws each of its triangles. a texture whose texels
 * came in parts is made at its load. the capture holds each of these, and
 * palettes that change */
int adapter_capture(const char *path)
{
    using Corners = std::array<uint32_t, 3>;
    const char *const no_roots[] = {nullptr};
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;
    std::vector<uint8_t> file;
    uint8_t chunk[4096];
    size_t got;
    FILE *in = fopen(path, "rb");

    CHECK(in != nullptr);
    while ((got = fread(chunk, 1, sizeof chunk, in)) != 0) {
        file.insert(file.end(), chunk, chunk + got);
    }
    fclose(in);
    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 64;
    desc.events = 128;
    desc.platform = &options;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    sa_pc_client *client = sa_pc_client_create(engine, no_roots);
    auto *held = reinterpret_cast<pc::Client *>(client);
    /* the open picture's triangles as its records hold them. parted says
     * one record holds the most points. filling counts the picture's draws
     * of the mesh that fills its record */
    std::vector<Corners> given;
    bool parted = false;
    uint32_t filling = 0;
    /* that mesh, and its strip's triangles */
    uint32_t filled = SA_CAP_NONE;
    uint32_t filled_triangles = 0;
    /* the palettes records, the pictures in parts and those of that mesh */
    uint32_t versions = 0;
    uint32_t in_parts = 0;
    uint32_t of_filled = 0;
    /* the texture the last texels were a part of, and the textures made of
     * their texels */
    uint32_t parted_texture = SA_CAP_NONE;
    uint32_t of_texels = 0;

    for (size_t at = 0; at < file.size();) {
        const uint8_t *body = &file[at];
        sa_record_head head;

        CHECK(file.size() - at >= sizeof head);
        memcpy(&head, body, sizeof head);
        CHECK(head.size >= sizeof head && head.size <= file.size() - at &&
              head.size % 4 == 0);
        if (head.kind == SA_CAP_PALETTES) {
            versions++;
        } else if (head.kind == SA_CAP_PICTURE) {
            sa_cap_picture picture;

            CHECK(head.size >= sizeof picture);
            memcpy(&picture, body, sizeof picture);
            CHECK(held->names.palettes.id == picture.palettes &&
                  held->names.palettes.bytes() == sizeof(sa_palettes) &&
                  held->totals.palettes_given == versions);
            given.clear();
            parted = false;
            filling = 0;
            fake.last.clear();
        } else if (head.kind == SA_CAP_MESH &&
                   head.size == SA_CAP_RECORD_MOST) {
            const uint8_t *model = body + sizeof(sa_cap_mesh);
            sa_cap_mesh mesh;
            uint32_t at_sets;
            uint32_t at_strip;
            uint16_t sets;
            uint16_t strips;
            uint16_t points;

            memcpy(&mesh, body, sizeof mesh);
            memcpy(&at_sets, model + 12, sizeof at_sets);
            memcpy(&sets, model + 20, sizeof sets);
            at_sets -= SA_CAP_INLINE_BASE;
            CHECK(sets == 1 && at_sets + 24 <= head.size - sizeof mesh);
            memcpy(&strips, model + at_sets + 2, sizeof strips);
            memcpy(&at_strip, model + at_sets + 4, sizeof at_strip);
            at_strip -= SA_CAP_INLINE_BASE;
            CHECK(strips == 1 && at_strip + 2 <= head.size - sizeof mesh);
            /* a strip's first word counts its points */
            memcpy(&points, model + at_strip, sizeof points);
            CHECK(points >= 3);
            filled = mesh.mesh;
            filled_triangles = points - 2U;
        } else if (head.kind == SA_REC_DRAW) {
            sa_draw draw{};

            memcpy(&draw, body, std::min<size_t>(sizeof draw, head.size));
            filling += draw.mesh == filled ? 1 : 0;
        } else if (head.kind == SA_CAP_TRIANGLES) {
            sa_cap_triangles draw;

            CHECK(head.size >= sizeof draw);
            memcpy(&draw, body, sizeof draw);
            std::vector<sa_point> points(
                sa_triangles_points(draw.form, draw.count));

            CHECK(head.size == sizeof draw + points.size() * sizeof points[0]);
            if (!points.empty()) {
                memcpy(points.data(), body + sizeof draw,
                       points.size() * sizeof points[0]);
            }
            parted = parted || points.size() == SA_CAP_POINTS_MOST;
            const std::vector<uint32_t> colors =
                corner_colors(draw.form, draw.count, points);

            for (size_t i = 0; i < colors.size(); i += 3) {
                given.push_back({colors[i], colors[i + 1], colors[i + 2]});
            }
        } else if (head.kind == SA_CAP_TEXELS) {
            sa_cap_texels part;

            CHECK(head.size >= sizeof part);
            memcpy(&part, body, sizeof part);
            parted_texture = part.texture;
        }
        for (bool &b : fake.busy) {
            b = false;
        }
        sa_result result = sa_pc_client_record(client, body);

        CHECK(result == SA_OK || result == SA_PC_ENDED);
        if (head.kind == SA_CAP_LOAD) {
            sa_cap_life life;

            CHECK(head.size >= sizeof life);
            memcpy(&life, body, sizeof life);
            if (life.kind == SA_KIND_TEXTURE && life.id == parted_texture) {
                auto made = held->names.textures.find(life.id);

                CHECK(made != held->names.textures.end() &&
                      made->second.handle != SA_NULL);
                of_texels++;
            }
        }
        if (result == SA_PC_ENDED && parted) {
            std::vector<Corners> drawn;

            CHECK(fake.last.size() == 3 * given.size());
            for (size_t i = 0; i < fake.last.size(); i += 3) {
                drawn.push_back({fake.last[i].base, fake.last[i + 1].base,
                                 fake.last[i + 2].base});
            }
            /* the backend puts a picture's opaque triangles ahead of its
             * blended ones, so the two compare sorted */
            std::sort(drawn.begin(), drawn.end());
            std::sort(given.begin(), given.end());
            CHECK(drawn == given);
            in_parts++;
        }
        if (result == SA_PC_ENDED && filling != 0) {
            CHECK(fake.last.size() == 3 * filled_triangles * filling);
            of_filled++;
        }
        at += head.size;
    }
    CHECK(versions >= 2 && in_parts >= 1 && of_filled >= 1 && of_texels >= 1);
    for (bool &b : fake.busy) {
        b = false;
    }
    sa_pc_client_destroy(client);
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* the transform between two turns by the shorter arc. scale and place move
 * on a line, and a shear blends element by element */
int transforms()
{
    const float still[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    /* a quarter turn about z, three times as large, moved 10 along x */
    const float turned[12] = {0, -3, 0, 10, 3, 0, 0, 0, 0, 0, 3, 0};
    const float mirrored[12] = {-1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    const float sheared[12] = {1, 1, 0, 4, 0, 1, 0, 0, 0, 0, 1, 0};
    float out[12];
    float c = 2.0F * cosf(0.78539816F);

    pc::blend_transform(still, turned, 0.5F, out);
    CHECK(fabsf(out[0] - c) < 1e-5F && fabsf(out[1] + c) < 1e-5F &&
          fabsf(out[4] - c) < 1e-5F && fabsf(out[10] - 2) < 1e-5F &&
          fabsf(out[3] - 5) < 1e-5F);
    pc::blend_transform(still, turned, 0.0F, out);
    CHECK(fabsf(out[0] - 1) < 1e-6F && fabsf(out[3]) < 1e-6F);
    /* a mirror against none: no turn joins them, so each element blends */
    pc::blend_transform(still, mirrored, 0.25F, out);
    CHECK(fabsf(out[0] - 0.5F) < 1e-6F);
    pc::blend_transform(still, sheared, 0.5F, out);
    CHECK(fabsf(out[1] - 0.5F) < 1e-6F && fabsf(out[3] - 2) < 1e-6F);
    /* a view turns about its eye: the eye's place goes on a line */
    const float view_a[12] = {1, 0, 0, -2, 0, 1, 0, 0, 0, 0, 1, 0};
    const float view_b[12] = {0, 0, -1, 0, 0, 1, 0, 0, 1, 0, 0, -6};
    pc::blend_view(view_a, view_b, 0.5F, out);
    /* the eyes lie 2 and 6 along x. halfway, the point 4 along x maps to
     * the origin */
    CHECK(fabsf((out[0] * 4) + out[3]) < 1e-4F &&
          fabsf((out[8] * 4) + out[11]) < 1e-4F);
    return 0;
}

/* pictures between the game's, from the render-owned scene. a rate needs
 * two pictures in flight. a moved instance goes part of the way. a born one
 * waits for its own picture, and an ended one goes. a cut joins nothing. a
 * release waits for the picture after its own */
int between()
{
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 1;
    desc.resources = 8;
    desc.events = 16;
    desc.platform = &options;
    options.rate = STEP_RATE;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_ERR_INVALID);
    desc.pictures = 2;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    Triangle triangle;
    sa_handle mesh = SA_NULL;
    sa_handle materials = SA_NULL;
    sa_material material{};
    static sa_palettes palettes;
    sa_shade shade{};
    sa_draw draw{};
    sa_pc_stats stats{};

    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &triangle, sizeof triangle,
               &mesh) == SA_OK);
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &material,
               sizeof material, &materials) == SA_OK);
    shade.head = {SA_REC_SHADE, sizeof shade};
    draw.head = {SA_REC_DRAW, sizeof draw};
    draw.mesh = mesh;
    draw.materials = materials;
    draw.payload = SA_NO_PAYLOAD;
    draw.instance = 5;
    draw.object_to_world[0] = draw.object_to_world[5] =
        draw.object_to_world[10] = 1;
    /* show a picture at tick, task 5 at x. six says task 6 draws, and
     * six_ends that task 6 ends while the picture is open */
    auto show = [&](uint32_t tick, float x, bool six, uint16_t flags,
                    bool six_ends = false) {
        sa_picture picture = picture_at(tick);
        sa_draw d = draw;

        picture.flags = flags;
        fake.frames.clear();
        fake.first_x.clear();
        for (bool &b : fake.busy) {
            b = false;
        }
        if (sa_present_begin(engine, &picture) != SA_OK ||
            sa_present_shade(engine, &shade) != SA_OK ||
            sa_present_palettes(engine, &palettes) != SA_OK) {
            return false;
        }
        d.object_to_world[3] = x;
        if (sa_present_draw(engine, &d, 1) != SA_OK) {
            return false;
        }
        d.instance = 6;
        d.object_to_world[3] = 100;
        return (!six || sa_present_draw(engine, &d, 1) == SA_OK) &&
               (!six_ends || sa_present_ended(engine, 6) == SA_OK) &&
               sa_present_end(engine) == SA_OK &&
               sa_pc_stats_read(engine, &stats) == SA_OK;
    };
    /* the first picture has none before it, so the display shows only it */
    CHECK(show(10, 0, false, 0) && stats.shown == 2 && fake.first_x[0] == 0);
    CHECK(show(12, 10, true, 0) && stats.shown == 2);
    /* the picture between goes first, in a room past the pictures in
     * flight. it shows task 5 halfway, and task 6 not yet */
    CHECK(fake.frames.size() == 2 && fake.frames[0] >= desc.pictures &&
          fake.first_x[0] == 5 && fake.first_x[1] == 10);
    CHECK(stats.moved == 1 && stats.born == 1 && stats.snapped == 0);
    /* task 6 stays where it was, unless its task ended in its picture */
    CHECK(show(14, 20, false, 0) && stats.moved == 1 && stats.stayed == 1);
    /* ended after its picture closed: it lived at the time between */
    CHECK(show(16, 30, true, 0) && stats.born == 1);
    CHECK(sa_present_ended(engine, 6) == SA_OK);
    CHECK(show(18, 40, false, 0) && stats.stayed == 1 && stats.ended == 0);
    CHECK(show(20, 50, true, 0) && stats.born == 1);
    CHECK(show(22, 60, true, 0, true) && stats.moved == 2);
    CHECK(show(24, 70, false, 0) && stats.ended == 1 && stats.stayed == 0);
    /* a cut: the picture's draws as they are */
    CHECK(show(26, 80, false, SA_PICTURE_CUT) && stats.shown == 2 &&
          stats.moved == 0 && fake.first_x[0] == 80);
    /* a pause gives the same tick: nothing moves */
    CHECK(show(26, 80, false, 0) && fake.first_x[0] == 80);
    /* task 6 ended, so the scene keeps no instance the picture didn't draw */
    CHECK(stats.resting == 0);
    /* a release waits for the picture after the one that last read it */
    uint32_t freed = static_cast<uint32_t>(fake.freed.size());
    sa_handle gone = SA_NULL;
    Pixel pixel;
    CHECK(make(engine, SA_KIND_TEXTURE, SA_FORMAT_PIXELS, &pixel, sizeof pixel,
               &gone) == SA_OK);
    (void)polled(engine);
    CHECK(sa_resource_release(engine, gone) == SA_OK);
    /* the picture before has retired, but the next one's between reads it */
    fake.busy[0] = fake.busy[1] = false;
    sa_picture next = picture_at(28);
    CHECK(sa_present_begin(engine, &next) == SA_OK &&
          fake.freed.size() == freed && sa_present_end(engine) == SA_OK);
    CHECK(sa_pc_stats_read(engine, &stats) == SA_OK &&
          fake.freed.size() == freed + 1);
    /* a sprite's texture doesn't blend. the picture between takes the
     * nearer picture's, the older one's at the half. its place still moves */
    sa_handle frames[2] = {SA_NULL, SA_NULL};
    CHECK(make(engine, SA_KIND_TEXTURE, SA_FORMAT_PIXELS, &pixel, sizeof pixel,
               &frames[0]) == SA_OK);
    CHECK(make(engine, SA_KIND_TEXTURE, SA_FORMAT_PIXELS, &pixel, sizeof pixel,
               &frames[1]) == SA_OK);
    uint32_t raster_frames[2] = {fake.next_texture - 2, fake.next_texture - 1};
    for (uint32_t k = 0; k < 2; k++) {
        sa_picture at = picture_at(40 + (k * 2));
        sa_sprite digit{};

        digit.head = {SA_REC_SPRITE, sizeof digit};
        digit.texture = frames[k];
        digit.instance = 9;
        digit.flags = SA_SPRITE_SCREEN;
        digit.object_to_world[0] = digit.object_to_world[5] = 1.0F;
        digit.at[0] = 100.0F + (k * 20.0F);
        digit.corner[1][0] = digit.corner[1][1] = 8.0F;
        digit.scale[0] = digit.scale[1] = 1.0F;
        fake.frames.clear();
        fake.first_x.clear();
        fake.first_texture.clear();
        for (bool &b : fake.busy) {
            b = false;
        }
        CHECK(sa_present_begin(engine, &at) == SA_OK &&
              sa_present_sprite(engine, &digit, 1) == SA_OK &&
              sa_present_end(engine) == SA_OK);
    }
    CHECK(fake.first_texture.size() == 2 &&
          fake.first_texture[0] == raster_frames[0] &&
          fake.first_texture[1] == raster_frames[1] && fake.first_x[0] == 110);
    /* ten of the game's steps show 24 pictures at 144 a second. five
     * pictures two steps apart show four or five each */
    sa_engine_destroy(engine);
    options.rate = 144000;
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &triangle, sizeof triangle,
               &mesh) == SA_OK);
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &material,
               sizeof material, &materials) == SA_OK);
    draw.mesh = mesh;
    draw.materials = materials;
    uint32_t shown = 0;
    CHECK(show(0, 0, false, 0));
    for (uint32_t i = 1; i <= 5; i++) {
        CHECK(show(i * 2, 0, false, 0) && stats.shown >= 4 && stats.shown <= 5);
        shown += stats.shown;
    }
    CHECK(shown == 24);
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* a draw shades from the palettes its picture took last before it, in the
 * game's picture and in one the scene draws from what it kept. a picture
 * takes palettes, task 5's draw, other palettes and task 6's draw. at a cut
 * the scene draws it again, each draw in its own palettes' color. the next
 * picture takes third palettes and task 5's draw alone. the picture between
 * the two shows the older picture's draws to the half, each in the color the
 * older picture gave it */
int palette_order()
{
    const uint32_t colors[3] = {0xff102030, 0xff405060, 0xff708090};
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;
    Triangle triangle;
    sa_handle mesh = SA_NULL;
    sa_handle materials = SA_NULL;
    sa_material material{};
    static sa_palettes palettes[3];
    sa_shade shade{};
    sa_draw draw{};
    sa_pc_stats stats{};
    sa_picture picture = picture_at(10);

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 131072; /* a picture's two palettes take 49,152 bytes */
    desc.pictures = 2;
    desc.resources = 8;
    desc.events = 16;
    desc.platform = &options;
    options.rate = STEP_RATE;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &triangle, sizeof triangle,
               &mesh) == SA_OK);
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &material,
               sizeof material, &materials) == SA_OK);
    for (uint32_t k = 0; k < 3; k++) {
        palettes[k].head = {SA_REC_PALETTES, sizeof palettes[k]};
        for (uint32_t i = 0; i < SA_PALETTE_ENTRIES; i++) {
            palettes[k].words[i * 2] = colors[k];
        }
    }
    shade.head = {SA_REC_SHADE, sizeof shade};
    draw.head = {SA_REC_DRAW, sizeof draw};
    draw.mesh = mesh;
    draw.materials = materials;
    draw.payload = SA_NO_PAYLOAD;
    draw.instance = 5;
    draw.object_to_world[0] = draw.object_to_world[5] =
        draw.object_to_world[10] = 1;
    sa_draw second = draw;

    second.instance = 6;
    second.object_to_world[3] = 100;
    /* a triangle's points in palettes first's color. three more follow in
     * palettes then's, where then is no less than 0 */
    auto in_colors = [&](int first, int then) {
        std::vector<uint32_t> points(3, as_raster(colors[first]));

        if (then >= 0) {
            points.insert(points.end(), 3, as_raster(colors[then]));
        }
        return points;
    };

    picture.flags = SA_PICTURE_CUT;
    for (bool &b : fake.busy) {
        b = false;
    }
    fake.bases.clear();
    CHECK(sa_present_begin(engine, &picture) == SA_OK &&
          sa_present_shade(engine, &shade) == SA_OK &&
          sa_present_palettes(engine, &palettes[0]) == SA_OK &&
          sa_present_draw(engine, &draw, 1) == SA_OK &&
          sa_present_palettes(engine, &palettes[1]) == SA_OK &&
          sa_present_draw(engine, &second, 1) == SA_OK &&
          sa_present_end(engine) == SA_OK);
    /* the scene's picture goes first, and then the game's own */
    CHECK(fake.bases.size() == 2 && fake.bases[1] == in_colors(0, 1) &&
          fake.bases[0] == fake.bases[1]);
    for (bool &b : fake.busy) {
        b = false;
    }
    fake.bases.clear();
    picture = picture_at(12);
    draw.object_to_world[3] = 10;
    CHECK(sa_present_begin(engine, &picture) == SA_OK &&
          sa_present_shade(engine, &shade) == SA_OK &&
          sa_present_palettes(engine, &palettes[2]) == SA_OK &&
          sa_present_draw(engine, &draw, 1) == SA_OK &&
          sa_present_end(engine) == SA_OK &&
          sa_pc_stats_read(engine, &stats) == SA_OK);
    /* task 5 moves and task 6 stays. the game's own picture has the third
     * palettes' color */
    CHECK(stats.moved == 1 && stats.stayed == 1 && fake.bases.size() == 2 &&
          fake.bases[0] == in_colors(0, 1) &&
          fake.bases[1] == in_colors(2, -1));
    for (bool &b : fake.busy) {
        b = false;
    }
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* a draw flagged SA_DRAW_OUT, which the game's own view test left out. the
 * scene keeps its instance at the draw's place. a picture between shows it
 * only if the nearer picture drew it. after a texture's release, the
 * overview leaves out what reads it */
int out_of_view()
{
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 8;
    desc.events = 16;
    desc.platform = &options;
    options.rate = STEP_RATE;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    Triangle triangle;
    Pixel pixel;
    sa_handle mesh = SA_NULL;
    sa_handle texture = SA_NULL;
    sa_handle plain = SA_NULL;
    sa_handle textured = SA_NULL;
    /* a resource borrows its bytes, so each list has its own */
    sa_material material{};
    sa_material texture_material{};

    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &triangle, sizeof triangle,
               &mesh) == SA_OK);
    CHECK(make(engine, SA_KIND_TEXTURE, SA_FORMAT_PIXELS, &pixel, sizeof pixel,
               &texture) == SA_OK);
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &material,
               sizeof material, &plain) == SA_OK);
    texture_material.texture = texture;
    texture_material.flags = SA_MAT_TEXTURE;
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &texture_material,
               sizeof texture_material, &textured) == SA_OK);
    static sa_palettes palettes;
    sa_shade shade{};
    sa_draw draw{};
    sa_pc_stats stats{};
    std::vector<uint8_t> rgba(SA_PC_WIDTH * SA_PC_HEIGHT * 4);
    auto rgba_bytes = static_cast<uint32_t>(rgba.size());
    sa_picture above = picture_at(0);
    uint32_t drawn = 0;

    shade.head = {SA_REC_SHADE, sizeof shade};
    draw.head = {SA_REC_DRAW, sizeof draw};
    draw.mesh = mesh;
    draw.materials = plain;
    draw.payload = SA_NO_PAYLOAD;
    draw.instance = 8;
    draw.object_to_world[0] = draw.object_to_world[5] =
        draw.object_to_world[10] = 1;
    auto show = [&](uint32_t tick, float x, uint16_t flags) {
        sa_picture picture = picture_at(tick);
        sa_draw d = draw;

        fake.frames.clear();
        fake.first_x.clear();
        for (bool &b : fake.busy) {
            b = false;
        }
        d.object_to_world[3] = x;
        d.flags = flags;
        return sa_present_begin(engine, &picture) == SA_OK &&
               sa_present_shade(engine, &shade) == SA_OK &&
               sa_present_palettes(engine, &palettes) == SA_OK &&
               sa_present_draw(engine, &d, 1) == SA_OK &&
               sa_present_end(engine) == SA_OK &&
               sa_pc_stats_read(engine, &stats) == SA_OK;
    };
    /* the overview's meshes, and the place of the first: the fake raster
     * reads no pixels back */
    auto overview = [&]() {
        fake.first_x.clear();
        return sa_pc_read_overview(engine, &above, rgba.data(), rgba_bytes,
                                   &drawn) == SA_ERR_UNSUPPORTED &&
               fake.first_x.size() == 1;
    };
    /* out of view in two pictures, it is kept but drawn by neither, nor by
     * the picture between */
    CHECK(show(10, 0, SA_DRAW_OUT) && stats.outside == 1 &&
          stats.resting == 0 && isnan(fake.first_x[0]));
    CHECK(show(12, 10, SA_DRAW_OUT) && isnan(fake.first_x[0]) &&
          isnan(fake.first_x[1]));
    /* the scene followed it out of view */
    CHECK(overview() && drawn == 1 && fake.first_x[0] == 10);
    /* into view: to the half, the between leaves it out as the older did */
    CHECK(show(14, 20, 0) && stats.outside == 0 && isnan(fake.first_x[0]) &&
          fake.first_x[1] == 20);
    /* out again: shown to the half, on its way from one draw to the other */
    CHECK(show(16, 30, SA_DRAW_OUT) && fake.first_x[0] == 25 &&
          isnan(fake.first_x[1]));
    /* a mesh that names a texture, then the texture's release */
    draw.materials = textured;
    draw.instance = 9;
    CHECK(show(18, 40, 0) && overview() && drawn == 2);
    CHECK(sa_resource_release(engine, texture) == SA_OK);
    CHECK(overview() && drawn == 1 && fake.first_x[0] == 30);
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* an act's scene. its calls come in their order. a part's names are
 * checked before any part given with it is taken. nothing is kept without a
 * rate. under one, a part stays an instance whether a picture draws it or
 * not. a picture's draws under its instance add none. the first places it,
 * whether or not the game says it moves. the scene's end ends the part,
 * between two pictures or inside the one that drew it. so does a release of
 * what the part reads. an open scene and the kept one each have their room.
 * a scene's lights go in once, while it is open, and stay until it ends */
int scenes()
{
    Triangle triangle;
    Triangle other;
    sa_material material{};
    static sa_palettes palettes;
    sa_scene scene{};
    sa_shade shade{};
    sa_draw part{};
    sa_lighting lights{};
    sa_pc_stats stats{};
    std::vector<uint8_t> rgba(SA_PC_WIDTH * SA_PC_HEIGHT * 4);
    auto rgba_bytes = static_cast<uint32_t>(rgba.size());
    sa_picture above = picture_at(0);
    uint32_t drawn = 0;

    scene.head = {SA_REC_SCENE, sizeof scene};
    scene.scene = 0x0100;
    lights.head = {SA_REC_LIGHTING, sizeof lights};
    lights.direction[1] = -1;
    lights.count = 2;
    shade.head = {SA_REC_SHADE, sizeof shade};
    shade.shade = 3;
    part.head = {SA_REC_DRAW, sizeof part};
    part.payload = SA_NO_PAYLOAD;
    part.instance = SA_INSTANCE_SCENE | 4;
    part.shade = 3;
    part.object_to_world[0] = part.object_to_world[5] =
        part.object_to_world[10] = 1;
    part.object_to_world[3] = 5;
    for (uint32_t rate : {0U, STEP_RATE}) {
        sa_pc_options options{};
        sa_engine_desc desc{};
        sa_engine *engine = nullptr;
        uint32_t bytes = 0;
        sa_handle mesh = SA_NULL;
        sa_handle gone = SA_NULL;
        sa_handle plain = SA_NULL;

        desc.abi = SA_ENGINE_ABI_VERSION;
        desc.size = sizeof desc;
        desc.game = SA_GAME_SA1;
        desc.arena = 65536;
        desc.pictures = 2;
        desc.resources = 8;
        desc.events = 16;
        desc.platform = &options;
        options.rate = rate;
        CHECK(sa_engine_memory(&desc, &bytes) == SA_OK &&
              bytes <= sizeof block);
        CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
        CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &triangle,
                   sizeof triangle, &mesh) == SA_OK);
        CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &other, sizeof other,
                   &gone) == SA_OK);
        CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &material,
                   sizeof material, &plain) == SA_OK);
        CHECK(sa_resource_release(engine, gone) == SA_OK);
        part.mesh = mesh;
        part.materials = plain;
        /* a picture of draws, which may be a part's own, and its figures */
        auto show = [&](uint32_t tick, const sa_draw *draws, uint32_t count) {
            sa_picture picture = picture_at(tick);

            for (bool &b : fake.busy) {
                b = false;
            }
            return sa_present_begin(engine, &picture) == SA_OK &&
                   sa_present_shade(engine, &shade) == SA_OK &&
                   sa_present_palettes(engine, &palettes) == SA_OK &&
                   (count == 0 ||
                    sa_present_draw(engine, draws, count) == SA_OK) &&
                   sa_present_end(engine) == SA_OK &&
                   sa_pc_stats_read(engine, &stats) == SA_OK;
        };
        /* the overview's meshes, and the place of the first */
        auto overview = [&]() {
            fake.first_x.clear();
            return sa_pc_read_overview(engine, &above, rgba.data(), rgba_bytes,
                                       &drawn) == SA_ERR_UNSUPPORTED &&
                   fake.first_x.size() == 1;
        };

        /* no scene is open, and none stays */
        CHECK(sa_scene_shade(engine, &shade) == SA_ERR_INVALID);
        CHECK(sa_scene_part(engine, &part, 1) == SA_ERR_INVALID);
        CHECK(sa_scene_end(engine) == SA_ERR_INVALID);
        CHECK(sa_scene_ended(engine, scene.scene) == SA_ERR_INVALID);
        CHECK(sa_scene_lighting(engine, &lights) == SA_ERR_INVALID);
        CHECK(sa_scene_begin(engine, &scene) == SA_OK);
        CHECK(sa_scene_begin(engine, &scene) == SA_ERR_INVALID);
        CHECK(sa_scene_ended(engine, scene.scene) == SA_ERR_INVALID);
        /* lights of more sources than the record holds are refused. so is
         * a NaN in the light, or in a source it counts. a NaN past the count
         * goes unread. the scene takes its lights once */
        sa_lighting dark = lights;

        dark.count = SA_LIGHTING_SOURCES + 1;
        CHECK(sa_scene_lighting(engine, &dark) == SA_ERR_INVALID);
        dark = lights;
        dark.ambient = nanf("");
        CHECK(sa_scene_lighting(engine, &dark) == SA_ERR_INVALID);
        dark = lights;
        dark.source[1].range = nanf("");
        CHECK(sa_scene_lighting(engine, &dark) == SA_ERR_INVALID);
        lights.source[2].range = nanf("");
        CHECK(sa_scene_lighting(engine, &lights) == SA_OK);
        CHECK(sa_scene_lighting(engine, &lights) == SA_ERR_INVALID);
        /* a shade the scene never had, though an open picture has it. a
         * scene's shade of lights is refused, as no call gives it lights */
        sa_picture picture = picture_at(8);

        CHECK(sa_present_begin(engine, &picture) == SA_OK &&
              sa_present_shade(engine, &shade) == SA_OK);
        CHECK(sa_scene_part(engine, &part, 1) == SA_ERR_HANDLE);
        sa_shade lit = shade;

        lit.mode = SA_SHADE_LIGHTS;
        CHECK(sa_scene_shade(engine, &lit) == SA_ERR_INVALID);
        CHECK(sa_scene_shade(engine, &shade) == SA_OK);
        /* a name that is no part's, and a payload */
        sa_draw bad = part;

        bad.instance = 8;
        CHECK(sa_scene_part(engine, &bad, 1) == SA_ERR_INVALID);
        bad.instance = SA_INSTANCE_UNOWNED | SA_INSTANCE_SCENE | 4;
        CHECK(sa_scene_part(engine, &bad, 1) == SA_ERR_INVALID);
        bad = part;
        bad.payload = 0;
        CHECK(sa_scene_part(engine, &bad, 1) == SA_ERR_INVALID);
        /* a resource gone: of two given together, neither is taken */
        sa_draw two[2] = {part, part};

        two[0].instance = SA_INSTANCE_SCENE | 9;
        two[1].mesh = gone;
        CHECK(sa_scene_part(engine, two, 2) == SA_ERR_HANDLE);
        two[1] = part;
        two[1].materials = mesh;
        CHECK(sa_scene_part(engine, two, 2) == SA_ERR_HANDLE);
        /* a scene's part goes in while a picture is open, and is none of it */
        CHECK(sa_scene_part(engine, &part, 1) == SA_OK);
        CHECK(sa_scene_end(engine) == SA_OK);
        CHECK(sa_present_palettes(engine, &palettes) == SA_OK &&
              sa_present_end(engine) == SA_OK &&
              sa_pc_stats_read(engine, &stats) == SA_OK && stats.draws == 0);
        CHECK(sa_scene_part(engine, &part, 1) == SA_ERR_INVALID);
        CHECK(sa_scene_end(engine) == SA_ERR_INVALID);
        CHECK(sa_scene_lighting(engine, &lights) == SA_ERR_INVALID);
        CHECK(sa_scene_begin(engine, &scene) == SA_ERR_INVALID);
        CHECK(sa_scene_ended(engine, scene.scene + 1) == SA_ERR_INVALID);
        /* the one light and its two sources, where a scene is kept */
        CHECK(stats.lights == (rate != 0 ? 3U : 0U));
        if (rate == 0) {
            /* no scene is kept: the picture counts no part, and none shows */
            CHECK(stats.parts == 0);
            CHECK(sa_pc_read_overview(engine, &above, rgba.data(), rgba_bytes,
                                      &drawn) == SA_ERR_INVALID);
        } else {
            /* no picture drew it: one part, of the two tried, where it was
             * given */
            CHECK(stats.parts == 1 && stats.resting == 1);
            CHECK(overview() && drawn == 1 && fake.first_x[0] == 5);
            /* a task's draw is another instance. a draw under the part's
             * instance is the part, which then stands where that draw does.
             * every draw stands in the world the part was given in, so the
             * part needs no SA_DRAW_MOVING */
            sa_draw draws[2] = {part, part};

            draws[0].instance = 8;
            draws[1].object_to_world[3] = 20;
            CHECK(show(10, draws, 2) && stats.parts == 1 && stats.resting == 0);
            CHECK(overview() && drawn == 2);
            CHECK(sa_present_ended(engine, 8) == SA_OK);
            CHECK(overview() && drawn == 1 && fake.first_x[0] == 20);
        }
        /* the act went: the part with it, and another scene may begin */
        CHECK(sa_scene_ended(engine, scene.scene) == SA_OK);
        CHECK(sa_scene_ended(engine, scene.scene) == SA_ERR_INVALID);
        CHECK(show(16, nullptr, 0) && stats.parts == 0 && stats.resting == 0);
        CHECK(stats.lights == 0);
        /* a part flagged SA_DRAW_MOVING goes where a draw of it stands, as
         * any part does. that holds out of the game's view too, and the part
         * stays there undrawn */
        sa_draw moving = part;

        moving.flags = SA_DRAW_MOVING;
        CHECK(sa_scene_begin(engine, &scene) == SA_OK &&
              sa_scene_shade(engine, &shade) == SA_OK &&
              sa_scene_part(engine, &moving, 1) == SA_OK &&
              sa_scene_end(engine) == SA_OK);
        if (rate != 0) {
            moving.flags = SA_DRAW_OUT;
            moving.object_to_world[3] = 30;
            CHECK(show(17, &moving, 1) && stats.parts == 1 &&
                  stats.outside == 1);
            CHECK(show(18, nullptr, 0) && stats.parts == 1 &&
                  stats.resting == 1);
            CHECK(overview() && drawn == 1 && fake.first_x[0] == 30);
        }
        CHECK(sa_scene_ended(engine, scene.scene) == SA_OK);
        CHECK(show(19, nullptr, 0) && stats.parts == 0);
        /* empty the ring of events the test never took, for the pictures
         * after */
        (void)polled(engine);
        /* a picture that draws a part twice makes one instance of it */
        CHECK(sa_scene_begin(engine, &scene) == SA_OK &&
              sa_scene_shade(engine, &shade) == SA_OK &&
              sa_scene_part(engine, &part, 1) == SA_OK &&
              sa_scene_end(engine) == SA_OK);
        sa_draw twice[2] = {part, part};

        twice[1].object_to_world[3] = 7;
        CHECK(show(20, twice, 2) && stats.resting == 0);
        if (rate != 0) {
            /* where the first of the two put it */
            CHECK(stats.parts == 1 && overview() && drawn == 1 &&
                  fake.first_x[0] == 5);
            CHECK(show(21, nullptr, 0) && stats.parts == 1 &&
                  stats.resting == 1);
        }
        /* the scene ends while the picture that drew its part is open.
         * nothing of the part stays, in that picture or after it */
        picture = picture_at(22);
        for (bool &b : fake.busy) {
            b = false;
        }
        CHECK(sa_present_begin(engine, &picture) == SA_OK &&
              sa_present_shade(engine, &shade) == SA_OK &&
              sa_present_palettes(engine, &palettes) == SA_OK &&
              sa_present_draw(engine, &part, 1) == SA_OK);
        CHECK(sa_scene_ended(engine, scene.scene) == SA_OK);
        CHECK(sa_present_end(engine) == SA_OK &&
              sa_pc_stats_read(engine, &stats) == SA_OK && stats.draws == 1 &&
              stats.parts == 0 && stats.resting == 0);
        if (rate != 0) {
            CHECK(overview() && drawn == 0);
            CHECK(show(23, nullptr, 0) && stats.resting == 0 && overview() &&
                  drawn == 0);
        }
        /* a part given again under its instance takes the first's place */
        CHECK(sa_scene_begin(engine, &scene) == SA_OK &&
              sa_scene_shade(engine, &shade) == SA_OK &&
              sa_scene_part(engine, &part, 1) == SA_OK &&
              sa_scene_part(engine, &part, 1) == SA_OK &&
              sa_scene_end(engine) == SA_OK);
        CHECK(show(24, nullptr, 0) && stats.parts == (rate != 0 ? 1U : 0U));
        /* a release of what it reads ends the part alone, not the scene */
        CHECK(sa_resource_release(engine, mesh) == SA_OK);
        CHECK(show(26, nullptr, 0) && stats.parts == 0);
        CHECK(sa_scene_ended(engine, scene.scene) == SA_OK);
        (void)polled(engine);
        /* an open scene holds 256 shades, and the kept scene 65536
         * instances. a shade or parts past them are refused, and no part of
         * a refused call is taken. a backend that keeps none has room for
         * all */
        CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &triangle,
                   sizeof triangle, &mesh) == SA_OK);
        CHECK(sa_scene_begin(engine, &scene) == SA_OK);
        for (uint32_t i = 0; i < 256; i++) {
            sa_shade one = shade;

            one.shade = 100 + i;
            CHECK(sa_scene_shade(engine, &one) == SA_OK);
        }
        CHECK(sa_scene_shade(engine, &shade) == SA_ERR_FULL);
        std::vector<sa_draw> crowd(65537, part);

        for (uint32_t i = 0; i < crowd.size(); i++) {
            crowd[i].mesh = mesh;
            crowd[i].shade = 100;
            crowd[i].instance = SA_INSTANCE_SCENE | i;
        }
        CHECK(sa_scene_part(engine, crowd.data(), 65537) ==
              (rate != 0 ? SA_ERR_FULL : SA_OK));
        CHECK(sa_scene_part(engine, crowd.data(), 2) == SA_OK);
        CHECK(sa_scene_end(engine) == SA_OK);
        CHECK(show(28, nullptr, 0) && stats.parts == (rate != 0 ? 2U : 0U));
        /* a scene given no lights has none */
        CHECK(stats.lights == 0);
        CHECK(sa_scene_ended(engine, scene.scene) == SA_OK);
        sa_engine_destroy(engine);
        fake = Fake{};
    }
    return 0;
}

/* a resource released, under a rate, while the picture that drew it is
 * open. the picture keeps what it took. the next picture's between reads it
 * too, so it retires behind that picture's fence. it retires once, and its
 * bytes go. the scene keeps no instance of a released mesh or list, then or
 * after. an instance whose list names a released texture waits for a picture
 * to draw it again. a resource made in a released one's slot is another. a
 * release between two pictures leaves nothing a later picture brings back.
 * an instance that stands once a draw's mesh went stays as it stood. the test
 * lets a fence go only where a comment says the device finishes */
int released()
{
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 16;
    desc.events = 64;
    desc.platform = &options;
    options.rate = STEP_RATE;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    Triangle triangle;
    Pixel pixel;
    sa_material material{};
    sa_material texture_material{};
    sa_handle first = SA_NULL;
    sa_handle second = SA_NULL;
    sa_handle plain = SA_NULL;
    sa_handle list = SA_NULL;
    sa_handle texture = SA_NULL;
    sa_handle textured = SA_NULL;
    Page first_bytes(&triangle, sizeof triangle);
    Page second_bytes(&triangle, sizeof triangle);
    Page list_bytes(&material, sizeof material);

    CHECK(first_bytes.bytes != MAP_FAILED && second_bytes.bytes != MAP_FAILED &&
          list_bytes.bytes != MAP_FAILED);
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, first_bytes.bytes,
               sizeof triangle, &first) == SA_OK);
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, second_bytes.bytes,
               sizeof triangle, &second) == SA_OK);
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &material,
               sizeof material, &plain) == SA_OK);
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, list_bytes.bytes,
               sizeof material, &list) == SA_OK);
    CHECK(make(engine, SA_KIND_TEXTURE, SA_FORMAT_PIXELS, &pixel, sizeof pixel,
               &texture) == SA_OK);
    uint32_t raster_texture = fake.next_texture - 1;

    texture_material.texture = texture;
    texture_material.flags = SA_MAT_TEXTURE;
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &texture_material,
               sizeof texture_material, &textured) == SA_OK);
    static sa_palettes palettes;
    sa_shade shade{};
    sa_draw draw{};
    sa_pc_stats stats{};
    std::vector<sa_handle> retired;
    std::vector<uint8_t> rgba(SA_PC_WIDTH * SA_PC_HEIGHT * 4);
    auto rgba_bytes = static_cast<uint32_t>(rgba.size());
    sa_picture above = picture_at(0);
    uint32_t drawn = 0;

    shade.head = {SA_REC_SHADE, sizeof shade};
    draw.head = {SA_REC_DRAW, sizeof draw};
    draw.payload = SA_NO_PAYLOAD;
    draw.object_to_world[0] = draw.object_to_world[5] =
        draw.object_to_world[10] = 1;
    auto begin = [&](uint32_t tick) {
        sa_picture picture = picture_at(tick);

        fake.frames.clear();
        fake.first_x.clear();
        fake.xs.clear();
        return sa_present_begin(engine, &picture) == SA_OK &&
               sa_present_shade(engine, &shade) == SA_OK &&
               sa_present_palettes(engine, &palettes) == SA_OK;
    };
    auto give = [&](sa_handle mesh, sa_handle materials, uint32_t instance,
                    float x) {
        sa_draw d = draw;

        d.mesh = mesh;
        d.materials = materials;
        d.instance = instance;
        d.object_to_world[3] = x;
        return sa_present_draw(engine, &d, 1);
    };
    /* the overview's meshes, and the place of the first */
    auto overview = [&]() {
        fake.first_x.clear();
        return sa_pc_read_overview(engine, &above, rgba.data(), rgba_bytes,
                                   &drawn) == SA_ERR_UNSUPPORTED &&
               fake.first_x.size() == 1;
    };
    auto gone = [&](sa_handle handle) {
        return std::count(retired.begin(), retired.end(), handle);
    };
    /* the pictures take slots 0 and 1 by turns. slot is the next one's,
     * which the picture before the last holds until then */
    uint32_t tick = 10;
    uint32_t slot = 0;
    auto empty = [&]() {
        bool taken = begin(tick) && sa_present_end(engine) == SA_OK;

        tick += 2;
        slot ^= 1;
        return taken;
    };
    /* the device finishes every picture, and the last one's figures tell */
    auto finish = [&]() {
        for (bool &b : fake.busy) {
            b = false;
        }
        bool read = sa_pc_stats_read(engine, &stats) == SA_OK;

        take(engine, retired);
        return read;
    };

    /* a mesh and a list go while the picture that drew them is open. the
     * handles end at once, and the scene keeps neither instance */
    CHECK(begin(tick) && give(first, plain, 5, 3) == SA_OK &&
          give(second, list, 6, 4) == SA_OK);
    CHECK(sa_resource_release(engine, first) == SA_OK &&
          sa_resource_release(engine, list) == SA_OK);
    CHECK(give(first, plain, 5, 3) == SA_ERR_HANDLE &&
          give(second, list, 6, 4) == SA_ERR_HANDLE);
    CHECK(sa_present_end(engine) == SA_OK);
    tick += 2;
    slot ^= 1;
    CHECK(overview() && drawn == 0);
    /* the next picture draws neither. the one between shows both where they
     * stood, from the bytes still lent */
    CHECK(empty() && fake.xs.size() == 2 && fake.xs[0].size() == 6 &&
          fake.xs[0][0] == 3 && fake.xs[0][3] == 4 && fake.xs[1].empty());
    CHECK(overview() && drawn == 0);
    /* a third picture waits for the first's fence, and the second's stands */
    CHECK(empty());
    take(engine, retired);
    CHECK(retired.empty());
    /* the device finishes the second: each retires, once, and its bytes go */
    fake.busy[slot] = false;
    CHECK(sa_pc_stats_read(engine, &stats) == SA_OK);
    take(engine, retired);
    CHECK(retired.size() == 2 && gone(first) == 1 && gone(list) == 1);
    CHECK(first_bytes.take_back() && list_bytes.take_back());
    /* nothing reads them again: not the overview, nor a picture, nor the
     * one between it and the last */
    CHECK(overview() && drawn == 0);
    CHECK(empty() && finish() && stats.stayed == 0 && stats.moved == 0);
    CHECK(overview() && drawn == 0);
    CHECK(sa_present_ended(engine, 5) == SA_OK &&
          sa_present_ended(engine, 6) == SA_OK);

    /* a texture a list names goes while the picture that drew with it is
     * open. its instance stays but draws nothing. the raster frees the
     * texture once the picture after retires */
    CHECK(begin(tick) && give(second, textured, 7, 5) == SA_OK);
    CHECK(sa_resource_release(engine, texture) == SA_OK);
    CHECK(sa_present_end(engine) == SA_OK);
    tick += 2;
    slot ^= 1;
    CHECK(overview() && drawn == 0);
    CHECK(empty() && fake.xs[0].size() == 3 && fake.xs[0][0] == 5);
    CHECK(empty());
    take(engine, retired);
    CHECK(gone(texture) == 0 && fake.freed.empty());
    CHECK(finish() && gone(texture) == 1 && fake.freed.size() == 1 &&
          fake.freed[0] == raster_texture);
    CHECK(overview() && drawn == 0);
    /* drawn again with a list whose textures live, it draws again */
    CHECK(begin(tick) && give(second, plain, 7, 5) == SA_OK &&
          sa_present_end(engine) == SA_OK);
    tick += 2;
    slot ^= 1;
    CHECK(overview() && drawn == 1 && fake.first_x[0] == 5);
    CHECK(sa_present_ended(engine, 7) == SA_OK);
    CHECK(sa_resource_release(engine, textured) == SA_OK);

    /* a mesh made in the slot of one released in the same picture is
     * another. the scene keeps the instance that drew the new mesh, not the
     * old */
    sa_handle third = SA_NULL;
    sa_handle fourth = SA_NULL;
    Page third_bytes(&triangle, sizeof triangle);
    Page fourth_bytes(&triangle, sizeof triangle);

    CHECK(third_bytes.bytes != MAP_FAILED && fourth_bytes.bytes != MAP_FAILED);
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, third_bytes.bytes,
               sizeof triangle, &third) == SA_OK);
    CHECK(begin(tick) && give(third, plain, 8, 3) == SA_OK);
    CHECK(sa_resource_release(engine, third) == SA_OK);
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, fourth_bytes.bytes,
               sizeof triangle, &fourth) == SA_OK);
    CHECK(slot_of(fourth) == slot_of(third) && fourth != third);
    CHECK(give(fourth, plain, 9, 7) == SA_OK &&
          sa_present_end(engine) == SA_OK);
    tick += 2;
    slot ^= 1;
    CHECK(overview() && drawn == 1 && fake.first_x[0] == 7);

    /* a release between two pictures ends its instance at once. the picture
     * after reads the mesh for its between, but brings no instance back */
    CHECK(sa_resource_release(engine, fourth) == SA_OK);
    CHECK(overview() && drawn == 0);
    CHECK(empty() && fake.xs[0].size() == 6 && fake.xs[0][0] == 3 &&
          fake.xs[0][3] == 7);
    CHECK(overview() && drawn == 0);
    CHECK(empty());
    take(engine, retired);
    CHECK(gone(third) == 0 && gone(fourth) == 0);
    CHECK(finish() && gone(third) == 1 && gone(fourth) == 1);
    CHECK(third_bytes.take_back() && fourth_bytes.take_back());
    CHECK(overview() && drawn == 0);
    CHECK(empty() && finish() && stats.stayed == 0);
    CHECK(sa_present_ended(engine, 8) == SA_OK &&
          sa_present_ended(engine, 9) == SA_OK);

    /* a scene's part ends where its mesh goes while a picture that drew it
     * is open. the draw leaves no instance in its place */
    sa_scene scene{};
    sa_draw part = draw;

    scene.head = {SA_REC_SCENE, sizeof scene};
    scene.scene = 0x0100;
    part.mesh = second;
    part.materials = plain;
    part.instance = SA_INSTANCE_SCENE | 1;
    part.object_to_world[3] = 9;
    CHECK(sa_scene_begin(engine, &scene) == SA_OK &&
          sa_scene_shade(engine, &shade) == SA_OK &&
          sa_scene_part(engine, &part, 1) == SA_OK &&
          sa_scene_end(engine) == SA_OK);
    CHECK(overview() && drawn == 1 && fake.first_x[0] == 9);
    CHECK(begin(tick) && sa_present_draw(engine, &part, 1) == SA_OK);
    CHECK(sa_resource_release(engine, second) == SA_OK);
    CHECK(sa_present_end(engine) == SA_OK);
    tick += 2;
    slot ^= 1;
    CHECK(overview() && drawn == 0);
    CHECK(empty() && empty() && finish() && stats.parts == 0 &&
          gone(second) == 1);
    CHECK(second_bytes.take_back());
    CHECK(overview() && drawn == 0);
    CHECK(sa_scene_ended(engine, scene.scene) == SA_OK);

    /* a part is given again once its mesh went, its scene open across the
     * picture that drew it. the scene keeps the part as last given, where
     * that put it. nothing of the draw before stays */
    sa_handle fifth = SA_NULL;
    sa_handle sixth = SA_NULL;
    Page fifth_bytes(&triangle, sizeof triangle);
    Page sixth_bytes(&triangle, sizeof triangle);

    CHECK(fifth_bytes.bytes != MAP_FAILED && sixth_bytes.bytes != MAP_FAILED);
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, fifth_bytes.bytes,
               sizeof triangle, &fifth) == SA_OK);
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, sixth_bytes.bytes,
               sizeof triangle, &sixth) == SA_OK);
    scene.scene = 0x0200;
    part.mesh = fifth;
    CHECK(sa_scene_begin(engine, &scene) == SA_OK &&
          sa_scene_shade(engine, &shade) == SA_OK &&
          sa_scene_part(engine, &part, 1) == SA_OK);
    CHECK(begin(tick) && sa_present_draw(engine, &part, 1) == SA_OK);
    CHECK(sa_resource_release(engine, fifth) == SA_OK);
    part.mesh = sixth;
    part.object_to_world[3] = 11;
    CHECK(sa_scene_part(engine, &part, 1) == SA_OK &&
          sa_scene_end(engine) == SA_OK);
    CHECK(sa_present_end(engine) == SA_OK);
    tick += 2;
    slot ^= 1;
    CHECK(overview() && drawn == 1 && fake.first_x[0] == 11);
    CHECK(empty() && empty() && finish() && stats.parts == 1 &&
          gone(fifth) == 1);
    CHECK(fifth_bytes.take_back());
    CHECK(overview() && drawn == 1 && fake.first_x[0] == 11);
    CHECK(empty() && finish() && stats.parts == 1);
    CHECK(sa_scene_ended(engine, scene.scene) == SA_OK);
    CHECK(overview() && drawn == 0);
    /* each of the ten released retired once, and nothing else did */
    CHECK(sa_resource_release(engine, sixth) == SA_OK &&
          sa_resource_release(engine, plain) == SA_OK);
    CHECK(empty() && empty() && finish());
    CHECK(retired.size() == 10 && gone(plain) == 1 && gone(textured) == 1 &&
          gone(sixth) == 1);
    sa_engine_destroy(engine);
    fake = Fake{};

    /* the scene by its own calls. a handle whose slot's generation wrapped
     * names another resource after its release. the draw before the release
     * leaves no instance. the draw after leaves one, of its own mesh */
    pc::World world;
    pc::MeshView before;
    pc::MeshView after;
    Triangle other;
    const uint32_t no_texture = 0;
    const uint32_t some_texture = 77;
    sa_handle name = (static_cast<sa_handle>(SA_KIND_MESH) << 28) | 1;

    CHECK(pc::mesh_view(&triangle, sizeof triangle, before) &&
          pc::mesh_view(&other, sizeof other, after));
    world.open(4);
    draw.mesh = name;
    draw.materials = (static_cast<sa_handle>(SA_KIND_MATERIAL) << 28) | 2;
    draw.instance = 5;
    world.begin(picture_at(10), 1);
    world.draw(draw, before, &material, &no_texture, shade);
    world.released(name, 0);
    world.draw(draw, after, &material, &no_texture, shade);
    world.end();
    CHECK(world.residents().size() == 1 &&
          (world.residents()[0].flags & pc::RESIDENT_SHAPE) != 0 &&
          world.residents()[0].lent.points == after.points);
    /* a texture released after the draw: the instance, and no shape */
    draw.instance = 6;
    world.begin(picture_at(12), 2);
    world.draw(draw, after, &material, &some_texture, shade);
    world.released((static_cast<sa_handle>(SA_KIND_TEXTURE) << 28) | 3,
                   some_texture);
    world.end();
    CHECK(world.residents().size() == 2 &&
          world.residents()[1].key.instance == 6 &&
          (world.residents()[1].flags & pc::RESIDENT_SHAPE) == 0 &&
          (world.residents()[1].flags & pc::RESIDENT_SEEN) != 0);
    /* a release between two pictures ends what names it, and a picture
     * that draws nothing brings none back */
    world.released(name, 0);
    CHECK(world.residents().empty());
    world.begin(picture_at(14), 3);
    world.end();
    CHECK(world.residents().empty());
    /* a picture draws an instance with another mesh, which goes while the
     * picture is open. the instance stays as if the picture missed it, with
     * the live mesh it had */
    sa_handle lasting = (static_cast<sa_handle>(SA_KIND_MESH) << 28) | 4;
    sa_handle passing = (static_cast<sa_handle>(SA_KIND_MESH) << 28) | 5;

    draw.instance = 7;
    draw.thing = 9;
    draw.mesh = lasting;
    world.begin(picture_at(16), 4);
    world.draw(draw, before, &material, &no_texture, shade);
    world.end();
    draw.mesh = passing;
    world.begin(picture_at(18), 5);
    world.draw(draw, after, &material, &no_texture, shade);
    world.released(passing, 0);
    world.end();
    CHECK(world.residents().size() == 1 &&
          world.residents()[0].mesh == lasting &&
          world.residents()[0].lent.points == before.points &&
          (world.residents()[0].flags & pc::RESIDENT_SHAPE) != 0 &&
          (world.residents()[0].flags & pc::RESIDENT_SEEN) == 0);
    return 0;
}

/* what a client lent comes back before it frees a byte. under a rate a
 * release waits one picture more than the device needs, for the pictures
 * between. with no next picture, the last one's fence lets nothing go.
 * sa_pc_retire waits for the fences and lets the scene's pictures go, and
 * every release retires, once. the picture after stands alone, and reads
 * nothing of the bytes taken back. a client that ends does the same, and
 * counts its loans back. it frees none it has no retirement for. the next
 * client on the engine joins none of its pictures. the test lets a fence go
 * only where a comment says the device finishes */
int loans()
{
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 16;
    desc.events = 64;
    desc.platform = &options;
    Triangle triangle;
    sa_material material{};
    static sa_palettes palettes;
    sa_shade shade{};
    sa_draw draw{};
    sa_pc_stats stats{};
    std::vector<sa_handle> retired;
    std::vector<uint8_t> rgba(SA_PC_WIDTH * SA_PC_HEIGHT * 4);
    auto rgba_bytes = static_cast<uint32_t>(rgba.size());
    sa_picture above = picture_at(0);
    uint32_t drawn = 0;

    shade.head = {SA_REC_SHADE, sizeof shade};
    draw.head = {SA_REC_DRAW, sizeof draw};
    draw.payload = SA_NO_PAYLOAD;
    draw.instance = 3;
    draw.object_to_world[0] = draw.object_to_world[5] =
        draw.object_to_world[10] = 1;
    auto begin = [&](uint32_t tick) {
        sa_picture picture = picture_at(tick);

        fake.frames.clear();
        fake.first_x.clear();
        return sa_present_begin(engine, &picture) == SA_OK &&
               sa_present_shade(engine, &shade) == SA_OK &&
               sa_present_palettes(engine, &palettes) == SA_OK;
    };
    auto give = [&](sa_handle mesh, sa_handle materials, float x) {
        sa_draw d = draw;

        d.mesh = mesh;
        d.materials = materials;
        d.object_to_world[3] = x;
        return sa_present_draw(engine, &d, 1) == SA_OK;
    };
    auto show = [&](uint32_t tick, sa_handle mesh, sa_handle materials,
                    float x) {
        return begin(tick) && give(mesh, materials, x) &&
               sa_present_end(engine) == SA_OK;
    };
    auto gone = [&](sa_handle handle) {
        return std::count(retired.begin(), retired.end(), handle);
    };

    /* without a rate a release waits for the pictures begun, and the call
     * waits for their fences */
    {
        Page mesh_bytes(&triangle, sizeof triangle);
        sa_handle mesh = SA_NULL;
        sa_handle list = SA_NULL;

        CHECK(mesh_bytes.bytes != MAP_FAILED);
        CHECK(sa_engine_memory(&desc, &bytes) == SA_OK &&
              bytes <= sizeof block);
        CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
        CHECK(sa_pc_retire(nullptr) == SA_ERR_INVALID &&
              sa_pc_retire(engine) == SA_OK);
        CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, mesh_bytes.bytes,
                   sizeof triangle, &mesh) == SA_OK);
        CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &material,
                   sizeof material, &list) == SA_OK);
        CHECK(show(10, mesh, list, 3));
        CHECK(sa_resource_release(engine, mesh) == SA_OK);
        take(engine, retired);
        CHECK(retired.empty());
        fake.waits = 0;
        CHECK(sa_pc_retire(engine) == SA_OK && fake.waits == 1);
        take(engine, retired);
        CHECK(retired.size() == 1 && gone(mesh) == 1);
        CHECK(mesh_bytes.take_back());
        sa_engine_destroy(engine);
        fake = Fake{};
        retired.clear();
    }

    options.rate = STEP_RATE;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    {
        Page mesh_bytes(&triangle, sizeof triangle);
        Page list_bytes(&material, sizeof material);
        Page next_mesh_bytes(&triangle, sizeof triangle);
        Page next_list_bytes(&material, sizeof material);
        sa_handle mesh = SA_NULL;
        sa_handle list = SA_NULL;

        CHECK(mesh_bytes.bytes != MAP_FAILED &&
              list_bytes.bytes != MAP_FAILED &&
              next_mesh_bytes.bytes != MAP_FAILED &&
              next_list_bytes.bytes != MAP_FAILED);
        CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, mesh_bytes.bytes,
                   sizeof triangle, &mesh) == SA_OK);
        CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS,
                   list_bytes.bytes, sizeof material, &list) == SA_OK);
        /* the second picture's between shows the mesh halfway */
        CHECK(show(10, mesh, list, 3) && show(12, mesh, list, 5) &&
              fake.first_x.size() == 2 && fake.first_x[0] == 4);
        /* with a picture open the call is refused and lets nothing go. the
         * picture still joins the one before it */
        CHECK(begin(14) && sa_pc_retire(engine) == SA_ERR_INVALID &&
              give(mesh, list, 7) && sa_present_end(engine) == SA_OK &&
              fake.first_x[0] == 6);
        CHECK(sa_resource_release(engine, mesh) == SA_OK &&
              sa_resource_release(engine, list) == SA_OK);
        take(engine, retired);
        CHECK(retired.empty());
        /* the call waits for the fences of the two pictures in flight */
        fake.waits = 0;
        CHECK(sa_pc_retire(engine) == SA_OK && fake.waits == 2);
        take(engine, retired);
        CHECK(retired.size() == 2 && gone(mesh) == 1 && gone(list) == 1);
        CHECK(mesh_bytes.take_back() && list_bytes.take_back());
        /* the picture after the call stands alone, though it is no cut.
         * each picture shown for it is its own, whole. the one after it
         * joins it */
        CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, next_mesh_bytes.bytes,
                   sizeof triangle, &mesh) == SA_OK);
        CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS,
                   next_list_bytes.bytes, sizeof material, &list) == SA_OK);
        CHECK(show(16, mesh, list, 9) && fake.first_x.size() == 2 &&
              fake.first_x[0] == 9 && fake.first_x[1] == 9);
        CHECK(sa_pc_stats_read(engine, &stats) == SA_OK && stats.moved == 0 &&
              stats.stayed == 0);
        CHECK(show(18, mesh, list, 11) && fake.first_x[0] == 10);
        fake.first_x.clear();
        CHECK(sa_pc_read_overview(engine, &above, rgba.data(), rgba_bytes,
                                  &drawn) == SA_ERR_UNSUPPORTED &&
              drawn == 1 && fake.first_x[0] == 11);
        /* released between pictures, each waits for the picture after the
         * last. the device finishes every picture, and neither retires */
        CHECK(sa_resource_release(engine, mesh) == SA_OK &&
              sa_resource_release(engine, list) == SA_OK);
        for (bool &b : fake.busy) {
            b = false;
        }
        CHECK(sa_pc_stats_read(engine, &stats) == SA_OK);
        take(engine, retired);
        CHECK(retired.size() == 2);
        CHECK(sa_pc_retire(engine) == SA_OK);
        take(engine, retired);
        CHECK(retired.size() == 4 && gone(mesh) == 1 && gone(list) == 1);
        CHECK(next_mesh_bytes.take_back() && next_list_bytes.take_back());
        /* nothing waits, and nothing more retires */
        CHECK(sa_pc_retire(engine) == SA_OK);
        take(engine, retired);
        CHECK(retired.size() == 4);
        CHECK(sa_pc_read_overview(engine, &above, rgba.data(), rgba_bytes,
                                  &drawn) == SA_ERR_UNSUPPORTED &&
              drawn == 0);
        CHECK(begin(20) && sa_present_end(engine) == SA_OK &&
              sa_pc_stats_read(engine, &stats) == SA_OK && stats.stayed == 0);
    }
    sa_engine_destroy(engine);
    fake = Fake{};

    /* a client draws two pictures of its mesh, and ends with both fences
     * standing. it waits for them and takes every event. it then frees what
     * it lent, with nothing left to read it */
    const char *const no_roots[] = {nullptr};
    sa_cap_mesh mesh{};
    sa_cap_material cap_material{};
    sa_cap_materials list{};
    uint32_t material_id = 0;
    std::vector<uint8_t> ids(sizeof material_id, 0);
    static sa_cap_palettes cap_palettes;
    sa_cap_picture picture{};
    sa_cap_end end{};

    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    mesh.format = SA_MESH_NINJA_INLINE;
    cap_material.texture = SA_CAP_NONE;
    list.count = 1;
    auto names = [&](sa_pc_client *client) {
        for (const std::vector<uint8_t> &r :
             {record(SA_CAP_MESH, mesh, inline_model()),
              record(SA_CAP_MATERIAL, cap_material),
              record(SA_CAP_MATERIALS, list, ids), record(SA_REC_SHADE, shade),
              record(SA_CAP_PALETTES, cap_palettes)}) {
            if (sa_pc_client_record(client, r.data()) != SA_OK) {
                return false;
            }
        }
        return true;
    };
    /* a picture's records up to its end's, its tick two past the last's */
    auto open = [&](sa_pc_client *client, float x) {
        sa_draw d = draw;

        d.object_to_world[3] = x;
        picture.tick += 2;
        fake.frames.clear();
        fake.first_x.clear();
        std::vector<uint8_t> opened = record(SA_CAP_PICTURE, picture);
        std::vector<uint8_t> drew = record(SA_REC_DRAW, d);

        return sa_pc_client_record(client, opened.data()) == SA_OK &&
               sa_pc_client_record(client, drew.data()) == SA_OK;
    };
    auto close = [&](sa_pc_client *client) {
        std::vector<uint8_t> ended = record(SA_CAP_END, end);

        picture.picture++;
        end.picture++;
        return sa_pc_client_record(client, ended.data()) == SA_PC_ENDED;
    };
    sa_pc_client *first = sa_pc_client_create(engine, no_roots);

    CHECK(names(first) && open(first, 3) && close(first) && open(first, 5) &&
          close(first) && fake.first_x[0] == 4);
    fake.waits = 0;
    CHECK(sa_pc_client_destroy(first) == SA_OK && fake.waits == 2);
    CHECK(polled(engine) == 0);
    /* another client on the engine draws the same mesh, its ticks after the
     * first's. its first picture joins nothing, and its second joins it */
    sa_pc_client *second = sa_pc_client_create(engine, no_roots);
    auto *client = reinterpret_cast<pc::Client *>(second);

    CHECK(names(second) && open(second, 9) && close(second) &&
          fake.first_x.size() == 2 && fake.first_x[0] == 9);
    CHECK(sa_pc_stats_read(engine, &stats) == SA_OK && stats.moved == 0 &&
          stats.stayed == 0);
    CHECK(open(second, 11) && close(second) && fake.first_x[0] == 10);
    CHECK(client->failures.empty() && client->lent.size() == 2);
    /* inside a picture a client stays, and has released nothing */
    CHECK(sa_pc_client_destroy(nullptr) == SA_ERR_INVALID);
    CHECK(open(second, 13) && sa_pc_client_destroy(second) == SA_ERR_INVALID &&
          client->totals.released == 0 && client->lent.size() == 2);
    CHECK(close(second) && fake.first_x[0] == 12);
    /* the capture frees the mesh, and the test takes its retirement from
     * the engine ahead of the client. with none for the mesh, the client
     * refuses its end and keeps the mesh's bytes */
    sa_cap_life life{};
    std::vector<sa_handle> taken;

    life.kind = SA_KIND_MESH;
    std::vector<uint8_t> freed = record(SA_CAP_FREE, life);
    CHECK(sa_pc_client_record(second, freed.data()) == SA_OK);
    CHECK(open(second, 15) && close(second));
    for (bool &b : fake.busy) {
        b = false;
    }
    CHECK(sa_pc_stats_read(engine, &stats) == SA_OK);
    take(engine, taken);
    CHECK(taken.size() == 1 && client->lent.count(taken[0]) == 1);
    CHECK(sa_pc_client_destroy(second) == SA_ERR_INVALID &&
          client->lent.size() == 1 && client->lent.count(taken[0]) == 1);
    /* no call tells the client of the retirement, so it refuses its end
     * again. the process keeps it from here */
    CHECK(sa_pc_client_destroy(second) == SA_ERR_INVALID &&
          client->lent.size() == 1 && polled(engine) == 0);
    stays = second;
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* the render-owned scene's room for a picture, at each of its stores and
 * one draw past. a picture the stores hold is kept whole. the picture
 * between it and the last moves every draw both made. one past a store is
 * kept as none. each picture the display shows for it is the game's own,
 * whole. the figures name the store and count those pictures. the instances
 * stay as the picture before left them. the picture after stands alone, and
 * the one after that joins it. a picture between two kept whole may lack
 * room for a draw only the older made. it is then the game's own picture,
 * and so is each shown after it */
int rooms()
{
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 16;
    desc.events = 64;
    desc.platform = &options;
    options.rate = STEP_RATE;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    Triangle triangle;
    static sa_material materials[1024];
    static sa_palettes palettes;
    sa_handle mesh = SA_NULL;
    sa_handle list = SA_NULL;
    sa_shade shade{};
    sa_draw draw{};
    sa_sprite sprite{};
    sa_triangles strip{};
    sa_pc_stats stats{};
    std::vector<uint8_t> rgba(SA_PC_WIDTH * SA_PC_HEIGHT * 4);
    auto rgba_bytes = static_cast<uint32_t>(rgba.size());
    sa_picture above = picture_at(0);
    uint32_t drawn = 0;
    uint32_t tick = 10;

    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &triangle, sizeof triangle,
               &mesh) == SA_OK);
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, materials,
               sizeof materials, &list) == SA_OK);
    shade.head = {SA_REC_SHADE, sizeof shade};
    draw.head = {SA_REC_DRAW, sizeof draw};
    draw.mesh = mesh;
    draw.materials = list;
    draw.payload = SA_NO_PAYLOAD;
    draw.instance = 5;
    draw.object_to_world[0] = draw.object_to_world[5] =
        draw.object_to_world[10] = 1;
    sprite.head = {SA_REC_SPRITE, sizeof sprite};
    sprite.instance = 9;
    sprite.flags = SA_SPRITE_SCREEN;
    sprite.object_to_world[0] = sprite.object_to_world[5] = 1.0F;
    sprite.corner[1][0] = sprite.corner[1][1] = 8.0F;
    sprite.scale[0] = sprite.scale[1] = 1.0F;
    strip.head = {SA_REC_TRIANGLES, sizeof strip};
    strip.form = SA_TRIANGLES_STRIP;
    strip.flags = SA_TRIANGLES_SCREEN;
    /* begin a picture two ticks after the last, its view down z with the
     * near plane at 1. end ends it and reads its figures */
    auto begin = [&](uint16_t flags = 0) {
        sa_picture picture = picture_at(tick);

        tick += 2;
        picture.flags = flags;
        picture.view.world_to_view[0] = picture.view.world_to_view[5] =
            picture.view.world_to_view[10] = 1.0F;
        picture.view.focal[0] = picture.view.focal[1] = 320.0F;
        picture.view.near_z = 1.0F;
        fake.frames.clear();
        fake.first_x.clear();
        fake.xs.clear();
        for (bool &b : fake.busy) {
            b = false;
        }
        (void)polled(engine);
        return sa_present_begin(engine, &picture) == SA_OK &&
               sa_present_shade(engine, &shade) == SA_OK &&
               sa_present_palettes(engine, &palettes) == SA_OK;
    };
    auto end = [&](uint32_t shown = 2) {
        return sa_present_end(engine) == SA_OK &&
               sa_pc_stats_read(engine, &stats) == SA_OK &&
               fake.xs.size() == shown;
    };
    /* whether the picture shown ahead of the picture's own lies between it
     * and the last. it moved count draws, the first to x. any other waits
     * for its own picture, as one the last never drew */
    auto joined = [&](uint32_t count, float x) {
        return stats.full == 0 && stats.repeated == 0 && stats.shown == 2 &&
               stats.moved == count &&
               stats.moved + stats.born == stats.draws &&
               (stats.born != 0 || fake.xs[0].size() == fake.xs[1].size()) &&
               fake.xs[0][0] == x;
    };
    /* whether it is the picture's own again, whole, for a store that had no
     * room */
    auto own = [&](uint32_t store) {
        return stats.full == store && stats.repeated == 1 && stats.shown == 2 &&
               stats.moved == 0 && !fake.xs[1].empty() &&
               fake.xs[0] == fake.xs[1];
    };
    /* whether it is the picture's own for standing alone, as after a cut */
    auto alone = [&]() {
        return stats.full == 0 && stats.repeated == 0 && stats.shown == 2 &&
               stats.moved == 0 && fake.xs[0] == fake.xs[1];
    };
    auto overview = [&]() {
        fake.first_x.clear();
        return sa_pc_read_overview(engine, &above, rgba.data(), rgba_bytes,
                                   &drawn) == SA_ERR_UNSUPPORTED &&
               fake.first_x.size() == 1;
    };

    /* the tasks that end while a picture is open: 4,096, and one more */
    auto ending = [&](uint32_t count, float x) {
        sa_draw d = draw;

        d.object_to_world[3] = x;
        if (!begin() || sa_present_draw(engine, &d, 1) != SA_OK) {
            return false;
        }
        for (uint32_t i = 0; i < count; i++) {
            if (sa_present_ended(engine, 1000 + i) != SA_OK) {
                return false;
            }
        }
        return end();
    };
    CHECK(ending(4096, 0) && alone());
    CHECK(ending(4096, 2) && joined(1, 1));
    CHECK(overview() && drawn == 1 && fake.first_x[0] == 2);
    /* kept as none, the picture leaves the mesh's instance where the one
     * before drew it. the picture after stands alone */
    CHECK(ending(4097, 4) && own(SA_PC_FULL_ENDED));
    CHECK(overview() && drawn == 1 && fake.first_x[0] == 2);
    CHECK(ending(4096, 6) && alone());
    CHECK(overview() && drawn == 1 && fake.first_x[0] == 6);
    CHECK(ending(4096, 8) && joined(1, 7));

    /* a picture holds 65536 draws, sprites and triangles. here they are a
     * mesh and sprites, and then one more. sprites the last picture lacked
     * wait for their own picture */
    auto crowd = [&](uint32_t count, float x) {
        std::vector<sa_sprite> sprites(count - 1, sprite);
        sa_draw d = draw;

        d.object_to_world[3] = x;
        for (uint32_t i = 0; i < count - 1; i++) {
            sprites[i].at[0] = x + static_cast<float>(i % 512);
        }
        return begin() && sa_present_draw(engine, &d, 1) == SA_OK &&
               sa_present_sprite(engine, sprites.data(), count - 1) == SA_OK &&
               end();
    };
    CHECK(crowd(65536, 10) && joined(1, 9));
    CHECK(crowd(65536, 12) && joined(65536, 11));
    CHECK(crowd(65537, 14) && own(SA_PC_FULL_DRAWS));

    /* a picture holds 262144 of its meshes' materials' textures. here 256
     * draws take two meshes of 1024 materials by turns. one more draw of the
     * mesh last drawn adds none, so it fits. a draw of a mesh of one material
     * adds one texture too many */
    struct Many {
        sa_mesh mesh = {3, 3, 1, 1024};
        float points[9] = {0, 0, 1, 1, 0, 1, 0, 1, 1};
        float normals[9] = {0, 0, 1, 0, 0, 1, 0, 0, 1};
        sa_meshset set = {1023, 0, 3};
        uint16_t corners[4] = {0, 1, 2, 0};
    } many[2];
    sa_handle manies[2] = {SA_NULL, SA_NULL};

    for (int i = 0; i < 2; i++) {
        CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &many[i],
                   sizeof many[i], &manies[i]) == SA_OK);
    }
    auto listed = [&](uint32_t count, float x, uint16_t flags = 0,
                      bool again = false) {
        std::vector<sa_draw> draws(count, draw);

        for (uint32_t i = 0; i < count; i++) {
            draws[i].mesh = i < 256 ? manies[i & 1] : again ? manies[1] : mesh;
            draws[i].instance = 100 + i;
            draws[i].object_to_world[3] = x;
        }
        return begin(flags) &&
               sa_present_draw(engine, draws.data(), count) == SA_OK && end();
    };
    CHECK(listed(256, 20, SA_PICTURE_CUT) && alone());
    CHECK(listed(256, 22) && joined(256, 21));
    CHECK(listed(257, 24, 0, true) && joined(256, 23));
    CHECK(listed(257, 26) && own(SA_PC_FULL_TEXTURES));

    /* a picture holds 262144 triangle points. here two lists of 65535 and
     * 21845 triangles and a strip of 2 fill it. a strip of 3 is one point
     * more */
    std::vector<sa_point> points(3 * 65535);
    auto pointed = [&](uint16_t last, float x, uint16_t flags = 0) {
        const uint16_t counts[3] = {65535, 21845, last};

        for (sa_point &p : points) {
            p.at[0] = x;
        }
        if (!begin(flags)) {
            return false;
        }
        for (uint32_t i = 0; i < 3; i++) {
            sa_triangles t = strip;

            t.instance = 30 + i;
            t.count = counts[i];
            t.form = i == 2 ? SA_TRIANGLES_STRIP : SA_TRIANGLES_LIST;
            if (sa_present_triangles(engine, &t, points.data()) != SA_OK) {
                return false;
            }
        }
        return end();
    };
    CHECK(pointed(2, 30, SA_PICTURE_CUT) && alone());
    CHECK(pointed(2, 32) && joined(3, 31));
    CHECK(pointed(3, 34) && own(SA_PC_FULL_POINTS));

    /* a picture between two kept whole, with no room for a draw. a picture
     * counts a mesh's triangles as it takes them, and a line's as it places
     * them. a line behind the near plane places none. so a picture of such
     * lines, then meshes, has room for all. the picture between it and one of
     * the meshes alone takes the meshes first. it then has no room for the
     * lines only the older picture drew */
    constexpr uint32_t BIG = 65537; /* triangles */
    Triangle head;
    const size_t corners_at = sizeof head - sizeof head.corners;
    std::vector<uint8_t> big(corners_at + ((BIG * 6 + 3) & ~3U));
    sa_handle large = SA_NULL;
    sa_triangles lines = strip;
    std::vector<sa_point> ends(2 * 65535);

    head.set.corners = BIG * 3;
    memcpy(big.data(), &head, corners_at);
    for (uint32_t i = 0; i < BIG * 3; i++) {
        uint16_t corner = static_cast<uint16_t>(i % 3);

        memcpy(big.data() + corners_at + (i * 2), &corner, sizeof corner);
    }
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, big.data(),
               static_cast<uint32_t>(big.size()), &large) == SA_OK);
    lines.instance = 40;
    lines.form = SA_TRIANGLES_LINES;
    lines.flags = 0;
    lines.count = 65535;
    lines.object_to_world[0] = lines.object_to_world[5] =
        lines.object_to_world[10] = 1.0F;
    for (sa_point &p : ends) {
        p.at[2] = 0.5F;
    }
    auto heavy = [&](bool lined, float x, uint16_t flags = 0,
                     uint32_t shown = 2) {
        sa_draw draws[6];

        for (uint32_t i = 0; i < 6; i++) {
            draws[i] = draw;
            draws[i].mesh = large;
            draws[i].instance = 50 + i;
            draws[i].object_to_world[3] = x;
        }
        return begin(flags) &&
               (!lined ||
                sa_present_triangles(engine, &lines, ends.data()) == SA_OK) &&
               sa_present_draw(engine, draws, 6) == SA_OK && end(shown);
    };
    /* the tasks whose instances the scene kept from above end first */
    for (uint32_t instance = 0; instance < 512; instance++) {
        CHECK(sa_present_ended(engine, instance) == SA_OK);
    }
    CHECK(heavy(true, 40, SA_PICTURE_CUT) && alone() &&
          stats.triangles == 6 * BIG);
    CHECK(heavy(false, 42) && own(SA_PC_FULL_BETWEEN) &&
          stats.triangles == 6 * BIG);
    /* the scene kept that picture: its instances stand where it drew them */
    CHECK(overview() && drawn == 6 && fake.first_x[0] == 42);
    /* four steps on, the display shows four pictures for one. the lines
     * stay to the half, so the first between has no room. each from it on
     * is the game's own picture, though the third lies past the half. so
     * none steps back */
    CHECK(heavy(true, 44, SA_PICTURE_CUT) && alone());
    tick += 2;
    CHECK(heavy(false, 48, 0, 4) && stats.shown == 4 &&
          stats.full == SA_PC_FULL_BETWEEN && stats.repeated == 3 &&
          stats.moved == 0);
    for (uint32_t i = 0; i < 3; i++) {
        CHECK(!fake.xs[3].empty() && fake.xs[i] == fake.xs[3]);
    }
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* an SA_REC_VIEW of id, its world to view by rows. its projection is zeros,
 * as picture_at's is */
sa_picture_view view_of(uint32_t id, const float rows[12])
{
    sa_picture_view made{};

    made.head = {SA_REC_VIEW, sizeof made};
    made.view = id;
    memcpy(made.value.world_to_view, rows, sizeof made.value.world_to_view);
    return made;
}

bool at(const pc::Vertex &v, float x, float y, float z)
{
    return fabsf(v.at[0] - x) < 1e-5F && fabsf(v.at[1] - y) < 1e-5F &&
           fabsf(v.at[2] - z) < 1e-5F;
}

/* a picture's views. each draw shows under the view it names. the raster
 * looks through the view the picture's first draw names. every point is
 * placed in its space. a draw under that view goes as it is given. a draw
 * under another is carried there with its light. the kept scene holds each
 * draw where it stands in the world. a picture between two shows each draw
 * under a view between the two it was made under. a view's call, and a draw
 * that names one, are refused as sa_engine.h says */
int views()
{
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;
    const float still[12] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    /* a quarter turn about y, the eye 4 along x. a point (x, y, z) shows at
     * (z, y, 4 - x) */
    const float turned[12] = {0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 4};
    std::vector<uint8_t> rgba(SA_PC_WIDTH * SA_PC_HEIGHT * 4);
    auto rgba_bytes = static_cast<uint32_t>(rgba.size());
    uint32_t kept = 0;

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 8;
    desc.events = 16;
    desc.platform = &options;
    options.rate = STEP_RATE;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    Triangle triangle;
    Pixel pixel;
    sa_handle mesh = SA_NULL;
    sa_handle materials = SA_NULL;
    sa_handle texture = SA_NULL;
    sa_material material{};
    sa_shade shade{};
    sa_draw draw{};
    sa_sprite sprite{};
    sa_triangles strip{};
    sa_point points[3] = {};
    sa_pc_stats stats{};

    material.diffuse = 0xffffffff;
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &triangle, sizeof triangle,
               &mesh) == SA_OK);
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &material,
               sizeof material, &materials) == SA_OK);
    CHECK(make(engine, SA_KIND_TEXTURE, SA_FORMAT_PIXELS, &pixel, sizeof pixel,
               &texture) == SA_OK);
    /* Ninja's ramp: a point whose normal lies half against the light takes
     * half its material's color */
    shade.head = {SA_REC_SHADE, sizeof shade};
    shade.mode = SA_SHADE_RAMP;
    shade.diffuse = 1.0F;
    shade.light[2] = -0.5F;
    draw.head = {SA_REC_DRAW, sizeof draw};
    draw.mesh = mesh;
    draw.materials = materials;
    draw.payload = SA_NO_PAYLOAD;
    draw.instance = 5;
    draw.object_to_world[0] = draw.object_to_world[5] =
        draw.object_to_world[10] = 1;
    sprite.head = {SA_REC_SPRITE, sizeof sprite};
    sprite.texture = texture;
    sprite.instance = 9;
    sprite.object_to_world[0] = sprite.object_to_world[5] =
        sprite.object_to_world[10] = 1;
    sprite.at[2] = 1.0F;
    sprite.corner[1][0] = sprite.corner[1][1] = 8.0F;
    sprite.scale[0] = sprite.scale[1] = 1.0F;
    strip.head = {SA_REC_TRIANGLES, sizeof strip};
    strip.instance = 10;
    strip.count = 1;
    strip.form = SA_TRIANGLES_STRIP;
    strip.object_to_world[0] = strip.object_to_world[5] =
        strip.object_to_world[10] = 1;
    /* show a picture at tick with flags, its own view own and view 1 given.
     * its first draw names view first, 0 or 1, at the world's origin. unless
     * other is false, a second names the other view, 2 along x */
    auto show = [&](uint32_t tick, const float *own, const float *given,
                    uint8_t first, uint16_t flags = 0, bool other = true) {
        sa_picture picture = picture_at(tick);
        sa_picture_view named = view_of(1, given);
        sa_draw d = draw;

        picture.flags = flags;
        memcpy(picture.view.world_to_view, own,
               sizeof picture.view.world_to_view);
        fake.frames.clear();
        fake.xs.clear();
        for (bool &b : fake.busy) {
            b = false;
        }
        if (sa_present_begin(engine, &picture) != SA_OK ||
            sa_present_shade(engine, &shade) != SA_OK ||
            sa_present_view(engine, &named) != SA_OK) {
            return false;
        }
        d.view = first;
        if (sa_present_draw(engine, &d, 1) != SA_OK) {
            return false;
        }
        d.view = static_cast<uint8_t>(1 - first);
        d.instance = 6;
        d.object_to_world[3] = 2;
        return (!other || sa_present_draw(engine, &d, 1) == SA_OK) &&
               sa_present_end(engine) == SA_OK &&
               sa_pc_stats_read(engine, &stats) == SA_OK;
    };
    sa_picture above = picture_at(0);

    memcpy(above.view.world_to_view, still, sizeof still);
    /* the first draw names the picture's own view, which the raster looks
     * through, so it goes as given. the other, 2 along x, shows where the
     * turned view shows it, lit as it was */
    CHECK(show(10, still, turned, 0));
    CHECK(memcmp(fake.uniform.view, still, sizeof still) == 0);
    CHECK(fake.last.size() == 6 && at(fake.last[0], 0, 0, 1) &&
          at(fake.last[3], 1, 0, 2) && at(fake.last[4], 1, 0, 1) &&
          at(fake.last[5], 1, 1, 2));
    CHECK((fake.last[0].base & 0xff) == 127 &&
          fake.last[0].base == fake.last[3].base);
    /* the kept scene holds it where it stands in the world: 2 along x */
    CHECK(sa_present_ended(engine, 5) == SA_OK);
    fake.first_x.clear();
    CHECK(sa_pc_read_overview(engine, &above, rgba.data(), rgba_bytes, &kept) ==
              SA_ERR_UNSUPPORTED &&
          kept == 1 && fake.first_x[0] == 2);
    /* the first draw names the other view, so the raster looks through it.
     * the draw under it goes as given, and the one under the picture's own
     * is carried */
    CHECK(show(12, still, turned, 1, SA_PICTURE_CUT));
    CHECK(memcmp(fake.uniform.view, turned, sizeof turned) == 0);
    CHECK(at(fake.last[0], 0, 0, 1) && at(fake.last[3], 3, 0, 2) &&
          fake.last[0].base == fake.last[3].base);
    /* a cut: the picture between is the picture whole, placed as it is */
    CHECK(fake.xs.size() == 2 && fake.xs[0] == fake.xs[1]);
    /* two pictures whose other view lies 4 and then 8 along x. between
     * them, the draw under it shows as a view 6 along x shows it. the draw
     * under each picture's own view shows as it was */
    const float four[12] = {1, 0, 0, -4, 0, 1, 0, 0, 0, 0, 1, 0};
    const float eight[12] = {1, 0, 0, -8, 0, 1, 0, 0, 0, 0, 1, 0};

    CHECK(show(14, still, four, 0, SA_PICTURE_CUT));
    CHECK(show(16, still, eight, 0) && stats.moved == 2);
    CHECK(fake.xs.size() == 2 && fake.xs[0].size() == 6 && fake.xs[0][0] == 0 &&
          fabsf(fake.xs[0][3] + 4) < 1e-4F && fake.xs[1][3] == -6);
    /* the picture after doesn't draw it, and its own view lies 2 along x.
     * the older picture drew it 6 back along x from its own view. to the
     * half, it shows as far back from the view between the two */
    const float two[12] = {1, 0, 0, -2, 0, 1, 0, 0, 0, 0, 1, 0};

    CHECK(show(18, two, eight, 0, 0, false) && stats.stayed == 1);
    CHECK(fake.xs.size() == 2 && fake.xs[0].size() == 6 &&
          fabsf(fake.xs[0][3] + 6) < 1e-4F);
    CHECK(fabsf(fake.uniform.view[3] + 2) < 1e-6F);

    /* refused: a view outside a picture, a null one or one of id 0. so are
     * an id past SA_VIEWS, an id already given, and a NaN */
    sa_picture_view named = view_of(1, turned);
    sa_picture picture = picture_at(20);

    CHECK(sa_present_view(engine, &named) == SA_ERR_INVALID);
    for (bool &b : fake.busy) {
        b = false;
    }
    memcpy(picture.view.world_to_view, still, sizeof still);
    CHECK(sa_present_begin(engine, &picture) == SA_OK &&
          sa_present_shade(engine, &shade) == SA_OK);
    CHECK(sa_present_view(engine, nullptr) == SA_ERR_INVALID);
    named.view = 0;
    CHECK(sa_present_view(engine, &named) == SA_ERR_INVALID);
    named.view = SA_VIEWS + 1;
    CHECK(sa_present_view(engine, &named) == SA_ERR_INVALID);
    named.view = SA_VIEWS;
    CHECK(sa_present_view(engine, &named) == SA_OK);
    CHECK(sa_present_view(engine, &named) == SA_ERR_INVALID);
    named.view = 2;
    named.value.world_to_view[7] = NAN;
    CHECK(sa_present_view(engine, &named) == SA_ERR_INVALID);
    /* a view whose projection differs from the picture's own is taken */
    named = view_of(2, four);
    named.value.focal[0] = 500.0F;
    CHECK(sa_present_view(engine, &named) == SA_OK);
    /* a draw, sprite or triangles naming a view the picture never had is
     * refused. of two draws given together, neither is taken */
    sa_draw pair[2] = {draw, draw};

    pair[1].view = 3;
    CHECK(sa_present_draw(engine, pair, 2) == SA_ERR_HANDLE);
    pair[1].view = SA_VIEWS + 1;
    CHECK(sa_present_draw(engine, pair, 2) == SA_ERR_HANDLE);
    sprite.view = 3;
    CHECK(sa_present_sprite(engine, &sprite, 1) == SA_ERR_HANDLE);
    strip.view = 3;
    CHECK(sa_present_triangles(engine, &strip, points) == SA_ERR_HANDLE);
    /* the first draw names the view that projects another way, 4 along x.
     * the raster looks through its matrix but projects as the picture's own
     * view does. that draw goes as given. a draw, a sprite and triangles
     * under the turned view show where that view shows them. their point at
     * (x, y, z) lies at (z + 4, y, 4 - x) */
    pair[0].view = 2;
    pair[1].view = SA_VIEWS;
    sprite.view = SA_VIEWS;
    strip.view = SA_VIEWS;
    CHECK(sa_present_draw(engine, pair, 2) == SA_OK &&
          sa_present_sprite(engine, &sprite, 1) == SA_OK &&
          sa_present_triangles(engine, &strip, points) == SA_OK);
    CHECK(sa_present_end(engine) == SA_OK &&
          sa_pc_stats_read(engine, &stats) == SA_OK && stats.draws == 4);
    CHECK(memcmp(fake.uniform.view, four, sizeof four) == 0 &&
          fake.uniform.project[0] == 0.0F);
    CHECK(fake.last.size() == 15 && at(fake.last[0], 0, 0, 1) &&
          at(fake.last[3], 5, 0, 4) && at(fake.last[6], 5, 0, 4) &&
          at(fake.last[8], 5, 0, -4) && at(fake.last[12], 4, 0, 4));
    /* a new picture starts with its own alone */
    for (bool &b : fake.busy) {
        b = false;
    }
    picture = picture_at(22);
    CHECK(sa_present_begin(engine, &picture) == SA_OK &&
          sa_present_shade(engine, &shade) == SA_OK);
    CHECK(sa_present_draw(engine, &pair[1], 1) == SA_ERR_HANDLE);
    /* its own view is all zeros, and its first draw names it. a view of
     * zeros has no inverse, so no draw under another view is carried into
     * it. such a draw is refused, and so are a sprite and triangles. those on
     * the screen are taken, as no view places them */
    named = view_of(1, still);
    CHECK(sa_present_view(engine, &named) == SA_OK &&
          sa_present_draw(engine, &draw, 1) == SA_OK);
    pair[1].view = 1;
    sprite.view = 1;
    strip.view = 1;
    CHECK(sa_present_draw(engine, &pair[1], 1) == SA_ERR_INVALID);
    CHECK(sa_present_sprite(engine, &sprite, 1) == SA_ERR_INVALID);
    CHECK(sa_present_triangles(engine, &strip, points) == SA_ERR_INVALID);
    sprite.flags = SA_SPRITE_SCREEN;
    strip.flags = SA_TRIANGLES_SCREEN;
    CHECK(sa_present_sprite(engine, &sprite, 1) == SA_OK);
    CHECK(sa_present_triangles(engine, &strip, points) == SA_OK);
    CHECK(sa_present_end(engine) == SA_OK &&
          sa_pc_stats_read(engine, &stats) == SA_OK && stats.draws == 3);
    /* a shade of lights, under the picture's own view and the turned one.
     * each draw is lit by the lights as its own view holds them. so the mesh
     * takes the same colors under each. the draw under the turned view is
     * carried, its normals with it. its first point takes half the
     * red of a light that shines half against its normal. it takes half the
     * green of a point light 2 above it, past its near. lit as the picture's
     * first view holds the lights, the carried draw would take neither */
    sa_lights lights{};
    sa_shade lit = shade;

    lights.head = {SA_REC_LIGHTS, sizeof lights};
    lights.shade = 2;
    lights.count = 2;
    lights.light[0].type = SA_LIGHT_DIRECTION;
    lights.light[0].direction[0] = sqrtf(0.75F);
    lights.light[0].direction[2] = -0.5F;
    lights.light[0].diffuse[0] = 1.0F;
    lights.light[1].type = SA_LIGHT_POINT;
    lights.light[1].point[2] = 3.0F;
    lights.light[1].diffuse[1] = 1.0F;
    lights.light[1].near = 2.0F;
    lights.light[1].far = 100.0F;
    lit.shade = 2;
    lit.mode = SA_SHADE_LIGHTS;
    for (bool &b : fake.busy) {
        b = false;
    }
    picture = picture_at(24);
    picture.flags = SA_PICTURE_CUT;
    memcpy(picture.view.world_to_view, still, sizeof still);
    named = view_of(1, turned);
    pair[0] = pair[1] = draw;
    pair[0].shade = pair[1].shade = 2;
    pair[1].view = 1;
    pair[1].instance = 6;
    CHECK(sa_present_begin(engine, &picture) == SA_OK &&
          sa_present_view(engine, &named) == SA_OK &&
          sa_present_lights(engine, &lights) == SA_OK &&
          sa_present_shade(engine, &lit) == SA_OK &&
          sa_present_draw(engine, pair, 2) == SA_OK &&
          sa_present_end(engine) == SA_OK);
    CHECK(fake.last.size() == 6 && at(fake.last[0], 0, 0, 1) &&
          at(fake.last[3], 1, 0, 4));
    CHECK(fake.last[0].base == 0xff007f7f);
    for (int k = 0; k < 3; k++) {
        CHECK(fake.last[3 + k].base == fake.last[k].base);
    }
    for (bool &b : fake.busy) {
        b = false;
    }
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

int run()
{
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.events = 16;
    desc.platform = &options;
    /* an engine with room for no resource is refused, and none is made */
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block &&
          sa_engine_create(&desc, block, bytes, &engine) == SA_ERR_INVALID &&
          engine == nullptr);
    desc.resources = 8;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);

    /* each picture the game draws shows once, so 60 shows the game's two
     * pictures a frame. the call that changes the aspect applies 30. 16:9
     * asked of the picture shows from the next picture, where the window is
     * 16:9 too. it gives 4:3 where the window is 4:3. a window wider than
     * the picture is refused, as is a request outside the contract */
    sa_mode asked{SA_ASPECT_4_3, 30, 0, 0};
    sa_mode applied{};

    CHECK(sa_present_mode(engine, &asked, &applied) == SA_OK &&
          applied.aspect == SA_ASPECT_4_3 && applied.rate == 30 &&
          applied.held == 0);
    asked.rate = 60;
    CHECK(sa_present_mode(engine, &asked, &applied) == SA_OK &&
          applied.rate == 60);
    asked = sa_mode{SA_ASPECT_16_9, 30, 0, 0};
    CHECK(sa_present_mode(engine, &asked, &applied) == SA_OK &&
          applied.aspect == SA_ASPECT_4_3);
    asked.rate = 60;
    CHECK(sa_present_window(engine, SA_ASPECT_16_9) == SA_OK &&
          sa_present_mode(engine, &asked, &applied) == SA_OK &&
          applied.aspect == SA_ASPECT_16_9 && applied.rate == 30);
    CHECK(sa_present_mode(engine, &asked, &applied) == SA_OK &&
          applied.aspect == SA_ASPECT_16_9 && applied.rate == 60);
    asked.rate = 30;
    asked.aspect = SA_ASPECT_4_3;
    CHECK(sa_present_mode(engine, &asked, &applied) == SA_ERR_UNSUPPORTED);
    asked.rate = 45;
    CHECK(sa_present_mode(engine, &asked, &applied) == SA_ERR_INVALID);
    CHECK(sa_present_window(engine, SA_ASPECT_16_9 + 1) == SA_ERR_INVALID);
    asked.rate = 30;
    CHECK(sa_present_window(engine, SA_ASPECT_4_3) == SA_OK &&
          sa_present_mode(engine, &asked, &applied) == SA_OK &&
          applied.aspect == SA_ASPECT_4_3);

    /* a mesh, a texture and materials go in. bad bytes don't. a corner of 5
     * names no point of the three */
    Triangle triangle;
    Triangle wrong;
    Pixel pixel;
    sa_handle mesh = SA_NULL;
    sa_handle texture = SA_NULL;
    sa_handle materials = SA_NULL;
    sa_handle unused = SA_NULL;

    wrong.corners[2] = 5;
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &wrong, sizeof wrong,
               &unused) == SA_ERR_INVALID);
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &triangle, 8, &unused) ==
          SA_ERR_INVALID);
    CHECK(make(engine, SA_KIND_TEXTURE, SA_FORMAT_MESH, &triangle,
               sizeof triangle, &unused) == SA_ERR_INVALID);
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &triangle, sizeof triangle,
               &mesh) == SA_OK);
    CHECK(make(engine, SA_KIND_TEXTURE, SA_FORMAT_PIXELS, &pixel, sizeof pixel,
               &texture) == SA_OK);
    sa_material material{};

    material.texture = mesh; /* another kind's handle */
    material.flags = SA_MAT_TEXTURE;
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &material,
               sizeof material, &unused) == SA_ERR_HANDLE);
    material.texture = texture;
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &material,
               sizeof material, &materials) == SA_OK);

    /* a picture of two draws. a draw needs its shade and palettes first.
     * draws given together go in together or not at all */
    static sa_palettes palettes;
    sa_shade shade{};
    sa_draw draws[2] = {};
    sa_picture picture = picture_at(1);

    palettes.head = {SA_REC_PALETTES, sizeof palettes};
    shade.head = {SA_REC_SHADE, sizeof shade};
    shade.shade = 7;
    draws[0].head = {SA_REC_DRAW, sizeof draws[0]};
    draws[0].mesh = mesh;
    draws[0].materials = materials;
    draws[0].payload = SA_NO_PAYLOAD;
    draws[0].shade = 7;
    draws[1] = draws[0];
    CHECK(sa_present_draw(engine, draws, 1) == SA_ERR_INVALID); /* no picture */
    CHECK(sa_present_begin(engine, &picture) == SA_OK);
    CHECK(sa_present_begin(engine, &picture) == SA_ERR_INVALID);
    CHECK(sa_present_draw(engine, draws, 1) == SA_ERR_HANDLE); /* no shade */
    CHECK(sa_present_shade(engine, &shade) == SA_OK);
    CHECK(sa_present_draw(engine, draws, 1) ==
          SA_ERR_INVALID); /* no palettes */
    CHECK(sa_present_palettes(engine, &palettes) == SA_OK);
    draws[1].mesh = texture;
    CHECK(sa_present_draw(engine, draws, 2) == SA_ERR_HANDLE);
    draws[1].mesh = mesh;
    draws[1].object_to_world[7] = nanf("");
    CHECK(sa_present_draw(engine, draws, 2) == SA_ERR_INVALID);
    draws[1].object_to_world[7] = 0.0F;
    CHECK(sa_present_draw(engine, draws, 2) == SA_OK);
    /* a draw the game's own test left out of view counts, and adds nothing */
    draws[1].flags = SA_DRAW_OUT;
    CHECK(sa_present_draw(engine, &draws[1], 1) == SA_OK);
    draws[1].flags = 0;
    /* a draw under a shade of lights needs its lights first. a type no
     * backend lights is refused, and so is a NaN */
    static sa_lights lights;
    sa_shade lit = shade;

    lights.head = {SA_REC_LIGHTS, sizeof lights};
    lights.shade = 8;
    lights.count = 1;
    lights.light[0].type = SA_LIGHT_AMBIENT | SA_LIGHT_DIRECTION;
    lit.shade = 8;
    lit.mode = SA_SHADE_LIGHTS;
    CHECK(sa_present_shade(engine, &lit) == SA_OK);
    draws[1].shade = 8;
    draws[1].flags = SA_DRAW_OUT;
    CHECK(sa_present_draw(engine, &draws[1], 1) == SA_ERR_HANDLE);
    lights.light[0].type = 0x08; /* a spot light */
    CHECK(sa_present_lights(engine, &lights) == SA_ERR_UNSUPPORTED);
    /* a type is no float. one whose bits read as a NaN is a type no backend
     * lights, like any other */
    lights.light[0].type = 0x7fc00000;
    CHECK(sa_present_lights(engine, &lights) == SA_ERR_UNSUPPORTED);
    lights.light[0].type = SA_LIGHT_AMBIENT | SA_LIGHT_DIRECTION;
    lights.count = SA_LIGHTS + 1;
    CHECK(sa_present_lights(engine, &lights) == SA_ERR_INVALID);
    lights.count = 1;
    lights.light[0].diffuse[1] = nanf("");
    CHECK(sa_present_lights(engine, &lights) == SA_ERR_INVALID);
    lights.light[0].diffuse[1] = 0.0F;
    /* a light's floats are read from its first to its last. nothing of a
     * light past the count is read */
    lights.light[0].point[0] = nanf("");
    CHECK(sa_present_lights(engine, &lights) == SA_ERR_INVALID);
    lights.light[0].point[0] = 0.0F;
    lights.light[0].far = nanf("");
    CHECK(sa_present_lights(engine, &lights) == SA_ERR_INVALID);
    lights.light[0].far = 0.0F;
    lights.light[1].type = 0x7fc00000;
    CHECK(sa_present_lights(engine, &lights) == SA_OK);
    lights.light[1].type = 0;
    CHECK(sa_present_draw(engine, &draws[1], 1) == SA_OK);
    draws[1].shade = 7;
    draws[1].flags = 0;

    /* a release in the middle of a picture ends the handle at once. what the
     * picture took stays */
    CHECK(sa_resource_release(engine, mesh) == SA_OK);
    CHECK(sa_present_draw(engine, draws, 1) == SA_ERR_HANDLE);
    CHECK(sa_resource_release(engine, mesh) == SA_ERR_HANDLE);
    CHECK(sa_resource_release(engine, texture) == SA_OK);
    CHECK(polled(engine) == 0);
    CHECK(sa_present_end(engine) == SA_OK);
    CHECK(fake.vertices == 6);
    /* nothing retires ahead of its fence */
    CHECK(polled(engine) == 0 && fake.freed.empty());
    sa_pc_stats stats;
    CHECK(sa_pc_stats_read(engine, &stats) == SA_OK && stats.draws == 4 &&
          stats.triangles == 2);
    /* the picture first, then what it held, in the order they were let go */
    CHECK(polled(engine) == 311 && fake.freed.size() == 1);

    /* materials outlive their texture, but no draw takes them */
    sa_handle again = SA_NULL;
    picture = picture_at(2);
    /* a view down z, its near plane at 1 */
    picture.view.world_to_view[0] = 1.0F;
    picture.view.world_to_view[5] = 1.0F;
    picture.view.world_to_view[10] = 1.0F;
    picture.view.focal[0] = 320.0F;
    picture.view.focal[1] = 320.0F;
    picture.view.near_z = 1.0F;
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &triangle, sizeof triangle,
               &again) == SA_OK);
    /* the last slot let go comes back, under a new generation */
    CHECK(slot_of(again) == slot_of(texture) &&
          generation_of(again) != generation_of(texture));
    draws[0].mesh = again;
    CHECK(sa_present_begin(engine, &picture) == SA_OK);
    CHECK(sa_present_shade(engine, &shade) == SA_OK);
    CHECK(sa_present_palettes(engine, &palettes) == SA_OK);
    CHECK(sa_present_draw(engine, draws, 1) == SA_ERR_HANDLE);

    /* a mesh a writer made lives in the picture's arena: per picture */
    Pixel white;
    sa_handle fresh = SA_NULL;
    uint32_t payload = 0;

    CHECK(make(engine, SA_KIND_TEXTURE, SA_FORMAT_PIXELS, &white, sizeof white,
               &fresh) == SA_OK);
    material.texture = fresh;
    sa_handle list = SA_NULL;
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &material,
               sizeof material, &list) == SA_OK);
    CHECK(sa_present_copy(engine, &triangle, sizeof triangle, &payload) ==
          SA_OK);
    draws[0].mesh = SA_NULL;
    draws[0].materials = list;
    draws[0].payload = payload;
    triangle.corners[0] = 9; /* a copy, so the source may change */
    CHECK(sa_present_draw(engine, draws, 1) == SA_OK);
    triangle.corners[0] = 0;
    draws[0].payload = 3;
    CHECK(sa_present_draw(engine, draws, 1) == SA_ERR_INVALID);
    CHECK(sa_present_copy(engine, block, sizeof block, &payload) ==
          SA_ERR_FULL);

    /* sprites on the screen, by their pri and at a point, and written
     * triangles. each refuses a dead texture and takes a live one */
    sa_sprite sprite{};
    sa_triangles strip{};
    sa_point points[4] = {};

    sprite.head = {SA_REC_SPRITE, sizeof sprite};
    sprite.texture = texture;
    sprite.flags = SA_SPRITE_SCREEN | SA_SPRITE_ALPHA;
    sprite.src = SA_BLEND_SRC_ALPHA;
    sprite.dst = SA_BLEND_INV_SRC_ALPHA;
    sprite.scale[0] = 1.0F;
    sprite.scale[1] = 1.0F;
    sprite.corner[1][0] = 8.0F;
    sprite.corner[1][1] = 8.0F;
    sprite.bias = -2.0F;
    CHECK(sa_present_sprite(engine, &sprite, 1) == SA_ERR_HANDLE);
    sprite.texture = fresh;
    sprite.at[0] = nanf("");
    CHECK(sa_present_sprite(engine, &sprite, 1) == SA_ERR_INVALID);
    sprite.at[0] = 0.0F;
    sprite.at[2] = nanf(""); /* a 2D sprite's z, which the game leaves unset */
    CHECK(sa_present_sprite(engine, &sprite, 1) == SA_OK);
    strip.head = {SA_REC_TRIANGLES, sizeof strip};
    strip.texture = texture;
    strip.count = 2;
    strip.form = SA_TRIANGLES_STRIP;
    strip.flags = SA_TRIANGLES_TEXTURE;
    CHECK(sa_present_triangles(engine, &strip, points) == SA_ERR_HANDLE);
    strip.texture = fresh;
    CHECK(sa_present_triangles(engine, &strip, points) == SA_OK);

    /* a panel on the screen, and a line ahead of the eye. the line goes to
     * the screen as a strip, and the two share one batch */
    sa_triangles panel = strip;
    sa_triangles line = strip;
    sa_point ends[2] = {{{0.0F, 0.0F, 10.0F}, {}, 0},
                        {{1.0F, 1.0F, 10.0F}, {}, 0}};

    panel.texture = SA_NULL;
    panel.flags = SA_TRIANGLES_SCREEN;
    CHECK(sa_present_triangles(engine, &panel, points) == SA_OK);
    line.texture = SA_NULL;
    line.flags = 0;
    line.form = SA_TRIANGLES_LINES;
    line.count = 1;
    line.object_to_world[0] = 1.0F;
    line.object_to_world[5] = 1.0F;
    line.object_to_world[10] = 1.0F;
    CHECK(sa_present_triangles(engine, &line, ends) == SA_OK);
    /* a form past the lines' gives its points no count, so none is read */
    Page unread(ends, sizeof ends);

    CHECK(unread.bytes != MAP_FAILED && unread.take_back());
    line.form = SA_TRIANGLES_LINES + 1;
    CHECK(sa_present_triangles(engine, &line,
                               static_cast<const sa_point *>(unread.bytes)) ==
          SA_ERR_INVALID);

    /* fog as the game gives it */
    sa_fog fog{};

    fog.head = {SA_REC_FOG, sizeof fog};
    fog.color = 0xff102030;
    fog.density = 2.5F;
    fog.table[1] = 0x8040;
    CHECK(sa_present_fog(engine, &fog) == SA_OK);
    CHECK(sa_present_end(engine) == SA_OK);
    /* the draw, the sprite, the strip, the panel and the line */
    CHECK(fake.vertices == 3 + 6 + 6 + 6 + 6);
    CHECK(fake.screen_batches == 2 && fake.blend_batches == 1);
    CHECK(fake.uniform.fog[0] == 2.5F && fake.uniform.table[2] == 128.0F &&
          fake.uniform.table[3] == 64.0F);

    /* a slot's arena is its picture's until the picture retires. so a third
     * picture waits for the first, and no longer */
    fake.waits = 0;
    picture = picture_at(3);
    CHECK(sa_present_begin(engine, &picture) == SA_OK);
    CHECK(fake.waits == 0); /* picture 1 retired above */
    CHECK(sa_present_end(engine) == SA_OK);
    picture = picture_at(4);
    CHECK(sa_present_begin(engine, &picture) == SA_OK);
    CHECK(fake.waits == 1);
    CHECK(sa_present_end(engine) == SA_OK);

    /* a retirement waits for every picture begun before its release. a
     * later picture's fence says nothing of an earlier one's */
    uint32_t freed = static_cast<uint32_t>(fake.freed.size());

    CHECK(polled(engine) == 3); /* picture 2's, seen at picture 4's begin */
    CHECK(sa_resource_release(engine, fresh) == SA_OK);
    fake.busy[1] = false; /* picture 4's. picture 3's, on 0, still runs */
    CHECK(sa_resource_release(engine, again) == SA_OK);
    CHECK(polled(engine) == 0 && fake.freed.size() == freed);
    fake.busy[0] = false;
    CHECK(sa_resource_release(engine, list) == SA_OK);
    /* pictures 3 and 4 showed, then the three releases retired */
    CHECK(polled(engine) == 33111 && fake.freed.size() == freed + 1);

    /* releases behind a fence fill the event queue, not memory. making one
     * more is refused until the fence lets them go */
    CHECK(sa_resource_release(engine, materials) == SA_OK);
    picture = picture_at(5);
    CHECK(sa_present_begin(engine, &picture) == SA_OK);
    CHECK(sa_present_end(engine) == SA_OK);
    uint32_t made = 0;
    sa_result result = SA_OK;

    while (made < 100 && result == SA_OK) {
        result = make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &triangle,
                      sizeof triangle, &again);
        if (result == SA_OK) {
            CHECK(sa_resource_release(engine, again) == SA_OK);
            made++;
        }
    }
    CHECK(result == SA_ERR_FULL && made > desc.resources);
    fake.busy[0] = false;
    fake.busy[1] = false;
    CHECK(sa_pc_stats_read(engine, &stats) == SA_OK);
    (void)polled(engine);
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &triangle, sizeof triangle,
               &again) == SA_OK);

    /* a background of three colors is a plane under the rest; of one, none */
    picture = picture_at(9);
    picture.back[1] = 0xff102030;
    CHECK(sa_present_begin(engine, &picture) == SA_OK);
    CHECK(sa_present_end(engine) == SA_OK);
    CHECK(fake.vertices == 6);
    picture.back[1] = 0;
    fake.busy[0] = false;
    fake.busy[1] = false;
    CHECK(sa_present_begin(engine, &picture) == SA_OK);
    CHECK(sa_present_end(engine) == SA_OK);
    CHECK(fake.vertices == 0);
    fake.busy[0] = false;
    fake.busy[1] = false;
    CHECK(sa_pc_stats_read(engine, &stats) == SA_OK);
    (void)polled(engine);

    /* a PVR file goes in by its PVRT, past its GBIX: here a twiddled 4 by
     * 4. a GBIX longer than the file is refused. 8 and its length wrap to 4,
     * where the bytes after read as a PVRT's header */
    const uint8_t header[12] = {40, 0, 0, 0, 2, 1, 0, 0, 4, 0, 4, 0};
    std::vector<uint8_t> pvr(16 + 16 + 32, 0);
    std::vector<uint8_t> turned(8 + 12 + 32, 0);
    sa_handle decoded = SA_NULL;

    memcpy(&pvr[0], "GBIX\x08", 5);
    memcpy(&pvr[16], "PVRT", 4);
    memcpy(&pvr[20], header, sizeof header);
    memcpy(&turned[0], "GBIX\xfc\xff\xff\xff", 8);
    memcpy(&turned[8], header, sizeof header);
    CHECK(make(engine, SA_KIND_TEXTURE, SA_FORMAT_PVR, turned.data(),
               static_cast<uint32_t>(turned.size()),
               &unused) == SA_ERR_INVALID);
    CHECK(make(engine, SA_KIND_TEXTURE, SA_FORMAT_PVR, pvr.data(),
               static_cast<uint32_t>(pvr.size()), &decoded) == SA_OK);
    CHECK(sa_resource_release(engine, decoded) == SA_OK);
    (void)polled(engine);

    CHECK(client_records(engine) == 0);

    /* the mode asked before a picture applies from its begin, so this one is
     * 16:9. the engine's end waits for it */
    picture = picture_at(6);
    asked.aspect = SA_ASPECT_16_9;
    CHECK(sa_present_window(engine, SA_ASPECT_16_9) == SA_OK &&
          sa_present_mode(engine, &asked, &applied) == SA_OK && !fake.wide);
    CHECK(sa_present_begin(engine, &picture) == SA_OK && fake.wide);
    CHECK(sa_present_end(engine) == SA_OK);
    fake.waits = 0;
    sa_engine_destroy(engine);
    CHECK(fake.waits == 1);
    return 0;
}

/* a writer's mesh made a resource at each pose, its draws naming one thing.
 * under SA_DRAW_WRITTEN the picture between moves its points. two meshes
 * drawn as two things are two instances, which the between snaps from the
 * older picture's place. one mesh drawn as two things is two instances too.
 * each is found again by the name its draw gave, whatever the order or the
 * other draws. where a draw names no thing, the scene names it itself. it
 * uses the draw's mesh and its place among the task's draws of that mesh.
 * those names stay apart from every thing a draw names */
int written()
{
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 16;
    desc.events = 32;
    desc.platform = &options;
    options.rate = STEP_RATE;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    Triangle poses[4];
    sa_handle meshes[4] = {SA_NULL, SA_NULL, SA_NULL, SA_NULL};
    sa_handle materials = SA_NULL;
    sa_material material{};
    static sa_palettes palettes;
    sa_shade shade{};
    sa_draw draw{};
    sa_pc_stats stats{};

    for (int i = 0; i < 4; i++) {
        poses[i].points[0] = static_cast<float>(i);
        CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &poses[i],
                   sizeof poses[i], &meshes[i]) == SA_OK);
    }
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &material,
               sizeof material, &materials) == SA_OK);
    shade.head = {SA_REC_SHADE, sizeof shade};
    draw.head = {SA_REC_DRAW, sizeof draw};
    draw.materials = materials;
    draw.payload = SA_NO_PAYLOAD;
    draw.instance = 5;
    draw.object_to_world[0] = draw.object_to_world[5] =
        draw.object_to_world[10] = 1;
    auto show = [&](uint32_t tick, sa_handle mesh, uint16_t flags,
                    uint32_t thing) {
        sa_picture picture = picture_at(tick);
        sa_draw d = draw;

        d.mesh = mesh;
        d.flags = flags;
        d.thing = thing;
        fake.frames.clear();
        fake.first_x.clear();
        for (bool &b : fake.busy) {
            b = false;
        }
        return sa_present_begin(engine, &picture) == SA_OK &&
               sa_present_shade(engine, &shade) == SA_OK &&
               sa_present_palettes(engine, &palettes) == SA_OK &&
               sa_present_draw(engine, &d, 1) == SA_OK &&
               sa_present_end(engine) == SA_OK &&
               sa_pc_stats_read(engine, &stats) == SA_OK;
    };
    CHECK(show(10, meshes[0], SA_DRAW_WRITTEN, 1));
    CHECK(show(12, meshes[1], SA_DRAW_WRITTEN, 1) && stats.moved == 1 &&
          stats.snapped == 0);
    /* its first point halfway between the two poses' */
    CHECK(fake.first_x.size() == 2 && fake.first_x[0] == 0.5F &&
          fake.first_x[1] == 1.0F);
    CHECK(show(14, meshes[2], 0, 2));
    CHECK(show(16, meshes[3], 0, 3) && stats.moved == 0 && stats.snapped == 1);
    struct Named {
        uint32_t thing;
        float x;
    };
    auto things = [&](uint32_t tick, const std::vector<Named> &named) {
        sa_picture picture = picture_at(tick);
        bool taken = sa_present_begin(engine, &picture) == SA_OK &&
                     sa_present_shade(engine, &shade) == SA_OK &&
                     sa_present_palettes(engine, &palettes) == SA_OK;

        fake.frames.clear();
        fake.first_x.clear();
        fake.xs.clear();
        for (bool &b : fake.busy) {
            b = false;
        }
        for (const Named &n : named) {
            sa_draw d = draw;

            d.mesh = meshes[0];
            d.instance = 6;
            d.thing = n.thing;
            d.object_to_world[3] = n.x;
            taken = taken && sa_present_draw(engine, &d, 1) == SA_OK;
        }
        return taken && sa_present_end(engine) == SA_OK &&
               sa_pc_stats_read(engine, &stats) == SA_OK;
    };
    CHECK(things(18, {{1, 0}, {2, 10}}));
    /* drawn the other way round, each moves from its own place. the first
     * drawn is thing 2, halfway from 10 to 12 */
    CHECK(things(20, {{2, 12}, {1, 2}}) && stats.moved == 2 &&
          fake.first_x.size() == 2 && fake.first_x[0] == 11);
    /* a third thing drawn ahead of them changes neither's name. the between
     * leaves it out to the half. so it shows thing 1 first, halfway from 2
     * to 4 */
    CHECK(things(22, {{3, 50}, {1, 4}, {2, 14}}) && stats.moved == 2 &&
          stats.snapped == 0 && fake.first_x[0] == 3);
    /* three draws of one mesh that name no thing are three instances. the
     * between shows each halfway from its own earlier place */
    CHECK(things(24, {{0, 0}, {0, 10}, {0, 20}}));
    CHECK(things(26, {{0, 2}, {0, 14}, {0, 26}}) && stats.moved == 3 &&
          stats.snapped == 0 && fake.xs.size() == 2 && fake.xs[0].size() == 9 &&
          fake.xs[0][0] == 1 && fake.xs[0][3] == 12 && fake.xs[0][6] == 23);
    /* a thing numbered as the scene names the unnamed draws, by their mesh's
     * handle, is none of them. it moves halfway from 0 to 2, though a second
     * unnamed draw comes. the unnamed draws, one in the older picture and two
     * in the newer, pair with nothing. so the older's shows where it was */
    CHECK(things(28, {{meshes[0], 0}, {0, 10}}));
    CHECK(things(30, {{meshes[0], 2}, {0, 12}, {0, 30}}) && stats.moved == 1 &&
          stats.snapped == 1 && fake.xs[0].size() == 6 && fake.xs[0][0] == 1 &&
          fake.xs[0][3] == 10);
    for (bool &b : fake.busy) {
        b = false;
    }
    sa_engine_destroy(engine);
    return 0;
}

/* SA1's shade. a point takes its palette entry's color, and a point of an
 * unlit blended material takes white. the Dreamcast's walk writes that white
 * into every point of the model. so the adapter marks such a material
 * SA_MAT_WHITE. each meshset after its first in the draw takes white too,
 * whatever its material. so a material named on both sides of it keeps the
 * entry's color before and takes white after. an opaque meshset keeps the
 * entry's alpha, and a blended one takes its material's. an unlit blended
 * material without the flag whitens only its own meshsets. the Ninja forms'
 * constant color is every meshset's. Ninja's own shades read no white */
int whitened()
{
    const char *const no_roots[] = {nullptr};
    /* the materials each model's meshsets name, in the walk's order. model 0
     * is the adapter's test's, whose material 1 is unlit and blended. model 1
     * names materials 0 and 3, the blended one, on both sides of material 1 */
    const std::vector<uint16_t> orders[2] = {{0, 1, 2, 0},
                                             {3, 0, 1, 3, 0, 1, 2, 0}};
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;
    sa_cap_mesh mesh{};
    sa_cap_material materials[3][4] = {};
    sa_cap_materials list{};
    sa_shade shade{};
    sa_lights lights{};
    static sa_cap_palettes palettes;
    sa_cap_picture picture{};
    sa_draw draw{};
    sa_cap_end end{};

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 16;
    desc.events = 32;
    desc.platform = &options;
    options.rate = STEP_RATE;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    sa_pc_client *client = sa_pc_client_create(engine, no_roots);
    auto give = [&](const std::vector<uint8_t> &r, sa_result result) {
        return sa_pc_client_record(client, r.data()) == result;
    };
    /* show model order under the list and shade given. colors gets each
     * meshset's first point's color, in the model's order. the raster takes
     * the opaque meshsets' points first, then the blended ones', each in the
     * model's order */
    auto show = [&](uint32_t order, uint32_t materials_id, uint32_t shade_id,
                    std::vector<uint32_t> &colors) {
        const std::vector<uint16_t> &named = orders[order];
        /* the place among the raster's triangles of the next opaque
         * meshset, and of the next blended one */
        size_t opaque = 0;
        size_t blended = 0;

        picture.picture++;
        picture.tick += 2;
        end.picture = picture.picture;
        draw.mesh = order;
        draw.materials = materials_id;
        draw.shade = shade_id;
        for (bool &b : fake.busy) {
            b = false;
        }
        if (!give(record(SA_CAP_PICTURE, picture), SA_OK) ||
            !give(record(SA_REC_DRAW, draw), SA_OK) ||
            !give(record(SA_CAP_END, end), SA_PC_ENDED) ||
            fake.last.size() != 3 * named.size()) {
            return false;
        }
        for (uint16_t m : named) {
            blended += materials[materials_id][m].flags & SA_MAT_BLEND ? 0 : 1;
        }
        colors.clear();
        for (uint16_t m : named) {
            size_t &at = materials[materials_id][m].flags & SA_MAT_BLEND
                             ? blended
                             : opaque;

            colors.push_back(fake.last[3 * at++].base);
        }
        return true;
    };

    /* list 0 is the walk's, as the adapter records it. its second material
     * is unlit and blended, and so SA_MAT_WHITE. its fourth is blended. list
     * 1 is the same without the flag. in list 2, the Ninja forms', every
     * material is unlit, blended and of the constant color */
    for (int i = 0; i < 3; i++) {
        materials[i][0].diffuse = 0xff203040;
        materials[i][1].diffuse = 0xff506070;
        materials[i][2].diffuse = 0xff8090a0;
        materials[i][3].diffuse = 0xffb0c0d0;
        materials[i][1].flags = SA_MAT_BLEND | SA_MAT_UNLIT;
        materials[i][1].alpha = 0x40;
        materials[i][3].flags = SA_MAT_BLEND;
        materials[i][3].alpha = 0x20;
    }
    materials[0][1].flags |= SA_MAT_WHITE;
    for (sa_cap_material &m : materials[2]) {
        m.flags = SA_MAT_BLEND | SA_MAT_UNLIT | SA_MAT_CONSTANT;
        m.alpha = 0x40;
    }
    mesh.format = SA_MESH_NINJA_INLINE;
    for (uint32_t i = 0; i < 2; i++) {
        mesh.mesh = i;
        CHECK(give(record(SA_CAP_MESH, mesh, inline_sets(orders[i])), SA_OK));
    }
    list.count = 4;
    for (uint32_t i = 0; i < 3; i++) {
        std::vector<uint8_t> ids(4 * sizeof(uint32_t));

        for (uint32_t k = 0; k < 4; k++) {
            uint32_t id = 4 * i + k;

            materials[i][k].material = id;
            materials[i][k].texture = SA_CAP_NONE;
            memcpy(&ids[k * sizeof id], &id, sizeof id);
            CHECK(give(record(SA_CAP_MATERIAL, materials[i][k]), SA_OK));
        }
        list.materials = i;
        CHECK(give(record(SA_CAP_MATERIALS, list, ids), SA_OK));
    }
    /* shade 0 is SA1's, and the others Ninja's. they are its ramp, its
     * simple drawer's and its full drawer's under one ambient light */
    lights.shade = SA_SHADE_LIGHTS;
    lights.count = 1;
    lights.light[0].type = SA_LIGHT_AMBIENT;
    lights.light[0].ambient[0] = lights.light[0].ambient[1] =
        lights.light[0].ambient[2] = 1;
    CHECK(give(record(SA_REC_LIGHTS, lights), SA_OK));
    shade.ambient = 1;
    for (uint8_t mode = SA_SHADE_PALETTE; mode <= SA_SHADE_LIGHTS; mode++) {
        shade.shade = mode;
        shade.mode = mode;
        CHECK(give(record(SA_REC_SHADE, shade), SA_OK));
    }
    for (uint32_t i = 0; i < SA_PALETTE_ENTRIES; i++) {
        palettes.words[i * 2] = 0x80102030;
    }
    CHECK(give(record(SA_CAP_PALETTES, palettes), SA_OK));
    for (int i = 0; i < 12; i += 5) {
        picture.view.world_to_view[i] = 1;
        draw.object_to_world[i] = 1;
    }
    draw.payload = SA_NO_PAYLOAD;
    draw.tint = 0x00112233;
    std::vector<uint32_t> colors;
    std::vector<uint32_t> plain;
    /* the entry's color as the raster takes it, red lowest. from the unlit
     * blended meshset on, each takes white, the first material's too */
    const std::vector<uint32_t> fixture = {0x80302010, 0x40ffffff, 0x80ffffff,
                                           0x80ffffff};
    const std::vector<uint32_t> both = {0x20302010, 0x80302010, 0x40ffffff,
                                        0x20ffffff, 0x80ffffff, 0x40ffffff,
                                        0x80ffffff, 0x80ffffff};
    const std::vector<uint32_t> unmarked = {0x20302010, 0x80302010, 0x40ffffff,
                                            0x20302010, 0x80302010, 0x40ffffff,
                                            0x80302010, 0x80302010};

    CHECK(show(0, 0, SA_SHADE_PALETTE, colors) && colors == fixture);
    CHECK(show(1, 0, SA_SHADE_PALETTE, colors) && colors == both);
    CHECK(show(1, 1, SA_SHADE_PALETTE, colors) && colors == unmarked);
    /* the constant color with the material's alpha, at every meshset */
    CHECK(show(1, 2, SA_SHADE_PALETTE, colors) &&
          colors == std::vector<uint32_t>(8, 0x40332211));
    /* Ninja's shades give each meshset its material's color, lit or not,
     * with the flag as without it */
    for (uint8_t mode : {SA_SHADE_RAMP, SA_SHADE_LINEAR, SA_SHADE_LIGHTS}) {
        CHECK(show(1, 0, mode, colors) && show(1, 1, mode, plain) &&
              colors == plain);
        CHECK((colors[1] & 0xffffff) == (as_raster(0xff203040) & 0xffffff) &&
              colors[1] == colors[4] && colors[4] == colors[7]);
        CHECK(colors[0] == colors[3] && colors[2] == colors[5] &&
              colors[6] != colors[7]);
    }
    for (bool &b : fake.busy) {
        b = false;
    }
    sa_pc_client_destroy(client);
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* a batch samples as its material says. the filter and the mipmap's D
 * adjust reach the raster in the batch's sample. two meshsets that differ in
 * either alone are two batches, and two that agree are one. the sample also
 * holds how u and v run past the edge. a sprite and written triangles give
 * their filter and a D adjust of 1.0. their records hold no adjust */
int sampled()
{
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 32;
    desc.events = 64;
    desc.platform = &options;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    TwoSets sets;
    Pixel pixel;
    sa_handle mesh = SA_NULL;
    sa_handle texture = SA_NULL;
    static sa_palettes palettes;
    sa_shade shade{};
    sa_draw draw{};
    sa_sprite sprite{};
    sa_triangles strip{};
    sa_point points[3] = {};
    sa_pc_stats stats{};
    uint32_t tick = 10;

    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &sets, sizeof sets,
               &mesh) == SA_OK);
    CHECK(make(engine, SA_KIND_TEXTURE, SA_FORMAT_PIXELS, &pixel, sizeof pixel,
               &texture) == SA_OK);
    palettes.head = {SA_REC_PALETTES, sizeof palettes};
    shade.head = {SA_REC_SHADE, sizeof shade};
    draw.head = {SA_REC_DRAW, sizeof draw};
    draw.mesh = mesh;
    draw.payload = SA_NO_PAYLOAD;
    draw.instance = 5;
    draw.object_to_world[0] = draw.object_to_world[5] =
        draw.object_to_world[10] = 1;
    /* a picture of the mesh. its two meshsets' materials differ only in
     * each one's filter and D adjust, and the second's flags */
    auto show = [&](uint8_t first_filter, uint8_t first_adjust,
                    uint8_t second_filter, uint8_t second_adjust,
                    uint16_t second_flags = 0) {
        sa_picture picture = picture_at(tick);
        sa_material material[2] = {};
        sa_handle materials = SA_NULL;

        tick += 2;
        for (sa_material &m : material) {
            m.texture = texture;
            m.flags = SA_MAT_TEXTURE;
        }
        material[1].flags |= second_flags;
        material[0].filter = first_filter;
        material[0].mip_bias = first_adjust;
        material[1].filter = second_filter;
        material[1].mip_bias = second_adjust;
        for (bool &b : fake.busy) {
            b = false;
        }
        if (make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, material,
                 sizeof material, &materials) != SA_OK) {
            return false;
        }
        draw.materials = materials;
        return sa_present_begin(engine, &picture) == SA_OK &&
               sa_present_shade(engine, &shade) == SA_OK &&
               sa_present_palettes(engine, &palettes) == SA_OK &&
               sa_present_draw(engine, &draw, 1) == SA_OK &&
               sa_present_end(engine) == SA_OK &&
               sa_pc_stats_read(engine, &stats) == SA_OK;
    };

    CHECK(show(SA_FILTER_BILINEAR, 4, SA_FILTER_BILINEAR, 4) &&
          stats.batches == 1 &&
          fake.samples[0] == pc::sample_of(SA_FILTER_BILINEAR, 4));
    CHECK(show(SA_FILTER_POINT, 4, SA_FILTER_BILINEAR, 4) &&
          stats.batches == 2 &&
          fake.samples[0] == pc::sample_of(SA_FILTER_POINT, 4) &&
          fake.samples[1] == pc::sample_of(SA_FILTER_BILINEAR, 4));
    CHECK(show(SA_FILTER_BILINEAR, 4, SA_FILTER_TRILINEAR, 4) &&
          stats.batches == 2 &&
          fake.samples[0] == pc::sample_of(SA_FILTER_BILINEAR, 4) &&
          fake.samples[1] == pc::sample_of(SA_FILTER_TRILINEAR, 4));
    CHECK(show(SA_FILTER_BILINEAR, 4, SA_FILTER_BILINEAR, 3) &&
          stats.batches == 2 &&
          fake.samples[0] == pc::sample_of(SA_FILTER_BILINEAR, 4) &&
          fake.samples[1] == pc::sample_of(SA_FILTER_BILINEAR, 3));
    /* an adjust of 0, the most one of 3.75, and the edges, beside them */
    CHECK(show(SA_FILTER_POINT, 0, SA_FILTER_TRILINEAR, 15,
               SA_MAT_CLAMP_U | SA_MAT_FLIP_V) &&
          stats.batches == 2 &&
          fake.samples[0] == pc::sample_of(SA_FILTER_POINT, 0) &&
          fake.samples[1] == (pc::sample_of(SA_FILTER_TRILINEAR, 15) |
                              pc::SAMPLE_CLAMP_U | pc::SAMPLE_FLIP_V));
    sa_picture picture = picture_at(tick);

    sprite.head = {SA_REC_SPRITE, sizeof sprite};
    sprite.texture = texture;
    sprite.instance = 9;
    sprite.object_to_world[0] = sprite.object_to_world[5] =
        sprite.object_to_world[10] = 1;
    sprite.at[2] = 1.0F;
    sprite.corner[1][0] = sprite.corner[1][1] = 8.0F;
    sprite.scale[0] = sprite.scale[1] = 1.0F;
    sprite.filter = SA_FILTER_TRILINEAR;
    strip.head = {SA_REC_TRIANGLES, sizeof strip};
    strip.texture = texture;
    strip.instance = 10;
    strip.count = 1;
    strip.form = SA_TRIANGLES_STRIP;
    strip.flags = SA_TRIANGLES_TEXTURE;
    strip.object_to_world[0] = strip.object_to_world[5] =
        strip.object_to_world[10] = 1;
    strip.filter = SA_FILTER_POINT;
    for (bool &b : fake.busy) {
        b = false;
    }
    CHECK(sa_present_begin(engine, &picture) == SA_OK &&
          sa_present_sprite(engine, &sprite, 1) == SA_OK &&
          sa_present_triangles(engine, &strip, points) == SA_OK &&
          sa_present_end(engine) == SA_OK && fake.samples.size() == 2);
    CHECK(fake.samples[0] ==
              pc::sample_of(SA_FILTER_TRILINEAR, pc::ADJUST_ONE) &&
          fake.samples[1] == pc::sample_of(SA_FILTER_POINT, pc::ADJUST_ONE));
    for (bool &b : fake.busy) {
        b = false;
    }
    sa_engine_destroy(engine);
    return 0;
}

/* a picture's figures keep apart two counts of the blend fragments it shows
 * without. fragments_lost counts those its lists had no room for.
 * fragments_cut counts those a pixel held past the ones it composes */
int left_out()
{
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;
    sa_picture picture = picture_at(10);
    sa_pc_stats stats{};

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 8;
    desc.events = 16;
    desc.platform = &options;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    for (bool &b : fake.busy) {
        b = false;
    }
    CHECK(sa_present_begin(engine, &picture) == SA_OK &&
          sa_present_end(engine) == SA_OK);
    fake.lost = 3;
    fake.cut = 5;
    CHECK(sa_pc_stats_read(engine, &stats) == SA_OK &&
          stats.fragments_lost == 3 && stats.fragments_cut == 5);
    fake.lost = 0;
    fake.cut = 0;
    sa_engine_destroy(engine);
    return 0;
}

/* a draw out of the game's view adds no triangle, so it takes none of a
 * picture's room. draws of more triangles than a picture holds are refused
 * whole in view. they go in where all but one are out of it */
int out_of_view_room()
{
    constexpr uint32_t TRIANGLES = 4096; /* a mesh's */
    constexpr uint32_t DRAWS = 129; /* more than a picture's triangles hold */
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;
    sa_picture picture = picture_at(10);
    sa_pc_stats stats{};
    /* a mesh of three points, and one meshset of the same triangle over */
    const sa_mesh head = {3, 0, 1, 1};
    const float points[9] = {0, 0, 1, 1, 0, 1, 0, 1, 1};
    const sa_meshset set = {0, 0, TRIANGLES * 3};
    std::vector<uint16_t> corners(TRIANGLES * 3);
    std::vector<uint8_t> large(sizeof head + sizeof points + sizeof set +
                               (corners.size() * sizeof corners[0]));
    sa_handle mesh = SA_NULL;
    sa_handle materials = SA_NULL;
    sa_material material{};
    static sa_palettes palettes;
    sa_shade shade{};
    std::vector<sa_draw> draws(DRAWS);

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 8;
    desc.events = 16;
    desc.platform = &options;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    for (bool &b : fake.busy) {
        b = false;
    }
    for (size_t i = 0; i < corners.size(); i++) {
        corners[i] = static_cast<uint16_t>(i % 3);
    }
    memcpy(&large[0], &head, sizeof head);
    memcpy(&large[sizeof head], points, sizeof points);
    memcpy(&large[sizeof head + sizeof points], &set, sizeof set);
    memcpy(&large[sizeof head + sizeof points + sizeof set], corners.data(),
           corners.size() * sizeof corners[0]);
    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, large.data(),
               static_cast<uint32_t>(large.size()), &mesh) == SA_OK);
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &material,
               sizeof material, &materials) == SA_OK);
    palettes.head = {SA_REC_PALETTES, sizeof palettes};
    shade.head = {SA_REC_SHADE, sizeof shade};
    shade.shade = 7;
    for (sa_draw &d : draws) {
        d.head = {SA_REC_DRAW, sizeof d};
        d.mesh = mesh;
        d.materials = materials;
        d.payload = SA_NO_PAYLOAD;
        d.shade = 7;
    }
    CHECK(sa_present_begin(engine, &picture) == SA_OK &&
          sa_present_shade(engine, &shade) == SA_OK &&
          sa_present_palettes(engine, &palettes) == SA_OK);
    CHECK(sa_present_draw(engine, draws.data(), DRAWS) == SA_ERR_FULL);
    for (uint32_t i = 1; i < DRAWS; i++) {
        draws[i].flags = SA_DRAW_OUT;
    }
    CHECK(sa_present_draw(engine, draws.data(), DRAWS) == SA_OK);
    CHECK(sa_present_end(engine) == SA_OK);
    CHECK(sa_pc_stats_read(engine, &stats) == SA_OK && stats.draws == DRAWS &&
          stats.triangles == TRIANGLES);
    sa_engine_destroy(engine);
    return 0;
}

/* a picture the device refuses stays the engine's open one, as its end
 * found it. no other begins, and sa_pc_stats_read is refused. its end made
 * again shows it, with its own draws and the pictures between it was owed.
 * that holds whichever of them the device refused. a device that draws no
 * more says why, and then gives no picture's figures */
int refused()
{
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 16;
    desc.events = 32;
    desc.platform = &options;
    /* just under three pictures to two of the game's steps. a picture a step
     * after the last shows two or one, by what the ones before left owed */
    options.rate = 89910;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    Triangle triangle;
    sa_handle mesh = SA_NULL;
    sa_handle materials = SA_NULL;
    sa_material material{};
    static sa_palettes palettes;
    sa_shade shade{};
    sa_draw draw{};
    sa_pc_stats stats{};
    char why[64] = "";

    CHECK(make(engine, SA_KIND_MESH, SA_FORMAT_MESH, &triangle, sizeof triangle,
               &mesh) == SA_OK);
    CHECK(make(engine, SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, &material,
               sizeof material, &materials) == SA_OK);
    palettes.head = {SA_REC_PALETTES, sizeof palettes};
    shade.head = {SA_REC_SHADE, sizeof shade};
    draw.head = {SA_REC_DRAW, sizeof draw};
    draw.mesh = mesh;
    draw.materials = materials;
    draw.payload = SA_NO_PAYLOAD;
    draw.instance = 5;
    draw.object_to_world[0] = draw.object_to_world[5] =
        draw.object_to_world[10] = 1;
    /* a picture of the one triangle, given and not yet ended */
    auto give = [&](uint32_t tick) {
        sa_picture picture = picture_at(tick);

        for (bool &b : fake.busy) {
            b = false;
        }
        fake.frames.clear();
        return sa_present_begin(engine, &picture) == SA_OK &&
               sa_present_shade(engine, &shade) == SA_OK &&
               sa_present_palettes(engine, &palettes) == SA_OK &&
               sa_present_draw(engine, &draw, 1) == SA_OK;
    };
    /* it ends, and shows as many pictures, the last its own triangle */
    auto ends = [&](uint32_t shown) {
        return sa_present_end(engine) == SA_OK &&
               sa_pc_stats_read(engine, &stats) == SA_OK &&
               stats.shown == shown && fake.frames.size() == shown &&
               stats.draws == 1 && fake.vertices == 3;
    };
    sa_picture other = picture_at(99);

    CHECK(sa_pc_broken(engine, why, sizeof why) == SA_EMPTY);
    CHECK(sa_pc_broken(engine, nullptr, sizeof why) == SA_ERR_INVALID);
    CHECK(give(10) && ends(2));
    /* refused at the picture between that goes before its own */
    CHECK(give(11));
    fake.refuses = 1;
    CHECK(sa_present_end(engine) == SA_ERR_UNSUPPORTED);
    CHECK(sa_present_begin(engine, &other) == SA_ERR_INVALID &&
          sa_pc_stats_read(engine, &stats) == SA_ERR_INVALID &&
          sa_pc_broken(engine, why, sizeof why) == SA_EMPTY);
    CHECK(ends(2));
    /* at its own, the one picture it shows */
    CHECK(give(12));
    fake.refuses = 1;
    CHECK(sa_present_end(engine) == SA_ERR_UNSUPPORTED);
    CHECK(ends(1));
    /* and at its own after the picture between */
    CHECK(give(13));
    fake.takes = 1;
    fake.refuses = 1;
    CHECK(sa_present_end(engine) == SA_ERR_UNSUPPORTED && fake.refuses == 0);
    fake.frames.clear();
    CHECK(ends(2));
    CHECK(give(14) && ends(1));
    /* the device draws no more */
    fake.broken = "the test's device is lost";
    CHECK(sa_pc_stats_read(engine, &stats) == SA_ERR_UNSUPPORTED);
    CHECK(give(15) && sa_present_end(engine) == SA_ERR_UNSUPPORTED);
    CHECK(sa_pc_broken(engine, why, sizeof why) == SA_OK &&
          strcmp(why, fake.broken) == 0);
    CHECK(sa_pc_broken(engine, why, 5) == SA_OK && strcmp(why, "the ") == 0);
    /* the engine's end shows none of the picture left open, though the
     * device would now take it */
    fake.broken = nullptr;
    fake.frames.clear();
    for (bool &b : fake.busy) {
        b = false;
    }
    sa_engine_destroy(engine);
    CHECK(fake.frames.empty());
    fake = Fake{};
    return 0;
}

/* an engine of two pictures under options, made in block, or null */
sa_engine *engine_under(const sa_pc_options &options)
{
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 8;
    desc.events = 16;
    desc.platform = &options;
    if (sa_engine_memory(&desc, &bytes) != SA_OK || bytes > sizeof block ||
        sa_engine_create(&desc, block, bytes, &engine) != SA_OK) {
        return nullptr;
    }
    return engine;
}

/* the overview draws in a room of its own. each of the last four pictures
 * between keeps the room sa_pc_read_shown reads it from. so does the
 * picture itself */
int overview_room()
{
    sa_pc_options options{};
    sa_picture picture = picture_at(10);
    sa_picture above = picture_at(0);
    std::vector<uint8_t> rgba(SA_PC_WIDTH * SA_PC_HEIGHT * 4);
    auto rgba_bytes = static_cast<uint32_t>(rgba.size());
    sa_pc_stats stats{};
    uint32_t drawn = 0;

    /* three of the display's pictures a step: the first picture's two steps
     * show five between and then itself */
    options.rate = STEP_RATE * 3;
    sa_engine *engine = engine_under(options);

    CHECK(engine != nullptr);
    CHECK(sa_present_begin(engine, &picture) == SA_OK &&
          sa_present_end(engine) == SA_OK &&
          sa_pc_stats_read(engine, &stats) == SA_OK && stats.shown == 6 &&
          fake.frames.size() == 6);
    /* an overview, then a second, each in a room that is none of theirs.
     * the fake raster reads no pixels back */
    for (uint32_t i = 0; i < 2; i++) {
        CHECK(sa_pc_read_overview(engine, &above, rgba.data(), rgba_bytes,
                                  &drawn) == SA_ERR_UNSUPPORTED &&
              fake.frames.size() == 7 + i);
        CHECK(std::find(fake.frames.begin() + 1, fake.frames.begin() + 6,
                        fake.frames.back()) == fake.frames.begin() + 6);
    }
    for (bool &b : fake.busy) {
        b = false;
    }
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* a rate may owe one of the game's pictures more of the display's than it
 * shows. the rest goes unshown, and the picture after shows its own steps'
 * share */
int shown_bound()
{
    sa_pc_options options{};
    sa_pc_stats stats{};

    /* 20 of the display's pictures a step. a picture two steps after the
     * last is owed 40, and shows 32 */
    options.rate = STEP_RATE * 20;
    sa_engine *engine = engine_under(options);
    /* a picture at tick, and how many the display shows for it, or 0 */
    auto shows = [&](uint32_t tick) -> uint32_t {
        sa_picture picture = picture_at(tick);

        for (bool &b : fake.busy) {
            b = false;
        }
        fake.frames.clear();
        return sa_present_begin(engine, &picture) == SA_OK &&
                       sa_present_end(engine) == SA_OK &&
                       sa_pc_stats_read(engine, &stats) == SA_OK &&
                       fake.frames.size() == stats.shown
                   ? stats.shown
                   : 0;
    };

    CHECK(engine != nullptr);
    CHECK(shows(10) == 32 && shows(12) == 32 && shows(14) == 32);
    /* a step after: its own 20, and none of the 24 those three left */
    CHECK(shows(15) == 20);
    for (bool &b : fake.busy) {
        b = false;
    }
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* in a window, a picture's end shows the first of its pictures. it leaves
 * the rest to their times. a read of its figures shows them first. where
 * the device draws no more by then, the read says so. so does a read of the
 * picture */
int window_broken()
{
    sa_pc_options options{};
    sa_picture picture = picture_at(10);
    std::vector<uint8_t> rgba(SA_PC_WIDTH * SA_PC_HEIGHT * 4);
    auto rgba_bytes = static_cast<uint32_t>(rgba.size());
    sa_pc_stats stats{};
    char why[64] = "";

    /* two of the display's pictures a step: four for a picture's two */
    fake.window = true;
    options.flags = SA_PC_WINDOW;
    options.rate = STEP_RATE * 2;
    sa_engine *engine = engine_under(options);

    CHECK(engine != nullptr);
    CHECK(sa_present_begin(engine, &picture) == SA_OK &&
          sa_present_end(engine) == SA_OK && fake.frames.size() == 1);
    CHECK(sa_pc_stats_read(engine, &stats) == SA_OK && stats.shown == 4 &&
          fake.frames.size() == 4);
    for (bool &b : fake.busy) {
        b = false;
    }
    picture = picture_at(12);
    CHECK(sa_present_begin(engine, &picture) == SA_OK &&
          sa_present_end(engine) == SA_OK && fake.frames.size() == 5);
    fake.broken = "the test's device is lost";
    CHECK(sa_pc_stats_read(engine, &stats) == SA_ERR_UNSUPPORTED);
    CHECK(sa_pc_read(engine, rgba.data(), rgba_bytes) == SA_ERR_UNSUPPORTED);
    CHECK(sa_pc_broken(engine, why, sizeof why) == SA_OK &&
          strcmp(why, fake.broken) == 0 && fake.frames.size() == 5);
    for (bool &b : fake.busy) {
        b = false;
    }
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* a client whose picture the device refuses ends it again at once. where
 * the device refuses that too, it ends it before the next picture. no
 * picture's draws join another's. while the device refuses the open one,
 * the pictures after it go to none */
int client_refused()
{
    const char *const no_roots[] = {nullptr};
    sa_pc_options options{};
    sa_engine_desc desc{};
    sa_engine *engine = nullptr;
    uint32_t bytes = 0;
    sa_cap_mesh mesh{};
    sa_cap_material material{};
    sa_cap_materials list{};
    uint32_t material_id = 0;
    std::vector<uint8_t> ids(sizeof material_id, 0);
    sa_shade shade{};
    static sa_cap_palettes palettes;
    sa_cap_picture picture{};
    sa_draw draw{};
    sa_cap_end end{};

    desc.abi = SA_ENGINE_ABI_VERSION;
    desc.size = sizeof desc;
    desc.game = SA_GAME_SA1;
    desc.arena = 65536;
    desc.pictures = 2;
    desc.resources = 8;
    desc.events = 16;
    desc.platform = &options;
    CHECK(sa_engine_memory(&desc, &bytes) == SA_OK && bytes <= sizeof block);
    CHECK(sa_engine_create(&desc, block, bytes, &engine) == SA_OK);
    sa_pc_client *client = sa_pc_client_create(engine, no_roots);
    /* give a picture of draws draws of the one triangle, through its end.
     * begun and ended are what its record and its end's should give */
    auto give = [&](uint32_t draws, sa_result begun, sa_result ended) {
        for (bool &b : fake.busy) {
            b = false;
        }
        picture.picture++;
        picture.tick += 2;
        end.picture = picture.picture;
        if (sa_pc_client_record(
                client, record(SA_CAP_PICTURE, picture).data()) != begun) {
            return false;
        }
        for (uint32_t i = 0; i < draws; i++) {
            if (sa_pc_client_record(client, record(SA_REC_DRAW, draw).data()) !=
                SA_OK) {
                return false;
            }
        }
        return sa_pc_client_record(client, record(SA_CAP_END, end).data()) ==
               ended;
    };

    mesh.format = SA_MESH_NINJA_INLINE;
    material.texture = SA_CAP_NONE;
    list.count = 1;
    for (int i = 0; i < 12; i += 5) {
        picture.view.world_to_view[i] = 1;
    }
    draw.payload = SA_NO_PAYLOAD;
    for (const std::vector<uint8_t> &r :
         {record(SA_CAP_MESH, mesh, inline_model()),
          record(SA_CAP_MATERIAL, material),
          record(SA_CAP_MATERIALS, list, ids), record(SA_REC_SHADE, shade),
          record(SA_CAP_PALETTES, palettes)}) {
        CHECK(sa_pc_client_record(client, r.data()) == SA_OK);
    }
    /* refused once: the picture shows, once */
    fake.refuses = 1;
    CHECK(give(1, SA_OK, SA_PC_ENDED) && fake.frames.size() == 1 &&
          fake.vertices == 3);
    /* refused twice: it stays, and ends alone before the next begins */
    fake.refuses = 2;
    CHECK(give(1, SA_OK, SA_ERR_UNSUPPORTED) && fake.frames.size() == 1);
    CHECK(give(2, SA_OK, SA_PC_ENDED) && fake.frames.size() == 3 &&
          fake.xs[1].size() == 3 && fake.xs[2].size() == 6);
    /* refused through a picture's time, that picture's draws go to none.
     * the one after finds the first shown and its own begun */
    fake.refuses = 4;
    CHECK(give(1, SA_OK, SA_ERR_UNSUPPORTED));
    CHECK(give(2, SA_ERR_UNSUPPORTED, SA_OK) && fake.refuses == 0 &&
          fake.frames.size() == 3);
    CHECK(give(2, SA_OK, SA_PC_ENDED) && fake.frames.size() == 5 &&
          fake.xs[3].size() == 3 && fake.xs[4].size() == 6);
    for (bool &b : fake.busy) {
        b = false;
    }
    sa_pc_client_destroy(client);
    sa_engine_destroy(engine);
    fake = Fake{};
    return 0;
}

/* run a test whose device refuses pictures, with stderr on /dev/null. the
 * backend prints each refusal there */
int unheard(int (*test)())
{
    fflush(stderr);
    int kept = dup(STDERR_FILENO);
    int none = open("/dev/null", O_WRONLY);

    dup2(none, STDERR_FILENO);
    close(none);
    int line = test();

    fflush(stderr);
    dup2(kept, STDERR_FILENO);
    close(kept);
    return line;
}

} // namespace

int main(int argc, char **argv)
{
    int line = argc > 1 ? adapter_capture(argv[1]) : transforms();

    if (argc > 1) {
        if (line != 0) {
            fprintf(stderr, "pictures: line %d\n", line);
        }
        return line != 0;
    }
    if (line == 0) {
        line = between();
    }
    if (line == 0) {
        line = palette_order();
    }
    if (line == 0) {
        line = written();
    }
    if (line == 0) {
        line = out_of_view();
    }
    if (line == 0) {
        line = scenes();
    }
    if (line == 0) {
        line = released();
    }
    if (line == 0) {
        line = loans();
    }
    if (line == 0) {
        line = rooms();
    }
    if (line == 0) {
        line = views();
    }
    if (line == 0) {
        line = client_scene();
    }
    if (line == 0) {
        line = named_textures();
    }
    if (line == 0) {
        line = palette_versions();
    }
    if (line == 0) {
        line = record_bounds();
    }
    if (line == 0) {
        line = client_released();
    }
    if (line == 0) {
        line = run();
    }
    if (line == 0) {
        line = whitened();
    }
    if (line == 0) {
        line = sampled();
    }
    if (line == 0) {
        line = left_out();
    }
    if (line == 0) {
        line = out_of_view_room();
    }
    if (line == 0) {
        line = unheard(refused);
    }
    if (line == 0) {
        line = overview_room();
    }
    if (line == 0) {
        line = shown_bound();
    }
    if (line == 0) {
        line = unheard(window_broken);
    }
    if (line == 0) {
        line = unheard(client_refused);
    }
    if (line != 0) {
        fprintf(stderr, "pictures: line %d\n", line);
    }
    return line != 0;
}
