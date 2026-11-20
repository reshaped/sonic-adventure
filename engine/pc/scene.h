/* the scene turns a picture's draws into what the raster takes. it lights
 * each point as the game lights it and places it in world space, on the
 * host. it batches the triangles by state. under trace() a mesh's draw
 * becomes an instance of a traced picture, and the scene computes no
 * lighting for it.
 *
 * the raster looks through one view, the home. a draw under the home's view
 * goes as it is given. a draw under another view is carried by the home's
 * inverse times its own view. its shade's light and its sa_lights go with
 * it. the home then shows it where its own view would. every draw is
 * projected as the picture's own view projects */
#ifndef ENGINE_PC_SCENE_H
#define ENGINE_PC_SCENE_H

#include <stdint.h>

#include <vector>

#include "raster.h"
#include "sa_engine.h"
#include "sa_pc.h"

namespace pc {

/* a tick of sa_picture's in seconds: one of the game's 59.94 steps a
 * second */
constexpr double TICK_SECONDS = 1001.0 / 60000.0;

/* whether instance names a part of the act's scene, as sa_engine.h says a
 * part's does */
constexpr bool of_scene(uint32_t instance)
{
    return (instance & (SA_INSTANCE_UNOWNED | SA_INSTANCE_SCENE)) ==
           SA_INSTANCE_SCENE;
}

/* SA_FORMAT_MESH's bytes, read in place */
struct MeshView {
    const float *points = nullptr;
    const float *normals = nullptr; /* null where it has none */
    const uint8_t *sets = nullptr; /* the first sa_meshset */
    uint32_t count = 0; /* points */
    uint32_t meshsets = 0;
    uint32_t materials = 0; /* a draw's list must be this long */
    uint32_t triangles = 0;
    /* the raster's mesh of these bytes for ray tracing, or 0 for none */
    uint32_t rays = 0;
    /* half the diagonal of the box its points span, in mesh space, or 0
     * where a point is not finite. it fills the struct, which has no
     * padding, so a view compares by its bytes */
    float extent = 0.0F;
};
/* read bytes as a mesh. it returns false where they hold no whole one, or a
 * count or corner is invalid */
bool mesh_view(const void *bytes, uint32_t size, MeshView &view);
/* where a meshset's uv lie under SA_MESHSET_UV, from its head. a uint16_t
 * for each corner follows the head, padded to 4 bytes */
inline uint32_t meshset_uv_at(const sa_meshset &head)
{
    return static_cast<uint32_t>(sizeof head) + ((head.corners * 2 + 3) & ~3U);
}
/* a meshset's size, from its head to the next's. SA_MESHSET_UV adds two
 * floats a corner */
inline uint32_t meshset_bytes(const sa_meshset &head)
{
    uint32_t bytes = meshset_uv_at(head);

    return head.flags & SA_MESHSET_UV ? bytes + (head.corners * 8) : bytes;
}

/* a mesh's triangles as the raster holds them for ray tracing: each
 * meshset's corners, their uv and its material, a triangle at a time. out
 * names view's points and normals, so it lasts no longer than they do */
void ray_mesh(const MeshView &view, RayMesh &out);

/* what a traced picture reads of a raster texture's texels, each ARGB */
struct TextureColors {
    /* the mean color, each texel counted by its alpha. its alpha is the
     * mean of the texels' own. it tints an environment-mapped material's
     * reflection, and colors a sprite's lamp */
    uint32_t mean = 0xffffffff;
    /* the mean color of the bright texels, each counted by how far its
     * brightest channel lies from EMISSIVE_DIM to EMISSIVE_LIT and by its
     * alpha. the high byte is the share of the texture they cover, from 1,
     * and the whole is 0 where no texel is bright. it colors an emissive
     * material's lamps */
    uint32_t emission = 0;
};

/* the transform each named instance had in the traced picture last sent,
 * and that picture's view and time. the raster keeps a pixel's lighting from
 * picture to picture, and needs each point's position before to reproject
 * it. pictures pair in the order they are sent, which is the order a display
 * shows them */
class RayPast {
public:
    /* give each of p's instances its transform in the picture last kept,
     * and p that picture's view and the seconds since it. an instance that
     * picture had none of keeps RAY_NEW, and one it had loses it. with no
     * picture kept, p is cut and its step is 0. else p is not cut, and its
     * step is its time less the kept picture's, or 0 where time ran back.
     * view and project are p's own. it changes nothing of what is kept, so
     * a picture the device refuses may be paired again */
    void pair(RayPicture &p, const float *view, const float *project);
    /* the device took the picture last paired: keep its transforms, its
     * view and its time for the next */
    void keep();
    /* keep nothing: the next picture paired is cut */
    void forget() { held_ = false; }

private:
    struct Place {
        uint64_t name;
        float at[12];
    };
    std::vector<Place> places_; /* in their names' order */
    std::vector<Place> next_; /* the picture last paired's */
    float view_[12] = {};
    float project_[4] = {};
    double time_ = 0.0;
    float next_view_[12] = {};
    float next_project_[4] = {};
    double next_time_ = 0.0;
    bool held_ = false;
};

/* the lamps that a traced picture's named draws made, kept from picture to
 * picture. the game doesn't draw an object outside its view, so the object's
 * lamps would go out as the camera turned away, and the lighting of what the
 * picture still shows would change with the camera. a picture is given the kept
 * lamps of each draw it lacks, unless the lamp's position is in view: a lamp
 * that would be in view, from a draw the picture was not given, belongs to an
 * object that is gone */
class LampMemory {
public:
    /* add to lamps each kept lamp whose owner is not in drawn, unless the
     * lamp's position is in view, and its triangles to patches, where
     * RAY_PATCHES leaves room for them. drawn holds the names of the draws the
     * picture was given, in ascending order. view is world to view, a 3 by 4
     * matrix by rows, and project its focal x and y, then its center, in a
     * WIDTH by HEIGHT screen. with learn, the picture is the game's own: the
     * lamps whose owners[i] is not 0 replace what was kept of the owners in
     * drawn, and a kept lamp that is in view is forgotten. without learn,
     * nothing kept changes */
    void hold(std::vector<RayLamp> &lamps, std::vector<RayPatch> &patches,
              const std::vector<uint64_t> &owners,
              const std::vector<uint64_t> &drawn, const float *view,
              const float *project, bool learn);
    /* keep nothing, as at the end of an act */
    void forget()
    {
        held_.clear();
        shapes_.clear();
    }

private:
    struct Held {
        uint64_t owner;
        RayLamp lamp; /* its triangles are a run of shapes_ */
    };
    std::vector<Held> held_; /* in their owners' order */
    std::vector<RayPatch> shapes_;
};

/* the inverse of a 3 by 4 matrix by rows. it returns false where m has none */
bool affine_inverse(const float *m, double *out);
/* a times b. each is read with a last row of 0 0 0 1 */
void affine_product(const double *a, const double *b, double *out);

/* a picture between two the game drew adds at most one view for each slot
 * of World's lenses_. a row holds a slot for each view id. two rows are for
 * the older and the newer pictures' unpaired draws. a row for each view id is
 * for the paired draws whose older end lay under it */
constexpr uint32_t LENS_UNPAIRED_ROWS = 2;
constexpr uint32_t LENS_ROWS = LENS_UNPAIRED_ROWS + SA_VIEWS + 1;
constexpr uint32_t LENSES = (SA_VIEWS + 1) * LENS_ROWS;

/* a triangle of the blend list. draw_z is its draw's depth and own_z is its
 * own, for finish()'s sort. the sort puts screen triangles over the rest,
 * ordered by own_z */
struct BlendTriangle {
    State state;
    Vertex corners[3];
    float draw_z;
    float own_z;
    bool screen;
};

class Scene {
public:
    /* take blend, an SA_PC_BLEND_*, and room for the most a picture holds.
     * colors holds each raster texture's TextureColors, by its index, for a
     * traced picture to read. it outlives the scene, and may grow. null
     * gives every texture the default */
    void open(uint32_t blend, const std::vector<TextureColors> *colors);
    /* from the next picture, trace each mesh draw whose mesh the raster holds
     * for ray tracing, with samples rays a pixel. 0, the default, shades every
     * draw on the host and rasterizes it. the picture's sprites, triangles and
     * screen draws are batches either way */
    void trace(uint32_t samples) { samples_ = samples; }
    /* give each traced picture the lamps that memory keeps of the draws the
     * picture lacks, as LampMemory::hold does. learns says that the scene's
     * pictures are the game's own, which memory learns from. memory must
     * outlive the scene. null, the default, gives none */
    void remember(LampMemory *memory, bool learns)
    {
        memory_ = memory;
        learns_ = learns;
    }
    /* a picture opens under its own view, view 0, which is its home. time
     * is the picture's, in seconds, which a traced picture carries */
    void begin(const sa_picture &picture, double time);
    /* whether the open picture is traced */
    bool traces() const { return tracing_; }
    /* add a further view of the picture's, of which the scene reads the
     * matrix alone. it returns the view's index, from 1 */
    uint32_t view(const sa_view &view);
    /* take view index for the home, ahead of the picture's first draw */
    void home(uint32_t index) { home_ = index; }
    /* whether a draw under view index can go into view home's space. it can
     * where they share a matrix or home's has an inverse */
    bool carries(uint32_t home, uint32_t index) const;
    void fog(const sa_fog &fog) { fog_ = fog; }
    void palettes(const uint32_t *words);
    /* whether the scene has room for this many triangles more */
    bool room(uint32_t triangles) const;
    /* textures[i] is materials[i]'s raster texture, 0 for white. lights
     * are an SA_SHADE_LIGHTS shade's, and null elsewhere. view is the
     * scene's index of the draw's view, and d.view goes unread. a draw whose
     * view carries() refuses goes as it is given */
    void draw(const sa_draw &d, const MeshView &mesh,
              const sa_material *materials, const uint32_t *textures,
              const sa_shade &shade, const sa_lights *lights = nullptr,
              uint32_t view = 0);
    /* a mesh that is in the picture's world though the picture draws none of
     * it: one outside the game's view, or a part of the act's scene that the
     * game left out of its draw list. only a traced picture takes it, and only
     * shadow, bounce and reflection rays hit it. it adds no batch and no
     * triangle to the picture's count. it must come before finish(), which
     * chooses among the picture's lamps. view is the scene's index of the view
     * to carry the draw from: the one it was made under, or the picture's own,
     * index 0, for a part that no draw of the picture placed */
    void hidden(const sa_draw &d, const MeshView &mesh,
                const sa_material *materials, const uint32_t *textures,
                const sa_shade &shade, uint32_t view);
    /* whether the open picture, a traced one, was given a draw of the part
     * of the act's scene that instance names, by draw() or hidden() */
    bool holds(uint32_t instance) const;
    void sprite(const sa_sprite &s, uint32_t texture, uint32_t view = 0);
    void triangles(const sa_triangles &t, const sa_point *points,
                   uint32_t texture, uint32_t view = 0);
    /* put the blend list after the opaque, as the PowerVR does, in the order
     * open()'s blend asks. a traced picture then takes its eye and the lamps
     * kept of the draws it lacks, to RAY_LAMPS at most */
    void finish();
    void uniform(PictureUniform &out) const;

    const std::vector<Vertex> &vertices() const { return vertices_; }
    const std::vector<Batch> &batches() const { return batches_; }
    bool sorted() const { return blend_ == SA_PC_BLEND_PIXEL; }
    uint32_t clear_color() const { return picture_.back[0]; }
    uint32_t triangle_count() const { return triangle_count_; }
    bool has_palettes() const { return palettes_ != nullptr; }
    /* a traced picture's input to the trace pass, complete once finish() has
     * closed it, or null for a picture that is only rasterized */
    const RayPicture *ray_picture() const
    {
        return tracing_ ? &rays_ : nullptr;
    }
    /* pair a traced picture that finish() has closed with the one past kept, as
     * RayPast::pair does, by the view the picture is drawn through. it does
     * nothing for a picture that is only rasterized */
    void pair(RayPast &past);

private:
    /* a view of the picture's, and what carries a draw under it into the
     * home's space. same is set where the views share a matrix or the home
     * has no inverse. a draw then goes as it is given. else carry is the
     * home's inverse times the view, and turn turns a light as a normal
     * does */
    struct Lens {
        sa_view view;
        uint32_t home; /* the home carry and turn were made for, or NONE */
        bool same;
        double carry[12];
        double turn[9];
    };
    static constexpr uint32_t NONE = UINT32_MAX;

    /* the home's matrix, world to view */
    const float *look() const { return lenses_[home_].view.world_to_view; }
    const Lens *carried(uint32_t index);
    float view_z(const float *p) const;
    void back_plane(const uint32_t back[3]);
    void take(const sa_draw &d, const MeshView &mesh,
              const sa_material *materials, const uint32_t *textures,
              const sa_shade &shade, const sa_lights *lights, uint32_t view,
              bool seen);
    void placed(const sa_draw &d, const MeshView &mesh,
                const sa_material *materials, const uint32_t *textures,
                const sa_shade &shade, const sa_lights *lights);
    void placed(const sa_sprite &s, uint32_t texture);
    void placed(const sa_triangles &t, const sa_point *points,
                uint32_t texture);
    void lines(const sa_triangles &t, const sa_point *points,
               const State &state);
    void instance(const sa_draw &d, const MeshView &mesh,
                  const sa_material *materials, const uint32_t *textures,
                  const sa_shade &shade, bool seen);
    TextureColors colors(uint32_t texture) const;
    void add_lamp(const RayLamp &lamp);
    void lamp(const sa_sprite &s, uint32_t texture, const float *at);
    void emitters(const sa_draw &d, const MeshView &mesh,
                  const sa_material *materials, const uint32_t *textures);

    uint32_t blend_ = SA_PC_BLEND_PIXEL;
    sa_picture picture_{};
    std::vector<Lens> lenses_; /* the picture's own, then those given */
    uint32_t home_ = 0;
    sa_fog fog_{};
    const uint32_t *palettes_ = nullptr;
    uint32_t triangle_count_ = 0;
    std::vector<Vertex> vertices_;
    std::vector<Batch> batches_;
    std::vector<BlendTriangle> blend_list_;
    /* a draw's points as placed() places and lights them. these keep their
     * room from draw to draw */
    std::vector<float> world_;
    std::vector<uint32_t> base_;
    std::vector<uint32_t> offset_;
    std::vector<float> env_;
    std::vector<float> intensity_;
    std::vector<float> normal_; /* in world space, under SA_SHADE_LIGHTS */
    /* the material + 1 a point was lit for, 0 for none yet */
    std::vector<uint32_t> lit_;
    const std::vector<TextureColors> *colors_ = nullptr; /* open()'s */
    uint32_t samples_ = 0; /* trace()'s */
    bool tracing_ = false; /* whether the open picture is traced */
    bool sunned_ = false; /* a draw has given the traced picture its sun */
    /* the instances of the act's parts the traced picture was given, in
     * ascending order */
    std::vector<uint32_t> parts_;
    /* emitters()'s triangles of one material: each one's centroid, its shape
     * and the cell it falls in */
    struct Emitting {
        float centroid[3];
        RayPatch patch;
        int cell;
    };
    std::vector<Emitting> emitting_;
    LampMemory *memory_ = nullptr; /* remember()'s */
    bool learns_ = false;
    /* the name of the draw whose lamps emitters() is making, or 0 */
    uint64_t owner_ = 0;
    /* each of rays_.lamps' owners, 0 for a lamp that has none, and the
     * names of the traced picture's draws that are no part of the act's
     * scene */
    std::vector<uint64_t> lamp_owners_;
    std::vector<uint64_t> drawn_;
    RayPicture rays_; /* the traced picture's input to the trace pass */
};

} // namespace pc

#endif
