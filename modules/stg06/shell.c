/* the cannons' shells. each flies an arc toward where it's aimed, trailing
 * the points it last passed. it lands there, on its floor, or on whatever it
 * touches, with a blast or a burst */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "collision/ground.h"
#include "collision/polygon.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "sound/sound.h"
#include "stg06/blast.h"
#include "stg06/models.h"
#include "stg06/shell.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

extern NJS_OBJECT shell_object;
extern NJS_OBJECT shell_glow_object;
extern NJS_OBJECT shell_flame_object;
extern NJS_OBJECT shell_trail_object;
extern NJS_OBJECT shell_flash_object;
extern NJS_OBJECT spot_object;

/* a trail keeps up to TRAIL_POINTS of the points its shell passed. each
 * piece of it, between two points, is the trail model bent between them.
 * the model has TRAIL_MODEL_POINTS points */
#define TRAIL_POINTS 15
#define TRAIL_MODEL_POINTS 16
/* the z at which the trail model bends. in each piece, the model's points
 * above this z lie about the piece's older point. the rest lie about its
 * newer one */
#define TRAIL_BEND (-3.0F)
/* a shell's floor lies FLOOR_BELOW below it. a SHELL_SHADOW shell takes the
 * floor found below it instead, if there is one */
#define FLOOR_BELOW 10000.0F
/* the sound of a shell landing, with a blast or a burst, and its volume */
#define SE_SHELL_LAND 170
#define SHELL_VOLUME 96
/* the steps over which a landed shell fades and its flash grows. the task
 * ends after them */
#define FLASH_STEPS 8
/* a shell's attack level, and its sphere's radius at a scale of 1 */
#define SHELL_ATTACK 2
#define SHELL_RADIUS 6.0F
/* the steps given a shell fired with no speed in x and z. such a shell can't
 * cover its distance to its target */
#define NO_PACE_STEPS 0x7f00
/* a shell's glow grows in over GLOW_STEPS, and turns once every
 * GLOW_TURN_STEPS. its flame stretches to full length over FLAME_STEPS, then
 * on at half that rate. it turns once every FLAME_TURN_STEPS */
#define GLOW_STEPS 20.0F
#define GLOW_TURN_STEPS 24.0F
#define FLAME_STEPS 50.0F
#define FLAME_TURN_STEPS 30.0F
/* p is the share of its steps a SHELL_SPOT shell has flown. its spot is then (1
 * + SPOT_GROW * p^3) times the spec's scale, in x and in z */
#define SPOT_GROW 2.6F
/* n steps after a shell lands, its flash scales by (FLASH_FROM + FLASH_GROW *
 * n)^2 */
#define FLASH_FROM 0.9F
#define FLASH_GROW 0.06F

/* a shell's record on the heap, which its destructor frees. it holds the
 * shell's trail, spec and facing */
struct shell_record {
    /* a ring of the trail's points: count of them, oldest at first */
    NJS_POINT3 points[TRAIL_POINTS];
    int count;
    int first;
    struct shell_spec *spec;
    Uint8 unk_c0[12];
    Angle pitch;
    Angle yaw;
};

/* task_work as a shell lays it out */
struct shell_work {
    Sint8 mode;
    Sint8 unk_01;
    Uint8 unk_02;
    Uint8 unk_03;
    Sint16 flags;
    Uint16 timer;
    /* the target sky_deck_shell_fire_at hands to the spec's arrive */
    struct sky_deck_target *entry;
    /* the steps the shell takes to reach target, and once it lands, the steps
     * it flew */
    int steps;
    struct shell_record *record;
    NJS_POINT3 target;
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    struct collision_work *collision;
    struct event_work *event;
};

enum shell_mode { SHELL_FLY = 1, SHELL_LANDED };

/* a trail keeps its points more than shell_trail_spacing times the spec's
 * scale apart. its newest piece draws at shell_trail_alpha. its width is
 * shell_trail_width times the scale */
static Float shell_trail_spacing = 5.0F;
static Float shell_trail_alpha = -0.78F;
static Float shell_trail_width = 1.3F;
/* a spec no code uses */
static struct shell_spec unused_spec = {0, 0, -0.1F, 0.5F, NULL};
static struct collision_shape sky_deck_shell_shape = {
    SHAPE_SPHERE | SHAPE_HURTABLE | SHAPE_UNTURNED,
    SHELL_ATTACK | GUARD(3) | SHAPE_HURTS_PLAYERS,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    SHELL_RADIUS,
    0.0F,
    0.0F,
    {0, 0, 0},
    0,
    NULL};

static void sky_deck_shell_step(struct task *task);
static void sky_deck_shell_draw(struct task *task);
static void sky_deck_shell_destroy(struct task *task);

/* add pos as the trail's newest point, once pos lies far enough from the
 * current newest. far enough is more than shell_trail_spacing times scale. an
 * empty trail takes pos at once. a full trail lets its oldest point go for
 * it */
static void shell_trail_add(struct task *task, NJS_POINT3 *pos, Float scale)
{
    struct shell_work *work = (struct shell_work *)task->work;
    int newest = -1;
    int count = work->record->count;
    int first = work->record->first;
    int next;

    if (count >= 1) {
        newest = count + first - 1;
        if (newest >= TRAIL_POINTS) {
            newest -= TRAIL_POINTS;
        }
    }
    next = count + first;
    if (next >= TRAIL_POINTS) {
        next -= TRAIL_POINTS;
    }
    /* @bug logic: pos is stored into the slot after the newest point
     * (MOV.L R2,@R4 at 0x0c9103b2) ahead of the spacing test. in a full trail
     * that slot holds the oldest point. a pos too near the newest overwrites it
     * there, while first stays. the trail's oldest piece then reaches out to
     * pos. a landed shell, whose pos stays put, does this each step */
    work->record->points[next] = *pos;
    if (newest < 0 || njDistanceP2P(&work->record->points[newest], pos) >
                          shell_trail_spacing * scale) {
        if (count >= TRAIL_POINTS) {
            first++;
            if (first >= TRAIL_POINTS) {
                first -= TRAIL_POINTS;
            }
            work->record->first = first;
        } else {
            work->record->count = count + 1;
        }
    }
}

/* place the trail model's count points into placed. the points above bend in
 * z go about head, turned to face along ahead. the rest go about tail,
 * turned to face along back. the scale flattens z to 0 */
static void shell_trail_bend(NJS_POINT3 *head, NJS_VECTOR *ahead,
                             NJS_POINT3 *tail, NJS_VECTOR *back, Float bend,
                             Float scale, NJS_POINT3 *points,
                             NJS_POINT3 *placed, int count)
{
    struct angles head_ang;
    struct angles tail_ang;
    NJS_POINT3 *from;
    NJS_POINT3 *to;
    int left;
    Angle ang;
    Float x;
    Float z;

    x = ahead->x;
    z = ahead->z;
    head_ang.x =
        (Angle)(atan2f(ahead->y, njSqrt((x * x) + (z * z))) * ANGLE_PER_RADIAN);
    head_ang.y = (Angle)(atan2f(ahead->x, ahead->z) * ANGLE_PER_RADIAN);
    x = back->x;
    z = back->z;
    tail_ang.x =
        (Angle)(atan2f(back->y, njSqrt((x * x) + (z * z))) * ANGLE_PER_RADIAN);
    tail_ang.y = (Angle)(atan2f(back->x, back->z) * ANGLE_PER_RADIAN);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, head);
    TURN_VIA(njRotateY, head_ang.y, ang);
    TURN_VIA(njRotateX, head_ang.x, ang);
    njScale(NULL, scale, scale, 0.0F);
    from = points;
    to = placed;
    left = count;
    while (left--) {
        if (from->z > bend) {
            njCalcPoint(NULL, from++, to++);
        } else {
            from++;
            to++;
        }
    }
    njPopMatrix(1);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, tail);
    TURN_VIA(njRotateY, tail_ang.y, ang);
    TURN_VIA(njRotateX, tail_ang.x, ang);
    njScale(NULL, scale, scale, 0.0F);
    from = points;
    to = placed;
    left = count;
    while (left--) {
        if (from->z > bend) {
            from++;
            to++;
        } else {
            njCalcPoint(NULL, from++, to++);
        }
    }
    njPopMatrix(1);
}

/* draw the trail from its newest point back, a piece between each two
 * points. the newest piece draws at alpha. each older one draws less by
 * 1 / TRAIL_POINTS of the way from alpha to -1. it draws on a frame's first
 * step only */
static void shell_trail_draw(struct task *task, Float scale, Float alpha)
{
    struct shell_work *work = (struct shell_work *)task->work;
    NJS_POINT3 placed[TRAIL_MODEL_POINTS];
    NJS_VECTOR ahead;
    NJS_VECTOR back;
    NJS_POINT3 *newer;
    NJS_POINT3 *point;
    NJS_POINT3 *points;
    int count;
    int first;
    int index;
    int i;
    Float fade;

    if (frame_step) {
        return;
    }
    scale *= shell_trail_width;
    count = work->record->count;
    first = work->record->first;
    njSetTexture(&obj_skydeck_texlist);
    newer = NULL;
    fade = (-1.0F - alpha) * (1.0F / TRAIL_POINTS);
    for (i = 0; i < count; i++) {
        index = count + first - i - 1;
        if (index >= TRAIL_POINTS) {
            index -= TRAIL_POINTS;
        }
        point = &work->record->points[index];
        if (newer == NULL) {
            ahead = work->speed;
        } else {
            /* @hack spelling: ahead taken a Float at a time through cursors,
             * for the image's @R5+ and @R7+ loads (0x0c910724, 0x0c910728); by
             * member, 154 of 192 words and 21 of 36 other words differ */
            Float *to = &ahead.x;
            Float *from = &newer->x;
            Float *by = &point->x;
            *to++ = *from++ - *by++;
            *to++ = *from++ - *by++;
            *to = *from - *by;
        }
        if (i > 0) {
            points = shell_trail_object.model->points;
            shell_trail_object.model->points = placed;
            shell_trail_object.model->r = shell_trail_spacing + 7.0F;
            shell_trail_bend(point, &ahead, newer, &back, TRAIL_BEND, scale,
                             points, placed, TRAIL_MODEL_POINTS);
            shell_trail_object.model->center = *point;
            draw_set_material(alpha, 0.0F, 0.0F, 0.0F);
            draw_model(shell_trail_object.model);
            draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            alpha += fade;
            shell_trail_object.model->points = points;
        }
        back = ahead;
        newer = point;
    }
}

/* set a material to blend additively, by its alpha */
#define ADDITIVE(material)                                                     \
    ((material)->attrflags =                                                   \
         ((material)->attrflags & ~(NJD_SA_MASK | NJD_DA_MASK)) | NJD_SA_SRC | \
         NJD_DA_ONE | NJD_FLAG_USE_ALPHA)

/* start the shell flying. unless the set editor holds it, give it its
 * sphere. the sphere attacks at SHELL_ATTACK, or with SHELL_HARMLESS not at
 * all. the shell's five models then blend additively */
static void sky_deck_shell_init(struct task *task)
{
    struct shell_work *work = (struct shell_work *)task->work;

    if (!placement_editing(task)) {
        collision_init(task, &sky_deck_shell_shape, COLLISION_OBJECT, 1, 1);
        if (work->record->spec->flags & SHELL_HARMLESS) {
            work->collision->shapes->levels =
                work->collision->shapes->levels & ~SHAPE_ATTACK;
        } else {
            work->collision->shapes->levels =
                (work->collision->shapes->levels & ~SHAPE_ATTACK) |
                SHELL_ATTACK;
        }
    }
    task->draw = sky_deck_shell_draw;
    task->step = sky_deck_shell_step;
    task->destructor = sky_deck_shell_destroy;
    work->timer = 0;
    work->mode = SHELL_FLY;
    ADDITIVE(shell_trail_object.model->mats);
    ADDITIVE(shell_flame_object.model->mats);
    ADDITIVE(shell_flash_object.model->mats);
    ADDITIVE(shell_object.model->mats);
    ADDITIVE(shell_glow_object.model->mats);
}

static void sky_deck_shell_destroy(struct task *task)
{
    struct shell_work *work = (struct shell_work *)task->work;

    if (work->record != NULL) {
        heap_free(work->record);
    }
}

/* face the shell along its speed and draw it. a flying shell lands once its
 * steps are up. it lands sooner once its next move would reach its floor. a
 * shell without SHELL_HARMLESS also lands once it touches something. a shell
 * whose steps are up lands at its target and calls its spec's arrive. it then
 * makes a blast there, or with SHELL_BURST a burst. a shell that lands sooner
 * bursts where it is. each landing plays SE_SHELL_LAND. a landed shell's task
 * ends after FLASH_STEPS */
static void sky_deck_shell_step(struct task *task)
{
    struct shell_work *work = (struct shell_work *)task->work;
    /* @hack redundant: 12 bytes of frame that no code reads, of a type unknown,
     * declared as a point. the frame takes 244 bytes (the MOV.W of -244 at
     * 0x0c910952, from the pool word at 0x0c9109e0); without them, that pool
     * word changes, and 0 of 350 words and 2 of 114 other words differ */
    NJS_POINT3 unused;
    struct sides_query query;
    NJS_POINT3 shadow;
    struct angles slope;
    struct surface *below;
    /* @hack spelling: the speed read through a byte cursor, for the image's
     * @Rn+ loads; through the members, 73 of 350 words differ */
    char *speed = (char *)&work->speed;
    Float x;
    Float y;
    Float z;
    Float floor;

    x = *(Float *)speed;
    speed += sizeof(Float);
    y = *(Float *)speed;
    speed += sizeof(Float);
    z = *(Float *)speed;
    work->record->yaw = (Angle)(atan2f(x, z) * ANGLE_PER_RADIAN);
    work->record->pitch =
        -(Angle)(atan2f(y, njSqrt((x * x) + (z * z))) * ANGLE_PER_RADIAN);
    sky_deck_shell_draw(task);
    if (placement_editing(task)) {
        return;
    }
    switch (work->mode) {
    case SHELL_FLY:
        floor = work->pos.y - FLOOR_BELOW;
        query.pos = work->pos;
        if ((work->record->spec->flags & SHELL_SHADOW) &&
            ground_find_sides(&query) && query.sides[SIDE_BELOW].found) {
            below = &query.sides[SIDE_BELOW];
            shadow.x = work->pos.x;
            shadow.y = floor = below->level;
            shadow.z = work->pos.z;
            slope.x = below->ang_x;
            slope.y = 0;
            slope.z = below->ang_z;
            shadow_draw_at(&slope, &shadow, work->record->spec->scale);
        }
        if (work->pos.y + work->speed.y <= floor || work->steps < work->timer ||
            (!(work->record->spec->flags & SHELL_HARMLESS) &&
             (work->collision->flags & COLLISION_HIT))) {
            if (work->steps < work->timer) {
                work->pos = work->target;
                if (work->record->spec->arrive) {
                    work->record->spec->arrive(work->entry, &work->pos,
                                               &work->speed);
                }
                if (!(work->record->spec->flags & SHELL_BURST)) {
                    Angle ang_x = 0;
                    Angle ang_z = 0;
                    query.pos.y += 3.0F;
                    if (ground_find_sides(&query)) {
                        below = &query.sides[SIDE_BELOW];
                        /* @bug logic: the floor's distance from the target
                         * is tested against speed.y (FABS and FCMP/GT at
                         * 0x0c910b1e, 0x0c910b20). speed.y is below 0 while
                         * the shell falls, so a falling shell's blast is
                         * never tilted to the floor */
                        if (njAbs(below->level - work->pos.y) <=
                            work->speed.y) {
                            ang_x = below->ang_x;
                            ang_z = below->ang_z;
                        }
                    }
                    sky_deck_blast_create(&work->pos, work->record->spec->scale,
                                          ang_x, ang_z);
                } else {
                    sky_deck_burst_create(&work->pos,
                                          work->record->spec->scale);
                }
            } else {
                sky_deck_burst_create(&work->pos, work->record->spec->scale);
            }
            work->steps = work->timer;
            sound_play_at(SE_SHELL_LAND, 0, 0, SHELL_VOLUME, work->pos.x,
                          work->pos.y, work->pos.z);
            work->mode = SHELL_LANDED;
            work->timer = 0;
        } else {
            struct collision_shape *shape = work->collision->shapes;
            /* @hack spelling: speed added into pos through Float walks; as byte
             * cursors, 15 of 350 words differ */
            Float *to = &work->pos.x;
            Float *by = &work->speed.x;
            *to++ += *by++;
            *to++ += *by++;
            *to += *by;
            work->speed.y += work->record->spec->gravity;
            shape->radius = work->record->spec->scale * SHELL_RADIUS;
        }
        break;
    case SHELL_LANDED:
        if (work->timer > FLASH_STEPS) {
            task_kill(task);
        }
    }
    shell_trail_add(task, &work->pos, work->record->spec->scale);
    collision_register((struct task_work *)work);
    work->timer++;
}

/* draw a SHELL_SPOT shell's spot, then the trail and the shell, with its
 * glow and flame. once the shell has landed, it is held as it landed. it and
 * its trail fade over FLASH_STEPS as its flash grows */
static void sky_deck_shell_draw(struct task *task)
{
    struct shell_work *work = (struct shell_work *)task->work;
    Float scale = work->record->spec->scale;
    Float time = (Float)work->timer;
    Float alpha;
    Float fade;
    Float size;
    Float part;
    Angle ang;

    if (work->mode == SHELL_FLY && (work->record->spec->flags & SHELL_SPOT)) {
        part = (Float)work->timer / (Float)work->steps;
        size = (1.0F + (SPOT_GROW * (part * part * part))) * scale;
        sky_deck_spot_draw(&work->target, size, 1.0F, size);
    }
    alpha = shell_trail_alpha;
    if (work->mode == SHELL_LANDED) {
        fade = -((Float)work->timer * (1.0F / FLASH_STEPS));
        time = (Float)work->steps;
        shell_trail_draw(task, scale, alpha + fade);
        draw_set_material(fade, 0.0F, 0.0F, 0.0F);
    } else {
        shell_trail_draw(task, scale, alpha);
    }
    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njScale(NULL, scale, scale, scale);
    TURN_VIA(njRotateY, work->record->yaw, ang);
    TURN_VIA(njRotateX, work->record->pitch, ang);
    njPushMatrix(NULL);
    size = 1.0F;
    if (time < GLOW_STEPS) {
        size = time * (1.0F / GLOW_STEPS);
    }
    njScale(NULL, size, size, size);
    TURN_VIA(njRotateZ, (Angle)(time * (65536.0F / GLOW_TURN_STEPS)), ang);
    draw_tree(&shell_glow_object);
    njPopMatrix(1);
    draw_tree(&shell_object);
    njPushMatrix(NULL);
    part = time * (1.0F / FLAME_STEPS);
    if (time < FLAME_STEPS) {
        size = part;
    } else {
        size = 1.0F + ((part - 1.0F) * 0.5F);
    }
    njScale(NULL, 1.0F, 1.0F, size);
    TURN_VIA(njRotateZ, (Angle)(time * (65536.0F / FLAME_TURN_STEPS)), ang);
    draw_tree(&shell_flame_object);
    njPopMatrix(1);
    njPopMatrix(1);
    if (work->mode == SHELL_LANDED) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        njScale(NULL, scale, scale, scale);
        TURN_VIA(njRotateY, work->record->yaw, ang);
        TURN_VIA(njRotateX, work->record->pitch, ang);
        scale = FLASH_FROM + (FLASH_GROW * (Float)work->timer);
        scale *= scale;
        njScale(NULL, scale, scale, scale);
        draw_tree(&shell_flash_object);
        njPopMatrix(1);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}

struct task *sky_deck_shell_fire(NJS_POINT3 *pos, NJS_POINT3 *target,
                                 NJS_VECTOR *speed, struct shell_spec *spec)
{
    struct task *task = task_create(TASK_WORK, 3, sky_deck_shell_init);
    struct shell_work *work;
    NJS_VECTOR across;
    Float distance;
    Float pace;

    if (task && !spec->arrive) {
        work = (struct shell_work *)task->work;
        {
            /* @hack spelling: across taken a Float at a time through cursors,
             * for the image's @R5+ and @R4+ loads (0x0c911078, 0x0c91107c);
             * by member, 113 of 123 words differ */
            Float *to = &across.x;
            Float *from = &target->x;
            Float *by = &pos->x;
            *to++ = *from++ - *by++;
            *to++ = *from++ - *by++;
            *to = *from - *by;
        }
        across.y = 0.0F;
        distance = njScalor(&across);
        across = *speed;
        across.y = 0.0F;
        pace = njScalor(&across);
        if (pace > 0.0F) {
            work->steps = (int)((int)distance / pace);
        } else {
            work->steps = NO_PACE_STEPS;
        }
        work->pos = *pos;
        work->speed = *speed;
        work->target = *target;
        work->record = heap_calloc(1, sizeof(struct shell_record));
        work->record->spec = spec;
        return task;
    }
    return NULL;
}

struct task *sky_deck_shell_fire_at(NJS_POINT3 *pos, NJS_POINT3 *target,
                                    NJS_VECTOR *speed,
                                    struct sky_deck_target *entry,
                                    struct shell_spec *spec)
{
    struct task *task = task_create(TASK_WORK, 3, sky_deck_shell_init);
    struct shell_work *work;
    NJS_VECTOR across;
    Float distance;
    Float pace;

    if (task && spec->arrive) {
        work = (struct shell_work *)task->work;
        {
            /* @hack spelling: across taken a Float at a time through cursors,
             * for the image's @R4+ and @R5+ loads (0x0c9111a4, 0x0c9111a8);
             * by member, 114 of 125 words and 1 of 23 other words differ */
            Float *to = &across.x;
            Float *from = &target->x;
            Float *by = &pos->x;
            *to++ = *from++ - *by++;
            *to++ = *from++ - *by++;
            *to = *from - *by;
        }
        across.y = 0.0F;
        distance = njScalor(&across);
        across = *speed;
        across.y = 0.0F;
        pace = njScalor(&across);
        if (pace > 0.0F) {
            work->steps = (int)((int)distance / pace);
        } else {
            work->steps = NO_PACE_STEPS;
        }
        work->pos = *pos;
        work->speed = *speed;
        work->target = *target;
        work->record = heap_calloc(1, sizeof(struct shell_record));
        work->record->spec = spec;
        work->entry = entry;
        return task;
    }
    return NULL;
}

void sky_deck_spot_draw(NJS_POINT3 *pos, Float sx, Float sy, Float sz)
{
    /* @hack spelling: pos read a Float at a time through a cursor, for the
     * image's
     * @R4+ loads (0x0c9112a6, 0x0c9112ac); through the members, 48 of 50
     * words differ */
    Float *at = &pos->x;
    Float x = *at++;
    Float y = *at++;
    Float z = *at;

    njSetTexture(&obj_skydeck_texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, x, y + 1.0F, z);
    njScale(NULL, sx, sy, sz);
    draw_tree(&spot_object);
    njPopMatrix(1);
}
