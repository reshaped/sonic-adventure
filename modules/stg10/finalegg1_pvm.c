/* the name of the pvm act 0's land loads its textures from. the land is
 * generated data, which can point at this array's symbol but not at a string
 * literal. SHC aligns an array to 1, and a literal to 4. as the only constant
 * in its object file, this array still builds as the literal would */
#include "stg10/finalegg1_pvm.h"

const char finalegg1_pvm[10] = {
#include "stg10/finalegg1_pvm/finalegg1_pvm.inc"
};
