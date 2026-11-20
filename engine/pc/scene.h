/* the scene turns a picture's draws into what the raster takes. it lights
 * each point as the game lights it and places it in world space, on the
 * host. it batches the triangles by state.
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

/* SA_FORMAT_MESH's bytes, read in place */
struct MeshView {
    const float *points = nullptr;
    const float *normals = nullptr; /* null where it has none */
    const uint8_t *sets = nullptr; /* the first sa_meshset */
    uint32_t count = 0; /* points */
    uint32_t meshsets = 0;
    uint32_t materials = 0; /* a draw's list must be this long */
    uint32_t triangles = 0;
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
    /* take blend, an SA_PC_BLEND_*, and room for the most a picture holds */
    void open(uint32_t blend);
    /* a picture opens under its own view, view 0, which is its home */
    void begin(const sa_picture &picture);
    /* add a further view of the picture's, of which the scene reads the
     * matrix alone. it returns the view's index, from 1 */
    uint32_t view(const sa_view &view);
    /* take view index for the home, ahead of the picture's first draw */
    void home(uint32_t index) { home_ = index; }
    /* whether a draw under view index can go into view home's space. it can
     * where they share a matrix or home's has an inverse */
    bool carries(uint32_t home, uint32_t index) const;
    void fog(const sa_fog &fog) { fog_ = fog; }
    void palettes(const uint32_t *words) { palettes_ = words; }
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
    void sprite(const sa_sprite &s, uint32_t texture, uint32_t view = 0);
    void triangles(const sa_triangles &t, const sa_point *points,
                   uint32_t texture, uint32_t view = 0);
    /* put the blend list after the opaque, as the PowerVR does, in the order
     * open()'s blend asks */
    void finish();
    void uniform(PictureUniform &out) const;

    const std::vector<Vertex> &vertices() const { return vertices_; }
    const std::vector<Batch> &batches() const { return batches_; }
    bool sorted() const { return blend_ == SA_PC_BLEND_PIXEL; }
    uint32_t clear_color() const { return picture_.back[0]; }
    uint32_t triangle_count() const { return triangle_count_; }
    bool has_palettes() const { return palettes_ != nullptr; }

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
    void placed(const sa_draw &d, const MeshView &mesh,
                const sa_material *materials, const uint32_t *textures,
                const sa_shade &shade, const sa_lights *lights);
    void placed(const sa_sprite &s, uint32_t texture);
    void placed(const sa_triangles &t, const sa_point *points,
                uint32_t texture);
    void lines(const sa_triangles &t, const sa_point *points,
               const State &state);

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
};

} // namespace pc

#endif
