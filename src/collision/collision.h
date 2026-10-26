/* collision records, registration and the checks each step runs */
#ifndef COLLISION_COLLISION_H
#define COLLISION_COLLISION_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* the kinds of collision record. collision_register enters a record in
 * its kind's list, and the checks test each list against others. the
 * comment beside a kind names the kinds its records are tested against */
#define COLLISION_PLAYER 0 /* every kind */
/* a player's attack, like Sonic's charge */
#define COLLISION_ATTACK 1 /* every kind but walls and attacks */
/* an item box, a target, a switch */
#define COLLISION_ITEM 2 /* players and attacks */
#define COLLISION_ENEMY 3 /* players, attacks, enemies and all objects */
#define COLLISION_OBJECT 4 /* players, attacks and enemies */
#define COLLISION_OBJECT_MUTUAL 5 /* players, attacks, enemies, each other */
/* an emblem, a freed animal, a warp */
#define COLLISION_PICKUP 6 /* players and attacks */
#define COLLISION_RING 7 /* players and attacks */
/* walls and bounds, which only a player's first shape meets */
#define COLLISION_WALL 8 /* players */

/* a ring's work id. the checks list a target with this id near a player in
 * near_rings, and any other target in near_tasks */
#define RING_ID 11

/* in a task's work flags, set by the checks and cleared by
 * collision_register. in a player's flags, COLLISION_DAMAGED is the bit
 * player.h names PLAYER_HURT */
#define COLLISION_DAMAGED 0x4 /* a shape of its record was hurt */
#define COLLISION_TOUCHING 0x8 /* its record touched another */
#define COLLISION_UNDER 0x10 /* its record is under another */
#define COLLISION_ON_TOP 0x20 /* its record is on top of another */
#define COLLISION_TOUCHED                                                      \
    (COLLISION_TOUCHING | COLLISION_UNDER | COLLISION_ON_TOP)

/* in a record's flags, set by the checks and cleared by collision_register */
#define COLLISION_HIT 0x2 /* it touched another record this step */
#define COLLISION_HIT_TOP 0x4 /* it was touched at its top */
#define COLLISION_HIT_SIDE 0x8 /* at a side */
#define COLLISION_HIT_BOTTOM 0x10 /* at its bottom */
#define COLLISION_HIT_END 0x40 /* at a cylinder2's flat end */
#define COLLISION_PUSHED 0x80 /* another record's shape pushed it */
/* later touches this step leave its hit, own_shape and hit_shape as they
 * are */
#define COLLISION_HIT_KEPT 0x4000
/* in a record's flags, from collision_init. the record's shapes are its own
 * copy, which collision_free frees */
#define COLLISION_OWN_SHAPES 0x20
/* a target, which a player can aim at. collision_init makes the items, the
 * enemies and the rings targets. the checks list the targets near players
 * 0 and 1 */
#define COLLISION_TARGET 0x800

/* the kind of a shape, in the low four bits of its flags. the axis of a
 * cylinder2 or a capsule turns with its task's angles and its own. a
 * rectangle2 also tilts about z, by its own ang.z */
#define SHAPE_KIND 0xf
#define SHAPE_SPHERE 0
#define SHAPE_CYLINDER 1 /* upright cylinder */
#define SHAPE_CYLINDER2 2 /* cylinder whose axis turns */
#define SHAPE_CAPSULE 3 /* cylinder whose axis turns, with round ends */
#define SHAPE_RECTANGLE 4 /* box turned about y */
#define SHAPE_RECTANGLE2 5 /* box turned about y, then tilted about z */
/* a face across its x and y, which pushes a player out along its z */
#define SHAPE_WALL 6
#define SHAPE_BOUND 7 /* circle that keeps a player's first shape inside */
/* a bound draws as BOUND_SEGMENTS lines round its circle, each spanning
 * BOUND_TURN */
#define BOUND_SEGMENTS 32
#define BOUND_TURN (0x10000 / BOUND_SEGMENTS)

/* in a shape's flags, above its kind */
#define SHAPE_OFF 0x10 /* the checks pass it over */
/* its center is a world position, and its task's angles don't turn it */
#define SHAPE_IN_WORLD 0x20
#define SHAPE_TURN_YZX 0x40 /* its angles apply in y-z-x order, not z-x-y */
#define SHAPE_HURTABLE 0x80 /* another shape's attack can hurt it */
#define SHAPE_DRAWN 0x100 /* drawn by this step's checks */
/* its center is an offset from its task's position, and its task's angles
 * turn neither the offset nor the shape */
#define SHAPE_UNTURNED 0x2000
/* the checks pass the shape over against the kinds beside each */
#define SHAPE_IGNORE_PLAYERS 0x10000 /* players and attacks */
#define SHAPE_IGNORE_ENEMIES 0x20000 /* items and enemies */
#define SHAPE_IGNORE_OBJECTS 0x40000 /* both kinds of object */
#define SHAPE_IGNORE_OTHERS 0x80000 /* pickups and rings */
/* a player it pushes back at its side doesn't start to push against it, as
 * players do against other shapes */
#define SHAPE_NO_PUSH 0x800
/* a record that touches the shape takes COLLISION_HIT_KEPT. its hit is
 * then the shape's record, unless it already held a player's or an
 * attack's record, which hit keeps against other kinds */
#define SHAPE_KEEPS_HIT 0x200000
/* on a record's first shape, Knuckles can dig while he stands on the
 * record's task */
#define SHAPE_DIGGABLE 0x400000
/* an enemy hurt by a player's or an attack's shape with this flag is thrown
 * into the air. without the flag, the enemy bounces the player off */
#define SHAPE_THROWS_ENEMIES 0x200

/* a shape's levels: its attack in the low two bits, its guard in the two
 * above them, then the damage flags. one shape hurts another when its
 * attack exceeds the other's guard, the other is SHAPE_HURTABLE, and the
 * first's SHAPE_HURTS_* flags name the other's kind */
#define SHAPE_ATTACK 0x3
#define SHAPE_GUARD 0xc
/* an attack level in SHAPE_ATTACK's bits, and a guard level shifted into
 * SHAPE_GUARD's */
#define ATTACK(level) (level)
#define GUARD(level) ((level) << 2)
#define SHAPE_HURT 0x10 /* its latest touch this step hurt it */
#define SHAPE_HURTS_PLAYERS 0x20
#define SHAPE_HURTS_ENEMIES 0x40 /* enemies and items */
#define SHAPE_HURTS_OBJECTS 0x80 /* both kinds of object */

/* a shape's push. its low four bits name the groups of kinds the shape
 * pushes, a bit for each. PUSH_FIRM of a group sets that group's bit in the
 * high four, and the group's shapes then can't push it. attacks are in no
 * group */
#define PUSH_PLAYERS 0x1
#define PUSH_ENEMIES 0x2 /* items and enemies */
#define PUSH_OBJECTS 0x4 /* both kinds of object */
#define PUSH_OTHERS 0x8 /* pickups and rings */
#define PUSH_ALL 0xf
#define PUSH_FIRM(groups) ((groups) << 4)

/* one shape, as a task describes it to collision_init */
struct collision_shape {
    Uint32 flags; /* its kind in the low four bits, then SHAPE_* */
    Uint8 levels; /* its attack and guard, then SHAPE_HURT and the rest */
    Uint8 push; /* the groups it pushes, and PUSH_FIRM of those it resists */
    NJS_POINT3 center; /* task-relative unless SHAPE_IN_WORLD */
    Float radius; /* its radius, or half its size along x */
    Float size_y; /* half its sizes along y and z, where its kind has them */
    Float size_z;
    /* its own angles, which turn it within the frame its task's angles
     * turn */
    struct angles ang;
    /* the record it touched this step and the index of that record's shape,
     * or null and -1. only a touch of this shape sets them, as it sets its
     * record's hit and hit_shape */
    Sint16 hit_shape;
    struct collision_work *hit;
};

/* a task's collision record, at its work's collision */
struct collision_work {
    Uint16 kind; /* COLLISION_PLAYER to COLLISION_WALL */
    Uint16 flags; /* COLLISION_HIT and the rest */
    Uint16 count; /* how many shapes it has */
    Uint16 current; /* the index of the shape the running check tests */
    Float radius; /* how far its shapes reach from the task's position */
    struct collision_shape *shapes; /* owned only with COLLISION_OWN_SHAPES */
    struct task *owner;
    /* the record it touched this step and the indexes of the two shapes that
     * touched, or null and -1. a later touch this step replaces all three,
     * unless COLLISION_HIT_KEPT is set. hit keeps a player's or an attack's
     * record against another kind's, but own_shape and hit_shape take the
     * later touch's indexes */
    Sint16 own_shape;
    Sint16 hit_shape;
    struct collision_work *hit;
};

/* a task near a player, and how far */
struct near_task {
    struct task_work *work;
    Float distance; /* squared, between the two tasks' positions */
};

/* the most entries a player's near_tasks list and near_rings list hold.
 * an entry whose work is null ends each, and isn't counted */
#define NEAR_TASKS 1160
#define NEAR_RINGS 512

/* the targets within 100 of player 0, in the order this step's checks met
 * them, and in the _player1 lists those of player 1. a target whose work id
 * is RING_ID goes in a near_rings list, and any other in a near_tasks list.
 * player 0 goes in player 1's near_tasks when its record is a target */
extern struct near_task near_tasks[NEAR_TASKS + 1];
extern struct near_task near_rings[NEAR_RINGS + 1];
extern struct near_task near_tasks_player1[NEAR_TASKS + 1];
extern struct near_task near_rings_player1[NEAR_RINGS + 1];

/* the records collision_register has entered this step: a list for each
 * kind, and the count each list holds. stage.h declares the item and enemy
 * counts. other files read only the item and enemy lists.
 *
 * SHC lays out bss in the order names are first declared. so the counts
 * and lists that lie before the item and enemy lists are declared here,
 * ahead of them */
extern Uint16 collision_player_count;
extern Uint16 collision_attack_count;
extern Uint16 collision_object_count;
extern Uint16 collision_mutual_object_count;
extern Uint16 collision_pickup_count;
extern Uint16 collision_ring_count;
extern Uint16 collision_wall_count;
extern struct collision_work *collision_players[8];
extern struct collision_work *collision_attacks[128];
extern struct collision_work *collision_items[128];
extern struct collision_work *collision_enemies[256];

/* the models the checks draw shapes with, which other files draw too. each
 * is 20 across but the cube, which is 10 */
extern NJS_OBJECT marker_object; /* a sphere */
extern NJS_OBJECT marker_cylinder_object; /* a cylinder 20 high */
/* @hack type: an array of one, whose address SHC keeps in a register in a
 * function that draws it more than once, as shape_draw_wall keeps it in R8
 * (0x8c02fade); declared as one object, 800 of the 5001 words of the 25
 * functions that draw it differ */
extern NJS_OBJECT marker_cube_object[1];
extern NJS_OBJECT marker_ball_object; /* a rougher sphere */
extern NJS_OBJECT marker_dome_object; /* a half sphere, a capsule's end */
extern NJS_OBJECT marker_tube_object; /* a tube 20 high, a capsule's side */

/* a radius or a half size times MARKER_FIT scales a marker to span the
 * shape, and times CUBE_FIT the cube */
#define MARKER_FIT 0.1F
#define CUBE_FIT 0.2F

/* draw a sphere of the record's radius about its task's position */
void collision_draw_radius(struct collision_work *collision);
/* set the record's radius from its shapes: 1.01 times the farthest any
 * shape reaches from the task's position, or 25 when that is under 0.1.
 * call it after the shapes change */
void collision_set_radius(struct collision_work *collision);
/* give task's work a record of kind, with its own copy of count shapes. the
 * items, enemies and rings become targets. a player's or an attack's
 * record takes a radius of 25, and any other the one collision_set_radius
 * finds. unused is never read.
 *
 * count must be positive, and the work must have no record, which this
 * would leak. when the record can't be allocated, the game hangs, printing
 * a message while debug_hidden is 0 or more. when the copy can't be,
 * work->collision stays as it was and the new record leaks. most of the
 * game's callers leave work->collision unchecked */
void collision_init(struct task *task, const struct collision_shape *shapes,
                    Uint8 kind, int unused, Uint8 count);
/* clear the record's hits and collision flags, then enter it in its kind's
 * list for this step's checks. a record whose list is full is cleared and
 * left out. while the set editor is open, or without a record, nothing
 * changes. a registered task must live until collision_collide_all or
 * collision_clear_lists empties the lists. a record takes part in the
 * checks only in steps it registers in. its hits stay valid until it
 * registers again, and only while the hit task's record lives */
void collision_register(struct task_work *work);
/* empty the lists collision_register fills */
void collision_clear_lists(void);
/* the player whose record is the record's hit, 0 or 1, or -1 for any other
 * hit, for none and without a record */
int collision_get_hit_player(struct task_work *work);
/* where the shape's center is in the world, as its task stands now */
void collision_get_shape_center(struct task_work *work,
                                struct collision_shape *shape, NJS_POINT3 *pos);
/* run this step's checks, unless the set editor is open, and empty the
 * lists either way */
void collision_collide_all(void);
/* free the work's record, and its shapes if it owns them, and clear
 * work->collision. this step's lists and other records' hits keep pointing
 * at the freed record */
void collision_free(struct task_work *work);
/* set *normal to the direction of the record's hit, a unit vector, and
 * return 1. against a hit sphere, it points from the center of the
 * record's first shape to the sphere's. against a rectangle, it is the
 * outward normal of the face whose side the task's position is on. against
 * the other kinds, it lies across x and z, out of the hit shape toward the
 * first shape's center. centers that meet give (0, 0, 1).
 *
 * it returns 0, with *normal unchanged, when the record has no hit or no
 * shapes, or the hit shape is a wall or a bound. the result is a Sint8,
 * which callers sign-extend to test it.
 *
 * the work must have a record, and its hit_shape must index a shape of its
 * hit. the function refuses a hit_shape over the hit's count, but not the
 * count itself or one below 0 */
Sint8 collision_get_hit_normal(struct task_work *work, NJS_VECTOR *normal);

#endif
