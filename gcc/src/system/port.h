/* the port's calls and state: the switches, the overlay, 16:9, the split
 * frame and the skip. ninja.h and frame.h include it under GCC */
#ifndef GCC_SYSTEM_PORT_H
#define GCC_SYSTEM_PORT_H
#include <ninja.h>
#include <sg_xpt.h>

struct shadow_place;
struct task;

/* syTmrGetCount's counts in a second, at 1.28 microseconds a count, and in
 * a vsync, at 59.94 Hz */
#define COUNTS_PER_SECOND 781250
#define VSYNC_COUNTS 13034

/* the port's switches, each off at boot. overlay_pad flips them */
enum port_toggle { TOGGLE_COUNTER, TOGGLE_WIDE, TOGGLE_FAST, TOGGLES };
/* TRUE where a switch is on: the counter's readout shows, or 16:9 or 60 fps
 * is asked for. frame_end applies a mode only where play lets it */
extern int port_toggles[TOGGLES];
/* fade a black curtain over the game in where on is TRUE, and out where it
 * is FALSE. while draw_off is TRUE it falls at once. the readout and the
 * note that names a switch as it flips show over it */
void port_curtain(int on);
/* TRUE where the curtain stood faded all the way in as overlay_advance last
 * ran, before it moved the fade on */
int port_curtain_shown(void);
/* add the frame to the counter, and age the note by vsyncs, the vsyncs the
 * frame took. the times are in syTmrGetCount's counts. wait is when the
 * frame's wait began, and busy the time from the end of the last frame's
 * wait to wait, less a split frame's wait for its middle vsync. reason is
 * present_reason's for the frame */
void overlay_end(Uint32 wait, Uint32 busy, int vsyncs, int reason);
/* fade the curtain a frame further. frame_end calls it once a frame. while
 * draw_off is TRUE it does nothing */
void overlay_advance(void);
/* draw the curtain, the readout and the note over the picture. a split
 * frame draws them over each of its pictures. while draw_off is TRUE it
 * does nothing */
void overlay_draw(void);
/* read the switches' chord after each serve of the pads. the chord is L
 * and R held on the first pad, and a fresh press of the d-pad: up steps the
 * counter's readout, right flips 16:9 and down flips 60 fps. from that
 * press until L, R and the d-pad are all up, the game sees none of them */
void overlay_pad(void);

/* 16:9. Ninja's aspect, _nj_scr_aspect_x_, stays the 1 njInitGlobal gives
 * it, so each projection the game makes lands in the 4:3 picture. 16:9 sets
 * two aspects of the port's in its place, each 1 or 3/4, and the rule is who
 * may read each. tests/test_wide.py holds the lists.
 *
 * port_draw_aspect is the picture's. at 3/4 the screen's 640 pixels hold a
 * view a third wider, which a 16:9 screen stretches back to its shape. the
 * draws read it: Ninja's renderers, the port's models and sprites, the land
 * list's clip, the 2D draws' squeeze, which the movie takes too, and an
 * event's picture that fills the screen: a backdrop, or a still whose color
 * adds to the scene's. the squeeze moves x by it, so that each 2D draw keeps
 * its shape on that screen: njDrawSprite2D, njDrawTexture,
 * njDrawTextureMemList, njDrawPolygon and njDrawPolygon2D. none of the
 * game's logic reads it.
 *
 * port_view_aspect is the window: how wide the game's tests of what is on
 * the screen take it to be. it is the only 16:9 the rest of the game's
 * logic reads, in five places, each answering as SEGA's code would at that
 * aspect, up to rounding: draw_in_view, enemy_on_screen, draw_on_screen,
 * npc_watch and the Sky Chase reticle's bounds. in play the window is the
 * picture's aspect, so objects in the wide sides act as in view.
 *
 * pillarbox holds the window at 1, so that the game is the 4:3 game, while
 * an attract demo or a recording runs. the picture stays 3/4, each 2D draw
 * squeezes about the middle whatever its anchor and cover, and black bands
 * cover the sides.
 *
 * port_wide_anchor is the pivot the 2D draws squeeze x about, in pixels
 * from the screen's middle, x 320: WIDE_MIDDLE, 0, at boot, and WIDE_LEFT
 * and WIDE_RIGHT at the screen's left and right edges. a piece that flies
 * from one pivot to another takes a pivot that travels with it, as
 * Knuckles' taken piece does. a layer that covers the screen, as a fade
 * does, keeps x, so that it stretches across it: an untextured polygon
 * whose corners port_wide_covers finds at both edges, and any 2D draw while
 * port_wide_cover is TRUE.
 *
 * a function that sets the anchor or the cover puts back WIDE_MIDDLE and
 * FALSE after its draws, before it returns. present_run and present_draw
 * start and end each task's step and callback at WIDE_MIDDLE and FALSE, and
 * a child's step, which its parent's step runs, at what the parent set, so
 * that what one task leaves set reaches no other's draws. what the draws
 * outside tasks leave set lasts until the next task's step or callback, or
 * wide_end */
#define WIDE_MIDDLE 0.0F
#define WIDE_LEFT (-320.0F)
#define WIDE_RIGHT 320.0F
extern Float port_wide_anchor;
extern Int port_wide_cover;
/* x as the 2D draws squeeze it: about the pivot, or the middle under
 * pillarbox */
Float port_wide_x(Float x);
/* TRUE where the 2D draws may squeeze: under 16:9 while port_wide_cover is
 * FALSE, and under pillarbox. an untextured polygon port_wide_covers finds
 * keeps x even then */
Int port_wide_squeezes(void);
/* TRUE outside pillarbox where an untextured polygon's count corners span
 * the screen's width, so that the polygon keeps x: each corner's x lies within
 * a pixel of 0 or 640, and corners lie at both. the corners' x lie stride
 * floats apart from x on */
Int port_wide_covers(const Float *x, Int count, Int stride);
/* the picture's aspect, 1 or 3/4 */
extern Float port_draw_aspect;
/* the window's aspect, 1 or 3/4 */
extern Float port_view_aspect;
/* x on the screen, as Ninja projects a point, moved to where the window
 * takes that point to be: _nj_screen_.cx plus x's offset from it scaled by
 * port_view_aspect, and x itself at an aspect of 1 */
Float port_view_x(Float x);
/* TRUE under pillarbox, until wide_end */
extern Int port_pillarbox;
/* 16:9's modes: off, on, and pillarbox, which holds the window at 1 */
enum wide_mode { WIDE_OFF, WIDE_ON, WIDE_PILLARBOX };
/* apply mode, setting both aspects and pillarbox. frame_end calls it
 * between frames, so that each frame draws at one aspect */
void wide_apply(enum wide_mode mode);
/* end the frame that wide_apply's mode drew: put back the anchor and the
 * cover, under pillarbox draw its bands, and end pillarbox, so that the
 * overlay drawn after takes its own cover */
void wide_end(void);
/* the mode wide_apply last applied */
extern enum wide_mode wide_applied;

/* frame_wait's call once the frame's wait ends. wait is when that wait
 * began, in syTmrGetCount's counts, and late the vsyncs the frame ran late.
 * it measures the frame, runs the skip, plans the next frame's split, draws
 * the overlay over this one and applies the next frame's 16:9 */
void frame_end(Uint32 wait, int late);
/* the split frame, which 60 fps takes. a frame of play runs its steps as
 * SEGA's 30 fps runs them, and shows two pictures: what step 0 drew, at the
 * frame's middle vsync, and after the last step a second, which draws as
 * SEGA's pause draws. a picture must change nothing a step reads.
 *
 * frame_redrawing is TRUE while the port draws a picture the shipped frame
 * doesn't: the second picture's pass. a draw then must change nothing a
 * step reads: the pass keeps rand()'s state, puts back each task's work after
 * its callback, and a player's motion, record and shapes, and SEGA's entries
 * that would register a collision, make a particle, play a sound, clear a
 * touch or end a placement refuse it */
extern int frame_redrawing;
/* TRUE from a frame's first picture until the next frame's step 0 ends, or
 * until the next frame's end where no stage loop runs. the sound server
 * then runs at each vsync */
extern int frame_split;
/* TRUE where frame_split is and step 1's models and sprite_draw_3d's
 * sprites draw for the second picture. it is FALSE where that picture may
 * take a camera pose between the pictures', since their draws take step 1's
 * view. the pass then draws each task, but for the sprites present.c
 * refuses */
extern int frame_later_draws;
/* TRUE on a step whose draws reach a picture: a frame's first, and step 1
 * where frame_later_draws is TRUE */
#define FRAME_DRAWS()                                                          \
    (frame_step == 0 || (frame_later_draws && frame_step == 1))
/* the stage loops' call after each step. after step 0 of a split frame it
 * replays the last frame's 2D and sprites from after its step 0, and shows
 * the picture at the frame's middle vsync */
void present_step(void);
/* the stage loops' call after their last step: the second picture, where the
 * first showed. it drops that picture where the pass or a record doesn't fit,
 * or where the camera moved over step 0 alone */
void present_second(void);
/* the readout's reason for a held split in the frame ending, given want and
 * late as present_end takes them: 0 where want is FALSE or no stage loop
 * ran, else 'T' where the frame ran late, else step 0's: 'R' where the PVR
 * couldn't take the first picture on time, 'T' where step 0, or step 1 and
 * the pass, didn't fit their half, and 0 otherwise */
int present_reason(int want, int late);
/* frame_wait's call before it renders. where present_second dropped a split
 * frame's second picture, it holds the first. it then uploads the queued
 * bitmaps, as the frame would, and waits out the vsync a flip would. it
 * returns TRUE where it holds the first picture, and else FALSE */
int present_hold(void);
/* the counts this frame's first picture waited for the middle vsync, which
 * the frame's busy time leaves out. 0 in a frame that doesn't split */
Uint32 present_idle(void);
/* frame_end's call once a frame, late the vsyncs the frame ran late. it
 * ends the frame's records, draws the overlay over its picture and plans the
 * next frame, which splits where want is TRUE, a stage loop ran this frame,
 * and the frames before it fit */
void present_end(int want, int late);
/* task.c's run of task's step, which keys the draws it makes to task. the
 * second picture's pass steps nothing */
void present_run(struct task *task, void (*step)(struct task *task));
/* task.c's draw of task by draw, which may be null. the second picture's
 * pass draws a task by its callback, by what it drew on step 1, or by what
 * it drew on step 0. it calls no killed task's callback, and replays a
 * killed task's 2D and njDrawSprite3D's sprites only where it has no
 * destructor */
void present_draw(struct task *task, void (*draw)(struct task *task));
/* task_destroy's call, before the next task made can take task's slot. the
 * split frame's records then name task no more. no picture replays its draws
 * where it has a destructor, which may release what they take. else the
 * pictures still replay its 2D and njDrawSprite3D's sprites */
void present_forget(struct task *task);
/* the owned 3D draws' call before their work, begin TRUE, and after it,
 * begin FALSE, and a game source's around a draw it adds to step 1. on step
 * 1 of a split frame, whose draws shipped skips, timer 1 stops from the
 * outermost begin to its end, so that SEGA's extra step times the step as at
 * 30 fps */
void present_untimed(int begin);
/* the shadow functions' call for each shadow they place. their easy draws
 * are none of present_record's, so present.c draws each task's shadows once
 * a picture. a shadow draws on step 0. step 1 of a split frame draws none,
 * since a step may call the callback the pass calls, and keeps each, with
 * timer 1 stopped, for the task whose step placed it. the pass draws the
 * shadows its callbacks place, then those step 1 kept for each task whose
 * callback placed none and that wasn't deleted: a player's, whose step
 * places them, or those of a task the pass draws by what its step drew */
void present_shadow(const struct shadow_place *place);
/* the draws the renderer owns: draw_model's and draw_model_nested's models,
 * sprite_draw_3d's and njDrawSprite3D's sprites, and the five 2D draws 16:9
 * squeezes */
enum present_kind {
    PRESENT_MODEL,
    PRESENT_SPRITE,
    PRESENT_NJ_SPRITE,
    PRESENT_POLYGON_2D,
    PRESENT_TEXTURE,
    PRESENT_TEXTURE_MEMLIST,
    PRESENT_POLYGON,
    PRESENT_SPRITE_2D,
};
/* each owned draw's call as it begins, with its arguments: what, the model,
 * sprite, vertices or polygon; count, their count; a, a sprite's frame or a
 * texture; pri and attr. a picture to come may draw it again. present.c
 * copies the vertices, or the sprite with its frame. it takes the model, the
 * texture list and the palette by pointer, so they must stay valid through
 * the next frame's first picture */
void present_record(enum present_kind kind, const void *what, Int count, Int a,
                    Float pri, Uint32 attr);
/* the shading draw_model takes: the light's direction in view space, its
 * copy, and the palette */
struct draw_shading {
    NJS_VECTOR dir;
    NJS_VECTOR base;
    const Uint32 *palette;
};
void draw_get_shading(struct draw_shading *shading);
void draw_put_shading(const struct draw_shading *shading);
/* where the easy draws keep their light's direction, in view space, which
 * njSetEasyLight sets */
NJS_VECTOR *port_easy_light(void);

/* TRUE from when the skip's curtain hides the scene until control comes
 * back, while the rest of the event runs with each wait one step long */
extern Bool event_skipping;
/* the steps past one that this step's collapsed wait stands for, which each
 * mover owes. skip_frame zeroes it */
extern int skip_owed;
/* frame_end's call before the overlay advances. a fresh press of Start on the
 * first pad, while an event holds control and no demo plays, lowers the
 * curtain. once a frame has rendered it whole, event_skipping turns TRUE, and
 * nothing renders, the frames wait for no vsync and the streams stop. all ends
 * as control comes back */
void skip_frame(void);
/* event_wait's call at each step of its wait while event_skipping is TRUE,
 * with the steps left. each runner stands at its target, and each mover owes
 * the rest. it returns 1, which ends the wait after this step */
int skip_wait(int left);

#endif
