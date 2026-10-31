/* Super Sonic's aura, wake, spin balls and dash in the Perfect Chaos fight.
 * his model, his texture list and the first steps of the effects' tasks are in
 * the HEAD section. 1ST_READ.BIN's sonic.c reaches them there */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos7/super.h"
#include "collision/collision.h"
#include "draw/tree.h"
#include "effect/dust.h"
#include "effect/splash.h"
#include "player/player.h"
#include "player/sonic_effect_models.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 10
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 10
#define TEXT_4_LENGTH 12
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 12
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 14
#define TEXT_9_LENGTH 14
#define TEXT_10_LENGTH 12
#define TEXT_11_LENGTH 12
#define TEXT_12_LENGTH 12
#define TEXT_13_LENGTH 12
#define TEXT_14_LENGTH 9
#define TEXT_15_LENGTH 11
#define TEXT_16_LENGTH 9
#define TEXT_17_LENGTH 9
#define TEXT_18_LENGTH 9
#define TEXT_19_LENGTH 9
#define TEXT_20_LENGTH 9
#define TEXT_21_LENGTH 9
#define TEXT_22_LENGTH 9
#define TEXT_23_LENGTH 9
#define TEXT_24_LENGTH 9
#define TEXT_25_LENGTH 9
#define TEXT_26_LENGTH 9
#define TEXT_27_LENGTH 9
#define TEXT_28_LENGTH 9
#define TEXT_29_LENGTH 9
#define TEXT_30_LENGTH 9
#define TEXT_31_LENGTH 9
#define TEXT_32_LENGTH 9
#define TEXT_33_LENGTH 11
#define TEXT_34_LENGTH 11
#define TEXT_35_LENGTH 11
#define TEXT_36_LENGTH 11
#define TEXT_37_LENGTH 11
#define TEXT_38_LENGTH 11
#define TEXT_39_LENGTH 11
#define TEXT_40_LENGTH 11
#define TEXT_41_LENGTH 11
#define TEXT_42_LENGTH 11
#define TEXT_43_LENGTH 11
#define TEXT_44_LENGTH 11
#define TEXT_45_LENGTH 11
#define TEXT_46_LENGTH 11
#define TEXT_47_LENGTH 11
#define TEXT_48_LENGTH 11
#define TEXT_49_LENGTH 9
#define TEXT_50_LENGTH 9
#define TEXT_51_LENGTH 9
#define TEXT_52_LENGTH 9
#define TEXT_53_LENGTH 9
#define TEXT_54_LENGTH 9
#define TEXT_55_LENGTH 9
#define TEXT_56_LENGTH 9
#define TEXT_57_LENGTH 9
#define TEXT_58_LENGTH 9
#define TEXT_59_LENGTH 9
#define TEXT_60_LENGTH 9
#define TEXT_61_LENGTH 9
#define TEXT_62_LENGTH 9
#define TEXT_63_LENGTH 9
#define TEXT_64_LENGTH 9
#define TEXT_65_LENGTH 9
#define TEXT_66_LENGTH 9
#define TEXT_67_LENGTH 9
#define TEXT_68_LENGTH 9
#define TEXT_69_LENGTH 9
#define TEXT_70_LENGTH 9
#define TEXT_71_LENGTH 9
#define TEXT_72_LENGTH 9
#define TEXT_73_LENGTH 9
#include "b_chaos7/super/text.inc"

extern NJS_MODEL sea_floor_model;

/* how many textures the SUPERSONIC pvm gives super_sonic_texlist */
#define SUPERSONIC_TEXTURES 73

/* by the player's speed, the aura draws aura_slow_object up to AURA_QUICK,
 * aura_object past it and aura_fast_object past AURA_FAST. SE_AURA starts past
 * AURA_QUICK, and SE_AURA_FAST takes its place past AURA_FAST. either stops
 * once the speed falls below AURA_QUIET */
#define AURA_QUICK 7.0F
#define AURA_FAST 13.0F
#define AURA_QUIET 6.0F
#define SE_AURA 0x40b
#define SE_AURA_FAST 0x40c

/* on the sea floor, the wake throws two drops of spray each step past
 * SPRAY_SPEED. the wide wake grows past WAKE_WIDE_SPEED, and the widest past
 * WAKE_WIDEST_SPEED. each of those also throws two drops on some steps, by
 * chance */
#define SPRAY_SPEED 2.0F
#define WAKE_WIDE_SPEED 6.0F
#define WAKE_WIDEST_SPEED 9.0F
/* the wake's size is WAKE_SCALE times the square root of the player's speed.
 * the wide and widest wakes grow toward 1 and shrink to 0 by WAKE_GROWTH a
 * step */
#define WAKE_SCALE 0.4F
#define WAKE_GROWTH 0.05F

/* how far up from the player a spin ball starts, turned as the player
 * faces */
#define BALL_HEIGHT 5.0F
/* a follow ball draws BALL_STEPS steps after its first, and a shrink ball
 * SHRINK_STEPS. each fades out over them */
#define BALL_STEPS 5
#define SHRINK_STEPS 10

/* in a dash's work->flags: it draws super_dash_textures_b, the second set */
#define DASH_SECOND_SET 0x8
/* the steps a dash draws with the first set. with the second, it ends on
 * step SECOND_SET_END, before that step draws */
#define DASH_STEPS 20
#define SECOND_SET_END 19
/* the streak model's length along x */
#define STREAK_LENGTH 8.0F

struct aura_vars {
    Uint8 player; /* the player's number, which sonic.c sets */
};

struct ball_vars {
    Uint8 player; /* the player's number, which sonic.c sets */
    Uint8 unk_01[7];
    NJS_TEXLIST *texlist; /* ss_ball_a's texlist or ss_ball_b's, at random */
};

struct dash_vars {
    Uint8 player; /* the player's number, which sonic.c sets */
    Uint8 unk_01[7];
    NJS_OBJECT *object; /* the dash's own copy of the streak */
};

#define AURA(work) ((struct aura_vars *)&(work)->vars)
#define BALL(work) ((struct ball_vars *)&(work)->vars)
#define DASH(work) ((struct dash_vars *)&(work)->vars)

/* in an aura's work->scl: the wake's size, then the wide and widest wakes'
 * growth, from 0 to 1. nothing draws the widest. the aura's work->tick counts
 * its steps, and picks its animations' textures */
#define WAKE(work) ((work)->scl.x)
#define WAKE_WIDE(work) ((work)->scl.y)
#define WAKE_WIDEST(work) ((work)->scl.z)

enum aura_sound { AURA_SILENT, AURA_RUNNING, AURA_RUNNING_FAST };

static void super_aura_destroy(struct task *task);
static void super_aura_draw(struct task *task);
static void super_aura_step(struct task *task);
static void super_ball_follow_place(struct task_work *work);
static void super_ball_shrink_place(struct task_work *work);
static void super_ball_follow_draw(struct task *task);
static void super_ball_follow_step(struct task *task);
static void super_ball_shrink_draw(struct task *task);
static void super_ball_shrink_step(struct task *task);
static Bool super_dash_alloc(struct task_work *work);
static void super_dash_draw(struct task *task);
static void super_dash_step(struct task *task);
static void super_dash_destroy(struct task *task);

#pragma section HEAD
static NJS_TEXNAME super_sonic_textures[SUPERSONIC_TEXTURES] = {
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
    {TEXT_45, 0, 0}, {TEXT_46, 0, 0}, {TEXT_47, 0, 0}, {TEXT_48, 0, 0},
    {TEXT_49, 0, 0}, {TEXT_50, 0, 0}, {TEXT_51, 0, 0}, {TEXT_52, 0, 0},
    {TEXT_53, 0, 0}, {TEXT_54, 0, 0}, {TEXT_55, 0, 0}, {TEXT_56, 0, 0},
    {TEXT_57, 0, 0}, {TEXT_58, 0, 0}, {TEXT_59, 0, 0}, {TEXT_60, 0, 0},
    {TEXT_61, 0, 0}, {TEXT_62, 0, 0}, {TEXT_63, 0, 0}, {TEXT_64, 0, 0},
    {TEXT_65, 0, 0}, {TEXT_66, 0, 0}, {TEXT_67, 0, 0}, {TEXT_68, 0, 0},
    {TEXT_69, 0, 0}, {TEXT_70, 0, 0}, {TEXT_71, 0, 0}, {TEXT_72, 0, 0},
    {TEXT_73, 0, 0},
};
/* nothing reads these three */
static NJS_TEXLIST super_aura_texlist = {
#include "b_chaos7/super/super_aura_texlist.inc"
};
static NJS_TEXLIST super_aura_fast_texlist = {
#include "b_chaos7/super/super_aura_fast_texlist.inc"
};
static NJS_TEXLIST super_wake_texlist = {
#include "b_chaos7/super/super_wake_texlist.inc"
};
NJS_TEXLIST super_sonic_texlist = {
#include "b_chaos7/super/super_sonic_texlist.inc"
};
/* Super Sonic's models, then the aura's and the wake's. sonic.c draws his,
 * and this file draws the others. the aura's and the wake's texture names
 * point at super_sonic_textures' strings, which SHC shares only within one
 * file */
static NJS_OBJECT dat_0c901f24 = {
#include "b_chaos7/super/dat_0c901f24.inc"
};
static NJS_OBJECT dat_0c901f58 = {
#include "b_chaos7/super/dat_0c901f58.inc"
};
static NJS_OBJECT dat_0c901f8c = {
#include "b_chaos7/super/dat_0c901f8c.inc"
};
static NJS_MATERIAL dat_0c901fc0[1] = {
#include "b_chaos7/super/dat_0c901fc0.inc"
};
static Sint16 dat_0c901fd4[15] = {
#include "b_chaos7/super/dat_0c901fd4.inc"
};
static NJS_COLOR dat_0c901ff4[13] = {
#include "b_chaos7/super/dat_0c901ff4.inc"
};
static NJS_MESHSET dat_0c902028[1] = {
#include "b_chaos7/super/dat_0c902028.inc"
};
static NJS_POINT3 dat_0c902040[7] = {
#include "b_chaos7/super/dat_0c902040.inc"
};
static NJS_VECTOR dat_0c902094[7] = {
#include "b_chaos7/super/dat_0c902094.inc"
};
static NJS_MODEL dat_0c9020e8 = {
#include "b_chaos7/super/dat_0c9020e8.inc"
};
static NJS_OBJECT dat_0c902110 = {
#include "b_chaos7/super/dat_0c902110.inc"
};
static NJS_OBJECT dat_0c902144 = {
#include "b_chaos7/super/dat_0c902144.inc"
};
static NJS_OBJECT dat_0c902178 = {
#include "b_chaos7/super/dat_0c902178.inc"
};
static NJS_OBJECT dat_0c9021ac = {
#include "b_chaos7/super/dat_0c9021ac.inc"
};
static NJS_MATERIAL dat_0c9021e0[1] = {
#include "b_chaos7/super/dat_0c9021e0.inc"
};
static Sint16 dat_0c9021f4[11] = {
#include "b_chaos7/super/dat_0c9021f4.inc"
};
static NJS_COLOR dat_0c90220c[10] = {
#include "b_chaos7/super/dat_0c90220c.inc"
};
static NJS_MESHSET dat_0c902234[1] = {
#include "b_chaos7/super/dat_0c902234.inc"
};
static NJS_POINT3 dat_0c90224c[8] = {
#include "b_chaos7/super/dat_0c90224c.inc"
};
static NJS_VECTOR dat_0c9022ac[8] = {
#include "b_chaos7/super/dat_0c9022ac.inc"
};
static NJS_MODEL dat_0c90230c = {
#include "b_chaos7/super/dat_0c90230c.inc"
};
NJS_OBJECT dat_0c902334 = {
#include "b_chaos7/super/dat_0c902334.inc"
};
static NJS_MATERIAL dat_0c902368[1] = {
#include "b_chaos7/super/dat_0c902368.inc"
};
static Sint16 dat_0c90237c[11] = {
#include "b_chaos7/super/dat_0c90237c.inc"
};
static NJS_COLOR dat_0c902394[10] = {
#include "b_chaos7/super/dat_0c902394.inc"
};
static NJS_MESHSET dat_0c9023bc[1] = {
#include "b_chaos7/super/dat_0c9023bc.inc"
};
static NJS_POINT3 dat_0c9023d4[8] = {
#include "b_chaos7/super/dat_0c9023d4.inc"
};
static NJS_VECTOR dat_0c902434[8] = {
#include "b_chaos7/super/dat_0c902434.inc"
};
static NJS_MODEL dat_0c902494 = {
#include "b_chaos7/super/dat_0c902494.inc"
};
NJS_OBJECT dat_0c9024bc = {
#include "b_chaos7/super/dat_0c9024bc.inc"
};
static NJS_MATERIAL dat_0c9024f0[3] = {
#include "b_chaos7/super/dat_0c9024f0.inc"
};
static Sint16 dat_0c90252c[19] = {
#include "b_chaos7/super/dat_0c90252c.inc"
};
static Sint16 dat_0c902552[25] = {
#include "b_chaos7/super/dat_0c902552.inc"
};
static Sint16 dat_0c902584[10] = {
#include "b_chaos7/super/dat_0c902584.inc"
};
static NJS_COLOR dat_0c902598[16] = {
#include "b_chaos7/super/dat_0c902598.inc"
};
static NJS_COLOR dat_0c9025d8[20] = {
#include "b_chaos7/super/dat_0c9025d8.inc"
};
static NJS_COLOR dat_0c902628[8] = {
#include "b_chaos7/super/dat_0c902628.inc"
};
static NJS_MESHSET dat_0c902648[3] = {
#include "b_chaos7/super/dat_0c902648.inc"
};
static NJS_POINT3 dat_0c902690[16] = {
#include "b_chaos7/super/dat_0c902690.inc"
};
static NJS_VECTOR dat_0c902750[16] = {
#include "b_chaos7/super/dat_0c902750.inc"
};
static NJS_MODEL dat_0c902810 = {
#include "b_chaos7/super/dat_0c902810.inc"
};
NJS_OBJECT dat_0c902838 = {
#include "b_chaos7/super/dat_0c902838.inc"
};
static NJS_MATERIAL dat_0c90286c[5] = {
#include "b_chaos7/super/dat_0c90286c.inc"
};
static Sint16 dat_0c9028d0[25] = {
#include "b_chaos7/super/dat_0c9028d0.inc"
};
static Sint16 dat_0c902902[12] = {
#include "b_chaos7/super/dat_0c902902.inc"
};
static Sint16 dat_0c90291a[17] = {
#include "b_chaos7/super/dat_0c90291a.inc"
};
static Sint16 dat_0c90293c[32] = {
#include "b_chaos7/super/dat_0c90293c.inc"
};
static Sint16 dat_0c90297c[12] = {
#include "b_chaos7/super/dat_0c90297c.inc"
};
static NJS_COLOR dat_0c902994[21] = {
#include "b_chaos7/super/dat_0c902994.inc"
};
static NJS_COLOR dat_0c9029e8[11] = {
#include "b_chaos7/super/dat_0c9029e8.inc"
};
static NJS_COLOR dat_0c902a14[14] = {
#include "b_chaos7/super/dat_0c902a14.inc"
};
static NJS_COLOR dat_0c902a4c[26] = {
#include "b_chaos7/super/dat_0c902a4c.inc"
};
static NJS_COLOR dat_0c902ab4[10] = {
#include "b_chaos7/super/dat_0c902ab4.inc"
};
static NJS_MESHSET dat_0c902adc[5] = {
#include "b_chaos7/super/dat_0c902adc.inc"
};
static NJS_POINT3 dat_0c902b54[29] = {
#include "b_chaos7/super/dat_0c902b54.inc"
};
static NJS_VECTOR dat_0c902cb0[29] = {
#include "b_chaos7/super/dat_0c902cb0.inc"
};
static NJS_MODEL dat_0c902e0c = {
#include "b_chaos7/super/dat_0c902e0c.inc"
};
NJS_OBJECT dat_0c902e34 = {
#include "b_chaos7/super/dat_0c902e34.inc"
};
static NJS_MATERIAL dat_0c902e68[3] = {
#include "b_chaos7/super/dat_0c902e68.inc"
};
static Sint16 dat_0c902ea4[11] = {
#include "b_chaos7/super/dat_0c902ea4.inc"
};
static Sint16 dat_0c902eba[41] = {
#include "b_chaos7/super/dat_0c902eba.inc"
};
static Sint16 dat_0c902f0c[6] = {
#include "b_chaos7/super/dat_0c902f0c.inc"
};
static NJS_COLOR dat_0c902f18[10] = {
#include "b_chaos7/super/dat_0c902f18.inc"
};
static NJS_COLOR dat_0c902f40[35] = {
#include "b_chaos7/super/dat_0c902f40.inc"
};
static NJS_COLOR dat_0c902fcc[5] = {
#include "b_chaos7/super/dat_0c902fcc.inc"
};
static NJS_MESHSET dat_0c902fe0[3] = {
#include "b_chaos7/super/dat_0c902fe0.inc"
};
static NJS_POINT3 dat_0c903028[23] = {
#include "b_chaos7/super/dat_0c903028.inc"
};
static NJS_VECTOR dat_0c90313c[23] = {
#include "b_chaos7/super/dat_0c90313c.inc"
};
static NJS_MODEL dat_0c903250 = {
#include "b_chaos7/super/dat_0c903250.inc"
};
NJS_OBJECT dat_0c903278 = {
#include "b_chaos7/super/dat_0c903278.inc"
};
static NJS_OBJECT dat_0c9032ac = {
#include "b_chaos7/super/dat_0c9032ac.inc"
};
static NJS_OBJECT dat_0c9032e0 = {
#include "b_chaos7/super/dat_0c9032e0.inc"
};
static NJS_OBJECT dat_0c903314 = {
#include "b_chaos7/super/dat_0c903314.inc"
};
static NJS_OBJECT dat_0c903348 = {
#include "b_chaos7/super/dat_0c903348.inc"
};
static NJS_OBJECT dat_0c90337c = {
#include "b_chaos7/super/dat_0c90337c.inc"
};
static NJS_MATERIAL dat_0c9033b0[1] = {
#include "b_chaos7/super/dat_0c9033b0.inc"
};
static Sint16 dat_0c9033c4[11] = {
#include "b_chaos7/super/dat_0c9033c4.inc"
};
static NJS_COLOR dat_0c9033dc[10] = {
#include "b_chaos7/super/dat_0c9033dc.inc"
};
static NJS_MESHSET dat_0c903404[1] = {
#include "b_chaos7/super/dat_0c903404.inc"
};
static NJS_POINT3 dat_0c90341c[8] = {
#include "b_chaos7/super/dat_0c90341c.inc"
};
static NJS_VECTOR dat_0c90347c[8] = {
#include "b_chaos7/super/dat_0c90347c.inc"
};
static NJS_MODEL dat_0c9034dc = {
#include "b_chaos7/super/dat_0c9034dc.inc"
};
NJS_OBJECT dat_0c903504 = {
#include "b_chaos7/super/dat_0c903504.inc"
};
static NJS_MATERIAL dat_0c903538[1] = {
#include "b_chaos7/super/dat_0c903538.inc"
};
static Sint16 dat_0c90354c[11] = {
#include "b_chaos7/super/dat_0c90354c.inc"
};
static NJS_COLOR dat_0c903564[10] = {
#include "b_chaos7/super/dat_0c903564.inc"
};
static NJS_MESHSET dat_0c90358c[1] = {
#include "b_chaos7/super/dat_0c90358c.inc"
};
static NJS_POINT3 dat_0c9035a4[8] = {
#include "b_chaos7/super/dat_0c9035a4.inc"
};
static NJS_VECTOR dat_0c903604[8] = {
#include "b_chaos7/super/dat_0c903604.inc"
};
static NJS_MODEL dat_0c903664 = {
#include "b_chaos7/super/dat_0c903664.inc"
};
NJS_OBJECT dat_0c90368c = {
#include "b_chaos7/super/dat_0c90368c.inc"
};
static NJS_MATERIAL dat_0c9036c0[3] = {
#include "b_chaos7/super/dat_0c9036c0.inc"
};
static Sint16 dat_0c9036fc[19] = {
#include "b_chaos7/super/dat_0c9036fc.inc"
};
static Sint16 dat_0c903722[22] = {
#include "b_chaos7/super/dat_0c903722.inc"
};
static Sint16 dat_0c90374e[10] = {
#include "b_chaos7/super/dat_0c90374e.inc"
};
static NJS_COLOR dat_0c903764[16] = {
#include "b_chaos7/super/dat_0c903764.inc"
};
static NJS_COLOR dat_0c9037a4[18] = {
#include "b_chaos7/super/dat_0c9037a4.inc"
};
static NJS_COLOR dat_0c9037ec[8] = {
#include "b_chaos7/super/dat_0c9037ec.inc"
};
static NJS_MESHSET dat_0c90380c[3] = {
#include "b_chaos7/super/dat_0c90380c.inc"
};
static NJS_POINT3 dat_0c903854[16] = {
#include "b_chaos7/super/dat_0c903854.inc"
};
static NJS_VECTOR dat_0c903914[16] = {
#include "b_chaos7/super/dat_0c903914.inc"
};
static NJS_MODEL dat_0c9039d4 = {
#include "b_chaos7/super/dat_0c9039d4.inc"
};
NJS_OBJECT dat_0c9039fc = {
#include "b_chaos7/super/dat_0c9039fc.inc"
};
static NJS_MATERIAL dat_0c903a30[5] = {
#include "b_chaos7/super/dat_0c903a30.inc"
};
static Sint16 dat_0c903a94[25] = {
#include "b_chaos7/super/dat_0c903a94.inc"
};
static Sint16 dat_0c903ac6[12] = {
#include "b_chaos7/super/dat_0c903ac6.inc"
};
static Sint16 dat_0c903ade[14] = {
#include "b_chaos7/super/dat_0c903ade.inc"
};
static Sint16 dat_0c903afa[32] = {
#include "b_chaos7/super/dat_0c903afa.inc"
};
static Sint16 dat_0c903b3a[12] = {
#include "b_chaos7/super/dat_0c903b3a.inc"
};
static NJS_COLOR dat_0c903b54[21] = {
#include "b_chaos7/super/dat_0c903b54.inc"
};
static NJS_COLOR dat_0c903ba8[11] = {
#include "b_chaos7/super/dat_0c903ba8.inc"
};
static NJS_COLOR dat_0c903bd4[12] = {
#include "b_chaos7/super/dat_0c903bd4.inc"
};
static NJS_COLOR dat_0c903c04[26] = {
#include "b_chaos7/super/dat_0c903c04.inc"
};
static NJS_COLOR dat_0c903c6c[10] = {
#include "b_chaos7/super/dat_0c903c6c.inc"
};
static NJS_MESHSET dat_0c903c94[5] = {
#include "b_chaos7/super/dat_0c903c94.inc"
};
static NJS_POINT3 dat_0c903d0c[29] = {
#include "b_chaos7/super/dat_0c903d0c.inc"
};
static NJS_VECTOR dat_0c903e68[29] = {
#include "b_chaos7/super/dat_0c903e68.inc"
};
static NJS_MODEL dat_0c903fc4 = {
#include "b_chaos7/super/dat_0c903fc4.inc"
};
NJS_OBJECT dat_0c903fec = {
#include "b_chaos7/super/dat_0c903fec.inc"
};
static NJS_MATERIAL dat_0c904020[3] = {
#include "b_chaos7/super/dat_0c904020.inc"
};
static Sint16 dat_0c90405c[11] = {
#include "b_chaos7/super/dat_0c90405c.inc"
};
static Sint16 dat_0c904072[41] = {
#include "b_chaos7/super/dat_0c904072.inc"
};
static Sint16 dat_0c9040c4[6] = {
#include "b_chaos7/super/dat_0c9040c4.inc"
};
static NJS_COLOR dat_0c9040d0[10] = {
#include "b_chaos7/super/dat_0c9040d0.inc"
};
static NJS_COLOR dat_0c9040f8[35] = {
#include "b_chaos7/super/dat_0c9040f8.inc"
};
static NJS_COLOR dat_0c904184[5] = {
#include "b_chaos7/super/dat_0c904184.inc"
};
static NJS_MESHSET dat_0c904198[3] = {
#include "b_chaos7/super/dat_0c904198.inc"
};
static NJS_POINT3 dat_0c9041e0[23] = {
#include "b_chaos7/super/dat_0c9041e0.inc"
};
static NJS_VECTOR dat_0c9042f4[23] = {
#include "b_chaos7/super/dat_0c9042f4.inc"
};
static NJS_MODEL dat_0c904408 = {
#include "b_chaos7/super/dat_0c904408.inc"
};
NJS_OBJECT dat_0c904430 = {
#include "b_chaos7/super/dat_0c904430.inc"
};
static NJS_OBJECT dat_0c904464 = {
#include "b_chaos7/super/dat_0c904464.inc"
};
static NJS_OBJECT dat_0c904498 = {
#include "b_chaos7/super/dat_0c904498.inc"
};
static NJS_OBJECT dat_0c9044cc = {
#include "b_chaos7/super/dat_0c9044cc.inc"
};
static NJS_OBJECT dat_0c904500 = {
#include "b_chaos7/super/dat_0c904500.inc"
};
static NJS_OBJECT dat_0c904534 = {
#include "b_chaos7/super/dat_0c904534.inc"
};
static NJS_MATERIAL dat_0c904568[1] = {
#include "b_chaos7/super/dat_0c904568.inc"
};
static Sint16 dat_0c90457c[19] = {
#include "b_chaos7/super/dat_0c90457c.inc"
};
static NJS_COLOR dat_0c9045a4[16] = {
#include "b_chaos7/super/dat_0c9045a4.inc"
};
static NJS_MESHSET dat_0c9045e4[1] = {
#include "b_chaos7/super/dat_0c9045e4.inc"
};
static NJS_POINT3 dat_0c9045fc[8] = {
#include "b_chaos7/super/dat_0c9045fc.inc"
};
static NJS_VECTOR dat_0c90465c[8] = {
#include "b_chaos7/super/dat_0c90465c.inc"
};
static NJS_MODEL dat_0c9046bc = {
#include "b_chaos7/super/dat_0c9046bc.inc"
};
NJS_OBJECT dat_0c9046e4 = {
#include "b_chaos7/super/dat_0c9046e4.inc"
};
static NJS_MATERIAL dat_0c904718[1] = {
#include "b_chaos7/super/dat_0c904718.inc"
};
static Sint16 dat_0c90472c[11] = {
#include "b_chaos7/super/dat_0c90472c.inc"
};
static NJS_COLOR dat_0c904744[10] = {
#include "b_chaos7/super/dat_0c904744.inc"
};
static NJS_MESHSET dat_0c90476c[1] = {
#include "b_chaos7/super/dat_0c90476c.inc"
};
static NJS_POINT3 dat_0c904784[8] = {
#include "b_chaos7/super/dat_0c904784.inc"
};
static NJS_VECTOR dat_0c9047e4[8] = {
#include "b_chaos7/super/dat_0c9047e4.inc"
};
static NJS_MODEL dat_0c904844 = {
#include "b_chaos7/super/dat_0c904844.inc"
};
NJS_OBJECT dat_0c90486c = {
#include "b_chaos7/super/dat_0c90486c.inc"
};
static NJS_MATERIAL dat_0c9048a0[2] = {
#include "b_chaos7/super/dat_0c9048a0.inc"
};
static Sint16 dat_0c9048c8[39] = {
#include "b_chaos7/super/dat_0c9048c8.inc"
};
static Sint16 dat_0c904916[11] = {
#include "b_chaos7/super/dat_0c904916.inc"
};
static NJS_COLOR dat_0c90492c[32] = {
#include "b_chaos7/super/dat_0c90492c.inc"
};
static NJS_COLOR dat_0c9049ac[10] = {
#include "b_chaos7/super/dat_0c9049ac.inc"
};
static NJS_MESHSET dat_0c9049d4[2] = {
#include "b_chaos7/super/dat_0c9049d4.inc"
};
static NJS_POINT3 dat_0c904a04[16] = {
#include "b_chaos7/super/dat_0c904a04.inc"
};
static NJS_VECTOR dat_0c904ac4[16] = {
#include "b_chaos7/super/dat_0c904ac4.inc"
};
static NJS_MODEL dat_0c904b84 = {
#include "b_chaos7/super/dat_0c904b84.inc"
};
NJS_OBJECT dat_0c904bac = {
#include "b_chaos7/super/dat_0c904bac.inc"
};
static NJS_MATERIAL dat_0c904be0[1] = {
#include "b_chaos7/super/dat_0c904be0.inc"
};
static Sint16 dat_0c904bf4[35] = {
#include "b_chaos7/super/dat_0c904bf4.inc"
};
static NJS_COLOR dat_0c904c3c[30] = {
#include "b_chaos7/super/dat_0c904c3c.inc"
};
static NJS_MESHSET dat_0c904cb4[1] = {
#include "b_chaos7/super/dat_0c904cb4.inc"
};
static NJS_POINT3 dat_0c904ccc[12] = {
#include "b_chaos7/super/dat_0c904ccc.inc"
};
static NJS_VECTOR dat_0c904d5c[12] = {
#include "b_chaos7/super/dat_0c904d5c.inc"
};
static NJS_MODEL dat_0c904dec = {
#include "b_chaos7/super/dat_0c904dec.inc"
};
static NJS_OBJECT dat_0c904e14 = {
#include "b_chaos7/super/dat_0c904e14.inc"
};
static NJS_MATERIAL dat_0c904e48[2] = {
#include "b_chaos7/super/dat_0c904e48.inc"
};
static Sint16 dat_0c904e70[19] = {
#include "b_chaos7/super/dat_0c904e70.inc"
};
static Sint16 dat_0c904e96[5] = {
#include "b_chaos7/super/dat_0c904e96.inc"
};
static NJS_COLOR dat_0c904ea0[16] = {
#include "b_chaos7/super/dat_0c904ea0.inc"
};
static NJS_COLOR dat_0c904ee0[4] = {
#include "b_chaos7/super/dat_0c904ee0.inc"
};
static NJS_MESHSET dat_0c904ef0[2] = {
#include "b_chaos7/super/dat_0c904ef0.inc"
};
static NJS_POINT3 dat_0c904f20[9] = {
#include "b_chaos7/super/dat_0c904f20.inc"
};
static NJS_VECTOR dat_0c904f8c[9] = {
#include "b_chaos7/super/dat_0c904f8c.inc"
};
static NJS_MODEL dat_0c904ff8 = {
#include "b_chaos7/super/dat_0c904ff8.inc"
};
NJS_OBJECT dat_0c905020 = {
#include "b_chaos7/super/dat_0c905020.inc"
};
static NJS_MATERIAL dat_0c905054[3] = {
#include "b_chaos7/super/dat_0c905054.inc"
};
static Sint16 dat_0c905090[41] = {
#include "b_chaos7/super/dat_0c905090.inc"
};
static Sint16 dat_0c9050e2[6] = {
#include "b_chaos7/super/dat_0c9050e2.inc"
};
static Sint16 dat_0c9050ee[11] = {
#include "b_chaos7/super/dat_0c9050ee.inc"
};
static NJS_COLOR dat_0c905104[35] = {
#include "b_chaos7/super/dat_0c905104.inc"
};
static NJS_COLOR dat_0c905190[5] = {
#include "b_chaos7/super/dat_0c905190.inc"
};
static NJS_COLOR dat_0c9051a4[10] = {
#include "b_chaos7/super/dat_0c9051a4.inc"
};
static NJS_MESHSET dat_0c9051cc[3] = {
#include "b_chaos7/super/dat_0c9051cc.inc"
};
static NJS_POINT3 dat_0c905214[23] = {
#include "b_chaos7/super/dat_0c905214.inc"
};
static NJS_VECTOR dat_0c905328[23] = {
#include "b_chaos7/super/dat_0c905328.inc"
};
static NJS_MODEL dat_0c90543c = {
#include "b_chaos7/super/dat_0c90543c.inc"
};
NJS_OBJECT dat_0c905464 = {
#include "b_chaos7/super/dat_0c905464.inc"
};
static NJS_OBJECT dat_0c905498 = {
#include "b_chaos7/super/dat_0c905498.inc"
};
static NJS_OBJECT dat_0c9054cc = {
#include "b_chaos7/super/dat_0c9054cc.inc"
};
static NJS_OBJECT dat_0c905500 = {
#include "b_chaos7/super/dat_0c905500.inc"
};
static NJS_OBJECT dat_0c905534 = {
#include "b_chaos7/super/dat_0c905534.inc"
};
static NJS_OBJECT dat_0c905568 = {
#include "b_chaos7/super/dat_0c905568.inc"
};
static NJS_MATERIAL dat_0c90559c[1] = {
#include "b_chaos7/super/dat_0c90559c.inc"
};
static Sint16 dat_0c9055b0[16] = {
#include "b_chaos7/super/dat_0c9055b0.inc"
};
static NJS_COLOR dat_0c9055d0[14] = {
#include "b_chaos7/super/dat_0c9055d0.inc"
};
static NJS_MESHSET dat_0c905608[1] = {
#include "b_chaos7/super/dat_0c905608.inc"
};
static NJS_POINT3 dat_0c905620[8] = {
#include "b_chaos7/super/dat_0c905620.inc"
};
static NJS_VECTOR dat_0c905680[8] = {
#include "b_chaos7/super/dat_0c905680.inc"
};
static NJS_MODEL dat_0c9056e0 = {
#include "b_chaos7/super/dat_0c9056e0.inc"
};
NJS_OBJECT dat_0c905708 = {
#include "b_chaos7/super/dat_0c905708.inc"
};
static NJS_MATERIAL dat_0c90573c[1] = {
#include "b_chaos7/super/dat_0c90573c.inc"
};
static Sint16 dat_0c905750[11] = {
#include "b_chaos7/super/dat_0c905750.inc"
};
static NJS_COLOR dat_0c905768[10] = {
#include "b_chaos7/super/dat_0c905768.inc"
};
static NJS_MESHSET dat_0c905790[1] = {
#include "b_chaos7/super/dat_0c905790.inc"
};
static NJS_POINT3 dat_0c9057a8[8] = {
#include "b_chaos7/super/dat_0c9057a8.inc"
};
static NJS_VECTOR dat_0c905808[8] = {
#include "b_chaos7/super/dat_0c905808.inc"
};
static NJS_MODEL dat_0c905868 = {
#include "b_chaos7/super/dat_0c905868.inc"
};
NJS_OBJECT dat_0c905890 = {
#include "b_chaos7/super/dat_0c905890.inc"
};
static NJS_MATERIAL dat_0c9058c4[2] = {
#include "b_chaos7/super/dat_0c9058c4.inc"
};
static Sint16 dat_0c9058ec[39] = {
#include "b_chaos7/super/dat_0c9058ec.inc"
};
static Sint16 dat_0c90593a[11] = {
#include "b_chaos7/super/dat_0c90593a.inc"
};
static NJS_COLOR dat_0c905950[32] = {
#include "b_chaos7/super/dat_0c905950.inc"
};
static NJS_COLOR dat_0c9059d0[10] = {
#include "b_chaos7/super/dat_0c9059d0.inc"
};
static NJS_MESHSET dat_0c9059f8[2] = {
#include "b_chaos7/super/dat_0c9059f8.inc"
};
static NJS_POINT3 dat_0c905a28[16] = {
#include "b_chaos7/super/dat_0c905a28.inc"
};
static NJS_VECTOR dat_0c905ae8[16] = {
#include "b_chaos7/super/dat_0c905ae8.inc"
};
static NJS_MODEL dat_0c905ba8 = {
#include "b_chaos7/super/dat_0c905ba8.inc"
};
NJS_OBJECT dat_0c905bd0 = {
#include "b_chaos7/super/dat_0c905bd0.inc"
};
static NJS_MATERIAL dat_0c905c04[1] = {
#include "b_chaos7/super/dat_0c905c04.inc"
};
static Sint16 dat_0c905c18[35] = {
#include "b_chaos7/super/dat_0c905c18.inc"
};
static NJS_COLOR dat_0c905c60[30] = {
#include "b_chaos7/super/dat_0c905c60.inc"
};
static NJS_MESHSET dat_0c905cd8[1] = {
#include "b_chaos7/super/dat_0c905cd8.inc"
};
static NJS_POINT3 dat_0c905cf0[12] = {
#include "b_chaos7/super/dat_0c905cf0.inc"
};
static NJS_VECTOR dat_0c905d80[12] = {
#include "b_chaos7/super/dat_0c905d80.inc"
};
static NJS_MODEL dat_0c905e10 = {
#include "b_chaos7/super/dat_0c905e10.inc"
};
static NJS_OBJECT dat_0c905e38 = {
#include "b_chaos7/super/dat_0c905e38.inc"
};
static NJS_MATERIAL dat_0c905e6c[2] = {
#include "b_chaos7/super/dat_0c905e6c.inc"
};
static Sint16 dat_0c905e94[19] = {
#include "b_chaos7/super/dat_0c905e94.inc"
};
static Sint16 dat_0c905eba[5] = {
#include "b_chaos7/super/dat_0c905eba.inc"
};
static NJS_COLOR dat_0c905ec4[16] = {
#include "b_chaos7/super/dat_0c905ec4.inc"
};
static NJS_COLOR dat_0c905f04[4] = {
#include "b_chaos7/super/dat_0c905f04.inc"
};
static NJS_MESHSET dat_0c905f14[2] = {
#include "b_chaos7/super/dat_0c905f14.inc"
};
static NJS_POINT3 dat_0c905f44[9] = {
#include "b_chaos7/super/dat_0c905f44.inc"
};
static NJS_VECTOR dat_0c905fb0[9] = {
#include "b_chaos7/super/dat_0c905fb0.inc"
};
static NJS_MODEL dat_0c90601c = {
#include "b_chaos7/super/dat_0c90601c.inc"
};
NJS_OBJECT dat_0c906044 = {
#include "b_chaos7/super/dat_0c906044.inc"
};
static NJS_MATERIAL dat_0c906078[3] = {
#include "b_chaos7/super/dat_0c906078.inc"
};
static Sint16 dat_0c9060b4[38] = {
#include "b_chaos7/super/dat_0c9060b4.inc"
};
static Sint16 dat_0c906100[6] = {
#include "b_chaos7/super/dat_0c906100.inc"
};
static Sint16 dat_0c90610c[11] = {
#include "b_chaos7/super/dat_0c90610c.inc"
};
static NJS_COLOR dat_0c906124[33] = {
#include "b_chaos7/super/dat_0c906124.inc"
};
static NJS_COLOR dat_0c9061a8[5] = {
#include "b_chaos7/super/dat_0c9061a8.inc"
};
static NJS_COLOR dat_0c9061bc[10] = {
#include "b_chaos7/super/dat_0c9061bc.inc"
};
static NJS_MESHSET dat_0c9061e4[3] = {
#include "b_chaos7/super/dat_0c9061e4.inc"
};
static NJS_POINT3 dat_0c90622c[23] = {
#include "b_chaos7/super/dat_0c90622c.inc"
};
static NJS_VECTOR dat_0c906340[23] = {
#include "b_chaos7/super/dat_0c906340.inc"
};
static NJS_MODEL dat_0c906454 = {
#include "b_chaos7/super/dat_0c906454.inc"
};
NJS_OBJECT dat_0c90647c = {
#include "b_chaos7/super/dat_0c90647c.inc"
};
static NJS_OBJECT dat_0c9064b0 = {
#include "b_chaos7/super/dat_0c9064b0.inc"
};
static NJS_OBJECT dat_0c9064e4 = {
#include "b_chaos7/super/dat_0c9064e4.inc"
};
static NJS_OBJECT dat_0c906518 = {
#include "b_chaos7/super/dat_0c906518.inc"
};
static NJS_OBJECT dat_0c90654c = {
#include "b_chaos7/super/dat_0c90654c.inc"
};
static NJS_OBJECT dat_0c906580 = {
#include "b_chaos7/super/dat_0c906580.inc"
};
static NJS_MATERIAL dat_0c9065b4[1] = {
#include "b_chaos7/super/dat_0c9065b4.inc"
};
static Sint16 dat_0c9065c8[123] = {
#include "b_chaos7/super/dat_0c9065c8.inc"
};
static NJS_COLOR dat_0c9066c0[110] = {
#include "b_chaos7/super/dat_0c9066c0.inc"
};
static NJS_MESHSET dat_0c906878[1] = {
#include "b_chaos7/super/dat_0c906878.inc"
};
static NJS_POINT3 dat_0c906890[56] = {
#include "b_chaos7/super/dat_0c906890.inc"
};
static NJS_VECTOR dat_0c906b30[56] = {
#include "b_chaos7/super/dat_0c906b30.inc"
};
static NJS_MODEL dat_0c906dd0 = {
#include "b_chaos7/super/dat_0c906dd0.inc"
};
static NJS_OBJECT dat_0c906df8 = {
#include "b_chaos7/super/dat_0c906df8.inc"
};
static NJS_MATERIAL dat_0c906e2c[4] = {
#include "b_chaos7/super/dat_0c906e2c.inc"
};
static Sint16 dat_0c906e7c[396] = {
#include "b_chaos7/super/dat_0c906e7c.inc"
};
static Sint16 dat_0c907194[122] = {
#include "b_chaos7/super/dat_0c907194.inc"
};
static Sint16 dat_0c907288[73] = {
#include "b_chaos7/super/dat_0c907288.inc"
};
static Sint16 dat_0c90731a[25] = {
#include "b_chaos7/super/dat_0c90731a.inc"
};
static NJS_COLOR dat_0c90734c[352] = {
#include "b_chaos7/super/dat_0c90734c.inc"
};
static NJS_COLOR dat_0c9078cc[108] = {
#include "b_chaos7/super/dat_0c9078cc.inc"
};
static NJS_COLOR dat_0c907a7c[55] = {
#include "b_chaos7/super/dat_0c907a7c.inc"
};
static NJS_COLOR dat_0c907b58[22] = {
#include "b_chaos7/super/dat_0c907b58.inc"
};
static NJS_MESHSET dat_0c907bb0[4] = {
#include "b_chaos7/super/dat_0c907bb0.inc"
};
static NJS_POINT3 dat_0c907c10[194] = {
#include "b_chaos7/super/dat_0c907c10.inc"
};
static NJS_VECTOR dat_0c908528[194] = {
#include "b_chaos7/super/dat_0c908528.inc"
};
static NJS_MODEL dat_0c908e40 = {
#include "b_chaos7/super/dat_0c908e40.inc"
};
static NJS_OBJECT dat_0c908e68 = {
#include "b_chaos7/super/dat_0c908e68.inc"
};
static NJS_MATERIAL dat_0c908e9c[1] = {
#include "b_chaos7/super/dat_0c908e9c.inc"
};
static Sint16 dat_0c908eb0[18] = {
#include "b_chaos7/super/dat_0c908eb0.inc"
};
static NJS_COLOR dat_0c908ed4[16] = {
#include "b_chaos7/super/dat_0c908ed4.inc"
};
static NJS_MESHSET dat_0c908f14[1] = {
#include "b_chaos7/super/dat_0c908f14.inc"
};
static NJS_POINT3 dat_0c908f2c[11] = {
#include "b_chaos7/super/dat_0c908f2c.inc"
};
static NJS_VECTOR dat_0c908fb0[11] = {
#include "b_chaos7/super/dat_0c908fb0.inc"
};
static NJS_MODEL dat_0c909034 = {
#include "b_chaos7/super/dat_0c909034.inc"
};
static NJS_OBJECT dat_0c90905c = {
#include "b_chaos7/super/dat_0c90905c.inc"
};
static NJS_OBJECT dat_0c909090 = {
#include "b_chaos7/super/dat_0c909090.inc"
};
static NJS_OBJECT dat_0c9090c4 = {
#include "b_chaos7/super/dat_0c9090c4.inc"
};
static NJS_OBJECT dat_0c9090f8 = {
#include "b_chaos7/super/dat_0c9090f8.inc"
};
static NJS_MATERIAL dat_0c90912c[1] = {
#include "b_chaos7/super/dat_0c90912c.inc"
};
static Sint16 dat_0c909140[18] = {
#include "b_chaos7/super/dat_0c909140.inc"
};
static NJS_COLOR dat_0c909164[16] = {
#include "b_chaos7/super/dat_0c909164.inc"
};
static NJS_MESHSET dat_0c9091a4[1] = {
#include "b_chaos7/super/dat_0c9091a4.inc"
};
static NJS_POINT3 dat_0c9091bc[11] = {
#include "b_chaos7/super/dat_0c9091bc.inc"
};
static NJS_VECTOR dat_0c909240[11] = {
#include "b_chaos7/super/dat_0c909240.inc"
};
static NJS_MODEL dat_0c9092c4 = {
#include "b_chaos7/super/dat_0c9092c4.inc"
};
static NJS_OBJECT dat_0c9092ec = {
#include "b_chaos7/super/dat_0c9092ec.inc"
};
static NJS_OBJECT dat_0c909320 = {
#include "b_chaos7/super/dat_0c909320.inc"
};
static NJS_OBJECT dat_0c909354 = {
#include "b_chaos7/super/dat_0c909354.inc"
};
static NJS_OBJECT dat_0c909388 = {
#include "b_chaos7/super/dat_0c909388.inc"
};
static NJS_OBJECT dat_0c9093bc = {
#include "b_chaos7/super/dat_0c9093bc.inc"
};
static NJS_OBJECT dat_0c9093f0 = {
#include "b_chaos7/super/dat_0c9093f0.inc"
};
static NJS_OBJECT dat_0c909424 = {
#include "b_chaos7/super/dat_0c909424.inc"
};
static NJS_MATERIAL dat_0c909458[2] = {
#include "b_chaos7/super/dat_0c909458.inc"
};
static Sint16 dat_0c909480[107] = {
#include "b_chaos7/super/dat_0c909480.inc"
};
static Sint16 dat_0c909556[20] = {
#include "b_chaos7/super/dat_0c909556.inc"
};
static NJS_COLOR dat_0c909580[96] = {
#include "b_chaos7/super/dat_0c909580.inc"
};
static NJS_COLOR dat_0c909700[16] = {
#include "b_chaos7/super/dat_0c909700.inc"
};
static NJS_MESHSET dat_0c909740[2] = {
#include "b_chaos7/super/dat_0c909740.inc"
};
static NJS_POINT3 dat_0c909770[43] = {
#include "b_chaos7/super/dat_0c909770.inc"
};
static NJS_VECTOR dat_0c909974[43] = {
#include "b_chaos7/super/dat_0c909974.inc"
};
static NJS_MODEL dat_0c909b78 = {
#include "b_chaos7/super/dat_0c909b78.inc"
};
static NJS_OBJECT dat_0c909ba0 = {
#include "b_chaos7/super/dat_0c909ba0.inc"
};
static NJS_OBJECT dat_0c909bd4 = {
#include "b_chaos7/super/dat_0c909bd4.inc"
};
static NJS_OBJECT dat_0c909c08 = {
#include "b_chaos7/super/dat_0c909c08.inc"
};
static NJS_OBJECT dat_0c909c3c = {
#include "b_chaos7/super/dat_0c909c3c.inc"
};
static NJS_OBJECT dat_0c909c70 = {
#include "b_chaos7/super/dat_0c909c70.inc"
};
static NJS_OBJECT dat_0c909ca4 = {
#include "b_chaos7/super/dat_0c909ca4.inc"
};
static NJS_OBJECT dat_0c909cd8 = {
#include "b_chaos7/super/dat_0c909cd8.inc"
};
static NJS_OBJECT dat_0c909d0c = {
#include "b_chaos7/super/dat_0c909d0c.inc"
};
static NJS_OBJECT dat_0c909d40 = {
#include "b_chaos7/super/dat_0c909d40.inc"
};
static NJS_OBJECT dat_0c909d74 = {
#include "b_chaos7/super/dat_0c909d74.inc"
};
NJS_OBJECT super_sonic_object = {
#include "b_chaos7/super/super_sonic_object.inc"
};
static NJS_MATERIAL dat_0c909ddc[1] = {
#include "b_chaos7/super/dat_0c909ddc.inc"
};
static Sint16 dat_0c909df0[507] = {
#include "b_chaos7/super/dat_0c909df0.inc"
};
static NJS_COLOR dat_0c90a1e8[478] = {
#include "b_chaos7/super/dat_0c90a1e8.inc"
};
static NJS_MESHSET dat_0c90a960[1] = {
#include "b_chaos7/super/dat_0c90a960.inc"
};
static NJS_POINT3 dat_0c90a978[212] = {
#include "b_chaos7/super/dat_0c90a978.inc"
};
static NJS_VECTOR dat_0c90b368[212] = {
#include "b_chaos7/super/dat_0c90b368.inc"
};
static NJS_MODEL dat_0c90bd58 = {
#include "b_chaos7/super/dat_0c90bd58.inc"
};
NJS_OBJECT dat_0c90bd80 = {
#include "b_chaos7/super/dat_0c90bd80.inc"
};
static NJS_TEXNAME dat_0c90bdb4[1] = {{TEXT_17, 0, 0}};
static NJS_TEXLIST dat_0c90bdc0 = {
#include "b_chaos7/super/dat_0c90bdc0.inc"
};
static NJS_MATERIAL dat_0c90bdc8[1] = {
#include "b_chaos7/super/dat_0c90bdc8.inc"
};
static Sint16 dat_0c90bddc[136] = {
#include "b_chaos7/super/dat_0c90bddc.inc"
};
static NJS_COLOR dat_0c90beec[128] = {
#include "b_chaos7/super/dat_0c90beec.inc"
};
static NJS_MESHSET dat_0c90c0ec[1] = {
#include "b_chaos7/super/dat_0c90c0ec.inc"
};
static NJS_POINT3 dat_0c90c104[58] = {
#include "b_chaos7/super/dat_0c90c104.inc"
};
static NJS_VECTOR dat_0c90c3bc[58] = {
#include "b_chaos7/super/dat_0c90c3bc.inc"
};
static NJS_MODEL dat_0c90c674 = {
#include "b_chaos7/super/dat_0c90c674.inc"
};
static NJS_OBJECT aura_slow_object = {
#include "b_chaos7/super/aura_slow_object.inc"
};
static NJS_TEXNAME dat_0c90c6d0[1] = {{TEXT_17, 0, 0}};
static NJS_TEXLIST dat_0c90c6dc = {
#include "b_chaos7/super/dat_0c90c6dc.inc"
};
static NJS_MATERIAL dat_0c90c6e4[1] = {
#include "b_chaos7/super/dat_0c90c6e4.inc"
};
static Sint16 dat_0c90c6f8[144] = {
#include "b_chaos7/super/dat_0c90c6f8.inc"
};
static NJS_COLOR dat_0c90c818[132] = {
#include "b_chaos7/super/dat_0c90c818.inc"
};
static NJS_MESHSET dat_0c90ca28[1] = {
#include "b_chaos7/super/dat_0c90ca28.inc"
};
static NJS_POINT3 dat_0c90ca40[61] = {
#include "b_chaos7/super/dat_0c90ca40.inc"
};
static NJS_VECTOR dat_0c90cd1c[61] = {
#include "b_chaos7/super/dat_0c90cd1c.inc"
};
static NJS_MODEL dat_0c90cff8 = {
#include "b_chaos7/super/dat_0c90cff8.inc"
};
static NJS_OBJECT aura_object = {
#include "b_chaos7/super/aura_object.inc"
};
static NJS_TEXNAME dat_0c90d054[1] = {{TEXT_25, 0, 0}};
static NJS_TEXLIST dat_0c90d060 = {
#include "b_chaos7/super/dat_0c90d060.inc"
};
static NJS_MATERIAL dat_0c90d068[1] = {
#include "b_chaos7/super/dat_0c90d068.inc"
};
static Sint16 dat_0c90d07c[144] = {
#include "b_chaos7/super/dat_0c90d07c.inc"
};
static NJS_COLOR dat_0c90d19c[132] = {
#include "b_chaos7/super/dat_0c90d19c.inc"
};
static NJS_MESHSET dat_0c90d3ac[1] = {
#include "b_chaos7/super/dat_0c90d3ac.inc"
};
static NJS_POINT3 dat_0c90d3c4[61] = {
#include "b_chaos7/super/dat_0c90d3c4.inc"
};
static NJS_VECTOR dat_0c90d6a0[61] = {
#include "b_chaos7/super/dat_0c90d6a0.inc"
};
static NJS_MODEL dat_0c90d97c = {
#include "b_chaos7/super/dat_0c90d97c.inc"
};
static NJS_OBJECT aura_fast_object = {
#include "b_chaos7/super/aura_fast_object.inc"
};
static NJS_TEXNAME dat_0c90d9d8[2] = {{TEXT_33, 0, 0}, {TEXT_34, 0, 0}};
static NJS_TEXLIST dat_0c90d9f0 = {
#include "b_chaos7/super/dat_0c90d9f0.inc"
};
static NJS_MATERIAL dat_0c90d9f8[2] = {
#include "b_chaos7/super/dat_0c90d9f8.inc"
};
static Sint16 dat_0c90da20[18] = {
#include "b_chaos7/super/dat_0c90da20.inc"
};
static Sint16 dat_0c90da44[9] = {
#include "b_chaos7/super/dat_0c90da44.inc"
};
static NJS_COLOR dat_0c90da58[16] = {
#include "b_chaos7/super/dat_0c90da58.inc"
};
static NJS_COLOR dat_0c90da98[8] = {
#include "b_chaos7/super/dat_0c90da98.inc"
};
static NJS_MESHSET dat_0c90dab8[2] = {
#include "b_chaos7/super/dat_0c90dab8.inc"
};
static NJS_POINT3 dat_0c90dae8[16] = {
#include "b_chaos7/super/dat_0c90dae8.inc"
};
static NJS_VECTOR dat_0c90dba8[16] = {
#include "b_chaos7/super/dat_0c90dba8.inc"
};
static NJS_MODEL dat_0c90dc68 = {
#include "b_chaos7/super/dat_0c90dc68.inc"
};
static NJS_OBJECT wake_object = {
#include "b_chaos7/super/wake_object.inc"
};
static NJS_TEXNAME dat_0c90dcc4[2] = {{TEXT_33, 0, 0}, {TEXT_34, 0, 0}};
static NJS_TEXLIST dat_0c90dcdc = {
#include "b_chaos7/super/dat_0c90dcdc.inc"
};
static NJS_MATERIAL dat_0c90dce4[2] = {
#include "b_chaos7/super/dat_0c90dce4.inc"
};
static Sint16 dat_0c90dd0c[18] = {
#include "b_chaos7/super/dat_0c90dd0c.inc"
};
static Sint16 dat_0c90dd30[9] = {
#include "b_chaos7/super/dat_0c90dd30.inc"
};
static NJS_COLOR dat_0c90dd44[16] = {
#include "b_chaos7/super/dat_0c90dd44.inc"
};
static NJS_COLOR dat_0c90dd84[8] = {
#include "b_chaos7/super/dat_0c90dd84.inc"
};
static NJS_MESHSET dat_0c90dda4[2] = {
#include "b_chaos7/super/dat_0c90dda4.inc"
};
static NJS_POINT3 dat_0c90ddd4[16] = {
#include "b_chaos7/super/dat_0c90ddd4.inc"
};
static NJS_VECTOR dat_0c90de94[16] = {
#include "b_chaos7/super/dat_0c90de94.inc"
};
static NJS_MODEL dat_0c90df54 = {
#include "b_chaos7/super/dat_0c90df54.inc"
};
static NJS_OBJECT wake_back_object = {
#include "b_chaos7/super/wake_back_object.inc"
};
#pragma section

/* the texlists the aura and the wake cycle through, each a run of
 * super_sonic_textures. super_aura_textures holds s1sonic01 to 08 and
 * super_aura_fast_textures s2sonic01 to 08, one to a texlist.
 * super_wake_textures holds ss_waterl01 to 16, two to a texlist */
static NJS_TEXLIST super_aura_textures[8] = {
#include "b_chaos7/super/super_aura_textures.inc"
};
static NJS_TEXLIST super_aura_fast_textures[8] = {
#include "b_chaos7/super/super_aura_fast_textures.inc"
};
static NJS_TEXLIST super_wake_textures[8] = {
#include "b_chaos7/super/super_wake_textures.inc"
};

static NJS_TEXLIST super_ball_textures[2] = {
#include "b_chaos7/super/super_ball_textures.inc"
};

/* a dash's texture for each step it draws: ss_dash01, ss_dash02, ss_dash03 for
 * nine steps, then ss_dash04 to ss_dash12. the second set has ss_dash01,
 * ss_dash13, ss_dash14 for eight steps, then ss_dash15 to ss_dash23. a dash
 * with the second set ends before it draws ss_dash23 */
static NJS_TEXLIST super_dash_textures[DASH_STEPS] = {
#include "b_chaos7/super/super_dash_textures.inc"
};
static NJS_TEXLIST super_dash_textures_b[SECOND_SET_END] = {
#include "b_chaos7/super/super_dash_textures_b.inc"
};

/* the wake's spray, as splash_create_from takes it: drops that grow and fall.
 * super_aura_spray sets its pos, speed and scale for each drop. a drop keeps
 * it for its life, but reads only rate, drag, lift and growth after its
 * start */
static struct dust_params super_spray = {
    1.0F,
    0.038F,
    0.34F,
    0.0F,
    -0.06F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {1.0F, 0.4F, 0.4F, 0.4F},
};

/* a turn at the streak's scale, which super_dash_step's second step builds.
 * the turn points from the player's middle back to where it was a step
 * before. that step moves the end of the streak's copy at x 0 by it */
static NJS_MATRIX super_dash_matrix;
/* which of the aura's sounds plays, as an enum aura_sound: none, SE_AURA or
 * SE_AURA_FAST. every aura task shares it */
static Sint8 super_aura_sound;

#pragma section HEAD
void super_aura_start(struct task *task)
{
    task->step = super_aura_step;
    task->draw = super_aura_draw;
    task->destructor = super_aura_destroy;
    super_aura_draw(task);
    super_aura_sound = AURA_SILENT;
}

static void super_aura_destroy(struct task *task)
{
    sound_stop(SE_AURA);
    sound_stop(SE_AURA_FAST);
}

void super_ball_follow_start(struct task *task)
{
    super_ball_follow_place(task->work);
    task->step = super_ball_follow_step;
    task->draw = super_ball_follow_draw;
    super_ball_follow_draw(task);
}

void super_ball_shrink_start(struct task *task)
{
    super_ball_shrink_place(task->work);
    task->step = super_ball_shrink_step;
    task->draw = super_ball_shrink_draw;
    super_ball_shrink_draw(task);
}
#pragma section

static void super_aura_spray(NJS_POINT3 *pos, NJS_VECTOR *speed, Float scale)
{
    super_spray.pos = *pos;
    super_spray.speed = *speed;
    super_spray.scale = scale;
    splash_create_from(&super_spray);
}

/* the aura about the player's middle, and his wake under him while he stands
 * on the sea floor. it draws only on a frame's first step */
static void super_aura_draw(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[AURA(work)->player];
    struct player *record = player_get(AURA(work)->player);
    Angle ang;
    /* @hack redundant: unused, a vector nothing reads, which makes the frame 24
     * bytes (ADD #-24,R15 at 0x0c91d556); without it, 2 of 258 words differ */
    NJS_VECTOR unused;

    if (frame_step != 0) {
        return;
    }
    njSetTexture(&super_aura_textures[work->tick & 7]);
    njPushMatrix(NULL);
    njTranslateV(NULL, &player->collision->shapes->center);
    TURN_VIA(njRotateZ, player->ang.z, ang);
    TURN_VIA(njRotateX, player->ang.x, ang);
    TURN_VIA(njRotateY, -player->ang.y, ang);
    njScale(NULL, -1.0F, 1.0F, 1.0F);
    if (record->speed.x > AURA_FAST) {
        njSetTexture(&super_aura_fast_textures[(work->tick >> 1) & 7]);
        draw_tree(&aura_fast_object);
    } else if (record->speed.x > AURA_QUICK) {
        njSetTexture(&super_aura_textures[(work->tick >> 1) & 7]);
        draw_tree(&aura_object);
    } else {
        draw_tree(&aura_slow_object);
    }
    njPopMatrix(1);
    if ((player->flags & PLAYER_ON_LAND) && record->ground_object != NULL &&
        record->ground_object->model == &sea_floor_model) {
        njSetTexture(&super_wake_textures[(work->tick >> 1) & 7]);
        njPushMatrix(NULL);
        njTranslateV(NULL, &player->pos);
        TURN_VIA(njRotateZ, player->ang.z, ang);
        TURN_VIA(njRotateX, player->ang.x, ang);
        TURN_VIA(njRotateY, -player->ang.y, ang);
        if (WAKE(work) != 0.0F) {
            njPushMatrix(NULL);
            /* @hack redundant: the products by 1.0F, and the negation written
             * as a product by -1.0F, which keep a multiply by 1 (FMUL
             * FR15,FR14 at 0x0c91d72e) whose result the negation takes (FNEG
             * FR4 at 0x0c91d732); as -WAKE(work), WAKE(work) and WAKE(work),
             * 235 of 258 words differ */
            njScale(NULL, -1.0F * WAKE(work), 1.0F * WAKE(work),
                    1.0F * WAKE(work));
            draw_tree(&wake_object);
            draw_tree(&wake_back_object);
            njPopMatrix(1);
        }
        if (WAKE_WIDE(work) != 0.0F) {
            njPushMatrix(NULL);
            njScale(NULL, -0.8F * WAKE_WIDE(work), 0.8F * WAKE_WIDE(work),
                    1.3F * WAKE_WIDE(work));
            draw_tree(&wake_object);
            draw_tree(&wake_back_object);
            njPopMatrix(1);
        }
        njPopMatrix(1);
    }
}

/* set pos to where a drop thrown at speed starts. that is two steps of speed
 * out from the player, less three steps of his motion's speed. the wake's
 * drops pass speed in the world. the wide and widest wakes' drops pass it in
 * the player's own frame, before player_turn_to_world turns it */
#define SPRAY_FROM(pos, player, motion, speed)                                 \
    ((pos).x =                                                                 \
         (player)->pos.x + ((speed).x * 2.0F) - ((motion)->speed.x * 3.0F),    \
     (pos).y =                                                                 \
         (player)->pos.y + ((speed).y * 2.0F) - ((motion)->speed.y * 3.0F),    \
     (pos).z =                                                                 \
         (player)->pos.z + ((speed).z * 2.0F) - ((motion)->speed.z * 3.0F))

/* end the aura once the player's record is gone or he is no longer super.
 * every wake is 0 while his ground_object is anything but the sea floor */
static void super_aura_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[AURA(work)->player];
    struct task_motion *motion = player_motion[AURA(work)->player];
    struct player *record = player_get(AURA(work)->player);
    NJS_VECTOR speed;
    NJS_POINT3 pos;

    if (record == NULL || !(record->upgrades & PLAYER_SONIC_SUPER)) {
        task_kill(task);
        return;
    }
    work->tick++;
    WAKE(work) = njSqrt(_builtin_fabsf(record->speed.x)) * WAKE_SCALE;
    if (record->speed.x > SPRAY_SPEED && (player->flags & PLAYER_ON_LAND) &&
        record->ground_object != NULL &&
        record->ground_object->model == &sea_floor_model) {
        speed.x = record->speed.x * 0.6F;
        speed.y = 0.7F;
        speed.z = -0.75F - (rand() * RAND_SCALE * 0.3F);
        player_turn_to_world(player, &speed);
        SPRAY_FROM(pos, player, motion, speed);
        super_aura_spray(&pos, &speed, 0.28F);
        /* @hack redundant: the copy of the player's position, which
         * SPRAY_FROM overwrites whole before any read (MOV.L @(32,R14),R3 at
         * 0x0c91d93c); without it, 621 of 693 words differ */
        pos = player->pos;
        speed.x = record->speed.x * 0.6F;
        speed.y = 0.7F;
        speed.z = 0.75F + (rand() * RAND_SCALE * 0.3F);
        player_turn_to_world(player, &speed);
        SPRAY_FROM(pos, player, motion, speed);
        super_aura_spray(&pos, &speed, 0.28F);
    }
    if (record->speed.x > WAKE_WIDE_SPEED) {
        if (WAKE_WIDE(work) < 1.0F) {
            WAKE_WIDE(work) += WAKE_GROWTH;
        }
        if ((player->flags & PLAYER_ON_LAND) && record->ground_object != NULL &&
            record->ground_object->model == &sea_floor_model) {
            if (rand() * RAND_SCALE > 0.94F) {
                speed.x = record->speed.x * 0.4F;
                speed.y = 0.3F;
                speed.z = -2.0F - (rand() * RAND_SCALE * 0.3F);
                SPRAY_FROM(pos, player, motion, speed);
                player_turn_to_world(player, &speed);
                super_aura_spray(&pos, &speed, 0.4F);
                speed.x = record->speed.x * 0.4F;
                speed.y = 0.3F;
                speed.z = 2.0F + (rand() * RAND_SCALE * 0.3F);
                SPRAY_FROM(pos, player, motion, speed);
                player_turn_to_world(player, &speed);
                super_aura_spray(&pos, &speed, 0.4F);
            }
        }
    } else if (WAKE_WIDE(work) > 0.0F) {
        WAKE_WIDE(work) -= WAKE_GROWTH;
    } else {
        WAKE_WIDE(work) = 0.0F;
    }
    if (record->speed.x > WAKE_WIDEST_SPEED) {
        if (WAKE_WIDEST(work) < 1.0F) {
            WAKE_WIDEST(work) += WAKE_GROWTH;
        }
        if ((player->flags & PLAYER_ON_LAND) && record->ground_object != NULL &&
            record->ground_object->model == &sea_floor_model) {
            if (rand() * RAND_SCALE < 0.2F) {
                speed.x = record->speed.x * 0.2F;
                speed.y = 0.6F;
                speed.z = -1.25F - (rand() * RAND_SCALE * 0.5F);
                SPRAY_FROM(pos, player, motion, speed);
                player_turn_to_world(player, &speed);
                super_aura_spray(&pos, &speed, 0.6F);
                speed.x = record->speed.x * 0.2F;
                speed.y = 0.6F;
                speed.z = 1.25F + (rand() * RAND_SCALE * 0.5F);
                SPRAY_FROM(pos, player, motion, speed);
                player_turn_to_world(player, &speed);
                super_aura_spray(&pos, &speed, 0.6F);
            }
        }
    } else if (WAKE_WIDEST(work) > 0.0F) {
        WAKE_WIDEST(work) -= WAKE_GROWTH;
    } else {
        WAKE_WIDEST(work) = 0.0F;
    }
    if (record->ground_object == NULL ||
        record->ground_object->model != &sea_floor_model) {
        WAKE(work) = 0.0F;
        WAKE_WIDE(work) = 0.0F;
        WAKE_WIDEST(work) = 0.0F;
    }
    switch (super_aura_sound) {
    case AURA_SILENT:
        if (record->speed.x > AURA_QUICK) {
            sound_play(SE_AURA, 0, 0, 127);
            super_aura_sound = AURA_RUNNING;
        }
        break;
    case AURA_RUNNING:
        if (record->speed.x > AURA_FAST) {
            sound_stop(SE_AURA);
            sound_play(SE_AURA_FAST, 0, 0, 127);
            super_aura_sound = AURA_RUNNING_FAST;
        } else if (record->speed.x < AURA_QUIET) {
            sound_stop(SE_AURA);
            super_aura_sound = AURA_SILENT;
        }
        break;
    case AURA_RUNNING_FAST:
        if (record->speed.x < AURA_QUIET) {
            sound_stop(SE_AURA_FAST);
            super_aura_sound = AURA_SILENT;
        }
        break;
    }
    super_aura_draw(task);
}

/* place a follow ball as an offset from the player's position, which scl
 * keeps. it starts BALL_HEIGHT up, turned as the player faces, then 1 to 3
 * further in a random direction. TURN reads its angle twice. each random turn
 * draws one rand for its test and another for its angle */
static void super_ball_follow_place(struct task_work *work)
{
    struct task_work *player = player_work[BALL(work)->player];
    NJS_VECTOR vec;
    NJS_VECTOR offset;

    if (player != NULL) {
        vec.x = vec.z = 0.0F;
        vec.y = BALL_HEIGHT;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, player->ang.z);
        TURN(njRotateX, player->ang.x);
        TURN(njRotateY, -player->ang.y);
        njCalcVector(NULL, &vec, &work->pos);
        vec.x = vec.z = 0.0F;
        vec.y = RANDOM(1.0F, 3.0F);
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, (Angle)RANDOM_ANGLE());
        TURN(njRotateX, (Angle)RANDOM_ANGLE());
        TURN(njRotateY, player->ang.y);
        njCalcVector(NULL, &vec, &offset);
        njPopMatrix(2);
        njAddVector(&work->pos, &offset);
        work->scl = player->pos;
    }
    if (rand() * RAND_SCALE > 0.5F) {
        BALL(work)->texlist = &super_ball_textures[0];
    } else {
        BALL(work)->texlist = &super_ball_textures[1];
    }
}

/* place a shrink ball in the world, BALL_HEIGHT up from the player and turned
 * as he faces. it starts at three times its size */
static void super_ball_shrink_place(struct task_work *work)
{
    struct task_work *player = player_work[BALL(work)->player];
    NJS_VECTOR vec;

    if (player != NULL) {
        vec.x = vec.z = 0.0F;
        vec.y = BALL_HEIGHT;
        njPushMatrix(&_unit_matrix_);
        TURN(njRotateZ, player->ang.z);
        TURN(njRotateX, player->ang.x);
        TURN(njRotateY, -player->ang.y);
        njCalcVector(NULL, &vec, &work->pos);
        njAddVector(&work->pos, &player->pos);
        njPopMatrix(1);
    }
    if (rand() * RAND_SCALE > 0.5F) {
        BALL(work)->texlist = &super_ball_textures[0];
    } else {
        BALL(work)->texlist = &super_ball_textures[1];
    }
    work->scl.x = 3.0F;
}

/* the ball where the player was, turned as it faced, and fading */
static void super_ball_follow_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;

    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_ALPHA);
    color.r = color.g = color.b = 0.0F;
    color.a = -(TASK_TIMER(work) * (1.0F / BALL_STEPS));
    njSetConstantMaterial(&color);
    njSetTexture(BALL(work)->texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x + work->scl.x, work->pos.y + work->scl.y,
                work->pos.z + work->scl.z);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, -work->ang.y);
    draw_tree(&sonic_ball_object);
    njPopMatrix(1);
    color.a = 0.0F;
    njSetConstantMaterial(&color);
    draw_attr_restore();
}

/* draw the ball where the player was a step before, for BALL_STEPS steps,
 * then end */
static void super_ball_follow_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[BALL(work)->player];

    if (TASK_TIMER(work)++ >= BALL_STEPS) {
        task_kill(task);
        return;
    }
    super_ball_follow_draw(task);
    work->scl = player->pos;
    work->ang = player->ang;
}

/* the ball where it was left, at its size, and fading */
static void super_ball_shrink_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_ARGB color;

    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_ALPHA);
    color.r = color.g = color.b = 0.0F;
    color.a = -(TASK_TIMER(work) * (1.0F / SHRINK_STEPS));
    njSetConstantMaterial(&color);
    njSetTexture(BALL(work)->texlist);
    njPushMatrix(NULL);
    njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
    njScale(NULL, work->scl.x, work->scl.x, work->scl.x);
    draw_tree(&sonic_ball_object);
    njPopMatrix(1);
    color.a = 0.0F;
    njSetConstantMaterial(&color);
    draw_attr_restore();
}

/* draw the ball for SHRINK_STEPS steps, shrinking it after each, then end */
static void super_ball_shrink_step(struct task *task)
{
    struct task_work *work = task->work;

    if (TASK_TIMER(work)++ >= SHRINK_STEPS) {
        task_kill(task);
        return;
    }
    super_ball_shrink_draw(task);
    work->scl.x -= 0.4F;
}

/* set ang[1] and ang[2] to the angles about y and z that turn the x axis
 * toward vector. clear ang[0]. a vector of no length leaves all three */
/* @hack type: ang is an Angle * that callers point at a struct angles' x,
 * and the function reaches the other members by index, which ISO C leaves
 * undefined. super_dash_step so keeps &aim.x in R12 and reads aim through it
 * (ADD #4,R12 at 0x0c91e860, MOV.L @(4,R12),R2 at 0x0c91e872). as a struct
 * angles *, 61 of 412 words differ, all of them super_dash_step's */
static void super_dash_angles(NJS_VECTOR *vector, Angle *ang)
{
    Float length = njScalor(vector);

    if (length != 0.0F) {
        ang[1] = (Angle)(atan2f(-vector->z, vector->x) * ANGLE_PER_RADIAN);
        ang[2] = (Angle)(asinf(vector->y / length) * ANGLE_PER_RADIAN);
        ang[0] = 0;
    }
}

/* copy the streak, its model and its points onto the heap. the dash's vars
 * keep the copy's object. start the dash at the player's middle. FALSE if the
 * heap has no room, with none of the copy left allocated */
static Bool super_dash_alloc(struct task_work *work)
{
    NJS_OBJECT *object = &sonic_dash_object;
    Uint32 count = object->model->nbPoint;
    NJS_OBJECT *copy;
    NJS_POINT3 *to;
    NJS_POINT3 *from;
    Uint32 i;
    struct task_work *player;
    struct collision_shape *shape;

    copy = heap_alloc(sizeof(NJS_OBJECT));
    /* @hack spelling: an else that returns FALSE, not an early return, which
     * puts the model copy's source in R2 and its destination in R3
     * (0x0c91e410, 0x0c91e412); with an early return, the two trade and 22
     * of 142 words differ */
    if (copy != NULL) {
        *copy = *object;
        DASH(work)->object = copy;
    } else {
        return FALSE;
    }
    if ((copy->model = heap_alloc(sizeof(NJS_MODEL))) != NULL) {
        *copy->model = *object->model;
    } else {
        heap_free(copy);
        return FALSE;
    }
    if ((copy->model->points = heap_alloc(count * sizeof(NJS_POINT3))) !=
        NULL) {
        to = copy->model->points;
        from = object->model->points;
        for (i = 0; i < count; i++) {
            VECTOR_COPY(to, from);
            to++;
            from++;
        }
    } else {
        heap_free(copy->model);
        heap_free(copy);
        return FALSE;
    }
    player = player_work[DASH(work)->player];
    if (player != NULL) {
        shape = player->collision->shapes;
        work->pos = shape->center;
    }
    return TRUE;
}

/* the streak in the step's texture: the model itself on the first step, and
 * its copy after */
static void super_dash_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = DASH(work)->object;

    if (work->flags & DASH_SECOND_SET) {
        njSetTexture(&super_dash_textures_b[TASK_TIMER(work) - 1]);
    } else {
        njSetTexture(&super_dash_textures[TASK_TIMER(work) - 1]);
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    TURN(njRotateZ, work->ang.z);
    njScaleV(NULL, &work->scl);
    if (TASK_TIMER(work) == 1) {
        draw_tree(&sonic_dash_object);
    } else {
        draw_tree(object);
    }
    njPopMatrix(1);
}

/* draw the dash for DASH_STEPS steps, or with the second set until step
 * SECOND_SET_END. the first step stretches the streak from the player's
 * middle back to where the dash started. it takes the second set if
 * own.sonic.dashes shows a dash two steps back but none one step back. the
 * second step turns the copy's end at x 0, its odd points, to
 * super_dash_matrix */
static void super_dash_step(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = DASH(work)->object;
    NJS_VECTOR vec;
    NJS_VECTOR offset;
    struct angles aim;

    /* @hack redundant: the second test as an else after the first's return,
     * which loads its SECOND_SET_END into R2 (MOV #19,R2 at 0x0c91e5e4);
     * without the else, the load takes R3, and 2 of 370 words differ */
    if (TASK_TIMER(work)++ >= DASH_STEPS) {
        task_kill(task);
        return;
    } else if ((work->flags & DASH_SECOND_SET) &&
               TASK_TIMER(work) >= SECOND_SET_END) {
        task_kill(task);
        return;
    }
    if (TASK_TIMER(work) == 1) {
        struct task_work *player = player_work[DASH(work)->player];
        struct player *record;

        if (player != NULL) {
            vec.x = vec.z = 0.0F;
            vec.y = player_get(DASH(work)->player)->consts.center_y;
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateZ, player->ang.z);
            TURN(njRotateX, player->ang.x);
            TURN(njRotateY, -player->ang.y);
            njCalcVector(NULL, &vec, &offset);
            njPopMatrix(1);
            njAddVector(&offset, &player->pos);
            vec = work->pos;
            work->pos = offset;
            njSubVector(&vec, &offset);
            super_dash_angles(&vec, &work->ang.x);
            work->scl.x = njScalor(&vec) * (1.0F / STREAK_LENGTH);
            work->scl.y = 1.0F;
            work->scl.z = 1.0F;
            record = player_motion[DASH(work)->player]->record;
            if ((record->own.sonic.dashes[0] & 4) &&
                !(record->own.sonic.dashes[0] & 2)) {
                work->flags |= DASH_SECOND_SET;
            }
        }
    } else if (TASK_TIMER(work) == 2) {
        struct task_work *player = player_work[DASH(work)->player];
        /* @hack spelling: the streak read through original, which loads its
         * address into R4 and its model through it (0x0c91e7bc, 0x0c91e7c0).
         * read as sonic_dash_object, the pool holds its model field's address,
         * and 6 of 370 words and a pool word differ */
        NJS_OBJECT *original;
        NJS_POINT3 *point;
        Uint32 count;
        Uint32 i;

        if (player != NULL) {
            original = &sonic_dash_object;
            point = object->model->points;
            count = original->model->nbPoint;
            vec.x = vec.z = 0.0F;
            vec.y = player_get(DASH(work)->player)->consts.center_y;
            njPushMatrix(&_unit_matrix_);
            TURN(njRotateZ, player->ang.z);
            TURN(njRotateX, player->ang.x);
            TURN(njRotateY, -player->ang.y);
            njCalcVector(NULL, &vec, &offset);
            njPopMatrix(1);
            njAddVector(&offset, &player->pos);
            vec = work->pos;
            njSubVector(&vec, &offset);
            if (njScalor(&vec) != 0.0F) {
                super_dash_angles(&vec, &aim.x);
                njUnitMatrix(&super_dash_matrix);
                TURN_MATRIX(njRotateY, &super_dash_matrix, aim.y);
                TURN_MATRIX(njRotateZ, &super_dash_matrix, aim.z);
                njScaleV(&super_dash_matrix, &work->scl);
                njPushMatrix(&_unit_matrix_);
                TURN(njRotateY, work->ang.y);
                TURN(njRotateZ, work->ang.z);
                njScaleV(NULL, &work->scl);
                njInvertMatrix(NULL);
                njMultiMatrix(NULL, &super_dash_matrix);
                for (i = 0, point++; i < count; i += 2, point += 2) {
                    vec = *point;
                    njCalcPoint(NULL, &vec, point);
                }
                njPopMatrix(1);
            }
        }
    }
    super_dash_draw(task);
}

/* free the dash's copy: its points, its model and its object. the model and
 * the object are each read through before their NULL tests. no test can fail:
 * super_dash_start sets this destructor only once super_dash_alloc has the
 * whole copy */
static void super_dash_destroy(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *object = DASH(work)->object;

    if (object->model->points != NULL) {
        heap_free(object->model->points);
    }
    if (object->model != NULL) {
        heap_free(object->model);
    }
    if (object != NULL) {
        heap_free(object);
    }
}

#pragma section HEAD
void super_dash_start(struct task *task)
{
    if (super_dash_alloc(task->work) == TRUE) {
        task->step = super_dash_step;
        task->draw = super_dash_draw;
        task->destructor = super_dash_destroy;
    } else {
        task_kill(task);
    }
}
#pragma section
