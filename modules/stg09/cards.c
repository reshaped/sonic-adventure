/* Card: the second table's thirteen cards, which a touch deals into a hand
 * of five. CardDisp: the display of the hand dealt so far */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <stdlib.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "draw/model.h"
#include "object/common.h"
#include "player/player.h"
#include "sound/sound.h"
#include "stage/stage.h"
#include "stg09/cards.h"
#include "stg09/casino.h"
#include "stg09/obj_casino_textures.h" /* NOLINT(misc-include-cleaner) */
#include "stg09/pinball.h"
#include "system/frame.h"
#include "system/task.h"
#include "system/util.h"

/* each face's model, faces 0 to 9 in order */
extern NJS_MODEL card_model_0;
extern NJS_MODEL card_model_1;
extern NJS_MODEL card_model_2;
extern NJS_MODEL card_model_3;
extern NJS_MODEL card_model_4;
extern NJS_MODEL card_model_5;
extern NJS_MODEL card_model_6;
extern NJS_MODEL card_model_7;
extern NJS_MODEL card_model_8;
extern NJS_MODEL card_model_9;

/* faces 0 to 4 are side A's, and 5 to 9 side B's */
#define CARD_FACES 10
#define CARD_SIDE 5
/* card_deal draws faces below this one, so face 9 never comes up */
#define CARD_DEALT_FACES 9
/* the face that adds to pinball_idea_draws as it is dealt into the hand */
#define CARD_FACE_IDEA 4
/* a place's face in card_table_faces until its card first draws */
#define CARD_NONE 10
/* card_deal draws again while the face has this many copies or more */
#define CARD_COPIES 5
#define CARD_TABLE 13 /* places on the table, which a Card's scl.x picks */

/* card_hand_has's bits: what card_hand_score finds in the hand */
#define HAS_SIDE_A 0x1 /* five cards of side A */
#define HAS_SIDE_B 0x2 /* five cards of side B */
/* the five faces differ. it is set only with HAS_SIDE_A or HAS_SIDE_B */
#define HAS_ALL_DIFFERENT 0x4
#define HAS_PAIR 0x8
#define HAS_SECOND_PAIR 0x10
#define HAS_THREE 0x20
#define HAS_FOUR 0x40
/* a bit for each face of a side, all set when a side's five cards differ */
#define SIDE_FACES ((1 << CARD_SIDE) - 1)

/* the hands that pay out, best first, in the order of card_hand_texanims */
#define HAND_ONE_SIDE 0 /* five different faces of one side */
#define HAND_FOUR_OF_A_KIND 1
#define HAND_FULL_HOUSE 2
#define HAND_THREE_OF_A_KIND 3
#define HAND_TWO_PAIR 4
#define HAND_ONE_PAIR 5
#define HANDS 6
#define HAND_NONE 255

/* the steps the hand's task runs. the name shows on all but the last,
 * which ends the task. while fewer than HAND_FADE steps are left, the
 * name's alpha falls by HAND_FADE_RATE each frame */
#define HAND_SHOWN 120
#define HAND_FADE 20
#define HAND_FADE_RATE 0.1F
/* the countdown's bit that hides the five cards while it is set, so that
 * they blink */
#define HAND_BLINK 8

/* the dealt cards' column on the screen, from the lowest card up */
#define COLUMN_X 592.0F
#define COLUMN_BOTTOM 432.0F
#define COLUMN_RISE 72.0F

#define SE_HAND 244
#define SE_COUNTDOWN_OVER 243

/* in a Card's work->flags. the paused draw sets it, and the step clears it.
 * the card's sparkles neither move nor draw while it is set */
#define CARD_HELD 0x100

/* a Card's work->mode, in the order it takes them */
enum card_mode { CARD_RISING, CARD_WAITING, CARD_SPINNING, CARD_SLOWING };

#define CARD_DROP 50.0F /* how far below its place a card starts to rise */
#define CARD_RISE 0.5F /* a rising card's rise each step */
/* the steps a touched card spins at full speed, which TASK_TIMER(work)
 * counts, and its turn each step */
#define CARD_SPIN_STEPS 120
#define CARD_SPIN 0x800
/* a slowing card loses CARD_SHRINK of its full width each step. it loses 1 /
 * CARD_SLOW of its full spin too */
#define CARD_SHRINK 0.02F
#define CARD_SLOW 60

/* the sparkle's sprite frames. it shows each once, then ends */
#define SPARKLE_FRAMES 6
/* how far above the card and out from it a sparkle starts */
#define SPARKLE_RISE 12.0F
#define SPARKLE_RADIUS 10.0F

/* the face each place's card shows, or CARD_NONE */
static Uint8 card_table_faces[CARD_TABLE] = {
    CARD_NONE, CARD_NONE, CARD_NONE, CARD_NONE, CARD_NONE, CARD_NONE, CARD_NONE,
    CARD_NONE, CARD_NONE, CARD_NONE, CARD_NONE, CARD_NONE, CARD_NONE,
};
static NJS_MODEL *card_models[CARD_FACES] = {
    &card_model_0, &card_model_1, &card_model_2, &card_model_3, &card_model_4,
    &card_model_5, &card_model_6, &card_model_7, &card_model_8, &card_model_9,
};
static NJS_TEXANIM card_sparkle_texanims[SPARKLE_FRAMES] = {
#include "stg09/cards/card_sparkle_texanims.inc"
};
static NJS_SPRITE card_sparkle_sprite = {
#include "stg09/cards/card_sparkle_sprite.inc"
};
static NJS_TEXANIM card_face_texanims[CARD_FACES] = {
#include "stg09/cards/card_face_texanims.inc"
};
static NJS_SPRITE card_face_sprite = {
#include "stg09/cards/card_face_sprite.inc"
};
/* the hands' names, each a quarter of one of two textures */
static NJS_TEXANIM card_hand_texanims[HANDS] = {
#include "stg09/cards/card_hand_texanims.inc"
};
static NJS_SPRITE card_hand_sprite = {
#include "stg09/cards/card_hand_sprite.inc"
};
static struct collision_shape card_shape = {
    SHAPE_RECTANGLE,
    ATTACK(3),
    PUSH_FIRM(PUSH_ALL),
    {0.0F, 0.0F, 0.0F},
    10.0F,
    15.0F,
    0.1F,
    {0},
    0,
    NULL,
};
static Sint8 card_hand_has = 0; /* the HAS_ bits card_hand_score finds */

struct card_hand_vars {
    int hand; /* a HAND_ index */
    int countdown; /* the steps the name has left */
    Float fade; /* the name's alpha */
};

struct card_vars {
    Angle ang_y; /* its placement's, which it rises at again after a spin */
    int spin; /* its turn each step */
    int slow; /* what it takes off spin each step it slows */
};

struct card_sparkle_vars {
    Float radius;
    int frame;
};

#define HAND_VARS(work) ((struct card_hand_vars *)&(work)->vars)
#define CARD_VARS(work) ((struct card_vars *)&(work)->vars)
#define SPARKLE_VARS(work) ((struct card_sparkle_vars *)&(work)->vars)

/* show the hand's name over the five cards, then end, as HAND_SHOWN
 * counts. outside act 3 the task ends at once. the hand takes effect as the
 * task ends, unless one of the second table's countdowns runs. three of a
 * kind or better starts a countdown of its own and clears
 * CASINO_BANJU_DOOR_OPEN. one pair or two pairs set
 * CASINO_BANJU_DOOR_OPEN instead, with a sound, if it is clear */
static void card_hand_run(struct task *task)
{
    struct task_work *work = task->work;
    int i;

    if (stage_get_stage_act() != STAGE_ACT(STAGE_CASINO, 3)) {
        task_kill(task);
        return;
    }
    if (!--HAND_VARS(work)->countdown) {
        if (!(casino_flags &
              (CASINO_COUNTDOWN_ACT3_0 | CASINO_COUNTDOWN_ACT3_1 |
               CASINO_COUNTDOWN_ACT3_2 | CASINO_COUNTDOWN_ACT3_3))) {
            switch (HAND_VARS(work)->hand) {
            case HAND_ONE_PAIR:
            case HAND_TWO_PAIR:
                if (!(casino_flags & CASINO_BANJU_DOOR_OPEN)) {
                    casino_flags |= CASINO_BANJU_DOOR_OPEN;
                    sound_play(SE_COUNTDOWN_OVER, 0, 0, 0);
                }
                break;
            case HAND_THREE_OF_A_KIND:
                task_create(TASK_WORK, 3, pinball_act3_countdown0_step)
                    ->work->timer = 1800;
                casino_flags |= CASINO_COUNTDOWN_ACT3_0;
                pinball_right_lane_touches = 1;
                casino_flags &= ~CASINO_BANJU_DOOR_OPEN;
                break;
            case HAND_FULL_HOUSE:
                task_create(TASK_WORK, 3, pinball_act3_countdown1_step)
                    ->work->timer = 1800;
                casino_flags |= CASINO_COUNTDOWN_ACT3_1;
                pinball_ride_touches = 0;
                casino_flags &= ~CASINO_BANJU_DOOR_OPEN;
                break;
            case HAND_FOUR_OF_A_KIND:
                task_create(TASK_WORK, 3, pinball_act3_countdown2_step)
                    ->work->timer = 2700;
                casino_flags |= CASINO_COUNTDOWN_ACT3_2;
                pinball_target_bonuses = 0;
                casino_flags &= ~CASINO_BANJU_DOOR_OPEN;
                break;
            case HAND_ONE_SIDE:
                task_create(TASK_WORK, 3, pinball_act3_countdown3_step)
                    ->work->timer = 3600;
                casino_flags |= CASINO_COUNTDOWN_ACT3_3;
                casino_flags &= ~CASINO_BANJU_DOOR_OPEN;
                break;
            }
        }
        task_kill(task);
        return;
    }
    if (!frame_step) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        draw_set_material(0.5F, 0.0F, 0.0F, 0.0F);
        card_white_sprite.sx = 80.0F;
        card_white_sprite.p.x = 0.0F;
        njDrawSprite2D(&card_white_sprite, 0, -3.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        if (!(HAND_VARS(work)->countdown & HAND_BLINK)) {
            card_face_sprite.p.x = COLUMN_X;
            card_face_sprite.p.y = COLUMN_BOTTOM;
            for (i = 0; i < PINBALL_CARDS; i++) {
                njDrawSprite2D(&card_face_sprite, pinball_cards[i], -2.0F, 0);
                card_face_sprite.p.y -= COLUMN_RISE;
            }
        }
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        if (HAND_VARS(work)->countdown < HAND_FADE) {
            HAND_VARS(work)->fade -= HAND_FADE_RATE;
        }
        draw_set_material(HAND_VARS(work)->fade, 1.0F, 1.0F, 1.0F);
        njDrawSprite2D(&card_hand_sprite, HAND_VARS(work)->hand, -2.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        njDrawSprite2D(&card_hand_sprite, HAND_VARS(work)->hand, -2.01F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    }
}

/* score the five cards in pinball_cards. for a hand that pays out, give
 * its rings and play SE_HAND. start card_hand_run's task for it too, which
 * shows the hand's name */
static void card_hand_score(void)
{
    Sint8 i;
    Sint8 j;
    Sint8 count;
    Sint8 used;
    Uint8 hand = HAND_NONE;
    int rings;
    struct task_work *work;

    card_hand_has = 0;
    count = 0;
    for (i = 0; i < PINBALL_CARDS; i++) {
        if (pinball_cards[i] < CARD_SIDE) {
            count++;
        }
    }
    if (count == PINBALL_CARDS) {
        card_hand_has |= HAS_SIDE_A;
        count = 0;
        for (i = 0; i < PINBALL_CARDS; i++) {
            count |= 1 << pinball_cards[i];
        }
        if (count == SIDE_FACES) {
            card_hand_has |= HAS_ALL_DIFFERENT;
        }
    } else {
        count = 0;
        for (i = 0; i < PINBALL_CARDS; i++) {
            if (pinball_cards[i] >= CARD_SIDE) {
                count++;
            }
        }
        if (count == PINBALL_CARDS) {
            card_hand_has |= HAS_SIDE_B;
            count = 0;
            for (i = 0; i < PINBALL_CARDS; i++) {
                count |= 1 << (pinball_cards[i] - CARD_SIDE);
            }
            if (count == SIDE_FACES) {
                card_hand_has |= HAS_ALL_DIFFERENT;
            }
        }
    }
    used = 0;
    for (i = 0; i < PINBALL_CARDS; i++) {
        count = 0;
        for (j = 0; j < PINBALL_CARDS; j++) {
            if (j != i && !(used & (1 << j)) &&
                pinball_cards[i] == pinball_cards[j]) {
                count++;
                used |= 1 << j;
                used |= 1 << i;
            }
        }
        switch (count) {
        case 1:
            if (!(card_hand_has & HAS_PAIR)) {
                card_hand_has |= HAS_PAIR;
            } else {
                card_hand_has |= HAS_SECOND_PAIR;
            }
            break;
        case 2:
            card_hand_has |= HAS_THREE;
            break;
        case 3:
            card_hand_has |= HAS_FOUR;
            break;
        }
    }
    if (card_hand_has & HAS_ALL_DIFFERENT) {
        hand = HAND_ONE_SIDE;
    } else if (card_hand_has & HAS_FOUR) {
        hand = HAND_FOUR_OF_A_KIND;
    } else if (card_hand_has & HAS_THREE) {
        if (card_hand_has & HAS_PAIR) {
            hand = HAND_FULL_HOUSE;
        } else {
            hand = HAND_THREE_OF_A_KIND;
        }
    } else if (card_hand_has & HAS_SECOND_PAIR) {
        hand = HAND_TWO_PAIR;
    } else if (card_hand_has & HAS_PAIR) {
        hand = HAND_ONE_PAIR;
    }
    if (hand != HAND_NONE) {
        work = task_create(TASK_WORK, 3, card_hand_run)->work;
        sound_play(SE_HAND, 0, 0, 0);
        rings = 1;
        switch (hand) {
        case HAND_ONE_SIDE:
            rings = 100;
            break;
        case HAND_FOUR_OF_A_KIND:
            rings = 70;
            break;
        case HAND_FULL_HOUSE:
            rings = 50;
            break;
        case HAND_THREE_OF_A_KIND:
            rings = 30;
            break;
        case HAND_TWO_PAIR:
            rings = 20;
            break;
        case HAND_ONE_PAIR:
            rings = 10;
            break;
        }
        /* @hack type: hand cast to Sint8 and masked back, which
         * zero-extends it again for the store (EXTU.B R8,R8 at 0x0c907e52),
         * apart from the copy the switch tests. as hand, 26 of 438 words
         * differ */
        HAND_VARS(work)->hand = (Sint8)hand & 0xff;
        HAND_VARS(work)->countdown = HAND_SHOWN;
        HAND_VARS(work)->fade = 1.0F;
        pinball_give_rings(rings);
    }
}

/* give the card at place work->submode a face drawn at random, in
 * work->tick and card_table_faces. it draws again while the face has
 * CARD_COPIES copies or more. the copies count over all thirteen places and
 * all five of pinball_cards. the place's old face counts. so do the entries
 * of pinball_cards at or past pinball_cards_dealt, which still hold the last
 * hand's faces */
static void card_deal(struct task_work *work)
{
    Sint8 i;
    Sint8 count;
    /* @hack type: face signed, which SHC sign-extends for the compares (EXTS.B
     * at 0x0c907ec8). as a Uint8, 1 of 146 words differ */
    Sint8 face;

    do {
        face = rand() * (CARD_DEALT_FACES * RAND_SCALE);
        count = 0;
        for (i = 0; i < CARD_TABLE; i++) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            if (face == card_table_faces[i]) {
                count++;
            }
        }
        for (i = 0; i < PINBALL_CARDS; i++) {
            /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
            if (face == pinball_cards[i]) {
                count++;
            }
        }
    } while (count >= CARD_COPIES);
    card_table_faces[work->submode] = work->tick = face;
}

static void card_display_destroy(struct task *task)
{
}

static void card_display_draw(struct task *task)
{
    int i;

    if (frame_step) {
        return;
    }
    if (pinball_cards_dealt) {
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        draw_set_material(0.5F, 0.0F, 0.0F, 0.0F);
        card_white_sprite.sx = 12.0F;
        card_white_sprite.p.x = 544.0F;
        njDrawSprite2D(&card_white_sprite, 0, -3.0F,
                       NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
        draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
        card_face_sprite.p.x = COLUMN_X;
        card_face_sprite.p.y = COLUMN_BOTTOM;
        for (i = 0; i < pinball_cards_dealt; i++) {
            njDrawSprite2D(&card_face_sprite, pinball_cards[i], -2.0F, 0);
            card_face_sprite.p.y -= COLUMN_RISE;
        }
    }
}

static void card_display_run(struct task *task)
{
    card_display_draw(task);
}

void card_display_step(struct task *task)
{
    card_white_sprite.sy = 60.0F;
    card_white_sprite.p.y = 0.0F;
    task->step = card_display_run;
    task->draw = card_display_draw;
    task->destructor = card_display_destroy;
}

static void card_destroy(struct task *task)
{
    task_destroy_children(task);
}

/* a sparkle drifts out from the card and down, facing the camera. it runs
 * on a frame's first step alone, and not while its card has CARD_HELD */
static void card_sparkle_run(struct task *task)
{
    struct task_work *work = task->work;
    Float x;
    Float z;
    Angle ang;

    if (frame_step) {
        return;
    }
    if (!(task->parent->work->flags & CARD_HELD)) {
        x = njSin(work->ang.y) * SPARKLE_VARS(work)->radius;
        z = njCos(work->ang.y) * SPARKLE_VARS(work)->radius;
        njPushMatrix(NULL);
        njTranslate(NULL, work->pos.x + x, work->pos.y, work->pos.z + z);
        TURN_VIA(njRotateY, camera_work->ang.y, ang);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_ONE);
        njDrawSprite3D(&card_sparkle_sprite, SPARKLE_VARS(work)->frame,
                       NJD_SPRITE_ALPHA);
        njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
        njColorBlendingMode(NJD_DESTINATION_COLOR,
                            NJD_COLOR_BLENDING_INVSRCALPHA);
        njPopMatrix(1);
        SPARKLE_VARS(work)->radius += 1.0F;
        work->pos.y -= 2.0F;
        if (++SPARKLE_VARS(work)->frame == SPARKLE_FRAMES) {
            task_kill(task);
        }
    }
}

/* scl.z is the card's width as it shrinks away */
static void card_draw(struct task *task)
{
    struct task_work *work = task->work;
    Angle ang;

    casino_set_texture(CASINO_TEXTURES_ACTS_34);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN_VIA(njRotateZ, work->ang.z, ang);
    TURN_VIA(njRotateX, work->ang.x, ang);
    TURN_VIA(njRotateY, work->ang.y, ang);
    njScale(NULL, work->scl.z, 1.0F, work->scl.z);
    draw_model(card_models[work->tick]);
    njPopMatrix(1);
}

static void card_draw_held(struct task *task)
{
    task->work->flags |= CARD_HELD;
    task_step_children(task);
    card_draw(task);
}

/* a card rises to its place and waits for a touch. the touch spins it one
 * way or the other, by where player 0 stands. after CARD_SPIN_STEPS steps, it
 * slows as it shrinks. as it vanishes, it deals its face into the hand and
 * rises again with a new one */
static void card_run(struct task *task)
{
    struct task_work *work = task->work;
    struct task_work *player;
    struct task_work *sparkle;
    Angle ang;
    Float x;
    int back;
    int ahead;

    switch (work->mode) {
    case CARD_RISING:
        work->pos.y += CARD_RISE;
        if (work->pos.y > work->scl.y) {
            work->pos.y = work->scl.y;
            work->mode++;
        }
        break;
    case CARD_WAITING:
        if (work->collision->flags & COLLISION_HIT) {
            /* @hack spelling: player taken inside x's expression, which keeps
             * it in the frame (MOV.L R3,@R15 at 0x0c9083f4). set in a
             * statement of its own first, 96 of 233 words differ */
            x = work->pos.x - (player = player_work[0])->pos.x;
            ang = (work->ang.y +
                   (Angle)(atan2f(work->pos.z - player->pos.z, x) *
                           ANGLE_PER_RADIAN) -
                   NJM_DEG_ANG(90)) &
                  0xffff;
            /* @hack spelling: both turns through locals, set ahead of the
             * tests, which places CARD_SPIN's pool word ahead of its
             * negation's (0x0c908462). as constants at the stores, 3 of 233
             * words differ */
            ahead = CARD_SPIN;
            back = -CARD_SPIN;
            if (ang < NJM_DEG_ANG(180)) {
                if (ang > NJM_DEG_ANG(90)) {
                    CARD_VARS(work)->spin = ahead;
                } else {
                    CARD_VARS(work)->spin = back;
                }
            } else {
                if (ang > NJM_DEG_ANG(270)) {
                    CARD_VARS(work)->spin = ahead;
                } else {
                    CARD_VARS(work)->spin = back;
                }
            }
            CARD_VARS(work)->slow = CARD_VARS(work)->spin / CARD_SLOW;
            work->mode++;
        }
        break;
    case CARD_SPINNING:
        if (++TASK_TIMER(work) == CARD_SPIN_STEPS) {
            work->mode++;
        }
        work->ang.y += CARD_VARS(work)->spin;
        sparkle = task_create_child(TASK_WORK, card_sparkle_run, task)->work;
        sparkle->pos.x = work->pos.x;
        sparkle->pos.y = work->pos.y + SPARKLE_RISE;
        sparkle->pos.z = work->pos.z;
        SPARKLE_VARS(sparkle)->radius = SPARKLE_RADIUS;
        if (!(TASK_TIMER(work) & 1)) {
            sparkle->ang.y = work->ang.y - NJM_DEG_ANG(90);
        } else {
            sparkle->ang.y = work->ang.y + NJM_DEG_ANG(90);
        }
        break;
    case CARD_SLOWING:
        if (work->scl.z < 0.0F) {
            if (work->tick == CARD_FACE_IDEA) {
                pinball_idea_draws++;
            }
            pinball_cards[pinball_cards_dealt] = work->tick;
            pinball_cards_dealt++;
            if (pinball_cards_dealt == PINBALL_CARDS) {
                pinball_cards_dealt = 0;
                card_hand_score();
            }
            card_deal(work);
            work->scl.z = 1.0F;
            work->ang.y = CARD_VARS(work)->ang_y;
            work->pos.y = work->scl.y - CARD_DROP;
            work->timer = 0;
            work->mode = CARD_RISING;
        } else {
            work->scl.z -= CARD_SHRINK;
            CARD_VARS(work)->spin -= CARD_VARS(work)->slow;
            work->ang.y += CARD_VARS(work)->spin;
        }
        break;
    }
    work->flags &= ~CARD_HELD;
    task_step_children(task);
    card_draw(task);
    collision_register(work);
    object_clear_touch(work, NULL);
}

/* the card keeps its place in work->submode and the place's height in
 * scl.y, and starts CARD_DROP below it */
void card_step(struct task *task)
{
    struct task_work *work = task->work;

    collision_init(task, &card_shape, COLLISION_OBJECT, 1, 1);
    work->mode = CARD_RISING;
    work->submode = work->scl.x;
    work->scl.y = work->pos.y;
    CARD_VARS(work)->ang_y = work->ang.y;
    work->pos.y = work->scl.y - CARD_DROP;
    work->scl.z = 1.0F;
    card_deal(work);
    task->step = card_run;
    task->draw = card_draw_held;
    task->destructor = card_destroy;
}
