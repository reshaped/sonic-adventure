/* Casinopolis stage task, act requests, the texture banks the module's
 * objects draw with, and the sounds they share */
#ifndef STG09_CASINO_H
#define STG09_CASINO_H
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/model.h"
#include "system/loader.h"
#include "system/task.h"

/* values of casino_request: an act plus 1, so that CASINO_REQUEST_NONE asks
 * for none. in act 0 the stage task takes CASINO_REQUEST_ACT2 or
 * CASINO_REQUEST_ACT3, the tables. in act 1 it takes CASINO_REQUEST_ACT0. it
 * takes none while player 0 is Knuckles or the stage isn't in play. a request
 * it takes changes the act and clears casino_request. a request it doesn't
 * take stays in casino_request */
#define CASINO_REQUEST_NONE 0
#define CASINO_REQUEST_ACT0 1
#define CASINO_REQUEST_ACT2 3
#define CASINO_REQUEST_ACT3 4

/* texture banks for casino_set_texture, named for their acts counted from 1: 12
 * for acts 0 and 1, and 34 for the tables, acts 2 and 3 */
#define CASINO_TEXTURES_ACTS_12 12
#define CASINO_TEXTURES_ACTS_34 34

/* the palette the stage task reads from PL_9MB.BIN in acts 0 and 1, and its
 * size. O MUD tints the players' palette toward it */
#define MUDDY_PALETTE SET_AREA_4
#define PALETTE_SIZE sizeof(draw_palettes[DRAW_PLAYER_PALETTE])

/* the stage's own sounds, each played by more than one of its objects */
#define SE_SHOWER 267
#define SE_DOOR 268
#define SE_SLAM 277

extern int casino_request;
/* the rings deposited in O KIN's heap. gold.c sets the heap's height, its
 * wall and the statue's level from it. stage.c zeroes the count only in
 * STAGE_RESTART_FADE, which only the cart and the Chao race enter */
extern int casino_rings_deposited;

/* OBJ_CASINO9's textures, which the stage task loads for acts 0 and 1 */
extern NJS_TEXLIST obj_casino9_texlist;

/* the stage task step, which 1ST_READ.BIN's stage_steps names */
void casino_step(struct task *task);
/* make OBJ_CASINO9's texture list current for CASINO_TEXTURES_ACTS_12, or
 * OBJ_CASINO's for CASINO_TEXTURES_ACTS_34; any other value loops forever */
void casino_set_texture(int acts);

#endif
