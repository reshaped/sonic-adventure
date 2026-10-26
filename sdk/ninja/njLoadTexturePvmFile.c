/* load a texture list's names and textures from a PVM archive, in a file or in
 * memory */
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>
#include <shinobi.h>
#include <string.h>

#include "ninja/njpvr.h"
#include "ninja/njtexmem.h"

/* a PVMH chunk after its name. count entries follow it, one for each
 * texture: a 2-byte number, then a field for each of the bits PVMH_PVRNAME,
 * PVMH_CATEGORYCODE, PVMH_ENTRYINFO and PVMH_GLOBALINDEX set in status, in
 * that order. PVMH_BANKID adds no field */
struct pvmh_header {
    Uint32 size; /* the bytes after this word */
    Uint16 status;
    Sint16 count;
};
#define PVMH_PVRNAME 0x0001 /* a PVM_NAME_SIZE-byte name */
#define PVMH_CATEGORYCODE 0x0002 /* 2 bytes the loads skip */
#define PVMH_ENTRYINFO 0x0004 /* 2 bytes the loads skip */
#define PVMH_GLOBALINDEX 0x0008 /* a 4-byte global index */
#define PVMH_BANKID 0x0400 /* a palette bank in the index's top 6 bits */

/* a PVRT chunk after its name, before the pixels */
struct pvrt_header {
    Uint32 size; /* the bytes after this word */
    Uint32 type; /* the pixel format and texture type */
    Uint16 width;
    Uint16 height;
};

/* njTexMemList's info of the next free record, which a load fills before
 * njEntryTexMemList takes that record */
static NJS_TEXINFO *load_info;
/* a failed load from memory copies the texture's surface here, and nothing
 * reads it */
static NJS_TEXINFO error_info;
/* the archive in memory: its start, and the next byte to read */
static Uint8 *mem_start;
static Uint8 *mem_ptr;

void njMemOpen(Uint8 *addr)
{
    mem_start = addr;
    mem_ptr = mem_start;
}

void njMemRead(void *buf, Sint32 size)
{
    njMemCopy(buf, mem_ptr, size);
    mem_ptr += size;
}

/* move the next byte to read by offset under GDD_SEEK_CUR, or to offset from
 * the start under GDD_SEEK_SET. another type leaves it */
void njMemFseek(Sint32 offset, Sint32 type)
{
    switch (type) {
    case GDD_SEEK_CUR:
        mem_ptr += offset;
        break;
    case GDD_SEEK_SET:
        mem_ptr = mem_start + offset;
        break;
    }
}

/* get_w copies an entry's 2-byte field and get_l its 4-byte one, a byte at a
 * time: the fields lie at even offsets, so a 4-byte one can be unaligned */
void get_w(void *dst, const void *src)
{
    njMemCopy(dst, src, 2);
}

void get_l(void *dst, const void *src)
{
    njMemCopy(dst, src, 4);
}

/* write the texture path and name to path, adding .PVM unless the name ends in
 * it, ignoring case, and return 1. name needs 4 characters or more: the test
 * reads its last 4 */
Sint32 njCheckFileNamePvm(char *path, const char *name)
{
    char ext[5];
    Sint32 len;

    len = strlen(name);
    _builtin_strcpy(ext, name + (len - 4));
    strupr(ext);
    if (ext[0] == '.') {
        /* @hack spelling: the empty then, with a .PVM name's copy in the else,
         * which puts _slow_strcpy's address in R3 for that copy and in R2 for
         * the copy below, at 0x8c6594f4 and 0x8c659506. as the negated test,
         * with the copy in its then, the two trade and 4 of 59 words differ */
        if (_builtin_strcmp(ext, ".PVM")) {
        } else {
            _builtin_strcpy(path, _nj_current_texture_path);
            strcat(path, name);
            return 1;
        }
        /* @hack redundant: an empty else, which keeps _slow_strcpy's address in
         * R2 for the copy below, at 0x8c659506; without it, R3 holds it, and 2
         * of 59 words differ */
    } else {
    }
    _builtin_strcpy(path, _nj_current_texture_path);
    strcat(path, name);
    strcat(path, ".PVM");
    /* @hack redundant: a label nothing jumps to, which keeps a .PVM name's
     * strcat call, at 0x8c6594fc, apart from the last one; without it, that
     * call joins the last one, 30 of 59 words differ and the function is 6
     * bytes shorter */
done: /* NOLINT(clang-diagnostic-unused-label) */
    return 1;
}

/* the bytes of an entry under status */
Sint32 njCalcPvmStatusSize(Uint16 status)
{
    Sint32 size = 2;

    if (status & PVMH_PVRNAME) {
        size = 2 + PVM_NAME_SIZE;
    }
    if (status & PVMH_CATEGORYCODE) {
        size += 2;
    }
    if (status & PVMH_ENTRYINFO) {
        size += 2;
    }
    if (status & PVMH_GLOBALINDEX) {
        size += 4;
    }
    return size;
}

/* read pvmh's count entries, size bytes each, into texlist's first count
 * names, as ninja.h's njLoadTexturePvmFile says. 1, or -1 if a read fails */
Sint32 njSetTexListToPvm(NJS_TEXLIST *texlist, Sint32 size,
                         struct pvmh_header *pvmh)
{
    Uint16 number;
    Sint16 count;
    Uint16 status;
    Uint8 entry[40];
    NJS_TEXNAME *texname;
    Sint32 i;
    Sint32 offset;

    status = pvmh->status;
    count = pvmh->count;
    for (i = 0; i < count; i++) {
        if (njTexFread(entry, size) < 0) {
            return -1;
        }
        texname = &texlist->textures[i];
        get_w(&number, entry);
        texname->attr = (texname->attr & 0xffff0000) | number;
        offset = 2;
        if (status & PVMH_PVRNAME) {
            njMemCopy(texname->filename, entry + 2, PVM_NAME_SIZE);
            offset = 2 + PVM_NAME_SIZE;
        }
        if (status & PVMH_CATEGORYCODE) {
            offset += 2;
        }
        if (status & PVMH_ENTRYINFO) {
            offset += 2;
        }
        if (status & PVMH_BANKID) {
            if (status & PVMH_GLOBALINDEX) {
                get_l(&texname->texaddr, entry + offset);
                texname->attr |= NJD_TEXATTR_PALGLOBALINDEX;
            }
        } else {
            if (status & PVMH_GLOBALINDEX) {
                get_l(&texname->texaddr, entry + offset);
            }
        }
    }
    return 1;
}

/* read on to the next PVRT chunk and leave the file at its name, skipping
 * other chunks by their size; a PVPL chunk on the way sets the palette's mode
 * and colors. 1, or -1 if a read fails */
Sint32 njSearchPvrChunk(void)
{
    Sint32 skip;
    Uint32 name;
    struct pvpl_header pvpl;
    Uint8 colors[4096]; /* room for the palette's 1024 entries at 4 bytes */
    /* @hack redundant: 12 bytes of frame at sp+8 that no instruction reads or
     * writes, of a type unknown. without them, the frame is 12 bytes smaller,
     * which changes ADD #20,R11 at 0x8c6596fe and 7 pool words: the frame's
     * size, at 0x8c6596a0 and 0x8c6597cc, and 5 frame offsets from
     * 0x8c6597c2. 1 of 130 words differs */
    Uint8 unused[12];
    Sint32 bytes;

    do {
        if (njTexFread(&name, 4) < 0) {
            return -1;
        }
        switch (name) {
        case IMGC:
        case PVMI:
        case COMM:
        case MDLN:
        case CONV:
            if (njTexFread(&skip, 4) < 0) {
                return -1;
            }
            njTexFseek(skip, GDD_SEEK_CUR);
            break;
        case PVPL:
            if (njTexFread(&pvpl, sizeof(pvpl)) != 1) {
                return -1;
            }
            /* @bug logic: the current vertex context's palette bank takes the
             * chunk's mode, not its bank as njLoadPaletteFile gives it, at
             * 0x8c6596e4 */
            njSetPaletteBank(pvpl.mode);
            if (pvpl.mode == NJD_TEXFMT_ARGB_8888) {
                bytes = 4;
            } else {
                bytes = 2;
            }
            if (njTexFread(colors, pvpl.count * bytes) < 0) {
                return -1;
            }
            skip = pvpl.size - (pvpl.count * bytes) - 8;
            njTexFseek(skip, GDD_SEEK_CUR);
            njSetPaletteMode(pvpl.mode);
            njSetPaletteData(pvpl.offset + (pvpl.bank * 16), pvpl.count,
                             colors);
            break;
        case PVRT:
            njTexFseek(-4, GDD_SEEK_CUR);
            break;
        default:
            if (njTexFread(&skip, 4) < 0) {
                return -1;
            }
            njTexFseek(skip, GDD_SEEK_CUR);
            break;
        }
    } while (name != PVRT);
    return 1;
}

/* the index in texlist of the first name whose number, attr's low 16 bits, is
 * number. some name must have it: the search stops only at a match */
Sint32 njSearchEntryCount(NJS_TEXLIST *texlist, Uint32 number)
{
    Sint32 i = 0;

    while ((Uint16)texlist->textures[i++].attr != number) {
    }
    return i - 1;
}

void njSetPvmTextureList(NJS_TEXLIST *texlist, NJS_TEXNAME *texname,
                         char *filename, Sint32 size)
{
    Sint32 i;

    for (i = 0; i < size; i++) {
        texname[i].filename = filename + (i * PVM_NAME_SIZE);
    }
    texlist->textures = texname;
}

Sint32 njLoadTexturePvmFile(const char *filename, NJS_TEXLIST *texlist)
{
    char path[256];
    Uint32 name;
    Sint16 count;
    Uint16 status;
    struct pvmh_header pvmh;
    Uint32 *handle;
    Uint32 *other;
    NJS_TEXNAME *texname;
    /* @hack redundant: a frame word at sp+48 that no instruction reads or
     * writes, of a type unknown. of the 344 bytes below PR, the parameters
     * take sp+336 and sp+340, path sp+80, and the other locals sp+0 to sp+79
     * but this word. without it, the function is 20 bytes shorter and 1213 of
     * 1281 words differ */
    Uint32 unused;
    Sint32 index;
    Sint32 bank_index;
    Uint32 globalIndex;
    Sint32 result;
    Sint32 i;
    Sint32 n;
    Sint32 size;
    Sint32 skip;
    Sint32 bank;
    Sint32 palettized;
    Sint32 listing;
    Uint32 flags;

    bank = -1;
    /* @hack redundant: a store the loop overwrites for each name before any
     * read, which keeps the image's store of 0 at 0x8c6597fc; without it,
     * 1244 of 1281 words differ and the function is 4 bytes shorter */
    palettized = 0;
    njInitTextureErrStat();
    njCheckFileNamePvm(path, filename);
    if ((result = njTexFopen(path)) < 0) {
        return -1;
    }
    if (njTexFread(&name, 4) < 0) {
        njTexFclose();
        return -1;
    }
    if (name == PVMH) {
        if (njTexFread(&pvmh, sizeof(pvmh)) < 0) {
            njTexFclose();
            return -1;
        }
        status = pvmh.status;
        count = pvmh.count;
        texlist->nbTexture = count;
        size = njCalcPvmStatusSize(status);
        njSetTexListToPvm(texlist, size, &pvmh);
        njTexFseek(0, GDD_SEEK_SET);
        njTexFseek(pvmh.size + 8, GDD_SEEK_CUR);
    } else {
        _nj_texerr_.texerr = NJD_TEXERR_HEADER;
        njTexFclose();
        return -1;
    }
    if (njSearchPvrChunk() < 0) {
        njTexFclose();
        return -1;
    }
    for (i = 0; i < count; i++) {
        n = njSearchEntryCount(texlist, i);
        texname = &texlist->textures[n];
        handle = &texname->texaddr;
        palettized = 0;
        /* @hack spelling: the name's attr read through texlist->textures[n]
         * rather than texname, which the image indexes anew, 0x8c65990e to
         * 0x8c659920; through texname, 1161 of 1281 words differ and the
         * function is 16 bytes shorter */
        if (!(texlist->textures[n].attr & NJD_TEXATTR_PALGLOBALINDEX)) {
            globalIndex = texname->texaddr;
            index =
                njSearchTexList(_nj_texmemlist, _nj_texmemsize, globalIndex);
            if (index < 0) {
                listing = UNLISTED;
            } else {
                listing = LISTED;
            }
        } else {
            /* @hack spelling: the global index read through
             * texlist->textures[n] rather than texname, indexed anew,
             * 0x8c65994c to 0x8c659962; through texname, 1127 of 1281 words
             * differ and the function is 16 bytes shorter */
            globalIndex = texlist->textures[n].texaddr & NJD_TEXPALGLOBAL_MASK;
            bank = (texname->texaddr & NJD_TEXBANK_MASK) >> NJD_TEXBANK_SHIFT;
            index =
                njSearchTexList(_nj_texmemlist, _nj_texmemsize, globalIndex);
            if (index < 0) {
                listing = UNLISTED;
            } else {
                bank_index = njSearchTexList3(_nj_texmemlist, _nj_texmemsize,
                                              globalIndex, bank);
                if (bank_index < 0) {
                    listing = OTHER_BANK;
                } else {
                    listing = LISTED;
                }
            }
            palettized = 1;
        }
        switch (listing) {
        case OTHER_BANK:
            /* @bug uninitialized: nothing sets other, so this case stores the
             * record's address through whatever its frame word at sp+56
             * holds, and reads the record back through it, at 0x8c6599ec and
             * 0x8c659a02 */
            /* NOLINTBEGIN(clang-diagnostic-uninitialized) */
            *other = (Uint32)&_nj_texmemlist[index];
            /* NOLINTEND(clang-diagnostic-uninitialized) */
            flags = RECORD(other)->texaddr;
            njTexFseek(4, GDD_SEEK_CUR);
            if (njTexFread(&skip, 4) < 0) {
                _nj_texerr_.n = n;
                _nj_texerr_.globalIndex = globalIndex;
                njTexFclose();
                return -1;
            }
            njTexFseek(skip, GDD_SEEK_CUR);
            /* @bug memory: until njEntryTexMemList stores a record's
             * address at *handle, *handle still holds the name's texaddr from
             * before the load. a cache load before then writes the texture's
             * info 20 bytes past that value, and a failed load its error 66
             * bytes past it, at 0x8c659ac4, 0x8c659adc, 0x8c659c3c and
             * 0x8c659d24 */
            if (flags & TEXMEM_LOADED) {
                switch (texname->attr & NJD_TEXATTR_READAREA_MASK) {
                case 0:
                    njEntryTexMemList(handle, &_nj_texmemlist[index].texinfo,
                                      globalIndex, TEXMEM_LOADED, bank);
                    RECORD(handle)->dummy = 0;
                    RECORD(handle)->count = 1;
                    njSetTextureParam(RECORD(handle));
                    njSetPaletteBankLN(RECORD(handle), bank);
                    break;
                case NJD_TEXATTR_CACHE:
                case NJD_TEXATTR_BOTH:
                    if (!(RECORD(other)->texaddr & TEXMEM_CACHED)) {
                        /* @bug logic: the cache load at 0x8c659ac6 reads the
                         * chunk after this texture's. the file is already past
                         * this texture's chunk, skipped at 0x8c659a3a */
                        if ((result = njLoadCacheFromFile(
                                 texname, &RECORD(handle)->texinfo)) < 0) {
                            RECORD(handle)->dummy = -result;
                            _nj_texerr_.n = n;
                            _nj_texerr_.globalIndex = globalIndex;
                            njTexFclose();
                            return -1;
                        } else {
                            njEntryTexMemList(handle,
                                              &_nj_texmemlist[index].texinfo,
                                              globalIndex, TEXMEM_LOADED, bank);
                            RECORD(handle)->dummy = 0;
                            RECORD(handle)->count = 1;
                            njSetTextureParam(RECORD(handle));
                            njSetPaletteBankLN(RECORD(handle), bank);
                            /* @bug logic: this loop counts with i, the entry
                             * loop's counter, at sp+28 (0x8c659b42,
                             * 0x8c659b92), so the entry loop resumes at number
                             * _nj_texmemsize + 1 */
                            for (i = 0; (Uint32)i < _nj_texmemsize; i++) {
                                if (_nj_texmemlist[i].globalIndex ==
                                    globalIndex) {
                                    _nj_texmemlist[i].texaddr |= TEXMEM_CACHED;
                                    _nj_texmemlist[i].texinfo.texaddr =
                                        RECORD(handle)->texinfo.texaddr;
                                }
                            }
                        }
                    } else {
                        njEntryTexMemList(
                            handle, &_nj_texmemlist[index].texinfo, globalIndex,
                            TEXMEM_CACHED | TEXMEM_LOADED, bank);
                        RECORD(handle)->dummy = 0;
                        RECORD(handle)->count = 1;
                        njSetTextureParam(RECORD(handle));
                        njSetPaletteBankLN(RECORD(handle), bank);
                    }
                }
            } else {
                switch (texname->attr & NJD_TEXATTR_READAREA_MASK) {
                case 0:
                    if ((result = njLoadTexMemFromCache(
                             texname, &_nj_texmemlist[index].texinfo)) < 0) {
                        RECORD(handle)->dummy = -result;
                        _nj_texerr_.n = n;
                        _nj_texerr_.globalIndex = globalIndex;
                        njTexFclose();
                        return -1;
                    } else {
                        njEntryTexMemList(handle,
                                          &_nj_texmemlist[index].texinfo,
                                          globalIndex, TEXMEM_CACHED, bank);
                        RECORD(handle)->dummy = 0;
                        njSetFlagAddrAllTexture(
                            globalIndex, TEXMEM_LOADED,
                            RECORD(handle)->texinfo.texaddr);
                    }
                    break;
                case NJD_TEXATTR_CACHE:
                    njEntryTexMemList(handle, &_nj_texmemlist[index].texinfo,
                                      globalIndex, TEXMEM_CACHED, bank);
                    break;
                case NJD_TEXATTR_BOTH:
                    if ((result = njLoadTexMemFromCache(
                             texname, &_nj_texmemlist[index].texinfo)) < 0) {
                        RECORD(handle)->dummy = -result;
                        _nj_texerr_.n = n;
                        _nj_texerr_.globalIndex = globalIndex;
                        njTexFclose();
                        return -1;
                    } else {
                        njEntryTexMemList(handle,
                                          &_nj_texmemlist[index].texinfo,
                                          globalIndex, TEXMEM_CACHED, bank);
                        RECORD(handle)->dummy = 0;
                        njSetFlagAddrAllTexture(
                            globalIndex, TEXMEM_LOADED,
                            RECORD(handle)->texinfo.texaddr);
                    }
                }
            }
            break;
        case LISTED:
            *handle = (Uint32)&_nj_texmemlist[index];
            switch (texname->attr & NJD_TEXATTR_READAREA_MASK) {
            case 0:
                njTexFseek(4, GDD_SEEK_CUR);
                if (njTexFread(&skip, 4) < 0) {
                    _nj_texerr_.n = n;
                    _nj_texerr_.globalIndex = globalIndex;
                    njTexFclose();
                    return -1;
                }
                njTexFseek(skip, GDD_SEEK_CUR);
                if (RECORD(handle)->texaddr & TEXMEM_LOADED) {
                    RECORD(handle)->count++;
                } else {
                    if ((result = njLoadTexMemFromCache(
                             texname, &_nj_texmemlist[index].texinfo)) < 0) {
                        RECORD(handle)->dummy = -result;
                        _nj_texerr_.n = n;
                        _nj_texerr_.globalIndex = globalIndex;
                        njTexFclose();
                        return -1;
                    } else {
                        RECORD(handle)->dummy = 0;
                        RECORD(handle)->texaddr |= TEXMEM_LOADED;
                        RECORD(handle)->count++;
                        if (palettized) {
                            njSetFlagAddrAllTexture(
                                globalIndex, TEXMEM_LOADED,
                                RECORD(handle)->texinfo.texaddr);
                        }
                    }
                }
                break;
            case NJD_TEXATTR_CACHE:
                if (!(RECORD(handle)->texaddr & TEXMEM_CACHED)) {
                    if ((result = njLoadCacheFromFile(
                             texname, &RECORD(handle)->texinfo)) < 0) {
                        RECORD(handle)->dummy = -result;
                        _nj_texerr_.n = n;
                        _nj_texerr_.globalIndex = globalIndex;
                        njTexFclose();
                        return -1;
                    } else {
                        RECORD(handle)->dummy = 0;
                        RECORD(handle)->texaddr |= TEXMEM_CACHED;
                    }
                } else {
                    njTexFseek(4, GDD_SEEK_CUR);
                    if (njTexFread(&skip, 4) < 0) {
                        _nj_texerr_.n = n;
                        _nj_texerr_.globalIndex = globalIndex;
                        njTexFclose();
                        return -1;
                    }
                    njTexFseek(skip, GDD_SEEK_CUR);
                }
                break;
            case NJD_TEXATTR_BOTH:
                if (RECORD(handle)->texaddr & TEXMEM_LOADED) {
                    RECORD(handle)->count++;
                    if (!(RECORD(handle)->texaddr & TEXMEM_CACHED)) {
                        if ((result = njLoadCacheFromFile(
                                 texname, &RECORD(handle)->texinfo)) < 0) {
                            RECORD(handle)->dummy = -result;
                            _nj_texerr_.n = n;
                            _nj_texerr_.globalIndex = globalIndex;
                            njTexFclose();
                            return -1;
                        } else {
                            RECORD(handle)->dummy = 0;
                            RECORD(handle)->texaddr |= TEXMEM_CACHED;
                        }
                    } else {
                        njTexFseek(4, GDD_SEEK_CUR);
                        if (njTexFread(&skip, 4) < 0) {
                            _nj_texerr_.n = n;
                            _nj_texerr_.globalIndex = globalIndex;
                            njTexFclose();
                            return -1;
                        }
                        njTexFseek(skip, GDD_SEEK_CUR);
                    }
                } else {
                    njTexFseek(4, GDD_SEEK_CUR);
                    if (njTexFread(&skip, 4) < 0) {
                        _nj_texerr_.n = n;
                        _nj_texerr_.globalIndex = globalIndex;
                        njTexFclose();
                        return -1;
                    }
                    njTexFseek(skip, GDD_SEEK_CUR);
                    if ((result = njLoadTexMemFromCache(
                             texname, &RECORD(handle)->texinfo)) < 0) {
                        RECORD(handle)->dummy = -result;
                        _nj_texerr_.n = n;
                        _nj_texerr_.globalIndex = globalIndex;
                        njTexFclose();
                        return -1;
                    } else {
                        RECORD(handle)->dummy = 0;
                        RECORD(handle)->texaddr |= TEXMEM_LOADED;
                        RECORD(handle)->count++;
                        if (palettized) {
                            njSetFlagAddrAllTexture(
                                globalIndex, TEXMEM_LOADED,
                                RECORD(handle)->texinfo.texaddr);
                        }
                    }
                }
            }
            break;
        case UNLISTED:
            switch (texname->attr & NJD_TEXATTR_READAREA_MASK) {
            case 0:
                load_info = njTexMemList(globalIndex);
                if ((result = njLoadTexMemFromFile(texname, load_info)) < 0) {
                    njEntryTexMemList(handle, load_info, globalIndex, 0, bank);
                    RECORD(handle)->dummy = -result;
                    RECORD(handle)->count = 0;
                    _nj_texerr_.n = n;
                    _nj_texerr_.globalIndex = globalIndex;
                    njTexFclose();
                    return -1;
                } else {
                    njEntryTexMemList(handle, load_info, globalIndex,
                                      TEXMEM_LOADED, bank);
                    RECORD(handle)->dummy = 0;
                    njSetTextureParam(RECORD(handle));
                    if (palettized) {
                        njSetPaletteBankLN(RECORD(handle), bank);
                    }
                }
                break;
            case NJD_TEXATTR_CACHE:
                load_info = njTexMemList(globalIndex);
                if ((result = njLoadCacheFromFile(texname, load_info)) < 0) {
                    njEntryTexMemList(handle, load_info, globalIndex, 0, bank);
                    RECORD(handle)->dummy = -result;
                    _nj_texerr_.n = n;
                    _nj_texerr_.globalIndex = globalIndex;
                    njTexFclose();
                    return -1;
                } else {
                    njEntryTexMemList(handle, load_info, globalIndex,
                                      TEXMEM_CACHED, bank);
                    RECORD(handle)->dummy = 0;
                }
                break;
            case NJD_TEXATTR_BOTH:
                load_info = njTexMemList(globalIndex);
                if ((result = njLoadTexMemFromFile(texname, load_info)) < 0) {
                    njEntryTexMemList(handle, load_info, globalIndex, 0, bank);
                    RECORD(handle)->dummy = -result;
                    RECORD(handle)->count = 0;
                    _nj_texerr_.n = n;
                    _nj_texerr_.globalIndex = globalIndex;
                    njTexFclose();
                    return -1;
                } else {
                    /* @bug logic: the test reads 0 as a failed copy, but
                     * njTexDownloadCache fails with -NJD_TEXERR_MAINMEMORY, at
                     * 0x8c66b970, so a failed copy passes and the record is
                     * entered as cached, at 0x8c65a25a and 0x8c65a25c */
                    if ((result = njTexDownloadCache(NULL, load_info)) == 0) {
                        njTexFclose();
                        _nj_texerr_.n = n;
                        _nj_texerr_.globalIndex = globalIndex;
                        return -1;
                    }
                    njEntryTexMemList(handle, load_info, globalIndex,
                                      TEXMEM_CACHED | TEXMEM_LOADED, bank);
                    RECORD(handle)->dummy = 0;
                    njSetTextureParam(RECORD(handle));
                    if (palettized) {
                        njSetPaletteBankLN(RECORD(handle), bank);
                    }
                }
            }
        }
    }
    njTexFclose();
    return 1;
}

/* njSetTexListToPvm from the archive in memory, with no result */
void njSetTexListToPvmM(NJS_TEXLIST *texlist, Sint32 size,
                        struct pvmh_header *pvmh)
{
    Uint16 number;
    Sint16 count;
    Uint32 banked_index;
    Uint16 status;
    Uint8 entry[40];
    NJS_TEXNAME *texname;
    Sint32 i;
    Sint32 offset;

    status = pvmh->status;
    count = pvmh->count;
    for (i = 0; i < count; i++) {
        njMemRead(entry, size);
        texname = &texlist->textures[i];
        get_w(&number, entry);
        texname->attr = (texname->attr & 0xffff0000) | number;
        offset = 2;
        if (status & PVMH_PVRNAME) {
            njMemCopy(texname->filename, entry + 2, PVM_NAME_SIZE);
            offset = 2 + PVM_NAME_SIZE;
        }
        if (status & PVMH_CATEGORYCODE) {
            offset += 2;
        }
        if (status & PVMH_ENTRYINFO) {
            offset += 2;
        }
        if (status & PVMH_BANKID) {
            if (status & PVMH_GLOBALINDEX) {
                /* @bug logic: a banked global index goes to banked_index, which
                 * nothing reads, where njSetTexListToPvm stores it in the
                 * name's texaddr (BSR at 0x8c65a38c, R4 a frame word). the name
                 * keeps the texaddr it had, though it still takes
                 * NJD_TEXATTR_PALGLOBALINDEX */
                get_l(&banked_index, entry + offset);
                texname->attr |= NJD_TEXATTR_PALGLOBALINDEX;
            }
        } else {
            if (status & PVMH_GLOBALINDEX) {
                get_l(&texname->texaddr, entry + offset);
            }
        }
    }
}

/* njSearchPvrChunk in the archive in memory, with no result */
void njSearchPvrChunkM(void)
{
    Sint32 skip;
    Uint32 name;
    struct pvpl_header pvpl;
    Uint8 colors[4096];
    Sint32 bytes;

    do {
        njMemRead(&name, 4);
        switch (name) {
        case IMGC:
        case PVMI:
        case COMM:
        case MDLN:
        case CONV:
            njMemRead(&skip, 4);
            njMemFseek(skip, GDD_SEEK_CUR);
            break;
        case PVPL:
            njMemRead(&pvpl, sizeof(pvpl));
            /* @bug logic: the current vertex context's palette bank takes the
             * chunk's mode, not its bank, at 0x8c65a444 */
            njSetPaletteBank(pvpl.mode);
            /* @bug uninitialized: the test compares the chunk's bank with
             * NJD_TEXFMT_ARGB_8888 where njSearchPvrChunk compares its mode,
             * so a 32-bit palette in a bank other than 6, that constant's
             * value, reads count * 2 bytes into colors, and
             * njSetPaletteData, set to 32-bit colors, copies count * 4 from
             * it, at 0x8c65a44c and 0x8c65a4a0 */
            if (pvpl.bank == NJD_TEXFMT_ARGB_8888) {
                bytes = 4;
            } else {
                bytes = 2;
            }
            njMemRead(colors, pvpl.count * bytes);
            skip = pvpl.size - (pvpl.count * bytes) - 8;
            njMemFseek(skip, GDD_SEEK_CUR);
            njSetPaletteMode(pvpl.mode);
            njSetPaletteData(pvpl.offset + (pvpl.bank * 16), pvpl.count,
                             colors);
            break;
        case PVRT:
            njMemFseek(-4, GDD_SEEK_CUR);
            break;
        default:
            njMemRead(&skip, 4);
            njMemFseek(skip, GDD_SEEK_CUR);
            break;
        }
    } while (name != PVRT);
}

/* read on past the next PVRT chunk, skipping other chunks by their size. info
 * takes the texture's width, height and pixels' address, and its type in
 * Type's high 16 bits */
void njSetInfo(NJS_TEXINFO *info)
{
    Uint32 name;
    Sint32 skip;
    struct pvrt_header pvrt;

    do {
        njMemRead(&name, 4);
        switch (name) {
        case PVRT:
            njMemRead(&pvrt, sizeof(pvrt));
            info->texsurface.nWidth = pvrt.width;
            info->texsurface.nHeight = pvrt.height;
            info->texsurface.Type = pvrt.type << 16;
            break;
        default:
            njMemRead(&skip, 4);
            njMemFseek(skip, GDD_SEEK_CUR);
        }
    } while (name != PVRT);
    info->texaddr = mem_ptr;
    njMemFseek(pvrt.size - 8, GDD_SEEK_CUR);
}

Sint32 njLoadTexturePvmMemory(Uint8 *addr, NJS_TEXLIST *texlist)
{
    NJS_TEXNAME *texname;
    Uint32 *handle;
    Sint32 bank;
    Sint32 size;
    Sint32 i;
    Sint32 palettized;
    Uint32 flags;
    Sint32 n;
    Sint32 listing;
    Sint32 result;
    Sint32 index;
    Sint32 bank_index;
    Uint32 globalIndex;
    Uint32 name;
    Uint16 status;
    Sint16 count;
    struct pvmh_header pvmh;
    NJS_TEXINFO info;

    bank = -1;
    njInitTextureErrStat();
    njMemOpen(addr);
    njMemRead(&name, 4);
    if (name == PVMH) {
        njMemRead(&pvmh, sizeof(pvmh));
        status = pvmh.status;
        count = pvmh.count;
        texlist->nbTexture = count;
        size = njCalcPvmStatusSize(status);
        njSetTexListToPvmM(texlist, size, &pvmh);
        njMemFseek(0, GDD_SEEK_SET);
        njMemFseek(pvmh.size + 8, GDD_SEEK_CUR);
    } else {
        _nj_texerr_.texerr = NJD_TEXERR_HEADER;
        return -1;
    }
    njSearchPvrChunkM();
    for (i = 0; i < count; i++) {
        n = njSearchEntryCount(texlist, i);
        /* @hack type: a cast to texname's own type, which keeps the name's
         * address in R6 for the test of its attr below, 0x8c65a65a to
         * 0x8c65a668; without it, 552 of 671 words differ and the function is
         * 4 bytes longer */
        /* NOLINTNEXTLINE(readability-redundant-casting) */
        texname = (NJS_TEXNAME *)&texlist->textures[n];
        handle = &texname->texaddr;
        palettized = 0;
        /* @hack spelling: the name's attr read through texlist->textures[n]
         * rather than texname, which reads the name at R6, at 0x8c65a668;
         * through texname, 552 of 671 words differ and the function is 4
         * bytes longer */
        if (!(texlist->textures[n].attr & NJD_TEXATTR_PALGLOBALINDEX)) {
            globalIndex = texname->texaddr;
            index =
                njSearchTexList(_nj_texmemlist, _nj_texmemsize, globalIndex);
            if (index < 0) {
                listing = UNLISTED;
            } else {
                listing = LISTED;
            }
        } else {
            /* @hack spelling: the global index read through
             * texlist->textures[n] rather than texname, indexed anew,
             * 0x8c65a68a to 0x8c65a694; through texname, 571 of 671 words
             * differ and the function is 4 bytes shorter */
            globalIndex = texlist->textures[n].texaddr & NJD_TEXPALGLOBAL_MASK;
            bank = (texname->texaddr & NJD_TEXBANK_MASK) >> NJD_TEXBANK_SHIFT;
            index =
                njSearchTexList(_nj_texmemlist, _nj_texmemsize, globalIndex);
            if (index < 0) {
                listing = UNLISTED;
            } else {
                bank_index = njSearchTexList3(_nj_texmemlist, _nj_texmemsize,
                                              globalIndex, bank);
                if (bank_index < 0) {
                    listing = OTHER_BANK;
                } else {
                    listing = LISTED;
                }
            }
            palettized = 1;
        }
        switch (listing) {
        case OTHER_BANK:
            /* @bug logic: the switches below read the record's bank as the
             * name's attr, at 0x8c65a710. texname now points at the record
             * listed in another bank, at 0x8c65a6fc, and the loads below take
             * the record as the name. no bank the loaders record, 0 to 63 or
             * -1, selects NJD_TEXATTR_CACHE or NJD_TEXATTR_BOTH: 0 to 63 takes
             * case 0, and -1 takes no case, so the name gets no record */
            texname = (NJS_TEXNAME *)&_nj_texmemlist[index];
            flags = _nj_texmemlist[index].texaddr;
            njSetInfo(&info);
            /* @bug memory: until njEntryTexMemList stores a record's
             * address at *handle, *handle still holds the name's texaddr from
             * before the load, and a failed load writes its error 66 bytes
             * past that value, at 0x8c65a976 */
            if (flags & TEXMEM_LOADED) {
                switch (texname->attr & NJD_TEXATTR_READAREA_MASK) {
                case 0:
                    njEntryTexMemList(handle, &_nj_texmemlist[index].texinfo,
                                      globalIndex, TEXMEM_LOADED, bank);
                    RECORD(handle)->dummy = 0;
                    RECORD(handle)->count = 1;
                    njSetTextureParam(RECORD(handle));
                    njSetPaletteBankLN(RECORD(handle), bank);
                    break;
                case NJD_TEXATTR_CACHE:
                case NJD_TEXATTR_BOTH:
                    if (!(((NJS_TEXMEMLIST *)texname)->texaddr &
                          TEXMEM_CACHED)) {
                        njEntryTexMemList(handle,
                                          &_nj_texmemlist[index].texinfo,
                                          globalIndex, TEXMEM_LOADED, bank);
                        RECORD(handle)->dummy = 0;
                        RECORD(handle)->count = 1;
                        njSetTextureParam(RECORD(handle));
                        njSetPaletteBankLN(RECORD(handle), bank);
                        for (i = 0; (Uint32)i < _nj_texmemsize; i++) {
                            if (_nj_texmemlist[i].globalIndex == globalIndex) {
                                _nj_texmemlist[i].texaddr |= TEXMEM_CACHED;
                                _nj_texmemlist[i].texinfo.texaddr =
                                    RECORD(handle)->texinfo.texaddr;
                            }
                        }
                    } else {
                        njEntryTexMemList(
                            handle, &_nj_texmemlist[index].texinfo, globalIndex,
                            TEXMEM_CACHED | TEXMEM_LOADED, bank);
                        RECORD(handle)->dummy = 0;
                        RECORD(handle)->count = 1;
                        njSetTextureParam(RECORD(handle));
                        njSetPaletteBankLN(RECORD(handle), bank);
                    }
                }
            } else {
                switch (texname->attr & NJD_TEXATTR_READAREA_MASK) {
                case 0:
                    if ((result = njLoadTexMemFromCache(
                             texname, &_nj_texmemlist[index].texinfo)) < 0) {
                        RECORD(handle)->dummy = -result;
                        _nj_texerr_.n = n;
                        _nj_texerr_.globalIndex = globalIndex;
                        return -1;
                    } else {
                        njEntryTexMemList(handle,
                                          &_nj_texmemlist[index].texinfo,
                                          globalIndex, TEXMEM_CACHED, bank);
                        RECORD(handle)->dummy = 0;
                        RECORD(handle)->count = 1;
                        njSetFlagAddrAllTexture(
                            globalIndex, TEXMEM_LOADED,
                            RECORD(handle)->texinfo.texaddr);
                    }
                    break;
                case NJD_TEXATTR_CACHE:
                    njEntryTexMemList(handle, &_nj_texmemlist[index].texinfo,
                                      globalIndex, TEXMEM_CACHED, bank);
                    break;
                case NJD_TEXATTR_BOTH:
                    if ((result = njLoadTexMemFromCache(
                             texname, &_nj_texmemlist[index].texinfo)) < 0) {
                        RECORD(handle)->dummy = -result;
                        _nj_texerr_.n = n;
                        _nj_texerr_.globalIndex = globalIndex;
                        return -1;
                    } else {
                        njEntryTexMemList(handle,
                                          &_nj_texmemlist[index].texinfo,
                                          globalIndex, TEXMEM_CACHED, bank);
                        RECORD(handle)->dummy = 0;
                        RECORD(handle)->count = 1;
                        njSetFlagAddrAllTexture(
                            globalIndex, TEXMEM_LOADED,
                            RECORD(handle)->texinfo.texaddr);
                    }
                }
            }
            break;
        case LISTED:
            njSetInfo(&info);
            *handle = (Uint32)&_nj_texmemlist[index];
            switch (texname->attr & NJD_TEXATTR_READAREA_MASK) {
            case 0:
                if (RECORD(handle)->texaddr & TEXMEM_LOADED) {
                    RECORD(handle)->count++;
                } else {
                    if ((result = njLoadTexMemFromCache(
                             texname, &RECORD(handle)->texinfo)) < 0) {
                        RECORD(handle)->dummy = -result;
                        _nj_texerr_.n = n;
                        _nj_texerr_.globalIndex = globalIndex;
                        return -1;
                    } else {
                        RECORD(handle)->dummy = 0;
                        RECORD(handle)->texaddr |= TEXMEM_LOADED;
                        RECORD(handle)->count++;
                        njSetTextureParam(RECORD(handle));
                        if (palettized) {
                            njSetFlagAddrAllTexture(
                                globalIndex, TEXMEM_LOADED,
                                RECORD(handle)->texinfo.texaddr);
                        }
                    }
                }
                break;
            case NJD_TEXATTR_CACHE:
                RECORD(handle)->texinfo.texaddr = info.texaddr;
                RECORD(handle)->texaddr |= TEXMEM_CACHED;
                break;
            case NJD_TEXATTR_BOTH:
                if (RECORD(handle)->texaddr & TEXMEM_LOADED) {
                    RECORD(handle)->count++;
                    if (!(RECORD(handle)->texaddr & TEXMEM_CACHED)) {
                        RECORD(handle)->texinfo.texaddr = info.texaddr;
                        RECORD(handle)->texaddr |= TEXMEM_CACHED;
                    }
                } else {
                    if ((result = njLoadTexMemFromCache(
                             texname, &RECORD(handle)->texinfo)) < 0) {
                        RECORD(handle)->dummy = -result;
                        _nj_texerr_.n = n;
                        _nj_texerr_.globalIndex = globalIndex;
                        return -1;
                    } else {
                        RECORD(handle)->dummy = 0;
                        RECORD(handle)->texaddr |= TEXMEM_LOADED;
                        RECORD(handle)->count++;
                        njSetTextureParam(RECORD(handle));
                        if (palettized) {
                            njSetFlagAddrAllTexture(
                                globalIndex, TEXMEM_LOADED,
                                RECORD(handle)->texinfo.texaddr);
                        }
                    }
                }
            }
            break;
        case UNLISTED:
            njSetInfo(&info);
            switch (texname->attr & NJD_TEXATTR_READAREA_MASK) {
            case 0:
                load_info = njTexMemList(globalIndex);
                njMemCopy(load_info, &info, sizeof(NJS_TEXINFO));
                if ((result = njLoadTexMemFromMemory(texname, load_info)) < 0) {
                    njMemCopy(&error_info.texsurface, &load_info->texsurface,
                              sizeof(NJS_TEXSURFACE));
                    njEntryTexMemList(handle, load_info, globalIndex, 0, bank);
                    RECORD(handle)->dummy = -result;
                    RECORD(handle)->count = 0;
                    _nj_texerr_.n = n;
                    _nj_texerr_.globalIndex = globalIndex;
                    return -1;
                } else {
                    /* @hack spelling: &texname->texaddr in place of handle, its
                     * equal, which the image computes from texname's frame
                     * word plus 8, at 0x8c65aa38 and 0x8c65aa3c, rather than
                     * taking handle's R14; through handle, 144 of 671 words
                     * differ and the function is 4 bytes shorter */
                    njEntryTexMemList(&texname->texaddr, load_info, globalIndex,
                                      TEXMEM_LOADED, bank);
                    RECORD(handle)->dummy = 0;
                    njSetTextureParam(RECORD(handle));
                    if (palettized) {
                        njSetPaletteBankLN(RECORD(handle), bank);
                    }
                }
                break;
            case NJD_TEXATTR_CACHE:
                load_info = njTexMemList(globalIndex);
                njMemCopy(load_info, &info, sizeof(NJS_TEXINFO));
                njEntryTexMemList(handle, load_info, globalIndex, TEXMEM_CACHED,
                                  bank);
                break;
            case NJD_TEXATTR_BOTH:
                load_info = njTexMemList(globalIndex);
                njMemCopy(load_info, &info, sizeof(NJS_TEXINFO));
                if ((result = njLoadTexMemFromMemory(texname, load_info)) < 0) {
                    njMemCopy(&error_info.texsurface, &load_info->texsurface,
                              sizeof(NJS_TEXSURFACE));
                    njEntryTexMemList(handle, load_info, globalIndex, 0, bank);
                    RECORD(handle)->dummy = -result;
                    RECORD(handle)->count = 0;
                    return -1;
                } else {
                    njEntryTexMemList(handle, load_info, globalIndex,
                                      TEXMEM_CACHED | TEXMEM_LOADED, bank);
                    njSetTextureParam(RECORD(handle));
                    RECORD(handle)->dummy = 0;
                    if (palettized) {
                        njSetPaletteBankLN(RECORD(handle), bank);
                    }
                }
                /* @hack redundant: a break that ends this switch's last case,
                 * which leaves R2 to step the entry counter, 0x8c65ab2e to
                 * 0x8c65ab32; without it, R3 steps it, and 3 of 671 words
                 * differ */
                break;
            }
            /* @hack redundant: a break that ends the last case of the switch on
             * listing, which lets case OTHER_BANK's NJD_TEXATTR_CACHE case,
             * for a texture not loaded, branch to the njEntryTexMemList call
             * in case UNLISTED's, 0x8c65a820 to 0x8c65aa74; without it, that
             * case makes the call itself, 376 of 671 words differ and the
             * function is 8 bytes longer */
            break;
        }
    }
    return 1;
}
