/* an event's orbit: a task that keeps up to seven others on a turning
 * circle. on event_orbit_scatter, they fly straight out from its middle */
#include <ninja.h>
#include <sg_xpt.h>

#include "shared/orbit.h"
#include "shared/sprite.h"
#include "system/task.h"
#include "system/util.h"

/* the orbit's work modes. no code sets ORBIT_STOP */
enum orbit_mode { ORBIT_START, ORBIT_TURN, ORBIT_SCATTER, ORBIT_STOP };

/* the orbit's record, from the heap, which its destructor frees. pos and
 * ang are each task's place and its angle around the circle */
struct orbit {
    struct task *tasks[EVENT_ORBIT_TASKS];
    NJS_POINT3 pos[EVENT_ORBIT_TASKS];
    /* each task's change of place a step, once they scatter */
    NJS_VECTOR speed[EVENT_ORBIT_TASKS];
    Angle ang[EVENT_ORBIT_TASKS];
    Angle turned; /* how far the circle has turned */
};

struct orbit_vars {
    Uint8 unk_00[8];
    struct orbit *orbit;
};

#define ORBIT(work) ((struct orbit_vars *)&(work)->vars)

/* the tasks' angles around the circle before it turns, a seventh of a turn
 * apart */
static const Angle event_orbit_angles[EVENT_ORBIT_TASKS] = {
    0, 0x2492, 0x4924, 0x6db6, 0x9249, 0xb6db, 0xdb6d,
};

static struct task *event_orbit_task = NULL;
/* the last orbit's radius, and its circle's turn a step. event_orbit_create
 * sets both before any code reads them */
static Float event_orbit_radius = 50.0F;
static Angle event_orbit_speed = NJM_DEG_ANG(1);

static void event_orbit_init(struct task *task);

static void event_orbit_start(struct task *tasks[], NJS_POINT3 pos)
{
    struct orbit *orbit;
    struct task_work *work;
    int i;

    event_orbit_task = task_create(TASK_WORK, 5, event_orbit_init);
    orbit = heap_alloc(sizeof(struct orbit));
    work = event_orbit_task->work;
    ORBIT(work)->orbit = orbit;
    work->mode = ORBIT_START;
    work->pos = pos;
    orbit->turned = 0;
    for (i = 0; i < EVENT_ORBIT_TASKS; ++i) {
        orbit->tasks[i] = tasks[i];
        orbit->ang[i] = event_orbit_angles[i] + orbit->turned;
        orbit->pos[i].x =
            work->pos.x + (event_orbit_radius * njCos(orbit->ang[i]));
        orbit->pos[i].y = work->pos.y;
        orbit->pos[i].z =
            work->pos.z + (event_orbit_radius * njSin(orbit->ang[i]));
    }
}

void event_orbit_create(Float x, Float y, Float z, Float radius, Angle speed,
                        struct task *task_1, struct task *task_2,
                        struct task *task_3, struct task *task_4,
                        struct task *task_5, struct task *task_6,
                        struct task *task_7)
{
    NJS_POINT3 pos;
    struct task *tasks[EVENT_ORBIT_TASKS];

    pos.x = x;
    pos.y = y;
    pos.z = z;
    event_orbit_radius = radius;
    event_orbit_speed = speed;
    tasks[0] = task_1;
    tasks[1] = task_2;
    tasks[2] = task_3;
    tasks[3] = task_4;
    tasks[4] = task_5;
    tasks[5] = task_6;
    tasks[6] = task_7;
    event_orbit_start(tasks, pos);
}

static void event_orbit_kill_sprite(int number);

void event_orbit_kill(void)
{
    int i;

    if (event_orbit_task != NULL) {
        for (i = 0; i < EVENT_ORBIT_TASKS; ++i) {
            event_orbit_kill_sprite(i + 1);
        }
        task_kill(event_orbit_task);
        event_orbit_task = NULL;
    }
}

void event_orbit_add_sprite(Sint32 number, NJS_TEXLIST *texlist, Float scale,
                            Float alpha, Float red, Float green, Float blue)
{
    struct task_work *work = event_orbit_task->work;
    struct orbit *orbit = ORBIT(work)->orbit;

    if (orbit->tasks[number - 1] != NULL) {
        /* @hack type: number cast to Sint16 in the slots of
         * event_sprite_create and event_sprite_set_color. with the casts, SHC
         * works the slot out a second time and keeps it in a short at sp+28
         * (0x0cb84996, 0x0cb849be in EV00F7); without them, 66 of 75 words
         * differ and the function ends 12 bytes short */
        event_sprite_create(orbit->tasks[number - 1], (Sint16)number - 1, 0.0F,
                            0.0F, 0.0F, texlist);
        event_sprite_set_scale(number - 1, scale, scale);
        event_sprite_set_color((Sint16)number - 1, alpha, red, green, blue);
    }
}

static void event_orbit_kill_sprite(int number)
{
    struct task_work *work = event_orbit_task->work;
    struct orbit *orbit = ORBIT(work)->orbit;

    if (orbit->tasks[number - 1] != NULL) {
        event_sprite_kill(number - 1);
    }
}

void event_orbit_scatter(Float speed)
{
    struct task_work *work = event_orbit_task->work;
    struct orbit *orbit = ORBIT(work)->orbit;
    int i;

    for (i = 0; i < EVENT_ORBIT_TASKS; ++i) {
        orbit->speed[i].x =
            (orbit->pos[i].x - work->pos.x) / event_orbit_radius * speed;
        orbit->speed[i].y = 0.0F;
        orbit->speed[i].z =
            (orbit->pos[i].z - work->pos.z) / event_orbit_radius * speed;
    }
    work->mode = ORBIT_SCATTER;
}

static void event_orbit_step(struct task *task);
static void event_orbit_destroy(struct task *task);

static void event_orbit_init(struct task *task)
{
    task->step = event_orbit_step;
    task->draw = NULL;
    task->destructor = event_orbit_destroy;
}

/* turn the circle and keep the tasks on it, or once they scatter, move
 * each by its speed */
static void event_orbit_step(struct task *task)
{
    struct task_work *work = task->work;
    struct orbit *orbit = ORBIT(work)->orbit;
    int i;

    switch (work->mode) {
    case ORBIT_START:
        work->mode = ORBIT_TURN;
        break;
    case ORBIT_TURN:
        orbit->turned += event_orbit_speed;
        for (i = 0; i < EVENT_ORBIT_TASKS; ++i) {
            orbit->ang[i] = event_orbit_angles[i] + orbit->turned;
            orbit->pos[i].x =
                work->pos.x + (event_orbit_radius * njCos(orbit->ang[i]));
            orbit->pos[i].y = work->pos.y;
            orbit->pos[i].z =
                work->pos.z + (event_orbit_radius * njSin(orbit->ang[i]));
            if (orbit->tasks[i] != NULL) {
                orbit->tasks[i]->work->pos = orbit->pos[i];
            }
        }
        break;
    case ORBIT_SCATTER:
        for (i = 0; i < EVENT_ORBIT_TASKS; ++i) {
            orbit->pos[i].x += orbit->speed[i].x;
            orbit->pos[i].y += orbit->speed[i].y;
            orbit->pos[i].z += orbit->speed[i].z;
            if (orbit->tasks[i] != NULL) {
                orbit->tasks[i]->work->pos = orbit->pos[i];
            }
        }
        break;
    case ORBIT_STOP:
        break;
    }
}

/* kill the sprites in slots 0 to 6 too, whatever tasks they are on */
static void event_orbit_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct orbit *orbit = ORBIT(work)->orbit;
    int i;

    for (i = 0; i < EVENT_ORBIT_TASKS; ++i) {
        event_sprite_kill(i);
    }
    if (orbit != NULL) {
        heap_free(orbit);
    }
}
