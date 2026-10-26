/* the Egg Viper boss's steps, the record and spline its tasks share, and its
 * trees and node matrices */
#ifndef B_EGM3_EGG_VIPER_H
#define B_EGM3_EGG_VIPER_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the cockpit's node, which Eggman rides while the boss's front is open */
#define EGG_VIPER_EGGMAN_NODE 50
/* egg_viper_node_matrices[0] is this node's matrix */
#define EGG_VIPER_NODE_MATRICES_FIRST 48

/* the boss's motions, by index in egg_viper_motions. a _SHAPE entry is the
 * shape motion that plays beside the motion before it. EGG_VIPER_FRONT_OPEN
 * has no _SHAPE entry, and egg_viper_motions[EGG_VIPER_WAVE_SHAPE] is null */
#define EGG_VIPER_BACK_OPEN 0
#define EGG_VIPER_BACK_OPEN_SHAPE 1
#define EGG_VIPER_FRONT_OPEN 2
#define EGG_VIPER_WAVE 3
#define EGG_VIPER_WAVE_SHAPE 4

/* a sprite to draw at a frame of its animation. frames counts the frames,
 * and attr is the attribute njDrawSprite3D draws it with */
struct egg_viper_sprite {
    int frames;
    NJS_SPRITE *sprite;
    Uint32 attr;
};

/* the record in task->extra of the boss's tasks and most of its effects'.
 * each task's creator takes it zeroed from the heap */
struct egg_viper_record {
    Float scale; /* an effect's size, 1 for its model's or sprite's own */
    Uint8 unk_04[8];
    NJS_VECTOR moved; /* how far the task moves in a step, on each axis */
    /* part_strike writes them, and nothing reads them */
    Float unk_18;
    Float unk_1c;
    Float unk_20;
    NJS_POINT3 target; /* the point the task heads for */
    /* the angles the boss turns toward, about x and about y */
    Angle target_ang_x;
    Angle target_ang_y;
    Uint8 unk_38[8];
    /* the angle whose sine sets the boss's height in an arc or a bob */
    Angle phase;
    Uint8 unk_44[8];
    /* a needle's turn each step: spin about y, and roll about z */
    Angle spin;
    Angle roll;
    /* player 0's side of the arena as the boss's mode took it, to index the
     * mode's points. a mode that reads x alone sets 0 below x 0, and 1 from x
     * 0 up. a mode that reads z too sets 0 or 2 by x. it then adds 1 for
     * player 0 on one side of a line in z. that line lies at z 0, at the boss's
     * z or at the row's middle, by mode */
    Sint32 side;
    /* the boss's place along its spline, from 0 to 1. a ball keeps its
     * speed here. gathering energy keeps its sprites' scale, and a pillar of
     * fire the height scale of its top section */
    Float along;
    /* script indexes scripts, and step the mode in it viper_next_mode takes
     * next */
    Sint32 script;
    Sint32 step;
    Uint32 flags; /* VIPER_, PART_ or FALL bits, by the task's kind */
    Uint8 unk_68[4];
    /* the motion the task's tree plays, and the shape motion that plays
     * beside it, or null for none */
    NJS_MOTION *motion;
    NJS_MOTION *shape;
    struct egg_viper_sprite *sprite; /* an effect's sprite */
    /* the frame of the motion or the sprite, and the frames it moves each
     * step */
    Float frame;
    Float speed;
};

/* a cubic spline through count points. each array holds count values. the
 * caller fills x, y and z, and egg_viper_spline_init fills the rest */
struct egg_viper_spline {
    Float *x;
    Float *y;
    Float *z;
    /* each point's place along the curve, from 0 at the first to 1 at the last.
     * it is the straight distance to it through the points before, over the
     * whole */
    Float *knots;
    /* a sixth of the curve's second derivative at each point, in x, y and
     * z */
    Float *ddx;
    Float *ddy;
    Float *ddz;
    Sint32 count;
};

/* the steps of the boss's task, of each needle's and of each segment's. a
 * needle's tick is its number, 0 or 1. a segment's tick is its number from the
 * boss, 0 to 12, plus its kind. the kind is 0x10 for a joint, 0x20 for the
 * tail, and else 0 */
void egg_viper_step(struct task *task);
void egg_viper_needle_step(struct task *task);
void egg_viper_segment_step(struct task *task);
/* the arena's step. it scrolls the smoke under the floor and pulses the
 * joints' light. it puffs smoke about the boss 15 at a time at a hit on
 * Eggman. it also puffs smoke every 6 steps from the boss's last hit until
 * the boss is gone. it
 * downs player 0 once he falls below y -200. it saves the smoke model's uvs
 * on its first step, and its destructor puts them back */
void egg_viper_arena_step(struct task *task);
/* put the matrix of each node of object's tree, on the current matrix, in
 * matrices. matrices must hold one for each node. a node comes before its
 * children, and they before its siblings. each node's matrix takes
 * njTranslate, njRotateZ, njRotateY, njRotateX and njScale in turn, whatever
 * its evalflags say. the tree must nest no more than 32 levels below
 * object */
void egg_viper_tree_matrices(NJS_OBJECT *object, NJS_MATRIX *matrices);
/* fill spline's knots and its second derivatives from its points. count
 * must be 3 to 16, and no point may repeat the one before it */
void egg_viper_spline_init(struct egg_viper_spline *spline);
/* put spline's point at along in pos: along 0 is the first point, and 1
 * the last */
void egg_viper_spline_point(struct egg_viper_spline *spline, Float along,
                            NJS_POINT3 *pos);
/* return from turned toward to by step at most, the short way round. from
 * and to are read as 16-bit angles, and the result is one */
Angle egg_viper_angle_approach(Angle from, Angle to, Angle step);

/* the trees of the boss and its parts, and Eggman's */
extern NJS_OBJECT *egg_viper_objects[];
extern NJS_OBJECT *egg_viper_eggman_objects[];
/* their motions, the boss's by EGG_VIPER_ index */
extern NJS_MOTION *egg_viper_motions[];
extern NJS_MOTION *egg_viper_eggman_motions[];
/* the texlist the boss, its parts and Eggman draw with */
extern NJS_TEXLIST egm3mdl_texlist;

/* egg_viper.c's globals, in the order of their bss in the image. SHC lays
 * out bss in the order it first meets each name */

/* the steps the energy mode holds open before it flies its path. a hit
 * cuts it to 10 */
extern Sint32 egg_viper_energy_time;
/* the matrices of the boss tree's 51 nodes, as egg_viper_tree_matrices last
 * left them. SHC lays out three objects in turn: nodes 0 to 46, node 47, and
 * nodes 48 on. the walk from egg_viper_matrices writes on past its end into the
 * other two */
extern NJS_MATRIX egg_viper_matrices[47];
extern NJS_MATRIX egg_viper_anchor_matrix; /* node 47, which segment 0 trails */
extern NJS_MATRIX egg_viper_node_matrices[16];
/* the boss's angle about y as its task starts, which its creator sets */
extern Angle egg_viper_start_ang;
/* the boss sets egg_viper_camera_next to move the camera on to the next
 * subphase of its phase. the camera sets egg_viper_camera_ready once
 * VIPER_LASER1's view is in place, and the boss waits for it */
extern Bool egg_viper_camera_next;
extern Bool egg_viper_camera_ready;

#endif
