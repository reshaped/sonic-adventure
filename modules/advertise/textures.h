/* the menus' textures: each screen's PVMs, loaded as the screen opens and
 * released once it has closed, but for those the next screen needs */
#ifndef ADVERTISE_TEXTURES_H
#define ADVERTISE_TEXTURES_H

#include <ninja.h>
#include <sg_xpt.h>

/* the menus' PVMs. a list of them, as the functions below take it, ends at
 * TEX_END, which is also their count */
enum advertise_texture {
    TEX_AVA_BACK,
    TEX_ADV_WINDOW,
    TEX_AVA_CHSEL,
    TEX_AVA_SUUJI,
    TEX_AVA_CSR,
    TEX_AVA_DLG,
    TEX_AVA_FSDLG,
    TEX_AVA_FILESEL,
    TEX_AVA_FSCMN,
    TEX_AVA_SNDTEST,
    TEX_AVA_SQUARE,
    TEX_AVA_STNAM,
    TEX_AVA_TRIALACTSEL,
    TEX_AVA_VMSSEL,
    TEX_B_CHNAM,
    TEX_M_CHNAM,
    TEX_TX_CHNAM,
    TEX_AVA_GTITLE0,
    TEX_AVA_TITLE,
    TEX_AVA_OPTION,
    TEX_AVA_EMBLEM,
    TEX_AVA_NEW16NO,
    TEX_AVA_SAN,
    TEX_END
};

/* the textures of the PVM ava_dlg, by index in ava_dlg_texlist */
enum {
    DLG_CANCEL,
    DLG_ENDING,
    DLG_START,
    DLG_DECIDE,
    DLG_NO,
    DLG_TUTORIAL,
    DLG_SS_PLAYER,
    DLG_SS_STAGE,
    DLG_ERASE,
    DLG_YES
};

/* the textures of the PVM ava_trialactsel, by index in its texlist */
enum {
    ACTSEL_1ST,
    ACTSEL_2ND,
    ACTSEL_3RD,
    ACTSEL_BAR_P,
    ACTSEL_BAR_WINDOW_1,
    ACTSEL_BAR_WINDOW_2,
    ACTSEL_BAR_WINDOW_ABC,
    ACTSEL_BAR_Y,
    ACTSEL_BAR_Y_SMALL,
    ACTSEL_TITLE,
    ACTSEL_BEST_CLEAR,
    ACTSEL_BEST_WEIGHT,
    ACTSEL_EMBLEMS,
    ACTSEL_HISCORE,
    ACTSEL_MINI_GAME,
    ACTSEL_RINGS,
    ACTSEL_SCORE,
    ACTSEL_TIME
};

/* the textures of the PVM ava_fscmn, by index in its texlist: the words
 * "file" and "back", a save's number, and the back row's arrow */
enum {
    FSCMN_FILE,
    FSCMN_BACK_ARROW,
    FSCMN_NUMBER_1,
    FSCMN_NUMBER_2,
    FSCMN_NUMBER_3,
    FSCMN_BACK
};

/* the textures of the PVM ava_title, by index in its texlist */
enum {
    TITLE_PIECE_1,
    TITLE_PIECE_2,
    TITLE_PIECE_3,
    TITLE_PIECE_4,
    TITLE_PIECE_5,
    TITLE_PIECE_6,
    TITLE_PIECE_7,
    TITLE_PIECE_8,
    TITLE_ACTION,
    TITLE_ADVENTURE,
    TITLE_INTERNET,
    TITLE_MINI_GAME,
    TITLE_OPTIONS,
    TITLE_TRIAL,
    TITLE_WINDOW
};

/* the textures of the PVM ava_option, by index in its texlist */
enum {
    OPTION_MONO,
    OPTION_VOICE,
    OPTION_VOICE_TEXT,
    OPTION_STEREO,
    OPTION_FILE_ARROW,
    OPTION_FILE_BACKING,
    OPTION_FILE,
    OPTION_FILE_CARD,
    OPTION_BACK,
    OPTION_MESSAGES_C,
    OPTION_MESSAGES_BANG,
    OPTION_MESSAGES_I,
    OPTION_MESSAGES_N,
    OPTION_MESSAGES_O,
    OPTION_MESSAGES_S,
    OPTION_MESSAGES_BACKING,
    OPTION_MESSAGES_SONIC1,
    OPTION_MESSAGES_SONIC2,
    OPTION_MESSAGES,
    OPTION_SPEAKERS_BACKING,
    OPTION_SPEAKER,
    OPTION_SPEAKERS,
    OPTION_SOUND_TEST_CD1,
    OPTION_SOUND_TEST_CD2,
    OPTION_SOUND_TEST_NOTE,
    OPTION_SOUND_TEST_BACKING,
    OPTION_SOUND_TEST,
    OPTION_TITLE
};

/* the textures of the PVM ava_sndtest, by index in its texlist */
enum {
    SNDTEST_AB,
    SNDTEST_ACTION,
    SNDTEST_ADVENTURE,
    SNDTEST_ARROW,
    SNDTEST_ARROW_SMALL,
    SNDTEST_ARROW_FRAME,
    SNDTEST_CHARACTER,
    SNDTEST_EVENT,
    SNDTEST_JINGLE,
    SNDTEST_EFFECTS,
    SNDTEST_BACK,
    SNDTEST_OTHER,
    SNDTEST_TITLE,
    SNDTEST_WINDOW_LARGE,
    SNDTEST_WINDOW,
    SNDTEST_WINDOW_SMALL
};

/* the PVMs' texlists, but for those of adv_window, ava_csr and ava_square:
 * screen_box_texlist, menu_row_texlist and menu_column_texlist */
extern NJS_TEXLIST ava_back_texlist;
extern NJS_TEXLIST ava_chsel_texlist;
extern NJS_TEXLIST ava_suuji_texlist;
extern NJS_TEXLIST ava_dlg_texlist;
extern NJS_TEXLIST ava_fsdlg_texlist;
extern NJS_TEXLIST ava_filesel_texlist;
extern NJS_TEXLIST ava_fscmn_texlist;
extern NJS_TEXLIST ava_sndtest_texlist;
extern NJS_TEXLIST ava_stnam_texlist;
extern NJS_TEXLIST ava_trialactsel_texlist;
extern NJS_TEXLIST ava_vmssel_texlist;
extern NJS_TEXLIST b_chnam_texlist;
extern NJS_TEXLIST m_chnam_texlist;
extern NJS_TEXLIST tx_chnam_texlist;
extern NJS_TEXLIST ava_gtitle0_texlist;
extern NJS_TEXLIST ava_title_texlist;
extern NJS_TEXLIST ava_option_texlist;
extern NJS_TEXLIST ava_emblem_texlist;
extern NJS_TEXLIST ava_new16no_texlist;
extern NJS_TEXLIST ava_san_texlist;

/* the texture in ava_stnam of each of the STAGE_NAMES stage names, by the
 * name's number. each entry is its own index */
#define STAGE_NAMES 33
extern const Uint8 stage_name_textures[STAGE_NAMES];
/* FALSE from the time a screen loads its PVMs while another closes, until
 * the closed screen's are released, and TRUE otherwise. the music and voices
 * wait for TRUE, and so do some screens' buttons */
extern Bool textures_ready;

/* textures_load loads each PVM of list, which can be NULL, even one already
 * loaded. textures_release releases each PVM of list that is loaded, and list
 * can be NULL */
void textures_load(const int *list);
void textures_release(const int *list);
/* textures_init marks no PVM loaded, without releasing any, and sets
 * textures_ready TRUE. textures_release_all releases every PVM loaded */
void textures_init(void);
void textures_release_all(void);
/* textures_load_screen and textures_switch need the menus' task live.
 * textures_load_screen loads the PVMs of screen, and sets textures_ready FALSE
 * unless ADVERTISE_WORK()->last_screen is SCREEN_NONE. textures_switch
 * releases the PVMs of ADVERTISE_WORK()->last_screen, the screen closing, but
 * for those ADVERTISE_WORK()->screen needs. as the menus close, it releases
 * them all. it then sets textures_ready TRUE. it ignores screen */
void textures_load_screen(int screen);
void textures_switch(int screen);
/* TRUE if list, which can be NULL, holds texture, and FALSE if not */
Bool textures_has(const int *list, int texture);

#endif
