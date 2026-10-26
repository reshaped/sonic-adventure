/* a garden's Chao or egg: the state its task keeps beside its record, as
 * far as the code reads it, and the tasks' creates */
#ifndef AL_MAIN_CHAO_H
#define AL_MAIN_CHAO_H
#include <ninja.h>
#include <sg_xpt.h>

#include "animal/animal.h"
#include "chao/chao.h"
#include "collision/collision.h"
#include "collision/polygon.h"
#include "system/task.h"

/* bits of struct alife_chao's flags */
/* its name fades in while this is set, for 150 frames at most, and out
 * otherwise */
#define CHAO_SHOW_NAME 0x2
/* in a cocoon: the draw shows the cocoon, and neither moves nor draws the
 * icon */
#define CHAO_IN_COCOON 0x4
/* nothing sets or tests it. chao_init and the card menu's preview_setup
 * clear it */
#define CHAO_UNK_8 0x8
#define CHAO_SHAPE_SHOWN 0x10 /* its shape draws only while this is set */
/* set once shakes have pleased it as it is held. it counts no more shakes
 * until it is picked up again */
#define CHAO_SHAKEN 0x20
/* alife_chao_show and alife_chao_draw take it as at distance 0, so they
 * never draw a far tree for it */
#define CHAO_NO_FAR_TREES 0x100

#define CHAO_ON_GROUND 0x1 /* in its work's flags, not struct alife_chao's */

/* in its player_flags: within 100 of player 0 */
#define CHAO_NEAR_PLAYER 0x1

/* in its handling: what a player has done with it */
enum chao_handling {
    HANDLING_FREE = 0,
    HANDLING_HELD = 1,
    HANDLING_THROWN = 2,
    HANDLING_VMS = 3, /* let go on VMS, which can take it in */
    /* an egg a cocoon has just made, which keeps still until the cocoon is
     * gone */
    HANDLING_REBORN = 5
};

/* in its move: how it makes for its target. each change of move clears
 * its work's submode and its mode_timer */
enum chao_move {
    MOVE_WALK = 0,
    /* a walker whose record's skills.run is below 1000 stops now and then.
     * it makes a face, then goes back to MODE_IDLE */
    MOVE_STOP = 1,
    MOVE_FLY = 2,
    MOVE_3 = 3, /* move.c's two switches do nothing at it */
    MOVE_SWIM = 4,
    MOVE_DESCEND = 6, /* once a flight is over */
    MOVE_CLIMB = 7
};

/* in its motion's flags */
#define MOTION_BLEND 0x1 /* blending into next */
#define MOTION_ENDED 0x2 /* stopped at its end */
#define MOTION_CHANGED 0x8 /* current just changed */
#define MOTION_NEW 0x10 /* a new one is asked for in id */
#define MOTION_DONE 0x40 /* came round to its end this step */

enum motion_end { END_LOOP, END_BLEND, END_CHANGE, END_STOP };

struct chao_motion_entry {
    NJS_MOTION *motion;
    Sint16 end; /* an enum motion_end */
    Sint16 next; /* the index of the entry that follows */
    /* the frames it plays from and to */
    Float first;
    Float last;
    /* the frames it moves a step, by the motion's rate. below 0, it plays
     * backward */
    Float speed;
    Float blend; /* what a blend into it adds to the ratio each step */
};

/* current, next and id are indices into table */
struct chao_motion {
    Sint16 end;
    Uint16 flags;
    Uint16 timer; /* the steps since current last changed */
    Sint16 current;
    Sint16 next;
    Sint16 id; /* the motion asked for, with MOTION_NEW */
    Float frame;
    Uint8 unk_10[4];
    Float next_frame;
    Float ratio;
    Float rate;
    struct chao_motion_entry *table;
};

#define CHAO_NODES 50 /* room in objects */
/* indices in objects of the nodes the face moves */
#define NODE_EYE_0 17
#define NODE_EYE_0_TURN 18
#define NODE_EYE_1 24
#define NODE_EYE_1_TURN 25
#define NODE_MOUTH 27
#define NODE_ICON 32 /* where the icon floats */

/* a Chao's own tree, a copy of its type's growth-0 tree, and the copy's
 * nodes. the six arrays after objects hold the same nodes of the type's
 * six trees, which alife_shape_blend blends the copy from. start is the
 * tree at growth 0, and center the one at h_pos and v_pos 0. the other
 * four lean toward their skills */
struct chao_shape {
    NJS_OBJECT *model;
    NJS_OBJECT *objects[CHAO_NODES]; /* depth first */
    NJS_OBJECT *start[CHAO_NODES];
    NJS_OBJECT *center[CHAO_NODES];
    NJS_OBJECT *fly[CHAO_NODES];
    NJS_OBJECT *run[CHAO_NODES];
    NJS_OBJECT *power[CHAO_NODES];
    NJS_OBJECT *swim[CHAO_NODES];
};

/* in a face's flags */
#define FACE_FLAG_2 0x2 /* only alife_face_set_eyes touches it, to clear it */
#define FACE_OPENING 0x8
#define FACE_CLOSING 0x10

/* its eyes turn about x as they shut and about z as they slant */
struct chao_face {
    Uint16 blink_wait; /* the steps it waits between blinks */
    Uint16 blink_steps;
    Uint8 unk_04[4];
    Sint32 eyes_steps; /* the steps other_eyes last, below 0 for none */
    Sint32 mouth_steps; /* the same for other_mouth_0 and _1 */
    Sint32 look_steps; /* the same for look_x and look_z */
    Uint32 flags;
    Angle lid;
    Angle eye_x;
    Angle open_x;
    Angle look_x;
    Angle rest_x;
    Angle eye_z;
    Uint8 unk_30[4];
    Angle look_z;
    Angle rest_z;
    /* the face's textures, from here to the end */
    Sint16 eyes;
    Sint16 other_eyes;
    Sint16 mouth_0;
    Sint16 mouth_1;
    Sint16 other_mouth_0;
    Sint16 other_mouth_1;
};

/* in its terrain's flags, as alife_find_sides leaves them */
/* the nearest surface above the query's point is water, and lies less than
 * 5 above that point */
#define TERRAIN_WATER_ABOVE 0x400
#define TERRAIN_WATER_BELOW 0x800 /* the nearest surface below it is water */
/* in its terrain's flags, as alife_collide_terrain leaves them */
#define TERRAIN_CEILING 0x8 /* the ceiling pushed it down */
#define TERRAIN_FLOOR 0x10 /* the floor pushed it up */
#define TERRAIN_WALL 0x40 /* a wall pushed it */
/* a wall pushed it as it headed into the wall: its forward way and the
 * wall's normal had a dot product below -0.35 */
#define TERRAIN_FACING_WALL 0x80
/* water_level was more than 1 above its pos. alife_find_sides sets it too,
 * with either of the water flags above */
#define TERRAIN_DEEP 0x1000

/* the ground and water around a Chao, and what they do to it */
struct chao_terrain {
    Uint16 flags;
    Uint8 unk_02[2];
    Float unk_04;
    Float query_height; /* how far above its work's pos query looks from */
    Float unk_0c;
    Float head_height; /* how far its head is above query's point */
    Float unk_14;
    Float unk_18;
    Uint8 unk_1c[4];
    /* a wall it moves into takes twice this share of its speed into the
     * wall: 0.5 stops that speed, and 1 reflects it */
    Float wall_bounce;
    Uint8 unk_24[0xc];
    Angle wall_turn; /* how far a wall it moves into turns its yaw */
    Float unk_34;
    Float unk_38;
    /* as alife_collide_terrain leaves it: the highest water surface above or
     * below the query's point, at most 0, or -GROUND_FAR with none */
    Float water_level;
    struct sides_query query;
};

/* the ball or the symbol of a Chao's icon */
struct icon_part {
    Uint16 frames; /* the frames it has shown */
    Uint16 texture;
    Uint16 state;
    Uint8 unk_06[10];
    NJS_POINT3 pos; /* from the icon's */
    NJS_VECTOR speed;
    NJS_VECTOR size;
    NJS_VECTOR grow; /* how fast size changes */
};

struct chao_icon {
    Uint8 unk_00[2];
    Uint16 kind; /* an enum icon_kind */
    Uint16 timer; /* the frames the symbol stays up, counting down */
    Uint16 unk_06;
    Uint8 unk_08[4];
    NJS_POINT3 pos;
    Uint8 unk_18[0x18];
    Angle phase; /* where the ball is in its pulse */
    struct icon_part symbol;
    struct icon_part ball;
    /* the Chao's own color, which its name takes too */
    Uint8 blue;
    Uint8 green;
    Uint8 red;
    Uint8 alpha;
};

/* a Chao's or an egg's state, at its task's motion */
struct alife_chao {
    void *unk_000;
    NJS_VECTOR speed;
    NJS_VECTOR accel; /* cleared once the step has used it */
    Uint8 unk_01c[0x10];
    Angle turn_speed; /* how fast it turns toward target */
    Uint8 unk_030[4];
    Float carry_reach; /* at task_motion.drive's offset for carry_set_kind */
    Uint8 unk_038[4];
    Float gravity; /* at task_motion.gravity's offset */
    Uint8 unk_040[8];
    Sint16 mode_timer;
    /* a count that each user reads its own way. a behavior keeps in it the
     * bound it sets for its work's timer, or for its repeats. chao_held
     * counts the Chao's shakes in it, and MOVE_FLY counts its steps down in
     * it */
    Uint16 limit;
    Uint16 sound_timer; /* a behavior's count toward its next sound */
    Uint8 unk_04e[2];
    Sint16 last_mode; /* the mode it left, as its work keeps it */
    Uint8 unk_052[0xa];
    Sint16 handling; /* an enum chao_handling */
    Uint8 unk_05e[2];
    Uint32 flags;
    Uint16 move; /* an enum chao_move */
    Uint8 unk_066[6];
    /* the cocoon's scale and alpha, as alife_chao_show and alife_chao_draw
     * draw it */
    Float cocoon_scale;
    Float cocoon_alpha;
    Sint32 name_alpha; /* from 0 to 255 */
    Uint16 name_frames; /* the frames it has faded in for */
    Uint16 hit_wait; /* the calls to alife_take_hit since it was last hit */
    /* how far its shape turns about z, about a point 2.5 above its work's
     * pos. the draw turns it about x by unk_080 first, at the same point */
    Angle spin;
    Sint32 unk_080; /* no code sets it but to 0 */
    Uint8 unk_084[8];
    NJS_POINT3 home; /* its work's pos when its task was set up */
    NJS_POINT3 target; /* where it goes */
    NJS_POINT3 last_pos; /* its work's pos, as alife_save_pos last kept it */
    Float unk_0b0;
    Float unk_0b4;
    Float unk_0b8;
    Angle bob_phase; /* where MOVE_FLY is in its rise and fall */
    /* the yaw that faces target, as alife_turn_to_target last found it */
    Angle aim;
    Angle unk_0c4;
    Uint8 unk_0c8[0xc];
    /* how fast its work's scl.y springs toward 1; a jolt sets it */
    Float scale_speed;
    Float scale;
    /* the values alife_grow_step moves the record's own toward */
    Float h_pos;
    Float v_pos;
    Float growth;
    Uint16 unk_0e8;
    Uint8 unk_0ea[2];
    struct chao *record; /* the Chao as the memory card keeps it */
    Uint8 unk_0f0[0x22];
    /* a hit and fun_0c915b00 set its bit 0x1, and no code reads it */
    Uint16 unk_112;
    Uint8 unk_114[4];
    struct chao_motion motion;
    struct chao_terrain terrain;
    struct chao_shape shape;
    NJS_TEXLIST *texlist; /* the textures its shape draws with */
    struct chao_face face;
    struct chao_icon icon;
    Uint32 player_flags; /* CHAO_NEAR_PLAYER */
    struct task *task; /* the Chao's own task */
};

/* a garden Chao's three collision shapes. an egg and the race's Chao take
 * the first two */
extern struct collision_shape chao_collisions[];
/* the slots an animal's parts fill: one for CHAO_PART_FHEAD, two each for
 * the horns, ears, wings, hands and feet, and one for the tail */
#define CHAO_PART_SLOTS 12
/* by animal kind and enum chao_part, the chance, from 0 to 1, that the
 * animal gives a Chao the part */
extern Float animal_part_chances[ANIMAL_KINDS][CHAO_PARTS];
/* by animal kind to ANIMAL_LION, what the animal adds to swim, fly, run
 * and power */
extern Sint16 animal_skill_bonuses[ANIMAL_LION + 1][4];
/* 8 bytes ahead of alife_paused, which no code reads. bss follows each
 * name's first declaration, and chao.c includes this header, so they are
 * declared here, ahead of alife_paused */
extern Uint8 dat_0ca84420[8];
/* while nonzero, chao_step and egg_step stop the Chao or the egg. each
 * steps only a handling of HANDLING_VMS, then shows the Chao or the egg
 * with alife_chao_show or egg_show. chao_step still steps its children and
 * clears its entry's ENTRY_LET_GO */
extern Sint32 alife_paused;

/* create the task of record's Chao or egg, which keeps record until it is
 * destroyed, and set record's unk_17a to 1. it returns null for a type
 * past CHAO_EGG, and when the task pool or the heap runs out. when the
 * heap can't give the struct alife_chao, task_kill sets the step of the
 * task already destroyed. most of the game's callers leave the result
 * unchecked */
struct task *alife_chao_create(struct chao *record);
/* give task a struct alife_chao at its motion, and return 1. it is zeroed
 * but for record, the task, a scale of 1 and CHAO_SHAPE_SHOWN. record must
 * stay valid until the task is destroyed. when the heap can't give one, it
 * destroys task and returns 0 */
int alife_chao_alloc(struct task *task, struct chao *record);
/* set the type of the record of task's Chao to CHAO_NOTHING, and kill
 * task, which destroys itself on its next step */
void alife_chao_remove(struct task *task);

/* alife_shape_free, for the destructors of the Chao of the card menu and
 * the race */
void fun_0c9002ac(struct task_work *work, struct alife_chao *chao);
/* seed rand from the clock, then create the task of each record in
 * garden's list. each starts at a random one of the garden's spots, a
 * little off it, facing a random way. each record must be a Chao or an
 * egg, and each create must succeed: a null task is written through */
void alife_chao_create_garden(int garden);

#endif
