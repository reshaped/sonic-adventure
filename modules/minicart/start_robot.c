/* START ROBOT, a robot that waves its flag as it slides aside during the
 * countdown */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/draw.h"
#include "minicart/race.h"
#include "minicart/start_robot.h"
#include "object/placement.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* a flag shows its textures by turns, each for FLAG_STEPS steps */
#define FLAG_TEXTURES 2
#define FLAG_STEPS 30

/* the textures of the robot's flag, by their names in OBJ_MINI_CART */
#define MIN_HATA5 30
#define MIN_HATA6 31

/* the robot's action plays WAVE_SPEED motion frames a step, and the robot
 * slides SLIDE_SPEED a step */
#define WAVE_SPEED 0.5F
#define SLIDE_SPEED 0.75F

/* a flag's material, and the textures it shows */
struct flag_textures {
    NJS_MATERIAL *material;
    Uint16 textures[FLAG_TEXTURES];
};

/* the robot's record, in task->motion */
struct start_robot_record {
    Sint8 mode;
    Sint8 shown; /* the index in textures of the one each flag shows */
    Sint16 wait; /* the steps until the flags' next texture */
    Sint16 flag_count;
    Float frame; /* the action's motion frame */
    Float frames; /* the action's frame count less 1, where the wave loops */
    Float slide; /* how far along its x the robot has slid */
    NJS_ACTION *action;
    const struct flag_textures *flags;
};

enum start_robot_mode { START_ROBOT_WAIT, START_ROBOT_SLIDE, START_ROBOT_GONE };

/* the robot's model's materials, the flag's at FLAG_MATERIAL */
#define FLAG_MATERIAL 7
extern NJS_MATERIAL robot_materials[];
extern NJS_ACTION start_robot_action;

static void start_robot_init(struct task *task, void *arg);
static void start_robot_run(struct task *task);
static void start_robot_draw(struct task *task);
static void start_robot_show_flags(struct start_robot_record *record);
static void start_robot_wave_flags(struct start_robot_record *record);

static const struct task_desc start_robot_desc = {
    sizeof(struct start_robot_record),
    TASK_WORK,
    start_robot_init,
    start_robot_run,
    start_robot_draw,
    NULL};
static const struct flag_textures start_robot_flags[1] = {
    {&robot_materials[FLAG_MATERIAL], {MIN_HATA5, MIN_HATA6}}};

void start_robot_step(struct task *task)
{
    task_apply_desc(task, &start_robot_desc, NULL);
}

static void start_robot_init(struct task *task, void *arg)
{
    struct start_robot_record *record =
        (struct start_robot_record *)task->motion;
    NJS_ACTION *action = &start_robot_action;

    record->mode = START_ROBOT_WAIT;
    record->shown = 0;
    record->wait = FLAG_STEPS;
    record->flag_count = 1;
    record->frame = 0.0F;
    record->frames = action->motion->nbFrame - 1;
    record->slide = 0.0F;
    record->action = action;
    record->flags = start_robot_flags;
    start_robot_show_flags(record);
}

/* each step, wave the flags and advance the action. while the set editor
 * holds the robot, it stands slid by scl.x. otherwise it waits for the
 * countdown, then slides along its x toward scl.x. it destroys itself the
 * step after it arrives. outside the set editor, a robot still waiting past
 * the countdown destroys itself too */
static void start_robot_run(struct task *task)
{
    struct start_robot_record *record;
    struct task_work *work;
    Float slide;

    if (placement_try_leave(task)) {
        return;
    }
    work = task->work;
    slide = work->scl.x;
    record = (struct start_robot_record *)task->motion;
    start_robot_wave_flags(record);
    record->frame += WAVE_SPEED;
    if (record->frame >= record->frames) {
        record->frame -= record->frames;
    }
    if (placement_editing(task)) {
        record->slide = slide;
    } else {
        switch (record->mode) {
        case START_ROBOT_WAIT: {
            Sint8 mode = cart_race_mode();

            switch (mode) {
            case RACE_NONE:
            case RACE_READY:
                break;
            case RACE_COUNTDOWN:
                record->mode = START_ROBOT_SLIDE;
                break;
            default:
                if (!edit_mode) {
                    task_kill(task);
                }
                break;
            }
            break;
        }
        case START_ROBOT_SLIDE:
            if (slide >= 0.0F) {
                record->slide += SLIDE_SPEED;
                if (record->slide >= slide) {
                    record->slide = slide;
                    record->mode = START_ROBOT_GONE;
                }
            } else {
                /* @hack spelling: -SLIDE_SPEED added, an FADD of -0.75 from
                 * a pool word of its own (0x0c90c300); subtracted, the other
                 * way's word serves both, start_robot_run is 4 bytes shorter
                 * and 45 of 119 words differ */
                record->slide += -SLIDE_SPEED;
                if (record->slide <= slide) {
                    record->slide = slide;
                    record->mode = START_ROBOT_GONE;
                }
            }
            break;
        case START_ROBOT_GONE:
            task_kill(task);
            break;
        }
    }
    start_robot_draw(task);
}

/* on a frame's first step, draw the robot slid along its x. it draws only
 * while it slides, or in the set editor */
static void start_robot_draw(struct task *task)
{
    struct start_robot_record *record =
        (struct start_robot_record *)task->motion;
    struct task_work *work;
    struct angles *ang;
    Bool visible = FALSE;

    if (frame_step == 0) {
        if (edit_mode) {
            visible = TRUE;
        } else {
            switch (record->mode) {
            case START_ROBOT_SLIDE:
                visible = TRUE;
                break;
            case START_ROBOT_WAIT:
            case START_ROBOT_GONE:
                break;
            }
        }
    }
    if (visible) {
        work = task->work;
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        ang = &work->ang;
        TURN(njRotateY, ang->y);
        njTranslate(NULL, record->slide, 0.0F, 0.0F);
        texture_set_stage();
        draw_action(record->action, record->frame);
        njPopMatrix(1);
    }
}

/* set each flag's material to the texture shown */
static void start_robot_show_flags(struct start_robot_record *record)
{
    int count = record->flag_count;
    const struct flag_textures *flag;
    int i;

    for (i = 0, flag = record->flags; i < count; i++, flag++) {
        flag->material->attr_texId = flag->textures[record->shown];
    }
}

/* show the flags' next texture once FLAG_STEPS steps have passed */
static void start_robot_wave_flags(struct start_robot_record *record)
{
    record->wait--;
    if (record->wait <= 0) {
        record->wait = FLAG_STEPS;
        record->shown++;
        if (record->shown >= FLAG_TEXTURES) {
            record->shown = 0;
        }
        start_robot_show_flags(record);
    }
}
