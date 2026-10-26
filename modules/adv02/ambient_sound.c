/* the ambient sound the object list names O SE: one of eight tones, played
 * at the object's position */
#include <ninja.h>
#include <sg_xpt.h>

#include "adv02/ambient_sound.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "object/placement.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "system/task.h"

/* the updates sound_play_at_for asks for. each step renews the request, so
 * the tone stops TONE_UPDATES + 1 of sound_send_queue's updates after the
 * last renewal */
#define TONE_UPDATES 30

/* the step means NIGHT_TONE to take DAY_TONE's place at night */
#define DAY_TONE 0x43a
#define NIGHT_TONE 0x43c

static int mystic_ruins_ambient_sound_tones[] = {
    0x438, DAY_TONE, 0x43e, 0x440, 0x442, 0x444, 0x24b, 0x24c,
};

#define TONES                                                                  \
    (sizeof(mystic_ruins_ambient_sound_tones) /                                \
     sizeof(mystic_ruins_ambient_sound_tones[0]))

/* ang.x picks the tone, clamped to the table, and scl.y gives its volume.
 * scl.x, which the set editor labels RADIUS, goes unread. the step plays
 * nothing while battle_rival holds a rival */
void mystic_ruins_ambient_sound_step(struct task *task)
{
    struct task_work *work = task->work;
    int volume;
    int tone;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        /* @hack spelling: the scale held in a local set ahead of njPushMatrix,
         * so SHC loads it into FR15 before the call (FLDI1 FR15 at
         * 0x0c911b24); as 1.0F in njScale's arguments, 22 of 130 words
         * differ */
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
    if (battle_rival >= 0) {
        return;
    }
    volume = work->scl.y;
    tone = work->ang.x;
    if (tone < 0) {
        tone = 0;
        /* NOLINTNEXTLINE(clang-diagnostic-sign-compare) */
    } else if (tone >= TONES) {
        tone = TONES - 1;
    }
    /* @bug logic: tone holds an index from 0 to 7 by now, but the test
     * compares it with a sound, DAY_TONE (CMP/EQ R3,R13 at 0x0c911c4a). it
     * never passes, and NIGHT_TONE never plays */
    if (tone == DAY_TONE && story_get_time() == TIME_NIGHT) {
        tone = NIGHT_TONE;
    }
    sound_play_at_for(mystic_ruins_ambient_sound_tones[tone], (int)work, 1,
                      volume, work->pos.x, work->pos.y, work->pos.z,
                      TONE_UPDATES);
}
