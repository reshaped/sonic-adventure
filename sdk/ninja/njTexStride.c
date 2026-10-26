/* render to texture, stride width and frame buffer BMP capture */
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

/* a BMP file header as C lays it out: 16 bytes, where the file's header takes
 * 14. njFrameBufferBmp writes the file's header a field at a time, and reads
 * only size from this one, which holds the pixels' size */
struct bmp_file_header {
    Uint16 type;
    Uint32 size;
    Uint16 reserved1;
    Uint16 reserved2;
    Uint32 offset;
};

#define BMP_FILE_HEADER_SIZE 14
#define BMP_INFO_HEADER_SIZE 40
#define BMP_PIXELS (BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE)
/* "BM", the file's first two bytes, as a little-endian half-word */
#define BMP_TYPE 0x4d42
/* the address of pixel x of row y, past the headers */
#define PIXEL(bmp, width, x, y)                                                \
    ((bmp) + ((((y) * (width)) + (x)) * 3) + BMP_PIXELS)

void njRenderTextureNum(Uint32 n)
{
    NJS_TEXMEMLIST *texmemlist;

    texmemlist = (NJS_TEXMEMLIST *)_nj_current_texlist->textures[n].texaddr;
    kmSetPixelClipping(0, 0, texmemlist->texinfo.texsurface.nWidth,
                       texmemlist->texinfo.texsurface.nHeight);
    kmRenderTexture((PKMSURFACEDESC)&texmemlist->texinfo.texsurface);
    kmSetPixelClipping(0, 0, _nj_frame_size_w, _nj_frame_size_h);
}

void njRenderTextureNumG(Uint32 globalIndex)
{
    Sint32 n;

    n = njSearchTexList(_nj_texmemlist, _nj_texmemsize, globalIndex);
    if (n >= 0) {
        kmSetPixelClipping(0, 0, _nj_texmemlist[n].texinfo.texsurface.nWidth,
                           _nj_texmemlist[n].texinfo.texsurface.nHeight);
        kmRenderTexture((PKMSURFACEDESC)&_nj_texmemlist[n].texinfo.texsurface);
        kmSetPixelClipping(0, 0, _nj_frame_size_w, _nj_frame_size_h);
    }
}

void njSetRenderWidth(Uint32 nWidth)
{
    kmSetStrideWidth(nWidth);
}

/* put_b, put_l and put_w store their value's low 1, 4 and 2 bytes at p, low
 * byte first. they store a byte at a time, so p needs no alignment */
void put_b(Uint32 b, char *p)
{
    p[0] = b;
}

void put_l(Uint32 l, char *p)
{
    p[0] = l;
    p[1] = l >> 8;
    p[2] = l >> 16;
    p[3] = l >> 24;
}

void put_w(Uint32 w, char *p)
{
    p[0] = w;
    p[1] = w >> 8;
}

void njFrameBufferBmp(char *bmp, char *buffer)
{
    struct bmp_file_header header;
    KMISYSTEMMETRICS metrics;
    NJS_TEXSURFACE *surface;
    Sint32 width;
    Sint32 height;
    char *src;
    KMDWORD *texture;
    /* @hack redundant: pixels, which nothing reads, takes the pixels' address
     * in each kmuConvertFBtoBMP call below. the stores have SHC compute the
     * address in R4 ahead of the switch (ADD #54,R4 at 0x8c65ae7a in
     * 1ST_READ.BIN); as a cast in its place, 59 of 206 words differ */
    void *pixels;
    Sint32 x;
    Sint32 y;
    Uint32 shown;

    kmfGetInternalSystemMetrics(&metrics);
    shown = metrics.FBParams.FBStartAddr[0];
    if (_nj_framebuffer_surface0_.TextureSize == shown) {
        surface = &_nj_framebuffer_surface0_;
    } else {
        surface = &_nj_framebuffer_surface1_;
    }
    width = surface->nWidth;
    height = surface->nHeight;
    src = buffer;
    texture = (KMDWORD *)buffer;
    header.size = width * height * 3;
    kmGetTexture(texture, (PKMSURFACEDESC)surface);
    if (bmp == NULL) {
        return;
    }

    put_w(BMP_TYPE, bmp);
    /* @bug logic: the file's size field takes header.size, the pixels' size,
     * where BMP wants the whole file's (MOV.L @(R0,R15),R4 in the slot of the
     * BSR to put_l at 0x8c65ae06) */
    put_l(header.size, bmp + 2);
    put_l(0, bmp + 6);
    put_l(BMP_PIXELS, bmp + 10);
    put_l(BMP_INFO_HEADER_SIZE, bmp + 14);
    put_l(width, bmp + 18);
    put_l(height, bmp + 22);
    put_w(1, bmp + 26);
    put_w(24, bmp + 28);
    put_l(0, bmp + 30);
    put_l(0, bmp + 34);
    put_l(0, bmp + 38);
    put_l(0, bmp + 42);
    put_l(0, bmp + 46);
    put_l(0, bmp + 50);

    switch (_nj_frame_mode_) {
    case NJD_FRAMEBUFFER_MODE_RGB565:
        kmuConvertFBtoBMP(pixels = bmp + BMP_PIXELS, texture, width, height,
                          KM_DSPBPP_RGB565);
        break;
    case NJD_FRAMEBUFFER_MODE_RGB555:
        kmuConvertFBtoBMP(pixels = bmp + BMP_PIXELS, texture, width, height,
                          KM_DSPBPP_RGB555);
        break;
    case NJD_FRAMEBUFFER_MODE_ARGB1555:
        kmuConvertFBtoBMP(pixels = bmp + BMP_PIXELS, texture, width, height,
                          KM_DSPBPP_ARGB1555);
        break;
    case NJD_FRAMEBUFFER_MODE_RGB888:
        for (y = height - 1; y >= 0; y--) {
            for (x = 0; x < width; x++) {
                /* @bug memory: each pixel's first byte goes to its
                 * byte 3, which is the next pixel's byte 0. the frame's top
                 * right pixel is the last in bmp, so its first byte lands one
                 * past the pixels, and bmp needs a byte to spare (MOV #57,R0
                 * at 0x8c65aee4 in 1ST_READ.BIN) */
                PIXEL(bmp, width, x, y)[3] = *src++;
                PIXEL(bmp, width, x, y)[1] = *src++;
                PIXEL(bmp, width, x, y)[2] = *src++;
            }
        }
        break;
    case NJD_FRAMEBUFFER_MODE_ARGB8888:
        for (y = height - 1; y >= 0; y--) {
            for (x = 0; x < width; x++) {
                /* @bug memory: as in the RGB888 copy (MOV #57,R0 at
                 * 0x8c65af24 in 1ST_READ.BIN) */
                PIXEL(bmp, width, x, y)[3] = *src++;
                PIXEL(bmp, width, x, y)[1] = *src++;
                PIXEL(bmp, width, x, y)[2] = *src++;
                src++;
            }
        }
        break;
    }
}
