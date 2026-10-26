/* the pvms the act and the fight load */
#include "b_chaos7/fight_pvms.h"
#include "b_chaos7/ball.h"
#include "b_chaos7/chaos7_textures.h"
#include "b_chaos7/damage.h"
#include "b_chaos7/death.h"
#include "b_chaos7/ray.h"
#include "b_chaos7/super.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 15
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 14
#define TEXT_4_LENGTH 14
#define TEXT_5_LENGTH 10
#define TEXT_6_LENGTH 18
#define TEXT_7_LENGTH 16
#define TEXT_8_LENGTH 18
#include "b_chaos7/fight_pvms/text.inc"

struct texture_pvm chaos7_act_pvms[5] = {
    {TEXT_1, &chaos7_surface_texlist}, {TEXT_2, &chaos7_texlist},
    {TEXT_3, &chaos7_damage_texlist},  {TEXT_4, &chaos7_breath_texlist},
    {TEXT_5, &super_sonic_texlist},
};
struct texture_pvm chaos7_fight_pvms[5] = {
    {TEXT_4, &chaos7_breath_texlist},
    {TEXT_6, &ball_sprite_texlist},
    {TEXT_7, &ball_splash_texlist},
    {TEXT_8, &chaos7_death_texlist},
    {NULL, NULL},
};
