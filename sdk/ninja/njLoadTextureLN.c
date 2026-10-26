/* load one texture of a texture list into a texture record, from its file or
 * from memory. each loader switches on where the texture stands in the
 * records, njtexmem.h's OTHER_BANK, LISTED or UNLISTED, and then on the name's
 * read area */
#include <ninja.h>
#include <sg_xpt.h>

#include "ninja/njpvr.h"
#include "ninja/njtexmem.h"

Sint32 njLoadFileTexture(NJS_TEXLIST *texlist, Uint32 n);
Sint32 njLoadMemoryTexture(NJS_TEXLIST *texlist, Uint32 n);

/* njLoadTextureLN's loaders: 0 for a texture file, 1 for a name under
 * NJD_TEXATTR_TYPE_MEMORY */
Sint32 (*_njloadtexture[])(NJS_TEXLIST *texlist, Uint32 n) = {
    njLoadFileTexture,
    njLoadMemoryTexture,
};

/* njTexMemList's info of the next free record, which a load fills before
 * njEntryTexMemList takes that record */
static NJS_TEXINFO *load_info;
/* the info a record takes when a texture's file fails to open, and when a
 * texture without a palette has the global index -1. a failed open sets its
 * surface to _nj_tex_err_surface before the record takes it. a failed load
 * from memory copies its texture's surface here, though its record takes
 * load_info, so the global index -1 takes the surface the last failure left,
 * or zeros before any */
static NJS_TEXINFO error_info;

Sint32 njLoadTextureLN(NJS_TEXLIST *texlist, Uint32 n)
{
    Int source;

    source = 0;
    if (texlist->textures[n].attr & NJD_TEXATTR_TYPE_MEMORY) {
        source = 1;
    }
    return _njloadtexture[source](texlist, n);
}

/* njLoadTextureLN for a name not under NJD_TEXATTR_TYPE_MEMORY */
Sint32 njLoadFileTexture(NJS_TEXLIST *texlist, Uint32 n)
{
    Uint32 *handle;
    Uint32 *previous_handle;
    NJS_TEXMEMLIST *other;
    NJS_TEXNAME *texname;
    NJS_TEXNAME *previous;
    /* @hack redundant: a frame word at sp+296 that no instruction reads or
     * writes, among the 320 bytes of locals below the arguments and PR.
     * without it, the frame is 4 bytes smaller, which changes 48 pool words of
     * frame offsets, and 0 of 1497 words differ in each image */
    Sint32 unused;
    Sint32 result;
    char path[256];
    Sint32 index;
    Sint32 bank_index;
    Sint32 palettized;
    Uint32 globalIndex;
    Uint32 bank;
    Sint32 listing;
    Sint32 type;
    Uint32 flags;
    Uint32 i;

    palettized = 0;
    bank = -1;
    texname = &texlist->textures[n];
    handle = &texname->texaddr;
    if (texname->attr & NJD_TEXATTR_TEXCONTINUE) {
        if (n > 0) {
            previous = &texlist->textures[n - 1];
            previous_handle = &previous->texaddr;
            bank = (texname->texaddr & NJD_TEXBANK_MASK) >> NJD_TEXBANK_SHIFT;
            /* @bug uninitialized: nothing on this path sets globalIndex, so
             * njSearchTexList searches for, and njEntryTexMemList records,
             * whatever its frame word at sp+20 holds, at 0x8c668fd2 and
             * 0x8c669002 in 1ST_READ.BIN */
            /* NOLINTBEGIN(clang-diagnostic-uninitialized) */
            index =
                njSearchTexList(_nj_texmemlist, _nj_texmemsize, globalIndex);
            /* NOLINTEND(clang-diagnostic-uninitialized) */
            if (_nj_texmemlist[index].texaddr & TEXMEM_CACHED) {
                njEntryTexMemList(handle, &RECORD(previous_handle)->texinfo,
                                  globalIndex, TEXMEM_CACHED | TEXMEM_LOADED,
                                  bank);
            } else {
                njEntryTexMemList(handle, &RECORD(previous_handle)->texinfo,
                                  globalIndex, TEXMEM_LOADED, bank);
            }
            RECORD(handle)->dummy = 0;
            njSetTextureParam(RECORD(handle));
            njSetPaletteBankLN(RECORD(handle), bank);
            /* @hack redundant: this store to palettized, which nothing reads,
             * keeps the image's MOV.L R2,@(24,R15) at 0x8c66903c in
             * 1ST_READ.BIN, R2 already holding the 1 the function returns;
             * without it, 1379 of 1497 words differ and the function is 12
             * bytes shorter */
            palettized = 1;
            return 1;
        }
    }
    if ((result = njCheckFileName(path, texname->filename)) > 0) {
        /* @hack spelling: four reads of the name go through
         * texlist->textures[n] rather than texname: the tests of attr for
         * NJD_TEXATTR_GLOBALINDEX and NJD_TEXATTR_PALGLOBALINDEX, and the two
         * reads of the global index from texaddr. the image indexes the list
         * anew for each, as at 0x8c66909c and 0x8c6690b8 in 1ST_READ.BIN;
         * through texname, any one of them makes the function 16 or 20 bytes
         * shorter, and with all four, 1345 of 1497 words differ */
        if (texlist->textures[n].attr & NJD_TEXATTR_GLOBALINDEX) {
            globalIndex = texlist->textures[n].texaddr;
            if ((index = njSearchTexList(_nj_texmemlist, _nj_texmemsize,
                                         globalIndex)) < 0) {
                listing = UNLISTED;
            } else {
                listing = LISTED;
            }
            if ((result = njTexFopen(path)) < 0) {
                njMemCopy(&error_info.texsurface, &_nj_tex_err_surface,
                          sizeof(NJS_TEXSURFACE));
                njEntryTexMemList(handle, &error_info, globalIndex, 0, -1);
                RECORD(handle)->dummy = -result;
                RECORD(handle)->count = 0;
                return -1;
            }
        } else if (texlist->textures[n].attr & NJD_TEXATTR_PALGLOBALINDEX) {
            globalIndex = texlist->textures[n].texaddr & NJD_TEXPALGLOBAL_MASK;
            bank = (texname->texaddr & NJD_TEXBANK_MASK) >> NJD_TEXBANK_SHIFT;
            if ((index = njSearchTexList(_nj_texmemlist, _nj_texmemsize,
                                         globalIndex)) < 0) {
                listing = UNLISTED;
            } else if ((bank_index =
                            njSearchTexList3(_nj_texmemlist, _nj_texmemsize,
                                             globalIndex, bank)) < 0) {
                listing = OTHER_BANK;
            } else {
                listing = LISTED;
            }
            palettized = 1;
            if ((result = njTexFopen(path)) < 0) {
                njMemCopy(&error_info.texsurface, &_nj_tex_err_surface,
                          sizeof(NJS_TEXSURFACE));
                njEntryTexMemList(handle, &error_info, globalIndex, 0, bank);
                RECORD(handle)->dummy = -result;
                RECORD(handle)->count = 0;
                return -1;
            }
        } else {
            if ((result = njTexFopen(path)) < 0) {
                njMemCopy(&error_info.texsurface, &_nj_tex_err_surface,
                          sizeof(NJS_TEXSURFACE));
                /* @bug uninitialized: the record takes globalIndex before
                 * njGetFileGlobalIndex sets it, whatever its frame word at
                 * sp+20 holds, at 0x8c669290 in 1ST_READ.BIN */
                njEntryTexMemList(handle, &error_info, globalIndex, 0, -1);
                RECORD(handle)->dummy = -result;
                RECORD(handle)->count = 0;
                return -1;
            }
            globalIndex = njGetFileGlobalIndex(texname);
            if ((type = njGetFileTexType()) == -1) {
                _nj_texerr_.texerr = NJD_TEXERR_OTHER;
                njTexFclose();
                return -1;
            }
            if ((palettized =
                     njCheckPaletteType(type & NJD_TEXFMT_TYPE_MASK)) == 1) {
                bank =
                    (texname->texaddr & NJD_TEXBANK_MASK) >> NJD_TEXBANK_SHIFT;
                if ((index = njSearchTexList(_nj_texmemlist, _nj_texmemsize,
                                             globalIndex)) < 0) {
                    listing = UNLISTED;
                } else if ((bank_index =
                                njSearchTexList3(_nj_texmemlist, _nj_texmemsize,
                                                 globalIndex, bank)) < 0) {
                    listing = OTHER_BANK;
                } else {
                    listing = LISTED;
                }
            } else {
                if (globalIndex == (Uint32)-1) {
                    njEntryTexMemList(handle, &error_info, globalIndex, 0, -1);
                    RECORD(handle)->dummy = NJD_TEXERR_GLOBALINDEX;
                    RECORD(handle)->count = 0;
                    njTexFclose();
                    _nj_texerr_.texerr = NJD_TEXERR_GLOBALINDEX;
                    return -1;
                }
                if ((index = njSearchTexList(_nj_texmemlist, _nj_texmemsize,
                                             globalIndex)) < 0) {
                    listing = UNLISTED;
                } else {
                    listing = LISTED;
                }
            }
        }
        /* @hack redundant: in the cases below, the else after each failed
         * load's return steers the registers of the success path in it, such as
         * R2 for the bank pushed at 0x8c6694fe and 0x8c669502 in 1ST_READ.BIN;
         * without one, 2 to 6 of 1497 words differ */
        switch (listing) {
        case OTHER_BANK:
            /* @bug memory: until njEntryTexMemList stores a record's
             * address at *handle, *handle still holds the name's texaddr from
             * before the load. a cache load before then writes the texture's
             * info 20 bytes past that value, and a failed load its error 66
             * bytes past it, at 0x8c6694be, 0x8c6694e2, 0x8c669670 and
             * 0x8c66973e in 1ST_READ.BIN */
            other = &_nj_texmemlist[index];
            flags = other->texaddr;
            if (flags & TEXMEM_LOADED) {
                switch (texname->attr & NJD_TEXATTR_READAREA_MASK) {
                case 0:
                    njEntryTexMemList(handle, &other->texinfo, globalIndex,
                                      TEXMEM_LOADED, bank);
                    RECORD(handle)->dummy = 0;
                    RECORD(handle)->count = 1;
                    njSetTextureParam(RECORD(handle));
                    njSetPaletteBankLN(RECORD(handle), bank);
                    break;
                case NJD_TEXATTR_CACHE:
                case NJD_TEXATTR_BOTH:
                    if (!(other->texaddr & TEXMEM_CACHED)) {
                        if ((result = njLoadCacheFromFile(
                                 texname, &RECORD(handle)->texinfo)) < 0) {
                            RECORD(handle)->dummy = -result;
                            _nj_texerr_.n = n;
                            _nj_texerr_.globalIndex = globalIndex;
                            njTexFclose();
                            return -1;
                        } else {
                            njEntryTexMemList(handle, &other->texinfo,
                                              globalIndex, TEXMEM_LOADED, bank);
                            RECORD(handle)->dummy = 0;
                            RECORD(handle)->count = 1;
                            njSetTextureParam(RECORD(handle));
                            njSetPaletteBankLN(RECORD(handle), bank);
                            for (i = 0; i < _nj_texmemsize; i++) {
                                if (_nj_texmemlist[i].globalIndex ==
                                    globalIndex) {
                                    _nj_texmemlist[i].texaddr |= TEXMEM_CACHED;
                                    _nj_texmemlist[i].texinfo.texaddr =
                                        RECORD(handle)->texinfo.texaddr;
                                }
                            }
                        }
                    } else {
                        njEntryTexMemList(handle, &other->texinfo, globalIndex,
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
                    if ((result = njLoadTexMemFromCache(texname,
                                                        &other->texinfo)) < 0) {
                        RECORD(handle)->dummy = -result;
                        _nj_texerr_.n = n;
                        _nj_texerr_.globalIndex = globalIndex;
                        njTexFclose();
                        return -1;
                    } else {
                        njEntryTexMemList(handle, &other->texinfo, globalIndex,
                                          TEXMEM_CACHED, bank);
                        RECORD(handle)->dummy = 0;
                        njSetFlagAddrAllTexture(
                            globalIndex, TEXMEM_LOADED,
                            RECORD(handle)->texinfo.texaddr);
                    }
                    break;
                case NJD_TEXATTR_CACHE:
                    njEntryTexMemList(handle, &other->texinfo, globalIndex,
                                      TEXMEM_CACHED, bank);
                    break;
                case NJD_TEXATTR_BOTH:
                    if ((result = njLoadTexMemFromCache(texname,
                                                        &other->texinfo)) < 0) {
                        RECORD(handle)->dummy = -result;
                        _nj_texerr_.n = n;
                        _nj_texerr_.globalIndex = globalIndex;
                        njTexFclose();
                        return -1;
                    } else {
                        njEntryTexMemList(handle, &other->texinfo, globalIndex,
                                          TEXMEM_CACHED, bank);
                        RECORD(handle)->dummy = 0;
                        njSetFlagAddrAllTexture(
                            globalIndex, TEXMEM_LOADED,
                            RECORD(handle)->texinfo.texaddr);
                    }
                    /* no break ends the switch's last case. with one, R2 and R3
                     * trade as the start of case LISTED computes the record's
                     * address (0x8c669790 to 0x8c6697a4 in 1ST_READ.BIN), and 9
                     * of 1497 words differ */
                }
            }
            break;
        case LISTED:
            *handle = (Uint32)&_nj_texmemlist[index];
            switch (texname->attr & NJD_TEXATTR_READAREA_MASK) {
            case 0:
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
                    }
                } else {
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
                /* no break ends the switch's last case, which keeps case
                 * LISTED's branches on its own njTexFclose call below
                 * (0x8c669a40 in 1ST_READ.BIN). with one, they branch to case
                 * UNLISTED's, and 123 of 1497 words differ */
            }
            njTexFclose();
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
                     * entered as cached, at 0x8c669c8e and 0x8c669c94 in
                     * 1ST_READ.BIN */
                    if ((result = njTexDownloadCache(NULL, load_info)) == 0) {
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
                /* @hack redundant: a break that ends this switch's last case,
                 * which keeps case LISTED's branches on its own njTexFclose
                 * call, at 0x8c669a40 in 1ST_READ.BIN; without it, they branch
                 * to the one below and 130 of 1497 words differ */
                break;
            }
            njTexFclose();
            /* @hack redundant: a break that ends the last case of the switch on
             * listing, which leaves R2 to hold njMemCopy's address in the else
             * below, at 0x8c669ce4 in 1ST_READ.BIN; without it, R2 and R3
             * trade and 5 of 1497 words differ */
            break;
        }
    } else {
        /* njCheckFileName returns 1 for every name, so this else never runs */
        njMemCopy(&error_info.texsurface, &_nj_tex_err_surface,
                  sizeof(NJS_TEXSURFACE));
        njEntryTexMemList(handle, &error_info, -1, 0, bank);
        RECORD(handle)->dummy = NJD_TEXERR_EXTND;
        RECORD(handle)->count = 0;
        _nj_texerr_.n = n;
        _nj_texerr_.globalIndex = globalIndex;
        return -1;
    }
    return 1;
}

/* njLoadTextureLN for a name under NJD_TEXATTR_TYPE_MEMORY */
Sint32 njLoadMemoryTexture(NJS_TEXLIST *texlist, Uint32 n)
{
    Uint32 *handle;
    Uint32 *previous_handle;
    NJS_TEXNAME *texname;
    NJS_TEXNAME *previous;
    NJS_TEXINFO *info;
    Uint8 *data;
    Uint32 offset;
    Uint32 name;
    Uint32 skip;
    Sint32 unindexed;
    struct pvrt_chunk pvrt;
    struct gbix_chunk gbix;
    Sint32 result;
    Sint32 index;
    Sint32 bank_index;
    Sint32 palettized;
    Uint32 globalIndex;
    Uint32 bank;
    Sint32 listing;
    Uint32 flags;
    Uint32 i;

    bank = -1;
    texname = &texlist->textures[n];
    handle = &texname->texaddr;
    if (texname->attr & NJD_TEXATTR_TEXCONTINUE) {
        if (n > 0) {
            previous = &texlist->textures[n - 1];
            previous_handle = &previous->texaddr;
            bank = (texname->texaddr & NJD_TEXBANK_MASK) >> NJD_TEXBANK_SHIFT;
            /* @bug uninitialized: nothing on this path sets globalIndex.
             * njSearchTexList searches for the stack pointer's value, and the
             * record takes that value less 4 as its global index, at
             * 0x8c669dae and 0x8c669dfa in 1ST_READ.BIN */
            /* NOLINTBEGIN(clang-diagnostic-uninitialized) */
            index =
                njSearchTexList(_nj_texmemlist, _nj_texmemsize, globalIndex);
            /* NOLINTEND(clang-diagnostic-uninitialized) */
            if (_nj_texmemlist[index].texaddr & TEXMEM_CACHED) {
                njEntryTexMemList(handle, &RECORD(previous_handle)->texinfo,
                                  globalIndex, TEXMEM_CACHED | TEXMEM_LOADED,
                                  bank);
            } else {
                njEntryTexMemList(handle, &RECORD(previous_handle)->texinfo,
                                  globalIndex, TEXMEM_LOADED, bank);
            }
            RECORD(handle)->dummy = 0;
            njSetTextureParam(RECORD(handle));
            njSetPaletteBankLN(RECORD(handle), bank);
            return 1;
        }
    }
    if (texname->attr & NJD_TEXATTR_GLOBALINDEX) {
        globalIndex = texname->texaddr;
        info = (NJS_TEXINFO *)texname->filename;
        data = info->texaddr;
        offset = 0;
        if ((index = njSearchTexList(_nj_texmemlist, _nj_texmemsize,
                                     globalIndex)) < 0) {
            /* find the texture chunk, past a global index chunk, and step
             * texaddr past the texture chunk's header, whose width, height and
             * type info takes. any other chunk ends the search and leaves
             * texaddr where it was, even past a global index chunk */
            do {
                njMemCopy(&name, data + offset, 4);
                switch (name) {
                case GBIX:
                    njMemCopy(&gbix, data + offset, sizeof(gbix));
                    offset += gbix.size + 8;
                    break;
                case PVRT:
                    njMemCopy(&pvrt, data + offset, sizeof(pvrt));
                    offset += sizeof(pvrt);
                    info->texsurface.nWidth = pvrt.width;
                    info->texsurface.nHeight = pvrt.height;
                    info->texsurface.Type = pvrt.type << 16;
                    break;
                default:
                    offset = 0;
                    name = PVRT;
                    break;
                }
            } while (name != PVRT);
            info->texaddr = (Uint8 *)info->texaddr + offset;
            listing = UNLISTED;
        } else {
            listing = LISTED;
        }
        palettized = 0;
    } else if (texlist->textures[n].attr & NJD_TEXATTR_PALGLOBALINDEX) {
        /* @hack spelling: this read and the test above it go through
         * texlist->textures[n] rather than texname, so the image indexes the
         * list anew for each, at 0x8c669ece and 0x8c669eda in 1ST_READ.BIN;
         * through texname, either makes the function 4 bytes shorter, and with
         * both, 895 of 932 words differ */
        globalIndex = texlist->textures[n].texaddr & NJD_TEXPALGLOBAL_MASK;
        bank = (texname->texaddr & NJD_TEXBANK_MASK) >> NJD_TEXBANK_SHIFT;
        /* NOLINTBEGIN(clang-diagnostic-sometimes-uninitialized) */
        /* NOLINTBEGIN(clang-diagnostic-uninitialized) */
        if ((index = njSearchTexList(_nj_texmemlist, _nj_texmemsize,
                                     globalIndex)) < 0) {
            /* @bug uninitialized: nothing on this path sets info, data or
             * offset. the chunk copies read from the stack pointer plus
             * offset, which starts as whatever the caller left in R9, at
             * 0x8c669f04, 0x8c669f06, 0x8c669f1e and 0x8c669f34. a texture
             * chunk then overwrites four frame words: its width goes to n's,
             * at 0x8c669f48, its height to skip's, at 0x8c669f4e, its type to
             * palettized's, at 0x8c669f56, and the stepped texaddr to index's,
             * at 0x8c669f6a. case UNLISTED's new record copies its info from
             * offset's last value as an address, at 0x8c66a41e, 0x8c66a47e
             * and 0x8c66a4ce, in 1ST_READ.BIN */
            do {
                njMemCopy(&name, data + offset, 4);
                switch (name) {
                case GBIX:
                    njMemCopy(&gbix, data + offset, sizeof(gbix));
                    offset += gbix.size + 8;
                    break;
                case PVRT:
                    njMemCopy(&pvrt, data + offset, sizeof(pvrt));
                    offset += sizeof(pvrt);
                    info->texsurface.nWidth = pvrt.width;
                    info->texsurface.nHeight = pvrt.height;
                    info->texsurface.Type = pvrt.type << 16;
                    break;
                default:
                    offset = 0;
                    name = PVRT;
                    break;
                }
            } while (name != PVRT);
            /* @hack type: texaddr stepped through a Uint8 ** view, which keeps
             * the sum in R3, ADD R9,R3 at 0x8c669f68 in 1ST_READ.BIN; as a
             * plain sum it lands in R9, offset's register, and 2 of 932 words
             * differ */
            *(Uint8 **)&info->texaddr += offset;
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
        /* NOLINTEND(clang-diagnostic-uninitialized) */
        /* NOLINTEND(clang-diagnostic-sometimes-uninitialized) */
        palettized = 1;
    } else {
        info = (NJS_TEXINFO *)texname->filename;
        data = info->texaddr;
        offset = 0;
        unindexed = 1;
        /* find the texture chunk and step texaddr past its header, skipping
         * other chunks by their size; a global index chunk on the way gives
         * the global index */
        do {
            njMemCopy(&name, data + offset, 4);
            switch (name) {
            case GBIX:
                njMemCopy(&gbix, data + offset, sizeof(gbix));
                offset += gbix.size + 8;
                globalIndex = gbix.globalIndex;
                unindexed = 0;
                break;
            case PVRT:
                njMemCopy(&pvrt, data + offset, sizeof(pvrt));
                offset += sizeof(pvrt);
                info->texsurface.nWidth = pvrt.width;
                info->texsurface.nHeight = pvrt.height;
                info->texsurface.Type = pvrt.type << 16;
                break;
            default:
                offset += 4;
                njMemCopy(&skip, data + offset, 4);
                offset += skip + 4;
                break;
            }
        } while (name != PVRT);
        /* @hack type: texaddr stepped through a Uint8 ** view; as a plain sum,
         * the test of unindexed and the loop below trade R1 and R3,
         * 0x8c66a068 to 0x8c66a080 in 1ST_READ.BIN, and 10 of 932 words
         * differ */
        *(Uint8 **)&info->texaddr += offset;
        if (unindexed) {
            /* with no global index chunk, the global index is the first one
             * no record holds, counting down from _nj_user_globalIndex, which
             * keeps it */
            while (njSearchTexList(_nj_texmemlist, _nj_texmemsize,
                                   _nj_user_globalIndex) != -1) {
                _nj_user_globalIndex--;
            }
            globalIndex = _nj_user_globalIndex;
        }
        if ((palettized =
                 njCheckPaletteType(pvrt.type & NJD_TEXFMT_TYPE_MASK)) == 1) {
            bank = (texname->texaddr & NJD_TEXBANK_MASK) >> NJD_TEXBANK_SHIFT;
            if ((index = njSearchTexList(_nj_texmemlist, _nj_texmemsize,
                                         globalIndex)) < 0) {
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
        } else {
            if (globalIndex == (Uint32)-1) {
                njEntryTexMemList(handle, &error_info, globalIndex, 0, -1);
                RECORD(handle)->dummy = NJD_TEXERR_GLOBALINDEX;
                RECORD(handle)->count = 0;
                _nj_texerr_.texerr = NJD_TEXERR_GLOBALINDEX;
                return -1;
            }
            if ((index = njSearchTexList(_nj_texmemlist, _nj_texmemsize,
                                         globalIndex)) < 0) {
                listing = UNLISTED;
            } else {
                listing = LISTED;
            }
        }
    }
    /* @hack redundant: in cases LISTED and UNLISTED, the else after each failed
     * load's return steers the registers of the success path in it, such as
     * R2 for the 0 stored to dummy at 0x8c66a346 and 0x8c66a34c in
     * 1ST_READ.BIN; without one, 3 to 11 of 932 words differ */
    switch (listing) {
    case OTHER_BANK:
        /* @bug memory: index takes its record's address, at
         * 0x8c66a14c, and the code below still indexes _nj_texmemlist by it:
         * the new records copy their info from there, and the cache loads read
         * and write there, at 0x8c66a184, 0x8c66a1f8, 0x8c66a28c, 0x8c66a2a2
         * and 0x8c66a2c8 in 1ST_READ.BIN */
    again:
        index = (Sint32)&_nj_texmemlist[index];
        /* @hack redundant: a goto back to the store to index, never taken,
         * which keeps index in the frame word at sp+0, as at 0x8c669e46 in
         * 1ST_READ.BIN; without it, index, offset and name move to sp+8, sp+0
         * and sp+4, the function runs 8 bytes short and 530 of 932 words
         * differ */
        if (0) {
            goto again;
        }
        flags = ((NJS_TEXMEMLIST *)index)->texaddr;
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
                if (!(((NJS_TEXMEMLIST *)index)->texaddr & TEXMEM_CACHED)) {
                    njEntryTexMemList(handle, &_nj_texmemlist[index].texinfo,
                                      globalIndex, TEXMEM_LOADED, bank);
                    RECORD(handle)->dummy = 0;
                    RECORD(handle)->count = 1;
                    njSetTextureParam(RECORD(handle));
                    njSetPaletteBankLN(RECORD(handle), bank);
                    for (i = 0; i < _nj_texmemsize; i++) {
                        if (_nj_texmemlist[i].globalIndex == globalIndex) {
                            _nj_texmemlist[i].texaddr |= TEXMEM_CACHED;
                            _nj_texmemlist[i].texinfo.texaddr =
                                RECORD(handle)->texinfo.texaddr;
                        }
                    }
                } else {
                    njEntryTexMemList(handle, &_nj_texmemlist[index].texinfo,
                                      globalIndex,
                                      TEXMEM_CACHED | TEXMEM_LOADED, bank);
                    RECORD(handle)->dummy = 0;
                    RECORD(handle)->count = 1;
                    njSetTextureParam(RECORD(handle));
                    njSetPaletteBankLN(RECORD(handle), bank);
                }
            }
        } else {
            /* @bug memory: a failed cache load writes its error through
             * *handle before njEntryTexMemList stores a record's address
             * there, so the error lands 66 bytes past the name's texaddr from
             * before the load, at 0x8c66a3ba in 1ST_READ.BIN */
            switch (texname->attr & NJD_TEXATTR_READAREA_MASK) {
            case 0:
                if ((result = njLoadTexMemFromCache(
                         texname, &_nj_texmemlist[index].texinfo)) < 0) {
                    RECORD(handle)->dummy = -result;
                    _nj_texerr_.n = n;
                    _nj_texerr_.globalIndex = globalIndex;
                    return -1;
                } else {
                    njEntryTexMemList(handle, &_nj_texmemlist[index].texinfo,
                                      globalIndex, TEXMEM_CACHED, bank);
                    RECORD(handle)->dummy = 0;
                    RECORD(handle)->count = 1;
                    njSetFlagAddrAllTexture(globalIndex, TEXMEM_LOADED,
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
                    njEntryTexMemList(handle, &_nj_texmemlist[index].texinfo,
                                      globalIndex, TEXMEM_CACHED, bank);
                    RECORD(handle)->dummy = 0;
                    RECORD(handle)->count = 1;
                    njSetFlagAddrAllTexture(globalIndex, TEXMEM_LOADED,
                                            RECORD(handle)->texinfo.texaddr);
                }
                /* no break ends the switch's last case. with one, R1 and R2
                 * trade as the start of case LISTED computes the record's
                 * address (0x8c66a2f6 to 0x8c66a306 in 1ST_READ.BIN), and 9 of
                 * 932 words differ */
            }
        }
        break;
    case LISTED:
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
            info = (NJS_TEXINFO *)texname->filename;
            RECORD(handle)->texinfo.texaddr = info->texaddr;
            RECORD(handle)->texaddr |= TEXMEM_CACHED;
            break;
        case NJD_TEXATTR_BOTH:
            if (RECORD(handle)->texaddr & TEXMEM_LOADED) {
                RECORD(handle)->count++;
                if (!(RECORD(handle)->texaddr & TEXMEM_CACHED)) {
                    info = (NJS_TEXINFO *)texname->filename;
                    RECORD(handle)->texinfo.texaddr = info->texaddr;
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
            /* @hack redundant: a break that ends this switch's last case, which
             * leaves R2 to hold njTexMemList's address in case UNLISTED's
             * first case, at 0x8c66a412 in 1ST_READ.BIN; without it, R2 and R3
             * trade and 11 of 932 words differ */
            break;
        }
        break;
    case UNLISTED:
        switch (texname->attr & NJD_TEXATTR_READAREA_MASK) {
        case 0:
            load_info = njTexMemList(globalIndex);
            njMemCopy(load_info, info, sizeof(NJS_TEXINFO));
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
                 * equal, which the image computes as R10 plus 8 at 0x8c66a44a
                 * and 0x8c66a456 in 1ST_READ.BIN rather than taking handle's
                 * R14; through handle, 37 of 932 words differ */
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
            njMemCopy(load_info, info, sizeof(NJS_TEXINFO));
            njEntryTexMemList(handle, load_info, globalIndex, TEXMEM_CACHED,
                              bank);
            break;
        case NJD_TEXATTR_BOTH:
            load_info = njTexMemList(globalIndex);
            njMemCopy(load_info, info, sizeof(NJS_TEXINFO));
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
                njEntryTexMemList(handle, load_info, globalIndex,
                                  TEXMEM_CACHED | TEXMEM_LOADED, bank);
                njSetTextureParam(RECORD(handle));
                RECORD(handle)->dummy = 0;
                if (palettized) {
                    njSetPaletteBankLN(RECORD(handle), bank);
                }
            }
        }
        /* @hack redundant: a break that ends the last case of the switch on
         * listing, which lets case OTHER_BANK's NJD_TEXATTR_CACHE case, for a
         * texture not loaded, branch to the njEntryTexMemList call in case
         * UNLISTED's, 0x8c66a29e to 0x8c66a48e in 1ST_READ.BIN; without it,
         * that case makes the call itself, the function is 8 bytes longer,
         * and 343 of 932 words differ */
        break;
    }
    return 1;
}
