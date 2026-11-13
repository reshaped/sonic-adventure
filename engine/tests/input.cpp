/* the PC's pads over SDL's virtual gamepads, which need no device or
 * display. the test checks the default bindings, a settings file's bindings
 * and dead zone, and the triggers' reach. it checks gamepads that come and
 * go, and the game's vibration through engine/native/vibration.c to SDL's
 * rumble. the input sees only the test's own gamepads' events, so no host
 * pad is read or shaken. main returns 0, the number of the first check that
 * fails, or 99 where SDL fails to start */
#include <stdio.h>
#include <string.h>

#include <string>

#include "input.h"

extern "C" {
extern int (*native_rumble)(uint32_t strength, uint32_t ms, uint32_t fade_ms);
int32_t pdVibMxIsReady(uint32_t port);
int32_t pdVibMxSetStopTime(uint32_t port, uint32_t time);
int32_t pdVibMxStart(uint32_t port, const void *param);
}

namespace {

/* Shinobi's bits */
constexpr uint32_t PAD_B = 1 << 1, PAD_A = 1 << 2, PAD_START = 1 << 3,
                   PAD_UP = 1 << 4, PAD_DOWN = 1 << 5, PAD_LEFT = 1 << 6,
                   PAD_RIGHT = 1 << 7, PAD_Y = 1 << 9, PAD_X = 1 << 10,
                   PAD_TR = 1 << 16, PAD_TL = 1 << 17;
constexpr uint32_t PDD_PORT_A1 = 1, PDD_PORT_A2 = 2;
/* PDS_VIBPARAM's flags */
constexpr uint8_t CONTINUOUS = 1 << 0, CONVERGENCE = 1 << 7;
/* power 4's strength, of 7 */
constexpr uint16_t POWER_4 = 4 * 65535 / 7;

struct Shaken {
    uint16_t low, high;
    int calls;
};

bool SDLCALL rumbled(void *user, Uint16 low, Uint16 high)
{
    auto *shaken = static_cast<Shaken *>(user);

    shaken->low = low;
    shaken->high = high;
    shaken->calls++;
    return true;
}

struct Pad {
    SDL_JoystickID id;
    SDL_Joystick *joystick; /* the test's, to set its buttons and axes */
    SDL_Gamepad *gamepad; /* the test's, to read what SDL maps */
};

/* a gamepad of every button and axis, which shakes where shaken is given */
Pad attach(Shaken *shaken, Uint16 product)
{
    SDL_VirtualJoystickDesc desc;

    SDL_INIT_INTERFACE(&desc);
    desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    desc.vendor_id = 0x5341;
    desc.product_id = product;
    desc.naxes = SDL_GAMEPAD_AXIS_COUNT;
    desc.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    desc.name = "input test";
    desc.userdata = shaken;
    desc.Rumble = shaken != nullptr ? rumbled : nullptr;
    Pad pad = {SDL_AttachVirtualJoystick(&desc), nullptr, nullptr};

    pad.joystick = SDL_OpenJoystick(pad.id);
    pad.gamepad = SDL_OpenGamepad(pad.id);
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFT_TRIGGER,
                               SDL_JOYSTICK_AXIS_MIN);
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,
                               SDL_JOYSTICK_AXIS_MIN);
    return pad;
}

void detach(Pad &pad)
{
    SDL_CloseGamepad(pad.gamepad);
    SDL_CloseJoystick(pad.joystick);
    SDL_DetachVirtualJoystick(pad.id);
}

pc::Input *input;
SDL_JoystickID mine[3];
bool keys[SDL_SCANCODE_COUNT];
uint64_t now_ns;

/* SDL's adds and removals of the test's gamepads alone, to the input */
void pump()
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if ((event.type == SDL_EVENT_GAMEPAD_ADDED ||
             event.type == SDL_EVENT_GAMEPAD_REMOVED) &&
            (event.gdevice.which == mine[0] || event.gdevice.which == mine[1] ||
             event.gdevice.which == mine[2])) {
            pc::input_event(input, event);
        }
    }
}

sa_pad sample(bool alt = false)
{
    sa_pad pad{};

    pump();
    pc::input_pad(input, 0, keys, alt, pad);
    return pad;
}

int rumble(uint32_t strength, uint32_t ms, uint32_t fade_ms)
{
    return pc::input_rumble(input, 0, static_cast<uint16_t>(strength),
                            static_cast<uint16_t>(strength), ms, fade_ms,
                            now_ns) == SA_OK;
}

/* what vibration_start asks of Shinobi for pad 0, given arguments it keeps
 * as they are. param holds a PDS_VIBPARAM, as engine/native/vibration.c
 * lays it out */
void vibration_start(int power, int freq, int time)
{
    uint8_t param[8] = {1, 0, static_cast<uint8_t>(power),
                        static_cast<uint8_t>(freq)};

    if (!pdVibMxIsReady(PDD_PORT_A1)) {
        return;
    }
    if (time != 0) {
        param[1] = CONTINUOUS;
        pdVibMxSetStopTime(PDD_PORT_A1, static_cast<uint32_t>(time));
    }
    pdVibMxStart(PDD_PORT_A1, param);
}

/* what vibration_start_fading asks of Shinobi for pad 0, likewise */
void vibration_start_fading(int power, int freq, int length)
{
    int span = freq * length / (power * 4);
    uint8_t param[8] = {1, CONVERGENCE, static_cast<uint8_t>(power),
                        static_cast<uint8_t>(freq),
                        static_cast<uint8_t>(255 / (span <= 0 ? 1 : span))};

    if (pdVibMxIsReady(PDD_PORT_A1)) {
        pdVibMxStart(PDD_PORT_A1, param);
    }
}

bool refused(const char *settings, const char *said)
{
    std::string why;
    pc::Input *made = pc::input_open(settings, why);

    pc::input_close(made);
    return made == nullptr && why.find(said) != std::string::npos;
}

int check()
{
    std::string why;

    /* what no settings can hold */
    if (!refused("key a Nonesuch", "settings line 1: no key named") ||
        !refused("\n# a comment\npad a nonesuch", "line 3: no gamepad input") ||
        !refused("pad x1 a", "x1 takes a gamepad's axis") ||
        !refused("key x1 Z", "x1 takes") || !refused("key jump Z", "target") ||
        !refused("deadzone 1", "deadzone") || !refused("rumble 2", "rumble") ||
        !refused("turbo 1", "no setting named turbo")) {
        return 1;
    }

    /* the default keys. the arrows drive the stick too, and return presses
     * no start under alt */
    input = pc::input_open(nullptr, why);
    const struct {
        SDL_Scancode key;
        uint32_t bit;
    } table[] = {
        {SDL_SCANCODE_UP, PAD_UP},       {SDL_SCANCODE_DOWN, PAD_DOWN},
        {SDL_SCANCODE_LEFT, PAD_LEFT},   {SDL_SCANCODE_RIGHT, PAD_RIGHT},
        {SDL_SCANCODE_Z, PAD_A},         {SDL_SCANCODE_X, PAD_B},
        {SDL_SCANCODE_A, PAD_X},         {SDL_SCANCODE_S, PAD_Y},
        {SDL_SCANCODE_RETURN, PAD_START}};
    for (const auto &t : table) {
        keys[t.key] = true;
        sa_pad pad = sample();

        keys[t.key] = false;
        int x = t.bit == PAD_LEFT ? -128 : t.bit == PAD_RIGHT ? 127 : 0;
        int y = t.bit == PAD_UP ? -128 : t.bit == PAD_DOWN ? 127 : 0;

        if (pad.on != t.bit || pad.press != t.bit || pad.x1 != x ||
            pad.y1 != y || pad.l != 0 || pad.r != 0 || !pad.present ||
            pad.head.kind != SA_REC_PAD || pad.head.size != sizeof pad) {
            return 2;
        }
        if (sample().release != t.bit) {
            return 3;
        }
    }
    keys[SDL_SCANCODE_RETURN] = true;
    sa_pad under_alt = sample(true);

    keys[SDL_SCANCODE_RETURN] = false;
    keys[SDL_SCANCODE_Q] = keys[SDL_SCANCODE_W] = true;
    sa_pad triggers = sample();

    keys[SDL_SCANCODE_Q] = keys[SDL_SCANCODE_W] = false;
    if (under_alt.on != 0 || triggers.l != 255 || triggers.r != 255 ||
        triggers.on != (PAD_TL | PAD_TR)) {
        return 4;
    }

    /* A1 holds no pack without a pad that shakes, nor where native_rumble is
     * null. a run with no live pad leaves it null. with both, A1 alone holds
     * one */
    native_rumble = rumble;
    if (pdVibMxIsReady(PDD_PORT_A1)) {
        return 5;
    }
    static Shaken shaken[2];
    Pad pad = attach(&shaken[0], 1);

    mine[0] = pad.id;
    sample();
    native_rumble = nullptr;
    if (pdVibMxIsReady(PDD_PORT_A1)) {
        return 6;
    }
    native_rumble = rumble;
    if (!pdVibMxIsReady(PDD_PORT_A1) || pdVibMxIsReady(PDD_PORT_A2)) {
        return 7;
    }

    /* the pad's buttons, its d-pad driving the stick and the stick's reach.
     * its triggers read as SDL's, and press TL and TR from 128 */
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_SOUTH, true);
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_EAST, true);
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_DPAD_UP,
                                 true);
    sa_pad held = sample();

    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_SOUTH, false);
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_EAST, false);
    SDL_SetJoystickVirtualButton(pad.joystick, SDL_GAMEPAD_BUTTON_DPAD_UP,
                                 false);
    if (held.on != (PAD_A | PAD_B | PAD_UP) || held.y1 != -128) {
        return 8;
    }
    const Sint16 reaches[] = {32767, -32768, 1000, -1000};

    for (Sint16 reach : reaches) {
        SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTX, reach);
        SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_RIGHTY,
                                   reach);
        sa_pad pushed = sample();

        if (pushed.x1 != reach / 256 || pushed.y2 != reach / 256 ||
            pushed.y1 != 0 || pushed.on != 0) {
            return 9;
        }
    }
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFTX, 0);
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_RIGHTY, 0);
    const Sint16 pulls[] = {SDL_JOYSTICK_AXIS_MIN, 0, 1000,
                            SDL_JOYSTICK_AXIS_MAX};

    for (Sint16 pull : pulls) {
        SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFT_TRIGGER,
                                   pull);
        SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,
                                   pull);
        sa_pad pulled = sample();
        int read =
            SDL_GetGamepadAxis(pad.gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) /
            128;
        uint32_t on = read >= 128 ? PAD_TL | PAD_TR : 0;

        if (pulled.l != read || pulled.r != read || pulled.on != on ||
            (pull == SDL_JOYSTICK_AXIS_MIN && read != 0) ||
            (pull == SDL_JOYSTICK_AXIS_MAX && read != 255)) {
            return 10;
        }
    }
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_LEFT_TRIGGER,
                               SDL_JOYSTICK_AXIS_MIN);
    SDL_SetJoystickVirtualAxis(pad.joystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,
                               SDL_JOYSTICK_AXIS_MIN);
    sample();

    /* power 4 runs to a stop time of 10, 2.75 s, and power 0 stops it. a
     * fade of power 4 at freq 50 over length 10 takes inc 8. the pack falls
     * a power each 8 cycles of 39 ms, so the fade lasts 1,248 ms */
    now_ns = 1000000000;
    vibration_start(4, 30, 10);
    if (shaken[0].low != POWER_4 || shaken[0].high != POWER_4 ||
        shaken[0].calls != 1) {
        return 11;
    }
    vibration_start(0, 7, 0);
    if (shaken[0].low != 0 || shaken[0].high != 0 || shaken[0].calls != 2) {
        return 12;
    }
    vibration_start_fading(4, 50, 10);
    pc::input_update(input, now_ns + 624000000);
    uint16_t half = shaken[0].low;

    pc::input_update(input, now_ns + 1248000000);
    if (half != POWER_4 / 2 || shaken[0].low != 0 || shaken[0].calls != 5) {
        return 13;
    }

    /* A1 holds no pack with the pad gone, nor with one that doesn't shake. a
     * pad that shakes beside it brings one */
    detach(pad);
    sample();
    if (pdVibMxIsReady(PDD_PORT_A1)) {
        return 14;
    }
    Pad still = attach(nullptr, 2);

    mine[1] = still.id;
    SDL_SetJoystickVirtualButton(still.joystick, SDL_GAMEPAD_BUTTON_SOUTH,
                                 true);
    if (sample().on != PAD_A || pdVibMxIsReady(PDD_PORT_A1)) {
        return 15;
    }
    Pad again = attach(&shaken[1], 3);

    mine[2] = again.id;
    SDL_SetJoystickVirtualButton(again.joystick, SDL_GAMEPAD_BUTTON_NORTH,
                                 true);
    if (sample().on != (PAD_A | PAD_Y) || !pdVibMxIsReady(PDD_PORT_A1)) {
        return 16;
    }
    vibration_start(4, 30, 10);
    if (shaken[1].low != POWER_4) {
        return 17;
    }
    pc::input_close(input);

    /* settings bind space and the east button to a, and a shoulder to l.
     * each drops its target's defaults of that kind, but east keeps its
     * default b. the dead zone is a quarter, and the rumble half */
    input = pc::input_open("key a Space\npad a b # east\npad l leftshoulder\n"
                           "deadzone 0.25\nrumble 0.5\n",
                           why);
    if (input == nullptr) {
        fprintf(stderr, "input: %s\n", why.c_str());
        return 18;
    }
    SDL_SetJoystickVirtualButton(still.joystick, SDL_GAMEPAD_BUTTON_SOUTH,
                                 false);
    SDL_SetJoystickVirtualButton(again.joystick, SDL_GAMEPAD_BUTTON_NORTH,
                                 false);
    SDL_Event event{};

    event.type = SDL_EVENT_GAMEPAD_ADDED;
    event.gdevice.which = again.id;
    pc::input_event(input, event);
    keys[SDL_SCANCODE_SPACE] = true;
    sa_pad spaced = sample();

    keys[SDL_SCANCODE_SPACE] = false;
    keys[SDL_SCANCODE_Z] = true;
    sa_pad zed = sample();

    keys[SDL_SCANCODE_Z] = false;
    if (spaced.on != PAD_A || zed.on != 0) {
        return 19;
    }
    SDL_SetJoystickVirtualButton(again.joystick, SDL_GAMEPAD_BUTTON_SOUTH,
                                 true);
    SDL_SetJoystickVirtualAxis(again.joystick, SDL_GAMEPAD_AXIS_LEFT_TRIGGER,
                               SDL_JOYSTICK_AXIS_MAX);
    sa_pad south = sample();

    SDL_SetJoystickVirtualButton(again.joystick, SDL_GAMEPAD_BUTTON_SOUTH,
                                 false);
    SDL_SetJoystickVirtualButton(again.joystick, SDL_GAMEPAD_BUTTON_EAST, true);
    SDL_SetJoystickVirtualButton(again.joystick,
                                 SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, true);
    sa_pad east = sample();

    SDL_SetJoystickVirtualButton(again.joystick, SDL_GAMEPAD_BUTTON_EAST,
                                 false);
    SDL_SetJoystickVirtualButton(again.joystick,
                                 SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, false);
    if (south.on != 0 || south.l != 0 || east.on != (PAD_A | PAD_B | PAD_TL) ||
        east.l != 255) {
        return 20;
    }
    const struct {
        Sint16 reach;
        int16_t x1;
    } dead[] = {{8000, 0}, {16384, 42}, {32767, 127}, {-32768, -128}};
    for (const auto &d : dead) {
        SDL_SetJoystickVirtualAxis(again.joystick, SDL_GAMEPAD_AXIS_LEFTX,
                                   d.reach);
        if (sample().x1 != d.x1) {
            return 21;
        }
    }
    vibration_start(4, 30, 10);
    if (shaken[1].low != POWER_4 / 2) {
        return 22;
    }
    pc::input_close(input);
    input = pc::input_open("rumble 0", why);
    if (input == nullptr) {
        fprintf(stderr, "input: %s\n", why.c_str());
        return 23;
    }
    pc::input_event(input, event);
    if (pdVibMxIsReady(PDD_PORT_A1)) {
        return 24;
    }
    pc::input_close(input);
    detach(still);
    detach(again);
    return 0;
}

} // namespace

int main()
{
    /* SDL's HIDAPI drivers set up the host's pads, so the test turns them
     * off */
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0");
    if (!SDL_Init(SDL_INIT_GAMEPAD)) {
        fprintf(stderr, "input: %s\n", SDL_GetError());
        return 99;
    }
    int failed = check();

    SDL_Quit();
    if (failed) {
        fprintf(stderr, "input: check %d fails\n", failed);
    }
    return failed;
}
