/* the render-owned scene keeps the instances the pictures' draws describe
 * from one picture to the next. it also keeps the parts the game gives of an
 * act's scene. ENGINE.md#the-render-owned-scene gives its rules. a mesh's
 * draw that names a thing keys its instance by its drawer, the thing and its
 * nth. a later picture finds the instance again by that name. a scene's part
 * is kept under its instance alone. Key names every other draw by its place
 * in the picture. the two pictures last ended keep each draw as an
 * sa_draw_kept, with its words in the picture's store. a picture between
 * them is drawn from those alone. a picture is kept whole or not at all. one
 * that fills a store keeps none, and no picture is drawn between it and
 * another */
#ifndef ENGINE_PC_WORLD_H
#define ENGINE_PC_WORLD_H

#include <stdint.h>

#include <vector>

#include "sa_engine.h"
#include "scene.h"

namespace pc {

/* a kept draw's kind: a mesh, a sprite or triangles */
enum : uint8_t { KEPT_MESH = 0, KEPT_SPRITE = 1, KEPT_TRIANGLES = 2 };

/* an instance's name across pictures. instance is a task's serial,
 * SA_INSTANCE_UNOWNED for any draw no task owns, or a scene's part's own.
 * where a mesh's draw names a thing, asset is the thing and part is its nth.
 * named is then 1. a scene's part takes no thing from its draw. any other
 * draw's part is its place among the picture's unnamed draws of its
 * instance, kind and asset. asset is then a mesh's name, or 0 for triangles
 * and a scene's part. a sprite's asset is 1 on the screen and 0 off it. its
 * texture can't name it, as the texture steps through its frames. named
 * stops a name the scene worked out from matching one a draw gave */
struct Key {
    uint32_t instance = 0;
    uint32_t asset = 0;
    uint32_t part = 0;
    uint8_t kind = 0;
    uint8_t named = 0;
};

/* a mesh as a draw took it. mesh views a resource's bytes as lent, or a
 * payload's in its picture's arena. name is the resource's handle. a mesh a
 * writer made is named by its shape, which posing leaves alone. so each pose
 * of one model is the same mesh from picture to picture */
struct KeptMesh {
    MeshView mesh;
    const sa_material *materials;
    uint32_t textures; /* where its raster textures start in Store::textures */
    uint32_t name;
};
/* the words of a mesh's draw record that the snapshot keeps nowhere else */
struct KeptDraw {
    sa_handle mesh;
    sa_handle materials;
    uint32_t payload;
    uint32_t tint;
    uint16_t generation;
    uint16_t reserved; /* 0 */
};
/* a mesh's draw's shade. lights are an SA_SHADE_LIGHTS shade's, in the
 * picture's arena, which lives while the picture may be read. palettes are
 * those the picture took last before the draw, in the same arena, or null */
struct KeptShade {
    const sa_lights *lights;
    const uint32_t *palettes;
    sa_shade shade;
    uint32_t reserved; /* 0 */
};
/* the same as KeptDraw for a sprite */
struct KeptSprite {
    float at[3];
    float scale[2];
    float corner[2][2];
    float uv[2][2];
    float angle;
    float bias;
    uint32_t color;
    uint32_t raster; /* the raster's texture */
    uint8_t depth;
    uint8_t fog;
    uint8_t src;
    uint8_t dst;
    uint8_t filter;
    uint8_t reserved[3]; /* 0 */
};
/* the same as KeptDraw for triangles */
struct KeptTriangles {
    sa_handle texture;
    uint32_t raster; /* the raster's texture */
    uint16_t count;
    uint8_t form;
    uint8_t depth;
    uint8_t fog;
    uint8_t src;
    uint8_t dst;
    uint8_t filter;
};
/* a picture's store: the tables a kept draw's fields name. each table but
 * textures and points gains an entry only where the new one differs from its
 * last. an entry holds no padding, so its bytes are its words. the PC's draws
 * come in world space, so a kept draw's matrix is object to world. its
 * geometry is a mesh's entry, a sprite's texture's handle or triangles' first
 * point */
struct Store {
    std::vector<KeptDraw> draws; /* a mesh's state */
    std::vector<KeptSprite> sprites; /* a sprite's */
    std::vector<KeptTriangles> triangles; /* triangles' */
    std::vector<KeptShade> shades;
    std::vector<KeptMesh> meshes;
    std::vector<float> matrices; /* 12 an entry */
    std::vector<uint32_t> textures;
    std::vector<sa_point> points;
};

/* a resource released while a picture was open. name is its handle, or the
 * raster's texture it held. only the picture's draws before the release name
 * it. a later draw under the name names another resource */
struct Release {
    uint32_t name;
    uint32_t before; /* the picture's draws when it went */
};

/* one picture, kept while a picture between it and the next may read it.
 * its meshes and payloads stay lent that long */
struct Snapshot {
    bool valid = false; /* ended, so its tick stands */
    /* SA_PC_FULL_* for the store it filled, or 0. from then it takes no
     * draw, and once ended it holds none */
    uint8_t full = 0;
    uint64_t serial = 0;
    sa_picture picture{};
    sa_view views[SA_VIEWS + 1] = {}; /* by the id a draw names, 0 its own */
    sa_fog fog{};
    /* the palettes it took last, or null. each draw keeps those that stood
     * at it in its KeptShade. the overview shades from these */
    const uint32_t *palettes = nullptr;
    std::vector<sa_draw_kept> kept;
    std::vector<uint32_t> instances; /* the one each draw's record gave */
    Store store;
    std::vector<Key> keys;
    std::vector<uint32_t> groups; /* how many draws share each key but part */
    std::vector<uint32_t> by_key; /* kept's indices, in key order */
    std::vector<uint32_t> ended; /* tasks that ended while it was open */
    /* the resources released while it was open, and the raster's textures
     * they held. they lie in their names' order once it ends. the picture
     * reads them until it retires, and no instance it leaves keeps one */
    std::vector<Release> released;
    std::vector<Release> released_textures;
    /* how many draws it took before the act's scene ended during it, or 0 */
    uint32_t scene_gone = 0;
};

/* an instance as the scene keeps it, drawn in the last picture or not. it
 * goes when its task ends or its mesh or materials are released. a
 * resource's bytes stay lent, and a writer's mesh is the scene's own copy */
enum : uint8_t {
    RESIDENT_SEEN = 0x1, /* the last picture took a draw of it */
    RESIDENT_SHAPE = 0x2, /* a mesh it can draw again, out of view too */
    RESIDENT_BROKE = 0x4, /* its last move was a discontinuity */
    RESIDENT_OUT = 0x8, /* its last draw was out of the game's view */
    RESIDENT_PART = 0x10 /* a part the scene was given, until the scene ends */
};
struct Resident {
    Key key;
    sa_handle mesh = SA_NULL; /* SA_NULL for a writer's */
    sa_handle materials = SA_NULL;
    MeshView lent; /* a resource's bytes, under a handle */
    uint32_t bytes = 0; /* where a writer's mesh starts in store_ */
    uint32_t size = 0; /* 0 for a resource's */
    uint32_t textures = 0; /* where its raster textures start in store_ */
    uint32_t material_count = 0;
    const sa_material *material_list = nullptr;
    sa_shade shade{};
    float now[12] = {}; /* object to world, as last drawn */
    float before[12] = {}; /* as drawn in the picture before that */
    uint32_t asset_version = 0; /* bumps as its mesh or materials change */
    uint32_t shape_version = 0; /* bumps as a writer's mesh is drawn again */
    uint16_t generation = 0;
    uint8_t flags = 0;
    uint64_t seen = 0; /* the serial of the last picture that drew it */
};

/* what the last picture between two took from them */
struct BetweenCounts {
    uint32_t moved = 0; /* drawn at a transform between the two */
    uint32_t snapped = 0; /* at the nearer one's, paired with no draw */
    uint32_t born = 0; /* left out to the half: the newer drew it first */
    uint32_t stayed = 0; /* to the half, the older's alone, at its place */
    uint32_t ended = 0; /* left out: its task ended in the older picture */
    uint32_t cut = 0; /* 1 where the picture is the newer one whole */
};

class World {
public:
    /* releases is the most resources that go while one picture is open */
    void open(uint32_t releases);
    /* a picture opens, with its arena's serial */
    void begin(const sa_picture &picture, uint64_t serial);
    /* a further view of the open picture's, by the id its draws name */
    void view(uint32_t id, const sa_view &view) { now_.views[id] = view; }
    void fog(const sa_fog &fog) { now_.fog = fog; }
    /* the palettes the open picture's draws after the call shade from.
     * words lie in the picture's arena */
    void palettes(const uint32_t *words) { now_.palettes = words; }
    /* textures[i] is materials[i]'s raster texture, mesh.materials of them */
    void draw(const sa_draw &d, const MeshView &mesh,
              const sa_material *materials, const uint32_t *textures,
              const sa_shade &shade, const sa_lights *lights = nullptr);
    void sprite(const sa_sprite &s, uint32_t texture);
    void triangles(const sa_triangles &t, const sa_point *points,
                   uint32_t count, uint32_t texture);
    /* a task ended, in the open picture or between two */
    void ended(uint32_t instance);
    /* whether the scene has room for count parts more, which take bytes of
     * its store */
    bool room_for_parts(uint32_t count, uint64_t bytes) const;
    /* the bytes of the store a part of mesh takes under a draw's flags. they
     * hold its raster textures, and a writer's mesh too */
    static uint64_t part_bytes(const MeshView &mesh, uint32_t flags);
    /* a part the game gives of the act's scene, which room_for_parts found
     * room for. undrawn, it stays only while it holds RESIDENT_SHAPE.
     * scene_ended or a release of its mesh or materials removes it. it
     * stands where it was given until a picture's draw of it places it. one
     * given again under the same instance takes the first's place */
    void part(const sa_draw &d, const MeshView &mesh,
              const sa_material *materials, const uint32_t *textures,
              const sa_shade &shade);
    /* the act's scene's lights, as sa_scene_lighting gives them: kept until
     * scene_ended. no picture is drawn by them */
    void lighting(const sa_lighting &l);
    /* the act's scene ended, in the open picture or between two. its parts
     * and lights go. the open picture's draws of a part before now leave no
     * instance */
    void scene_ended();
    /* a resource was released, in the open picture or between two. texture
     * is the raster's texture it held, or 0. each instance that names the
     * resource goes. one whose materials name texture is drawn again only
     * once a picture draws it. no instance takes either from the open
     * picture's draws before the release */
    void released(sa_handle handle, uint32_t texture);
    /* the open picture is whole: fold its draws into the scene's instances.
     * a picture that filled a store keeps none */
    void end();
    /* between pictures, let go of the two last pictures and all they name.
     * no picture between then reads what they borrowed, and the next picture
     * stands alone. the instances stay */
    void forget();
    /* whether pictures between the two last move from one to the other. it
     * is false after a cut, a full store or forget, or with no picture
     * before. it is false where the ticks part by none or past world.cpp's
     * MOST_STEPS */
    bool joined() const;
    /* the game's steps from the older picture to the newer, by their ticks,
     * cut or not. it gives a frame's 2 where no older picture is kept. it
     * gives 2 too where the ticks part by none or past MOST_STEPS */
    uint32_t steps() const;
    /* SA_PC_FULL_* for the store the picture last ended filled, or 0. that
     * picture is then kept as none of it */
    uint32_t full() const { return now_.full; }
    /* draw into out the picture alpha of the way from the older to the
     * newer. it returns false, with every count 0, where out had no room for
     * a draw. so it does where the newer picture is kept as none, and leaves
     * out as it was */
    bool between(float alpha, Scene &out);

    const BetweenCounts &counts() const { return counts_; }
    const std::vector<Resident> &residents() const { return residents_; }
    uint32_t resting() const; /* instances kept that the last picture missed */
    uint32_t outside() const; /* those it took out of view */
    uint32_t parts() const; /* the scene's parts kept */
    /* the scene's lights kept, or 0 where it was given none. it counts the
     * one that shines one way and each source */
    uint32_t lights() const;
    /* draw into out the meshes the scene keeps, in view or not, as seen from
     * view. it reads the scene alone and runs none of the game's code. it
     * returns how many meshes it drew */
    uint32_t overview(const sa_picture &view, Scene &out);

private:
    bool room(uint32_t textures, uint32_t points);
    void add(sa_draw_kept &k, uint32_t instance, const Key &key,
             const float *matrix);
    uint32_t find(const Snapshot &s, const Key &key) const;
    bool drew(const Snapshot &s, uint32_t instance) const;
    bool joins(uint32_t from, uint32_t to) const;
    void index(Snapshot &s);
    void fold();
    uint32_t lens(const Snapshot &s, uint32_t i, uint32_t from, float alpha,
                  const sa_view &home, Scene &out);
    void put(const Snapshot &s, uint32_t i, uint32_t from, float alpha,
             uint32_t view, Scene &out);
    bool keep(Resident &r, uint32_t i);
    bool hold(Resident &r, const MeshView &mesh, bool copied,
              const sa_material *materials, const uint32_t *textures,
              const sa_shade &shade);
    void compact();

    Snapshot before_;
    Snapshot now_;
    bool open_ = false;
    std::vector<Resident> residents_;
    std::vector<Resident> merged_;
    sa_lighting lighting_{}; /* the act's scene's, where lit_ */
    bool lit_ = false;
    std::vector<uint8_t> store_; /* writers' meshes and textures, kept */
    std::vector<uint8_t> spare_; /* compact's new store_, swapped in */
    std::vector<float> scratch_; /* a payload's points between two */
    std::vector<sa_point> scratch_points_;
    /* the Scene's index of each view the picture between has made, or
     * world.cpp's NONE. a row holds one for each view id. row 0 is for the
     * older picture's unpaired draws, and row 1 the newer's. row
     * LENS_UNPAIRED_ROWS + v is for paired draws whose older end lay under
     * view v */
    uint32_t lenses_[LENSES] = {};
    BetweenCounts counts_;
    /* draws the picture between being drawn had no room for */
    uint32_t unplaced_ = 0;
};

/* the transform alpha of the way from a to b. rotation takes the shorter
 * arc, and scale and translation move along a line. a matrix that is no
 * rotation and scale, as a shear, blends element by element. so does a
 * mirror against a matrix that isn't one */
void blend_transform(const float *a, const float *b, float alpha, float *out);
/* the same for a view, which turns about the eye */
void blend_view(const float *a, const float *b, float alpha, float *out);

} // namespace pc

#endif
