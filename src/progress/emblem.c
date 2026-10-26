/* emblems: their bits in the save, the fields' pickups and the count
 * display */
#include <mathf.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "camera/camera.h"
#include "collision/collision.h"
#include "debug/editor.h"
#include "draw/draw.h"
#include "draw/tree.h"
#include "hud/hud.h"
#include "main.h"
#include "object/placement.h"
#include "player/player.h"
#include "progress/emblem.h"
#include "save/card.h"
#include "save/save.h"
#include "sound/sound.h"
#include "sound/stream.h"
#include "stage/stage.h"
#include "system/frame.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"
#include "vmu/vibration.h"

#define TEXT_1_LENGTH 13
#define TEXT_2_LENGTH 13
#define TEXT_3_LENGTH 13
#define TEXT_4_LENGTH 13
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 13
#define TEXT_7_LENGTH 13
#include "progress/emblem/text.inc"

/* the mission emblems' numbers. the action stages' take a block of
 * EMBLEM_ACTION_BLOCK numbers for each grade, A's from 0, then B's and
 * C's. the mini games' take blocks of EMBLEM_MINIGAME_BLOCK the same way
 * from EMBLEM_MINIGAMES. each block holds the cart's, the two Sky Chases',
 * Sand Hill's and Hedgehog Hammer's, whose A is EMBLEM_PRACTICE. the
 * race's A to C take EMBLEM_RACE to EMBLEM_RACE + 2. those are also the
 * numbers of grade C's cart and two Sky Chases */
#define EMBLEM_ACTION_BLOCK 32
#define EMBLEM_MINIGAMES 96
#define EMBLEM_MINIGAME_BLOCK 5
#define EMBLEM_PRACTICE 100
#define EMBLEM_RACE 106

/* the first of each field's four pickups' emblems. EMBLEM_STORY is the
 * first of the stories', one for each character with a story */
#define EMBLEM_STATION_SQUARE 118
#define EMBLEM_EGG_CARRIER 122
#define EMBLEM_MYSTIC_RUINS 126
#define EMBLEM_STORY 111

enum emblem_mode {
    EMBLEM_INIT,
    EMBLEM_WAIT, /* for a player to take it */
    EMBLEM_TAKEN
};

/* a shine's modes: it shows the count, then blinks it as it fades */
enum shine_mode { SHINE_INIT, SHINE_COUNT, SHINE_FADE, SHINE_DONE };

/* a shine shows the count until its timer passes SHINE_COUNT_TIME. as it
 * fades, the count turns dim at step SHINE_DIM_STEP of each blink of
 * SHINE_BLINK_STEPS steps, and bright at its end */
#define SHINE_COUNT_TIME 45
#define SHINE_DIM_STEP 6
#define SHINE_BLINK_STEPS 12

/* a count's colors, in argb: white, and white at about a quarter alpha */
#define COUNT_BRIGHT 0xffffffff
#define COUNT_DIM 0x3fffffff

/* the count display's modes: the count before the new emblem, then the new
 * count blinking, then a fade */
enum show_mode { SHOW_INIT, SHOW_BEFORE, SHOW_AFTER, SHOW_FADE, SHOW_HOLD };

/* the steps the count before the new emblem shows, and the new count
 * blinks, dim for the first half of each blink of SHOW_BLINK_STEPS */
#define SHOW_BEFORE_STEPS 30
#define SHOW_AFTER_STEPS 90
#define SHOW_BLINK_STEPS 30

struct emblem_vars {
    NJS_OBJECT *object;
    Uint8 unk_04[4];
    Sint32 number;
};

#define VARS(work) ((struct emblem_vars *)&(work)->vars)

/* the work of a shine and of the count display, in place of task_work. a
 * shine is the emblem a player takes, growing and fading where it was */
struct shine_work {
    Sint8 mode;
    Uint8 unk_01[5];
    Uint16 timer;
    Float alpha; /* the shine's, from 0 down to -1 */
    Float unk_0c;
    Sint32 count; /* how many emblems the save has */
    struct angles ang;
    NJS_POINT3 pos;
    Float scale;
    Float scale_y;
    Float fade; /* the count display's emblem's alpha, from 1 down to 0 */
};

/* @hack spelling: task->work read again for each store, as the image does
 * (MOV.L @(32,R4),R3 at 0x8c0ad016 and MOV.L @(32,R4),R2 at 0x8c0ad686);
 * through a local, 46 of the 77 words of shine_create and
 * emblem_start_count_display differ */
#define WORK(task) ((struct shine_work *)(task)->work)

static void emblem_set(struct save *save, int number);
static int emblem_story_number(int character);
static void emblem_draw(struct task *task);
static void emblem_destroy(struct task *task);
static struct task *shine_create(NJS_POINT3 *pos);
static void shine_init(struct task *task);
static void shine_step(struct task *task);
static void shine_draw(struct task *task);
static void show_count_step(struct task *task);
static void show_count_destroy(struct task *task);

static NJS_TEXNAME emblem_texnames[5] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0},
};
NJS_TEXLIST emblem_texlist = {
#include "progress/emblem/emblem_texlist.inc"
};

static NJS_MATERIAL emblem_materials[5] = {
#include "progress/emblem/emblem_materials.inc"
};

static Sint16 emblem_meshes0[5] = {
#include "progress/emblem/emblem_meshes0.inc"
};

static Sint16 emblem_meshes1[11] = {
#include "progress/emblem/emblem_meshes1.inc"
};

static Sint16 emblem_meshes2[34] = {
#include "progress/emblem/emblem_meshes2.inc"
};

static Sint16 emblem_meshes3[10] = {
#include "progress/emblem/emblem_meshes3.inc"
};

static Sint16 emblem_meshes4[38] = {
#include "progress/emblem/emblem_meshes4.inc"
};

static NJS_COLOR emblem_uvs0[4] = {
#include "progress/emblem/emblem_uvs0.inc"
};

static NJS_COLOR emblem_uvs1[10] = {
#include "progress/emblem/emblem_uvs1.inc"
};

static NJS_COLOR emblem_uvs2[28] = {
#include "progress/emblem/emblem_uvs2.inc"
};

static NJS_COLOR emblem_uvs3[8] = {
#include "progress/emblem/emblem_uvs3.inc"
};

static NJS_COLOR emblem_uvs4[36] = {
#include "progress/emblem/emblem_uvs4.inc"
};

static NJS_MESHSET emblem_meshsets[5] = {
#include "progress/emblem/emblem_meshsets.inc"
};

static NJS_POINT3 emblem_points[62] = {
#include "progress/emblem/emblem_points.inc"
};

static NJS_VECTOR emblem_normals[62] = {
#include "progress/emblem/emblem_normals.inc"
};

static NJS_MODEL emblem_model = {
#include "progress/emblem/emblem_model.inc"
};

static NJS_OBJECT emblem_object = {
#include "progress/emblem/emblem_object.inc"
};

/* each character's action stages with mission emblems, bit n for stage n.
 * emblem_mission_number numbers the emblems in this order */
static Uint16 emblem_stages[CHARACTER_COUNT] = {
    0x07fe, 0x0000, 0x0354, 0x02f0, 0x0000, 0x1408, 0x1426, 0x110a,
};

/* the shape a player takes the emblem by */
static struct collision_shape emblem_shape = {
    SHAPE_SPHERE,
    0,
    PUSH_FIRM(PUSH_PLAYERS | PUSH_ENEMIES | PUSH_OTHERS),
    {0.0F, 0.0F, 0.0F},
    5.0F,
    0.0F,
    0.0F,
    {0},
    0,
    NULL};

/* the count over a shine. only a shine's fade sets its color, and a fade
 * that runs to its end leaves it bright. a shine destroyed while the count
 * is dim leaves the next shine's count dim, until step SHINE_BLINK_STEPS of
 * that shine's fade */
static struct hud_number shine_number = {
    NUMBER_SCALED, NUMBER_BOTTOM, 0, 999, 0, {SCREEN_MIDDLE_X, 370.0F, 0.0F},
    2.5F,          COUNT_BRIGHT,
};

/* the count's emblem, in its two halves */
static NJS_TEXNAME show_count_texnames[2] = {
    {TEXT_6, 0, 0},
    {TEXT_7, 0, 0},
};
static NJS_TEXLIST show_count_texlist = {
#include "progress/emblem/show_count_texlist.inc"
};
static NJS_TEXANIM show_count_texanims[2] = {
#include "progress/emblem/show_count_texanims.inc"
};
static struct hud_number show_count_number = {
    NUMBER_SCALED | NUMBER_CLAMPED,
    0,
    0,
    999,
    0,
    {272.0F, 336.0F, 0.0F},
    2.0F,
    COUNT_BRIGHT,
};

void emblem_clear_all(struct save *save)
{
    /* @hack spelling: the bytes through a byte cursor, which counts the loop
     * down with DT R4 (0x8c0ac9c8); subscripted, SHC compares i with 17 and
     * 9 of 10 words differ */
    Sint8 *byte = save->emblems;
    int i;

    for (i = 0; i < EMBLEM_BYTES; i++) {
        *byte = 0;
        byte++;
    }
}

Bool emblem_check(struct save *save, int number)
{
    int index = number / 8;
    int bit = 1 << (number % 8);

    if (save->emblems[index] & bit) {
        return TRUE;
    }
    return FALSE;
}

static void emblem_set(struct save *save, int number)
{
    int index = number / 8;
    int bit = 1 << (number % 8);

    save->emblems[index] |= bit;
    if (game_mode == GAME_MODE_FIELD) {
        save_update_write();
    }
}

int emblem_count(struct save *save)
{
    int i;
    int count = 0;
    /* @hack spelling: the bytes through a byte cursor, which loads each through
     * R7 and an add (0x8c0aca70, 0x8c0aca8e); subscripted, SHC loads them
     * with MOV.B @R0+ and 23 of 23 words differ */
    Sint8 *byte = save->emblems;
    Uint8 bits;

    for (i = 0; i < EMBLEM_BYTES; i++) {
        /* @hack spelling: bits = bits >> 1, which widens the shifted byte
         * into R3 (EXTU.B R4,R3 at 0x8c0aca84); as >>=, into R2, and 2 of 23
         * words differ */
        for (bits = *byte; bits != 0; bits = bits >> 1) {
            if (bits & 1) {
                count++;
            }
        }
        byte++;
    }
    return count;
}

/* character's story emblem: from EMBLEM_STORY, in character order, with
 * Eggman and Tikal skipped */
static int emblem_story_number(int character)
{
    if (character >= CHARACTER_AMY) {
        character--;
    }
    if (character >= CHARACTER_TAILS) {
        character--;
    }
    return character + EMBLEM_STORY;
}

/* an action stage's number in a grade is its place, from 0, among the
 * stages emblem_stages marks. they run through every earlier character's
 * stages, then this character's up to stage */
int emblem_mission_number(int character, int stage, int mission)
{
    int number = 0;
    int i;
    Uint16 mask;
    Uint16 bits;
    int count;

    if (stage == STAGE_PRACTICE) {
        /* @hack spelling: the number set, then added to, which loads the 100
         * in the BRA's slot (MOV #100,R7 at 0x8c0acac2); as one sum, SHC adds
         * 100 to the product and 2 of 82 words differ */
        number = EMBLEM_PRACTICE;
        number += mission * EMBLEM_MINIGAME_BLOCK;
    } else if (stage <= STAGE_ACTION_LAST) {
        mask = 0xffff >> (15 - stage);
        /* @hack spelling: the bound through a local, which SHC keeps in R11
         * and mask in R10 (0x8c0acb08, 0x8c0acb14); as i < character + 1,
         * the two swap places and 7 of 82 words differ */
        count = character + 1;
        for (i = 0; i < count; i++) {
            if (character == i) {
                /* @hack spelling: bits set from mask, then masked with the
                 * table's halfword, which loads it into R3 (MOV.W @(R0,R1),R3
                 * at 0x8c0acb26); as one statement, into R5, and 53 of 82
                 * words differ */
                bits = mask;
                bits &= emblem_stages[i];
            } else {
                bits = emblem_stages[i];
            }
            /* @hack spelling: bits = bits >> 1, which copies bits through R2
             * before the shift (0x8c0acb40, 0x8c0acb44); as >>=, SHC extends
             * bits in place, without the copy, and 39 of 82 words differ */
            for (; bits; bits = bits >> 1) {
                if (bits & 1) {
                    number++;
                }
            }
        }
        number--;
        number += mission * EMBLEM_ACTION_BLOCK;
    } else if (stage > STAGE_PAST && stage <= STAGE_SUNABOARD) {
        number = stage + (EMBLEM_MINIGAMES - STAGE_CART);
        number += mission * EMBLEM_MINIGAME_BLOCK;
    } else if (stage == STAGE_ALIFE_RACE) {
        number = EMBLEM_RACE;
        number += mission;
    }
    return number;
}

void emblem_set_story(struct save *save, int character)
{
    emblem_set(save, emblem_story_number(character));
}

Bool emblem_check_story(struct save *save, int character)
{
    return emblem_check(save, emblem_story_number(character));
}

Bool emblem_check_mission(struct save *save, int character, int stage,
                          int mission)
{
    return emblem_check(save, emblem_mission_number(character, stage, mission));
}

void emblem_set_mission(struct save *save, int character, int stage,
                        int mission)
{
    emblem_set(save, emblem_mission_number(character, stage, mission));
}

/* draws on a frame's first step only */
static void emblem_draw(struct task *task)
{
    struct task_work *work = task->work;

    if (frame_step == 0) {
        njSetTexture(&emblem_texlist);
        njPushMatrix(NULL);
        njTranslateV(NULL, &work->pos);
        TURN(njRotateY, work->ang.y);
        draw_tree(VARS(work)->object);
        njPopMatrix(1);
    }
}

static void emblem_destroy(struct task *task)
{
    task_kill(task);
}

void emblem_step(struct task *task)
{
    struct task_work *work = task->work;
    /* @hack spelling: the player through a local, which SHC stores at @R15
     * and loads back for the call (0x8c0acf64, 0x8c0acf68); passed directly,
     * 271 of 273 words differ */
    Sint8 player;

    if (placement_try_leave(task)) {
        return;
    }
    if (edit_mode) {
        VARS(work)->object = &emblem_object;
        work->ang.y += work->ang.x;
        emblem_draw(task);
        if (!placement_editing(task)) {
            return;
        }
        njPrintC(NJM_LOCATION(17, 7), "> POSITION");
        njPrintC(NJM_LOCATION(17, 8), "> POSITION");
        njPrintC(NJM_LOCATION(17, 9), "> POSITION");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_X), "> ROT RATE");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Y), ">");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_ANG_Z), ">");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_X), "> ID (0-3):");
        if (work->scl.x >= 3.0F) {
            njPrintC(NJM_LOCATION(29, EDITOR_ROW_SCL_X), "3");
        } else if (work->scl.x >= 2.0F) {
            njPrintC(NJM_LOCATION(29, EDITOR_ROW_SCL_X), "2");
        } else if (work->scl.x >= 1.0F) {
            njPrintC(NJM_LOCATION(29, EDITOR_ROW_SCL_X), "1");
        } else {
            njPrintC(NJM_LOCATION(29, EDITOR_ROW_SCL_X), "0");
        }
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Y), ">");
        njPrintC(EDITOR_LABEL(EDITOR_ROW_SCL_Z), ">");
    } else {
        switch (work->mode) {
        case EMBLEM_INIT:
            /* a field's later stages take its first's emblems. the Past
             * takes the Mystic Ruins', and any other stage takes Station
             * Square's */
            switch (stage_get_stage_act() >> 8) {
            default:
            case STAGE_STATION_SQUARE:
            case STAGE_STATION_SQUARE_2:
            case STAGE_STATION_SQUARE_3:
                VARS(work)->number = EMBLEM_STATION_SQUARE;
                break;
            case STAGE_EGG_CARRIER:
            case STAGE_EGG_CARRIER_2:
            case STAGE_EGG_CARRIER_3:
                VARS(work)->number = EMBLEM_EGG_CARRIER;
                break;
            case STAGE_MYSTIC_RUINS:
            case STAGE_PAST:
                VARS(work)->number = EMBLEM_MYSTIC_RUINS;
                break;
            }
            if (work->scl.x >= 3.0F) {
                VARS(work)->number += 3;
            } else if (work->scl.x >= 2.0F) {
                VARS(work)->number += 2;
            } else if (work->scl.x >= 1.0F) {
                VARS(work)->number += 1;
            }
            if (emblem_check(&saves[save_slot], VARS(work)->number)) {
                work->mode = EMBLEM_TAKEN;
            } else {
                work->mode = EMBLEM_WAIT;
            }
            VARS(work)->object = &emblem_object;
            task->draw = emblem_draw;
            task->destructor = emblem_destroy;
            collision_init(task, &emblem_shape, COLLISION_PICKUP, 1, 1);
            work->collision->radius = 5.0F;
            break;
        case EMBLEM_WAIT:
            if ((work->collision->flags & COLLISION_HIT) &&
                work->collision->hit->kind == COLLISION_PLAYER) {
                player = PLAYER_VARS(work->collision->hit->owner->work)->player;
                vibration_start(player, 1, 48, 0);
                emblem_set(&saves[save_slot], VARS(work)->number);
                work->mode = EMBLEM_TAKEN;
                shine_create(&work->pos);
            }
            work->ang.y += work->ang.x;
            emblem_draw(task);
            collision_register(work);
            break;
        case EMBLEM_TAKEN:
            placement_kill(task);
            break;
        }
    }
}

static struct task *shine_create(NJS_POINT3 *pos)
{
    struct task *task;

    /* @hack spelling: task assigned in the test, which tests task_create's R0
     * with the copy to R4 in the BT/S's slot (0x8c0ad010, 0x8c0ad012);
     * assigned apart, SHC tests R4 and 3 of 36 words differ */
    if ((task = task_create(TASK_WORK, 3, shine_init)) != NULL) {
        WORK(task)->mode = SHINE_INIT;
        WORK(task)->pos.x = pos->x;
        WORK(task)->pos.y = pos->y;
        WORK(task)->pos.z = pos->z;
        return task;
    }
    return NULL;
}

static void shine_init(struct task *task)
{
    struct shine_work *work = (struct shine_work *)task->work;

    work->ang.x = 0;
    work->ang.z = 0;
    work->mode = SHINE_INIT;
    work->alpha = 0.0F;
    work->timer = 0;
    work->count = 0;
    work->unk_0c = 0.0F;
    work->scale = 1.0F;
    task->step = shine_step;
    task->draw = shine_draw;
}

/* the shine faces the camera. it shows the count, then fades for 51 steps,
 * growing on all but the last, while the count blinks */
static void shine_step(struct task *task)
{
    struct shine_work *work = (struct shine_work *)task->work;
    Float dx;
    Float dz;

    if (placement_try_leave(task)) {
        return;
    }
    switch (work->mode) {
    case SHINE_INIT:
        work->mode = SHINE_COUNT;
        work->timer = 0;
        work->count = emblem_count(&saves[save_slot]);
        sound_play(SE_NOTICE, 0, 0, 0);
        break;
    case SHINE_COUNT:
        work->timer++;
        if (work->timer > SHINE_COUNT_TIME) {
            work->timer = 0;
            work->mode = SHINE_FADE;
        }
        dx = camera_work->pos.x - work->pos.x;
        dz = camera_work->pos.z - work->pos.z;
        work->ang.y = (Angle)(atan2f(dx, dz) * ANGLE_PER_RADIAN);
        shine_number.value = work->count;
        hud_draw_number(&shine_number);
        break;
    case SHINE_FADE:
        work->timer++;
        if (work->timer == SHINE_BLINK_STEPS) {
            work->timer = 0;
            shine_number.color = COUNT_BRIGHT;
        } else if (work->timer == SHINE_DIM_STEP) {
            shine_number.color = COUNT_DIM;
        }
        hud_draw_number(&shine_number);
        work->alpha -= 0.02F;
        if (work->alpha <= -1.0F) {
            work->alpha = -1.0F;
            work->mode = SHINE_DONE;
            task_kill(task);
        } else {
            dx = camera_work->pos.x - work->pos.x;
            dz = camera_work->pos.z - work->pos.z;
            work->ang.y = (Angle)(atan2f(dx, dz) * ANGLE_PER_RADIAN);
            work->scale += 0.08F;
        }
        break;
    case SHINE_DONE:
        break;
    }
    shine_draw(task);
}

static void shine_draw(struct task *task)
{
    struct shine_work *work = (struct shine_work *)task->work;
    Float scale = work->scale;

    njSetTexture(&emblem_texlist);
    draw_attr_save();
    draw_attr_set(0, NJD_FLAG_USE_ALPHA);
    draw_set_material(work->alpha, 0.0F, 0.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &work->pos);
    TURN(njRotateY, work->ang.y);
    njScale(NULL, scale, scale, scale);
    draw_tree(&emblem_object);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    draw_attr_restore();
}

static void show_count_step(struct task *task)
{
    struct shine_work *work = (struct shine_work *)task->work;
    NJS_SPRITE sprite;
    NJS_ARGB color;
    /* @hack redundant: nothing uses this sprite, but it takes the 32 bytes
     * the image's frame holds between quad and color (ADD R0,R15 at
     * 0x8c0ad32c reserves 148); without it, 288 of 290 words differ */
    NJS_SPRITE unused;
    /* @hack type: quad as an array of one, so that its address stays in R13
     * (0x8c0ad532, 0x8c0ad538); as a struct, 41 of 290 words differ */
    NJS_POINT2COL quad[1];
    NJS_POINT2 points[4];
    NJS_COLOR colors[4];

    if (hud_hidden < 0) {
        return;
    }
    switch (work->mode) {
    case SHOW_INIT:
        work->scale = 1.0F;
        work->scale_y = 1.0F;
        work->fade = 1.0F;
        work->mode = SHOW_BEFORE;
        work->timer = SHOW_BEFORE_STEPS;
        pad_disable();
        sound_play(SE_NOTICE, 0, 0, 0);
        work->count = emblem_count(&saves[save_slot]);
        show_count_number.value = work->count - 1;
        if (show_count_number.value > 9) {
            show_count_number.pos.x = 280.0F;
        }
        if (show_count_number.value > 99) {
            show_count_number.pos.x = 288.0F;
        }
        break;
    case SHOW_BEFORE:
        if (--work->timer <= 0) {
            work->mode = SHOW_AFTER;
            work->timer = SHOW_AFTER_STEPS;
            show_count_number.value = work->count;
            if (show_count_number.value > 9) {
                show_count_number.pos.x = 280.0F;
            }
            if (show_count_number.value > 99) {
                show_count_number.pos.x = 288.0F;
            }
        }
        break;
    case SHOW_AFTER:
        if (--work->timer <= 0) {
            work->mode = SHOW_FADE;
        }
        if (work->timer % SHOW_BLINK_STEPS < SHOW_BLINK_STEPS / 2) {
            show_count_number.color = COUNT_BRIGHT;
        } else {
            show_count_number.color = COUNT_DIM;
        }
        break;
    case SHOW_FADE:
        work->fade -= 0.025F;
        if (work->fade <= 0.0F) {
            pad_enable();
            task_kill(task);
            return;
        }
        break;
    case SHOW_HOLD:
        break;
    }
    sprite.tlist = &show_count_texlist;
    sprite.tanim = show_count_texanims;
    sprite.sx = work->scale;
    sprite.sy = work->scale_y;
    sprite.p.x = work->pos.x;
    sprite.p.y = work->pos.y;
    njColorBlendingMode(NJD_SOURCE_COLOR, NJD_COLOR_BLENDING_SRCALPHA);
    njColorBlendingMode(NJD_DESTINATION_COLOR, NJD_COLOR_BLENDING_INVSRCALPHA);
    njSetTexture(&show_count_texlist);
    /* @hack spelling: &points[0], which loads the points' address into R5 and
     * the colors' into R4 (0x8c0ad524, 0x8c0ad530); as points, the two swap
     * places and 21 of 290 words differ */
    quad->p = &points[0];
    quad->col = colors;
    quad->tex = NULL;
    quad->num = 4;
    points[0].x = 0.0F;
    points[0].y = 0.0F;
    points[1].x = SCREEN_WIDTH;
    points[1].y = 0.0F;
    points[2].x = SCREEN_WIDTH;
    points[2].y = SCREEN_HEIGHT;
    points[3].x = 0.0F;
    points[3].y = SCREEN_HEIGHT;
    colors[0].color = colors[1].color = colors[2].color = colors[3].color =
        0xbb080808;
    njDrawPolygon2D(quad, 4, -1.15F, NJD_FILL | NJD_TRANSPARENT);
    color.a = work->fade;
    color.b = 1.0F;
    color.g = 1.0F;
    color.r = 1.0F;
    njSetConstantMaterial(&color);
    njDrawSprite2D(&sprite, 0, 0.0F, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    njDrawSprite2D(&sprite, 1, 0.0F, NJD_SPRITE_ALPHA | NJD_SPRITE_COLOR);
    color.b = 0.0F;
    color.g = 0.0F;
    color.r = 0.0F;
    color.a = 0.0F;
    njSetConstantMaterial(&color);
    hud_draw_number(&show_count_number);
}

static void show_count_destroy(struct task *task)
{
    njReleaseTexture(&show_count_texlist);
}

struct task *emblem_start_count_display(void)
{
    struct task *task;

    stream_destroy();
    njLoadTexture(&show_count_texlist);
    stream_reset();
    /* @hack spelling: task assigned in the test, as in shine_create
     * (0x8c0ad680, 0x8c0ad682); assigned apart, 3 of 41 words differ */
    if ((task = task_create(TASK_WORK, 1, show_count_step)) != NULL) {
        WORK(task)->mode = SHOW_INIT;
        WORK(task)->pos.x = SCREEN_MIDDLE_X;
        WORK(task)->pos.y = 256.0F;
        task->destructor = show_count_destroy;
        return task;
    }
    return NULL;
}
