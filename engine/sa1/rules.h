/* the rules of Ninja and of the game's model.src that both sides of an SA1
 * draw follow. the Dreamcast's drawers in engine/dc/sa1/ draw a model in
 * place. on a host, live.c and records.c beside this record the draw, and
 * basic.cpp reads the record. this holds each number of those rules once. it
 * holds the two rules that are one function each, drawn_mode and set_header.
 *
 * it includes nothing and defines only macros and static inline functions, so
 * C17 and C++17 compile it alike. a source without the SDK's headers can use
 * the macros that name none of its numbers or types. drawn_mode lies under
 * NINJA_H, ninja.h's guard, so only a source that includes ninja.h before this
 * has it */
#ifndef SA1_RULES_H
#define SA1_RULES_H

/* masks, as njModel.c names them */
#define MATERIAL_ID 0x3fff /* the material index in a meshset's type_matId */
#define MATERIAL_TEXTURE 0x3fffffff /* the texture number in attr_texId */
#define STRIP_COUNT 0x7fff /* a strip's point count, below NJD_TRIMESH_END */
/* the shift that takes a meshset's NJD_MESHSET_ type to its index in
 * engine/dc/sa1/model.c's polygon_sizes */
#define MESHSET_TYPE_SHIFT 14

/* fields of the polygon header's global and TSP words. njkm.c names each but
 * GLOBAL_VERTEX_TYPE and MODULATE_ALPHA. GLOBAL_VERTEX_TYPE is the object
 * control byte's color type, texture, offset, Gouraud and 16-bit UV bits.
 * MODULATE_ALPHA sets both bits of TSP_SHADING, the texture and shading
 * instruction, where decal clears both */
#define GLOBAL_TRANSLUCENT 0x02000000
#define GLOBAL_OBJECT_CONTROL 0x000000ff
#define GLOBAL_VERTEX_TYPE 0x0000003f
#define TSP_SRC 0xe0000000
#define TSP_DST 0x1c000000
#define TSP_ALPHA 0x00100000
#define TSP_IGNORE_ALPHA 0x00080000
#define TSP_FLIP 0x00060000
#define TSP_CLAMP 0x00018000
#define TSP_FILTER 0x00006000
#define TSP_SUPERSAMPLE 0x00001000
#define TSP_MIPMAP_D 0x00000f00
#define TSP_SHADING 0x000000c0
#define TSP_UV_SIZE 0x0000003f
#define MODULATE_ALPHA TSP_SHADING
/* the shifts down from TSP_FILTER to a filter and from TSP_MIPMAP_D to a
 * mipmap D adjust. records.c reads them, and no drawer does */
#define TSP_FILTER_SHIFT 13
#define TSP_MIPMAP_D_SHIFT 8
/* fields of the header's ISP and TSP words that records.c reads, and no
 * drawer does. the shifts take a depth compare mode down from ISP_DEPTH and
 * a fog mode from TSP_FOG. njkm.c names ISP_Z_WRITE_DISABLE, and Kamui's
 * kmvertex_kamui_lib.c the other two */
#define ISP_DEPTH 0xe0000000
#define ISP_DEPTH_SHIFT 29
#define ISP_Z_WRITE_DISABLE 0x04000000
#define TSP_FOG 0x00c00000
#define TSP_FOG_SHIFT 22
/* blend factors, as njkm.c names them. SRC puts one in TSP_SRC, and DST
 * one in TSP_DST, by that field's shift */
#define BLEND_ZERO 0
#define BLEND_ONE 1
#define BLEND_SRC_ALPHA 4
#define BLEND_INV_SRC_ALPHA 5
#define TSP_SRC_SHIFT 29
#define TSP_DST_SHIFT 26
#define SRC(factor) ((Uint32)(factor) << TSP_SRC_SHIFT)
#define DST(factor) ((Uint32)(factor) << TSP_DST_SHIFT)

/* the modes set_header takes. a mode's GLOBAL_MODE bits go to the global word,
 * and its TSP_MODE bits to the TSP word. its TSP_SHADING bits overlap the
 * object control byte's shadow and volume bits, so GLOBAL_MODE leaves them
 * out. an untextured meshset takes ninja.h's COLOR_MODE_OPAQUE or
 * COLOR_MODE_TRANS. a textured one takes MODE_DECAL, MODE_OPAQUE_TEXTURED or
 * MODE_TRANS_TEXTURED, which add a texture and an offset color */
#define GLOBAL_MODE (GLOBAL_TRANSLUCENT | GLOBAL_VERTEX_TYPE)
#define TSP_MODE (TSP_ALPHA | TSP_IGNORE_ALPHA | TSP_SHADING)
#define TEXTURED (COLOR_MODE_TEXTURE | COLOR_MODE_OFFSET)
#define MODE_DECAL (COLOR_MODE_OPAQUE | TEXTURED)
#define MODE_OPAQUE_TEXTURED (COLOR_MODE_OPAQUE | TEXTURED | MODULATE_ALPHA)
#define MODE_TRANS_TEXTURED (COLOR_MODE_TRANS | TEXTURED | MODULATE_ALPHA)
#ifdef NINJA_H
/* the mode a meshset's header takes from drawn, its material's flags after
 * Ninja's constant attributes */
static inline Uint32 drawn_mode(Uint32 drawn)
{
    if (!(drawn & NJD_FLAG_USE_TEXTURE)) {
        return drawn & NJD_FLAG_USE_ALPHA ? COLOR_MODE_TRANS
                                          : COLOR_MODE_OPAQUE;
    }
    if (drawn & NJD_FLAG_USE_ALPHA) {
        return MODE_TRANS_TEXTURED;
    }
    return drawn & NJD_FLAG_IGNORE_LIGHT ? MODE_DECAL : MODE_OPAQUE_TEXTURED;
}
#endif

/* MATERIAL_TSP is the TSP bits njSetkmModelAttr takes from a material's
 * flags. an opaque polygon's header clears TSP_BLEND and ORs in TSP_OPAQUE,
 * as njkm.c builds it */
#define MATERIAL_TSP                                                           \
    (TSP_SRC | TSP_DST | TSP_FLIP | TSP_CLAMP | TSP_FILTER | TSP_SUPERSAMPLE | \
     TSP_MIPMAP_D)
#define TSP_BLEND (TSP_SRC | TSP_DST | TSP_ALPHA)
#define TSP_OPAQUE (SRC(BLEND_ONE) | DST(BLEND_ZERO) | TSP_IGNORE_ALPHA)
#ifdef TEXTURE_RECORD
/* set the header's words for a meshset of material in mode. it does what
 * model.src's param_set_attrs, param_set_texture and param_set_flags do. drawn
 * is material's flags after Ninja's constant attributes. unlike
 * njSetkmTextureNum, it reads no bank and checks no address.
 *
 * it reads and leaves Ninja's header, current texture and texture list under
 * engine/dc/sa1/header.h's names. it finds the record a texture name's texaddr
 * addresses by TEXTURE_RECORD(texaddr). a source that calls it includes
 * njkamui.h, has those names and defines TEXTURE_RECORD, all before it
 * includes this */
static inline void set_header(const NJS_MATERIAL *material, Uint32 drawn,
                              Uint32 mode)
{
    struct nj_parameterkm *header = &sa1_live_header;
    Uint32 tsp =
        (header->TSPPARAMBUFFER & ~MATERIAL_TSP) | (drawn & MATERIAL_TSP);

    if (drawn & NJD_FLAG_USE_TEXTURE) {
        Uint32 *current = &sa1_live_texture;
        const NJS_TEXMEMLIST *texture = TEXTURE_RECORD(
            sa1_live_texlist->textures[material->attr_texId & MATERIAL_TEXTURE]
                .texaddr);

        if (texture->globalIndex != *current) {
            *current = texture->globalIndex;
            tsp =
                (tsp & ~TSP_UV_SIZE) | (texture->tspparambuffer & TSP_UV_SIZE);
            header->TexturePARAMBUFFER = texture->texparambuffer;
        }
    }
    header->GLOBALPARAMBUFFER =
        (header->GLOBALPARAMBUFFER &
         ~(GLOBAL_TRANSLUCENT | GLOBAL_OBJECT_CONTROL)) |
        (mode & GLOBAL_MODE);
    header->TSPPARAMBUFFER = (tsp & ~TSP_MODE) | (mode & TSP_MODE);
}
#endif
/* the flags of an unlit material in the translucent list */
#define UNLIT_TRANSLUCENT (NJD_FLAG_IGNORE_LIGHT | NJD_FLAG_USE_ALPHA)
/* the 3D control flags the Ninja forms of src/draw/model.h require */
#define NINJA_FORMS_CONTROL                                                    \
    (NJD_CONTROL_3D_CONSTANT_ATTR | NJD_CONTROL_3D_CONSTANT_MATERIAL)

/* the top 6 bits of a list's write pointer, as njkm.c names them. the open
 * list's store queue address leaves them out. only the Dreamcast's walks,
 * model.c and easy.h, read them */
#define LIST_TOP 0xfc000000

/* model.src's 1/255, H'3B808083, which lies 2 units in the last place above
 * H'3B808081, the float nearest 1/255. the draws scale u, v and alpha by it,
 * so that they keep model.src's bits */
#define INV_255 0x1.010106p-8F
#define HALF_255 127.5F

#endif
