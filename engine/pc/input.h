/* the Dreamcast's pad in port 0, from the keyboard and every SDL gamepad as
 * the settings bind them. the game's shaking goes to the gamepads that can
 * shake */
#ifndef ENGINE_PC_INPUT_H
#define ENGINE_PC_INPUT_H

#include <stdint.h>

#include <string>

#include <SDL3/SDL.h>

#include "sa_engine.h"

namespace pc {

struct Input;

/* bindings from settings, lines of text, or null for the defaults:
 *
 *   key TARGET NAME   the key SDL names NAME holds TARGET
 *   pad TARGET NAME   the gamepad's button or axis SDL names NAME does
 *   deadzone F        a stick reads none within F of its reach, from 0 to
 *                     under 1. past F, its push rescales from none. 0 is
 *                     the default, as the game's own dead zone takes a
 *                     stick at rest
 *   rumble F          a pad shakes at F of its strength, from 0 to 1. at 0
 *                     input_rumble answers SA_EMPTY
 *
 * TARGET is the pad's a, b, c, x, y, z, start, up, down, left, right, l or
 * r, or a gamepad stick's axis, x1, y1, x2 or y2. an axis bound to a button
 * holds it past half its reach. a key or button bound to a trigger holds it
 * full. a held direction also pushes the left stick full its way, as play
 * steers by the stick. a target's first key line drops its default keys,
 * and its first pad line drops its default gamepad inputs. a line with no
 * NAME only drops them. # begins a comment. it returns null, with why, at
 * the first line it can't take */
Input *input_open(const char *settings, std::string &why);
void input_close(Input *input);
/* open the gamepad SDL's event adds, or close the one it removes */
void input_event(Input *input, const SDL_Event &event);
/* fade the shaking as now_ns, SDL's ticks, passes */
void input_update(Input *input, uint64_t now_ns);
/* sample the pad in port from keys, SDL's keyboard state, and the
 * gamepads. it returns SA_OK, or SA_EMPTY for any port but 0. the return key
 * held with alt is the fullscreen key's, so it presses nothing */
sa_result input_pad(Input *input, uint32_t port, const bool *keys, bool alt,
                    sa_pad &pad);
/* do what sa_input_rumble asks, at now_ns */
sa_result input_rumble(Input *input, uint32_t port, uint16_t low, uint16_t high,
                       uint32_t ms, uint32_t fade_ms, uint64_t now_ns);

} // namespace pc

#endif
