/* Chaos 0's rain. each step, the rain's task makes rain_drops drops ahead
 * of the camera. a drop is a short grey line that falls to the floor below
 * it, where it splashes and fades */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "b_chaos0/rain.h"
#include "camera/camera.h"
#include "chaos/effect.h"
#include "chaos/models.h"
#include "collision/ground.h"
#include "draw/motion.h"
#include "player/player.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 14
#include "b_chaos0/rain/text.inc"

/* a drop starts within DROP_REACH of the camera across, at DROP_ARC_START
 * to DROP_ARC_START + DROP_ARC degrees from the camera's right toward its
 * front. that arc is centred on the way the camera faces */
#define DROP_REACH 150.0F
#define DROP_ARC 120.0F
#define DROP_ARC_START 30.0F
/* a drop starts at y DROP_TOP. it lands DROP_CLEARANCE above the highest
 * floor at or below FLOOR_FROM under it, and never below y DROP_CLEARANCE */
#define DROP_TOP 120.0F
#define FLOOR_FROM 5.0F
#define DROP_CLEARANCE 0.1F
/* while the boss is more than BOSS_ABOVE above player 0, a drop that lands
 * vanishes without a splash */
#define BOSS_ABOVE 60.0F
/* a splash starts at alpha 1 and loses SPLASH_FADE a step */
#define SPLASH_FADE (1.0F / 30.0F)
/* a splash's scale is its alpha times SPLASH_SIZE, so it shrinks as it
 * fades */
#define SPLASH_SIZE 0.25F
/* the pitch that lays a splash flat on the floor */
#define LIE_FLAT 0x4000

struct drop_work {
    Sint8 mode;
    Uint8 unk_01[7];
    Float alpha; /* the splash's alpha */
    Uint8 unk_0c[8];
    struct angles slope; /* the floor's slope, as ground_below writes it */
    NJS_POINT3 pos;
    Float speed; /* the y it moves a step, negative */
    Float floor; /* the y it lands at */
};

enum drop_mode { DROP_START, DROP_FALL, DROP_SPLASH };

static void rain_drop_draw(struct task *task);
static void rain_splash_draw(struct task *task);
static void rain_drop_step(struct task *task);
static void rain_step(struct task *task);

/* the drops the rain's task makes a step */
static int rain_drops = 4;
/* two floats nothing reads */
static Float dat_0c95f774 = 5.0F;
static Float dat_0c95f778 = 2.5F;
/* a drop falls drop_speed a step, and up to drop_speed_spread more, chosen
 * at random as it starts */
static Float drop_speed = 4.0F;
static Float drop_speed_spread = 2.0F;
/* six floats nothing reads */
static Float dat_0c95f784[6] = {0.0F, 2.0F, 2.0F, 0.0F, 10.0F, 10.0F};

/* the rain's own texture, its texlist and texanim, and a sprite of it,
 * which nothing draws */
static NJS_TEXNAME rain_textures[1] = {{TEXT_1, 0, 0}};
static NJS_TEXLIST rain_texlist = {
#include "b_chaos0/rain/rain_texlist.inc"
};
static NJS_TEXANIM rain_texanim[1] = {
#include "b_chaos0/rain/rain_texanim.inc"
};
static NJS_SPRITE rain_sprite = {
#include "b_chaos0/rain/rain_sprite.inc"
};

/* a drop's line: its two points, their colors, and the line that names
 * them, which rain_drop_draw draws at the drop's pos */
static NJS_POINT3 drop_points[2] = {
#include "b_chaos0/rain/drop_points.inc"
};
static NJS_COLOR drop_colors[2] = {
#include "b_chaos0/rain/drop_colors.inc"
};
static NJS_POINT3COL drop_line = {
#include "b_chaos0/rain/drop_line.inc"
};

/* the task that makes the drops, NULL while stopped */
static struct task *rain_task = NULL;

/* the splash's constant material. its red, green and blue are 0, 0 and -1,
 * and rain_splash_draw gives it the drop's alpha */
static NJS_ARGB splash_color = {0.0F, 0.0F, 0.0F, -1.0F};

static void rain_drop_draw(struct task *task)
{
    struct drop_work *work = (struct drop_work *)task->work;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    njDrawLine3D(&drop_line, 1, NJD_DRAW_NORMAL);
    njPopMatrix(1);
}

/* draw chaos_spot_sprite flat where the drop landed, faded and shrunk by
 * the drop's alpha */
static void rain_splash_draw(struct task *task)
{
    struct drop_work *work = (struct drop_work *)task->work;
    Float size;

    splash_color.a = work->alpha;
    njSetConstantMaterial(&splash_color);
    njSetTexture(&chaos_effect_texlist);
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    size = work->alpha * SPLASH_SIZE;
    njRotateX(NULL, LIE_FLAT);
    njScale(NULL, size, size, size);
    njDrawSprite3D(&chaos_spot_sprite, 0, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njPopMatrix(1);
}

static void rain_drop_step(struct task *task)
{
    struct drop_work *work = (struct drop_work *)task->work;
    Float reach;
    Float x;
    Float z;
    Angle ang;

    switch (work->mode) {
    case DROP_START:
        reach = rand() * (DROP_REACH * RAND_SCALE);
        ang = (Angle)((-(rand() * (DROP_ARC * RAND_SCALE)) - DROP_ARC_START) *
                      ANGLE_PER_DEGREE);
        ang -= camera_work->ang.y;
        x = reach * njCos(ang);
        z = reach * njSin(ang);
        work->pos.x = camera_work->pos.x + x;
        work->pos.z = camera_work->pos.z + z;
        work->pos.y = DROP_TOP;
        work->speed = -drop_speed - (drop_speed_spread * rand() * RAND_SCALE);
        work->floor =
            ground_below(work->pos.x, FLOOR_FROM, work->pos.z, &work->slope) +
            DROP_CLEARANCE;
        work->floor =
            work->floor > DROP_CLEARANCE ? work->floor : DROP_CLEARANCE;
        task->draw = rain_drop_draw;
        work->mode = DROP_FALL;
        break;
    case DROP_FALL:
        work->pos.y += work->speed;
        if (work->pos.y < work->floor) {
            if (boss_work->pos.y - player_work[0]->pos.y <= BOSS_ABOVE) {
                work->pos.y = work->floor;
                work->alpha = 1.0F;
                task->draw = rain_splash_draw;
                work->mode = DROP_SPLASH;
            } else {
                task_destroy(task);
                return;
            }
        } else {
            rain_drop_draw(task);
            return;
        }
        break;
    case DROP_SPLASH:
        work->alpha -= SPLASH_FADE;
        if (work->alpha < 0.0F) {
            task_destroy(task);
            return;
        }
        rain_splash_draw(task);
        return;
    }
}

static void rain_step(struct task *task)
{
    int i;

    for (i = 0; i < rain_drops; i++) {
        task_create(TASK_WORK, 3, rain_drop_step);
    }
}

void chaos0_rain_start(void)
{
    rain_task = task_create(TASK_WORK, 2, rain_step);
}

void chaos0_rain_stop(void)
{
    if (rain_task != NULL) {
        task_destroy(rain_task);
        rain_task = NULL;
    }
}
