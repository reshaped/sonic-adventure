/* the field and sky tasks of the Egg Carrier's inside stage, and the texture
 * lists the stage and its acts load */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv0100/station.h"
#include "adv0130/dat_0ca14ff0.h"
#include "adv0130/egg_carrier_inside.h"
#include "adv0130/hall_lights.h"
#include "adv0130/people.h"
#include "adv0130/reservoir.h"
#include "camera/camera.h"
#include "event/play.h"
#include "main.h"
#include "player/player.h"
#include "progress/emblem.h"
#include "progress/progress.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg00/mole_textures.h"
#include "system/task.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 12
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 13
#define TEXT_7_LENGTH 11
#define TEXT_8_LENGTH 12
#define TEXT_9_LENGTH 12
#define TEXT_10_LENGTH 12
#define TEXT_11_LENGTH 12
#define TEXT_12_LENGTH 12
#define TEXT_13_LENGTH 12
#define TEXT_14_LENGTH 12
#define TEXT_15_LENGTH 11
#define TEXT_16_LENGTH 11
#define TEXT_17_LENGTH 11
#define TEXT_18_LENGTH 11
#define TEXT_19_LENGTH 11
#define TEXT_20_LENGTH 11
#define TEXT_21_LENGTH 11
#define TEXT_22_LENGTH 11
#define TEXT_23_LENGTH 13
#define TEXT_24_LENGTH 13
#define TEXT_25_LENGTH 13
#define TEXT_26_LENGTH 11
#define TEXT_27_LENGTH 12
#define TEXT_28_LENGTH 11
#define TEXT_29_LENGTH 11
#define TEXT_30_LENGTH 11
#define TEXT_31_LENGTH 11
#define TEXT_32_LENGTH 11
#define TEXT_33_LENGTH 13
#define TEXT_34_LENGTH 13
#define TEXT_35_LENGTH 13
#define TEXT_36_LENGTH 11
#define TEXT_37_LENGTH 11
#define TEXT_38_LENGTH 11
#define TEXT_39_LENGTH 12
#define TEXT_40_LENGTH 12
#define TEXT_41_LENGTH 12
#define TEXT_42_LENGTH 12
#define TEXT_43_LENGTH 12
#define TEXT_44_LENGTH 10
#define TEXT_45_LENGTH 12
#define TEXT_46_LENGTH 12
#define TEXT_47_LENGTH 16
#define TEXT_48_LENGTH 11
#define TEXT_49_LENGTH 13
#define TEXT_50_LENGTH 11
#define TEXT_51_LENGTH 14
#define TEXT_52_LENGTH 12
#define TEXT_53_LENGTH 14
#define TEXT_54_LENGTH 10
#define TEXT_55_LENGTH 17
#define TEXT_56_LENGTH 12
#define TEXT_57_LENGTH 13
#define TEXT_58_LENGTH 12
#define TEXT_59_LENGTH 9
#define TEXT_60_LENGTH 10
#define TEXT_61_LENGTH 10
#define TEXT_62_LENGTH 11
#define TEXT_63_LENGTH 13
#define TEXT_64_LENGTH 13
#define TEXT_65_LENGTH 6
#define TEXT_66_LENGTH 10
#define TEXT_67_LENGTH 12
#define TEXT_68_LENGTH 10
#define TEXT_69_LENGTH 10
#define TEXT_70_LENGTH 10
#define TEXT_71_LENGTH 10
#define TEXT_72_LENGTH 10
#define TEXT_73_LENGTH 10
#define TEXT_74_LENGTH 10
#define TEXT_75_LENGTH 12
#define TEXT_76_LENGTH 12
#define TEXT_77_LENGTH 12
#define TEXT_78_LENGTH 7
#define TEXT_79_LENGTH 8
#define TEXT_80_LENGTH 7
#define TEXT_81_LENGTH 9
#define TEXT_82_LENGTH 6
#define TEXT_83_LENGTH 12
#define TEXT_84_LENGTH 7
#define TEXT_85_LENGTH 13
#define TEXT_86_LENGTH 14
#define TEXT_87_LENGTH 9
#define TEXT_88_LENGTH 10
#define TEXT_89_LENGTH 6
#define TEXT_90_LENGTH 7
#define TEXT_91_LENGTH 11
#define TEXT_92_LENGTH 11
#define TEXT_93_LENGTH 7
#define TEXT_94_LENGTH 6
#define TEXT_95_LENGTH 7
#define TEXT_96_LENGTH 7
#define TEXT_97_LENGTH 12
#define TEXT_98_LENGTH 8
#define TEXT_99_LENGTH 9
#define TEXT_100_LENGTH 9
#define TEXT_101_LENGTH 9
#define TEXT_102_LENGTH 6
#define TEXT_103_LENGTH 12
#define TEXT_104_LENGTH 14
#define TEXT_105_LENGTH 14
#define TEXT_106_LENGTH 14
#define TEXT_107_LENGTH 14
#define TEXT_108_LENGTH 14
#define TEXT_109_LENGTH 13
#define TEXT_110_LENGTH 11
#define TEXT_111_LENGTH 10
#define TEXT_112_LENGTH 13
#define TEXT_113_LENGTH 12
#define TEXT_114_LENGTH 12
#define TEXT_115_LENGTH 12
#define TEXT_116_LENGTH 11
#define TEXT_117_LENGTH 12
#define TEXT_118_LENGTH 13
#define TEXT_119_LENGTH 12
#define TEXT_120_LENGTH 12
#define TEXT_121_LENGTH 11
#define TEXT_122_LENGTH 11
#define TEXT_123_LENGTH 13
#define TEXT_124_LENGTH 12
#define TEXT_125_LENGTH 12
#define TEXT_126_LENGTH 12
#define TEXT_127_LENGTH 10
#define TEXT_128_LENGTH 12
#define TEXT_129_LENGTH 12
#define TEXT_130_LENGTH 9
#define TEXT_131_LENGTH 10
#define TEXT_132_LENGTH 10
#define TEXT_133_LENGTH 13
#define TEXT_134_LENGTH 13
#define TEXT_135_LENGTH 16
#define TEXT_136_LENGTH 13
#define TEXT_137_LENGTH 12
#define TEXT_138_LENGTH 13
#define TEXT_139_LENGTH 13
#define TEXT_140_LENGTH 13
#define TEXT_141_LENGTH 13
#define TEXT_142_LENGTH 13
#define TEXT_143_LENGTH 13
#define TEXT_144_LENGTH 13
#define TEXT_145_LENGTH 13
#define TEXT_146_LENGTH 13
#define TEXT_147_LENGTH 13
#define TEXT_148_LENGTH 13
#define TEXT_149_LENGTH 13
#define TEXT_150_LENGTH 12
#define TEXT_151_LENGTH 13
#define TEXT_152_LENGTH 15
#define TEXT_153_LENGTH 16
#define TEXT_154_LENGTH 11
#define TEXT_155_LENGTH 10
#define TEXT_156_LENGTH 10
#define TEXT_157_LENGTH 10
#define TEXT_158_LENGTH 10
#define TEXT_159_LENGTH 17
#define TEXT_160_LENGTH 11
#define TEXT_161_LENGTH 10
#define TEXT_162_LENGTH 12
#define TEXT_163_LENGTH 7
#define TEXT_164_LENGTH 12
#define TEXT_165_LENGTH 14
#define TEXT_166_LENGTH 10
#define TEXT_167_LENGTH 10
#define TEXT_168_LENGTH 10
#define TEXT_169_LENGTH 10
#define TEXT_170_LENGTH 10
#define TEXT_171_LENGTH 10
#define TEXT_172_LENGTH 10
#define TEXT_173_LENGTH 10
#define TEXT_174_LENGTH 10
#define TEXT_175_LENGTH 10
#define TEXT_176_LENGTH 10
#define TEXT_177_LENGTH 10
#define TEXT_178_LENGTH 10
#define TEXT_179_LENGTH 10
#define TEXT_180_LENGTH 13
#define TEXT_181_LENGTH 11
#define TEXT_182_LENGTH 8
#define TEXT_183_LENGTH 10
#define TEXT_184_LENGTH 10
#define TEXT_185_LENGTH 9
#define TEXT_186_LENGTH 10
#define TEXT_187_LENGTH 12
#define TEXT_188_LENGTH 13
#define TEXT_189_LENGTH 12
#define TEXT_190_LENGTH 12
#define TEXT_191_LENGTH 11
#define TEXT_192_LENGTH 11
#define TEXT_193_LENGTH 11
#define TEXT_194_LENGTH 11
#define TEXT_195_LENGTH 8
#define TEXT_196_LENGTH 8
#define TEXT_197_LENGTH 8
#define TEXT_198_LENGTH 6
#define TEXT_199_LENGTH 8
#define TEXT_200_LENGTH 8
#define TEXT_201_LENGTH 8
#define TEXT_202_LENGTH 12
#define TEXT_203_LENGTH 8
#define TEXT_204_LENGTH 8
#define TEXT_205_LENGTH 8
#define TEXT_206_LENGTH 8
#include "adv0130/egg_carrier_inside/text.inc"

/* the act the field or sky task last started */
#define INSIDE_ACT(work) TASK_TIMER(work)

/* capacity of inside_act_tasks, the act's tasks that inside_act_end destroys */
#define INSIDE_ACT_TASKS 8

enum inside_mode { INSIDE_INIT, INSIDE_START, INSIDE_RUN, INSIDE_END };

/* the music task asks for its music on its step MUSIC_DELAY + 1 */
#define MUSIC_DELAY 10

static void inside_act_start(struct task *task);
static void inside_act_step(struct task *task);
static void inside_act_end(struct task *task);
static void inside_destroy(struct task *task);
static void music_step(struct task *task);
static void sky_act_start(struct task *task);
static void sky_destroy(struct task *task);

static struct task *inside_act_tasks[INSIDE_ACT_TASKS];

/* #pragma section HEAD puts what follows in PHEAD, CHEAD and DHEAD, which
 * lead the module. 1ST_READ.BIN names three of these items by address:
 * sky_steps the sky task's step, stage_pvms egg_carrier_inside_pvms, and
 * adv0130_acts the rows of egg_carrier_inside_act_pvms. the texture lists those
 * tables name lie in the head too */
#pragma section HEAD
void egg_carrier_inside_sky_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case INSIDE_INIT:
        INSIDE_ACT(work) = (Uint8)stage_get_stage_act();
        task->destructor = sky_destroy;
        work->mode = INSIDE_START;
        break;
    case INSIDE_START:
        sky_act_start(task);
        work->mode = INSIDE_RUN;
        break;
    case INSIDE_RUN:
        if ((Uint8)stage_get_stage_act() != INSIDE_ACT(work)) {
            work->mode = INSIDE_END;
            /* @hack redundant: a break that repeats the case's own, which ends
             * the case on the tail it shares with INSIDE_INIT: the BRA at
             * 0x0c92004e, with the mode's store in its delay slot, which
             * INSIDE_INIT's BRA at 0x0c92002e reaches too. without it, count
             * unmeasured: the step outgrows PHEAD's row, and the link fails */
            break;
        }
        break;
    case INSIDE_END:
        work->mode = INSIDE_START;
        break;
    }
}

static NJS_TEXNAME adv_ec30_textures[] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0}, {TEXT_28, 0, 0},
    {TEXT_29, 0, 0}, {TEXT_30, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},
    {TEXT_33, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0}, {TEXT_36, 0, 0},
    {TEXT_37, 0, 0}, {TEXT_38, 0, 0}, {TEXT_39, 0, 0}, {TEXT_40, 0, 0},
    {TEXT_41, 0, 0}, {TEXT_42, 0, 0}, {TEXT_43, 0, 0}, {TEXT_44, 0, 0},
};
static NJS_TEXLIST adv_ec30_texlist = {
#include "adv0130/egg_carrier_inside/adv_ec30_texlist.inc"
};

static NJS_TEXNAME adv_ec31_textures[] = {
    {TEXT_45, 0, 0}, {TEXT_46, 0, 0}, {TEXT_2, 0, 0},  {TEXT_47, 0, 0},
    {TEXT_48, 0, 0}, {TEXT_49, 0, 0}, {TEXT_50, 0, 0}, {TEXT_51, 0, 0},
    {TEXT_8, 0, 0},  {TEXT_36, 0, 0}, {TEXT_52, 0, 0}, {TEXT_53, 0, 0},
    {TEXT_54, 0, 0}, {TEXT_55, 0, 0}, {TEXT_56, 0, 0}, {TEXT_57, 0, 0},
    {TEXT_58, 0, 0}, {TEXT_59, 0, 0}, {TEXT_60, 0, 0}, {TEXT_61, 0, 0},
    {TEXT_44, 0, 0}, {TEXT_62, 0, 0}, {TEXT_63, 0, 0}, {TEXT_64, 0, 0},
    {TEXT_65, 0, 0}, {TEXT_66, 0, 0}, {TEXT_67, 0, 0}, {TEXT_68, 0, 0},
    {TEXT_69, 0, 0}, {TEXT_70, 0, 0}, {TEXT_71, 0, 0}, {TEXT_72, 0, 0},
    {TEXT_73, 0, 0}, {TEXT_74, 0, 0}, {TEXT_75, 0, 0}, {TEXT_76, 0, 0},
    {TEXT_77, 0, 0}, {TEXT_78, 0, 0}, {TEXT_79, 0, 0}, {TEXT_80, 0, 0},
    {TEXT_81, 0, 0}, {TEXT_82, 0, 0}, {TEXT_83, 0, 0}, {TEXT_84, 0, 0},
    {TEXT_85, 0, 0},
};
static NJS_TEXLIST adv_ec31_texlist = {
#include "adv0130/egg_carrier_inside/adv_ec31_texlist.inc"
};

static NJS_TEXNAME adv_ec32_textures[] = {
    {TEXT_86, 0, 0},  {TEXT_47, 0, 0}, {TEXT_44, 0, 0},  {TEXT_87, 0, 0},
    {TEXT_88, 0, 0},  {TEXT_89, 0, 0}, {TEXT_83, 0, 0},  {TEXT_84, 0, 0},
    {TEXT_90, 0, 0},  {TEXT_91, 0, 0}, {TEXT_92, 0, 0},  {TEXT_93, 0, 0},
    {TEXT_94, 0, 0},  {TEXT_95, 0, 0}, {TEXT_96, 0, 0},  {TEXT_97, 0, 0},
    {TEXT_98, 0, 0},  {TEXT_99, 0, 0}, {TEXT_100, 0, 0}, {TEXT_101, 0, 0},
    {TEXT_102, 0, 0},
};
static NJS_TEXLIST adv_ec32_texlist = {
#include "adv0130/egg_carrier_inside/adv_ec32_texlist.inc"
};

static NJS_TEXNAME adv_ec33_textures[] = {
    {TEXT_2, 0, 0},   {TEXT_103, 0, 0}, {TEXT_104, 0, 0}, {TEXT_105, 0, 0},
    {TEXT_106, 0, 0}, {TEXT_107, 0, 0}, {TEXT_108, 0, 0}, {TEXT_86, 0, 0},
    {TEXT_109, 0, 0}, {TEXT_110, 0, 0}, {TEXT_47, 0, 0},  {TEXT_111, 0, 0},
    {TEXT_112, 0, 0}, {TEXT_48, 0, 0},  {TEXT_113, 0, 0}, {TEXT_114, 0, 0},
    {TEXT_115, 0, 0}, {TEXT_49, 0, 0},  {TEXT_116, 0, 0}, {TEXT_44, 0, 0},
};
static NJS_TEXLIST adv_ec33_texlist = {
#include "adv0130/egg_carrier_inside/adv_ec33_texlist.inc"
};

static NJS_TEXNAME adv_ec34_textures[] = {
    {TEXT_117, 0, 0}, {TEXT_5, 0, 0},  {TEXT_118, 0, 0}, {TEXT_6, 0, 0},
    {TEXT_7, 0, 0},   {TEXT_8, 0, 0},  {TEXT_9, 0, 0},   {TEXT_10, 0, 0},
    {TEXT_12, 0, 0},  {TEXT_13, 0, 0}, {TEXT_14, 0, 0},  {TEXT_15, 0, 0},
    {TEXT_16, 0, 0},  {TEXT_18, 0, 0}, {TEXT_21, 0, 0},  {TEXT_22, 0, 0},
    {TEXT_25, 0, 0},  {TEXT_26, 0, 0}, {TEXT_119, 0, 0}, {TEXT_27, 0, 0},
    {TEXT_120, 0, 0}, {TEXT_28, 0, 0}, {TEXT_29, 0, 0},  {TEXT_30, 0, 0},
    {TEXT_121, 0, 0}, {TEXT_31, 0, 0}, {TEXT_32, 0, 0},  {TEXT_122, 0, 0},
    {TEXT_123, 0, 0}, {TEXT_34, 0, 0}, {TEXT_35, 0, 0},  {TEXT_36, 0, 0},
    {TEXT_37, 0, 0},  {TEXT_38, 0, 0}, {TEXT_39, 0, 0},  {TEXT_124, 0, 0},
    {TEXT_125, 0, 0}, {TEXT_41, 0, 0}, {TEXT_126, 0, 0}, {TEXT_42, 0, 0},
    {TEXT_43, 0, 0},  {TEXT_44, 0, 0},
};
static NJS_TEXLIST adv_ec34_texlist = {
#include "adv0130/egg_carrier_inside/adv_ec34_texlist.inc"
};

static NJS_TEXNAME adv_ec35_textures[] = {
    {TEXT_127, 0, 0}, {TEXT_128, 0, 0}, {TEXT_129, 0, 0}, {TEXT_1, 0, 0},
    {TEXT_36, 0, 0},  {TEXT_55, 0, 0},  {TEXT_44, 0, 0},  {TEXT_73, 0, 0},
    {TEXT_88, 0, 0},  {TEXT_75, 0, 0},  {TEXT_84, 0, 0},  {TEXT_93, 0, 0},
    {TEXT_95, 0, 0},  {TEXT_97, 0, 0},
};
static NJS_TEXLIST adv_ec35_texlist = {
#include "adv0130/egg_carrier_inside/adv_ec35_texlist.inc"
};

static NJS_TEXNAME obj_ec30_textures[] = {
    {TEXT_130, 0, 0}, {TEXT_131, 0, 0}, {TEXT_132, 0, 0}, {TEXT_46, 0, 0},
    {TEXT_1, 0, 0},   {TEXT_2, 0, 0},   {TEXT_133, 0, 0}, {TEXT_134, 0, 0},
    {TEXT_135, 0, 0}, {TEXT_136, 0, 0}, {TEXT_47, 0, 0},  {TEXT_111, 0, 0},
    {TEXT_112, 0, 0}, {TEXT_48, 0, 0},  {TEXT_137, 0, 0}, {TEXT_49, 0, 0},
    {TEXT_51, 0, 0},  {TEXT_117, 0, 0}, {TEXT_5, 0, 0},   {TEXT_9, 0, 0},
    {TEXT_10, 0, 0},  {TEXT_11, 0, 0},  {TEXT_13, 0, 0},  {TEXT_16, 0, 0},
    {TEXT_20, 0, 0},  {TEXT_21, 0, 0},  {TEXT_138, 0, 0}, {TEXT_139, 0, 0},
    {TEXT_140, 0, 0}, {TEXT_24, 0, 0},  {TEXT_25, 0, 0},  {TEXT_141, 0, 0},
    {TEXT_142, 0, 0}, {TEXT_143, 0, 0}, {TEXT_144, 0, 0}, {TEXT_145, 0, 0},
    {TEXT_26, 0, 0},  {TEXT_27, 0, 0},  {TEXT_28, 0, 0},  {TEXT_29, 0, 0},
    {TEXT_31, 0, 0},  {TEXT_32, 0, 0},  {TEXT_33, 0, 0},  {TEXT_146, 0, 0},
    {TEXT_147, 0, 0}, {TEXT_123, 0, 0}, {TEXT_148, 0, 0}, {TEXT_34, 0, 0},
    {TEXT_149, 0, 0}, {TEXT_35, 0, 0},  {TEXT_36, 0, 0},  {TEXT_37, 0, 0},
    {TEXT_38, 0, 0},  {TEXT_150, 0, 0}, {TEXT_151, 0, 0}, {TEXT_152, 0, 0},
    {TEXT_153, 0, 0}, {TEXT_154, 0, 0}, {TEXT_155, 0, 0}, {TEXT_156, 0, 0},
    {TEXT_157, 0, 0}, {TEXT_158, 0, 0}, {TEXT_54, 0, 0},  {TEXT_55, 0, 0},
    {TEXT_159, 0, 0}, {TEXT_56, 0, 0},  {TEXT_125, 0, 0}, {TEXT_58, 0, 0},
    {TEXT_126, 0, 0}, {TEXT_42, 0, 0},  {TEXT_43, 0, 0},  {TEXT_60, 0, 0},
    {TEXT_61, 0, 0},  {TEXT_160, 0, 0}, {TEXT_161, 0, 0}, {TEXT_44, 0, 0},
    {TEXT_63, 0, 0},  {TEXT_66, 0, 0},  {TEXT_67, 0, 0},  {TEXT_69, 0, 0},
    {TEXT_88, 0, 0},  {TEXT_76, 0, 0},  {TEXT_162, 0, 0}, {TEXT_78, 0, 0},
    {TEXT_79, 0, 0},  {TEXT_81, 0, 0},  {TEXT_82, 0, 0},  {TEXT_84, 0, 0},
    {TEXT_98, 0, 0},  {TEXT_163, 0, 0}, {TEXT_164, 0, 0}, {TEXT_165, 0, 0},
};
NJS_TEXLIST obj_ec30_texlist = {
#include "adv0130/egg_carrier_inside/obj_ec30_texlist.inc"
};

static NJS_TEXNAME ec_water_textures[] = {
    {TEXT_166, 0, 0}, {TEXT_167, 0, 0}, {TEXT_168, 0, 0}, {TEXT_169, 0, 0},
    {TEXT_170, 0, 0}, {TEXT_171, 0, 0}, {TEXT_172, 0, 0}, {TEXT_173, 0, 0},
    {TEXT_174, 0, 0}, {TEXT_175, 0, 0}, {TEXT_176, 0, 0}, {TEXT_177, 0, 0},
    {TEXT_178, 0, 0}, {TEXT_179, 0, 0},
};
NJS_TEXLIST ec_water_texlist = {
#include "adv0130/egg_carrier_inside/ec_water_texlist.inc"
};

static NJS_TEXNAME ec_tarai_textures[] = {
    {TEXT_109, 0, 0}, {TEXT_159, 0, 0}, {TEXT_180, 0, 0}, {TEXT_181, 0, 0},
    {TEXT_182, 0, 0}, {TEXT_183, 0, 0}, {TEXT_184, 0, 0}, {TEXT_99, 0, 0},
    {TEXT_185, 0, 0}, {TEXT_186, 0, 0}, {TEXT_187, 0, 0}, {TEXT_188, 0, 0},
    {TEXT_189, 0, 0}, {TEXT_190, 0, 0}, {TEXT_191, 0, 0}, {TEXT_192, 0, 0},
    {TEXT_193, 0, 0}, {TEXT_194, 0, 0},
};
NJS_TEXLIST ec_tarai_texlist = {
#include "adv0130/egg_carrier_inside/ec_tarai_texlist.inc"
};

/* only the first of the four slot rows holds a list, obj_ec30_texlist,
 * which texture_set_stage makes current for the objects that call it */
struct texture_pvm egg_carrier_inside_pvms[8] = {
    {TEXT_195, &obj_ec30_texlist},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {TEXT_196, &ec_alife_texlist},
    {TEXT_197, &ec_tarai_texlist},
    {TEXT_198, &emblem_texlist},
    {NULL, NULL},
};

/* the acts' rows in act order, as adv0130_acts counts them: one each, but two
 * for act 2 and two for act 4 */
struct texture_pvm egg_carrier_inside_act_pvms[8] = {
    {TEXT_199, &adv_ec30_texlist}, {TEXT_200, &adv_ec31_texlist},
    {TEXT_201, &adv_ec32_texlist}, {TEXT_202, &mole_texlist},
    {TEXT_203, &adv_ec33_texlist}, {TEXT_204, &adv_ec34_texlist},
    {TEXT_205, &ec_water_texlist}, {TEXT_206, &adv_ec35_texlist},
};
#pragma section

void egg_carrier_inside_step(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    switch (work->mode) {
    case INSIDE_INIT:
        for (i = 0; i < INSIDE_ACT_TASKS; i++) {
            inside_act_tasks[i] = NULL;
        }
        INSIDE_ACT(work) = (Uint8)stage_get_stage_act();
        task->destructor = inside_destroy;
        camera_set_start_mode(CAMERA_MODE_KNUCKLES);
        egg_carrier_inside_people_init();
        work->mode = INSIDE_START;
        break;
    case INSIDE_START:
        inside_act_start(task);
        work->mode = INSIDE_RUN;
        break;
    case INSIDE_RUN:
        if ((Uint8)stage_get_stage_act() != INSIDE_ACT(work)) {
            work->mode = INSIDE_END;
        } else {
            inside_act_step(task);
        }
        break;
    case INSIDE_END:
        inside_act_end(task);
        work->mode = INSIDE_START;
        break;
    }
}

static void inside_act_start(struct task *task)
{
    struct task_work *work = task->work;
    struct task *music;

    INSIDE_ACT(work) = (Uint8)stage_get_stage_act();
    player_set_mode(0, PLAYER_MODE_RESET);
    player_set_speed(0, 0.0F, 0.0F, 0.0F);
    njSetBackColor(0, 0, 0);
    music = task_create(TASK_WORK, 0, music_step);
    music->work->mode = MUSIC_EGCARER1;
    music->work->timer = MUSIC_DELAY;
    switch (INSIDE_ACT(work)) {
    case 0:
        break;
    case 1:
        inside_act_tasks[0] =
            task_create(TASK_WORK, 3, egg_carrier_hall_lights_step);
        event_flags[FLAG_MONORAIL_CALLED] = 0;
        break;
    case 2:
    case 3:
        break;
    case 4:
        inside_act_tasks[0] =
            task_create(TASK_WORK | TASK_FORCE, 3, egg_carrier_reservoir_step);
        break;
    case 5:
        break;
    }
}

static void inside_act_step(struct task *task)
{
    /* @hack redundant: work, read and never used. where egg_carrier_inside_step
     * expands the call, it keeps the task in R4, a MOV R13,R4 in a BRA's
     * delay slot (0x0c922514); without it, 28 of 67 words differ */
    struct task_work *work = task->work;
}

static void inside_act_end(struct task *task)
{
    int i;

    for (i = 0; i < INSIDE_ACT_TASKS; i++) {
        if (inside_act_tasks[i] != NULL) {
            task_destroy(inside_act_tasks[i]);
            inside_act_tasks[i] = NULL;
        }
    }
}

static void inside_destroy(struct task *task)
{
    inside_act_end(task);
}

/* count work->timer down, and once it runs out, destroy the task. if no event
 * plays then, as event_check_music says, reset the streams first and ask for
 * the music in work->mode */
static void music_step(struct task *task)
{
    struct task_work *work = task->work;

    work->timer--;
    if (work->timer < 0) {
        if (event_check_music()) {
            stream_reset();
            stream_request_and_save_music(work->mode);
        }
        task_destroy(task);
    }
}

static void sky_act_start(struct task *task)
{
    struct task_work *work = task->work;

    INSIDE_ACT(work) = (Uint8)stage_get_stage_act();
    njSetBackColor(0, 0, 0);
    switch (INSIDE_ACT(work)) {
    case 0:
        njSetFogColor(0xff000000);
        njGenerateFogTable3(fog_table, -5000.0F, -8000.0F);
        njSetFogTable(fog_table);
        break;
    case 1:
        njSetFogColor(0xffa0a0a0);
        njGenerateFogTable3(fog_table, -400.0F, -1500.0F);
        njSetFogTable(fog_table);
        break;
    case 2:
        njSetFogColor(0xff000000);
        njGenerateFogTable3(fog_table, -5000.0F, -8000.0F);
        njSetFogTable(fog_table);
        break;
    case 3:
        njSetFogColor(0xff808040);
        njGenerateFogTable3(fog_table, -80.0F, -400.0F);
        njSetFogTable(fog_table);
        break;
    case 4:
        njSetFogColor(0xff000000);
        njGenerateFogTable3(fog_table, -5000.0F, -8000.0F);
        njSetFogTable(fog_table);
        break;
    case 5:
        njSetFogColor(0xff000000);
        njGenerateFogTable3(fog_table, -5000.0F, -8000.0F);
        njSetFogTable(fog_table);
        break;
    }
}

static void sky_destroy(struct task *task)
{
}
