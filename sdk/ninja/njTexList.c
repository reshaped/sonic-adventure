/* Ninja's texture record list and its searches */
#include <ninja.h>
#include <sg_xpt.h>

#include "ninja/njtexmem.h"

Sint32 njSearchTexList2(NJS_TEXMEMLIST *list, Uint32 n, NJS_TEXMEMLIST *tex);

/* the record njEntryTexMemList and njTexMemList try first. when it isn't
 * free, they search from record 0 */
static Uint32 texmem_next_free = 0;

void njInitTexMemList(NJS_TEXMEMLIST *addr, Uint32 n)
{
    Uint32 i;

    _nj_texmemlist = addr;
    _nj_texmemsize = n;
    for (i = 0; i < _nj_texmemsize; i++) {
        _nj_texmemlist[i].globalIndex = TEXMEM_FREE;
        _nj_texmemlist[i].bank = TEXMEM_FREE;
        _nj_texmemlist[i].count = 0;
    }
    texmem_next_free = 0;
    _nj_tex_count = 0;
}

void njEntryTexMemList(Uint32 *handle, NJS_TEXINFO *info, Uint32 globalIndex,
                       Uint32 texaddr, Uint32 bank)
{
    Uint32 i;
    Uint32 next;
    Uint32 n;
    NJS_TEXMEMLIST *list;

    next = texmem_next_free;
    list = _nj_texmemlist;
    n = _nj_texmemsize;
    if (list[next].globalIndex == TEXMEM_FREE) {
        i = next;
    } else {
        for (i = 0; i < n; i++) {
            /* @hack redundant: the else with a continue, which keeps the loop's
             * test at its bottom (CMP/HS R13,R12 at 0x8c66b0f4 in
             * 1ST_READ.BIN); without it, 106 of 128 words differ */
            if (list[i].globalIndex == TEXMEM_FREE) {
                break;
            } else {
                continue;
            }
        }
    }
    if (i != n - 1) {
        texmem_next_free = i + 1;
    } else {
        texmem_next_free = 0;
    }
    *handle = (Uint32)&_nj_texmemlist[i];
    _nj_texmemlist[i].texinfo.texsurface.Type = info->texsurface.Type;
    _nj_texmemlist[i].texinfo.texsurface.BitDepth = info->texsurface.BitDepth;
    _nj_texmemlist[i].texinfo.texsurface.PixelFormat =
        info->texsurface.PixelFormat;
    _nj_texmemlist[i].texinfo.texsurface.nWidth = info->texsurface.nWidth;
    _nj_texmemlist[i].texinfo.texsurface.nHeight = info->texsurface.nHeight;
    _nj_texmemlist[i].texinfo.texsurface.TextureSize =
        info->texsurface.TextureSize;
    _nj_texmemlist[i].texinfo.texsurface.fSurfaceFlags =
        info->texsurface.fSurfaceFlags;
    _nj_texmemlist[i].texinfo.texsurface.pSurface = info->texsurface.pSurface;
    _nj_texmemlist[i].texinfo.texaddr = info->texaddr;
    _nj_texmemlist[i].globalIndex = globalIndex;
    _nj_texmemlist[i].bank = bank;
    _nj_texmemlist[i].texaddr = texaddr;
    /* @hack spelling: the list through tex, which reads _nj_texmemlist once
     * for both arms, in the test's slot (MOV.L @R14,R4 at 0x8c66b17e in
     * 1ST_READ.BIN); as _nj_texmemlist[i] in each, 26 of 128 words differ */
    {
        NJS_TEXMEMLIST *tex = _nj_texmemlist;

        /* a record whose texaddr holds TEXMEM_LOADED or TEXMEM_UNK_4 starts
         * with one user, and any other with none */
        if (texaddr & (TEXMEM_LOADED | TEXMEM_UNK_4)) {
            tex[i].count = 1;
        } else {
            tex[i].count = 0;
        }
    }
    if ((Uint32)_nj_tex_count <= i) {
        _nj_tex_count++;
    }
}

void njFreeTexMemList(NJS_TEXMEMLIST *tex)
{
    Uint32 i;
    Uint32 last;

    tex->globalIndex = TEXMEM_FREE;
    tex->bank = TEXMEM_FREE;
    tex->texinfo.texaddr = NULL;
    tex->texinfo.texsurface.Type = 0;
    tex->texinfo.texsurface.BitDepth = 0;
    tex->texinfo.texsurface.PixelFormat = 0;
    tex->texinfo.texsurface.nWidth = 0;
    tex->texinfo.texsurface.nHeight = 0;
    tex->texinfo.texsurface.TextureSize = 0;
    tex->texinfo.texsurface.fSurfaceFlags = 0;
    tex->texinfo.texsurface.pSurface = NULL;
    tex->texaddr = 0;
    tex->count = 0;
    i = ((Uint32)tex - (Uint32)_nj_texmemlist) / sizeof(NJS_TEXMEMLIST);
    if (_nj_tex_count > 0 && i == (Uint32)(_nj_tex_count - 1)) {
        while (_nj_tex_count > 0 &&
               _nj_texmemlist[_nj_tex_count - 1].globalIndex == TEXMEM_FREE) {
            _nj_tex_count--;
        }
    }
    last = _nj_tex_count - 1;
    if (texmem_next_free > last) {
        texmem_next_free = last;
    } else {
        texmem_next_free = 0;
    }
}

Sint32 njSearchTexList(NJS_TEXMEMLIST *list, Uint32 n, Uint32 globalIndex)
{
    Uint32 i;

    if (list == NULL) {
        return -1;
    }
    for (i = 0; i < n; i++) {
        if (list[i].globalIndex == globalIndex) {
            return i;
        }
    }
    return -1;
}

/* the index in list of the record at tex. it returns -1 for a null tex or
 * list, and for an address that is not one of list's n records */
Sint32 njSearchTexList2(NJS_TEXMEMLIST *list, Uint32 n, NJS_TEXMEMLIST *tex)
{
    Uint32 offset;
    Uint32 i;
    Uint32 rest;
    Uint32 size;

    if (tex == NULL) {
        return -1;
    }
    if (list == NULL) {
        return -1;
    }
    /* @hack spelling: the record's size through size, set before offset,
     * which loads it ahead of the subtraction (MOV #68,R7 at 0x8c66b266 in
     * 1ST_READ.BIN); as sizeof in both operations, 4 of 35 words differ */
    size = sizeof(NJS_TEXMEMLIST);
    offset = (Uint32)tex - (Uint32)list;
    rest = offset % size;
    if (rest != 0) {
        return -1;
    }
    i = offset / size;
    if (i >= n) {
        return -1;
    }
    return i;
}

Sint32 njSearchTexList3(NJS_TEXMEMLIST *list, Uint32 n, Uint32 globalIndex,
                        Uint32 bank)
{
    Uint32 i;

    if (list == NULL) {
        return -1;
    }
    for (i = 0; i < n; i++) {
        /* @hack spelling: two tests, not one &&, which compute the record's
         * address again for the bank's test (0x8c66b2de to 0x8c66b2ea in
         * 1ST_READ.BIN); as one, 30 of 37 words differ */
        if (list[i].globalIndex == globalIndex) {
            if (list[i].bank == bank) {
                return i;
            }
        }
    }
    return -1;
}

void njSetFlagAddrAllTexture(Uint32 globalIndex, Uint32 flag, void *addr)
{
    Uint32 i;
    NJS_TEXMEMLIST *tex;

    for (i = 0; i < _nj_texmemsize; i++) {
        tex = &_nj_texmemlist[i];
        if (tex->globalIndex == globalIndex) {
            tex->texaddr |= flag;
            tex->texinfo.texaddr = addr;
            if (flag & TEXMEM_LOADED) {
                tex->count = 1;
                njSetTextureParam(tex);
                njSetPaletteBankLN(tex, tex->bank);
            }
        }
    }
}

NJS_TEXINFO *njTexMemList(Uint32 globalIndex)
{
    Uint32 i;
    Uint32 next;
    NJS_TEXMEMLIST *list;

    next = texmem_next_free;
    list = _nj_texmemlist;
    if (list[next].globalIndex == TEXMEM_FREE) {
        i = next;
    } else {
        for (i = 0; i < _nj_texmemsize; i++) {
            if (list[i].globalIndex == TEXMEM_FREE) {
                break;
            }
        }
    }
    return &list[i].texinfo;
}
