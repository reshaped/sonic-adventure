/* the level items, O LevelItem: the upgrades and Big's lures. each waits on
 * its stand in a field until its character takes it */
#ifndef OBJECT_LEVEL_ITEM_H
#define OBJECT_LEVEL_ITEM_H
#include <ninja.h>
#include <sg_xpt.h>

struct level_item;
struct task;

/* the items, by the number the set editor shows. an upgrade's number is
 * its bit in the player's upgrades. a lure's stand shows the lure that
 * level_item_pick counts as Big's next, whichever lure it was placed with */
enum level_item_kind {
    LEVEL_ITEM_CRYSTAL_RING = 0,
    LEVEL_ITEM_LIGHT_SHOES = 1,
    LEVEL_ITEM_ANCIENT_LIGHT = 2,
    LEVEL_ITEM_JET_ANKLET = 3,
    LEVEL_ITEM_RHYTHM_BROOCH = 4,
    LEVEL_ITEM_FIGHTING_GLOVES = 5,
    LEVEL_ITEM_SHOVEL_CLAWS = 6,
    LEVEL_ITEM_LONG_HAMMER = 7,
    LEVEL_ITEM_WARRIOR_FEATHER = 8,
    LEVEL_ITEM_LASER = 9,
    LEVEL_ITEM_BOOSTER = 10,
    LEVEL_ITEM_POWER_ROD = 11,
    LEVEL_ITEM_LIFE_BELT = 12,
    LEVEL_ITEM_LURE = 13, /* RUAR_NORMAL, then red, blue, silver and gold */
    LEVEL_ITEM_COUNT = LEVEL_ITEM_LURE + 5
};

/* the record of the stand stepping or drawing now. only level_item.c reads
 * it. it is extern for its place in bss: as a static, it would go after
 * big_lure_count */
extern struct level_item *level_item_current;
/* the lures Big has, as last counted. fishing_count_lures counts the four
 * fields' lure flags from 0. a lure's stand counts from 1 instead, adding
 * one for each of these flags set: the egg carrier's, counted twice, the
 * mystic ruins' and FLAG_BIG_LURE_5 */
extern int big_lure_count;
/* the ancient light's textures, which the mystic ruins load: its glow,
 * then its sparks */
extern NJS_TEXLIST ancient_light_texlist;

/* create a stand for item kind at pos. it uses task_create's result
 * unchecked, so with no task or heap room left it reads through null */
void level_item_create(int kind, NJS_POINT3 *pos);
/* the step of O LevelItem, which the fields place */
void level_item_step(struct task *task);

#endif
