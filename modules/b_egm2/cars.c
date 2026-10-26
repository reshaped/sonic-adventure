/* the street's parked cars. each blows up and breaks apart once the Egg
 * Walker or its attacks hit it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_egm2/blast.h"
#include "b_egm2/cars.h"
#include "b_egm2/texlists.h"
#include "collision/collision.h"
#include "draw/state.h"
#include "draw/tree.h"
#include "enemy/enemy.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* a car's modes. CAR_HIT and CAR_EXPLODING each count down from CAR_DELAY,
 * and end as the count passes below 0 */
enum egm2_car_mode {
    CAR_START,
    CAR_PARKED,
    CAR_HIT,
    CAR_BLAST, /* sets the blast off, in one step */
    CAR_EXPLODING /* as the blast plays, until the car breaks into pieces */
};

/* work->vars of a car: the countdown in CAR_HIT and CAR_EXPLODING */
struct egm2_car_vars {
    Uint8 unk_00[4];
    Sint32 delay;
};

#define VARS(work) ((struct egm2_car_vars *)&(work)->vars)

/* where a car parks, and its turn about y */
struct egm2_car_spot {
    NJS_POINT3 pos;
    Angle ang;
};

/* a piece a car breaks into */
struct egm2_car_piece {
    NJS_OBJECT *object;
    Float scale; /* 1.0F in every piece, and unread */
};

/* a flying piece's record, at its task's extra */
struct egm2_debris {
    NJS_VECTOR speed;
    /* the turn each step adds about each axis */
    Angle spin_x;
    Angle spin_y;
    Angle spin_z;
    Uint8 piece; /* an index into egm2_car_pieces */
    Float alpha;
};

#define CARS 3
#define PIECES 6

#define CAR_DELAY 30
/* how far a car's collision shapes reach from its position */
#define CAR_REACH 40.0F
/* each step sets the car's sphere to this radius before it registers the
 * car's record. the radius egm2_car_shapes gives the sphere never
 * registers */
#define CAR_SPHERE_RADIUS 10.0F

#define SE_CAR_BLAST 0x1db

/* a car's blast: its grow_end, fade_start, end and size */
#define BLAST_GROWN 20
#define BLAST_FADE 40
#define BLAST_END 60
#define BLAST_SIZE 25.0F

/* a piece starts within PIECE_SCATTER of the car's position on each axis.
 * it flies off at up to PIECE_SPEED either way along x and z. it rises at
 * up to twice that. it spins about each axis by up to PIECE_SPIN a step.
 * each step adds PIECE_FALL to its upward speed, and takes PIECE_FADE off
 * its alpha. it bounces off the street, keeping PIECE_BOUNCE of its
 * vertical speed. its alpha falls below 0 after 143 steps. the piece ends
 * after 286, as its alpha falls below -1 */
#define PIECE_SCATTER 5.0F
#define PIECE_SPEED 1.0F
#define PIECE_SPIN NJM_DEG_ANG(1)
#define PIECE_FALL (-9.8F / 360.0F)
#define PIECE_BOUNCE 0.8F
#define PIECE_FADE 0.007F

static void egm2_car_create(NJS_POINT3 *pos, Angle ang);
static void egm2_car_step(struct task *task);
static void egm2_car_draw(struct task *task);
static void egm2_car_break(NJS_POINT3 *pos);
static void egm2_debris_create(NJS_POINT3 *pos, Angle ang, Uint8 piece);
static void egm2_debris_step(struct task *task);
static void egm2_debris_draw(struct task *task);

static const struct egm2_car_spot egm2_car_spots[CARS] = {
    {{-505.0F, -2.5F, 1110.0F}, 0x4000},
    {{-505.0F, -2.5F, 1295.0F}, 0x4000},
    {{-384.0F, -2.5F, 1168.0F}, 0xc000},
};

static NJS_MATERIAL egm2_car_node2_materials[2] = {
#include "b_egm2/cars/egm2_car_node2_materials.inc"
};

static Sint16 egm2_car_node2_meshes0[38] = {
#include "b_egm2/cars/egm2_car_node2_meshes0.inc"
};

static Sint16 egm2_car_node2_meshes1[36] = {
#include "b_egm2/cars/egm2_car_node2_meshes1.inc"
};

static NJS_COLOR egm2_car_node2_uvs0[36] = {
#include "b_egm2/cars/egm2_car_node2_uvs0.inc"
};

static NJS_COLOR egm2_car_node2_uvs1[32] = {
#include "b_egm2/cars/egm2_car_node2_uvs1.inc"
};

static NJS_MESHSET egm2_car_node2_meshsets[2] = {
#include "b_egm2/cars/egm2_car_node2_meshsets.inc"
};

static NJS_POINT3 egm2_car_node2_points[32] = {
#include "b_egm2/cars/egm2_car_node2_points.inc"
};

static NJS_VECTOR egm2_car_node2_normals[32] = {
#include "b_egm2/cars/egm2_car_node2_normals.inc"
};

static NJS_MODEL egm2_car_node2_model = {
#include "b_egm2/cars/egm2_car_node2_model.inc"
};

static NJS_OBJECT egm2_car_node2_object = {
#include "b_egm2/cars/egm2_car_node2_object.inc"
};

static NJS_MATERIAL egm2_car_node1_materials[2] = {
#include "b_egm2/cars/egm2_car_node1_materials.inc"
};

static Sint16 egm2_car_node1_meshes0[38] = {
#include "b_egm2/cars/egm2_car_node1_meshes0.inc"
};

static Sint16 egm2_car_node1_meshes1[18] = {
#include "b_egm2/cars/egm2_car_node1_meshes1.inc"
};

static NJS_COLOR egm2_car_node1_uvs0[36] = {
#include "b_egm2/cars/egm2_car_node1_uvs0.inc"
};

static NJS_COLOR egm2_car_node1_uvs1[16] = {
#include "b_egm2/cars/egm2_car_node1_uvs1.inc"
};

static NJS_MESHSET egm2_car_node1_meshsets[2] = {
#include "b_egm2/cars/egm2_car_node1_meshsets.inc"
};

static NJS_POINT3 egm2_car_node1_points[32] = {
#include "b_egm2/cars/egm2_car_node1_points.inc"
};

static NJS_VECTOR egm2_car_node1_normals[32] = {
#include "b_egm2/cars/egm2_car_node1_normals.inc"
};

static NJS_MODEL egm2_car_node1_model = {
#include "b_egm2/cars/egm2_car_node1_model.inc"
};

static NJS_OBJECT egm2_car_node1_object = {
#include "b_egm2/cars/egm2_car_node1_object.inc"
};

static NJS_MATERIAL egm2_car_materials[9] = {
#include "b_egm2/cars/egm2_car_materials.inc"
};

static Sint16 egm2_car_meshes0[30] = {
#include "b_egm2/cars/egm2_car_meshes0.inc"
};

static Sint16 egm2_car_meshes1[24] = {
#include "b_egm2/cars/egm2_car_meshes1.inc"
};

static Sint16 egm2_car_meshes2[176] = {
#include "b_egm2/cars/egm2_car_meshes2.inc"
};

static Sint16 egm2_car_meshes3[35] = {
#include "b_egm2/cars/egm2_car_meshes3.inc"
};

static Sint16 egm2_car_meshes4[5] = {
#include "b_egm2/cars/egm2_car_meshes4.inc"
};

static Sint16 egm2_car_meshes5[30] = {
#include "b_egm2/cars/egm2_car_meshes5.inc"
};

static Sint16 egm2_car_meshes6[20] = {
#include "b_egm2/cars/egm2_car_meshes6.inc"
};

static Sint16 egm2_car_meshes7[5] = {
#include "b_egm2/cars/egm2_car_meshes7.inc"
};

static Sint16 egm2_car_meshes8[10] = {
#include "b_egm2/cars/egm2_car_meshes8.inc"
};

static NJS_COLOR egm2_car_uvs0[24] = {
#include "b_egm2/cars/egm2_car_uvs0.inc"
};

static NJS_COLOR egm2_car_uvs1[19] = {
#include "b_egm2/cars/egm2_car_uvs1.inc"
};

static NJS_COLOR egm2_car_uvs2[152] = {
#include "b_egm2/cars/egm2_car_uvs2.inc"
};

static NJS_COLOR egm2_car_uvs3[30] = {
#include "b_egm2/cars/egm2_car_uvs3.inc"
};

static NJS_COLOR egm2_car_uvs4[4] = {
#include "b_egm2/cars/egm2_car_uvs4.inc"
};

static NJS_COLOR egm2_car_uvs5[24] = {
#include "b_egm2/cars/egm2_car_uvs5.inc"
};

static NJS_COLOR egm2_car_uvs6[16] = {
#include "b_egm2/cars/egm2_car_uvs6.inc"
};

static NJS_COLOR egm2_car_uvs7[4] = {
#include "b_egm2/cars/egm2_car_uvs7.inc"
};

static NJS_COLOR egm2_car_uvs8[8] = {
#include "b_egm2/cars/egm2_car_uvs8.inc"
};

static NJS_MESHSET egm2_car_meshsets[9] = {
#include "b_egm2/cars/egm2_car_meshsets.inc"
};

static NJS_POINT3 egm2_car_points[153] = {
#include "b_egm2/cars/egm2_car_points.inc"
};

static NJS_VECTOR egm2_car_normals[153] = {
#include "b_egm2/cars/egm2_car_normals.inc"
};

static NJS_MODEL egm2_car_model = {
#include "b_egm2/cars/egm2_car_model.inc"
};

static NJS_OBJECT egm2_car_object = {
#include "b_egm2/cars/egm2_car_object.inc"
};

static struct collision_shape egm2_car_shapes[2] = {
    {SHAPE_RECTANGLE | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 0.0F, 0.0F},
     25.0F,
     15.0F,
     10.0F,
     {0, 0, 0},
     0,
     NULL},
    {SHAPE_SPHERE | SHAPE_HURTABLE,
     SHAPE_HURTS_PLAYERS | SHAPE_HURTS_ENEMIES | SHAPE_HURTS_OBJECTS,
     PUSH_OBJECTS | PUSH_FIRM(PUSH_PLAYERS),
     {0.0F, 0.0F, 0.0F},
     27.0F,
     0.0F,
     0.0F,
     {0, 0, 0},
     0,
     NULL},
};

static NJS_MATERIAL egm2_car_debris0_materials[2] = {
#include "b_egm2/cars/egm2_car_debris0_materials.inc"
};

static Sint16 egm2_car_debris0_meshes0[28] = {
#include "b_egm2/cars/egm2_car_debris0_meshes0.inc"
};

static Sint16 egm2_car_debris0_meshes1[10] = {
#include "b_egm2/cars/egm2_car_debris0_meshes1.inc"
};

static NJS_COLOR egm2_car_debris0_uvs0[24] = {
#include "b_egm2/cars/egm2_car_debris0_uvs0.inc"
};

static NJS_COLOR egm2_car_debris0_uvs1[8] = {
#include "b_egm2/cars/egm2_car_debris0_uvs1.inc"
};

static NJS_MESHSET egm2_car_debris0_meshsets[2] = {
#include "b_egm2/cars/egm2_car_debris0_meshsets.inc"
};

static NJS_POINT3 egm2_car_debris0_points[12] = {
#include "b_egm2/cars/egm2_car_debris0_points.inc"
};

static NJS_VECTOR egm2_car_debris0_normals[12] = {
#include "b_egm2/cars/egm2_car_debris0_normals.inc"
};

static NJS_MODEL egm2_car_debris0_model = {
#include "b_egm2/cars/egm2_car_debris0_model.inc"
};

static NJS_OBJECT egm2_car_debris0_object = {
#include "b_egm2/cars/egm2_car_debris0_object.inc"
};

static NJS_MATERIAL egm2_car_debris1_materials[3] = {
#include "b_egm2/cars/egm2_car_debris1_materials.inc"
};

static Sint16 egm2_car_debris1_meshes0[14] = {
#include "b_egm2/cars/egm2_car_debris1_meshes0.inc"
};

static Sint16 egm2_car_debris1_meshes1[17] = {
#include "b_egm2/cars/egm2_car_debris1_meshes1.inc"
};

static Sint16 egm2_car_debris1_meshes2[14] = {
#include "b_egm2/cars/egm2_car_debris1_meshes2.inc"
};

static NJS_COLOR egm2_car_debris1_uvs0[11] = {
#include "b_egm2/cars/egm2_car_debris1_uvs0.inc"
};

static NJS_COLOR egm2_car_debris1_uvs1[16] = {
#include "b_egm2/cars/egm2_car_debris1_uvs1.inc"
};

static NJS_COLOR egm2_car_debris1_uvs2[11] = {
#include "b_egm2/cars/egm2_car_debris1_uvs2.inc"
};

static NJS_MESHSET egm2_car_debris1_meshsets[3] = {
#include "b_egm2/cars/egm2_car_debris1_meshsets.inc"
};

static NJS_POINT3 egm2_car_debris1_points[14] = {
#include "b_egm2/cars/egm2_car_debris1_points.inc"
};

static NJS_VECTOR egm2_car_debris1_normals[14] = {
#include "b_egm2/cars/egm2_car_debris1_normals.inc"
};

static NJS_MODEL egm2_car_debris1_model = {
#include "b_egm2/cars/egm2_car_debris1_model.inc"
};

static NJS_OBJECT egm2_car_debris1_object = {
#include "b_egm2/cars/egm2_car_debris1_object.inc"
};

static NJS_MATERIAL egm2_car_debris2_materials[5] = {
#include "b_egm2/cars/egm2_car_debris2_materials.inc"
};

static Sint16 egm2_car_debris2_meshes0[5] = {
#include "b_egm2/cars/egm2_car_debris2_meshes0.inc"
};

static Sint16 egm2_car_debris2_meshes1[35] = {
#include "b_egm2/cars/egm2_car_debris2_meshes1.inc"
};

static Sint16 egm2_car_debris2_meshes2[35] = {
#include "b_egm2/cars/egm2_car_debris2_meshes2.inc"
};

static Sint16 egm2_car_debris2_meshes3[22] = {
#include "b_egm2/cars/egm2_car_debris2_meshes3.inc"
};

static Sint16 egm2_car_debris2_meshes4[19] = {
#include "b_egm2/cars/egm2_car_debris2_meshes4.inc"
};

static NJS_COLOR egm2_car_debris2_uvs0[4] = {
#include "b_egm2/cars/egm2_car_debris2_uvs0.inc"
};

static NJS_COLOR egm2_car_debris2_uvs1[32] = {
#include "b_egm2/cars/egm2_car_debris2_uvs1.inc"
};

static NJS_COLOR egm2_car_debris2_uvs2[29] = {
#include "b_egm2/cars/egm2_car_debris2_uvs2.inc"
};

static NJS_COLOR egm2_car_debris2_uvs3[19] = {
#include "b_egm2/cars/egm2_car_debris2_uvs3.inc"
};

static NJS_COLOR egm2_car_debris2_uvs4[16] = {
#include "b_egm2/cars/egm2_car_debris2_uvs4.inc"
};

static NJS_MESHSET egm2_car_debris2_meshsets[5] = {
#include "b_egm2/cars/egm2_car_debris2_meshsets.inc"
};

static NJS_POINT3 egm2_car_debris2_points[41] = {
#include "b_egm2/cars/egm2_car_debris2_points.inc"
};

static NJS_VECTOR egm2_car_debris2_normals[41] = {
#include "b_egm2/cars/egm2_car_debris2_normals.inc"
};

static NJS_MODEL egm2_car_debris2_model = {
#include "b_egm2/cars/egm2_car_debris2_model.inc"
};

static NJS_OBJECT egm2_car_debris2_object = {
#include "b_egm2/cars/egm2_car_debris2_object.inc"
};

static NJS_MATERIAL egm2_car_debris3_materials[13] = {
#include "b_egm2/cars/egm2_car_debris3_materials.inc"
};

static Sint16 egm2_car_debris3_meshes0[29] = {
#include "b_egm2/cars/egm2_car_debris3_meshes0.inc"
};

static Sint16 egm2_car_debris3_meshes1[103] = {
#include "b_egm2/cars/egm2_car_debris3_meshes1.inc"
};

static Sint16 egm2_car_debris3_meshes2[11] = {
#include "b_egm2/cars/egm2_car_debris3_meshes2.inc"
};

static Sint16 egm2_car_debris3_meshes3[80] = {
#include "b_egm2/cars/egm2_car_debris3_meshes3.inc"
};

static Sint16 egm2_car_debris3_meshes4[18] = {
#include "b_egm2/cars/egm2_car_debris3_meshes4.inc"
};

static Sint16 egm2_car_debris3_meshes5[15] = {
#include "b_egm2/cars/egm2_car_debris3_meshes5.inc"
};

static Sint16 egm2_car_debris3_meshes6[5] = {
#include "b_egm2/cars/egm2_car_debris3_meshes6.inc"
};

static Sint16 egm2_car_debris3_meshes7[5] = {
#include "b_egm2/cars/egm2_car_debris3_meshes7.inc"
};

static Sint16 egm2_car_debris3_meshes8[15] = {
#include "b_egm2/cars/egm2_car_debris3_meshes8.inc"
};

static Sint16 egm2_car_debris3_meshes9[5] = {
#include "b_egm2/cars/egm2_car_debris3_meshes9.inc"
};

static Sint16 egm2_car_debris3_meshes10[5] = {
#include "b_egm2/cars/egm2_car_debris3_meshes10.inc"
};

static Sint16 egm2_car_debris3_meshes11[24] = {
#include "b_egm2/cars/egm2_car_debris3_meshes11.inc"
};

static Sint16 egm2_car_debris3_meshes12[39] = {
#include "b_egm2/cars/egm2_car_debris3_meshes12.inc"
};

static NJS_COLOR egm2_car_debris3_uvs0[24] = {
#include "b_egm2/cars/egm2_car_debris3_uvs0.inc"
};

static NJS_COLOR egm2_car_debris3_uvs1[83] = {
#include "b_egm2/cars/egm2_car_debris3_uvs1.inc"
};

static NJS_COLOR egm2_car_debris3_uvs2[9] = {
#include "b_egm2/cars/egm2_car_debris3_uvs2.inc"
};

static NJS_COLOR egm2_car_debris3_uvs3[64] = {
#include "b_egm2/cars/egm2_car_debris3_uvs3.inc"
};

static NJS_COLOR egm2_car_debris3_uvs4[16] = {
#include "b_egm2/cars/egm2_car_debris3_uvs4.inc"
};

static NJS_COLOR egm2_car_debris3_uvs5[12] = {
#include "b_egm2/cars/egm2_car_debris3_uvs5.inc"
};

static NJS_COLOR egm2_car_debris3_uvs6[4] = {
#include "b_egm2/cars/egm2_car_debris3_uvs6.inc"
};

static NJS_COLOR egm2_car_debris3_uvs7[4] = {
#include "b_egm2/cars/egm2_car_debris3_uvs7.inc"
};

static NJS_COLOR egm2_car_debris3_uvs8[12] = {
#include "b_egm2/cars/egm2_car_debris3_uvs8.inc"
};

static NJS_COLOR egm2_car_debris3_uvs9[4] = {
#include "b_egm2/cars/egm2_car_debris3_uvs9.inc"
};

static NJS_COLOR egm2_car_debris3_uvs10[4] = {
#include "b_egm2/cars/egm2_car_debris3_uvs10.inc"
};

static NJS_COLOR egm2_car_debris3_uvs11[20] = {
#include "b_egm2/cars/egm2_car_debris3_uvs11.inc"
};

static NJS_COLOR egm2_car_debris3_uvs12[32] = {
#include "b_egm2/cars/egm2_car_debris3_uvs12.inc"
};

static NJS_MESHSET egm2_car_debris3_meshsets[13] = {
#include "b_egm2/cars/egm2_car_debris3_meshsets.inc"
};

static NJS_POINT3 egm2_car_debris3_points[100] = {
#include "b_egm2/cars/egm2_car_debris3_points.inc"
};

static NJS_VECTOR egm2_car_debris3_normals[100] = {
#include "b_egm2/cars/egm2_car_debris3_normals.inc"
};

static NJS_MODEL egm2_car_debris3_model = {
#include "b_egm2/cars/egm2_car_debris3_model.inc"
};

static NJS_OBJECT egm2_car_debris3_object = {
#include "b_egm2/cars/egm2_car_debris3_object.inc"
};

static NJS_MATERIAL egm2_car_debris4_materials[13] = {
#include "b_egm2/cars/egm2_car_debris4_materials.inc"
};

static Sint16 egm2_car_debris4_meshes0[24] = {
#include "b_egm2/cars/egm2_car_debris4_meshes0.inc"
};

static Sint16 egm2_car_debris4_meshes1[113] = {
#include "b_egm2/cars/egm2_car_debris4_meshes1.inc"
};

static Sint16 egm2_car_debris4_meshes2[7] = {
#include "b_egm2/cars/egm2_car_debris4_meshes2.inc"
};

static Sint16 egm2_car_debris4_meshes3[68] = {
#include "b_egm2/cars/egm2_car_debris4_meshes3.inc"
};

static Sint16 egm2_car_debris4_meshes4[19] = {
#include "b_egm2/cars/egm2_car_debris4_meshes4.inc"
};

static Sint16 egm2_car_debris4_meshes5[39] = {
#include "b_egm2/cars/egm2_car_debris4_meshes5.inc"
};

static Sint16 egm2_car_debris4_meshes6[26] = {
#include "b_egm2/cars/egm2_car_debris4_meshes6.inc"
};

static Sint16 egm2_car_debris4_meshes7[19] = {
#include "b_egm2/cars/egm2_car_debris4_meshes7.inc"
};

static Sint16 egm2_car_debris4_meshes8[5] = {
#include "b_egm2/cars/egm2_car_debris4_meshes8.inc"
};

static Sint16 egm2_car_debris4_meshes9[10] = {
#include "b_egm2/cars/egm2_car_debris4_meshes9.inc"
};

static Sint16 egm2_car_debris4_meshes10[5] = {
#include "b_egm2/cars/egm2_car_debris4_meshes10.inc"
};

static Sint16 egm2_car_debris4_meshes11[5] = {
#include "b_egm2/cars/egm2_car_debris4_meshes11.inc"
};

static Sint16 egm2_car_debris4_meshes12[4] = {
#include "b_egm2/cars/egm2_car_debris4_meshes12.inc"
};

static NJS_COLOR egm2_car_debris4_uvs0[20] = {
#include "b_egm2/cars/egm2_car_debris4_uvs0.inc"
};

static NJS_COLOR egm2_car_debris4_uvs1[94] = {
#include "b_egm2/cars/egm2_car_debris4_uvs1.inc"
};

static NJS_COLOR egm2_car_debris4_uvs2[6] = {
#include "b_egm2/cars/egm2_car_debris4_uvs2.inc"
};

static NJS_COLOR egm2_car_debris4_uvs3[58] = {
#include "b_egm2/cars/egm2_car_debris4_uvs3.inc"
};

static NJS_COLOR egm2_car_debris4_uvs4[16] = {
#include "b_egm2/cars/egm2_car_debris4_uvs4.inc"
};

static NJS_COLOR egm2_car_debris4_uvs5[34] = {
#include "b_egm2/cars/egm2_car_debris4_uvs5.inc"
};

static NJS_COLOR egm2_car_debris4_uvs6[21] = {
#include "b_egm2/cars/egm2_car_debris4_uvs6.inc"
};

static NJS_COLOR egm2_car_debris4_uvs7[16] = {
#include "b_egm2/cars/egm2_car_debris4_uvs7.inc"
};

static NJS_COLOR egm2_car_debris4_uvs8[4] = {
#include "b_egm2/cars/egm2_car_debris4_uvs8.inc"
};

static NJS_COLOR egm2_car_debris4_uvs9[8] = {
#include "b_egm2/cars/egm2_car_debris4_uvs9.inc"
};

static NJS_COLOR egm2_car_debris4_uvs10[4] = {
#include "b_egm2/cars/egm2_car_debris4_uvs10.inc"
};

static NJS_COLOR egm2_car_debris4_uvs11[4] = {
#include "b_egm2/cars/egm2_car_debris4_uvs11.inc"
};

static NJS_MESHSET egm2_car_debris4_meshsets[13] = {
#include "b_egm2/cars/egm2_car_debris4_meshsets.inc"
};

static NJS_POINT3 egm2_car_debris4_points[115] = {
#include "b_egm2/cars/egm2_car_debris4_points.inc"
};

static NJS_VECTOR egm2_car_debris4_normals[115] = {
#include "b_egm2/cars/egm2_car_debris4_normals.inc"
};

static NJS_MODEL egm2_car_debris4_model = {
#include "b_egm2/cars/egm2_car_debris4_model.inc"
};

static NJS_OBJECT egm2_car_debris4_object = {
#include "b_egm2/cars/egm2_car_debris4_object.inc"
};

static NJS_MATERIAL egm2_car_debris5_materials[9] = {
#include "b_egm2/cars/egm2_car_debris5_materials.inc"
};

static Sint16 egm2_car_debris5_meshes0[24] = {
#include "b_egm2/cars/egm2_car_debris5_meshes0.inc"
};

static Sint16 egm2_car_debris5_meshes1[118] = {
#include "b_egm2/cars/egm2_car_debris5_meshes1.inc"
};

static Sint16 egm2_car_debris5_meshes2[13] = {
#include "b_egm2/cars/egm2_car_debris5_meshes2.inc"
};

static Sint16 egm2_car_debris5_meshes3[30] = {
#include "b_egm2/cars/egm2_car_debris5_meshes3.inc"
};

static Sint16 egm2_car_debris5_meshes4[62] = {
#include "b_egm2/cars/egm2_car_debris5_meshes4.inc"
};

static Sint16 egm2_car_debris5_meshes5[130] = {
#include "b_egm2/cars/egm2_car_debris5_meshes5.inc"
};

static Sint16 egm2_car_debris5_meshes6[5] = {
#include "b_egm2/cars/egm2_car_debris5_meshes6.inc"
};

static Sint16 egm2_car_debris5_meshes7[51] = {
#include "b_egm2/cars/egm2_car_debris5_meshes7.inc"
};

static Sint16 egm2_car_debris5_meshes8[65] = {
#include "b_egm2/cars/egm2_car_debris5_meshes8.inc"
};

static NJS_COLOR egm2_car_debris5_uvs0[20] = {
#include "b_egm2/cars/egm2_car_debris5_uvs0.inc"
};

static NJS_COLOR egm2_car_debris5_uvs1[92] = {
#include "b_egm2/cars/egm2_car_debris5_uvs1.inc"
};

static NJS_COLOR egm2_car_debris5_uvs2[11] = {
#include "b_egm2/cars/egm2_car_debris5_uvs2.inc"
};

static NJS_COLOR egm2_car_debris5_uvs3[24] = {
#include "b_egm2/cars/egm2_car_debris5_uvs3.inc"
};

static NJS_COLOR egm2_car_debris5_uvs4[51] = {
#include "b_egm2/cars/egm2_car_debris5_uvs4.inc"
};

static NJS_COLOR egm2_car_debris5_uvs5[106] = {
#include "b_egm2/cars/egm2_car_debris5_uvs5.inc"
};

static NJS_COLOR egm2_car_debris5_uvs6[4] = {
#include "b_egm2/cars/egm2_car_debris5_uvs6.inc"
};

static NJS_COLOR egm2_car_debris5_uvs7[43] = {
#include "b_egm2/cars/egm2_car_debris5_uvs7.inc"
};

static NJS_COLOR egm2_car_debris5_uvs8[55] = {
#include "b_egm2/cars/egm2_car_debris5_uvs8.inc"
};

static NJS_MESHSET egm2_car_debris5_meshsets[9] = {
#include "b_egm2/cars/egm2_car_debris5_meshsets.inc"
};

static NJS_POINT3 egm2_car_debris5_points[146] = {
#include "b_egm2/cars/egm2_car_debris5_points.inc"
};

static NJS_VECTOR egm2_car_debris5_normals[146] = {
#include "b_egm2/cars/egm2_car_debris5_normals.inc"
};

static NJS_MODEL egm2_car_debris5_model = {
#include "b_egm2/cars/egm2_car_debris5_model.inc"
};

static NJS_OBJECT egm2_car_debris5_object = {
#include "b_egm2/cars/egm2_car_debris5_object.inc"
};

static const struct egm2_car_piece egm2_car_pieces[PIECES] = {
    {&egm2_car_debris0_object, 1.0F}, {&egm2_car_debris1_object, 1.0F},
    {&egm2_car_debris2_object, 1.0F}, {&egm2_car_debris3_object, 1.0F},
    {&egm2_car_debris4_object, 1.0F}, {&egm2_car_debris5_object, 1.0F},
};

static void egm2_car_create(NJS_POINT3 *pos, Angle ang)
{
    struct task *task;
    struct task_work *work;

    if ((task = task_create(TASK_WORK, 3, egm2_car_step)) != NULL) {
        work = task->work;
        enemy_alloc_record(task, work);
        work->pos = *pos;
        work->ang.y = ang;
        task->step = egm2_car_step;
        task->draw = egm2_car_draw;
        collision_init(task, egm2_car_shapes, COLLISION_OBJECT_MUTUAL, 1, 2);
        work->collision->radius = CAR_REACH;
    }
}

/* a parked car is hit when its first shape touches a COLLISION_ITEM or
 * COLLISION_ENEMY record. the boss's parts, its flames and the blasts are
 * COLLISION_ENEMY records. the stomps' shockwaves are COLLISION_ITEM
 * records */
static void egm2_car_step(struct task *task)
{
    struct task_work *work = task->work;
    struct blast_params params;
    Uint16 kind;

    switch (work->mode) {
    case CAR_START:
        work->mode = CAR_PARKED;
    case CAR_PARKED:
        if (work->collision->flags & COLLISION_HIT) {
            if (work->collision->shapes->hit != NULL) {
                kind = work->collision->shapes->hit->kind;
                if (kind == COLLISION_ITEM || kind == COLLISION_ENEMY) {
                    VARS(work)->delay = CAR_DELAY;
                    work->mode = CAR_HIT;
                }
            }
        }
        break;
    case CAR_HIT:
        if (--VARS(work)->delay < 0) {
            work->mode = CAR_BLAST;
        }
        break;
    case CAR_BLAST:
        params.pos = &work->pos;
        params.flags = BLAST_COPY_POS | BLAST_HURTS;
        params.beam_start = 0;
        params.beam_end = 0;
        params.beam_length = 0.0F;
        params.beam_speed = 0;
        params.grow_start = 0;
        params.grow_end = BLAST_GROWN;
        params.fade_start = BLAST_FADE;
        params.end = BLAST_END;
        params.size = BLAST_SIZE;
        egg_walker_blast_create(&params);
        sound_play(SE_CAR_BLAST, 0, 0, 0);
        VARS(work)->delay = CAR_DELAY;
        work->mode = CAR_EXPLODING;
        break;
    case CAR_EXPLODING:
        if (--VARS(work)->delay < 0) {
            egm2_car_break(&work->pos);
            task_kill(task);
        }
        break;
    }
    work->collision->shapes[1].radius = CAR_SPHERE_RADIUS;
    collision_register(work);
    egm2_car_draw(task);
}

static void egm2_car_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&egm2_car_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        draw_tree(&egm2_car_object);
        njPopMatrix(1);
    }
}

void egm2_cars_create(void)
{
    int i;
    const struct egm2_car_spot *spot;

    for (i = 0; i < CARS; i++) {
        spot = &egm2_car_spots[i];
        egm2_car_create((NJS_POINT3 *)&spot->pos, spot->ang);
    }
}

static void egm2_car_break(NJS_POINT3 *pos)
{
    int i;
    NJS_POINT3 at;

    for (i = 0; i < PIECES; i++) {
        at = *pos;
        at.x += RANDOM(-PIECE_SCATTER, PIECE_SCATTER);
        at.y += RANDOM(-PIECE_SCATTER, PIECE_SCATTER);
        at.z += RANDOM(-PIECE_SCATTER, PIECE_SCATTER);
        egm2_debris_create(&at, 0, i);
    }
}

/* make a flying egm2_car_pieces[piece] at *pos, turned ang about y. it
 * makes none for a piece of PIECES or more, or when no task or memory is
 * free */
static void egm2_debris_create(NJS_POINT3 *pos, Angle ang, Uint8 piece)
{
    struct task *task;
    struct task_work *work;
    struct egm2_debris *debris;

    if (piece < PIECES) {
        if ((task = task_create(TASK_WORK, 3, egm2_debris_step)) != NULL) {
            work = task->work;
            /* @hack spelling: debris tested as it is assigned, which copies the
             * pointer to R14 in the BF/S's delay slot (0x0c90343a); assigned
             * first, the copy comes before the test and 3 of 119 words
             * differ */
            if ((debris = heap_calloc(1, sizeof(struct egm2_debris))) == NULL) {
                task_kill(task);
                return;
            }
            enemy_alloc_record(task, work);
            task->extra = debris;
            work->pos = *pos;
            work->ang.y = ang;
            debris->piece = piece;
            debris->alpha = 1.0F;
            debris->speed.x = RANDOM(-PIECE_SPEED, PIECE_SPEED);
            debris->speed.z = RANDOM(-PIECE_SPEED, PIECE_SPEED);
            debris->speed.y = rand() * (2.0F * PIECE_SPEED * RAND_SCALE);
            debris->spin_x = (Angle)(rand() * (PIECE_SPIN * RAND_SCALE));
            debris->spin_y = (Angle)(rand() * (PIECE_SPIN * RAND_SCALE));
            debris->spin_z = (Angle)(rand() * (PIECE_SPIN * RAND_SCALE));
            task->step = egm2_debris_step;
            task->draw = egm2_debris_draw;
        }
    }
}

static void egm2_debris_step(struct task *task)
{
    struct task_work *work = task->work;
    struct egm2_debris *debris = task->extra;

    VECTOR_SUM(&work->pos, &work->pos, &debris->speed);
    debris->speed.y += PIECE_FALL;
    if (work->pos.y < 0.0F) {
        work->pos.y = 0.0F;
        debris->speed.y = njAbs(debris->speed.y) * PIECE_BOUNCE;
    }
    work->ang.x += debris->spin_x;
    work->ang.y += debris->spin_y;
    work->ang.z += debris->spin_z;
    if ((debris->alpha -= PIECE_FADE) < -1.0F) {
        task_kill(task);
    }
    egm2_debris_draw(task);
}

/* draw the piece translucent at its alpha, on a frame's first step. its
 * alpha is below 0 for the second half of its life */
static void egm2_debris_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct egm2_debris *debris = task->extra;
    Angle ang;

    if (frame_step == 0) {
        njSetTexture(&egm2_car_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        TURN_VIA(njRotateX, work->ang.x, ang);
        TURN_VIA(njRotateZ, work->ang.z, ang);
        draw_state_save();
        draw_translucent(debris->alpha);
        njDrawObject(egm2_car_pieces[debris->piece].object);
        njPopMatrix(1);
        draw_state_restore();
    }
}
