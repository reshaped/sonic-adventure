#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

/* palettized records with the same global index share one surface, which is
 * freed only when the last of them is released. while others share it,
 * releasing a record drops one of its users, and frees the record at its last
 * user but not the surface. a shared record that is also TEXMEM_CACHED is left
 * as it is */
Sint32 njReleaseTextureLN(Uint32 texaddr)
{
    NJS_TEXMEMLIST *tex = (NJS_TEXMEMLIST *)texaddr;
    Uint32 i;
    Sint32 sharing;
    Sint32 palette;

    if (tex->texaddr & TEXMEM_LOADED) {
        palette = njCheckPaletteType((tex->texinfo.texsurface.Type >> 16) &
                                     NJD_TEXFMT_TYPE_MASK);
        if (palette != 1) {
            if (tex->count != 0) {
                tex->count--;
            }
            if (tex->count == 0) {
                kmFreeTexture((PKMSURFACEDESC)&tex->texinfo.texsurface);
                tex->texaddr &= ~TEXMEM_LOADED;
                if (_nj_current_texture == tex->globalIndex) {
                    _nj_current_texture = -1;
                }
                if (tex->texaddr == 0) {
                    njFreeTexMemList(tex);
                }
            }
        } else {
            sharing = 0;
            for (i = 0; i < _nj_texmemsize; i++) {
                if (_nj_texmemlist[i].globalIndex == tex->globalIndex) {
                    sharing++;
                }
            }
            if (sharing > 1) {
                if (!(tex->texaddr & TEXMEM_CACHED)) {
                    tex->count--;
                    if (tex->count == 0) {
                        tex->texaddr &= ~TEXMEM_LOADED;
                        njFreeTexMemList(tex);
                    }
                }
            } else {
                tex->count--;
                if (tex->count == 0) {
                    kmFreeTexture((PKMSURFACEDESC)&tex->texinfo.texsurface);
                    tex->texaddr &= ~TEXMEM_LOADED;
                    if (!(tex->texaddr & TEXMEM_CACHED)) {
                        njFreeTexMemList(tex);
                    }
                }
            }
        }
    } else {
        _nj_texerr_.texerr = NJD_TEXERR_OTHER;
        return -1;
    }
    return 1;
}

Sint32 njReleaseTextureNum(Uint32 n)
{
    Sint32 result;

    result = njReleaseTextureLN(_nj_current_texlist->textures[n].texaddr);
    if (result < 0) {
        _nj_texerr_.n = n;
    }
    return result;
}
