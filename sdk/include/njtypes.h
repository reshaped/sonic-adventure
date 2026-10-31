/* Ninja's types and constants, built on sg_xpt.h's: include sg_xpt.h first.
 *
 * this header must include nothing. a source that includes src/vmu/draw.h
 * opens it fifth, through vmu/screen.h, main.h and ninja.h, so an #include
 * here would be made inside five open headers. SHC's releases R10 and R32 stop
 * with error 2500 (E) two lines past such an #include.
 *
 * it must declare no object either. R10 lays out bss in the order of each
 * name's first declaration, and njGlobal.c includes this header, not ninja.h,
 * so that its own definitions set that order */
#ifndef NJTYPES_H
#define NJTYPES_H

typedef Sint32 Angle; /* 65536 to a full turn */
#ifdef GCC
/* an FMOV with FPSCR.SZ set moves 8 bytes, and takes an address error at an
 * address that isn't 8-byte aligned. some of Ninja's and SH4ZAM's routines
 * move a matrix that way. GCC would align a Float[16] to 4, so NJS_MATRIX
 * takes 8. inside a structure, config/gcc.mk's -fpack-struct=4 still aligns
 * a matrix to 4, so each layout stays SHC's. NJS_VIEW and NJS_LIGHT align
 * theirs to 8, below */
typedef Float NJS_MATRIX[16] __attribute__((aligned(8)));
#else
typedef Float NJS_MATRIX[16];
#endif
typedef struct {
    Float x, y;
} NJS_POINT2;
typedef struct {
    Float x, y, z;
} NJS_POINT3, NJS_VECTOR;
typedef struct {
    Float a, r, g, b;
} NJS_ARGB;
/* a fog table's entries */
#define FOG_TABLE_ENTRIES 128
typedef Float NJS_FOG_TABLE[FOG_TABLE_ENTRIES];

typedef struct {
    Sint16 u, v;
} NJS_TEX;
typedef struct {
    Uint8 b, g, r, a;
} NJS_BGRA;
typedef union {
    Uint32 color;
    NJS_TEX tex;
    NJS_BGRA argb;
} NJS_COLOR;
/* a texture coordinate pair packed in one word, as a model's data holds it: u
 * in the low half and v in the high */
#define UV(u, v) ((Uint32)(Uint16)(u) | (Uint32)(Uint16)(v) << 16)

typedef struct {
    NJS_COLOR diffuse;
    NJS_COLOR specular;
    Float exponent;
    Uint32 attr_texId;
    Uint32 attrflags;
} NJS_MATERIAL;

/* a model's polygons of one type and material */
typedef struct {
    /* an NJD_MESHSET_ type in the top two bits, and below them the index of
     * the meshset's material in the model's mats */
    Uint16 type_matId;
    Uint16 nbMesh;
    /* each polygon's points, as indices into the model's points. a strip, or
     * an NJD_MESHSET_N polygon, starts with its point count */
    Sint16 *meshes;
    Uint32 *attrs;
    NJS_VECTOR *normals;
    NJS_COLOR *vertcolor;
    NJS_COLOR *vertuv;
} NJS_MESHSET;

#define NJD_MESHSET_3 0x0000
#define NJD_MESHSET_4 0x4000
#define NJD_MESHSET_N 0x8000
#define NJD_MESHSET_TRIMESH 0xc000
#define NJD_MESHSET_MASK 0xc000
/* in a trimesh, the bit above a strip's count that reverses its winding */
#define NJD_TRIMESH_END 0x8000

typedef struct {
    NJS_POINT3 *points;
    NJS_VECTOR *normals;
    Sint32 nbPoint;
    NJS_MESHSET *meshsets;
    NJS_MATERIAL *mats;
    Uint16 nbMeshset;
    Uint16 nbMat;
    NJS_POINT3 center;
    Float r;
} NJS_MODEL;

/* a node of a model tree; pos, ang and scl place it in its parent */
typedef struct obj {
    Uint32 evalflags;
    NJS_MODEL *model;
    Float pos[3];
    Angle ang[3];
    Float scl[3];
    struct obj *child;
    struct obj *sibling;
} NJS_OBJECT;

/* evalflags bits. a UNIT_ bit leaves its part out of the node's matrix */
#define NJD_EVAL_UNIT_POS 0x01
#define NJD_EVAL_UNIT_ANG 0x02
#define NJD_EVAL_UNIT_SCL 0x04
#define NJD_EVAL_HIDE 0x08 /* hide the node's model */
#define NJD_EVAL_BREAK 0x10 /* skip the node's children */
#define NJD_EVAL_ZXY_ANG 0x20 /* turn as njRotateZXY does, not njRotateXYZ */
/* the node has no record in a motion, and under NJD_EVAL_SHAPE_SKIP none in a
 * shape motion */
#define NJD_EVAL_SKIP 0x40
#define NJD_EVAL_SHAPE_SKIP 0x80

typedef struct {
    void *mdata; /* the nodes' records, NJS_MDATA1 to 4 by the factor count */
    Uint32 nbFrame;
    Uint16 type;
    /* the factor count, and the interpolation in the NJD_MTYPE_MASK bits */
    Uint16 inp_fn;
} NJS_MOTION;
/* NJS_MOTION type bits: the factors the motion moves. a shape motion moves
 * points under NJD_MTYPE_VERT_4 and normals under NJD_MTYPE_NORM_5. a camera
 * motion moves its look by NJD_MTYPE_TARGET_3's point or, without it,
 * NJD_MTYPE_VEC_3's vector, its roll by NJD_MTYPE_ROLL_6 and its angle of view
 * by NJD_MTYPE_ANGLE_7. from NJD_MTYPE_POS_0 to NJD_MTYPE_NORM_5, a name's
 * number is its bit's. the look's two bits share 3. NJD_MTYPE_ROLL_6 and
 * NJD_MTYPE_ANGLE_7 count on from 5 */
#define NJD_MTYPE_POS_0 0x1
#define NJD_MTYPE_ANG_1 0x2
#define NJD_MTYPE_SCL_2 0x4
#define NJD_MTYPE_VEC_3 0x8
#define NJD_MTYPE_VERT_4 0x10
#define NJD_MTYPE_NORM_5 0x20
#define NJD_MTYPE_TARGET_3 0x40
#define NJD_MTYPE_ROLL_6 0x80
#define NJD_MTYPE_ANGLE_7 0x100
/* inp_fn's interpolation bits, all 0 for linear */
#define NJD_MTYPE_MASK 0xc0

/* motion keys: at keyframe, a position or scale, angles, or a pointer to more
 * data, such as a shape's points */
typedef struct {
    Uint32 keyframe;
    Float key[3];
} NJS_MKEY_F;
typedef struct {
    Uint32 keyframe;
    Angle key[3];
} NJS_MKEY_A;
typedef struct {
    Uint32 keyframe;
    void *key;
} NJS_MKEY_P;

/* a node's motion data: a pointer to each factor's keys, and the key counts */
typedef struct {
    void *p[1];
    Uint32 nb[1];
} NJS_MDATA1;
typedef struct {
    void *p[2];
    Uint32 nb[2];
} NJS_MDATA2;
typedef struct {
    void *p[3];
    Uint32 nb[3];
} NJS_MDATA3;
typedef struct {
    void *p[4];
    Uint32 nb[4];
} NJS_MDATA4;

typedef struct {
    NJS_OBJECT *object;
    NJS_MOTION *motion;
} NJS_ACTION;

/* two motions to blend between, each at its own frame */
typedef struct {
    NJS_MOTION *motion[2];
    Float frame[2];
} NJS_MOTION_LINK;
typedef struct {
    NJS_OBJECT *object;
    NJS_MOTION_LINK *motionlink;
} NJS_ACTION_LINK;

/* arrays of the position, angles and scale a motion gives each node of a tree
 * at a frame, three values per node */
typedef struct {
    Float *pos;
    Angle *ang;
    Float *scl;
} NJS_MOTION_DATA;

typedef struct {
    void *filename;
    Uint32 attr;
    Uint32 texaddr;
} NJS_TEXNAME;
typedef struct {
    NJS_TEXNAME *textures;
    Uint32 nbTexture;
} NJS_TEXLIST;

typedef struct {
    Uint32 Type;
    Uint32 BitDepth;
    Uint32 PixelFormat;
    Uint32 nWidth;
    Uint32 nHeight;
    Uint32 TextureSize;
    Uint32 fSurfaceFlags;
    Uint32 *pSurface;
    Uint32 *pVirtual;
    Uint32 *pPhysical;
} NJS_TEXSURFACE;
typedef struct {
    void *texaddr;
    NJS_TEXSURFACE texsurface;
} NJS_TEXINFO;
typedef struct {
    Uint32 globalIndex;
    Uint32 bank;
    Uint32 tspparambuffer;
    Uint32 texparambuffer;
    Uint32 texaddr;
    NJS_TEXINFO texinfo;
    Uint16 count;
    Uint16 dummy;
} NJS_TEXMEMLIST;
/* texaddr bits of an NJS_TEXMEMLIST record: the texture's pixels have a copy in
 * the texture cache, and one in video memory. TEXMEM_UNK_4 is a third flag
 * whose role the code doesn't show: no loader in 1ST_READ.BIN passes it to
 * njEntryTexMemList */
#define TEXMEM_CACHED 1
#define TEXMEM_LOADED 2
#define TEXMEM_UNK_4 4

/* a palette read from a file */
typedef struct {
    void *palette;
    Uint16 mode; /* the color format */
    Sint16 bank;
    Sint16 offset; /* the first entry it fills */
    Sint16 count;
} NJS_TEXPALETTE;

/* the last texture error */
typedef struct {
    Uint32 n;
    Uint32 globalIndex;
    Sint32 texerr; /* an NJD_TEXERR_ code */
    Sint32 gdstat;
    Sint32 gderr;
    Sint32 reserved0;
    Sint32 reserved1;
    Sint32 reserved2;
} NJS_TEXERRSTAT;
#define NJD_TEXERR_OTHER 1
#define NJD_TEXERR_FILEOPEN 2
#define NJD_TEXERR_EXTND 3
#define NJD_TEXERR_HEADER 4
#define NJD_TEXERR_FILELOAD 5
#define NJD_TEXERR_SURFACE 6
#define NJD_TEXERR_MAINMEMORY 7
#define NJD_TEXERR_TEXMEMLOAD 8
#define NJD_TEXERR_GLOBALINDEX 9

/* a sprite's region of a texture */
typedef struct {
    Sint16 sx, sy; /* size */
    Sint16 cx, cy; /* center */
    Sint16 u1, v1;
    Sint16 u2, v2;
    Sint16 texid;
    Sint16 attr;
} NJS_TEXANIM;
typedef struct {
    NJS_POINT3 p;
    Float sx, sy;
    Angle ang;
    NJS_TEXLIST *tlist;
    NJS_TEXANIM *tanim;
} NJS_SPRITE;

/* njDrawSprite2D and njDrawSprite3D flags */
#define NJD_SPRITE_ANGLE 0x01
#define NJD_SPRITE_COLOR 0x02
#define NJD_SPRITE_HFLIP 0x04
#define NJD_SPRITE_VFLIP 0x08
#define NJD_SPRITE_SCALE 0x10
#define NJD_SPRITE_ALPHA 0x20

typedef struct {
    NJS_POINT2 *p;
    NJS_COLOR *col;
    NJS_COLOR *tex;
    Uint32 num;
} NJS_POINT2COL;
typedef struct {
    NJS_POINT3 *p;
    NJS_COLOR *col;
    NJS_COLOR *tex;
    Uint32 num;
} NJS_POINT3COL;

/* a screen vertex of a polygon, for njDrawPolygon */
typedef struct {
    Float x, y, z;
    Uint32 col;
} NJS_POLYGON_VTX;

/* a screen vertex of a textured polygon, for njDrawTexture and
 * njDrawTextureMemList */
typedef struct {
    Float x, y, z;
    Float u, v;
    Uint32 col;
} NJS_TEXTURE_VTX;

#define NJD_PI 3.141592

/* a line or a plane: a point on it, and its direction or its normal */
typedef struct {
    Float px, py, pz;
    Float vx, vy, vz;
} NJS_LINE, NJS_PLANE;

typedef struct {
    NJS_POINT3 c;
    Float r;
} NJS_SPHERE;

/* an interpolation's parameters, which njLinear takes and doesn't read */
typedef struct {
    int keys;
    int *iparam;
    float *fparam;
} NJS_SPLINE;

/* the screen: the distance to it, its size and its center, in pixels */
typedef struct {
    Float dist;
    Float w, h;
    Float cx, cy;
} NJS_SCREEN;

#ifdef GCC
/* njSetView passes a view's matrix to njTranslate. GCC's njTranslate passes
 * it to SH4ZAM as an shz_mat4x4_t, which must be 8-byte aligned. pack(8)
 * lifts -fpack-struct=4's cap, so NJS_MATRIX's aligned(8) aligns the
 * structure, and the matrix at its start, to 8. no other member aligns past
 * 4, and the size is a multiple of 8, so no offset or size changes */
#pragma pack(push, 8)
#endif
/* a view: its matrix; its position, direction and roll; and its absolute
 * position, direction and roll, from which njRotateViewXAbsolute,
 * njRotateViewYAbsolute, njRotateViewZAbsolute and njTranslateViewAbsolute
 * set the others */
typedef struct {
    NJS_MATRIX m;
    Float px, py, pz;
    Float vx, vy, vz;
    Angle roll;
    Float apx, apy, apz;
    Float avx, avy, avz;
    Angle aroll;
} NJS_VIEW;
#ifdef GCC
#pragma pack(pop)
#endif

typedef struct {
    Float ratten, ipd;
    Float nrr, frr, cosi, cose, idev, odev;
    Float rate, intns;
    /* the specular power, as a step: 0 for the power 0, 1 to 16 for the powers
     * 0.5, 1, 1.5, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96 and 128, and any
     * other step for 128 */
    Int exp;
    Int reserve;
    NJS_POINT3 lpnt;
    NJS_VECTOR lvctr;
    NJS_VECTOR lmvctr;
    NJS_ARGB atten, amb, dif, spc;
    NJS_ARGB mamb, mdif, mspc;
} NJS_LIGHT_CAL;
typedef struct {
    Int lsrc; /* the light's type */
    Float iamb, idif, ispc;
    Float nrang, frang;
    void *func;
    Angle iang, oang;
    NJS_ARGB argb;
} NJS_LIGHT_ATTR;
#ifdef GCC
/* pack(8), as for NJS_VIEW: njTranslateLight passes a light's matrix to
 * njTranslate */
#pragma pack(push, 8)
#endif
typedef struct {
    NJS_MATRIX mtrx;
    NJS_POINT3 pnt;
    NJS_VECTOR vctr;
    Int stat;
    Int reserve;
    NJS_LIGHT_CAL ltcal; /* values Ninja derives */
    NJS_LIGHT_ATTR attr; /* values the caller sets */
} NJS_LIGHT;
#ifdef GCC
#pragma pack(pop)
#endif

typedef struct {
    NJS_ARGB ambient;
    NJS_ARGB diffuse;
    NJS_ARGB specular;
    Float exponent;
} NJS_LIGHT_MATERIAL;
#define NJD_LIGHT_MAX 32

typedef struct {
    Float px, py, pz;
    Float vx, vy, vz; /* the view direction */
    Angle roll;
    Angle ang; /* the angle of view */
    Float n_clip;
    Float f_clip;
    NJS_VECTOR lx, ly;
} NJS_CAMERA;
typedef struct {
    NJS_CAMERA *camera;
    NJS_MOTION *motion;
} NJS_CACTION;

typedef struct NJS_VERTEX_BUF NJS_VERTEX_BUF;

/* where a motion stands */
struct nj_motion_info {
    void *mdata; /* the next node's record */
    void **p; /* the node's keys, by factor */
    Uint32 *nb; /* the node's key counts, by factor */
    Float frame; /* the frame the motion is at */
    Uint32 nbFrame; /* the motion's frame count */
    Uint32 factor; /* the factor the next step takes */
    NJS_MATRIX *m; /* the matrix the steps move, NULL for the current one */
};
/* where a motion stands in a motion link's slot: nj_motion_info's fields, with
 * unk_18 in m's place */
struct nj_motlink_data_info {
    void *mdata;
    void **p;
    Uint32 *nb;
    Float frame;
    Uint32 nbFrame;
    Uint32 factor;
    Uint32 unk_18;
};
/* the vertex contexts Ninja keeps */
#define NJD_CONTEXT_MAX 6

#endif
