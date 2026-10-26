/* the 3D draws' settings: the vertex buffer, the control flags, the constant
 * attributes and material, and the depth queue */
#include <ninja.h>
#include <sg_xpt.h>

void njInit3D(NJS_VERTEX_BUF *vbuf, Int vn)
{
    _nj_vertex_buf_ = vbuf;
    _nj_vertex_num_ = vn;
    _nj_control_3d_flag_ = 0;
    _nj_constant_attr_and_ = 0xffffffffUL;
    _nj_constant_attr_or_ = 0;
    _nj_constant_material_.a = 255.0F;
    _nj_constant_material_.r = 0.0F;
    _nj_constant_material_.g = 0.0F;
    _nj_constant_material_.b = 0.0F;
}

void njControl3D(Uint32 flag)
{
    _nj_control_3d_flag_ = flag;
}

void njSetConstantAttr(Uint32 and_attr, Uint32 or_attr)
{
    _nj_constant_attr_and_ = and_attr;
    _nj_constant_attr_or_ = or_attr;
}

void njSetConstantMaterial(NJS_ARGB *mat)
{
    _nj_constant_material_.a = mat->a;
    _nj_constant_material_.r = mat->r;
    _nj_constant_material_.g = mat->g;
    _nj_constant_material_.b = mat->b;
}

void njSetDepthQueue(Float dnear, Float dfar)
{
    _nj_depth_near_ = -dnear;
    _nj_depth_far_ = -dfar;
    _nj_depth_range_ = 1.0F / (_nj_depth_far_ - _nj_depth_near_);
}
