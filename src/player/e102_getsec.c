/* the seconds E-102 gets, as a count rising in the world, facing the camera */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "player/e102_bonus.h"
#include "player/e102_getsec.h"
#include "player/lockon.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

enum getsec_mode { GETSEC_START, GETSEC_RISE };

struct getsec_vars {
    Float age; /* the share of its life gone, from 0 to 1 */
    Uint8 unk_04[4];
    Sint16 seconds;
};

#define VARS(work) ((struct getsec_vars *)&(work)->vars)

static NJS_ARGB getsec_material = {1.0F, 1.0F, 1.0F, 1.0F};

/* the count rises 0.3 a step for 120 steps, facing the camera. it draws
 * its backdrop, then its digits from the last leftward */
static void e102_getsec_step(struct task *task)
{
    struct task_work *work = task->work;
    int digits;
    int seconds;

    switch (work->mode) {
    case GETSEC_START:
        VARS(work)->age = 0.0F;
        work->mode = GETSEC_RISE;
        break;
    case GETSEC_RISE:
        work->pos.y += 0.3F;
        VARS(work)->age += 1.0F / 120.0F;
        if (VARS(work)->age >= 1.0F) {
            task_destroy(task);
            return;
        }
        break;
    }
    if (frame_step != 0) {
        return;
    }
    seconds = VARS(work)->seconds;
    if (seconds > 99) {
        digits = 3;
    } else if (seconds > 9) {
        digits = 2;
    } else {
        digits = 1;
    }
    njSetConstantMaterial(&getsec_material);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njSetTexture(&e102time_texlist);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, camera_work->ang.y);
    TURN(njRotateX, camera_work->ang.x + NJM_DEG_ANG(180));
    njTranslate(NULL, 0.0F, 0.0F, -10.0F);
    njDrawSprite3D(&lockon_sprite, E102TIME_GETSECBG,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    while (digits > 0) {
        njTranslate(NULL, -8.0F, 0.0F, 0.0F);
        njDrawSprite3D(&lockon_sprite, seconds % 10,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        digits--;
        seconds /= 10;
    }
    njPopMatrix(1);
}

void e102_getsec_create(struct task_work *from, int seconds)
{
    struct task *task;

    task = task_create(TASK_WORK, 2, e102_getsec_step);
    if (task != NULL) {
        task->work->pos = from->pos;
        VARS(task->work)->seconds = seconds;
    }
}
