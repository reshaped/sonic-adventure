/* math.src's vector and point math, angle lookups, color conversion and
 * memory fills, on SH4ZAM. make dc compiles this in place of
 * src/system/math.src. each FSCA and FSRRA comes from SH4ZAM, and each FIPR
 * from ninja_zam.h with its operands in math.src's order. where math.src's
 * FIPR multiplies a register its caller left by a zero, this one multiplies
 * 0 by 0. an infinity or a NaN left in that register makes math.src's result
 * a NaN, but not this one's. the product and sum of each other step are
 * math.src's, in its order */
#include <ninja.h>

#include "system/math.h"

/* math.h's macros that run these functions' helpers inline. this file
 * defines the functions from the helpers */
#undef vector_inner_product_across
#undef vector_magnitude_across
#undef vector_unit_across
#undef point_calc_xy

#define QUARTER 0x4000
#define HALF 0x8000

/* each table ends with 9, the NOP past its last entry that aligns the
 * image's code after it. a lookup of the last entry reads the NOP too, and
 * scales it by a fraction of 0. math.src keeps the tables in its code. here
 * each links after the image's other data */
/* the angle whose tangent is n/512, for n from 0 to 512 */
LINKED_LAST_DATA(atan_table)
static const Sint16 atan_table[514] = {
    0,    20,   40,   61,   81,   101,  122,  142,  162,  183,  203,  224,
    244,  264,  285,  305,  325,  346,  366,  386,  407,  427,  447,  468,
    488,  508,  529,  549,  569,  590,  610,  630,  651,  671,  691,  711,
    732,  752,  772,  792,  813,  833,  853,  873,  894,  914,  934,  954,
    974,  995,  1015, 1035, 1055, 1075, 1096, 1116, 1136, 1156, 1176, 1196,
    1216, 1236, 1256, 1277, 1297, 1317, 1337, 1357, 1377, 1397, 1417, 1437,
    1457, 1477, 1497, 1517, 1537, 1556, 1576, 1596, 1616, 1636, 1656, 1676,
    1696, 1715, 1735, 1755, 1775, 1795, 1814, 1834, 1854, 1874, 1893, 1913,
    1933, 1952, 1972, 1992, 2011, 2031, 2051, 2070, 2090, 2109, 2129, 2148,
    2168, 2187, 2207, 2226, 2246, 2265, 2285, 2304, 2323, 2343, 2362, 2381,
    2401, 2420, 2439, 2459, 2478, 2497, 2516, 2536, 2555, 2574, 2593, 2612,
    2631, 2650, 2669, 2688, 2708, 2727, 2746, 2765, 2784, 2802, 2821, 2840,
    2859, 2878, 2897, 2916, 2935, 2953, 2972, 2991, 3010, 3028, 3047, 3066,
    3084, 3103, 3122, 3140, 3159, 3177, 3196, 3214, 3233, 3251, 3270, 3288,
    3307, 3325, 3343, 3362, 3380, 3398, 3416, 3435, 3453, 3471, 3489, 3508,
    3526, 3544, 3562, 3580, 3598, 3616, 3634, 3652, 3670, 3688, 3706, 3724,
    3742, 3759, 3777, 3795, 3813, 3831, 3848, 3866, 3884, 3901, 3919, 3937,
    3954, 3972, 3989, 4007, 4024, 4042, 4059, 4077, 4094, 4112, 4129, 4146,
    4164, 4181, 4198, 4215, 4233, 4250, 4267, 4284, 4301, 4318, 4335, 4352,
    4369, 4386, 4403, 4420, 4437, 4454, 4471, 4488, 4505, 4521, 4538, 4555,
    4572, 4588, 4605, 4622, 4638, 4655, 4671, 4688, 4704, 4721, 4737, 4754,
    4770, 4787, 4803, 4819, 4836, 4852, 4868, 4884, 4901, 4917, 4933, 4949,
    4965, 4981, 4997, 5013, 5029, 5045, 5061, 5077, 5093, 5109, 5125, 5141,
    5156, 5172, 5188, 5204, 5219, 5235, 5251, 5266, 5282, 5297, 5313, 5328,
    5344, 5359, 5375, 5390, 5406, 5421, 5436, 5452, 5467, 5482, 5497, 5513,
    5528, 5543, 5558, 5573, 5588, 5603, 5618, 5633, 5648, 5663, 5678, 5693,
    5708, 5723, 5738, 5752, 5767, 5782, 5797, 5811, 5826, 5841, 5855, 5870,
    5884, 5899, 5913, 5928, 5942, 5957, 5971, 5985, 6000, 6014, 6028, 6043,
    6057, 6071, 6085, 6100, 6114, 6128, 6142, 6156, 6170, 6184, 6198, 6212,
    6226, 6240, 6254, 6268, 6282, 6295, 6309, 6323, 6337, 6350, 6364, 6378,
    6391, 6405, 6419, 6432, 6446, 6459, 6473, 6486, 6500, 6513, 6527, 6540,
    6553, 6567, 6580, 6593, 6606, 6620, 6633, 6646, 6659, 6672, 6685, 6698,
    6711, 6724, 6737, 6750, 6763, 6776, 6789, 6802, 6815, 6828, 6841, 6853,
    6866, 6879, 6892, 6904, 6917, 6930, 6942, 6955, 6967, 6980, 6992, 7005,
    7017, 7030, 7042, 7055, 7067, 7079, 7092, 7104, 7116, 7129, 7141, 7153,
    7165, 7177, 7190, 7202, 7214, 7226, 7238, 7250, 7262, 7274, 7286, 7298,
    7310, 7322, 7333, 7345, 7357, 7369, 7381, 7392, 7404, 7416, 7428, 7439,
    7451, 7462, 7474, 7486, 7497, 7509, 7520, 7532, 7543, 7555, 7566, 7577,
    7589, 7600, 7611, 7623, 7634, 7645, 7657, 7668, 7679, 7690, 7701, 7712,
    7724, 7735, 7746, 7757, 7768, 7779, 7790, 7801, 7812, 7823, 7833, 7844,
    7855, 7866, 7877, 7888, 7898, 7909, 7920, 7931, 7941, 7952, 7963, 7973,
    7984, 7994, 8005, 8015, 8026, 8036, 8047, 8057, 8068, 8078, 8089, 8099,
    8109, 8120, 8130, 8140, 8151, 8161, 8171, 8181, 8192, 9,
};

/* the angle whose sine is n/256, for n from -256 to 256 */
LINKED_LAST_DATA(asin_table)
static const Sint16 asin_table[514] = {
    -16384, -15461, -15078, -14785, -14537, -14318, -14120, -13938, -13769,
    -13609, -13458, -13314, -13177, -13045, -12918, -12795, -12676, -12560,
    -12448, -12339, -12233, -12129, -12027, -11928, -11830, -11735, -11641,
    -11549, -11459, -11370, -11283, -11197, -11112, -11028, -10946, -10865,
    -10784, -10705, -10627, -10550, -10474, -10398, -10323, -10250, -10176,
    -10104, -10033, -9962,  -9891,  -9822,  -9753,  -9685,  -9617,  -9550,
    -9483,  -9417,  -9352,  -9287,  -9222,  -9158,  -9094,  -9031,  -8969,
    -8906,  -8845,  -8783,  -8722,  -8662,  -8601,  -8542,  -8482,  -8423,
    -8364,  -8306,  -8248,  -8190,  -8132,  -8075,  -8018,  -7962,  -7906,
    -7850,  -7794,  -7739,  -7683,  -7629,  -7574,  -7520,  -7465,  -7412,
    -7358,  -7305,  -7251,  -7198,  -7146,  -7093,  -7041,  -6989,  -6937,
    -6885,  -6834,  -6782,  -6731,  -6680,  -6630,  -6579,  -6529,  -6479,
    -6429,  -6379,  -6329,  -6280,  -6230,  -6181,  -6132,  -6083,  -6034,
    -5986,  -5937,  -5889,  -5841,  -5793,  -5745,  -5697,  -5650,  -5602,
    -5555,  -5507,  -5460,  -5413,  -5366,  -5320,  -5273,  -5227,  -5180,
    -5134,  -5088,  -5042,  -4996,  -4950,  -4904,  -4858,  -4813,  -4767,
    -4722,  -4677,  -4632,  -4587,  -4542,  -4497,  -4452,  -4407,  -4363,
    -4318,  -4274,  -4229,  -4185,  -4141,  -4096,  -4052,  -4008,  -3964,
    -3921,  -3877,  -3833,  -3790,  -3746,  -3703,  -3659,  -3616,  -3572,
    -3529,  -3486,  -3443,  -3400,  -3357,  -3314,  -3271,  -3228,  -3186,
    -3143,  -3100,  -3058,  -3015,  -2973,  -2930,  -2888,  -2846,  -2803,
    -2761,  -2719,  -2677,  -2635,  -2592,  -2550,  -2509,  -2467,  -2425,
    -2383,  -2341,  -2299,  -2258,  -2216,  -2174,  -2133,  -2091,  -2049,
    -2008,  -1966,  -1925,  -1883,  -1842,  -1801,  -1759,  -1718,  -1677,
    -1635,  -1594,  -1553,  -1512,  -1471,  -1430,  -1388,  -1347,  -1306,
    -1265,  -1224,  -1183,  -1142,  -1101,  -1060,  -1019,  -978,   -937,
    -896,   -856,   -815,   -774,   -733,   -692,   -651,   -611,   -570,
    -529,   -488,   -447,   -407,   -366,   -325,   -284,   -243,   -203,
    -162,   -121,   -80,    -40,    0,      41,     81,     122,    163,
    204,    244,    285,    326,    367,    408,    448,    489,    530,
    571,    612,    652,    693,    734,    775,    816,    857,    897,
    938,    979,    1020,   1061,   1102,   1143,   1184,   1225,   1266,
    1307,   1348,   1389,   1431,   1472,   1513,   1554,   1595,   1636,
    1678,   1719,   1760,   1802,   1843,   1884,   1926,   1967,   2009,
    2050,   2092,   2134,   2175,   2217,   2259,   2300,   2342,   2384,
    2426,   2468,   2510,   2551,   2593,   2636,   2678,   2720,   2762,
    2804,   2847,   2889,   2931,   2974,   3016,   3059,   3101,   3144,
    3187,   3229,   3272,   3315,   3358,   3401,   3444,   3487,   3530,
    3573,   3617,   3660,   3704,   3747,   3791,   3834,   3878,   3922,
    3965,   4009,   4053,   4097,   4142,   4186,   4230,   4275,   4319,
    4364,   4408,   4453,   4498,   4543,   4588,   4633,   4678,   4723,
    4768,   4814,   4859,   4905,   4951,   4997,   5043,   5089,   5135,
    5181,   5228,   5274,   5321,   5367,   5414,   5461,   5508,   5556,
    5603,   5651,   5698,   5746,   5794,   5842,   5890,   5938,   5987,
    6035,   6084,   6133,   6182,   6231,   6281,   6330,   6380,   6430,
    6480,   6530,   6580,   6631,   6681,   6732,   6783,   6835,   6886,
    6938,   6990,   7042,   7094,   7147,   7199,   7252,   7306,   7359,
    7413,   7466,   7521,   7575,   7630,   7684,   7740,   7795,   7851,
    7907,   7963,   8019,   8076,   8133,   8191,   8249,   8307,   8365,
    8424,   8483,   8543,   8602,   8663,   8723,   8784,   8846,   8907,
    8970,   9032,   9095,   9159,   9223,   9288,   9353,   9418,   9484,
    9551,   9618,   9686,   9754,   9823,   9892,   9963,   10034,  10105,
    10177,  10251,  10324,  10399,  10475,  10551,  10628,  10706,  10785,
    10866,  10947,  11029,  11113,  11198,  11284,  11371,  11460,  11550,
    11642,  11736,  11831,  11929,  12028,  12130,  12234,  12340,  12449,
    12561,  12677,  12796,  12919,  13046,  13178,  13315,  13459,  13610,
    13770,  13939,  14121,  14319,  14538,  14786,  15079,  15462,  16384,
    9,
};

/* the angle whose cosine is n/256, for n from -256 to 256. the first entry,
 * a half turn, is H'8000, which a Sint16 reads as -HALF */
LINKED_LAST_DATA(acos_table)
static const Sint16 acos_table[514] = {
    -32768, 31846, 31463, 31170, 30922, 30703, 30505, 30323, 30154, 29994,
    29843,  29699, 29562, 29430, 29303, 29180, 29061, 28945, 28833, 28724,
    28618,  28514, 28412, 28313, 28215, 28120, 28026, 27934, 27844, 27755,
    27668,  27582, 27497, 27413, 27331, 27250, 27169, 27090, 27012, 26935,
    26859,  26783, 26708, 26635, 26561, 26489, 26418, 26347, 26276, 26207,
    26138,  26070, 26002, 25935, 25868, 25802, 25737, 25672, 25607, 25543,
    25479,  25416, 25354, 25291, 25230, 25168, 25107, 25047, 24986, 24927,
    24867,  24808, 24749, 24691, 24633, 24575, 24517, 24460, 24403, 24347,
    24291,  24235, 24179, 24124, 24068, 24014, 23959, 23905, 23850, 23797,
    23743,  23690, 23636, 23583, 23531, 23478, 23426, 23374, 23322, 23270,
    23219,  23167, 23116, 23065, 23015, 22964, 22914, 22864, 22814, 22764,
    22714,  22665, 22615, 22566, 22517, 22468, 22419, 22371, 22322, 22274,
    22226,  22178, 22130, 22082, 22035, 21987, 21940, 21892, 21845, 21798,
    21751,  21705, 21658, 21612, 21565, 21519, 21473, 21427, 21381, 21335,
    21289,  21243, 21198, 21152, 21107, 21062, 21017, 20972, 20927, 20882,
    20837,  20792, 20748, 20703, 20659, 20614, 20570, 20526, 20481, 20437,
    20393,  20349, 20306, 20262, 20218, 20175, 20131, 20088, 20044, 20001,
    19957,  19914, 19871, 19828, 19785, 19742, 19699, 19656, 19613, 19571,
    19528,  19485, 19443, 19400, 19358, 19315, 19273, 19231, 19188, 19146,
    19104,  19062, 19020, 18977, 18935, 18894, 18852, 18810, 18768, 18726,
    18684,  18643, 18601, 18559, 18518, 18476, 18434, 18393, 18351, 18310,
    18268,  18227, 18186, 18144, 18103, 18062, 18020, 17979, 17938, 17897,
    17856,  17815, 17773, 17732, 17691, 17650, 17609, 17568, 17527, 17486,
    17445,  17404, 17363, 17322, 17281, 17241, 17200, 17159, 17118, 17077,
    17036,  16996, 16955, 16914, 16873, 16832, 16792, 16751, 16710, 16669,
    16628,  16588, 16547, 16506, 16465, 16425, 16384, 16343, 16303, 16262,
    16221,  16180, 16140, 16099, 16058, 16017, 15976, 15936, 15895, 15854,
    15813,  15772, 15732, 15691, 15650, 15609, 15568, 15527, 15487, 15446,
    15405,  15364, 15323, 15282, 15241, 15200, 15159, 15118, 15077, 15036,
    14995,  14953, 14912, 14871, 14830, 14789, 14748, 14706, 14665, 14624,
    14582,  14541, 14500, 14458, 14417, 14375, 14334, 14292, 14250, 14209,
    14167,  14125, 14084, 14042, 14000, 13958, 13916, 13874, 13833, 13791,
    13748,  13706, 13664, 13622, 13580, 13537, 13495, 13453, 13410, 13368,
    13325,  13283, 13240, 13197, 13155, 13112, 13069, 13026, 12983, 12940,
    12897,  12854, 12811, 12767, 12724, 12680, 12637, 12593, 12550, 12506,
    12462,  12419, 12375, 12331, 12287, 12242, 12198, 12154, 12109, 12065,
    12020,  11976, 11931, 11886, 11841, 11796, 11751, 11706, 11661, 11616,
    11570,  11525, 11479, 11433, 11387, 11341, 11295, 11249, 11203, 11156,
    11110,  11063, 11017, 10970, 10923, 10876, 10828, 10781, 10733, 10686,
    10638,  10590, 10542, 10494, 10446, 10397, 10349, 10300, 10251, 10202,
    10153,  10103, 10054, 10004, 9954,  9904,  9854,  9804,  9753,  9703,
    9652,   9601,  9549,  9498,  9446,  9394,  9342,  9290,  9237,  9185,
    9132,   9078,  9025,  8971,  8918,  8863,  8809,  8754,  8700,  8644,
    8589,   8533,  8477,  8421,  8365,  8308,  8251,  8193,  8135,  8077,
    8019,   7960,  7901,  7841,  7782,  7721,  7661,  7600,  7538,  7477,
    7414,   7352,  7289,  7225,  7161,  7096,  7031,  6966,  6900,  6833,
    6766,   6698,  6630,  6561,  6492,  6421,  6350,  6279,  6207,  6133,
    6060,   5985,  5909,  5833,  5756,  5678,  5599,  5518,  5437,  5355,
    5271,   5186,  5100,  5013,  4924,  4834,  4742,  4648,  4553,  4455,
    4356,   4254,  4150,  4044,  3935,  3823,  3707,  3588,  3465,  3338,
    3206,   3069,  2925,  2774,  2614,  2445,  2263,  2065,  1846,  1598,
    1305,   922,   0,     9,
};

/* sin(ang) / |cos(ang)|: FSCA's sine times FSRRA's 1 / sqrt of the cosine
 * squared */
Float angle_sin_over_abs_cos(Angle ang)
{
    shz_sincos_t pair = zam_fsca(ang);

    return _builtin_fsrra(pair.cos * pair.cos) * pair.sin;
}

/* FIPR FV4,FV0 of (b, 0) and (a, 0) */
Float vector_inner_product(NJS_VECTOR *a, NJS_VECTOR *b)
{
    return zam_fipr(zam_load_vec4(b, 0.0F), zam_load_vec4(a, 0.0F));
}

Float vector_inner_product_across(NJS_VECTOR *a, NJS_VECTOR *b)
{
    return zam_vector_inner_product_across(a, b);
}

/* each part is a FIPR of two of a's parts, the second negated, with two of
 * b's. the parts go z first. a later FIPR's fourth lanes are 0 and the part
 * before it, which math.src leaves in FR7. each part goes to out before the
 * next part's reads, as in math.src */
void vector_outer_product(NJS_VECTOR *a, NJS_VECTOR *b, NJS_VECTOR *out)
{
    Float z = zam_fipr(shz_vec4_init(a->x, -a->y, 0.0F, 0.0F),
                       shz_vec4_init(b->y, b->x, 0.0F, 0.0F));
    Float y;

    out->z = z;
    y = zam_fipr(shz_vec4_init(a->z, -a->x, 0.0F, 0.0F),
                 shz_vec4_init(b->x, b->z, 0.0F, z));
    out->y = y;
    out->x = zam_fipr(shz_vec4_init(a->y, -a->z, 0.0F, 0.0F),
                      shz_vec4_init(b->z, b->y, 0.0F, y));
}

Float vector_magnitude(NJS_VECTOR *vector)
{
    return __builtin_sqrtf(zam_squared_length(zam_load_vec3(vector)));
}

Float vector_magnitude_across(NJS_VECTOR *vector)
{
    return zam_vector_magnitude_across(vector);
}

Float vector_magnitude_squared(NJS_VECTOR *vector)
{
    return zam_squared_length(zam_load_vec3(vector));
}

Float vector_magnitude_squared_across(NJS_VECTOR *vector)
{
    return zam_squared_length(zam_load_across(vector));
}

Float vector_unit(NJS_VECTOR *vector, NJS_VECTOR *out)
{
    shz_vec3_t unscaled = zam_load_vec3(vector);
    Float scale = _builtin_fsrra(zam_squared_length(unscaled));

    zam_store_vec3(out, shz_vec3_scale(unscaled, scale));
    return scale;
}

Float vector_unit_across(NJS_VECTOR *vector, NJS_VECTOR *out)
{
    return zam_vector_unit_across(vector, out);
}

/* a . b times FSRRA's reciprocals of the two lengths. the dot's FIPR takes
 * (a, 0) first. b follows, with b's reciprocal in its fourth lane, as
 * math.src leaves FR11. a's 0 multiplies that reciprocal, so a zero b, whose
 * reciprocal is infinite, gives a NaN */
Float vector_cosine(NJS_VECTOR *a, NJS_VECTOR *b)
{
    shz_vec3_t va = zam_load_vec3(a);
    shz_vec3_t vb = zam_load_vec3(b);
    Float reciprocal_a = _builtin_fsrra(zam_squared_length(va));
    Float reciprocal_b = _builtin_fsrra(zam_squared_length(vb));

    return reciprocal_a * reciprocal_b *
           zam_fipr(shz_vec3_vec4(va, 0.0F), shz_vec3_vec4(vb, reciprocal_b));
}

/* the difference math.src squares: from - to in x and y, and to - from in z */
ALWAYS_INLINE shz_vec3_t apart(const NJS_POINT3 *from, const NJS_POINT3 *to)
{
    return shz_vec3_init(from->x - to->x, from->y - to->y, to->z - from->z);
}

ALWAYS_INLINE shz_vec3_t apart_across(const NJS_POINT3 *from,
                                      const NJS_POINT3 *to)
{
    return shz_vec3_init(from->x - to->x, 0.0F, to->z - from->z);
}

Float point_distance(NJS_POINT3 *from, NJS_POINT3 *to)
{
    return __builtin_sqrtf(zam_squared_length(apart(from, to)));
}

Float point_distance_across(NJS_POINT3 *from, NJS_POINT3 *to)
{
    return __builtin_sqrtf(zam_squared_length(apart_across(from, to)));
}

Float point_distance_squared(NJS_POINT3 *from, NJS_POINT3 *to)
{
    return zam_squared_length(apart(from, to));
}

Float point_distance_squared_across(NJS_POINT3 *from, NJS_POINT3 *to)
{
    return zam_squared_length(apart_across(from, to));
}

/* atan_table gives the angle whose tangent is the smaller of |x| and |y|
 * over the larger. it is interpolated between two entries and rounded. the
 * octant turns that angle into the result */
Angle angle_arc_tan2(Float y, Float x)
{
    Float ratio;
    Float at;
    Sint32 entry;
    Sint32 angle;
    int octant = 0;

    if (y == 0.0F && x == 0.0F) {
        return (Angle)0x80000000;
    }
    if (!(x > 0.0F)) {
        x = __builtin_fabsf(x);
        octant += 4;
    }
    if (!(y > 0.0F)) {
        y = __builtin_fabsf(y);
        octant += 2;
    }
    if (x > y) {
        ratio = y / x;
    } else {
        ratio = x / y;
        octant += 1;
    }
    at = 512.0F * ratio;
    entry = (Sint32)at;
    at = at - (Float)entry;
    angle = atan_table[entry] +
            (Sint32)((Float)(atan_table[entry + 1] - atan_table[entry]) * at +
                     0.5F);
    switch (octant) {
    case 0:
        return angle;
    case 1:
        return QUARTER - angle;
    case 2:
        return -angle;
    case 3:
        return angle - QUARTER;
    case 4:
        return HALF - angle;
    case 5:
        return QUARTER + angle;
    case 6:
        return angle - HALF;
    default:
        return -QUARTER - angle;
    }
}

void point_project_centered(NJS_POINT3 *pos, NJS_POINT3 *out)
{
    Float z = pos->z;
    Float scale;

    out->z = z;
    scale = _nj_screen_.dist / z;
    out->y = _nj_scr_aspect_y_ * scale * pos->y;
    out->x = _nj_scr_aspect_x_ * scale * pos->x;
}

Float float_mod(Float x, Float y)
{
    if (y == 0.0F) {
        return 0.0F;
    }
    return x - (Float)(Sint32)(x / y) * y;
}

/* the angle for value, from -1 to 1, in a 513-entry table whose middle is
 * entry 256. value times 256, truncated toward 0, is n. the result adds to
 * entry 256 + n the step to the next entry, times the dropped fraction. that
 * fraction keeps value's sign, and the product truncates toward 0 */
ALWAYS_INLINE Angle angle_from_table(const Sint16 *table, Float value)
{
    Float at = 256.0F * value;
    Sint32 entry = (Sint32)at;
    const Sint16 *read = &table[256 + entry];

    at = at - (Float)entry;
    return read[0] + (Sint32)((Float)(read[1] - read[0]) * at);
}

Angle angle_arc_sin(Float sine)
{
    return angle_from_table(asin_table, sine);
}

Angle angle_arc_cos(Float cosine)
{
    return angle_from_table(acos_table, cosine);
}

void point_calc_xy(NJS_MATRIX *matrix, NJS_POINT3 *point, Float *out)
{
    zam_point_calc_xy(matrix, point, out);
}

/* a hair under 1/255, H'3B808080 */
#define PER_255 0x1.0101p-8F

/* the byte of color that starts shift bits above its lowest bit */
#define BYTE(color, shift) (((color) >> (shift)) & 0xff)

void color_to_argb(Uint32 color, NJS_ARGB *argb)
{
    argb->b = (Float)(Sint32)BYTE(color, 0);
    argb->g = (Float)(Sint32)BYTE(color, 8);
    argb->r = (Float)(Sint32)BYTE(color, 16);
    argb->a = (Float)(Sint32)(color >> 24);
}

Uint32 color_from_argb(NJS_ARGB *argb)
{
    return (Uint32)(Sint32)argb->a << 24 | (Uint32)(Sint32)argb->r << 16 |
           (Uint32)(Sint32)argb->g << 8 | (Uint32)(Sint32)argb->b;
}

/* one channel of color_multiply: the two as fractions of 1, multiplied,
 * held to 1, times 255 and rounded */
ALWAYS_INLINE Uint32 channel_multiply(Uint32 color, Uint32 by, int shift)
{
    Float product = (Float)(Sint32)BYTE(color, shift) * PER_255 *
                    ((Float)(Sint32)BYTE(by, shift) * PER_255);

    if (!(1.0F > product)) {
        product = 1.0F;
    }
    return (Uint32)(Sint32)(product * 255.0F + 0.5F) << shift;
}

Uint32 color_multiply(Uint32 color, Uint32 by)
{
    return channel_multiply(color, by, 0) | channel_multiply(color, by, 8) |
           channel_multiply(color, by, 16) | channel_multiply(color, by, 24);
}

void color_to_argb_unit(Uint32 color, NJS_ARGB *argb)
{
    argb->b = (Float)(Sint32)BYTE(color, 0) * PER_255;
    argb->g = (Float)(Sint32)BYTE(color, 8) * PER_255;
    argb->r = (Float)(Sint32)BYTE(color, 16) * PER_255;
    argb->a = (Float)(Sint32)(color >> 24) * PER_255;
}

/* each channel times 255 plus 0.5, one FMAC, truncated */
Uint32 color_from_argb_unit(NJS_ARGB *argb)
{
    return (Uint32)(Sint32)__builtin_fmaf(argb->a, 255.0F, 0.5F) << 24 |
           (Uint32)(Sint32)__builtin_fmaf(argb->r, 255.0F, 0.5F) << 16 |
           (Uint32)(Sint32)__builtin_fmaf(argb->g, 255.0F, 0.5F) << 8 |
           (Uint32)(Sint32)__builtin_fmaf(argb->b, 255.0F, 0.5F);
}

/* from the last byte down. a count of 0 writes 2^32 bytes below mem, as in
 * math.src */
void fill_bytes(void *mem, int value, int count)
{
    Uint8 *at = (Uint8 *)mem + count;
    Uint32 left = (Uint32)count;

    do {
        *--at = (Uint8)value;
    } while (--left != 0);
}

/* eight longs a pass, from the last down. it runs size / 32 passes, or 2^32
 * for a size under 32, as in math.src */
void fill_blocks(void *mem, Uint32 value, int size)
{
    Uint32 *at = (Uint32 *)((Uint8 *)mem + size);
    Uint32 passes = (Uint32)size >> 5;
    int i;

    do {
        for (i = 0; i < 8; i++) {
            *--at = value;
        }
    } while (--passes != 0);
}
