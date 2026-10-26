/* each stage's kinds of animal, and making one */
#include <sg_xpt.h>
#include <stdlib.h>

#include "animal/animal.h"
#include "animal/kinds.h"
#include "system/task.h"
#include "system/util.h"

/* the stages the table covers, STAGE_PRACTICE to STAGE_SHELTER */
#define STAGES 13
/* each stage's kinds: four, then the rare one */
#define STAGE_KINDS 5
#define RARE 4

/* @hack type: int, not stage.h's Uint16, which SHC shifts down by SHAD
 * without extending it first (0x8c0161d4, 0x8c01624a); as stage.h declares
 * it, EXTU.W and SHLR8, and 5 of 92 words differ */
extern int stage_get_stage_act(void);

static void (*kind_steps[ANIMAL_KINDS])(struct task *task) = {
    animal_start_goma, animal_start_pen,  animal_start_rako, animal_start_kuja,
    animal_start_tuba, animal_start_oum,  animal_start_banb, animal_start_usa,
    animal_start_wara, animal_start_gori, animal_start_lion, animal_start_zou,
    animal_start_mogu, animal_start_koar, animal_start_suka,
};

/* each stage's kinds, by stage number. a row of 0s makes every kind there
 * ANIMAL_GOMA */
static Sint8 stage_kinds[STAGES][STAGE_KINDS] = {
    /* STAGE_PRACTICE */
    {0, 0, 0, 0, 0},
    /* STAGE_BEACH */
    {ANIMAL_GOMA, ANIMAL_PEN, ANIMAL_RAKO, ANIMAL_USA, ANIMAL_KOAR},
    /* STAGE_WINDY */
    {ANIMAL_OUM, ANIMAL_TUBA, ANIMAL_KUJA, ANIMAL_GOMA, ANIMAL_MOGU},
    /* STAGE_TWINKLE */
    {ANIMAL_BANB, ANIMAL_WARA, ANIMAL_USA, ANIMAL_ZOU, ANIMAL_KOAR},
    /* STAGE_HIGHWAY */
    {ANIMAL_BANB, ANIMAL_WARA, ANIMAL_USA, ANIMAL_TUBA, ANIMAL_KOAR},
    /* STAGE_MOUNTAIN */
    {ANIMAL_GORI, ANIMAL_ZOU, ANIMAL_LION, ANIMAL_OUM, ANIMAL_MOGU},
    /* STAGE_SKY_DECK */
    {ANIMAL_OUM, ANIMAL_TUBA, ANIMAL_KUJA, ANIMAL_LION, ANIMAL_SUKA},
    /* STAGE_RUIN */
    {ANIMAL_GORI, ANIMAL_ZOU, ANIMAL_LION, ANIMAL_PEN, ANIMAL_MOGU},
    /* STAGE_ICECAP */
    {ANIMAL_GOMA, ANIMAL_PEN, ANIMAL_RAKO, ANIMAL_GORI, ANIMAL_MOGU},
    /* STAGE_CASINO */
    {ANIMAL_OUM, ANIMAL_TUBA, ANIMAL_KUJA, ANIMAL_USA, ANIMAL_KOAR},
    /* STAGE_FINAL */
    {ANIMAL_BANB, ANIMAL_WARA, ANIMAL_USA, ANIMAL_GORI, ANIMAL_MOGU},
    /* 11, which no stage has */
    {0, 0, 0, 0, 0},
    /* STAGE_SHELTER */
    {ANIMAL_GOMA, ANIMAL_PEN, ANIMAL_RAKO, ANIMAL_BANB, ANIMAL_SUKA},
};

void animal_create_random(int unused, Float x, Float y, Float z)
{
    Uint16 stage;
    Uint16 slot;

    stage = (Uint8)(stage_get_stage_act() >> 8);
    if (rand() * RAND_SCALE < 0.1F) {
        slot = RARE;
    } else {
        slot = rand() * RAND_SCALE * 3.99F;
    }
    animal_create(x, y, z, kind_steps[stage_kinds[stage][slot]]);
}

void animal_create_rare(Float x, Float y, Float z)
{
    Uint16 stage;

    stage = (Uint8)(stage_get_stage_act() >> 8);
    animal_create(x, y, z, kind_steps[stage_kinds[stage][RARE]]);
}
