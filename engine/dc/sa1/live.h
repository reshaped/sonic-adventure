/* SA1's live draws on the Dreamcast. SA1's compatibility layer,
 * src/draw/model.c, hands each draw to the model walk as an sa_draw_live of
 * SA_SA1_BASIC. the walk is engine/dc/sa1/model.c. this header declares what
 * the layer lends the walk, and defines sa_present_live inline. the walk
 * reads Ninja's state through these names alone, during the call alone. it
 * leaves that state for the next draw, as src/draw/model.src, the game's own
 * walk, does.
 *
 * the layer lends Ninja's words and the two calls that open and close its
 * context by name. it declares sa1_live_material, sa1_live_open,
 * sa1_live_close and the names of header.h and screen.h as Ninja's own
 * symbols. so the walk reads each word at its address, as model.src does.
 * the link keeps the words' lines off the cache sets of the kernels' literal
 * pools, as PORTS.md says. lent through a pointer each, the words would cost
 * 91 to 393 cycles a draw in a model of the console. a console's own counts
 * still want comparison. the walk and the layer take Ninja's numbers and
 * model.src's from rules.h, which a host's adapter includes too */
#ifndef SA1_LIVE_H
#define SA1_LIVE_H

#include <kamui.h>
#include <ninja.h>

#include "sa1/header.h"

/* the record a texture name's texaddr points to. rules.h's set_header finds
 * a texture's record by it, so it comes before rules.h. on the Dreamcast,
 * texaddr is the record's address */
#define TEXTURE_RECORD(texaddr) ((const NJS_TEXMEMLIST *)(texaddr))

#include "rules.h"
#include "sa1/screen.h"
#include "sa_dc.h"
#include "sa_sa1.h"

/* the SH-4's operand cache holds one line of OC_LINE bytes in each of its
 * OC_SETS sets. bits 5 to 13 of an address give its set. so OC_SIZE bytes
 * in a row take each set once. the layer lays out the walk's scratch and the
 * palettes by it */
#define OC_LINE 32
#define OC_SETS 512
#define OC_SIZE (OC_LINE * OC_SETS)

/* how a draw goes, as model.h's entries take it */
enum sa1_live_how {
    /* draw_model_nested's, in a context its caller opened, in the forms
     * sa1_live_forms last set */
    SA1_LIVE_NESTED,
    /* draw_model's: the view test, then the model between its own opening
     * and closing of Ninja's context */
    SA1_LIVE_MODEL,
    /* draw_model_unculled's, in a run sa1_live_run started */
    SA1_LIVE_LAND
};

/* the constant material, beside header.h's words */
extern NJS_ARGB sa1_live_material;
/* the light the models shade by, in view space, and the palette, which
 * the layer keeps */
extern NJS_VECTOR sa1_live_light;
extern const Uint32 *sa1_live_palette;
/* njGetkmPointer and njSetkmPointer, lent by name. sa1_live_open copies the
 * current context's words and the lists' write pointers into
 * sa1_live_header for a draw. sa1_live_close copies them back */
void sa1_live_open(void);
void sa1_live_close(void);
/* the palettes as sa_sa1.h lays them out, each entry a base color and an
 * offset color. the offset colors of an unlit material's points follow, by
 * point index. the layer defines them */
extern Uint32
    sa1_live_colors[SA_PALETTES * SA_PALETTE_ENTRIES * 2 + SA_UNLIT_OFFSETS];

/* an sa_draw_live's state: how the draw goes, SA1_LIVE_* */
struct sa1_live_state {
    Uint32 how;
};

/* the walk compiles inside the layer's object, so sa1_live_screen,
 * sa1_live_run and sa1_live_forms are static.
 *
 * sa1_live_screen takes Ninja's screen, y aspect and near clip, through
 * screen.h's names, until its next call. the draws project by them and by
 * aspect_x, the picture's x aspect. they project a model's points into
 * scratch, which src/draw/model.c places in _nj_vertex_buf_ */
static void sa1_live_screen(Float aspect_x, void *scratch);
/* start a run of land draws. they take the constant attributes as this call
 * finds them */
static void sa1_live_run(void);
/* where on is TRUE, make the SA1_LIVE_NESTED draws that follow take Ninja's
 * forms, as model.h says. they then draw unlit and translucent. where on is
 * FALSE, they draw as model.src's draw_model_nested */
ALWAYS_INLINE void sa1_live_forms(Bool on);
/* forget the land's bakes, for a land about to load. a bake holds a land
 * model's palette entries under one light, kept across pictures */
void sa1_live_forget(void);

/* the walk's entries, each a draw of SA1_LIVE_* as its name says */
ALWAYS_INLINE void sa1_live_model(const NJS_MODEL *model);
ALWAYS_INLINE void sa1_live_nested(const NJS_MODEL *model);
ALWAYS_INLINE void sa1_live_land(const NJS_MODEL *model);

/* draw a basic model as its state says, consumed in the call. a program
 * compiles against one backend, and the Dreamcast draws within the call. so
 * each call compiles into the walk's entry for its state. the descriptor and
 * its state stay in registers. the draw takes no stack frame or dispatch of
 * its own. out of line, the call would cost 43 to 119 cycles a draw in the
 * same model. the engine goes unread: the machine holds the Dreamcast's
 * state, and the layer lends Ninja's words by name */
ALWAYS_INLINE sa_result sa_present_live(sa_engine *engine,
                                        const sa_draw_live *draw)
{
    const NJS_MODEL *model = draw->geometry;
    Uint32 how;

    (void)engine;
    if (draw->format != SA_SA1_BASIC) {
        return SA_ERR_UNSUPPORTED;
    }
    how = ((const struct sa1_live_state *)draw->state)->how;
    if (how == SA1_LIVE_LAND) {
        sa1_live_land(model);
    } else if (how == SA1_LIVE_MODEL) {
        sa1_live_model(model);
    } else {
        sa1_live_nested(model);
    }
    return SA_OK;
}

#endif
