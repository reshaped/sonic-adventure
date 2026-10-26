/* EV00FA.PRS's splash beside the Tornado 2: ripples, a column of water and
 * rings of spray */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "draw/tree.h"
#include "ev00fa/splash.h"
#include "system/task.h"

#define TEXT_1_LENGTH 6
#define TEXT_2_LENGTH 6
#define TEXT_3_LENGTH 6
#include "ev00fa/splash/text.inc"

/* the modes of the ripples', a ring's and the splash's work. a step in
 * SPLASH_END kills its task */
enum splash_mode { SPLASH_START, SPLASH_PLAY, SPLASH_END };

enum column_mode {
    COLUMN_START,
    COLUMN_GROW, /* widens by scl_step each step, to full width */
    COLUMN_HOLD,
    COLUMN_SHRINK, /* narrows by scl_step each step, to COLUMN_THIN */
    COLUMN_END
};

/* the steps the column takes to widen at speed 1 */
#define COLUMN_GROW_STEPS 15.0F

/* the column's scale across, x and z, when it starts, and its scale on
 * every axis when it ends */
#define COLUMN_THIN 0.01F

/* the column holds its full width until its count passes this */
#define COLUMN_HOLD_COUNT 75

/* the column's record, which ev00fa_splash_column_create takes from the
 * heap and ev00fa_splash_column_destroy frees */
struct splash_column {
    /* the splash's scale. the draw multiplies it by work->scl */
    NJS_VECTOR scale;
    /* what COLUMN_GROW adds to work->scl each step, and COLUMN_SHRINK takes
     * off. it is 0 in y */
    NJS_VECTOR scl_step;
    /* the column's step changes it, but its draw never reads it */
    NJS_ARGB color;
    /* what COLUMN_GROW adds to color each step, and COLUMN_SHRINK takes off */
    NJS_ARGB color_step;
    Sint16 scroll; /* the outer cylinder's uv shift each step */
};

/* the ripples' and a ring's view of work->vars */
struct splash_play_vars {
    Float frame;
    Float speed; /* the motion frames added to frame each step */
};

/* the column's and the splash's view of work->vars */
struct splash_count_vars {
    /* the splash's steps after its first, or the column's in COLUMN_HOLD */
    Uint32 count;
    Float speed; /* the splash's speed, as ev00fa_splash_create takes it */
    struct splash_column *column; /* the column's record. null for the splash */
};

#define PLAY(work) ((struct splash_play_vars *)&(work)->vars)
#define COUNT(work) ((struct splash_count_vars *)&(work)->vars)

/* how many steps the splash takes to play the given motion frames at its
 * speed: frames / speed, truncated */
#define FRAMES_TO_STEPS(work, frames)                                          \
    ((Uint32)(int)((frames) / COUNT(work)->speed))

/* the splash's schedule, in motion frames */
#define COLUMN_FRAME 8.0F
#define FIRST_RING_FRAME 20.0F
#define RING_PERIOD 5.0F
#define END_FRAME 60.0F

static void ev00fa_splash_ripples_init(struct task *task);
static void ev00fa_splash_ripples_step(struct task *task);
static void ev00fa_splash_ripples_draw(struct task *task);
static void ev00fa_splash_ripples_destroy(struct task *task);
static void ev00fa_splash_column_init(struct task *task);
static void ev00fa_splash_column_step(struct task *task);
static void ev00fa_splash_column_draw(struct task *task);
static void ev00fa_splash_column_destroy(struct task *task);
static void ev00fa_splash_ring_init(struct task *task);
static void ev00fa_splash_ring_step(struct task *task);
static void ev00fa_splash_ring_draw(struct task *task);
static void ev00fa_splash_ring_destroy(struct task *task);
static void ev00fa_splash_init(struct task *task);
static void ev00fa_splash_step(struct task *task);
static void ev00fa_splash_draw(struct task *task);
static void ev00fa_splash_destroy(struct task *task);

/* the splash's texture names, whose textures the event loads from L_SIBUKI */
static NJS_TEXNAME ev00fa_splash_textures[3] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
};
NJS_TEXLIST ev00fa_splash_texlist = {
#include "ev00fa/splash/ev00fa_splash_texlist.inc"
};

static NJS_MATERIAL ev00fa_splash_ripples_3_materials[1] = {
#include "ev00fa/splash/ev00fa_splash_ripples_3_materials.inc"
};
static Sint16 ev00fa_splash_ripples_3_meshes0[32] = {
#include "ev00fa/splash/ev00fa_splash_ripples_3_meshes0.inc"
};
static NJS_COLOR ev00fa_splash_ripples_3_uvs0[32] = {
#include "ev00fa/splash/ev00fa_splash_ripples_3_uvs0.inc"
};
static NJS_MESHSET ev00fa_splash_ripples_3_meshsets[1] = {
#include "ev00fa/splash/ev00fa_splash_ripples_3_meshsets.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_3_points[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_3_points.inc"
};
static NJS_VECTOR ev00fa_splash_ripples_3_normals[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_3_normals.inc"
};
static NJS_MODEL ev00fa_splash_ripples_3_model = {
#include "ev00fa/splash/ev00fa_splash_ripples_3_model.inc"
};
static NJS_OBJECT ev00fa_splash_ripples_3 = {
#include "ev00fa/splash/ev00fa_splash_ripples_3.inc"
};
static NJS_MATERIAL ev00fa_splash_ripples_2_materials[1] = {
#include "ev00fa/splash/ev00fa_splash_ripples_2_materials.inc"
};
static Sint16 ev00fa_splash_ripples_2_meshes0[32] = {
#include "ev00fa/splash/ev00fa_splash_ripples_2_meshes0.inc"
};
static NJS_COLOR ev00fa_splash_ripples_2_uvs0[32] = {
#include "ev00fa/splash/ev00fa_splash_ripples_2_uvs0.inc"
};
static NJS_MESHSET ev00fa_splash_ripples_2_meshsets[1] = {
#include "ev00fa/splash/ev00fa_splash_ripples_2_meshsets.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_2_points[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_2_points.inc"
};
static NJS_VECTOR ev00fa_splash_ripples_2_normals[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_2_normals.inc"
};
static NJS_MODEL ev00fa_splash_ripples_2_model = {
#include "ev00fa/splash/ev00fa_splash_ripples_2_model.inc"
};
static NJS_OBJECT ev00fa_splash_ripples_2 = {
#include "ev00fa/splash/ev00fa_splash_ripples_2.inc"
};
static NJS_MATERIAL ev00fa_splash_ripples_1_materials[1] = {
#include "ev00fa/splash/ev00fa_splash_ripples_1_materials.inc"
};
static Sint16 ev00fa_splash_ripples_1_meshes0[32] = {
#include "ev00fa/splash/ev00fa_splash_ripples_1_meshes0.inc"
};
static NJS_COLOR ev00fa_splash_ripples_1_uvs0[32] = {
#include "ev00fa/splash/ev00fa_splash_ripples_1_uvs0.inc"
};
static NJS_MESHSET ev00fa_splash_ripples_1_meshsets[1] = {
#include "ev00fa/splash/ev00fa_splash_ripples_1_meshsets.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_1_points[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_1_points.inc"
};
static NJS_VECTOR ev00fa_splash_ripples_1_normals[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_1_normals.inc"
};
static NJS_MODEL ev00fa_splash_ripples_1_model = {
#include "ev00fa/splash/ev00fa_splash_ripples_1_model.inc"
};
static NJS_OBJECT ev00fa_splash_ripples_1 = {
#include "ev00fa/splash/ev00fa_splash_ripples_1.inc"
};
static NJS_OBJECT ev00fa_splash_ripples_object = {
#include "ev00fa/splash/ev00fa_splash_ripples_object.inc"
};
static NJS_MATERIAL ev00fa_splash_column_2_materials[1] = {
#include "ev00fa/splash/ev00fa_splash_column_2_materials.inc"
};
static Sint16 ev00fa_splash_column_2_meshes0[32] = {
#include "ev00fa/splash/ev00fa_splash_column_2_meshes0.inc"
};
static NJS_COLOR ev00fa_splash_column_2_uvs0[32] = {
#include "ev00fa/splash/ev00fa_splash_column_2_uvs0.inc"
};
static NJS_MESHSET ev00fa_splash_column_2_meshsets[1] = {
#include "ev00fa/splash/ev00fa_splash_column_2_meshsets.inc"
};
static NJS_POINT3 ev00fa_splash_column_2_points[16] = {
#include "ev00fa/splash/ev00fa_splash_column_2_points.inc"
};
static NJS_VECTOR ev00fa_splash_column_2_normals[16] = {
#include "ev00fa/splash/ev00fa_splash_column_2_normals.inc"
};
static NJS_MODEL ev00fa_splash_column_2_model = {
#include "ev00fa/splash/ev00fa_splash_column_2_model.inc"
};
static NJS_OBJECT ev00fa_splash_column_2 = {
#include "ev00fa/splash/ev00fa_splash_column_2.inc"
};
static NJS_MATERIAL ev00fa_splash_column_1_materials[1] = {
#include "ev00fa/splash/ev00fa_splash_column_1_materials.inc"
};
static Sint16 ev00fa_splash_column_1_meshes0[32] = {
#include "ev00fa/splash/ev00fa_splash_column_1_meshes0.inc"
};
static NJS_COLOR ev00fa_splash_column_1_uvs0[32] = {
#include "ev00fa/splash/ev00fa_splash_column_1_uvs0.inc"
};
static NJS_MESHSET ev00fa_splash_column_1_meshsets[1] = {
#include "ev00fa/splash/ev00fa_splash_column_1_meshsets.inc"
};
static NJS_POINT3 ev00fa_splash_column_1_points[16] = {
#include "ev00fa/splash/ev00fa_splash_column_1_points.inc"
};
static NJS_VECTOR ev00fa_splash_column_1_normals[16] = {
#include "ev00fa/splash/ev00fa_splash_column_1_normals.inc"
};
static NJS_MODEL ev00fa_splash_column_1_model = {
#include "ev00fa/splash/ev00fa_splash_column_1_model.inc"
};
static NJS_OBJECT ev00fa_splash_column_1 = {
#include "ev00fa/splash/ev00fa_splash_column_1.inc"
};
/* @hack type: an array of one. ev00fa_splash_column_create then loads
 * ev00fa_splash_column_copy's address into R3 ahead of its last stores to the
 * column's record (0x0cb80e76). as one NJS_OBJECT, that load comes after them,
 * into R2, and 36 of the function's 138 words differ */
static NJS_OBJECT ev00fa_splash_column_object[1] = {
#include "ev00fa/splash/ev00fa_splash_column_object.inc"
};
static NJS_MATERIAL ev00fa_splash_ring_materials[1] = {
#include "ev00fa/splash/ev00fa_splash_ring_materials.inc"
};
static Sint16 ev00fa_splash_ring_meshes0[32] = {
#include "ev00fa/splash/ev00fa_splash_ring_meshes0.inc"
};
static NJS_COLOR ev00fa_splash_ring_uvs0[32] = {
#include "ev00fa/splash/ev00fa_splash_ring_uvs0.inc"
};
static NJS_MESHSET ev00fa_splash_ring_meshsets[1] = {
#include "ev00fa/splash/ev00fa_splash_ring_meshsets.inc"
};
static NJS_POINT3 ev00fa_splash_ring_points[16] = {
#include "ev00fa/splash/ev00fa_splash_ring_points.inc"
};
static NJS_VECTOR ev00fa_splash_ring_normals[16] = {
#include "ev00fa/splash/ev00fa_splash_ring_normals.inc"
};
static NJS_MODEL ev00fa_splash_ring_model = {
#include "ev00fa/splash/ev00fa_splash_ring_model.inc"
};
static NJS_OBJECT ev00fa_splash_ring_object = {
#include "ev00fa/splash/ev00fa_splash_ring_object.inc"
};
static NJS_MDATA3 ev00fa_splash_ripples_mdata[4] = {
#include "ev00fa/splash/ev00fa_splash_ripples_mdata.inc"
};
static NJS_MOTION ev00fa_splash_ripples_motion = {
#include "ev00fa/splash/ev00fa_splash_ripples_motion.inc"
};
/* @hack type: an array of one. ev00fa_splash_ripples_step then pools its
 * address and reads the motion 4 bytes past it (0x0cb80d38). as one NJS_ACTION,
 * the pool word holds the address of its motion field, and 3 of the step's 51
 * words differ */
static NJS_ACTION ev00fa_splash_ripples_action[1] = {
#include "ev00fa/splash/ev00fa_splash_ripples_action.inc"
};
static NJS_MKEY_F ev00fa_splash_ring_pos0[2] = {
#include "ev00fa/splash/ev00fa_splash_ring_pos0.inc"
};
static NJS_MKEY_A ev00fa_splash_ring_ang0[2] = {
#include "ev00fa/splash/ev00fa_splash_ring_ang0.inc"
};
static NJS_MKEY_F ev00fa_splash_ring_scl0[31] = {
#include "ev00fa/splash/ev00fa_splash_ring_scl0.inc"
};
static NJS_MDATA3 ev00fa_splash_ring_mdata[1] = {
#include "ev00fa/splash/ev00fa_splash_ring_mdata.inc"
};
static NJS_MOTION ev00fa_splash_ring_motion = {
#include "ev00fa/splash/ev00fa_splash_ring_motion.inc"
};
/* @hack type: an array of one. SHC then pools its address once for both uses:
 * ev00fa_splash_ring_step reads the motion 4 bytes past it (0x0cb812e6), and
 * ev00fa_splash_ring_draw passes it (0x0cb81342). as one NJS_ACTION, the step's
 * pool word holds the address of its motion field, the pool grows a word, and 6
 * of the two functions' 78 words differ */
static NJS_ACTION ev00fa_splash_ring_action[1] = {
#include "ev00fa/splash/ev00fa_splash_ring_action.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_0[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_0.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_0[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_0.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_1[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_1.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_1[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_1.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_2[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_2.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_2[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_2.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_3[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_3.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_3[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_3.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_4[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_4.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_4[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_4.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_5[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_5.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_5[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_5.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_6[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_6.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_6[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_6.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_7[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_7.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_7[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_7.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_8[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_8.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_8[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_8.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_9[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_9.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_9[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_9.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_10[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_10.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_10[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_10.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_11[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_11.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_11[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_11.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_12[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_12.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_12[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_12.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_13[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_13.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_13[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_13.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_14[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_14.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_14[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_14.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_15[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_15.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_15[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_15.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_16[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_16.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_16[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_16.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_17[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_17.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_17[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_17.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_18[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_18.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_18[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_18.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_19[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_19.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_19[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_19.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_20[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_20.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_20[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_20.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_21[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_21.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_21[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_21.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_22[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_22.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_22[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_22.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_23[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_23.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_23[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_23.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_24[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_24.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_24[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_24.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_25[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_25.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_25[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_25.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_26[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_26.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_26[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_26.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_27[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_27.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_27[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_27.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points1_59[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1_59.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals1_59[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1_59.inc"
};
static NJS_MKEY_P ev00fa_splash_ripples_shape_points1[29] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points1.inc"
};
static NJS_MKEY_P ev00fa_splash_ripples_shape_normals1[29] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals1.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_0[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_0.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_0[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_0.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_1[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_1.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_1[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_1.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_2[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_2.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_2[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_2.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_3[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_3.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_3[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_3.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_4[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_4.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_4[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_4.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_5[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_5.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_5[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_5.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_6[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_6.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_6[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_6.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_7[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_7.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_7[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_7.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_8[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_8.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_8[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_8.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_9[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_9.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_9[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_9.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_10[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_10.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_10[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_10.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_11[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_11.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_11[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_11.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_12[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_12.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_12[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_12.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_13[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_13.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_13[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_13.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_14[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_14.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_14[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_14.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_15[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_15.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_15[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_15.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_16[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_16.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_16[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_16.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_17[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_17.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_17[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_17.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_18[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_18.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_18[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_18.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_19[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_19.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_19[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_19.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_20[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_20.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_20[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_20.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_21[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_21.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_21[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_21.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_22[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_22.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_22[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_22.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_23[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_23.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_23[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_23.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points2_59[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2_59.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals2_59[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2_59.inc"
};
static NJS_MKEY_P ev00fa_splash_ripples_shape_points2[25] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points2.inc"
};
static NJS_MKEY_P ev00fa_splash_ripples_shape_normals2[25] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals2.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_0[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_0.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_0[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_0.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_1[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_1.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_1[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_1.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_2[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_2.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_2[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_2.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_3[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_3.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_3[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_3.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_4[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_4.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_4[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_4.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_5[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_5.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_5[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_5.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_6[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_6.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_6[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_6.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_7[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_7.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_7[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_7.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_8[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_8.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_8[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_8.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_9[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_9.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_9[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_9.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_10[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_10.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_10[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_10.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_11[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_11.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_11[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_11.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_12[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_12.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_12[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_12.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_13[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_13.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_13[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_13.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_14[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_14.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_14[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_14.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_15[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_15.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_15[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_15.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_16[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_16.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_16[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_16.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_17[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_17.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_17[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_17.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_18[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_18.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_18[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_18.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_19[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_19.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_19[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_19.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_20[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_20.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_20[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_20.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_21[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_21.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_21[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_21.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_22[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_22.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_22[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_22.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_23[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_23.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_23[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_23.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_24[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_24.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_24[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_24.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_25[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_25.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_25[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_25.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_26[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_26.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_26[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_26.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_27[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_27.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_27[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_27.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_28[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_28.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_28[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_28.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_29[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_29.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_29[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_29.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_30[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_30.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_30[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_30.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_31[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_31.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_31[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_31.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_points3_59[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3_59.inc"
};
static NJS_POINT3 ev00fa_splash_ripples_shape_normals3_59[16] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3_59.inc"
};
static NJS_MKEY_P ev00fa_splash_ripples_shape_points3[33] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_points3.inc"
};
static NJS_MKEY_P ev00fa_splash_ripples_shape_normals3[33] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_normals3.inc"
};
static NJS_MDATA2 ev00fa_splash_ripples_shape_mdata[4] = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape_mdata.inc"
};
static NJS_MOTION ev00fa_splash_ripples_shape = {
#include "ev00fa/splash/ev00fa_splash_ripples_shape.inc"
};

/* the last ripples created, or null once a ripples task ends */
static struct task *ev00fa_splash_ripples_task = NULL;
/* the last column created, or null once a column task ends.
 * ev00fa_splash_column_scroll reads its record through this slot */
static struct task *ev00fa_splash_column_task = NULL;
/* the last splash created. a splash that ends leaves the slot pointing at
 * its freed task */
static struct task *ev00fa_splash_task = NULL;

/* a copy of the column object's root, which each column's create takes and
 * the column draws. it shares the root's children, so it draws the uvs
 * ev00fa_splash_column_scroll shifts */
static NJS_OBJECT ev00fa_splash_column_copy;

static void ev00fa_splash_ripples_create(Float x, Float y, Float z, Float speed,
                                         Float scale)
{
    struct task_work *work;

    ev00fa_splash_ripples_task =
        task_create(TASK_WORK, 5, ev00fa_splash_ripples_init);
    work = ev00fa_splash_ripples_task->work;
    work->mode = SPLASH_START;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->scl.x = scale;
    work->scl.y = scale;
    work->scl.z = scale;
    PLAY(work)->speed = speed;
    PLAY(work)->frame = 0.0F;
}

static void ev00fa_splash_ripples_init(struct task *task)
{
    task->step = ev00fa_splash_ripples_step;
    task->draw = ev00fa_splash_ripples_draw;
    task->destructor = ev00fa_splash_ripples_destroy;
}

static void ev00fa_splash_ripples_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SPLASH_START:
        work->mode = SPLASH_PLAY;
        break;
    case SPLASH_PLAY:
        PLAY(work)->frame += PLAY(work)->speed;
        if (PLAY(work)->frame >=
            ev00fa_splash_ripples_action->motion->nbFrame) {
            work->mode = SPLASH_END;
        }
        break;
    case SPLASH_END:
        if (task != NULL) {
            task_kill(task);
            ev00fa_splash_ripples_task = NULL;
        }
        break;
    }
    ev00fa_splash_ripples_draw(task);
}

static void ev00fa_splash_ripples_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&ev00fa_splash_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njScaleV(NULL, &work->scl);
    draw_shape_motion(&ev00fa_splash_ripples_object,
                      &ev00fa_splash_ripples_motion,
                      &ev00fa_splash_ripples_shape, PLAY(work)->frame);
    njPopMatrix(1);
}

static void ev00fa_splash_ripples_destroy(struct task *task)
{
}

static void ev00fa_splash_column_create(Float x, Float y, Float z, Float speed,
                                        Float scale)
{
    struct splash_column *column;
    struct task_work *work;

    /* @hack redundant: a store of y to x that never runs. SHC then keeps x,
     * y, z and speed in the image's stack slots (0x0cb80dbe to 0x0cb80e12).
     * without it, they take others, and 13 of 138 words differ */
    if (0) {
        x = y;
    }
    ev00fa_splash_column_task =
        task_create(TASK_WORK, 5, ev00fa_splash_column_init);
    column = heap_alloc(sizeof(struct splash_column));
    work = ev00fa_splash_column_task->work;
    COUNT(work)->column = column;
    work->mode = COLUMN_START;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->scl.x = COLUMN_THIN;
    work->scl.y = 1.0F;
    work->scl.z = COLUMN_THIN;
    COUNT(work)->speed = speed;
    COUNT(work)->count = 0;
    column->scale.x = scale;
    column->scale.y = scale;
    column->scale.z = scale;
    column->scl_step.x =
        (1.0F - work->scl.x) * COUNT(work)->speed * (1.0F / COLUMN_GROW_STEPS);
    column->scl_step.y = 0.0F;
    column->scl_step.z =
        (1.0F - work->scl.z) * COUNT(work)->speed * (1.0F / COLUMN_GROW_STEPS);
    column->color_step.a = COUNT(work)->speed * (1.0F / COLUMN_GROW_STEPS);
    column->color_step.r = 0.0F;
    column->color_step.g = 0.0F;
    column->color_step.b = 0.0F;
    column->color.a = -1.0F;
    column->color.r = 1.0F;
    column->color.g = 1.0F;
    column->color.b = 1.0F;
    column->scroll = 10;
    ev00fa_splash_column_copy = *ev00fa_splash_column_object;
}

static void ev00fa_splash_column_init(struct task *task)
{
    task->step = ev00fa_splash_column_step;
    task->draw = ev00fa_splash_column_draw;
    task->destructor = ev00fa_splash_column_destroy;
}

/* shift the uvs of the column object's two cylinders by the scroll of the
 * column in ev00fa_splash_column_task. the outer cylinder's u falls by
 * scroll and its v rises by it. the inner cylinder's u and v move by
 * scroll / 2. the object's own uvs change, so the shifts add up from step
 * to step */
static void ev00fa_splash_column_scroll(void)
{
    struct task_work *work = ev00fa_splash_column_task->work;
    struct splash_column *column = COUNT(work)->column;
    NJS_OBJECT *object;
    NJS_OBJECT *base = ev00fa_splash_column_object;
    NJS_COLOR *uv;
    int i;

    /* @hack redundant: object's address, taken and never used. SHC then keeps
     * object at @R15, stored there ahead of each loop (0x0cb80f5a,
     * 0x0cb80f96). without it, base takes R14, ev00fa_splash_column_scroll
     * runs 8 bytes short, and 79 of its 79 words differ */
    (void)&object;
    object = base;
    uv = base->child->model->meshsets->vertuv;
    i = 0;
    /* @hack spelling: this loop and the next, each as an if that tests object
     * around a do-while that tests base. SHC then makes each first test
     * through a copy of base (MOV R5,R2 at 0x0cb80f52, MOV R5,R3 at
     * 0x0cb80f94). as while loops on base, ev00fa_splash_column_scroll runs 4
     * bytes short, and 78 of its 79 words differ */
    if (i < object->child->model->nbPoint * 2) {
        do {
            uv->tex.u -= column->scroll;
            uv->tex.v += column->scroll;
            i++;
            uv++;
        } while (i < base->child->model->nbPoint * 2);
    }
    object = base;
    uv = base->child->sibling->model->meshsets->vertuv;
    i = 0;
    if (i < object->child->sibling->model->nbPoint * 2) {
        do {
            uv->tex.u -= column->scroll / 2;
            uv->tex.v += column->scroll / 2;
            i++;
            uv++;
        } while (i < base->child->sibling->model->nbPoint * 2);
    }
}

static void ev00fa_splash_column_step(struct task *task)
{
    struct task_work *work = task->work;
    struct splash_column *column = COUNT(work)->column;

    switch (work->mode) {
    case COLUMN_START:
        work->mode = COLUMN_GROW;
        break;
    case COLUMN_GROW:
        ev00fa_splash_column_scroll();
        work->scl.x += column->scl_step.x;
        work->scl.y += column->scl_step.y;
        work->scl.z += column->scl_step.z;
        column->color.a += column->color_step.a;
        column->color.r += column->color_step.r;
        column->color.g += column->color_step.g;
        column->color.b += column->color_step.b;
        if (work->scl.x >= 1.0F) {
            work->scl.x = 1.0F;
            work->scl.y = 1.0F;
            work->scl.z = 1.0F;
            column->color.a = 1.0F;
            column->color.r = 1.0F;
            column->color.g = 1.0F;
            column->color.b = 1.0F;
            work->mode = COLUMN_HOLD;
        }
        break;
    case COLUMN_HOLD:
        ev00fa_splash_column_scroll();
        if (++COUNT(work)->count > COLUMN_HOLD_COUNT) {
            work->mode = COLUMN_SHRINK;
        }
        break;
    case COLUMN_SHRINK:
        ev00fa_splash_column_scroll();
        work->scl.x -= column->scl_step.x;
        work->scl.y -= column->scl_step.y;
        work->scl.z -= column->scl_step.z;
        column->color.a -= column->color_step.a;
        column->color.r -= column->color_step.r;
        column->color.g -= column->color_step.g;
        column->color.b -= column->color_step.b;
        if (work->scl.x <= COLUMN_THIN) {
            work->scl.x = COLUMN_THIN;
            work->scl.y = COLUMN_THIN;
            work->scl.z = COLUMN_THIN;
            work->mode = COLUMN_END;
        }
        break;
    case COLUMN_END:
        if (task != NULL) {
            task_kill(task);
            ev00fa_splash_column_task = NULL;
        }
        break;
    }
    ev00fa_splash_column_draw(task);
}

static void ev00fa_splash_column_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct splash_column *column = COUNT(work)->column;
    /* @hack redundant: color, which nothing reads. the function copies its
     * initializer from its constants into its frame (0x0cb8116a to
     * 0x0cb81184). without it, ev00fa_splash_column_draw runs 26 bytes
     * short, and 72 of its 78 words differ */
    NJS_ARGB color = {0.0F, 0.0F, 0.0F, 0.0F};

    njSetTexture(&ev00fa_splash_texlist);
    draw_set_material(0.0F, -1.0F, -1.0F, -1.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    /* z takes scl.x, not scl.z. the two start, grow and shrink alike, so the
     * column stays round */
    njScale(NULL, work->scl.x * column->scale.x, work->scl.y * column->scale.y,
            work->scl.x * column->scale.z);
    draw_tree(&ev00fa_splash_column_copy);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

static void ev00fa_splash_column_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct splash_column *column = COUNT(work)->column;

    if (column != NULL) {
        heap_free(column);
    }
}

static void ev00fa_splash_ring_create(Float x, Float y, Float z, Float speed,
                                      Float scale)
{
    struct task *task = task_create(TASK_WORK, 5, ev00fa_splash_ring_init);
    struct task_work *work = task->work;

    work->mode = SPLASH_START;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->scl.x = scale;
    work->scl.y = scale;
    work->scl.z = scale;
    PLAY(work)->speed = speed;
    /* @hack type: the frame cleared through vars.l[0]. SHC then stores the
     * 0 already in R5 for the mode (MOV.L R5,@(8,R4) at 0x0cb8129e). as
     * PLAY(work)->frame = 0.0F, SHC loads it with FLDI0,
     * ev00fa_splash_ring_create runs 4 bytes long, and 31 of its 46 words
     * differ */
    work->vars.l[0] = 0;
}

static void ev00fa_splash_ring_init(struct task *task)
{
    task->step = ev00fa_splash_ring_step;
    task->draw = ev00fa_splash_ring_draw;
    task->destructor = ev00fa_splash_ring_destroy;
}

static void ev00fa_splash_ring_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SPLASH_START:
        work->mode = SPLASH_PLAY;
        break;
    case SPLASH_PLAY:
        PLAY(work)->frame += PLAY(work)->speed;
        if (PLAY(work)->frame >= ev00fa_splash_ring_action->motion->nbFrame) {
            work->mode = SPLASH_END;
        }
        break;
    case SPLASH_END:
        if (task != NULL) {
            task_kill(task);
        }
        break;
    }
    ev00fa_splash_ring_draw(task);
}

static void ev00fa_splash_ring_draw(struct task *task)
{
    struct task_work *work = task->work;

    njSetTexture(&ev00fa_splash_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njScaleV(NULL, &work->scl);
    draw_action(ev00fa_splash_ring_action, PLAY(work)->frame);
    njPopMatrix(1);
}

static void ev00fa_splash_ring_destroy(struct task *task)
{
}

void ev00fa_splash_create(Float x, Float y, Float z, Float speed, Float scale)
{
    struct task_work *work;

    ev00fa_splash_task = task_create(TASK_WORK, 5, ev00fa_splash_init);
    work = ev00fa_splash_task->work;
    work->mode = SPLASH_START;
    work->pos.x = x;
    work->pos.y = y;
    work->pos.z = z;
    work->scl.x = scale;
    work->scl.y = scale;
    work->scl.z = scale;
    COUNT(work)->speed = speed;
    COUNT(work)->count = 0;
}

void ev00fa_splash_kill(void)
{
    if (ev00fa_splash_task != NULL) {
        /* @bug memory: a splash that ends leaves the slot set, as its step
         * kills its task and nothing clears the slot (JSR @R3 at 0x0cb814a8).
         * killing the slot then stores task_destroy into the step of a task
         * back in the task pool, which may hold another object by now (JSR
         * @R1 at 0x0cb813c0). EV00FA's script starts a splash at a speed of
         * 1, which ends at a count of 60, and kills its slot at least 262
         * steps later */
        task_kill(ev00fa_splash_task);
        ev00fa_splash_task = NULL;
    }
}

static void ev00fa_splash_init(struct task *task)
{
    task->step = ev00fa_splash_step;
    task->draw = ev00fa_splash_draw;
    task->destructor = ev00fa_splash_destroy;
}

static void ev00fa_splash_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SPLASH_START:
        ev00fa_splash_ripples_create(work->pos.x, work->pos.y, work->pos.z,
                                     COUNT(work)->speed, work->scl.x);
        work->mode = SPLASH_PLAY;
        break;
    case SPLASH_PLAY:
        ++COUNT(work)->count;
        if (COUNT(work)->count == FRAMES_TO_STEPS(work, COLUMN_FRAME)) {
            ev00fa_splash_column_create(work->pos.x, work->pos.y, work->pos.z,
                                        COUNT(work)->speed, work->scl.x);
        }
        if (COUNT(work)->count >= FRAMES_TO_STEPS(work, FIRST_RING_FRAME)) {
            if (COUNT(work)->count % FRAMES_TO_STEPS(work, RING_PERIOD) == 0) {
                ev00fa_splash_ring_create(work->pos.x, work->pos.y, work->pos.z,
                                          COUNT(work)->speed, work->scl.x);
            }
        }
        if (COUNT(work)->count >= FRAMES_TO_STEPS(work, END_FRAME)) {
            work->mode = SPLASH_END;
        }
        break;
    case SPLASH_END:
        if (task != NULL) {
            task_kill(task);
        }
        break;
    }
    /* @bug logic: the step also draws a ring on the splash's own task, at
     * its place and scale (BRA at 0x0cb814b2). ev00fa_splash_ring_draw reads
     * the frame through PLAY, so it gets the splash's integer count read as
     * a float (FMOV.S @(R0,R14),FR4 at 0x0cb81346, R0 8). a count below 2^23
     * reads as a float below 2^-126, so the ring draws its motion's first
     * keys, flat at a y scale of 1e-5 */
    ev00fa_splash_ring_draw(task);
}

static void ev00fa_splash_draw(struct task *task)
{
}

static void ev00fa_splash_destroy(struct task *task)
{
}
