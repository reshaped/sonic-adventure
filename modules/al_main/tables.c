/* the Chao's collision shapes, and what each kind of animal gives a Chao:
 * the chance of each part, and its skill bonuses */
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "animal/animal.h"
#include "chao/chao.h"
#include "collision/collision.h"

struct collision_shape chao_collisions[3] = {
#include "al_main/tables/chao_collisions.inc"
};
Float animal_part_chances[ANIMAL_KINDS][CHAO_PARTS] = {
#include "al_main/tables/animal_part_chances.inc"
};
Sint16 animal_skill_bonuses[ANIMAL_LION + 1][4] = {
#include "al_main/tables/animal_skill_bonuses.inc"
};
