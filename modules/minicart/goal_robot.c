/* GOAL ROBOT: two robots by the goal that wave their flags on the last
 * lap's home stretch */
#include <ninja.h>
#include <sg_xpt.h>

#include "debug/editor.h"
#include "draw/draw.h"
#include "minicart/goal_robot.h"
#include "minicart/race.h"
#include "object/placement.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/task_desc.h"
#include "system/texture.h"
#include "system/util.h"

/* a flag shows its textures by turns, each for FLAG_STEPS steps */
#define FLAG_TEXTURES 2
#define FLAG_STEPS 30

/* the textures of the robots' flag, by their names in OBJ_MINI_CART */
#define MIN_HATA7 32
#define MIN_HATA8 33

/* the motion frames the wave plays a step */
#define WAVE_SPEED 0.25F

/* half a turn, in angle units */
#define HALF_TURN 0x8000

/* a flag's material, and the textures it shows */
struct flag_textures {
    NJS_MATERIAL *material;
    Uint16 textures[FLAG_TEXTURES];
};

/* the pair's record, in task->motion */
struct goal_robot_record {
    Sint8 mode;
    Sint8 shown; /* the index in textures of the one each flag shows */
    Sint16 wait; /* the steps until the flags' next texture */
    Float frame; /* the action's motion frame */
    Float frames; /* the action's frame count less 1, where the wave loops */
    NJS_ACTION *action;
    const struct flag_textures *flags;
    Sint16 flag_count;
};

enum goal_robot_mode { GOAL_ROBOT_WAVE };

/* the robot's model's materials, the flag's at FLAG_MATERIAL */
#define FLAG_MATERIAL 7
extern NJS_MATERIAL robot_materials[];
extern NJS_ACTION goal_robot_action;

static void goal_robot_init(struct task *task, void *arg);
static void goal_robot_run(struct task *task);
static void goal_robot_draw(struct task *task);
static void goal_robot_show_flags(struct goal_robot_record *record);
static void goal_robot_wave_flags(struct goal_robot_record *record);

static const struct task_desc goal_robot_desc = {
    sizeof(struct goal_robot_record),
    TASK_WORK,
    goal_robot_init,
    goal_robot_run,
    goal_robot_draw,
    NULL};
static const struct flag_textures goal_robot_flags[1] = {
    {&robot_materials[FLAG_MATERIAL], {MIN_HATA7, MIN_HATA8}}};

void goal_robot_step(struct task *task)
{
    task_apply_desc(task, &goal_robot_desc, NULL);
}

static void goal_robot_init(struct task *task, void *arg)
{
    struct goal_robot_record *record = (struct goal_robot_record *)task->motion;
    NJS_ACTION *action = &goal_robot_action;

    record->mode = GOAL_ROBOT_WAVE;
    record->shown = 0;
    record->wait = FLAG_STEPS;
    record->frame = 0.0F;
    record->frames = action->motion->nbFrame - 1;
    record->action = action;
    record->flags = goal_robot_flags;
    record->flag_count = 1;
}

/* while the pair shows, wave its flags, play its action on and draw it. it
 * shows on the last lap's home stretch, or while the set editor is open */
static void goal_robot_run(struct task *task)
{
    struct goal_robot_record *record;
    Bool visible;

    if (placement_try_leave(task)) {
        return;
    }
    visible = FALSE;
    if (edit_mode) {
        visible = TRUE;
    } else {
        Sint8 dial = cart_race_dial();

        if (dial == DIAL_HOME_STRETCH) {
            visible = TRUE;
        }
    }
    if (visible) {
        record = (struct goal_robot_record *)task->motion;
        goal_robot_wave_flags(record);
        switch (record->mode) {
        case GOAL_ROBOT_WAVE:
            record->frame += WAVE_SPEED;
            if (record->frame >= record->frames) {
                record->frame -= record->frames;
            }
            break;
        }
        goal_robot_draw(task);
    }
}

/* on a frame's first step, draw the pair while it shows. the robots stand
 * scl.z either way along the pair's z. the second faces the other way, and
 * plays the wave backward */
static void goal_robot_draw(struct task *task)
{
    struct goal_robot_record *record;
    struct task_work *work;
    Float frame;
    Float back_frame;
    Float apart;
    struct angles *ang;
    Bool visible = FALSE;

    if (frame_step == 0) {
        if (edit_mode) {
            visible = TRUE;
        } else {
            Sint8 dial = cart_race_dial();

            if (dial == DIAL_HOME_STRETCH) {
                visible = TRUE;
            }
        }
    }
    if (visible) {
        record = (struct goal_robot_record *)task->motion;
        work = task->work;
        frame = record->frame;
        back_frame = frame <= 0.0F ? 0.0F : record->frames - frame;
        apart = work->scl.z;
        goal_robot_show_flags(record);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        ang = &work->ang;
        TURN(njRotateY, ang->y);
        texture_set_stage();
        njPushMatrix(NULL);
        njTranslate(NULL, 0.0F, 0.0F, apart);
        draw_action(record->action, frame);
        njPopMatrix(1);
        njTranslate(NULL, 0.0F, 0.0F, -apart);
        njRotateY(NULL, HALF_TURN);
        draw_action(record->action, back_frame);
        njPopMatrix(1);
    }
}

/* set each flag's material to the texture shown */
static void goal_robot_show_flags(struct goal_robot_record *record)
{
    int count = record->flag_count;
    const struct flag_textures *flag;
    int i;

    for (i = 0, flag = record->flags; i < count; i++, flag++) {
        flag->material->attr_texId = flag->textures[record->shown];
    }
}

/* move shown on to the flags' next texture once FLAG_STEPS steps have
 * passed. the draw sets it on each flag's material */
static void goal_robot_wave_flags(struct goal_robot_record *record)
{
    record->wait--;
    if (record->wait <= 0) {
        record->wait = FLAG_STEPS;
        record->shown++;
        if (record->shown >= FLAG_TEXTURES) {
            record->shown = 0;
        }
    }
}
