/* the SDL3 window. it gives the raster a surface, and the backend its pads
 * and sound. window_none.cpp stands in where a host has no SDL3 */
#ifndef ENGINE_PC_WINDOW_H
#define ENGINE_PC_WINDOW_H

#include <stdint.h>

#include <string>

#include "sa_engine.h"

namespace pc {

struct Window;
struct Image;

/* open a window of width by height, fullscreen as SA_PC_FULLSCREEN and
 * SA_PC_EXCLUSIVE in flags say. settings binds its pads (input.h). icon is
 * its icon unless null. it gives null, with why, where SDL or input_open
 * fails */
Window *window_open(const char *title, uint32_t width, uint32_t height,
                    uint32_t flags, const char *settings, const Image *icon,
                    std::string &why);
void window_close(Window *window);
/* sa_pc_alert's */
void window_alert(const char *title, const char *words);
/* the instance extensions a surface needs */
const char *const *window_extensions(uint32_t &count);
/* make surface, a VkSurfaceKHR, on instance. it gives false where SDL
 * makes none */
bool window_surface(Window *window, void *instance, uint64_t &surface);
/* the window's size in the display's pixels */
void window_size(Window *window, uint32_t &width, uint32_t &height);
/* take the platform's events and the fullscreen key. it gives true where
 * the platform asks to end */
bool window_pump(Window *window);
/* the pad in port, by Shinobi's bits: SA_OK, or SA_EMPTY */
sa_result window_pad(Window *window, uint32_t port, sa_pad &pad);
/* sa_input_rumble's */
sa_result window_rumble(Window *window, uint32_t port, uint16_t low,
                        uint16_t high, uint32_t ms, uint32_t fade_ms);
/* open the default device at SA_PC_SOUND_RATE, two int16_t a frame. render
 * runs on the device's thread as it asks. it gives false, with why, where
 * none opens. window_sound_close ends those calls before it returns */
typedef void (*SoundRender)(void *user, int16_t *out, uint32_t frames);
bool window_sound_open(Window *window, SoundRender render, void *user,
                       std::string &why);
void window_sound_close(Window *window);

} // namespace pc

#endif
