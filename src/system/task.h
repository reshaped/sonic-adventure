/* tasks: the game's objects, and the heap their records come from */
#ifndef SYSTEM_TASK_H
#define SYSTEM_TASK_H
#include <ninja.h>
#include <sg_xpt.h>

struct collision_work;
struct event_work;
struct placement;
struct player;
struct task;

/* root tasks are linked in eight levels, 0 to 7. task_step_all and
 * task_draw_all run level 7 first, then 0 to 6. task_create names level 7
 * TASK_LEVEL_FIRST, since its 7 means TASK_UNLINKED. task_step_level,
 * task_draw_level and task_destroy_level take either */
#define TASK_LEVELS 8
/* task_create's level for a task in no level, to link as a child */
#define TASK_UNLINKED 7
#define TASK_LEVEL_FIRST 8 /* level 7, which runs first */

/* task_create's flags: the records it allocates with a task, zeroed, from
 * the heap. task_destroy frees the records in work, motion, force and extra
 * with heap_free. a record a task puts in one of them must come from the
 * heap too */
#define TASK_MOTION 0x1
#define TASK_WORK 0x2
#define TASK_FORCE 0x4
#define TASK_EXTRA 0x8

/* the struct task_force records TASK_FORCE allocates, and the bytes of
 * TASK_EXTRA's record */
#define TASK_FORCES 2
#define TASK_EXTRA_SIZE 16

struct angles {
    Angle x, y, z;
};

/* twelve bytes of variables, which each kind of task lays out its own way */
union task_vars {
    Sint8 b[12];
    Sint16 w[6];
    Sint32 l[3];
    Float f[3];
    void *p[3];
};

struct task_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 id;
    Uint8 tick;
    Sint16 flags;
    Sint16 timer;
    union task_vars vars;
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT3 scl;
    struct collision_work *collision;
    struct event_work *event;
};

/* work's timer as a Uint16 lvalue, for the kinds of task whose count there
 * is unsigned */
#define TASK_TIMER(work) (*(Uint16 *)&(work)->timer)

/* TASK_MOTION's record. drive and slip are a player's, and other kinds of
 * task use them their own ways */
struct task_motion {
    struct player *record; /* a player task's own record */
    NJS_VECTOR speed;
    NJS_VECTOR accel; /* cleared once the step has used it */
    struct angles aim; /* the angles it turns toward */
    Sint32 unk_28;
    Uint8 unk_2c[4];
    Angle phase; /* an angle each kind of task advances its own way */
    /* for a player, the stick's push ahead this step */
    Float drive;
    /* for a player, the part of its push the ground couldn't grip, which
     * spins its feet faster. the step clears it */
    Float slip;
    Float gravity; /* the speed it gains each step it falls */
};

/* how far a task turns and moves this step. its object's surfaces, and the
 * players standing on them, move with it. TASK_FORCE allocates TASK_FORCES
 * of them: player 0's, then the one every other player reads */
struct task_force {
    /* moves a player riding the task, or null for none. rider is the
     * player's task and work its work. during the call, work's pos is where
     * the player's collision ball is */
    void (*carry)(struct task *rider, struct task_work *work,
                  struct task_force *force);
    struct angles turn;
    NJS_VECTOR speed;
};

/* a node in a tree of tasks. next and prev link a root task into its level,
 * and a child among its parent's children. task_step_all runs only the
 * roots' steps. a child's step runs when its parent's step runs it, as
 * task_step_children does. task_draw_all draws every task, each child after
 * its parent */
struct task {
    struct task *next;
    struct task *prev;
    struct task *parent;
    struct task *child; /* the first of its children */
    void (*step)(struct task *task);
    void (*draw)(struct task *task);
    void (*destructor)(struct task *task);
    struct placement *placement; /* the placement that made it, or null */
    struct task_work *work;
    struct task_motion *motion; /* TASK_MOTION's record, or a task_desc's */
    void *force; /* TASK_FORCE's records, or a record of the task's own */
    void *extra; /* TASK_EXTRA's record, or a record of the task's own */
};

/* a child for task_create_children to create. a table of specs ends at one
 * with a null step. flags, ang, pos and var set the child's work. a child
 * without a work ignores them */
struct task_spec {
    void (*step)(struct task *task);
    Uint16 create; /* task_create's flags */
    Sint16 flags; /* the work's flags */
    struct angles ang; /* added to the parent's angles */
    NJS_POINT3 pos; /* turned by the parent's angles, then added to its pos */
    Sint32 var; /* the work's vars.l[0] */
};

void task_idle(struct task *task);
/* the step task_step_level is running for a root task, or null outside
 * one. while a child's step runs, it is still the root's */
void (*task_get_current_root_step(void))(struct task *task);
/* take a pool of 4096 free tasks from the heap and empty the levels.
 * heap_init discards the pool, so call task_init after each heap_init, and
 * only then: each call takes a new pool. when the heap can't give one,
 * task_init leaves the levels as they were. every task_create then fails */
void task_init(void);
/* a task from the pool, with the records flags asks for. it goes to the
 * head of level, so its step first runs on the level's next pass. a
 * TASK_UNLINKED task goes into no level, and keeps its next and prev from
 * the free list for the caller to set. a null step makes it task_idle.
 * returns null when the pool or the heap runs out, with nothing allocated.
 * many of the game's callers leave the result unchecked */
struct task *task_create(Uint16 flags, int level,
                         void (*step)(struct task *task));
/* task_create without records, which returns null only when the pool runs
 * out */
struct task *task_alloc(int level, void (*step)(struct task *task));
/* destroy task now. its destructor runs first, while its records and
 * children remain. task_destroy then destroys the children, unlinks the
 * task, frees its records and clears PLACEMENT_LIVE in its placement. the
 * task returns to the pool, and every pointer to it or its records dangles.
 *
 * task_step_level, task_draw_level, task_draw_children and
 * task_step_children each read a task's next before its step or draw runs.
 * so a step or a draw may destroy its own task. it may not destroy these:
 * - the next sibling, whose next then leads into the free list. the free
 *   tasks' steps and draws are null. the level walks and
 *   task_draw_children skip them to the free list's end, and the later
 *   siblings miss the pass. task_step_children calls the first null step it
 *   meets;
 * - an ancestor whose children task_step_children is walking, which then
 *   reaches a null step the same way.
 *
 * task_kill destroys a task on its own next step instead */
void task_destroy(struct task *task);
/* make task_destroy task's step, so it destroys itself the next time it is
 * stepped. a child is stepped only when its parent runs its children */
void task_kill(struct task *task);
void task_step_level(int level);
void task_draw_level(int level);
void task_draw_children(struct task *task);
void task_destroy_level(int level);
void task_step_all(void);
void task_draw_all(void);
void task_destroy_levels_0_to_6(void);
void task_destroy_all(void);
void *task_alloc_work(void);
void *task_alloc_motion(void);
void *task_alloc_force(void);
void *task_alloc_extra(void);
void task_free_work(struct task *task);
void task_free_motion(struct task *task);
void task_free_force(struct task *task);
void task_free_extra(struct task *task);
/* task_create's task as parent's first child, or null when task_create
 * returns null. when both have a work, the child starts at the parent's pos
 * and angles. parent must not be null, unchecked. many of the game's
 * callers leave the result unchecked */
struct task *task_create_child(Uint16 flags, void (*step)(struct task *task),
                               struct task *parent);
/* create a child for each spec, ahead of parent's children and in spec
 * order. parent must have a work if any child does, unchecked. returns the
 * first child, or null if none was created. a spec whose task_create fails
 * is skipped: a child returned doesn't mean every spec made one */
struct task *task_create_children(struct task_spec *spec, struct task *parent);
/* called directly, destroy task's children now. as task's step, run the
 * children's steps each call until none is left, then make task destroy
 * itself on its next step */
void task_destroy_children(struct task *task);
/* run the steps of task's children, first to last. none may be null */
void task_step_children(struct task *task);

/* the heap: 0x90000 bytes of blocks, which heap_alloc hands out first fit.
 * a block is a 16-byte header, then its size rounded up to 16. an empty
 * block's header past the 0x90000 bytes ends the heap */

/* make the heap one free block, its bytes filled with 0xff. every block
 * allocated before is lost */
void heap_init(void);
/* size bytes, not cleared, or null for 0 or when no free block fits. size
 * must be positive, unchecked. many of the game's callers leave the result
 * unchecked */
void *heap_alloc(int size);
/* count records of size rounded up to a word, cleared, or null as from
 * heap_alloc. count times that must fit an int, unchecked. many of the
 * game's callers leave the result unchecked */
void *heap_calloc(int count, int size);
/* takes null */
void heap_free(void *mem);
/* the free blocks' bytes, headers included, in *total, and the largest's
 * in *largest. either pointer can be null */
void heap_stats(int *total, int *largest);

#endif
