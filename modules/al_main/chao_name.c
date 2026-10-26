/* a Chao's name, in letters above its head */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "al_main/chao_name.h"
#include "al_main/models.h"
#include "chao/chao.h"
#include "system/task.h"

/* the font's characters, from a space up, sixteen to a row of the
 * texture, each sixteen texels square */
#define FONT_FIRST 32
#define FONT_LAST 191
#define FONT_COLUMNS 16
#define FONT_SIZE 16

/* the alpha the name loses each frame, the alpha it gains while
 * CHAO_SHOW_NAME is set, and the most frames it gains for */
#define FADE_OUT 30
#define FADE_IN 70
#define FADE_FRAMES 150

/* the name is drawn once its alpha is past this */
#define NAME_SHOWN 10

#define NAME_ATTR (NJD_USE_TEXTURE | NJD_TRANSPARENT | NJD_FILL)

static void letter_draw(Uint8 letter, NJS_COLOR *color);
static void window_draw(NJS_COLOR *color);

/* draw letter as a unit square in the current matrix, in color, or white
 * for a null color. a letter outside the font draws as a space */
static void letter_draw(Uint8 letter, NJS_COLOR *color)
{
    NJS_POINT3COL polygon;
    NJS_POINT3 letter_points[4] = {
#include "al_main/chao_name/letter_points.inc"
    };
    NJS_COLOR colors[4];
    NJS_COLOR uvs[4];
    int tex_u;
    int tex_v;
    Uint8 glyph;

    polygon.p = letter_points;
    polygon.col = colors;
    polygon.tex = uvs;
    polygon.num = 4;
    if (color != NULL) {
        polygon.col[0].argb = color->argb;
        polygon.col[1].argb = color->argb;
        polygon.col[2].argb = color->argb;
        polygon.col[3].argb = color->argb;
    } else {
        polygon.col[0].color = 0xffffffff;
        polygon.col[1].color = 0xffffffff;
        polygon.col[2].color = 0xffffffff;
        polygon.col[3].color = 0xffffffff;
    }
    if (letter >= FONT_FIRST && letter <= FONT_LAST) {
        glyph = letter - FONT_FIRST;
    } else {
        glyph = 0;
    }
    tex_u = (glyph % FONT_COLUMNS) * FONT_SIZE;
    tex_v = (glyph / FONT_COLUMNS) * FONT_SIZE;
    polygon.tex[0].tex.u = tex_u;
    polygon.tex[0].tex.v = tex_v + FONT_SIZE - 1;
    polygon.tex[1].tex.u = tex_u;
    polygon.tex[1].tex.v = tex_v;
    polygon.tex[2].tex.u = tex_u + FONT_SIZE - 1;
    polygon.tex[2].tex.v = tex_v;
    polygon.tex[3].tex.u = tex_u + FONT_SIZE - 1;
    polygon.tex[3].tex.v = tex_v + FONT_SIZE - 1;
    njSetTexture(&alife_window_texlist);
    njSetTextureNum(0);
    njDrawPolygon3D(&polygon, 4, NAME_ATTR);
}

/* draw the window behind the letters, in white at color's alpha */
static void window_draw(NJS_COLOR *color)
{
    NJS_POINT3COL polygon;
    NJS_POINT3 window_points[4] = {
#include "al_main/chao_name/window_points.inc"
    };
    NJS_COLOR colors[4];
    NJS_COLOR uvs[4];

    polygon.p = window_points;
    polygon.col = colors;
    polygon.tex = uvs;
    polygon.num = 4;
    polygon.col[0].color = 0xffffffff;
    polygon.col[1].color = 0xffffffff;
    polygon.col[2].color = 0xffffffff;
    polygon.col[3].color = 0xffffffff;
    polygon.col[0].argb.a = color->argb.a;
    polygon.col[1].argb.a = color->argb.a;
    polygon.col[2].argb.a = color->argb.a;
    polygon.col[3].argb.a = color->argb.a;
    polygon.tex[0].tex.u = 0;
    polygon.tex[0].tex.v = 255;
    polygon.tex[1].tex.u = 0;
    polygon.tex[1].tex.v = 0;
    polygon.tex[2].tex.u = 255;
    polygon.tex[2].tex.v = 0;
    polygon.tex[3].tex.u = 255;
    polygon.tex[3].tex.v = 255;
    njTextureFilterMode(NJD_TEXTUREFILTER_POINT_SAMPLE);
    njSetTexture(&alife_window_texlist);
    njSetTextureNum(1);
    njDrawPolygon3D(&polygon, 4, NAME_ATTR);
    njTextureFilterMode(NJD_TEXTUREFILTER_BILINEAR);
}

/* the name's center, which the draw moves by pos and the current
 * matrix */
static const NJS_POINT3 name_origin = {0.0F, 0.0F, 0.0F};

void chao_name_draw_at(struct chao *chao, NJS_POINT3 *pos, Float scale,
                       NJS_COLOR *color)
{
    NJS_POINT3 origin;
    NJS_POINT3 at;
    Uint8 *letter = chao->name;
    int count = 0;
    int i = 0;
    Float middle;
    Float x;

    origin = name_origin;
    if (*letter == 0) {
        return;
    }
    letter[CHAO_NAME_SIZE - 1] = 0;
    while (*letter != 0) {
        letter++;
        count++;
    }
    letter = chao->name;
    njPushMatrix(NULL);
    njTranslateV(NULL, pos);
    njCalcPoint(NULL, &origin, &at);
    njPopMatrix(1);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &at);
    middle = (count - 1.0F) * 0.5F;
    while (*letter != 0) {
        x = (i - middle) * scale;
        njPushMatrix(NULL);
        njTranslate(NULL, x, 0.0F, 0.0F);
        njScale(NULL, scale, scale, scale);
        letter_draw(*letter, color);
        letter++;
        njPopMatrix(1);
        i++;
        if (i == count) {
            break;
        }
    }
    njScale(NULL, scale, scale, scale);
    window_draw(color);
    njPopMatrix(1);
}

void chao_name_step(struct task_work *work, struct alife_chao *chao)
{
    NJS_POINT3 pos;
    NJS_COLOR color;
    Float scale;

    chao->name_alpha -= FADE_OUT;
    if (chao->flags & CHAO_SHOW_NAME) {
        if (chao->name_frames < FADE_FRAMES) {
            chao->name_alpha += FADE_IN;
            chao->name_frames++;
        }
    } else {
        chao->name_frames = 0;
    }
    if (chao->name_alpha > 255) {
        chao->name_alpha = 255;
    } else if (chao->name_alpha < 0) {
        chao->name_alpha = 0;
    }
    if (chao->name_alpha > NAME_SHOWN) {
        color.argb.r = chao->icon.red;
        color.argb.g = chao->icon.green;
        color.argb.b = chao->icon.blue;
        color.argb.a = chao->name_alpha;
        scale = chao->name_alpha * chao->scale * (1.0F / 255.0F);
        pos = work->pos;
        pos.y += (scale * 3.0F) + 5.0F;
        chao_name_draw_at(chao->record, &pos, scale, &color);
    }
}

void chao_name_draw(struct task_work *work, struct alife_chao *chao)
{
    NJS_POINT3 pos;
    NJS_COLOR color;
    Float scale;

    if (chao->name_alpha > NAME_SHOWN) {
        color.argb.r = chao->icon.red;
        color.argb.g = chao->icon.green;
        color.argb.b = chao->icon.blue;
        color.argb.a = chao->name_alpha;
        scale = chao->name_alpha * chao->scale * (1.0F / 255.0F);
        pos = work->pos;
        pos.y += (scale * 3.0F) + 5.0F;
        chao_name_draw_at(chao->record, &pos, scale, &color);
    }
}
