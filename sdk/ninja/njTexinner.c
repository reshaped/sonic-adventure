/* texture loading internals: pixels to video memory or the texture cache, and a
 * texture file's name, global index and format */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

#include "ninja/njpvr.h"

/* how many entries of fileext njCheckFileName compares an extension with,
 * though fileext holds one */
#define FILEEXT_NUM 3
/* the index in fileext of the texture file's extension, which njCheckFileName
 * sets. it is 0, .PVR's, where none matches */
extern Sint32 _nj_tex_ext_;

/* load the pixels at tex, or at info->texaddr when tex is null, into info's
 * surface in video memory, and wait for the transfer to end. it returns 1. if
 * Kamui refuses the load, it returns -NJD_TEXERR_TEXMEMLOAD with
 * _nj_texerr_.texerr set to NJD_TEXERR_TEXMEMLOAD */
Sint32 njTexDownloadMipMap(void *tex, NJS_TEXINFO *info)
{
    if (tex == NULL) {
        if (kmLoadTexture((PKMSURFACEDESC)&info->texsurface,
                          (PKMDWORD)info->texaddr, FALSE, FALSE)) {
            _nj_texerr_.texerr = NJD_TEXERR_TEXMEMLOAD;
            return -NJD_TEXERR_TEXMEMLOAD;
        }
    } else {
        if (kmLoadTexture((PKMSURFACEDESC)&info->texsurface, (PKMDWORD)tex,
                          FALSE, FALSE)) {
            _nj_texerr_.texerr = NJD_TEXERR_TEXMEMLOAD;
            return -NJD_TEXERR_TEXMEMLOAD;
        }
    }
    while (kmQueryFinishLastTextureDMA()) {
    }
    return 1;
}

Sint32 njTexDownloadCache(void *tex, NJS_TEXINFO *info)
{
    Uint32 size;
    void *cache;

    if (info->texsurface.fSurfaceFlags & KM_SURFACEFLAGS_MIPMAPED) {
        size = info->texsurface.TextureSize - NJD_PVR_MM_DUMMYBYTE;
    } else {
        size = info->texsurface.TextureSize;
    }
    cache = njCacheTextureMalloc(size);
    if (cache == NULL) {
        _nj_texerr_.texerr = NJD_TEXERR_MAINMEMORY;
        return -NJD_TEXERR_MAINMEMORY;
    }
    njMemCopy(cache, info->texaddr, size);
    info->texaddr = cache;
    return (Sint32)cache;
}

Sint32 njCheckFileName(char *path, char *name)
{
    char ext[5];
    Sint32 i;
    Sint32 len;

    /* @hack spelling: strlen's result through len, which keeps a copy of it
     * in R4 (MOV R0,R4 at 0x8c66b9a8 in 1ST_READ.BIN); as strlen(name) in the
     * sum, 70 of 87 words differ */
    len = strlen(name);
    _builtin_strcpy(ext, name + (len - 4));
    strupr(ext);
    if (ext[0] == '.') {
        /* @bug memory: the loop compares ext with FILEEXT_NUM entries,
         * but fileext holds one, .PVR. the two past it are "" and "*", read
         * from the pad after it and from mipmap_sizes. an ext that starts with
         * . matches neither (MOV #3,R11 at 0x8c66b9d0 in 1ST_READ.BIN, and at
         * 0x8ce1f2b8 in LOADER.BIN) */
        for (i = 0; i < FILEEXT_NUM; i++) {
            if (_builtin_strcmp(ext, fileext[i]) == 0) {
                break;
            }
        }
        /* @hack spelling: a switch on i whose default takes a match, which
         * keeps _nj_tex_ext_'s address in R2 (0x8c66b9f4 in 1ST_READ.BIN); as
         * an if, 4 of 87 words differ */
        switch (i) {
        default:
            _nj_tex_ext_ = i;
            _builtin_strcpy(path, _nj_current_texture_path);
            strcat(path, name);
            return 1;
        case FILEEXT_NUM:
            break;
        }
    }
    _builtin_strcpy(path, _nj_current_texture_path);
    strcat(path, name);
    strcat(path, ".PVR");
    _nj_tex_ext_ = 0;
    return 1;
}

Sint32 njGetFileGlobalIndex(NJS_TEXNAME *texname)
{
    Sint32 name;
    Sint32 skip;
    struct gbix_chunk gbix;
    Uint32 globalIndex;

    if (texname->attr & NJD_TEXATTR_GLOBALINDEX) {
        globalIndex = texname->texaddr;
    } else {
        if (_nj_tex_ext_ == 0) {
            do {
                if (njTexFread(&name, 4) < 0) {
                    return -1;
                }
                switch (name) {
                case GBIX:
                    njTexFseek(-4, GDD_SEEK_CUR);
                    if (njTexFread(&gbix, sizeof(gbix)) < 0) {
                        return -1;
                    }
                    break;
                default:
                    if (njTexFread(&skip, 4) < 0) {
                        return -1;
                    }
                    njTexFseek(skip, GDD_SEEK_CUR);
                    break;
                }
            } while (name != PVRT && name != GBIX);
            if (name == GBIX) {
                globalIndex = gbix.globalIndex;
            } else {
                while (njSearchTexList(_nj_texmemlist, _nj_texmemsize,
                                       _nj_user_globalIndex) != -1) {
                    _nj_user_globalIndex--;
                }
                globalIndex = _nj_user_globalIndex;
            }
        } else {
            while (njSearchTexList(_nj_texmemlist, _nj_texmemsize,
                                   _nj_user_globalIndex) != -1) {
                _nj_user_globalIndex--;
            }
            globalIndex = _nj_user_globalIndex;
        }
    }
    njTexFseek(0, GDD_SEEK_SET);
    return globalIndex;
}

Sint32 njGetFileTexType(void)
{
    Sint32 name;
    Sint32 skip;
    struct pvrt_chunk pvrt;

    do {
        if (njTexFread(&name, 4) < 0) {
            return -1;
        }
        switch (name) {
        case PVRT:
            njTexFseek(-4, GDD_SEEK_CUR);
            if (njTexFread(&pvrt, sizeof(pvrt)) < 0) {
                return -1;
            }
            break;
        default:
            if (njTexFread(&skip, 4) < 0) {
                return -1;
            }
            njTexFseek(skip, GDD_SEEK_CUR);
            break;
        }
    } while (name != PVRT);
    njTexFseek(0, GDD_SEEK_SET);
    return pvrt.type;
}

Sint32 njCheckPaletteType(Uint32 type)
{
    switch (type) {
    case NJD_TEXFMT_PALETTIZE4:
    case NJD_TEXFMT_PALETTIZE4_MM:
    case NJD_TEXFMT_PALETTIZE8:
    case NJD_TEXFMT_PALETTIZE8_MM:
        return 1;
    default:
        return 0;
    }
}
