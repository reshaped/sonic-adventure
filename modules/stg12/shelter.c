/* Hot Shelter's stage task, and its sky task, which sets each act's view.
 * in acts 0 to 2, the sky task also sets the fog and the far clip */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "draw/light.h"
#include "event/play.h"
#include "object/switch.h"
#include "object/target.h"
#include "player/player.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "stg12/box_switch.h"
#include "stg12/cargo_ride.h"
#include "stg12/dam.h"
#include "stg12/shelter.h"
#include "stg12/upper_pool.h"
#include "stg12/water.h"
#include "system/task.h"
#include "system/util.h"

/* the switch that starts EV00D9 for Big in act 0, and the event. in EV00D9,
 * Big spots Froggy */
#define EVENT_SWITCH 20
#define EVENT_BIG 217

/* the far clips the sky sets. it starts act 0 with VIEW_FAR, and acts 1 and
 * 2 with START_FAR. a change into any of the three sets VIEW_FAR */
#define VIEW_FAR (-1000.0F)
#define START_FAR (-1200.0F)

enum shelter_mode {
    SHELTER_INIT,
    SHELTER_RUN,
    /* Big in act 0, waiting for EVENT_SWITCH. with the dam switch kept, it
     * restores his progress instead */
    SHELTER_BIG_WAIT,
    SHELTER_BIG_EVENT,
    SHELTER_BIG_KEEP, /* Big in act 0, keeping his progress for a retry */
    SHELTER_END /* no code sets it */
};

enum shelter_sky_mode { SKY_INIT, SKY_RUN, SKY_END };

struct shelter_sky_vars {
    Uint8 unk_00[8];
    /* @hack type: the act the sky step last saw, as an array of one.
     * SHC reaches it through a pointer, work plus 16 (MOV R14,R2 and ADD
     * #16,R2 at 0x0c900486); as a plain Sint8, it uses R0 as the offset, and
     * 64 of 145 words differ */
    Sint8 act[1];
};

#define VARS(work) ((struct shelter_sky_vars *)&(work)->vars)

/* whether the dam switch has been on, and whether Big has pulled the plug.
 * SHELTER_BIG_KEEP sets them as Big plays act 0, and nothing clears them,
 * so they outlast a retry */
static Bool shelter_dam_switch_kept = FALSE;
static Bool shelter_pulled_kept = FALSE;

/* the act whose start ran last, and the act now */
static Uint8 shelter_act_started;
static Uint8 shelter_act_now;
static NJS_FOG_TABLE shelter_fog;

static void shelter_init(struct task *task);
static void shelter_act0_start(struct task *task);
static void shelter_act1_start(struct task *task);
static void shelter_act2_start(struct task *task);
static void shelter_end(struct task *task);
static void shelter_view_reset(void);
static void shelter_act0_fog(void);
static void shelter_act1_fog(void);
static void shelter_act2_fog(void);

void shelter_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SHELTER_INIT:
        shelter_init(task);
        shelter_act_now = shelter_act_started = stage_get_stage_act();
        /* @hack spelling: a switch of one case, which tests the act with EXTU.B
         * and CMP/EQ #0 (0x0c900058, 0x0c90005a); as an if, it tests it with
         * TST, and 111 of 219 words differ */
        switch (shelter_act_now) {
        case 0:
            if (get_character() == CHARACTER_BIG) {
                work->mode = SHELTER_BIG_WAIT;
            }
            break;
        }
        /* falls through: SHELTER_RUN runs this step too */
    case SHELTER_RUN:
        shelter_act_now = stage_get_stage_act();
        if (shelter_act_now != shelter_act_started) {
            shelter_act_started = shelter_act_now;
            switch (shelter_act_now) {
            case 0:
                light_load_act();
                shelter_act0_start(task);
                break;
            case 1:
                light_load_act();
                shelter_act1_start(task);
                break;
            case 2:
                light_load_act();
                shelter_act2_start(task);
                break;
            case 3:
                light_load_act();
                stream_reset();
                stream_request_and_save_music(MUSIC_SHELTER2);
                /* @hack spelling: the two resets as one expression, which calls
                 * switch_reset with a JSR (0x0c9000fc) and returns through the
                 * step's shared exit; as two statements, SHC tail-calls
                 * switch_reset with a JMP, and 116 of 219 words differ */
                target_reset(), switch_reset();
                break;
            case 4:
                light_load_act();
                shelter_act2_start(task);
                break;
            }
        }
        break;
    case SHELTER_BIG_WAIT:
        if (!shelter_dam_switch_kept) {
            /* @hack spelling: the switch compared with TRUE, which SHC tests
             * with CMP/EQ #1 (0x0c90016e). switch_is_on returns only TRUE or
             * FALSE. as a plain test, SHC writes TST, and 2 of 219 words
             * differ */
            if (switch_is_on(EVENT_SWITCH) == TRUE) {
                event_load(EVENT_BIG);
                event_start();
                work->mode = SHELTER_BIG_EVENT;
            }
        } else {
            /* restore Big's progress. the dam's and the upper pool's starts
             * cleared act 0's restore of it */
            /* @hack redundant: shelter_dam_switch_kept tested again where it
             * holds, which SHC writes as a BT on the first test's T bit
             * (0x0c900186); without it, 41 of 219 words differ */
            if (shelter_dam_switch_kept) {
                switch_set(DAM_SWITCH, TRUE);
            }
            if (shelter_pulled_kept) {
                shelter_big_flags |= BIG_PULLED;
            }
            work->mode = SHELTER_BIG_KEEP;
        }
        break;
    case SHELTER_BIG_EVENT:
        if (!event_in_control()) {
            stream_reset();
            stream_request_and_save_music(MUSIC_SHELTER1);
            work->mode = SHELTER_BIG_KEEP;
        }
        break;
    case SHELTER_BIG_KEEP:
        if (switch_is_on(DAM_SWITCH)) {
            shelter_dam_switch_kept = TRUE;
        }
        if (shelter_big_flags) {
            shelter_pulled_kept = TRUE;
        }
        break;
    case SHELTER_END:
        shelter_end(task);
        break;
    }
}

static void shelter_init(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 stage_act = stage_get_stage_act();

    switch ((Uint8)stage_act) {
    case 0:
        shelter_act0_start(task);
        work->mode = SHELTER_RUN;
        break;
    case 1:
        shelter_act1_start(task);
        work->mode = SHELTER_RUN;
        break;
    case 2:
        shelter_act2_start(task);
        work->mode = SHELTER_RUN;
        break;
    case 3:
        stream_reset();
        stream_request_and_save_music(MUSIC_SHELTER2);
        target_reset();
        switch_reset();
        work->mode = SHELTER_RUN;
        break;
    case 4:
        shelter_act2_start(task);
        work->mode = SHELTER_RUN;
        break;
    default:
        target_reset();
        work->mode = SHELTER_RUN;
        break;
    }
}

static void shelter_act0_start(struct task *task)
{
    stream_reset();
    stream_request_and_save_music(MUSIC_SHELTER1);
    shelter_water_start();
    target_reset();
    switch_reset();
    if (get_character() == CHARACTER_BIG) {
        /* this restore doesn't hold. the dam's and the upper pool's starts
         * run later in this pass of the tasks. they clear the dam switch and
         * shelter_big_flags again. SHELTER_BIG_WAIT, which follows a start
         * in act 0, restores them while the dam switch is kept */
        if (shelter_dam_switch_kept) {
            switch_set(DAM_SWITCH, TRUE);
        }
        if (shelter_pulled_kept) {
            shelter_big_flags |= BIG_PULLED;
        }
    }
}

static void shelter_act1_start(struct task *task)
{
    stream_reset();
    stream_request_and_save_music(MUSIC_SHELTER2);
    target_reset();
    box_switch_reset();
    switch_reset();
}

static void shelter_act2_start(struct task *task)
{
    stream_reset();
    stream_request_and_save_music(MUSIC_SHELTER2);
    target_reset();
    switch_reset();
    cargo_ride_start();
}

static void shelter_end(struct task *task)
{
}

void shelter_sky_step(struct task *task)
{
    struct task_work *work = task->work;
    Uint16 act = stage_get_stage_act();

    /* @hack spelling: the mask in a statement of its own, which copies the act
     * to R4 before its EXTU.B (0x0c90038c, 0x0c900390) and keeps work in R14;
     * masked in act's initializer, the act is extended from R0, work takes
     * R13, and 53 of 145 words differ */
    act &= 0xff;
    draw_control_save();
    draw_control_clear(NJD_CONTROL_3D_DEPTH_QUEUE);
    switch (work->mode) {
    case SKY_INIT:
        switch (act) {
        case 0:
            shelter_view_reset();
            shelter_act0_fog();
            njClipZ(-3.0F, VIEW_FAR);
            break;
        case 1:
            shelter_view_reset();
            shelter_act1_fog();
            njClipZ(-3.0F, START_FAR);
            break;
        case 2:
            shelter_view_reset();
            shelter_act2_fog();
            njClipZ(-3.0F, START_FAR);
            break;
        case 3:
        case 4:
        case 5:
            shelter_view_reset();
            break;
        }
        VARS(work)->act[0] = act;
        work->mode = SKY_RUN;
        break;
    case SKY_RUN:
        if (act != VARS(work)->act[0]) {
            VARS(work)->act[0] = act;
            switch (act) {
            case 0:
                shelter_view_reset();
                shelter_act0_fog();
                njClipZ(-3.0F, VIEW_FAR);
                break;
            case 1:
                shelter_view_reset();
                shelter_act1_fog();
                njClipZ(-3.0F, VIEW_FAR);
                break;
            case 2:
                shelter_view_reset();
                shelter_act2_fog();
                njClipZ(-3.0F, VIEW_FAR);
                break;
            }
        }
        break;
    case SKY_END:
        /* @hack redundant: a switch of six empty cases, which SHC still writes
         * as six CMP/EQ tests of the act (0x0c9004de to 0x0c9004f4); without
         * it, 44 of 145 words differ */
        switch (act) {
        case 0:
            break;
        case 1:
            break;
        case 2:
            break;
        case 3:
            break;
        case 4:
            break;
        case 5:
            break;
        }
        task_kill(task);
        break;
    default:
        task_kill(task);
        break;
    }
    draw_control_restore();
}

static void shelter_view_reset(void)
{
    njSetDepthQueue(-100.0F, -500.0F);
    camera_set_perspective(NJM_DEG_ANG(70));
    njSetBackColor(0, 0, 0);
}

static void shelter_act0_fog(void)
{
    njSetFogColor(0xff000000);
    njGenerateFogTable3(shelter_fog, -350.0F, -950.0F);
    njSetFogTable(shelter_fog);
    njFogEnable();
}

static void shelter_act1_fog(void)
{
    njSetFogColor(0xff000000);
    njGenerateFogTable3(shelter_fog, -550.0F, -850.0F);
    njSetFogTable(shelter_fog);
    njFogEnable();
}

static void shelter_act2_fog(void)
{
    njSetFogColor(0xff000000);
    njGenerateFogTable3(shelter_fog, -550.0F, -800.0F);
    njSetFogTable(shelter_fog);
    njFogEnable();
}
