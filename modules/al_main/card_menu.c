/* the card menu's tasks. the menu makes its drives, buttons, cursor and
 * panel, and each drive a preview of its card's Chao. the notice is a task
 * of its own */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "al_main/camera.h"
#include "al_main/card.h"
#include "al_main/card_menu.h"
#include "al_main/chao.h"
#include "al_main/chao_name.h"
#include "al_main/face.h"
#include "al_main/icon.h"
#include "al_main/models.h"
#include "al_main/motion.h"
#include "al_main/shape.h"
#include "al_main/status.h"
#include "camera/camera.h"
#include "chao/chao.h"
#include "draw/draw.h"
#include "draw/pose.h"
#include "draw/tree.h"
#include "save/card.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* the slots, two to a drive: its Chao's and its egg's */
#define CARD_SLOTS (2 * CARD_DRIVES)
#define CARD_BUTTONS 6 /* button n draws card_menu_sprite's image n */
#define CURSOR_SPRITE 14 /* the cursor's image of card_menu_sprite */

/* the panel's message as the menu opens for putting, and for taking */
#define MESSAGE_PUT 2
#define MESSAGE_TAKE 3
/* a message's place on the screen, its x moved by its own offset */
#define MESSAGE_X 370.0F
#define MESSAGE_Y 90.0F

enum { MENU_OPENING, MENU_CHOOSING, MENU_CLOSING, MENU_CLOSED };
#define OPEN_WAIT 60
#define CLOSE_WAIT 120
#define CURSOR_WAIT 10
#define CURSOR_STEPS_MAX 10000

/* the modes of a part that fades in and then out: a button, the cursor,
 * the panel or the notice */
enum { PART_HIDDEN, PART_FADING_IN, PART_SHOWN, PART_FADING_OUT };
#define NOTICE_HOLD 150

/* a drive's modes: shown, then closed once the menu closes */
enum { DRIVE_SHOWN, DRIVE_CLOSED };
/* a preview's modes: idle, then glad while the cursor is on its slot */
enum { PREVIEW_IDLE, PREVIEW_GLAD };

/* the menu's sounds */
#define SE_TAKE_CURSOR 0x28d /* the take menu's cursor moves */
#define SE_PUT_CURSOR 0x291 /* the put menu's cursor moves */
/* the take menu takes a slot's record, or the put menu refuses a full
 * slot */
#define SE_SLOT_FULL 0x387
/* the notice shows, or the take menu refuses an empty slot */
#define SE_CARD_NOTICE 0x374
/* behavior.c's hatch, which the put menu plays as it refuses a drive with
 * no card */
#define SE_HATCH 0x265
/* a preview's two calls, as the cursor comes to its slot */
#define SE_PREVIEW_CALL 0x383
#define SE_PREVIEW_CALL_2 0x384

/* the coats a preview's COAT_NODES can wear: a gold Chao's, a pal Chao's,
 * and each jewel's, by its record's jewel */
#define TEX_GOLD_COAT 158
#define TEX_PAL_COAT 161
#define TEX_AME 217
#define TEX_EME 218
#define TEX_RUB 219
#define TEX_SAF 220

/* each drive's place in the camera's frame, and its turn about y, by
 * drive */
extern NJS_POINT3 card_drive_positions[CARD_DRIVES];
extern Angle card_drive_angles[CARD_DRIVES];
/* the buttons' places on the screen */
extern NJS_POINT3 card_button_positions[CARD_BUTTONS];
/* the panel's place in the camera's frame, which the notice takes too */
extern NJS_POINT3 card_notice_pos;
extern NJS_SPRITE card_menu_sprite;
/* the cursor's place on the screen, by card_menu.cursor */
extern NJS_POINT3 card_cursor_positions[CARD_SLOTS];
extern NJS_TEXLIST card_message_texlist;
extern NJS_SPRITE card_message_sprite;
/* by message: its offset from MESSAGE_X, and the one or two textures it
 * draws, the second -1 for none */
extern Float card_message_offsets[];
extern Sint32 card_message_parts[][2];

struct card_menu card_menu;
/* 4 bytes between card_menu and card_menu_cursor_steps, which no code
 * reads */
static Uint8 dat_0cb1e8b4[4];
/* the steps the menus have spent in MENU_CHOOSING since the cursor last
 * took a press of a direction. it takes the next only once the count passes
 * CURSOR_WAIT. the put menu caps the count at CURSOR_STEPS_MAX */
static Sint32 card_menu_cursor_steps;

static void card_menu_init(struct task *task);
static void card_menu_take_step(struct task *task);
static void card_menu_put_step(struct task *task);
static void card_menu_destroy(struct task *task);
static void card_menu_drive_init(struct task *task);
static void card_menu_drive_step(struct task *task);
static void drive_draw(struct task_work *work);
static void card_menu_button_step(struct task *task);
static void card_menu_cursor_step(struct task *task);
static void card_menu_panel_step(struct task *task);
static struct task *card_menu_preview_create(struct chao *record);
static void preview_init(struct task *task);
static void preview_destroy(struct task *task);
static void preview_setup(struct task *task);
static void preview_step(struct task *task);
static void preview_draw(struct task_work *work, struct alife_chao *chao);

void card_menu_put_create(void)
{
    card_menu.failed = FALSE;
    task_create(TASK_WORK, 3, card_menu_init);
    card_menu.mode = CARD_MENU_PUT;
    card_menu.message = MESSAGE_PUT;
}

void card_menu_take_create(void)
{
    card_menu.failed = FALSE;
    task_create(TASK_WORK, 3, card_menu_init);
    card_menu.mode = CARD_MENU_TAKE;
    card_menu.message = MESSAGE_TAKE;
}

/* the menu's first step. it makes the drives, the buttons and the panel
 * as its children, and the cursor too when taking */
static void card_menu_init(struct task *task)
{
    struct card_menu *menu = &card_menu;
    struct task *child;
    struct task_work *work;
    int i;

    if (!card_drives_start()) {
        /* never runs: card_drives_start always returns 1. if it ran, the step
         * set below would replace task_kill's, and the menu would run on
         * with failed set */
        task_kill(task);
        menu->failed = TRUE;
    }
    camera_set_perspective(NJM_DEG_ANG(80));
    card_drives_refresh();
    card_drives_take_changes();
    for (i = 0; i < CARD_DRIVES; i++) {
        child = task_create_child(TASK_WORK, card_menu_drive_init, task);
        work = child->work;
        work->pos = card_drive_positions[i];
        work->ang.y = card_drive_angles[i];
        work->tick = i;
    }
    for (i = 0; i < CARD_BUTTONS; i++) {
        child = task_create_child(TASK_WORK, card_menu_button_step, task);
        work = child->work;
        work->pos = card_button_positions[i];
        work->tick = i;
    }
    child = task_create_child(TASK_WORK, card_menu_panel_step, task);
    work = child->work;
    work->pos = card_notice_pos;
    menu->open = FALSE;
    menu->closing = FALSE;
    menu->chosen = FALSE;
    menu->placed = FALSE;
    menu->cursor = 0;
    menu->record = NULL;
    garden_camera_fixed = TRUE;
    alife_paused = 1;
    pad_disable_player(0);
    sound_play(SE_MENU_OPEN, 0, 0, 0);
    if (menu->mode == CARD_MENU_TAKE) {
        task->step = card_menu_take_step;
        task_create_child(TASK_WORK, card_menu_cursor_step, task);
    } else {
        task->step = card_menu_put_step;
    }
    task->destructor = card_menu_destroy;
}

/* the take menu's step. the menu opens once the timer passes OPEN_WAIT.
 * then A on a slot that isn't empty chooses the slot's record and closes
 * the menu, and B closes it. the task ends once the timer, restarted at the
 * close, passes CLOSE_WAIT */
static void card_menu_take_step(struct task *task)
{
    struct task_work *work = task->work;
    struct card_menu *menu = &card_menu;
    int one;

    /* @hack spelling: TRUE through one, set in the switch's comma expression,
     * which loads 0 into R9 at 0x0c90f1b2 ahead of 1 into R11 at
     * 0x0c90f1c4; written in place, 20 of 250 words differ */
    switch ((one = 1, work->mode)) {
    case MENU_OPENING:
        if (TASK_TIMER(work)++ > OPEN_WAIT) {
            menu->open = one;
            work->mode++;
        }
        break;
    case MENU_CHOOSING:
        if (card_menu_cursor_steps++ > CURSOR_WAIT) {
            if (pad_press_repeat[0] & PDD_DGT_KU) {
                card_menu_cursor_steps = 0;
                sound_play(SE_TAKE_CURSOR, 0, 0, 0);
                if (menu->cursor % 4 != 0) {
                    menu->cursor--;
                }
            }
            if (pad_press_repeat[0] & PDD_DGT_KD) {
                card_menu_cursor_steps = 0;
                sound_play(SE_TAKE_CURSOR, 0, 0, 0);
                if (menu->cursor % 4 != 3) {
                    menu->cursor++;
                }
            }
            if (pad_press_repeat[0] & PDD_DGT_KL) {
                card_menu_cursor_steps = 0;
                sound_play(SE_TAKE_CURSOR, 0, 0, 0);
                if (menu->cursor / 4 != 0) {
                    menu->cursor -= 4;
                }
            }
            if (pad_press_repeat[0] & PDD_DGT_KR) {
                card_menu_cursor_steps = 0;
                sound_play(SE_TAKE_CURSOR, 0, 0, 0);
                if (menu->cursor / 4 != 3) {
                    menu->cursor += 4;
                }
            }
        }
        if (per_info[0]->press & PDD_DGT_TA) {
            /* @hack spelling: the drive's number through index, which
             * halves the cursor into R5 at 0x0c90f2e8, ahead of the call of
             * __modls, for the MULS.W at 0x0c90f2f6; read in place, 37 of
             * 250 words differ */
            int index = menu->cursor / 2;
            int slot = menu->cursor % 2;
            struct card_menu_drive *drive = &card_menu_drives[index];

            if (slot == 0) {
                if (drive->chao.type != CHAO_NOTHING) {
                    sound_play(SE_SLOT_FULL, 0, 0, 0);
                    menu->record = &drive->chao;
                    menu->chosen = one;
                    work->mode++;
                } else {
                    sound_play(SE_CARD_NOTICE, 0, 0, 0);
                }
            } else {
                if (drive->egg.type != CHAO_NOTHING) {
                    sound_play(SE_SLOT_FULL, 0, 0, 0);
                    menu->record = &drive->egg;
                    menu->chosen = one;
                    work->mode++;
                } else {
                    sound_play(SE_CARD_NOTICE, 0, 0, 0);
                }
            }
        }
        /* @bug logic: B also adds 1 to the mode in the step in which A
         * chooses a slot (0x0c90f36a, 0x0c90f386). the mode then skips
         * MENU_CLOSING, so open stays set and closing is never set. nothing
         * restarts the timer either, and MENU_CLOSED counts it on from
         * OPEN_WAIT + 2, where MENU_OPENING's test left it */
        if (per_info[0]->press & PDD_DGT_TB) {
            work->mode++;
        }
        break;
    case MENU_CLOSING:
        menu->open = FALSE;
        menu->closing = one;
        work->timer = 0;
        work->mode++;
        break;
    case MENU_CLOSED:
        if (TASK_TIMER(work)++ > CLOSE_WAIT) {
            task_kill(task);
            return;
        }
        break;
    }
    card_drives_take_changes();
    task_step_children(task);
}

/* the put menu's step. the menu opens once the timer passes OPEN_WAIT.
 * then A on a drive whose card has held's slot empty chooses the drive and
 * closes the menu, and B closes it. the task ends once the timer,
 * restarted at the close, passes CLOSE_WAIT */
static void card_menu_put_step(struct task *task)
{
    /* @hack spelling: sound_play called through play, which loads card_menu's
     * address at 0x0c90f418 ahead of sound_play's at 0x0c90f41c; called by
     * name, the two trade, and 2 of 253 words and 4 pool words differ */
    int (*play)(int, int, int, int) = sound_play;
    struct task_work *work = task->work;
    struct card_menu *menu = &card_menu;
    int row;

    switch (work->mode) {
    case MENU_OPENING:
        if (TASK_TIMER(work)++ > OPEN_WAIT) {
            menu->open = TRUE;
            work->mode++;
        }
        break;
    case MENU_CHOOSING:
        if (card_menu_cursor_steps++ > CURSOR_WAIT) {
            if (pad_press_repeat[0] & PDD_DGT_KU) {
                card_menu_cursor_steps = 0;
                play(SE_PUT_CURSOR, 0, 0, 0);
                /* @hack spelling: row assigned in its test, which copies
                 * __modls's R0 to R4 in the delay slot of the BT/S at
                 * 0x0c90f4ac; assigned before the if, 3 of 253 words
                 * differ */
                if ((row = menu->cursor % 4) != 0 && row != 1) {
                    menu->cursor -= 2;
                }
            }
            if (pad_press_repeat[0] & PDD_DGT_KD) {
                card_menu_cursor_steps = 0;
                play(SE_PUT_CURSOR, 0, 0, 0);
                row = menu->cursor % 4;
                if (row != 2 && row != 3) {
                    menu->cursor += 2;
                }
            }
            if (pad_press_repeat[0] & PDD_DGT_KL) {
                card_menu_cursor_steps = 0;
                play(SE_PUT_CURSOR, 0, 0, 0);
                if (menu->cursor / 4 != 0) {
                    menu->cursor -= 4;
                }
            }
            if (pad_press_repeat[0] & PDD_DGT_KR) {
                card_menu_cursor_steps = 0;
                play(SE_PUT_CURSOR, 0, 0, 0);
                if (menu->cursor / 4 != 3) {
                    menu->cursor += 4;
                }
            }
            if (card_menu_cursor_steps > CURSOR_STEPS_MAX) {
                card_menu_cursor_steps = CURSOR_STEPS_MAX;
            }
        }
        if (per_info[0]->press & PDD_DGT_TA) {
            struct card_menu_drive *drive = &card_menu_drives[menu->cursor / 2];

            if (drive->present) {
                /* @hack spelling: held through a local, which loads it
                 * into R5 at 0x0c90f56e for the read of its type at
                 * 0x0c90f570; read in place, 2 of 253 words differ */
                struct chao *held = menu->held;

                if (held->type == CHAO_EGG) {
                    if (drive->egg.type == CHAO_NOTHING) {
                        menu->placed = TRUE;
                        work->mode++;
                    } else {
                        play(SE_SLOT_FULL, 0, 0, 0);
                    }
                } else if (held->type != CHAO_PARTNER) {
                    if (drive->chao.type == CHAO_NOTHING) {
                        menu->placed = TRUE;
                        work->mode++;
                    } else {
                        play(SE_SLOT_FULL, 0, 0, 0);
                    }
                }
            } else {
                play(SE_HATCH, 0, 0, 0);
            }
        }
        /* @bug logic: as in the take menu, B also adds 1 to the mode in
         * the step in which A chooses a drive (0x0c90f5e0, 0x0c90f60a), and
         * the mode passes MENU_CLOSING */
        if (per_info[0]->press & PDD_DGT_TB) {
            work->mode++;
        }
        break;
    case MENU_CLOSING:
        menu->open = FALSE;
        menu->closing = TRUE;
        work->timer = 0;
        play(SE_MENU_CLOSE, 0, 0, 0);
        work->mode++;
        break;
    case MENU_CLOSED:
        if (TASK_TIMER(work)++ > CLOSE_WAIT) {
            task_kill(task);
        }
        break;
    }
    card_drives_take_changes();
    task_step_children(task);
}

static void card_menu_destroy(struct task *task)
{
}

static void card_menu_drive_init(struct task *task)
{
    task->step = card_menu_drive_step;
}

/* a drive grows toward 0.8 with a card in it, or 1.05 under the cursor
 * when putting, and shrinks toward 0 without one. it ends the step after
 * the menu closes. in a step whose poll finds a card new to the drive, it
 * sets was_present and plays SE_MENU_OPEN. it then makes the preview of the
 * card's Chao. its work's tick is the drive, scl.x the size it grows
 * toward, and scl.y its size */
static void card_menu_drive_step(struct task *task)
{
    struct task_work *work = task->work;
    struct card_menu_drive *drive = &card_menu_drives[work->tick];
    Float delta;

    switch (work->mode) {
    case DRIVE_SHOWN:
        if (drive->present) {
            if (card_menu.mode == CARD_MENU_PUT) {
                if (card_menu.cursor / 2 == work->tick) {
                    work->scl.x = 1.05F;
                } else {
                    work->scl.x = 0.8F;
                }
            } else {
                work->scl.x = 0.8F;
            }
        } else {
            work->scl.x = 0.0F;
        }
        if (card_menu.closing) {
            work->scl.x = 0.0F;
            work->mode++;
        }
        break;
    case DRIVE_CLOSED:
        /* @bug logic: the test reads scl.x, which DRIVE_SHOWN set to 0 as
         * the menu closed, so it always holds (FMOV.S @(R0,R14),FR2 at
         * 0x0c90f77c), and the drive ends the step after the close, before it
         * has shrunk. scl.y, its size, reads as the one meant, an inference */
        if (work->scl.x < 0.1F) {
            task_kill(task);
        }
        break;
    }
    /* @hack spelling: the difference through delta, which subtracts into
     * FR4 at 0x0c90f796 for the FMAC into FR3 at 0x0c90f79c; written in
     * place, 65 of 137 words and 2 pool words differ */
    delta = work->scl.x - work->scl.y;
    work->scl.y += delta * 0.2F;
    if (drive->present && drive->arrived) {
        struct task *preview;

        drive->was_present = TRUE;
        sound_play(SE_MENU_OPEN, 0, 0, 0);
        if (drive->chao.type != CHAO_NOTHING) {
            preview = card_menu_preview_create(&drive->chao);
            /* @bug memory: the preview is null for a Chao slot of type
             * CHAO_PARTNER, which card_admit_type admits, and when the task
             * pool or the heap runs out. the step reads its work through it
             * without a test (0x0c90f7d2) */
#ifdef GCC
            /* repairs the read the tag names: the step places a preview only
             * where one was made, and GCC makes none for an egg */
            if (preview != NULL) {
                preview->work->pos.x = work->pos.x;
                preview->work->pos.y = work->pos.y + 0.65F;
                preview->work->pos.z = work->pos.z + 0.3F;
                preview->work->tick = work->tick * 2;
            }
#else
            preview->work->pos.x = work->pos.x;
            preview->work->pos.y = work->pos.y + 0.65F;
            preview->work->pos.z = work->pos.z + 0.3F;
            preview->work->tick = work->tick * 2;
#endif
        }
    }
    drive_draw(work);
    task_step_children(task);
}

/* the drive at its size, the panel over it while the cursor is on one of
 * its slots, and its card's egg */
static void drive_draw(struct task_work *work)
{
    Float scale = work->scl.y;
    /* @hack spelling: the drive through a local, which adds the egg's
     * offset, 0x224, to the drive's address in one ADD at 0x0c90f872; read
     * in place, the object ends 32 bytes early, and 226 of 245 words
     * differ */
    struct card_menu_drive *drive = &card_menu_drives[work->tick];
    struct chao *egg = &drive->egg;
    Angle ang;

    njSetTexture(&alife_window_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &camera_work->pos);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    if (scale > 0.1F) {
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, scale * 0.15F, scale * 0.13F, scale * 0.15F);
        draw_tree(&card_drive_object);
        njPopMatrix(1);
    }
    if (card_menu.cursor / 2 == work->tick) {
        njSetTexture(&alife_window_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y + 0.65F,
                    work->pos.z - 0.01F);
        njScale(NULL, 0.06F, 0.15F, 0.15F);
        draw_tree(&card_panel_object);
        njPopMatrix(1);
    }
    if (egg->type == CHAO_EGG) {
        njSetTexture(&alife_chao_texlist);
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y + 0.1F, work->pos.z + 0.2F);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, scale * 0.1F, scale * 0.1F, scale * 0.1F);
        if (egg->flags & CHAO_GOLD) {
            draw_tree(&alife_gold_egg_object);
        } else if (egg->flags & CHAO_PAL) {
            draw_tree(&alife_pal_egg_object);
        } else if (egg->flags & CHAO_BEGG) {
            draw_tree(&alife_begg_egg_object);
        } else {
            draw_control_clear(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
            draw_set_material(0.0F, 1.0F, 1.0F, 1.0F);
            njSimpleDrawObject(&alife_egg_object);
        }
        njPopMatrix(1);
        draw_control_set(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
    njPopMatrix(1);
}

/* a button fades in as the menu opens and out as it closes. its work's
 * tick is its number, and scl.z its fade */
static void card_menu_button_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case PART_HIDDEN:
        if (card_menu.open) {
            work->mode++;
        }
        break;
    case PART_FADING_IN:
        work->scl.z += 0.05F;
        if (work->scl.z > 1.0F) {
            work->scl.z = 1.0F;
            work->mode++;
        }
        break;
    case PART_SHOWN:
        if (!card_menu.open) {
            work->mode++;
        }
        break;
    case PART_FADING_OUT:
        work->scl.z -= 0.05F;
        if (work->scl.z < 0.0F) {
            task_kill(task);
        }
        break;
    }
    if (work->scl.z > 0.1F) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        draw_set_material(work->scl.z, 1.0F, 1.0F, 1.0F);
        card_menu_sprite.p.x = work->pos.x;
        card_menu_sprite.p.y = work->pos.y;
        card_menu_sprite.ang = 0;
        njDrawSprite2D(&card_menu_sprite, work->tick, -1.0F, NJD_SPRITE_ALPHA);
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* the cursor fades in as the menu opens and out as it closes, over the
 * slot card_menu.cursor names. it draws only on a frame's first step */
static void card_menu_cursor_step(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: the fade through a local, ahead of menu, which pools
     * 0.05 at 0x0c90fc5c ahead of card_menu's address; written in place, 2
     * of 105 words and 4 pool words differ */
    Float fade = 0.05F;
    struct card_menu *menu = &card_menu;
    /* @hack spelling: open read once into a local, which loads it into R5
     * in the delay slot at 0x0c90fbee for the tests of PART_HIDDEN and
     * PART_SHOWN; read in place, 20 of 105 words and 1 pool word differ */
    Bool open = menu->open;

    switch (work->mode) {
    case PART_HIDDEN:
        if (open) {
            work->mode++;
        }
        break;
    case PART_FADING_IN:
        work->scl.z += fade;
        if (work->scl.z > 1.0F) {
            work->scl.z = 1.0F;
            work->mode++;
        }
        break;
    case PART_SHOWN:
        if (!open) {
            work->mode++;
        }
        break;
    case PART_FADING_OUT:
        work->scl.z -= fade;
        if (work->scl.z < 0.0F) {
            task_kill(task);
        }
        break;
    }
    if (!frame_step) {
        if (work->scl.z > 0.1F) {
            card_menu_sprite.p.x = card_cursor_positions[menu->cursor].x;
            card_menu_sprite.p.y = card_cursor_positions[menu->cursor].y;
            card_menu_sprite.ang = NJM_DEG_ANG(30);
            njDrawSprite2D(&card_menu_sprite, CURSOR_SPRITE, -1.0F,
                           NJD_SPRITE_ALPHA | NJD_SPRITE_ANGLE);
        }
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* the panel fades in as the menu opens and out as it closes, showing
 * card_menu.message */
static void card_menu_panel_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case PART_HIDDEN:
        if (card_menu.open) {
            work->mode++;
        }
        break;
    case PART_FADING_IN:
        work->scl.z += 0.05F;
        if (work->scl.z > 1.0F) {
            work->scl.z = 1.0F;
            work->mode++;
        }
        break;
    case PART_SHOWN:
        if (!card_menu.open) {
            work->mode++;
        }
        break;
    case PART_FADING_OUT:
        work->scl.z -= 0.05F;
        if (work->scl.z < 0.0F) {
            task_kill(task);
        }
        break;
    }
    if (work->scl.z > 0.1F) {
        Float scale = work->scl.z;
        Angle ang;
        int message;

        njSetTexture(&alife_window_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &camera_work->pos);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        njTranslateV(NULL, &work->pos);
        njScale(NULL, 0.25F, scale * 0.12F, scale * 0.15F);
        draw_tree(&card_panel_object);
        njPopMatrix(1);
        njSetTexture(&alife_window_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &camera_work->pos);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        njTranslate(NULL, 0.0F, -0.49F, -5.5F);
        njScale(NULL, 0.4F, scale * 0.45F, scale * 0.15F);
        draw_set_material(1.0F, 1.0F, 0.5F, -0.2F);
        draw_tree(&card_panel_object);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPopMatrix(1);
        message = card_menu.message;
        card_message_sprite.p.x = card_message_offsets[message] + MESSAGE_X;
        card_message_sprite.p.y = MESSAGE_Y;
        card_message_sprite.sx = 1.5F;
        card_message_sprite.sy = scale * 1.5F;
        njSetTexture(&card_message_texlist);
        njDrawSprite2D(&card_message_sprite, card_message_parts[message][0],
                       -1.0F, NJD_SPRITE_ALPHA);
        if (card_message_parts[message][1] != -1) {
            njDrawSprite2D(&card_message_sprite, card_message_parts[message][1],
                           -1.0F, NJD_SPRITE_ALPHA);
        }
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* the notice: a panel of its own, which shows the message its work's
 * submode holds as it fades in, holds and fades out */
static void notice_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case PART_HIDDEN:
        work->mode++;
        sound_play(SE_CARD_NOTICE, 0, 0, 0);
        break;
    case PART_FADING_IN:
        work->scl.z += 0.05F;
        if (work->scl.z > 1.0F) {
            work->scl.z = 1.0F;
            work->mode++;
        }
        break;
    case PART_SHOWN:
        if (TASK_TIMER(work)++ > NOTICE_HOLD) {
            work->mode++;
        }
        break;
    case PART_FADING_OUT:
        work->scl.z -= 0.05F;
        if (work->scl.z < 0.0F) {
            task_kill(task);
        }
        break;
    }
    if (work->scl.z > 0.1F) {
        Float scale = work->scl.z;
        Angle ang;
        int message;

        njSetTexture(&alife_window_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &camera_work->pos);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        njTranslateV(NULL, &work->pos);
        njScale(NULL, 0.25F, scale * 0.12F, scale * 0.15F);
        draw_tree(&card_panel_object);
        njPopMatrix(1);
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        message = work->submode;
        card_message_sprite.p.x = card_message_offsets[message] + MESSAGE_X;
        card_message_sprite.p.y = MESSAGE_Y;
        card_message_sprite.sx = 1.5F;
        card_message_sprite.sy = scale * 1.5F;
        njSetTexture(&card_message_texlist);
        njDrawSprite2D(&card_message_sprite, card_message_parts[message][0],
                       -1.0F, NJD_SPRITE_ALPHA);
        if (card_message_parts[message][1] != -1) {
            njDrawSprite2D(&card_message_sprite, card_message_parts[message][1],
                           -1.0F, NJD_SPRITE_ALPHA);
        }
    }
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

void card_menu_notice_create(Sint16 message)
{
    struct task *task = task_create(TASK_WORK, 3, notice_step);
    struct task_work *work = task->work;

    work->submode = message;
    work->pos = card_notice_pos;
}

/* a task showing record's Chao on its card, which calls out as the cursor
 * comes to its slot. it returns null for a type past CHAO_EGG, and when the
 * task pool or the heap runs out. for an egg it returns task unset */
static struct task *card_menu_preview_create(struct chao *record)
{
    struct task *task;
    /* @hack spelling: the type through a local, which reads it into R4 at
     * 0x0c9101ea and widens it into R0 for the switch; read in place, 2 of
     * 51 words differ */
    Uint8 type = record->type;

    switch (type) {
    /* @bug memory: an egg leaves task unset, and the drive's work is taken
     * for a task. the BT at 0x0c91020c goes past the only write to R14, at
     * 0x0c9102aa, so the test of task at 0x0c9102ae reads R14 as the caller
     * left it: card_menu_drive_step's work. alife_chao_alloc stores the
     * alife_chao over the work's pos.y, as task->motion (MOV.L R4,@(36,R14)
     * at 0x0c900040), and the drive step writes the preview's place through
     * the work's pos.x, read as the preview's work (0x0c90f7d2) */
    /* NOLINTNEXTLINE(clang-diagnostic-sometimes-uninitialized) */
    case CHAO_EGG:
#ifdef GCC
        /* repairs the unset task the tag names: an egg gets no preview */
        task = NULL;
#endif
        break;
    case CHAO_CHILD:
    case CHAO_STANDARD:
    case CHAO_SWIM:
    case CHAO_FLY:
    case CHAO_RUN:
    case CHAO_POWER:
    case CHAO_CHAOS:
        task = task_create(TASK_WORK, 3, preview_init);
        break;
    default:
        return NULL;
    }
    record->unk_17a = 1;
    if (task && !alife_chao_alloc(task, record)) {
        /* @bug memory: writes the step of a task already freed, by the JSR
         * to task_kill at 0x0c9102c4. alife_chao_alloc destroys the task when
         * it fails, which puts it back on the free list */
        task_kill(task);
        return NULL;
    }
    return task;
}

static void preview_init(struct task *task)
{
    task->destructor = preview_destroy;
    preview_setup(task);
    task->step = preview_step;
}

static void preview_destroy(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;

    fun_0c9002ac(work, chao);
}

/* cover a jewel Chao's COAT_NODES with its jewel's texture, mapped as an
 * environment. a jewel past 3 takes TEX_AME */
static void preview_wear_jewel(struct task_work *work, struct alife_chao *chao)
{
    int nodes[14] = COAT_NODES;
    Uint16 textures[4] = {TEX_AME, TEX_EME, TEX_RUB, TEX_SAF};
    int *node = nodes;
    struct chao_shape *shape = &chao->shape;
    Uint8 jewel = chao->record->jewel;

    if (jewel > 3) {
        jewel = 0;
    }
    while (*node != -1) {
        NJS_OBJECT *object = shape->objects[*node];
        NJS_MATERIAL *materials = object->model->mats;
        int count = object->model->nbMat;
        int i;

        for (i = 0; i < count; ++i) {
            materials[i].attr_texId = textures[jewel];
            materials[i].attrflags |= NJD_FLAG_USE_ENV;
        }
        ++node;
    }
}

/* the same with TEX_GOLD_COAT */
static void preview_wear_gold(struct task_work *work, struct alife_chao *chao)
{
    int nodes[14] = COAT_NODES;
    struct chao_shape *shape = &chao->shape;
    int *node = nodes;

    while (*node != -1) {
        NJS_OBJECT *object = shape->objects[*node];
        NJS_MATERIAL *materials = object->model->mats;
        int count = object->model->nbMat;
        int i;

        for (i = 0; i < count; ++i) {
            materials[i].attr_texId = TEX_GOLD_COAT;
            materials[i].attrflags |= NJD_FLAG_USE_ENV;
        }
        ++node;
    }
}

/* the same with TEX_PAL_COAT */
static void preview_wear_pal(struct task_work *work, struct alife_chao *chao)
{
    int nodes[14] = COAT_NODES;
    struct chao_shape *shape = &chao->shape;
    int *node = nodes;

    while (*node != -1) {
        NJS_OBJECT *object = shape->objects[*node];
        NJS_MATERIAL *materials = object->model->mats;
        int count = object->model->nbMat;
        int i;

        for (i = 0; i < count; ++i) {
            materials[i].attr_texId = TEX_PAL_COAT;
            materials[i].attrflags |= NJD_FLAG_USE_ENV;
        }
        ++node;
    }
}

/* set up the preview's Chao at a fifth of its size: its motion, icon,
 * face and shape, then its coat */
static void preview_setup(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;

    work->mode = PREVIEW_IDLE;
    work->submode = 0;
    /* @hack redundant: the timer, cleared again below before anything
     * reads it (0x0c9105b4, 0x0c9105ec); without this clear, 106 of 136
     * words differ */
    work->timer = 0;
    work->ang.x = 0;
    work->ang.z = 0;
    chao->spin = 0;
    chao->unk_080 = 0;
    chao->v_pos = chao->record->v_pos;
    chao->h_pos = chao->record->h_pos;
    chao->growth = chao->record->growth;
    work->timer = 0;
    alife_motion_init(work, chao);
    fun_0c90d634(work, chao);
    alife_icon_clear(work, chao);
    alife_face_init(work, chao);
    alife_shape_init(work, chao);
    alife_shape_blend(work, chao);
    alife_shape_set_parts(work, chao);
    alife_shape_set_wins(work, chao);
    chao->flags &= ~(CHAO_SHOW_NAME | CHAO_IN_COCOON | CHAO_UNK_8);
    chao->record->flags &= ~CHAO_GROWTH_DUE;
    chao->name_alpha = 0;
    chao->name_frames = 0;
    chao->scale = 0.2F;
    if (chao->record->flags & CHAO_JEWEL) {
        preview_wear_jewel(work, chao);
    } else if (chao->record->flags & CHAO_GOLD) {
        preview_wear_gold(work, chao);
    } else if (chao->record->flags & CHAO_PAL) {
        preview_wear_pal(work, chao);
    }
}

/* PREVIEW_IDLE: motion 5, until the cursor comes to the preview's slot.
 * then the Chao calls out, with one of two calls by rand, and turns glad */
static void preview_idle(struct task_work *work, struct alife_chao *chao)
{
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 5;
        work->submode++;
        /* @hack spelling: the return, which gives the case an epilogue of its
         * own at 0x0c910728; as a break, 5 of 67 words differ */
        return;
    case 1:
        if (card_menu.cursor == work->tick) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            chao->last_mode = work->mode;
            work->mode = PREVIEW_GLAD;
            work->submode = 0;
            work->timer = 0;
            chao->mode_timer = 0;
            if ((Float)rand() * RAND_SCALE > 0.5F) {
                sound_play(SE_PREVIEW_CALL, 0, 0, 0);
            } else {
                sound_play(SE_PREVIEW_CALL_2, 0, 0, 0);
            }
        }
        break;
    /* @hack redundant: the default's return, which gives it an epilogue of its
     * own at 0x0c910712; without it, 17 of 67 words differ */
    default:
        return;
    }
}

/* PREVIEW_GLAD: motion 6, until the cursor leaves the preview's slot. the
 * icon shows the glad symbol, as ICON_GLAD_1, with its timer at 90 */
static void preview_glad(struct task_work *work, struct alife_chao *chao)
{
    switch (work->submode) {
    case 0:
        chao->motion.flags |= MOTION_NEW;
        chao->motion.id = 6;
        work->submode++;
        chao->icon.kind = ICON_GLAD_1;
        chao->icon.symbol.state = 0;
        chao->icon.ball.state = 0;
        chao->icon.timer = 90;
        /* @hack spelling: the return, which ends the case in an RTS of its own
         * at 0x0c910828; as a break, 1 of 48 words differ */
        return;
    case 1:
        if (card_menu.cursor != work->tick) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            chao->last_mode = work->mode;
            work->mode = PREVIEW_IDLE;
            work->submode = 0;
            work->timer = 0;
            chao->mode_timer = 0;
        }
        break;
    /* @hack redundant: the default's return, which ends it in an RTS of its own
     * at 0x0c9107c6; without it, 1 of 48 words differ */
    default:
        return;
    }
}

/* the preview's step, until the menu closes or its record empties. its
 * work's tick is its slot */
static void preview_step(struct task *task)
{
    struct task_work *work = task->work;
    struct alife_chao *chao = (struct alife_chao *)task->motion;

    switch (work->mode) {
    case PREVIEW_IDLE:
        preview_idle(work, chao);
        break;
    case PREVIEW_GLAD:
        preview_glad(work, chao);
        break;
    }
    alife_motion_step(work, chao);
    alife_face_step(work, chao);
    preview_draw(work, chao);
    work->timer++;
    task_step_children(task);
    if (card_menu.closing) {
        task_kill(task);
    }
    if (chao->record->type == CHAO_NOTHING) {
        task_kill(task);
    }
}

/* the Chao in the camera's frame while it has CHAO_SHAPE_SHOWN, posed by
 * its motion or blending into the next. its icon stands at NODE_ICON
 * unless it has CHAO_IN_COCOON, and chao_name_step draws its name from an
 * alpha of 255. it draws only on a frame's first step */
static void preview_draw(struct task_work *work, struct alife_chao *chao)
{
    Float width = chao->scale * work->scl.x;
    Float height = chao->scale * work->scl.y;
    struct chao_motion *motion = &chao->motion;
    struct chao_motion_entry *current = &motion->table[motion->current];
    struct chao_motion_entry *next = &motion->table[motion->next];

    alife_squash_step(work, chao);
    if (frame_step) {
        return;
    }
    draw_control_clear(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
    if (chao->flags & CHAO_SHAPE_SHOWN) {
        Angle ang;

        njSetTexture(&alife_chao_texlist);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPushMatrix(NULL);
        njTranslateV(NULL, &camera_work->pos);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        TURN_VIA(njRotateX, camera_work->ang.x, ang);
        njTranslateV(NULL, &work->pos);
        TURN_VIA(njRotateY, work->ang.y, ang);
        njScale(NULL, width, height, width);
        if (motion->flags & MOTION_BLEND) {
            /* the distance goes unused, here and below. alife_chao_show
             * picks a simpler model by it, but the preview draws its own
             * shape at any distance */
            (void)njDistanceP2P(&work->pos, &camera_work->pos);
            if (chao->record->flags & CHAO_JEWEL) {
                NJS_MOTION_LINK link;
                NJS_ACTION_LINK action;

                link.motion[0] = current->motion;
                link.motion[1] = next->motion;
                link.frame[0] = motion->frame;
                link.frame[1] = motion->next_frame;
                action.motionlink = &link;
                action.object = chao->shape.model;
                draw_action_link(&action, motion->ratio);
            } else if (chao->record->flags & (CHAO_GOLD | CHAO_PAL)) {
                NJS_MOTION_LINK link;
                NJS_ACTION_LINK action;

                link.motion[0] = current->motion;
                link.motion[1] = next->motion;
                link.frame[0] = motion->frame;
                link.frame[1] = motion->next_frame;
                action.motionlink = &link;
                action.object = chao->shape.model;
                if (chao->record->flags & CHAO_BEGG) {
                    njSimpleActionLink(&action, motion->ratio);
                } else {
                    draw_action_link(&action, motion->ratio);
                }
            } else {
                NJS_MOTION_LINK link;
                NJS_ACTION_LINK action;

                link.motion[0] = current->motion;
                link.motion[1] = next->motion;
                link.frame[0] = motion->frame;
                link.frame[1] = motion->next_frame;
                action.motionlink = &link;
                action.object = chao->shape.model;
                njSimpleActionLink(&action, motion->ratio);
            }
            {
                NJS_POINT3 origin = {0.0F, 0.0F, 0.0F};
                NJS_ACTION action;
                NJS_MATRIX matrix;
                NJS_POINT3 pos;
                NJS_POINT3 next_pos;
                Float ratio = motion->ratio;
                Float rest = 1.0F - ratio;

                action.object = chao->shape.model;
                action.motion = current->motion;
                pose_action(NULL, &action, motion->frame, NODE_ICON, &matrix);
                njCalcPoint(&matrix, &origin, &pos);
                action.motion = next->motion;
                pose_action(NULL, &action, motion->next_frame, NODE_ICON,
                            &matrix);
                njCalcPoint(&matrix, &origin, &next_pos);
                chao->icon.pos.x = (pos.x * rest) + (next_pos.x * ratio);
                chao->icon.pos.y = (pos.y * rest) + (next_pos.y * ratio);
                chao->icon.pos.z = (pos.z * rest) + (next_pos.z * ratio);
                alife_icon_step(work, chao);
            }
            motion->ratio += next->blend;
            if (motion->ratio > 1.0F) {
                motion->ratio = 0.0F;
                motion->frame = motion->next_frame;
                motion->current = motion->next;
                motion->end = next->end;
                motion->flags &= ~MOTION_BLEND;
                motion->flags |= MOTION_CHANGED;
                motion->rate = 1.0F;
            }
        } else {
            (void)njDistanceP2P(&work->pos, &camera_work->pos);
            if (chao->record->flags & CHAO_JEWEL) {
                draw_motion(chao->shape.model, current->motion, motion->frame);
            } else if (chao->record->flags & (CHAO_GOLD | CHAO_PAL)) {
                if (chao->record->flags & CHAO_BEGG) {
                    njSimpleDrawMotion(chao->shape.model, current->motion,
                                       motion->frame);
                } else {
                    draw_motion(chao->shape.model, current->motion,
                                motion->frame);
                }
            } else {
                njSimpleDrawMotion(chao->shape.model, current->motion,
                                   motion->frame);
            }
            if (!(chao->flags & CHAO_IN_COCOON)) {
                NJS_POINT3 origin = {0.0F, 0.0F, 0.0F};
                NJS_ACTION action;
                NJS_MATRIX matrix;

                action.object = chao->shape.model;
                action.motion = current->motion;
                pose_action(NULL, &action, motion->frame, NODE_ICON, &matrix);
                njCalcPoint(&matrix, &origin, &chao->icon.pos);
                alife_icon_step(work, chao);
                /* @hack redundant: flag 0x1 tested to no effect, which keeps
                 * the TST at 0x0c910dc6; without it, 103 of 508 words and 6
                 * pool words differ */
                if (chao->flags & 0x1) {
                }
            }
        }
        njPopMatrix(1);
    }
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    draw_attr_restore();
    chao->name_alpha = 255;
    chao_name_step(work, chao);
    draw_control_set(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}
