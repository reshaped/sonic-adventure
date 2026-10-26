/* sprites: textured rectangles that face the screen and play through a list
 * of frames as they move. sprite_create runs one on a task of its own */
#ifndef EFFECT_SPRITE_H
#define EFFECT_SPRITE_H
#include <ninja.h>
#include <sg_xpt.h>

#include "system/task.h"

/* a frame's flags */
#define SPRITE_TEXTURE_SIZE 0x1 /* sprite_bind gives it its texture's size */
#define SPRITE_CENTERED 0x2 /* sprite_bind sets its center to its middle */
#define SPRITE_ADDITIVE 0x4 /* it adds color times alpha to what's behind */

/* the texture of the entry that ends a list, after its last frame.
 * SPRITE_END stops the list there, and SPRITE_LOOP starts it over.
 * sprite_play_step returns them as a list stops or starts over */
#define SPRITE_END (-1)
#define SPRITE_LOOP (-2)

/* a frame of a sprite's list: how the sprite looks once its play's age
 * reaches age. between two frames of one texture, the sprite blends from
 * the first to the second. width, height and the center are lengths in
 * view space, which the body's scale multiplies. the center is the point
 * drawn at pos, measured from the frame's top-left corner */
struct sprite_frame {
    Sint16 texture; /* in its texlist, or SPRITE_END or SPRITE_LOOP */
    Uint8 flags;
    Uint8 unk_03;
    /* the texture's global index, once sprite_bind has run. in a
     * SPRITE_LOOP entry, how many times the list plays, at least once */
    Uint32 index;
    Float width;
    Float height;
    Float center_x;
    Float center_y;
    Float age;
    NJS_ARGB color;
};

struct sprite_motion {
    Float drag; /* the share of its speed it loses each step */
    Float gravity; /* added to its y speed each step */
};

struct sprite_body {
    NJS_POINT3 pos;
    NJS_VECTOR speed;
    Float scale;
    /* added to its view z for the near clip and for the z it draws at, but
     * not for its projection */
    Float depth;
};

struct sprite_play {
    Float age;
    Float rate; /* added to age each step */
    int frame; /* the index of the frame it is on */
    /* how many times it has reached a SPRITE_LOOP list's last frame */
    Uint32 loops;
};

struct sprite {
    /* its own step, run in place of moving and playing, or null. it
     * returns as sprite_play_step does, and SPRITE_END ends sprite_create's
     * task */
    int (*step)(struct sprite *sprite);
    NJS_POINT3 *pos; /* where it draws: its body's pos, or the caller's point */
    struct sprite_body body;
    struct sprite_motion motion;
    const struct sprite_frame *frames;
    struct sprite_play play;
};

/* what sprite_set sets a sprite up from. the sprite copies all of it but
 * the frames, which must outlive the sprite */
struct sprite_desc {
    int (*step)(struct sprite *sprite);
    NJS_VECTOR speed;
    Float scale;
    Float depth;
    Float drag;
    Float gravity;
    const struct sprite_frame *frames;
    Float rate;
};

/* sprite_set's and sprite_create's flags. with SPRITE_MOVES the sprite
 * draws at its body, which starts at a copy of pos. without it the sprite
 * draws at pos itself, which must outlive it */
#define SPRITE_MOVES 0x1

/* sprite_bind's flags: load the texlist with njLoadTexture first */
#define SPRITE_LOAD 0x1

/* set play's age to age, and its frame to the last frame whose age is at
 * most age. for an age at or past the last frame's, it finds the frame for
 * age mod the last frame's age, whichever entry ends the list. play's age
 * stays age itself, not that remainder. its rate and loops stay as they
 * were */
void sprite_play_seek(struct sprite_play *play,
                      const struct sprite_frame *frames, Float age);
/* set each frame's index to its texture's global index in texlist, writing
 * through the const. a frame with SPRITE_TEXTURE_SIZE takes its texture's
 * size in pixels, and one with SPRITE_CENTERED half its size as its center.
 * a frame whose texture isn't loaded is left as it is. flags takes
 * SPRITE_LOAD */
void sprite_bind(NJS_TEXLIST *texlist, const struct sprite_frame *frames,
                 int flags);
/* start play on the first frame, at age 0 and a rate of 1, with no loops */
void sprite_play_init(struct sprite_play *play);
/* set sprite up from desc at pos, playing from the first frame at desc's
 * rate. flags takes SPRITE_MOVES */
void sprite_set(struct sprite *sprite, NJS_POINT3 *pos,
                struct sprite_desc *desc, int flags);
/* advance play by its rate. the play moves on to the next frame once its
 * age reaches that frame's, but never onto the list's last frame. as its
 * age reaches the last frame's, a SPRITE_LOOP list starts over, keeping the
 * age past it, until loops reaches the SPRITE_LOOP entry's count. the step
 * that starts it over returns SPRITE_LOOP. otherwise the list stops: the
 * play holds on the frame before the last, at the last frame's age. the
 * step that stops it and every later one return SPRITE_END, as does every
 * step of a list of one frame. any other step returns 0 */
int sprite_play_step(struct sprite_play *play,
                     const struct sprite_frame *frames);
/* run the sprite's own step, or without one, move its body and play on.
 * returns the own step's result, or sprite_play_step's */
int sprite_step(struct sprite *sprite);
/* draw the frame play is on at pos, blended toward the next as play ages,
 * at the body's scale and depth. it draws only while frame_step is 0, and
 * not when pos's view z plus depth is short of the near clip. drawing sets
 * Ninja's color blending for the frame, and leaves it so */
void sprite_render(NJS_POINT3 *pos, struct sprite_body *body,
                   struct sprite_play *play, const struct sprite_frame *frames);
/* draw the sprite at pos in place of sprite->pos */
void sprite_draw_at(struct sprite *sprite, NJS_POINT3 *pos);
/* make a task that draws and steps a sprite set up as sprite_set does. the
 * task kills itself once sprite_step returns SPRITE_END. returns the task,
 * or null when the task or the sprite can't be allocated */
struct task *sprite_create(NJS_POINT3 *pos, struct sprite_desc *desc,
                           int flags);

#endif
