/* Eggman's textures and chaos6_eggman_texlist, which eggman.c draws with
 * and chaos6_pvms names. a collision shape lies beside them, with a byte
 * taken for its count. nothing names either */
#include <ninja.h>
#include <sg_xpt.h>

#include "b_chaos6/eggman_textures.h"
#include "collision/collision.h"

#define TEXT_1_LENGTH 8
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 9
#define TEXT_4_LENGTH 5
#define TEXT_5_LENGTH 5
#define TEXT_6_LENGTH 6
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 8
#define TEXT_9_LENGTH 6
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 7
#define TEXT_12_LENGTH 6
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 9
#define TEXT_15_LENGTH 6
#define TEXT_16_LENGTH 7
#define TEXT_17_LENGTH 9
#define TEXT_18_LENGTH 8
#define TEXT_19_LENGTH 9
#define TEXT_20_LENGTH 7
#define TEXT_21_LENGTH 8
#define TEXT_22_LENGTH 8
#define TEXT_23_LENGTH 7
#define TEXT_24_LENGTH 7
#define TEXT_25_LENGTH 6
#define TEXT_26_LENGTH 6
#define TEXT_27_LENGTH 6
#include "b_chaos6/eggman_textures/text.inc"

/* a const byte of 1, taken for chaos6_eggman_shapes' count: jets.c and
 * bomb.c each keep such a byte for the count they pass collision_init */
const Uint8 chaos6_eggman_shape_count = {
#include "b_chaos6/eggman_textures/chaos6_eggman_shape_count.inc"
};
static NJS_TEXNAME chaos6_eggman_textures[27] = {
    {TEXT_1, 0, 0},  {TEXT_2, 0, 0},  {TEXT_3, 0, 0},  {TEXT_4, 0, 0},
    {TEXT_5, 0, 0},  {TEXT_6, 0, 0},  {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0},  {TEXT_10, 0, 0}, {TEXT_11, 0, 0}, {TEXT_12, 0, 0},
    {TEXT_13, 0, 0}, {TEXT_14, 0, 0}, {TEXT_15, 0, 0}, {TEXT_16, 0, 0},
    {TEXT_17, 0, 0}, {TEXT_18, 0, 0}, {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0}, {TEXT_24, 0, 0},
    {TEXT_25, 0, 0}, {TEXT_26, 0, 0}, {TEXT_27, 0, 0},
};
NJS_TEXLIST chaos6_eggman_texlist = {
#include "b_chaos6/eggman_textures/chaos6_eggman_texlist.inc"
};
struct collision_shape chaos6_eggman_shapes[1] = {
#include "b_chaos6/eggman_textures/chaos6_eggman_shapes.inc"
};
