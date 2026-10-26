/* the past's stage task and sky task. in PAST_CHAOS and PAST_FIRE, a task the
 * stage task creates cycles land textures and scrolls one model's uvs. in
 * PAST_FIRE, the sky task swings the land's colors between two palettes */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv03/crowd.h"
#include "adv03/past.h"
#include "camera/camera.h"
#include "draw/light.h"
#include "draw/palette.h"
#include "draw/tree.h"
#include "player/player.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 18
#define TEXT_2_LENGTH 18
#define TEXT_3_LENGTH 20
#define TEXT_4_LENGTH 18
#define TEXT_5_LENGTH 20
#define TEXT_6_LENGTH 20
#define TEXT_7_LENGTH 18
#define TEXT_8_LENGTH 17
#define TEXT_9_LENGTH 19
#define TEXT_10_LENGTH 20
#define TEXT_11_LENGTH 19
#define TEXT_12_LENGTH 20
#define TEXT_13_LENGTH 20
#define TEXT_14_LENGTH 18
#define TEXT_15_LENGTH 18
#define TEXT_16_LENGTH 11
#define TEXT_17_LENGTH 12
#define TEXT_18_LENGTH 13
#define TEXT_19_LENGTH 18
#define TEXT_20_LENGTH 17
#define TEXT_21_LENGTH 15
#define TEXT_22_LENGTH 14
#define TEXT_23_LENGTH 20
#define TEXT_24_LENGTH 19
#define TEXT_25_LENGTH 19
#define TEXT_26_LENGTH 17
#define TEXT_27_LENGTH 19
#define TEXT_28_LENGTH 18
#define TEXT_29_LENGTH 16
#define TEXT_30_LENGTH 12
#define TEXT_31_LENGTH 15
#define TEXT_32_LENGTH 15
#define TEXT_33_LENGTH 14
#define TEXT_34_LENGTH 14
#define TEXT_35_LENGTH 13
#define TEXT_36_LENGTH 11
#define TEXT_37_LENGTH 11
#define TEXT_38_LENGTH 13
#define TEXT_39_LENGTH 13
#define TEXT_40_LENGTH 13
#define TEXT_41_LENGTH 13
#define TEXT_42_LENGTH 13
#define TEXT_43_LENGTH 13
#define TEXT_44_LENGTH 14
#define TEXT_45_LENGTH 13
#define TEXT_46_LENGTH 11
#define TEXT_47_LENGTH 14
#define TEXT_48_LENGTH 15
#define TEXT_49_LENGTH 15
#define TEXT_50_LENGTH 16
#define TEXT_51_LENGTH 13
#define TEXT_52_LENGTH 11
#define TEXT_53_LENGTH 14
#define TEXT_54_LENGTH 11
#define TEXT_55_LENGTH 16
#define TEXT_56_LENGTH 18
#define TEXT_57_LENGTH 14
#define TEXT_58_LENGTH 15
#define TEXT_59_LENGTH 16
#define TEXT_60_LENGTH 15
#define TEXT_61_LENGTH 13
#define TEXT_62_LENGTH 14
#define TEXT_63_LENGTH 15
#define TEXT_64_LENGTH 15
#define TEXT_65_LENGTH 16
#include "adv03/past/text.inc"

/* the land objects the rows of cycles name, PAST_CHAOS's four and then
 * PAST_FIRE's. after them come the models whose uvs act1_animate and
 * act2_animate scroll */
extern NJS_OBJECT dat_0c9958f4;
extern NJS_OBJECT dat_0c995680;
extern NJS_OBJECT dat_0c996324;
extern NJS_OBJECT dat_0c96e844;
extern NJS_OBJECT dat_0c9c7440;
extern NJS_OBJECT dat_0c9c71cc;
extern NJS_OBJECT dat_0c9c7e70;
extern NJS_OBJECT dat_0c99eb34;
extern NJS_OBJECT dat_0c995f4c;
extern NJS_OBJECT dat_0c9c7a98;

/* the act each task last started, which it keeps in its work's timer */
#define PAST_ACT(work) TASK_TIMER(work)

#define PAST_ACT_TASKS 8

/* both tasks' modes. PAST_RUN steps the act until the act changes, PAST_END
 * ends it, and PAST_START starts the new one */
enum past_mode { PAST_INIT, PAST_START, PAST_RUN, PAST_END };

/* the time of day whose sky the sky task draws, which it keeps in its work's
 * tick. past_sky_act_start sets TIME_DAY in PAST_KNUCKLES and PAST_CHAOS, and
 * TIME_NIGHT in PAST_FIRE */
#define SKY_TIME(work) ((work)->tick)

struct sky_vars {
    Uint8 unk_00[8];
    Angle swing; /* the phase of PAST_FIRE's palette swing */
};

#define VARS(work) ((struct sky_vars *)&(work)->vars)

/* a texture animation on a land object's model. it sets the texture of the
 * mats that materials[0] to materials[count - 1] index. timer counts the
 * steps, and each time it reaches delay, the mats move on to the next
 * texture. the textures run from number first to first + frames - 1, then
 * start again at first. frame holds the texture shown, less first */
struct texture_cycle {
    NJS_OBJECT *object;
    Uint8 delay;
    Uint8 count;
    Uint8 materials[2];
    Uint16 first;
    Uint16 frames;
    Uint16 timer;
    Uint16 frame;
};

/* each act's rows of cycles */
#define ACT_CYCLES 4

/* act1_animate's and act2_animate's modes */
enum animate_mode { ANIMATE_INIT, ANIMATE_RUN };

/* scroll_uvs lowers the v of the first SCROLLED_UVS uvs of a model's first
 * meshset by UV_SCROLL each step */
#define SCROLLED_UVS 96
#define UV_SCROLL 8

/* past_act_step refreshes this sound on each step of PAST_KNUCKLES, so it
 * plays for as long as the act runs */
#define SE_ACT0 0x440

/* each step of PAST_FIRE advances the swing's phase by SWING_SPEED, a 64th of
 * a turn, so one swing takes 64 steps */
#define SWING_SPEED 0x400

static void past_act_start(struct task *task);
static void past_act_step(struct task *task);
static void past_act_end(struct task *task);
static void past_destroy(struct task *task);
static void past_sky_destroy(struct task *task);
static void past_sky_act_start(struct task *task);
static void past_sky_act_step(struct task *task);
static void past_sky_act_end(struct task *task);
static void past_sky_draw(struct task *task);
static void load_light(struct task *task);
static void act1_animate(struct task *task);
static void act2_animate(struct task *task);
static void cycles_reset(struct texture_cycle *cycle, int count);
static void cycles_step(struct texture_cycle *cycle, int count);
static void scroll_uvs(NJS_OBJECT *object);
static void fog_init(void);

static NJS_OBJECT sky_act0_day_object;
static NJS_OBJECT sky_act0_evening_object;
static NJS_OBJECT sky_act0_night_object;
static NJS_OBJECT sky_act1_day_object;
static NJS_OBJECT sky_act1_evening_object;
static NJS_OBJECT sky_act1_night_object;
static NJS_TEXLIST sky_act0_texlist;
static NJS_TEXLIST sky_act1_texlist;

/* the skies by act, then by time of day, TIME_DAY to TIME_NIGHT. PAST_FIRE
 * draws PAST_CHAOS's skies, with PAST_CHAOS's texture list */
static NJS_OBJECT *sky_models[3][3] = {
    {&sky_act0_day_object, &sky_act0_evening_object, &sky_act0_night_object},
    {&sky_act1_day_object, &sky_act1_evening_object, &sky_act1_night_object},
    {&sky_act1_day_object, &sky_act1_evening_object, &sky_act1_night_object},
};
static NJS_TEXLIST *sky_texlists[3] = {
    &sky_act0_texlist,
    &sky_act1_texlist,
    &sky_act1_texlist,
};

/* PAST_CHAOS's ACT_CYCLES animations, then PAST_FIRE's. nothing reads
 * PAST_FIRE's: as the @bug in act2_animate says, PAST_FIRE steps
 * PAST_CHAOS's */
static struct texture_cycle cycles[2 * ACT_CYCLES] = {
#include "adv03/past/cycles.inc"
};

static NJS_TEXNAME sky_act1_textures[9] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0}, {TEXT_6, 0, 0},
    {TEXT_7, 0, 0}, {TEXT_8, 0, 0}, {TEXT_9, 0, 0},
};
static NJS_TEXLIST sky_act1_texlist = {
#include "adv03/past/sky_act1_texlist.inc"
};

static NJS_TEXNAME sky_act0_textures[6] = {
    {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0},
};
static NJS_TEXLIST sky_act0_texlist = {
#include "adv03/past/sky_act0_texlist.inc"
};

static NJS_MATERIAL sky_act1_day_materials[3] = {
#include "adv03/past/sky_act1_day_materials.inc"
};

static Sint16 sky_act1_day_meshes0[175] = {
#include "adv03/past/sky_act1_day_meshes0.inc"
};

static Sint16 sky_act1_day_meshes1[35] = {
#include "adv03/past/sky_act1_day_meshes1.inc"
};

static Sint16 sky_act1_day_meshes2[34] = {
#include "adv03/past/sky_act1_day_meshes2.inc"
};

static NJS_COLOR sky_act1_day_uvs0[170] = {
#include "adv03/past/sky_act1_day_uvs0.inc"
};

static NJS_COLOR sky_act1_day_uvs1[34] = {
#include "adv03/past/sky_act1_day_uvs1.inc"
};

static NJS_COLOR sky_act1_day_uvs2[28] = {
#include "adv03/past/sky_act1_day_uvs2.inc"
};

static NJS_MESHSET sky_act1_day_meshsets[3] = {
#include "adv03/past/sky_act1_day_meshsets.inc"
};

static NJS_POINT3 sky_act1_day_points[113] = {
#include "adv03/past/sky_act1_day_points.inc"
};

static NJS_VECTOR sky_act1_day_normals[113] = {
#include "adv03/past/sky_act1_day_normals.inc"
};

static NJS_MODEL sky_act1_day_model = {
#include "adv03/past/sky_act1_day_model.inc"
};

static NJS_OBJECT sky_act1_day_object = {
#include "adv03/past/sky_act1_day_object.inc"
};

static NJS_MATERIAL sky_act1_night_materials[3] = {
#include "adv03/past/sky_act1_night_materials.inc"
};

static Sint16 sky_act1_night_meshes0[175] = {
#include "adv03/past/sky_act1_night_meshes0.inc"
};

static Sint16 sky_act1_night_meshes1[35] = {
#include "adv03/past/sky_act1_night_meshes1.inc"
};

static Sint16 sky_act1_night_meshes2[34] = {
#include "adv03/past/sky_act1_night_meshes2.inc"
};

static NJS_COLOR sky_act1_night_uvs0[170] = {
#include "adv03/past/sky_act1_night_uvs0.inc"
};

static NJS_COLOR sky_act1_night_uvs1[34] = {
#include "adv03/past/sky_act1_night_uvs1.inc"
};

static NJS_COLOR sky_act1_night_uvs2[28] = {
#include "adv03/past/sky_act1_night_uvs2.inc"
};

static NJS_MESHSET sky_act1_night_meshsets[3] = {
#include "adv03/past/sky_act1_night_meshsets.inc"
};

static NJS_POINT3 sky_act1_night_points[113] = {
#include "adv03/past/sky_act1_night_points.inc"
};

static NJS_VECTOR sky_act1_night_normals[113] = {
#include "adv03/past/sky_act1_night_normals.inc"
};

static NJS_MODEL sky_act1_night_model = {
#include "adv03/past/sky_act1_night_model.inc"
};

static NJS_OBJECT sky_act1_night_object = {
#include "adv03/past/sky_act1_night_object.inc"
};

static NJS_MATERIAL sky_act1_evening_materials[3] = {
#include "adv03/past/sky_act1_evening_materials.inc"
};

static Sint16 sky_act1_evening_meshes0[175] = {
#include "adv03/past/sky_act1_evening_meshes0.inc"
};

static Sint16 sky_act1_evening_meshes1[35] = {
#include "adv03/past/sky_act1_evening_meshes1.inc"
};

static Sint16 sky_act1_evening_meshes2[34] = {
#include "adv03/past/sky_act1_evening_meshes2.inc"
};

static NJS_COLOR sky_act1_evening_uvs0[170] = {
#include "adv03/past/sky_act1_evening_uvs0.inc"
};

static NJS_COLOR sky_act1_evening_uvs1[34] = {
#include "adv03/past/sky_act1_evening_uvs1.inc"
};

static NJS_COLOR sky_act1_evening_uvs2[28] = {
#include "adv03/past/sky_act1_evening_uvs2.inc"
};

static NJS_MESHSET sky_act1_evening_meshsets[3] = {
#include "adv03/past/sky_act1_evening_meshsets.inc"
};

static NJS_POINT3 sky_act1_evening_points[113] = {
#include "adv03/past/sky_act1_evening_points.inc"
};

static NJS_VECTOR sky_act1_evening_normals[113] = {
#include "adv03/past/sky_act1_evening_normals.inc"
};

static NJS_MODEL sky_act1_evening_model = {
#include "adv03/past/sky_act1_evening_model.inc"
};

static NJS_OBJECT sky_act1_evening_object = {
#include "adv03/past/sky_act1_evening_object.inc"
};

static NJS_MATERIAL sky_act0_day_materials[2] = {
#include "adv03/past/sky_act0_day_materials.inc"
};

static Sint16 sky_act0_day_meshes0[175] = {
#include "adv03/past/sky_act0_day_meshes0.inc"
};

static Sint16 sky_act0_day_meshes1[34] = {
#include "adv03/past/sky_act0_day_meshes1.inc"
};

static NJS_COLOR sky_act0_day_uvs0[170] = {
#include "adv03/past/sky_act0_day_uvs0.inc"
};

static NJS_COLOR sky_act0_day_uvs1[28] = {
#include "adv03/past/sky_act0_day_uvs1.inc"
};

static NJS_MESHSET sky_act0_day_meshsets[2] = {
#include "adv03/past/sky_act0_day_meshsets.inc"
};

static NJS_POINT3 sky_act0_day_points[97] = {
#include "adv03/past/sky_act0_day_points.inc"
};

static NJS_VECTOR sky_act0_day_normals[97] = {
#include "adv03/past/sky_act0_day_normals.inc"
};

static NJS_MODEL sky_act0_day_model = {
#include "adv03/past/sky_act0_day_model.inc"
};

static NJS_OBJECT sky_act0_day_object = {
#include "adv03/past/sky_act0_day_object.inc"
};

static NJS_MATERIAL sky_act0_evening_materials[2] = {
#include "adv03/past/sky_act0_evening_materials.inc"
};

static Sint16 sky_act0_evening_meshes0[175] = {
#include "adv03/past/sky_act0_evening_meshes0.inc"
};

static Sint16 sky_act0_evening_meshes1[34] = {
#include "adv03/past/sky_act0_evening_meshes1.inc"
};

static NJS_COLOR sky_act0_evening_uvs0[170] = {
#include "adv03/past/sky_act0_evening_uvs0.inc"
};

static NJS_COLOR sky_act0_evening_uvs1[28] = {
#include "adv03/past/sky_act0_evening_uvs1.inc"
};

static NJS_MESHSET sky_act0_evening_meshsets[2] = {
#include "adv03/past/sky_act0_evening_meshsets.inc"
};

static NJS_POINT3 sky_act0_evening_points[97] = {
#include "adv03/past/sky_act0_evening_points.inc"
};

static NJS_VECTOR sky_act0_evening_normals[97] = {
#include "adv03/past/sky_act0_evening_normals.inc"
};

static NJS_MODEL sky_act0_evening_model = {
#include "adv03/past/sky_act0_evening_model.inc"
};

static NJS_OBJECT sky_act0_evening_object = {
#include "adv03/past/sky_act0_evening_object.inc"
};

static NJS_MATERIAL sky_act0_night_materials[2] = {
#include "adv03/past/sky_act0_night_materials.inc"
};

static Sint16 sky_act0_night_meshes0[211] = {
#include "adv03/past/sky_act0_night_meshes0.inc"
};

static Sint16 sky_act0_night_meshes1[34] = {
#include "adv03/past/sky_act0_night_meshes1.inc"
};

static NJS_COLOR sky_act0_night_uvs0[194] = {
#include "adv03/past/sky_act0_night_uvs0.inc"
};

static NJS_COLOR sky_act0_night_uvs1[28] = {
#include "adv03/past/sky_act0_night_uvs1.inc"
};

static NJS_MESHSET sky_act0_night_meshsets[2] = {
#include "adv03/past/sky_act0_night_meshsets.inc"
};

static NJS_POINT3 sky_act0_night_points[97] = {
#include "adv03/past/sky_act0_night_points.inc"
};

static NJS_VECTOR sky_act0_night_normals[97] = {
#include "adv03/past/sky_act0_night_normals.inc"
};

static NJS_MODEL sky_act0_night_model = {
#include "adv03/past/sky_act0_night_model.inc"
};

static NJS_OBJECT sky_act0_night_object = {
#include "adv03/past/sky_act0_night_object.inc"
};

static NJS_TEXNAME obj_past_textures[50] = {
    {TEXT_16, 0, 0}, {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0},
    {TEXT_20, 0, 0}, {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0},
    {TEXT_24, 0, 0}, {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0},
    {TEXT_28, 0, 0}, {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0},
    {TEXT_32, 0, 0}, {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0},
    {TEXT_36, 0, 0}, {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0},
    {TEXT_40, 0, 0}, {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0},
    {TEXT_44, 0, 0}, {TEXT_45, 0, 0}, {TEXT_46, 0, 0}, {TEXT_47, 0, 0},
    {TEXT_48, 0, 0}, {TEXT_49, 0, 0}, {TEXT_50, 0, 0}, {TEXT_51, 0, 0},
    {TEXT_52, 0, 0}, {TEXT_53, 0, 0}, {TEXT_54, 0, 0}, {TEXT_55, 0, 0},
    {TEXT_56, 0, 0}, {TEXT_57, 0, 0}, {TEXT_58, 0, 0}, {TEXT_59, 0, 0},
    {TEXT_60, 0, 0}, {TEXT_61, 0, 0}, {TEXT_62, 0, 0}, {TEXT_63, 0, 0},
    {TEXT_64, 0, 0}, {TEXT_65, 0, 0},
};
NJS_TEXLIST obj_past_texlist = {
#include "adv03/past/obj_past_texlist.inc"
};

static NJS_MATERIAL pyramid_snake_materials[2] = {
#include "adv03/past/pyramid_snake_materials.inc"
};

static Sint16 pyramid_snake_meshes0[160] = {
#include "adv03/past/pyramid_snake_meshes0.inc"
};

static Sint16 pyramid_snake_meshes1[120] = {
#include "adv03/past/pyramid_snake_meshes1.inc"
};

static NJS_COLOR pyramid_snake_uvs0[126] = {
#include "adv03/past/pyramid_snake_uvs0.inc"
};

static NJS_COLOR pyramid_snake_uvs1[96] = {
#include "adv03/past/pyramid_snake_uvs1.inc"
};

static NJS_MESHSET pyramid_snake_meshsets[2] = {
#include "adv03/past/pyramid_snake_meshsets.inc"
};

static NJS_POINT3 pyramid_snake_points[68] = {
#include "adv03/past/pyramid_snake_points.inc"
};

static NJS_VECTOR pyramid_snake_normals[68] = {
#include "adv03/past/pyramid_snake_normals.inc"
};

static NJS_MODEL pyramid_snake_model = {
#include "adv03/past/pyramid_snake_model.inc"
};

NJS_OBJECT pyramid_snake_object = {
#include "adv03/past/pyramid_snake_object.inc"
};

static NJS_FOG_TABLE past_fog;
/* the tasks the stage task creates for an act, for past_act_end to kill.
 * past_act_start fills only the first slot */
static struct task *past_act_tasks[PAST_ACT_TASKS];

/* under section HEAD, SHC writes the two steps to section PHEAD, which
 * leads the module. 1ST_READ.BIN's stage_steps names past_step at
 * 0x0c900000, and its sky_steps names past_sky_step at 0x0c9000c0 */
#pragma section HEAD
void past_step(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    switch (work->mode) {
    case PAST_INIT:
        PAST_ACT(work) = (Uint8)stage_get_stage_act();
        past_messages_load();
        for (i = 0; i < PAST_ACT_TASKS; i++) {
            past_act_tasks[i] = NULL;
        }
        task->destructor = past_destroy;
        work->mode = PAST_START;
        break;
    case PAST_START:
        past_act_start(task);
        work->mode = PAST_RUN;
        break;
    case PAST_RUN:
        if ((Uint8)stage_get_stage_act() != PAST_ACT(work)) {
            work->mode = PAST_END;
        } else {
            past_act_step(task);
        }
        break;
    case PAST_END:
        past_act_end(task);
        work->mode = PAST_START;
        break;
    }
}
/* the functions down to past_sky_step go back to P. built in this run of
 * HEAD, right after past_step, past_sky_step would end past.c's PHEAD at
 * 0x0c900180. that is 32 bytes into start.c's PHEAD, whose past_place_player
 * 1ST_READ.BIN calls at 0x0c900160 */
#pragma section

static void past_act_start(struct task *task)
{
    struct task_work *work = task->work;

    PAST_ACT(work) = (Uint8)stage_get_stage_act();
    player_set_mode(0, PLAYER_MODE_RESET);
    player_set_speed(0, 0.0F, 0.0F, 0.0F);
    camera_set_start_mode(CAMERA_MODE_KNUCKLES);
    task_create(0, 1, stream_init_step);
    switch (PAST_ACT(work)) {
    case PAST_KNUCKLES:
        break;
    case PAST_CHAOS:
        past_act_tasks[0] = task_create(TASK_WORK, 2, act1_animate);
        break;
    case PAST_FIRE:
        past_act_tasks[0] = task_create(TASK_WORK, 2, act2_animate);
        break;
    }
}

static void past_act_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (PAST_ACT(work)) {
    case PAST_KNUCKLES:
        sound_play_for(SE_ACT0, (int)work, 1, 0, 2);
        break;
    case PAST_CHAOS:
        break;
    case PAST_FIRE:
        break;
    }
}

static void past_act_end(struct task *task)
{
    int i;

    for (i = 0; i < PAST_ACT_TASKS; i++) {
        if (past_act_tasks[i] != NULL) {
            task_kill(past_act_tasks[i]);
            past_act_tasks[i] = NULL;
        }
    }
}

static void past_destroy(struct task *task)
{
    past_act_end(task);
}

#pragma section HEAD
void past_sky_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case PAST_INIT:
        task->destructor = past_sky_destroy;
        task->draw = past_sky_draw;
        work->mode = PAST_START;
        break;
    case PAST_START:
        past_sky_act_start(task);
        work->mode = PAST_RUN;
        break;
    case PAST_RUN:
        if ((Uint8)stage_get_stage_act() != PAST_ACT(work)) {
            work->mode = PAST_END;
            return;
        }
        past_sky_act_step(task);
        past_sky_draw(task);
        break;
    case PAST_END:
        past_sky_act_end(task);
        work->mode = PAST_START;
        break;
    }
}
#pragma section

static void past_sky_destroy(struct task *task)
{
    past_sky_act_end(task);
}

static void past_sky_act_start(struct task *task)
{
    struct task_work *work = task->work;

    PAST_ACT(work) = (Uint8)stage_get_stage_act();
    switch (PAST_ACT(work)) {
    case PAST_KNUCKLES:
        SKY_TIME(work) = TIME_DAY;
        fog_init();
        break;
    case PAST_CHAOS:
        SKY_TIME(work) = TIME_DAY;
        fog_init();
        break;
    case PAST_FIRE:
        SKY_TIME(work) = TIME_NIGHT;
        fog_init();
        break;
    }
    load_light(task);
    njSetFogColor(0);
    njSetLightIntensity(&dir_light, 0.8F, 0.9F, 0.5F);
    njSetLightColor(&dir_light, 1.0F, 1.0F, 0.95F);
    njSetLightDirection(&dir_light, 0.5F, -0.866025F, 0.0F);
    njClipZ(-3.0F, -3000.0F);
}

/* in PAST_FIRE, swing palette 0's colors between palette 4's and palette 5's.
 * its base and offset colors take a share of palette 4's, and the rest of
 * palette 5's. the share is (sin(swing) + 1) / 2, which runs from 0 to 1 and
 * back */
static void past_sky_act_step(struct task *task)
{
    struct task_work *work = task->work;
    Float blend;

    switch (PAST_ACT(work)) {
    case PAST_KNUCKLES:
        break;
    case PAST_CHAOS:
        break;
    case PAST_FIRE:
        blend = (njSin(VARS(work)->swing) + 1.0F) * 0.5F;
        VARS(work)->swing += SWING_SPEED;
        palette_mix(palette_get(0, 0), palette_get(4, 0), palette_get(5, 0),
                    blend);
        palette_mix(palette_get(0, 1), palette_get(4, 1), palette_get(5, 1),
                    blend);
        break;
    }
}

static void past_sky_act_end(struct task *task)
{
    struct task_work *work = task->work;

    switch (PAST_ACT(work)) {
    case PAST_KNUCKLES:
    case PAST_CHAOS:
    case PAST_FIRE:
        break;
    }
}

static void past_sky_draw(struct task *task)
{
    struct task_work *work = task->work;
    int act = PAST_ACT(work);
    int time = SKY_TIME(work);

    draw_control_save();
    draw_control_set(NJD_CONTROL_3D_NO_CLIP_CHECK);
    draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
    njFogDisable();
    njPushMatrix(NULL);
    njTranslate(NULL, camera_work->pos.x, 0.0F, camera_work->pos.z);
    njScale(NULL, 2.0F, 2.0F, 2.0F);
    njSetTexture(sky_texlists[act]);
    draw_tree(sky_models[act][time]);
    njPopMatrix(1);
    njFogEnable();
    draw_control_restore();
}

static void load_light(struct task *task)
{
    struct task_work *work = task->work;
    int act = PAST_ACT(work);

    light_read_disc_sources(STAGE_PAST, act);
    light_read_disc_palettes(STAGE_PAST, act);
}

static void act1_animate(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case ANIMATE_INIT:
        cycles_reset(cycles, ACT_CYCLES);
        work->mode = ANIMATE_RUN;
        break;
    case ANIMATE_RUN:
        cycles_step(cycles, ACT_CYCLES);
        scroll_uvs(&dat_0c995f4c);
        break;
    }
}

static void act2_animate(struct task *task)
{
    struct task_work *work = task->work;

    /* @bug logic: both calls pass cycles, PAST_CHAOS's rows, where
     * PAST_FIRE's start at cycles + ACT_CYCLES. the MOV.L at 0x0c90074a loads
     * the address act1_animate's at 0x0c900718 loads. in PAST_FIRE,
     * cycles_step animates PAST_CHAOS's land objects, and PAST_FIRE's keep the
     * textures they load with */
    switch (work->mode) {
    case ANIMATE_INIT:
        cycles_reset(cycles, ACT_CYCLES);
        work->mode = ANIMATE_RUN;
        break;
    case ANIMATE_RUN:
        cycles_step(cycles, ACT_CYCLES);
        scroll_uvs(&dat_0c9c7a98);
        break;
    }
}

static void cycles_reset(struct texture_cycle *cycle, int count)
{
    int i;
    int j;

    for (i = 0; i < count; i++, cycle++) {
        cycle->timer = 0;
        cycle->frame = 0;
        for (j = 0; j < cycle->count; j++) {
            cycle->object->model->mats[cycle->materials[j]].attr_texId =
                cycle->first;
        }
    }
}

static void cycles_step(struct texture_cycle *cycle, int count)
{
    int i;
    int j;
    int texture;

    for (i = 0; i < count; i++, cycle++) {
        cycle->timer++;
        if (cycle->timer == cycle->delay) {
            cycle->timer = 0;
            cycle->frame++;
            if (cycle->frame == cycle->frames) {
                cycle->frame = 0;
            }
            texture = cycle->first + cycle->frame;
            for (j = 0; j < cycle->count; j++) {
                cycle->object->model->mats[cycle->materials[j]].attr_texId =
                    texture;
            }
        }
    }
}

static void scroll_uvs(NJS_OBJECT *object)
{
    NJS_COLOR *uv = object->model->meshsets->vertuv;
    int i;

    for (i = 0; i < SCROLLED_UVS; i++, uv++) {
        uv->tex.v -= UV_SCROLL;
    }
}

/* set black fog from 5000 to 8000 out, disabled until past_sky_draw enables
 * it after drawing the sky */
static void fog_init(void)
{
    njSetFogColor(0);
    njGenerateFogTable3(past_fog, -5000.0F, -8000.0F);
    njSetFogTable(past_fog);
    njFogDisable();
}
