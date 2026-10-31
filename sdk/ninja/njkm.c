/* polygon headers written straight into Kamui's vertex buffer through the
 * store queues, and the setters of the header words they hold */
#include <kamui.h>
#include <machine.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "ninja/njkamui.h"

/* the store queue address control registers */
#define QACR0 (*(Uint32 *)0xff000038)
#define QACR1 (*(Uint32 *)0xff00003c)

/* a list's address, split for the store queues. SQ_AREA moves its bits 26 to
 * 28 into a QACR's area field. SQ_ADDRESS keeps its low 26 bits, which address
 * it in the store queue area at SQ_BASE. LIST_TOP keeps its top 6 bits, which
 * njEndVertex puts back */
#define SQ_AREA(ptr) (((ptr) >> 24) & 0x1c)
#define SQ_ADDRESS 0x03ffffff
#define SQ_BASE 0xe0000000
#define LIST_TOP 0xfc000000

/* fields of a polygon header's global, ISP and TSP words. a one-bit field is
 * set for what its name says */
#define GLOBAL_LIST 0x07000000 /* the list type */
/* GLOBAL_LIST's values for the punch-through and the translucent list */
#define GLOBAL_PUNCHTHROUGH 0x04000000
#define GLOBAL_TRANSLUCENT 0x02000000
#define GLOBAL_USER_CLIP 0x00030000
#define GLOBAL_SHADOW 0x00000080
#define GLOBAL_OFFSET 0x00000004 /* the offset color, for specular light */
/* the object control byte: the shadow, volume, color type, texture, offset,
 * Gouraud and 16-bit UV bits */
#define GLOBAL_OBJECT_CONTROL 0x000000ff
#define ISP_CULL 0x18000000
#define ISP_Z_WRITE_DISABLE 0x04000000
#define TSP_SRC 0xe0000000 /* the source blend factor */
#define TSP_DST 0x1c000000 /* the destination blend factor */
#define TSP_ALPHA 0x00100000
#define TSP_IGNORE_ALPHA 0x00080000
#define TSP_FLIP 0x00060000
#define TSP_CLAMP 0x00018000
/* the texture filter: TSP_BILINEAR alone, or TSP_TRILINEAR for either pass of
 * a trilinear filter */
#define TSP_TRILINEAR 0x00004000
#define TSP_BILINEAR 0x00002000
#define TSP_FILTER (TSP_TRILINEAR | TSP_BILINEAR)
#define TSP_SUPERSAMPLE 0x00001000
#define TSP_MIPMAP_D 0x00000f00 /* the mipmap D adjust */
#define TSP_SHADING 0x000000c0
#define TSP_UV_SIZE 0x0000003f /* the U and V size codes */
#define TSP_FACTORS (TSP_SRC | TSP_DST)

/* blend factors, which SRC puts in TSP_SRC and DST in TSP_DST */
#define BLEND_ZERO 0
#define BLEND_ONE 1
#define BLEND_SRC_ALPHA 4
#define BLEND_INV_SRC_ALPHA 5
#define SRC(factor) ((Uint32)(factor) << 29)
#define DST(factor) ((Uint32)(factor) << 26)

/* an opaque or punch-through header clears TSP_BLEND in the TSP word, then
 * ORs in TSP_OPAQUE or TSP_PUNCHTHROUGH. TSP_OPAQUE's TSP_IGNORE_ALPHA lies
 * outside TSP_BLEND, so an opaque header sets it but never clears it */
#define TSP_BLEND (TSP_FACTORS | TSP_ALPHA)
#define TSP_OPAQUE (SRC(BLEND_ONE) | DST(BLEND_ZERO) | TSP_IGNORE_ALPHA)
#define TSP_PUNCHTHROUGH (SRC(BLEND_SRC_ALPHA) | DST(BLEND_INV_SRC_ALPHA))

/* the global word of a modifier volume's header, which keeps the current
 * global word's GLOBAL_USER_CLIP bits. the header of the volume's last polygon
 * takes GLOBAL_MODIFIER_LAST, and ISP_MODIFIER_LAST in its ISP word */
#define GLOBAL_MODIFIER 0x81000000
#define GLOBAL_MODIFIER_LAST 0x81000040
#define ISP_MODIFIER_LAST 0x20000000

/* the texture word's address field, in 8-byte units. TEXTURE_ADDRESS_MAX is
 * video memory's 8 MB in those units */
#define TEXTURE_ADDRESS_MASK 0x001fffff
#define TEXTURE_ADDRESS_MAX 0x00100000

/* a color mode's bits in the global word, and in the TSP word */
#define MODE_GLOBAL (GLOBAL_TRANSLUCENT | GLOBAL_OBJECT_CONTROL)
#define MODE_TSP (TSP_ALPHA | TSP_IGNORE_ALPHA)
/* the bits that make a color mode translucent */
#define MODE_TRANS (GLOBAL_TRANSLUCENT | TSP_ALPHA)

/* the TSP bits a material's flags hold, which njSetkmModelAttr copies */
#define MATERIAL_TSP                                                           \
    (TSP_FACTORS | TSP_FLIP | TSP_CLAMP | TSP_FILTER | TSP_SUPERSAMPLE |       \
     TSP_MIPMAP_D)

struct nj_parameterkm _nj_parameterkm_;
/* the LIST_TOP bits of the list the last njStart function opened */
Uint32 _nj_sq_base_;
/* the address of the write pointer of the list the last njStart function
 * opened. njEndVertex sets the write pointer through it */
Uint32 **_nj_listtype_;

extern Sint32 _nj_texglobal_color_;

void njGetkmPointer(void)
{
    _nj_parameterkm_.GLOBALPARAMBUFFER = _nj_current_ctx_->GLOBALPARAMBUFFER;
    _nj_parameterkm_.ISPPARAMBUFFER = _nj_current_ctx_->ISPPARAMBUFFER;
    _nj_parameterkm_.TSPPARAMBUFFER = _nj_current_ctx_->TSPPARAMBUFFER;
    _nj_parameterkm_.TexturePARAMBUFFER = _nj_current_ctx_->TexturePARAMBUFFER;
    /* @hack spelling: (&_nj_vertex_buffer_)->pCurrentPtr, which loads the
     * address from the pool twice and copies it to another register before
     * each read (0x8c6588bc to 0x8c6588e0 in 1ST_READ.BIN); as
     * _nj_vertex_buffer_.pCurrentPtr, the function is 8 bytes shorter and 26
     * of 39 words differ */
    _nj_parameterkm_.opaque_polygon =
        (&_nj_vertex_buffer_)->pCurrentPtr[KM_OPAQUE_POLYGON];
    _nj_parameterkm_.trans_polygon =
        (&_nj_vertex_buffer_)->pCurrentPtr[KM_TRANS_POLYGON];
    _nj_parameterkm_.opaque_modifier =
        (&_nj_vertex_buffer_)->pCurrentPtr[KM_OPAQUE_MODIFIER];
    _nj_parameterkm_.trans_modifier =
        (&_nj_vertex_buffer_)->pCurrentPtr[KM_TRANS_MODIFIER];
    _nj_parameterkm_.punchthrough_polygon =
        (&_nj_vertex_buffer_)->pCurrentPtr[KM_PUNCHTHROUGH_POLYGON];
}

void njSetkmPointer(void)
{
    _nj_current_ctx_->GLOBALPARAMBUFFER = _nj_parameterkm_.GLOBALPARAMBUFFER;
    _nj_current_ctx_->ISPPARAMBUFFER = _nj_parameterkm_.ISPPARAMBUFFER;
    _nj_current_ctx_->TSPPARAMBUFFER = _nj_parameterkm_.TSPPARAMBUFFER;
    _nj_current_ctx_->TexturePARAMBUFFER = _nj_parameterkm_.TexturePARAMBUFFER;
    /* @hack spelling: (&_nj_vertex_buffer_)->pCurrentPtr, which loads the
     * address from the pool twice and copies it to another register before
     * each write (0x8c65890a to 0x8c65892e in 1ST_READ.BIN); as
     * _nj_vertex_buffer_.pCurrentPtr, the function is 8 bytes shorter and 25
     * of 39 words differ */
    (&_nj_vertex_buffer_)->pCurrentPtr[KM_OPAQUE_POLYGON] =
        _nj_parameterkm_.opaque_polygon;
    (&_nj_vertex_buffer_)->pCurrentPtr[KM_TRANS_POLYGON] =
        _nj_parameterkm_.trans_polygon;
    (&_nj_vertex_buffer_)->pCurrentPtr[KM_OPAQUE_MODIFIER] =
        _nj_parameterkm_.opaque_modifier;
    (&_nj_vertex_buffer_)->pCurrentPtr[KM_TRANS_MODIFIER] =
        _nj_parameterkm_.trans_modifier;
    (&_nj_vertex_buffer_)->pCurrentPtr[KM_PUNCHTHROUGH_POLYGON] =
        _nj_parameterkm_.punchthrough_polygon;
}

/* each njStart function below maps its list's write pointer into the store
 * queue area and writes a polygon header there, last word first. prefetch on
 * a store queue address sends that queue's 32 bytes to the list. the write
 * pointer itself moves only at njEndVertex */

void *njStartVertexNonTexOpaque(void)
{
    /* @hack redundant: register on sq, which puts &_nj_parameterkm_ in R6
     * and area in R5 (0x8c658934 and 0x8c65894a in 1ST_READ.BIN); without
     * it, the two trade and 10 of 40 words differ */
    register Uint32 *sq = _nj_parameterkm_.opaque_polygon;
    Uint32 area;

    _nj_sq_base_ = (Uint32)sq & LIST_TOP;
    area = SQ_AREA((Uint32)sq);
    QACR0 = area;
    QACR1 = area;
    sq = (Uint32 *)(((Uint32)sq & SQ_ADDRESS) | SQ_BASE);
    sq += 3;
    *--sq = (_nj_parameterkm_.TSPPARAMBUFFER & ~TSP_BLEND) | TSP_OPAQUE;
    *--sq = _nj_parameterkm_.ISPPARAMBUFFER;
    *--sq = _nj_parameterkm_.GLOBALPARAMBUFFER;
    prefetch(sq);
    sq += 8;
    _nj_listtype_ = &_nj_parameterkm_.opaque_polygon;
    return sq;
}

void *njStartVertexNonTexTrans(void)
{
    /* @hack redundant: register on sq, which puts &_nj_parameterkm_ in R6
     * and area in R5 (0x8c658984 and 0x8c65899a in 1ST_READ.BIN); without
     * it, the two trade and 10 of 36 words differ */
    register Uint32 *sq = _nj_parameterkm_.trans_polygon;
    Uint32 area;

    _nj_sq_base_ = (Uint32)sq & LIST_TOP;
    area = SQ_AREA((Uint32)sq);
    QACR0 = area;
    QACR1 = area;
    sq = (Uint32 *)(((Uint32)sq & SQ_ADDRESS) | SQ_BASE);
    sq += 3;
    *--sq = _nj_parameterkm_.TSPPARAMBUFFER;
    *--sq = _nj_parameterkm_.ISPPARAMBUFFER;
    *--sq = _nj_parameterkm_.GLOBALPARAMBUFFER;
    prefetch(sq);
    sq += 8;
    _nj_listtype_ = &_nj_parameterkm_.trans_polygon;
    return sq;
}

void *njStartVertexTexOpaque(void)
{
    /* @hack redundant: register on sq, which puts &_nj_parameterkm_ in R6
     * and area in R5 (0x8c658a00 and 0x8c658a16 in 1ST_READ.BIN); without
     * it, the two trade and 11 of 42 words differ */
    register Uint32 *sq = _nj_parameterkm_.opaque_polygon;
    Uint32 area;

    _nj_sq_base_ = (Uint32)sq & LIST_TOP;
    area = SQ_AREA((Uint32)sq);
    QACR0 = area;
    QACR1 = area;
    sq = (Uint32 *)(((Uint32)sq & SQ_ADDRESS) | SQ_BASE);
    sq += 4;
    *--sq = _nj_parameterkm_.TexturePARAMBUFFER;
    *--sq = (_nj_parameterkm_.TSPPARAMBUFFER & ~TSP_BLEND) | TSP_OPAQUE;
    *--sq = _nj_parameterkm_.ISPPARAMBUFFER;
    *--sq = _nj_parameterkm_.GLOBALPARAMBUFFER;
    prefetch(sq);
    sq += 8;
    _nj_listtype_ = &_nj_parameterkm_.opaque_polygon;
    return sq;
}

void *njStartVertexTexTrans(void)
{
    /* @hack redundant: register on sq, which puts &_nj_parameterkm_ in R6
     * and area in R5 (0x8c658a54 and 0x8c658a6a in 1ST_READ.BIN); without
     * it, the two trade and 11 of 38 words differ */
    register Uint32 *sq = _nj_parameterkm_.trans_polygon;
    Uint32 area;

    _nj_sq_base_ = (Uint32)sq & LIST_TOP;
    area = SQ_AREA((Uint32)sq);
    QACR0 = area;
    QACR1 = area;
    sq = (Uint32 *)(((Uint32)sq & SQ_ADDRESS) | SQ_BASE);
    sq += 4;
    *--sq = _nj_parameterkm_.TexturePARAMBUFFER;
    *--sq = _nj_parameterkm_.TSPPARAMBUFFER;
    *--sq = _nj_parameterkm_.ISPPARAMBUFFER;
    *--sq = _nj_parameterkm_.GLOBALPARAMBUFFER;
    prefetch(sq);
    sq += 8;
    _nj_listtype_ = &_nj_parameterkm_.trans_polygon;
    return sq;
}

void *njStartVertexTexPunch(void)
{
    /* @hack redundant: register on sq, which puts &_nj_parameterkm_ in R6
     * and area in R5 (0x8c658aa0 and 0x8c658ab6 in 1ST_READ.BIN); without
     * it, the two trade and 11 of 45 words differ */
    register Uint32 *sq = _nj_parameterkm_.punchthrough_polygon;
    Uint32 area;

    _nj_sq_base_ = (Uint32)sq & LIST_TOP;
    area = SQ_AREA((Uint32)sq);
    QACR0 = area;
    QACR1 = area;
    sq = (Uint32 *)(((Uint32)sq & SQ_ADDRESS) | SQ_BASE);
    sq += 4;
    *--sq = _nj_parameterkm_.TexturePARAMBUFFER;
    *--sq = (_nj_parameterkm_.TSPPARAMBUFFER & ~TSP_BLEND) | TSP_PUNCHTHROUGH;
    *--sq = _nj_parameterkm_.ISPPARAMBUFFER;
    *--sq = (_nj_parameterkm_.GLOBALPARAMBUFFER & ~GLOBAL_LIST) |
            GLOBAL_PUNCHTHROUGH;
    prefetch(sq);
    sq += 8;
    _nj_listtype_ = &_nj_parameterkm_.punchthrough_polygon;
    return sq;
}

/* an Intens function's header carries color's four words, the face color, in
 * its words 4 to 7. an IntensH function's header takes 64 bytes, with its
 * words 4 to 7 unwritten. color's four words fill words 8 to 11, and offset's
 * four, the offset color, fill words 12 to 15 */

void *njStartVertexTexPunchIntens(Uint32 *color)
{
    /* @hack redundant: register on sq, which puts &_nj_parameterkm_ in R7
     * and area in R6 (0x8c658afa and 0x8c658b12 in 1ST_READ.BIN); without
     * it, the two trade and 11 of 57 words differ */
    register Uint32 *sq = _nj_parameterkm_.punchthrough_polygon;
    Uint32 area;

    _nj_sq_base_ = (Uint32)sq & LIST_TOP;
    area = SQ_AREA((Uint32)sq);
    QACR0 = area;
    QACR1 = area;
    sq = (Uint32 *)(((Uint32)sq & SQ_ADDRESS) | SQ_BASE);
    color += 4;
    sq += 8;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = _nj_parameterkm_.TexturePARAMBUFFER;
    *--sq = (_nj_parameterkm_.TSPPARAMBUFFER & ~TSP_BLEND) | TSP_PUNCHTHROUGH;
    *--sq = _nj_parameterkm_.ISPPARAMBUFFER;
    *--sq = (_nj_parameterkm_.GLOBALPARAMBUFFER & ~GLOBAL_LIST) |
            GLOBAL_PUNCHTHROUGH;
    prefetch(sq);
    sq += 8;
    _nj_listtype_ = &_nj_parameterkm_.punchthrough_polygon;
    return sq;
}

void *njStartVertexTexPunchIntensH(Uint32 *color, Uint32 *offset)
{
    /* @hack redundant: register on sq, which puts &_nj_parameterkm_ in R14,
     * saved on the stack (MOV.L R14,@-R15 at 0x8c658b6c in 1ST_READ.BIN);
     * without it, &_nj_parameterkm_ takes R7, the function is 4 bytes shorter,
     * and 66 of 75 words differ */
    register Uint32 *sq = _nj_parameterkm_.punchthrough_polygon;
    Uint32 area;

    _nj_sq_base_ = (Uint32)sq & LIST_TOP;
    area = SQ_AREA((Uint32)sq);
    QACR0 = area;
    QACR1 = area;
    sq = (Uint32 *)(((Uint32)sq & SQ_ADDRESS) | SQ_BASE);
    color += 3;
    offset += 3;
    sq += 4;
    *--sq = _nj_parameterkm_.TexturePARAMBUFFER;
    *--sq = (_nj_parameterkm_.TSPPARAMBUFFER & ~TSP_BLEND) | TSP_PUNCHTHROUGH;
    *--sq = _nj_parameterkm_.ISPPARAMBUFFER;
    *--sq = (_nj_parameterkm_.GLOBALPARAMBUFFER & ~GLOBAL_LIST) |
            GLOBAL_PUNCHTHROUGH;
    prefetch(sq);
    sq += 16;
    *--sq = *offset;
    *--sq = *--offset;
    *--sq = *--offset;
    *--sq = *--offset;
    *--sq = *color;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = *--color;
    prefetch(sq);
    sq += 8;
    _nj_listtype_ = &_nj_parameterkm_.punchthrough_polygon;
    return sq;
}

void *njStartVertexNonTexOpaqueIntens(Uint32 *color)
{
    Uint32 *sq = _nj_parameterkm_.opaque_polygon;
    Uint32 area;

    _nj_sq_base_ = (Uint32)sq & LIST_TOP;
    area = SQ_AREA((Uint32)sq);
    QACR0 = area;
    QACR1 = area;
    sq = (Uint32 *)(((Uint32)sq & SQ_ADDRESS) | SQ_BASE);
    color += 4;
    sq += 8;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = *--color;
    /* an untextured header leaves its texture word unwritten */
    --sq;
    *--sq = (_nj_parameterkm_.TSPPARAMBUFFER & ~TSP_BLEND) | TSP_OPAQUE;
    *--sq = _nj_parameterkm_.ISPPARAMBUFFER;
    *--sq = _nj_parameterkm_.GLOBALPARAMBUFFER;
    prefetch(sq);
    sq += 8;
    _nj_listtype_ = &_nj_parameterkm_.opaque_polygon;
    return sq;
}

void *njStartVertexNonTexTransIntens(Uint32 *color)
{
    Uint32 *sq = _nj_parameterkm_.trans_polygon;
    Uint32 area;

    _nj_sq_base_ = (Uint32)sq & LIST_TOP;
    area = SQ_AREA((Uint32)sq);
    QACR0 = area;
    QACR1 = area;
    sq = (Uint32 *)(((Uint32)sq & SQ_ADDRESS) | SQ_BASE);
    color += 4;
    sq += 8;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = *--color;
    /* an untextured header leaves its texture word unwritten */
    --sq;
    *--sq = _nj_parameterkm_.TSPPARAMBUFFER;
    *--sq = _nj_parameterkm_.ISPPARAMBUFFER;
    *--sq = _nj_parameterkm_.GLOBALPARAMBUFFER;
    prefetch(sq);
    sq += 8;
    _nj_listtype_ = &_nj_parameterkm_.trans_polygon;
    return sq;
}

void *njStartVertexTexOpaqueIntens(Uint32 *color)
{
    /* @hack redundant: register on sq, which puts &_nj_parameterkm_ in R7
     * and area in R6 (0x8c658d02 and 0x8c658d1a in 1ST_READ.BIN); without
     * it, the two trade and 11 of 54 words differ */
    register Uint32 *sq = _nj_parameterkm_.opaque_polygon;
    Uint32 area;

    _nj_sq_base_ = (Uint32)sq & LIST_TOP;
    area = SQ_AREA((Uint32)sq);
    QACR0 = area;
    QACR1 = area;
    sq = (Uint32 *)(((Uint32)sq & SQ_ADDRESS) | SQ_BASE);
    color += 4;
    sq += 8;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = _nj_parameterkm_.TexturePARAMBUFFER;
    *--sq = (_nj_parameterkm_.TSPPARAMBUFFER & ~TSP_BLEND) | TSP_OPAQUE;
    *--sq = _nj_parameterkm_.ISPPARAMBUFFER;
    *--sq = _nj_parameterkm_.GLOBALPARAMBUFFER;
    prefetch(sq);
    sq += 8;
    _nj_listtype_ = &_nj_parameterkm_.opaque_polygon;
    return sq;
}

void *njStartVertexTexTransIntens(Uint32 *color)
{
    /* @hack redundant: register on sq, which puts &_nj_parameterkm_ in R7
     * and area in R6 (0x8c658d6e and 0x8c658d86 in 1ST_READ.BIN); without
     * it, the two trade and 11 of 50 words differ */
    register Uint32 *sq = _nj_parameterkm_.trans_polygon;
    Uint32 area;

    _nj_sq_base_ = (Uint32)sq & LIST_TOP;
    area = SQ_AREA((Uint32)sq);
    QACR0 = area;
    QACR1 = area;
    sq = (Uint32 *)(((Uint32)sq & SQ_ADDRESS) | SQ_BASE);
    color += 4;
    sq += 8;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = _nj_parameterkm_.TexturePARAMBUFFER;
    *--sq = _nj_parameterkm_.TSPPARAMBUFFER;
    *--sq = _nj_parameterkm_.ISPPARAMBUFFER;
    *--sq = _nj_parameterkm_.GLOBALPARAMBUFFER;
    prefetch(sq);
    sq += 8;
    _nj_listtype_ = &_nj_parameterkm_.trans_polygon;
    return sq;
}

void *njStartVertexTexOpaqueIntensH(Uint32 *color, Uint32 *offset)
{
    /* @hack redundant: register on sq, which puts &_nj_parameterkm_ in R14,
     * saved on the stack (MOV.L R14,@-R15 at 0x8c658dd2 in 1ST_READ.BIN);
     * without it, &_nj_parameterkm_ takes R7, the function is 4 bytes shorter,
     * and 58 of 71 words differ */
    register Uint32 *sq = _nj_parameterkm_.opaque_polygon;
    Uint32 area;

    _nj_sq_base_ = (Uint32)sq & LIST_TOP;
    area = SQ_AREA((Uint32)sq);
    QACR0 = area;
    QACR1 = area;
    sq = (Uint32 *)(((Uint32)sq & SQ_ADDRESS) | SQ_BASE);
    color += 3;
    offset += 3;
    sq += 4;
    *--sq = _nj_parameterkm_.TexturePARAMBUFFER;
    *--sq = (_nj_parameterkm_.TSPPARAMBUFFER & ~TSP_BLEND) | TSP_OPAQUE;
    *--sq = _nj_parameterkm_.ISPPARAMBUFFER;
    *--sq = _nj_parameterkm_.GLOBALPARAMBUFFER;
    prefetch(sq);
    sq += 16;
    *--sq = *offset;
    *--sq = *--offset;
    *--sq = *--offset;
    *--sq = *--offset;
    *--sq = *color;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = *--color;
    prefetch(sq);
    sq += 8;
    _nj_listtype_ = &_nj_parameterkm_.opaque_polygon;
    return sq;
}

void *njStartVertexTexTransIntensH(Uint32 *color, Uint32 *offset)
{
    /* @hack redundant: register on sq, which puts &_nj_parameterkm_ in R14,
     * saved on the stack (MOV.L R14,@-R15 at 0x8c658e60 in 1ST_READ.BIN);
     * without it, &_nj_parameterkm_ takes R7, the function is 4 bytes shorter,
     * and 59 of 67 words differ */
    register Uint32 *sq = _nj_parameterkm_.trans_polygon;
    Uint32 area;

    _nj_sq_base_ = (Uint32)sq & LIST_TOP;
    area = SQ_AREA((Uint32)sq);
    QACR0 = area;
    QACR1 = area;
    sq = (Uint32 *)(((Uint32)sq & SQ_ADDRESS) | SQ_BASE);
    color += 3;
    offset += 3;
    sq += 4;
    *--sq = _nj_parameterkm_.TexturePARAMBUFFER;
    *--sq = _nj_parameterkm_.TSPPARAMBUFFER;
    *--sq = _nj_parameterkm_.ISPPARAMBUFFER;
    *--sq = _nj_parameterkm_.GLOBALPARAMBUFFER;
    prefetch(sq);
    sq += 16;
    *--sq = *offset;
    *--sq = *--offset;
    *--sq = *--offset;
    *--sq = *--offset;
    *--sq = *color;
    *--sq = *--color;
    *--sq = *--color;
    *--sq = *--color;
    prefetch(sq);
    sq += 8;
    _nj_listtype_ = &_nj_parameterkm_.trans_polygon;
    return sq;
}

void *njStartModifier(void)
{
    Uint32 *sq = _nj_parameterkm_.opaque_modifier;
    Uint32 area;

    _nj_sq_base_ = (Uint32)sq & LIST_TOP;
    area = SQ_AREA((Uint32)sq);
    QACR0 = area;
    QACR1 = area;
    sq = (Uint32 *)(((Uint32)sq & SQ_ADDRESS) | SQ_BASE);
    sq += 2;
    *--sq = 0;
    *--sq = (_nj_parameterkm_.GLOBALPARAMBUFFER & GLOBAL_USER_CLIP) |
            GLOBAL_MODIFIER;
    prefetch(sq);
    sq += 8;
    _nj_listtype_ = &_nj_parameterkm_.opaque_modifier;
    return sq;
}

void *njStartTransModifier(void)
{
    Uint32 *sq = _nj_parameterkm_.trans_modifier;
    Uint32 area;

    _nj_sq_base_ = (Uint32)sq & LIST_TOP;
    area = SQ_AREA((Uint32)sq);
    QACR0 = area;
    QACR1 = area;
    sq = (Uint32 *)(((Uint32)sq & SQ_ADDRESS) | SQ_BASE);
    sq += 2;
    *--sq = 0;
    *--sq = (_nj_parameterkm_.GLOBALPARAMBUFFER & GLOBAL_USER_CLIP) |
            GLOBAL_MODIFIER;
    prefetch(sq);
    sq += 8;
    _nj_listtype_ = &_nj_parameterkm_.trans_modifier;
    return sq;
}

/* write the header of a modifier volume's last polygon at sq, a store queue
 * address, and send it. it returns the address past the header's 32 bytes */
void *njEndModifier(Uint32 *sq)
{
    sq += 2;
    *--sq = ISP_MODIFIER_LAST;
    *--sq = (_nj_parameterkm_.GLOBALPARAMBUFFER & GLOBAL_USER_CLIP) |
            GLOBAL_MODIFIER_LAST;
    prefetch(sq);
    sq += 8;
    return sq;
}

/* vertex is the store queue address past the vertices written from an njStart
 * function's result */
/* NOLINTNEXTLINE(readability-non-const-parameter) */
void njEndVertex(Uint32 *vertex)
{
    *_nj_listtype_ = (Uint32 *)(((Uint32)vertex & SQ_ADDRESS) | _nj_sq_base_);
}

/* set Ninja's header words for color mode mode, and make mode the current
 * context's color mode. the global word takes mode's MODE_GLOBAL bits, and the
 * TSP word its MODE_TSP bits. GLOBAL_SHADOW is set when _nj_control_3d_flag_
 * holds NJD_CONTROL_3D_SHADOW. it is set too when the flags hold
 * NJD_CONTROL_3D_SHADOW_OPAQUE and mode has no MODE_TRANS bit. otherwise
 * GLOBAL_SHADOW is cleared */
void njSetkmColorMode(Uint32 mode)
{
    /* @hack redundant: register on global, tsp and isp, which keeps them in
     * R5, R6 and R7, beside &_nj_parameterkm_ in R14 (0x8c658fe0 to 0x8c659000
     * in 1ST_READ.BIN); without it, isp goes through the frame, the function
     * is 4 bytes longer, and 38 of 41 words differ */
    register Uint32 global = _nj_parameterkm_.GLOBALPARAMBUFFER & ~MODE_GLOBAL;
    register Uint32 tsp = _nj_parameterkm_.TSPPARAMBUFFER & ~MODE_TSP;
    register Uint32 isp;
    Uint32 flags;

    global |= mode & MODE_GLOBAL;
    tsp |= mode & MODE_TSP;
    /* @hack redundant: the ISP word read and written back unchanged (MOV.L
     * @(4,R14),R7 at 0x8c659000, MOV.L R7,@(4,R14) at 0x8c65901a in
     * 1ST_READ.BIN); without the read and the write, the function is 8 bytes
     * shorter and 37 of 41 words differ */
    isp = _nj_parameterkm_.ISPPARAMBUFFER;
    flags = _nj_control_3d_flag_;
    /* @hack spelling: the goto, which shares one arm between both tests and
     * keeps TST R0,R13 at 0x8c659004 in 1ST_READ.BIN; as one || of the two, it
     * is TST R13,R0 and 1 of 41 words differ */
    if (flags & NJD_CONTROL_3D_SHADOW) {
        goto shadow;
    } else if ((flags & NJD_CONTROL_3D_SHADOW_OPAQUE) && !(mode & MODE_TRANS)) {
    shadow:
        global |= GLOBAL_SHADOW;
    } else {
        global &= ~GLOBAL_SHADOW;
    }
    _nj_parameterkm_.GLOBALPARAMBUFFER = global;
    _nj_parameterkm_.ISPPARAMBUFFER = isp;
    _nj_parameterkm_.TSPPARAMBUFFER = tsp;
    *_nj_color_mode_ = mode;
}

Sint32 njSetkmTextureNum(Uint32 n)
{
    NJS_TEXMEMLIST *tex =
        (NJS_TEXMEMLIST *)_nj_current_texlist->textures[n].texaddr;
    Uint32 word;

    if (tex->globalIndex != _nj_current_texture ||
        _nj_current_texture_bank != tex->bank) {
        _nj_current_texture = tex->globalIndex;
        _nj_current_texture_bank = tex->bank;
        word = _nj_parameterkm_.TSPPARAMBUFFER;
        word &= ~TSP_UV_SIZE;
        word |= tex->tspparambuffer & TSP_UV_SIZE;
        _nj_parameterkm_.TSPPARAMBUFFER = word;
        word = tex->texparambuffer;
        if ((word & TEXTURE_ADDRESS_MASK) > TEXTURE_ADDRESS_MAX) {
            return -1;
        }
        _nj_parameterkm_.TexturePARAMBUFFER = word;
    }
    return 1;
}

/* njSetkmTextureNum for the record of _nj_texmemlist that holds globalIndex,
 * which njSearchTexList finds. globalIndex must be one a record holds, since
 * the search's -1 for any other goes unchecked */
Sint32 njSetkmTextureNumG(Uint32 globalIndex)
{
    Bool changed = FALSE;
    /* @hack redundant: register on i, tex and word, which keeps changed in
     * R14, globalIndex in R11, &_nj_texmemlist in R12 and njSearchTexList's
     * address in R13 (0x8c6590da to 0x8c6590ea in 1ST_READ.BIN); without it,
     * the four take other registers and 21 of 81 words differ */
    register Uint32 i;
    register NJS_TEXMEMLIST *tex;
    register Uint32 word;

    if (globalIndex != _nj_current_texture) {
        changed = TRUE;
        i = njSearchTexList(_nj_texmemlist, _nj_texmemsize, globalIndex);
    } else {
        i = njSearchTexList(_nj_texmemlist, _nj_texmemsize, globalIndex);
        /* @hack redundant: ++i and --i, which take the record's offset from i's
         * copy in R4 (MOV R4,R3 at 0x8c65910a in 1ST_READ.BIN); without them,
         * it comes from R0 and 1 of 81 words differ */
        ++i;
        --i;
        tex = &_nj_texmemlist[i];
        if (_nj_current_texture_bank != tex->bank) {
            changed = TRUE;
        }
    }
    if (changed) {
        i = njSearchTexList(_nj_texmemlist, _nj_texmemsize, globalIndex);
        tex = &_nj_texmemlist[i];
        _nj_current_texture = globalIndex;
        _nj_current_texture_bank = tex->bank;
        word = _nj_parameterkm_.TSPPARAMBUFFER;
        word &= ~TSP_UV_SIZE;
        word |= tex->tspparambuffer & TSP_UV_SIZE;
        _nj_parameterkm_.TSPPARAMBUFFER = word;
        word = tex->texparambuffer;
        if ((word & TEXTURE_ADDRESS_MASK) > TEXTURE_ADDRESS_MAX) {
            return -1;
        }
        _nj_parameterkm_.TexturePARAMBUFFER = word;
    }
    return 1;
}

/* njSetkmTextureNum for record n of _nj_texmemlist, even when that texture is
 * current */
Sint32 njSetkmTextureMemList(Uint32 n)
{
    /* @hack redundant: register on word, which puts &_nj_parameterkm_ in R14,
     * saved on the stack (MOV.L R14,@-R15 at 0x8c659180 in 1ST_READ.BIN);
     * without it, &_nj_parameterkm_ takes R5, the function is 4 bytes
     * shorter, and 38 of 39 words differ */
    register Uint32 word;

    _nj_current_texture = _nj_texmemlist[n].globalIndex;
    _nj_current_texture_bank = _nj_texmemlist[n].bank;
    word = _nj_parameterkm_.TSPPARAMBUFFER;
    word &= ~TSP_UV_SIZE;
    word |= _nj_texmemlist[n].tspparambuffer & TSP_UV_SIZE;
    _nj_parameterkm_.TSPPARAMBUFFER = word;
    word = _nj_texmemlist[n].texparambuffer;
    if ((word & TEXTURE_ADDRESS_MASK) > TEXTURE_ADDRESS_MAX) {
        return -1;
    }
    _nj_parameterkm_.TexturePARAMBUFFER = word;
    return 1;
}

/* the setters below take a field's value already shifted to its place in the
 * word, or TRUE or FALSE for one bit. a one-bit setter ignores any other
 * value */

void njSetkmModelAttr(Uint32 attr)
{
    Uint32 tsp = _nj_parameterkm_.TSPPARAMBUFFER & ~MATERIAL_TSP;

    tsp |= attr & MATERIAL_TSP;
    _nj_parameterkm_.TSPPARAMBUFFER = tsp;
}

void njZBufferkmMode(Int mode)
{
    switch (mode) {
    case TRUE:
        _nj_parameterkm_.ISPPARAMBUFFER &= ~ISP_Z_WRITE_DISABLE;
        break;
    case FALSE:
        _nj_parameterkm_.ISPPARAMBUFFER |= ISP_Z_WRITE_DISABLE;
        break;
    }
}

void njMipmapkmAdjust(Uint32 mode)
{
    _nj_parameterkm_.TSPPARAMBUFFER =
        (_nj_parameterkm_.TSPPARAMBUFFER & ~TSP_MIPMAP_D) | mode;
}

void njTextureFilterkmMode(Uint32 mode)
{
    _nj_parameterkm_.TSPPARAMBUFFER =
        (_nj_parameterkm_.TSPPARAMBUFFER & ~TSP_FILTER) | mode;
}

void njSuperSamplekmMode(Int mode)
{
    switch (mode) {
    case TRUE:
        _nj_parameterkm_.TSPPARAMBUFFER |= TSP_SUPERSAMPLE;
        break;
    case FALSE:
        _nj_parameterkm_.TSPPARAMBUFFER &= ~TSP_SUPERSAMPLE;
        break;
    }
}

void njTextureClampkmMode(Uint32 mode)
{
    _nj_parameterkm_.TSPPARAMBUFFER =
        (_nj_parameterkm_.TSPPARAMBUFFER & ~TSP_CLAMP) | mode;
}

void njTextureFlipkmMode(Uint32 mode)
{
    _nj_parameterkm_.TSPPARAMBUFFER =
        (_nj_parameterkm_.TSPPARAMBUFFER & ~TSP_FLIP) | mode;
}

void njkmPolygonCullingSize(Float size)
{
    kmSetCullingRegister(size);
}

void njPolygonCullingkmMode(Uint32 mode)
{
    _nj_parameterkm_.ISPPARAMBUFFER =
        (_nj_parameterkm_.ISPPARAMBUFFER & ~ISP_CULL) | mode;
}

/* the bit of njColorBlendingkmMode's mode that says it gives both factors */
#define BOTH_FACTORS 1

/* set the TSP word's blend factors from mode. a mode with BOTH_FACTORS gives
 * both. any other mode gives the source factor for target NJD_SOURCE_COLOR,
 * and the destination factor for any other target */
void njColorBlendingkmMode(Int target, Uint32 mode)
{
    Uint32 tsp = _nj_parameterkm_.TSPPARAMBUFFER;

    if (mode & BOTH_FACTORS) {
        tsp &= ~TSP_FACTORS;
        /* @bug logic: a mode with BOTH_FACTORS goes into the TSP word whole, so
         * BOTH_FACTORS lands in the V size code (OR R5,R6 at 0x8c6592b2 in
         * 1ST_READ.BIN, 0x8ce0e88a in LOADER.BIN). nothing in the game calls
         * the function: no image holds its address, and no BSR reaches it */
        tsp |= mode;
        _nj_parameterkm_.TSPPARAMBUFFER = tsp;
    } else {
        if (target == NJD_SOURCE_COLOR) {
            tsp &= ~TSP_SRC;
            tsp |= mode & TSP_SRC;
            _nj_parameterkm_.TSPPARAMBUFFER = tsp;
        } else {
            tsp &= ~TSP_DST;
            tsp |= mode & TSP_DST;
            _nj_parameterkm_.TSPPARAMBUFFER = tsp;
        }
    }
}

void njSpecularkmMode(Int mode)
{
    switch (mode) {
    case TRUE:
        _nj_parameterkm_.GLOBALPARAMBUFFER |= GLOBAL_OFFSET;
        break;
    case FALSE:
        _nj_parameterkm_.GLOBALPARAMBUFFER &= ~GLOBAL_OFFSET;
        break;
    }
}

void njAlphakmMode(Int mode)
{
    switch (mode) {
    case TRUE:
        _nj_parameterkm_.TSPPARAMBUFFER |= TSP_ALPHA;
        break;
    case FALSE:
        _nj_parameterkm_.TSPPARAMBUFFER &= ~TSP_ALPHA;
        break;
    }
}

void njIgnoreTextureAlphakmMode(Int mode)
{
    switch (mode) {
    case TRUE:
        _nj_parameterkm_.TSPPARAMBUFFER |= TSP_IGNORE_ALPHA;
        break;
    case FALSE:
        _nj_parameterkm_.TSPPARAMBUFFER &= ~TSP_IGNORE_ALPHA;
        break;
    }
}

void njTextureShadingkmMode(Uint32 mode)
{
    _nj_parameterkm_.TSPPARAMBUFFER =
        (_nj_parameterkm_.TSPPARAMBUFFER & ~TSP_SHADING) | mode;
}

/* the flags of a chunk model's texture number, above its texture's index */
#define CNK_SUPERSAMPLE 0x2000
#define CNK_BILINEAR 0x4000
#define CNK_FLAGS 0xe000
/* the clamp and flip bits of a chunk model's attr, which the TSP word holds 3
 * bits higher */
#define CNK_CLAMP_FLIP ((TSP_FLIP | TSP_CLAMP) >> 3)
/* the TSP bits njSetkmCnkTextureNum clears before attr's and n's go in */
#define CNK_CLEARED                                                            \
    (TSP_FLIP | TSP_CLAMP | TSP_TRILINEAR | TSP_SUPERSAMPLE | TSP_MIPMAP_D)

/* set Ninja's header words for a chunk model's texture number n, whose bits
 * below CNK_FLAGS index the current texture list. when that texture's global
 * index isn't current, the texture becomes current, but the current bank
 * stays. only then does the TSP word take the texture's UV sizes, and the
 * texture word the record's texparambuffer, with no address check. only then
 * does _nj_texglobal_color_ change: to TRUE for NJD_TEXFMT_ARGB_1555 pixels,
 * and to FALSE for any other.
 *
 * every call then sets the TSP word's mipmap D adjust, clamp and flip from
 * attr, and its super sampling from n. n's CNK_BILINEAR sets TSP_BILINEAR,
 * and a call without it leaves the bit as it was. every call clears
 * TSP_TRILINEAR. it returns _nj_texglobal_color_ */
Sint32 njSetkmCnkTextureNum(Uint32 attr, Uint32 n)
{
    NJS_TEXMEMLIST *tex =
        (NJS_TEXMEMLIST *)_nj_current_texlist->textures[n & ~CNK_FLAGS].texaddr;
    /* @hack redundant: register on tsp, which puts &_nj_parameterkm_ in R14,
     * saved on the stack (MOV.L R14,@-R15 at 0x8c659386 in 1ST_READ.BIN);
     * without it, &_nj_parameterkm_ takes R7, the function is 4 bytes
     * shorter, and 45 of 65 words differ */
    register Uint32 tsp;
    Uint32 format;

    if (tex->globalIndex != _nj_current_texture) {
        _nj_current_texture = tex->globalIndex;
        tsp = _nj_parameterkm_.TSPPARAMBUFFER;
        tsp &= ~TSP_UV_SIZE;
        tsp |= tex->tspparambuffer & TSP_UV_SIZE;
        _nj_parameterkm_.TSPPARAMBUFFER = tsp;
        _nj_parameterkm_.TexturePARAMBUFFER = tex->texparambuffer;
        format = (tex->texinfo.texsurface.Type >> 16) & NJD_TEXFMT_COLOR_MASK;
        if (format == NJD_TEXFMT_ARGB_1555) {
            _nj_texglobal_color_ = TRUE;
        } else {
            _nj_texglobal_color_ = FALSE;
        }
    }
    /* @bug logic: the mask keeps TSP_BILINEAR, so a texture number's
     * CNK_BILINEAR sets the bilinear filter and none clears it. CNK_CLEARED's
     * complement, 0xfff820ff (MOV.L at 0x8c6593d8 and AND R3,R6 at 0x8c6593de
     * in 1ST_READ.BIN, 0x8ce0e9b0 and 0x8ce0e9b6 in LOADER.BIN), clears
     * TSP_TRILINEAR, which neither attr nor n gives, but not TSP_BILINEAR */
    tsp = _nj_parameterkm_.TSPPARAMBUFFER & ~CNK_CLEARED;
    tsp |= (attr & CNK_CLAMP_FLIP) << 3;
    tsp |= attr & TSP_MIPMAP_D;
    tsp |= (n & CNK_SUPERSAMPLE) >> 1;
    tsp |= (n & CNK_BILINEAR) >> 1;
    _nj_parameterkm_.TSPPARAMBUFFER = tsp;
    return _nj_texglobal_color_;
}

/* set the blend factors for a chunk model from attr's bits 8 to 13 */
void njSetkmCnkBlendMode(Uint32 attr)
{
    Uint32 tsp = _nj_parameterkm_.TSPPARAMBUFFER & ~TSP_FACTORS;

    tsp |= (attr << 18) & TSP_FACTORS;
    _nj_parameterkm_.TSPPARAMBUFFER = tsp;
}
