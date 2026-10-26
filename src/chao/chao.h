/* the Chao records and each garden's list of them, the carried Chao, and
 * the row of animals the player carries */
#ifndef CHAO_CHAO_H
#define CHAO_CHAO_H
#include <ninja.h>
#include <sg_xpt.h>

/* the Chao records, which the three gardens share. the a-life file,
 * SONICADV_ALF, saves the gardens on a memory card and keeps the first 24 */
#define CHAO_COUNT 26

/* the bytes the a-life file keeps ahead of its records */
#define CHAO_FILE_DATA_SIZE 0x180

enum garden {
    GARDEN_STATION_SQUARE,
    GARDEN_EGG_CARRIER,
    GARDEN_MYSTIC_RUINS,
    GARDEN_COUNT
};

/* a Chao's type. each but CHAO_PARTNER is named for the label the gardens'
 * debug status page prints for it */
enum chao_type {
    CHAO_CHILD,
    CHAO_STANDARD,
    CHAO_SWIM,
    CHAO_FLY,
    CHAO_RUN,
    CHAO_POWER,
    CHAO_CHAOS,
    CHAO_EGG,
    CHAO_NOTHING, /* an empty record */
    /* the type of a card's egg slot that holds a partner for the card's Chao.
     * card_drives_take_changes breeds the two into an egg in the slot */
    CHAO_PARTNER
};

/* the parts an animal can give a Chao, each named for its status page label */
enum chao_part {
    CHAO_PART_FHEAD,
    CHAO_PART_HORN,
    CHAO_PART_EAR,
    CHAO_PART_WING,
    CHAO_PART_HAND,
    CHAO_PART_FOOT,
    CHAO_PART_TAIL,
    CHAO_PARTS
};

#define CHAO_NAME_SIZE 8
#define CHAO_FRUIT 8

struct chao_skills {
    Uint16 swim;
    Uint16 fly;
    Uint16 run;
    Uint16 power;
};

/* the characters a Chao keeps a like for: Sonic, Tails, Knuckles, Amy,
 * E-102 and Big, in that order */
#define CHAO_PLAYERS 6

struct chao_like {
    Sint8 like; /* -100 to 100 */
    Uint8 raised; /* times like has risen, up to 200 */
};

/* the kinds of race a Chao keeps a best time and a win for */
#define CHAO_RACES 5

/* a race time: minutes, seconds, and steps of the race's clock, 60 to a
 * second */
struct race_time {
    Uint8 steps;
    Uint8 seconds;
    Uint8 minutes;
};

/* bits of struct chao's flags. a bred child keeps each of CHAO_BEGG,
 * CHAO_GOLD, CHAO_PAL and CHAO_JEWEL that a parent has, at even odds. the
 * player can carry each of the black, gold and silver eggs into a garden
 * once. there it becomes a record with its egg's bit set */
/* the Chao's growth is due: status.c sets it at each growth tick, and
 * alife_grow_step grows the Chao and clears it */
#define CHAO_GROWTH_DUE 0x01
/* the black egg's bit. its egg wears cg_begg */
#define CHAO_BEGG 0x02
/* the gold egg's bit. its egg wears cg_gold, and its Chao chao_gold */
#define CHAO_GOLD 0x04
/* the silver egg's bit. its egg and its Chao both wear pal */
#define CHAO_PAL 0x08
/* the VMS takes a Chao with this bit off a card by deleting the card's
 * executable file, rather than by clearing its slot */
#define CHAO_DELETE_EXEC 0x20
#define CHAO_JEWEL 0x40 /* a jewel Chao, coated in the texture jewel picks */

/* a Chao's record, as the memory card keeps it. most fields are named for
 * the status page's labels. mark_c, mark_h, mark_a and mark_o spell CHAO in
 * every egg chao.c makes */
struct chao {
    Uint8 type; /* a chao_type */
    Uint8 garden; /* an enum garden */
    Sint8 user_like; /* from -100 to 100 */
    Uint8 mark_c;
    Uint8 name[CHAO_NAME_SIZE];
    struct chao_skills skills; /* each skill's level */
    Uint16 hp;
    Uint16 hp_max;
    /* the fruit the VMS, the gardens' memory card station, drops as it takes
     * the Chao off a card: a kind for each, or 0 */
    Sint8 fruit[CHAO_FRUIT];
    Float h_pos; /* how it leans: toward run above 0, power below */
    Float v_pos; /* toward fly above 0, swim below */
    Float growth;
    Uint16 like_changes; /* times its likes were raised or lowered, to 9999 */
    Uint16 life;
    Uint16 old;
    Uint16 minimal;
    Uint8 race_wins; /* 1 << kind for each kind of race it has won */
    Uint8 mark_h;
    Uint16 flags;
    NJS_POINT3 pos; /* where it is in its garden */
    Sint32 age;
    Uint32 id;
    Sint8 parts[CHAO_PARTS]; /* the kind of animal each came from, or -1 */
    Uint8 mark_a;
    struct chao_skills points; /* each skill's points toward its level */
    /* kind, agres and curio: three scores, each from -100 to 100 */
    Sint8 kind;
    Sint8 agres;
    Sint8 curio;
    Uint8 charm;
    Uint8 breed;
    Uint8 sleep;
    Uint8 hunger;
    Uint8 tedious;
    Uint8 unk_64;
    Uint8 unk_65;
    Uint8 unk_66;
    Uint8 pleasure;
    Uint8 anger;
    Uint8 sorrow;
    Uint8 fear;
    Uint8 unk_6b;
    struct chao_like likes[CHAO_PLAYERS]; /* its like for each character */
    Uint8 unk_78[0x100];
    Uint8 generation;
    Uint8 mark_o;
    Uint8 unk_17a;
    Uint8 unk_17b;
    /* reincarnation cocoons entered, even when no egg follows */
    Uint16 reincarnations;
    /* its best time in each kind of race, all 0 for none */
    struct race_time race_times[CHAO_RACES];
    Uint8 unk_18d[5];
    /* with CHAO_JEWEL set, the jewel texture it wears: 0 to 3 for ame, eme,
     * rub and saf. the gardens read any other value as 0 */
    Uint8 jewel;
    Uint8 unk_193[0x6d];
};

struct chao_node {
    Uint8 unk_00;
    /* the node's place through every garden's list, from 0. only the new
     * eggs chao_gardens_init makes get one, and nothing reads it */
    Uint8 number;
    struct chao *chao;
    struct chao_node *next;
};

/* an icon for each kind of animal, from EXTRA, the texture file
 * texture_load_regular loads */
extern NJS_TEXLIST extra_texlist;

/* the CHAO_COUNT records, and the CHAO_FILE_DATA_SIZE bytes the a-life file
 * keeps ahead of them. both are static arrays: a caller may write them, but
 * must not free them. chao_reset empties the records. alife_load can
 * replace the bytes and the first 24 records */
struct chao *chao_get_records(void);
Uint8 *chao_get_file_data(void);
/* returns 0: the static byte it reads starts at 0, and nothing writes it */
Sint8 fun_8c02616c(void);
/* the Chao the player carries, kept by pointer, not copied: it must stay
 * valid until replaced or cleared. its destination is a garden, or -1 for
 * none. chao_clear_carried sets both to none */
void chao_set_carried(struct chao *chao);
struct chao *chao_get_carried(void);
void chao_set_carried_to(Sint8 garden);
void chao_clear_carried(void);
Sint8 chao_get_carried_to(void);
/* garden's list, or NULL when the list is empty or garden is out of range.
 * destroying the task chao_gardens_init makes frees the listed nodes, once
 * the task has run a step. chao_list_remove and chao_move unlink a node
 * without freeing it */
struct chao_node *chao_get_list(int garden);
/* recount every garden from the records, not the lists, and return garden's
 * count. returns 0, without a recount, for a garden out of range */
Uint8 chao_count(int garden);
/* set Chao's id from the clock. the id is the clock's date and time in
 * seconds, times 100. it counts a month as 31 days and leaves out the year.
 * then it adds number, a random 0, 30 or 60, and a random 0 or 5 */
void chao_set_id(struct chao *chao, int number);
/* make Chao a new egg with random stats. it clears CHAO_DELETE_EXEC. Chao
 * keeps its garden, id, fruit, likes, race wins and times, reincarnations
 * and other flags */
void chao_init_egg(struct chao *chao);
/* turn Chao into its next generation in place, an egg again. it keeps each
 * part nine times in ten, and a tenth of each skill's level */
void chao_reborn(struct chao *chao);
/* make child an egg taking after first and second. each of child's parts
 * copies one parent's first part, and its hp takes after first's hp_max
 * alone. child's garden, id, pos, likes, race times and reincarnations stay
 * as they were. so do its charm three times in ten, and its jewel when
 * neither parent has CHAO_JEWEL */
void chao_breed(struct chao *first, struct chao *second, struct chao *child);
/* the first CHAO_NOTHING record, or NULL. it isn't reserved */
struct chao *chao_find_empty(void);
/* put a node keeping Chao's pointer at the head of garden's list. garden
 * must be 0 to GARDEN_COUNT - 1, and the node's allocation is unchecked.
 * Chao->garden and the counts stay as they were */
void chao_list_add(int garden, struct chao *chao);
/* unlink Chao's node from its garden's list without freeing it, and take
 * one from the garden's count. a node at the head isn't unlinked: it stays
 * listed, and every node after it drops out instead. a count of 0 drops to
 * 255, since the clamp tests an unsigned byte. a Chao whose garden's list
 * lacks its node changes nothing */
void chao_list_remove(struct chao *chao);
/* move Chao to garden, which must be 0 to GARDEN_COUNT - 1: unlink its node
 * from its list without freeing it, and set Chao->garden. a node at the head
 * stays in the list. garden's list doesn't gain the node, and the counts
 * wait for a recount. TRUE when Chao's list holds its node, and FALSE,
 * changing nothing, when it doesn't */
Bool chao_move(struct chao *chao, int garden);
/* make each garden's list of its Chao. the Chao are the records' when any
 * record holds one, or else those alife_load loads, or else two new eggs in
 * each garden. then a crowded current garden sends one Chao away, and a new
 * task owns the lists. this drops the old lists without freeing them, so
 * destroy the old task first */
void chao_gardens_init(void);
/* empty the animal row, the records and the carried Chao. the lists keep
 * their nodes, and chao_get_file_data's bytes keep what they held */
void chao_reset(void);
/* add an animal of kind to the end of the row the player carries, and ask
 * the row to show. a full row of 10 drops its oldest. kind must be -1, for
 * none, or 0 to ANIMAL_KINDS - 1: drawing and release don't check it */
void animal_icon_add(Sint8 kind);
/* create the row's task: in a garden it releases the animals, and elsewhere
 * it shows them as they come */
void animal_icons_create(void);

#endif
