/* talks: what player 0 starts talking to by pressing grab near it */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "player/player.h"
#include "player/talk.h"
#include "system/math.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* a talk starts only while player 0 is nearer to it in x and z than
 * TALK_FARTHEST */
#define TALK_FARTHEST 100.0F
/* a TALK_LEVEL talk starts only while player 0 is less than LEVEL_HEIGHT
 * above or below it */
#define LEVEL_HEIGHT 10.0F
/* a bearing is how far one thing faces off the way to another. a
 * TALK_FACING talk starts only while player 0's bearing to it is less than
 * PLAYER_ARC. a talk with an ang starts only while its bearing to player 0
 * is less than TALK_ARC */
#define PLAYER_ARC NJM_DEG_ANG(40)
#define TALK_ARC NJM_DEG_ANG(80)

/* a talk on talk_list, as talk_add gave it */
struct talk {
    struct talk *prev;
    struct talk *next;
    void *id; /* what talk_add and talk_remove find it by */
    NJS_POINT3 *pos; /* where it stands, or null for a fallback talk */
    Angle *ang; /* the way it faces, or null for every way */
    Bool *active; /* TRUE while the talk goes on */
    Float radius; /* its reach, added to player 0's radius */
    Sint32 flags; /* TALK_LEVEL and TALK_FACING */
};

static void talk_manager_step(struct task *task);
static void talk_manager_destroy(struct task *task);
static Bool talk_find(void *id, struct talk **found);
static void talk_free_all(void);

static struct talk *talk_list = NULL;
static struct talk *talk_current = NULL; /* the talk going on, or null */

struct task *talk_create_manager(void)
{
    struct task *task = task_create(0, 0, talk_manager_step);

    talk_list = NULL;
    talk_current = NULL;
    if (task != NULL) {
        task->destructor = talk_manager_destroy;
    }
    return task;
}

Bool talk_add(void *id, NJS_POINT3 *pos, Angle *ang, Bool *active, Float radius,
              Sint32 flags)
{
    struct talk *talk;
    struct talk *last;

    if (!talk_find(id, &talk)) {
        last = talk;
        talk = heap_alloc(sizeof(*talk));
        if (talk == NULL) {
            return FALSE;
        }
        talk->prev = last;
        if (last != NULL) {
            last->next = talk;
        } else {
            talk_list = talk;
        }
        talk->next = NULL;
    }
    talk->flags = flags;
    talk->pos = pos;
    talk->ang = ang;
    talk->active = active;
    talk->radius = radius;
    talk->id = id;
    return TRUE;
}

void talk_end(void)
{
    struct talk *talk = talk_current;

    if (talk != NULL) {
        *talk->active = FALSE;
        talk_current = NULL;
    }
    pad_enable_player(PAD_BOTH_PLAYERS);
}

void talk_remove(void *id)
{
    struct talk *talk;
    struct talk *prev;
    struct talk *next;

    if (talk_find(id, &talk)) {
        prev = talk->prev;
        next = talk->next;
        if (talk == talk_current) {
            talk_end();
        }
        if (prev != NULL) {
            prev->next = next;
        } else {
            talk_list = next;
        }
        if (next != NULL) {
            next->prev = prev;
        }
        heap_free(talk);
    }
}

/* set the talk's active TRUE and take the pads from the players. pad_scan
 * then clears their buttons, the press of grab included. player 0 stops */
static void talk_start(struct talk *talk)
{
    *talk->active = TRUE;
    talk_current = talk;
    pad_disable_player(PAD_BOTH_PLAYERS);
    pad_scan();
    player_clear_speed(player_motion[0], player_get(0));
}

/* when player 0 presses grab and no talk goes on, start a talk. it is the
 * nearest in reach whose flags and ang allow it. the first talk with no pos
 * starts when no talk with a pos qualifies */
static void talk_manager_step(struct task *task)
{
    /* @hack redundant: work, set and never read, which puts heading in R10
     * and angle_arc_tan2's address in R9 (0x8c1193b4, 0x8c1193ba); without
     * it, the two trade registers and 7 of 197 words differ */
    struct task_work *work = task->work;
    struct talk *head;
    struct talk *talk;
    struct talk *best;
    struct player *record;
    NJS_POINT3 pos;
    struct angles ang;
    NJS_VECTOR delta;
    /* @hack redundant: 4 bytes of frame that no code reads, of a type unknown,
     * declared as a Float[1]. the image's ADD #-40,R15 reserves them in its
     * 80-byte frame (0x8c119300); without them, 121 of 197 words differ */
    Float unused[1];
    /* the square of TALK_FARTHEST, then of the nearest talk's distance */
    Float nearest;
    Float player_radius;
    Angle direction;
    Angle player_bearing;
    Angle talk_bearing;
    /* @hack spelling: ang.y through heading, which SHC keeps in R10 (MOV
     * R15,R10 and ADD #20,R10 at 0x8c1193b4 and 0x8c1193be). read as ang.y,
     * angle_arc_tan2's address takes R10 in place of R9, the frame is 76
     * bytes, not 80, and 124 of 197 words differ */
    Angle *heading;
    Float reach; /* the square of the talk's reach */
    Float distance; /* the square of the talk's distance in x and z */

    /* @hack redundant: a loop that never runs, which SHC deletes. it puts
     * talk_list's address in R2 and pad_grab_buttons in R3 (0x8c119302,
     * 0x8c119312); without it, the two trade registers and 4 of 197 words
     * differ */
    while (0) {
    }
    head = talk_list;
    if (head == NULL || !PAD_BUTTONS(pad_records[0].press, pad_grab_buttons) ||
        talk_current != NULL) {
        return;
    }
    talk = head;
    best = NULL;
    nearest = TALK_FARTHEST * TALK_FARTHEST;
    record = player_get(0);
    player_radius = record != NULL ? record->consts.radius : 0.0F;
    player_get_history(0, 0, &pos, &ang);
    heading = &ang.y;
    do {
        if (talk->pos == NULL) {
            if (best == NULL) {
                best = talk;
            }
        } else {
            delta = pos;
            VECTOR_DIFF(&delta, &delta, talk->pos);
            direction = angle_arc_tan2(delta.x, delta.z);
            /* each bearing is 0 where the talk doesn't ask for it */
            player_bearing =
                (talk->flags & TALK_FACING)
                    ? (Sint16)(*heading + direction + NJM_DEG_ANG(90))
                    : 0;
            talk_bearing =
                talk->ang != NULL
                    ? (Sint16)(direction - *talk->ang + NJM_DEG_ANG(90))
                    : 0;
            if (njAbs(player_bearing) < PLAYER_ARC &&
                njAbs(talk_bearing) < TALK_ARC) {
                reach = (talk->radius + player_radius) *
                        (talk->radius + player_radius);
                distance = (delta.x * delta.x) + (delta.z * delta.z);
                if (!(talk->flags & TALK_LEVEL) ||
                    njAbs(delta.y) < LEVEL_HEIGHT) {
                    if (distance < reach && distance < nearest) {
                        nearest = distance;
                        best = talk;
                    }
                }
            }
        }
        talk = talk->next;
    } while (talk != NULL);
    if (best != NULL) {
        talk_start(best);
    }
}

static void talk_manager_destroy(struct task *task)
{
    talk_free_all();
}

/* TRUE when id has a talk, which goes to *found. FALSE when it hasn't, and
 * *found gets the last talk on the list, or null */
static Bool talk_find(void *id, struct talk **found)
{
    struct talk *talk;

    *found = NULL;
    for (talk = talk_list; talk != NULL; talk = talk->next) {
        *found = talk;
        if (talk->id == id) {
            return TRUE;
        }
    }
    return FALSE;
}

static void talk_free_all(void)
{
    struct talk *talk = talk_list;
    struct talk *gone;

    while ((gone = talk) != NULL) {
        talk = talk->next;
        heap_free(gone);
    }
    talk_list = NULL;
}
