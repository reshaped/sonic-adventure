/* the collision world: the act's land, which it draws too, the objects
 * tasks put in it, and gravity */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/binary.h"
#include "system/cache.h"
#include "system/frame.h"
#include "system/loader.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the most objects each of the world's arrays holds */
#define NEAR_ADDED 128 /* tasks' objects a collect keeps */
#define NEAR_LAND 128 /* land entries a collect keeps */
#define ADDED 256 /* objects tasks put in the world */
#define DRAWN 1024 /* land entries drawn in a frame */
#define POOL_OBJECTS 256 /* objects world_object_alloc lends */

/* the near radius the land's task starts with */
#define NEAR_RADIUS 400.0F
/* the near radius of the second point world_collect_near_players measures
 * from */
#define SECOND_RADIUS 40.0F
/* how much farther than radius world_list_near reaches for a task's object */
#define ADDED_MARGIN 30.0F

/* world_add sets WORLD_TURNED where the model's center lies off its origin
 * and the object is turned. off is a squared distance of OFF_CENTER_SQUARED,
 * 15 squared, or more. turned is any angle with a bit of TURNED_ANGLE_BITS */
#define OFF_CENTER_SQUARED 225.0F
#define TURNED_ANGLE_BITS 0xfff0

/* the land task's modes */
enum world_land_mode {
    WORLD_LAND_INIT, /* start the task, then load the act's land */
    WORLD_LAND_RUN, /* draw the land and play its moving parts */
    WORLD_LAND_CHANGE /* load the land of the act world_change_act moved to */
};

static Sint32 world_near_added_count = 0;
static Sint32 world_near_land_count = 0;
static Float world_near_radius = NEAR_RADIUS;
/* the stage and act whose land is loaded */
static Sint8 world_stage = 0;
static Sint8 world_act = 0;
/* whether the land is drawn, and whether the collects and lists take its
 * entries */
static Bool world_land_shown = TRUE;
static Bool world_land_enabled = TRUE;
/* nothing reads or writes it, so its type is unknown */
static Uint16 dat_8c1b52e0 = 0;
static char world_debug_mode_names[6][20] = {"NORMAL", "SCREEN",    "BOX",
                                             "BOTH",   "COLLISION", "ALL"};

Sint32 world_land_blocks = 0;

/* a land in a stage's module, at the address it loads at */
#define LAND(address) ((struct land_table *)(address))

/* each stage's land by act, or null. STAGE_STATION_SQUARE_2 and
 * STAGE_STATION_SQUARE_3 take station square's six lands, and the sand
 * board's two acts share one */
static struct land_table *world_lands[STAGE_COUNT][STAGE_ACTS] = {
    {LAND(0x0c900bac)},
    {LAND(0x0c981904), LAND(0x0c9def10), LAND(0x0ca1e110)},
    {LAND(0x0c90e000), LAND(0x0c90e364), LAND(0x0c90fa98)},
    {LAND(0x0c9180ac), LAND(0x0c919b64), LAND(0x0c91a77c)},
    {LAND(0x0c917f14), LAND(0x0c918b50), LAND(0x0c91aa68)},
    {LAND(0x0c919b78), LAND(0x0c91b7c0), LAND(0x0c91f220)},
    {LAND(0x0c922a5c), LAND(0x0c924ad8), LAND(0x0c9278fc)},
    {LAND(0x0c90dc60), LAND(0x0c964174), LAND(0x0c9f9994)},
    {LAND(0x0c915b24), LAND(0x0c9166f4), LAND(0x0c919094), LAND(0x0c91a930)},
    {LAND(0x0c92778c), LAND(0x0c96d994), LAND(0x0c9b0a00), LAND(0x0c9d9d24)},
    {LAND(0x0c91d2fc), LAND(0x0c983ec4), LAND(0x0c9e69c8)},
    {NULL},
    {LAND(0x0c91a5ac), LAND(0x0c953620), LAND(0x0c9b1c50)},
    {NULL},
    {NULL},
    {LAND(0x0c9001d8)},
    {LAND(0x0c9002b8)},
    {LAND(0x0c900260)},
    {LAND(0x0c900370), LAND(0x0c900398)},
    {LAND(0x0c901280)},
    {LAND(0x0c900130)},
    {LAND(0x0c900230)},
    {LAND(0x0c958840)},
    {LAND(0x0c900118)},
    {LAND(0x0c900070)},
    {LAND(0x0c900188)},
    {LAND(0x0c9163b4), LAND(0x0c9744b4), LAND(0x0c9c2db0), LAND(0x0c9dda80),
     LAND(0x0ca36408), LAND(0x0ca57d4c)},
    {LAND(0x0c9163b4), LAND(0x0c9744b4), LAND(0x0c9c2db0), LAND(0x0c9dda80),
     LAND(0x0ca36408), LAND(0x0ca57d4c)},
    {LAND(0x0c9163b4), LAND(0x0c9744b4), LAND(0x0c9c2db0), LAND(0x0c9dda80),
     LAND(0x0ca36408), LAND(0x0ca57d4c)},
    {LAND(0x0ca8294c), LAND(0x0ca838dc), LAND(0x0ca843d4), LAND(0x0ca857b4),
     LAND(0x0ca85f1c), LAND(0x0ca86de0)},
    {NULL},
    {NULL},
    {LAND(0x0c92ce4c), LAND(0x0c92dfb0), LAND(0x0c92e9d4), LAND(0x0c92f5bc),
     LAND(0x0c92ffac), LAND(0x0c930648)},
    {LAND(0x0c9182c0), LAND(0x0c99f114), LAND(0x0c900178), LAND(0x0c9001a0)},
    {LAND(0x0c900278), LAND(0x0c9002a0), LAND(0x0c9002c8)},
    {LAND(0x0c901a54), LAND(0x0c902f90), LAND(0x0c904368), LAND(0x0c90568c),
     LAND(0x0c906694), LAND(0x0c9078dc)},
    {NULL},
    {NULL},
    {LAND(0x0c902df4), LAND(0x0c902df4)},
    {LAND(0x0cb903f0)},
    {LAND(0x0cb8d9c8)},
    {LAND(0x0cb8f7dc)},
    {LAND(0x0cb8027c), LAND(0x0cb80e70)},
};

Sint16 world_count;
struct world_object world_objects[WORLD_OBJECTS];
/* the tasks' objects and the land entries the last collect kept, which the
 * lists choose from */
static struct world_object world_near_added[NEAR_ADDED];
static struct land_entry world_near_land[NEAR_LAND];
/* the land the task loaded, or null */
static struct land_table *world_land;
/* the land task's work, while the task runs */
static struct task_work *world_land_work;
/* world_reset zeroes it, and nothing reads it */
static Sint32 dat_8c76225c;
/* the gray the land's constant material takes */
static Float world_land_material;
static Sint16 world_added_count;
static struct world_object world_added[ADDED];
/* the land's entries to draw this frame */
static Sint16 world_drawn_count;
static struct land_entry *world_drawn[DRAWN];
/* nothing reads or writes it, so its type is unknown */
static Uint32 dat_8c763e6c[1024];
/* world_land_list_visible sets it to -1, and nothing reads it */
static Sint16 dat_8c764e6c;
/* nothing reads or writes it, so its type is unknown */
static Uint32 dat_8c764e70[1024];
/* the objects world_object_alloc lends, and which are lent */
static NJS_OBJECT world_object_pool[POOL_OBJECTS];
static Uint8 world_object_used[POOL_OBJECTS];

static void world_reset(struct task *task);
static void world_land_start(void);
static void world_land_destroy(struct task *task);
static void world_land_run(struct task *task);
static void world_land_draw_visible(void);
static void world_land_draw_motions(void);
static void world_land_play_motions(void);
static void world_land_list_visible(void);
static void world_land_draw(struct task *task);
static void world_object_free_all(void);

void world_land_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case WORLD_LAND_INIT:
        world_reset(task);
        world_land_work = work;
        world_object_free_all();
        task->draw = world_land_draw;
        task->destructor = world_land_destroy;
        world_set_near_radius(NEAR_RADIUS);
        /* fall through */
    case WORLD_LAND_CHANGE:
        world_land_start();
        work->mode = WORLD_LAND_RUN;
        /* fall through */
    case WORLD_LAND_RUN:
        world_land_run(task);
        break;
    default:
        work->mode = WORLD_LAND_INIT;
        break;
    }
}

/* empty the world and forget the land and its task. the land is left shown
 * and enabled. task is unused */
static void world_reset(struct task *task)
{
    world_count = 0;
    world_stage = 0;
    world_act = 0;
    world_land = NULL;
    world_land_work = NULL;
    dat_8c76225c = 0;
    world_land_material = 0.0F;
    world_land_shown = TRUE;
    world_land_enabled = TRUE;
    world_added_count = 0;
    world_drawn_count = 0;
    world_near_added_count = 0;
    world_near_land_count = 0;
}

void world_load_act(int stage, int act)
{
    struct land_table *land = world_lands[stage][act];
    struct land_entry *entry;
    struct land_motion *motion;
    char *data;
    int count;
    int i;

    if (land == NULL) {
        return;
    }
    count = land->count;
    if (land->flags & LAND_FILE) {
        if (!(land->users & LAND_FILE_USERS)) {
            /* loader_read_alloc overwrites its second parameter, result,
             * before it reads it. so the image's call sets R4 alone and
             * leaves act in R5 (0x8c054d06), and passing act makes the same
             * call */
            data = loader_read_alloc(land->file, act);
            land->data = data;
            njb_relocate(data);
            for (i = count, entry = land->entries; i > 0; i--, entry++) {
                entry->object = (NJS_OBJECT *)(data + (int)entry->object);
            }
            if (land->flags & LAND_MOTIONS) {
                /* the loop relocates as many motions as the land has entries,
                 * not motion_count */
                for (i = count, motion = land->motions; i > 0; i--, motion++) {
                    motion->object = (NJS_OBJECT *)(data + (int)motion->object);
                    motion->play.motion =
                        (NJS_MOTION *)(data + (int)motion->play.motion);
                }
            }
        }
        land->users += LAND_FILE_USER;
    }
    if (land->flags & LAND_TEXLIST) {
        if (!(land->users & LAND_TEXTURE_USERS)) {
            njLoadTexture(land->texlist);
        }
        land->users += 1;
    } else if (land->flags & LAND_PVM) {
        if (land->pvm != NULL) {
            if (!(land->users & LAND_TEXTURE_USERS)) {
                texture_load_pvm(land->pvm, land->texlist);
            }
            land->users += 1;
        }
    } else if (land->flags & LAND_MOTIONS) {
        motion = land->motions;
        count = land->motion_count;
        for (i = count; i > 0; i--, motion++) {
            if (motion->texlist != NULL) {
                njLoadTexture(motion->texlist);
            }
        }
    }
}

void world_release_act(int stage, int act)
{
    struct land_table *land = world_lands[stage][act];
    struct land_motion *motion;
    int count;
    int i;

    if (land != NULL) {
        count = land->count;
        if (land->flags & LAND_FILE) {
            if (land->users & LAND_FILE_USERS) {
                land->users -= LAND_FILE_USER;
                if (!(land->users & LAND_FILE_USERS)) {
                    loader_free(land->data);
                    land->data = NULL;
                }
            }
        }
        if (land->flags & LAND_TEXLIST) {
            if (land->users & LAND_TEXTURE_USERS) {
                land->users -= 1;
                if (!(land->users & LAND_TEXTURE_USERS)) {
                    njReleaseTexture(land->texlist);
                }
            }
        } else if (land->flags & LAND_PVM) {
            if (land->pvm != NULL) {
                if (land->users & LAND_TEXTURE_USERS) {
                    land->users -= 1;
                    if (!(land->users & LAND_TEXTURE_USERS)) {
                        njReleaseTexture(land->texlist);
                    }
                }
            }
        } else if (land->flags & LAND_MOTIONS) {
            /* the loop never moves motion on, and count is the entries'
             * count. so it releases the first motion's texlist once for each
             * entry, and no other motion's, though world_load_act loads each
             * motion's texlist once */
            motion = land->motions;
            for (i = count; i > 0; i--) {
                if (motion->texlist != NULL) {
                    njReleaseTexture(motion->texlist);
                }
            }
        }
    }
}

void world_change_act(Sint16 by)
{
    world_release_act(world_stage, world_act);
    world_land_work->mode = WORLD_LAND_CHANGE;
    world_act += by;
}

/* load the land of world_stage and world_act, which the task takes from the
 * current stage as it starts. an act without land is neither drawn nor
 * listed */
static void world_land_start(void)
{
    Uint16 stage_act;

    if (world_land_work != NULL && world_land_work->mode == WORLD_LAND_INIT) {
        stage_act = stage_get_stage_act();
        world_stage = STAGE_OF(stage_act);
        world_act = stage_act;
    }
    world_land_shown = TRUE;
    world_land_enabled = TRUE;
    world_land = world_lands[world_stage][world_act];
    if (world_land == NULL) {
        world_land_shown = FALSE;
        world_land_enabled = FALSE;
        return;
    }
    sound_end_act();
    world_load_act(world_stage, world_act);
}

static void world_land_destroy(struct task *task)
{
    world_release_act(world_stage, world_act);
    world_reset(task);
    /* @hack redundant: a return where the function ends, which makes the frame
     * with ADD #-4,R15 and stores task in it (0x8c054fba, 0x8c054fc0);
     * without it, a push does both, the function runs 2 bytes short and all
     * 12 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

/* while the land is shown, draw it on a frame's first step and play its
 * moving parts on every step. world_land_draw, the task's draw, draws the
 * shown land and plays nothing */
static void world_land_run(struct task *task)
{
    draw_set_material(0.0F, world_land_material, world_land_material,
                      world_land_material);
    draw_model_init();
    draw_attr_save();
    if (world_land_shown == TRUE) {
        if (!frame_step) {
            world_land_list_visible();
            world_land_draw_visible();
            world_land_play_motions();
            world_land_draw_motions();
        } else {
            world_land_play_motions();
        }
    }
    draw_attr_restore();
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

static void world_land_draw(struct task *task)
{
    draw_set_material(0.0F, world_land_material, world_land_material,
                      world_land_material);
    draw_model_init();
    draw_attr_save();
    if (world_land_shown == TRUE && !frame_step) {
        world_land_list_visible();
        world_land_draw_visible();
        world_land_draw_motions();
    }
    draw_attr_restore();
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* draw the entries the listing found, unculled, since the listing culled
 * them. a land with a texlist or pvm sets its texlist first. the image has
 * the loop twice, once for each case */
static void world_land_draw_visible(void)
{
    struct land_entry *entry;
    NJS_OBJECT *object;
    Sint16 i;

    draw_models_begin();
    njPushMatrix(NULL);
    if (world_land->flags & (LAND_TEXLIST | LAND_PVM)) {
        njSetTexture(world_land->texlist);
        for (i = world_drawn_count - 1; i >= 0; i--) {
            entry = world_drawn[i];
            object = entry->object;
            njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
            njRotateXYZ(NULL, object->ang[0], object->ang[1], object->ang[2]);
            draw_model_unculled(object->model);
            matrix_load_parent();
        }
    } else {
        for (i = world_drawn_count - 1; i >= 0; i--) {
            entry = world_drawn[i];
            object = entry->object;
            njTranslate(NULL, object->pos[0], object->pos[1], object->pos[2]);
            njRotateXYZ(NULL, object->ang[0], object->ang[1], object->ang[2]);
            draw_model_unculled(object->model);
            matrix_load_parent();
        }
    }
    njPopMatrix(1);
    draw_models_end();
}

/* draw the land's moving parts, each at its frame */
static void world_land_draw_motions(void)
{
    struct land_table *land;
    struct land_motion *motion;
    int flags;
    int count;
    Float frame;

    /* @hack spelling: land is stored inside the read of its flags, because
     * the image reads the flags as it stores land; stored first, 7 of 56
     * words differ */
    flags = (land = world_land)->flags;
    if (!(flags & LAND_MOTIONS)) {
        return;
    }
    motion = land->motions;
    count = land->motion_count;
    if (flags & LAND_FILE) {
        for (; count > 0; count--, motion++) {
            frame = motion->frame;
            njSetTexture(motion->texlist);
            draw_motion(motion->object, motion->play.motion, frame);
        }
    } else {
        for (; count > 0; count--, motion++) {
            frame = motion->frame;
            njSetTexture(motion->texlist);
            draw_action(motion->play.action, frame);
        }
    }
}

/* advance each moving part by its speed. a part goes back to frame 0 once
 * the new frame is within speed + 1 of frames, its motion's nbFrame. the
 * image has the loop twice, by LAND_FILE, and a return after the first */
static void world_land_play_motions(void)
{
    struct land_table *land;
    struct land_motion *motion;
    int flags;
    int count;
    Float frame;
    Float speed;

    /* @hack spelling: land is stored inside the read of its flags, as in
     * world_land_draw_motions; stored first, 7 of 56 words differ */
    flags = (land = world_land)->flags;
    if (!(flags & LAND_MOTIONS)) {
        return;
    }
    motion = land->motions;
    count = land->motion_count;
    if (flags & LAND_FILE) {
        for (; count > 0; count--, motion++) {
            frame = motion->frame;
            speed = motion->speed;
            frame += speed;
            if (frame >= motion->frames - speed - 1.0F) {
                frame = 0.0F;
            }
            motion->frame = frame;
        }
        return;
    } else {
        for (; count > 0; count--, motion++) {
            frame = motion->frame;
            speed = motion->speed;
            frame += speed;
            if (frame >= motion->frames - speed - 1.0F) {
                frame = 0.0F;
            }
            motion->frame = frame;
        }
    }
}

void world_hide_land(void)
{
    world_land_shown = FALSE;
}

void world_show_land(void)
{
    world_land_shown = TRUE;
}

/* list the land's entries to draw, up to DRAWN: those with LAND_DRAWN whose
 * blocks are drawn and whose spheres come near the camera. near is within
 * half the clip of a point half the clip ahead of the camera, or with
 * LAND_FAR within ninja's far clip of it. the clip is the land's with
 * LAND_CLIP, else ninja's far clip */
static void world_land_list_visible(void)
{
    struct land_entry *entry;
    NJS_POINT3 offset;
    Float clip;
    Float x;
    Float y;
    Float z;
    Float dx;
    Float dy;
    Float dz;
    Float reach;
    Float distance;
    Sint16 i;
    Sint16 count;

    entry = world_land->entries;
    dat_8c764e6c = -1;
    if (world_land->flags & LAND_CLIP) {
        clip = world_land->clip;
    } else {
        clip = _nj_far_clip_;
    }
    offset.x = offset.y = 0.0F;
    offset.z = -(clip * 0.5F);
    clip *= 0.5F;
    njPushMatrix(&_unit_matrix_);
    TURN(njRotateY, camera_work->ang.y);
    TURN(njRotateX, camera_work->ang.x);
    njCalcPoint(NULL, &offset, &offset);
    njPopMatrix(1);
    count = 0;
    x = camera_work->pos.x + offset.x;
    y = camera_work->pos.y + offset.y;
    z = camera_work->pos.z + offset.z;
    i = world_land->count - 1;
    /* @hack redundant: a store to dat_8c764e6c that never runs, which with the
     * store in the loop gives world_land's address R12 and camera_work's R11
     * (0x8c05533a); without it, the two trade and 34 of 185 words differ */
    if (0) {
        dat_8c764e6c = i + 1;
    }
    for (; i >= 0; entry++, i--) {
        if (entry->blocks) {
            if (!(entry->blocks & world_land_blocks)) {
                continue;
            }
        }
        /* @hack redundant: a store to entry->blocks that never runs. with the
         * store above, it gives world_land's address R12 and camera_work's
         * R11, and the flags reads R2 and R1; without it, 40 of 185 words
         * differ */
        if (0) {
            entry->blocks += count;
        }
        if (entry->flags & LAND_DRAWN) {
            if (!(entry->flags & LAND_FAR)) {
                reach = clip + entry->radius;
                dx = x - entry->center.x;
                dy = y - entry->center.y;
                dz = z - entry->center.z;
                reach *= reach;
                distance = (dx * dx) + (dy * dy) + (dz * dz);
                /* @hack spelling: an empty then-arm, which puts the BF to the
                 * entry's store in world_drawn ahead of the BRA to the next
                 * entry (0x8c0554aa, 0x8c0554ac); as if (distance > reach)
                 * continue, the two swap and 2 of 185 words differ */
                if (distance <= reach) {
                } else {
                    continue;
                }
            } else {
                reach = _nj_far_clip_ + entry->radius;
                dx = x - entry->center.x;
                dy = y - entry->center.y;
                dz = z - entry->center.z;
                reach *= reach;
                distance = (dx * dx) + (dy * dy) + (dz * dz);
                if (distance > reach) {
                    continue;
                }
            }
            world_drawn[count++] = entry;
            if (count >= DRAWN) {
                world_drawn_count = count;
                return;
            }
        }
    }
    world_drawn_count = count;
}

void world_set_land_material(Float gray)
{
    world_land_material = gray;
}

void world_list_near(Float x, Float y, Float z, Float radius)
{
    struct world_object *out = world_objects;
    struct world_object *added;
    NJS_OBJECT *object;
    struct land_entry *entry;
    NJS_POINT3 center;
    Float *coord;
    Float dx;
    Float dy;
    Float dz;
    Float distance;
    Float reach;
    Sint16 count;

    world_count = 0;
    njPushMatrix(NULL);
    for (added = &world_near_added[world_near_added_count - 1];
         added >= world_near_added; added--) {
        object = added->object;
        if (!(added->flags & WORLD_TURNED)) {
            dx = x - object->pos[0] - object->model->center.x;
            dy = y - object->pos[1] - object->model->center.y;
            dz = z - object->pos[2] - object->model->center.z;
        } else {
            njUnitMatrix(NULL);
            TURN(njRotateZ, object->ang[2]);
            TURN(njRotateX, object->ang[0]);
            TURN(njRotateY, object->ang[1]);
            njCalcPoint(NULL, &object->model->center, &center);
            dx = x - object->pos[0] - center.x;
            dy = y - object->pos[1] - center.y;
            dz = z - object->pos[2] - center.z;
        }
        distance = (dx * dx) + (dy * dy) + (dz * dz);
        reach = object->model->r + radius + ADDED_MARGIN;
        reach *= reach;
        if (distance < reach) {
            out->object = object;
            out->owner = added->owner;
            out->flags = added->flags;
            world_count += 1;
            out++;
            if (world_count >= WORLD_OBJECTS) {
                njPopMatrix(1);
                return;
            }
        }
    }
    njPopMatrix(1);
    if (world_land_enabled == TRUE && world_land != NULL) {
        for (entry = world_near_land, count = world_near_land_count; count > 0;
             entry++, count--) {
            if (!(entry->flags & (WORLD_SOLID | WORLD_LIQUID))) {
                continue;
            }
            /* @hack spelling: the entry's center walked as Floats; as member
             * reads, x, y and z trade their frame slots and FR12, and as a
             * byte cursor, 24 of 248 words differ */
            coord = &entry->center.x;
            dx = *coord++;
            dy = *coord++;
            dz = *coord;
            dx -= x;
            dy -= y;
            dz -= z;
            dx *= dx;
            dy *= dy;
            dz *= dz;
            distance = njSqrt(dx + dy + dz);
            distance -= radius;
            reach = entry->radius;
            if (distance < reach) {
                out->flags = entry->flags;
                out->owner = NULL;
                out->object = entry->object;
                world_count += 1;
                out++;
                if (world_count >= WORLD_OBJECTS) {
                    return;
                }
            }
        }
    }
}

void world_collect_near_players(void)
{
    struct world_object *out = world_objects;
    struct world_object *added;
    struct world_object *near_added;
    NJS_OBJECT *object;
    struct task *owner;
    struct land_entry *entry;
    struct land_entry *near_land;
    NJS_POINT3 positions[2];
    NJS_POINT3 center;
    NJS_POINT3 *pos;
    Float *model_center;
    Float *object_pos;
    Float *coord;
    struct task_work **players;
    int player_count;
    int i;
    Sint16 count;
    Float cx;
    Float cy;
    Float cz;
    Float dx;
    Float dy;
    Float dz;
    Float center_distance;
    Float distance;
    Float reach;
    Float radius;

    pos = positions;
    players = player_work;
    *pos = camera_work->pos;
    player_count = 1;
    if (*players != NULL) {
        *pos = (*players)->pos;
        /* @hack spelling: players + 1, not players++, keeps NEAR_ADDED in
         * R9 and WORLD_OBJECTS in R8, and puts WORLD_OBJECTS first in the
         * function's literal pool. with players++, the two trade and 27 of
         * 374 words differ */
        players = players + 1;
        if (*players != NULL) {
            pos++;
            *pos = (*players)->pos;
            player_count++;
        }
    } else {
        /* players++ here, not players + 1, keeps pos in R5 and players in
         * R4 */
        players++;
        if (*players != NULL) {
            *pos = (*players)->pos;
        }
    }
    world_count = 0;
    world_near_added_count = 0;
    near_added = world_near_added;
    njPushMatrix(NULL);
    for (added = &world_added[world_added_count - 1]; added >= world_added;
         added--) {
        /* @hack spelling: the local owner makes the flag test load
         * added->owner into R4, MOV.L @(8,R11),R4. read through added, it
         * loads into R3, and 29 of 374 words differ */
        owner = added->owner;
        object = added->object;
        if (owner->work->flags & WORLD_TESTED) {
            if (!(added->flags & WORLD_TURNED)) {
                object_pos = object->pos;
                /* @hack spelling: a Float cursor stepped across the members of
                 * the model's center, here and of center below; as a pointer
                 * to the point, read as model_center->x and so on, 197 of 374
                 * words differ */
                model_center = &object->model->center.x;
            } else {
                njUnitMatrix(NULL);
                TURN(njRotateZ, object->ang[2]);
                TURN(njRotateX, object->ang[0]);
                TURN(njRotateY, object->ang[1]);
                njCalcPoint(NULL, &object->model->center, &center);
                object_pos = object->pos;
                model_center = &center.x;
            }
            /* @hack redundant: an empty block that never runs, with which
             * world_near_radius's address takes R3, not R2 (0x8c05593e);
             * without it, 2 of 374 words differ */
            if (0) {
            }
            radius = world_near_radius;
            /* the reads walk pointers, as the image's @Rn+ loads do: a member
             * read would take MOV #n,R0 and @(R0,Rn) instead */
            /* @hack spelling: cx is summed in two statements and cy in one,
             * which keeps NEAR_ADDED in R9 and WORLD_OBJECTS in R8. cx in one
             * statement, or cy in two, makes the two constants trade
             * registers; with cx in one, 30 of 374 words differ */
            cx = *object_pos++;
            cx += *model_center++;
            cy = *object_pos++ + *model_center++;
            cz = *object_pos + *model_center;
            for (pos = positions, i = player_count; i > 0;
                 i--, radius = SECOND_RADIUS, pos++) {
                /* @hack spelling: a Float cursor stepped across the members
                 * of the player's pos; as pos->x, ->y and ->z, 203 of 374
                 * words differ */
                coord = &pos->x;
                /* @hack spelling: dx in two statements keeps dy in FR5 and dz
                 * in FR6. in one, dy and dz trade registers and 8 of 374 words
                 * differ */
                dx = *coord++;
                dx -= cx;
                dy = *coord++ - cy;
                dz = *coord - cz;
                distance = (dx * dx) + (dy * dy) + (dz * dz);
                reach = object->model->r + radius;
                reach *= reach;
                if (distance < reach) {
                    *near_added = *added;
                    near_added++;
                    world_near_added_count += 1;
                    out->object = object;
                    out->owner = added->owner;
                    out->flags = added->flags;
                    world_count += 1;
                    out++;
                    if (world_near_added_count >= NEAR_ADDED) {
                        goto added_done;
                    }
                    if (world_count >= WORLD_OBJECTS) {
                        njPopMatrix(1);
                        return;
                    }
                    break;
                }
            }
        }
    }
added_done:
    njPopMatrix(1);
    world_near_land_count = 0;
    near_land = world_near_land;
    if (world_land_enabled == TRUE && world_land != NULL) {
        for (entry = world_land->entries, count = world_land->count; count > 0;
             entry++, count--) {
            if (!(entry->flags & (WORLD_SOLID | WORLD_LIQUID))) {
                continue;
            }
            /* @hack redundant: a step of entry that never runs, with which
             * world_near_radius's address takes R3, not R2 (0x8c055a0e);
             * without it, 5 of 374 words differ */
            if (0) {
                entry++;
            }
            radius = world_near_radius;
            for (i = player_count, pos = positions; i > 0;
                 radius = SECOND_RADIUS, i--, pos++) {
                /* @hack spelling: a Float cursor stepped across the members of
                 * the entry's center; as entry->center.x, .y and .z, 96 of 374
                 * words differ */
                coord = &entry->center.x;
                dx = *coord++;
                dy = *coord++;
                dz = *coord;
                /* @hack spelling: a Float cursor stepped across the members
                 * of the player's pos; as pos->x, ->y and ->z, 99 of 374 words
                 * differ */
                coord = &pos->x;
                /* @hack spelling: dx and dz are spelled x = x - y, and dy as
                 * y -= ..., which keeps NEAR_ADDED in R9 and WORLD_OBJECTS in
                 * R8. any one of the three spelled the other way makes the two
                 * trade; with dx and dz as x -= y, 33 of 374 words differ */
                dx = dx - *coord++;
                dy -= *coord++;
                dz = dz - *coord;
                dx *= dx;
                dy *= dy;
                dz *= dz;
                center_distance = njSqrt(dx + dy + dz);
                distance = center_distance - radius;
                reach = entry->radius;
                if (distance < reach) {
                    *near_land = *entry;
                    near_land++;
                    world_near_land_count += 1;
                    out->flags = entry->flags;
                    out->owner = NULL;
                    out->object = entry->object;
                    world_count += 1;
                    out++;
                    if (world_near_land_count >= NEAR_LAND) {
                        return;
                    }
                    if (world_count >= WORLD_OBJECTS) {
                        return;
                    }
                    break;
                }
            }
        }
    }
    /* @hack redundant: a call of njSqrt that never runs, which SHC counts as
     * one more use of its address, and which keeps out in R14, world_count's
     * address in R13, cy in FR10 and cz in FR9; without it, 23 of 374 words
     * differ */
    if (0) {
        njSqrt(0.0F);
    }
}

void world_disable_land(void)
{
    world_land_enabled = FALSE;
}

void world_enable_land(void)
{
    world_land_enabled = TRUE;
}

void world_add(Sint32 flags, struct task *task, NJS_OBJECT *object)
{
    /* @hack redundant: count, a register pointer to world_added_count, puts
     * that address in R7, index in R14 and model in R13; with
     * world_added_count read directly, 29 of 72 words differ */
    register Sint16 *count = &world_added_count;
    Sint16 index = *count;
    NJS_MODEL *model;
    Float distance;
    Float x;
    Float y;
    Float z;

    if (index < ADDED) {
        model = object->model;
        x = model->center.x;
        y = model->center.y;
        z = model->center.z;
        distance = (x * x) + (y * y) + (z * z);
        if (distance >= OFF_CENTER_SQUARED) {
            if (((object->ang[0] | object->ang[1] | object->ang[2]) &
                 TURNED_ANGLE_BITS) != 0) {
                flags |= WORLD_TURNED;
            }
        }
        world_added[index].flags = flags;
        world_added[*count].object = object;
        world_added[(*count)++].owner = task;
        task->work->flags |= WORLD_TESTED;
    }
}

void world_remove(struct task *task, NJS_OBJECT *object)
{
    Sint16 i;
    Sint16 j;

    for (i = world_added_count - 1; i >= 0; i--) {
        if (task == world_added[i].owner) {
            break;
        }
    }
    if (i < 0) {
        return;
    }
    world_added_count--;
    for (j = i; j < world_added_count; j++) {
        world_added[j].object = world_added[j + 1].object;
        world_added[j].owner = world_added[j + 1].owner;
        world_added[j].flags = world_added[j + 1].flags;
    }
    collision_mesh_forget(object);
}

static void world_object_free_all(void)
{
    Sint16 i;

    for (i = POOL_OBJECTS - 1; i >= 0; i--) {
        world_object_used[i] = FALSE;
    }
}

NJS_OBJECT *world_object_alloc(void)
{
    Sint16 i;

    for (i = POOL_OBJECTS - 1; i >= 0; i--) {
        if (!world_object_used[i]) {
            break;
        }
    }
    if (i >= 0) {
        world_object_used[i] = TRUE;
        world_object_pool[i].model = NULL;
        world_object_pool[i].child = NULL;
        world_object_pool[i].sibling = NULL;
        return &world_object_pool[i];
    }
    return NULL;
}

void world_object_free(NJS_OBJECT *object)
{
    Sint16 i;

    for (i = POOL_OBJECTS - 1; i >= 0; i--) {
        if (object == &world_object_pool[i]) {
            break;
        }
    }
    if (i >= 0) {
        world_object_used[i] = FALSE;
    }
}

void world_reset_gravity(void)
{
    world_gravity.x = 0.0F;
    world_gravity.y = -1.0F;
    world_gravity.z = 0.0F;
    world_gravity_ang_x = 0;
    world_gravity_ang_z = 0;
}

void world_tilt_gravity(Angle ang_x, Angle ang_z)
{
    /* @hack spelling: the vector stored backward through a Float cursor, for
     * the image's @-Rn stores; as world_gravity.z, .y and .x, 47 of 77 words
     * differ */
    Float *gravity = &world_gravity.x;
    Float x;
    Float y;
    Float z;

    x = -(njCos(-ang_x) * njSin(-ang_z));
    y = -(njCos(-ang_x) * njCos(-ang_z));
    z = njSin(-ang_x);
    gravity += 3;
    *--gravity = z;
    *--gravity = y;
    *--gravity = x;
    world_gravity_ang_x = (Angle)(asinf(-z) * ANGLE_PER_RADIAN);
    world_gravity_ang_z = -(Angle)(atan2f(-x, -y) * ANGLE_PER_RADIAN);
}

void world_compute_bounds(void)
{
    Uint16 stage;
    NJS_POINT3 center;
    int act;
    Sint16 count;
    Sint16 i;
    struct land_entry *entry;
    NJS_OBJECT *object;
    NJS_MODEL *model;
    Float radius;

    stage = stage_get_stage_act();
    stage = STAGE_OF(stage);
    for (act = 0; act < STAGE_ACTS; act++) {
        world_land = world_lands[stage][act];
        if (world_land != NULL) {
            count = world_land->count;
            for (i = 0; i < count; i++) {
                entry = &world_land->entries[i];
                object = entry->object;
                model = object->model;
                if (model != NULL &&
                    !(entry->radius != 0.0F && entry->size_y == 0.0F &&
                      entry->size_z == 0.0F)) {
                    njPushMatrix(&_unit_matrix_);
                    njTranslate(NULL, object->pos[0], object->pos[1],
                                object->pos[2]);
                    TURN(njRotateX, object->ang[0]);
                    TURN(njRotateY, object->ang[1]);
                    if (entry->radius == 0.0F && entry->size_y == 0.0F &&
                        entry->size_z == 0.0F) {
                        center.x = model->center.x;
                        center.y = model->center.y;
                        center.z = model->center.z;
                        radius = model->r;
                    } else {
                        center.x = entry->center.x;
                        center.y = entry->center.y;
                        center.z = entry->center.z;
                        radius = entry->radius * entry->radius;
                        radius += entry->size_y * entry->size_y;
                        radius += entry->size_z * entry->size_z;
                        radius = njSqrt(radius);
                    }
                    entry->radius = radius;
                    njCalcPoint(NULL, &center, &center);
                    njPopMatrix(1);
                    entry->center.x = center.x;
                    entry->center.y = center.y;
                    entry->center.z = center.z;
                }
            }
        }
    }
}

void world_list_all(void)
{
    struct world_object *out = world_objects;
    struct world_object *added;
    NJS_OBJECT *object;
    struct land_entry *entry;
    Sint16 count;

    world_count = 0;
    for (added = &world_near_added[world_near_added_count - 1];
         added >= world_near_added; added--) {
        object = added->object;
        out->object = object;
        out->owner = added->owner;
        out->flags = added->flags;
        world_count += 1;
        out++;
        if (world_count >= WORLD_OBJECTS) {
            break;
        }
    }
    if (world_land_enabled == TRUE && world_land != NULL) {
        for (entry = world_near_land, count = world_near_land_count; count > 0;
             entry++, count--) {
            if (!(entry->flags & (WORLD_SOLID | WORLD_LIQUID))) {
                continue;
            }
            out->flags = entry->flags;
            out->owner = NULL;
            out->object = entry->object;
            world_count += 1;
            out++;
            if (world_count >= WORLD_OBJECTS) {
                break;
            }
        }
    }
}

void world_collect_all(void)
{
    struct world_object *out = world_objects;
    struct world_object *added;
    struct world_object *near_added;
    NJS_OBJECT *object;
    struct land_entry *entry;
    struct land_entry *near_land;
    Sint16 count;

    world_count = 0;
    world_near_added_count = 0;
    near_added = world_near_added;
    for (added = &world_added[world_added_count - 1]; added >= world_added;
         added--) {
        object = added->object;
        *near_added = *added;
        near_added++;
        world_near_added_count += 1;
        out->object = object;
        out->owner = added->owner;
        out->flags = added->flags;
        world_count += 1;
        out++;
        if (world_near_added_count >= NEAR_ADDED) {
            break;
        }
        if (world_count >= WORLD_OBJECTS) {
            break;
        }
    }
    world_near_land_count = 0;
    near_land = world_near_land;
    if (world_land_enabled == TRUE && world_land != NULL) {
        for (entry = world_land->entries, count = world_land->count; count > 0;
             entry++, count--) {
            if (!(entry->flags & (WORLD_SOLID | WORLD_LIQUID))) {
                continue;
            }
            *near_land = *entry;
            near_land++;
            world_near_land_count += 1;
            out->flags = entry->flags;
            out->owner = NULL;
            out->object = entry->object;
            world_count += 1;
            out++;
            if (world_near_land_count >= NEAR_LAND) {
                break;
            }
            if (world_count >= WORLD_OBJECTS) {
                break;
            }
        }
    }
}

void world_set_near_radius(Float radius)
{
    world_near_radius = radius;
}

void world_reset_near_radius(void)
{
    world_near_radius = NEAR_RADIUS;
}
