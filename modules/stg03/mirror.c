/* Twinkle Park's act 2 mirrors, and the tasks that draw reflections in them */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "player/player.h"
#include "stage/stage.h"
#include "stg03/mirror.h"
#include "system/task.h"
#include "system/util.h"

#define MIRROR_COUNT 5

/* the farthest a task can be in front of a mirror's plane and be reflected in
 * it */
#define MIRROR_RANGE 200.0F

static struct {
    int count;
    struct twinkle_mirror mirrors[MIRROR_COUNT];
} twinkle_mirrors;

struct reflect_extra {
    Sint32 mode;
};

enum reflect_mode { REFLECT_INIT, REFLECT_RUN };

Bool twinkle_mirrors_clear(void)
{
    twinkle_mirrors.count = 0;
    return TRUE;
}

Bool twinkle_mirrors_add(int count, struct twinkle_mirror *mirrors)
{
    int i;
    int at = twinkle_mirrors.count;

    if (count + at > MIRROR_COUNT) {
        return FALSE;
    }
    for (i = 0; i < count; i++, at++) {
        twinkle_mirrors.mirrors[at] = mirrors[i];
    }
    twinkle_mirrors.count = at;
    return TRUE;
}

/* TRUE when mirror index sees work, and FALSE otherwise. a mirror sees work
 * in front of its plane, within MIRROR_RANGE of it. the plane's point nearest
 * work has to lie in the mirror's box too. *plane gets the mirror's plane
 * either way */
static Bool mirror_sees(struct task_work *work, union plane_view *plane,
                        int index)
{
    NJS_POINT3 nearest;
    NJS_VECTOR to;
    NJS_POINT3 low;
    NJS_POINT3 high;

    plane->plane.px = twinkle_mirrors.mirrors[index].corner0.x;
    plane->plane.py = twinkle_mirrors.mirrors[index].corner0.y;
    plane->plane.pz = twinkle_mirrors.mirrors[index].corner0.z;
    plane->plane.vx = twinkle_mirrors.mirrors[index].normal.x;
    plane->plane.vy = twinkle_mirrors.mirrors[index].normal.y;
    plane->plane.vz = twinkle_mirrors.mirrors[index].normal.z;
    if (njDistanceP2PL(&work->pos, &plane->plane, &nearest) > MIRROR_RANGE) {
        return FALSE;
    }
    to = nearest;
    njSubVector(&to, &work->pos);
    if (njInnerProduct(&to, &plane->split.normal) >= 0.0F) {
        return FALSE;
    }
    low = twinkle_mirrors.mirrors[index].corner1;
    if (low.x > plane->plane.px) {
        high.x = low.x;
        low.x = plane->plane.px;
    } else {
        high.x = plane->plane.px;
    }
    if (low.y > plane->plane.py) {
        high.y = low.y;
        low.y = plane->plane.py;
    } else {
        high.y = plane->plane.py;
    }
    if (low.z > plane->plane.pz) {
        high.z = low.z;
        low.z = plane->plane.pz;
    } else {
        high.z = plane->plane.pz;
    }
    if (low.x > nearest.x || nearest.x > high.x) {
        return FALSE;
    }
    if (low.y > nearest.y || nearest.y > high.y) {
        return FALSE;
    }
    if (low.z > nearest.z || nearest.z > high.z) {
        return FALSE;
    }
    return TRUE;
}

/* draw task in each mirror that sees it. when a mirror's pair sees task too,
 * draw task twice more, reflected between the two. return how many mirrors
 * saw task, or 0 in the set editor */
static int reflect_draw(struct task *task)
{
    struct task_work *work;
    int shown = 0;
    int count;
    int i;
    union plane_view plane;
    union plane_view across;

    /* @hack redundant: the else after the return, which loads the mirror count
     * through R3 into R1 and stores it ahead of the BF (0x0c9079ae to
     * 0x0c9079b8); without it, R2 and R3 hold it, the store moves into the
     * BF/S's slot and 5 of 120 words differ */
    if (edit_mode) {
        return 0;
    } else {
        work = task->work;
        /* @hack spelling: count assigned inside the test, which keeps shown at
         * @R15 and count at @(4,R15) (0x0c9079a6, 0x0c9079b6); assigned ahead
         * of the test, the two trade slots and 7 of 120 words differ */
        if ((count = twinkle_mirrors.count) == 0) {
            return 0;
        }
        for (i = 0; i < count; i++) {
            if (mirror_sees(work, &plane, i) == TRUE) {
                int pair;

                njPushMatrix(NULL);
                njMirror(NULL, &plane.plane);
                task->draw(task);
                njPopMatrix(1);
                shown++;
                pair = twinkle_mirrors.mirrors[i].pair;
                if (pair != 0 && mirror_sees(work, &across, i + pair)) {
                    njPushMatrix(NULL);
                    njMirror(NULL, &across.plane);
                    njMirror(NULL, &plane.plane);
                    task->draw(task);
                    njPopMatrix(1);
                    njPushMatrix(NULL);
                    njMirror(NULL, &plane.plane);
                    njMirror(NULL, &across.plane);
                    njMirror(NULL, &plane.plane);
                    task->draw(task);
                    njPopMatrix(1);
                }
            }
        }
    }
    return shown;
}

static void reflect_parent_draw(struct task *task)
{
    reflect_draw(task->parent);
}

static void reflect_player_draw(struct task *task)
{
    reflect_draw(player_get_task(0));
}

static void reflect_destroy(struct task *task)
{
    task_kill(task);
}

/* the step of a child that draws its parent in the mirrors */
static void reflect_step(struct task *task)
{
    struct reflect_extra *extra;

    if (!edit_mode) {
        extra = task->extra;
        switch (extra->mode) {
        case REFLECT_INIT:
            extra->mode = REFLECT_RUN;
            task->draw = reflect_parent_draw;
            task->destructor = reflect_destroy;
            break;
        case REFLECT_RUN:
            if (stage_get_stage_act() == STAGE_TWINKLE_MIRRORS) {
                reflect_draw(task->parent);
            }
            break;
        }
    }
}

/* the step of a task that draws player 0 in the mirrors */
static void reflect_player_step(struct task *task)
{
    struct reflect_extra *extra;

    if (!edit_mode) {
        extra = task->extra;
        switch (extra->mode) {
        case REFLECT_INIT:
            extra->mode = REFLECT_RUN;
            task->draw = reflect_player_draw;
            task->destructor = reflect_destroy;
            break;
        case REFLECT_RUN:
            /* @hack spelling: the return outside the mirrors' act, which loads
             * player_get_task's address into R2 (0x0c907b76); as an if around
             * the call, the address goes into R1 and 2 of 36 words differ */
            if (stage_get_stage_act() != STAGE_TWINKLE_MIRRORS) {
                return;
            }
            reflect_draw(player_get_task(0));
            break;
        }
    }
}

struct task *twinkle_reflect(struct task *task)
{
    struct task *child;

    if ((child = task_create_child(TASK_EXTRA, reflect_step, task)) != NULL) {
        ((struct reflect_extra *)child->extra)->mode = REFLECT_INIT;
        return child;
    }
    return NULL;
}

struct task *twinkle_reflect_player(struct task *task)
{
    struct task *reflect;

    if ((reflect = task_create(TASK_EXTRA, 1, reflect_player_step)) != NULL) {
        ((struct reflect_extra *)reflect->extra)->mode = REFLECT_INIT;
        return reflect;
    }
    return NULL;
}
