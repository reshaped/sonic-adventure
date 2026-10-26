/* texture lists: the effects', the objects' and the hud's regular textures,
 * and the screens' own */
#ifndef SYSTEM_TEXLISTS_H
#define SYSTEM_TEXLISTS_H
#include <ninja.h>
#include <sg_xpt.h>

#define EFF_REGULAR_TEXTURES 138
#define CON_REGULAR_TEXTURES 56

/* the names of EFF_REGULAR's and CON_REGULAR's textures. texlists
 * elsewhere, such as snow_texlist, point into these arrays instead of
 * naming the textures again */
extern NJS_TEXNAME eff_regular_textures[EFF_REGULAR_TEXTURES];
extern NJS_TEXLIST eff_regular_texlist;
extern NJS_TEXNAME con_regular_textures[CON_REGULAR_TEXTURES];
extern NJS_TEXLIST con_regular_texlist;
/* the OBJ_REGULAR textures */
extern NJS_TEXLIST obj_regular_texlist;
/* the screens' lists. each screen loads the lists it shows from their
 * pvms, and releases them after */
extern NJS_TEXLIST loading_texlist;
extern NJS_TEXLIST game_over_texlist;
extern NJS_TEXLIST coming_soon_texlist;
extern NJS_TEXLIST score_act_texlist;
extern NJS_TEXLIST score_back_texlist;
extern NJS_TEXLIST score_board_texlist;
extern NJS_TEXLIST score_boss_texlist;
extern NJS_TEXLIST score_cart_texlist;
extern NJS_TEXLIST score_mole_texlist;
extern NJS_TEXLIST score_result_texlist;
extern NJS_TEXLIST score_shoot_texlist;

/* indexes in game_over_texlist, each beside the name of the GAMEOVER
 * texture it selects */
#define GAME_OVER_BANNER 1 /* gameover */
#define GAME_OVER_CURSOR 2 /* casol */
#define GAME_OVER_BACK 3 /* sphe0 */
#define GAME_OVER_FRAME 4 /* sphe1 */
#define GAME_OVER_CONTINUE 5 /* npa_saikai */
#define GAME_OVER_QUIT 6 /* npa_stop */

#endif
