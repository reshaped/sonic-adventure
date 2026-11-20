/* SA1's text on the Dreamcast. sa_present_live here sends one character of
 * Ninja's print, which it reads during the call alone. SA1's layer for it is
 * njCharacter, in sdk/ninja/njDebug.c. the prints open the font's strip and
 * call njCharacter for each character */
#ifndef SA1_TEXT_H
#define SA1_TEXT_H

#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "sa1/strip.h"
#include "sa_dc.h"
#include "sa_sa1.h"

/* the characters in each row of the font, which has as many rows */
#define SA1_FONT_COLUMNS 16
/* a character's side in texture coordinates */
#define SA1_FONT_CELL (1.0F / SA1_FONT_COLUMNS)

/* an sa_draw_live's state for SA_SA1_CHARACTER. x and y are the character's
 * top left corner, and size is its side, in pixels */
struct sa1_live_character {
    Sint16 x, y;
    Float size;
    Uint32 color;
};

/* send character code as state says from sq on, and return the address past
 * its last vertex. code is unsigned, so that codes 128 to 255 take rows 8 to
 * 15, the font's bottom half */
ALWAYS_INLINE Uint32 *
sa1_character(Uint32 *sq, const struct sa1_live_character *state, Uint8 code)
{
    Sint16 column = code % SA1_FONT_COLUMNS;
    Sint16 row = code / SA1_FONT_COLUMNS;
    Float left = column * SA1_FONT_CELL;
    Float top = row * SA1_FONT_CELL;
    Float right = left + SA1_FONT_CELL;
    Float bottom = top + SA1_FONT_CELL;
    Float x = state->x;
    Float y = state->y;

    sq = sa1_send_vertex(sq, KM_VERTEXPARAM_NORMAL, x, y, 1.0F, TRUE, left, top,
                         state->color);
    sq = sa1_send_vertex(sq, KM_VERTEXPARAM_NORMAL, x, y + state->size, 1.0F,
                         TRUE, left, bottom, state->color);
    sq = sa1_send_vertex(sq, KM_VERTEXPARAM_NORMAL, x + state->size, y, 1.0F,
                         TRUE, right, top, state->color);
    return sa1_send_vertex(sq, KM_VERTEXPARAM_ENDOFSTRIP, x + state->size,
                           y + state->size, 1.0F, TRUE, right, bottom,
                           state->color);
}

ALWAYS_INLINE sa_result sa_present_live(sa_engine *engine,
                                        const sa_draw_live *draw)
{
    const struct sa1_live_character *state = draw->state;

    (void)engine;
    if (draw->format != SA_SA1_CHARACTER) {
        return SA_ERR_UNSUPPORTED;
    }
    sa1_live_sq =
        sa1_character(sa1_live_sq, state, *(const char *)draw->geometry);
    return SA_OK;
}

#endif
