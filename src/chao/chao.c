/* the Chao records and each garden's list of them, the carried Chao, and
 * the row of animals the player carries */
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <stdlib.h>
#include <string.h>

#include "al_main/animal.h"
#include "al_main/card.h"
#include "animal/animal.h"
#include "camera/camera.h"
#include "chao/chao.h"
#include "player/player.h"
#include "stage/stage.h"
#include "system/task.h"
#include "system/util.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 8
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 8
#define TEXT_7_LENGTH 9
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 9
#define TEXT_10_LENGTH 9
#define TEXT_11_LENGTH 9
#define TEXT_12_LENGTH 8
#define TEXT_13_LENGTH 8
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 9
#include "chao/chao/text.inc"

#define POINTS_MAX 9999
#define GENERATION_MAX 200
/* a new egg's skill points gain SKILL_DRAWS draws of POINTS_PER_DRAW, each
 * to a skill picked at random */
#define SKILL_DRAWS 10
#define POINTS_PER_DRAW 100
/* the new eggs each garden gets when neither the records nor the a-life
 * file hold a Chao */
#define NEW_EGGS 2
/* a garden holding GARDEN_CROWDED Chao or more sends one away. the Chao
 * goes to the garden after it in the cycle Station Square, Egg Carrier,
 * Mystic Ruins, if that one holds fewer than GARDEN_FULL. otherwise it goes
 * to the remaining garden, however many that holds */
#define GARDEN_CROWDED 9
#define GARDEN_FULL 8

#define ANIMAL_ICONS 10
/* where the row draws its icons. ICON_RIGHT is the x of the oldest, which
 * stands rightmost. ICON_Y is every icon's y, and ICON_GAP the gap leftward
 * to each newer one */
#define ICON_RIGHT 603.0F
#define ICON_Y 410.0F
#define ICON_GAP 35.0F
/* the sprite's scale for an icon at a scale of 1 */
#define ICON_SPRITE_SCALE 0.8F
/* an icon's scale as it arrives, which shrinks to 1 as the row shows */
#define ICON_NEW_SCALE 2.5F
/* the counts the row's timer must pass before the row slides out, before a
 * garden's first release, and before each next */
#define ROW_STAY 150
#define RELEASE_DELAY 60
#define RELEASE_GAP 10

struct animal_icon {
    Sint8 kind; /* -1 for none */
    Float scale;
    Float unk_08;
};

/* the row's task_work, as the row's steps read it */
struct icon_row_work {
    Sint8 mode;
    Uint8 unk_01[2];
    Uint8 released; /* icons released, which nothing reads */
    Uint8 unk_04[2];
    Uint16 timer;
    Uint8 unk_08[12];
    Angle slide; /* 0 with the row out, up to a quarter turn with it all in */
    Uint8 unk_18[8];
    Float x; /* how far the row is from fully in */
    Float width; /* how far it slides */
    Float offset;
};

/* the row's work->mode while icon_row_step runs the row */
enum icon_row_mode { ROW_WAIT, ROW_SLIDE_IN, ROW_SLIDE_OUT };
/* the row's work->mode while icon_row_release runs it */
enum release_mode { RELEASE_WAIT, RELEASE_GO, RELEASE_END };

enum lists_mode { LISTS_START, LISTS_KEEP };

static Bool chao_recount_all(void);
static void icon_row_step(struct task *task);
static void icon_row_release(struct task *task);
static void icon_row_destroy(struct task *task);

static int dat_8c1ab7e8 = 15;
static Sint8 dat_8c1ab7ec = 0;

static NJS_TEXNAME extra_textures[ANIMAL_KINDS] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0},
};
NJS_TEXLIST extra_texlist = {
#include "chao/chao/extra_texlist.inc"
};

static NJS_TEXANIM icon_texanims[ANIMAL_KINDS] = {
#include "chao/chao/icon_texanims.inc"
};

static NJS_SPRITE icon_sprite = {
#include "chao/chao/icon_sprite.inc"
};

/* each garden's count of Chao, and a slot past the last garden.
 * chao_gardens_init clears that slot with the counts, and nothing else
 * uses it */
static Uint8 garden_counts[GARDEN_COUNT + 1];
static struct chao_node *garden_lists[GARDEN_COUNT];
static Uint8 dat_8c6c0e80[8];
/* set once chao_clear_records has emptied the records. at boot they are
 * zeroed, and a zeroed record's type is CHAO_CHILD, not CHAO_NOTHING */
static Uint8 records_cleared;
static Sint8 carried_to;
static struct chao *carried;
static Uint8 dat_8c6c0e90[0x180];
static Uint8 chao_file_data[CHAO_FILE_DATA_SIZE];
static struct chao chao_records[CHAO_COUNT];
static Uint8 dat_8c6c4590[10];
static Uint8 icon_count;
static Bool icon_row_request;
static struct animal_icon animal_icons[ANIMAL_ICONS];
/* nothing reads these: chao_reset sets the byte to 0 and the array to -1 */
static Uint8 dat_8c6c4618;
static Sint8 dat_8c6c4619[ANIMAL_ICONS];

struct chao *chao_get_records(void)
{
    return chao_records;
}

Uint8 *chao_get_file_data(void)
{
    return chao_file_data;
}

Sint8 fun_8c02616c(void)
{
    return dat_8c1ab7ec;
}

void chao_set_carried(struct chao *chao)
{
    carried = chao;
}

struct chao *chao_get_carried(void)
{
    return carried;
}

void chao_set_carried_to(Sint8 garden)
{
    carried_to = garden;
}

void chao_clear_carried(void)
{
    carried_to = -1;
    carried = NULL;
}

Sint8 chao_get_carried_to(void)
{
    return carried_to;
}

struct chao_node *chao_get_list(int garden)
{
    if (garden < 0 || garden > GARDEN_COUNT - 1) {
        return NULL;
    }
    return garden_lists[garden];
}

Uint8 chao_count(int garden)
{
    if (garden < 0 || garden > GARDEN_COUNT - 1) {
        return 0;
    }
    chao_recount_all();
    return garden_counts[garden];
}

void chao_set_id(struct chao *chao, int number)
{
    SYS_RTC_DATE date;
    int id;

    syRtcGetDate(&date);
    id = 0;
    id += date.month * (31 * 24 * 60 * 60);
    id += date.day * (24 * 60 * 60);
    id += date.hour * (60 * 60);
    id += date.minute * 60;
    id += date.second;
    id *= 100;
    id += number;
    id += (Uint32)(rand() * RAND_SCALE * 2.99F) * 30;
    id += (Uint32)(rand() * RAND_SCALE * 1.99F) * 5;
    chao->id = id;
}

void chao_init_egg(struct chao *chao)
{
    int i;
    Uint32 skill;
    char *points;
    Sint8 *part;

    for (i = 0; i < CHAO_NAME_SIZE; i++) {
        chao->name[i] = 0;
    }
    chao->type = CHAO_EGG;
    chao->h_pos = 0.0F;
    chao->v_pos = 0.0F;
    chao->growth = 0.0F;
    chao->age = 0;
    chao->user_like = 0;
    chao->minimal = 0;
    chao->pos.x = rand() * RAND_SCALE * 100.0F;
    chao->pos.y = 20.0F;
    chao->pos.z = rand() * RAND_SCALE * 100.0F;
    part = chao->parts;
    for (i = 0; i < CHAO_PARTS; i++, part++) {
        *part = -1;
    }
    chao->hp = (int)(rand() * RAND_SCALE * 1000.0F) + 1000;
    chao->hp_max = chao->hp;
    chao->life = (int)(rand() * RAND_SCALE * 80.0F) + 100;
    chao->old = 0;
    chao->skills.swim = 0;
    chao->skills.fly = 0;
    chao->skills.run = 0;
    chao->skills.power = 0;
    chao->points.swim = 0;
    chao->points.fly = 0;
    chao->points.run = 0;
    chao->points.power = 0;
    for (i = 0; i < SKILL_DRAWS; i++) {
        skill = rand() * RAND_SCALE * 3.99F;
        /* @hack spelling: the skill's points by number, through a byte
         * cursor over the four; as a switch on the skill, the function
         * runs 64 bytes short and 272 of 287 words differ */
        points = (char *)&chao->points;
        *(Uint16 *)(points + (skill * sizeof(Uint16))) += POINTS_PER_DRAW;
    }
    chao->kind = 100 - (int)(rand() * RAND_SCALE * 200.99F);
    chao->agres = 100 - (int)(rand() * RAND_SCALE * 200.99F);
    chao->curio = 100 - (int)(rand() * RAND_SCALE * 200.99F);
    chao->charm = (int)(rand() * RAND_SCALE * 20.0F) + 90;
    chao->breed = (int)(rand() * RAND_SCALE * 20.0F);
    chao->sleep = (int)(rand() * RAND_SCALE * 200.0F);
    chao->tedious = (int)(rand() * RAND_SCALE * 200.0F);
    chao->hunger = (int)(rand() * RAND_SCALE * 200.0F);
    chao->unk_65 = 0;
    chao->unk_64 = 0;
    chao->unk_66 = 0;
    chao->pleasure = 0;
    chao->anger = 0;
    chao->sorrow = 0;
    chao->fear = 0;
    chao->unk_6b = 0;
    chao->user_like = 0;
    chao->mark_c = 'C';
    chao->mark_h = 'H';
    chao->mark_a = 'A';
    chao->mark_o = 'O';
    chao->like_changes = 0;
    chao->unk_17a = 1;
    chao->jewel = 0;
    chao->generation = 0;
    chao->flags &= ~CHAO_DELETE_EXEC;
}

void chao_reborn(struct chao *chao)
{
    int i;
    Uint32 skill;
    char *points;
    Sint8 *part;

    switch (chao->type) {
    case CHAO_SWIM:
        chao->h_pos = chao->h_pos * 0.1F;
        chao->v_pos = -(rand() * RAND_SCALE * 0.2F) - 0.4F;
        break;
    case CHAO_FLY:
        chao->h_pos = chao->h_pos * 0.1F;
        chao->v_pos = (rand() * RAND_SCALE * 0.2F) + 0.4F;
        break;
    case CHAO_RUN:
        chao->h_pos = (rand() * RAND_SCALE * 0.2F) + 0.4F;
        chao->v_pos = chao->v_pos * 0.1F;
        break;
    case CHAO_POWER:
        chao->h_pos = -(rand() * RAND_SCALE * 0.2F) - 0.4F;
        chao->v_pos = chao->v_pos * 0.1F;
        break;
    default:
        chao->h_pos = chao->h_pos * 0.1F;
        chao->v_pos = chao->v_pos * 0.1F;
        break;
    }
    chao->type = CHAO_EGG;
    chao->growth = (chao->growth * 0.3F) + 0.1F;
    chao->age = 0;
    chao->minimal = 0;
    part = chao->parts;
    for (i = 0; i < CHAO_PARTS; i++, part++) {
        if (rand() * RAND_SCALE < 0.1F) {
            *part = -1;
        } else {
            /* @hack redundant: an empty else, which keeps the parts' bound of 3
             * in R3 (MOV #3,R3 at 0x8c026722); without it, R2, and 2 of 428
             * words differ */
        }
    }
    chao->hp = ((chao->hp_max - 1000) *
                (Uint16)((rand() * RAND_SCALE * 5.0F) + 2.0F) / 10) +
               1000;
    chao->hp_max = chao->hp;
    chao->life = (int)(rand() * RAND_SCALE * 80.0F) + 100;
    chao->old = 0;
    chao->points.swim = chao->skills.swim;
    chao->points.fly = chao->skills.fly;
    chao->points.run = chao->skills.run;
    chao->points.power = chao->skills.power;
    chao->skills.swim = chao->skills.swim / 10;
    chao->skills.fly = chao->skills.fly / 10;
    chao->skills.run = chao->skills.run / 10;
    chao->skills.power = chao->skills.power / 10;
    for (i = 0; i < SKILL_DRAWS; i++) {
        skill = rand() * RAND_SCALE * 3.99F;
        /* @hack spelling: the skill's points by number, through a byte
         * cursor over the four; as a switch on the skill, the function
         * runs 32 bytes short and 162 of 428 words differ */
        points = (char *)&chao->points;
        *(Uint16 *)(points + (skill * sizeof(Uint16))) += POINTS_PER_DRAW;
    }
    if (chao->points.swim > POINTS_MAX) {
        chao->points.swim = POINTS_MAX;
    }
    if (chao->points.fly > POINTS_MAX) {
        chao->points.fly = POINTS_MAX;
    }
    if (chao->points.run > POINTS_MAX) {
        chao->points.run = POINTS_MAX;
    }
    if (chao->points.power > POINTS_MAX) {
        chao->points.power = POINTS_MAX;
    }
    chao->charm = (int)(rand() * RAND_SCALE * 20.0F) + 90;
    chao->breed = (int)(rand() * RAND_SCALE * 20.0F);
    chao->sleep = (int)(rand() * RAND_SCALE * 200.0F);
    chao->tedious = (int)(rand() * RAND_SCALE * 200.0F);
    chao->hunger = (int)(rand() * RAND_SCALE * 200.0F);
    chao->unk_65 = 0;
    chao->unk_64 = 0;
    chao->unk_66 = 0;
    chao->pleasure = 0;
    chao->anger = 0;
    chao->sorrow = 0;
    chao->fear = 0;
    chao->unk_6b = 0;
    chao->mark_c = 'C';
    chao->mark_h = 'H';
    chao->mark_a = 'A';
    chao->mark_o = 'O';
    chao->unk_17a = 1;
    chao->generation++;
    if (chao->generation > GENERATION_MAX) {
        chao->generation = GENERATION_MAX;
    }
}

void chao_breed(struct chao *first, struct chao *second, struct chao *child)
{
    int i;
    Float pick;
    Uint32 skill;
    char *points;
    Sint8 *from_first;
    Sint8 *from_second;
    Sint8 *part;
    Uint16 best;

    for (i = 0; i < CHAO_NAME_SIZE; i++) {
        child->name[i] = 0;
    }
    child->type = CHAO_EGG;
    if (rand() * RAND_SCALE > 0.5F) {
        child->h_pos = first->h_pos * 0.2F;
    } else {
        child->h_pos = second->h_pos * 0.2F;
    }
    if (rand() * RAND_SCALE > 0.5F) {
        child->v_pos = first->v_pos * 0.2F;
    } else {
        child->v_pos = second->v_pos * 0.2F;
    }
    for (i = 0; i < CHAO_FRUIT; i++) {
        child->fruit[i] = 0;
    }
    child->growth = ((first->growth + second->growth) * 0.15F) + 0.1F;
    child->like_changes = (first->like_changes + second->like_changes) / 2;
    child->age = 0;
    child->user_like = 0;
    child->minimal = 0;
    child->race_wins = 0;
    part = child->parts;
    from_first = first->parts;
    from_second = second->parts;
    /* @bug logic: every part of the child copies one parent's first part,
     * since the loop steps part alone. it reads the parents' parts through
     * the pointers at @R15 and @(4,R15) (0x8c026aee, 0x8c026af6), which it
     * never moves, while part steps (ADD #1,R9 at 0x8c026b00) */
    for (i = 0; i < CHAO_PARTS; i++, part++) {
        if (rand() * RAND_SCALE < 0.5F) {
            *part = *from_first;
        } else {
            *part = *from_second;
        }
    }
    /* @bug logic: the average takes first's hp_max twice, and second's not
     * at all: MOV.W @(22,R11),R0 at 0x8c026bbc reads first's, and ADD R0,R1
     * at 0x8c026bc8 adds it to itself */
    child->hp = ((((first->hp_max + first->hp_max) / 2) - 1000) *
                 (Uint16)((rand() * RAND_SCALE * 5.0F) + 2.0F) / 10) +
                1000;
    child->hp_max = child->hp;
    child->life = (int)(rand() * RAND_SCALE * 80.0F) + 100;
    child->old = 0;
    if (first->skills.swim > second->skills.swim) {
        best = first->skills.swim;
    } else {
        best = second->skills.swim;
    }
    child->points.swim = best;
    if (first->skills.fly > second->skills.fly) {
        best = first->skills.fly;
    } else {
        best = second->skills.fly;
    }
    child->points.fly = best;
    if (first->skills.run > second->skills.run) {
        best = first->skills.run;
    } else {
        best = second->skills.run;
    }
    child->points.run = best;
    if (first->skills.power > second->skills.power) {
        best = first->skills.power;
    } else {
        best = second->skills.power;
    }
    child->points.power = best;
    child->skills.swim = child->points.swim / 10;
    child->skills.fly = child->points.fly / 10;
    child->skills.run = child->points.run / 10;
    child->skills.power = child->points.power / 10;
    for (i = 0; i < SKILL_DRAWS; i++) {
        skill = rand() * RAND_SCALE * 3.99F;
        /* @hack spelling: the skill's points by number, through a byte
         * cursor over the four; as a switch on the skill, the function
         * runs 32 bytes short and 343 of 678 words differ */
        points = (char *)&child->points;
        *(Uint16 *)(points + (skill * sizeof(Uint16))) += POINTS_PER_DRAW;
    }
    if (child->points.swim > POINTS_MAX) {
        child->points.swim = POINTS_MAX;
    }
    if (child->points.fly > POINTS_MAX) {
        child->points.fly = POINTS_MAX;
    }
    if (child->points.run > POINTS_MAX) {
        child->points.run = POINTS_MAX;
    }
    if (child->points.power > POINTS_MAX) {
        child->points.power = POINTS_MAX;
    }
    if (rand() * RAND_SCALE < 0.7F) {
        if (rand() * RAND_SCALE < 0.5F) {
            child->kind = first->kind;
        } else {
            child->kind = second->kind;
        }
        if (rand() * RAND_SCALE < 0.5F) {
            child->agres = first->agres;
        } else {
            child->agres = second->agres;
        }
        if (rand() * RAND_SCALE < 0.5F) {
            child->curio = first->curio;
        } else {
            child->curio = second->curio;
        }
    } else {
        child->kind = 100 - (int)(rand() * RAND_SCALE * 200.99F);
        child->agres = 100 - (int)(rand() * RAND_SCALE * 200.99F);
        child->curio = 100 - (int)(rand() * RAND_SCALE * 200.99F);
    }
    pick = rand() * RAND_SCALE;
    if (pick < 0.2F) {
        child->charm = first->charm;
    } else if (pick < 0.4F) {
        child->charm = second->charm;
    } else if (pick < 0.7F) {
        child->charm = (first->charm + second->charm) / 2;
    } else {
        rand(); /* a number drawn and dropped: charm keeps what it held */
    }
    child->breed = (int)(rand() * RAND_SCALE * 20.0F);
    child->sleep = (int)(rand() * RAND_SCALE * 200.0F);
    child->tedious = (int)(rand() * RAND_SCALE * 200.0F);
    child->hunger = (int)(rand() * RAND_SCALE * 200.0F);
    child->unk_65 = 0;
    child->unk_64 = 0;
    child->unk_66 = 0;
    child->pleasure = 0;
    child->anger = 0;
    child->sorrow = 0;
    child->fear = 0;
    child->unk_6b = 0;
    child->mark_c = 'C';
    child->mark_h = 'H';
    child->mark_a = 'A';
    child->mark_o = 'O';
    child->unk_17a = 1;
    child->generation = 0;
    child->flags = first->flags | second->flags;
    if (first->flags & CHAO_JEWEL) {
        if (second->flags & CHAO_JEWEL) {
            if (rand() * RAND_SCALE > 0.5F) {
                child->jewel = first->jewel;
            } else {
                child->jewel = second->jewel;
            }
        } else {
            child->jewel = first->jewel;
        }
    } else if (second->flags & CHAO_JEWEL) {
        child->jewel = second->jewel;
    }
    if (rand() * RAND_SCALE > 0.5F) {
        child->flags &= ~CHAO_JEWEL;
    }
    if (rand() * RAND_SCALE > 0.5F) {
        child->flags &= ~CHAO_GOLD;
    }
    if (rand() * RAND_SCALE > 0.5F) {
        child->flags &= ~CHAO_PAL;
    }
    if (rand() * RAND_SCALE > 0.5F) {
        child->flags &= ~CHAO_BEGG;
    }
}

struct chao *chao_find_empty(void)
{
    struct chao *chao = chao_records;
    struct chao *end = chao + CHAO_COUNT;
    struct chao *found = NULL;

    for (; chao < end; chao++) {
        if (chao->type == CHAO_NOTHING) {
            found = chao;
            break;
        }
    }
    return found;
}

/* make an egg and return it. with first and second, it is their child, in
 * an empty record. with first alone, it is first, reborn. with no first, it
 * is a new egg in an empty record, whatever second is. NULL when it needs an
 * empty record and finds none */
static struct chao *chao_make(struct chao *first, struct chao *second)
{
    struct chao *child;

    if (first != NULL && second != NULL) {
        child = chao_find_empty();
        if (child != NULL) {
            chao_breed(first, second, child);
            return child;
        }
    } else if (first != NULL) {
        chao_reborn(first);
        return first;
    } else {
        child = chao_find_empty();
        if (child != NULL) {
            chao_init_egg(child);
            return child;
        }
    }
    return NULL;
}

void chao_list_add(int garden, struct chao *chao)
{
    struct chao_node *node = heap_alloc(sizeof(struct chao_node));
    struct chao_node *head = garden_lists[garden];

    node->chao = chao;
    node->next = head;
    garden_lists[garden] = node;
}

/* send a Chao away from the current stage's garden when it is crowded, as
 * GARDEN_CROWDED says. a stage that isn't a garden counts as Station Square.
 * the Chao is one of the first six on the garden's list, at random. only its
 * record's garden changes: it stays on its list, and the counts wait for a
 * recount */
static void chao_send_away(void)
{
    struct chao_node *node;
    int garden;
    int pick;
    Uint16 stage;

    stage = (Uint8)(stage_get_stage_act() >> 8);
    switch (stage) {
    case STAGE_ALIFE_SS:
        garden = GARDEN_STATION_SQUARE;
        break;
    case STAGE_ALIFE_EG:
        garden = GARDEN_EGG_CARRIER;
        break;
    case STAGE_ALIFE_MR:
        garden = GARDEN_MYSTIC_RUINS;
        break;
    default:
        garden = GARDEN_STATION_SQUARE;
        break;
    }
    if (garden_counts[garden] < GARDEN_CROWDED) {
        return;
    }
    node = garden_lists[garden];
    pick = rand() * RAND_SCALE * 6.0F;
    for (; pick > 0; pick--) {
        if (node == NULL) {
            return;
        }
        node = node->next;
    }
    switch (garden) {
    case GARDEN_STATION_SQUARE:
        if (garden_counts[GARDEN_EGG_CARRIER] < GARDEN_FULL) {
            node->chao->garden = GARDEN_EGG_CARRIER;
        } else {
            node->chao->garden = GARDEN_MYSTIC_RUINS;
        }
        break;
    case GARDEN_EGG_CARRIER:
        if (garden_counts[GARDEN_MYSTIC_RUINS] < GARDEN_FULL) {
            node->chao->garden = GARDEN_MYSTIC_RUINS;
        } else {
            node->chao->garden = GARDEN_STATION_SQUARE;
        }
        break;
    case GARDEN_MYSTIC_RUINS:
        if (garden_counts[GARDEN_STATION_SQUARE] < GARDEN_FULL) {
            node->chao->garden = GARDEN_STATION_SQUARE;
        } else {
            node->chao->garden = GARDEN_EGG_CARRIER;
        }
        break;
    }
}

static void chao_number(void)
{
    struct chao_node *node;
    int garden;
    int number = 0;

    for (garden = 0; garden < GARDEN_COUNT; garden++) {
        for (node = garden_lists[garden]; node != NULL; node = node->next) {
            node->number = number++;
        }
    }
}

/* list each record's Chao in its record's garden, which must be 0 to
 * GARDEN_COUNT - 1: nothing checks it */
static void chao_list_all(void)
{
    struct chao *chao = chao_records;
    struct chao *end = chao + CHAO_COUNT;

    for (; chao < end; chao++) {
        if (chao->type != CHAO_NOTHING) {
            chao_list_add(chao->garden, chao);
        }
    }
}

static void chao_make_eggs(void)
{
    struct chao *chao;
    int garden;
    int i;
    Sint8 number = 0;

    for (garden = 0; garden < GARDEN_COUNT; garden++) {
        for (i = 0; i < NEW_EGGS; i++) {
            chao = chao_make(NULL, NULL);
            if (chao != NULL) {
                chao->garden = garden;
                chao_set_id(chao, number);
                chao_list_add(garden, chao);
                number++;
            }
        }
    }
    chao_number();
}

static void chao_clear_records(void)
{
    struct chao *records = chao_records;
    int i;

    records_cleared = 1;
    memset(chao_records, 0, sizeof(chao_records));
    /* @bug logic: the clear sized for chao_file_data is given
     * chao_records (MOV R14,R4 in the JSR's slot at 0x8c027382, with 0x180
     * from 0x8c02737e), so it clears the records' first 0x180 bytes again,
     * and chao_file_data keeps what it held */
    memset(chao_records, 0, sizeof(chao_file_data));
    for (i = 0; i < CHAO_COUNT; i++) {
        /* @hack redundant: the store made twice, which keeps the record's
         * stride in R7 and the 0 in R6 (0x8c027384, 0x8c02738a); stored once,
         * the two trade and 6 of 34 words differ */
        records[i].type = CHAO_NOTHING;
        records[i].type = CHAO_NOTHING;
    }
}

/* recount each garden's Chao from the records. TRUE when any record holds a
 * Chao, whatever its garden */
static Bool chao_recount_all(void)
{
    Bool found = FALSE;
    Uint8 garden;
    int i;

    garden_counts[GARDEN_STATION_SQUARE] = 0;
    garden_counts[GARDEN_EGG_CARRIER] = 0;
    garden_counts[GARDEN_MYSTIC_RUINS] = 0;
    for (i = 0; i < CHAO_COUNT; i++) {
        if (chao_records[i].type != CHAO_NOTHING) {
            garden = chao_records[i].garden;
            if (garden >= 0 && garden <= GARDEN_COUNT - 1) {
                garden_counts[garden]++;
            }
            found = TRUE;
        }
    }
    return found;
}

void chao_list_remove(struct chao *chao)
{
    struct chao_node *node;
    struct chao_node *prev;
    /* @hack spelling: garden_counts through a local, whose address the image
     * loads after the call into R7 (0x8c02752e); by name, R5, and 11 of 45
     * words differ */
    Uint8 *counts;

    node = chao_get_list(chao->garden);
    counts = garden_counts;
    for (prev = NULL; node != NULL; prev = node, node = node->next) {
        if (node->chao == chao) {
            /* @bug logic: a node at the head isn't unlinked, and every node
             * after it drops out instead. with no prev, the test skips the
             * unlink (TST R6,R6 and BT at 0x8c027540, 0x8c027542), and
             * nothing moves garden_lists' head off the node. the store below
             * then clears the head's next (MOV.L R5,@(8,R4) at 0x8c027548) */
            if (prev != NULL) {
                prev->next = node->next;
            }
            node->next = NULL;
            counts[chao->garden]--;
            /* @bug logic: the count is a byte, so CMP/PZ at
             * 0x8c02755c always holds and BT at 0x8c02755e skips the clamp's
             * store: a count of 0 drops to 255 */
            if (counts[chao->garden] < 0) {
                counts[chao->garden] = 0;
            }
            return;
        }
    }
}

Bool chao_move(struct chao *chao, int garden)
{
    struct chao_node *from = chao_get_list(chao->garden);
    struct chao_node *node;
    struct chao_node *prev;

    /* garden's list is fetched and dropped: the node never joins it */
    chao_get_list(garden);
    for (node = from, prev = NULL; node != NULL;
         prev = node, node = node->next) {
        if (node->chao == chao) {
            if (prev != NULL) {
                prev->next = node->next;
                node->next = NULL;
            }
            break;
        }
    }
    if (node == NULL) {
        return FALSE;
    }
    chao->garden = garden;
    return TRUE;
}

/* free every listed node */
static void chao_lists_free(struct task *task)
{
    struct chao_node *node;
    int garden;

    for (garden = 0; garden < GARDEN_COUNT; garden++) {
        /* @bug memory: the loop reads each node's next after freeing the
         * node: heap_free's JSR at 0x8c0275f0 takes the node, and MOV.L
         * @(8,R14),R14 at 0x8c0275f4 then reads its next */
        for (node = chao_get_list(garden); node != NULL; node = node->next) {
            heap_free(node);
        }
    }
}

/* the step of the task that owns the lists. its first run makes
 * chao_lists_free the task's destructor: destroyed before that, the task
 * frees no node */
static void chao_lists_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case LISTS_START:
        task->destructor = chao_lists_free;
        work->mode++;
        break;
    }
}

void chao_gardens_init(void)
{
    int i;

    for (i = 0; i < GARDEN_COUNT + 1; i++) {
        garden_counts[i] = 0;
    }
    for (i = 0; i < GARDEN_COUNT; i++) {
        garden_lists[i] = NULL;
    }
    if (!records_cleared) {
        chao_clear_records();
    }
    if (chao_recount_all()) {
        chao_list_all();
    } else if (alife_load()) {
        if (chao_recount_all()) {
            chao_list_all();
        } else {
            chao_make_eggs();
        }
    } else {
        chao_make_eggs();
    }
    chao_send_away();
    task_create(TASK_WORK, 2, chao_lists_step);
}

void chao_reset(void)
{
    int i;

    for (i = 0; i < ANIMAL_ICONS; i++) {
        animal_icons[i].kind = -1;
        animal_icons[i].scale = 1.0F;
    }
    for (i = 0; i < ANIMAL_ICONS; i++) {
        dat_8c6c4619[i] = -1;
    }
    icon_count = 0;
    dat_8c6c4618 = 0;
    icon_row_request = FALSE;
    chao_clear_records();
    chao_clear_carried();
}

void animal_icon_add(Sint8 kind)
{
    struct animal_icon *icon;
    struct animal_icon *end;

    if (icon_count < ANIMAL_ICONS) {
        animal_icons[icon_count].kind = kind;
        animal_icons[icon_count].scale = ICON_NEW_SCALE;
        icon_count++;
    } else {
        end = &animal_icons[ANIMAL_ICONS - 1];
        for (icon = animal_icons; icon < end; icon++) {
            icon[0] = icon[1];
        }
        icon->kind = kind;
        icon->scale = ICON_NEW_SCALE;
        icon->unk_08 = 0.0F;
    }
    icon_row_request = TRUE;
}

static void icon_row_init(struct task *task)
{
    Uint16 stage = (Uint8)(stage_get_stage_act() >> 8);

    if (stage == STAGE_ALIFE_SS || stage == STAGE_ALIFE_EG ||
        stage == STAGE_ALIFE_MR) {
        task->step = icon_row_release;
    } else {
        task->step = icon_row_step;
    }
    task->destructor = icon_row_destroy;
}

/* the row's step outside a garden. when animal_icon_add asks, the row slides
 * in from the right, and its new icons shrink to a scale of 1. the row
 * slides out once its timer passes ROW_STAY. a request while it slides in
 * restarts the timer */
static void icon_row_step(struct task *task)
{
    struct icon_row_work *work = (struct icon_row_work *)task->work;
    Sint8 kind;
    Float scale;
    int i;

#ifdef GCC
    /* the port's 16:9 squeezes the row about the screen's right edge */
    port_wide_anchor = WIDE_RIGHT;
#endif
    switch (work->mode) {
    case ROW_WAIT:
        if (icon_row_request) {
            work->mode++;
            work->timer = 0;
            work->width = 380.0F;
            work->slide = 0;
            icon_row_request = FALSE;
            work->offset = 0.0F;
        }
        break;
    case ROW_SLIDE_IN:
        work->x = (1.0F - njSin(work->slide)) * work->width;
        for (i = 0; i < icon_count; i++) {
            kind = animal_icons[i].kind;
            scale = animal_icons[i].scale;
            animal_icons[i].scale -= 0.04F;
            if (animal_icons[i].scale < 1.0F) {
                animal_icons[i].scale = 1.0F;
            }
            if (hud_hidden >= 0 && kind != -1) {
                icon_sprite.p.x =
                    work->x + work->offset - (i * ICON_GAP) + ICON_RIGHT;
                icon_sprite.p.y = ICON_Y;
                icon_sprite.sx = scale * ICON_SPRITE_SCALE;
                icon_sprite.sy = scale * ICON_SPRITE_SCALE;
                njDrawSprite2D(&icon_sprite, kind, -1.0F, 0);
            }
        }
        if (work->slide < NJM_DEG_ANG(90)) {
            work->slide += 0x400;
        }
        if (work->timer++ > ROW_STAY) {
            work->mode++;
            work->timer = 0;
        } else if (icon_row_request) {
            icon_row_request = FALSE;
            work->timer = 0;
        }
        break;
    case ROW_SLIDE_OUT:
        work->x = (1.0F - njSin(work->slide)) * work->width;
        for (i = 0; i < icon_count; i++) {
            kind = animal_icons[i].kind;
            scale = animal_icons[i].scale;
            animal_icons[i].scale -= 0.02F;
            if (animal_icons[i].scale < 1.0F) {
                animal_icons[i].scale = 1.0F;
            }
            if (hud_hidden >= 0 && kind != -1) {
                icon_sprite.p.x = work->x - (i * ICON_GAP) + ICON_RIGHT;
                icon_sprite.p.y = ICON_Y;
                icon_sprite.sx = scale * ICON_SPRITE_SCALE;
                icon_sprite.sy = scale * ICON_SPRITE_SCALE;
                njDrawSprite2D(&icon_sprite, kind, -1.0F, 0);
            }
        }
        work->slide -= 0x200;
        if (work->slide == 0) {
            work->mode = ROW_WAIT;
            work->timer = 0;
        }
        break;
    }
#ifdef GCC
    port_wide_anchor = WIDE_MIDDLE;
#endif
}

/* the row's step in a garden. it releases the row's animals, newest first,
 * and then ends. the release starts once the timer passes RELEASE_DELAY,
 * and an animal goes each time the timer then passes RELEASE_GAP. while it
 * waits, it runs icon_row_step on the same work. RELEASE_WAIT's value is
 * ROW_WAIT's, so on a pending request, icon_row_step moves the mode on to
 * RELEASE_GO, which ends the wait early */
static void icon_row_release(struct task *task)
{
    struct icon_row_work *work = (struct icon_row_work *)task->work;
    NJS_POINT3 pos;
    NJS_VECTOR velocity;
    NJS_VECTOR out;
    Angle ang;
    Sint8 kind;
    Float scale;
    int i;

    switch (work->mode) {
    case RELEASE_WAIT:
        if (work->timer++ > RELEASE_DELAY) {
            work->mode++;
            work->timer = 0;
            work->released = 0;
        }
        icon_row_step(task);
        break;
    case RELEASE_GO:
        if (icon_count == 0) {
            work->mode++;
        } else if (work->timer++ > RELEASE_GAP) {
            work->timer = 0;
            work->released++;
            kind = animal_icons[icon_count - 1].kind;
            /* unless the icon is empty, its animal leaps from 5 above
             * player 0, a quarter turn off the camera's heading, give or take
             * 60 degrees */
            if (kind != -1) {
                pos = player_work[0]->pos;
                pos.y += 5.0F;
                velocity.x = 0.8F + ((rand() * RAND_SCALE) - 0.5F);
                velocity.y = 1.2F;
                velocity.z = 0.0F;
                ang =
                    ((rand() * RAND_SCALE) - 0.5F) * 120.0F * ANGLE_PER_DEGREE;
                njPushMatrix(&_unit_matrix_);
                TURN(njRotateY, camera_work->ang.y + ang + NJM_DEG_ANG(90));
                njCalcVector(NULL, &velocity, &out);
                njPopMatrix(1);
                alife_animal_create(
                    &pos, &out, rand() * RAND_SCALE * 360.0F * ANGLE_PER_DEGREE,
                    kind);
            }
            icon_count--;
        }
#ifdef GCC
        port_wide_anchor = WIDE_RIGHT;
#endif
        for (i = 0; i < icon_count; i++) {
            kind = animal_icons[i].kind;
            scale = animal_icons[i].scale;
            if (hud_hidden >= 0 && kind != -1) {
                icon_sprite.p.x = ICON_RIGHT - (i * ICON_GAP);
                icon_sprite.p.y = ICON_Y;
                icon_sprite.sx = scale * ICON_SPRITE_SCALE;
                icon_sprite.sy = scale * ICON_SPRITE_SCALE;
                njDrawSprite2D(&icon_sprite, kind, -1.0F, 0);
            }
        }
#ifdef GCC
        port_wide_anchor = WIDE_MIDDLE;
#endif
        break;
    case RELEASE_END:
        task_kill(task);
        break;
    }
}

static void icon_row_destroy(struct task *task)
{
}

void animal_icons_create(void)
{
    task_create(TASK_WORK, 2, icon_row_init);
}
