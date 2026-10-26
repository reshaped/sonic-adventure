/* model trees: drawn node by node, as each node's evalflags say */
#include <ninja.h>

#include "draw/model.h"
#include "draw/tree.h"

/* the evalflags bits that each spare a node one move: its translation,
 * its rotation or its scale */
#define UNIT_ALL (NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL)

/* draw a node and each sibling after it, with the nodes under each */
static void draw_nodes(NJS_OBJECT *node)
{
    do {
        /* a case for each set of those bits, so a node makes only the
         * moves it needs */
        switch (node->evalflags & UNIT_ALL) {
        case UNIT_ALL:
            /* nothing to move: it draws in its parent's matrix */
            if (!(node->evalflags & NJD_EVAL_HIDE)) {
                draw_model_nested(node->model);
            }
            if (!(node->evalflags & NJD_EVAL_BREAK)) {
                draw_nodes(node->child);
            }
            break;
        case 0:
            njPushMatrix(NULL);
            njTranslate(NULL, node->pos[0], node->pos[1], node->pos[2]);
            if (node->evalflags & NJD_EVAL_ZXY_ANG) {
                njRotateZXY(NULL, node->ang[0], node->ang[1], node->ang[2]);
            } else {
                njRotateXYZ(NULL, node->ang[0], node->ang[1], node->ang[2]);
            }
            njScale(NULL, node->scl[0], node->scl[1], node->scl[2]);
            if (!(node->evalflags & NJD_EVAL_HIDE)) {
                draw_model_nested(node->model);
            }
            if (!(node->evalflags & NJD_EVAL_BREAK)) {
                draw_nodes(node->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_POS:
            njPushMatrix(NULL);
            if (node->evalflags & NJD_EVAL_ZXY_ANG) {
                njRotateZXY(NULL, node->ang[0], node->ang[1], node->ang[2]);
            } else {
                njRotateXYZ(NULL, node->ang[0], node->ang[1], node->ang[2]);
            }
            njScale(NULL, node->scl[0], node->scl[1], node->scl[2]);
            if (!(node->evalflags & NJD_EVAL_HIDE)) {
                draw_model_nested(node->model);
            }
            if (!(node->evalflags & NJD_EVAL_BREAK)) {
                draw_nodes(node->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_ANG:
            njPushMatrix(NULL);
            njTranslate(NULL, node->pos[0], node->pos[1], node->pos[2]);
            njScale(NULL, node->scl[0], node->scl[1], node->scl[2]);
            if (!(node->evalflags & NJD_EVAL_HIDE)) {
                draw_model_nested(node->model);
            }
            if (!(node->evalflags & NJD_EVAL_BREAK)) {
                draw_nodes(node->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_ANG:
            njPushMatrix(NULL);
            njScale(NULL, node->scl[0], node->scl[1], node->scl[2]);
            if (!(node->evalflags & NJD_EVAL_HIDE)) {
                draw_model_nested(node->model);
            }
            if (!(node->evalflags & NJD_EVAL_BREAK)) {
                draw_nodes(node->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_SCL:
            njPushMatrix(NULL);
            njTranslate(NULL, node->pos[0], node->pos[1], node->pos[2]);
            if (node->evalflags & NJD_EVAL_ZXY_ANG) {
                njRotateZXY(NULL, node->ang[0], node->ang[1], node->ang[2]);
            } else {
                njRotateXYZ(NULL, node->ang[0], node->ang[1], node->ang[2]);
            }
            if (!(node->evalflags & NJD_EVAL_HIDE)) {
                draw_model_nested(node->model);
            }
            if (!(node->evalflags & NJD_EVAL_BREAK)) {
                draw_nodes(node->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_POS | NJD_EVAL_UNIT_SCL:
            njPushMatrix(NULL);
            if (node->evalflags & NJD_EVAL_ZXY_ANG) {
                njRotateZXY(NULL, node->ang[0], node->ang[1], node->ang[2]);
            } else {
                njRotateXYZ(NULL, node->ang[0], node->ang[1], node->ang[2]);
            }
            if (!(node->evalflags & NJD_EVAL_HIDE)) {
                draw_model_nested(node->model);
            }
            if (!(node->evalflags & NJD_EVAL_BREAK)) {
                draw_nodes(node->child);
            }
            njPopMatrix(1);
            break;
        case NJD_EVAL_UNIT_ANG | NJD_EVAL_UNIT_SCL:
            njPushMatrix(NULL);
            njTranslate(NULL, node->pos[0], node->pos[1], node->pos[2]);
            if (!(node->evalflags & NJD_EVAL_HIDE)) {
                draw_model_nested(node->model);
            }
            if (!(node->evalflags & NJD_EVAL_BREAK)) {
                draw_nodes(node->child);
            }
            njPopMatrix(1);
            break;
        }
        node = node->sibling;
    } while (node != NULL);
}

void draw_tree(NJS_OBJECT *object)
{
    njGetkmPointer();
    draw_nodes(object);
    njSetkmPointer();
}
