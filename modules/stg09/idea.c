/* Idea, which shows the second table's idea pieces in place. it shows the
 * next piece too, from when the cards deal it until player 0 carries it */
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "stg09/casino.h"
#include "stg09/idea.h"
#include "stg09/pinball.h"
#include "system/task.h"

extern NJS_OBJECT idea_object_0;
extern NJS_OBJECT idea_object_1;
extern NJS_OBJECT idea_object_2;
extern NJS_OBJECT idea_object_3;
extern NJS_OBJECT idea_object_4;
extern NJS_MOTION idea_motion;

/* idea_motion's last frame, where it starts over, and the frames it moves on
 * each step */
#define LAST_FRAME 15.0F
#define FRAME_RATE 0.1F

/* the pieces' models, in the order they're placed */
static NJS_OBJECT *idea_objects[PINBALL_IDEAS] = {
    &idea_object_0, &idea_object_1, &idea_object_2,
    &idea_object_3, &idea_object_4,
};
/* where each piece draws once it's in place */
static NJS_POINT3 idea_places[PINBALL_IDEAS] = {
    {-24.0F, 287.0F, -2934.0F},  {-40.0F, 287.0F, -2878.0F},
    {-100.0F, 287.0F, -2878.0F}, {-117.0F, 287.0F, -2934.0F},
    {-71.0F, 287.0F, -2970.0F},
};

struct idea_vars {
    /* idea_motion's frame, in the task's first var, which first holds the
     * object kind's param */
    Float frame;
};

#define VARS(work) ((struct idea_vars *)&(work)->vars)

static void idea_destroy(struct task *task)
{
}

static void idea_draw(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    if (pinball_idea_draws >= PINBALL_IDEA_DRAWS) {
        pinball_idea_draws = PINBALL_IDEA_DRAWS;
#ifdef GCC
        /* repairs the read the tag names: GCC draws no next piece once all
         * PINBALL_IDEAS are in place */
        if (!(casino_flags & CASINO_IDEA_CARRIED) &&
            pinball_ideas < PINBALL_IDEAS) {
#else
        if (!(casino_flags & CASINO_IDEA_CARRIED)) {
#endif
            njPushMatrix(NULL);
            njTranslateV(NULL, &work->pos);
            /* @bug memory: with every piece in place, this reads the
             * word past idea_objects, idea_places[0]'s x, as the next
             * piece's model, at 0x0c9075f8 */
            draw_motion(idea_objects[pinball_ideas], &idea_motion,
                        VARS(work)->frame);
            njPopMatrix(1);
        }
    }
    for (i = 0; i < pinball_ideas; i++) {
        njPushMatrix(NULL);
        njTranslate(NULL, idea_places[i].x, idea_places[i].y, idea_places[i].z);
        draw_motion(idea_objects[i], &idea_motion, VARS(work)->frame);
        njPopMatrix(1);
    }
}

static void idea_run(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->frame += FRAME_RATE;
    if (VARS(work)->frame >= LAST_FRAME) {
        VARS(work)->frame = 0.0F;
    }
    idea_draw(task);
}

void idea_step(struct task *task)
{
    struct task_work *work = task->work;

    VARS(work)->frame = 0.0F;
    casino_flags &= ~CASINO_IDEA_CARRIED;
    pinball_ideas = 1;
    task->step = idea_run;
    task->draw = idea_draw;
    task->destructor = idea_destroy;
}
