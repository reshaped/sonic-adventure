/* EV0053, Tails' story: in place of the crash movie, a message that the
 * movie isn't made yet */
#include "ev0053/script.h"
#include "event/play.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 41
#include "ev0053/script/text.inc"

struct texture_pvm ev0053_textures[] = {{NULL, NULL}};

void ev0053_script(int phase)
{
    switch (phase) {
    case EVENT_PLAY:
        event_message_wait(90, TEXT_1);
        break;
    }
}
