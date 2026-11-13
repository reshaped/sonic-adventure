/* a capture's records, given to the runtime one at a time. the client
 * holds each resource's bytes until its SA_EVENT_RETIRED */
#include "client.h"

#include <string.h>

#include <algorithm>

#include "basic.h"
#include "png.h"

namespace pc {

/* lend bytes to a new resource. it returns SA_NULL where the runtime
 * refuses it */
sa_handle Client::make(uint32_t kind, uint32_t format,
                       std::vector<uint8_t> &&bytes)
{
    sa_resource_desc desc{};
    sa_handle handle = SA_NULL;

    desc.kind = kind;
    desc.format = format;
    desc.bytes = static_cast<uint32_t>(bytes.size());
    desc.data = bytes.data();
    sa_result result = sa_resource_create(engine, &desc, &handle);
    if (result != SA_OK) {
        failures["sa_resource_create gave " + std::to_string(result)]++;
        return SA_NULL;
    }
    totals.created++;
    held += desc.bytes;
    totals.held_most = std::max(totals.held_most, held);
    lent[handle] = std::move(bytes);
    totals.lent_most =
        std::max(totals.lent_most, static_cast<uint32_t>(lent.size()));
    return handle;
}

void Client::release(sa_handle &handle)
{
    if (handle == SA_NULL) {
        return;
    }
    sa_result result = sa_resource_release(engine, handle);

    if (result != SA_OK) {
        failures["sa_resource_release gave " + std::to_string(result)]++;
    }
    totals.released++;
    stale.push_back(handle);
    handle = SA_NULL;
}

/* the runtime refuses a list that names a released texture, so each list
 * naming texture goes with it */
void Client::release_lists_of(uint32_t texture)
{
    for (auto &entry : names.material_lists) {
        MaterialList &list = entry.second;

        if (list.handle == SA_NULL) {
            continue;
        }
        for (const sa_cap_material &m : list.materials) {
            if (m.texture == texture) {
                release(list.handle);
                break;
            }
        }
    }
}

void Client::evict()
{
    for (auto &entry : names.material_lists) {
        release(entry.second.handle);
    }
    for (auto &entry : names.textures) {
        release(entry.second.handle);
        entry.second.tried = false;
    }
    for (auto &entry : names.meshes) {
        release(entry.second.handle);
        entry.second.tried = false;
    }
}

/* a mesh undrawn for over MESH_KEEP pictures is released. a later draw
 * makes it again from its bytes. the meshes lent stay bounded where a
 * capture frees none. end ages them every AGE_EVERY pictures */
constexpr uint32_t MESH_KEEP = 30;
constexpr uint32_t AGE_EVERY = 16;

void Client::age_meshes()
{
    for (auto &entry : names.meshes) {
        Mesh &m = entry.second;

        if (m.handle != SA_NULL && picture.picture - m.drawn > MESH_KEEP) {
            release(m.handle);
            m.tried = false;
        }
    }
}

/* take what the engine reports. a retired resource's bytes go back */
void Client::events()
{
    sa_event event;

    while (sa_event_poll(engine, &event) == SA_OK) {
        if (event.kind == SA_EVENT_RETIRED) {
            auto found = lent.find(event.handle);

            if (found == lent.end()) {
                failures["a retirement of nothing lent"]++;
                continue;
            }
            held -= static_cast<uint32_t>(found->second.size());
            lent.erase(found);
            totals.retired++;
        } else if (event.kind == SA_EVENT_PRESENTED) {
            totals.presented++;
        } else if (event.kind == SA_EVENT_QUIT) {
            quit = true;
        }
    }
}

bool Client::mesh(uint32_t id, Mesh *&out)
{
    auto found = names.meshes.find(id);

    if (found == names.meshes.end()) {
        return false;
    }
    Mesh &m = found->second;

    out = &m;
    if (m.tried) {
        return m.ok;
    }
    m.tried = true;
    if (m.bytes.empty()) {
        failures["a mesh whose record holds no model"]++;
        return false;
    }
    Model model;
    std::string why;
    m.ok = ninja_model(m.bytes, SA_CAP_INLINE_BASE, 0, model, why);
    if (!m.ok) {
        failures[why]++;
        return false;
    }
    std::vector<uint8_t> bytes;

    mesh_bytes(model, bytes);
    m.handle = make(SA_KIND_MESH, SA_FORMAT_MESH, std::move(bytes));
    m.ok = m.handle != SA_NULL;
    return m.ok;
}

sa_handle Client::texture(uint32_t id)
{
    auto found = names.textures.find(id);

    if (found == names.textures.end()) {
        return SA_NULL;
    }
    TextureRecord &t = found->second;
    if (!t.tried) {
        Image image;
        std::string why;

        t.tried = true;
        if (!t.texels.empty()
                ? pvr_decode(t.texels, 0, image, why)
                : pvrs.load(disc, t.global_index, t.name, t.file, image, why)) {
            std::vector<uint8_t> bytes(sizeof(sa_pixels) + image.rgba.size());
            sa_pixels size{image.width, image.height};

            memcpy(bytes.data(), &size, sizeof size);
            memcpy(bytes.data() + sizeof size, image.rgba.data(),
                   image.rgba.size());
            t.handle =
                make(SA_KIND_TEXTURE, SA_FORMAT_PIXELS, std::move(bytes));
            if (!textures_out.empty()) {
                png_write(textures_out + "/" + t.name + ".png", image.width,
                          image.height, image.rgba);
            }
        } else {
            failures[why]++;
        }
    }
    return t.handle;
}

sa_handle Client::materials(uint32_t id)
{
    auto found = names.material_lists.find(id);

    if (found == names.material_lists.end()) {
        return SA_NULL;
    }
    MaterialList &list = found->second;
    if (list.handle != SA_NULL) {
        return list.handle;
    }
    std::vector<uint8_t> bytes(list.materials.size() * sizeof(sa_material));

    for (size_t i = 0; i < list.materials.size(); i++) {
        const sa_cap_material &c = list.materials[i];
        sa_material m{};

        m.texture = c.flags & SA_MAT_TEXTURE ? texture(c.texture) : SA_NULL;
        if ((c.flags & SA_MAT_TEXTURE) && m.texture == SA_NULL) {
            counts.untextured++;
        }
        m.specular = c.specular;
        m.flags = c.flags;
        m.alpha = c.alpha;
        m.filter = c.filter;
        m.src = c.src;
        m.dst = c.dst;
        m.mip_bias = c.mip_bias;
        m.exponent = c.exponent;
        m.diffuse = c.diffuse;
        memcpy(bytes.data() + i * sizeof m, &m, sizeof m);
    }
    list.handle = make(SA_KIND_MATERIAL, SA_FORMAT_MATERIALS, std::move(bytes));
    return list.handle;
}

void Client::shade(uint32_t id)
{
    if (shades_given.insert(id).second) {
        auto found = names.shades.find(id);

        if (found == names.shades.end()) {
            failures["a draw whose shade has no record"]++;
            return;
        }
        if (found->second.mode == SA_SHADE_LIGHTS) {
            auto lights = names.lights.find(id);

            if (lights == names.lights.end()) {
                failures["a shade of lights with no lights"]++;
                return;
            }
            if (sa_present_lights(engine, &lights->second) != SA_OK) {
                failures["lights the runtime refused"]++;
                return;
            }
        }
        sa_present_shade(engine, &found->second);
    }
}

/* each handle released since the last picture goes in a draw or sprite
 * the runtime must refuse. the draw's other names are ones the runtime
 * takes, so only the dead handle can be refused. its shade is SA_CAP_NONE,
 * which no record names. a list's draw takes an empty mesh from the
 * picture's arena. a mesh's draw takes a live list, and goes untried where
 * the client holds none. a list goes untried where a texture was released
 * too. the runtime refuses a list that names a dead texture, whatever
 * became of the list */
void Client::try_stale()
{
    sa_mesh empty{};
    sa_shade shade{};
    uint32_t payload = SA_NO_PAYLOAD;
    sa_handle list = SA_NULL;
    bool texture_went = false;

    if (stale.empty()) {
        return;
    }
    for (sa_handle handle : stale) {
        texture_went =
            texture_went || SA_HANDLE_KIND(handle) == SA_KIND_TEXTURE;
    }
    sa_present_copy(engine, &empty, sizeof empty, &payload);
    shade.head = {SA_REC_SHADE, sizeof shade};
    shade.shade = SA_CAP_NONE;
    shade.mode = SA_SHADE_RAMP;
    sa_present_shade(engine, &shade);
    for (const auto &entry : names.material_lists) {
        if (entry.second.handle != SA_NULL) {
            list = entry.second.handle;
            break;
        }
    }
    for (sa_handle handle : stale) {
        sa_draw d{};
        sa_sprite s{};
        sa_result result;

        d.head = {SA_REC_DRAW, sizeof d};
        d.shade = shade.shade;
        if (SA_HANDLE_KIND(handle) == SA_KIND_MESH) {
            if (list == SA_NULL) {
                continue;
            }
            d.mesh = handle;
            d.materials = list;
            d.payload = SA_NO_PAYLOAD;
            result = sa_present_draw(engine, &d, 1);
        } else if (SA_HANDLE_KIND(handle) == SA_KIND_MATERIAL) {
            if (texture_went) {
                continue;
            }
            d.materials = handle;
            d.payload = payload;
            result = sa_present_draw(engine, &d, 1);
        } else {
            s.head = {SA_REC_SPRITE, sizeof s};
            s.texture = handle;
            result = sa_present_sprite(engine, &s, 1);
        }
        if (result == SA_ERR_HANDLE) {
            totals.stale_refused++;
        } else {
            totals.stale_taken++;
        }
    }
    stale.clear();
}

/* give d as a part of the open scene, its shade first where the scene
 * hasn't had it. age_meshes counts its mesh as drawn by the latest picture */
void Client::part(const sa_draw &d)
{
    Mesh *m = nullptr;

    if (!mesh(d.mesh, m)) {
        scenes.unmade++;
        return;
    }
    m->drawn = picture.picture;
    sa_draw out = d;

    out.mesh = m->handle;
    out.flags |= SA_DRAW_WRITTEN;
    out.materials = materials(d.materials);
    auto shade = names.shades.find(d.shade);

    if (shade != names.shades.end() && scene_shades.insert(d.shade).second &&
        sa_scene_shade(engine, &shade->second) != SA_OK) {
        scenes.refused++;
    }
    if (sa_scene_part(engine, &out, 1) == SA_OK) {
        scenes.parts++;
    } else {
        scenes.refused++;
    }
}

void Client::draw(const sa_draw &d)
{
    Mesh *m = nullptr;

    counts.draws++;
    /* only a task's serial, with neither bit set, can name a dead task */
    if ((d.instance & (SA_INSTANCE_UNOWNED | SA_INSTANCE_SCENE)) == 0 &&
        d.instance <= newest && alive.count(d.instance) == 0) {
        counts.dead++;
    }
    if (!mesh(d.mesh, m)) {
        counts.unmade++;
        return;
    }
    m->drawn = picture.picture;
    sa_draw out = d;

    out.mesh = m->handle;
    out.flags |= SA_DRAW_WRITTEN;
    out.materials = materials(d.materials);
    out.payload = SA_NO_PAYLOAD;
    shade(d.shade);
    if (sa_present_draw(engine, &out, 1) != SA_OK) {
        counts.refused++;
    }
}

void Client::sprite(const sa_sprite &s)
{
    sa_sprite out = s;

    counts.sprites++;
    out.head.kind = SA_REC_SPRITE;
    out.texture = texture(s.texture);
    if (out.texture == SA_NULL) {
        counts.untextured++;
    }
    if (sa_present_sprite(engine, &out, 1) != SA_OK) {
        counts.refused++;
    }
}

void Client::triangles(const sa_triangles &t, const uint8_t *points)
{
    sa_triangles out = t;
    std::vector<sa_point> copy(sa_triangles_points(t.form, t.count));

    memcpy(copy.data(), points, copy.size() * sizeof(sa_point));
    out.head.kind = SA_REC_TRIANGLES;
    out.texture = t.flags & SA_TRIANGLES_TEXTURE ? texture(t.texture) : SA_NULL;
    if ((t.flags & SA_TRIANGLES_TEXTURE) && out.texture == SA_NULL) {
        counts.untextured++;
    }
    if (sa_present_triangles(engine, &out, copy.data()) != SA_OK) {
        counts.refused++;
    }
}

void Client::load(const sa_cap_life &life)
{
    if (life.kind == SA_KIND_TEXTURE) {
        texture(life.id);
    } else {
        Mesh *m = nullptr;

        mesh(life.id, m);
    }
}

/* no later record names what a free names, so its name goes with it */
void Client::free_one(const sa_cap_life &life)
{
    if (life.kind == SA_KIND_TEXTURE) {
        auto found = names.textures.find(life.id);

        if (found != names.textures.end()) {
            release_lists_of(life.id);
            release(found->second.handle);
            names.textures.erase(found);
        }
    } else if (life.kind == SA_KIND_MATERIAL) {
        auto found = names.material_lists.find(life.id);

        if (found != names.material_lists.end()) {
            release(found->second.handle);
            names.material_lists.erase(found);
        }
    } else if (life.kind == SA_CAP_MATERIAL) {
        names.materials.erase(life.id);
    } else if (life.kind == SA_REC_SHADE) {
        names.shades.erase(life.id);
        names.lights.erase(life.id);
    } else {
        auto found = names.meshes.find(life.id);

        if (found != names.meshes.end()) {
            release(found->second.handle);
            names.meshes.erase(found);
        }
    }
}

static void most(uint32_t &most, size_t now)
{
    most = std::max(most, static_cast<uint32_t>(now));
}

/* end the engine's open picture. a picture the device refuses stays open
 * and whole, so its end is tried once more. one refused twice stays open,
 * for the next SA_CAP_PICTURE to end first */
sa_result Client::end()
{
    sa_result ended = sa_present_end(engine);

    if (ended == SA_ERR_UNSUPPORTED) {
        ended = sa_present_end(engine);
    }
    unended = ended == SA_ERR_UNSUPPORTED;
    if (ended != SA_OK) {
        return ended;
    }
    totals.refused += counts.refused;
    totals.views_most = std::max(totals.views_most, counts.views);
    totals.views_split += counts.views > 1 ? 1 : 0;
    if (picture.picture % AGE_EVERY == 0) {
        age_meshes();
    }
    events();
    return SA_OK;
}

/* take one record of a capture, with sa_pc_client_record's results */
sa_result Client::record(const uint8_t *body, uint32_t size)
{
    sa_record_head head;

    if (size < sizeof head) {
        return SA_ERR_INVALID;
    }
    memcpy(&head, body, sizeof head);
    if (head.size < sizeof head || head.size > size) {
        return SA_ERR_INVALID;
    }
    /* the fewest bytes each record holds, which a newer capture may follow
     * with more. a picture's record may stop before clear_right, as a
     * draw's may before view */
    static const std::map<uint16_t, uint32_t> sizes = {
        {SA_CAP_MESH, sizeof(sa_cap_mesh)},
        {SA_CAP_TEXTURE, sizeof(sa_cap_texture)},
        {SA_CAP_TEXELS, sizeof(sa_cap_texels)},
        {SA_CAP_MATERIAL, sizeof(sa_cap_material)},
        {SA_CAP_MATERIALS, sizeof(sa_cap_materials)},
        {SA_CAP_PALETTES, sizeof(sa_cap_palettes)},
        {SA_CAP_PICTURE, offsetof(sa_cap_picture, clear_right)},
        {SA_REC_SHADE, sizeof(sa_shade)},
        {SA_REC_LIGHTS, sizeof(sa_lights)},
        {SA_REC_DRAW, offsetof(sa_draw, view)},
        {SA_REC_VIEW, sizeof(sa_picture_view)},
        {SA_CAP_SPRITE, sizeof(sa_sprite)},
        {SA_CAP_TRIANGLES, sizeof(sa_triangles)},
        {SA_CAP_LOAD, sizeof(sa_cap_life)},
        {SA_CAP_FREE, sizeof(sa_cap_life)},
        {SA_CAP_END, sizeof(sa_cap_end)},
        {SA_CAP_SCENE, sizeof(sa_scene)},
        {SA_CAP_SCENE_END, sizeof(sa_cap_scene_end)},
        {SA_CAP_SCENE_ENDED, sizeof(sa_scene)},
        {SA_CAP_SCENE_LIGHTING, sizeof(sa_lighting)},
        {SA_REC_TASK_CREATE, sizeof(sa_task_create)},
        {SA_REC_TASK_DESTROY, sizeof(sa_task_destroy)}};
    auto need = sizes.find(head.kind);

    if (need != sizes.end() && head.size < need->second) {
        return SA_ERR_INVALID;
    }
    switch (head.kind) {
    case SA_CAP_MESH: {
        sa_cap_mesh m;
        memcpy(&m, body, sizeof m);
        Mesh &mesh = names.meshes[m.mesh];
        if (m.format == SA_MESH_NINJA_INLINE) {
            mesh.bytes.assign(body + sizeof m, body + head.size);
        }
        most(totals.meshes_most, names.meshes.size());
        break;
    }
    case SA_CAP_TEXTURE: {
        sa_cap_texture t;
        memcpy(&t, body, sizeof t);
        TextureRecord &texture = names.textures[t.texture];
        texture.global_index = t.global_index;
        texture.file = t.file;
        texture.name.assign(t.name, strnlen(t.name, sizeof t.name));
        texture.texels.clear();
        most(totals.textures_most, names.textures.size());
        break;
    }
    case SA_CAP_TEXELS: {
        sa_cap_texels t;
        memcpy(&t, body, sizeof t);
        std::vector<uint8_t> &texels = names.textures[t.texture].texels;
        texels.insert(texels.end(), body + sizeof t, body + head.size);
        break;
    }
    case SA_CAP_MATERIAL: {
        sa_cap_material m;
        memcpy(&m, body, sizeof m);
        names.materials[m.material] = m;
        most(totals.materials_most, names.materials.size());
        break;
    }
    case SA_CAP_MATERIALS: {
        sa_cap_materials m;
        memcpy(&m, body, sizeof m);
        if (head.size < sizeof m + m.count * sizeof(uint32_t)) {
            return SA_ERR_INVALID;
        }
        std::vector<sa_cap_material> &made =
            names.material_lists[m.materials].materials;
        made.assign(m.count, sa_cap_material{});
        for (uint32_t i = 0; i < m.count; i++) {
            uint32_t id;
            memcpy(&id, body + sizeof m + (i * sizeof id), sizeof id);
            auto material = names.materials.find(id);

            if (material != names.materials.end()) {
                made[i] = material->second;
            }
        }
        most(totals.lists_most, names.material_lists.size());
        break;
    }
    case SA_REC_SHADE: {
        sa_shade s;
        memcpy(&s, body, sizeof s);
        names.shades[s.shade] = s;
        most(totals.shades_most, names.shades.size());
        break;
    }
    case SA_REC_LIGHTS: {
        sa_lights l;
        memcpy(&l, body, sizeof l);
        l.head = {SA_REC_LIGHTS, sizeof l};
        names.lights[l.shade] = l;
        break;
    }
    case SA_CAP_PALETTES: {
        /* the version before ends here, and no later picture names it */
        Palettes &p = names.palettes;
        const size_t at = offsetof(sa_cap_palettes, words);
        memcpy(&p.id, body + offsetof(sa_cap_palettes, palettes), sizeof p.id);
        p.record.head = {SA_REC_PALETTES, sizeof p.record};
        memcpy(p.record.words, body + at,
               std::min<size_t>(sizeof p.record.words, head.size - at));
        totals.palettes_given++;
        break;
    }
    case SA_CAP_LOAD: {
        sa_cap_life life;
        memcpy(&life, body, sizeof life);
        load(life);
        break;
    }
    case SA_CAP_FREE: {
        sa_cap_life life;
        memcpy(&life, body, sizeof life);
        free_one(life);
        break;
    }
    case SA_REC_TASK_CREATE: {
        sa_task_create t;
        memcpy(&t, body, sizeof t);
        alive.insert(t.task);
        newest = std::max(newest, t.task);
        births++;
        break;
    }
    case SA_REC_TASK_DESTROY: {
        sa_task_destroy t;
        memcpy(&t, body, sizeof t);
        alive.erase(t.task);
        deaths++;
        sa_present_ended(engine, t.task);
        break;
    }
    case SA_CAP_PICTURE: {
        /* a picture the device refused ends before another begins. while
         * the device refuses it, the runtime gets none of the pictures
         * after it */
        if (unended) {
            sa_result ended = end();

            if (ended != SA_OK) {
                return ended;
            }
        }
        picture = sa_cap_picture{};
        memcpy(&picture, body, std::min<size_t>(sizeof picture, head.size));
        in_picture = picture.picture >= from && picture.picture - from < count;
        if (!in_picture) {
            break;
        }
        sa_picture p{};
        sa_fog fog{};
        /* without its other corners, the background is clear alone */
        bool corners = picture.clear_right != 0 || picture.clear_below != 0;

        p.head = {SA_REC_PICTURE, sizeof p};
        p.tick = picture.tick;
        p.flags = static_cast<uint16_t>(picture.flags & SA_PICTURE_CUT);
        p.view = picture.view;
        p.back[0] = picture.clear;
        p.back[1] = corners ? picture.clear_right : picture.clear;
        p.back[2] = corners ? picture.clear_below : picture.clear;
        fog.head = {SA_REC_FOG, sizeof fog};
        fog.color = picture.fog;
        fog.density = picture.fog_density;
        memcpy(fog.table, picture.fog_table, sizeof fog.table);
        counts = Counts{};
        shades_given.clear();
        sa_result begun = sa_present_begin(engine, &p);
        if (begun != SA_OK) {
            return begun;
        }
        sa_present_fog(engine, &fog);
        if (names.palettes.bytes() == 0 ||
            names.palettes.id != picture.palettes ||
            sa_present_palettes(engine, &names.palettes.record) != SA_OK) {
            failures["a picture whose palettes have no record"]++;
        }
        try_stale();
        break;
    }
    case SA_CAP_SCENE: {
        sa_scene s;
        memcpy(&s, body, sizeof s);
        s.head = {SA_REC_SCENE, sizeof s};
        in_scene = true;
        scene_shades.clear();
        scenes.begun++;
        if (sa_scene_begin(engine, &s) != SA_OK) {
            scenes.refused++;
        }
        break;
    }
    case SA_CAP_SCENE_LIGHTING: {
        sa_lighting l;
        memcpy(&l, body, sizeof l);
        l.head = {SA_REC_LIGHTING, sizeof l};
        if (sa_scene_lighting(engine, &l) == SA_OK) {
            scenes.lit++;
            scenes.sources += l.count;
        } else {
            scenes.refused++;
        }
        break;
    }
    case SA_CAP_SCENE_END: {
        sa_cap_scene_end e;
        memcpy(&e, body, sizeof e);
        in_scene = false;
        scenes.lost += e.lost;
        if (sa_scene_end(engine) != SA_OK) {
            scenes.refused++;
        }
        break;
    }
    case SA_CAP_SCENE_ENDED: {
        sa_scene s;
        memcpy(&s, body, sizeof s);
        scenes.ended++;
        if (sa_scene_ended(engine, s.scene) != SA_OK) {
            scenes.refused++;
        }
        break;
    }
    case SA_REC_VIEW:
        if (in_picture) {
            sa_picture_view v;
            memcpy(&v, body, sizeof v);
            v.head = {SA_REC_VIEW, sizeof v};
            counts.views++;
            if (sa_present_view(engine, &v) != SA_OK) {
                counts.refused++;
            }
        }
        break;
    case SA_REC_DRAW:
        /* a draw inside a scene is one of its parts, in a picture or between
         * two. a draw that ends before view takes 0, the picture's own */
        if (in_scene || in_picture) {
            sa_draw d{};
            memcpy(&d, body, std::min<size_t>(sizeof d, head.size));
            if (in_scene) {
                part(d);
            } else {
                draw(d);
            }
        }
        break;
    case SA_CAP_SPRITE:
        if (in_picture) {
            sa_sprite s;
            memcpy(&s, body, sizeof s);
            sprite(s);
        }
        break;
    case SA_CAP_TRIANGLES:
        if (in_picture) {
            sa_triangles t;
            memcpy(&t, body, sizeof t);
            if (head.size < sizeof t + sa_triangles_points(t.form, t.count) *
                                           sizeof(sa_point)) {
                return SA_ERR_INVALID;
            }
            triangles(t, body + sizeof t);
        }
        break;
    case SA_CAP_END: {
        sa_cap_end e;
        memcpy(&e, body, sizeof e);
        if (!in_picture) {
            break;
        }
        in_picture = false;
        counts.lost = e.lost;
        sa_result ended = end();
        if (ended != SA_OK) {
            return ended;
        }
        return quit ? SA_PC_QUIT : SA_PC_ENDED;
    }
    default:
        break;
    }
    return SA_OK;
}

void Client::report()
{
    for (const auto &f : failures) {
        printf("failed %u: %s\n", f.second, f.first.c_str());
    }
    printf("resources made %u, released %u, retired %u, %zu lent, at most %u "
           "at once and %u KB; %u pictures shown; %u draws of a dead handle "
           "refused, %u taken; %u draws, sprites, triangles and views "
           "refused\n",
           totals.created, totals.released, totals.retired, lent.size(),
           totals.lent_most, totals.held_most / 1024, totals.presented,
           totals.stale_refused, totals.stale_taken, totals.refused);
    if (scenes.begun != 0) {
        printf("scenes begun %u, ended %u: %u parts given, %u the capture "
               "lost, %u unmade, %u calls refused; lights given to %u, of "
               "%u sources\n",
               scenes.begun, scenes.ended, scenes.parts, scenes.lost,
               scenes.unmade, scenes.refused, scenes.lit, scenes.sources);
    }
    printf("views given: at most %u to a picture, and two or more to %u "
           "pictures\n",
           totals.views_most, totals.views_split);
    printf("names held: %zu meshes, %zu textures, %zu materials, %zu lists "
           "and %zu shades; at most %u, %u, %u, %u and %u\n",
           names.meshes.size(), names.textures.size(), names.materials.size(),
           names.material_lists.size(), names.shades.size(), totals.meshes_most,
           totals.textures_most, totals.materials_most, totals.lists_most,
           totals.shades_most);
    printf("palettes held: %zu bytes, the last of %u versions given\n",
           names.palettes.bytes(), totals.palettes_given);
}

} // namespace pc

extern "C" {

sa_pc_client *sa_pc_client_create(sa_engine *engine, const char *const *roots)
{
    auto *made = new pc::Client;

    made->engine = engine;
    for (; roots != nullptr && *roots != nullptr; roots++) {
        made->disc.add_root(*roots);
    }
    made->pvrs.index(made->disc, "SONICADV");
    return reinterpret_cast<sa_pc_client *>(made);
}

sa_result sa_pc_client_record(sa_pc_client *client, const void *record)
{
    sa_record_head head;

    if (client == nullptr || record == nullptr) {
        return SA_ERR_INVALID;
    }
    memcpy(&head, record, sizeof head);
    return reinterpret_cast<pc::Client *>(client)->record(
        static_cast<const uint8_t *>(record), head.size);
}

void sa_pc_client_report(sa_pc_client *client)
{
    reinterpret_cast<pc::Client *>(client)->report();
}

sa_result sa_pc_client_destroy(sa_pc_client *client)
{
    if (client == nullptr) {
        return SA_ERR_INVALID;
    }
    auto *c = reinterpret_cast<pc::Client *>(client);
    /* sa_pc_retire refuses with a picture open, so it runs before evict */
    sa_result result = sa_pc_retire(c->engine);

    if (result != SA_OK) {
        return result;
    }
    /* sa_pc_retire queues the SA_EVENT_RETIRED of each release evict makes */
    c->evict();
    result = sa_pc_retire(c->engine);
    c->events();
    /* a byte whose retirement never came is the engine's to read yet */
    if (result == SA_OK && !c->lent.empty()) {
        result = SA_ERR_INVALID;
    }
    if (result == SA_OK) {
        delete c;
    }
    return result;
}

} // extern "C"
