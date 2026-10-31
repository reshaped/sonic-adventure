/* the main loop: the game's start, its modes, and the hand-off to
 * LOADER.BIN */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <sofdec.h>
#include <stdlib.h>

#include "advertise/advertise.h"
#include "draw/fade.h"
#include "draw/model.h"
#include "draw/text.h"
#include "hud/hud.h"
#include "main.h"
#include "object/common.h"
#include "player/player.h"
#include "progress/demo.h"
#include "progress/movie.h"
#include "progress/progress.h"
#include "progress/select.h"
#include "progress/title.h"
#include "save/card.h"
#include "save/save.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "summary/summary.h"
#include "system/frame.h"
#include "system/history.h"
#include "system/loader.h"
#include "system/pad.h"
#include "system/sbinit.h"
#include "system/texture.h"
#include "system/timer.h"
#include "vmu/screen.h"

#define MATRIX_DEPTH 128
/* system_boot_init gives texture_buffer to Ninja as its texture buffer,
 * and again as its 3D vertex buffer, of VERTEX_SIZE-byte vertices */
#define TEXTURE_BUFFER_SIZE 0x40000
#define VERTEX_SIZE 64
#define VERTEX_COUNT (TEXTURE_BUFFER_SIZE / VERTEX_SIZE)

/* an overlay loads at MODULE_AREA, with its table of entry points at its
 * head. each pointer is good only while its overlay is loaded */
#define ADVERTISE ((struct advertise *)MODULE_AREA)
#define SUMMARY ((struct summary *)MODULE_AREA)

#define RESET_RENDERER_AND_TEXTURES()                                          \
    (texture_release_regular(), texture_release_characters(), njExitTexture(), \
     display_reset_full())

extern Sint8 texture_buffer[];
extern NJS_MATRIX matrix_stack[];
#ifdef GCC
/* 1ST_READ.BIN's tables place the two in B32, the bss whose objects start
 * on a 32-byte boundary, where no source holds them. the port defines them
 * here, on a 32-byte boundary too */
Sint8 texture_buffer[TEXTURE_BUFFER_SIZE] __attribute__((aligned(32)));
NJS_MATRIX matrix_stack[MATRIX_DEPTH] __attribute__((aligned(32)));
/* draw_lights holds model.src's MOST_LIGHTS point lights of LT_SIZE bytes,
 * and draw_work its work, which ends with the three halfwords at ENV_INDEX,
 * byte 472. 1ST_READ.BIN's tables name both past its image, and
 * config/1st_read/census.txt places them in this source. GCC's renderer,
 * which replaces model.src, reads neither */
Uint32 draw_lights[96];
Uint32 draw_work[120];
#endif
/* the address main reads LOADER.BIN to, and LOADER.BIN's entry */
extern char loader_buffer[];
extern void loader_entry(void);

enum menu_entrance menu_entrance;
int cable_mode;
enum game_mode game_mode;
NJS_TEXMEMLIST texmemlist[TEXTURE_COUNT];
Bool loader_error_flag;
int dat_8c78b4d8;
int dat_8c78b4dc;
int dat_8c78b4e0;
Float shape_buffer[6144];
NJS_VIEW view;
NJS_SCREEN screen;
NJS_FOG_TABLE fog_table;

static Bool game_leaving = FALSE;

void main(void)
{
    game_init();
    while (!game_leaving) {
        timer_start();
        ADXT_ExecServer();
        if (!card_notice_busy()) {
            game_mode_step();
        }
        card_frame_update();
        if (!card_saving) {
            vmu_frame_update(game_mode);
        }
        sound_send_queue();
        /* @hack redundant: an empty test of hud_hidden, which keeps the image's
         * CMP/PZ (0x8c056d1c); without it, 83 of 113 words differ */
        if (hud_hidden >= 0) {
        }
        frame_wait();
    }
    {
        char name[20] = "loader.bin";
        Sint32 size;
        GDFS file;
        Sint32 sectors;

        sound_kill_all();
        stream_destroy();
        gdFsChangeDir("..");
        file = gdFsOpen(name, NULL);
        gdFsGetFileSize(file, &size);
        /* @hack spelling: the count through a local, which loads
         * loader_buffer's address before gdFsRead's (0x8c056d6c) and has the
         * loop keep card_notice_busy's address in R12, not sound_send_queue's
         * in R8 (0x8c056cd8 to 0x8c056ce6); read in the call, 24 of 113 words
         * differ */
        sectors = SECTORS(size);
        gdFsRead(file, sectors, loader_buffer);
        gdFsClose(file);
        game_release_textures();
        sbExitSystem();
        loader_entry();
    }
}

void game_leave(void)
{
    game_leaving = TRUE;
}

void game_init(void)
{
    mwPlyPreInitSofdec();
    system_boot_init();
    frame_wait();
    frame_wait();
    game_mode = GAME_MODE_BOOT;
    ADXT_ConfigVsyncSvr(0);
    ADXT_Init();
    read_queue_init();
    history_init();
    loader_error_flag = FALSE;
    gdFsEntryErrFuncAll(loader_note_error, NULL);
}

/* SHC gives each jump in this function a scratch register, R2 or R3 by
 * turns, even a jump it deletes as dead. the code after a jump then loads
 * its first scratch value into the other one. several tags below keep a
 * jump for that alone, and the staff roll's last case has no break for it */
int game_mode_step(void)
{
    int result;
    int status;

    result = 0;
    switch (game_mode) {
    case GAME_MODE_BOOT:
        boot_new_game();
        boot_init();
        game_mode = GAME_MODE_LOGO;
        boot_finish();
        break;
    case GAME_MODE_LOGO:
        result = logo_step();
        if (result == LOGO_ADVERTISE) {
            menu_begin();
            game_mode = GAME_MODE_MENU;
            menu_entrance = MENU_FROM_TOP;
        } else if (result == LOGO_MOVIE) {
            movie_request = 0;
            game_mode = GAME_MODE_MOVIE;
        }
        break;
    case GAME_MODE_TITLE:
        draw_off = FALSE;
        result = title_step();
        if (result == TITLE_PLAY) {
            draw_off = TRUE;
            texture_load_regular(0);
            if (stage_begin() == STAGE_GROUP_FIELD) {
                game_mode = GAME_MODE_FIELD;
            } else {
                game_mode = GAME_MODE_ACTION;
            }
            result = 0;
        } else if (result == TITLE_STAGE_SELECT) {
            game_mode = GAME_MODE_STAGE_SELECT;
            result = 0;
            stage_select_start();
        }
        pad_scan();
        pad_enable();
        break;
    case GAME_MODE_STAGE_SELECT:
        result = stage_select_step();
        if (result == SELECT_STEP_PICKED) {
            texture_load_regular(0);
            if (stage_begin() == STAGE_GROUP_FIELD) {
                game_mode = GAME_MODE_FIELD;
            } else {
                game_mode = GAME_MODE_ACTION;
            }
            result = 0;
        }
        break;
    case GAME_MODE_ACTION:
        result = stage_step_action();
        if (result == STAGE_STEP_RESET) {
            RESET_RENDERER_AND_TEXTURES();
            result = 0;
            menu_begin();
            game_mode = GAME_MODE_MENU;
            goto transition;
        }
        if (result == STAGE_GROUP_FIELD) {
            game_mode = GAME_MODE_FIELD;
            result = 0;
            break;
        }
        if (result == STAGE_STEP_QUIT) {
            int next;

            RESET_RENDERER_AND_TEXTURES();
            result = 0;
            next = demo_next_mode();
            if (next == 0) {
                menu_begin();
                game_mode = GAME_MODE_MENU;
                menu_entrance = MENU_FROM_TOP;
            } else {
                game_mode = next;
            }
            break;
        }
        if (result == STAGE_STEP_STAFF_ROLL) {
            RESET_RENDERER_AND_TEXTURES();
            game_mode = GAME_MODE_STAFF_ROLL;
            result = 0;
        }
        break;
    case GAME_MODE_FIELD:
        result = stage_step_field();
        if (result == STAGE_STEP_RESET) {
            RESET_RENDERER_AND_TEXTURES();
            result = 0;
            menu_begin();
            game_mode = GAME_MODE_MENU;
            goto transition;
        }
        if (result == STAGE_GROUP_ACTION) {
            game_mode = GAME_MODE_ACTION;
            result = 0;
            break;
        }
        if (result == STAGE_STEP_QUIT) {
            int next;

            RESET_RENDERER_AND_TEXTURES();
            result = 0;
            next = demo_next_mode();
            if (next == 0) {
                menu_begin();
                game_mode = GAME_MODE_MENU;
                menu_entrance = MENU_FROM_TOP;
            } else {
                game_mode = next;
            }
            break;
        }
        if (result == STAGE_STEP_STAFF_ROLL) {
            RESET_RENDERER_AND_TEXTURES();
            game_mode = GAME_MODE_STAFF_ROLL;
            result = 0;
        }
        break;
    case GAME_MODE_MOVIE:
        if (movie_play(movie_request) != 0) {
            RESET_RENDERER_AND_TEXTURES();
            result = 0;
            menu_begin();
            game_mode = GAME_MODE_MENU;
            goto transition;
            /* @hack redundant: a break after the goto, whose deleted jump
             * has the else arm load its first scratch value into R2
             * (0x8c0571b2), not R3; without it, 13 of 812 words differ */
            break;
        } else {
            RESET_RENDERER_AND_TEXTURES();
            draw_off = TRUE;
            texture_load_regular(0);
            if (stage_begin() == STAGE_GROUP_FIELD) {
                game_mode = GAME_MODE_FIELD;
            } else {
                game_mode = GAME_MODE_ACTION;
            }
            result = 0;
        }
        break;
    case GAME_MODE_TRIAL:
        result = stage_step_action();
        if (result == STAGE_STEP_RESET) {
            RESET_RENDERER_AND_TEXTURES();
            result = 0;
            pad_enable_player(2);
            menu_begin();
            game_mode = GAME_MODE_MENU;
            /* @hack spelling: the reset arms of the action, field and movie
             * cases jump to this label (0x8c057014, 0x8c0570ea, 0x8c0571aa) to
             * share the store below. written out in each arm, the stores change
             * what the prologue keeps in R8 to R11 (0x8c056ece to 0x8c056ede)
             * and 146 of 812 words differ */
        transition:
            /* @hack spelling: menu_entrance set from result, which every path
             * here leaves at MENU_FROM_TOP; set to MENU_FROM_TOP, 7 of 812
             * words differ */
            menu_entrance = result;
        } else if (result == STAGE_GROUP_FIELD) {
            result = 0;
            pad_enable_player(2);
            game_mode = GAME_MODE_MENU;
            menu_entrance = MENU_FROM_TRIAL;
        } else if (result == STAGE_STEP_QUIT) {
            result = 0;
            game_mode = GAME_MODE_MENU;
            menu_entrance = MENU_FROM_TRIAL;
            pad_enable_player(2);
        }
        break;
    case GAME_MODE_MENU:
        loader_read_prs("advertise.prs", ADVERTISE);
        ADVERTISE->start(&menu_entrance);
        ++game_mode;
        /* fall through */
    case GAME_MODE_MENU_RUN:
        result = ADVERTISE->step(&menu_entrance);
        switch (result) {
        /* @hack redundant: a case the default would take, whose break has
         * OVERLAY_BACK's case load its first scratch value into R2
         * (0x8c05730c), not R3; without it, 117 of 812 words differ */
        case OVERLAY_RUNNING:
            break;
        case OVERLAY_BACK:
            ADVERTISE->end(NULL);
            game_mode = GAME_MODE_TITLE;
            title_setup();
            break;
        case OVERLAY_RESET:
            RESET_RENDERER_AND_TEXTURES();
            menu_begin();
            game_mode = GAME_MODE_MENU;
            menu_entrance = MENU_FROM_TOP;
            result = 0;
            break;
        case MENU_EXIT:
            sbExitSystem();
            syBtExit();
            /* @hack spelling: a return without a value after syBtExit, which
             * never returns, so that SHC jumps to syBtExit last (0x8c057354);
             * with return 0, the jump becomes a call and 65 of 812 words
             * differ */
#ifdef GCC
            return 0;
#else
            return; /* NOLINT(clang-diagnostic-return-mismatch) */
#endif
            /* @hack redundant: a break after the return, whose deleted jump
             * has the next case load its first scratch value into R3
             * (0x8c057358), not R2; without it, 49 of 812 words differ */
            break;
        case MENU_TRIAL:
            ADVERTISE->end(NULL);
            fade_out_start();
            draw_off = TRUE;
            texture_load_regular(0);
            set_character(get_character());
            stage_begin();
            hud_reset();
            trial_times_clear();
            game_mode = GAME_MODE_TRIAL;
            result = 0;
            break;
        case OVERLAY_DONE:
            ADVERTISE->end(NULL);
            draw_off = TRUE;
            texture_load_regular(0);
            stage_begin();
            game_mode = GAME_MODE_TRIAL;
            result = 0;
            break;
        case MENU_ADVENTURE:
            ADVERTISE->end(NULL);
            game_mode = GAME_MODE_SUMMARY;
            result = 0;
            break;
        case MENU_TUTORIAL:
            ADVERTISE->end(NULL);
            game_mode = GAME_MODE_TUTORIAL;
            result = 0;
            break;
        /* @hack redundant: a default between cases, whose break has the next
         * case load its first scratch value into R3 (0x8c05742c), not R2;
         * without it, 9 of 812 words differ */
        default:
            break;
        case MENU_STAFF_ROLL:
            ADVERTISE->end(NULL);
            game_mode = GAME_MODE_STAFF_ROLL;
            result = 0;
            break;
        case MENU_INTERNET:
            stream_destroy();
            game_leaving = TRUE;
            break;
        case MENU_DEMO:
            demo_advance();
            texture_load_regular(0);
            if (stage_begin() == STAGE_GROUP_FIELD) {
                game_mode = GAME_MODE_FIELD;
            } else {
                game_mode = GAME_MODE_ACTION;
            }
            result = 0;
            break;
        case MENU_UNK_106:
            break;
        }
        break;
    case GAME_MODE_SUMMARY:
        loader_read_prs("summary.prs", SUMMARY);
        status = SUMMARY->start(NULL);
        if (status == OVERLAY_EMPTY) {
            int character;

            result = 0;
            game_mode = GAME_MODE_FIELD;
            hud_reset();
            character = get_character();
            save_load();
            /* Knuckles' story opens with MOVIE_SA5 until the story is
             * cleared. the pads' quit during the movie goes back to the
             * menus */
            if (character == CHARACTER_KNUCKLES &&
                !event_flag_check(FLAG_CLEAR_KNUCKLES) &&
                movie_play(MOVIE_SA5) < 0) {
                RESET_RENDERER_AND_TEXTURES();
                menu_begin();
                game_mode = GAME_MODE_MENU;
                menu_entrance = MENU_FROM_TOP;
                result = 0;
                break;
            }
            story_enable();
            set_character(character);
            story_set_character(character);
            story_set_next_stage(player_record->section);
            draw_off = TRUE;
            texture_load_regular(0);
            break;
        } else {
            /* @hack redundant: an else after an arm that always breaks, whose
             * deleted jump has the increment take R2 (0x8c057560), not R3;
             * without it, 3 of 812 words differ */
            ++game_mode;
        }
        /* fall through */
    case GAME_MODE_SUMMARY_RUN:
        result = SUMMARY->step(NULL);
        if (result == OVERLAY_DONE) {
            int character;

            result = 0;
            character = get_character();
            hud_reset();
            save_load();
            story_enable();
            set_character(character);
            story_set_character(character);
            story_set_next_stage(player_record->section);
            draw_off = TRUE;
            texture_load_regular(0);
            if (stage_group() == STAGE_GROUP_ACTION) {
                game_mode = GAME_MODE_ACTION;
                break;
            }
            game_mode = GAME_MODE_FIELD;
            break;
        }
        if (result == OVERLAY_BACK) {
            result = 0;
            game_mode = GAME_MODE_MENU;
            menu_entrance = MENU_FROM_SUMMARY;
            break;
        }
        if (result == OVERLAY_RESET) {
            RESET_RENDERER_AND_TEXTURES();
            menu_begin();
            game_mode = GAME_MODE_MENU;
            menu_entrance = MENU_FROM_TOP;
            result = 0;
        }
        break;
    case GAME_MODE_TUTORIAL:
        loader_read_prs("summary.prs", SUMMARY);
        SUMMARY->tutorial_start(NULL);
        ++game_mode;
        /* fall through */
    case GAME_MODE_TUTORIAL_RUN:
        result = SUMMARY->tutorial_step(NULL);
        if (result == OVERLAY_DONE) {
            result = 0;
            game_mode = GAME_MODE_SUMMARY;
        } else if (result == OVERLAY_BACK) {
            result = 0;
            game_mode = GAME_MODE_MENU;
            menu_entrance = MENU_FROM_SUMMARY;
        } else if (result == OVERLAY_RESET) {
            RESET_RENDERER_AND_TEXTURES();
            menu_begin();
            game_mode = GAME_MODE_MENU;
            menu_entrance = MENU_FROM_TOP;
            result = 0;
        }
        break;
    case GAME_MODE_STAFF_ROLL:
        loader_read_prs("summary.prs", SUMMARY);
        SUMMARY->staff_roll_start(NULL);
        draw_off = FALSE;
        ++game_mode;
        /* fall through */
    case GAME_MODE_STAFF_ROLL_RUN:
        result = SUMMARY->staff_roll_step(NULL);
        switch (result) {
        case OVERLAY_RUNNING:
            break;
        case OVERLAY_DONE:
            logo_start();
            game_mode = GAME_MODE_LOGO;
            break;
        case OVERLAY_BACK:
            game_mode = GAME_MODE_MENU;
            menu_entrance = MENU_FROM_SUMMARY;
            break;
        case OVERLAY_RESET:
            RESET_RENDERER_AND_TEXTURES();
            menu_begin();
            game_mode = GAME_MODE_MENU;
            menu_entrance = MENU_FROM_TOP;
            result = 0;
            /* no break, which the last case doesn't need. one would load
             * fade_draw_cover's address after the switch into R2
             * (0x8c05772c), not R3, and 4 of 812 words would differ */
        }
        break;
    default:
        result = 0;
        break;
    }
    fade_draw_cover();
    play_time_count();
    /* only the stage select's reset leaves result at -2 here.
     * OVERLAY_RESET and STAGE_STEP_RESET are -2 too, but each case that
     * takes one sets result to 0 */
    if (result == SELECT_STEP_RESET) {
        return -1;
    }
    return 0;
}

void game_shutdown(void)
{
    texture_release_regular();
    texture_release_characters();
    njExitTexture();
    sbExitSystem();
}

void game_release_textures(void)
{
    texture_release_regular();
    texture_release_characters();
    njExitTexture();
}

void texture_release_all(void)
{
    njReleaseTextureAll();
}

void display_reset_full(void)
{
    NJS_ARGB col;

    njInitMatrix(matrix_stack, MATRIX_DEPTH, 0);
    screen.w = SCREEN_WIDTH;
    screen.h = SCREEN_HEIGHT;
    screen.cx = screen.w * 0.5F;
    screen.cy = screen.h * 0.5F;
    njSetScreen(&screen);
    njSetPerspective(NJM_DEG_ANG(70));
    njInitView(&view);
    njSetView(&view);
    njInitTexture(texmemlist, TEXTURE_COUNT);
    njSetBackColor(0, 0, 0);
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
    njSuperSampleMode(0);
    njTextureClampMode(NJD_TEXTURECLAMP_NOCLAMP);
    njTextureFlipMode(NJD_TEXTUREFLIP_NOFLIP);
    njControl3D(NJD_CONTROL_3D_CONSTANT_ATTR | NJD_CONTROL_3D_ENABLE_ALPHA |
                NJD_CONTROL_3D_OFFSET_MATERIAL | NJD_CONTROL_3D_DEPTH_QUEUE |
                NJD_CONTROL_3D_MODEL_CLIP |
                NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
    njSetConstantAttr(0xffffffff, NJD_FILTER_BILINEAR);
    col.a = col.r = col.g = col.b = 0.0F;
    njSetConstantMaterial(&col);
    njSetDepthQueue(-1800.0F, -2000.0F);
    njClipZ(-3.0F, -3000.0F);
    njSetFogColor(0xffffffff);
    njSetFogDensity(0xff12);
    njGenerateFogTable(fog_table);
    njSetFogTable(fog_table);
    njFogEnable();
    njPolygonCullingMode(NJD_POLYGON_CULLINGSMALL);
    njPolygonCullingSize(0.05F);
    srand(0xdead0cab);
    njPrintSize(16);
    draw_model_init();
}

void system_boot_init(void)
{
    dat_8c78b4d8 = dat_8c78b4dc = 0;
    njSetTextureMemorySize(0x0011c000);
    cable_mode = syCblCheckCable();
    if (cable_mode != SYE_CBL_CABLE_VGA) {
        sbInitSystem(NJD_RESOLUTION_640x480_NTSCNI, NJD_FRAMEBUFFER_MODE_RGB565,
                     1);
    } else {
        sbInitSystem(NJD_RESOLUTION_VGA, NJD_FRAMEBUFFER_MODE_RGB565, 1);
    }
    njSetBorderColor(0);
    frame_pacing_init(2, 2);
    sound_system_init();
    njInitTextureBuffer(texture_buffer, TEXTURE_BUFFER_SIZE);
    njInitVertexBuffer(-1, 0, 0x20000, 0, 0x8000);
    njInitPrint(NULL, 0, 0);
    text_style_init(NULL, 0, 0);
    njInit3D((NJS_VERTEX_BUF *)texture_buffer, VERTEX_COUNT);
    njInitShape(shape_buffer);
    display_reset_full();
    vmu_init();
    card_init();
    per_info[0] = (PDS_PERIPHERAL *)njGetPeripheral(NJD_PORT_A0);
    per_info[1] = (PDS_PERIPHERAL *)njGetPeripheral(NJD_PORT_B0);
    per_info[2] = (PDS_PERIPHERAL *)njGetPeripheral(NJD_PORT_C0);
    per_info[3] = (PDS_PERIPHERAL *)njGetPeripheral(NJD_PORT_D0);
}
