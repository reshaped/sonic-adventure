/* the menus' drawing: the screens' fading tints, and names, numbers, times
 * and marks from the menus' PVMs */
#include <ninja.h>
#include <sg_xpt.h>

#include "advertise/draw.h"
#include "advertise/textures.h"
#include "draw/screen.h"
#include "hud/menu.h"
#include "save/save.h"

#define FRAMES_PER_SECOND 60

#define NEW16NO_STEP 18.0F /* the dots between ava_new16no's digits' places */

/* ava_new16no's colon, and ava_san's shadow of its arrow */
#define NEW16NO_COLON 2
#define SAN_SHADOW 1

static const Uint8 b_chnam_textures_by_story[] = {0, 1, 2, 3, 4, 5, 6};
static const Uint8 m_chnam_textures_by_story[] = {0, 1, 2, 3, 4, 5, 6};
static const Uint8 tx_chnam_textures_by_story[] = {0, 1, 2, 3, 4, 5, 6};
static const Uint8 fscmn_textures_by_row[] = {FSCMN_NUMBER_1, FSCMN_NUMBER_2,
                                              FSCMN_NUMBER_3, FSCMN_BACK_ARROW};
static const Uint8 suuji_digits[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
static const Uint8 new16no_digits[] = {0, 1, 4, 5, 6, 7, 8, 9, 10, 11};

Uint32 color_fade_in(Float fade)
{
    return menu_blend_color(0x00ffffff, 0xffffffff, fade);
}

Uint32 color_fade_out(Float fade)
{
    if (fade < 0.75F) {
        /* @hack redundant: fade * 0.0F, which SHC multiplies out (FLDI0 FR0
         * and FMUL FR0,FR3 at 0x8c900474) and adds to the alpha by its FMAC;
         * without it, the function runs 4 bytes short, item_draw moves, and
         * 25 of 28 words differ */
        return ((Uint32)((fade * 0.0F) + ((0.75F - fade) * 255.0F)) << 24) |
               0x00ffffff;
    }
    return 0;
}

void item_draw(Uint32 color, Float x, Float y, Float z)
{
    menu_draw_row_item(color, x, y, z, 1.0F, 1.0F);
}

void act_panel_draw(Float x, Float y, Float z, enum panel_window window)
{
    njSetTexture(&ava_trialactsel_texlist);
    screen_quad_colors(0xffff8800, 0xffffff00, 0xffff8800, 0xffffff00);
    screen_quad_draw_scaled(ACTSEL_BAR_Y, x, y, z - 0.3F, 1.281F, 1.281F);
    screen_quad_color(0xffffffff);
    screen_quad_draw_scaled(ACTSEL_BAR_Y_SMALL, x, y, z, 1.312F, 1.312F);
    screen_quad_draw_scaled(ACTSEL_BAR_P, x + 5.0F, y + 24.0F, z - 0.9F, 1.375F,
                            1.375F);
    switch (window) {
    case PANEL_RECORD:
        screen_quad_draw_scaled(ACTSEL_BAR_WINDOW_1, x + 169.0F, y + 7.0F,
                                z - 0.6F, 1.203F, 1.203F);
        break;
    case PANEL_EMBLEMS_2:
        screen_quad_draw_scaled(ACTSEL_BAR_WINDOW_2, x + 169.0F, y + 7.0F,
                                z - 0.6F, 1.203F, 1.203F);
        break;
    default:
        screen_quad_draw_scaled(ACTSEL_BAR_WINDOW_ABC, x + 169.0F, y + 7.0F,
                                z - 0.6F, 1.203F, 1.203F);
        break;
    }
}

void b_chnam_draw(int story, Uint32 color, Float x, Float y, Float z)
{
    njSetTexture(&b_chnam_texlist);
    screen_quad_color(color);
    screen_quad_draw(b_chnam_textures_by_story[story], x, y, z);
}

void m_chnam_draw(int story, Uint32 color, Float x, Float y, Float z,
                  Float scale_x, Float scale_y)
{
    njSetTexture(&m_chnam_texlist);
    screen_quad_color(color);
    screen_quad_draw_scaled(m_chnam_textures_by_story[story] | QUAD_CENTERED, x,
                            y, z, scale_x, scale_y);
}

void tx_chnam_draw(int story, Uint32 color, Float x, Float y, Float z,
                   Float scale_x, Float scale_y)
{
    njSetTexture(&tx_chnam_texlist);
    screen_quad_color(color);
    screen_quad_draw_scaled(tx_chnam_textures_by_story[story] | QUAD_CENTERED,
                            x, y, z, scale_x, scale_y);
}

void fscmn_draw(Uint8 row, Float x, Float y, Float z)
{
    njSetTexture(&ava_fscmn_texlist);
    screen_quad_color(0xffffffff);
    screen_quad_draw(fscmn_textures_by_row[row] | QUAD_CENTERED, x, y, z);
}

/* the number of digits in number, 1 for 0 */
static int digit_count(int number)
{
    int digits = 0;

    for (;;) {
        number /= 10;
        ++digits;
        if (number == 0) {
            break;
        }
    }
    return digits;
}

void suuji_draw(int number, int digits, Uint8 zeros, Float x, Float y, Float z,
                Float spacing)
{
    int left;
    int digit;
    Uint32 width;
    Uint32 height;
    Uint8 texture;

    if (digits) {
        left = digit_count(number);
        njSetTexture(&ava_suuji_texlist);
        screen_quad_color(0xffffffff);
        screen_texture_size(&ava_suuji_texlist, 1, &width, &height);
        /* @hack type: the last place's offset cast to int, which SHC converts
         * as signed (FLOAT at 0x8c900802). from the Uint32, SHC adds a test of
         * its sign and a correction, the function runs 12 bytes longer, the
         * functions after it move, and 63 of 99 words differ */
        x += (Float)(int)((digits - 1) * width) * spacing;
        for (; digits; --digits) {
            digit = number % 10;
            texture = suuji_digits[digit];
            if (zeros == 1 || left > 0) {
                screen_quad_draw(texture | QUAD_CENTERED, x, y, z);
                /* @hack type: width cast to int, which SHC converts as
                 * signed (FLOAT at 0x8c900838). from the Uint32, SHC adds a
                 * test of its sign and a correction, the function runs 12
                 * bytes longer, the functions after it move, and 35 of 99
                 * words differ */
                x -= (Float)(int)width * spacing;
            }
            number /= 10;
            --left;
        }
    }
}

void suuji_draw_all(int number, Float x, Float y, Float z)
{
    suuji_draw(number, digit_count(number), 1, x, y, z, 1.0F);
}

void new16no_draw(int number, int digits, Uint8 zeros, Float x, Float y,
                  Float z, Float scale_x, Float scale_y)
{
    int left;
    int digit;
    Uint8 texture;

    if (digits) {
        left = digit_count(number);
        njSetTexture(&ava_new16no_texlist);
        screen_quad_color(0xffffffff);
        x += (digits - 1) * NEW16NO_STEP;
        for (; digits; --digits) {
            digit = number % 10;
            texture = new16no_digits[digit];
            if (zeros == 1 || left > 0) {
                screen_quad_draw_scaled(texture | QUAD_CENTERED, x, y, z,
                                        scale_x * 1.1F, scale_y * 1.1F);
                x -= NEW16NO_STEP;
            }
            number /= 10;
            --left;
        }
    }
}

void new16no_draw_all(int number, Float x, Float y, Float z, Float scale_x,
                      Float scale_y)
{
    new16no_draw(number, digit_count(number), 1, x, y, z, scale_x, scale_y);
}

void ava_emblem_draw(Float x, Float y, Float z)
{
    njSetTexture(&ava_emblem_texlist);
    screen_quad_color(0xffffffff);
    screen_quad_draw(QUAD_CENTERED, x, y, z);
}

void scroll_arrow_draw(Bool up, Float x, Float y, Float z)
{
    int texture;

    if (!up) {
        texture = QUAD_TOP;
    } else {
        texture = QUAD_FLIP_V | QUAD_BOTTOM;
    }
    njSetTexture(&ava_san_texlist);
    screen_quad_color(0xffffffff);
    screen_quad_draw(texture, x, y, z - 0.3F);
    screen_quad_draw(texture | SAN_SHADOW, x, y, z);
}

void time_draw(struct save_time *time, Float x, Float y, Float z)
{
    Uint32 hundredths;

    new16no_draw(time->minutes, 2, 1, x, y, z, 1.0F, 1.0F);
    new16no_draw(time->seconds, 2, 1, x + 50.0F, y, z, 1.0F, 1.0F);
    hundredths = time->frames * (100.0F * (1.0F / FRAMES_PER_SECOND));
    new16no_draw(hundredths, 2, 1, x + 100.0F, y, z, 1.0F, 1.0F);
    njSetTexture(&ava_new16no_texlist);
    screen_quad_color(0xffffffff);
    screen_quad_draw(QUAD_CENTERED | NEW16NO_COLON, x + 34.0F, y, z);
    screen_quad_draw(QUAD_CENTERED | NEW16NO_COLON, x + 84.0F, y, z);
}

void time_draw_hundredths(struct save_time *time, Float x, Float y, Float z)
{
    new16no_draw(time->minutes, 2, 1, x, y, z, 1.0F, 1.0F);
    new16no_draw(time->seconds, 2, 1, x + 50.0F, y, z, 1.0F, 1.0F);
    new16no_draw(time->frames, 2, 1, x + 100.0F, y, z, 1.0F, 1.0F);
    njSetTexture(&ava_new16no_texlist);
    screen_quad_color(0xffffffff);
    screen_quad_draw(QUAD_CENTERED | NEW16NO_COLON, x + 34.0F, y, z);
    screen_quad_draw(QUAD_CENTERED | NEW16NO_COLON, x + 84.0F, y, z);
}
