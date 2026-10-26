/* what the Chaos fights share: the boss's record and setup, the puddle he
 * flows as, his water's animation, his blink, and small helpers */
#ifndef CHAOS_CHAOS_H
#define CHAOS_CHAOS_H
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/motion.h"

struct task;
struct task_work;

/* a Chaos boss's record, in his task's extra. motion_create allocates it,
 * so it opens with his motion. an event's puddle keeps one too */
struct chaos {
    struct motion motion;
    /* a count the boss's step raises by 1. while blink is 1, the blinking
     * materials show white when the count is odd */
    Sint32 steps;
    /* the water animation's calls since frame last moved on. the fourth
     * moves frame on and starts the count again */
    Sint8 tick;
    /* the water's texture frame: one of six for chaos_animate_surface, and
     * of four for Perfect Chaos's water */
    Sint8 frame;
    Sint8 unk_3e;
    Sint8 puddle_mode; /* an enum puddle_mode */
    Uint8 unk_40;
    /* a hit sets it to 1, which makes the blinking materials blink. 0 stops
     * them */
    Sint8 blink;
    Sint8 camera_mode; /* the mode the boss last gave camera_override_mode */
    Uint8 unk_43;
    Sint16 flags;
    /* a bit for each figure the boss draws. chaos_setup sets 1, his body */
    Sint16 figures;
    Uint8 unk_48[4];
    NJS_VECTOR scale;
    NJS_POINT3 jump_from; /* where Chaos 0's jump starts */
    /* a point the boss heads for: where chaos_steer steers him, where Chaos
     * 0 jumps to, and where a leg of Chaos 6's weave ends */
    NJS_POINT3 target;
    Float health;
    Float reach; /* how far Chaos 0's or Chaos 2's arm stretches out */
    /* the pitch to player 0 his arm stretches at. Chaos 0's stretched arm
     * turns by it */
    Angle pitch;
    Angle facing; /* how far off player 0 chaos_face_player aims him */
    /* the constant materials his figure is drawn with. the second is Chaos
     * 0's alone, for the nodes chaos0_set_material gives it */
    NJS_ARGB colors[2];
    Uint8 unk_a0[4];
    Sint16 attack_timer; /* the steps to his next attack */
    Uint8 unk_a6;
    /* the steps since Chaos 2's barrier last threw the player off, up to
     * 11 */
    Sint8 barrier_steps;
    Float barrier_glow; /* how brightly Chaos 2's barrier glows */
    NJS_TEXLIST *texlist;
    /* a height each form uses its own way. Chaos 2 splashes down at it. for
     * Chaos 0, it is the floor under him, as shadow_draw gives it. for
     * Chaos 6, it is the ground his leaps land on, at least 744 */
    Float ground;
    Uint8 unk_b4[4];
    Float unk_b8;
};

/* bits of a record's flags */
#define CHAOS_NO_BEADS 0x4 /* hides the ribbon's beads */
/* set as chaos_set_material draws a blinking node. the next node that
 * doesn't blink clears it and puts the record's first color back */
#define CHAOS_BLINKED 0x10

/* the attrflags bit of a material that chaos_set_material blinks */
#define MATERIAL_BLINKS 0x1

/* a melt, a rise or a hit runs its time from 0 to MELT_END. a melt's or a
 * hit's motion pace is sqrt(time) * MELT_PACE, from 0 to 1. a rise's runs
 * back from 1 to 0. Chaos 0, 2 and 6 melt and rise, and Chaos 0 and 2 take
 * hits */
#define MELT_END 10000.0F
#define MELT_PACE 0.01F

/* how chaos_puddle_deform deforms a puddle */
enum puddle_mode {
    PUDDLE_MAKE, /* keep the model's own points, then reset */
    /* restore the points, put the whole trail at the work, then flow */
    PUDDLE_RESET,
    PUDDLE_FLOW, /* the points trail the work's last places, on the ground */
    PUDDLE_FREE /* restore the model's own points, and free the kept ones */
};

/* a point on a node of the boss's pose, for chaos_track_points. a table of
 * them ends at a node below 0 */
struct boss_mark {
    Sint8 node;
    Sint8 axis; /* which of the node's axes the point takes: 0 x, 1 y, 2 z */
    NJS_POINT3 pos; /* in the node's frame */
};

/* list object's tree into list, depth first, and end the list with a null.
 * returns the number of nodes listed. list must have room for them and the
 * null, and no node may lie more than 32 below object: neither is
 * checked */
int chaos_list_nodes(NJS_OBJECT *object, NJS_OBJECT **list);
/* make form the Chaos that fights. this sets chaos_current_form,
 * chaos_params and his record. it gives his puddle and blob the form's
 * texture, and clears the emeralds. only Chaos 4 is placed, face to face
 * with player 0 */
void chaos_setup(struct task_work *work, struct chaos *chaos, int form);
/* put player 0 at the act's start and Chaos at his form's start, and turn
 * each to face the other */
void chaos_face_off(struct task_work *work);
/* deform object, a puddle's model, as chaos->puddle_mode says. the deform
 * keeps the model's own points, the puddle's trail and its ground heights
 * in statics, so only one puddle deforms at a time. PUDDLE_MAKE allocates
 * the points' buffer, unchecked, and PUDDLE_FREE frees it: run PUDDLE_FREE
 * once for each PUDDLE_MAKE */
void chaos_puddle_deform(struct task_work *work, struct chaos *chaos,
                         NJS_OBJECT *object);
/* draw the three axes as lines from -scale to scale, in color, or in white
 * when color is null */
void chaos_draw_axes(Float scale, NJS_COLOR *color);
/* move boss_points onto marks, one point for each mark before the first
 * whose node is below 0. at most BOSS_POINTS marks may come before it,
 * unchecked. each point keeps its last pos in last. its new pos is its
 * mark's pos on the mark's node of the last pose. its axis is the axis the
 * mark picks, turned as that node is. the pose stands at the work's pos,
 * turned by the work's ang.z and then its ang.x. the pose then turns about
 * y by 180 degrees less ang.y, or by 90 degrees less for Chaos 6. offset
 * raises the pose along its turned y */
void chaos_track_points(struct task_work *work, struct boss_mark *marks,
                        Float offset);
/* make a task at level with step, at source's place. a non-null offset is
 * added to source's position after the copy, not to the task's. returns the
 * task, or null */
struct task *chaos_create_task(int level, void (*step)(struct task *task),
                               struct task *source, NJS_POINT3 *offset);
/* make a child of parent with step, and flags as task_create_child takes
 * them, at parent's place. a non-null offset is added to parent's position
 * after the copy, not to the child's. returns the child, or null */
struct task *chaos_create_child(Uint16 flags, void (*step)(struct task *task),
                                struct task *parent, NJS_POINT3 *offset);
/* animate the water. every fourth call, the puddle texture in chaos's
 * texlist moves on to the next of CHAOS_SURFACE's six frames. the last
 * chaos_setup chose that texture by its form, and nothing checks it
 * against the texlist */
void chaos_animate_surface(struct chaos *chaos);
/* a motion callback for chaos_record's figure. a node whose first material
 * has MATERIAL_BLINKS shows white while the record's blink is 1 and its
 * steps odd. the next node without it puts the record's first color back */
void chaos_set_material(NJS_OBJECT *object);
/* keep point within radius of center, measured across, until the task is
 * destroyed. the task keeps a copy of center. a point pulled back below
 * the ground there rises to it. point must outlive the task. returns the
 * task, or null */
struct task *chaos_tether_create(NJS_POINT3 *point, NJS_POINT3 *center,
                                 Float radius);
/* set Ninja's 3D controls and constant attributes for drawing a Chaos, and
 * a constant material of 0 */
void chaos_draw_begin(void);
/* the distance from from to to in x and z alone.
 * chaos_distance_across_squared returns its square */
Float chaos_distance_across(NJS_POINT3 *from, NJS_POINT3 *to);
Float chaos_distance_across_squared(NJS_POINT3 *from, NJS_POINT3 *to);

#endif
