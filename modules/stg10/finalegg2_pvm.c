/* the name of the pvm act 1's land loads its textures from. the land is
 * generated data, which can point at this array's symbol but not at a string
 * literal. SHC aligns an array to 1, and a literal to 4. as the only constant
 * in its object file, this array still builds as the literal would */
#include "stg10/finalegg2_pvm.h"

const char finalegg2_pvm[10] = {
#include "stg10/finalegg2_pvm/finalegg2_pvm.inc"
};
