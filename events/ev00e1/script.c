/* EV00E1: a placeholder for Big's crash movie, whose one message says it
 * isn't made yet */
#include "ev00e1/script.h"
#include "event/play.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 41
#include "ev00e1/script/text.inc"

struct texture_pvm ev00e1_textures[] = {{NULL, NULL}};

void ev00e1_script(int phase)
{
    switch (phase) {
    case EVENT_PLAY:
        event_message_wait(90, TEXT_1);
        break;
    }
}
