/* Chaos 2's flash and ball sprites, and the fade of the land's objects
 * that stand in the camera's way */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos2/effects.h"
#include "b_chaos2/land.h"
#include "b_chaos2/textures.h"
#include "camera/camera.h"
#include "collision/world.h"
#include "draw/tree.h"
#include "player/player.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* the objects of chaos2_land's entries that fade, in generated data */
extern NJS_OBJECT dat_0c96d308;
extern NJS_OBJECT dat_0c971bb4;
extern NJS_OBJECT dat_0c971be8;
extern NJS_OBJECT dat_0c971c1c;
extern NJS_OBJECT dat_0c971c50;
extern NJS_OBJECT dat_0c971c84;
extern NJS_OBJECT dat_0c971cb8;
extern NJS_OBJECT dat_0c971cec;
extern NJS_OBJECT dat_0c971d20;
extern NJS_OBJECT dat_0c971d54;
extern NJS_OBJECT dat_0c971d88;
extern NJS_OBJECT dat_0c971dbc;
extern NJS_OBJECT dat_0c971df0;
extern NJS_OBJECT dat_0c971e24;
extern NJS_OBJECT dat_0c971e58;
extern NJS_OBJECT dat_0c971e8c;

/* the flash plays its sprite's FLASH_FRAMES frames over FLASH_STEPS
 * steps, and ends once its frame reaches FLASH_FRAMES */
#define FLASH_FRAMES 8.0F
#define FLASH_STEPS 30.0F
#define FLASH_PACE (FLASH_FRAMES / FLASH_STEPS)

/* the flash's work->mode */
enum flash_mode { FLASH_START, FLASH_RUN };

/* FADE_OBJECTS of the land's objects fade. each draw moves an object's
 * alpha by FADE_PACE, and keeps it from FADE_LEAST to 1 */
#define FADE_OBJECTS 16
#define FADE_LEAST 0.5F
#define FADE_PACE 0.1F
/* an object is in the camera's way within FADE_NEAR of the camera. farther
 * off, it is in the way while it is nearer the camera than player 0 and the
 * screen test holds. the test asks whether the two screen xs sum to less
 * than the two sizes on the screen. a size on the screen is
 * _nj_screen_.dist times the size over the distance from the camera.
 * FADE_PLAYER_SIZE is player 0's size and FADE_SIZE an object's, in world
 * units */
#define FADE_NEAR 15.0F
#define FADE_PLAYER_SIZE 5.0F
#define FADE_SIZE 15.0F

/* the frame the flash or the ball sprite shows. chaos2_ball_sprite_draw
 * takes a drop's work, whose vars, ball.c's struct drop_vars, start with
 * the same Float */
struct sprite_vars {
    Float frame;
};

#define VARS(work) ((struct sprite_vars *)&(work)->vars)

/* the flash's frames, c2_flash01 to 08 of chaos2_effect_texlist, and the
 * ball sprite's, c2_ball01 to 08 */
static NJS_TEXANIM flash_frames[8] = {
#include "b_chaos2/effects/flash_frames.inc"
};
static NJS_SPRITE flash_sprite = {
#include "b_chaos2/effects/flash_sprite.inc"
};
static NJS_TEXANIM ball_frames[8] = {
#include "b_chaos2/effects/ball_frames.inc"
};
static NJS_SPRITE ball_sprite = {
#include "b_chaos2/effects/ball_sprite.inc"
};
/* the constant materials the two draws set. neither reaches its sprite:
 * njDrawSprite3D colors a sprite white unless NJD_SPRITE_COLOR is set (TST
 * at 0x8c6237d4 in 1ST_READ.BIN) */
static NJS_ARGB flash_material = {0.5F, 0.0F, 0.0F, 0.0F};
static NJS_ARGB ball_sprite_material = {1.0F, 0.0F, 0.0F, 0.0F};

/* each must be the object of one of chaos2_land's entries. the draw reads
 * through its entry, which chaos2_land_fade_step sets only for an object
 * the land has */
static NJS_OBJECT *fade_objects[FADE_OBJECTS] = {
    &dat_0c96d308, &dat_0c971bb4, &dat_0c971be8, &dat_0c971c1c,
    &dat_0c971c50, &dat_0c971c84, &dat_0c971cb8, &dat_0c971cec,
    &dat_0c971d20, &dat_0c971d54, &dat_0c971d88, &dat_0c971dbc,
    &dat_0c971df0, &dat_0c971e24, &dat_0c971e58, &dat_0c971e8c,
};

Sint8 chaos2_land_fade_on;
/* by fade_objects' order: each object's entry in chaos2_land, and the
 * alpha the draw gives it */
static struct land_entry *fade_entries[FADE_OBJECTS];
static Float fade_alphas[FADE_OBJECTS];

/* draw the flash at the work's pos, added to what lies behind it */
static void flash_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetConstantMaterial(&flash_material);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    njDrawSprite3D(&flash_sprite, (int)VARS(work)->frame,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE);
    njPopMatrix(1);
}

void chaos2_flash_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case FLASH_START:
        VARS(work)->frame = 0.0F;
        task->draw = flash_draw;
        work->mode = FLASH_RUN;
        /* fall through */
    case FLASH_RUN:
        VARS(work)->frame += FLASH_PACE;
        if (VARS(work)->frame >= FLASH_FRAMES) {
            task_destroy(task);
            return;
        }
        break;
    }
    flash_draw(task);
}

void chaos2_ball_sprite_draw(struct task_work *work)
{
    Angle ang;

    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_ONE);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
    njSetConstantMaterial(&ball_sprite_material);
    njPushMatrix(NULL);
    TURN_VIA(njRotateY, camera_work->ang.y, ang);
    TURN_VIA(njRotateX, camera_work->ang.x, ang);
    njDrawSprite3D(&ball_sprite, (int)VARS(work)->frame,
                   NJD_SPRITE_ALPHA | NJD_SPRITE_SCALE);
    njPopMatrix(1);
}

/* fade each of fade_objects toward FADE_LEAST while chaos2_land_fade_on is
 * set and the object is in the camera's way, and back toward 1 otherwise.
 * then draw it at its alpha through the constant material */
static void land_fade_draw(struct task *task)
{
    struct task_work *player = player_work[0];
    NJS_ARGB color = {0.0F, 0.0F, 0.0F, 0.0F};
    struct land_entry **next;
    struct land_entry *entry;
    Float player_distance;
    Float entry_distance;
    Float change;
    NJS_POINT2 player_screen;
    NJS_POINT2 entry_screen;
    NJS_POINT3 *center;
    int i;

    next = fade_entries;
    njSetTexture(&chaos2_land_texlist);
    draw_control_save();
    draw_control_set(NJD_CONTROL_3D_CONSTANT_MATERIAL);
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    for (i = 0; i < FADE_OBJECTS; i++) {
        change = FADE_PACE;
        entry = *next++;
        if (chaos2_land_fade_on) {
            player_distance = njDistanceP2P(&camera_work->pos, &player->pos);
            center = &entry->center;
            entry_distance = njDistanceP2P(&camera_work->pos, center);
            if (entry_distance < FADE_NEAR) {
                change = -FADE_PACE;
            } else if (player_distance > entry_distance) {
                Float player_size;
                Float entry_size;
                Float sum;

                njProjectScreen(NULL, &player->pos, &player_screen);
                njProjectScreen(NULL, center, &entry_screen);
                player_size =
                    _nj_screen_.dist * FADE_PLAYER_SIZE / player_distance;
                entry_size = _nj_screen_.dist * FADE_SIZE / entry_distance;
                /* @bug logic: the sum takes each screen x as signed
                 * about the screen's middle, as its FABSs show (0x0c90621a
                 * and 0x0c90621e), but njProjectScreen measures x from the
                 * left edge (FADD of _nj_screen_.cx at 0x8c628aec in
                 * 1ST_READ.BIN). so it sums the two distances from that
                 * edge, and the test holds less often the farther right
                 * the two stand on the screen */
                sum = _builtin_fabsf(player_screen.x) +
                      _builtin_fabsf(entry_screen.x);
                if (sum < player_size + entry_size) {
                    change = -FADE_PACE;
                }
            }
        }
        fade_alphas[i] += change;
        fade_alphas[i] = CLAMP(fade_alphas[i], FADE_LEAST, 1.0F);
        color.a = fade_alphas[i];
        njSetConstantMaterial(&color);
        draw_tree(entry->object);
    }
    color.a = 1.0F;
    njSetConstantMaterial(&color);
    draw_attr_restore();
    draw_control_restore();
}

static void land_fade_run(struct task *task)
{
    if (!frame_step) {
        land_fade_draw(task);
    }
}

void chaos2_land_fade_step(struct task *task)
{
    int i;
    int j;
    int count = chaos2_land.count;
    struct land_entry *entry;
    NJS_OBJECT *object;

    for (i = 0; i < FADE_OBJECTS; i++) {
        object = fade_objects[i];
        entry = chaos2_land.entries;
        fade_alphas[i] = 1.0F;
        for (j = 0; j < count; j++, entry++) {
            if (entry->object == object) {
                fade_entries[i] = entry;
                break;
            }
        }
    }
    task->draw = land_fade_draw;
    task->step = land_fade_run;
    chaos2_land_fade_on = 0;
}
