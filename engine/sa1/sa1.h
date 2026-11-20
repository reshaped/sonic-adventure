/* the game adapter's own parts: what turns the game's draws into the
 * contracts' records. it compiles against the game's headers, with Ninja's
 * models and motions in the SH-4's layout. it reads Ninja's real symbols at
 * each call. nothing past the adapter reads them, or a
 * game pointer, once the call returns */
#ifndef SA1_H
#define SA1_H
#include <ninja.h>
#include <stddef.h>
#include <stdint.h>

#include "sa_capture.h"

/* where a draw reads its header words. SA1_CONTEXT reads Ninja's context,
 * where a draw's own njGetkmPointer finds them. SA1_RUN reads
 * _nj_parameterkm_, the copy the last njGetkmPointer made */
enum sa1_header { SA1_CONTEXT, SA1_RUN };

/* end the open picture, if one is */
void sa1_picture_end(void);
/* TRUE from where frame_wait ends the frame's picture until frame_wait
 * returns. the overlay draws nothing meanwhile. the next frame_wait draws it
 * into the next picture as that call begins */
extern int sa1_frame_shown;

/* Ninja's 3D control flags and constant attributes as a draw takes them. a
 * draw in a run takes them as live.h's sa1_live_run says, and any other at
 * its call. forms is TRUE for a draw in the Ninja forms of src/draw/model.h.
 * a nested draw takes it as sa1_live_forms last set it. a draw by one of
 * Ninja's own drawers takes it from the flags and attributes */
struct sa1_attrs {
    int constant;
    uint32_t flags, and_bits, or_bits;
    int forms;
};
/* the attributes Ninja's state holds now, with forms FALSE */
struct sa1_attrs sa1_attrs_now(void);
/* a draw's record, made as the call begins from the state it finds. flags
 * are its SA_DRAW_*: SA_DRAW_OUT where the drawer's own test sends nothing */
void sa1_record_model(const NJS_MODEL *model, uint8_t mode,
                      enum sa1_header header, const struct sa1_attrs *attrs,
                      uint16_t flags);
/* a sprite's record. y_scale is the y scale the caller's Dreamcast drawer
 * sends, sprite->sx or sprite->sy */
void sa1_record_sprite(const NJS_SPRITE *sprite, int entry, uint32_t attr,
                       float y_scale, float bias, int screen);
void sa1_record_triangles(const NJS_POINT3COL *points, int count,
                          uint32_t attr);
void sa1_record_polygon_2d(const NJS_POINT2COL *p, int n, float pri,
                           uint32_t attr);
void sa1_record_lines(const NJS_POINT3COL *p, int n, uint32_t attr);
/* njDrawTexture's and njDrawPolygon's strip of count vertices on the
 * screen, a texture's named by its global index */
void sa1_record_screen_strip(const void *vertices, int count, int textured,
                             uint32_t global_index, int blend);
/* the overlay's text at x, y on the screen, in color. each character is a
 * square side pixels wide. font is the texels of Ninja's built-in font */
void sa1_record_text(const char *text, float x, float y, float side,
                     uint32_t color, const void *font);

/* the screen live.c's walk projects by, as sa1_live_screen takes it */
struct sa1_screen {
    float cx, cy, scale_x, scale_y, near_clip;
};
extern struct sa1_screen sa1_screen;
/* the background plane's colors, ARGB, as kmSetBackGroundPlane takes its
 * three corners: top left, top right and bottom left */
extern uint32_t sa1_back[3];
/* Ninja's light tint: a, r, g, b */
const float *sa1_light_tint(void);
/* the light Ninja's easy or simple drawer takes at the call. light is its
 * direction in view space */
void sa1_easy_light(double light[3], float *ambient, float *diffuse);
void sa1_simple_light(double light[3], float *ambient, float *diffuse);

/* TRUE where the held bytes at adx hold an ADX file whole, by the length its
 * header gives. sound.c's ADXT_StartMem reads a file to that length */
int sa1_adx_whole(const void *adx, size_t held);

#endif
