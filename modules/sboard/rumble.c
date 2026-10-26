/* the pad's rumble for what moves near the camera on the sand hill */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "sboard/rumble.h"
#include "system/task.h"
#include "vmu/vibration.h"

void rumble_near_camera(const NJS_POINT3 *pos, Float range)
{
    Float distance;

    if (pos != NULL && camera_work != NULL) {
        distance = njDistanceP2P((NJS_POINT3 *)pos, &camera_work->pos);
        if (distance <= range) {
            vibration_start(0, 5, 15, 0);
        }
    }
}
