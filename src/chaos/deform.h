/* a Chaos's model melted down and out, and put back */
#ifndef CHAOS_DEFORM_H
#define CHAOS_DEFORM_H
#include <ninja.h>
#include <sg_xpt.h>

struct deform_point;
struct task_work;

/* what deform_step does, by the deformer's mode */
enum deform_mode {
    DEFORM_MAKE = 0, /* keep each model's own points, then restore */
    /* melt the points down and out, as far as the scale says */
    DEFORM_MELT = 1,
    /* set by fun_8c0fce40. deform_step has no case for it, so it leaves the
     * points as they are */
    DEFORM_HOLD = 3,
    DEFORM_RESTORE = 4, /* give each model its own points back, then idle */
    DEFORM_IDLE = 5,
    DEFORM_FREE = 6 /* free the deformer and all it kept */
};

/* a node of the deformer's tree, which deform_create lists depth first */
struct deform_node {
    Sint16 depth; /* how far below the tree's root it lies, unread */
    NJS_OBJECT *object;
    NJS_POINT3 *points; /* a copy of its model's own points */
    struct deform_point *posed; /* its model's points, as the pose put them */
    NJS_POINT3 *moved; /* the points deform_use_moved gives its model */
};

struct deform {
    Sint8 mode; /* an enum deform_mode */
    Sint16 count; /* how many nodes the array nodes holds */
    Float scale; /* how far it melts: 0 leaves the model as it was */
    struct deform_node *nodes;
    /* fun_8c0fce40 writes unk_0c, unk_10, unk_18, unk_1c and unk_24, and
     * nothing reads any of these */
    Float unk_0c;
    Float unk_10;
    Uint8 unk_14[4];
    Sint32 unk_18;
    Sint32 unk_1c;
    Uint8 unk_20[4];
    Sint32 unk_24;
    Uint8 unk_28[4];
};

/* step the deformer by its mode. DEFORM_MAKE and DEFORM_MELT take each
 * node's matrix from the last pose, unchecked, so pose the same tree first.
 * DEFORM_MELT reads chaos_params too. a failed allocation in DEFORM_MAKE
 * hangs */
void deform_step(struct deform *deform);
/* make a deformer for root's tree, and leave it idle. it keeps each model's
 * own points, and where the last pose put them. the tree must stay valid,
 * and its models writable, until deform_free. the tree may hold 120 nodes,
 * none more than 12 below root: nothing checks. a failed allocation hangs.
 * work is unused */
struct deform *deform_create(NJS_OBJECT *root, struct task_work *work);
/* free the deformer and its kept points now, leaving the models as they
 * are: step DEFORM_RESTORE first to put them back */
void deform_free(struct deform *deform);
/* put the deformer in DEFORM_HOLD, and set unk_ fields that nothing
 * reads */
void fun_8c0fce40(struct deform *deform);
/* point each model's points at its node's moved. the caller supplies and
 * owns them: deform_create doesn't allocate them, nor deform_free free
 * them */
void deform_use_moved(struct deform *deform);

#endif
