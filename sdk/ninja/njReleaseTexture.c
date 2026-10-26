#include <ninja.h>
#include <sg_xpt.h>

Sint32 njReleaseTexture(NJS_TEXLIST *texlist)
{
    Uint32 i;

    for (i = 0; i < texlist->nbTexture; i++) {
        if (njReleaseTextureLN(texlist->textures[i].texaddr) < 0) {
            return -1;
        }
    }
    return 1;
}
