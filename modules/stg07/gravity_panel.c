/* the gravity panels: O TPANEL, walls to walk on, and O TSWITCH, the floor
 * switch that moves them. a route is a layout of the panels, and the switches
 * and panels share one of four. a press of a switch moves the route on to the
 * next. the panels then slide to their new places over SLIDE_FRAMES frames.
 * each panel's set entry names the routes it is lit on. while player 0 stands
 * in front of a lit panel, gravity tilts toward it */
#include <ninja.h>
#include <sg_xpt.h>
#include <stdio.h>

#include "collision/collision.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "draw/tree.h"
#include "hud/hud.h"
#include "object/placement.h"
#include "player/npc.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg07/gravity_panel.h"
#include "stg07/light.h"
#include "stg07/obj_ruin_textures.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"
#include "vmu/vibration.h"

/* the switch borrows the water switch's model, whose child is the button */
extern NJS_OBJECT water_switch_object;
extern NJS_OBJECT water_switch_button;
extern NJS_MATERIAL water_switch_button_materials[2];
/* the arrow is the panel's child in the model, and the glow is the arrow's.
 * the panel draws each part's model itself, so it can leave the arrow and the
 * glow out */
extern NJS_OBJECT gravity_panel_object;
extern NJS_OBJECT gravity_panel_arrow_object;
extern NJS_OBJECT gravity_panel_glow_object;
extern NJS_MATERIAL gravity_panel_materials[5];
/* the wall each panel copies into the world */
extern NJS_OBJECT gravity_panel_wall_object;

/* a panel keeps its set entry's ang.x in its timer. bit n of it lights the
 * panel on route n. bits 4 to 6 give its row of route_places */
#define ROUTES(work) TASK_TIMER(work)
#define ROUTE_ROW(work) ((ROUTES(work) >> 4) & 7)

/* where a panel stands now: its place on the route, or a point of its
 * slide */
#define PANEL_AT(work) ((work)->scl)

/* the bits of a panel's tick, which its draw reads */
#define PANEL_LIT 0x1 /* it is lit on the route */
#define PANEL_ARROW 0x2 /* it rests lit, and shows its arrow */
#define PANEL_GLOW 0x4 /* the arrow shows its glow. nothing clears this */

struct panel_switch_vars {
    Uint8 unk_00[8];
    Float button; /* its height in the switch's model */
};

struct gravity_panel_vars {
    Uint8 unk_00[8];
    /* its copy of gravity_panel_wall_object in the world, or in the set
     * editor, null */
    NJS_OBJECT *wall;
};

#define SWITCH(work) ((struct panel_switch_vars *)&(work)->vars)
#define PANEL(work) ((struct gravity_panel_vars *)&(work)->vars)

enum panel_switch_mode { SWITCH_UP, SWITCH_DOWN };
/* PANEL_DARK from a panel's start and once it rests unlit, and PANEL_SHOWN
 * once it rests lit */
enum gravity_panel_submode { PANEL_DARK, PANEL_SHOWN };

#define ROUTE_COUNT 4

/* the frames a slide takes, and the frames of one pulse of the glow */
#define SLIDE_FRAMES 60
#define PULSE_FRAMES 240

/* a panel draws only while the sphere of VIEW_RADIUS round its set place is
 * in view */
#define VIEW_RADIUS 80.0F

/* how hard and how fast pad 0 shakes as a panel comes to rest lit */
#define SHAKE_POWER 4
#define SHAKE_FREQ 15

/* a press puts the button down at BUTTON_DOWN. off it, the button rises
 * BUTTON_RISE each step, up to BUTTON_UP */
#define BUTTON_UP 7.0F
#define BUTTON_DOWN 1.0F
#define BUTTON_RISE 0.3F

/* half a panel's width: a route's places are whole panels apart */
#define PANEL_HALF 25.0F

/* the world tests a task's objects only while player 0 or 1 is near it.
 * near is within TEST_REACH past the panel model's radius */
#define TEST_REACH 100.0F

/* a player stands on the button within PRESS_RADIUS of the switch's center,
 * and PRESS_LOW to PRESS_HIGH above it */
#define PRESS_RADIUS 15.0F
#define PRESS_LOW 5.0F
#define PRESS_HIGH 15.0F

/* gravity tilts toward a panel while player 0 stands between FRONT_NEAR and
 * FRONT_FAR in front of it */
#define FRONT_NEAR 4.0F
#define FRONT_FAR 11.0F

#define SE_GRIND 196

/* the set editor draws a panel's routes ROUTES_FRONT in front of its set
 * place */
#define ROUTES_FRONT 5.1F
/* it draws each route's letter LETTER_SCALE times the size of its strokes */
#define LETTER_SCALE 12.0F

/* a panel's wall moves with the panel, so its mesh is made again each step */
#define WALL_FLAGS (WORLD_LARGE | WORLD_MOVES | WORLD_SOLID)

/* the frames since the slide began, or at rest, since the pulse began */
static Uint32 route_frames = 0;
/* 1 while the panels slide */
static Uint8 route_sliding = 0;
/* the route the panels rest on or slide to, from 0 to 3: D, A, B and C */
static Uint8 route = 0;
/* play_time_get's low byte when route_update last ran the route */
static Uint8 route_last_frame = 0;
/* how many switches and panels share the route */
static Uint8 route_users = 0;
/* clang-format off */
static struct collision_shape panel_switch_shapes[2] = {
    {SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 4.0F, 0.0F},
     15.0F, 3.5F, 0.0F, {0}, 0, NULL},
    {SHAPE_CYLINDER, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {0.0F, 7.5F, 0.0F},
     15.0F, 3.5F, 0.0F, {0}, 0, NULL},
};
/* clang-format on */

/* for each row, where a panel stands on each route, across and up, in
 * whole panels */
static const Sint8 route_places[8][ROUTE_COUNT][2] = {
#include "stg07/gravity_panel/route_places.inc"
};

static void route_update(struct task *task);
static void panel_switch_destroy(struct task *task);
static void panel_switch_draw(struct task *task);
static void panel_switch_run(struct task *task);
static void gravity_panel_destroy(struct task *task);
static void gravity_panel_draw(struct task *task);
static void gravity_panel_slide(struct task *task);
static void gravity_panel_rest(struct task *task);
static void gravity_panel_run(struct task *task);
static void gravity_panel_print(struct task *task);
static void route_draw_letter(const NJS_POINT2 (*strokes)[2], int count,
                              Uint32 color, Float scale);
static void route_draw_square(Uint32 color, int outline, int lit);
static void route_translate(Sint8 row, Sint8 on);
static void gravity_panel_draw_routes(struct task *task);
static void gravity_panel_edit(struct task *task);

/* the first task to join starts the route over, at rest on route 0 */
static void route_join(struct task *task)
{
    if (route_users++ == 0) {
        route_frames = 0;
        route_sliding = 0;
        route = 0;
        route_last_frame = play_time_get();
    }
}

/* every call sets whether the world tests the task's objects. the first call in
 * a frame also runs the route. it counts the frame, and ends a slide at
 * SLIDE_FRAMES. at rest, the switch's button glows, its materials ignoring the
 * light. the pulse starts over every PULSE_FRAMES. that call last points
 * gravity straight down. a lit panel's step then tilts it again while player 0
 * stands in front of the panel */
static void route_update(struct task *task)
{
    struct task_work *work = task->work;
    int frame = play_time_get();

    if (player_in_range(&work->pos,
                        gravity_panel_object.model->r + TEST_REACH)) {
        work->flags |= WORLD_TESTED;
    } else {
        work->flags &= ~WORLD_TESTED;
    }
    if (route_last_frame == (Uint8)frame) {
        return;
    }
    route_frames++;
    route_last_frame = frame;
    switch (route_sliding) {
    case 1:
        water_switch_button_materials[0].attrflags &= ~NJD_FLAG_IGNORE_LIGHT;
        water_switch_button_materials[1].attrflags &= ~NJD_FLAG_IGNORE_LIGHT;
        if (route_frames >= SLIDE_FRAMES) {
            route_frames = 0;
            route_sliding = 0;
        }
        break;
    case 0:
        water_switch_button_materials[0].attrflags |= NJD_FLAG_IGNORE_LIGHT;
        water_switch_button_materials[1].attrflags |= NJD_FLAG_IGNORE_LIGHT;
        if (route_frames >= PULSE_FRAMES) {
            route_frames = 0;
        }
        break;
    }
    world_tilt_gravity(0, 0);
}

static void panel_switch_destroy(struct task *task)
{
    route_users--;
}

static void panel_switch_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    water_switch_button.pos[1] = SWITCH(work)->button;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    texture_set_stage();
    draw_tree(&water_switch_object);
    njPopMatrix(1);
}

/* the switch's step. the nearest player presses the button by stepping on
 * it. a press moves the route on, unless the panels still slide */
static void panel_switch_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;
    Float x;
    Float y;
    Float z;
    Float across_squared;

    route_update(task);
    if (placement_try_leave(task)) {
        return;
    }
    player = player_work[player_nearest(&work->pos)];
    x = player->pos.x - work->pos.x;
    y = player->pos.y - work->pos.y;
    z = player->pos.z - work->pos.z;
    across_squared = (x * x) + (z * z);
    if (across_squared < SQUARE(PRESS_RADIUS) && y > PRESS_LOW &&
        y < PRESS_HIGH) {
        if (work->mode != SWITCH_DOWN) {
            work->mode = SWITCH_DOWN;
            sound_play_for(SE_GRIND, (int)task, 1, 0, 30);
            SWITCH(work)->button = BUTTON_DOWN;
            if (route_sliding == 0) {
                route_sliding = 1;
                route = (route + 1) & (ROUTE_COUNT - 1);
                route_frames = 0;
            }
        }
    } else {
        work->mode = SWITCH_UP;
        SWITCH(work)->button += BUTTON_RISE;
        if (SWITCH(work)->button > BUTTON_UP) {
            SWITCH(work)->button = BUTTON_UP;
        }
    }
    panel_switch_draw(task);
    collision_register(work);
}

void panel_switch_step(struct task *task)
{
    struct task_work *work = task->work;

    task->destructor = panel_switch_destroy;
    task->step = panel_switch_run;
    task->draw = panel_switch_draw;
    SWITCH(work)->button = BUTTON_UP;
    route_join(task);
    /* @hack spelling: the empty arm, which puts collision_init's address in R2
     * and its fifth argument, 2, in R3 (0x0c905b66, 0x0c905b64); with
     * !placement_editing, the two trade and 4 of 33 words differ */
    if (placement_editing(task)) {
    } else {
        collision_init(task, panel_switch_shapes, COLLISION_OBJECT, 1, 2);
    }
}

static void gravity_panel_destroy(struct task *task)
{
    struct task_work *work = task->work;

    route_users--;
    if (!placement_editing(task)) {
        world_remove(task, PANEL(work)->wall);
        world_object_free(PANEL(work)->wall);
        npc_set_disabled(FALSE);
    }
}

static void gravity_panel_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, VIEW_RADIUS)) {
        return;
    }
    if (work->tick & PANEL_LIT) {
        gravity_panel_materials[1].attr_texId = 1;
    } else {
        gravity_panel_materials[1].attr_texId = 0;
    }
    njPushMatrix(NULL);
    njTranslateV(NULL, &PANEL_AT(work));
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslateV(NULL, (NJS_VECTOR *)gravity_panel_object.pos);
    njSetTexture(&gravity_panel_texlist);
    draw_model(gravity_panel_object.model);
    if (work->tick & PANEL_ARROW) {
        njPushMatrix(NULL);
        njTranslateV(NULL, (NJS_VECTOR *)gravity_panel_arrow_object.pos);
        njScaleV(NULL, (NJS_VECTOR *)gravity_panel_arrow_object.scl);
        draw_model(gravity_panel_arrow_object.model);
        if (work->tick & PANEL_GLOW) {
            njPushMatrix(NULL);
            njTranslateV(NULL, (NJS_VECTOR *)gravity_panel_glow_object.pos);
            draw_set_material(
                njSin(route_frames * NJM_DEG_ANG(180) / PULSE_FRAMES), 0.0F,
                0.0F, 0.0F);
            draw_model(gravity_panel_glow_object.model);
            draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            njPopMatrix(1);
        }
        njPopMatrix(1);
    }
    njPopMatrix(1);
}

/* move the panel along its slide, from the last route's place to this
 * one's. it hides the arrow */
static void gravity_panel_slide(struct task *task)
{
    struct task_work *work = task->work;
    Sint8 row = ROUTE_ROW(work);
    const Sint8 *from = route_places[row][(route + 3) & (ROUTE_COUNT - 1)];
    const Sint8 *to;
    Float from_x;
    Float from_y;
    Float to_x;
    Float to_y;
    Float cos_y;
    Float sin_y;
    Float done;
    Float left;

    from_x = from[0] * 2 * PANEL_HALF;
    from_y = from[1] * 2 * PANEL_HALF;
    to = route_places[row][route];
    to_x = to[0] * 2 * PANEL_HALF;
    to_y = to[1] * 2 * PANEL_HALF;
    cos_y = njCos(work->ang.y);
    sin_y = njSin(work->ang.y);
    done = route_frames * (1.0F / (SLIDE_FRAMES - 1));
    left = 1.0F - done;
    PANEL_AT(work).x =
        work->pos.x + (cos_y * ((left * from_x) + (to_x * done)));
    PANEL_AT(work).y = work->pos.y + (left * from_y) + (done * to_y);
    PANEL_AT(work).z =
        work->pos.z - (((left * from_x) + (to_x * done)) * sin_y);
    work->tick &= ~PANEL_ARROW;
}

/* hold the panel at its place on the route. as it rests lit in PANEL_DARK,
 * a glow spreads from it and pad 0 shakes */
static void gravity_panel_rest(struct task *task)
{
    struct task_work *work = task->work;
    Sint8 row = ROUTE_ROW(work);
    const Sint8 *at = route_places[row][route];
    Float x;
    Float y;
    Float cos_y;
    Float sin_y;
    struct task *glow;

    x = at[0] * 2 * PANEL_HALF;
    y = at[1] * 2 * PANEL_HALF;
    cos_y = njCos(work->ang.y);
    sin_y = njSin(work->ang.y);
    PANEL_AT(work).x = work->pos.x + (cos_y * x);
    PANEL_AT(work).y = work->pos.y + y;
    PANEL_AT(work).z = work->pos.z - (x * sin_y);
    if (ROUTES(work) & (1 << route)) {
        work->tick |= PANEL_ARROW | PANEL_GLOW;
        if (work->submode == PANEL_DARK) {
            if ((glow = task_create(TASK_WORK, 2, ruin_light_glow_step)) !=
                NULL) {
                glow->work->pos.x = PANEL_AT(work).x;
                glow->work->pos.y = PANEL_AT(work).y;
                glow->work->pos.z = PANEL_AT(work).z;
                glow->work->ang.z = 0;
                glow->work->ang.x = 0;
                glow->work->ang.y = work->ang.y;
                vibration_start(0, SHAKE_POWER, SHAKE_FREQ, 0);
            }
            work->submode = PANEL_SHOWN;
        }
    } else {
        work->tick &= ~PANEL_ARROW;
        work->submode = PANEL_DARK;
    }
}

/* the panel's step: it slides or rests. while it is lit and player 0 stands
 * in front of it, gravity tilts toward it. its wall in the world follows it */
static void gravity_panel_run(struct task *task)
{
    struct task_work *work = task->work;
    NJS_OBJECT *wall = PANEL(work)->wall;
    struct task_work *player;
    Angle ang;
    Float cos_y;
    Float sin_y;
    Float x;
    Float y;
    Float z;
    Float across;
    Float front;

    route_update(task);
    if (placement_try_leave(task)) {
        return;
    }
    switch (route_sliding) {
    case 1:
        gravity_panel_slide(task);
        break;
    case 0:
        gravity_panel_rest(task);
        break;
    }
    if (ROUTES(work) & (1 << route)) {
        work->tick |= PANEL_LIT;
        player = player_work[0];
        ang = work->ang.y;
        cos_y = njCos(ang);
        sin_y = njSin(ang);
        z = player->pos.z - PANEL_AT(work).z;
        x = player->pos.x - PANEL_AT(work).x;
        front = (cos_y * z) + (sin_y * x);
        across = (cos_y * x) - (sin_y * z);
        y = player->pos.y - PANEL_AT(work).y;
        if (across > -PANEL_HALF && across < PANEL_HALF && y > -PANEL_HALF &&
            y < PANEL_HALF && front > FRONT_NEAR && front < FRONT_FAR) {
            world_tilt_gravity(ang + NJM_DEG_ANG(90), NJM_DEG_ANG(90));
        }
    } else {
        work->tick &= ~PANEL_LIT;
    }
    /* @bug memory: in the set editor, gravity_panel_step puts no wall in the
     * world, and wall stays null. the store then writes the panel's place at
     * addresses 8 to 19 (MOV.L R2,@(8,R3) at 0x0c9060a6) */
    /* @hack type: wall->pos, a Float[3], stored in place as an NJS_POINT3;
     * without it, as three element stores, the function runs 32 bytes long and
     * 101 of 135 words differ */
    *(NJS_POINT3 *)wall->pos = PANEL_AT(work);
    gravity_panel_draw(task);
}

/* print the set editor's label. PATH is the panel's row, from 1, and GROUP
 * the letters of its routes */
static void gravity_panel_print(struct task *task)
{
    struct task_work *work = task->work;
    char text[128];

    njPrintSize(16);
    sprintf(text, "<PATH:%1d GROUP:---->", ROUTE_ROW(work) + 1);
    if (ROUTES(work) & 2) {
        text[14] = 'A';
    }
    if (ROUTES(work) & 4) {
        text[15] = 'B';
    }
    if (ROUTES(work) & 8) {
        text[16] = 'C';
    }
    if (ROUTES(work) & 1) {
        text[17] = 'D';
    }
    njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), text);
}

/* each route's letter, in route order: D, A, B and C. a letter is strokes,
 * each a line between two points. the points lie in a square of side 2 round
 * the origin */
static const NJS_POINT2 letter_d[5][2] = {
#include "stg07/gravity_panel/letter_d.inc"
};
static const NJS_POINT2 letter_a[3][2] = {
#include "stg07/gravity_panel/letter_a.inc"
};
static const NJS_POINT2 letter_b[8][2] = {
#include "stg07/gravity_panel/letter_b.inc"
};
static const NJS_POINT2 letter_c[5][2] = {
#include "stg07/gravity_panel/letter_c.inc"
};

static void route_draw_letter(const NJS_POINT2 (*strokes)[2], int count,
                              Uint32 color, Float scale)
{
    NJS_POINT3COL line;
    NJS_COLOR colors[2];
    NJS_POINT3 points[2];
    int i;

    line.p = points;
    line.col = colors;
    line.num = 2;
    colors[0].color = colors[1].color = color;
    points[0].z = points[1].z = 0.0F;
    for (i = 0; i < count; i++) {
        points[0].x = scale * strokes[i][0].x;
        points[0].y = scale * strokes[i][0].y;
        points[1].x = scale * strokes[i][1].x;
        points[1].y = scale * strokes[i][1].y;
        njDrawLine3D(&line, 1, 0);
    }
}

/* draw a panel's square: its outline if outline is set, and its face,
 * translucent, if lit is set. every caller sets outline, so the outline
 * always draws */
static void route_draw_square(Uint32 color, int outline, int lit)
{
    NJS_POINT3COL square;
    NJS_POINT3 points[4] = {
#include "stg07/gravity_panel/points.inc"
    };
    NJS_COLOR colors[4];

    square.p = points;
    square.col = colors;
    square.num = 4;
    colors[0].color = colors[1].color = colors[2].color = colors[3].color =
        color;
    if (outline) {
        njDrawPolygon3D(&square, 4, 0);
    }
    if (lit) {
        njDrawPolygon3D(&square, 4, NJD_FILL | NJD_TRANSPARENT);
    }
}

static void route_translate(Sint8 row, Sint8 on)
{
    const Sint8 *at = route_places[row][on];

    njTranslate(NULL, at[0] * 2 * PANEL_HALF, at[1] * 2 * PANEL_HALF, 0.0F);
}

/* draw the panel's place on each route, with the route's letter. the place's
 * square is lit on the routes the panel is lit on */
static void gravity_panel_draw_routes(struct task *task)
{
    struct task_work *work = task->work;
    int row = ROUTE_ROW(work);
    Angle ang;

    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njTranslate(NULL, 0.0F, 0.0F, ROUTES_FRONT);
    njPushMatrix(NULL);
    route_translate(row, 0);
    route_draw_letter(letter_d, 5, 0x00cccc00, LETTER_SCALE);
    route_draw_square(0x66cccc00, 1, ROUTES(work) & 1);
    njPopMatrix(1);
    njPushMatrix(NULL);
    route_translate(row, 1);
    route_draw_letter(letter_a, 3, 0x00ff0000, LETTER_SCALE);
    route_draw_square(0x66ff0000, 1, ROUTES(work) & 2);
    njPopMatrix(1);
    njPushMatrix(NULL);
    route_translate(row, 2);
    route_draw_letter(letter_b, 8, 0x0000ff00, LETTER_SCALE);
    route_draw_square(0x6600ff00, 1, ROUTES(work) & 4);
    njPopMatrix(1);
    njPushMatrix(NULL);
    route_translate(row, 3);
    route_draw_letter(letter_c, 5, 0x000000ff, LETTER_SCALE);
    route_draw_square(0x660000ff, 1, ROUTES(work) & 8);
    njPopMatrix(1);
    njPopMatrix(1);
}

/* @hack spelling: the routes read through an inline function, which reads
 * task->work again at each call (MOV.L @(32,R4),R4 at 0x0c9064a6 and
 * 0x0c9064da). read in place, summed over gravity_panel_edit and
 * gravity_panel_step, 91 of 109 words differ */
static void gravity_panel_read_routes(struct task *task)
{
    struct task_work *work = task->work;

    ROUTES(work) = work->ang.x;
}

/* the panel's step in the set editor, which also shows its routes and its
 * label */
static void gravity_panel_edit(struct task *task)
{
    gravity_panel_read_routes(task);
    gravity_panel_run(task);
    gravity_panel_print(task);
    gravity_panel_draw_routes(task);
}

void gravity_panel_step(struct task *task)
{
    struct task_work *work = task->work;
    struct task_force *force;
    NJS_OBJECT *wall;

    task->destructor = gravity_panel_destroy;
    task->step = gravity_panel_run;
    task->draw = gravity_panel_draw;
    gravity_panel_read_routes(task);
    work->tick = 0;
    route_join(task);
    npc_set_disabled(TRUE);
    if (placement_editing(task)) {
        task->step = gravity_panel_edit;
    } else {
        wall = world_object_alloc();
        *wall = gravity_panel_wall_object;
        /* @hack type: wall->pos, a Float[3], stored in place as an NJS_POINT3;
         * without it, as three element stores, the function runs 32 bytes long
         * and 69 of 95 words differ */
        *(NJS_POINT3 *)wall->pos = work->pos;
        wall->ang[1] = work->ang.y;
        world_add(WALL_FLAGS, task, wall);
        PANEL(work)->wall = wall;
    }
    force = task->force;
    force->speed.x = 0.0F;
    force->speed.y = 0.0F;
    force->speed.z = 0.0F;
    force->turn.x = 0;
    force->turn.y = 0;
    force->turn.z = 0;
}
