/* EV0003, Sonic's story: in Station Square, Sonic sees Tails' plane trail smoke
 * and fall. he runs off after it */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/draw.h"
#include "effect/dust.h"
#include "ev0003/plane_textures.h"
#include "ev0003/script.h"
#include "event/event.h"
#include "event/play.h"
#include "player/textures.h"
#include "shared/fade.h"
#include "shared/sound.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 5
#define TEXT_2_LENGTH 11
#define TEXT_3_LENGTH 33
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 25
#define TEXT_6_LENGTH 16
#define TEXT_7_LENGTH 1
#define TEXT_8_LENGTH 3
#define TEXT_9_LENGTH 2
#define TEXT_10_LENGTH 21
#define TEXT_11_LENGTH 1
#define TEXT_12_LENGTH 5
#include "ev0003/script/text.inc"

/* the first of the four points of the last curve the plane flies. that
 * curve runs from ev0003_flight[52] to ev0003_flight[53], the flight's last
 * point */
#define LAST_CURVE 51

/* plane_work.mode. the plane flies along ev0003_flight, and is down past
 * its end */
enum plane_mode { PLANE_FLYING, PLANE_DOWN };

/* the plane's task->work, laid over struct task_work. mode, timer, ang and pos
 * keep their offsets there, and the camera reads ang and pos. vars holds
 * tails_frame, place and plane_frame, and the first 8 bytes of scl hold last */
struct plane_work {
    Sint8 mode;
    Uint8 unk_01[5];
    Uint16 timer;
    Float tails_frame;
    Float place; /* how far along ev0003_flight the plane is, in points */
    Float plane_frame;
    struct angles ang;
    NJS_POINT3 pos;
    NJS_POINT2 last; /* the flight point the plane was at the step before */
};

extern NJS_ACTION sonic_stand_action;
extern NJS_ACTION sonic_jump_land_action;

extern NJS_ACTION ev0003_sonic_action_0;
extern NJS_ACTION ev0003_sonic_action_1;
extern NJS_ACTION ev0003_sonic_action_2;
extern NJS_ACTION ev0003_sonic_action_3;
extern NJS_ACTION ev0003_tails_action;
extern NJS_ACTION ev0003_plane_action;

/* player 0's pad as Sonic runs off after the plane, a key a step. the 42nd to
 * 63rd keys hold A for one jump, and the 68th to 84th for another */
static const struct event_pad_key ev0003_run_keys[120] = {
#include "ev0003/script/ev0003_run_keys.inc"
};

static const struct event_pad_recording ev0003_run = {
    ev0003_run_keys, sizeof(ev0003_run_keys) / sizeof(ev0003_run_keys[0])};

static struct task *ev0003_fade = NULL;

/* the plane's flight in 54 points, as seen from the side. x runs along its
 * course, and y is its height. ev0003_flight_to_world places a point in
 * Station Square */
#ifdef GCC
/* repairs the read the tag in ev0003_plane_step names: GCC's flight
 * repeats its last point, so the last curve reaches that point in the
 * direction from the one before */
static NJS_POINT2 ev0003_flight[55] = {
#include "ev0003/script/ev0003_flight.inc"
    {3.8375F, -0.3136F},
};
#else
static NJS_POINT2 ev0003_flight[54] = {
#include "ev0003/script/ev0003_flight.inc"
};
#endif

/* the puffs of smoke the plane trails. ev0003_plane_step sets rate and pos
 * before each */
static struct dust_params ev0003_smoke = {
    3.0F,
    0.03F,
    0.14F,
    0.06F,
    0.02F,
    {0.0F, 0.0F, 0.0F},
    {0.0F, 0.0F, 0.0F},
    {0.5F, 0.9F, 0.9F, 0.9F},
};

static struct task *ev0003_plane = NULL;

struct texture_pvm ev0003_textures[] = {
    {"EV_S_T2C_BODY", &ev0003_plane_texlist},
    {NULL, NULL},
};

/* the point at along, 0 to 1, of a curve from points[1] to points[2]. the curve
 * blends two lines, each half as long as the segment from points[1] to
 * points[2]. the first leaves points[1] in the direction from points[0] to
 * points[2]. the second reaches points[2] in the direction from points[1] to
 * points[3]. the first line's weight is 1 - (3 - 2 * along) * along * along.
 * the second's is the rest */
static void ev0003_curve(NJS_POINT2 *points, Float along, NJS_POINT2 *out)
{
    Float x1;
    Float y1;
    Float x2;
    Float y2;
    Float dx;
    Float dy;
    Float length_squared;
    Float scale;
    Float blend;
    Float ax;
    Float ay;
    Float bx;
    Float by;

    x1 = points[1].x;
    y1 = points[1].y;
    x2 = points[2].x;
    y2 = points[2].y;
    dx = x2 - x1;
    dy = y2 - y1;
    length_squared = (dx * dx) + (dy * dy);
    ax = x2 - points[0].x;
    ay = y2 - points[0].y;
    scale = njSqrt(length_squared / ((ax * ax) + (ay * ay))) * 0.5F;
    ax *= scale;
    ay *= scale;
    bx = x1 - points[3].x;
    by = y1 - points[3].y;
    scale = njSqrt(length_squared / ((bx * bx) + (by * by))) * 0.5F;
    bx *= scale;
    by *= scale;
    blend = 1.0F + (((along * 2.0F) - 3.0F) * along * along);
    out->x = (blend * (x1 + (along * ax))) +
             ((1.0F - blend) * (x2 + ((1.0F - along) * bx)));
    out->y = (blend * (y1 + (along * ay))) +
             ((1.0F - blend) * (y2 + ((1.0F - along) * by)));
}

/* a point of the flight, placed in Station Square */
static void ev0003_flight_to_world(NJS_POINT2 *point, NJS_POINT3 *out)
{
    out->x = -(point->x * 442.86F) + 115.56F;
    out->y = 64.77F + (443.96F * point->y);
    out->z = -(point->x * 31.22F) + 2249.63F;
}

static void ev0003_plane_destroy(struct task *task)
{
}

/* the plane, and Tails in its seat, each at its action's frame */
static void ev0003_plane_draw(struct task *task)
{
    struct plane_work *work = (struct plane_work *)task->work;
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    njSetTexture(&ev0003_plane_texlist);
    draw_action(&ev0003_plane_action, work->plane_frame);
    njTranslate(NULL, 0.0F, 2.8F, -2.6F);
    njRotateY(NULL, 0x4000);
    njSetTexture(&ev0003_plane_texlist);
    draw_action(&ev0003_tails_action, work->tails_frame);
    njPopMatrix(1);
}

/* fly the plane 0.09 points along ev0003_flight a step, nose along its path,
 * and draw it. from point 8 to 16 it rolls a full turn one way. from 42 to 50
 * it rolls a full turn the other way. it trails a puff of smoke each step, and
 * the puffs play faster from point 22 to 35. once place reaches LAST_CURVE + 1,
 * the end of the last curve, the plane is down. from then on, the step does
 * nothing */
static void ev0003_plane_step(struct task *task)
{
    struct plane_work *work = (struct plane_work *)task->work;
    Float last_x = work->last.x;
    Float last_y = work->last.y;
    Float plane_frame = work->plane_frame;
    Float tails_frame = work->tails_frame;
    Float place = work->place;
    Float whole;
    Float along;
    int i;
    NJS_POINT2 point;
    Angle ang_x;
    Angle ang_z;

    if (work->mode == PLANE_DOWN) {
        return;
    }
    plane_frame += 1.0F;
    /* @hack spelling: motion read through the action's address. SHC then loads
     * ev0003_plane_action's address from the pool and reads motion at offset 4
     * (0x0cb8029c, 0x0cb802a2). as ev0003_plane_action.motion, SHC loads
     * motion's own address from the pool instead, and 10 of ev0003_plane_step's
     * 207 words differ */
    if (plane_frame > (Float)((&ev0003_plane_action)->motion->nbFrame - 1)) {
        plane_frame = 0.0F;
    }
    /* @hack spelling: the increment written out. SHC then keeps plane_frame in
     * FR13 and tails_frame in FR14 (0x0cb80288, 0x0cb8028c). as
     * tails_frame += 1.0F, plane_frame takes FR14 and tails_frame FR13, and
     * 10 of ev0003_plane_step's 207 words differ */
    tails_frame = tails_frame + 1.0F;
    /* @hack spelling: motion read through the action's address. SHC then loads
     * ev0003_tails_action's address from the pool and reads motion at offset 4
     * (0x0cb802c2, 0x0cb802c4). as ev0003_tails_action.motion, SHC loads
     * motion's own address from the pool instead, and 5 of ev0003_plane_step's
     * 207 words differ */
    if (tails_frame > (Float)((&ev0003_tails_action)->motion->nbFrame - 1)) {
        tails_frame = 0.0F;
    }
    place += 0.09F;
    whole = floorf(place);
    /* @hack redundant: i's address, taken and never used. SHC then keeps i in
     * the frame at @R15 (MOV.L R2,@R15 at 0x0cb802f8). without it,
     * ev0003_plane_step runs 4 bytes long, and 205 of its 207 words differ */
    (void)&i;
    i = whole;
    /* @hack type: i compared as a Uint32, though place never falls below 0.
     * SHC then tests it with CMP/HI (0x0cb802f4). as an int, SHC tests it
     * with CMP/GT, and 1 of ev0003_plane_step's 207 words differs */
    if ((Uint32)i > LAST_CURVE) {
        work->mode = PLANE_DOWN;
        return;
    }
    along = place - whole;
    /* @bug memory: the curve reads four points from ev0003_flight[i].
     * at i of LAST_CURVE it reads ev0003_flight[54], one past the flight.
     * there lie ev0003_smoke's scale and growth (FMOV.S @R4,FR2 at
     * 0x0cb800a2, ev0003_smoke at 0x0cb81b14) */
    ev0003_curve(&ev0003_flight[i], along, &point);
    ev0003_flight_to_world(&point, &work->pos);
    ang_x =
        -(Angle)(atan2f(point.y - last_y, point.x - last_x) * ANGLE_PER_RADIAN);
    last_x = point.x;
    last_y = point.y;
    if (place > 8.0F && place < 16.0F) {
        ang_z = (place - 8.0F) * 8192.0F;
    } else if (place > 42.0F && place < 50.0F) {
        ang_z = -((place - 42.0F) * 8192.0F);
    } else {
        ang_z = 0;
    }
    work->ang.x = ang_x;
    work->ang.z = ang_z;
    work->last.x = last_x;
    work->last.y = last_y;
    work->tails_frame = tails_frame;
    work->plane_frame = plane_frame;
    work->place = place;
    ev0003_plane_draw(task);
    if (++work->timer == 1) {
        work->timer = 0;
        if (place > 22.0F && place < 35.0F) {
            ev0003_smoke.rate = 0.52F;
        } else {
            ev0003_smoke.rate = 0.14F;
        }
        ev0003_smoke.pos = work->pos;
        dust_create_from(&ev0003_smoke);
    }
}

static void ev0003_plane_init(struct task *task)
{
    struct plane_work *work = (struct plane_work *)task->work;

    work->timer = 0;
    work->plane_frame = 0.0F;
    work->place = 0.0F;
    work->tails_frame = 0.0F;
    work->ang.y = 0xbd22;
    work->last.x = 0.0F;
    work->last.y = 0.0F;
    work->mode = PLANE_FLYING;
    task->step = ev0003_plane_step;
    task->draw = ev0003_plane_draw;
    task->destructor = ev0003_plane_destroy;
}

void ev0003_script(int phase)
{
    switch (phase) {
    case EVENT_LOAD:
        break;
    case EVENT_PLAY:
        event_camera_take();
        event_pads_off();
        event_take_control();
        event_reset_player(0);
        sound_load_file("e_0003.mlt");
        event_sound_init(3);
        stream_request_music(MUSIC_S_SQUARE);
        ev0003_fade = event_fade_create(1.0F, 0.1F, 0.0F, 0.0F, 0.0F);
        event_look_clear(event_get_player(0));
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x438e);
        event_set_pos(event_get_player(0), -225.232F, 0.3F, 1813.276F);
        event_set_ang(event_get_player(0), 0x01f9, 0x019e, 0);
        event_play_action(event_get_player(0), &ev0003_sonic_action_1,
                          &sonic_texlist, 0.7F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0ce0, 0x7274, 0xfc00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -251.9F, 34.126F, 1907.423F);
        event_wait(20);
        /* @hack spelling: the test and the call as one expression, as
         * src/event/play.h's EVENT_KILL has them. SHC then keeps ev0003_plane's
         * address in R9 and event_play_action's in R10 (0x0cb80450,
         * 0x0cb80560). as an if, the two addresses trade registers, and 31 of
         * ev0003_script's 1005 words differ */
        /* NOLINTNEXTLINE(clang-diagnostic-unused-value) */
        ev0003_fade != NULL && (event_fade_set_speed(ev0003_fade, -0.025F), 1);
        event_wait(10);
        event_camera_set_ang(EVENT_EASE_LINEAR, 105, 0x0b00, 0x7300, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 105, -221.1626F, 6.327F,
                             1814.416F);
        event_wait(104);
        event_camera_set_ang(EVENT_EASE_LINEAR, 75, 0x1200, 0x6900, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 75, -218.4F, 4.4662F,
                             1809.5862F);
        event_wait(24);
        event_talk_start(event_get_player(0), TEXT_6);
        event_wait(50);
        event_camera_set_ang(EVENT_EASE_LINEAR, 60, 0x1200, 0x6900, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 60, -219.245F, 4.444F,
                             1809.0613F);
        event_wait(60);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xda00, 0x0d00, 0xfd00);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -222.4F, 13.5F, 1820.92F);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -223.19F, 10.4F, 1818.5F);
        event_wait(40);
        ev0003_plane = task_create(TASK_WORK, 5, ev0003_plane_init);
        event_sound_start(0, 1223, 1800);
        event_sound_fade_volume(0, -60, 1);
        event_talk_start(event_get_player(0), TEXT_7);
        event_wait(1);
        event_sound_fade_volume(0, -10, 100);
        event_wait(29);
        event_play_action(event_get_player(0), &ev0003_sonic_action_2,
                          &sonic_texlist, 0.75F, 0, 0);
        event_wait(10);
        event_wait_motions(event_get_player(0));
        event_play_action(event_get_player(0), &ev0003_sonic_action_3,
                          &sonic_texlist, 0.8F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(0), TEXT_8);
        event_voice_play(933);
        event_play_action(event_get_player(0), &ev0003_sonic_action_3,
                          &sonic_texlist, 0.4F, EVENT_REPEAT, 0);
        stream_stop_music();
        event_message_show(TEXT_1);
        event_message_close();
        event_wait(15);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_set_pos(event_get_player(0), -339.72F, -3.0F, 1867.1F);
        event_set_ang(event_get_player(0), 0, 0x1400, 0);
        event_look_at(event_get_player(0), -287.7F, 65.356F, 1967.818F);
        event_camera_set_fov(EVENT_EASE_SMOOTH, 1, 0x3111);
        stream_request_music(MUSIC_CIRCUIT);
        event_camera_set_ang(EVENT_EASE_LINEAR, 0, 0x0700, 0x9a00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 0, -328.07F, 2.086F, 1830.488F);
        event_sound_fade_pan(0, -128, 1);
        event_camera_set_ang(EVENT_EASE_LINEAR, 76, 0x0d00, 0x8c00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 76, -339.21F, 2.086F,
                             1847.346F);
        event_wait(1);
        event_sound_fade_pan(0, 0, 100);
        event_wait(46);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 100, -343.0376F, 2.086F,
                             1848.5065F);
        event_wait(10);
        event_wait(52);
        event_wait(3);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -170.242F, 80.0F, 2219.456F);
        event_camera_look_at_task(EVENT_EASE_SMOOTH, 1, ev0003_plane, 0.0F,
                                  1.0F, 0.0F, 0);
        event_sound_fade_volume(0, 0, 1);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_talk_start(event_get_player(0), TEXT_9);
        event_camera_follow(ev0003_plane);
        event_voice_play(947);
        event_message_show(TEXT_2);
        event_look_clear(event_get_player(0));
        event_look_at(event_get_player(0), -436.25F, 73.69F, 1988.8666F);
        event_set_ang(event_get_player(0), 0, 0xe600, 0);
        event_wait(33);
        event_camera_follow_clear();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 28, 0x0500, 0x5900, 0);
        event_wait(23);
        event_message_close();
        event_camera_look_clear();
        event_sound_fade_volume(0, -10, 1);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0xfe00, 0xdbf4, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -350.324F, 4.45F, 1875.68F);
        event_camera_set_ang(EVENT_EASE_LINEAR, 50, 0x0f00, 0xdbf4, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 50, -345.15F, 4.54F, 1871.44F);
        event_talk_stop(event_get_player(0));
        event_voice_play(948);
        event_talk_start(event_get_player(0), TEXT_10);
        event_message_show(TEXT_3);
        event_voice_wait();
        event_message_close();
        event_talk_stop(event_get_player(0));
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0c00, 0x5c00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -320.3F, 5.48F, 1854.9F);
        event_sound_fade_volume(0, -70, 120);
        event_camera_set_ang(EVENT_EASE_LINEAR, 110, 0x0c00, 0x5c00, 0);
        event_camera_set_pos(EVENT_EASE_LINEAR, 110, -321.2F, 1.65F, 1855.636F);
        event_wait(105);
        event_look_clear(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_jump_land_action,
                          &sonic_texlist, 0.5F, EVENT_REPEAT, 6);
        event_talk_stop(event_get_player(0));
        event_wait(5);
        event_sound_stop(0);
        vibration_start(0, 4, 50, 10);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1400, 0x5c00, 0);
        event_sound_start(1, 1225, 1800);
        event_sound_fade_volume(1, -20, 1);
        event_wait(2);
        event_talk_start(event_get_player(0), TEXT_11);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x0500, 0x5c00, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1300, 0x5c00, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x0300, 0x5c00, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1100, 0x5c00, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x0700, 0x5c00, 0);
        vibration_start(0, 3, 50, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1100, 0x5c00, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x0900, 0x5c00, 0);
        event_wait(2);
        event_look_clear(event_get_player(0));
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x1000, 0x5c00, 0);
        event_wait(2);
        event_camera_set_ang(EVENT_EASE_LINEAR, 3, 0x0c00, 0x5c00, 0);
        event_sound_stop(1);
        event_voice_play(949);
        event_message_show(TEXT_4);
        event_wait(25);
        event_talk_start(event_get_player(0), TEXT_12);
        event_wait(20);
        event_clear_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 0);
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x1600, 0xd700, 0x0200);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -346.35F, 5.074F, 1869.94F);
        event_message_show(TEXT_5);
        event_camera_set_ang(EVENT_EASE_LINEAR, 100, 0x1800, 0xd300, 0xff00);
        event_camera_set_pos(EVENT_EASE_LINEAR, 100, -345.26F, 3.16F, 1869.93F);
        event_wait(5);
        event_play_action(event_get_player(0), &ev0003_sonic_action_0,
                          &sonic_texlist, 0.2F, 0, 16);
        event_wait_motions(event_get_player(0));
        event_play_action(event_get_player(0), &sonic_stand_action,
                          &sonic_texlist, 1.0F, EVENT_REPEAT, 16);
        event_voice_wait();
        event_talk_stop(event_get_player(0));
        event_message_close();
        event_camera_set_ang(EVENT_EASE_SMOOTH, 0, 0x0200, 0xcc00, 0);
        event_camera_set_pos(EVENT_EASE_SMOOTH, 0, -395.67F, 1.1F, 1872.486F);
        event_clear_motions(event_get_player(0));
        event_camera_look_at_task(EVENT_EASE_LINEAR, 40, event_get_player(0),
                                  0.0F, 0.0F, 0.0F, 0);
        event_play_pad(0, &ev0003_run);
        event_wait(103);
        break;
    case EVENT_END:
        event_set_pos(event_get_player(0), -456.0F, 0.0F, 1963.0F);
        event_set_ang(event_get_player(0), 0, 0xe8d0, 0);
        event_reset_player(0);
        event_camera_release();
        event_pads_on();
        if (ev0003_plane != NULL) {
            task_kill(ev0003_plane);
            ev0003_plane = NULL;
        }
        event_sound_stop_all();
        EVENT_KILL(ev0003_fade);
        break;
    case EVENT_UNLOAD:
        break;
    }
}
