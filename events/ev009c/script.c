/* EV009C, Knuckles' story: a placeholder for the crash movie, whose one message
 * says it isn't made yet */
#include "ev009c/script.h"
#include "event/play.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 41
#include "ev009c/script/text.inc"

struct texture_pvm ev009c_textures[] = {{NULL, NULL}};

void ev009c_script(int phase)
{
    switch (phase) {
    case EVENT_PLAY:
        event_message_wait(90, TEXT_1);
        break;
    }
}
