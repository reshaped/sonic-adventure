#include <kamui.h>
#include <ninja.h>

void njFadeEnable(void)
{
    _nj_current_ctx_->RenderState = KM_FOGMODE;
    _nj_current_ctx_->FogMode = KM_FOGTABLE;
    kmProcessVertexRenderState(_nj_current_ctx_);
    kmSetVertexRenderState(_nj_current_ctx_);
}
