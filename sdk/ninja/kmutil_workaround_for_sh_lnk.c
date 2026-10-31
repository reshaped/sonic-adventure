/* Kamui's utilities: twiddled textures and their mipmaps, a frame buffer's
 * pixels as a BMP's, and pass tables, which hold a vertex context for each
 * pass a draw takes */
#include <kamui.h>
#include <sg_xpt.h>

/* a texel of a KM_TEXTURE_BMP texture, which as a little-endian word is
 * 0xRRGGBBAA */
struct texel {
    KMBYTE alpha;
    KMBYTE blue;
    KMBYTE green;
    KMBYTE red;
};

/* a column's channels, each summed down four rows weighted 1, 2, 2 and 1 */
struct channels {
    int red;
    int green;
    int blue;
    int alpha;
};

static KMDWORD pass_target = KMU_TARGET_CLX2;

/* the byte offset in a twiddled mipmap of each level, by the log2 of its side.
 * the 1 by 1 level starts 2 bytes in, and each larger level follows the one
 * before, at 2 bytes a texel */
static const int mip_offset[11] = {
    2, 4, 12, 44, 172, 684, 2732, 10924, 43692, 174764, 699052,
};

/* each check_ function tests table[pass] for one thing the target can't draw.
 * it returns KMU_PASS_OK, or an error with *pPass set to the pass at fault,
 * counted from 1 */
static KMUPASSSTATUS check_param_type(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                      PKMUINT32 pPass);
static KMUPASSSTATUS check_list_type(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                     PKMUINT32 pPass);
static KMUPASSSTATUS check_mipmap(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                  PKMUINT32 pPass);
static KMUPASSSTATUS check_fog(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                               PKMUINT32 pPass);
static KMUPASSSTATUS check_filter(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                  PKMUINT32 pPass);
static KMUPASSSTATUS check_texture_shading(PPKMVERTEXCONTEXT table,
                                           KMUINT32 pass, PKMUINT32 pPass);
static KMUPASSSTATUS check_color_type(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                      PKMUINT32 pPass);
static KMUPASSSTATUS check_shading(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                   PKMUINT32 pPass);
static KMUPASSSTATUS check_user_clip(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                     PKMUINT32 pPass);
static KMUPASSSTATUS check_trilinear(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                     PKMUINT32 pPass);
static KMUPASSSTATUS check_sprite(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                  PKMUINT32 pPass);
static KMUPASSSTATUS check_blending(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                    PKMUINT32 pPass);
static KMUPASSSTATUS check_modifier(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                    PKMUINT32 pPass);
static KMUINT32 trilinear_passes(PKMVERTEXCONTEXT context);
static void trilinear_fill(PPKMVERTEXCONTEXT table);
static int mip_level(int width, int height);
static void make_mipmaps(PKMDWORD output, PKMDWORD input, KMBOOLEAN dither,
                         int size, KMDWORD format);
static int quantize_3(int value);
static int error_6(int value);
static int error_5(int value);
static int error_4(int value);
static void dither_texels(struct texel *texels, int width, int height,
                          KMDWORD format);
static void twiddle(KMWORD *output, struct texel *input, int size,
                    KMDWORD format);
static void filter_row(struct texel *row0, struct texel *row1,
                       struct texel *row2, struct texel *row3,
                       struct texel *output, int width);
static void halve(struct texel *texels, int width, int height);

KMSTATUS kmuCreateTwiddledTexture(PKMDWORD pOutputTexture,
                                  PKMDWORD pInputTexture, KMBOOLEAN bAutoMipMap,
                                  KMBOOLEAN bDither, KMINT32 USize,
                                  KMTEXTURETYPE nTextureType)
{
    KMDWORD format;

    if (USize < KM_MAPSIZE_8 || USize > KM_MAPSIZE_1024) {
        return KMSTATUS_INVALID_TEXTURE_TYPE;
    }
    nTextureType = (KMBYTE)nTextureType;
    switch (nTextureType) {
    case KM_TEXTURE_ARGB1555:
        format = KM_PIXELFORMAT_ARGB1555;
        break;
    case KM_TEXTURE_RGB565:
        format = KM_PIXELFORMAT_RGB565;
        break;
    case KM_TEXTURE_ARGB4444:
        format = KM_PIXELFORMAT_ARGB4444;
        break;
    default:
        return KMSTATUS_INVALID_TEXTURE_TYPE;
    }
    if (bAutoMipMap == TRUE) {
        make_mipmaps(pOutputTexture, pInputTexture, bDither, USize, format);
    } else {
        if (bDither == TRUE) {
            dither_texels((struct texel *)pInputTexture, USize, USize, format);
        }
        twiddle((KMWORD *)pOutputTexture, (struct texel *)pInputTexture, USize,
                format);
    }
    return KMSTATUS_SUCCESS;
}

/* write nWidth by nHeight 16-bit pixels in nBpp's format as a BMP's pixel
 * rows: bottom row first, three bytes a pixel, blue first, and no padding
 * after a row. each channel's bits go to the top of its byte */
KMSTATUS kmuConvertFBtoBMP(PKMDWORD pOutputData, PKMDWORD pInputData,
                           KMINT32 nWidth, KMINT32 nHeight, KMBPPMODE nBpp)
{
    KMBYTE *out = (KMBYTE *)pOutputData;
    KMWORD *in;
    int x;
    int y;
    /* @hack redundant: a, which no line reads and SHC drops, keeps the masks
     * 0x7c00 and 0x03e0 in R9 and R10 (0x8c66bcac, 0x8c66bcb0); without it,
     * 10 of 99 words differ */
    /* NOLINTNEXTLINE(clang-diagnostic-unused-but-set-variable) */
    KMBYTE a;
    KMBYTE r;
    KMBYTE g;
    KMBYTE b;

#ifdef GCC
    /* repairs the unset r, g and b the tag below names: for an nBpp the
     * switch doesn't name, GCC writes black pixels */
    r = 0;
    g = 0;
    b = 0;
#endif
    for (y = nHeight - 1; y >= 0; --y) {
        in = (KMWORD *)pInputData + (y * nWidth);
        for (x = 0; x < nWidth; ++x, ++in) {
            /* NOLINTNEXTLINE(clang-diagnostic-switch) */
            switch (nBpp) {
            case KM_DSPBPP_RGB565:
                a = 0xff;
                r = (*in & 0xf800) >> 8;
                g = (*in & 0x07e0) >> 3;
                b = (*in & 0x001f) << 3;
                break;
            case KM_DSPBPP_RGB555:
                a = 0xff;
                r = (*in & 0x7c00) >> 7;
                g = (*in & 0x03e0) >> 2;
                b = (*in & 0x001f) << 3;
                break;
            case KM_DSPBPP_ARGB1555:
                a = (*in & 0x8000) >> 8;
                r = (*in & 0x7c00) >> 7;
                g = (*in & 0x03e0) >> 2;
                b = (*in & 0x001f) << 3;
                break;
            }
            /* @bug uninitialized: for an nBpp the switch doesn't name, no
             * case sets r, g or b (BRA at 0x8c66bcda), and each pixel takes
             * what their registers hold. b takes the low byte of R12 and g
             * that of R13, both as they were at the call, and r that of R7,
             * which holds nHeight - 1 (MOV.B R12,@R14 at 0x8c66bd36, MOV R13,R0
             * at 0x8c66bd38, MOV R7,R0 at 0x8c66bd3e) */
            out[0] = b;
            out[1] = g;
            out[2] = r;
            out += 3;
        }
    }
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmuSetTarget(KMDWORD dwTarget)
{
    pass_target = dwTarget;
    return KMSTATUS_SUCCESS;
}

/* on the CLX chips, copy pVertexContext into the contexts that the table's
 * first entries point to, one for each pass it draws in, and set the passes
 * after the first for their parts, as trilinear_fill does. on the ARC1 the
 * call does nothing and returns KMU_PASS_ERROR_NOT_IMPLEMENTED, and on any
 * other target it does nothing and returns KMU_PASS_OK.
 *
 * a null pVertexContext returns KMU_PASS_ERROR_VERTEXCONTEXT. otherwise *pPass
 * gets the pass count, and a null table returns KMU_PASS_OK there. an
 * nNumContext below the pass count returns KMU_PASS_ERROR_VERTEXCONTEXT_PASS,
 * with nothing copied. a call that copies reads the table's first three
 * entries, whatever the pass count */
KMUPASSSTATUS kmuGeneratePassTable(PKMVERTEXCONTEXT pVertexContext,
                                   KMUINT32 nNumContext,
                                   PPKMVERTEXCONTEXT ppVertexContextTable,
                                   PKMUINT32 pPass)
{
    KMUINT32 i;

    if (pass_target == KMU_TARGET_CLX1 || pass_target == KMU_TARGET_CLX2) {
        if (pVertexContext == NULL) {
            return KMU_PASS_ERROR_VERTEXCONTEXT;
        }
        *pPass = trilinear_passes(pVertexContext);
        if (!ppVertexContextTable) {
            return KMU_PASS_OK;
        }
        if (nNumContext < *pPass) {
            return KMU_PASS_ERROR_VERTEXCONTEXT_PASS;
        }
        for (i = 0; i < *pPass; ++i) {
            *ppVertexContextTable[i] = *pVertexContext;
        }
        trilinear_fill(ppVertexContextTable);
    }
    if (pass_target == KMU_TARGET_ARC1) {
        return KMU_PASS_ERROR_NOT_IMPLEMENTED;
    }
    return KMU_PASS_OK;
}

/* check the table's nNumContext passes in order for what the target can't
 * draw, and return the first failure's error, or KMU_PASS_OK. *pPass gets the
 * pass at fault, counted from 1. a null entry fails as
 * KMU_PASS_ERROR_VERTEXCONTEXT with *pPass its index, counted from 0, and an
 * empty table as KMU_PASS_ERROR_VERTEXCONTEXT_PASS with *pPass 0. on the CLX
 * chips the check can read the two entries past the table's end */
KMUPASSSTATUS kmuCheckPassTable(PPKMVERTEXCONTEXT ppVertexContextTable,
                                KMUINT32 nNumContext, PKMUINT32 pPass)
{
    KMUINT32 i;
    KMUPASSSTATUS status;

    if (nNumContext < 1) {
        *pPass = 0;
        return KMU_PASS_ERROR_VERTEXCONTEXT_PASS;
    }
    for (i = 0; i < nNumContext; ++i) {
        if (ppVertexContextTable[i]) {
            status = check_param_type(ppVertexContextTable, i, pPass);
            if (status) {
                goto done;
            }
            status = check_list_type(ppVertexContextTable, i, pPass);
            if (status) {
                goto done;
            }
            status = check_mipmap(ppVertexContextTable, i, pPass);
            if (status) {
                goto done;
            }
            {
                /* @hack spelling: the target read through a pointer, which
                 * loads pass_target's address into R11 once (0x8c66be66) for
                 * both tests; without it, 135 of 150 words differ */
                KMDWORD *chip = &pass_target;

                if (*chip == KMU_TARGET_ARC1) {
                    status = check_fog(ppVertexContextTable, i, pPass);
                    if (status) {
                        goto done;
                    }
                    status = check_filter(ppVertexContextTable, i, pPass);
                    if (status) {
                        goto done;
                    }
                    status =
                        check_texture_shading(ppVertexContextTable, i, pPass);
                    if (status) {
                        goto done;
                    }
                    status = check_color_type(ppVertexContextTable, i, pPass);
                    if (status) {
                        goto done;
                    }
                    status = check_shading(ppVertexContextTable, i, pPass);
                    if (status) {
                        goto done;
                    }
                }
                if (*chip == KMU_TARGET_CLX1 || *chip == KMU_TARGET_CLX2) {
                    status = check_user_clip(ppVertexContextTable, i, pPass);
                    if (status) {
                        goto done;
                    }
                    status = check_trilinear(ppVertexContextTable, i, pPass);
                    if (status) {
                        goto done;
                    }
                    status = check_sprite(ppVertexContextTable, i, pPass);
                    if (status) {
                        goto done;
                    }
                }
            }
            status = check_blending(ppVertexContextTable, i, pPass);
            if (status) {
                goto done;
            }
            status = check_modifier(ppVertexContextTable, i, pPass);
            /* @hack spelling: an empty arm for success, which makes the test a
             * BT past a BRA to done (0x8c66bf08). as if (status) goto done, as
             * the tests above write it, 2 of 150 words differ */
            if (status == KMU_PASS_OK) {
            } else {
                goto done;
            }
        } else {
            *pPass = i;
            return KMU_PASS_ERROR_VERTEXCONTEXT;
        }
    }
done:
    return status;
}

/* the CLX chips draw polygons, modifier volumes and sprites; the ARC1 draws
 * polygons and modifier volumes */
static KMUPASSSTATUS check_param_type(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                      PKMUINT32 pPass)
{
    PKMVERTEXCONTEXT context = table[pass];

    if (pass_target == KMU_TARGET_CLX1 || pass_target == KMU_TARGET_CLX2) {
        if (context->ParamType != KM_POLYGON &&
            context->ParamType != KM_MODIFIERVOLUME &&
            context->ParamType != KM_SPRITE) {
            *pPass = pass + 1;
            return KMU_PASS_ERROR_PARAMTYPE;
        }
    }
    if (pass_target == KMU_TARGET_ARC1) {
        if (context->ParamType != KM_POLYGON &&
            context->ParamType != KM_MODIFIERVOLUME) {
            *pPass = pass + 1;
            return KMU_PASS_ERROR_PARAMTYPE;
        }
    }
    return KMU_PASS_OK;
}

/* a polygon or sprite goes in the opaque or translucent list, or on the CLX2
 * also the punch-through list. a modifier volume goes in the opaque or
 * translucent modifier list */
static KMUPASSSTATUS check_list_type(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                     PKMUINT32 pPass)
{
    PKMVERTEXCONTEXT context = table[pass];
    KMINT32 value;

    if (pass_target == KMU_TARGET_CLX1 || pass_target == KMU_TARGET_CLX2) {
        if ((value = context->ParamType) == KM_POLYGON || value == KM_SPRITE) {
            if (pass_target == KMU_TARGET_CLX2) {
                KMINT32 list = context->ListType;

                if (list != KM_OPAQUE_POLYGON && list != KM_TRANS_POLYGON &&
                    list != KM_PUNCHTHROUGH_POLYGON) {
                    goto invalid;
                }
            }
            if (pass_target == KMU_TARGET_CLX1 &&
                (value = context->ListType) != KM_OPAQUE_POLYGON &&
                value != KM_TRANS_POLYGON) {
                goto invalid;
            }
        }
    } else {
        /* @hack redundant: an empty else, which loads ParamType into R3 for the
         * ARC1's polygon test (0x8c66bfea); without it, R2 takes it and 2 of
         * 80 words differ */
    }
    if (pass_target == KMU_TARGET_ARC1 && context->ParamType == KM_POLYGON) {
        KMINT32 list = context->ListType;

        if (list != KM_OPAQUE_POLYGON && list != KM_TRANS_POLYGON) {
            goto invalid;
        }
    }
    if (context->ParamType == KM_MODIFIERVOLUME) {
        KMINT32 list;

        if ((list = context->ListType) != KM_OPAQUE_MODIFIER &&
            list != KM_TRANS_MODIFIER) {
        invalid:
            *pPass = pass + 1;
            return KMU_PASS_ERROR_LISTTYPE;
        }
    }
    return KMU_PASS_OK;
}

/* a textured polygon or sprite needs a MipMapAdjust other than 0 */
static KMUPASSSTATUS check_mipmap(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                  PKMUINT32 pPass)
{
    PKMVERTEXCONTEXT context = table[pass];
    KMINT32 value;

    if (pass_target == KMU_TARGET_CLX1 || pass_target == KMU_TARGET_CLX2) {
        if (((value = context->ParamType) == KM_POLYGON ||
             value == KM_SPRITE) &&
            ((value = context->ShadingMode) == KM_TEXTUREFLAT ||
             value == KM_TEXTUREGOURAUD) &&
            !context->MipMapAdjust) {
            goto invalid;
        }
    }
    if (pass_target == KMU_TARGET_ARC1 && context->ParamType == KM_POLYGON) {
        KMINT32 shading = context->ShadingMode;

        if ((shading == KM_TEXTUREFLAT || shading == KM_TEXTUREGOURAUD) &&
            !context->MipMapAdjust) {
        invalid:
            *pPass = pass + 1;
            return KMU_PASS_ERROR_MIPMAP_D_ADJUST;
        }
    }
    return KMU_PASS_OK;
}

/* the ARC1 has no second fog table */
static KMUPASSSTATUS check_fog(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                               PKMUINT32 pPass)
{
    PKMVERTEXCONTEXT context = table[pass];

    if (pass_target == KMU_TARGET_ARC1 && context->ParamType == KM_POLYGON &&
        context->FogMode == KM_FOGTABLE_2) {
        *pPass = pass + 1;
        return KMU_PASS_ERROR_FOGMODE;
    }
    return KMU_PASS_OK;
}

/* the ARC1 has no trilinear filter */
static KMUPASSSTATUS check_filter(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                  PKMUINT32 pPass)
{
    PKMVERTEXCONTEXT context = table[pass];

    if (pass_target == KMU_TARGET_ARC1 && context->ParamType == KM_POLYGON) {
        KMINT32 value = context->ShadingMode;

        if (value == KM_TEXTUREFLAT || value == KM_TEXTUREGOURAUD) {
            if ((value = context->FilterMode) == KM_TRILINEAR_A ||
                value == KM_TRILINEAR_B) {
                *pPass = pass + 1;
                return KMU_PASS_ERROR_FILTERMODE;
            }
        }
    }
    return KMU_PASS_OK;
}

/* the ARC1 has no decal texture shading */
static KMUPASSSTATUS check_texture_shading(PPKMVERTEXCONTEXT table,
                                           KMUINT32 pass, PKMUINT32 pPass)
{
    PKMVERTEXCONTEXT context = table[pass];

    if (pass_target == KMU_TARGET_ARC1 && context->ParamType == KM_POLYGON) {
        KMINT32 shading = context->ShadingMode;

        if (shading == KM_TEXTUREFLAT || shading == KM_TEXTUREGOURAUD) {
            if (context->TextureShadingMode == KM_DECAL) {
                *pPass = pass + 1;
                return KMU_PASS_ERROR_TEXTURESHADINGMODE;
            }
        }
    }
    return KMU_PASS_OK;
}

/* the ARC1 has no intensity color type that uses the previous face's color */
static KMUPASSSTATUS check_color_type(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                      PKMUINT32 pPass)
{
    PKMVERTEXCONTEXT context = table[pass];

    if (pass_target == KMU_TARGET_ARC1 && context->ParamType == KM_POLYGON &&
        context->ColorType == KM_INTENSITY_PREV_FACE_COL) {
        *pPass = pass + 1;
        return KMU_PASS_ERROR_COLORTYPE;
    }
    return KMU_PASS_OK;
}

/* the ARC1 has no untextured flat shading */
static KMUPASSSTATUS check_shading(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                   PKMUINT32 pPass)
{
    PKMVERTEXCONTEXT context = table[pass];

    if (pass_target == KMU_TARGET_ARC1 && context->ParamType == KM_POLYGON &&
        context->ShadingMode == KM_NOTEXTUREFLAT) {
        *pPass = pass + 1;
        return KMU_PASS_ERROR_SHADINGMODE;
    }
    return KMU_PASS_OK;
}

/* the CLX chips refuse the UserClipMode KM_USERCLIP_RESERVE */
static KMUPASSSTATUS check_user_clip(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                     PKMUINT32 pPass)
{
    PKMVERTEXCONTEXT context = table[pass];

    if ((pass_target == KMU_TARGET_CLX1 || pass_target == KMU_TARGET_CLX2) &&
        context->UserClipMode == KM_USERCLIP_RESERVE) {
        *pPass = pass + 1;
        return KMU_PASS_ERROR_USERCLIPMODE;
    }
    return KMU_PASS_OK;
}

/* a trilinear filter's first pass, filter A blending one and zero, must be
 * followed by the passes trilinear_fill sets: a second with filter B blending
 * one and one, and after a translucent first pass, a third with filter B, whose
 * blending isn't checked. a missing pass fails as KMU_PASS_ERROR_VERTEXCONTEXT
 * and a wrong one as KMU_PASS_ERROR_TRILINEAR_SETTING, with *pPass that pass */
static KMUPASSSTATUS check_trilinear(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                     PKMUINT32 pPass)
{
    PPKMVERTEXCONTEXT passes = table + pass;
    PKMVERTEXCONTEXT first = passes[0];
    PKMVERTEXCONTEXT second = passes[1];
    PKMVERTEXCONTEXT third = passes[2];

    if (pass_target == KMU_TARGET_CLX1 || pass_target == KMU_TARGET_CLX2) {
        if (first->ListType == KM_OPAQUE_POLYGON &&
            (first->ShadingMode == KM_TEXTUREFLAT ||
             first->ShadingMode == KM_TEXTUREGOURAUD) &&
            first->FilterMode == KM_TRILINEAR_A &&
            first->SRCBlendingMode == KM_ONE &&
            first->DSTBlendingMode == KM_ZERO && first->bSRCSel == FALSE &&
            first->bDSTSel == FALSE) {
            if (!second) {
                goto no_second;
            }
            if (!(second->ListType == KM_TRANS_POLYGON &&
                  (second->ShadingMode == KM_TEXTUREFLAT ||
                   second->ShadingMode == KM_TEXTUREGOURAUD) &&
                  second->FilterMode == KM_TRILINEAR_B &&
                  second->SRCBlendingMode == KM_ONE &&
                  second->DSTBlendingMode == KM_ONE &&
                  second->bSRCSel == FALSE && second->bDSTSel == FALSE)) {
                *pPass = pass + 2;
                return KMU_PASS_ERROR_TRILINEAR_SETTING;
            }
        }
        if (first->ListType == KM_TRANS_POLYGON &&
            (first->ShadingMode == KM_TEXTUREFLAT ||
             first->ShadingMode == KM_TEXTUREGOURAUD) &&
            first->FilterMode == KM_TRILINEAR_A &&
            first->SRCBlendingMode == KM_ONE &&
            first->DSTBlendingMode == KM_ZERO && first->bSRCSel == FALSE &&
            first->bDSTSel == TRUE) {
            if (!second) {
                goto no_second;
            }
            if (!(second->ListType == KM_TRANS_POLYGON &&
                  (second->ShadingMode == KM_TEXTUREFLAT ||
                   second->ShadingMode == KM_TEXTUREGOURAUD) &&
                  second->FilterMode == KM_TRILINEAR_B &&
                  second->SRCBlendingMode == KM_ONE &&
                  second->DSTBlendingMode == KM_ONE &&
                  second->bSRCSel == FALSE && second->bDSTSel == TRUE)) {
                *pPass = pass + 2;
                return KMU_PASS_ERROR_TRILINEAR_SETTING;
            }
            if (!third) {
                goto no_third;
            }
            if (!(third->ListType == KM_TRANS_POLYGON &&
                  (third->ShadingMode == KM_TEXTUREFLAT ||
                   third->ShadingMode == KM_TEXTUREGOURAUD) &&
                  third->FilterMode == KM_TRILINEAR_B &&
                  third->bSRCSel == TRUE && third->bDSTSel == FALSE)) {
                *pPass = pass + 3;
                return KMU_PASS_ERROR_TRILINEAR_SETTING;
            }
        }
    }
    /* @hack spelling: goto valid, which puts the shared exits ahead of the
     * success's MOV #0,R0 at 0x8c66c314; with return KMU_PASS_OK in its place,
     * 18 of 164 words differ */
    goto valid;
no_third:
    *pPass = pass + 3;
    return KMU_PASS_ERROR_VERTEXCONTEXT;
no_second:
    *pPass = pass + 2;
    return KMU_PASS_ERROR_VERTEXCONTEXT;
valid:
    return KMU_PASS_OK;
}

/* a sprite on the CLX chips is untextured flat with a packed color, or textured
 * flat with a packed color and 16-bit UVs */
static KMUPASSSTATUS check_sprite(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                  PKMUINT32 pPass)
{
    PKMVERTEXCONTEXT context = table[pass];

    if ((pass_target == KMU_TARGET_CLX1 || pass_target == KMU_TARGET_CLX2) &&
        context->ParamType == KM_SPRITE) {
        KMINT32 shading = context->ShadingMode;

        if (shading != KM_NOTEXTUREFLAT && shading != KM_TEXTUREFLAT) {
            goto invalid;
        } else {
            /* @hack redundant: an empty else, which loads ColorType into R3 for
             * the textured test (0x8c66c350); without it, R2 takes it and 2 of
             * 45 words differ */
        }
        if (context->ShadingMode == KM_TEXTUREFLAT &&
            (context->ColorType != KM_PACKEDCOLOR ||
             context->UVFormat != KM_16BITUV)) {
            goto invalid;
        } else {
            /* @hack redundant: an empty else, which loads ShadingMode into R2
             * for the untextured test (0x8c66c35c); without it, R3 takes it and
             * 2 of 45 words differ */
        }
        if (context->ShadingMode == KM_NOTEXTUREFLAT &&
            context->ColorType != KM_PACKEDCOLOR) {
        invalid:
            *pPass = pass + 1;
            return KMU_PASS_ERROR_SPRITE_SETTING;
        }
    }
    return KMU_PASS_OK;
}

/* the opaque list requires blending one and zero */
static KMUPASSSTATUS check_blending(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                    PKMUINT32 pPass)
{
    PKMVERTEXCONTEXT context = table[pass];

    if (context->ListType == KM_OPAQUE_POLYGON &&
        (context->SRCBlendingMode != KM_ONE ||
         context->DSTBlendingMode != KM_ZERO)) {
        *pPass = pass + 1;
        return KMU_PASS_ERROR_BLENDINGMODE_SETTING;
    }
    return KMU_PASS_OK;
}

/* a modifier volume's ModifierInstruction must be one of the five
 * KM_MODIFIER_ names, which hold three values */
static KMUPASSSTATUS check_modifier(PPKMVERTEXCONTEXT table, KMUINT32 pass,
                                    PKMUINT32 pPass)
{
    PKMVERTEXCONTEXT context = table[pass];
    KMDWORD value;

    if (context->ParamType == KM_MODIFIERVOLUME) {
        /* KM_MODIFIER_INCLUDE_FIRST_POLY and KM_MODIFIER_EXCLUDE_FIRST_POLY are
         * KM_MODIFIER_NORMAL_POLY, so the first three tests are the same, and
         * SHC makes all three (TST R4,R4 at 0x8c66c3b6, 0x8c66c3ba and
         * 0x8c66c3be) */
        /* NOLINTBEGIN(misc-redundant-expression) */
        if ((value = context->ModifierInstruction) != KM_MODIFIER_NORMAL_POLY &&
            value != KM_MODIFIER_INCLUDE_FIRST_POLY &&
            value != KM_MODIFIER_EXCLUDE_FIRST_POLY &&
            value != KM_MODIFIER_INCLUDE_LAST_POLY &&
            value != KM_MODIFIER_EXCLUDE_LAST_POLY) {
            /* NOLINTEND(misc-redundant-expression) */
            *pPass = pass + 1;
            return KMU_PASS_ERROR_MODIFIER_SETTING;
        }
    }
    return KMU_PASS_OK;
}

/* the number of passes the context draws in: two for a trilinear filter's
 * opaque first pass, three for a translucent one, and otherwise one */
static KMUINT32 trilinear_passes(PKMVERTEXCONTEXT context)
{
    KMUINT32 count = 1;

    if (context->ListType == KM_OPAQUE_POLYGON &&
        (context->ShadingMode == KM_TEXTUREFLAT ||
         context->ShadingMode == KM_TEXTUREGOURAUD) &&
        context->FilterMode == KM_TRILINEAR_A &&
        context->SRCBlendingMode == KM_ONE &&
        context->DSTBlendingMode == KM_ZERO && context->bSRCSel == FALSE &&
        context->bDSTSel == FALSE) {
        count = 2;
    }
    if (context->ListType == KM_TRANS_POLYGON &&
        (context->ShadingMode == KM_TEXTUREFLAT ||
         context->ShadingMode == KM_TEXTUREGOURAUD) &&
        context->FilterMode == KM_TRILINEAR_A &&
        context->SRCBlendingMode == KM_ONE &&
        context->DSTBlendingMode == KM_ZERO && context->bSRCSel == FALSE &&
        context->bDSTSel == TRUE) {
        count = 3;
    }
    return count;
}

/* set the list, filter, blending and selections of the passes after a
 * trilinear filter's first pass, which check_trilinear then accepts */
static void trilinear_fill(PPKMVERTEXCONTEXT table)
{
    PKMVERTEXCONTEXT first = table[0];
    PKMVERTEXCONTEXT second = table[1];
    PKMVERTEXCONTEXT third = table[2];

    if (first->ListType == KM_OPAQUE_POLYGON &&
        (first->ShadingMode == KM_TEXTUREFLAT ||
         first->ShadingMode == KM_TEXTUREGOURAUD) &&
        first->FilterMode == KM_TRILINEAR_A &&
        first->SRCBlendingMode == KM_ONE && first->DSTBlendingMode == KM_ZERO &&
        first->bSRCSel == FALSE && first->bDSTSel == FALSE) {
        second->ListType = KM_TRANS_POLYGON;
        second->FilterMode = KM_TRILINEAR_B;
        second->SRCBlendingMode = KM_ONE;
        second->DSTBlendingMode = KM_ONE;
        second->bSRCSel = FALSE;
        second->bDSTSel = FALSE;
    }
    if (first->ListType == KM_TRANS_POLYGON &&
        (first->ShadingMode == KM_TEXTUREFLAT ||
         first->ShadingMode == KM_TEXTUREGOURAUD) &&
        first->FilterMode == KM_TRILINEAR_A &&
        first->SRCBlendingMode == KM_ONE && first->DSTBlendingMode == KM_ZERO &&
        first->bSRCSel == FALSE && first->bDSTSel == TRUE) {
        second->ListType = KM_TRANS_POLYGON;
        second->FilterMode = KM_TRILINEAR_B;
        second->SRCBlendingMode = KM_ONE;
        second->DSTBlendingMode = KM_ONE;
        second->bSRCSel = FALSE;
        second->bDSTSel = TRUE;
        third->ListType = KM_TRANS_POLYGON;
        third->FilterMode = KM_TRILINEAR_B;
        third->SRCBlendingMode = KM_SRCALPHA;
        third->DSTBlendingMode = KM_INVSRCALPHA;
        third->bSRCSel = TRUE;
        third->bDSTSel = FALSE;
    }
}

/* the log2 of the smaller side, its level's index in mip_offset, or 3 for a
 * side that is no power of 2 up to 1024 */
static int mip_level(int width, int height)
{
    if (width >= height) {
        width = height;
    }
    switch (width) {
    case KM_MAPSIZE_1:
        return 0;
    case KM_MAPSIZE_2:
        return 1;
    case KM_MAPSIZE_4:
        return 2;
    case KM_MAPSIZE_16:
        return 4;
    case KM_MAPSIZE_32:
        return 5;
    case KM_MAPSIZE_64:
        return 6;
    case KM_MAPSIZE_128:
        return 7;
    case KM_MAPSIZE_256:
        return 8;
    case KM_MAPSIZE_512:
        return 9;
    case KM_MAPSIZE_1024:
        return 10;
    case KM_MAPSIZE_8:
    default:
        return 3;
    }
}

/* twiddle each level of a size by size texture into output at its mip_offset,
 * from the full size down to 1 by 1, dithering each first when dither is set.
 * halve makes each smaller level in place, over input */
static void make_mipmaps(PKMDWORD output, PKMDWORD input, KMBOOLEAN dither,
                         int size, KMDWORD format)
{
    int level = mip_level(size, size);

    if (dither) {
        dither_texels((struct texel *)input, size, size, format);
    }
    twiddle((KMWORD *)((KMBYTE *)output + mip_offset[level]),
            (struct texel *)input, size, format);
    --level;
    do {
        halve((struct texel *)input, size, size);
        size >>= 1;
        if (dither) {
            dither_texels((struct texel *)input, size, size, format);
        }
        twiddle((KMWORD *)((KMBYTE *)output + mip_offset[level]),
                (struct texel *)input, size, format);
    } while (--level >= 0);
}

/* the 3-bit level nearest value's low byte, ties going up, where a level
 * below 4 stands for 32 times it and the rest for 32 times it plus 31. only
 * dither_texels calls it, from code that never runs */
static int quantize_3(int value)
{
    int sample = (KMBYTE)value;

    if (sample < 128) {
        value = 3;
    } else {
        value = 4;
    }
    if (sample < 112) {
        value = (sample + 16) >> 5;
    }
    if (sample > 143) {
        value = (sample - 15) >> 5;
    }
    return value;
}

/* error_6, error_5 and error_4: the error a channel's value takes at 6, 5 or
 * 4 bits, which dither_texels spreads: the value less a point in the step that
 * twiddle truncates it to, so negative below that point. at 6 and 5 bits the
 * point is the step's middle. at 4 bits it is 4 into the step for a value
 * below 128, and 12 into the step from 128 */
static int error_6(int value)
{
    return value - ((value & 0xfc) | 2);
}

static int error_5(int value)
{
    return value - ((value & 0xf8) | 4);
}

static int error_4(int value)
{
    return value - ((value & 0xf0) | ((value >> 4) & 8) | 4);
}

/* dither the width by height texels in place for format. red, green and blue
 * each add the error spread onto them, are clamped to 0 to 255, then spread
 * their own error at format's bits: three eighths to the texel after, three
 * eighths to the texel below and the rest to the texel below the one after.
 * the rows run on, so the texel after a row's last is the next row's first.
 * alpha's error is spread with blue's share, but no alpha takes it. a texture
 * 1 wide or 1 high is left as it is */
static void dither_texels(struct texel *texels, int width, int height,
                          KMDWORD format)
{
    int red;
    int green;
    int blue;
    int alpha;
    int share;
    /* the error the texel before passes on: right to this texel, and
     * below_right to the texel below this one */
    int right[4];
    int below_right[4];
    /* the error each texel of the next row takes from this row, by column */
    int below[KM_MAPSIZE_1024][4];
    int *carried;
    int x;
    int y;
    struct texel *texel;

    if (width == 1 || height == 1) {
        return;
    }
    carried = below[0];
    for (x = width; x; --x) {
        carried[0] = 0;
        carried[1] = 0;
        carried[2] = 0;
        carried[3] = 0;
        carried += 4;
    }
    right[0] = 0;
    right[1] = 0;
    right[2] = 0;
    right[3] = 0;
    below_right[0] = 0;
    below_right[1] = 0;
    below_right[2] = 0;
    below_right[3] = 0;
    texel = texels;
    for (y = height; y; --y) {
        carried = below[0];
        for (x = width; x; --x) {
            red = texel->red + carried[0] + right[0];
            green = texel->green + carried[1] + right[1];
            blue = texel->blue + carried[2] + right[2];
            alpha = texel->alpha;
            /* NOLINTBEGIN(readability-avoid-nested-conditional-operator) */
            red = red < 0 ? 0 : (red > 255 ? 255 : red);
            green = green < 0 ? 0 : (green > 255 ? 255 : green);
            blue = blue < 0 ? 0 : (blue > 255 ? 255 : blue);
            alpha = alpha < 0 ? 0 : (alpha > 255 ? 255 : alpha);
            /* NOLINTEND(readability-avoid-nested-conditional-operator) */
            texel->red = red;
            texel->green = green;
            texel->blue = blue;
            texel->alpha = alpha;
            if (format == KM_PIXELFORMAT_RGB565) {
                red = error_5(red);
                green = error_6(green);
                blue = error_5(blue);
                alpha = 0;
            } else if (format == KM_PIXELFORMAT_ARGB4444) {
                red = error_4(red);
                green = error_4(green);
                blue = error_4(blue);
                alpha = error_4(alpha);
            } else if (format == KM_PIXELFORMAT_ARGB1555) {
                red = error_5(red);
                green = error_5(green);
                blue = error_5(blue);
            }
            share = ((red * 3) + 4) >> 3;
            right[0] = share;
            carried[0] = below_right[0] + share;
            below_right[0] = red - ((share * 2));
            share = ((green * 3) + 4) >> 3;
            right[1] = share;
            carried[1] = below_right[1] + share;
            below_right[1] = green - ((share * 2));
            share = ((blue * 3) + 4) >> 3;
            right[2] = share;
            carried[2] = below_right[2] + share;
            below_right[2] = blue - ((share * 2));
            share = ((blue * 3) + 4) >> 3;
            right[3] = share;
            carried[3] = below_right[3] + share;
            below_right[3] = alpha - ((share * 2));
            ++texel;
            carried += 4;
        }
    }
    /* @hack redundant: a call of quantize_3 that never runs, which keeps it in
     * the object, as in the image: SHC leaves out a static function nothing
     * calls; without it, quantize_3 goes, the pool in this function at
     * 0x8c66c6a0 moves, the function runs 6 bytes long and 265 of 295 words
     * differ */
    if (0) {
        quantize_3(0);
    }
}

/* pack each texel of the size by size input into format and store it at its
 * twiddled index, which interleaves the bits of its row and column: the row's
 * in the even bits and the column's in the odd. the locals row and column
 * count in those bits alone. each steps by setting the other's bits, adding 1,
 * which carries over them, and masking them off again */
static void twiddle(KMWORD *output, struct texel *input, int size,
                    KMDWORD format)
{
    struct texel *row_start = input;
    int column;
    int row = 0;
    int x;
    int y;
    int value;
    int mask;

#ifdef GCC
    /* GCC can't see that every path sets value: each caller's format is
     * KM_PIXELFORMAT_RGB565, _ARGB4444 or _ARGB1555, which the tests below
     * name */
    value = 0;
#endif
    for (y = size; y; --y) {
        column = 0;
        input = row_start;
        for (x = size; x; --x, ++input) {
            mask = 0xf8;
            if (format == KM_PIXELFORMAT_RGB565) {
                value = (input->red & mask) << 8;
                mask = 0xfc;
                value |= (input->green & mask) << 3;
                value |= input->blue >> 3;
            } else if (format == KM_PIXELFORMAT_ARGB4444) {
                mask = 0xf0;
                value = (input->alpha & mask) << 8;
                value |= (input->red & mask) << 4;
                value |= input->green & mask;
                value |= input->blue >> 4;
            } else if (format == KM_PIXELFORMAT_ARGB1555) {
                value = (input->alpha & 0x80) << 8;
                value |= (input->red & mask) << 7;
                value |= (input->green & mask) << 2;
                value |= input->blue >> 3;
            }
            output[row | column] = value;
            /* @hack spelling: the column mask through mask, which loads
             * 0xaaaaaaaa into mask's R13 (0x8c66c97c) and keeps row in R10 and
             * x in R11; without it, 13 of 116 words differ */
            mask = 0xaaaaaaaa;
            column = (column + 0x55555556) & mask;
        }
        row_start += size;
        row = (row + 0xaaaaaaab) & 0x55555555;
    }
}

/* about sum / 36, 36 being the total of filter_row's weights: sum * 57 /
 * 2048. of the sums 0 to 9180 that a channel can take, 2,165 come out one more
 * than sum / 36 rounded down, as 539 gives 15 */
#define APPROX_36TH(sum) (((sum) * 57) >> 11)

/* write the next level's row of width texels from the four rows row0 to row3
 * of 2 * width texels. output[k] takes APPROX_36TH of the sum of the 16 texels
 * of columns 2k - 1 to 2k + 2, weighted 1, 2, 2 and 1 down and across: their
 * weighted mean, or one more. the columns wrap at the rows' ends */
static void filter_row(struct texel *row0, struct texel *row1,
                       struct texel *row2, struct texel *row3,
                       struct texel *output, int width)
{
    struct channels left;
    struct channels right;
    struct channels first_right;
    struct channels first_left;
    struct texel *l0;
    struct texel *l1;
    struct texel *l2;
    struct texel *l3;
    struct texel *r0;
    struct texel *r1;
    struct texel *r2;
    struct texel *r3;
    int n;
    int sum;

    left.red = (l0 = row0 + ((2 * width) - 1))->red +
               ((l1 = row1 + ((2 * width) - 1))->red * 2) +
               ((l2 = row2 + ((2 * width) - 1))->red * 2) +
               (l3 = row3 + ((2 * width) - 1))->red;
    left.green = l0->green + (l1->green * 2) + (l2->green * 2) + l3->green;
    left.blue = l0->blue + (l1->blue * 2) + (l2->blue * 2) + l3->blue;
    left.alpha = l0->alpha + (l1->alpha * 2) + (l2->alpha * 2) + l3->alpha;
    right.red = row0->red + (row1->red * 2) + (row2->red * 2) + row3->red;
    right.green =
        row0->green + (row1->green * 2) + (row2->green * 2) + row3->green;
    right.blue = row0->blue + (row1->blue * 2) + (row2->blue * 2) + row3->blue;
    right.alpha =
        row0->alpha + (row1->alpha * 2) + (row2->alpha * 2) + row3->alpha;
    first_right = right;
    first_left = left;
    for (n = width - 1; n; --n) {
        sum = left.red + (2 * right.red);
        left.red = (l0 = row0 + 1)->red + ((l1 = row1 + 1)->red * 2) +
                   ((l2 = row2 + 1)->red * 2) + (l3 = row3 + 1)->red;
        right.red = (r0 = row0 + 2)->red + ((r1 = row1 + 2)->red * 2) +
                    ((r2 = row2 + 2)->red * 2) + (r3 = row3 + 2)->red;
        sum += (2 * left.red) + right.red;
        output->red = APPROX_36TH(sum);
        sum = left.green + (2 * right.green);
        left.green = l0->green + (l1->green * 2) + (l2->green * 2) + l3->green;
        right.green = r0->green + (r1->green * 2) + (r2->green * 2) + r3->green;
        sum += (2 * left.green) + right.green;
        output->green = APPROX_36TH(sum);
        sum = left.blue + (2 * right.blue);
        left.blue = l0->blue + (l1->blue * 2) + (l2->blue * 2) + l3->blue;
        right.blue = r0->blue + (r1->blue * 2) + (r2->blue * 2) + r3->blue;
        sum += (2 * left.blue) + right.blue;
        output->blue = APPROX_36TH(sum);
        sum = left.alpha + (2 * right.alpha);
        left.alpha = l0->alpha + (l1->alpha * 2) + (l2->alpha * 2) + l3->alpha;
        right.alpha = r0->alpha + (r1->alpha * 2) + (r2->alpha * 2) + r3->alpha;
        sum += (2 * left.alpha) + right.alpha;
        output->alpha = APPROX_36TH(sum);
        row0 += 2;
        row1 += 2;
        row2 += 2;
        ++output;
        row3 += 2;
    }
    sum = left.red + (2 * right.red) + (2 * first_left.red) + first_right.red;
    output->red = APPROX_36TH(sum);
    sum = left.green + (2 * right.green) + (2 * first_left.green) +
          first_right.green;
    output->green = APPROX_36TH(sum);
    sum =
        left.blue + (2 * right.blue) + (2 * first_left.blue) + first_right.blue;
    output->blue = APPROX_36TH(sum);
    sum = left.alpha + (2 * right.alpha) + (2 * first_left.alpha) +
          first_right.alpha;
    output->alpha = APPROX_36TH(sum);
}

/* halve the width by height texels in place into the next level, width / 2
 * by height / 2. output row r is filter_row of rows 2r - 1 to 2r + 2, and the
 * last takes the first row from first_row, a copy made before output
 * overwrites it */
static void halve(struct texel *texels, int width, int height)
{
    struct texel first_row[KM_MAPSIZE_1024];
    int n;
    /* @hack spelling: columns and rows, copies of width and height, which put
     * texels in R7, rows in R12 and columns in R9 (0x8c66cd7e to 0x8c66cd8a);
     * without them, 37 of 94 words differ */
    int columns;
    int rows;
    int half_columns;
    int half_rows;
    struct texel *above;
    struct texel *middle;
    struct texel *below;
    struct texel *next;
    struct texel *output;

    columns = width;
    rows = height;
    half_columns = columns / 2;
    half_rows = rows / 2;
    {
        struct texel *from = texels;
        struct texel *to = first_row;

        for (n = columns; n; --n) {
            *to = *from;
            ++from;
            ++to;
        }
    }
    output = texels;
    /* @bug logic: above, the row above the first, is meant to wrap to
     * the last row, as next wraps to the first for the last output row. it
     * steps (rows - 1) * columns 2-byte words rather than texels (SHLL R12 at
     * 0x8c66cdca), so it starts in the middle of the row whose index is
     * rows / 2 - 1 */
    above = (struct texel *)((KMWORD *)texels + ((rows - 1) * columns));
    middle = texels;
    for (n = half_rows - 1; n; --n) {
        below = middle + columns;
        next = middle + (columns * 2);
        filter_row(above, middle, below, next, output, half_columns);
        above = below;
        middle = next;
        output += half_columns;
    }
    below = middle + columns;
    next = first_row;
    filter_row(above, middle, below, next, output, half_columns);
}
