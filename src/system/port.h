/* the non-matching code's calls and state that the game reaches. ninja.h
 * and frame.h include it under NONMATCHING. it includes
 * engine/dc/dreamcast.h, which declares the Dreamcast engine's part.
 * chord.h declares what passes between port.c and a machine's overlay */
#ifndef SYSTEM_PORT_H
#define SYSTEM_PORT_H
#ifdef NONMATCHING
#include <sg_xpt.h>
/* njtypes.h uses sg_xpt.h's types, so it comes after sg_xpt.h */
#include <njtypes.h>

#include "../../engine/dc/dreamcast.h"

struct motion;
struct shadow_place;
struct task;

/* put name, a function a change adds, in a section of its own. tools/ld.py
 * places it after the image's other code, ahead of only what a kernel.txt
 * cold row names. it moves no code ahead of it, but moves with any that
 * changes size outside the kernels' windows. GCC's clones of the function
 * go there too */
#define LINKED_LAST(name) __attribute__((section(".text_last." #name)))
/* put name, a datum a change adds, in a section of its own. tools/ld.py
 * places it after the image's other data, ahead of only what a kernel.txt
 * cold row names. it moves no datum ahead of it. ld.py keeps it off the
 * operand cache's sets of a kernel's literal pool that points into it. the
 * pools of the kernels that kernel runs with stay off its sets too. ld.py
 * moves the last data on by whole lines to do so */
#define LINKED_LAST_DATA(name) __attribute__((section(".data_last." #name)))

/* the port's toggles, each off at boot. chord.h's port_chord flips them */
enum port_toggle { TOGGLE_READOUT, TOGGLE_WIDE, TOGGLE_FAST, TOGGLES };
/* TRUE where a toggle is on: the timing readout shows, or 16:9 or 60 fps
 * is asked for. frame_end asks 60 only where the game's frames may
 * split, each showing two pictures */
extern int port_toggles[TOGGLES];
/* fade a black curtain over the game in where on is TRUE, and out where it
 * is FALSE. while draw_off is TRUE it falls at once. the readout and the
 * note that names a toggle as it flips show over it */
void port_curtain(int on);
/* TRUE where port_curtain_advance last found the curtain faded all the way
 * in, before it moved the fade on */
int port_curtain_shown(void);
/* fade the curtain a frame further. frame_end calls it once a frame. while
 * draw_off is TRUE it does nothing */
void port_curtain_advance(void);
/* TRUE where the frames to come want the split: the game asked 60 and the
 * engine applied it. present.c's pacing gives or holds the split by frame */
extern Int port_split_wanted;

/* 16:9's window and the 2D draws' pivot, the game's part of the rule
 * engine/dc/dreamcast.h gives for 16:9's aspects. sdk/ninja/njGlobal.c
 * defines the pivot and the cover.
 *
 * port_view_aspect is the window: how wide the game's tests of what is on
 * screen take it to be. it is the only 16:9 the rest of the game's logic
 * reads. five places read it: draw_in_view_any_step, enemy_on_screen,
 * draw_on_screen, npc_watch and the Sky Chase reticle's bounds. each answers
 * as SEGA's code would at that aspect, up to rounding. in play the window is
 * the picture's aspect, so objects in the wide sides act as in view.
 *
 * port_wide_anchor is the pivot the 2D draws squeeze x about. it is in
 * pixels from the screen's middle, where x is 320, and WIDE_MIDDLE at boot.
 * WIDE_LEFT and WIDE_RIGHT are the screen's left and right edges. a piece
 * that flies between pivots takes a pivot that travels with it, as
 * Knuckles' taken piece does. a layer that covers the screen, as a fade
 * does, keeps x, so that it stretches across it. one such layer is an
 * untextured polygon whose corners port_wide_covers finds at both edges.
 * any 2D draw while port_wide_cover is TRUE is another.
 *
 * a function that sets the anchor or the cover puts back WIDE_MIDDLE and
 * FALSE before it returns. present_run and present_draw start and end each
 * task's step and callback at WIDE_MIDDLE and FALSE. a child's step, which
 * its parent's step runs, starts and ends at what the parent set. so what
 * one task leaves set reaches no other's draws */
#define WIDE_MIDDLE 0.0F
#define WIDE_LEFT (-320.0F)
#define WIDE_RIGHT 320.0F
extern Float port_wide_anchor;
extern Int port_wide_cover;
/* the window's aspect, 1 or 3/4, which frame_end sets between frames */
extern Float port_view_aspect;
/* x on the screen, as Ninja projects a point, moved to where the window
 * takes it. it scales x's offset from _nj_screen_.cx by port_view_aspect,
 * and returns x itself at an aspect of 1 */
Float port_view_x(Float x);

/* frame_wait's call once the frame's wait ends. wait is when that wait
 * began, in syTmrGetCount's counts, and late the vsyncs the frame ran late.
 * it measures the frame, runs skip_frame and draws the overlay over this
 * frame. it readies the next frame's split and 16:9 */
void frame_end(Uint32 wait, int late);
/* the split frame, which 60 fps takes. a frame of play runs its steps as
 * SEGA's 30 fps runs them, and shows two pictures. the first is what step 0
 * drew, shown at the frame's middle vsync. the second follows the last step
 * and draws as SEGA's pause draws. a picture must change nothing a step
 * reads.
 *
 * frame_redrawing is TRUE while the port draws a picture the shipped frame
 * doesn't: the second picture's pass. to keep that rule, the pass keeps
 * rand()'s state and puts back each task's work after its callback. it puts
 * back a player's motion, record and shapes too. while it is TRUE, SEGA's
 * entries that register a collision or make a particle do nothing. so do
 * those that play a sound, clear a touch or end a placement */
extern int frame_redrawing;
/* TRUE from a frame's first picture until the next frame's step 0 ends. a
 * next frame that runs no stage loop ends it at its own end. while it is
 * TRUE, the act's hook and the sound server run at each vsync */
extern int frame_split;
/* TRUE where frame_split is and step 1's models and sprite_draw_3d's
 * sprites draw for the second picture. it is FALSE where that picture may
 * take a camera pose between the first's and camera_work's. step 1's draws
 * take step 1's view, so they draw nothing then. the pass draws each task
 * step 0 drew for in their place, but for the sprites present.c refuses */
extern int frame_later_draws;
/* TRUE on a step whose draws reach a picture */
#define FRAME_DRAWS()                                                          \
    (frame_step == 0 || (frame_later_draws && frame_step == 1))
/* the stage loops' call after each step. after step 0 of a split frame it
 * shows the first picture at the frame's middle vsync. that picture replays
 * the last frame's 2D and sprites from after its step 0 */
void present_step(void);
/* the stage loops' call after their last step. it draws the second picture
 * where the first showed. it drops that picture where the pass or a record
 * doesn't fit. it drops it too where the camera moved over step 0 alone */
void present_second(void);
/* what a frame ending showed. each HELD_ is why a frame that wanted the
 * split showed no second picture. a frame held for more than one reason
 * takes the first of HELD_LATE to HELD_SPARE that holds */
enum present_shown {
    /* port_split_wanted is FALSE, or no stage loop ran */
    SHOWN_UNWANTED,
    SHOWN_BOTH, /* both pictures, on time or late */
    /* none below held it: the split is engaging, or draw_off or the frame's
     * pacing held it */
    HELD_NONE,
    HELD_LATE, /* the frame ran late */
    HELD_STOPPED, /* the pass stopped */
    HELD_DROPPED, /* present_second dropped the picture for another reason */
    HELD_FULL, /* a store ran out, in this frame or as the split ended */
    HELD_FIRST, /* step 0 ran past its half */
    HELD_PVR, /* the PVR couldn't take the first picture on time */
    HELD_SECOND, /* step 1 and the pass wouldn't fit the frame's second half */
    HELD_BITMAP, /* a bitmap waited for its upload */
    /* frames fit without the room to spare that engages the split. the split
     * stays off while they do */
    HELD_SPARE,
    SHOWNS
};
/* what the frame ending showed, before present_end. late is as present_end
 * takes it */
enum present_shown present_shown(int late);
/* the stores a split frame keeps its records in */
enum present_store {
    STORE_DRAWS, /* step 0's draws */
    STORE_SPRITES, /* the later steps' njDrawSprite3D sprites */
    STORE_FLATS, /* both halves' 2D */
    STORE_MARKS, /* the tasks the second picture marks */
    STORE_SHAPES, /* a player's shapes */
    STORE_SHADOWS, /* step 1's shadows */
    STORES
};
/* the frame ending's figures for the readout. its times are in
 * syTmrGetCount's counts */
struct present_figures {
    Uint32 step0; /* step 0's time, as present_step measures it */
    /* when the frame's last step past step 0 ended, as present_step found
     * it. a frame of one step leaves it as an earlier frame set it */
    Uint32 later_end;
    Uint32 pass; /* the second picture's pass's time, 0 where none ran */
    Uint32 ta; /* the last transfer's length, as step 0's end found it */
    /* the store that ran out as the split last ended, or STORES where none
     * holds the split off. a full store holds it off until a frame that
     * doesn't want it or runs no stage loop */
    int out;
    /* the percent of each store's room this frame took, 100 where it ran
     * out */
    Uint32 full[STORES];
};
/* fill figures for the frame ending, before present_end. it returns FALSE,
 * and fills nothing, where no stage loop ran */
int present_figures(struct present_figures *figures);
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
 * next frame. that frame may split where port_split_wanted is TRUE, a stage
 * loop ran and the split is engaged. three frames in a row that fit with a
 * fourteenth of a vsync to spare engage it. a frame that misses ends it
 * where another missed within the split's last 30 frames */
void present_end(int late);
/* task.c runs task's step through it. a game source also passes it a draw
 * it makes for task in another task's step. each run keys the draws it makes
 * to task. on step 0 of a frame keeping records, it marks task where the
 * step sent its own draw. it times the step too, less the runs of
 * present_run inside it. present_draw takes the mark as task's step drawing
 * on step 0. it does nothing in the second picture's pass */
void present_run(struct task *task, void (*step)(struct task *task));
/* task.c's draw of task by draw, which may be null. the second picture's
 * pass leaves a task that drew on step 1 to those draws. where its step drew
 * on step 0, the pass calls its callback. where it has none, or its callback
 * is its step, the pass replays those draws instead. it calls no killed
 * task's callback. it replays a killed task's 2D and njDrawSprite3D's
 * sprites only where the task has no destructor */
void present_draw(struct task *task, void (*draw)(struct task *task));
/* TRUE while the second picture's pass calls task's own callback. FALSE
 * outside the pass, and while another task's callback draws task, as
 * Twinkle Park's mirrors do */
int present_drawing(const struct task *task);
/* task_destroy's call, before the next task made can take task's slot. the
 * split frame's records then name task no more. no picture replays its draws
 * where it has a destructor, which may release what they take. else the
 * pictures still replay its 2D and njDrawSprite3D's sprites */
void present_forget(struct task *task);
/* the owned 3D draws' call before their work, begin TRUE, and after it,
 * begin FALSE. a game source calls it around a draw it adds to step 1 too.
 * the shipped game skips step 1's draws. on step 1 of a split frame, timer 1
 * stops from the outermost begin to its end. so timer 1 times the step for
 * SEGA's extra step as at 30 fps */
void present_untimed(int begin);
/* the shadow functions' call for each shadow they place. they draw by
 * Ninja's easy draws, which present_record never sees. so present.c draws
 * each task's shadows once a picture. a shadow draws on step 0. step 1 of a
 * split frame draws none, since a step may call the callback the pass
 * calls. present_shadow keeps each, with timer 1 stopped, for the task whose
 * step placed it. the pass draws the shadows its callbacks place. it then
 * draws those step 1 kept for each task whose callback placed none and that
 * wasn't deleted. a player is such a task, as is one the pass draws by what
 * its step drew */
void present_shadow(const struct shadow_place *place);
/* the draws the renderer owns, in this order: draw_model's and
 * draw_model_nested's models, then sprite_draw_3d's and njDrawSprite3D's
 * sprites. the five 2D draws 16:9 squeezes follow, then text.c's prints,
 * which it doesn't. present.c takes each kind from PRESENT_POLYGON_2D on as
 * 2D. it takes each before PRESENT_NJ_SPRITE as a model or sprite_draw_3d's
 * sprite */
enum present_kind {
    PRESENT_MODEL,
    PRESENT_SPRITE,
    PRESENT_NJ_SPRITE,
    PRESENT_POLYGON_2D,
    PRESENT_TEXTURE,
    PRESENT_TEXTURE_MEMLIST,
    PRESENT_POLYGON,
    PRESENT_SPRITE_2D,
    PRESENT_TEXT,
};
/* each owned draw's call as it begins, or a print's as it ends. what is the
 * model, sprite, vertices, polygon or the print's cells. count is their
 * count. a is a sprite's frame, a texture or a cell's side. pri and attr are
 * the draw's, or a print's 1/w and color. a picture to come may draw it
 * again. present.c copies the vertices or cells, or the sprite with its
 * frame. it keeps the model, the texture list and the palette by pointer.
 * the caller keeps them valid through the next frame's first picture. the
 * caller also keeps the model's bytes as they were, but for a model
 * present_welded names. it returns TRUE where the draw is to draw, and else
 * FALSE. it returns FALSE only in the second picture's pass. a 2D draw or
 * njDrawSprite3D's sprite draws nothing there where its task drew that kind
 * after step 0 */
Int present_record(enum present_kind kind, const void *what, Int count, Int a,
                   Float pri, Uint32 attr);
/* call it after motion_weld where the task's step draws the welded models
 * and the task has no callback. it names the models motion's welds write,
 * which a later step writes again. on step 0, a kept draw of a named model
 * copies the model's points and normals. the second picture replays that
 * copy */
void present_welded(const struct motion *motion);
/* njDrawSprite2D's and njDrawSprite3D's present_record. in a picture's
 * replay, and until the first frame ends, it keeps nothing and returns TRUE.
 * else it returns present_record's answer. LOADER.BIN, which never runs
 * beside present.c, links Ninja's sprites without present.c. so LOADER.BIN
 * defines a present_sprite of its own, which keeps nothing */
Int present_sprite(enum present_kind kind, const void *what, Int count, Int a,
                   Float pri, Uint32 attr);
/* on a host, the count of the draws its adapter has taken. it leaves out
 * each model the game's own tests left out of view. a host keeps no display
 * lists, so present.c reads this where the Dreamcast reads the lists'
 * growth. port_sprites_sent counts njDrawSprite3D's sprites among them. it
 * runs on and wraps, so a count is a difference of two reads */
extern Uint32 port_draws_sent;

/* the shading the model draws take */
struct draw_shading {
    NJS_VECTOR dir; /* the light's direction in view space */
    NJS_VECTOR base; /* the light as draw_set_light gave it */
    const Uint32 *palette;
};
void draw_get_shading(struct draw_shading *shading);
void draw_put_shading(const struct draw_shading *shading);

/* TRUE from when the skip's curtain hides the scene until control comes
 * back. while it is TRUE, the rest of the event runs with each wait one step
 * long */
extern Bool event_skipping;
/* the steps the wait collapsed on this step stood for, less the one that
 * ran. each of events/shared/move.c's movers owes them. skip_frame zeroes
 * it */
extern int skip_owed;
/* frame_end's call before the curtain advances. a fresh press of Start on the
 * first pad lowers the curtain. it does so only while an event holds control
 * and no demo plays. once a frame has rendered it whole, event_skipping
 * turns TRUE. nothing then renders, the frames wait for no vsync, and the
 * music and voice streams stop. the skip ends, and the curtain lifts, as
 * control comes back */
void skip_frame(void);
/* event_wait's call at each step of its wait while event_skipping is TRUE.
 * left is the wait's steps left. each player an event runs to a target
 * stands at it. each mover owes the rest of the steps. it returns 1 as the
 * steps left, which ends the wait after this step */
int skip_wait(int left);

#endif
#endif
