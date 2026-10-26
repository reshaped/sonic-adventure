/* loading and releasing the texture lists of the characters, the regular
 * sets, the stages and their acts */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdio.h>

#include "adv0100/texlists.h"
#include "animal/barrier.h"
#include "animal/textures.h"
#include "chao/chao.h"
#include "enemy/leon.h"
#include "enemy/robo.h"
#include "enemy/saru.h"
#include "enemy/snake.h"
#include "enemy/spina.h"
#include "enemy/uni.h"
#include "hud/boss_name.h"
#include "hud/life_gauge.h"
#include "object/piece.h"
#include "object/tikal_light.h"
#include "player/big_kaeru_textures.h"
#include "player/big_models.h"
#include "player/e102_bonus.h"
#include "player/e102_hit.h"
#include "player/effects.h"
#include "player/player.h"
#include "player/race.h"
#include "player/textures.h"
#include "progress/emblem.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/loader.h"
#include "system/prs.h"
#include "system/texlists.h"
#include "system/texture.h"

/* texlist_slots' slots: the regular slot, then from SLOT_STAGE the stage's
 * four */
#define SLOT_REGULAR 0
#define SLOT_STAGE 1
#define TEXLIST_SLOTS 5

/* the most textures texture_load_pvm can take from one PVM: the length of
 * its scratch list */
#define PVM_TEXTURES 512

/* the room in regular_texlists and in stage_texlists, the null included */
#define REGULAR_TEXLISTS 32
#define STAGE_TEXLISTS 65

/* the tries load_prs and texture_load_prs_file make at an open, and at a
 * read. between two opens, they wait out OPEN_WAIT passes of an empty
 * loop */
#define TRIES 3
#define OPEN_WAIT 1000

/* the room a file is read into below the end of its area: its size,
 * rounded up to a multiple of 256 */
#define FILE_ROOM(size) (((size) + 255) & ~255)

/* MODULE_TEXLIST, MODULE_PVMS and MODULE_ACT take an address in a module
 * as a texture list, a list of pvms or an act's textures. the address holds
 * one only while that module is loaded */
#define MODULE_TEXLIST(address) ((NJS_TEXLIST *)(address))
#define MODULE_PVMS(address) ((struct texture_pvm *)(address))
#define MODULE_ACT(address) ((struct act_textures *)(address))

static Sint32 load_prs(char *name, NJS_TEXLIST *texlist);

/* the lists texture_release_regular releases, up to a null. they are
 * meant to be the regular set's lists after its first. texture_load_regular
 * stores them all in the first entry, and then a null over them */
static NJS_TEXLIST *regular_texlists[REGULAR_TEXLISTS];
/* a regular set's first list, then the current stage's first four, or null
 * for an empty slot */
static NJS_TEXLIST *texlist_slots[TEXLIST_SLOTS];
/* the stage's lists after its first four, ending at a null */
static NJS_TEXLIST *stage_texlists[STAGE_TEXLISTS];
/* the scratch list texture_load_pvm loads each PVM into. past the last
 * PVM's textures, it holds what earlier loads left */
static NJS_TEXNAME pvm_textures[PVM_TEXTURES];

/* the shelter keeps its acts' pvms in its module */
static struct act_textures shelter_acts[] = {
    {STAGE_ACT(STAGE_SHELTER, 0), 10, 0, MODULE_PVMS(0x0caa0d60)},
    {STAGE_ACT(STAGE_SHELTER, 1), 2, 0, MODULE_PVMS(0x0caa0db0)},
    {STAGE_ACT(STAGE_SHELTER, 2), 4, 0, MODULE_PVMS(0x0caa0dc0)},
    {STAGE_ACT(STAGE_SHELTER, 3), 1, 0, MODULE_PVMS(0x0caa0de0)},
};

/* each stage's list of pvms, from here to stage_pvms. the first four
 * entries take the stage's slots, and a null texlist leaves its slot empty.
 * the entries after the fourth end at a null texlist */
static struct texture_pvm shooting1_pvms[] = {
    {"SHOOTING0", MODULE_TEXLIST(0x0c90ada4)},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, MODULE_TEXLIST(0x0c955d38)},
    {NULL, MODULE_TEXLIST(0x0cad4248)},
    {NULL, MODULE_TEXLIST(0x0c955bac)},
    {"SHOOTING1", MODULE_TEXLIST(0x0c956ba4)},
    {NULL, NULL},
};

static struct texture_pvm shooting2_pvms[] = {
    {"SHOOTING0", MODULE_TEXLIST(0x0c90ada4)},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, MODULE_TEXLIST(0x0c955d38)},
    {NULL, MODULE_TEXLIST(0x0cad4248)},
    {NULL, MODULE_TEXLIST(0x0c955bac)},
    {"SHOOTING2", MODULE_TEXLIST(0x0c9e04b4)},
    {"CHAOS_LIFEGAUGE", &chaos_lifegauge_texlist},
    {NULL, &b32ascii_texlist},
    {NULL, NULL},
};

static struct texture_pvm highway_pvms[] = {
    {"OBJ_HIGHWAY", MODULE_TEXLIST(0x0ca3eab8)},
    {"NISEPAT", MODULE_TEXLIST(0x0ca791b4)},
    {"SUPI_SUPI", &supi_supi_texlist},
    {"UNI_A_UNIBODY", &uni_a_unibody_texlist},
    {"TOGEBALL_TOGEBALL", &togeball_togeball_texlist},
    {"OBJ_HIGHWAY2", MODULE_TEXLIST(0x0ca3ed68)},
    {"USA", &usa_texlist},
    {"BANB", &banb_texlist},
    {"WARA", &wara_texlist},
    {"TUBA", &tuba_texlist},
    {"KOAR", &koar_texlist},
    {"MILESRACE", &milesrace_texlist},
    {NULL, NULL},
};

static struct texture_pvm mountain_pvms[] = {
    {"OBJ_MOUNTAIN", MODULE_TEXLIST(0x0ca3a3f8)},
    {"E_SARU", &e_saru_texlist},
    {"SUPI_SUPI", &supi_supi_texlist},
    {"UNI_C_UNIBODY", &uni_c_unibody_texlist},
    {"E_BOMB", &e_bomb_texlist},
    {"TOGEBALL_TOGEBALL", &togeball_togeball_texlist},
    {"MOUNTAIN_STEAM", MODULE_TEXLIST(0x0cad74a8)},
    {"MOUNTAIN_E104", MODULE_TEXLIST(0x0caaf3c0)},
    {"MOUNTAIN_MEXPLOSION", MODULE_TEXLIST(0x0cad5a18)},
    {"YOUGAN_ANIM", MODULE_TEXLIST(0x0ca9cb6c)},
    {"ZOU", &zou_texlist},
    {"GORI", &gori_texlist},
    {"LION", &lion_texlist},
    {"OUM", &oum_texlist},
    {"MOGU", &mogu_texlist},
    {"CHAOS_LIFEGAUGE", &chaos_lifegauge_texlist},
    {NULL, &b32ascii_texlist},
    {"MOUNTAIN_CARRIER", MODULE_TEXLIST(0x0caaeea0)},
    {"TUBA", &tuba_texlist},
    {NULL, NULL},
};

static struct texture_pvm sky_deck_pvms[] = {
    {"OBJ_SKYDECK", MODULE_TEXLIST(0x0c9ff584)},
    {"E_AIRCRAFT", MODULE_TEXLIST(0x0cae591c)},
    {"SUPI_SUPI", &supi_supi_texlist},
    {"UNI_A_UNIBODY", &uni_a_unibody_texlist},
    {"TOGEBALL_TOGEBALL", &togeball_togeball_texlist},
    {"AIR_SIGNAL", MODULE_TEXLIST(0x0cae5bbc)},
    {"TUBA", &tuba_texlist},
    {"OUM", &oum_texlist},
    {"KUJA", &kuja_texlist},
    {"LION", &lion_texlist},
    {"SUKA", &suka_texlist},
    {"MILESRACE", &milesrace_texlist},
    {NULL, MODULE_TEXLIST(0x0cae7544)},
    {NULL, NULL},
};

static struct texture_pvm ruin_pvms[] = {
    {"OBJ_RUIN", MODULE_TEXLIST(0x0ca27d14)},
    {"OBJ_RUIN2", MODULE_TEXLIST(0x0ca27f20)},
    {"E_LEON", &e_leon_texlist},
    {"E_SNAKE", &e_snake_texlist},
    {"UNI_C_UNIBODY", &uni_c_unibody_texlist},
    {"GORI", &gori_texlist},
    {"LION", &lion_texlist},
    {"ZOU", &zou_texlist},
    {"PEN", &pen_texlist},
    {"MOGU", &mogu_texlist},
    {NULL, NULL},
};

static struct texture_pvm final_pvms[] = {
    {"OBJ_FINALEGG", MODULE_TEXLIST(0x0ca89484)},
    {"E_ROBO", &e_robo_texlist},
    {"SUPI_SUPI", &supi_supi_texlist},
    {NULL, NULL},
    {"EFF_FINALEGG_POM", MODULE_TEXLIST(0x0cb0e2d0)},
    {"GACHAPON", MODULE_TEXLIST(0x0cb012c4)},
    {"USA", &usa_texlist},
    {"BANB", &banb_texlist},
    {"WARA", &wara_texlist},
    {"GORI", &gori_texlist},
    {"MOGU", &mogu_texlist},
    {NULL, NULL},
};

static struct texture_pvm station_square_pvms[] = {
    {"OBJ_SS", MODULE_TEXLIST(0x0ca5f4b4)},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {"SSCAR", MODULE_TEXLIST(0x0ca9b9cc)},
    {"SS_DENTOU", MODULE_TEXLIST(0x0ca97c0c)},
    {"SS_KANBAN", MODULE_TEXLIST(0x0ca97c44)},
    {"SS_PEOPLE", MODULE_TEXLIST(0x0cb2c92c)},
    {"SS_EKIIN", MODULE_TEXLIST(0x0cb2c994)},
    {"SS_MIZUGI", MODULE_TEXLIST(0x0cb2c9f0)},
    {"SS_BURGER", MODULE_TEXLIST(0x0cb2cac4)},
    {"SS_BG", MODULE_TEXLIST(0x0ca5f4ec)},
    {"EMBLEM", &emblem_texlist},
    {NULL, NULL},
};

static struct texture_pvm alife_ss_pvms[] = {
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {"CHAO", MODULE_TEXLIST(0x0ca54a5c)},
    {"AL_ICON", MODULE_TEXLIST(0x0c91d654)},
    {"CHAO_OBJECT", MODULE_TEXLIST(0x0ca54e0c)},
    {"CHAO_HYOUJI", MODULE_TEXLIST(0x0ca54f1c)},
    {"GARDEN00_OBJECT", MODULE_TEXLIST(0x0cb904d8)},
    {"GOMA", &goma_texlist},
    {"USA", &usa_texlist},
    {"BANB", &banb_texlist},
    {"KOAR", &koar_texlist},
    {"OUM", &oum_texlist},
    {"KUJA", &kuja_texlist},
    {"PEN", &pen_texlist},
    {"TUBA", &tuba_texlist},
    {"WARA", &wara_texlist},
    {"ZOU", &zou_texlist},
    {"GORI", &gori_texlist},
    {"LION", &lion_texlist},
    {"MOGU", &mogu_texlist},
    {"RAKO", &rako_texlist},
    {"SUKA", &suka_texlist},
    {NULL, NULL},
};

static struct texture_pvm alife_eg_pvms[] = {
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {"CHAO", MODULE_TEXLIST(0x0ca54a5c)},
    {"AL_ICON", MODULE_TEXLIST(0x0c91d654)},
    {"CHAO_OBJECT", MODULE_TEXLIST(0x0ca54e0c)},
    {"CHAO_HYOUJI", MODULE_TEXLIST(0x0ca54f1c)},
    {"GOMA", &goma_texlist},
    {"USA", &usa_texlist},
    {"BANB", &banb_texlist},
    {"KOAR", &koar_texlist},
    {"OUM", &oum_texlist},
    {"KUJA", &kuja_texlist},
    {"PEN", &pen_texlist},
    {"TUBA", &tuba_texlist},
    {"WARA", &wara_texlist},
    {"ZOU", &zou_texlist},
    {"GORI", &gori_texlist},
    {"LION", &lion_texlist},
    {"MOGU", &mogu_texlist},
    {"RAKO", &rako_texlist},
    {"SUKA", &suka_texlist},
    {NULL, NULL},
};

static struct texture_pvm alife_mr_pvms[] = {
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {"CHAO", MODULE_TEXLIST(0x0ca54a5c)},
    {"AL_ICON", MODULE_TEXLIST(0x0c91d654)},
    {"CHAO_OBJECT", MODULE_TEXLIST(0x0ca54e0c)},
    {"CHAO_HYOUJI", MODULE_TEXLIST(0x0ca54f1c)},
    {"GARDEN02_OBJECT", MODULE_TEXLIST(0x0cb8f9ac)},
    {"GOMA", &goma_texlist},
    {"USA", &usa_texlist},
    {"BANB", &banb_texlist},
    {"KOAR", &koar_texlist},
    {"OUM", &oum_texlist},
    {"KUJA", &kuja_texlist},
    {"PEN", &pen_texlist},
    {"TUBA", &tuba_texlist},
    {"WARA", &wara_texlist},
    {"ZOU", &zou_texlist},
    {"GORI", &gori_texlist},
    {"LION", &lion_texlist},
    {"MOGU", &mogu_texlist},
    {"RAKO", &rako_texlist},
    {"SUKA", &suka_texlist},
    {NULL, NULL},
};

/* a garden's Chao and animals, without its objects. nothing reads this
 * list */
static struct texture_pvm alife_pvms[] = {
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {"CHAO", MODULE_TEXLIST(0x0ca54a5c)},
    {"AL_ICON", MODULE_TEXLIST(0x0c91d654)},
    {"GOMA", &goma_texlist},
    {"USA", &usa_texlist},
    {"BANB", &banb_texlist},
    {"KOAR", &koar_texlist},
    {"OUM", &oum_texlist},
    {"KUJA", &kuja_texlist},
    {"PEN", &pen_texlist},
    {"TUBA", &tuba_texlist},
    {"WARA", &wara_texlist},
    {"ZOU", &zou_texlist},
    {"GORI", &gori_texlist},
    {"LION", &lion_texlist},
    {"MOGU", &mogu_texlist},
    {"RAKO", &rako_texlist},
    {"SUKA", &suka_texlist},
    {NULL, NULL},
};

/* most stages keep their pvm lists in their modules */
static struct texture_pvm *stage_pvms[STAGE_COUNT] = {
    MODULE_PVMS(0x0c900cdc), /* 0 practice */
    MODULE_PVMS(0x0cab1348), /* 1 beach */
    MODULE_PVMS(0x0ca146b8), /* 2 windy */
    MODULE_PVMS(0x0c9bff88), /* 3 twinkle */
    highway_pvms, /* 4 highway */
    mountain_pvms, /* 5 mountain */
    sky_deck_pvms, /* 6 sky deck */
    ruin_pvms, /* 7 ruin */
    MODULE_PVMS(0x0cac1b90), /* 8 icecap */
    MODULE_PVMS(0x0cb17258), /* 9 casino */
    final_pvms, /* 10 final */
    NULL, /* 11 */
    MODULE_PVMS(0x0caa0ce0), /* 12 shelter */
    NULL, /* 13 */
    NULL, /* 14 */
    MODULE_PVMS(0x0c900208), /* 15 Chaos 0 */
    MODULE_PVMS(0x0c900220), /* 16 Chaos 2 */
    MODULE_PVMS(0x0c9001d0), /* 17 Chaos 4 */
    MODULE_PVMS(0x0c900288), /* 18 Chaos 6 */
    MODULE_PVMS(0x0c901a90), /* 19 Chaos 7 */
    MODULE_PVMS(0x0c9000d8), /* 20 eggmobile1 */
    MODULE_PVMS(0x0c9001b0), /* 21 eggmobile2 */
    MODULE_PVMS(0x0c9588d8), /* 22 eggmobile3 */
    MODULE_PVMS(0x0c900140), /* 23 eggmanrobo */
    MODULE_PVMS(0x0c9000a0), /* 24 e101 */
    MODULE_PVMS(0x0c900150), /* 25 e101 r */
    station_square_pvms, /* 26 station square */
    station_square_pvms, /* 27 station square 2 */
    station_square_pvms, /* 28 station square 3 */
    MODULE_PVMS(0x0c923a8c), /* 29 egg carrier, in ADV0100 */
    NULL, /* 30 egg carrier 2 */
    NULL, /* 31 egg carrier 3 */
    MODULE_PVMS(0x0c921a58), /* 32 egg carrier inside, in ADV0130 */
    MODULE_PVMS(0x0c902834), /* 33 mystic ruins */
    MODULE_PVMS(0x0c9001f8), /* 34 past */
    MODULE_PVMS(0x0c90a0e0), /* 35 cart */
    shooting1_pvms, /* 36 shooting1 */
    shooting2_pvms, /* 37 shooting2 */
    MODULE_PVMS(0x0c902e18), /* 38 sunaboard */
    alife_ss_pvms, /* 39 alife ss */
    alife_eg_pvms, /* 40 alife eg */
    alife_mr_pvms, /* 41 alife mr */
    MODULE_PVMS(0x0cb80ea0), /* 42 alife race */
};

/* the regular sets texture_load_regular loads, by regular_sets: the
 * regular pvms, and the shooting stages' two */
static struct texture_pvm regular_pvms[] = {
    {"OBJ_REGULAR", &obj_regular_texlist},
    {"EFF_REGULAR", &eff_regular_texlist},
    {"CON_REGULAR", &con_regular_texlist},
    {"EXTRA", &extra_texlist},
    {"HINT", &hint_texlist},
    {"EME_KIRAN", &eme_kiran_texlist},
    {NULL, &al_barria_texlist},
    {NULL, NULL},
};

static struct texture_pvm shooting_regular_pvms[] = {
    {"EFF_REGULAR", &eff_regular_texlist},
    {"CON_REGULAR", &con_regular_texlist},
    {NULL, NULL},
};

static struct texture_pvm *regular_sets[] = {
    regular_pvms,
    regular_pvms,
    shooting_regular_pvms,
};

Sint8 character_users[CHARACTER_COUNT] = {0};

static struct texture_pvm sonic_pvms[] = {
    {"SONIC", &sonic_texlist},
    {"SON_EFF", &son_eff_texlist},
    {NULL, NULL},
};

static struct texture_pvm tails_pvms[] = {
    {"MILES", &tails_texlist},
    {NULL, NULL},
};

static struct texture_pvm eggman_pvms[] = {
    {"EGGMAN", &eggman_texlist},
    {NULL, NULL},
};

static struct texture_pvm knuckles_pvms[] = {
    {"KNUCKLES", &knuckles_texlist},
    {"KNU_EFF", &knu_eff_texlist},
    {NULL, NULL},
};

static struct texture_pvm amy_pvms[] = {
    {"AMY", &amy_texlist},
    {"AMY_EFF", &amy_eff_texlist},
    {NULL, NULL},
};

static struct texture_pvm big_pvms[] = {
    {"BIG", &big_texlist},
    {"FISHING", &fishing_texlist},
    {"BIG_KAERU", &big_kaeru_texlist},
    {NULL, NULL},
};

static struct texture_pvm e102_pvms[] = {
    {"E102", &e102_texlist},
    {"E102BEAM", &e102beam_texlist},
    {"E102EFFECT", &e102effect_texlist},
    {"E102TIME", &e102time_texlist},
    {"E102HIT", &e102hit_texlist},
    {NULL, NULL},
};

static struct texture_pvm tikal_pvms[] = {
    {"TIKAL", MODULE_TEXLIST(0x0cb019d8)},
    {NULL, NULL},
};

static struct texture_pvm *character_pvms[CHARACTER_COUNT] = {
    sonic_pvms, eggman_pvms, tails_pvms, knuckles_pvms,
    tikal_pvms, amy_pvms,    e102_pvms,  big_pvms,
};

/* the acts' pvms. an act_textures takes count of them, from the one its
 * pvms points at */
static struct texture_pvm beach_act_pvms[] = {
    {"BEACH01", MODULE_TEXLIST(0x0c9121c4)},
    {"BEACH02", MODULE_TEXLIST(0x0c981d30)},
    {"BEACH03", MODULE_TEXLIST(0x0c9df3a0)},
};

static struct texture_pvm windy_act_pvms[] = {
    {"WINDY01", MODULE_TEXLIST(0x0c90fbb0)},
    {"WINDY02", MODULE_TEXLIST(0x0c948fa4)},
    {"WINDY03", MODULE_TEXLIST(0x0c956fe0)},
};

static struct texture_pvm highway_act_pvms[] = {
    {"HIGHWAY01", MODULE_TEXLIST(0x0c91aebc)},
    {"HIGHWAY02", MODULE_TEXLIST(0x0c99cf48)},
    {"HIGHWAY03", MODULE_TEXLIST(0x0c9a9c1c)},
};

static struct texture_pvm mountain_act_pvms[] = {
    {"MOUNTAIN01", MODULE_TEXLIST(0x0c91f464)},
    {"MOUNTAIN02", MODULE_TEXLIST(0x0c9aef94)},
    {"MOUNTAIN03", MODULE_TEXLIST(0x0c9fdde4)},
};

static struct texture_pvm sky_deck_act_pvms[] = {
    {"SKYDECK01", MODULE_TEXLIST(0x0c927cb0)},
    {"SKYDECK02", MODULE_TEXLIST(0x0c97ec10)},
    {"SKYDECK03", MODULE_TEXLIST(0x0c9b03a4)},
};

static struct texture_pvm ruin_act_pvms[] = {
    {"RUIN01", MODULE_TEXLIST(0x0c90df40)},
    {"RUIN02", MODULE_TEXLIST(0x0c96460c)},
    {"RUIN03", MODULE_TEXLIST(0x0c9f9b20)},
};

static struct texture_pvm icecap_act_pvms[] = {
    {"ICECAP01", MODULE_TEXLIST(0x0c91aaf0)},
    {"ICECAP02", MODULE_TEXLIST(0x0c95cb44)},
    {"ICECAP03", MODULE_TEXLIST(0x0c9c68c0)},
    {"ICECAP02", MODULE_TEXLIST(0x0c95cb44)},
};

static struct texture_pvm casino_act_pvms[] = {
    {"CASINO01", MODULE_TEXLIST(0x0c927dd4)},
    {"CASINO02", MODULE_TEXLIST(0x0c96dc70)},
    {"CASINO03", MODULE_TEXLIST(0x0c9b0b60)},
    {"CASINO04", MODULE_TEXLIST(0x0c9da09c)},
};

static struct texture_pvm station_square_act_pvms[] = {
    {"ADVSS00", MODULE_TEXLIST(0x0c916e28)},
    {"ADVSS01", MODULE_TEXLIST(0x0c975150)},
    {"SS_CASINO", MODULE_TEXLIST(0x0caa5960)},
    {"SS_TRAIN", MODULE_TEXLIST(0x0ca8c740)},
    {"ADVSS02", MODULE_TEXLIST(0x0c9c3078)},
    {"ADVSS03", MODULE_TEXLIST(0x0c9de708)},
    {"SS_BOAT", MODULE_TEXLIST(0x0ca97d54)},
    {"ADVSS04", MODULE_TEXLIST(0x0ca369c4)},
    {"ADVSS05", MODULE_TEXLIST(0x0ca57efc)},
    {"SS_TWINS", MODULE_TEXLIST(0x0caaea38)},
};

static struct texture_pvm mystic_ruins_act_pvms[] = {
    {"ADV_MR00", MODULE_TEXLIST(0x0c900914)},
    {"MR_SKY00", MODULE_TEXLIST(0x0c902768)},
    {"MR_TRAIN", MODULE_TEXLIST(0x0c902498)},
    {"ADV_MR01", MODULE_TEXLIST(0x0c900d54)},
    {"MR_SKY01", MODULE_TEXLIST(0x0c9027dc)},
    {"ADV_MR02", MODULE_TEXLIST(0x0c901344)},
    {"MR_SKY02", MODULE_TEXLIST(0x0c90282c)},
    {"MR_PYRAMID", MODULE_TEXLIST(0x0c90259c)},
    {"MR_FINALEGG", MODULE_TEXLIST(0x0c90231c)},
    {"ADV_MR03", MODULE_TEXLIST(0x0c90182c)},
};

static struct texture_pvm past_act_pvms[] = {
    {"PAST00", MODULE_TEXLIST(0x0c92b078)},
    {"MR_SKY02", MODULE_TEXLIST(0x0c9052ec)},
    {"PAST01", MODULE_TEXLIST(0x0c9698dc)},
    {"MR_SKY00", MODULE_TEXLIST(0x0c90529c)},
    {"PAST02", MODULE_TEXLIST(0x0c999bcc)},
    {"MR_SKY00", MODULE_TEXLIST(0x0c90529c)},
};

static struct act_textures beach_acts[] = {
    {STAGE_ACT(STAGE_BEACH, 0), 1, 0, &beach_act_pvms[0]},
    {STAGE_ACT(STAGE_BEACH, 1), 1, 0, &beach_act_pvms[1]},
    {STAGE_ACT(STAGE_BEACH, 2), 1, 0, &beach_act_pvms[2]},
};

static struct act_textures windy_acts[] = {
    {STAGE_ACT(STAGE_WINDY, 0), 1, 0, &windy_act_pvms[0]},
    {STAGE_ACT(STAGE_WINDY, 1), 1, 0, &windy_act_pvms[1]},
    {STAGE_ACT(STAGE_WINDY, 2), 1, 0, &windy_act_pvms[2]},
};

/* twinkle keeps its acts' pvms in its module */
static struct act_textures twinkle_acts[] = {
    {STAGE_ACT(STAGE_TWINKLE, 0), 2, 0, MODULE_PVMS(0x0c9bfff0)},
    {STAGE_ACT(STAGE_TWINKLE, 1), 5, 0, MODULE_PVMS(0x0c9c0000)},
    {STAGE_ACT(STAGE_TWINKLE, 2), 2, 0, MODULE_PVMS(0x0c9c0028)},
};

/* act_list leaves highway's acts out, so texture_load_act refuses them */
static struct act_textures highway_acts[] = {
    {STAGE_ACT(STAGE_HIGHWAY, 0), 1, 0, &highway_act_pvms[0]},
    {STAGE_ACT(STAGE_HIGHWAY, 1), 1, 0, &highway_act_pvms[1]},
    {STAGE_ACT(STAGE_HIGHWAY, 2), 1, 0, &highway_act_pvms[2]},
};

static struct act_textures mountain_acts[] = {
    {STAGE_ACT(STAGE_MOUNTAIN, 0), 1, 0, &mountain_act_pvms[0]},
    {STAGE_ACT(STAGE_MOUNTAIN, 1), 1, 0, &mountain_act_pvms[1]},
    {STAGE_ACT(STAGE_MOUNTAIN, 2), 1, 0, &mountain_act_pvms[2]},
};

static struct act_textures sky_deck_acts[] = {
    {STAGE_ACT(STAGE_SKY_DECK, 0), 1, 0, &sky_deck_act_pvms[0]},
    {STAGE_ACT(STAGE_SKY_DECK, 1), 1, 0, &sky_deck_act_pvms[1]},
    {STAGE_ACT(STAGE_SKY_DECK, 2), 1, 0, &sky_deck_act_pvms[2]},
};

static struct act_textures ruin_acts[] = {
    {STAGE_ACT(STAGE_RUIN, 0), 1, 0, &ruin_act_pvms[0]},
    {STAGE_ACT(STAGE_RUIN, 1), 1, 0, &ruin_act_pvms[1]},
    {STAGE_ACT(STAGE_RUIN, 2), 1, 0, &ruin_act_pvms[2]},
};

static struct act_textures icecap_acts[] = {
    {STAGE_ACT(STAGE_ICECAP, 0), 1, 0, &icecap_act_pvms[0]},
    {STAGE_ACT(STAGE_ICECAP, 1), 1, 0, &icecap_act_pvms[1]},
    {STAGE_ACT(STAGE_ICECAP, 2), 1, 0, &icecap_act_pvms[2]},
    {STAGE_ACT(STAGE_ICECAP, 3), 1, 0, &icecap_act_pvms[3]},
};

static struct act_textures casino_acts[] = {
    {STAGE_ACT(STAGE_CASINO, 0), 1, 0, &casino_act_pvms[0]},
    {STAGE_ACT(STAGE_CASINO, 1), 1, 0, &casino_act_pvms[1]},
    {STAGE_ACT(STAGE_CASINO, 2), 1, 0, &casino_act_pvms[2]},
    {STAGE_ACT(STAGE_CASINO, 3), 1, 0, &casino_act_pvms[3]},
};

static struct act_textures station_square_acts[] = {
    {STAGE_ACT(STAGE_STATION_SQUARE, 0), 1, 0, &station_square_act_pvms[0]},
    {STAGE_ACT(STAGE_STATION_SQUARE, 1), 3, 0, &station_square_act_pvms[1]},
    {STAGE_ACT(STAGE_STATION_SQUARE, 2), 1, 0, &station_square_act_pvms[4]},
    {STAGE_ACT(STAGE_STATION_SQUARE, 3), 2, 0, &station_square_act_pvms[5]},
    {STAGE_ACT(STAGE_STATION_SQUARE, 4), 1, 0, &station_square_act_pvms[7]},
    {STAGE_ACT(STAGE_STATION_SQUARE, 5), 2, 0, &station_square_act_pvms[8]},
};

/* the egg carrier's acts, outside and inside, with their pvms in ADV0100
 * and ADV0130 */
static struct act_textures adv0100_acts[] = {
    {STAGE_ACT(STAGE_EGG_CARRIER, 0), 1, 0, MODULE_PVMS(0x0c923b14)},
    {STAGE_ACT(STAGE_EGG_CARRIER, 1), 1, 0, MODULE_PVMS(0x0c923b1c)},
    {STAGE_ACT(STAGE_EGG_CARRIER, 2), 1, 0, MODULE_PVMS(0x0c923b24)},
    {STAGE_ACT(STAGE_EGG_CARRIER, 3), 1, 0, MODULE_PVMS(0x0c923b2c)},
    {STAGE_ACT(STAGE_EGG_CARRIER, 4), 1, 0, MODULE_PVMS(0x0c923b34)},
    {STAGE_ACT(STAGE_EGG_CARRIER, 5), 2, 0, MODULE_PVMS(0x0c923b3c)},
    {STAGE_ACT(STAGE_EGG_CARRIER, 6), 1, 0, MODULE_PVMS(0x0c923b4c)},
};

static struct act_textures adv0130_acts[] = {
    {STAGE_ACT(STAGE_EGG_CARRIER_INSIDE, 0), 1, 0, MODULE_PVMS(0x0c921a98)},
    {STAGE_ACT(STAGE_EGG_CARRIER_INSIDE, 1), 1, 0, MODULE_PVMS(0x0c921aa0)},
    {STAGE_ACT(STAGE_EGG_CARRIER_INSIDE, 2), 2, 0, MODULE_PVMS(0x0c921aa8)},
    {STAGE_ACT(STAGE_EGG_CARRIER_INSIDE, 3), 1, 0, MODULE_PVMS(0x0c921ab8)},
    {STAGE_ACT(STAGE_EGG_CARRIER_INSIDE, 4), 2, 0, MODULE_PVMS(0x0c921ac0)},
    {STAGE_ACT(STAGE_EGG_CARRIER_INSIDE, 5), 1, 0, MODULE_PVMS(0x0c921ad0)},
};

static struct act_textures mystic_ruins_acts[] = {
    {STAGE_ACT(STAGE_MYSTIC_RUINS, 0), 3, 0, &mystic_ruins_act_pvms[0]},
    {STAGE_ACT(STAGE_MYSTIC_RUINS, 1), 2, 0, &mystic_ruins_act_pvms[3]},
    {STAGE_ACT(STAGE_MYSTIC_RUINS, 2), 4, 0, &mystic_ruins_act_pvms[5]},
    {STAGE_ACT(STAGE_MYSTIC_RUINS, 3), 1, 0, &mystic_ruins_act_pvms[9]},
};

static struct act_textures past_acts[] = {
    {STAGE_ACT(STAGE_PAST, 0), 2, 0, &past_act_pvms[0]},
    {STAGE_ACT(STAGE_PAST, 1), 2, 0, &past_act_pvms[2]},
    {STAGE_ACT(STAGE_PAST, 2), 2, 0, &past_act_pvms[4]},
};

/* the acts texture_load_act knows, to a null */
static struct act_textures *act_list[] = {
    &beach_acts[0],
    &beach_acts[1],
    &beach_acts[2],
    &windy_acts[0],
    &windy_acts[1],
    &windy_acts[2],
    &twinkle_acts[0],
    &twinkle_acts[1],
    &twinkle_acts[2],
    &mountain_acts[0],
    &mountain_acts[1],
    &mountain_acts[2],
    &sky_deck_acts[0],
    &sky_deck_acts[1],
    &sky_deck_acts[2],
    &ruin_acts[0],
    &ruin_acts[1],
    &ruin_acts[2],
    &icecap_acts[0],
    &icecap_acts[1],
    &icecap_acts[2],
    &icecap_acts[3],
    &casino_acts[0],
    &casino_acts[1],
    &casino_acts[2],
    &casino_acts[3],
    &shelter_acts[0],
    &shelter_acts[1],
    &shelter_acts[2],
    &shelter_acts[3],
    /* Chaos 7's act 0: chaos7_act_textures, in B_CHAOS7 */
    MODULE_ACT(0x0c901ad0),
    &station_square_acts[0],
    &station_square_acts[1],
    &station_square_acts[2],
    &station_square_acts[3],
    &station_square_acts[4],
    &station_square_acts[5],
    &mystic_ruins_acts[0],
    &mystic_ruins_acts[1],
    &mystic_ruins_acts[2],
    &mystic_ruins_acts[3],
    &past_acts[0],
    &past_acts[1],
    &past_acts[2],
    &adv0100_acts[0],
    &adv0100_acts[1],
    &adv0100_acts[2],
    &adv0100_acts[3],
    &adv0100_acts[4],
    &adv0100_acts[5],
    &adv0100_acts[6],
    &adv0130_acts[0],
    &adv0130_acts[1],
    &adv0130_acts[2],
    &adv0130_acts[3],
    &adv0130_acts[4],
    &adv0130_acts[5],
    NULL,
};

void texture_load_character(int character)
{
    struct texture_pvm *pvm = character_pvms[character];

    if (pvm == NULL) {
        return;
    }
    if (character_users[character] == 0) {
        for (; pvm->texlist != NULL; pvm++) {
            if (pvm->name == NULL) {
                njLoadTexture(pvm->texlist);
            } else {
                texture_load_pvm(pvm->name, pvm->texlist);
            }
        }
    }
    character_users[character]++;
}

void texture_release_character(int character)
{
    struct texture_pvm *pvm = character_pvms[character];

    if (pvm == NULL) {
        return;
    }
    if (character_users[character] == 1) {
        for (; pvm->texlist != NULL; pvm++) {
            njReleaseTexture(pvm->texlist);
        }
    }
    if (character_users[character] > 0) {
        character_users[character]--;
    }
}

void texture_release_characters(void)
{
    struct texture_pvm *pvm;
    int i;

    for (i = 0; i < CHARACTER_COUNT; i++) {
        pvm = character_pvms[i];
        /* @hack redundant: an else after the continue, which loads the users
         * byte into R2 (MOV.B @R12,R2 at 0x8c09ea74); without it, R3 takes
         * it and 4 of 54 words differ */
        if (pvm == NULL) {
            continue;
        } else if (character_users[i] > 0) {
            for (; pvm->texlist != NULL; pvm++) {
                njReleaseTexture(pvm->texlist);
            }
        }
        character_users[i] = 0;
    }
}

void texture_load_regular(int set)
{
    NJS_TEXLIST **slot = &texlist_slots[SLOT_REGULAR];
    struct texture_pvm *pvm = regular_sets[set];
    NJS_TEXLIST *texlist;
    NJS_TEXLIST **list;

    if (pvm == NULL) {
        *slot = NULL;
    } else {
        texlist = pvm->texlist;
        *slot = texlist;
        if (texlist != NULL) {
            if (pvm->name != NULL) {
                texture_load_pvm(pvm->name, texlist);
            } else {
                njLoadTexture(texlist);
            }
        }
        pvm++;
        /* @bug logic: list never advances, so each list after the
         * first goes to regular_texlists[0] (MOV.L R13,@R11 at 0x8c09eb3e),
         * and the null after the loop clears it (0x8c09eb48).
         * texture_release_regular then finds none of them */
        list = regular_texlists;
        texlist = pvm->texlist;
        while (texlist != NULL) {
            if (pvm->name != NULL) {
                texture_load_pvm(pvm->name, texlist);
            } else {
                njLoadTexture(texlist);
            }
            *list = texlist;
            pvm++;
            texlist = pvm->texlist;
        }
        *list = NULL;
    }
}

void texture_release_regular(void)
{
    NJS_TEXLIST **texlist;

    if (texlist_slots[SLOT_REGULAR] != NULL) {
        njReleaseTexture(texlist_slots[SLOT_REGULAR]);
        /* @hack spelling: *texlist_slots, which has SHC load the array's
         * address again, into R3, for the store (0x8c09eb6e); as
         * texlist_slots[SLOT_REGULAR], it stores through R14, which holds it
         * already, and 19 of 30 words differ */
        *texlist_slots = NULL;
    }
    for (texlist = regular_texlists; *texlist != NULL; texlist++) {
        njReleaseTexture(*texlist);
        *texlist = NULL;
    }
}

void texture_load_stage(void)
{
    struct texture_pvm *pvm;
    Uint16 stage;
    int i;

    stage = STAGE_OF(stage_get_stage_act());
    pvm = stage_pvms[stage];
    /* once the current character's story has sunk the egg carrier, the
     * stage takes ADV0100's other list */
    if (stage == STAGE_EGG_CARRIER) {
        if (egg_carrier_has_sunk()) {
            pvm = egg_carrier_sea_pvms;
        }
    }
    if (pvm == NULL) {
        for (i = SLOT_STAGE; i < TEXLIST_SLOTS; i++) {
            texlist_slots[i] = NULL;
        }
        stage_texlists[0] = NULL;
    } else {
        for (i = SLOT_STAGE; i < TEXLIST_SLOTS; i++) {
            texlist_slots[i] = pvm->texlist;
            if (pvm->texlist != NULL) {
                if (pvm->name != NULL) {
                    texture_load_pvm(pvm->name, pvm->texlist);
                } else {
                    njLoadTexture(pvm->texlist);
                }
            }
            pvm++;
        }
        for (i = 0; pvm->texlist != NULL; i++) {
            stage_texlists[i] = pvm->texlist;
            if (pvm->name != NULL) {
                texture_load_pvm(pvm->name, pvm->texlist);
            } else {
                njLoadTexture(pvm->texlist);
            }
            pvm++;
        }
        stage_texlists[i] = NULL;
    }
}

Bool texture_set_regular(void)
{
    /* @hack spelling: the return of FALSE in an else, which puts njSetTexture's
     * call ahead of it (0x8c09ed2c); returned early, the call follows the
     * return and 14 of 19 words differ */
    if (texlist_slots[SLOT_REGULAR] != NULL) {
        njSetTexture(texlist_slots[SLOT_REGULAR]);
    } else {
        return FALSE;
    }
    return TRUE;
}

Bool texture_has_primary_stage_texlist(void)
{
    if (texlist_slots[SLOT_STAGE] != NULL) {
        return TRUE;
    }
    return FALSE;
}

Bool texture_set_stage(void)
{
    /* @hack spelling: the return of FALSE in an else, which puts njSetTexture's
     * call ahead of it (0x8c09ed92). returned early, the call follows the
     * return and 15 of 19 words differ */
    if (texlist_slots[SLOT_STAGE] != NULL) {
        njSetTexture(texlist_slots[SLOT_STAGE]);
    } else {
        return FALSE;
    }
    return TRUE;
}

void texture_release_stage(void)
{
    int i;

    for (i = SLOT_STAGE; i < TEXLIST_SLOTS; i++) {
        if (texlist_slots[i] == NULL) {
            continue;
        }
        njReleaseTexture(texlist_slots[i]);
        texlist_slots[i] = NULL;
    }
    i = 0;
    /* @bug logic: i never advances, so the loop releases and clears
     * stage_texlists[0] where it means each entry in turn, and the cleared
     * entry ends it (0x8c09ede8 to 0x8c09edf2). the stage's later lists
     * stay loaded */
    while (stage_texlists[i] != NULL) {
        njReleaseTexture(stage_texlists[i]);
        stage_texlists[i] = NULL;
    }
}

Bool texture_load_act(Uint16 stage_act)
{
    struct act_textures **act;
    struct act_textures *textures;
    struct texture_pvm *pvm;
    int i;

    for (act = act_list; *act != NULL; act++) {
        textures = *act;
        if (textures->stage_act == stage_act) {
            if (textures->users == 0) {
                pvm = textures->pvms;
                /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
                for (i = textures->count; i > 0; i--) {
                    texture_load_pvm(pvm->name, pvm->texlist);
                    pvm++;
                }
            }
            textures->users++;
            return TRUE;
        }
    }
    return FALSE;
}

Bool texture_act_in_use(Uint16 stage_act)
{
    struct act_textures **act;

    for (act = act_list; *act != NULL; act++) {
        if ((*act)->stage_act == stage_act) {
            /* @hack redundant: the else after the return of TRUE, which has the
             * loop read the next entry into R3 (MOV.L @R5,R3 at 0x8c09eea2);
             * without it, R2 takes it and 2 of 23 words differ */
            if ((*act)->users > 0) {
                return TRUE;
            } else {
                return FALSE;
            }
        }
    }
    return FALSE;
}

Bool texture_release_act(Uint16 stage_act)
{
    struct act_textures **act;
    struct act_textures *textures;
    struct texture_pvm *pvm;
    int i;

    for (act = act_list; *act != NULL; act++) {
        textures = *act;
        if (textures->stage_act == stage_act) {
            textures->users--;
            if (textures->users == 0) {
                pvm = textures->pvms;
                /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
                for (i = textures->count; i > 0; i--) {
                    njReleaseTexture(pvm->texlist);
                    pvm++;
                }
            } else if (textures->users < 0) {
                for (;;) {
                }
            }
            return TRUE;
        }
    }
    return FALSE;
}

void texture_release_acts(void)
{
    struct act_textures **act;
    struct act_textures *textures;
    struct texture_pvm *pvm;
    int i;

    for (act = act_list; *act != NULL; act++) {
        textures = *act;
        if (textures->users != 0) {
            textures->users = 0;
            pvm = textures->pvms;
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            for (i = textures->count; i > 0; i--) {
                njReleaseTexture(pvm->texlist);
                pvm++;
            }
        }
    }
}

void texture_load_pvm(char *name, NJS_TEXLIST *texlist)
{
    NJS_TEXLIST loaded;
    char names[PVM_TEXTURES * PVM_NAME_SIZE];
    NJS_TEXNAME *textures;
    int count;
    int i;

    njSetPvmTextureList(&loaded, pvm_textures, names, PVM_TEXTURES);
    load_prs(name, &loaded);
    count = texlist->nbTexture;
    textures = texlist->textures;
    for (i = 0; i < count; i++) {
        textures[i].texaddr = pvm_textures[i].texaddr;
        textures[i].attr = pvm_textures[i].attr;
    }
}

/* load name.PRS into texlist, as texture_load_pvm says. -1 when every
 * open fails, and njLoadTexturePvmMemory's result otherwise */
static Sint32 load_prs(char *name, NJS_TEXLIST *texlist)
{
    char path[256];
    Sint32 size;
    GDFS gdfs;
    Sint32 sectors;
    Uint8 *file;
    int tries;
    int i;

    sprintf(path, "%s.PRS", name);
    for (tries = TRIES; tries > 0; tries--) {
        gdfs = gdFsOpen(path, NULL);
        if (gdfs != NULL) {
            break;
        }
        for (i = 0; i < OPEN_WAIT; i++) {
        }
    }
    if (gdfs == NULL) {
        return -1;
    }
    gdFsGetFileSize(gdfs, &size);
    sectors = SECTORS(size);
    file = WORK_AREA_END - FILE_ROOM(size);
    for (tries = TRIES; tries > 0; tries--) {
        if (gdFsRead(gdfs, sectors, file) == GDD_ERR_OK) {
            break;
        }
    }
    gdFsClose(gdfs);
    prs_decompress(file, WORK_AREA);
    return njLoadTexturePvmMemory(WORK_AREA, texlist);
}

Sint32 texture_load_prs_file(char *name, NJS_TEXLIST *texlist)
{
    Sint32 size;
    GDFS gdfs;
    Sint32 sectors;
    Uint8 *file;
    int tries;
    int i;

    for (tries = TRIES; tries > 0; tries--) {
        gdfs = gdFsOpen(name, NULL);
        if (gdfs != NULL) {
            break;
        }
        for (i = 0; i < OPEN_WAIT; i++) {
        }
    }
    if (gdfs == NULL) {
        return -1;
    }
    gdFsGetFileSize(gdfs, &size);
    sectors = SECTORS(size);
    file = WORK_AREA - FILE_ROOM(size);
    for (tries = TRIES; tries > 0; tries--) {
        if (gdFsRead(gdfs, sectors, file) == GDD_ERR_OK) {
            break;
        }
    }
    gdFsClose(gdfs);
    prs_decompress(file, MODULE_AREA);
    return njLoadTexturePvmMemory(MODULE_AREA, texlist);
}
