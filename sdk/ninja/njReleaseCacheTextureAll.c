#include <ninja.h>
#include <sg_xpt.h>

void njReleaseCacheTextureAll(void)
{
    Uint32 i;

    for (i = 0; i < _nj_texmemsize; i++) {
        if (_nj_texmemlist[i].texaddr & TEXMEM_CACHED) {
            njCacheTextureFree(_nj_texmemlist[i].texinfo.texaddr);
            _nj_texmemlist[i].texaddr &= ~TEXMEM_CACHED;
            /* texaddr holds the record's TEXMEM_ flags. with none left, the
             * record holds no copy of its texture, and is freed */
            if (_nj_texmemlist[i].texaddr == 0) {
                njFreeTexMemList(&_nj_texmemlist[i]);
            }
        }
    }
}
