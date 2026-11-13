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
#ifdef NONMATCHING
#else
extern Uint32 draw_palettes[DRAW_PALETTES][DRAW_PALETTE_ENTRIES * 2];
#endif
/* the palette the player characters shade from. other characters and some
 * objects take it too */
#define DRAW_PLAYER_PALETTE 2
/* the offset colors of an NJD_FLAG_IGNORE_LIGHT material's points, by point
 * index. they are 0 until a stage's module fills them, or until a palettes
 * file longer than draw_palettes runs on over them, as light.h says */
#define DRAW_UNLIT_OFFSETS 512
#ifdef NONMATCHING
/* the most points a model may have. the largest in the game's images has
 * 1900 */
#define DRAW_MOST_POINTS 2048
/* how far into _nj_vertex_buf_, src/main.c's texture_buffer, the draws'
 * scratch may reach. model.c starts it at most 16 KB in, the operand cache's
 * size. each point takes 32 bytes of it */
#define DRAW_SCRATCH_START 0x4000
#define DRAW_SCRATCH_SIZE (DRAW_SCRATCH_START + (DRAW_MOST_POINTS * 32))
/* draw_palettes, then draw_unlit_offsets, as one object. C defines no read
 * past an array's end, so model.c reads the palettes through words, which
 * spans both arrays. the draws read an unlit point's offset color by the
 * point's index, so unlit_offsets holds one for each point a model may have.
 * those from DRAW_UNLIT_OFFSETS on stay 0 */
union draw_colors {
    struct {
        Uint32 palettes[DRAW_PALETTES][DRAW_PALETTE_ENTRIES * 2];
        Uint32 unlit_offsets[DRAW_MOST_POINTS];
    };
    Uint32 words[DRAW_PALETTES * DRAW_PALETTE_ENTRIES * 2 + DRAW_MOST_POINTS];
};
extern union draw_colors draw_colors;
#define draw_palettes (draw_colors.palettes)
#define draw_unlit_offsets (draw_colors.unlit_offsets)
#else
extern Uint32 draw_unlit_offsets[DRAW_UNLIT_OFFSETS];
#endif
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
#ifdef NONMATCHING
/* draw_tree and draw_action for models Ninja's constant attributes make
 * unlit and translucent in the constant material. these forms need
 * NJD_CONTROL_3D_CONSTANT_ATTR and NJD_CONTROL_3D_CONSTANT_MATERIAL, with
 * NJD_FLAG_IGNORE_LIGHT and NJD_FLAG_USE_ALPHA in _nj_constant_attr_or_.
 * without them draw_tree_like_ninja draws by njDrawObject, and
 * draw_action_like_ninja by njAction, as for an afterimage's lit copy. the
 * task that draws in these forms needs a draw callback other than its step.
 * a frame's second picture calls that callback. it replays other tasks'
 * models by draw_model, without these forms.
 *
 * no point shades, and no meshset's header takes an offset color. a point
 * takes the constant material's color and alpha. a meshset whose material
 * lacks NJD_FLAG_DOUBLE_SIDE loses its back faces through the header's cull
 * mode. Ninja's _cull drawers drop them too.
 *
 * the forms depart from njDrawModel in three ways. njDrawModel clamps the
 * constant material's color and alpha between 0 and 1. the forms clamp its
 * color alone. a draw drops a model whose bounding sphere is out of view.
 * the sphere takes the model's own radius, whatever the matrix's scale.
 * njDrawModel tests a sphere only under NJD_CONTROL_3D_MODEL_CLIP. a
 * single-sided quad or polygon loses each of its triangles that faces away.
 * Ninja's keeps or drops it whole as its first triangle faces */
void draw_tree_like_ninja(NJS_OBJECT *object);
void draw_action_like_ninja(NJS_ACTION *action, Float frame);
#endif
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
#ifdef NONMATCHING
/* draw_in_view's test on any step, for what step 1 draws for a frame's
 * second picture */
Bool draw_in_view_any_step(Float x, Float y, Float z, Float radius);
/* forget the land's shading, which the draws bake and keep across pictures,
 * for a land about to load */
void draw_bake_forget(void);
#endif

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
