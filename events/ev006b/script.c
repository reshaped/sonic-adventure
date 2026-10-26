/* EV006B, Amy's story: a placeholder for the crash movie. its only text,
 * TEXT_1, says the movie isn't made yet */
#include "ev006b/script.h"
#include "event/play.h"
#include "system/texture.h"

#define TEXT_1_LENGTH 41
#include "ev006b/script/text.inc"

struct texture_pvm ev006b_textures[] = {{NULL, NULL}};

void ev006b_script(int phase)
{
    switch (phase) {
    case EVENT_PLAY:
        event_message_wait(90, TEXT_1);
        break;
    }
}
