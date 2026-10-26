/* the hint box, O HINT in the object lists and O HINTBOX in Hot Shelter's.
 * as player 0 comes near in front of it, the box lights up and turns to
 * face the player. a player that talks to it is shown its message, with the
 * camera on the box and the time stopped */
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "collision/ground.h"
#include "debug/editor.h"
#include "draw/model.h"
#include "event/play.h"
#include "event/script.h"
#include "hud/hud.h"
#include "object/common.h"
#include "object/helpers.h"
#include "object/hint.h"
#include "object/models.h"
#include "object/placement.h"
#include "player/player.h"
#include "player/talk.h"
#include "progress/progress.h"
#include "sound/sound.h"
#include "system/frame.h"
#include "system/math.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the box's reach is REACH beyond player 0's radius, measured across x and
 * z. once the player is near, the box turns back when the player is
 * TURN_BACK_MARGIN beyond its reach. TALK_RADIUS is the radius of the box's
 * talk, which talk_add adds to the player's */
#define REACH 15.0F
#define TURN_BACK_MARGIN 10.0F
#define TALK_RADIUS 25.0F

/* player 0 is in front of the box within FACING either side of the box's
 * home heading. a turn stops within TURN_CLOSE of the heading it turns to,
 * once its speed is under TURN_SLOW */
#define FACING NJM_DEG_ANG(30)
#define TURN_CLOSE NJM_DEG_ANG(20)
#define TURN_SLOW NJM_DEG_ANG(1)

/* the change in a turn's speed in a frame, where left is the angle still
 * to turn. the speed it eases toward is left / 15, which would finish the
 * turn in 15 frames. each frame, it closes a seventh of the gap */
#define TURN_EASE(speed, left) (((left) - ((speed) * 15.0F)) * (1.0F / 105.0F))

/* the box bobs BOB_HEIGHT above and below the height it was placed at, its
 * phase advancing BOB_STEP a frame */
#define BOB_STEP NJM_DEG_ANG(4)
#define BOB_HEIGHT 1.5F

/* the index of the lamp's material in the box's model */
#define LAMP_MATERIAL 10

/* a change of the lamp fades its level by LAMP_STEP, about a sixth, for
 * LAMP_FRAMES frames. on the frame after, the level takes its end. at level
 * 1 the lamp's gray is LAMP_GRAY */
#define LAMP_FRAMES 5
#define LAMP_STEP 0.16666668F
#define LAMP_GRAY 0xb2

/* the opaque ARGB gray whose red, green and blue are each c's low byte */
#define GRAY(c) (0xff000000 | (Uint8)(c) << 16 | (Uint8)(c) << 8 | (Uint8)(c))

/* the camera's distance from the box while the message shows, where the
 * set file's camera_distance is under 0.1 */
#define CAMERA_DISTANCE 80.0F

/* the sound hint_wait plays as player 0 comes within reach */
#define SE_NEAR 14

/* x without its sign, evaluating x twice */
#define ABS(x) ((x) > 0 ? (x) : -(x))

/* the box's work->mode */
enum hint_mode {
    HINT_WAIT, /* waits for player 0 within reach and in front */
    HINT_NEAR, /* turns to player 0 until the player talks or leaves */
    HINT_TURN, /* turns to face player 0, then shows the message */
    HINT_SPEAK, /* faces player 0 until the message is done */
    HINT_RETURN /* turns back home, and meanwhile waits as HINT_WAIT does */
};

/* the box's view of its task_work. hint_init checks that pos, id,
 * collision, event and flags sit where task_work keeps them. the set file's
 * scale gives camera_distance, camera_height and row */
struct hint_work {
    Sint8 mode;
    Sint8 submode;
    Uint8 id;
    Uint8 shown; /* 1 while the message shows, as message_set_shown sets it */
    Sint16 flags;
    Sint16 timer;
    Bool talking; /* set while a player talks to the box, as talk_add sets it */
    struct task *lamp; /* the lamp's task, or NULL */
    struct script_pages *pick; /* the message the script last picked */
    /* y is the box's heading. ground_below writes the ground's slope to x
     * and z */
    struct angles ang;
    NJS_POINT3 pos;
    /* the camera's distance from the box across x and z while the message
     * shows, and its height above the box */
    Float camera_distance;
    Float camera_height;
    Float row; /* the row of the stage's script table that picks the message */
    struct collision_work *collision;
    struct event_work *event;
};

/* the box's record, at its task's extra */
struct hint_extra {
    Angle turn; /* the turn's speed, a frame */
    Angle bob; /* the bob's phase */
    Angle home; /* the heading the box was placed with */
    Angle aim; /* the heading that faces player 0, as hint_aim last set it */
    Float base_y; /* the height the box was placed at */
    Float ground_y; /* the ground's height below the box */
    Float lift; /* the shadow's height, less the box's */
    Float reach; /* player 0's radius plus REACH */
    NJS_MODEL *model; /* the box's copy of its model, whose lamp it lights */
};

/* the lamp's record, at its task's extra */
struct hint_lamp {
    NJS_MODEL **model; /* the address of the box's extra->model */
    Float level; /* from off, 0, to on, 1 */
    Sint32 timer; /* the frames of fading left */
    Uint8 unk_0c[4];
    Bool on; /* the end the level fades toward: TRUE for on */
};

#define HINT(task) ((struct hint_work *)(task)->work)

static Bool hint_alloc(struct task *task);
static NJS_MODEL *hint_copy_model(void);
static void hint_aim(struct task *task);
static Bool hint_aim_and_test_reach(struct task *task, Float reach);
static Bool hint_turn_to_player(struct task *task);
static Bool hint_turn_home(struct task *task);
static void hint_wait(struct task *task);
static void hint_return(struct task *task);
static void hint_near(struct task *task);
static void hint_turn(struct task *task);
static void hint_speak(struct task *task);
static void hint_step(struct task *task);
static void hint_draw(struct task *task);
static void hint_destroy(struct task *task);
static void hint_lamp_step(struct task *task);
static struct task *hint_set_lamp(struct task *task, Bool on);
static void hint_take_camera(struct task *task);
static void hint_print_labels(void);

/* @hack type: the shape as an array of one, whose address hint_init keeps
 * in R11 (MOV.L at 0x8c064194). as a plain record, hint_init saves one
 * register fewer, and summed over hint_init and hint_draw, 218 of 288 words
 * differ */
/* clang-format off */
static struct collision_shape hint_shape[1] = {
    {SHAPE_SPHERE, 0, PUSH_ALL | PUSH_FIRM(PUSH_ALL), {-2.5F, -1.5F, 0.0F},
     7.0F, 0.0F, 0.0F, {0, 0, NJM_DEG_ANG(90)}, 0, NULL},
};
/* clang-format on */

/* hint_step runs the one for work->mode */
static void (*const hint_modes[])(struct task *task) = {
    hint_wait, hint_near, hint_turn, hint_speak, hint_return,
};
/* the lamp's gray and level once it is off, [0], or on, [1] */
static const Uint8 hint_lamp_grays[2] = {0x00, LAMP_GRAY};
static const Float hint_lamp_levels[2] = {0.0F, 1.0F};

void hint_init(struct task *task)
{
    struct hint_work *work = HINT(task);
    struct hint_extra *extra;
    struct player *player;

    /* @hack redundant: pos tested a second time, which keeps the image's
     * second ADD #32,R2 (0x8c064134, 0x8c064176). without it, 81 of 176
     * words differ */
    /* NOLINTBEGIN(misc-redundant-expression) */
    if (!WORK_SAME_PLACE(struct hint_work, pos) ||
        !WORK_SAME_PLACE(struct hint_work, id) ||
        !WORK_SAME_PLACE(struct hint_work, collision) ||
        !WORK_SAME_PLACE(struct hint_work, event) ||
        !WORK_SAME_PLACE(struct hint_work, pos) ||
        !WORK_SAME_PLACE(struct hint_work, flags)) {
        return;
    }
    /* NOLINTEND(misc-redundant-expression) */
    if (edit_mode) {
        NJS_POINT3 center;

        VECTOR_SUM(&center, &hint_shape->center, &work->pos);
        marker_draw(&center, hint_shape->radius);
        hint_print_labels();
    } else {
        struct script_pages pick;

        if (script_pick((int)work->row, &pick) && hint_alloc(task)) {
            *work->pick = pick;
            work->mode = HINT_WAIT;
            extra = task->extra;
            extra->home = work->ang.y;
            extra->base_y = work->pos.y;
            extra->ground_y = ground_below(work->pos.x, extra->base_y,
                                           work->pos.z, &work->ang);
            player = player_get(0);
            extra->reach =
                (player != NULL ? player->consts.radius : 0.0F) + REACH;
            collision_init(task, hint_shape, COLLISION_OBJECT, 1, 1);
            task->step = hint_step;
            task->draw = hint_draw;
            task->destructor = hint_destroy;
            return;
        }
        task_destroy(task);
    }
}

/* allocate the box's record, its message's record and its copy of the
 * model, and keep them in the task. returns TRUE, or FALSE with all three
 * freed when the heap has no room for one of them */
static Bool hint_alloc(struct task *task)
{
    struct hint_extra *extra = heap_calloc(1, sizeof(struct hint_extra));
    struct script_pages *pick = heap_alloc(sizeof(struct script_pages));
    NJS_MODEL *model = hint_copy_model();

    if (extra != NULL && pick != NULL && model != NULL) {
        struct hint_work *work = HINT(task);

        task->extra = extra;
        work->pick = pick;
        extra->model = model;
        return TRUE;
    }
    heap_free(extra);
    heap_free(pick);
    if (model != NULL) {
        heap_free(model->mats);
        heap_free(model);
    }
    return FALSE;
}

/* a copy of hint_object's model and materials from the heap, with the lamp
 * off. NULL, with neither kept, when the heap has no room */
static NJS_MODEL *hint_copy_model(void)
{
    NJS_MODEL *from = hint_object->model;
    NJS_MODEL *model = heap_alloc(sizeof(NJS_MODEL));
    NJS_MATERIAL *mats = heap_alloc(from->nbMat * sizeof(NJS_MATERIAL));

    if (model != NULL && mats != NULL) {
        njMemCopy4(model, from, sizeof(NJS_MODEL) / 4);
        njMemCopy4(mats, from->mats, from->nbMat * (sizeof(NJS_MATERIAL) / 4));
        model->mats = mats;
        mats[LAMP_MATERIAL].diffuse.color = GRAY(0);
    } else {
        heap_free(model);
        heap_free(mats);
        model = NULL;
    }
    return model;
}

/* set extra->aim to the heading that faces the box to player 0 */
static void hint_aim(struct task *task)
{
    struct hint_work *work = HINT(task);
    struct hint_extra *extra = task->extra;
    NJS_POINT3 player;

    player_get_history(0, 0, &player, NULL);
    njSubVector(&player, &work->pos);
    extra->aim = (Sint16)(angle_arc_tan2(player.x, player.z) + NJM_DEG_ANG(90));
}

/* TRUE while player 0 is within reach of the box across x and z, and
 * within FACING of its home heading. whenever the player is within reach,
 * it sets extra->aim */
static Bool hint_aim_and_test_reach(struct task *task, Float reach)
{
    struct hint_work *work = HINT(task);
    struct hint_extra *extra = task->extra;
    NJS_POINT3 player;

    player_get_history(0, 0, &player, NULL);
    if (point_distance_squared_across(&player, &work->pos) > reach * reach) {
        return FALSE;
    }
    hint_aim(task);
    if (ABS((Sint16)(extra->aim - extra->home)) > FACING) {
        return FALSE;
    }
    return TRUE;
}

/* turn the box for a frame toward extra->aim. returns TRUE once it is
 * within TURN_CLOSE of the aim */
static Bool hint_turn_to_player(struct task *task)
{
    Bool done = FALSE;
    struct hint_extra *extra = task->extra;
    struct hint_work *work = HINT(task);
    int turn = (Sint16)(extra->aim - work->ang.y);
    Angle speed = extra->turn;

    if (ABS(turn) < TURN_CLOSE) {
        done = TRUE;
    }
    speed += TURN_EASE(speed, turn);
    if (done && ABS(speed) < TURN_SLOW) {
        speed = 0;
    }
    work->ang.y += speed;
    extra->turn = speed;
    return done;
}

/* turn the box for a frame toward its home heading. returns TRUE, with the
 * turn stopped, once it is within TURN_CLOSE of home and its speed is
 * under TURN_SLOW */
static Bool hint_turn_home(struct task *task)
{
    Bool done = FALSE;
    struct hint_extra *extra = task->extra;
    struct hint_work *work = HINT(task);
    int turn = (Sint16)(extra->home - work->ang.y);
    Angle speed = extra->turn;

    speed += TURN_EASE(speed, turn);
    if (ABS(turn) < TURN_CLOSE && ABS(speed) < TURN_SLOW) {
        speed = 0;
        done = TRUE;
    }
    work->ang.y += speed;
    extra->turn = speed;
    return done;
}

/* @hack spelling: the lamp turned on and off through helpers SHC expands,
 * which load task into R4 ahead of the BSR to hint_set_lamp and on into R5
 * in its slot (0x8c06464a, 0x8c064650 in hint_wait). called directly, the
 * two loads trade places, and summed over hint_wait, hint_near and
 * hint_speak, 111 of 150 words differ */
static struct task *hint_lamp_on(struct task *task)
{
    return hint_set_lamp(task, TRUE);
}

static struct task *hint_lamp_off(struct task *task)
{
    return hint_set_lamp(task, FALSE);
}

/* once player 0 is within reach and in front, light the lamp and add the
 * box's talk. outside an event, the player also looks at the box and SE_NEAR
 * plays */
static void hint_wait(struct task *task)
{
    struct hint_work *work = HINT(task);
    struct hint_extra *extra = task->extra;

    if (hint_aim_and_test_reach(task, extra->reach)) {
        work->mode = HINT_NEAR;
        work->lamp = hint_lamp_on(task);
        if (!event_in_control()) {
            player_set_look_point(0, &work->pos);
            sound_play(SE_NEAR, 0, 0, 0);
        }
        talk_add(task, &work->pos, NULL, &work->talking, TALK_RADIUS, 0);
    }
}

static void hint_return(struct task *task)
{
    if (hint_turn_home(task)) {
        struct hint_work *work = HINT(task);

        work->mode = HINT_WAIT;
    }
    hint_wait(task);
}

static void hint_near(struct task *task)
{
    struct hint_work *work = HINT(task);
    struct hint_extra *extra = task->extra;

    if (!hint_aim_and_test_reach(task, extra->reach + TURN_BACK_MARGIN)) {
        work->mode = HINT_RETURN;
        work->lamp = hint_lamp_off(task);
        if (!event_in_control()) {
            player_clear_look(0);
        }
        talk_remove(task);
    } else {
        hint_turn_to_player(task);
        if (work->talking) {
            time_stop();
            work->mode++;
        }
    }
}

/* once the box faces player 0, run the box's script again and show the
 * message it picks. where the heap has no room or the script fails, show
 * the message picked before. then point the camera at the box */
static void hint_turn(struct task *task)
{
    struct hint_work *work = HINT(task);
    struct script_pages *pick;

    if (hint_turn_to_player(task)) {
        message_set_shown(&work->shown);
        pick = heap_alloc(sizeof(struct script_pages));
        if (pick != NULL && script_pick((int)work->row, pick)) {
            heap_free(work->pick);
            work->pick = pick;
        } else {
            heap_free(pick);
        }
        script_show(work->pick);
        hint_take_camera(task);
        work->mode++;
    }
}

static void hint_speak(struct task *task)
{
    struct hint_work *work = HINT(task);

    hint_turn_to_player(task);
    if (!work->shown) {
        work->mode = HINT_WAIT;
        work->lamp = hint_lamp_off(task);
        if (!event_in_control()) {
            player_clear_look(0);
        }
        talk_remove(task);
        hint_wait(task);
        time_start();
        camera_restore();
    }
}

static void hint_step(struct task *task)
{
    struct hint_work *work = HINT(task);
    struct hint_extra *extra = task->extra;
    Float sine;
    Float cosine;

    if (placement_try_leave(task)) {
        return;
    }
    if (placement_editing(task)) {
        hint_print_labels();
        extra->ground_y =
            ground_below(work->pos.x, extra->base_y, work->pos.z, &work->ang);
    }
    if (event_in_control() && !event_flags[FLAG_HINTS_IN_EVENTS]) {
        return;
    }
    /* the modes, the bob and the draw run on a frame's first step only, so
     * the box counts frames, not steps */
    if (!frame_step) {
        hint_modes[work->mode](task);
        extra->bob += BOB_STEP;
        _builtin_fsca(extra->bob, &sine, &cosine);
        work->pos.y = extra->base_y + (sine * BOB_HEIGHT);
        extra->lift = extra->ground_y - work->pos.y + SHADOW_LIFT;
        hint_draw(task);
    }
    collision_register((struct task_work *)work);
    task_step_children(task);
}

/* draw the box, and below it its shadow, which shrinks as the ground falls
 * away. it draws only in view, on a frame's first step */
static void hint_draw(struct task *task)
{
    struct hint_work *work = HINT(task);
    struct hint_extra *extra = task->extra;
    Float lift;
    Float scale;

    if (!frame_step &&
        draw_in_view(work->pos.x + hint_shape->center.x,
                     work->pos.y + hint_shape->center.y,
                     work->pos.z + hint_shape->center.z, hint_shape->radius)) {
        texture_set_regular();
        draw_control_save();
        draw_control_clear(NJD_CONTROL_3D_CONSTANT_TEXTURE_MATERIAL);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        if (extra->model != NULL) {
            njDrawModel(extra->model);
        }
        lift = extra->lift;
        njTranslate(NULL, 0.0F, lift, 0.0F);
        scale = 1.5F / (1.0F - (lift * 0.03F));
        njScale(NULL, scale, scale, scale * 0.8F);
        njEasyDrawModel(&shadow_model);
        njPopMatrix(1);
        draw_control_restore();
    }
}

static void hint_destroy(struct task *task)
{
    struct hint_work *work = HINT(task);
    struct hint_extra *extra = task->extra;
    NJS_MODEL *model = extra->model;

    if (model != NULL) {
        heap_free(model->mats);
        heap_free(model);
    }
    talk_remove(task);
    heap_free(work->pick);
}

/* the lamp's step. each frame, it fades the level by LAMP_STEP toward on
 * or off and grays the lamp's material by it. once timer is 0, it sets the
 * level and the gray to their ends instead */
static void hint_lamp_step(struct task *task)
{
    struct hint_lamp *lamp;
    Uint32 *diffuse;
    Bool on;
    Uint8 gray;

    if (!frame_step) {
        lamp = task->extra;
        diffuse = &(*lamp->model)->mats[LAMP_MATERIAL].diffuse.color;
        on = lamp->on;
        if (!lamp->timer) {
            gray = hint_lamp_grays[on];
            lamp->level = hint_lamp_levels[on];
            *diffuse = GRAY(gray);
            return;
        }
        lamp->timer--;
        lamp->level += !on ? -LAMP_STEP : LAMP_STEP;
        gray = lamp->level * LAMP_GRAY;
        *diffuse = GRAY(gray);
    }
}

/* turn the box's lamp on or off and return its task: work->lamp, or a new
 * child. with no room for a new one, set the lamp's gray at once, to 0 or
 * 0xff, and return NULL. a box with no model returns NULL */
static struct task *hint_set_lamp(struct task *task, Bool on)
{
    struct hint_extra *extra = task->extra;
    struct hint_work *work = HINT(task);
    struct task *current = work->lamp;
    struct task *child;
    struct hint_lamp *lamp;
    Uint32 *diffuse;
    Uint8 gray;

    if (extra->model == NULL) {
        return NULL;
    }
    if (current != NULL) {
        lamp = current->extra;
        if (lamp->on != on) {
            lamp->on = on;
            /* turned back midway, the lamp fades back for as many frames as
             * it has faded: LAMP_FRAMES less timer. SHC reads 5.9999999F as
             * just under 6 (0x40bfffff at 0x8c064c10), so the difference
             * truncates to that */
            lamp->timer = (int)(5.9999999F - lamp->timer);
        }
        return current;
    }
    if ((child = task_create_child(0, hint_lamp_step, task)) != NULL) {
        /* @hack redundant: an else after the return, which loads
         * task_destroy's address into R2 (MOV.L at 0x8c064c38). without it,
         * R3 takes it and 2 of 98 words differ */
        if ((lamp = heap_calloc(1, sizeof(struct hint_lamp))) != NULL) {
            child->extra = lamp;
            lamp->on = on;
            /* @bug logic: a new lamp turning off starts at level 0xff,
             * not 1 (MOV.W of 0xff at 0x8c064bde). for its LAMP_FRAMES
             * frames, its gray is then the low byte of level * LAMP_GRAY
             * (EXTU.B at 0x8c064b5a), not a fade from LAMP_GRAY to 0 */
            lamp->level = !on ? 0xff : 0;
            lamp->model = &extra->model;
            lamp->timer = LAMP_FRAMES;
            return child;
        } else {
            task_destroy(child);
        }
    }
    gray = !on ? 0 : 0xff;
    diffuse = &extra->model->mats[LAMP_MATERIAL].diffuse.color;
    *diffuse = GRAY(gray);
    return NULL;
}

/* point the camera at the box from player 0's side, camera_distance away
 * across x and z and camera_height above the box */
static void hint_take_camera(struct task *task)
{
    struct hint_work *work = HINT(task);
    struct task_work *camera = camera_work;
    NJS_POINT3 pos;
    NJS_VECTOR way;
    Float distance;

    camera_override(camera_step_work_view, 1, CAMERA_LOOK_AT);
    player_get_history(0, 0, &pos, NULL);
    distance = work->camera_distance;
    VECTOR_DIFF(&way, &pos, &work->pos);
    if (distance < 0.1F) {
        distance = CAMERA_DISTANCE;
    }
    way.y = 0.0F;
    vector_unit(&way, &way);
    VECTOR_SCALE(&way, &way, distance);
    way.y = work->camera_height;
    VECTOR_SUM(&pos, &way, &work->pos);
    camera->scl = pos;
    CAMERA_VARS(camera)->target = work->pos;
}

/* label the set editor's scale rows with what the box reads from them */
static void hint_print_labels(void)
{
    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "<-CAM Dist(def. 80.0)");
    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), "<-CAM Hight");
    njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), "<-Message ID");
}
