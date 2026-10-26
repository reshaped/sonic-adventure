/* the animals' models and textures, and the barrier's. kind by kind from the
 * seal (goma) to the otter (rako), each kind's model, motion and action come
 * first, then its texture table and texlist. the barrier's texture table,
 * texlist and model follow the otter's. the texture tables point at the
 * textures' names, strings in this file's constants
 *
 * one object file can hold all of this, as this file does. R32, which compiles
 * the game's own sources, starts an object file's data on a multiple of 8 and
 * aligns no item past 4. so an item 4 past a multiple of 8, straight after the
 * item before, lies in that item's object file. goma_texlist and the penguin's
 * first material each lie so (0x8c17a244, 0x8c17a24c). that puts goma_textures,
 * goma_texlist and the material in one object file. the data either side meets
 * them on a multiple of 8 with no gap (0x8c17a208, 0x8c17a260). so the bytes
 * tie no other data to that object file. in the constants, each name starts on
 * the first multiple of 4 past the end of the one before, as one object file's
 * strings do */
#include <ninja.h>
#include <sg_xpt.h>

#include "animal/animal.h"
#include "animal/barrier.h"
#include "animal/textures.h"

#define TEXT_1_LENGTH 9
#define TEXT_2_LENGTH 8
#define TEXT_3_LENGTH 7
#define TEXT_4_LENGTH 8
#define TEXT_5_LENGTH 9
#define TEXT_6_LENGTH 9
#define TEXT_7_LENGTH 8
#define TEXT_8_LENGTH 7
#define TEXT_9_LENGTH 7
#define TEXT_10_LENGTH 8
#define TEXT_11_LENGTH 8
#define TEXT_12_LENGTH 11
#define TEXT_13_LENGTH 9
#define TEXT_14_LENGTH 8
#define TEXT_15_LENGTH 10
#define TEXT_16_LENGTH 11
#define TEXT_17_LENGTH 8
#define TEXT_18_LENGTH 10
#define TEXT_19_LENGTH 13
#define TEXT_20_LENGTH 9
#define TEXT_21_LENGTH 7
#define TEXT_22_LENGTH 9
#define TEXT_23_LENGTH 9
#define TEXT_24_LENGTH 9
#define TEXT_25_LENGTH 10
#define TEXT_26_LENGTH 9
#define TEXT_27_LENGTH 8
#define TEXT_28_LENGTH 7
#define TEXT_29_LENGTH 11
#define TEXT_30_LENGTH 11
#define TEXT_31_LENGTH 8
#define TEXT_32_LENGTH 9
#define TEXT_33_LENGTH 8
#define TEXT_34_LENGTH 8
#define TEXT_35_LENGTH 11
#define TEXT_36_LENGTH 8
#define TEXT_37_LENGTH 9
#define TEXT_38_LENGTH 10
#define TEXT_39_LENGTH 7
#define TEXT_40_LENGTH 8
#define TEXT_41_LENGTH 9
#define TEXT_42_LENGTH 8
#define TEXT_43_LENGTH 9
#define TEXT_44_LENGTH 7
#define TEXT_45_LENGTH 8
#define TEXT_46_LENGTH 7
#define TEXT_47_LENGTH 8
#define TEXT_48_LENGTH 8
#define TEXT_49_LENGTH 11
#define TEXT_50_LENGTH 8
#define TEXT_51_LENGTH 8
#define TEXT_52_LENGTH 9
#define TEXT_53_LENGTH 8
#define TEXT_54_LENGTH 7
#define TEXT_55_LENGTH 11
#define TEXT_56_LENGTH 8
#define TEXT_57_LENGTH 9
#define TEXT_58_LENGTH 7
#define TEXT_59_LENGTH 11
#define TEXT_60_LENGTH 8
#define TEXT_61_LENGTH 10
#define TEXT_62_LENGTH 8
#define TEXT_63_LENGTH 9
#include "animal/models/text.inc"

static NJS_MATERIAL dat_8c177eb8[1] = {
#include "animal/models/dat_8c177eb8.inc"
};
static Sint16 dat_8c177ecc[10] = {
#include "animal/models/dat_8c177ecc.inc"
};
static NJS_COLOR dat_8c177ee0[8] = {
#include "animal/models/dat_8c177ee0.inc"
};
static NJS_MESHSET dat_8c177f00[1] = {
#include "animal/models/dat_8c177f00.inc"
};
static NJS_POINT3 dat_8c177f18[6] = {
#include "animal/models/dat_8c177f18.inc"
};
static NJS_VECTOR dat_8c177f60[6] = {
#include "animal/models/dat_8c177f60.inc"
};
static NJS_MODEL dat_8c177fa8 = {
#include "animal/models/dat_8c177fa8.inc"
};
static NJS_OBJECT dat_8c177fd0 = {
#include "animal/models/dat_8c177fd0.inc"
};
static NJS_OBJECT dat_8c178004 = {
#include "animal/models/dat_8c178004.inc"
};
static NJS_OBJECT dat_8c178038 = {
#include "animal/models/dat_8c178038.inc"
};
static NJS_OBJECT dat_8c17806c = {
#include "animal/models/dat_8c17806c.inc"
};
static NJS_MATERIAL dat_8c1780a0[1] = {
#include "animal/models/dat_8c1780a0.inc"
};
static Sint16 dat_8c1780b4[4] = {
#include "animal/models/dat_8c1780b4.inc"
};
static NJS_COLOR dat_8c1780bc[3] = {
#include "animal/models/dat_8c1780bc.inc"
};
static NJS_MESHSET dat_8c1780c8[1] = {
#include "animal/models/dat_8c1780c8.inc"
};
static NJS_POINT3 dat_8c1780e0[3] = {
#include "animal/models/dat_8c1780e0.inc"
};
static NJS_VECTOR dat_8c178104[3] = {
#include "animal/models/dat_8c178104.inc"
};
static NJS_MODEL dat_8c178128 = {
#include "animal/models/dat_8c178128.inc"
};
static NJS_OBJECT dat_8c178150 = {
#include "animal/models/dat_8c178150.inc"
};
static NJS_OBJECT dat_8c178184 = {
#include "animal/models/dat_8c178184.inc"
};
static NJS_OBJECT dat_8c1781b8 = {
#include "animal/models/dat_8c1781b8.inc"
};
static NJS_OBJECT dat_8c1781ec = {
#include "animal/models/dat_8c1781ec.inc"
};
static NJS_MATERIAL dat_8c178220[1] = {
#include "animal/models/dat_8c178220.inc"
};
static Sint16 dat_8c178234[4] = {
#include "animal/models/dat_8c178234.inc"
};
static NJS_COLOR dat_8c17823c[3] = {
#include "animal/models/dat_8c17823c.inc"
};
static NJS_MESHSET dat_8c178248[1] = {
#include "animal/models/dat_8c178248.inc"
};
static NJS_POINT3 dat_8c178260[3] = {
#include "animal/models/dat_8c178260.inc"
};
static NJS_VECTOR dat_8c178284[3] = {
#include "animal/models/dat_8c178284.inc"
};
static NJS_MODEL dat_8c1782a8 = {
#include "animal/models/dat_8c1782a8.inc"
};
static NJS_OBJECT dat_8c1782d0 = {
#include "animal/models/dat_8c1782d0.inc"
};
static NJS_OBJECT dat_8c178304 = {
#include "animal/models/dat_8c178304.inc"
};
static NJS_OBJECT dat_8c178338 = {
#include "animal/models/dat_8c178338.inc"
};
static NJS_OBJECT dat_8c17836c = {
#include "animal/models/dat_8c17836c.inc"
};
static NJS_MATERIAL dat_8c1783a0[1] = {
#include "animal/models/dat_8c1783a0.inc"
};
static Sint16 dat_8c1783b4[10] = {
#include "animal/models/dat_8c1783b4.inc"
};
static NJS_COLOR dat_8c1783c8[8] = {
#include "animal/models/dat_8c1783c8.inc"
};
static NJS_MESHSET dat_8c1783e8[1] = {
#include "animal/models/dat_8c1783e8.inc"
};
static NJS_POINT3 dat_8c178400[6] = {
#include "animal/models/dat_8c178400.inc"
};
static NJS_VECTOR dat_8c178448[6] = {
#include "animal/models/dat_8c178448.inc"
};
static NJS_MODEL dat_8c178490 = {
#include "animal/models/dat_8c178490.inc"
};
static NJS_OBJECT dat_8c1784b8 = {
#include "animal/models/dat_8c1784b8.inc"
};
static NJS_OBJECT dat_8c1784ec = {
#include "animal/models/dat_8c1784ec.inc"
};
static NJS_OBJECT dat_8c178520 = {
#include "animal/models/dat_8c178520.inc"
};
static NJS_OBJECT dat_8c178554 = {
#include "animal/models/dat_8c178554.inc"
};
static NJS_MATERIAL dat_8c178588[1] = {
#include "animal/models/dat_8c178588.inc"
};
static Sint16 dat_8c17859c[10] = {
#include "animal/models/dat_8c17859c.inc"
};
static NJS_COLOR dat_8c1785b0[8] = {
#include "animal/models/dat_8c1785b0.inc"
};
static NJS_MESHSET dat_8c1785d0[1] = {
#include "animal/models/dat_8c1785d0.inc"
};
static NJS_POINT3 dat_8c1785e8[6] = {
#include "animal/models/dat_8c1785e8.inc"
};
static NJS_VECTOR dat_8c178630[6] = {
#include "animal/models/dat_8c178630.inc"
};
static NJS_MODEL dat_8c178678 = {
#include "animal/models/dat_8c178678.inc"
};
static NJS_OBJECT dat_8c1786a0 = {
#include "animal/models/dat_8c1786a0.inc"
};
static NJS_OBJECT dat_8c1786d4 = {
#include "animal/models/dat_8c1786d4.inc"
};
static NJS_OBJECT dat_8c178708 = {
#include "animal/models/dat_8c178708.inc"
};
static NJS_OBJECT dat_8c17873c = {
#include "animal/models/dat_8c17873c.inc"
};
static NJS_MATERIAL dat_8c178770[1] = {
#include "animal/models/dat_8c178770.inc"
};
static Sint16 dat_8c178784[10] = {
#include "animal/models/dat_8c178784.inc"
};
static NJS_COLOR dat_8c178798[8] = {
#include "animal/models/dat_8c178798.inc"
};
static NJS_MESHSET dat_8c1787b8[1] = {
#include "animal/models/dat_8c1787b8.inc"
};
static NJS_POINT3 dat_8c1787d0[6] = {
#include "animal/models/dat_8c1787d0.inc"
};
static NJS_VECTOR dat_8c178818[6] = {
#include "animal/models/dat_8c178818.inc"
};
static NJS_MODEL dat_8c178860 = {
#include "animal/models/dat_8c178860.inc"
};
static NJS_OBJECT dat_8c178888 = {
#include "animal/models/dat_8c178888.inc"
};
static NJS_OBJECT dat_8c1788bc = {
#include "animal/models/dat_8c1788bc.inc"
};
static NJS_OBJECT dat_8c1788f0 = {
#include "animal/models/dat_8c1788f0.inc"
};
static NJS_OBJECT dat_8c178924 = {
#include "animal/models/dat_8c178924.inc"
};
static NJS_MATERIAL dat_8c178958[2] = {
#include "animal/models/dat_8c178958.inc"
};
static Sint16 dat_8c178980[59] = {
#include "animal/models/dat_8c178980.inc"
};
static Sint16 dat_8c1789f6[89] = {
#include "animal/models/dat_8c1789f6.inc"
};
static NJS_COLOR dat_8c178aa8[50] = {
#include "animal/models/dat_8c178aa8.inc"
};
static NJS_COLOR dat_8c178b70[72] = {
#include "animal/models/dat_8c178b70.inc"
};
static NJS_MESHSET dat_8c178c90[2] = {
#include "animal/models/dat_8c178c90.inc"
};
static NJS_POINT3 dat_8c178cc0[37] = {
#include "animal/models/dat_8c178cc0.inc"
};
static NJS_VECTOR dat_8c178e7c[37] = {
#include "animal/models/dat_8c178e7c.inc"
};
static NJS_MODEL dat_8c179038 = {
#include "animal/models/dat_8c179038.inc"
};
static NJS_OBJECT dat_8c179060 = {
#include "animal/models/dat_8c179060.inc"
};
static NJS_OBJECT dat_8c179094 = {
#include "animal/models/dat_8c179094.inc"
};
static NJS_OBJECT dat_8c1790c8 = {
#include "animal/models/dat_8c1790c8.inc"
};
static NJS_OBJECT dat_8c1790fc = {
#include "animal/models/dat_8c1790fc.inc"
};
static NJS_OBJECT goma_object = {
#include "animal/models/goma_object.inc"
};
static NJS_MKEY_F dat_8c179164[2] = {
#include "animal/models/dat_8c179164.inc"
};
static NJS_MKEY_A dat_8c179184[2] = {
#include "animal/models/dat_8c179184.inc"
};
static NJS_MKEY_F dat_8c1791a4[17] = {
#include "animal/models/dat_8c1791a4.inc"
};
static NJS_MKEY_A dat_8c1792b4[2] = {
#include "animal/models/dat_8c1792b4.inc"
};
static NJS_MKEY_F dat_8c1792d4[2] = {
#include "animal/models/dat_8c1792d4.inc"
};
static NJS_MKEY_A dat_8c1792f4[17] = {
#include "animal/models/dat_8c1792f4.inc"
};
static NJS_MKEY_F dat_8c179404[2] = {
#include "animal/models/dat_8c179404.inc"
};
static NJS_MKEY_A dat_8c179424[2] = {
#include "animal/models/dat_8c179424.inc"
};
static NJS_MKEY_F dat_8c179444[2] = {
#include "animal/models/dat_8c179444.inc"
};
static NJS_MKEY_A dat_8c179464[2] = {
#include "animal/models/dat_8c179464.inc"
};
static NJS_MKEY_F dat_8c179484[2] = {
#include "animal/models/dat_8c179484.inc"
};
static NJS_MKEY_A dat_8c1794a4[2] = {
#include "animal/models/dat_8c1794a4.inc"
};
static NJS_MKEY_F dat_8c1794c4[2] = {
#include "animal/models/dat_8c1794c4.inc"
};
static NJS_MKEY_A dat_8c1794e4[17] = {
#include "animal/models/dat_8c1794e4.inc"
};
static NJS_MKEY_F dat_8c1795f4[2] = {
#include "animal/models/dat_8c1795f4.inc"
};
static NJS_MKEY_A dat_8c179614[2] = {
#include "animal/models/dat_8c179614.inc"
};
static NJS_MKEY_F dat_8c179634[2] = {
#include "animal/models/dat_8c179634.inc"
};
static NJS_MKEY_A dat_8c179654[2] = {
#include "animal/models/dat_8c179654.inc"
};
static NJS_MKEY_F dat_8c179674[2] = {
#include "animal/models/dat_8c179674.inc"
};
static NJS_MKEY_A dat_8c179694[2] = {
#include "animal/models/dat_8c179694.inc"
};
static NJS_MKEY_F dat_8c1796b4[2] = {
#include "animal/models/dat_8c1796b4.inc"
};
static NJS_MKEY_A dat_8c1796d4[17] = {
#include "animal/models/dat_8c1796d4.inc"
};
static NJS_MKEY_F dat_8c1797e4[2] = {
#include "animal/models/dat_8c1797e4.inc"
};
static NJS_MKEY_A dat_8c179804[2] = {
#include "animal/models/dat_8c179804.inc"
};
static NJS_MKEY_F dat_8c179824[2] = {
#include "animal/models/dat_8c179824.inc"
};
static NJS_MKEY_A dat_8c179844[2] = {
#include "animal/models/dat_8c179844.inc"
};
static NJS_MKEY_F dat_8c179864[2] = {
#include "animal/models/dat_8c179864.inc"
};
static NJS_MKEY_A dat_8c179884[2] = {
#include "animal/models/dat_8c179884.inc"
};
static NJS_MKEY_F dat_8c1798a4[2] = {
#include "animal/models/dat_8c1798a4.inc"
};
static NJS_MKEY_A dat_8c1798c4[17] = {
#include "animal/models/dat_8c1798c4.inc"
};
static NJS_MKEY_F dat_8c1799d4[2] = {
#include "animal/models/dat_8c1799d4.inc"
};
static NJS_MKEY_A dat_8c1799f4[2] = {
#include "animal/models/dat_8c1799f4.inc"
};
static NJS_MKEY_F dat_8c179a14[2] = {
#include "animal/models/dat_8c179a14.inc"
};
static NJS_MKEY_A dat_8c179a34[2] = {
#include "animal/models/dat_8c179a34.inc"
};
static NJS_MKEY_F dat_8c179a54[2] = {
#include "animal/models/dat_8c179a54.inc"
};
static NJS_MKEY_A dat_8c179a74[2] = {
#include "animal/models/dat_8c179a74.inc"
};
static NJS_MKEY_F dat_8c179a94[2] = {
#include "animal/models/dat_8c179a94.inc"
};
static NJS_MKEY_A dat_8c179ab4[17] = {
#include "animal/models/dat_8c179ab4.inc"
};
static NJS_MKEY_F dat_8c179bc4[2] = {
#include "animal/models/dat_8c179bc4.inc"
};
static NJS_MKEY_A dat_8c179be4[2] = {
#include "animal/models/dat_8c179be4.inc"
};
static NJS_MKEY_F dat_8c179c04[2] = {
#include "animal/models/dat_8c179c04.inc"
};
static NJS_MKEY_A dat_8c179c24[2] = {
#include "animal/models/dat_8c179c24.inc"
};
static NJS_MKEY_F dat_8c179c44[2] = {
#include "animal/models/dat_8c179c44.inc"
};
static NJS_MKEY_A dat_8c179c64[2] = {
#include "animal/models/dat_8c179c64.inc"
};
static NJS_MKEY_F dat_8c179c84[2] = {
#include "animal/models/dat_8c179c84.inc"
};
static NJS_MKEY_A dat_8c179ca4[17] = {
#include "animal/models/dat_8c179ca4.inc"
};
static NJS_MKEY_F dat_8c179db4[2] = {
#include "animal/models/dat_8c179db4.inc"
};
static NJS_MKEY_A dat_8c179dd4[2] = {
#include "animal/models/dat_8c179dd4.inc"
};
static NJS_MKEY_F dat_8c179df4[2] = {
#include "animal/models/dat_8c179df4.inc"
};
static NJS_MKEY_A dat_8c179e14[2] = {
#include "animal/models/dat_8c179e14.inc"
};
static NJS_MKEY_F dat_8c179e34[2] = {
#include "animal/models/dat_8c179e34.inc"
};
static NJS_MKEY_A dat_8c179e54[2] = {
#include "animal/models/dat_8c179e54.inc"
};
static NJS_MKEY_F dat_8c179e74[2] = {
#include "animal/models/dat_8c179e74.inc"
};
static NJS_MKEY_A dat_8c179e94[17] = {
#include "animal/models/dat_8c179e94.inc"
};
static NJS_MKEY_F dat_8c179fa4[2] = {
#include "animal/models/dat_8c179fa4.inc"
};
static NJS_MKEY_A dat_8c179fc4[2] = {
#include "animal/models/dat_8c179fc4.inc"
};
static NJS_MKEY_F dat_8c179fe4[2] = {
#include "animal/models/dat_8c179fe4.inc"
};
static NJS_MKEY_A dat_8c17a004[2] = {
#include "animal/models/dat_8c17a004.inc"
};
static NJS_MDATA2 dat_8c17a024[29] = {
#include "animal/models/dat_8c17a024.inc"
};
static NJS_MOTION goma_motion = {
#include "animal/models/goma_motion.inc"
};
NJS_ACTION goma_action = {
#include "animal/models/goma_action.inc"
};

static NJS_TEXNAME goma_textures[5] = {
    {TEXT_1, 0, 0}, {TEXT_2, 0, 0}, {TEXT_3, 0, 0},
    {TEXT_4, 0, 0}, {TEXT_5, 0, 0},
};
NJS_TEXLIST goma_texlist = {
#include "animal/models/goma_texlist.inc"
};
static NJS_MATERIAL dat_8c17a24c[1] = {
#include "animal/models/dat_8c17a24c.inc"
};
static Sint16 dat_8c17a260[5] = {
#include "animal/models/dat_8c17a260.inc"
};
static NJS_COLOR dat_8c17a26c[4] = {
#include "animal/models/dat_8c17a26c.inc"
};
static NJS_MESHSET dat_8c17a27c[1] = {
#include "animal/models/dat_8c17a27c.inc"
};
static NJS_POINT3 dat_8c17a294[4] = {
#include "animal/models/dat_8c17a294.inc"
};
static NJS_VECTOR dat_8c17a2c4[4] = {
#include "animal/models/dat_8c17a2c4.inc"
};
static NJS_MODEL dat_8c17a2f4 = {
#include "animal/models/dat_8c17a2f4.inc"
};
static NJS_OBJECT dat_8c17a31c = {
#include "animal/models/dat_8c17a31c.inc"
};
static NJS_MATERIAL dat_8c17a350[1] = {
#include "animal/models/dat_8c17a350.inc"
};
static Sint16 dat_8c17a364[10] = {
#include "animal/models/dat_8c17a364.inc"
};
static NJS_COLOR dat_8c17a378[8] = {
#include "animal/models/dat_8c17a378.inc"
};
static NJS_MESHSET dat_8c17a398[1] = {
#include "animal/models/dat_8c17a398.inc"
};
static NJS_POINT3 dat_8c17a3b0[6] = {
#include "animal/models/dat_8c17a3b0.inc"
};
static NJS_VECTOR dat_8c17a3f8[6] = {
#include "animal/models/dat_8c17a3f8.inc"
};
static NJS_MODEL dat_8c17a440 = {
#include "animal/models/dat_8c17a440.inc"
};
static NJS_OBJECT dat_8c17a468 = {
#include "animal/models/dat_8c17a468.inc"
};
static NJS_MATERIAL dat_8c17a49c[1] = {
#include "animal/models/dat_8c17a49c.inc"
};
static Sint16 dat_8c17a4b0[151] = {
#include "animal/models/dat_8c17a4b0.inc"
};
static NJS_COLOR dat_8c17a5e0[116] = {
#include "animal/models/dat_8c17a5e0.inc"
};
static NJS_MESHSET dat_8c17a7b0[1] = {
#include "animal/models/dat_8c17a7b0.inc"
};
static NJS_POINT3 dat_8c17a7c8[25] = {
#include "animal/models/dat_8c17a7c8.inc"
};
static NJS_VECTOR dat_8c17a8f4[25] = {
#include "animal/models/dat_8c17a8f4.inc"
};
static NJS_MODEL dat_8c17aa20 = {
#include "animal/models/dat_8c17aa20.inc"
};
static NJS_OBJECT dat_8c17aa48 = {
#include "animal/models/dat_8c17aa48.inc"
};
static NJS_MATERIAL dat_8c17aa7c[1] = {
#include "animal/models/dat_8c17aa7c.inc"
};
static Sint16 dat_8c17aa90[17] = {
#include "animal/models/dat_8c17aa90.inc"
};
static NJS_COLOR dat_8c17aab4[13] = {
#include "animal/models/dat_8c17aab4.inc"
};
static NJS_MESHSET dat_8c17aae8[1] = {
#include "animal/models/dat_8c17aae8.inc"
};
static NJS_POINT3 dat_8c17ab00[7] = {
#include "animal/models/dat_8c17ab00.inc"
};
static NJS_VECTOR dat_8c17ab54[7] = {
#include "animal/models/dat_8c17ab54.inc"
};
static NJS_MODEL dat_8c17aba8 = {
#include "animal/models/dat_8c17aba8.inc"
};
static NJS_OBJECT dat_8c17abd0 = {
#include "animal/models/dat_8c17abd0.inc"
};
static NJS_OBJECT dat_8c17ac04 = {
#include "animal/models/dat_8c17ac04.inc"
};
static NJS_OBJECT dat_8c17ac38 = {
#include "animal/models/dat_8c17ac38.inc"
};
static NJS_OBJECT dat_8c17ac6c = {
#include "animal/models/dat_8c17ac6c.inc"
};
static NJS_OBJECT dat_8c17aca0 = {
#include "animal/models/dat_8c17aca0.inc"
};
static NJS_MATERIAL dat_8c17acd4[1] = {
#include "animal/models/dat_8c17acd4.inc"
};
static Sint16 dat_8c17ace8[17] = {
#include "animal/models/dat_8c17ace8.inc"
};
static NJS_COLOR dat_8c17ad0c[13] = {
#include "animal/models/dat_8c17ad0c.inc"
};
static NJS_MESHSET dat_8c17ad40[1] = {
#include "animal/models/dat_8c17ad40.inc"
};
static NJS_POINT3 dat_8c17ad58[7] = {
#include "animal/models/dat_8c17ad58.inc"
};
static NJS_VECTOR dat_8c17adac[7] = {
#include "animal/models/dat_8c17adac.inc"
};
static NJS_MODEL dat_8c17ae00 = {
#include "animal/models/dat_8c17ae00.inc"
};
static NJS_OBJECT dat_8c17ae28 = {
#include "animal/models/dat_8c17ae28.inc"
};
static NJS_OBJECT dat_8c17ae5c = {
#include "animal/models/dat_8c17ae5c.inc"
};
static NJS_OBJECT dat_8c17ae90 = {
#include "animal/models/dat_8c17ae90.inc"
};
static NJS_OBJECT dat_8c17aec4 = {
#include "animal/models/dat_8c17aec4.inc"
};
static NJS_OBJECT dat_8c17aef8 = {
#include "animal/models/dat_8c17aef8.inc"
};
static NJS_MATERIAL dat_8c17af2c[1] = {
#include "animal/models/dat_8c17af2c.inc"
};
static Sint16 dat_8c17af40[16] = {
#include "animal/models/dat_8c17af40.inc"
};
static NJS_COLOR dat_8c17af60[14] = {
#include "animal/models/dat_8c17af60.inc"
};
static NJS_MESHSET dat_8c17af98[1] = {
#include "animal/models/dat_8c17af98.inc"
};
static NJS_POINT3 dat_8c17afb0[8] = {
#include "animal/models/dat_8c17afb0.inc"
};
static NJS_VECTOR dat_8c17b010[8] = {
#include "animal/models/dat_8c17b010.inc"
};
static NJS_MODEL dat_8c17b070 = {
#include "animal/models/dat_8c17b070.inc"
};
static NJS_OBJECT dat_8c17b098 = {
#include "animal/models/dat_8c17b098.inc"
};
static NJS_OBJECT dat_8c17b0cc = {
#include "animal/models/dat_8c17b0cc.inc"
};
static NJS_OBJECT dat_8c17b100 = {
#include "animal/models/dat_8c17b100.inc"
};
static NJS_OBJECT dat_8c17b134 = {
#include "animal/models/dat_8c17b134.inc"
};
static NJS_MATERIAL dat_8c17b168[1] = {
#include "animal/models/dat_8c17b168.inc"
};
static Sint16 dat_8c17b17c[16] = {
#include "animal/models/dat_8c17b17c.inc"
};
static NJS_COLOR dat_8c17b19c[14] = {
#include "animal/models/dat_8c17b19c.inc"
};
static NJS_MESHSET dat_8c17b1d4[1] = {
#include "animal/models/dat_8c17b1d4.inc"
};
static NJS_POINT3 dat_8c17b1ec[8] = {
#include "animal/models/dat_8c17b1ec.inc"
};
static NJS_VECTOR dat_8c17b24c[8] = {
#include "animal/models/dat_8c17b24c.inc"
};
static NJS_MODEL dat_8c17b2ac = {
#include "animal/models/dat_8c17b2ac.inc"
};
static NJS_OBJECT dat_8c17b2d4 = {
#include "animal/models/dat_8c17b2d4.inc"
};
static NJS_OBJECT dat_8c17b308 = {
#include "animal/models/dat_8c17b308.inc"
};
static NJS_OBJECT dat_8c17b33c = {
#include "animal/models/dat_8c17b33c.inc"
};
static NJS_OBJECT dat_8c17b370 = {
#include "animal/models/dat_8c17b370.inc"
};
static NJS_MATERIAL dat_8c17b3a4[1] = {
#include "animal/models/dat_8c17b3a4.inc"
};
static Sint16 dat_8c17b3b8[4] = {
#include "animal/models/dat_8c17b3b8.inc"
};
static NJS_COLOR dat_8c17b3c0[3] = {
#include "animal/models/dat_8c17b3c0.inc"
};
static NJS_MESHSET dat_8c17b3cc[1] = {
#include "animal/models/dat_8c17b3cc.inc"
};
static NJS_POINT3 dat_8c17b3e4[3] = {
#include "animal/models/dat_8c17b3e4.inc"
};
static NJS_VECTOR dat_8c17b408[3] = {
#include "animal/models/dat_8c17b408.inc"
};
static NJS_MODEL dat_8c17b42c = {
#include "animal/models/dat_8c17b42c.inc"
};
static NJS_OBJECT dat_8c17b454 = {
#include "animal/models/dat_8c17b454.inc"
};
static NJS_OBJECT dat_8c17b488 = {
#include "animal/models/dat_8c17b488.inc"
};
static NJS_OBJECT dat_8c17b4bc = {
#include "animal/models/dat_8c17b4bc.inc"
};
static NJS_OBJECT dat_8c17b4f0 = {
#include "animal/models/dat_8c17b4f0.inc"
};
static NJS_MATERIAL dat_8c17b524[1] = {
#include "animal/models/dat_8c17b524.inc"
};
static Sint16 dat_8c17b538[4] = {
#include "animal/models/dat_8c17b538.inc"
};
static NJS_COLOR dat_8c17b540[3] = {
#include "animal/models/dat_8c17b540.inc"
};
static NJS_MESHSET dat_8c17b54c[1] = {
#include "animal/models/dat_8c17b54c.inc"
};
static NJS_POINT3 dat_8c17b564[3] = {
#include "animal/models/dat_8c17b564.inc"
};
static NJS_VECTOR dat_8c17b588[3] = {
#include "animal/models/dat_8c17b588.inc"
};
static NJS_MODEL dat_8c17b5ac = {
#include "animal/models/dat_8c17b5ac.inc"
};
static NJS_OBJECT dat_8c17b5d4 = {
#include "animal/models/dat_8c17b5d4.inc"
};
static NJS_OBJECT dat_8c17b608 = {
#include "animal/models/dat_8c17b608.inc"
};
static NJS_OBJECT dat_8c17b63c = {
#include "animal/models/dat_8c17b63c.inc"
};
static NJS_OBJECT dat_8c17b670 = {
#include "animal/models/dat_8c17b670.inc"
};
static NJS_MATERIAL dat_8c17b6a4[1] = {
#include "animal/models/dat_8c17b6a4.inc"
};
static Sint16 dat_8c17b6b8[6] = {
#include "animal/models/dat_8c17b6b8.inc"
};
static NJS_COLOR dat_8c17b6c4[5] = {
#include "animal/models/dat_8c17b6c4.inc"
};
static NJS_MESHSET dat_8c17b6d8[1] = {
#include "animal/models/dat_8c17b6d8.inc"
};
static NJS_POINT3 dat_8c17b6f0[5] = {
#include "animal/models/dat_8c17b6f0.inc"
};
static NJS_VECTOR dat_8c17b72c[5] = {
#include "animal/models/dat_8c17b72c.inc"
};
static NJS_MODEL dat_8c17b768 = {
#include "animal/models/dat_8c17b768.inc"
};
static NJS_OBJECT dat_8c17b790 = {
#include "animal/models/dat_8c17b790.inc"
};
static NJS_OBJECT dat_8c17b7c4 = {
#include "animal/models/dat_8c17b7c4.inc"
};
static NJS_OBJECT dat_8c17b7f8 = {
#include "animal/models/dat_8c17b7f8.inc"
};
static NJS_OBJECT dat_8c17b82c = {
#include "animal/models/dat_8c17b82c.inc"
};
static NJS_OBJECT dat_8c17b860 = {
#include "animal/models/dat_8c17b860.inc"
};
static NJS_OBJECT dat_8c17b894 = {
#include "animal/models/dat_8c17b894.inc"
};
static NJS_OBJECT dat_8c17b8c8 = {
#include "animal/models/dat_8c17b8c8.inc"
};
static NJS_OBJECT pen_object = {
#include "animal/models/pen_object.inc"
};
static NJS_MKEY_F dat_8c17b930[2] = {
#include "animal/models/dat_8c17b930.inc"
};
static NJS_MKEY_A dat_8c17b950[2] = {
#include "animal/models/dat_8c17b950.inc"
};
static NJS_MKEY_F dat_8c17b970[17] = {
#include "animal/models/dat_8c17b970.inc"
};
static NJS_MKEY_A dat_8c17ba80[2] = {
#include "animal/models/dat_8c17ba80.inc"
};
static NJS_MKEY_F dat_8c17baa0[2] = {
#include "animal/models/dat_8c17baa0.inc"
};
static NJS_MKEY_A dat_8c17bac0[17] = {
#include "animal/models/dat_8c17bac0.inc"
};
static NJS_MKEY_F dat_8c17bbd0[2] = {
#include "animal/models/dat_8c17bbd0.inc"
};
static NJS_MKEY_A dat_8c17bbf0[2] = {
#include "animal/models/dat_8c17bbf0.inc"
};
static NJS_MKEY_F dat_8c17bc10[2] = {
#include "animal/models/dat_8c17bc10.inc"
};
static NJS_MKEY_A dat_8c17bc30[2] = {
#include "animal/models/dat_8c17bc30.inc"
};
static NJS_MKEY_F dat_8c17bc50[2] = {
#include "animal/models/dat_8c17bc50.inc"
};
static NJS_MKEY_A dat_8c17bc70[17] = {
#include "animal/models/dat_8c17bc70.inc"
};
static NJS_MKEY_F dat_8c17bd80[2] = {
#include "animal/models/dat_8c17bd80.inc"
};
static NJS_MKEY_A dat_8c17bda0[2] = {
#include "animal/models/dat_8c17bda0.inc"
};
static NJS_MKEY_F dat_8c17bdc0[2] = {
#include "animal/models/dat_8c17bdc0.inc"
};
static NJS_MKEY_A dat_8c17bde0[2] = {
#include "animal/models/dat_8c17bde0.inc"
};
static NJS_MKEY_F dat_8c17be00[2] = {
#include "animal/models/dat_8c17be00.inc"
};
static NJS_MKEY_A dat_8c17be20[2] = {
#include "animal/models/dat_8c17be20.inc"
};
static NJS_MKEY_F dat_8c17be40[2] = {
#include "animal/models/dat_8c17be40.inc"
};
static NJS_MKEY_A dat_8c17be60[17] = {
#include "animal/models/dat_8c17be60.inc"
};
static NJS_MKEY_F dat_8c17bf70[2] = {
#include "animal/models/dat_8c17bf70.inc"
};
static NJS_MKEY_A dat_8c17bf90[2] = {
#include "animal/models/dat_8c17bf90.inc"
};
static NJS_MKEY_F dat_8c17bfb0[2] = {
#include "animal/models/dat_8c17bfb0.inc"
};
static NJS_MKEY_A dat_8c17bfd0[2] = {
#include "animal/models/dat_8c17bfd0.inc"
};
static NJS_MKEY_F dat_8c17bff0[2] = {
#include "animal/models/dat_8c17bff0.inc"
};
static NJS_MKEY_A dat_8c17c010[2] = {
#include "animal/models/dat_8c17c010.inc"
};
static NJS_MKEY_F dat_8c17c030[2] = {
#include "animal/models/dat_8c17c030.inc"
};
static NJS_MKEY_A dat_8c17c050[17] = {
#include "animal/models/dat_8c17c050.inc"
};
static NJS_MKEY_F dat_8c17c160[2] = {
#include "animal/models/dat_8c17c160.inc"
};
static NJS_MKEY_A dat_8c17c180[2] = {
#include "animal/models/dat_8c17c180.inc"
};
static NJS_MKEY_F dat_8c17c1a0[2] = {
#include "animal/models/dat_8c17c1a0.inc"
};
static NJS_MKEY_A dat_8c17c1c0[2] = {
#include "animal/models/dat_8c17c1c0.inc"
};
static NJS_MKEY_F dat_8c17c1e0[2] = {
#include "animal/models/dat_8c17c1e0.inc"
};
static NJS_MKEY_A dat_8c17c200[2] = {
#include "animal/models/dat_8c17c200.inc"
};
static NJS_MKEY_F dat_8c17c220[2] = {
#include "animal/models/dat_8c17c220.inc"
};
static NJS_MKEY_A dat_8c17c240[17] = {
#include "animal/models/dat_8c17c240.inc"
};
static NJS_MKEY_F dat_8c17c350[2] = {
#include "animal/models/dat_8c17c350.inc"
};
static NJS_MKEY_A dat_8c17c370[2] = {
#include "animal/models/dat_8c17c370.inc"
};
static NJS_MKEY_F dat_8c17c390[2] = {
#include "animal/models/dat_8c17c390.inc"
};
static NJS_MKEY_A dat_8c17c3b0[2] = {
#include "animal/models/dat_8c17c3b0.inc"
};
static NJS_MKEY_F dat_8c17c3d0[2] = {
#include "animal/models/dat_8c17c3d0.inc"
};
static NJS_MKEY_A dat_8c17c3f0[2] = {
#include "animal/models/dat_8c17c3f0.inc"
};
static NJS_MKEY_F dat_8c17c410[2] = {
#include "animal/models/dat_8c17c410.inc"
};
static NJS_MKEY_A dat_8c17c430[17] = {
#include "animal/models/dat_8c17c430.inc"
};
static NJS_MKEY_F dat_8c17c540[2] = {
#include "animal/models/dat_8c17c540.inc"
};
static NJS_MKEY_A dat_8c17c560[2] = {
#include "animal/models/dat_8c17c560.inc"
};
static NJS_MKEY_F dat_8c17c580[2] = {
#include "animal/models/dat_8c17c580.inc"
};
static NJS_MKEY_A dat_8c17c5a0[2] = {
#include "animal/models/dat_8c17c5a0.inc"
};
static NJS_MKEY_F dat_8c17c5c0[17] = {
#include "animal/models/dat_8c17c5c0.inc"
};
static NJS_MKEY_A dat_8c17c6d0[2] = {
#include "animal/models/dat_8c17c6d0.inc"
};
static NJS_MKEY_F dat_8c17c6f0[2] = {
#include "animal/models/dat_8c17c6f0.inc"
};
static NJS_MKEY_A dat_8c17c710[17] = {
#include "animal/models/dat_8c17c710.inc"
};
static NJS_MKEY_F dat_8c17c820[2] = {
#include "animal/models/dat_8c17c820.inc"
};
static NJS_MKEY_A dat_8c17c840[17] = {
#include "animal/models/dat_8c17c840.inc"
};
static NJS_MKEY_F dat_8c17c950[2] = {
#include "animal/models/dat_8c17c950.inc"
};
static NJS_MKEY_A dat_8c17c970[2] = {
#include "animal/models/dat_8c17c970.inc"
};
static NJS_MKEY_F dat_8c17c990[2] = {
#include "animal/models/dat_8c17c990.inc"
};
static NJS_MKEY_A dat_8c17c9b0[2] = {
#include "animal/models/dat_8c17c9b0.inc"
};
static NJS_MKEY_F dat_8c17c9d0[17] = {
#include "animal/models/dat_8c17c9d0.inc"
};
static NJS_MKEY_A dat_8c17cae0[2] = {
#include "animal/models/dat_8c17cae0.inc"
};
static NJS_MKEY_F dat_8c17cb00[2] = {
#include "animal/models/dat_8c17cb00.inc"
};
static NJS_MKEY_A dat_8c17cb20[17] = {
#include "animal/models/dat_8c17cb20.inc"
};
static NJS_MKEY_F dat_8c17cc30[2] = {
#include "animal/models/dat_8c17cc30.inc"
};
static NJS_MKEY_A dat_8c17cc50[17] = {
#include "animal/models/dat_8c17cc50.inc"
};
static NJS_MKEY_F dat_8c17cd60[2] = {
#include "animal/models/dat_8c17cd60.inc"
};
static NJS_MKEY_A dat_8c17cd80[2] = {
#include "animal/models/dat_8c17cd80.inc"
};
static NJS_MKEY_F dat_8c17cda0[2] = {
#include "animal/models/dat_8c17cda0.inc"
};
static NJS_MKEY_A dat_8c17cdc0[2] = {
#include "animal/models/dat_8c17cdc0.inc"
};
static NJS_MKEY_F dat_8c17cde0[2] = {
#include "animal/models/dat_8c17cde0.inc"
};
static NJS_MKEY_A dat_8c17ce00[2] = {
#include "animal/models/dat_8c17ce00.inc"
};
static NJS_MKEY_F dat_8c17ce20[2] = {
#include "animal/models/dat_8c17ce20.inc"
};
static NJS_MKEY_A dat_8c17ce40[2] = {
#include "animal/models/dat_8c17ce40.inc"
};
static NJS_MKEY_F dat_8c17ce60[2] = {
#include "animal/models/dat_8c17ce60.inc"
};
static NJS_MKEY_A dat_8c17ce80[2] = {
#include "animal/models/dat_8c17ce80.inc"
};
static NJS_MDATA2 dat_8c17cea0[37] = {
#include "animal/models/dat_8c17cea0.inc"
};
static NJS_MOTION pen_motion = {
#include "animal/models/pen_motion.inc"
};
NJS_ACTION pen_action = {
#include "animal/models/pen_action.inc"
};
static NJS_TEXNAME pen_textures[5] = {
    {TEXT_6, 0, 0}, {TEXT_7, 0, 0},  {TEXT_8, 0, 0},
    {TEXT_9, 0, 0}, {TEXT_10, 0, 0},
};
NJS_TEXLIST pen_texlist = {
#include "animal/models/pen_texlist.inc"
};
static NJS_MATERIAL dat_8c17d148[1] = {
#include "animal/models/dat_8c17d148.inc"
};
static Sint16 dat_8c17d15c[37] = {
#include "animal/models/dat_8c17d15c.inc"
};
static NJS_COLOR dat_8c17d1a8[33] = {
#include "animal/models/dat_8c17d1a8.inc"
};
static NJS_MESHSET dat_8c17d22c[1] = {
#include "animal/models/dat_8c17d22c.inc"
};
static NJS_POINT3 dat_8c17d244[15] = {
#include "animal/models/dat_8c17d244.inc"
};
static NJS_VECTOR dat_8c17d2f8[15] = {
#include "animal/models/dat_8c17d2f8.inc"
};
static NJS_MODEL dat_8c17d3ac = {
#include "animal/models/dat_8c17d3ac.inc"
};
static NJS_OBJECT dat_8c17d3d4 = {
#include "animal/models/dat_8c17d3d4.inc"
};
static NJS_MATERIAL dat_8c17d408[1] = {
#include "animal/models/dat_8c17d408.inc"
};
static Sint16 dat_8c17d41c[4] = {
#include "animal/models/dat_8c17d41c.inc"
};
static NJS_COLOR dat_8c17d424[3] = {
#include "animal/models/dat_8c17d424.inc"
};
static NJS_MESHSET dat_8c17d430[1] = {
#include "animal/models/dat_8c17d430.inc"
};
static NJS_POINT3 dat_8c17d448[3] = {
#include "animal/models/dat_8c17d448.inc"
};
static NJS_VECTOR dat_8c17d46c[3] = {
#include "animal/models/dat_8c17d46c.inc"
};
static NJS_MODEL dat_8c17d490 = {
#include "animal/models/dat_8c17d490.inc"
};
static NJS_OBJECT dat_8c17d4b8 = {
#include "animal/models/dat_8c17d4b8.inc"
};
static NJS_OBJECT dat_8c17d4ec = {
#include "animal/models/dat_8c17d4ec.inc"
};
static NJS_OBJECT dat_8c17d520 = {
#include "animal/models/dat_8c17d520.inc"
};
static NJS_OBJECT dat_8c17d554 = {
#include "animal/models/dat_8c17d554.inc"
};
static NJS_MATERIAL dat_8c17d588[1] = {
#include "animal/models/dat_8c17d588.inc"
};
static Sint16 dat_8c17d59c[11] = {
#include "animal/models/dat_8c17d59c.inc"
};
static NJS_COLOR dat_8c17d5b4[9] = {
#include "animal/models/dat_8c17d5b4.inc"
};
static NJS_MESHSET dat_8c17d5d8[1] = {
#include "animal/models/dat_8c17d5d8.inc"
};
static NJS_POINT3 dat_8c17d5f0[7] = {
#include "animal/models/dat_8c17d5f0.inc"
};
static NJS_VECTOR dat_8c17d644[7] = {
#include "animal/models/dat_8c17d644.inc"
};
static NJS_MODEL dat_8c17d698 = {
#include "animal/models/dat_8c17d698.inc"
};
static NJS_OBJECT dat_8c17d6c0 = {
#include "animal/models/dat_8c17d6c0.inc"
};
static NJS_OBJECT dat_8c17d6f4 = {
#include "animal/models/dat_8c17d6f4.inc"
};
static NJS_OBJECT dat_8c17d728 = {
#include "animal/models/dat_8c17d728.inc"
};
static NJS_OBJECT dat_8c17d75c = {
#include "animal/models/dat_8c17d75c.inc"
};
static NJS_MATERIAL dat_8c17d790[1] = {
#include "animal/models/dat_8c17d790.inc"
};
static Sint16 dat_8c17d7a4[11] = {
#include "animal/models/dat_8c17d7a4.inc"
};
static NJS_COLOR dat_8c17d7bc[9] = {
#include "animal/models/dat_8c17d7bc.inc"
};
static NJS_MESHSET dat_8c17d7e0[1] = {
#include "animal/models/dat_8c17d7e0.inc"
};
static NJS_POINT3 dat_8c17d7f8[7] = {
#include "animal/models/dat_8c17d7f8.inc"
};
static NJS_VECTOR dat_8c17d84c[7] = {
#include "animal/models/dat_8c17d84c.inc"
};
static NJS_MODEL dat_8c17d8a0 = {
#include "animal/models/dat_8c17d8a0.inc"
};
static NJS_OBJECT dat_8c17d8c8 = {
#include "animal/models/dat_8c17d8c8.inc"
};
static NJS_OBJECT dat_8c17d8fc = {
#include "animal/models/dat_8c17d8fc.inc"
};
static NJS_OBJECT dat_8c17d930 = {
#include "animal/models/dat_8c17d930.inc"
};
static NJS_OBJECT dat_8c17d964 = {
#include "animal/models/dat_8c17d964.inc"
};
static NJS_MATERIAL dat_8c17d998[1] = {
#include "animal/models/dat_8c17d998.inc"
};
static Sint16 dat_8c17d9ac[11] = {
#include "animal/models/dat_8c17d9ac.inc"
};
static NJS_COLOR dat_8c17d9c4[9] = {
#include "animal/models/dat_8c17d9c4.inc"
};
static NJS_MESHSET dat_8c17d9e8[1] = {
#include "animal/models/dat_8c17d9e8.inc"
};
static NJS_POINT3 dat_8c17da00[6] = {
#include "animal/models/dat_8c17da00.inc"
};
static NJS_VECTOR dat_8c17da48[6] = {
#include "animal/models/dat_8c17da48.inc"
};
static NJS_MODEL dat_8c17da90 = {
#include "animal/models/dat_8c17da90.inc"
};
static NJS_OBJECT dat_8c17dab8 = {
#include "animal/models/dat_8c17dab8.inc"
};
static NJS_MATERIAL dat_8c17daec[1] = {
#include "animal/models/dat_8c17daec.inc"
};
static Sint16 dat_8c17db00[6] = {
#include "animal/models/dat_8c17db00.inc"
};
static NJS_COLOR dat_8c17db0c[5] = {
#include "animal/models/dat_8c17db0c.inc"
};
static NJS_MESHSET dat_8c17db20[1] = {
#include "animal/models/dat_8c17db20.inc"
};
static NJS_POINT3 dat_8c17db38[5] = {
#include "animal/models/dat_8c17db38.inc"
};
static NJS_VECTOR dat_8c17db74[5] = {
#include "animal/models/dat_8c17db74.inc"
};
static NJS_MODEL dat_8c17dbb0 = {
#include "animal/models/dat_8c17dbb0.inc"
};
static NJS_OBJECT dat_8c17dbd8 = {
#include "animal/models/dat_8c17dbd8.inc"
};
static NJS_OBJECT dat_8c17dc0c = {
#include "animal/models/dat_8c17dc0c.inc"
};
static NJS_OBJECT dat_8c17dc40 = {
#include "animal/models/dat_8c17dc40.inc"
};
static NJS_OBJECT dat_8c17dc74 = {
#include "animal/models/dat_8c17dc74.inc"
};
static NJS_OBJECT dat_8c17dca8 = {
#include "animal/models/dat_8c17dca8.inc"
};
static NJS_MATERIAL dat_8c17dcdc[1] = {
#include "animal/models/dat_8c17dcdc.inc"
};
static Sint16 dat_8c17dcf0[11] = {
#include "animal/models/dat_8c17dcf0.inc"
};
static NJS_COLOR dat_8c17dd08[9] = {
#include "animal/models/dat_8c17dd08.inc"
};
static NJS_MESHSET dat_8c17dd2c[1] = {
#include "animal/models/dat_8c17dd2c.inc"
};
static NJS_POINT3 dat_8c17dd44[6] = {
#include "animal/models/dat_8c17dd44.inc"
};
static NJS_VECTOR dat_8c17dd8c[6] = {
#include "animal/models/dat_8c17dd8c.inc"
};
static NJS_MODEL dat_8c17ddd4 = {
#include "animal/models/dat_8c17ddd4.inc"
};
static NJS_OBJECT dat_8c17ddfc = {
#include "animal/models/dat_8c17ddfc.inc"
};
static NJS_MATERIAL dat_8c17de30[1] = {
#include "animal/models/dat_8c17de30.inc"
};
static Sint16 dat_8c17de44[6] = {
#include "animal/models/dat_8c17de44.inc"
};
static NJS_COLOR dat_8c17de50[5] = {
#include "animal/models/dat_8c17de50.inc"
};
static NJS_MESHSET dat_8c17de64[1] = {
#include "animal/models/dat_8c17de64.inc"
};
static NJS_POINT3 dat_8c17de7c[5] = {
#include "animal/models/dat_8c17de7c.inc"
};
static NJS_VECTOR dat_8c17deb8[5] = {
#include "animal/models/dat_8c17deb8.inc"
};
static NJS_MODEL dat_8c17def4 = {
#include "animal/models/dat_8c17def4.inc"
};
static NJS_OBJECT dat_8c17df1c = {
#include "animal/models/dat_8c17df1c.inc"
};
static NJS_OBJECT dat_8c17df50 = {
#include "animal/models/dat_8c17df50.inc"
};
static NJS_OBJECT dat_8c17df84 = {
#include "animal/models/dat_8c17df84.inc"
};
static NJS_OBJECT dat_8c17dfb8 = {
#include "animal/models/dat_8c17dfb8.inc"
};
static NJS_OBJECT dat_8c17dfec = {
#include "animal/models/dat_8c17dfec.inc"
};
static NJS_MATERIAL dat_8c17e020[1] = {
#include "animal/models/dat_8c17e020.inc"
};
static Sint16 dat_8c17e034[5] = {
#include "animal/models/dat_8c17e034.inc"
};
static NJS_COLOR dat_8c17e040[4] = {
#include "animal/models/dat_8c17e040.inc"
};
static NJS_MESHSET dat_8c17e050[1] = {
#include "animal/models/dat_8c17e050.inc"
};
static NJS_POINT3 dat_8c17e068[4] = {
#include "animal/models/dat_8c17e068.inc"
};
static NJS_VECTOR dat_8c17e098[4] = {
#include "animal/models/dat_8c17e098.inc"
};
static NJS_MODEL dat_8c17e0c8 = {
#include "animal/models/dat_8c17e0c8.inc"
};
static NJS_OBJECT dat_8c17e0f0 = {
#include "animal/models/dat_8c17e0f0.inc"
};
static NJS_MATERIAL dat_8c17e124[1] = {
#include "animal/models/dat_8c17e124.inc"
};
static Sint16 dat_8c17e138[5] = {
#include "animal/models/dat_8c17e138.inc"
};
static NJS_COLOR dat_8c17e144[4] = {
#include "animal/models/dat_8c17e144.inc"
};
static NJS_MESHSET dat_8c17e154[1] = {
#include "animal/models/dat_8c17e154.inc"
};
static NJS_POINT3 dat_8c17e16c[4] = {
#include "animal/models/dat_8c17e16c.inc"
};
static NJS_VECTOR dat_8c17e19c[4] = {
#include "animal/models/dat_8c17e19c.inc"
};
static NJS_MODEL dat_8c17e1cc = {
#include "animal/models/dat_8c17e1cc.inc"
};
static NJS_OBJECT dat_8c17e1f4 = {
#include "animal/models/dat_8c17e1f4.inc"
};
static NJS_MATERIAL dat_8c17e228[1] = {
#include "animal/models/dat_8c17e228.inc"
};
static Sint16 dat_8c17e23c[82] = {
#include "animal/models/dat_8c17e23c.inc"
};
static NJS_COLOR dat_8c17e2e0[66] = {
#include "animal/models/dat_8c17e2e0.inc"
};
static NJS_MESHSET dat_8c17e3e8[1] = {
#include "animal/models/dat_8c17e3e8.inc"
};
static NJS_POINT3 dat_8c17e400[19] = {
#include "animal/models/dat_8c17e400.inc"
};
static NJS_VECTOR dat_8c17e4e4[19] = {
#include "animal/models/dat_8c17e4e4.inc"
};
static NJS_MODEL dat_8c17e5c8 = {
#include "animal/models/dat_8c17e5c8.inc"
};
static NJS_OBJECT dat_8c17e5f0 = {
#include "animal/models/dat_8c17e5f0.inc"
};
static NJS_OBJECT dat_8c17e624 = {
#include "animal/models/dat_8c17e624.inc"
};
static NJS_OBJECT dat_8c17e658 = {
#include "animal/models/dat_8c17e658.inc"
};
static NJS_OBJECT dat_8c17e68c = {
#include "animal/models/dat_8c17e68c.inc"
};
static NJS_OBJECT dat_8c17e6c0 = {
#include "animal/models/dat_8c17e6c0.inc"
};
static NJS_OBJECT dat_8c17e6f4 = {
#include "animal/models/dat_8c17e6f4.inc"
};
static NJS_OBJECT dat_8c17e728 = {
#include "animal/models/dat_8c17e728.inc"
};
static NJS_OBJECT tuba_object = {
#include "animal/models/tuba_object.inc"
};
static NJS_MKEY_F dat_8c17e790[2] = {
#include "animal/models/dat_8c17e790.inc"
};
static NJS_MKEY_A dat_8c17e7b0[2] = {
#include "animal/models/dat_8c17e7b0.inc"
};
static NJS_MKEY_F dat_8c17e7d0[2] = {
#include "animal/models/dat_8c17e7d0.inc"
};
static NJS_MKEY_A dat_8c17e7f0[2] = {
#include "animal/models/dat_8c17e7f0.inc"
};
static NJS_MKEY_F dat_8c17e810[2] = {
#include "animal/models/dat_8c17e810.inc"
};
static NJS_MKEY_A dat_8c17e830[2] = {
#include "animal/models/dat_8c17e830.inc"
};
static NJS_MKEY_F dat_8c17e850[2] = {
#include "animal/models/dat_8c17e850.inc"
};
static NJS_MKEY_A dat_8c17e870[2] = {
#include "animal/models/dat_8c17e870.inc"
};
static NJS_MKEY_F dat_8c17e890[2] = {
#include "animal/models/dat_8c17e890.inc"
};
static NJS_MKEY_A dat_8c17e8b0[2] = {
#include "animal/models/dat_8c17e8b0.inc"
};
static NJS_MKEY_F dat_8c17e8d0[2] = {
#include "animal/models/dat_8c17e8d0.inc"
};
static NJS_MKEY_A dat_8c17e8f0[2] = {
#include "animal/models/dat_8c17e8f0.inc"
};
static NJS_MKEY_F dat_8c17e910[2] = {
#include "animal/models/dat_8c17e910.inc"
};
static NJS_MKEY_A dat_8c17e930[2] = {
#include "animal/models/dat_8c17e930.inc"
};
static NJS_MKEY_F dat_8c17e950[2] = {
#include "animal/models/dat_8c17e950.inc"
};
static NJS_MKEY_A dat_8c17e970[2] = {
#include "animal/models/dat_8c17e970.inc"
};
static NJS_MKEY_F dat_8c17e990[2] = {
#include "animal/models/dat_8c17e990.inc"
};
static NJS_MKEY_A dat_8c17e9b0[2] = {
#include "animal/models/dat_8c17e9b0.inc"
};
static NJS_MKEY_F dat_8c17e9d0[2] = {
#include "animal/models/dat_8c17e9d0.inc"
};
static NJS_MKEY_A dat_8c17e9f0[2] = {
#include "animal/models/dat_8c17e9f0.inc"
};
static NJS_MKEY_F dat_8c17ea10[2] = {
#include "animal/models/dat_8c17ea10.inc"
};
static NJS_MKEY_A dat_8c17ea30[2] = {
#include "animal/models/dat_8c17ea30.inc"
};
static NJS_MKEY_F dat_8c17ea50[2] = {
#include "animal/models/dat_8c17ea50.inc"
};
static NJS_MKEY_A dat_8c17ea70[17] = {
#include "animal/models/dat_8c17ea70.inc"
};
static NJS_MKEY_F dat_8c17eb80[2] = {
#include "animal/models/dat_8c17eb80.inc"
};
static NJS_MKEY_A dat_8c17eba0[17] = {
#include "animal/models/dat_8c17eba0.inc"
};
static NJS_MKEY_F dat_8c17ecb0[2] = {
#include "animal/models/dat_8c17ecb0.inc"
};
static NJS_MKEY_A dat_8c17ecd0[2] = {
#include "animal/models/dat_8c17ecd0.inc"
};
static NJS_MKEY_F dat_8c17ecf0[2] = {
#include "animal/models/dat_8c17ecf0.inc"
};
static NJS_MKEY_A dat_8c17ed10[2] = {
#include "animal/models/dat_8c17ed10.inc"
};
static NJS_MKEY_F dat_8c17ed30[2] = {
#include "animal/models/dat_8c17ed30.inc"
};
static NJS_MKEY_A dat_8c17ed50[2] = {
#include "animal/models/dat_8c17ed50.inc"
};
static NJS_MKEY_F dat_8c17ed70[2] = {
#include "animal/models/dat_8c17ed70.inc"
};
static NJS_MKEY_A dat_8c17ed90[2] = {
#include "animal/models/dat_8c17ed90.inc"
};
static NJS_MKEY_F dat_8c17edb0[2] = {
#include "animal/models/dat_8c17edb0.inc"
};
static NJS_MKEY_A dat_8c17edd0[17] = {
#include "animal/models/dat_8c17edd0.inc"
};
static NJS_MKEY_F dat_8c17eee0[2] = {
#include "animal/models/dat_8c17eee0.inc"
};
static NJS_MKEY_A dat_8c17ef00[17] = {
#include "animal/models/dat_8c17ef00.inc"
};
static NJS_MKEY_F dat_8c17f010[2] = {
#include "animal/models/dat_8c17f010.inc"
};
static NJS_MKEY_A dat_8c17f030[2] = {
#include "animal/models/dat_8c17f030.inc"
};
static NJS_MKEY_F dat_8c17f050[2] = {
#include "animal/models/dat_8c17f050.inc"
};
static NJS_MKEY_A dat_8c17f070[2] = {
#include "animal/models/dat_8c17f070.inc"
};
static NJS_MKEY_F dat_8c17f090[2] = {
#include "animal/models/dat_8c17f090.inc"
};
static NJS_MKEY_A dat_8c17f0b0[2] = {
#include "animal/models/dat_8c17f0b0.inc"
};
static NJS_MKEY_F dat_8c17f0d0[2] = {
#include "animal/models/dat_8c17f0d0.inc"
};
static NJS_MKEY_A dat_8c17f0f0[2] = {
#include "animal/models/dat_8c17f0f0.inc"
};
static NJS_MKEY_F dat_8c17f110[2] = {
#include "animal/models/dat_8c17f110.inc"
};
static NJS_MKEY_A dat_8c17f130[2] = {
#include "animal/models/dat_8c17f130.inc"
};
static NJS_MKEY_F dat_8c17f150[2] = {
#include "animal/models/dat_8c17f150.inc"
};
static NJS_MKEY_A dat_8c17f170[2] = {
#include "animal/models/dat_8c17f170.inc"
};
static NJS_MKEY_F dat_8c17f190[2] = {
#include "animal/models/dat_8c17f190.inc"
};
static NJS_MKEY_A dat_8c17f1b0[2] = {
#include "animal/models/dat_8c17f1b0.inc"
};
static NJS_MKEY_F dat_8c17f1d0[2] = {
#include "animal/models/dat_8c17f1d0.inc"
};
static NJS_MKEY_A dat_8c17f1f0[2] = {
#include "animal/models/dat_8c17f1f0.inc"
};
static NJS_MKEY_F dat_8c17f210[2] = {
#include "animal/models/dat_8c17f210.inc"
};
static NJS_MKEY_A dat_8c17f230[2] = {
#include "animal/models/dat_8c17f230.inc"
};
static NJS_MKEY_F dat_8c17f250[2] = {
#include "animal/models/dat_8c17f250.inc"
};
static NJS_MKEY_A dat_8c17f270[2] = {
#include "animal/models/dat_8c17f270.inc"
};
static NJS_MKEY_F dat_8c17f290[2] = {
#include "animal/models/dat_8c17f290.inc"
};
static NJS_MKEY_A dat_8c17f2b0[2] = {
#include "animal/models/dat_8c17f2b0.inc"
};
static NJS_MKEY_F dat_8c17f2d0[2] = {
#include "animal/models/dat_8c17f2d0.inc"
};
static NJS_MKEY_A dat_8c17f2f0[2] = {
#include "animal/models/dat_8c17f2f0.inc"
};
static NJS_MKEY_F dat_8c17f310[2] = {
#include "animal/models/dat_8c17f310.inc"
};
static NJS_MKEY_A dat_8c17f330[17] = {
#include "animal/models/dat_8c17f330.inc"
};
static NJS_MKEY_F dat_8c17f440[2] = {
#include "animal/models/dat_8c17f440.inc"
};
static NJS_MKEY_A dat_8c17f460[2] = {
#include "animal/models/dat_8c17f460.inc"
};
static NJS_MKEY_F dat_8c17f480[2] = {
#include "animal/models/dat_8c17f480.inc"
};
static NJS_MKEY_A dat_8c17f4a0[2] = {
#include "animal/models/dat_8c17f4a0.inc"
};
static NJS_MKEY_F dat_8c17f4c0[2] = {
#include "animal/models/dat_8c17f4c0.inc"
};
static NJS_MKEY_A dat_8c17f4e0[2] = {
#include "animal/models/dat_8c17f4e0.inc"
};
static NJS_MDATA2 dat_8c17f500[35] = {
#include "animal/models/dat_8c17f500.inc"
};
static NJS_MOTION tuba_motion = {
#include "animal/models/tuba_motion.inc"
};
NJS_ACTION tuba_action = {
#include "animal/models/tuba_action.inc"
};
static NJS_TEXNAME tuba_textures[5] = {
    {TEXT_11, 0, 0}, {TEXT_12, 0, 0}, {TEXT_13, 0, 0},
    {TEXT_14, 0, 0}, {TEXT_15, 0, 0},
};
NJS_TEXLIST tuba_texlist = {
#include "animal/models/tuba_texlist.inc"
};
static NJS_MATERIAL dat_8c17f788[1] = {
#include "animal/models/dat_8c17f788.inc"
};
static Sint16 dat_8c17f79c[8] = {
#include "animal/models/dat_8c17f79c.inc"
};
static NJS_COLOR dat_8c17f7ac[7] = {
#include "animal/models/dat_8c17f7ac.inc"
};
static NJS_MESHSET dat_8c17f7c8[1] = {
#include "animal/models/dat_8c17f7c8.inc"
};
static NJS_POINT3 dat_8c17f7e0[7] = {
#include "animal/models/dat_8c17f7e0.inc"
};
static NJS_VECTOR dat_8c17f834[7] = {
#include "animal/models/dat_8c17f834.inc"
};
static NJS_MODEL dat_8c17f888 = {
#include "animal/models/dat_8c17f888.inc"
};
static NJS_OBJECT dat_8c17f8b0 = {
#include "animal/models/dat_8c17f8b0.inc"
};
static NJS_OBJECT dat_8c17f8e4 = {
#include "animal/models/dat_8c17f8e4.inc"
};
static NJS_OBJECT dat_8c17f918 = {
#include "animal/models/dat_8c17f918.inc"
};
static NJS_OBJECT dat_8c17f94c = {
#include "animal/models/dat_8c17f94c.inc"
};
static NJS_MATERIAL dat_8c17f980[1] = {
#include "animal/models/dat_8c17f980.inc"
};
static Sint16 dat_8c17f994[11] = {
#include "animal/models/dat_8c17f994.inc"
};
static NJS_COLOR dat_8c17f9ac[9] = {
#include "animal/models/dat_8c17f9ac.inc"
};
static NJS_MESHSET dat_8c17f9d0[1] = {
#include "animal/models/dat_8c17f9d0.inc"
};
static NJS_POINT3 dat_8c17f9e8[7] = {
#include "animal/models/dat_8c17f9e8.inc"
};
static NJS_VECTOR dat_8c17fa3c[7] = {
#include "animal/models/dat_8c17fa3c.inc"
};
static NJS_MODEL dat_8c17fa90 = {
#include "animal/models/dat_8c17fa90.inc"
};
static NJS_OBJECT dat_8c17fab8 = {
#include "animal/models/dat_8c17fab8.inc"
};
static NJS_OBJECT dat_8c17faec = {
#include "animal/models/dat_8c17faec.inc"
};
static NJS_OBJECT dat_8c17fb20 = {
#include "animal/models/dat_8c17fb20.inc"
};
static NJS_OBJECT dat_8c17fb54 = {
#include "animal/models/dat_8c17fb54.inc"
};
static NJS_MATERIAL dat_8c17fb88[1] = {
#include "animal/models/dat_8c17fb88.inc"
};
static Sint16 dat_8c17fb9c[11] = {
#include "animal/models/dat_8c17fb9c.inc"
};
static NJS_COLOR dat_8c17fbb4[9] = {
#include "animal/models/dat_8c17fbb4.inc"
};
static NJS_MESHSET dat_8c17fbd8[1] = {
#include "animal/models/dat_8c17fbd8.inc"
};
static NJS_POINT3 dat_8c17fbf0[7] = {
#include "animal/models/dat_8c17fbf0.inc"
};
static NJS_VECTOR dat_8c17fc44[7] = {
#include "animal/models/dat_8c17fc44.inc"
};
static NJS_MODEL dat_8c17fc98 = {
#include "animal/models/dat_8c17fc98.inc"
};
static NJS_OBJECT dat_8c17fcc0 = {
#include "animal/models/dat_8c17fcc0.inc"
};
static NJS_OBJECT dat_8c17fcf4 = {
#include "animal/models/dat_8c17fcf4.inc"
};
static NJS_OBJECT dat_8c17fd28 = {
#include "animal/models/dat_8c17fd28.inc"
};
static NJS_OBJECT dat_8c17fd5c = {
#include "animal/models/dat_8c17fd5c.inc"
};
static NJS_MATERIAL dat_8c17fd90[1] = {
#include "animal/models/dat_8c17fd90.inc"
};
static Sint16 dat_8c17fda4[18] = {
#include "animal/models/dat_8c17fda4.inc"
};
static NJS_COLOR dat_8c17fdc8[15] = {
#include "animal/models/dat_8c17fdc8.inc"
};
static NJS_MESHSET dat_8c17fe04[1] = {
#include "animal/models/dat_8c17fe04.inc"
};
static NJS_POINT3 dat_8c17fe1c[9] = {
#include "animal/models/dat_8c17fe1c.inc"
};
static NJS_VECTOR dat_8c17fe88[9] = {
#include "animal/models/dat_8c17fe88.inc"
};
static NJS_MODEL dat_8c17fef4 = {
#include "animal/models/dat_8c17fef4.inc"
};
static NJS_OBJECT dat_8c17ff1c = {
#include "animal/models/dat_8c17ff1c.inc"
};
static NJS_MATERIAL dat_8c17ff50[1] = {
#include "animal/models/dat_8c17ff50.inc"
};
static Sint16 dat_8c17ff64[6] = {
#include "animal/models/dat_8c17ff64.inc"
};
static NJS_COLOR dat_8c17ff70[5] = {
#include "animal/models/dat_8c17ff70.inc"
};
static NJS_MESHSET dat_8c17ff84[1] = {
#include "animal/models/dat_8c17ff84.inc"
};
static NJS_POINT3 dat_8c17ff9c[5] = {
#include "animal/models/dat_8c17ff9c.inc"
};
static NJS_VECTOR dat_8c17ffd8[5] = {
#include "animal/models/dat_8c17ffd8.inc"
};
static NJS_MODEL dat_8c180014 = {
#include "animal/models/dat_8c180014.inc"
};
static NJS_OBJECT dat_8c18003c = {
#include "animal/models/dat_8c18003c.inc"
};
static NJS_OBJECT dat_8c180070 = {
#include "animal/models/dat_8c180070.inc"
};
static NJS_OBJECT dat_8c1800a4 = {
#include "animal/models/dat_8c1800a4.inc"
};
static NJS_OBJECT dat_8c1800d8 = {
#include "animal/models/dat_8c1800d8.inc"
};
static NJS_OBJECT dat_8c18010c = {
#include "animal/models/dat_8c18010c.inc"
};
static NJS_MATERIAL dat_8c180140[1] = {
#include "animal/models/dat_8c180140.inc"
};
static Sint16 dat_8c180154[18] = {
#include "animal/models/dat_8c180154.inc"
};
static NJS_COLOR dat_8c180178[15] = {
#include "animal/models/dat_8c180178.inc"
};
static NJS_MESHSET dat_8c1801b4[1] = {
#include "animal/models/dat_8c1801b4.inc"
};
static NJS_POINT3 dat_8c1801cc[9] = {
#include "animal/models/dat_8c1801cc.inc"
};
static NJS_VECTOR dat_8c180238[9] = {
#include "animal/models/dat_8c180238.inc"
};
static NJS_MODEL dat_8c1802a4 = {
#include "animal/models/dat_8c1802a4.inc"
};
static NJS_OBJECT dat_8c1802cc = {
#include "animal/models/dat_8c1802cc.inc"
};
static NJS_MATERIAL dat_8c180300[1] = {
#include "animal/models/dat_8c180300.inc"
};
static Sint16 dat_8c180314[6] = {
#include "animal/models/dat_8c180314.inc"
};
static NJS_COLOR dat_8c180320[5] = {
#include "animal/models/dat_8c180320.inc"
};
static NJS_MESHSET dat_8c180334[1] = {
#include "animal/models/dat_8c180334.inc"
};
static NJS_POINT3 dat_8c18034c[5] = {
#include "animal/models/dat_8c18034c.inc"
};
static NJS_VECTOR dat_8c180388[5] = {
#include "animal/models/dat_8c180388.inc"
};
static NJS_MODEL dat_8c1803c4 = {
#include "animal/models/dat_8c1803c4.inc"
};
static NJS_OBJECT dat_8c1803ec = {
#include "animal/models/dat_8c1803ec.inc"
};
static NJS_OBJECT dat_8c180420 = {
#include "animal/models/dat_8c180420.inc"
};
static NJS_OBJECT dat_8c180454 = {
#include "animal/models/dat_8c180454.inc"
};
static NJS_OBJECT dat_8c180488 = {
#include "animal/models/dat_8c180488.inc"
};
static NJS_OBJECT dat_8c1804bc = {
#include "animal/models/dat_8c1804bc.inc"
};
static NJS_MATERIAL dat_8c1804f0[1] = {
#include "animal/models/dat_8c1804f0.inc"
};
static Sint16 dat_8c180504[4] = {
#include "animal/models/dat_8c180504.inc"
};
static NJS_COLOR dat_8c18050c[3] = {
#include "animal/models/dat_8c18050c.inc"
};
static NJS_MESHSET dat_8c180518[1] = {
#include "animal/models/dat_8c180518.inc"
};
static NJS_POINT3 dat_8c180530[3] = {
#include "animal/models/dat_8c180530.inc"
};
static NJS_VECTOR dat_8c180554[3] = {
#include "animal/models/dat_8c180554.inc"
};
static NJS_MODEL dat_8c180578 = {
#include "animal/models/dat_8c180578.inc"
};
static NJS_OBJECT dat_8c1805a0 = {
#include "animal/models/dat_8c1805a0.inc"
};
static NJS_MATERIAL dat_8c1805d4[1] = {
#include "animal/models/dat_8c1805d4.inc"
};
static Sint16 dat_8c1805e8[47] = {
#include "animal/models/dat_8c1805e8.inc"
};
static NJS_COLOR dat_8c180648[40] = {
#include "animal/models/dat_8c180648.inc"
};
static NJS_MESHSET dat_8c1806e8[1] = {
#include "animal/models/dat_8c1806e8.inc"
};
static NJS_POINT3 dat_8c180700[15] = {
#include "animal/models/dat_8c180700.inc"
};
static NJS_VECTOR dat_8c1807b4[15] = {
#include "animal/models/dat_8c1807b4.inc"
};
static NJS_MODEL dat_8c180868 = {
#include "animal/models/dat_8c180868.inc"
};
static NJS_OBJECT dat_8c180890 = {
#include "animal/models/dat_8c180890.inc"
};
static NJS_OBJECT dat_8c1808c4 = {
#include "animal/models/dat_8c1808c4.inc"
};
static NJS_OBJECT dat_8c1808f8 = {
#include "animal/models/dat_8c1808f8.inc"
};
static NJS_OBJECT dat_8c18092c = {
#include "animal/models/dat_8c18092c.inc"
};
static NJS_MATERIAL dat_8c180960[1] = {
#include "animal/models/dat_8c180960.inc"
};
static Sint16 dat_8c180974[45] = {
#include "animal/models/dat_8c180974.inc"
};
static NJS_COLOR dat_8c1809d0[40] = {
#include "animal/models/dat_8c1809d0.inc"
};
static NJS_MESHSET dat_8c180a70[1] = {
#include "animal/models/dat_8c180a70.inc"
};
static NJS_POINT3 dat_8c180a88[17] = {
#include "animal/models/dat_8c180a88.inc"
};
static NJS_VECTOR dat_8c180b54[17] = {
#include "animal/models/dat_8c180b54.inc"
};
static NJS_MODEL dat_8c180c20 = {
#include "animal/models/dat_8c180c20.inc"
};
static NJS_OBJECT dat_8c180c48 = {
#include "animal/models/dat_8c180c48.inc"
};
static NJS_OBJECT dat_8c180c7c = {
#include "animal/models/dat_8c180c7c.inc"
};
static NJS_OBJECT dat_8c180cb0 = {
#include "animal/models/dat_8c180cb0.inc"
};
static NJS_OBJECT dat_8c180ce4 = {
#include "animal/models/dat_8c180ce4.inc"
};
static NJS_OBJECT kuja_object = {
#include "animal/models/kuja_object.inc"
};
static NJS_MKEY_F dat_8c180d4c[2] = {
#include "animal/models/dat_8c180d4c.inc"
};
static NJS_MKEY_A dat_8c180d6c[2] = {
#include "animal/models/dat_8c180d6c.inc"
};
static NJS_MKEY_F dat_8c180d8c[17] = {
#include "animal/models/dat_8c180d8c.inc"
};
static NJS_MKEY_A dat_8c180e9c[2] = {
#include "animal/models/dat_8c180e9c.inc"
};
static NJS_MKEY_F dat_8c180ebc[2] = {
#include "animal/models/dat_8c180ebc.inc"
};
static NJS_MKEY_A dat_8c180edc[17] = {
#include "animal/models/dat_8c180edc.inc"
};
static NJS_MKEY_F dat_8c180fec[2] = {
#include "animal/models/dat_8c180fec.inc"
};
static NJS_MKEY_A dat_8c18100c[2] = {
#include "animal/models/dat_8c18100c.inc"
};
static NJS_MKEY_F dat_8c18102c[2] = {
#include "animal/models/dat_8c18102c.inc"
};
static NJS_MKEY_A dat_8c18104c[2] = {
#include "animal/models/dat_8c18104c.inc"
};
static NJS_MKEY_F dat_8c18106c[17] = {
#include "animal/models/dat_8c18106c.inc"
};
static NJS_MKEY_A dat_8c18117c[2] = {
#include "animal/models/dat_8c18117c.inc"
};
static NJS_MKEY_F dat_8c18119c[2] = {
#include "animal/models/dat_8c18119c.inc"
};
static NJS_MKEY_A dat_8c1811bc[17] = {
#include "animal/models/dat_8c1811bc.inc"
};
static NJS_MKEY_F dat_8c1812cc[2] = {
#include "animal/models/dat_8c1812cc.inc"
};
static NJS_MKEY_A dat_8c1812ec[2] = {
#include "animal/models/dat_8c1812ec.inc"
};
static NJS_MKEY_F dat_8c18130c[2] = {
#include "animal/models/dat_8c18130c.inc"
};
static NJS_MKEY_A dat_8c18132c[2] = {
#include "animal/models/dat_8c18132c.inc"
};
static NJS_MKEY_F dat_8c18134c[2] = {
#include "animal/models/dat_8c18134c.inc"
};
static NJS_MKEY_A dat_8c18136c[2] = {
#include "animal/models/dat_8c18136c.inc"
};
static NJS_MKEY_F dat_8c18138c[2] = {
#include "animal/models/dat_8c18138c.inc"
};
static NJS_MKEY_A dat_8c1813ac[2] = {
#include "animal/models/dat_8c1813ac.inc"
};
static NJS_MKEY_F dat_8c1813cc[2] = {
#include "animal/models/dat_8c1813cc.inc"
};
static NJS_MKEY_A dat_8c1813ec[16] = {
#include "animal/models/dat_8c1813ec.inc"
};
static NJS_MKEY_F dat_8c1814ec[2] = {
#include "animal/models/dat_8c1814ec.inc"
};
static NJS_MKEY_A dat_8c18150c[2] = {
#include "animal/models/dat_8c18150c.inc"
};
static NJS_MKEY_F dat_8c18152c[2] = {
#include "animal/models/dat_8c18152c.inc"
};
static NJS_MKEY_A dat_8c18154c[2] = {
#include "animal/models/dat_8c18154c.inc"
};
static NJS_MKEY_F dat_8c18156c[2] = {
#include "animal/models/dat_8c18156c.inc"
};
static NJS_MKEY_A dat_8c18158c[2] = {
#include "animal/models/dat_8c18158c.inc"
};
static NJS_MKEY_F dat_8c1815ac[2] = {
#include "animal/models/dat_8c1815ac.inc"
};
static NJS_MKEY_A dat_8c1815cc[2] = {
#include "animal/models/dat_8c1815cc.inc"
};
static NJS_MKEY_F dat_8c1815ec[2] = {
#include "animal/models/dat_8c1815ec.inc"
};
static NJS_MKEY_A dat_8c18160c[2] = {
#include "animal/models/dat_8c18160c.inc"
};
static NJS_MKEY_F dat_8c18162c[2] = {
#include "animal/models/dat_8c18162c.inc"
};
static NJS_MKEY_A dat_8c18164c[17] = {
#include "animal/models/dat_8c18164c.inc"
};
static NJS_MKEY_F dat_8c18175c[2] = {
#include "animal/models/dat_8c18175c.inc"
};
static NJS_MKEY_A dat_8c18177c[2] = {
#include "animal/models/dat_8c18177c.inc"
};
static NJS_MKEY_F dat_8c18179c[2] = {
#include "animal/models/dat_8c18179c.inc"
};
static NJS_MKEY_A dat_8c1817bc[2] = {
#include "animal/models/dat_8c1817bc.inc"
};
static NJS_MKEY_F dat_8c1817dc[2] = {
#include "animal/models/dat_8c1817dc.inc"
};
static NJS_MKEY_A dat_8c1817fc[2] = {
#include "animal/models/dat_8c1817fc.inc"
};
static NJS_MKEY_F dat_8c18181c[2] = {
#include "animal/models/dat_8c18181c.inc"
};
static NJS_MKEY_A dat_8c18183c[2] = {
#include "animal/models/dat_8c18183c.inc"
};
static NJS_MKEY_F dat_8c18185c[2] = {
#include "animal/models/dat_8c18185c.inc"
};
static NJS_MKEY_A dat_8c18187c[2] = {
#include "animal/models/dat_8c18187c.inc"
};
static NJS_MKEY_F dat_8c18189c[2] = {
#include "animal/models/dat_8c18189c.inc"
};
static NJS_MKEY_A dat_8c1818bc[17] = {
#include "animal/models/dat_8c1818bc.inc"
};
static NJS_MKEY_F dat_8c1819cc[2] = {
#include "animal/models/dat_8c1819cc.inc"
};
static NJS_MKEY_A dat_8c1819ec[2] = {
#include "animal/models/dat_8c1819ec.inc"
};
static NJS_MKEY_F dat_8c181a0c[2] = {
#include "animal/models/dat_8c181a0c.inc"
};
static NJS_MKEY_A dat_8c181a2c[2] = {
#include "animal/models/dat_8c181a2c.inc"
};
static NJS_MKEY_F dat_8c181a4c[2] = {
#include "animal/models/dat_8c181a4c.inc"
};
static NJS_MKEY_A dat_8c181a6c[2] = {
#include "animal/models/dat_8c181a6c.inc"
};
static NJS_MKEY_F dat_8c181a8c[2] = {
#include "animal/models/dat_8c181a8c.inc"
};
static NJS_MKEY_A dat_8c181aac[17] = {
#include "animal/models/dat_8c181aac.inc"
};
static NJS_MKEY_F dat_8c181bbc[2] = {
#include "animal/models/dat_8c181bbc.inc"
};
static NJS_MKEY_A dat_8c181bdc[2] = {
#include "animal/models/dat_8c181bdc.inc"
};
static NJS_MKEY_F dat_8c181bfc[2] = {
#include "animal/models/dat_8c181bfc.inc"
};
static NJS_MKEY_A dat_8c181c1c[2] = {
#include "animal/models/dat_8c181c1c.inc"
};
static NJS_MKEY_F dat_8c181c3c[2] = {
#include "animal/models/dat_8c181c3c.inc"
};
static NJS_MKEY_A dat_8c181c5c[2] = {
#include "animal/models/dat_8c181c5c.inc"
};
static NJS_MKEY_F dat_8c181c7c[2] = {
#include "animal/models/dat_8c181c7c.inc"
};
static NJS_MKEY_A dat_8c181c9c[17] = {
#include "animal/models/dat_8c181c9c.inc"
};
static NJS_MKEY_F dat_8c181dac[2] = {
#include "animal/models/dat_8c181dac.inc"
};
static NJS_MKEY_A dat_8c181dcc[2] = {
#include "animal/models/dat_8c181dcc.inc"
};
static NJS_MKEY_F dat_8c181dec[2] = {
#include "animal/models/dat_8c181dec.inc"
};
static NJS_MKEY_A dat_8c181e0c[2] = {
#include "animal/models/dat_8c181e0c.inc"
};
static NJS_MDATA2 dat_8c181e2c[34] = {
#include "animal/models/dat_8c181e2c.inc"
};
static NJS_MOTION kuja_motion = {
#include "animal/models/kuja_motion.inc"
};
NJS_ACTION kuja_action = {
#include "animal/models/kuja_action.inc"
};
static NJS_TEXNAME kuja_textures[5] = {
    {TEXT_16, 0, 0}, {TEXT_17, 0, 0}, {TEXT_18, 0, 0},
    {TEXT_19, 0, 0}, {TEXT_20, 0, 0},
};
NJS_TEXLIST kuja_texlist = {
#include "animal/models/kuja_texlist.inc"
};
static NJS_MATERIAL dat_8c1820a4[1] = {
#include "animal/models/dat_8c1820a4.inc"
};
static Sint16 dat_8c1820b8[35] = {
#include "animal/models/dat_8c1820b8.inc"
};
static NJS_COLOR dat_8c182100[31] = {
#include "animal/models/dat_8c182100.inc"
};
static NJS_MESHSET dat_8c18217c[1] = {
#include "animal/models/dat_8c18217c.inc"
};
static NJS_POINT3 dat_8c182194[15] = {
#include "animal/models/dat_8c182194.inc"
};
static NJS_VECTOR dat_8c182248[15] = {
#include "animal/models/dat_8c182248.inc"
};
static NJS_MODEL dat_8c1822fc = {
#include "animal/models/dat_8c1822fc.inc"
};
static NJS_OBJECT dat_8c182324 = {
#include "animal/models/dat_8c182324.inc"
};
static NJS_MATERIAL dat_8c182358[1] = {
#include "animal/models/dat_8c182358.inc"
};
static Sint16 dat_8c18236c[5] = {
#include "animal/models/dat_8c18236c.inc"
};
static NJS_COLOR dat_8c182378[4] = {
#include "animal/models/dat_8c182378.inc"
};
static NJS_MESHSET dat_8c182388[1] = {
#include "animal/models/dat_8c182388.inc"
};
static NJS_POINT3 dat_8c1823a0[4] = {
#include "animal/models/dat_8c1823a0.inc"
};
static NJS_VECTOR dat_8c1823d0[4] = {
#include "animal/models/dat_8c1823d0.inc"
};
static NJS_MODEL dat_8c182400 = {
#include "animal/models/dat_8c182400.inc"
};
static NJS_OBJECT dat_8c182428 = {
#include "animal/models/dat_8c182428.inc"
};
static NJS_OBJECT dat_8c18245c = {
#include "animal/models/dat_8c18245c.inc"
};
static NJS_OBJECT dat_8c182490 = {
#include "animal/models/dat_8c182490.inc"
};
static NJS_OBJECT dat_8c1824c4 = {
#include "animal/models/dat_8c1824c4.inc"
};
static NJS_MATERIAL dat_8c1824f8[1] = {
#include "animal/models/dat_8c1824f8.inc"
};
static Sint16 dat_8c18250c[11] = {
#include "animal/models/dat_8c18250c.inc"
};
static NJS_COLOR dat_8c182524[9] = {
#include "animal/models/dat_8c182524.inc"
};
static NJS_MESHSET dat_8c182548[1] = {
#include "animal/models/dat_8c182548.inc"
};
static NJS_POINT3 dat_8c182560[7] = {
#include "animal/models/dat_8c182560.inc"
};
static NJS_VECTOR dat_8c1825b4[7] = {
#include "animal/models/dat_8c1825b4.inc"
};
static NJS_MODEL dat_8c182608 = {
#include "animal/models/dat_8c182608.inc"
};
static NJS_OBJECT dat_8c182630 = {
#include "animal/models/dat_8c182630.inc"
};
static NJS_OBJECT dat_8c182664 = {
#include "animal/models/dat_8c182664.inc"
};
static NJS_OBJECT dat_8c182698 = {
#include "animal/models/dat_8c182698.inc"
};
static NJS_OBJECT dat_8c1826cc = {
#include "animal/models/dat_8c1826cc.inc"
};
static NJS_MATERIAL dat_8c182700[1] = {
#include "animal/models/dat_8c182700.inc"
};
static Sint16 dat_8c182714[11] = {
#include "animal/models/dat_8c182714.inc"
};
static NJS_COLOR dat_8c18272c[9] = {
#include "animal/models/dat_8c18272c.inc"
};
static NJS_MESHSET dat_8c182750[1] = {
#include "animal/models/dat_8c182750.inc"
};
static NJS_POINT3 dat_8c182768[7] = {
#include "animal/models/dat_8c182768.inc"
};
static NJS_VECTOR dat_8c1827bc[7] = {
#include "animal/models/dat_8c1827bc.inc"
};
static NJS_MODEL dat_8c182810 = {
#include "animal/models/dat_8c182810.inc"
};
static NJS_OBJECT dat_8c182838 = {
#include "animal/models/dat_8c182838.inc"
};
static NJS_OBJECT dat_8c18286c = {
#include "animal/models/dat_8c18286c.inc"
};
static NJS_OBJECT dat_8c1828a0 = {
#include "animal/models/dat_8c1828a0.inc"
};
static NJS_OBJECT dat_8c1828d4 = {
#include "animal/models/dat_8c1828d4.inc"
};
static NJS_MATERIAL dat_8c182908[1] = {
#include "animal/models/dat_8c182908.inc"
};
static Sint16 dat_8c18291c[12] = {
#include "animal/models/dat_8c18291c.inc"
};
static NJS_COLOR dat_8c182934[10] = {
#include "animal/models/dat_8c182934.inc"
};
static NJS_MESHSET dat_8c18295c[1] = {
#include "animal/models/dat_8c18295c.inc"
};
static NJS_POINT3 dat_8c182974[7] = {
#include "animal/models/dat_8c182974.inc"
};
static NJS_VECTOR dat_8c1829c8[7] = {
#include "animal/models/dat_8c1829c8.inc"
};
static NJS_MODEL dat_8c182a1c = {
#include "animal/models/dat_8c182a1c.inc"
};
static NJS_OBJECT dat_8c182a44 = {
#include "animal/models/dat_8c182a44.inc"
};
static NJS_MATERIAL dat_8c182a78[1] = {
#include "animal/models/dat_8c182a78.inc"
};
static Sint16 dat_8c182a8c[5] = {
#include "animal/models/dat_8c182a8c.inc"
};
static NJS_COLOR dat_8c182a98[4] = {
#include "animal/models/dat_8c182a98.inc"
};
static NJS_MESHSET dat_8c182aa8[1] = {
#include "animal/models/dat_8c182aa8.inc"
};
static NJS_POINT3 dat_8c182ac0[4] = {
#include "animal/models/dat_8c182ac0.inc"
};
static NJS_VECTOR dat_8c182af0[4] = {
#include "animal/models/dat_8c182af0.inc"
};
static NJS_MODEL dat_8c182b20 = {
#include "animal/models/dat_8c182b20.inc"
};
static NJS_OBJECT dat_8c182b48 = {
#include "animal/models/dat_8c182b48.inc"
};
static NJS_OBJECT dat_8c182b7c = {
#include "animal/models/dat_8c182b7c.inc"
};
static NJS_OBJECT dat_8c182bb0 = {
#include "animal/models/dat_8c182bb0.inc"
};
static NJS_OBJECT dat_8c182be4 = {
#include "animal/models/dat_8c182be4.inc"
};
static NJS_OBJECT dat_8c182c18 = {
#include "animal/models/dat_8c182c18.inc"
};
static NJS_MATERIAL dat_8c182c4c[1] = {
#include "animal/models/dat_8c182c4c.inc"
};
static Sint16 dat_8c182c60[12] = {
#include "animal/models/dat_8c182c60.inc"
};
static NJS_COLOR dat_8c182c78[10] = {
#include "animal/models/dat_8c182c78.inc"
};
static NJS_MESHSET dat_8c182ca0[1] = {
#include "animal/models/dat_8c182ca0.inc"
};
static NJS_POINT3 dat_8c182cb8[7] = {
#include "animal/models/dat_8c182cb8.inc"
};
static NJS_VECTOR dat_8c182d0c[7] = {
#include "animal/models/dat_8c182d0c.inc"
};
static NJS_MODEL dat_8c182d60 = {
#include "animal/models/dat_8c182d60.inc"
};
static NJS_OBJECT dat_8c182d88 = {
#include "animal/models/dat_8c182d88.inc"
};
static NJS_MATERIAL dat_8c182dbc[1] = {
#include "animal/models/dat_8c182dbc.inc"
};
static Sint16 dat_8c182dd0[5] = {
#include "animal/models/dat_8c182dd0.inc"
};
static NJS_COLOR dat_8c182ddc[4] = {
#include "animal/models/dat_8c182ddc.inc"
};
static NJS_MESHSET dat_8c182dec[1] = {
#include "animal/models/dat_8c182dec.inc"
};
static NJS_POINT3 dat_8c182e04[4] = {
#include "animal/models/dat_8c182e04.inc"
};
static NJS_VECTOR dat_8c182e34[4] = {
#include "animal/models/dat_8c182e34.inc"
};
static NJS_MODEL dat_8c182e64 = {
#include "animal/models/dat_8c182e64.inc"
};
static NJS_OBJECT dat_8c182e8c = {
#include "animal/models/dat_8c182e8c.inc"
};
static NJS_OBJECT dat_8c182ec0 = {
#include "animal/models/dat_8c182ec0.inc"
};
static NJS_OBJECT dat_8c182ef4 = {
#include "animal/models/dat_8c182ef4.inc"
};
static NJS_OBJECT dat_8c182f28 = {
#include "animal/models/dat_8c182f28.inc"
};
static NJS_OBJECT dat_8c182f5c = {
#include "animal/models/dat_8c182f5c.inc"
};
static NJS_MATERIAL dat_8c182f90[1] = {
#include "animal/models/dat_8c182f90.inc"
};
static Sint16 dat_8c182fa4[5] = {
#include "animal/models/dat_8c182fa4.inc"
};
static NJS_COLOR dat_8c182fb0[4] = {
#include "animal/models/dat_8c182fb0.inc"
};
static NJS_MESHSET dat_8c182fc0[1] = {
#include "animal/models/dat_8c182fc0.inc"
};
static NJS_POINT3 dat_8c182fd8[4] = {
#include "animal/models/dat_8c182fd8.inc"
};
static NJS_VECTOR dat_8c183008[4] = {
#include "animal/models/dat_8c183008.inc"
};
static NJS_MODEL dat_8c183038 = {
#include "animal/models/dat_8c183038.inc"
};
static NJS_OBJECT dat_8c183060 = {
#include "animal/models/dat_8c183060.inc"
};
static NJS_MATERIAL dat_8c183094[1] = {
#include "animal/models/dat_8c183094.inc"
};
static Sint16 dat_8c1830a8[15] = {
#include "animal/models/dat_8c1830a8.inc"
};
static NJS_COLOR dat_8c1830c8[13] = {
#include "animal/models/dat_8c1830c8.inc"
};
static NJS_MESHSET dat_8c1830fc[1] = {
#include "animal/models/dat_8c1830fc.inc"
};
static NJS_POINT3 dat_8c183114[9] = {
#include "animal/models/dat_8c183114.inc"
};
static NJS_VECTOR dat_8c183180[9] = {
#include "animal/models/dat_8c183180.inc"
};
static NJS_MODEL dat_8c1831ec = {
#include "animal/models/dat_8c1831ec.inc"
};
static NJS_OBJECT dat_8c183214 = {
#include "animal/models/dat_8c183214.inc"
};
static NJS_MATERIAL dat_8c183248[1] = {
#include "animal/models/dat_8c183248.inc"
};
static Sint16 dat_8c18325c[51] = {
#include "animal/models/dat_8c18325c.inc"
};
static NJS_COLOR dat_8c1832c4[43] = {
#include "animal/models/dat_8c1832c4.inc"
};
static NJS_MESHSET dat_8c183370[1] = {
#include "animal/models/dat_8c183370.inc"
};
static NJS_POINT3 dat_8c183388[18] = {
#include "animal/models/dat_8c183388.inc"
};
static NJS_VECTOR dat_8c183460[18] = {
#include "animal/models/dat_8c183460.inc"
};
static NJS_MODEL dat_8c183538 = {
#include "animal/models/dat_8c183538.inc"
};
static NJS_OBJECT dat_8c183560 = {
#include "animal/models/dat_8c183560.inc"
};
static NJS_OBJECT dat_8c183594 = {
#include "animal/models/dat_8c183594.inc"
};
static NJS_OBJECT dat_8c1835c8 = {
#include "animal/models/dat_8c1835c8.inc"
};
static NJS_OBJECT dat_8c1835fc = {
#include "animal/models/dat_8c1835fc.inc"
};
static NJS_OBJECT dat_8c183630 = {
#include "animal/models/dat_8c183630.inc"
};
static NJS_OBJECT dat_8c183664 = {
#include "animal/models/dat_8c183664.inc"
};
static NJS_OBJECT dat_8c183698 = {
#include "animal/models/dat_8c183698.inc"
};
static NJS_OBJECT oum_object = {
#include "animal/models/oum_object.inc"
};
static NJS_MKEY_F dat_8c183700[2] = {
#include "animal/models/dat_8c183700.inc"
};
static NJS_MKEY_A dat_8c183720[2] = {
#include "animal/models/dat_8c183720.inc"
};
static NJS_MKEY_F dat_8c183740[17] = {
#include "animal/models/dat_8c183740.inc"
};
static NJS_MKEY_A dat_8c183850[2] = {
#include "animal/models/dat_8c183850.inc"
};
static NJS_MKEY_F dat_8c183870[2] = {
#include "animal/models/dat_8c183870.inc"
};
static NJS_MKEY_A dat_8c183890[2] = {
#include "animal/models/dat_8c183890.inc"
};
static NJS_MKEY_F dat_8c1838b0[2] = {
#include "animal/models/dat_8c1838b0.inc"
};
static NJS_MKEY_A dat_8c1838d0[2] = {
#include "animal/models/dat_8c1838d0.inc"
};
static NJS_MKEY_F dat_8c1838f0[2] = {
#include "animal/models/dat_8c1838f0.inc"
};
static NJS_MKEY_A dat_8c183910[2] = {
#include "animal/models/dat_8c183910.inc"
};
static NJS_MKEY_F dat_8c183930[2] = {
#include "animal/models/dat_8c183930.inc"
};
static NJS_MKEY_A dat_8c183950[17] = {
#include "animal/models/dat_8c183950.inc"
};
static NJS_MKEY_F dat_8c183a60[2] = {
#include "animal/models/dat_8c183a60.inc"
};
static NJS_MKEY_A dat_8c183a80[2] = {
#include "animal/models/dat_8c183a80.inc"
};
static NJS_MKEY_F dat_8c183aa0[2] = {
#include "animal/models/dat_8c183aa0.inc"
};
static NJS_MKEY_A dat_8c183ac0[2] = {
#include "animal/models/dat_8c183ac0.inc"
};
static NJS_MKEY_F dat_8c183ae0[2] = {
#include "animal/models/dat_8c183ae0.inc"
};
static NJS_MKEY_A dat_8c183b00[2] = {
#include "animal/models/dat_8c183b00.inc"
};
static NJS_MKEY_F dat_8c183b20[2] = {
#include "animal/models/dat_8c183b20.inc"
};
static NJS_MKEY_A dat_8c183b40[2] = {
#include "animal/models/dat_8c183b40.inc"
};
static NJS_MKEY_F dat_8c183b60[2] = {
#include "animal/models/dat_8c183b60.inc"
};
static NJS_MKEY_A dat_8c183b80[2] = {
#include "animal/models/dat_8c183b80.inc"
};
static NJS_MKEY_F dat_8c183ba0[2] = {
#include "animal/models/dat_8c183ba0.inc"
};
static NJS_MKEY_A dat_8c183bc0[17] = {
#include "animal/models/dat_8c183bc0.inc"
};
static NJS_MKEY_F dat_8c183cd0[2] = {
#include "animal/models/dat_8c183cd0.inc"
};
static NJS_MKEY_A dat_8c183cf0[2] = {
#include "animal/models/dat_8c183cf0.inc"
};
static NJS_MKEY_F dat_8c183d10[2] = {
#include "animal/models/dat_8c183d10.inc"
};
static NJS_MKEY_A dat_8c183d30[2] = {
#include "animal/models/dat_8c183d30.inc"
};
static NJS_MKEY_F dat_8c183d50[2] = {
#include "animal/models/dat_8c183d50.inc"
};
static NJS_MKEY_A dat_8c183d70[2] = {
#include "animal/models/dat_8c183d70.inc"
};
static NJS_MKEY_F dat_8c183d90[2] = {
#include "animal/models/dat_8c183d90.inc"
};
static NJS_MKEY_A dat_8c183db0[2] = {
#include "animal/models/dat_8c183db0.inc"
};
static NJS_MKEY_F dat_8c183dd0[2] = {
#include "animal/models/dat_8c183dd0.inc"
};
static NJS_MKEY_A dat_8c183df0[2] = {
#include "animal/models/dat_8c183df0.inc"
};
static NJS_MKEY_F dat_8c183e10[2] = {
#include "animal/models/dat_8c183e10.inc"
};
static NJS_MKEY_A dat_8c183e30[17] = {
#include "animal/models/dat_8c183e30.inc"
};
static NJS_MKEY_F dat_8c183f40[2] = {
#include "animal/models/dat_8c183f40.inc"
};
static NJS_MKEY_A dat_8c183f60[2] = {
#include "animal/models/dat_8c183f60.inc"
};
static NJS_MKEY_F dat_8c183f80[2] = {
#include "animal/models/dat_8c183f80.inc"
};
static NJS_MKEY_A dat_8c183fa0[2] = {
#include "animal/models/dat_8c183fa0.inc"
};
static NJS_MKEY_F dat_8c183fc0[2] = {
#include "animal/models/dat_8c183fc0.inc"
};
static NJS_MKEY_A dat_8c183fe0[2] = {
#include "animal/models/dat_8c183fe0.inc"
};
static NJS_MKEY_F dat_8c184000[2] = {
#include "animal/models/dat_8c184000.inc"
};
static NJS_MKEY_A dat_8c184020[2] = {
#include "animal/models/dat_8c184020.inc"
};
static NJS_MKEY_F dat_8c184040[2] = {
#include "animal/models/dat_8c184040.inc"
};
static NJS_MKEY_A dat_8c184060[2] = {
#include "animal/models/dat_8c184060.inc"
};
static NJS_MKEY_F dat_8c184080[2] = {
#include "animal/models/dat_8c184080.inc"
};
static NJS_MKEY_A dat_8c1840a0[17] = {
#include "animal/models/dat_8c1840a0.inc"
};
static NJS_MKEY_F dat_8c1841b0[2] = {
#include "animal/models/dat_8c1841b0.inc"
};
static NJS_MKEY_A dat_8c1841d0[2] = {
#include "animal/models/dat_8c1841d0.inc"
};
static NJS_MKEY_F dat_8c1841f0[2] = {
#include "animal/models/dat_8c1841f0.inc"
};
static NJS_MKEY_A dat_8c184210[2] = {
#include "animal/models/dat_8c184210.inc"
};
static NJS_MKEY_F dat_8c184230[2] = {
#include "animal/models/dat_8c184230.inc"
};
static NJS_MKEY_A dat_8c184250[2] = {
#include "animal/models/dat_8c184250.inc"
};
static NJS_MKEY_F dat_8c184270[2] = {
#include "animal/models/dat_8c184270.inc"
};
static NJS_MKEY_A dat_8c184290[17] = {
#include "animal/models/dat_8c184290.inc"
};
static NJS_MKEY_F dat_8c1843a0[2] = {
#include "animal/models/dat_8c1843a0.inc"
};
static NJS_MKEY_A dat_8c1843c0[2] = {
#include "animal/models/dat_8c1843c0.inc"
};
static NJS_MKEY_F dat_8c1843e0[2] = {
#include "animal/models/dat_8c1843e0.inc"
};
static NJS_MKEY_A dat_8c184400[2] = {
#include "animal/models/dat_8c184400.inc"
};
static NJS_MKEY_F dat_8c184420[2] = {
#include "animal/models/dat_8c184420.inc"
};
static NJS_MKEY_A dat_8c184440[2] = {
#include "animal/models/dat_8c184440.inc"
};
static NJS_MKEY_F dat_8c184460[2] = {
#include "animal/models/dat_8c184460.inc"
};
static NJS_MKEY_A dat_8c184480[17] = {
#include "animal/models/dat_8c184480.inc"
};
static NJS_MKEY_F dat_8c184590[2] = {
#include "animal/models/dat_8c184590.inc"
};
static NJS_MKEY_A dat_8c1845b0[2] = {
#include "animal/models/dat_8c1845b0.inc"
};
static NJS_MKEY_F dat_8c1845d0[2] = {
#include "animal/models/dat_8c1845d0.inc"
};
static NJS_MKEY_A dat_8c1845f0[2] = {
#include "animal/models/dat_8c1845f0.inc"
};
static NJS_MKEY_F dat_8c184610[2] = {
#include "animal/models/dat_8c184610.inc"
};
static NJS_MKEY_A dat_8c184630[2] = {
#include "animal/models/dat_8c184630.inc"
};
static NJS_MDATA2 dat_8c184650[35] = {
#include "animal/models/dat_8c184650.inc"
};
static NJS_MOTION oum_motion = {
#include "animal/models/oum_motion.inc"
};
NJS_ACTION oum_action = {
#include "animal/models/oum_action.inc"
};
static NJS_TEXNAME oum_textures[5] = {
    {TEXT_21, 0, 0}, {TEXT_22, 0, 0}, {TEXT_23, 0, 0},
    {TEXT_24, 0, 0}, {TEXT_25, 0, 0},
};
NJS_TEXLIST oum_texlist = {
#include "animal/models/oum_texlist.inc"
};
static NJS_MATERIAL dat_8c1848d8[1] = {
#include "animal/models/dat_8c1848d8.inc"
};
static Sint16 dat_8c1848ec[37] = {
#include "animal/models/dat_8c1848ec.inc"
};
static NJS_COLOR dat_8c184938[31] = {
#include "animal/models/dat_8c184938.inc"
};
static NJS_MESHSET dat_8c1849b4[1] = {
#include "animal/models/dat_8c1849b4.inc"
};
static NJS_POINT3 dat_8c1849cc[12] = {
#include "animal/models/dat_8c1849cc.inc"
};
static NJS_VECTOR dat_8c184a5c[12] = {
#include "animal/models/dat_8c184a5c.inc"
};
static NJS_MODEL dat_8c184aec = {
#include "animal/models/dat_8c184aec.inc"
};
static NJS_OBJECT dat_8c184b14 = {
#include "animal/models/dat_8c184b14.inc"
};
static NJS_MATERIAL dat_8c184b48[1] = {
#include "animal/models/dat_8c184b48.inc"
};
static Sint16 dat_8c184b5c[19] = {
#include "animal/models/dat_8c184b5c.inc"
};
static NJS_COLOR dat_8c184b84[16] = {
#include "animal/models/dat_8c184b84.inc"
};
static NJS_MESHSET dat_8c184bc4[1] = {
#include "animal/models/dat_8c184bc4.inc"
};
static NJS_POINT3 dat_8c184bdc[7] = {
#include "animal/models/dat_8c184bdc.inc"
};
static NJS_VECTOR dat_8c184c30[7] = {
#include "animal/models/dat_8c184c30.inc"
};
static NJS_MODEL dat_8c184c84 = {
#include "animal/models/dat_8c184c84.inc"
};
static NJS_OBJECT dat_8c184cac = {
#include "animal/models/dat_8c184cac.inc"
};
static NJS_MATERIAL dat_8c184ce0[1] = {
#include "animal/models/dat_8c184ce0.inc"
};
static Sint16 dat_8c184cf4[10] = {
#include "animal/models/dat_8c184cf4.inc"
};
static NJS_COLOR dat_8c184d08[8] = {
#include "animal/models/dat_8c184d08.inc"
};
static NJS_MESHSET dat_8c184d28[1] = {
#include "animal/models/dat_8c184d28.inc"
};
static NJS_POINT3 dat_8c184d40[4] = {
#include "animal/models/dat_8c184d40.inc"
};
static NJS_VECTOR dat_8c184d70[4] = {
#include "animal/models/dat_8c184d70.inc"
};
static NJS_MODEL dat_8c184da0 = {
#include "animal/models/dat_8c184da0.inc"
};
static NJS_OBJECT dat_8c184dc8 = {
#include "animal/models/dat_8c184dc8.inc"
};
static NJS_OBJECT dat_8c184dfc = {
#include "animal/models/dat_8c184dfc.inc"
};
static NJS_OBJECT dat_8c184e30 = {
#include "animal/models/dat_8c184e30.inc"
};
static NJS_OBJECT dat_8c184e64 = {
#include "animal/models/dat_8c184e64.inc"
};
static NJS_OBJECT dat_8c184e98 = {
#include "animal/models/dat_8c184e98.inc"
};
static NJS_MATERIAL dat_8c184ecc[1] = {
#include "animal/models/dat_8c184ecc.inc"
};
static Sint16 dat_8c184ee0[19] = {
#include "animal/models/dat_8c184ee0.inc"
};
static NJS_COLOR dat_8c184f08[16] = {
#include "animal/models/dat_8c184f08.inc"
};
static NJS_MESHSET dat_8c184f48[1] = {
#include "animal/models/dat_8c184f48.inc"
};
static NJS_POINT3 dat_8c184f60[7] = {
#include "animal/models/dat_8c184f60.inc"
};
static NJS_VECTOR dat_8c184fb4[7] = {
#include "animal/models/dat_8c184fb4.inc"
};
static NJS_MODEL dat_8c185008 = {
#include "animal/models/dat_8c185008.inc"
};
static NJS_OBJECT dat_8c185030 = {
#include "animal/models/dat_8c185030.inc"
};
static NJS_MATERIAL dat_8c185064[1] = {
#include "animal/models/dat_8c185064.inc"
};
static Sint16 dat_8c185078[10] = {
#include "animal/models/dat_8c185078.inc"
};
static NJS_COLOR dat_8c18508c[8] = {
#include "animal/models/dat_8c18508c.inc"
};
static NJS_MESHSET dat_8c1850ac[1] = {
#include "animal/models/dat_8c1850ac.inc"
};
static NJS_POINT3 dat_8c1850c4[4] = {
#include "animal/models/dat_8c1850c4.inc"
};
static NJS_VECTOR dat_8c1850f4[4] = {
#include "animal/models/dat_8c1850f4.inc"
};
static NJS_MODEL dat_8c185124 = {
#include "animal/models/dat_8c185124.inc"
};
static NJS_OBJECT dat_8c18514c = {
#include "animal/models/dat_8c18514c.inc"
};
static NJS_OBJECT dat_8c185180 = {
#include "animal/models/dat_8c185180.inc"
};
static NJS_OBJECT dat_8c1851b4 = {
#include "animal/models/dat_8c1851b4.inc"
};
static NJS_OBJECT dat_8c1851e8 = {
#include "animal/models/dat_8c1851e8.inc"
};
static NJS_OBJECT dat_8c18521c = {
#include "animal/models/dat_8c18521c.inc"
};
static NJS_MATERIAL dat_8c185250[1] = {
#include "animal/models/dat_8c185250.inc"
};
static Sint16 dat_8c185264[22] = {
#include "animal/models/dat_8c185264.inc"
};
static NJS_COLOR dat_8c185290[18] = {
#include "animal/models/dat_8c185290.inc"
};
static NJS_MESHSET dat_8c1852d8[1] = {
#include "animal/models/dat_8c1852d8.inc"
};
static NJS_POINT3 dat_8c1852f0[7] = {
#include "animal/models/dat_8c1852f0.inc"
};
static NJS_VECTOR dat_8c185344[7] = {
#include "animal/models/dat_8c185344.inc"
};
static NJS_MODEL dat_8c185398 = {
#include "animal/models/dat_8c185398.inc"
};
static NJS_OBJECT dat_8c1853c0 = {
#include "animal/models/dat_8c1853c0.inc"
};
static NJS_OBJECT dat_8c1853f4 = {
#include "animal/models/dat_8c1853f4.inc"
};
static NJS_OBJECT dat_8c185428 = {
#include "animal/models/dat_8c185428.inc"
};
static NJS_OBJECT dat_8c18545c = {
#include "animal/models/dat_8c18545c.inc"
};
static NJS_MATERIAL dat_8c185490[1] = {
#include "animal/models/dat_8c185490.inc"
};
static Sint16 dat_8c1854a4[22] = {
#include "animal/models/dat_8c1854a4.inc"
};
static NJS_COLOR dat_8c1854d0[18] = {
#include "animal/models/dat_8c1854d0.inc"
};
static NJS_MESHSET dat_8c185518[1] = {
#include "animal/models/dat_8c185518.inc"
};
static NJS_POINT3 dat_8c185530[7] = {
#include "animal/models/dat_8c185530.inc"
};
static NJS_VECTOR dat_8c185584[7] = {
#include "animal/models/dat_8c185584.inc"
};
static NJS_MODEL dat_8c1855d8 = {
#include "animal/models/dat_8c1855d8.inc"
};
static NJS_OBJECT dat_8c185600 = {
#include "animal/models/dat_8c185600.inc"
};
static NJS_OBJECT dat_8c185634 = {
#include "animal/models/dat_8c185634.inc"
};
static NJS_OBJECT dat_8c185668 = {
#include "animal/models/dat_8c185668.inc"
};
static NJS_OBJECT dat_8c18569c = {
#include "animal/models/dat_8c18569c.inc"
};
static NJS_MATERIAL dat_8c1856d0[1] = {
#include "animal/models/dat_8c1856d0.inc"
};
static Sint16 dat_8c1856e4[43] = {
#include "animal/models/dat_8c1856e4.inc"
};
static NJS_COLOR dat_8c18573c[38] = {
#include "animal/models/dat_8c18573c.inc"
};
static NJS_MESHSET dat_8c1857d4[1] = {
#include "animal/models/dat_8c1857d4.inc"
};
static NJS_POINT3 dat_8c1857ec[16] = {
#include "animal/models/dat_8c1857ec.inc"
};
static NJS_VECTOR dat_8c1858ac[16] = {
#include "animal/models/dat_8c1858ac.inc"
};
static NJS_MODEL dat_8c18596c = {
#include "animal/models/dat_8c18596c.inc"
};
static NJS_OBJECT dat_8c185994 = {
#include "animal/models/dat_8c185994.inc"
};
static NJS_MATERIAL dat_8c1859c8[1] = {
#include "animal/models/dat_8c1859c8.inc"
};
static Sint16 dat_8c1859dc[5] = {
#include "animal/models/dat_8c1859dc.inc"
};
static NJS_COLOR dat_8c1859e8[4] = {
#include "animal/models/dat_8c1859e8.inc"
};
static NJS_MESHSET dat_8c1859f8[1] = {
#include "animal/models/dat_8c1859f8.inc"
};
static NJS_POINT3 dat_8c185a10[4] = {
#include "animal/models/dat_8c185a10.inc"
};
static NJS_VECTOR dat_8c185a40[4] = {
#include "animal/models/dat_8c185a40.inc"
};
static NJS_MODEL dat_8c185a70 = {
#include "animal/models/dat_8c185a70.inc"
};
static NJS_OBJECT dat_8c185a98 = {
#include "animal/models/dat_8c185a98.inc"
};
static NJS_OBJECT dat_8c185acc = {
#include "animal/models/dat_8c185acc.inc"
};
static NJS_OBJECT dat_8c185b00 = {
#include "animal/models/dat_8c185b00.inc"
};
static NJS_OBJECT dat_8c185b34 = {
#include "animal/models/dat_8c185b34.inc"
};
static NJS_MATERIAL dat_8c185b68[1] = {
#include "animal/models/dat_8c185b68.inc"
};
static Sint16 dat_8c185b7c[5] = {
#include "animal/models/dat_8c185b7c.inc"
};
static NJS_COLOR dat_8c185b88[4] = {
#include "animal/models/dat_8c185b88.inc"
};
static NJS_MESHSET dat_8c185b98[1] = {
#include "animal/models/dat_8c185b98.inc"
};
static NJS_POINT3 dat_8c185bb0[4] = {
#include "animal/models/dat_8c185bb0.inc"
};
static NJS_VECTOR dat_8c185be0[4] = {
#include "animal/models/dat_8c185be0.inc"
};
static NJS_MODEL dat_8c185c10 = {
#include "animal/models/dat_8c185c10.inc"
};
static NJS_OBJECT dat_8c185c38 = {
#include "animal/models/dat_8c185c38.inc"
};
static NJS_OBJECT dat_8c185c6c = {
#include "animal/models/dat_8c185c6c.inc"
};
static NJS_OBJECT dat_8c185ca0 = {
#include "animal/models/dat_8c185ca0.inc"
};
static NJS_OBJECT dat_8c185cd4 = {
#include "animal/models/dat_8c185cd4.inc"
};
static NJS_OBJECT dat_8c185d08 = {
#include "animal/models/dat_8c185d08.inc"
};
static NJS_OBJECT dat_8c185d3c = {
#include "animal/models/dat_8c185d3c.inc"
};
static NJS_OBJECT dat_8c185d70 = {
#include "animal/models/dat_8c185d70.inc"
};
static NJS_OBJECT dat_8c185da4 = {
#include "animal/models/dat_8c185da4.inc"
};
static NJS_OBJECT dat_8c185dd8 = {
#include "animal/models/dat_8c185dd8.inc"
};
static NJS_OBJECT dat_8c185e0c = {
#include "animal/models/dat_8c185e0c.inc"
};
static NJS_OBJECT koar_object = {
#include "animal/models/koar_object.inc"
};
static NJS_MKEY_F dat_8c185e74[2] = {
#include "animal/models/dat_8c185e74.inc"
};
static NJS_MKEY_A dat_8c185e94[2] = {
#include "animal/models/dat_8c185e94.inc"
};
static NJS_MKEY_F dat_8c185eb4[17] = {
#include "animal/models/dat_8c185eb4.inc"
};
static NJS_MKEY_A dat_8c185fc4[2] = {
#include "animal/models/dat_8c185fc4.inc"
};
static NJS_MKEY_F dat_8c185fe4[2] = {
#include "animal/models/dat_8c185fe4.inc"
};
static NJS_MKEY_A dat_8c186004[17] = {
#include "animal/models/dat_8c186004.inc"
};
static NJS_MKEY_F dat_8c186114[2] = {
#include "animal/models/dat_8c186114.inc"
};
static NJS_MKEY_A dat_8c186134[2] = {
#include "animal/models/dat_8c186134.inc"
};
static NJS_MKEY_F dat_8c186154[2] = {
#include "animal/models/dat_8c186154.inc"
};
static NJS_MKEY_A dat_8c186174[2] = {
#include "animal/models/dat_8c186174.inc"
};
static NJS_MKEY_F dat_8c186194[2] = {
#include "animal/models/dat_8c186194.inc"
};
static NJS_MKEY_A dat_8c1861b4[17] = {
#include "animal/models/dat_8c1861b4.inc"
};
static NJS_MKEY_F dat_8c1862c4[2] = {
#include "animal/models/dat_8c1862c4.inc"
};
static NJS_MKEY_A dat_8c1862e4[2] = {
#include "animal/models/dat_8c1862e4.inc"
};
static NJS_MKEY_F dat_8c186304[2] = {
#include "animal/models/dat_8c186304.inc"
};
static NJS_MKEY_A dat_8c186324[2] = {
#include "animal/models/dat_8c186324.inc"
};
static NJS_MKEY_F dat_8c186344[2] = {
#include "animal/models/dat_8c186344.inc"
};
static NJS_MKEY_A dat_8c186364[17] = {
#include "animal/models/dat_8c186364.inc"
};
static NJS_MKEY_F dat_8c186474[2] = {
#include "animal/models/dat_8c186474.inc"
};
static NJS_MKEY_A dat_8c186494[2] = {
#include "animal/models/dat_8c186494.inc"
};
static NJS_MKEY_F dat_8c1864b4[2] = {
#include "animal/models/dat_8c1864b4.inc"
};
static NJS_MKEY_A dat_8c1864d4[2] = {
#include "animal/models/dat_8c1864d4.inc"
};
static NJS_MKEY_F dat_8c1864f4[2] = {
#include "animal/models/dat_8c1864f4.inc"
};
static NJS_MKEY_A dat_8c186514[2] = {
#include "animal/models/dat_8c186514.inc"
};
static NJS_MKEY_F dat_8c186534[2] = {
#include "animal/models/dat_8c186534.inc"
};
static NJS_MKEY_A dat_8c186554[17] = {
#include "animal/models/dat_8c186554.inc"
};
static NJS_MKEY_F dat_8c186664[2] = {
#include "animal/models/dat_8c186664.inc"
};
static NJS_MKEY_A dat_8c186684[2] = {
#include "animal/models/dat_8c186684.inc"
};
static NJS_MKEY_F dat_8c1866a4[2] = {
#include "animal/models/dat_8c1866a4.inc"
};
static NJS_MKEY_A dat_8c1866c4[2] = {
#include "animal/models/dat_8c1866c4.inc"
};
static NJS_MKEY_F dat_8c1866e4[2] = {
#include "animal/models/dat_8c1866e4.inc"
};
static NJS_MKEY_A dat_8c186704[2] = {
#include "animal/models/dat_8c186704.inc"
};
static NJS_MKEY_F dat_8c186724[2] = {
#include "animal/models/dat_8c186724.inc"
};
static NJS_MKEY_A dat_8c186744[2] = {
#include "animal/models/dat_8c186744.inc"
};
static NJS_MKEY_F dat_8c186764[2] = {
#include "animal/models/dat_8c186764.inc"
};
static NJS_MKEY_A dat_8c186784[17] = {
#include "animal/models/dat_8c186784.inc"
};
static NJS_MKEY_F dat_8c186894[2] = {
#include "animal/models/dat_8c186894.inc"
};
static NJS_MKEY_A dat_8c1868b4[2] = {
#include "animal/models/dat_8c1868b4.inc"
};
static NJS_MKEY_F dat_8c1868d4[2] = {
#include "animal/models/dat_8c1868d4.inc"
};
static NJS_MKEY_A dat_8c1868f4[2] = {
#include "animal/models/dat_8c1868f4.inc"
};
static NJS_MKEY_F dat_8c186914[2] = {
#include "animal/models/dat_8c186914.inc"
};
static NJS_MKEY_A dat_8c186934[2] = {
#include "animal/models/dat_8c186934.inc"
};
static NJS_MKEY_F dat_8c186954[2] = {
#include "animal/models/dat_8c186954.inc"
};
static NJS_MKEY_A dat_8c186974[17] = {
#include "animal/models/dat_8c186974.inc"
};
static NJS_MKEY_F dat_8c186a84[2] = {
#include "animal/models/dat_8c186a84.inc"
};
static NJS_MKEY_A dat_8c186aa4[2] = {
#include "animal/models/dat_8c186aa4.inc"
};
static NJS_MKEY_F dat_8c186ac4[2] = {
#include "animal/models/dat_8c186ac4.inc"
};
static NJS_MKEY_A dat_8c186ae4[2] = {
#include "animal/models/dat_8c186ae4.inc"
};
static NJS_MKEY_F dat_8c186b04[17] = {
#include "animal/models/dat_8c186b04.inc"
};
static NJS_MKEY_A dat_8c186c14[2] = {
#include "animal/models/dat_8c186c14.inc"
};
static NJS_MKEY_F dat_8c186c34[2] = {
#include "animal/models/dat_8c186c34.inc"
};
static NJS_MKEY_A dat_8c186c54[17] = {
#include "animal/models/dat_8c186c54.inc"
};
static NJS_MKEY_F dat_8c186d64[2] = {
#include "animal/models/dat_8c186d64.inc"
};
static NJS_MKEY_A dat_8c186d84[17] = {
#include "animal/models/dat_8c186d84.inc"
};
static NJS_MKEY_F dat_8c186e94[2] = {
#include "animal/models/dat_8c186e94.inc"
};
static NJS_MKEY_A dat_8c186eb4[2] = {
#include "animal/models/dat_8c186eb4.inc"
};
static NJS_MKEY_F dat_8c186ed4[2] = {
#include "animal/models/dat_8c186ed4.inc"
};
static NJS_MKEY_A dat_8c186ef4[2] = {
#include "animal/models/dat_8c186ef4.inc"
};
static NJS_MKEY_F dat_8c186f14[2] = {
#include "animal/models/dat_8c186f14.inc"
};
static NJS_MKEY_A dat_8c186f34[2] = {
#include "animal/models/dat_8c186f34.inc"
};
static NJS_MKEY_F dat_8c186f54[17] = {
#include "animal/models/dat_8c186f54.inc"
};
static NJS_MKEY_A dat_8c187064[2] = {
#include "animal/models/dat_8c187064.inc"
};
static NJS_MKEY_F dat_8c187084[2] = {
#include "animal/models/dat_8c187084.inc"
};
static NJS_MKEY_A dat_8c1870a4[17] = {
#include "animal/models/dat_8c1870a4.inc"
};
static NJS_MKEY_F dat_8c1871b4[2] = {
#include "animal/models/dat_8c1871b4.inc"
};
static NJS_MKEY_A dat_8c1871d4[17] = {
#include "animal/models/dat_8c1871d4.inc"
};
static NJS_MKEY_F dat_8c1872e4[2] = {
#include "animal/models/dat_8c1872e4.inc"
};
static NJS_MKEY_A dat_8c187304[2] = {
#include "animal/models/dat_8c187304.inc"
};
static NJS_MKEY_F dat_8c187324[2] = {
#include "animal/models/dat_8c187324.inc"
};
static NJS_MKEY_A dat_8c187344[2] = {
#include "animal/models/dat_8c187344.inc"
};
static NJS_MKEY_F dat_8c187364[2] = {
#include "animal/models/dat_8c187364.inc"
};
static NJS_MKEY_A dat_8c187384[2] = {
#include "animal/models/dat_8c187384.inc"
};
static NJS_MKEY_F dat_8c1873a4[2] = {
#include "animal/models/dat_8c1873a4.inc"
};
static NJS_MKEY_A dat_8c1873c4[2] = {
#include "animal/models/dat_8c1873c4.inc"
};
static NJS_MDATA2 dat_8c1873e4[37] = {
#include "animal/models/dat_8c1873e4.inc"
};
static NJS_MOTION koar_motion = {
#include "animal/models/koar_motion.inc"
};
NJS_ACTION koar_action = {
#include "animal/models/koar_action.inc"
};
static NJS_TEXNAME koar_textures[4] = {
    {TEXT_26, 0, 0},
    {TEXT_27, 0, 0},
    {TEXT_28, 0, 0},
    {TEXT_29, 0, 0},
};
NJS_TEXLIST koar_texlist = {
#include "animal/models/koar_texlist.inc"
};
static NJS_MATERIAL dat_8c187680[1] = {
#include "animal/models/dat_8c187680.inc"
};
static Sint16 dat_8c187694[14] = {
#include "animal/models/dat_8c187694.inc"
};
static NJS_COLOR dat_8c1876b0[12] = {
#include "animal/models/dat_8c1876b0.inc"
};
static NJS_MESHSET dat_8c1876e0[1] = {
#include "animal/models/dat_8c1876e0.inc"
};
static NJS_POINT3 dat_8c1876f8[6] = {
#include "animal/models/dat_8c1876f8.inc"
};
static NJS_VECTOR dat_8c187740[6] = {
#include "animal/models/dat_8c187740.inc"
};
static NJS_MODEL dat_8c187788 = {
#include "animal/models/dat_8c187788.inc"
};
static NJS_OBJECT dat_8c1877b0 = {
#include "animal/models/dat_8c1877b0.inc"
};
static NJS_MATERIAL dat_8c1877e4[1] = {
#include "animal/models/dat_8c1877e4.inc"
};
static Sint16 dat_8c1877f8[16] = {
#include "animal/models/dat_8c1877f8.inc"
};
static NJS_COLOR dat_8c187818[12] = {
#include "animal/models/dat_8c187818.inc"
};
static NJS_MESHSET dat_8c187848[1] = {
#include "animal/models/dat_8c187848.inc"
};
static NJS_POINT3 dat_8c187860[4] = {
#include "animal/models/dat_8c187860.inc"
};
static NJS_VECTOR dat_8c187890[4] = {
#include "animal/models/dat_8c187890.inc"
};
static NJS_MODEL dat_8c1878c0 = {
#include "animal/models/dat_8c1878c0.inc"
};
static NJS_OBJECT dat_8c1878e8 = {
#include "animal/models/dat_8c1878e8.inc"
};
static NJS_OBJECT dat_8c18791c = {
#include "animal/models/dat_8c18791c.inc"
};
static NJS_OBJECT dat_8c187950 = {
#include "animal/models/dat_8c187950.inc"
};
static NJS_OBJECT dat_8c187984 = {
#include "animal/models/dat_8c187984.inc"
};
static NJS_OBJECT dat_8c1879b8 = {
#include "animal/models/dat_8c1879b8.inc"
};
static NJS_MATERIAL dat_8c1879ec[1] = {
#include "animal/models/dat_8c1879ec.inc"
};
static Sint16 dat_8c187a00[7] = {
#include "animal/models/dat_8c187a00.inc"
};
static NJS_COLOR dat_8c187a10[6] = {
#include "animal/models/dat_8c187a10.inc"
};
static NJS_MESHSET dat_8c187a28[1] = {
#include "animal/models/dat_8c187a28.inc"
};
static NJS_POINT3 dat_8c187a40[4] = {
#include "animal/models/dat_8c187a40.inc"
};
static NJS_VECTOR dat_8c187a70[4] = {
#include "animal/models/dat_8c187a70.inc"
};
static NJS_MODEL dat_8c187aa0 = {
#include "animal/models/dat_8c187aa0.inc"
};
static NJS_OBJECT dat_8c187ac8 = {
#include "animal/models/dat_8c187ac8.inc"
};
static NJS_OBJECT dat_8c187afc = {
#include "animal/models/dat_8c187afc.inc"
};
static NJS_OBJECT dat_8c187b30 = {
#include "animal/models/dat_8c187b30.inc"
};
static NJS_OBJECT dat_8c187b64 = {
#include "animal/models/dat_8c187b64.inc"
};
static NJS_MATERIAL dat_8c187b98[1] = {
#include "animal/models/dat_8c187b98.inc"
};
static Sint16 dat_8c187bac[25] = {
#include "animal/models/dat_8c187bac.inc"
};
static NJS_COLOR dat_8c187be0[19] = {
#include "animal/models/dat_8c187be0.inc"
};
static NJS_MESHSET dat_8c187c2c[1] = {
#include "animal/models/dat_8c187c2c.inc"
};
static NJS_POINT3 dat_8c187c44[7] = {
#include "animal/models/dat_8c187c44.inc"
};
static NJS_VECTOR dat_8c187c98[7] = {
#include "animal/models/dat_8c187c98.inc"
};
static NJS_MODEL dat_8c187cec = {
#include "animal/models/dat_8c187cec.inc"
};
static NJS_OBJECT dat_8c187d14 = {
#include "animal/models/dat_8c187d14.inc"
};
static NJS_MATERIAL dat_8c187d48[1] = {
#include "animal/models/dat_8c187d48.inc"
};
static Sint16 dat_8c187d5c[12] = {
#include "animal/models/dat_8c187d5c.inc"
};
static NJS_COLOR dat_8c187d74[9] = {
#include "animal/models/dat_8c187d74.inc"
};
static NJS_MESHSET dat_8c187d98[1] = {
#include "animal/models/dat_8c187d98.inc"
};
static NJS_POINT3 dat_8c187db0[4] = {
#include "animal/models/dat_8c187db0.inc"
};
static NJS_VECTOR dat_8c187de0[4] = {
#include "animal/models/dat_8c187de0.inc"
};
static NJS_MODEL dat_8c187e10 = {
#include "animal/models/dat_8c187e10.inc"
};
static NJS_OBJECT dat_8c187e38 = {
#include "animal/models/dat_8c187e38.inc"
};
static NJS_OBJECT dat_8c187e6c = {
#include "animal/models/dat_8c187e6c.inc"
};
static NJS_OBJECT dat_8c187ea0 = {
#include "animal/models/dat_8c187ea0.inc"
};
static NJS_OBJECT dat_8c187ed4 = {
#include "animal/models/dat_8c187ed4.inc"
};
static NJS_OBJECT dat_8c187f08 = {
#include "animal/models/dat_8c187f08.inc"
};
static NJS_MATERIAL dat_8c187f3c[1] = {
#include "animal/models/dat_8c187f3c.inc"
};
static Sint16 dat_8c187f50[14] = {
#include "animal/models/dat_8c187f50.inc"
};
static NJS_COLOR dat_8c187f6c[12] = {
#include "animal/models/dat_8c187f6c.inc"
};
static NJS_MESHSET dat_8c187f9c[1] = {
#include "animal/models/dat_8c187f9c.inc"
};
static NJS_POINT3 dat_8c187fb4[6] = {
#include "animal/models/dat_8c187fb4.inc"
};
static NJS_VECTOR dat_8c187ffc[6] = {
#include "animal/models/dat_8c187ffc.inc"
};
static NJS_MODEL dat_8c188044 = {
#include "animal/models/dat_8c188044.inc"
};
static NJS_OBJECT dat_8c18806c = {
#include "animal/models/dat_8c18806c.inc"
};
static NJS_MATERIAL dat_8c1880a0[1] = {
#include "animal/models/dat_8c1880a0.inc"
};
static Sint16 dat_8c1880b4[13] = {
#include "animal/models/dat_8c1880b4.inc"
};
static NJS_COLOR dat_8c1880d0[10] = {
#include "animal/models/dat_8c1880d0.inc"
};
static NJS_MESHSET dat_8c1880f8[1] = {
#include "animal/models/dat_8c1880f8.inc"
};
static NJS_POINT3 dat_8c188110[4] = {
#include "animal/models/dat_8c188110.inc"
};
static NJS_VECTOR dat_8c188140[4] = {
#include "animal/models/dat_8c188140.inc"
};
static NJS_MODEL dat_8c188170 = {
#include "animal/models/dat_8c188170.inc"
};
static NJS_OBJECT dat_8c188198 = {
#include "animal/models/dat_8c188198.inc"
};
static NJS_OBJECT dat_8c1881cc = {
#include "animal/models/dat_8c1881cc.inc"
};
static NJS_OBJECT dat_8c188200 = {
#include "animal/models/dat_8c188200.inc"
};
static NJS_OBJECT dat_8c188234 = {
#include "animal/models/dat_8c188234.inc"
};
static NJS_OBJECT dat_8c188268 = {
#include "animal/models/dat_8c188268.inc"
};
static NJS_MATERIAL dat_8c18829c[1] = {
#include "animal/models/dat_8c18829c.inc"
};
static Sint16 dat_8c1882b0[7] = {
#include "animal/models/dat_8c1882b0.inc"
};
static NJS_COLOR dat_8c1882c0[6] = {
#include "animal/models/dat_8c1882c0.inc"
};
static NJS_MESHSET dat_8c1882d8[1] = {
#include "animal/models/dat_8c1882d8.inc"
};
static NJS_POINT3 dat_8c1882f0[4] = {
#include "animal/models/dat_8c1882f0.inc"
};
static NJS_VECTOR dat_8c188320[4] = {
#include "animal/models/dat_8c188320.inc"
};
static NJS_MODEL dat_8c188350 = {
#include "animal/models/dat_8c188350.inc"
};
static NJS_OBJECT dat_8c188378 = {
#include "animal/models/dat_8c188378.inc"
};
static NJS_OBJECT dat_8c1883ac = {
#include "animal/models/dat_8c1883ac.inc"
};
static NJS_OBJECT dat_8c1883e0 = {
#include "animal/models/dat_8c1883e0.inc"
};
static NJS_OBJECT dat_8c188414 = {
#include "animal/models/dat_8c188414.inc"
};
static NJS_MATERIAL dat_8c188448[1] = {
#include "animal/models/dat_8c188448.inc"
};
static Sint16 dat_8c18845c[10] = {
#include "animal/models/dat_8c18845c.inc"
};
static NJS_COLOR dat_8c188470[8] = {
#include "animal/models/dat_8c188470.inc"
};
static NJS_MESHSET dat_8c188490[1] = {
#include "animal/models/dat_8c188490.inc"
};
static NJS_POINT3 dat_8c1884a8[5] = {
#include "animal/models/dat_8c1884a8.inc"
};
static NJS_VECTOR dat_8c1884e4[5] = {
#include "animal/models/dat_8c1884e4.inc"
};
static NJS_MODEL dat_8c188520 = {
#include "animal/models/dat_8c188520.inc"
};
static NJS_OBJECT dat_8c188548 = {
#include "animal/models/dat_8c188548.inc"
};
static NJS_OBJECT dat_8c18857c = {
#include "animal/models/dat_8c18857c.inc"
};
static NJS_OBJECT dat_8c1885b0 = {
#include "animal/models/dat_8c1885b0.inc"
};
static NJS_OBJECT dat_8c1885e4 = {
#include "animal/models/dat_8c1885e4.inc"
};
static NJS_MATERIAL dat_8c188618[1] = {
#include "animal/models/dat_8c188618.inc"
};
static Sint16 dat_8c18862c[10] = {
#include "animal/models/dat_8c18862c.inc"
};
static NJS_COLOR dat_8c188640[8] = {
#include "animal/models/dat_8c188640.inc"
};
static NJS_MESHSET dat_8c188660[1] = {
#include "animal/models/dat_8c188660.inc"
};
static NJS_POINT3 dat_8c188678[5] = {
#include "animal/models/dat_8c188678.inc"
};
static NJS_VECTOR dat_8c1886b4[5] = {
#include "animal/models/dat_8c1886b4.inc"
};
static NJS_MODEL dat_8c1886f0 = {
#include "animal/models/dat_8c1886f0.inc"
};
static NJS_OBJECT dat_8c188718 = {
#include "animal/models/dat_8c188718.inc"
};
static NJS_OBJECT dat_8c18874c = {
#include "animal/models/dat_8c18874c.inc"
};
static NJS_OBJECT dat_8c188780 = {
#include "animal/models/dat_8c188780.inc"
};
static NJS_OBJECT dat_8c1887b4 = {
#include "animal/models/dat_8c1887b4.inc"
};
static NJS_MATERIAL dat_8c1887e8[1] = {
#include "animal/models/dat_8c1887e8.inc"
};
static Sint16 dat_8c1887fc[39] = {
#include "animal/models/dat_8c1887fc.inc"
};
static NJS_COLOR dat_8c18884c[34] = {
#include "animal/models/dat_8c18884c.inc"
};
static NJS_MESHSET dat_8c1888d4[1] = {
#include "animal/models/dat_8c1888d4.inc"
};
static NJS_POINT3 dat_8c1888ec[14] = {
#include "animal/models/dat_8c1888ec.inc"
};
static NJS_VECTOR dat_8c188994[14] = {
#include "animal/models/dat_8c188994.inc"
};
static NJS_MODEL dat_8c188a3c = {
#include "animal/models/dat_8c188a3c.inc"
};
static NJS_OBJECT dat_8c188a64 = {
#include "animal/models/dat_8c188a64.inc"
};
static NJS_OBJECT dat_8c188a98 = {
#include "animal/models/dat_8c188a98.inc"
};
static NJS_OBJECT dat_8c188acc = {
#include "animal/models/dat_8c188acc.inc"
};
static NJS_OBJECT dat_8c188b00 = {
#include "animal/models/dat_8c188b00.inc"
};
static NJS_MATERIAL dat_8c188b34[1] = {
#include "animal/models/dat_8c188b34.inc"
};
static Sint16 dat_8c188b48[41] = {
#include "animal/models/dat_8c188b48.inc"
};
static NJS_COLOR dat_8c188b9c[35] = {
#include "animal/models/dat_8c188b9c.inc"
};
static NJS_MESHSET dat_8c188c28[1] = {
#include "animal/models/dat_8c188c28.inc"
};
static NJS_POINT3 dat_8c188c40[14] = {
#include "animal/models/dat_8c188c40.inc"
};
static NJS_VECTOR dat_8c188ce8[14] = {
#include "animal/models/dat_8c188ce8.inc"
};
static NJS_MODEL dat_8c188d90 = {
#include "animal/models/dat_8c188d90.inc"
};
static NJS_OBJECT dat_8c188db8 = {
#include "animal/models/dat_8c188db8.inc"
};
static NJS_OBJECT dat_8c188dec = {
#include "animal/models/dat_8c188dec.inc"
};
static NJS_OBJECT dat_8c188e20 = {
#include "animal/models/dat_8c188e20.inc"
};
static NJS_OBJECT dat_8c188e54 = {
#include "animal/models/dat_8c188e54.inc"
};
static NJS_OBJECT wara_object = {
#include "animal/models/wara_object.inc"
};
static NJS_MKEY_F dat_8c188ebc[2] = {
#include "animal/models/dat_8c188ebc.inc"
};
static NJS_MKEY_A dat_8c188edc[2] = {
#include "animal/models/dat_8c188edc.inc"
};
static NJS_MKEY_F dat_8c188efc[2] = {
#include "animal/models/dat_8c188efc.inc"
};
static NJS_MKEY_A dat_8c188f1c[2] = {
#include "animal/models/dat_8c188f1c.inc"
};
static NJS_MKEY_F dat_8c188f3c[2] = {
#include "animal/models/dat_8c188f3c.inc"
};
static NJS_MKEY_A dat_8c188f5c[17] = {
#include "animal/models/dat_8c188f5c.inc"
};
static NJS_MKEY_F dat_8c18906c[2] = {
#include "animal/models/dat_8c18906c.inc"
};
static NJS_MKEY_A dat_8c18908c[2] = {
#include "animal/models/dat_8c18908c.inc"
};
static NJS_MKEY_F dat_8c1890ac[2] = {
#include "animal/models/dat_8c1890ac.inc"
};
static NJS_MKEY_A dat_8c1890cc[2] = {
#include "animal/models/dat_8c1890cc.inc"
};
static NJS_MKEY_F dat_8c1890ec[17] = {
#include "animal/models/dat_8c1890ec.inc"
};
static NJS_MKEY_A dat_8c1891fc[2] = {
#include "animal/models/dat_8c1891fc.inc"
};
static NJS_MKEY_F dat_8c18921c[2] = {
#include "animal/models/dat_8c18921c.inc"
};
static NJS_MKEY_A dat_8c18923c[17] = {
#include "animal/models/dat_8c18923c.inc"
};
static NJS_MKEY_F dat_8c18934c[2] = {
#include "animal/models/dat_8c18934c.inc"
};
static NJS_MKEY_A dat_8c18936c[2] = {
#include "animal/models/dat_8c18936c.inc"
};
static NJS_MKEY_F dat_8c18938c[2] = {
#include "animal/models/dat_8c18938c.inc"
};
static NJS_MKEY_A dat_8c1893ac[2] = {
#include "animal/models/dat_8c1893ac.inc"
};
static NJS_MKEY_F dat_8c1893cc[2] = {
#include "animal/models/dat_8c1893cc.inc"
};
static NJS_MKEY_A dat_8c1893ec[2] = {
#include "animal/models/dat_8c1893ec.inc"
};
static NJS_MKEY_F dat_8c18940c[2] = {
#include "animal/models/dat_8c18940c.inc"
};
static NJS_MKEY_A dat_8c18942c[17] = {
#include "animal/models/dat_8c18942c.inc"
};
static NJS_MKEY_F dat_8c18953c[2] = {
#include "animal/models/dat_8c18953c.inc"
};
static NJS_MKEY_A dat_8c18955c[2] = {
#include "animal/models/dat_8c18955c.inc"
};
static NJS_MKEY_F dat_8c18957c[2] = {
#include "animal/models/dat_8c18957c.inc"
};
static NJS_MKEY_A dat_8c18959c[2] = {
#include "animal/models/dat_8c18959c.inc"
};
static NJS_MKEY_F dat_8c1895bc[2] = {
#include "animal/models/dat_8c1895bc.inc"
};
static NJS_MKEY_A dat_8c1895dc[2] = {
#include "animal/models/dat_8c1895dc.inc"
};
static NJS_MKEY_F dat_8c1895fc[2] = {
#include "animal/models/dat_8c1895fc.inc"
};
static NJS_MKEY_A dat_8c18961c[17] = {
#include "animal/models/dat_8c18961c.inc"
};
static NJS_MKEY_F dat_8c18972c[2] = {
#include "animal/models/dat_8c18972c.inc"
};
static NJS_MKEY_A dat_8c18974c[2] = {
#include "animal/models/dat_8c18974c.inc"
};
static NJS_MKEY_F dat_8c18976c[2] = {
#include "animal/models/dat_8c18976c.inc"
};
static NJS_MKEY_A dat_8c18978c[2] = {
#include "animal/models/dat_8c18978c.inc"
};
static NJS_MKEY_F dat_8c1897ac[17] = {
#include "animal/models/dat_8c1897ac.inc"
};
static NJS_MKEY_A dat_8c1898bc[2] = {
#include "animal/models/dat_8c1898bc.inc"
};
static NJS_MKEY_F dat_8c1898dc[2] = {
#include "animal/models/dat_8c1898dc.inc"
};
static NJS_MKEY_A dat_8c1898fc[17] = {
#include "animal/models/dat_8c1898fc.inc"
};
static NJS_MKEY_F dat_8c189a0c[2] = {
#include "animal/models/dat_8c189a0c.inc"
};
static NJS_MKEY_A dat_8c189a2c[2] = {
#include "animal/models/dat_8c189a2c.inc"
};
static NJS_MKEY_F dat_8c189a4c[2] = {
#include "animal/models/dat_8c189a4c.inc"
};
static NJS_MKEY_A dat_8c189a6c[2] = {
#include "animal/models/dat_8c189a6c.inc"
};
static NJS_MKEY_F dat_8c189a8c[2] = {
#include "animal/models/dat_8c189a8c.inc"
};
static NJS_MKEY_A dat_8c189aac[2] = {
#include "animal/models/dat_8c189aac.inc"
};
static NJS_MKEY_F dat_8c189acc[2] = {
#include "animal/models/dat_8c189acc.inc"
};
static NJS_MKEY_A dat_8c189aec[17] = {
#include "animal/models/dat_8c189aec.inc"
};
static NJS_MKEY_F dat_8c189bfc[2] = {
#include "animal/models/dat_8c189bfc.inc"
};
static NJS_MKEY_A dat_8c189c1c[17] = {
#include "animal/models/dat_8c189c1c.inc"
};
static NJS_MKEY_F dat_8c189d2c[2] = {
#include "animal/models/dat_8c189d2c.inc"
};
static NJS_MKEY_A dat_8c189d4c[2] = {
#include "animal/models/dat_8c189d4c.inc"
};
static NJS_MKEY_F dat_8c189d6c[2] = {
#include "animal/models/dat_8c189d6c.inc"
};
static NJS_MKEY_A dat_8c189d8c[2] = {
#include "animal/models/dat_8c189d8c.inc"
};
static NJS_MKEY_F dat_8c189dac[2] = {
#include "animal/models/dat_8c189dac.inc"
};
static NJS_MKEY_A dat_8c189dcc[2] = {
#include "animal/models/dat_8c189dcc.inc"
};
static NJS_MKEY_F dat_8c189dec[2] = {
#include "animal/models/dat_8c189dec.inc"
};
static NJS_MKEY_A dat_8c189e0c[2] = {
#include "animal/models/dat_8c189e0c.inc"
};
static NJS_MKEY_F dat_8c189e2c[2] = {
#include "animal/models/dat_8c189e2c.inc"
};
static NJS_MKEY_A dat_8c189e4c[17] = {
#include "animal/models/dat_8c189e4c.inc"
};
static NJS_MKEY_F dat_8c189f5c[2] = {
#include "animal/models/dat_8c189f5c.inc"
};
static NJS_MKEY_A dat_8c189f7c[17] = {
#include "animal/models/dat_8c189f7c.inc"
};
static NJS_MKEY_F dat_8c18a08c[2] = {
#include "animal/models/dat_8c18a08c.inc"
};
static NJS_MKEY_A dat_8c18a0ac[2] = {
#include "animal/models/dat_8c18a0ac.inc"
};
static NJS_MKEY_F dat_8c18a0cc[2] = {
#include "animal/models/dat_8c18a0cc.inc"
};
static NJS_MKEY_A dat_8c18a0ec[2] = {
#include "animal/models/dat_8c18a0ec.inc"
};
static NJS_MKEY_F dat_8c18a10c[2] = {
#include "animal/models/dat_8c18a10c.inc"
};
static NJS_MKEY_A dat_8c18a12c[2] = {
#include "animal/models/dat_8c18a12c.inc"
};
static NJS_MKEY_F dat_8c18a14c[17] = {
#include "animal/models/dat_8c18a14c.inc"
};
static NJS_MKEY_A dat_8c18a25c[2] = {
#include "animal/models/dat_8c18a25c.inc"
};
static NJS_MKEY_F dat_8c18a27c[2] = {
#include "animal/models/dat_8c18a27c.inc"
};
static NJS_MKEY_A dat_8c18a29c[17] = {
#include "animal/models/dat_8c18a29c.inc"
};
static NJS_MKEY_F dat_8c18a3ac[2] = {
#include "animal/models/dat_8c18a3ac.inc"
};
static NJS_MKEY_A dat_8c18a3cc[2] = {
#include "animal/models/dat_8c18a3cc.inc"
};
static NJS_MKEY_F dat_8c18a3ec[2] = {
#include "animal/models/dat_8c18a3ec.inc"
};
static NJS_MKEY_A dat_8c18a40c[2] = {
#include "animal/models/dat_8c18a40c.inc"
};
static NJS_MKEY_F dat_8c18a42c[2] = {
#include "animal/models/dat_8c18a42c.inc"
};
static NJS_MKEY_A dat_8c18a44c[2] = {
#include "animal/models/dat_8c18a44c.inc"
};
static NJS_MKEY_F dat_8c18a46c[2] = {
#include "animal/models/dat_8c18a46c.inc"
};
static NJS_MKEY_A dat_8c18a48c[17] = {
#include "animal/models/dat_8c18a48c.inc"
};
static NJS_MKEY_F dat_8c18a59c[2] = {
#include "animal/models/dat_8c18a59c.inc"
};
static NJS_MKEY_A dat_8c18a5bc[17] = {
#include "animal/models/dat_8c18a5bc.inc"
};
static NJS_MKEY_F dat_8c18a6cc[2] = {
#include "animal/models/dat_8c18a6cc.inc"
};
static NJS_MKEY_A dat_8c18a6ec[2] = {
#include "animal/models/dat_8c18a6ec.inc"
};
static NJS_MKEY_F dat_8c18a70c[2] = {
#include "animal/models/dat_8c18a70c.inc"
};
static NJS_MKEY_A dat_8c18a72c[2] = {
#include "animal/models/dat_8c18a72c.inc"
};
static NJS_MKEY_F dat_8c18a74c[2] = {
#include "animal/models/dat_8c18a74c.inc"
};
static NJS_MKEY_A dat_8c18a76c[2] = {
#include "animal/models/dat_8c18a76c.inc"
};
static NJS_MDATA2 dat_8c18a78c[43] = {
#include "animal/models/dat_8c18a78c.inc"
};
static NJS_MOTION wara_motion = {
#include "animal/models/wara_motion.inc"
};
NJS_ACTION wara_action = {
#include "animal/models/wara_action.inc"
};
static NJS_TEXNAME wara_textures[4] = {
    {TEXT_30, 0, 0},
    {TEXT_31, 0, 0},
    {TEXT_32, 0, 0},
    {TEXT_33, 0, 0},
};
NJS_TEXLIST wara_texlist = {
#include "animal/models/wara_texlist.inc"
};
static NJS_MATERIAL dat_8c18aa88[1] = {
#include "animal/models/dat_8c18aa88.inc"
};
static Sint16 dat_8c18aa9c[12] = {
#include "animal/models/dat_8c18aa9c.inc"
};
static NJS_COLOR dat_8c18aab4[10] = {
#include "animal/models/dat_8c18aab4.inc"
};
static NJS_MESHSET dat_8c18aadc[1] = {
#include "animal/models/dat_8c18aadc.inc"
};
static NJS_POINT3 dat_8c18aaf4[7] = {
#include "animal/models/dat_8c18aaf4.inc"
};
static NJS_VECTOR dat_8c18ab48[7] = {
#include "animal/models/dat_8c18ab48.inc"
};
static NJS_MODEL dat_8c18ab9c = {
#include "animal/models/dat_8c18ab9c.inc"
};
static NJS_OBJECT dat_8c18abc4 = {
#include "animal/models/dat_8c18abc4.inc"
};
static NJS_OBJECT dat_8c18abf8 = {
#include "animal/models/dat_8c18abf8.inc"
};
static NJS_OBJECT dat_8c18ac2c = {
#include "animal/models/dat_8c18ac2c.inc"
};
static NJS_OBJECT dat_8c18ac60 = {
#include "animal/models/dat_8c18ac60.inc"
};
static NJS_MATERIAL dat_8c18ac94[1] = {
#include "animal/models/dat_8c18ac94.inc"
};
static Sint16 dat_8c18aca8[12] = {
#include "animal/models/dat_8c18aca8.inc"
};
static NJS_COLOR dat_8c18acc0[10] = {
#include "animal/models/dat_8c18acc0.inc"
};
static NJS_MESHSET dat_8c18ace8[1] = {
#include "animal/models/dat_8c18ace8.inc"
};
static NJS_POINT3 dat_8c18ad00[7] = {
#include "animal/models/dat_8c18ad00.inc"
};
static NJS_VECTOR dat_8c18ad54[7] = {
#include "animal/models/dat_8c18ad54.inc"
};
static NJS_MODEL dat_8c18ada8 = {
#include "animal/models/dat_8c18ada8.inc"
};
static NJS_OBJECT dat_8c18add0 = {
#include "animal/models/dat_8c18add0.inc"
};
static NJS_OBJECT dat_8c18ae04 = {
#include "animal/models/dat_8c18ae04.inc"
};
static NJS_OBJECT dat_8c18ae38 = {
#include "animal/models/dat_8c18ae38.inc"
};
static NJS_OBJECT dat_8c18ae6c = {
#include "animal/models/dat_8c18ae6c.inc"
};
static NJS_MATERIAL dat_8c18aea0[1] = {
#include "animal/models/dat_8c18aea0.inc"
};
static Sint16 dat_8c18aeb4[63] = {
#include "animal/models/dat_8c18aeb4.inc"
};
static NJS_COLOR dat_8c18af34[54] = {
#include "animal/models/dat_8c18af34.inc"
};
static NJS_MESHSET dat_8c18b00c[1] = {
#include "animal/models/dat_8c18b00c.inc"
};
static NJS_POINT3 dat_8c18b024[20] = {
#include "animal/models/dat_8c18b024.inc"
};
static NJS_VECTOR dat_8c18b114[20] = {
#include "animal/models/dat_8c18b114.inc"
};
static NJS_MODEL dat_8c18b204 = {
#include "animal/models/dat_8c18b204.inc"
};
static NJS_OBJECT dat_8c18b22c = {
#include "animal/models/dat_8c18b22c.inc"
};
static NJS_OBJECT dat_8c18b260 = {
#include "animal/models/dat_8c18b260.inc"
};
static NJS_OBJECT dat_8c18b294 = {
#include "animal/models/dat_8c18b294.inc"
};
static NJS_OBJECT dat_8c18b2c8 = {
#include "animal/models/dat_8c18b2c8.inc"
};
static NJS_MATERIAL dat_8c18b2fc[1] = {
#include "animal/models/dat_8c18b2fc.inc"
};
static Sint16 dat_8c18b310[15] = {
#include "animal/models/dat_8c18b310.inc"
};
static NJS_COLOR dat_8c18b330[12] = {
#include "animal/models/dat_8c18b330.inc"
};
static NJS_MESHSET dat_8c18b360[1] = {
#include "animal/models/dat_8c18b360.inc"
};
static NJS_POINT3 dat_8c18b378[5] = {
#include "animal/models/dat_8c18b378.inc"
};
static NJS_VECTOR dat_8c18b3b4[5] = {
#include "animal/models/dat_8c18b3b4.inc"
};
static NJS_MODEL dat_8c18b3f0 = {
#include "animal/models/dat_8c18b3f0.inc"
};
static NJS_OBJECT dat_8c18b418 = {
#include "animal/models/dat_8c18b418.inc"
};
static NJS_OBJECT dat_8c18b44c = {
#include "animal/models/dat_8c18b44c.inc"
};
static NJS_OBJECT dat_8c18b480 = {
#include "animal/models/dat_8c18b480.inc"
};
static NJS_OBJECT dat_8c18b4b4 = {
#include "animal/models/dat_8c18b4b4.inc"
};
static NJS_MATERIAL dat_8c18b4e8[1] = {
#include "animal/models/dat_8c18b4e8.inc"
};
static Sint16 dat_8c18b4fc[37] = {
#include "animal/models/dat_8c18b4fc.inc"
};
static NJS_COLOR dat_8c18b548[33] = {
#include "animal/models/dat_8c18b548.inc"
};
static NJS_MESHSET dat_8c18b5cc[1] = {
#include "animal/models/dat_8c18b5cc.inc"
};
static NJS_POINT3 dat_8c18b5e4[15] = {
#include "animal/models/dat_8c18b5e4.inc"
};
static NJS_VECTOR dat_8c18b698[15] = {
#include "animal/models/dat_8c18b698.inc"
};
static NJS_MODEL dat_8c18b74c = {
#include "animal/models/dat_8c18b74c.inc"
};
static NJS_OBJECT dat_8c18b774 = {
#include "animal/models/dat_8c18b774.inc"
};
static NJS_MATERIAL dat_8c18b7a8[1] = {
#include "animal/models/dat_8c18b7a8.inc"
};
static Sint16 dat_8c18b7bc[17] = {
#include "animal/models/dat_8c18b7bc.inc"
};
static NJS_COLOR dat_8c18b7e0[14] = {
#include "animal/models/dat_8c18b7e0.inc"
};
static NJS_MESHSET dat_8c18b818[1] = {
#include "animal/models/dat_8c18b818.inc"
};
static NJS_POINT3 dat_8c18b830[6] = {
#include "animal/models/dat_8c18b830.inc"
};
static NJS_VECTOR dat_8c18b878[6] = {
#include "animal/models/dat_8c18b878.inc"
};
static NJS_MODEL dat_8c18b8c0 = {
#include "animal/models/dat_8c18b8c0.inc"
};
static NJS_OBJECT dat_8c18b8e8 = {
#include "animal/models/dat_8c18b8e8.inc"
};
static NJS_OBJECT dat_8c18b91c = {
#include "animal/models/dat_8c18b91c.inc"
};
static NJS_OBJECT dat_8c18b950 = {
#include "animal/models/dat_8c18b950.inc"
};
static NJS_OBJECT dat_8c18b984 = {
#include "animal/models/dat_8c18b984.inc"
};
static NJS_MATERIAL dat_8c18b9b8[1] = {
#include "animal/models/dat_8c18b9b8.inc"
};
static Sint16 dat_8c18b9cc[15] = {
#include "animal/models/dat_8c18b9cc.inc"
};
static NJS_COLOR dat_8c18b9ec[12] = {
#include "animal/models/dat_8c18b9ec.inc"
};
static NJS_MESHSET dat_8c18ba1c[1] = {
#include "animal/models/dat_8c18ba1c.inc"
};
static NJS_POINT3 dat_8c18ba34[5] = {
#include "animal/models/dat_8c18ba34.inc"
};
static NJS_VECTOR dat_8c18ba70[5] = {
#include "animal/models/dat_8c18ba70.inc"
};
static NJS_MODEL dat_8c18baac = {
#include "animal/models/dat_8c18baac.inc"
};
static NJS_OBJECT dat_8c18bad4 = {
#include "animal/models/dat_8c18bad4.inc"
};
static NJS_OBJECT dat_8c18bb08 = {
#include "animal/models/dat_8c18bb08.inc"
};
static NJS_OBJECT dat_8c18bb3c = {
#include "animal/models/dat_8c18bb3c.inc"
};
static NJS_OBJECT dat_8c18bb70 = {
#include "animal/models/dat_8c18bb70.inc"
};
static NJS_MATERIAL dat_8c18bba4[1] = {
#include "animal/models/dat_8c18bba4.inc"
};
static Sint16 dat_8c18bbb8[15] = {
#include "animal/models/dat_8c18bbb8.inc"
};
static NJS_COLOR dat_8c18bbd8[12] = {
#include "animal/models/dat_8c18bbd8.inc"
};
static NJS_MESHSET dat_8c18bc08[1] = {
#include "animal/models/dat_8c18bc08.inc"
};
static NJS_POINT3 dat_8c18bc20[5] = {
#include "animal/models/dat_8c18bc20.inc"
};
static NJS_VECTOR dat_8c18bc5c[5] = {
#include "animal/models/dat_8c18bc5c.inc"
};
static NJS_MODEL dat_8c18bc98 = {
#include "animal/models/dat_8c18bc98.inc"
};
static NJS_OBJECT dat_8c18bcc0 = {
#include "animal/models/dat_8c18bcc0.inc"
};
static NJS_OBJECT dat_8c18bcf4 = {
#include "animal/models/dat_8c18bcf4.inc"
};
static NJS_OBJECT dat_8c18bd28 = {
#include "animal/models/dat_8c18bd28.inc"
};
static NJS_OBJECT dat_8c18bd5c = {
#include "animal/models/dat_8c18bd5c.inc"
};
static NJS_MATERIAL dat_8c18bd90[1] = {
#include "animal/models/dat_8c18bd90.inc"
};
static Sint16 dat_8c18bda4[15] = {
#include "animal/models/dat_8c18bda4.inc"
};
static NJS_COLOR dat_8c18bdc4[12] = {
#include "animal/models/dat_8c18bdc4.inc"
};
static NJS_MESHSET dat_8c18bdf4[1] = {
#include "animal/models/dat_8c18bdf4.inc"
};
static NJS_POINT3 dat_8c18be0c[5] = {
#include "animal/models/dat_8c18be0c.inc"
};
static NJS_VECTOR dat_8c18be48[5] = {
#include "animal/models/dat_8c18be48.inc"
};
static NJS_MODEL dat_8c18be84 = {
#include "animal/models/dat_8c18be84.inc"
};
static NJS_OBJECT dat_8c18beac = {
#include "animal/models/dat_8c18beac.inc"
};
static NJS_OBJECT dat_8c18bee0 = {
#include "animal/models/dat_8c18bee0.inc"
};
static NJS_OBJECT dat_8c18bf14 = {
#include "animal/models/dat_8c18bf14.inc"
};
static NJS_OBJECT dat_8c18bf48 = {
#include "animal/models/dat_8c18bf48.inc"
};
static NJS_OBJECT dat_8c18bf7c = {
#include "animal/models/dat_8c18bf7c.inc"
};
static NJS_OBJECT dat_8c18bfb0 = {
#include "animal/models/dat_8c18bfb0.inc"
};
static NJS_OBJECT dat_8c18bfe4 = {
#include "animal/models/dat_8c18bfe4.inc"
};
static NJS_OBJECT banb_object = {
#include "animal/models/banb_object.inc"
};
static NJS_MKEY_F dat_8c18c04c[2] = {
#include "animal/models/dat_8c18c04c.inc"
};
static NJS_MKEY_A dat_8c18c06c[2] = {
#include "animal/models/dat_8c18c06c.inc"
};
static NJS_MKEY_F dat_8c18c08c[17] = {
#include "animal/models/dat_8c18c08c.inc"
};
static NJS_MKEY_A dat_8c18c19c[2] = {
#include "animal/models/dat_8c18c19c.inc"
};
static NJS_MKEY_F dat_8c18c1bc[2] = {
#include "animal/models/dat_8c18c1bc.inc"
};
static NJS_MKEY_A dat_8c18c1dc[17] = {
#include "animal/models/dat_8c18c1dc.inc"
};
static NJS_MKEY_F dat_8c18c2ec[2] = {
#include "animal/models/dat_8c18c2ec.inc"
};
static NJS_MKEY_A dat_8c18c30c[2] = {
#include "animal/models/dat_8c18c30c.inc"
};
static NJS_MKEY_F dat_8c18c32c[2] = {
#include "animal/models/dat_8c18c32c.inc"
};
static NJS_MKEY_A dat_8c18c34c[2] = {
#include "animal/models/dat_8c18c34c.inc"
};
static NJS_MKEY_F dat_8c18c36c[2] = {
#include "animal/models/dat_8c18c36c.inc"
};
static NJS_MKEY_A dat_8c18c38c[17] = {
#include "animal/models/dat_8c18c38c.inc"
};
static NJS_MKEY_F dat_8c18c49c[2] = {
#include "animal/models/dat_8c18c49c.inc"
};
static NJS_MKEY_A dat_8c18c4bc[2] = {
#include "animal/models/dat_8c18c4bc.inc"
};
static NJS_MKEY_F dat_8c18c4dc[2] = {
#include "animal/models/dat_8c18c4dc.inc"
};
static NJS_MKEY_A dat_8c18c4fc[2] = {
#include "animal/models/dat_8c18c4fc.inc"
};
static NJS_MKEY_F dat_8c18c51c[2] = {
#include "animal/models/dat_8c18c51c.inc"
};
static NJS_MKEY_A dat_8c18c53c[2] = {
#include "animal/models/dat_8c18c53c.inc"
};
static NJS_MKEY_F dat_8c18c55c[2] = {
#include "animal/models/dat_8c18c55c.inc"
};
static NJS_MKEY_A dat_8c18c57c[17] = {
#include "animal/models/dat_8c18c57c.inc"
};
static NJS_MKEY_F dat_8c18c68c[2] = {
#include "animal/models/dat_8c18c68c.inc"
};
static NJS_MKEY_A dat_8c18c6ac[2] = {
#include "animal/models/dat_8c18c6ac.inc"
};
static NJS_MKEY_F dat_8c18c6cc[2] = {
#include "animal/models/dat_8c18c6cc.inc"
};
static NJS_MKEY_A dat_8c18c6ec[2] = {
#include "animal/models/dat_8c18c6ec.inc"
};
static NJS_MKEY_F dat_8c18c70c[2] = {
#include "animal/models/dat_8c18c70c.inc"
};
static NJS_MKEY_A dat_8c18c72c[2] = {
#include "animal/models/dat_8c18c72c.inc"
};
static NJS_MKEY_F dat_8c18c74c[2] = {
#include "animal/models/dat_8c18c74c.inc"
};
static NJS_MKEY_A dat_8c18c76c[17] = {
#include "animal/models/dat_8c18c76c.inc"
};
static NJS_MKEY_F dat_8c18c87c[2] = {
#include "animal/models/dat_8c18c87c.inc"
};
static NJS_MKEY_A dat_8c18c89c[2] = {
#include "animal/models/dat_8c18c89c.inc"
};
static NJS_MKEY_F dat_8c18c8bc[2] = {
#include "animal/models/dat_8c18c8bc.inc"
};
static NJS_MKEY_A dat_8c18c8dc[2] = {
#include "animal/models/dat_8c18c8dc.inc"
};
static NJS_MKEY_F dat_8c18c8fc[2] = {
#include "animal/models/dat_8c18c8fc.inc"
};
static NJS_MKEY_A dat_8c18c91c[2] = {
#include "animal/models/dat_8c18c91c.inc"
};
static NJS_MKEY_F dat_8c18c93c[2] = {
#include "animal/models/dat_8c18c93c.inc"
};
static NJS_MKEY_A dat_8c18c95c[17] = {
#include "animal/models/dat_8c18c95c.inc"
};
static NJS_MKEY_F dat_8c18ca6c[2] = {
#include "animal/models/dat_8c18ca6c.inc"
};
static NJS_MKEY_A dat_8c18ca8c[2] = {
#include "animal/models/dat_8c18ca8c.inc"
};
static NJS_MKEY_F dat_8c18caac[2] = {
#include "animal/models/dat_8c18caac.inc"
};
static NJS_MKEY_A dat_8c18cacc[2] = {
#include "animal/models/dat_8c18cacc.inc"
};
static NJS_MKEY_F dat_8c18caec[2] = {
#include "animal/models/dat_8c18caec.inc"
};
static NJS_MKEY_A dat_8c18cb0c[2] = {
#include "animal/models/dat_8c18cb0c.inc"
};
static NJS_MKEY_F dat_8c18cb2c[2] = {
#include "animal/models/dat_8c18cb2c.inc"
};
static NJS_MKEY_A dat_8c18cb4c[2] = {
#include "animal/models/dat_8c18cb4c.inc"
};
static NJS_MKEY_F dat_8c18cb6c[2] = {
#include "animal/models/dat_8c18cb6c.inc"
};
static NJS_MKEY_A dat_8c18cb8c[17] = {
#include "animal/models/dat_8c18cb8c.inc"
};
static NJS_MKEY_F dat_8c18cc9c[2] = {
#include "animal/models/dat_8c18cc9c.inc"
};
static NJS_MKEY_A dat_8c18ccbc[2] = {
#include "animal/models/dat_8c18ccbc.inc"
};
static NJS_MKEY_F dat_8c18ccdc[2] = {
#include "animal/models/dat_8c18ccdc.inc"
};
static NJS_MKEY_A dat_8c18ccfc[2] = {
#include "animal/models/dat_8c18ccfc.inc"
};
static NJS_MKEY_F dat_8c18cd1c[17] = {
#include "animal/models/dat_8c18cd1c.inc"
};
static NJS_MKEY_A dat_8c18ce2c[2] = {
#include "animal/models/dat_8c18ce2c.inc"
};
static NJS_MKEY_F dat_8c18ce4c[2] = {
#include "animal/models/dat_8c18ce4c.inc"
};
static NJS_MKEY_A dat_8c18ce6c[17] = {
#include "animal/models/dat_8c18ce6c.inc"
};
static NJS_MKEY_F dat_8c18cf7c[2] = {
#include "animal/models/dat_8c18cf7c.inc"
};
static NJS_MKEY_A dat_8c18cf9c[2] = {
#include "animal/models/dat_8c18cf9c.inc"
};
static NJS_MKEY_F dat_8c18cfbc[2] = {
#include "animal/models/dat_8c18cfbc.inc"
};
static NJS_MKEY_A dat_8c18cfdc[2] = {
#include "animal/models/dat_8c18cfdc.inc"
};
static NJS_MKEY_F dat_8c18cffc[2] = {
#include "animal/models/dat_8c18cffc.inc"
};
static NJS_MKEY_A dat_8c18d01c[2] = {
#include "animal/models/dat_8c18d01c.inc"
};
static NJS_MKEY_F dat_8c18d03c[2] = {
#include "animal/models/dat_8c18d03c.inc"
};
static NJS_MKEY_A dat_8c18d05c[17] = {
#include "animal/models/dat_8c18d05c.inc"
};
static NJS_MKEY_F dat_8c18d16c[2] = {
#include "animal/models/dat_8c18d16c.inc"
};
static NJS_MKEY_A dat_8c18d18c[2] = {
#include "animal/models/dat_8c18d18c.inc"
};
static NJS_MKEY_F dat_8c18d1ac[2] = {
#include "animal/models/dat_8c18d1ac.inc"
};
static NJS_MKEY_A dat_8c18d1cc[2] = {
#include "animal/models/dat_8c18d1cc.inc"
};
static NJS_MKEY_F dat_8c18d1ec[2] = {
#include "animal/models/dat_8c18d1ec.inc"
};
static NJS_MKEY_A dat_8c18d20c[2] = {
#include "animal/models/dat_8c18d20c.inc"
};
static NJS_MKEY_F dat_8c18d22c[2] = {
#include "animal/models/dat_8c18d22c.inc"
};
static NJS_MKEY_A dat_8c18d24c[17] = {
#include "animal/models/dat_8c18d24c.inc"
};
static NJS_MKEY_F dat_8c18d35c[2] = {
#include "animal/models/dat_8c18d35c.inc"
};
static NJS_MKEY_A dat_8c18d37c[2] = {
#include "animal/models/dat_8c18d37c.inc"
};
static NJS_MKEY_F dat_8c18d39c[2] = {
#include "animal/models/dat_8c18d39c.inc"
};
static NJS_MKEY_A dat_8c18d3bc[2] = {
#include "animal/models/dat_8c18d3bc.inc"
};
static NJS_MDATA2 dat_8c18d3dc[37] = {
#include "animal/models/dat_8c18d3dc.inc"
};
static NJS_MOTION banb_motion = {
#include "animal/models/banb_motion.inc"
};
NJS_ACTION banb_action = {
#include "animal/models/banb_action.inc"
};
static NJS_TEXNAME banb_textures[4] = {
    {TEXT_34, 0, 0},
    {TEXT_35, 0, 0},
    {TEXT_36, 0, 0},
    {TEXT_37, 0, 0},
};
NJS_TEXLIST banb_texlist = {
#include "animal/models/banb_texlist.inc"
};
static NJS_MATERIAL dat_8c18d678[1] = {
#include "animal/models/dat_8c18d678.inc"
};
static Sint16 dat_8c18d68c[14] = {
#include "animal/models/dat_8c18d68c.inc"
};
static NJS_COLOR dat_8c18d6a8[11] = {
#include "animal/models/dat_8c18d6a8.inc"
};
static NJS_MESHSET dat_8c18d6d4[1] = {
#include "animal/models/dat_8c18d6d4.inc"
};
static NJS_POINT3 dat_8c18d6ec[7] = {
#include "animal/models/dat_8c18d6ec.inc"
};
static NJS_VECTOR dat_8c18d740[7] = {
#include "animal/models/dat_8c18d740.inc"
};
static NJS_MODEL dat_8c18d794 = {
#include "animal/models/dat_8c18d794.inc"
};
static NJS_OBJECT dat_8c18d7bc = {
#include "animal/models/dat_8c18d7bc.inc"
};
static NJS_OBJECT dat_8c18d7f0 = {
#include "animal/models/dat_8c18d7f0.inc"
};
static NJS_OBJECT dat_8c18d824 = {
#include "animal/models/dat_8c18d824.inc"
};
static NJS_OBJECT dat_8c18d858 = {
#include "animal/models/dat_8c18d858.inc"
};
static NJS_MATERIAL dat_8c18d88c[1] = {
#include "animal/models/dat_8c18d88c.inc"
};
static Sint16 dat_8c18d8a0[14] = {
#include "animal/models/dat_8c18d8a0.inc"
};
static NJS_COLOR dat_8c18d8bc[11] = {
#include "animal/models/dat_8c18d8bc.inc"
};
static NJS_MESHSET dat_8c18d8e8[1] = {
#include "animal/models/dat_8c18d8e8.inc"
};
static NJS_POINT3 dat_8c18d900[7] = {
#include "animal/models/dat_8c18d900.inc"
};
static NJS_VECTOR dat_8c18d954[7] = {
#include "animal/models/dat_8c18d954.inc"
};
static NJS_MODEL dat_8c18d9a8 = {
#include "animal/models/dat_8c18d9a8.inc"
};
static NJS_OBJECT dat_8c18d9d0 = {
#include "animal/models/dat_8c18d9d0.inc"
};
static NJS_OBJECT dat_8c18da04 = {
#include "animal/models/dat_8c18da04.inc"
};
static NJS_OBJECT dat_8c18da38 = {
#include "animal/models/dat_8c18da38.inc"
};
static NJS_OBJECT dat_8c18da6c = {
#include "animal/models/dat_8c18da6c.inc"
};
static NJS_MATERIAL dat_8c18daa0[1] = {
#include "animal/models/dat_8c18daa0.inc"
};
static Sint16 dat_8c18dab4[10] = {
#include "animal/models/dat_8c18dab4.inc"
};
static NJS_COLOR dat_8c18dac8[8] = {
#include "animal/models/dat_8c18dac8.inc"
};
static NJS_MESHSET dat_8c18dae8[1] = {
#include "animal/models/dat_8c18dae8.inc"
};
static NJS_POINT3 dat_8c18db00[4] = {
#include "animal/models/dat_8c18db00.inc"
};
static NJS_VECTOR dat_8c18db30[4] = {
#include "animal/models/dat_8c18db30.inc"
};
static NJS_MODEL dat_8c18db60 = {
#include "animal/models/dat_8c18db60.inc"
};
static NJS_OBJECT dat_8c18db88 = {
#include "animal/models/dat_8c18db88.inc"
};
static NJS_OBJECT dat_8c18dbbc = {
#include "animal/models/dat_8c18dbbc.inc"
};
static NJS_OBJECT dat_8c18dbf0 = {
#include "animal/models/dat_8c18dbf0.inc"
};
static NJS_OBJECT dat_8c18dc24 = {
#include "animal/models/dat_8c18dc24.inc"
};
static NJS_MATERIAL dat_8c18dc58[1] = {
#include "animal/models/dat_8c18dc58.inc"
};
static Sint16 dat_8c18dc6c[10] = {
#include "animal/models/dat_8c18dc6c.inc"
};
static NJS_COLOR dat_8c18dc80[8] = {
#include "animal/models/dat_8c18dc80.inc"
};
static NJS_MESHSET dat_8c18dca0[1] = {
#include "animal/models/dat_8c18dca0.inc"
};
static NJS_POINT3 dat_8c18dcb8[4] = {
#include "animal/models/dat_8c18dcb8.inc"
};
static NJS_VECTOR dat_8c18dce8[4] = {
#include "animal/models/dat_8c18dce8.inc"
};
static NJS_MODEL dat_8c18dd18 = {
#include "animal/models/dat_8c18dd18.inc"
};
static NJS_OBJECT dat_8c18dd40 = {
#include "animal/models/dat_8c18dd40.inc"
};
static NJS_OBJECT dat_8c18dd74 = {
#include "animal/models/dat_8c18dd74.inc"
};
static NJS_OBJECT dat_8c18dda8 = {
#include "animal/models/dat_8c18dda8.inc"
};
static NJS_OBJECT dat_8c18dddc = {
#include "animal/models/dat_8c18dddc.inc"
};
static NJS_MATERIAL dat_8c18de10[1] = {
#include "animal/models/dat_8c18de10.inc"
};
static Sint16 dat_8c18de24[10] = {
#include "animal/models/dat_8c18de24.inc"
};
static NJS_COLOR dat_8c18de38[8] = {
#include "animal/models/dat_8c18de38.inc"
};
static NJS_MESHSET dat_8c18de58[1] = {
#include "animal/models/dat_8c18de58.inc"
};
static NJS_POINT3 dat_8c18de70[5] = {
#include "animal/models/dat_8c18de70.inc"
};
static NJS_VECTOR dat_8c18deac[5] = {
#include "animal/models/dat_8c18deac.inc"
};
static NJS_MODEL dat_8c18dee8 = {
#include "animal/models/dat_8c18dee8.inc"
};
static NJS_OBJECT dat_8c18df10 = {
#include "animal/models/dat_8c18df10.inc"
};
static NJS_OBJECT dat_8c18df44 = {
#include "animal/models/dat_8c18df44.inc"
};
static NJS_OBJECT dat_8c18df78 = {
#include "animal/models/dat_8c18df78.inc"
};
static NJS_OBJECT dat_8c18dfac = {
#include "animal/models/dat_8c18dfac.inc"
};
static NJS_MATERIAL dat_8c18dfe0[1] = {
#include "animal/models/dat_8c18dfe0.inc"
};
static Sint16 dat_8c18dff4[10] = {
#include "animal/models/dat_8c18dff4.inc"
};
static NJS_COLOR dat_8c18e008[8] = {
#include "animal/models/dat_8c18e008.inc"
};
static NJS_MESHSET dat_8c18e028[1] = {
#include "animal/models/dat_8c18e028.inc"
};
static NJS_POINT3 dat_8c18e040[5] = {
#include "animal/models/dat_8c18e040.inc"
};
static NJS_VECTOR dat_8c18e07c[5] = {
#include "animal/models/dat_8c18e07c.inc"
};
static NJS_MODEL dat_8c18e0b8 = {
#include "animal/models/dat_8c18e0b8.inc"
};
static NJS_OBJECT dat_8c18e0e0 = {
#include "animal/models/dat_8c18e0e0.inc"
};
static NJS_OBJECT dat_8c18e114 = {
#include "animal/models/dat_8c18e114.inc"
};
static NJS_OBJECT dat_8c18e148 = {
#include "animal/models/dat_8c18e148.inc"
};
static NJS_OBJECT dat_8c18e17c = {
#include "animal/models/dat_8c18e17c.inc"
};
static NJS_MATERIAL dat_8c18e1b0[1] = {
#include "animal/models/dat_8c18e1b0.inc"
};
static Sint16 dat_8c18e1c4[10] = {
#include "animal/models/dat_8c18e1c4.inc"
};
static NJS_COLOR dat_8c18e1d8[8] = {
#include "animal/models/dat_8c18e1d8.inc"
};
static NJS_MESHSET dat_8c18e1f8[1] = {
#include "animal/models/dat_8c18e1f8.inc"
};
static NJS_POINT3 dat_8c18e210[5] = {
#include "animal/models/dat_8c18e210.inc"
};
static NJS_VECTOR dat_8c18e24c[5] = {
#include "animal/models/dat_8c18e24c.inc"
};
static NJS_MODEL dat_8c18e288 = {
#include "animal/models/dat_8c18e288.inc"
};
static NJS_OBJECT dat_8c18e2b0 = {
#include "animal/models/dat_8c18e2b0.inc"
};
static NJS_OBJECT dat_8c18e2e4 = {
#include "animal/models/dat_8c18e2e4.inc"
};
static NJS_OBJECT dat_8c18e318 = {
#include "animal/models/dat_8c18e318.inc"
};
static NJS_OBJECT dat_8c18e34c = {
#include "animal/models/dat_8c18e34c.inc"
};
static NJS_MATERIAL dat_8c18e380[1] = {
#include "animal/models/dat_8c18e380.inc"
};
static Sint16 dat_8c18e394[79] = {
#include "animal/models/dat_8c18e394.inc"
};
static NJS_COLOR dat_8c18e434[66] = {
#include "animal/models/dat_8c18e434.inc"
};
static NJS_MESHSET dat_8c18e53c[1] = {
#include "animal/models/dat_8c18e53c.inc"
};
static NJS_POINT3 dat_8c18e554[22] = {
#include "animal/models/dat_8c18e554.inc"
};
static NJS_VECTOR dat_8c18e65c[22] = {
#include "animal/models/dat_8c18e65c.inc"
};
static NJS_MODEL dat_8c18e764 = {
#include "animal/models/dat_8c18e764.inc"
};
static NJS_OBJECT dat_8c18e78c = {
#include "animal/models/dat_8c18e78c.inc"
};
static NJS_OBJECT dat_8c18e7c0 = {
#include "animal/models/dat_8c18e7c0.inc"
};
static NJS_OBJECT dat_8c18e7f4 = {
#include "animal/models/dat_8c18e7f4.inc"
};
static NJS_OBJECT dat_8c18e828 = {
#include "animal/models/dat_8c18e828.inc"
};
static NJS_MATERIAL dat_8c18e85c[1] = {
#include "animal/models/dat_8c18e85c.inc"
};
static Sint16 dat_8c18e870[42] = {
#include "animal/models/dat_8c18e870.inc"
};
static NJS_COLOR dat_8c18e8c4[37] = {
#include "animal/models/dat_8c18e8c4.inc"
};
static NJS_MESHSET dat_8c18e958[1] = {
#include "animal/models/dat_8c18e958.inc"
};
static NJS_POINT3 dat_8c18e970[16] = {
#include "animal/models/dat_8c18e970.inc"
};
static NJS_VECTOR dat_8c18ea30[16] = {
#include "animal/models/dat_8c18ea30.inc"
};
static NJS_MODEL dat_8c18eaf0 = {
#include "animal/models/dat_8c18eaf0.inc"
};
static NJS_OBJECT dat_8c18eb18 = {
#include "animal/models/dat_8c18eb18.inc"
};
static NJS_OBJECT dat_8c18eb4c = {
#include "animal/models/dat_8c18eb4c.inc"
};
static NJS_OBJECT dat_8c18eb80 = {
#include "animal/models/dat_8c18eb80.inc"
};
static NJS_OBJECT dat_8c18ebb4 = {
#include "animal/models/dat_8c18ebb4.inc"
};
static NJS_OBJECT usa_object = {
#include "animal/models/usa_object.inc"
};
static NJS_MKEY_F dat_8c18ec1c[2] = {
#include "animal/models/dat_8c18ec1c.inc"
};
static NJS_MKEY_A dat_8c18ec3c[2] = {
#include "animal/models/dat_8c18ec3c.inc"
};
static NJS_MKEY_F dat_8c18ec5c[17] = {
#include "animal/models/dat_8c18ec5c.inc"
};
static NJS_MKEY_A dat_8c18ed6c[2] = {
#include "animal/models/dat_8c18ed6c.inc"
};
static NJS_MKEY_F dat_8c18ed8c[2] = {
#include "animal/models/dat_8c18ed8c.inc"
};
static NJS_MKEY_A dat_8c18edac[17] = {
#include "animal/models/dat_8c18edac.inc"
};
static NJS_MKEY_F dat_8c18eebc[2] = {
#include "animal/models/dat_8c18eebc.inc"
};
static NJS_MKEY_A dat_8c18eedc[2] = {
#include "animal/models/dat_8c18eedc.inc"
};
static NJS_MKEY_F dat_8c18eefc[2] = {
#include "animal/models/dat_8c18eefc.inc"
};
static NJS_MKEY_A dat_8c18ef1c[2] = {
#include "animal/models/dat_8c18ef1c.inc"
};
static NJS_MKEY_F dat_8c18ef3c[17] = {
#include "animal/models/dat_8c18ef3c.inc"
};
static NJS_MKEY_A dat_8c18f04c[2] = {
#include "animal/models/dat_8c18f04c.inc"
};
static NJS_MKEY_F dat_8c18f06c[2] = {
#include "animal/models/dat_8c18f06c.inc"
};
static NJS_MKEY_A dat_8c18f08c[17] = {
#include "animal/models/dat_8c18f08c.inc"
};
static NJS_MKEY_F dat_8c18f19c[2] = {
#include "animal/models/dat_8c18f19c.inc"
};
static NJS_MKEY_A dat_8c18f1bc[2] = {
#include "animal/models/dat_8c18f1bc.inc"
};
static NJS_MKEY_F dat_8c18f1dc[2] = {
#include "animal/models/dat_8c18f1dc.inc"
};
static NJS_MKEY_A dat_8c18f1fc[2] = {
#include "animal/models/dat_8c18f1fc.inc"
};
static NJS_MKEY_F dat_8c18f21c[2] = {
#include "animal/models/dat_8c18f21c.inc"
};
static NJS_MKEY_A dat_8c18f23c[2] = {
#include "animal/models/dat_8c18f23c.inc"
};
static NJS_MKEY_F dat_8c18f25c[2] = {
#include "animal/models/dat_8c18f25c.inc"
};
static NJS_MKEY_A dat_8c18f27c[17] = {
#include "animal/models/dat_8c18f27c.inc"
};
static NJS_MKEY_F dat_8c18f38c[2] = {
#include "animal/models/dat_8c18f38c.inc"
};
static NJS_MKEY_A dat_8c18f3ac[2] = {
#include "animal/models/dat_8c18f3ac.inc"
};
static NJS_MKEY_F dat_8c18f3cc[2] = {
#include "animal/models/dat_8c18f3cc.inc"
};
static NJS_MKEY_A dat_8c18f3ec[2] = {
#include "animal/models/dat_8c18f3ec.inc"
};
static NJS_MKEY_F dat_8c18f40c[2] = {
#include "animal/models/dat_8c18f40c.inc"
};
static NJS_MKEY_A dat_8c18f42c[2] = {
#include "animal/models/dat_8c18f42c.inc"
};
static NJS_MKEY_F dat_8c18f44c[2] = {
#include "animal/models/dat_8c18f44c.inc"
};
static NJS_MKEY_A dat_8c18f46c[17] = {
#include "animal/models/dat_8c18f46c.inc"
};
static NJS_MKEY_F dat_8c18f57c[2] = {
#include "animal/models/dat_8c18f57c.inc"
};
static NJS_MKEY_A dat_8c18f59c[2] = {
#include "animal/models/dat_8c18f59c.inc"
};
static NJS_MKEY_F dat_8c18f5bc[2] = {
#include "animal/models/dat_8c18f5bc.inc"
};
static NJS_MKEY_A dat_8c18f5dc[2] = {
#include "animal/models/dat_8c18f5dc.inc"
};
static NJS_MKEY_F dat_8c18f5fc[2] = {
#include "animal/models/dat_8c18f5fc.inc"
};
static NJS_MKEY_A dat_8c18f61c[2] = {
#include "animal/models/dat_8c18f61c.inc"
};
static NJS_MKEY_F dat_8c18f63c[2] = {
#include "animal/models/dat_8c18f63c.inc"
};
static NJS_MKEY_A dat_8c18f65c[17] = {
#include "animal/models/dat_8c18f65c.inc"
};
static NJS_MKEY_F dat_8c18f76c[2] = {
#include "animal/models/dat_8c18f76c.inc"
};
static NJS_MKEY_A dat_8c18f78c[2] = {
#include "animal/models/dat_8c18f78c.inc"
};
static NJS_MKEY_F dat_8c18f7ac[2] = {
#include "animal/models/dat_8c18f7ac.inc"
};
static NJS_MKEY_A dat_8c18f7cc[2] = {
#include "animal/models/dat_8c18f7cc.inc"
};
static NJS_MKEY_F dat_8c18f7ec[17] = {
#include "animal/models/dat_8c18f7ec.inc"
};
static NJS_MKEY_A dat_8c18f8fc[2] = {
#include "animal/models/dat_8c18f8fc.inc"
};
static NJS_MKEY_F dat_8c18f91c[2] = {
#include "animal/models/dat_8c18f91c.inc"
};
static NJS_MKEY_A dat_8c18f93c[17] = {
#include "animal/models/dat_8c18f93c.inc"
};
static NJS_MKEY_F dat_8c18fa4c[2] = {
#include "animal/models/dat_8c18fa4c.inc"
};
static NJS_MKEY_A dat_8c18fa6c[2] = {
#include "animal/models/dat_8c18fa6c.inc"
};
static NJS_MKEY_F dat_8c18fa8c[2] = {
#include "animal/models/dat_8c18fa8c.inc"
};
static NJS_MKEY_A dat_8c18faac[2] = {
#include "animal/models/dat_8c18faac.inc"
};
static NJS_MKEY_F dat_8c18facc[17] = {
#include "animal/models/dat_8c18facc.inc"
};
static NJS_MKEY_A dat_8c18fbdc[2] = {
#include "animal/models/dat_8c18fbdc.inc"
};
static NJS_MKEY_F dat_8c18fbfc[2] = {
#include "animal/models/dat_8c18fbfc.inc"
};
static NJS_MKEY_A dat_8c18fc1c[17] = {
#include "animal/models/dat_8c18fc1c.inc"
};
static NJS_MKEY_F dat_8c18fd2c[2] = {
#include "animal/models/dat_8c18fd2c.inc"
};
static NJS_MKEY_A dat_8c18fd4c[2] = {
#include "animal/models/dat_8c18fd4c.inc"
};
static NJS_MKEY_F dat_8c18fd6c[2] = {
#include "animal/models/dat_8c18fd6c.inc"
};
static NJS_MKEY_A dat_8c18fd8c[2] = {
#include "animal/models/dat_8c18fd8c.inc"
};
static NJS_MKEY_F dat_8c18fdac[2] = {
#include "animal/models/dat_8c18fdac.inc"
};
static NJS_MKEY_A dat_8c18fdcc[2] = {
#include "animal/models/dat_8c18fdcc.inc"
};
static NJS_MKEY_F dat_8c18fdec[2] = {
#include "animal/models/dat_8c18fdec.inc"
};
static NJS_MKEY_A dat_8c18fe0c[17] = {
#include "animal/models/dat_8c18fe0c.inc"
};
static NJS_MKEY_F dat_8c18ff1c[2] = {
#include "animal/models/dat_8c18ff1c.inc"
};
static NJS_MKEY_A dat_8c18ff3c[2] = {
#include "animal/models/dat_8c18ff3c.inc"
};
static NJS_MKEY_F dat_8c18ff5c[2] = {
#include "animal/models/dat_8c18ff5c.inc"
};
static NJS_MKEY_A dat_8c18ff7c[2] = {
#include "animal/models/dat_8c18ff7c.inc"
};
static NJS_MKEY_F dat_8c18ff9c[2] = {
#include "animal/models/dat_8c18ff9c.inc"
};
static NJS_MKEY_A dat_8c18ffbc[2] = {
#include "animal/models/dat_8c18ffbc.inc"
};
static NJS_MKEY_F dat_8c18ffdc[2] = {
#include "animal/models/dat_8c18ffdc.inc"
};
static NJS_MKEY_A dat_8c18fffc[17] = {
#include "animal/models/dat_8c18fffc.inc"
};
static NJS_MKEY_F dat_8c19010c[2] = {
#include "animal/models/dat_8c19010c.inc"
};
static NJS_MKEY_A dat_8c19012c[2] = {
#include "animal/models/dat_8c19012c.inc"
};
static NJS_MKEY_F dat_8c19014c[2] = {
#include "animal/models/dat_8c19014c.inc"
};
static NJS_MKEY_A dat_8c19016c[2] = {
#include "animal/models/dat_8c19016c.inc"
};
static NJS_MDATA2 dat_8c19018c[37] = {
#include "animal/models/dat_8c19018c.inc"
};
static NJS_MOTION usa_motion = {
#include "animal/models/usa_motion.inc"
};
NJS_ACTION usa_action = {
#include "animal/models/usa_action.inc"
};
static NJS_TEXNAME usa_textures[4] = {
    {TEXT_38, 0, 0},
    {TEXT_39, 0, 0},
    {TEXT_40, 0, 0},
    {TEXT_41, 0, 0},
};
NJS_TEXLIST usa_texlist = {
#include "animal/models/usa_texlist.inc"
};
static NJS_MATERIAL dat_8c190428[1] = {
#include "animal/models/dat_8c190428.inc"
};
static Sint16 dat_8c19043c[14] = {
#include "animal/models/dat_8c19043c.inc"
};
static NJS_COLOR dat_8c190458[11] = {
#include "animal/models/dat_8c190458.inc"
};
static NJS_MESHSET dat_8c190484[1] = {
#include "animal/models/dat_8c190484.inc"
};
static NJS_POINT3 dat_8c19049c[7] = {
#include "animal/models/dat_8c19049c.inc"
};
static NJS_VECTOR dat_8c1904f0[7] = {
#include "animal/models/dat_8c1904f0.inc"
};
static NJS_MODEL dat_8c190544 = {
#include "animal/models/dat_8c190544.inc"
};
static NJS_OBJECT dat_8c19056c = {
#include "animal/models/dat_8c19056c.inc"
};
static NJS_OBJECT dat_8c1905a0 = {
#include "animal/models/dat_8c1905a0.inc"
};
static NJS_OBJECT dat_8c1905d4 = {
#include "animal/models/dat_8c1905d4.inc"
};
static NJS_OBJECT dat_8c190608 = {
#include "animal/models/dat_8c190608.inc"
};
static NJS_MATERIAL dat_8c19063c[1] = {
#include "animal/models/dat_8c19063c.inc"
};
static Sint16 dat_8c190650[17] = {
#include "animal/models/dat_8c190650.inc"
};
static NJS_COLOR dat_8c190674[13] = {
#include "animal/models/dat_8c190674.inc"
};
static NJS_MESHSET dat_8c1906a8[1] = {
#include "animal/models/dat_8c1906a8.inc"
};
static NJS_POINT3 dat_8c1906c0[7] = {
#include "animal/models/dat_8c1906c0.inc"
};
static NJS_VECTOR dat_8c190714[7] = {
#include "animal/models/dat_8c190714.inc"
};
static NJS_MODEL dat_8c190768 = {
#include "animal/models/dat_8c190768.inc"
};
static NJS_OBJECT dat_8c190790 = {
#include "animal/models/dat_8c190790.inc"
};
static NJS_OBJECT dat_8c1907c4 = {
#include "animal/models/dat_8c1907c4.inc"
};
static NJS_OBJECT dat_8c1907f8 = {
#include "animal/models/dat_8c1907f8.inc"
};
static NJS_OBJECT dat_8c19082c = {
#include "animal/models/dat_8c19082c.inc"
};
static NJS_MATERIAL dat_8c190860[1] = {
#include "animal/models/dat_8c190860.inc"
};
static Sint16 dat_8c190874[19] = {
#include "animal/models/dat_8c190874.inc"
};
static NJS_COLOR dat_8c19089c[16] = {
#include "animal/models/dat_8c19089c.inc"
};
static NJS_MESHSET dat_8c1908dc[1] = {
#include "animal/models/dat_8c1908dc.inc"
};
static NJS_POINT3 dat_8c1908f4[7] = {
#include "animal/models/dat_8c1908f4.inc"
};
static NJS_VECTOR dat_8c190948[7] = {
#include "animal/models/dat_8c190948.inc"
};
static NJS_MODEL dat_8c19099c = {
#include "animal/models/dat_8c19099c.inc"
};
static NJS_OBJECT dat_8c1909c4 = {
#include "animal/models/dat_8c1909c4.inc"
};
static NJS_OBJECT dat_8c1909f8 = {
#include "animal/models/dat_8c1909f8.inc"
};
static NJS_OBJECT dat_8c190a2c = {
#include "animal/models/dat_8c190a2c.inc"
};
static NJS_OBJECT dat_8c190a60 = {
#include "animal/models/dat_8c190a60.inc"
};
static NJS_MATERIAL dat_8c190a94[1] = {
#include "animal/models/dat_8c190a94.inc"
};
static Sint16 dat_8c190aa8[19] = {
#include "animal/models/dat_8c190aa8.inc"
};
static NJS_COLOR dat_8c190ad0[16] = {
#include "animal/models/dat_8c190ad0.inc"
};
static NJS_MESHSET dat_8c190b10[1] = {
#include "animal/models/dat_8c190b10.inc"
};
static NJS_POINT3 dat_8c190b28[7] = {
#include "animal/models/dat_8c190b28.inc"
};
static NJS_VECTOR dat_8c190b7c[7] = {
#include "animal/models/dat_8c190b7c.inc"
};
static NJS_MODEL dat_8c190bd0 = {
#include "animal/models/dat_8c190bd0.inc"
};
static NJS_OBJECT dat_8c190bf8 = {
#include "animal/models/dat_8c190bf8.inc"
};
static NJS_OBJECT dat_8c190c2c = {
#include "animal/models/dat_8c190c2c.inc"
};
static NJS_OBJECT dat_8c190c60 = {
#include "animal/models/dat_8c190c60.inc"
};
static NJS_OBJECT dat_8c190c94 = {
#include "animal/models/dat_8c190c94.inc"
};
static NJS_MATERIAL dat_8c190cc8[1] = {
#include "animal/models/dat_8c190cc8.inc"
};
static Sint16 dat_8c190cdc[20] = {
#include "animal/models/dat_8c190cdc.inc"
};
static NJS_COLOR dat_8c190d04[15] = {
#include "animal/models/dat_8c190d04.inc"
};
static NJS_MESHSET dat_8c190d40[1] = {
#include "animal/models/dat_8c190d40.inc"
};
static NJS_POINT3 dat_8c190d58[5] = {
#include "animal/models/dat_8c190d58.inc"
};
static NJS_VECTOR dat_8c190d94[5] = {
#include "animal/models/dat_8c190d94.inc"
};
static NJS_MODEL dat_8c190dd0 = {
#include "animal/models/dat_8c190dd0.inc"
};
static NJS_OBJECT dat_8c190df8 = {
#include "animal/models/dat_8c190df8.inc"
};
static NJS_OBJECT dat_8c190e2c = {
#include "animal/models/dat_8c190e2c.inc"
};
static NJS_OBJECT dat_8c190e60 = {
#include "animal/models/dat_8c190e60.inc"
};
static NJS_OBJECT dat_8c190e94 = {
#include "animal/models/dat_8c190e94.inc"
};
static NJS_MATERIAL dat_8c190ec8[1] = {
#include "animal/models/dat_8c190ec8.inc"
};
static Sint16 dat_8c190edc[21] = {
#include "animal/models/dat_8c190edc.inc"
};
static NJS_COLOR dat_8c190f08[18] = {
#include "animal/models/dat_8c190f08.inc"
};
static NJS_MESHSET dat_8c190f50[1] = {
#include "animal/models/dat_8c190f50.inc"
};
static NJS_POINT3 dat_8c190f68[9] = {
#include "animal/models/dat_8c190f68.inc"
};
static NJS_VECTOR dat_8c190fd4[9] = {
#include "animal/models/dat_8c190fd4.inc"
};
static NJS_MODEL dat_8c191040 = {
#include "animal/models/dat_8c191040.inc"
};
static NJS_OBJECT dat_8c191068 = {
#include "animal/models/dat_8c191068.inc"
};
static NJS_OBJECT dat_8c19109c = {
#include "animal/models/dat_8c19109c.inc"
};
static NJS_OBJECT dat_8c1910d0 = {
#include "animal/models/dat_8c1910d0.inc"
};
static NJS_OBJECT dat_8c191104 = {
#include "animal/models/dat_8c191104.inc"
};
static NJS_MATERIAL dat_8c191138[1] = {
#include "animal/models/dat_8c191138.inc"
};
static Sint16 dat_8c19114c[94] = {
#include "animal/models/dat_8c19114c.inc"
};
static NJS_COLOR dat_8c191208[80] = {
#include "animal/models/dat_8c191208.inc"
};
static NJS_MESHSET dat_8c191348[1] = {
#include "animal/models/dat_8c191348.inc"
};
static NJS_POINT3 dat_8c191360[28] = {
#include "animal/models/dat_8c191360.inc"
};
static NJS_VECTOR dat_8c1914b0[28] = {
#include "animal/models/dat_8c1914b0.inc"
};
static NJS_MODEL dat_8c191600 = {
#include "animal/models/dat_8c191600.inc"
};
static NJS_OBJECT dat_8c191628 = {
#include "animal/models/dat_8c191628.inc"
};
static NJS_OBJECT dat_8c19165c = {
#include "animal/models/dat_8c19165c.inc"
};
static NJS_OBJECT dat_8c191690 = {
#include "animal/models/dat_8c191690.inc"
};
static NJS_OBJECT dat_8c1916c4 = {
#include "animal/models/dat_8c1916c4.inc"
};
static NJS_OBJECT mogu_object = {
#include "animal/models/mogu_object.inc"
};
static NJS_MKEY_F dat_8c19172c[2] = {
#include "animal/models/dat_8c19172c.inc"
};
static NJS_MKEY_A dat_8c19174c[2] = {
#include "animal/models/dat_8c19174c.inc"
};
static NJS_MKEY_F dat_8c19176c[17] = {
#include "animal/models/dat_8c19176c.inc"
};
static NJS_MKEY_A dat_8c19187c[2] = {
#include "animal/models/dat_8c19187c.inc"
};
static NJS_MKEY_F dat_8c19189c[2] = {
#include "animal/models/dat_8c19189c.inc"
};
static NJS_MKEY_A dat_8c1918bc[17] = {
#include "animal/models/dat_8c1918bc.inc"
};
static NJS_MKEY_F dat_8c1919cc[2] = {
#include "animal/models/dat_8c1919cc.inc"
};
static NJS_MKEY_A dat_8c1919ec[2] = {
#include "animal/models/dat_8c1919ec.inc"
};
static NJS_MKEY_F dat_8c191a0c[2] = {
#include "animal/models/dat_8c191a0c.inc"
};
static NJS_MKEY_A dat_8c191a2c[2] = {
#include "animal/models/dat_8c191a2c.inc"
};
static NJS_MKEY_F dat_8c191a4c[2] = {
#include "animal/models/dat_8c191a4c.inc"
};
static NJS_MKEY_A dat_8c191a6c[2] = {
#include "animal/models/dat_8c191a6c.inc"
};
static NJS_MKEY_F dat_8c191a8c[2] = {
#include "animal/models/dat_8c191a8c.inc"
};
static NJS_MKEY_A dat_8c191aac[17] = {
#include "animal/models/dat_8c191aac.inc"
};
static NJS_MKEY_F dat_8c191bbc[2] = {
#include "animal/models/dat_8c191bbc.inc"
};
static NJS_MKEY_A dat_8c191bdc[2] = {
#include "animal/models/dat_8c191bdc.inc"
};
static NJS_MKEY_F dat_8c191bfc[2] = {
#include "animal/models/dat_8c191bfc.inc"
};
static NJS_MKEY_A dat_8c191c1c[2] = {
#include "animal/models/dat_8c191c1c.inc"
};
static NJS_MKEY_F dat_8c191c3c[2] = {
#include "animal/models/dat_8c191c3c.inc"
};
static NJS_MKEY_A dat_8c191c5c[2] = {
#include "animal/models/dat_8c191c5c.inc"
};
static NJS_MKEY_F dat_8c191c7c[2] = {
#include "animal/models/dat_8c191c7c.inc"
};
static NJS_MKEY_A dat_8c191c9c[17] = {
#include "animal/models/dat_8c191c9c.inc"
};
static NJS_MKEY_F dat_8c191dac[2] = {
#include "animal/models/dat_8c191dac.inc"
};
static NJS_MKEY_A dat_8c191dcc[2] = {
#include "animal/models/dat_8c191dcc.inc"
};
static NJS_MKEY_F dat_8c191dec[2] = {
#include "animal/models/dat_8c191dec.inc"
};
static NJS_MKEY_A dat_8c191e0c[2] = {
#include "animal/models/dat_8c191e0c.inc"
};
static NJS_MKEY_F dat_8c191e2c[2] = {
#include "animal/models/dat_8c191e2c.inc"
};
static NJS_MKEY_A dat_8c191e4c[2] = {
#include "animal/models/dat_8c191e4c.inc"
};
static NJS_MKEY_F dat_8c191e6c[2] = {
#include "animal/models/dat_8c191e6c.inc"
};
static NJS_MKEY_A dat_8c191e8c[17] = {
#include "animal/models/dat_8c191e8c.inc"
};
static NJS_MKEY_F dat_8c191f9c[2] = {
#include "animal/models/dat_8c191f9c.inc"
};
static NJS_MKEY_A dat_8c191fbc[2] = {
#include "animal/models/dat_8c191fbc.inc"
};
static NJS_MKEY_F dat_8c191fdc[2] = {
#include "animal/models/dat_8c191fdc.inc"
};
static NJS_MKEY_A dat_8c191ffc[2] = {
#include "animal/models/dat_8c191ffc.inc"
};
static NJS_MKEY_F dat_8c19201c[2] = {
#include "animal/models/dat_8c19201c.inc"
};
static NJS_MKEY_A dat_8c19203c[2] = {
#include "animal/models/dat_8c19203c.inc"
};
static NJS_MKEY_F dat_8c19205c[2] = {
#include "animal/models/dat_8c19205c.inc"
};
static NJS_MKEY_A dat_8c19207c[17] = {
#include "animal/models/dat_8c19207c.inc"
};
static NJS_MKEY_F dat_8c19218c[2] = {
#include "animal/models/dat_8c19218c.inc"
};
static NJS_MKEY_A dat_8c1921ac[2] = {
#include "animal/models/dat_8c1921ac.inc"
};
static NJS_MKEY_F dat_8c1921cc[2] = {
#include "animal/models/dat_8c1921cc.inc"
};
static NJS_MKEY_A dat_8c1921ec[2] = {
#include "animal/models/dat_8c1921ec.inc"
};
static NJS_MKEY_F dat_8c19220c[2] = {
#include "animal/models/dat_8c19220c.inc"
};
static NJS_MKEY_A dat_8c19222c[2] = {
#include "animal/models/dat_8c19222c.inc"
};
static NJS_MKEY_F dat_8c19224c[2] = {
#include "animal/models/dat_8c19224c.inc"
};
static NJS_MKEY_A dat_8c19226c[17] = {
#include "animal/models/dat_8c19226c.inc"
};
static NJS_MKEY_F dat_8c19237c[2] = {
#include "animal/models/dat_8c19237c.inc"
};
static NJS_MKEY_A dat_8c19239c[2] = {
#include "animal/models/dat_8c19239c.inc"
};
static NJS_MKEY_F dat_8c1923bc[2] = {
#include "animal/models/dat_8c1923bc.inc"
};
static NJS_MKEY_A dat_8c1923dc[2] = {
#include "animal/models/dat_8c1923dc.inc"
};
static NJS_MKEY_F dat_8c1923fc[2] = {
#include "animal/models/dat_8c1923fc.inc"
};
static NJS_MKEY_A dat_8c19241c[2] = {
#include "animal/models/dat_8c19241c.inc"
};
static NJS_MKEY_F dat_8c19243c[2] = {
#include "animal/models/dat_8c19243c.inc"
};
static NJS_MKEY_A dat_8c19245c[17] = {
#include "animal/models/dat_8c19245c.inc"
};
static NJS_MKEY_F dat_8c19256c[2] = {
#include "animal/models/dat_8c19256c.inc"
};
static NJS_MKEY_A dat_8c19258c[2] = {
#include "animal/models/dat_8c19258c.inc"
};
static NJS_MKEY_F dat_8c1925ac[2] = {
#include "animal/models/dat_8c1925ac.inc"
};
static NJS_MKEY_A dat_8c1925cc[2] = {
#include "animal/models/dat_8c1925cc.inc"
};
static NJS_MDATA2 dat_8c1925ec[29] = {
#include "animal/models/dat_8c1925ec.inc"
};
static NJS_MOTION mogu_motion = {
#include "animal/models/mogu_motion.inc"
};
NJS_ACTION mogu_action = {
#include "animal/models/mogu_action.inc"
};
static NJS_TEXNAME mogu_textures[4] = {
    {TEXT_42, 0, 0},
    {TEXT_43, 0, 0},
    {TEXT_44, 0, 0},
    {TEXT_45, 0, 0},
};
NJS_TEXLIST mogu_texlist = {
#include "animal/models/mogu_texlist.inc"
};
static NJS_MATERIAL dat_8c192808[1] = {
#include "animal/models/dat_8c192808.inc"
};
static Sint16 dat_8c19281c[18] = {
#include "animal/models/dat_8c19281c.inc"
};
static NJS_COLOR dat_8c192840[15] = {
#include "animal/models/dat_8c192840.inc"
};
static NJS_MESHSET dat_8c19287c[1] = {
#include "animal/models/dat_8c19287c.inc"
};
static NJS_POINT3 dat_8c192894[7] = {
#include "animal/models/dat_8c192894.inc"
};
static NJS_VECTOR dat_8c1928e8[7] = {
#include "animal/models/dat_8c1928e8.inc"
};
static NJS_MODEL dat_8c19293c = {
#include "animal/models/dat_8c19293c.inc"
};
static NJS_OBJECT dat_8c192964 = {
#include "animal/models/dat_8c192964.inc"
};
static NJS_OBJECT dat_8c192998 = {
#include "animal/models/dat_8c192998.inc"
};
static NJS_OBJECT dat_8c1929cc = {
#include "animal/models/dat_8c1929cc.inc"
};
static NJS_OBJECT dat_8c192a00 = {
#include "animal/models/dat_8c192a00.inc"
};
static NJS_MATERIAL dat_8c192a34[1] = {
#include "animal/models/dat_8c192a34.inc"
};
static Sint16 dat_8c192a48[15] = {
#include "animal/models/dat_8c192a48.inc"
};
static NJS_COLOR dat_8c192a68[12] = {
#include "animal/models/dat_8c192a68.inc"
};
static NJS_MESHSET dat_8c192a98[1] = {
#include "animal/models/dat_8c192a98.inc"
};
static NJS_POINT3 dat_8c192ab0[5] = {
#include "animal/models/dat_8c192ab0.inc"
};
static NJS_VECTOR dat_8c192aec[5] = {
#include "animal/models/dat_8c192aec.inc"
};
static NJS_MODEL dat_8c192b28 = {
#include "animal/models/dat_8c192b28.inc"
};
static NJS_OBJECT dat_8c192b50 = {
#include "animal/models/dat_8c192b50.inc"
};
static NJS_OBJECT dat_8c192b84 = {
#include "animal/models/dat_8c192b84.inc"
};
static NJS_OBJECT dat_8c192bb8 = {
#include "animal/models/dat_8c192bb8.inc"
};
static NJS_OBJECT dat_8c192bec = {
#include "animal/models/dat_8c192bec.inc"
};
static NJS_MATERIAL dat_8c192c20[1] = {
#include "animal/models/dat_8c192c20.inc"
};
static Sint16 dat_8c192c34[15] = {
#include "animal/models/dat_8c192c34.inc"
};
static NJS_COLOR dat_8c192c54[12] = {
#include "animal/models/dat_8c192c54.inc"
};
static NJS_MESHSET dat_8c192c84[1] = {
#include "animal/models/dat_8c192c84.inc"
};
static NJS_POINT3 dat_8c192c9c[5] = {
#include "animal/models/dat_8c192c9c.inc"
};
static NJS_VECTOR dat_8c192cd8[5] = {
#include "animal/models/dat_8c192cd8.inc"
};
static NJS_MODEL dat_8c192d14 = {
#include "animal/models/dat_8c192d14.inc"
};
static NJS_OBJECT dat_8c192d3c = {
#include "animal/models/dat_8c192d3c.inc"
};
static NJS_OBJECT dat_8c192d70 = {
#include "animal/models/dat_8c192d70.inc"
};
static NJS_OBJECT dat_8c192da4 = {
#include "animal/models/dat_8c192da4.inc"
};
static NJS_OBJECT dat_8c192dd8 = {
#include "animal/models/dat_8c192dd8.inc"
};
static NJS_MATERIAL dat_8c192e0c[1] = {
#include "animal/models/dat_8c192e0c.inc"
};
static Sint16 dat_8c192e20[15] = {
#include "animal/models/dat_8c192e20.inc"
};
static NJS_COLOR dat_8c192e40[12] = {
#include "animal/models/dat_8c192e40.inc"
};
static NJS_MESHSET dat_8c192e70[1] = {
#include "animal/models/dat_8c192e70.inc"
};
static NJS_POINT3 dat_8c192e88[5] = {
#include "animal/models/dat_8c192e88.inc"
};
static NJS_VECTOR dat_8c192ec4[5] = {
#include "animal/models/dat_8c192ec4.inc"
};
static NJS_MODEL dat_8c192f00 = {
#include "animal/models/dat_8c192f00.inc"
};
static NJS_OBJECT dat_8c192f28 = {
#include "animal/models/dat_8c192f28.inc"
};
static NJS_OBJECT dat_8c192f5c = {
#include "animal/models/dat_8c192f5c.inc"
};
static NJS_OBJECT dat_8c192f90 = {
#include "animal/models/dat_8c192f90.inc"
};
static NJS_OBJECT dat_8c192fc4 = {
#include "animal/models/dat_8c192fc4.inc"
};
static NJS_MATERIAL dat_8c192ff8[1] = {
#include "animal/models/dat_8c192ff8.inc"
};
static Sint16 dat_8c19300c[15] = {
#include "animal/models/dat_8c19300c.inc"
};
static NJS_COLOR dat_8c19302c[12] = {
#include "animal/models/dat_8c19302c.inc"
};
static NJS_MESHSET dat_8c19305c[1] = {
#include "animal/models/dat_8c19305c.inc"
};
static NJS_POINT3 dat_8c193074[5] = {
#include "animal/models/dat_8c193074.inc"
};
static NJS_VECTOR dat_8c1930b0[5] = {
#include "animal/models/dat_8c1930b0.inc"
};
static NJS_MODEL dat_8c1930ec = {
#include "animal/models/dat_8c1930ec.inc"
};
static NJS_OBJECT dat_8c193114 = {
#include "animal/models/dat_8c193114.inc"
};
static NJS_OBJECT dat_8c193148 = {
#include "animal/models/dat_8c193148.inc"
};
static NJS_OBJECT dat_8c19317c = {
#include "animal/models/dat_8c19317c.inc"
};
static NJS_OBJECT dat_8c1931b0 = {
#include "animal/models/dat_8c1931b0.inc"
};
static NJS_MATERIAL dat_8c1931e4[1] = {
#include "animal/models/dat_8c1931e4.inc"
};
static Sint16 dat_8c1931f8[5] = {
#include "animal/models/dat_8c1931f8.inc"
};
static NJS_COLOR dat_8c193204[4] = {
#include "animal/models/dat_8c193204.inc"
};
static NJS_MESHSET dat_8c193214[1] = {
#include "animal/models/dat_8c193214.inc"
};
static NJS_POINT3 dat_8c19322c[4] = {
#include "animal/models/dat_8c19322c.inc"
};
static NJS_VECTOR dat_8c19325c[4] = {
#include "animal/models/dat_8c19325c.inc"
};
static NJS_MODEL dat_8c19328c = {
#include "animal/models/dat_8c19328c.inc"
};
static NJS_OBJECT dat_8c1932b4 = {
#include "animal/models/dat_8c1932b4.inc"
};
static NJS_OBJECT dat_8c1932e8 = {
#include "animal/models/dat_8c1932e8.inc"
};
static NJS_OBJECT dat_8c19331c = {
#include "animal/models/dat_8c19331c.inc"
};
static NJS_OBJECT dat_8c193350 = {
#include "animal/models/dat_8c193350.inc"
};
static NJS_MATERIAL dat_8c193384[1] = {
#include "animal/models/dat_8c193384.inc"
};
static Sint16 dat_8c193398[5] = {
#include "animal/models/dat_8c193398.inc"
};
static NJS_COLOR dat_8c1933a4[4] = {
#include "animal/models/dat_8c1933a4.inc"
};
static NJS_MESHSET dat_8c1933b4[1] = {
#include "animal/models/dat_8c1933b4.inc"
};
static NJS_POINT3 dat_8c1933cc[4] = {
#include "animal/models/dat_8c1933cc.inc"
};
static NJS_VECTOR dat_8c1933fc[4] = {
#include "animal/models/dat_8c1933fc.inc"
};
static NJS_MODEL dat_8c19342c = {
#include "animal/models/dat_8c19342c.inc"
};
static NJS_OBJECT dat_8c193454 = {
#include "animal/models/dat_8c193454.inc"
};
static NJS_OBJECT dat_8c193488 = {
#include "animal/models/dat_8c193488.inc"
};
static NJS_OBJECT dat_8c1934bc = {
#include "animal/models/dat_8c1934bc.inc"
};
static NJS_OBJECT dat_8c1934f0 = {
#include "animal/models/dat_8c1934f0.inc"
};
static NJS_MATERIAL dat_8c193524[1] = {
#include "animal/models/dat_8c193524.inc"
};
static Sint16 dat_8c193538[29] = {
#include "animal/models/dat_8c193538.inc"
};
static NJS_COLOR dat_8c193574[25] = {
#include "animal/models/dat_8c193574.inc"
};
static NJS_MESHSET dat_8c1935d8[1] = {
#include "animal/models/dat_8c1935d8.inc"
};
static NJS_POINT3 dat_8c1935f0[13] = {
#include "animal/models/dat_8c1935f0.inc"
};
static NJS_VECTOR dat_8c19368c[13] = {
#include "animal/models/dat_8c19368c.inc"
};
static NJS_MODEL dat_8c193728 = {
#include "animal/models/dat_8c193728.inc"
};
static NJS_OBJECT dat_8c193750 = {
#include "animal/models/dat_8c193750.inc"
};
static NJS_OBJECT dat_8c193784 = {
#include "animal/models/dat_8c193784.inc"
};
static NJS_OBJECT dat_8c1937b8 = {
#include "animal/models/dat_8c1937b8.inc"
};
static NJS_OBJECT dat_8c1937ec = {
#include "animal/models/dat_8c1937ec.inc"
};
static NJS_MATERIAL dat_8c193820[1] = {
#include "animal/models/dat_8c193820.inc"
};
static Sint16 dat_8c193834[40] = {
#include "animal/models/dat_8c193834.inc"
};
static NJS_COLOR dat_8c193884[34] = {
#include "animal/models/dat_8c193884.inc"
};
static NJS_MESHSET dat_8c19390c[1] = {
#include "animal/models/dat_8c19390c.inc"
};
static NJS_POINT3 dat_8c193924[13] = {
#include "animal/models/dat_8c193924.inc"
};
static NJS_VECTOR dat_8c1939c0[13] = {
#include "animal/models/dat_8c1939c0.inc"
};
static NJS_MODEL dat_8c193a5c = {
#include "animal/models/dat_8c193a5c.inc"
};
static NJS_OBJECT dat_8c193a84 = {
#include "animal/models/dat_8c193a84.inc"
};
static NJS_OBJECT dat_8c193ab8 = {
#include "animal/models/dat_8c193ab8.inc"
};
static NJS_OBJECT dat_8c193aec = {
#include "animal/models/dat_8c193aec.inc"
};
static NJS_OBJECT dat_8c193b20 = {
#include "animal/models/dat_8c193b20.inc"
};
static NJS_MATERIAL dat_8c193b54[1] = {
#include "animal/models/dat_8c193b54.inc"
};
static Sint16 dat_8c193b68[38] = {
#include "animal/models/dat_8c193b68.inc"
};
static NJS_COLOR dat_8c193bb4[33] = {
#include "animal/models/dat_8c193bb4.inc"
};
static NJS_MESHSET dat_8c193c38[1] = {
#include "animal/models/dat_8c193c38.inc"
};
static NJS_POINT3 dat_8c193c50[15] = {
#include "animal/models/dat_8c193c50.inc"
};
static NJS_VECTOR dat_8c193d04[15] = {
#include "animal/models/dat_8c193d04.inc"
};
static NJS_MODEL dat_8c193db8 = {
#include "animal/models/dat_8c193db8.inc"
};
static NJS_OBJECT dat_8c193de0 = {
#include "animal/models/dat_8c193de0.inc"
};
static NJS_OBJECT dat_8c193e14 = {
#include "animal/models/dat_8c193e14.inc"
};
static NJS_OBJECT dat_8c193e48 = {
#include "animal/models/dat_8c193e48.inc"
};
static NJS_OBJECT dat_8c193e7c = {
#include "animal/models/dat_8c193e7c.inc"
};
static NJS_OBJECT zou_object = {
#include "animal/models/zou_object.inc"
};
static NJS_MKEY_F dat_8c193ee4[2] = {
#include "animal/models/dat_8c193ee4.inc"
};
static NJS_MKEY_A dat_8c193f04[2] = {
#include "animal/models/dat_8c193f04.inc"
};
static NJS_MKEY_F dat_8c193f24[17] = {
#include "animal/models/dat_8c193f24.inc"
};
static NJS_MKEY_A dat_8c194034[2] = {
#include "animal/models/dat_8c194034.inc"
};
static NJS_MKEY_F dat_8c194054[2] = {
#include "animal/models/dat_8c194054.inc"
};
static NJS_MKEY_A dat_8c194074[17] = {
#include "animal/models/dat_8c194074.inc"
};
static NJS_MKEY_F dat_8c194184[2] = {
#include "animal/models/dat_8c194184.inc"
};
static NJS_MKEY_A dat_8c1941a4[2] = {
#include "animal/models/dat_8c1941a4.inc"
};
static NJS_MKEY_F dat_8c1941c4[2] = {
#include "animal/models/dat_8c1941c4.inc"
};
static NJS_MKEY_A dat_8c1941e4[2] = {
#include "animal/models/dat_8c1941e4.inc"
};
static NJS_MKEY_F dat_8c194204[2] = {
#include "animal/models/dat_8c194204.inc"
};
static NJS_MKEY_A dat_8c194224[2] = {
#include "animal/models/dat_8c194224.inc"
};
static NJS_MKEY_F dat_8c194244[2] = {
#include "animal/models/dat_8c194244.inc"
};
static NJS_MKEY_A dat_8c194264[2] = {
#include "animal/models/dat_8c194264.inc"
};
static NJS_MKEY_F dat_8c194284[2] = {
#include "animal/models/dat_8c194284.inc"
};
static NJS_MKEY_A dat_8c1942a4[2] = {
#include "animal/models/dat_8c1942a4.inc"
};
static NJS_MKEY_F dat_8c1942c4[2] = {
#include "animal/models/dat_8c1942c4.inc"
};
static NJS_MKEY_A dat_8c1942e4[2] = {
#include "animal/models/dat_8c1942e4.inc"
};
static NJS_MKEY_F dat_8c194304[17] = {
#include "animal/models/dat_8c194304.inc"
};
static NJS_MKEY_A dat_8c194414[2] = {
#include "animal/models/dat_8c194414.inc"
};
static NJS_MKEY_F dat_8c194434[2] = {
#include "animal/models/dat_8c194434.inc"
};
static NJS_MKEY_A dat_8c194454[17] = {
#include "animal/models/dat_8c194454.inc"
};
static NJS_MKEY_F dat_8c194564[2] = {
#include "animal/models/dat_8c194564.inc"
};
static NJS_MKEY_A dat_8c194584[2] = {
#include "animal/models/dat_8c194584.inc"
};
static NJS_MKEY_F dat_8c1945a4[2] = {
#include "animal/models/dat_8c1945a4.inc"
};
static NJS_MKEY_A dat_8c1945c4[2] = {
#include "animal/models/dat_8c1945c4.inc"
};
static NJS_MKEY_F dat_8c1945e4[2] = {
#include "animal/models/dat_8c1945e4.inc"
};
static NJS_MKEY_A dat_8c194604[2] = {
#include "animal/models/dat_8c194604.inc"
};
static NJS_MKEY_F dat_8c194624[2] = {
#include "animal/models/dat_8c194624.inc"
};
static NJS_MKEY_A dat_8c194644[17] = {
#include "animal/models/dat_8c194644.inc"
};
static NJS_MKEY_F dat_8c194754[2] = {
#include "animal/models/dat_8c194754.inc"
};
static NJS_MKEY_A dat_8c194774[2] = {
#include "animal/models/dat_8c194774.inc"
};
static NJS_MKEY_F dat_8c194794[2] = {
#include "animal/models/dat_8c194794.inc"
};
static NJS_MKEY_A dat_8c1947b4[2] = {
#include "animal/models/dat_8c1947b4.inc"
};
static NJS_MKEY_F dat_8c1947d4[2] = {
#include "animal/models/dat_8c1947d4.inc"
};
static NJS_MKEY_A dat_8c1947f4[2] = {
#include "animal/models/dat_8c1947f4.inc"
};
static NJS_MKEY_F dat_8c194814[2] = {
#include "animal/models/dat_8c194814.inc"
};
static NJS_MKEY_A dat_8c194834[17] = {
#include "animal/models/dat_8c194834.inc"
};
static NJS_MKEY_F dat_8c194944[2] = {
#include "animal/models/dat_8c194944.inc"
};
static NJS_MKEY_A dat_8c194964[2] = {
#include "animal/models/dat_8c194964.inc"
};
static NJS_MKEY_F dat_8c194984[2] = {
#include "animal/models/dat_8c194984.inc"
};
static NJS_MKEY_A dat_8c1949a4[2] = {
#include "animal/models/dat_8c1949a4.inc"
};
static NJS_MKEY_F dat_8c1949c4[17] = {
#include "animal/models/dat_8c1949c4.inc"
};
static NJS_MKEY_A dat_8c194ad4[2] = {
#include "animal/models/dat_8c194ad4.inc"
};
static NJS_MKEY_F dat_8c194af4[2] = {
#include "animal/models/dat_8c194af4.inc"
};
static NJS_MKEY_A dat_8c194b14[17] = {
#include "animal/models/dat_8c194b14.inc"
};
static NJS_MKEY_F dat_8c194c24[2] = {
#include "animal/models/dat_8c194c24.inc"
};
static NJS_MKEY_A dat_8c194c44[2] = {
#include "animal/models/dat_8c194c44.inc"
};
static NJS_MKEY_F dat_8c194c64[2] = {
#include "animal/models/dat_8c194c64.inc"
};
static NJS_MKEY_A dat_8c194c84[2] = {
#include "animal/models/dat_8c194c84.inc"
};
static NJS_MKEY_F dat_8c194ca4[17] = {
#include "animal/models/dat_8c194ca4.inc"
};
static NJS_MKEY_A dat_8c194db4[2] = {
#include "animal/models/dat_8c194db4.inc"
};
static NJS_MKEY_F dat_8c194dd4[2] = {
#include "animal/models/dat_8c194dd4.inc"
};
static NJS_MKEY_A dat_8c194df4[17] = {
#include "animal/models/dat_8c194df4.inc"
};
static NJS_MKEY_F dat_8c194f04[2] = {
#include "animal/models/dat_8c194f04.inc"
};
static NJS_MKEY_A dat_8c194f24[2] = {
#include "animal/models/dat_8c194f24.inc"
};
static NJS_MKEY_F dat_8c194f44[2] = {
#include "animal/models/dat_8c194f44.inc"
};
static NJS_MKEY_A dat_8c194f64[2] = {
#include "animal/models/dat_8c194f64.inc"
};
static NJS_MKEY_F dat_8c194f84[17] = {
#include "animal/models/dat_8c194f84.inc"
};
static NJS_MKEY_A dat_8c195094[2] = {
#include "animal/models/dat_8c195094.inc"
};
static NJS_MKEY_F dat_8c1950b4[2] = {
#include "animal/models/dat_8c1950b4.inc"
};
static NJS_MKEY_A dat_8c1950d4[17] = {
#include "animal/models/dat_8c1950d4.inc"
};
static NJS_MKEY_F dat_8c1951e4[2] = {
#include "animal/models/dat_8c1951e4.inc"
};
static NJS_MKEY_A dat_8c195204[2] = {
#include "animal/models/dat_8c195204.inc"
};
static NJS_MKEY_F dat_8c195224[2] = {
#include "animal/models/dat_8c195224.inc"
};
static NJS_MKEY_A dat_8c195244[2] = {
#include "animal/models/dat_8c195244.inc"
};
static NJS_MKEY_F dat_8c195264[17] = {
#include "animal/models/dat_8c195264.inc"
};
static NJS_MKEY_A dat_8c195374[2] = {
#include "animal/models/dat_8c195374.inc"
};
static NJS_MKEY_F dat_8c195394[2] = {
#include "animal/models/dat_8c195394.inc"
};
static NJS_MKEY_A dat_8c1953b4[17] = {
#include "animal/models/dat_8c1953b4.inc"
};
static NJS_MKEY_F dat_8c1954c4[2] = {
#include "animal/models/dat_8c1954c4.inc"
};
static NJS_MKEY_A dat_8c1954e4[2] = {
#include "animal/models/dat_8c1954e4.inc"
};
static NJS_MKEY_F dat_8c195504[2] = {
#include "animal/models/dat_8c195504.inc"
};
static NJS_MKEY_A dat_8c195524[2] = {
#include "animal/models/dat_8c195524.inc"
};
static NJS_MKEY_F dat_8c195544[2] = {
#include "animal/models/dat_8c195544.inc"
};
static NJS_MKEY_A dat_8c195564[2] = {
#include "animal/models/dat_8c195564.inc"
};
static NJS_MKEY_F dat_8c195584[2] = {
#include "animal/models/dat_8c195584.inc"
};
static NJS_MKEY_A dat_8c1955a4[17] = {
#include "animal/models/dat_8c1955a4.inc"
};
static NJS_MKEY_F dat_8c1956b4[2] = {
#include "animal/models/dat_8c1956b4.inc"
};
static NJS_MKEY_A dat_8c1956d4[2] = {
#include "animal/models/dat_8c1956d4.inc"
};
static NJS_MKEY_F dat_8c1956f4[2] = {
#include "animal/models/dat_8c1956f4.inc"
};
static NJS_MKEY_A dat_8c195714[2] = {
#include "animal/models/dat_8c195714.inc"
};
static NJS_MDATA2 dat_8c195734[41] = {
#include "animal/models/dat_8c195734.inc"
};
static NJS_MOTION zou_motion = {
#include "animal/models/zou_motion.inc"
};
NJS_ACTION zou_action = {
#include "animal/models/zou_action.inc"
};
static NJS_TEXNAME zou_textures[3] = {
    {TEXT_46, 0, 0},
    {TEXT_47, 0, 0},
    {TEXT_48, 0, 0},
};
NJS_TEXLIST zou_texlist = {
#include "animal/models/zou_texlist.inc"
};
static NJS_MATERIAL dat_8c195a04[1] = {
#include "animal/models/dat_8c195a04.inc"
};
static Sint16 dat_8c195a18[5] = {
#include "animal/models/dat_8c195a18.inc"
};
static NJS_COLOR dat_8c195a24[4] = {
#include "animal/models/dat_8c195a24.inc"
};
static NJS_MESHSET dat_8c195a34[1] = {
#include "animal/models/dat_8c195a34.inc"
};
static NJS_POINT3 dat_8c195a4c[4] = {
#include "animal/models/dat_8c195a4c.inc"
};
static NJS_VECTOR dat_8c195a7c[4] = {
#include "animal/models/dat_8c195a7c.inc"
};
static NJS_MODEL dat_8c195aac = {
#include "animal/models/dat_8c195aac.inc"
};
static NJS_OBJECT dat_8c195ad4 = {
#include "animal/models/dat_8c195ad4.inc"
};
static NJS_OBJECT dat_8c195b08 = {
#include "animal/models/dat_8c195b08.inc"
};
static NJS_OBJECT dat_8c195b3c = {
#include "animal/models/dat_8c195b3c.inc"
};
static NJS_OBJECT dat_8c195b70 = {
#include "animal/models/dat_8c195b70.inc"
};
static NJS_MATERIAL dat_8c195ba4[1] = {
#include "animal/models/dat_8c195ba4.inc"
};
static Sint16 dat_8c195bb8[5] = {
#include "animal/models/dat_8c195bb8.inc"
};
static NJS_COLOR dat_8c195bc4[4] = {
#include "animal/models/dat_8c195bc4.inc"
};
static NJS_MESHSET dat_8c195bd4[1] = {
#include "animal/models/dat_8c195bd4.inc"
};
static NJS_POINT3 dat_8c195bec[4] = {
#include "animal/models/dat_8c195bec.inc"
};
static NJS_VECTOR dat_8c195c1c[4] = {
#include "animal/models/dat_8c195c1c.inc"
};
static NJS_MODEL dat_8c195c4c = {
#include "animal/models/dat_8c195c4c.inc"
};
static NJS_OBJECT dat_8c195c74 = {
#include "animal/models/dat_8c195c74.inc"
};
static NJS_OBJECT dat_8c195ca8 = {
#include "animal/models/dat_8c195ca8.inc"
};
static NJS_OBJECT dat_8c195cdc = {
#include "animal/models/dat_8c195cdc.inc"
};
static NJS_OBJECT dat_8c195d10 = {
#include "animal/models/dat_8c195d10.inc"
};
static NJS_MATERIAL dat_8c195d44[1] = {
#include "animal/models/dat_8c195d44.inc"
};
static Sint16 dat_8c195d58[15] = {
#include "animal/models/dat_8c195d58.inc"
};
static NJS_COLOR dat_8c195d78[13] = {
#include "animal/models/dat_8c195d78.inc"
};
static NJS_MESHSET dat_8c195dac[1] = {
#include "animal/models/dat_8c195dac.inc"
};
static NJS_POINT3 dat_8c195dc4[9] = {
#include "animal/models/dat_8c195dc4.inc"
};
static NJS_VECTOR dat_8c195e30[9] = {
#include "animal/models/dat_8c195e30.inc"
};
static NJS_MODEL dat_8c195e9c = {
#include "animal/models/dat_8c195e9c.inc"
};
static NJS_OBJECT dat_8c195ec4 = {
#include "animal/models/dat_8c195ec4.inc"
};
static NJS_OBJECT dat_8c195ef8 = {
#include "animal/models/dat_8c195ef8.inc"
};
static NJS_OBJECT dat_8c195f2c = {
#include "animal/models/dat_8c195f2c.inc"
};
static NJS_OBJECT dat_8c195f60 = {
#include "animal/models/dat_8c195f60.inc"
};
static NJS_MATERIAL dat_8c195f94[1] = {
#include "animal/models/dat_8c195f94.inc"
};
static Sint16 dat_8c195fa8[39] = {
#include "animal/models/dat_8c195fa8.inc"
};
static NJS_COLOR dat_8c195ff8[35] = {
#include "animal/models/dat_8c195ff8.inc"
};
static NJS_MESHSET dat_8c196084[1] = {
#include "animal/models/dat_8c196084.inc"
};
static NJS_POINT3 dat_8c19609c[17] = {
#include "animal/models/dat_8c19609c.inc"
};
static NJS_VECTOR dat_8c196168[17] = {
#include "animal/models/dat_8c196168.inc"
};
static NJS_MODEL dat_8c196234 = {
#include "animal/models/dat_8c196234.inc"
};
static NJS_OBJECT dat_8c19625c = {
#include "animal/models/dat_8c19625c.inc"
};
static NJS_OBJECT dat_8c196290 = {
#include "animal/models/dat_8c196290.inc"
};
static NJS_OBJECT dat_8c1962c4 = {
#include "animal/models/dat_8c1962c4.inc"
};
static NJS_OBJECT dat_8c1962f8 = {
#include "animal/models/dat_8c1962f8.inc"
};
static NJS_MATERIAL dat_8c19632c[1] = {
#include "animal/models/dat_8c19632c.inc"
};
static Sint16 dat_8c196340[12] = {
#include "animal/models/dat_8c196340.inc"
};
static NJS_COLOR dat_8c196358[10] = {
#include "animal/models/dat_8c196358.inc"
};
static NJS_MESHSET dat_8c196380[1] = {
#include "animal/models/dat_8c196380.inc"
};
static NJS_POINT3 dat_8c196398[5] = {
#include "animal/models/dat_8c196398.inc"
};
static NJS_VECTOR dat_8c1963d4[5] = {
#include "animal/models/dat_8c1963d4.inc"
};
static NJS_MODEL dat_8c196410 = {
#include "animal/models/dat_8c196410.inc"
};
static NJS_OBJECT dat_8c196438 = {
#include "animal/models/dat_8c196438.inc"
};
static NJS_OBJECT dat_8c19646c = {
#include "animal/models/dat_8c19646c.inc"
};
static NJS_OBJECT dat_8c1964a0 = {
#include "animal/models/dat_8c1964a0.inc"
};
static NJS_OBJECT dat_8c1964d4 = {
#include "animal/models/dat_8c1964d4.inc"
};
static NJS_MATERIAL dat_8c196508[1] = {
#include "animal/models/dat_8c196508.inc"
};
static Sint16 dat_8c19651c[12] = {
#include "animal/models/dat_8c19651c.inc"
};
static NJS_COLOR dat_8c196534[10] = {
#include "animal/models/dat_8c196534.inc"
};
static NJS_MESHSET dat_8c19655c[1] = {
#include "animal/models/dat_8c19655c.inc"
};
static NJS_POINT3 dat_8c196574[5] = {
#include "animal/models/dat_8c196574.inc"
};
static NJS_VECTOR dat_8c1965b0[5] = {
#include "animal/models/dat_8c1965b0.inc"
};
static NJS_MODEL dat_8c1965ec = {
#include "animal/models/dat_8c1965ec.inc"
};
static NJS_OBJECT dat_8c196614 = {
#include "animal/models/dat_8c196614.inc"
};
static NJS_OBJECT dat_8c196648 = {
#include "animal/models/dat_8c196648.inc"
};
static NJS_OBJECT dat_8c19667c = {
#include "animal/models/dat_8c19667c.inc"
};
static NJS_OBJECT dat_8c1966b0 = {
#include "animal/models/dat_8c1966b0.inc"
};
static NJS_MATERIAL dat_8c1966e4[1] = {
#include "animal/models/dat_8c1966e4.inc"
};
static Sint16 dat_8c1966f8[12] = {
#include "animal/models/dat_8c1966f8.inc"
};
static NJS_COLOR dat_8c196710[10] = {
#include "animal/models/dat_8c196710.inc"
};
static NJS_MESHSET dat_8c196738[1] = {
#include "animal/models/dat_8c196738.inc"
};
static NJS_POINT3 dat_8c196750[5] = {
#include "animal/models/dat_8c196750.inc"
};
static NJS_VECTOR dat_8c19678c[5] = {
#include "animal/models/dat_8c19678c.inc"
};
static NJS_MODEL dat_8c1967c8 = {
#include "animal/models/dat_8c1967c8.inc"
};
static NJS_OBJECT dat_8c1967f0 = {
#include "animal/models/dat_8c1967f0.inc"
};
static NJS_OBJECT dat_8c196824 = {
#include "animal/models/dat_8c196824.inc"
};
static NJS_OBJECT dat_8c196858 = {
#include "animal/models/dat_8c196858.inc"
};
static NJS_OBJECT dat_8c19688c = {
#include "animal/models/dat_8c19688c.inc"
};
static NJS_MATERIAL dat_8c1968c0[1] = {
#include "animal/models/dat_8c1968c0.inc"
};
static Sint16 dat_8c1968d4[12] = {
#include "animal/models/dat_8c1968d4.inc"
};
static NJS_COLOR dat_8c1968ec[10] = {
#include "animal/models/dat_8c1968ec.inc"
};
static NJS_MESHSET dat_8c196914[1] = {
#include "animal/models/dat_8c196914.inc"
};
static NJS_POINT3 dat_8c19692c[5] = {
#include "animal/models/dat_8c19692c.inc"
};
static NJS_VECTOR dat_8c196968[5] = {
#include "animal/models/dat_8c196968.inc"
};
static NJS_MODEL dat_8c1969a4 = {
#include "animal/models/dat_8c1969a4.inc"
};
static NJS_OBJECT dat_8c1969cc = {
#include "animal/models/dat_8c1969cc.inc"
};
static NJS_OBJECT dat_8c196a00 = {
#include "animal/models/dat_8c196a00.inc"
};
static NJS_OBJECT dat_8c196a34 = {
#include "animal/models/dat_8c196a34.inc"
};
static NJS_OBJECT dat_8c196a68 = {
#include "animal/models/dat_8c196a68.inc"
};
static NJS_MATERIAL dat_8c196a9c[1] = {
#include "animal/models/dat_8c196a9c.inc"
};
static Sint16 dat_8c196ab0[6] = {
#include "animal/models/dat_8c196ab0.inc"
};
static NJS_COLOR dat_8c196abc[5] = {
#include "animal/models/dat_8c196abc.inc"
};
static NJS_MESHSET dat_8c196ad0[1] = {
#include "animal/models/dat_8c196ad0.inc"
};
static NJS_POINT3 dat_8c196ae8[4] = {
#include "animal/models/dat_8c196ae8.inc"
};
static NJS_VECTOR dat_8c196b18[4] = {
#include "animal/models/dat_8c196b18.inc"
};
static NJS_MODEL dat_8c196b48 = {
#include "animal/models/dat_8c196b48.inc"
};
static NJS_OBJECT dat_8c196b70 = {
#include "animal/models/dat_8c196b70.inc"
};
static NJS_MATERIAL dat_8c196ba4[1] = {
#include "animal/models/dat_8c196ba4.inc"
};
static Sint16 dat_8c196bb8[15] = {
#include "animal/models/dat_8c196bb8.inc"
};
static NJS_COLOR dat_8c196bd8[12] = {
#include "animal/models/dat_8c196bd8.inc"
};
static NJS_MESHSET dat_8c196c08[1] = {
#include "animal/models/dat_8c196c08.inc"
};
static NJS_POINT3 dat_8c196c20[5] = {
#include "animal/models/dat_8c196c20.inc"
};
static NJS_VECTOR dat_8c196c5c[5] = {
#include "animal/models/dat_8c196c5c.inc"
};
static NJS_MODEL dat_8c196c98 = {
#include "animal/models/dat_8c196c98.inc"
};
static NJS_OBJECT dat_8c196cc0 = {
#include "animal/models/dat_8c196cc0.inc"
};
static NJS_OBJECT dat_8c196cf4 = {
#include "animal/models/dat_8c196cf4.inc"
};
static NJS_OBJECT dat_8c196d28 = {
#include "animal/models/dat_8c196d28.inc"
};
static NJS_OBJECT dat_8c196d5c = {
#include "animal/models/dat_8c196d5c.inc"
};
static NJS_OBJECT dat_8c196d90 = {
#include "animal/models/dat_8c196d90.inc"
};
static NJS_MATERIAL dat_8c196dc4[1] = {
#include "animal/models/dat_8c196dc4.inc"
};
static Sint16 dat_8c196dd8[48] = {
#include "animal/models/dat_8c196dd8.inc"
};
static NJS_COLOR dat_8c196e38[41] = {
#include "animal/models/dat_8c196e38.inc"
};
static NJS_MESHSET dat_8c196edc[1] = {
#include "animal/models/dat_8c196edc.inc"
};
static NJS_POINT3 dat_8c196ef4[16] = {
#include "animal/models/dat_8c196ef4.inc"
};
static NJS_VECTOR dat_8c196fb4[16] = {
#include "animal/models/dat_8c196fb4.inc"
};
static NJS_MODEL dat_8c197074 = {
#include "animal/models/dat_8c197074.inc"
};
static NJS_OBJECT dat_8c19709c = {
#include "animal/models/dat_8c19709c.inc"
};
static NJS_OBJECT dat_8c1970d0 = {
#include "animal/models/dat_8c1970d0.inc"
};
static NJS_OBJECT dat_8c197104 = {
#include "animal/models/dat_8c197104.inc"
};
static NJS_OBJECT dat_8c197138 = {
#include "animal/models/dat_8c197138.inc"
};
static NJS_OBJECT lion_object = {
#include "animal/models/lion_object.inc"
};
static NJS_MKEY_F dat_8c1971a0[2] = {
#include "animal/models/dat_8c1971a0.inc"
};
static NJS_MKEY_A dat_8c1971c0[2] = {
#include "animal/models/dat_8c1971c0.inc"
};
static NJS_MKEY_F dat_8c1971e0[17] = {
#include "animal/models/dat_8c1971e0.inc"
};
static NJS_MKEY_A dat_8c1972f0[2] = {
#include "animal/models/dat_8c1972f0.inc"
};
static NJS_MKEY_F dat_8c197310[2] = {
#include "animal/models/dat_8c197310.inc"
};
static NJS_MKEY_A dat_8c197330[17] = {
#include "animal/models/dat_8c197330.inc"
};
static NJS_MKEY_F dat_8c197440[2] = {
#include "animal/models/dat_8c197440.inc"
};
static NJS_MKEY_A dat_8c197460[2] = {
#include "animal/models/dat_8c197460.inc"
};
static NJS_MKEY_F dat_8c197480[2] = {
#include "animal/models/dat_8c197480.inc"
};
static NJS_MKEY_A dat_8c1974a0[2] = {
#include "animal/models/dat_8c1974a0.inc"
};
static NJS_MKEY_F dat_8c1974c0[2] = {
#include "animal/models/dat_8c1974c0.inc"
};
static NJS_MKEY_A dat_8c1974e0[2] = {
#include "animal/models/dat_8c1974e0.inc"
};
static NJS_MKEY_F dat_8c197500[2] = {
#include "animal/models/dat_8c197500.inc"
};
static NJS_MKEY_A dat_8c197520[17] = {
#include "animal/models/dat_8c197520.inc"
};
static NJS_MKEY_F dat_8c197630[2] = {
#include "animal/models/dat_8c197630.inc"
};
static NJS_MKEY_A dat_8c197650[17] = {
#include "animal/models/dat_8c197650.inc"
};
static NJS_MKEY_F dat_8c197760[2] = {
#include "animal/models/dat_8c197760.inc"
};
static NJS_MKEY_A dat_8c197780[2] = {
#include "animal/models/dat_8c197780.inc"
};
static NJS_MKEY_F dat_8c1977a0[2] = {
#include "animal/models/dat_8c1977a0.inc"
};
static NJS_MKEY_A dat_8c1977c0[2] = {
#include "animal/models/dat_8c1977c0.inc"
};
static NJS_MKEY_F dat_8c1977e0[2] = {
#include "animal/models/dat_8c1977e0.inc"
};
static NJS_MKEY_A dat_8c197800[2] = {
#include "animal/models/dat_8c197800.inc"
};
static NJS_MKEY_F dat_8c197820[17] = {
#include "animal/models/dat_8c197820.inc"
};
static NJS_MKEY_A dat_8c197930[2] = {
#include "animal/models/dat_8c197930.inc"
};
static NJS_MKEY_F dat_8c197950[2] = {
#include "animal/models/dat_8c197950.inc"
};
static NJS_MKEY_A dat_8c197970[17] = {
#include "animal/models/dat_8c197970.inc"
};
static NJS_MKEY_F dat_8c197a80[2] = {
#include "animal/models/dat_8c197a80.inc"
};
static NJS_MKEY_A dat_8c197aa0[2] = {
#include "animal/models/dat_8c197aa0.inc"
};
static NJS_MKEY_F dat_8c197ac0[2] = {
#include "animal/models/dat_8c197ac0.inc"
};
static NJS_MKEY_A dat_8c197ae0[2] = {
#include "animal/models/dat_8c197ae0.inc"
};
static NJS_MKEY_F dat_8c197b00[17] = {
#include "animal/models/dat_8c197b00.inc"
};
static NJS_MKEY_A dat_8c197c10[2] = {
#include "animal/models/dat_8c197c10.inc"
};
static NJS_MKEY_F dat_8c197c30[2] = {
#include "animal/models/dat_8c197c30.inc"
};
static NJS_MKEY_A dat_8c197c50[17] = {
#include "animal/models/dat_8c197c50.inc"
};
static NJS_MKEY_F dat_8c197d60[2] = {
#include "animal/models/dat_8c197d60.inc"
};
static NJS_MKEY_A dat_8c197d80[2] = {
#include "animal/models/dat_8c197d80.inc"
};
static NJS_MKEY_F dat_8c197da0[2] = {
#include "animal/models/dat_8c197da0.inc"
};
static NJS_MKEY_A dat_8c197dc0[2] = {
#include "animal/models/dat_8c197dc0.inc"
};
static NJS_MKEY_F dat_8c197de0[17] = {
#include "animal/models/dat_8c197de0.inc"
};
static NJS_MKEY_A dat_8c197ef0[2] = {
#include "animal/models/dat_8c197ef0.inc"
};
static NJS_MKEY_F dat_8c197f10[2] = {
#include "animal/models/dat_8c197f10.inc"
};
static NJS_MKEY_A dat_8c197f30[16] = {
#include "animal/models/dat_8c197f30.inc"
};
static NJS_MKEY_F dat_8c198030[2] = {
#include "animal/models/dat_8c198030.inc"
};
static NJS_MKEY_A dat_8c198050[2] = {
#include "animal/models/dat_8c198050.inc"
};
static NJS_MKEY_F dat_8c198070[2] = {
#include "animal/models/dat_8c198070.inc"
};
static NJS_MKEY_A dat_8c198090[2] = {
#include "animal/models/dat_8c198090.inc"
};
static NJS_MKEY_F dat_8c1980b0[17] = {
#include "animal/models/dat_8c1980b0.inc"
};
static NJS_MKEY_A dat_8c1981c0[2] = {
#include "animal/models/dat_8c1981c0.inc"
};
static NJS_MKEY_F dat_8c1981e0[2] = {
#include "animal/models/dat_8c1981e0.inc"
};
static NJS_MKEY_A dat_8c198200[17] = {
#include "animal/models/dat_8c198200.inc"
};
static NJS_MKEY_F dat_8c198310[2] = {
#include "animal/models/dat_8c198310.inc"
};
static NJS_MKEY_A dat_8c198330[2] = {
#include "animal/models/dat_8c198330.inc"
};
static NJS_MKEY_F dat_8c198350[2] = {
#include "animal/models/dat_8c198350.inc"
};
static NJS_MKEY_A dat_8c198370[2] = {
#include "animal/models/dat_8c198370.inc"
};
static NJS_MKEY_F dat_8c198390[2] = {
#include "animal/models/dat_8c198390.inc"
};
static NJS_MKEY_A dat_8c1983b0[2] = {
#include "animal/models/dat_8c1983b0.inc"
};
static NJS_MKEY_F dat_8c1983d0[2] = {
#include "animal/models/dat_8c1983d0.inc"
};
static NJS_MKEY_A dat_8c1983f0[17] = {
#include "animal/models/dat_8c1983f0.inc"
};
static NJS_MKEY_F dat_8c198500[2] = {
#include "animal/models/dat_8c198500.inc"
};
static NJS_MKEY_A dat_8c198520[2] = {
#include "animal/models/dat_8c198520.inc"
};
static NJS_MKEY_F dat_8c198540[2] = {
#include "animal/models/dat_8c198540.inc"
};
static NJS_MKEY_A dat_8c198560[2] = {
#include "animal/models/dat_8c198560.inc"
};
static NJS_MKEY_F dat_8c198580[2] = {
#include "animal/models/dat_8c198580.inc"
};
static NJS_MKEY_A dat_8c1985a0[2] = {
#include "animal/models/dat_8c1985a0.inc"
};
static NJS_MKEY_F dat_8c1985c0[2] = {
#include "animal/models/dat_8c1985c0.inc"
};
static NJS_MKEY_A dat_8c1985e0[17] = {
#include "animal/models/dat_8c1985e0.inc"
};
static NJS_MKEY_F dat_8c1986f0[2] = {
#include "animal/models/dat_8c1986f0.inc"
};
static NJS_MKEY_A dat_8c198710[2] = {
#include "animal/models/dat_8c198710.inc"
};
static NJS_MKEY_F dat_8c198730[2] = {
#include "animal/models/dat_8c198730.inc"
};
static NJS_MKEY_A dat_8c198750[2] = {
#include "animal/models/dat_8c198750.inc"
};
static NJS_MKEY_F dat_8c198770[2] = {
#include "animal/models/dat_8c198770.inc"
};
static NJS_MKEY_A dat_8c198790[2] = {
#include "animal/models/dat_8c198790.inc"
};
static NJS_MKEY_F dat_8c1987b0[2] = {
#include "animal/models/dat_8c1987b0.inc"
};
static NJS_MKEY_A dat_8c1987d0[2] = {
#include "animal/models/dat_8c1987d0.inc"
};
static NJS_MKEY_F dat_8c1987f0[2] = {
#include "animal/models/dat_8c1987f0.inc"
};
static NJS_MKEY_A dat_8c198810[2] = {
#include "animal/models/dat_8c198810.inc"
};
static NJS_MKEY_F dat_8c198830[2] = {
#include "animal/models/dat_8c198830.inc"
};
static NJS_MKEY_A dat_8c198850[2] = {
#include "animal/models/dat_8c198850.inc"
};
static NJS_MKEY_F dat_8c198870[2] = {
#include "animal/models/dat_8c198870.inc"
};
static NJS_MKEY_A dat_8c198890[2] = {
#include "animal/models/dat_8c198890.inc"
};
static NJS_MKEY_F dat_8c1988b0[2] = {
#include "animal/models/dat_8c1988b0.inc"
};
static NJS_MKEY_A dat_8c1988d0[2] = {
#include "animal/models/dat_8c1988d0.inc"
};
static NJS_MKEY_F dat_8c1988f0[2] = {
#include "animal/models/dat_8c1988f0.inc"
};
static NJS_MKEY_A dat_8c198910[2] = {
#include "animal/models/dat_8c198910.inc"
};
static NJS_MKEY_F dat_8c198930[2] = {
#include "animal/models/dat_8c198930.inc"
};
static NJS_MKEY_A dat_8c198950[2] = {
#include "animal/models/dat_8c198950.inc"
};
static NJS_MDATA2 dat_8c198970[43] = {
#include "animal/models/dat_8c198970.inc"
};
static NJS_MOTION lion_motion = {
#include "animal/models/lion_motion.inc"
};
NJS_ACTION lion_action = {
#include "animal/models/lion_action.inc"
};
static NJS_TEXNAME lion_textures[4] = {
    {TEXT_49, 0, 0},
    {TEXT_50, 0, 0},
    {TEXT_51, 0, 0},
    {TEXT_52, 0, 0},
};
NJS_TEXLIST lion_texlist = {
#include "animal/models/lion_texlist.inc"
};
static NJS_MATERIAL dat_8c198c6c[1] = {
#include "animal/models/dat_8c198c6c.inc"
};
static Sint16 dat_8c198c80[22] = {
#include "animal/models/dat_8c198c80.inc"
};
static NJS_COLOR dat_8c198cac[18] = {
#include "animal/models/dat_8c198cac.inc"
};
static NJS_MESHSET dat_8c198cf4[1] = {
#include "animal/models/dat_8c198cf4.inc"
};
static NJS_POINT3 dat_8c198d0c[7] = {
#include "animal/models/dat_8c198d0c.inc"
};
static NJS_VECTOR dat_8c198d60[7] = {
#include "animal/models/dat_8c198d60.inc"
};
static NJS_MODEL dat_8c198db4 = {
#include "animal/models/dat_8c198db4.inc"
};
static NJS_OBJECT dat_8c198ddc = {
#include "animal/models/dat_8c198ddc.inc"
};
static NJS_OBJECT dat_8c198e10 = {
#include "animal/models/dat_8c198e10.inc"
};
static NJS_OBJECT dat_8c198e44 = {
#include "animal/models/dat_8c198e44.inc"
};
static NJS_OBJECT dat_8c198e78 = {
#include "animal/models/dat_8c198e78.inc"
};
static NJS_MATERIAL dat_8c198eac[1] = {
#include "animal/models/dat_8c198eac.inc"
};
static Sint16 dat_8c198ec0[22] = {
#include "animal/models/dat_8c198ec0.inc"
};
static NJS_COLOR dat_8c198eec[18] = {
#include "animal/models/dat_8c198eec.inc"
};
static NJS_MESHSET dat_8c198f34[1] = {
#include "animal/models/dat_8c198f34.inc"
};
static NJS_POINT3 dat_8c198f4c[7] = {
#include "animal/models/dat_8c198f4c.inc"
};
static NJS_VECTOR dat_8c198fa0[7] = {
#include "animal/models/dat_8c198fa0.inc"
};
static NJS_MODEL dat_8c198ff4 = {
#include "animal/models/dat_8c198ff4.inc"
};
static NJS_OBJECT dat_8c19901c = {
#include "animal/models/dat_8c19901c.inc"
};
static NJS_OBJECT dat_8c199050 = {
#include "animal/models/dat_8c199050.inc"
};
static NJS_OBJECT dat_8c199084 = {
#include "animal/models/dat_8c199084.inc"
};
static NJS_OBJECT dat_8c1990b8 = {
#include "animal/models/dat_8c1990b8.inc"
};
static NJS_MATERIAL dat_8c1990ec[1] = {
#include "animal/models/dat_8c1990ec.inc"
};
static Sint16 dat_8c199100[26] = {
#include "animal/models/dat_8c199100.inc"
};
static NJS_COLOR dat_8c199134[22] = {
#include "animal/models/dat_8c199134.inc"
};
static NJS_MESHSET dat_8c19918c[1] = {
#include "animal/models/dat_8c19918c.inc"
};
static NJS_POINT3 dat_8c1991a4[9] = {
#include "animal/models/dat_8c1991a4.inc"
};
static NJS_VECTOR dat_8c199210[9] = {
#include "animal/models/dat_8c199210.inc"
};
static NJS_MODEL dat_8c19927c = {
#include "animal/models/dat_8c19927c.inc"
};
static NJS_OBJECT dat_8c1992a4 = {
#include "animal/models/dat_8c1992a4.inc"
};
static NJS_OBJECT dat_8c1992d8 = {
#include "animal/models/dat_8c1992d8.inc"
};
static NJS_OBJECT dat_8c19930c = {
#include "animal/models/dat_8c19930c.inc"
};
static NJS_OBJECT dat_8c199340 = {
#include "animal/models/dat_8c199340.inc"
};
static NJS_MATERIAL dat_8c199374[1] = {
#include "animal/models/dat_8c199374.inc"
};
static Sint16 dat_8c199388[26] = {
#include "animal/models/dat_8c199388.inc"
};
static NJS_COLOR dat_8c1993bc[22] = {
#include "animal/models/dat_8c1993bc.inc"
};
static NJS_MESHSET dat_8c199414[1] = {
#include "animal/models/dat_8c199414.inc"
};
static NJS_POINT3 dat_8c19942c[9] = {
#include "animal/models/dat_8c19942c.inc"
};
static NJS_VECTOR dat_8c199498[9] = {
#include "animal/models/dat_8c199498.inc"
};
static NJS_MODEL dat_8c199504 = {
#include "animal/models/dat_8c199504.inc"
};
static NJS_OBJECT dat_8c19952c = {
#include "animal/models/dat_8c19952c.inc"
};
static NJS_OBJECT dat_8c199560 = {
#include "animal/models/dat_8c199560.inc"
};
static NJS_OBJECT dat_8c199594 = {
#include "animal/models/dat_8c199594.inc"
};
static NJS_OBJECT dat_8c1995c8 = {
#include "animal/models/dat_8c1995c8.inc"
};
static NJS_MATERIAL dat_8c1995fc[1] = {
#include "animal/models/dat_8c1995fc.inc"
};
static Sint16 dat_8c199610[80] = {
#include "animal/models/dat_8c199610.inc"
};
static NJS_COLOR dat_8c1996b0[70] = {
#include "animal/models/dat_8c1996b0.inc"
};
static NJS_MESHSET dat_8c1997c8[1] = {
#include "animal/models/dat_8c1997c8.inc"
};
static NJS_POINT3 dat_8c1997e0[27] = {
#include "animal/models/dat_8c1997e0.inc"
};
static NJS_VECTOR dat_8c199924[27] = {
#include "animal/models/dat_8c199924.inc"
};
static NJS_MODEL dat_8c199a68 = {
#include "animal/models/dat_8c199a68.inc"
};
static NJS_OBJECT dat_8c199a90 = {
#include "animal/models/dat_8c199a90.inc"
};
static NJS_OBJECT dat_8c199ac4 = {
#include "animal/models/dat_8c199ac4.inc"
};
static NJS_OBJECT dat_8c199af8 = {
#include "animal/models/dat_8c199af8.inc"
};
static NJS_OBJECT dat_8c199b2c = {
#include "animal/models/dat_8c199b2c.inc"
};
static NJS_OBJECT gori_object = {
#include "animal/models/gori_object.inc"
};
static NJS_MKEY_F dat_8c199b94[2] = {
#include "animal/models/dat_8c199b94.inc"
};
static NJS_MKEY_A dat_8c199bb4[2] = {
#include "animal/models/dat_8c199bb4.inc"
};
static NJS_MKEY_F dat_8c199bd4[17] = {
#include "animal/models/dat_8c199bd4.inc"
};
static NJS_MKEY_A dat_8c199ce4[2] = {
#include "animal/models/dat_8c199ce4.inc"
};
static NJS_MKEY_F dat_8c199d04[2] = {
#include "animal/models/dat_8c199d04.inc"
};
static NJS_MKEY_A dat_8c199d24[17] = {
#include "animal/models/dat_8c199d24.inc"
};
static NJS_MKEY_F dat_8c199e34[2] = {
#include "animal/models/dat_8c199e34.inc"
};
static NJS_MKEY_A dat_8c199e54[2] = {
#include "animal/models/dat_8c199e54.inc"
};
static NJS_MKEY_F dat_8c199e74[2] = {
#include "animal/models/dat_8c199e74.inc"
};
static NJS_MKEY_A dat_8c199e94[2] = {
#include "animal/models/dat_8c199e94.inc"
};
static NJS_MKEY_F dat_8c199eb4[2] = {
#include "animal/models/dat_8c199eb4.inc"
};
static NJS_MKEY_A dat_8c199ed4[2] = {
#include "animal/models/dat_8c199ed4.inc"
};
static NJS_MKEY_F dat_8c199ef4[2] = {
#include "animal/models/dat_8c199ef4.inc"
};
static NJS_MKEY_A dat_8c199f14[17] = {
#include "animal/models/dat_8c199f14.inc"
};
static NJS_MKEY_F dat_8c19a024[2] = {
#include "animal/models/dat_8c19a024.inc"
};
static NJS_MKEY_A dat_8c19a044[2] = {
#include "animal/models/dat_8c19a044.inc"
};
static NJS_MKEY_F dat_8c19a064[2] = {
#include "animal/models/dat_8c19a064.inc"
};
static NJS_MKEY_A dat_8c19a084[2] = {
#include "animal/models/dat_8c19a084.inc"
};
static NJS_MKEY_F dat_8c19a0a4[2] = {
#include "animal/models/dat_8c19a0a4.inc"
};
static NJS_MKEY_A dat_8c19a0c4[2] = {
#include "animal/models/dat_8c19a0c4.inc"
};
static NJS_MKEY_F dat_8c19a0e4[2] = {
#include "animal/models/dat_8c19a0e4.inc"
};
static NJS_MKEY_A dat_8c19a104[17] = {
#include "animal/models/dat_8c19a104.inc"
};
static NJS_MKEY_F dat_8c19a214[2] = {
#include "animal/models/dat_8c19a214.inc"
};
static NJS_MKEY_A dat_8c19a234[2] = {
#include "animal/models/dat_8c19a234.inc"
};
static NJS_MKEY_F dat_8c19a254[2] = {
#include "animal/models/dat_8c19a254.inc"
};
static NJS_MKEY_A dat_8c19a274[2] = {
#include "animal/models/dat_8c19a274.inc"
};
static NJS_MKEY_F dat_8c19a294[17] = {
#include "animal/models/dat_8c19a294.inc"
};
static NJS_MKEY_A dat_8c19a3a4[2] = {
#include "animal/models/dat_8c19a3a4.inc"
};
static NJS_MKEY_F dat_8c19a3c4[2] = {
#include "animal/models/dat_8c19a3c4.inc"
};
static NJS_MKEY_A dat_8c19a3e4[17] = {
#include "animal/models/dat_8c19a3e4.inc"
};
static NJS_MKEY_F dat_8c19a4f4[2] = {
#include "animal/models/dat_8c19a4f4.inc"
};
static NJS_MKEY_A dat_8c19a514[2] = {
#include "animal/models/dat_8c19a514.inc"
};
static NJS_MKEY_F dat_8c19a534[2] = {
#include "animal/models/dat_8c19a534.inc"
};
static NJS_MKEY_A dat_8c19a554[2] = {
#include "animal/models/dat_8c19a554.inc"
};
static NJS_MKEY_F dat_8c19a574[17] = {
#include "animal/models/dat_8c19a574.inc"
};
static NJS_MKEY_A dat_8c19a684[2] = {
#include "animal/models/dat_8c19a684.inc"
};
static NJS_MKEY_F dat_8c19a6a4[2] = {
#include "animal/models/dat_8c19a6a4.inc"
};
static NJS_MKEY_A dat_8c19a6c4[17] = {
#include "animal/models/dat_8c19a6c4.inc"
};
static NJS_MKEY_F dat_8c19a7d4[2] = {
#include "animal/models/dat_8c19a7d4.inc"
};
static NJS_MKEY_A dat_8c19a7f4[2] = {
#include "animal/models/dat_8c19a7f4.inc"
};
static NJS_MKEY_F dat_8c19a814[2] = {
#include "animal/models/dat_8c19a814.inc"
};
static NJS_MKEY_A dat_8c19a834[2] = {
#include "animal/models/dat_8c19a834.inc"
};
static NJS_MDATA2 dat_8c19a854[21] = {
#include "animal/models/dat_8c19a854.inc"
};
static NJS_MOTION gori_motion = {
#include "animal/models/gori_motion.inc"
};
NJS_ACTION gori_action = {
#include "animal/models/gori_action.inc"
};
static NJS_TEXNAME gori_textures[2] = {
    {TEXT_53, 0, 0},
    {TEXT_54, 0, 0},
};
NJS_TEXLIST gori_texlist = {
#include "animal/models/gori_texlist.inc"
};
static NJS_MATERIAL dat_8c19a9d8[1] = {
#include "animal/models/dat_8c19a9d8.inc"
};
static Sint16 dat_8c19a9ec[37] = {
#include "animal/models/dat_8c19a9ec.inc"
};
static NJS_COLOR dat_8c19aa38[32] = {
#include "animal/models/dat_8c19aa38.inc"
};
static NJS_MESHSET dat_8c19aab8[1] = {
#include "animal/models/dat_8c19aab8.inc"
};
static NJS_POINT3 dat_8c19aad0[13] = {
#include "animal/models/dat_8c19aad0.inc"
};
static NJS_VECTOR dat_8c19ab6c[13] = {
#include "animal/models/dat_8c19ab6c.inc"
};
static NJS_MODEL dat_8c19ac08 = {
#include "animal/models/dat_8c19ac08.inc"
};
static NJS_OBJECT dat_8c19ac30 = {
#include "animal/models/dat_8c19ac30.inc"
};
static NJS_OBJECT dat_8c19ac64 = {
#include "animal/models/dat_8c19ac64.inc"
};
static NJS_OBJECT dat_8c19ac98 = {
#include "animal/models/dat_8c19ac98.inc"
};
static NJS_OBJECT dat_8c19accc = {
#include "animal/models/dat_8c19accc.inc"
};
static NJS_MATERIAL dat_8c19ad00[1] = {
#include "animal/models/dat_8c19ad00.inc"
};
static Sint16 dat_8c19ad14[12] = {
#include "animal/models/dat_8c19ad14.inc"
};
static NJS_COLOR dat_8c19ad2c[10] = {
#include "animal/models/dat_8c19ad2c.inc"
};
static NJS_MESHSET dat_8c19ad54[1] = {
#include "animal/models/dat_8c19ad54.inc"
};
static NJS_POINT3 dat_8c19ad6c[5] = {
#include "animal/models/dat_8c19ad6c.inc"
};
static NJS_VECTOR dat_8c19ada8[5] = {
#include "animal/models/dat_8c19ada8.inc"
};
static NJS_MODEL dat_8c19ade4 = {
#include "animal/models/dat_8c19ade4.inc"
};
static NJS_OBJECT dat_8c19ae0c = {
#include "animal/models/dat_8c19ae0c.inc"
};
static NJS_OBJECT dat_8c19ae40 = {
#include "animal/models/dat_8c19ae40.inc"
};
static NJS_OBJECT dat_8c19ae74 = {
#include "animal/models/dat_8c19ae74.inc"
};
static NJS_OBJECT dat_8c19aea8 = {
#include "animal/models/dat_8c19aea8.inc"
};
static NJS_MATERIAL dat_8c19aedc[1] = {
#include "animal/models/dat_8c19aedc.inc"
};
static Sint16 dat_8c19aef0[12] = {
#include "animal/models/dat_8c19aef0.inc"
};
static NJS_COLOR dat_8c19af08[10] = {
#include "animal/models/dat_8c19af08.inc"
};
static NJS_MESHSET dat_8c19af30[1] = {
#include "animal/models/dat_8c19af30.inc"
};
static NJS_POINT3 dat_8c19af48[5] = {
#include "animal/models/dat_8c19af48.inc"
};
static NJS_VECTOR dat_8c19af84[5] = {
#include "animal/models/dat_8c19af84.inc"
};
static NJS_MODEL dat_8c19afc0 = {
#include "animal/models/dat_8c19afc0.inc"
};
static NJS_OBJECT dat_8c19afe8 = {
#include "animal/models/dat_8c19afe8.inc"
};
static NJS_OBJECT dat_8c19b01c = {
#include "animal/models/dat_8c19b01c.inc"
};
static NJS_OBJECT dat_8c19b050 = {
#include "animal/models/dat_8c19b050.inc"
};
static NJS_OBJECT dat_8c19b084 = {
#include "animal/models/dat_8c19b084.inc"
};
static NJS_MATERIAL dat_8c19b0b8[1] = {
#include "animal/models/dat_8c19b0b8.inc"
};
static Sint16 dat_8c19b0cc[12] = {
#include "animal/models/dat_8c19b0cc.inc"
};
static NJS_COLOR dat_8c19b0e4[10] = {
#include "animal/models/dat_8c19b0e4.inc"
};
static NJS_MESHSET dat_8c19b10c[1] = {
#include "animal/models/dat_8c19b10c.inc"
};
static NJS_POINT3 dat_8c19b124[5] = {
#include "animal/models/dat_8c19b124.inc"
};
static NJS_VECTOR dat_8c19b160[5] = {
#include "animal/models/dat_8c19b160.inc"
};
static NJS_MODEL dat_8c19b19c = {
#include "animal/models/dat_8c19b19c.inc"
};
static NJS_OBJECT dat_8c19b1c4 = {
#include "animal/models/dat_8c19b1c4.inc"
};
static NJS_OBJECT dat_8c19b1f8 = {
#include "animal/models/dat_8c19b1f8.inc"
};
static NJS_OBJECT dat_8c19b22c = {
#include "animal/models/dat_8c19b22c.inc"
};
static NJS_OBJECT dat_8c19b260 = {
#include "animal/models/dat_8c19b260.inc"
};
static NJS_MATERIAL dat_8c19b294[1] = {
#include "animal/models/dat_8c19b294.inc"
};
static Sint16 dat_8c19b2a8[12] = {
#include "animal/models/dat_8c19b2a8.inc"
};
static NJS_COLOR dat_8c19b2c0[10] = {
#include "animal/models/dat_8c19b2c0.inc"
};
static NJS_MESHSET dat_8c19b2e8[1] = {
#include "animal/models/dat_8c19b2e8.inc"
};
static NJS_POINT3 dat_8c19b300[5] = {
#include "animal/models/dat_8c19b300.inc"
};
static NJS_VECTOR dat_8c19b33c[5] = {
#include "animal/models/dat_8c19b33c.inc"
};
static NJS_MODEL dat_8c19b378 = {
#include "animal/models/dat_8c19b378.inc"
};
static NJS_OBJECT dat_8c19b3a0 = {
#include "animal/models/dat_8c19b3a0.inc"
};
static NJS_OBJECT dat_8c19b3d4 = {
#include "animal/models/dat_8c19b3d4.inc"
};
static NJS_OBJECT dat_8c19b408 = {
#include "animal/models/dat_8c19b408.inc"
};
static NJS_OBJECT dat_8c19b43c = {
#include "animal/models/dat_8c19b43c.inc"
};
static NJS_MATERIAL dat_8c19b470[1] = {
#include "animal/models/dat_8c19b470.inc"
};
static Sint16 dat_8c19b484[4] = {
#include "animal/models/dat_8c19b484.inc"
};
static NJS_COLOR dat_8c19b48c[3] = {
#include "animal/models/dat_8c19b48c.inc"
};
static NJS_MESHSET dat_8c19b498[1] = {
#include "animal/models/dat_8c19b498.inc"
};
static NJS_POINT3 dat_8c19b4b0[3] = {
#include "animal/models/dat_8c19b4b0.inc"
};
static NJS_VECTOR dat_8c19b4d4[3] = {
#include "animal/models/dat_8c19b4d4.inc"
};
static NJS_MODEL dat_8c19b4f8 = {
#include "animal/models/dat_8c19b4f8.inc"
};
static NJS_OBJECT dat_8c19b520 = {
#include "animal/models/dat_8c19b520.inc"
};
static NJS_MATERIAL dat_8c19b554[1] = {
#include "animal/models/dat_8c19b554.inc"
};
static Sint16 dat_8c19b568[4] = {
#include "animal/models/dat_8c19b568.inc"
};
static NJS_COLOR dat_8c19b570[3] = {
#include "animal/models/dat_8c19b570.inc"
};
static NJS_MESHSET dat_8c19b57c[1] = {
#include "animal/models/dat_8c19b57c.inc"
};
static NJS_POINT3 dat_8c19b594[3] = {
#include "animal/models/dat_8c19b594.inc"
};
static NJS_VECTOR dat_8c19b5b8[3] = {
#include "animal/models/dat_8c19b5b8.inc"
};
static NJS_MODEL dat_8c19b5dc = {
#include "animal/models/dat_8c19b5dc.inc"
};
static NJS_OBJECT dat_8c19b604 = {
#include "animal/models/dat_8c19b604.inc"
};
static NJS_MATERIAL dat_8c19b638[1] = {
#include "animal/models/dat_8c19b638.inc"
};
static Sint16 dat_8c19b64c[50] = {
#include "animal/models/dat_8c19b64c.inc"
};
static NJS_COLOR dat_8c19b6b0[42] = {
#include "animal/models/dat_8c19b6b0.inc"
};
static NJS_MESHSET dat_8c19b758[1] = {
#include "animal/models/dat_8c19b758.inc"
};
static NJS_POINT3 dat_8c19b770[15] = {
#include "animal/models/dat_8c19b770.inc"
};
static NJS_VECTOR dat_8c19b824[15] = {
#include "animal/models/dat_8c19b824.inc"
};
static NJS_MODEL dat_8c19b8d8 = {
#include "animal/models/dat_8c19b8d8.inc"
};
static NJS_OBJECT dat_8c19b900 = {
#include "animal/models/dat_8c19b900.inc"
};
static NJS_OBJECT dat_8c19b934 = {
#include "animal/models/dat_8c19b934.inc"
};
static NJS_OBJECT dat_8c19b968 = {
#include "animal/models/dat_8c19b968.inc"
};
static NJS_OBJECT dat_8c19b99c = {
#include "animal/models/dat_8c19b99c.inc"
};
static NJS_MATERIAL dat_8c19b9d0[1] = {
#include "animal/models/dat_8c19b9d0.inc"
};
static Sint16 dat_8c19b9e4[44] = {
#include "animal/models/dat_8c19b9e4.inc"
};
static NJS_COLOR dat_8c19ba3c[38] = {
#include "animal/models/dat_8c19ba3c.inc"
};
static NJS_MESHSET dat_8c19bad4[1] = {
#include "animal/models/dat_8c19bad4.inc"
};
static NJS_POINT3 dat_8c19baec[15] = {
#include "animal/models/dat_8c19baec.inc"
};
static NJS_VECTOR dat_8c19bba0[15] = {
#include "animal/models/dat_8c19bba0.inc"
};
static NJS_MODEL dat_8c19bc54 = {
#include "animal/models/dat_8c19bc54.inc"
};
static NJS_OBJECT dat_8c19bc7c = {
#include "animal/models/dat_8c19bc7c.inc"
};
static NJS_OBJECT dat_8c19bcb0 = {
#include "animal/models/dat_8c19bcb0.inc"
};
static NJS_OBJECT dat_8c19bce4 = {
#include "animal/models/dat_8c19bce4.inc"
};
static NJS_OBJECT dat_8c19bd18 = {
#include "animal/models/dat_8c19bd18.inc"
};
static NJS_OBJECT suka_object = {
#include "animal/models/suka_object.inc"
};
static NJS_MKEY_F dat_8c19bd80[2] = {
#include "animal/models/dat_8c19bd80.inc"
};
static NJS_MKEY_A dat_8c19bda0[2] = {
#include "animal/models/dat_8c19bda0.inc"
};
static NJS_MKEY_F dat_8c19bdc0[17] = {
#include "animal/models/dat_8c19bdc0.inc"
};
static NJS_MKEY_A dat_8c19bed0[2] = {
#include "animal/models/dat_8c19bed0.inc"
};
static NJS_MKEY_F dat_8c19bef0[2] = {
#include "animal/models/dat_8c19bef0.inc"
};
static NJS_MKEY_A dat_8c19bf10[17] = {
#include "animal/models/dat_8c19bf10.inc"
};
static NJS_MKEY_F dat_8c19c020[2] = {
#include "animal/models/dat_8c19c020.inc"
};
static NJS_MKEY_A dat_8c19c040[2] = {
#include "animal/models/dat_8c19c040.inc"
};
static NJS_MKEY_F dat_8c19c060[2] = {
#include "animal/models/dat_8c19c060.inc"
};
static NJS_MKEY_A dat_8c19c080[2] = {
#include "animal/models/dat_8c19c080.inc"
};
static NJS_MKEY_F dat_8c19c0a0[2] = {
#include "animal/models/dat_8c19c0a0.inc"
};
static NJS_MKEY_A dat_8c19c0c0[2] = {
#include "animal/models/dat_8c19c0c0.inc"
};
static NJS_MKEY_F dat_8c19c0e0[2] = {
#include "animal/models/dat_8c19c0e0.inc"
};
static NJS_MKEY_A dat_8c19c100[2] = {
#include "animal/models/dat_8c19c100.inc"
};
static NJS_MKEY_F dat_8c19c120[2] = {
#include "animal/models/dat_8c19c120.inc"
};
static NJS_MKEY_A dat_8c19c140[2] = {
#include "animal/models/dat_8c19c140.inc"
};
static NJS_MKEY_F dat_8c19c160[2] = {
#include "animal/models/dat_8c19c160.inc"
};
static NJS_MKEY_A dat_8c19c180[2] = {
#include "animal/models/dat_8c19c180.inc"
};
static NJS_MKEY_F dat_8c19c1a0[2] = {
#include "animal/models/dat_8c19c1a0.inc"
};
static NJS_MKEY_A dat_8c19c1c0[2] = {
#include "animal/models/dat_8c19c1c0.inc"
};
static NJS_MKEY_F dat_8c19c1e0[2] = {
#include "animal/models/dat_8c19c1e0.inc"
};
static NJS_MKEY_A dat_8c19c200[2] = {
#include "animal/models/dat_8c19c200.inc"
};
static NJS_MKEY_F dat_8c19c220[2] = {
#include "animal/models/dat_8c19c220.inc"
};
static NJS_MKEY_A dat_8c19c240[2] = {
#include "animal/models/dat_8c19c240.inc"
};
static NJS_MKEY_F dat_8c19c260[2] = {
#include "animal/models/dat_8c19c260.inc"
};
static NJS_MKEY_A dat_8c19c280[17] = {
#include "animal/models/dat_8c19c280.inc"
};
static NJS_MKEY_F dat_8c19c390[2] = {
#include "animal/models/dat_8c19c390.inc"
};
static NJS_MKEY_A dat_8c19c3b0[2] = {
#include "animal/models/dat_8c19c3b0.inc"
};
static NJS_MKEY_F dat_8c19c3d0[2] = {
#include "animal/models/dat_8c19c3d0.inc"
};
static NJS_MKEY_A dat_8c19c3f0[2] = {
#include "animal/models/dat_8c19c3f0.inc"
};
static NJS_MKEY_F dat_8c19c410[2] = {
#include "animal/models/dat_8c19c410.inc"
};
static NJS_MKEY_A dat_8c19c430[2] = {
#include "animal/models/dat_8c19c430.inc"
};
static NJS_MKEY_F dat_8c19c450[2] = {
#include "animal/models/dat_8c19c450.inc"
};
static NJS_MKEY_A dat_8c19c470[17] = {
#include "animal/models/dat_8c19c470.inc"
};
static NJS_MKEY_F dat_8c19c580[2] = {
#include "animal/models/dat_8c19c580.inc"
};
static NJS_MKEY_A dat_8c19c5a0[2] = {
#include "animal/models/dat_8c19c5a0.inc"
};
static NJS_MKEY_F dat_8c19c5c0[2] = {
#include "animal/models/dat_8c19c5c0.inc"
};
static NJS_MKEY_A dat_8c19c5e0[2] = {
#include "animal/models/dat_8c19c5e0.inc"
};
static NJS_MKEY_F dat_8c19c600[17] = {
#include "animal/models/dat_8c19c600.inc"
};
static NJS_MKEY_A dat_8c19c710[2] = {
#include "animal/models/dat_8c19c710.inc"
};
static NJS_MKEY_F dat_8c19c730[2] = {
#include "animal/models/dat_8c19c730.inc"
};
static NJS_MKEY_A dat_8c19c750[17] = {
#include "animal/models/dat_8c19c750.inc"
};
static NJS_MKEY_F dat_8c19c860[2] = {
#include "animal/models/dat_8c19c860.inc"
};
static NJS_MKEY_A dat_8c19c880[2] = {
#include "animal/models/dat_8c19c880.inc"
};
static NJS_MKEY_F dat_8c19c8a0[2] = {
#include "animal/models/dat_8c19c8a0.inc"
};
static NJS_MKEY_A dat_8c19c8c0[2] = {
#include "animal/models/dat_8c19c8c0.inc"
};
static NJS_MKEY_F dat_8c19c8e0[17] = {
#include "animal/models/dat_8c19c8e0.inc"
};
static NJS_MKEY_A dat_8c19c9f0[2] = {
#include "animal/models/dat_8c19c9f0.inc"
};
static NJS_MKEY_F dat_8c19ca10[2] = {
#include "animal/models/dat_8c19ca10.inc"
};
static NJS_MKEY_A dat_8c19ca30[17] = {
#include "animal/models/dat_8c19ca30.inc"
};
static NJS_MKEY_F dat_8c19cb40[2] = {
#include "animal/models/dat_8c19cb40.inc"
};
static NJS_MKEY_A dat_8c19cb60[2] = {
#include "animal/models/dat_8c19cb60.inc"
};
static NJS_MKEY_F dat_8c19cb80[2] = {
#include "animal/models/dat_8c19cb80.inc"
};
static NJS_MKEY_A dat_8c19cba0[2] = {
#include "animal/models/dat_8c19cba0.inc"
};
static NJS_MKEY_F dat_8c19cbc0[2] = {
#include "animal/models/dat_8c19cbc0.inc"
};
static NJS_MKEY_A dat_8c19cbe0[2] = {
#include "animal/models/dat_8c19cbe0.inc"
};
static NJS_MKEY_F dat_8c19cc00[2] = {
#include "animal/models/dat_8c19cc00.inc"
};
static NJS_MKEY_A dat_8c19cc20[17] = {
#include "animal/models/dat_8c19cc20.inc"
};
static NJS_MKEY_F dat_8c19cd30[2] = {
#include "animal/models/dat_8c19cd30.inc"
};
static NJS_MKEY_A dat_8c19cd50[2] = {
#include "animal/models/dat_8c19cd50.inc"
};
static NJS_MKEY_F dat_8c19cd70[2] = {
#include "animal/models/dat_8c19cd70.inc"
};
static NJS_MKEY_A dat_8c19cd90[2] = {
#include "animal/models/dat_8c19cd90.inc"
};
static NJS_MDATA2 dat_8c19cdb0[31] = {
#include "animal/models/dat_8c19cdb0.inc"
};
static NJS_MOTION suka_motion = {
#include "animal/models/suka_motion.inc"
};
NJS_ACTION suka_action = {
#include "animal/models/suka_action.inc"
};
static NJS_TEXNAME suka_textures[4] = {
    {TEXT_55, 0, 0},
    {TEXT_56, 0, 0},
    {TEXT_57, 0, 0},
    {TEXT_58, 0, 0},
};
NJS_TEXLIST suka_texlist = {
#include "animal/models/suka_texlist.inc"
};
static NJS_MATERIAL dat_8c19cfec[1] = {
#include "animal/models/dat_8c19cfec.inc"
};
static Sint16 dat_8c19d000[5] = {
#include "animal/models/dat_8c19d000.inc"
};
static NJS_COLOR dat_8c19d00c[4] = {
#include "animal/models/dat_8c19d00c.inc"
};
static NJS_MESHSET dat_8c19d01c[1] = {
#include "animal/models/dat_8c19d01c.inc"
};
static NJS_POINT3 dat_8c19d034[4] = {
#include "animal/models/dat_8c19d034.inc"
};
static NJS_VECTOR dat_8c19d064[4] = {
#include "animal/models/dat_8c19d064.inc"
};
static NJS_MODEL dat_8c19d094 = {
#include "animal/models/dat_8c19d094.inc"
};
static NJS_OBJECT dat_8c19d0bc = {
#include "animal/models/dat_8c19d0bc.inc"
};
static NJS_MATERIAL dat_8c19d0f0[1] = {
#include "animal/models/dat_8c19d0f0.inc"
};
static Sint16 dat_8c19d104[4] = {
#include "animal/models/dat_8c19d104.inc"
};
static NJS_COLOR dat_8c19d10c[3] = {
#include "animal/models/dat_8c19d10c.inc"
};
static NJS_MESHSET dat_8c19d118[1] = {
#include "animal/models/dat_8c19d118.inc"
};
static NJS_POINT3 dat_8c19d130[3] = {
#include "animal/models/dat_8c19d130.inc"
};
static NJS_VECTOR dat_8c19d154[3] = {
#include "animal/models/dat_8c19d154.inc"
};
static NJS_MODEL dat_8c19d178 = {
#include "animal/models/dat_8c19d178.inc"
};
static NJS_OBJECT dat_8c19d1a0 = {
#include "animal/models/dat_8c19d1a0.inc"
};
static NJS_OBJECT dat_8c19d1d4 = {
#include "animal/models/dat_8c19d1d4.inc"
};
static NJS_OBJECT dat_8c19d208 = {
#include "animal/models/dat_8c19d208.inc"
};
static NJS_OBJECT dat_8c19d23c = {
#include "animal/models/dat_8c19d23c.inc"
};
static NJS_MATERIAL dat_8c19d270[1] = {
#include "animal/models/dat_8c19d270.inc"
};
static Sint16 dat_8c19d284[4] = {
#include "animal/models/dat_8c19d284.inc"
};
static NJS_COLOR dat_8c19d28c[3] = {
#include "animal/models/dat_8c19d28c.inc"
};
static NJS_MESHSET dat_8c19d298[1] = {
#include "animal/models/dat_8c19d298.inc"
};
static NJS_POINT3 dat_8c19d2b0[3] = {
#include "animal/models/dat_8c19d2b0.inc"
};
static NJS_VECTOR dat_8c19d2d4[3] = {
#include "animal/models/dat_8c19d2d4.inc"
};
static NJS_MODEL dat_8c19d2f8 = {
#include "animal/models/dat_8c19d2f8.inc"
};
static NJS_OBJECT dat_8c19d320 = {
#include "animal/models/dat_8c19d320.inc"
};
static NJS_OBJECT dat_8c19d354 = {
#include "animal/models/dat_8c19d354.inc"
};
static NJS_OBJECT dat_8c19d388 = {
#include "animal/models/dat_8c19d388.inc"
};
static NJS_OBJECT dat_8c19d3bc = {
#include "animal/models/dat_8c19d3bc.inc"
};
static NJS_MATERIAL dat_8c19d3f0[1] = {
#include "animal/models/dat_8c19d3f0.inc"
};
static Sint16 dat_8c19d404[5] = {
#include "animal/models/dat_8c19d404.inc"
};
static NJS_COLOR dat_8c19d410[4] = {
#include "animal/models/dat_8c19d410.inc"
};
static NJS_MESHSET dat_8c19d420[1] = {
#include "animal/models/dat_8c19d420.inc"
};
static NJS_POINT3 dat_8c19d438[4] = {
#include "animal/models/dat_8c19d438.inc"
};
static NJS_VECTOR dat_8c19d468[4] = {
#include "animal/models/dat_8c19d468.inc"
};
static NJS_MODEL dat_8c19d498 = {
#include "animal/models/dat_8c19d498.inc"
};
static NJS_OBJECT dat_8c19d4c0 = {
#include "animal/models/dat_8c19d4c0.inc"
};
static NJS_MATERIAL dat_8c19d4f4[1] = {
#include "animal/models/dat_8c19d4f4.inc"
};
static Sint16 dat_8c19d508[53] = {
#include "animal/models/dat_8c19d508.inc"
};
static NJS_COLOR dat_8c19d574[46] = {
#include "animal/models/dat_8c19d574.inc"
};
static NJS_MESHSET dat_8c19d62c[1] = {
#include "animal/models/dat_8c19d62c.inc"
};
static NJS_POINT3 dat_8c19d644[18] = {
#include "animal/models/dat_8c19d644.inc"
};
static NJS_VECTOR dat_8c19d71c[18] = {
#include "animal/models/dat_8c19d71c.inc"
};
static NJS_MODEL dat_8c19d7f4 = {
#include "animal/models/dat_8c19d7f4.inc"
};
static NJS_OBJECT dat_8c19d81c = {
#include "animal/models/dat_8c19d81c.inc"
};
static NJS_OBJECT dat_8c19d850 = {
#include "animal/models/dat_8c19d850.inc"
};
static NJS_OBJECT dat_8c19d884 = {
#include "animal/models/dat_8c19d884.inc"
};
static NJS_OBJECT dat_8c19d8b8 = {
#include "animal/models/dat_8c19d8b8.inc"
};
static NJS_MATERIAL dat_8c19d8ec[1] = {
#include "animal/models/dat_8c19d8ec.inc"
};
static Sint16 dat_8c19d900[22] = {
#include "animal/models/dat_8c19d900.inc"
};
static NJS_COLOR dat_8c19d92c[18] = {
#include "animal/models/dat_8c19d92c.inc"
};
static NJS_MESHSET dat_8c19d974[1] = {
#include "animal/models/dat_8c19d974.inc"
};
static NJS_POINT3 dat_8c19d98c[7] = {
#include "animal/models/dat_8c19d98c.inc"
};
static NJS_VECTOR dat_8c19d9e0[7] = {
#include "animal/models/dat_8c19d9e0.inc"
};
static NJS_MODEL dat_8c19da34 = {
#include "animal/models/dat_8c19da34.inc"
};
static NJS_OBJECT dat_8c19da5c = {
#include "animal/models/dat_8c19da5c.inc"
};
static NJS_OBJECT dat_8c19da90 = {
#include "animal/models/dat_8c19da90.inc"
};
static NJS_OBJECT dat_8c19dac4 = {
#include "animal/models/dat_8c19dac4.inc"
};
static NJS_OBJECT dat_8c19daf8 = {
#include "animal/models/dat_8c19daf8.inc"
};
static NJS_MATERIAL dat_8c19db2c[1] = {
#include "animal/models/dat_8c19db2c.inc"
};
static Sint16 dat_8c19db40[22] = {
#include "animal/models/dat_8c19db40.inc"
};
static NJS_COLOR dat_8c19db6c[18] = {
#include "animal/models/dat_8c19db6c.inc"
};
static NJS_MESHSET dat_8c19dbb4[1] = {
#include "animal/models/dat_8c19dbb4.inc"
};
static NJS_POINT3 dat_8c19dbcc[7] = {
#include "animal/models/dat_8c19dbcc.inc"
};
static NJS_VECTOR dat_8c19dc20[7] = {
#include "animal/models/dat_8c19dc20.inc"
};
static NJS_MODEL dat_8c19dc74 = {
#include "animal/models/dat_8c19dc74.inc"
};
static NJS_OBJECT dat_8c19dc9c = {
#include "animal/models/dat_8c19dc9c.inc"
};
static NJS_MATERIAL dat_8c19dcd0[1] = {
#include "animal/models/dat_8c19dcd0.inc"
};
static Sint16 dat_8c19dce4[4] = {
#include "animal/models/dat_8c19dce4.inc"
};
static NJS_COLOR dat_8c19dcec[3] = {
#include "animal/models/dat_8c19dcec.inc"
};
static NJS_MESHSET dat_8c19dcf8[1] = {
#include "animal/models/dat_8c19dcf8.inc"
};
static NJS_POINT3 dat_8c19dd10[3] = {
#include "animal/models/dat_8c19dd10.inc"
};
static NJS_VECTOR dat_8c19dd34[3] = {
#include "animal/models/dat_8c19dd34.inc"
};
static NJS_MODEL dat_8c19dd58 = {
#include "animal/models/dat_8c19dd58.inc"
};
static NJS_OBJECT dat_8c19dd80 = {
#include "animal/models/dat_8c19dd80.inc"
};
static NJS_OBJECT dat_8c19ddb4 = {
#include "animal/models/dat_8c19ddb4.inc"
};
static NJS_OBJECT dat_8c19dde8 = {
#include "animal/models/dat_8c19dde8.inc"
};
static NJS_OBJECT dat_8c19de1c = {
#include "animal/models/dat_8c19de1c.inc"
};
static NJS_MATERIAL dat_8c19de50[1] = {
#include "animal/models/dat_8c19de50.inc"
};
static Sint16 dat_8c19de64[14] = {
#include "animal/models/dat_8c19de64.inc"
};
static NJS_COLOR dat_8c19de80[11] = {
#include "animal/models/dat_8c19de80.inc"
};
static NJS_MESHSET dat_8c19deac[1] = {
#include "animal/models/dat_8c19deac.inc"
};
static NJS_POINT3 dat_8c19dec4[7] = {
#include "animal/models/dat_8c19dec4.inc"
};
static NJS_VECTOR dat_8c19df18[7] = {
#include "animal/models/dat_8c19df18.inc"
};
static NJS_MODEL dat_8c19df6c = {
#include "animal/models/dat_8c19df6c.inc"
};
static NJS_OBJECT dat_8c19df94 = {
#include "animal/models/dat_8c19df94.inc"
};
static NJS_OBJECT dat_8c19dfc8 = {
#include "animal/models/dat_8c19dfc8.inc"
};
static NJS_OBJECT dat_8c19dffc = {
#include "animal/models/dat_8c19dffc.inc"
};
static NJS_OBJECT dat_8c19e030 = {
#include "animal/models/dat_8c19e030.inc"
};
static NJS_MATERIAL dat_8c19e064[1] = {
#include "animal/models/dat_8c19e064.inc"
};
static Sint16 dat_8c19e078[14] = {
#include "animal/models/dat_8c19e078.inc"
};
static NJS_COLOR dat_8c19e094[11] = {
#include "animal/models/dat_8c19e094.inc"
};
static NJS_MESHSET dat_8c19e0c0[1] = {
#include "animal/models/dat_8c19e0c0.inc"
};
static NJS_POINT3 dat_8c19e0d8[7] = {
#include "animal/models/dat_8c19e0d8.inc"
};
static NJS_VECTOR dat_8c19e12c[7] = {
#include "animal/models/dat_8c19e12c.inc"
};
static NJS_MODEL dat_8c19e180 = {
#include "animal/models/dat_8c19e180.inc"
};
static NJS_OBJECT dat_8c19e1a8 = {
#include "animal/models/dat_8c19e1a8.inc"
};
static NJS_OBJECT dat_8c19e1dc = {
#include "animal/models/dat_8c19e1dc.inc"
};
static NJS_OBJECT dat_8c19e210 = {
#include "animal/models/dat_8c19e210.inc"
};
static NJS_OBJECT dat_8c19e244 = {
#include "animal/models/dat_8c19e244.inc"
};
static NJS_MATERIAL dat_8c19e278[1] = {
#include "animal/models/dat_8c19e278.inc"
};
static Sint16 dat_8c19e28c[9] = {
#include "animal/models/dat_8c19e28c.inc"
};
static NJS_COLOR dat_8c19e2a0[8] = {
#include "animal/models/dat_8c19e2a0.inc"
};
static NJS_MESHSET dat_8c19e2c0[1] = {
#include "animal/models/dat_8c19e2c0.inc"
};
static NJS_POINT3 dat_8c19e2d8[5] = {
#include "animal/models/dat_8c19e2d8.inc"
};
static NJS_VECTOR dat_8c19e314[5] = {
#include "animal/models/dat_8c19e314.inc"
};
static NJS_MODEL dat_8c19e350 = {
#include "animal/models/dat_8c19e350.inc"
};
static NJS_OBJECT dat_8c19e378 = {
#include "animal/models/dat_8c19e378.inc"
};
static NJS_OBJECT dat_8c19e3ac = {
#include "animal/models/dat_8c19e3ac.inc"
};
static NJS_OBJECT dat_8c19e3e0 = {
#include "animal/models/dat_8c19e3e0.inc"
};
static NJS_OBJECT dat_8c19e414 = {
#include "animal/models/dat_8c19e414.inc"
};
static NJS_MATERIAL dat_8c19e448[1] = {
#include "animal/models/dat_8c19e448.inc"
};
static Sint16 dat_8c19e45c[40] = {
#include "animal/models/dat_8c19e45c.inc"
};
static NJS_COLOR dat_8c19e4ac[35] = {
#include "animal/models/dat_8c19e4ac.inc"
};
static NJS_MESHSET dat_8c19e538[1] = {
#include "animal/models/dat_8c19e538.inc"
};
static NJS_POINT3 dat_8c19e550[15] = {
#include "animal/models/dat_8c19e550.inc"
};
static NJS_VECTOR dat_8c19e604[15] = {
#include "animal/models/dat_8c19e604.inc"
};
static NJS_MODEL dat_8c19e6b8 = {
#include "animal/models/dat_8c19e6b8.inc"
};
static NJS_OBJECT dat_8c19e6e0 = {
#include "animal/models/dat_8c19e6e0.inc"
};
static NJS_OBJECT dat_8c19e714 = {
#include "animal/models/dat_8c19e714.inc"
};
static NJS_OBJECT dat_8c19e748 = {
#include "animal/models/dat_8c19e748.inc"
};
static NJS_OBJECT dat_8c19e77c = {
#include "animal/models/dat_8c19e77c.inc"
};
static NJS_OBJECT rako_object = {
#include "animal/models/rako_object.inc"
};
static NJS_MKEY_F dat_8c19e7e4[2] = {
#include "animal/models/dat_8c19e7e4.inc"
};
static NJS_MKEY_A dat_8c19e804[2] = {
#include "animal/models/dat_8c19e804.inc"
};
static NJS_MKEY_F dat_8c19e824[17] = {
#include "animal/models/dat_8c19e824.inc"
};
static NJS_MKEY_A dat_8c19e934[2] = {
#include "animal/models/dat_8c19e934.inc"
};
static NJS_MKEY_F dat_8c19e954[2] = {
#include "animal/models/dat_8c19e954.inc"
};
static NJS_MKEY_A dat_8c19e974[17] = {
#include "animal/models/dat_8c19e974.inc"
};
static NJS_MKEY_F dat_8c19ea84[2] = {
#include "animal/models/dat_8c19ea84.inc"
};
static NJS_MKEY_A dat_8c19eaa4[2] = {
#include "animal/models/dat_8c19eaa4.inc"
};
static NJS_MKEY_F dat_8c19eac4[2] = {
#include "animal/models/dat_8c19eac4.inc"
};
static NJS_MKEY_A dat_8c19eae4[2] = {
#include "animal/models/dat_8c19eae4.inc"
};
static NJS_MKEY_F dat_8c19eb04[2] = {
#include "animal/models/dat_8c19eb04.inc"
};
static NJS_MKEY_A dat_8c19eb24[2] = {
#include "animal/models/dat_8c19eb24.inc"
};
static NJS_MKEY_F dat_8c19eb44[2] = {
#include "animal/models/dat_8c19eb44.inc"
};
static NJS_MKEY_A dat_8c19eb64[17] = {
#include "animal/models/dat_8c19eb64.inc"
};
static NJS_MKEY_F dat_8c19ec74[2] = {
#include "animal/models/dat_8c19ec74.inc"
};
static NJS_MKEY_A dat_8c19ec94[2] = {
#include "animal/models/dat_8c19ec94.inc"
};
static NJS_MKEY_F dat_8c19ecb4[2] = {
#include "animal/models/dat_8c19ecb4.inc"
};
static NJS_MKEY_A dat_8c19ecd4[2] = {
#include "animal/models/dat_8c19ecd4.inc"
};
static NJS_MKEY_F dat_8c19ecf4[17] = {
#include "animal/models/dat_8c19ecf4.inc"
};
static NJS_MKEY_A dat_8c19ee04[2] = {
#include "animal/models/dat_8c19ee04.inc"
};
static NJS_MKEY_F dat_8c19ee24[2] = {
#include "animal/models/dat_8c19ee24.inc"
};
static NJS_MKEY_A dat_8c19ee44[17] = {
#include "animal/models/dat_8c19ee44.inc"
};
static NJS_MKEY_F dat_8c19ef54[2] = {
#include "animal/models/dat_8c19ef54.inc"
};
static NJS_MKEY_A dat_8c19ef74[2] = {
#include "animal/models/dat_8c19ef74.inc"
};
static NJS_MKEY_F dat_8c19ef94[2] = {
#include "animal/models/dat_8c19ef94.inc"
};
static NJS_MKEY_A dat_8c19efb4[2] = {
#include "animal/models/dat_8c19efb4.inc"
};
static NJS_MKEY_F dat_8c19efd4[17] = {
#include "animal/models/dat_8c19efd4.inc"
};
static NJS_MKEY_A dat_8c19f0e4[2] = {
#include "animal/models/dat_8c19f0e4.inc"
};
static NJS_MKEY_F dat_8c19f104[2] = {
#include "animal/models/dat_8c19f104.inc"
};
static NJS_MKEY_A dat_8c19f124[17] = {
#include "animal/models/dat_8c19f124.inc"
};
static NJS_MKEY_F dat_8c19f234[2] = {
#include "animal/models/dat_8c19f234.inc"
};
static NJS_MKEY_A dat_8c19f254[2] = {
#include "animal/models/dat_8c19f254.inc"
};
static NJS_MKEY_F dat_8c19f274[2] = {
#include "animal/models/dat_8c19f274.inc"
};
static NJS_MKEY_A dat_8c19f294[2] = {
#include "animal/models/dat_8c19f294.inc"
};
static NJS_MKEY_F dat_8c19f2b4[2] = {
#include "animal/models/dat_8c19f2b4.inc"
};
static NJS_MKEY_A dat_8c19f2d4[2] = {
#include "animal/models/dat_8c19f2d4.inc"
};
static NJS_MKEY_F dat_8c19f2f4[2] = {
#include "animal/models/dat_8c19f2f4.inc"
};
static NJS_MKEY_A dat_8c19f314[17] = {
#include "animal/models/dat_8c19f314.inc"
};
static NJS_MKEY_F dat_8c19f424[2] = {
#include "animal/models/dat_8c19f424.inc"
};
static NJS_MKEY_A dat_8c19f444[2] = {
#include "animal/models/dat_8c19f444.inc"
};
static NJS_MKEY_F dat_8c19f464[2] = {
#include "animal/models/dat_8c19f464.inc"
};
static NJS_MKEY_A dat_8c19f484[2] = {
#include "animal/models/dat_8c19f484.inc"
};
static NJS_MKEY_F dat_8c19f4a4[2] = {
#include "animal/models/dat_8c19f4a4.inc"
};
static NJS_MKEY_A dat_8c19f4c4[2] = {
#include "animal/models/dat_8c19f4c4.inc"
};
static NJS_MKEY_F dat_8c19f4e4[2] = {
#include "animal/models/dat_8c19f4e4.inc"
};
static NJS_MKEY_A dat_8c19f504[2] = {
#include "animal/models/dat_8c19f504.inc"
};
static NJS_MKEY_F dat_8c19f524[2] = {
#include "animal/models/dat_8c19f524.inc"
};
static NJS_MKEY_A dat_8c19f544[17] = {
#include "animal/models/dat_8c19f544.inc"
};
static NJS_MKEY_F dat_8c19f654[2] = {
#include "animal/models/dat_8c19f654.inc"
};
static NJS_MKEY_A dat_8c19f674[2] = {
#include "animal/models/dat_8c19f674.inc"
};
static NJS_MKEY_F dat_8c19f694[2] = {
#include "animal/models/dat_8c19f694.inc"
};
static NJS_MKEY_A dat_8c19f6b4[2] = {
#include "animal/models/dat_8c19f6b4.inc"
};
static NJS_MKEY_F dat_8c19f6d4[17] = {
#include "animal/models/dat_8c19f6d4.inc"
};
static NJS_MKEY_A dat_8c19f7e4[2] = {
#include "animal/models/dat_8c19f7e4.inc"
};
static NJS_MKEY_F dat_8c19f804[2] = {
#include "animal/models/dat_8c19f804.inc"
};
static NJS_MKEY_A dat_8c19f824[2] = {
#include "animal/models/dat_8c19f824.inc"
};
static NJS_MKEY_F dat_8c19f844[2] = {
#include "animal/models/dat_8c19f844.inc"
};
static NJS_MKEY_A dat_8c19f864[2] = {
#include "animal/models/dat_8c19f864.inc"
};
static NJS_MKEY_F dat_8c19f884[2] = {
#include "animal/models/dat_8c19f884.inc"
};
static NJS_MKEY_A dat_8c19f8a4[2] = {
#include "animal/models/dat_8c19f8a4.inc"
};
static NJS_MKEY_F dat_8c19f8c4[2] = {
#include "animal/models/dat_8c19f8c4.inc"
};
static NJS_MKEY_A dat_8c19f8e4[2] = {
#include "animal/models/dat_8c19f8e4.inc"
};
static NJS_MKEY_F dat_8c19f904[2] = {
#include "animal/models/dat_8c19f904.inc"
};
static NJS_MKEY_A dat_8c19f924[2] = {
#include "animal/models/dat_8c19f924.inc"
};
static NJS_MKEY_F dat_8c19f944[2] = {
#include "animal/models/dat_8c19f944.inc"
};
static NJS_MKEY_A dat_8c19f964[17] = {
#include "animal/models/dat_8c19f964.inc"
};
static NJS_MKEY_F dat_8c19fa74[2] = {
#include "animal/models/dat_8c19fa74.inc"
};
static NJS_MKEY_A dat_8c19fa94[2] = {
#include "animal/models/dat_8c19fa94.inc"
};
static NJS_MKEY_F dat_8c19fab4[2] = {
#include "animal/models/dat_8c19fab4.inc"
};
static NJS_MKEY_A dat_8c19fad4[2] = {
#include "animal/models/dat_8c19fad4.inc"
};
static NJS_MKEY_F dat_8c19faf4[2] = {
#include "animal/models/dat_8c19faf4.inc"
};
static NJS_MKEY_A dat_8c19fb14[2] = {
#include "animal/models/dat_8c19fb14.inc"
};
static NJS_MKEY_F dat_8c19fb34[2] = {
#include "animal/models/dat_8c19fb34.inc"
};
static NJS_MKEY_A dat_8c19fb54[17] = {
#include "animal/models/dat_8c19fb54.inc"
};
static NJS_MKEY_F dat_8c19fc64[2] = {
#include "animal/models/dat_8c19fc64.inc"
};
static NJS_MKEY_A dat_8c19fc84[2] = {
#include "animal/models/dat_8c19fc84.inc"
};
static NJS_MKEY_F dat_8c19fca4[2] = {
#include "animal/models/dat_8c19fca4.inc"
};
static NJS_MKEY_A dat_8c19fcc4[2] = {
#include "animal/models/dat_8c19fcc4.inc"
};
static NJS_MKEY_F dat_8c19fce4[2] = {
#include "animal/models/dat_8c19fce4.inc"
};
static NJS_MKEY_A dat_8c19fd04[2] = {
#include "animal/models/dat_8c19fd04.inc"
};
static NJS_MDATA2 dat_8c19fd24[40] = {
#include "animal/models/dat_8c19fd24.inc"
};
static NJS_MOTION rako_motion = {
#include "animal/models/rako_motion.inc"
};
NJS_ACTION rako_action = {
#include "animal/models/rako_action.inc"
};
static NJS_TEXNAME rako_textures[4] = {
    {TEXT_59, 0, 0},
    {TEXT_60, 0, 0},
    {TEXT_61, 0, 0},
    {TEXT_62, 0, 0},
};
NJS_TEXLIST rako_texlist = {
#include "animal/models/rako_texlist.inc"
};
static NJS_TEXNAME al_barria_textures[1] = {
    {TEXT_63, 0, 0},
};
NJS_TEXLIST al_barria_texlist = {
#include "animal/models/al_barria_texlist.inc"
};
static NJS_MATERIAL dat_8c1a0004[1] = {
#include "animal/models/dat_8c1a0004.inc"
};
static Sint16 dat_8c1a0018[338] = {
#include "animal/models/dat_8c1a0018.inc"
};
static NJS_COLOR dat_8c1a02bc[300] = {
#include "animal/models/dat_8c1a02bc.inc"
};
static NJS_MESHSET dat_8c1a076c[1] = {
#include "animal/models/dat_8c1a076c.inc"
};
static NJS_POINT3 dat_8c1a0784[114] = {
#include "animal/models/dat_8c1a0784.inc"
};
static NJS_VECTOR dat_8c1a0cdc[114] = {
#include "animal/models/dat_8c1a0cdc.inc"
};
static NJS_MODEL dat_8c1a1234 = {
#include "animal/models/dat_8c1a1234.inc"
};
NJS_OBJECT al_barria_object = {
#include "animal/models/al_barria_object.inc"
};
