/* the O TELEPOT teleporter, and the props the object list names */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/placement.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stg09/casino.h"
#include "stg09/helpers.h"
#include "stg09/props.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* @hack type: int, not stage.h's Uint16, so SHC compares the result without
 * extending it, as at 0x0c91176a and 0x0c911a8c. as stage.h declares it,
 * summed over telepot_step, telepot_destroy and telepot_run, 209 of 482 words
 * differ */
extern int stage_get_stage_act(void);
/* @hack type: vibration_start declared without a prototype, so the hold's
 * call can pass three arguments. prototyped, with a time of 0 as a fourth,
 * 104 of telepot_run's 372 words differ */
/* NOLINTNEXTLINE(clang-diagnostic-strict-prototypes) */
extern void vibration_start();

/* the teleporter's root model and its one child, and the meshsets whose
 * UVs scroll */
extern NJS_OBJECT telepot_object;
extern NJS_MESHSET telepot_meshsets[];
extern NJS_MESHSET telepot_child_meshsets[];

extern NJS_OBJECT light_a_object;
extern NJS_OBJECT light_b_object;
extern NJS_OBJECT fk_a_object;
extern NJS_OBJECT fk_b_object;
extern NJS_OBJECT slotisu_object;
extern NJS_OBJECT dento_object;
extern NJS_OBJECT flower_a_object;
extern NJS_OBJECT flower_b_object;
extern NJS_OBJECT saku_object;
extern NJS_OBJECT taru_object;
extern NJS_OBJECT hako_object;
extern NJS_OBJECT gbox_object;
extern NJS_OBJECT gomi_b_object;
extern NJS_ACTION gomi_b_action;
extern NJS_OBJECT gomi_r_object;
extern NJS_ACTION gomi_r_action;
extern NJS_OBJECT rlt_object;
extern NJS_OBJECT yukh_object;
extern NJS_MESHSET yukh_meshsets[];
extern NJS_OBJECT rlti_object;
extern NJS_MESHSET rlti_meshsets[];
extern NJS_OBJECT kb_b_object;
extern NJS_MESHSET kb_b_meshsets[];
extern NJS_OBJECT kb_r_object;
extern NJS_MESHSET kb_r_meshsets[];
extern NJS_OBJECT kb_1_object;
extern NJS_MESHSET kb_1_meshsets[];
extern NJS_OBJECT kb_2_object;
extern NJS_MESHSET kb_2_meshsets[];
extern NJS_OBJECT kb_3_object;
extern NJS_MESHSET kb_3_meshsets[];
extern NJS_OBJECT kb_4_object;
extern NJS_MESHSET kb_4_meshsets[];
extern NJS_OBJECT tdenki_object;
extern NJS_MESHSET tdenki_meshsets[];
extern NJS_OBJECT sdfuti_r_object;
extern NJS_MESHSET sdfuti_r_meshsets[];
extern NJS_OBJECT sdfuti_b_object;
extern NJS_MESHSET sdfuti_b_meshsets[];
extern NJS_OBJECT map2f_object;
extern NJS_OBJECT cfo_object;
extern NJS_MATERIAL cfo_materials[];
extern NJS_OBJECT mizu_a_object;
extern NJS_MESHSET mizu_a_meshsets[];
extern NJS_OBJECT mizu_b_object;
extern NJS_MESHSET mizu_b_meshsets[];
extern NJS_OBJECT sy_a_object;
extern NJS_OBJECT sy_b_object;
extern NJS_OBJECT shwrl_object;
extern NJS_OBJECT shwhd_object;
extern NJS_OBJECT neokab_object;
extern NJS_MESHSET neokab_meshsets[];

/* placement_try_leave_by's squared range: about 600 from the camera or
 * player 0 */
#define RANGE 360010.0F

/* the hold's sound, which each step of the hold keeps playing */
#define SE_HOLD 269

/* a teleporter's work->mode */
enum telepot_mode { TELEPOT_WAITING, TELEPOT_PULLING, TELEPOT_HOLDING };

/* stage_get_stage_act's result as the teleporter started */
#define STARTED_IN(work) ((work)->vars.w[5])

/* the angle player 0 turns by about y each step of a hold. the hold's nth
 * step turns him by n / 40 of it. from the 31st he turns by all of it */
static int telepot_spin = 2300;
static Float telepot_pull_speed = 2.0F; /* player 0's speed in a pull */
/* in a hold, the root model fades in over telepot_fade_steps. the child
 * then fades in over telepot_child_fade_steps. after that, each model's x
 * and z scale by its shrink each step */
static int telepot_fade_steps = 30;
static int telepot_child_fade_steps = 25;
/* the angle each model turns by about y each step of a hold */
static int telepot_turn = 10;
static int telepot_child_turn = -1100;
static Float telepot_shrink = 0.975F;
static Float telepot_child_shrink = 0.975F;
static Float telepot_child_scale = 2.4F; /* the child's, in x and z */
/* two words the image holds here, which no code reads */
static int dat_0cb14154 = 5;
static int dat_0cb14158 = 4;
/* the phase the UVs scroll by each step of a hold */
static Sint16 telepot_scroll_rate = 9;
static struct uv_scroll telepot_scrolls[2] = {
#include "stg09/props/telepot_scrolls.inc"
};
static struct collision_shape telepot_shape = {
    SHAPE_CYLINDER | SHAPE_NO_PUSH,
    0,
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 20.0F, 0.0F},
    5.0F,
    20.0F,
    0.0F,
    {0},
    0,
    NULL,
};
/* the teleporter whose hold began while no other held, until that hold
 * ends. NULL while none holds */
static struct task *telepot_holder = NULL;

static struct collision_shape flower_a_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 3.0F, 0.0F},
    5.0F,
    3.0F,
    11.0F,
    {0},
    0,
    NULL,
};
static struct collision_shape flower_b_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, -0.5F, 0.0F},
    5.0F,
    3.25F,
    16.0F,
    {0},
    0,
    NULL,
};
static struct collision_shape saku_shapes[2] = {
    {
        SHAPE_CYLINDER2,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 6.1F, 0.0F},
        1.5F,
        23.0F,
        0.0F,
        {0, 0, NJM_DEG_ANG(90)},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 2.3F, 0.0F},
        22.5F,
        2.3F,
        0.1F,
        {0},
        0,
        NULL,
    },
};
static struct collision_shape taru_shape = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 6.6F, 0.0F},
    5.4F,
    6.6F,
    0.0F,
    {0},
    0,
    NULL,
};
static struct collision_shape dat_0cb142bc = {
    SHAPE_CYLINDER,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, -1.5F, 0.0F},
    2.0F,
    3.5F,
    0.0F,
    {0},
    0,
    NULL,
};
static struct collision_shape hako_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 7.5F, 0.0F},
    7.0F,
    7.5F,
    10.0F,
    {0},
    0,
    NULL,
};
static struct collision_shape gbox_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 7.5F, 0.0F},
    7.0F,
    7.5F,
    10.0F,
    {0},
    0,
    NULL,
};
static struct collision_shape gomi_b_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 5.0F, 0.0F},
    3.0F,
    5.0F,
    2.0F,
    {0},
    0,
    NULL,
};
static struct collision_shape gomi_r_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 5.0F, 0.0F},
    3.0F,
    5.0F,
    2.0F,
    {0},
    0,
    NULL,
};
static struct collision_shape kb_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.6814999F},
    27.0F,
    19.0F,
    0.80849994F,
    {0},
    0,
    NULL,
};
static struct collision_shape sy_shapes[2] = {
    {
        SHAPE_RECTANGLE,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 7.0F, 1.5F},
        20.0F,
        7.0F,
        1.5F,
        {0},
        0,
        NULL,
    },
    {
        SHAPE_RECTANGLE2,
        0,
        PUSH_ALL | PUSH_FIRM(PUSH_ALL),
        {0.0F, 3.43F, 4.45F},
        20.0F,
        3.43F,
        4.45F,
        {NJM_DEG_ANG(20), 0, 0},
        0,
        NULL,
    },
};
static struct collision_shape shwrl_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.1F, 1.5F},
    1.8F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL,
};
static struct collision_shape shwhd_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 2.35F},
    1.5F,
    1.15F,
    0.0F,
    {NJM_DEG_ANG(90), 0, 0},
    0,
    NULL,
};
static struct collision_shape slotisu_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.19999981F, 4.5F, 0.405F},
    4.5F,
    12.499999F,
    2.495F,
    {0, 0, NJM_DEG_ANG(90)},
    0,
    NULL,
};
static struct collision_shape dento_shape = {
    SHAPE_CAPSULE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {1.6999999F, 35.0F, 0.0F},
    10.0F,
    25.0F,
    0.0F,
    {0},
    0,
    NULL,
};
static struct collision_shape sdfuti_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 28.0F, 0.0F},
    5.0F,
    28.0F,
    5.0F,
    {0},
    0,
    NULL,
};
static struct collision_shape map2f_shape = {
    SHAPE_RECTANGLE,
    0,
    PUSH_ALL | PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 1.0F},
    23.0F,
    26.0F,
    1.0F,
    {0},
    0,
    NULL,
};
static struct texture_swap cfo_swaps[8] = {
#include "stg09/props/cfo_swaps.inc"
};
static struct uv_shift yukh_shifts[3] = {
#include "stg09/props/yukh_shifts.inc"
};
static struct uv_shift rlti_shifts[9] = {
#include "stg09/props/rlti_shifts.inc"
};
static struct uv_shift kb_b_shifts[6] = {
#include "stg09/props/kb_b_shifts.inc"
};
static struct uv_shift kb_r_shifts[6] = {
#include "stg09/props/kb_r_shifts.inc"
};
static struct uv_shift kb_1_shifts[1] = {
#include "stg09/props/kb_1_shifts.inc"
};
static struct uv_shift kb_2_shifts[1] = {
#include "stg09/props/kb_2_shifts.inc"
};
static struct uv_shift kb_3_shifts[1] = {
#include "stg09/props/kb_3_shifts.inc"
};
static struct uv_shift kb_4_shifts[1] = {
#include "stg09/props/kb_4_shifts.inc"
};
static struct uv_shift tdenki_shifts[1] = {
#include "stg09/props/tdenki_shifts.inc"
};
static struct uv_shift sdfuti_r_shifts[3] = {
#include "stg09/props/sdfuti_r_shifts.inc"
};
static struct uv_shift sdfuti_b_shifts[3] = {
#include "stg09/props/sdfuti_b_shifts.inc"
};
static struct uv_scroll mizu_a_scrolls[2] = {
#include "stg09/props/mizu_a_scrolls.inc"
};
static struct uv_scroll mizu_b_scrolls[1] = {
#include "stg09/props/mizu_b_scrolls.inc"
};
static struct uv_shift neokab_shifts[6] = {
#include "stg09/props/neokab_shifts.inc"
};

static void telepot_destroy(struct task *task);
static void telepot_run(struct task *task);
static void telepot_draw(struct task *task);

/* set each offset color of the players' palette to color */
static void telepot_set_offsets(NJS_BGRA *color)
{
    Uint32 offset = ((Uint32)color->a << 24) | ((Uint32)color->r << 16) |
                    ((Uint32)color->g << 8) | color->b;
    Uint32 *entry = draw_palettes[DRAW_PLAYER_PALETTE];
    int left = DRAW_PALETTE_ENTRIES;

    while (left--) {
        entry++;
        *entry++ = offset;
    }
}

void telepot_step(struct task *task)
{
    struct task_work *work = task->work;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    if (!placement_editing(task)) {
        collision_init(task, &telepot_shape, COLLISION_OBJECT, 1, 1);
        work->collision->shapes->radius = _builtin_fabsf(work->scl.y + 10.0F);
    }
    work->mode = TELEPOT_WAITING;
    work->timer = 0;
    STARTED_IN(work) = stage_get_stage_act();
    work->vars.w[4] = 0;
    task->step = telepot_run;
    task->draw = telepot_draw;
    task->destructor = telepot_destroy;
    uv_scroll_save(telepot_scrolls, 2);
}

static void telepot_destroy(struct task *task)
{
    struct task_work *work = task->work;
    NJS_BGRA color;

    if (work->mode == TELEPOT_HOLDING &&
        STARTED_IN(work) == stage_get_stage_act()) {
        color.a = 0;
        color.r = 0;
        color.g = 0;
        color.b = 0;
        telepot_set_offsets(&color);
    }
    if (telepot_holder == task) {
        telepot_holder = NULL;
    }
    uv_scroll_release(telepot_scrolls, 2);
}

static void telepot_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;
    Float distance;
    Float scaled;
    Float ratio;
    Angle spin;
    Angle angle;
    Sint16 *low;
    int timer;

    if (placement_try_leave_by(task, RANGE)) {
        return;
    }
    telepot_draw(task);
    if (placement_editing(task)) {
        NJS_POINT3 center;

        center = work->pos;
        center.y += 20.0F;
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_X), "-> Act");
        njPrintC(EDITOR_LABEL_RIGHT(EDITOR_ROW_SCL_Y), "->Size");
        casino_draw_cylinder(&center, work->scl.y + 10.0F, 20.0F);
        return;
    }
    /* @bug memory: a teleporter the set editor held as it started has no
     * collision record. each case reads the record's flags through null, at
     * 0x0c911844, 0x0c9119e2 and 0x0c911b5a */
    switch (work->mode) {
    case TELEPOT_PULLING: {
        NJS_POINT3 offset;

        if (work->collision->flags & COLLISION_HIT) {
            player = player_work[0];
            /* the image calls njScalor at 0x0c91185a and drops its result */
            njScalor(&player_motion[0]->speed);
            {
                /* @hack spelling: the differences taken through cursors, for
                 * the image's post-increment loads (0x0c911868 to
                 * 0x0c911878). through the fields, with the test below gone
                 * too, 306 of 372 words differ */
                Float *to = VECTOR_FLOATS(&player->pos);
                Float *out = VECTOR_FLOATS(&offset);
                Float *from = VECTOR_FLOATS(&work->pos);

                *out++ = *from++ - *to++;
                *out++ = *from++ - *to++;
                *out = *from - *to;
                /* @hack redundant: a test of from and out that never runs.
                 * its uses give from R4 and out R5, with to in R6 (0x0c91185e
                 * to 0x0c911866). without it, from takes R6, out R4 and to
                 * R5, and 75 of 372 words differ */
                if (0) {
                    if (from && out) {
                    }
                }
            }
            offset.y -= 10.0F;
            player_get(0)->launch_steps = 3;
            distance = njScalor(&offset);
            if ((offset.x * offset.x) + (offset.z * offset.z) < 10.0F) {
                if (!telepot_holder) {
                    telepot_holder = task;
                }
                work->mode = TELEPOT_HOLDING;
            } else {
                Float scale = telepot_pull_speed / distance;
                /* @hack spelling: the offset scaled through a cursor, which R4
                 * steps across the three products (0x0c911950 to
                 * 0x0c911964). through the fields, 53 of 372 words differ */
                Float *at = VECTOR_FLOATS(&offset);

                *at++ *= scale;
                *at++ *= scale;
                *at *= scale;
                angle = NJM_DEG_ANG(180) -
                        (Angle)(atan2f(offset.z, offset.x) * ANGLE_PER_RADIAN);
                player_set_angles(0, 0, angle + NJM_DEG_ANG(90), 0);
                player_set_speed(0, offset.x, offset.y, offset.z);
                break;
            }
        } else {
            work->mode = TELEPOT_WAITING;
        }
        work->timer = 0;
        break;
    }
    case TELEPOT_HOLDING: {
        NJS_BGRA color;

        if (work->collision->flags & COLLISION_HIT) {
            player = player_work[0];
            player_get(0)->launch_steps = 3;
            /* @bug mismatched-call: vibration_start takes four arguments and
             * gets three. its time, R7 at the JSR at 0x0c911a04, is slot_count
             * as placement_editing left it (MOV.W at 0x8c04551e): 1 until the
             * set editor changes it */
            /* NOLINTNEXTLINE(clang-diagnostic-deprecated-non-prototype) */
            vibration_start(2, 50, 0);
            sound_play_for(SE_HOLD, (int)work, 1, 0, 5);
            player_set_speed(0, 0.0F, 0.0F, 0.0F);
            player_motion[0]->accel.y += player_get(0)->consts.gravity + 0.1F;
            /* @hack type: the timer read into an int and cast at each
             * use. SHC then keeps 2 in R11 and telepot_holder's address in
             * R10 (0x0c911828, 0x0c911830), and from in R4 and out in R5
             * (0x0c911860, 0x0c911864). through TASK_TIMER, each pair trades
             * registers, and 23 of 372 words differ */
            timer = work->timer;
            ratio = (Uint16)timer > 30 ? 1.0F : (Uint16)timer * 0.025F;
            /* telepot_spin times a ratio of 0 to 1 fits a Sint16. scaling
             * spin's low halfword then scales it whole */
            spin = telepot_spin;
            /* @hack type: spin's low halfword read and rewritten through a
             * Sint16 pointer to its stack slot. the image stores the word at
             * 0x0c911a6a, and loads and stores the halfword through R4 = R15
             * at 0x0c911a6c and 0x0c911a78. as union angle_halves, 175 of 372
             * words differ */
            low = (Sint16 *)&spin;
            /* @hack spelling: the halfword scaled through a Float local, which
             * keeps ratio in FR5 and the product in FR4 (0x0c911a54 to
             * 0x0c911a74). as *low = *low * ratio, 75 of 372 words differ */
            scaled = *low;
            /* @hack spelling: scaled = scaled * ratio, not scaled *= ratio. it
             * keeps offset.x in FR6 and offset.z in FR5 (0x0c9118a8,
             * 0x0c9118ac), and the registers the timer's int keeps. as
             * scaled *= ratio, 84 of 372 words differ */
            scaled = scaled * ratio;
            *low = scaled;
            player->ang.y += spin;
            if (STARTED_IN(work) == stage_get_stage_act()) {
                color.a = 0xff;
                color.r = work->timer * 2;
                color.g = work->timer * 2;
                color.b = work->timer * 2;
                telepot_set_offsets(&color);
            }
            if (TASK_TIMER(work) == 100) {
                casino_request = (int)work->scl.x;
            }
        } else {
            color.a = 0xff;
            color.r = 0;
            color.g = 0;
            color.b = 0;
            telepot_set_offsets(&color);
            work->mode = TELEPOT_WAITING;
            work->timer = 0;
            if (telepot_holder == task) {
                telepot_holder = NULL;
            }
        }
        break;
    }
    default:
        if (TASK_TIMER(work) > 150) {
            work->timer = 0;
        }
        if (work->collision->flags & COLLISION_HIT) {
            work->mode = TELEPOT_PULLING;
            work->timer = 0;
        }
        break;
    }
    collision_register(work);
    work->timer++;
}

/* draw a holding teleporter. its two models turn, fade in and shrink as the
 * hold goes on, and their UVs scroll. it draws nothing outside a hold or on
 * a frame's later steps */
static void telepot_draw(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack type: room for 72 UVs past the 28 the two rows use, which
     * makes the image's 412-byte frame (0x0c911bd6). room fills it from
     * offset 12. as room[28], the code runs 160 bytes short, count
     * unmeasured */
    NJS_TEX room[100];
    int start;
    Float fade;

    if (placement_try_leave_by(task, RANGE) || frame_step) {
        return;
    }
    casino_set_texture(CASINO_TEXTURES_ACTS_12);
    if (work->mode != TELEPOT_HOLDING) {
        return;
    }
    uv_scroll_move(telepot_scrolls, 2, room, telepot_scroll_rate * work->timer);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    /* @hack spelling: start set inside the test with a comma. SHC then loads it
     * and telepot_child_fade_steps before the timer (0x0c911c42 to
     * 0x0c911c46), and puts telepot_fade_steps' pool word after njPopMatrix's
     * (0x0c911cfc). set before the if, 10 of 297 words differ */
    if (start = telepot_fade_steps,
        TASK_TIMER(work) > start + telepot_child_fade_steps) {
        Float shrink = powf(telepot_shrink, TASK_TIMER(work) - start -
                                                telepot_child_fade_steps);

        njPushMatrix(NULL);
        TURN(njRotateY, telepot_turn * TASK_TIMER(work));
        njScale(NULL, shrink, 1.0F, shrink);
        draw_model(telepot_object.model);
        njPopMatrix(1);
    } else {
        fade = TASK_TIMER(work) / (Float)start;
        if (fade > 1.0F) {
            fade = 1.0F;
        }
        fade = 1.0F - fade;
        njPushMatrix(NULL);
        TURN(njRotateY, telepot_turn * TASK_TIMER(work));
        draw_set_material(-fade, 0.0F, 0.0F, 0.0F);
        draw_model(telepot_object.model);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        njPopMatrix(1);
    }
    /* @hack spelling: start read again after the root's draw, which reloads
     * telepot_fade_steps into R10 (0x0c911d78 to 0x0c911d7c). without it, the
     * function runs 36 bytes short and 153 of 297 words differ */
    start = telepot_fade_steps;
    if (TASK_TIMER(work) > start) {
        if (TASK_TIMER(work) > start + telepot_child_fade_steps) {
            Float shrink =
                powf(telepot_child_shrink,
                     TASK_TIMER(work) - start - telepot_child_fade_steps);

            njPushMatrix(NULL);
            TURN(njRotateY, telepot_child_turn * TASK_TIMER(work));
            njScale(NULL, telepot_child_scale * shrink, 1.0F,
                    telepot_child_scale * shrink);
            draw_model(telepot_object.child->model);
            njPopMatrix(1);
        } else {
            fade = (TASK_TIMER(work) - start) / (Float)telepot_child_fade_steps;
            if (fade > 1.0F) {
                fade = 1.0F;
            }
            fade = 1.0F - fade;
            njPushMatrix(NULL);
            TURN(njRotateY, telepot_child_turn * TASK_TIMER(work));
            njScale(NULL, telepot_child_scale, 1.0F, telepot_child_scale);
            draw_set_material(-fade, 0.0F, 0.0F, 0.0F);
            draw_model(telepot_object.child->model);
            draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
            njPopMatrix(1);
        }
    }
    uv_scroll_restore(telepot_scrolls, 2);
    njPopMatrix(1);
}

Bool telepot_holding(void)
{
    if (telepot_holder) {
        return TRUE;
    }
    return FALSE;
}

void light_a_step(struct task *task)
{
    scaled_zxy_prop_start(task, &light_a_object);
}

void light_b_step(struct task *task)
{
    scaled_zxy_prop_start(task, &light_b_object);
}

void fk_a_step(struct task *task)
{
    world_prop_start(task, &fk_a_object);
}

void fk_b_step(struct task *task)
{
    world_prop_start(task, &fk_b_object);
}

void slotisu_step(struct task *task)
{
    ranged_prop_start(250.0F * 250.0F, task, &slotisu_object, &slotisu_shape,
                      1);
}

void dento_step(struct task *task)
{
    ranged_scaled_solid_prop_start(task, 400.0F * 400.0F, &dento_object,
                                   &dento_shape, 1);
}

void flower_a_step(struct task *task)
{
    ranged_prop_start(400.0F * 400.0F, task, &flower_a_object, &flower_a_shape,
                      1);
}

void flower_b_step(struct task *task)
{
    ranged_prop_start(400.0F * 400.0F, task, &flower_b_object, &flower_b_shape,
                      1);
}

void flower_al_step(struct task *task)
{
    unlit_prop_start(task, 400.0F * 400.0F, &flower_a_object, &flower_a_shape,
                     1);
}

void flower_bl_step(struct task *task)
{
    unlit_prop_start(task, 400.0F * 400.0F, &flower_b_object, &flower_b_shape,
                     1);
}

void saku_step(struct task *task)
{
    ranged_prop_start(400.0F * 400.0F, task, &saku_object, saku_shapes, 2);
}

void taru_step(struct task *task)
{
    ranged_prop_start(400.0F * 400.0F, task, &taru_object, &taru_shape, 1);
}

void hako_step(struct task *task)
{
    ranged_prop_start(400.0F * 400.0F, task, &hako_object, &hako_shape, 1);
}

void gbox_step(struct task *task)
{
    ranged_prop_start(400.0F * 400.0F, task, &gbox_object, &gbox_shape, 1);
}

void gomi_b_step(struct task *task)
{
    pushed_prop_start(task, &gomi_b_object, &gomi_b_shape, &gomi_b_action);
}

void gomi_r_step(struct task *task)
{
    pushed_prop_start(task, &gomi_r_object, &gomi_r_shape, &gomi_r_action);
}

void rlt_step(struct task *task)
{
    spin_prop_start(task, &rlt_object, 364);
}

void yukh_step(struct task *task)
{
    shift_prop_start(task, &yukh_object, yukh_shifts, 1, 3);
}

void rlti_step(struct task *task)
{
    shift_prop_start(task, &rlti_object, rlti_shifts, 3, 3);
}

void kb_b_step(struct task *task)
{
    shift_world_prop_start(task, &kb_b_object, kb_b_shifts, 2, 3);
}

void kb_r_step(struct task *task)
{
    shift_world_prop_start(task, &kb_r_object, kb_r_shifts, 2, 3);
}

void kb_1_step(struct task *task)
{
    shift_solid_prop_start(task, &kb_1_object, &kb_shape, 1, kb_1_shifts, 1, 1);
}

void kb_2_step(struct task *task)
{
    shift_solid_prop_start(task, &kb_2_object, &kb_shape, 1, kb_2_shifts, 1, 1);
}

void kb_3_step(struct task *task)
{
    shift_solid_prop_start(task, &kb_3_object, &kb_shape, 1, kb_3_shifts, 1, 1);
}

void kb_4_step(struct task *task)
{
    shift_solid_prop_start(task, &kb_4_object, &kb_shape, 1, kb_4_shifts, 1, 1);
}

void tdenki_step(struct task *task)
{
    shift_prop_start(task, &tdenki_object, tdenki_shifts, 1, 1);
}

void sdfuti_r_step(struct task *task)
{
    shift_solid_prop_start(task, &sdfuti_r_object, &sdfuti_shape, 1,
                           sdfuti_r_shifts, 1, 3);
}

void sdfuti_b_step(struct task *task)
{
    shift_solid_prop_start(task, &sdfuti_b_object, &sdfuti_shape, 1,
                           sdfuti_b_shifts, 1, 3);
}

void map2f_step(struct task *task)
{
    ranged_scaled_solid_prop_start(task, 600.0F * 600.0F, &map2f_object,
                                   &map2f_shape, 1);
}

void cfo_step(struct task *task)
{
    swap_turn_prop_start(task, &cfo_object, cfo_swaps, 8, 1);
}

void mizu_a_step(struct task *task)
{
    scroll_sound_prop_start(task, &mizu_a_object, mizu_a_scrolls, 2, SE_SHOWER);
}

void mizu_b_step(struct task *task)
{
    scroll_sound_prop_start(task, &mizu_b_object, mizu_b_scrolls, 1, SE_SHOWER);
}

void sy_a_step(struct task *task)
{
    ranged_prop_start(400.0F * 400.0F, task, &sy_a_object, sy_shapes, 2);
}

void sy_b_step(struct task *task)
{
    ranged_prop_start(400.0F * 400.0F, task, &sy_b_object, sy_shapes, 2);
}

void shwrl_step(struct task *task)
{
    ranged_prop_start(250.0F * 250.0F, task, &shwrl_object, &shwrl_shape, 1);
}

void shwhd_step(struct task *task)
{
    ranged_prop_start(250.0F * 250.0F, task, &shwhd_object, &shwhd_shape, 1);
}

void neokab_step(struct task *task)
{
    shift_prop_start(task, &neokab_object, neokab_shifts, 2, 3);
}
