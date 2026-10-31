/* event records and actors */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "event/event.h"
#include "player/player.h"
#include "player/weld.h"
#include "system/task.h"
#include "system/util.h"

/* an angle part of the way from one path point to the next, the short way
 * round. the points keep angles in 256ths of a turn */
#define PATH_ANG(from, to, part)                                               \
    ((Angle)((((from) + ((Sint8)((to) - (from)) * (part))) * 256.0F) + 0.5F))

/* each actor's collision sphere, of radius 0. an actor touches nothing until
 * event_set_radius gives it a radius */
/* clang-format off */
static struct collision_shape actor_shape = {
    SHAPE_SPHERE, 0, PUSH_PLAYERS | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F}, 0.0F, 0.0F, 0.0F, {0}, 0, NULL};
/* clang-format on */

void event_alloc(struct task_work *work)
{
    struct event_work *event;

    if (work->event == NULL) {
        event = heap_calloc(1, sizeof(struct event_work));
        work->event = event;
        event->flags = EVENT_GROUND | EVENT_SLOPE;
    }
}

void event_free(struct task_work *work)
{
    if (work->event != NULL) {
        event_motions_clear(work);
        event_paths_clear(work);
        heap_free(work->event);
        work->event = NULL;
    }
}

void event_motions_clear(struct task_work *work)
{
    struct event_work *event = work->event;
    struct event_motion *entry;
    struct event_motion *next;

    for (entry = event->motions; entry != NULL; entry = next) {
        next = entry->next;
        heap_free(entry);
    }
    event->motions = NULL;
    if (event->previous != NULL) {
        heap_free(event->previous);
        event->previous = NULL;
    }
    event->motion_frame = 0.0F;
}

void event_motions_add_action(struct task_work *work, NJS_ACTION *action,
                              NJS_TEXLIST *texlist, Float speed, int flags,
                              int blend)
{
    struct event_work *event = work->event;
    struct event_motion **link = &event->motions;
    struct event_motion *entry;

    while (*link != NULL) {
        link = &(*link)->next;
    }
    entry = heap_calloc(1, sizeof(struct event_motion));
    *link = entry;
    entry->flags = flags;
    entry->blend = blend;
    entry->action = *action;
    entry->texlist = texlist;
    entry->speed = speed;
}

void event_motions_add(struct task_work *work, NJS_OBJECT *object,
                       NJS_MOTION *motion, NJS_TEXLIST *texlist, Float speed,
                       int flags, int blend)
{
    struct event_work *event = work->event;
    struct event_motion **link = &event->motions;
    struct event_motion *entry;

    while (*link != NULL) {
        link = &(*link)->next;
    }
    entry = heap_calloc(1, sizeof(struct event_motion));
    *link = entry;
    entry->flags = flags;
    entry->blend = blend;
    entry->action.object = object;
    entry->action.motion = motion;
    entry->texlist = texlist;
    entry->speed = speed;
}

void event_motions_draw(struct task_work *work)
{
    struct event_work *event = work->event;
    struct event_motion *entry;
    struct event_motion *previous;
    NJS_MOTION_LINK link;
    NJS_ACTION_LINK action;
    Float frames;
    Float frame;
    int i;

    if (event == NULL) {
        return;
    }
    entry = event->motions;
    previous = event->previous;
    if (entry == NULL) {
        return;
    }
    frames = entry->action.motion->nbFrame;
    frame = event->motion_frame;
    if (entry->texlist != NULL) {
        njSetTexture(entry->texlist);
    }
    draw_set_palette(DRAW_PLAYER_PALETTE);
    if (previous != NULL && entry->blend) {
        action.motionlink = &link;
        action.object = entry->action.object;
        link.motion[0] = previous->action.motion;
        link.motion[1] = entry->action.motion;
        if (previous->flags & EVENT_REPEAT) {
            link.frame[0] = 0.0F;
        } else {
            link.frame[0] = previous->action.motion->nbFrame - 1;
        }
        link.frame[1] = 0.0F;
        for (i = 0; i < PLAYER_COUNT; i++) {
            if (work == player_work[i]) {
                if (floorf(frame) * 2.0F < entry->blend) {
                    player_weld(i, entry->action.object, link.motion[0],
                                link.frame[0]);
                } else {
                    player_weld(i, entry->action.object, link.motion[1],
                                link.frame[1]);
                }
                break;
            }
        }
        draw_action_link(&action, (frame + 1.0F) / (entry->blend + 1));
        frame += 1.0F;
        if (floorf(frame) >= entry->blend) {
            heap_free(previous);
            event->previous = NULL;
            frame = 0.0F;
        }
    } else {
        for (i = 0; i < PLAYER_COUNT; i++) {
            if (work == player_work[i]) {
                player_weld(i, entry->action.object, entry->action.motion,
                            event->motion_frame);
                break;
            }
        }
        draw_action(&entry->action, event->motion_frame);
        frame += entry->speed;
        if (entry->flags & EVENT_REPEAT) {
            if (frame >= frames) {
                if (entry->next != NULL) {
                    goto ended;
                }
                frame -= frames;
            }
        } else if (frame >= frames - 1.0F) {
        ended:
            event->previous = entry;
            frame = 0.0F;
            event->motions = entry->next;
        }
    }
    draw_set_palette(0);
    event->motion_frame = frame;
}

void event_paths_add(struct task_work *work, struct event_path *path,
                     Float speed, int flags)
{
    struct event_work *event = work->event;
    struct event_path_entry **link = &event->paths;
    struct event_path_entry *entry;

    if (*link != NULL) {
        do {
            link = &(*link)->next;
        } while (*link != NULL);
    } else if (flags & EVENT_FROM_HERE) {
        event->origin = work->pos;
    } else {
        event->origin.x = event->origin.y = event->origin.z = 0.0F;
    }
    entry = heap_calloc(1, sizeof(struct event_path_entry));
    *link = entry;
    entry->flags = flags;
    entry->path = path;
    entry->speed = speed;
}

void event_paths_clear(struct task_work *work)
{
    struct event_path_entry *entry;
    struct event_path_entry *next;

    for (entry = work->event->paths; entry != NULL; entry = next) {
        next = entry->next;
        heap_free(entry);
    }
    work->event->paths = NULL;
    work->event->path_frame = 0.0F;
}

/* replace ang, the angles njRotateXYZ turns by, with angles that make the
 * same turn in the order an actor draws in. that order is z, x and y */
static void angles_to_zxy(struct angles *ang)
{
    NJS_MATRIX matrix;
    NJS_VECTOR y_axis = {0.0F, 1.0F, 0.0F};
    NJS_VECTOR x_axis = {1.0F, 0.0F, 0.0F};
    NJS_VECTOR turned;

    njUnitMatrix(&matrix);
    njRotateXYZ(&matrix, ang->x, ang->y, ang->z);
    njCalcVector(&matrix, &y_axis, &turned);
    ang->z = (Angle)(atan2f(-turned.x, turned.y) * ANGLE_PER_RADIAN);
    ang->x = (Angle)(atan2f(turned.z, njSqrt((turned.y * turned.y) +
                                             (turned.x * turned.x))) *
                     ANGLE_PER_RADIAN);
    njInvertMatrix(&matrix);
    TURN_MATRIX(njRotateZ, &matrix, ang->z);
    TURN_MATRIX(njRotateX, &matrix, ang->x);
    njCalcVector(&matrix, &x_axis, &turned);
    ang->y = (Angle)(atan2f(turned.z, turned.x) * ANGLE_PER_RADIAN);
}

void event_paths_step(struct task_work *work)
{
    struct event_work *event = work->event;
    struct event_path_entry *entry;
    struct event_path *path;
    struct event_path_point *point;
    struct event_path_point *next;
    Float frame;
    Float count;
    Float part;
    Float ground;
    int index;
    struct angles slope;

    if (event == NULL) {
        return;
    }
    entry = event->paths;
    if (entry == NULL) {
        return;
    }
    path = entry->path;
    count = path->count;
    frame = event->path_frame;
    index = (int)floorf(frame);
    part = njFraction(frame);
    /* from the last point, a repeating path runs back to the first */
    next = path->points;
    point = &next[index];
    if (index + 1 < path->count) {
        next = point + 1;
    }
    work->pos.x =
        event->origin.x + point->pos.x + ((next->pos.x - point->pos.x) * part);
    work->pos.y =
        event->origin.y + point->pos.y + ((next->pos.y - point->pos.y) * part);
    work->pos.z =
        event->origin.z + point->pos.z + ((next->pos.z - point->pos.z) * part);
    work->ang.x = PATH_ANG(point->ang_x, next->ang_x, part);
    work->ang.y = PATH_ANG(point->ang_y, next->ang_y, part);
    work->ang.z = PATH_ANG(point->ang_z, next->ang_z, part);
    angles_to_zxy(&work->ang);
    work->ang.y = NJM_DEG_ANG(180) - work->ang.y;
    if (work->id == PLAYER_ID) {
        player_motion[PLAYER_VARS(work)->player]->aim.y = work->ang.y;
    }
    if ((event->flags & EVENT_GROUND) || (event->flags & EVENT_SLOPE)) {
        ground = ground_below(work->pos.x, work->pos.y, work->pos.z, &slope);
        if (event->flags & EVENT_GROUND) {
            work->pos.y = ground;
        }
        if (event->flags & EVENT_SLOPE) {
            work->ang.x = slope.x;
            work->ang.z = slope.z;
        }
    }
    frame += entry->speed;
    if (entry->flags & EVENT_REPEAT) {
        if (frame >= count) {
            if (entry->next != NULL) {
                goto ended;
            }
            frame -= count;
        }
    } else if (frame >= count - 1.0F) {
    ended:
        event->paths = entry->next;
        heap_free(entry);
        frame = 0.0F;
        /* this reads entry after heap_free. heap_free writes only the heap's
         * block headers, so entry's flags are as they were */
        if (entry->flags & EVENT_FROM_HERE) {
            event->origin = work->pos;
        } else {
            event->origin.x = event->origin.y = event->origin.z = 0.0F;
        }
    }
    event->path_frame = frame;
}

void event_mouth_start(struct task_work *work, const char *text)
{
    struct event_mouth *mouth = &work->event->mouth;

    mouth->text = text;
    if (mouth->steps == 0) {
        mouth->timer = 0;
        mouth->steps = 1;
        mouth->shape = 0;
        mouth->next_shape = 0;
    }
}

/* take the next byte of the mouth's text, and return its script's number.
 * A to Z return 0 to 25, and a to z 26 to 51. '0' returns -1, and any other
 * byte its own code, negative from 0x80 on. the end of the text lets go of
 * it, and the end or no text returns 0 */
static int mouth_next_script(struct task_work *work)
{
    struct event_mouth *mouth = &work->event->mouth;
    int code;

    if (mouth->text != NULL) {
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        code = *mouth->text++;
        if (code != 0) {
            if (code >= 'A' && code <= 'Z') {
                return code - 'A';
            } else if (code >= 'a' && code <= 'z') {
                return code - 'a' + 26;
            } else if (code != '0') {
                /* @hack spelling: return code stands after return 0, so the
                 * BF at 0x8c04ec80, from the compare with '0', reaches the
                 * last RTS with code still in R0; return code here gets an
                 * RTS of its own, the function runs 4 bytes long and 9 of 40
                 * words differ */
                goto other;
            }
            return -1;
        }
        mouth->text = NULL;
    }
    return 0;
other:
    return code;
}

/* start the mouth on script number, or stop it for a number below 0. number
 * 0, or one whose script is null or empty, starts script 0 three times in
 * four and script 1 otherwise */
static void mouth_set_script(struct task_work *work, int number)
{
    struct event_mouth *mouth;
    struct event_mouth_key *script;

    if (number < 0) {
        event_mouth_stop(work);
    } else {
        mouth = &work->event->mouth;
        /* @bug memory: nothing checks number against the table's 31
         * scripts, so f to z, 31 to 51, and any other byte from 31 to 127
         * read past its end (MOV.L @(R0,R14),R4 at 0x8c04ecb2), and a word
         * there that isn't null is read as a key (MOV.B @R4,R3 at
         * 0x8c04ecb8). the 736 texts the events pass event_talk_start hold
         * only capitals, a to c and '0', so no path reaches a number past
         * 28 */
#ifdef GCC
        /* repairs the read the tag names: GCC takes a number past the table
         * as a null script */
        script =
            number < EVENT_MOUTH_SCRIPTS ? event_mouth_scripts[number] : NULL;
#else
        script = event_mouth_scripts[number];
#endif
        if (number == 0 || script == NULL || script->steps == 0) {
            if (rand() * RAND_SCALE < 0.75F) {
                script = event_mouth_scripts[0];
            } else {
                script = event_mouth_scripts[1];
            }
        }
        mouth->script = script;
    }
}

void event_mouth_step(struct task_work *work)
{
    struct event_mouth *mouth = &work->event->mouth;
    struct event_mouth_key *key;

    if (mouth->steps == 0) {
        return;
    }
    if (++mouth->timer < mouth->steps) {
        return;
    }
    mouth->shape = mouth->next_shape;
    mouth->timer = 0;
    key = mouth->script;
    if (key == NULL || key->steps == 0) {
        mouth_set_script(work, mouth_next_script(work));
        if (mouth->steps == 0) {
            return;
        }
    }
    key = mouth->script++;
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    mouth->steps = key->steps;
    mouth->next_shape = key->shape;
}

void event_mouth_stop(struct task_work *work)
{
    struct event_mouth *mouth = &work->event->mouth;

    mouth->text = NULL;
    mouth->script = NULL;
    mouth->steps = 0;
}

struct task *event_actor_create(void)
{
    struct task *task = task_create(TASK_WORK, 5, event_actor_step);

    event_alloc(task->work);
    collision_init(task, &actor_shape, COLLISION_OBJECT, 1, 1);
    return task;
}

void event_actor_step(struct task *task)
{
    struct task_work *work = task->work;
    struct event_work *event = work->event;
    NJS_POINT3 under;
    struct angles slope;
    Float x;
    Float y;
    Float z;
    int flags;

    event_mouth_step(work);
    /* @hack spelling: both tests read work->event again, not event; through
     * event, 128 of 140 words differ */
    if (work->event->paths != NULL) {
        event_paths_step(work);
    }
    if (work->event->motions == NULL) {
        return;
    }
    flags = event->flags;
    x = work->pos.x;
    y = work->pos.y;
    z = work->pos.z;
    under.y = ground_below(x, y, z, &slope);
    if (flags & EVENT_GROUND) {
        work->pos.y = under.y;
    }
    if (flags & EVENT_SLOPE) {
        work->ang.x = slope.x;
        work->ang.z = slope.z;
    }
    if (event->shadow > 0.0F) {
        under.x = x;
        under.y += SHADOW_LIFT;
        under.z = z;
        shadow_draw_at(&slope, &under, event->shadow);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, NJM_DEG_ANG(180) - work->ang.y);
    njTranslateV(NULL, &event->offset);
    event_motions_draw(work);
    njPopMatrix(1);
    if (work->collision->shapes->radius > 0.0F) {
        collision_register(work);
    }
}
