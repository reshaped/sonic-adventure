#include "input.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

namespace pc {

namespace {

/* Shinobi's PDD_DGT bits, as the game reads a pad */
enum {
    PAD_C = 1 << 0,
    PAD_B = 1 << 1,
    PAD_A = 1 << 2,
    PAD_START = 1 << 3,
    PAD_UP = 1 << 4,
    PAD_DOWN = 1 << 5,
    PAD_LEFT = 1 << 6,
    PAD_RIGHT = 1 << 7,
    PAD_Z = 1 << 8,
    PAD_Y = 1 << 9,
    PAD_X = 1 << 10,
    PAD_TR = 1 << 16,
    PAD_TL = 1 << 17
};

/* what a binding holds: the pad's buttons, its triggers and its sticks */
enum Target {
    A,
    B,
    C,
    X,
    Y,
    Z,
    START,
    UP,
    DOWN,
    LEFT,
    RIGHT,
    L,
    R,
    X1,
    Y1,
    X2,
    Y2,
    TARGETS
};
const char *const TARGET_NAMES[TARGETS] = {
    "a",    "b",     "c", "x", "y",  "z",  "start", "up", "down",
    "left", "right", "l", "r", "x1", "y1", "x2",    "y2"};
const uint32_t TARGET_BITS[L] = {PAD_A,    PAD_B,    PAD_C,     PAD_X,
                                 PAD_Y,    PAD_Z,    PAD_START, PAD_UP,
                                 PAD_DOWN, PAD_LEFT, PAD_RIGHT};

enum Kind { KEY, BUTTON, AXIS };
struct Binding {
    uint8_t target;
    uint8_t kind;
    int16_t code; /* SDL_Scancode, SDL_GamepadButton or SDL_GamepadAxis */
};

const Binding DEFAULTS[] = {{UP, KEY, SDL_SCANCODE_UP},
                            {DOWN, KEY, SDL_SCANCODE_DOWN},
                            {LEFT, KEY, SDL_SCANCODE_LEFT},
                            {RIGHT, KEY, SDL_SCANCODE_RIGHT},
                            {A, KEY, SDL_SCANCODE_Z},
                            {B, KEY, SDL_SCANCODE_X},
                            {X, KEY, SDL_SCANCODE_A},
                            {Y, KEY, SDL_SCANCODE_S},
                            {START, KEY, SDL_SCANCODE_RETURN},
                            {L, KEY, SDL_SCANCODE_Q},
                            {R, KEY, SDL_SCANCODE_W},
                            {A, BUTTON, SDL_GAMEPAD_BUTTON_SOUTH},
                            {B, BUTTON, SDL_GAMEPAD_BUTTON_EAST},
                            {X, BUTTON, SDL_GAMEPAD_BUTTON_WEST},
                            {Y, BUTTON, SDL_GAMEPAD_BUTTON_NORTH},
                            {START, BUTTON, SDL_GAMEPAD_BUTTON_START},
                            {UP, BUTTON, SDL_GAMEPAD_BUTTON_DPAD_UP},
                            {DOWN, BUTTON, SDL_GAMEPAD_BUTTON_DPAD_DOWN},
                            {LEFT, BUTTON, SDL_GAMEPAD_BUTTON_DPAD_LEFT},
                            {RIGHT, BUTTON, SDL_GAMEPAD_BUTTON_DPAD_RIGHT},
                            {L, AXIS, SDL_GAMEPAD_AXIS_LEFT_TRIGGER},
                            {R, AXIS, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER},
                            {X1, AXIS, SDL_GAMEPAD_AXIS_LEFTX},
                            {Y1, AXIS, SDL_GAMEPAD_AXIS_LEFTY},
                            {X2, AXIS, SDL_GAMEPAD_AXIS_RIGHTX},
                            {Y2, AXIS, SDL_GAMEPAD_AXIS_RIGHTY}};

/* the most an SDL axis reads */
constexpr int32_t REACH = 32767;
/* Shinobi's peripheral server presses L or R from 128 of a trigger's 255 */
constexpr uint32_t TRIGGER_PRESSED = 128;

} // namespace

struct Input {
    std::vector<Binding> bindings;
    float deadzone;
    float rumble;
    std::vector<SDL_Gamepad *> pads;
    uint32_t held; /* the last sample's, which press and release differ from */
    /* the shaking's strengths at full, and when it ends. it falls to none
     * over its last fade_ms */
    uint16_t low, high;
    uint64_t end_ns;
    uint32_t fade_ms;
    uint16_t sent_low, sent_high;
};

namespace {

bool parse_number(const char *text, float most, bool below, float &value)
{
    char *end = nullptr;
    double read = strtod(text, &end);

    while (*end == ' ' || *end == '\t') {
        end++;
    }
    if (end == text || *end != '\0' || !(read >= 0) ||
        (below ? read >= most : read > most)) {
        return false;
    }
    value = static_cast<float>(read);
    return true;
}

/* apply one line, its comment and end blanks cut. it returns false, with
 * why, where it fails. replaced marks, keys then pads, the targets whose
 * defaults are dropped */
bool parse_line(Input &input, char *line, bool (&replaced)[TARGETS][2],
                std::string &why)
{
    char *end = line + strcspn(line, " \t");
    char *rest = end + strspn(end, " \t");

    *end = '\0';
    if (!strcmp(line, "deadzone")) {
        if (!parse_number(rest, 1.0F, true, input.deadzone)) {
            why = "deadzone takes 0 to under 1";
            return false;
        }
        return true;
    }
    if (!strcmp(line, "rumble")) {
        if (!parse_number(rest, 1.0F, false, input.rumble)) {
            why = "rumble takes 0 to 1";
            return false;
        }
        return true;
    }
    bool key = !strcmp(line, "key");

    if (!key && strcmp(line, "pad") != 0) {
        why = std::string("no setting named ") + line;
        return false;
    }
    char *source = rest + strcspn(rest, " \t");
    uint32_t target = 0;

    if (*source != '\0') {
        *source++ = '\0';
        source += strspn(source, " \t");
    }
    while (target < TARGETS && strcmp(rest, TARGET_NAMES[target]) != 0) {
        target++;
    }
    if (target == TARGETS) {
        why = std::string("no target named ") + rest;
        return false;
    }
    Binding made = {static_cast<uint8_t>(target), KEY, 0};

    if (*source != '\0' && key) {
        made.code = static_cast<int16_t>(SDL_GetScancodeFromName(source));
        if (made.code == SDL_SCANCODE_UNKNOWN) {
            why = std::string("no key named ") + source;
            return false;
        }
    } else if (*source != '\0') {
        SDL_GamepadButton button = SDL_GetGamepadButtonFromString(source);
        SDL_GamepadAxis axis = SDL_GetGamepadAxisFromString(source);

        if (button == SDL_GAMEPAD_BUTTON_INVALID &&
            axis == SDL_GAMEPAD_AXIS_INVALID) {
            why = std::string("no gamepad input named ") + source;
            return false;
        }
        made.kind = axis != SDL_GAMEPAD_AXIS_INVALID ? AXIS : BUTTON;
        made.code =
            static_cast<int16_t>(made.kind == AXIS ? static_cast<int>(axis)
                                                   : static_cast<int>(button));
    }
    if (target >= X1 && (key || (*source != '\0' && made.kind != AXIS))) {
        why = std::string(TARGET_NAMES[target]) + " takes a gamepad's axis";
        return false;
    }
    if (!replaced[target][!key]) {
        replaced[target][!key] = true;
        for (size_t i = input.bindings.size(); i-- > 0;) {
            const Binding &b = input.bindings[i];

            if (b.target == target && (b.kind == KEY) == key) {
                input.bindings.erase(input.bindings.begin() +
                                     static_cast<std::ptrdiff_t>(i));
            }
        }
    }
    if (*source != '\0') {
        input.bindings.push_back(made);
    }
    return true;
}

bool parse(Input &input, const char *settings, std::string &why)
{
    bool replaced[TARGETS][2] = {};
    std::string text = settings;
    uint32_t number = 0;

    for (size_t at = 0; at < text.size();) {
        size_t end = text.find('\n', at);

        end = end == std::string::npos ? text.size() : end;
        std::string line = text.substr(at, end - at);

        at = end + 1;
        number++;
        line = line.substr(0, line.find('#'));
        size_t first = line.find_first_not_of(" \t\r");

        if (first == std::string::npos) {
            continue;
        }
        line = line.substr(first, line.find_last_not_of(" \t\r") + 1 - first);
        if (!parse_line(input, &line[0], replaced, why)) {
            why = "settings line " + std::to_string(number) + ": " + why;
            return false;
        }
    }
    return true;
}

bool shakes(SDL_Gamepad *pad)
{
    return SDL_GetBooleanProperty(SDL_GetGamepadProperties(pad),
                                  SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false);
}

/* take the dead zone out of x and y, rescaling the rest from its edge */
void past_dead(float deadzone, int32_t &x, int32_t &y)
{
    if (deadzone <= 0) {
        return;
    }
    float reach =
        sqrtf(static_cast<float>(x) * x + static_cast<float>(y) * y) / REACH;

    if (reach <= deadzone) {
        x = y = 0;
        return;
    }
    float scale = (reach - deadzone) / (1.0F - deadzone) / reach;

    x = static_cast<int32_t>(lroundf(x * scale));
    y = static_cast<int32_t>(lroundf(y * scale));
    x = x < -REACH - 1 ? -REACH - 1 : x > REACH ? REACH : x;
    y = y < -REACH - 1 ? -REACH - 1 : y > REACH ? REACH : y;
}

/* what a gamepad's input reads, a button as an axis at its reach */
int32_t reading(SDL_Gamepad *gamepad, const Binding &b)
{
    if (b.kind == AXIS) {
        return SDL_GetGamepadAxis(gamepad,
                                  static_cast<SDL_GamepadAxis>(b.code));
    }
    return SDL_GetGamepadButton(gamepad, static_cast<SDL_GamepadButton>(b.code))
               ? REACH
               : 0;
}

/* send the pads what the shaking is at now_ns, where it changed or begins */
void shake(Input &input, uint64_t now_ns, bool begins)
{
    uint16_t low = 0;
    uint16_t high = 0;
    uint32_t left_ms = 0;

    if (now_ns < input.end_ns) {
        uint64_t left_ns = input.end_ns - now_ns;
        uint64_t fade_ns = static_cast<uint64_t>(input.fade_ms) * 1000000U;
        double part = left_ns < fade_ns ? static_cast<double>(left_ns) /
                                              static_cast<double>(fade_ns)
                                        : 1.0;

        low = static_cast<uint16_t>(input.low * part);
        high = static_cast<uint16_t>(input.high * part);
        left_ms = static_cast<uint32_t>((left_ns + 999999U) / 1000000U);
    }
    if (!begins && low == input.sent_low && high == input.sent_high) {
        return;
    }
    for (SDL_Gamepad *pad : input.pads) {
        if (shakes(pad)) {
            SDL_RumbleGamepad(pad, low, high, left_ms);
        }
    }
    input.sent_low = low;
    input.sent_high = high;
    if (low == 0 && high == 0) {
        input.end_ns = 0;
    }
}

} // namespace

Input *input_open(const char *settings, std::string &why)
{
    auto *input = new Input{};

    input->bindings.assign(DEFAULTS,
                           DEFAULTS + sizeof DEFAULTS / sizeof *DEFAULTS);
    input->rumble = 1.0F;
    if (settings != nullptr && !parse(*input, settings, why)) {
        delete input;
        return nullptr;
    }
    return input;
}

void input_close(Input *input)
{
    if (input == nullptr) {
        return;
    }
    for (SDL_Gamepad *pad : input->pads) {
        SDL_CloseGamepad(pad);
    }
    delete input;
}

void input_event(Input *input, const SDL_Event &event)
{
    if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
        SDL_Gamepad *pad = SDL_OpenGamepad(event.gdevice.which);

        if (pad != nullptr) {
            input->pads.push_back(pad);
        }
    } else if (event.type == SDL_EVENT_GAMEPAD_REMOVED) {
        for (size_t i = 0; i < input->pads.size(); i++) {
            if (SDL_GetGamepadID(input->pads[i]) == event.gdevice.which) {
                SDL_CloseGamepad(input->pads[i]);
                input->pads.erase(input->pads.begin() +
                                  static_cast<std::ptrdiff_t>(i));
                break;
            }
        }
    }
}

void input_update(Input *input, uint64_t now_ns)
{
    if (input->end_ns != 0) {
        shake(*input, now_ns, false);
    }
}

sa_result input_pad(Input *input, uint32_t port, const bool *keys, bool alt,
                    sa_pad &pad)
{
    if (port != 0) {
        return SA_EMPTY;
    }
    uint32_t held = 0;
    uint32_t trigger[2] = {0, 0};
    /* each stick, x then y, from the pad that pushes it furthest */
    int32_t stick[2][2] = {{0, 0}, {0, 0}};
    int64_t furthest[2] = {0, 0};

    pad = sa_pad{};
    pad.head.kind = SA_REC_PAD;
    pad.head.size = sizeof pad;
    pad.present = 1;
    for (const Binding &b : input->bindings) {
        if (b.kind == KEY && keys[b.code] &&
            !(alt && b.code == SDL_SCANCODE_RETURN)) {
            if (b.target < L) {
                held |= TARGET_BITS[b.target];
            } else {
                trigger[b.target - L] = 255;
            }
        }
    }
    for (SDL_Gamepad *gamepad : input->pads) {
        int32_t axes[2][2] = {{0, 0}, {0, 0}};

        for (const Binding &b : input->bindings) {
            if (b.kind == KEY) {
                continue;
            }
            int32_t value = reading(gamepad, b);

            if (b.target < L) {
                held |= value > REACH / 2 ? TARGET_BITS[b.target] : 0;
            } else if (b.target <= R) {
                /* the pad's trigger reads 0 to 255, where SDL's reads 15
                 * bits */
                uint32_t read =
                    value > 0 ? static_cast<uint32_t>(value) / 128U : 0;

                trigger[b.target - L] =
                    read > trigger[b.target - L] ? read : trigger[b.target - L];
            } else {
                int32_t &axis = axes[(b.target - X1) / 2][(b.target - X1) % 2];

                axis = abs(value) > abs(axis) ? value : axis;
            }
        }
        for (int s = 0; s < 2; s++) {
            past_dead(input->deadzone, axes[s][0], axes[s][1]);
            int64_t pushed = static_cast<int64_t>(axes[s][0]) * axes[s][0] +
                             static_cast<int64_t>(axes[s][1]) * axes[s][1];

            if (pushed > furthest[s]) {
                furthest[s] = pushed;
                stick[s][0] = axes[s][0];
                stick[s][1] = axes[s][1];
            }
        }
    }
    /* the pad's stick reads -128 to 127, where SDL's reads 16 bits */
    pad.x1 = static_cast<int16_t>(stick[0][0] / 256);
    pad.y1 = static_cast<int16_t>(stick[0][1] / 256);
    pad.x2 = static_cast<int16_t>(stick[1][0] / 256);
    pad.y2 = static_cast<int16_t>(stick[1][1] / 256);
    if (held & (PAD_LEFT | PAD_RIGHT)) {
        pad.x1 = held & PAD_LEFT ? -128 : 127;
    }
    if (held & (PAD_UP | PAD_DOWN)) {
        pad.y1 = held & PAD_UP ? -128 : 127;
    }
    pad.l = static_cast<uint16_t>(trigger[0]);
    pad.r = static_cast<uint16_t>(trigger[1]);
    held |= trigger[0] >= TRIGGER_PRESSED ? PAD_TL : 0;
    held |= trigger[1] >= TRIGGER_PRESSED ? PAD_TR : 0;
    pad.on = held;
    pad.press = held & ~input->held;
    pad.release = input->held & ~held;
    input->held = held;
    return SA_OK;
}

sa_result input_rumble(Input *input, uint32_t port, uint16_t low, uint16_t high,
                       uint32_t ms, uint32_t fade_ms, uint64_t now_ns)
{
    bool any = false;

    for (SDL_Gamepad *pad : input->pads) {
        any |= shakes(pad);
    }
    if (port != 0 || !any || input->rumble <= 0) {
        return SA_EMPTY;
    }
    if (ms == 0) {
        return SA_OK;
    }
    input->low = static_cast<uint16_t>(low * input->rumble);
    input->high = static_cast<uint16_t>(high * input->rumble);
    input->end_ns = now_ns + static_cast<uint64_t>(ms) * 1000000U;
    input->fade_ms = fade_ms < ms ? fade_ms : ms;
    shake(*input, now_ns, true);
    return SA_OK;
}

} // namespace pc
