#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

void njReleaseTextureAll(void)
{
    Uint32 i;

    for (i = 0; i < _nj_texmemsize; i++) {
        /* @bug logic: the test takes a record whose texaddr is
         * TEXMEM_LOADED alone (CMP/EQ #2,R0 at 0x8c65ab96), so clearing that
         * flag leaves texaddr 0 (TST R1,R1 at 0x8c65abb8), and the branch that
         * keeps a record with another flag (0x8c65abbc to 0x8c65abc8) never
         * runs. a record that is also TEXMEM_CACHED is not released */
        if (_nj_texmemlist[i].texaddr == TEXMEM_LOADED) {
            kmFreeTexture(
                (PKMSURFACEDESC)&_nj_texmemlist[i].texinfo.texsurface);
            _nj_texmemlist[i].texaddr &= ~TEXMEM_LOADED;
            if (_nj_texmemlist[i].texaddr) {
                _nj_texmemlist[i].count = 0;
                _nj_texmemlist[i].texinfo.texsurface.pSurface = NULL;
            } else {
                njFreeTexMemList(&_nj_texmemlist[i]);
            }
        } else if (_nj_texmemlist[i].texaddr == TEXMEM_UNK_4) {
            njFreeTexMemList(&_nj_texmemlist[i]);
        }
    }
    _nj_current_texture = -1;
}
