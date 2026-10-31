/* sprites through the current matrix: an entry of an NJS_SPRITE's texture
 * animation, drawn as vertices written straight into Kamui's vertex buffer.
 * make gcc compiles this in place of src/draw/sprite.src. an entry without
 * NJD_SPRITE_SCALE goes by njSprite.c's sprite_send_plane, in place of
 * sprite.src's sprite_send_flat and sprite_send_clipped.
 *
 * sprite_draw_3d maps a list's write pointer into the store queue area and
 * leaves room there for the strip's polygon header. a send writes the strip's
 * vertices after that room. when a strip is sent, the draw writes its header
 * into the room with sprite_begin, and njEndVertex ends it */
#include <kamui.h>
#include <ninja.h>
#include <sg_xpt.h>

#include "draw/sprite.h"
#include "gcc/sdk/ninja/njsprite.h"
#include "sdk/ninja/njkamui.h"
#include "system/frame.h"

/* the store queues. once QACR0 and QACR1 hold SQ_AREA(address), a PREF of a
 * queue filled at SQ_BASE | (address & SQ_ADDRESS) sends it to address.
 * SQ_AREA moves address's bits 26 to 28 to bits 2 to 4, where the registers
 * take them */
#define QACR0 (*(volatile Uint32 *)0xff000038)
#define QACR1 (*(volatile Uint32 *)0xff00003c)
#define SQ_AREA(address) (((address) >> 24) & 0x1c)
#define SQ_ADDRESS 0x03ffffff
#define SQ_BASE 0xe0000000

static Float sprite_depth;

void sprite_set_depth(Float depth)
{
    sprite_depth = depth;
}

Float sprite_get_depth(void)
{
    return sprite_depth;
}

/* send entry facing the screen at sprite->p, through the current matrix, as
 * sprite_send_facing_at does. its z is the point's z plus the depth
 * sprite_set_depth last set */
static __attribute__((noinline)) struct sprite_vertex *
sprite_send_facing(const NJS_SPRITE *sprite, Int entry, Uint32 attr,
                   struct sprite_vertex *sq)
{
    shz_vec3_t view = shz_xmtrx_transform_point3(zam_load_vec3(&sprite->p));

    /* @bug logic: the corners' y scales by sprite->sx, where
     * sprite_send_plane's scales by sprite->sy. sprite.src loads the y scale
     * at 0x8c0a442c while R0 still holds SP_SX, before its MOV #SP_SY,R0 at
     * 0x8c0a443a */
    return sprite_send_facing_at(sprite, entry, attr, sprite->sx, view,
                                 view.z + sprite_depth, sq);
}

/* sprite.h's sprite_draw_3d, but it draws on each step FRAME_DRAWS()
 * names: step 0, and step 1 where the split frame's second picture takes
 * that step's draws */
void sprite_draw_3d(NJS_SPRITE *sprite, Int entry, Uint32 attr)
{
    Uint32 list;
    struct sprite_vertex *sq;
    struct sprite_vertex *end;

    if (!FRAME_DRAWS()) {
        return;
    }
    present_untimed(TRUE);
    present_record(PRESENT_SPRITE, sprite, 0, entry, 0.0F, attr);
    njGetkmPointer();
    list = (Uint32)(attr & NJD_SPRITE_ALPHA ? _nj_parameterkm_.trans_polygon
                                            : _nj_parameterkm_.opaque_polygon);
    /* the sends' PREFs must follow these writes. sprite_send_corner's asm
     * names no memory, so GCC could move it above them in a send it inlined.
     * sprite_send_facing and sprite_send_plane are noinline, and a volatile
     * write is done before the call after it */
    QACR1 = SQ_AREA(list);
    QACR0 = SQ_AREA(list);
    /* the first vertex, past the header's room */
    sq = (struct sprite_vertex *)((list & SQ_ADDRESS) | SQ_BASE) + 1;
    if (attr & NJD_SPRITE_SCALE) {
        end = sprite_send_facing(sprite, entry, attr, sq);
    } else {
        end = sprite_send_plane(sprite, entry, attr, sq);
    }
    if (end != NULL) {
        (void)sprite_begin(sprite, entry, attr);
        njEndVertex((Uint32 *)end);
    }
    njSetkmPointer();
    present_untimed(FALSE);
}
