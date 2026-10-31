/* Ninja's state in Kamui's records: its vertex contexts, its vertex buffer and
 * its system setup. Ninja's own copy of a context's header words and the
 * buffer's write pointers */
#ifndef NJKAMUI_H
#define NJKAMUI_H
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

/* the vertex contexts and each one's color mode. njSetCurrentContext(n) points
 * _nj_current_ctx_ at _nj_context_[n] and _nj_color_mode_ at
 * _nj_ctx_color_mode_[n] */
extern KMVERTEXCONTEXT _nj_context_[NJD_CONTEXT_MAX];
extern Uint32 _nj_ctx_color_mode_[NJD_CONTEXT_MAX];
extern Uint32 *_nj_color_mode_;
/* the background plane's context */
extern KMVERTEXCONTEXT _nj_background_ctx_;
/* the vertex buffer's descriptor, which the system setup's pBufferDesc points
 * at */
extern KMVERTEXBUFFDESC _nj_vertex_buffer_;
/* the system setup njInitSystem fills, which njInitVertexBuffer and
 * njChangeTextureMemorySize give Kamui. its ppSurfaceDescArray points at
 * _nj_ppSurfaceArray, the two frame buffers' surfaces */
extern KMSYSTEMCONFIGSTRUCT _nj_SystemConfigStruct_;
extern PKMSURFACEDESC _nj_ppSurfaceArray[2];
/* the polygon header words the njStart functions write, and each list's write
 * pointer in Kamui's vertex buffer. njGetkmPointer copies them from the
 * current vertex context and the vertex buffer, and njSetkmPointer copies
 * them back */
struct nj_parameterkm {
    Uint32 GLOBALPARAMBUFFER;
    Uint32 ISPPARAMBUFFER;
    Uint32 TSPPARAMBUFFER;
    Uint32 TexturePARAMBUFFER;
    Uint32 *opaque_polygon;
    Uint32 *trans_polygon;
    Uint32 *punchthrough_polygon;
    Uint32 *opaque_modifier;
    Uint32 *trans_modifier;
};
extern struct nj_parameterkm _nj_parameterkm_;

#endif
