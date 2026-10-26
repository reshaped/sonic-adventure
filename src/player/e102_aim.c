/* E-102's aim: the beam he sweeps, and his lock-ons on what it reaches */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "collision/collision.h"
#include "player/e102.h"
#include "player/e102_aim.h"
#include "player/e102_bonus.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* his aim's sound. it starts with an aim, and the aim's end stops it */
#define SE_AIM 790

#define AIM_TIME (3 * 60) /* the steps he can hold an aim */
#define AIM_DRAWS 8 /* what each new lock-on sets aim_draws to */

/* the most lock-ons he holds, and the highest number one shows */
#define MOST_LOCKONS 32
#define HIGHEST_NUMBER 99

/* the last frame of the cannon's action, where it's open */
#define CANNON_OPEN 19.0F

#define BEAM_LENGTH 250.0F
/* the last steps of his aim, over which the beam turns from red to blue */
#define BLUE_STEPS 60

/* the bounds of a lock-on. within NEAR_RANGE of the muzzle, a target must
 * lie within NEAR_TURN of his aim across x and z. farther out, it must lie
 * within FAR_TURN of his aim, and within BEAM_RADIUS of the beam's line */
#define NEAR_RANGE 50.0F
#define NEAR_TURN NJM_DEG_ANG(30)
#define FAR_TURN NJM_DEG_ANG(12)
#define BEAM_RADIUS 60.0F

enum search_mode { SEARCH_START, SEARCH_LOOK };
enum beam_mode { BEAM_START, BEAM_FADE };

struct search_extra {
    Sint8 mode;
    Uint8 unk_01[3];
    struct task_work *work; /* E-102's work */
    struct e102 *e102;
    Uint8 unk_0c[4];
};

struct beam_vars {
    Uint8 age; /* steps it has lived: its index into beam_brightness */
    Uint8 unk_01[3];
    Float length;
    struct e102 *e102;
};

#define BEAM(work) ((struct beam_vars *)&(work)->vars)

static void search_step(struct task *task);
static void beam_step(struct task *task);

/* a beam's brightness by step; 0 ends it */
static Uint8 beam_brightness[6] = {
#include "player/e102_aim/beam_brightness.inc"
};

Bool e102_target_counts(struct task *task)
{
    struct task_work *work = task->work;
    struct collision_work *collision;

    if (work == NULL) {
        return FALSE;
    }
    collision = work->collision;
    if (collision == NULL) {
        return FALSE;
    }
    if (collision->shapes->flags & LOCKON_NO_NUMBER) {
        return FALSE;
    }
    return TRUE;
}

/* the destructor target_take_destructors gives a target. a target with
 * LOCKON_HIT that counts toward his bonus passes its lock-on's bonus,
 * or NULL, to e102_bonus_count. then its lock-on goes, and its own
 * destructor runs */
static void target_destroy(struct task *task)
{
    struct lockon_target *record = LOCKON_TARGET(task);

    if (record != NULL) {
        struct task *lockon = record->lockon;
        struct lockon_extra *extra = NULL;

        if (record->flags & LOCKON_HIT) {
            if (lockon != NULL) {
                extra = lockon->extra;
            }
            if (e102_target_counts(task)) {
                if (extra != NULL) {
                    e102_bonus_count(extra->bonus);
                } else {
                    e102_bonus_count(NULL);
                }
            }
        }
    }
    lockon_release(task);
    if (record != NULL && record->destructor != NULL) {
        record->destructor(task);
    }
}

/* give target_destroy to each task of list that he can lock on to, and
 * keep its own destructor in its lockon_target. a task whose first shape
 * has the highest guard, which no attack exceeds, is no target */
static void target_take_destructors(struct collision_work **list, int count)
{
    int i;

    for (i = count; i > 0; i--) {
        struct collision_work *collision = *list++;
        struct task *task = collision->owner;
        struct lockon_target *record = LOCKON_TARGET(task);

        if (record == NULL) {
            continue;
        }
        if (task->destructor == target_destroy) {
            continue;
        }
        if ((collision->shapes->levels & SHAPE_GUARD) == SHAPE_GUARD) {
            continue;
        }
        record->destructor = task->destructor;
        task->destructor = target_destroy;
    }
}

void e102_aim(struct task_work *work, struct player *record, struct e102 *e102)
{
    struct task *task;
    struct search_extra *extra;
    struct task_work *beam;
    Float x;
    Float y;
    Float z;
    Float across;
    struct task **lockon;
    int i;

    switch (e102->aim_mode) {
    case E102_AIM_INIT:
        e102->cannon_frame = 0.0F;
        e102->aim_draws = 0;
        e102->beam_length = 0.0F;
        e102->aim_mode = E102_AIM_READY;
        break;
    case E102_AIM_READY:
        if (pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) {
            if (e102_can_aim(work)) {
                e102->aim_mode = E102_AIM_START;
            }
        }
        break;
    case E102_AIM_START:
        e102->search = task_create(0, 6, search_step);
        if (e102->search != NULL) {
            extra = heap_calloc(1, sizeof(struct search_extra));
            if (extra != NULL) {
                e102->search->extra = extra;
                extra->work = work;
                extra->e102 = e102;
                e102->aim_time = AIM_TIME;
                e102->aim_mode = E102_AIM_LOCKING;
            } else {
                task_destroy(e102->search);
            }
        }
        if (e102_in_play) {
            sound_play_at(SE_AIM, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        break;
    case E102_AIM_LOCKING:
        /* a new beam each step, from the muzzle along his aim */
        task = task_create(TASK_WORK, 2, beam_step);
        if (task != NULL) {
            beam = task->work;
            beam->pos = e102->muzzle;
            x = e102->aim.x;
            y = e102->aim.y;
            z = e102->aim.z;
            across = njSqrt((x * x) + (z * z));
            beam->ang.y = -(Angle)(atan2f(z, x) * ANGLE_PER_RADIAN);
            beam->ang.z = (Angle)(atan2f(y, across) * ANGLE_PER_RADIAN);
            BEAM(beam)->e102 = e102;
        }
        if (!(pad_records[PLAYER_VARS(work)->player].on & pad_action_buttons)) {
            e102->aim_mode = E102_AIM_LET_GO;
            e102->lockon_number = 0;
            e102->unk_0c = 0;
        }
        /* @hack spelling: the decrement written out, which keeps pad_records in
         * R11 and pad_action_buttons' address in R10 (0x8c0dc4c2, 0x8c0dc4b8),
         * and in the cancel's loop i in R8 and lockon in the frame (0x8c0dc686,
         * 0x8c0dc68a). with --, each pair trades, and 85 of 319 words differ */
        e102->aim_time = e102->aim_time - 1;
        if (e102->aim_time <= 0 || !e102_can_aim(work)) {
            e102->lockon_number = 0;
            e102->unk_0c = 0;
            e102->aim_mode = E102_AIM_CANCEL;
        }
        break;
    case E102_AIM_CANCEL:
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        for (i = e102->lockon_count, lockon = e102->lockons; i > 0;
             i--, lockon++) {
            struct lockon_extra *held = (*lockon)->extra;

            if (held == NULL) {
                continue;
            }
            lockon_release_hit(held->target);
        }
        e102->aim_mode = E102_AIM_CANCELLED;
        e102->shot_phase = E102_SHOT_START;
        break;
    case E102_AIM_LET_GO:
    case E102_AIM_CANCELLED:
        task_destroy(e102->search);
        e102->search = NULL;
        e102->aim_mode = E102_AIM_READY;
        sound_stop(SE_AIM);
        if (pad_records[PLAYER_VARS(work)->player].press & pad_action_buttons) {
            if (e102_can_aim(work)) {
                e102->aim_mode = E102_AIM_START;
            }
        }
        break;
    }
    if (pad_records[PLAYER_VARS(work)->player].on & pad_action_buttons) {
        e102->cannon_frame += 1.0F;
    } else {
        e102->cannon_frame -= 1.0F;
    }
    e102->cannon_frame = CLAMP(e102->cannon_frame, 0.0F, CANNON_OPEN);
}

/* the size of the turn from a to b, whichever way it goes */
#define TURN_SIZE(a, b)                                                        \
    (angle_difference(a, b) < 0 ? -angle_difference(a, b)                      \
                                : angle_difference(a, b))

/* the square of the distance from his muzzle to the first shape of collision.
 * it's 0 for a collision with no owner or no work, past beam_reach, or outside
 * the bounds of a lock-on */
static Float target_reach(struct collision_work *collision, struct e102 *e102)
{
    NJS_VECTOR to;
    NJS_LINE line;
    NJS_POINT3 center;
    Float distance;
    Float off;
    struct task_work *work;
    Angle aim;
    Angle ang;
    Angle turn;

    if (collision->owner == NULL) {
        return 0.0F;
    }
    if (collision->owner->work == NULL) {
        return 0.0F;
    }
    work = collision->owner->work;
    collision_get_shape_center(work, collision->shapes, &center);
    to = center;
    njSubVector(&to, &e102->muzzle);
    distance = njScalor2(&to);
    if (distance > e102->beam_reach) {
        return 0.0F;
    }
    aim = (Angle)(atan2f(e102->aim.z, e102->aim.x) * ANGLE_PER_RADIAN);
    ang = (Angle)(atan2f(to.z, to.x) * ANGLE_PER_RADIAN);
    turn = TURN_SIZE(aim, ang);
    if (distance <= NEAR_RANGE * NEAR_RANGE) {
        if (turn > NEAR_TURN) {
            return 0.0F;
        }
    } else {
        if (turn > FAR_TURN) {
            return 0.0F;
        }
        line.px = e102->muzzle.x;
        line.py = e102->muzzle.y;
        line.pz = e102->muzzle.z;
        line.vx = e102->aim.x;
        line.vy = e102->aim.y;
        line.vz = e102->aim.z;
        off = _builtin_fabsf(njDistanceP2L(&center, &line, NULL));
        if (off > BEAM_RADIUS) {
            return 0.0F;
        }
    }
    return distance;
}

/* the nearest task of list that the beam reaches and he can lock on to, or
 * NULL. it passes over a task already locked on, one whose first shape has the
 * highest guard, and one with LOCKON_HIT. *distance gets the target_reach
 * of the task it gives, or 99999999 with none */
static struct task *target_find(struct e102 *e102, struct collision_work **list,
                                int count, Float *distance)
{
    Float best = 99999999.0F;
    struct task *found = NULL;
    int i;

    for (i = count; i > 0; i--) {
        struct collision_work *collision = *list++;
        Float reach = target_reach(collision, e102);
        struct lockon_target *record;
        struct task *owner;

        if (reach > 0.0F) {
            record = LOCKON_TARGET(collision->owner);
            if (record != NULL) {
                owner = collision->owner;
                if (record->lockon != NULL) {
                    continue;
                }
                if ((collision->shapes->levels & SHAPE_GUARD) == SHAPE_GUARD) {
                    continue;
                }
                if (record->flags & LOCKON_HIT) {
                    continue;
                }
                if (best > reach) {
                    found = owner;
                    best = reach;
                }
            }
        }
    }
    *distance = best;
    return found;
}

/* the step of the search, which runs while he aims. whenever aim_draws is
 * 0, it locks on to the nearest item or enemy the beam reaches, until he
 * holds MOST_LOCKONS. each step it also gives target_destroy to the items
 * and enemies he can lock on to */
static void search_step(struct task *task)
{
    struct search_extra *search = task->extra;
    struct task_work *work = search->work;
    struct e102 *e102 = search->e102;
    struct task *found;
    struct task *other;
    struct task *lockon;
    struct lockon_extra *extra;
    Float found_distance;
    Float other_distance;
    int player;

    switch (search->mode) {
    case SEARCH_START:
        search->mode = SEARCH_LOOK;
        /* fall through */
    case SEARCH_LOOK:
        if (e102->aim_draws > 0) {
            break;
        }
        if (e102->lockon_count >= MOST_LOCKONS) {
            break;
        }
        found = target_find(e102, collision_items, collision_item_count,
                            &found_distance);
        other = target_find(e102, collision_enemies, collision_enemy_count,
                            &other_distance);
        if (found == NULL) {
            found = other;
        } else if (other != NULL && found_distance > other_distance) {
            found = other;
        }
        if (found != NULL) {
            lockon = task_create(TASK_WORK, 5, lockon_step);
            if (lockon != NULL) {
                extra = heap_calloc(1, sizeof(struct lockon_extra));
                if (extra == NULL) {
                    task_destroy(lockon);
                } else {
                    player = PLAYER_VARS(work)->player;
                    if (e102_target_counts(found)) {
                        e102->lockon_number++;
                        e102->lockon_number =
                            e102->lockon_number < HIGHEST_NUMBER
                                ? e102->lockon_number
                                : HIGHEST_NUMBER;
                    }
                    e102->target = lockon;
                    e102->aim_draws = AIM_DRAWS;
                    lockon->work->pos = found->work->pos;
                    lockon->extra = extra;
                    extra->number = e102->lockon_number;
                    extra->target = found;
                    extra->player = work;
                    extra->e102 = e102;
                    extra->shot = NULL;
                    extra->step = found->step;
                    extra->upgrades = player_motion[player]->record->upgrades;
                }
            }
        }
        break;
    }
    target_take_destructors(collision_items, collision_item_count);
    target_take_destructors(collision_enemies, collision_enemy_count);
}

/* a beam, a line from the muzzle along his aim. it fades through
 * beam_brightness, an entry a step, and turns from red to blue over his
 * aim's last BLUE_STEPS. its first step sets his beam_length and
 * beam_reach */
static void beam_step(struct task *task)
{
    struct task_work *work = task->work;
    struct e102 *e102 = BEAM(work)->e102;
    Uint8 brightness;
    Uint8 red;
    Uint8 blue;
    int left;
    Float blueness;
    Float x;

    switch (work->mode) {
    case BEAM_START:
        e102->beam_length = BEAM(work)->length = BEAM_LENGTH;
        e102->beam_reach = e102->beam_length * e102->beam_length;
        BEAM(work)->age = 0;
        work->mode = BEAM_FADE;
        /* fall through */
    case BEAM_FADE:
        brightness = beam_brightness[BEAM(work)->age++];
        if (brightness == 0) {
            task_destroy(task);
            return;
        }
        break;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, work->ang.z);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    left = BLUE_STEPS - e102->aim_time;
    blueness = (Float)(left > 0 ? left : 0);
    blueness *= 1.0F / BLUE_STEPS;
    red = (1.0F - blueness) * brightness;
    blue = brightness * blueness;
    e102_beam_colors[0].argb.r = red;
    e102_beam_colors[1].argb.r = red;
    e102_beam_colors[0].argb.b = blue;
    e102_beam_colors[1].argb.b = blue;
    /* the test picks the beam's end whenever x + 7.5 falls short of it, so the
     * loop draws one segment. it runs from 0 to the beam's length, or to 7.5 if
     * the beam is shorter */
    x = 0.0F;
    do {
        e102_beam_points[0].x = x;
        x = x + 7.5F < BEAM(work)->length ? BEAM(work)->length : x + 7.5F;
        e102_beam_points[1].x = x;
        njDrawLine3D(&e102_beam, 1, NJD_TRANSPARENT);
    } while (BEAM(work)->length > x);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njPopMatrix(1);
}
