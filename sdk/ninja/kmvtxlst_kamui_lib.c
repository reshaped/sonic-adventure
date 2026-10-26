/* Kamui's vertex list writes: a strip's global parameter, a vertex and a
 * user clip, each sent to its list through the store queues */
#include <kamui.h>
#include <machine.h>

/* the store queue address control registers, one for each queue. each
 * holds in bits 2 to 4 the memory area its queue writes to, bits 26 to 28 of
 * the address */
#define QACR0 (*(volatile KMDWORD *)0xff000038)
#define QACR1 (*(volatile KMDWORD *)0xff00003c)
/* the store queues' area. a write to STORE_QUEUE_BASE | (address &
 * STORE_QUEUE_MASK) goes into a queue. a prefetch then writes the queue to
 * address, in the area its QACR register holds */
#define STORE_QUEUE_BASE 0xe0000000
#define STORE_QUEUE_MASK 0x03ffffff

/* the parameter control word's bits. the top three give the parameter's type:
 * a polygon or modifier volume header, a sprite header, or a user clip */
#define PCW_POLYGON 0x80000000
#define PCW_SPRITE 0xa0000000
#define PCW_USER_CLIP 0x20000000
#define PCW_MODIFIER_LIST 0x01000000
#define PCW_GROUP 0x00800000
#define PCW_CLIP 0x00030000
#define PCW_VOLUMES 0x00000040

/* the ListType bit of the two modifier volume lists, KM_OPAQUE_MODIFIER and
 * KM_TRANS_MODIFIER */
#define LIST_MODIFIER 1

/* the ISP word's offset color bit */
#define ISP_OFFSET 0x01000000
/* blank_header's TSP word: no fog, a D adjust of KM_MIPMAP_D_ADJUST_1_00, and
 * KM_MODULATE texture shading */
#define BLANK_TSP                                                              \
    (((KMDWORD)KM_NOFOG << 22) | ((KMDWORD)KM_MIPMAP_D_ADJUST_1_00 << 8) |     \
     ((KMDWORD)KM_MODULATE << 6))

/* send size bytes from source to list's write pointer through the store
 * queues, and advance the pointer past them. a queue holds 32 bytes, and a
 * prefetch writes it out. SEND writes out the second queue only when size is
 * 64, so size must be 32 or 64 */
#define SEND(desc, list, source, size)                                         \
    do {                                                                       \
        KMDWORD *sq = (desc)->pCurrentPtr[list];                               \
        PKMDWORD from = (PKMDWORD)(source);                                    \
        QACR0 = (KMDWORD)sq >> 24;                                             \
        QACR1 = (KMDWORD)sq >> 24;                                             \
        sq = (KMDWORD *)(((KMDWORD)sq & STORE_QUEUE_MASK) | STORE_QUEUE_BASE); \
        kmiMemCopy8(sq, from, size);                                           \
        prefetch(sq);                                                          \
        if ((size) == 64) {                                                    \
            prefetch(sq + 8);                                                  \
        }                                                                      \
        (desc)->pCurrentPtr[list] += (size) >> 2;                              \
    } while (0)
/* @hack spelling: SEND for 64 bytes, without SEND's test of the size. it
 * keeps STORE_QUEUE_MASK in R9 and STORE_QUEUE_BASE in R8 (1ST_READ.BIN
 * 0x8c65fb46 to 0x8c65fb48, LOADER.BIN 0x8ce13c8c to 0x8ce13c90). as
 * SEND(..., 64), the two trade registers, as do Xmin's and Ymin's stack
 * words, and 12 of kmSetUserClipping's 136 words differ in each image */
#define SEND64(desc, list, source)                                             \
    do {                                                                       \
        KMDWORD *sq = (desc)->pCurrentPtr[list];                               \
        PKMDWORD from = (PKMDWORD)(source);                                    \
        QACR0 = (KMDWORD)sq >> 24;                                             \
        QACR1 = (KMDWORD)sq >> 24;                                             \
        sq = (KMDWORD *)(((KMDWORD)sq & STORE_QUEUE_MASK) | STORE_QUEUE_BASE); \
        kmiMemCopy8(sq, from, 64);                                             \
        prefetch(sq);                                                          \
        prefetch(sq + 8);                                                      \
        (desc)->pCurrentPtr[list] += 64 >> 2;                                  \
    } while (0)

/* the header kmSetUserClipping sends ahead of two blank_sprite copies.
 * kmSetUserClipping sets its control word for the list */
static KMDWORD blank_header[8] = {
    0xffffffff, ISP_OFFSET, BLANK_TSP, 0, 0, 0, 0xffffffff, 0xffffffff,
};
/* blank_sprite's z values initialize unions without braces, which SHC rejects
 * there. SHC 5.0's R10 and R32 releases round a float literal toward zero, so
 * 0.100000002F spells the float nearest 0.1 */
/* NOLINTBEGIN(clang-diagnostic-missing-braces) */
/* a sprite off the screen, above and left of its top left corner: x runs from
 * -1.5 to -0.5, and y from -1 to -0.5 */
static KMVERTEX16 blank_sprite = {
    KM_VERTEXPARAM_ENDOFSTRIP,
    -1.5F,
    -1.0F,
    0.100000002F,
    -0.5F,
    -1.0F,
    0.100000002F,
    -0.5F,
    -0.5F,
    0.100000002F,
    -1.5F,
    -0.5F,
    0xffffffff,
    0xffffffff,
    0xffffffff,
    0xffffffff,
};
/* NOLINTEND(clang-diagnostic-missing-braces) */

KMSTATUS kmStartVertexStrip(PKMVERTEXBUFFDESC pBufferDesc)
{
    KMDWORD list = pBufferDesc->pCurrentListState->ListType;
    KMDWORD size = pBufferDesc->pCurrentListState->GlobalParamSize;

    SEND(pBufferDesc, list, pBufferDesc->pGlobalParam, size);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetVertex(PKMVERTEXBUFFDESC pBufferDesc, void *pVertex,
                     KMVERTEXTYPE VertexType, KMINT32 StructSize)
{
    KMDWORD list;

    kmiCurrentListState.Reserved0 = VertexType;
    list = pBufferDesc->pCurrentListState->ListType;
    SEND(pBufferDesc, list, pVertex, StructSize);
    return KMSTATUS_SUCCESS;
}

KMSTATUS kmSetUserClipping(PKMVERTEXBUFFDESC pBufferDesc, KMINT32 ListType,
                           KMINT32 Xmin, KMINT32 Ymin, KMINT32 Xmax,
                           KMINT32 Ymax)
{
    /* the user clip's control word, three words left unwritten, then the
     * bounds in tiles */
    KMDWORD clip[8];
    int i;

    if (kmGetCurrentVertexOffset(ListType) != LIST_HEADER_WORDS) {
        /* @hack spelling: the global parameter's control word, the header
         * and the mask each in a local. SHC then loads and masks the control
         * word once, before the branch on the list type (1ST_READ.BIN
         * 0x8c65fb5c to 0x8c65fb6c, LOADER.BIN 0x8ce13ca0 to 0x8ce13cb0).
         * folded into the stores, each arm loads and masks it. 116 of
         * kmSetUserClipping's 136 words then differ in 1ST_READ.BIN, and 114
         * of 136 in LOADER.BIN, where it runs 4 bytes long */
        KMDWORD global = pBufferDesc->pGlobalParam[0];
        KMDWORD *header = blank_header;
        KMDWORD control = PCW_GROUP | PCW_CLIP;

        control &= global;
        if (ListType & LIST_MODIFIER) {
            header[0] =
                control | (PCW_POLYGON | PCW_MODIFIER_LIST | PCW_VOLUMES);
        } else {
            header[0] = control | PCW_SPRITE;
        }
        SEND(pBufferDesc, ListType, blank_header, sizeof(blank_header));
        for (i = 0; i < 2; i++) {
            SEND64(pBufferDesc, ListType, &blank_sprite);
        }
    }
    clip[0] = PCW_USER_CLIP;
    clip[4] = Xmin;
    clip[5] = Ymin;
    clip[6] = Xmax;
    clip[7] = Ymax;
    /* @bug uninitialized: no path writes clip[1] to clip[3]. the 32-byte copy
     * that sends clip reads them (JSR @R3 at 0x8c65fc16 in 1ST_READ.BIN and
     * 0x8ce13d5a in LOADER.BIN, with R5 at clip) */
    SEND(pBufferDesc, ListType, clip, sizeof(clip));
    return KMSTATUS_SUCCESS;
}
