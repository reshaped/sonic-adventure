/* lock-ons: the two cubes that turn round each target E-102 has locked on
 * to, and its number */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "player/e102.h"
#include "player/e102_bonus.h"
#include "player/e102_shot.h"
#include "player/lockon.h"
#include "player/player.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* a lock-on's work flags. LOCKON_SHOT_FLIES_ON lets the shot at the lock-on
 * fly on once the lock-on ends. LOCKON_FADES makes lockon_release fade the
 * lock-on for LOCKON_FADE_STEPS before it ends */
#define LOCKON_SHOT_FLIES_ON 0x100
#define LOCKON_FADES 0x200
#define LOCKON_FADE_STEPS 60

/* a lock-on's scale, as a part of its target's radius, once it has grown
 * for GROW_STEPS */
#define LOCKON_SCALE 0.15F
#define GROW_STEPS 10
/* the radius of a target without a collision */
#define DEFAULT_RADIUS 10.0F

/* the ends of each cube's twelve edges */
#define CUBE_POINTS 24
/* the points of each cube that a fade colors. the rest keep the colors
 * lockon_draw last gave them */
#define FADE_POINTS 16

/* the angles the cubes turn by each step */
#define SPIN_Y 0x200
#define SPIN_Z 0x300

/* the last steps of E-102's aim, over which the cubes go from red to blue,
 * as they do over a fade */
#define AIM_BLUE_STEPS 60

/* the width of a digit, from one to the next */
#define DIGIT_WIDTH 8.0F

#define SE_LOCKON 794

enum lockon_mode { LOCKON_START, LOCKON_GROW };
enum number_mode { NUMBER_START, NUMBER_SHOW };

struct lockon_vars {
    Uint8 unk_00[4];
    Sint32 fade; /* steps since it began to fade */
    Float scale; /* as a part of the target's radius */
};

struct number_vars {
    Uint8 unk_00[4];
    NJS_POINT3 *pos; /* the lock-on's pos, which the number follows */
    /* @hack type: an array of one, whose offset SHC forms with ADD #16 as
     * for a subscript (0x8c0df01a). as a plain Sint8, SHC loads it through
     * R0 with MOV #16, and 120 of the 307 words of lockon_step and
     * lockon_number_draw differ */
    Sint8 number[1];
};

#define LOCKON(work) ((struct lockon_vars *)&(work)->vars)
#define NUMBER(work) ((struct number_vars *)&(work)->vars)

static void lockon_destroy(struct task *task);
static void lockon_fade_draw(struct task *task);
static void lockon_number_step(struct task *task);

/* E102TIME's digits, colon, e102_getsecbg and e102_lockon0 */
static NJS_TEXANIM lockon_texanims[13] = {
#include "player/lockon/lockon_texanims.inc"
};

NJS_SPRITE lockon_sprite = {
#include "player/lockon/lockon_sprite.inc"
};
/* unreferenced: a copy of lockon_sprite, which the image's data keeps
 * after it */
static NJS_SPRITE lockon_sprite_copy = {
#include "player/lockon/lockon_sprite_copy.inc"
};

/* draw the cubes at the lock-on's scale, turned opposite ways. the inner
 * one goes from red to blue, and the outer one from yellow, over E-102's
 * last AIM_BLUE_STEPS of aim */
static void lockon_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_extra *extra = task->extra;
    struct e102 *e102;
    Float scale;
    Float blueness;
    int left;
    Uint8 red;
    Uint8 blue;
    NJS_COLOR inner;
    NJS_COLOR outer;
    NJS_COLOR *in;
    NJS_COLOR *out;
    int i;

    if (frame_step != 0) {
        return;
    }
    njPushMatrix(NULL);
    scale = LOCKON(work)->scale * extra->radius;
    e102 = extra->e102;
    njTranslateV(NULL, &work->pos);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    /* @hack spelling: left set inside the test, which puts the clamped count
     * in R2 (MOV R4,R2 at 0x8c0de976). set in a statement of its own, R3
     * takes it, and 3 of 188 words differ */
    blueness =
        (Float)((left = AIM_BLUE_STEPS - e102->aim_time) > 0 ? left : 0) *
        (1.0F / AIM_BLUE_STEPS);
    red = (1.0F - blueness) * 255.0F;
    blue = blueness * 255.0F;
    in = e102->inner.lines.col;
    out = e102->outer.lines.col;
    inner.color = 0xff000000 | (Uint32)red << 16 | blue;
    outer.color = 0xff000000 | (Uint32)red << 16 | (Uint32)red << 8 | blue;
    for (i = 0; i < CUBE_POINTS; i++) {
        *in++ = inner;
        *out++ = outer;
    }
    njPushMatrix(NULL);
    TURN(njRotateZ, extra->ang.z);
    TURN(njRotateY, extra->ang.y);
    njScale(NULL, scale, scale, scale);
    e102_draw_strip(&e102->inner, NJD_TRANSPARENT);
    njPopMatrix(1);
    njPushMatrix(NULL);
    TURN(njRotateZ, -extra->ang.y);
    TURN(njRotateY, -extra->ang.z);
    njScale(NULL, scale, scale, scale);
    e102_draw_strip(&e102->outer, NJD_TRANSPARENT);
    njPopMatrix(1);
    njPopMatrix(1);
}

/* on its first step, the lock-on becomes its target's and takes the radius
 * of the target's first shape. a child task shows its number unless the
 * shape has LOCKON_NO_NUMBER. each step, it grows toward LOCKON_SCALE,
 * follows the shape and lists itself in E-102's lock-ons. it ends once its
 * target is gone */
void lockon_step(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_extra *extra = task->extra;
    struct task_work *target_work;
    struct task *target;
    struct lockon_target *record;
    struct task *child;

    switch (work->mode) {
    case LOCKON_START:
        task->destructor = lockon_destroy;
        if (extra->target == NULL) {
            task_kill(task);
            return;
        }
        target_work = extra->target->work;
        if (target_work == NULL) {
            task_kill(task);
            return;
        }
        LOCKON(work)->scale = 0.0F;
        if (target_work->collision != NULL) {
            extra->radius = target_work->collision->shapes->radius;
        } else {
            extra->radius = DEFAULT_RADIUS;
        }
        target = extra->target;
        record = LOCKON_TARGET(target);
        if (record == NULL) {
            record = heap_calloc(1, sizeof(struct lockon_target));
            target->motion = (struct task_motion *)record;
        }
        record->lockon = task;
        if (target_work->collision != NULL) {
            if (!(target_work->collision->shapes->flags & LOCKON_NO_NUMBER)) {
                child = task_create_child(TASK_WORK, lockon_number_step, task);
                if (child != NULL) {
                    NUMBER(child->work)->number[0] = extra->number;
                    NUMBER(child->work)->pos = &work->pos;
                }
            }
        }
        if (e102_in_play) {
            sound_play_at(SE_LOCKON, 0, 0, 0, work->pos.x, work->pos.y,
                          work->pos.z);
        }
        task->draw = lockon_draw;
        work->mode = LOCKON_GROW;
        /* fall through */
    case LOCKON_GROW:
        if (LOCKON(work)->scale < LOCKON_SCALE) {
            LOCKON(work)->scale += LOCKON_SCALE / GROW_STEPS;
        }
        if (extra->target == NULL) {
            task_kill(task);
            return;
        }
        target_work = extra->target->work;
        if (target_work == NULL) {
            task_kill(task);
            return;
        }
        if (target_work->collision != NULL) {
            collision_get_shape_center(
                target_work, target_work->collision->shapes, &work->pos);
        }
        break;
    }
    extra->ang.y += SPIN_Y;
    extra->ang.z += SPIN_Z;
    e102_current->lockons[e102_current->lockon_count++] = task;
    lockon_draw(task);
    task_step_children(task);
}

/* E-102's cannon, if it turns to the lock-on, stops. with
 * LOCKON_SHOT_FLIES_ON the shot at it flies on, and otherwise the shot
 * forgets it. the target forgets it too */
static void lockon_destroy(struct task *task)
{
    struct lockon_extra *extra = task->extra;
    struct task *shot = extra->shot;
    struct task *target = extra->target;
    struct lockon_target *record;

    if (e102_current->target == task) {
        e102_current->target = NULL;
        e102_current->aim_draws = 0;
    }
    if (shot != NULL) {
        if (task->work->flags & LOCKON_SHOT_FLIES_ON) {
            e102_release_shot(shot, NULL);
        } else if (shot->work != NULL) {
            SHOT(shot)->lockon = NULL;
        }
    }
    if (target != NULL) {
        record = LOCKON_TARGET(target);
        if (record != NULL) {
            record->lockon = NULL;
        }
    }
}

/* draw the cubes at the scale they grew to. the inner one goes from red to
 * blue, and the outer one from yellow, over the fade's LOCKON_FADE_STEPS */
static void lockon_fade_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_extra *extra = task->extra;
    struct e102 *e102;
    Float left;
    Float scale;
    Uint32 red;
    Uint32 blue;
    Uint32 red_field;
    NJS_COLOR inner;
    NJS_COLOR outer;
    NJS_COLOR *in;
    NJS_COLOR *out;
    int i;

    left = (Float)(LOCKON_FADE_STEPS - LOCKON(work)->fade) *
           (1.0F / LOCKON_FADE_STEPS);
    njPushMatrix(NULL);
    scale = LOCKON(work)->scale * extra->radius;
    e102 = e102_current;
    njTranslateV(NULL, &work->pos);
    njScale(NULL, scale, scale, scale);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    /* @hack spelling: red and blue set inside inner's color, and red's shift
     * kept in red_field for outer's, which SHC keeps at @R15 (0x8c0dee08,
     * 0x8c0dee1c). set in statements of their own and shifted again, 32 of
     * 167 words differ */
    /* @hack redundant: red and blue masked with 0xff though they're bytes,
     * which adds an AND after each EXTU.B (0x8c0dee00, 0x8c0dee10).
     * unmasked, 120 of 167 words differ */
    inner.color = 0xff000000 |
                  (red_field = (red = (Uint8)(left * 255.0F) & 0xff) << 16) |
                  (blue = (Uint8)((1.0F - left) * 255.0F) & 0xff);
    outer.color = 0xff000000 | red_field | red << 8 | blue;
    in = e102->inner.lines.col;
    out = e102->outer.lines.col;
    for (i = 0; i < FADE_POINTS; i++) {
        *in++ = inner;
        *out++ = outer;
    }
    njPushMatrix(NULL);
    TURN(njRotateZ, extra->ang.z);
    TURN(njRotateY, extra->ang.y);
    e102_draw_strip(&e102->inner, NJD_TRANSPARENT);
    njPopMatrix(1);
    njPushMatrix(NULL);
    TURN(njRotateZ, -extra->ang.y);
    TURN(njRotateY, -extra->ang.z);
    e102_draw_strip(&e102->outer, NJD_TRANSPARENT);
    njPopMatrix(1);
    njPopMatrix(1);
}

/* the step of a lock-on that lockon_release lets fade. it turns in place
 * for LOCKON_FADE_STEPS, then ends */
static void lockon_fade_step(struct task *task)
{
    struct task_work *work = task->work;
    struct lockon_extra *extra = task->extra;

    if (LOCKON(work)->fade >= LOCKON_FADE_STEPS) {
        task_kill(task);
        return;
    }
    LOCKON(work)->fade++;
    extra->ang.y += SPIN_Y;
    extra->ang.z += SPIN_Z;
    task->draw = lockon_fade_draw;
    lockon_fade_draw(task);
}

/* draw e102_lockon0 and the number's one or two digits, facing the camera
 * 10 in front of the lock-on */
static void lockon_number_draw(struct task *task)
{
    struct task_work *work = task->work;
    int digit;
    /* @hack redundant: a local never read, which puts njDrawSprite3D's address
     * in R13 (MOV.L at 0x8c0df006). without it, R12 takes the address, and
     * 8 of 128 words differ */
    Sint8 number = NUMBER(work)->number[0];

    if (frame_step != 0) {
        return;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, NUMBER(work)->pos);
    TURN(njRotateY, camera_work->ang.y);
    TURN(njRotateX, camera_work->ang.x + NJM_DEG_ANG(180));
    njTranslate(NULL, 0.0F, 0.0F, -10.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njDrawSprite3D(&lockon_sprite, E102TIME_LOCKON0,
                   NJD_SPRITE_COLOR | NJD_SPRITE_ALPHA);
    njTranslate(NULL, DIGIT_WIDTH, 0.0F, 0.0F);
    /* @hack spelling: the ones digit drawn in each arm, which SHC merges into
     * one draw (0x8c0df044), and which puts njTranslate's address in R11
     * (MOV.L at 0x8c0defd6). drawn once after the if, R13 takes it, and 12
     * of 128 words differ */
    if (NUMBER(work)->number[0] >= 10) {
        digit = NUMBER(work)->number[0] / 10;
        njDrawSprite3D(&lockon_sprite, digit,
                       NJD_SPRITE_COLOR | NJD_SPRITE_ALPHA);
        njTranslate(NULL, DIGIT_WIDTH, 0.0F, 0.0F);
        digit = NUMBER(work)->number[0] % 10;
        njDrawSprite3D(&lockon_sprite, digit,
                       NJD_SPRITE_COLOR | NJD_SPRITE_ALPHA);
    } else {
        digit = NUMBER(work)->number[0] % 10;
        njDrawSprite3D(&lockon_sprite, digit,
                       NJD_SPRITE_COLOR | NJD_SPRITE_ALPHA);
    }
    njPopMatrix(1);
}

static void lockon_number_step(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step != 0) {
        return;
    }
    switch (work->mode) {
    case NUMBER_START:
        task->draw = lockon_number_draw;
        work->mode = NUMBER_SHOW;
        break;
    /* @hack redundant: a case that only breaks, which gives the switch the
     * image's CMP/EQ #1 (0x8c0df090). without it, 13 of 20 words differ */
    case NUMBER_SHOW:
        break;
    }
    lockon_number_draw(task);
}

struct task *lockon_get_shot(struct task *lockon)
{
    struct lockon_extra *extra;

    if (lockon == NULL) {
        return NULL;
    }
    extra = lockon->extra;
    return extra->shot;
}

struct task *lockon_of_shot(struct task *shot)
{
    if (shot == NULL) {
        return NULL;
    }
    return SHOT(shot)->lockon;
}

void lockon_release(struct task *target)
{
    struct lockon_target *record;
    struct task *lockon;
    struct lockon_extra *extra;
    struct task *shot;

    if (target == NULL) {
        return;
    }
    record = LOCKON_TARGET(target);
    if (record == NULL) {
        return;
    }
    lockon = record->lockon;
    if (lockon == NULL) {
        return;
    }
    if (lockon->work == NULL) {
        return;
    }
    if (lockon->work->flags & LOCKON_FADES) {
        lockon->step = lockon_fade_step;
        LOCKON(lockon->work)->fade = 0;
    } else {
        lockon->step = task_destroy;
    }
    record->lockon = NULL;
    extra = lockon->extra;
    extra->target = NULL;
    if (!(extra->upgrades & PLAYER_E102_LASER)) {
        return;
    }
    shot = extra->shot;
    if (shot == NULL) {
        return;
    }
    if (record->flags & LOCKON_HIT) {
        shot->work->flags |= SHOT_PIERCED;
    }
}

void lockon_release_hit(struct task *target)
{
    struct lockon_target *record;

    if (target == NULL) {
        return;
    }
    record = LOCKON_TARGET(target);
    if (record == NULL) {
        return;
    }
    if (record->lockon == NULL) {
        return;
    }
    if (record->lockon->work != NULL) {
        record->lockon->work->flags |= LOCKON_SHOT_FLIES_ON;
    }
    lockon_release(target);
}

void lockon_release_hit_fading(struct task *target)
{
    struct lockon_target *record;

    if (target == NULL) {
        return;
    }
    record = LOCKON_TARGET(target);
    if (record == NULL) {
        return;
    }
    if (record->lockon == NULL) {
        return;
    }
    if (record->lockon->work != NULL) {
        record->lockon->work->flags |= LOCKON_SHOT_FLIES_ON | LOCKON_FADES;
    }
    lockon_release(target);
}
