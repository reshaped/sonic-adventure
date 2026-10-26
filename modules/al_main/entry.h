/* the a-life's entries: a task's sphere in one of four lists, by category.
 * each step, the a-life's task finds the entries each Chao touches */
#ifndef AL_MAIN_ENTRY_H
#define AL_MAIN_ENTRY_H
#include <ninja.h>
#include <sg_xpt.h>

#include "al_main/chao.h"
#include "system/task.h"

enum alife_category {
    ALIFE_CHAO, /* Chao and eggs */
    ALIFE_ANIMALS,
    ALIFE_PLACES, /* the garden's places and things */
    ALIFE_CALLS /* the calls of courting Chao, and nothing else */
};

/* the kinds of entry */
#define ALIFE_HATCHED 0 /* a Chao out of its egg */
#define ALIFE_EGG 1
#define ALIFE_ANIMAL 2
#define ALIFE_FRUIT 3
#define ALIFE_TREE 10
#define ALIFE_VMS 11 /* the VMS, the gardens' memory card station */
/* a courting Chao's call. an idle Chao that touches it, unless a child or a
 * Chaos Chao, makes for it in MODE_JOIN_COURT */
#define ALIFE_COURTING 32

/* the kinds of place the garden's markers enter */
#define ALIFE_GROUND 35
#define ALIFE_WATER 36
#define ALIFE_ROAD 37
#define ALIFE_FLY 38
#define ALIFE_CLIFF 39

/* the garden is full at this count of ALIFE_CHAO's entries, its Chao and eggs.
 * in a full garden, no idle Chao starts to court, and the VMS takes no Chao off
 * a card. alife_breed there takes no breed or life, and lays no egg */
#define ALIFE_CHAO_FULL 8

/* an entry's flags */
#define ENTRY_TOUCHING 0x2 /* it touches other entries this step */
#define ENTRY_HELD 0x4 /* a Chao holds it */
#define ENTRY_LET_GO 0x20 /* it's asked to let go */
#define ENTRY_HITTING 0x40 /* what it touches gets ENTRY_HIT */
#define ENTRY_HIT 0x80

/* the most touched entries an entry keeps in a step. it keeps the first it
 * finds */
#define ENTRY_HITS 4

/* an entry, at its task's extra, from the heap */
struct alife_entry {
    Uint16 category;
    Uint16 kind;
    Uint32 flags;
    /* two entries touch where their distance is below their radii's sum */
    Float radius;
    Sint8 hit_count;
    Uint8 unk_0d[3];
    struct alife_entry *hits[ENTRY_HITS]; /* those it touches this step */
    struct task *task;
    Uint32 unk_24;
    struct alife_entry *prev;
    struct alife_entry *next;
    struct alife_entry *held; /* what it holds, or what holds it */
};

/* give task a zeroed entry at its extra, first in category's list, and
 * return TRUE. when the heap has no room, set extra to NULL and return
 * FALSE. the game's callers leave the result unchecked */
Bool alife_entry_add(struct task *task, Uint16 category, Uint16 kind,
                     Float radius);
/* unlink entry from its category's list. entry may be NULL. the entry isn't
 * freed: it stays at its task's extra, and among the hits of the entries
 * that touched it this step */
void alife_entry_remove(struct alife_entry *entry);
/* the first entry of kind among those the Chao touches this step, or NULL.
 * the Chao's task must have an entry. work is unused */
struct alife_entry *alife_entry_touching(struct task_work *work,
                                         struct alife_chao *chao, int kind);

/* the head of each category's list, its newest entry, and each list's count
 * of entries */
struct alife_entry *alife_get_chao_list(void);
struct alife_entry *alife_get_animal_list(void);
struct alife_entry *alife_get_place_list(void);
struct alife_entry *alife_get_call_list(void);
int alife_get_chao_count(void);
int alife_get_animal_count(void);
int alife_get_place_count(void);
int alife_get_call_count(void);

#endif
