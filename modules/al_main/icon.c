/* a Chao's icon: the ball over its head, and a symbol that springs up from
 * it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "al_main/chao.h"
#include "al_main/icon.h"
#include "chao/chao.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 9
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 9
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 9
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 9
#define TEXT_11_LENGTH 10
#define TEXT_12_LENGTH 10
#define TEXT_13_LENGTH 10
#define TEXT_14_LENGTH 10
#include "al_main/icon/text.inc"

#define ICON_ATTR (NJD_USE_TEXTURE | NJD_TRANSPARENT | NJD_FILL)

/* a random Angle, drawn in degrees and converted */
#define RANDOM_TURN()                                                          \
    ((Angle)(((Float)rand() * RAND_SCALE) * 360.0F * ANGLE_PER_DEGREE))

/* the textures' indices in alife_icon_texlist */
#define TEX_NONE 0 /* the symbol's, while the ball is alone */
#define TEX_BALL 1
#define TEX_GLAD 2
#define TEX_SYMBOL_3 3
#define TEX_SYMBOL_4 4
#define TEX_SYMBOL_5 5
#define TEX_HURT 6
#define TEX_RING 11

#define RING_COLOR 0xccffffff

#define BALL_PULSE 0xc00

/* @bug logic: a symbol springs up once symbol.frames passes this.
 * nothing restarts symbol.frames, which each call of alife_icon_step and
 * alife_icon_draw counts up, so the test fails only for 11 counts in each
 * 65536. a symbol so waits only the call after it starts (CMP/GT R3,R1 at
 * 0x0c9093c8 in icon_glad_step, and the same in each symbol's step) */
#define SYMBOL_WAIT 10

enum symbol_state { SYMBOL_START, SYMBOL_WAITING, SYMBOL_UP, SYMBOL_AWAY };
enum ball_state { BALL_START, BALL_SQUASH, BALL_SPRING };

/* end the symbol, and turn the icon back to the ball alone */
/* @hack spelling: the icon written through chao, not through icon. SHC
 * addresses each store from R5, with its offset in R0 (0x0c9094f2 to
 * 0x0c909500 in icon_glad_step); through icon, the five symbol steps run 192
 * bytes shorter and 1287 of 1373 words differ */
#define ICON_BACK_TO_BALL(chao)                                                \
    {                                                                          \
        (chao)->icon.kind = ICON_BALL;                                         \
        (chao)->icon.symbol.state = SYMBOL_START;                              \
        (chao)->icon.ball.state = BALL_START;                                  \
        (chao)->icon.timer = 0;                                                \
    }

static NJS_TEXNAME icon_textures[14] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0},
};
/* the icon's tint for each type of Chao */
static NJS_BGRA icon_tints[7] = {
#include "al_main/icon/icon_tints.inc"
};
NJS_TEXLIST alife_icon_texlist = {
#include "al_main/icon/alife_icon_texlist.inc"
};

/* draw the ball, then the symbol, each a square of its own texture and size
 * at its pos from the icon's. both take the icon's color, scaled by the tint
 * of the Chao's type */
static void icon_draw(struct alife_chao *chao, struct chao_icon *icon)
{
    NJS_POINT3COL poly;
    NJS_POINT3 points[4];
    NJS_COLOR colors[4];
    NJS_COLOR uvs[4];
    NJS_COLOR color;
    Uint8 type = chao->record->type;
    Float scale = chao->scale;
    Float shade;

    /* @hack spelling: each shade in two statements, the product and then its
     * scale by 1 / 255, which has SHC load the 1 / 255 after the first shade's
     * reads (MOVA at 0x0c908db2). in one statement each, the product times the
     * 1 / 255, which groups the same, SHC loads it at the entry and 235 of 277
     * words differ */
    shade = (Float)icon->red * icon_tints[type].r;
    shade *= 1.0F / 255.0F;
    color.argb.r = shade;
    shade = (Float)icon->green * icon_tints[type].g;
    shade *= 1.0F / 255.0F;
    color.argb.g = shade;
    shade = (Float)icon->blue * icon_tints[type].b;
    shade *= 1.0F / 255.0F;
    color.argb.b = shade;
    color.argb.a = 255;
    poly.p = points;
    poly.col = colors;
    poly.tex = uvs;
    poly.num = 4;
    poly.p[0].x = -0.5F;
    poly.p[0].y = 0.5F;
    poly.p[0].z = -0.4F;
    poly.tex[0].tex.u = 0;
    poly.tex[0].tex.v = 0;
    poly.col[0].color = color.color;
    poly.p[1].x = -0.5F;
    poly.p[1].y = -0.5F;
    poly.p[1].z = -0.4F;
    poly.tex[1].tex.u = 0;
    poly.tex[1].tex.v = 255;
    poly.col[1].color = color.color;
    poly.p[2].x = 0.5F;
    poly.p[2].y = -0.5F;
    poly.p[2].z = -0.4F;
    poly.tex[2].tex.u = 255;
    poly.tex[2].tex.v = 255;
    poly.col[2].color = color.color;
    poly.p[3].x = 0.5F;
    poly.p[3].y = 0.5F;
    poly.p[3].z = -0.4F;
    poly.tex[3].tex.u = 255;
    poly.tex[3].tex.v = 0;
    poly.col[3].color = color.color;
    njSetTexture(&alife_icon_texlist);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &icon->pos);
    njScale(NULL, scale, -scale, scale);
    njPushMatrix(NULL);
    njTranslateV(NULL, &icon->ball.pos);
    njSetTextureNum(icon->ball.texture);
    njScale(NULL, icon->ball.size.x, icon->ball.size.y, 1.0F);
    njDrawPolygon3D(&poly, 4, ICON_ATTR);
    njPopMatrix(1);
    njTranslateV(NULL, &icon->symbol.pos);
    njSetTextureNum(icon->symbol.texture);
    njScale(NULL, icon->symbol.size.x, icon->symbol.size.y, 1.0F);
    njDrawPolygon3D(&poly, 4, ICON_ATTR);
    njPopMatrix(1);
}

/* draw a Chaos Chao's icon, the ring of light. it is a square of TEX_RING
 * at twice the scale, added to what's behind it. each draw turns it at
 * random about z */
static void icon_draw_chaos(struct alife_chao *chao, struct chao_icon *icon)
{
    NJS_POINT3COL poly;
    NJS_POINT3 points[4];
    NJS_COLOR colors[4];
    NJS_COLOR uvs[4];
    NJS_COLOR color;
    Float scale;

    scale = chao->scale * 2.0F;
    color.color = RING_COLOR;
    poly.p = points;
    poly.col = colors;
    poly.tex = uvs;
    poly.num = 4;
    poly.p[0].x = -0.5F;
    poly.p[0].y = 0.5F;
    poly.p[0].z = -0.4F;
    poly.tex[0].tex.u = 0;
    poly.tex[0].tex.v = 0;
    poly.col[0].color = color.color;
    poly.p[1].x = -0.5F;
    poly.p[1].y = -0.5F;
    poly.p[1].z = -0.4F;
    poly.tex[1].tex.u = 0;
    poly.tex[1].tex.v = 255;
    poly.col[1].color = color.color;
    poly.p[2].x = 0.5F;
    poly.p[2].y = -0.5F;
    poly.p[2].z = -0.4F;
    poly.tex[2].tex.u = 255;
    poly.tex[2].tex.v = 255;
    poly.col[2].color = color.color;
    poly.p[3].x = 0.5F;
    poly.p[3].y = 0.5F;
    poly.p[3].z = -0.4F;
    poly.tex[3].tex.u = 255;
    poly.tex[3].tex.v = 0;
    poly.col[3].color = color.color;
    njSetTexture(&alife_icon_texlist);
    njPushMatrix(&_unit_matrix_);
    njTranslateV(NULL, &icon->pos);
    njScale(NULL, scale, -scale, scale);
    /* the test draws one random turn, and the turn another */
    if (RANDOM_TURN() != 0) {
        njRotateZ(NULL, RANDOM_TURN());
    }
    njTranslateV(NULL, &icon->ball.pos);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetTextureNum(TEX_RING);
    njDrawPolygon3D(&poly, 4, ICON_ATTR);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njPopMatrix(1);
}

/* ICON_BALL's step: the ball alone, growing wider and taller by turns. the
 * symbol above it shows TEX_NONE */
static void icon_ball_step(struct task_work *work, struct alife_chao *chao)
{
    struct chao_icon *icon = &chao->icon;

    switch (icon->ball.state) {
    case BALL_START:
        icon->symbol.texture = TEX_NONE;
        icon->symbol.pos.x = 0.0F;
        icon->symbol.pos.y = 2.5F;
        icon->symbol.pos.z = 0.0F;
        icon->symbol.size.x = 3.5F;
        icon->symbol.size.y = 3.5F;
        icon->ball.texture = TEX_BALL;
        icon->ball.pos.x = 0.0F;
        icon->ball.pos.y = 0.0F;
        icon->ball.pos.z = 0.0F;
        icon->ball.size.x = 1.0F;
        icon->ball.size.y = 1.0F;
        icon->phase = 0;
        icon->ball.state++;
        break;
    case BALL_SQUASH:
        icon->ball.size.x = 1.0F + (njSin(icon->phase) * 0.15F);
        icon->ball.size.y = 2.0F - icon->ball.size.x;
        break;
    }
    icon->phase += BALL_PULSE;
}

/* the symbol of ICON_GLAD_1 to ICON_GLAD_3, TEX_GLAD. it springs up from the
 * ball to its size and height, and holds there while the icon's timer runs.
 * then it shrinks and sinks back, and the icon turns back to the ball alone.
 * the ball meanwhile squashes to half its width, and springs back */
static void icon_glad_step(struct task_work *work, struct alife_chao *chao)
{
    struct chao_icon *icon = &chao->icon;
    Float off;

    switch (icon->symbol.state) {
    case SYMBOL_START:
        icon->symbol.texture = TEX_GLAD;
        icon->symbol.pos.x = 0.0F;
        icon->symbol.pos.y = 0.0F;
        icon->symbol.pos.z = 0.01F;
        icon->symbol.speed.y = 0.0F;
        icon->symbol.size.x = 0.5F;
        icon->symbol.size.y = 0.2F;
        icon->symbol.grow.x = 0.0F;
        icon->symbol.grow.y = 0.0F;
        icon->symbol.state++;
        break;
    case SYMBOL_WAITING:
        if (icon->symbol.frames > SYMBOL_WAIT) {
            icon->symbol.state++;
        }
        break;
    case SYMBOL_UP:
        off = icon->symbol.size.x - 3.0F;
        icon->symbol.grow.x += (-icon->symbol.grow.x * 0.08F) - (off * 0.32F);
        icon->symbol.size.x += icon->symbol.grow.x;
        off = icon->symbol.size.y - 3.0F;
        icon->symbol.grow.y += (-icon->symbol.grow.y * 0.12F) - (off * 0.32F);
        icon->symbol.size.y += icon->symbol.grow.y;
        off = icon->symbol.pos.y - 2.2F;
        icon->symbol.speed.y += (-icon->symbol.speed.y * 0.16F) - (off * 0.4F);
        icon->symbol.pos.y += icon->symbol.speed.y;
        if (icon->timer-- == 0) {
            icon->symbol.state++;
        }
        break;
    case SYMBOL_AWAY:
        icon->symbol.size.x -= 0.4F;
        if (icon->symbol.size.x < 0.0F) {
            icon->symbol.size.x = 0.0F;
        }
        icon->symbol.size.y -= 0.4F;
        if (icon->symbol.size.y < 0.0F) {
            icon->symbol.size.y = 0.0F;
        }
        icon->symbol.pos.y -= 0.4F;
        if (icon->symbol.pos.y < 0.0F) {
            icon->symbol.pos.y = 0.0F;
            ICON_BACK_TO_BALL(chao);
        }
        break;
    }
    switch (icon->ball.state) {
    case BALL_START:
        icon->ball.texture = TEX_BALL;
        icon->ball.pos.x = 0.0F;
        icon->ball.pos.y = 0.0F;
        icon->ball.pos.z = 0.0F;
        icon->ball.size.x = 1.0F;
        icon->ball.size.y = 1.0F;
        icon->ball.state++;
        return;
    case BALL_SQUASH:
        icon->ball.size.x -= 0.05F;
        icon->ball.size.y = 2.0F - icon->ball.size.x;
        if (icon->ball.size.x < 0.5F) {
            icon->ball.state++;
        }
        return;
    case BALL_SPRING:
        off = icon->ball.size.x - 1.0F;
        icon->ball.grow.x += (-icon->ball.grow.x * 0.03F) - (off * 0.32F);
        icon->ball.size.x += icon->ball.grow.x;
        icon->ball.size.y = 2.0F - icon->ball.size.x;
        return;
    default:
        return;
    }
}

/* ICON_SYMBOL_3's symbol, TEX_SYMBOL_3: ICON_GLAD's, but larger and higher,
 * and it narrows faster as it shrinks away */
static void icon_symbol_3_step(struct task_work *work, struct alife_chao *chao)
{
    struct chao_icon *icon = &chao->icon;
    Float off;

    switch (icon->symbol.state) {
    case SYMBOL_START:
        icon->symbol.texture = TEX_SYMBOL_3;
        icon->symbol.pos.x = 0.0F;
        icon->symbol.pos.y = 0.0F;
        icon->symbol.pos.z = 0.01F;
        icon->symbol.speed.y = 0.0F;
        icon->symbol.size.x = 0.5F;
        icon->symbol.size.y = 0.2F;
        icon->symbol.grow.x = 0.0F;
        icon->symbol.grow.y = 0.0F;
        icon->symbol.state++;
        break;
    case SYMBOL_WAITING:
        if (icon->symbol.frames > SYMBOL_WAIT) {
            icon->symbol.state++;
        }
        break;
    case SYMBOL_UP:
        off = icon->symbol.size.x - 3.5F;
        icon->symbol.grow.x += (-icon->symbol.grow.x * 0.06F) - (off * 0.12F);
        icon->symbol.size.x += icon->symbol.grow.x;
        off = icon->symbol.size.y - 3.5F;
        icon->symbol.grow.y += (-icon->symbol.grow.y * 0.12F) - (off * 0.32F);
        icon->symbol.size.y += icon->symbol.grow.y;
        off = icon->symbol.pos.y - 2.5F;
        icon->symbol.speed.y += (-icon->symbol.speed.y * 0.16F) - (off * 0.4F);
        icon->symbol.pos.y += icon->symbol.speed.y;
        if (icon->timer-- == 0) {
            icon->symbol.state++;
        }
        break;
    case SYMBOL_AWAY:
        icon->symbol.size.x -= 0.7F;
        if (icon->symbol.size.x < 0.0F) {
            icon->symbol.size.x = 0.0F;
        }
        icon->symbol.size.y -= 0.4F;
        if (icon->symbol.size.y < 0.0F) {
            icon->symbol.size.y = 0.0F;
        }
        icon->symbol.pos.y -= 0.4F;
        if (icon->symbol.pos.y < 0.0F) {
            icon->symbol.pos.y = 0.0F;
            ICON_BACK_TO_BALL(chao);
        }
        break;
    }
    switch (icon->ball.state) {
    case BALL_START:
        icon->ball.texture = TEX_BALL;
        icon->ball.pos.x = 0.0F;
        icon->ball.pos.y = 0.0F;
        icon->ball.pos.z = 0.0F;
        icon->ball.size.x = 1.0F;
        icon->ball.size.y = 1.0F;
        icon->ball.state++;
        return;
    case BALL_SQUASH:
        icon->ball.size.x -= 0.05F;
        icon->ball.size.y = 2.0F - icon->ball.size.x;
        if (icon->ball.size.x < 0.5F) {
            icon->ball.state++;
        }
        return;
    case BALL_SPRING:
        off = icon->ball.size.x - 1.0F;
        icon->ball.grow.x += (-icon->ball.grow.x * 0.03F) - (off * 0.32F);
        icon->ball.size.x += icon->ball.grow.x;
        icon->ball.size.y = 2.0F - icon->ball.size.x;
        return;
    default:
        return;
    }
}

/* under ICON_SYMBOL_4's symbol, the ball springs back wider and flatter */
static void icon_symbol_4_step(struct task_work *work, struct alife_chao *chao)
{
    struct chao_icon *icon = &chao->icon;
    Float off;

    switch (icon->symbol.state) {
    case SYMBOL_START:
        icon->symbol.texture = TEX_SYMBOL_4;
        icon->symbol.pos.x = -0.2F;
        icon->symbol.pos.y = 0.0F;
        icon->symbol.pos.z = 0.01F;
        icon->symbol.speed.y = 0.0F;
        icon->symbol.size.x = 0.5F;
        icon->symbol.size.y = 0.2F;
        icon->symbol.grow.x = 0.0F;
        icon->symbol.grow.y = 0.0F;
        icon->symbol.state++;
        break;
    case SYMBOL_WAITING:
        if (icon->symbol.frames > SYMBOL_WAIT) {
            icon->symbol.state++;
        }
        break;
    case SYMBOL_UP:
        off = icon->symbol.size.x - 3.5F;
        icon->symbol.grow.x += (-icon->symbol.grow.x * 0.3F) - (off * 0.32F);
        icon->symbol.size.x += icon->symbol.grow.x;
        off = icon->symbol.size.y - 3.5F;
        icon->symbol.grow.y += (-icon->symbol.grow.y * 0.3F) - (off * 0.32F);
        icon->symbol.size.y += icon->symbol.grow.y;
        off = icon->symbol.pos.y - 1.8F;
        icon->symbol.speed.y += (-icon->symbol.speed.y * 0.5F) - (off * 0.4F);
        icon->symbol.pos.y += icon->symbol.speed.y;
        if (icon->timer-- == 0) {
            icon->symbol.state++;
        }
        break;
    case SYMBOL_AWAY:
        icon->symbol.size.x -= 0.4F;
        if (icon->symbol.size.x < 0.0F) {
            icon->symbol.size.x = 0.0F;
        }
        icon->symbol.size.y -= 0.4F;
        if (icon->symbol.size.y < 0.0F) {
            icon->symbol.size.y = 0.0F;
        }
        icon->symbol.pos.y -= 0.25F;
        if (icon->symbol.pos.y < 0.0F) {
            icon->symbol.pos.y = 0.0F;
            ICON_BACK_TO_BALL(chao);
        }
        break;
    }
    switch (icon->ball.state) {
    case BALL_START:
        icon->ball.texture = TEX_BALL;
        icon->ball.pos.x = 0.0F;
        icon->ball.pos.y = 0.0F;
        icon->ball.pos.z = 0.0F;
        icon->ball.size.x = 1.0F;
        icon->ball.size.y = 1.0F;
        icon->ball.state++;
        return;
    case BALL_SQUASH:
        icon->ball.size.x -= 0.05F;
        icon->ball.size.y = 2.0F - icon->ball.size.x;
        if (icon->ball.size.x < 0.5F) {
            icon->ball.state++;
        }
        return;
    case BALL_SPRING:
        off = icon->ball.size.x - 1.5F;
        icon->ball.grow.x += (-icon->ball.grow.x * 0.3F) - (off * 0.32F);
        icon->ball.size.x += icon->ball.grow.x;
        icon->ball.size.y = (3.0F - icon->ball.size.x) * 0.7F;
        return;
    default:
        return;
    }
}

/* ICON_SYMBOL_5's symbol, TEX_SYMBOL_5, which grows from nothing as it
 * rises, and shrinks away without sinking */
static void icon_symbol_5_step(struct task_work *work, struct alife_chao *chao)
{
    struct chao_icon *icon = &chao->icon;
    Float off;

    switch (icon->symbol.state) {
    case SYMBOL_START:
        icon->symbol.texture = TEX_SYMBOL_5;
        icon->symbol.pos.x = 0.0F;
        icon->symbol.pos.y = 0.0F;
        icon->symbol.pos.z = -0.01F;
        icon->symbol.speed.y = 0.0F;
        icon->symbol.size.x = 0.0F;
        icon->symbol.size.y = 0.0F;
        icon->symbol.grow.x = 0.0F;
        icon->symbol.grow.y = 0.0F;
        icon->symbol.state++;
        break;
    case SYMBOL_WAITING:
        if (icon->symbol.frames > SYMBOL_WAIT) {
            icon->symbol.state++;
        }
        break;
    case SYMBOL_UP:
        off = icon->symbol.size.x - 3.3F;
        icon->symbol.grow.x += (-icon->symbol.grow.x * 0.08F) - (off * 0.15F);
        icon->symbol.size.x += icon->symbol.grow.x;
        off = icon->symbol.size.y - 3.1F;
        icon->symbol.grow.y += (-icon->symbol.grow.y * 0.12F) - (off * 0.32F);
        icon->symbol.size.y += icon->symbol.grow.y;
        off = icon->symbol.pos.y - 1.0F;
        icon->symbol.speed.y += (-icon->symbol.speed.y * 0.16F) - (off * 0.4F);
        icon->symbol.pos.y += icon->symbol.speed.y;
        if (icon->timer-- == 0) {
            icon->symbol.state++;
        }
        break;
    case SYMBOL_AWAY:
        icon->symbol.size.x -= 0.4F;
        if (icon->symbol.size.x < 0.0F) {
            icon->symbol.size.x = 0.0F;
        }
        icon->symbol.size.y -= 0.4F;
        if (icon->symbol.size.y < 0.0F) {
            icon->symbol.size.y = 0.0F;
            ICON_BACK_TO_BALL(chao);
        }
        break;
    }
    switch (icon->ball.state) {
    case BALL_START:
        icon->ball.texture = TEX_BALL;
        icon->ball.pos.x = 0.0F;
        icon->ball.pos.y = 0.0F;
        icon->ball.pos.z = 0.0F;
        icon->ball.size.x = 1.0F;
        icon->ball.size.y = 1.0F;
        icon->ball.state++;
        return;
    case BALL_SQUASH:
        icon->ball.size.x -= 0.05F;
        icon->ball.size.y = 2.0F - icon->ball.size.x;
        if (icon->ball.size.x < 0.5F) {
            icon->ball.state++;
        }
        return;
    case BALL_SPRING:
        off = icon->ball.size.x - 1.0F;
        icon->ball.grow.x += (-icon->ball.grow.x * 0.03F) - (off * 0.32F);
        icon->ball.size.x += icon->ball.grow.x;
        icon->ball.size.y = 2.0F - icon->ball.size.x;
        return;
    default:
        return;
    }
}

/* ICON_HURT's symbol, TEX_HURT: ICON_SYMBOL_5's, but larger and higher */
static void icon_hurt_step(struct task_work *work, struct alife_chao *chao)
{
    struct chao_icon *icon = &chao->icon;
    Float off;

    switch (icon->symbol.state) {
    case SYMBOL_START:
        icon->symbol.texture = TEX_HURT;
        icon->symbol.pos.x = 0.0F;
        icon->symbol.pos.y = 0.0F;
        icon->symbol.pos.z = -0.01F;
        icon->symbol.speed.y = 0.0F;
        icon->symbol.size.x = 0.0F;
        icon->symbol.size.y = 0.0F;
        icon->symbol.grow.x = 0.0F;
        icon->symbol.grow.y = 0.0F;
        icon->symbol.state++;
        break;
    case SYMBOL_WAITING:
        if (icon->symbol.frames > SYMBOL_WAIT) {
            icon->symbol.state++;
        }
        break;
    case SYMBOL_UP:
        off = icon->symbol.size.x - 3.8F;
        icon->symbol.grow.x += (-icon->symbol.grow.x * 0.08F) - (off * 0.15F);
        icon->symbol.size.x += icon->symbol.grow.x;
        off = icon->symbol.size.y - 3.8F;
        icon->symbol.grow.y += (-icon->symbol.grow.y * 0.12F) - (off * 0.32F);
        icon->symbol.size.y += icon->symbol.grow.y;
        off = icon->symbol.pos.y - 2.2F;
        icon->symbol.speed.y += (-icon->symbol.speed.y * 0.16F) - (off * 0.4F);
        icon->symbol.pos.y += icon->symbol.speed.y;
        /* @bug logic: the spring on x adds to speed.y and pos.y, where
         * speed.x and pos.x are meant (FMOV.S FR0,@(R0,R4) at 0x0c909f96 and
         * 0x0c909fa4, with R0 84 and 72). pos.x stays 0, and pos.y moves twice
         * each step */
        off = icon->symbol.pos.x - 0.5F;
        icon->symbol.speed.y += (-icon->symbol.speed.y * 0.16F) - (off * 0.4F);
        icon->symbol.pos.y += icon->symbol.speed.y;
        if (icon->timer-- == 0) {
            icon->symbol.state++;
        }
        break;
    case SYMBOL_AWAY:
        icon->symbol.size.x -= 0.4F;
        if (icon->symbol.size.x < 0.0F) {
            icon->symbol.size.x = 0.0F;
        }
        icon->symbol.size.y -= 0.4F;
        if (icon->symbol.size.y < 0.0F) {
            icon->symbol.size.y = 0.0F;
            ICON_BACK_TO_BALL(chao);
        }
        break;
    }
    switch (icon->ball.state) {
    case BALL_START:
        icon->ball.texture = TEX_BALL;
        icon->ball.pos.x = 0.0F;
        icon->ball.pos.y = 0.0F;
        icon->ball.pos.z = 0.0F;
        icon->ball.size.x = 1.0F;
        icon->ball.size.y = 1.0F;
        icon->ball.state++;
        return;
    case BALL_SQUASH:
        icon->ball.size.x -= 0.05F;
        icon->ball.size.y = 2.0F - icon->ball.size.x;
        if (icon->ball.size.x < 0.5F) {
            icon->ball.state++;
        }
        return;
    case BALL_SPRING:
        off = icon->ball.size.x - 1.0F;
        icon->ball.grow.x += (-icon->ball.grow.x * 0.03F) - (off * 0.32F);
        icon->ball.size.x += icon->ball.grow.x;
        icon->ball.size.y = 2.0F - icon->ball.size.x;
        return;
    default:
        return;
    }
}

void alife_icon_clear(struct task_work *work, struct alife_chao *chao)
{
    struct chao_icon *icon = &chao->icon;

    icon->kind = ICON_BALL;
    icon->unk_06 = 0xffff;
}

void alife_icon_step(struct task_work *work, struct alife_chao *chao)
{
    struct chao_icon *icon = &chao->icon;

    switch (icon->kind) {
    case ICON_BALL:
        icon_ball_step(work, chao);
        break;
    case ICON_GLAD_1:
    case ICON_GLAD:
    case ICON_GLAD_3:
        icon_glad_step(work, chao);
        break;
    case ICON_SYMBOL_3:
        icon_symbol_3_step(work, chao);
        break;
    case ICON_SYMBOL_4:
        icon_symbol_4_step(work, chao);
        break;
    case ICON_SYMBOL_5:
        icon_symbol_5_step(work, chao);
        break;
    case ICON_HURT:
        icon_hurt_step(work, chao);
        break;
    case ICON_HOLD:
    case ICON_HOLD_2:
        break;
    }
    if (chao->record->type != CHAO_CHAOS) {
        icon_draw(chao, icon);
    } else {
        icon_draw_chaos(chao, icon);
    }
    icon->symbol.frames++;
    icon->ball.frames++;
}

void alife_icon_draw(struct task_work *work, struct alife_chao *chao)
{
    struct chao_icon *icon = &chao->icon;

    if (chao->record->type != CHAO_CHAOS) {
        icon_draw(chao, icon);
    } else {
        icon_draw_chaos(chao, icon);
    }
    icon->symbol.frames++;
    icon->ball.frames++;
}
