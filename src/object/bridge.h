/* the rope bridge that objects of STAGE_WINDY, STAGE_MOUNTAIN and
 * STAGE_ICECAP build, such as O TURI1. planks hang between posts, and ropes
 * run from plank to plank. a copy of the stage's model, in the collision
 * world, moves its points with the planks */
#ifndef OBJECT_BRIDGE_H
#define OBJECT_BRIDGE_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;
struct task_work;

/* a bridge's params. bridge_init gives defaults to parts without any. only
 * width is read: bridge_swing uses its own constants where it could read
 * the defaults' unk_00[3] to [6] and unk_20[0], whose values they share */
struct bridge_params {
    Float unk_00[7];
    /* how far a plank's feet stand to either side of its middle */
    Float width;
    Float unk_20[2];
};

/* the span along one axis, min and max excluded, where player 0 can stand
 * on the bridge */
struct bridge_range {
    Float min;
    Float max;
};

/* what a stage builds its bridge from, put in BRIDGE_VARS(work)->parts
 * before bridge_init. the model has four points to a plank. each plank
 * takes its corners' point numbers from first, last or middle, by enum
 * bridge_corner */
struct bridge_parts {
    Sint16 first[2][4]; /* the first two planks' */
    Sint16 last[2][4]; /* the last two planks' */
    Sint16 middle[4]; /* the others': plank i's are these plus 4 * i */
    struct bridge_range box[3]; /* x, y and z, in the bridge's frame */
    Float length; /* along x, from the first plank to the last */
    Float scale; /* the planks' scale */
    NJS_TEXLIST *texlist;
    NJS_MODEL *model;
    NJS_MODEL *post;
    NJS_MODEL *rope;
    NJS_MODEL *plank;
    struct bridge_params *params;
};

/* a plank's corners in the model. the tops of its posts carry the ropes,
 * and the posts' feet stand params->width to either side */
enum bridge_corner {
    CORNER_TOP_PLUS_Z,
    CORNER_FOOT_PLUS_Z,
    CORNER_FOOT_MINUS_Z,
    CORNER_TOP_MINUS_Z
};

struct bridge_plank {
    Sint16 fixed; /* set on the first and last planks, which don't move */
    Sint16 corner[4]; /* model points, by enum bridge_corner */
    Float height; /* its middle's y, in the bridge's frame */
    Float speed; /* upward, per step */
    Float force; /* upward, summed this step */
    Float torque; /* about x, summed this step */
    Float tilt; /* about x, in degrees */
    Float spin; /* change in tilt per step */
    /* how tall its posts stand, least at the bridge's middle */
    Float post;
    Uint8 unk_28[4];
};

/* a bridge's work->vars. the stage sets parts before bridge_init, which
 * sets the rest. BRIDGE_PLANK_COUNT is the planks' count, in work->timer.
 * the bridge keeps where player 0 last stood on it in work->scl.x and
 * scl.z, which nothing reads. it counts the player's steps there in
 * work->tick, from 0 to 11 and over again */
struct bridge_vars {
    /* its own copy of the parts' model, in the collision world */
    NJS_OBJECT *object;
    struct bridge_plank *planks;
    struct bridge_parts *parts;
};

#define BRIDGE_VARS(work) ((struct bridge_vars *)&(work)->vars)
#define BRIDGE_PLANK_COUNT(work) TASK_TIMER(work)

/* a falling plank's work->vars. its work->scl.x is its scale, and
 * work->tick counts down while it waits to fall */
struct plank_vars {
    NJS_MODEL *model;
    Uint8 unk_04[4];
    NJS_TEXLIST *texlist;
};

#define PLANK_VARS(work) ((struct plank_vars *)&(work)->vars)

/* a falling plank's view of its task_motion: its speed, and its spins
 * about x and y, per step */
struct plank_motion {
    Uint8 unk_00[4];
    NJS_VECTOR speed;
    Uint8 unk_10[0x18];
    Angle spin_x;
    Angle spin_y;
};

/* draw the parts' model where the bridge stands, for the set editor */
void bridge_edit_draw(struct task_work *work);
/* set the task up as a bridge from the parts in its vars. it takes the
 * bridge's step, draw and destructor, and a stage may set its own step
 * after. the heap and the collision world's pool of objects must have
 * room: with either out, it writes through null */
void bridge_init(struct task *task);
/* a stage's step runs one of these for its bridge: whole, or once its
 * planks have fallen. each draws the bridge, swings its planks under
 * player 0's weight and moves the model's points to them. each sets
 * WORLD_TESTED in work->flags while a player is near the model, and clears
 * it otherwise. once the camera and player 0 are both more than 2000 from
 * the bridge, it destroys itself on its next step */
void bridge_step_whole(struct task *task);
void bridge_step_broken(struct task *task);
/* the step of a plank that bridge_collapse drops. the plank stays where it
 * is for work->tick + 1 steps, then falls for 92 steps and is killed */
void bridge_plank_step(struct task *task);
/* drop each plank but the first and the last as a falling plank task,
 * with dust. the planks from 34 to 46 fall first, together, and the rest
 * follow out from them, fewer to a step farther out. the bridge keeps its
 * own planks, which bridge_step_broken doesn't draw */
void bridge_collapse(struct task_work *work);

#endif
