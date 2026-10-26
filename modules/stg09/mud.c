/* O MUD tints the players' palette by player 0's mud level, and O MUDOUT
 * lowers the level */
#include <ninja.h>
#include <sg_xpt.h>
#include <string.h>

#include "debug/editor.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "stg09/mud.h"
#include "system/task.h"

/* destroyed beyond about 600 units of the camera and player 0 */
#define RANGE ((600.0F * 600.0F) + 10.0F)

/* O MUDOUT lowers the level by MUD_OUT_DROP each step for MUD_OUT_STEPS
 * steps. SE_MUD_OUT plays MUD_OUT_SOUND_DELAY steps after player 0 enters,
 * if player 0's mode is still PLAYER_MODE_MUD_OUT */
#define MUD_OUT_STEPS 150
#define MUD_OUT_DROP 0.003F
#define MUD_OUT_SOUND_DELAY 68

/* while a change of level is pending, O MUD counts its steps. it tints the
 * palette by the level on the step after it has counted TINT_DELAY */
#define TINT_DELAY 5

#define SE_MUD_OUT 262

struct mud_out_vars {
    /* 0 while no sound is pending. player 0's entry sets it to 1, and it
     * rises by 1 each step until it passes MUD_OUT_SOUND_DELAY */
    Uint32 sound;
};

struct mud_vars {
    Uint8 unk_08[4];
    Uint32 *muddy; /* a copy of MUDDY_PALETTE */
    Uint32 *clean; /* a copy of the players' palette as it was */
};

#define OUT_VARS(work) ((struct mud_out_vars *)&(work)->vars)
#define VARS(work) ((struct mud_vars *)&(work)->vars)

static Float mud_level = 0.0F;
/* the level changed since O MUD last tinted the palette */
static int mud_changed = 0;

static void mud_out_run(struct task *task);
static void mud_destroy(struct task *task);
static void mud_run(struct task *task);

void mud_out_step(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_try_leave_by(task, RANGE)) {
        task->step = mud_out_run;
        work->timer = 0;
        OUT_VARS(work)->sound = 0;
    }
}

/* set player 0's mode to PLAYER_MODE_MUD_OUT the first time it is on land
 * within scl.x of O MUDOUT. then lower the level for MUD_OUT_STEPS steps */
static void mud_out_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> scl ");
        casino_draw_sphere(&work->pos, work->scl.x > 0.0F ? work->scl.x : 0.0F);
        return;
    }
    if (OUT_VARS(work)->sound > 0) {
        /* @hack spelling: the test for the wrap to 0, written <= 0 for the
         * image's CMP/HI (0x0c91f4cc). sound is a Uint32, so <= 0 holds only
         * at 0. the decrement undoes the wrap. as == 0, 2 of mud_out_run's 127
         * words differ */
        if (++OUT_VARS(work)->sound <= 0) {
            --OUT_VARS(work)->sound;
        }
        if (OUT_VARS(work)->sound > MUD_OUT_SOUND_DELAY) {
            if (player_work[0]->submode == PLAYER_MODE_MUD_OUT) {
                sound_play(SE_MUD_OUT, 0, 0, 0);
            }
            OUT_VARS(work)->sound = 0;
        }
    }
    /* @hack spelling: player assigned in the test, which keeps it on the stack
     * (MOV.L R3,@(8,R15) at 0x0c91f500). assigned apart, 91 of
     * mud_out_run's 127 words differ */
    if ((player = player_work[0])->flags & PLAYER_ON_LAND) {
        if (njDistanceP2P(&work->pos, &player->pos) < work->scl.x) {
            player_set_mode(0, PLAYER_MODE_MUD_OUT);
            work->scl.x = -1.0F;
            work->timer = 1;
            OUT_VARS(work)->sound = 1;
        }
    }
    if (TASK_TIMER(work) > 0) {
        mud_set_level(mud_get_level() - MUD_OUT_DROP);
        work->timer++;
        if (TASK_TIMER(work) > MUD_OUT_STEPS) {
            work->timer = 0;
        }
    }
}

Float mud_get_level(void)
{
    return mud_level;
}

void mud_set_level(Float level)
{
    if (level > 1.0F) {
        level = 1.0F;
    }
    if (level < 0.0F) {
        level = 0.0F;
    }
    if (mud_level != level) {
        mud_changed = 1;
        mud_level = level;
    }
}

void mud_step(struct task *task)
{
    struct task_work *work = task->work;

    task->destructor = mud_destroy;
    task->step = mud_run;
    if (placement_editing(task)) {
        VARS(work)->clean = NULL;
        VARS(work)->muddy = NULL;
        return;
    }
    VARS(work)->clean = heap_alloc(PALETTE_SIZE);
    VARS(work)->muddy = heap_alloc(PALETTE_SIZE);
    memcpy(VARS(work)->clean, draw_palettes[DRAW_PLAYER_PALETTE], PALETTE_SIZE);
    memcpy(VARS(work)->muddy, MUDDY_PALETTE, PALETTE_SIZE);
    TASK_TIMER(work) = 0xffff;
}

static void mud_destroy(struct task *task)
{
    struct task_work *work = task->work;

    if (!placement_editing(task)) {
        if (VARS(work)->clean && VARS(work)->muddy) {
            casino_blend_palette(VARS(work)->clean, VARS(work)->muddy,
                                 draw_palettes[DRAW_PLAYER_PALETTE], 0.0F);
        }
        heap_free(VARS(work)->clean);
        heap_free(VARS(work)->muddy);
    }
}

static void mud_run(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_editing(task)) {
        njPrintC(NJM_LOCATION(20, EDITOR_ROW_SCL_X), "Only 1 Obj!");
        return;
    }
    /* @bug logic: TASK_TIMER reads the timer unsigned, so the test never
     * holds (EXTU.W and CMP/PZ at 0x0c91f6c2 and 0x0c91f6c4). the 0xffff
     * mud_step leaves in the timer reads as 65535, not -1, and the first
     * step tints no palette */
    if (TASK_TIMER(work) < 0) {
        /* @hack spelling: the palette through a local of its own, which SHC
         * loads into R6 ahead of the level (0x0c91f6d0, 0x0c91f6d2). passed
         * directly, the two trade, and 2 of mud_run's 59 words differ */
        Uint32 *palette = draw_palettes[DRAW_PLAYER_PALETTE];

        casino_blend_palette(VARS(work)->clean, VARS(work)->muddy, palette,
                             mud_level);
        work->timer = 0;
        mud_changed = 0;
    }
    if (mud_changed) {
        work->timer++;
        if (TASK_TIMER(work) > TINT_DELAY) {
            /* @hack spelling: the palette through a local of its own, which SHC
             * loads into R6 ahead of the level (0x0c91f6fa, 0x0c91f6fc).
             * passed directly, the two trade, and 2 of mud_run's 59 words
             * differ */
            Uint32 *palette = draw_palettes[DRAW_PLAYER_PALETTE];

            casino_blend_palette(VARS(work)->clean, VARS(work)->muddy, palette,
                                 mud_level);
            mud_changed = 0;
            work->timer = 0;
        }
    }
}
