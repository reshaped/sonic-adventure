#include "window.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif

#include <algorithm>

#include "input.h"
#include "pvr.h"
#include "sa_pc.h"

namespace pc {

struct Window {
    SDL_Window *window;
    Input *input;
    SDL_AudioStream *sound;
    SoundRender render;
    void *user;
    bool exclusive; /* fullscreen at the display's own mode */
};

namespace {

/* fullscreen is a borderless window across the desktop. where exclusive, it
 * is the listed display mode nearest the desktop's, if SDL lists one */
void fullscreen(Window *window, bool on)
{
    SDL_DisplayMode mode{};
    const SDL_DisplayMode *desktop =
        SDL_GetDesktopDisplayMode(SDL_GetDisplayForWindow(window->window));
    bool exclusive = on && window->exclusive && desktop != nullptr &&
                     SDL_GetClosestFullscreenDisplayMode(
                         desktop->displayID, desktop->w, desktop->h,
                         desktop->refresh_rate, true, &mode);

    if (on) {
        SDL_SetWindowFullscreenMode(window->window,
                                    exclusive ? &mode : nullptr);
    }
    SDL_SetWindowFullscreen(window->window, on);
}

} // namespace

Window *window_open(const char *title, uint32_t width, uint32_t height,
                    uint32_t flags, const char *settings, const Image *icon,
                    std::string &why)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        why = SDL_GetError();
        return nullptr;
    }
    Input *input = input_open(settings, why);

    if (input == nullptr) {
        SDL_Quit();
        return nullptr;
    }
    /* the window draws every display pixel, even where the system scales */
    SDL_Window *made = SDL_CreateWindow(
        title, static_cast<int>(width), static_cast<int>(height),
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE |
            SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (made == nullptr) {
        why = SDL_GetError();
        input_close(input);
        SDL_Quit();
        return nullptr;
    }
    if (icon != nullptr) {
        SDL_Surface *picture = SDL_CreateSurfaceFrom(
            static_cast<int>(icon->width), static_cast<int>(icon->height),
            SDL_PIXELFORMAT_RGBA32, const_cast<uint8_t *>(icon->rgba.data()),
            static_cast<int>(icon->width * 4));

        if (picture != nullptr) {
            SDL_SetWindowIcon(made, picture);
            SDL_DestroySurface(picture);
        }
    }
    auto *window = new Window{made,    input,   nullptr,
                              nullptr, nullptr, (flags & SA_PC_EXCLUSIVE) != 0};
    if (flags & SA_PC_FULLSCREEN) {
        fullscreen(window, true);
    }
    return window;
}

void window_close(Window *window)
{
    if (window == nullptr) {
        return;
    }
    window_sound_close(window);
    input_close(window->input);
    SDL_DestroyWindow(window->window);
    SDL_Quit();
    delete window;
}

void window_alert(const char *title, const char *words)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, words, nullptr);
}

const char *const *window_extensions(uint32_t &count)
{
    Uint32 made = 0;
    const char *const *names = SDL_Vulkan_GetInstanceExtensions(&made);

    count = made;
    return names;
}

bool window_surface(Window *window, void *instance, uint64_t &surface)
{
    VkSurfaceKHR made = 0;

    if (!SDL_Vulkan_CreateSurface(window->window,
                                  static_cast<VkInstance>(instance), nullptr,
                                  &made)) {
        return false;
    }
    surface = reinterpret_cast<uint64_t>(made);
    return true;
}

void window_size(Window *window, uint32_t &width, uint32_t &height)
{
    int w = 0;
    int h = 0;

    SDL_GetWindowSizeInPixels(window->window, &w, &h);
    width = static_cast<uint32_t>(w);
    height = static_cast<uint32_t>(h);
}

bool window_pump(Window *window)
{
    SDL_Event event;
    bool quit = false;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT ||
            event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
            quit = true;
        } else if (event.type == SDL_EVENT_GAMEPAD_ADDED ||
                   event.type == SDL_EVENT_GAMEPAD_REMOVED) {
            input_event(window->input, event);
        } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat &&
                   (event.key.key == SDLK_F11 ||
                    (event.key.key == SDLK_RETURN &&
                     (event.key.mod & SDL_KMOD_ALT) != 0))) {
            fullscreen(window, (SDL_GetWindowFlags(window->window) &
                                SDL_WINDOW_FULLSCREEN) == 0);
        }
    }
    input_update(window->input, SDL_GetTicksNS());
    return quit;
}

sa_result window_pad(Window *window, uint32_t port, sa_pad &pad)
{
    if (window == nullptr) {
        return SA_EMPTY;
    }
    return input_pad(window->input, port, SDL_GetKeyboardState(nullptr),
                     (SDL_GetModState() & SDL_KMOD_ALT) != 0, pad);
}

sa_result window_rumble(Window *window, uint32_t port, uint16_t low,
                        uint16_t high, uint32_t ms, uint32_t fade_ms)
{
    if (window == nullptr) {
        return SA_EMPTY;
    }
    return input_rumble(window->input, port, low, high, ms, fade_ms,
                        SDL_GetTicksNS());
}

namespace {

/* the frames a piece of sound holds, and a frame's bytes: two int16_t */
constexpr int PIECE = 1024;
constexpr int FRAME_BYTES = 2 * sizeof(int16_t);
#if defined(__SSE__)
/* the host's own rounding: every exception masked, to nearest, no flush */
constexpr unsigned int DEFAULT_MXCSR = 0x1f80U;
#endif

/* render what the device's thread asks for, a piece at a time. render under
 * the host's own rounding, whatever mode that thread holds */
void SDLCALL feed(void *user, SDL_AudioStream *stream, int wanted, int)
{
    auto *window = static_cast<Window *>(user);
    int16_t frames[2 * PIECE];
#if defined(__SSE__)
    unsigned int mode = _mm_getcsr();

    _mm_setcsr(DEFAULT_MXCSR);
#endif
    while (wanted > 0) {
        /* a part of a frame asked for renders whole. bytes holds to a piece
         * first, so no sum passes INT_MAX */
        int bytes = std::min(wanted, PIECE * FRAME_BYTES);
        int count = (bytes + FRAME_BYTES - 1) / FRAME_BYTES;

        window->render(window->user, frames, static_cast<uint32_t>(count));
        SDL_PutAudioStreamData(stream, frames, count * FRAME_BYTES);
        wanted -= count * FRAME_BYTES;
    }
#if defined(__SSE__)
    _mm_setcsr(mode);
#endif
}

} // namespace

bool window_sound_open(Window *window, SoundRender render, void *user,
                       std::string &why)
{
    const SDL_AudioSpec spec = {SDL_AUDIO_S16, 2, SA_PC_SOUND_RATE};

    if (window == nullptr || !SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        why = window == nullptr ? "no window" : SDL_GetError();
        return false;
    }
    window->render = render;
    window->user = user;
    window->sound = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                              &spec, feed, window);
    if (window->sound == nullptr) {
        why = SDL_GetError();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }
    SDL_ResumeAudioStreamDevice(window->sound);
    return true;
}

void window_sound_close(Window *window)
{
    if (window == nullptr || window->sound == nullptr) {
        return;
    }
    SDL_DestroyAudioStream(window->sound);
    window->sound = nullptr;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

} // namespace pc
