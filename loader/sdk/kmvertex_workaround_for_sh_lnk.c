/* SDK 1.20J Kamui's polygon headers. kmProcessVertexRenderState computes a
 * vertex context's four header words. kmSetVertexRenderState makes them the
 * current header. the kmChangeContext functions change one of its modes at a
 * time. the global parameter, kmiGlobalWork, holds the current header in the
 * layout kmStartVertexStrip sends ahead of a strip */
#include <kamui.h>
#include <sg_xpt.h> /* NOLINT(misc-include-cleaner) */

/* the bits of a polygon header's four words, as the tile accelerator reads
 * them. PCW_ marks the parameter control word's bits, ISP_ the ISP word's,
 * TSP_ the TSP word's and TEX_ the texture word's. kamui.h holds TSP_V_SIZE
 * and the TEX_ fields that other sources share */
#define PCW_POLYGON 0x80000000
#define PCW_SPRITE 0xa0000000
#define PCW_TYPE 0xe0000000
#define PCW_SPRITE_BIT 0x20000000
#define PCW_LIST 0x07000000
#define PCW_MODIFIER_LIST 0x01000000
#define PCW_GROUP 0x00800000
#define PCW_STRIP 0x000c0000
#define PCW_CLIP 0x00030000
#define PCW_SHADOW 0x00000080
#define PCW_VOLUMES 0x00000040
#define PCW_COLOR 0x00000030
#define PCW_COLOR_HIGH 0x00000020
#define PCW_COLOR_LOW 0x00000010
#define PCW_TEXTURE 0x00000008
#define PCW_OFFSET 0x00000004
#define PCW_GOURAUD 0x00000002
#define PCW_UV16 0x00000001
/* the control word's bits kmProcessVertexRenderState keeps when it stores the
 * word back in the context */
#define PCW_KEPT 0xe78f00ff

#define ISP_DEPTH 0xe0000000
#define ISP_CULL 0x18000000
#define ISP_NO_WRITE 0x04000000
#define ISP_TEXTURE 0x02000000
#define ISP_OFFSET 0x01000000
#define ISP_GOURAUD 0x00800000
#define ISP_UV16 0x00400000
#define ISP_EXACT_D 0x00100000
/* the ISP word's bits kmProcessVertexRenderState keeps when it stores the word
 * back in the context */
#define ISP_KEPT 0xffd00000

#define TSP_SRC 0xe0000000
#define TSP_DST 0x1c000000
#define TSP_SRC_SELECT 0x02000000
#define TSP_DST_SELECT 0x01000000
#define TSP_FOG 0x00c00000
#define TSP_COLOR_CLAMP 0x00200000
#define TSP_ALPHA 0x00100000
#define TSP_IGNORE_ALPHA 0x00080000
#define TSP_FLIP 0x00060000
#define TSP_CLAMP 0x00018000
#define TSP_FILTER 0x00006000
#define TSP_SUPERSAMPLE 0x00001000
#define TSP_MIPMAP_D 0x00000f00
#define TSP_SHADING 0x000000c0
#define TSP_U_SIZE 0x00000038

/* the blend factors of the TSP word's two 3-bit fields. SRC(factor) shifts a
 * factor into the source's field, and DST(factor) into the destination's. in
 * the source's field, BLEND_OTHER_COLOR is the destination color. in the
 * destination's field, it is the source color */
#define BLEND_ZERO 0
#define BLEND_ONE 1
#define BLEND_OTHER_COLOR 2
#define BLEND_INV_OTHER_COLOR 3
#define BLEND_SRC_ALPHA 4
#define BLEND_INV_SRC_ALPHA 5
#define BLEND_DST_ALPHA 6
#define BLEND_INV_DST_ALPHA 7
#define SRC(factor) ((KMDWORD)(factor) << 29)
#define DST(factor) ((KMDWORD)(factor) << 26)

#define TEX_MIPMAP 0x80000000

/* the video register that sets cheap shadows, as the offset kmiWriteRegister
 * takes. its low byte holds the intensity, and its SHADOW_ON bit turns cheap
 * shadows on */
#define SHADOW_SCALE 0x074
#define SHADOW_ON 0x100

/* the ListType bit of the two modifier volume lists, KM_OPAQUE_MODIFIER and
 * KM_TRANS_MODIFIER */
#define LIST_MODIFIER 1

#define MODIFIER_LAST                                                          \
    (KM_MODIFIER_INCLUDE_LAST_POLY | KM_MODIFIER_EXCLUDE_LAST_POLY)

/* a polygon header's type, which sets the words it holds after its first
 * four. CREATE_GLOBAL_PARAMETER writes them for each type up to HEADER_SPRITE.
 * a header in a modifier list, HEADER_MODIFIER or HEADER_MODIFIER_SPRITE,
 * takes the modifier volume's layout */
enum header_type {
    HEADER_PLAIN = 0,
    HEADER_INTENSITY = 1,
    HEADER_INTENSITY_OFFSET = 2,
    HEADER_TWO_VOLUMES = 3,
    HEADER_TWO_VOLUMES_INTENSITY = 4,
    HEADER_SPRITE = 16,
    HEADER_MODIFIER = 32,
    HEADER_MODIFIER_SPRITE = 255 /* a sprite in a modifier list */
};

struct header_volume {
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD unk_0c;
    KMFLOAT face_alpha;
    KMFLOAT face_red;
    KMFLOAT face_green;
    KMFLOAT face_blue;
    KMFLOAT offset_alpha;
    KMFLOAT offset_red;
    KMFLOAT offset_green;
    KMFLOAT offset_blue;
};

/* a polygon header as Kamui keeps it: the control word, three words nothing
 * uses, then two volumes. a header with two volumes takes volume[1]'s TSP and
 * texture words and face color for its second. a modifier list's header takes
 * volume[1].isp as its instruction word */
struct polygon_header {
    KMDWORD control;
    KMDWORD unk_04[3];
    struct header_volume volume[2];
};

/* the PCW_VOLUMES bit kmProcessVertexRenderState gives a context whose
 * SelectModifier is odd, a polygon a modifier volume touches. 1 gives the
 * polygon two volumes. kmSetCheapShadowMode makes it 0 while cheap shadows are
 * on. the modifier volume then darkens the polygon's one volume */
static KMDWORD two_volumes = 1;
/* how many words kmiPutGlobalParam copies. no source writes it, so it stays
 * 0 */
KMDWORD kmiGlobalNum = 0;
struct kmi_global_parameter kmiGlobalWork = {
    0xffffffff,
    0xffffffff,
    0xffffffff,
    0xffffffff,
    {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
     0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff}};
#define P HEADER_PLAIN
#define I HEADER_INTENSITY
#define IO HEADER_INTENSITY_OFFSET
#define V HEADER_TWO_VOLUMES
#define VI HEADER_TWO_VOLUMES_INTENSITY
#define M HEADER_MODIFIER
#define S HEADER_SPRITE
#define MS HEADER_MODIFIER_SPRITE
/* the header type at each index HEADER_TYPE makes of a control word. the
 * index's top three bits are the sprite, modifier list and two volume bits,
 * and pick a row of eight. its low three bits pick the color type and offset
 * color along the row. the color types run packed, floating, intensity and
 * intensity of the previous face, each without and then with an offset color.
 * a sprite or a header in a modifier list ignores PCW_VOLUMES, so each fills
 * two rows */
static enum header_type header_types[64] = {
    P,  P,  P,  P,  I,  IO, P,  P, /* a polygon */
    V,  V,  V,  V,  VI, VI, V,  V, /* a polygon with two volumes */
    M,  M,  M,  M,  M,  M,  M,  M, /* a modifier volume */
    M,  M,  M,  M,  M,  M,  M,  M,
    S,  S,  S,  S,  S,  S,  S,  S, /* a sprite */
    S,  S,  S,  S,  S,  S,  S,  S,
    MS, MS, MS, MS, MS, MS, MS, MS, /* a sprite in a modifier list */
    MS, MS, MS, MS, MS, MS, MS, MS,
};
#undef P
#undef I
#undef IO
#undef V
#undef VI
#undef M
#undef S
#undef MS
/* the header the polygons sent next draw with */
static struct polygon_header current_header;
/* current_header's type when the global parameter was last built from it */
static KMDWORD current_header_type;

/* f's bits as a KMDWORD. the tile accelerator takes a color channel as a
 * float */
/* @hack type: a float read as its bits through a KMDWORD cast of its address,
 * which SHC loads with one MOV.L (MOV.L @(16,R1),R1 at 0x8ce112d4). read
 * through a union local, each float goes to the frame by FMOV.S and back, the
 * section is 212 bytes longer, and 844 of 882 words differ in
 * kmiCreateGlobalParameter, kmSetVertexRenderState and
 * kmSetModifierRenderState */
#define FLOAT_WORD(f) (*(KMDWORD *)&(f))
/* f, a color channel from 0 to 1, as a byte: f times 255, truncated */
#define CHANNEL(f) ((KMDWORD)(KMINT32)((f) * 255.0F))

/* control's header type, from header_types. the index is control's sprite,
 * modifier list, two volume, color type and offset color bits from high to
 * low */
/* @hack spelling: PCW_COLOR's two bits masked and shifted one at a time; as
 * one ((control) & PCW_COLOR) >> 3, 648 of 882 words differ in
 * kmiCreateGlobalParameter, kmSetVertexRenderState and
 * kmSetModifierRenderState */
#define HEADER_TYPE(control)                                                   \
    header_types[((control) & PCW_SPRITE_BIT) >> 24 |                          \
                 ((control) & PCW_MODIFIER_LIST) >> 20 |                       \
                 ((control) & PCW_VOLUMES) >> 3 |                              \
                 ((control) & PCW_COLOR_HIGH) >> 3 |                           \
                 ((control) & PCW_COLOR_LOW) >> 3 |                            \
                 ((control) & PCW_OFFSET) >> 2]

/* write a volume's face or offset color at out: alpha, red, green and blue, a
 * word each. first and rest name the same volume: alpha is read through first,
 * and the other channels through rest */
/* @hack spelling: two names for one volume. CREATE_GLOBAL_PARAMETER's local
 * volume points at header's first volume. the macro passes *volume as first for
 * HEADER_INTENSITY's and HEADER_INTENSITY_OFFSET's face colors, and reads
 * HEADER_SPRITE's packed face color through volume. with header's first volume
 * read through (header)->volume[0] in every case, 534 of 882 words differ in
 * kmiCreateGlobalParameter, kmSetVertexRenderState and
 * kmSetModifierRenderState */
#define PUT_COLOR(out, first, rest, color)                                     \
    *(out)++ = FLOAT_WORD((first).color##_alpha);                              \
    *(out)++ = FLOAT_WORD((rest).color##_red);                                 \
    *(out)++ = FLOAT_WORD((rest).color##_green);                               \
    *(out)++ = FLOAT_WORD((rest).color##_blue)

/* volume's face or offset color in one word, a byte a channel, alpha
 * highest */
#define PACKED(volume, color)                                                  \
    (CHANNEL((volume).color##_alpha) << 24 |                                   \
     CHANNEL((volume).color##_red) << 16 |                                     \
     CHANNEL((volume).color##_green) << 8 | CHANNEL((volume).color##_blue))

/* write *header into the global parameter at base as its type lays it out.
 * out must point at base's first word. the macro leaves out past the header's
 * last word. *list and kmiCurrentListState.ListType take the header's
 * ListType, current_header_type its type, and GlobalParamSize its size in
 * bytes. a modifier list's header is the control word, volume[1].isp and six
 * words left as they were */
/* @hack spelling: the header, the global parameter and the list type reached
 * through header, base and list, which each function that expands this points
 * at current_header, kmiGlobalWork and kmiCurrentListType. with the globals
 * named in place, 872 of 882 words differ in kmiCreateGlobalParameter,
 * kmSetVertexRenderState and kmSetModifierRenderState */
#define CREATE_GLOBAL_PARAMETER(header, base, out, list)                       \
    current_header_type = HEADER_TYPE((header)->control);                      \
    *(out)++ = (header)->control;                                              \
    *(list) = ((header)->control & PCW_LIST) >> 24;                            \
    if (!(*(list) & LIST_MODIFIER)) {                                          \
        struct header_volume *volume;                                          \
        *(out)++ = (header)->volume[0].isp;                                    \
        *(out)++ = (header)->volume[0].tsp;                                    \
        *(out)++ = (header)->volume[0].texture;                                \
        volume = &(header)->volume[0];                                         \
        switch (current_header_type) {                                         \
        case HEADER_PLAIN:                                                     \
            (out) += 4;                                                        \
            break;                                                             \
        case HEADER_INTENSITY:                                                 \
            PUT_COLOR(out, *volume, (header)->volume[0], face);                \
            break;                                                             \
        case HEADER_INTENSITY_OFFSET:                                          \
            (out) += 4;                                                        \
            PUT_COLOR(out, *volume, (header)->volume[0], face);                \
            PUT_COLOR(out, (header)->volume[0], (header)->volume[0], offset);  \
            break;                                                             \
        case HEADER_TWO_VOLUMES:                                               \
            *(out)++ = (header)->volume[1].tsp;                                \
            *(out)++ = (header)->volume[1].texture;                            \
            (out) += 2;                                                        \
            break;                                                             \
        case HEADER_TWO_VOLUMES_INTENSITY:                                     \
            *(out)++ = (header)->volume[1].tsp;                                \
            *(out)++ = (header)->volume[1].texture;                            \
            (out) += 2;                                                        \
            PUT_COLOR(out, (header)->volume[0], (header)->volume[0], face);    \
            PUT_COLOR(out, (header)->volume[1], (header)->volume[1], face);    \
            break;                                                             \
        case HEADER_SPRITE:                                                    \
            *(out)++ = PACKED(*volume, face);                                  \
            *(out)++ = PACKED((header)->volume[0], offset);                    \
            (out) += 2;                                                        \
            break;                                                             \
        }                                                                      \
    } else {                                                                   \
        *(out) = (header)->volume[1].isp;                                      \
        (out) += 7;                                                            \
    }                                                                          \
    kmiCurrentListState.ListType = *(list);                                    \
    kmiCurrentListState.GlobalParamSize = (char *)(out) - (char *)(base)

KMDWORD kmiConvertUVnum(KMDWORD size)
{
    if (size == 8) {
        size = 0;
    } else if (size == 16) {
        size = 1;
    } else if (size == 32) {
        size = 2;
    } else if (size == 64) {
        size = 3;
    } else if (size == 128) {
        size = 4;
    } else if (size == 256) {
        size = 5;
    } else if (size == 512) {
        size = 6;
    } else if (size == 1024) {
        size = 7;
    }
    return size;
}

void kmiCreateGlobalParameter(void)
{
    struct kmi_global_parameter *base = &kmiGlobalWork;
    /* @hack redundant: register on out, which keeps kmiGlobalWork's address
     * in R6 and the control word in R14 (0x8ce1122e, 0x8ce11238);
     * without it, the two trade, and 59 of 270 words differ */
    register KMDWORD *out = &base->control;
    KMDWORD *list = &kmiCurrentListType;
    struct polygon_header *header = &current_header;

    CREATE_GLOBAL_PARAMETER(header, base, out, list);
}

KMSTATUS kmProcessVertexRenderState(PKMVERTEXCONTEXT pVertexContext)
{
    KMDWORD value;
    KMDWORD changes;
    KMDWORD isp = pVertexContext->ISPPARAMBUFFER;
    KMDWORD tsp = pVertexContext->TSPPARAMBUFFER;
    KMDWORD texture = pVertexContext->TexturePARAMBUFFER;
    KMDWORD control = pVertexContext->GLOBALPARAMBUFFER;
    /* the TSP word's filter code for each FilterMode */
    KMDWORD filter_codes[4] = {0, 2, 1, 3};

    changes = pVertexContext->RenderState;

    if (changes & KM_DEPTHMODE) {
        isp = (isp & ~ISP_DEPTH) | ((KMDWORD)pVertexContext->DepthMode << 29);
    }
    if (changes & KM_CULLINGMODE) {
        isp = (isp & ~ISP_CULL) | ((KMDWORD)pVertexContext->CullingMode << 27);
    }
    if (changes & KM_ZWRITEDISABLE) {
        if (pVertexContext->bZWriteDisable) {
            isp = isp | ISP_NO_WRITE;
        } else {
            isp = isp & ~ISP_NO_WRITE;
        }
    }
    if (changes & KM_SHADINGMODE) {
        isp = (isp & ~ISP_TEXTURE) |
              (((KMDWORD)pVertexContext->ShadingMode >> 1) << 25);
    }
    if (changes & KM_USESPECULAR) {
        isp = (isp & ~ISP_OFFSET) | (pVertexContext->bUseSpecular << 24);
    }
    if (changes & KM_SHADINGMODE) {
        isp = (isp & ~ISP_GOURAUD) |
              (((KMDWORD)pVertexContext->ShadingMode & 1) << 23);
    }
    if (changes & KM_UVFORMAT) {
        isp = (isp & ~ISP_UV16) | ((KMDWORD)pVertexContext->UVFormat << 22);
    }
    if (changes & KM_DCALCEXACT) {
        if (pVertexContext->bDCalcExact) {
            isp = isp | ISP_EXACT_D;
        } else {
            isp = isp & ~ISP_EXACT_D;
        }
    }

    if (changes & KM_SRCBLENDINGMODE) {
        switch (pVertexContext->SRCBlendingMode) {
        case KM_BOTHINVSRCALPHA:
            tsp = (tsp & ~TSP_SRC) | SRC(BLEND_INV_SRC_ALPHA);
            tsp = (tsp & ~TSP_DST) | DST(BLEND_SRC_ALPHA);
            break;
        case KM_BOTHSRCALPHA:
            tsp = (tsp & ~TSP_SRC) | SRC(BLEND_SRC_ALPHA);
            tsp = (tsp & ~TSP_DST) | DST(BLEND_INV_SRC_ALPHA);
            break;
        case KM_DESTALPHA:
            tsp = (tsp & ~TSP_SRC) | SRC(BLEND_DST_ALPHA);
            break;
        case KM_DESTCOLOR:
            tsp = (tsp & ~TSP_SRC) | SRC(BLEND_OTHER_COLOR);
            break;
        case KM_INVDESTALPHA:
            tsp = (tsp & ~TSP_SRC) | SRC(BLEND_INV_DST_ALPHA);
            break;
        case KM_INVDESTCOLOR:
            tsp = (tsp & ~TSP_SRC) | SRC(BLEND_INV_OTHER_COLOR);
            break;
        case KM_INVSRCALPHA:
            tsp = (tsp & ~TSP_SRC) | SRC(BLEND_INV_SRC_ALPHA);
            break;
        case KM_SRCALPHA:
            tsp = (tsp & ~TSP_SRC) | SRC(BLEND_SRC_ALPHA);
            break;
        case KM_ONE:
            tsp = (tsp & ~TSP_SRC) | SRC(BLEND_ONE);
            break;
        case KM_ZERO:
            tsp = (tsp & ~TSP_SRC) | SRC(BLEND_ZERO);
            break;
        }
    }
    if (changes & KM_DSTBLENDINGMODE) {
        switch (pVertexContext->DSTBlendingMode) {
        case KM_BOTHINVSRCALPHA:
            tsp = (tsp & ~TSP_SRC) | SRC(BLEND_INV_SRC_ALPHA);
            tsp = (tsp & ~TSP_DST) | DST(BLEND_SRC_ALPHA);
            break;
        case KM_BOTHSRCALPHA:
            tsp = (tsp & ~TSP_SRC) | SRC(BLEND_SRC_ALPHA);
            tsp = (tsp & ~TSP_DST) | DST(BLEND_INV_SRC_ALPHA);
            break;
        case KM_DESTALPHA:
            tsp = (tsp & ~TSP_DST) | DST(BLEND_DST_ALPHA);
            break;
        case KM_INVDESTALPHA:
            tsp = (tsp & ~TSP_DST) | DST(BLEND_INV_DST_ALPHA);
            break;
        case KM_INVSRCALPHA:
            tsp = (tsp & ~TSP_DST) | DST(BLEND_INV_SRC_ALPHA);
            break;
        case KM_INVSRCCOLOR:
            tsp = (tsp & ~TSP_DST) | DST(BLEND_INV_OTHER_COLOR);
            break;
        case KM_SRCALPHA:
            tsp = (tsp & ~TSP_DST) | DST(BLEND_SRC_ALPHA);
            break;
        case KM_SRCCOLOR:
            tsp = (tsp & ~TSP_DST) | DST(BLEND_OTHER_COLOR);
            break;
        case KM_ONE:
            tsp = (tsp & ~TSP_DST) | DST(BLEND_ONE);
            break;
        case KM_ZERO:
            tsp = (tsp & ~TSP_DST) | DST(BLEND_ZERO);
            break;
        }
    }
    if (changes & KM_SRCSELECT) {
        if (pVertexContext->bSRCSel) {
            tsp = tsp | TSP_SRC_SELECT;
        } else {
            tsp = tsp & ~TSP_SRC_SELECT;
        }
    }
    if (changes & KM_DSTSELECT) {
        if (pVertexContext->bDSTSel) {
            tsp = tsp | TSP_DST_SELECT;
        } else {
            tsp = tsp & ~TSP_DST_SELECT;
        }
    }
    if (changes & KM_FOGMODE) {
        value = pVertexContext->FogMode & 3;
        tsp = (tsp & ~TSP_FOG) | (value << 22);
    }
    if (changes & KM_COLORCLAMP) {
        if (pVertexContext->bColorClamp) {
            tsp = tsp | TSP_COLOR_CLAMP;
        } else {
            tsp = tsp & ~TSP_COLOR_CLAMP;
        }
    }
    if (changes & KM_USEALPHA) {
        if (pVertexContext->bUseAlpha) {
            tsp = tsp | TSP_ALPHA;
        } else {
            tsp = tsp & ~TSP_ALPHA;
        }
    }
    if (changes & KM_IGNORETEXTUREALPHA) {
        if (pVertexContext->bIgnoreTextureAlpha) {
            tsp = tsp | TSP_IGNORE_ALPHA;
        } else {
            tsp = tsp & ~TSP_IGNORE_ALPHA;
        }
    }
    if (changes & KM_CLAMPUV) {
        tsp = (tsp & ~TSP_CLAMP) | ((KMDWORD)pVertexContext->ClampUV << 15);
    }
    if (changes & KM_FLIPUV) {
        tsp = (tsp & ~TSP_FLIP) | (pVertexContext->FlipUV << 17);
    }
    if (changes & KM_FILTERMODE) {
        /* @hack spelling: the mode, then its entry in filter_codes, through
         * value in two steps; in one, the frame's slots and registers differ
         * from 0x8ce11496 (MOV.L R3,@R15) on, and 446 of 670 words differ */
        value = pVertexContext->FilterMode;
        value = filter_codes[value];
        tsp = (tsp & ~TSP_FILTER) | (value << 13);
    }
    if (changes & KM_SUPERSAMPLE) {
        if (pVertexContext->bSuperSample) {
            tsp = tsp | TSP_SUPERSAMPLE;
        } else {
            tsp = tsp & ~TSP_SUPERSAMPLE;
        }
    }
    if (changes & KM_MIPMAPDADJUST) {
        /* a MipMapAdjust whose low four bits are 0 is taken as
         * KM_MIPMAP_D_ADJUST_1_00 */
        value = pVertexContext->MipMapAdjust & 15;
        if (value == 0) {
            value = KM_MIPMAP_D_ADJUST_1_00;
        }
        tsp = (tsp & ~TSP_MIPMAP_D) | (value << 8);
    }
    if (changes & KM_TEXTURESHADINGMODE) {
        value = (pVertexContext->TextureShadingMode & 3) << 6;
        tsp = (tsp & ~TSP_SHADING) | value;
    }
    if ((pVertexContext->ShadingMode >> 1) == 1 &&
        pVertexContext->pTextureSurfaceDesc != NULL) {
        value = (tsp & ~TSP_U_SIZE) |
                (kmiConvertUVnum(pVertexContext->pTextureSurfaceDesc->u0.Width)
                 << 3);
        tsp = (value & ~TSP_V_SIZE) |
              kmiConvertUVnum(pVertexContext->pTextureSurfaceDesc->u1.Height);
    }

    if ((pVertexContext->ShadingMode >> 1) == 1 &&
        pVertexContext->pTextureSurfaceDesc != NULL) {
        texture = (texture & ~TEX_MIPMAP) |
                  ((pVertexContext->pTextureSurfaceDesc->fSurfaceFlags &
                    KM_SURFACEFLAGS_MIPMAPED)
                   << 31);
        texture = (texture & ~TEX_VQ) |
                  ((pVertexContext->pTextureSurfaceDesc->fSurfaceFlags &
                    KM_SURFACEFLAGS_VQ)
                   << 27);
        texture =
            (texture & ~TEX_FORMAT) |
            (pVertexContext->pTextureSurfaceDesc->PixelFormat & TEX_FORMAT);
        if ((texture & TEX_FORMAT) == KM_PIXELFORMAT_PALETTIZED_4BPP ||
            (texture & TEX_FORMAT) == KM_PIXELFORMAT_PALETTIZED_8BPP) {
            texture = (texture & ~TEX_PALETTE) |
                      ((pVertexContext->PaletteBank & 63) << TEX_PALETTE_SHIFT);
        } else {
            texture = (texture & ~TEX_NOT_TWIDDLED) |
                      ((~(pVertexContext->pTextureSurfaceDesc->fSurfaceFlags &
                          KM_SURFACEFLAGS_TWIDDLED) &
                        KM_SURFACEFLAGS_TWIDDLED)
                       << 24);
            texture = (texture & ~TEX_STRIDE) |
                      ((pVertexContext->pTextureSurfaceDesc->fSurfaceFlags &
                        KM_SURFACEFLAGS_STRIDE)
                       << 21);
        }
        texture = (texture & ~TEX_ADDRESS) |
                  ((KMDWORD)pVertexContext->pTextureSurfaceDesc->pSurface >> 3);
    }

    if (changes & KM_PARAMTYPE) {
        value = pVertexContext->ParamType;
        if (value == KM_POLYGON) {
            control = (control & ~PCW_TYPE) | PCW_POLYGON;
        }
        if (value == KM_MODIFIERVOLUME) {
            control = (control & ~PCW_TYPE) | PCW_POLYGON;
        }
        if (value == KM_SPRITE) {
            control = (control & ~PCW_TYPE) | PCW_SPRITE;
        }
    }
    if (changes & KM_LISTTYPE) {
        control = (control & ~PCW_LIST) | (pVertexContext->ListType << 24);
    }
    control = control | PCW_GROUP;
    if (changes & KM_STRIPLENGTH) {
        control = (control & ~PCW_STRIP) | (pVertexContext->StripLength << 18);
    } else {
        control =
            (control & ~PCW_STRIP) | (kmiDeviceExtension.strip_length << 18);
    }
    if (changes & KM_USERCLIPMODE) {
        control = (control & ~PCW_CLIP) | (pVertexContext->UserClipMode << 16);
    }
    /* every call clears PCW_SHADOW and PCW_VOLUMES. a modifier volume's last
     * polygon, in a modifier list, then sets PCW_VOLUMES */
    if ((pVertexContext->ModifierInstruction & MODIFIER_LAST) &&
        (pVertexContext->ListType & LIST_MODIFIER)) {
        control = (control & ~(PCW_SHADOW | PCW_VOLUMES)) | PCW_VOLUMES;
    } else {
        control = control & ~(PCW_SHADOW | PCW_VOLUMES);
    }
    /* a polygon a modifier volume touches, SelectModifier odd, sets PCW_SHADOW
     * and sets PCW_VOLUMES to two_volumes */
    if (changes & KM_MODIFIER) {
        if (pVertexContext->SelectModifier & 1) {
            control = (control & ~PCW_SHADOW) |
                      ((KMDWORD)pVertexContext->SelectModifier << 7);
            control = (control & ~PCW_VOLUMES) | (two_volumes << 6);
        }
    }
    if (changes & KM_COLORTYPE) {
        control = (control & ~PCW_COLOR) | (pVertexContext->ColorType << 4);
    }
    if (changes & KM_SHADINGMODE) {
        control = (control & ~PCW_TEXTURE) |
                  (((KMDWORD)pVertexContext->ShadingMode >> 1) << 3);
    }
    if (changes & KM_SHADINGMODE) {
        control = (control & ~PCW_GOURAUD) |
                  (((KMDWORD)pVertexContext->ShadingMode & 1) << 1);
    }
    if (changes & KM_USESPECULAR) {
        if (pVertexContext->bUseSpecular) {
            control |= PCW_OFFSET;
        } else {
            control &= ~PCW_OFFSET;
        }
    }
    if (changes & KM_UVFORMAT) {
        control = (control & ~PCW_UV16) | pVertexContext->UVFormat;
    }

    value = control & PCW_KEPT;
    pVertexContext->ISPPARAMBUFFER = isp & ISP_KEPT;
    pVertexContext->TSPPARAMBUFFER = tsp;
    pVertexContext->TexturePARAMBUFFER = texture;
    pVertexContext->GLOBALPARAMBUFFER = value;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetVertexRenderState(PKMVERTEXCONTEXT pVertexContext)
{
    struct kmi_global_parameter *base = &kmiGlobalWork;
    /* @hack redundant: register on out, which keeps current_header's
     * address in R14 and saves R13 (0x8ce11b06, 0x8ce11b0c); without it, the
     * address takes R5, a frame slot takes R13's place, the function runs 8
     * bytes long and 288 of 315 words differ */
    register KMDWORD *out;
    KMDWORD *list = &kmiCurrentListType;
    struct polygon_header *header = &current_header;

    header->control = pVertexContext->GLOBALPARAMBUFFER;
    header->volume[0].isp = pVertexContext->ISPPARAMBUFFER;
    header->volume[0].tsp = pVertexContext->TSPPARAMBUFFER;
    if (((header->control >> 24) & (PCW_LIST >> 24)) == KM_OPAQUE_POLYGON) {
        header->volume[0].tsp = (header->volume[0].tsp & ~(TSP_SRC | TSP_DST)) |
                                (SRC(BLEND_ONE) | DST(BLEND_ZERO));
    }
    header->volume[0].texture = pVertexContext->TexturePARAMBUFFER;
    header->volume[0].face_alpha = pVertexContext->fFaceColorAlpha;
    header->volume[0].face_red = pVertexContext->fFaceColorRed;
    header->volume[0].face_green = pVertexContext->fFaceColorGreen;
    header->volume[0].face_blue = pVertexContext->fFaceColorBlue;
    header->volume[0].offset_alpha = pVertexContext->fOffsetColorAlpha;
    header->volume[0].offset_red = pVertexContext->fOffsetColorRed;
    header->volume[0].offset_green = pVertexContext->fOffsetColorGreen;
    header->volume[0].offset_blue = pVertexContext->fOffsetColorBlue;
    header->volume[1].isp = pVertexContext->ModifierInstruction;
    out = &base->control;
    CREATE_GLOBAL_PARAMETER(header, base, out, list);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetModifierRenderState(PKMVERTEXCONTEXT pVertexContext)
{
    struct kmi_global_parameter *base = &kmiGlobalWork;
    /* @hack redundant: register on out, which keeps kmiGlobalWork's address
     * in R6 and the control word in R14 (0x8ce11dae, 0x8ce11de8);
     * without it, they take R14 and R7, and 65 of 297 words differ */
    register KMDWORD *out;
    KMDWORD *list = &kmiCurrentListType;
    struct polygon_header *header = &current_header;

    header->volume[1].tsp = pVertexContext->TSPPARAMBUFFER;
    header->volume[1].texture = pVertexContext->TexturePARAMBUFFER;
    header->volume[1].face_alpha = pVertexContext->fFaceColorAlpha;
    header->volume[1].face_red = pVertexContext->fFaceColorRed;
    header->volume[1].face_green = pVertexContext->fFaceColorGreen;
    header->volume[1].face_blue = pVertexContext->fFaceColorBlue;
    out = &base->control;
    CREATE_GLOBAL_PARAMETER(header, base, out, list);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetStripLength(KMINT32 StripLength)
{
    kmiDeviceExtension.strip_length = StripLength;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetCheapShadowMode(KMINT32 nIntensity)
{
    KMDWORD value;

    if (nIntensity < 0) {
        two_volumes = 1;
    } else {
        two_volumes = 0;
    }
    value = ((1 - two_volumes) * SHADOW_ON) | (KMBYTE)nIntensity;
    kmiWriteRegister(SHADOW_SCALE, value);
    return KMSTATUS_SUCCESS;
}

PKMDWORD *kmiPutGlobalParam(void)
{
    kmiDstVertexBuffer = kmiCurrentVertexBuff + kmiCurrentListType;
    kmiMemCopy8(*kmiDstVertexBuffer, (PKMDWORD)&kmiGlobalWork,
                kmiGlobalNum * 4);
    *kmiDstVertexBuffer += kmiGlobalNum;
    return kmiDstVertexBuffer;
}

KMSTATUS kmChangeContextColorType(KMINT32 Color)
{
    current_header.control =
        (current_header.control & ~PCW_COLOR) | (Color << 4);
    /* @hack spelling: the word stored through kmiGlobalWork's address (MOV.L
     * R2,@R1 at 0x8ce120a0); as kmiGlobalWork.control, 5 of 12 words differ */
    (&kmiGlobalWork)->control = current_header.control;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeContextDepthMode(KMINT32 DepthMode)
{
    current_header.volume[0].isp =
        (current_header.volume[0].isp & ~ISP_DEPTH) | (DepthMode << 29);
    /* @hack spelling: the word stored through kmiGlobalWork's address
     * (MOV.L R3,@(4,R0) at 0x8ce120be); as kmiGlobalWork.isp, 10 of 15 words
     * differ */
    (&kmiGlobalWork)->isp = current_header.volume[0].isp;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeContextCullingMode(KMINT32 CullingMode)
{
    current_header.volume[0].isp =
        (current_header.volume[0].isp & ~ISP_CULL) | (CullingMode << 27);
    /* @hack spelling: the word stored through kmiGlobalWork's address
     * (MOV.L R3,@(4,R0) at 0x8ce120dc); as kmiGlobalWork.isp, 9 of 15 words
     * differ */
    (&kmiGlobalWork)->isp = current_header.volume[0].isp;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeContextZWriteDisable(KMBOOLEAN fZWriteDisable)
{
    if (fZWriteDisable) {
        current_header.volume[0].isp =
            current_header.volume[0].isp | ISP_NO_WRITE;
    } else {
        current_header.volume[0].isp =
            current_header.volume[0].isp & ~ISP_NO_WRITE;
    }
    /* @hack spelling: the word stored through kmiGlobalWork's address
     * (MOV.L R2,@(4,R3) at 0x8ce12104); as kmiGlobalWork.isp, 3 of 20 words
     * differ */
    (&kmiGlobalWork)->isp = current_header.volume[0].isp;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeContextSRCBlendMode(KMINT32 SRCBlend)
{
    switch (SRCBlend) {
    case KM_BOTHINVSRCALPHA:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_SRC) |
            SRC(BLEND_INV_SRC_ALPHA);
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_DST) | DST(BLEND_SRC_ALPHA);
        break;
    case KM_BOTHSRCALPHA:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_SRC) | SRC(BLEND_SRC_ALPHA);
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_DST) |
            DST(BLEND_INV_SRC_ALPHA);
        break;
    case KM_DESTALPHA:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_SRC) | SRC(BLEND_DST_ALPHA);
        break;
    case KM_DESTCOLOR:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_SRC) | SRC(BLEND_OTHER_COLOR);
        break;
    case KM_INVDESTALPHA:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_SRC) |
            SRC(BLEND_INV_DST_ALPHA);
        break;
    case KM_INVDESTCOLOR:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_SRC) |
            SRC(BLEND_INV_OTHER_COLOR);
        break;
    case KM_INVSRCALPHA:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_SRC) |
            SRC(BLEND_INV_SRC_ALPHA);
        break;
    case KM_SRCALPHA:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_SRC) | SRC(BLEND_SRC_ALPHA);
        break;
    case KM_ONE:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_SRC) | SRC(BLEND_ONE);
        break;
    case KM_ZERO:
        current_header.volume[0].tsp = current_header.volume[0].tsp & ~TSP_SRC;
        break;
    default:
        break;
    }
    /* @hack spelling: the word stored through kmiGlobalWork's address
     * (MOV.L R2,@(8,R3) at 0x8ce1222c); as kmiGlobalWork.tsp, 3 of 99 words
     * differ */
    (&kmiGlobalWork)->tsp = current_header.volume[0].tsp;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeContextDSTBlendMode(KMINT32 DSTBlend)
{
    switch (DSTBlend) {
    case KM_BOTHINVSRCALPHA:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_SRC) |
            SRC(BLEND_INV_SRC_ALPHA);
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_DST) | DST(BLEND_SRC_ALPHA);
        break;
    case KM_BOTHSRCALPHA:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_SRC) | SRC(BLEND_SRC_ALPHA);
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_DST) |
            DST(BLEND_INV_SRC_ALPHA);
        break;
    case KM_DESTALPHA:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_DST) | DST(BLEND_DST_ALPHA);
        break;
    case KM_INVDESTALPHA:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_DST) |
            DST(BLEND_INV_DST_ALPHA);
        break;
    case KM_INVSRCALPHA:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_DST) |
            DST(BLEND_INV_SRC_ALPHA);
        break;
    case KM_INVSRCCOLOR:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_DST) |
            DST(BLEND_INV_OTHER_COLOR);
        break;
    case KM_SRCALPHA:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_DST) | DST(BLEND_SRC_ALPHA);
        break;
    case KM_SRCCOLOR:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_DST) | DST(BLEND_OTHER_COLOR);
        break;
    case KM_ONE:
        current_header.volume[0].tsp =
            (current_header.volume[0].tsp & ~TSP_DST) | DST(BLEND_ONE);
        break;
    case KM_ZERO:
        current_header.volume[0].tsp = current_header.volume[0].tsp & ~TSP_DST;
        break;
    default:
        break;
    }
    /* @hack spelling: the word stored through kmiGlobalWork's address
     * (MOV.L R3,@(8,R2) at 0x8ce122f2); as kmiGlobalWork.tsp, 3 of 99 words
     * differ */
    (&kmiGlobalWork)->tsp = current_header.volume[0].tsp;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeContextFogMode(KMINT32 FogMode)
{
    current_header.volume[0].tsp =
        (current_header.volume[0].tsp & ~TSP_FOG) | ((KMDWORD)FogMode << 22);
    /* @hack spelling: the word stored through kmiGlobalWork's address
     * (MOV.L R3,@(8,R0) at 0x8ce12310); as kmiGlobalWork.tsp, 9 of 15 words
     * differ */
    (&kmiGlobalWork)->tsp = current_header.volume[0].tsp;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeContextFlipUV(KMINT32 FlipMode)
{
    current_header.volume[0].tsp =
        (current_header.volume[0].tsp & ~TSP_FLIP) | (FlipMode << 17);
    /* @hack spelling: the word stored through kmiGlobalWork's address
     * (MOV.L R2,@(8,R1) at 0x8ce1232e); as kmiGlobalWork.tsp, 3 of 15 words
     * differ */
    (&kmiGlobalWork)->tsp = current_header.volume[0].tsp;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeContextClampUV(KMINT32 ClampMode)
{
    current_header.volume[0].tsp =
        (current_header.volume[0].tsp & ~TSP_CLAMP) | (ClampMode << 15);
    /* @hack spelling: the word stored through kmiGlobalWork's address
     * (MOV.L R3,@(8,R0) at 0x8ce1234c); as kmiGlobalWork.tsp, 9 of 15 words
     * differ */
    (&kmiGlobalWork)->tsp = current_header.volume[0].tsp;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeContextFilterMode(KMINT32 FilterMode)
{
    /* the TSP word's filter code for each FilterMode */
    KMDWORD filter_codes[4] = {0, 2, 1, 3};

    FilterMode = filter_codes[FilterMode];
    current_header.volume[0].tsp =
        (current_header.volume[0].tsp & ~TSP_FILTER) |
        ((KMDWORD)FilterMode << 13);
    /* @hack spelling: the word stored through kmiGlobalWork's address
     * (MOV.L R3,@(8,R2) at 0x8ce123cc); as kmiGlobalWork.tsp, 9 of 29 words
     * differ */
    (&kmiGlobalWork)->tsp = current_header.volume[0].tsp;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeContextSuperSample(KMBOOLEAN fEnable)
{
    if (fEnable) {
        current_header.volume[0].tsp =
            current_header.volume[0].tsp | TSP_SUPERSAMPLE;
    } else {
        current_header.volume[0].tsp =
            current_header.volume[0].tsp & ~TSP_SUPERSAMPLE;
    }
    /* @hack spelling: the word stored through kmiGlobalWork's address
     * (MOV.L R2,@(8,R3) at 0x8ce123f8); as kmiGlobalWork.tsp, 3 of 20 words
     * differ */
    (&kmiGlobalWork)->tsp = current_header.volume[0].tsp;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeContextTextureShadingMode(KMINT32 TexShadingmode)
{
    current_header.volume[0].tsp =
        (current_header.volume[0].tsp & ~TSP_SHADING) | (TexShadingmode << 6);
    /* @hack spelling: the word stored through kmiGlobalWork's address
     * (MOV.L R2,@(8,R1) at 0x8ce12418); as kmiGlobalWork.tsp, 3 of 16 words
     * differ */
    (&kmiGlobalWork)->tsp = current_header.volume[0].tsp;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeContextColorClamp(KMBOOLEAN fColorClamp)
{
    if (fColorClamp) {
        current_header.volume[0].tsp =
            current_header.volume[0].tsp | TSP_COLOR_CLAMP;
    } else {
        current_header.volume[0].tsp =
            current_header.volume[0].tsp & ~TSP_COLOR_CLAMP;
    }
    /* @hack spelling: the word stored through kmiGlobalWork's address
     * (MOV.L R2,@(8,R3) at 0x8ce12440); as kmiGlobalWork.tsp, 3 of 20 words
     * differ */
    (&kmiGlobalWork)->tsp = current_header.volume[0].tsp;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeContextPaletteBank(KMDWORD Bank)
{
    if ((current_header.volume[0].texture & TEX_FORMAT) ==
            KM_PIXELFORMAT_PALETTIZED_4BPP ||
        (current_header.volume[0].texture & TEX_FORMAT) ==
            KM_PIXELFORMAT_PALETTIZED_8BPP) {
        current_header.volume[0].texture =
            (current_header.volume[0].texture & ~TEX_PALETTE) |
            ((Bank & 63) << TEX_PALETTE_SHIFT);
        /* @hack spelling: the word stored through kmiGlobalWork's address
         * (MOV.L R2,@(12,R3) at 0x8ce12476); as kmiGlobalWork.texture, 6 of 28
         * words differ */
        (&kmiGlobalWork)->texture = current_header.volume[0].texture;
    }
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeContextStripLength(KMINT32 StripLength)
{
    current_header.control =
        (current_header.control & ~PCW_STRIP) | (StripLength << 18) | PCW_GROUP;
    /* @hack spelling: the word stored through kmiGlobalWork's address (MOV.L
     * R2,@R0 at 0x8ce12496); as kmiGlobalWork.control, 13 of 15 words differ */
    (&kmiGlobalWork)->control = current_header.control;
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmChangeContextUserClipMode(KMINT32 UserClipMode)
{
    current_header.control =
        (current_header.control & ~PCW_CLIP) | (UserClipMode << 16) | PCW_GROUP;
    /* @hack spelling: the word stored through kmiGlobalWork's address (MOV.L
     * R2,@R0 at 0x8ce124b2); as kmiGlobalWork.control, 11 of 14 words differ */
    (&kmiGlobalWork)->control = current_header.control;
    return KMSTATUS_SUCCESS;
}
