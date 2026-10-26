/* the chunk names of PVR texture, PVP palette and PVM archive files, the
 * chunks that more than one of Ninja's texture loads read, and the padding a
 * mipmapped surface takes */
#ifndef NJPVR_H
#define NJPVR_H
#include <sg_xpt.h>

/* chunk names, each its four ASCII bytes as a little-endian word. GBIX holds a
 * texture's global index, PVRT its pixels and PVPL a palette's colors */
#define GBIX 0x58494247
#define PVRT 0x54525650
#define PVPL 0x4c505650
/* a PVM archive's first chunk, and five chunks the PVM loads step over */
#define PVMH 0x484d5650
#define IMGC 0x43474d49
#define PVMI 0x494d5650
#define COMM 0x4d4d4f43
#define MDLN 0x4e4c444d
#define CONV 0x564e4f43

struct gbix_chunk {
    Uint32 name;
    Uint32 size; /* the bytes after this word */
    Uint32 globalIndex;
};

/* a PVRT chunk from its name to the pixels */
struct pvrt_chunk {
    Uint32 name;
    Uint32 size; /* the bytes after this word */
    Uint32 type; /* an NJD_TEXFMT_ pixel format and texture type */
    Uint16 width;
    Uint16 height;
};

/* a PVPL chunk after its name, before the colors */
struct pvpl_header {
    Uint32 size; /* the bytes after this word */
    Uint16 mode; /* the colors' NJD_TEXFMT_ format */
    Sint16 bank;
    Sint16 offset; /* the colors fill palette entries from bank * 16 + offset */
    Sint16 count;
};

/* the bytes njCreateTextureSurface adds to a mipmapped surface's TextureSize
 * past its entry in njTexLoad.c's mipmap_sizes. a copy into the texture cache
 * leaves them out */
#define NJD_PVR_MM_DUMMYBYTE 4

#endif
