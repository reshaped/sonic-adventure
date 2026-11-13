/* the Dreamcast engine's calls and state that the game and the SDK reach.
 * src/system/port.h includes it */
#ifndef ENGINE_DC_DREAMCAST_H
#define ENGINE_DC_DREAMCAST_H
#include <sg_xpt.h>
/* njtypes.h uses sg_xpt.h's types, so it comes after sg_xpt.h */
#include <njtypes.h>

/* syTmrGetCount's counts in a second, at 1.28 microseconds a count, and in
 * a vsync, at 59.94 Hz */
#define COUNTS_PER_SECOND 781250
#define VSYNC_COUNTS 13034
/* add the frame to the readout's counter and hitch counts, and to its page
 * where one shows. the readout is the port's on-screen timing display. a
 * frame at frame_interval 0, as in a skip, adds to none. age the note that
 * names a flipped toggle by the frame's vsyncs, frame_interval plus late.
 * the times are in syTmrGetCount's counts. wait is when the frame's wait
 * began. busy is the time from the end of the last frame's wait up to wait.
 * a split frame, which shows two pictures, leaves its wait for the middle
 * vsync out of busy */
void overlay_end(Uint32 wait, Uint32 busy, int late);
/* frame_wait's and frame_wait_step's call after each serve of the pads. it
 * gives the first pad to port_chord. the readout times steps 0 and 1 from
 * here, where their waits end */
void overlay_pad(void);
/* draw the curtain at port_curtain_level's level, then the readout and the
 * note, over the picture. a split frame draws them over each of its
 * pictures. while draw_off is TRUE it does nothing */
void overlay_draw(void);
/* the marks present.c gives the readout's cache page. they are step 0's end,
 * a later step's end, and the second picture's pass's start and end */
enum overlay_mark {
    MARK_STEP0_END,
    MARK_LATER_END,
    MARK_PASS_START,
    MARK_PASS_END
};
/* read the SH-4's counters at mark while the cache page shows, and else do
 * nothing */
void overlay_mark(enum overlay_mark mark);

/* 16:9. Ninja's aspect, _nj_scr_aspect_x_, stays the 1 njInitGlobal gives
 * it. so each projection the game makes lands in the 4:3 picture. 16:9 sets
 * two aspects of the port's in its place, each 1 or 3/4. the picture's is
 * port_draw_aspect, declared here. the window's is port_view_aspect, which
 * src/system/port.h declares with the 2D draws' pivot. a rule says who may
 * read each, and tests/test_wide.py holds its lists.
 *
 * at 3/4 the screen's 640 pixels hold a view a third wider. a 16:9 screen
 * stretches it back to its shape. Ninja's renderers, the engine's drawers
 * and the land list's clip read port_draw_aspect. the 2D draws' squeeze
 * reads it, and the movie takes the squeeze too. so does an event's picture
 * that fills the screen. that is a backdrop, or a still whose color adds to
 * the scene's. the squeeze scales x by it, so that a 2D draw keeps its shape
 * on that screen. the 2D draws are njDrawSprite2D, njDrawTexture,
 * njDrawTextureMemList, njDrawPolygon and njDrawPolygon2D. none of the
 * game's logic reads it.
 *
 * pillarbox is 16:9 while an attract demo or a recording runs. the game
 * then holds the window at 1, so that it plays as the 4:3 game. the picture
 * stays 3/4, and black bands cover its sides. each 2D draw squeezes about
 * the middle, whatever port_wide_anchor and port_wide_cover say */

/* both aspects' value under 16:9 */
#define WIDE_ASPECT 0.75F
/* x as the 2D draws squeeze it: about the pivot, or the middle under
 * pillarbox */
Float port_wide_x(Float x);
/* TRUE where the 2D draws may squeeze: under 16:9 while port.h's
 * port_wide_cover is FALSE, and under pillarbox. outside pillarbox, an
 * untextured polygon that port_wide_covers finds keeps x even so */
Int port_wide_squeezes(void);
/* TRUE outside pillarbox where count corners span the screen's width, so
 * that an untextured polygon keeps x. each corner's x lies within a pixel of
 * 0 or 640, and corners lie at both. the first corner's x is at x, and each
 * next one stride floats on */
Int port_wide_covers(const Float *x, Int count, Int stride);
/* the picture's aspect, 1 or 3/4 */
extern Float port_draw_aspect;
/* TRUE under pillarbox, until wide_end */
extern Int port_pillarbox;
/* the picture's 16:9: off, on, and pillarbox, where the game's window is 1 */
enum wide_mode { WIDE_OFF, WIDE_ON, WIDE_PILLARBOX };
/* the game asks a mode once a frame, by sa_present_window and
 * sa_present_mode. the engine applies it from the next frame. the picture's
 * aspect and the window make the 16:9 mode. a window wider than the picture
 * is refused. the game asks 60 only where its frames may split. the engine
 * grants it only where the 16:9 mode asked is the one applied. so a frame
 * after which 16:9 changes doesn't split the next. that frame's first
 * picture would replay this frame's 2D, pillarbox's bands among it, under
 * the other mode. port.h's port_split_wanted is TRUE where it grants 60.
 *
 * apply the 16:9 mode the game last asked to the picture, setting the
 * picture's aspect and pillarbox. frame_end calls it between frames, just
 * after it sets the window. so each frame draws at one aspect and plays in
 * one window */
void wide_apply(void);
/* end the frame that wide_apply's mode drew. under pillarbox it draws the
 * bands and ends pillarbox. the overlay drawn after then takes its own
 * cover */
void wide_end(void);
/* draw an untextured quad by njDrawPolygon2D from (x0, y0) to (x1, y1). it
 * draws under the blend, the anchor and the cover its caller set */
void port_band(Float x0, Float y0, Float x1, Float y1, Uint32 color, Float pri,
               Uint32 attr);
/* the mode wide_apply last applied */
extern enum wide_mode wide_applied;

/* the bytes njDrawSprite3D has sent, which present.c sets against the lists'
 * growth over a task's step. it runs on and wraps, so a count is a
 * difference of two reads */
extern Uint32 port_sprites_sent;

/* where the easy draws keep their light's direction, in view space, which
 * njSetEasyLight sets */
NJS_VECTOR *port_easy_light(void);

#endif
