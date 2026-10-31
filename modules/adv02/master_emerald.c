/* the emerald the object list names O MASTER EMERALD: the Master Emerald,
 * which turns and bobs under two glows, in the form FLAG_MASTER_EMERALD
 * picks */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/master_emerald.h"
#include "adv02/texlists.h"
#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "debug/print.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "event/play.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 17
#include "adv02/master_emerald/text.inc"

extern NJS_OBJECT mystic_ruins_master_emerald_0_object;
extern NJS_OBJECT mystic_ruins_master_emerald_1_object;
extern NJS_OBJECT mystic_ruins_master_emerald_2_object;
extern NJS_OBJECT mystic_ruins_master_emerald_3_object;
extern NJS_OBJECT mystic_ruins_master_emerald_4_object;
extern NJS_MODEL mystic_ruins_master_emerald_glow_model;
#ifdef GCC
/* repairs the read past the glow's one normal. its model has 4 points but
 * one normal, at 0x0cb04da4. njSimpleDrawModel reads a normal for each
 * point, so the image's last 3 reads run past that one, into the meshset
 * and the first point (the reads at 0x8c62bba8 or 0x8c62bd64 in
 * 1ST_READ.BIN). GCC's glow holds a normal for each point. its material
 * ignores the light, so no normal changes how it draws */
static NJS_VECTOR master_emerald_glow_normals[4] = {
    {0.0F, 1.0F, 0.0F},
    {0.0F, 1.0F, 0.0F},
    {0.0F, 1.0F, 0.0F},
    {0.0F, 1.0F, 0.0F},
};
#endif

#define FORMS 5 /* the emerald's forms, in mystic_ruins_master_emeralds */

/* placement_try_leave_by ends the task once the camera and player 0 are
 * both beyond 1800 */
#define RANGE (1800.0F * 1800.0F)

#define RADIUS 50.0F /* the collision record's radius */

#define BOB_SPEED 200 /* the bob's phase, in angle units each step */
#define BOB_HEIGHT 1.0F
#define TURN_SPEED 160 /* the emerald's turn, in angle units each step */

/* a quarter turn about x, which lays the second glow flat */
#define LIE_FLAT (-0x4000)

/* two events of Knuckles' story: EV0082, where he searches Station Square,
 * and EV0091, where he brings pieces of the emerald back to the altar */
#define EVENT_STATION_SEARCH 130
#define EVENT_PIECES_BACK 145

enum master_emerald_mode { EMERALD_INIT, EMERALD_RUN };

/* the emerald's form, an index into mystic_ruins_master_emeralds, in
 * work->tick. work->timer counts the emerald's steps */
#define FORM(work) ((work)->tick)

static void mystic_ruins_master_emerald_init(struct task *task);
static void mystic_ruins_master_emerald_reset(struct task *task);
static void mystic_ruins_master_emerald_update(struct task *task);
static void mystic_ruins_master_emerald_draw(struct task *task);
static void mystic_ruins_master_emerald_destroy(struct task *task);

static NJS_ACTION mystic_ruins_master_emeralds[FORMS] = {
#include "adv02/master_emerald/mystic_ruins_master_emeralds.inc"
};

/* a word no code reads, which the image holds after
 * mystic_ruins_master_emeralds */
static int dat_0cb20868 = 0;

/* while the set editor holds the emerald, each step sets FLAG_MASTER_EMERALD
 * again and labels the editor's fields. the field it labels KIND, scale x, is
 * never read: the form follows the flag */
void mystic_ruins_master_emerald_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        if (!placement_editing(task)) {
            switch (work->mode) {
            case EMERALD_INIT:
                mystic_ruins_master_emerald_init(task);
                break;
            case EMERALD_RUN:
                mystic_ruins_master_emerald_draw(task);
                mystic_ruins_master_emerald_update(task);
                break;
            }
        } else {
            switch (work->mode) {
            case EMERALD_INIT:
                mystic_ruins_master_emerald_init(task);
                break;
            case EMERALD_RUN:
                mystic_ruins_master_emerald_reset(task);
                mystic_ruins_master_emerald_draw(task);
                mystic_ruins_master_emerald_update(task);
                debug_print(EDITOR_LABEL(EDITOR_ROW_ANG_Y), "-> ROTATE Y");
                debug_print(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> KIND (0-%d)",
                            FORMS - 1);
                break;
            }
        }
    }
}

/* a texture of the emerald's own, and a sprite of it, which no code in this
 * image draws */
static NJS_TEXNAME mystic_ruins_master_emerald_textures[1] = {
    {TEXT_1, 0, 0},
};
static NJS_TEXLIST mystic_ruins_master_emerald_texlist = {
#include "adv02/master_emerald/mystic_ruins_master_emerald_texlist.inc"
};
static NJS_TEXANIM mystic_ruins_master_emerald_frames[1] = {
#include "adv02/master_emerald/mystic_ruins_master_emerald_frames.inc"
};
static NJS_SPRITE mystic_ruins_master_emerald_sprite = {
#include "adv02/master_emerald/mystic_ruins_master_emerald_sprite.inc"
};

/* a sphere round the emerald, which pushes every group and which no group
 * can push */
static struct collision_shape mystic_ruins_master_emerald_shape[1] = {
    {SHAPE_SPHERE | SHAPE_NO_PUSH,
     0,
     PUSH_ALL | PUSH_FIRM(PUSH_ALL),
     {0.0F, 8.0F, 0.0F},
     25.0F,
     8.0F,
     0.0F,
     {0},
     0,
     NULL},
};

/* the glows' material, in draw_set_material's order: an alpha, then red,
 * green and blue. each color moves by the pulse times its
 * master_emerald_glow_*_swing. the pulse is a sine whose phase moves
 * master_emerald_glow_pulse_speed each step */
static Float master_emerald_glow_alpha = 0.25F;
static Float master_emerald_glow_red = -1.0F;
static Float master_emerald_glow_green = -0.3F;
static Float master_emerald_glow_blue = -0.9F;
static Float master_emerald_glow_red_swing = 0.0F;
static Float master_emerald_glow_green_swing = 0.3F;
static Float master_emerald_glow_blue_swing = 0.1F;
static Angle master_emerald_glow_pulse_speed = 300;
/* the glow that faces the camera: its scale, its height over the bobbing
 * emerald, and how far it stands toward the camera */
static Float master_emerald_glow_scale = 3.3F;
static Float master_emerald_glow_height = 23.0F;
static Float master_emerald_glow_forward = 28.8F;
/* the glow that lies flat: its height over the emerald's position, and its
 * scale */
static Float master_emerald_floor_glow_height = 9.7F;
static Float master_emerald_floor_glow_scale = 6.5F;

static void mystic_ruins_master_emerald_init(struct task *task)
{
    struct task_work *work = task->work;

    task->draw = mystic_ruins_master_emerald_draw;
    task->destructor = mystic_ruins_master_emerald_destroy;
    work->timer = 0;
    collision_init(task, mystic_ruins_master_emerald_shape, COLLISION_OBJECT, 1,
                   1);
    work->collision->radius = RADIUS;
    mystic_ruins_master_emerald_reset(task);
    work->mode = EMERALD_RUN;
}

/* set FLAG_MASTER_EMERALD from the story so far, for Sonic, Knuckles and
 * E-102. Knuckles leaves it as it is while an event has control */
static void mystic_ruins_master_emerald_reset(struct task *task)
{
    switch (get_character()) {
    case CHARACTER_SONIC:
        if (event_flag_check(FLAG_LAST_STORY)) {
            event_flags[FLAG_MASTER_EMERALD] = 0;
        } else {
            event_flags[FLAG_MASTER_EMERALD] = MASTER_EMERALD_INCOMPLETE;
        }
        break;
    case CHARACTER_KNUCKLES:
        if (event_in_control()) {
            break;
        }
        /* @hack redundant: a never-run store of MASTER_EMERALD_INCOMPLETE to
         * the flag, which keeps 0 in R12 and 2 in R13 (0x0c90946e,
         * 0x0c909478), as the image has them; without it, the two trade and 6
         * of 66 words differ */
        if (0) {
            event_flags[FLAG_MASTER_EMERALD] = MASTER_EMERALD_INCOMPLETE;
        }
        if (event_flag_check(FLAG_CLEAR_KNUCKLES)) {
            event_flags[FLAG_MASTER_EMERALD] = 0;
        } else if (event_was_played(EVENT_PIECES_BACK)) {
            event_flags[FLAG_MASTER_EMERALD] = MASTER_EMERALD_INCOMPLETE;
        } else if (event_was_played(EVENT_STATION_SEARCH)) {
            event_flags[FLAG_MASTER_EMERALD] = MASTER_EMERALD_SHATTERED;
        } else {
            event_flags[FLAG_MASTER_EMERALD] = 0;
        }
        break;
    case CHARACTER_E102:
        event_flags[FLAG_MASTER_EMERALD] = MASTER_EMERALD_INCOMPLETE;
    }
}

/* the form follows FLAG_MASTER_EMERALD: MASTER_EMERALD_SHATTERED and
 * MASTER_EMERALD_INCOMPLETE pick forms 1 and 2, and MASTER_EMERALD_WHOLE
 * picks form 0. any other value leaves the form as it is, so no step picks
 * forms 3 and 4 */
static void mystic_ruins_master_emerald_update(struct task *task)
{
    struct task_work *work = task->work;

    switch (event_flags[FLAG_MASTER_EMERALD]) {
    case MASTER_EMERALD_SHATTERED:
        FORM(work) = 1;
        break;
    case MASTER_EMERALD_INCOMPLETE:
        FORM(work) = 2;
        break;
    case MASTER_EMERALD_WHOLE:
        FORM(work) = 0;
    default:
        break;
    }
    collision_register(work);
    work->timer++;
}

/* draw the emerald, then the glow that faces the camera and the flat glow
 * below it. both glows blend additively. at a frame's later steps the draw
 * does nothing */
static void mystic_ruins_master_emerald_draw(struct task *task)
{
    struct task_work *work = task->work;
    Float pulse;
    Float x;
    Float y;
    Float z;
    Angle spin;
    Angle yaw;

    if (frame_step) {
        return;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, njSin(TASK_TIMER(work) * BOB_SPEED) * BOB_HEIGHT,
                0.0F);
    njPushMatrix(NULL);
    TURN_VIA(njRotateY, TASK_TIMER(work) * TURN_SPEED, spin);
    njSetTexture(&mrobj_texlist);
    draw_tree(mystic_ruins_master_emeralds[FORM(work)].object);
    njPopMatrix(1);
    /* @hack redundant: a never-run block of calls of njSetTexture and
     * njTranslate, which keeps the image's registers the same way:
     * njPushMatrix's address in R12, njTranslate's in R13, njSetTexture's in
     * R11 and njPopMatrix's in R10 (0x0c909604 to 0x0c909670); without it, each
     * pair trades, the stack slots shift, and 28 of 269 words differ */
    if (0) {
        njSetTexture(&mystic_ruins_master_emerald_texlist);
        njTranslate(NULL, 0.0F, 0.0F, 0.0F);
    }
    pulse = njSin(TASK_TIMER(work) * master_emerald_glow_pulse_speed);
    draw_set_material(
        master_emerald_glow_alpha,
        master_emerald_glow_red + (pulse * master_emerald_glow_red_swing),
        master_emerald_glow_green + (pulse * master_emerald_glow_green_swing),
        master_emerald_glow_blue + (pulse * master_emerald_glow_blue_swing));
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    x = camera_work->pos.x - work->pos.x;
    y = camera_work->pos.y - work->pos.y - master_emerald_glow_height;
    z = camera_work->pos.z - work->pos.z;
    njTranslate(NULL, 0.0F, master_emerald_glow_height, 0.0F);
    TURN(njRotateY, yaw = (Angle)(atan2f(x, z) * ANGLE_PER_RADIAN));
    TURN(njRotateX,
         (Angle)(atan2f(-y, njSqrt((x * x) + (z * z))) * ANGLE_PER_RADIAN));
    njTranslate(NULL, 0.0F, 0.0F, master_emerald_glow_forward);
    njScale(NULL, master_emerald_glow_scale, master_emerald_glow_scale,
            master_emerald_glow_scale);
    njSetTexture(&mrobj_texlist);
#ifdef GCC
    mystic_ruins_master_emerald_glow_model.normals =
        master_emerald_glow_normals;
#endif
    njSimpleDrawModel(&mystic_ruins_master_emerald_glow_model);
    njPopMatrix(1);
    njTranslate(NULL, 0.0F, master_emerald_floor_glow_height, 0.0F);
    njRotateX(NULL, LIE_FLAT);
    njScale(NULL, master_emerald_floor_glow_scale,
            master_emerald_floor_glow_scale, master_emerald_floor_glow_scale);
    njSetTexture(&mrobj_texlist);
    njSimpleDrawModel(&mystic_ruins_master_emerald_glow_model);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
}

static void mystic_ruins_master_emerald_destroy(struct task *task)
{
}
