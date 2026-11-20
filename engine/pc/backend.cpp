/* the PC backend: the runtime's resources and pictures on Vulkan, its clock,
 * pads and sound. it draws in a window or headless. ENGINE.md#pc-rendering
 * gives its rules. a release ends its handle at once. the resource retires
 * once the fence of the last picture that could read it has signaled */
#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <algorithm>
#include <map>
#include <new>
#include <string>
#include <type_traits>

#include "ninja.h"
#include "pvr.h"
#include "raster.h"
#include "runtime.h"
#include "sa_pc.h"
#include "sa_sa1.h"
#include "scene.h"
#include "sound.h"
#include "window.h"
#include "world.h"

namespace {

/* a live resource, by its handle's slot */
struct Resource {
    pc::MeshView mesh;
    const sa_material *materials = nullptr;
    uint32_t material_count = 0;
    uint32_t texture = 0; /* the raster's */
    bool refused = false; /* the raster refused its mesh for ray tracing */
};

/* a release on its way to SA_EVENT_RETIRED: once serial has retired */
struct Retiring {
    sa_handle handle;
    uint64_t serial;
    uint32_t texture;
    uint32_t mesh; /* the raster's mesh for ray tracing, or 0 */
};

/* a part of the act's scene, kept for ray tracing: the draw that last gave
 * or drew it, in the game's view or out of it, and that draw's shade. a
 * traced picture that leaves the part out still gives it to shadow, bounce
 * and reflection rays */
struct Part {
    sa_draw draw;
    sa_shade shade;
};

/* a picture in flight, by its arena's slot */
struct Flight {
    uint64_t serial; /* 0 for none */
    uint32_t tick;
    uint16_t step;
    bool ended; /* the raster has it, so its slot's fence is its own */
};

/* what the open picture may hold */
constexpr uint32_t MOST_SHADES = 4096;
constexpr uint32_t MOST_LIGHTS = 1024; /* SA_SHADE_LIGHTS shades' */
constexpr uint32_t MOST_MATERIALS = 1024; /* in a list */
constexpr uint32_t MOST_SCENE_SHADES = 256; /* the open scene's */
/* an id the open picture has no view under */
constexpr uint32_t NO_VIEW = UINT32_MAX;
/* under a rate, the display shows pictures between the game's own. the
 * raster draws them in BETWEEN_ROOMS rooms, its frames after those of the
 * pictures in flight. sa_pc_read_overview draws in the room after those, so
 * it draws over no picture between. the display shows at most MOST_SHOWN
 * for one of the game's pictures. at the game's 30, that is 16 a step */
constexpr uint32_t BETWEEN_ROOMS = 4;
constexpr uint32_t OVERVIEW_ROOM = BETWEEN_ROOMS;
constexpr uint32_t RATE_ROOMS = OVERVIEW_ROOM + 1;
constexpr uint32_t MOST_SHOWN = 32;

struct Backend {
    pc::Raster raster;
    pc::Scene scene;
    /* under a rate: the render-owned scene, and the pictures between */
    pc::World world;
    pc::Scene between;
    uint32_t rate = 0; /* sa_pc_options.rate, in thousandths of a hertz */
    /* the rays a pixel of a traced picture, or 0 where every picture is
     * rasterized */
    uint32_t rays = 0;
    bool traces = false; /* whether the device traces */
    uint32_t rays_asked = 0; /* the count F10 turns ray tracing back on to */
    /* the window's title as it was asked. on a device that traces, the
     * title shown adds whether ray tracing is on */
    std::string title;
    /* pairs each traced picture with the traced picture sent before it */
    pc::RayPast past;
    /* the lamps of the draws a traced picture lacks, which both scenes
     * hold and the game's own pictures teach */
    pc::LampMemory lamps;
    /* on a device that traces, each raster texture's colors, by its index,
     * as Scene::open takes them */
    std::vector<pc::TextureColors> colors;
    /* on a device that traces, the parts of the scene that stays, by their
     * instance. they are kept while ray tracing is off too, so that turning
     * it on again finds the act whole */
    std::map<uint32_t, Part> parts;
    /* the open picture has the parts it left out, which sa_present_end gives
     * it once however often the call is made */
    bool parts_hidden = false;
    double per_step = 0; /* the display's pictures a step of the game */
    double owed = 0; /* the display's pictures the steps so far owe */
    uint32_t shown = 1; /* for the picture last ended */
    /* of those, the ones that repeat the picture itself */
    uint32_t repeated = 0;
    uint32_t full = 0; /* SA_PC_FULL_*, or 0 */
    uint32_t between_room[MOST_SHOWN] = {}; /* each picture between's room */
    uint64_t between_made = 0;
    uint64_t between_ns = 0;
    /* in a window, show_next is the first index the display has yet to
     * show. the last index is the picture itself, in show_slot. index i is
     * due at show_first_ns plus i times period_ns */
    uint32_t show_next = 1; /* nothing yet */
    uint32_t show_slot = 0;
    uint64_t show_first_ns = 0;
    uint64_t period_ns = 0;
    pc::Window *window = nullptr;
    Resource *resources = nullptr;
    Retiring *retiring = nullptr; /* a ring of desc.events */
    uint32_t retiring_first = 0;
    uint32_t retiring_count = 0;
    Flight *flights = nullptr; /* one for each of desc.pictures */
    sa::Arena *picture = nullptr; /* the open picture's, or null */
    uint32_t picture_used = 0; /* what the picture last ended took of it */
    sa_shade shades[MOST_SHADES]; /* the open picture's */
    uint32_t shade_count = 0;
    /* the open picture's lights, in its arena, and the shade each is for */
    const sa_lights *lights[MOST_LIGHTS] = {};
    uint32_t light_count = 0;
    /* the scene's index of the open picture's view each id names, or
     * NO_VIEW. id 0 names the picture's own. the picture is drawn in the
     * space of the scene's home: the view its first draw names, or a traced
     * picture's own view. a draw under another view is carried into the
     * home */
    uint32_t views[SA_VIEWS + 1] = {};
    uint32_t home = 0;
    bool homed = false;
    /* the act's scene is open from sa_scene_begin to sa_scene_end, and stays
     * from then until sa_scene_ended names it. scene_lit says whether it has
     * its lighting */
    bool scene_open = false;
    bool scene_stays = false;
    bool scene_lit = false;
    uint32_t scene_name = 0;
    sa_shade scene_shades[MOST_SCENE_SHADES];
    uint32_t scene_shade_count = 0;
    uint64_t opened = 0; /* the pictures begun: the last one's serial */
    uint64_t retired = 0; /* every picture up to this one has retired */
    uint64_t began_ns = 0;
    sa_pc_stats stats{};
    uint32_t draws = 0;
    uint32_t last_frame = 0; /* the slot of the picture last ended */
    /* the window the game last asked, SA_ASPECT_*, and whether the pictures
     * from the next are 16:9 */
    uint32_t window_asked = SA_ASPECT_4_3;
    bool wide = false;
    bool quit_sent = false;
    /* sound, which a window's device renders on its own thread, or else
     * sa_pc_audio_render on the caller's */
    pc::Sound sound;
    bool sound_made = false;
    bool sound_device = false;
};

void render_sound(void *user, int16_t *out, uint32_t frames)
{
    static_cast<pc::Sound *>(user)->render(out, frames);
}

Backend &backend(sa_engine &engine)
{
    return *static_cast<Backend *>(engine.backend);
}

uint64_t now_ns()
{
    timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);
    return static_cast<uint64_t>(t.tv_sec) * 1000000000U +
           static_cast<uint64_t>(t.tv_nsec);
}

void post(sa_engine &engine, uint32_t kind, sa_handle handle, int32_t result,
          uint32_t tick)
{
    sa_event event{};

    event.kind = kind;
    event.handle = handle;
    event.result = result;
    event.tick = tick;
    event.time_ns = now_ns();
    sa::event_post(engine, event);
}

/* take the news of the pictures in flight, oldest first, then retire what
 * waited on them */
void collect(sa_engine &engine)
{
    Backend &b = backend(engine);

    while (b.retired < b.opened) {
        uint32_t slot = static_cast<uint32_t>(b.retired % engine.desc.pictures);
        Flight &flight = b.flights[slot];

        if (!flight.ended || !b.raster.done(slot)) {
            break;
        }
        post(engine, SA_EVENT_PRESENTED, SA_NULL, flight.step, flight.tick);
        b.retired = flight.serial;
    }
    while (b.retiring_count != 0) {
        Retiring &r = b.retiring[b.retiring_first];

        if (r.serial > b.retired) {
            break;
        }
        if (r.texture != 0) {
            b.raster.free_texture(r.texture);
        }
        if (r.mesh != 0) {
            b.raster.free_mesh(r.mesh);
        }
        post(engine, SA_EVENT_RETIRED, r.handle, 0, engine.tick);
        b.retiring_first = (b.retiring_first + 1) % engine.desc.events;
        b.retiring_count--;
    }
}

/* whether count floats from at, which may cross a record's fields, are all
 * finite */
bool finite(const void *at, size_t count)
{
    const auto *bytes = static_cast<const uint8_t *>(at);

    for (size_t i = 0; i < count; i++) {
        float f;

        memcpy(&f, bytes + i * sizeof f, sizeof f);
        if (!isfinite(f)) {
            return false;
        }
    }
    return true;
}

/* finite over a field or a record that is floats alone. an array a function
 * takes as a parameter is a pointer, whose size is not the floats' */
template <typename Floats> bool all_finite(const Floats &floats)
{
    static_assert(!std::is_pointer_v<Floats>, "all_finite of a pointer");
    return finite(&floats, sizeof floats / sizeof(float));
}

/* the runs of floats that finite reads across a record's fields */
constexpr size_t POINT_FLOATS = offsetof(sa_point, color) / sizeof(float);
constexpr size_t LIGHT_FLOATS =
    (sizeof(sa_light) - offsetof(sa_light, point)) / sizeof(float);
constexpr size_t LIGHTING_FLOATS =
    (offsetof(sa_lighting, count) - offsetof(sa_lighting, direction)) /
    sizeof(float);

/* whether the floats a sprite's draw uses are finite. a screen sprite's
 * depth comes from its bias, not its z. its angle counts only under
 * SA_SPRITE_ANGLE */
bool sprite_finite(const sa_sprite &s)
{
    bool screen = (s.flags & SA_SPRITE_SCREEN) != 0;

    return all_finite(s.object_to_world) && finite(s.at, 2) &&
           all_finite(s.scale) && all_finite(s.corner) && all_finite(s.uv) &&
           all_finite(s.bias) && (screen || all_finite(s.at[2])) &&
           (!(s.flags & SA_SPRITE_ANGLE) || all_finite(s.angle));
}

/* a live resource of kind, or null */
Resource *live(sa_engine &engine, sa_handle handle, uint32_t kind)
{
    int32_t slot = engine.resources.find(handle);

    if (slot < 0 || SA_HANDLE_KIND(handle) != kind) {
        return nullptr;
    }
    return &backend(engine).resources[slot];
}

/* the raster's texture a record names, to texture, 0 for none. it returns
 * false where the handle names no live texture */
bool texture_of(sa_engine &engine, sa_handle handle, uint32_t &texture)
{
    texture = 0;
    if (handle == SA_NULL) {
        return true;
    }
    Resource *r = live(engine, handle, SA_KIND_TEXTURE);

    if (r == nullptr) {
        return false;
    }
    texture = r->texture;
    return true;
}

/* the last of count shades given under id, or null */
const sa_shade *shade_of(const sa_shade *shades, uint32_t count, uint32_t id)
{
    for (uint32_t i = count; i > 0; i--) {
        if (shades[i - 1].shade == id) {
            return &shades[i - 1];
        }
    }
    return nullptr;
}

/* whether each of count materials names a live texture, or none */
bool textures_live(sa_engine &engine, const sa_material *materials,
                   uint32_t count)
{
    uint32_t unused = 0;

    for (uint32_t i = 0; i < count; i++) {
        if (!texture_of(engine, materials[i].texture, unused)) {
            return false;
        }
    }
    return true;
}

const sa_lights *lights_of(const Backend &b, uint32_t shade)
{
    for (uint32_t i = b.light_count; i > 0; i--) {
        if (b.lights[i - 1]->shade == shade) {
            return b.lights[i - 1];
        }
    }
    return nullptr;
}

/* the scene's index of the open picture's home, where first is the view
 * the first record of a call names: the home the picture has, or the one
 * that call gives it. a traced picture's home is its own view, as
 * sa_engine.h says of a backend that makes a picture of its own. the trace
 * pass takes the home's space for the game's world, whose y is up and in
 * which the parts a picture leaves out stand. another picture's home is the
 * view its first record names */
uint32_t home_of(const Backend &b, uint32_t first)
{
    if (b.homed) {
        return b.home;
    }
    return b.scene.traces() ? 0 : b.views[first];
}

/* first is the view the first record of the call names. it and id must be
 * views the picture has. a record placed in the world needs the scene to
 * carry its view into the home */
sa_result check_view(const Backend &b, uint32_t id, uint32_t first, bool places)
{
    if (id > SA_VIEWS || b.views[id] == NO_VIEW || first > SA_VIEWS ||
        b.views[first] == NO_VIEW) {
        return SA_ERR_HANDLE;
    }
    return !places || b.scene.carries(home_of(b, first), b.views[id])
               ? SA_OK
               : SA_ERR_INVALID;
}

/* the picture's first record, which names view id, gives the scene its
 * home */
void take_home(Backend &b, uint32_t id)
{
    if (!b.homed) {
        b.home = home_of(b, id);
        b.homed = true;
        b.scene.home(b.home);
    }
}

/* the mesh a draw names: a resource's, or its payload's */
sa_result draw_mesh(sa_engine &engine, const sa_draw &d, pc::MeshView &mesh)
{
    Backend &b = backend(engine);

    if (d.mesh != SA_NULL) {
        Resource *r = live(engine, d.mesh, SA_KIND_MESH);

        if (r == nullptr) {
            return SA_ERR_HANDLE;
        }
        mesh = r->mesh;
        return SA_OK;
    }
    uint32_t used = b.picture->used();

    if (d.payload >= used || d.payload % 4 != 0 ||
        !pc::mesh_view(b.picture->at(d.payload), used - d.payload, mesh)) {
        return SA_ERR_INVALID;
    }
    return SA_OK;
}

/* the raster's mesh, for ray tracing, of the resource a draw names. the
 * raster takes the mesh the first time a draw names it, and holds it until
 * the resource retires. 0 where the draw's mesh is a payload's, or the
 * device refuses the mesh, which then says so once and is rasterized */
uint32_t ray_mesh_of(sa_engine &engine, const sa_draw &d)
{
    Backend &b = backend(engine);
    Resource *r =
        d.mesh != SA_NULL ? live(engine, d.mesh, SA_KIND_MESH) : nullptr;

    if (r == nullptr || r->refused) {
        return 0;
    }
    if (r->mesh.rays == 0) {
        pc::RayMesh mesh;
        std::string why;

        pc::ray_mesh(r->mesh, mesh);
        r->mesh.rays = b.raster.add_mesh(mesh, why);
        if (r->mesh.rays == 0) {
            fprintf(stderr, "engine: %s\n", why.c_str());
            r->refused = true;
        }
    }
    return r->mesh.rays;
}

/* check a part's names before any part given with it is taken. its mesh
 * and materials must be live, and its shade one the open scene has */
sa_result check_part(sa_engine &engine, const sa_draw &d, pc::MeshView &mesh,
                     const Resource *&materials, const sa_shade *&shade)
{
    Backend &b = backend(engine);

    if (!pc::of_scene(d.instance) || d.payload != SA_NO_PAYLOAD ||
        !all_finite(d.object_to_world)) {
        return SA_ERR_INVALID;
    }
    Resource *r = live(engine, d.mesh, SA_KIND_MESH);

    materials = live(engine, d.materials, SA_KIND_MATERIAL);
    shade = shade_of(b.scene_shades, b.scene_shade_count, d.shade);
    if (r == nullptr || materials == nullptr || shade == nullptr) {
        return SA_ERR_HANDLE;
    }
    mesh = r->mesh;
    if (materials->material_count < mesh.materials) {
        return SA_ERR_INVALID;
    }
    return textures_live(engine, materials->materials,
                         materials->material_count)
               ? SA_OK
               : SA_ERR_HANDLE;
}

/* check a draw's names before any draw given with it is taken */
sa_result check_draw(sa_engine &engine, const sa_draw &d, pc::MeshView &mesh,
                     const Resource *&materials, const sa_shade *&shade,
                     const sa_lights *&lights)
{
    Backend &b = backend(engine);
    sa_result result = draw_mesh(engine, d, mesh);

    if (result != SA_OK) {
        return result;
    }
    if (!all_finite(d.object_to_world)) {
        return SA_ERR_INVALID;
    }
    materials = live(engine, d.materials, SA_KIND_MATERIAL);
    shade = shade_of(b.shades, b.shade_count, d.shade);
    if (materials == nullptr || shade == nullptr) {
        return SA_ERR_HANDLE;
    }
    if (materials->material_count < mesh.materials) {
        return SA_ERR_INVALID;
    }
    if (shade->mode == SA_SHADE_PALETTE && !b.scene.has_palettes()) {
        return SA_ERR_INVALID;
    }
    lights = nullptr;
    if (shade->mode == SA_SHADE_LIGHTS) {
        lights = lights_of(b, d.shade);
        if (lights == nullptr) {
            return SA_ERR_HANDLE;
        }
    }
    return textures_live(engine, materials->materials,
                         materials->material_count)
               ? SA_OK
               : SA_ERR_HANDLE;
}

/* give scene, which holds a traced picture, each of the act's parts that it was
 * given no draw of, placed where the part's last draw put it. no camera ray
 * hits them. such a part goes under the picture's own view, so it is where the
 * game's world has it. a part the picture draws under another view is carried
 * to the home, so the two line up only where that view has the picture's own
 * matrix. a part whose mesh or materials were freed is dropped */
void hide_parts(sa_engine &engine, pc::Scene &scene)
{
    Backend &b = backend(engine);

    for (auto at = b.parts.begin(); at != b.parts.end();) {
        Part &part = at->second;
        Resource *mesh = live(engine, part.draw.mesh, SA_KIND_MESH);
        Resource *materials =
            live(engine, part.draw.materials, SA_KIND_MATERIAL);
        uint32_t textures[MOST_MATERIALS];

        if (mesh == nullptr || materials == nullptr ||
            materials->material_count < mesh->mesh.materials ||
            !textures_live(engine, materials->materials,
                           materials->material_count)) {
            at = b.parts.erase(at);
            continue;
        }
        ++at;
        if (scene.holds(part.draw.instance) || mesh->mesh.triangles == 0) {
            continue;
        }
        pc::MeshView view = mesh->mesh;

        view.rays = ray_mesh_of(engine, part.draw);
        for (uint32_t k = 0; k < materials->material_count; k++) {
            texture_of(engine, materials->materials[k].texture, textures[k]);
        }
        scene.hidden(part.draw, view, materials->materials, textures,
                     part.shade, 0);
    }
}

/* trace the pictures from the next one with rays rays a pixel, or rasterize
 * them where rays is 0. the first traced picture after a change reuses nothing
 * of the pictures before it */
void take_rays(Backend &b, uint32_t rays)
{
    b.rays = rays;
    b.scene.trace(rays);
    b.between.trace(rays);
    b.past.forget();
    b.lamps.forget();
    if (b.traces) {
        pc::window_title(
            b.window,
            (b.title + (rays != 0 ? " (rays on)" : " (rays off)")).c_str());
    }
}

/* send scene's finished picture to the raster's frame. a traced picture is
 * paired with the traced picture sent before it, and is the one the next
 * pairs with once the device has taken it. it prints why the device refuses
 * a picture */
bool send(Backend &b, uint32_t frame, pc::Scene &scene)
{
    pc::PictureUniform uniform;
    std::string why;

    scene.uniform(uniform);
    scene.pair(b.past);
    if (!b.raster.submit(frame, uniform, scene.clear_color(), scene.vertices(),
                         scene.batches(), scene.sorted(), scene.ray_picture(),
                         why)) {
        fprintf(stderr, "engine: %s\n", why.c_str());
        return false;
    }
    if (scene.traces()) {
        b.past.keep();
    }
    return true;
}

/* draw and send picture index of those shown for the picture last ended.
 * the last is the picture itself, and each before it is a picture between */
bool show(sa_engine &engine, uint32_t index)
{
    Backend &b = backend(engine);
    uint64_t began = now_ns();

    if (index + 1 == b.shown) {
        if (!send(b, b.show_slot, b.scene)) {
            return false;
        }
        b.flights[b.show_slot].ended = true;
        b.last_frame = b.show_slot;
        return true;
    }
    uint32_t room = engine.desc.pictures +
                    static_cast<uint32_t>(b.between_made++ % BETWEEN_ROOMS);
    pc::Scene *scene = &b.between;

    b.raster.wait(room);
    /* the display never shows part of a picture. where the render-owned
     * scene can't make a whole picture between, the game's own shows in its
     * place. full then stays set, and each later picture between is the
     * game's own, so none steps back */
    if (b.full == SA_PC_FULL_BETWEEN ||
        !b.world.between(static_cast<float>(index + 1) / b.shown, b.between)) {
        scene = &b.scene;
        b.repeated++;
        if (b.full == 0) {
            b.full = SA_PC_FULL_BETWEEN;
        }
    } else {
        /* a traced picture between takes the parts it left out before it is
         * finished, as the game's own does */
        if (b.between.traces()) {
            hide_parts(engine, b.between);
        }
        b.between.finish();
    }
    if (!send(b, room, *scene)) {
        return false;
    }
    b.between_room[index] = room;
    b.between_ns += now_ns() - began;
    return true;
}

/* sleep to time_ns on the monotonic clock */
void sleep_until(uint64_t time_ns)
{
    timespec t;

    t.tv_sec = static_cast<time_t>(time_ns / 1000000000U);
    t.tv_nsec = static_cast<long>(time_ns % 1000000000U);
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, nullptr) ==
           EINTR) {
    }
}

/* show what is due by until, each at its time. UINT64_MAX shows all now */
bool show_due(sa_engine &engine, uint64_t until)
{
    Backend &b = backend(engine);

    while (b.show_next < b.shown) {
        uint64_t due = b.show_first_ns + (b.show_next * b.period_ns);

        if (until != UINT64_MAX) {
            if (due > until) {
                break;
            }
            sleep_until(due);
        }
        if (!show(engine, b.show_next)) {
            /* the picture itself stays due until the device takes it, as
             * collect retires nothing past a flight not ended. a picture
             * between that the device refuses is passed over */
            if (b.show_next + 1 < b.shown) {
                b.show_next++;
            }
            return false;
        }
        b.show_next++;
    }
    return true;
}

/* an image's mean color, ARGB. its alpha is the mean of its texels' own,
 * and each texel counts toward its color by its alpha */
uint32_t mean_color(const pc::Image &image)
{
    uint64_t sum[4] = {};
    size_t texels = image.rgba.size() / 4;

    for (size_t i = 0; i < texels; i++) {
        const uint8_t *texel = &image.rgba[i * 4];

        for (int k = 0; k < 3; k++) {
            sum[k] += static_cast<uint64_t>(texel[k]) * texel[3];
        }
        sum[3] += texel[3];
    }
    if (texels == 0 || sum[3] == 0) {
        return 0;
    }
    return static_cast<uint32_t>(sum[3] / texels) << 24 |
           static_cast<uint32_t>(sum[0] / sum[3]) << 16 |
           static_cast<uint32_t>(sum[1] / sum[3]) << 8 |
           static_cast<uint32_t>(sum[2] / sum[3]);
}

/* an image's emission under an emissive material, as TextureColors holds
 * it: the mean color of its bright texels, each counted by how far its
 * brightest channel lies from EMISSIVE_DIM to EMISSIVE_LIT and by its alpha,
 * with the share of the image they cover in the high byte. trace.comp
 * weighs a texel the same way */
uint32_t emission_color(const pc::Image &image)
{
    double sum[3] = {};
    double bright = 0.0;
    size_t texels = image.rgba.size() / 4;

    for (size_t i = 0; i < texels; i++) {
        const uint8_t *texel = &image.rgba[i * 4];
        float peak =
            static_cast<float>(std::max({texel[0], texel[1], texel[2]})) /
            255.0F;
        float t = std::clamp((peak - pc::EMISSIVE_DIM) /
                                 (pc::EMISSIVE_LIT - pc::EMISSIVE_DIM),
                             0.0F, 1.0F);
        double gives = t * t * (3.0 - 2.0 * t) * texel[3] / 255.0;

        for (int k = 0; k < 3; k++) {
            sum[k] += texel[k] * gives;
        }
        bright += gives;
    }
    if (texels == 0 || bright <= 0.0) {
        return 0;
    }
    uint32_t share = static_cast<uint32_t>(std::clamp(
        bright / static_cast<double>(texels) * 255.0 + 0.5, 1.0, 255.0));

    return share << 24 | static_cast<uint32_t>(sum[0] / bright) << 16 |
           static_cast<uint32_t>(sum[1] / bright) << 8 |
           static_cast<uint32_t>(sum[2] / bright);
}

/* a PVR's bytes, a GBIX chunk before its PVRT or not, as RGBA */
bool pvr_file(const void *data, uint32_t size, pc::Image &image)
{
    const auto *from = static_cast<const uint8_t *>(data);
    std::vector<uint8_t> bytes(from, from + size);
    std::string why;
    uint32_t at = 0;

    /* a GBIX chunk's length follows its name, as a PVRT's does. a chunk
     * that passes the bytes leaves no PVRT. 8 + length could wrap past 32
     * bits, back into the chunk's name and length */
    if (size >= 8 && memcmp(from, "GBIX", 4) == 0) {
        uint32_t length;

        memcpy(&length, from + 4, sizeof length);
        if (length > size - 8) {
            return false;
        }
        at = 8 + length;
    }
    return pc::pvr_decode(bytes, at, image, why);
}

/* where the engine can't be made: why, to options.why, or to stderr without
 * it */
void refuse(const sa_pc_options &options, const std::string &why)
{
    if (options.why != nullptr) {
        snprintf(options.why, SA_PC_WHY, "%s", why.c_str());
    } else {
        fprintf(stderr, "engine: %s\n", why.c_str());
    }
}

} // namespace

namespace sa {

uint32_t backend_memory(const sa_engine_desc &desc)
{
    /* backend_create aligns the records after Backend to 8, which takes
     * under 8 of these bytes */
    const uint64_t slack = 64;
    uint64_t bytes = sizeof(Backend) +
                     (uint64_t{desc.resources} * sizeof(Resource)) +
                     (uint64_t{desc.events} * sizeof(Retiring)) +
                     (uint64_t{desc.pictures} * sizeof(Flight)) + slack;

    /* past 32 bits it gives UINT32_MAX, which the core's check refuses */
    return bytes > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(bytes);
}

sa_result backend_create(sa_engine &engine)
{
    static const sa_pc_options none = {};
    const auto *options =
        engine.desc.platform != nullptr
            ? static_cast<const sa_pc_options *>(engine.desc.platform)
            : &none;
    auto *bytes = static_cast<uint8_t *>(engine.backend);
    std::string why;

    if (engine.desc.resources == 0 || options->rays > SA_PC_PIXEL_RAYS ||
        !(options->render_scale == 0.0F ||
          (options->render_scale >= SA_PC_RENDER_SCALE_LEAST &&
           options->render_scale <= 1.0F))) {
        return SA_ERR_INVALID;
    }
    auto *made = new (bytes) Backend();

    bytes += sizeof(Backend);
    bytes = reinterpret_cast<uint8_t *>(
        (reinterpret_cast<uintptr_t>(bytes) + 7) & ~uintptr_t{7});
    made->resources = reinterpret_cast<Resource *>(bytes);
    for (uint32_t i = 0; i < engine.desc.resources; i++) {
        new (&made->resources[i]) Resource();
    }
    bytes += engine.desc.resources * sizeof(Resource);
    made->retiring = reinterpret_cast<Retiring *>(bytes);
    bytes += engine.desc.events * sizeof(Retiring);
    made->flights = reinterpret_cast<Flight *>(bytes);
    for (uint32_t i = 0; i < engine.desc.pictures; i++) {
        made->flights[i] = Flight{};
    }
    pc::RasterOptions raster;

    raster.device = options->device != nullptr ? options->device : "";
    raster.frames = engine.desc.pictures;
    if (options->rate != 0) {
        /* a picture between reads the picture before it, whose arena the
         * next picture in its slot resets */
        if (engine.desc.pictures < 2) {
            made->~Backend();
            return SA_ERR_INVALID;
        }
        /* the game's 59.94 steps a second, as the Dreamcast's vsync */
        const double step_hz = 60000.0 / 1001.0;
        /* a rate within sa_pc.h's 0.2% of a multiple of step_hz counts as
         * that multiple */
        const double snap = 0.002;
        double per_step = options->rate / 1000.0 / step_hz;
        double nearest = floor(per_step + 0.5);

        if (nearest >= 1 && fabs(per_step - nearest) <= nearest * snap) {
            per_step = nearest;
        }
        made->rate = options->rate;
        made->per_step = per_step;
        made->period_ns = 1000000000000ULL / options->rate;
        raster.frames += RATE_ROOMS;
    }
    raster.vsync = (options->flags & SA_PC_NO_VSYNC) == 0;
    raster.readback = (options->flags & SA_PC_READBACK) != 0;
    raster.resolution = options->resolution;
    raster.rays = options->rays != 0;
    raster.render_scale =
        options->render_scale != 0.0F ? options->render_scale : 1.0F;
    made->title =
        options->title != nullptr ? options->title : "Sonic Adventure";
    if (options->flags & SA_PC_WINDOW) {
        uint32_t scale = options->scale != 0 ? options->scale : 2;
        pc::Image icon;
        bool has_icon = options->icon != nullptr &&
                        pvr_file(options->icon, options->icon_bytes, icon);

        made->window = pc::window_open(
            made->title.c_str(), SA_PC_WIDTH * scale, SA_PC_HEIGHT * scale,
            options->flags & (SA_PC_FULLSCREEN | SA_PC_EXCLUSIVE),
            options->settings, has_icon ? &icon : nullptr, why);
        if (made->window == nullptr) {
            refuse(*options, why);
            made->~Backend();
            return SA_ERR_UNSUPPORTED;
        }
        raster.window = made->window;
    }
    if (!made->raster.open(raster, why)) {
        pc::Window *window = made->window;

        /* the raster's surface ends before the window it shows in */
        refuse(*options, why);
        made->~Backend();
        pc::window_close(window);
        return SA_ERR_UNSUPPORTED;
    }
    made->scene.open(options->blend, &made->colors);
    made->scene.remember(&made->lamps, true);
    made->between.remember(&made->lamps, false);
    if (options->rays != 0 && !made->raster.traces()) {
        fprintf(stderr,
                "engine: %s can't ray trace, so pictures are rasterized\n",
                made->raster.device_name().c_str());
    }
    made->traces = made->raster.traces();
    made->rays_asked = made->traces ? options->rays : 0;
    take_rays(*made, made->rays_asked);
    if (made->rate != 0) {
        made->between.open(options->blend, &made->colors);
        /* a release holds an event until it retires. one made while a
         * picture is open retires after it. desc.events thus bounds a
         * picture's releases */
        made->world.open(engine.desc.events);
    }
    made->sound_made = made->sound.create();
    if (made->sound_made && (options->flags & SA_PC_SOUND) &&
        made->window != nullptr) {
        made->sound_device = pc::window_sound_open(made->window, render_sound,
                                                   &made->sound, why);
        if (!made->sound_device) {
            fprintf(stderr, "engine: no sound: %s\n", why.c_str());
        }
    }
    return SA_OK;
}

void backend_destroy(sa_engine &engine)
{
    Backend &b = backend(engine);

    show_due(engine, UINT64_MAX);
    for (uint32_t i = 0; i < engine.desc.pictures; i++) {
        b.raster.wait(i);
    }
    pc::Window *window = b.window;

    pc::window_sound_close(window);
    b.sound.destroy();
    b.~Backend();
    pc::window_close(window);
}

} // namespace sa

extern "C" {

uint64_t sa_clock_ns(sa_engine *)
{
    return now_ns();
}

void sa_pc_alert(const char *title, const char *words)
{
    pc::window_alert(title, words);
}

sa_result sa_clock_wait(sa_engine *engine, uint64_t time_ns)
{
    if (engine == nullptr) {
        return SA_ERR_INVALID;
    }
    /* a window shows each picture as its time comes, while the client waits */
    Backend &b = backend(*engine);
    sa_result result = SA_OK;

    if (b.show_next < b.shown && b.picture == nullptr &&
        !show_due(*engine, time_ns)) {
        result = SA_ERR_UNSUPPORTED;
    }
    /* the client keeps its time by the wait, so a picture the device refuses
     * cuts it no shorter */
    sleep_until(time_ns);
    return result;
}

sa_result sa_input_sample(sa_engine *engine, uint32_t port, sa_pad *pad)
{
    if (engine == nullptr || pad == nullptr) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    if (b.window == nullptr) {
        return SA_EMPTY;
    }
    if (pc::window_pump(b.window) && !b.quit_sent &&
        sa::event_reserve(*engine) == SA_OK) {
        b.quit_sent = true;
        post(*engine, SA_EVENT_QUIT, SA_NULL, 0, engine->tick);
    }
    return pc::window_pad(b.window, port, *pad);
}

sa_result sa_input_rumble(sa_engine *engine, uint32_t port, uint16_t low,
                          uint16_t high, uint32_t ms, uint32_t fade_ms)
{
    if (engine == nullptr) {
        return SA_ERR_INVALID;
    }
    return pc::window_rumble(backend(*engine).window, port, low, high, ms,
                             fade_ms);
}

sa_result sa_audio_submit(sa_engine *engine, const sa_audio_command *commands,
                          uint32_t count)
{
    if (engine == nullptr || (commands == nullptr && count != 0)) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    return b.sound_made ? b.sound.submit(commands, count) : SA_ERR_UNSUPPORTED;
}

sa_result sa_audio_load(sa_engine *engine, const sa_audio_bank *bank)
{
    if (engine == nullptr || bank == nullptr) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    return b.sound_made ? b.sound.load(*bank) : SA_ERR_UNSUPPORTED;
}

sa_result sa_audio_stream(sa_engine *engine, uint32_t stream, const void *data,
                          uint32_t bytes)
{
    if (engine == nullptr) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    return b.sound_made ? b.sound.stream(stream, data, bytes)
                        : SA_ERR_UNSUPPORTED;
}

sa_result sa_pc_audio_render(sa_engine *engine, int16_t *out, uint32_t frames)
{
    if (engine == nullptr || (out == nullptr && frames != 0)) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    if (!b.sound_made || b.sound_device) {
        return SA_ERR_UNSUPPORTED;
    }
    b.sound.render(out, frames);
    return SA_OK;
}

sa_result sa_pc_sound_stats_read(sa_engine *engine, sa_pc_sound_stats *stats)
{
    if (engine == nullptr || stats == nullptr) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    if (!b.sound_made) {
        return SA_ERR_UNSUPPORTED;
    }
    *stats = b.sound.stats();
    return SA_OK;
}

sa_result sa_resource_create(sa_engine *engine, const sa_resource_desc *desc,
                             sa_handle *resource)
{
    if (engine == nullptr || desc == nullptr || resource == nullptr ||
        (desc->data == nullptr && desc->bytes != 0)) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);
    Resource made;
    pc::Image image;

    switch (desc->format) {
    case SA_FORMAT_MESH:
        if (desc->kind != SA_KIND_MESH ||
            !pc::mesh_view(desc->data, desc->bytes, made.mesh)) {
            return SA_ERR_INVALID;
        }
        break;
    case SA_FORMAT_PIXELS: {
        sa_pixels size;

        if (desc->kind != SA_KIND_TEXTURE || desc->bytes < sizeof size) {
            return SA_ERR_INVALID;
        }
        memcpy(&size, desc->data, sizeof size);
        if (size.width == 0 || size.height == 0 || size.width > 4096 ||
            size.height > 4096 ||
            desc->bytes < sizeof size + (size.width * size.height * 4)) {
            return SA_ERR_INVALID;
        }
        const auto *rows =
            static_cast<const uint8_t *>(desc->data) + sizeof size;

        image.width = size.width;
        image.height = size.height;
        image.rgba.assign(rows, rows + (size.width * size.height * 4));
        break;
    }
    case SA_FORMAT_PVR:
        if (desc->kind != SA_KIND_TEXTURE ||
            !pvr_file(desc->data, desc->bytes, image)) {
            return SA_ERR_INVALID;
        }
        break;
    case SA_FORMAT_MATERIALS: {
        if (desc->kind != SA_KIND_MATERIAL ||
            desc->bytes % sizeof(sa_material) != 0 ||
            desc->bytes / sizeof(sa_material) > MOST_MATERIALS) {
            return SA_ERR_INVALID;
        }
        made.materials = static_cast<const sa_material *>(desc->data);
        made.material_count = desc->bytes / sizeof(sa_material);
        if (!textures_live(*engine, made.materials, made.material_count)) {
            return SA_ERR_HANDLE;
        }
        break;
    }
    default:
        return SA_ERR_INVALID;
    }
    sa_result result = sa::event_reserve(*engine);

    if (result != SA_OK) {
        return result;
    }
    sa_handle handle = engine->resources.take(desc->kind);

    if (handle == SA_NULL) {
        engine->event_reserved--;
        return SA_ERR_FULL;
    }
    if (desc->kind == SA_KIND_TEXTURE) {
        std::string why;

        made.texture = b.raster.add_texture(image, why);
        if (made.texture == 0) {
            fprintf(stderr, "engine: %s\n", why.c_str());
            engine->resources.give(handle);
            engine->event_reserved--;
            return SA_ERR_FULL;
        }
        /* only a traced picture reads a texture's colors */
        if (b.traces) {
            if (b.colors.size() <= made.texture) {
                b.colors.resize(made.texture + 1);
            }
            b.colors[made.texture] = {mean_color(image), emission_color(image)};
        }
    }
    b.resources[engine->resources.find(handle)] = made;
    *resource = handle;
    return SA_OK;
}

sa_result sa_resource_release(sa_engine *engine, sa_handle resource)
{
    if (engine == nullptr) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);
    uint32_t kind = SA_HANDLE_KIND(resource);
    int32_t slot = engine->resources.find(resource);

    if (slot < 0 || (kind != SA_KIND_MESH && kind != SA_KIND_TEXTURE &&
                     kind != SA_KIND_MATERIAL)) {
        return SA_ERR_HANDLE;
    }
    uint32_t at = (b.retiring_first + b.retiring_count) % engine->desc.events;

    /* the resource retires after the last picture begun, which may read it.
     * under a rate it waits one picture more, for the pictures between.
     * each release holds the event sa_resource_create reserved for it. the
     * ring has a place for each event, so it can't fill */
    b.retiring[at] =
        Retiring{resource, b.opened + (b.rate != 0 ? 1 : 0),
                 b.resources[slot].texture, b.resources[slot].mesh.rays};
    if (b.rate != 0) {
        b.world.released(
            resource, kind == SA_KIND_TEXTURE ? b.resources[slot].texture : 0);
    }
    b.retiring_count++;
    b.resources[slot] = Resource{};
    engine->resources.give(resource);
    collect(*engine);
    return SA_OK;
}

sa_result sa_present_begin(sa_engine *engine, const sa_picture *picture)
{
    if (engine == nullptr || picture == nullptr ||
        backend(*engine).picture != nullptr || !all_finite(picture->view)) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    /* what the display has yet to show of the last picture shows now, at
     * that picture's aspect */
    if (!show_due(*engine, UINT64_MAX)) {
        return SA_ERR_UNSUPPORTED;
    }
    if (b.traces && pc::window_rays_key(b.window)) {
        take_rays(b, b.rays != 0 ? 0 : b.rays_asked);
    }
    b.raster.wide(b.wide);
    uint32_t slot = static_cast<uint32_t>(b.opened % engine->desc.pictures);
    uint64_t began = now_ns();
    sa::Arena &arena = engine->pictures[slot];

    /* the slot's last picture retires before its arena is written over */
    if (b.flights[slot].serial != 0) {
        b.raster.wait(slot);
    }
    collect(*engine);
    sa_result result = sa::event_reserve(*engine);

    if (result != SA_OK) {
        return result;
    }
    arena.reset();
    b.shade_count = 0;
    b.light_count = 0;
    b.opened++;
    b.flights[slot] = Flight{b.opened, picture->tick, picture->step, false};
    b.picture = &arena;
    b.began_ns = began;
    b.draws = 0;
    b.views[0] = 0;
    for (uint32_t i = 1; i <= SA_VIEWS; i++) {
        b.views[i] = NO_VIEW;
    }
    b.homed = false;
    b.parts_hidden = false;
    /* a traced picture after a cut reuses nothing of the one before it */
    if (picture->flags & SA_PICTURE_CUT) {
        b.past.forget();
    }
    b.scene.begin(*picture, picture->tick * pc::TICK_SECONDS);
    if (b.rate != 0) {
        b.world.begin(*picture, b.opened);
    }
    return SA_OK;
}

sa_result sa_present_fog(sa_engine *engine, const sa_fog *fog)
{
    if (engine == nullptr || fog == nullptr ||
        backend(*engine).picture == nullptr) {
        return SA_ERR_INVALID;
    }
    backend(*engine).scene.fog(*fog);
    if (backend(*engine).rate != 0) {
        backend(*engine).world.fog(*fog);
    }
    return SA_OK;
}

sa_result sa_present_palettes(sa_engine *engine, const sa_palettes *palettes)
{
    if (engine == nullptr || palettes == nullptr ||
        backend(*engine).picture == nullptr) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);
    void *copy = b.picture->take(sizeof palettes->words, 4);

    if (copy == nullptr) {
        return SA_ERR_FULL;
    }
    memcpy(copy, palettes->words, sizeof palettes->words);
    b.scene.palettes(static_cast<const uint32_t *>(copy));
    if (b.rate != 0) {
        b.world.palettes(static_cast<const uint32_t *>(copy));
    }
    return SA_OK;
}

sa_result sa_present_copy(sa_engine *engine, const void *data, uint32_t bytes,
                          uint32_t *payload)
{
    if (engine == nullptr || data == nullptr || payload == nullptr ||
        backend(*engine).picture == nullptr) {
        return SA_ERR_INVALID;
    }
    sa::Arena &arena = *backend(*engine).picture;
    void *copy = arena.take(bytes, 4);

    if (copy == nullptr) {
        return SA_ERR_FULL;
    }
    memcpy(copy, data, bytes);
    *payload = arena.used() - bytes;
    return SA_OK;
}

sa_result sa_present_shade(sa_engine *engine, const sa_shade *shade)
{
    if (engine == nullptr || shade == nullptr ||
        backend(*engine).picture == nullptr) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    if (b.shade_count == MOST_SHADES) {
        return SA_ERR_FULL;
    }
    b.shades[b.shade_count++] = *shade;
    return SA_OK;
}

sa_result sa_present_lights(sa_engine *engine, const sa_lights *lights)
{
    if (engine == nullptr || lights == nullptr ||
        backend(*engine).picture == nullptr || lights->count > SA_LIGHTS) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    for (uint32_t i = 0; i < lights->count; i++) {
        if (!finite(lights->light[i].point, LIGHT_FLOATS)) {
            return SA_ERR_INVALID;
        }
    }
    for (uint32_t i = 0; i < lights->count; i++) {
        if (!pc::ninja_light_known(lights->light[i].type)) {
            return SA_ERR_UNSUPPORTED;
        }
    }
    if (b.light_count == MOST_LIGHTS) {
        return SA_ERR_FULL;
    }
    void *copy = b.picture->take(sizeof *lights, 4);

    if (copy == nullptr) {
        return SA_ERR_FULL;
    }
    memcpy(copy, lights, sizeof *lights);
    b.lights[b.light_count++] = static_cast<const sa_lights *>(copy);
    return SA_OK;
}

/* a further view of the open picture's. the scene reads only its matrix:
 * the raster projects every draw as the picture's own view does */
sa_result sa_present_view(sa_engine *engine, const sa_picture_view *view)
{
    if (engine == nullptr || view == nullptr ||
        backend(*engine).picture == nullptr || view->view == 0 ||
        view->view > SA_VIEWS ||
        backend(*engine).views[view->view] != NO_VIEW ||
        !all_finite(view->value)) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    b.views[view->view] = b.scene.view(view->value);
    if (b.rate != 0) {
        b.world.view(view->view, view->value);
    }
    return SA_OK;
}

sa_result sa_present_draw(sa_engine *engine, const sa_draw *draws,
                          uint32_t count)
{
    if (engine == nullptr || draws == nullptr ||
        backend(*engine).picture == nullptr) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);
    uint64_t triangles = 0;

    for (uint32_t i = 0; i < count; i++) {
        pc::MeshView mesh;
        const Resource *materials = nullptr;
        const sa_shade *shade = nullptr;
        const sa_lights *lights = nullptr;
        sa_result result =
            check_draw(*engine, draws[i], mesh, materials, shade, lights);

        if (result == SA_OK) {
            result = check_view(b, draws[i].view, draws[0].view, true);
        }
        if (result != SA_OK) {
            return result;
        }
        /* a draw out of the game's view adds no triangle to the picture */
        if (!(draws[i].flags & SA_DRAW_OUT)) {
            triangles += mesh.triangles;
        }
    }
    if (triangles > UINT32_MAX ||
        !b.scene.room(static_cast<uint32_t>(triangles))) {
        return SA_ERR_FULL;
    }
    if (count != 0) {
        take_home(b, draws[0].view);
    }
    for (uint32_t i = 0; i < count; i++) {
        pc::MeshView mesh;
        const Resource *materials = nullptr;
        const sa_shade *shade = nullptr;
        const sa_lights *lights = nullptr;
        uint32_t textures[MOST_MATERIALS];

        /* the loop above passed this draw, so the call can't fail */
        check_draw(*engine, draws[i], mesh, materials, shade, lights);
        /* no picture shows a draw out of the game's view. the render-owned
         * scene still keeps its instance where the draw put it */
        bool out = (draws[i].flags & SA_DRAW_OUT) != 0;

        /* a draw of a part, in the game's view or out of it, is the part as
         * ray tracing keeps it from then. so a picture's last draw of a part
         * is the one kept */
        if (b.traces && b.scene_stays && draws[i].mesh != SA_NULL &&
            pc::of_scene(draws[i].instance)) {
            b.parts[draws[i].instance] = Part{draws[i], *shade};
        }
        if (out && b.rate == 0 && b.rays == 0) {
            b.draws++;
            continue;
        }
        for (uint32_t k = 0; k < materials->material_count; k++) {
            texture_of(*engine, materials->materials[k].texture, textures[k]);
        }
        if (b.rays != 0 && mesh.triangles != 0) {
            mesh.rays = ray_mesh_of(*engine, draws[i]);
        }
        if (!out && mesh.meshsets != 0) {
            b.scene.draw(draws[i], mesh, materials->materials, textures, *shade,
                         lights, b.views[draws[i].view]);
        } else if (out && mesh.meshsets != 0) {
            /* a traced picture gives a draw out of the game's view to
             * shadow, bounce and reflection rays */
            b.scene.hidden(draws[i], mesh, materials->materials, textures,
                           *shade, b.views[draws[i].view]);
        }
        if (b.rate != 0) {
            b.world.draw(draws[i], mesh, materials->materials, textures, *shade,
                         lights);
        }
        b.draws++;
    }
    return SA_OK;
}

sa_result sa_present_sprite(sa_engine *engine, const sa_sprite *sprites,
                            uint32_t count)
{
    if (engine == nullptr || sprites == nullptr ||
        backend(*engine).picture == nullptr) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);
    uint32_t texture = 0;

    for (uint32_t i = 0; i < count; i++) {
        if (!texture_of(*engine, sprites[i].texture, texture)) {
            return SA_ERR_HANDLE;
        }
        if (!sprite_finite(sprites[i])) {
            return SA_ERR_INVALID;
        }
        sa_result result =
            check_view(b, sprites[i].view, sprites[0].view,
                       (sprites[i].flags & SA_SPRITE_SCREEN) == 0);

        if (result != SA_OK) {
            return result;
        }
    }
    if (!b.scene.room(count * 2)) {
        return SA_ERR_FULL;
    }
    if (count != 0) {
        take_home(b, sprites[0].view);
    }
    for (uint32_t i = 0; i < count; i++) {
        texture_of(*engine, sprites[i].texture, texture);
        b.scene.sprite(sprites[i], texture, b.views[sprites[i].view]);
        if (b.rate != 0) {
            b.world.sprite(sprites[i], texture);
        }
        b.draws++;
    }
    return SA_OK;
}

sa_result sa_present_triangles(sa_engine *engine, const sa_triangles *triangles,
                               const sa_point *points)
{
    if (engine == nullptr || triangles == nullptr || points == nullptr ||
        backend(*engine).picture == nullptr) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);
    uint32_t texture = 0;

    if (!texture_of(*engine, triangles->texture, texture)) {
        return SA_ERR_HANDLE;
    }
    /* the form sets how many points are read, so an unknown form reads none */
    if (triangles->form > SA_TRIANGLES_LINES ||
        !all_finite(triangles->object_to_world)) {
        return SA_ERR_INVALID;
    }
    uint32_t count = sa_triangles_points(triangles->form, triangles->count);

    for (uint32_t i = 0; i < count; i++) {
        if (!finite(points[i].at, POINT_FLOATS)) {
            return SA_ERR_INVALID;
        }
    }
    sa_result result =
        check_view(b, triangles->view, triangles->view,
                   (triangles->flags & SA_TRIANGLES_SCREEN) == 0);

    if (result != SA_OK) {
        return result;
    }
    if (!b.scene.room(triangles->form == SA_TRIANGLES_LINES
                          ? 2U * triangles->count
                          : triangles->count)) {
        return SA_ERR_FULL;
    }
    take_home(b, triangles->view);
    b.scene.triangles(*triangles, points, texture, b.views[triangles->view]);
    if (b.rate != 0) {
        b.world.triangles(*triangles, points, count, texture);
    }
    b.draws++;
    return SA_OK;
}

sa_result sa_present_end(sa_engine *engine)
{
    if (engine == nullptr || backend(*engine).picture == nullptr) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);
    uint32_t slot =
        static_cast<uint32_t>((b.opened - 1) % engine->desc.pictures);
    double owed = b.owed;

    if (b.scene.traces() && !b.parts_hidden) {
        hide_parts(*engine, b.scene);
        b.parts_hidden = true;
    }
    b.scene.finish();
    b.shown = 1;
    b.repeated = 0;
    b.full = 0;
    b.between_ns = 0;
    b.show_next = 0;
    b.show_slot = slot;
    b.show_first_ns = now_ns();
    if (b.rate != 0) {
        /* owed sums per_step in doubles, so a whole count of pictures can
         * come out just under it */
        const double rounding = 1e-6;

        b.world.end();
        b.full = b.world.full();
        b.owed += b.per_step * b.world.steps();
        double whole = floor(b.owed + rounding);

        b.shown = whole < 1            ? 1
                  : whole > MOST_SHOWN ? MOST_SHOWN
                                       : static_cast<uint32_t>(whole);
        /* the display never shows what the steps owe past MOST_SHOWN. so
         * owed keeps only the fraction past the whole pictures. where the
         * steps owe less than one, one picture shows and owed goes to 0 */
        b.owed = whole < 1 || b.owed < whole ? 0 : b.owed - whole;
    }
    /* the pictures between go first, each a part of the way from the picture
     * before to this one. a window shows the first now and the rest as their
     * times come. a headless engine draws them all now. where the device
     * refuses one, the picture stays open and whole for the call to be made
     * again */
    if (!show(*engine, b.show_next++) ||
        (b.window == nullptr && !show_due(*engine, UINT64_MAX))) {
        b.owed = owed;
        b.show_next = b.shown;
        return SA_ERR_UNSUPPORTED;
    }
    b.picture_used = b.picture->used();
    b.picture = nullptr;
    b.stats = sa_pc_stats{};
    b.stats.draws = b.draws;
    b.stats.triangles = b.scene.triangle_count();
    b.stats.batches = static_cast<uint32_t>(b.scene.batches().size());
    b.stats.arena = b.picture_used;
    b.stats.shades = b.shade_count;
    b.stats.shown = b.shown;
    if (const pc::RayPicture *rays = b.scene.ray_picture()) {
        b.stats.rays = rays->samples;
        b.stats.traced = static_cast<uint32_t>(rays->instances.size());
        for (const pc::RayInstance &instance : rays->instances) {
            b.stats.hidden += instance.flags & pc::RAY_SEEN ? 0 : 1;
        }
        b.stats.lamps = static_cast<uint32_t>(rays->lamps.size());
    }
    if (b.rate != 0) {
        const pc::BetweenCounts &c = b.world.counts();

        b.stats.moved = c.moved;
        b.stats.snapped = c.snapped;
        b.stats.born = c.born;
        b.stats.stayed = c.stayed;
        b.stats.ended = c.ended;
        b.stats.resting = b.world.resting();
        b.stats.outside = b.world.outside();
        b.stats.parts = b.world.parts();
        b.stats.lights = b.world.lights();
    }
    if (b.window != nullptr && pc::window_pump(b.window) && !b.quit_sent &&
        sa::event_reserve(*engine) == SA_OK) {
        b.quit_sent = true;
        post(*engine, SA_EVENT_QUIT, SA_NULL, 0, engine->tick);
    }
    collect(*engine);
    b.stats.cpu_ns = now_ns() - b.began_ns;
    return SA_OK;
}

sa_result sa_pc_read(sa_engine *engine, void *rgba, uint32_t bytes)
{
    if (engine == nullptr || rgba == nullptr ||
        bytes < SA_PC_WIDTH * SA_PC_HEIGHT * 4) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    if (b.opened == 0 || b.picture != nullptr) {
        return SA_ERR_INVALID;
    }
    if (!show_due(*engine, UINT64_MAX)) {
        return SA_ERR_UNSUPPORTED;
    }
    b.raster.wait(b.last_frame);
    collect(*engine);
    return b.raster.read(b.last_frame, rgba) ? SA_OK : SA_ERR_UNSUPPORTED;
}

sa_result sa_pc_read_shown(sa_engine *engine, uint32_t index, void *rgba,
                           uint32_t bytes)
{
    if (engine == nullptr) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    if (index + 1 == b.shown) {
        return sa_pc_read(engine, rgba, bytes);
    }
    if (rgba == nullptr || bytes < SA_PC_WIDTH * SA_PC_HEIGHT * 4 ||
        b.picture != nullptr || index >= b.shown ||
        b.shown - 1 - index > BETWEEN_ROOMS) {
        return SA_ERR_INVALID;
    }
    if (!show_due(*engine, UINT64_MAX)) {
        return SA_ERR_UNSUPPORTED;
    }
    uint32_t room = b.between_room[index];

    b.raster.wait(room);
    return b.raster.read(room, rgba) ? SA_OK : SA_ERR_UNSUPPORTED;
}

sa_result sa_pc_read_overview(sa_engine *engine, const sa_picture *picture,
                              void *rgba, uint32_t bytes, uint32_t *drawn)
{
    if (engine == nullptr || picture == nullptr || rgba == nullptr ||
        drawn == nullptr || bytes < SA_PC_WIDTH * SA_PC_HEIGHT * 4 ||
        backend(*engine).rate == 0 || backend(*engine).picture != nullptr ||
        !all_finite(picture->view)) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    if (!show_due(*engine, UINT64_MAX)) {
        return SA_ERR_UNSUPPORTED;
    }
    uint32_t room = engine->desc.pictures + OVERVIEW_ROOM;
    pc::PictureUniform made;
    std::string why;

    b.raster.wait(room);
    /* the overview is rasterized, whether or not the pictures are traced */
    b.between.trace(0);
    *drawn = b.world.overview(*picture, b.between);
    b.between.trace(b.rays);
    b.between.uniform(made);
    if (!b.raster.submit(room, made, b.between.clear_color(),
                         b.between.vertices(), b.between.batches(),
                         b.between.sorted(), nullptr, why)) {
        fprintf(stderr, "engine: %s\n", why.c_str());
        return SA_ERR_UNSUPPORTED;
    }
    b.raster.wait(room);
    return b.raster.read(room, rgba) ? SA_OK : SA_ERR_UNSUPPORTED;
}

sa_result sa_present_ended(sa_engine *engine, uint32_t instance)
{
    if (engine == nullptr) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    if (b.rate != 0) {
        b.world.ended(instance);
    }
    return SA_OK;
}

sa_result sa_scene_begin(sa_engine *engine, const sa_scene *scene)
{
    if (engine == nullptr || scene == nullptr || backend(*engine).scene_open ||
        backend(*engine).scene_stays) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    b.scene_open = true;
    b.scene_name = scene->scene;
    b.scene_shade_count = 0;
    b.scene_lit = false;
    return SA_OK;
}

sa_result sa_scene_lighting(sa_engine *engine, const sa_lighting *lighting)
{
    if (engine == nullptr || lighting == nullptr ||
        !backend(*engine).scene_open || backend(*engine).scene_lit ||
        lighting->count > SA_LIGHTING_SOURCES ||
        !finite(lighting->direction, LIGHTING_FLOATS) ||
        !finite(lighting->source,
                lighting->count * (sizeof(sa_light_source) / sizeof(float)))) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    b.scene_lit = true;
    if (b.rate != 0) {
        b.world.lighting(*lighting);
    }
    return SA_OK;
}

sa_result sa_scene_shade(sa_engine *engine, const sa_shade *shade)
{
    if (engine == nullptr || shade == nullptr || !backend(*engine).scene_open ||
        shade->mode == SA_SHADE_LIGHTS) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    if (b.scene_shade_count == MOST_SCENE_SHADES) {
        return SA_ERR_FULL;
    }
    b.scene_shades[b.scene_shade_count++] = *shade;
    return SA_OK;
}

sa_result sa_scene_part(sa_engine *engine, const sa_draw *parts, uint32_t count)
{
    if (engine == nullptr || parts == nullptr || !backend(*engine).scene_open) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);
    uint64_t bytes = 0;

    for (uint32_t i = 0; i < count; i++) {
        pc::MeshView mesh;
        const Resource *materials = nullptr;
        const sa_shade *shade = nullptr;
        sa_result result =
            check_part(*engine, parts[i], mesh, materials, shade);

        if (result != SA_OK) {
            return result;
        }
        bytes += pc::World::part_bytes(mesh, parts[i].flags);
    }
    /* of parts given together, none is taken where the render-owned scene
     * has no room for them all */
    if (b.rate != 0 && !b.world.room_for_parts(count, bytes)) {
        return SA_ERR_FULL;
    }
    if (b.traces) {
        for (uint32_t i = 0; i < count; i++) {
            const sa_shade *shade =
                shade_of(b.scene_shades, b.scene_shade_count, parts[i].shade);

            b.parts[parts[i].instance] = Part{parts[i], *shade};
        }
    }
    if (b.rate == 0) {
        return SA_OK;
    }
    for (uint32_t i = 0; i < count; i++) {
        pc::MeshView mesh;
        const Resource *materials = nullptr;
        const sa_shade *shade = nullptr;
        uint32_t textures[MOST_MATERIALS];

        /* the loop above passed this part, so the call can't fail */
        check_part(*engine, parts[i], mesh, materials, shade);
        for (uint32_t k = 0; k < materials->material_count; k++) {
            texture_of(*engine, materials->materials[k].texture, textures[k]);
        }
        b.world.part(parts[i], mesh, materials->materials, textures, *shade);
    }
    return SA_OK;
}

sa_result sa_scene_end(sa_engine *engine)
{
    if (engine == nullptr || !backend(*engine).scene_open) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    b.scene_open = false;
    b.scene_stays = true;
    return SA_OK;
}

sa_result sa_scene_ended(sa_engine *engine, uint32_t scene)
{
    if (engine == nullptr || !backend(*engine).scene_stays ||
        backend(*engine).scene_name != scene) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    b.scene_stays = false;
    b.parts.clear();
    b.lamps.forget();
    if (b.rate != 0) {
        b.world.scene_ended();
    }
    return SA_OK;
}

/* the picture is 16:9 where the game asks 16:9 of both it and its window,
 * else 4:3. a 4:3 picture under a 16:9 window is refused. where the aspect
 * changes, applied holds 60 at 30, as the Dreamcast's engine does */
sa_result sa_present_mode(sa_engine *engine, const sa_mode *asked,
                          sa_mode *applied)
{
    if (engine == nullptr || asked == nullptr || applied == nullptr ||
        asked->aspect > SA_ASPECT_16_9 ||
        (asked->rate != 30 && asked->rate != 60) || asked->held != 0 ||
        asked->reserved != 0) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    if (asked->aspect == SA_ASPECT_4_3 && b.window_asked == SA_ASPECT_16_9) {
        return SA_ERR_UNSUPPORTED;
    }
    bool wide =
        asked->aspect == SA_ASPECT_16_9 && b.window_asked == SA_ASPECT_16_9;

    *applied = *asked;
    applied->aspect = wide ? SA_ASPECT_16_9 : SA_ASPECT_4_3;
    applied->rate = wide == b.wide ? asked->rate : 30;
    b.wide = wide;
    return SA_OK;
}

sa_result sa_present_window(sa_engine *engine, uint32_t aspect)
{
    if (engine == nullptr || aspect > SA_ASPECT_16_9) {
        return SA_ERR_INVALID;
    }
    backend(*engine).window_asked = aspect;
    return SA_OK;
}

sa_result sa_pc_stats_read(sa_engine *engine, sa_pc_stats *stats)
{
    if (engine == nullptr || stats == nullptr) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    if (b.opened == 0 || b.picture != nullptr) {
        return SA_ERR_INVALID;
    }
    if (!show_due(*engine, UINT64_MAX)) {
        return SA_ERR_UNSUPPORTED;
    }
    std::string why;

    b.raster.wait(b.last_frame);
    collect(*engine);
    /* a device that draws no more gives no picture's figures */
    if (b.raster.broken(why)) {
        return SA_ERR_UNSUPPORTED;
    }
    *stats = b.stats;
    stats->between_ns = b.between_ns;
    stats->repeated = b.repeated;
    stats->full = b.full;
    stats->fragments_lost = b.raster.lost(b.last_frame);
    stats->fragments_cut = b.raster.cut(b.last_frame);
    stats->gpu_ns = b.raster.gpu_ns(b.last_frame);
    stats->events = engine->event_most;
    return SA_OK;
}

sa_result sa_pc_retire(sa_engine *engine)
{
    if (engine == nullptr || backend(*engine).picture != nullptr) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    if (!show_due(*engine, UINT64_MAX)) {
        return SA_ERR_UNSUPPORTED;
    }
    for (uint32_t i = 0; i < engine->desc.pictures; i++) {
        if (b.flights[i].serial > b.retired) {
            b.raster.wait(i);
        }
    }
    collect(*engine);
    /* a picture the device never took has no fence, and what waits on it
     * stays */
    if (b.retired != b.opened) {
        return SA_ERR_UNSUPPORTED;
    }
    if (b.rate != 0) {
        /* a release waits one picture more, as the pictures between read
         * the two pictures the render-owned scene keeps. once forget lets
         * those go, nothing reads the resource */
        b.world.forget();
        for (uint32_t i = 0; i < b.retiring_count; i++) {
            Retiring &r =
                b.retiring[(b.retiring_first + i) % engine->desc.events];

            r.serial = r.serial > b.opened ? b.opened : r.serial;
        }
        collect(*engine);
    }
    return SA_OK;
}

sa_result sa_pc_rays(sa_engine *engine, uint32_t rays)
{
    if (engine == nullptr || backend(*engine).picture != nullptr ||
        rays > SA_PC_PIXEL_RAYS) {
        return SA_ERR_INVALID;
    }
    Backend &b = backend(*engine);

    /* what the display has yet to show of the last picture shows as that
     * picture was made */
    if (!b.traces || !show_due(*engine, UINT64_MAX)) {
        return SA_ERR_UNSUPPORTED;
    }
    if (rays != 0) {
        b.rays_asked = rays;
    }
    /* the count already in force changes nothing, so the next traced
     * picture still pairs with the one before it */
    if (rays != b.rays) {
        take_rays(b, rays);
    }
    return SA_OK;
}

sa_result sa_pc_device(sa_engine *engine, char *name, uint32_t bytes)
{
    if (engine == nullptr || name == nullptr || bytes == 0) {
        return SA_ERR_INVALID;
    }
    std::string made = backend(*engine).raster.device_name();

    snprintf(name, bytes, "%s", made.c_str());
    return SA_OK;
}

sa_result sa_pc_broken(sa_engine *engine, char *why, uint32_t bytes)
{
    std::string reason;

    if (engine == nullptr || why == nullptr || bytes == 0) {
        return SA_ERR_INVALID;
    }
    if (!backend(*engine).raster.broken(reason)) {
        return SA_EMPTY;
    }
    snprintf(why, bytes, "%s", reason.c_str());
    return SA_OK;
}

} // extern "C"
