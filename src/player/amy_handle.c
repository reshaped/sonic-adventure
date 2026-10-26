/* the handle an object calls Amy to, and how she works it */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "player/amy.h"
#include "player/amy_handle.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* the length past which a push of player 0's stick counts. amy_read_stick
 * scales each axis by 256, so a full push is 32768, and this is about 0.46
 * of it */
#define STICK_PUSHED 15000.0F

static struct amy_handle amy_handle;

/* player 0's work, or null when player 0 has no task or the game's
 * character isn't Amy */
static struct task_work *amy_get_work(void)
{
    struct task_work *work = player_work[0];

    if (work == NULL || get_character() != CHARACTER_AMY) {
        return NULL;
    }
    return work;
}

Bool amy_read_stick(Angle *ang)
{
    Float x = pad_records[0].x1 << 8;
    Float y = -(Float)(pad_records[0].y1 << 8);

    if ((x * x) + (y * y) > STICK_PUSHED * STICK_PUSHED) {
        *ang = (Angle)(atan2f(x, y) * ANGLE_PER_RADIAN);
        return TRUE;
    }
    return FALSE;
}

struct amy_handle *amy_offer_handle(NJS_POINT3 *pos, Angle ang)
{
    struct amy_handle *handle;
    Angle stick;

    if (amy_get_work() == NULL) {
        return NULL;
    }
    handle = &amy_handle;
    if (pos != NULL) {
        handle->pos = *pos;
    }
    handle->ang = ang;
    handle->held = FALSE;
    handle->pushed = FALSE;
    if (amy_get_action(0) == AMY_HOLDING_HANDLE) {
        handle->held = TRUE;
        if (amy_read_stick(&stick)) {
            handle->pushed = TRUE;
            handle->stick_ang = stick;
        }
    }
    if (!amy_call_to_handle()) {
        handle->held = FALSE;
        handle->pushed = FALSE;
        return NULL;
    }
    return handle;
}

struct amy_handle *amy_get_handle(void)
{
    return &amy_handle;
}
