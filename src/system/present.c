/* the port's split frame, which the 60 fps toggle takes: a frame of play that
 * shows two pictures. its steps run as SEGA's 30 fps runs them. the first
 * picture is what step 0 drew, shown at the frame's middle vsync. the second,
 * after the last step, shows what step 1 drew. its pass then draws each other
 * task whose step drew on step 0, as SEGA's pause does. the pass calls the
 * task's callback. where the task has none or its callback is its step, the
 * pass replays its step 0 draws. the replay moves them to the second picture's
 * view. a task whose step drew only njDrawSprite3D sprites, on step 0 and
 * after it, takes no callback. its steps' sprites stand for all it would draw.
 *
 * a task's step drew on step 0 where it made one of present_record's draws or
 * placed a shadow. it drew too where it grew the display lists past what its
 * nested present_run steps sent. Ninja's own draws and the land's grow them
 * so. step 1 runs at frame_step 1, where most draws return, so step 0 stands
 * for it. a task that starts to draw on step 1, as one made there does, shows
 * a picture late. a task that stops drawing there shows a picture longer.
 * either holds unless the task's callback tests the change.
 *
 * each picture also replays the 2D the other half of the frame drew, text.c's
 * prints among it. it replays too the sprites njDrawSprite3D drew there, moved
 * to the picture's view. 2D and njDrawSprite3D draw on every step in the
 * shipped game. each picture so shows what the shipped one shows. the later
 * half runs to the frame's end. the next first picture so replays too the 2D
 * the main loop draws past the stage loop. the memory card's NOW SAVING is
 * such 2D. in the pass, a task's callback draws no 2D where its step drew 2D
 * after step 0. it draws no njDrawSprite3D sprite where its step drew one
 * after step 0. what its steps drew stands for them. no picture replays a draw
 * whose task has a destructor and was killed or deleted. the destructor may
 * release what the draw takes. a kept model that step 0's welds wrote replays
 * from a copy of it, its points and normals. a later step writes the model
 * again.
 *
 * a picture must change nothing a step reads. port.h's frame_redrawing says
 * how the pass keeps that rule. a callback that writes anything else a step
 * reads still breaks it.
 *
 * the second picture takes the view the last step left, except where the
 * camera moves once a frame. a path camera poses itself on step 0 alone. the
 * main camera takes that pose on each step. camera_work so moves over the
 * later steps alone, and each pose would show in two pictures. a frame's
 * camera can stand over step 0 after the last frame's moved so. step 1's
 * models and sprite_draw_3d sprites then draw nothing, as port.h's FRAME_DRAWS
 * says. they would take step 1's view. the pass draws each task step 0 drew
 * for instead, but for the sprites present_record refuses. where the camera
 * then moves, the second picture takes the pose halfway between the first
 * picture's and camera_work's. a cut takes no halfway pose. step 1's other
 * draws, njDrawSprite3D's among them, keep step 1's view. a camera that moves
 * over step 0 alone showed its pose in the first picture. a pose past it would
 * be a guess, so the first picture holds.
 *
 * the shadow functions draw by Ninja's easy draws, which present_record never
 * sees. the second picture draws the shadows the pass's callbacks place. for
 * each other task it draws those its step placed on step 1, as port.h's
 * present_shadow says.
 *
 * for 16:9, present_run and present_draw put back the 2D draws' anchor and
 * cover around each step and callback.
 *
 * three rules pace it. the first picture shows where all of these hold. the
 * middle vsync is still ahead, with the first picture's room. the PVR can take
 * the picture on time. no bitmap waits for its upload. step 1 and the pass fit
 * the frame's second half, or the last frame's first picture held. step 0's
 * records and the last frame's later 2D and sprites all fit.
 *
 * the second picture shows where its records all fit and its pass ends in
 * time. the pass ends with its tail: step 1's shadows, step 0's replayed
 * draws, the particles and the 2D. its time leaves room for the TA, the PVR's
 * tile accelerator, to take in the lists it leaves. the pass takes each
 * callback to need what its task's step took on step 0. it stops before one
 * that would end late, and the first picture then holds.
 *
 * a run of frames that fit with room to spare engages the split. a frame
 * misses where it runs late, or where its pacing holds it to one picture for
 * time. a miss ends the split where another came within the split's last
 * SETTLE_FRAMES frames. the run's last frame must leave room too for the first
 * picture's replay of what it kept. so a scene that can't hold 60 fps settles
 * at 30, rather than switch by turns. a scene whose frames fit with about that
 * room regains 60 fps a few frames at a time. a held frame measures no shares.
 * after a frame fails the second half's test, the same shares test the next.
 * the split so rides out a lone late frame or miss of the first half. it rides
 * out a miss of the second half only where the next step 0 is shorter.
 *
 * a frame fits only where its own records and those the split last kept all
 * fit. only a frame that may split, or whose fit would engage the split, keeps
 * records. a scene whose records overflow in the split so holds it off, for
 * REFIT_FRAMES at first and twice as long at each overflow after, up to
 * REFIT_MOST. a frame that doesn't want the split or runs no stage loop ends
 * the hold sooner. a stage's load or pause runs none. while the hold lasts, no
 * frame keeps records.
 *
 * it carries four costs between frames. three are step 1's and the pass's
 * shares of step 0's time, and the pass's tail. only a split frame measures
 * them. while none does, they age back toward what a stage starts from, as
 * step1_share and tail_cost say. a frame that doesn't want the split or runs
 * no stage loop sets them back. the fourth is the TA's intake of the picture a
 * frame shows at its end. the next frame's step 0 measures it.
 *
 * timer 1 times the steps for SEGA's extra step. it stops across the port's
 * own work: the first picture, and step 1's draws, which shipped skips. it
 * stops too while a frame keeps a step's draw or forgets a deleted task. it
 * stops across the reads that time a planned step 0's steps. it runs on across
 * the calls into that work. it runs on across present_record's test for a draw
 * with nothing to keep. 30 fps makes neither. it runs on, too, across a split
 * frame's step 1. that step starts once the TA has the first picture's lists.
 * so it spans the middle vsync's interrupt, as 30 fps's step 1 does. it
 * follows that picture's work, which has taken lines of the caches. a planned
 * frame whose steps end near SEGA's limit can so take an extra step 30 fps
 * doesn't.
 *
 * the sound server runs at each picture's vsync. step 0 sends its sounds as
 * step 1 starts. in a split frame that is ahead of the middle vsync. the
 * sounds so start at the same vsync as at 30 fps. make dc links this into
 * 1ST_READ.BIN after the image's sources.
 *
 * a host links it too. there syTmrGetCount's clock moves only at a vsync,
 * Kamui's statistics read zero and no TA takes lists. each frame so fits, and
 * the split engages as on a console whose frames all fit. a host keeps no
 * display lists, so its adapter counts the draws it takes in their place */
#ifdef NONMATCHING
#include <kamui.h>
#include <kmistat.h>
#include <ninja.h>
#include <sg_xpt.h>
#ifdef DREAMCAST
#include <sh4zam/shz_mem.h>
#include <sh4zam/shz_xmtrx.h>
#endif
#include <shinobi.h>
#include <string.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "draw/model.h"
#include "draw/motion.h"
#include "draw/sprite.h"
#include "draw/text.h"
#include "effect/particle.h"
#ifdef DREAMCAST
#include "engine/dc/owner.h"
#endif
#include "main.h"
#include "player/player.h"
#include "sa_engine.h"
#include "sdk/ninja/njkamui.h"
#include "system/bitmap.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/timer.h"
#include "system/util.h"

/* rand()'s state */
extern unsigned long _rnext;
/* the bitmaps that wait for their upload, which util.c's non-matching side
 * gives a name outside util.c */
extern int texture_queue_count;
/* Ninja's current view, its matrix stack's base and the stack's depth, which
 * njPopMatrix trusts */
extern NJS_VIEW *_nj_view_;
extern NJS_MATRIX *_nj_base_matrix_ptr_;
extern Sint32 _stack_size_;

/* camera_action_state where no camera action gives the view, camera.c's
 * ACTION_NONE */
#define ACTION_NONE 0
/* a move over a frame's later steps past either of these is a cut. the second
 * picture's pose doesn't span a cut. CUT_MOVE is along an axis: three steps at
 * player.c's limit of 16 a step. CUT_TURN is an eighth of a turn about one */
#define CUT_MOVE 48.0F
#define CUT_TURN 0x2000

/* _nj_current_texture where no texture is current, as Ninja starts it */
#define NO_CURRENT_TEXTURE 0xffffffff

/* timer 1's start register, and its bit that runs timer 1, as timer.c
 * names them */
#define TSTR (*(volatile Uint8 *)0xffd80004)
#define STR1 0x02

/* frame_countdown as step 0 ends with the middle vsync still ahead. a
 * console's flip waited out the frame's first vsync, and a host's waits for
 * none */
#ifdef DREAMCAST
#define MIDDLE_AHEAD 1
#else
#define MIDDLE_AHEAD 2
#endif

/* the room in syTmrGetCount's counts that the first picture takes past step 0,
 * before the middle vsync. FIRST_ROOM, 0.76 ms, covers the overlay, the submit
 * and the lag of the frame's start behind its vsync. the rest of it is margin.
 * each sprite replayed adds SPRITE_ROOM, and each 2D draw FLAT_ROOM. a text.c
 * print adds CELL_ROOM for each of its cells besides. a first picture past its
 * room can run its frame a vsync late */
#define FIRST_ROOM (VSYNC_COUNTS / 22)
#define SPRITE_ROOM 8
#define FLAT_ROOM 4
#define CELL_ROOM 2
/* the room the second half keeps past the pass and the TA's intake of its
 * lists: 0.83 ms. it covers the hud, the fades, the overlay and the submit. it
 * covers too what follows the frame's wait, with a margin. it covers the lag
 * of the frame's start behind its vsync, from which the middle vsync is
 * counted. it keeps no margin for an error in the shares, the tail or the
 * intake. a pass whose shares misjudge the second half stops at its time, and
 * the first picture holds. a tail that ends past the room drops its picture.
 * so does a pass whose lists' intake outgrows the room left for it */
#define SECOND_ROOM (VSYNC_COUNTS / 20)
/* tail_price's room for a pass's tail past its draws: 0.17 ms. it prices each
 * draw as the first picture's room does */
#define TAIL_ROOM (VSYNC_COUNTS / 100)
/* the margin pvr_fits adds to the last render's length, since the next render
 * can be longer */
#define RENDER_ROOM (VSYNC_COUNTS / 10)
/* a whole step 0 in the shares' unit, 256ths of it */
#define WHOLE_SHARE 256
/* the pass's and step 1's shares of step 0 that a stage starts from.
 * second_fits takes their sum, which starts at a whole step 0. where a stage's
 * sum passes a whole step 0, its first pass stops rather than run late */
#define PASS_SHARE 160
#define STEP1_SHARE (WHOLE_SHARE - PASS_SHARE)
/* the frames in a row that fit with room to spare, which engage the split, and
 * that room. a frame spares it where it would fit with step 0 longer by
 * ENGAGE_ROOM, 1.19 ms. the frame that would engage the split spares the first
 * picture's replay of what it kept besides. three such frames, 0.1 s at 30
 * fps, engage it. a smaller room would let the split engage and end by turns.
 * the room costs frames that would show both pictures. a frame that misses
 * ends the split where another missed within the split's last SETTLE_FRAMES
 * frames. that is a second at 30 fps. the shares and the tail age by the same
 * second, as step1_share and tail_cost say */
#define ENGAGE_FRAMES 3
#define ENGAGE_ROOM (VSYNC_COUNTS / 14)
#define SETTLE_FRAMES 30
/* the frames a split its records overflowed stays off: a second at 30 fps,
 * twice that at each overflow after, up to REFIT_MOST, 16 seconds. a split
 * that runs REFIT_MOST frames with no overflow waits REFIT_FRAMES again */
#define REFIT_FRAMES SETTLE_FRAMES
#define REFIT_MOST (16 * REFIT_FRAMES)

/* the operand cache's line, and the unit of a store's entries, which a kept
 * draw's indexes count */
#define HELD_LINE 32
#define HELD_UNIT 16
/* the most draws step 0 keeps for the second picture: a pinball table of
 * Casinopolis's keeps over 400 sprites a frame. the most sprites the later
 * steps keep for the next first picture. the most views a list's draws are
 * made under */
#define FIRST_MOST 512
#define LATER_MOST 256
#define VIEWS_MOST 4
/* a store's bytes for each draw it may keep. a model's draw takes 176 at most:
 * itself, a state, a matrix and its shading. a sprite's takes 208, with its
 * copy in place of the shading. a store of sprites with their own state and
 * matrix fills at 12/13 of its most draws */
#define HELD_BYTES 192
/* each half's 2D draws, and the words their vertices take */
#define FLATS_MOST 96
#define FLAT_WORDS 1536
/* the slots of the tasks the second picture marks: one for each value
 * marks_slot hashes a task to. they take MARKS_MOST tasks, three quarters of
 * the slots, so a probe always meets an empty one */
#define MARKS_BITS 10
#define MARKS_SLOTS (1 << MARKS_BITS)
#define MARKS_MOST (MARKS_SLOTS * 3 / 4)
/* the most shapes the pass keeps for a player */
#define SHAPES_KEPT 8
/* the most shadows step 1 keeps for the pass */
#define SHADOWS_MOST 128

/* a map of four parts, by the state below that holds each and what writes it.
 *
 * the records: what a step writes and a picture reads. present_end and
 * present_step alone set keeping. present_run and present_draw alone set
 * present_task. keep_record fills first_draws and later_draws by keep_draw,
 * and first_flats and later_flats by keep_flat. present_welded fills welded on
 * step 0, and present_shadow fills shadows on a split frame's step 1.
 * marks_add enters each task in marks. the pass reads first_draws,
 * first_flats, shadows and marks, and show_first later_draws and later_flats.
 *
 * the copies: what may change before a replay. keep_draw and keep_flat copy a
 * draw's state, and a sprite and its frame. keep_draw copies the draw's matrix
 * and the view's, and a model's shading. welded_copy copies a welded model.
 * keep_flat copies the vertices or the cells. other models, texture lists and
 * palettes stay the caller's, as port.h's present_record says. present_end
 * empties first_draws, first_flats, welded and marks. present_forget clears a
 * deleted task's marks and shadows. it keys the task's draws and 2D to
 * deleted_task or released_task. present_step empties later_draws, later_flats
 * and shadows as step 0 ends. it copies the camera's pose to first_pose, and
 * present_second to start_pose. camera_between alone writes between.
 *
 * what a picture puts back, and where. present_second alone sets
 * frame_redrawing, across the pass. show_first and present_second save Ninja's
 * draw state in saved by ninja_save, and put it back by ninja_restore.
 * draws_replay and draw_flats clear sprites_kept across a replay. records_keep
 * keeps a task's work before its callback, and a player's motion, record and
 * shapes. records_put puts them back after the callback. present_second keeps
 * _rnext, frame_step and the pose camera_between moves in its locals. it puts
 * them back after ninja_restore, before it clears frame_redrawing. the pass
 * marks HOLDS, DREW_FLAT, DREW_SPRITE and DREW_SHADOW for its tail alone.
 *
 * the pacing: what engages, holds or ends the split. present_step alone writes
 * step0_cost, last_ta and second_blocks. it alone sets frame_split and
 * frame_later_draws TRUE. of the four costs, it measures step1_share and
 * second_ta, and present_second pass_share and tail_cost. step_measured prices
 * each callback in marks. pass_stop tests the prices against pass_end, which
 * present_second alone sets. pass_stop sets stopped. present_second sets
 * dropped, which present_end hands on as holding. present_end alone writes
 * engaged, fit_run, calm, planned, records_fit and its wait. it alone sets the
 * shares back, and ages them by shares_age, which present_second restarts */

int frame_redrawing;
int frame_split;
int frame_later_draws;

/* what present_record keeps. KEEP_FIRST keeps the draws up to step 0's end,
 * and KEEP_LATER the 2D and njDrawSprite3D sprites after it */
static enum { KEEP_NONE, KEEP_FIRST, KEEP_LATER } keeping;
/* the task whose step present_run runs, or whose callback present_draw calls
 * in the pass, or null */
static struct task *present_task;

/* two of the costs carried between frames, each in 256ths of its frame's step
 * 0. step1_share is step 1's, from the first picture's flip, and pass_share
 * the pass's. only a split frame measures them, since step 1 draws only there.
 * a share scales with this frame's step 0, timed from the end of the last
 * frame's wait. the cost it gives so falls with step 0 as a heavy scene ends.
 * a frame that doesn't want the split or runs no stage loop sets each back to
 * its start. so each stage learns its own. a share past its start holds off
 * frames the start would split, and nothing measures it then. each
 * SETTLE_FRAMES after a pass last measured it, it so loses half its excess.
 * the shares that ended the split thus hold it off as it settles. a scene
 * whose second half grows lighter takes the split back within seconds. a share
 * under its start holds no frame off, and stays until the next split frame
 * measures it */
static Uint32 step1_share = STEP1_SHARE;
static Uint32 pass_share = PASS_SHARE;
static int shares_age;
/* this frame's step 0 in syTmrGetCount's counts, from the end of the last
 * frame's wait */
static Uint32 step0_cost;
/* the counts the last pass that ended took for its tail, and the frames since.
 * it stands for the tail for SETTLE_FRAMES. past that, tail_price stands for
 * it. so it does after a frame that doesn't want the split or runs no stage
 * loop */
static Uint32 tail_cost;
static int tail_age = SETTLE_FRAMES;
/* the counts the TA took for the last end picture, the picture a frame shows
 * at its end. the next frame's step 0 reads them as the last transfer's
 * length. a split frame's end picture is its second. another frame's is its
 * one picture, which shows what the second would. a second picture can take
 * the TA longer than its frame's first. while the pass runs, the last transfer
 * is the first picture's. the pass and second_fits so take the larger of this
 * and the last transfer. every end picture is measured, so no long intake
 * holds the split off past the next */
static Uint32 second_ta;
/* second_blocks is the 32-byte blocks of the lists the TA took in second_ta.
 * end_blocks is those of the end picture of the frame that ended last. it is 0
 * where that frame didn't want the split */
static Uint32 second_blocks;
static Uint32 end_blocks;
/* TRUE where the frame that ended showed an end picture. the next frame's step
 * 0 then measures second_ta */
static int end_shown;
/* what present_figures gives the readout, the port's on-screen timing display.
 * last_ta is the last transfer's length as step 0 ended. later_end is when the
 * last step after a step 0 ended. pass_cost is the counts the pass took, 0
 * where none ran */
static Uint32 last_ta;
static Uint32 later_end;
static Uint32 pass_cost;
/* fit_run counts the frames in a row that fit with ENGAGE_ROOM to spare while
 * the split isn't engaged. planned is TRUE where the frame running keeps the
 * records a split takes. present_end sets it where the toggle is on and the
 * split is engaged or one fit from engaging. a frame splits only after one
 * that kept its later 2D and sprites. the frame that engages the split so
 * keeps them, and the next can split. that frame's fit counts what keeping
 * them costs, which the frames before it don't pay */
static int engaged;
static int fit_run;
static int planned;
/* the frames of the split since one missed, up to SETTLE_FRAMES */
static int calm = SETTLE_FRAMES;
/* TRUE where the last frame that kept records while the split was engaged kept
 * them all. where they overflowed, the split ends, and records_fit stays FALSE
 * for refit_wait frames, so the split stays off that long. the next overflow
 * waits refit_next, and refit_calm counts the frames the split has run since
 * the last. the frame whose fit would engage the split keeps records too, but
 * leaves records_fit be. where they overflow, that frame doesn't fit, and
 * fit_run starts over. a frame that doesn't want the split or runs no stage
 * loop sets records_fit back to TRUE. while it is FALSE, held_store is the
 * store that ran out, which the readout names */
static int records_fit = TRUE;
static int held_store;
static int refit_wait;
static int refit_next = REFIT_FRAMES;
static int refit_calm;
/* stage_ran is TRUE where a stage loop ran step 0 this frame. first_fits is
 * TRUE where step 0 left room for the first picture. spare_fits is TRUE where
 * both halves fit with step 0 longer by ENGAGE_ROOM. later_kept is TRUE where
 * the last frame kept its later 2D and sprites */
static int stage_ran;
static int first_fits;
static int spare_fits;
static int later_kept;
/* render_fits is TRUE where step 0 found the PVR able to take the first
 * picture on time. second_short is TRUE where first_fits and render_fits are
 * and step 0 found the second half too short for step 1 and the pass.
 * bitmap_waits is TRUE where a bitmap waited for its upload as step 0 ended */
static int render_fits;
static int second_short;
static int bitmap_waits;
/* step1_start is when step 1 began, and middle the middle vsync as show_first
 * sets it. middle_wait is the counts the first picture's flip waited */
static Uint32 step1_start;
static Uint32 middle;
static Uint32 middle_wait;
/* pass_end is when the pass's callbacks must end, leaving room for the tail,
 * the TA's intake and SECOND_ROOM. stopped is TRUE where the pass stopped
 * before a callback that could end past pass_end. it is TRUE too where the
 * callbacks ended past it, and the pass stopped before its tail. stopped_at is
 * when that callback would end, taking what its task's step took on step 0.
 * else it is when the pass stopped before its tail */
static Uint32 pass_end;
static int stopped;
static Uint32 stopped_at;
/* TRUE where present_second dropped the second picture. present_end hands it
 * on as holding, to present_hold and to the next frame's step 0 */
static int dropped;
static int holding;
/* the store, STORE_MARKS, STORE_SHAPES or STORE_SHADOWS, that a record the
 * second picture needs didn't fit in. it is 0 where each fit. shapes_most is
 * the most shapes of a player's the pass kept or tried to */
static int lost;
static int shapes_most;
/* untimed_run is TSTR as the outermost present_untimed begin found it.
 * untimed_depth counts the begins yet to end */
static Uint8 untimed_run;
static int untimed_depth;
/* TRUE where present_sprite keeps njDrawSprite2D's and njDrawSprite3D's
 * sprites: from the first present_end on, except in a picture's replay */
static int sprites_kept LINKED_LAST_DATA(sprites_kept);

/* the camera's pose: camera_work, its angles and position, and the eye of the
 * view built from them. camera_add_offset's shakes move the eye */
struct pose {
    struct task_work *work;
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT3 eye;
};
/* how the camera went from one pose to the next. WENT_ELSE is another camera
 * or none. the others are the same camera, standing or moving */
enum went { WENT_ELSE, WENT_STOOD, WENT_MOVED };
/* the pose the last frame's steps left, and the pose step 0 left */
static struct pose start_pose;
static struct pose first_pose;
/* TRUE where the last frame's camera stood over step 0 and moved over the
 * later steps */
static int moved_later;
/* the second picture's view, from the pose between the pictures' */
static NJS_VIEW between;

/* the Ninja state a kept draw keeps, with the 2D draws' anchor, cover and
 * pillarbox */
struct ninja_state {
    /* the current context's polygon header words */
    Uint32 header[4];
    Uint32 control;
    Uint32 attr_and;
    Uint32 attr_or;
    NJS_ARGB material;
    NJS_TEXLIST *texlist;
    Uint32 texture;
    Uint32 bank;
    Float anchor;
    Sint16 cover;
    Sint16 pillarbox;
};

/* a kept draw's copy of its sprite, of the frame it draws and of the depth it
 * took. a later draw may change the frame in the texture animation. the copy's
 * sprite points at the copy's frame, so a replay draws it as kept */
struct kept_sprite {
    NJS_SPRITE sprite;
    NJS_TEXANIM frame;
    Float depth;
};

/* the tables of a list's store. a table gains an entry only where its words
 * differ from its last */
enum { TABLE_STATE, TABLE_MATRIX, TABLE_SHADING, TABLES };

#ifdef DREAMCAST
/* pack(8), as for NJS_VIEW. Ninja moves a matrix on an 8-byte boundary by
 * pairs of floats, in half the moves. the views start 16 bytes in, and pack(8)
 * aligns each list to 8 */
#pragma pack(push, 8)
#endif
/* a list of draws a picture to come may replay, each an sa_draw_kept. the
 * draws fill the store from its bottom. their entries fill it from the top
 * down: the tables', and the copies of sprites and welded models. top is the
 * store's bytes below the entries. full is TRUE where a draw didn't fit. last
 * is each table's last entry, or 0 for none. the tasks lie apart from the
 * draws, so that present_forget reads a line for each eight draws */
struct draws {
    Uint8 *store;
    struct task **task;
    int most;
    int count;
    NJS_MATRIX views[VIEWS_MOST];
    int view_count;
    int full;
    Uint32 top;
    Uint16 last[TABLES];
};
#ifdef DREAMCAST
#pragma pack(pop)
_Static_assert(__builtin_offsetof(struct draws, views) % 8 == 0 &&
                   _Alignof(struct draws) % 8 == 0,
               "a view's matrix lies off the 8-byte boundary");
#endif

/* a 2D draw kept for the other picture, with its task or null and its
 * arguments. a sprite keeps a copy of itself and its frame. words is where its
 * vertices start in flats' words */
struct flat {
    struct task *task;
    enum present_kind kind;
    Int count;
    Int a;
    Float pri;
    Uint32 attr;
    NJS_SPRITE sprite;
    NJS_TEXANIM frame;
    int words;
    struct ninja_state state;
};

/* a half's 2D draws, and their vertices. full is TRUE where a draw didn't
 * fit */
struct flats {
    struct flat flat[FLATS_MOST];
    int count;
    Uint32 words[FLAT_WORDS];
    int used;
    int full;
};

/* a task's marks for the second picture. present_end clears every task's marks
 * as the frame ends, and present_forget a deleted task's */
/* its step drew models or sprite_draw_3d sprites on step 1 */
#define DREW_LATER 0x1
/* the pass replays its kept draws */
#define HOLDS 0x2
/* it has kept draws */
#define HAS_HELD 0x4
/* its callback drew 2D in the pass */
#define DREW_FLAT 0x8
/* its step drew njDrawSprite3D sprites after step 0 */
#define SPRITES_LATER 0x10
/* its callback drew an njDrawSprite3D sprite in the pass */
#define DREW_SPRITE 0x20
/* its callback placed a shadow in the pass */
#define DREW_SHADOW 0x40
/* its step drew on step 0 */
#define DREW_FIRST 0x80
/* its step drew 2D after step 0 */
#define FLATS_LATER 0x100
/* its step sent more on step 0 than njDrawSprite3D sprites */
#define DREW_OTHER 0x200
/* above SPENT_SHIFT, a DREW_FIRST task keeps the counts its step took on step
 * 0. those exclude the steps it ran. the pass takes them as what its callback
 * needs */
#define SPENT_SHIFT 10
static struct {
    struct task *task;
    Uint32 marks;
} marks[MARKS_SLOTS];
static int marks_used;

/* first_held keeps step 0's draws for the second picture. they are its
 * njDrawSprite3D sprites, and the draws of a task callback_draws refuses.
 * later_held keeps the njDrawSprite3D sprites after step 0 for the next first
 * picture */
static Uint8 first_held[FIRST_MOST * HELD_BYTES]
    __attribute__((aligned(HELD_LINE)));
static Uint8 later_held[LATER_MOST * HELD_BYTES]
    __attribute__((aligned(HELD_LINE)));
static struct task *first_tasks[FIRST_MOST];
static struct task *later_tasks[LATER_MOST];
/* each by position: dc.mk's float pass, tools/floats.py, reads no
 * designator */
static struct draws first_draws = {
    first_held, first_tasks, FIRST_MOST, 0, {{0}}, 0, 0, sizeof(first_held)};
static struct draws later_draws = {
    later_held, later_tasks, LATER_MOST, 0, {{0}}, 0, 0, sizeof(later_held)};
/* the models step 0's welds wrote, as present_welded names them. they lie in
 * an entry of first_held taken for the first model, and welded is null until
 * then. a pointer is a word on the Dreamcast, so WELDED_MOST models and the
 * count fill HELD_BYTES. a kept draw of one keeps a copy of the model, its
 * points and normals in another entry. they take the store's room, since the
 * image's .bss lies under the module area */
#define WELDED_MOST ((int)(HELD_BYTES / sizeof(Uint32)) - 1)
struct welded {
    int count;
    const NJS_MODEL *model[WELDED_MOST];
};
static struct welded *welded;
/* step 0's 2D, for the second picture, and the 2D after step 0, for the next
 * frame's first */
static struct flats first_flats;
static struct flats later_flats;
/* a shadow step 1 of a split frame placed, and the task whose step placed
 * it, or null */
struct kept_shadow {
    struct shadow_place place;
    struct task *task;
};
/* the shadows step 1 placed, which the pass draws except for a task whose
 * callback drew one */
static struct kept_shadow shadows[SHADOWS_MOST];
static int shadows_count;

#ifdef DREAMCAST
/* pack(8), as for struct draws: it aligns saved and its two matrices to 8 */
#pragma pack(push, 8)
#endif
/* Ninja's draw state, which a picture the shipped frame doesn't draw saves and
 * puts back */
static struct {
    KMVERTEXCONTEXT contexts[NJD_CONTEXT_MAX];
    Uint32 color_modes[NJD_CONTEXT_MAX];
    KMVERTEXCONTEXT *context;
    Uint32 *color_mode;
    NJS_VIEW *view;
    NJS_MATRIX view_matrix;
    NJS_MATRIX matrix;
    NJS_MATRIX *matrix_ptr;
    Sint32 stack_size;
    struct ninja_state state;
    struct draw_shading shading;
    Float depth;
    NJS_VECTOR easy_light;
} saved;
#ifdef DREAMCAST
#pragma pack(pop)
_Static_assert(__builtin_offsetof(__typeof__(saved), view_matrix) % 8 == 0 &&
                   __builtin_offsetof(__typeof__(saved), matrix) % 8 == 0 &&
                   _Alignof(__typeof__(saved)) % 8 == 0,
               "a saved matrix lies off the 8-byte boundary");
#endif

/* copy words 32-bit words from from to to. -Os calls the runtime's memcpy for
 * a struct of 16 bytes or more, which is slower */
static inline __attribute__((always_inline)) void
words_copy(void *to, const void *from, int words)
{
    Uint32 *x = to;
    const Uint32 *y = from;
    int i;

#pragma GCC unroll 16
    for (i = 0; i < words; i++) {
        x[i] = y[i];
    }
}

static void state_get(struct ninja_state *state)
{
    const KMVERTEXCONTEXT *context = _nj_current_ctx_;

    state->header[0] = context->GLOBALPARAMBUFFER;
    state->header[1] = context->ISPPARAMBUFFER;
    state->header[2] = context->TSPPARAMBUFFER;
    state->header[3] = context->TexturePARAMBUFFER;
    state->control = _nj_control_3d_flag_;
    state->attr_and = _nj_constant_attr_and_;
    state->attr_or = _nj_constant_attr_or_;
    words_copy(&state->material, &_nj_constant_material_,
               sizeof(NJS_ARGB) / sizeof(Uint32));
    state->texlist = _nj_current_texlist;
    state->texture = _nj_current_texture;
    state->bank = _nj_current_texture_bank;
    state->anchor = port_wide_anchor;
    state->cover = port_wide_cover;
    state->pillarbox = port_pillarbox;
}

static void state_put(const struct ninja_state *state)
{
    KMVERTEXCONTEXT *context = _nj_current_ctx_;

    context->GLOBALPARAMBUFFER = state->header[0];
    context->ISPPARAMBUFFER = state->header[1];
    context->TSPPARAMBUFFER = state->header[2];
    context->TexturePARAMBUFFER = state->header[3];
    _nj_control_3d_flag_ = state->control;
    _nj_constant_attr_and_ = state->attr_and;
    _nj_constant_attr_or_ = state->attr_or;
    words_copy(&_nj_constant_material_, &state->material,
               sizeof(NJS_ARGB) / sizeof(Uint32));
    _nj_current_texlist = state->texlist;
    _nj_current_texture = state->texture;
    _nj_current_texture_bank = state->bank;
    port_wide_anchor = state->anchor;
    port_wide_cover = state->cover;
    port_pillarbox = state->pillarbox;
}

static void ninja_save(void)
{
    memcpy(saved.contexts, _nj_context_, sizeof(saved.contexts));
    memcpy(saved.color_modes, _nj_ctx_color_mode_, sizeof(saved.color_modes));
    saved.context = _nj_current_ctx_;
    saved.color_mode = _nj_color_mode_;
    saved.view = _nj_view_;
    memcpy(saved.view_matrix, _nj_view_matrix_, sizeof(NJS_MATRIX));
    njGetMatrix(&saved.matrix);
    saved.matrix_ptr = _nj_current_matrix_ptr_;
    saved.stack_size = _stack_size_;
    state_get(&saved.state);
    draw_get_shading(&saved.shading);
    saved.depth = sprite_get_depth();
    saved.easy_light = *port_easy_light();
}

static void ninja_restore(void)
{
    memcpy(_nj_context_, saved.contexts, sizeof(saved.contexts));
    memcpy(_nj_ctx_color_mode_, saved.color_modes, sizeof(saved.color_modes));
    _nj_current_ctx_ = saved.context;
    _nj_color_mode_ = saved.color_mode;
    _nj_view_ = saved.view;
    memcpy(_nj_view_matrix_, saved.view_matrix, sizeof(NJS_MATRIX));
    _nj_current_matrix_ptr_ = saved.matrix_ptr;
    _stack_size_ = saved.stack_size;
    njSetMatrix(NULL, &saved.matrix);
    state_put(&saved.state);
    draw_put_shading(&saved.shading);
    sprite_set_depth(saved.depth);
    *port_easy_light() = saved.easy_light;
}

/* stop timer 1, and return TSTR as it was, which the caller writes back. the
 * port's own work stops it, so SEGA's extra step times the steps as at 30
 * fps */
static Uint8 timer_stop(void)
{
    Uint8 run = TSTR;

    TSTR = run & ~STR1;
    return run;
}

/* task's first slot in marks: the top MARKS_BITS bits of a multiplicative
 * hash of its address */
static Uint32 marks_slot(const struct task *task)
{
    return ((Uint32)task >> 4) * 2654435761U >> (32 - MARKS_BITS);
}

/* task's marks, or null where it has none */
static Uint32 *marks_find(const struct task *task)
{
    Uint32 i = marks_slot(task);

    for (;; i = (i + 1) & (MARKS_SLOTS - 1)) {
        if (marks[i].task == task) {
            return &marks[i].marks;
        }
        if (marks[i].task == NULL) {
            return NULL;
        }
    }
}

/* mark task with mark, and return its marks. past MARKS_MOST tasks it marks
 * nothing, the second picture is lost, and it returns null */
static Uint32 *marks_add(struct task *task, Uint32 mark)
{
    Uint32 i = marks_slot(task);

    for (;; i = (i + 1) & (MARKS_SLOTS - 1)) {
        if (marks[i].task == task) {
            marks[i].marks |= mark;
            return &marks[i].marks;
        }
        if (marks[i].task == NULL) {
            break;
        }
    }
    if (marks_used >= MARKS_MOST) {
        lost = STORE_MARKS;
        return NULL;
    }
    marks_used++;
    marks[i].task = task;
    marks[i].marks = mark;
    return &marks[i].marks;
}

/* the last task keep_step marked DREW_FIRST, and its marks. first_task is null
 * outside step 0 of a frame that keeps records, and once its task is
 * deleted */
static struct task *first_task;
static Uint32 *first_mark;

/* mark task DREW_FIRST for a draw its step made on step 0, and make it
 * first_task. it stays out of line, so that keep_step keeps the present kernel
 * in its window in config/1st_read/kernel.txt */
static void marks_first(struct task *task) __attribute__((noinline));
static void marks_first(struct task *task)
{
    first_mark = marks_add(task, DREW_FIRST);
    first_task = task;
}

/* TRUE where matrices a and b have the same bits. it compares words, where
 * the runtime's memcmp compares bytes */
static int matrix_same(const Float *a, const Float *b)
{
    const Uint32 *x = (const Uint32 *)a;
    const Uint32 *y = (const Uint32 *)b;
    Uint32 i;

    for (i = 0; i < sizeof(NJS_MATRIX) / sizeof(Uint32); i++) {
        if (x[i] != y[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

/* the index in list's views of Ninja's view matrix, which it adds where new.
 * it returns -1 where the views are full */
static int view_index(struct draws *list)
{
    int i;

    for (i = list->view_count - 1; i >= 0; i--) {
        if (matrix_same(list->views[i], _nj_view_matrix_)) {
            return i;
        }
    }
    if (list->view_count >= VIEWS_MOST) {
        return -1;
    }
    memcpy(list->views[list->view_count], _nj_view_matrix_, sizeof(NJS_MATRIX));
    return list->view_count++;
}

/* list's store's entry at index */
#define HELD_ENTRY(list, index) ((void *)((list)->store + (index) * HELD_UNIT))

/* where the line of list's store that its next draw takes ends. a draw takes
 * a unit, and two fill a line */
static inline __attribute__((always_inline)) Uint32
draws_reach(const struct draws *list)
{
    return ((list->count + 2) & ~1) * HELD_UNIT;
}

/* empty list's store and views */
static void draws_clear(struct draws *list)
{
    list->count = list->view_count = list->full = 0;
    list->top = list->most * HELD_BYTES;
    list->last[TABLE_STATE] = list->last[TABLE_MATRIX] =
        list->last[TABLE_SHADING] = 0;
}

/* take bytes from the top of list's store, rounded to whole units. it returns
 * null where they would reach the line the next draw takes. a replay reads
 * only words a keep wrote there, so MOVCA.L may claim each whole line
 * unread */
static void *store_take(struct draws *list, Uint32 bytes)
{
    Uint32 reach = draws_reach(list);
    Uint32 size = (bytes + HELD_UNIT - 1) & ~(HELD_UNIT - 1);
    Uint32 line;

    if (list->top < reach + size) {
        return NULL;
    }
#ifdef DREAMCAST
    for (line = (list->top - size + HELD_LINE - 1) & ~(HELD_LINE - 1);
         line + HELD_LINE <= list->top; line += HELD_LINE) {
        shz_dcache_alloc_line(list->store + line);
    }
#endif
    list->top -= size;
    return list->store + list->top;
}

/* the index of an entry of list's table that holds words, which is bytes long.
 * it is the table's last entry where that holds them already, and else a new
 * one. it is 0 where no new one fits. it compares from the end where backward
 * is TRUE. two matrices that rotate alike differ first in their translations,
 * which lie at the end */
static inline __attribute__((always_inline)) Uint16
table_entry(struct draws *list, int table, const void *words, Uint32 bytes,
            int backward)
{
    const Uint32 *x = words;
    const Uint32 *y = HELD_ENTRY(list, list->last[table]);
    Uint32 n = bytes / sizeof(Uint32);
    void *entry;
    Uint32 i;

    if (list->last[table] != 0) {
        for (i = 0; i < n; i++) {
            if (x[backward ? n - 1 - i : i] != y[backward ? n - 1 - i : i]) {
                break;
            }
        }
        if (i == n) {
            return list->last[table];
        }
    }
    entry = store_take(list, bytes);
    if (entry == NULL) {
        return 0;
    }
    words_copy(entry, words, bytes / sizeof(Uint32));
    list->last[table] = list->top / HELD_UNIT;
    return list->last[table];
}

/* it returns model where step 0's welds didn't write it. else it returns a
 * copy in list's store, with the model's points and normals. it returns null
 * where the copy doesn't fit */
static const NJS_MODEL *welded_copy(struct draws *list, const NJS_MODEL *model)
    __attribute__((noinline));
static const NJS_MODEL *welded_copy(struct draws *list, const NJS_MODEL *model)
{
    Uint32 points = model->nbPoint * sizeof(NJS_POINT3);
    NJS_MODEL *copy;
    int i;

    for (i = 0; i < welded->count && welded->model[i] != model; i++) {
    }
    if (i == welded->count) {
        return model;
    }
    copy = store_take(list, sizeof(NJS_MODEL) +
                                points * (model->normals != NULL ? 2 : 1));
    if (copy == NULL) {
        return NULL;
    }
    words_copy(copy, model, sizeof(NJS_MODEL) / sizeof(Uint32));
    copy->points = (NJS_POINT3 *)(copy + 1);
    memcpy(copy->points, model->points, points);
    if (model->normals != NULL) {
        copy->normals = (NJS_VECTOR *)((Uint8 *)copy->points + points);
        memcpy(copy->normals, model->normals, points);
    }
    return copy;
}

/* keep in list a draw of present_task's step as an sa_draw_kept. its state,
 * matrix and shading are entries of list's tables. a sprite keeps a copy of
 * itself and its frame. a model step 0's welds wrote keeps a copy of the
 * model. it stays out of line, as marks_first does */
static void keep_draw(struct draws *list, enum present_kind kind,
                      const void *what, Int entry, Uint32 attr)
    __attribute__((noinline));
static void keep_draw(struct draws *list, enum present_kind kind,
                      const void *what, Int entry, Uint32 attr)
{
    const NJS_SPRITE *sprite = what;
    struct ninja_state state;
    NJS_MATRIX matrix;
    struct draw_shading shading;
    struct kept_sprite *copy;
    sa_draw_kept keep;
    int view;

    if (list->count >= list->most || list->top < draws_reach(list) ||
        (view = view_index(list)) < 0 ||
        (kind == PRESENT_MODEL && welded != NULL &&
         (what = welded_copy(list, what)) == NULL)) {
        list->full = TRUE;
        return;
    }
    keep.kind = kind;
    keep.view = view;
    keep.attr = attr;
    keep.shade = 0;
    state_get(&state);
    if (kind == PRESENT_MODEL) {
        /* draw_tree, draw_action, draw_motion and draw_shape_motion draw their
         * models in one run from njGetkmPointer to njSetkmPointer.
         * njSetkmPointer writes the run's header back to the context as the
         * run ends. inside such a run, the context's header lags the texture
         * set_header made current. a model kept there so keeps no current
         * texture, and its replay sets each texture its materials name */
        state.texture = NO_CURRENT_TEXTURE;
        draw_get_shading(&shading);
        keep.shade =
            table_entry(list, TABLE_SHADING, &shading, sizeof(shading), FALSE);
        keep.geometry = (Uint32)what;
    } else {
        copy = store_take(list, sizeof(struct kept_sprite));
        if (copy == NULL) {
            list->full = TRUE;
            return;
        }
        words_copy(&copy->sprite, sprite, sizeof(NJS_SPRITE) / sizeof(Uint32));
        words_copy(&copy->frame, &sprite->tanim[entry],
                   sizeof(NJS_TEXANIM) / sizeof(Uint32));
        copy->sprite.tanim = &copy->frame;
        copy->depth = sprite_get_depth();
        keep.geometry = list->top / HELD_UNIT;
    }
    njGetMatrix(&matrix);
    keep.state = table_entry(list, TABLE_STATE, &state, sizeof(state), FALSE);
    keep.matrix =
        table_entry(list, TABLE_MATRIX, matrix, sizeof(NJS_MATRIX), TRUE);
    if (keep.state == 0 || keep.matrix == 0 ||
        (kind == PRESENT_MODEL && keep.shade == 0)) {
        list->full = TRUE;
        return;
    }
#ifdef DREAMCAST
    /* a draw at an even index starts a line, and the draw to come takes its
     * other half. MOVCA.L may so claim the line unread */
    if ((list->count & 1) == 0) {
        shz_dcache_alloc_line(list->store + list->count * HELD_UNIT);
    }
#endif
    words_copy((sa_draw_kept *)list->store + list->count, &keep,
               sizeof(keep) / sizeof(Uint32));
    list->task[list->count++] = present_task;
}

/* the words a 2D draw's vertices take */
static int flat_words(enum present_kind kind, Int count, Uint32 attr)
{
    switch (kind) {
    case PRESENT_POLYGON_2D:
        /* two words for a point, one for its color, and with a texture one for
         * its uv */
        return count * (attr & NJD_USE_TEXTURE ? 4 : 3);
    case PRESENT_TEXTURE:
    case PRESENT_TEXTURE_MEMLIST:
        return count * (int)(sizeof(NJS_TEXTURE_VTX) / sizeof(Uint32));
    case PRESENT_POLYGON:
        return count * (int)(sizeof(NJS_POLYGON_VTX) / sizeof(Uint32));
    case PRESENT_TEXT:
        return count * (int)(sizeof(struct text_cell) / sizeof(Uint32));
    default:
        return 0;
    }
}

/* keep a 2D draw in flats with a copy of its vertices or cells. a sprite keeps
 * a copy of itself and the frame it draws, which may change before the
 * replay. flats is full where the draw doesn't fit. it stays out of line, as
 * marks_first does */
static void keep_flat(struct flats *flats, enum present_kind kind,
                      const void *what, Int count, Int a, Float pri,
                      Uint32 attr) __attribute__((noinline));
static void keep_flat(struct flats *flats, enum present_kind kind,
                      const void *what, Int count, Int a, Float pri,
                      Uint32 attr)
{
    int words = flat_words(kind, count, attr);
    struct flat *keep;
    Uint32 *to;

    if (flats->count >= FLATS_MOST || count < 0 ||
        words > FLAT_WORDS - flats->used) {
        flats->full = TRUE;
        return;
    }
    keep = &flats->flat[flats->count++];
    keep->task = present_task;
    keep->kind = kind;
    keep->count = count;
    keep->a = a;
    keep->pri = pri;
    keep->attr = attr;
    keep->words = flats->used;
    to = &flats->words[flats->used];
    flats->used += words;
    if (kind == PRESENT_POLYGON_2D) {
        const NJS_POINT2COL *p = what;

        memcpy(to, p->p, count * sizeof(NJS_POINT2));
        memcpy(to + count * 2, p->col, count * sizeof(NJS_COLOR));
        if (attr & NJD_USE_TEXTURE) {
            memcpy(to + count * 3, p->tex, count * sizeof(NJS_COLOR));
        }
    } else if (kind == PRESENT_SPRITE_2D) {
        keep->sprite = *(const NJS_SPRITE *)what;
        keep->frame = keep->sprite.tanim[a];
    } else {
        memcpy(to, what, words * sizeof(Uint32));
    }
    state_get(&keep->state);
}

/* TRUE where the pass draws task by its callback: it has one, and it isn't
 * its step. else the pass replays the draws its step made on step 0 */
static int callback_draws(const struct task *task)
{
    return task->draw != NULL && task->draw != task->step;
}

/* keep present_record's draw for a picture to come, and mark its task */
static void keep_record(enum present_kind kind, const void *what, Int count,
                        Int a, Float pri, Uint32 attr)
{
    struct task *task = present_task;

    if (keeping == KEEP_FIRST && frame_step == 0 && task != NULL) {
        marks_first(task);
    }
    if (kind >= PRESENT_POLYGON_2D) {
        keep_flat(keeping == KEEP_FIRST ? &first_flats : &later_flats, kind,
                  what, count, a, pri, attr);
        /* 2D draws on every step in the shipped game too. the shipped picture
         * shows a later step's 2D beside step 0's. the next frame's first
         * picture replays this draw. in the pass, the task's callback then
         * draws no 2D */
        if (keeping == KEEP_LATER && frame_split && task != NULL) {
            marks_add(task, FLATS_LATER);
        }
    } else if (task == NULL) {
        return;
    } else if (keeping == KEEP_FIRST) {
        /* the second picture may replay each of step 0's njDrawSprite3D
         * sprites. it may replay a task's other draws where callback_draws is
         * FALSE for it */
        if (frame_step != 0) {
            return;
        }
        if (kind == PRESENT_NJ_SPRITE) {
            keep_draw(&first_draws, kind, what, a, attr);
        } else if (!callback_draws(task)) {
            marks_add(task, HAS_HELD);
            keep_draw(&first_draws, kind, what, a, attr);
        }
    } else if (kind == PRESENT_NJ_SPRITE) {
        /* njDrawSprite3D draws on every step in the shipped game too, beside
         * step 0's sprites. the next frame's first picture replays this
         * sprite. in the pass, the task's callback then draws no
         * njDrawSprite3D sprite */
        keep_draw(&later_draws, kind, what, a, attr);
        if (frame_split) {
            marks_add(task, SPRITES_LATER);
        }
    } else if (frame_split && frame_step == 1) {
        marks_add(task, DREW_LATER);
    }
}

/* keep_record's call, with timer 1 stopped. it stays out of line, so that
 * present_record's frequent early returns save none of the registers it needs.
 * inlined, those saves cost 17 cycles a call */
static void keep_step(enum present_kind kind, const void *what, Int count,
                      Int a, Float pri, Uint32 attr) __attribute__((noinline));
static void keep_step(enum present_kind kind, const void *what, Int count,
                      Int a, Float pri, Uint32 attr)
{
    Uint8 run = timer_stop();

    keep_record(kind, what, count, a, pri, attr);
    TSTR = run;
}

/* mark present_task as drawing kind, 2D or an njDrawSprite3D sprite, in its
 * callback in the pass. the pass then replays none of step 0's draws of that
 * kind for it. it returns TRUE. where the task's step drew that kind after
 * step 0, it marks nothing and returns FALSE. what its steps drew stands for
 * the callback's draw then. it stays out of line, as keep_step does */
static int pass_mark(enum present_kind kind) __attribute__((noinline));
static int pass_mark(enum present_kind kind)
{
    const Uint32 *mark = marks_find(present_task);
    int sprite = kind == PRESENT_NJ_SPRITE;

    if (mark != NULL && (*mark & (sprite ? SPRITES_LATER : FLATS_LATER))) {
        return FALSE;
    }
    marks_add(present_task, sprite ? DREW_SPRITE : DREW_FLAT);
    return TRUE;
}

Int present_record(enum present_kind kind, const void *what, Int count, Int a,
                   Float pri, Uint32 attr)
{
    if (frame_redrawing) {
        return kind < PRESENT_NJ_SPRITE || present_task == NULL ||
               pass_mark(kind);
    }
    /* first_task is marked already. where the pass draws it by its callback,
     * its step's models and sprite_draw_3d sprites need no keeping. the test
     * stays here, where timer 1 runs, since keep_step's saves of its registers
     * would take longer */
    if (keeping != KEEP_NONE &&
        (present_task != first_task || present_task == NULL ||
         kind >= PRESENT_NJ_SPRITE || !callback_draws(present_task))) {
        keep_step(kind, what, count, a, pri, attr);
    }
    return TRUE;
}

/* add model to welded, the models step 0's welds wrote. where welded can't
 * take it, first_draws is full */
static void welded_add(const NJS_MODEL *model)
{
    int i;

    if (welded == NULL) {
        welded = store_take(&first_draws, sizeof(struct welded));
        if (welded == NULL) {
            first_draws.full = TRUE;
            return;
        }
        welded->count = 0;
    }
    for (i = 0; i < welded->count; i++) {
        if (welded->model[i] == model) {
            return;
        }
    }
    if (welded->count >= WELDED_MOST) {
        first_draws.full = TRUE;
        return;
    }
    welded->model[welded->count++] = model;
}

void present_welded(const struct motion *motion)
{
    const struct weld *weld = motion->welds;
    const NJS_OBJECT *base;
    Uint8 run;

    if (keeping != KEEP_FIRST || frame_step != 0 || weld == NULL) {
        return;
    }
    run = timer_stop();
    base = motion->actions[motion->wanted].action->object;
    for (; weld->base != NULL; weld++) {
        if (weld->base == base && weld->kind >= WELD_BOTH &&
            weld->kind <= WELD_TO_B) {
            welded_add(weld->object_a->model);
            welded_add(weld->object_b->model);
        }
    }
    TSTR = run;
}

LINKED_LAST(present_sprite)
Int present_sprite(enum present_kind kind, const void *what, Int count, Int a,
                   Float pri, Uint32 attr)
{
    if (!sprites_kept) {
        return TRUE;
    }
    return present_record(kind, what, count, a, pri, attr);
}

void present_untimed(int begin)
{
    if (!frame_split || frame_step != 1) {
        return;
    }
    if (begin) {
        if (untimed_depth++ == 0) {
            untimed_run = timer_stop();
        }
    } else if (--untimed_depth == 0) {
        TSTR = untimed_run;
    }
}

void present_shadow(const struct shadow_place *place)
{
    struct kept_shadow *keep;
    Uint8 run;

    if (frame_redrawing) {
        shadow_draw_placed(place);
        if (present_task != NULL) {
            marks_add(present_task, DREW_SHADOW);
        }
    } else if (frame_step == 0) {
        if (keeping == KEEP_FIRST && present_task != NULL) {
            run = timer_stop();
            marks_add(present_task, DREW_FIRST);
            TSTR = run;
        }
        shadow_draw_placed(place);
    } else if (frame_split && frame_step == 1) {
        run = timer_stop();
        if (shadows_count < SHADOWS_MOST) {
            keep = &shadows[shadows_count++];
            keep->place = *place;
            keep->task = present_task;
        } else {
            lost = STORE_SHADOWS;
        }
        TSTR = run;
    }
}

/* the task a kept draw names once the task that made it is deleted with no
 * destructor. a task made in the freed slot so takes none of its draws. no
 * destructor released their textures, so the pictures still replay the deleted
 * task's 2D and njDrawSprite3D sprites. so the Chaos fights' sparks replay,
 * though the task that draws them deletes itself. deleted_task has no marks,
 * so the pass holds none of the deleted task's other draws */
static struct task deleted_task;

/* the task a kept draw names once its own is deleted with a destructor */
static struct task released_task;

/* TRUE where task was deleted with a destructor, or is killed and has one. a
 * destructor may release what the task's draws take, so no picture replays
 * them */
static int released(const struct task *task)
{
    return task == &released_task ||
           (task != NULL && task->step == task_destroy &&
            task->destructor != NULL);
}

/* key each of list's draws that task made to deleted_task, or to released_task
 * where task has a destructor */
static void draws_forget(struct draws *list, const struct task *task)
{
    struct task *now =
        task->destructor == NULL ? &deleted_task : &released_task;
    int i;

    for (i = 0; i < list->count; i++) {
        if (list->task[i] == task) {
            list->task[i] = now;
        }
    }
}

/* key each of flats' draws that task made to deleted_task, or to released_task
 * where task has a destructor. a draw no task made keeps null, and no picture
 * skips it */
static void flats_forget(struct flats *flats, const struct task *task)
{
    struct task *now =
        task->destructor == NULL ? &deleted_task : &released_task;
    int i;

    for (i = 0; i < flats->count; i++) {
        if (flats->flat[i].task == task) {
            flats->flat[i].task = now;
        }
    }
}

/* drop each shadow step 1 kept for task */
static void shadows_forget(const struct task *task)
{
    int kept = 0;
    int i;

    for (i = 0; i < shadows_count; i++) {
        if (shadows[i].task != task) {
            shadows[kept++] = shadows[i];
        }
    }
    shadows_count = kept;
}

void present_forget(struct task *task)
{
    Uint32 *mark;
    Uint8 run;

    if (keeping == KEEP_NONE) {
        return;
    }
    run = timer_stop();
    mark = marks_find(task);
    if (mark != NULL) {
        *mark = 0;
    }
    if (task == first_task) {
        first_task = NULL;
    }
    draws_forget(&first_draws, task);
    draws_forget(&later_draws, task);
    flats_forget(&first_flats, task);
    flats_forget(&later_flats, task);
    shadows_forget(task);
    TSTR = run;
}

/* the sum of the five display lists' write pointers. a draw moves its list's
 * pointer by the bytes it sends. the sum's change over a run of draws so
 * counts what they sent. on a host it is port_draws_sent, the count of draws
 * its adapter took */
static Uint32 lists_end(void)
{
#ifdef DREAMCAST
    PKMDWORD *end = _nj_vertex_buffer_.pCurrentPtr;

    return (Uint32)end[KM_OPAQUE_POLYGON] + (Uint32)end[KM_OPAQUE_MODIFIER] +
           (Uint32)end[KM_TRANS_POLYGON] + (Uint32)end[KM_TRANS_MODIFIER] +
           (Uint32)end[KM_PUNCHTHROUGH_POLYGON];
#else
    return port_draws_sent;
#endif
}

/* what the present_run steps nested in step_measured's step have sent, and the
 * counts they took. step_measured zeroes both for each step it runs. after the
 * step, it puts back the outer step's sums, plus what the step sent and
 * took */
static Uint32 nested_sent;
static Uint32 nested_spent;

static void present_wide(Float anchor, Int cover)
{
    port_wide_anchor = anchor;
    port_wide_cover = cover;
}

/* run task's step on step 0. mark task DREW_FIRST where its step sent more
 * than the steps it ran. a task marked so, or still first_task, adds to its
 * marks the counts its step took. those exclude the steps it ran. a task that
 * sent nothing of its own and isn't first_task adds none. the pass then takes
 * its callback to need none. a task its step deleted has no step and takes no
 * mark. a task the step made after the delete can take its slot, and the mark
 * with it. timer 1 stops across all but the step. it stays out of line, so
 * that present_run keeps the present kernel in its window */
static void step_measured(struct task *task, void (*step)(struct task *task))
    __attribute__((noinline));
static void step_measured(struct task *task, void (*step)(struct task *task))
{
    Uint8 run = timer_stop();
    Uint32 nested = nested_sent;
    Uint32 nested_time = nested_spent;
    Uint32 sprites = port_sprites_sent;
    Uint32 began;
    Uint32 spent;
    Uint32 sent;
    Uint32 *mark;

    nested_sent = 0;
    nested_spent = 0;
    sent = lists_end();
    began = syTmrGetCount();
    TSTR = run;
    step(task);
    run = timer_stop();
    spent = syTmrGetCount() - began;
    sent = lists_end() - sent;
    if (task->step != NULL) {
        if (sent != nested_sent) {
            mark = marks_add(task, sent == port_sprites_sent - sprites
                                       ? DREW_FIRST
                                       : DREW_FIRST | DREW_OTHER);
        } else {
            mark = task == first_task ? first_mark : NULL;
        }
        if (mark != NULL) {
            *mark += (spent - nested_spent) << SPENT_SHIFT;
        }
    }
    nested_sent = nested + sent;
    nested_spent = nested_time + spent;
    TSTR = run;
}

void present_run(struct task *task, void (*step)(struct task *task))
{
    struct task *outer = present_task;
    /* a root task's step starts and ends at WIDE_MIDDLE and FALSE. a child's,
     * which its parent's step runs, starts and ends at what the parent set */
    Float anchor = outer != NULL ? port_wide_anchor : WIDE_MIDDLE;
    Int cover = outer != NULL ? port_wide_cover : FALSE;

    if (frame_redrawing) {
        return;
    }
    present_wide(anchor, cover);
    present_task = task;
    if (keeping == KEEP_FIRST && frame_step == 0) {
        step_measured(task, step);
    } else {
        step(task);
    }
    present_task = outer;
    present_wide(anchor, cover);
}

/* the records of a task that the pass puts back after its callback. the pass
 * copies a whole task_work. a task whose callback it calls must have a whole
 * one or none */
struct records {
    struct task_work work;
    struct task_motion motion;
    struct player record;
    struct collision_shape shapes[SHAPES_KEPT];
    /* the player's record, or null, and the shapes kept */
    struct player *player;
    int shapes_kept;
};

/* keep task's records. it returns TRUE, or FALSE where a player's shapes don't
 * fit, which loses the second picture */
static int records_keep(struct records *keep, const struct task *task)
{
    int i;

    keep->player = NULL;
    keep->shapes_kept = 0;
    if (task->work != NULL) {
        words_copy(&keep->work, task->work,
                   sizeof(struct task_work) / sizeof(Uint32));
    }
    for (i = 0; i < PLAYER_COUNT; i++) {
        if (player_tasks[i] == task && player_records[i] != NULL) {
            keep->player = player_records[i];
        }
    }
    if (keep->player == NULL) {
        return TRUE;
    }
    keep->motion = *task->motion;
    keep->record = *keep->player;
    if (task->work != NULL && task->work->collision != NULL) {
        keep->shapes_kept = task->work->collision->count;
        if (keep->shapes_kept > shapes_most) {
            shapes_most = keep->shapes_kept;
        }
        if (keep->shapes_kept > SHAPES_KEPT) {
            lost = STORE_SHAPES;
            return FALSE;
        }
        memcpy(keep->shapes, task->work->collision->shapes,
               keep->shapes_kept * sizeof(struct collision_shape));
    }
    return TRUE;
}

static void records_put(const struct records *keep, struct task *task)
{
    if (task->work != NULL) {
        words_copy(task->work, &keep->work,
                   sizeof(struct task_work) / sizeof(Uint32));
    }
    if (keep->player == NULL) {
        return;
    }
    *task->motion = keep->motion;
    *keep->player = keep->record;
    if (keep->shapes_kept != 0) {
        memcpy(task->work->collision->shapes, keep->shapes,
               keep->shapes_kept * sizeof(struct collision_shape));
    }
}

/* stop the pass where its callbacks, given need counts more from now, would
 * end past pass_end. stopped_at keeps when they would end. it returns TRUE
 * where the pass has stopped, now or before */
static int pass_stop(Uint32 need)
{
    Uint32 now;

    if (!stopped) {
        now = syTmrGetCount();
        if ((Sint32)(now + need - pass_end) > 0) {
            stopped = TRUE;
            stopped_at = now + need;
        }
    }
    return stopped;
}

void present_draw(struct task *task, void (*draw)(struct task *task))
{
    struct task *outer = present_task;
    struct records keep;
    Uint32 *mark;

    /* a callback starts and ends at WIDE_MIDDLE and FALSE, since
     * task_draw_all, which calls each, runs outside every task */
    if (!frame_redrawing) {
        if (draw != NULL) {
            present_wide(WIDE_MIDDLE, FALSE);
            draw(task);
            present_wide(WIDE_MIDDLE, FALSE);
        }
        return;
    }
    /* a killed task draws nothing. the pass calls no callback of the camera
     * task. the underwater tint it draws shows in each picture already, from
     * both steps. it clears camera_in_dry_box too, which a step sets */
    if (task->step == task_destroy ||
        (camera_work != NULL && task->work == camera_work)) {
        return;
    }
    mark = marks_find(task);
    if (mark != NULL && (*mark & DREW_LATER)) {
        return;
    }
    if (callback_draws(task)) {
        /* the pass calls no callback of a task whose step drew nothing on step
         * 0. nor does it call one whose step drew only njDrawSprite3D sprites,
         * on step 0 and after it. its later sprites stand for the callback's.
         * the pass takes each callback to need what its step took on step 0.
         * it stops before one that would end late */
        if (mark == NULL || !(*mark & DREW_FIRST) ||
            (*mark & (DREW_OTHER | SPRITES_LATER)) == SPRITES_LATER ||
            pass_stop(*mark >> SPENT_SHIFT) || !records_keep(&keep, task)) {
            return;
        }
        present_wide(WIDE_MIDDLE, FALSE);
        present_task = task;
        draw(task);
        present_task = outer;
        records_put(&keep, task);
        present_wide(WIDE_MIDDLE, FALSE);
        return;
    }
    if (mark != NULL) {
        *mark |= HOLDS;
    }
}

int present_drawing(const struct task *task)
{
    return frame_redrawing && present_task == task;
}

/* v turned by m's rotation, as a row times m's first three rows and
 * columns */
static NJS_VECTOR turned(const Float *m, const NJS_VECTOR *v)
{
    NJS_VECTOR to;

    to.x = v->x * m[0] + v->y * m[4] + v->z * m[8];
    to.y = v->x * m[1] + v->y * m[5] + v->z * m[9];
    to.z = v->x * m[2] + v->y * m[6] + v->z * m[10];
    return to;
}

/* the move from the view from to the view now: from's inverse times the view
 * now. a matrix made under from is a world times from. that matrix times the
 * move is the world times the view now */
static void view_move(NJS_MATRIX *move, const NJS_MATRIX *from)
{
    NJS_MATRIX inverse;

    memcpy(inverse, *from, sizeof(NJS_MATRIX));
    njInvertMatrix(&inverse);
    memcpy(*move, _nj_view_matrix_, sizeof(NJS_MATRIX));
    njMultiMatrix(move, &inverse);
}

/* TRUE where a picture replays draw, which task's step made. the first picture
 * replays each of the last frame's later sprites. the second replays each of
 * step 0's njDrawSprite3D sprites, except where the pass drew one for the
 * task. it replays step 0's other draws where the pass holds their task.
 * neither replays a draw whose task has a destructor and was killed or
 * deleted. the destructor may release the draw's textures */
static int replays(const sa_draw_kept *draw, const struct task *task,
                   int second)
{
    const Uint32 *mark;

    if (released(task)) {
        return FALSE;
    }
    if (!second) {
        return TRUE;
    }
    mark = marks_find(task);
    if (draw->kind == PRESENT_NJ_SPRITE) {
        return mark == NULL || !(*mark & DREW_SPRITE);
    }
    return mark != NULL && (*mark & HOLDS);
}

/* draw again each of list's draws that the picture replays, moved from its
 * view to the view now. second is TRUE for the second picture. the current
 * matrix goes back after, since particle_draw_all draws through it. a replay
 * keeps nothing, so present_sprite keeps none of its sprites */
static void draws_replay(struct draws *list, int second)
{
    NJS_MATRIX moves[VIEWS_MOST] __attribute__((aligned(8)));
    NJS_MATRIX current;
    struct draw_shading shading;
    const sa_draw_kept *draw;
    struct kept_sprite *copy;
    int i;

    for (i = 0; i < list->view_count; i++) {
        view_move(&moves[i], &list->views[i]);
    }
    njGetMatrix(&current);
    sprites_kept = FALSE;
    for (i = 0; i < list->count; i++) {
        draw = (const sa_draw_kept *)list->store + i;
        if (!replays(draw, list->task[i], second)) {
            continue;
        }
        /* the draw's matrix times the move, made in the current matrix */
#ifdef DREAMCAST
        shz_xmtrx_load_4x4((const shz_mat4x4_t *)&moves[draw->view]);
        shz_xmtrx_apply_4x4(HELD_ENTRY(list, draw->matrix));
#else
        njSetMatrix(NULL, &moves[draw->view]);
        njMultiMatrix(NULL, HELD_ENTRY(list, draw->matrix));
#endif
        state_put(HELD_ENTRY(list, draw->state));
        if (draw->kind == PRESENT_MODEL) {
            /* the light lies in view space, so it turns with the move */
            shading =
                *(const struct draw_shading *)HELD_ENTRY(list, draw->shade);
            shading.dir = turned(moves[draw->view], &shading.dir);
            shading.base = turned(moves[draw->view], &shading.base);
            draw_put_shading(&shading);
            draw_model((NJS_MODEL *)draw->geometry);
            continue;
        }
        copy = HELD_ENTRY(list, draw->geometry);
        if (draw->kind == PRESENT_SPRITE) {
            sprite_set_depth(copy->depth);
            sprite_draw_3d(&copy->sprite, 0, draw->attr);
        } else {
            njDrawSprite3D(&copy->sprite, 0, draw->attr);
        }
    }
    sprites_kept = TRUE;
    njSetMatrix(NULL, &current);
}

/* draw again each of flats' 2D draws, except a released task's. where fresh is
 * TRUE, it skips too those of a task the pass drew 2D for. present_sprite
 * keeps none of its sprites, as in draws_replay */
static void draw_flats(struct flats *flats, int fresh)
{
    const Uint32 *mark;
    struct flat *draw;
    Uint32 *words;
    NJS_POINT2COL points;
    NJS_SPRITE sprite;

    sprites_kept = FALSE;
    for (draw = flats->flat; draw < flats->flat + flats->count; draw++) {
        if (released(draw->task)) {
            continue;
        }
        if (fresh && draw->task != NULL) {
            mark = marks_find(draw->task);
            if (mark != NULL && (*mark & DREW_FLAT)) {
                continue;
            }
        }
        words = &flats->words[draw->words];
        state_put(&draw->state);
        switch (draw->kind) {
        case PRESENT_POLYGON_2D:
            points.p = (NJS_POINT2 *)words;
            points.col = (NJS_COLOR *)(words + draw->count * 2);
            points.tex = draw->attr & NJD_USE_TEXTURE
                             ? (NJS_COLOR *)(words + draw->count * 3)
                             : NULL;
            points.num = draw->count;
            njDrawPolygon2D(&points, draw->count, draw->pri, draw->attr);
            break;
        case PRESENT_TEXTURE:
            njDrawTexture((NJS_TEXTURE_VTX *)words, draw->count, draw->a,
                          (Int)draw->attr);
            break;
        case PRESENT_TEXTURE_MEMLIST:
            njDrawTextureMemList((NJS_TEXTURE_VTX *)words, draw->count, draw->a,
                                 (Int)draw->attr);
            break;
        case PRESENT_POLYGON:
            njDrawPolygon((NJS_POLYGON_VTX *)words, draw->count,
                          (Int)draw->attr);
            break;
        case PRESENT_TEXT:
            text_draw_cells((struct text_cell *)words, draw->count, draw->a,
                            draw->pri, draw->attr);
            break;
        default:
            sprite = draw->sprite;
            sprite.tanim = &draw->frame;
            njDrawSprite2D(&sprite, 0, draw->pri, draw->attr);
            break;
        }
    }
    sprites_kept = TRUE;
}

/* the camera's pose now */
static void pose_get(struct pose *pose)
{
    pose->work = camera_work;
    if (camera_work != NULL) {
        pose->ang = camera_work->ang;
        pose->pos = camera_work->pos;
    }
    pose->eye.x = view.px;
    pose->eye.y = view.py;
    pose->eye.z = view.pz;
}

/* how the camera went from from to to */
static enum went pose_went(const struct pose *from, const struct pose *to)
{
    if (from->work == NULL || from->work != to->work) {
        return WENT_ELSE;
    }
    if (memcmp(&from->ang, &to->ang, sizeof(struct angles)) == 0 &&
        memcmp(&from->pos, &to->pos, sizeof(NJS_POINT3)) == 0) {
        return WENT_STOOD;
    }
    return WENT_MOVED;
}

/* TRUE where the move from from to to is a cut. a NaN fails each test of
 * the move, so it cuts */
static int pose_cut(const struct pose *from, const struct pose *to)
{
    Float x = to->pos.x - from->pos.x;
    Float y = to->pos.y - from->pos.y;
    Float z = to->pos.z - from->pos.z;
    Angle ang_x = angle_difference(from->ang.x, to->ang.x);
    Angle ang_y = angle_difference(from->ang.y, to->ang.y);
    Angle ang_z = angle_difference(from->ang.z, to->ang.z);

    return !(x <= CUT_MOVE && x >= -CUT_MOVE && y <= CUT_MOVE &&
             y >= -CUT_MOVE && z <= CUT_MOVE && z >= -CUT_MOVE) ||
           ang_x > CUT_TURN || ang_x < -CUT_TURN || ang_y > CUT_TURN ||
           ang_y < -CUT_TURN || ang_z > CUT_TURN || ang_z < -CUT_TURN;
}

/* TRUE where Ninja's view is main.c's view and no camera action gives it, so
 * camera_work's pose makes it */
static int camera_viewing(void)
{
    return _nj_view_ == &view && camera_action_state == ACTION_NONE;
}

/* give camera_work the pose halfway between from and to, each angle by its
 * shortest way round. make the view from it as camera.c's camera_apply_view
 * does, but with its eye halfway between from's and to's. the eye so keeps
 * camera_add_offset's shakes. the models' light and the easy draws' light turn
 * with the view */
static void camera_between(const struct pose *from, const struct pose *to)
{
    struct task_work *work = camera_work;
    NJS_MATRIX *top = _nj_current_matrix_ptr_;
    Sint32 depth = _stack_size_;
    NJS_MATRIX base;
    NJS_MATRIX move;
    struct draw_shading shading;

    work->ang.x = from->ang.x + angle_difference(from->ang.x, to->ang.x) / 2;
    work->ang.y = from->ang.y + angle_difference(from->ang.y, to->ang.y) / 2;
    work->ang.z = from->ang.z + angle_difference(from->ang.z, to->ang.z) / 2;
    work->pos.x = (from->pos.x + to->pos.x) * 0.5F;
    work->pos.y = (from->pos.y + to->pos.y) * 0.5F;
    work->pos.z = (from->pos.z + to->pos.z) * 0.5F;
    between = view;
    njRotateViewZAbsolute(&between, work->ang.z);
    njRotateViewXAbsolute(&between, work->ang.x);
    njRotateViewYRelative(&between, work->ang.y);
    between.px = (from->eye.x + to->eye.x) * 0.5F;
    between.py = (from->eye.y + to->eye.y) * 0.5F;
    between.pz = (from->eye.z + to->eye.z) * 0.5F;
    /* njSetView empties the matrix stack, but the pass's draws push above what
     * it held. its depth, top and base so go back under the new view */
    memcpy(base, *_nj_base_matrix_ptr_, sizeof(NJS_MATRIX));
    njSetView(&between);
    _stack_size_ = depth;
    _nj_current_matrix_ptr_ = top;
    memcpy(*_nj_base_matrix_ptr_, base, sizeof(NJS_MATRIX));
    /* the lights lie in view space, so they turn with the move */
    view_move(&move, &saved.view_matrix);
    draw_get_shading(&shading);
    shading.dir = turned(move, &shading.dir);
    shading.base = turned(move, &shading.base);
    draw_put_shading(&shading);
    *port_easy_light() = turned(move, port_easy_light());
}

/* the TA's intake of the second picture's lists. it is the longer of the last
 * transfer and second_ta */
static Uint32 second_intake(void)
{
    Uint32 ta = kmiIntStatisticData.ta_time;

    return ta > second_ta ? ta : second_ta;
}

/* the 32-byte blocks of the lists Kamui holds for the TA's intake at the flip.
 * vertex_buffer_list is not among them: the game's NOBUF model sends it as the
 * draws write it. present_second takes the TA's intake to grow in step with
 * these blocks. on a host, which has no TA, it returns 0 */
static Uint32 held_blocks(void)
{
    Uint32 bytes = 0;
#ifdef DREAMCAST
    Uint32 list;

    for (list = KM_OPAQUE_POLYGON; list <= KM_PUNCHTHROUGH_POLYGON; list++) {
        if (list != kmiDeviceExtension.vertex_buffer_list) {
            bytes += (Uint32)kmiCurrentVertexBuff[list] -
                     (Uint32)kmiVertexBuff[list];
        }
    }
#endif
    return bytes / 32;
}

/* TRUE where step 1 and the pass fit the second half after a step 0 of step0
 * counts. the shares price step 1 and the pass, and the half holds the TA's
 * intake and SECOND_ROOM too */
static int second_fits(Uint32 step0)
{
    return step0 * (step1_share + pass_share) / WHOLE_SHARE + SECOND_ROOM +
               second_intake() <=
           VSYNC_COUNTS;
}

/* the cells of the prints among flats' 2D */
static int flats_cells(const struct flats *flats)
{
    const struct flat *draw;
    int cells = 0;

    for (draw = flats->flat; draw < flats->flat + flats->count; draw++) {
        if (draw->kind == PRESENT_TEXT) {
            cells += draw->count;
        }
    }
    return cells;
}

/* the room the pass's tail takes where tail_cost doesn't stand for it */
static Uint32 tail_price(void)
{
    return TAIL_ROOM + (first_draws.count + shadows_count) * SPRITE_ROOM +
           first_flats.count * FLAT_ROOM +
           flats_cells(&first_flats) * CELL_ROOM;
}

/* the room the first picture takes past step 0, for what it will replay */
static Uint32 first_room(void)
{
    return FIRST_ROOM + later_draws.count * SPRITE_ROOM +
           later_flats.count * FLAT_ROOM +
           flats_cells(&later_flats) * CELL_ROOM;
}

/* TRUE where the PVR can take the first picture on time. the picture's replay
 * ends first counts into the frame. Kamui starts a render only at a vsync. the
 * render running now must end by the middle vsync, where the first picture's
 * starts. the first picture's lists must reach the TA by then too. its own
 * render must end by the frame's last vsync, where the second's starts. the
 * last render's and transfer's lengths, as Kamui times them, stand in for the
 * next. a first picture's own transfer can run longer, and past FIRST_ROOM's
 * margin it can miss the middle vsync. Kamui keeps a render's start as TCNT0,
 * which counts down as syTmrGetCount counts up. its complement is the start on
 * syTmrGetCount's clock */
static int pvr_fits(Uint32 first)
{
    Uint32 started = ~kmiIntStatisticData.render_start;
    Uint32 length = kmiIntStatisticData.render_time;

    if (kmiDeviceExtension.render_busy != 0 &&
        (Sint32)(started + length - _nj_wait_vsync_out_) + RENDER_ROOM >
            VSYNC_COUNTS) {
        return FALSE;
    }
    return first + kmiIntStatisticData.ta_time <= VSYNC_COUNTS &&
           length + RENDER_ROOM <= VSYNC_COUNTS;
}

/* TRUE where this frame fits with ENGAGE_ROOM to spare. step 0's tests took
 * the room of the first picture's replay of what the last frame kept. a frame
 * that kept its later 2D and sprites takes the room of their replay too. the
 * frame whose fit would engage the split is such a frame */
static int spares(void)
{
    return spare_fits &&
           (keeping != KEEP_LATER ||
            step0_cost + ENGAGE_ROOM + first_room() + last_ta <= VSYNC_COUNTS);
}

/* show the first picture. it draws the last frame's later sprites and 2D and
 * the overlay over what step 0 drew. the PVR renders it from the middle vsync,
 * a vsync past the frame's start, where step 0's room ends. the flip waits for
 * no vsync. a console's waits until the TA has the lists, so step 1 starts
 * then. a host's waits for nothing */
static void show_first(void)
{
    Uint8 run = timer_stop();
    Uint32 flip;

    ninja_save();
    draws_replay(&later_draws, FALSE);
    draw_flats(&later_flats, FALSE);
    /* the overlay draws outside pillarbox, as at the frame's end */
    port_pillarbox = FALSE;
    overlay_draw();
    ninja_restore();
    /* render and count as njWaitVSync does before its flip */
    kmRender();
    _nj_calc_vertex_ = 0;
    _nj_calc_polygon_ = 0;
    _nj_draw_polygon_ = 0;
    _nj_vsync_count_++;
    flip = syTmrGetCount();
#ifdef DREAMCAST
    owner_flip_on();
#else
    kmFlipFrameBuffer();
#endif
    middle_wait = syTmrGetCount() - flip;
    middle = _nj_wait_vsync_out_ + VSYNC_COUNTS;
    TSTR = run;
}

void present_step(void)
{
    Uint32 now = syTmrGetCount();
    Uint32 room;
    Uint8 run;

    overlay_mark(frame_step == 0 ? MARK_STEP0_END : MARK_LATER_END);
    if (frame_step == 1 && frame_split && step0_cost != 0) {
        step1_share = (now - step1_start) * WHOLE_SHARE / step0_cost;
    }
    if (frame_step != 0) {
        later_end = now;
        return;
    }
    stage_ran = TRUE;
    step0_cost = now - _nj_wait_vsync_out_;
    last_ta = kmiIntStatisticData.ta_time;
    if (end_shown) {
        second_ta = kmiIntStatisticData.ta_time;
        second_blocks = end_blocks;
    }
    /* first_room's count of the prints' cells, which 30 fps doesn't make, runs
     * with timer 1 stopped. only a frame that may split keeps a print to
     * count */
    if (later_flats.count != 0) {
        run = timer_stop();
        room = first_room();
        TSTR = run;
    } else {
        room = first_room();
    }
    first_fits = step0_cost + room <= VSYNC_COUNTS;
    render_fits = pvr_fits(step0_cost + room);
    /* pvr_fits tests step 0's time with the first picture's room, so
     * spare_fits needs no first_fits test */
    spare_fits = pvr_fits(step0_cost + ENGAGE_ROOM + room) &&
                 second_fits(step0_cost + ENGAGE_ROOM);
    second_short = first_fits && render_fits && !second_fits(step0_cost);
    bitmap_waits = texture_queue_count != 0;
    frame_split = FALSE;
    keeping = KEEP_NONE;
    first_task = NULL;
    /* a bitmap step 0 queued uploads at the frame's end, as at 30 fps. a frame
     * with one waiting so shows one picture. where the last frame's first
     * picture held, this frame shows its first at the middle vsync. one
     * picture at its end would leave the held one on screen three vsyncs. its
     * pass then stops where the second half runs short */
    if (planned && engaged && later_kept && !lost && !first_flats.full &&
        !first_draws.full && first_fits && render_fits &&
        (second_fits(step0_cost) || holding) &&
        frame_countdown == MIDDLE_AHEAD && draw_off != TRUE && !bitmap_waits) {
        show_first();
        frame_split = TRUE;
    }
    /* step 1's models and sprite_draw_3d sprites draw for the second picture.
     * they draw nothing where it may take a pose between the pictures'. there
     * the last frame's camera moved over its later steps alone, and this
     * frame's stood over step 0 */
    pose_get(&first_pose);
    frame_later_draws =
        frame_split && !(moved_later && camera_viewing() &&
                         pose_went(&start_pose, &first_pose) == WENT_STOOD);
    later_flats.count = later_flats.used = later_flats.full = 0;
    draws_clear(&later_draws);
    shadows_count = 0;
    if (planned) {
        keeping = KEEP_LATER;
    }
    step1_start = syTmrGetCount();
}

void present_second(void)
{
    Uint32 start = syTmrGetCount();
    unsigned long rnext = _rnext;
    int step = frame_step;
    struct pose now;
    enum went first;
    enum went later;
    const Uint32 *mark;
    Uint32 tail;
    Uint32 tail_need;
    Uint32 intake;
    Uint32 grown;
    Uint32 deadline;
    Uint32 end;
    Uint32 share;
    int halfway;
    int i;

    overlay_mark(MARK_PASS_START);
    /* how the camera went over step 0, and over the later steps */
    pose_get(&now);
    first = pose_went(&start_pose, &first_pose);
    later = pose_went(&first_pose, &now);
    start_pose = now;
    moved_later = first == WENT_STOOD && later == WENT_MOVED;
    if (!frame_split) {
        return;
    }
    /* the first picture holds where the camera moved over step 0 alone, and
     * where a record didn't fit. a picture that lacks a record would show a
     * draw in one picture alone */
    if ((first == WENT_MOVED && later == WENT_STOOD) || lost) {
        dropped = TRUE;
        return;
    }
    /* the pass must end by deadline, which leaves SECOND_ROOM and the TA's
     * intake before the frame's last vsync. its callbacks must end by
     * pass_end, which leaves room for the tail. the tail is priced at what the
     * last pass that ended took for it, within SETTLE_FRAMES. where none did,
     * tail_price prices it */
    tail_need = tail_age < SETTLE_FRAMES ? tail_cost : tail_price();
    intake = second_intake();
    deadline = middle + VSYNC_COUNTS - SECOND_ROOM - intake;
    pass_end = deadline - tail_need;
    /* where frame_later_draws is FALSE, a camera that moved over the later
     * steps takes the pose halfway. a cut keeps the last step's view */
    halfway = !frame_later_draws && later == WENT_MOVED && camera_viewing() &&
              !pose_cut(&first_pose, &now);
    /* the pass draws as SEGA's pause does, at frame_step 0, where the draws'
     * tests let them draw */
    frame_redrawing = TRUE;
    frame_step = 0;
    ninja_save();
    if (halfway) {
        camera_between(&first_pose, &now);
    }
    task_draw_all();
    tail = syTmrGetCount();
    if (!pass_stop(0)) {
        /* step 1's shadows, but those of a task whose callback drew one */
        for (i = 0; i < shadows_count; i++) {
            mark = shadows[i].task != NULL ? marks_find(shadows[i].task) : NULL;
            if (mark == NULL || !(*mark & DREW_SHADOW)) {
                shadow_draw_placed(&shadows[i].place);
            }
        }
        draws_replay(&first_draws, TRUE);
        particle_draw_all();
        draw_flats(&first_flats, TRUE);
    }
    ninja_restore();
    if (halfway) {
        camera_work->ang = now.ang;
        camera_work->pos = now.pos;
    }
    frame_step = step;
    _rnext = rnext;
    frame_redrawing = FALSE;
    end = syTmrGetCount();
    overlay_mark(MARK_PASS_END);
    pass_cost = end - start;
    if (stopped) {
        /* a stopped pass is measured as though its callbacks ran to stopped_at
         * and its tail followed */
        end = stopped_at + tail_need;
    } else {
        tail_cost = end - tail;
        tail_age = 0;
    }
    if (step0_cost != 0) {
        share = (end - start) * WHOLE_SHARE / step0_cost;
        if (!stopped || share > pass_share) {
            pass_share = share;
        }
        shares_age = 0;
    }
    /* the TA takes the pass's lists at the rate it took the last end picture's
     * blocks. where that intake passes the one deadline left room for,
     * deadline comes in by the excess */
    if (second_blocks != 0) {
        grown = held_blocks() * second_ta / second_blocks;
        if (grown > intake) {
            deadline -= grown - intake;
        }
    }
    /* the first picture holds too where the pass stopped or a record it needs
     * didn't fit. it holds where the tail ended past deadline, since the
     * picture could show a vsync late */
    if (stopped || lost || (Sint32)(end - deadline) > 0) {
        dropped = TRUE;
    }
}

/* the first store, in present_store's order, that ran out this frame, or
 * STORES where none did */
static int store_out(void)
{
    return first_draws.full                       ? STORE_DRAWS
           : later_draws.full                     ? STORE_SPRITES
           : first_flats.full || later_flats.full ? STORE_FLATS
           : lost != 0                            ? lost
                                                  : STORES;
}

enum present_shown present_shown(int late)
{
    /* a store ran out as the split ended, or in this frame */
    int ran_out =
        !records_fit || (keeping == KEEP_LATER && store_out() != STORES);

    if (!port_split_wanted || !stage_ran) {
        return SHOWN_UNWANTED;
    }
    /* a frame held for more than one reason takes the first named here. a pass
     * that stops drops its picture, but counts as stopped. a store that ran
     * out holds the split off, so it counts ahead of a test of time. pvr_fits
     * counts step 0's time too, so a step 0 past its half counts as HELD_FIRST,
     * ahead of the PVR */
    return frame_split && !dropped   ? SHOWN_BOTH
           : late != 0               ? HELD_LATE
           : stopped                 ? HELD_STOPPED
           : dropped                 ? HELD_DROPPED
           : ran_out                 ? HELD_FULL
           : !first_fits             ? HELD_FIRST
           : !render_fits            ? HELD_PVR
           : second_short            ? HELD_SECOND
           : engaged && bitmap_waits ? HELD_BITMAP
           : !engaged && !spares()   ? HELD_SPARE
                                     : HELD_NONE;
}

/* the percent of most that count takes, or 100 where its store ran out */
static Uint32 full(int count, int most, int out)
{
    return out ? 100 : count * 100 / most;
}

/* the larger percent of their room that flats' draws or their words take. it
 * is 100 where a draw didn't fit */
static Uint32 flats_full(const struct flats *flats)
{
    Uint32 draws = full(flats->count, FLATS_MOST, flats->full);
    Uint32 words = full(flats->used, FLAT_WORDS, flats->full);

    return draws > words ? draws : words;
}

/* the larger percent of their room that list's draws or its store's bytes
 * take. it is 100 where a draw didn't fit */
static Uint32 held_full(const struct draws *list)
{
    Uint32 bytes = list->most * HELD_BYTES;
    Uint32 draws = full(list->count, list->most, list->full);
    Uint32 used =
        full(list->count * HELD_UNIT + bytes - list->top, bytes, list->full);

    return draws > used ? draws : used;
}

int present_figures(struct present_figures *figures)
{
    Uint32 first;
    Uint32 later;

    if (!stage_ran) {
        return FALSE;
    }
    figures->step0 = step0_cost;
    figures->later_end = later_end;
    figures->pass = pass_cost;
    figures->ta = last_ta;
    figures->out = records_fit ? STORES : held_store;
    figures->full[STORE_DRAWS] = held_full(&first_draws);
    figures->full[STORE_SPRITES] = held_full(&later_draws);
    first = flats_full(&first_flats);
    later = flats_full(&later_flats);
    figures->full[STORE_FLATS] = first > later ? first : later;
    figures->full[STORE_MARKS] =
        full(marks_used, MARKS_MOST, lost == STORE_MARKS);
    figures->full[STORE_SHAPES] =
        full(shapes_most, SHAPES_KEPT, lost == STORE_SHAPES);
    figures->full[STORE_SHADOWS] =
        full(shadows_count, SHADOWS_MOST, lost == STORE_SHADOWS);
    return TRUE;
}

int present_hold(void)
{
    if (!holding) {
        return FALSE;
    }
    /* the queue uploads once a frame, as at 30 fps. the flip would wait out
     * the frame's first vsync, and a host's waits for none */
    bitmap_flush_queue();
#ifdef DREAMCAST
    while (frame_countdown == frame_interval) {
    }
#endif
    return TRUE;
}

Uint32 present_idle(void)
{
    return middle_wait;
}

void present_end(int late)
{
    int want = port_split_wanted;
    int kept;
    int missed;
    int fits;

    later_kept =
        keeping == KEEP_LATER && !later_flats.full && !later_draws.full;
    kept = later_kept && !lost && !first_flats.full && !first_draws.full;
    if (keeping == KEEP_LATER && engaged) {
        records_fit = kept;
        held_store = store_out();
    }
    /* a frame missed where it ran late, or where its pacing held it to one
     * picture for time. a stopped pass is no miss: its share holds the next
     * frame where the second half runs short */
    missed = late != 0 || (!frame_split && !(first_fits && render_fits &&
                                             second_fits(step0_cost)));
    if (!engaged && !records_fit && refit_wait > 0 && --refit_wait == 0) {
        records_fit = TRUE;
    }
    fits = stage_ran && !missed && records_fit &&
           (keeping != KEEP_LATER || kept) && spares();
    keeping = KEEP_NONE;
    holding = dropped;
    dropped = FALSE;
    if (!stage_ran) {
        frame_split = frame_later_draws = FALSE;
    }
    if (!want || !stage_ran) {
        engaged = fit_run = 0;
        calm = SETTLE_FRAMES;
        records_fit = TRUE;
        refit_wait = refit_calm = 0;
        refit_next = REFIT_FRAMES;
        step1_share = STEP1_SHARE;
        pass_share = PASS_SHARE;
        shares_age = 0;
        tail_age = SETTLE_FRAMES;
    } else if (engaged && !records_fit) {
        engaged = FALSE;
        refit_wait = refit_next;
        refit_next = refit_next * 2 < REFIT_MOST ? refit_next * 2 : REFIT_MOST;
        refit_calm = 0;
    } else if (engaged && missed) {
        if (calm < SETTLE_FRAMES) {
            engaged = FALSE;
        }
        calm = 0;
    } else if (engaged) {
        if (calm < SETTLE_FRAMES) {
            calm++;
        }
    } else if (fits) {
        if (++fit_run >= ENGAGE_FRAMES) {
            engaged = TRUE;
            fit_run = 0;
        }
    } else {
        fit_run = 0;
    }
    if (engaged && ++refit_calm >= REFIT_MOST) {
        refit_calm = 0;
        refit_next = REFIT_FRAMES;
    }
    if (++shares_age == SETTLE_FRAMES) {
        shares_age = 0;
        if (step1_share > STEP1_SHARE) {
            step1_share = (step1_share + STEP1_SHARE) / 2;
        }
        if (pass_share > PASS_SHARE) {
            pass_share = (pass_share + PASS_SHARE) / 2;
        }
    }
    if (tail_age < SETTLE_FRAMES) {
        tail_age++;
    }
    end_shown = !holding;
    planned = want && (engaged || fit_run == ENGAGE_FRAMES - 1);
    sprites_kept = TRUE;
    overlay_draw();
    end_blocks = want ? held_blocks() : 0;
    first_flats.count = first_flats.used = first_flats.full = 0;
    draws_clear(&first_draws);
    welded = NULL;
    lost = shapes_most = 0;
    middle_wait = pass_cost = 0;
    if (marks_used != 0) {
        memset(marks, 0, sizeof(marks));
        marks_used = 0;
    }
    first_task = NULL;
    stage_ran = first_fits = spare_fits = render_fits = stopped = FALSE;
    if (planned) {
        keeping = KEEP_FIRST;
    }
}
#endif
