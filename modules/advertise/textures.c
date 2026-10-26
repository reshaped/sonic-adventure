/* the menus' textures: each screen's PVMs, loaded as the screen opens and
 * released once it has closed, but for those the next screen needs */
#include <ninja.h>
#include <sg_xpt.h>

#include "advertise/screen.h"
#include "advertise/textures.h"
#include "draw/screen.h"
#include "hud/menu.h"
#include "system/texture.h"

/* the screens that have a list of PVMs: every one before SCREEN_CLOSE */
#define SCREENS SCREEN_CLOSE
/* room for any screen's list, whose longest holds 12 PVMs and TEX_END */
#define SCREEN_LIST_SIZE 16

struct texture_file {
    char *name;
    NJS_TEXLIST *texlist;
};

const Uint8 stage_name_textures[] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
};

static const struct texture_file texture_files[TEX_END] = {
    {"ava_back", &ava_back_texlist},
    {"adv_window", &screen_box_texlist},
    {"ava_chsel", &ava_chsel_texlist},
    {"ava_suuji", &ava_suuji_texlist},
    {"ava_csr", &menu_row_texlist},
    {"ava_dlg", &ava_dlg_texlist},
    {"ava_fsdlg", &ava_fsdlg_texlist},
    {"ava_filesel", &ava_filesel_texlist},
    {"ava_fscmn", &ava_fscmn_texlist},
    {"ava_sndtest", &ava_sndtest_texlist},
    {"ava_square", &menu_column_texlist},
    {"ava_stnam", &ava_stnam_texlist},
    {"ava_trialactsel", &ava_trialactsel_texlist},
    {"ava_vmssel", &ava_vmssel_texlist},
    {"b_chnam", &b_chnam_texlist},
    {"m_chnam", &m_chnam_texlist},
    {"tx_chnam", &tx_chnam_texlist},
    {"ava_gtitle0", &ava_gtitle0_texlist},
    {"ava_title", &ava_title_texlist},
    {"ava_option", &ava_option_texlist},
    {"ava_emblem", &ava_emblem_texlist},
    {"ava_new16no", &ava_new16no_texlist},
    {"ava_san", &ava_san_texlist},
};
static const int no_textures[] = {
    TEX_END,
};
static const int character_textures[] = {
    TEX_AVA_BACK, TEX_ADV_WINDOW, TEX_AVA_CSR,   TEX_AVA_DLG,   TEX_B_CHNAM,
    TEX_M_CHNAM,  TEX_AVA_SUUJI,  TEX_AVA_CHSEL, TEX_AVA_FSCMN, TEX_END,
};
static const int file_textures[] = {
    TEX_AVA_BACK,  TEX_ADV_WINDOW, TEX_AVA_SQUARE, TEX_AVA_CSR, TEX_AVA_DLG,
    TEX_AVA_FSDLG, TEX_AVA_EMBLEM, TEX_AVA_SUUJI,  TEX_M_CHNAM, TEX_AVA_FILESEL,
    TEX_AVA_FSCMN, TEX_AVA_STNAM,  TEX_END,
};
static const int trial_stage_textures[] = {
    TEX_AVA_BACK, TEX_ADV_WINDOW, TEX_AVA_SQUARE,      TEX_AVA_CSR,
    TEX_AVA_DLG,  TEX_AVA_SAN,    TEX_AVA_EMBLEM,      TEX_AVA_NEW16NO,
    TEX_B_CHNAM,  TEX_TX_CHNAM,   TEX_AVA_TRIALACTSEL, TEX_AVA_STNAM,
    TEX_END,
};
static const int mini_game_textures[] = {
    TEX_AVA_BACK, TEX_ADV_WINDOW, TEX_AVA_SQUARE,      TEX_AVA_CSR,
    TEX_AVA_DLG,  TEX_AVA_SAN,    TEX_AVA_EMBLEM,      TEX_AVA_NEW16NO,
    TEX_B_CHNAM,  TEX_TX_CHNAM,   TEX_AVA_TRIALACTSEL, TEX_AVA_STNAM,
    TEX_END,
};
static const int drive_textures[] = {
    TEX_AVA_BACK, TEX_AVA_VMSSEL, TEX_ADV_WINDOW,
    TEX_AVA_CSR,  TEX_AVA_DLG,    TEX_END,
};
static const int sound_test_textures[] = {
    TEX_AVA_BACK,    TEX_ADV_WINDOW,  TEX_AVA_SQUARE, TEX_AVA_SAN,
    TEX_AVA_NEW16NO, TEX_AVA_SNDTEST, TEX_END,
};
static const int card_textures[] = {
    TEX_AVA_GTITLE0,
    TEX_ADV_WINDOW,
    TEX_END,
};
static const int mode_textures[] = {
    TEX_AVA_TITLE,
    TEX_ADV_WINDOW,
    TEX_AVA_SQUARE,
    TEX_END,
};
static const int options_textures[] = {
    TEX_AVA_BACK, TEX_ADV_WINDOW, TEX_AVA_SQUARE, TEX_AVA_CSR,
    TEX_AVA_DLG,  TEX_AVA_OPTION, TEX_END,
};
static const int game_bug_textures[] = {
    TEX_END,
};
static const int screen_none_textures[] = {
    TEX_END,
};

/* the PVMs' texlists. their entries stay empty until texture_load_pvm copies
 * the PVM's textures in */
static NJS_TEXNAME b_chnam_textures[7] = {{NULL, 0, 0}};
NJS_TEXLIST b_chnam_texlist = {
#include "advertise/textures/b_chnam_texlist.inc"
};
static NJS_TEXNAME m_chnam_textures[7] = {{NULL, 0, 0}};
NJS_TEXLIST m_chnam_texlist = {
#include "advertise/textures/m_chnam_texlist.inc"
};
static NJS_TEXNAME tx_chnam_textures[7] = {{NULL, 0, 0}};
NJS_TEXLIST tx_chnam_texlist = {
#include "advertise/textures/tx_chnam_texlist.inc"
};
static NJS_TEXNAME ava_chsel_textures[11] = {{NULL, 0, 0}};
NJS_TEXLIST ava_chsel_texlist = {
#include "advertise/textures/ava_chsel_texlist.inc"
};
static NJS_TEXNAME ava_suuji_textures[12] = {{NULL, 0, 0}};
NJS_TEXLIST ava_suuji_texlist = {
#include "advertise/textures/ava_suuji_texlist.inc"
};
static NJS_TEXNAME ava_dlg_textures[10] = {{NULL, 0, 0}};
NJS_TEXLIST ava_dlg_texlist = {
#include "advertise/textures/ava_dlg_texlist.inc"
};
static NJS_TEXNAME ava_fsdlg_textures[6] = {{NULL, 0, 0}};
NJS_TEXLIST ava_fsdlg_texlist = {
#include "advertise/textures/ava_fsdlg_texlist.inc"
};
static NJS_TEXNAME ava_stnam_textures[35] = {{NULL, 0, 0}};
NJS_TEXLIST ava_stnam_texlist = {
#include "advertise/textures/ava_stnam_texlist.inc"
};
static NJS_TEXNAME ava_trialactsel_textures[18] = {{NULL, 0, 0}};
NJS_TEXLIST ava_trialactsel_texlist = {
#include "advertise/textures/ava_trialactsel_texlist.inc"
};
static NJS_TEXNAME ava_filesel_textures[32] = {{NULL, 0, 0}};
NJS_TEXLIST ava_filesel_texlist = {
#include "advertise/textures/ava_filesel_texlist.inc"
};
static NJS_TEXNAME ava_fscmn_textures[6] = {{NULL, 0, 0}};
NJS_TEXLIST ava_fscmn_texlist = {
#include "advertise/textures/ava_fscmn_texlist.inc"
};
static NJS_TEXNAME ava_vmssel_textures[19] = {{NULL, 0, 0}};
NJS_TEXLIST ava_vmssel_texlist = {
#include "advertise/textures/ava_vmssel_texlist.inc"
};
static NJS_TEXNAME ava_sndtest_textures[16] = {{NULL, 0, 0}};
NJS_TEXLIST ava_sndtest_texlist = {
#include "advertise/textures/ava_sndtest_texlist.inc"
};
static NJS_TEXNAME ava_gtitle0_textures[8] = {{NULL, 0, 0}};
NJS_TEXLIST ava_gtitle0_texlist = {
#include "advertise/textures/ava_gtitle0_texlist.inc"
};
static NJS_TEXNAME ava_title_textures[15] = {{NULL, 0, 0}};
NJS_TEXLIST ava_title_texlist = {
#include "advertise/textures/ava_title_texlist.inc"
};
static NJS_TEXNAME ava_option_textures[28] = {{NULL, 0, 0}};
NJS_TEXLIST ava_option_texlist = {
#include "advertise/textures/ava_option_texlist.inc"
};
static NJS_TEXNAME ava_back_textures[8] = {{NULL, 0, 0}};
NJS_TEXLIST ava_back_texlist = {
#include "advertise/textures/ava_back_texlist.inc"
};
static NJS_TEXNAME ava_emblem_textures[2] = {{NULL, 0, 0}};
NJS_TEXLIST ava_emblem_texlist = {
#include "advertise/textures/ava_emblem_texlist.inc"
};
static NJS_TEXNAME ava_new16no_textures[12] = {{NULL, 0, 0}};
NJS_TEXLIST ava_new16no_texlist = {
#include "advertise/textures/ava_new16no_texlist.inc"
};
static NJS_TEXNAME ava_san_textures[2] = {{NULL, 0, 0}};
NJS_TEXLIST ava_san_texlist = {
#include "advertise/textures/ava_san_texlist.inc"
};

static const int *screen_textures[SCREENS] = {
    screen_none_textures, character_textures, file_textures,
    trial_stage_textures, drive_textures,     sound_test_textures,
    card_textures,        mode_textures,      options_textures,
    game_bug_textures,    mini_game_textures,
};

Bool textures_ready;
/* bit n is set while PVM n is loaded */
static Uint32 textures_loaded;

void textures_load(const int *list)
{
    int texture;

    if (list != NULL) {
        /* @hack spelling: the texture read and then tested, in a comma
         * expression. SHC then reads it into R14 and keeps textures_loaded's
         * address in R13 (MOV.L @R10+,R14 at 0x8c902078). read in the test,
         * the two trade registers, and 7 of 41 words differ */
        while (texture = *list++, texture != TEX_END) {
            textures_loaded |= 1 << texture;
            texture_load_pvm(texture_files[texture].name,
                             texture_files[texture].texlist);
        }
    }
}

/* texture's bit in textures_loaded: nonzero while it is loaded, and 0
 * otherwise */
static Uint32 texture_is_loaded(int texture)
{
    return textures_loaded & (1 << texture);
}

void textures_release(const int *list)
{
    int texture;

    if (list != NULL) {
        /* @hack spelling: the texture read and then tested, in a comma
         * expression. SHC then keeps list in R13 and njReleaseTexture's
         * address in R11 (MOV R4,R13 at 0x8c902096, the MOV.L into R11 at
         * 0x8c9020c0). read in the test, the two trade registers, and 8 of 41
         * words differ */
        while (texture = *list++, texture != TEX_END) {
            if (texture_is_loaded(texture)) {
                textures_loaded &= ~(1 << texture);
                njReleaseTexture(texture_files[texture].texlist);
            }
        }
    }
}

void textures_init(void)
{
    textures_loaded = 0;
    textures_load(no_textures);
    textures_ready = TRUE;
}

void textures_release_all(void)
{
    int list[TEX_END + 1];
    int i;

    for (i = 0; i < TEX_END; i++) {
        list[i] = i;
    }
    list[i] = TEX_END;
    textures_release(list);
}

void textures_load_screen(int screen)
{
    textures_load(screen_textures[screen]);
    if (ADVERTISE_WORK()->last_screen != SCREEN_NONE) {
        textures_ready = FALSE;
    }
}

Bool textures_has(const int *list, int texture)
{
    Bool has = FALSE;

    while (list != NULL && *list != TEX_END) {
        if (*list == texture) {
            has = TRUE;
        }
        list++;
    }
    return has;
}

void textures_switch(int screen)
{
    int release[SCREEN_LIST_SIZE];
    const int *next;
    const int *last;
    int *out;

    if (ADVERTISE_WORK()->screen > 0 && ADVERTISE_WORK()->screen < SCREENS) {
        next = screen_textures[ADVERTISE_WORK()->screen];
    } else {
        next = NULL;
    }
    last = screen_textures[ADVERTISE_WORK()->last_screen];
    out = release;
    while (*last != TEX_END) {
        if (!textures_has(next, *last)) {
            *out++ = *last;
        }
        last++;
    }
    *out = TEX_END;
    textures_release(release);
    textures_ready = TRUE;
}
