/* Kamui, the graphics library under Ninja, as far as the sources use it.
 * LOADER.BIN links SDK 1.20J's Kamui, and 1ST_READ.BIN links 1.30J's. both
 * share this header */
#ifndef KAMUI_H
#define KAMUI_H

typedef unsigned int KMSTATUS;
typedef unsigned char KMBYTE;
typedef unsigned short KMWORD;
typedef int KMINT32;
typedef int KMBOOLEAN;
typedef unsigned int KMUINT32;
typedef KMUINT32 *PKMUINT32;
typedef KMINT32 *PKMINT32;
typedef unsigned long KMDWORD;
typedef KMDWORD *PKMDWORD;
typedef float KMFLOAT;
typedef KMFLOAT *PKMFLOAT;
typedef unsigned int KMTEXTURETYPE;
typedef void *PVOID;
typedef void *PKMVOID;

#define KMSTATUS_SUCCESS 0
#define KMSTATUS_INVALID_DISPLAY_MODE 1
#define KMSTATUS_INVALID_VIDEO_MODE 2
#define KMSTATUS_NOT_ENOUGH_MEMORY 3
#define KMSTATUS_INVALID_TEXTURE_TYPE 4
#define KMSTATUS_INVALID_ADDRESS 7
#define KMSTATUS_HARDWARE_NOT_PRESENTED 8
#define KMSTATUS_INVALID_VERTEX_TYPE 10
#define KMSTATUS_INVALID_LIST_TYPE 11
#define KMSTATUS_INVALID_BLOCKNUMBER 12
#define KMSTATUS_LAST_LIST_UNDER_DMA 13
#define KMSTATUS_INVALID_SURFACE 14
#define KMSTATUS_INVALID_SURFACE_TYPE 15
#define KMSTATUS_INVALID_SURFACE_SIZE 16
#define KMSTATUS_NOT_FINISH_DMA 17
#define KMSTATUS_NOT_IMPLEMENTED 120
#define KMSTATUS_INVALID_PARAMETER 0xdddd

/* a color packed in one word, blue in the low byte */
typedef union _tagKMPACKEDARGB {
    KMDWORD dwPacked;
    struct {
        KMBYTE bBlue;
        KMBYTE bGreen;
        KMBYTE bRed;
        KMBYTE bAlpha;
    } byte;
} KMPACKEDARGB;

typedef void (*PKMCALLBACKFUNC)(void *pArgs);

/* a surface in video memory, Ninja's NJS_TEXSURFACE under Kamui's name. its
 * unions take the names the SDK gives them under _STRICT_UNION_ */
typedef struct tagKMSURFACEDESC {
    KMDWORD SurfaceType;
    KMDWORD BitDepth;
    KMDWORD PixelFormat;
    union {
        KMDWORD USize;
        KMDWORD Width;
    } u0;
    union {
        KMDWORD VSize;
        KMDWORD Height;
    } u1;
    union {
        KMDWORD dwTextureSize;
        KMDWORD dwFrameBufferSize;
    } uSize;
    KMDWORD fSurfaceFlags;
    /* the surface's offset in video memory, not a CPU address. video memory has
     * two views. in the 64-bit view, its two halves alternate a long at a time;
     * in the 32-bit view, each half is contiguous. the texture allocators store
     * the offset in the 64-bit view, and kmiFBalloc and kmiFBallocEx store it
     * in the 32-bit view */
    PKMDWORD pSurface;
    PKMDWORD pVirtual;
    PKMDWORD pPhysical;
} KMSURFACEDESC, *PKMSURFACEDESC;
/* fSurfaceFlags bits */
#define KM_SURFACEFLAGS_MIPMAPED 0x001
#define KM_SURFACEFLAGS_TWIDDLED 0x004
#define KM_SURFACEFLAGS_VQ 0x008
#define KM_SURFACEFLAGS_STRIDE 0x010
#define KM_SURFACEFLAGS_PALETTIZED 0x020
/* BitDepth values: a pixel's bytes less one. a palettized texture takes that of
 * its palette's colors */
#define KMI_BITDEPTH_16 1
#define KMI_BITDEPTH_24 2
#define KMI_BITDEPTH_32 3
/* SurfaceType values. a small VQ texture shares a page of video memory with
 * others of its shape */
#define KM_SURFACETYPE_FRAMEBUFFER 1
#define KM_SURFACETYPE_TEXTURE 2
#define KM_SURFACETYPE_SMALLVQ_TEXTURE 3

/* a vertex context, which sets how the polygons after it draw. its owner sets
 * the fields, and marks each field it set in RenderState. Kamui computes the
 * four words of a polygon header from the fields */
typedef struct tagKMVERTEXCONTEXT {
    KMDWORD RenderState;
    KMINT32 ParamType;
    KMINT32 ListType;
    KMINT32 ColorType;
    KMINT32 UVFormat;
    KMINT32 DepthMode;
    KMINT32 CullingMode;
    KMINT32 ScreenCoordination;
    KMINT32 ShadingMode;
    KMINT32 SelectModifier;
    KMBOOLEAN bZWriteDisable;
    KMINT32 SRCBlendingMode;
    KMINT32 DSTBlendingMode;
    KMBOOLEAN bSRCSel;
    KMBOOLEAN bDSTSel;
    KMINT32 FogMode;
    KMBOOLEAN bUseSpecular;
    KMBOOLEAN bUseAlpha;
    KMBOOLEAN bIgnoreTextureAlpha;
    KMINT32 ClampUV;
    KMINT32 FlipUV;
    KMINT32 FilterMode;
    KMBOOLEAN bSuperSample;
    KMDWORD MipMapAdjust;
    KMINT32 TextureShadingMode;
    KMBOOLEAN bColorClamp;
    KMDWORD PaletteBank;
    PKMSURFACEDESC pTextureSurfaceDesc;
    KMFLOAT fFaceColorAlpha;
    KMFLOAT fFaceColorRed;
    KMFLOAT fFaceColorGreen;
    KMFLOAT fFaceColorBlue;
    KMFLOAT fOffsetColorAlpha;
    KMFLOAT fOffsetColorRed;
    KMFLOAT fOffsetColorGreen;
    KMFLOAT fOffsetColorBlue;
    KMDWORD GLOBALPARAMBUFFER;
    KMDWORD ISPPARAMBUFFER;
    KMDWORD TSPPARAMBUFFER;
    KMDWORD TexturePARAMBUFFER;
    KMDWORD ModifierInstruction;
    KMFLOAT fBoundingBoxXmin;
    KMFLOAT fBoundingBoxYmin;
    KMFLOAT fBoundingBoxXmax;
    KMFLOAT fBoundingBoxYmax;
    KMBOOLEAN bDCalcExact;
    KMINT32 StripLength;
    KMINT32 UserClipMode;
} KMVERTEXCONTEXT, *PKMVERTEXCONTEXT, **PPKMVERTEXCONTEXT;

/* RenderState bits for some of the fields. from KM_DEPTHMODE to
 * KM_PALETTEBANK, the bits run in their fields' order. that order alone names
 * KM_SCREENCOORDINATION and KM_PALETTEBANK, which kmProcessVertexRenderState
 * doesn't read */
#define KM_DEPTHMODE 0x00000001
#define KM_CULLINGMODE 0x00000002
#define KM_SCREENCOORDINATION 0x00000004
#define KM_SHADINGMODE 0x00000008
#define KM_MODIFIER 0x00000010
#define KM_ZWRITEDISABLE 0x00000020
#define KM_SRCBLENDINGMODE 0x00000040
#define KM_DSTBLENDINGMODE 0x00000080
#define KM_FOGMODE 0x00000100
#define KM_USESPECULAR 0x00000200
#define KM_USEALPHA 0x00000400
#define KM_IGNORETEXTUREALPHA 0x00000800
#define KM_CLAMPUV 0x00001000
#define KM_FLIPUV 0x00002000
#define KM_FILTERMODE 0x00004000
#define KM_SUPERSAMPLE 0x00008000
#define KM_MIPMAPDADJUST 0x00010000
#define KM_TEXTURESHADINGMODE 0x00020000
#define KM_COLORCLAMP 0x00040000
#define KM_PALETTEBANK 0x00080000
#define KM_PARAMTYPE 0x00100000
#define KM_LISTTYPE 0x00200000
#define KM_COLORTYPE 0x00400000
#define KM_UVFORMAT 0x00800000
#define KM_SRCSELECT 0x01000000
#define KM_DSTSELECT 0x02000000
#define KM_DCALCEXACT 0x04000000
#define KM_STRIPLENGTH 0x08000000
#define KM_USERCLIPMODE 0x10000000
/* ParamType values */
#define KM_POLYGON 0
#define KM_MODIFIERVOLUME 1
#define KM_SPRITE 2
/* ListType values */
#define KM_OPAQUE_POLYGON 0
#define KM_OPAQUE_MODIFIER 1
#define KM_TRANS_POLYGON 2
#define KM_TRANS_MODIFIER 3
#define KM_PUNCHTHROUGH_POLYGON 4
/* ColorType values */
#define KM_PACKEDCOLOR 0
#define KM_FLOATINGCOLOR 1
#define KM_INTENSITY_PREV_FACE_COL 3
/* UVFormat values */
#define KM_32BITUV 0 /* float UVs */
#define KM_16BITUV 1
/* DepthMode values */
#define KM_GREATER 4
#define KM_GREATEREQUAL 6
#define KM_ALWAYS 7
/* CullingMode values */
#define KM_NOCULLING 0
#define KM_CULLSMALL 1
#define KM_CULLCCW 2
#define KM_CULLCW 3
/* ScreenCoordination values */
#define KM_SCREEN 0
/* ShadingMode values */
#define KM_NOTEXTUREFLAT 0
#define KM_NOTEXTUREGOURAUD 1
#define KM_TEXTUREFLAT 2
#define KM_TEXTUREGOURAUD 3
/* SelectModifier values */
#define KM_NOMODIFIER 0
/* SRCBlendingMode and DSTBlendingMode values. KM_BOTHSRCALPHA and
 * KM_BOTHINVSRCALPHA set both factors, whichever field holds them.
 * KM_BOTHSRCALPHA makes the source factor the source alpha and the destination
 * factor its inverse. KM_BOTHINVSRCALPHA makes the source factor the inverse
 * and the destination factor the source alpha */
#define KM_BOTHINVSRCALPHA 0
#define KM_BOTHSRCALPHA 1
#define KM_DESTALPHA 2
#define KM_DESTCOLOR 3
#define KM_INVDESTALPHA 4
#define KM_INVDESTCOLOR 5
#define KM_INVSRCALPHA 6
#define KM_INVSRCCOLOR 7
#define KM_SRCALPHA 8
#define KM_SRCCOLOR 9
#define KM_ONE 10
#define KM_ZERO 11
/* FogMode values */
#define KM_FOGTABLE 0
#define KM_NOFOG 2
#define KM_FOGTABLE_2 3
/* ClampUV values */
#define KM_NOCLAMP 0
#define KM_CLAMP_V 1
#define KM_CLAMP_U 2
#define KM_CLAMP_UV 3
/* FlipUV values */
#define KM_NOFLIP 0
#define KM_FLIP_V 1
#define KM_FLIP_U 2
#define KM_FLIP_UV 3
/* FilterMode values. KM_TRILINEAR_A and KM_TRILINEAR_B are a trilinear
 * filter's two passes */
#define KM_POINT_SAMPLE 0
#define KM_TRILINEAR_A 1
#define KM_BILINEAR 2
#define KM_TRILINEAR_B 3
/* MipMapAdjust values */
#define KM_MIPMAP_D_ADJUST_1_00 0x00000004
/* TextureShadingMode values */
#define KM_DECAL 0
#define KM_MODULATE 1
#define KM_DECAL_ALPHA 2
#define KM_MODULATE_ALPHA 3
/* ModifierInstruction values, as the CLX chips take them */
#define KM_MODIFIER_NORMAL_POLY 0
#define KM_MODIFIER_INCLUDE_FIRST_POLY KM_MODIFIER_NORMAL_POLY
#define KM_MODIFIER_EXCLUDE_FIRST_POLY KM_MODIFIER_NORMAL_POLY
#define KM_MODIFIER_INCLUDE_LAST_POLY (1 << 29)
#define KM_MODIFIER_EXCLUDE_LAST_POLY (2 << 29)
/* UserClipMode values */
#define KM_USERCLIP_RESERVE 1

/* palette color formats, for kmSetPaletteMode */
#define KM_PALETTE_16BPP_ARGB1555 0
#define KM_PALETTE_16BPP_RGB565 1
#define KM_PALETTE_16BPP_ARGB4444 2
#define KM_PALETTE_32BPP_ARGB8888 3

/* a vertex buffer's current list */
typedef struct _tagKMCURRENTLISTSTATE {
    KMDWORD ListType;
    KMDWORD GlobalParamSize; /* bytes in each polygon's global parameter */
    KMDWORD Reserved0; /* the VertexType of the last kmSetVertex call */
    KMDWORD VertexBank;
} KMCURRENTLISTSTATE, *PKMCURRENTLISTSTATE;

/* a vertex buffer's descriptor, 32 bytes */
typedef struct _tagKMVERTEXBUFFDESC {
    PKMDWORD *pCurrentPtr; /* each list's write pointer, by ListType */
    PKMDWORD pGlobalParam; /* the next strip's global parameter */
    PKMCURRENTLISTSTATE pCurrentListState;
    PKMDWORD **ppBuffer; /* each bank's list bases, by ListType */
    KMDWORD fActiveList; /* a bit for each list in use, by ListType */
    KMDWORD Reserved1;
    KMDWORD Reserved2;
    KMDWORD Reserved3;
} KMVERTEXBUFFDESC, *PKMVERTEXBUFFDESC;

/* end the lists written since the last kmRender, and record them in the write
 * slot. Kamui keeps four frame slots, and each slot holds every list's start
 * and size. the write slot, kmiDeviceExtension.write_slot, is the slot whose
 * lists are being written. in the write slot, kmRender records each list's
 * start, and its size in bytes. it also records each list's largest size in
 * kmiIntStatisticData.list_max. the result is KMSTATUS_SUCCESS.
 *
 * on a chip with KMI_CAP_DUMMY_OPAQUE, kmRender first adds kmiDummyOpaque, a
 * dummy opaque polygon, to the opaque list. it does so only with presort on,
 * and when kmiDeviceExtension.opaque_opb and translucent_opb both give their
 * lists object pointer blocks.
 *
 * how kmRender ends the lists depends on the latency model:
 * - under KM_LATENCYMODEL_3V, each list in use takes its end-of-list word, and
 *   each other list a size of 0;
 * - under KM_LATENCYMODEL_2V with a NOBUF vertex buffer model, every list but
 *   kmiDeviceExtension.vertex_buffer_list ends the same way. that list goes to
 *   the tile accelerator with no buffer, and kmRender neither ends it nor
 *   records its size. while it is empty, kmRender sends it a header through the
 *   store queues;
 * - under KM_LATENCYMODEL_2V with another model, kmRender calls
 *   kmQueryFinishFlushVertexBuffer, then ends every list.
 *
 * when a list in use has passed its list_limit, kmRender cuts each list in use
 * back to a header, but for a NOBUF model's list. the other lists take a size
 * of 0, and kmRender calls kmiFatalErrorEntry(KMI_FATAL_LIST_LIMIT). last,
 * kmRender calls kmiFlipVertexBuffer with bank 1 on its first call, bank 0 on
 * its second, and so on */
KMSTATUS kmRender(void);
/* hand the write slot's lists on, wait for the flip, and only then make the
 * next of the four slots the write slot. the result is KMSTATUS_SUCCESS.
 *
 * kmFlipFrameBuffer hands no lists on when kmRenderDirect has been called since
 * the last kmRender. under KM_LATENCYMODEL_3V, it marks the slot ready, and the
 * interrupt handler gives its lists to the tile accelerator at a vertical
 * blank. under KM_LATENCYMODEL_2V, the tile accelerator takes them now, by DMA,
 * vertex_buffer_list's first. under a NOBUF model, that list has gone already,
 * and only its end-of-list word goes, through the store queues.
 *
 * kmFlipFrameBuffer then waits while kmiDeviceExtension.flip_pending is set,
 * running the wait-vsync callback. once vsync_count passes vertex_timeout +
 * render_timeout + 1, it clears vsync_count and render_busy and waits again. if
 * vsync_count then passes 20, it calls
 * kmiFatalErrorEntry(KMI_FATAL_FLIP_TIMEOUT) and stops waiting.
 *
 * after the wait, the next slot becomes the write slot. under
 * KM_LATENCYMODEL_2V, or after kmRenderDirect, kmFlipFrameBuffer then resets
 * the tile accelerator and sets it up for the new write slot's bank. after
 * kmRenderDirect, it also gives the tile accelerator the new write slot, which
 * ta_slot then names. it sets slots_held, so that the interrupt handler holds
 * every slot */
KMSTATUS kmFlipFrameBuffer(void);
/* each call below registers callback for its event, to be called with argument.
 * argument must stay valid while the callback is registered. a null callback
 * restores the event's default handler, which does nothing, but for the fatal
 * error event's, which sets the border color by the error's code and never
 * returns. neither kmSetVSyncCallback nor kmSetHSyncCallback enables or
 * disables an interrupt: both blank interrupts stay enabled from kmInitDevice
 * on, whatever callback is set. kmfSetFatalErrorCallback takes no argument:
 * kmiFatalErrorEntry calls the fatal error callback with the error's code.
 * each call returns KMSTATUS_SUCCESS */
KMSTATUS kmSetEORCallback(PKMCALLBACKFUNC callback, void *argument);
KMSTATUS kmSetVSyncCallback(PKMCALLBACKFUNC callback, void *argument);
KMSTATUS kmSetHSyncCallback(PKMCALLBACKFUNC callback, void *argument);
KMSTATUS kmSetTexOverflowCallback(PKMCALLBACKFUNC callback, void *argument);
KMSTATUS kmSetStripOverRunCallback(PKMCALLBACKFUNC callback, void *argument);
KMSTATUS kmSetEndOfVertexCallback(PKMCALLBACKFUNC callback, void *argument);
KMSTATUS kmSetEndOfYUVCallback(PKMCALLBACKFUNC callback, void *argument);
KMSTATUS kmSetWaitVsyncCallback(PKMCALLBACKFUNC callback, void *argument);
KMSTATUS kmfSetFatalErrorCallback(PKMCALLBACKFUNC callback);
/* kmSetHSyncLine and kmGetCurrentScanline count lines from the top of the
 * picture. kmSetHSyncLine sets the line that raises the horizontal blank
 * interrupt. in a 480-line mode other than VGA, it first halves line. it then
 * adds kmiDeviceExtension.origin_y1 + offset_y, the line where the first field
 * starts. a field's line count is the high half of sync_load, halved when the
 * screen is sent as two fields. a sum below 0 has that count added to it once,
 * and the sum is then clamped from 0 to the count. kmSetHSyncLine returns
 * KMSTATUS_SUCCESS */
KMSTATUS kmSetHSyncLine(KMINT32 line);
KMSTATUS kmGetCurrentScanline(KMINT32 *line);
KMSTATUS kmSetBorderColor(KMPACKEDARGB color);
/* update the context's four header words from the fields its RenderState marks.
 * whatever RenderState marks, a textured context with a surface takes the
 * surface's size, format and address. for a palettized surface, it also takes
 * the palette bank from PaletteBank. every call writes the control word's group
 * bit, strip length, shadow bit and volumes bit. it also clears the control
 * word's bits 8 to 15, 20 to 22, 27 and 28, and the ISP word's bits 0 to 19 and
 * 21. the result is KMSTATUS_SUCCESS */
KMSTATUS kmProcessVertexRenderState(PKMVERTEXCONTEXT pVertexContext);
/* make the context's four header words, colors and ModifierInstruction the
 * current header, and build the global parameter from it. a header in the
 * opaque list blends with a source factor of one and a destination factor of
 * zero, whatever the context's TSP word holds. the result is
 * KMSTATUS_SUCCESS */
KMSTATUS kmSetVertexRenderState(PKMVERTEXCONTEXT pVertexContext);
/* make the context's TSP and texture words and face color the header's second
 * volume, for the polygons a modifier volume touches. the call then rebuilds
 * the global parameter from the whole current header, as
 * kmSetVertexRenderState does. the result is KMSTATUS_SUCCESS */
KMSTATUS kmSetModifierRenderState(PKMVERTEXCONTEXT pVertexContext);
/* set the strip length of a context that does not set its own */
KMSTATUS kmSetStripLength(KMINT32 StripLength);
/* turn cheap shadows on at an intensity from 0 to 255, or off for a negative
 * one */
KMSTATUS kmSetCheapShadowMode(KMINT32 nIntensity);
/* each changes one mode of the current polygon header and of the global
 * parameter built from it. kmChangeContextColorType changes only the two
 * control words, so the global parameter keeps the old color type's layout,
 * and kmiCurrentListState.GlobalParamSize its size. kmChangeContextPaletteBank
 * changes nothing unless the header's texture is palettized */
KMSTATUS kmChangeContextColorType(KMINT32 Color);
KMSTATUS kmChangeContextDepthMode(KMINT32 DepthMode);
KMSTATUS kmChangeContextCullingMode(KMINT32 CullingMode);
KMSTATUS kmChangeContextZWriteDisable(KMBOOLEAN fZWriteDisable);
KMSTATUS kmChangeContextSRCBlendMode(KMINT32 SRCBlend);
KMSTATUS kmChangeContextDSTBlendMode(KMINT32 DSTBlend);
KMSTATUS kmChangeContextFogMode(KMINT32 FogMode);
KMSTATUS kmChangeContextFlipUV(KMINT32 FlipMode);
KMSTATUS kmChangeContextClampUV(KMINT32 ClampMode);
KMSTATUS kmChangeContextFilterMode(KMINT32 FilterMode);
KMSTATUS kmChangeContextSuperSample(KMBOOLEAN fEnable);
KMSTATUS kmChangeContextTextureShadingMode(KMINT32 TexShadingmode);
KMSTATUS kmChangeContextColorClamp(KMBOOLEAN fColorClamp);
KMSTATUS kmChangeContextPaletteBank(KMDWORD Bank);
KMSTATUS kmChangeContextStripLength(KMINT32 StripLength);
KMSTATUS kmChangeContextUserClipMode(KMINT32 UserClipMode);
/* set the palettes' color format to Palettemode, a KM_PALETTE_ value. any other
 * value leaves the format as it was. the result is KMSTATUS_SUCCESS */
KMSTATUS kmSetPaletteMode(KMINT32 Palettemode);
/* describe pSurfaceDesc as an nWidth by nHeight texture of nTextureType, and
 * allocate its memory. a small VQ type takes a slot in a small VQ page. each
 * side must be KM_MAPSIZE_8 to KM_MAPSIZE_1024. the sides must be equal but in
 * a rectangle, stride or twiddled rectangle texture, and in a palettized one
 * without mipmaps. a type that isn't palettized needs a pixel format, the
 * type's low byte, from KM_TEXTURE_ARGB1555 to KM_TEXTURE_BUMP.
 *
 * a type or size that breaks these rules returns KMSTATUS_INVALID_TEXTURE_TYPE,
 * with the description filled in and nothing allocated. when memory runs out,
 * kmCreateTextureSurface calls the texture overflow callback and returns
 * KMSTATUS_NOT_ENOUGH_MEMORY. else the result is KMSTATUS_SUCCESS */
KMSTATUS kmCreateTextureSurface(PKMSURFACEDESC pSurfaceDesc, KMINT32 nWidth,
                                KMINT32 nHeight, KMTEXTURETYPE nTextureType);
/* copy the texture at pTexture into pSurfaceDesc's memory through
 * kmiCopyToFrameBuffer. kmQueryFinishLastTextureDMA reports on the copy's last
 * transfer by DMA. the result is KMSTATUS_INVALID_TEXTURE_TYPE, with nothing
 * copied, when fAutoMipMap or fDither is TRUE, and else KMSTATUS_SUCCESS */
KMSTATUS kmLoadTexture(PKMSURFACEDESC pSurfaceDesc, PKMDWORD pTexture,
                       KMBOOLEAN fAutoMipMap, KMBOOLEAN fDither);
/* set vertex_base, the address kmCreateVertexBuffer lays the lists out from, to
 * dwBaseAddress. dwVertexBufferSize goes to vertex_size, which nothing reads.
 * kmInitDevice sets both fields again from the profile, and
 * kmSetSystemConfiguration sets vertex_base to its pVertexBuffer */
KMSTATUS kmfSetVertexBufferBaseAddress(KMDWORD dwBaseAddress,
                                       KMDWORD dwVertexBufferSize);
typedef struct _tagKMISYSTEMMETRICS {
    struct _tagNativeParams {
        KMDWORD NativeITPBase;
        KMDWORD NativeOBJBase;
        KMDWORD NativeRegBase;
        KMDWORD NativeITPLimit;
        KMDWORD NativeOBJLimit;
        KMDWORD NativeLastITPCurrent;
    } NativeParams[2];
    struct _tagFBParams {
        KMDWORD FBStartAddr[2];
        KMDWORD FBLineStride[2];
        KMDWORD XClip;
        KMDWORD YClip;
    } FBParams;
    KMDWORD RenderTimeOutCount;
    KMDWORD VertexTimeOutCount;
    KMDWORD OBJOverflowCount;
    KMDWORD ITPOverflowCount;
    KMDWORD DMADataAddress[2][5];
    KMDWORD InternalVertexBufSize[5];
    KMDWORD MaxVertexBufSize[5];
    KMDWORD LastTATime;
    KMDWORD LastRenderTime;
    KMDWORD Reserved2;
    KMDWORD Reserved3;
    KMDWORD Reserved4;
    KMDWORD Reserved5;
    KMDWORD Reserved6;
    KMDWORD Reserved7;
} KMISYSTEMMETRICS, *PKMISYSTEMMETRICS;
/* fill in *pSystemMetrics, all but its Reserved words. NativeParams and
 * FBLineStride run in reverse order. NativeParams[0] describes the tile
 * accelerator's bank 1, and NativeParams[1] its bank 0. FBLineStride[0] holds
 * field_offset[1], the second field's read offset, and FBLineStride[1] the
 * first's; neither is a stride. FBStartAddr[0] is fb_start[write_slot & 1]
 * under KM_LATENCYMODEL_3V, and the other frame buffer's start under
 * KM_LATENCYMODEL_2V. the result is KMSTATUS_SUCCESS */
KMSTATUS kmfGetInternalSystemMetrics(PKMISYSTEMMETRICS pSystemMetrics);
/* store nVertexTimeOut in vertex_timeout and nRenderTimeOut in render_timeout.
 * the interrupt handler stops a transfer to the tile accelerator once more than
 * vertex_timeout of its checks have found the transfer unfinished. it stops a
 * render once more than render_timeout checks have found the render unfinished.
 * a render timeout of 0 thus stops a render at its first check. Kamui's own
 * waits count vertical blanks against the same fields. Kamui resets the
 * renderer before each render when the render timeout is 0, or when
 * BUS_REVISION is 4, 5, 6 or 8. the result is KMSTATUS_SUCCESS */
KMSTATUS kmfSetTimeOutCount(KMDWORD nVertexTimeOut, KMDWORD nRenderTimeOut);
/* kmfInitializeYUVConverter's modes: the converter fills one texture, or a run
 * of them */
#define KMF_YUVCONVERT_ONE_TEXTURE 0
#define KMF_YUVCONVERT_NUMOF_TEXTURE 1
/* set the YUV converter to fill pSurface from macroblocks, xMBSize across and
 * yMBSize down, 1 to 64 each. the macroblocks hold 4:2:2 data for a TextureType
 * of KM_TEXTURE_YUV422, and 4:2:0 data for any other */
KMSTATUS kmfInitializeYUVConverter(PKMSURFACEDESC pSurface, KMDWORD TextureType,
                                   KMDWORD ConvertMode, KMDWORD xMBSize,
                                   KMDWORD yMBSize);
/* turn on the common modifier buffer, BufferSize bytes from pCommonBuffer.
 * Kamui sends the buffer to the tile accelerator ahead of each modifier list.
 * it first writes the list's type into bits 24 to 27 of the buffer's first
 * word. the buffer must stay valid until it is turned off or replaced. a
 * BufferSize of 0 turns it off. a buffer or size that isn't a multiple of 32
 * turns it off too, and returns KMSTATUS_INVALID_ADDRESS. else the result is
 * KMSTATUS_SUCCESS */
KMSTATUS kmfSetCommonModifier(PKMDWORD pCommonBuffer, KMDWORD BufferSize);

/* render the lists as they stand into pTextureSurface, then start the lists
 * over. the surface must be ARGB1555, RGB565 or ARGB4444, with a BitDepth of
 * KMI_BITDEPTH_16, and not twiddled. a twiddled surface, or one of another
 * BitDepth, returns KMSTATUS_INVALID_TEXTURE_TYPE. each wait counts vertical
 * blanks against a timeout. a wait that runs out returns one of four statuses.
 * - (KMSTATUS)-1 under KM_LATENCYMODEL_3V, when the tile accelerator is still
 *   busy past vertex_timeout before the render;
 * - (KMSTATUS)-2 when the renderer is still busy past render_timeout before the
 *   render;
 * - (KMSTATUS)-3 under KM_LATENCYMODEL_2V, when the render itself runs past
 *   render_timeout;
 * - KMSTATUS_SUCCESS under KM_LATENCYMODEL_3V, when the tile accelerator is
 *   still taking the lists past vertex_timeout once they are sent. no render
 *   starts, and render_busy stays set unless the interrupt handler clears it
 *   first, when hold_wait passes 16 or a render ends.
 * else the result is KMSTATUS_SUCCESS */
KMSTATUS kmRenderTexture(PKMSURFACEDESC pTextureSurface);
/* read a surface back into pTexture */
KMSTATUS kmGetTexture(PKMDWORD pTexture, PKMSURFACEDESC pSurfaceDesc);
KMSTATUS kmFreeTexture(PKMSURFACEDESC pSurfaceDesc);
/* set the rectangle of pixels Kamui draws in, up to 2047 on each axis. a 24-bit
 * frame buffer clips on even pixels, with the low edges rounded up and the high
 * edges down. a minimum above its maximum returns KMSTATUS_INVALID_PARAMETER,
 * and else the result is KMSTATUS_SUCCESS */
KMSTATUS kmSetPixelClipping(KMINT32 Xmin, KMINT32 Ymin, KMINT32 Xmax,
                            KMINT32 Ymax);
/* set the width of every stride texture to nWidth texels, a multiple of 32 */
KMSTATUS kmSetStrideWidth(KMINT32 nWidth);
/* frame buffer pixel formats */
typedef enum {
    KM_DSPBPP_RGB565 = 0,
    KM_DSPBPP_RGB555 = 1,
    KM_DSPBPP_ARGB1555 = 3,
    KM_DSPBPP_RGB888 = 4,
    KM_DSPBPP_ARGB8888 = 5
} KMBPPMODE;
/* convert a frame buffer that kmGetTexture read back into a BMP's pixel rows */
KMSTATUS kmuConvertFBtoBMP(PKMDWORD pOutputData, PKMDWORD pInputData,
                           KMINT32 nWidth, KMINT32 nHeight, KMBPPMODE nBpp);
/* texture types: the pixel format in the low byte, and the kind in the high
 * byte, where a kind ending in _MM has mipmaps */
#define KM_TEXTURE_ARGB1555 0x00
#define KM_TEXTURE_RGB565 0x01
#define KM_TEXTURE_ARGB4444 0x02
#define KM_TEXTURE_YUV422 0x03
#define KM_TEXTURE_BUMP 0x04
#define KM_TEXTURE_TWIDDLED 0x0100
#define KM_TEXTURE_TWIDDLED_MM 0x0200
#define KM_TEXTURE_VQ 0x0300
#define KM_TEXTURE_VQ_MM 0x0400
#define KM_TEXTURE_PALETTIZE4 0x0500
#define KM_TEXTURE_PALETTIZE4_MM 0x0600
#define KM_TEXTURE_PALETTIZE8 0x0700
#define KM_TEXTURE_PALETTIZE8_MM 0x0800
#define KM_TEXTURE_RECTANGLE 0x0900
#define KM_TEXTURE_STRIDE 0x0b00
#define KM_TEXTURE_TWIDDLED_RECTANGLE 0x0d00
#define KM_TEXTURE_BMP 0x0e00 /* kmuCreateTwiddledTexture's input */
#define KM_TEXTURE_BMP_MM 0x0f00
#define KM_TEXTURE_SMALLVQ 0x1000
#define KM_TEXTURE_SMALLVQ_MM 0x1100
/* texture side lengths, in texels */
#define KM_MAPSIZE_1 1
#define KM_MAPSIZE_2 2
#define KM_MAPSIZE_4 4
#define KM_MAPSIZE_8 8
#define KM_MAPSIZE_16 16
#define KM_MAPSIZE_32 32
#define KM_MAPSIZE_64 64
#define KM_MAPSIZE_128 128
#define KM_MAPSIZE_256 256
#define KM_MAPSIZE_512 512
#define KM_MAPSIZE_1024 1024
/* twiddle a KM_TEXTURE_BMP texture, 32 bits per texel, into the pixel format
 * that nTextureType's low byte names: KM_TEXTURE_ARGB1555, KM_TEXTURE_RGB565 or
 * KM_TEXTURE_ARGB4444. USize runs from KM_MAPSIZE_8 to KM_MAPSIZE_1024.
 * dithering or mipmaps overwrite the input */
KMSTATUS kmuCreateTwiddledTexture(PKMDWORD pOutputTexture,
                                  PKMDWORD pInputTexture, KMBOOLEAN bAutoMipMap,
                                  KMBOOLEAN bDither, KMINT32 USize,
                                  KMTEXTURETYPE nTextureType);
/* pixel format values of a texture word */
#define KM_PIXELFORMAT_ARGB1555 0x00000000
#define KM_PIXELFORMAT_RGB565 0x08000000
#define KM_PIXELFORMAT_ARGB4444 0x10000000
#define KM_PIXELFORMAT_YUV422 0x18000000
#define KM_PIXELFORMAT_BUMP 0x20000000
#define KM_PIXELFORMAT_PALETTIZED_4BPP 0x28000000
#define KM_PIXELFORMAT_PALETTIZED_8BPP 0x30000000
/* the fields of a polygon header's texture word, as the tile accelerator reads
 * them. TEX_FORMAT holds a KM_PIXELFORMAT_ value */
#define TEX_VQ 0x40000000
#define TEX_FORMAT 0x38000000
#define TEX_NOT_TWIDDLED 0x04000000
#define TEX_STRIDE 0x02000000
/* a palettized texture's palette bank, bits 21 to 26, which counts banks of 16
 * colors. the field covers the TEX_NOT_TWIDDLED and TEX_STRIDE bits, which only
 * the other formats use */
#define TEX_PALETTE 0x07e00000
#define TEX_PALETTE_SHIFT 21
/* the texture's offset in video memory, divided by 8 */
#define TEX_ADDRESS 0x001fffff
/* the field of a polygon header's TSP word, bits 0 to 2, that holds the size
 * code of a texture's height. the width's code takes bits 3 to 5 */
#define TSP_V_SIZE 0x00000007
/* kmuSetTarget's chips, for which the pass tables are made; the Dreamcast has
 * the CLX2 */
#define KMU_TARGET_ARC1 0x00000001
#define KMU_TARGET_CLX1 0x00000002
#define KMU_TARGET_CLX2 0x00000003
typedef KMUINT32 KMUPASSSTATUS;
#define KMU_PASS_OK 0
#define KMU_PASS_ERROR_VERTEXCONTEXT 1
#define KMU_PASS_ERROR_VERTEXCONTEXT_PASS 2
#define KMU_PASS_ERROR_PARAMTYPE 3
#define KMU_PASS_ERROR_LISTTYPE 4
#define KMU_PASS_ERROR_MIPMAP_D_ADJUST 5
#define KMU_PASS_ERROR_FOGMODE 6
#define KMU_PASS_ERROR_FILTERMODE 7
#define KMU_PASS_ERROR_TEXTURESHADINGMODE 8
#define KMU_PASS_ERROR_COLORTYPE 9
#define KMU_PASS_ERROR_SHADINGMODE 10
#define KMU_PASS_ERROR_USERCLIPMODE 11
#define KMU_PASS_ERROR_TRILINEAR_SETTING 12
#define KMU_PASS_ERROR_SPRITE_SETTING 13
#define KMU_PASS_ERROR_BLENDINGMODE_SETTING 14
#define KMU_PASS_ERROR_MODIFIER_SETTING 15
#define KMU_PASS_ERROR_NOT_IMPLEMENTED 120
KMSTATUS kmuSetTarget(KMDWORD dwTarget);
KMUPASSSTATUS kmuGeneratePassTable(PKMVERTEXCONTEXT pVertexContext,
                                   KMUINT32 nNumContext,
                                   PPKMVERTEXCONTEXT ppVertexContextTable,
                                   PKMUINT32 pPass);
KMUPASSSTATUS kmuCheckPassTable(PPKMVERTEXCONTEXT ppVertexContextTable,
                                KMUINT32 nNumContext, PKMUINT32 pPass);
/* store the free bytes of texture memory in *pSizeOfTexture, counting idle
 * blocks and free VQ page slots. store the bytes of its largest free or idle
 * block in *pMaxBlockSizeOfTexture. the result is KMSTATUS_SUCCESS */
KMSTATUS kmGetFreeTextureMem(PKMUINT32 pSizeOfTexture,
                             PKMUINT32 pMaxBlockSizeOfTexture);
/* sort every video memory list with kmiFBSortQueue, then compact each bank with
 * kmiFBGarbageCollect and return its result */
KMSTATUS kmGarbageCollectTexture(void);
/* KMSTATUS_SUCCESS when kmiCopyToFrameBuffer's last transfer by DMA has
 * finished, or when frame_dma_wait makes it wait for each transfer. else
 * KMSTATUS_NOT_FINISH_DMA */
KMSTATUS kmQueryFinishLastTextureDMA(void);
KMSTATUS kmSetFogTableColor(KMPACKEDARGB FogTableColor);
/* pfFogTable holds 128 densities from 0 to 1, by depth */
KMSTATUS kmSetFogTable(PKMFLOAT pfFogTable);
KMSTATUS kmSetFogDensity(KMDWORD fogDensity);

#define KM_TEXTURE_YUV420 0x06 /* a kmLoadYUVTexture input format */
/* load macroblocks of 16 by 16 texels from pTexture into pSurfaceDesc, through
 * the YUV converter. pTexture must be 32-byte aligned. nFormat is
 * KM_TEXTURE_YUV420, 384 bytes a macroblock, or KM_TEXTURE_YUV422, 512 bytes.
 * nWidth and nHeight count macroblocks across and down, 1 to 64 each. a count
 * of 1 by 1 loads the surface as one texture, USize / 16 by VSize / 16
 * macroblocks. both its sides must then be at least 16. any larger count loads
 * nWidth by nHeight macroblocks as a run of textures, as
 * KMF_YUVCONVERT_NUMOF_TEXTURE does. the surface must not be mipmapped,
 * twiddled, VQ or palettized. its PixelFormat must share a bit with
 * KM_PIXELFORMAT_YUV422, which an ARGB1555 or bump surface's doesn't.
 * bWaitEndOfDMA TRUE waits for the transfer to finish.
 *
 * the result is KMSTATUS_INVALID_ADDRESS for an unaligned pTexture, and
 * KMSTATUS_INVALID_TEXTURE_TYPE for another surface, format or count. it is
 * KMSTATUS_HARDWARE_NOT_PRESENTED when the transfer is not queued, and else
 * KMSTATUS_SUCCESS */
KMSTATUS kmLoadYUVTexture(PKMSURFACEDESC pSurfaceDesc, PKMDWORD pTexture,
                          KMINT32 nWidth, KMINT32 nHeight, KMINT32 nFormat,
                          KMBOOLEAN bWaitEndOfDMA);
/* create two surfaces of one size and type, each allocated separately, and
 * free the first when the second fails */
KMSTATUS kmCreateCombinedTextureSurface(PKMSURFACEDESC pSurfaceDesc1,
                                        PKMSURFACEDESC pSurfaceDesc2,
                                        KMINT32 nWidth, KMINT32 nHeight,
                                        KMTEXTURETYPE nTextureType);
/* create nTexture surfaces side by side in one run of texture memory, from
 * pSurfaceDesc[0]. a small VQ type returns KMSTATUS_INVALID_TEXTURE_TYPE. when
 * a surface finds no memory, the surfaces before it keep theirs */
KMSTATUS kmCreateContiguousTextureSurface(PKMSURFACEDESC pSurfaceDesc,
                                          KMINT32 nTexture, KMINT32 nWidth,
                                          KMINT32 nHeight,
                                          KMTEXTURETYPE nTextureType);
/* load part of a texture into its surface. kmLoadTextureBlock loads block
 * nBlockNum, of nBlockSize * 32 bytes. kmLoadTexturePart loads the nSize bytes
 * at nOffset, each of the two rounded down to a multiple of 4. a part that runs
 * past the texture's end stops there. in a 16-bit twiddled texture with
 * mipmaps, blocks and offsets count from the surface's fifth byte: neither call
 * loads its first four.
 *
 * kmLoadTexturePart first returns KMSTATUS_SUCCESS, loading nothing, for an
 * nSize below 4, whatever the texture or offset. past that test, a small VQ
 * texture returns KMSTATUS_INVALID_TEXTURE_TYPE. a part that starts at or past
 * the end returns KMSTATUS_INVALID_BLOCKNUMBER from kmLoadTextureBlock and
 * KMSTATUS_INVALID_ADDRESS from kmLoadTexturePart. else the result is
 * KMSTATUS_SUCCESS */
KMSTATUS kmLoadTextureBlock(PKMSURFACEDESC pSurfaceDesc, PKMDWORD pTexture,
                            KMUINT32 nBlockNum, KMUINT32 nBlockSize);
KMSTATUS kmLoadTexturePart(PKMSURFACEDESC pSurfaceDesc, PKMDWORD pTexture,
                           KMUINT32 nOffset, KMUINT32 nSize);
/* load a rectangle of 16-bit texels, width by height, into a rectangle or
 * stride texture at dst_X, dst_Y. the rectangle comes from src_X, src_Y of a
 * source whose rows are src_U texels wide. with src_U 0, it comes from the
 * source's start, in rows width texels wide. a twiddled, mipmapped, VQ or
 * palettized surface returns KMSTATUS_INVALID_TEXTURE_TYPE, and any other
 * KMSTATUS_SUCCESS */
KMSTATUS kmLoadRectangleTexturePart(PKMSURFACEDESC pSurfaceDesc,
                                    PKMDWORD pTexture, KMUINT32 width,
                                    KMUINT32 height, KMUINT32 dst_X,
                                    KMUINT32 dst_Y, KMUINT32 src_X,
                                    KMUINT32 src_Y, KMUINT32 src_U);
/* load a VQ texture's codebook into its surface, the size its type's codebook
 * takes */
KMSTATUS kmLoadVQCodebook(PKMSURFACEDESC pSurfaceDesc, PKMDWORD pTexture);
/* reload one level of a mipmapped texture into its surface: the level
 * nMipmapCount texels on a side, from pTexture */
KMSTATUS kmReLoadMipmap(PKMSURFACEDESC pSurfaceDesc, PKMVOID pTexture,
                        KMINT32 nMipmapCount);

/* vertex types 0 to 2, in the layout the video chip reads */
typedef struct tagKMVERTEX0 {
    KMDWORD ParamControlWord;
    KMFLOAT fX;
    KMFLOAT fY;
    union {
        KMFLOAT fZ;
        KMFLOAT fInvW;
    } u;
    KMDWORD Reserved0;
    KMDWORD Reserved1;
    KMPACKEDARGB uBaseRGB;
    KMDWORD Reserved2;
} KMVERTEX0, *PKMVERTEX0;
typedef struct tagKMVERTEX1 {
    KMDWORD ParamControlWord;
    KMFLOAT fX;
    KMFLOAT fY;
    union {
        KMFLOAT fZ;
        KMFLOAT fInvW;
    } u;
    KMFLOAT fBaseAlpha;
    KMFLOAT fBaseRed;
    KMFLOAT fBaseGreen;
    KMFLOAT fBaseBlue;
} KMVERTEX1, *PKMVERTEX1;
typedef struct tagKMVERTEX2 {
    KMDWORD ParamControlWord;
    KMFLOAT fX;
    KMFLOAT fY;
    union {
        KMFLOAT fZ;
        KMFLOAT fInvW;
    } u;
    KMDWORD Reserved0;
    KMDWORD Reserved1;
    KMFLOAT fBaseIntensity;
    KMDWORD Reserved2;
} KMVERTEX2, *PKMVERTEX2;

typedef enum { KM_VERTEXTYPE_00 = 0, KM_VERTEXTYPE_03 = 3 } KMVERTEXTYPE;
/* vertex type 3. its control word marks whether the strip continues after
 * it */
typedef struct tagKMVERTEX3 {
    KMDWORD ParamControlWord;
    KMFLOAT fX;
    KMFLOAT fY;
    union {
        KMFLOAT fZ;
        KMFLOAT fInvW;
    } u;
    KMFLOAT fU;
    KMFLOAT fV;
    KMPACKEDARGB uBaseRGB;
    KMPACKEDARGB uOffsetRGB;
} KMVERTEX3, *PKMVERTEX3;
#define KM_VERTEXPARAM_NORMAL 0xe0000000
#define KM_VERTEXPARAM_ENDOFSTRIP 0xf0000000

/* vertex types 4 and up, in the layout the video chip reads. from type 9 on,
 * the 1m values are for the second volume of a polygon a modifier volume
 * touches */
typedef struct tagKMVERTEX4 {
    KMDWORD ParamControlWord;
    KMFLOAT fX;
    KMFLOAT fY;
    union {
        KMFLOAT fZ;
        KMFLOAT fInvW;
    } u;
    KMDWORD dwUV;
    KMDWORD Reserved0;
    KMPACKEDARGB uBaseRGB;
    KMPACKEDARGB uOffsetRGB;
} KMVERTEX4, *PKMVERTEX4;
typedef struct tagKMVERTEX5 {
    KMDWORD ParamControlWord;
    KMFLOAT fX;
    KMFLOAT fY;
    union {
        KMFLOAT fZ;
        KMFLOAT fInvW;
    } u;
    KMFLOAT fU;
    KMFLOAT fV;
    KMDWORD Reserved0;
    KMDWORD Reserved1;
    KMFLOAT fBaseAlpha;
    KMFLOAT fBaseRed;
    KMFLOAT fBaseGreen;
    KMFLOAT fBaseBlue;
    KMFLOAT fOffsetAlpha;
    KMFLOAT fOffsetRed;
    KMFLOAT fOffsetGreen;
    KMFLOAT fOffsetBlue;
} KMVERTEX5, *PKMVERTEX5;
typedef struct tagKMVERTEX6 {
    KMDWORD ParamControlWord;
    KMFLOAT fX;
    KMFLOAT fY;
    union {
        KMFLOAT fZ;
        KMFLOAT fInvW;
    } u;
    KMDWORD dwUV;
    KMDWORD Reserved0;
    KMDWORD Reserved1;
    KMDWORD Reserved2;
    KMFLOAT fBaseAlpha;
    KMFLOAT fBaseRed;
    KMFLOAT fBaseGreen;
    KMFLOAT fBaseBlue;
    KMFLOAT fOffsetAlpha;
    KMFLOAT fOffsetRed;
    KMFLOAT fOffsetGreen;
    KMFLOAT fOffsetBlue;
} KMVERTEX6, *PKMVERTEX6;
typedef struct tagKMVERTEX7 {
    KMDWORD ParamControlWord;
    KMFLOAT fX;
    KMFLOAT fY;
    union {
        KMFLOAT fZ;
        KMFLOAT fInvW;
    } u;
    KMFLOAT fU;
    KMFLOAT fV;
    KMFLOAT fBaseIntensity;
    KMFLOAT fOffsetIntensity;
} KMVERTEX7, *PKMVERTEX7;
typedef struct tagKMVERTEX8 {
    KMDWORD ParamControlWord;
    KMFLOAT fX;
    KMFLOAT fY;
    union {
        KMFLOAT fZ;
        KMFLOAT fInvW;
    } u;
    KMDWORD dwUV;
    KMDWORD Reserved0;
    KMFLOAT fBaseIntensity;
    KMFLOAT fOffsetIntensity;
} KMVERTEX8, *PKMVERTEX8;
typedef struct tagKMVERTEX9 {
    KMDWORD ParamControlWord;
    KMFLOAT fX;
    KMFLOAT fY;
    union {
        KMFLOAT fZ;
        KMFLOAT fInvW;
    } u;
    KMPACKEDARGB uBaseRGB0m;
    KMPACKEDARGB uBaseRGB1m;
    KMDWORD Reserved0;
    KMDWORD Reserved1;
} KMVERTEX9, *PKMVERTEX9;
typedef struct tagKMVERTEX10 {
    KMDWORD ParamControlWord;
    KMFLOAT fX;
    KMFLOAT fY;
    union {
        KMFLOAT fZ;
        KMFLOAT fInvW;
    } u;
    KMFLOAT fBaseIntensity0m;
    KMFLOAT fBaseIntensity1m;
    KMDWORD Reserved0;
    KMDWORD Reserved1;
} KMVERTEX10, *PKMVERTEX10;
typedef struct tagKMVERTEX11 {
    KMDWORD ParamControlWord;
    KMFLOAT fX;
    KMFLOAT fY;
    union {
        KMFLOAT fZ;
        KMFLOAT fInvW;
    } u;
    KMFLOAT fU0m;
    KMFLOAT fV0m;
    KMPACKEDARGB uBaseRGB0m;
    KMPACKEDARGB uOffsetRGB0m;
    KMFLOAT fU1m;
    KMFLOAT fV1m;
    KMPACKEDARGB uBaseRGB1m;
    KMPACKEDARGB uOffsetRGB1m;
    KMDWORD Reserved0;
    KMDWORD Reserved1;
    KMDWORD Reserved2;
    KMDWORD Reserved3;
} KMVERTEX11, *PKMVERTEX11;
typedef struct tagKMVERTEX12 {
    KMDWORD ParamControlWord;
    KMFLOAT fX;
    KMFLOAT fY;
    union {
        KMFLOAT fZ;
        KMFLOAT fInvW;
    } u;
    KMDWORD dwUV0m;
    KMDWORD Reserved0;
    KMPACKEDARGB uBaseRGB0m;
    KMPACKEDARGB uOffsetRGB0m;
    KMDWORD dwUV1m;
    KMDWORD Reserved1;
    KMPACKEDARGB uBaseRGB1m;
    KMPACKEDARGB uOffsetRGB1m;
    KMDWORD Reserved2;
    KMDWORD Reserved3;
    KMDWORD Reserved4;
    KMDWORD Reserved5;
} KMVERTEX12, *PKMVERTEX12;
typedef struct tagKMVERTEX13 {
    KMDWORD ParamControlWord;
    KMFLOAT fX;
    KMFLOAT fY;
    union {
        KMFLOAT fZ;
        KMFLOAT fInvW;
    } u;
    KMFLOAT fU0m;
    KMFLOAT fV0m;
    KMFLOAT fBaseIntensity0m;
    KMFLOAT fOffsetIntensity0m;
    KMFLOAT fU1m;
    KMFLOAT fV1m;
    KMFLOAT fBaseIntensity1m;
    KMFLOAT fOffsetIntensity1m;
    KMDWORD Reserved0;
    KMDWORD Reserved1;
    KMDWORD Reserved2;
    KMDWORD Reserved3;
} KMVERTEX13, *PKMVERTEX13;
typedef struct tagKMVERTEX14 {
    KMDWORD ParamControlWord;
    KMFLOAT fX;
    KMFLOAT fY;
    union {
        KMFLOAT fZ;
        KMFLOAT fInvW;
    } u;
    KMDWORD dwUV0m;
    KMDWORD Reserved0;
    KMFLOAT fBaseIntensity0m;
    KMFLOAT fOffsetIntensity0m;
    KMDWORD dwUV1m;
    KMDWORD Reserved1;
    KMFLOAT fBaseIntensity1m;
    KMFLOAT fOffsetIntensity1m;
    KMDWORD Reserved2;
    KMDWORD Reserved3;
    KMDWORD Reserved4;
    KMDWORD Reserved5;
} KMVERTEX14, *PKMVERTEX14;
/* a sprite, vertex type 16 */
typedef struct tagKMVERTEX16 {
    KMDWORD ParamControlWord;
    KMFLOAT fAX;
    KMFLOAT fAY;
    union {
        KMFLOAT fAZ;
        KMFLOAT fAInvW;
    } uA;
    KMFLOAT fBX;
    KMFLOAT fBY;
    union {
        KMFLOAT fBZ;
        KMFLOAT fBInvW;
    } uB;
    KMFLOAT fCX;
    KMFLOAT fCY;
    union {
        KMFLOAT fCZ;
        KMFLOAT fCInvW;
    } uC;
    KMFLOAT fDX;
    KMFLOAT fDY;
    KMDWORD Reserved0;
    KMDWORD dwUVA;
    KMDWORD dwUVB;
    KMDWORD dwUVC;
} KMVERTEX16, *PKMVERTEX16;

/* start a strip, sending its global parameter */
KMSTATUS kmStartVertexStrip(PKMVERTEXBUFFDESC pBufferDesc);
/* send the StructSize bytes of the vertex at pVertex to the current list, and
 * record VertexType in kmiCurrentListState.Reserved0. StructSize must be 32 or
 * 64. the result is KMSTATUS_SUCCESS */
KMSTATUS kmSetVertex(PKMVERTEXBUFFDESC pBufferDesc, void *pVertex,
                     KMVERTEXTYPE VertexType, KMINT32 StructSize);
/* the words written to a list since its start */
KMDWORD kmGetCurrentVertexOffset(KMINT32 ListType);
/* move a list's write pointer to VertexOffset words from its start */
KMSTATUS kmChangeVertexOffset(KMINT32 ListType, KMDWORD VertexOffset);
/* point kmiDstVertexBuffer at a list's write pointer, write VertexPCW there,
 * then advance the pointer IncPtr words */
KMSTATUS kmChangeVertexPCW(KMINT32 ListType, KMDWORD VertexPCW, KMDWORD IncPtr);
/* send a list to the tile accelerator by DMA, waiting while the DMA queue is
 * full. then move the list's write pointer back to its start, past its header.
 * the write pointer must be past the list's start */
KMSTATUS kmFlushVertexBuffer(KMINT32 ListType);
/* KMSTATUS_LAST_LIST_UNDER_DMA while the last kmFlushVertexBuffer's transfer
 * is waiting or running, else KMSTATUS_SUCCESS */
KMSTATUS kmQueryFinishFlushVertexBuffer(void);
/* copy Size bytes from SrcAdr to dstPtr, a pair of longs at a time when all
 * three are multiples of 8. otherwise copy Size / 4 longs, as kmiMemCopy4 does.
 * dstPtr and SrcAdr must be 4-byte aligned, and Size at least 4. FPSCR's PR and
 * SZ bits must be clear, and they are clear again on return. the result is no
 * pointer. it is dstPtr | SrcAdr | Size after a copy by pairs, and the value of
 * the last long copied after a copy by longs */
PKMDWORD kmiMemCopy8(PKMDWORD dstPtr, PKMDWORD SrcAdr, KMDWORD Size);

/* start each list over past its header, in the vertex buffer bank kmRender last
 * flipped to. what was written to the lists is dropped, not rendered. under
 * KM_LATENCYMODEL_2V, kmResetPointer first resets the tile accelerator and sets
 * it up for the write slot's bank */
void kmResetPointer(void);

/* a display mode holds its video output, a KMVIDEOMODE, in its low two bits.
 * the bits above hold its scan type, width and height. VGA's one display mode
 * is KM_DSPMODE_VGA */
typedef enum { KM_NTSC = 0, KM_VGA = 1, KM_PAL = 2 } KMVIDEOMODE;
#define KM_DSPMODE_INTERLACE 0x04
#define KM_DSPMODE_PSEUDONONINTERLACE 0x08
#define KM_DSPMODE_WIDTH_640 0x010
#define KM_DSPMODE_HEIGHT_480 0x020
typedef enum { KM_DSPMODE_VGA = KM_VGA } KMDISPLAYMODE;

#define KM_PALETTE_ENTRY 1024
typedef union _tagKMPALETTEDATA {
    KMDWORD dwPaletteData[KM_PALETTE_ENTRY];
} KMPALETTEDATA, *PKMPALETTEDATA;

/* kmSetUserClipLevelAdjust's types: the lines each field's display read skips
 * at the top. KM_LEVEL_ADJUST_NORMAL skips none, and KM_LEVEL_ADJUST_HALF skips
 * 16 on VGA or at 480 lines, and 8 otherwise */
typedef enum _tagKMADJUSTTYPE {
    KM_LEVEL_ADJUST_NORMAL,
    KM_LEVEL_ADJUST_HALF
} KMADJUSTTYPE;

/* set the size below which polygons are culled */
KMSTATUS kmSetCullingRegister(KMFLOAT fCullVal);

/* the latency models. under the 3V model each list has two banks of vertex
 * buffer, under the 2V model one */
typedef enum { KM_LATENCYMODEL_3V = 0, KM_LATENCYMODEL_2V = 1 } KMLATENCYMODEL;
/* vertex buffer models. under KM_LATENCYMODEL_2V, a NOBUF model sends one list
 * straight to the tile accelerator, with no buffer. the model's low four bits
 * are that list's ListType */
typedef enum {
    KM_VERTEXBUFMODEL_NORMAL = 0x00,
    KM_VERTEXBUFMODEL_NOBUF_OPAQUE = 0x20,
    KM_VERTEXBUFMODEL_NOBUF_OPAQUE_MODIFIER = 0x21,
    KM_VERTEXBUFMODEL_NOBUF_TRANS = 0x22,
    KM_VERTEXBUFMODEL_NOBUF_TRANS_MODIFIER = 0x23,
    KM_VERTEXBUFMODEL_NOBUF_PUNCH_THROUGH = 0x24
} KMVERTEXBUFMODEL;

/* kmSetSystemConfiguration's flags */
#define KM_CONFIGFLAG_ENABLE_CLEAR_FRAMEBUFFER 0x00000001
#define KM_CONFIGFLAG_ENABLE_STRIPBUFFER 0x00000002
#define KM_CONFIGFLAG_ENABLE_2V_LATENCY 0x00000010
#define KM_CONFIGFLAG_NOWAITVSYNC 0x00000020
#define KM_CONFIGFLAG_USEDIRECTMODE 0x00010000
#define KM_CONFIGFLAG_NOWAIT_FINISH_TEXTUREDMA 0x00100000

/* what kmSetSystemConfiguration sets up: the frame buffers, the texture
 * memory, and the vertex buffer with each list's percentage of it, by list
 * type */
typedef struct _tagKMSYSTEMCONFIGSTRUCT {
    KMDWORD dwSize;
    KMDWORD flags; /* KM_CONFIGFLAG_ bits */
    PKMSURFACEDESC *ppSurfaceDescArray; /* the front, then the back buffer */
    KMUINT32 nNumOfFrameBuffer;
    KMUINT32 nWidth;
    KMUINT32 nHeight;
    KMBPPMODE nBpp;
    KMUINT32 nTextureMemorySize;
    PKMVERTEXBUFFDESC pBufferDesc;
    PKMDWORD pVertexBuffer;
    KMUINT32 nVertexBufferSize;
    KMFLOAT fBufferSize[5];
    KMDWORD reserved01;
    KMDWORD reserved02;
    KMDWORD reserved03;
    KMDWORD reserved04;
    KMDWORD reserved05;
    KMDWORD reserved06;
    KMDWORD reserved07;
    KMVERTEXBUFMODEL nVbufModel;
} KMSYSTEMCONFIGSTRUCT, *PKMSYSTEMCONFIGSTRUCT;

/* make the frame buffers, the vertex buffer and the tile accelerator's two
 * buffers that pSystemConfigStruct describes. kmSetSystemConfiguration first
 * empties video memory with kmiFBinit, and calls kmiDiscardRenderPipeline.
 *
 * the vertex buffer starts at pVertexBuffer, rounded up to 32 bytes. under
 * KM_LATENCYMODEL_3V, each of its two banks takes half of it. each list's size
 * counts words. it is the percentage of a bank that the list's fBufferSize
 * entry gives, by ListType, rounded down to a multiple of 8. when the five
 * entries sum to more than 100, each is scaled down in proportion, so that they
 * sum to 100. on a chip with KMI_CAP_DUMMY_OPAQUE, an opaque size below 56 then
 * grows by 56. when the sizes sum to more than a bank, the largest list gives
 * up the excess, the first of equals. its size is then rounded down to a
 * multiple of 32. last, each size that isn't 0 shrinks by 16.
 *
 * under KM_CONFIGFLAG_USEDIRECTMODE, there is no vertex buffer: the lists in
 * pBufferDesc's fActiveList go straight to the tile accelerator.
 *
 * the result is KMSTATUS_NOT_ENOUGH_MEMORY when the tile accelerator's buffers
 * can't hold its region array and object pointer blocks. a failure to allocate
 * those buffers returns its status. otherwise kmSetSystemConfiguration makes
 * the frame buffers with kmiAllocSysConfig, and returns its status */
KMSTATUS kmSetSystemConfiguration(PKMSYSTEMCONFIGSTRUCT pSystemConfigStruct);

/* kmCreateFrameBufferSurface, kmActivateFrameBuffer, and kmCreateVertexBuffer
 * or kmCreateTABuffer make in steps the configuration that
 * kmSetSystemConfiguration makes in one call. until the configuration is made,
 * each of them records its request in kmiSystemConfig. kmActivateFrameBuffer,
 * kmCreateVertexBuffer and kmCreateTABuffer make the configuration once they
 * find the three requests recorded, their own included.
 * kmCreateFrameBufferSurface never makes it. pBufferDesc and the surfaces must
 * stay valid until the configuration is made. after that, each call acts at
 * once */

/* record the surface as the next of kmiSystemConfig's two frame buffer
 * requests. once the configuration is made, kmCreateFrameBufferSurface also
 * allocates the surface, and clears it when fBufferClear is TRUE. the result is
 * then the allocation's status, and before that KMSTATUS_SUCCESS */
KMSTATUS kmCreateFrameBufferSurface(PKMSURFACEDESC pSurfaceDesc, KMINT32 nWidth,
                                    KMINT32 nHeight, KMBOOLEAN fStripBuffer,
                                    KMBOOLEAN fBufferClear);
/* record the front and back frame buffers, fStripBuffer and fWaitVSync. once
 * the configuration is made, kmActivateFrameBuffer also points the video chip's
 * two frame buffer write addresses at the front and back buffers. both read
 * addresses then go to the front buffer. with fStripBuffer TRUE, the second
 * read address goes to the back buffer instead, and each field's read offset is
 * set to 0.
 *
 * the result is KMSTATUS_INVALID_SURFACE, with nothing recorded, when either
 * surface is null. once the configuration is made, it is
 * KMSTATUS_INVALID_SURFACE_TYPE when the two differ in fSurfaceFlags, and
 * KMSTATUS_INVALID_SURFACE_SIZE when they differ in size. the call that makes
 * the configuration returns kmiAllocSysConfig's status. else the result is
 * KMSTATUS_SUCCESS */
KMSTATUS kmActivateFrameBuffer(PKMSURFACEDESC pPrimarySurfaceDesc,
                               PKMSURFACEDESC pBackBufferSurfaceDesc,
                               KMBOOLEAN fStripBuffer, KMBOOLEAN fWaitVSync);
/* give the opaque polygon and modifier lists and the translucent polygon and
 * modifier lists vertex buffers, laid out from kmiDeviceExtension.vertex_base.
 * the lists take one bank under KM_LATENCYMODEL_2V, and two otherwise.
 * kmCreateVertexBuffer also gives the tile accelerator two buffers for them,
 * and points pBufferDesc at the lists. each size counts words. on a chip with
 * KMI_CAP_DUMMY_OPAQUE, the opaque list's size first grows by 32 words. in a
 * request recorded before the configuration is made, it grows by 32 twice: when
 * recorded, and when made. each size that isn't 0 then shrinks by 16 words. the
 * result is KMSTATUS_SUCCESS, or the status of allocating the tile
 * accelerator's buffers when that fails. the call that makes the configuration
 * returns kmiAllocSysConfig's status */
KMSTATUS kmCreateVertexBuffer(PKMVERTEXBUFFDESC pBufferDesc,
                              KMINT32 OpaquePolygonBuffer,
                              KMINT32 OpaqueModifierBuffer,
                              KMINT32 TransPolygonBuffer,
                              KMINT32 TransModifierBuffer);
/* give the tile accelerator two buffers, TABuffer bytes in all, for the lists
 * in pBufferDesc's fActiveList to go straight to it. the result is
 * KMSTATUS_SUCCESS, or the status of allocating the buffers when that fails.
 * the call that makes the configuration returns kmiAllocSysConfig's status.
 *
 * a call that leaves the configuration waiting returns no status of its own, a
 * shipped bug, so its caller can't test the result. 1.20J's returns
 * buffer_mode's KMI_BUFFERS_TA, 2, and 1.30J's the value its caller left in
 * R12 */
KMSTATUS kmCreateTABuffer(PKMVERTEXBUFFDESC pBufferDesc, KMINT32 TABuffer);
/* free the tile accelerator buffers kmCreateVertexBuffer or kmCreateTABuffer
 * last allocated, then forget the lists' headers. the result is kmiFBfreeEx's
 * status, and a failure leaves the headers as they were */
KMSTATUS kmDiscardVertexBuffer(PKMVERTEXBUFFDESC pBufferDesc);
/* wait for the tile accelerator to finish, running the wait-vsync callback, for
 * as long as vsync_count stays at most 10. then set the latency and vertex
 * buffer models, and reset the slots. the result is KMSTATUS_SUCCESS */
KMSTATUS kmChangeLatencyModel(KMLATENCYMODEL LatencyModel,
                              KMVERTEXBUFMODEL VbufModel);

/* reset Kamui's driver state, then set the hardware up for video output
 * nVideoMode. kmInitDevice reads nVideoMode through the profile's VideoMode
 * key, which the fixed profile leaves as it is. a mode outside KM_NTSC to
 * KM_PAL returns KMSTATUS_INVALID_VIDEO_MODE after the reset, with the hardware
 * not set up. else the result is KMSTATUS_SUCCESS */
KMSTATUS kmInitDevice(KMVIDEOMODE nVideoMode);
/* discard the render pipeline, then set the display up for nDisplayMode at
 * nBpp, with fDither and fAntiAlias, and leave it blanked. VGA shows 480 lines,
 * and any other video output 240 per field. kmSetDisplayMode reads its
 * arguments through the profile's DisplayMode, AntiAlias and Dither keys, which
 * the fixed profile leaves as they are. before any test, it sets video_mode to
 * nDisplayMode's low two bits, its video output.
 *
 * an nBpp outside KMBPPMODE returns KMSTATUS_INVALID_DISPLAY_MODE, and sets
 * nothing else up. a display mode whose low two bits are 3, which names no
 * video output, also returns KMSTATUS_INVALID_DISPLAY_MODE. kmSetDisplayMode
 * sets that mode's frame buffer and display registers up before it returns, but
 * no output's timing. else the result is KMSTATUS_SUCCESS */
KMSTATUS kmSetDisplayMode(KMDISPLAYMODE nDisplayMode, KMBPPMODE nBpp,
                          KMBOOLEAN fDither, KMBOOLEAN fAntiAlias);
/* blank the picture and stop the renderer */
KMSTATUS kmStopDisplayFrameBuffer(void);
/* move the picture nXAdjust right and nYAdjust down from its origins. it moves
 * across only when origin_x + nXAdjust is above 10, and down only when
 * origin_y1 + nYAdjust or origin_y2 + nYAdjust is above 0. moved down, the
 * display reads fewer lines, so that the picture still ends on screen. the
 * result is KMSTATUS_SUCCESS */
KMSTATUS kmAdjustDisplayCenter(long nXAdjust, long nYAdjust);
/* make the interrupt handler move the slots on once in every nCount vertical
 * blanks it counts. the new count takes effect when the count under way runs
 * out. nCount must be at least 1. the result is KMSTATUS_SUCCESS */
KMSTATUS kmSetWaitVsyncCount(KMINT32 nCount);
/* take the background plane's colors and its ISP, TSP and texture words from
 * pVertexContext */
KMSTATUS kmSetBackGroundRenderState(PKMVERTEXCONTEXT pVertexContext);
/* write the background plane's packet from pVertex's three vertices of
 * VertexType, then clear the low 20 bits of the packet's first word, its ISP
 * word. VertexType indexes kmiBackGroundPlaneFuncs unchecked, so it must be
 * below 18. types 15 to 17 write no packet and return
 * KMSTATUS_INVALID_VERTEX_TYPE, and the others return KMSTATUS_SUCCESS.
 * StructSize is not read */
KMSTATUS kmSetBackGroundPlane(PVOID pVertex[3], KMVERTEXTYPE VertexType,
                              KMINT32 StructSize);
KMSTATUS kmSetColorClampValue(KMPACKEDARGB MaxVal, KMPACKEDARGB MinVal);
/* send ListType a user clip of the tiles from Xmin, Ymin to Xmax, Ymax. unless
 * the list holds exactly LIST_HEADER_WORDS words, as it does with only its
 * opening header, a blank header and two copies of a sprite off the screen go
 * first. the blank header keeps the global parameter's group bit and user clip
 * mode. in a modifier list, it heads a modifier volume, and in any other list a
 * sprite. the result is KMSTATUS_SUCCESS */
KMSTATUS kmSetUserClipping(PKMVERTEXBUFFDESC pBufferDesc, KMINT32 ListType,
                           KMINT32 Xmin, KMINT32 Ymin, KMINT32 Xmax,
                           KMINT32 Ymax);

/* Kamui's internals */

/* start addresses of the tile accelerator's buffers */
struct kmi_ta_layout {
    KMDWORD region_array;
    KMDWORD object_list;
    KMDWORD isp_params;
    KMDWORD tsp_params;
};

struct kmi_version_info {
    KMDWORD major, minor, revision, build;
    /* the driver's notice: 621 bytes, with no NUL after them, so not a C
     * string */
    char *copyright;
};

struct kmi_callback {
    PKMCALLBACKFUNC function;
    void *argument;
};

/* one of the tile accelerator's two banks: its start, and its register values
 * for the tile accelerator and the renderer */
struct kmi_ta_bank {
    /* where kmiSetTAEnvironment lays the bank out from. the bank takes
     * ta_buffer_size bytes. its ISP parameters come first, and its region array
     * and any dummy translucent object come last */
    KMDWORD start;
    KMDWORD region_base;
    KMDWORD unk_08;
    KMDWORD ol_base;
    KMDWORD ol_limit;
    KMDWORD next_opb_init;
    KMDWORD isp_base;
    KMDWORD isp_limit;
};

/* kmi_device_extension.slot_state's values. from SLOT_FILLING up, the states
 * mark a frame's stages: an even state is a stage under way, and an odd one a
 * stage done */
enum {
    /* ta_slot's state after an ISP parameter overflow. the interrupt handler
     * shows such a slot as it shows a SLOT_RENDERED one */
    SLOT_OVERFLOW = -3,
    SLOT_FREE = -1, /* the slot holds no frame */
    SLOT_FILLING, /* the write slot, while Kamui writes its lists */
    SLOT_READY, /* the interrupt handler is to send its lists on */
    SLOT_IN_TA, /* the tile accelerator takes its lists */
    SLOT_TA_DONE,
    SLOT_RENDERING,
    SLOT_RENDERED,
    SLOT_SHOWN = 7
};

/* kmi_device_extension.callback's indexes, one for each event, and CALLBACKS,
 * their count */
enum {
    END_RENDER_CALLBACK,
    VSYNC_CALLBACK,
    HSYNC_CALLBACK,
    TEXTURE_OVERFLOW_CALLBACK,
    STRIP_OVERRUN_CALLBACK,
    END_VERTEX_CALLBACK,
    END_YUV_CALLBACK,
    WAIT_VSYNC_CALLBACK,
    FATAL_ERROR_CALLBACK,
    CALLBACKS
};

/* kmi_device_extension.capabilities' bits. with KMI_CAP_ISP_CURRENT,
 * kmiFinalizeRender takes TA_ISP_CURRENT as the end of a bank's ISP
 * parameters, where it copies the background packet */
#define KMI_CAP_ISP_CURRENT 0x01
/* the chip reports the field and the scan line in SYNC_STATUS */
#define KMI_CAP_SYNC_STATUS 0x02
/* the frame buffer, texture and VQ page allocators choose a block's bank by
 * the room each bank has, as their contracts say, rather than take a fixed
 * one */
#define KMI_CAP_BALANCE_BANKS 0x04
/* Kamui resets the renderer before each render */
#define KMI_CAP_RESET_BEFORE_RENDER 0x10
/* each tile takes two region array entries, the second writing the tile out,
 * or four with dummy_translucent set */
#define KMI_CAP_TWO_ENTRIES 0x20
/* kmRender writes kmiDummyOpaque into the opaque list, and the opaque list's
 * vertex buffer grows to hold it */
#define KMI_CAP_DUMMY_OPAQUE 0x40

/* kmi_device_extension.fb_read_size's line field, bits 10 to 19: the lines
 * read, less one */
#define READ_LINES 0x3ff
#define READ_LINES_SHIFT 10

/* Kamui's record of the video chip and its use of it, as far as the sources
 * read it. 1.20J's Kamui, which LOADER.BIN links, keeps the same record */
struct kmi_device_extension {
    struct kmi_version_info *version;
    KMDWORD core; /* the chip's core, 5 or 6. Kamui runs only on 6 */
    /* the chip's KMI_CAP_ bits, which kmiInitializeDevice sets by
     * BUS_REVISION. for a BUS_REVISION above 2, it also sets 0x08, which no
     * source names or tests. kmfSetTimeOutCount also sets and clears
     * KMI_CAP_RESET_BEFORE_RENDER */
    KMDWORD capabilities;
    unsigned char unk_00c[8];
    KMDWORD memory_alignment;
    KMDWORD texture_memory_size; /* 8 MB, or 16 MB if the profile allows */
    KMDWORD unk_01c;
    KMINT32 video_mode;
    KMINT32 display_mode;
    KMINT32 bpp; /* the frame buffer's KMBPPMODE */
    KMBOOLEAN dither;
    KMBOOLEAN anti_alias;
    /* whether the frame buffer is drawn a strip at a time */
    KMBOOLEAN strip_buffer;
    KMINT32 level_adjust; /* the kmSetUserClipLevelAdjust type */
    KMINT32 skip_lines; /* the lines level_adjust skips at the top */
    KMDWORD fb_width; /* the frame buffer's line length in longs, less one */
    KMDWORD fb_lines; /* the frame buffer's line count, less one */
    KMDWORD display_width;
    KMDWORD display_height;
    /* offsets the program sets. Kamui adds them to the origins each video
     * output gives, origin_x across and origin_y1 and origin_y2 for the two
     * fields, to make video_start_x and video_start_y */
    KMINT32 offset_x;
    KMINT32 offset_y;
    KMINT32 origin_x;
    KMINT32 origin_y1;
    KMINT32 origin_y2;
    KMDWORD polling_ta;
    /* the options kmiSetTAEnvironment takes. kmiFBControl sets
     * OPTION_BY_COLUMNS for anti-aliasing, which renders the frame twice as
     * wide. while OPTION_BY_COLUMNS is set, the region arrays list the tiles
     * by columns */
    KMDWORD ta_options;
    struct kmi_ta_layout ta_layout[2];
    KMDWORD bank_limit[2];
    KMDWORD isp_size;
    KMDWORD tsp_size;
    KMINT32 palette_mode;
    KMINT32 alpha_threshold;
    /* the background plane. its packet starts with the ISP, TSP and texture
     * words */
    KMDWORD background_packet[62];
    KMDWORD background_size; /* the packet's size in bytes */
    KMDWORD background_isp;
    KMDWORD background_tsp;
    KMDWORD background_texture;
    KMDWORD background_skip; /* the plane tag's skip, shadow and bypass bits */
    KMDWORD background_depth;
    /* the plane's tag: background_skip's bits, and the word offset of the
     * plane's packet in the ISP parameters */
    KMDWORD background_tag;
    KMDWORD background_base_color;
    KMDWORD background_offset_color;
    /* a bit for each bank whose region array is to be rebuilt */
    KMINT32 regions_stale;
    /* set when translucent polygons are left in the order they were sent */
    KMBOOLEAN presort;
    KMBOOLEAN global_clip;
    KMDWORD global_clip_width;
    KMDWORD global_clip_height;
    /* the modifier list kmUseAnotherModifier selected */
    KMBOOLEAN another_modifier;
    KMINT32 another_modifier_list;
    /* the strip length of a context that does not set its own */
    KMDWORD strip_length;
    /* the countdown to the slots' next move. the interrupt handler takes one
     * off it at each vertical blank it counts. when the countdown falls below
     * 0, the handler lets the slots move and sets it to vsync_wait again */
    KMINT32 vsync_remaining;
    /* the kmSetWaitVsyncCount count, less one */
    KMINT32 vsync_wait;
    /* the fWaitVSync of the frame buffers kmActivateFrameBuffer last set */
    KMBOOLEAN wait_vsync;
    KMDWORD unk_1ec;
    unsigned char unk_1f0[0x18];
    /* render_busy is nonzero while the renderer is busy, and ta_busy while the
     * tile accelerator is */
    KMDWORD render_busy;
    KMDWORD ta_busy;
    unsigned char unk_210[8];
    /* each field's display read start, as an offset from the frame buffer's
     * start */
    KMINT32 field_offset[2];
    unsigned char unk_220[4];
    KMDWORD render_timeout;
    KMDWORD vertex_timeout;
    KMDWORD vsync_timeout;
    /* each event's callback, by its _CALLBACK index */
    struct kmi_callback callback[CALLBACKS];
    unsigned char unk_278[4];
    /* the stack pointer kmRenderTexture stores at the end of a render */
    KMDWORD stack_pointer;
    unsigned char unk_280[0x20];
    KMBOOLEAN display_changed;
    KMDWORD unk_2a4;
    /* a bit for each callback the interrupt handler is to run, by its index
     * in callback */
    KMDWORD callback_pending;
    /* the values Kamui writes to the chip's frame buffer, sync and video output
     * registers */
    KMDWORD fb_clip_x;
    KMDWORD fb_clip_y;
    KMDWORD unk_2b4;
    KMDWORD fb_read_size;
    KMDWORD fb_write_stride;
    KMDWORD fb_read_control;
    KMDWORD fb_write_control;
    KMDWORD fb_read_start1;
    KMDWORD fb_read_start2;
    unsigned char unk_2d0[8];
    KMDWORD sync_control;
    KMDWORD hblank_interrupt;
    KMDWORD vblank_interrupt;
    KMDWORD hblank;
    KMDWORD sync_load;
    KMDWORD vblank;
    KMDWORD sync_width;
    KMDWORD y_coefficients;
    KMDWORD scaler_control;
    KMDWORD video_control;
    KMDWORD video_start_x;
    KMDWORD video_start_y;
    unsigned char unk_308[4];
    KMDWORD feed_config;
    /* the lines where the picture ends and starts, and its line count, 480 on
     * VGA and otherwise 240 */
    KMDWORD picture_end;
    KMDWORD picture_start;
    KMDWORD picture_lines;
    unsigned char unk_31c[0x10];
    /* each frame buffer's start, as the frame buffer start registers take it */
    KMDWORD fb_start[2];
    KMDWORD unk_334[2];
    KMDWORD latency_model; /* a KMLATENCYMODEL */
    unsigned char unk_340[4];
    /* each of the four frame slots' SLOT_ state, by slot number. a slot's bank
     * of the tile accelerator is its number & 1 */
    KMINT32 slot_state[4];
    /* the slot whose lists are being written */
    KMINT32 write_slot;
    /* the slot the renderer draws */
    KMINT32 render_slot;
    /* the slot whose lists the tile accelerator takes */
    KMINT32 ta_slot;
    /* a bit for each list the tile accelerator is given, by ListType */
    KMDWORD lists_sent;
    /* a bit for each list the tile accelerator has finished, by ListType */
    KMDWORD lists_done;
    KMDWORD vsync_count;
    /* set until the interrupt handler next moves the slots on at a vertical
     * blank. kmFlipFrameBuffer waits while it is set */
    KMINT32 flip_pending;
    /* nonzero while a frame renders into a texture */
    KMDWORD texture_render;
    /* set when the tile accelerator is handed the write slot, by
     * kmFlipFrameBuffer after kmRenderDirect or by kmRenderTextureDirect.
     * kmRenderDirect and kmiInitInterrupt clear it. the interrupt handler holds
     * every slot while it is set */
    KMDWORD slots_held;
    /* each list's start in each of the four slots, by ListType. word holds the
     * same starts as one array of twenty, list by list */
    union {
        PKMDWORD row[5][4];
        PKMDWORD word[20];
    } list_start;
    /* each list's size in bytes in each of the four slots, by ListType */
    KMDWORD list_size[5][4];
    KMDWORD vertex_buffer_model; /* a KMVERTEXBUFMODEL */
    KMDWORD vertex_buffer_list; /* vertex_buffer_model's ListType */
    KMDWORD flush_dma; /* the handle of kmFlushVertexBuffer's last transfer */
    /* frame_dma holds the handle of kmiCopyToFrameBuffer's last transfer by
     * DMA. frame_dma_wait is 1 to wait for each such transfer to finish */
    KMINT32 frame_dma;
    KMINT32 frame_dma_wait;
    /* common_modifier is 1 to send common_modifier_size bytes from
     * common_modifier_data to the tile accelerator ahead of each modifier list.
     * Kamui sets the list's type in the first of those words */
    KMBOOLEAN common_modifier;
    PKMDWORD common_modifier_data;
    KMDWORD common_modifier_size;
    KMDWORD vertex_base;
    KMDWORD vertex_size;
    KMDWORD unk_440;
    /* the bytes at each bank's start set apart for its ISP parameters, three
     * quarters of ta_buffer_size */
    KMDWORD isp_bytes;
    KMDWORD unk_448;
    KMDWORD unk_44c;
    /* the bytes of each of the tile accelerator's two buffers */
    KMDWORD ta_buffer_size;
    /* the frame's size in tiles of 32 by 32 pixels */
    KMDWORD tiles_x;
    KMDWORD tiles_y;
    /* set, on a chip with KMI_CAP_TWO_ENTRIES, when the opaque modifier,
     * translucent and punch-through lists all have object pointer blocks. each
     * tile then takes four region array entries. the second draws the dummy
     * translucent object, one triangle that kmiCreateRegionArray writes past
     * the bank's region arrays */
    KMDWORD dummy_translucent;
    KMDWORD lists_used; /* a bit for each list in use, by ListType */
    struct kmi_ta_bank ta_bank[2];
    /* set when the tile accelerator allocates object pointer blocks downward,
     * from just below each bank's region array toward its ISP parameters */
    KMDWORD opb_downward;
    /* each list's object pointer block size, in ListType order, as a 2-bit
     * code. 0 gives the list no blocks, and 1, 2 and 3 give it blocks of 8, 16
     * and 32 words */
    KMDWORD opaque_opb;
    KMDWORD opaque_modifier_opb;
    KMDWORD translucent_opb;
    KMDWORD translucent_modifier_opb;
    KMDWORD punchthrough_opb;
    KMDWORD list_limit[5]; /* each list's vertex buffer bytes, by ListType */
};
/* volatile: the interrupt handler changes fields that other functions wait on,
 * such as flip_pending, ta_busy and render_busy */
extern volatile struct kmi_device_extension kmiDeviceExtension;
/* run the wait-vsync callback with its argument */
#define RUN_WAIT_VSYNC()                                                       \
    kmiDeviceExtension.callback[WAIT_VSYNC_CALLBACK].function(                 \
        kmiDeviceExtension.callback[WAIT_VSYNC_CALLBACK].argument)
/* the device record kmiInitializeDriver selects, which is kmiDeviceExtension.
 * it is null before kmiInitializeDriver and after kmiReleaseHardware */
extern volatile struct kmi_device_extension *volatile kmiHardwareResource;

/* 1 on VGA, else 0 */
/* @hack redundant: ? 1 : 0 on a comparison that is already 1 or 0, which has
 * SHC compute the 0 or 1 into a register and then test that register, as the
 * image does, rather than test the mode's bit. without it, in the five
 * functions that use it in the two images, 339 of 487 words differ */
#define VGA_MODE ((kmiDeviceExtension.display_mode & KM_VGA) == KM_VGA ? 1 : 0)
/* 1 when the screen is sent as two fields, else 0 */
/* @hack spelling: conditional expressions, which have SHC compute the 0 or 1
 * into a register and then test that register, as the image does, rather than
 * test the mode's bits. as the two tests joined by &&, which give the same 1 or
 * 0, in the six functions that use it in the two images, 751 of 790 words
 * differ */
#define TWO_FIELDS                                                             \
    ((kmiDeviceExtension.display_mode & KM_VGA) == KM_VGA ? 0                  \
     : (kmiDeviceExtension.display_mode &                                      \
        (KM_DSPMODE_INTERLACE | KM_DSPMODE_PSEUDONONINTERLACE |                \
         KM_DSPMODE_HEIGHT_480))                                               \
         ? 1                                                                   \
         : 0)
/* 1 for a 480-line mode that isn't VGA, interlaced or pseudo non-interlaced,
 * else 0. the flicker filter halves such a mode's lines */
/* @hack spelling: conditional expressions, which have SHC compute the 0 or 1
 * into a register and then test that register, as the image does, rather than
 * test the mode's bits. as the two tests joined by &&, which give the same 1 or
 * 0, in the seven functions that use it in the two images, 685 of 1,221 words
 * differ */
#define FLICKER_FREE                                                           \
    ((kmiDeviceExtension.display_mode & KM_VGA) == KM_VGA ? 0                  \
     : (kmiDeviceExtension.display_mode &                                      \
        (KM_DSPMODE_INTERLACE | KM_DSPMODE_PSEUDONONINTERLACE |                \
         KM_DSPMODE_HEIGHT_480)) == KM_DSPMODE_HEIGHT_480                      \
         ? 1                                                                   \
         : 0)

/* the system bus's revision register */
#define BUS_REVISION (*(KMDWORD *)0xa05f689c)

void kmiInitCallback(void);
/* with interrupts masked, give device's callback slot, a _CALLBACK index,
 * callback and argument. a null callback gives the slot its default handler
 * instead, and leaves the slot's argument as it was */
void kmiRegisterCallback(volatile struct kmi_device_extension *device,
                         KMINT32 slot, PKMCALLBACKFUNC callback,
                         void *argument);
/* each returns TRUE and writes no register, whatever enable is */
KMINT32 kmiEnableVSyncInterrupt(KMBOOLEAN enable);
KMINT32 kmiEnableHSyncInterrupt(KMBOOLEAN enable);

/* write and read a video chip register by its offset */
KMINT32 kmiWriteRegister(KMDWORD reg, KMDWORD value);
KMDWORD kmiReadRegister(KMDWORD reg);
/* copy size / 4 longs from src to dst. dst and src must be 4-byte aligned,
 * and size at least 4 */
void kmiMemCopy4(void *dst, void *src, KMDWORD size);
/* kmiCopyToPVRMemory copies size / 4 longs with kmiMemCopy4 from src into video
 * memory, at offset dst in the 32-bit view. kmiCopyFromPVRMemory copies them
 * out of video memory, from offset src, into dst */
KMINT32 kmiCopyToPVRMemory(void *dst, void *src, KMDWORD size);
KMINT32 kmiCopyFromPVRMemory(void *dst, void *src, KMDWORD size);
/* copy size / 4 longs from src into video memory at offset in the 64-bit view.
 * when offset and src are 32-byte aligned and the interrupt mask is 0, the size
 * rounded down to 32 bytes goes by DMA. the rest goes by kmiMemCopy4. frame_dma
 * takes the transfer's handle, and kmiCopyToFrameBuffer waits for the transfer
 * when frame_dma_wait is 1. otherwise all of it goes by kmiMemCopy4 */
KMINT32 kmiCopyToFrameBuffer(KMDWORD offset, KMDWORD *src, KMDWORD size);
/* copy size / 4 longs out of video memory at offset in the 64-bit view into
 * dst, then write dst's cache lines back */
KMINT32 kmiCopyFromFrameBuffer(KMDWORD *dst, KMDWORD offset, KMDWORD size);
/* the profile's setting for key in section. the profile is fixed, with no file
 * behind it. in section "HARDWARE", key "EnableTexture16MB" gives 0, and any
 * other key a value no code sets. in any other section, every key gives
 * fallback */
KMINT32 kmiGetProfileInt(const char *section, const char *key,
                         KMINT32 fallback);
/* reset the video chip's tile accelerator and renderer */
void kmiResetBoard(void);
/* reset the slots, then wait for the renderer to finish, and under
 * KM_LATENCYMODEL_3V for the tile accelerator too. each wait runs the
 * wait-vsync callback for as long as vsync_count stays at most 10. then mark
 * both idle */
void kmiDiscardRenderPipeline(void);
/* set DMA channel 2 up, aim both store queues at area 4, and empty both
 * request queues */
void kmiInitializeDMA(void);
/* kmiDMAtoTARequest's results */
#define DMA_QUEUED 0
#define DMA_QUEUE_FULL 0x100
#define DMA_EMPTY 0x103
/* kmiDMAtoTARequest's flags */
#define DMA_CYCLE_STEAL 4 /* cycle steal mode, rather than burst */
#define DMA_SECOND_QUEUE 8 /* the second queue, rather than the first */
#define DMA_SOURCE_UP 0x10 /* a source address that increments */
/* queue a transfer of length bytes from source to target, on the second queue
 * when flags holds DMA_SECOND_QUEUE, and else on the first. source and length
 * are rounded up to 32 bytes. the transfer starts at once when no queued
 * transfer runs. the result is DMA_EMPTY for a length of 0, and DMA_QUEUE_FULL
 * when the queue's next slot is busy. else it is DMA_QUEUED, and only then does
 * *handle point at the request's handle */
KMSTATUS kmiDMAtoTARequest(KMDWORD target, const KMDWORD *source,
                           KMDWORD length, KMDWORD flags, KMDWORD **handle);
/* TRUE unless a transfer with handle is waiting or running */
KMBOOLEAN kmiQueryFinishDMA(KMDWORD handle);
/* wait until no transfer with handle is waiting or running. the result is
 * TRUE */
KMBOOLEAN kmiWaitForFinishDMA(KMDWORD handle);
/* stop channel 2's transfer, reset the tile accelerator and empty both queues.
 * the result is 0. kmiDMAHazardReset also sets the queue of the request started
 * last to 7, past both queues, a shipped bug. until a queued transfer starts,
 * kmiDMAtoTARequest then reads a word past them, and kmiExecuteDMAChain clears
 * that word's low two bits */
KMINT32 kmiDMAHazardReset(void);
/* mark the request started last finished, and start the next waiting one, from
 * the first queue before the second. the result is TRUE when a request starts,
 * and else FALSE */
KMBOOLEAN kmiExecuteDMAChain(void);
/* set the video chip's timing up for PAL, NTSC or VGA output */
void kmiInitPAL(void);
void kmiInitNTSC(void);
void kmiInitVGA(void);
/* compute kmiDeviceExtension's frame buffer fields for a width by height
 * display at bpp, writing no register. kmiFBControl records the frame buffer
 * register values for display_mode, field_offset, and the frame's size in
 * pixels and in tiles. skip_lines becomes 0 and level_adjust
 * KM_LEVEL_ADJUST_NORMAL. interlace sends the frame as two fields. with
 * anti_alias, the frame renders twice as wide for the scaler to halve, and a
 * height of 240 becomes 480. anti_alias also sets OPTION_BY_COLUMNS in
 * ta_options, which kmiGetPackOBPParam passes on to kmiSetTAEnvironment. the
 * result is 0 */
KMINT32 kmiFBControl(KMINT32 width, KMINT32 height, KMBPPMODE bpp,
                     KMINT32 interlace, KMBOOLEAN dither, KMBOOLEAN anti_alias);
/* the background plane's packet writers, indexed by vertex type. each takes
 * three vertices */
extern KMSTATUS (*kmiBackGroundPlaneFuncs[18])(void *pVertex[3]);
/* the bits of Kamui's configuration flag word, one for each request recorded in
 * kmiSystemConfig by the calls that kmSetSystemConfiguration replaces.
 * KMI_CONFIG_VERTEX_BUFFER is kmCreateVertexBuffer's or kmCreateTABuffer's
 * request, KMI_CONFIG_FRAME_BUFFER kmCreateFrameBufferSurface's and
 * KMI_CONFIG_ACTIVATE kmActivateFrameBuffer's. KMI_CONFIG_ALL is all three. the
 * word becomes KMI_CONFIGURED when the configuration is made */
#define KMI_CONFIG_VERTEX_BUFFER 0x100
#define KMI_CONFIG_FRAME_BUFFER 0x200
#define KMI_CONFIG_ACTIVATE 0x800
#define KMI_CONFIG_ALL 0xb00
#define KMI_CONFIGURED 0x80000000
/* kmiAddSysConfigFlag adds flag to Kamui's configuration flag word, and
 * kmiSetSysConfigFlag sets the word to flag. each returns the word, as
 * kmiGetSysConfigFlag does */
KMDWORD kmiAddSysConfigFlag(KMDWORD flag);
KMDWORD kmiSetSysConfigFlag(KMDWORD flag);
KMDWORD kmiGetSysConfigFlag(void);

/* a frame buffer surface asked for, by kmCreateFrameBufferSurface or
 * kmSetSystemConfiguration */
struct kmi_frame_request {
    PKMSURFACEDESC surface;
    KMINT32 width;
    KMINT32 height;
    KMBOOLEAN clear;
};

/* kmi_system_config.buffer_mode's values, one for each call that asks for
 * vertex buffers */
#define KMI_BUFFERS_MADE 0 /* kmSetSystemConfiguration, which makes them */
#define KMI_BUFFERS_VERTEX 1 /* kmCreateVertexBuffer */
#define KMI_BUFFERS_TA 2 /* kmCreateTABuffer */

/* Kamui's system configuration, as the calls that make it record it */
struct kmi_system_config {
    PKMVERTEXBUFFDESC buffer;
    /* each list's vertex buffer words */
    KMDWORD opaque;
    KMDWORD opaque_modifier;
    KMDWORD translucent;
    KMDWORD translucent_modifier;
    KMDWORD punchthrough;
    KMBOOLEAN strip_buffer;
    struct kmi_frame_request request[2];
    PKMSURFACEDESC front;
    PKMSURFACEDESC back;
    KMBOOLEAN wait_vsync;
    KMINT32 width;
    KMINT32 height;
    KMINT32 bytes_per_pixel;
    /* which call last asked for vertex buffers, a KMI_BUFFERS_ value. while it
     * isn't KMI_BUFFERS_MADE, each frame buffer request computes the display
     * size again */
    KMINT32 buffer_mode;
    /* the bytes of each of the tile accelerator's two buffers, or
     * kmCreateTABuffer's TABuffer while its request waits */
    KMDWORD ta_buffer_size;
    KMDWORD texture_size;
    KMDWORD frame_buffer_size;
    /* the bytes the region array and the object pointer blocks take */
    KMDWORD region_opb_size;
};
extern struct kmi_system_config kmiSystemConfig;
/* make the configuration that kmiSystemConfig records. the configuration flag
 * word must be KMI_CONFIGURED. buffer_mode is kmiSystemConfig.buffer_mode.
 * KMI_BUFFERS_VERTEX makes kmCreateVertexBuffer's buffers, and KMI_BUFFERS_TA
 * makes kmCreateTABuffer's. KMI_BUFFERS_MADE makes no buffers, since
 * kmSetSystemConfiguration has made them. kmiAllocSysConfig then makes the
 * frame buffers, only one when both requests name the same surface, and
 * activates them.
 *
 * kmiAllocSysConfig last resets the tile accelerator and sets it up for the
 * write slot's bank. it does so only under KM_LATENCYMODEL_2V, or with the
 * lists going straight to the tile accelerator. ta_slot then names the write
 * slot, and flip_pending is set. the result is the status of the first call
 * that fails, and else KMSTATUS_SUCCESS */
KMSTATUS kmiAllocSysConfig(KMDWORD buffer_mode);
/* compute the display width and height into kmiSystemConfig, and the bytes a
 * frame takes at bpp. bpp is a KMBPPMODE, or DISPLAY_BPP for the display's own.
 * a strip buffer's height is rounded up to 32 lines, and its strip size also
 * goes into the frame buffer read control */
void kmiGetDisplayData(KMDWORD bpp, KMDWORD strip_buffer, KMDWORD height);
#define DISPLAY_BPP 0xff
/* allocate a frame buffer of size bytes, rounded up to a multiple of 32, for
 * surface. unless a second buffer of that size waits, kmiFBalloc allocates two
 * in one block, interleaved a long at a time. surface gets the first, and the
 * second waits. a call of that size then takes the waiting second, which
 * allocates nothing. on a chip with KMI_CAP_BALANCE_BANKS, the block comes from
 * the bank with more free bytes, or bank 0 on a tie. other chips take it from
 * bank 0. pSurface gets the buffer's offset in the 32-bit view, which
 * kmiFBfreeEx takes and kmiFBfree does not, and dwFrameBufferSize the rounded
 * size. the result is KMSTATUS_NOT_ENOUGH_MEMORY when no free block is large
 * enough or no block record is left, and else KMSTATUS_SUCCESS */
KMSTATUS kmiFBalloc(KMINT32 size, KMSURFACEDESC *surface);
/* allocate size bytes of texture memory for surface, and store their offset in
 * its pSurface. kmiTXalloc takes them from the bank whose largest free or idle
 * block is larger, and from bank 0 on a tie. on a chip without
 * KMI_CAP_BALANCE_BANKS, it always takes them from bank 1. kmiABSalloc
 * allocates at the offset pSurface already holds. the result is
 * KMSTATUS_NOT_ENOUGH_MEMORY when no free block holds the texture or no block
 * record is left, and else KMSTATUS_SUCCESS. surface must stay valid until it
 * is freed, since kmiFBGarbageCollect updates its pSurface */
KMSTATUS kmiTXalloc(KMINT32 size, KMSURFACEDESC *surface);
KMSTATUS kmiABSalloc(KMINT32 size, KMSURFACEDESC *surface);
/* give a small VQ texture a slot in a page of its shape, and store the slot's
 * offset in its pSurface. USize must be 8, 16, 32 or 64. kmiVQalloc takes a
 * free slot in a page of the shape when one has it. otherwise it allocates a
 * new page, from the bank kmiTXalloc would choose. surface must stay valid
 * until it is freed, since kmiFBGarbageCollect updates its pSurface.
 *
 * the result is KMSTATUS_NOT_ENOUGH_MEMORY when no page record is left, or when
 * no free block holds a new page, and else KMSTATUS_SUCCESS. a failure of the
 * second kind returns the new page's record to the pool. the record stays on
 * the list of pages, with its shape and no block. a later kmiVQalloc can then
 * give a texture a slot in that page, at an offset no block holds for it. the
 * pool can also hand the record out again, and its second push onto the list
 * joins the list into a loop. kmiFBreset and kmiFBgetfreemem then walk the loop
 * forever, and so do kmiVQalloc and kmiVQFBfree whenever they find nothing on
 * it */
KMSTATUS kmiVQalloc(KMSURFACEDESC *surface);
/* free the block allocated at offset, or return KMSTATUS_INVALID_ADDRESS when
 * none starts there. a paired block's twin is freed with it, unless the twin is
 * live; then the block only goes idle */
KMSTATUS kmiFBfree(KMINT32 offset);
/* free a small VQ texture's slot, and its page once the page is empty, or
 * return KMSTATUS_INVALID_ADDRESS when no slot is at the surface's pSurface.
 * requires a USize of 8, 16, 32 or 64 */
KMSTATUS kmiVQFBfree(KMSURFACEDESC *surface);
/* store in *bank0 and *bank1 the free bytes in each bank of video memory, idle
 * blocks included. store in *vq the free bytes in the VQ pages' slots, and in
 * *largest the bytes of the largest free or idle block. the result is
 * KMSTATUS_SUCCESS */
KMSTATUS kmiFBgetfreemem(KMINT32 *bank0, KMINT32 *bank1, KMINT32 *vq,
                         KMINT32 *largest);
/* sort every video memory list by offset */
KMSTATUS kmiFBSortQueue(void);
/* move each bank's unpaired textures and VQ pages down over the free space
 * between them, and update the pSurface of each surface moved. every video
 * memory list must be sorted by offset, as kmiFBSortQueue leaves them. on an
 * unsorted list, kmiFBGarbageCollect can loop forever. a free block whose
 * offset isn't a multiple of 32 is first split in two, which takes a block
 * record from a pool of 4096. the result is KMSTATUS_NOT_ENOUGH_MEMORY, with
 * the compaction stopped, when the pool is empty, and else KMSTATUS_SUCCESS */
KMSTATUS kmiFBGarbageCollect(void);
/* write the operand cache line that holds addr back to memory if it is
 * dirty. the line stays valid */
void kmiPurgeCache(void *addr);
/* run the callback at event, a _CALLBACK index, with that slot's argument */
void kmiDefaultInterruptHandler(KMINT32 event);
/* call function with argument, with FPSCR's SZ bit clear; FPSCR and FR0 to
 * FR15 are kept for the caller. function must return with FPSCR's SZ and FR
 * bits as it got them */
void kmiExCallBackFunc(PKMCALLBACKFUNC function, void *argument);
/* install handler for interrupt level 6 with interrupts masked, clear the
 * pending interrupts, and enable the video chip's interrupts on level 6 */
void kmiRegisterPhIntHandler(KMINT32 (*handler)(void));
/* write a region array of width by height tiles for bank. width and height must
 * be kmiDeviceExtension's tiles_x and tiles_y. a tile's entry takes 24 bytes:
 * its control word, then where each list's object pointer blocks for the tile
 * start. on a chip with KMI_CAP_TWO_ENTRIES, each tile takes two entries, or
 * four with dummy_translucent set. the array starts index * 24 * width * height
 * bytes past the bank's region_base. order must be BY_COLUMNS or BY_ROWS. with
 * four entries a tile, the second draws the dummy translucent object. that
 * object is written 4 * 24 * width * height bytes past the region base.
 * kmiCreateRegionArray first sets the tile accelerator up for bank, unless
 * ta_busy is set. unused is not read */
void kmiCreateRegionArray(KMDWORD width, KMDWORD height, KMDWORD index,
                          KMINT32 unused, KMINT32 bank, KMDWORD order);
/* kmiCreateRegionArray's orders, which list the tiles by columns or by rows */
#define BY_COLUMNS 0
#define BY_ROWS 1

/* what Kamui records of its frame slots. times are in ticks of the CPU's timer
 * 0, which counts down. kmistat.h declares the one instance,
 * kmiIntStatisticData */
struct kmi_interrupt_statistics {
    /* render_wait counts the checks that have found the running render
     * unfinished, against render_timeout. ta_wait counts those that have found
     * the running transfer to the tile accelerator unfinished, against
     * vertex_timeout */
    KMDWORD render_wait;
    KMDWORD ta_wait;
    /* renders and transfers stopped unfinished */
    KMDWORD render_timeouts;
    KMDWORD ta_timeouts;
    /* ol_overflow_wait and isp_overflow_wait count the object list and ISP
     * parameter overflows met while opb_downward is 1. each starts over when
     * the handler finds room in a bank to grow its area */
    KMDWORD ol_overflow_wait;
    KMDWORD isp_overflow_wait;
    /* the checks that have found a render into a texture unfinished */
    KMDWORD hold_wait;
    /* object list and ISP parameter overflows */
    KMDWORD ol_overflows;
    KMDWORD isp_overflows;
    /* each bank's TA_ISP_CURRENT when the tile accelerator last finished it */
    KMDWORD isp_current[2];
    /* each list's largest size, by ListType */
    KMDWORD list_max[5];
    /* render_start holds the timer at the running render's start, and
     * render_time the last finished render's ticks. ta_start and ta_time hold
     * the same for the transfer to the tile accelerator */
    KMDWORD render_start;
    KMDWORD render_time;
    KMDWORD ta_start;
    KMDWORD ta_time;
};
extern struct kmi_version_info kmiVersionInfo;
KMSTATUS kmiInitTimer(void);
/* select kmiDeviceExtension as the device record, clearing it, when no record
 * is selected. then reset the record's driver state, taking settings from the
 * fixed profile */
void kmiInitializeDriver(void);
/* unload the selected device and clear its handle */
void kmiReleaseHardware(void);
/* run the FATAL_ERROR_CALLBACK with code as its argument in place of its own;
 * that callback must hold a handler */
void kmiFatalErrorEntry(KMDWORD code);
/* kmiFatalErrorEntry's codes */
/* a vertical blank found the DMA queue full as it handed a slot's lists to the
 * tile accelerator */
#define KMI_FATAL_DMA_QUEUE_FULL 1
/* kmRender or kmRenderTexture found a list past its list_limit */
#define KMI_FATAL_LIST_LIMIT 2
/* an ISP parameter or object list overflow found a bank with no room to grow */
#define KMI_FATAL_NO_ROOM 3
/* kmFlipFrameBuffer's wait for the flip ran out */
#define KMI_FATAL_FLIP_TIMEOUT 4
/* busy wait for seconds * 2000 loop passes, truncated to an integer. seconds
 * must be nonnegative, and small enough that the count fits a KMINT32 */
void kmiWaitAccurate(KMFLOAT seconds);
KMINT32 kmiVerifyWriteRegister(KMDWORD reg, KMDWORD data);

void kmiInitRenderer(void);
KMINT32 kmiInitializeDevice(void);
void kmiInitializeFog(void);
void kmiUnloadDevice(void);
/* install kmiInterruptHandlerSH and clear the pending callbacks, the render
 * into a texture, and kmiIntStatisticData's counts and list maxima */
void kmiInitInterrupt(void);
/* the handler kmiInitInterrupt installs: act on the video chip's pending
 * interrupts and errors, clearing each it acts on, then run the callbacks they
 * leave pending. the result is 0 */
KMINT32 kmiInterruptHandlerSH(void);
void kmiModuleInitialize(void);
KMSTATUS kmiInitializeBackGroundPlane(void);
/* empty video memory. kmiFBinit returns every block and VQ page to its pool,
 * and sets the texture control register to 0. it then makes each bank of
 * texture memory one free block. the banks are two of 4 MB, or one of 8 MB when
 * KM_TEXTURE_ONE_BANK is 1. they lie in the 64-bit view, bank 1 from where bank
 * 0 ends. the profile's EnableTexture16MB of 1 would double them, but the fixed
 * profile gives 0 */
KMSTATUS kmiFBinit(void);
/* reset the tile accelerator and call kmiInitializeDMA. then start ta_bank[0]
 * at 0x400000 and ta_bank[1] at 0, and set opb_downward to 1. kmiInitializeTA
 * also sets three fields of kmiDeviceExtension that no other function reads:
 * unk_440 to 0x800000, and unk_448 and unk_44c to 0x600000 */
void kmiInitializeTA(void);
/* reset the tile accelerator */
void kmiResetTA(void);
void kmiInitializeHardware(void);
/* lay both banks of the tile accelerator out by options, and keep options in
 * ta_options. OPTION_BY_COLUMNS in options lists the tiles by columns rather
 * than rows, and OPB_DOWNWARD sets opb_downward. each list's object pointer
 * block size code takes the two bits from OPB_SHIFT(list), list a ListType.
 * each bank takes ta_buffer_size bytes from its start. kmiSetTAEnvironment then
 * writes bank 0's region array, and then bank 1's. each write sets the tile
 * accelerator up for its bank unless ta_busy is set. an idle tile accelerator
 * thus ends up set for bank 1. region_array, object_list, isp_params,
 * tsp_params and unk are not read */
void kmiSetTAEnvironment(KMDWORD region_array, KMDWORD object_list,
                         KMDWORD isp_params, KMDWORD tsp_params,
                         KMDWORD options, KMDWORD unk);
#define OPTION_BY_COLUMNS 1
#define OPB_DOWNWARD_SHIFT 16
#define OPB_DOWNWARD (1UL << OPB_DOWNWARD_SHIFT)
#define OPB_SHIFT(list) (17 + (2 * (list)))

/* the global parameter as written to a list; data holds the words the header's
 * type adds */
struct kmi_global_parameter {
    KMDWORD control;
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD data[12];
};
extern struct kmi_global_parameter kmiGlobalWork;

/* the vertex buffer member's zeroed globals. SHC, the compiler, lays such
 * globals out in the order of their first declarations, and an included
 * header's declarations come before the source's own. this header declares all
 * of them, even those no other source uses, so that they keep the order
 * LOADER.BIN and 1ST_READ.BIN hold them in */
extern KMCURRENTLISTSTATE kmiCurrentListState;
/* the current bank's list bases, by ListType */
extern PKMDWORD *kmiVertexBuff;
/* each list's write pointer, by ListType, and the address of the one a write
 * goes through */
extern PKMDWORD *kmiCurrentVertexBuff;
extern PKMDWORD *kmiDstVertexBuffer;
/* each bank's row of kmi_list_bases, which a descriptor's ppBuffer gets */
extern PKMDWORD *kmi_banks[2];
/* each bank's list bases, by ListType */
extern PKMDWORD kmi_list_bases[2][5];
/* the write pointers kmiCurrentVertexBuff and a descriptor's pCurrentPtr point
 * at */
extern PKMDWORD kmi_write_pointers[5];
/* where each list starts in the current bank */
extern PKMDWORD kmi_list_starts[5];
/* the current list's ListType, which building the global parameter sets and
 * copies into kmiCurrentListState */
extern KMDWORD kmiCurrentListType;
/* a polygon header that opens a list, with the list's ListType in its control
 * word */
struct list_header {
    KMDWORD control;
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    KMDWORD data[4];
};
#define LIST_HEADER_WORDS (sizeof(struct list_header) / sizeof(KMDWORD))
/* a polygon header and the triangle it opens, as kmiDummyOpaque holds them */
struct dummy_polygon {
    struct list_header header;
    KMVERTEX1 vertex[3];
};
/* make bank the current one, or bank 0 under KM_LATENCYMODEL_2V, and start
 * each list over past its header */
void kmiFlipVertexBuffer(KMDWORD bank);
/* a texture's width or height as its TSP size code, 0 for 8 to 7 for 1024. any
 * other size is returned as it is */
KMDWORD kmiConvertUVnum(KMDWORD size);
/* build the global parameter from the current polygon header */
void kmiCreateGlobalParameter(void);
/* copy kmiGlobalNum words of the global parameter into the current list,
 * move that list's write pointer past them, and return the pointer's address.
 * no source calls it. kmiGlobalNum stays 0, so a call would pass kmiMemCopy8
 * a Size of 0, below the 4 it requires. kmiMemCopy8's loop count would then
 * wrap from 0, and the copy run 2^32 rounds, not none */
PKMDWORD *kmiPutGlobalParam(void);

#endif
