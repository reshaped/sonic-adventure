/* the movie player. it plays a list's movies one after another, each fading
 * as its entry in movies says. a list stops at its end or at the pads' quit.
 * the quit is A, B, X, Y and Start held together. a list stops early at a
 * movie mwPlyCreateSofdec makes no player for, or whose sound ADX fails to
 * decode. Start stops the opening too */
#include <ninja.h>
#include <sg_mwgsc.h>
#include <sg_mwply.h>
#include <sg_ov.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <sofdec.h>

#include "draw/text.h"
#include "main.h"
#include "movie/cast.h"
#include "movie/movie.h"
#include "system/loader.h"
#include "system/pad.h"
#include "system/sbinit.h"

/* a movie of the lists: its file and its fades */
struct movie {
    char *file;
    int fade; /* FADE_OUT_ONLY, FADE_FULL or FADE_HALF */
    int fade_frame; /* the frame its fade out starts at */
};

/* how a movie fades. each fades out over FADE_OUT_FRAMES from its fade
 * frame, and all but FADE_OUT_ONLY fade in from dark over FADE_IN_TIME
 * first. FADE_HALF's fade out starts from half brightness */
#define FADE_OUT_ONLY 0
#define FADE_FULL 1
#define FADE_HALF 2

/* a fade in's length, in hundredths of a second, and a fade out's, in
 * frames. the fade out after Start skips the opening takes SKIP_FRAMES */
#define FADE_IN_TIME 100
#define FADE_OUT_FRAMES 30
#define SKIP_FRAMES 15

/* the skip's fade lowers the volume VOLUME_STEP a frame, down to
 * VOLUME_LEAST, the quietest mwPlySetOutVol sets */
#define VOLUME_STEP 3
#define VOLUME_LEAST (-999)

/* a list's last entry: LIST_END ends the list, and LIST_REPEAT plays it
 * again from its first movie */
#define LIST_END (-1)
#define LIST_REPEAT (-2)

/* the opening's list, which Start skips, and the list the cast is drawn
 * over. in the opening, the pads' quit takes the console back to the boot
 * ROM */
#define LIST_OPENING 0
#define LIST_CAST 7

/* where the player decodes: the 3 MB from MODULE_AREA, addressed through
 * P1 */
#define WORK ((Sint8 *)0x8c900000)
#define WORK_SIZE 0x300000

/* the brightness a fade in reaches, and a full fade out starts from */
#define BRIGHT 192

/* movie_display's modes: the display for the movies, and the game's */
#define DISPLAY_MOVIES 0
#define DISPLAY_GAME 1

/* where a movie is in its fades. movie_fade_step moves it on each frame */
enum fade_state {
    FADE_IN, /* from dark, over FADE_IN_TIME */
    FADE_PLAY, /* until the movie's fade frame */
    FADE_OUT, /* to dark, over FADE_OUT_FRAMES */
    FADE_DONE, /* dark for the rest of the movie */
    FADE_SKIP, /* Start, in the opening: the skip's fade starts this frame */
    FADE_SKIP_OUT /* to dark, over SKIP_FRAMES */
};

static void movie_fade_start(void);
static int movie_fade_step(int list);
static void movie_display(int mode, int list);
static void movie_vsync(void);

static struct movie movies[10] = {
    {"SLH448_1.SFD", FADE_FULL, 170},    {"SAH448_6.SFD", FADE_FULL, 3180},
    {"SA1_600.SFD", FADE_HALF, 2870},    {"SA2_600.SFD", FADE_FULL, 378},
    {"SA3_600.SFD", FADE_FULL, 860},     {"SA4_600.SFD", FADE_FULL, 550},
    {"SA5_600.SFD", FADE_OUT_ONLY, 390}, {"SA6_600.SFD", FADE_FULL, 630},
    {"SA7_300.SFD", FADE_FULL, 300},     {"SA8_600.SFD", FADE_FULL, 540},
};

static Sint8 movie_list0[] = {0, 1, LIST_END};
static Sint8 movie_list1[] = {2, LIST_END};
static Sint8 movie_list2[] = {3, LIST_END};
static Sint8 movie_list3[] = {4, LIST_END};
static Sint8 movie_list4[] = {5, LIST_END};
static Sint8 movie_list5[] = {6, LIST_END};
static Sint8 movie_list6[] = {7, LIST_END};
static Sint8 movie_list7[] = {8, LIST_END};
static Sint8 movie_list8[] = {9, LIST_END};
static Sint8 *movie_lists[9] = {movie_list0, movie_list1, movie_list2,
                                movie_list3, movie_list4, movie_list5,
                                movie_list6, movie_list7, movie_list8};

static MWPLY movie_player;
static MWS_PLY_CPRM_SFD movie_params;
static MWE_PLY_STAT movie_stat;
static Sint8 *movie_list;
static int movie_current; /* the movie playing, as its index in movies */
static Sint32 movie_ncount;
static Sint32 movie_tscale;
static int movie_time; /* the time played, in hundredths of a second */
static int movie_frame; /* frames since the movie started, one each vsync */
static enum fade_state movie_fade_state;
static int movie_fade_frame; /* the frame the fade out started at */
static Bool movie_skipped; /* Start came during the fade in */
/* 1 while the list plays on, and -1 after the pads' quit. 0 once a movie
 * stops the list early, or Start skips the opening */
static int movie_result;

int movie_main(int list)
{
    int i;

    i = 0;
    movie_result = 1;
    ovLibEntryTbl(&mwg_gsc_libhdl);
    ovLibEntryTbl(&mwg_sj_rbf_libhdl);
    njSetVSyncFunction(movie_vsync);
    njWaitVSync();
    movie_display(DISPLAY_MOVIES, list);
    sdMemBlkSetTransferMode(SDE_MEMBLK_TRANSFER_MODE_DMA);
    pad_enable();
    pad_scan();
    if (list == LIST_CAST) {
        cast_load();
    }
    mwPlyInitSofdec(NULL);
    movie_list = movie_lists[list];
    for (;;) {
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        movie_current = movie_list[i++];
        if (movie_current == LIST_END) {
            break;
        }
        if (movie_current == LIST_REPEAT) {
            i = 0;
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            movie_current = movie_list[i++];
        }
        movie_params.ftype = MWD_PLY_FTYPE_SFD;
        movie_params.dtype = MWD_PLY_DTYPE_AUTO;
        movie_params.work = WORK;
        movie_params.wksize = WORK_SIZE;
        if ((movie_player = mwPlyCreateSofdec(&movie_params)) == NULL) {
            movie_result = 0;
            break;
        }
        if (list == LIST_OPENING || list == LIST_CAST) {
            mwPlySetFastHalfpel(movie_player, 1);
        }
        movie_fade_start();
        mwPlyStartFname(movie_player, (Sint8 *)movies[movie_current].file);
        movie_frame = -1;
        for (;;) {
            njWaitVSync();
            mwPlyStartFrame();
            movie_frame++;
            if (adxpd_internal_error != 0) {
                movie_result = 0;
                adxpd_internal_error = 0;
                break;
            }
            pad_scan();
            if (pad_quit_pressed(0)) {
                if (list == LIST_OPENING) {
                    sbExitSystem();
                    syBtExit();
                } else {
                    movie_result = -1;
                    break;
                }
            }
            /* @hack spelling: the Start test nested in the opening's, which
             * keeps movie_main's frame at 12 bytes (ADD #-12,R15 at
             * 0x8ceb0030); joined by &&, the frame takes 16, and 224 of 280
             * words differ */
#ifdef GCC
            /* Start skips any movie, not the opening alone */
            {
#else
            if (list == LIST_OPENING) {
#endif
                if (per_info[0]->press & PDD_DGT_ST) {
                    movie_result = 0;
                    if (movie_fade_state == FADE_IN) {
                        movie_skipped = TRUE;
                    } else {
                        if (movie_fade_state == FADE_DONE) {
                            break;
                        }
                        if (movie_fade_state == FADE_PLAY) {
                            movie_fade_state = FADE_SKIP;
                        }
                    }
                }
            }
            if (movie_fade_step(list)) {
                break;
            }
            movie_stat = mwPlyGetStat(movie_player);
            if (movie_stat == MWE_PLY_STAT_PLAYEND ||
                movie_stat == MWE_PLY_STAT_ERROR) {
                break;
            }
            if (list == LIST_CAST) {
                cast_draw(movie_frame);
            }
            mwPlyExecServer();
        }
        mwPlyStop(movie_player);
        njWaitVSync();
        mwPlyDestroy(movie_player);
        if (movie_result <= 0) {
            break;
        }
    }
    mwPlyFinishSofdec();
    if (list == LIST_CAST) {
        cast_free();
    }
    njWaitVSync();
    movie_display(DISPLAY_GAME, list);
    njWaitVSync();
    if (movie_result >= 0) {
        if (list == LIST_OPENING) {
            movie_result = 1;
        } else {
            movie_result = 0;
        }
    }
    return movie_result;
}

/* clear the skip and the cast's black, and start the movie's fades. a movie
 * that fades in starts dark */
static void movie_fade_start(void)
{
    movie_skipped = cast_fade = 0;
    if (movies[movie_current].fade != FADE_OUT_ONLY) {
        mwPlySetBright(movie_player, 0);
        movie_fade_state = FADE_IN;
    } else {
        movie_fade_state = FADE_PLAY;
    }
}

/* step the fades a frame. returns 1 once the skip's fade out reaches dark,
 * and 0 otherwise. over the cast, a fade out raises the cast's black rather
 * than lowering the movie's brightness */
static int movie_fade_step(int list)
{
    int level;
    int vol;

    mwPlyGetTime(movie_player, &movie_ncount, &movie_tscale);
    movie_time = movie_ncount * 100 / movie_tscale;
    switch (movie_fade_state) { /* NOLINT(clang-diagnostic-switch) */
    case FADE_IN:
        level = movie_time * BRIGHT / FADE_IN_TIME;
        if (level >= BRIGHT) {
            movie_fade_state++;
            level = BRIGHT;
            if (movie_skipped) {
                movie_fade_state = FADE_SKIP;
            }
        }
        mwPlySetBright(movie_player, level);
        break;
    case FADE_PLAY:
        if (movie_frame >= movies[movie_current].fade_frame) {
            movie_fade_frame = movie_frame;
            movie_fade_state++;
        }
        break;
    case FADE_OUT:
        if (list != LIST_CAST) {
            if (movies[movie_current].fade == FADE_HALF) {
                level = (movie_fade_frame - movie_frame + FADE_OUT_FRAMES) *
                        BRIGHT / (2 * FADE_OUT_FRAMES);
            } else {
                level = (movie_fade_frame - movie_frame + FADE_OUT_FRAMES) *
                        BRIGHT / FADE_OUT_FRAMES;
            }
            if (level < 0) {
                movie_fade_state++;
                level = 0;
            }
            mwPlySetBright(movie_player, level);
        } else {
            cast_fade =
                (movie_frame - movie_fade_frame) * 255 / FADE_OUT_FRAMES;
            if (cast_fade >= 255) {
                cast_fade = CAST_SOLID;
                movie_fade_state++;
            }
        }
        break;
    case FADE_SKIP:
        movie_fade_frame = movie_frame;
        movie_fade_state++;
        /* no break: the skip's fade starts this frame */
    case FADE_SKIP_OUT:
        vol = (movie_fade_frame - movie_frame) * VOLUME_STEP;
        if (vol < VOLUME_LEAST) {
            vol = VOLUME_LEAST;
        }
        mwPlySetOutVol(movie_player, vol);
        if (list != LIST_CAST) {
            level = (movie_fade_frame - movie_frame + SKIP_FRAMES) * BRIGHT /
                    SKIP_FRAMES;
            if (level < 0) {
                level = 0;
                movie_fade_state = FADE_DONE;
            }
            mwPlySetBright(movie_player, level);
            if (level == 0) {
                return 1;
            }
        } else {
            cast_fade = (movie_frame - movie_fade_frame) * 255 / SKIP_FRAMES;
            if (cast_fade >= 255) {
                cast_fade = CAST_SOLID;
                movie_fade_state = FADE_DONE;
            }
            if (cast_fade < 0) {
                return 1;
            }
        }
        break;
    }
    return 0;
}

/* release every texture, and set the display up for mode. a VGA cable
 * takes VGA, and any other 640 by 480 NTSC, interlaced for the movies alone.
 * the movies take 32-bit color, but the cast's list takes 16-bit, as the
 * game does */
static void movie_display(int mode, int list)
{
    njReleaseTextureAll();
    njExitPrint();
    if (mode == DISPLAY_MOVIES) {
        if (cable_mode == SYE_CBL_CABLE_VGA) {
            if (list != LIST_CAST) {
                njChangeSystem(NJD_RESOLUTION_VGA,
                               NJD_FRAMEBUFFER_MODE_ARGB8888, 2);
            } else {
                njChangeSystem(NJD_RESOLUTION_VGA, NJD_FRAMEBUFFER_MODE_RGB565,
                               2);
            }
        } else {
            if (list != LIST_CAST) {
                njChangeSystem(NJD_RESOLUTION_640x480_NTSCI,
                               NJD_FRAMEBUFFER_MODE_ARGB8888, 1);
            } else {
                njChangeSystem(NJD_RESOLUTION_640x480_NTSCI,
                               NJD_FRAMEBUFFER_MODE_RGB565, 1);
            }
        }
    } else {
        if (cable_mode == SYE_CBL_CABLE_VGA) {
            njChangeSystem(NJD_RESOLUTION_VGA, NJD_FRAMEBUFFER_MODE_RGB565, 1);
        } else {
            njChangeSystem(NJD_RESOLUTION_640x480_NTSCNI,
                           NJD_FRAMEBUFFER_MODE_RGB565, 1);
        }
    }
    njInitVertexBuffer(-1, 0, 0x20000, 0, 0x8000);
    njInitPrint(NULL, 0, 0);
    text_style_init(NULL, 0, 0);
    njPrintSize(16);
}

/* the vsync function while the movies play */
static void movie_vsync(void)
{
    ADXT_ExecServer();
    sdSysServer();
    loader_poll_drive();
    buExecServer();
}
