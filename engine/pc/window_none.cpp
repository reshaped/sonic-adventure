/* a build with no SDL3: headless alone */
#include "window.h"

namespace pc {

Window *window_open(const char *, uint32_t, uint32_t, uint32_t, const char *,
                    const Image *, std::string &why)
{
    why = "this build has no SDL3, so no window";
    return nullptr;
}

void window_close(Window *)
{
}

void window_alert(const char *, const char *)
{
}

const char *const *window_extensions(uint32_t &count)
{
    count = 0;
    return nullptr;
}

bool window_surface(Window *, void *, uint64_t &)
{
    return false;
}

void window_size(Window *, uint32_t &width, uint32_t &height)
{
    width = 0;
    height = 0;
}

bool window_pump(Window *)
{
    return false;
}

sa_result window_pad(Window *, uint32_t, sa_pad &)
{
    return SA_EMPTY;
}

sa_result window_rumble(Window *, uint32_t, uint16_t, uint16_t, uint32_t,
                        uint32_t)
{
    return SA_EMPTY;
}

bool window_sound_open(Window *, SoundRender, void *, std::string &why)
{
    why = "this build has no SDL3, so no sound device";
    return false;
}

void window_sound_close(Window *)
{
}

} // namespace pc
