/* Ninja's global state */
/* clang-format off */
#ifndef __clang__
/* the eleven bss objects and _unit_matrix_, each on a 32-byte boundary. R10,
 * which compiles this file, aligns no object inside a section to 32 bytes */
/* @asm open: no inspect rule reads the bytes its .RES.B reserves */
#pragma asm
        .EXPORT     __nj_view_
        .EXPORT     __nj_view_matrix_
        .EXPORT     __nj_tmp_local_mtrx_
        .EXPORT     __nj_cnk_easy_multilight_buffer_
        .EXPORT     __nj_cnk_easy_multilight_buffer_dst_
        .EXPORT     __nj_cnk_simple_multilight_buffer_
        .EXPORT     __nj_cnk_simple_multilight_buffer_dst_
        .EXPORT     __nj_light_material_
        .EXPORT     __nj_light_control_
        .EXPORT     __nj_light_queue_
        .EXPORT     __nj_camera_matrix_
        .EXPORT     __unit_matrix_
        .SECTION    BSG32,DATA,ALIGN=32
        .ALIGN      32
__nj_view_:
        .RES.B      4
        .ALIGN      32
__nj_view_matrix_:
        .RES.B      64
        .ALIGN      32
__nj_tmp_local_mtrx_:
        .RES.B      64
        .ALIGN      32
__nj_cnk_easy_multilight_buffer_:
        .RES.B      64
        .ALIGN      32
__nj_cnk_easy_multilight_buffer_dst_:
        .RES.B      128
        .ALIGN      32
__nj_cnk_simple_multilight_buffer_:
        .RES.B      64
        .ALIGN      32
__nj_cnk_simple_multilight_buffer_dst_:
        .RES.B      128
        .ALIGN      32
__nj_light_material_:
        .RES.B      52
        .ALIGN      32
__nj_light_control_:
        .RES.B      132
        .ALIGN      32
__nj_light_queue_:
        .RES.B      132
        .ALIGN      32
__nj_camera_matrix_:
        .RES.B      64
        .SECTION    DSG32,DATA,ALIGN=32
        .ALIGN      32
__unit_matrix_:
        .FDATA.S    F'1.0, F'0.0, F'0.0, F'0.0
        .FDATA.S    F'0.0, F'1.0, F'0.0, F'0.0
        .FDATA.S    F'0.0, F'0.0, F'1.0, F'0.0
        .FDATA.S    F'0.0, F'0.0, F'0.0, F'1.0
        .SECTION    PSG,CODE,ALIGN=4
        ;
#pragma endasm
#endif
/* clang-format on */

#include <kamui.h>
#include <sg_xpt.h>
/* Ninja's types, without ninja.h's declarations: R10 lays out bss in the order
 * each name is first declared, and ninja.h declares the names below in another
 * order than their definitions */
#include <njtypes.h>

Uint32 _nj_texture_memory_size = 0x00100000;
Uint32 _nj_vertex_buffer_op_ = 0;
Uint32 _nj_vertex_buffer_om_ = 0;
Uint32 _nj_vertex_buffer_tp_ = 0;
Uint32 _nj_vertex_buffer_tm_ = 0;
Uint32 _nj_vertex_buffer_pt_ = 0;
Int _nj_flip_on_ = 1;
void (*_nj_eor_function_)(void) = NULL;
NJS_TEXLIST *_nj_current_texlist = NULL;
NJS_TEXMEMLIST *_nj_texmemlist = NULL;
Uint32 _nj_texmemsize = 0;
Uint32 _nj_current_texture = 0xffffffff;
Uint32 _nj_current_texture_bank = 0xffffffff;
Uint32 _nj_user_globalIndex = 0xffffffef;
Sint32 _nj_texglobal_color_ = 0;
Int _nj_texload_wait_ = 1;
Uint32 _nj_vertex_buffer_size_ = 0;
Uint32 *_nj_usr_vertex_buffer_ptr_ = NULL;
Uint32 _nj_usr_vertex_buffer_size_ = 0;

NJS_SCREEN _nj_screen_;
NJS_MATRIX *_nj_current_matrix_ptr_;
NJS_MATRIX *_nj_base_matrix_ptr_;
NJS_POINT2 _nj_2d_clip_[2];
Float _nj_near_clip_;
Float _nj_far_clip_;
Float _nj_scr_aspect_x_;
Float _nj_scr_aspect_y_;
Int _nj_display_mode_;
Int _nj_frame_mode_;
Int _nj_waitvsync_count;
Uint32 _nj_control_3d_flag_;
Int _nj_frame_size_w;
Int _nj_frame_size_h;
NJS_VERTEX_BUF *_nj_vertex_buf_;
Int _nj_vertex_num_;
Int _nj_calc_vertex_;
Int _nj_calc_polygon_;
Int _nj_draw_polygon_;
Uint32 _nj_constant_attr_and_;
Uint32 _nj_constant_attr_or_;
NJS_ARGB _nj_constant_material_;
Uint32 _nj_vsync_count_;
Float _nj_depth_near_;
Float _nj_depth_far_;
Float _nj_depth_range_;
Float _nj_vertex_offset_;
Int _nj_depth_enable_;
volatile Int _nj_fatal_err_callback_;
Sint32 _max_stack_size_;
Sint32 _stack_size_;
Sint32 _nj_tex_count;
char _nj_current_texture_path[256];
Uint32 _nj_tex_max_free_size;
NJS_TEXERRSTAT _nj_texerr_;
volatile Int _nj_eor_flag_;
volatile Int _nj_eov_flag_;
volatile Int _nj_render_flag_;
NJS_TEXSURFACE _nj_framebuffer_surface0_;
NJS_TEXSURFACE _nj_framebuffer_surface1_;
KMVERTEXCONTEXT _nj_context_[NJD_CONTEXT_MAX];
KMVERTEXCONTEXT *_nj_current_ctx_;
Uint32 _nj_ctx_color_mode_[NJD_CONTEXT_MAX];
Uint32 *_nj_color_mode_;
KMVERTEXBUFFDESC _nj_vertex_buffer_;
KMVERTEXCONTEXT _nj_background_ctx_;
KMSYSTEMCONFIGSTRUCT _nj_SystemConfigStruct_;
PKMSURFACEDESC _nj_ppSurfaceArray[2];
Uint32 _nj_wait_vsync_in_;
Uint32 _nj_wait_vsync_out_;
void (*_nj_motion_data_func_)(NJS_OBJECT *obj);
void (*_nj_shape_data_func_)(NJS_OBJECT *obj);
void (*_nj_motion_translate_)(NJS_OBJECT *obj);
void (*_nj_motion_rotate_)(NJS_OBJECT *obj);
void (*_nj_motion_scale_)(NJS_OBJECT *obj);
void (*_nj_motion_rotate_zxy_)(NJS_OBJECT *obj);
void (*_nj_motion_shape_)(NJS_OBJECT *obj);
struct nj_motion_info _nj_motion_data_info_;
struct nj_motion_info _nj_shape_data_info_;
Uint16 _nj_mot_data_cnt_;
Uint16 _nj_mot_data_type_;
Uint16 _nj_mot_shape_data_cnt_;
Uint16 _nj_mot_shape_data_type_;
void (*_nj_motlink_data_func_[2])(NJS_OBJECT *object);
void (*_nj_shapelink_data_func_[2])(NJS_OBJECT *object);
void (*_nj_rotate_func_)(NJS_MATRIX *m, Angle angx, Angle angy, Angle angz);
void (*_nj_int_f_func_)(Float *values, Float *out, Float rate);
void (*_nj_int_a_func_)(Angle *values, Angle *out, Float rate);
void (*_nj_motion_translate_data_[2])(NJS_OBJECT *object);
void (*_nj_motion_rotate_data_[2])(NJS_OBJECT *object);
void (*_nj_motion_scale_data_[2])(NJS_OBJECT *object);
void (*_nj_motion_shape_data_[2])(NJS_OBJECT *object);
struct nj_motlink_data_info _nj_motlink_data_info_[2];
struct nj_motlink_data_info _nj_shapelink_data_info_[2];
Uint16 _nj_motlink_data_cnt_[2];
Uint16 _nj_motlink_data_type_[2];
Uint16 _nj_motlink_shape_data_cnt_[2];
Uint16 _nj_motlink_shape_data_type_[2];
Uint16 _nj_mot_current_data_;
Float _nj_mot_current_frame_;
Uint16 _nj_pos_int_flag_;
Uint16 _nj_ang_int_flag_;
Uint16 _nj_scl_int_flag_;
Uint16 _nj_pnt_int_flag_;
Uint16 _nj_nml_int_flag_;
Float _nj_motion_link_pos_[12];
Float _nj_motion_link_scl_[12];
Angle _nj_motion_link_ang_[12];
NJS_POINT3 *_nj_shape_link_point_[4];
NJS_VECTOR *_nj_shape_link_normal_[4];
void (*_nj_motion_int_f_func_)(NJS_MKEY_F *keys, Uint32 index, Uint32 nb,
                               Float *out);
void (*_nj_motion_int_a_func_)(NJS_MKEY_A *keys, Uint32 index, Uint32 nb,
                               Angle *out);
void (*_nj_draw_model_sub_)(NJS_MODEL *model);
Uint16 _nj_mot_spline_cnt_;
Uint16 _nj_mot_shape_spline_cnt_;
Float *_nj_shape_buf_;
Float *_nj_mot_pos_buf_ptr_;
Float *_nj_mot_scl_buf_ptr_;
Angle *_nj_mot_ang_buf_ptr_;
NJS_TEXSURFACE _nj_tex_err_surface;
Sint32 _nj_tex_ext_;
Sint8 *_nj_texture_buffer_st;
Uint32 _nj_texture_buffer_max_size;
Sint32 _nj_palette_bpp_;
Sint32 _nj_palette_mode_;
Int _nj_matrix_flag_;
void (*_nj_motion_callback_)(NJS_OBJECT *obj);
#ifdef GCC
/* port.h's pivot and cover for the 2D draws' squeeze, WIDE_MIDDLE and FALSE
 * at boot */
Float port_wide_anchor;
Int port_wide_cover;
#endif
