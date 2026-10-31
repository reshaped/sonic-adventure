/* the sky chase's stage task, with a step for each act. each step moves
 * the tornado's point along the act's course. it starts the act's event,
 * then the score. the first step also makes an actor for each of the
 * course's scenes. this file holds the kinds of object the acts place too */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "hud/hud.h"
#include "object/placement.h"
#include "object/ring.h"
#include "player/player.h"
#include "shooting/arm.h"
#include "shooting/boss.h"
#include "shooting/camera.h"
#include "shooting/carrier.h"
#include "shooting/event.h"
#include "shooting/furniture.h"
#include "shooting/hud.h"
#include "shooting/kirai.h"
#include "shooting/object_list.h"
#include "shooting/renzako.h"
#include "shooting/reticle.h"
#include "shooting/rocket.h"
#include "shooting/scene_renzako.h"
#include "shooting/scene_zako.h"
#include "shooting/scenery.h"
#include "shooting/shooting.h"
#include "shooting/streaks.h"
#include "shooting/zako.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/task.h"

/* each act's course, and its count of points */
extern NJS_POINT3 act1_path[];
extern int act1_path_count;
extern NJS_POINT3 act2_path[];
extern int act2_path_count;
/* the path act 2's stage task gives shooting_path once the course is
 * flown. the tornado holds at its point CARRIER_POINT through the carrier
 * fight */
extern NJS_POINT3 act2_end_path[];
extern int act2_end_path_count;

struct shooting_flight shooting_flight;
int shooting_path_index;
Bool shooting_path_running;
struct shooting_actor shooting_actors[SHOOTING_ACTORS];
int shooting_actor_count;
NJS_POINT3 *shooting_path;
int shooting_path_count;
int shooting_act;
int dat_0cad53d8;
Float shooting_lives;
int shooting_score;
int shooting_course_part;
int dat_0cad53e8;
int dat_0cad53ec;
int shooting_request;
int shooting_stage_mode;
struct shooting_scene *shooting_scenes;
int dat_0cad53fc;
int dat_0cad5400;

/* the placement range, squared, of the kirai, the hodai and the arm */
#define NEAR (5000.0F * 5000.0F)
/* the range most KIND_ALWAYS kinds carry, which placement never reads */
#define ALWAYS_RANGE 20000.0F

static struct object_kind shooting_object_kinds[] = {
    {TASK_WORK, 3, KIND_ALWAYS, 0.0F, NULL, ring_step, "RING   "},
    {TASK_WORK, 3, KIND_ALWAYS, ALWAYS_RANGE, NULL, zako1_step, "ZAKO1"},
    {TASK_WORK, 3, KIND_ALWAYS, ALWAYS_RANGE, NULL, furniture_tv1_step, "TV1"},
    {TASK_WORK, 3, KIND_ALWAYS, ALWAYS_RANGE, NULL, furniture_isu1_step,
     "ISU1"},
    {TASK_WORK, 3, KIND_ALWAYS, ALWAYS_RANGE, NULL, furniture_isu2_step,
     "ISU2"},
    {TASK_WORK, 3, KIND_ALWAYS, ALWAYS_RANGE, NULL, furniture_tansu_step,
     "TANSU"},
    {TASK_WORK, 3, KIND_ALWAYS, ALWAYS_RANGE, NULL, furniture_kabin_step,
     "KABIN"},
    {TASK_WORK, 3, KIND_ALWAYS, ALWAYS_RANGE, NULL, furniture_kaiga_step,
     "KAIGA"},
    {TASK_WORK, 3, KIND_ALWAYS, ALWAYS_RANGE, NULL, furniture_tv2_step, "TV2"},
    {TASK_WORK, 3, KIND_ALWAYS, ALWAYS_RANGE, NULL, furniture_reizou_step,
     "REIZOU"},
    {TASK_WORK, 3, KIND_ALWAYS, ALWAYS_RANGE, NULL, furniture_sofa_step,
     "SOFA"},
    {TASK_WORK, 3, KIND_ALWAYS, ALWAYS_RANGE, NULL, furniture_tana_step,
     "TANA"},
    {TASK_WORK, 3, KIND_ALWAYS, ALWAYS_RANGE, NULL, furniture_yakan_step,
     "YAKAN"},
    {TASK_WORK, 3, KIND_RANGE, NEAR, NULL, kirai_step, "KIRAI"},
    {TASK_WORK, 3, KIND_RANGE, NEAR, NULL, hodai_step, "HODAI"},
    {TASK_WORK, 3, KIND_ALWAYS, ALWAYS_RANGE, NULL, renzako_step, "RENZAKO"},
    {TASK_WORK, 3, KIND_RANGE, NEAR, NULL, arm_step, "ARM"},
    {TASK_WORK, 3, KIND_ALWAYS, ALWAYS_RANGE, NULL, shooting_rocket_step,
     "ROCKET"},
    {TASK_WORK, 3, KIND_ALWAYS, ALWAYS_RANGE, NULL, boss_step, "BOSS"},
    {TASK_WORK, 6, KIND_ALWAYS, ALWAYS_RANGE, NULL, reticle_step, "TARGET"},
};

#define KINDS (sizeof(shooting_object_kinds) / sizeof(shooting_object_kinds[0]))

struct object_list shooting_object_list = {KINDS, shooting_object_kinds};

/* where 1ST_READ.BIN's loader reads shtpath1.PRS or shtpath2.PRS, whose
 * first word points at the course's scenes */
static struct shooting_scene **shooting_scene_file =
    (struct shooting_scene **)0x0cb80000;

/* the field of view the chase flies with, and the default. act 2 puts the
 * default back for the carrier fight */
#define SHOOTING_FOV NJM_DEG_ANG(100)
#define DEFAULT_FOV NJM_DEG_ANG(70)

/* act 1's event starts ACT1_EVENT_EARLY points short of the course's end.
 * act 2's starts at ACT2_EVENT_POINT, where its second half then starts */
#define ACT1_EVENT_EARLY 100
#define ACT2_EVENT_POINT 5500
/* the steps each act's event lasts */
#define EVENT1_STEPS 900
#define EVENT2_STEPS 700
/* SHOOTING_FALL moves the tornado's point on to FALL_END, then enters
 * SHOOTING_DOWN */
#define FALL_END 5500
/* the point of act2_end_path the tornado holds at through the carrier
 * fight */
#define CARRIER_POINT 15

/* act 1's camera swings round the tornado at SWING_POINT, with VOICE_SWING.
 * VOICE_SWUNG plays at SWUNG_POINT, and VOICE_EVENT1 as the event starts */
#define SWING_POINT 8440
#define SWUNG_POINT 8500
#define VOICE_SWING 0x405
#define VOICE_SWUNG 0x406
#define VOICE_EVENT1 0xbd
/* act 2's voices at points of its course. VOICE_EVENT2 also plays as a
 * request starts the event in SHOOTING_FLY */
#define VOICE_1_POINT 5000
#define VOICE_2_POINT 5190
#define VOICE_3_POINT 5300
#define VOICE_EVENT2_POINT 5480
#define VOICE_ACT2_1 0x430
#define VOICE_ACT2_2 0x431
#define VOICE_ACT2_3 0x432
#define VOICE_EVENT2 0x390

static void set_view(void);
static void create_actors(void);
static void shooting_destroy(struct task *task);

void shooting_act1_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SHOOTING_INIT:
        task->destructor = shooting_destroy;
        set_view();
        stream_reset();
        stream_request_and_save_music(MUSIC_SCRAMBLE);
        dat_0cad53fc = -1;
        dat_0cad5400 = 0xff07;
        shooting_path_index = 0;
        shooting_path_running = TRUE;
        njSetBackColor(0x2010, 0x103042, 0x304486);
        shooting_path = act1_path;
        shooting_path_count = act1_path_count;
        dat_0cad53d8 = 3;
        shooting_act = 0;
        shooting_course_part = 0;
        shooting_camera_set_spacing(0);
        camera_set_mode(CAMERA_MODE_BACK2, CAMERA_ADJUST_NONE);
        create_actors();
        task_create(TASK_WORK, 2, shooting_scenery_step);
        task_create(TASK_WORK, 2, shooting_streaks_step);
        task_create(TASK_WORK, 2, shooting_hud_step);
        task_create(TASK_WORK, 2, carrier_step);
        work->mode = SHOOTING_FLY;
        break;
    case SHOOTING_FLY:
        if (shooting_path_index == SWING_POINT) {
            shooting_camera_swing_start(player_work[0], 5.0F, 30.0F, 240);
            camera_set_mode(CAMERA_MODE_BACK, CAMERA_ADJUST_NONE);
            stream_request_voice(VOICE_SWING);
        }
        if (shooting_path_index == SWUNG_POINT) {
            stream_request_voice(VOICE_SWUNG);
        }
        if (shooting_path_running) {
            if (++shooting_path_index >=
                shooting_path_count - ACT1_EVENT_EARLY) {
                task_create(TASK_WORK, 1, shooting_event1_step);
                work->mode = SHOOTING_EVENT1;
                work->timer = EVENT1_STEPS;
                pause_disable();
                stream_request_voice(VOICE_EVENT1);
                break;
            }
        }
        if (shooting_lives <= 0.0F) {
            work->mode = SHOOTING_FALL;
        }
        if (shooting_request) {
            switch (shooting_request) {
            case REQUEST_EVENT1:
                task_create(TASK_WORK, 1, shooting_event1_step);
                work->mode = SHOOTING_EVENT1;
                work->timer = EVENT1_STEPS;
                stream_request_voice(VOICE_EVENT1);
                break;
            case REQUEST_CLEAR:
                work->mode = SHOOTING_CLEAR;
                break;
            }
            shooting_request = 0;
        }
        break;
    case SHOOTING_EVENT1:
        if (--TASK_TIMER(work) <= 0) {
            task_create(TASK_WORK, 1, score_step);
            work->mode = SHOOTING_SCORE;
        }
        pause_disable();
        break;
    case SHOOTING_AFTER:
        task_create(TASK_WORK, 1, score_step);
        work->mode = SHOOTING_SCORE;
        break;
    case SHOOTING_FALL:
        if (shooting_path_running) {
            if (++shooting_path_index >= FALL_END) {
                work->mode = SHOOTING_DOWN;
            }
        }
        break;
    case SHOOTING_CLEAR:
        if (shooting_lives <= 0.0F) {
            work->mode = SHOOTING_FALL;
        }
        break;
    case SHOOTING_SCORE:
        pause_disable();
        break;
    case SHOOTING_END:
        task_destroy(task);
        break;
    /* @hack redundant: case SHOOTING_DOWN, which only breaks as the default
     * does, keeps the image's CMP/EQ #2 at 0x0c900b0a; without it, 235 of 265
     * words differ */
    case SHOOTING_DOWN:
        break;
    /* @hack redundant: a default that only breaks, which leaves
     * shooting_stage_mode's address in R2 and the mode in R3 at 0x0c900e16;
     * without it, the two trade and 3 of 265 words differ */
    default:
        break;
    }
    /* work->mode holds a mode, not a character */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    shooting_stage_mode = work->mode;
}

void shooting_act2_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case SHOOTING_INIT:
        task->destructor = shooting_destroy;
        set_view();
        stream_reset();
        stream_request_and_save_music(MUSIC_SCRAMBLE);
        dat_0cad53fc = -1;
        dat_0cad5400 = 0xff07;
        shooting_path_index = 0;
        shooting_path_running = TRUE;
        njSetBackColor(0, 0, 0);
        shooting_path = act2_path;
        shooting_path_count = act2_path_count;
        dat_0cad53d8 = 4;
        shooting_act = 1;
        shooting_course_part = 1;
        shooting_camera_set_spacing(0);
        camera_set_mode(CAMERA_MODE_BACK2, CAMERA_ADJUST_NONE);
        create_actors();
        task_create(TASK_WORK, 2, shooting_scenery_step);
        task_create(TASK_WORK, 2, shooting_streaks_step);
        task_create(TASK_WORK, 2, shooting_hud_step);
        task_create(TASK_WORK, 2, carrier_step);
        work->mode = SHOOTING_FLY;
        break;
    case SHOOTING_FLY:
        if (shooting_path_index == VOICE_1_POINT) {
            stream_request_voice(VOICE_ACT2_1);
        }
        if (shooting_path_index == VOICE_2_POINT) {
            stream_request_voice(VOICE_ACT2_2);
        }
        if (shooting_path_index == VOICE_3_POINT) {
            stream_request_voice(VOICE_ACT2_3);
        }
        if (shooting_path_index == VOICE_EVENT2_POINT) {
            stream_request_voice(VOICE_EVENT2);
        }
        if (shooting_path_running) {
            if (++shooting_path_index >= ACT2_EVENT_POINT) {
                task_create(TASK_WORK, 1, shooting_event2_step);
                work->mode = SHOOTING_EVENT2;
                work->timer = EVENT2_STEPS;
                break;
            }
        }
        if (shooting_lives <= 0.0F) {
            work->mode = SHOOTING_DOWN;
        }
        if (shooting_request) {
            /* @hack spelling: a switch of one case, which makes SHOOTING_EVENT2
             * in R3 (MOV #6,R3 at 0x0c901024); as an if, in R2, and 9 of 310
             * words differ */
            switch (shooting_request) {
            case REQUEST_EVENT2:
                task_create(TASK_WORK, 1, shooting_event2_step);
                work->mode = SHOOTING_EVENT2;
                work->timer = EVENT2_STEPS;
                stream_request_voice(VOICE_EVENT2);
                break;
            }
            shooting_request = 0;
        }
        break;
    case SHOOTING_FALL:
        if (shooting_path_running) {
            if (++shooting_path_index >= FALL_END) {
                work->mode = SHOOTING_DOWN;
            }
        }
        break;
    /* @hack redundant: case SHOOTING_DOWN, which only breaks, keeps its own BRA
     * to the end (0x0c90107e); without it, 304 of 310 words differ */
    case SHOOTING_DOWN:
        break;
    case SHOOTING_SCORE:
        /* @hack spelling: a goto into SHOOTING_EVENT2's call of pause_disable,
         * which SHC writes as a BRA at 0x0c901082 to the call at 0x0c901096;
         * as a call of its own, 139 of 310 words differ */
        goto paused;
    case SHOOTING_EVENT2:
        if (--TASK_TIMER(work) <= 0) {
            work->mode = SHOOTING_AFTER;
        }
    paused:
        pause_disable();
        break;
    case SHOOTING_AFTER:
        set_view();
        shooting_camera_set_spacing(1);
        shooting_path_index = ACT2_EVENT_POINT;
        shooting_course_part = 2;
        work->mode = SHOOTING_SECOND;
        pause_enable();
        break;
    case SHOOTING_SECOND:
        set_view();
        if (shooting_path_running) {
            if (++shooting_path_index >= act2_path_count) {
                work->mode = SHOOTING_CLEAR;
                shooting_path = act2_end_path;
                shooting_path_count = act2_end_path_count;
                shooting_path_index = CARRIER_POINT;
                shooting_path_running = TRUE;
                carrier_arrived = 1;
                shooting_camera_set_spacing(0);
                break;
            }
        }
        if (shooting_lives <= 0.0F) {
            work->mode = SHOOTING_DOWN;
        }
        if (shooting_request) {
            /* @hack spelling: a switch of one case, as in SHOOTING_FLY, which
             * makes SHOOTING_EVENT2 in R2 (MOV #6,R2 at 0x0c90113e); as an
             * if, in R3, and 14 of 310 words differ */
            switch (shooting_request) {
            case REQUEST_EVENT2:
                task_create(TASK_WORK, 1, shooting_event2_step);
                work->mode = SHOOTING_EVENT2;
                work->timer = EVENT2_STEPS;
                break;
            }
            shooting_request = 0;
        }
        break;
    case SHOOTING_CLEAR:
        camera_set_perspective(DEFAULT_FOV);
        if (carrier_arrived == -1) {
            task_create(TASK_WORK, 1, score_step);
            work->mode = SHOOTING_SCORE;
            camera_start_swing(player_work[0], 5.0F, 50.0F, 240);
            camera_set_mode(CAMERA_MODE_CHAOS_STINIT, CAMERA_ADJUST_NONE);
        }
        break;
    case SHOOTING_END:
        task_destroy(task);
        break;
    }
    /* work->mode holds a mode, not a character */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    shooting_stage_mode = work->mode;
}

static void set_view(void)
{
    camera_set_perspective(SHOOTING_FOV);
    njClipZ(-3.0F, -65535.0F);
}

/* make an actor for each of the course's scenes, and list it in
 * shooting_actors as inactive. a null task ends the list */
static void create_actors(void)
{
    struct shooting_scene *scenes;
    struct task *task;
    struct task_work *work;
    int i;

    shooting_scenes = *shooting_scene_file;
    /* @hack spelling: scenes read through shooting_scene_file again, which
     * loads the file's first word a second time (MOV.L @R4,R11 at
     * 0x0c901288); as scenes = shooting_scenes, 73 of 84 words differ */
    scenes = *shooting_scene_file;
    for (i = 0; scenes[i].path; i++) {
        switch (scenes[i].kind) {
        /* @hack redundant: case SCENE_ZAKO, which does what the default does,
         * keeps the image's CMP/EQ #1 at 0x0c90129c; without it, 64 of 84
         * words differ */
        case SCENE_ZAKO:
            task = task_create(TASK_WORK, 3, scene_zako_step);
            break;
        case SCENE_RENZAKO:
            task = task_create(TASK_WORK, 3, scene_renzako_step);
            break;
        default:
            task = task_create(TASK_WORK, 3, scene_zako_step);
            break;
        }
        work = task->work;
        SCENE_VARS(work)->scene = i;
        work->pos.x = 0.0F;
        work->pos.y = 0.0F;
        work->pos.z = 0.0F;
        shooting_actors[i].task = task;
        shooting_actors[i].active = 0;
    }
    shooting_actors[i].task = NULL;
    shooting_actors[i].active = 0;
}

void shooting_actors_clear(void)
{
    shooting_actor_count = 0;
    shooting_actors[0].task = NULL;
}

void shooting_actors_add(struct task *task)
{
    if (shooting_actor_count < SHOOTING_ACTORS - 1) {
        shooting_actors[shooting_actor_count++].task = task;
        shooting_actors[shooting_actor_count].task = NULL;
    }
}

static void shooting_destroy(struct task *task)
{
}

Bool shooting_event_playing(void)
{
    switch (shooting_stage_mode) {
    case SHOOTING_SCORE:
    case SHOOTING_EVENT1:
    case SHOOTING_EVENT2:
        return TRUE;
    }
    return FALSE;
}
