/* SA1's live draws on a host. SA1's compatibility layer, src/draw/model.c,
 * includes this file on a host, and engine/dc/sa1/live.h on the Dreamcast.
 * a host's backend reads what a draw borrows until the picture retires. so
 * live.c defines sa_present_live out of line, and takes the draw at its
 * call */
#ifndef SA1_LIVE_H
#define SA1_LIVE_H

#include <ninja.h>

#include "rules.h"
#include "sa_pc.h"
#include "sa_sa1.h"

/* the SH-4's operand cache, which a host lacks. src/draw/model.c still
 * computes the walk's scratch by these numbers */
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

/* the light the models shade by, in view space, and the palette they shade
 * from, within sa1_live_colors. the layer defines both */
extern NJS_VECTOR sa1_live_light;
extern const Uint32 *sa1_live_palette;
/* the palettes as sa_sa1.h lays them out, then the offset colors of an
 * unlit material's points. the layer defines them */
extern Uint32
    sa1_live_colors[SA_PALETTES * SA_PALETTE_ENTRIES * 2 + SA_UNLIT_OFFSETS];

/* an sa_draw_live's state: how the draw goes, SA1_LIVE_* */
struct sa1_live_state {
    Uint32 how;
};

/* a draw's record projects by Ninja's screen, y aspect and near clip as the
 * last call found them. aspect_x is the picture's x aspect. live.c projects
 * no point itself, so scratch goes unread */
void sa1_live_screen(Float aspect_x, void *scratch);
/* start a run of land draws. they take the constant attributes as this call
 * or the last draw in view found them */
void sa1_live_run(void);
/* where on is TRUE, the SA1_LIVE_NESTED draws that follow take model.h's
 * Ninja forms. where on is FALSE, they draw as model.src's
 * draw_model_nested */
void sa1_live_forms(Bool on);
/* forget the Dreamcast walk's bakes of land palette entries, for a land about
 * to load. a host keeps none, as its records name a model by content */
void sa1_live_forget(void);

#endif
