/* how time, fruit and animals change what a Chao's record keeps: its likes
 * and feelings, growth and type, parts and skills. two debug pages print
 * records */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>

#include "al_main/alife.h"
#include "al_main/behavior.h"
#include "al_main/chao.h"
#include "al_main/entry.h"
#include "al_main/shape.h"
#include "al_main/status.h"
#include "animal/animal.h"
#include "chao/chao.h"
#include "collision/collision.h"
#include "draw/draw.h"
#include "player/player.h"
#include "system/pad.h"
#include "system/task.h"
#include "system/util.h"

/* minimal's bit for an animal kind, and minimal with every kind's bit */
#define MINIMAL_BIT(kind) (1 << (ANIMAL_KINDS - 1 - (kind)))
#define MINIMAL_ALL ((1 << ANIMAL_KINDS) - 1)

/* the status page's step to a record's h_pos or v_pos for each press, and
 * the most either reaches on the page, above or below 0 */
#define PAGE_LEAN_STEP 0.05F
#define PAGE_LEAN_LIMIT 2.0F
/* how far the status page's pointer turns about y each step */
#define POINTER_TURN 0x600

/* the animal kinds fall in groups of GROUP_KINDS, in enum order. each group
 * but the last leans a Chao toward its skill, and the last toward one at
 * random */
#define GROUP_KINDS 3
enum lean_group { GROUP_SWIM, GROUP_FLY, GROUP_RUN, GROUP_POWER, GROUP_RANDOM };

/* each timer's limit. a timer ticks once every limit + 2 steps */
#define EMOTION_LIMIT 300
#define LIFE_LIMIT 5400
#define GROWTH_LIMIT 3000
#define AGE_LIMIT 2700
#define OLD_LIMIT 3600
#define BREED_LIMIT 7200
#define SLEEP_LIMIT 12000
#define HUNGER_LIMIT 540
#define TEDIOUS_LIMIT 60

/* the Chao the status page shows, by its index in the entries' Chao list,
 * or -1 for none */
static int status_index = -1;
/* the status page's labels for each enum chao_type and enum chao_part */
static char type_names[CHAO_NOTHING + 1][10] = {
    "CHILD", "STANDARD", "SWIM", "FLY",     "RUN",
    "POWER", "CHAOS",    "EGG",  "NOTHING",
};
static char part_names[CHAO_PARTS][6] = {
    "FHEAD", "HORN", "EAR", "WING", "HAND", "FOOT", "TAIL",
};
/* the status page's labels for the animal kind a part came from, indexed
 * by kind + 1. a part of -1, none, prints "" */
static char animal_names[ANIMAL_KINDS + 1][6] = {
    "",    "GOMA",  "PEN",  "RAKKO", "PECOK", "SWALW", "PAROT", "BANBI",
    "USA", "WALAB", "GORI", "LION",  "ZOU",   "MOGU",  "KOALA", "SKUNK",
};
/* by part slot: the node it fills in the Chao's shape, its enum chao_part,
 * and whether the next slot is its pair */
static Sint16 slot_nodes[CHAO_PART_SLOTS] = {28, 29, 30, 19, 21, 35,
                                             37, 2,  5,  8,  10, 12};
static Sint16 slot_parts[CHAO_PART_SLOTS] = {
    CHAO_PART_FHEAD, CHAO_PART_HORN, CHAO_PART_HORN, CHAO_PART_EAR,
    CHAO_PART_EAR,   CHAO_PART_WING, CHAO_PART_WING, CHAO_PART_HAND,
    CHAO_PART_HAND,  CHAO_PART_FOOT, CHAO_PART_FOOT, CHAO_PART_TAIL,
};
static Sint16 slot_starts_pair[CHAO_PART_SLOTS] = {0, 1, 0, 1, 0, 1,
                                                   0, 1, 0, 1, 0, 0};

/* the timers, a count and a tick each, which status_timers_step steps. a
 * tick is 1 for the step its count restarts, and 0 for every other step */
static Uint16 emotion_count;
static Uint16 life_count;
static Uint16 growth_count;
static Uint16 age_count;
static Uint16 old_count;
static Uint16 breed_count;
static Uint16 sleep_count;
static Uint16 hunger_count;
static Uint16 tedious_count;
static Uint16 emotion_tick;
static Uint16 life_tick;
static Uint16 growth_tick;
static Uint16 age_tick;
static Uint16 old_tick; /* nothing reads it */
static Uint16 breed_tick;
static Uint16 sleep_tick;
static Uint16 hunger_tick;
static Uint16 tedious_tick;
/* how far the status page's pointer has turned about y */
static Angle pointer_spin;
/* 8 bytes no code reads; their type is unknown */
static Uint8 dat_0cb1e880[8];

void status_likes_print(void)
{
    struct chao_node *node = chao_get_list(GARDEN_EGG_CARRIER);
    int column = 0;
    Sint8 like;

    while (node) {
        like = node->chao->likes[0].like;
        njPrintC(NJM_LOCATION(0, 15), "LIKE");
        njPrintD(NJM_LOCATION((column * 5) + 5, 15), like, 4);
        node = node->next;
        column++;
    }
}

void status_page_step(void)
{
    struct alife_entry *entry;
    struct chao *record;
    NJS_POINT3 chao_pos;
    NJS_POINT3 player_pos;
    int count;
    int i;

    if (pad_records[0].press & PDD_DGT_TY) {
        status_index++;
    }
    count = alife_get_chao_count();
    if (count < status_index + 1) {
        status_index = -1;
        return;
    }
    if (status_index == -1) {
        return;
    }

    entry = alife_get_chao_list();
    for (i = 0; i < status_index; i++) {
        if (entry->next) {
            entry = entry->next;
        }
    }
    player_pos = player_work[0]->pos;
    player_pos.y += 5.0F;
    chao_pos = entry->task->work->pos;
    chao_pos.y += 5.0F;
    draw_line_v(&chao_pos, &player_pos);

    /* the pointer, an arrow of three stretched cubes: a shaft and two
     * barbs */
    draw_set_material(0.0F, 1.0F, 1.0F, 0.0F);
    njPushMatrix(NULL);
    njTranslateV(NULL, &entry->task->work->pos);
    TURN(njRotateY, pointer_spin);
    njTranslate(NULL, 0.0F, 14.0F, 0.0F);
    njScale(NULL, 0.5F, 0.3F, 0.5F);
    njRotateX(NULL, NJM_DEG_ANG(90));
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 10.0F);
    njScale(NULL, 0.1F, 0.1F, 2.0F);
    njSimpleDrawObject(marker_cube_object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 20.0F);
    njRotateX(NULL, NJM_DEG_ANG(45));
    njTranslate(NULL, 0.0F, 0.0F, -3.0F);
    njScale(NULL, 0.1F, 0.1F, 0.7F);
    njSimpleDrawObject(marker_cube_object);
    njPopMatrix(1);
    njPushMatrix(NULL);
    njTranslate(NULL, 0.0F, 0.0F, 20.0F);
    njRotateX(NULL, NJM_DEG_ANG(-45));
    njTranslate(NULL, 0.0F, 0.0F, -3.0F);
    njScale(NULL, 0.1F, 0.1F, 0.7F);
    njSimpleDrawObject(marker_cube_object);
    njPopMatrix(1);
    njPopMatrix(1);
    draw_set_material(0.0F, 0.0F, 0.0F, 0.0F);
    pointer_spin += POINTER_TURN;

    record = ((struct alife_chao *)entry->task->motion)->record;
    if (pad_records[0].press & PDD_DGT_KU) {
        record->v_pos += PAGE_LEAN_STEP;
        if (record->v_pos > PAGE_LEAN_LIMIT) {
            record->v_pos = PAGE_LEAN_LIMIT;
        }
    }
    if (pad_records[0].press & PDD_DGT_KD) {
        record->v_pos -= PAGE_LEAN_STEP;
        if (record->v_pos < -PAGE_LEAN_LIMIT) {
            record->v_pos = -PAGE_LEAN_LIMIT;
        }
    }
    if (pad_records[0].press & PDD_DGT_KL) {
        record->h_pos -= PAGE_LEAN_STEP;
        if (record->h_pos < -PAGE_LEAN_LIMIT) {
            record->h_pos = -PAGE_LEAN_LIMIT;
        }
    }
    if (pad_records[0].press & PDD_DGT_KR) {
        record->h_pos += PAGE_LEAN_STEP;
        if (record->h_pos > PAGE_LEAN_LIMIT) {
            record->h_pos = PAGE_LEAN_LIMIT;
        }
    }

    njPrintColor(0x77ee8888);
    njPrintC(NJM_LOCATION(3, 2), "CHAO's STATUS");
    njPrintColor(0x77ffffff);
    njPrintC(NJM_LOCATION(3, 3), "GD");
    njPrintD(NJM_LOCATION(6, 3), record->garden, 3);
    njPrintD(NJM_LOCATION(17, 2), status_index, 2);
    njPrintC(NJM_LOCATION(19, 2), "/");
    njPrintD(NJM_LOCATION(20, 2), count, 2);
    njPrintD(NJM_LOCATION(20, 3), chao_count(record->garden), 2);

    njPrintColor(0x77ee7044);
    njPrintC(NJM_LOCATION(1, 4), "TYPE");
    njPrintColor(0x77ffffff);
    njPrintC(NJM_LOCATION(6, 4), type_names[record->type]);
    njPrintColor(0x77ee7044);
    njPrintC(NJM_LOCATION(1, 5), "NAME");
    njPrintColor(0x77ffffff);
    njPrintC(NJM_LOCATION(6, 5), "???????");
    njPrintColor(0x77ee7044);
    njPrintC(NJM_LOCATION(1, 6), "AGE");
    njPrintColor(0x77ffffff);
    njPrintH(NJM_LOCATION(6, 6), record->age, 4);
    njPrintColor(0x77ee7044);
    njPrintC(NJM_LOCATION(1, 7), "USER LIKE");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(8, 7), record->user_like, 5);
    njPrintColor(0x77ee7044);
    njPrintC(NJM_LOCATION(1, 8), "PL LIKE");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(8, 8), record->likes[alife_character].like, 3);
    njPrintColor(0x77ee7044);
    njPrintC(NJM_LOCATION(1, 9), "HP");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(8, 9), record->hp, 5);
    njPrintColor(0x77ee7044);
    njPrintC(NJM_LOCATION(1, 10), "HP MAX");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(8, 10), record->hp_max, 5);
    njPrintColor(0x77ee7044);
    njPrintC(NJM_LOCATION(1, 11), "LIFE");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(8, 11), record->life, 5);
    njPrintColor(0x77ee7044);
    njPrintC(NJM_LOCATION(1, 12), "OLD");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(8, 12), record->old, 5);
    njPrintColor(0x77ee8888);
    njPrintC(NJM_LOCATION(1, 14), "ID");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(3, 14), record->id, 10);
    /* minimal in binary from bit 15 at column 9, and above it the animal
     * kinds' initials in enum order, in groups of three. kind k's bit,
     * 14 - k, prints at column 10 + k, and its initial at 9 + k */
    njPrintColor(0x7755ff55);
    njPrintC(NJM_LOCATION(1, 26), "MINIMAL");
    njPrintColor(0x77ffffff);
    njPrintB(NJM_LOCATION(9, 27), record->minimal, 16);
    njPrintColor(0x77cc88cc);
    njPrintC(NJM_LOCATION(9, 26), "APR");
    njPrintColor(0x77ddcc00);
    njPrintC(NJM_LOCATION(12, 26), "KSO");
    njPrintColor(0x778888ee);
    njPrintC(NJM_LOCATION(15, 26), "BUW");
    njPrintColor(0x77ee8888);
    njPrintC(NJM_LOCATION(18, 26), "GLE");
    njPrintColor(0x77ee7044);
    njPrintC(NJM_LOCATION(21, 26), "MKS");
    njPrintColor(0x7755ff55);
    njPrintC(NJM_LOCATION(1, 17), "PARTS INFO");
    for (i = 0; i < CHAO_PARTS; i++) {
        njPrintColor(0x7788cc88);
        njPrintC(NJM_LOCATION(1, i + 18), part_names[i]);
        njPrintColor(0x77ffffff);
        njPrintC(NJM_LOCATION(7, i + 18), animal_names[record->parts[i] + 1]);
    }

    njPrintColor(0x778888ee);
    njPrintC(NJM_LOCATION(14, 4), "H POS");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(21, 4), (int)(record->h_pos * 1000.0F), 4);
    njPrintColor(0x778888ee);
    njPrintC(NJM_LOCATION(14, 5), "V POS");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(21, 5), (int)(record->v_pos * 1000.0F), 4);
    njPrintColor(0x778888ee);
    njPrintC(NJM_LOCATION(14, 6), "GROWTH");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(21, 6), (int)(record->growth * 1000.0F), 4);
    njPrintColor(0x77ddcc00);
    njPrintC(NJM_LOCATION(14, 8), "CHARM");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(22, 8), record->charm, 3);
    njPrintColor(0x77ddcc00);
    njPrintC(NJM_LOCATION(14, 9), "BREED");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(22, 9), record->breed, 3);
    njPrintColor(0x77ddcc00);
    njPrintC(NJM_LOCATION(14, 10), "SLEEP");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(22, 10), record->sleep, 3);
    njPrintColor(0x77ddcc00);
    njPrintC(NJM_LOCATION(14, 11), "HUNGER");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(22, 11), record->hunger, 3);
    njPrintColor(0x77ddcc00);
    njPrintC(NJM_LOCATION(14, 12), "TEDIOUS");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(22, 12), record->tedious, 3);
    njPrintColor(0x77ddcc00);
    njPrintC(NJM_LOCATION(14, 17), "PLEASURE");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(22, 17), record->pleasure, 3);
    njPrintColor(0x77ddcc00);
    njPrintC(NJM_LOCATION(14, 18), "ANGER");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(22, 18), record->anger, 3);
    njPrintColor(0x77ddcc00);
    njPrintC(NJM_LOCATION(14, 19), "SORROW");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(22, 19), record->sorrow, 3);
    njPrintColor(0x77ddcc00);
    njPrintC(NJM_LOCATION(14, 20), "FEAR");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(22, 20), record->fear, 3);
    /* the image calls njPrintColor twice here, with the same color */
    njPrintColor(0x77ddcc00);
    njPrintColor(0x77ddcc00);
    njPrintC(NJM_LOCATION(14, 23), "KIND");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(22, 23), record->kind, 3);
    njPrintColor(0x77ddcc00);
    njPrintC(NJM_LOCATION(14, 24), "AGRES");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(22, 24), record->agres, 3);
    njPrintColor(0x77ddcc00);
    njPrintC(NJM_LOCATION(14, 25), "CURIO");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(22, 25), record->curio, 3);

    njPrintColor(0x77cc88cc);
    njPrintC(NJM_LOCATION(26, 6), "SIZE");
    njPrintColor(0x77ffffff);
    njPrintH(NJM_LOCATION(30, 6), sizeof(struct chao), 5);
    njPrintColor(0x77cc88cc);
    njPrintC(NJM_LOCATION(26, 9), "SWIM");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 9), record->skills.swim, 5);
    njPrintColor(0x77cc88cc);
    njPrintC(NJM_LOCATION(26, 10), "FLY");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 10), record->skills.fly, 5);
    njPrintColor(0x77cc88cc);
    njPrintC(NJM_LOCATION(26, 11), "RUN");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 11), record->skills.run, 5);
    njPrintColor(0x77cc88cc);
    njPrintC(NJM_LOCATION(26, 12), "POWER");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 12), record->skills.power, 5);
    njPrintColor(0x77cc88cc);
    njPrintC(NJM_LOCATION(26, 14), "SWIM_PT");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 14), record->points.swim, 5);
    njPrintColor(0x77cc88cc);
    njPrintC(NJM_LOCATION(26, 15), "FLY_PT");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 15), record->points.fly, 5);
    njPrintColor(0x77cc88cc);
    njPrintC(NJM_LOCATION(26, 16), "RUN_PT");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 16), record->points.run, 5);
    njPrintColor(0x77cc88cc);
    njPrintC(NJM_LOCATION(26, 17), "POWER_PT");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 17), record->points.power, 5);

    njPrintColor(0xaa8888ff);
    njPrintC(NJM_LOCATION(26, 18), "TIMER");
    /* the EMOTION row prints growth_count, as the GROWTH row does, and not
     * emotion_count (0x0c90d546, 0x0c90d562) */
    njPrintColor(0x778888ee);
    njPrintC(NJM_LOCATION(26, 19), "EMOTION");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 19), growth_count, 5);
    njPrintColor(0x778888ee);
    njPrintC(NJM_LOCATION(26, 20), "GROWTH");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 20), growth_count, 5);
    njPrintColor(0x778888ee);
    njPrintC(NJM_LOCATION(26, 21), "AGE");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 21), age_count, 5);
    njPrintColor(0x778888ee);
    njPrintC(NJM_LOCATION(26, 22), "OLD");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 22), old_count, 5);
    njPrintColor(0x778888ee);
    njPrintC(NJM_LOCATION(26, 23), "BREED");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 23), breed_count, 5);
    njPrintColor(0x778888ee);
    njPrintC(NJM_LOCATION(26, 24), "SLEEP");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 24), sleep_count, 5);
    njPrintColor(0x778888ee);
    njPrintC(NJM_LOCATION(26, 25), "HUNGER");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 25), hunger_count, 5);
    njPrintColor(0x778888ee);
    njPrintC(NJM_LOCATION(26, 26), "TEDIOUS");
    njPrintColor(0x77ffffff);
    njPrintD(NJM_LOCATION(34, 26), tedious_count, 5);
    njPrintColor(0xffffffff);
}

void fun_0c90d634(struct task_work *work, struct alife_chao *chao)
{
}

void status_raise_like(struct task_work *work, struct alife_chao *chao,
                       int amount)
{
    struct chao_like *like = &chao->record->likes[alife_character];
    struct chao *record = chao->record;

    like->like += amount;
    if (like->like > 100) {
        like->like = 100;
    } else if (like->like < -100) {
        like->like = -100;
    }
    like->raised++;
    if (like->raised > 200) {
        like->raised = 200;
    }

    record->user_like += amount;
    if (record->user_like > 100) {
        record->user_like = 100;
    } else if (record->user_like < -100) {
        record->user_like = -100;
    }
    record->like_changes++;
    if (record->like_changes > 9999) {
        record->like_changes = 9999;
    }
}

void status_hurt(struct task_work *work, struct alife_chao *chao, int amount)
{
    struct chao_like *like = &chao->record->likes[alife_character];
    struct chao *record = chao->record;

    like->like -= amount;
    if (like->like > 100) {
        like->like = 100;
    } else if (like->like < -100) {
        like->like = -100;
    }

    record->user_like -= amount;
    if (record->user_like > 100) {
        record->user_like = 100;
    } else if (record->user_like < -100) {
        record->user_like = -100;
    }
    if (record->life > 0) {
        record->life--;
    }
    record->like_changes++;
    if (record->like_changes > 9999) {
        record->like_changes = 9999;
    }
    record->pleasure = 0;
}

void status_timers_step(void)
{
    if (emotion_count++ > EMOTION_LIMIT) {
        emotion_tick = 1;
        emotion_count = 0;
    } else {
        emotion_tick = 0;
    }
    if (life_count++ > LIFE_LIMIT) {
        life_tick = 1;
        life_count = 0;
    } else {
        life_tick = 0;
    }
    if (growth_count++ > GROWTH_LIMIT) {
        growth_tick = 1;
        growth_count = 0;
    } else {
        growth_tick = 0;
    }
    if (age_count++ > AGE_LIMIT) {
        age_tick = 1;
        age_count = 0;
    } else {
        age_tick = 0;
    }
    if (old_count++ > OLD_LIMIT) {
        old_tick = 1;
        old_count = 0;
    } else {
        old_tick = 0;
    }
    if (breed_count++ > BREED_LIMIT) {
        breed_tick = 1;
        breed_count = 0;
    } else {
        breed_tick = 0;
    }
    if (sleep_count++ > SLEEP_LIMIT) {
        sleep_tick = 1;
        sleep_count = 0;
    } else {
        sleep_tick = 0;
    }
    if (hunger_count++ > HUNGER_LIMIT) {
        hunger_tick = 1;
        hunger_count = 0;
    } else {
        hunger_tick = 0;
    }
    if (tedious_count++ > TEDIOUS_LIMIT) {
        tedious_tick = 1;
        tedious_count = 0;
    } else {
        tedious_tick = 0;
    }
    /* @hack redundant: a return at the end, which gives the last timer's
     * restart an RTS of its own, at 0x0c90d9d0; without it, the restart
     * branches to the shared RTS, and 1 of 130 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

void status_step(void)
{
    int garden;
    struct chao_node *node;
    struct chao *record;
    int life;
    int i;
    Uint8 count;
    struct chao *egg;

    status_timers_step();
    for (garden = 0; garden < GARDEN_COUNT; garden++) {
        node = chao_get_list(garden);
        while (node) {
            record = node->chao;
            if (record->type == CHAO_EGG || record->type == CHAO_NOTHING) {
                node = node->next;
                continue;
            }
            if (life_tick) {
                if (record->type != CHAO_CHILD && record->type != CHAO_EGG &&
                    record->type != CHAO_NOTHING &&
                    record->type != CHAO_CHAOS) {
                    life = record->life;
                    life -= (record->old / 200) + 1;
                    if (life < 0) {
                        life = 0;
                        if (alife_garden != record->garden) {
                            if (record->user_like > 80) {
                                chao_reborn(record);
                            } else {
                                chao_list_remove(record);
                                record->type = CHAO_NOTHING;
                            }
                        }
                    }
                    /* these stores run after chao_reborn too, and replace
                     * the reborn record's life and old with 0 and 1
                     * (0x0c90db0c, 0x0c90db14) */
                    record->life = life;
                    record->old++;
                }
            }
            if (emotion_tick) {
                short value;
                for (i = 0; i < CHAO_PLAYERS; i++) {
                    /* @bug arithmetic: raised is an unsigned byte, so a
                     * count of 0 falls to 255, which the clamp to at least
                     * 10 lets stand, at 0x0c90db30 and 0x0c90db3e */
                    record->likes[i].raised--;
                    if (record->likes[i].raised < 10) {
                        record->likes[i].raised = 10;
                        /* @hack redundant: a discarded read of the count
                         * after the clamp's store, for which SHC works out
                         * the store's address afresh from the record and
                         * R5, at 0x0c90db44 to 0x0c90db4a, ahead of the
                         * store at 0x0c90db4c; without it, 409 of 517 words
                         * differ, and the code after it moves 32 bytes
                         * back */
                        (void)record->likes[i].raised;
                    }
                }
                value = record->pleasure;
                value -= 5;
                if (value < 0) {
                    value = 0;
                }
                record->pleasure = value;
                value = record->anger;
                value -= 5 - ((record->agres + 100) / 40);
                if (value < 0) {
                    value = 0;
                }
                record->anger = value;
                value = record->sorrow;
                value -= 5 - ((100 - record->curio) / 40);
                if (value < 0) {
                    value = 0;
                }
                record->sorrow = value;
                value = record->fear;
                value -= 5 - ((100 - record->agres) / 40);
                if (value < 0) {
                    value = 0;
                }
                record->fear = value;
                value = record->unk_6b;
                value -= 5;
                if (value < 0) {
                    value = 0;
                }
                record->unk_6b = value;
            }
            if (growth_tick) {
                record->flags |= CHAO_GROWTH_DUE;
                if (alife_garden != record->garden) {
                    if (record->type == CHAO_CHILD) {
                        record->growth += 0.008F;
                    } else {
                        record->growth += 0.004F;
                    }
                    if (record->growth > 1.5F) {
                        record->growth = 1.5F;
                    }
                    if (record->growth > 1.0F && record->type == CHAO_CHILD) {
                        status_evolve_record(record);
                    }
                }
            }
            if (age_tick) {
                /* @hack spelling: age = age + 1, after which SHC loads
                 * breed_tick's address into R2 and the tick into R3, at
                 * 0x0c90dd4e and 0x0c90dd50; as ++, the address and the tick
                 * swap registers, and 19 of 517 words differ */
                record->age = record->age + 1;
            }
            if (record->type != CHAO_CHILD && record->type != CHAO_EGG &&
                record->type != CHAO_NOTHING && record->type != CHAO_CHAOS) {
                if (breed_tick) {
                    record->breed += (int)((400 - record->unk_65) * 0.015F);
                    if (record->breed > 200) {
                        record->breed = 200;
                        if (alife_garden != record->garden) {
                            if (rand() * RAND_SCALE < 0.1F) {
                                count = chao_count(record->garden);
                                if (count > 1 && count < 8) {
                                    egg = chao_find_empty();
                                    if (egg) {
                                        chao_init_egg(egg);
                                        chao_list_add(record->garden, egg);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (sleep_tick) {
                record->sleep += (int)((record->unk_64 + 200) * 0.025F);
                if (record->sleep > 200) {
                    record->sleep = 200;
                }
            }
            if (alife_garden == record->garden) {
                if (hunger_tick) {
                    record->hunger++;
                    if (record->hunger > 200) {
                        record->hunger = 200;
                    }
                }
                if (tedious_tick) {
                    record->tedious++;
                    if (record->tedious > 200) {
                        record->tedious = 200;
                    }
                }
            }
            node = node->next;
        }
    }
}

void status_evolve(struct task_work *work, struct alife_chao *chao)
{
    struct chao *record = chao->record;
    Float h_pos = record->h_pos;
    Float v_pos = record->v_pos;
    Float sum = h_pos + v_pos;
    Float difference = h_pos - v_pos;

    if (h_pos > -0.3F && h_pos < 0.3F && v_pos > -0.3F && v_pos < 0.3F) {
        record->type = CHAO_STANDARD;
        /* @hack spelling: minimal and user_like read through chao->record, so
         * that SHC loads the record from the Chao again for each, at
         * 0x0c90df34 and 0x0c90df46; as record->minimal and
         * record->user_like, 55 of 103 words differ */
        if ((~chao->record->minimal & MINIMAL_ALL) == 0) {
            if (chao->record->user_like > 90) {
                if (record->generation >= 2) {
                    record->type = CHAO_CHAOS;
                }
            }
        }
    } else if (sum < 0.0F && difference >= 0.0F) {
        record->type = CHAO_SWIM;
    } else if (sum >= 0.0F && difference < 0.0F) {
        record->type = CHAO_FLY;
    } else if (sum >= 0.0F && difference >= 0.0F) {
        record->type = CHAO_RUN;
    } else if (sum < 0.0F && difference < 0.0F) {
        record->type = CHAO_POWER;
    }
    record->growth *= 0.1F;
    chao->growth = record->growth;
    record->h_pos *= 0.1F;
    record->v_pos *= 0.1F;
    chao->h_pos = record->h_pos;
    chao->v_pos = record->v_pos;
}

void status_evolve_record(struct chao *record)
{
    Float h_pos = record->h_pos;
    Float v_pos = record->v_pos;
    Float sum = h_pos + v_pos;
    Float difference = h_pos - v_pos;

    if (h_pos > -0.3F && h_pos < 0.3F && v_pos > -0.3F && v_pos < 0.3F) {
        record->type = CHAO_STANDARD;
        if ((~record->minimal & MINIMAL_ALL) == 0) {
            if (record->user_like > 90) {
                if (record->generation >= 2) {
                    record->type = CHAO_CHAOS;
                }
            }
        }
    } else if (sum < 0.0F && difference >= 0.0F) {
        record->type = CHAO_SWIM;
    } else if (sum >= 0.0F && difference < 0.0F) {
        record->type = CHAO_FLY;
    } else if (sum >= 0.0F && difference >= 0.0F) {
        record->type = CHAO_RUN;
    } else if (sum < 0.0F && difference < 0.0F) {
        record->type = CHAO_POWER;
    }
    record->growth *= 0.1F;
    record->h_pos *= 0.1F;
    record->v_pos *= 0.1F;
}

void status_clamp(struct task_work *work, struct alife_chao *chao)
{
    struct chao *record = chao->record;

    if (chao->h_pos > 1.5F) {
        chao->h_pos = 1.5F;
    } else if (chao->h_pos < -1.5F) {
        chao->h_pos = -1.5F;
    }
    if (chao->v_pos > 1.5F) {
        chao->v_pos = 1.5F;
    } else if (chao->v_pos < -1.5F) {
        chao->v_pos = -1.5F;
    }
    if (chao->growth > 1.5F) {
        chao->growth = 1.5F;
    } else if (chao->growth < 0.0F) {
        chao->growth = 0.0F;
    }
    if (record->life > 9999) {
        record->life = 9999;
    }
    if (record->skills.swim > 9999) {
        record->skills.swim = 9999;
    }
    if (record->skills.fly > 9999) {
        record->skills.fly = 9999;
    }
    if (record->skills.run > 9999) {
        record->skills.run = 9999;
    }
    if (record->skills.power > 9999) {
        record->skills.power = 9999;
    }
    if (record->hp_max > 9999) {
        record->hp_max = 9999;
    }
    if (record->hp > record->hp_max) {
        record->hp = record->hp_max;
    }
}

void status_give_parts(struct task_work *work, struct alife_chao *chao,
                       Uint8 kind)
{
    struct chao_shape *shape = &chao->shape;
    Uint8 type = chao->record->type;
    int i;
    /* a slot's part, boxed in an array of one, which the frame holds at
     * sp+40 */
    struct boxed_part {
        short value[1];
    };
    /* a slot's two trees, as child_part_objects holds them */
    typedef NJS_OBJECT *slot_trees[2];
    /* one kind's row of child_part_objects, as a struct for the casts
     * below to read it through */
    struct kind_slots {
        slot_trees slots[CHAO_PART_SLOTS];
    };

    /* the first three slots take a tree's model, and the rest a tree's
     * child */
    for (i = 0; i < 3; i++) {
        NJS_OBJECT *source;
        short index = slot_nodes[i];
        /* @hack type: the part boxed in an array of one, so that SHC stores
         * it through R1, at 0x0c90e28a, and both arms load source into R2
         * and share one store of it, at 0x0c90e30c; as a short, 44 of 516
         * words differ */
        struct boxed_part part;
        part.value[0] = slot_parts[i];
        if (!type) {
            /* @hack type: kind's row read as a struct's array, with i added
             * first, so that SHC puts i * 8 in R3 and the row in R1 and adds
             * them into R3, at 0x0c90e29a; without it, 99 of 516 words
             * differ */
            source = (*(
                i + ((struct kind_slots *)child_part_objects[kind])->slots))[1];
        } else {
            source = adult_part_objects[kind][i][1];
        }
        if (source == NULL) {
            if (rand() * RAND_SCALE < 0.4F) {
                shape->objects[index]->model = NULL;
                chao->record->parts[slot_parts[i]] = -1;
                if (slot_starts_pair[i]) {
                    ++i;
                    index = slot_nodes[i];
                    shape->objects[index]->model = NULL;
                }
            } else if (slot_starts_pair[i]) {
                ++i;
            }
        } else if (rand() * RAND_SCALE <
                   animal_part_chances[kind][part.value[0]]) {
            NJS_MODEL *model;
            NJS_MATERIAL *material;
            unsigned short count;
            int j;
            shape->objects[index]->model = source->model;
            chao->record->parts[slot_parts[i]] = kind;
            model = shape->objects[index]->model;
            /* @hack redundant: a never-run test of model, which keeps the
             * adult row at sp+8 and index * 4 at sp+4, at 0x0c90e254 and
             * 0x0c90e312; without it, the two swap slots, and 7 of 516 words
             * differ */
            if (0) {
                if (model) {
                }
            }
            count = model->nbMat;
            material = model->mats;
            for (j = 0; j < count; ++j, ++material) {
                material->diffuse.argb.r = 255;
                material->diffuse.argb.g = 255;
                material->diffuse.argb.b = 255;
            }
            if (slot_starts_pair[i]) {
                ++i;
                index = slot_nodes[i];
                if (!type) {
                    /* @hack type: kind's row cast to the struct and back,
                     * so that SHC loads the row once for the add both arms
                     * share, at 0x0c90e436 and 0x0c90e442; without it, 94
                     * of 516 words differ */
                    source = ((slot_trees *)(struct kind_slots *)
                                  child_part_objects[kind])[i][1];
                } else {
                    source = adult_part_objects[kind][i][1];
                }
                shape->objects[index]->model = source->model;
            }
            model = shape->objects[index]->model;
            count = model->nbMat;
            material = model->mats;
            /* @hack redundant: a never-run test of count, which keeps chao
             * at sp+16, the child row at sp+20 and source at sp+12, at
             * 0x0c90e21c, 0x0c90e252 and 0x0c90e30c; without it, they take
             * other slots, and 41 of 516 words differ */
            if (0) {
                if (count) {
                }
            }
            for (j = 0; j < count; ++j, ++material) {
                material->diffuse.argb.r = 255;
                material->diffuse.argb.g = 255;
                material->diffuse.argb.b = 255;
            }
        } else if (slot_starts_pair[i]) {
            ++i;
        }
    }
    for (i = 3; i < CHAO_PART_SLOTS; i++) {
        NJS_OBJECT *source;
        short index = slot_nodes[i];
        /* @hack type: the part boxed in an array of one, so that SHC stores
         * it through R1, at 0x0c90e514, and both arms load source into R2
         * and share one store of it, at 0x0c90e538; as a short, 37 of 516
         * words differ */
        struct boxed_part part;
        part.value[0] = slot_parts[i];
        if (!type) {
            /* @hack type: kind's row read as a struct's array, with i added
             * first, so that SHC puts i * 8 in R3 and the row in R1 and adds
             * them into R3, at 0x0c90e524; without it, 104 of 516 words
             * differ */
            source = (*(
                i + ((struct kind_slots *)child_part_objects[kind])->slots))[1];
        } else {
            source = adult_part_objects[kind][i][1];
        }
        if (source == NULL) {
            if (rand() * RAND_SCALE < 0.4F) {
                shape->objects[index]->child = shape->objects[index + 1];
                chao->record->parts[slot_parts[i]] = -1;
                if (slot_starts_pair[i]) {
                    ++i;
                    index = slot_nodes[i];
                    shape->objects[index]->child = shape->objects[index + 1];
                }
            } else if (slot_starts_pair[i]) {
                ++i;
            }
        } else if (rand() * RAND_SCALE <
                   animal_part_chances[kind][part.value[0]]) {
            NJS_MODEL *model;
            NJS_MATERIAL *material;
            unsigned short count;
            int j;
            shape->objects[index]->child = source->child;
            chao->record->parts[slot_parts[i]] = kind;
            model = shape->objects[index]->child->model;
            /* @hack redundant: a never-run test of model, which keeps the
             * child row at sp+28 and the adult row at sp+8, at 0x0c90e4e2 and
             * 0x0c90e4ea; without it, the two swap slots, and 6 of 516 words
             * differ */
            if (0) {
                if (model) {
                }
            }
            count = model->nbMat;
            material = model->mats;
            for (j = 0; j < count; ++j, ++material) {
                material->diffuse.argb.r = 255;
                material->diffuse.argb.g = 255;
                material->diffuse.argb.b = 255;
            }
            if (slot_starts_pair[i]) {
                ++i;
                index = slot_nodes[i];
                if (!type) {
                    /* @hack type: kind's row cast to the struct and back,
                     * so that SHC loads the row once for the add both arms
                     * share, at 0x0c90e682 and 0x0c90e6c6; without it, 115
                     * of 516 words differ */
                    source = ((slot_trees *)(struct kind_slots *)
                                  child_part_objects[kind])[i][1];
                } else {
                    source = adult_part_objects[kind][i][1];
                }
                shape->objects[index]->child = source->child;
            }
            model = shape->objects[index]->child->model;
            count = model->nbMat;
            material = model->mats;
            for (j = 0; j < count; ++j, ++material) {
                material->diffuse.argb.r = 255;
                material->diffuse.argb.g = 255;
                material->diffuse.argb.b = 255;
            }
            /* @hack redundant: a never-run test of j, which keeps the type
             * at sp+24, the first loop's child row at sp+20 and adult row
             * at sp+8, and the chances' row at sp+28, at 0x0c90e232,
             * 0x0c90e252, 0x0c90e254 and 0x0c90e270; without it, they take
             * other slots, and 23 of 516 words differ */
            if (0) {
                if (j) {
                }
            }
        } else if (slot_starts_pair[i]) {
            ++i;
        }
    }
}

void status_give_skills(struct task_work *work, struct alife_chao *chao,
                        Uint8 kind)
{
    struct chao *record = chao->record;

    /* @bug memory: the test lets ANIMAL_ZOU through, though
     * animal_skill_bonuses has rows only up to ANIMAL_LION. ZOU's row is the
     * 8 bytes past the table, child_part_objects' first two pointers, which
     * are null, so it adds nothing, at 0x0c90e742, 0x0c90e760 and
     * 0x0c90e7a2 */
    if (kind < ANIMAL_MOGU) {
        if (record->type == CHAO_SWIM) {
            record->skills.swim += animal_skill_bonuses[kind][0] * 2;
        } else {
            record->skills.swim += animal_skill_bonuses[kind][0];
        }
        if (record->type == CHAO_FLY) {
            record->skills.fly += animal_skill_bonuses[kind][1] * 2;
        } else {
            record->skills.fly += animal_skill_bonuses[kind][1];
        }
        if (record->type == CHAO_RUN) {
            record->skills.run += animal_skill_bonuses[kind][2] * 2;
        } else {
            record->skills.run += animal_skill_bonuses[kind][2];
        }
        if (record->type == CHAO_POWER) {
            record->skills.power += animal_skill_bonuses[kind][3] * 2;
        } else {
            record->skills.power += animal_skill_bonuses[kind][3];
        }
    } else if (rand() * RAND_SCALE > 0.5F) {
        if (rand() * RAND_SCALE > 0.5F) {
            record->skills.swim += 100;
        } else {
            record->skills.fly += 100;
        }
    } else {
        if (rand() * RAND_SCALE > 0.5F) {
            record->skills.run += 100;
        } else {
            record->skills.power += 100;
        }
    }
}

void status_add_minimal(struct task_work *work, struct alife_chao *chao,
                        Uint8 kind)
{
    chao->record->minimal |= MINIMAL_BIT(kind);
}

void status_eat_fruit(struct task_work *work, struct alife_chao *chao,
                      Sint8 kind)
{
    struct chao *record = chao->record;

    status_raise_like(work, chao, 3);
    if (record->hunger > 50) {
        record->hunger -= 50;
    } else {
        record->hunger = 0;
    }
    record->sleep += 50;
    if (record->sleep > 200) {
        record->sleep = 200;
    }
    record->pleasure += 40;
    if (record->pleasure > 200) {
        record->pleasure = 200;
    }
    /* @hack spelling: the type read through chao->record, so that SHC loads the
     * record from the Chao again, at 0x0c90e916; as record->type, 10 of 180
     * words differ */
    if (chao->record->type == CHAO_CHILD) {
        chao->growth += 0.03F;
    } else {
        chao->growth += 0.016F;
    }
    /* @hack spelling: the life read through chao->record, so that SHC loads the
     * record from the Chao again, at 0x0c90e970; as record->life, 119 of 180
     * words and 15 of the function's 64 words of data differ */
    chao->record->life += 5;
    switch ((Uint8)kind) {
    case 0:
        break;
    case 1:
        record->hp_max += 120;
        record->hp = record->hp_max;
        record->breed = 200;
        break;
    case 2:
        record->skills.power += 20;
        record->hp += 120;
        record->hp_max += 40;
        break;
    case 3:
        record->skills.swim += 20;
        record->hp += 120;
        record->hp_max += 40;
        break;
    case 4:
        record->skills.run += 20;
        record->hp += 120;
        record->hp_max += 40;
        break;
    case 5:
        record->skills.fly += 20;
        record->hp += 120;
        record->hp_max += 40;
        break;
    case 6:
        record->skills.swim += 20;
        record->skills.fly += 20;
        record->skills.run += 20;
        record->skills.power += 20;
        record->hp_max += 80;
        record->hp = record->hp_max;
        break;
    case 7:
        chao->unk_0e8 = 3600;
        break;
    case 8:
        record->agres += 20;
        if (record->agres > 100) {
            record->agres = 100;
        }
        record->anger = 200;
        break;
    case 9:
        if (record->sleep < 150) {
            record->sleep = 150;
        }
        record->fear = 200;
        record->sorrow = 200;
        break;
    case 10:
        /* @hack spelling: hp and hp_max read through chao->record, so that SHC
         * loads the record from the Chao again for each, at 0x0c90ea4c and
         * 0x0c90ea56; as record->hp and record->hp_max, 76 of 180 words and 6
         * of the function's 64 words of data differ */
        chao->record->hp += 40;
        chao->record->hp_max += 40;
        break;
    }
    status_clamp(work, chao);
    alife_shape_blend(work, chao);
}

void status_lean(struct task_work *work, struct alife_chao *chao, int kind)
{
    struct chao *record = chao->record;
    Float factor = 1.0F;

    if (record->type == CHAO_CHILD) {
        switch ((Uint8)kind / GROUP_KINDS) {
        case GROUP_SWIM:
            if (record->skills.swim < record->points.swim) {
                factor = 2.0F;
            } else {
                factor = 1.0F;
            }
            chao->v_pos -= factor * 0.03F;
            break;
        case GROUP_FLY:
            if (record->skills.fly < record->points.fly) {
                factor = 2.0F;
            } else {
                factor = 1.0F;
            }
            chao->v_pos += factor * 0.03F;
            break;
        case GROUP_RUN:
            if (record->skills.run < record->points.run) {
                factor = 2.0F;
            } else {
                factor = 1.0F;
            }
            chao->h_pos += factor * 0.03F;
            break;
        case GROUP_POWER:
            if (record->skills.power < record->points.power) {
                factor = 2.0F;
            } else {
                factor = 1.0F;
            }
            chao->h_pos -= factor * 0.03F;
            break;
        case GROUP_RANDOM:
            if (rand() * RAND_SCALE > 0.5F) {
                if (rand() * RAND_SCALE > 0.5F) {
                    chao->v_pos += 0.15F;
                } else {
                    chao->v_pos -= 0.15F;
                }
            } else {
                if (rand() * RAND_SCALE > 0.5F) {
                    chao->h_pos += 0.15F;
                } else {
                    chao->h_pos -= 0.15F;
                }
            }
            break;
        }
        chao->growth += factor * 0.015F;
    } else {
        switch ((Uint8)kind / GROUP_KINDS) {
        case GROUP_SWIM:
            if (record->skills.swim < record->points.swim) {
                factor = 2.0F;
            } else {
                factor = 1.0F;
            }
            if (record->type == CHAO_SWIM) {
                chao->v_pos -= factor * 0.0225F;
            } else {
                chao->v_pos -= factor * 0.015F;
            }
            break;
        case GROUP_FLY:
            if (record->skills.fly < record->points.fly) {
                factor = 2.0F;
            } else {
                factor = 1.0F;
            }
            if (record->type == CHAO_FLY) {
                chao->v_pos += factor * 0.0225F;
            } else {
                chao->v_pos += factor * 0.015F;
            }
            break;
        case GROUP_RUN:
            if (record->skills.run < record->points.run) {
                factor = 2.0F;
            } else {
                factor = 1.0F;
            }
            if (record->type == CHAO_RUN) {
                chao->h_pos += factor * 0.0225F;
            } else {
                chao->h_pos += factor * 0.015F;
            }
            break;
        case GROUP_POWER:
            if (record->skills.power < record->points.power) {
                factor = 2.0F;
            } else {
                factor = 1.0F;
            }
            if (record->type == CHAO_POWER) {
                chao->h_pos -= factor * 0.0225F;
            } else {
                chao->h_pos -= factor * 0.015F;
            }
            break;
        case GROUP_RANDOM:
            if (rand() * RAND_SCALE > 0.5F) {
                if (rand() * RAND_SCALE > 0.5F) {
                    chao->v_pos += 0.075F;
                } else {
                    chao->v_pos -= 0.075F;
                }
            } else {
                if (rand() * RAND_SCALE > 0.5F) {
                    chao->h_pos += 0.075F;
                } else {
                    chao->h_pos -= 0.075F;
                }
            }
            break;
        }
        chao->growth += factor * 0.008F;
    }
    status_clamp(work, chao);
    alife_shape_blend(work, chao);
    /* @hack redundant: a return after the last call, with which SHC reserves a
     * stack slot with ADD #-4,R15 and stores work in it, at 0x0c90eaae and
     * 0x0c90eab4; without it, SHC pushes work with MOV.L R4,@-R15, and 51
     * of 330 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

Bool status_require_minimal(struct alife_chao *chao, int kind)
{
    struct chao *record = chao->record;
    struct task_work *work = chao->task->work;

    if (record->minimal & MINIMAL_BIT(kind)) {
        return TRUE;
    }
    work->mode = MODE_IDLE;
    work->submode = 0;
    work->timer = 0;
    record->tedious += 20;
    if (record->tedious > 200) {
        record->tedious = 200;
    }
    return FALSE;
}

Bool status_in_minimal(struct alife_chao *chao, int kind)
{
    struct chao *record = chao->record;

    /* @hack redundant: a discarded read of the Chao's task, which keeps the
     * record in R6, at 0x0c90eeca and 0x0c90eed0; without it, the record
     * takes R4, and 2 of 16 words differ */
    (void)chao->task;
    if (record->minimal & MINIMAL_BIT(kind)) {
        return TRUE;
    }
    return FALSE;
}
