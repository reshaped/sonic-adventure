/* the texture palette's color format and colors, and the palette banks the
 * current vertex context and each palettized texture draw with */
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "ninja/njpvr.h"

/* the palette's 1024 colors, a word each, among the video chip's registers,
 * and the bits that map an address into the uncached area, which the palette
 * is written through */
#define PALETTE 0x005f9000
#define UNCACHED 0xa0000000

/* the bits of bank each palettized texture keeps. a 4-bit texture keeps all
 * six, one of 64 banks of 16 colors, and an 8-bit texture bits 4 and 5, one of
 * 4 banks of 256 */
#define BANK_MASK_4BPP 0x3f
#define BANK_MASK_8BPP 0x30

/* the palette's mode, an NJD_TEXFMT_ format, and the size of its colors,
 * NJD_TEXPALETTE_16BPP or NJD_TEXPALETTE_32BPP */
extern Sint32 _nj_palette_mode_;
extern Sint32 _nj_palette_bpp_;

void njSetPaletteBank(Uint32 bank)
{
    _nj_current_ctx_->PaletteBank = bank;
    kmSetVertexRenderState(_nj_current_ctx_);
}

void njSetPaletteBankLN(NJS_TEXMEMLIST *tex, Sint32 bank)
{
    Sint32 bits = (tex->texinfo.texsurface.Type >> 16) & NJD_TEXFMT_TYPE_MASK;

    switch (bits) {
    case NJD_TEXFMT_PALETTIZE4:
    case NJD_TEXFMT_PALETTIZE4_MM:
        bits = BANK_MASK_4BPP;
        break;
    case NJD_TEXFMT_PALETTIZE8:
    case NJD_TEXFMT_PALETTIZE8_MM:
        bits = BANK_MASK_8BPP;
        break;
    default:
        return;
    }
    bits &= bank;
    tex->texparambuffer &= ~TEX_PALETTE;
    tex->texparambuffer |= bits << TEX_PALETTE_SHIFT;
    tex->bank = bank;
}

void njSetPaletteBankNum(Uint32 n, Sint32 bank)
{
    NJS_TEXMEMLIST *tex;

    tex = (NJS_TEXMEMLIST *)_nj_current_texlist->textures[n].texaddr;
    njSetPaletteBankLN(tex, bank);
}

void njSetPaletteBankNumG(Uint32 globalIndex, Sint32 bank)
{
    Sint32 i;

    i = njSearchTexList(_nj_texmemlist, _nj_texmemsize, globalIndex);
    if (i >= 0) {
        njSetPaletteBankLN(&_nj_texmemlist[i], bank);
    }
}

void njSetPaletteMode(Uint32 mode)
{
    Int km_mode;

    _nj_palette_mode_ = mode;
    switch (mode) {
    case NJD_TEXFMT_ARGB_1555:
        km_mode = KM_PALETTE_16BPP_ARGB1555;
        _nj_palette_bpp_ = NJD_TEXPALETTE_16BPP;
        break;
    case NJD_TEXFMT_RGB_565:
        km_mode = KM_PALETTE_16BPP_RGB565;
        _nj_palette_bpp_ = NJD_TEXPALETTE_16BPP;
        break;
    case NJD_TEXFMT_ARGB_4444:
        km_mode = KM_PALETTE_16BPP_ARGB4444;
        _nj_palette_bpp_ = NJD_TEXPALETTE_16BPP;
        break;
    case NJD_TEXFMT_ARGB_8888:
        km_mode = KM_PALETTE_32BPP_ARGB8888;
        _nj_palette_bpp_ = NJD_TEXPALETTE_32BPP;
        break;
    default:
        km_mode = KM_PALETTE_16BPP_ARGB1555;
        _nj_palette_bpp_ = NJD_TEXPALETTE_16BPP;
        break;
    }
    kmSetPaletteMode(km_mode);
}

Uint32 njGetPaletteMode(void)
{
    return _nj_palette_mode_;
}

/* copy count colors from data into the palette from entry offset. while
 * _nj_palette_bpp_ is NJD_TEXPALETTE_16BPP, data holds a Uint16 per color,
 * which fills a palette word, and otherwise a Uint32 */
void njSetPaletteData(Sint32 offset, Sint32 count, void *data)
{
    Sint32 i;
    Uint16 *color;

    if (_nj_palette_bpp_ == NJD_TEXPALETTE_16BPP) {
        color = data;
        for (i = offset; i < offset + count; i++) {
            *(Uint32 *)((PALETTE + (i * 4)) | UNCACHED) = *color++;
        }
    } else {
        njMemCopy4((void *)((PALETTE + (offset * 4)) | UNCACHED), data, count);
    }
}

/* copy count colors from the palette from entry offset into data. while
 * _nj_palette_bpp_ is NJD_TEXPALETTE_16BPP, data takes a Uint16 per color, the
 * low half of its palette word, and otherwise a Uint32 */
void njGetPaletteData(Sint32 offset, Sint32 count, void *data)
{
    Sint32 i;
    Uint16 *color;

    if (_nj_palette_bpp_ == NJD_TEXPALETTE_16BPP) {
        color = data;
        for (i = offset; i < offset + count; i++) {
            *color++ = *(Uint32 *)(PALETTE + (i * 4));
        }
    } else {
        njMemCopy4(data, (void *)(PALETTE + (offset * 4)), count);
    }
}

/* read the PVPL file filename into pl: its colors into pl->palette, which must
 * hold them all, and its bank, offset, count and mode. the colors go to memory
 * alone; njSetPaletteData writes them to the palette. the file's bank becomes
 * the current palette bank before the colors are read, and stays current when
 * that read fails. returns 1, or -1 when the file can't be opened or read or
 * isn't a PVPL file */
Sint32 njLoadPaletteFile(const char *filename, NJS_TEXPALETTE *pl)
{
    Uint32 name;
    struct pvpl_header header;

    njInitTextureErrStat();
    if (njTexFopen(filename) < 0) {
        return -1;
    }
    if (njTexFread(&name, 4) < 0) {
        njTexFclose();
        return -1;
    }
    switch (name) {
    case PVPL:
        if (njTexFread(&header, sizeof(header)) < 0) {
            njTexFclose();
            return -1;
        }
        njSetPaletteBank(header.bank);
        if (njTexFread(pl->palette, header.size - 8) < 0) {
            njTexFclose();
            return -1;
        }
        pl->bank = header.bank;
        pl->offset = header.offset;
        pl->count = header.count;
        /* @hack type: the mode read by its low byte (MOV.B at 0x8c66a7cc);
         * as a Uint16, 25 of 67 words differ */
        pl->mode = *(Uint8 *)&header.mode;
        break;
    default:
        njTexFclose();
        return -1;
    }
    njTexFclose();
    return 1;
}
