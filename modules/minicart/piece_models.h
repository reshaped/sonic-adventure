/* the pieces a wrecked cart breaks into */
#ifndef MINICART_PIECE_MODELS_H
#define MINICART_PIECE_MODELS_H

#include <ninja.h>

/* cart_object's ten pieces come first, then Big's and E-102's, and a null
 * ends each ten. e_cart.c draws only cart_object's */
extern NJS_MODEL *cart_piece_models[];

#endif
