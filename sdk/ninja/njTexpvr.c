/* PVR texture header reading and surface creation */
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>

#include "ninja/njpvr.h"

/* a PVRT chunk after its name, before the pixels */
struct pvrt_header {
    Uint32 size; /* the bytes after this word */
    Uint32 type; /* an NJD_TEXFMT_ pixel format and texture type */
    Uint16 width;
    Uint16 height;
};

Sint32 _njLoadPvrHeader(Sint32 *width, Sint32 *height, Sint32 *type,
                        Sint32 *size);

/* read the header of the next PVRT chunk in the file njTexFopen opened,
 * stepping over the chunks before it. on success *size, *width, *height and
 * *type hold its fields. it returns 1, or -1 when a read fails. it returns
 * -NJD_TEXERR_HEADER, with _nj_texerr_.texerr set, for a width or height other
 * than a power of 2 from 8 to 1024 */
Sint32 _njLoadPvrHeader(Sint32 *width, Sint32 *height, Sint32 *type,
                        Sint32 *size)
{
    struct pvrt_header header;
    Uint32 skip;
    Uint32 name;

    do {
        if (njTexFread(&name, 4) < 0) {
            return -1;
        }
        switch (name) {
        case PVRT:
            if (njTexFread(&header, sizeof(header)) < 0) {
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
    *size = header.size;
    *width = header.width;
    switch (*width) {
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
    case 256:
    case 512:
    case 1024:
        break;
    default:
        _nj_texerr_.texerr = NJD_TEXERR_HEADER;
        return -NJD_TEXERR_HEADER;
    }
    *height = header.height;
    switch (*height) {
    case 8:
    case 16:
    case 32:
    case 64:
    case 128:
    case 256:
    case 512:
    case 1024:
        break;
    default:
        _nj_texerr_.texerr = NJD_TEXERR_HEADER;
        return -NJD_TEXERR_HEADER;
    }
    *type = header.type;
    return 1;
}

/* read the next PVRT chunk's header, as _njLoadPvrHeader does, and create
 * info's surface for it in video memory. a type of NJD_TEXFMT_ABGR becomes
 * NJD_TEXFMT_ABGR_MM under NJD_TEXATTR_AUTOMIPMAP. the surface's Type takes
 * the type in its top half, ORed in. it returns 1, or _njLoadPvrHeader's
 * negative result. it returns -NJD_TEXERR_SURFACE, with _nj_texerr_.texerr
 * set, when Kamui can't create the surface */
Sint32 njGetSurfacePvr(NJS_TEXINFO *info, Uint32 attr, Sint32 *size)
{
    Sint32 width;
    Sint32 height;
    Sint32 type;
    Sint32 result;

    /* @hack spelling: the assignment inside the test, which puts the copy of
     * the result in BT/S's delay slot (MOV R0,R4 at 0x8c6752ba in
     * 1ST_READ.BIN); as a statement before the test, the copy comes first and
     * 3 of 67 words differ */
    if ((result = _njLoadPvrHeader(&width, &height, &type, size)) < 0) {
        return result;
    }
    if (type == NJD_TEXFMT_ABGR) {
        if (attr & NJD_TEXATTR_AUTOMIPMAP) {
            type = (type & NJD_TEXFMT_COLOR_MASK) | NJD_TEXFMT_ABGR_MM;
        }
    }
    if (kmCreateTextureSurface((PKMSURFACEDESC)&info->texsurface, width, height,
                               type) != 0) {
        _nj_texerr_.texerr = NJD_TEXERR_SURFACE;
        return -NJD_TEXERR_SURFACE;
    }
    /* @hack redundant: Type's low half ORed into Type again, which keeps info
     * in R14 across the calls (MOV R4,R14 at 0x8c6752a2 in 1ST_READ.BIN);
     * without it, info goes through the frame, the function is 12 bytes
     * shorter, and 63 of 67 words differ */
    info->texsurface.Type |=
        (((Uint32)type << 16) & 0xffff0000) | (info->texsurface.Type & 0xffff);
    return 1;
}

/* njGetSurfacePvr, except that njCreateTextureSurface describes the surface
 * without allocating video memory. the surface's Type becomes type << 16,
 * with 0 in its low half */
Sint32 njGetInfoPvr(NJS_TEXINFO *info, Uint32 attr, Sint32 *size)
{
    Sint32 width;
    Sint32 height;
    Sint32 type;
    Sint32 result;

    /* @hack spelling: the assignment inside the test, which puts the copy of
     * the result in BT/S's delay slot (MOV R0,R4 at 0x8c67533e in
     * 1ST_READ.BIN); as a statement before the test, the copy comes first and
     * 3 of 53 words differ */
    if ((result = _njLoadPvrHeader(&width, &height, &type, size)) < 0) {
        return result;
    }
    if (type == NJD_TEXFMT_ABGR) {
        if (attr & NJD_TEXATTR_AUTOMIPMAP) {
            type = (type & NJD_TEXFMT_COLOR_MASK) | NJD_TEXFMT_ABGR_MM;
        }
    }
    if (njCreateTextureSurface(&info->texsurface, width, height, type) != 1) {
        _nj_texerr_.texerr = NJD_TEXERR_SURFACE;
        return -NJD_TEXERR_SURFACE;
    }
    return 1;
}
