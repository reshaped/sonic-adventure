/* an event's picture: a texture over the whole screen, in a color and at
 * an alpha */
#ifndef EVENT_PICTURE_H
#define EVENT_PICTURE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* make a picture of texture 0 of texlist: white, at alpha 0 and depth -1.
 * its blend is NJD_COLOR_BLENDING_SRCALPHA for the source and
 * NJD_COLOR_BLENDING_INVSRCALPHA for the destination. its step draws it as
 * well as its draw. returns null when task_create fails */
struct task *event_picture_create(NJS_TEXLIST *texlist);
/* the picture's red, green and blue, each 1 at full */
void event_picture_set_color(struct task *task, Float red, Float green,
                             Float blue);
/* move the picture's alpha to alpha over steps steps, by the same amount
 * each step. with steps 0 or below, the alpha stays where it is */
void event_picture_fade(struct task *task, Float alpha, int steps);
/* the priority njDrawSprite2D draws the picture at */
void event_picture_set_depth(struct task *task, Float depth);
/* the texture of the picture's texlist it draws, by index */
void event_picture_set_texture(struct task *task, int texture);
/* set the picture's blend for which, NJD_SOURCE_COLOR or
 * NJD_DESTINATION_COLOR, to mode, an NJD_COLOR_BLENDING_ mode */
void event_picture_set_blend(struct task *task, int which, Uint8 mode);
/* set every picture's height in pixels, which starts at 480. the pictures
 * share one texanim, and height is its sy */
void event_picture_set_height(Sint16 height);

#endif
