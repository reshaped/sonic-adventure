/* a capture: the pictures a build of the game drew, as records a renderer can
 * draw again. a mesh is its model's bytes, and a texture is named by what it
 * is on the disc. a draw is placed in world space. no record holds a game
 * address or a Dreamcast vertex.
 *
 * a capture is records, each opening with sa_record_head. SA_CAP_HEAD comes
 * first, then for each picture SA_CAP_PICTURE, its draws and SA_CAP_END. a
 * mesh's draw is the contract's SA_REC_DRAW. its mesh and materials are the
 * capture's ids where a process holds handles. its shade is an SA_REC_SHADE,
 * and an SA_SHADE_LIGHTS one follows the SA_REC_LIGHTS it reads. a record that
 * gives an id comes before the first record that names it. no later record
 * gives that id again. a view a picture's draws use is the contract's
 * SA_REC_VIEW. it lies inside the picture, before the first record that names
 * it. a task's birth and death are sa_sa1.h's SA_REC_TASK_CREATE and
 * SA_REC_TASK_DESTROY. a resource's load and free are SA_CAP_LOAD and
 * SA_CAP_FREE. an act's scene, SA_CAP_SCENE to SA_CAP_SCENE_END, comes where
 * the act loaded, between two pictures or inside one. SA_CAP_SCENE_ENDED comes
 * where the act went */
#ifndef SA_CAPTURE_H
#define SA_CAPTURE_H

#include "sa_sa1.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SA_CAPTURE_MAGIC 0x50414353U /* "SCAP" */
#define SA_CAPTURE_VERSION 0U
/* the most bytes a record holds, its head among them. a head's size is a
 * multiple of 4, and this is the largest its 16 bits can say. a writer holds
 * each record to it before it names anything. a draw's triangles and a
 * texture's texels that one record can't hold go in several. a mesh or a
 * list of materials too big for one stays out. its draw counts as lost */
#define SA_CAP_RECORD_MOST (UINT16_MAX & ~3U)
/* an id that names nothing */
#define SA_CAP_NONE 0xffffffffU
/* the address an inline mesh's bytes start at, which its pointers count
 * from */
#define SA_CAP_INLINE_BASE 0x10000000U

enum {
    SA_CAP_HEAD = 64,
    /* 65 is reserved */
    SA_CAP_MESH = 66,
    SA_CAP_TEXTURE = 67,
    SA_CAP_MATERIAL = 68,
    SA_CAP_PALETTES = 69,
    SA_CAP_PICTURE = 70,
    SA_CAP_MATERIALS = 71,
    SA_CAP_SPRITE = 72,
    SA_CAP_END = 73,
    SA_CAP_TRIANGLES = 74,
    SA_CAP_LOAD = 75,
    SA_CAP_FREE = 76,
    SA_CAP_TEXELS = 77,
    SA_CAP_SCENE = 78,
    SA_CAP_SCENE_END = 79,
    SA_CAP_SCENE_ENDED = 80,
    SA_CAP_SCENE_LIGHTING = 81
};

typedef struct sa_cap_head {
    sa_record_head head;
    uint32_t magic; /* SA_CAPTURE_MAGIC */
    uint32_t version; /* SA_CAPTURE_VERSION */
    uint32_t game; /* SA_GAME_* */
    /* the first 32 bits of the captured build's commit. 0 from a writer that
     * has none: the adapter writes 0 */
    uint32_t build;
} sa_cap_head;

/* a mesh: a model as it stands at a draw's call, and the form its bytes
 * take. form 1 is reserved */
enum {
    /* an NJS_MODEL as the SH-4 lays it out, little-endian, in the bytes
     * that follow the record. its pointers are addresses that count from
     * the model at SA_CAP_INLINE_BASE */
    SA_MESH_NINJA_INLINE = 2
};
typedef struct sa_cap_mesh {
    sa_record_head head;
    uint32_t mesh;
    uint32_t format; /* SA_MESH_* */
    sa_ref at; /* in SA_SPACE_NONE */
} sa_cap_mesh;

/* a texture, by the global index its PVR file gives it and the name its
 * texlist gives it. file is the sa_name_id of the file its texels come from,
 * by its upper-case path on the disc. that file is a PVM or a PVR of its own,
 * as "SONICADV/OBJ_CASINO.PRS". two files may hold other texels under one
 * index and name. so a reader takes them from that file alone. file is 0 where
 * the capture saw none. a reader then finds the texture by name and index in
 * any file. a texture the game made in memory, which no file holds, comes with
 * its SA_CAP_TEXELS */
typedef struct sa_cap_texture {
    sa_record_head head;
    uint32_t texture;
    uint32_t global_index;
    uint32_t file;
    char name[28]; /* 0-filled */
} sa_cap_texture;

/* a part of the PVRT chunk of a texture the game made in memory. its bytes
 * follow, to head.size. the parts follow the texture's record in order. the
 * chunk ends in zeros where its texels' bytes are no multiple of 4, and its
 * length counts them */
typedef struct sa_cap_texels {
    sa_record_head head;
    uint32_t texture;
} sa_cap_texels;

/* a material as a draw took it, as sa_material says. its texture is a
 * capture's id or SA_CAP_NONE */
typedef struct sa_cap_material {
    sa_record_head head;
    uint32_t material;
    uint32_t texture;
    uint32_t specular;
    uint16_t flags;
    uint8_t alpha;
    uint8_t filter;
    uint8_t src;
    uint8_t dst;
    uint8_t mip_bias;
    uint8_t exponent; /* as sa_material's */
    uint32_t diffuse;
} sa_cap_material;

/* the materials a draw took, one for each of its mesh's. their ids follow,
 * to head.size. two equal lists share one id. a list names its materials as
 * they stood at its record. so a material's SA_CAP_FREE may come while a list
 * still names it, as one a scene's part reads */
typedef struct sa_cap_materials {
    sa_record_head head;
    uint32_t materials;
    uint16_t count;
    uint16_t reserved; /* 0 */
} sa_cap_materials;

/* the palettes a draw shades its points from, as sa_palettes says. a
 * version lasts from its record to the next palettes record. a picture names
 * the last one ahead of it, and no record names an older one. so a reader
 * that takes the records in order holds one version at a time */
#define SA_CAP_PALETTES_COUNT SA_PALETTES
#define SA_CAP_PALETTE_ENTRIES SA_PALETTE_ENTRIES
#define SA_CAP_UNLIT_OFFSETS SA_UNLIT_OFFSETS
typedef struct sa_cap_palettes {
    sa_record_head head;
    uint32_t palettes; /* its id */
    uint32_t words[SA_CAP_PALETTES_COUNT * SA_CAP_PALETTE_ENTRIES * 2 +
                   SA_CAP_UNLIT_OFFSETS];
} sa_cap_palettes;

/* a picture: its own view as sa_picture's, its background, and its fog as
 * sa_fog says. a writer that learns the view only as the picture ends sets
 * it then */
typedef struct sa_cap_picture {
    sa_record_head head;
    uint32_t picture; /* from 0 */
    uint32_t tick; /* sa_picture's tick, counted by play_time */
    /* the vblanks the writer had counted at the picture's first draw. 0 from
     * a writer that counts none: the adapter writes 0 */
    uint32_t vblank;
    sa_view view;
    uint32_t clear; /* ARGB */
    uint32_t fog; /* ARGB */
    float fog_density;
    uint16_t fog_table[128]; /* each two 8-bit alphas, as the PowerVR's */
    uint32_t palettes; /* the palettes' id */
    /* the background's other colors, as sa_picture.back: top right and
     * bottom left. 0 for both, or a record that ends before them, is clear
     * alone */
    uint32_t clear_right;
    uint32_t clear_below;
    /* sa_picture's SA_PICTURE_*. a record that ends before it has none */
    uint32_t flags;
} sa_cap_picture;

/* SA_CAP_SPRITE and SA_CAP_TRIANGLES are the contract's sa_sprite and
 * sa_triangles. their texture is a capture's id or SA_CAP_NONE. triangles'
 * points follow, to head.size */
typedef sa_sprite sa_cap_sprite;
typedef sa_triangles sa_cap_triangles;
typedef sa_point sa_cap_point;
/* the most points that follow an SA_CAP_TRIANGLES in its record. a draw of
 * more points goes in several SA_CAP_TRIANGLES, one after another. each is the
 * draw's own but for its count and points. each is whole, so a reader draws
 * it as it comes. a strip's part that more follow holds an even count of
 * triangles. its next part opens with the two points it closed with, so each
 * triangle keeps its turn. a fan's next part opens with the fan's first
 * point */
#define SA_CAP_POINTS_MOST                                                     \
    ((SA_CAP_RECORD_MOST - sizeof(sa_cap_triangles)) / sizeof(sa_cap_point))

/* a resource's life. SA_CAP_LOAD says the game loaded it here. a resource
 * no SA_CAP_LOAD names came with the first draw that named it. SA_CAP_FREE
 * says a reader may let it go, and no later record names it. a draw that did
 * would be refused. kind is SA_KIND_MESH, _TEXTURE or _MATERIAL, and id is
 * the capture's. _MATERIAL is an SA_CAP_MATERIALS list. a free may also
 * name one material by SA_CAP_MATERIAL, or a shade by SA_REC_SHADE */
typedef struct sa_cap_life {
    sa_record_head head;
    uint32_t kind;
    uint32_t id;
} sa_cap_life;

/* an act's scene, as sa_scene_begin's calls give one. SA_CAP_SCENE is the
 * contract's sa_scene, and names the act. each SA_REC_DRAW from there to
 * SA_CAP_SCENE_END is a part of it, never a picture's draw. its mesh and
 * materials are the capture's ids, as a draw's are. its shade is an
 * SA_REC_SHADE that came before it, in the scene or ahead of it. no
 * SA_CAP_FREE names the mesh or materials a part reads while it reads them. a
 * part reads those it was given with until a picture's first draw under its
 * instance names others. it reads those from then on. SA_CAP_SCENE_ENDED, an
 * sa_scene too, says the act went. SA_CAP_SCENE_LIGHTING is the contract's
 * sa_lighting, the scene's lights as sa_scene_lighting gives them. at most one
 * comes from SA_CAP_SCENE to SA_CAP_SCENE_END */
typedef sa_scene sa_cap_scene;
typedef struct sa_cap_scene_end {
    sa_record_head head;
    uint32_t scene;
    uint32_t parts; /* the parts it held */
    /* the parts the capture couldn't name or had no room to hold */
    uint32_t lost;
} sa_cap_scene_end;

typedef struct sa_cap_end {
    sa_record_head head;
    uint32_t picture;
    uint32_t draws; /* the draws, sprites and triangles it held */
    /* the draws, sprites and triangles the capture dropped, or whose lights
     * or glyphs it cut */
    uint32_t lost;
} sa_cap_end;

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#define SA_CAP_LAYOUT(type, bytes)                                             \
    static_assert(sizeof(type) == (bytes), #type "'s layout")
#else
#define SA_CAP_LAYOUT(type, bytes)                                             \
    _Static_assert(sizeof(type) == (bytes), #type "'s layout")
#endif
SA_CAP_LAYOUT(sa_cap_head, 20);
SA_CAP_LAYOUT(sa_cap_mesh, 24);
SA_CAP_LAYOUT(sa_cap_texture, 44);
SA_CAP_LAYOUT(sa_cap_texels, 8);
SA_CAP_LAYOUT(sa_cap_material, 28);
SA_CAP_LAYOUT(sa_cap_materials, 12);
SA_CAP_LAYOUT(sa_cap_palettes, 24584);
SA_CAP_LAYOUT(sa_cap_picture, 372);
SA_CAP_LAYOUT(sa_cap_life, 12);
SA_CAP_LAYOUT(sa_cap_scene_end, 16);
SA_CAP_LAYOUT(sa_cap_end, 16);
#undef SA_CAP_LAYOUT

#endif
