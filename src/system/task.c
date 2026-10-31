/* tasks: the game's objects, and the heap their records come from */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "event/event.h"
#include "object/placement.h"
#include "system/task.h"
#include "system/util.h"

#define TASK_COUNT 4096
#define HEAP_SIZE 0x90000

/* a block's header, ahead of its bytes. next and prev link the free list,
 * and both are null while the block is in use. after and before are the
 * blocks beside it in memory. the end block, an empty header past
 * HEAP_SIZE, has a null after. it stays last in the free list, so every
 * other free block has a non-null next */
struct heap_block {
    struct heap_block *next;
    struct heap_block *prev;
    struct heap_block *after;
    struct heap_block *before;
};

/* the heap: HEAP_SIZE bytes of blocks, then the end block's header */
extern Uint8 heap_arena[];
#ifdef GCC
/* 1ST_READ.BIN's tables place it in B32, the bss whose objects start on a
 * 32-byte boundary, where no source holds it. the port defines it here, on
 * a 32-byte boundary too */
Uint8 heap_arena[HEAP_SIZE + sizeof(struct heap_block)]
    __attribute__((aligned(32)));
#endif

static struct heap_block *heap_free_list;
static int dat_8c7a7de4; /* heap_init sets it to 0, and no code reads it */
#ifdef GCC
/* nothing reads the pool's start again, so GCC keeps it as used, as the
 * image keeps it, for a probe of the running game to name each task by */
static struct task *task_pool __attribute__((used));
#endif
static struct task *task_pool;
static struct task *task_free_list;
static struct task *task_levels[TASK_LEVELS];

static void (*task_current_root_step)(struct task *task) = NULL;

void task_idle(struct task *task)
{
}

void (*task_get_current_root_step(void))(struct task *task)
{
    return task_current_root_step;
}

void task_init(void)
{
    struct task *task;
    struct task *next;
    struct task *prev;
    int i;

    task_pool = heap_alloc(TASK_COUNT * sizeof(*task_pool));
    task_free_list = task_pool;
    if (task_free_list == NULL) {
        return;
    }
    for (i = 0; i < TASK_LEVELS; i++) {
        task_levels[i] = NULL;
    }
    task = task_free_list;
    prev = NULL;
    next = task;
    for (i = 0; i < TASK_COUNT; i++) {
        next++;
        if (i >= TASK_COUNT - 1) {
            next = NULL;
        }
        task->next = next;
        task->prev = prev;
        task->placement = NULL;
        task->work = NULL;
        task->motion = NULL;
        task->force = NULL;
        task->extra = NULL;
        task->child = NULL;
        task->parent = NULL;
        task->step = NULL;
        task->draw = NULL;
        task->destructor = NULL;
        prev = task;
        task = next;
    }
}

struct task *task_create(Uint16 flags, int level,
                         void (*step)(struct task *task))
{
    struct task *task;
    void *record;

    task = task_alloc(level, step);
    if (task == NULL) {
        return NULL;
    }
    if (flags & TASK_WORK) {
        record = task_alloc_work();
        if (record == NULL) {
            task_destroy(task);
            return NULL;
        }
        task->work = record;
    }
    if (flags & TASK_FORCE) {
        record = task_alloc_force();
        if (record == NULL) {
            task_destroy(task);
            return NULL;
        }
        task->force = record;
    }
    if (flags & TASK_MOTION) {
        record = task_alloc_motion();
        if (record == NULL) {
            task_destroy(task);
            return NULL;
        }
        task->motion = record;
    }
    if (flags & TASK_EXTRA) {
        record = task_alloc_extra();
        if (record == NULL) {
            task_destroy(task);
            return NULL;
        }
        task->extra = record;
    }
    return task;
}

struct task *task_alloc(int level, void (*step)(struct task *task))
{
    struct task *task;
    struct task *rest;
    /* @hack redundant: register on head, which keeps task in R14 (MOV.L
     * @R3,R14 at 0x8c09e082); without it, task takes R7, the frame goes,
     * task_alloc runs 32 bytes short and 55 of 56 words differ */
    register struct task *head;
    /* @hack spelling: none, a local every null store here writes, which
     * keeps the zero in R6 (MOV #0,R6 at 0x8c09e094); with NULL written in
     * its place, the zero takes R1 and 24 of 56 words differ */
    /* @hack redundant: register on none, which keeps the zero in R6 (MOV
     * #0,R6 at 0x8c09e094); without it, the zero takes R1 and 24 of 56 words
     * differ */
    register void *none;

    task = task_free_list;
    if (task == NULL) {
        return NULL;
    }
    none = NULL;
    rest = task->next;
    if (rest != NULL) {
        rest->prev = none;
    }
    task_free_list = rest;
    if (step != NULL) {
        task->step = step;
    } else {
        task->step = task_idle;
    }
    if (level != TASK_UNLINKED) {
        if (level == TASK_LEVEL_FIRST) {
            level = TASK_LEVELS - 1;
        }
        if (task_levels[level] == NULL) {
            task_levels[level] = task;
            task->next = none;
            task->prev = none;
        } else {
            head = task_levels[level];
            task->prev = none;
            task->next = head;
            head->prev = task;
            task_levels[level] = task;
        }
    }
    task->placement = none;
    task->work = none;
    task->motion = none;
    task->force = none;
    task->extra = none;
    task->draw = (void (*)(struct task *))none;
    task->destructor = (void (*)(struct task *))none;
    task->child = none;
    task->parent = none;
    return task;
}

void task_destroy(struct task *task)
{
    struct task *next;
    struct task *prev;

#ifdef GCC
    /* the split frame forgets what task drew, since its slot goes free */
    present_forget(task);
#endif
    if (task->destructor != NULL) {
        task->destructor(task);
    }
    task->step = NULL;
    task->draw = NULL;
    task->destructor = NULL;
    next = task->next;
    prev = task->prev;
    if (task->child != NULL) {
        task_destroy_children(task);
    }
    if (next != NULL && prev != NULL) {
        next->prev = prev;
        prev->next = next;
    } else if (next == NULL && prev != NULL) {
        prev->next = NULL;
    } else if (task->parent != NULL) {
        if (next != NULL) {
            next->prev = NULL;
        }
        task->parent->child = next;
    } else {
        int level;

        if (next != NULL) {
            next->prev = NULL;
        }
        for (level = 0; level < TASK_LEVELS; level++) {
            if (task == task_levels[level]) {
                task_levels[level] = next;
                break;
            }
        }
    }
    if (task->work != NULL) {
        task_free_work(task);
    }
    if (task->force != NULL) {
        task_free_force(task);
    }
    if (task->motion != NULL) {
        task_free_motion(task);
    }
    if (task->extra != NULL) {
        task_free_extra(task);
    }
    if (task->placement != NULL) {
        task->placement->flags &= ~PLACEMENT_LIVE;
        task->placement = NULL;
    }
    next = task_free_list;
    if (task_free_list == NULL) {
        task_free_list = task;
        task->prev = NULL;
        task->next = NULL;
    } else {
        next->prev = task;
        task->next = next;
        task->prev = NULL;
        task_free_list = task;
    }
    task->child = NULL;
    task->parent = NULL;
}

void task_kill(struct task *task)
{
    task->step = task_destroy;
}

void task_step_level(int level)
{
    struct task *task;
    struct task *next;
    void (*step)(struct task *task);

    if (level == TASK_LEVEL_FIRST) {
        level = TASK_LEVELS - 1;
    }
    task = task_levels[level];
    if (task != NULL) {
        do {
            step = task->step;
            next = task->next;
            task_current_root_step = step;
            if (step != NULL) {
#ifdef GCC
                /* keys the step's draws to task, for the split frame */
                present_run(task, step);
#else
                step(task);
#endif
            }
            task_current_root_step = NULL;
            task = next;
        } while (task != NULL);
    }
}

void task_draw_level(int level)
{
    struct task *task;
    struct task *next;
    void (*draw)(struct task *task);

    if (level == TASK_LEVEL_FIRST) {
        level = TASK_LEVELS - 1;
    }
    for (task = task_levels[level]; task != NULL; task = next) {
        draw = task->draw;
        next = task->next;
#ifdef GCC
        /* present_draw takes a task with no callback too: the second
         * picture may replay the draws its step made */
        present_draw(task, draw);
#else
        if (draw != NULL) {
            draw(task);
        }
#endif
        if (task->child != NULL) {
            task_draw_children(task);
        }
    }
}

void task_draw_children(struct task *task)
{
    struct task *child;
    struct task *next;
    void (*draw)(struct task *task);

    for (child = task->child; child != NULL; child = next) {
        draw = child->draw;
        next = child->next;
#ifdef GCC
        /* present_draw takes a task with no callback too: the second
         * picture may replay the draws its step made */
        present_draw(child, draw);
#else
        if (draw != NULL) {
            draw(child);
        }
#endif
        if (child->child != NULL) {
            task_draw_children(child);
        }
    }
}

void task_destroy_level(int level)
{
    struct task *next;
    struct task *task;
    void (*step)(struct task *task);

    if (level == TASK_LEVEL_FIRST) {
        level = TASK_LEVELS - 1;
    }
    task = task_levels[level];
    if (task != NULL) {
        do {
            step = task->step;
            next = task->next;
            if (step != NULL) {
                task_destroy(task);
            }
            task = next;
        } while (task != NULL);
    }
}

void task_step_all(void)
{
    int level;

    task_step_level(TASK_LEVEL_FIRST);
    for (level = 0; level < TASK_LEVELS - 1; level++) {
        task_step_level(level);
    }
}

void task_draw_all(void)
{
    int level;

    task_draw_level(TASK_LEVEL_FIRST);
    for (level = 0; level < TASK_LEVELS - 1; level++) {
        task_draw_level(level);
    }
}

void task_destroy_levels_0_to_6(void)
{
    int level;

    for (level = 0; level < TASK_LEVELS - 1; level++) {
        task_destroy_level(level);
    }
}

void task_destroy_all(void)
{
    int level;

    for (level = 0; level < TASK_LEVELS; level++) {
        task_destroy_level(level);
    }
}

void *task_alloc_work(void)
{
    /* @hack spelling: the record through a local, which keeps heap_calloc's
     * call a BSR that returns here (BSR at 0x8c09e3d0); returned in place,
     * the call becomes a branch, the function runs 8 bytes short and 7 of 7
     * words differ */
    void *work;

    work = heap_calloc(1, sizeof(struct task_work));
    return work;
}

void *task_alloc_motion(void)
{
    /* @hack spelling: the record through a local, which keeps heap_calloc's
     * call a BSR that returns here (BSR at 0x8c09e3de); returned in place,
     * the call becomes a branch, the function runs 8 bytes short and 7 of 7
     * words differ */
    void *motion;

    motion = heap_calloc(1, sizeof(struct task_motion));
    return motion;
}

void *task_alloc_force(void)
{
    /* @hack spelling: the records through a local, which keeps heap_calloc's
     * call a BSR that returns here (BSR at 0x8c09e3ec); returned in place,
     * the call becomes a branch, the function runs 8 bytes short and 7 of 7
     * words differ */
    void *force;

    force = heap_calloc(TASK_FORCES, sizeof(struct task_force));
    return force;
}

void *task_alloc_extra(void)
{
    /* @hack spelling: the record through a local, which keeps heap_calloc's
     * call a BSR that returns here (BSR at 0x8c09e3fa); returned in place,
     * the call becomes a branch, the function runs 8 bytes short and 7 of 7
     * words differ */
    void *extra;

    extra = heap_calloc(1, TASK_EXTRA_SIZE);
    return extra;
}

void task_free_work(struct task *task)
{
    if (task->work->collision != NULL) {
        collision_free(task->work);
    }
    if (task->work->event != NULL) {
        event_free(task->work);
    }
    heap_free(task->work);
    task->work = NULL;
}

void task_free_motion(struct task *task)
{
    heap_free(task->motion);
    task->motion = NULL;
}

void task_free_force(struct task *task)
{
    heap_free(task->force);
    task->force = NULL;
}

void task_free_extra(struct task *task)
{
    heap_free(task->extra);
    task->extra = NULL;
}

void heap_init(void)
{
    struct heap_block *end;
    Uint8 *fill;
    Uint32 i;

    fill = heap_arena;
    heap_free_list = (struct heap_block *)fill;
    end = (struct heap_block *)(fill + HEAP_SIZE);
    for (i = 0; i < HEAP_SIZE; i++, fill++) {
        *fill = 0xff;
    }
    heap_free_list->next = end;
    heap_free_list->prev = NULL;
    heap_free_list->after = end;
    heap_free_list->before = NULL;
    end->next = NULL;
    end->prev = heap_free_list;
    end->after = NULL;
    end->before = heap_free_list;
    dat_8c7a7de4 = 0;
}

void *heap_alloc(int size)
{
    struct heap_block *block;
    struct heap_block *head;
    Uint32 need;
    Uint32 room;

    if (size == 0) {
        return NULL;
    }
    head = heap_free_list;
    if (head == NULL) {
        return NULL;
    }
    if (head->after == NULL) {
        return NULL;
    }
    need = (size + sizeof(struct heap_block) + 15) & ~15;
    block = head;
    while (block != NULL && block->after != NULL) {
        room = (char *)block->after - (char *)block;
        if (room == need) {
            struct heap_block *next;
            struct heap_block *prev;

            next = block->next;
            prev = block->prev;
            if (next != NULL) {
                next->prev = prev;
            }
            if (prev != NULL) {
                prev->next = next;
            } else {
                heap_free_list = next;
            }
            block->next = NULL;
            block->prev = NULL;
            block++;
            return block;
        } else if (room > need) {
            struct heap_block *rest;
            struct heap_block *next;
            struct heap_block *prev;

            next = block->next;
            prev = block->prev;
            rest = (struct heap_block *)((char *)block + need);
            rest->next = next;
            rest->prev = prev;
            rest->after = block->after;
            rest->before = block;
            block->after->before = rest;
            block->after = rest;
            if (next != NULL) {
                next->prev = rest;
            }
            if (prev != NULL) {
                prev->next = rest;
            } else {
                heap_free_list = rest;
            }
            block->next = NULL;
            block->prev = NULL;
            block++;
            return block;
        }
        block = block->next;
    }
    return NULL;
}

void *heap_calloc(int count, int size)
{
    int bytes;
    int *mem;
    int words;

    bytes = count * ((size + 3) & ~3);
    mem = heap_alloc(bytes);
    if (mem != NULL) {
        int *word;

        for (word = mem, words = (Uint32)bytes >> 2; words != 0; words--) {
            *word++ = 0;
        }
    }
    return mem;
}

/* merge the block with each free neighbor. the end block never merges: its
 * null after marks the heap's end */
void heap_free(void *mem)
{
    struct heap_block *block;
    struct heap_block *before;
    struct heap_block *after;

    if (mem == NULL) {
        return;
    }
    block = mem;
    block--;
    before = block->before;
    after = block->after;
    if (before != NULL && (before->next != NULL || before->prev != NULL) &&
        (after->next != NULL || after->prev != NULL) && after->after != NULL) {
        struct heap_block *next;
        struct heap_block *prev;

        after->before = before;
        before->after = after;
        next = after->next;
        prev = after->prev;
        if (next != NULL) {
            next->prev = prev;
        }
        if (prev != NULL) {
            prev->next = next;
        } else {
            heap_free_list = next;
        }
        after->after->before = before;
        before->after = after->after;
    } else if (before != NULL &&
               (before->next != NULL || before->prev != NULL)) {
        after->before = before;
        before->after = after;
    } else if ((after->next != NULL || after->prev != NULL) &&
               after->after != NULL) {
        struct heap_block *next;
        struct heap_block *prev;

        next = after->next;
        prev = after->prev;
        block->next = next;
        block->prev = prev;
        if (next != NULL) {
            after->next->prev = block;
        }
        if (prev != NULL) {
            after->prev->next = block;
        } else {
            heap_free_list = block;
        }
        block->after = after->after;
        after->after->before = block;
    } else {
        struct heap_block *head;

        head = heap_free_list;
        heap_free_list = block;
        if (head != NULL) {
            head->prev = block;
        }
        block->next = head;
        block->prev = NULL;
    }
}

void heap_stats(int *total, int *largest)
{
    struct heap_block *block;
    int sum;
    Uint32 best;
    Uint32 room;

    sum = 0;
    best = 0;
    for (block = heap_free_list; block != NULL; block = block->next) {
        if (block->after != NULL) {
            room = (char *)block->after - (char *)block;
            sum += room;
            if (room > best) {
                best = room;
            }
        }
    }
    if (total != NULL) {
        *total = sum;
    }
    if (largest != NULL) {
        *largest = best;
    }
}

struct task *task_create_child(Uint16 flags, void (*step)(struct task *task),
                               struct task *parent)
{
    struct task *child;
    struct task_work *to;
    struct task_work *from;

    child = task_create(flags, TASK_UNLINKED, step);
    /* @hack redundant: the else's return of NULL, which gives the failure a
     * return of its own (MOV #0,R0 at 0x8c09e796); without it, the failure
     * shares child's return and 15 of 51 words differ */
    if (child != NULL) {
        child->next = parent->child;
        if (parent->child != NULL) {
            parent->child->prev = child;
        }
        parent->child = child;
        child->prev = NULL;
        child->parent = parent;
        to = child->work;
        from = parent->work;
        if (to != NULL && from != NULL) {
            to->pos = from->pos;
            to->ang = from->ang;
        }
    } else {
        return NULL;
    }
    return child;
}

/* @hack redundant: register on spec, which keeps it in R14 (MOV R4,R14 at
 * 0x8c09e7f4); without it, spec takes R11, from R13, child R14 and last
 * R12, and 42 of 135 words differ */
struct task *task_create_children(register struct task_spec *spec,
                                  struct task *parent)
{
    /* @hack redundant: register on child, which keeps it in R13 (MOV R0,R13
     * at 0x8c09e852); without it, child takes R11, from R13 and last R12,
     * and 30 of 135 words differ */
    register struct task *child;
    /* @hack redundant: register on from, which keeps it in R12 (MOV.L
     * @(32,R10),R12 at 0x8c09e7fe); without it, from and last trade R12 and
     * R11, and 25 of 135 words differ */
    register struct task_work *from;
    struct task_work *to;
    /* @hack redundant: register on last, which keeps the null in R8, apart
     * from last's R11 and first's R9 (MOV #0,R8 at 0x8c09e7f8); without it,
     * last's R11 holds the null and first takes R8, task_create_children
     * runs 2 bytes short and 105 of 135 words differ */
    register struct task *last;
    struct task *first;
    NJS_POINT3 offset;
    /* @hack type: volatile on ang, the local TURN_VIA passes the
     * parent's angles through. it keeps child in R13, from in R12 and last
     * in R11 (0x8c09e852, 0x8c09e7fe, 0x8c09e7fc); without it, or with TURN,
     * child takes R11, from R13 and last R12, and 30 of 135 words differ */
    volatile Angle ang;

    from = parent->work;
    last = NULL;
    first = NULL;
    njPushMatrix(&_unit_matrix_);
    if (from != NULL) {
        njTranslateV(NULL, &from->pos);
        TURN_VIA(njRotateZ, from->ang.z, ang);
        TURN_VIA(njRotateX, from->ang.x, ang);
        TURN_VIA(njRotateY, from->ang.y, ang);
    }
    for (; spec->step != NULL; spec++) {
        /* @hack spelling: spec's create and step through locals, which load
         * ahead of TASK_UNLINKED's 7 and leave it the BSR's slot (MOV #7,R5
         * at 0x8c09e850); read in the call, the 7 loads first and 3 of 135
         * words differ */
        /* @hack redundant: register on create and step, which keeps the null
         * in R8 (MOV #0,R8 at 0x8c09e7f8); without it, R8 holds
         * njAddVector's address, task_create_children runs 2 bytes short and
         * 104 of 135 words differ */
        register Uint16 create;
        register void (*step)(struct task *task);

        create = spec->create;
        step = spec->step;
        /* @hack spelling: the call and the test in one comma expression, which
         * keeps spec in R14 and child in R13 (MOV R4,R14 at 0x8c09e7f4, MOV
         * R0,R13 at 0x8c09e852); as a statement and a test, the two trade,
         * and 21 of 135 words differ */
        if ((child = task_create(create, TASK_UNLINKED, step), child != NULL)) {
            child->prev = last;
            if (last != NULL) {
                last->next = child;
            } else {
                first = child;
            }
            child->next = NULL;
            child->parent = parent;
            to = child->work;
            if (to != NULL) {
                to->flags = spec->flags;
                njCalcVector(NULL, &spec->pos, &offset);
                njAddVector(&offset, &from->pos);
                njAddVector(&to->pos, &offset);
                to->ang.x = from->ang.x + spec->ang.x;
                to->ang.y = from->ang.y + spec->ang.y;
                to->ang.z = from->ang.z + spec->ang.z;
                to->vars.l[0] = spec->var;
            }
            last = child;
        }
    }
    njPopMatrix(1);
    if (first != NULL) {
        if (parent->child != NULL) {
            parent->child->prev = last;
            last->next = parent->child;
        }
        parent->child = first;
    }
    return first;
}

void task_destroy_children(struct task *task)
{
    if (task->step == task_destroy_children) {
        task_step_children(task);
        if (task->child == NULL) {
            task->step = task_destroy;
        }
        return;
    }
    while (task->child != NULL) {
        task_destroy(task->child);
    }
}

void task_step_children(struct task *task)
{
    struct task *child;
    struct task *next;

    for (child = task->child; child != NULL; child = next) {
        next = child->next;
#ifdef GCC
        /* keys the step's draws to child. the second picture's pass, which
         * draws the children itself, steps none */
        present_run(child, child->step);
#else
        child->step(child);
#endif
    }
}
