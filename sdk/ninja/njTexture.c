/* texture setup and teardown, texture list loading and selection, texture
 * memory queries, and the texture error status */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "ninja/njtexmem.h"

/* the TSP word's U and V size codes */
#define TSP_UV_SIZE 0x3f
/* the texture word's address field, in 8-byte units. TEXTURE_ADDRESS_MAX is
 * video memory's 8 MB in those units */
#define TEXTURE_ADDRESS_MASK 0x1fffff
#define TEXTURE_ADDRESS_MAX 0x100000

void njInitTextureGlobalIndex(Uint32 globalIndex)
{
    if (globalIndex >= 0xfffffff0) {
        _nj_user_globalIndex = 0xffffffef;
    } else {
        _nj_user_globalIndex = globalIndex;
    }
}

/* make addr the buffer through which the texture loads read a file's pixels.
 * it holds size bytes, rounded down to whole sectors */
void njInitTextureBuffer(Sint8 *addr, Uint32 size)
{
    _nj_texture_buffer_st = addr;
    _nj_texture_buffer_max_size = (size >> SECTOR_SHIFT) << SECTOR_SHIFT;
}

void njInitTexture(NJS_TEXMEMLIST *addr, Uint32 n)
{
    njInitTexMemList(addr, n);
    _nj_current_texture_path[0] = '\0';
    _nj_tex_err_surface.Type = 0;
    _nj_tex_err_surface.BitDepth = 0;
    _nj_tex_err_surface.PixelFormat = 0;
    _nj_tex_err_surface.nWidth = 0;
    _nj_tex_err_surface.nHeight = 0;
    _nj_tex_err_surface.TextureSize = 0;
    _nj_tex_err_surface.fSurfaceFlags = 0;
    _nj_tex_err_surface.pSurface = NULL;
}

void njExitTexture(void)
{
    njReleaseTextureAll();
    njReleaseCacheTextureAll();
    _nj_tex_err_surface.Type = 0;
    _nj_tex_err_surface.BitDepth = 0;
    _nj_tex_err_surface.PixelFormat = 0;
    _nj_tex_err_surface.nWidth = 0;
    _nj_tex_err_surface.nHeight = 0;
    _nj_tex_err_surface.TextureSize = 0;
    _nj_tex_err_surface.fSurfaceFlags = 0;
    _nj_tex_err_surface.pSurface = NULL;
}

/* clear the texture error status, then load each texture of texlist with
 * njLoadTextureLN, stopping at the first that fails. it returns 1, or -1 for
 * a texlist without textures, with _nj_texerr_.texerr set to
 * NJD_TEXERR_OTHER. it returns -1 for a texture that fails, with its index in
 * _nj_texerr_.n */
Sint32 njLoadTexture(NJS_TEXLIST *texlist)
{
    Uint32 i;

    njInitTextureErrStat();
    /* @hack spelling: nbTexture, which is unsigned, tested as <= 0, which
     * compares it with a 0 in R2 (MOV #0,R2 at 0x8c65afe2, CMP/HI R2,R3 at
     * 0x8c65afe8 in 1ST_READ.BIN); as == 0, a TST tests it, the function is 2
     * bytes shorter, and 29 of 37 words differ */
    if (texlist->nbTexture <= 0) {
        _nj_texerr_.texerr = NJD_TEXERR_OTHER;
        return -1;
    }
    for (i = 0; i < texlist->nbTexture; i++) {
        if (njLoadTextureLN(texlist, i) < 0) {
            _nj_texerr_.n = i;
            return -1;
        }
    }
    return 1;
}

Sint32 njSetTexture(NJS_TEXLIST *texlist)
{
    if (texlist == NULL) {
        return -1;
    }
    _nj_current_texlist = texlist;
    return 1;
}

/* make texture n of the current texture list current, unless it is current
 * already. the current vertex context's TSP word then takes the texture's UV
 * sizes, and its texture word the record's texparambuffer. Kamui then takes
 * the context through kmSetVertexRenderState. it returns 1, or -1 for a
 * texture whose address is above TEXTURE_ADDRESS_MAX. that texture still
 * becomes current and gives its UV sizes, but the context keeps its texture
 * word, and Kamui isn't called */
Sint32 njSetTextureNum(Uint32 n)
{
    NJS_TEXMEMLIST *texmemlist;
    Uint32 tsp;
    Uint32 texture;

    texmemlist = (NJS_TEXMEMLIST *)_nj_current_texlist->textures[n].texaddr;
    if (texmemlist->globalIndex != _nj_current_texture ||
        _nj_current_texture_bank != texmemlist->bank) {
        _nj_current_texture = texmemlist->globalIndex;
        _nj_current_texture_bank = texmemlist->bank;
        tsp = _nj_current_ctx_->TSPPARAMBUFFER & ~TSP_UV_SIZE;
        _nj_current_ctx_->TSPPARAMBUFFER =
            tsp | (texmemlist->tspparambuffer & TSP_UV_SIZE);
        texture = texmemlist->texparambuffer;
        if ((texture & TEXTURE_ADDRESS_MASK) > TEXTURE_ADDRESS_MAX) {
            return -1;
        }
        _nj_current_ctx_->TexturePARAMBUFFER = texture;
        kmSetVertexRenderState(_nj_current_ctx_);
    }
    return 1;
}

Uint32 njCalcTexture(Uint32 flag)
{
    KMUINT32 size;
    KMUINT32 block;

    /* @hack redundant: the else after the return, which puts
     * kmGetFreeTextureMem's address in R2 and size in R3 (0x8c65b0ec to
     * 0x8c65b0f6 in 1ST_READ.BIN); without it, the two trade and 4 of 31 words
     * differ */
    if (flag == NJD_TEXMEM_MAXSIZE) {
        return _nj_tex_max_free_size;
    } else {
        kmGetFreeTextureMem(&size, &block);
        if (flag == NJD_TEXMEM_FREESIZE) {
            return size;
        }
        return block;
    }
}

void njSetTexturePath(const char *path)
{
    _builtin_strcpy(_nj_current_texture_path, path);
}

/* 1 once the last texture transfer has ended, else -1 */
Sint32 njLoadTextureFinishDMA(void)
{
    if (kmQueryFinishLastTextureDMA() == 0) {
        return 1;
    }
    return -1;
}

/* compact texture memory, then recompute the header words of each of addr's n
 * records that isn't free */
void njGarbageTexture(NJS_TEXMEMLIST *addr, Uint32 n)
{
    Uint32 i;

    kmGarbageCollectTexture();
    for (i = 0; i < n; i++) {
        if (addr[i].globalIndex != TEXMEM_FREE) {
            njSetTextureParam(&addr[i]);
        }
    }
}

NJS_TEXERRSTAT *njGetTextureErrStat(void)
{
    return &_nj_texerr_;
}

void njInitTextureErrStat(void)
{
    _nj_texerr_.n = 0xffffffff;
    _nj_texerr_.globalIndex = 0xffffffff;
    _nj_texerr_.texerr = 0;
    _nj_texerr_.gdstat = 0;
    _nj_texerr_.gderr = 0;
}
