/* SA1's capture client, which sa_client.h declares. it makes each mesh from
 * its record's bytes, and each texture from its record's texels or the disc */
#ifndef ENGINE_SA1_CLIENT_H
#define ENGINE_SA1_CLIENT_H

#include <stdint.h>
#include <stdio.h>

#include <map>
#include <set>
#include <string>
#include <vector>

#include "disc.h"
#include "pvr.h"
#include "sa_capture.h"
#include "sa_client.h"
#include "sa_pc.h"

namespace pc {

struct Mesh {
    std::vector<uint8_t> bytes; /* its model's, as its record holds them */
    uint32_t drawn = 0; /* the picture that last drew it */
    sa_handle handle = SA_NULL;
    bool tried = false;
    bool ok = false;
};

struct TextureRecord {
    uint32_t global_index = 0;
    uint32_t file = 0; /* its file's name id, or 0 for none named */
    std::string name;
    std::vector<uint8_t> texels; /* a PVRT chunk, for one made in memory */
    sa_handle handle = SA_NULL;
    bool tried = false;
};

/* a list copies its materials at its record. a material the capture frees
 * later stays in the list */
struct MaterialList {
    std::vector<sa_cap_material> materials;
    sa_handle handle = SA_NULL;
};

/* the palettes the pictures shade from, as the last SA_CAP_PALETTES record
 * gave them. a version lasts until the next such record, so the client holds
 * one */
struct Palettes {
    sa_palettes record;
    uint32_t id = SA_CAP_NONE; /* the version's, or none before the first */

    /* the bytes held of the versions given */
    size_t bytes() const { return id != SA_CAP_NONE ? sizeof record : 0; }
};

/* what a capture names, by id, as its records give it */
struct Names {
    std::map<uint32_t, Mesh> meshes;
    std::map<uint32_t, TextureRecord> textures;
    std::map<uint32_t, sa_cap_material> materials;
    std::map<uint32_t, MaterialList> material_lists;
    std::map<uint32_t, sa_shade> shades;
    std::map<uint32_t, sa_lights> lights; /* by the shade they're for */
    Palettes palettes;
};

/* over the open picture */
struct Counts {
    uint32_t draws = 0;
    uint32_t sprites = 0;
    uint32_t lost = 0; /* draws the capture couldn't name */
    uint32_t unmade = 0; /* draws whose mesh wasn't made */
    uint32_t untextured = 0; /* textures named that did not load */
    uint32_t dead = 0; /* draws of an instance whose death came before */
    /* draws, sprites, triangles and views the runtime refused */
    uint32_t refused = 0;
    uint32_t views = 0; /* views given beside the picture's own */
};

/* over a run: the acts' scenes, and what became of their parts */
struct Scenes {
    uint32_t begun = 0;
    uint32_t ended = 0;
    uint32_t parts = 0; /* given, and taken */
    uint32_t lost = 0; /* parts the capture couldn't name or hold */
    uint32_t unmade = 0; /* parts whose mesh wasn't made */
    uint32_t refused = 0; /* calls of a scene's the runtime refused */
    uint32_t lit = 0; /* scenes whose lights were given, and taken */
    uint32_t sources = 0; /* the sources those lights held */
};

/* over a run: what went in through the runtime, and what came back */
struct Totals {
    uint32_t created = 0;
    uint32_t released = 0;
    uint32_t retired = 0; /* releases whose SA_EVENT_RETIRED came */
    uint32_t presented = 0;
    uint32_t stale_refused = 0; /* draws of a dead handle, refused */
    uint32_t stale_taken = 0; /* the same, taken: a fault */
    /* draws, sprites, triangles and views the capture named and the runtime
     * refused */
    uint32_t refused = 0;
    /* views_most is the most views a picture was given. views_split counts
     * the pictures given two or more */
    uint32_t views_most = 0;
    uint32_t views_split = 0;
    uint32_t held_most = 0; /* the most bytes lent at once */
    uint32_t lent_most = 0; /* the most resources lent at once */
    /* the most names of each kind held at once */
    uint32_t meshes_most = 0;
    uint32_t textures_most = 0;
    uint32_t materials_most = 0;
    uint32_t lists_most = 0;
    uint32_t shades_most = 0;
    /* the versions of the palettes given, of which one is held */
    uint32_t palettes_given = 0;
};

class Client {
public:
    Disc disc;
    Textures pvrs;
    sa_engine *engine = nullptr;
    Names names;
    std::set<uint32_t> alive;
    /* the newest task made: serials rise, so one at or below it not alive is
     * dead */
    uint32_t newest = 0;
    std::map<std::string, uint32_t> failures;
    /* each resource's bytes, lent until its SA_EVENT_RETIRED */
    std::map<sa_handle, std::vector<uint8_t>> lent;
    std::vector<sa_handle> stale; /* released, not yet tried by a draw */
    std::set<uint32_t> shades_given; /* in the open picture */
    std::set<uint32_t> scene_shades; /* given to the open scene */
    bool in_scene = false; /* between SA_CAP_SCENE and SA_CAP_SCENE_END */
    Scenes scenes;
    uint32_t births = 0;
    uint32_t deaths = 0;
    uint32_t held = 0;
    Totals totals;
    Counts counts;
    std::string textures_out; /* where each decoded texture goes, or none */
    sa_cap_picture picture{};
    bool in_picture = false;
    /* the device refused the last picture's end, and it is the engine's
     * open one still */
    bool unended = false;
    bool quit = false; /* the platform asked the game to end */
    uint32_t from = 0;
    uint32_t count = UINT32_MAX;

    sa_result record(const uint8_t *body, uint32_t size);
    sa_result end();
    sa_handle make(uint32_t kind, uint32_t format,
                   std::vector<uint8_t> &&bytes);
    void release(sa_handle &handle);
    void release_lists_of(uint32_t texture);
    void evict();
    void age_meshes();
    void events();
    bool mesh(uint32_t id, Mesh *&out);
    sa_handle texture(uint32_t id);
    sa_handle materials(uint32_t id);
    void shade(uint32_t id);
    void try_stale();
    void draw(const sa_draw &d);
    void part(const sa_draw &d);
    void sprite(const sa_sprite &s);
    void triangles(const sa_triangles &t, const uint8_t *points);
    void load(const sa_cap_life &life);
    void free_one(const sa_cap_life &life);
    void report();
};

} // namespace pc

#endif
