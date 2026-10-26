/* the item box, O ITEMBOX in the object lists. a player or an attack opens
 * it, and the item its face shows is given */
#ifndef OBJECT_ITEM_BOX_H
#define OBJECT_ITEM_BOX_H
#include <ninja.h>
#include <sg_xpt.h>

struct task;
struct task_work;

/* the kinds of item, which a box keeps in its scl.x. for a floating box,
 * the set editor shows each kind's name: the part after ITEM_, or the name
 * the kind's comment gives */
enum item_kind {
    ITEM_SPEED_UP, /* named HAYAASI */
    ITEM_INVINCIBLE, /* named MUTEKI */
    ITEM_5RING,
    ITEM_10RING,
    ITEM_RANDOMRING, /* 1, 5, 10, 15, 20 or 40 rings, at random */
    ITEM_BARRIER, /* named BARIA */
    ITEM_1UP,
    ITEM_EXPLOSION, /* named EKUSUPUROSYON */
    ITEM_MAGNETIC_BARRIER, /* named ITEM_TH_BARIA */
    ITEM_COUNT
};

/* how a kind of item shows, and how it is given */
struct item {
    Uint32 texture; /* the face's texture number in the OBJ_REGULAR texlist */
    /* give the item. work is the opened box's, whose collision's hit is
     * the touch that opened it */
    void (*give)(struct task_work *work);
};

/* each kind of item, by enum item_kind */
extern struct item item_kinds[ITEM_COUNT];
/* each character's extra-life face. a draw of an item's face first copies
 * the character's into item_kinds[ITEM_1UP] */
extern Uint32 item_1up_textures[];

/* make an item box of kind at pos, which grows in. with the task pool or
 * the heap out of room, it reads through a null task */
void item_box_create(NJS_POINT3 *pos, enum item_kind kind);
/* the object lists' step for O ITEMBOX, and the step of a box
 * item_box_create makes. the box's kind is its scl.x, which the step wraps
 * into enum item_kind's range */
void item_box_step(struct task *task);

#endif
