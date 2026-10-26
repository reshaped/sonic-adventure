/* Chaos 2 as the events pose him, with welds that close his model's seams
 * in each pose */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "player/player.h"
#include "shared/chaos0_water_textures.h"
#include "shared/chaos2.h"
#include "shared/weld.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 10
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 10
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 10
#include "shared/chaos2/text.inc"

/* the nodes in his model. event_weld poses all of them */
#define CHAOS2_NODES 92
#define CHAOS2_ATTR NJD_FLAG_USE_ALPHA
/* how far a melt sinks his body in the steps create took */
#define CHAOS2_DEPTH 25.0F
/* how far above the puddle his body draws, plus lift */
#define CHAOS2_BODY_HEIGHT 10.0F
/* the share of the puddle's x and z scale its wobble sways it by */
#define CHAOS2_SWAY 0.125F

/* what his step does, by work->mode */
enum chaos2_mode {
    CHAOS2_STAND, /* he loops his action */
    CHAOS2_MELT, /* his body sinks as the puddle grows */
    CHAOS2_RISE, /* his body rises as the puddle shrinks */
    CHAOS2_PUDDLE, /* the puddle alone, wobbling in place */
    CHAOS2_SLIDE /* the puddle alone, wobbling as speed moves it */
};

/* his state, which event_chaos2_create allocates and his destructor frees */
struct chaos2_state {
    NJS_POINT3 to; /* where the slide is headed. nothing reads it */
    NJS_VECTOR speed; /* how far the slide moves him each step */
    Sint32 steps; /* the slide lasts steps + 1 steps */
    Sint32 form_steps; /* the steps a melt or a rise divides its change into */
    Float action_speed; /* how many frames of his action play each step */
    Float frame; /* his action's frame */
    Float lift; /* added to CHAOS2_BODY_HEIGHT */
    Float lift_speed;
    NJS_VECTOR puddle_scale;
    NJS_VECTOR form_speed;
    NJS_VECTOR form; /* the puddle's scale as he melts and rises */
    /* the rise's draw passes alpha - 1 to draw_set_material for the puddle.
     * a melt raises it, but no other draw reads it */
    Float alpha;
    Float alpha_speed;
    Angle wobble; /* the phase of the wobble that sways the puddle's scale */
};

struct chaos2_vars {
    Uint8 unk_00[4];
    Sint32 timer; /* steps into a melt, a rise or the slide */
    struct chaos2_state *state;
};

#define VARS(work) ((struct chaos2_vars *)&(work)->vars)

/* Chaos 0's puddle, which each event defines in its own data */
extern NJS_OBJECT event_chaos0_puddle_object;

static NJS_TEXNAME event_chaos2_texnames[6] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
};
NJS_TEXLIST event_chaos2_texlist = {
#include "shared/chaos2/event_chaos2_texlist.inc"
};

/* his model and his action. a node's name gives its place in the model's
 * walk, as tree_node_index counts it, from 0 at event_chaos2_object. the
 * parts of a node's model and the motion's keys for it carry its number */
static NJS_OBJECT event_chaos2_node91 = {
#include "shared/chaos2/event_chaos2_node91.inc"
};
static NJS_OBJECT event_chaos2_node90 = {
#include "shared/chaos2/event_chaos2_node90.inc"
};
static NJS_OBJECT event_chaos2_node89 = {
#include "shared/chaos2/event_chaos2_node89.inc"
};
static NJS_OBJECT event_chaos2_node88 = {
#include "shared/chaos2/event_chaos2_node88.inc"
};
static NJS_OBJECT event_chaos2_node87 = {
#include "shared/chaos2/event_chaos2_node87.inc"
};
static NJS_OBJECT event_chaos2_node86 = {
#include "shared/chaos2/event_chaos2_node86.inc"
};
static NJS_OBJECT event_chaos2_node85 = {
#include "shared/chaos2/event_chaos2_node85.inc"
};
static NJS_OBJECT event_chaos2_node84 = {
#include "shared/chaos2/event_chaos2_node84.inc"
};
static NJS_OBJECT event_chaos2_node83 = {
#include "shared/chaos2/event_chaos2_node83.inc"
};
static NJS_OBJECT event_chaos2_node82 = {
#include "shared/chaos2/event_chaos2_node82.inc"
};
static NJS_MATERIAL event_chaos2_node81_materials[1] = {
#include "shared/chaos2/event_chaos2_node81_materials.inc"
};
static Sint16 event_chaos2_node81_meshes0[36] = {
#include "shared/chaos2/event_chaos2_node81_meshes0.inc"
};
static Sint16 event_chaos2_node81_meshes1[8] = {
#include "shared/chaos2/event_chaos2_node81_meshes1.inc"
};
static NJS_COLOR event_chaos2_node81_uvs0[36] = {
#include "shared/chaos2/event_chaos2_node81_uvs0.inc"
};
static NJS_COLOR event_chaos2_node81_uvs1[8] = {
#include "shared/chaos2/event_chaos2_node81_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node81_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node81_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node81_points[16] = {
#include "shared/chaos2/event_chaos2_node81_points.inc"
};
static NJS_VECTOR event_chaos2_node81_normals[16] = {
#include "shared/chaos2/event_chaos2_node81_normals.inc"
};
static NJS_MODEL event_chaos2_node81_model = {
#include "shared/chaos2/event_chaos2_node81_model.inc"
};
static NJS_OBJECT event_chaos2_node81 = {
#include "shared/chaos2/event_chaos2_node81.inc"
};
static NJS_MATERIAL event_chaos2_node80_materials[1] = {
#include "shared/chaos2/event_chaos2_node80_materials.inc"
};
static Sint16 event_chaos2_node80_meshes0[64] = {
#include "shared/chaos2/event_chaos2_node80_meshes0.inc"
};
static NJS_COLOR event_chaos2_node80_uvs0[64] = {
#include "shared/chaos2/event_chaos2_node80_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node80_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node80_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node80_points[40] = {
#include "shared/chaos2/event_chaos2_node80_points.inc"
};
static NJS_VECTOR event_chaos2_node80_normals[40] = {
#include "shared/chaos2/event_chaos2_node80_normals.inc"
};
static NJS_MODEL event_chaos2_node80_model = {
#include "shared/chaos2/event_chaos2_node80_model.inc"
};
static NJS_OBJECT event_chaos2_node80 = {
#include "shared/chaos2/event_chaos2_node80.inc"
};
static NJS_MATERIAL event_chaos2_node79_materials[1] = {
#include "shared/chaos2/event_chaos2_node79_materials.inc"
};
static Sint16 event_chaos2_node79_meshes0[24] = {
#include "shared/chaos2/event_chaos2_node79_meshes0.inc"
};
static NJS_COLOR event_chaos2_node79_uvs0[24] = {
#include "shared/chaos2/event_chaos2_node79_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node79_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node79_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node79_points[8] = {
#include "shared/chaos2/event_chaos2_node79_points.inc"
};
static NJS_VECTOR event_chaos2_node79_normals[8] = {
#include "shared/chaos2/event_chaos2_node79_normals.inc"
};
static NJS_MODEL event_chaos2_node79_model = {
#include "shared/chaos2/event_chaos2_node79_model.inc"
};
static NJS_OBJECT event_chaos2_node79 = {
#include "shared/chaos2/event_chaos2_node79.inc"
};
static NJS_MATERIAL event_chaos2_node78_materials[1] = {
#include "shared/chaos2/event_chaos2_node78_materials.inc"
};
static Sint16 event_chaos2_node78_meshes0[36] = {
#include "shared/chaos2/event_chaos2_node78_meshes0.inc"
};
static Sint16 event_chaos2_node78_meshes1[96] = {
#include "shared/chaos2/event_chaos2_node78_meshes1.inc"
};
static NJS_COLOR event_chaos2_node78_uvs0[36] = {
#include "shared/chaos2/event_chaos2_node78_uvs0.inc"
};
static NJS_COLOR event_chaos2_node78_uvs1[96] = {
#include "shared/chaos2/event_chaos2_node78_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node78_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node78_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node78_points[32] = {
#include "shared/chaos2/event_chaos2_node78_points.inc"
};
static NJS_VECTOR event_chaos2_node78_normals[32] = {
#include "shared/chaos2/event_chaos2_node78_normals.inc"
};
static NJS_MODEL event_chaos2_node78_model = {
#include "shared/chaos2/event_chaos2_node78_model.inc"
};
static NJS_OBJECT event_chaos2_node78 = {
#include "shared/chaos2/event_chaos2_node78.inc"
};
static NJS_MATERIAL event_chaos2_node77_materials[1] = {
#include "shared/chaos2/event_chaos2_node77_materials.inc"
};
static Sint16 event_chaos2_node77_meshes0[60] = {
#include "shared/chaos2/event_chaos2_node77_meshes0.inc"
};
static Sint16 event_chaos2_node77_meshes1[88] = {
#include "shared/chaos2/event_chaos2_node77_meshes1.inc"
};
static NJS_COLOR event_chaos2_node77_uvs0[60] = {
#include "shared/chaos2/event_chaos2_node77_uvs0.inc"
};
static NJS_COLOR event_chaos2_node77_uvs1[88] = {
#include "shared/chaos2/event_chaos2_node77_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node77_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node77_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node77_points[42] = {
#include "shared/chaos2/event_chaos2_node77_points.inc"
};
static NJS_VECTOR event_chaos2_node77_normals[42] = {
#include "shared/chaos2/event_chaos2_node77_normals.inc"
};
static NJS_MODEL event_chaos2_node77_model = {
#include "shared/chaos2/event_chaos2_node77_model.inc"
};
static NJS_OBJECT event_chaos2_node77 = {
#include "shared/chaos2/event_chaos2_node77.inc"
};
static NJS_MATERIAL event_chaos2_node76_materials[2] = {
#include "shared/chaos2/event_chaos2_node76_materials.inc"
};
static Sint16 event_chaos2_node76_meshes0[492] = {
#include "shared/chaos2/event_chaos2_node76_meshes0.inc"
};
static Sint16 event_chaos2_node76_meshes1[252] = {
#include "shared/chaos2/event_chaos2_node76_meshes1.inc"
};
static Sint16 event_chaos2_node76_meshes2[60] = {
#include "shared/chaos2/event_chaos2_node76_meshes2.inc"
};
static Sint16 event_chaos2_node76_meshes3[88] = {
#include "shared/chaos2/event_chaos2_node76_meshes3.inc"
};
static NJS_COLOR event_chaos2_node76_uvs0[492] = {
#include "shared/chaos2/event_chaos2_node76_uvs0.inc"
};
static NJS_COLOR event_chaos2_node76_uvs1[252] = {
#include "shared/chaos2/event_chaos2_node76_uvs1.inc"
};
static NJS_COLOR event_chaos2_node76_uvs2[60] = {
#include "shared/chaos2/event_chaos2_node76_uvs2.inc"
};
static NJS_COLOR event_chaos2_node76_uvs3[88] = {
#include "shared/chaos2/event_chaos2_node76_uvs3.inc"
};
static NJS_MESHSET event_chaos2_node76_meshsets[4] = {
#include "shared/chaos2/event_chaos2_node76_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node76_points[181] = {
#include "shared/chaos2/event_chaos2_node76_points.inc"
};
static NJS_VECTOR event_chaos2_node76_normals[181] = {
#include "shared/chaos2/event_chaos2_node76_normals.inc"
};
static NJS_MODEL event_chaos2_node76_model = {
#include "shared/chaos2/event_chaos2_node76_model.inc"
};
static NJS_OBJECT event_chaos2_node76 = {
#include "shared/chaos2/event_chaos2_node76.inc"
};
static NJS_OBJECT event_chaos2_node75 = {
#include "shared/chaos2/event_chaos2_node75.inc"
};
static NJS_OBJECT event_chaos2_node74 = {
#include "shared/chaos2/event_chaos2_node74.inc"
};
static NJS_OBJECT event_chaos2_node73 = {
#include "shared/chaos2/event_chaos2_node73.inc"
};
static NJS_OBJECT event_chaos2_node72 = {
#include "shared/chaos2/event_chaos2_node72.inc"
};
static NJS_MATERIAL event_chaos2_node71_materials[1] = {
#include "shared/chaos2/event_chaos2_node71_materials.inc"
};
static Sint16 event_chaos2_node71_meshes0[96] = {
#include "shared/chaos2/event_chaos2_node71_meshes0.inc"
};
static NJS_COLOR event_chaos2_node71_uvs0[96] = {
#include "shared/chaos2/event_chaos2_node71_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node71_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node71_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node71_points[21] = {
#include "shared/chaos2/event_chaos2_node71_points.inc"
};
static NJS_VECTOR event_chaos2_node71_normals[21] = {
#include "shared/chaos2/event_chaos2_node71_normals.inc"
};
static NJS_MODEL event_chaos2_node71_model = {
#include "shared/chaos2/event_chaos2_node71_model.inc"
};
static NJS_OBJECT event_chaos2_node71 = {
#include "shared/chaos2/event_chaos2_node71.inc"
};
static NJS_MATERIAL event_chaos2_node70_materials[1] = {
#include "shared/chaos2/event_chaos2_node70_materials.inc"
};
static Sint16 event_chaos2_node70_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node70_meshes0.inc"
};
static NJS_COLOR event_chaos2_node70_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node70_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node70_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node70_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node70_points[24] = {
#include "shared/chaos2/event_chaos2_node70_points.inc"
};
static NJS_VECTOR event_chaos2_node70_normals[24] = {
#include "shared/chaos2/event_chaos2_node70_normals.inc"
};
static NJS_MODEL event_chaos2_node70_model = {
#include "shared/chaos2/event_chaos2_node70_model.inc"
};
static NJS_OBJECT event_chaos2_node70 = {
#include "shared/chaos2/event_chaos2_node70.inc"
};
static NJS_MATERIAL event_chaos2_node69_materials[1] = {
#include "shared/chaos2/event_chaos2_node69_materials.inc"
};
static Sint16 event_chaos2_node69_meshes0[60] = {
#include "shared/chaos2/event_chaos2_node69_meshes0.inc"
};
static NJS_COLOR event_chaos2_node69_uvs0[60] = {
#include "shared/chaos2/event_chaos2_node69_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node69_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node69_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node69_points[20] = {
#include "shared/chaos2/event_chaos2_node69_points.inc"
};
static NJS_VECTOR event_chaos2_node69_normals[20] = {
#include "shared/chaos2/event_chaos2_node69_normals.inc"
};
static NJS_MODEL event_chaos2_node69_model = {
#include "shared/chaos2/event_chaos2_node69_model.inc"
};
static NJS_OBJECT event_chaos2_node69 = {
#include "shared/chaos2/event_chaos2_node69.inc"
};
static NJS_MATERIAL event_chaos2_node68_materials[1] = {
#include "shared/chaos2/event_chaos2_node68_materials.inc"
};
static Sint16 event_chaos2_node68_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node68_meshes0.inc"
};
static NJS_COLOR event_chaos2_node68_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node68_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node68_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node68_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node68_points[24] = {
#include "shared/chaos2/event_chaos2_node68_points.inc"
};
static NJS_VECTOR event_chaos2_node68_normals[24] = {
#include "shared/chaos2/event_chaos2_node68_normals.inc"
};
static NJS_MODEL event_chaos2_node68_model = {
#include "shared/chaos2/event_chaos2_node68_model.inc"
};
static NJS_OBJECT event_chaos2_node68 = {
#include "shared/chaos2/event_chaos2_node68.inc"
};
static NJS_MATERIAL event_chaos2_node67_materials[1] = {
#include "shared/chaos2/event_chaos2_node67_materials.inc"
};
static Sint16 event_chaos2_node67_meshes0[132] = {
#include "shared/chaos2/event_chaos2_node67_meshes0.inc"
};
static Sint16 event_chaos2_node67_meshes1[68] = {
#include "shared/chaos2/event_chaos2_node67_meshes1.inc"
};
static NJS_COLOR event_chaos2_node67_uvs0[132] = {
#include "shared/chaos2/event_chaos2_node67_uvs0.inc"
};
static NJS_COLOR event_chaos2_node67_uvs1[68] = {
#include "shared/chaos2/event_chaos2_node67_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node67_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node67_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node67_points[39] = {
#include "shared/chaos2/event_chaos2_node67_points.inc"
};
static NJS_VECTOR event_chaos2_node67_normals[39] = {
#include "shared/chaos2/event_chaos2_node67_normals.inc"
};
static NJS_MODEL event_chaos2_node67_model = {
#include "shared/chaos2/event_chaos2_node67_model.inc"
};
static NJS_OBJECT event_chaos2_node67 = {
#include "shared/chaos2/event_chaos2_node67.inc"
};
static NJS_MATERIAL event_chaos2_node66_materials[1] = {
#include "shared/chaos2/event_chaos2_node66_materials.inc"
};
static Sint16 event_chaos2_node66_meshes0[24] = {
#include "shared/chaos2/event_chaos2_node66_meshes0.inc"
};
static NJS_COLOR event_chaos2_node66_uvs0[24] = {
#include "shared/chaos2/event_chaos2_node66_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node66_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node66_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node66_points[6] = {
#include "shared/chaos2/event_chaos2_node66_points.inc"
};
static NJS_VECTOR event_chaos2_node66_normals[6] = {
#include "shared/chaos2/event_chaos2_node66_normals.inc"
};
static NJS_MODEL event_chaos2_node66_model = {
#include "shared/chaos2/event_chaos2_node66_model.inc"
};
static NJS_OBJECT event_chaos2_node66 = {
#include "shared/chaos2/event_chaos2_node66.inc"
};
static NJS_MATERIAL event_chaos2_node65_materials[1] = {
#include "shared/chaos2/event_chaos2_node65_materials.inc"
};
static Sint16 event_chaos2_node65_meshes0[24] = {
#include "shared/chaos2/event_chaos2_node65_meshes0.inc"
};
static NJS_COLOR event_chaos2_node65_uvs0[24] = {
#include "shared/chaos2/event_chaos2_node65_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node65_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node65_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node65_points[6] = {
#include "shared/chaos2/event_chaos2_node65_points.inc"
};
static NJS_VECTOR event_chaos2_node65_normals[6] = {
#include "shared/chaos2/event_chaos2_node65_normals.inc"
};
static NJS_MODEL event_chaos2_node65_model = {
#include "shared/chaos2/event_chaos2_node65_model.inc"
};
static NJS_OBJECT event_chaos2_node65 = {
#include "shared/chaos2/event_chaos2_node65.inc"
};
static NJS_MATERIAL event_chaos2_node64_materials[1] = {
#include "shared/chaos2/event_chaos2_node64_materials.inc"
};
static Sint16 event_chaos2_node64_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node64_meshes0.inc"
};
static NJS_COLOR event_chaos2_node64_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node64_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node64_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node64_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node64_points[24] = {
#include "shared/chaos2/event_chaos2_node64_points.inc"
};
static NJS_VECTOR event_chaos2_node64_normals[24] = {
#include "shared/chaos2/event_chaos2_node64_normals.inc"
};
static NJS_MODEL event_chaos2_node64_model = {
#include "shared/chaos2/event_chaos2_node64_model.inc"
};
static NJS_OBJECT event_chaos2_node64 = {
#include "shared/chaos2/event_chaos2_node64.inc"
};
static NJS_MATERIAL event_chaos2_node63_materials[1] = {
#include "shared/chaos2/event_chaos2_node63_materials.inc"
};
static Sint16 event_chaos2_node63_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node63_meshes0.inc"
};
static NJS_COLOR event_chaos2_node63_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node63_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node63_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node63_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node63_points[24] = {
#include "shared/chaos2/event_chaos2_node63_points.inc"
};
static NJS_VECTOR event_chaos2_node63_normals[24] = {
#include "shared/chaos2/event_chaos2_node63_normals.inc"
};
static NJS_MODEL event_chaos2_node63_model = {
#include "shared/chaos2/event_chaos2_node63_model.inc"
};
static NJS_OBJECT event_chaos2_node63 = {
#include "shared/chaos2/event_chaos2_node63.inc"
};
static NJS_MATERIAL event_chaos2_node62_materials[1] = {
#include "shared/chaos2/event_chaos2_node62_materials.inc"
};
static Sint16 event_chaos2_node62_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node62_meshes0.inc"
};
static NJS_COLOR event_chaos2_node62_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node62_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node62_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node62_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node62_points[24] = {
#include "shared/chaos2/event_chaos2_node62_points.inc"
};
static NJS_VECTOR event_chaos2_node62_normals[24] = {
#include "shared/chaos2/event_chaos2_node62_normals.inc"
};
static NJS_MODEL event_chaos2_node62_model = {
#include "shared/chaos2/event_chaos2_node62_model.inc"
};
static NJS_OBJECT event_chaos2_node62 = {
#include "shared/chaos2/event_chaos2_node62.inc"
};
static NJS_MATERIAL event_chaos2_node61_materials[1] = {
#include "shared/chaos2/event_chaos2_node61_materials.inc"
};
static Sint16 event_chaos2_node61_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node61_meshes0.inc"
};
static NJS_COLOR event_chaos2_node61_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node61_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node61_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node61_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node61_points[24] = {
#include "shared/chaos2/event_chaos2_node61_points.inc"
};
static NJS_VECTOR event_chaos2_node61_normals[24] = {
#include "shared/chaos2/event_chaos2_node61_normals.inc"
};
static NJS_MODEL event_chaos2_node61_model = {
#include "shared/chaos2/event_chaos2_node61_model.inc"
};
static NJS_OBJECT event_chaos2_node61 = {
#include "shared/chaos2/event_chaos2_node61.inc"
};
static NJS_MATERIAL event_chaos2_node60_materials[1] = {
#include "shared/chaos2/event_chaos2_node60_materials.inc"
};
static Sint16 event_chaos2_node60_meshes0[57] = {
#include "shared/chaos2/event_chaos2_node60_meshes0.inc"
};
static Sint16 event_chaos2_node60_meshes1[60] = {
#include "shared/chaos2/event_chaos2_node60_meshes1.inc"
};
static NJS_COLOR event_chaos2_node60_uvs0[57] = {
#include "shared/chaos2/event_chaos2_node60_uvs0.inc"
};
static NJS_COLOR event_chaos2_node60_uvs1[60] = {
#include "shared/chaos2/event_chaos2_node60_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node60_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node60_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node60_points[28] = {
#include "shared/chaos2/event_chaos2_node60_points.inc"
};
static NJS_VECTOR event_chaos2_node60_normals[28] = {
#include "shared/chaos2/event_chaos2_node60_normals.inc"
};
static NJS_MODEL event_chaos2_node60_model = {
#include "shared/chaos2/event_chaos2_node60_model.inc"
};
static NJS_OBJECT event_chaos2_node60 = {
#include "shared/chaos2/event_chaos2_node60.inc"
};
static NJS_MATERIAL event_chaos2_node59_materials[1] = {
#include "shared/chaos2/event_chaos2_node59_materials.inc"
};
static Sint16 event_chaos2_node59_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node59_meshes0.inc"
};
static NJS_COLOR event_chaos2_node59_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node59_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node59_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node59_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node59_points[24] = {
#include "shared/chaos2/event_chaos2_node59_points.inc"
};
static NJS_VECTOR event_chaos2_node59_normals[24] = {
#include "shared/chaos2/event_chaos2_node59_normals.inc"
};
static NJS_MODEL event_chaos2_node59_model = {
#include "shared/chaos2/event_chaos2_node59_model.inc"
};
static NJS_OBJECT event_chaos2_node59 = {
#include "shared/chaos2/event_chaos2_node59.inc"
};
static NJS_MATERIAL event_chaos2_node58_materials[1] = {
#include "shared/chaos2/event_chaos2_node58_materials.inc"
};
static Sint16 event_chaos2_node58_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node58_meshes0.inc"
};
static NJS_COLOR event_chaos2_node58_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node58_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node58_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node58_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node58_points[24] = {
#include "shared/chaos2/event_chaos2_node58_points.inc"
};
static NJS_VECTOR event_chaos2_node58_normals[24] = {
#include "shared/chaos2/event_chaos2_node58_normals.inc"
};
static NJS_MODEL event_chaos2_node58_model = {
#include "shared/chaos2/event_chaos2_node58_model.inc"
};
static NJS_OBJECT event_chaos2_node58 = {
#include "shared/chaos2/event_chaos2_node58.inc"
};
static NJS_MATERIAL event_chaos2_node57_materials[1] = {
#include "shared/chaos2/event_chaos2_node57_materials.inc"
};
static Sint16 event_chaos2_node57_meshes0[9] = {
#include "shared/chaos2/event_chaos2_node57_meshes0.inc"
};
static Sint16 event_chaos2_node57_meshes1[24] = {
#include "shared/chaos2/event_chaos2_node57_meshes1.inc"
};
static NJS_COLOR event_chaos2_node57_uvs0[9] = {
#include "shared/chaos2/event_chaos2_node57_uvs0.inc"
};
static NJS_COLOR event_chaos2_node57_uvs1[24] = {
#include "shared/chaos2/event_chaos2_node57_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node57_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node57_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node57_points[10] = {
#include "shared/chaos2/event_chaos2_node57_points.inc"
};
static NJS_VECTOR event_chaos2_node57_normals[10] = {
#include "shared/chaos2/event_chaos2_node57_normals.inc"
};
static NJS_MODEL event_chaos2_node57_model = {
#include "shared/chaos2/event_chaos2_node57_model.inc"
};
static NJS_OBJECT event_chaos2_node57 = {
#include "shared/chaos2/event_chaos2_node57.inc"
};
static NJS_MATERIAL event_chaos2_node56_materials[1] = {
#include "shared/chaos2/event_chaos2_node56_materials.inc"
};
static Sint16 event_chaos2_node56_meshes0[80] = {
#include "shared/chaos2/event_chaos2_node56_meshes0.inc"
};
static NJS_COLOR event_chaos2_node56_uvs0[80] = {
#include "shared/chaos2/event_chaos2_node56_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node56_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node56_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node56_points[25] = {
#include "shared/chaos2/event_chaos2_node56_points.inc"
};
static NJS_VECTOR event_chaos2_node56_normals[25] = {
#include "shared/chaos2/event_chaos2_node56_normals.inc"
};
static NJS_MODEL event_chaos2_node56_model = {
#include "shared/chaos2/event_chaos2_node56_model.inc"
};
static NJS_OBJECT event_chaos2_node56 = {
#include "shared/chaos2/event_chaos2_node56.inc"
};
static NJS_OBJECT event_chaos2_node55 = {
#include "shared/chaos2/event_chaos2_node55.inc"
};
static NJS_OBJECT event_chaos2_node54 = {
#include "shared/chaos2/event_chaos2_node54.inc"
};
static NJS_OBJECT event_chaos2_node53 = {
#include "shared/chaos2/event_chaos2_node53.inc"
};
static NJS_OBJECT event_chaos2_node52 = {
#include "shared/chaos2/event_chaos2_node52.inc"
};
static NJS_OBJECT event_chaos2_node51 = {
#include "shared/chaos2/event_chaos2_node51.inc"
};
static NJS_MATERIAL event_chaos2_node50_materials[1] = {
#include "shared/chaos2/event_chaos2_node50_materials.inc"
};
static Sint16 event_chaos2_node50_meshes0[96] = {
#include "shared/chaos2/event_chaos2_node50_meshes0.inc"
};
static NJS_COLOR event_chaos2_node50_uvs0[96] = {
#include "shared/chaos2/event_chaos2_node50_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node50_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node50_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node50_points[21] = {
#include "shared/chaos2/event_chaos2_node50_points.inc"
};
static NJS_VECTOR event_chaos2_node50_normals[21] = {
#include "shared/chaos2/event_chaos2_node50_normals.inc"
};
static NJS_MODEL event_chaos2_node50_model = {
#include "shared/chaos2/event_chaos2_node50_model.inc"
};
static NJS_OBJECT event_chaos2_node50 = {
#include "shared/chaos2/event_chaos2_node50.inc"
};
static NJS_MATERIAL event_chaos2_node49_materials[1] = {
#include "shared/chaos2/event_chaos2_node49_materials.inc"
};
static Sint16 event_chaos2_node49_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node49_meshes0.inc"
};
static NJS_COLOR event_chaos2_node49_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node49_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node49_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node49_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node49_points[24] = {
#include "shared/chaos2/event_chaos2_node49_points.inc"
};
static NJS_VECTOR event_chaos2_node49_normals[24] = {
#include "shared/chaos2/event_chaos2_node49_normals.inc"
};
static NJS_MODEL event_chaos2_node49_model = {
#include "shared/chaos2/event_chaos2_node49_model.inc"
};
static NJS_OBJECT event_chaos2_node49 = {
#include "shared/chaos2/event_chaos2_node49.inc"
};
static NJS_MATERIAL event_chaos2_node48_materials[1] = {
#include "shared/chaos2/event_chaos2_node48_materials.inc"
};
static Sint16 event_chaos2_node48_meshes0[60] = {
#include "shared/chaos2/event_chaos2_node48_meshes0.inc"
};
static NJS_COLOR event_chaos2_node48_uvs0[60] = {
#include "shared/chaos2/event_chaos2_node48_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node48_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node48_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node48_points[20] = {
#include "shared/chaos2/event_chaos2_node48_points.inc"
};
static NJS_VECTOR event_chaos2_node48_normals[20] = {
#include "shared/chaos2/event_chaos2_node48_normals.inc"
};
static NJS_MODEL event_chaos2_node48_model = {
#include "shared/chaos2/event_chaos2_node48_model.inc"
};
static NJS_OBJECT event_chaos2_node48 = {
#include "shared/chaos2/event_chaos2_node48.inc"
};
static NJS_MATERIAL event_chaos2_node47_materials[1] = {
#include "shared/chaos2/event_chaos2_node47_materials.inc"
};
static Sint16 event_chaos2_node47_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node47_meshes0.inc"
};
static NJS_COLOR event_chaos2_node47_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node47_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node47_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node47_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node47_points[24] = {
#include "shared/chaos2/event_chaos2_node47_points.inc"
};
static NJS_VECTOR event_chaos2_node47_normals[24] = {
#include "shared/chaos2/event_chaos2_node47_normals.inc"
};
static NJS_MODEL event_chaos2_node47_model = {
#include "shared/chaos2/event_chaos2_node47_model.inc"
};
static NJS_OBJECT event_chaos2_node47 = {
#include "shared/chaos2/event_chaos2_node47.inc"
};
static NJS_MATERIAL event_chaos2_node46_materials[1] = {
#include "shared/chaos2/event_chaos2_node46_materials.inc"
};
static Sint16 event_chaos2_node46_meshes0[132] = {
#include "shared/chaos2/event_chaos2_node46_meshes0.inc"
};
static Sint16 event_chaos2_node46_meshes1[68] = {
#include "shared/chaos2/event_chaos2_node46_meshes1.inc"
};
static NJS_COLOR event_chaos2_node46_uvs0[132] = {
#include "shared/chaos2/event_chaos2_node46_uvs0.inc"
};
static NJS_COLOR event_chaos2_node46_uvs1[68] = {
#include "shared/chaos2/event_chaos2_node46_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node46_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node46_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node46_points[39] = {
#include "shared/chaos2/event_chaos2_node46_points.inc"
};
static NJS_VECTOR event_chaos2_node46_normals[39] = {
#include "shared/chaos2/event_chaos2_node46_normals.inc"
};
static NJS_MODEL event_chaos2_node46_model = {
#include "shared/chaos2/event_chaos2_node46_model.inc"
};
static NJS_OBJECT event_chaos2_node46 = {
#include "shared/chaos2/event_chaos2_node46.inc"
};
static NJS_MATERIAL event_chaos2_node45_materials[1] = {
#include "shared/chaos2/event_chaos2_node45_materials.inc"
};
static Sint16 event_chaos2_node45_meshes0[24] = {
#include "shared/chaos2/event_chaos2_node45_meshes0.inc"
};
static NJS_COLOR event_chaos2_node45_uvs0[24] = {
#include "shared/chaos2/event_chaos2_node45_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node45_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node45_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node45_points[6] = {
#include "shared/chaos2/event_chaos2_node45_points.inc"
};
static NJS_VECTOR event_chaos2_node45_normals[6] = {
#include "shared/chaos2/event_chaos2_node45_normals.inc"
};
static NJS_MODEL event_chaos2_node45_model = {
#include "shared/chaos2/event_chaos2_node45_model.inc"
};
static NJS_OBJECT event_chaos2_node45 = {
#include "shared/chaos2/event_chaos2_node45.inc"
};
static NJS_MATERIAL event_chaos2_node44_materials[1] = {
#include "shared/chaos2/event_chaos2_node44_materials.inc"
};
static Sint16 event_chaos2_node44_meshes0[24] = {
#include "shared/chaos2/event_chaos2_node44_meshes0.inc"
};
static NJS_COLOR event_chaos2_node44_uvs0[24] = {
#include "shared/chaos2/event_chaos2_node44_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node44_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node44_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node44_points[6] = {
#include "shared/chaos2/event_chaos2_node44_points.inc"
};
static NJS_VECTOR event_chaos2_node44_normals[6] = {
#include "shared/chaos2/event_chaos2_node44_normals.inc"
};
static NJS_MODEL event_chaos2_node44_model = {
#include "shared/chaos2/event_chaos2_node44_model.inc"
};
static NJS_OBJECT event_chaos2_node44 = {
#include "shared/chaos2/event_chaos2_node44.inc"
};
static NJS_MATERIAL event_chaos2_node43_materials[1] = {
#include "shared/chaos2/event_chaos2_node43_materials.inc"
};
static Sint16 event_chaos2_node43_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node43_meshes0.inc"
};
static NJS_COLOR event_chaos2_node43_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node43_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node43_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node43_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node43_points[24] = {
#include "shared/chaos2/event_chaos2_node43_points.inc"
};
static NJS_VECTOR event_chaos2_node43_normals[24] = {
#include "shared/chaos2/event_chaos2_node43_normals.inc"
};
static NJS_MODEL event_chaos2_node43_model = {
#include "shared/chaos2/event_chaos2_node43_model.inc"
};
static NJS_OBJECT event_chaos2_node43 = {
#include "shared/chaos2/event_chaos2_node43.inc"
};
static NJS_MATERIAL event_chaos2_node42_materials[1] = {
#include "shared/chaos2/event_chaos2_node42_materials.inc"
};
static Sint16 event_chaos2_node42_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node42_meshes0.inc"
};
static NJS_COLOR event_chaos2_node42_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node42_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node42_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node42_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node42_points[24] = {
#include "shared/chaos2/event_chaos2_node42_points.inc"
};
static NJS_VECTOR event_chaos2_node42_normals[24] = {
#include "shared/chaos2/event_chaos2_node42_normals.inc"
};
static NJS_MODEL event_chaos2_node42_model = {
#include "shared/chaos2/event_chaos2_node42_model.inc"
};
static NJS_OBJECT event_chaos2_node42 = {
#include "shared/chaos2/event_chaos2_node42.inc"
};
static NJS_MATERIAL event_chaos2_node41_materials[1] = {
#include "shared/chaos2/event_chaos2_node41_materials.inc"
};
static Sint16 event_chaos2_node41_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node41_meshes0.inc"
};
static NJS_COLOR event_chaos2_node41_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node41_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node41_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node41_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node41_points[24] = {
#include "shared/chaos2/event_chaos2_node41_points.inc"
};
static NJS_VECTOR event_chaos2_node41_normals[24] = {
#include "shared/chaos2/event_chaos2_node41_normals.inc"
};
static NJS_MODEL event_chaos2_node41_model = {
#include "shared/chaos2/event_chaos2_node41_model.inc"
};
static NJS_OBJECT event_chaos2_node41 = {
#include "shared/chaos2/event_chaos2_node41.inc"
};
static NJS_MATERIAL event_chaos2_node40_materials[1] = {
#include "shared/chaos2/event_chaos2_node40_materials.inc"
};
static Sint16 event_chaos2_node40_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node40_meshes0.inc"
};
static NJS_COLOR event_chaos2_node40_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node40_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node40_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node40_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node40_points[24] = {
#include "shared/chaos2/event_chaos2_node40_points.inc"
};
static NJS_VECTOR event_chaos2_node40_normals[24] = {
#include "shared/chaos2/event_chaos2_node40_normals.inc"
};
static NJS_MODEL event_chaos2_node40_model = {
#include "shared/chaos2/event_chaos2_node40_model.inc"
};
static NJS_OBJECT event_chaos2_node40 = {
#include "shared/chaos2/event_chaos2_node40.inc"
};
static NJS_MATERIAL event_chaos2_node39_materials[1] = {
#include "shared/chaos2/event_chaos2_node39_materials.inc"
};
static Sint16 event_chaos2_node39_meshes0[57] = {
#include "shared/chaos2/event_chaos2_node39_meshes0.inc"
};
static Sint16 event_chaos2_node39_meshes1[60] = {
#include "shared/chaos2/event_chaos2_node39_meshes1.inc"
};
static NJS_COLOR event_chaos2_node39_uvs0[57] = {
#include "shared/chaos2/event_chaos2_node39_uvs0.inc"
};
static NJS_COLOR event_chaos2_node39_uvs1[60] = {
#include "shared/chaos2/event_chaos2_node39_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node39_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node39_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node39_points[28] = {
#include "shared/chaos2/event_chaos2_node39_points.inc"
};
static NJS_VECTOR event_chaos2_node39_normals[28] = {
#include "shared/chaos2/event_chaos2_node39_normals.inc"
};
static NJS_MODEL event_chaos2_node39_model = {
#include "shared/chaos2/event_chaos2_node39_model.inc"
};
static NJS_OBJECT event_chaos2_node39 = {
#include "shared/chaos2/event_chaos2_node39.inc"
};
static NJS_MATERIAL event_chaos2_node38_materials[1] = {
#include "shared/chaos2/event_chaos2_node38_materials.inc"
};
static Sint16 event_chaos2_node38_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node38_meshes0.inc"
};
static NJS_COLOR event_chaos2_node38_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node38_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node38_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node38_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node38_points[24] = {
#include "shared/chaos2/event_chaos2_node38_points.inc"
};
static NJS_VECTOR event_chaos2_node38_normals[24] = {
#include "shared/chaos2/event_chaos2_node38_normals.inc"
};
static NJS_MODEL event_chaos2_node38_model = {
#include "shared/chaos2/event_chaos2_node38_model.inc"
};
static NJS_OBJECT event_chaos2_node38 = {
#include "shared/chaos2/event_chaos2_node38.inc"
};
static NJS_MATERIAL event_chaos2_node37_materials[1] = {
#include "shared/chaos2/event_chaos2_node37_materials.inc"
};
static Sint16 event_chaos2_node37_meshes0[88] = {
#include "shared/chaos2/event_chaos2_node37_meshes0.inc"
};
static NJS_COLOR event_chaos2_node37_uvs0[88] = {
#include "shared/chaos2/event_chaos2_node37_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node37_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node37_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node37_points[24] = {
#include "shared/chaos2/event_chaos2_node37_points.inc"
};
static NJS_VECTOR event_chaos2_node37_normals[24] = {
#include "shared/chaos2/event_chaos2_node37_normals.inc"
};
static NJS_MODEL event_chaos2_node37_model = {
#include "shared/chaos2/event_chaos2_node37_model.inc"
};
static NJS_OBJECT event_chaos2_node37 = {
#include "shared/chaos2/event_chaos2_node37.inc"
};
static NJS_MATERIAL event_chaos2_node36_materials[1] = {
#include "shared/chaos2/event_chaos2_node36_materials.inc"
};
static Sint16 event_chaos2_node36_meshes0[9] = {
#include "shared/chaos2/event_chaos2_node36_meshes0.inc"
};
static Sint16 event_chaos2_node36_meshes1[24] = {
#include "shared/chaos2/event_chaos2_node36_meshes1.inc"
};
static NJS_COLOR event_chaos2_node36_uvs0[9] = {
#include "shared/chaos2/event_chaos2_node36_uvs0.inc"
};
static NJS_COLOR event_chaos2_node36_uvs1[24] = {
#include "shared/chaos2/event_chaos2_node36_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node36_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node36_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node36_points[10] = {
#include "shared/chaos2/event_chaos2_node36_points.inc"
};
static NJS_VECTOR event_chaos2_node36_normals[10] = {
#include "shared/chaos2/event_chaos2_node36_normals.inc"
};
static NJS_MODEL event_chaos2_node36_model = {
#include "shared/chaos2/event_chaos2_node36_model.inc"
};
static NJS_OBJECT event_chaos2_node36 = {
#include "shared/chaos2/event_chaos2_node36.inc"
};
static NJS_MATERIAL event_chaos2_node35_materials[1] = {
#include "shared/chaos2/event_chaos2_node35_materials.inc"
};
static Sint16 event_chaos2_node35_meshes0[80] = {
#include "shared/chaos2/event_chaos2_node35_meshes0.inc"
};
static NJS_COLOR event_chaos2_node35_uvs0[80] = {
#include "shared/chaos2/event_chaos2_node35_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node35_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node35_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node35_points[25] = {
#include "shared/chaos2/event_chaos2_node35_points.inc"
};
static NJS_VECTOR event_chaos2_node35_normals[25] = {
#include "shared/chaos2/event_chaos2_node35_normals.inc"
};
static NJS_MODEL event_chaos2_node35_model = {
#include "shared/chaos2/event_chaos2_node35_model.inc"
};
static NJS_OBJECT event_chaos2_node35 = {
#include "shared/chaos2/event_chaos2_node35.inc"
};
static NJS_OBJECT event_chaos2_node34 = {
#include "shared/chaos2/event_chaos2_node34.inc"
};
static NJS_OBJECT event_chaos2_node33 = {
#include "shared/chaos2/event_chaos2_node33.inc"
};
static NJS_OBJECT event_chaos2_node32 = {
#include "shared/chaos2/event_chaos2_node32.inc"
};
static NJS_OBJECT event_chaos2_node31 = {
#include "shared/chaos2/event_chaos2_node31.inc"
};
static NJS_OBJECT event_chaos2_node30 = {
#include "shared/chaos2/event_chaos2_node30.inc"
};
static NJS_MATERIAL event_chaos2_node29_materials[1] = {
#include "shared/chaos2/event_chaos2_node29_materials.inc"
};
static Sint16 event_chaos2_node29_meshes0[408] = {
#include "shared/chaos2/event_chaos2_node29_meshes0.inc"
};
static Sint16 event_chaos2_node29_meshes1[104] = {
#include "shared/chaos2/event_chaos2_node29_meshes1.inc"
};
static NJS_COLOR event_chaos2_node29_uvs0[408] = {
#include "shared/chaos2/event_chaos2_node29_uvs0.inc"
};
static NJS_COLOR event_chaos2_node29_uvs1[104] = {
#include "shared/chaos2/event_chaos2_node29_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node29_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node29_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node29_points[104] = {
#include "shared/chaos2/event_chaos2_node29_points.inc"
};
static NJS_VECTOR event_chaos2_node29_normals[104] = {
#include "shared/chaos2/event_chaos2_node29_normals.inc"
};
static NJS_MODEL event_chaos2_node29_model = {
#include "shared/chaos2/event_chaos2_node29_model.inc"
};
static NJS_OBJECT event_chaos2_node29 = {
#include "shared/chaos2/event_chaos2_node29.inc"
};
static NJS_OBJECT event_chaos2_node28 = {
#include "shared/chaos2/event_chaos2_node28.inc"
};
static NJS_OBJECT event_chaos2_node27 = {
#include "shared/chaos2/event_chaos2_node27.inc"
};
static NJS_OBJECT event_chaos2_node26 = {
#include "shared/chaos2/event_chaos2_node26.inc"
};
static NJS_MATERIAL event_chaos2_node25_materials[1] = {
#include "shared/chaos2/event_chaos2_node25_materials.inc"
};
static Sint16 event_chaos2_node25_meshes0[66] = {
#include "shared/chaos2/event_chaos2_node25_meshes0.inc"
};
static Sint16 event_chaos2_node25_meshes1[4] = {
#include "shared/chaos2/event_chaos2_node25_meshes1.inc"
};
static NJS_COLOR event_chaos2_node25_uvs0[66] = {
#include "shared/chaos2/event_chaos2_node25_uvs0.inc"
};
static NJS_COLOR event_chaos2_node25_uvs1[4] = {
#include "shared/chaos2/event_chaos2_node25_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node25_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node25_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node25_points[17] = {
#include "shared/chaos2/event_chaos2_node25_points.inc"
};
static NJS_VECTOR event_chaos2_node25_normals[17] = {
#include "shared/chaos2/event_chaos2_node25_normals.inc"
};
static NJS_MODEL event_chaos2_node25_model = {
#include "shared/chaos2/event_chaos2_node25_model.inc"
};
static NJS_OBJECT event_chaos2_node25 = {
#include "shared/chaos2/event_chaos2_node25.inc"
};
static NJS_MATERIAL event_chaos2_node24_materials[1] = {
#include "shared/chaos2/event_chaos2_node24_materials.inc"
};
static Sint16 event_chaos2_node24_meshes0[20] = {
#include "shared/chaos2/event_chaos2_node24_meshes0.inc"
};
static NJS_COLOR event_chaos2_node24_uvs0[20] = {
#include "shared/chaos2/event_chaos2_node24_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node24_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node24_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node24_points[10] = {
#include "shared/chaos2/event_chaos2_node24_points.inc"
};
static NJS_VECTOR event_chaos2_node24_normals[10] = {
#include "shared/chaos2/event_chaos2_node24_normals.inc"
};
static NJS_MODEL event_chaos2_node24_model = {
#include "shared/chaos2/event_chaos2_node24_model.inc"
};
static NJS_OBJECT event_chaos2_node24 = {
#include "shared/chaos2/event_chaos2_node24.inc"
};
static NJS_OBJECT event_chaos2_node23 = {
#include "shared/chaos2/event_chaos2_node23.inc"
};
static NJS_MATERIAL event_chaos2_node22_materials[1] = {
#include "shared/chaos2/event_chaos2_node22_materials.inc"
};
static Sint16 event_chaos2_node22_meshes0[48] = {
#include "shared/chaos2/event_chaos2_node22_meshes0.inc"
};
static Sint16 event_chaos2_node22_meshes1[24] = {
#include "shared/chaos2/event_chaos2_node22_meshes1.inc"
};
static NJS_COLOR event_chaos2_node22_uvs0[48] = {
#include "shared/chaos2/event_chaos2_node22_uvs0.inc"
};
static NJS_COLOR event_chaos2_node22_uvs1[24] = {
#include "shared/chaos2/event_chaos2_node22_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node22_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node22_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node22_points[17] = {
#include "shared/chaos2/event_chaos2_node22_points.inc"
};
static NJS_VECTOR event_chaos2_node22_normals[17] = {
#include "shared/chaos2/event_chaos2_node22_normals.inc"
};
static NJS_MODEL event_chaos2_node22_model = {
#include "shared/chaos2/event_chaos2_node22_model.inc"
};
static NJS_OBJECT event_chaos2_node22 = {
#include "shared/chaos2/event_chaos2_node22.inc"
};
static NJS_MATERIAL event_chaos2_node21_materials[1] = {
#include "shared/chaos2/event_chaos2_node21_materials.inc"
};
static Sint16 event_chaos2_node21_meshes0[15] = {
#include "shared/chaos2/event_chaos2_node21_meshes0.inc"
};
static Sint16 event_chaos2_node21_meshes1[12] = {
#include "shared/chaos2/event_chaos2_node21_meshes1.inc"
};
static NJS_COLOR event_chaos2_node21_uvs0[15] = {
#include "shared/chaos2/event_chaos2_node21_uvs0.inc"
};
static NJS_COLOR event_chaos2_node21_uvs1[12] = {
#include "shared/chaos2/event_chaos2_node21_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node21_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node21_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node21_points[10] = {
#include "shared/chaos2/event_chaos2_node21_points.inc"
};
static NJS_VECTOR event_chaos2_node21_normals[10] = {
#include "shared/chaos2/event_chaos2_node21_normals.inc"
};
static NJS_MODEL event_chaos2_node21_model = {
#include "shared/chaos2/event_chaos2_node21_model.inc"
};
static NJS_OBJECT event_chaos2_node21 = {
#include "shared/chaos2/event_chaos2_node21.inc"
};
static NJS_MATERIAL event_chaos2_node20_materials[1] = {
#include "shared/chaos2/event_chaos2_node20_materials.inc"
};
static Sint16 event_chaos2_node20_meshes0[60] = {
#include "shared/chaos2/event_chaos2_node20_meshes0.inc"
};
static NJS_COLOR event_chaos2_node20_uvs0[60] = {
#include "shared/chaos2/event_chaos2_node20_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node20_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node20_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node20_points[15] = {
#include "shared/chaos2/event_chaos2_node20_points.inc"
};
static NJS_VECTOR event_chaos2_node20_normals[15] = {
#include "shared/chaos2/event_chaos2_node20_normals.inc"
};
static NJS_MODEL event_chaos2_node20_model = {
#include "shared/chaos2/event_chaos2_node20_model.inc"
};
static NJS_OBJECT event_chaos2_node20 = {
#include "shared/chaos2/event_chaos2_node20.inc"
};
static NJS_OBJECT event_chaos2_node19 = {
#include "shared/chaos2/event_chaos2_node19.inc"
};
static NJS_OBJECT event_chaos2_node18 = {
#include "shared/chaos2/event_chaos2_node18.inc"
};
static NJS_OBJECT event_chaos2_node17 = {
#include "shared/chaos2/event_chaos2_node17.inc"
};
static NJS_OBJECT event_chaos2_node16 = {
#include "shared/chaos2/event_chaos2_node16.inc"
};
static NJS_OBJECT event_chaos2_node15 = {
#include "shared/chaos2/event_chaos2_node15.inc"
};
static NJS_MATERIAL event_chaos2_node14_materials[1] = {
#include "shared/chaos2/event_chaos2_node14_materials.inc"
};
static Sint16 event_chaos2_node14_meshes0[66] = {
#include "shared/chaos2/event_chaos2_node14_meshes0.inc"
};
static Sint16 event_chaos2_node14_meshes1[4] = {
#include "shared/chaos2/event_chaos2_node14_meshes1.inc"
};
static NJS_COLOR event_chaos2_node14_uvs0[66] = {
#include "shared/chaos2/event_chaos2_node14_uvs0.inc"
};
static NJS_COLOR event_chaos2_node14_uvs1[4] = {
#include "shared/chaos2/event_chaos2_node14_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node14_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node14_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node14_points[17] = {
#include "shared/chaos2/event_chaos2_node14_points.inc"
};
static NJS_VECTOR event_chaos2_node14_normals[17] = {
#include "shared/chaos2/event_chaos2_node14_normals.inc"
};
static NJS_MODEL event_chaos2_node14_model = {
#include "shared/chaos2/event_chaos2_node14_model.inc"
};
static NJS_OBJECT event_chaos2_node14 = {
#include "shared/chaos2/event_chaos2_node14.inc"
};
static NJS_MATERIAL event_chaos2_node13_materials[1] = {
#include "shared/chaos2/event_chaos2_node13_materials.inc"
};
static Sint16 event_chaos2_node13_meshes0[20] = {
#include "shared/chaos2/event_chaos2_node13_meshes0.inc"
};
static NJS_COLOR event_chaos2_node13_uvs0[20] = {
#include "shared/chaos2/event_chaos2_node13_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node13_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node13_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node13_points[10] = {
#include "shared/chaos2/event_chaos2_node13_points.inc"
};
static NJS_VECTOR event_chaos2_node13_normals[10] = {
#include "shared/chaos2/event_chaos2_node13_normals.inc"
};
static NJS_MODEL event_chaos2_node13_model = {
#include "shared/chaos2/event_chaos2_node13_model.inc"
};
static NJS_OBJECT event_chaos2_node13 = {
#include "shared/chaos2/event_chaos2_node13.inc"
};
static NJS_OBJECT event_chaos2_node12 = {
#include "shared/chaos2/event_chaos2_node12.inc"
};
static NJS_MATERIAL event_chaos2_node11_materials[1] = {
#include "shared/chaos2/event_chaos2_node11_materials.inc"
};
static Sint16 event_chaos2_node11_meshes0[48] = {
#include "shared/chaos2/event_chaos2_node11_meshes0.inc"
};
static Sint16 event_chaos2_node11_meshes1[24] = {
#include "shared/chaos2/event_chaos2_node11_meshes1.inc"
};
static NJS_COLOR event_chaos2_node11_uvs0[48] = {
#include "shared/chaos2/event_chaos2_node11_uvs0.inc"
};
static NJS_COLOR event_chaos2_node11_uvs1[24] = {
#include "shared/chaos2/event_chaos2_node11_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node11_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node11_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node11_points[17] = {
#include "shared/chaos2/event_chaos2_node11_points.inc"
};
static NJS_VECTOR event_chaos2_node11_normals[17] = {
#include "shared/chaos2/event_chaos2_node11_normals.inc"
};
static NJS_MODEL event_chaos2_node11_model = {
#include "shared/chaos2/event_chaos2_node11_model.inc"
};
static NJS_OBJECT event_chaos2_node11 = {
#include "shared/chaos2/event_chaos2_node11.inc"
};
static NJS_MATERIAL event_chaos2_node10_materials[1] = {
#include "shared/chaos2/event_chaos2_node10_materials.inc"
};
static Sint16 event_chaos2_node10_meshes0[15] = {
#include "shared/chaos2/event_chaos2_node10_meshes0.inc"
};
static Sint16 event_chaos2_node10_meshes1[12] = {
#include "shared/chaos2/event_chaos2_node10_meshes1.inc"
};
static NJS_COLOR event_chaos2_node10_uvs0[15] = {
#include "shared/chaos2/event_chaos2_node10_uvs0.inc"
};
static NJS_COLOR event_chaos2_node10_uvs1[12] = {
#include "shared/chaos2/event_chaos2_node10_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node10_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node10_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node10_points[10] = {
#include "shared/chaos2/event_chaos2_node10_points.inc"
};
static NJS_VECTOR event_chaos2_node10_normals[10] = {
#include "shared/chaos2/event_chaos2_node10_normals.inc"
};
static NJS_MODEL event_chaos2_node10_model = {
#include "shared/chaos2/event_chaos2_node10_model.inc"
};
static NJS_OBJECT event_chaos2_node10 = {
#include "shared/chaos2/event_chaos2_node10.inc"
};
static NJS_MATERIAL event_chaos2_node9_materials[1] = {
#include "shared/chaos2/event_chaos2_node9_materials.inc"
};
static Sint16 event_chaos2_node9_meshes0[60] = {
#include "shared/chaos2/event_chaos2_node9_meshes0.inc"
};
static NJS_COLOR event_chaos2_node9_uvs0[60] = {
#include "shared/chaos2/event_chaos2_node9_uvs0.inc"
};
static NJS_MESHSET event_chaos2_node9_meshsets[1] = {
#include "shared/chaos2/event_chaos2_node9_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node9_points[15] = {
#include "shared/chaos2/event_chaos2_node9_points.inc"
};
static NJS_VECTOR event_chaos2_node9_normals[15] = {
#include "shared/chaos2/event_chaos2_node9_normals.inc"
};
static NJS_MODEL event_chaos2_node9_model = {
#include "shared/chaos2/event_chaos2_node9_model.inc"
};
static NJS_OBJECT event_chaos2_node9 = {
#include "shared/chaos2/event_chaos2_node9.inc"
};
static NJS_OBJECT event_chaos2_node8 = {
#include "shared/chaos2/event_chaos2_node8.inc"
};
static NJS_OBJECT event_chaos2_node7 = {
#include "shared/chaos2/event_chaos2_node7.inc"
};
static NJS_OBJECT event_chaos2_node6 = {
#include "shared/chaos2/event_chaos2_node6.inc"
};
static NJS_OBJECT event_chaos2_node5 = {
#include "shared/chaos2/event_chaos2_node5.inc"
};
static NJS_OBJECT event_chaos2_node4 = {
#include "shared/chaos2/event_chaos2_node4.inc"
};
static NJS_MATERIAL event_chaos2_node3_materials[1] = {
#include "shared/chaos2/event_chaos2_node3_materials.inc"
};
static Sint16 event_chaos2_node3_meshes0[102] = {
#include "shared/chaos2/event_chaos2_node3_meshes0.inc"
};
static Sint16 event_chaos2_node3_meshes1[40] = {
#include "shared/chaos2/event_chaos2_node3_meshes1.inc"
};
static NJS_COLOR event_chaos2_node3_uvs0[102] = {
#include "shared/chaos2/event_chaos2_node3_uvs0.inc"
};
static NJS_COLOR event_chaos2_node3_uvs1[40] = {
#include "shared/chaos2/event_chaos2_node3_uvs1.inc"
};
static NJS_MESHSET event_chaos2_node3_meshsets[2] = {
#include "shared/chaos2/event_chaos2_node3_meshsets.inc"
};
static NJS_POINT3 event_chaos2_node3_points[34] = {
#include "shared/chaos2/event_chaos2_node3_points.inc"
};
static NJS_VECTOR event_chaos2_node3_normals[34] = {
#include "shared/chaos2/event_chaos2_node3_normals.inc"
};
static NJS_MODEL event_chaos2_node3_model = {
#include "shared/chaos2/event_chaos2_node3_model.inc"
};
static NJS_OBJECT event_chaos2_node3 = {
#include "shared/chaos2/event_chaos2_node3.inc"
};
static NJS_OBJECT event_chaos2_node2 = {
#include "shared/chaos2/event_chaos2_node2.inc"
};
static NJS_OBJECT event_chaos2_node1 = {
#include "shared/chaos2/event_chaos2_node1.inc"
};
static NJS_OBJECT event_chaos2_object = {
#include "shared/chaos2/event_chaos2_object.inc"
};

static NJS_MKEY_F event_chaos2_motion_pos0[32] = {
#include "shared/chaos2/event_chaos2_motion_pos0.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang5[32] = {
#include "shared/chaos2/event_chaos2_motion_ang5.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang6[32] = {
#include "shared/chaos2/event_chaos2_motion_ang6.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang7[32] = {
#include "shared/chaos2/event_chaos2_motion_ang7.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang16[32] = {
#include "shared/chaos2/event_chaos2_motion_ang16.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang17[32] = {
#include "shared/chaos2/event_chaos2_motion_ang17.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang18[32] = {
#include "shared/chaos2/event_chaos2_motion_ang18.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang27[32] = {
#include "shared/chaos2/event_chaos2_motion_ang27.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang30[2] = {
#include "shared/chaos2/event_chaos2_motion_ang30.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang31[32] = {
#include "shared/chaos2/event_chaos2_motion_ang31.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang32[32] = {
#include "shared/chaos2/event_chaos2_motion_ang32.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang33[32] = {
#include "shared/chaos2/event_chaos2_motion_ang33.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang39[32] = {
#include "shared/chaos2/event_chaos2_motion_ang39.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang52[32] = {
#include "shared/chaos2/event_chaos2_motion_ang52.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang53[32] = {
#include "shared/chaos2/event_chaos2_motion_ang53.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang54[32] = {
#include "shared/chaos2/event_chaos2_motion_ang54.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang60[32] = {
#include "shared/chaos2/event_chaos2_motion_ang60.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang66[2] = {
#include "shared/chaos2/event_chaos2_motion_ang66.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang72[32] = {
#include "shared/chaos2/event_chaos2_motion_ang72.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang73[32] = {
#include "shared/chaos2/event_chaos2_motion_ang73.inc"
};
static NJS_MKEY_A event_chaos2_motion_ang74[32] = {
#include "shared/chaos2/event_chaos2_motion_ang74.inc"
};
static NJS_MDATA2 event_chaos2_motion_mdata[92] = {
#include "shared/chaos2/event_chaos2_motion_mdata.inc"
};
static NJS_MOTION event_chaos2_motion = {
#include "shared/chaos2/event_chaos2_motion.inc"
};
static NJS_ACTION event_chaos2_action = {
#include "shared/chaos2/event_chaos2_action.inc"
};

/* the last Chaos 2 created. nothing clears it */
static struct task *event_chaos2_task = NULL;

/* each weld's vertex pairs: a vertex's index in object_a's model, then its
 * index in object_b's. a name gives the two nodes, object_a's first */
static Uint16 event_chaos2_indices_76_81[16] = {
#include "shared/chaos2/event_chaos2_indices_76_81.inc"
};
static Uint16 event_chaos2_indices_81_29[16] = {
#include "shared/chaos2/event_chaos2_indices_81_29.inc"
};
static Uint16 event_chaos2_indices_29_3[12] = {
#include "shared/chaos2/event_chaos2_indices_29_3.inc"
};
static Uint16 event_chaos2_indices_3_25[10] = {
#include "shared/chaos2/event_chaos2_indices_3_25.inc"
};
static Uint16 event_chaos2_indices_25_24[10] = {
#include "shared/chaos2/event_chaos2_indices_25_24.inc"
};
static Uint16 event_chaos2_indices_24_20[10] = {
#include "shared/chaos2/event_chaos2_indices_24_20.inc"
};
static Uint16 event_chaos2_indices_20_21[10] = {
#include "shared/chaos2/event_chaos2_indices_20_21.inc"
};
static Uint16 event_chaos2_indices_21_22[8] = {
#include "shared/chaos2/event_chaos2_indices_21_22.inc"
};
static Uint16 event_chaos2_indices_3_14[10] = {
#include "shared/chaos2/event_chaos2_indices_3_14.inc"
};
static Uint16 event_chaos2_indices_14_13[10] = {
#include "shared/chaos2/event_chaos2_indices_14_13.inc"
};
static Uint16 event_chaos2_indices_13_9[10] = {
#include "shared/chaos2/event_chaos2_indices_13_9.inc"
};
static Uint16 event_chaos2_indices_9_10[10] = {
#include "shared/chaos2/event_chaos2_indices_9_10.inc"
};
static Uint16 event_chaos2_indices_10_11[8] = {
#include "shared/chaos2/event_chaos2_indices_10_11.inc"
};
static Uint16 event_chaos2_indices_29_71[10] = {
#include "shared/chaos2/event_chaos2_indices_29_71.inc"
};
static Uint16 event_chaos2_indices_71_69[10] = {
#include "shared/chaos2/event_chaos2_indices_71_69.inc"
};
static Uint16 event_chaos2_indices_69_56[10] = {
#include "shared/chaos2/event_chaos2_indices_69_56.inc"
};
static Uint16 event_chaos2_indices_56_60[10] = {
#include "shared/chaos2/event_chaos2_indices_56_60.inc"
};
static Uint16 event_chaos2_indices_29_50[10] = {
#include "shared/chaos2/event_chaos2_indices_29_50.inc"
};
static Uint16 event_chaos2_indices_50_48[10] = {
#include "shared/chaos2/event_chaos2_indices_50_48.inc"
};
static Uint16 event_chaos2_indices_48_35[10] = {
#include "shared/chaos2/event_chaos2_indices_48_35.inc"
};
static Uint16 event_chaos2_indices_35_39[10] = {
#include "shared/chaos2/event_chaos2_indices_35_39.inc"
};

static struct weld event_chaos2_welds[22] = {
    {&event_chaos2_object, &event_chaos2_node76, &event_chaos2_node81, 8,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_76_81},
    {&event_chaos2_object, &event_chaos2_node81, &event_chaos2_node29, 8,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_81_29},
    {&event_chaos2_object, &event_chaos2_node29, &event_chaos2_node3, 6,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_29_3},
    {&event_chaos2_object, &event_chaos2_node3, &event_chaos2_node25, 5,
     WELD_TO_B, 0, 0, NULL, event_chaos2_indices_3_25},
    {&event_chaos2_object, &event_chaos2_node25, &event_chaos2_node24, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_25_24},
    {&event_chaos2_object, &event_chaos2_node24, &event_chaos2_node20, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_24_20},
    {&event_chaos2_object, &event_chaos2_node20, &event_chaos2_node21, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_20_21},
    {&event_chaos2_object, &event_chaos2_node21, &event_chaos2_node22, 4,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_21_22},
    {&event_chaos2_object, &event_chaos2_node3, &event_chaos2_node14, 5,
     WELD_TO_B, 0, 0, NULL, event_chaos2_indices_3_14},
    {&event_chaos2_object, &event_chaos2_node14, &event_chaos2_node13, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_14_13},
    {&event_chaos2_object, &event_chaos2_node13, &event_chaos2_node9, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_13_9},
    {&event_chaos2_object, &event_chaos2_node9, &event_chaos2_node10, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_9_10},
    {&event_chaos2_object, &event_chaos2_node10, &event_chaos2_node11, 4,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_10_11},
    {&event_chaos2_object, &event_chaos2_node29, &event_chaos2_node71, 5,
     WELD_TO_B, 0, 0, NULL, event_chaos2_indices_29_71},
    {&event_chaos2_object, &event_chaos2_node71, &event_chaos2_node69, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_71_69},
    {&event_chaos2_object, &event_chaos2_node69, &event_chaos2_node56, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_69_56},
    {&event_chaos2_object, &event_chaos2_node56, &event_chaos2_node60, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_56_60},
    {&event_chaos2_object, &event_chaos2_node29, &event_chaos2_node50, 5,
     WELD_TO_B, 0, 0, NULL, event_chaos2_indices_29_50},
    {&event_chaos2_object, &event_chaos2_node50, &event_chaos2_node48, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_50_48},
    {&event_chaos2_object, &event_chaos2_node48, &event_chaos2_node35, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_48_35},
    {&event_chaos2_object, &event_chaos2_node35, &event_chaos2_node39, 5,
     WELD_TO_A, 0, 0, NULL, event_chaos2_indices_35_39},
    {NULL, NULL, NULL, 0, 0, 0, 0, NULL, NULL},
};

static void event_chaos2_init(struct task *task);
static void event_chaos2_step(struct task *task);
static void event_chaos2_draw(struct task *task);
static void event_chaos2_destroy(struct task *task);

void event_chaos2_load_textures(void)
{
    njLoadTexture(&event_chaos2_texlist);
    njLoadTexture(&event_chaos0_water_texlist);
}

void event_chaos2_release_textures(void)
{
    njReleaseTexture(&event_chaos2_texlist);
    njReleaseTexture(&event_chaos0_water_texlist);
}

void event_chaos2_create(Float x, Float y, Float z, Angle ang_x, Angle ang_y,
                         Angle ang_z, int steps)
{
    struct chaos2_state *state;
    struct task_work *work;

    event_chaos2_task = task_create(TASK_WORK, 5, event_chaos2_init);
    state = heap_alloc(sizeof(struct chaos2_state));
    work = event_chaos2_task->work;
    VARS(work)->state = state;
    work->mode = CHAOS2_STAND;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->ang.x = ang_x;
    work->ang.y = ang_y;
    work->ang.z = ang_z;
    state->frame = 0.0F;
    state->action_speed = 0.1F;
    state->form_steps = steps;
    state->lift = 0.0F;
    state->lift_speed = CHAOS2_DEPTH / state->form_steps;
    state->puddle_scale.x = 4.0F;
    state->puddle_scale.y = 2.0F;
    state->puddle_scale.z = 4.0F;
    state->form_speed.x = 0.2F / state->form_steps;
    state->form_speed.y = 1.0F / state->form_steps;
    state->form_speed.z = 0.2F / state->form_steps;
    state->form.x = 0.8F;
    state->form.y = 0.01F;
    state->form.z = 0.8F;
    state->alpha = 0.05F;
    state->alpha_speed = 0.5F / state->form_steps;
    state->wobble = 0;
    event_weld(event_chaos2_welds, &event_chaos2_action, state->frame,
               CHAOS2_NODES, PLAYER_WELD_MAKE);
}

void event_chaos2_kill(void)
{
    if (event_chaos2_task != NULL) {
        task_kill(event_chaos2_task);
    }
}

void event_chaos2_melt(void)
{
    struct task_work *work = event_chaos2_task->work;

    VARS(work)->timer = 0;
    work->mode = CHAOS2_MELT;
}

void event_chaos2_rise(void)
{
    struct task_work *work = event_chaos2_task->work;

    VARS(work)->timer = 0;
    work->mode = CHAOS2_RISE;
}

void event_chaos2_set_speed(Float speed)
{
    struct task_work *work = event_chaos2_task->work;
    struct chaos2_state *state = VARS(work)->state;

    state->action_speed = speed;
}

void event_chaos2_set_pos(Float x, Float y, Float z)
{
    struct task_work *work = event_chaos2_task->work;

    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
}

void event_chaos2_set_ang(Angle x, Angle y, Angle z)
{
    struct task_work *work = event_chaos2_task->work;

    work->ang.x = x;
    work->ang.y = y;
    work->ang.z = z;
}

void event_chaos2_slide(Float x, Float y, Float z, int steps)
{
    struct task_work *work = event_chaos2_task->work;
    struct chaos2_state *state = VARS(work)->state;

    state->to.x = x;
    state->to.y = y;
    state->to.z = z;
    state->speed.x = (x - work->pos.x) / steps;
    state->speed.y = (y - work->pos.y) / steps;
    state->speed.z = (z - work->pos.z) / steps;
    state->steps = steps;
    work->mode = CHAOS2_SLIDE;
    VARS(work)->timer = 0;
    state->frame = 0.0F;
}

void event_chaos2_puddle(void)
{
    struct task_work *work = event_chaos2_task->work;
    struct chaos2_state *state = VARS(work)->state;

    state->lift = -CHAOS2_DEPTH;
    state->form.x = 1.0F;
    state->form.y = 1.0F;
    state->form.z = 1.0F;
    state->alpha = 0.6F;
    work->mode = CHAOS2_PUDDLE;
}

static void event_chaos2_init(struct task *task)
{
    task->step = event_chaos2_step;
    task->draw = event_chaos2_draw;
    task->destructor = event_chaos2_destroy;
}

static void event_chaos2_step(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos2_state *state = VARS(work)->state;
    /* @hack spelling: the action through a local, which SHC keeps in R12
     * through the step (MOV.L at 0x0cb82842 in EV000B); without it, the code
     * runs 4 bytes short and 249 of the step's 256 words differ */
    NJS_ACTION *action = &event_chaos2_action;

    event_weld(event_chaos2_welds, action, state->frame, CHAOS2_NODES,
               PLAYER_WELDING);
    switch (work->mode) {
    case CHAOS2_STAND:
        state->frame += state->action_speed;
        if (state->frame >= action->motion->nbFrame) {
            state->frame = 0.0F;
        }
        break;
    case CHAOS2_MELT:
        state->frame += state->action_speed;
        if (state->frame >= action->motion->nbFrame) {
            state->frame = 0.0F;
        }
        if (++VARS(work)->timer > state->form_steps) {
            work->mode = CHAOS2_PUDDLE;
            VARS(work)->timer = 0;
            state->form.x = 1.0F;
            state->form.y = 1.0F;
            state->form.z = 1.0F;
        }
        state->lift -= state->lift_speed;
        state->form.x += state->form_speed.x;
        state->form.y += state->form_speed.y;
        state->form.z += state->form_speed.z;
        state->alpha += state->alpha_speed;
        break;
    case CHAOS2_RISE:
        state->frame += state->action_speed;
        if (state->frame >= action->motion->nbFrame) {
            state->frame = 0.0F;
        }
        if (++VARS(work)->timer > state->form_steps) {
            work->mode = CHAOS2_STAND;
            VARS(work)->timer = 0;
            state->form.x = 0.8F;
            state->form.y = 0.01F;
            state->form.z = 0.8F;
            state->lift = 0.0F;
        }
        state->lift += state->lift_speed;
        state->form.x -= state->form_speed.x;
        state->form.y -= state->form_speed.y;
        state->form.z -= state->form_speed.z;
        state->alpha -= state->alpha_speed;
        break;
    case CHAOS2_PUDDLE:
        state->wobble += NJM_DEG_ANG(1);
        break;
    case CHAOS2_SLIDE:
        work->pos.x += state->speed.x;
        work->pos.y += state->speed.y;
        work->pos.z += state->speed.z;
        if (++VARS(work)->timer > state->steps) {
            work->mode = CHAOS2_PUDDLE;
            state->frame = 0.0F;
        }
        state->wobble += NJM_DEG_ANG(2);
        break;
    }
    event_chaos2_draw(task);
}

static void event_chaos2_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos2_state *state = VARS(work)->state;
    /* @hack redundant: 24 bytes of frame that no code reads, of a type unknown,
     * declared as two vectors. the image's ADD #-28,R15 reserves them
     * (0x0cb82ac6 in EV000B); without them, 2 of 336 words differ */
    NJS_VECTOR unused[2];

    draw_set_palette(DRAW_PLAYER_PALETTE);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    njTranslate(NULL, 0.0F, 2.0F, 0.0F);
    switch (work->mode) {
    case CHAOS2_STAND:
        njSetTexture(&event_chaos2_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + CHAOS2_BODY_HEIGHT, 0.0F);
        draw_action(&event_chaos2_action, state->frame);
        njPopMatrix(1);
        break;
    case CHAOS2_MELT:
        njSetTexture(&event_chaos2_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + CHAOS2_BODY_HEIGHT, 0.0F);
        draw_action(&event_chaos2_action, state->frame);
        njPopMatrix(1);
        njSetTexture(&event_chaos0_water_texlist);
        njPushMatrix(NULL);
        njScaleV(NULL, &state->form);
        njScaleV(NULL, &state->puddle_scale);
        draw_tree(&event_chaos0_puddle_object);
        njPopMatrix(1);
        break;
    case CHAOS2_RISE:
        njSetTexture(&event_chaos2_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, state->lift + CHAOS2_BODY_HEIGHT, 0.0F);
        draw_action(&event_chaos2_action, state->frame);
        njPopMatrix(1);
        njSetTexture(&event_chaos0_water_texlist);
        draw_attr_save();
        draw_attr_set(0, CHAOS2_ATTR);
        draw_set_material(state->alpha - 1.0F, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njPushMatrix(NULL);
        njScaleV(NULL, &state->form);
        njScaleV(NULL, &state->puddle_scale);
        draw_tree(&event_chaos0_puddle_object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        break;
    case CHAOS2_PUDDLE:
        njSetTexture(&event_chaos0_water_texlist);
        draw_attr_save();
        draw_attr_set(0, CHAOS2_ATTR);
        draw_set_material(-0.5F, 0.0F, 0.0F, 0.0F);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njPushMatrix(NULL);
        njScaleV(NULL, &state->puddle_scale);
        njScale(NULL, 1.0F + (CHAOS2_SWAY * njSin(state->wobble)), 1.0F,
                1.0F + (CHAOS2_SWAY * njCos(state->wobble)));
        draw_tree(&event_chaos0_puddle_object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        draw_attr_restore();
        break;
    case CHAOS2_SLIDE:
        njSetTexture(&event_chaos0_water_texlist);
        njPushMatrix(NULL);
        njScaleV(NULL, &state->puddle_scale);
        njScale(NULL, 1.0F + (CHAOS2_SWAY * njSin(state->wobble)), 1.0F,
                1.0F + (CHAOS2_SWAY * njCos(state->wobble)));
        draw_tree(&event_chaos0_puddle_object);
        njPopMatrix(1);
        break;
    }
    njPopMatrix(1);
    draw_set_palette(0);
}

static void event_chaos2_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct chaos2_state *state = VARS(work)->state;

    /* @bug memory: event_weld's frame is read from state before the test of
     * state (FMOV.S at 0x0cb82d9c, TST at 0x0cb82da6 in EV000B). state is
     * null only when heap_alloc failed in event_chaos2_create, and is then
     * read through */
    event_weld(event_chaos2_welds, &event_chaos2_action, state->frame,
               CHAOS2_NODES, PLAYER_WELD_FREE);
    if (VARS(work)->state != NULL) {
        heap_free(VARS(work)->state);
    }
}
