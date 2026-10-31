/* particle pools: those every stage makes, and those each stage adds */
#include <sg_xpt.h>

#include "adv01obj/pools.h"
#include "al_race/pools.h"
#include "b_chaos6/snow.h"
#include "effect/bomb.h"
#include "effect/dust.h"
#include "effect/explosion.h"
#include "effect/flame.h"
#include "effect/hit.h"
#include "effect/panlight.h"
#include "effect/particle.h"
#include "effect/pools.h"
#include "effect/snow.h"
#include "effect/sparkle.h"
#include "effect/splash.h"
#include "object/rocket.h"
#include "sboard/pools.h"
#include "stage/stage.h"
#include "stg03/coaster.h"
#include "stg05/rock_burst.h"
#include "stg08/snowman.h"
#include "stg09/water_smoke.h"
#include "stg10/pom.h"
#include "stg10/puff.h"

static struct particle_pool_spec *common_pools[] = {
    &flame_pool,
    &dust_pool,
    &dust_glow_pool,
    &explosion_pool,
    &bomb_pool,
    &panlight_pool,
    &sparkle_pool,
    &hit_pool,
    &rocket_thrust_pool,
    &splash_pool,
    &snow_pool,
    &lightning_pool,
    NULL,
};

static struct particle_pool_spec *beach_pools[] = {NULL};
static struct particle_pool_spec *twinkle_pools[] = {
    &twinkle_coaster_sparkle_pool, NULL};
static struct particle_pool_spec *highway_pools[] = {NULL};
/* STG05's rock_burst_dust_pool */
static struct particle_pool_spec *mountain_pools[] = {&rock_burst_dust_pool,
                                                      NULL};
/* STG08's snowman_snow_pool */
static struct particle_pool_spec *icecap_pools[] = {&snowman_snow_pool, NULL};
/* STG09's water_smoke_pool */
static struct particle_pool_spec *casino_pools[] = {&water_smoke_pool, NULL};
/* STG12's final_puff_pool */
static struct particle_pool_spec *shelter_pools[] = {&final_puff_pool, NULL};
static struct particle_pool_spec *alife_ss_pools[] = {&sparkle_pool, NULL};
/* B_CHAOS6's chaos6_snow_pool */
static struct particle_pool_spec *chaos6_pools[] = {&chaos6_snow_pool, NULL};
static struct particle_pool_spec *chaos7_pools[] = {&splash_pool, NULL};

/* each stage's own pools, or null for none */
static struct particle_pool_spec **stage_pools[STAGE_COUNT] = {
    NULL, /* STAGE_PRACTICE */
    beach_pools, /* STAGE_BEACH */
    NULL, /* STAGE_WINDY */
    twinkle_pools, /* STAGE_TWINKLE */
    highway_pools, /* STAGE_HIGHWAY */
    mountain_pools, /* STAGE_MOUNTAIN */
    NULL, /* STAGE_SKY_DECK */
    NULL, /* STAGE_RUIN */
    icecap_pools, /* STAGE_ICECAP */
    casino_pools, /* STAGE_CASINO */
    final_pools, /* STAGE_FINAL: STG10's final_pools */
    NULL, /* 11, unnamed */
    shelter_pools, /* STAGE_SHELTER */
    NULL, /* 13, unnamed */
    NULL, /* 14, unnamed */
    NULL, /* STAGE_CHAOS0 */
    NULL, /* STAGE_CHAOS2 */
    NULL, /* STAGE_CHAOS4 */
    chaos6_pools, /* STAGE_CHAOS6 */
    chaos7_pools, /* STAGE_CHAOS7 */
    NULL, /* STAGE_EGGMOBILE1 */
    NULL, /* STAGE_EGGMOBILE2 */
    NULL, /* STAGE_EGGMOBILE3 */
    NULL, /* STAGE_EGGMANROBO */
    NULL, /* STAGE_E101 */
    NULL, /* STAGE_E101_R */
    NULL, /* STAGE_STATION_SQUARE */
    NULL, /* STAGE_STATION_SQUARE_2 */
    NULL, /* STAGE_STATION_SQUARE_3 */
    /* STAGE_EGG_CARRIER: ADV01OBJ's egg_carrier_pools */
    egg_carrier_pools,
    NULL, /* STAGE_EGG_CARRIER_2 */
    NULL, /* STAGE_EGG_CARRIER_3 */
    /* STAGE_EGG_CARRIER_INSIDE: the same list, as ADV01OBJ loads in this
     * stage too */
    egg_carrier_pools,
    NULL, /* STAGE_MYSTIC_RUINS */
    NULL, /* STAGE_PAST */
    NULL, /* STAGE_CART */
    NULL, /* STAGE_SHOOTING1 */
    NULL, /* STAGE_SHOOTING2 */
    sunaboard_pools, /* STAGE_SUNABOARD: SBOARD's sunaboard_pools */
    alife_ss_pools, /* STAGE_ALIFE_SS */
    NULL, /* STAGE_ALIFE_EG */
    NULL, /* STAGE_ALIFE_MR */
    race_pools, /* STAGE_ALIFE_RACE: AL_RACE's race_pools */
};

static void create_pools(struct particle_pool_spec **list)
{
    struct particle_pool_spec *spec;

    for (; *list != NULL; list++) {
        spec = *list;
        spec->pool = particle_pool_create(spec->anim, spec->size);
        if (spec->init != NULL) {
            spec->init(spec->pool);
        }
    }
}

void particle_create_pools(void)
{
    Uint16 stage;
    struct particle_pool_spec **list;

    create_pools(common_pools);
    stage = STAGE_OF(stage_get_stage_act());
    /* @hack spelling: list assigned in the test, which keeps the stage in R5
     * (MOV R0,R5 at 0x8c0a3d92); assigned before the test, 10 of 24 words
     * differ */
    if ((list = stage_pools[stage])) {
        create_pools(list);
    }
}
