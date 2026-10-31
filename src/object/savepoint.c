/* the savepoint: a gate with two lamps. player 0's touch saves player 0's
 * place as the restart point */
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/helpers.h"
#include "draw/model.h"
#include "effect/sparkle.h"
#include "hud/hud.h"
#include "object/models.h"
#include "object/placement.h"
#include "object/savepoint.h"
#include "player/player.h"
#include "player/target.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/texture.h"
#include "system/util.h"

/* the steps the saved time shows for after a touch */
#define SHOW_TIME 300
/* the saved time blinks for its first 120 steps: it hides through every
 * other run of BLINK_STEPS steps */
#define BLINK_STEPS 10

#define SE_TOUCH 10

/* the lamps' textures in the regular texture list, unlit and lit */
#define LAMP_UNLIT 59
#define LAMP_LIT 60

/* the lamps keep SPIN_KEPT of their spin about y each step. once the spin
 * falls below SWING_SPIN a step, they swing about x toward 0 */
#define SPIN_KEPT 0.995F
#define SWING_SPIN NJM_DEG_ANG(20)

/* where a player starts again, and the clock's time when it was saved */
struct restart_point {
    Uint16 stage_act; /* the stage and act it was saved in */
    Uint8 minutes;
    Uint8 seconds;
    Uint8 frames;
    Bool saved;
    NJS_POINT3 pos;
    struct angles ang;
};

/* a savepoint's own record, reached through task->extra */
struct savepoint {
    Uint8 unk_00[4];
    struct angles ang; /* the lamps' turn: x tilts them and y spins them */
    struct angles speed; /* added to ang.x and ang.y each step */
    struct task *lamps[2]; /* child tasks whose works follow the lamps */
    /* the constant material's red, green and blue as a lamp draws. this
     * file leaves them at 0 */
    Float color[3];
    Sint32 timer; /* the steps left to show the saved time */
    Sint16 sparkles; /* the sparkles made above player 0 so far */
};

struct savepoint_extra {
    struct savepoint *savepoint;
};

/* a savepoint's bytes in work->vars */
struct savepoint_vars {
    NJS_OBJECT *object; /* the tree it draws, set by savepoint_draw */
};

#define VARS(work) ((struct savepoint_vars *)&(work)->vars)

/* @hack spelling: the record reached through task->extra at every use, which
 * the image reads again each time (MOV.L @(44,R14) at 0x8c06867a,
 * 0x8c06869a and 0x8c0686a6); held in a local in each function, 258 of 297
 * words differ */
#define EXTRA(task) ((struct savepoint_extra *)(task)->extra)

/* a savepoint's modes, in work->mode */
enum savepoint_mode {
    SAVEPOINT_INIT,
    SAVEPOINT_WAIT, /* for player 0's touch */
    SAVEPOINT_SPIN, /* touched: the lamps spin, then turn to rest */
    SAVEPOINT_DONE, /* touched, with the lamps at rest */
    SAVEPOINT_DRAW, /* it only draws */
    SAVEPOINT_KILL
};

static void savepoint_destroy(struct task *task);
static void savepoint_draw(struct task *task);
static struct savepoint *savepoint_alloc(void);
static void lamp_draw(NJS_OBJECT *object);
static void lamp_step(struct task *task);
static void lamps_place(struct task_work *work);
static void savepoint_touch(struct task *task, struct task_work *work);
static void lamps_swing(struct task *task, struct task_work *work);
static void savepoint_show_time(struct task *task, struct task_work *work);
static Float savepoint_pass_speed(struct task *task, struct task_work *work);

/* the gate's one shape, for a touch */
/* clang-format off */
static struct collision_shape savepoint_shape = {
    SHAPE_RECTANGLE, 0, PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
    {0.0F, 6.0F, 1.0F}, 12.0F, 4.0F, 0.5F, {0}, 0, NULL};
/* clang-format on */

/* draw_object leaves the lamps to lamp_draw */
static struct draw_override lamp_overrides[] = {
    {savepoint_lamp_plus_x, lamp_draw},
    {savepoint_lamp_minus_x, lamp_draw},
    {NULL, NULL},
};

/* the lamps' places on the gate, before the gate turns */
static NJS_VECTOR lamp_pos[2] = {{-15.0F, 9.8F, 0.0F}, {15.0F, 9.8F, 0.0F}};

/* where savepoint_show_time draws the saved time */
static struct hud_layout time_layout = {
    {450.0F, 400.0F}, {1.0F, 1.0F}, {30.0F, 0.0F}};

/* the record of the savepoint that last stepped or drew. lamp_draw, which
 * gets no task, reads it */
static struct savepoint *savepoint_current;
static struct restart_point restart_point;

void restart_clear(void)
{
    Uint8 *byte;
    int i;

    byte = (Uint8 *)&restart_point;
    for (i = sizeof(restart_point); i != 0; i--) {
        *byte++ = 0;
    }
}

Bool restart_keep_this_act(void)
{
    if (stage_get_stage_act() != restart_point.stage_act) {
        restart_clear();
    }
    return restart_point.saved;
}

void restart_restore(NJS_POINT3 *pos, struct angles *ang)
{
    *pos = restart_point.pos;
    *ang = restart_point.ang;
    time_set(restart_point.minutes, restart_point.seconds,
             restart_point.frames);
    ring_add(-ring_count());
    player_grant_invincibility(0);
}

void restart_save(NJS_POINT3 *pos, struct angles *ang)
{
    time_get(&restart_point.minutes, &restart_point.seconds,
             &restart_point.frames);
    restart_point.pos = *pos;
    restart_point.ang = *ang;
    restart_point.saved = TRUE;
    restart_point.stage_act = stage_get_stage_act();
}

void savepoint_step(struct task *task)
{
    struct task_work *work;

    work = task->work;
    if (placement_try_leave(task)) {
        return;
    }
    savepoint_current = EXTRA(task)->savepoint;
    /* in the set editor's hands it only draws */
    if (placement_editing(task)) {
        if (work->mode == SAVEPOINT_INIT) {
            work->mode++;
            EXTRA(task)->savepoint = savepoint_alloc();
            task->destructor = savepoint_destroy;
            task->draw = savepoint_draw;
        }
        savepoint_current = EXTRA(task)->savepoint;
        savepoint_draw(task);
    } else {
        switch (work->mode) {
        case SAVEPOINT_INIT:
            work->mode = SAVEPOINT_WAIT;
            EXTRA(task)->savepoint = savepoint_alloc();
            savepoint_current = EXTRA(task)->savepoint;
            task->destructor = savepoint_destroy;
            task->draw = savepoint_draw;
            savepoint_current->lamps[0] =
                task_create_child(TASK_WORK, lamp_step, task);
            savepoint_current->lamps[1] =
                task_create_child(TASK_WORK, lamp_step, task);
            collision_init(task, &savepoint_shape, COLLISION_OBJECT, 1, 1);
            lamps_place(work);
            savepoint_current->ang.x = NJM_DEG_ANG(90);
            savepoint_current->ang.y = NJM_DEG_ANG(-90);
            if (task->placement != NULL) {
                placement_keep(task);
            }
            /* an earlier task of this placement was touched, so this one
             * starts lit with its lamps at rest */
            if (placement_used(task)) {
                savepoint_current->ang.x = 0;
                work->mode = SAVEPOINT_DONE;
            }
            break;
        case SAVEPOINT_WAIT:
            savepoint_draw(task);
            savepoint_touch(task, work);
            break;
        case SAVEPOINT_SPIN:
            savepoint_draw(task);
            savepoint_touch(task, work);
            lamps_swing(task, work);
            savepoint_show_time(task, work);
            break;
        case SAVEPOINT_DONE:
            savepoint_draw(task);
            savepoint_touch(task, work);
            savepoint_show_time(task, work);
            break;
        case SAVEPOINT_DRAW:
            savepoint_draw(task);
            break;
        /* @hack redundant: case SAVEPOINT_KILL, which shares the default's
         * code, keeps the image's CMP/EQ #5 at 0x8c0686e0; without it, 6 of 186
         * words differ */
        case SAVEPOINT_KILL:
        default:
            placement_kill(task);
            break;
        }
    }
}

static void savepoint_destroy(struct task *task)
{
    if (EXTRA(task)->savepoint != NULL) {
        heap_free(EXTRA(task)->savepoint);
        EXTRA(task)->savepoint = NULL;
    }
}

static void savepoint_draw(struct task *task)
{
    struct task_work *work;

    work = task->work;
    if (!draw_in_view(work->pos.x, work->pos.y, work->pos.z, 20.0F)) {
        return;
    }
    savepoint_current = EXTRA(task)->savepoint;
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateZ, work->ang.z);
    TURN(njRotateX, work->ang.x);
    TURN(njRotateY, work->ang.y);
    texture_set_regular();
    if (work->mode != SAVEPOINT_WAIT) {
        savepoint_lamp_plus_x->model->mats->attr_texId = LAMP_LIT;
        savepoint_lamp_minus_x->model->mats->attr_texId = LAMP_LIT;
    } else {
        savepoint_lamp_plus_x->model->mats->attr_texId = LAMP_UNLIT;
        savepoint_lamp_minus_x->model->mats->attr_texId = LAMP_UNLIT;
    }
    VARS(work)->object = &savepoint_gate;
    draw_object(&savepoint_gate, lamp_overrides);
    njPopMatrix(1);
}

/* a new record from the heap, zeroed */
static struct savepoint *savepoint_alloc(void)
{
    struct savepoint *savepoint;
    int i;

    /* @hack redundant: the new record tested with nothing to do (TST R0,R0 at
     * 0x8c0689f6); assigned plainly, 17 of 23 words differ */
    if ((savepoint = heap_alloc(sizeof(*savepoint))) != NULL) {
    }
    /* NOLINTNEXTLINE(clang-diagnostic-sign-compare) */
    for (i = 0; i < sizeof(*savepoint); i++) {
        ((Uint8 *)savepoint)[i] = 0;
    }
    return savepoint;
}

/* draw a lamp turned by the record's angles, the two lamps spinning
 * opposite ways about y. the record's color is its constant material */
static void lamp_draw(NJS_OBJECT *object)
{
    draw_set_material(1.0F, savepoint_current->color[0],
                      savepoint_current->color[1], savepoint_current->color[2]);
    njTranslateV(NULL, (NJS_VECTOR *)object->pos);
    if (object == savepoint_lamp_plus_x) {
        TURN(njRotateY, savepoint_current->ang.y);
        TURN(njRotateX, savepoint_current->ang.x);
    } else {
        TURN(njRotateY, -savepoint_current->ang.y);
        TURN(njRotateX, savepoint_current->ang.x);
    }
    draw_model(object->model);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
}

/* a lamp's task does nothing itself: savepoint_touch turns its work */
static void lamp_step(struct task *task)
{
}

/* move each lamp's task, which starts at the gate's pos, to its lamp's
 * place on the turned gate */
static void lamps_place(struct task_work *work)
{
    NJS_VECTOR offset[2];
    NJS_MATRIX matrix;

    njUnitMatrix(&matrix);
    TURN_MATRIX(njRotateZ, &matrix, work->ang.z);
    TURN_MATRIX(njRotateX, &matrix, work->ang.x);
    TURN_MATRIX(njRotateY, &matrix, work->ang.y);
    njCalcVector(&matrix, &lamp_pos[0], &offset[0]);
    njCalcVector(&matrix, &lamp_pos[1], &offset[1]);
    savepoint_current->lamps[0]->work->pos.x += offset[0].x;
    savepoint_current->lamps[0]->work->pos.y += offset[0].y;
    savepoint_current->lamps[0]->work->pos.z += offset[0].z;
    savepoint_current->lamps[1]->work->pos.x += offset[1].x;
    savepoint_current->lamps[1]->work->pos.y += offset[1].y;
    savepoint_current->lamps[1]->work->pos.z += offset[1].z;
}

/* turn the lamps' tasks as the lamps turn. in SAVEPOINT_WAIT, a touch by
 * player 0 saves player 0's place as the restart point and starts the
 * spin */
static void savepoint_touch(struct task *task, struct task_work *work)
{
    savepoint_current->lamps[0]->work->ang.x =
        savepoint_current->ang.x + work->ang.x;
    savepoint_current->lamps[0]->work->ang.y =
        -savepoint_current->ang.y + work->ang.y;
    savepoint_current->lamps[0]->work->ang.z =
        savepoint_current->ang.z + work->ang.z;
    savepoint_current->lamps[1]->work->ang.x =
        savepoint_current->ang.x + work->ang.x;
    savepoint_current->lamps[1]->work->ang.y =
        savepoint_current->ang.y + work->ang.y;
    savepoint_current->lamps[1]->work->ang.z =
        savepoint_current->ang.z + work->ang.z;
    if (work->mode == SAVEPOINT_WAIT) {
        if ((work->collision->flags & COLLISION_HIT) &&
            work->collision->hit->owner == player_get_task(0)) {
            work->mode = SAVEPOINT_SPIN;
            /* ten degrees a step for each unit of player 0's speed */
            savepoint_current->speed.y =
                savepoint_pass_speed(task, work) * (10.0F * ANGLE_PER_DEGREE);
            savepoint_current->timer = SHOW_TIME;
            restart_save(&player_work[0]->pos, &player_work[0]->ang);
            placement_set_used(task);
            sound_play(SE_TOUCH, 0, 0, 0);
        }
    }
    collision_register(work);
}

/* the lamps spin about y, slowing, then swing about x toward 0, faster as
 * the spin dies. SAVEPOINT_DONE follows once both speeds reach 0. each of
 * the first three steps leaves a sparkle above player 0 */
static void lamps_swing(struct task *task, struct task_work *work)
{
    /* Sint16 copies of the record's ang and speed, x and y */
    /* @hack type: each array's z, which nothing reads, gives the image's
     * 44 bytes of locals (ADD #-44,R15 at 0x8c068cea); as arrays of two, 3
     * of 155 words differ */
    Sint16 speed[3];
    Sint16 ang[3];
    NJS_VECTOR velocity;
    NJS_POINT3 pos;
    Sint16 from;
    Sint16 spin;
    Sint16 push;

    ang[0] = savepoint_current->ang.x;
    ang[1] = savepoint_current->ang.y;
    speed[0] = savepoint_current->speed.x;
    speed[1] = savepoint_current->speed.y;
    ang[0] += speed[0];
    ang[1] += speed[1];
    speed[1] = speed[1] * SPIN_KEPT;
    spin = njAbs(speed[1]);
    if (spin < SWING_SPIN) {
        if (ang[0] > NJM_DEG_ANG(45)) {
            from = NJM_DEG_ANG(101) - ang[0];
        } else {
            from = ang[0];
        }
        /* @hack spelling: the push through a local, which puts its NEG in R4
         * (NEG R3,R4 at 0x8c068d7e); stored straight to speed[0], the NEG
         * stays in R3 and 2 of 155 words differ */
        push = -(int)((SWING_SPIN - spin) * (Float)from * (1.0F / 81920.0F));
        speed[0] = push;
    }
    savepoint_current->ang.x = ang[0];
    savepoint_current->ang.y = ang[1];
    savepoint_current->speed.x = speed[0];
    savepoint_current->speed.y = speed[1];
    if (speed[0] == 0 && speed[1] == 0) {
        work->mode = SAVEPOINT_DONE;
    } else if (savepoint_current->sparkles < 3) {
        velocity.x = 0.0F;
        velocity.y = -0.3F;
        velocity.z = 0.0F;
        pos = player_work[0]->pos;
        pos.y += 4.0F;
        sparkle_create(&pos, &velocity, 0.1F, NJM_DEG_ANG(5));
        savepoint_current->sparkles++;
    }
}

/* count the timer down, drawing the saved time while it runs */
static void savepoint_show_time(struct task *task, struct task_work *work)
{
    if (savepoint_current->timer > 0) {
        savepoint_current->timer--;
        switch ((SHOW_TIME - savepoint_current->timer) / BLINK_STEPS) {
        case 1:
        case 3:
        case 5:
        case 7:
        case 9:
        case 11:
            break;
        default:
#ifdef GCC
            /* the port's 16:9 squeezes the time about the screen's right
             * edge */
            port_wide_anchor = WIDE_RIGHT;
#endif
            hud_draw_time(&time_layout, restart_point.minutes,
                          restart_point.seconds);
#ifdef GCC
            port_wide_anchor = WIDE_MIDDLE;
#endif
            break;
        }
    }
}

/* player 0's speed, negative when player 0 passes through the gate
 * backward */
static Float savepoint_pass_speed(struct task *task, struct task_work *work)
{
    Float speed;
    Angle difference;

    speed = njScalor(&player_get(0)->speed);
    difference =
        angle_difference(NJM_DEG_ANG(90) - player_work[0]->ang.y, work->ang.y);
    if ((int)njAbs(difference) > NJM_DEG_ANG(90)) {
        return -speed;
    }
    return speed;
}
