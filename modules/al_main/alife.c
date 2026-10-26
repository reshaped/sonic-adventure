/* the a-life's task, which each garden's stage task starts, and the four
 * lists of entries it tests each step for touching */
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/alife.h"
#include "al_main/chao.h"
#include "al_main/entry.h"
#include "al_main/physics.h"
#include "al_main/status.h"
#include "chao/chao.h"
#include "player/player.h"
#include "player/target.h"
#include "progress/progress.h"
#include "stage/stage.h"
#include "system/task.h"

Sint16 alife_fruit_count;
Sint32 alife_character;
Sint32 alife_garden;
Uint8 dat_0cb1e8cc[8];
Sint32 alife_chao_count;
Sint32 alife_animal_count;
Sint32 alife_place_count;
Sint32 alife_call_count;
struct alife_entry *alife_chao_list;
struct alife_entry *alife_animal_list;
struct alife_entry *alife_place_list;
struct alife_entry *alife_call_list;
Uint16 alife_mark_count;
/* 6 bytes no code reads or writes */
static Uint8 dat_0cb1e8f6[6];
/* 1 from the task's draw until its next step sets it to 0. nothing reads
 * it */
static Sint32 dat_0cb1e8fc;
/* 152 bytes no code reads or writes */
static Uint8 dat_0cb1e900[0x98];

/* where player 0 starts in a garden, and its heading */
struct garden_start {
    NJS_POINT3 pos;
    Angle ang_y;
};

/* each garden's starts, by stage_get_entrance's entrance.
 * garden_place_player takes an entrance outside 0 to 2 as 0, so it never
 * reads a garden's fourth start */
static struct garden_start station_square_starts[4];
static struct garden_start egg_carrier_starts[4];
static struct garden_start mystic_ruins_starts[4];

static void garden_place_player(void)
{
    struct task_work *work = player_work[0];
    /* NOLINTNEXTLINE(bugprone-signed-char-misuse) */
    int entrance = stage_get_entrance();
    struct garden_start *start;

    if (entrance > 2 || entrance < 0) {
        entrance = 0;
    }
    switch ((Uint8)(stage_get_stage_act() >> 8)) {
    case STAGE_ALIFE_SS:
        start = &station_square_starts[entrance];
        break;
    case STAGE_ALIFE_EG:
        start = &egg_carrier_starts[entrance];
        break;
    case STAGE_ALIFE_MR:
        start = &mystic_ruins_starts[entrance];
        break;
    default:
        start = &station_square_starts[entrance];
        break;
    }
    work->pos = start->pos;
    work->ang.x = 0;
    work->ang.y = start->ang_y;
    work->ang.z = 0;
}

/* set alife_character from player 0's character. Tikal and Eggman leave
 * it as alife_step's first step set it, 0 */
static void garden_set_character(void)
{
    int character = PLAYER_VARS(player_work[0])->character;

    switch (character) {
    case CHARACTER_SONIC:
        alife_character = 0;
        return;
    case CHARACTER_TAILS:
        alife_character = 1;
        return;
    case CHARACTER_KNUCKLES:
        alife_character = 2;
        return;
    case CHARACTER_AMY:
        alife_character = 3;
        return;
    case CHARACTER_E102:
        alife_character = 4;
        return;
    case CHARACTER_BIG:
        alife_character = 5;
        return;
    }
    /* @hack redundant: the return, which gives a character no case matches an
     * RTS of its own (0x0c910efa); without it, a BRA to the shared RTS takes
     * its place and 1 of 35 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

struct alife_entry *alife_get_chao_list(void)
{
    return alife_chao_list;
}

struct alife_entry *alife_get_animal_list(void)
{
    return alife_animal_list;
}

struct alife_entry *alife_get_place_list(void)
{
    return alife_place_list;
}

struct alife_entry *alife_get_call_list(void)
{
    return alife_call_list;
}

int alife_get_chao_count(void)
{
    return alife_chao_count;
}

int alife_get_animal_count(void)
{
    return alife_animal_count;
}

int alife_get_place_count(void)
{
    return alife_place_count;
}

int alife_get_call_count(void)
{
    return alife_call_count;
}

static void entries_clear(void)
{
    alife_chao_count = 0;
    alife_animal_count = 0;
    alife_place_count = 0;
    alife_call_count = 0;
    alife_chao_list = NULL;
    alife_animal_list = NULL;
    alife_place_list = NULL;
    alife_call_list = NULL;
}

/* link the entry at the head of its category's list, and count it */
static void entry_link(struct alife_entry *entry)
{
    Uint16 category = entry->category;

    switch (category) {
    case ALIFE_CHAO:
        entry->next = alife_chao_list;
        alife_chao_list = entry;
        entry->prev = NULL;
        if (entry->next) {
            entry->next->prev = entry;
        }
        alife_chao_count++;
        break;
    case ALIFE_ANIMALS:
        entry->next = alife_animal_list;
        alife_animal_list = entry;
        entry->prev = NULL;
        if (entry->next) {
            entry->next->prev = entry;
        }
        alife_animal_count++;
        break;
    case ALIFE_PLACES:
        entry->next = alife_place_list;
        alife_place_list = entry;
        entry->prev = NULL;
        if (entry->next) {
            entry->next->prev = entry;
        }
        alife_place_count++;
        break;
    case ALIFE_CALLS:
        entry->next = alife_call_list;
        alife_call_list = entry;
        entry->prev = NULL;
        if (entry->next) {
            entry->next->prev = entry;
        }
        alife_call_count++;
        break;
    }
    /* @hack redundant: the return, which gives a category no case matches an
     * RTS of its own (0x0c910ff6); without it, a BRA to the shared RTS takes
     * its place and 1 of 68 words differ */
    return; /* NOLINT(readability-redundant-control-flow) */
}

void alife_entry_remove(struct alife_entry *entry)
{
    if (entry) {
        Uint16 category = entry->category;
        struct alife_entry *prev = entry->prev;
        struct alife_entry *next = entry->next;

        switch (category) {
        case ALIFE_CHAO:
            if (prev) {
                prev->next = next;
            } else {
                alife_chao_list = next;
            }
            alife_chao_count--;
            break;
        case ALIFE_ANIMALS:
            if (prev) {
                prev->next = next;
            } else {
                alife_animal_list = next;
            }
            alife_animal_count--;
            break;
        case ALIFE_PLACES:
            if (prev) {
                prev->next = next;
            } else {
                alife_place_list = next;
            }
            alife_place_count--;
            break;
        case ALIFE_CALLS:
            if (prev) {
                prev->next = next;
            } else {
                alife_call_list = next;
            }
            alife_call_count--;
            break;
        }
        if (next) {
            next->prev = prev;
        }
    }
}

static void entry_clear_hits(struct alife_entry *entry)
{
    int i;

    entry->hit_count = 0;
    for (i = 0; i < ENTRY_HITS; i++) {
        entry->hits[i] = NULL;
    }
    entry->flags &= ~ENTRY_TOUCHING;
}

static void entries_clear_hits(void)
{
    struct alife_entry *entry;

    for (entry = alife_chao_list; entry; entry = entry->next) {
        entry_clear_hits(entry);
    }
    for (entry = alife_animal_list; entry; entry = entry->next) {
        entry_clear_hits(entry);
    }
    for (entry = alife_place_list; entry; entry = entry->next) {
        entry_clear_hits(entry);
    }
    for (entry = alife_call_list; entry; entry = entry->next) {
        entry_clear_hits(entry);
    }
}

Bool alife_entry_add(struct task *task, Uint16 category, Uint16 kind,
                     Float radius)
{
    struct alife_entry *entry = heap_calloc(1, sizeof(struct alife_entry));

    if (!entry) {
        task->extra = NULL;
        return FALSE;
    }
    entry->category = category;
    entry->kind = kind;
    entry->flags = 0;
    entry->radius = radius;
    entry_clear_hits(entry);
    entry->task = task;
    entry->unk_24 = 0;
    entry->prev = NULL;
    entry->next = NULL;
    task->extra = entry;
    entry_link(entry);
    return TRUE;
}

/* when the two entries' spheres overlap, mark both ENTRY_TOUCHING, and
 * list each in the other's hits while the other has room. an entry with
 * ENTRY_HITTING gives the other ENTRY_HIT */
static void entries_touch(struct alife_entry *entry, struct alife_entry *other)
{
    /* @hack spelling: entry's pos walked as Floats from its x; as a byte
     * cursor, 35 of 81 words differ */
    Float *pos = (Float *)&entry->task->work->pos;
    /* @hack spelling: other's pos walked the same way; as a byte cursor, the
     * function runs 4 bytes long and 72 of 81 words differ */
    Float *other_pos = (Float *)&other->task->work->pos;
    /* @hack spelling: the radii through locals, which load other's task
     * into R7 and entry's into R6 (0x0c911200, 0x0c911206); summed in
     * place, the two trade and 33 of 81 words differ */
    Float r1 = entry->radius;
    Float r2 = other->radius;
    Float radius = r1 + r2;
    Float radius_squared = radius * radius;
    Float dx;
    Float dy;
    Float dz;
    Float ax;
    Float ay;
    Float by;
    Float az;
    Float bz;
    Float distance;

    /* @hack spelling: y's and z's reads spelled ahead of their differences,
     * which interleave the reads with the subtractions (0x0c911212 to
     * 0x0c911228); as three differences, the function runs 2 bytes short
     * and 78 of 81 words differ */
    ax = *pos++;
    dx = ax - *other_pos++;
    ay = *pos++;
    by = *other_pos++;
    az = *pos;
    bz = *other_pos;
    dy = ay - by;
    dz = az - bz;
    distance = (dx * dx) + (dy * dy) + (dz * dz);
    if (distance < radius_squared) {
        Sint8 count = entry->hit_count;
        Sint8 other_count = other->hit_count;

        /* @hack spelling: the ors and increments below spelled out, which
         * load other's task ahead of entry's (0x0c911200, 0x0c911206); with
         * |= and ++, entry's comes first and 21 of 81 words differ */
        entry->flags = entry->flags | ENTRY_TOUCHING;
        other->flags = other->flags | ENTRY_TOUCHING;
        if (entry->flags & ENTRY_HITTING) {
            other->flags = other->flags | ENTRY_HIT;
        }
        if (other->flags & ENTRY_HITTING) {
            entry->flags = entry->flags | ENTRY_HIT;
        }
        if (count < ENTRY_HITS) {
            entry->hits[count] = other;
            entry->hit_count = entry->hit_count + 1;
        }
        if (other_count < ENTRY_HITS) {
            other->hits[other_count] = entry;
            other->hit_count = other->hit_count + 1;
        }
    }
}

/* test each Chao against each Chao after it in its list, and against every
 * entry of the other three lists. two entries that aren't Chao are never
 * tested against each other */
static void entries_find_touching(void)
{
    struct alife_entry *entry;
    struct alife_entry *other;

    for (entry = alife_chao_list; entry; entry = entry->next) {
        for (other = entry->next; other; other = other->next) {
            entries_touch(entry, other);
        }
        for (other = alife_animal_list; other; other = other->next) {
            entries_touch(entry, other);
        }
        for (other = alife_place_list; other; other = other->next) {
            entries_touch(entry, other);
        }
        for (other = alife_call_list; other; other = other->next) {
            entries_touch(entry, other);
        }
    }
}

static void entries_update(void)
{
    int i;

    /* @hack redundant: a loop of one pass, which keeps SHC from taking the
     * function into alife_step (BSR 0x0c911316 at 0x0c91159e); as two plain
     * calls, alife_step takes them in, alife.c's code runs 64 bytes short
     * and 8 of 8 words differ */
    for (i = 0; i < 1; i++) {
        entries_clear_hits();
        entries_find_touching();
    }
}

static void alife_destroy(struct task *task)
{
}

static void alife_draw(struct task *task)
{
    njFogDisable();
    dat_0cb1e8fc = 1;
}

/* the egg player 0 holds becomes an egg Chao of its color in the first
 * empty record. the egg is listed in the garden, and its task goes into
 * player 0's hands. an egg whose flag is set, or one with no empty record
 * to take, changes nothing */
static void garden_take_egg(void)
{
    struct chao *chao;
    struct task *task;
    int held = player_get_held_kind(0);

    switch (held) {
    case HELD_GOLD_EGG:
        if (!event_flag_check(FLAG_GOLD_EGG_IN)) {
            chao = chao_find_empty();
            if (chao) {
                chao->flags |= CHAO_GOLD;
                chao_init_egg(chao);
                task = alife_chao_create(chao);
                chao_list_add(alife_garden, chao);
                event_flag_set(FLAG_GOLD_EGG_IN);
                player_set_held_kind(0, 0);
                player_hand_task(0, task);
                chao->garden = alife_garden;
                chao_set_id(chao, 0);
            }
        }
        break;
    case HELD_SILVER_EGG:
        if (!event_flag_check(FLAG_SILVER_EGG_IN)) {
            chao = chao_find_empty();
            if (chao) {
                chao->flags |= CHAO_PAL;
                chao_init_egg(chao);
                task = alife_chao_create(chao);
                chao_list_add(alife_garden, chao);
                event_flag_set(FLAG_SILVER_EGG_IN);
                player_set_held_kind(0, 0);
                player_hand_task(0, task);
                chao->garden = alife_garden;
                chao_set_id(chao, 0);
            }
        }
        break;
    case HELD_BLACK_EGG:
        if (!event_flag_check(FLAG_BLACK_EGG_IN)) {
            chao = chao_find_empty();
            if (chao) {
                chao->flags |= CHAO_BEGG;
                chao_init_egg(chao);
                task = alife_chao_create(chao);
                chao_list_add(alife_garden, chao);
                event_flag_set(FLAG_BLACK_EGG_IN);
                player_set_held_kind(0, 0);
                player_hand_task(0, task);
                chao->garden = alife_garden;
                chao_set_id(chao, 0);
            }
        }
        break;
    }
}

/* alife_step's steps, by work->mode. ALIFE_START is the first, and
 * ALIFE_WAIT a wait of one step. ALIFE_PLACE places player 0, and ALIFE_RUN
 * is each step after it */
enum alife_mode { ALIFE_START, ALIFE_WAIT, ALIFE_PLACE, ALIFE_RUN };

static void alife_step(struct task *task)
{
    struct task_work *work = task->work;

    switch (work->mode) {
    case ALIFE_START:
        alife_character = 0;
        alife_fruit_count = 0;
        work->mode++;
        task->destructor = alife_destroy;
        alife_garden = alife_get_garden();
        task->draw = alife_draw;
        alife_mark_count = 0;
        break;
    case ALIFE_WAIT:
        work->mode++;
        break;
    case ALIFE_PLACE:
        garden_place_player();
        chao_clear_carried();
        garden_set_character();
        garden_take_egg();
        work->mode++;
        break;
    case ALIFE_RUN:
        entries_update();
        break;
    }
    dat_0cb1e8fc = 0;
    status_step();
}

void alife_create(void)
{
    entries_clear();
    task_create(TASK_WORK, 1, alife_step);
}

static struct garden_start station_square_starts[4] = {
    {{2.38F, 0.0F, 119.0F}, 0xc354},
    {{158.6F, 3.0F, 104.0F}, 0x81b8},
    {{0.0F, 0.0F, -136.0F}, 0x3f7d},
    {{0.0F, 0.0F, 0.0F}, 0},
};
static struct garden_start egg_carrier_starts[4] = {
    {{-272.3F, 4.5F, -134.0F}, 0x250},
    {{113.0F, 70.9F, -37.5F}, 0xe3a7},
    {{-272.3F, 4.5F, -134.0F}, 0xce3a},
    {{-272.3F, 4.5F, -134.0F}, 0xce3a},
};
static struct garden_start mystic_ruins_starts[4] = {
    {{74.8F, 12.1F, -159.6F}, 0x12bf},
    {{199.2F, 15.4F, -80.9F}, 0x5d75},
    {{74.8F, 12.1F, -159.6F}, 0x12bf},
    {{74.8F, 12.1F, -159.6F}, 0x12bf},
};

struct alife_entry *alife_entry_touching(struct task_work *work,
                                         struct alife_chao *chao, int kind)
{
    struct alife_entry *entry = chao->task->extra;
    struct alife_entry **hit;
    struct alife_entry **end;
    Sint8 count;

    if (entry->flags & ENTRY_TOUCHING) {
        count = entry->hit_count;
        end = entry->hits + count;
        for (hit = entry->hits; hit < end; hit++) {
            if ((*hit)->kind == kind) {
                return *hit;
            }
        }
    }
    return NULL;
}
