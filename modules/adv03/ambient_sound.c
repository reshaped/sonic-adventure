/* O SOUND, a tone that plays at the object's position */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv03/ambient_sound.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "sound/sound.h"
#include "system/task.h"

/* the updates sound_play_at_for asks for. each step renews the request, so
 * the tone stops TONE_UPDATES + 1 of sound_send_queue's updates after the
 * last renewal */
#define TONE_UPDATES 30

static int ambient_sound_tones[] = {
    0x438, 0x43a, 0x43c, 0x43e, 0x440, 0x442, 0x444,
};

#define TONES (sizeof(ambient_sound_tones) / sizeof(ambient_sound_tones[0]))

void ambient_sound_step(struct task *task)
{
    struct task_work *work = task->work;
    int tone;
    int volume;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        /* @hack spelling: the marker's scale, 1.0F held in a local, which has
         * SHC load it into FR15 at 0x0c90263e and keep it there across the
         * calls; with 1.0F passed to njScale, 22 of 111 words differ */
        Float scale = 1.0F;

        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x, work->pos.y, work->pos.z);
        draw_set_material(0.02F, 0.2F, 0.2F, 0.5F);
        njScale(NULL, scale, scale, scale);
        njSimpleDrawObject(&marker_object);
        njPopMatrix(1);
    }
    if (placement_editing(task)) {
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "-> TONE[MAX:");
        njPrintH(NJM_LOCATION(29, EDITOR_ROW_ANG_X), TONES, 2);
        njPrintC(NJM_LOCATION(31, EDITOR_ROW_ANG_X), "]");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "-> RADIUS");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "-> VOLUME");
    }
    tone = work->ang.x;
    volume = work->scl.y;
    if (tone < 0) {
        tone = 0;
        /* NOLINTNEXTLINE(clang-diagnostic-sign-compare) */
    } else if (tone >= TONES) {
        tone = TONES - 1;
    }
    sound_play_at_for(ambient_sound_tones[tone], (int)work, 1, volume,
                      work->pos.x, work->pos.y, work->pos.z, TONE_UPDATES);
}
