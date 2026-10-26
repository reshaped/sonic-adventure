/* Ninja's state in Kamui's records: its vertex contexts, its vertex buffer and
 * its system setup */
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

#endif
