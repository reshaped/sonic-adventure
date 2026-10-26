/* the race's camera: its modes, the steps they give camera_override, and
 * the walls that stop it */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "al_race/course.h"
#include "al_race/game.h"
#include "al_race/race_camera.h"
#include "al_race/racer.h"
#include "camera/camera.h"
#include "collision/polygon.h"
#include "collision/world.h"
#include "debug/editor.h"
#include "path/path.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* race_camera_avoid_walls takes a surface whose normal.y is above FACING to
 * face up, and one whose normal.y is below -FACING to face down */
#define FACING 4551.0F

/* an easing point's speed gains EASE_PULL of the way left to its goal each
 * step, then loses EASE_DRAG of itself */
#define EASE_PULL 0.01F
#define EASE_DRAG 0.2F

/* ease point a step toward goal: change speed as EASE_PULL and EASE_DRAG
 * say, then move point by it. delta is scratch */
#define EASE(point, speed, goal, delta)                                        \
    do {                                                                       \
        VECTOR_DIFF(delta, goal, point);                                       \
        VECTOR_SCALE(delta, delta, EASE_PULL);                                 \
        VECTOR_ADD(speed, delta);                                              \
        VECTOR_SCALE(delta, speed, EASE_DRAG);                                 \
        VECTOR_SUB(speed, delta);                                              \
        VECTOR_ADD(point, speed);                                              \
    } while (0)

/* how far the follow and ahead modes keep from the racer, and how far above
 * it they look */
#define RACER_DISTANCE 20.0F
#define LOOK_HEIGHT 3.0F

/* the middle of the hall, which RACE_CAMERA_HALL bounds the camera about */
#define HALL_X 2018.0F
#define HALL_Y 33.0F
#define HALL_Z 0.0F
/* how far RACE_CAMERA_HALL's aim keeps from player 0 and above it, and how
 * far above player 0 the camera looks */
#define HALL_DISTANCE 40.0F
#define HALL_HEIGHT 22.0F
#define HALL_LOOK_HEIGHT 7.0F
/* within HALL_NEAR of where it looks, RACE_CAMERA_HALL aims at the middle.
 * a player 0 within HALL_MIDDLE of the middle moves the aim along x to
 * HALL_MIDDLE_AIM_X */
#define HALL_NEAR 20.0F
#define HALL_MIDDLE 5.0F
#define HALL_MIDDLE_AIM_X 2070.0F
/* the lowest and the highest RACE_CAMERA_HALL lets the camera go */
#define HALL_LOW_Y 5.0F
#define HALL_HIGH_Y 48.0F
/* the hall's bounds: an ellipse about the middle that reaches HALL_BOUND_X
 * along x and HALL_BOUND_Z along z. an aim outside them goes back onto the
 * ellipse just inside them, of HALL_EDGE_X and HALL_EDGE_Z */
#define HALL_BOUND_X 78.0F
#define HALL_BOUND_Z 48.0F
#define HALL_EDGE_X 76.0F
#define HALL_EDGE_Z 46.0F
/* RACE_CAMERA_HALL turns camera->angle by this times the turn it asks for */
#define HALL_TURN 1.2F

/* where race_camera_orbit puts the camera: at pos, moved radius_x times the
 * cosine of a heading along x, and radius_z times its sine along z. the
 * heading starts at heading, and turns by step each step */
struct race_camera_orbit {
    Float radius_x;
    Float radius_z;
    Angle heading;
    Angle step;
    NJS_POINT3 *pos;
};

/* where a fixed view puts the camera, and how it turns it */
struct race_camera_view {
    NJS_POINT3 pos;
    struct angles ang;
};

static void race_camera_at_angles(struct camera_view *view);
static void race_camera_look_at(struct camera_view *view);
static void race_camera_follow(struct race_camera *camera,
                               struct race_game *game, Angle heading);
static void race_camera_orbit(struct race_camera *camera,
                              struct race_game *game,
                              struct race_camera_orbit *orbit);
static void race_camera_hall(struct race_camera *camera);
static void race_camera_fixed(struct race_camera *camera, int view);
static void race_camera_ease(struct race_camera *camera, struct race_game *game,
                             NJS_POINT3 *aim, NJS_POINT3 *target);
static void race_camera_ahead(struct race_camera *camera,
                              struct race_game *game);
static void race_camera_finish(struct race_camera *camera,
                               struct race_game *game);
static void race_camera_free(struct race_camera *camera,
                             struct race_game *game);
static void race_camera_avoid_walls(struct race_camera *camera, NJS_POINT3 *pos,
                                    NJS_POINT3 *from);

/* the hall's view, then the course's three, by race kind as
 * race_camera_step picks them */
static const struct race_camera_view race_camera_views[4] = {
    {{HALL_VIEW_X, HALL_VIEW_Y, HALL_VIEW_Z},
     {HALL_VIEW_ANG_X, HALL_VIEW_ANG_Y, 0}},
    {{171.0F, 122.0F, -38.0F}, {-13471, 15834, 0}},
    {{-299.0F, 121.0F, 139.0F}, {-8463, 16107, 0}},
    {{229.0F, 50.0F, 246.0F}, {-9009, -13923, 0}}};

/* the step the fixed and free modes give camera_override, under
 * CAMERA_AT_ANGLES: the camera stands at camera_place.pos, turned by
 * camera_place.ang */
static void race_camera_at_angles(struct camera_view *view)
{
    VECTOR_COPY(&camera_state->pos, &camera_place.pos);
    camera_state->ang_x = camera_place.ang.x;
    camera_state->ang_y = camera_place.ang.y;
    camera_state->ang_z = camera_place.ang.z;
}

/* the step the other modes give camera_override, under CAMERA_LOOK_AT: the
 * camera stands at camera_place.pos and looks at camera_place.target, with
 * no roll */
static void race_camera_look_at(struct camera_view *view)
{
    VECTOR_COPY(&camera_state->pos, &camera_place.pos);
    VECTOR_COPY(&camera_state->target, &camera_place.target);
    camera_state->ang_z = 0;
}

void fun_0cb889fa(struct camera_view *view)
{
    VECTOR_COPY(&camera_state->target, &camera_place.target);
    camera_state->ang_x = camera_place.ang.x;
    camera_state->ang_y = camera_place.ang.y;
    camera_state->ang_z = camera_place.ang.z;
    camera_state->distance = camera_place.distance;
}

void race_camera_reset(struct race_camera *camera)
{
    camera->mode = RACE_CAMERA_NONE;
    camera->previous = RACE_CAMERA_NONE;
}

void race_camera_step(struct race_camera *camera, struct race_game *game)
{
    struct racer *racer = &game->racers[game->lane];
    const struct race_course *course = &race_course;

    if (edit_mode && camera->mode != RACE_CAMERA_EDITOR) {
        race_camera_release(camera);
        camera->mode = RACE_CAMERA_EDITOR;
        camera->phase = 0;
    }
    switch (camera->mode) {
    /* @hack redundant: a case for RACE_CAMERA_NONE that only breaks, which
     * starts the switch's table at mode 0 (0x0cb88af4); without it, 162 of 193
     * words differ */
    case RACE_CAMERA_NONE:
        break;
    case RACE_CAMERA_HALL:
        race_camera_hall(camera);
        break;
    case RACE_CAMERA_HALL_FIXED:
        race_camera_fixed(camera, 0);
        break;
    case RACE_CAMERA_AHEAD:
        race_camera_ahead(camera, game);
        break;
    case RACE_CAMERA_COURSE_FIXED:
        switch (game->kind) {
        case 0:
        case 1:
        case 4:
            race_camera_fixed(camera, 1);
            break;
        case 2:
            race_camera_fixed(camera, 2);
            break;
        /* @hack redundant: case 3, which shares the default's code, keeps the
         * image's CMP/EQ #3 at 0x0cb88b70; without it, 135 of 193 words
         * differ */
        case 3:
        default:
            race_camera_fixed(camera, 3);
            break;
        }
        break;
    case RACE_CAMERA_FOLLOW:
        if (racer_path_index(racer) == 0) {
            race_camera_follow(camera, game, NJM_DEG_ANG(135));
        } else if (racer_path_index(racer) == 1) {
            race_camera_follow(camera, game, NJM_DEG_ANG(225));
        } else {
            race_camera_follow(camera, game, NJM_DEG_ANG(180));
        }
        break;
    case RACE_CAMERA_FRONT:
    case RACE_CAMERA_FRONT_2:
        race_camera_follow(camera, game, 0);
        break;
    case RACE_CAMERA_BEHIND:
        race_camera_follow(camera, game, NJM_DEG_ANG(180));
        break;
    case RACE_CAMERA_WATCH: {
        struct race_camera_orbit orbit;

        orbit.radius_x = 0.0F;
        orbit.radius_z = 0.0F;
        orbit.heading = 0;
        orbit.step = 0;
        if (racer_path_index(racer) == 0 && racer->work->pos.y < 20.0F) {
            orbit.pos = &course->targets[18];
        } else if (racer_path_index(racer) == 1) {
            orbit.pos = &course->targets[19];
        } else {
            orbit.pos = &course->segments[racer->segment].camera;
        }
        race_camera_orbit(camera, game, &orbit);
        break;
    }
    case RACE_CAMERA_FINISH:
        race_camera_finish(camera, game);
        break;
    case RACE_CAMERA_FREE:
        race_camera_free(camera, game);
        break;
    /* @hack redundant: a case for RACE_CAMERA_EDITOR that only breaks, which
     * runs the switch's table to mode 13 (MOV #14,R1 at 0x0cb88acc); without
     * it, 19 of 193 words and 5 table words differ */
    case RACE_CAMERA_EDITOR:
        break;
    }
    camera->previous = camera->mode;
    camera->flags &= ~RACE_CAMERA_HEADING;
}

int race_camera_get_mode(struct race_camera *camera)
{
    return camera->mode;
}

void race_camera_release(struct race_camera *camera)
{
    if (camera->flags & RACE_CAMERA_OVERRIDE) {
        camera_restore();
        camera->flags &= ~RACE_CAMERA_OVERRIDE;
    }
}

void fun_0cb88d14(struct race_camera *camera)
{
    race_camera_release(camera);
    camera->mode = RACE_CAMERA_WATCH;
    camera->phase = 0;
}

void fun_0cb88d2a(struct race_camera *camera)
{
    race_camera_release(camera);
}

/* ease the camera toward its aim, which stands RACER_DISTANCE from the racer,
 * camera->angle above it and follow.heading round it. the camera looks
 * LOOK_HEIGHT above the racer. race_camera_avoid_walls stops the aim short of a
 * wall between it and that point. follow.heading turns a degree a step toward
 * 180 degrees while RACE_CAMERA_WALLED is set, else toward camera->heading
 * while RACE_CAMERA_HEADING is, else toward heading.
 *
 * the first step sets camera->angle to 28 degrees, and follow.heading straight
 * to camera->heading or heading. it puts the camera at its aim, unless the mode
 * before was RACE_CAMERA_WATCH. with no Chao, the first step puts the camera at
 * the origin, and only a change of lane moves it after.
 *
 * a change of the followed lane can move the camera at once to a lookout. it
 * does when the camera is more than 80 from the new racer, or when the new
 * racer's segment isn't the one last followed. the lookout is race_course's
 * target 18 while racer_path_index gives 0, and target 19 while it gives 1.
 * otherwise it is the camera point race_course gives the racer's segment */
static void race_camera_follow(struct race_camera *camera,
                               struct race_game *game, Angle heading)
{
    struct racer *racer = &game->racers[game->lane];
    const struct race_course *course = &race_course;
    union race_camera_state *state = &camera->state;
    struct camera_place *place;
    NJS_POINT3 aim;
    NJS_VECTOR delta;
    Angle angle;
    Float across;
    NJS_POINT3 *jump_to;

    if (camera->flags & RACE_CAMERA_HEADING) {
        heading = camera->heading;
    }
    /* @hack redundant: three calls of racer_has_chao that never run, which
     * SHC deletes. they weigh racer up, so racer takes R13, place R12 and
     * heading R8 (0x0cb88d82, 0x0cb88da4, 0x0cb88d84). without it, racer
     * and place trade, heading goes to the frame, and 361 of 571 words
     * differ */
    if (0) {
        racer_has_chao(racer);
        racer_has_chao(racer);
        racer_has_chao(racer);
    }
    /* @hack spelling: place set in the switch's comma, which puts its address
     * after RACER_DISTANCE's 20.0F in the pool (0x0cb88dcc, 0x0cb88dd0).
     * set before the switch, it comes first, and 2 of 571 words and 4 pool
     * words differ */
    switch ((place = &camera_place), camera->phase) {
    case 0:
        camera_override(race_camera_look_at, 0, CAMERA_LOOK_AT);
        camera->flags |= RACE_CAMERA_OVERRIDE;
        camera->angle = NJM_DEG_ANG(28);
        VECTOR_CLEAR(&camera->speed);
        state->follow.heading = heading;
        state->follow.segment = 0;
        /* the lane holds 0 to 7, and widening it keeps the value */
        /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
        state->follow.lane = game->lane;
        if (racer_has_chao(racer)) {
            if (camera->previous != RACE_CAMERA_WATCH) {
                angle = racer->work->ang.y + state->follow.heading;
                /* @hack spelling: aim's members through &aim. SHC forms each
                 * one's address before its call and keeps it on the stack
                 * across the call (0x0cb88e2c to 0x0cb88e42 for y). by name,
                 * each stores to the frame after its call, and 494 of 571
                 * words differ */
                (&aim)->y = RACER_DISTANCE * njSin(camera->angle);
                across = RACER_DISTANCE * njCos(camera->angle);
                (&aim)->x = across * njCos(angle);
                (&aim)->z = -(across * njSin(angle));
                VECTOR_SUM(&place->pos, &racer->work->pos, &aim);
            }
        } else {
            VECTOR_CLEAR(&place->pos);
        }
        camera->phase++;
        /* fall through */
    case 1:
        if (racer_has_chao(racer)) {
            if (camera->flags & RACE_CAMERA_WALLED) {
                heading = NJM_DEG_ANG(180);
            }
            angle = state->follow.heading - heading;
            if (njAbs(angle) < NJM_DEG_ANG(1)) {
                state->follow.heading = heading;
            } else if (angle > 0) {
                state->follow.heading -= NJM_DEG_ANG(1);
            } else {
                state->follow.heading += NJM_DEG_ANG(1);
            }
            place->target = racer->work->pos;
            place->target.y += LOOK_HEIGHT;
            heading = racer->work->ang.y + state->follow.heading;
            /* @hack spelling: aim's members through &aim, as in phase 0
             * (0x0cb88f94 to 0x0cb88fa4 for y); by name, 362 of 571 words
             * differ */
            (&aim)->y = RACER_DISTANCE * njSin(camera->angle);
            across = RACER_DISTANCE * njCos(camera->angle);
            (&aim)->x = across * njCos(heading);
            (&aim)->z = -(across * njSin(heading));
            VECTOR_SUM(&aim, &racer->work->pos, &aim);
            race_camera_avoid_walls(camera, &aim, &place->target);
            EASE(&place->pos, &camera->speed, &aim, &delta);
        }
        if (state->follow.lane != game->lane) {
            VECTOR_DIFF(&delta, &place->pos, &racer->work->pos);
            if (VECTOR_SQUARED(&delta) > 80.0F * 80.0F ||
                state->follow.segment != racer->segment) {
                if (racer_path_index(racer) == 0) {
                    jump_to = &course->targets[18];
                } else if (racer_path_index(racer) == 1) {
                    jump_to = &course->targets[19];
                } else {
                    jump_to = &course->segments[racer->segment].camera;
                }
                VECTOR_COPY(&place->pos, jump_to);
            }
        }
        break;
    }
    state->follow.segment = racer->segment;
    /* the lane holds 0 to 7, and widening it keeps the value */
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    state->follow.lane = game->lane;
}

/* put the camera where orbit says, and look at the racer. with no Chao, look at
 * the racer's distance along race_paths[2], the course's path, or at the origin
 * where the path has no point there. camera_place.target jumps to what the
 * camera looks at as the racer enters another segment, and eases there
 * otherwise */
static void race_camera_orbit(struct race_camera *camera,
                              struct race_game *game,
                              struct race_camera_orbit *orbit)
{
    struct racer *racer = &game->racers[game->lane];
    union race_camera_state *state = &camera->state;
    struct camera_place *place = &camera_place;
    NJS_POINT3 *target = NULL;
    NJS_POINT3 pos;
    struct path_sample sample;
    NJS_POINT3 raised;
    NJS_VECTOR delta;

    switch (camera->phase) {
    case 0:
        camera_override(race_camera_look_at, 0, CAMERA_LOOK_AT);
        VECTOR_CLEAR(&camera->speed);
        VECTOR_CLEAR(&camera->target_speed);
        camera->flags |= RACE_CAMERA_OVERRIDE;
        if (racer_has_chao(racer)) {
            VECTOR_COPY(&place->target, &racer->work->pos);
        } else {
            VECTOR_CLEAR(&place->target);
        }
        state->orbit.heading = orbit->heading;
        state->orbit.segment = 0;
        camera->phase++;
        /* fall through */
    case 1:
        if (racer_has_chao(racer)) {
            target = &racer->work->pos;
        } else {
            sample.distance = racer->distance;
            if (path_sample(race_paths[2], &sample)) {
                pos.x = sample.pos.x;
                pos.y = sample.pos.y;
                pos.z = sample.pos.z;
            } else {
                VECTOR_CLEAR(&pos);
            }
            target = &pos;
        }
        state->orbit.heading += orbit->step;
        VECTOR_COPY(&place->pos, orbit->pos);
        if (orbit->radius_x != 0.0F || orbit->radius_z != 0.0F) {
            place->pos.x += orbit->radius_x * njCos(state->orbit.heading);
            place->pos.z += orbit->radius_z * njSin(state->orbit.heading);
        }
        break;
    }
    if (state->orbit.segment != racer->segment) {
        VECTOR_COPY(&place->target, target);
    } else {
        /* @hack redundant: raised, a copy of target 3 higher, which nothing
         * reads. the image stores it (0x0cb894c0 to 0x0cb894ea), then eases
         * toward target itself (0x0cb894ee); without it, 207 of 356 words
         * differ */
        VECTOR_COPY(&raised, target);
        raised.y += 3.0F;
        EASE(&place->target, &camera->target_speed, target, &delta);
    }
    state->orbit.segment = racer->segment;
}

void fun_0cb89620(NJS_POINT3 *pos)
{
    VECTOR_COPY(&camera_place.target, pos);
}

/* ease the camera toward its aim, HALL_DISTANCE from player 0 at
 * camera->angle and HALL_HEIGHT above it, and look HALL_LOOK_HEIGHT above
 * player 0. the first step sets camera->angle to 0 and puts the camera at its
 * aim.
 *
 * within HALL_NEAR of where it looks, the camera aims at the hall's middle
 * instead, and camera->angle points from player 0 at the middle. a player 0
 * within HALL_MIDDLE of the middle along x and z makes the aim
 * (HALL_MIDDLE_AIM_X, HALL_Y, HALL_Z) instead, and camera->angle 0.
 *
 * farther off, the camera keeps from HALL_LOW_Y to HALL_HIGH_Y high. each step,
 * camera->angle turns by HALL_TURN times a turn. the right trigger asks for a
 * turn of 2 degrees and the left for -2, but not while hall.at_edge is set. an
 * aim outside the hall's bounds goes back onto the ellipse inside them, and
 * sets hall.at_edge. the turn is then the change the move back makes in the
 * aim's direction from the camera, negated */
static void race_camera_hall(struct race_camera *camera)
{
    NJS_POINT3 *pos = &camera_place.pos;
    NJS_POINT3 *target = &camera_place.target;
    NJS_VECTOR *speed = &camera->speed;
    NJS_VECTOR *push = &camera->push;
    union race_camera_state *state;
    struct task_work **players;
    /* @hack redundant: unused, which takes players' address, so players keeps a
     * frame word at @R15. the image stores it there (0x0cb8968c) and reloads
     * it at each read of player 0 (0x0cb89698); without it, SHC keeps
     * players in R5, and 157 of 740 words differ */
    struct task_work ***unused = &players;
    NJS_POINT3 aim;
    NJS_VECTOR delta;
    NJS_VECTOR *from;
    Float before_z;
    Float before_x;
    Float edge_z;
    Float edge_x;
    Angle turn = 0;
    int near = 0;
    Angle heading;
    Float cosine;
    Float sine;

    state = &camera->state;
    if (camera->phase == 0) {
        state->hall.at_edge = 0;
    }
    /* @hack redundant: an assignment of player_work to players that never
     * runs, the first of two SHC deletes. the two weigh players up, so
     * player 0's test loads into R2 (0x0cb8967a), and edge_x and edge_z
     * keep the image's frame slots (0x0cb89ba0, 0x0cb89bae). without both,
     * 5 of 740 words differ; without it, the two slots trade and 3 of 740
     * words differ */
    if (0) {
        players = player_work;
    }
    /* @hack redundant: the second assignment of player_work to players that
     * never runs, which does as the first does; without it, edge_x's and
     * edge_z's slots trade (0x0cb89b9a to 0x0cb89bb0) and 3 of 740 words
     * differ */
    if (0) {
        players = player_work;
    }
    /* @hack redundant: a jump to the next statement, which moves SHC's choice
     * of a scratch register, so player 0's test loads into R2 as well
     * (0x0cb8967a, 0x0cb8967c); without it, R3, and 2 of 740 words differ */
    goto player_test;
player_test:
    if (player_work[0]) {
        if (camera->phase == 0) {
            camera->angle = 0;
        }
        /* @hack spelling: players set in the first read, which spills it after
         * 7.0F's load (0x0cb8968a, 0x0cb8968c); set before the read, SHC
         * first copies it to R3, and 53 of 740 words differ */
        target->x = (players = player_work)[0]->pos.x;
        target->y = players[0]->pos.y;
        target->z = players[0]->pos.z;
        target->y += HALL_LOOK_HEIGHT;
        VECTOR_COPY(&aim, &players[0]->pos);
    } else {
        race_camera_release(camera);
        camera->mode = RACE_CAMERA_NONE;
        camera->phase = 0;
        return;
    }
    /* @hack spelling: the two tests nested, which put the first button
     * test in R1 and R0 (0x0cb89730, 0x0cb89738); joined with &&, it takes
     * R2 and R1, and 3 of 740 words differ */
    if (!state->hall.at_edge) {
        if (!edit_mode) {
            if (per_info[0]->on & PDD_DGT_TR) {
                turn = NJM_DEG_ANG(2);
            }
            if (per_info[0]->on & PDD_DGT_TL) {
                turn -= NJM_DEG_ANG(2);
            }
        }
    }
    aim.y += HALL_HEIGHT;
    aim.x += HALL_DISTANCE * njCos(camera->angle);
    aim.z += -(HALL_DISTANCE * njSin(camera->angle));
    switch (camera->phase) {
    case 0:
        camera_override(race_camera_look_at, 0, CAMERA_LOOK_AT);
        camera->flags |= RACE_CAMERA_OVERRIDE;
        VECTOR_CLEAR(speed);
        VECTOR_COPY(pos, &aim);
        state->hall.unk_01 = 0;
        camera->phase++;
        break;
    case 1:
        VECTOR_DIFF(&delta, target, pos);
        if (VECTOR_SQUARED(&delta) < HALL_NEAR * HALL_NEAR) {
            near = 1;
            delta.x = target->x - HALL_X;
            delta.z = target->z - HALL_Z;
            if ((delta.x * delta.x) + (delta.z * delta.z) <
                HALL_MIDDLE * HALL_MIDDLE) {
                aim.x = HALL_MIDDLE_AIM_X;
                aim.y = HALL_Y;
                aim.z = HALL_Z;
                camera->angle = 0;
            } else {
                aim.x = HALL_X;
                aim.y = HALL_Y;
                aim.z = HALL_Z;
                camera->angle = atan2f(delta.z, -delta.x) * ANGLE_PER_RADIAN;
            }
        }
        VECTOR_DIFF(&delta, &aim, pos);
        VECTOR_SCALE(&delta, &delta, EASE_PULL);
        VECTOR_ADD(push, &delta);
        if (near) {
            VECTOR_ADD(speed, push);
            VECTOR_SCALE(&delta, speed, EASE_DRAG);
            VECTOR_SUB(speed, &delta);
            VECTOR_ADD(pos, speed);
            VECTOR_CLEAR(push);
            break;
        }
        /* @hack spelling: delta's members through from, which keeps delta's
         * address in R10 (0x0cb89aa2); by name, each access forms the
         * address again, and 281 of 740 words differ */
        from = &delta;
        from->x = aim.x - HALL_X;
        from->z = aim.z - HALL_Z;
        from->y = pos->y - HALL_Y;
        heading = atan2f(-from->z, from->x) * ANGLE_PER_RADIAN;
        if (from->y > (HALL_HIGH_Y - HALL_Y)) {
            speed->y = 0.0F;
            push->y = 0.0F;
            pos->y = HALL_HIGH_Y;
        }
        if (from->y < (HALL_LOW_Y - HALL_Y)) {
            speed->y = 0.0F;
            push->y = 0.0F;
            pos->y = HALL_LOW_Y;
        }
        cosine = njCos(heading);
        /* @hack redundant: a negation of cosine that never runs, which SHC
         * deletes. it weighs cosine up, so the sums of squares take FR6 and
         * FR5 (0x0cb898b0, 0x0cb89b2e), and before_x and before_z keep the
         * image's frame slots (0x0cb89b68, 0x0cb89b76); without it, 15 of
         * 740 words differ */
        if (0) {
            cosine = -cosine;
        }
        sine = njSin(heading);
        state->hall.at_edge = 0;
        if ((cosine * cosine * (HALL_BOUND_X * HALL_BOUND_X)) +
                (sine * sine * (HALL_BOUND_Z * HALL_BOUND_Z)) <
            ACROSS_SQUARED(from)) {
            state->hall.at_edge = 1;
            VECTOR_CLEAR(push);
            VECTOR_CLEAR(speed);
            before_x = aim.x - pos->x;
            before_z = aim.z - pos->z;
            aim.x = HALL_X + (HALL_EDGE_X * cosine);
            aim.z = HALL_Z - (HALL_EDGE_Z * sine);
            edge_x = aim.x - pos->x;
            edge_z = aim.z - pos->z;
            heading = (Angle)(atan2f(-edge_z, edge_x) * ANGLE_PER_RADIAN) -
                      (Angle)(atan2f(-before_z, before_x) * ANGLE_PER_RADIAN);
            /* the change the short way round: heading's low 16 bits, sign
             * extended */
            /* @hack spelling: the sign extension written out, which tests bit
             * 15 (0x0cb89bdc); as a (Sint16) cast, SHC writes EXTS.W, and 122
             * of 740 words differ */
            turn = -((heading & 0x8000) ? (heading | 0xffff0000)
                                        : (heading & 0xffff));
        }
        camera->angle += turn * HALL_TURN;
        VECTOR_ADD(speed, push);
        VECTOR_SCALE(&delta, speed, EASE_DRAG);
        VECTOR_SUB(speed, &delta);
        VECTOR_ADD(pos, speed);
        VECTOR_CLEAR(push);
    }
}

/* the first step puts the camera at race_camera_views[view], and the steps
 * after leave it there */
static void race_camera_fixed(struct race_camera *camera, int view)
{
    NJS_POINT3 *pos = &camera_place.pos;
    struct angles *ang = &camera_place.ang;
    const struct race_camera_view *fixed = &race_camera_views[view];

    switch (camera->phase) {
    case 0:
        camera_override(race_camera_at_angles, 0, CAMERA_AT_ANGLES);
        camera->flags |= RACE_CAMERA_OVERRIDE;
        VECTOR_COPY(pos, &fixed->pos);
        VECTOR_COPY(ang, &fixed->ang);
        camera->phase++;
        break;
    /* @hack redundant: a case 1 that only breaks, which keeps the image's
     * CMP/EQ #1 at 0x0cb89cf4; without it, 58 of 58 words differ */
    case 1:
        break;
    }
}

/* the first step puts the camera at aim, looking at target. from then on,
 * EASE moves camera_place.pos toward aim and camera_place.target toward
 * target */
static void race_camera_ease(struct race_camera *camera, struct race_game *game,
                             NJS_POINT3 *aim, NJS_POINT3 *target)
{
    NJS_POINT3 *place_pos = &camera_place.pos;
    NJS_POINT3 *place_target = &camera_place.target;
    NJS_VECTOR *speed = &camera->speed;
    NJS_VECTOR *target_speed = &camera->target_speed;
    NJS_VECTOR delta;

    switch (camera->phase) {
    case 0:
        camera_override(race_camera_look_at, 0, CAMERA_LOOK_AT);
        camera->flags |= RACE_CAMERA_OVERRIDE;
        VECTOR_CLEAR(speed);
        VECTOR_CLEAR(target_speed);
        VECTOR_COPY(place_pos, aim);
        VECTOR_COPY(place_target, target);
        camera->phase++;
        /* fall through */
    case 1:
        EASE(place_pos, speed, aim, &delta);
        EASE(place_target, target_speed, target, &delta);
        break;
    }
}

/* ease the camera with race_camera_ease toward a point RACER_DISTANCE from
 * the racer, ahead of it and 18 degrees above it. the camera looks
 * LOOK_HEIGHT above the racer. while the racer has no Chao, the step does
 * nothing */
static void race_camera_ahead(struct race_camera *camera,
                              struct race_game *game)
{
    struct racer *racer = &game->racers[game->lane];
    NJS_POINT3 aim;
    NJS_POINT3 target;
    Float across;

    if (racer_has_chao(racer)) {
        /* @hack spelling: aim's members through &aim, whose addresses SHC forms
         * before the call and keeps in R8 across it (0x0cb8a04c to
         * 0x0cb8a066); by name, 135 of 140 words differ */
        (&aim)->y = RACER_DISTANCE * njSin(NJM_DEG_ANG(18));
        across = RACER_DISTANCE * njCos(NJM_DEG_ANG(18));
        (&aim)->x = across * njCos(racer->work->ang.y);
        (&aim)->z = -(across * njSin(racer->work->ang.y));
        VECTOR_ADD(&aim, &racer->work->pos);
        VECTOR_COPY(&target, &racer->work->pos);
        target.y += LOOK_HEIGHT;
        race_camera_ease(camera, game, &aim, &target);
    }
}

/* ease the camera with race_camera_ease through finish.phase's four phases,
 * and look at the winner. FINISH_HOLD and FINISH_HOLD_2 hold at race_course's
 * target 16 for 37 steps each. FINISH_MOVE moves from 16 to target 17, 3/700
 * of the way a step. FINISH_CIRCLE goes from 17 round a circle of radius 6, a
 * degree a step. while the winner has no Chao, the step does nothing */
static void race_camera_finish(struct race_camera *camera,
                               struct race_game *game)
{
    struct racer *racer = &game->racers[game->standings.lanes[0]];
    NJS_POINT3 aim;
    NJS_POINT3 target;
    NJS_VECTOR delta;
    NJS_POINT3 *start;
    NJS_POINT3 *end;
    NJS_POINT3 *points;
    union race_camera_state *state;

    if (racer_has_chao(racer)) {
        state = &camera->state;
        points = race_course.targets;
        VECTOR_COPY(&target, &racer->work->pos);
        if (!camera->phase) {
            state->finish.phase = FINISH_HOLD;
            state->finish.timer = 0.0F;
            state->finish.heading = 0;
        }
        /* @hack spelling: start and end set in the switch's comma, after the
         * phase's load (0x0cb8a1da to 0x0cb8a1e8); set as statements before
         * the switch, they come first, and 6 of 257 words differ */
        switch ((start = &points[16], end = &points[17], state->finish.phase)) {
        case FINISH_HOLD:
        case FINISH_HOLD_2:
            state->finish.timer += 1.0F;
            if (state->finish.timer > 0.6F * 60.0F) {
                state->finish.timer = 0.0F;
                state->finish.phase++;
            }
            VECTOR_COPY(&aim, start);
            break;
        case FINISH_MOVE:
            /* @bug logic: the phase is FINISH_MOVE here, so the test of
             * FINISH_HOLD, TST R2,R2 at 0x0cb8a262, never holds, and only the
             * second arm runs */
            if (state->finish.phase == FINISH_HOLD) {
                state->finish.timer += 0.1F / 60.0F;
                if (state->finish.timer > 0.1F) {
                    state->finish.phase++;
                }
            } else {
                state->finish.timer += 3.0F / 700.0F;
                if (state->finish.timer > 1.0F) {
                    state->finish.phase++;
                }
            }
            VECTOR_DIFF(&delta, end, start);
            VECTOR_SCALE(&aim, &delta, state->finish.timer);
            VECTOR_ADD(&aim, start);
            break;
        case FINISH_CIRCLE:
            VECTOR_COPY(&aim, end);
            aim.x += -6.0F + (6.0F * njCos(state->finish.heading));
            aim.z += 6.0F * njSin(state->finish.heading);
            state->finish.heading += NJM_DEG_ANG(1);
            break;
        }
        /* @hack redundant: a call of njCalcVector that never runs, which
         * SHC deletes. it keeps start and end in R13 and R12 (0x0cb8a1dc,
         * 0x0cb8a1de). without it, SHC gives R13 to game, saves one register
         * fewer, the function runs 2 bytes short, race_camera_free after it
         * moves, and 122 of 257 words differ */
        if (0) {
            njCalcVector(NULL, start, end);
        }
        /* @bug uninitialized: a phase outside the four leaves aim
         * unwritten. the switch's BRA at 0x0cb8a1fa skips every case, and
         * race_camera_ease reads aim (0x0cb89e22) through the call's pointer
         * (0x0cb8a39c). the phase starts at FINISH_HOLD and moves one on
         * at a time, and FINISH_CIRCLE moves it no further, so the read
         * never runs */
        race_camera_ease(camera, game, &aim, &target);
    }
}

/* pad 0 flies the camera. its d-pad turns the camera 1.5 degrees a step, and
 * its stick, pushed past 20, moves it 1 along the camera's own x and z. L
 * and R move it 1 up and down. with B held, it turns 2.5 degrees and moves 2
 * a step. its x angle stays within 89 degrees. in the hall, the first step
 * puts the camera at (2110, 10, 0). njPrint shows where it is and how it
 * turns */
static void race_camera_free(struct race_camera *camera, struct race_game *game)
{
    NJS_POINT3 *pos = &camera_place.pos;
    struct angles *ang = &camera_place.ang;
    NJS_VECTOR move;
    NJS_VECTOR world;
    NJS_MATRIX matrix;
    /* @hack type: the act read through an unsigned int, which widens it
     * into R4 (0x0cb8a3ce). the stick's limits, 0x1400 and -0x1400, then
     * take R6 and R5 (0x0cb8a4f4, 0x0cb8a504). as one Sint16, the act stays
     * in R0, the two trade, and 8 of 257 words differ */
    unsigned int act_byte = (Uint8)stage_get_stage_act();
    Sint16 act = act_byte;
    Float speed;
    Angle step;

    switch (camera->phase) {
    case 0:
        camera_override(race_camera_at_angles, 0, CAMERA_AT_ANGLES);
        camera->flags |= RACE_CAMERA_OVERRIDE;
        VECTOR_CLEAR(&camera->target_speed);
        VECTOR_CLEAR(&camera->speed);
        ang->x = 0;
        ang->y = 0;
        ang->z = 0;
        if (act == ACT_HALL) {
            pos->x = 2110.0F;
            pos->y = 10.0F;
            pos->z = 0.0F;
        }
        camera->phase++;
        /* fall through */
    case 1:
        speed = 1.0F;
        step = NJM_DEG_ANG(1.5);
        VECTOR_CLEAR(&move);
        if (!edit_mode) {
            if (per_info[0]->on & PDD_DGT_TB) {
                speed = 2.0F;
                step = NJM_DEG_ANG(2.5);
            }
            if (per_info[0]->on & PDD_DGT_KR) {
                ang->y -= step;
            }
            if (per_info[0]->on & PDD_DGT_KL) {
                ang->y += step;
            }
            if (per_info[0]->on & PDD_DGT_KD) {
                ang->x -= step;
            }
            if (per_info[0]->on & PDD_DGT_KU) {
                ang->x += step;
            }
            if (ang->x > NJM_DEG_ANG(89)) {
                ang->x = NJM_DEG_ANG(89);
            }
            if (ang->x < -NJM_DEG_ANG(89)) {
                ang->x = -NJM_DEG_ANG(89);
            }
            if (per_info[0]->x1 * 256 > 0x1400) {
                move.x = speed;
            }
            if (per_info[0]->x1 * 256 < -0x1400) {
                move.x = -speed;
            }
            if (per_info[0]->y1 * 256 > 0x1400) {
                move.z = speed;
            }
            if (per_info[0]->y1 * 256 < -0x1400) {
                move.z = -speed;
            }
            if (per_info[0]->on & PDD_DGT_TL) {
                move.y = speed;
            }
            if (per_info[0]->on & PDD_DGT_TR) {
                move.y = -speed;
            }
        }
        njUnitMatrix(&matrix);
        njRotateXYZ(&matrix, ang->x, ang->y, 0);
        njCalcVector(&matrix, &move, &world);
        VECTOR_ADD(pos, &world);
        break;
    }
    njPrintC(NJM_LOCATION(20, 2), "FREE");
    njPrintF(NJM_LOCATION(20, 3), pos->x, 4);
    njPrintF(NJM_LOCATION(20, 4), pos->y, 4);
    njPrintF(NJM_LOCATION(20, 5), pos->z, 4);
    njPrintH(NJM_LOCATION(20, 6), ang->x, 8);
    njPrintH(NJM_LOCATION(20, 7), ang->y, 8);
}

/* list the world's objects within 30 of pos with world_list_near. for each,
 * take its nearest surfaces along x and along z on from's side of pos. where
 * such a surface lies between from and pos, stop pos 1.5 short of it. the call
 * clears RACE_CAMERA_WALLED, and sets it again where it stops pos. the code
 * also lifts camera_place.pos.y over a floor above pos, but the @bug logic
 * below keeps the lift from running */
static void race_camera_avoid_walls(struct race_camera *camera, NJS_POINT3 *pos,
                                    NJS_POINT3 *from)
{
    struct sides_query query;
    struct world_object *objects;
    struct world_object *object;
    int i;
    Float ceiling;
    Float floor;
    Float level;

    /* @hack spelling: objects loaded before world_list_near, which keeps
     * world_objects' address in a frame word across the call (0x0cb8a6a6,
     * 0x0cb8a6e8). with object set straight from world_objects after the
     * call, R11 loads it then, and 134 of 227 words differ */
    objects = world_objects;
    world_list_near(pos->x, pos->y, pos->z, 30.0F);
    VECTOR_COPY(&query.pos, pos);
    camera->flags &= ~RACE_CAMERA_WALLED;
    i = 0;
    floor = 10000.0F;
    ceiling = 10000.0F;
    object = objects;
    /* @hack spelling: a test, then a do-while, which loads the loop's constants
     * once world_count tests above 0 (0x0cb8a6ee to 0x0cb8a6f6); as a for
     * loop, SHC loads them before the test branches, and 17 of 227 words
     * differ */
    if (world_count > 0) {
        do {
            if (polygon_find_sides(&query, object->object, object->flags)) {
                if (query.sides[SIDE_ABOVE].found) {
                    /* @bug logic: FACING, 4551, is 25 degrees as an
                     * Angle, but normal is a unit vector, so neither test
                     * holds (FCMP/GT at 0x0cb8a71e and 0x0cb8a788). ceiling
                     * and floor stay 10000, and the lift below never runs */
                    if (query.sides[SIDE_ABOVE].normal.y < -FACING) {
                        if (query.sides[SIDE_ABOVE].level < ceiling) {
                            ceiling = query.sides[SIDE_ABOVE].level;
                        }
                    } else if (query.sides[SIDE_ABOVE].normal.y > FACING) {
                        if (query.sides[SIDE_ABOVE].level < floor) {
                            floor = query.sides[SIDE_ABOVE].level;
                        }
                    }
                }
                if (pos->x - from->x < 0.0F) {
                    if (query.sides[SIDE_PLUS_X].found) {
                        level = query.sides[SIDE_PLUS_X].level;
                        if (level < from->x && level > pos->x) {
                            camera->flags |= RACE_CAMERA_WALLED;
                            pos->x = level + 1.5F;
                        }
                    }
                } else {
                    if (query.sides[SIDE_MINUS_X].found) {
                        level = query.sides[SIDE_MINUS_X].level;
                        if (level > from->x && level < pos->x) {
                            camera->flags |= RACE_CAMERA_WALLED;
                            pos->x = level - 1.5F;
                        }
                    }
                }
                if (pos->z - from->z < 0.0F) {
                    if (query.sides[SIDE_PLUS_Z].found) {
                        level = query.sides[SIDE_PLUS_Z].level;
                        if (level < from->z && level > pos->z) {
                            camera->flags |= RACE_CAMERA_WALLED;
                            pos->z = level + 1.5F;
                        }
                    }
                } else {
                    if (query.sides[SIDE_MINUS_Z].found) {
                        level = query.sides[SIDE_MINUS_Z].level;
                        if (level > from->z && level < pos->z) {
                            camera->flags |= RACE_CAMERA_WALLED;
                            pos->z = level - 1.5F;
                        }
                    }
                }
            }
            /* @hack spelling: object = object + 1. as object++, R8 and R9
             * trade (0x0cb8a6c8, 0x0cb8a6f0), and so do FR14 and FR15
             * (0x0cb8a6e4, 0x0cb8a6f6), and 15 of 227 words differ */
            object = object + 1;
        } while (++i < world_count);
    }
    if (ceiling > floor) {
        if (camera_place.pos.y + 20.0F > floor) {
            camera_place.pos.y = floor + 10.0F;
        }
    }
}

void race_camera_set_heading(struct race_camera *camera, int heading)
{
    camera->flags |= RACE_CAMERA_HEADING;
    camera->heading = heading;
}

int race_camera_to_world(NJS_POINT3 *out, NJS_POINT3 *point)
{
    struct task_work *camera = camera_work;
    NJS_MATRIX matrix;

    if (!camera) {
        return 0;
    }
    njUnitMatrix(&matrix);
    njTranslate(&matrix, camera->pos.x, camera->pos.y, camera->pos.z);
    njRotateXYZ(&matrix, camera->ang.x, camera->ang.y, 0);
    njCalcPoint(&matrix, point, out);
    return 1;
}
