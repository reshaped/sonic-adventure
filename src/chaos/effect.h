/* the Chaos fights' sparks, flashes, trails, drops, spots and pulses, his
 * cyan light, and alpha fades */
#ifndef CHAOS_EFFECT_H
#define CHAOS_EFFECT_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;
struct task_work;

/* the spot's sprite, which the bosses draw with too */
extern NJS_SPRITE chaos_spot_sprite;

/* set up the sprites, and the effects' starting sizes and spark speed */
void chaos_effect_init(void);
/* a spot's step. the spot is a dark sprite laid flat where the boss stood
 * at its first step. it fades out, and lowers as it fades. its creator sets
 * its alpha in vars.f[0]. once the alpha is below 0.5, the spot shrinks
 * too */
void chaos_spot_step(struct task *task);
/* make count drops of size size, thrown up from height over the work */
void chaos_effect_splash(struct task_work *work, int count, Float size,
                         Float height);
/* make count sparks, or 2 when count is 0, that gather into pos */
void chaos_effect_gather(NJS_POINT3 *pos, int count);
/* make a flash that follows target. target must stay valid for the
 * flash's 20 steps */
void chaos_effect_flash(NJS_POINT3 *target);
/* make the one cyan point light, with range, and turn it on */
void chaos_light_create(Float range);
/* place the light at work's position */
void chaos_light_follow(struct task_work *work);
/* place the light height above pos */
void chaos_light_place(NJS_POINT3 *pos, Float height);
void chaos_light_delete(void);
/* pulse object's scale between 0.85 and 1.15, on a task that never ends
 * itself. the task isn't returned, so object must outlive it. owner is
 * unread */
void chaos_effect_pulse(NJS_OBJECT *object, struct task *owner);
/* make a trail of pieces from from toward to, and count sparks at from, or
 * 2 when count is 0. the pieces are evenly spaced at least 3.5 apart, and
 * the last stops a space short of to. from and to closer than 7 make one
 * piece, at from */
void chaos_effect_trail(NJS_POINT3 *from, NJS_POINT3 *to, int count);
/* set alpha, 1 for opaque, on every material of objects, a list a null
 * ends. each material's NJD_FLAG_USE_ALPHA is set too. every object needs
 * a model */
void chaos_set_alpha(NJS_OBJECT **objects, Float alpha);
/* fade objects' alpha from from to to over steps steps, on a child task of
 * parent. steps must be above 0. returns the child, or null. the fade's
 * record is allocated unchecked, and the list must outlive the fade */
struct task *chaos_fade_alpha(struct task *parent, NJS_OBJECT **objects,
                              Float from, Float to, int steps);
/* set the flashes', trails' and sparks' sizes and the sparks' speed. a
 * value of 0 or less leaves its setting as it is */
void chaos_effect_set_sizes(Float flash, Float trail, Float spark, Float speed);

#endif
