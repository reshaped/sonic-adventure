/* Kamui background plane: the triangle drawn behind every other polygon */
#include <kamui.h>
#include <sg_xpt.h>

/* the ISP word's bits that set which words a vertex holds */
#define ISP_TEXTURE 0x02000000 /* a texture: the vertex holds UVs */
#define ISP_OFFSET 0x01000000 /* an offset color */
#define ISP_UV16 0x00400000 /* 16-bit UVs, both in one word */

/* the background tag word's fields. the 3 bits from SKIP_SHIFT count the words
 * each vertex holds after x, y and z. END_PACKET leaves SHADOW and BYPASS
 * clear. BYPASS is the cache bypass bit */
#define SKIP_SHIFT 24
#define SHADOW 0x08000000
#define BYPASS 0x10000000

/* the ISP, TSP and texture words, then three vertices */
#define PACKET kmiDeviceExtension.background_packet

/* a float's bits, stored unconverted */
/* @hack type: the float read through its address cast to a KMDWORD pointer,
 * which SHC loads with one MOV.L (MOV.L @(4,R1),R1 at 0x8c666814 in
 * 1ST_READ.BIN, 0x8ce15f58 in LOADER.BIN). kmiProcessVertex_00 to _14 use it.
 * read through a union local, each float goes to the frame and back through
 * FMOV.S. the section then runs 1664 bytes longer in 1ST_READ.BIN and 1552 in
 * LOADER.BIN, and 3247 of the functions' 3419 words differ in 1ST_READ.BIN and
 * 3243 of 3417 in LOADER.BIN */
#define FLOAT_WORD(f) (*(KMDWORD *)&(f))

/* a color of floats from 0 to 1, packed in one word. the alpha is cast to
 * unsigned, since it reaches the sign bit */
#define PACK_ARGB(alpha, red, green, blue)                                     \
    ((KMDWORD)(KMINT32)((alpha) * 255.0F) << 24 |                              \
     (KMINT32)((red) * 255.0F) << 16 | (KMINT32)((green) * 255.0F) << 8 |      \
     (KMINT32)((blue) * 255.0F))

#define PACK_INTENSITY(i) PACK_ARGB(i, i, i, i)

#define PUT_POSITION(next, vertex)                                             \
    PACKET[(next)++] = FLOAT_WORD((vertex)->fX);                               \
    PACKET[(next)++] = FLOAT_WORD((vertex)->fY);                               \
    PACKET[(next)++] = FLOAT_WORD((vertex)->u.fZ)

#define PUT_HEADER(next, isp, tsp, texture)                                    \
    PACKET[(next)++] = (isp);                                                  \
    PACKET[(next)++] = (tsp);                                                  \
    PACKET[(next)++] = (texture)

#define END_PACKET(next, isp, first)                                           \
    kmiDeviceExtension.background_size = (next) * 4;                           \
    background_set_skip(background_vertex_words(isp), FALSE, FALSE);           \
    kmiDeviceExtension.background_depth = FLOAT_WORD((first)->u.fZ)

KMSTATUS kmiProcessVertex_00(void *pVertex[3]);
KMSTATUS kmiProcessVertex_01(void *pVertex[3]);
KMSTATUS kmiProcessVertex_02(void *pVertex[3]);
KMSTATUS kmiProcessVertex_03(void *pVertex[3]);
KMSTATUS kmiProcessVertex_04(void *pVertex[3]);
KMSTATUS kmiProcessVertex_05(void *pVertex[3]);
KMSTATUS kmiProcessVertex_06(void *pVertex[3]);
KMSTATUS kmiProcessVertex_07(void *pVertex[3]);
KMSTATUS kmiProcessVertex_08(void *pVertex[3]);
KMSTATUS kmiProcessVertex_09(void *pVertex[3]);
KMSTATUS kmiProcessVertex_10(void *pVertex[3]);
KMSTATUS kmiProcessVertex_11(void *pVertex[3]);
KMSTATUS kmiProcessVertex_12(void *pVertex[3]);
KMSTATUS kmiProcessVertex_13(void *pVertex[3]);
KMSTATUS kmiProcessVertex_14(void *pVertex[3]);
KMSTATUS kmiProcessVertex_15(void *pVertex[3]);
KMSTATUS kmiProcessVertex_16(void *pVertex[3]);
KMSTATUS kmiProcessVertex_17(void *pVertex[3]);

static void background_set_skip(int count, KMBOOLEAN shadow, KMBOOLEAN bypass)
{
    kmiDeviceExtension.background_skip = (count & 7) << SKIP_SHIFT;
    if (bypass) {
        kmiDeviceExtension.background_skip |= BYPASS;
    }
    if (shadow) {
        kmiDeviceExtension.background_skip |= SHADOW;
    }
}

/* the words a vertex holds after x, y and z under isp. it holds its color, and
 * with a texture its UVs, in one word or two, and any offset color */
static int background_vertex_words(KMDWORD isp)
{
    int count = 1;

    if (isp & ISP_TEXTURE) {
        count++;
        if (isp & ISP_OFFSET) {
            count++;
        }
        if (!(isp & ISP_UV16)) {
            count++;
        }
    }
    return count;
}

static int background_put_offset(KMDWORD isp, int next, KMDWORD color)
{
    if (isp & ISP_OFFSET) {
        PACKET[next++] = color;
    }
    return next;
}

/* store a vertex's packed 16-bit UVs at next when isp has a texture. they go in
 * as they are under ISP_UV16, or else as two floats, each UV the high half of
 * its float. the result is the index after them */
static int background_put_uv16(KMDWORD isp, int next, KMDWORD uv)
{
    if (isp & ISP_TEXTURE) {
        if (isp & ISP_UV16) {
            PACKET[next++] = uv;
        } else {
            PACKET[next++] = uv & 0xffff0000;
            PACKET[next++] = (KMDWORD)(unsigned short)uv << 16;
        }
    }
    return next;
}

static int background_put_uv32(KMDWORD isp, int next, KMDWORD u, KMDWORD v)
{
    if (isp & ISP_TEXTURE) {
        PACKET[next++] = u;
        PACKET[next++] = v;
    }
    return next;
}

/* write the packet Kamui starts with, an opaque black triangle at a depth of
 * 0.00001. its vertices are (2, 260), (150, 2) and (560, 260). it is
 * untextured, and its TSP word blends with a source factor of one and a
 * destination factor of zero. each vertex holds only its color after x, y and
 * z. background_depth and background_skip take the packet's depth and skip
 * count. the result is KMSTATUS_SUCCESS */
KMSTATUS kmiInitializeBackGroundPlane(void)
{
    PACKET[0] = 0x80800000;
    PACKET[1] = 0x20800000;
    PACKET[2] = 0x00000000;
    PACKET[3] = 0x40000000;
    PACKET[4] = 0x43820000;
    PACKET[5] = 0x3727c5ac;
    PACKET[6] = 0xff000000;
    PACKET[7] = 0x43160000;
    PACKET[8] = 0x40000000;
    PACKET[9] = 0x3727c5ac;
    PACKET[10] = 0xff000000;
    PACKET[11] = 0x440c0000;
    PACKET[12] = 0x43820000;
    PACKET[13] = 0x3727c5ac;
    PACKET[14] = 0xff000000;
    kmiDeviceExtension.background_depth = PACKET[5];
    kmiDeviceExtension.background_skip = 1 << SKIP_SHIFT;
    return KMSTATUS_SUCCESS;
}

/* kmiProcessVertex_00 to _14 each write the background plane's packet from
 * three vertices of their type, and return KMSTATUS_SUCCESS */

KMSTATUS kmiProcessVertex_00(void *pVertex[3])
{
    PKMVERTEX0 points[3];
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD next = 0;

    points[0] = pVertex[0];
    points[1] = pVertex[1];
    points[2] = pVertex[2];
    isp = kmiDeviceExtension.background_isp;
    tsp = kmiDeviceExtension.background_tsp;
    texture = kmiDeviceExtension.background_texture;
    PUT_HEADER(next, isp, tsp, texture);

    PUT_POSITION(next, points[0]);
    PACKET[next++] = points[0]->uBaseRGB.dwPacked;

    PUT_POSITION(next, points[1]);
    PACKET[next++] = points[1]->uBaseRGB.dwPacked;

    PUT_POSITION(next, points[2]);
    PACKET[next++] = points[2]->uBaseRGB.dwPacked;

    END_PACKET(next, isp, points[0]);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiProcessVertex_01(void *pVertex[3])
{
    PKMVERTEX1 points[3];
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD next = 0;

    points[0] = pVertex[0];
    points[1] = pVertex[1];
    points[2] = pVertex[2];
    isp = kmiDeviceExtension.background_isp;
    tsp = kmiDeviceExtension.background_tsp;
    texture = kmiDeviceExtension.background_texture;
    PUT_HEADER(next, isp, tsp, texture);

    PUT_POSITION(next, points[0]);
    PACKET[next++] = PACK_ARGB(points[0]->fBaseAlpha, points[0]->fBaseRed,
                               points[0]->fBaseGreen, points[0]->fBaseBlue);

    PUT_POSITION(next, points[1]);
    PACKET[next++] = PACK_ARGB(points[1]->fBaseAlpha, points[1]->fBaseRed,
                               points[1]->fBaseGreen, points[1]->fBaseBlue);

    PUT_POSITION(next, points[2]);
    PACKET[next++] = PACK_ARGB(points[2]->fBaseAlpha, points[2]->fBaseRed,
                               points[2]->fBaseGreen, points[2]->fBaseBlue);

    END_PACKET(next, isp, points[0]);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiProcessVertex_02(void *pVertex[3])
{
    PKMVERTEX2 points[3];
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD next;

    points[0] = pVertex[0];
    /* @hack spelling: next zeroed here, between the copies of pVertex[0] and
     * pVertex[1], not at its declaration. in 1ST_READ.BIN, this keeps
     * &points[1] in R7 and &points[2] in R6 (0x8c666ad0, 0x8c666ad6); in
     * LOADER.BIN, it changes nothing. zeroed at its declaration, the two trade
     * registers, and 14 of 178 words differ */
    next = 0;
    points[1] = pVertex[1];
    points[2] = pVertex[2];
    isp = kmiDeviceExtension.background_isp;
    tsp = kmiDeviceExtension.background_tsp;
    texture = kmiDeviceExtension.background_texture;
    PUT_HEADER(next, isp, tsp, texture);

    PUT_POSITION(next, points[0]);
    PACKET[next++] = PACK_INTENSITY(points[0]->fBaseIntensity);

    PUT_POSITION(next, points[1]);
    PACKET[next++] = PACK_INTENSITY(points[1]->fBaseIntensity);

    PUT_POSITION(next, points[2]);
    PACKET[next++] = PACK_INTENSITY(points[2]->fBaseIntensity);

    END_PACKET(next, isp, points[0]);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiProcessVertex_03(void *pVertex[3])
{
    PKMVERTEX3 points[3];
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD next;

    points[0] = pVertex[0];
    /* @hack spelling: next zeroed here, between the copies of pVertex[0] and
     * pVertex[1], not at its declaration. in 1ST_READ.BIN, this keeps the loads
     * at 0x8c666c54 and 0x8c666c58 in R4 and R5; in LOADER.BIN, it changes
     * nothing. zeroed at its declaration, the two trade registers, and 4 of 184
     * words differ */
    next = 0;
    points[1] = pVertex[1];
    points[2] = pVertex[2];
    isp = kmiDeviceExtension.background_isp;
    tsp = kmiDeviceExtension.background_tsp;
    texture = kmiDeviceExtension.background_texture;
    PUT_HEADER(next, isp, tsp, texture);

    PUT_POSITION(next, points[0]);
    next = background_put_uv32(isp, next, FLOAT_WORD(points[0]->fU),
                               FLOAT_WORD(points[0]->fV));
    PACKET[next++] = points[0]->uBaseRGB.dwPacked;
    next = background_put_offset(isp, next, points[0]->uOffsetRGB.dwPacked);

    PUT_POSITION(next, points[1]);
    next = background_put_uv32(isp, next, FLOAT_WORD(points[1]->fU),
                               FLOAT_WORD(points[1]->fV));
    PACKET[next++] = points[1]->uBaseRGB.dwPacked;
    next = background_put_offset(isp, next, points[1]->uOffsetRGB.dwPacked);

    PUT_POSITION(next, points[2]);
    next = background_put_uv32(isp, next, FLOAT_WORD(points[2]->fU),
                               FLOAT_WORD(points[2]->fV));
    PACKET[next++] = points[2]->uBaseRGB.dwPacked;
    next = background_put_offset(isp, next, points[2]->uOffsetRGB.dwPacked);

    END_PACKET(next, isp, points[0]);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiProcessVertex_04(void *pVertex[3])
{
    PKMVERTEX4 points[3];
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD next = 0;

    points[0] = pVertex[0];
    points[1] = pVertex[1];
    points[2] = pVertex[2];
    isp = kmiDeviceExtension.background_isp;
    tsp = kmiDeviceExtension.background_tsp;
    texture = kmiDeviceExtension.background_texture;
    PUT_HEADER(next, isp, tsp, texture);

    PUT_POSITION(next, points[0]);
    next = background_put_uv16(isp, next, points[0]->dwUV);
    PACKET[next++] = points[0]->uBaseRGB.dwPacked;
    next = background_put_offset(isp, next, points[0]->uOffsetRGB.dwPacked);

    PUT_POSITION(next, points[1]);
    next = background_put_uv16(isp, next, points[1]->dwUV);
    PACKET[next++] = points[1]->uBaseRGB.dwPacked;
    /* @bug logic: the second vertex and the third take the first vertex's
     * offset color, read through points[0] (MOV.L @R14,R6 at 0x8c666e98 and
     * 0x8c666ee4, 0x8ce16604 and 0x8ce16650 in LOADER.BIN), where
     * kmiProcessVertex_03 takes each vertex's own */
    next = background_put_offset(isp, next, points[0]->uOffsetRGB.dwPacked);

    PUT_POSITION(next, points[2]);
    next = background_put_uv16(isp, next, points[2]->dwUV);
    PACKET[next++] = points[2]->uBaseRGB.dwPacked;
    next = background_put_offset(isp, next, points[0]->uOffsetRGB.dwPacked);

    END_PACKET(next, isp, points[0]);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiProcessVertex_05(void *pVertex[3])
{
    PKMVERTEX5 points[3];
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD next;

    points[0] = pVertex[0];
    /* @hack spelling: next zeroed here, between the copies of pVertex[0] and
     * pVertex[1], not at its declaration. in 1ST_READ.BIN, this keeps
     * &points[1] in R11 and &points[2] in R10 (0x8c666f5a, 0x8c666f60); in
     * LOADER.BIN, it changes nothing. zeroed at its declaration, the two trade
     * registers, and 26 of 358 words differ */
    next = 0;
    points[1] = pVertex[1];
    points[2] = pVertex[2];
    isp = kmiDeviceExtension.background_isp;
    tsp = kmiDeviceExtension.background_tsp;
    texture = kmiDeviceExtension.background_texture;
    PUT_HEADER(next, isp, tsp, texture);

    PUT_POSITION(next, points[0]);
    next = background_put_uv32(isp, next, FLOAT_WORD(points[0]->fU),
                               FLOAT_WORD(points[0]->fV));
    PACKET[next++] = PACK_ARGB(points[0]->fBaseAlpha, points[0]->fBaseRed,
                               points[0]->fBaseGreen, points[0]->fBaseBlue);
    next = background_put_offset(
        isp, next,
        PACK_ARGB(points[0]->fOffsetAlpha, points[0]->fOffsetRed,
                  points[0]->fOffsetGreen, points[0]->fOffsetBlue));

    PUT_POSITION(next, points[1]);
    next = background_put_uv32(isp, next, FLOAT_WORD(points[1]->fU),
                               FLOAT_WORD(points[1]->fV));
    PACKET[next++] = PACK_ARGB(points[1]->fBaseAlpha, points[1]->fBaseRed,
                               points[1]->fBaseGreen, points[1]->fBaseBlue);
    next = background_put_offset(
        isp, next,
        PACK_ARGB(points[1]->fOffsetAlpha, points[1]->fOffsetRed,
                  points[1]->fOffsetGreen, points[1]->fOffsetBlue));

    PUT_POSITION(next, points[2]);
    next = background_put_uv32(isp, next, FLOAT_WORD(points[2]->fU),
                               FLOAT_WORD(points[2]->fV));
    PACKET[next++] = PACK_ARGB(points[2]->fBaseAlpha, points[2]->fBaseRed,
                               points[2]->fBaseGreen, points[2]->fBaseBlue);
    next = background_put_offset(
        isp, next,
        PACK_ARGB(points[2]->fOffsetAlpha, points[2]->fOffsetRed,
                  points[2]->fOffsetGreen, points[2]->fOffsetBlue));

    END_PACKET(next, isp, points[0]);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiProcessVertex_06(void *pVertex[3])
{
    PKMVERTEX6 points[3];
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD next;

    points[0] = pVertex[0];
    /* @hack spelling: next zeroed here, between the copies of pVertex[0] and
     * pVertex[1], not at its declaration. in 1ST_READ.BIN, this keeps the loads
     * at 0x8c667272 and 0x8c667276 in R5 and R4, and next in R12 (0x8c667440);
     * in LOADER.BIN, it changes nothing. zeroed at its declaration, the three
     * move, and 13 of 348 words differ */
    next = 0;
    points[1] = pVertex[1];
    points[2] = pVertex[2];
    isp = kmiDeviceExtension.background_isp;
    tsp = kmiDeviceExtension.background_tsp;
    texture = kmiDeviceExtension.background_texture;
    PUT_HEADER(next, isp, tsp, texture);

    PUT_POSITION(next, points[0]);
    next = background_put_uv16(isp, next, points[0]->dwUV);
    PACKET[next++] = PACK_ARGB(points[0]->fBaseAlpha, points[0]->fBaseRed,
                               points[0]->fBaseGreen, points[0]->fBaseBlue);
    next = background_put_offset(
        isp, next,
        PACK_ARGB(points[0]->fOffsetAlpha, points[0]->fOffsetRed,
                  points[0]->fOffsetGreen, points[0]->fOffsetBlue));

    PUT_POSITION(next, points[1]);
    /* @bug logic: the second vertex and the third take the first vertex's UVs,
     * read through points[0] (MOV.L @R14,R6 at 0x8c667376 and 0x8c667432,
     * 0x8ce16ac0 and 0x8ce16b7c in LOADER.BIN), where each takes its own
     * color and offset color */
    next = background_put_uv16(isp, next, points[0]->dwUV);
    PACKET[next++] = PACK_ARGB(points[1]->fBaseAlpha, points[1]->fBaseRed,
                               points[1]->fBaseGreen, points[1]->fBaseBlue);
    next = background_put_offset(
        isp, next,
        PACK_ARGB(points[1]->fOffsetAlpha, points[1]->fOffsetRed,
                  points[1]->fOffsetGreen, points[1]->fOffsetBlue));

    PUT_POSITION(next, points[2]);
    next = background_put_uv16(isp, next, points[0]->dwUV);
    PACKET[next++] = PACK_ARGB(points[2]->fBaseAlpha, points[2]->fBaseRed,
                               points[2]->fBaseGreen, points[2]->fBaseBlue);
    next = background_put_offset(
        isp, next,
        PACK_ARGB(points[2]->fOffsetAlpha, points[2]->fOffsetRed,
                  points[2]->fOffsetGreen, points[2]->fOffsetBlue));

    END_PACKET(next, isp, points[0]);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiProcessVertex_07(void *pVertex[3])
{
    PKMVERTEX7 points[3];
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD next;

    points[0] = pVertex[0];
    /* @hack spelling: next zeroed here, between the copies of pVertex[0] and
     * pVertex[1], not at its declaration. in 1ST_READ.BIN, this keeps
     * &points[1] in R11 and &points[2] in R10 (0x8c66753a, 0x8c667540); in
     * LOADER.BIN, it changes nothing. zeroed at its declaration, the two trade
     * registers, and 38 of 303 words differ */
    next = 0;
    points[1] = pVertex[1];
    points[2] = pVertex[2];
    isp = kmiDeviceExtension.background_isp;
    tsp = kmiDeviceExtension.background_tsp;
    texture = kmiDeviceExtension.background_texture;
    PUT_HEADER(next, isp, tsp, texture);

    PUT_POSITION(next, points[0]);
    next = background_put_uv32(isp, next, FLOAT_WORD(points[0]->fU),
                               FLOAT_WORD(points[0]->fV));
    PACKET[next++] = PACK_INTENSITY(points[0]->fBaseIntensity);
    next = background_put_offset(isp, next,
                                 PACK_INTENSITY(points[0]->fOffsetIntensity));
    /* @bug logic: the first vertex's offset intensity is written a second time,
     * after the vertex's other words, whether or not the ISP word has an
     * offset color (MOV.L R3,@(R0,R2) at 0x8c66762c, 0x8ce16d68 in
     * LOADER.BIN). the first vertex thus holds one word more than
     * background_skip counts for each vertex */
    PACKET[next++] = PACK_INTENSITY(points[0]->fOffsetIntensity);

    PUT_POSITION(next, points[1]);
    next = background_put_uv32(isp, next, FLOAT_WORD(points[1]->fU),
                               FLOAT_WORD(points[1]->fV));
    PACKET[next++] = PACK_INTENSITY(points[1]->fBaseIntensity);
    next = background_put_offset(isp, next,
                                 PACK_INTENSITY(points[1]->fOffsetIntensity));

    PUT_POSITION(next, points[2]);
    next = background_put_uv32(isp, next, FLOAT_WORD(points[2]->fU),
                               FLOAT_WORD(points[2]->fV));
    PACKET[next++] = PACK_INTENSITY(points[2]->fBaseIntensity);
    next = background_put_offset(isp, next,
                                 PACK_INTENSITY(points[2]->fOffsetIntensity));

    END_PACKET(next, isp, points[0]);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiProcessVertex_08(void *pVertex[3])
{
    PKMVERTEX8 points[3];
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD next;

    points[0] = pVertex[0];
    /* @hack spelling: next zeroed here, between the copies of pVertex[0] and
     * pVertex[1], not at its declaration. in 1ST_READ.BIN, this keeps next in
     * R9 (MOV R0,R9 at 0x8c6678d4); in LOADER.BIN, it changes nothing. zeroed
     * at its declaration, next moves to R12, and 22 of 275 words differ */
    next = 0;
    points[1] = pVertex[1];
    points[2] = pVertex[2];
    isp = kmiDeviceExtension.background_isp;
    tsp = kmiDeviceExtension.background_tsp;
    texture = kmiDeviceExtension.background_texture;
    PUT_HEADER(next, isp, tsp, texture);

    PUT_POSITION(next, points[0]);
    next = background_put_uv16(isp, next, points[0]->dwUV);
    PACKET[next++] = PACK_INTENSITY(points[0]->fBaseIntensity);
    next = background_put_offset(isp, next,
                                 PACK_INTENSITY(points[0]->fOffsetIntensity));

    PUT_POSITION(next, points[1]);
    /* @bug logic: the second vertex and the third take the first vertex's UVs,
     * read through points[0] (MOV.L @R14,R6 at 0x8c6678c4 and 0x8c66794e,
     * 0x8ce16fe4 and 0x8ce17072 in LOADER.BIN), where each takes its own
     * intensities */
    next = background_put_uv16(isp, next, points[0]->dwUV);
    PACKET[next++] = PACK_INTENSITY(points[1]->fBaseIntensity);
    next = background_put_offset(isp, next,
                                 PACK_INTENSITY(points[1]->fOffsetIntensity));

    PUT_POSITION(next, points[2]);
    next = background_put_uv16(isp, next, points[0]->dwUV);
    PACKET[next++] = PACK_INTENSITY(points[2]->fBaseIntensity);
    next = background_put_offset(isp, next,
                                 PACK_INTENSITY(points[2]->fOffsetIntensity));

    END_PACKET(next, isp, points[0]);
    return KMSTATUS_SUCCESS;
}

/* vertex types 9 to 14 hold two volumes; the packet takes the first volume's
 * values */
KMSTATUS kmiProcessVertex_09(void *pVertex[3])
{
    PKMVERTEX9 points[3];
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD next = 0;

    points[0] = pVertex[0];
    points[1] = pVertex[1];
    points[2] = pVertex[2];
    isp = kmiDeviceExtension.background_isp;
    tsp = kmiDeviceExtension.background_tsp;
    texture = kmiDeviceExtension.background_texture;
    PUT_HEADER(next, isp, tsp, texture);

    PUT_POSITION(next, points[0]);
    PACKET[next++] = points[0]->uBaseRGB0m.dwPacked;

    PUT_POSITION(next, points[1]);
    PACKET[next++] = points[1]->uBaseRGB0m.dwPacked;

    PUT_POSITION(next, points[2]);
    PACKET[next++] = points[2]->uBaseRGB0m.dwPacked;

    END_PACKET(next, isp, points[0]);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiProcessVertex_10(void *pVertex[3])
{
    PKMVERTEX10 points[3];
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD next;

    points[0] = pVertex[0];
    /* @hack spelling: next zeroed here, between the copies of pVertex[0] and
     * pVertex[1], not at its declaration. in 1ST_READ.BIN, this keeps
     * &points[1] in R7 and &points[2] in R6 (0x8c667b58, 0x8c667b5e); in
     * LOADER.BIN, it changes nothing. zeroed at its declaration, the two trade
     * registers, and 14 of 180 words differ */
    next = 0;
    points[1] = pVertex[1];
    points[2] = pVertex[2];
    isp = kmiDeviceExtension.background_isp;
    tsp = kmiDeviceExtension.background_tsp;
    texture = kmiDeviceExtension.background_texture;
    PUT_HEADER(next, isp, tsp, texture);

    PUT_POSITION(next, points[0]);
    PACKET[next++] = PACK_INTENSITY(points[0]->fBaseIntensity0m);

    PUT_POSITION(next, points[1]);
    PACKET[next++] = PACK_INTENSITY(points[1]->fBaseIntensity0m);

    PUT_POSITION(next, points[2]);
    PACKET[next++] = PACK_INTENSITY(points[2]->fBaseIntensity0m);

    END_PACKET(next, isp, points[0]);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiProcessVertex_11(void *pVertex[3])
{
    PKMVERTEX11 points[3];
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD next;

    points[0] = pVertex[0];
    /* @hack spelling: next zeroed here, between the copies of pVertex[0] and
     * pVertex[1], not at its declaration. in 1ST_READ.BIN, this keeps the loads
     * at 0x8c667d30 and 0x8c667d34 in R4 and R5; in LOADER.BIN, it changes
     * nothing. zeroed at its declaration, the two trade registers, and 4 of 192
     * words differ */
    next = 0;
    points[1] = pVertex[1];
    points[2] = pVertex[2];
    isp = kmiDeviceExtension.background_isp;
    tsp = kmiDeviceExtension.background_tsp;
    texture = kmiDeviceExtension.background_texture;
    PUT_HEADER(next, isp, tsp, texture);

    PUT_POSITION(next, points[0]);
    next = background_put_uv32(isp, next, FLOAT_WORD(points[0]->fU0m),
                               FLOAT_WORD(points[0]->fV0m));
    PACKET[next++] = points[0]->uBaseRGB0m.dwPacked;
    next = background_put_offset(isp, next, points[0]->uOffsetRGB0m.dwPacked);

    PUT_POSITION(next, points[1]);
    next = background_put_uv32(isp, next, FLOAT_WORD(points[1]->fU0m),
                               FLOAT_WORD(points[1]->fV0m));
    PACKET[next++] = points[1]->uBaseRGB0m.dwPacked;
    next = background_put_offset(isp, next, points[1]->uOffsetRGB0m.dwPacked);

    PUT_POSITION(next, points[2]);
    next = background_put_uv32(isp, next, FLOAT_WORD(points[2]->fU0m),
                               FLOAT_WORD(points[2]->fV0m));
    PACKET[next++] = points[2]->uBaseRGB0m.dwPacked;
    next = background_put_offset(isp, next, points[2]->uOffsetRGB0m.dwPacked);

    END_PACKET(next, isp, points[0]);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiProcessVertex_12(void *pVertex[3])
{
    PKMVERTEX12 points[3];
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD next = 0;

    points[0] = pVertex[0];
    points[1] = pVertex[1];
    points[2] = pVertex[2];
    isp = kmiDeviceExtension.background_isp;
    tsp = kmiDeviceExtension.background_tsp;
    texture = kmiDeviceExtension.background_texture;
    PUT_HEADER(next, isp, tsp, texture);

    PUT_POSITION(next, points[0]);
    next = background_put_uv16(isp, next, points[0]->dwUV0m);
    PACKET[next++] = points[0]->uBaseRGB0m.dwPacked;
    next = background_put_offset(isp, next, points[0]->uOffsetRGB0m.dwPacked);

    PUT_POSITION(next, points[1]);
    /* @bug logic: the second vertex takes the first vertex's UVs, read through
     * points[0] (MOV.L @R14,R6 at 0x8c667f42, 0x8ce17616 in LOADER.BIN), where
     * the third takes its own */
    next = background_put_uv16(isp, next, points[0]->dwUV0m);
    PACKET[next++] = points[1]->uBaseRGB0m.dwPacked;
    next = background_put_offset(isp, next, points[1]->uOffsetRGB0m.dwPacked);

    PUT_POSITION(next, points[2]);
    next = background_put_uv16(isp, next, points[2]->dwUV0m);
    PACKET[next++] = points[2]->uBaseRGB0m.dwPacked;
    next = background_put_offset(isp, next, points[2]->uOffsetRGB0m.dwPacked);

    END_PACKET(next, isp, points[0]);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiProcessVertex_13(void *pVertex[3])
{
    PKMVERTEX13 points[3];
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD next;

    points[0] = pVertex[0];
    /* @hack spelling: next zeroed here, between the copies of pVertex[0] and
     * pVertex[1], not at its declaration. in 1ST_READ.BIN, this keeps
     * &points[1] in R11 and &points[2] in R10 (0x8c66805a, 0x8c668060); in
     * LOADER.BIN, it changes nothing. zeroed at its declaration, the two trade
     * registers, and 32 of 285 words differ */
    next = 0;
    points[1] = pVertex[1];
    points[2] = pVertex[2];
    isp = kmiDeviceExtension.background_isp;
    tsp = kmiDeviceExtension.background_tsp;
    texture = kmiDeviceExtension.background_texture;
    PUT_HEADER(next, isp, tsp, texture);

    PUT_POSITION(next, points[0]);
    next = background_put_uv32(isp, next, FLOAT_WORD(points[0]->fU0m),
                               FLOAT_WORD(points[0]->fV0m));
    PACKET[next++] = PACK_INTENSITY(points[0]->fBaseIntensity0m);
    next = background_put_offset(isp, next,
                                 PACK_INTENSITY(points[0]->fOffsetIntensity0m));

    PUT_POSITION(next, points[1]);
    next = background_put_uv32(isp, next, FLOAT_WORD(points[1]->fU0m),
                               FLOAT_WORD(points[1]->fV0m));
    PACKET[next++] = PACK_INTENSITY(points[1]->fBaseIntensity0m);
    next = background_put_offset(isp, next,
                                 PACK_INTENSITY(points[1]->fOffsetIntensity0m));

    PUT_POSITION(next, points[2]);
    next = background_put_uv32(isp, next, FLOAT_WORD(points[2]->fU0m),
                               FLOAT_WORD(points[2]->fV0m));
    PACKET[next++] = PACK_INTENSITY(points[2]->fBaseIntensity0m);
    next = background_put_offset(isp, next,
                                 PACK_INTENSITY(points[2]->fOffsetIntensity0m));

    END_PACKET(next, isp, points[0]);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmiProcessVertex_14(void *pVertex[3])
{
    PKMVERTEX14 points[3];
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD next;

    points[0] = pVertex[0];
    /* @hack spelling: next zeroed here, between the copies of pVertex[0] and
     * pVertex[1], not at its declaration. in 1ST_READ.BIN, this keeps
     * &points[1] in R12 and next in R11 (0x8c6682d8, 0x8c6682e6); in
     * LOADER.BIN, it changes nothing. zeroed at its declaration, the two trade
     * registers, and 48 of 275 words differ */
    next = 0;
    points[1] = pVertex[1];
    points[2] = pVertex[2];
    isp = kmiDeviceExtension.background_isp;
    tsp = kmiDeviceExtension.background_tsp;
    texture = kmiDeviceExtension.background_texture;
    PUT_HEADER(next, isp, tsp, texture);

    PUT_POSITION(next, points[0]);
    next = background_put_uv16(isp, next, points[0]->dwUV0m);
    PACKET[next++] = PACK_INTENSITY(points[0]->fBaseIntensity0m);
    next = background_put_offset(isp, next,
                                 PACK_INTENSITY(points[0]->fOffsetIntensity0m));

    PUT_POSITION(next, points[1]);
    next = background_put_uv16(isp, next, points[1]->dwUV0m);
    PACKET[next++] = PACK_INTENSITY(points[1]->fBaseIntensity0m);
    next = background_put_offset(isp, next,
                                 PACK_INTENSITY(points[1]->fOffsetIntensity0m));

    PUT_POSITION(next, points[2]);
    next = background_put_uv16(isp, next, points[2]->dwUV0m);
    PACKET[next++] = PACK_INTENSITY(points[2]->fBaseIntensity0m);
    next = background_put_offset(isp, next,
                                 PACK_INTENSITY(points[2]->fOffsetIntensity0m));

    END_PACKET(next, isp, points[0]);
    return KMSTATUS_SUCCESS;
}

/* the sprite and modifier volume vertex types cannot form a background */

KMSTATUS kmiProcessVertex_15(void *pVertex[3])
{
    return KMSTATUS_INVALID_VERTEX_TYPE;
}

KMSTATUS kmiProcessVertex_16(void *pVertex[3])
{
    return KMSTATUS_INVALID_VERTEX_TYPE;
}

KMSTATUS kmiProcessVertex_17(void *pVertex[3])
{
    return KMSTATUS_INVALID_VERTEX_TYPE;
}

KMSTATUS (*kmiBackGroundPlaneFuncs[18])(void *pVertex[3]) = {
    kmiProcessVertex_00, kmiProcessVertex_01, kmiProcessVertex_02,
    kmiProcessVertex_03, kmiProcessVertex_04, kmiProcessVertex_05,
    kmiProcessVertex_06, kmiProcessVertex_07, kmiProcessVertex_08,
    kmiProcessVertex_09, kmiProcessVertex_10, kmiProcessVertex_11,
    kmiProcessVertex_12, kmiProcessVertex_13, kmiProcessVertex_14,
    kmiProcessVertex_15, kmiProcessVertex_16, kmiProcessVertex_17};
