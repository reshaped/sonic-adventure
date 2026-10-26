/* the pvms the race loads */
#include "al_race/pvms.h"
#include "al_main/icon.h"
#include "al_main/models.h"
#include "al_race/texlists.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 11
#define TEXT_2_LENGTH 4
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 11
#include "al_race/pvms/text.inc"

/* the first four rows fill the stage's four slots, and rows 1 to 3 leave
 * theirs empty. the rows after them load too, up to the null texlist */
#pragma section HEAD
struct texture_pvm race_pvms[8] = {
    {TEXT_1, &dat_0cbc089c},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {TEXT_2, &alife_chao_texlist},
    {TEXT_3, &alife_icon_texlist},
    {TEXT_4, &alife_window_texlist},
    {NULL, NULL},
};
#pragma section
