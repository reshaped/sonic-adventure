/* debris: a model's nodes, flung apart. they fall until every piece that
 * draws is at or below y 0 */
#ifndef B_EGM1_DEBRIS_H
#define B_EGM1_DEBRIS_H
#include <ninja.h>
#include <sg_xpt.h>

struct angles;

/* what egg_hornet_debris_create flings apart: object's tree, placed at pos
 * and turned by ang, at twice its size */
struct debris_config {
    NJS_TEXLIST *texlist;
    NJS_OBJECT *object;
    NJS_POINT3 *pos;
    struct angles *ang;
    /* called for each piece each step, with its position and the steps the
     * debris has flown, or null */
    void (*trail)(NJS_POINT3 *pos, int steps);
    /* the range of each piece's spin about each axis, half of it either
     * way. the pieces' angles take it, but the draw doesn't read them */
    Angle spin;
    Float speed; /* each piece's speed out from pos, in x and z */
    /* each piece's delay after the piece before it, which nothing reads */
    Uint16 delay;
};

/* fling config's object apart. config is read only during the call. its
 * texlist, its object's models and its trail must outlive the debris. the
 * allocations of the debris's record and its pieces go unchecked. a failed
 * one is written through null */
void egg_hornet_debris_create(struct debris_config *config);

#endif
