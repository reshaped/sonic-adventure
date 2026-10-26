/* the collision world: the act's land, which it draws too, the objects
 * tasks put in it, and gravity */
#ifndef COLLISION_WORLD_H
#define COLLISION_WORLD_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;

/* in a world object's flags: what a player stands on and runs into */
#define WORLD_SOLID 0x1
/* water: a player swims in it, and a carried thing falls through it */
#define WORLD_WATER 0x2
/* the ground a player grips least, and ground with little grip: a player
 * slides on both */
#define WORLD_GRIP_LEAST 0x4
#define WORLD_GRIP_LOW 0x8
/* sand, which a board throws up and a player grips half as well */
#define WORLD_SAND 0x10
/* ground a player grips more than most */
#define WORLD_GRIP_HIGH 0x80
/* ground a player runs on as if it were level, standing square to it at
 * any slope */
#define WORLD_NO_SLOPE 0x4000
/* ground that hurts a player */
#define WORLD_HURTS 0x10000
/* ground a player clings to at any slope: they fall into it, not along
 * gravity, and the stick steers them across its face */
#define WORLD_CLING 0x20000
/* ground that takes footprints */
#define WORLD_FOOTPRINTS 0x100000
/* an object that turns a player who runs into it aside at any speed, and
 * never crashes them */
#define WORLD_DEFLECT 0x200000
/* the grounds a player slides on */
#define WORLD_SLIPPERY (WORLD_GRIP_LEAST | WORLD_GRIP_LOW | WORLD_SAND)
/* a liquid thicker than water: it holds a player back more */
#define WORLD_THICK_LIQUID 0x400000
/* the liquids, which most tests take together */
#define WORLD_LIQUID (WORLD_WATER | WORLD_THICK_LIQUID)
/* an object that moves or changes shape: its collision mesh is made again
 * each step */
#define WORLD_MOVES 0x08000000
/* ground Knuckles can dig into, unless it also moves */
#define WORLD_DIGGABLE 0x100
/* an object a gliding Knuckles bounces off rather than climbing */
#define WORLD_UNCLIMBABLE 0x1000
/* what a gliding Knuckles bounces off: an unclimbable object, one that moves,
 * or one that deflects */
#define WORLD_NO_CLIMB (WORLD_MOVES | WORLD_DEFLECT | WORLD_UNCLIMBABLE)
/* an object's size, a 2-bit number at bit 29: WORLD_HUGE is 0 and
 * WORLD_SMALL is 3. collision/polygon.c's SIZE_REACH2 looks up a reach in
 * size_reach2 by it. it shifts the size down 13 places, not 29, so every
 * size but WORLD_HUGE reads past the table */
#define WORLD_SIZE 0x60000000
#define WORLD_HUGE 0
#define WORLD_LARGE 0x20000000
#define WORLD_MEDIUM 0x40000000
#define WORLD_SMALL 0x60000000
/* the longest reaches that take WORLD_SMALL, WORLD_MEDIUM and WORLD_LARGE,
 * by which an object that sizes itself picks its size. a longer reach takes
 * WORLD_HUGE. size_reach2 holds each reach a hundredth longer, squared */
#define WORLD_SMALL_REACH 30.0F
#define WORLD_MEDIUM_REACH 100.0F
#define WORLD_LARGE_REACH 200.0F
/* the object is turned, and its model's center is off its origin.
 * world_collect_near_players and world_list_near turn that center by the
 * object's angles before they measure to it. world_add sets it where the
 * center lies 15 or more from the origin and the object is turned. it is
 * turned when any of its three angles is 16 or more, mod 0x10000 */
#define WORLD_TURNED 0x10000000

/* an object of the collision world: a land entry's, or one a task put in it */
struct world_object {
    Sint32 flags;
    NJS_OBJECT *object;
    struct task *owner; /* null for the land */
};

/* in a land entry's flags, beside its world flags: the entry is drawn */
#define LAND_DRAWN 0x80000000
/* the land's task measures from a point half the clip ahead of the camera.
 * it draws an entry whose sphere comes within half the clip of that point,
 * or with LAND_FAR, within ninja's far clip. the clip is the land's own with
 * LAND_CLIP, else ninja's far clip */
#define LAND_FAR 0x20

/* an object of an act's land, and the sphere round it */
struct land_entry {
    NJS_POINT3 center;
    /* the sphere's radius. with size_y or size_z set, it is a box's size
     * along x, and world_compute_bounds takes the length of the three as
     * the radius */
    Float radius;
    Float size_y;
    Float size_z;
    NJS_OBJECT *object;
    /* the land blocks the entry is drawn in, tested against
     * world_land_blocks, or 0 for all of them */
    Sint32 blocks;
    Sint32 flags;
};

/* a part of the land that moves */
struct land_motion {
    Float frame;
    Float speed; /* the frames it advances each step */
    Float frames; /* its motion's nbFrame */
    NJS_OBJECT *object; /* what motion moves, with LAND_FILE */
    union {
        NJS_ACTION *action; /* without LAND_FILE */
        NJS_MOTION *motion; /* with LAND_FILE */
    } play;
    NJS_TEXLIST *texlist;
};

/* in a land's flags */
#define LAND_MOTIONS 0x1 /* it has parts that move */
#define LAND_TEXLIST 0x2 /* its texlist loads directly, not from a pvm */
#define LAND_CLIP 0x4 /* it has a clip of its own */
#define LAND_PVM 0x8 /* its texlist loads from the pvm it names */
/* its entries' and motions' objects, and the motions they play, are offsets
 * into the file it names. world_load_act reads the file and turns the
 * offsets into pointers */
#define LAND_FILE 0x10

/* a land's users: how many loads hold its textures, in LAND_TEXTURE_USERS,
 * and how many hold its file, LAND_FILE_USER apiece in LAND_FILE_USERS */
#define LAND_TEXTURE_USERS 0x3
#define LAND_FILE_USERS 0xc
#define LAND_FILE_USER 4

/* an act's land, which a stage's module holds */
struct land_table {
    Sint16 count; /* of entries */
    Sint16 motion_count;
    Sint16 flags;
    Sint16 users;
    Float clip; /* how far it's drawn, with LAND_CLIP */
    struct land_entry *entries;
    struct land_motion *motions;
    char *pvm; /* the pvm its texlist loads from, with LAND_PVM */
    NJS_TEXLIST *texlist;
    char *file; /* with LAND_FILE */
    void *data; /* the file, once it's loaded */
};

/* the blocks of the land that are drawn, a bit each, as a stage's module
 * sets them */
extern Sint32 world_land_blocks;

/* the world's objects, as the last collect or list found them */
#define WORLD_OBJECTS 1024
extern Sint16 world_count;
extern struct world_object world_objects[WORLD_OBJECTS];

/* in a task's work flags. without it, world_collect_near_players passes over
 * the objects the task put in the world. world_add sets it */
#define WORLD_TESTED 0x100

/* the step of the task that loads the act's land, draws it and plays its
 * moving parts */
void world_land_step(struct task *task);
/* load an act's land, or release it. an act without land does nothing. the
 * land counts its loads in users. the release that brings a count to 0 frees
 * the file, or releases the texlist with LAND_TEXLIST or LAND_PVM. a land with
 * LAND_PVM and no pvm loads no textures. a land with LAND_MOTIONS and neither
 * flag loads each motion's texlist on every load, uncounted. world_release_act
 * then releases the first motion's texlist once for each entry, and no
 * other's */
void world_load_act(int stage, int act);
void world_release_act(int stage, int act);
/* release the current act's land, and have the land's task load the land
 * that many acts on at its next step. the land's task must be running */
void world_change_act(Sint16 by);
/* hide or show the land. its moving parts stand still while it's hidden */
void world_hide_land(void);
void world_show_land(void);
/* set the gray of the land's constant material */
void world_set_land_material(Float gray);
/* keep the objects near the players for the lists, and list them in
 * world_objects. it keeps up to 128 objects of tasks with WORLD_TESTED, and
 * up to 128 of the land's solid and liquid entries. an object is near when
 * its sphere comes within the near radius of the first point, or within 40
 * of the second. the first point is player 0, else player 1, else the
 * camera. the second is player 1, when player 0 is there too */
void world_collect_near_players(void);
/* keep and list every task's object and every solid or liquid land entry,
 * up to the same counts, whatever the task's flags */
void world_collect_all(void);
/* list in world_objects the kept objects whose spheres come within radius
 * of the point, or within radius + 30 for a task's object. world_list_all
 * lists every kept object */
void world_list_near(Float x, Float y, Float z, Float radius);
void world_list_all(void);
/* leave the land out of the collects and lists, or put it back */
void world_disable_land(void);
void world_enable_land(void);
/* put a task's object in the world under flags, and set WORLD_TESTED on the
 * task. while tasks have 256 objects in the world, it does nothing */
void world_add(Sint32 flags, struct task *task, NJS_OBJECT *object);
/* take out the last object the task put in the world. then have the
 * collision meshes forget object, which need not be the one taken out. a
 * task with none in the world changes nothing */
void world_remove(struct task *task, NJS_OBJECT *object);
/* lend an object from a pool of 256, or return null when all are lent; or
 * take one back. a lent object's model, child and sibling are null, and the
 * rest is as its last borrower left it. the land's task takes every object
 * back as it starts. most of the game's callers leave world_object_alloc's
 * result unchecked */
NJS_OBJECT *world_object_alloc(void);
void world_object_free(NJS_OBJECT *object);
/* point world_gravity straight down, or point it straight down tilted by
 * ang_x about x and ang_z about z, whatever it pointed at before. the tilt
 * then works world_gravity_ang_x and world_gravity_ang_z back out of the new
 * vector */
void world_reset_gravity(void);
void world_tilt_gravity(Angle ang_x, Angle ang_z);
/* fit a sphere round each entry of each act's land in the current stage. an
 * entry without a model, or with only a radius, keeps its sphere. one with
 * no radius or box takes its model's sphere, and one with a box the sphere
 * round the box. the center then moves as the object does, turned about x
 * and y only. a second call moves a box's sphere again and grows it. the
 * collects and lists then see no land until the land's task starts */
void world_compute_bounds(void);
/* set the near radius, or put it back to 400, as the land's task does when
 * it starts */
void world_set_near_radius(Float radius);
void world_reset_near_radius(void);

#endif
