/* the data sdk/ninja/njGlobal.c defines in assembly, which make pc can't
 * build: config/pc.mk's PC_ADDED adds this beside njGlobal.c's C. the bss
 * block keeps the assembly's layout, each name 32 bytes aligned in the
 * order it gives them, since code reaches past a name to the next: as
 * ninja.h says, njInitGlobal writes _nj_cnk_easy_multilight_buffer_dst_'s
 * 32 words through _nj_cnk_easy_multilight_buffer_. each name is the
 * block's address plus its offset, by the assembler's .set */
#include <ninja.h>
#include <sg_xpt.h>

#include "xbox/include/asmname.h"

/* the block's names and their offsets, as njGlobal.c's BSG32 lays them out:
 *   0 _nj_view_ (4), 32 _nj_view_matrix_ (64), 96 _nj_tmp_local_mtrx_ (64),
 *   160 _nj_cnk_easy_multilight_buffer_ (64), 224 its _dst_ (128),
 *   352 _nj_cnk_simple_multilight_buffer_ (64), 416 its _dst_ (128),
 *   544 _nj_light_material_ (52), 608 _nj_light_control_ (132),
 *   768 _nj_light_queue_ (132), 928 _nj_camera_matrix_ (64) */
__attribute__((aligned(32), used)) Uint8 nj_global_block[992];

__asm__(
    ".globl " ASM_NAME(_nj_view_) "\n\t.set " ASM_NAME(_nj_view_) ", " ASM_NAME(nj_global_block) "+0\n\t"
    ".globl " ASM_NAME(_nj_view_matrix_) "\n\t.set " ASM_NAME(_nj_view_matrix_) ", " ASM_NAME(nj_global_block) "+32\n\t"
    ".globl " ASM_NAME(_nj_tmp_local_mtrx_) "\n\t.set " ASM_NAME(_nj_tmp_local_mtrx_) ", " ASM_NAME(nj_global_block) "+96\n\t"
    ".globl " ASM_NAME(_nj_cnk_easy_multilight_buffer_) "\n\t.set " ASM_NAME(_nj_cnk_easy_multilight_buffer_) ", " ASM_NAME(nj_global_block) "+160\n\t"
    ".globl " ASM_NAME(_nj_cnk_easy_multilight_buffer_dst_) "\n\t.set " ASM_NAME(_nj_cnk_easy_multilight_buffer_dst_) ", " ASM_NAME(nj_global_block) "+224\n\t"
    ".globl " ASM_NAME(_nj_cnk_simple_multilight_buffer_) "\n\t.set " ASM_NAME(_nj_cnk_simple_multilight_buffer_) ", " ASM_NAME(nj_global_block) "+352\n\t"
    ".globl " ASM_NAME(_nj_cnk_simple_multilight_buffer_dst_) "\n\t.set " ASM_NAME(_nj_cnk_simple_multilight_buffer_dst_) ", " ASM_NAME(nj_global_block) "+416\n\t"
    ".globl " ASM_NAME(_nj_light_material_) "\n\t.set " ASM_NAME(_nj_light_material_) ", " ASM_NAME(nj_global_block) "+544\n\t"
    ".globl " ASM_NAME(_nj_light_control_) "\n\t.set " ASM_NAME(_nj_light_control_) ", " ASM_NAME(nj_global_block) "+608\n\t"
    ".globl " ASM_NAME(_nj_light_queue_) "\n\t.set " ASM_NAME(_nj_light_queue_) ", " ASM_NAME(nj_global_block) "+768\n\t"
    ".globl " ASM_NAME(_nj_camera_matrix_) "\n\t.set " ASM_NAME(_nj_camera_matrix_) ", " ASM_NAME(nj_global_block) "+928\n\t"
);

/* njGlobal.c's DSG32 */
__attribute__((aligned(32))) NJS_MATRIX _unit_matrix_ = {
    1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
    0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};

_Static_assert(sizeof(NJS_LIGHT_MATERIAL) <= 52, "_nj_light_material_ fits its room");
_Static_assert(sizeof(_nj_light_control_) <= 132, "_nj_light_control_ fits its room");
_Static_assert(sizeof(_nj_light_queue_) <= 132, "_nj_light_queue_ fits its room");
