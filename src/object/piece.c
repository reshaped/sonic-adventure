/* O EME P, a place where a piece of the master emerald can lie for
 * Knuckles to find, and the piece as he takes it. also the sprite that
 * blinks over a target */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "effect/sparkle.h"
#include "object/models.h"
#include "object/piece.h"
#include "object/placement.h"
#include "player/knuckles.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/texlists.h" /* NOLINT(misc-include-cleaner) */
#include "system/texture.h"
#include "system/util.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 13
#define TEXT_4_LENGTH 13
#include "object/piece/text.inc"

/* a place's kind is the low four bits of its ang.x. its ang.z holds its
 * piece's id, one of knuckles.h's piece ids */
#define PIECE_KIND(work) ((work)->ang.x & 0xf)
#define PIECE_ID(work) ((work)->ang.z)

/* a place's kind: where its piece can lie, or a mark, where Knuckles
 * restarts once he takes the piece. the set editor draws a mark's red
 * triangle */
enum piece_kind { PIECE_PLACE, PIECE_MARK };

/* how near Knuckles takes a piece. he takes a buried one within DIG_REACH
 * of his position as he digs up. he takes any other within TAKE_REACH of
 * his middle, 5 above his position */
#define DIG_REACH 30.0F
#define TAKE_REACH 10.0F

/* a taken piece rises, then flies to TAKEN_ABOVE above Knuckles. each step
 * of its flight it keeps TAKEN_KEEP of its offset from that point. it goes
 * once its flight's timer passes TAKEN_TIME. past TAKEN_SPARKLE_TIME, on two
 * steps of every four, it throws a sparkle in place of drawing itself */
#define TAKEN_ABOVE 20.0F
#define TAKEN_KEEP 0.95F
#define TAKEN_TIME 120
#define TAKEN_SPARKLE_TIME 60

/* the piece's material flags but its blend. its glow adds onto what's
 * behind it, and the piece itself blends by its alpha */
#define PIECE_MATERIAL                                                         \
    (NJD_FLAG_USE_FLAT | NJD_FLAG_DOUBLE_SIDE | NJD_FLAG_USE_ENV |             \
     NJD_FLAG_USE_TEXTURE | NJD_FLAG_USE_ALPHA | NJD_FLAG_CLAMP_U |            \
     NJD_FLAG_CLAMP_V | NJD_FILTER_BILINEAR | NJD_D_100)
#define PIECE_GLOWING (NJD_SA_SRC | NJD_DA_ONE | PIECE_MATERIAL)
#define PIECE_SOLID (NJD_SA_SRC | NJD_DA_INV_SRC | PIECE_MATERIAL)

/* the taken piece's work mode */
enum taken_mode { TAKEN_START, TAKEN_RISE, TAKEN_FLY };

struct piece_vars {
    NJS_OBJECT *object; /* emerald_piece_object, which nothing reads here */
};

/* a glint's vars: the frame it draws, which gains 16/15 a step. its scale
 * is in scl.x */
struct glint_vars {
    Float frame;
};

/* the blink's vars: the phase of its blinking, which gains blink_speed a
 * step */
struct blink_vars {
    Uint32 phase;
};

#define VARS(work) ((struct piece_vars *)&(work)->vars)
#define GLINT(work) ((struct glint_vars *)&(work)->vars)
#define BLINK(work) ((struct blink_vars *)&(work)->vars)

static void piece_step(struct task *task);
static void piece_destroy(struct task *task);
static void blink_draw(struct task *task);

static NJS_TEXNAME eme_kiran_textures[] = {
    {TEXT_1, 0, 0},
    {TEXT_2, 0, 0},
    {TEXT_3, 0, 0},
    {TEXT_4, 0, 0},
};
NJS_TEXLIST eme_kiran_texlist = {
#include "object/piece/eme_kiran_texlist.inc"
};
static NJS_TEXANIM piece_glint_texanims[4] = {
#include "object/piece/piece_glint_texanims.inc"
};
static NJS_SPRITE piece_glint_sprite = {
#include "object/piece/piece_glint_sprite.inc"
};

/* the red triangle of a mark, in the set editor */
static NJS_POINT3 piece_marker_points[3] = {
#include "object/piece/piece_marker_points.inc"
};
static NJS_COLOR piece_marker_colors[3] = {
#include "object/piece/piece_marker_colors.inc"
};
static NJS_POINT3COL piece_marker = {
#include "object/piece/piece_marker.inc"
};

/* the blink is EFF_REGULAR's hit texture cut in two: the top, and the
 * bottom, which drops away from it a notch at a time */
static NJS_TEXLIST blink_texlist = {
#include "object/piece/blink_texlist.inc"
};
static NJS_TEXANIM blink_texanims[2] = {
#include "object/piece/blink_texanims.inc"
};
static NJS_SPRITE blink_sprite = {
#include "object/piece/blink_sprite.inc"
};
/* the phase the blink turns a step. a whole turn is 0x10000, which takes
 * just over 60 steps */
static Uint32 blink_speed = 0x444;
/* how many times faster than the phase a pulse turns. blink_draw works
 * the pulse out and never uses it */
static Sint32 blink_pulse = 4;
static Sint32 dat_8c1d03b8 = -5461;
/* the blink's scale for each unit of its depth, which keeps its size on
 * the screen */
static Float blink_size = 0.01F;
/* the phase the blink turns for each notch its bottom drops */
static Uint32 blink_drop_phase = 0x1000;
/* @hack type: the blink's color as four Floats, which
 * njSetConstantMaterial reads as one NJS_ARGB from blink_alpha. blink_draw
 * loads each Float's own address from the pool (MOV.L at 0x8c05ecb0 to
 * 0x8c05ecb8). ISO C leaves the read past blink_alpha undefined, but SHC
 * lays the four out in a row. as one NJS_ARGB, 154 of blink_draw's 156
 * words differ */
#ifdef GCC
/* keeps the read the tag names: GCC's four are the members of one
 * NJS_ARGB */
static NJS_ARGB blink_color = {1.0F, 0.0F, 0.0F, 0.0F};
#define blink_alpha blink_color.a
#define blink_red blink_color.r
#define blink_green blink_color.g
#define blink_blue blink_color.b
#else
static Float blink_alpha = 1.0F;
static Float blink_red = 0.0F;
static Float blink_green = 0.0F;
static Float blink_blue = 0.0F;
#endif

/* a glint draws at the scale in its scl.x, frames 1 to 3 on its first
 * three steps. its fourth step kills it */
static void piece_glint_step(struct task *task)
{
    struct task_work *work = task->work;
    Float scale;

    if ((GLINT(work)->frame += 16.0F / 15.0F) >= 4.0F) {
        task_kill(task);
#ifdef GCC
        /* repairs the read the tag below names: on the step that kills the
         * glint, GCC draws nothing, as the image does with a texanim of
         * size 0 */
        return;
#endif
    }
    draw_set_material(0.5F, 1.0F, 1.0F, 1.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    scale = work->scl.x;
    njScale(NULL, scale, scale, scale);
    /* @bug memory: on its fourth step the glint's frame is 4.27, so
     * after task_kill it draws texanim 4 of piece_glint_texanims' four,
     * which reads piece_glint_sprite's bytes (FTRC at 0x8c05e444,
     * njDrawSprite3D at 0x8c05e44a) */
    njDrawSprite3D(&piece_glint_sprite, (Uint32)GLINT(work)->frame,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
    njPopMatrix(1);
    draw_set_material(1.0F, 1.0F, 1.0F, 1.0F);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_BOTHSRCALPHA);
    placement_try_leave(task);
}

#ifdef GCC
/* draw a piece at pos, turned by ang, and its glow, as piece_draw does */
static void piece_draw_still(NJS_POINT3 *pos, Angle ang)
{
    NJS_OBJECT *piece = &emerald_piece_object;
    Float glow;
    Float scale;
    int i;

    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, pos);
    TURN(njRotateY, ang);
    njScale(NULL, 2.0F, 2.0F, 2.0F);
    draw_attr_save();
    draw_attr_clear(0, NJD_FLAG_IGNORE_SPECULAR);
    njSimpleDrawObject(piece);
    draw_attr_restore();
    draw_set_material(0.2F, 1.0F, 1.0F, 1.0F);
    glow = (njSin(ang << 2) + 1.0F) * 0.2F;
    scale = 1.0F + (glow * 0.3F);
    piece->model->mats->attrflags = PIECE_GLOWING;
    piece->model->mats->diffuse.argb.a = glow * (255.0F * 0.1F);
    for (i = 0; i < 4; i++) {
        njScale(NULL, scale, scale, scale);
        njSimpleDrawObject(piece);
    }
    piece->model->mats->attrflags = PIECE_SOLID;
    piece->model->mats->diffuse.argb.a = 204;
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
}
#endif

/* on a display frame's first step, draw a piece at pos and advance its
 * spin, *ang. four larger copies, added onto what's behind it, make a
 * glow that pulses with the spin. a tenth of the draws, at random, start a
 * glint on the camera's side of the piece. the glint starts 5 from the
 * piece across, and 1 to 6 above it */
static void piece_draw(NJS_POINT3 *pos, Angle *ang)
{
    struct task *task;
    NJS_OBJECT *piece;
    NJS_VECTOR offset;
    Float dx;
    Float dz;
    Float glow;
    Float scale;
    Angle way;
    int i;

    if (frame_step != 0) {
#ifdef GCC
        /* no callback draws the piece for the second picture's pass, so step 1
         * draws it for that picture where port.h's FRAME_DRAWS says, as step 0
         * left its spin, and starts no glint. where it says not, the piece
         * shows in the first picture alone */
        if (FRAME_DRAWS()) {
            present_untimed(TRUE);
            piece_draw_still(pos, *ang);
            present_untimed(FALSE);
        }
#endif
        return;
    }
    texture_set_regular();
    njPushMatrix(NULL);
    njTranslateV(NULL, pos);
    /* TURN reads its angle once to test it and again to rotate by it, so
     * the add runs twice. the spin advances 0x200 a draw, or 0x100 when the
     * first add leaves it at 0, which skips the turn */
    TURN(njRotateY, *ang += 0x100);
    njScale(NULL, 2.0F, 2.0F, 2.0F);
    draw_attr_save();
    draw_attr_clear(0, NJD_FLAG_IGNORE_SPECULAR);
    /* @hack spelling: piece assigned inside the call, which loads
     * emerald_piece_object's address into R14 and njSimpleDrawObject's into
     * R13 (0x8c05e530, 0x8c05e532); assigned apart, the two trade and 2 of
     * 271 words differ */
    njSimpleDrawObject(piece = &emerald_piece_object);
    draw_attr_restore();
    draw_set_material(0.2F, 1.0F, 1.0F, 1.0F);
    glow = (njSin(*ang << 2) + 1.0F) * 0.2F;
    scale = 1.0F + (glow * 0.3F);
    piece->model->mats->attrflags = PIECE_GLOWING;
    piece->model->mats->diffuse.argb.a = glow * (255.0F * 0.1F);
    for (i = 0; i < 4; i++) {
        njScale(NULL, scale, scale, scale);
        njSimpleDrawObject(piece);
    }
    piece->model->mats->attrflags = PIECE_SOLID;
    piece->model->mats->diffuse.argb.a = 204;
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    njPopMatrix(1);
    if (rand() * RAND_SCALE < 0.1F) {
        if ((task = task_create(TASK_WORK, 6, piece_glint_step)) != NULL) {
            way = RANDOM_ANGLE();
            offset.x = njCos(way) * 5.0F;
            offset.y = RANDOM(1.0F, 6.0F);
            offset.z = njSin(way) * 5.0F;
            dx = camera_work->pos.x - pos->x;
            dz = camera_work->pos.z - pos->z;
            if ((dx * offset.x) + (dz * offset.z) < 0.0F) {
                offset.x *= -1.0F;
                offset.z *= -1.0F;
            }
            task->work->pos = *pos;
            task->work->pos.x += offset.x;
            task->work->pos.y += offset.y;
            task->work->pos.z += offset.z;
            task->work->scl.x = RANDOM(0.5F, 1.0F);
        }
    }
}

static void piece_draw_marker(NJS_POINT3 *pos)
{
    njPushMatrix(NULL);
    njTranslateV(NULL, pos);
    njDrawPolygon3D(&piece_marker, 3, NJD_FILL);
    njPopMatrix(1);
}

void piece_start(struct task *task)
{
    struct task_work *work = task->work;

    if (edit_mode == 0) {
        if (PIECE_KIND(work) == PIECE_PLACE) {
            if (!knuckles_find_piece(PIECE_ID(work), &work->pos)) {
                placement_kill(task);
                return;
            }
        } else {
            knuckles_mark_piece(PIECE_ID(work), &work->pos);
            placement_kill(task);
            return;
        }
    }
    VARS(work)->object = &emerald_piece_object;
    task->destructor = piece_destroy;
    task->step = piece_step;
}

/* Knuckles takes the piece as DIG_REACH and TAKE_REACH say. a buried piece
 * isn't drawn. the place ends once its piece is taken, or when the piece
 * isn't one Knuckles has yet to take. it ends too when there is no player
 * 0, or the character isn't Knuckles. in the set editor, a place whose id
 * or kind is out of range blinks, 16 steps on and 16 off */
static void piece_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player = player_work[0];
    NJS_POINT3 middle;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode != 0) {
        if (!(++TASK_TIMER(work) & 16) ||
            ((Uint8)PIECE_ID(work) >= PIECE_ID_FIRST &&
             (Uint8)PIECE_ID(work) <= PIECE_ID_LAST &&
             PIECE_KIND(work) <= PIECE_MARK)) {
            piece_draw(&work->pos, &work->ang.y);
            if (PIECE_KIND(work) == PIECE_MARK) {
                piece_draw_marker(&work->pos);
            }
        }
        return;
    } else if (get_character() != CHARACTER_KNUCKLES || player == NULL ||
               !knuckles_find_piece(PIECE_ID(work), &work->pos)) {
        placement_kill(task);
        return;
    }
    if ((PIECE_ID(work) & 0xf0) == PIECE_BURIED) {
        if (njDistanceP2P(&work->pos, &player->pos) < DIG_REACH &&
            knuckles_get_activity(0) == KNUCKLES_DIGGING_UP) {
            knuckles_take_piece(PIECE_ID(work), &work->pos);
            placement_kill(task);
            return;
        }
    } else {
        middle = player->pos;
        middle.y += 5.0F;
        if (njDistanceP2P(&work->pos, &middle) < TAKE_REACH) {
            knuckles_take_piece(PIECE_ID(work), &work->pos);
            placement_kill(task);
            return;
        }
    }
    if ((PIECE_ID(work) & 0xf0) != PIECE_BURIED) {
        piece_draw(&work->pos, &work->ang.y);
    }
}

static void piece_destroy(struct task *task)
{
}

/* the taken piece rises, slowing, then flies to TAKEN_ABOVE above
 * Knuckles. scl holds its rise's speed in y, then its offset from that
 * point. it ends once there is no player 0, or player 0 isn't Knuckles */
void piece_taken_step(struct task *task)
{
    struct task_work *player = player_work[0];
    struct task_work *work = task->work;
    NJS_POINT3 pos;
    NJS_VECTOR way;

    if (player == NULL ||
        PLAYER_VARS(player)->character != CHARACTER_KNUCKLES) {
        task_kill(task);
        return;
    }
    switch (work->mode) {
    case TAKEN_START:
        work->scl.x = 0.0F;
        work->scl.y = 2.0F;
        work->scl.z = 0.0F;
        work->mode = TAKEN_RISE;
        break;
    case TAKEN_RISE:
        /* @hack spelling: -0.1F added, which keeps the image's FADD and its
         * -0.1F (0x8c05ea34); as -= 0.1F, SHC writes an FSUB and 1 of 234
         * words differ */
        if ((work->scl.y += -0.1F) <= 0.0F) {
            work->scl.x = work->pos.x - player->pos.x;
            work->scl.y = work->pos.y - player->pos.y - TAKEN_ABOVE;
            work->scl.z = work->pos.z - player->pos.z;
            work->mode = TAKEN_FLY;
            break;
        }
        work->pos.y += work->scl.y;
        break;
    case TAKEN_FLY:
        work->scl.x *= TAKEN_KEEP;
        work->scl.y *= TAKEN_KEEP;
        work->scl.z *= TAKEN_KEEP;
        work->pos.x = player->pos.x + work->scl.x;
        work->pos.y = player->pos.y + work->scl.y + TAKEN_ABOVE;
        work->pos.z = player->pos.z + work->scl.z;
        TASK_TIMER(work)++;
        if (TASK_TIMER(work) > TAKEN_TIME) {
            task_kill(task);
            return;
        } else if (TASK_TIMER(work) > TAKEN_SPARKLE_TIME &&
                   (TASK_TIMER(work) & 2)) {
            way.x = RANDOM(-0.5F, 0.5F);
            way.y = RANDOM(-0.5F, 0.5F);
            way.z = RANDOM(-0.5F, 0.5F);
            if (njUnitVector(&way) == 0.0F) {
                way.x = way.y = way.z = 0.0F;
            }
            pos = work->pos;
            pos.x -= way.x * 5.0F;
            pos.y -= way.y * 5.0F;
            pos.z -= way.z * 5.0F;
            sparkle_create(&pos, &way, 0.08F, NJM_DEG_ANG(5));
            return;
        }
        break;
    }
    piece_draw(&work->pos, &work->ang.y);
}

/* the blink faces the camera at a scale that keeps its size on the screen,
 * and draws only at a depth between 1 and 500. its top is red while
 * |sin(phase)| is at least 0.5, and yellow otherwise. its bottom is yellow.
 * in turn, it draws 1, 2 and 3 notches below the top, then not at all */
static void blink_draw(struct task *task)
{
    struct task_work *work = task->work;
    NJS_POINT3 screen;
    Float size;
    Float green;
    int drop;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njPushMatrix(NULL);
    njCalcPoint(NULL, &work->pos, &screen);
    if (screen.z > 1.0F && screen.z < 500.0F) {
        size = screen.z * blink_size;
        blink_sprite.sx = blink_sprite.sy = size;
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, camera_work->ang.y);
        TURN(njRotateX, camera_work->ang.x);
        blink_sprite.p.z = 10.0F;
        blink_sprite.p.y = size * 13.75F;
        green = (njAbs(njSin(BLINK(work)->phase)) >= 0.5F) ? 0.0F : 1.0F;
        blink_red = 1.0F;
        blink_green = green;
        blink_blue = 0.0F;
        njSetConstantMaterial((NJS_ARGB *)&blink_alpha);
        njDrawSprite3D(&blink_sprite, 0,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE | NJD_SPRITE_COLOR);
        drop = (BLINK(work)->phase / blink_drop_phase) & 3;
        if (drop > 0) {
            blink_sprite.p.y -= drop * size * 3.75F;
            /* @hack redundant: the pulse's sine, made absolute and stored in
             * green, which nothing reads (FABS FR0 at 0x8c05ed10); as a bare
             * call, the function runs 2 bytes short and 23 of 156 words
             * differ */
            green = njAbs(njSin(BLINK(work)->phase * blink_pulse));
            blink_red = 1.0F;
            blink_green = 1.0F;
            blink_blue = 0.0F;
            njSetConstantMaterial((NJS_ARGB *)&blink_alpha);
            njDrawSprite3D(&blink_sprite, 1,
                           NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE |
                               NJD_SPRITE_COLOR);
        }
        njPopMatrix(1);
    }
}

static void blink_step(struct task *task)
{
    struct task_work *work = task->work;

    BLINK(work)->phase += blink_speed;
    blink_draw(task);
}

void target_blink_start(struct task *task)
{
    task->step = blink_step;
    task->draw = blink_draw;
}
