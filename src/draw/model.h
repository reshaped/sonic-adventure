/* the model renderer, in assembly: Ninja basic models, shaded from a palette
 * and sent straight to the tile accelerator's lists */
#ifndef DRAW_MODEL_H
#define DRAW_MODEL_H
#include <ninja.h>
#include <sg_xpt.h>

/* the palettes the models shade from. a point takes the entry its normal's
 * dot with the light's direction picks: -1 picks entry 0, and 1 the last.
 * an entry is a base color, then an offset color. a model whose first
 * meshset's material has a specular color, without NJD_FLAG_IGNORE_SPECULAR,
 * takes its offset colors from the next palette. the next after the last
 * is draw_unlit_offsets, which follows draw_palettes */
#define DRAW_PALETTES 8
#define DRAW_PALETTE_ENTRIES 256
extern Uint32 draw_palettes[DRAW_PALETTES][DRAW_PALETTE_ENTRIES * 2];
/* the palette the player characters shade from. other characters and some
 * objects take it too */
#define DRAW_PLAYER_PALETTE 2
/* the offset colors of an NJD_FLAG_IGNORE_LIGHT material's points, by point
 * index. they are 0 until a stage's module fills them, or until a palettes
 * file longer than draw_palettes runs on over them, as light.h says */
#define DRAW_UNLIT_OFFSETS 512
extern Uint32 draw_unlit_offsets[DRAW_UNLIT_OFFSETS];
/* the counts debug_page 2 shows. the frame timer's setup zeroes them, and
 * nothing counts into them */
#define DRAW_DEBUG_COUNTS 20
extern int draw_debug_counts[DRAW_DEBUG_COUNTS];

/* point GBR at the renderer's work area, and copy the screen's center and
 * distance, its aspects and the near clip there. the model draws,
 * draw_models_begin, draw_set_palette and draw_take_point_lights reach the
 * area through GBR, so they need this call first, and GBR unchanged since.
 * the draws project by the copies, so a change to any of them takes another
 * call. the models shade from draw_palettes[0] after it */
void draw_model_init(void);
/* draw model in the current matrix, between its own njGetkmPointer and
 * njSetkmPointer. it draws on a frame's first step only. it draws nothing
 * when the model's bounding sphere is out of view, or when no point lies
 * farther than _nj_near_clip_. for an odd nbPoint it reads the 12 bytes past
 * the points as one more, so it draws nothing too when that one and all but
 * one of the model's points lie no farther */
void draw_model(NJS_MODEL *model);
/* draw model as draw_model does, between an njGetkmPointer and an
 * njSetkmPointer the caller makes */
void draw_model_nested(NJS_MODEL *model);
/* draw a run of models. draw_models_begin reads Ninja's 3D control flags
 * and constant attributes for the whole run, then calls njGetkmPointer.
 * draw_model_unculled draws a model as draw_model does, on any step and
 * whatever its bounding sphere. draw_models_end calls njSetkmPointer */
void draw_models_begin(void);
void draw_model_unculled(NJS_MODEL *model);
void draw_models_end(void);
/* whether the current matrix places the sphere of radius about (x, y, z) in
 * view. FALSE on any step but a frame's first. FALSE too when the sphere lies
 * wholly nearer than _nj_near_clip_ or past _nj_far_clip_. FALSE when the edge
 * tests below put it wholly beyond an edge of _nj_2d_clip_, and TRUE
 * otherwise. the edge tests project the sphere's sides at z + radius. the side
 * tests scale x by _nj_screen_'s dist times _nj_scr_aspect_x_. the top test
 * scales y by _nj_scr_aspect_y_ times _nj_screen_'s cx, not its dist. the
 * bottom test scales y as the side tests do, and compares it with the clip's
 * right edge, not its bottom */
Bool draw_in_view(Float x, Float y, Float z, Float radius);

/* the direction of the light the models shade by, in view space.
 * draw_set_light sets it and a copy of it. draw_divide_light sets it to the
 * copy divided by scale. a model drawn under an njScale of scale on every
 * axis then shades as it would unscaled. draw_reset_light sets it back to
 * the copy */
void draw_set_light(Float x, Float y, Float z);
void draw_divide_light(Float scale);
void draw_reset_light(void);
/* shade the models from draw_palettes[palette], until the next call or
 * draw_model_init. a model with a specular color then takes its offset
 * colors from the next palette, so with palette 7 it takes them from
 * draw_unlit_offsets */
void draw_set_palette(int palette);
/* take the point lights queued since the last call into the renderer's
 * list, turned by the current matrix, and empty the queue. no code queues a
 * point light, and no draw reads the list */
void draw_take_point_lights(void);

#endif
