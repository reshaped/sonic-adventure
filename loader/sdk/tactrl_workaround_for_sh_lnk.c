/* SDK 1.20J Kamui's tile accelerator setup: each bank's layout and region
 * array, and the registers that point the tile accelerator at a bank */
#include <kamui.h>
#include <sg_xpt.h>

/* texture memory's 32-bit view, through the uncached area. kmiCopyToPVRMemory
 * writes the region arrays at offsets into the same view */
#define TEXTURE_MEMORY 0xa5000000UL

/* the video chip's registers, as the offsets kmiWriteRegister takes */
#define SOFT_RESET 0x008
#define TA_OL_BASE 0x124
#define TA_ISP_BASE 0x128
#define TA_OL_LIMIT 0x12c
#define TA_ISP_LIMIT 0x130
#define TA_GLOB_TILE_CLIP 0x13c
#define TA_ALLOC_CTRL 0x140
#define TA_LIST_INIT 0x144

/* the SOFT_RESET value that holds the tile accelerator in reset */
#define RESET_TA 1
/* the TA_LIST_INIT value that starts the lists afresh */
#define INIT_LISTS 0x80000000

/* TA_ALLOC_CTRL holds each list's object pointer block size code in the four
 * bits from ALLOC_SHIFT(list), list a ListType */
#define ALLOC_SHIFT(list) (4 * (list))
/* opb_downward's bit in TA_ALLOC_CTRL: bit 20 on core 6, past the
 * punch-through list's code, and bit 16 on any other core */
#define ALLOC_DOWNWARD_SHIFT_CORE6 20
#define ALLOC_DOWNWARD_SHIFT 16

/* the object pointer block size code the options word holds for list */
#define OPB_CODE(options, list)                                                \
    (((options) & (3UL << OPB_SHIFT(list))) >> OPB_SHIFT(list))

/* REGION_X and REGION_Y place a tile's x and y, 6 bits each, in its region
 * array entry's control word */
#define REGION_X(x) (((x) & 63) << 2)
#define REGION_Y(y) (((y) & 63) << 8)
/* the control word's bit for the array's last entry */
#define REGION_LAST_TILE_SHIFT 31
#define REGION_LAST_TILE (1UL << REGION_LAST_TILE_SHIFT)
/* the control word's bit that leaves the tile's depths uncleared before it
 * draws */
#define REGION_KEEP_Z 0x40000000
/* the control word's bit that draws translucent polygons in the order sent,
 * unsorted */
#define REGION_PRESORT_SHIFT 29
#define REGION_PRESORT (1UL << REGION_PRESORT_SHIFT)
/* the control word's bit that keeps the tile from being written to the frame
 * buffer after it draws */
#define REGION_NO_FLUSH 0x10000000
/* a region array entry's bit for a list the tile doesn't draw: the whole list
 * word, or ORed into the list's start */
#define LIST_EMPTY 0x80000000

/* a tile's entry in the region array: its control word, then where each list's
 * object pointer blocks for the tile start, in ListType order */
struct region_tile {
    KMDWORD control;
    KMDWORD opaque;
    KMDWORD opaque_modifier;
    KMDWORD translucent;
    KMDWORD translucent_modifier;
    KMDWORD punchthrough;
};

/* the dummy translucent triangle's parameters: its ISP, TSP and texture words,
 * then its vertices */
struct dummy_triangle {
    KMDWORD isp;
    KMDWORD tsp;
    KMDWORD texture;
    struct {
        KMFLOAT x;
        KMFLOAT y;
        KMFLOAT z;
        KMDWORD color;
    } vertex[3];
};

/* the dummy triangle's ISP word: Gouraud shading, and a depth compare mode of
 * 0, which no DepthMode value names */
#define DUMMY_ISP 0x00800000
/* the dummy triangle's TSP word: blend factors one and one, no fog, and the
 * vertices' alpha */
#define DUMMY_TSP 0x24900000

/* the dummy translucent object: an object list of one triangle, then the
 * triangle */
struct dummy_object {
    KMDWORD list[2];
    struct dummy_triangle triangle;
};

/* the dummy object's object list. its first word is an entry for one triangle
 * whose vertices each hold a word past x, y and z. the entry's low bits hold
 * the triangle's address. its second word ends the list */
#define OL_ONE_TRIANGLE 0x80200000
#define OL_END 0xf0000000

/* the words in an object pointer block, by its size code */
static KMDWORD opb_words[4] = {0, 8, 16, 32};
/* when set, each tile's region array entry gives the punch-through list the
 * translucent list's object pointer blocks */
KMBOOLEAN gkmCopyTransToPunchthrough = 0;

/* the bytes the first object pointer blocks take for a frame of tiles tiles:
 * one block of each list's size for each tile */
static KMDWORD opb_bytes(KMDWORD tiles)
{
    KMDWORD bytes;

    /* @hack spelling: the product kept in bytes for the return. SHC sums it in
     * R2 and returns it through R4 (MOV R2,R4 at 0x8ce1cc74, MOV R4,R0 at
     * 0x8ce1cc7a). as one return expression, it sums in R0, the function runs
     * 4 bytes long, and 27 of 30 words differ */
    bytes = (opb_words[kmiDeviceExtension.opaque_opb] +
             opb_words[kmiDeviceExtension.opaque_modifier_opb] +
             opb_words[kmiDeviceExtension.translucent_opb] +
             opb_words[kmiDeviceExtension.translucent_modifier_opb] +
             opb_words[kmiDeviceExtension.punchthrough_opb]) *
            tiles * 4;
    return bytes;
}

/* set the tile accelerator up for bank and a frame of width by height tiles,
 * then initialize its lists */
static void set_ta_registers(KMDWORD width, KMDWORD height, KMDWORD bank)
{
    KMDWORD sizes;

    kmiWriteRegister(TA_OL_BASE, kmiDeviceExtension.ta_bank[bank].ol_base);
    kmiWriteRegister(TA_OL_LIMIT, kmiDeviceExtension.ta_bank[bank].ol_limit);
    kmiWriteRegister(TA_ISP_BASE, kmiDeviceExtension.ta_bank[bank].isp_base);
    kmiWriteRegister(TA_ISP_LIMIT, kmiDeviceExtension.ta_bank[bank].isp_limit);
    kmiWriteRegister(TA_GLOB_TILE_CLIP, ((height - 1) << 16) + width - 1);
    sizes = ((kmiDeviceExtension.opaque_opb & 3)
             << ALLOC_SHIFT(KM_OPAQUE_POLYGON)) |
            ((kmiDeviceExtension.opaque_modifier_opb & 3)
             << ALLOC_SHIFT(KM_OPAQUE_MODIFIER)) |
            ((kmiDeviceExtension.translucent_opb & 3)
             << ALLOC_SHIFT(KM_TRANS_POLYGON)) |
            ((kmiDeviceExtension.translucent_modifier_opb & 3)
             << ALLOC_SHIFT(KM_TRANS_MODIFIER));
    /* only core 6 gets the punch-through list's size code */
    if (kmiDeviceExtension.core == 6) {
        sizes |= (kmiDeviceExtension.opb_downward & 1)
                 << ALLOC_DOWNWARD_SHIFT_CORE6;
        sizes |= (kmiDeviceExtension.punchthrough_opb & 3)
                 << ALLOC_SHIFT(KM_PUNCHTHROUGH_POLYGON);
    } else {
        sizes |= (kmiDeviceExtension.opb_downward & 1) << ALLOC_DOWNWARD_SHIFT;
    }
    kmiWriteRegister(TA_ALLOC_CTRL, sizes);
    kmiWriteRegister(TA_LIST_INIT, INIT_LISTS);
}

/* write the dummy translucent object at offset in texture memory, and return
 * offset. its object list gives the triangle's address in words from base,
 * the byte offset where the tile accelerator's ISP parameters start */
KMDWORD kmiCreateDummyTranslucentObject(KMDWORD offset, KMDWORD base)
{
    /* @hack spelling: one cursor, which steps from the list to the triangle
     * (ADD #8,R6 at 0x8ce1cd96). through a pointer to the whole object, the
     * triangle's stores take displacements from 8, and 34 of 51 words
     * differ */
    union {
        KMDWORD list[2];
        struct dummy_triangle triangle;
    } *at = (void *)(TEXTURE_MEMORY | offset);

    at->list[0] = (((offset - base) >> 2) + 2) | OL_ONE_TRIANGLE;
    at->list[1] = OL_END;
    at = (void *)((KMDWORD *)at + 2);
    at->triangle.isp = DUMMY_ISP;
    at->triangle.tsp = DUMMY_TSP;
    at->triangle.texture = 0;
    at->triangle.vertex[0].x = 0.0F;
    at->triangle.vertex[0].y = 0.0F;
    at->triangle.vertex[0].z = 1.0e-10F;
    at->triangle.vertex[0].color = 0x80ff0000;
    at->triangle.vertex[1].x = 0.0F;
    at->triangle.vertex[1].y = 1280.0F;
    at->triangle.vertex[1].z = 1.0e-10F;
    at->triangle.vertex[1].color = 0x8000ff00;
    at->triangle.vertex[2].x = 2560.0F;
    at->triangle.vertex[2].y = 0.0F;
    at->triangle.vertex[2].z = 1.0e-10F;
    at->triangle.vertex[2].color = 0x800000ff;
    return offset;
}

void kmiCreateRegionArray(KMDWORD width, KMDWORD height, KMDWORD index,
                          KMINT32 unused, KMINT32 bank, KMDWORD order)
{
    KMDWORD tiles;
    KMDWORD array_bytes;
    KMDWORD dummy_list;
    char *destination;
    char *cursor;
    KMDWORD presort;
    KMDWORD opaque_stride;
    KMDWORD opaque_modifier_stride;
    KMDWORD translucent_stride;
    KMDWORD translucent_modifier_stride;
    KMDWORD punchthrough_stride;
    KMDWORD opaque_start;
    KMDWORD opaque_modifier_start;
    KMDWORD translucent_start;
    KMDWORD translucent_modifier_start;
    KMDWORD punchthrough_start;
    KMDWORD opaque_dummy = 0;
    KMDWORD opaque_empty;
    KMDWORD opaque_modifier_empty;
    KMDWORD translucent_empty;
    KMDWORD translucent_modifier_empty;
    KMDWORD punchthrough_empty;
    KMDWORD last;
    KMDWORD presort_bit;
    KMDWORD i;
    KMDWORD tile;
    KMDWORD clipped;
    KMDWORD x;
    /* @hack type: y as int, which SHC copies to R0 to mask for the control
     * word (AND #63,R0 at 0x8ce1d0a2). as KMDWORD, it masks y in place, the
     * function runs 4 bytes short, and 360 of 638 words differ */
    int y;
    struct region_tile first;
    struct region_tile second;
    struct region_tile third;
    struct region_tile fourth;

#ifdef GCC
    /* GCC can't see that these are set wherever they are read. the tile loop
     * reads dummy_list only under the two tests that set it, which nothing
     * between changes, and each caller's order is BY_COLUMNS or BY_ROWS, which
     * the switch that sets x and y names */
    dummy_list = LIST_EMPTY;
    x = 0;
    y = 0;
#endif
    /* @hack spelling: presort set to 1 or 0 in an if, which SHC keeps in R14
     * (MOVT R14 at 0x8ce1ce0a). as one assignment, it takes R13, and 26 of 638
     * words differ */
    if (kmiDeviceExtension.presort == TRUE) {
        presort = 1;
    } else {
        presort = 0;
    }
    if (!kmiDeviceExtension.ta_busy) {
        set_ta_registers(width, height, bank);
    }
    tiles = width * height;
    array_bytes = tiles * sizeof(struct region_tile);
    destination = (char *)kmiDeviceExtension.ta_bank[bank].region_base +
                  (index * array_bytes);
    if ((kmiDeviceExtension.capabilities & KMI_CAP_TWO_ENTRIES) &&
        kmiDeviceExtension.dummy_translucent) {
        dummy_list = kmiCreateDummyTranslucentObject(
            kmiDeviceExtension.ta_bank[bank].region_base + (array_bytes * 4),
            kmiDeviceExtension.ta_bank[bank].start);
        opaque_dummy = kmiDeviceExtension.opaque_opb == 0 &&
                       kmiDeviceExtension.translucent_opb != 0 && presort;
    }
    if (kmiDeviceExtension.opaque_opb == 0) {
        opaque_empty = LIST_EMPTY;
    } else {
        opaque_empty = 0;
    }
    if (kmiDeviceExtension.opaque_modifier_opb == 0) {
        opaque_modifier_empty = LIST_EMPTY;
    } else {
        opaque_modifier_empty = 0;
    }
    if (kmiDeviceExtension.translucent_opb == 0) {
        translucent_empty = LIST_EMPTY;
    } else {
        translucent_empty = 0;
    }
    if (kmiDeviceExtension.translucent_modifier_opb == 0) {
        translucent_modifier_empty = LIST_EMPTY;
    } else {
        translucent_modifier_empty = 0;
    }
    if (kmiDeviceExtension.punchthrough_opb == 0) {
        punchthrough_empty = LIST_EMPTY;
    } else {
        punchthrough_empty = 0;
    }
    opaque_stride = opb_words[kmiDeviceExtension.opaque_opb] * 4;
    opaque_modifier_stride =
        opb_words[kmiDeviceExtension.opaque_modifier_opb] * 4;
    translucent_stride = opb_words[kmiDeviceExtension.translucent_opb] * 4;
    translucent_modifier_stride =
        opb_words[kmiDeviceExtension.translucent_modifier_opb] * 4;
    punchthrough_stride = opb_words[kmiDeviceExtension.punchthrough_opb] * 4;
    /* each list has one first block for each tile. the opaque list's blocks
     * start at the bank's object list base, and each other list's blocks where
     * those of the list before it end */
    opaque_start = kmiDeviceExtension.ta_bank[bank].ol_base;
    opaque_modifier_start =
        opaque_start + (opb_words[kmiDeviceExtension.opaque_opb] * tiles * 4);
    translucent_start =
        opaque_modifier_start +
        (opb_words[kmiDeviceExtension.opaque_modifier_opb] * tiles * 4);
    translucent_modifier_start =
        translucent_start +
        (opb_words[kmiDeviceExtension.translucent_opb] * tiles * 4);
    punchthrough_start =
        translucent_modifier_start +
        (opb_words[kmiDeviceExtension.translucent_modifier_opb] * tiles * 4);
    last = tiles - 1;
    presort_bit = (presort & 1) << REGION_PRESORT_SHIFT;
    /* @hack redundant: a never-run store to gkmCopyTransToPunchthrough, which
     * keeps width at @R15, height at @(4,R15) and opaque_empty at @(68,R15)
     * (0x8ce1cdfc, 0x8ce1ceee). without it, they and the other lists' empty
     * flags and starts take other slots, and 27 of 638 words differ */
    if (0) {
        gkmCopyTransToPunchthrough = 0;
    }
    for (i = 0; i < tiles; i++) {
        /* @hack redundant: two never-run stores, which keep opaque_start at
         * @(32,R15) and opaque_modifier_start at @(28,R15) (0x8ce1cf86,
         * 0x8ce1cf9e). with opaque_start's alone, opaque_empty and
         * translucent_empty trade @(68,R15) and @(72,R15) instead, and 5
         * words differ. with destination's alone, 12 differ. without them, the
         * two starts trade slots, and 5 of 638 words differ */
        if (0) {
            destination = 0;
            opaque_start = 0;
        }
        clipped = 0;
        switch (order) {
        case BY_COLUMNS:
            x = i / kmiDeviceExtension.tiles_y;
            y = i % kmiDeviceExtension.tiles_y;
            break;
        case BY_ROWS:
            x = i % kmiDeviceExtension.tiles_x;
            y = i / kmiDeviceExtension.tiles_x;
            break;
        }
        if (kmiDeviceExtension.global_clip == TRUE &&
            (kmiDeviceExtension.global_clip_width < x ||
             kmiDeviceExtension.global_clip_height < (KMDWORD)y)) {
            clipped = LIST_EMPTY;
        }
        tile = (y * kmiDeviceExtension.tiles_x) + x;
        first.control = ((KMDWORD)(i == last) << REGION_LAST_TILE_SHIFT) |
                        REGION_Y(y) | REGION_X(x) | presort_bit;
        first.opaque =
            ((opaque_stride * tile) + opaque_start) | opaque_empty | clipped;
        first.opaque_modifier =
            ((opaque_modifier_stride * tile) + opaque_modifier_start) |
            opaque_modifier_empty | clipped;
        first.translucent = ((translucent_stride * tile) + translucent_start) |
                            translucent_empty | clipped;
        first.translucent_modifier = ((translucent_modifier_stride * tile) +
                                      translucent_modifier_start) |
                                     translucent_modifier_empty | clipped;
        if (gkmCopyTransToPunchthrough) {
            first.punchthrough = first.translucent;
        } else {
            first.punchthrough =
                ((punchthrough_stride * tile) + punchthrough_start) |
                punchthrough_empty | clipped;
        }
        if (kmiDeviceExtension.another_modifier) {
            if (kmiDeviceExtension.another_modifier_list == 1) {
                first.translucent_modifier = first.opaque_modifier;
            } else {
                first.opaque_modifier = first.translucent_modifier;
            }
        }
        /* @hack redundant: the else after the continue, which has the plain
         * copy take destination in R1 and i in R2 (0x8ce1d398, 0x8ce1d39e).
         * without it, they take R2 and R3, and 4 of 638 words differ */
        if (kmiDeviceExtension.capabilities & KMI_CAP_TWO_ENTRIES) {
            if (kmiDeviceExtension.dummy_translucent) {
                /* the tile's first entry draws its opaque lists and its
                 * punch-through list. the second entry draws the dummy
                 * translucent object. the third draws the tile's translucent
                 * lists. the fourth draws nothing and writes the tile out */
                /* @hack spelling: the three controls set in one chain, from one
                 * read of first.control into R4 at 0x8ce1d1b6 for the stores
                 * at 0x8ce1d1be to 0x8ce1d1ca. as three statements, the second
                 * and third read it again, the function runs 8 bytes long, and
                 * 233 of 638 words differ */
                fourth.control = third.control = second.control = first.control;
                first.control &= ~REGION_LAST_TILE;
                second.control &= ~REGION_LAST_TILE;
                third.control &= ~REGION_LAST_TILE;
                first.control &= ~REGION_KEEP_Z;
                second.control |= REGION_KEEP_Z;
                third.control |= REGION_KEEP_Z;
                fourth.control |= REGION_KEEP_Z;
                first.control |= REGION_NO_FLUSH;
                second.control |= REGION_NO_FLUSH;
                third.control |= REGION_NO_FLUSH;
                fourth.control &= ~REGION_NO_FLUSH;
                second.control &= ~REGION_PRESORT;
                if (opaque_dummy) {
                    first.opaque = dummy_list;
                }
                second.opaque = LIST_EMPTY;
                second.opaque_modifier = LIST_EMPTY;
                second.translucent = dummy_list;
                second.translucent_modifier = LIST_EMPTY;
                second.punchthrough = LIST_EMPTY;
                third.opaque = LIST_EMPTY;
                third.opaque_modifier = LIST_EMPTY;
                third.translucent = first.translucent;
                third.translucent_modifier = first.translucent_modifier;
                third.punchthrough = LIST_EMPTY;
                first.translucent = LIST_EMPTY;
                first.translucent_modifier = LIST_EMPTY;
                fourth.opaque = LIST_EMPTY;
                fourth.opaque_modifier = LIST_EMPTY;
                fourth.translucent = LIST_EMPTY;
                /* @hack spelling: fourth's last two lists set in one chain,
                 * which keeps opaque_dummy at @(88,R15) (0x8ce1ce02). as two
                 * stores, it leaves that slot, and 80 of 638 words differ */
                fourth.punchthrough = fourth.translucent_modifier = LIST_EMPTY;
                cursor = destination + (i * sizeof(struct region_tile) * 4);
                kmiCopyToPVRMemory(cursor, &first, sizeof first);
                kmiCopyToPVRMemory(cursor + sizeof first, &second,
                                   sizeof second);
                kmiCopyToPVRMemory(cursor + (sizeof first * 2), &third,
                                   sizeof third);
                kmiCopyToPVRMemory(cursor + (sizeof first * 3), &fourth,
                                   sizeof fourth);
            } else {
                /* the tile's first entry draws its lists, and the second
                 * draws nothing and writes the tile out */
                second.control = first.control;
                first.control &= ~REGION_LAST_TILE;
                first.control &= ~REGION_KEEP_Z;
                first.control |= REGION_NO_FLUSH;
                second.control |= REGION_KEEP_Z;
                second.control &= ~REGION_NO_FLUSH;
                second.opaque = LIST_EMPTY;
                second.opaque_modifier = LIST_EMPTY;
                second.translucent = LIST_EMPTY;
                second.translucent_modifier = LIST_EMPTY;
                second.punchthrough = LIST_EMPTY;
                cursor = destination + (i * sizeof(struct region_tile) * 2);
                kmiCopyToPVRMemory(cursor, &first, sizeof first);
                kmiCopyToPVRMemory(cursor + sizeof first, &second,
                                   sizeof second);
            }
            /* @hack redundant: a continue where the loop goes on anyway, which
             * has the loop entered at its test (BRA at 0x8ce1cff0). without it,
             * SHC tests for no tiles ahead of the loop as well, the section
             * runs 28 bytes long, and 558 of 638 words differ */
            continue;
        } else {
            cursor = destination + (i * sizeof(struct region_tile));
            kmiCopyToPVRMemory(cursor, &first, sizeof first);
        }
    }
}

void kmiInitializeTA(void)
{
    kmiResetTA();
    kmiInitializeDMA();
    kmiDeviceExtension.unk_440 = 0x00800000;
    kmiDeviceExtension.unk_448 = 0x00600000;
    kmiDeviceExtension.unk_44c = kmiDeviceExtension.unk_448;
    kmiDeviceExtension.ta_bank[0].start = kmiDeviceExtension.unk_440 >> 1;
    kmiDeviceExtension.ta_bank[1].start = 0;
    kmiDeviceExtension.opb_downward = 1;
}

void kmiSetTAEnvironment(KMDWORD region_array, KMDWORD object_list,
                         KMDWORD isp_params, KMDWORD tsp_params,
                         KMDWORD options, KMDWORD unk)
{
    KMDWORD order;
    KMDWORD bytes;
    int bank;

    kmiDeviceExtension.ta_options = options;
    if (options & OPTION_BY_COLUMNS) {
        order = BY_COLUMNS;
    } else {
        order = BY_ROWS;
    }
    kmiDeviceExtension.opb_downward =
        (options & OPB_DOWNWARD) >> OPB_DOWNWARD_SHIFT;
    kmiDeviceExtension.opaque_opb = OPB_CODE(options, KM_OPAQUE_POLYGON);
    kmiDeviceExtension.opaque_modifier_opb =
        OPB_CODE(options, KM_OPAQUE_MODIFIER);
    kmiDeviceExtension.translucent_opb = OPB_CODE(options, KM_TRANS_POLYGON);
    kmiDeviceExtension.translucent_modifier_opb =
        OPB_CODE(options, KM_TRANS_MODIFIER);
    kmiDeviceExtension.punchthrough_opb =
        OPB_CODE(options, KM_PUNCHTHROUGH_POLYGON);
    kmiDeviceExtension.isp_bytes = (kmiDeviceExtension.ta_buffer_size >> 2) * 3;
    bytes = opb_bytes(kmiDeviceExtension.tiles_x * kmiDeviceExtension.tiles_y);
    for (bank = 0; bank < 2; bank++) {
        /* the bank ends with its region array, one entry a tile. on a chip with
         * KMI_CAP_TWO_ENTRIES, each tile takes two entries. with
         * dummy_translucent set, each tile takes four, and the dummy
         * translucent object follows the array */
        kmiDeviceExtension.ta_bank[bank].region_base =
            kmiDeviceExtension.ta_bank[bank].start +
            kmiDeviceExtension.ta_buffer_size;
        if (kmiDeviceExtension.capabilities & KMI_CAP_TWO_ENTRIES) {
            if (kmiDeviceExtension.dummy_translucent) {
                kmiDeviceExtension.ta_bank[bank].region_base -=
                    sizeof(struct dummy_object);
                kmiDeviceExtension.ta_bank[bank].region_base -=
                    kmiDeviceExtension.tiles_x * kmiDeviceExtension.tiles_y *
                    sizeof(struct region_tile) * 4;
            } else {
                kmiDeviceExtension.ta_bank[bank].region_base -=
                    kmiDeviceExtension.tiles_x * kmiDeviceExtension.tiles_y *
                    sizeof(struct region_tile) * 2;
            }
        } else {
            kmiDeviceExtension.ta_bank[bank].region_base -=
                kmiDeviceExtension.tiles_x * kmiDeviceExtension.tiles_y *
                sizeof(struct region_tile);
        }
        /* @hack redundant: a loop of one pass, which has R4 set from bank
         * (MOV R11,R4 at 0x8ce1d4da) ahead of the bank loop's constants 2
         * and -128. without it, R4 is set after them, and 3 of 246 words
         * differ */
        do {
            /* the first object pointer blocks start isp_bytes into the bank,
             * past the ISP parameters. the tile accelerator allocates further
             * blocks above them, up to ol_limit. with opb_downward, the first
             * blocks end just below the region array, and further blocks run
             * downward to ol_limit */
            if (!kmiDeviceExtension.opb_downward) {
                kmiDeviceExtension.ta_bank[bank].ol_base =
                    (kmiDeviceExtension.ta_bank[bank].start +
                     kmiDeviceExtension.isp_bytes) &
                    ~31UL;
                kmiDeviceExtension.ta_bank[bank].ol_limit =
                    (kmiDeviceExtension.ta_bank[bank].region_base - bytes) &
                    ~127UL;
                kmiDeviceExtension.ta_bank[bank].next_opb_init =
                    kmiDeviceExtension.ta_bank[bank].ol_base + bytes;
            } else {
                kmiDeviceExtension.ta_bank[bank].ol_base =
                    (kmiDeviceExtension.ta_bank[bank].region_base - bytes) &
                    ~127UL;
                kmiDeviceExtension.ta_bank[bank].ol_limit =
                    (kmiDeviceExtension.ta_bank[bank].start +
                     kmiDeviceExtension.isp_bytes) &
                    ~31UL;
                kmiDeviceExtension.ta_bank[bank].next_opb_init =
                    kmiDeviceExtension.ta_bank[bank].ol_base;
            }
        } while (0);
        kmiDeviceExtension.ta_bank[bank].isp_base =
            kmiDeviceExtension.ta_bank[bank].start;
        kmiDeviceExtension.ta_bank[bank].isp_limit =
            kmiDeviceExtension.ta_bank[bank].start +
            kmiDeviceExtension.isp_bytes - 256;
    }
    kmiCreateRegionArray(kmiDeviceExtension.tiles_x, kmiDeviceExtension.tiles_y,
                         0, 1, 0, order);
    kmiCreateRegionArray(kmiDeviceExtension.tiles_x, kmiDeviceExtension.tiles_y,
                         0, 1, 1, order);
}

void kmiResetTA(void)
{
    kmiWriteRegister(SOFT_RESET, RESET_TA);
    kmiWriteRegister(SOFT_RESET, 0);
}

void kmiFastTA(void)
{
}

void kmiFastTADirect(void)
{
}

void kmiVertexTransferTADirect(void)
{
}
